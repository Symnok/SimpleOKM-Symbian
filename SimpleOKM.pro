# SimpleOKM-Symbian - an Odnoklassniki messaging client for Symbian Anna/Belle.
# Copyright (C) 2026 - GPL-2.0-or-later, see LICENSE.
#
# Build with the "Qt 4.7.4 for Symbian Anna/Belle" (SymbianSR1Qt474) kit for the phone, or
# with the Qt Simulator kit to run it on the PC. The protocol core (core.pri) is shared with
# the desktop harness in tools/okm-cli.

TEMPLATE = app
TARGET = SimpleOKM
VERSION = 1.0.0

QT += core gui network declarative

include(core.pri)

INCLUDEPATH += src/app

HEADERS += \
    src/app/appcontroller.h \
    src/app/conversationsmodel.h \
    src/app/messagesmodel.h \
    src/app/friendsmodel.h

SOURCES += \
    src/main.cpp \
    src/app/appcontroller.cpp \
    src/app/conversationsmodel.cpp \
    src/app/messagesmodel.cpp \
    src/app/friendsmodel.cpp

RESOURCES += qml.qrc translations.qrc

# The version reaches the About page as an unquoted macro (stringified in code), which
# survives every generator's quoting rules.
DEFINES += APP_VERSION=$$VERSION

TRANSLATIONS += \
    translations/simpleokm_ru.ts \
    translations/simpleokm_uk.ts

OTHER_FILES += \
    qml/*.qml \
    README.md \
    credentials.cfg.template

symbian {
    # Unprotected range: installs self-signed without Symbian Signed.
    TARGET.UID3 = 0xE31A0C4B
    TARGET.CAPABILITY += NetworkServices ReadUserData WriteUserData
    # 128 MB max heap: photos are decoded for upload and QML keeps image caches.
    TARGET.EPOCHEAPSIZE = 0x020000 0x8000000
    TARGET.EPOCSTACKSIZE = 0x14000
    ICON = icon.svg

    # Qt Quick Components for Symbian (built into Belle; Anna gets them through the Smart
    # Installer package, SimpleOKM_installer.sis).
    CONFIG += qt-components
    DEPLOYMENT.installer_header = 0x2002CCCF

    vendorinfo = \
        "; Localised and unique vendor names" \
        "%{\"Symnok\"}" \
        ":\"Symnok\""
    packageheader = "$${LITERAL_HASH}{\"SimpleOKM\"},(0xE31A0C4B),1,0,0,TYPE=SA,RU"
    deployment.pkg_prerules += packageheader vendorinfo
    DEPLOYMENT += deployment
}

simulator {
    DEFINES += Q_WS_SIMULATOR
}

CODECFORTR = UTF-8
CODECFORSRC = UTF-8
