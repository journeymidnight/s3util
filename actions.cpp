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


UploadObjectHandler::UploadObjectHandler(QObject *parent, std::shared_ptr<S3Client> client, QString bucketName,
                                         QString keyName, QString readFile, QString contentType):
    ObjectHandlerInterface(parent),m_client(client),m_uploadId("")
{
    m_status.store(static_cast<long>(TransferStatus::NOT_STARTED));
    m_bucketName = QString2AwsString(bucketName);
    m_keyName = QString2AwsString(keyName);
    m_readFile = QString2AwsString(readFile);
    m_contenttype = QString2AwsString(contentType);
    m_cancel.store(true);
}

void UploadObjectHandler::stop() {
}


int UploadObjectHandler::start() {
    std::function<void> f = [this]() {
        this->doUpload();
    };
    m_cancel.store(false);

    future = QtConcurrent::run(f);

    connect(&futureWatcher, SIGNAL(finished()), this, SIGNAL(finished()));
    futureWatcher.setFuture(future);

    m_status.store(static_cast<long>(TransferStatus::IN_PROGRESS));
    emit updateStatus(TransferStatus::IN_PROGRESS);
}

void UploadObjectHandler::waitForFinish() {
    future.waitForFinished();
}


void UploadObjectHandler::doUpload() {
    //should run in thread pool;
    auto fileStream = Aws::MakeShared<Aws::FStream>("LOCALSTREAM", fileName.c_str(), std::ios_base::in | std::ios_base::binary);

    fileStream->seekg(0, std::ios_base::end);
    m_totalSize = static_cast<size_t>(fileStream->tellg());
    fileStream->seekg(0, std::ios_base::beg);

    if(fileStream->good())
    {
        emit this->updateStatus(TransferStatus::IN_PROGRESS);
        if (m_totalSize > BUFFERSIZE) //5M
            doMultipartUpload(fileStream);
        else
            doSinglePartUpload(fileStream);
    }
    else
    {
        //handle->SetError(Aws::Client::AWSError<Aws::Client::CoreErrors>(static_cast<Aws::Client::CoreErrors>(Aws::S3::S3Errors::NO_SUCH_UPLOAD), "NoSuchUpload", "The requested file could not be opened.", false));
        //TODO error callback;
        emit this->updateStatus(Aws::Transfer::TransferStatus::FAILED);
    }
}


bool UploadObjectHandler::shouldContinue() {
    return !this->m_cancel.load();
}

void UploadObjectHandler::doMultipartUpload(const std::shared_ptr<IOStream> &fileStream) {
    //should run in thread pool;
    Aws::S3::Model::CreateMultipartUploadRequest createMultipartRequest;
    createMultipartRequest.WithBucket(m_bucketName)
            .WithKey(m_keyName);

    uint64_t partCount;
    auto createMultipartResponse = m_client->CreateMultipartUpload(createMultipartRequest);
    if (createMultipartResponse.IsSuccess()) {
        m_uploadId = createMultipartResponse.GetResult().GetUploadId();
        partCount = ( m_totalSize + BUFFERSIZE - 1 ) / BUFFERSIZE;
        for(int i = 0 ; i < partCount ; i++) {
            PartState *p = new PartState;
            p->partID = i + 1;
            p->rangeBegin = i * BUFFERSIZE;
            //the size of the last part is different
            uint64_t partSize = (i + 1 < partCount ) ? BUFFERSIZE: (m_totalSize - BUFFERSIZE * (partCount - 1));
            p->sizeInBytes = partSize;
            m_queueMap.insert(p->partID, p);
        }
    } else {
        emit updateStatus(TransferStatus::FAILED);
    }


    //REALLY UPLOAD echo queued parts;
    uint64_t sentBytes;
    int partNum = 1;
    char buf[BUFFERSIZE];
    while(sentBytes < m_totalSize && this->shouldContinue() && partNum <= partCount) {
        PartState *p = m_queueMap[partNum];
        uint64_t offset = p->rangeBegin;
        uint64_t lengthToWrite = p->sizeInBytes;
        fileStream->seekg(offset);
        fileStream->read(buf, lengthToWrite);
        Aws::S3::Model::UploadPartRequest uploadPartRequest;
        uploadPartRequest.WithBucket(m_bucketName).WithKey(m_keyName);
        uploadPartRequest.SetContinueRequestHandler([this](const Aws::Http::HttpRequest*){
            return this->shouldContinue();
        });


        uploadPartRequest.SetDataSentEventHandler([this, partNum](const Aws::Http::HttpRequest*, long long amount){

            emit this->updateProgress();
        });


        partNum ++;
    }
}


void UploadObjectHandler::doSinglePartUpload(const std::shared_ptr<Aws::IOStream>& fileStream) {
    //should run in thread pool;
    Aws::S3::Model::PutObjectRequest putObjectRequest;
    putObjectRequest.SetContinueRequestHandler([this](const Aws::Http::HttpRequest*) {
        return this->shouldContinue();
    });

    putObjectRequest.WithBucket(m_bucketName)
            .WithKey(m_keyName);
    putObjectRequest.SetContentType(m_contenttype);

    putObjectRequest.SetBody(fileStream);
    putObjectRequest.SetDataReceivedEventHandler([this](const Aws::Http::HttpRequest*, long long progress){
            m_totalTransfered += static_cast<uint64_t>(progress);
            emit updateProgress(m_totalTransfered, m_totalSize);
    });

    //TODO
    //putObjectRequest.SetRequestRetryHandler();
    auto putObjectOutcome = m_client->PutObject(putObjectRequest);
    if (putObjectOutcome.IsSuccess()) {
        m_status.store((static_cast<long>(TransferStatus::COMPLETED)));
        emit updateStatus(TransferStatus::COMPLETED);
    } else {
        emit errorStatus(putObjectOutcome.GetError());
        if (m_cancel.load() == true) {
            m_status.store(static_cast<long>(TransferStatus::CANCELED));
            emit updateStatus(TransferStatus::CANCELED);
        } else {
            m_status.store(static_cast<long>(TransferStatus::FAILED));
            emit updateStatus(TransferStatus::FAILED);
        }

    }
}


DownloadObjectHandler::DownloadObjectHandler(QObject *parent, std::shared_ptr<S3Client> client, const QString & bucketName,
                                             const QString & keyName, const QString &writeToFile):
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
    qDebug() << "DownloadObjectHandler set to stop";
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
