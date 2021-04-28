#ifndef VERIFIER_H
#define VERIFIER_H

#include <QObject>
#include <QFile>
#include <QStringList>
#include <QDebug>
#include <QMap>
#include <QList>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkRequest>

struct ChunkDetails
{
    ChunkDetails() { startOffset = 0; endOffset = 0; chunkSize = 0; chunkCRC = 0; knownCRC = 0; requestTime = 0; validated = false; }
    ChunkDetails(qlonglong start, qlonglong end, quint32 size, quint32 crc, qint64 time) { startOffset = start; endOffset = end; chunkSize = size; chunkCRC = crc; requestTime = time; validated = false; knownCRC = 0; }
    qlonglong startOffset;
    qlonglong endOffset;
    quint32 chunkSize;
    quint32 chunkCRC;
    quint32 knownCRC;
    qint64 requestTime;
    bool validated;
};

struct Host
{
    Host() { hostErrors = 0; }
    Host(QString ip) { hostIP = ip; hostErrors = 0; }

    QString hostIP;
    int hostErrors;
    QList<ChunkDetails> chunks;
};

struct SpanDetails
{
    SpanDetails()
    {
        clear();
    }

    void clear()
    {
        filename.clear();

        diagSpanStart = -1;
        diagSpanEnd = -1;
        spanSize = 0;
        spanFileSize = 0;
    }

    QString filename;
    qint64 diagSpanStart;
    qint64 diagSpanEnd;
    qint64 spanSize;
    qint64 spanFileSize;
};

struct DataDump
{
    DataDump() { clear(); }

    void clear() 
    { 
        active = false;  
        lineStart = 0; 
        lineEnd = 0; 
        crcErrorCount = 0; 
        cdnErrorCount = 0;  
        discontinuityCount = 0; 
        replayOK = false;
        replayErrType = 0;
        replayErrCode = 0;
        serverChecked = false; 
        spanDetails.clear();
        logFile.clear();
        cdnHostName.clear(); 
        zipFileName.clear(); 
        
        cdnUrl.clear(); 
        allHosts.clear(); 
        errorHosts.clear(); 
        sortedChunkList.clear(); 
    }

    int numRecords()
    {
        int nRecords = 0;
        for (QMap<QString, Host>::Iterator iter = allHosts.begin(); iter != allHosts.end(); iter++)
        {
            Host &cur = *iter;

            nRecords += cur.chunks.size();
        }

        return nRecords;
    }

    bool active;
    int lineStart;
    int lineEnd;
    int crcErrorCount;
    int cdnErrorCount;
    int discontinuityCount;
    bool replayOK;
    int replayErrType;
    int replayErrCode;
    bool serverChecked;
    SpanDetails spanDetails;
    QString logFile;
    QString cdnHostName;
    QString zipFileName;
    QString cdnUrl;
    QMap<QString, Host> allHosts;
    QMap<QString, Host> errorHosts;
    QMap<qint64, ChunkDetails> sortedChunkList;
};

struct RequestMetadata
{
    RequestMetadata() : didx(-1) {}
    RequestMetadata(int dumpIdx, QString ip, ChunkDetails copychunk) { didx = dumpIdx; hostIP = ip; chunk = copychunk; }
    int didx;
    ChunkDetails chunk;
    QString hostIP;
};

class Verifier : public QObject
{
    Q_OBJECT

public:
    Verifier(QStringList inputFiles, QStringList zipFiles, QStringList cdnURLs, QString replayDir);

private:
    QString findZipFilePath(QString zipName);
    void addCDNURL(QString rawUrl);
    QString findCDNURL(QString hostname, QString zipName);

    bool parseInputForLogFile(QString logFile);
    bool parseInput();
    void verifyDataDumpAgainstGameBuild(int idx, DataDump& dataDump);
    void verifyDataDumpsAgainstGameBuilds();
    void replayDataDump(int idx, DataDump& dataDump);
    void replayDataDumps();

    void checkDataDumpAgainstServer(int idx, DataDump& dataDump);

    void printSummary();

    void checkCompleted(int idx);
    void finish();
public slots:
    void run();
    void checkDataDumpsAgainstServer();
    void qnam_finished(QNetworkReply* reply);
private:
    QStringList _inputFiles;
    QStringList _zipFiles;
    QStringList _cdnURLs;
    QString _replayDir;

    QList<DataDump> _dataDumps;

    QMap<QString, RequestMetadata> _networkRequests;
    QNetworkAccessManager *_nam;
};

#endif // VERIFIER_H
