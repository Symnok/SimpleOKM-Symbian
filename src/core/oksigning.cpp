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
#include "oksigning.h"
#include "okconstants.h"

#include <QCryptographicHash>
#include <QStringList>
#include <algorithm>

namespace {

// Ordinal (code unit) comparison, matching StringComparer.Ordinal in the C# core. QString's
// operator< is also code-unit based, but spell it out so the intent survives.
bool ordinalLess(const QString &a, const QString &b)
{
    return QString::compare(a, b, Qt::CaseSensitive) < 0;
}

}

namespace OkSigning {

QString signature(const OkParams &params, const QString &secret)
{
    QStringList pairs;
    for (OkParams::const_iterator it = params.constBegin(); it != params.constEnd(); ++it) {
        if (it.key() == QLatin1String(OkConstants::ParamUrl)) continue;
        if (it.key() == QLatin1String(OkConstants::ParamSignature)) continue;
        pairs.append(it.key() + QLatin1Char('=') + it.value());
    }
    std::sort(pairs.begin(), pairs.end(), ordinalLess);

    QString joined;
    for (int i = 0; i < pairs.size(); ++i) joined += pairs.at(i);
    joined += secret;
    return md5Hex(joined);
}

QString md5Hex(const QString &text)
{
    const QByteArray digest = QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Md5);
    return QString::fromLatin1(digest.toHex()).toLower();
}

}
