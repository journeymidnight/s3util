#ifndef QS3CLIENT_H
#define QS3CLIENT_H

#include <QObject>
#include <QThreadPool>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/utils/ResourceManager.h>
#include <QRunnable>
#include <QDebug>
#include "actions.h"
#include "qlogs3.h"


namespace qlibs3 {

using namespace Aws;
using namespace Aws::S3;


#define ALLOCATION_TAG "QTS3CLIENT"

void S3API_INIT();
void S3API_SHUTDOWN();


//utf8 to utf16 string
QString AwsString2QString(const Aws::String &s);

//utf16 to utf8 string
Aws::String QString2AwsString(const QString &s);

//the caller of QS3Client should make sure some actions should not
//be called before Finished function returns result;


class QS3Client : public QObject
{
    Q_OBJECT
public:
    QS3Client(QObject *parent, QString endpoint, QString scheme, QString accessKey, QString secretKey);

    //should only be called once. do not have any handler for this
    int Connect();
    ListBucketAction *ListBuckets();
    ListObjectAction *ListObjects(const QString & bucketName, const QString &marker, const QString &prefix);
    CreateBucketAction *CreateBucket(const QString & bucketName);
    DeleteBucketAction *DeleteBucket(const QString & bucketName);
    DeleteObjectAction *DeleteObject(const QString &bucketName, const QString &objectName);

    //could have multiple uploads and downloads, it has a handler for this.
    UploadObjectHandler * UploadFile(const QString &fileName, const QString &bucketName,
                    const QString &keyName, const QString &contentType);


    DownloadObjectHandler * DownloadFile(const QString &bucketName,
                    const QString &keyName, const QString &writeToFile);

    ~QS3Client();
    friend class UploadObjectHandler;
    friend class DownloadObjectHandler;

signals:
    void logReceived(const QString &);

private:
    Aws::Client::ClientConfiguration m_clientConfig;
    std::shared_ptr<Aws::S3::S3Client> m_s3Client;


    QString m_endpoint;
    QString m_accessKey;
    QString m_secretKey;
    QString m_schema;
};

}
#endif // QS3CLIENT_H
