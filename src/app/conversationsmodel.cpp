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
#include "conversationsmodel.h"
#include "oksession.h"

#include <QDate>
#include <QHash>
#include <algorithm>

namespace {
bool newerFirst(const OkDialog &a, const OkDialog &b)
{
    return a.lastTime > b.lastTime;
}
}

ConversationsModel::ConversationsModel(OkSession *session, QObject *parent)
    : QAbstractListModel(parent), m_session(session)
{
    QHash<int, QByteArray> roles;
    roles[ConversationIdRole] = "conversationId";
    roles[TitleRole] = "title";
    roles[PreviewRole] = "preview";
    roles[UnreadRole] = "unread";
    roles[TimeTextRole] = "timeText";
    roles[AvatarRole] = "avatar";
    roles[IsChatRole] = "isChat";
    roles[IsDraftRole] = "isDraft";
    roles[PeerUidRole] = "peerUid";
    roles[IsSelfRole] = "isSelf";
    setRoleNames(roles);

    connect(session, SIGNAL(dialogsChanged()), this, SLOT(rebuild()));
    rebuild();
}

int ConversationsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QString ConversationsModel::shortTime(const QDateTime &utc)
{
    if (!utc.isValid()) return QString();
    const QDateTime local = utc.toLocalTime();
    const QDate today = QDate::currentDate();
    if (local.date() == today) return local.toString(QLatin1String("HH:mm"));
    if (local.date().daysTo(today) < 7) return local.toString(QLatin1String("ddd"));
    if (local.date().year() == today.year()) return local.toString(QLatin1String("dd.MM"));
    return local.toString(QLatin1String("dd.MM.yy"));
}

QVariant ConversationsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size()) return QVariant();
    const OkDialog &d = m_rows.at(index.row());
    switch (role) {
    case ConversationIdRole: return d.conversationId;
    case TitleRole: return d.title;
    case PreviewRole: return d.preview;
    case UnreadRole: return d.unreadCount;
    case TimeTextRole: return shortTime(d.lastTime);
    case AvatarRole: return d.hasUser() ? d.user.photo : QString();
    case IsChatRole: return d.isChat;
    case IsDraftRole: return d.isDraft;
    case PeerUidRole: return d.peerUid;
    // PRIVATE:<own uid>, which OK's API cannot read (see OK-Messenger-API.md, section 12).
    case IsSelfRole: return !d.isChat && d.peerUid.isEmpty();
    }
    return QVariant();
}

QVariantMap ConversationsModel::get(int row) const
{
    QVariantMap m;
    if (row < 0 || row >= m_rows.size()) return m;
    const QModelIndex idx = index(row);
    const QHash<int, QByteArray> names = roleNames();
    for (QHash<int, QByteArray>::const_iterator it = names.constBegin(); it != names.constEnd(); ++it)
        m.insert(QString::fromLatin1(it.value()), data(idx, it.key()));
    return m;
}

int ConversationsModel::indexOf(const QString &conversationId) const
{
    for (int i = 0; i < m_rows.size(); ++i)
        if (m_rows.at(i).conversationId == conversationId) return i;
    return -1;
}

void ConversationsModel::rebuild()
{
    beginResetModel();
    m_rows = m_session->dialogs();
    std::stable_sort(m_rows.begin(), m_rows.end(), newerFirst);
    endResetModel();
    emit countChanged();
}
