#include <QtCore/QCoreApplication>
#include <QFile>
#include <QStringList>
#include <QDebug>
#include <QMap>
#include <QDir>
#include <QList>
#include <QFileInfo>
#include <QTimer>
#include <QDirIterator>
#include <Windows.h>
#include "Verifier.h"

FILE* ofile = NULL;

void myDebugOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    //wchar_t debugMessage[1024] = { 0 };
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
    case QtInfoMsg:
    case QtWarningMsg:
    case QtCriticalMsg:
    case QtFatalMsg:
        //OutputDebugString((LPCWSTR)msg.utf16());
        printf("%s\n", localMsg.constData());
        if (ofile)
        {
            fprintf(ofile, "%s\n", localMsg.constData());
        }
        break;
    }
}

QString splitINI(QString splitLine)
{
    QStringList parts = splitLine.split("=", QString::SkipEmptyParts);
    if (parts.count() != 2)
        return QString();
    return parts.at(1);
}

QString convertToAbsolute(QString root, QString file, bool useRelative = true)
{
    if (file.isEmpty())
        return file;

    QFileInfo finfo(file);
    if (finfo.isAbsolute())
        return file;

    // Is it just already in our path?
    if (useRelative)
    {
        if (finfo.exists())
            return file;
    }

    return root + file;
}

void getAllFiles(QString pathToFiles, QString filter, QStringList& zipFiles)
{
    QDirIterator it(pathToFiles, QStringList() << filter, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QString zipFile = it.next();
        zipFiles.push_back(zipFile);
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qInstallMessageHandler(myDebugOutput);

    QString inputFilename;

    // Help me debug!
    if (argc < 2)
    {
        qDebug() << "USAGE: DownloadCRCParser <inputfile>";

#ifdef _DEBUG
        _wchdir(L"C:\\Dev\\EA\\Test\\corruption\\");

        inputFilename = "input.txt";
#else
        return -1;
#endif
    }
    else
    {
        inputFilename = argv[1];
    }

    QFileInfo inputFileInfo(inputFilename);

    QString inputRelativePath = inputFileInfo.absoluteDir().absolutePath() + QDir::separator();

    QFile inputData(inputFilename);

    if (!inputData.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Unable to parse input file: " << inputFilename;
        return -1;
    }

    QStringList logFiles;
    QStringList verifyZipFiles;
    QString outputFile;
    QStringList cdnURLs;
    QString replayDir;

    QTextStream textReader(&inputData);
    while (!textReader.atEnd())
    {
        QString inputLine = textReader.readLine();

        // Ignore comments and whitespace
        if (inputLine.startsWith(";") || inputLine.isEmpty() || inputLine == "\n")
            continue;

        if (inputLine.startsWith("OUTPUTFILE=", Qt::CaseInsensitive))
        {
            outputFile = splitINI(inputLine);
        }
        else if (inputLine.startsWith("INPUTFILES=", Qt::CaseInsensitive))
        {
            QString logFilesPath = splitINI(inputLine);

            logFilesPath = convertToAbsolute(inputRelativePath, logFilesPath);

            getAllFiles(logFilesPath, "*.txt", logFiles);
        }
        else if (inputLine.startsWith("ZIPFILES=", Qt::CaseInsensitive))
        {
            QString zipFilesPath = splitINI(inputLine);

            zipFilesPath = convertToAbsolute(inputRelativePath, zipFilesPath);

            getAllFiles(zipFilesPath, "*.zip", verifyZipFiles);
        }
        else if (inputLine.startsWith("CDNURL=", Qt::CaseInsensitive))
        {
            QString cdnURL = splitINI(inputLine);

            cdnURLs.push_back(cdnURL);
        }
        else if (inputLine.startsWith("REPLAYDIR=", Qt::CaseInsensitive))
        {
            replayDir = splitINI(inputLine);
        }
    }

    replayDir = convertToAbsolute(inputRelativePath, replayDir);

    if (logFiles.isEmpty())
    {
        qDebug() << "Your input folder must contain at least one log file.";
        return -1;
    }

    for (int i = 0; i < verifyZipFiles.count(); ++i)
    {
        QString verifyZipFile = verifyZipFiles[i];
        if (verifyZipFile.isEmpty() || !QFile::exists(verifyZipFile))
        {
            qDebug() << "Unable to open zip file! " << verifyZipFile;
            return -1;
        }
    }

    QDir replayDirInfo(replayDir);
    if (!replayDir.isEmpty() && !replayDirInfo.exists())
    {
        qDebug() << "Replay dir specified but does not exist! " << replayDir;
        return -1;
    }
    if (!replayDir.isEmpty() && !replayDir.endsWith("\\"))
    {
        replayDir.append("\\");
    }

    if (!outputFile.isEmpty())
    {
        outputFile = convertToAbsolute(inputRelativePath, outputFile, false);

        ofile = fopen(outputFile.toLocal8Bit().constData(), "w");
        if (!ofile)
        {
            qDebug() << "Unable to open output file!";
            return -1;
        }
    }

    for (int i = 0; i < cdnURLs.count(); ++i)
    {
        QString cdnURL = cdnURLs[i];
        if (!cdnURL.isEmpty() && !cdnURL.startsWith("http://", Qt::CaseInsensitive))
        {
            qDebug() << "URLs must use HTTP only. - " << cdnURL;
            return -1;
        }
    }

    if (verifyZipFiles.count() == 0)
    {
        qDebug() << "Your input folders must contain at least one zip file!";
        return -1;
    }

    qDebug() << "Client Log Files: " << logFiles.count();
    for (QStringList::iterator iter = logFiles.begin(); iter != logFiles.end(); iter++)
    {
        QString logFile = *iter;

        qDebug() << " - " << logFile;
    }
    qDebug() << "Zip Files: " << verifyZipFiles.count();
    for (QStringList::iterator iter = verifyZipFiles.begin(); iter != verifyZipFiles.end(); iter++)
    {
        QString zipFile = *iter;

        qDebug() << " - " << zipFile;
    }
    qDebug() << "CDN URLs: " << cdnURLs.count();
    for (QStringList::iterator iter = cdnURLs.begin(); iter != cdnURLs.end(); iter++)
    {
        QString cdnURL = *iter;

        qDebug() << " - " << cdnURL;
    }
    qDebug() << "";

    Verifier myObj(logFiles, verifyZipFiles, cdnURLs, replayDir);
    QTimer::singleShot(10, &myObj, SLOT(run()));

    return app.exec();
}
