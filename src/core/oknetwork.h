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
// The one QNetworkAccessManager everything goes through - API calls, uploads and the QML
// Image loads of avatars and photos. It exists for the same reason OKLumessenger has a
// TlsHttpHandler: the phone's 2011 certificate store cannot verify ok.ru (HARICA 2021), so
// every request is stamped with the app's own trust bundle (certs/ok-roots.pem), and with the
// client User-Agent. TLS 1.2 itself comes from the patched QtNetwork on the phone
// (nnproject.cc/qtls); this class only supplies the roots it has no way of knowing.
#ifndef OKNETWORK_H
#define OKNETWORK_H

#include <QNetworkAccessManager>
#include <QNetworkConfiguration>
#include <QSslConfiguration>
#include <QString>

class OkNetwork : public QNetworkAccessManager
{
    Q_OBJECT
public:
    explicit OkNetwork(QObject *parent = 0);

    /// The SSL configuration with the bundled roots, for anything that builds a request
    /// outside this manager.
    QSslConfiguration sslConfiguration() const { return m_ssl; }

    /// Whether the Qt build can do TLS at all (on the phone: whether QtNetwork was built
    /// with OpenSSL). Shown on the About screen.
    static bool sslSupported();

    /// The bearer configuration (the phone's access point) every manager - including the
    /// ones QML creates for images - should use. Set once by the controller.
    static void setDefaultConfiguration(const QNetworkConfiguration &cfg);

    /// Desktop testing hook: when set, "https://host/path" becomes "<prefix>host/path"
    /// (e.g. "http://127.0.0.1:8080/") so the whole stack can be exercised through a local
    /// TLS-terminating forwarder. Never set on the phone.
    static void setUrlRewritePrefix(const QString &prefix);
    static QUrl rewrite(const QUrl &url);

protected:
    QNetworkReply *createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData);

private slots:
    void onSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);

private:
    QSslConfiguration m_ssl;
    static QString s_rewritePrefix;
    static QNetworkConfiguration s_config;
    static bool s_hasConfig;
};

#endif // OKNETWORK_H
