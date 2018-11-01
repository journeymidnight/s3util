#include "s3consolemanager.h"
#include <QDebug>
#include <QThread>
#include <QTimer>
#include "actions.h"
#include "config.h"
//#include "cli.h"


hash_t hash_(char const* str)
{
    hash_t ret{basis};

    while(*str){
        ret ^= *str;
        ret *= prime;
        str++;
    }

    return ret;
}

constexpr hash_t hash_compile_time(char const* str, hash_t last_value = basis)
{
    return *str ? hash_compile_time(str+1, (*str ^ last_value) * prime) : last_value;
}

S3ConsoleManager::S3ConsoleManager(QObject *parent, QS3Config *config, Cli *cli) : QObject(parent)
{
    S3API_INIT();
//    s3 = new QS3Client(this,"los-cn-north-1.lecloudapis.com", "http",
//                                 "Ltiakby8pAAbHMjpUr3L", "qMTe5ibLW49iFDEHNKqspdnJ8pwaawA9GYrBXUYc");
    s3 = new QS3Client(this,config->m_endpoint, config->m_schema, config->m_accessKey, config->m_secretKey);
    cout << qPrintable(config->m_endpoint) << "\n";
    cout << qPrintable(config->m_schema) << "\n";
    cout << qPrintable(config->m_accessKey) << "\n";
    cout << qPrintable(config->m_secretKey) << "\n";
    m_cli = cli;
    connect(s3, SIGNAL(logReceived(QString)), this, SLOT(showLog(QString)));
}


S3ConsoleManager::~S3ConsoleManager() {
    delete s3;
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
    QString tmp;
    QStringList tmplist;
    std::string cli = m_cli->cmd.toStdString();
    cout << "cli cmd = "<< cli << "\n";
    hash_t hash = hash_(cli.c_str());
    QString dname;
    switch (hash) {
    case hash_compile_time("ls"):
        if (m_cli->firstTarget != ""){
            tmp = m_cli->firstTarget;
            tmp.remove(0,5);
            ListObjects(tmp,"","");
        } else {
            ListBuckets();
        }
        break;
    case hash_compile_time("put"):
        if (m_cli->firstTarget.contains('/')) {
           tmplist = m_cli->firstTarget.split("/");
           dname = tmplist.at(tmplist.size()-1);
        }else{
           dname = m_cli->firstTarget;
        }
        tmp = m_cli->secondTarget;
        tmp.remove(0,5);
        if (tmp.contains("/")) {
            tmplist = tmp.split("/");
            if (tmplist.at(tmplist.size()-1).isEmpty()) {
                PutObject(m_cli->firstTarget,tmplist.at(0),dname);
            } else {
                PutObject(m_cli->firstTarget,tmplist.at(0),tmplist.at(1));
            }
        } else {
           PutObject(m_cli->firstTarget,tmp, dname);
        }
        break;
    case hash_compile_time("get"):
        tmp = m_cli->firstTarget;
        tmp.remove(0,5);
        tmplist = tmp.split("/");
        if (m_cli->firstTarget != ""){
            GetObject(tmplist.at(0),tmplist.at(1),tmplist.at(1));
        } else if (m_cli->secondTarget != "") {
            GetObject(tmplist.at(0),tmplist.at(1),m_cli->secondTarget);
        } else {
            std::cout << "Bad Parameter" << endl;
            emit Finished();
        };
        break;
    case hash_compile_time("del"):
        tmp = m_cli->firstTarget;
        tmp.remove(0,5);
        tmplist = tmp.split("/");
        DeleteObject(tmplist.at(0),tmplist.at(1));
        break;
    case hash_compile_time("mb"):
        tmp = m_cli->firstTarget;
        tmp.remove(0,5);
        cout << "mb bucket = "<< tmp.toStdString() << "\n";
        CreateBucket(tmp);
        break;
    case hash_compile_time("rb"):
        tmp = m_cli->firstTarget;
        tmp.remove(0,5);
        DeleteBucket(tmp);
        break;
    default:
        std::cout << "Unkonw command" << endl;
        emit Finished();
    }
    return;
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
       std::cout << bucket.GetCreationDate().ToGmtString("%Y-%m-%d %H:%M") << "  s3://";
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
       std::cout << object.GetLastModified().ToGmtString("%Y-%m-%d %H:%M");
       std::cout << "         s3://";
       std::cout << bucketName.toUtf8().constData();;
       std::cout << "/";
       std::cout << object.GetKey() << std::endl;
    });
    action->waitForFinished();
}



void S3ConsoleManager::PutObject(const QString &srcPath, const QString &bucketName, const QString &objectName) {
    qDebug() << "srcPath" << srcPath;
    qDebug() << "bucketName" << bucketName;
    qDebug() << "objectName" << objectName;
    s3->Connect();
    //Upload related
     UploadObjectHandler *handler = s3->UploadFile(srcPath,bucketName,objectName, "");
     this ->h = handler;

    connect(handler, &ObjectHandlerInterface::updateProgress, this, [](uint64_t transfered, uint64_t total){
        qDebug() << transfered << "/"<< total;
        
        double progress = transfered/double(total); 
        //qDebug() << progress;
       // std::cout << "progress is:" << progress <<endl; 
        int barWidth = 70;
        std::cout << "[";
        int pos = barWidth * progress;
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) std::cout << "=";
            else if (i == pos) std::cout << ">";
            else std::cout << " ";
        }
        std::cout << "] " << int(progress * 100.0) << " %\r";
        std::cout.flush();
    });

    connect(handler, &ObjectHandlerInterface::finished, this, [=](bool success, s3error err){
        qDebug() << "\nUI thread:" << QThread::currentThread() << "result:" << success; 
	std::cout <<err.GetMessage();
        emit Finished();
    });
    handler->start();

    QTimer::singleShot(600000, this, SLOT(stop()));

}

void S3ConsoleManager::GetObject(const QString &bucketName, const QString &objectName, const QString &dstPath) {
    s3->Connect();
    //Downlaod related
    DownloadObjectHandler * pHandler = s3->DownloadFile(bucketName, objectName ,dstPath);
    this->h = pHandler;
    pHandler->start();
    connect(pHandler, &ObjectHandlerInterface::updateProgress, this, [](uint64_t transfered, uint64_t total){
       // qDebug() << transfered << "/"<< total;
   
        int barWidth = 70;
        double progress = transfered/double(total); 
    
        std::cout << "[";
        int pos = barWidth * progress;
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) std::cout << "=";
            else if (i == pos) std::cout << ">";
            else std::cout << " ";
        }
        std::cout << "] " << int(progress * 100.0) << " %\r";
        std::cout.flush();
    
    });

    connect(pHandler, &ObjectHandlerInterface::finished, this, [=](bool success, s3error err){
        qDebug() << "\nUI thread:" << QThread::currentThread() << "result:" << success; 
	std::cout <<err.GetMessage();
        emit Finished();
    });

    QTimer::singleShot(600000, this, SLOT(stop()));
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

void S3ConsoleManager::DeleteObject(const QString &bucketName,const QString &objectName) {
    s3->Connect();

    DeleteObjectAction *action = s3->DeleteObject(bucketName ,objectName);
    connect(action, &DeleteObjectAction::DeleteObjectFinished, this, [=](bool success, s3error err){
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
