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
#include "oknetwork.h"
#include "okconstants.h"

#include <QFile>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslCertificate>
#include <QSslSocket>
#include <QStringList>
#include <QUrl>
#include <QDebug>

QString OkNetwork::s_rewritePrefix;
QNetworkConfiguration OkNetwork::s_config;
bool OkNetwork::s_hasConfig = false;

OkNetwork::OkNetwork(QObject *parent)
    : QNetworkAccessManager(parent)
{
    m_ssl = QSslConfiguration::defaultConfiguration();

    // The bundled roots replace the system store for our requests: everything the app talks
    // to chains to one of them, and the system store (2011) has none of them.
    QFile pem(QLatin1String(":/certs/ok-roots.pem"));
    if (pem.open(QIODevice::ReadOnly)) {
        const QList<QSslCertificate> roots = QSslCertificate::fromData(pem.readAll(), QSsl::Pem);
        if (!roots.isEmpty()) {
            m_ssl.setCaCertificates(roots);
        } else {
            qWarning("OkNetwork: no certificates in ok-roots.pem");
        }
    } else {
        qWarning("OkNetwork: ok-roots.pem missing from resources");
    }
    m_ssl.setPeerVerifyMode(QSslSocket::VerifyPeer);
    // AnyProtocol is OpenSSL's SSLv23_client_method: it negotiates the highest version both
    // sides have, i.e. TLS 1.2 with the patched QtNetwork. (TlsV1 would pin TLS 1.0, which
    // ok.ru no longer accepts.) Qt 4.8's SecureProtocols is the same with SSLv2/3 disabled.
#if QT_VERSION >= 0x040800
    m_ssl.setProtocol(QSsl::SecureProtocols);
#else
    m_ssl.setProtocol(QSsl::AnyProtocol);
#endif

    if (s_hasConfig) setConfiguration(s_config);

    connect(this, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)),
            this, SLOT(onSslErrors(QNetworkReply*,QList<QSslError>)));
}

bool OkNetwork::sslSupported()
{
    return QSslSocket::supportsSsl();
}

void OkNetwork::setDefaultConfiguration(const QNetworkConfiguration &cfg)
{
    s_config = cfg;
    s_hasConfig = cfg.isValid();
}

void OkNetwork::setUrlRewritePrefix(const QString &prefix)
{
    s_rewritePrefix = prefix;
}

QUrl OkNetwork::rewrite(const QUrl &url)
{
    if (s_rewritePrefix.isEmpty() || url.scheme() != QLatin1String("https")) return url;
    QString rest = url.toString(QUrl::RemoveScheme);   // "//host/path?query"
    while (rest.startsWith(QLatin1Char('/'))) rest.remove(0, 1);
    return QUrl(s_rewritePrefix + rest);
}

QNetworkReply *OkNetwork::createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
{
    QNetworkRequest r(request);
    r.setUrl(rewrite(request.url()));
    if (!r.hasRawHeader("User-Agent"))
        r.setRawHeader("User-Agent", OkConstants::UserAgent);
    if (r.url().scheme() == QLatin1String("https"))
        r.setSslConfiguration(m_ssl);
    return QNetworkAccessManager::createRequest(op, r, outgoingData);
}

void OkNetwork::onSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    // Never ignored - a failed verification fails the request. The reasons are kept on the
    // reply so OkReply can show something better than "SSL handshake failed".
    QStringList texts;
    for (int i = 0; i < errors.size(); ++i) texts.append(errors.at(i).errorString());
    reply->setProperty("okSslErrors", texts.join(QLatin1String("; ")));
    qWarning() << "OkNetwork: TLS verification failed for" << reply->url().host() << ":" << texts;
}
