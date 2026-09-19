# Desktop harness for the protocol core (see main.cpp). Build with the Desktop Qt 4.8.1
# MinGW kit from the Qt SDK; needs OpenSSL 1.0.x DLLs (libeay32/ssleay32) next to the exe
# for TLS 1.2, or OKM_PROXY with tools/tls-forwarder.py.
TEMPLATE = app
TARGET = okm-cli
CONFIG += console
CONFIG -= app_bundle
QT -= gui

include(../../core.pri)

SOURCES += main.cpp
