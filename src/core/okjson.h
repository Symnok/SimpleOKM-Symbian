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
// A small JSON reader/writer on QVariant. Qt 4.7 has no JSON module and the phone has no
// Newtonsoft, so this is the stand-in for JToken/JObject in the other ports:
//   object -> QVariantMap, array -> QVariantList, string -> QString, number -> qlonglong or
//   double, true/false -> bool, null -> invalid QVariant.
#ifndef OKJSON_H
#define OKJSON_H

#include <QByteArray>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QVariantList>

namespace OkJson
{
    /// Parses UTF-8 JSON text. On a syntax error *ok is false and an invalid QVariant is
    /// returned; error, if given, gets a short description with the byte offset.
    QVariant parse(const QByteArray &text, bool *ok = 0, QString *error = 0);

    /// Serialises a QVariant tree to compact JSON (no whitespace), UTF-8.
    QByteArray serialize(const QVariant &value);

    // -- convenience accessors, tolerant of missing keys and mixed number/string types ----

    /// map[key] as a string; numbers are formatted, null/missing gives QString().
    QString str(const QVariantMap &map, const char *key);
    /// map[key] as an integer; a numeric string is converted; missing gives fallback.
    qlonglong integer(const QVariantMap &map, const char *key, qlonglong fallback = 0);
    /// map[key] as a bool; "true"/"false" strings count; missing gives fallback.
    bool boolean(const QVariantMap &map, const char *key, bool fallback = false);
    QVariantMap object(const QVariantMap &map, const char *key);
    QVariantList array(const QVariantMap &map, const char *key);
}

#endif // OKJSON_H
