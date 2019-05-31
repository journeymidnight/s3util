QT += core
QT += concurrent
QT -= gui

TARGET = s3util
CONFIG += console
CONFIG += c++11
CONFIG -= app_bundle
CONFIG += release
QMAKE_RPATHDIR += lib

TEMPLATE = app
mac {
  PKG_CONFIG = /usr/local/bin/pkg-config
}
QT_CONFIG -= no-pkg-config
CONFIG += link_pkgconfig
unix: PKGCONFIG += aws-cpp-sdk-s3
unix: INCLUDEPATH += /usr/local
unix: QMAKE_LFLAGS += "-Wl,-rpath,\'\$$ORIGIN/libs\'"

SOURCES += main.cpp \
    qs3client.cpp \
    s3consolemanager.cpp \
    actions.cpp \
    config.cpp

HEADERS += \
    actions.h \
    qs3client.h \
    s3consolemanager.h \
    config.h \
    cli.h

