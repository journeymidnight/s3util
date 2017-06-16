#ifndef ACTIONS_H
#define ACTIONS_H

#include <QObject>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <QRunnable>
#include <QDebug>
#include <QFuture>
#include <QFutureWatcher>



using namespace Aws;
using namespace Aws::S3;

/* from Aws::S3::TransferStatus */
enum class TransferStatus
{
    //this value is only used for directory synchronization
    EXACT_OBJECT_ALREADY_EXISTS,
    //Operation is still queued and has not begun processing
    NOT_STARTED,
    //Operation is now running
    IN_PROGRESS,
    //Operation was canceled. A Canceled operation can still be retried
    CANCELED,
    //Operation failed, A failed operaton can still be retried.
    FAILED,
    //Operation was successful
    COMPLETED,
    //Operation either failed or was canceled and a user deleted the multi-part upload from S3.
    ABORTED
};

typedef Aws::S3::Model::Bucket s3bucket;
typedef Aws::S3::Model::Object s3object;
typedef Aws::Client::AWSError<S3Errors> s3error;
typedef Aws::S3::Model::CommonPrefix s3prefix;

QString AwsString2QString(const Aws::String &s);
Aws::String QString2AwsString(const QString &s);

#define BUFFERSIZE (5<<20) //5MB

class CommandAction: public QObject {
    Q_OBJECT
public:
    CommandAction(QFuture<void> f, QObject *parent=0):QObject(parent), future(f) {
    }
    ~CommandAction(){
        qDebug() << "command action is destoried";
    }

    explicit CommandAction(QObject *parent=0):QObject(parent){
    }
    void setFuture(const QFuture<void> f);
    void waitForFinished();
    bool isFinished();
signals:
    void finished();
protected:
    QFuture<void> future;
    QFutureWatcher<void> m_watcher;
};


class ListBucketAction: public CommandAction {
    Q_OBJECT
public:
    ListBucketAction(QFuture<void> f, QObject * parent=0):CommandAction(f, parent){
    }
    explicit ListBucketAction(QObject * parent=0):CommandAction(parent){
    }
    ~ListBucketAction(){
        qDebug() << "listBucketAction is destoried";
    }

signals:
    void ListBucketInfo(s3bucket bucket);
    void ListBucketFinished(bool success, s3error error);

};


class ListObjectAction: public CommandAction {
    Q_OBJECT
public:
    ListObjectAction(QFuture<void> f, QObject *parent=0):CommandAction(f, parent) {
    };
    explicit ListObjectAction(QObject *parent=0):CommandAction(parent) {
    };
    ~ListObjectAction(){
        qDebug() << "listObjectAction is destoried";
    }

signals:
    void ListObjectInfo(s3object object, QString bucketName);
    void ListPrefixInfo(s3prefix prefix, QString bucketName);
    void ListObjectFinished(bool success, s3error error, bool truncated);
};


class ObjectHandlerInterface: public QObject{
    Q_OBJECT
public:
    ObjectHandlerInterface(QObject *parent=0):QObject(parent){

    }
    virtual int start() = 0;
    virtual void stop() = 0;
    virtual void waitForFinish() = 0;
    virtual ~ObjectHandlerInterface()=default;
signals:
    void updateProgress(uint64_t, uint64_t);
    void updateStatus(TransferStatus);
    void errorStatus(const s3error &error);
    void finished();
};

//for multipart uploads
struct PartState {
    Aws::String etag;
    int partID;
    uint64_t rangeBegin;
    uint64_t sizeInBytes;
    uint64_t currentProgress;
};

using PartStateMap = QMap<int, PartState*>;

class UploadObjectHandler: public ObjectHandlerInterface
{
    Q_OBJECT
public:
    UploadObjectHandler(QObject *parent, std::shared_ptr<S3Client> client, QString bucketName,
                        QString keyName, QString readFile, QString contentType);
    ~UploadObjectHandler() {
        qDebug() << "UploadObjectHandler: " << m_keyName.c_str() << "destoried";
    }
    int start() Q_DECL_OVERRIDE;
    void stop()  Q_DECL_OVERRIDE;
    void waitForFinish() Q_DECL_OVERRIDE;

private:
    bool shouldContinue();
    void doUpload();
    void doMultipartUpload(const std::shared_ptr<Aws::IOStream>& fileStream);
    void doSinglePartUpload(const std::shared_ptr<Aws::IOStream>& fileStream);
    std::shared_ptr<S3Client> m_client;
    Aws::String m_bucketName;
    Aws::String m_keyName;
    Aws::String m_readFile;
    Aws::String m_contenttype;
    std::atomic<long> m_status;
    std::atomic<bool> m_cancel;
    uint64_t m_totalSize;
    uint64_t m_totalTransfered;
    QFuture<void> future;

    Aws::String m_uploadId;
    PartStateMap m_queueMap;
    PartStateMap m_failedMap;
    PartStateMap m_completedMap;
};


class DownloadObjectHandler: public ObjectHandlerInterface
{
    Q_OBJECT
public:
    DownloadObjectHandler(QObject *parent, std::shared_ptr<S3Client> client, const QString & bucketName, const QString & keyName, const QString &writeToFile);
    int start() Q_DECL_OVERRIDE;
    void stop() Q_DECL_OVERRIDE;
    void waitForFinish() Q_DECL_OVERRIDE;
    ~DownloadObjectHandler() {
        qDebug() << "DownloadObjectHandler: " << m_keyName.c_str() << "Destoried";
    }


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
    QFuture<void> future;
};

#endif // ACTIONS_H
