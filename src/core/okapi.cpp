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
#include "okapi.h"
#include "okconstants.h"
#include "okjson.h"
#include "oknetwork.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QUuid>
#include <QDebug>

namespace {
const int ApiTimeoutMs = 40 * 1000;
const int UploadTimeoutMs = 180 * 1000;

// OKM_TRACE=1 in the environment logs every request line and reply status (desktop
// diagnostics; the password is masked).
bool traceEnabled()
{
    static int on = -1;
    if (on < 0) on = qgetenv("OKM_TRACE").isEmpty() ? 0 : 1;
    return on == 1;
}

QString masked(const QUrl &u)
{
    QString s = QString::fromLatin1(u.toEncoded());
    const int i = s.indexOf(QLatin1String("password="));
    if (i >= 0) {
        int e = s.indexOf(QLatin1Char('&'), i);
        if (e < 0) e = s.size();
        s.replace(i + 9, e - i - 9, QLatin1String("***"));
    }
    return s;
}
}

// -- OkReply ---------------------------------------------------------------------------

OkReply::OkReply(QNetworkReply *reply, bool parseJson, int timeoutMs, QObject *parent)
    : QObject(parent), m_reply(reply), m_timer(new QTimer(this)), m_parseJson(parseJson),
      m_ok(false), m_settled(false), m_code(-1), m_status(0)
{
    m_reply->setParent(this);
    connect(m_reply, SIGNAL(finished()), this, SLOT(onReplyFinished()));

    // Qt 4.7 has no request timeout of its own; a hung 3G connection would otherwise wait
    // forever. The timer aborts the reply, which makes it finish with OperationCanceledError.
    m_timer->setSingleShot(true);
    m_timer->setInterval(timeoutMs);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
    m_timer->start();
}

bool OkReply::isSessionError() const
{
    if (m_ok) return false;
    if (m_code == 102 || m_code == 103) return true;
    return m_message.contains(QLatin1String("SESSION_EXPIRED"), Qt::CaseInsensitive)
        || m_message.contains(QLatin1String("PARAM_SESSION_KEY"), Qt::CaseInsensitive);
}

void OkReply::fail(int code, const QString &message)
{
    m_ok = false;
    m_code = code;
    m_message = message;
}

void OkReply::onTimeout()
{
    if (m_settled) return;
    fail(-1, tr("timed out"));
    m_reply->abort();   // makes finished() fire; settle() sees m_message already set
}

void OkReply::onReplyFinished()
{
    if (m_settled) return;
    m_timer->stop();
    m_status = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body = m_reply->readAll();
    if (traceEnabled()) qDebug() << "OK <-" << m_status << m_reply->error() << body.left(300);

    if (m_reply->error() != QNetworkReply::NoError) {
        if (m_message.isEmpty()) {
            QString why = m_reply->errorString();
            const QString ssl = m_reply->property("okSslErrors").toString();
            if (!ssl.isEmpty()) why += QLatin1String(" (") + ssl + QLatin1Char(')');
            fail(-1, why);
        }
        // An OK error object still arrives with an HTTP error status sometimes; prefer its
        // message when there is one.
        if (m_parseJson && !body.isEmpty()) {
            bool ok = false;
            const QVariant v = OkJson::parse(body, &ok);
            if (ok && v.type() == QVariant::Map) {
                const QVariantMap m = v.toMap();
                if (m.contains(QLatin1String("error_code")))
                    fail(int(OkJson::integer(m, "error_code")), OkJson::str(m, "error_msg"));
            }
        }
        settle();
        return;
    }

    if (!m_parseJson) {
        m_ok = m_status >= 200 && m_status < 300;
        if (!m_ok) fail(m_status, QString::fromLatin1("upload failed: ") + QString::fromUtf8(body.left(200)));
        settle();
        return;
    }

    bool ok = false;
    QString err;
    const QVariant v = OkJson::parse(body, &ok, &err);
    if (!ok) {
        fail(-1, QString::fromLatin1("unparseable reply: ") + QString::fromUtf8(body.left(200)));
        settle();
        return;
    }

    if (v.type() == QVariant::Map) {
        const QVariantMap m = v.toMap();
        if (m.contains(QLatin1String("error_code"))) {
            fail(int(OkJson::integer(m, "error_code")), OkJson::str(m, "error_msg"));
            settle();
            return;
        }
        m_result = m;
    } else {
        // Some methods answer with a top-level array; wrap it so callers get a map.
        m_result.insert(QLatin1String("items"), v);
    }
    m_ok = true;
    settle();
}

void OkReply::settle()
{
    m_settled = true;
    emit finished(this);
    deleteLater();
}

// -- OkApi -----------------------------------------------------------------------------

OkApi::OkApi(OkNetwork *network, QObject *parent)
    : QObject(parent), m_net(network)
{
}

QByteArray OkApi::encode(const OkParams &params)
{
    QByteArray out;
    for (OkParams::const_iterator it = params.constBegin(); it != params.constEnd(); ++it) {
        if (!out.isEmpty()) out.append('&');
        out.append(QUrl::toPercentEncoding(it.key()));
        out.append('=');
        out.append(QUrl::toPercentEncoding(it.value()));
    }
    return out;
}

OkReply *OkApi::call(const QString &methodPath, OkParams params, const QString &secret,
                     const QString &serverBase)
{
    // Signature over what has been gathered so far, added last.
    params.insert(QLatin1String(OkConstants::ParamSignature), OkSigning::signature(params, secret));

    const QByteArray query = encode(params);
    QString base = serverBase.isEmpty() ? QLatin1String(OkConstants::ApiBase) : serverBase;
    if (!base.endsWith(QLatin1Char('/'))) base += QLatin1Char('/');
    const QString url = base + methodPath;

    // The query is already percent-encoded. It must go in through setEncodedQuery: Qt 4's
    // QUrl(QString) treats a '%' in the string as a literal and encodes it again, which turns
    // a password with '#' into "%2523" on the wire (and INVALID_CREDENTIALS).
    QUrl u(url);
    if (query.size() < 1000) u.setEncodedQuery(query);
    if (traceEnabled()) qDebug() << "OK ->" << (query.size() >= 1000 ? "POST" : "GET") << masked(u);

    QNetworkReply *reply;
    if (query.size() >= 1000) {
        // The client switches to POST for long queries; the body is the same encoding.
        QNetworkRequest req;
        req.setUrl(u);
        req.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/x-www-form-urlencoded"));
        reply = m_net->post(req, query);
    } else {
        reply = m_net->get(QNetworkRequest(u));
    }
    return new OkReply(reply, true, ApiTimeoutMs, this);
}

OkReply *OkApi::uploadFile(const QString &uploadUrl, const QByteArray &bytes, const QString &contentType,
                           const QString &fieldName, const QString &fileName, bool parseJson)
{
    // Qt 4.7 has no QHttpMultiPart; the body is assembled by hand. One file part with a
    // filename and the content type - OK reads the first file and ignores the field name.
    const QByteArray boundary = "----SimpleOKM" + QUuid::createUuid().toString().remove(QLatin1Char('{'))
                                                 .remove(QLatin1Char('}')).remove(QLatin1Char('-')).toLatin1();
    QByteArray body;
    body.append("--" + boundary + "\r\n");
    body.append("Content-Disposition: form-data; name=\"" + fieldName.toUtf8() + "\"; filename=\"" + fileName.toUtf8() + "\"\r\n");
    body.append("Content-Type: " + contentType.toLatin1() + "\r\n\r\n");
    body.append(bytes);
    body.append("\r\n--" + boundary + "--\r\n");

    // The upload url carries percent-encoded parameters (apiToken=...%3D...). It must go in
    // through fromEncoded: Qt 4's QUrl(QString) encodes the '%' again and the upload server
    // answers BAD_REQUEST to the mangled token.
    QNetworkRequest req;
    req.setUrl(QUrl::fromEncoded(uploadUrl.toUtf8()));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QString::fromLatin1("multipart/form-data; boundary=") + QString::fromLatin1(boundary));
    req.setHeader(QNetworkRequest::ContentLengthHeader, body.size());
    QNetworkReply *reply = m_net->post(req, body);
    return new OkReply(reply, parseJson, UploadTimeoutMs, this);
}
