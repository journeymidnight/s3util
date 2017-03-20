#include "qs3client.h"
#include "actions.h"
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/Object.h>

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
    objects_request.SetBucket(m_bucketName.toLatin1().data());
    objects_request.WithDelimiter("/").WithMarker(m_marker.toLatin1().data());
    auto list_objects_outcome = m_client->ListObjects(objects_request);
    if (list_objects_outcome.IsSuccess()) {
        Aws::Vector<Aws::S3::Model::Object> object_list =
                list_objects_outcome.GetResult().GetContents();
        for (auto const &s3_object: object_list) {
             emit ListObjectInfo(s3_object);
        }
        emit CommandFinished(true, list_objects_outcome.GetError(), false);
    } else {
        emit CommandFinished(false, list_objects_outcome.GetError(), false);
    }
}
