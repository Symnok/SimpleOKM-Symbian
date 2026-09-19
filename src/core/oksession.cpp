// SimpleOKM-Symbian - an Odnoklassniki messaging client for Symbian Anna/Belle.
// Copyright (C) 2026
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; see the LICENSE file. If not, see <https://www.gnu.org/licenses/>.
#include "oksession.h"
#include "oksession_p.h"
#include "okconstants.h"
#include "okjson.h"
#include "okparser.h"

#include <QTimer>
#include <QUuid>
#include <QDebug>
#include <algorithm>

using namespace OkJson;

namespace {

QString newUuid()
{
    return QUuid::createUuid().toString().remove(QLatin1Char('{')).remove(QLatin1Char('}')).remove(QLatin1Char('-'));
}

bool userNameLess(const OkUser &a, const OkUser &b)
{
    return QString::compare(a.displayName(), b.displayName(), Qt::CaseInsensitive) < 0;
}

}

// -- OkSession ---------------------------------------------------------------------------

OkSession::OkSession(OkApi *api, QObject *parent)
    : QObject(parent), m_api(api), m_renew(0), m_loginInFlight(false)
{
}

void OkSession::reset()
{
    m_creds = OkCredentials();
    m_users.clear();
    m_dialogs.clear();
    m_renew = 0;
    m_renewWaiters.clear();
    emit dialogsChanged();
}

// -- logging in --

void OkSession::loginByPassword(const QString &login, const QString &password, const QString &verificationToken)
{
    OkParams p;
    p.insert(QLatin1String(OkConstants::ParamApplicationKey), QLatin1String(OkConstants::ApplicationKey));
    p.insert(QLatin1String(OkConstants::ParamUserName), login);
    p.insert(QLatin1String(OkConstants::ParamPassword), password);
    p.insert(QLatin1String(OkConstants::ParamGenToken), QLatin1String("true"));
    p.insert(QLatin1String(OkConstants::ParamVerificationSupported), QLatin1String("1"));
    p.insert(QLatin1String(OkConstants::ParamFormat), QLatin1String("json"));
    if (!verificationToken.isEmpty())
        p.insert(QLatin1String(OkConstants::ParamVerificationToken), verificationToken);

    m_loginInFlight = true;
    OkReply *r = m_api->call(QLatin1String(OkConstants::LoginByPassword), p,
                             QLatin1String(OkConstants::ApplicationSecret), QLatin1String(OkConstants::ApiBase));
    connect(r, SIGNAL(finished(OkReply*)), this, SLOT(onLoginReply(OkReply*)));
}

void OkSession::loginByToken(const QString &token)
{
    OkParams p;
    p.insert(QLatin1String(OkConstants::ParamApplicationKey), QLatin1String(OkConstants::ApplicationKey));
    p.insert(QLatin1String(OkConstants::ParamToken), token);
    p.insert(QLatin1String(OkConstants::ParamVerificationSupported), QLatin1String("1"));
    p.insert(QLatin1String(OkConstants::ParamFormat), QLatin1String("json"));

    m_creds.token = token;
    m_loginInFlight = true;
    OkReply *r = m_api->call(QLatin1String(OkConstants::LoginByToken), p,
                             QLatin1String(OkConstants::ApplicationSecret), QLatin1String(OkConstants::ApiBase));
    connect(r, SIGNAL(finished(OkReply*)), this, SLOT(onLoginReply(OkReply*)));
}

bool OkSession::absorbLogin(const QVariantMap &res, QString *verificationUrl)
{
    const QString verify = str(res, "verification_url");
    if (!verify.isEmpty()) { *verificationUrl = verify; return false; }

    const QString token = str(res, "auth_token");
    if (!token.isEmpty()) m_creds.token = token;
    QString v;
    if (!(v = str(res, "session_key")).isEmpty()) m_creds.sessionKey = v;
    if (!(v = str(res, "session_secret_key")).isEmpty()) m_creds.sessionSecret = v;
    if (!(v = str(res, "api_server")).isEmpty()) m_creds.apiServer = v;
    if (!(v = str(res, "uid")).isEmpty()) m_creds.uid = v;
    return m_creds.hasSession();
}

void OkSession::onLoginReply(OkReply *reply)
{
    m_loginInFlight = false;
    if (!reply->ok()) {
        emit loginFinished(false, reply->errorMessage(), QString());
        return;
    }
    QString verify;
    if (!absorbLogin(reply->result(), &verify)) {
        emit loginFinished(false, verify.isEmpty() ? tr("login returned no session") : QString(), verify);
        return;
    }
    emit credentialsChanged();
    // Resolve the signed-in user in the background so "me" is there for the About screen.
    resolveUsers(QStringList() << m_creds.uid);
    emit loginFinished(true, QString(), QString());
}

// -- the signed-call plumbing --

OkReply *OkSession::signedCall(const QString &methodPath, const OkParams &extra)
{
    OkParams p;
    p.insert(QLatin1String(OkConstants::ParamApplicationKey), QLatin1String(OkConstants::ApplicationKey));
    p.insert(QLatin1String(OkConstants::ParamSessionKey), m_creds.sessionKey);
    p.insert(QLatin1String(OkConstants::ParamFormat), QLatin1String("json"));
    for (OkParams::const_iterator it = extra.constBegin(); it != extra.constEnd(); ++it)
        p.insert(it.key(), it.value());
    return m_api->call(methodPath, p, m_creds.sessionSecret, m_creds.apiServer);
}

OkSessionCall *OkSession::sessionCall(const QString &methodPath, const OkParams &extra)
{
    OkSessionCall *c = new OkSessionCall(this, methodPath, extra);
    c->start();
    return c;
}

/// Replaces the session whose key is deadKey by logging in with the auth token again.
/// Several pollers usually notice an expired session within the same second, so they all
/// wait for the same login; a caller that arrives after the renewal already happened (the
/// key on file is no longer the one it failed with) just retries.
void OkSession::renewSession(const QString &deadKey, OkSessionCall *waiter)
{
    if (m_creds.sessionKey != deadKey && !m_renew) {
        waiter->retry();
        return;
    }
    m_renewWaiters.append(waiter);
    if (m_renew) return;

    OkParams p;
    p.insert(QLatin1String(OkConstants::ParamApplicationKey), QLatin1String(OkConstants::ApplicationKey));
    p.insert(QLatin1String(OkConstants::ParamToken), m_creds.token);
    p.insert(QLatin1String(OkConstants::ParamVerificationSupported), QLatin1String("1"));
    p.insert(QLatin1String(OkConstants::ParamFormat), QLatin1String("json"));
    m_renew = m_api->call(QLatin1String(OkConstants::LoginByToken), p,
                          QLatin1String(OkConstants::ApplicationSecret), QLatin1String(OkConstants::ApiBase));
    connect(m_renew, SIGNAL(finished(OkReply*)), this, SLOT(onRenewReply(OkReply*)));
}

void OkSession::onRenewReply(OkReply *reply)
{
    m_renew = 0;
    QList<OkSessionCall *> waiters = m_renewWaiters;
    m_renewWaiters.clear();

    QString verify;
    const bool ok = reply->ok() && absorbLogin(reply->result(), &verify);
    if (ok) {
        emit credentialsChanged();
        for (int i = 0; i < waiters.size(); ++i) waiters.at(i)->retry();
    } else {
        const QString why = reply->ok() ? tr("session renewal needs verification") : reply->errorMessage();
        for (int i = 0; i < waiters.size(); ++i) waiters.at(i)->failNow(why);
        emit sessionLost(why);
    }
}

// -- users --

OkSessionCall *OkSession::resolveUsers(const QStringList &uids)
{
    QStringList need;
    for (int i = 0; i < uids.size(); ++i) {
        const QString u = uids.at(i);
        if (!u.isEmpty() && !m_users.contains(u) && !need.contains(u)) need.append(u);
    }
    if (need.isEmpty()) return 0;

    OkParams p;
    p.insert(QLatin1String("fields"), QLatin1String(OkConstants::UserFields));
    p.insert(QLatin1String("emptyPictures"), QLatin1String("false"));
    p.insert(QLatin1String("uids"), need.join(QLatin1String(",")));
    OkSessionCall *c = sessionCall(QLatin1String(OkConstants::UsersGetInfo), p);
    // Connected first, so the cache is filled before any later-connected slot runs.
    connect(c, SIGNAL(finished(OkSessionCall*)), this, SLOT(onUsersResolved(OkSessionCall*)));
    return c;
}

void OkSession::onUsersResolved(OkSessionCall *call)
{
    if (!call->ok()) return;
    QVariantList arr = array(call->result(), "items");
    if (arr.isEmpty()) arr = array(call->result(), "users");
    for (int i = 0; i < arr.size(); ++i) {
        OkUser u;
        if (OkParser::readUser(arr.at(i).toMap(), &u)) m_users.insert(u.uid, u);
    }
}

// -- conversations --

int OkSession::dialogIndex(const QString &conversationId) const
{
    for (int i = 0; i < m_dialogs.size(); ++i)
        if (m_dialogs.at(i).conversationId == conversationId) return i;
    return -1;
}

QString OkSession::privateConversationId(const QString &uid)
{
    return QString::fromLatin1((QLatin1String("PRIVATE:") + uid).toUtf8().toBase64());
}

QString OkSession::openPrivateConversation(const OkUser &user)
{
    const QString id = privateConversationId(user.uid);
    if (dialogIndex(id) >= 0) return id;

    OkDialog d;
    d.conversationId = id;
    d.peerUid = user.uid;
    d.user = user;
    d.title = user.displayName();
    d.lastTime = QDateTime::currentDateTime().toUTC();
    d.isDraft = true;
    m_users.insert(user.uid, user);
    m_dialogs.append(d);
    emit dialogsChanged();
    return id;
}

void OkSession::refreshConversations(int count)
{
    new RefreshConversationsOp(this, count);
}

/// Merges a fresh getList result into the existing list rather than replacing it, so the
/// entry the UI has open survives a background refresh: fields are updated in place,
/// genuinely new conversations are appended, ones no longer present are removed. Drafts
/// (a "New chat" nothing has been sent in yet) are not on the server's list and survive.
void OkSession::mergeDialogs(const QList<OkDialog> &fresh)
{
    QSet<QString> live;
    for (int i = 0; i < fresh.size(); ++i) {
        OkDialog f = fresh.at(i);
        if (!f.isChat && !f.peerUid.isEmpty() && m_users.contains(f.peerUid)) {
            f.user = m_users.value(f.peerUid);
            f.title = f.user.displayName();
        }
        live.insert(f.conversationId);

        const int idx = dialogIndex(f.conversationId);
        if (idx >= 0) {
            OkDialog &e = m_dialogs[idx];
            e.title = f.title;
            e.preview = f.preview;
            e.unreadCount = f.unreadCount;
            e.lastTime = f.lastTime;
            e.lastAuthorId = f.lastAuthorId;
            e.isChat = f.isChat;
            if (f.hasUser()) e.user = f.user;
            e.isDraft = false;   // the server lists it now: a real conversation
        } else {
            m_dialogs.append(f);
        }
    }
    for (int i = m_dialogs.size() - 1; i >= 0; --i)
        if (!m_dialogs.at(i).isDraft && !live.contains(m_dialogs.at(i).conversationId))
            m_dialogs.removeAt(i);
    emit dialogsChanged();
}

// -- history, sending, the rest --

void OkSession::loadHistory(const QString &conversationId, const QString &anchor, int count)
{
    new LoadHistoryOp(this, conversationId, anchor, count);
}

void OkSession::sendMessage(const QString &conversationId, const QString &text, const QString &replyToId, const QString &localId)
{
    new SendMessageOp(this, conversationId, text, replyToId, QStringList(), localId);
}

void OkSession::sendPhoto(const QString &conversationId, const QByteArray &imageBytes, const QString &caption,
                          const QString &contentType, const QString &localId)
{
    new SendPhotoOp(this, conversationId, imageBytes, caption, contentType, localId);
}

void OkSession::loadFriends()
{
    new FriendsOp(this);
}

void OkSession::deleteMessages(const QString &conversationId, const QStringList &messageIds)
{
    new SimpleOp(this, SimpleOp::DeleteMessages, conversationId, messageIds);
}

void OkSession::deleteConversation(const QString &conversationId)
{
    new SimpleOp(this, SimpleOp::DeleteConversation, conversationId, QStringList());
}

// -- OkSessionCall -----------------------------------------------------------------------

OkSessionCall::OkSessionCall(OkSession *session, const QString &methodPath, const OkParams &extra)
    : QObject(session), m_session(session), m_method(methodPath), m_extra(extra),
      m_retried(false), m_ok(false), m_code(-1)
{
}

void OkSessionCall::start()
{
    if (!m_session->isLoggedIn()) {
        // Report asynchronously so callers can connect first.
        QTimer::singleShot(0, this, SLOT(onNotLoggedIn()));
        return;
    }
    m_keyUsed = m_session->m_creds.sessionKey;
    OkReply *r = m_session->signedCall(m_method, m_extra);
    connect(r, SIGNAL(finished(OkReply*)), this, SLOT(onReply(OkReply*)));
}

void OkSessionCall::retry()
{
    m_keyUsed = m_session->m_creds.sessionKey;
    OkReply *r = m_session->signedCall(m_method, m_extra);
    connect(r, SIGNAL(finished(OkReply*)), this, SLOT(onReply(OkReply*)));
}

void OkSessionCall::failNow(const QString &message)
{
    m_ok = false;
    m_code = -1;
    m_message = message;
    emit finished(this);
    deleteLater();
}

void OkSessionCall::onNotLoggedIn()
{
    failNow(tr("not logged in"));
}

void OkSessionCall::onReply(OkReply *reply)
{

    if (!reply->ok() && reply->isSessionError() && !m_retried && !m_session->m_creds.token.isEmpty()) {
        m_retried = true;
        m_session->renewSession(m_keyUsed, this);
        return;
    }
    m_ok = reply->ok();
    m_code = reply->errorCode();
    m_message = reply->errorMessage();
    m_result = reply->result();
    emit finished(this);
    deleteLater();
}

// -- RefreshConversationsOp --------------------------------------------------------------

RefreshConversationsOp::RefreshConversationsOp(OkSession *s, int count)
    : QObject(s), m_s(s)
{
    OkParams p;
    p.insert(QLatin1String("fields"), QLatin1String("conversation.*"));
    p.insert(QLatin1String("count"), QString::number(count));
    OkSessionCall *c = m_s->sessionCall(QLatin1String(OkConstants::MessagesGetList), p);
    connect(c, SIGNAL(finished(OkSessionCall*)), this, SLOT(onList(OkSessionCall*)));
}

void RefreshConversationsOp::onList(OkSessionCall *call)
{
    if (!call->ok()) {
        emit m_s->conversationsRefreshed(false, call->errorMessage());
        deleteLater();
        return;
    }
    const QVariantList arr = array(call->result(), "conversation");
    QStringList peers;
    for (int i = 0; i < arr.size(); ++i) {
        OkDialog d;
        if (!OkParser::readConversation(arr.at(i).toMap(), m_s->m_creds.uid, &d)) continue;
        m_parsed.append(d);
        if (!d.isChat && !d.peerUid.isEmpty()) peers.append(d.peerUid);
    }
    // Names come from users/getInfo - the conversation only carries ids. The signed-in user
    // is resolved in the same call when still unknown.
    peers.append(m_s->m_creds.uid);
    OkSessionCall *u = m_s->resolveUsers(peers);
    if (u) connect(u, SIGNAL(finished(OkSessionCall*)), this, SLOT(onUsers(OkSessionCall*)));
    else finish();
}

void RefreshConversationsOp::onUsers(OkSessionCall *)
{
    // Even if getInfo failed the list is still useful (with id-titles); don't fail the refresh.
    finish();
}

void RefreshConversationsOp::finish()
{
    m_s->mergeDialogs(m_parsed);
    emit m_s->conversationsRefreshed(true, QString());
    deleteLater();
}

// -- LoadHistoryOp -----------------------------------------------------------------------

LoadHistoryOp::LoadHistoryOp(OkSession *s, const QString &conversationId, const QString &anchor, int count)
    : QObject(s), m_s(s), m_conv(conversationId), m_anchor(anchor)
{
    // Two cursors, from the official client's message sources: the latest page is
    // direction=Forward, anchor="U" (around the unread position, i.e. the most recent
    // messages); an older page is direction=Backward with the anchor a previous page returned.
    const bool latest = anchor.isEmpty();
    QVariantMap pars;
    pars.insert(QLatin1String("cnv_id"), conversationId);
    pars.insert(QLatin1String("direction"), QLatin1String(latest ? "Forward" : "Backward"));
    pars.insert(QLatin1String("anchor"), latest ? QString::fromLatin1("U") : anchor);
    pars.insert(QLatin1String("frmt"), QLatin1String("PLAIN_EXT_SMILES"));
    pars.insert(QLatin1String("count"), QString::number(count));
    pars.insert(QLatin1String("mark_as_read"), QLatin1String("true"));
    pars.insert(QLatin1String("fields"), QLatin1String(OkConstants::MessageFields));

    // getMessages returns each photo attachment as just an id; the picture urls come from
    // getAttachedResources, which takes those ids via the batch supplier. So the read is one
    // batch/execute - exactly what the official client does.
    QVariantMap getMessages;
    getMessages.insert(QLatin1String("method"), QLatin1String("messagesV2.getMessages"));
    getMessages.insert(QLatin1String("params"), pars);

    QVariantMap supplier;
    supplier.insert(QLatin1String("supplier"), QLatin1String("messagesV2.getMessages.attachment_ids"));
    QVariantMap resPars;
    resPars.insert(QLatin1String("attach_ids"), supplier);
    resPars.insert(QLatin1String("fields"), QLatin1String(OkConstants::AttachmentResourceFields));
    QVariantMap getResources;
    getResources.insert(QLatin1String("method"), QLatin1String("messagesV2.getAttachedResources"));
    getResources.insert(QLatin1String("params"), resPars);

    QVariantList methods;
    methods << getMessages << getResources;

    OkParams p;
    p.insert(QLatin1String("methods"), QString::fromUtf8(OkJson::serialize(methods)));
    OkSessionCall *c = m_s->sessionCall(QLatin1String(OkConstants::BatchExecute), p);
    connect(c, SIGNAL(finished(OkSessionCall*)), this, SLOT(onBatch(OkSessionCall*)));
}

void LoadHistoryOp::onBatch(OkSessionCall *call)
{
    if (!call->ok()) {
        emit m_s->historyFailed(m_conv, m_anchor, call->errorMessage());
        deleteLater();
        return;
    }
    const QVariantMap res = object(call->result(), "messagesV2_getMessages_response");
    m_page.anchor = str(res, "anchor");
    m_page.hasMore = boolean(res, "has_more", false);

    const QVariantList arr = array(res, "message");
    QStringList authors;
    for (int i = 0; i < arr.size(); ++i) {
        OkMessage m = OkParser::readMessage(arr.at(i).toMap(), m_s->m_creds.uid);
        m.conversationId = m_conv;
        m_page.messages.append(m);
        if (!m.authorId.isEmpty() && !authors.contains(m.authorId)) authors.append(m.authorId);
    }
    if (arr.isEmpty()) { finish(); return; }

    // Fill in photo urls from the paired getAttachedResources reply.
    const QVariantList resources = array(object(call->result(), "messagesV2_getAttachedResources_response"), "attachments");
    OkParser::applyAttachedResources(m_page.messages, resources);

    OkSessionCall *u = m_s->resolveUsers(authors);
    if (u) connect(u, SIGNAL(finished(OkSessionCall*)), this, SLOT(onUsers(OkSessionCall*)));
    else finish();
}

void LoadHistoryOp::onUsers(OkSessionCall *)
{
    finish();
}

void LoadHistoryOp::finish()
{
    for (int i = 0; i < m_page.messages.size(); ++i) {
        OkMessage &m = m_page.messages[i];
        if (m_s->m_users.contains(m.authorId)) m.senderName = m_s->m_users.value(m.authorId).displayName();
    }
    emit m_s->historyLoaded(m_conv, m_anchor, m_page);
    deleteLater();
}

// -- SendMessageOp -----------------------------------------------------------------------

SendMessageOp::SendMessageOp(OkSession *s, const QString &conversationId, const QString &text, const QString &replyToId,
                             const QStringList &photoTokens, const QString &localId)
    : QObject(s), m_s(s), m_conv(conversationId), m_localId(localId)
{
    OkParams p;
    p.insert(QLatin1String("cnv_id"), conversationId);
    p.insert(QLatin1String("text"), text);
    p.insert(QLatin1String("uuid"), newUuid());
    if (!replyToId.isEmpty()) p.insert(QLatin1String("reply_to_message_id"), replyToId);

    // attachments = {"attachments":[{"type":"UPLOADED_PHOTO","token":"..."}]} - the shape
    // SendConversationMessagesBatchable.BuildAttachmentsJson produces.
    if (!photoTokens.isEmpty()) {
        QVariantList atts;
        for (int i = 0; i < photoTokens.size(); ++i) {
            QVariantMap a;
            a.insert(QLatin1String("type"), QLatin1String("UPLOADED_PHOTO"));
            a.insert(QLatin1String("token"), photoTokens.at(i));
            atts.append(a);
        }
        QVariantMap wrap;
        wrap.insert(QLatin1String("attachments"), atts);
        p.insert(QLatin1String("attachments"), QString::fromUtf8(OkJson::serialize(wrap)));
    }

    OkSessionCall *c = m_s->sessionCall(QLatin1String(OkConstants::MessagesSend), p);
    connect(c, SIGNAL(finished(OkSessionCall*)), this, SLOT(onSent(OkSessionCall*)));
}

void SendMessageOp::onSent(OkSessionCall *call)
{
    if (!call->ok()) {
        emit m_s->sendFailed(m_conv, m_localId, call->errorMessage());
    } else {
        QString id = str(call->result(), "message_id");
        if (id.isEmpty()) id = str(call->result(), "id");
        emit m_s->messageSent(m_conv, m_localId, id);
    }
    deleteLater();
}

// -- SendPhotoOp -------------------------------------------------------------------------

SendPhotoOp::SendPhotoOp(OkSession *s, const QString &conversationId, const QByteArray &bytes, const QString &caption,
                         const QString &contentType, const QString &localId)
    : QObject(s), m_s(s), m_conv(conversationId), m_bytes(bytes), m_caption(caption),
      m_contentType(contentType), m_localId(localId)
{
    if (m_bytes.isEmpty()) {
        QTimer::singleShot(0, this, SLOT(onNoData()));
        return;
    }
    OkSessionCall *c = m_s->sessionCall(QLatin1String(OkConstants::PhotosGetUploadUrl), OkParams());
    connect(c, SIGNAL(finished(OkSessionCall*)), this, SLOT(onUploadUrl(OkSessionCall*)));
}

void SendPhotoOp::fail(const QString &why)
{
    emit m_s->sendFailed(m_conv, m_localId, why);
    deleteLater();
}

void SendPhotoOp::onNoData()
{
    fail(tr("no image data"));
}

void SendPhotoOp::onUploadUrl(OkSessionCall *call)
{
    if (!call->ok()) { fail(call->errorMessage()); return; }
    const QString uploadUrl = str(call->result(), "upload_url");
    if (uploadUrl.isEmpty()) { fail(tr("getUploadUrl returned no upload_url")); return; }

    OkReply *r = m_s->m_api->uploadFile(uploadUrl, m_bytes, m_contentType,
                                        QLatin1String("pic1"), QLatin1String("photo.jpg"), true);
    connect(r, SIGNAL(finished(OkReply*)), this, SLOT(onUploaded(OkReply*)));
}

void SendPhotoOp::onUploaded(OkReply *reply)
{
    if (!reply->ok()) { fail(reply->errorMessage()); return; }
    m_bytes.clear();

    // {"photos":{"<photoId>":{"token":"..."}}} - take the first token.
    const QVariantMap photos = object(reply->result(), "photos");
    QString token;
    for (QVariantMap::const_iterator it = photos.constBegin(); it != photos.constEnd() && token.isEmpty(); ++it)
        token = str(it.value().toMap(), "token");
    if (token.isEmpty()) { fail(tr("upload returned no photo token")); return; }

    new SendMessageOp(m_s, m_conv, m_caption, QString(), QStringList() << token, m_localId);
    deleteLater();
}

// -- FriendsOp ---------------------------------------------------------------------------

FriendsOp::FriendsOp(OkSession *s)
    : QObject(s), m_s(s)
{
    OkSessionCall *c = m_s->sessionCall(QLatin1String(OkConstants::FriendsGet), OkParams());
    connect(c, SIGNAL(finished(OkSessionCall*)), this, SLOT(onFriends(OkSessionCall*)));
}

void FriendsOp::onFriends(OkSessionCall *call)
{
    if (!call->ok()) {
        emit m_s->friendsLoaded(false, QList<OkUser>(), call->errorMessage());
        deleteLater();
        return;
    }
    const QVariantList arr = array(call->result(), "items");
    for (int i = 0; i < arr.size(); ++i) {
        const QString u = arr.at(i).toString();
        if (!u.isEmpty()) m_uids.append(u);
    }
    OkSessionCall *u = m_s->resolveUsers(m_uids);
    if (u) connect(u, SIGNAL(finished(OkSessionCall*)), this, SLOT(onUsers(OkSessionCall*)));
    else finish();
}

void FriendsOp::onUsers(OkSessionCall *call)
{
    if (!call->ok()) {
        emit m_s->friendsLoaded(false, QList<OkUser>(), call->errorMessage());
        deleteLater();
        return;
    }
    finish();
}

void FriendsOp::finish()
{
    QList<OkUser> list;
    for (int i = 0; i < m_uids.size(); ++i)
        if (m_s->m_users.contains(m_uids.at(i))) list.append(m_s->m_users.value(m_uids.at(i)));
    std::sort(list.begin(), list.end(), userNameLess);
    emit m_s->friendsLoaded(true, list, QString());
    deleteLater();
}

// -- SimpleOp ----------------------------------------------------------------------------

SimpleOp::SimpleOp(OkSession *s, Kind kind, const QString &conversationId, const QStringList &ids)
    : QObject(s), m_s(s), m_kind(kind), m_conv(conversationId), m_ids(ids)
{
    OkParams p;
    p.insert(QLatin1String("cnv_id"), conversationId);
    const char *method = OkConstants::ConversationDelete;
    if (kind == DeleteMessages) {
        p.insert(QLatin1String("msg_ids"), ids.join(QLatin1String(",")));
        method = OkConstants::MessagesDelete;
    }
    OkSessionCall *c = m_s->sessionCall(QLatin1String(method), p);
    connect(c, SIGNAL(finished(OkSessionCall*)), this, SLOT(onDone(OkSessionCall*)));
}

void SimpleOp::onDone(OkSessionCall *call)
{
    if (m_kind == DeleteMessages) {
        emit m_s->messagesDeleted(m_conv, m_ids, call->ok(), call->errorMessage());
    } else {
        if (call->ok()) {
            const int idx = m_s->dialogIndex(m_conv);
            if (idx >= 0) { m_s->m_dialogs.removeAt(idx); emit m_s->dialogsChanged(); }
        }
        emit m_s->conversationDeleted(m_conv, call->ok(), call->errorMessage());
    }
    deleteLater();
}
