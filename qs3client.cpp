#include "qs3client.h"
#include <QThreadPool>


using namespace Aws::S3;
QS3Client::QS3Client(QObject *parent) : QObject(parent)
{
     Aws::InitAPI(m_awsOptions);

     //QT style worker threadpool to handle normal operations
     //Aws::S3::TransferManager to handle transfer
    qRegisterMetaType<s3bucket>("s3bucket");
    qRegisterMetaType<s3error>("s3error");
    qRegisterMetaType<s3object>("s3object");
}

QS3Client::~QS3Client(){
    Aws::ShutdownAPI(m_awsOptions);
}

void QS3Client::Connect() {
    m_clientConfig.scheme = Aws::Http::Scheme::HTTP;
    m_clientConfig.connectTimeoutMs = 3000;
    m_clientConfig.requestTimeoutMs = 6000;
    m_clientConfig.endpointOverride= "yig-test.lecloudapis.com:3000";
    m_s3Client = Aws::MakeShared<S3Client>("MYTESTS3Client", Aws::Auth::AWSCredentials("hehehehe", "hehehehe"), m_clientConfig);


    //Setup transfer manager
    Aws::Transfer::TransferManagerConfiguration transferConfiguration;
    transferConfiguration.s3Client = m_s3Client;

    m_transferManager = Aws::MakeShared<Aws::Transfer::TransferManager>("MYTESTS3Client", transferConfiguration);


}

void QS3Client::ListBuckets(){
    ListBucketsAction * action = new ListBucketsAction(this, m_s3Client);
    //chain the signals;
    connect(action,SIGNAL(ListBucketInfo(const s3bucket&)),
                          this, SIGNAL(ListBucketInfo(const s3bucket&)));
    connect(action,SIGNAL(CommandFinished(bool, const s3error&)),
                            this, SIGNAL(ListBucketFinished(bool, const s3error&)));
    //Runable should be deleted automatically
    QThreadPool::globalInstance()->start(action);
}

void QS3Client::ListObjects() {
    //ListBucket
    ListObjectsAction * action = new ListObjectsAction(this, QString("smallfiles"), QString(""), m_s3Client);

    connect(action, SIGNAL(ListObjectInfo(const s3object&)),
            this, SIGNAL(ListObjectInfo(const s3object &)));

    connect(action, SIGNAL(CommandFinished(bool, const s3error&, bool)),
            this, SIGNAL(ListObjectFinished(bool,const s3error&, bool)));

    QThreadPool::globalInstance()->start(action);
}


void QS3Client::UploadFile(const QString &fileName, const QString &bucketName, const QString &keyName, const QString &contentType) {

}
