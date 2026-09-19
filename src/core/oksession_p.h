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
// The multi-step operations behind OkSession, one class per flow. Each is created by the
// session, walks its steps in slots and deletes itself when it has reported.
#ifndef OKSESSION_P_H
#define OKSESSION_P_H

#include "oksession.h"

/// getList -> users/getInfo for the peers -> merge into the session's dialog list.
class RefreshConversationsOp : public QObject
{
    Q_OBJECT
public:
    RefreshConversationsOp(OkSession *s, int count);
private slots:
    void onList(OkSessionCall *call);
    void onUsers(OkSessionCall *call);
private:
    void finish();
    OkSession *m_s;
    QList<OkDialog> m_parsed;
};

/// batch/execute(getMessages + getAttachedResources) -> users/getInfo for the authors.
class LoadHistoryOp : public QObject
{
    Q_OBJECT
public:
    LoadHistoryOp(OkSession *s, const QString &conversationId, const QString &anchor, int count);
private slots:
    void onBatch(OkSessionCall *call);
    void onUsers(OkSessionCall *call);
private:
    void finish();
    OkSession *m_s;
    QString m_conv;
    QString m_anchor;
    OkHistoryPage m_page;
};

/// messagesV2/send, with optional UPLOADED_PHOTO tokens.
class SendMessageOp : public QObject
{
    Q_OBJECT
public:
    SendMessageOp(OkSession *s, const QString &conversationId, const QString &text, const QString &replyToId,
                  const QStringList &photoTokens, const QString &localId);
private slots:
    void onSent(OkSessionCall *call);
private:
    OkSession *m_s;
    QString m_conv;
    QString m_localId;
};

/// photosV2/getUploadUrl -> multipart POST -> send with the returned token.
class SendPhotoOp : public QObject
{
    Q_OBJECT
public:
    SendPhotoOp(OkSession *s, const QString &conversationId, const QByteArray &bytes, const QString &caption,
                const QString &contentType, const QString &localId);
private slots:
    void onNoData();
    void onUploadUrl(OkSessionCall *call);
    void onUploaded(OkReply *reply);
private:
    void fail(const QString &why);
    OkSession *m_s;
    QString m_conv;
    QByteArray m_bytes;
    QString m_caption;
    QString m_contentType;
    QString m_localId;
};

/// friends/get (bare uid array) -> users/getInfo -> sorted list.
class FriendsOp : public QObject
{
    Q_OBJECT
public:
    explicit FriendsOp(OkSession *s);
private slots:
    void onFriends(OkSessionCall *call);
    void onUsers(OkSessionCall *call);
private:
    void finish();
    OkSession *m_s;
    QStringList m_uids;
};

/// The one-call operations: delete messages, delete a conversation.
class SimpleOp : public QObject
{
    Q_OBJECT
public:
    enum Kind { DeleteMessages, DeleteConversation };
    SimpleOp(OkSession *s, Kind kind, const QString &conversationId, const QStringList &ids);
private slots:
    void onDone(OkSessionCall *call);
private:
    OkSession *m_s;
    Kind m_kind;
    QString m_conv;
    QStringList m_ids;
};

#endif // OKSESSION_P_H
