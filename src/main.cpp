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

#include <QApplication>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDeclarativeNetworkAccessManagerFactory>
#include <QDeclarativeView>
#include <QSettings>
#include <QTranslator>
#include <QUrl>
#include <QtDeclarative>

/// QML's Image elements load through their own access manager (on a separate thread), so
/// avatars and photos must get the same trust bundle and User-Agent as the API calls.
class NetworkFactory : public QDeclarativeNetworkAccessManagerFactory
{
public:
    QNetworkAccessManager *create(QObject *parent) { return new OkNetwork(parent); }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QLatin1String("SimpleOKM"));
    app.setOrganizationName(QLatin1String("SimpleOKM"));

    // Translations: the chosen language, or the phone's, folded to what we ship.
    QSettings settings(QLatin1String("SimpleOKM"), QLatin1String("SimpleOKM"));
    const QString lang = AppController::effectiveLanguage(settings);
    QTranslator translator;
    if (lang != QLatin1String("en") && translator.load(QLatin1String(":/translations/simpleokm_") + lang))
        app.installTranslator(&translator);

    qmlRegisterType<ConversationsModel>();
    qmlRegisterType<MessagesModel>();
    qmlRegisterType<FriendsModel>();

    AppController controller;
    NetworkFactory factory;

    QDeclarativeView view;
    view.engine()->setNetworkAccessManagerFactory(&factory);
    view.setResizeMode(QDeclarativeView::SizeRootObjectToView);
    view.rootContext()->setContextProperty(QLatin1String("app"), &controller);
    view.rootContext()->setContextProperty(QLatin1String("uiLanguage"), lang);
    controller.setView(&view);
    view.setSource(QUrl(QLatin1String("qrc:/qml/main.qml")));

#if defined(Q_OS_SYMBIAN) || defined(Q_WS_SIMULATOR)
    view.setAttribute(Qt::WA_LockPortraitOrientation, true);
    view.showFullScreen();
#else
    view.resize(360, 640);
    view.show();
#endif

    controller.start();
    return app.exec();
}
