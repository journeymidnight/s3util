#include "qlogs3.h"

//for conversion from aws::string to QString
#include "qs3client.h"

QLogS3::QLogS3(LogLevel loglevel, QObject *parent):m_loglevel(loglevel), QObject(parent){
}


LogLevel QLogS3::GetLogLevel() const{
        return m_loglevel;
}



//not safe
void QLogS3::Log(LogLevel logLevel, const char* tag, const char* formatStr, ...) {
    va_list args;
    va_start(args, formatStr);

    QString text = QString::vasprintf(formatStr, args);
    va_end(args);
    emit logReceived(text);

    /*
        va_list args;
        va_list tmp_args;


        va_start(args, formatStr);
        va_copy(tmp_args, args);
        int requiredLength = vsnprintf(0, 0, formatStr, tmp_args) + 1;
        va_end(tmp_args);
        //char buf[requiredLength]; //c99
        char * buf = new char[requiredLength];
        vsnprintf(buf, requiredLength, formatStr, args);
        va_end(args);
        std::cout << buf << std::endl;
        delete []buf;
     */
}


void QLogS3::LogStream(LogLevel logLevel, const char* tag, const Aws::OStringStream &message_stream)
{
    emit logReceived(AwsString2QString(message_stream.str()));
}
