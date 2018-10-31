QT += core
QT += concurrent
QT -= gui

TARGET = S3Client
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

SOURCES += main.cpp \
    qs3client.cpp \
    s3consolemanager.cpp \
    actions.cpp \
    qlogs3.cpp

