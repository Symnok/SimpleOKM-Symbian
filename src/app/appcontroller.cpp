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
#include "appcontroller.h"
#include "conversationsmodel.h"
#include "messagesmodel.h"
#include "friendsmodel.h"
#include "oknetwork.h"
#include "okapi.h"
#include "oksession.h"
#include "okconstants.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDeclarativeView>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QLocale>
#include <QRegExp>
#include <QStringList>
#include <QNetworkConfigurationManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkSession>
#include <QTimer>
#include <QUrl>
#include <QDebug>

#ifndef APP_VERSION
#define APP_VERSION 0.0.0
#endif

namespace {
const int ListPollIntervalMs = 20 * 1000;
const char *const KeyToken = "ok/token";
const char *const KeyLogin = "ok/login";
const char *const KeyLoadImages = "ui/loadImages";
const char *const KeyLanguage = "ui/language";
}

AppController::AppController(QObject *parent)
    : QObject(parent),
      m_settings(QLatin1String("SimpleOKM"), QLatin1String("SimpleOKM")),
      m_net(new OkNetwork(this)),
      m_netMgr(0), m_netSession(0), m_listPoll(new QTimer(this)), m_view(0),
      m_state(QLatin1String("starting")), m_busy(false), m_listRefreshInFlight(false)
{
    m_api = new OkApi(m_net, this);
    m_session = new OkSession(m_api, this);
    m_conversations = new ConversationsModel(m_session, this);
    m_chat = new MessagesModel(m_session, this);
    m_friends = new FriendsModel(m_session, this);

    connect(m_session, SIGNAL(loginFinished(bool,QString,QString)), this, SLOT(onLoginFinished(bool,QString,QString)));
    connect(m_session, SIGNAL(credentialsChanged()), this, SLOT(onCredentialsChanged()));
    connect(m_session, SIGNAL(sessionLost(QString)), this, SLOT(onSessionLost(QString)));
    connect(m_session, SIGNAL(conversationsRefreshed(bool,QString)), this, SLOT(onConversationsRefreshed(bool,QString)));
    connect(m_session, SIGNAL(conversationDeleted(QString,bool,QString)), this, SLOT(onConversationDeleted(QString,bool,QString)));
    connect(m_chat, SIGNAL(sendFailedNotice(QString)), this, SLOT(onSendFailedNotice(QString)));

    m_listPoll->setInterval(ListPollIntervalMs);
    connect(m_listPoll, SIGNAL(timeout()), this, SLOT(onListPoll()));
}

AppController::~AppController()
{
    if (m_netSession) m_netSession->close();
}

// -- properties --

void AppController::setState(const QString &s)
{
    if (m_state == s) return;
    m_state = s;
    emit stateChanged();
}

void AppController::setBusy(bool b)
{
    if (m_busy == b) return;
    m_busy = b;
    emit busyChanged();
}

void AppController::setNotice(const QString &n)
{
    m_notice = n;
    emit noticeChanged();
}

void AppController::clearNotice()
{
    setNotice(QString());
}

QString AppController::myName() const
{
    return m_session->me().displayName();
}

QString AppController::savedLogin() const
{
    return m_settings.value(QLatin1String(KeyLogin)).toString();
}

QString AppController::version() const
{
    return QLatin1String(OK_STRINGIFY(APP_VERSION));
}

bool AppController::sslSupported() const
{
    return OkNetwork::sslSupported();
}

bool AppController::loadImages() const
{
    return m_settings.value(QLatin1String(KeyLoadImages), true).toBool();
}

void AppController::setLoadImages(bool on)
{
    m_settings.setValue(QLatin1String(KeyLoadImages), on);
    emit settingsChanged();
}

QString AppController::language() const
{
    return m_settings.value(QLatin1String(KeyLanguage)).toString();
}

void AppController::setLanguage(const QString &lang)
{
    if (language() == lang) return;
    m_settings.setValue(QLatin1String(KeyLanguage), lang);
    emit settingsChanged();
    setNotice(tr("The language changes the next time the app starts."));
}

QString AppController::effectiveLanguage(const QSettings &settings)
{
    const QString chosen = settings.value(QLatin1String(KeyLanguage)).toString();
    if (!chosen.isEmpty()) return chosen;
    const QString sys = QLocale::system().name().left(2).toLower();
    return (sys == QLatin1String("ru") || sys == QLatin1String("uk")) ? sys : QString::fromLatin1("en");
}

// -- start-up: network, then token --

void AppController::start()
{
    setState(QLatin1String("starting"));
    setBusy(true);

    // On the phone a socket without an open QNetworkSession either fails or prompts for an
    // access point on every request. Open the default configuration once, up front, and
    // hand it to the access manager so every request rides on it. A desktop build (or the
    // simulator) has no usable default configuration; then this is skipped.
    m_netMgr = new QNetworkConfigurationManager(this);
    const QNetworkConfiguration cfg = m_netMgr->defaultConfiguration();
    if (!cfg.isValid() || !(m_netMgr->capabilities() & QNetworkConfigurationManager::NetworkSessionRequired)) {
        continueStart();
        return;
    }
    OkNetwork::setDefaultConfiguration(cfg);
    m_net->setConfiguration(cfg);
    m_netSession = new QNetworkSession(cfg, this);
    connect(m_netSession, SIGNAL(opened()), this, SLOT(onNetworkOpened()));
    connect(m_netSession, SIGNAL(error(QNetworkSession::SessionError)), this, SLOT(onNetworkError()));
    m_netSession->open();
}

void AppController::onNetworkOpened()
{
    continueStart();
}

void AppController::onNetworkError()
{
    // The session could not be opened (no access point chosen, offline). Requests will
    // still be attempted - QNAM prompts for a connection itself on Symbian - so carry on.
    qWarning() << "network session error:" << (m_netSession ? m_netSession->errorString() : QString());
    continueStart();
}

void AppController::continueStart()
{
    static bool started = false;
    if (started) return;
    started = true;

    const QString token = m_settings.value(QLatin1String(KeyToken)).toString();
    if (token.isEmpty()) {
#ifndef Q_OS_SYMBIAN
        // Desktop/simulator testing: OKM_CREDS_FILE names a file with the login on line 1
        // and the password on line 2, and the app signs in with it by itself.
        const QByteArray credsFile = qgetenv("OKM_CREDS_FILE");
        if (!credsFile.isEmpty()) {
            QFile f(QString::fromLocal8Bit(credsFile));
            if (f.open(QIODevice::ReadOnly)) {
                const QStringList lines = QString::fromUtf8(f.readAll()).split(QRegExp(QLatin1String("\r?\n")));
                if (lines.size() >= 2) {
                    setState(QLatin1String("login"));
                    login(lines.at(0).trimmed(), lines.at(1).trimmed());
                    return;
                }
            }
        }
#endif
        setBusy(false);
        setState(QLatin1String("login"));
        return;
    }
    m_session->loginByToken(token);
}

// -- login --

void AppController::login(const QString &login, const QString &password)
{
    if (login.trimmed().isEmpty() || password.isEmpty()) {
        m_loginError = tr("Enter your login and password.");
        m_verificationUrl.clear();
        emit loginErrorChanged();
        return;
    }
    m_loginError.clear();
    m_verificationUrl.clear();
    emit loginErrorChanged();
    m_pendingLogin = login.trimmed();
    setBusy(true);
    m_session->loginByPassword(m_pendingLogin, password);
}

void AppController::openVerification()
{
    if (!m_verificationUrl.isEmpty()) openUrl(m_verificationUrl);
}

void AppController::onLoginFinished(bool ok, const QString &error, const QString &verificationUrl)
{
    setBusy(false);
    if (!ok) {
        m_verificationUrl = verificationUrl;
        if (!verificationUrl.isEmpty())
            m_loginError = tr("OK wants a one-time verification before it lets this device in. Finish it in the browser, then sign in again.");
        else if (m_state == QLatin1String("starting"))
            m_loginError = tr("The saved session could not be restored: %1").arg(error);
        else
            m_loginError = tr("OK refused the sign-in: %1").arg(error);
        emit loginErrorChanged();
        setState(QLatin1String("login"));
        return;
    }
    if (!m_pendingLogin.isEmpty()) {
        m_settings.setValue(QLatin1String(KeyLogin), m_pendingLogin);
        m_pendingLogin.clear();
    }
    m_loginError.clear();
    m_verificationUrl.clear();
    emit loginErrorChanged();
    setState(QLatin1String("ready"));
    refreshConversations();
    m_listPoll->start();
}

void AppController::onCredentialsChanged()
{
    m_settings.setValue(QLatin1String(KeyToken), m_session->credentials().token);
    m_settings.sync();
}

void AppController::onSessionLost(const QString &error)
{
    // The token no longer works (revoked, or OK wants verification): back to sign-in.
    m_listPoll->stop();
    m_chat->close();
    m_settings.remove(QLatin1String(KeyToken));
    m_session->reset();
    m_loginError = tr("The session has expired: %1").arg(error);
    m_verificationUrl.clear();
    emit loginErrorChanged();
    setState(QLatin1String("login"));
}

void AppController::signOut()
{
    m_listPoll->stop();
    m_chat->close();
    m_settings.remove(QLatin1String(KeyToken));
    m_settings.sync();
    m_session->reset();
    m_loginError.clear();
    m_verificationUrl.clear();
    emit loginErrorChanged();
    emit myNameChanged();
    setState(QLatin1String("login"));
}

// -- conversations --

void AppController::refreshConversations()
{
    if (m_state != QLatin1String("ready") || m_listRefreshInFlight) return;
    m_listRefreshInFlight = true;
    setBusy(true);
    m_session->refreshConversations();
}

void AppController::onListPoll()
{
    if (m_listRefreshInFlight) return;
    m_listRefreshInFlight = true;
    m_session->refreshConversations();
}

void AppController::setListPolling(bool on)
{
    if (on && m_state == QLatin1String("ready")) m_listPoll->start();
    else m_listPoll->stop();
}

void AppController::onConversationsRefreshed(bool ok, const QString &error)
{
    m_listRefreshInFlight = false;
    setBusy(false);
    const QString e = ok ? QString() : error;
    if (e != m_listError) { m_listError = e; emit listErrorChanged(); }
    emit myNameChanged();
    emit conversationsRefreshed();
}

void AppController::deleteConversation(const QString &conversationId)
{
    if (m_chat->conversationId() == conversationId) m_chat->close();
    m_session->deleteConversation(conversationId);
}

void AppController::onConversationDeleted(const QString &, bool ok, const QString &error)
{
    if (!ok) setNotice(tr("Could not delete: %1").arg(error));
}

void AppController::onSendFailedNotice(const QString &error)
{
    setNotice(tr("Not sent: %1").arg(error));
}

// -- host services --

void AppController::openUrl(const QString &url)
{
    if (url.isEmpty()) return;
    // fromEncoded: server urls are already percent-encoded (see OkApi::uploadFile).
    QDesktopServices::openUrl(QUrl::fromEncoded(url.toUtf8()));
}

void AppController::copyText(const QString &text)
{
    QApplication::clipboard()->setText(text);
    setNotice(tr("Copied"));
}

QString AppController::imagesFolder() const
{
    QString dir = QDesktopServices::storageLocation(QDesktopServices::PicturesLocation);
#ifdef Q_OS_SYMBIAN
    // Prefer the memory card's Images folder when there is one; the gallery indexes both.
    if (QDir(QLatin1String("E:/")).exists()) dir = QLatin1String("E:/Images");
    else dir = QLatin1String("C:/Data/Images");
#endif
    if (dir.isEmpty()) dir = QDir::homePath();
    return dir;
}

QString AppController::pickImage()
{
    const QString path = QFileDialog::getOpenFileName(m_view, tr("Choose a photo"), imagesFolder(),
                                                      tr("Images (*.jpg *.jpeg *.png)"));
    return path;
}

void AppController::saveImage(const QString &url)
{
    if (url.isEmpty()) return;
    QNetworkRequest req;
    req.setUrl(QUrl::fromEncoded(url.toUtf8()));
    QNetworkReply *reply = m_net->get(req);
    connect(reply, SIGNAL(finished()), this, SLOT(onImageDownloaded()));
    setNotice(tr("Saving..."));
}

void AppController::onImageDownloaded()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        setNotice(tr("Could not save: %1").arg(reply->errorString()));
        return;
    }
    const QByteArray bytes = reply->readAll();
    QDir dir(imagesFolder() + QLatin1String("/SimpleOKM"));
    if (!dir.exists()) dir.mkpath(QLatin1String("."));
    const QString ext = bytes.startsWith("\x89PNG") ? QLatin1String("png") : QLatin1String("jpg");
    const QString name = QLatin1String("ok_") + QDateTime::currentDateTime().toString(QLatin1String("yyyyMMdd_HHmmss")) + QLatin1Char('.') + ext;
    QFile f(dir.filePath(name));
    if (!f.open(QIODevice::WriteOnly) || f.write(bytes) != bytes.size()) {
        setNotice(tr("Could not save: %1").arg(f.errorString()));
        return;
    }
    setNotice(tr("Saved to %1").arg(QDir::toNativeSeparators(f.fileName())));
}
