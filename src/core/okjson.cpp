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
#include "okjson.h"

#include <QStringList>

namespace {

class Reader
{
public:
    Reader(const QByteArray &text) : failed(false), t(text), p(0) {}

    QVariant readDocument()
    {
        skipSpace();
        QVariant v = readValue();
        skipSpace();
        if (!failed && p != t.size()) fail("trailing characters");
        return failed ? QVariant() : v;
    }

    bool failed;
    QString errorText;

private:
    const QByteArray &t;
    int p;

    void fail(const char *why)
    {
        if (!failed) {
            failed = true;
            errorText = QString::fromLatin1("%1 at offset %2").arg(QLatin1String(why)).arg(p);
        }
    }

    void skipSpace()
    {
        while (p < t.size()) {
            char c = t.at(p);
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') ++p; else break;
        }
    }

    bool consume(const char *lit)
    {
        int n = int(qstrlen(lit));
        if (t.mid(p, n) == lit) { p += n; return true; }
        return false;
    }

    QVariant readValue()
    {
        if (failed) return QVariant();
        if (p >= t.size()) { fail("unexpected end"); return QVariant(); }
        char c = t.at(p);
        switch (c) {
        case '{': return readObject();
        case '[': return readArray();
        case '"': return readString();
        case 't': if (consume("true")) return QVariant(true); break;
        case 'f': if (consume("false")) return QVariant(false); break;
        case 'n': if (consume("null")) return QVariant(); break;
        default:
            if (c == '-' || (c >= '0' && c <= '9')) return readNumber();
        }
        fail("unexpected character");
        return QVariant();
    }

    QVariant readObject()
    {
        QVariantMap map;
        ++p; // {
        skipSpace();
        if (p < t.size() && t.at(p) == '}') { ++p; return map; }
        for (;;) {
            skipSpace();
            if (p >= t.size() || t.at(p) != '"') { fail("expected key"); return QVariant(); }
            QString key = readString();
            skipSpace();
            if (p >= t.size() || t.at(p) != ':') { fail("expected ':'"); return QVariant(); }
            ++p;
            skipSpace();
            QVariant v = readValue();
            if (failed) return QVariant();
            map.insert(key, v);
            skipSpace();
            if (p >= t.size()) { fail("unterminated object"); return QVariant(); }
            if (t.at(p) == ',') { ++p; continue; }
            if (t.at(p) == '}') { ++p; return map; }
            fail("expected ',' or '}'");
            return QVariant();
        }
    }

    QVariant readArray()
    {
        QVariantList list;
        ++p; // [
        skipSpace();
        if (p < t.size() && t.at(p) == ']') { ++p; return list; }
        for (;;) {
            skipSpace();
            QVariant v = readValue();
            if (failed) return QVariant();
            list.append(v);
            skipSpace();
            if (p >= t.size()) { fail("unterminated array"); return QVariant(); }
            if (t.at(p) == ',') { ++p; continue; }
            if (t.at(p) == ']') { ++p; return list; }
            fail("expected ',' or ']'");
            return QVariant();
        }
    }

    static int hexVal(char c)
    {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }

    int readHex4()
    {
        if (p + 4 > t.size()) return -1;
        int v = 0;
        for (int i = 0; i < 4; ++i) {
            int h = hexVal(t.at(p + i));
            if (h < 0) return -1;
            v = (v << 4) | h;
        }
        p += 4;
        return v;
    }

    QString readString()
    {
        ++p; // opening quote
        QByteArray raw;   // UTF-8 bytes, escapes resolved
        for (;;) {
            if (p >= t.size()) { fail("unterminated string"); return QString(); }
            char c = t.at(p++);
            if (c == '"') break;
            if (c != '\\') { raw.append(c); continue; }
            if (p >= t.size()) { fail("bad escape"); return QString(); }
            char e = t.at(p++);
            switch (e) {
            case '"': raw.append('"'); break;
            case '\\': raw.append('\\'); break;
            case '/': raw.append('/'); break;
            case 'b': raw.append('\b'); break;
            case 'f': raw.append('\f'); break;
            case 'n': raw.append('\n'); break;
            case 'r': raw.append('\r'); break;
            case 't': raw.append('\t'); break;
            case 'u': {
                int cp = readHex4();
                if (cp < 0) { fail("bad \\u escape"); return QString(); }
                // A surrogate pair arrives as two \u escapes; join them.
                if (cp >= 0xD800 && cp <= 0xDBFF && t.mid(p, 2) == "\\u") {
                    int save = p;
                    p += 2;
                    int lo = readHex4();
                    if (lo >= 0xDC00 && lo <= 0xDFFF)
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                    else
                        p = save;
                }
                QString s;
                if (cp > 0xFFFF) {
                    s.append(QChar::highSurrogate(uint(cp)));
                    s.append(QChar::lowSurrogate(uint(cp)));
                } else {
                    s.append(QChar(ushort(cp)));
                }
                raw.append(s.toUtf8());
                break;
            }
            default:
                fail("bad escape");
                return QString();
            }
        }
        return QString::fromUtf8(raw.constData(), raw.size());
    }

    QVariant readNumber()
    {
        int start = p;
        bool isFloat = false;
        if (t.at(p) == '-') ++p;
        while (p < t.size()) {
            char c = t.at(p);
            if (c >= '0' && c <= '9') { ++p; continue; }
            if (c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-') { isFloat = true; ++p; continue; }
            break;
        }
        QByteArray num = t.mid(start, p - start);
        bool ok = false;
        if (!isFloat) {
            qlonglong v = num.toLongLong(&ok);
            if (ok) return QVariant(v);
        }
        double d = num.toDouble(&ok);
        if (!ok) { fail("bad number"); return QVariant(); }
        return QVariant(d);
    }
};

void writeString(QByteArray &out, const QString &s)
{
    out.append('"');
    const QByteArray u = s.toUtf8();
    for (int i = 0; i < u.size(); ++i) {
        unsigned char c = (unsigned char)u.at(i);
        switch (c) {
        case '"': out.append("\\\""); break;
        case '\\': out.append("\\\\"); break;
        case '\n': out.append("\\n"); break;
        case '\r': out.append("\\r"); break;
        case '\t': out.append("\\t"); break;
        case '\b': out.append("\\b"); break;
        case '\f': out.append("\\f"); break;
        default:
            if (c < 0x20) {
                const char *hex = "0123456789abcdef";
                out.append("\\u00");
                out.append(hex[c >> 4]);
                out.append(hex[c & 15]);
            } else {
                out.append((char)c);
            }
        }
    }
    out.append('"');
}

void writeValue(QByteArray &out, const QVariant &v)
{
    switch (v.type()) {
    case QVariant::Invalid:
        out.append("null");
        break;
    case QVariant::Bool:
        out.append(v.toBool() ? "true" : "false");
        break;
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong:
        out.append(v.toString().toLatin1());
        break;
    case QVariant::Double:
        out.append(QByteArray::number(v.toDouble(), 'g', 15));
        break;
    case QVariant::Map: {
        const QVariantMap m = v.toMap();
        out.append('{');
        bool first = true;
        for (QVariantMap::const_iterator it = m.constBegin(); it != m.constEnd(); ++it) {
            if (!first) out.append(',');
            first = false;
            writeString(out, it.key());
            out.append(':');
            writeValue(out, it.value());
        }
        out.append('}');
        break;
    }
    case QVariant::List:
    case QVariant::StringList: {
        const QVariantList l = v.toList();
        out.append('[');
        for (int i = 0; i < l.size(); ++i) {
            if (i) out.append(',');
            writeValue(out, l.at(i));
        }
        out.append(']');
        break;
    }
    default:
        writeString(out, v.toString());
    }
}

} // namespace

namespace OkJson {

QVariant parse(const QByteArray &text, bool *ok, QString *error)
{
    Reader r(text);
    QVariant v = r.readDocument();
    if (ok) *ok = !r.failed;
    if (error) *error = r.errorText;
    return v;
}

QByteArray serialize(const QVariant &value)
{
    QByteArray out;
    writeValue(out, value);
    return out;
}

QString str(const QVariantMap &map, const char *key)
{
    const QVariant v = map.value(QLatin1String(key));
    if (!v.isValid() || v.isNull()) return QString();
    if (v.type() == QVariant::Map || v.type() == QVariant::List) return QString();
    return v.toString();
}

qlonglong integer(const QVariantMap &map, const char *key, qlonglong fallback)
{
    const QVariant v = map.value(QLatin1String(key));
    if (!v.isValid()) return fallback;
    bool ok = false;
    qlonglong n = v.toLongLong(&ok);
    if (ok) return n;
    double d = v.toDouble(&ok);
    return ok ? qlonglong(d) : fallback;
}

bool boolean(const QVariantMap &map, const char *key, bool fallback)
{
    const QVariant v = map.value(QLatin1String(key));
    if (!v.isValid()) return fallback;
    if (v.type() == QVariant::Bool) return v.toBool();
    const QString s = v.toString().toLower();
    if (s == QLatin1String("true")) return true;
    if (s == QLatin1String("false")) return false;
    return fallback;
}

QVariantMap object(const QVariantMap &map, const char *key)
{
    return map.value(QLatin1String(key)).toMap();
}

QVariantList array(const QVariantMap &map, const char *key)
{
    const QVariant v = map.value(QLatin1String(key));
    if (v.type() == QVariant::List || v.type() == QVariant::StringList) return v.toList();
    return QVariantList();
}

} // namespace OkJson
