#include <QCoreApplication>
#include <QTimer>
#include "s3consolemanager.h"
#include <QStringList>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);


    S3ConsoleManager m;
    QTimer::singleShot(0, &m, SLOT(Execute()));

    return a.exec();
}

