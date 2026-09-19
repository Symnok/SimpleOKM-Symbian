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
// The open conversation for QML: its messages oldest-first, plus the chat-side behaviour
// the WP ChatPage has - initial load, "load earlier", a poll that merges new messages in
// place, optimistic bubbles for what is being sent, reply, delete.
#ifndef MESSAGESMODEL_H
#define MESSAGESMODEL_H

#include "okmodels.h"

#include <QAbstractListModel>
#include <QList>
#include <QStringList>

class OkSession;
class QTimer;

class MessagesModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString conversationId READ conversationId NOTIFY conversationChanged)
    Q_PROPERTY(QString title READ title NOTIFY conversationChanged)
    Q_PROPERTY(bool isChat READ isChat NOTIFY conversationChanged)
    Q_PROPERTY(bool isSelf READ isSelf NOTIFY conversationChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(bool loadingEarlier READ loadingEarlier NOTIFY loadingChanged)
    Q_PROPERTY(bool hasMore READ hasMore NOTIFY loadingChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString replyToId READ replyToId NOTIFY replyChanged)
    Q_PROPERTY(QString replyToText READ replyToText NOTIFY replyChanged)
    Q_PROPERTY(int firstUnreadRow READ firstUnreadRow NOTIFY countChanged)
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        BodyRole,
        OutRole,
        TimeTextRole,
        DateTextRole,
        ShowDateRole,
        SenderNameRole,
        ShowSenderRole,
        IsSystemRole,
        PendingRole,
        FailedRole,
        EditedRole,
        HasImageRole,
        ImageUrlRole,
        FullImageUrlRole,
        HasVideoRole,
        HasAudioRole,
        MediaUrlRole,
        AttachmentTitleRole,
        AttachmentDurationRole,
        HasLinkRole,
        LinkUrlRole,
        ReplyTextRole,
        CanDeleteRole
    };

    explicit MessagesModel(OkSession *session, QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;

    QString conversationId() const { return m_conv; }
    QString title() const { return m_title; }
    bool isChat() const { return m_isChat; }
    bool isSelf() const { return m_isSelf; }
    bool loading() const { return m_loading; }
    bool loadingEarlier() const { return m_loadingEarlier; }
    bool hasMore() const { return m_hasMore; }
    QString error() const { return m_error; }
    QString replyToId() const { return m_replyToId; }
    QString replyToText() const { return m_replyToText; }
    /// The row of the first message after the last one the user had seen when the chat was
    /// opened (from the list's unread count), or -1: where the view should land on open.
    int firstUnreadRow() const { return m_firstUnreadRow; }

    /// Opens a conversation: clears the view and loads the latest page. unread is the
    /// list's unread count at that moment, used for firstUnreadRow.
    Q_INVOKABLE void open(const QString &conversationId, int unread);
    Q_INVOKABLE void close();
    Q_INVOKABLE void loadEarlier();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void send(const QString &text);
    Q_INVOKABLE void sendPhotoFile(const QString &path, const QString &caption);
    Q_INVOKABLE void retry(int row);
    Q_INVOKABLE void beginReply(int row);
    Q_INVOKABLE void cancelReply();
    Q_INVOKABLE void deleteMessage(int row);
    Q_INVOKABLE QVariantMap get(int row) const;

public slots:
    /// Poll cadence while the chat is open (the WP client's 10 s).
    void setPolling(bool on);

signals:
    void conversationChanged();
    void countChanged();
    void loadingChanged();
    void errorChanged();
    void replyChanged();
    /// New messages from the other side arrived through a poll (the view scrolls down).
    void newMessagesArrived(int count);
    void sendFailedNotice(const QString &error);

private slots:
    void onHistoryLoaded(const QString &conversationId, const QString &requestAnchor, const OkHistoryPage &page);
    void onHistoryFailed(const QString &conversationId, const QString &requestAnchor, const QString &error);
    void onMessageSent(const QString &conversationId, const QString &localId, const QString &serverId);
    void onSendFailed(const QString &conversationId, const QString &localId, const QString &error);
    void onMessagesDeleted(const QString &conversationId, const QStringList &messageIds, bool ok, const QString &error);
    void onPoll();

private:
    int rowOfId(const QString &id) const;
    int rowOfLocalId(const QString &localId) const;
    void mergeLatest(const QList<OkMessage> &fresh);
    void setError(const QString &e);
    void setLoading(bool loading, bool earlier);
    QString bodyForReply(int row) const;
    void appendLocal(OkMessage m);
    static bool sameDay(const QDateTime &a, const QDateTime &b);
    static QByteArray loadImageForUpload(const QString &path, QString *contentType);

    OkSession *m_session;
    QTimer *m_poll;
    QString m_conv;
    QString m_title;
    bool m_isChat;
    bool m_isSelf;
    QList<OkMessage> m_rows;
    QStringList m_localIds;          // parallel to m_rows: the local id of a bubble we created
    QString m_olderAnchor;
    bool m_hasMore;
    bool m_loading;
    bool m_loadingEarlier;
    bool m_pollInFlight;
    QString m_error;
    QString m_replyToId;
    QString m_replyToText;
    int m_firstUnreadRow;
    int m_openUnread;
    int m_localSeq;
};

#endif // MESSAGESMODEL_H
