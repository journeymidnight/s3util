QT += core
QT -= gui

TARGET = S3Client
CONFIG += console
CONFIG += c++11
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    qs3client.cpp \
    s3consolemanager.cpp \
    actions.cpp


unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += aws-cpp-sdk-s3
unix: PKGCONFIG += aws-cpp-sdk-transfer

HEADERS += \
    qs3client.h \
    s3consolemanager.h \
    actions.h

