#ifndef QS3CLIENT_H
#define QS3CLIENT_H

#include <QObject>
#include <QThreadPool>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <QRunnable>
#include <QDebug>
#include <aws/transfer/TransferManager.h>
#include "actions.h"
#include "qlogs3.h"


using namespace Aws;
using namespace Aws::S3;
using namespace Aws::Transfer;

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
    Q_DISABLE_COPY(QS3Client)
public:
    QS3Client(QObject *parent, QString endpoint, QString scheme, QString accessKey, QString secretKey);

    //should only be called once. do not have any handler for this
    int Connect();
    ListBucketAction *ListBuckets();
    ListObjectAction *ListObjects(const QString & bucketName, const QString &marker, const QString &prefix);

    //could have multiple uploads and downloads, it has a handler for this.
    UploadObjectHandler * UploadFile(const QString &fileName, const QString &bucketName,
                    const QString &keyName, const QString &contentType);


    DownloadObjectHandler * DownloadFile(const QString &bucketName,
                    const QString &keyName, const QString &writeToFile);

    ~QS3Client();

signals:
    void logReceived(const QString &);


private:
    Aws::Client::ClientConfiguration m_clientConfig;
    std::shared_ptr<Aws::S3::S3Client> m_s3Client;
//    std::shared_ptr<Aws::Transfer::TransferManager> m_transferManager;
//    QMutex m_mapLock;
//   QMap<const Aws::Transfer::TransferHandle *, UploadObjectHandler*> m_uploadHandlerMap;

    QString m_endpoint;
    QString m_accessKey;
    QString m_secretKey;
    QString m_schema;
};

#endif // QS3CLIENT_H
