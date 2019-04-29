#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTimer>
#include "s3consolemanager.h"
#include "config.h"
#include "cli.h"
#include <QStringList>
#include <QDebug>
#include <QSettings>
#include <iostream>
#include <string>

using namespace std;


enum CommandLineParseResult {
    CommandLineOk,
    CommandLineError,
    CommandLineVersionRequested,
    CommandLineHelpRequested,

};

CommandLineParseResult parseCommandLine(QCommandLineParser &parser, Cli *cli, QString *errorMessage)
{
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsCompactedShortOptions);
    parser.addPositionalArgument("cmd", QCoreApplication::translate("main", "Command to operate."));
    parser.addPositionalArgument("para1", QCoreApplication::translate("main", "Parameter1"));
    parser.addPositionalArgument("para2", QCoreApplication::translate("main", "Parameter2"));
    QCommandLineOption configFileOption("c", QCoreApplication::translate("main",
                                                                         "Specify config file path."));
    QCommandLineOption recursiveOption("recursive", QCoreApplication::translate("main",
                                                                                "Recursive upload, download or removal."));
    parser.addOption(configFileOption);
    parser.addOption(recursiveOption);
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
        cli->confPath = path;
    }
    if (parser.isSet(recursiveOption)) {
        cli->recursive = true;
    } else {
        cli->recursive = false;
    }

    const QStringList positionalArguments = parser.positionalArguments();
    if (positionalArguments.isEmpty()) {
        *errorMessage = "Argument 'cmd' missing.";
        return CommandLineError;
    }

    switch (positionalArguments.size()) {
    case 1:
        cli->cmd = positionalArguments.at(0);
        break;
    case 2:
        cli->cmd = positionalArguments.at(0);
        cli->para1 = positionalArguments.at(1);
        break;
    case 3:
        cli->cmd = positionalArguments.at(0);
        cli->para1 = positionalArguments.at(1);
        cli->para2 = positionalArguments.at(2);
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
    parser.setApplicationDescription("S3Client is a tool for managing objects in Amazon S3 storage. It allows for \
                                     making and removing buckets and uploading, downloading and removing objects \
                                      from these buckets.");
    parser.addHelpOption();
    parser.addVersionOption();

    Cli cli;
    QString errorMessage;
    QS3Config config;
    QString path;
    ConfigParseResult res;
    switch (parseCommandLine(parser, &cli, &errorMessage)) {
    case CommandLineOk:
        path = DEFAULT_CONFIG;
        if (cli.confPath.length() != 0) {
            path = cli.confPath;
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

    S3ConsoleManager m(0, &config, &cli);
    QObject::connect(&m, SIGNAL(Finished()), &app, SLOT(quit()));
    QTimer::singleShot(0, &m, SLOT(Execute()));

    return app.exec();
}

