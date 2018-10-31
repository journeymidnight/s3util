#include <QCoreApplication>
#include <QTimer>
#include "s3consolemanager.h"
#include <QStringList>
#include <QDebug>
#include <iostream>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    qWarning() << "enter main" << "\n";

    S3ConsoleManager m;
//    QTimer::singleShot(0, &m, SLOT(Execute()));
    m.Execute();
//    QCoreApplication::quit();
    return 0;
}

