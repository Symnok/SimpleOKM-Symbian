# The protocol core shared by the phone app (SimpleOKM.pro) and the desktop harness
# (tools/okm-cli). Include it after setting nothing; it adds its sources, QtNetwork and the
# OK_APP_KEY / OK_APP_SECRET defines read from credentials.cfg.

QT += core network

INCLUDEPATH += $$PWD/src/core

HEADERS += \
    $$PWD/src/core/okjson.h \
    $$PWD/src/core/okconstants.h \
    $$PWD/src/core/oksigning.h \
    $$PWD/src/core/okmodels.h \
    $$PWD/src/core/okparser.h \
    $$PWD/src/core/oknetwork.h \
    $$PWD/src/core/okapi.h \
    $$PWD/src/core/oksession.h \
    $$PWD/src/core/oksession_p.h

SOURCES += \
    $$PWD/src/core/okjson.cpp \
    $$PWD/src/core/oksigning.cpp \
    $$PWD/src/core/okparser.cpp \
    $$PWD/src/core/oknetwork.cpp \
    $$PWD/src/core/okapi.cpp \
    $$PWD/src/core/oksession.cpp

RESOURCES += $$PWD/certs.qrc

# credentials.cfg: KEY=VALUE lines (see credentials.cfg.template). qmake's $$cat splits on
# whitespace, so a value must not contain spaces - the key and secret never do. The values
# reach the code as unquoted macros and are stringified in okconstants.h, which sidesteps the
# quote-escaping differences between the Symbian (sbsv2) and MinGW generators.
CREDS_FILE = $$PWD/credentials.cfg
!exists($$CREDS_FILE) {
    error("credentials.cfg is missing - copy credentials.cfg.template to credentials.cfg and fill in the values")
}
CREDS = $$cat($$CREDS_FILE)
for(tok, CREDS) {
    contains(tok, "OK_APP_KEY=.*")    { DEFINES += $$tok }
    contains(tok, "OK_APP_SECRET=.*") { DEFINES += $$tok }
}
!contains(DEFINES, "OK_APP_KEY=.*")    { error("credentials.cfg has no OK_APP_KEY") }
!contains(DEFINES, "OK_APP_SECRET=.*") { error("credentials.cfg has no OK_APP_SECRET") }
