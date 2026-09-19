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
// The model: plain value types, the same shape as SimpleOKM.Core's OkModels. The UI-side
// list models (ConversationsModel, MessagesModel) wrap these for QML; the core never touches
// QObject property change notification, which keeps it cheap on the phone.
#ifndef OKMODELS_H
#define OKMODELS_H

#include <QDateTime>
#include <QList>
#include <QString>

/// A person. OK identifies them by a numeric uid string.
struct OkUser
{
    QString uid;
    QString firstName;
    QString lastName;
    QString photo;
    bool online;

    OkUser() : online(false) {}

    QString displayName() const
    {
        const QString name = (firstName + QLatin1Char(' ') + lastName).trimmed();
        return name.isEmpty() ? (QLatin1String("id") + uid) : name;
    }

    /// The web profile, for "Open in OK".
    QString profileUrl() const { return QLatin1String("https://ok.ru/profile/") + uid; }
};

/// One attachment on a message. OK's types are PHOTO, MOVIE, AUDIO_RECORDING, MUSIC, SHARE.
struct OkAttachment
{
    QString type;
    QString title;
    QString url;
    QString previewUrl;
    QString extra;
    qlonglong duration;   // seconds (or ms - see durationText)

    /// OK's opaque attachment id: the join key between getMessages and getAttachedResources.
    QString attachId;

    OkAttachment() : duration(0) {}

    bool isImage() const { return type.compare(QLatin1String("PHOTO"), Qt::CaseInsensitive) == 0 && !previewUrl.isEmpty(); }
    bool isVideo() const
    {
        return (type.compare(QLatin1String("MOVIE"), Qt::CaseInsensitive) == 0 ||
                type.compare(QLatin1String("VIDEO"), Qt::CaseInsensitive) == 0) && !url.isEmpty();
    }
    bool isAudio() const { return type.compare(QLatin1String("AUDIO_RECORDING"), Qt::CaseInsensitive) == 0 && !url.isEmpty(); }

    /// Media length as m:ss, for a caption under a clip or voice note.
    QString durationText() const
    {
        if (duration <= 0) return QString();
        qlonglong s = duration > 100000 ? duration / 1000 : duration;   // some fields are ms
        return QString::fromLatin1("%1:%2").arg(s / 60).arg(s % 60, 2, 10, QLatin1Char('0'));
    }
};

/// A single message.
struct OkMessage
{
    QString id;
    QString conversationId;
    QString authorId;
    bool out;                 // authored by me
    QDateTime date;           // UTC
    qulonglong editedMs;
    QString replyToId;
    bool isSystem;
    QString senderName;
    QString body;

    // UI-side state for a message the client created locally and has not seen back yet.
    bool isPending;
    bool failed;
    bool isLocal;

    QList<OkAttachment> attachments;

    OkMessage() : out(false), editedMs(0), isSystem(false), isPending(false), failed(false), isLocal(false) {}

    bool edited() const { return editedMs > 0; }
    bool canDelete() const { return out; }
};

/// A conversation in the list.
struct OkDialog
{
    /// The base64 conversation id - "PRIVATE:uid" or "CHAT:a:b" encoded.
    QString conversationId;
    bool isChat;              // group chat (type == "CHAT")
    QString peerUid;          // for a one-to-one, the other person's uid
    bool isDraft;             // started locally with "New chat", not yet on the server's list
    QDateTime lastTime;       // UTC
    QString title;
    QString preview;
    int unreadCount;
    QString lastAuthorId;
    OkUser user;              // the peer, once resolved (uid empty until then)

    OkDialog() : isChat(false), isDraft(false), unreadCount(0) {}
    bool hasUser() const { return !user.uid.isEmpty(); }
};

/// One page of history: the messages, plus how to ask for the page before it.
struct OkHistoryPage
{
    QList<OkMessage> messages;
    QString anchor;
    bool hasMore;
    OkHistoryPage() : hasMore(false) {}
};

#endif // OKMODELS_H
