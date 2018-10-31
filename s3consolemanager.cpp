#include "s3consolemanager.h"
#include <QDebug>
#include <QThread>
#include <QTimer>
#include "actions.h"


S3ConsoleManager::S3ConsoleManager(QObject *parent) : QObject(parent)
{
    S3API_INIT();
    //s3 = new QS3Client(this,"los-cn-north-1.lecloudapis.com", "http",
    //                             "Ltiakby8pAAbHMjpUr3L", "qMTe5ibLW49iFDEHNKqspdnJ8pwaawA9GYrBXUYc");
    s3 = new QS3Client(this,"oss-north-1.unicloudsrv.com", "http",
                                 "AK0HZFOYBQA5Q343374U", "zpA/bF+oUfv6Z/tvQXoyull5j17toEgwsrKvaLR3");

    connect(s3, SIGNAL(logReceived(QString)), this, SLOT(showLog(QString)));
}


S3ConsoleManager::~S3ConsoleManager() {
    std::cout << "enter sssssssssss\n";
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
//    for (int i = 0; i < args.size(); ++i){
//	 std::cout << "enter loop\n"; 
//         std::cout << args.at(i).toLocal8Bit().constData() << endl;
//}
    auto ops = args.at(1);
//    std::cout << "ops is:" << "type is:" << typeid(ops).name()<<endl;
    const QString LS = QString("ls");
    if (ops == LS) {
           std::cout << "ls all buckets\n";
           ListObjects(args.at(2),"",args.at(3));
    }
    if (ops == QString("put")){
           std::cout << "enter put";
           PutObject(args.at(2),args.at(3),args.at(4));
    }
  
    if (ops ==QString("get")){
       GetObject(args.at(2),args.at(3),args.at(4));
    }
 
    if (ops == QString("mb")){
       CreateBucket(args.at(2));
    }

    if (ops == QString("rb")){
       DeleteBucket(args.at(2));
    }

    if (ops == QString("lb")){
       ListBuckets();
    }
//
//    switch(ops){
//        case QString(LS):
//           std::cout << "ls all buckets";
//           break;
//        default:
//           std::cout << "unknown";
//    }

}



void S3ConsoleManager::ListBuckets() {
    s3->Connect();

    //ListBucket related
    ListBucketAction *action = s3->ListBuckets();
    connect(action, &ListBucketAction::ListBucketFinished, this, [=](bool success, s3error err){
        qDebug() << "UI thread:" << QThread::currentThread() << "result:" << success; 
	std::cout <<err.GetMessage();
	emit Finished();
    });

    connect(action,&ListBucketAction::ListBucketInfo,this,[=](s3bucket bucket){
       std::cout << bucket.GetName() << std::endl;
    });
    action->waitForFinished();
}

void S3ConsoleManager::ListObjects(const QString &bucketName, const QString &marker, const QString &prefix) {
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



/*
    //Upload related
     UploadObjectHandler *handler = s3->UploadFile("/tmp/abcde.txt","testlsx","testupload.txt", "");

    connect(handler, &ObjectHandlerInterface::updateProgress, this, [](uint64_t transfered, uint64_t total){
        qDebug() << transfered << "/"<< total;
    });

    connect(handler, &ObjectHandlerInterface::finished, this, [=](bool success, s3error err){
        qDebug() << "UI thread:" << QThread::currentThread() << "result:" << success; 
	std::cout <<err.GetMessage();
    });
    handler->start();

    QTimer::singleShot(2000, this, SLOT(stop()));
*/



/*
    //Downlaod related
    DownloadObjectHandler * pHandler = s3->DownloadFile("testlsx", "testupload.txt" ,"/tmp/testbig");
    this->h = pHandler;
    pHandler->start();
    connect(pHandler, &ObjectHandlerInterface::updateProgress, this, [](uint64_t transfered, uint64_t total){
        qDebug() << transfered << "/"<< total;
    });

    connect(pHandler, &ObjectHandlerInterface::finished, this, [=](bool success, s3error err){
        qDebug() << "UI thread:" << QThread::currentThread() << "result:" << success; 
	std::cout <<err.GetMessage();
    });

    QTimer::singleShot(6000000, this, SLOT(stop()));
    //finish Download related
*/

    /*
    //ListBucket related
    ListBucketAction *action = s3->ListBuckets();
    connect(action, &ListBucketAction::ListBucketFinished, this, [=](bool success, s3error err){
        qDebug() << "UI thread:" << QThread::currentThread() << "result:" << success; 
	std::cout <<err.GetMessage();
    });

    connect(action,&ListBucketAction::ListBucketInfo,this,[=](s3bucket bucket){
       std::cout << bucket.GetName() << std::endl;
    });
    action->waitForFinished();
    */

/*
   //CreateBucket related
   CreateBucketAction *action = s3->CreateBucket("testmake");
    connect(action, &CreateBucketAction::CreateBucketFinished, this, [=](bool success, s3error err){
        qDebug() << "UI thread:" << QThread::currentThread() << "result:" << success; 
	std::cout <<err.GetMessage();
    });
    action->waitForFinished();
    QTimer::singleShot(6000000, this, SLOT(stop()));
*/
/*
   DeleteBucketAction *action = s3->DeleteBucket("testmake");
    connect(action, &DeleteBucketAction::DeleteBucketFinished, this, [=](bool success, s3error err){
        qDebug() << "UI thread:" << QThread::currentThread() << "result:" << success; 
	std::cout <<err.GetMessage();
    });
    action->waitForFinished();
    QTimer::singleShot(6000000, this, SLOT(stop()));
 */ 

    ListObjectAction *action = s3->ListObjects(bucketName,marker,prefix);
    connect(action, &ListObjectAction::ListObjectFinished, this, [=](bool success, s3error err){
        qDebug() << "UI thread:" << QThread::currentThread() << "result:" << success; 
	std::cout <<err.GetMessage();
        emit Finished();
    });
    connect(action,&ListObjectAction::ListObjectInfo,this,[=](s3object object,QString bucketName){
       std::cout << object.GetKey() << std::endl;
    });
    action->waitForFinished();
    if (action->isFinished()){
        std::cout << "really finish\n";
    }else{
       std::cout << "wtf";
    }
//    QTimer::singleShot(6000000, this, SLOT(stop()));
}



void S3ConsoleManager::PutObject(const QString &srcPath, const QString &bucketName, const QString &objectName) {
    s3->Connect();
    //Upload related
     UploadObjectHandler *handler = s3->UploadFile(srcPath,bucketName,objectName, "");

    connect(handler, &ObjectHandlerInterface::updateProgress, this, [](uint64_t transfered, uint64_t total){
        qDebug() << transfered << "/"<< total;
    });

    connect(handler, &ObjectHandlerInterface::finished, this, [=](bool success, s3error err){
        qDebug() << "UI thread:" << QThread::currentThread() << "result:" << success; 
	std::cout <<err.GetMessage();
        emit Finished();
    });
    handler->start();

    QTimer::singleShot(2000, this, SLOT(stop()));

}

void S3ConsoleManager::GetObject(const QString &bucketName, const QString &objectName, const QString &dstPath) {
    s3->Connect();
    //Downlaod related
    DownloadObjectHandler * pHandler = s3->DownloadFile(bucketName, objectName ,dstPath);
    this->h = pHandler;
    pHandler->start();
    connect(pHandler, &ObjectHandlerInterface::updateProgress, this, [](uint64_t transfered, uint64_t total){
        qDebug() << transfered << "/"<< total;
    });

    connect(pHandler, &ObjectHandlerInterface::finished, this, [=](bool success, s3error err){
        qDebug() << "UI thread:" << QThread::currentThread() << "result:" << success; 
	std::cout <<err.GetMessage();
        emit Finished();
    });

    QTimer::singleShot(6000000, this, SLOT(stop()));
}

void S3ConsoleManager::CreateBucket(const QString &bucketName) {
    s3->Connect();


   //CreateBucket related
   CreateBucketAction *action = s3->CreateBucket(bucketName);
    connect(action, &CreateBucketAction::CreateBucketFinished, this, [=](bool success, s3error err){
        qDebug() << "UI thread:" << QThread::currentThread() << "result:" << success; 
	std::cout <<err.GetMessage();
	emit Finished();
    });
    action->waitForFinished();
}

void S3ConsoleManager::DeleteBucket(const QString &bucketName) {
    s3->Connect();

   DeleteBucketAction *action = s3->DeleteBucket(bucketName);
    connect(action, &DeleteBucketAction::DeleteBucketFinished, this, [=](bool success, s3error err){
        qDebug() << "UI thread:" << QThread::currentThread() << "result:" << success; 
	std::cout <<err.GetMessage();
	emit Finished();
    });
    action->waitForFinished();
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
