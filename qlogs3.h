#ifndef QLOGS3_H
#define QLOGS3_H

#include <aws/core/utils/logging/LogSystemInterface.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <QObject>


using namespace Aws::Utils;
using namespace Aws::Utils::Logging;

class QLogS3: public QObject, public LogSystemInterface{
    Q_OBJECT

signals:
    void logReceived(const QString &);
public:
    using Base = LogSystemInterface;

    QLogS3(LogLevel loglevel, QObject *parent=0);

    LogLevel GetLogLevel(void) const override;

    void Log(LogLevel logLevel, const char* tag, const char* formatStr, ...) override;

    void LogStream(LogLevel logLevel, const char* tag, const Aws::OStringStream &messageStream) override;
private:
    LogLevel m_loglevel;

};

#endif // QLOGS3_H
