#ifndef CLI_H
#define CLI_H
#include <QCoreApplication>
struct Cli
{
    QString cmd;
    QString para1;
    QString para2;
    QString acl;
    QString confPath;
    bool recursive;
};
#endif // CLI_H
