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
#include "okparser.h"
#include "okjson.h"

#include <QHash>

using namespace OkJson;

namespace {

QString firstNonEmpty(const QVariantMap &o, const char *a, const char *b = 0, const char *c = 0, const char *d = 0)
{
    QString v = str(o, a);
    if (v.isEmpty() && b) v = str(o, b);
    if (v.isEmpty() && c) v = str(o, c);
    if (v.isEmpty() && d) v = str(o, d);
    return v;
}

bool sameType(const QString &a, const char *b)
{
    return a.compare(QLatin1String(b), Qt::CaseInsensitive) == 0;
}

/// The participant uid that is not mine - the person on the other side.
QString otherParticipant(const QVariantMap &o, const QString &myUid)
{
    const QVariantList arr = array(o, "participant");
    for (int i = 0; i < arr.size(); ++i) {
        const QString uid = str(arr.at(i).toMap(), "id");
        if (!uid.isEmpty() && uid != myUid) return uid;
    }
    // Fallback: decode "PRIVATE:<uid>" from the base64 id.
    const QString raw = QString::fromUtf8(QByteArray::fromBase64(str(o, "id").toLatin1()));
    const int colon = raw.indexOf(QLatin1Char(':'));
    if (colon >= 0) {
        const QString rest = raw.mid(colon + 1);
        if (rest != myUid) return rest;
    }
    return QString();
}

QString https(const QString &url)
{
    if (url.startsWith(QLatin1String("http://"), Qt::CaseInsensitive))
        return QLatin1String("https://") + url.mid(7);
    return url;
}

/// The playable url of a voice-note resource: the content_locations entry whose ct is
/// audio/mpeg, or the first entry, or a bare "url" if the shape is flat.
QString audioUrl(const QVariantMap &res)
{
    const QVariantList locs = array(res, "content_locations");
    if (!locs.isEmpty()) {
        for (int i = 0; i < locs.size(); ++i) {
            const QVariantMap l = locs.at(i).toMap();
            if (sameType(str(l, "ct"), "audio/mpeg")) return str(l, "url");
        }
        return str(locs.at(0).toMap(), "url");
    }
    return str(res, "url");
}

QList<OkAttachment> readAttachments(const QVariantList &arr)
{
    QList<OkAttachment> list;
    for (int i = 0; i < arr.size(); ++i) {
        const QVariantMap o = arr.at(i).toMap();
        if (o.isEmpty()) continue;

        OkAttachment a;
        a.type = str(o, "type");
        a.attachId = str(o, "id");
        const QString type = a.type.toUpper();

        if (type == QLatin1String("PHOTO")) {
            // Only the id is here; urls come from getAttachedResources. The pic_* reads are a
            // harmless fallback for any inline url.
            a.previewUrl = firstNonEmpty(o, "pic_128x128", "pic_640x480");
            a.url = firstNonEmpty(o, "pic_640x480");
            if (a.url.isEmpty()) a.url = a.previewUrl;
            a.title = QLatin1String("Photo");
        } else if (type == QLatin1String("MOVIE") || type == QLatin1String("VIDEO")) {
            a.previewUrl = str(o, "thumbnail_url");
            a.url = str(o, "url");
            a.title = str(o, "title");
            if (a.title.isEmpty()) a.title = QLatin1String("Video");
            a.duration = integer(o, "duration");
        } else if (type == QLatin1String("AUDIO_RECORDING")) {
            a.title = QLatin1String("Voice message");
            a.duration = integer(o, "duration");
        } else if (type == QLatin1String("MUSIC") || type == QLatin1String("TRACK")) {
            QString t = str(o, "artist_name") + QLatin1String(" - ") + str(o, "title");
            while (t.startsWith(QLatin1Char(' ')) || t.startsWith(QLatin1Char('-'))) t.remove(0, 1);
            while (t.endsWith(QLatin1Char(' ')) || t.endsWith(QLatin1Char('-'))) t.chop(1);
            a.title = t;
            a.url = str(o, "url");
        } else if (type == QLatin1String("SHARE") || type == QLatin1String("LINK")) {
            a.title = firstNonEmpty(o, "title", "url");
            a.url = str(o, "url");
            a.extra = str(o, "description");
        } else {
            a.title = QLatin1Char('[') + a.type + QLatin1Char(']');
            a.url = str(o, "url");
        }
        list.append(a);
    }
    return list;
}

} // namespace

namespace OkParser {

QDateTime fromMs(qlonglong ms)
{
    if (ms <= 0) return QDateTime();
    QDateTime t = QDateTime::fromMSecsSinceEpoch(ms);
    return t.toUTC();
}

bool readConversation(const QVariantMap &o, const QString &myUid, OkDialog *d)
{
    const QString id = str(o, "id");
    if (id.isEmpty()) return false;

    d->conversationId = id;
    d->isChat = sameType(str(o, "type"), "CHAT");
    d->preview = str(o, "last_msg_text");
    d->unreadCount = int(integer(o, "new_msgs_count"));
    d->lastTime = fromMs(integer(o, "last_msg_time_ms"));
    d->lastAuthorId = str(o, "last_author_id");

    if (d->isChat) {
        d->title = str(o, "topic");
        if (d->title.isEmpty()) d->title = QLatin1String("Group chat");
    } else {
        d->peerUid = otherParticipant(o, myUid);
        // A friendly title is filled in later once the user is resolved.
        d->title = d->peerUid.isEmpty() ? QLatin1String("Conversation") : (QLatin1String("id") + d->peerUid);
    }
    return true;
}

OkMessage readMessage(const QVariantMap &o, const QString &myUid)
{
    OkMessage m;
    m.id = str(o, "id");
    m.authorId = str(o, "author_id");
    m.out = !m.authorId.isEmpty() && m.authorId == myUid;
    m.body = str(o, "text").trimmed();
    m.date = fromMs(integer(o, "date_ms"));
    m.editedMs = qulonglong(integer(o, "edit_time_ms"));
    m.replyToId = str(o, "reply_to_id");
    m.isSystem = sameType(str(o, "type"), "SYSTEM");
    m.attachments = readAttachments(array(o, "attachments"));
    return m;
}

void applyAttachedResources(QList<OkMessage> &messages, const QVariantList &resources)
{
    QHash<QString, QVariantMap> byId;
    for (int i = 0; i < resources.size(); ++i) {
        const QVariantMap o = resources.at(i).toMap();
        const QString id = str(o, "id");
        if (!id.isEmpty()) byId.insert(id, o);
    }

    for (int mi = 0; mi < messages.size(); ++mi) {
        QList<OkAttachment> &atts = messages[mi].attachments;
        for (int ai = 0; ai < atts.size(); ++ai) {
            OkAttachment &a = atts[ai];
            if (byId.isEmpty() || a.attachId.isEmpty() || !byId.contains(a.attachId)) {
                a.url = https(a.url);
                a.previewUrl = https(a.previewUrl);
                continue;
            }
            const QVariantMap res = byId.value(a.attachId);

            if (sameType(a.type, "PHOTO")) {
                QString p = firstNonEmpty(res, "pic640x480", "pic190x190", "pic128x128");
                if (!p.isEmpty()) a.previewUrl = p;
                QString u = firstNonEmpty(res, "pic1024max", "pic640x480");
                a.url = u.isEmpty() ? a.previewUrl : u;
            } else if (sameType(a.type, "MOVIE") || sameType(a.type, "VIDEO")) {
                QString p = str(res, "thumbnail_url");
                if (!p.isEmpty()) a.previewUrl = p;
                QString u = firstNonEmpty(res, "url_mobile", "url_medium", "url_high", "url_low");
                if (u.isEmpty()) u = firstNonEmpty(res, "url_fullhd", "url");
                if (!u.isEmpty()) a.url = u;
                qlonglong dur = integer(res, "duration");
                if (dur > 0) a.duration = dur;
                if (a.title.isEmpty()) { a.title = str(res, "title"); if (a.title.isEmpty()) a.title = QLatin1String("Video"); }
            } else if (sameType(a.type, "AUDIO_RECORDING")) {
                QString u = audioUrl(res);
                if (!u.isEmpty()) a.url = u;
                qlonglong dur = integer(res, "duration");
                if (dur > 0) a.duration = dur;
                a.title = QLatin1String("Voice message");
            } else {
                QString p = str(res, "thumbnail_url");
                if (!p.isEmpty()) a.previewUrl = p;
                QString u = str(res, "url");
                if (!u.isEmpty()) a.url = u;
            }

            // OK hands back playback urls as http; ok.ru serves the same redirect over TLS.
            a.url = https(a.url);
            a.previewUrl = https(a.previewUrl);
        }
    }
}

bool readUser(const QVariantMap &o, OkUser *u)
{
    const QString uid = firstNonEmpty(o, "uid", "id");
    if (uid.isEmpty()) return false;
    u->uid = uid;
    u->firstName = str(o, "first_name");
    u->lastName = str(o, "last_name");
    u->photo = firstNonEmpty(o, "pic128x128", "pic190x190", "pic50x50");
    const QString online = str(o, "online");
    u->online = !online.isEmpty() && !sameType(online, "OFFLINE");
    return true;
}

} // namespace OkParser
