#include "s3consolemanager.h"
#include <QDebug>
#include <QThread>
#include <QTimer>
#include <QTime>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include "actions.h"
#include "config.h"
#include <iostream>
#include <iomanip>

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

QFileInfoList GetFileList(QString path)
{
    QDir dir(path);
    QFileInfoList file_list = dir.entryInfoList(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    QFileInfoList folder_list = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);

    for(int i = 0; i != folder_list.size(); i++)
    {
         QString name = folder_list.at(i).absoluteFilePath();
         QFileInfoList child_file_list = GetFileList(name);
         file_list.append(child_file_list);
    }

    return file_list;
}

void S3ConsoleManager::Execute() {
    QString tmp;
    QStringList tmplist;
    std::string cli = m_cli->cmd.toStdString();
    hash_t hash = hash_(cli.c_str());
    QString dname, parentDirPath, childPath, srcPath, bucketName, prefix, dstTmp, dstObjName, objectName, dstPrefix;
    QFileInfoList fil;
    QFileInfo fi;
    switch (hash) {
    case hash_compile_time("ls"):
        if (m_cli->para1 != ""){
            tmp = m_cli->para1;
            tmp.remove(0,5);
            connect(this, &S3ConsoleManager::ContinueList, this, [=](QString bucketName, QString marker, QString prefix){this->ListObjects(bucketName,marker,prefix);});
            if (tmp.contains('/')) {
                tmplist = tmp.split("/");
                bucketName = tmplist.at(0);
                tmp.remove(0, bucketName.length()+1);
                if (tmp.length() >= 1) {
                    ListObjects(bucketName,"",tmp);
                } else {
                    ListObjects(bucketName,"","");
                }
            }else{
                bucketName = tmp;
                ListObjects(bucketName,"","");
            }

        } else {
            ListBuckets();
        }
        break;
    case hash_compile_time("put"):
        fi = QFileInfo{m_cli->para1};
        parentDirPath = fi.absolutePath() + '/';
        qDebug() << "parentDirPath="<< qPrintable(parentDirPath) << endl;
        dstTmp = m_cli->para2;
        dstTmp.remove(0,5);
        tmplist = dstTmp.split("/");
        bucketName = tmplist.at(0);
        if (m_cli->para2.endsWith('/')) {
            if (tmplist.at(1).isEmpty()) {
                dstPrefix = "";
            } else {
                dstPrefix = dstTmp.remove(0, bucketName.size()+1);
            }
            qDebug() << "dstPrefix="<< qPrintable(dstPrefix) << endl;
        } else {
            if (tmplist.size() == 1) {
                dstPrefix = "";
            } else {
                cout <<"ERROR: Parameter problem: Destination S3 URI must end with '/'." << endl;
                emit Finished();
                break;
            }
        }
        //check para1 if a file or directory, ie para1= test/
        if (fi.isDir()) {
            if (m_cli->recursive == true) {
                m_objectList.clear();
                connect(this, SIGNAL(Continue()), this, SLOT(PutObjects()));
                fil = GetFileList(m_cli->para1);
                for(int i = 0; i < fil.size(); i++) {
                    srcPath = fil.at(i).absoluteFilePath();
                    tmp = srcPath;
                    tmp.remove(0, parentDirPath.length());
                    dstObjName = dstPrefix + tmp;
                    qDebug() << "tmp="<< qPrintable(tmp) << endl;
                    qDebug() << "dstObjName="<< qPrintable(dstObjName) << endl;
                    ObjectInfo info;
                    info.fileName = srcPath;
                    info.bucketName = bucketName;
                    info.keyName = dstObjName;
                    m_objectList.append(info);
                }
                PutObjects();
            } else {
                cout << "ERROR: Parameter problem: Use --recursive to upload a directory: build \n";
                emit Finished();
            }
        } else {
            if (m_cli->para1.contains('/')) {
               tmplist = m_cli->para1.split("/");
               objectName = tmplist.at(tmplist.size()-1);
            }else{
               objectName = m_cli->para1;
            }
            tmp = m_cli->para2;
            tmp.remove(0,5);
            tmplist = tmp.split("/");
            bucketName = tmplist.at(0);
            if (tmp.contains("/")) {
                if (tmplist.at(1).isEmpty()) { // eg s3://test/
                    cout << "test1" << tmplist.size() << endl;
                    PutObject(m_cli->para1,bucketName, objectName);
                    break;
                }
                if (tmplist.at(tmplist.size()-1).isEmpty()) { // eg s3://test/test1/
                    cout << "test2" << tmplist.size() << endl;
                    tmp.remove(0, bucketName.size() + 1); // tmp = test1/
                    objectName = tmp + objectName; // fileName = test1/file
                    PutObject(m_cli->para1,bucketName,objectName);
                } else { // eg s3://test/test1
                    cout << "test3" << tmplist.size() << endl;
                    tmp.remove(0, bucketName.size() + 1); // tmp = test1
                    objectName = tmp;
                    PutObject(m_cli->para1,bucketName,objectName);
                }
            } else { // s3://test
               cout << "test4" << tmplist.size() << endl;
               PutObject(m_cli->para1,bucketName, objectName);
               cout << "past PutObject \n";
            }
        }
        break;
    case hash_compile_time("get"):
        tmp = m_cli->para1;
        tmp.remove(0,5);
        tmplist = tmp.split("/");
        bucketName = tmplist.at(0);
        if (tmp.endsWith('/')) {
            tmp.remove(0, bucketName.size() + 1);
            prefix = tmp;
            qDebug() << "start get objects \n";
            connect(this, &S3ConsoleManager::ContinuePrepareGet, this, [=](QString bucketName, QString marker, QString prefix, QString dstDir){this->PrePareGetObjects(bucketName,marker,prefix,dstDir);});
            PrePareGetObjects(bucketName,"",prefix,m_cli->para2);
        } else {

            dstObjName =  tmplist.at(tmplist.size()-1);
            tmp.remove(0, bucketName.size() + 1);
            objectName = tmp;
            if (m_cli->para2 != "") {
                GetObject(bucketName,objectName,m_cli->para2);
            } else if (m_cli->para1 != "") {
                GetObject(bucketName, objectName, dstObjName);
            } else {
                std::cout << "Bad Parameter" << endl;
                emit Finished();
            }
        }

        break;
    case hash_compile_time("del"):
        tmp = m_cli->para1;
        tmp.remove(0,5);
        tmplist = tmp.split("/");
        DeleteObject(tmplist.at(0),tmplist.at(1));
        break;
    case hash_compile_time("mb"):
        tmp = m_cli->para1;
        tmp.remove(0,5);
        cout << "mb bucket = "<< tmp.toStdString() << "\n";
        CreateBucket(tmp);
        break;
    case hash_compile_time("rb"):
        tmp = m_cli->para1;
        tmp.remove(0,5);
        DeleteBucket(tmp);
        break;
    default:
        std::cout << "Command not support" << endl;
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

void S3ConsoleManager::PrePareGetObjects(const QString &bucketName, const QString &marker, const QString &prefix, QString &dstDir) {
    s3->Connect();
    ListObjectAction *action = s3->ListObjects(bucketName, marker, prefix, QString(""));
    connect(action, &ListObjectAction::ListObjectFinished, this, [=](bool success, s3error err, bool truncated, QString nextMarker){
        qDebug() << "UI thread:" << QThread::currentThread() << "result:" << success; 
	std::cout <<err.GetMessage();
        if (truncated == false) {
            qDebug() << "enter insert to list\n";
            emit Continue();
        } else {
            emit ContinuePrepareGet(bucketName, nextMarker, prefix, dstDir);
        }
    });
    connect(action,&ListObjectAction::ListObjectInfo,this,[=](s3object object,QString bucketName){
       qDebug() << "enter insert to list\n";
       ObjectInfo info;
       info.bucketName = bucketName;
       info.fileName = dstDir;
       info.keyName = AwsString2QString(object.GetKey());
       m_objectList.append(info);
    });
    connect(this, SIGNAL(Continue()), this, SLOT(GetObjects()));
    action->waitForFinished();
}

void S3ConsoleManager::ListObjects(const QString &bucketName, const QString &marker, const QString &prefix) {
    s3->Connect();
    ListObjectAction *action = s3->ListObjects(bucketName, marker, prefix, QString('/'));
    connect(action, &ListObjectAction::ListObjectFinished, this, [=](bool success, s3error err, bool truncated, QString nextMarker){
        qDebug() << "UI thread:" << QThread::currentThread() << "result:" << success;
    std::cout <<err.GetMessage();
        if (truncated == false) {
            emit Finished();
        } else {
            emit ContinueList(bucketName, nextMarker, prefix);
        }
    });
    connect(action,&ListObjectAction::ListObjectInfo,this,[=](s3object object,QString bucketName){
       std::cout << std::left << std::setw(25) << object.GetLastModified().ToGmtString("%Y-%m-%d %H:%M");
       std::cout << std::setw(15) << object.GetSize();
       std::cout <<"s3://";
       std::cout << bucketName.toUtf8().constData();;
       std::cout << "/";
       std::cout << object.GetKey() << std::endl;
    });
    connect(action,&ListObjectAction::ListPrefixInfo,this,[=](s3prefix prefix,QString bucketName){
        std::cout << std::left << std::setw(25) <<"";
        std::cout << std::setw(15) <<"DIR";
        std::cout <<"s3://";
        std::cout << bucketName.toUtf8().constData();;
        std::cout << "/";
        std::cout << prefix.GetPrefix() << std::endl;
     });
    action->waitForFinished();
}
void sleep(unsigned int msec)
{
    QTime dieTime = QTime::currentTime().addMSecs(msec);
    while( QTime::currentTime() < dieTime )
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
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
    qDebug() << "start handle" << srcPath;
    handler->start();
    qDebug() << "end handle" << srcPath;
//    sleep(5000);
//    QTimer::singleShot(600000, this, SLOT(stop()));

}

void S3ConsoleManager::PutObjects() {
        cout << "start PutObjects \n";
        if (m_objectList.isEmpty()) {
            cout << "list empty \n";
            emit Finished();
            return;
        }
        s3->Connect();
        ObjectInfo item = m_objectList.constFirst();
        cout << qPrintable(item.fileName) << endl;
        cout << qPrintable(item.bucketName) << endl;
        cout << qPrintable(item.keyName) << endl;
        UploadObjectHandler *handler = s3->UploadFile(item.fileName,item.bucketName, item.keyName, "");
        this->h = handler;
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

        connect(handler, &ObjectHandlerInterface::finished, this, [&](bool success, s3error err){
            cout << "enter finished \n";
            qDebug() << "\nUI thread:" << QThread::currentThread() << "result:" << success;
            std::cout <<err.GetMessage();
            m_objectList.pop_front();
            if (m_objectList.isEmpty()) {
                cout << "list empty \n";
                emit Finished();
                return;
            } else {
                cout << "list not empty \n";
                emit Continue();
            }
        });
        handler->start();
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

void S3ConsoleManager::GetObjects() {
    cout << "start GetObjects \n";
    if (m_objectList.isEmpty()) {
        cout << "list empty \n";
        emit Finished();
        return;
    }
    s3->Connect();
    ObjectInfo item = m_objectList.constFirst();
    qDebug() << qPrintable(item.fileName) << endl;
    qDebug() << qPrintable(item.bucketName) << endl;
    qDebug() << qPrintable(item.keyName) << endl;
    QString url = m_cli->para1;//s3://test/ccc/
    url.remove(0, 5);// test/ccc/
    url.remove(0, item.bucketName.length()+1); // ccc/
    QString commonPrefix = url;
    qDebug() << qPrintable(commonPrefix) << endl;
    QString tmpkeyName = item.keyName;
    QString localKey = tmpkeyName.remove(0,commonPrefix.length());
    QDir dstDir(item.fileName);
    QString absolutePath = dstDir.absolutePath() + '/';
    qDebug() << "absolutePath:"<< qPrintable(absolutePath) << endl;
    QString localPath = absolutePath+localKey;
    QStringList strl = localPath.split('/');
    QString tmp = localPath;
    tmp.chop(strl.at(strl.size()-1).length());
    qDebug() << "parent dir:"<< qPrintable(tmp) << endl;
    QDir dir(tmp);
    if (!dir.exists()){
      dir.mkpath(tmp);
    }
    qDebug() << "bucketName:"<< qPrintable(item.bucketName) << endl;
    qDebug() << "keyName:"<< qPrintable(item.keyName) << endl;
    qDebug() << "localpath:"<< qPrintable(localPath) << endl;
    DownloadObjectHandler * pHandler = s3->DownloadFile(item.bucketName, item.keyName, localPath);
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
        if (success != true) {
            std::cout <<err.GetMessage();
            emit Finished();
            return;
        }
        m_objectList.pop_front();
        if (m_objectList.isEmpty()) {
            emit Finished();
            return;
        } else {
            emit Continue();
        }
    });
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
