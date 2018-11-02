QT += core
QT += concurrent
QT -= gui

TARGET = s3util
CONFIG += console
CONFIG += c++11
CONFIG -= app_bundle

TEMPLATE = app
mac {
  PKG_CONFIG = /usr/local/bin/pkg-config
}
QT_CONFIG -= no-pkg-config
CONFIG += link_pkgconfig
PKGCONFIG += aws-cpp-sdk-s3
unix: QMAKE_LFLAGS += "-Wl,-rpath,'$$ORIGIN/lib'"

SOURCES += main.cpp \
    qs3client.cpp \
    s3consolemanager.cpp \
    actions.cpp \
    qlogs3.cpp \
    config.cpp

HEADERS += \
    actions.h \
    qlogs3.h \
    qs3client.h \
    s3consolemanager.h \
    config.h \
    cli.h

