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
// How a request is signed - Ok.Api.UrlUtilities.QueryBuilder.CalculateSignature, copied, not
// reconstructed. Two details are easy to get wrong:
//   * every parameter takes part except "url" - the session key is NOT excluded;
//   * the "key=value" strings are sorted as whole strings, ordinally, not by key.
// The secret appended at the end is the application secret for login and the session
// secret for every call after; it is never sent on the wire.
#ifndef OKSIGNING_H
#define OKSIGNING_H

#include <QMap>
#include <QString>

typedef QMap<QString, QString> OkParams;

namespace OkSigning
{
    /// The value of the "sig" parameter for the parameters gathered so far (without a sig).
    QString signature(const OkParams &params, const QString &secret);

    /// Lowercase hex MD5 of the UTF-8 text, the form OK uses everywhere.
    QString md5Hex(const QString &text);
}

#endif // OKSIGNING_H
