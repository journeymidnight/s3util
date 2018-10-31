#include <QCoreApplication>
#include <QTimer>
#include "s3consolemanager.h"
#include <QStringList>
#include <QDebug>
#include <iostream>
#include <QObject>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    S3ConsoleManager m;
    m.args = a.arguments();
    QObject::connect(&m,SIGNAL(Finished()),&a,SLOT(quit()));
    QTimer::singleShot(0, &m, SLOT(Execute()));
//    QCoreApplication::quit();
    return a.exec();
}

