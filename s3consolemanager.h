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

    void ListBucketInfo(const s3bucket & bucket);
    void Result(bool, const s3error &error);

    void ListObjectInfo(const s3object &);
    void ListObjectResult(bool, const s3error &error, bool);
private:
    QS3Client *s3;
};

#endif // S3CONSOLEMANAGER_H
