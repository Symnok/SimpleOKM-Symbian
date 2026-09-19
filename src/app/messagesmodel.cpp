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
#include "messagesmodel.h"
#include "oksession.h"

#include <QBuffer>
#include <QDate>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QImage>
#include <QImageReader>
#include <QTimer>
#include <QUrl>
#include <algorithm>

namespace {
const int PollIntervalMs = 10 * 1000;
const int PageSize = 30;
const int MaxUploadEdge = 1280;

bool olderFirst(const OkMessage &a, const OkMessage &b)
{
    return a.date < b.date;
}
}

MessagesModel::MessagesModel(OkSession *session, QObject *parent)
    : QAbstractListModel(parent), m_session(session), m_poll(new QTimer(this)),
      m_isChat(false), m_isSelf(false), m_hasMore(false), m_loading(false), m_loadingEarlier(false),
      m_pollInFlight(false), m_firstUnreadRow(-1), m_openUnread(0), m_localSeq(0)
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "messageId";
    roles[BodyRole] = "body";
    roles[OutRole] = "out";
    roles[TimeTextRole] = "timeText";
    roles[DateTextRole] = "dateText";
    roles[ShowDateRole] = "showDate";
    roles[SenderNameRole] = "senderName";
    roles[ShowSenderRole] = "showSender";
    roles[IsSystemRole] = "isSystem";
    roles[PendingRole] = "pending";
    roles[FailedRole] = "failed";
    roles[EditedRole] = "edited";
    roles[HasImageRole] = "hasImage";
    roles[ImageUrlRole] = "imageUrl";
    roles[FullImageUrlRole] = "fullImageUrl";
    roles[HasVideoRole] = "hasVideo";
    roles[HasAudioRole] = "hasAudio";
    roles[MediaUrlRole] = "mediaUrl";
    roles[AttachmentTitleRole] = "attachmentTitle";
    roles[AttachmentDurationRole] = "attachmentDuration";
    roles[HasLinkRole] = "hasLink";
    roles[LinkUrlRole] = "linkUrl";
    roles[ReplyTextRole] = "replyText";
    roles[CanDeleteRole] = "canDelete";
    setRoleNames(roles);

    m_poll->setInterval(PollIntervalMs);
    connect(m_poll, SIGNAL(timeout()), this, SLOT(onPoll()));

    connect(session, SIGNAL(historyLoaded(QString,QString,OkHistoryPage)),
            this, SLOT(onHistoryLoaded(QString,QString,OkHistoryPage)));
    connect(session, SIGNAL(historyFailed(QString,QString,QString)),
            this, SLOT(onHistoryFailed(QString,QString,QString)));
    connect(session, SIGNAL(messageSent(QString,QString,QString)),
            this, SLOT(onMessageSent(QString,QString,QString)));
    connect(session, SIGNAL(sendFailed(QString,QString,QString)),
            this, SLOT(onSendFailed(QString,QString,QString)));
    connect(session, SIGNAL(messagesDeleted(QString,QStringList,bool,QString)),
            this, SLOT(onMessagesDeleted(QString,QStringList,bool,QString)));
}

int MessagesModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

bool MessagesModel::sameDay(const QDateTime &a, const QDateTime &b)
{
    return a.isValid() && b.isValid() && a.toLocalTime().date() == b.toLocalTime().date();
}

QString MessagesModel::bodyForReply(int row) const
{
    if (row < 0 || row >= m_rows.size()) return QString();
    const OkMessage &m = m_rows.at(row);
    if (!m.body.isEmpty()) return m.body;
    if (!m.attachments.isEmpty()) return m.attachments.first().title;
    return QString();
}

QVariant MessagesModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (!index.isValid() || row >= m_rows.size()) return QVariant();
    const OkMessage &m = m_rows.at(row);

    // The first attachment drives the bubble; OK messages carry at most one in practice.
    const OkAttachment *a = m.attachments.isEmpty() ? 0 : &m.attachments.first();
    const bool image = a && a->isImage();
    const bool video = a && a->isVideo();
    const bool audio = a && a->isAudio();
    const bool link = a && !image && !video && !audio && !a->url.isEmpty();

    switch (role) {
    case IdRole: return m.id;
    case BodyRole: return m.body;
    case OutRole: return m.out;
    case TimeTextRole: return m.date.isValid() ? m.date.toLocalTime().toString(QLatin1String("HH:mm")) : QString();
    case DateTextRole: {
        if (!m.date.isValid()) return QString();
        const QDate d = m.date.toLocalTime().date();
        if (d == QDate::currentDate()) return tr("Today");
        if (d == QDate::currentDate().addDays(-1)) return tr("Yesterday");
        return d.toString(d.year() == QDate::currentDate().year() ? QLatin1String("d MMMM") : QLatin1String("d MMMM yyyy"));
    }
    case ShowDateRole: return row == 0 || !sameDay(m.date, m_rows.at(row - 1).date);
    case SenderNameRole: return m.senderName;
    case ShowSenderRole: return m_isChat && !m.out && !m.isSystem;
    case IsSystemRole: return m.isSystem;
    case PendingRole: return m.isPending;
    case FailedRole: return m.failed;
    case EditedRole: return m.edited();
    case HasImageRole: return image;
    case ImageUrlRole: return image ? a->previewUrl : QString();
    case FullImageUrlRole: return image ? a->url : QString();
    case HasVideoRole: return video;
    case HasAudioRole: return audio;
    case MediaUrlRole: return (video || audio) ? a->url : QString();
    case AttachmentTitleRole: return a ? a->title : QString();
    case AttachmentDurationRole: return a ? a->durationText() : QString();
    case HasLinkRole: return link;
    case LinkUrlRole: return link ? a->url : QString();
    case ReplyTextRole: return m.replyToId.isEmpty() ? QString() : bodyForReply(rowOfId(m.replyToId));
    case CanDeleteRole: return m.canDelete() && !m.isPending && !m.id.isEmpty();
    }
    return QVariant();
}

QVariantMap MessagesModel::get(int row) const
{
    QVariantMap out;
    if (row < 0 || row >= m_rows.size()) return out;
    const QModelIndex idx = index(row);
    const QHash<int, QByteArray> names = roleNames();
    for (QHash<int, QByteArray>::const_iterator it = names.constBegin(); it != names.constEnd(); ++it)
        out.insert(QString::fromLatin1(it.value()), data(idx, it.key()));
    return out;
}

int MessagesModel::rowOfId(const QString &id) const
{
    if (id.isEmpty()) return -1;
    for (int i = m_rows.size() - 1; i >= 0; --i)
        if (m_rows.at(i).id == id) return i;
    return -1;
}

int MessagesModel::rowOfLocalId(const QString &localId) const
{
    for (int i = m_localIds.size() - 1; i >= 0; --i)
        if (m_localIds.at(i) == localId) return i;
    return -1;
}

void MessagesModel::setError(const QString &e)
{
    if (m_error == e) return;
    m_error = e;
    emit errorChanged();
}

void MessagesModel::setLoading(bool loading, bool earlier)
{
    m_loading = loading && !earlier;
    m_loadingEarlier = loading && earlier;
    emit loadingChanged();
}

// -- opening / closing --

void MessagesModel::open(const QString &conversationId, int unread)
{
    close();
    m_conv = conversationId;
    m_openUnread = unread;

    const int idx = m_session->dialogIndex(conversationId);
    if (idx >= 0) {
        const OkDialog &d = m_session->dialogs().at(idx);
        m_title = d.title;
        m_isChat = d.isChat;
        m_isSelf = !d.isChat && d.peerUid.isEmpty();
    } else {
        m_title = tr("Chat");
        m_isChat = false;
        m_isSelf = false;
    }
    emit conversationChanged();

    if (m_isSelf) return;   // OK's API cannot read a chat with yourself; the page says so
    setLoading(true, false);
    m_session->loadHistory(m_conv, QString(), PageSize);
}

void MessagesModel::close()
{
    setPolling(false);
    beginResetModel();
    m_rows.clear();
    m_localIds.clear();
    endResetModel();
    m_conv.clear();
    m_title.clear();
    m_olderAnchor.clear();
    m_hasMore = false;
    m_pollInFlight = false;
    m_firstUnreadRow = -1;
    m_loading = m_loadingEarlier = false;
    cancelReply();
    setError(QString());
    emit countChanged();
    emit loadingChanged();
}

void MessagesModel::setPolling(bool on)
{
    if (on && !m_conv.isEmpty() && !m_isSelf) m_poll->start();
    else m_poll->stop();
}

void MessagesModel::loadEarlier()
{
    if (m_conv.isEmpty() || !m_hasMore || m_olderAnchor.isEmpty() || m_loading || m_loadingEarlier) return;
    setLoading(true, true);
    m_session->loadHistory(m_conv, m_olderAnchor, PageSize);
}

void MessagesModel::refresh()
{
    if (m_conv.isEmpty() || m_isSelf) return;
    onPoll();
}

void MessagesModel::onPoll()
{
    if (m_conv.isEmpty() || m_pollInFlight || m_loading) return;
    m_pollInFlight = true;
    m_session->loadHistory(m_conv, QString(), PageSize);
}

// -- history --

void MessagesModel::onHistoryLoaded(const QString &conversationId, const QString &requestAnchor, const OkHistoryPage &page)
{
    if (conversationId != m_conv) return;

    if (!requestAnchor.isEmpty()) {
        // An older page: prepend, and remember where the page before it starts.
        QList<OkMessage> older = page.messages;
        std::stable_sort(older.begin(), older.end(), olderFirst);
        for (int i = older.size() - 1; i >= 0; --i)
            if (rowOfId(older.at(i).id) >= 0) older.removeAt(i);
        if (!older.isEmpty()) {
            beginInsertRows(QModelIndex(), 0, older.size() - 1);
            for (int i = older.size() - 1; i >= 0; --i) { m_rows.prepend(older.at(i)); m_localIds.prepend(QString()); }
            endInsertRows();
            // The date separator of the old first row may no longer apply.
            const QModelIndex changed = index(older.size());
            emit dataChanged(changed, changed);
            emit countChanged();
        }
        m_olderAnchor = page.anchor;
        m_hasMore = page.hasMore && !page.anchor.isEmpty() && !older.isEmpty();
        setLoading(false, true);
        return;
    }

    const bool initial = m_loading;
    if (initial) {
        beginResetModel();
        m_rows = page.messages;
        std::stable_sort(m_rows.begin(), m_rows.end(), olderFirst);
        m_localIds.clear();
        for (int i = 0; i < m_rows.size(); ++i) m_localIds.append(QString());
        endResetModel();
        m_olderAnchor = page.anchor;
        m_hasMore = page.hasMore && !page.anchor.isEmpty();
        m_firstUnreadRow = (m_openUnread > 0 && m_openUnread < m_rows.size()) ? m_rows.size() - m_openUnread : -1;
        emit countChanged();
        setError(QString());
        setLoading(false, false);
        setPolling(true);
    } else {
        m_pollInFlight = false;
        mergeLatest(page.messages);
    }
}

void MessagesModel::onHistoryFailed(const QString &conversationId, const QString &requestAnchor, const QString &error)
{
    if (conversationId != m_conv) return;
    m_pollInFlight = false;
    setLoading(false, !requestAnchor.isEmpty());
    setError(error);
}

/// Merges a freshly polled latest page: known messages are updated in place (edits,
/// resolved attachments), unknown ones appended in date order. A server copy of a message
/// this client sent replaces its pending bubble.
void MessagesModel::mergeLatest(const QList<OkMessage> &fresh)
{
    int arrived = 0;
    QList<OkMessage> sorted = fresh;
    std::stable_sort(sorted.begin(), sorted.end(), olderFirst);

    for (int i = 0; i < sorted.size(); ++i) {
        const OkMessage &f = sorted.at(i);
        int row = rowOfId(f.id);
        if (row < 0 && f.out) {
            // Our own message coming back before (or instead of) the send reply: match it to
            // the oldest pending bubble with the same text.
            for (int r = 0; r < m_rows.size(); ++r) {
                const OkMessage &m = m_rows.at(r);
                if (m.isLocal && m.id.isEmpty() && m.body == f.body) { row = r; break; }
            }
        }
        if (row >= 0) {
            OkMessage &m = m_rows[row];
            const bool changed = m.body != f.body || m.editedMs != f.editedMs || m.isPending || m.isLocal
                                 || m.attachments.size() != f.attachments.size();
            m = f;
            m_localIds[row] = QString();
            if (changed) { const QModelIndex idx = index(row); emit dataChanged(idx, idx); }
            continue;
        }
        // New: insert keeping date order (normally at the end).
        int pos = m_rows.size();
        while (pos > 0 && m_rows.at(pos - 1).date > f.date && !m_rows.at(pos - 1).isLocal) --pos;
        beginInsertRows(QModelIndex(), pos, pos);
        m_rows.insert(pos, f);
        m_localIds.insert(pos, QString());
        endInsertRows();
        if (!f.out) ++arrived;
    }
    if (!sorted.isEmpty()) emit countChanged();
    if (arrived > 0) emit newMessagesArrived(arrived);
}

// -- sending --

void MessagesModel::appendLocal(OkMessage m)
{
    m.conversationId = m_conv;
    m.out = true;
    m.isPending = true;
    m.isLocal = true;
    m.date = QDateTime::currentDateTime().toUTC();
    m.senderName = m_session->me().displayName();
    const QString localId = QString::fromLatin1("local-%1").arg(++m_localSeq);

    beginInsertRows(QModelIndex(), m_rows.size(), m_rows.size());
    m_rows.append(m);
    m_localIds.append(localId);
    endInsertRows();
    emit countChanged();

    // A photo bubble is sent by sendPhotoFile (it has the bytes); text goes out here.
    if (m.attachments.isEmpty())
        m_session->sendMessage(m_conv, m.body, m.replyToId, localId);
}

void MessagesModel::send(const QString &text)
{
    const QString body = text.trimmed();
    if (m_conv.isEmpty() || body.isEmpty()) return;
    OkMessage m;
    m.body = body;
    m.replyToId = m_replyToId;
    appendLocal(m);
    cancelReply();
}

QByteArray MessagesModel::loadImageForUpload(const QString &path, QString *contentType)
{
    // Decode straight to a bounded size: a 5-8 MP photo decoded at full size is 20-30 MB of
    // pixels, more than the phone should spend on an upload. JPEG decodes scaled cheaply.
    QImageReader reader(path);
    reader.setAutoDetectImageFormat(true);
    QSize size = reader.size();
    if (size.isValid() && (size.width() > MaxUploadEdge || size.height() > MaxUploadEdge)) {
        size.scale(MaxUploadEdge, MaxUploadEdge, Qt::KeepAspectRatio);
        reader.setScaledSize(size);
    }
    const QImage img = reader.read();
    if (img.isNull()) {
        // Not decodable here (or not an image): send the file as-is if it is a JPEG.
        QFile f(path);
        if (path.endsWith(QLatin1String(".jpg"), Qt::CaseInsensitive) || path.endsWith(QLatin1String(".jpeg"), Qt::CaseInsensitive)) {
            if (f.open(QIODevice::ReadOnly)) { *contentType = QLatin1String("image/jpeg"); return f.readAll(); }
        }
        return QByteArray();
    }
    QByteArray bytes;
    QBuffer buf(&bytes);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "JPEG", 85);
    *contentType = QLatin1String("image/jpeg");
    return bytes;
}

void MessagesModel::sendPhotoFile(const QString &path, const QString &caption)
{
    if (m_conv.isEmpty() || path.isEmpty()) return;
    QString contentType;
    const QByteArray bytes = loadImageForUpload(path, &contentType);
    if (bytes.isEmpty()) {
        emit sendFailedNotice(tr("Could not read the image"));
        return;
    }

    OkMessage m;
    m.body = caption.trimmed();
    OkAttachment a;
    a.type = QLatin1String("PHOTO");
    a.title = tr("Photo");
    a.previewUrl = QUrl::fromLocalFile(path).toString();
    a.url = a.previewUrl;
    m.attachments.append(a);
    appendLocal(m);
    m_session->sendPhoto(m_conv, bytes, m.body, contentType, m_localIds.last());
}

void MessagesModel::retry(int row)
{
    if (row < 0 || row >= m_rows.size()) return;
    OkMessage &m = m_rows[row];
    if (!m.failed || !m.isLocal) return;
    if (!m.attachments.isEmpty()) {
        // The bytes are gone; the user re-picks the photo. Drop the failed bubble.
        beginRemoveRows(QModelIndex(), row, row);
        m_rows.removeAt(row);
        m_localIds.removeAt(row);
        endRemoveRows();
        emit countChanged();
        return;
    }
    m.failed = false;
    m.isPending = true;
    const QModelIndex idx = index(row);
    emit dataChanged(idx, idx);
    m_session->sendMessage(m_conv, m.body, m.replyToId, m_localIds.at(row));
}

void MessagesModel::onMessageSent(const QString &conversationId, const QString &localId, const QString &serverId)
{
    if (conversationId != m_conv) return;
    const int row = rowOfLocalId(localId);
    if (row < 0) return;
    OkMessage &m = m_rows[row];
    m.isPending = false;
    m.failed = false;
    if (!serverId.isEmpty()) m.id = serverId;
    // Keep isLocal until the poll brings the server copy (with the real attachment urls).
    const QModelIndex idx = index(row);
    emit dataChanged(idx, idx);
    // Pull the server copy soon rather than at the next tick.
    QTimer::singleShot(1500, this, SLOT(onPoll()));
}

void MessagesModel::onSendFailed(const QString &conversationId, const QString &localId, const QString &error)
{
    if (conversationId != m_conv) return;
    const int row = rowOfLocalId(localId);
    if (row >= 0) {
        OkMessage &m = m_rows[row];
        m.isPending = false;
        m.failed = true;
        const QModelIndex idx = index(row);
        emit dataChanged(idx, idx);
    }
    emit sendFailedNotice(error);
}

// -- reply / delete --

void MessagesModel::beginReply(int row)
{
    if (row < 0 || row >= m_rows.size() || m_rows.at(row).id.isEmpty()) return;
    m_replyToId = m_rows.at(row).id;
    m_replyToText = bodyForReply(row);
    if (m_replyToText.isEmpty()) m_replyToText = tr("(attachment)");
    emit replyChanged();
}

void MessagesModel::cancelReply()
{
    if (m_replyToId.isEmpty() && m_replyToText.isEmpty()) return;
    m_replyToId.clear();
    m_replyToText.clear();
    emit replyChanged();
}

void MessagesModel::deleteMessage(int row)
{
    if (row < 0 || row >= m_rows.size()) return;
    const OkMessage &m = m_rows.at(row);
    if (m.id.isEmpty()) {
        // A local bubble that never reached the server: just drop it.
        beginRemoveRows(QModelIndex(), row, row);
        m_rows.removeAt(row);
        m_localIds.removeAt(row);
        endRemoveRows();
        emit countChanged();
        return;
    }
    m_session->deleteMessages(m_conv, QStringList() << m.id);
}

void MessagesModel::onMessagesDeleted(const QString &conversationId, const QStringList &messageIds, bool ok, const QString &error)
{
    if (conversationId != m_conv) return;
    if (!ok) { setError(error); return; }
    for (int i = 0; i < messageIds.size(); ++i) {
        const int row = rowOfId(messageIds.at(i));
        if (row < 0) continue;
        beginRemoveRows(QModelIndex(), row, row);
        m_rows.removeAt(row);
        m_localIds.removeAt(row);
        endRemoveRows();
    }
    emit countChanged();
}
