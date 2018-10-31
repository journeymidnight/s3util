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
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/DeleteBucketRequest.h>
#include <aws/core/client/DefaultRetryStrategy.h>

//global varibles

namespace qlibs3 {

static std::shared_ptr<QLogS3> s3log;
static Aws::SDKOptions awsOptions;

void S3API_INIT(){
//    awsOptions.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Debug;
    Aws::InitAPI(awsOptions);
    s3log = Aws::MakeShared<QLogS3>(ALLOCATION_TAG, Aws::Utils::Logging::LogLevel::Trace);
    Aws::Utils::Logging::InitializeAWSLogging(s3log);
}

void S3API_SHUTDOWN(){
    Aws::Utils::Logging::ShutdownAWSLogging();
    Aws::ShutdownAPI(awsOptions);
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
    qRegisterMetaType<TransferStatus>("TransferStatus");
    qRegisterMetaType<s3prefix>("s3prefix");
    //forward log signal outside
    connect(s3log.get(), SIGNAL(logReceived(QString)), this, SIGNAL(logReceived(QString)));
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
    /*
     * A bug happens in aws-s3-sdk-1.6.35.
     * If I use SetContinueRequestHandler, and the lambda function return true.
     * But AwsClient will try to retry. But some bug in retry logic happens, this leads to segmentfault.
     * So I disable retry to workaround this
     */

    m_clientConfig.retryStrategy = Aws::MakeShared<Aws::Client::DefaultRetryStrategy>(ALLOCATION_TAG, 0, 1);

    m_s3Client = Aws::MakeShared<S3Client>(ALLOCATION_TAG,
                                           Aws::Auth::AWSCredentials(QString2AwsString(m_accessKey), QString2AwsString(m_secretKey)),
                                           m_clientConfig);


    //TODO:
    //Setup transfer manager, I would use QThread for this transfer manager
    /*
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
        auto client = this->m_uploadHandlerMap[&handler];
        if(client != NULL)
            emit client->updateStatus(handler.GetStatus());
    };

    transferConfiguration.errorCallback = [this](const TransferManager*, const TransferHandle& handler, const Aws::Client::AWSError<Aws::S3::S3Errors>& error) {
        qDebug() << "error, you are fucked " << AwsString2QString(error.GetMessage());
        auto client = m_uploadHandlerMap[&handler];
        emit client->updateStatus(handler.GetStatus());
    };

    m_transferManager = Aws::MakeShared<Aws::Transfer::TransferManager>(ALLOCATION_TAG, transferConfiguration);
    */
   return 0;
}

DeleteObjectAction * QS3Client::DeleteObject(const QString &qbucketName, const QString &qobjectName) {
    DeleteObjectAction * action = new DeleteObjectAction;

    Aws::String bucketName = QString2AwsString(qbucketName);
    Aws::String objectName = QString2AwsString(qobjectName);

    auto future = QtConcurrent::run([=](){
        Aws::S3::Model::DeleteObjectRequest object_request;
        object_request.WithBucket(bucketName).WithKey(objectName);

        auto delete_object_outcome = this->m_s3Client->DeleteObject(object_request);
        if (delete_object_outcome.IsSuccess()) {
            qDebug() << "delete file" << qbucketName << " " << qobjectName;
            emit action->DeleteObjectFinished(true, delete_object_outcome.GetError());
            return;
        } else {
            qDebug() << "FAIL delete file" << qbucketName << " " << qobjectName;
            emit action->DeleteObjectFinished(false, delete_object_outcome.GetError());
        }
    });

    action->setFuture(future);
    return action;

}

ListBucketAction * QS3Client::ListBuckets(){
    ListBucketAction *action = new ListBucketAction();

    auto future = QtConcurrent::run([this, action](){
        auto list_buckets_outcome = this->m_s3Client->ListBuckets();
        if (list_buckets_outcome.IsSuccess()) {
            Aws::Vector<Aws::S3::Model::Bucket> bucket_list =
                    list_buckets_outcome.GetResult().GetBuckets();
            for (auto const &s3_bucket: bucket_list) {
                emit action->ListBucketInfo(s3_bucket);
            }
            emit action->ListBucketFinished(true, list_buckets_outcome.GetError());
        } else {
            emit action->ListBucketFinished(false, list_buckets_outcome.GetError());
        }
    });

    action->setFuture(future);
    return action;
}

CreateBucketAction * QS3Client::CreateBucket(const QString &qbucketName){
   Aws::String bucketName = QString2AwsString(qbucketName);
   CreateBucketAction *action = new CreateBucketAction();

   auto future = QtConcurrent::run([=](){
     Aws::S3::Model::CreateBucketRequest request;
     request.WithBucket(bucketName);
     auto outcome = this->m_s3Client->CreateBucket(request);
     if (outcome.IsSuccess())
     {
         emit action->CreateBucketFinished(true,outcome.GetError());
     } else {
std::cout << "Error while getting object " << outcome.GetError().GetExceptionName() <<
        "fuck " << outcome.GetError().GetMessage() << std::endl;
std::cout << int(outcome.GetError().GetErrorType()) <<"num\n";
         emit action->CreateBucketFinished(false,outcome.GetError());
     }
   });
   action->setFuture(future);
   return action;
}

DeleteBucketAction * QS3Client::DeleteBucket(const QString &qbucketName){
   Aws::String bucketName = QString2AwsString(qbucketName);
   DeleteBucketAction *action = new DeleteBucketAction();

   auto future = QtConcurrent::run([=](){
     Aws::S3::Model::DeleteBucketRequest request;
     request.SetBucket(bucketName);
     auto outcome = this->m_s3Client->DeleteBucket(request);
     if (outcome.IsSuccess())
     {
         emit action->DeleteBucketFinished(true,outcome.GetError());
     } else {
         emit action->DeleteBucketFinished(false,outcome.GetError());
     }
   });
   action->setFuture(future);
   return action;
}

ListObjectAction* QS3Client::ListObjects(const QString &qbucketName, const QString &qmarker, const QString &qprefix) {
    //ListBucket
    Aws::String bucketName = QString2AwsString(qbucketName);
    Aws::String marker = QString2AwsString(qmarker);
    Aws::String prefix = QString2AwsString(qprefix);

    ListObjectAction * action = new ListObjectAction();

    auto future = QtConcurrent::run([=](){

        Aws::S3::Model::ListObjectsRequest objects_request;
        objects_request.SetBucket(bucketName);
        objects_request.WithDelimiter("/").WithMarker(marker).WithPrefix(prefix);
        auto list_objects_outcome = this->m_s3Client->ListObjects(objects_request);
        if (list_objects_outcome.IsSuccess()) {
            const Aws::Vector<Aws::S3::Model::CommonPrefix> &common_prefixs = list_objects_outcome.GetResult().GetCommonPrefixes();

            for (auto const &prefix : common_prefixs) {
                emit action->ListPrefixInfo(prefix, qbucketName);
            }

            Aws::Vector<Aws::S3::Model::Object> object_list =
                    list_objects_outcome.GetResult().GetContents();

            for (auto const &s3_object: object_list) {
                emit action->ListObjectInfo(s3_object, qbucketName);
            }
            emit action->ListObjectFinished(true, list_objects_outcome.GetError(), list_objects_outcome.GetResult().GetIsTruncated());
        } else {
std::cout << "Error while getting object " << list_objects_outcome.GetError().GetExceptionName() <<
         list_objects_outcome.GetError().GetMessage() << std::endl;
         std::cout <<"Error num is"<< int(list_objects_outcome.GetError().GetErrorType()) <<"\n";
            emit action->ListObjectFinished(false, list_objects_outcome.GetError(), list_objects_outcome.GetResult().GetIsTruncated());
        }
    });

    action->setFuture(future);
    return action;
}

UploadObjectHandler * QS3Client::UploadFile(const QString &qfileName, const QString &qbucketName, const QString &qkeyName, const QString &qcontentType) {
    auto clientHandler = new UploadObjectHandler(this, m_s3Client, qbucketName, qkeyName, qfileName, qcontentType);
    return clientHandler;
}


DownloadObjectHandler * QS3Client::DownloadFile(const QString &bucketName, const QString &keyName, const QString &writeToFile) {
    auto clientHandler = new DownloadObjectHandler(this, m_s3Client, bucketName, keyName, writeToFile);
    return clientHandler;
}

}//namespace qlibs3
