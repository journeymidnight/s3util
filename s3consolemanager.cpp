#include "s3consolemanager.h"
#include <QDebug>
#include <QThread>
#include <QTimer>

S3ConsoleManager::S3ConsoleManager(QObject *parent) : QObject(parent)
{
    s3 = new QS3Client(this);
    connect(s3, SIGNAL(logReceived(QString)), this, SLOT(showLog(QString)));
}


S3ConsoleManager::~S3ConsoleManager() {
    delete s3;
}


void S3ConsoleManager::Execute() {


    s3->Connect();




    connect(s3, SIGNAL(ListBucketInfo(s3bucket)),
            this, SLOT(ListBucketInfo(s3bucket)));
    connect(s3, SIGNAL(ListBucketFinished(bool, s3error)),
            this, SLOT(Result(bool, s3error)));


    connect(s3, SIGNAL(ListObjectInfo(s3object )),
            this, SLOT(ListObjectInfo(s3object )));

    connect(s3, SIGNAL(ListPrefixInfo(s3prefix)), this, SLOT(ListPrefixInfo(s3prefix)));

    connect(s3, SIGNAL(ListObjectFinished(bool,s3error , bool)),
            this, SLOT(ListObjectResult(bool,s3error ,bool)));

    s3->ListBuckets();
    s3->ListObjects("why","","");


    /*
    auto handler = s3->UploadFile("/home/zhangdongmao/ver_00_22-1033839045-avc-799948-aac-63999-2802920-305780444-57a3b26e437e4f381ee3a78a948cf526-1458857629069.mp4","why","中文","");
    connect(handler,SIGNAL(updateProgress(uint64_t,uint64_t)), this, SLOT(myProgress(uint64_t, uint64_t)));
    */

    /*
    auto handler = s3->DownloadFile("why","bluestore.mkv", "x.mkv");
    connect(handler,SIGNAL(updateProgress(uint64_t,uint64_t)), this, SLOT(myProgress(uint64_t,uint64_t)), Qt::QueuedConnection);
    connect(handler, SIGNAL(updateStatus(Aws::Transfer::TransferStatus)), this, SLOT(downloadOrUploadresult(Aws::Transfer::TransferStatus)));
    int r = handler->start();
    */
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
    std::cout << "FUCKING GOOD" << transfered << std::endl;
    auto clientHandler = qobject_cast<DownloadObjectHandler*>(sender());
}


void S3ConsoleManager::downloadOrUploadresult(Aws::Transfer::TransferStatus s) {
    auto clientHandler = qobject_cast<DownloadObjectHandler*>(sender());
    if (s == Aws::Transfer::TransferStatus::FAILED) {
        std::cout << "failed" << std::endl;
    //    clientHandler->deleteLater();
    } else if (s == Aws::Transfer::TransferStatus::CANCELED) {
        std::cout << "completed" << std::endl;
     //   clientHandler->deleteLater();
    } else if (s == Aws::Transfer::TransferStatus::COMPLETED){
     //   clientHandler->deleteLater();
    } else {
        std::cout << "something happend" << static_cast<int>(s) << std::endl;

    }

}

void S3ConsoleManager::progressError(s3error error) {
    std::cout << error.GetMessage() << std::endl;
}


void S3ConsoleManager::showLog(const QString &log) {
    std::cout << log.toStdString() << std::endl;
}
