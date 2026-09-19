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
// The friend list "New chat" picks from - OK's messaging API has no separate contacts.
#ifndef FRIENDSMODEL_H
#define FRIENDSMODEL_H

#include "okmodels.h"

#include <QAbstractListModel>
#include <QList>

class OkSession;

class FriendsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString error READ error NOTIFY loadingChanged)
public:
    enum Roles { UidRole = Qt::UserRole + 1, NameRole, AvatarRole, OnlineRole };

    explicit FriendsModel(OkSession *session, QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool loading() const { return m_loading; }
    QString error() const { return m_error; }

    Q_INVOKABLE void load();
    /// Opens (or creates a draft for) the one-to-one with the friend at row; returns its
    /// conversation id.
    Q_INVOKABLE QString openConversation(int row);

signals:
    void countChanged();
    void loadingChanged();

private slots:
    void onFriends(bool ok, const QList<OkUser> &friends, const QString &error);

private:
    OkSession *m_session;
    QList<OkUser> m_rows;
    bool m_loading;
    QString m_error;
};

#endif // FRIENDSMODEL_H
