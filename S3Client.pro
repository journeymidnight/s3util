QT += core
QT += concurrent
QT -= gui

TARGET = s3util
CONFIG += console
CONFIG += c++11
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    qs3client.cpp \
    s3consolemanager.cpp \
    actions.cpp \
    qlogs3.cpp


unix: INCLUDE_LIBDIR += /usr/local/include
unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += aws-cpp-sdk-s3
unix: QMAKE_LFLAGS += "-Wl,-rpath,'$$ORIGIN/lib'"


HEADERS += \
    qs3client.h \
    s3consolemanager.h \
    actions.h \
    qlogs3.h

