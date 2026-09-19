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
// The conversation list for QML: a snapshot of the session's dialogs, newest first. It is
// rebuilt on every dialogsChanged - the list is short (tens of entries) and a reset is the
// cheapest correct thing on a QtQuick 1.1 ListView.
#ifndef CONVERSATIONSMODEL_H
#define CONVERSATIONSMODEL_H

#include "okmodels.h"

#include <QAbstractListModel>
#include <QList>

class OkSession;

class ConversationsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
public:
    enum Roles {
        ConversationIdRole = Qt::UserRole + 1,
        TitleRole,
        PreviewRole,
        UnreadRole,
        TimeTextRole,
        AvatarRole,
        IsChatRole,
        IsDraftRole,
        PeerUidRole,
        IsSelfRole
    };

    explicit ConversationsModel(OkSession *session, QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;

    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE int indexOf(const QString &conversationId) const;

    /// A short relative time for the list: "14:02" today, "Mon" this week, else "12.03".
    static QString shortTime(const QDateTime &utc);

signals:
    void countChanged();

private slots:
    void rebuild();

private:
    OkSession *m_session;
    QList<OkDialog> m_rows;
};

#endif // CONVERSATIONSMODEL_H
