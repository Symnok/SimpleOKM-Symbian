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
// Process-wide state, exposed to QML as "app": the network session (the Symbian access
// point), the signed-in OkSession, the models the pages bind to, settings, and the host
// services QML cannot do itself (file picker, saving an image, opening a url, clipboard).
#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QSettings>
#include <QString>

class OkNetwork;
class OkApi;
class OkSession;
class ConversationsModel;
class MessagesModel;
class FriendsModel;
class QNetworkConfigurationManager;
class QNetworkSession;
class QNetworkReply;
class QTimer;
class QDeclarativeView;

class AppController : public QObject
{
    Q_OBJECT
    /// "starting" (opening the network / token login), "login" (sign-in page) or "ready".
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString loginError READ loginError NOTIFY loginErrorChanged)
    Q_PROPERTY(QString verificationUrl READ verificationUrl NOTIFY loginErrorChanged)
    Q_PROPERTY(QString listError READ listError NOTIFY listErrorChanged)
    Q_PROPERTY(QString myName READ myName NOTIFY myNameChanged)
    Q_PROPERTY(QString savedLogin READ savedLogin CONSTANT)
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(bool sslSupported READ sslSupported CONSTANT)
    Q_PROPERTY(bool loadImages READ loadImages WRITE setLoadImages NOTIFY settingsChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY settingsChanged)
    Q_PROPERTY(ConversationsModel *conversations READ conversations CONSTANT)
    Q_PROPERTY(MessagesModel *chat READ chat CONSTANT)
    Q_PROPERTY(FriendsModel *friends READ friends CONSTANT)
    Q_PROPERTY(QString notice READ notice NOTIFY noticeChanged)
public:
    explicit AppController(QObject *parent = 0);
    ~AppController();

    OkNetwork *network() const { return m_net; }
    void setView(QDeclarativeView *view) { m_view = view; }

    QString state() const { return m_state; }
    bool busy() const { return m_busy; }
    QString loginError() const { return m_loginError; }
    QString verificationUrl() const { return m_verificationUrl; }
    QString listError() const { return m_listError; }
    QString myName() const;
    QString savedLogin() const;
    QString version() const;
    bool sslSupported() const;
    bool loadImages() const;
    void setLoadImages(bool on);
    QString language() const;
    void setLanguage(const QString &lang);
    ConversationsModel *conversations() const { return m_conversations; }
    MessagesModel *chat() const { return m_chat; }
    FriendsModel *friends() const { return m_friends; }
    QString notice() const { return m_notice; }

    /// Opens the network and either logs in with the saved token or shows the sign-in page.
    void start();

    /// The language the UI should use: the setting, or the phone's, folded to en/ru.
    static QString effectiveLanguage(const QSettings &settings);

public slots:
    void login(const QString &login, const QString &password);
    void openVerification();
    void signOut();
    void refreshConversations();
    void deleteConversation(const QString &conversationId);
    void openUrl(const QString &url);
    void copyText(const QString &text);
    /// Shows the platform file dialog; returns the chosen image path or "".
    QString pickImage();
    /// Downloads an image url into the phone's Images folder; reports through notice.
    void saveImage(const QString &url);
    void setListPolling(bool on);
    void clearNotice();

signals:
    void stateChanged();
    void busyChanged();
    void loginErrorChanged();
    void listErrorChanged();
    void myNameChanged();
    void settingsChanged();
    void noticeChanged();
    void conversationsRefreshed();

private slots:
    void onNetworkOpened();
    void onNetworkError();
    void onLoginFinished(bool ok, const QString &error, const QString &verificationUrl);
    void onCredentialsChanged();
    void onSessionLost(const QString &error);
    void onConversationsRefreshed(bool ok, const QString &error);
    void onConversationDeleted(const QString &conversationId, bool ok, const QString &error);
    void onSendFailedNotice(const QString &error);
    void onListPoll();
    void onImageDownloaded();

private:
    void setState(const QString &s);
    void setBusy(bool b);
    void setNotice(const QString &n);
    void continueStart();
    QString imagesFolder() const;

    QSettings m_settings;
    OkNetwork *m_net;
    OkApi *m_api;
    OkSession *m_session;
    ConversationsModel *m_conversations;
    MessagesModel *m_chat;
    FriendsModel *m_friends;
    QNetworkConfigurationManager *m_netMgr;
    QNetworkSession *m_netSession;
    QTimer *m_listPoll;
    QDeclarativeView *m_view;
    QString m_state;
    bool m_busy;
    bool m_listRefreshInFlight;
    QString m_loginError;
    QString m_verificationUrl;
    QString m_listError;
    QString m_notice;
    QString m_pendingLogin;
};

#endif // APPCONTROLLER_H
