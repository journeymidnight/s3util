#ifndef S3CONSOLEMANAGER_H
#define S3CONSOLEMANAGER_H

#include <QObject>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <qs3client.h>
#include <config.h>
#include <cli.h>

using namespace qlibs3;

typedef std::uint64_t hash_t;

constexpr hash_t prime = 0x100000001B3ull;
constexpr hash_t basis = 0xCBF29CE484222325ull;

hash_t hash_(char const* str);
constexpr hash_t hash_compile_time(char const* str, hash_t last_value);

struct ObjectInfo {
    QString fileName;
    QString bucketName;
    QString keyName;
};

class S3ConsoleManager : public QObject
{
    Q_OBJECT
public:
    Cli *m_cli;
    explicit S3ConsoleManager(QObject *parent = 0, QS3Config* config = 0, Cli* cli = 0);
    void ListObjects(const QString &bucketName, const QString &marker, const QString &prefix);
    void PutObject(const QString &srcPath, const QString &bucketName, const QString &objectName);

    void GetObject(const QString &bucketName, const QString &objectName, const QString &dstPath);
    void CreateBucket(const QString &bucketName);
    void DeleteBucket(const QString &bucketName);
    void DeleteObject(const QString &bucketName,const QString &objectName);
    void ListBuckets();

    explicit S3ConsoleManager(QObject *parent = 0);
    ~S3ConsoleManager();


signals:
    void Finished();
    void Continue();

public slots:
    void Execute();
    void PutObjects();
    void DeleteOneFile();
    void ListBucketInfo(s3bucket  bucket);
    void Result(bool, s3error error);

    void ListObjectInfo(s3object);
    void ListPrefixInfo(s3prefix);
    void ListObjectResult(bool success, s3error error, bool truncated);

    void stop();

    void myProgress(uint64_t, uint64_t);
    void downloadOrUploadresult(TransferStatus);
    void progressError(s3error error);


    void showLog(const QString &log);
private:
    QS3Client *s3;
    ObjectHandlerInterface *h;
    QList<ObjectInfo> m_objectList;
};

#endif // S3CONSOLEMANAGER_H
