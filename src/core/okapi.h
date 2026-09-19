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
// The transport: turns a method name and a bag of parameters into a signed request and hands
// back the parsed JSON. Follows Ok.Api.ApiRequest.BuildNetworkRequest - the URL is the server
// base plus the method path, parameters go in the query string, and a query of 1000
// characters or more is moved into a POST body instead.
//
// There is no async/await here, so each call is an OkReply object: connect to finished(),
// read the result in the slot. It deletes itself after the signal.
#ifndef OKAPI_H
#define OKAPI_H

#include "oksigning.h"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QVariantMap>

class OkNetwork;
class QNetworkReply;
class QTimer;

/// One request in flight. ok() is false for an OK error object ({"error_code","error_msg"}),
/// a network failure (code -1) or an unparseable reply (code -1).
class OkReply : public QObject
{
    Q_OBJECT
public:
    bool ok() const { return m_ok; }
    int errorCode() const { return m_code; }
    QString errorMessage() const { return m_message; }
    /// The reply object; a top-level array is wrapped as {"items": [...]}.
    const QVariantMap &result() const { return m_result; }
    int httpStatus() const { return m_status; }
    /// True for PARAM_SESSION_EXPIRED (102) / PARAM_SESSION_KEY (103) - the session is dead.
    bool isSessionError() const;

signals:
    void finished(OkReply *reply);

private slots:
    void onReplyFinished();
    void onTimeout();

private:
    friend class OkApi;
    OkReply(QNetworkReply *reply, bool parseJson, int timeoutMs, QObject *parent);
    void fail(int code, const QString &message);
    void settle();

    QNetworkReply *m_reply;
    QTimer *m_timer;
    bool m_parseJson;
    bool m_ok;
    bool m_settled;
    int m_code;
    int m_status;
    QString m_message;
    QVariantMap m_result;
};

class OkApi : public QObject
{
    Q_OBJECT
public:
    explicit OkApi(OkNetwork *network, QObject *parent = 0);

    OkNetwork *network() const { return m_net; }

    /// Signs and sends one call. secret is the application secret for login and the
    /// session secret for everything after; serverBase is the api_server login returned, or
    /// ApiBase before that.
    OkReply *call(const QString &methodPath, OkParams params, const QString &secret,
                  const QString &serverBase);

    /// POSTs bytes as one multipart/form-data file part to an absolute upload url (the one
    /// photosV2.getUploadUrl / video.getUploadUrl hands back - it carries its own auth). A raw
    /// octet body is rejected by OK's upload servers, hence multipart. With parseJson the
    /// reply is parsed like an API reply (photos); media upload answers "<retval>1</retval>",
    /// so there parseJson is false and a 2xx is the whole result.
    OkReply *uploadFile(const QString &uploadUrl, const QByteArray &bytes, const QString &contentType,
                        const QString &fieldName, const QString &fileName, bool parseJson);

    static QByteArray encode(const OkParams &params);

private:
    OkNetwork *m_net;
};

#endif // OKAPI_H
