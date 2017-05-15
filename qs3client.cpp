#include "qs3client.h"
#include <QThreadPool>
#include "qlogs3.h"
#include <aws/core/utils/logging/AWSLogging.h>
#include <QDebug>
#include <QtConcurrent>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/Object.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>


//global varibles

std::shared_ptr<QLogS3> m_s3log;
Aws::SDKOptions m_awsOptions;

void S3API_INIT(){
    Aws::InitAPI(m_awsOptions);
    m_s3log = Aws::MakeShared<QLogS3>(ALLOCATION_TAG, Aws::Utils::Logging::LogLevel::Trace);
    Aws::Utils::Logging::InitializeAWSLogging(m_s3log);
}

void S3API_SHUTDOWN(){
    Aws::Utils::Logging::ShutdownAWSLogging();
    Aws::ShutdownAPI(m_awsOptions);
}

//utf8 to utf16 string
QString AwsString2QString(const Aws::String &s) {
    return QString::fromUtf8(s.c_str(), s.size());
}

//utf16 to utf8 string
Aws::String QString2AwsString(const QString &s) {
    auto qarray = s.toUtf8();
    return Aws::String(qarray.constData(), qarray.size());
}

using namespace Aws::S3;
QS3Client::QS3Client(QObject *parent, QString endpoint, QString scheme, QString accessKey, QString secretKey) : QObject(parent),
    m_endpoint(endpoint), m_schema(scheme), m_accessKey(accessKey), m_secretKey(secretKey)
{


    qRegisterMetaType<s3bucket>("s3bucket");
    qRegisterMetaType<s3error>("s3error");
    qRegisterMetaType<s3object>("s3object");
    qRegisterMetaType<uint64_t>("uint64_t");
    qRegisterMetaType<Aws::Transfer::TransferStatus>("Aws::Transfer::TransferStatus");
    qRegisterMetaType<s3prefix>("s3prefix");
    //forward log signal outside
    connect(m_s3log.get(), SIGNAL(logReceived(QString)), this, SIGNAL(logReceived(QString)));
}

QS3Client::~QS3Client(){
}


int QS3Client::Connect() {


    if (m_schema.compare("http", Qt::CaseInsensitive) == 0) {
        m_clientConfig.scheme = Aws::Http::Scheme::HTTP;
    } else if (m_schema.compare("https", Qt::CaseInsensitive) == 0) {
        m_clientConfig.scheme = Aws::Http::Scheme::HTTPS;
    } else {
        return -1;
    }

    m_clientConfig.connectTimeoutMs = 3000;
    m_clientConfig.requestTimeoutMs = 6000;

    m_clientConfig.endpointOverride= QString2AwsString(m_endpoint);

    m_s3Client = Aws::MakeShared<S3Client>(ALLOCATION_TAG,
                                           Aws::Auth::AWSCredentials(QString2AwsString(m_accessKey), QString2AwsString(m_secretKey)),
                                           m_clientConfig);


    //TODO:
    //Setup transfer manager, I would use QThread for this transfer manager
    Aws::Transfer::TransferManagerConfiguration transferConfiguration;
    transferConfiguration.s3Client = m_s3Client;

    //global transfermanager callback functions
    transferConfiguration.downloadProgressCallback = [](const Aws::Transfer::TransferManager *, const Aws::Transfer::TransferHandle &handler) {
    };

    transferConfiguration.uploadProgressCallback = [this](const Aws::Transfer::TransferManager *, const Aws::Transfer::TransferHandle &handler) {
        auto client = m_uploadHandlerMap[&handler];
        uint64_t dataTransfered = handler.GetBytesTransferred();
        uint64_t dataTotal = handler.GetBytesTotalSize();
        emit client->updateProgress(dataTransfered, dataTotal);
    };

    transferConfiguration.transferStatusUpdatedCallback = [this](const Aws::Transfer::TransferManager *, const Aws::Transfer::TransferHandle &handler) {
        qDebug() << "STATUS:" << (int)handler.GetStatus();
        auto client = m_uploadHandlerMap[&handler];
        emit client->updateStatus(handler.GetStatus());
    };

    transferConfiguration.errorCallback = [this](const TransferManager*, const TransferHandle& handler, const Aws::Client::AWSError<Aws::S3::S3Errors>& error) {
        qDebug() << "error, you are fucked " << AwsString2QString(error.GetMessage());
        auto client = m_uploadHandlerMap[&handler];
        emit client->updateStatus(handler.GetStatus());
    };

    m_transferManager = Aws::MakeShared<Aws::Transfer::TransferManager>(ALLOCATION_TAG, transferConfiguration);

}


void QS3Client::ListBuckets(){
    /*
    ListBucketsAction * action = new ListBucketsAction(NULL, m_s3Client);
    //chain the signals;
    connect(action,SIGNAL(ListBucketInfo(s3bucket)),
                          this, SIGNAL(ListBucketInfo(s3bucket)));
    connect(action,SIGNAL(CommandFinished(bool, s3error)),
                            this, SIGNAL(ListBucketFinished(bool, s3error)));
    //Runable should be deleted automatically
    QThreadPool::globalInstance()->start(action);
    */
    QtConcurrent::run([this](){
        auto list_buckets_outcome = this->m_s3Client->ListBuckets();
        if (list_buckets_outcome.IsSuccess()) {
            Aws::Vector<Aws::S3::Model::Bucket> bucket_list =
                    list_buckets_outcome.GetResult().GetBuckets();
            for (auto const &s3_bucket: bucket_list) {
                emit this->ListBucketInfo(s3_bucket);
            }
            emit this->ListBucketFinished(true, list_buckets_outcome.GetError());
        } else {
            emit this->ListBucketFinished(false, list_buckets_outcome.GetError());
        }
    });
}

void QS3Client::ListObjects(const QString &qbucketName, const QString &qmarker, const QString &qprefix) {
    //ListBucket

    /*
    ListObjectsAction * action = new ListObjectsAction(NULL, qbucketName, qmarker, qprefix, m_s3Client);

    connect(action, SIGNAL(ListObjectInfo(s3object, QString)),
            this, SIGNAL(ListObjectInfo(s3object, QString)));

    connect(action, SIGNAL(ListPrefixInfo(s3prefix, QString)),
            this, SIGNAL(ListPrefixInfo(s3prefix, QString)));

    connect(action, SIGNAL(CommandFinished(bool, s3error, bool)),
            this, SIGNAL(ListObjectFinished(bool,s3error, bool)));

    QThreadPool::globalInstance()->start(action);
    */
    Aws::String bucketName = QString2AwsString(qbucketName);
    Aws::String marker = QString2AwsString(qmarker);
    Aws::String prefix = QString2AwsString(qprefix);

    QtConcurrent::run([=](){

        Aws::S3::Model::ListObjectsRequest objects_request;
        objects_request.SetBucket(bucketName);
        objects_request.WithDelimiter("/").WithMarker(marker).WithPrefix(prefix);
        auto list_objects_outcome = this->m_s3Client->ListObjects(objects_request);
        if (list_objects_outcome.IsSuccess()) {
            const Aws::Vector<Aws::S3::Model::CommonPrefix> &common_prefixs = list_objects_outcome.GetResult().GetCommonPrefixes();

            for (auto const &prefix : common_prefixs) {
                emit this->ListPrefixInfo(prefix, qbucketName);
            }

            Aws::Vector<Aws::S3::Model::Object> object_list =
                    list_objects_outcome.GetResult().GetContents();

            for (auto const &s3_object: object_list) {
                emit this->ListObjectInfo(s3_object, qbucketName);
            }
            emit this->ListObjectFinished(true, list_objects_outcome.GetError(), list_objects_outcome.GetResult().GetIsTruncated());
        } else {
            emit this->ListObjectFinished(false, list_objects_outcome.GetError(), list_objects_outcome.GetResult().GetIsTruncated());
        }
    });
}

UploadObjectHandler * QS3Client::UploadFile(const QString &qfileName, const QString &qbucketName, const QString &qkeyName, const QString &qcontentType) {

    Aws::String fileName, bucketName, keyName, contentType;
    fileName = QString2AwsString(qfileName);
    bucketName = QString2AwsString(qbucketName);
    keyName = QString2AwsString(qkeyName);
    if (qcontentType.isEmpty() || qcontentType.count() == 0) {
        contentType = "application/octet-stream";
    } else {
        contentType = QString2AwsString(qcontentType);
    }

    std::shared_ptr<Aws::Transfer::TransferHandle> requestPtr = m_transferManager->UploadFile(fileName, bucketName, keyName, contentType, Aws::Map<Aws::String, Aws::String>());

    auto clientHandler= new UploadObjectHandler(this, requestPtr);
    m_uploadHandlerMap.insert(requestPtr.get(), clientHandler);
    return clientHandler;
}


DownloadObjectHandler * QS3Client::DownloadFile(const QString &bucketName, const QString &keyName, const QString &writeToFile) {
    auto clientHandler = new DownloadObjectHandler(this, m_s3Client, bucketName, keyName, writeToFile);
    return clientHandler;
}
