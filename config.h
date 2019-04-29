#ifndef CONFIG_H
#define CONFIG_H

#include <QCoreApplication>
#include <iostream>
#include <string>
#include <QSettings>

using namespace std;
const QString DEFAULT_CONFIG = "./.S3Config.in";

enum ConfigParseResult {
    ConfigOK,
    ConfigNotExist,
    ConfigParaseError,
};

class QS3Config : public QObject
{
    Q_OBJECT
public:
    QString m_endpoint;
    QString m_accessKey;
    QString m_secretKey;
    QString m_schema;
    ConfigParseResult parseConfigFile(QString path);
    void genConfigFile();
};

//struct S3Config
//{
//    QString endpoint;
//    QString accessKey;
//    QString secretKey;
//    QString schema;
//};





#endif // CONFIG_H
