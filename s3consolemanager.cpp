#include "s3consolemanager.h"
#include <QDebug>
#include <QThread>
#include <QTimer>
#include "actions.h"


S3ConsoleManager::S3ConsoleManager(QObject *parent) : QObject(parent)
{
    S3API_INIT();
    s3 = new QS3Client(this,"los-cn-north-1.lecloudapis.com", "http",
                                 "Ltiakby8pAAbHMjpUr3L", "qMTe5ibLW49iFDEHNKqspdnJ8pwaawA9GYrBXUYc");

    connect(s3, SIGNAL(logReceived(QString)), this, SLOT(showLog(QString)));
}


S3ConsoleManager::~S3ConsoleManager() {
    delete s3;
    qDebug() << "quit";
    S3API_SHUTDOWN();
}


void S3ConsoleManager::DeleteOneFile() {
    auto action = s3->DeleteObject("document", "database.pdf");
    connect(action, &DeleteObjectAction::DeleteObjectFinished, this, [=](bool s, s3error err){
        qDebug() << s;
        action->deleteLater();
    });
}

void S3ConsoleManager::Execute() {


    s3->Connect();



    /*

    connect(s3, SIGNAL(ListBucketInfo(s3bucket)),
            this, SLOT(ListBucketInfo(s3bucket)));
    connect(s3, SIGNAL(ListBucketFinished(bool, s3error)),
            this, SLOT(Result(bool, s3error)));


    connect(s3, SIGNAL(ListObjectInfo(s3object )),
            this, SLOT(ListObjectInfo(s3object )));

    connect(s3, SIGNAL(ListPrefixInfo(s3prefix)), this, SLOT(ListPrefixInfo(s3prefix)));

    connect(s3, SIGNAL(ListObjectFinished(bool,s3error , bool)),
            this, SLOT(ListObjectResult(bool,s3error ,bool)));
    */

    //tBucketsAction *x = s3->ListBuckets();
    /*
    s3->ListObjects("why","","");
    */



 //     UploadObjectHandler *handler = s3->UploadFile("/Users/zhangdongmao/Documents/database.pdf","document","database.pdf", "");

//    connect(handler,SIGNAL(updateProgress(uint64_t,uint64_t)), this, SLOT(myProgress(uint64_t, uint64_t)));


//    connect(handler, SIGNAL(updateStatus(TransferStatus)), this, SLOT(downloadOrUploadresult(TransferStatus)));




      /*
      connect(handler,&ObjectHandlerInterface::updateStatus, this, [handler](TransferStatus s) {
         std::cout << "something happend " << static_cast<int>(s) << std::endl;
      });
      */
/*
    connect(handler, &ObjectHandlerInterface::finished, this, [this, handler] (bool success, s3error err){
        std::cout << "FINISHED CALLED" << std::endl;
        if (success) {
           std::cout << "COOL" << std::endl;
        } else {
           std::cout << err.GetMessage() << std::endl;
        }

        delete handler;

        this->DeleteOneFile();

    });
    */
/*
    connect(handler, &ObjectHandlerInterface::updateProgress, this, [this](uint64_t a, uint64_t b){
        std::cout << "progress" << a << " " << b  << std::endl;
    });
*/
    /*
    auto handler = s3->DownloadFile("why","ceph-operation-manual.pdf", "ux.mkv");
    qDebug() << "Main thread" << QThread::currentThread();
    connect(handler,SIGNAL(updateProgress(uint64_t,uint64_t)), this, SLOT(myProgress(uint64_t,uint64_t)));
    connect(handler, SIGNAL(updateStatus(Aws::Transfer::TransferStatus)), this, SLOT(downloadOrUploadresult(Aws::Transfer::TransferStatus)));
    int r = handler->start();
    */

    //handler->start();
    DownloadObjectHandler * pHandler = s3->DownloadFile("788909779878", "CentOS-7-x86_64-Minimal-1611.iso" ,"/Users/zhangdongmao/test.iso");
    this->h = pHandler;
    pHandler->start();
    connect(pHandler, &ObjectHandlerInterface::updateProgress, this, [](uint64_t transfered, uint64_t total){
        qDebug() << transfered << "/"<< total;
    });

    connect(pHandler, &ObjectHandlerInterface::finished, this, [=](bool success, s3error err){
        qDebug() << "UI thread:" << QThread::currentThread() << "result:" << success;
    });

    QTimer::singleShot(2000, this, SLOT(stop()));

    /*
    QTimer::singleShot(2000, [pHandler](){
        pHandler->stop();
    });
    */


}


void S3ConsoleManager::stop() {
    this->h->stop();
}

void S3ConsoleManager::ListBucketInfo(s3bucket bucket) {
    std::cout << bucket.GetName() << std::endl;
}

void S3ConsoleManager::Result(bool success, s3error error) {
    //Example: slot can get its origin sender using QObject::sender();
    //auto action = qobject_cast<ListBucketsAction *>(sender());

    if (success != true)
        std::cout << error.GetMessage() << std::endl;
    else {
        std::cout << "GOOD\n";
    }
}

void S3ConsoleManager::ListObjectInfo(s3object object) {
    //BUGY?
    std::cout << object.GetKey().c_str() << std::endl;
}

void S3ConsoleManager::ListObjectResult(bool success, s3error error, bool truncate) {
    if (success != true)
        std::cout << error.GetMessage() << std::endl;
    else {
        std::cout << "GOOD\n";
    }
}

void S3ConsoleManager::ListPrefixInfo(s3prefix prefix) {
    std::cout << "DIR:" << prefix.GetPrefix() << std::endl;
}

void S3ConsoleManager::myProgress(uint64_t transfered, uint64_t total) {
    qDebug() << "Progress Thread" << QThread::currentThread();
    auto clientHandler = qobject_cast<DownloadObjectHandler*>(sender());
}


void S3ConsoleManager::downloadOrUploadresult(TransferStatus s) {

    qDebug() << "End Thread" << QThread::currentThread();

    auto clientHandler = qobject_cast<DownloadObjectHandler*>(sender());
    if (s == TransferStatus::FAILED) {
        std::cout << "failed" << std::endl;
    } else if (s == TransferStatus::CANCELED) {
        std::cout << "completed" << std::endl;
    } else if (s == TransferStatus::COMPLETED){
    } else {
        std::cout << "something happend" << static_cast<int>(s) << std::endl;
    }

}

void S3ConsoleManager::progressError(s3error error) {
    std::cout << "FUCKED" << error.GetMessage() << std::endl;
}


void S3ConsoleManager::showLog(const QString &log) {
    //std::cout << log.toStdString() << std::endl;
}
