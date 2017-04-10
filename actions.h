#ifndef ACTIONS_H
#define ACTIONS_H

#include <QObject>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <QRunnable>
#include <aws/transfer/TransferManager.h>
#include <QDebug>

using namespace Aws;
using namespace Aws::S3;
using namespace Aws::Transfer;

typedef Aws::S3::Model::Bucket s3bucket;
typedef Aws::S3::Model::Object s3object;
typedef Aws::Client::AWSError<S3Errors> s3error;
typedef Aws::S3::Model::CommonPrefix s3prefix;

QString AwsString2QString(const Aws::String &s);
Aws::String QString2AwsString(const QString &s);

//All Actions class will be automatically deleted
class ListBucketsAction : public QObject, public QRunnable
{
    Q_OBJECT

public:

    ListBucketsAction(QObject *parent, std::shared_ptr<S3Client> s3client):QObject(parent), m_client(s3client){
    }
    void run();
signals:
    void ListBucketInfo(const s3bucket &bucket);
    void CommandFinished(bool success, const s3error & err);

private:
    std::shared_ptr<S3Client> m_client;
};


class ListObjectsAction : public QObject, public QRunnable
{
    Q_OBJECT
public:
    ListObjectsAction(QObject *parent, const QString &bucket, const QString &marker, const QString &prefix, std::shared_ptr<S3Client> s3client):
        QObject(parent), m_client(s3client)
    {
        m_marker = QString2AwsString(marker);
        m_bucketName = QString2AwsString(bucket);
        m_prefix = QString2AwsString(prefix);
    }
    void run();
signals:
    void ListObjectInfo(const s3object &bucket, QString bucketName);
    void ListPrefixInfo(const s3prefix &prefix, QString bucketName);
    void CommandFinished(bool success, const s3error & err, bool truncated);
private:
    std::shared_ptr<S3Client> m_client;
    Aws::String m_marker;
    Aws::String m_bucketName;
    Aws::String m_prefix;
};


class UploadObjectHandler: public QObject
{
    Q_OBJECT
public:
    UploadObjectHandler(QObject *parent, std::shared_ptr<Aws::Transfer::TransferHandle> ptr):QObject(parent), m_handler(ptr) {
    }
    ~UploadObjectHandler() {
        qDebug() << "UploadObjectHandler: " << m_handler->GetKey().c_str() << "destoried";
    }
    void stop();

signals:
    /* !IMPORTANT */
    void updateProgress(uint64_t, uint64_t);
    void updateStatus(Aws::Transfer::TransferStatus);
    void errorStatus(const s3error &error);

private:
    std::shared_ptr<Aws::Transfer::TransferHandle> m_handler;
};


/* A perfect solution should be :
 * DownloadObjecterHandler use a Qthread  and move itself to
 * the qthread, so the DownloadObjectHandler _live in_ int the qthread,
 * So, the QObject::deleteLater() could delete the QObject safely and make
 * sure the qthread has exsit;
 *
 * *raw implemation*
 * But in this implemations, after sending "Transferstatus::complete", the thread
 * do not use any member, so it is still safe to delete the DownloadObjectHandler after receiving
 * the TransferStatus::Complete
 *
*/
class DownloadObjectHandler: public QObject
{
    Q_OBJECT
public:
    DownloadObjectHandler(QObject *parent, std::shared_ptr<S3Client> client, const QString & bucketName, const QString & keyName, const QString &writeToFile);
    int start();
    void stop();
    ~DownloadObjectHandler() {
        qDebug() << "DownloadObjectHandler: " << m_keyName.c_str() << "Destoried";
    }

signals:
    void updateProgress(uint64_t, uint64_t);
    void updateStatus(Aws::Transfer::TransferStatus);
    void errorStatus(const s3error &error);

private:
    void doDownload();
    std::shared_ptr<S3Client> m_client;
    Aws::String m_bucketName;
    Aws::String m_keyName;
    Aws::String m_writeToFile;
    std::atomic<long> m_status;
    std::atomic<bool> m_cancel;
    uint64_t m_totalSize;
    uint64_t m_totalTransfered;
};

#endif // ACTIONS_H
