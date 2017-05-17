#ifndef ACTIONS_H
#define ACTIONS_H

#include <QObject>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <QRunnable>
#include <aws/transfer/TransferManager.h>
#include <QDebug>
#include <QFuture>
#include <QFutureWatcher>



using namespace Aws;
using namespace Aws::S3;
using namespace Aws::Transfer;

typedef Aws::S3::Model::Bucket s3bucket;
typedef Aws::S3::Model::Object s3object;
typedef Aws::Client::AWSError<S3Errors> s3error;
typedef Aws::S3::Model::CommonPrefix s3prefix;

QString AwsString2QString(const Aws::String &s);
Aws::String QString2AwsString(const QString &s);


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
    void updateStatus(Aws::Transfer::TransferStatus);
    void errorStatus(const s3error &error);
    void finished();
};

class UploadObjectHandler: public ObjectHandlerInterface
{
    Q_OBJECT
public:
    UploadObjectHandler(QObject *parent, std::shared_ptr<Aws::Transfer::TransferHandle> ptr):ObjectHandlerInterface(parent), m_handler(ptr) {
    }
    ~UploadObjectHandler() {
        qDebug() << "UploadObjectHandler: " << m_handler->GetKey().c_str() << "destoried";
    }
    int start() Q_DECL_OVERRIDE;
    void stop()  Q_DECL_OVERRIDE;
    void waitForFinish() Q_DECL_OVERRIDE;

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
    QFutureWatcher<void> futureWatcher;
};

#endif // ACTIONS_H
