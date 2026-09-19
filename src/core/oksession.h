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
//
// The session: login, and the calls a UI needs on top of it - the same surface as
// SimpleOKM.Core's OkSession, expressed as start-methods and completion signals since there
// is no async/await in Qt 4.7 C++. Each operation is a small QObject (oksession_p.h) that
// walks its steps in slots and reports through the session's signals.
#ifndef OKSESSION_H
#define OKSESSION_H

#include "okapi.h"
#include "okmodels.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>
#include <QStringList>

class OkSessionCall;

struct OkCredentials
{
    QString token;
    QString sessionKey;
    QString sessionSecret;
    QString apiServer;
    QString uid;

    bool hasSession() const { return !sessionKey.isEmpty() && !sessionSecret.isEmpty(); }
};

class OkSession : public QObject
{
    Q_OBJECT
public:
    explicit OkSession(OkApi *api, QObject *parent = 0);

    OkApi *api() const { return m_api; }
    const OkCredentials &credentials() const { return m_creds; }
    bool isLoggedIn() const { return m_creds.hasSession(); }

    /// Restores a saved token before loginByToken; nothing else is needed from storage.
    void setToken(const QString &token) { m_creds.token = token; }

    /// Forgets the session and everything cached (sign-out).
    void reset();

    // -- users (cached from users/getInfo) --
    bool hasUser(const QString &uid) const { return m_users.contains(uid); }
    OkUser user(const QString &uid) const { return m_users.value(uid); }
    /// The signed-in user, once resolved (uid empty until then).
    OkUser me() const { return m_users.value(m_creds.uid); }

    // -- the conversation list, owned here --
    const QList<OkDialog> &dialogs() const { return m_dialogs; }
    int dialogIndex(const QString &conversationId) const;
    /// The conversation id OK uses for a one-to-one with uid: base64 of "PRIVATE:uid".
    static QString privateConversationId(const QString &uid);
    /// The dialog for a one-to-one with user: the existing entry, or a new draft appended to
    /// the list (dialogsChanged is emitted) so the UI can open it and type.
    QString openPrivateConversation(const OkUser &user);

    // -- operations; each ends in the matching signal below --
    void loginByPassword(const QString &login, const QString &password, const QString &verificationToken = QString());
    void loginByToken(const QString &token);
    void refreshConversations(int count = 100);
    /// anchor empty = the latest page; otherwise the page before that anchor (load earlier).
    void loadHistory(const QString &conversationId, const QString &anchor = QString(), int count = 30);
    /// localId is the UI's temporary id for the bubble, echoed back in messageSent/sendFailed.
    void sendMessage(const QString &conversationId, const QString &text, const QString &replyToId, const QString &localId);
    void sendPhoto(const QString &conversationId, const QByteArray &imageBytes, const QString &caption,
                   const QString &contentType, const QString &localId);
    void loadFriends();
    void deleteMessages(const QString &conversationId, const QStringList &messageIds);
    void deleteConversation(const QString &conversationId);

    /// A signed call on the current session with the expired-session retry built in. Used
    /// by the operations; exposed for diagnostics.
    OkSessionCall *sessionCall(const QString &methodPath, const OkParams &extra);

    /// users/getInfo for the uids not yet cached. Returns 0 when nothing needs fetching;
    /// otherwise the cache is filled before the returned call's finished() reaches the
    /// caller's slot.
    OkSessionCall *resolveUsers(const QStringList &uids);

signals:
    void loginFinished(bool ok, const QString &error, const QString &verificationUrl);
    /// The token or session changed (login, or a silent renewal) - persist the token.
    void credentialsChanged();
    /// A silent re-login with the saved token failed; the UI should go back to sign-in.
    void sessionLost(const QString &error);

    void dialogsChanged();
    void conversationsRefreshed(bool ok, const QString &error);

    void historyLoaded(const QString &conversationId, const QString &requestAnchor, const OkHistoryPage &page);
    void historyFailed(const QString &conversationId, const QString &requestAnchor, const QString &error);

    void messageSent(const QString &conversationId, const QString &localId, const QString &serverId);
    void sendFailed(const QString &conversationId, const QString &localId, const QString &error);

    void friendsLoaded(bool ok, const QList<OkUser> &friends, const QString &error);
    void messagesDeleted(const QString &conversationId, const QStringList &messageIds, bool ok, const QString &error);
    void conversationDeleted(const QString &conversationId, bool ok, const QString &error);

private slots:
    void onLoginReply(OkReply *reply);
    void onRenewReply(OkReply *reply);
    void onUsersResolved(OkSessionCall *call);

private:
    friend class OkSessionCall;
    friend class RefreshConversationsOp;
    friend class LoadHistoryOp;
    friend class SendMessageOp;
    friend class SendPhotoOp;
    friend class FriendsOp;
    friend class SimpleOp;

    OkReply *signedCall(const QString &methodPath, const OkParams &extra);
    bool absorbLogin(const QVariantMap &res, QString *verificationUrl);
    void renewSession(const QString &deadKey, OkSessionCall *waiter);
    void mergeDialogs(const QList<OkDialog> &fresh);

    OkApi *m_api;
    OkCredentials m_creds;
    QHash<QString, OkUser> m_users;
    QList<OkDialog> m_dialogs;

    OkReply *m_renew;                 // the one in-flight token re-login
    QList<OkSessionCall *> m_renewWaiters;
    bool m_loginInFlight;
};

/// One signed call with the session-expiry retry: if the server answers 102/103 and there is
/// an auth token, the session is renewed once (shared by every call that hit the same dead
/// session) and the call repeated. Deletes itself after finished().
class OkSessionCall : public QObject
{
    Q_OBJECT
public:
    bool ok() const { return m_ok; }
    int errorCode() const { return m_code; }
    QString errorMessage() const { return m_message; }
    const QVariantMap &result() const { return m_result; }

signals:
    void finished(OkSessionCall *call);

private slots:
    void onReply(OkReply *reply);
    void onNotLoggedIn();

private:
    friend class OkSession;
    OkSessionCall(OkSession *session, const QString &methodPath, const OkParams &extra);
    void start();
    void retry();
    void failNow(const QString &message);

    OkSession *m_session;
    QString m_method;
    OkParams m_extra;
    QString m_keyUsed;
    bool m_retried;
    bool m_ok;
    int m_code;
    QString m_message;
    QVariantMap m_result;
};

#endif // OKSESSION_H
