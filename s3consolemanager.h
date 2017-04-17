#ifndef S3CONSOLEMANAGER_H
#define S3CONSOLEMANAGER_H

#include <QObject>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <qs3client.h>

class S3ConsoleManager : public QObject
{
    Q_OBJECT
public:
    explicit S3ConsoleManager(QObject *parent = 0);
    ~S3ConsoleManager();

signals:

public slots:
    void Execute();

    void ListBucketInfo(s3bucket  bucket);
    void Result(bool, s3error error);

    void ListObjectInfo(s3object);
    void ListPrefixInfo(s3prefix);
    void ListObjectResult(bool success, s3error error, bool truncated);


    void myProgress(uint64_t, uint64_t);
    void downloadOrUploadresult(Aws::Transfer::TransferStatus);
    void progressError(s3error error);


    void showLog(const QString &log);
private:
    QS3Client *s3;
};

#endif // S3CONSOLEMANAGER_H
