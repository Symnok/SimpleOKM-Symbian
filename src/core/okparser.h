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
// Turns OK's JSON into the model. The field names are the ones the official client reads
// (GetSingleConversation.ParseConversation, GetMessagesBase.ParseMessage) - the same map as
// SimpleOKM.Core's OkParser, on QVariant instead of JObject.
#ifndef OKPARSER_H
#define OKPARSER_H

#include "okmodels.h"

#include <QVariantList>
#include <QVariantMap>

namespace OkParser
{
    /// One conversation object from messagesV2/getList. Returns false when it has no id.
    bool readConversation(const QVariantMap &o, const QString &myUid, OkDialog *out);

    /// One message from messagesV2/getMessages (attachments carry only ids at this point).
    OkMessage readMessage(const QVariantMap &o, const QString &myUid);

    /// Fills in picture / playback urls on already-parsed messages from the paired
    /// getAttachedResources reply, matched by attachment id. Also promotes http urls to https.
    void applyAttachedResources(QList<OkMessage> &messages, const QVariantList &resources);

    /// One user from users/getInfo. Returns false when it has no uid.
    bool readUser(const QVariantMap &o, OkUser *out);

    /// Milliseconds since the epoch to a UTC QDateTime (invalid for 0/missing).
    QDateTime fromMs(qlonglong ms);
}

#endif // OKPARSER_H
