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
    QStringList x;
    x << "/" << "" <<  "/bucketname/dir1/obj" << "bucket" << "/bucketname/dir1/" << "/yo/";
    for(auto &s : x) {
        QStringList parts = s.split("/");
        if (parts.count() >= 2 && parts.last() == "") {
            qDebug() << "good:" << s;
        } else {
            continue;
        }

        qDebug() << "bucket name:" << parts[1];

        QString prefix;
        // start from bucketName, end before the last ""
        for(int i= 2; i < parts.size() - 1; i++) {
            prefix.append(parts[i]);
            prefix.append("/");
        }

        qDebug() << "prefix" << prefix;
    }

    return a.exec();
}

