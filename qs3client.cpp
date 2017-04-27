#include "qs3client.h"
#include <QThreadPool>
#include "qlogs3.h"
#include <aws/core/utils/logging/AWSLogging.h>


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
QS3Client::QS3Client(QObject *parent) : QObject(parent)
{
    Aws::InitAPI(m_awsOptions);

    m_s3log = Aws::MakeShared<QLogS3>(ALLOCATION_TAG, Aws::Utils::Logging::LogLevel::Trace);
    Aws::Utils::Logging::InitializeAWSLogging(m_s3log);

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
    Aws::Utils::Logging::ShutdownAWSLogging();
    Aws::ShutdownAPI(m_awsOptions);
}


void QS3Client::Connect() {


    m_clientConfig.scheme = Aws::Http::Scheme::HTTP;
    m_clientConfig.connectTimeoutMs = 3000;
    m_clientConfig.requestTimeoutMs = 6000;

    m_clientConfig.endpointOverride= "los-cn-north-1.lecloudapis.com";

    m_s3Client = Aws::MakeShared<S3Client>(ALLOCATION_TAG, Aws::Auth::AWSCredentials("zcChUVLazK7dSwvuXWnF", "XGrkWrF59CY45hyGUMoKwR3B7Cc5yTVvht7AHUdp"), m_clientConfig);


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
        std::cout << "status:" << (int)handler.GetStatus() << std::endl;
        auto client = m_uploadHandlerMap[&handler];
        emit client->updateStatus(handler.GetStatus());
    };

    transferConfiguration.errorCallback = [this](const TransferManager*, const TransferHandle&, const Aws::Client::AWSError<Aws::S3::S3Errors>& error) {

        std::cout << "error" << error.GetMessage() << std::endl;
    };

    m_transferManager = Aws::MakeShared<Aws::Transfer::TransferManager>(ALLOCATION_TAG, transferConfiguration);

}


void QS3Client::ListBuckets(){
    ListBucketsAction * action = new ListBucketsAction(NULL, m_s3Client);
    //chain the signals;
    connect(action,SIGNAL(ListBucketInfo(s3bucket)),
                          this, SIGNAL(ListBucketInfo(s3bucket)));
    connect(action,SIGNAL(CommandFinished(bool, s3error)),
                            this, SIGNAL(ListBucketFinished(bool, s3error)));
    //Runable should be deleted automatically
    QThreadPool::globalInstance()->start(action);
}

void QS3Client::ListObjects(const QString &qbucketName, const QString &qmarker, const QString &qprefix) {
    //ListBucket

    ListObjectsAction * action = new ListObjectsAction(NULL, qbucketName, qmarker, qprefix, m_s3Client);

    connect(action, SIGNAL(ListObjectInfo(s3object, QString)),
            this, SIGNAL(ListObjectInfo(s3object, QString)));

    connect(action, SIGNAL(ListPrefixInfo(s3prefix, QString)),
            this, SIGNAL(ListPrefixInfo(s3prefix, QString)));

    connect(action, SIGNAL(CommandFinished(bool, s3error, bool)),
            this, SIGNAL(ListObjectFinished(bool,s3error, bool)));

    QThreadPool::globalInstance()->start(action);
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
