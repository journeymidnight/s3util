#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTimer>
#include "s3consolemanager.h"
#include "config.h"
#include <QStringList>
#include <QDebug>
#include <QSettings>
#include <iostream>
#include <string>

using namespace std;
struct S3Query
{
    QString cmd;
    QString firstTarget;
    QString secondTarget;
    QString acl;
    QString confPath;
};

enum CommandLineParseResult
{
    CommandLineOk,
    CommandLineError,
    CommandLineVersionRequested,
    CommandLineHelpRequested,

};

CommandLineParseResult parseCommandLine(QCommandLineParser &parser, S3Query *query, QString *errorMessage)
{
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.addPositionalArgument("cmd", QCoreApplication::translate("main", "Command to operate."));
    parser.addPositionalArgument("first", QCoreApplication::translate("main", "Local target to operate."));
    parser.addPositionalArgument("second", QCoreApplication::translate("main", "Remote target to operate."));
    QCommandLineOption configFileOption("f", QCoreApplication::translate("main", "Specify config file path."));
    parser.addOption(configFileOption);
    const QCommandLineOption helpOption = parser.addHelpOption();
    const QCommandLineOption versionOption = parser.addVersionOption();

    if (!parser.parse(QCoreApplication::arguments())) {
        *errorMessage = parser.errorText();
        return CommandLineError;
    }

    if (parser.isSet(versionOption))
        return CommandLineVersionRequested;

    if (parser.isSet(helpOption))
        return CommandLineHelpRequested;

    if (parser.isSet(configFileOption)) {
        const QString path = parser.value(configFileOption);
        query->confPath = path;
    }

    const QStringList positionalArguments = parser.positionalArguments();
    if (positionalArguments.isEmpty()) {
        *errorMessage = "Argument 'cmd' missing.";
        return CommandLineError;
    }

    switch (positionalArguments.size()) {
    case 1:
        query->cmd = positionalArguments.at(0);
        break;
    case 2:
        query->cmd = positionalArguments.at(0);
        query->firstTarget = positionalArguments.at(1);
        break;
    case 3:
        query->cmd = positionalArguments.at(0);
        query->firstTarget = positionalArguments.at(1);
        query->secondTarget = positionalArguments.at(2);
        break;
    default:
        *errorMessage = "At most 3 arguments can be specified.";
        return CommandLineError;
    }

    return CommandLineOk;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("S3Client");
    QCoreApplication::setApplicationVersion("1.0");
    QCommandLineParser parser;
    parser.setApplicationDescription("Test helper");
    parser.addHelpOption();
    parser.addVersionOption();

    S3Query query;
    QString errorMessage;
    QS3Config config;
    QString path;
    ConfigParseResult res;

    switch (parseCommandLine(parser, &query, &errorMessage)) {
    case CommandLineOk:
        path = DEFAULT_CONFIG;
        if (query.confPath.length() != 0) {
            path = query.confPath;
        }
        res = config.parseConfigFile(path);
        if (res != ConfigOK) {
            std::cout << "Load config file error, try create one in current dir \n";
            config.genConfigFile();
        }
        break;
    case CommandLineError:
        std::cerr << qPrintable(errorMessage);
        std::cerr << qPrintable(parser.helpText());
        return 1;
    case CommandLineVersionRequested:
        printf("%s %s\n", qPrintable(QCoreApplication::applicationName()),
               qPrintable(QCoreApplication::applicationVersion()));
        return 0;
    case CommandLineHelpRequested:
        parser.showHelp();
        Q_UNREACHABLE();
        return 0;
    }

//    cout << qPrintable(config.endpoint) << "\n";
//    cout << qPrintable(config.schema) << "\n";
//    cout << qPrintable(config.accessKey) << "\n";
//    cout << qPrintable(config.secretKey) << "\n";
    S3ConsoleManager m(0, &config);
    QTimer::singleShot(0, &m, SLOT(Execute()));

    return app.exec();
}

