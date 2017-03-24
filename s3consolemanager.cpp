#include "s3consolemanager.h"
#include <QDebug>

S3ConsoleManager::S3ConsoleManager(QObject *parent) : QObject(parent)
{
    s3 = new QS3Client(this);
}


S3ConsoleManager::~S3ConsoleManager() {
    delete s3;
}


void S3ConsoleManager::Execute() {
    s3->Connect();

    /*
    connect(s3, SIGNAL(ListBucketInfo(const s3bucket&)),
            this, SLOT(ListBucketInfo(const s3bucket&)));
    connect(s3, SIGNAL(ListBucketFinished(bool, const s3error &)),
            this, SLOT(Result(bool, const s3error &)));


    connect(s3, SIGNAL(ListObjectInfo(const s3object &)),
            this, SLOT(ListObjectInfo(const s3object &)));
    connect(s3, SIGNAL(ListObjectFinished(bool,const s3error &, bool)),
            this, SLOT(ListObjectResult(bool,const s3error &,bool)));

    s3->ListBuckets();
    s3->ListObjects();
    */


    /*
    auto handler = s3->UploadFile("/home/zhangdongmao/ver_00_22-1033839045-avc-799948-aac-63999-2802920-305780444-57a3b26e437e4f381ee3a78a948cf526-1458857629069.mp4","why","中文","");
    connect(handler,SIGNAL(updateProgress(uint64_t,uint64_t)), this, SLOT(myProgress(uint64_t, uint64_t)), Qt::QueuedConnection);
    */
    auto handler = s3->DownloadFile("why","bluestore.mkv", "x.mkv");
    connect(handler,SIGNAL(updateProgress(uint64_t,uint64_t)), this, SLOT(myProgress(uint64_t,uint64_t)), Qt::QueuedConnection);
    connect(handler, SIGNAL(updateStatus(Aws::Transfer::TransferStatus)), this, SLOT(downloadOrUploadresult(Aws::Transfer::TransferStatus)));
    handler->start();
}

void S3ConsoleManager::ListBucketInfo(const s3bucket &bucket) {
    std::cout << bucket.GetName() << std::endl;
}

void S3ConsoleManager::Result(bool success, const s3error &error) {
    //Example: slot can get its origin sender using QObject::sender();
    //auto action = qobject_cast<ListBucketsAction *>(sender());

    if (success != true)
        std::cout << error.GetMessage() << std::endl;
    else {
        std::cout << "GOOD\n";
    }
}

void S3ConsoleManager::ListObjectInfo(const s3object & object) {
    //BUGY
    std::cout << object.GetKey().c_str() << std::endl;
}

void S3ConsoleManager::ListObjectResult(bool success, const s3error &error, bool truncate) {
    if (success != true)
        std::cout << error.GetMessage() << std::endl;
    else {
        std::cout << "GOOD\n";
    }
}

void S3ConsoleManager::myProgress(uint64_t transfered, uint64_t total) {
    std::cout << "FUCKING GOOD" << transfered << std::endl;
}


void S3ConsoleManager::downloadOrUploadresult(Aws::Transfer::TransferStatus s) {
    auto clientHandler = qobject_cast<DownloadObjectHandler*>(sender());
    if (s == Aws::Transfer::TransferStatus::FAILED) {
        std::cout << "fuck error" << std::endl;
        clientHandler->deleteLater();
    } else if (s == Aws::Transfer::TransferStatus::CANCELED) {
        std::cout << "completed" << std::endl;
        clientHandler->deleteLater();
    } else if (s == Aws::Transfer::TransferStatus::COMPLETED){
        clientHandler->deleteLater();
    } else {
        std::cout << "something happend" << static_cast<int>(s) << std::endl;

    }

}

void S3ConsoleManager::progressError(const s3error &error) {
    std::cout << error.GetMessage() << std::endl;
}
