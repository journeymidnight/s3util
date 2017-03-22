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

using namespace Aws;
using namespace Aws::S3;
using namespace Aws::Transfer;


//the caller of QS3Client should make sure some actions should not
//be called before Finished function returns result;

class QS3Client : public QObject
{
    Q_OBJECT
public:
    explicit QS3Client(QObject *parent = 0);

    //should only be called once. do not have any handler for this
    void Connect();
    void ListBuckets();
    void ListObjects();

    //could have multiple uploads and downloads, it has a handler for this.
    UploadObjectHandler * UploadFile(const QString &fileName, const QString &bucketName,
                    const QString &keyName, const QString &contentType);
    ~QS3Client();

signals:
    //list bucket callback
    void ListBucketInfo(const s3bucket &bucket);
    void ListBucketFinished(bool, const s3error &error);

    //make bucket

    //delete bucket

    //list object callback
    void ListObjectInfo(const s3object &object);
    void ListObjectFinished(bool, const s3error &error, bool);


private:
    Aws::SDKOptions m_awsOptions;
    Aws::Client::ClientConfiguration m_clientConfig;
    std::shared_ptr<Aws::S3::S3Client> m_s3Client;
    std::shared_ptr<Aws::Transfer::TransferManager> m_transferManager;
    QMap<const Aws::Transfer::TransferHandle *, UploadObjectHandler*> m_uploadHandlerMap;
};

#endif // QS3CLIENT_H
