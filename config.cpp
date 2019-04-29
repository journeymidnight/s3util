#include "config.h"


ConfigParseResult QS3Config::parseConfigFile(QString path)
{
    QSettings settings(path, QSettings::NativeFormat);
    QString endpoint = settings.value("endpoint", "").toString();
    if (endpoint.length() != 0) {
        m_endpoint = endpoint;
    } else {
        return ConfigParaseError;
    }
    QString schema = settings.value("schema", "").toString();
    if (endpoint.length() != 0) {
        m_schema = schema;
    } else {
        return ConfigParaseError;
    }
    QString accessKey = settings.value("accessKey", "").toString();
    if (endpoint.length() != 0) {
        m_accessKey = accessKey;
    } else {
        return ConfigParaseError;
    }

    QString secretKey = settings.value("secretKey", "").toString();
    if (endpoint.length() != 0) {
        m_secretKey = secretKey;
    } else {
        return ConfigParaseError;
    }
    return ConfigOK;
}

void QS3Config::genConfigFile()
{
    string endpoint;
    string schema;
    string accessKey;
    string secretKey;
    cout << "Enter S3 Endpoint:\n";
    getline (cin, endpoint);
    cout << "Enter schema: (http|https)\n";
    getline (cin, schema);
    cout << "Enter S3 accessKey:\n";
    getline (cin, accessKey);
    cout << "Enter S3 secretKey:\n";
    getline (cin, secretKey);


    m_endpoint = QString::fromStdString(endpoint);
    m_schema = QString::fromStdString(schema);
    m_accessKey = QString::fromStdString(accessKey);
    m_secretKey = QString::fromStdString(secretKey);

    QSettings settings(DEFAULT_CONFIG, QSettings::NativeFormat);
    settings.setValue("endpoint", m_endpoint);
    settings.setValue("schema", m_schema);
    settings.setValue("accessKey", m_accessKey);
    settings.setValue("secretKey", m_secretKey);
    return;
}
