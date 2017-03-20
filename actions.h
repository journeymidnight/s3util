#ifndef ACTIONS_H
#define ACTIONS_H

#include <QObject>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <QRunnable>
#include <QDebug>

using namespace Aws;
using namespace Aws::S3;

typedef Aws::S3::Model::Bucket s3bucket;
typedef Aws::S3::Model::Object s3object;
typedef Aws::Client::AWSError<S3Errors> s3error;


class ListBucketsAction : public QObject, public QRunnable
{
    Q_OBJECT

public:

    ListBucketsAction(QObject *parent, std::shared_ptr<S3Client> s3client):QObject(parent), m_client(s3client){
    }
    void run();
signals:
    void ListBucketInfo(const s3bucket &bucket);
    void CommandFinished(bool success, const s3error & err );

private:
    std::shared_ptr<S3Client> m_client;
};


class ListObjectsAction : public QObject, public QRunnable
{
    Q_OBJECT
public:
    ListObjectsAction(QObject *parent, QString bucket, QString marker, std::shared_ptr<S3Client> s3client):
        QObject(parent), m_client(s3client), m_marker(marker), m_bucketName(bucket)
    {
    }
    void run();
signals:
    void ListObjectInfo(const s3object &bucket);
    void CommandFinished(bool success, const s3error & err, bool truncated);
private:
    std::shared_ptr<S3Client> m_client;
    QString m_marker;
    QString m_bucketName;
};

#endif // ACTIONS_H
