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
#include "friendsmodel.h"
#include "oksession.h"

#include <QHash>

FriendsModel::FriendsModel(OkSession *session, QObject *parent)
    : QAbstractListModel(parent), m_session(session), m_loading(false)
{
    QHash<int, QByteArray> roles;
    roles[UidRole] = "uid";
    roles[NameRole] = "name";
    roles[AvatarRole] = "avatar";
    roles[OnlineRole] = "online";
    setRoleNames(roles);
    connect(session, SIGNAL(friendsLoaded(bool,QList<OkUser>,QString)),
            this, SLOT(onFriends(bool,QList<OkUser>,QString)));
}

int FriendsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant FriendsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size()) return QVariant();
    const OkUser &u = m_rows.at(index.row());
    switch (role) {
    case UidRole: return u.uid;
    case NameRole: return u.displayName();
    case AvatarRole: return u.photo;
    case OnlineRole: return u.online;
    }
    return QVariant();
}

void FriendsModel::load()
{
    if (m_loading) return;
    m_loading = true;
    m_error.clear();
    emit loadingChanged();
    m_session->loadFriends();
}

void FriendsModel::onFriends(bool ok, const QList<OkUser> &friends, const QString &error)
{
    m_loading = false;
    m_error = ok ? QString() : error;
    if (ok) {
        beginResetModel();
        m_rows = friends;
        endResetModel();
        emit countChanged();
    }
    emit loadingChanged();
}

QString FriendsModel::openConversation(int row)
{
    if (row < 0 || row >= m_rows.size()) return QString();
    return m_session->openPrivateConversation(m_rows.at(row));
}
