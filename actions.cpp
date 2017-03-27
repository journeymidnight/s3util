#include "qs3client.h"
#include "actions.h"
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/Object.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <fstream>
#include <QtConcurrent>

using namespace Aws::Transfer;

void ListBucketsAction::run()
{
     auto list_buckets_outcome = m_client->ListBuckets();
     if (list_buckets_outcome.IsSuccess()) {
         Aws::Vector<Aws::S3::Model::Bucket> bucket_list =
                 list_buckets_outcome.GetResult().GetBuckets();
         for (auto const &s3_bucket: bucket_list) {
             emit ListBucketInfo(s3_bucket);
         }
         emit CommandFinished(true, list_buckets_outcome.GetError());
     } else {
         emit CommandFinished(false, list_buckets_outcome.GetError());
    }
}


void ListObjectsAction::run()
{
    Aws::S3::Model::ListObjectsRequest objects_request;
    objects_request.SetBucket(m_bucketName);
    objects_request.WithDelimiter("/").WithMarker(m_marker).WithPrefix(m_prefix);
    auto list_objects_outcome = m_client->ListObjects(objects_request);
    if (list_objects_outcome.IsSuccess()) {
        Aws::Vector<Aws::S3::Model::Object> object_list =
                list_objects_outcome.GetResult().GetContents();
        for (auto const &s3_object: object_list) {
             emit ListObjectInfo(s3_object);
        }
        emit CommandFinished(true, list_objects_outcome.GetError(), list_objects_outcome.GetResult().GetIsTruncated());
    } else {
        emit CommandFinished(false, list_objects_outcome.GetError(), list_objects_outcome.GetResult().GetIsTruncated());
    }
}


void UploadObjectHandler::stop() {
    this->m_handler->Cancel();
}

DownloadObjectHandler::DownloadObjectHandler(QObject *parent, std::shared_ptr<S3Client> client, const QString & bucketName, const QString & keyName, const QString &writeToFile):
        QObject(parent), m_client(client){
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
    QtConcurrent::run(f);

    m_status.store(static_cast<long>(TransferStatus::IN_PROGRESS));
    emit updateStatus(TransferStatus::IN_PROGRESS);
}


void DownloadObjectHandler::stop() {
    m_cancel.store(true);
    //MAY trigger something
    m_status.store(static_cast<long>(TransferStatus::CANCELED));
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



        if (!fstream->good()) {
            m_status.store(static_cast<long>(TransferStatus::FAILED));
            s3error err(Aws::S3::S3Errors::NO_SUCH_UPLOAD, false);
            emit errorStatus(err);
            emit updateStatus(TransferStatus::FAILED);

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


        auto getObjectOutcome = m_client->GetObject(request);

        if (getObjectOutcome.IsSuccess()) {
            m_status.store(static_cast<long>(TransferStatus::COMPLETED));
            emit updateStatus(TransferStatus::COMPLETED);
        } else {
            m_status.store(static_cast<long>(TransferStatus::FAILED));
            emit errorStatus(getObjectOutcome.GetError());
            //could be canceled or failed
            if (m_cancel.load() == true)
                emit updateStatus(TransferStatus::CANCELED);
             else
                emit updateStatus(TransferStatus::FAILED);
        }
}
