#include "qs3client.h"
#include "actions.h"
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/Object.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <fstream>
#include <QtConcurrent>

using namespace Aws::Transfer;

void CommandAction::waitForFinished() {
    future.waitForFinished();
}


bool CommandAction::isFinished() {
    future.isFinished();
}

void CommandAction::setFuture(const QFuture<void> f) {
    future = f;
    m_watcher.setFuture(future);
    //:w
    connect(&m_watcher, SIGNAL(finished()), this, SIGNAL(finished()));
}


void UploadObjectHandler::stop() {
    this->m_handler->Cancel();
}


int UploadObjectHandler::start() {
    return 0;
}

void UploadObjectHandler::waitForFinish() {
    return;
}

DownloadObjectHandler::DownloadObjectHandler(QObject *parent, std::shared_ptr<S3Client> client, const QString & bucketName, const QString & keyName, const QString &writeToFile):
        ObjectHandlerInterface(parent), m_client(client){
        m_status.store(static_cast<long>(TransferStatus::NOT_STARTED));
        m_bucketName = QString2AwsString(bucketName);
        m_keyName = QString2AwsString(keyName);
        m_writeToFile = QString2AwsString(writeToFile);
        m_cancel.store(true);
}

int DownloadObjectHandler::start(){
    if (m_status.load() == static_cast<long>(TransferStatus::IN_PROGRESS))
            return -1;
    m_cancel.store(false);
    //TODO put into threadpool
    //set your self DO NOT AUTODELETE by threadpool

    std::function<void()> f = [this]() {
        this->doDownload();
    };
    future = QtConcurrent::run(f);

    connect(&futureWatcher, SIGNAL(finished()), this, SIGNAL(finished()));
    futureWatcher.setFuture(future);

    m_status.store(static_cast<long>(TransferStatus::IN_PROGRESS));
    emit updateStatus(TransferStatus::IN_PROGRESS);
}


void DownloadObjectHandler::stop() {
    m_cancel.store(true);
    //MAY trigger something
}


void DownloadObjectHandler::waitForFinish() {
    future.waitForFinished();
}

void DownloadObjectHandler::doDownload(){
        Aws::S3::Model::HeadObjectRequest headObjectRequest;
        headObjectRequest.WithBucket(m_bucketName).WithKey(m_keyName);
        auto headObjectOutcome = m_client->HeadObject(headObjectRequest);

        //no such file in S3
        if (!headObjectOutcome.IsSuccess())  {
            m_status.store(static_cast<long>(TransferStatus::FAILED));
            emit errorStatus(headObjectOutcome.GetError());
            emit updateStatus(TransferStatus::FAILED);
            return;
        } else {
            m_totalSize = headObjectOutcome.GetResult().GetContentLength();
            m_totalTransfered = 0;
        }


        Aws::S3::Model::GetObjectRequest request;
        request.WithBucket(m_bucketName).WithKey(m_keyName);
        request.SetContinueRequestHandler([this](const Aws::Http::HttpRequest*) {
            return !this->m_cancel.load();
        });
        request.SetDataReceivedEventHandler([this](const Aws::Http::HttpRequest*, Aws::Http::HttpResponse*, long long progress)
        {
            m_totalTransfered += static_cast<uint64_t>(progress);
            emit updateProgress(m_totalTransfered, m_totalSize);
        });

        //read localfile,
        //always try to append this
        Aws::FStream *fstream = Aws::New<Aws::FStream>("LOCALSTREAM", m_writeToFile.c_str(),
                                              std::ios_base::out | std::ios_base::app | std::ios_base::binary | std::ios_base::ate);



        //errno??
        if (!fstream->good()) {
            m_status.store(static_cast<long>(TransferStatus::FAILED));
            s3error err(Aws::S3::S3Errors::NO_SUCH_UPLOAD, false);
            emit errorStatus(err);
            emit updateStatus(TransferStatus::FAILED);
            qDebug() << "Not such file :" << AwsString2QString(m_writeToFile);
            return;
        }
        auto pos = fstream->tellg();
        Aws::StringStream ss;
        ss << "bytes=" << pos << "-";
        request.SetRange(ss.str());

        if (m_totalSize == pos) {
            m_status.store(static_cast<long>(TransferStatus::EXACT_OBJECT_ALREADY_EXISTS));
            emit updateStatus(TransferStatus::EXACT_OBJECT_ALREADY_EXISTS);
            return;
        }
        //request is responsible to close and delete the fstream;
        auto responseFactory = [fstream](){
            return fstream;
        };
        request.SetResponseStreamFactory(responseFactory);


        emit updateStatus(TransferStatus::IN_PROGRESS);
        auto getObjectOutcome = m_client->GetObject(request);

        if (getObjectOutcome.IsSuccess()) {
            m_status.store(static_cast<long>(TransferStatus::COMPLETED));
            emit updateStatus(TransferStatus::COMPLETED);
        } else {
            emit errorStatus(getObjectOutcome.GetError());
            //could be canceled or failed
            if (m_cancel.load() == true) {
                m_status.store(static_cast<long>(TransferStatus::CANCELED));
                emit updateStatus(TransferStatus::CANCELED);
            } else {
                m_status.store(static_cast<long>(TransferStatus::FAILED));
                emit updateStatus(TransferStatus::FAILED);
            }
        }
}
