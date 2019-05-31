#include "qs3client.h"
#include "actions.h"
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/Object.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/UploadPartRequest.h>
#include <aws/s3/model/UploadPartResult.h>
#include <aws/s3/model/CreateMultipartUploadRequest.h>
#include <fstream>
#include <iostream>
#include <QtConcurrent>
#include <aws/core/utils/memory/stl/AWSStreamFwd.h>
#include <aws/core/utils/stream/PreallocatedStreamBuf.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/s3/model/CompleteMultipartUploadRequest.h>
#include <aws/s3/model/AbortMultipartUploadRequest.h>


namespace qlibs3 {

void CommandAction::waitForFinished()
{
    future.waitForFinished();
}


bool CommandAction::isFinished()
{
    return future.isFinished();
}

void CommandAction::setFuture(const QFuture<void> f)
{
    future = f;
}


UploadObjectHandler::UploadObjectHandler(QObject *parent, std::shared_ptr<S3Client> client,
                                         QString bucketName,
                                         QString keyName, QString readFile, QString contentType):
    ObjectHandlerInterface(parent), m_client(client), m_uploadId("")
{
    m_status.store(static_cast<long>(TransferStatus::NOT_STARTED));
    m_bucketName = QString2AwsString(bucketName);
    m_keyName = QString2AwsString(keyName);
    m_readFile = readFile;


    //missing contenttype will crush this SDK;
    if (contentType.isEmpty())  {
        m_contenttype = "application/octet-stream";
    } else {
        m_contenttype = QString2AwsString(contentType);
    }
    m_totalTransfered = 0;
}

void UploadObjectHandler::stop()
{
    m_cancel.store(true);
}


int UploadObjectHandler::start()
{
    std::function<void()> f = [this]() {
        this->doUpload();
    };
    m_cancel.store(false);

    future = QtConcurrent::run(f);
//    this->doUpload();
    return 1;
}

void UploadObjectHandler::waitForFinish()
{
    future.waitForFinished();
}


void UploadObjectHandler::doUpload()
{

    //should run in thread pool;
    qDebug() << "Uploading" << m_readFile;
    auto fileStream = Aws::MakeShared<Aws::FStream>("LOCALSTREAM", m_readFile.toLocal8Bit().constData(),
                                                    std::ios_base::in | std::ios_base::binary);

    fileStream->seekg(0, std::ios_base::end);
    m_totalSize = static_cast<size_t>(fileStream->tellg());
    fileStream->seekg(0, std::ios_base::beg);


    if (!fileStream->good()) {
        std::cout << "enter no good\n";
        m_status.store(static_cast<long>(TransferStatus::FAILED));
        //not retriable error
        s3error err(Aws::S3::S3Errors::NO_SUCH_UPLOAD, false);
        err.SetMessage(QString2AwsString("Not such File :" + m_readFile));
        emit updateStatus(TransferStatus::FAILED);
        emit finished(false, err);
        qDebug() << "no such file " << m_readFile;
        return;
    }

    emit this->updateStatus(TransferStatus::IN_PROGRESS);

    std::cout << "enter send\n";
    if (m_totalSize > BUFFERSIZE) //5M
        doMultipartUpload(fileStream);
    else
        doSinglePartUpload(fileStream);
    return;
}


bool UploadObjectHandler::shouldContinue()
{
    return !this->m_cancel.load();
}

void UploadObjectHandler::doMultipartUpload(const std::shared_ptr<IOStream> &fileStream)
{


    qDebug() << "DOING MULTIPART";
    //should run in thread pool;
    Aws::S3::Model::CreateMultipartUploadRequest createMultipartRequest;
    createMultipartRequest.WithBucket(m_bucketName)
    .WithKey(m_keyName);

    uint64_t partCount;
    auto createMultipartResponse = m_client->CreateMultipartUpload(createMultipartRequest);
    if (createMultipartResponse.IsSuccess()) {
        qDebug() << "create multipart success";
        m_uploadId = createMultipartResponse.GetResult().GetUploadId();
        partCount = ( m_totalSize + BUFFERSIZE - 1 ) / BUFFERSIZE;
        for (int i = 0 ; i < partCount ; i++) {
            PartState *p = new PartState;
            p->partID = i + 1;
            p->rangeBegin = i * BUFFERSIZE;
            //the size of the last part is different
            uint64_t partSize = (i + 1 < partCount ) ? BUFFERSIZE : (m_totalSize - BUFFERSIZE *
                                                                     (partCount - 1));
            p->sizeInBytes = partSize;
            p->success = false;
            m_queueMap.insert(p->partID, p);
            qDebug() << p->partID << " " << p->rangeBegin << " "  << p->sizeInBytes;
        }
    } else {
        qDebug() << "create Multipart ID failed";
        m_status.store((static_cast<long>(TransferStatus::FAILED)));
        emit updateStatus(TransferStatus::FAILED);
        s3error err(Aws::S3::S3Errors::SERVICE_UNAVAILABLE, false);
        emit finished(false, err);
        return;
    }


    //REALLY UPLOAD echo queued parts;
    uint64_t sentBytes = 0;
    int partNum = 1;


    //auto buf = Aws::MakeShared<Aws::Utils::Array<uint64_t>>("LOCALSTREAM", BUFFERSIZE);

    Aws::Utils::Array<uint8_t> buffer(BUFFERSIZE);

    while (sentBytes < m_totalSize && this->shouldContinue() && partNum <= partCount) {

        //auto buffer = qobject_cast<QS3Client*>(parent())->m_bufferManager.Acquire();
        PartState *p = m_queueMap[partNum];
        uint64_t offset = p->rangeBegin;
        uint64_t lengthToWrite = p->sizeInBytes;
        fileStream->seekg(offset);



        fileStream->read((char *)buffer.GetUnderlyingData(), lengthToWrite);


        //
        auto streamBuf = Aws::New<Aws::Utils::Stream::PreallocatedStreamBuf>("LOCALSTREAM", reinterpret_cast<unsigned char*>(buffer.GetUnderlyingData()), static_cast<size_t>(lengthToWrite));
        auto preallocatedStreamReader = Aws::MakeShared<Aws::IOStream>("LOCALSTREAM", streamBuf);



        Aws::S3::Model::UploadPartRequest uploadPartRequest;
        //
        uploadPartRequest.SetPartNumber(partNum);
        uploadPartRequest.SetBucket(m_bucketName);
        uploadPartRequest.SetKey(m_keyName);
        uploadPartRequest.SetContentLength(lengthToWrite);
        uploadPartRequest.SetUploadId(m_uploadId);
        uploadPartRequest.SetContentType(m_contenttype);
        //

        uploadPartRequest.SetContinueRequestHandler([this](const Aws::Http::HttpRequest *) {
            return this->shouldContinue();
        });


        uploadPartRequest.SetDataSentEventHandler([this, partNum](const Aws::Http::HttpRequest *,
        long long amount) {
            m_totalTransfered += amount;
            emit updateProgress(m_totalTransfered, m_totalSize);
        });


        uploadPartRequest.SetBody(preallocatedStreamReader);



        qDebug() << "uploading " << "PartNum :" << p->partID;
        auto uploadPartOutcome = m_client->UploadPart(uploadPartRequest);

        //anyway, release the buffer;
        //qobject_cast<QS3Client*>(parent())->m_bufferManager.Release(buffer);
        Aws::Delete(streamBuf);


        if (uploadPartOutcome.IsSuccess()) {
            emit updateStatus(TransferStatus::IN_PROGRESS);
            p->etag = uploadPartOutcome.GetResult().GetETag();
            p->success = true;
            qDebug() << "\nPartNum :" << p->partID << "Size: " << p->sizeInBytes << "completed";
        } else {
            emit updateStatus(TransferStatus::FAILED);
            s3error err = uploadPartOutcome.GetError();
            m_status.store((static_cast<long>(TransferStatus::FAILED)));
            emit finished(false, err);
            return;
        }

        sentBytes += lengthToWrite;
        partNum ++;

    }

    Aws::S3::Model::CompletedMultipartUpload completedUpload;
    for (int i = 1; i <= partCount; i++) {
        //if not all the part is uploaded
        PartState *p = m_queueMap[i];

        if (!p->success) {
            s3error err(Aws::S3::S3Errors::NETWORK_CONNECTION, false);
            emit updateStatus(TransferStatus::FAILED);
            emit finished(false, err);
            return;
        }
        Aws::S3::Model::CompletedPart completedPart;
        completedPart.WithPartNumber(p->partID).WithETag(p->etag);
        completedUpload.AddParts(completedPart);
    }

    //clean up m_queueMap
    for (auto &p : m_queueMap) {
        delete p;
    }

    //ready to commit the upload
    Aws::S3::Model::CompleteMultipartUploadRequest completeMultipartUploadRequest;
    completeMultipartUploadRequest.SetContinueRequestHandler([this](const Aws::Http::HttpRequest * p) {
        return this->shouldContinue();
    });

    completeMultipartUploadRequest.WithBucket(m_bucketName)
    .WithKey(m_keyName)
    .WithUploadId(m_uploadId)
    .WithMultipartUpload(completedUpload);

    auto completeUploadOutcome = m_client->CompleteMultipartUpload(completeMultipartUploadRequest);
    if (completeUploadOutcome.IsSuccess()) {

        qDebug() << "commit upload success";
        m_status.store((static_cast<long>(TransferStatus::COMPLETED)));
        emit updateStatus(TransferStatus::COMPLETED);
        emit finished(true, s3error());
        return;
    } else {
        if (m_cancel.load() == true)  {
            m_status.store((static_cast<long>(TransferStatus::CANCELED)));
            emit updateStatus(TransferStatus::CANCELED);
        } else {
            m_status.store((static_cast<long>(TransferStatus::FAILED)));
            emit updateStatus(TransferStatus::FAILED);
        }
        emit finished(false, s3error());
    }
}


void UploadObjectHandler::doSinglePartUpload(const std::shared_ptr<Aws::IOStream> &fileStream)
{
    //should run in thread pool;
    Aws::S3::Model::PutObjectRequest putObjectRequest;

    putObjectRequest.SetContinueRequestHandler([this](const Aws::Http::HttpRequest *) {
        return this->shouldContinue();
    });

    putObjectRequest.WithBucket(m_bucketName).WithKey(m_keyName);
    qDebug() << "content type" << m_contenttype.c_str();
    putObjectRequest.SetContentType(m_contenttype);


    putObjectRequest.SetBody(fileStream);

    putObjectRequest.SetDataSentEventHandler([this](const Aws::Http::HttpRequest *,
    long long progress) {
        m_totalTransfered += static_cast<uint64_t>(progress);
        emit updateProgress(m_totalTransfered, m_totalSize);
    });


    //TODO
    //putObjectRequest.SetRequestRetryHandler();

    qDebug() << "UPLOADING to" <<  "BUCKETNAME" << m_bucketName.c_str() << "OBJECTNAME" <<
             m_keyName.c_str();

    auto putObjectOutcome = m_client->PutObject(putObjectRequest);

    qDebug() << "do upload truely";
    s3error err;

    if (putObjectOutcome.IsSuccess()) {
        m_status.store((static_cast<long>(TransferStatus::COMPLETED)));
        emit updateStatus(TransferStatus::COMPLETED);
        emit finished(true, err);
        return;
    } else {
        if (m_cancel.load() == true) {
            m_status.store(static_cast<long>(TransferStatus::CANCELED));
            emit updateStatus(TransferStatus::CANCELED);
        } else {
            m_status.store(static_cast<long>(TransferStatus::FAILED));
            emit updateStatus(TransferStatus::FAILED);
        }
        emit finished(false, putObjectOutcome.GetError());
        return;
    }
}


DownloadObjectHandler::DownloadObjectHandler(QObject *parent, std::shared_ptr<S3Client> client,
                                             const QString &bucketName,
                                             const QString &keyName, const QString &writeToFile):
    ObjectHandlerInterface(parent), m_client(client)
{
    m_status.store(static_cast<long>(TransferStatus::NOT_STARTED));
    m_bucketName = QString2AwsString(bucketName);
    m_keyName = QString2AwsString(keyName);
    m_writeToFile = writeToFile;
    m_cancel.store(true);

    m_totalTransfered = 0;
}

int DownloadObjectHandler::start()
{
    if (m_status.load() == static_cast<long>(TransferStatus::IN_PROGRESS))
        return -1;
    m_cancel.store(false);
    //TODO put into threadpool
    //set your self DO NOT AUTODELETE by threadpool

    std::function<void()> f = [this]() {
        this->doDownload();
    };
    future = QtConcurrent::run(f);

    m_status.store(static_cast<long>(TransferStatus::IN_PROGRESS));
    emit updateStatus(TransferStatus::IN_PROGRESS);
}


void DownloadObjectHandler::stop()
{
    qDebug() << "DownloadObjectHandler set to stop";
    m_cancel.store(true);
    //MAY trigger something
}

void DownloadObjectHandler::waitForFinish()
{
    future.waitForFinished();
}

void DownloadObjectHandler::doDownload()
{
    Aws::S3::Model::HeadObjectRequest headObjectRequest;
    //qDebug() << m_bucketName;
    headObjectRequest.WithBucket(m_bucketName).WithKey(m_keyName);
    auto headObjectOutcome = m_client->HeadObject(headObjectRequest);

    //no such file in S3
    if (!headObjectOutcome.IsSuccess())  {
        std::cout << headObjectOutcome.GetError().GetExceptionName() << " " <<
                  headObjectOutcome.GetError().GetMessage() << std::endl;
        std::cout << headObjectOutcome.GetError().GetMessage();
        std::cout << headObjectOutcome.GetResult().GetContentLength();
        std::cout << int(headObjectOutcome.GetError().GetErrorType());
        qDebug() << "enter do download11";
        m_status.store(static_cast<long>(TransferStatus::FAILED));
        emit updateStatus(TransferStatus::FAILED);
        emit finished(false, headObjectOutcome.GetError());
        return;
    } else {
        if (m_cancel.load() == true) {
            m_status.store(static_cast<long>(TransferStatus::CANCELED));
            emit updateStatus(TransferStatus::CANCELED);
        } else {
            m_totalSize = headObjectOutcome.GetResult().GetContentLength();
            m_totalTransfered = 0;
        }

    }


    Aws::S3::Model::GetObjectRequest request;
    request.WithBucket(m_bucketName).WithKey(m_keyName);
    request.SetContinueRequestHandler([this](const Aws::Http::HttpRequest *) {
        return !this->m_cancel.load();
    });


    //read localfile,
    //always try to append this

    // this fstream was used to implement continuing downloading via "bytes=pos-"
    // will be freed soon
    Aws::FStream *fstream = Aws::New<Aws::FStream>("LOCALSTREAM",
                                                   m_writeToFile.toLocal8Bit().constData(),
                                                   std::ios_base::out | std::ios_base::app | std::ios_base::binary | std::ios_base::ate);

    //errno??
    if (!fstream->good()) {
        m_status.store(static_cast<long>(TransferStatus::FAILED));
        s3error err(Aws::S3::S3Errors::NO_SUCH_UPLOAD, false);
        err.SetMessage(QString2AwsString("Not such File :" + m_writeToFile));
        emit updateStatus(TransferStatus::FAILED);
        emit finished(false, err);
        qDebug() << "Not such file :" << m_writeToFile;
        return;
    }

    auto pos = fstream->tellp();
    // fstream is useless. first close(), then free()
    fstream->close();
    Aws::Free(fstream);

    Aws::StringStream ss;
    ss << "bytes=" << pos << "-";
    if (m_totalSize != 0)
        request.SetRange(ss.str());

    if (m_totalSize != 0 && m_totalSize == pos) {
        m_status.store(static_cast<long>(TransferStatus::EXACT_OBJECT_ALREADY_EXISTS));
        emit updateStatus(TransferStatus::EXACT_OBJECT_ALREADY_EXISTS);
        emit finished(false, s3error());
        return;
    }

    m_totalTransfered += pos;
    request.SetDataReceivedEventHandler([this](const Aws::Http::HttpRequest *,
    Aws::Http::HttpResponse *, long long progress) {
        m_totalTransfered += static_cast<uint64_t>(progress);
        emit updateProgress(m_totalTransfered, m_totalSize);
    });

    /* Never pass outer fstream into SetResponseStreamFactory(), because aws-lib will free fstream,
     * and continue to invoke GetResponseStreamFactory(), thus crash...
     */
    //request is responsible to close and delete the fstream;
    auto responseFactory = [ = ]() {
        //return fstream;
        Aws::FStream *fstream = Aws::New< Aws::FStream >("LOCALSTREAM",
                                                         m_writeToFile.toLocal8Bit().constData(),
                                                         std::ios_base::out | std::ios_base::app | std::ios_base::binary | std::ios_base::ate);

        return fstream;
    };
    request.SetResponseStreamFactory(responseFactory);


    emit updateStatus(TransferStatus::IN_PROGRESS);

    auto getObjectOutcome = m_client->GetObject(request);

    if (getObjectOutcome.IsSuccess()) {
        m_status.store(static_cast<long>(TransferStatus::COMPLETED));
        emit updateStatus(TransferStatus::COMPLETED);
        emit finished(true, s3error());
        return;
    } else {
        //could be canceled or failed
        if (m_cancel.load() == true) {
            m_status.store(static_cast<long>(TransferStatus::CANCELED));
            emit updateStatus(TransferStatus::CANCELED);
        } else {
            m_status.store(static_cast<long>(TransferStatus::FAILED));
            emit updateStatus(TransferStatus::FAILED);
        }

        emit finished(true, getObjectOutcome.GetError());
        return;
    }
}

}//namespace qlibs3
