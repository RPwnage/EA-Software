#include "Verifier.h"
#include "CRC.h"
#include "ZipFileInfo/ZipInfo.h"
#include "ZipFileInfo/CorePackageFile.h"
#include "OriginStubs/UnpackStreamFile.h"

#include <QNetworkReply>
#include <QFileInfo>
#include <QDir>

#include <ShlObj.h>

Verifier::Verifier(QStringList inputFiles, QStringList zipFiles, QStringList cdnURLs, QString replayDir) :
    _inputFiles(inputFiles),
    _zipFiles(zipFiles),
    _cdnURLs(cdnURLs),
    _replayDir(replayDir),
    _nam(NULL)
{

}

QString Verifier::findZipFilePath(QString zipName)
{
    for (QStringList::iterator it = _zipFiles.begin(); it != _zipFiles.end(); it++)
    {
        QString zipFile = *it;

        if (zipFile.contains(zipName, Qt::CaseInsensitive))
            return zipFile;
    }

    return "";
}

void Verifier::addCDNURL(QString rawUrl)
{
    rawUrl = rawUrl.trimmed();

    QStringList urlParts = rawUrl.split("?", QString::SkipEmptyParts);
    if (urlParts.count() == 0)
        return;

    QString url = urlParts[0];

    if (url.startsWith("https", Qt::CaseInsensitive))
    {
        url.replace("https", "http");
        url.replace("ssl-lvlt.cdn.ea.com", "lvlt.cdn.ea.com"); // Hack this because for Level3 the hostnames are actually different
    }

    if (!_cdnURLs.contains(url))
    {
        qDebug() << "Found new URL: " << url;
        _cdnURLs.push_back(url);
    }
}

QString Verifier::findCDNURL(QString hostname, QString zipName)
{
    for (QStringList::iterator it = _cdnURLs.begin(); it != _cdnURLs.end(); it++)
    {
        QString cdnURL = *it;

        if (cdnURL.contains(hostname, Qt::CaseInsensitive) && cdnURL.contains(zipName, Qt::CaseInsensitive))
            return cdnURL;
    }

    return "";
}

bool Verifier::parseInputForLogFile(QString logFile)
{
    qDebug() << "Reading input data from: " << logFile;

    QFile inputData(logFile);

    if (!inputData.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Unable to parse input log data.";
        return false;
    }

    DataDump currentDump;

    qDebug() << "Parsing input.";

    int ignored = 0;
    int records = 0;
    int parseErrors = 0;
    int lineNum = 0;

    QTextStream textReader(&inputData);
    while (!textReader.atEnd())
    {
        QString inputLine = textReader.readLine();
        ++lineNum;

        if (inputLine.contains("################# Begin Downloader Diagnostic Data Dump #################"))
        {
            currentDump.active = true;
            currentDump.logFile = logFile;
            currentDump.lineStart = lineNum;

            qDebug() << "Found diagnostic data dump at line #" << lineNum;
        }
        else if (inputLine.contains("################# End Downloader Diagnostic Data Dump #################"))
        {
            if (currentDump.active)
            {
                currentDump.lineEnd = lineNum;

                qDebug() << "Diagnostic data dump ended at line #" << lineNum;


                // Save the current dump, and clear the current dump data
                _dataDumps.push_back(currentDump);
            }
            currentDump.clear();
        }

        if (inputLine.contains("URL: ", Qt::CaseInsensitive) && inputLine.contains("http"))
        {
            int httpPos = inputLine.lastIndexOf("http");

            QString scrapedUrl = inputLine.right(inputLine.length() - httpPos);

            addCDNURL(scrapedUrl);
        }

        // We aren't currently parsing a data dump
        if (!currentDump.active)
            continue;

        // Interested in this substring: DL CHECK [File=BF4_Beta_v67478_09252013.zip][IP=63.80.4.168][Start=495229685][End=495491829][Size=262144][CRC=143125884]
        //Content$+ID~OFB-EAST_109552409++File~the_sims_4_pc_fg__ww_us_beta_658__pcbgmlrprodretaildip_110631020_5cd5466e50694b599df4f2e912a65c94.zip+Host$+Name~origin-a.akamaihd.net++IP~72.247.182.27+Ck$+Time~55f32bd6++Rng~ccb85ef7-ccc6fef7++CRC~2334a268+Ck$+Time~55f32bdd++Rng~ccc6fef7-cd35fef7++CRC~f98b9e9b+Ck$+Time~55f32be0++Rng~cd35fef7-cda2fef7++CRC~2d05356d+Ck$+Time~55f32be5++Rng~cda2fef7-ce2afef7++CRC~928e782a+Ck$+Time~55f32bf2++Rng~ce2afef7-ceda7ef7++CRC~35021efc+Ck$+Time~55f32bfc++Rng~ceda7ef7-cf487ef7++CRC~1ba0565e+Ck$+Time~55f32c03++Rng~cf487ef7-cfa77ef7++CRC~83df2e80+Ck$+Time~55f32c0b++Rng~cfa77ef7-d00f7ef7++CRC~c296d152+Ck$+Time~55f32c12++Rng~d00f7ef7-d0717ef7++CRC~97e734f5+Ck$+Time~55f32c19++Rng~d0717ef7-d0dafef7++CRC~b83c19e6+Ck$+Time~55f32c21++Rng~d0dafef7-d14a7ef7++CRC~c41eb375+Ck$+Time~55f32c29++Rng~d14a7ef7-d1bb7ef7++CRC~65de7cef+Ck$+Time~55f32c60++Rng~d1bb7ef7-d1bbfef7++CRC~6237ca5d+Ck$+Time~55f32c94++Rng~d1bbfef7-d1ebfef7++CRC~51eb84e+Ck$+Time~55f32cab++Rng~d1ebfef7-d1ec96f7++CRC~dcb0055+Ck$+Time~55f32cab++Rng~d1ec96f7-d1f116f7++CRC~6793d3a3+

        if (!inputLine.contains("+Ck$+"))
            continue;

        // Chop off all the crap we don't need
        inputLine = inputLine.right(inputLine.length() - inputLine.indexOf("++File"));

        QStringList splitvals = inputLine.split("+", QString::SkipEmptyParts);

        if (splitvals.count() < 4)
        {
            qDebug() << "Couldn't split line: " << inputLine << " (Line " << lineNum << ")";
            ++parseErrors;
            continue;
        }

        QString file = splitvals.at(0);

        QStringList fileVals = file.split("~", QString::SkipEmptyParts);
        if (fileVals.count() != 2)
        {
            qDebug() << "Couldn't split line: " << inputLine << " (Line " << lineNum << ")";
            ++parseErrors;
            continue;
        }

        file = fileVals.at(1);

        // Make sure we actually had a path for this one
        QString zipFullPath = findZipFilePath(file);
        if (zipFullPath.isEmpty())
        {
            ++ignored;
            continue;
        }

        // Save the zip filename
        if (currentDump.zipFileName.isEmpty())
            currentDump.zipFileName = file;

        bool parseError = false;
        QString ip;
        for (int linePart = 2; linePart < splitvals.count(); ++linePart)
        {
            QString part = splitvals.at(linePart);
            if (part.startsWith("Name"))
            {
                QStringList hostnameParts = part.split("~", QString::SkipEmptyParts);
                if (hostnameParts.count() != 2)
                {
                    parseError = true;
                    qDebug() << "Couldn't split Hostname: " << inputLine << " (Line " << lineNum << ")";
                    ++parseErrors;
                    break;
                }

                QString hostname = hostnameParts.at(1);
                if (currentDump.cdnHostName.isEmpty())
                    currentDump.cdnHostName = hostname;
            }
            else if (part.startsWith("IP"))
            {
                QStringList ipParts = part.split("~", QString::SkipEmptyParts);
                if (ipParts.count() != 2)
                {
                    parseError = true;
                    qDebug() << "Couldn't split IP: " << inputLine << " (Line " << lineNum << ")";
                    ++parseErrors;
                    break;
                }

                ip = ipParts.at(1);

                // Insert the IP if needed
                if (!currentDump.allHosts.contains(ip))
                {
                    currentDump.allHosts.insert(ip, Host(ip));
                }
            }
            else if (part == "Ck$")
            {
                // Not valid, there aren't enough tokens left
                if (linePart + 3 >= splitvals.count())
                {
                    parseError = true;
                    qDebug() << "Couldn't split chunkdata: " << inputLine << " (Line " << lineNum << ")";
                    ++parseErrors;
                    break;
                }

                QString timePart = splitvals.at(linePart + 1);
                QString rangePart = splitvals.at(linePart + 2);
                QString crcPart = splitvals.at(linePart + 3);

                QStringList timeParts = timePart.split("~", QString::SkipEmptyParts);
                QStringList rangeParts = rangePart.split("~", QString::SkipEmptyParts);
                QStringList crcParts = crcPart.split("~", QString::SkipEmptyParts);

                if (timeParts.count() != 2 || rangeParts.count() != 2 || crcParts.count() != 2)
                {
                    parseError = true;
                    qDebug() << "Couldn't split chunkdata parts: " << inputLine << " (Line " << lineNum << ")";
                    ++parseErrors;
                    break;
                }

                QStringList rangeSubParts = rangeParts.at(1).split("-", QString::SkipEmptyParts);
                if (rangeSubParts.count() != 2)
                {
                    parseError = true;
                    qDebug() << "Couldn't split chunkdata range parts: " << inputLine << " (Line " << lineNum << ")";
                    ++parseErrors;
                    break;
                }

                QString strstart = rangeSubParts.at(0);
                QString strend = rangeSubParts.at(1);
                QString strcrc = crcParts.at(1);
                QString strtime = timeParts.at(1);

                qlonglong start = strstart.toLongLong(NULL, 16);
                qlonglong end = strend.toLongLong(NULL, 16);
                quint32 size = (quint32)(end - start);
                quint32 crc = strcrc.toULong(NULL, 16);
                qint64 time = strtime.toLongLong(NULL, 16);

                ++records;
                currentDump.allHosts[ip].chunks.push_back(ChunkDetails(start, end, size, crc, time));


                // Skip ahead
                linePart += 3;

                //qDebug() << "File = " << file << " IP = " << ip << " Start/End = " << start << "/" << end << " Size = " << size << " CRC = " << crc << " " << strcrc;
            }
        }

        if (parseError)
        {
            continue;
        }
    }

    qDebug() << "Parsed " << records << " records.  Ignored " << ignored << " records.  Errors: " << parseErrors;
    qDebug() << "Found " << _dataDumps.count() << " total diagnostic data dumps.";
    qDebug() << "";

    if (records > 0)
        return true;
    return false;
}

bool Verifier::parseInput()
{
    bool success = false;
    for (QStringList::iterator it = _inputFiles.begin(); it != _inputFiles.end(); it++)
    {
        QString inputFile = *it;

        success |= parseInputForLogFile(inputFile);
    }

    return success;
}

void Verifier::verifyDataDumpAgainstGameBuild(int idx, DataDump& dataDump)
{
    qDebug() << "Verifying data dump (#" << idx << ") for zip file: " << dataDump.zipFileName << " (Input lines " << dataDump.lineStart << " - " << dataDump.lineEnd << ")";

    QString zipFullPath = findZipFilePath(dataDump.zipFileName);
    QFile verifyZip(zipFullPath);
    if (!verifyZip.open(QIODevice::ReadOnly))
    {
        qDebug() << "Unable to open zip.";
        return;
    }

    qint64 zipSize = verifyZip.size();
    int records = dataDump.numRecords();
    int errorCount = 0;
    int processed = 0;

    for (QMap<QString, Host>::Iterator iter = dataDump.allHosts.begin(); iter != dataDump.allHosts.end(); iter++)
    {
        Host &cur = *iter;

        qDebug() << "Host: " << cur.hostIP << " contains " << cur.chunks.size() << " chunks.";

        for (QList<ChunkDetails>::Iterator iter = cur.chunks.begin(); iter != cur.chunks.end(); ++iter)
        {
            ChunkDetails &chunk = *iter;

            ++processed;
            if ((processed % 100) == 0 || processed == records)
                qDebug() << "   * Processed " << processed << " chunks.";

            // Insert it in the map
            dataDump.sortedChunkList[chunk.startOffset] = chunk;

            qint64 chunkSize = chunk.endOffset - chunk.startOffset;

            if (chunkSize != chunk.chunkSize)
            {
                qDebug() << "ERROR: Chunk size mismatch.  Calculated: " << chunkSize << " Actual: " << chunk.chunkSize;
                ++cur.hostErrors;
                ++errorCount;
                continue;
            }

            if (chunk.startOffset > zipSize || chunk.endOffset > zipSize)
            {
                qDebug() << "ERROR: Chunk offsets out of bounds! Start/End: " << chunk.startOffset << "/" << chunk.endOffset << " Zip Size: " << zipSize;
                ++cur.hostErrors;
                ++errorCount;
                continue;
            }

            verifyZip.seek(chunk.startOffset);

            QByteArray bytes = verifyZip.read(chunkSize);

            if (bytes.size() != chunkSize)
            {
                qDebug() << "ERROR: Read " << bytes.size() << " bytes, expected " << chunkSize << " bytes.";
                ++cur.hostErrors;
                ++errorCount;
                continue;
            }

            const char* buffer = bytes.constData();

            quint32 crc = CRCMap::CalculateCRCForChunk(buffer, chunkSize);

            if (crc != chunk.chunkCRC)
            {
                chunk.knownCRC = crc;

                qDebug() << "ERROR: Host: " << cur.hostIP << " Range: " << chunk.startOffset << "-" << chunk.endOffset << "Computed CRC: " << crc << " Expected CRC: " << chunk.chunkCRC << " Request Timestamp: " << chunk.requestTime;
                ++cur.hostErrors;
                ++errorCount;

                //resultFile << "ERROR Host: " << cur.hostIP << " Range: " << chunk.startOffset << "-" << chunk.endOffset << " Computed CRC: " << crc << " Expected CRC: " << chunk.chunkCRC << "\n";

                // Save the chunk details to the error list
                if (!dataDump.errorHosts.contains(cur.hostIP))
                {
                    dataDump.errorHosts.insert(cur.hostIP, Host(cur.hostIP));
                }

                dataDump.errorHosts[cur.hostIP].chunks.push_back(chunk);

                //break;
                continue;
            }

            chunk.validated = true;
        }

        if (cur.hostErrors > 0)
            qDebug() << "Host: " << cur.hostIP << " had " << cur.hostErrors << " errors.";
    }

    dataDump.crcErrorCount = errorCount;
    qDebug() << errorCount << " errors detected.";
    qDebug() << "";

    qDebug() << "Looking for diagnostic span discontinuities...";
    // Go over the sorted chunk list and look for overlaps or gaps
    int discontinuitiesFound = 0;
    qint64 lastChunkEnd = -1;
    qint64 diagSpanStart = -1;
    qint64 diagSpanEnd = -1;
    for (QMap<qint64, ChunkDetails>::iterator chunkIter = dataDump.sortedChunkList.begin(); chunkIter != dataDump.sortedChunkList.end(); ++chunkIter)
    {
        ChunkDetails chunk = chunkIter.value();

        // Initialize
        if (lastChunkEnd == -1)
        {
            lastChunkEnd = chunk.startOffset;
            diagSpanStart = chunk.startOffset;
        }

        diagSpanEnd = chunk.endOffset;

        if (chunk.startOffset > lastChunkEnd)
        {
            ++discontinuitiesFound;
            qint64 differential = chunk.startOffset - lastChunkEnd;
            qDebug() << "Gap at " << chunk.startOffset << ", " << differential << " bytes.";
        }
        else if (chunk.startOffset < lastChunkEnd)
        {
            ++discontinuitiesFound;
            qint64 differential = lastChunkEnd - chunk.startOffset;
            qDebug() << "Overlap at " << chunk.startOffset << ", " << differential << " bytes.";
        }

        lastChunkEnd = chunk.endOffset;
    }

    dataDump.spanDetails.diagSpanStart = diagSpanStart;
    dataDump.spanDetails.diagSpanEnd = diagSpanEnd;
    dataDump.discontinuityCount = discontinuitiesFound;

    if (discontinuitiesFound == 0)
    {
        qDebug() << "No discontinuities found.";
    }
    else
    {
        qDebug() << discontinuitiesFound << " discontinuities found.";
    }
    qDebug() << "";
}

void Verifier::verifyDataDumpsAgainstGameBuilds()
{
    int idx = 0;
    for (QList<DataDump>::Iterator iter = _dataDumps.begin(); iter != _dataDumps.end(); iter++, idx++)
    {
        DataDump& curDump = *iter;

        verifyDataDumpAgainstGameBuild(idx, curDump);
    }
}

void Verifier::replayDataDump(int idx, DataDump& dataDump)
{
    qDebug() << "Calculating zip info for data dump (#" << idx << ") for zip file: " << dataDump.zipFileName << " (Input lines " << dataDump.lineStart << " - " << dataDump.lineEnd << ")";

    QString zipFullPath = findZipFilePath(dataDump.zipFileName);
    QFile verifyZip(zipFullPath);
    if (!verifyZip.open(QIODevice::ReadOnly))
    {
        qDebug() << "Unable to open zip.";
        return;
    }
    qint64 zipSize = verifyZip.size();

    PackageFile* zipDirectory = ZipInfo::processZipFile(zipFullPath);
    qDebug() << "";
    PackageFileEntry *spanEntry = NULL;
    qint64 diagSpanSize = (dataDump.spanDetails.diagSpanEnd - dataDump.spanDetails.diagSpanStart);
    qDebug() << "Diagnostic data span size : " << diagSpanSize << " bytes. (" << dataDump.spanDetails.diagSpanStart << " - " << dataDump.spanDetails.diagSpanEnd << ")";
    PackageFileEntryList files = zipDirectory->GetEntries();
    for (PackageFileEntryList::iterator entry = files.begin(); entry != files.end(); entry++)
    {
        PackageFileEntry *pkgfile = *entry;

        if (dataDump.spanDetails.diagSpanStart >= pkgfile->GetOffsetToHeader() && dataDump.spanDetails.diagSpanStart < pkgfile->GetEndOffset())
        {
            qint64 zipDataSize = pkgfile->GetCompressedSize();
            qint64 adjDiagSpanSize = diagSpanSize;
            // Account for the header
            if (dataDump.spanDetails.diagSpanStart == pkgfile->GetOffsetToHeader())
                adjDiagSpanSize += (pkgfile->GetOffsetToFileData() - pkgfile->GetOffsetToHeader());
            double coveragePct = (adjDiagSpanSize / (double)zipDataSize) * 100.0;
            qDebug() << "Span matches file: " << pkgfile->GetFileName() << " Size: " << zipDataSize << " Header start: " << pkgfile->GetOffsetToHeader() << " Filedata start: " << pkgfile->GetOffsetToFileData() << " End: " << pkgfile->GetEndOffset() << " Coverage%: " << coveragePct;

            spanEntry = pkgfile;

            dataDump.spanDetails.filename = pkgfile->GetFileName();
            dataDump.spanDetails.spanSize = adjDiagSpanSize;
            dataDump.spanDetails.spanFileSize = pkgfile->GetCompressedSize();

            break;
        }
    }

    if (spanEntry == NULL)
    {
        qDebug() << "Unable to match data dump span to zip file entry.  Skipping replay.";
        return;
    }

    if (dataDump.crcErrorCount > 0 || dataDump.discontinuityCount > 0)
    {
        qDebug() << "Data dump (#" << idx << ") for zip file: " << dataDump.zipFileName << " (Input lines " << dataDump.lineStart << " - " << dataDump.lineEnd << ") had CRC errors or discontinuities.  Skipping replay.";
        qDebug() << "";

        return;
    }

    qDebug() << "Replaying data dump (#" << idx << ") for zip file: " << dataDump.zipFileName << " (Input lines " << dataDump.lineStart << " - " << dataDump.lineEnd << ")";

    bool headerConsumed = false;

    qint64 headerOffset = spanEntry->GetOffsetToHeader();

    // If there is no header, adjust for that
    if (headerOffset == 0)
    {
        headerOffset = spanEntry->GetOffsetToFileData();
        headerConsumed = true;
    }

    QString outputPath = _replayDir + spanEntry->GetFileName();
    QFileInfo outputInfo(outputPath);
    if (!outputInfo.absoluteDir().exists())
    {
        QString targetDir = outputInfo.absoluteDir().absolutePath();
        targetDir.replace("/", "\\");
        SHCreateDirectoryEx(NULL, (LPCWSTR)targetDir.utf16(), NULL);
    }

    quint32 bytesProcessed = 0;
    bool fileComplete = false;
    Origin::Downloader::UnpackStreamFile *streamFile = new Origin::Downloader::UnpackStreamFile(outputPath, spanEntry->GetCompressedSize(), spanEntry->GetUncompressedSize(), 0, 0, 0, (Origin::Downloader::UnpackType::code)spanEntry->GetCompressionMethod(), spanEntry->GetFileCRC(), "test", false);

    const quint32 defaultChunkSize = 1024 * 1024;
    qint64 lastOffset = -1;

    bool fatalError = false;
    for (QMap<qint64, ChunkDetails>::iterator chunkIter = dataDump.sortedChunkList.begin(); chunkIter != dataDump.sortedChunkList.end(); ++chunkIter)
    {
        ChunkDetails chunk = chunkIter.value();

        qint64 chunkSize = chunk.endOffset - chunk.startOffset;

        if (chunkSize != chunk.chunkSize)
        {
            qDebug() << "ERROR: Chunk size mismatch.  Calculated: " << chunkSize << " Actual: " << chunk.chunkSize;
            continue;
        }

        if (chunk.startOffset > zipSize || chunk.endOffset > zipSize)
        {
            qDebug() << "ERROR: Chunk offsets out of bounds! Start/End: " << chunk.startOffset << "/" << chunk.endOffset << " Zip Size: " << zipSize;
            continue;
        }

        lastOffset = chunk.endOffset;

        verifyZip.seek(chunk.startOffset);

        QByteArray bytes = verifyZip.read(chunkSize);

        if (bytes.size() != chunkSize)
        {
            qDebug() << "ERROR: Read " << bytes.size() << " bytes, expected " << chunkSize << " bytes.";

            continue;
        }

        char* pBuffer = (char*)bytes.constData();
        int dataLen = bytes.size();

        if (!headerConsumed && chunk.startOffset == headerOffset)
        {
            qDebug() << "Consuming zip local file header";

            // Figure out the new buffer
            int headerSize = spanEntry->GetOffsetToFileData() - headerOffset;
            pBuffer = pBuffer + headerSize;
            dataLen -= headerSize;

            headerConsumed = true;
        }

        if (!streamFile->processChunk(pBuffer, dataLen, bytesProcessed, fileComplete))
        {
            qDebug() << "Unpack stream file failure.";
            fatalError = true;

            dataDump.replayOK = false;
            dataDump.replayErrType = (int)streamFile->getErrorType();
            dataDump.replayErrCode = streamFile->getErrorCode();
            break;
        }
    }

    if (!fatalError && !fileComplete)
    {
        qDebug() << "*** Reached end of diagnostic data stream with no unpack errors.  Simulating remainder of the file.";
        qDebug() << "";
    }

    while (!fatalError && lastOffset != -1 && lastOffset < spanEntry->GetEndOffset())
    {
        qint64 chunkSize = defaultChunkSize;
        if (chunkSize > (spanEntry->GetEndOffset() - lastOffset))
            chunkSize = spanEntry->GetEndOffset() - lastOffset;

        verifyZip.seek(lastOffset);

        QByteArray bytes = verifyZip.read(chunkSize);

        if (bytes.size() != chunkSize)
        {
            qDebug() << "ERROR: Read " << bytes.size() << " bytes, expected " << chunkSize << " bytes.";

            break;
        }

        char* pBuffer = (char*)bytes.constData();
        int dataLen = bytes.size();

        if (!streamFile->processChunk(pBuffer, dataLen, bytesProcessed, fileComplete))
        {
            qDebug() << "Unpack stream file failure.";
            fatalError = true;

            dataDump.replayOK = false;
            dataDump.replayErrType = (int)streamFile->getErrorType();
            dataDump.replayErrCode = streamFile->getErrorCode();
            break;
        }

        if (fileComplete)
        {
            break;
        }

        lastOffset += chunkSize;
    }

    if (fileComplete)
    {
        qDebug() << "Unpack stream file complete.";

        dataDump.replayOK = true;
    }

    qDebug() << "";
}

void Verifier::replayDataDumps()
{
    if (_replayDir.isEmpty())
    {
        qDebug() << "No replay directory specified, skipping replay.";
        return;
    }

    int idx = 0;
    for (QList<DataDump>::Iterator iter = _dataDumps.begin(); iter != _dataDumps.end(); iter++, idx++)
    {
        DataDump& curDump = *iter;

        replayDataDump(idx, curDump);
    }
}

void Verifier::printSummary()
{
    qDebug() << "******* Summary *******";
    int idx = 0;
    for (QList<DataDump>::Iterator iter = _dataDumps.begin(); iter != _dataDumps.end(); iter++, idx++)
    {
        DataDump& curDump = *iter;

        qDebug() << "Data dump #" << idx << " had " << curDump.numRecords() << " chunk records. (Input lines " << curDump.lineStart << " - " << curDump.lineEnd << ")";
        qDebug() << "Input file: " << curDump.logFile;
        qDebug() << "Used local zip file: " << findZipFilePath(curDump.zipFileName);
        qDebug() << "Span corresponded to package file: " << curDump.spanDetails.filename << " (" << curDump.spanDetails.spanSize << " of " << curDump.spanDetails.spanFileSize << " bytes)";
        qDebug() << " - " << curDump.crcErrorCount << " CRC failures.";
        qDebug() << " - " << curDump.discontinuityCount << " Discontinuities.";
        if (curDump.crcErrorCount == 0 && curDump.discontinuityCount == 0)
        {
            if (!curDump.replayOK)
            {
                qDebug() << " - Unpack Replay FAILED: Error " << curDump.replayErrType << ":" << curDump.replayErrCode;
            }
            else
            {
                qDebug() << " - Unpack Replay OK.";
            }
        }
        if (curDump.serverChecked)
        {
            QString cdnURL = findCDNURL(curDump.cdnHostName, curDump.zipFileName);
            if (!cdnURL.isEmpty())
            {
                qDebug() << "Used CDN URL: " << cdnURL;
                qDebug() << " - " << curDump.cdnErrorCount << " CDN errors.";
            }
        }
        qDebug() << "";
    }
    qDebug() << "";
}

void Verifier::run()
{
    if (!parseInput())
        return;

    verifyDataDumpsAgainstGameBuilds();
    
    replayDataDumps();

    if (_cdnURLs.count() == 0)
    {
        qDebug() << "No CDN URLs specified.  Skipping online check.";

        finish();
    }
    else
    {
        checkDataDumpsAgainstServer();
    }
}

void Verifier::checkDataDumpAgainstServer(int idx, DataDump& dataDump)
{
    QString cdnURL = findCDNURL(dataDump.cdnHostName, dataDump.zipFileName);
    if (cdnURL.isEmpty())
    {
        qDebug() << "No CDN URL found for data dump (#" << idx << ") for zip file: " << dataDump.zipFileName << " (Input lines " << dataDump.lineStart << " - " << dataDump.lineEnd << ").  Skipping CDN check.";

        // Keep checking the rest of the dumps
        QMetaObject::invokeMethod(this, "checkDataDumpsAgainstServer", Qt::QueuedConnection);

        return;
    }

    qDebug() << "Checking data dump (#" << idx << ") against CDN for zip file: " << dataDump.zipFileName << " (Input lines " << dataDump.lineStart << " - " << dataDump.lineEnd << ")";
    qDebug() << "CDN Hostname: " << dataDump.cdnHostName;

    QString rangeStr("bytes=%1-%2");

    QString urlFormat(cdnURL);

    QString hostname = QUrl(urlFormat).host();

    for (QMap<QString, Host>::Iterator iter = dataDump.errorHosts.begin(); iter != dataDump.errorHosts.end(); iter++)
    {
        Host &cur = *iter;

        QString hosturl(urlFormat);

        hosturl.replace(hostname, cur.hostIP);

        qDebug() << hosturl << " contains " << cur.chunks.size() << " bad chunks.";

        int c = 0;
        for (QList<ChunkDetails>::Iterator iter = cur.chunks.begin(); iter != cur.chunks.end(); ++iter)
        {
            c++;

            ChunkDetails &chunk = *iter;

            QNetworkRequest request;

            request.setUrl(QUrl(hosturl));

            QString rangeHeader = rangeStr.arg(chunk.startOffset).arg(chunk.endOffset-1);

            request.setRawHeader("Range", rangeHeader.toLocal8Bit());

            request.setRawHeader("Host", hostname.toLocal8Bit());

            QString requestKey = cur.hostIP + "/" + rangeHeader;

            _networkRequests.insert(requestKey, RequestMetadata(idx, cur.hostIP, chunk));

            qDebug() << "Chunk " << c << ": Requesting " << (chunk.endOffset - chunk.startOffset) << " bytes.  Range: " << chunk.startOffset << "-" << chunk.endOffset;
            //qDebug() << requestKey;

            // Submit the request to NAM
            _nam->get(request);
        }
    }

    qDebug() << "All requests submitted.";
}

void Verifier::checkDataDumpsAgainstServer()
{
    if (!_nam)
    {
        // Initialize the QNAM stuff
        _nam = new QNetworkAccessManager(this);
        QObject::connect(_nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(qnam_finished(QNetworkReply*)));
    }

    int idx = 0;
    for (QList<DataDump>::Iterator iter = _dataDumps.begin(); iter != _dataDumps.end(); iter++, idx++)
    {
        DataDump& curDump = *iter;

        if (curDump.serverChecked)
            continue;

        if (curDump.crcErrorCount == 0)
        {
            qDebug() << "Data dump (#" << idx << ") for zip file: " << curDump.zipFileName << " (Input lines " << curDump.lineStart << " - " << curDump.lineEnd << ") had no CRC errors  Skipping CDN check.";
            continue;
        }

        curDump.serverChecked = true;
        
        checkDataDumpAgainstServer(idx, curDump);

        // We'll call this whole function again when this dump finishes processing
        return;
    }

    // We're all done
    finish();
}

void Verifier::qnam_finished(QNetworkReply* reply)
{
    //QDebug resultFile(&_outputFileObj);

    QString range = reply->request().rawHeader("Range");
    QString hostIP = reply->request().url().host();

    QString requestKey = hostIP + "/" + range;
    if (!_networkRequests.contains(requestKey))
    {
        qDebug() << "Couldn't match request to response. " << requestKey << _networkRequests.count();
        return;
    }

    RequestMetadata metadata = _networkRequests[requestKey];

    // Clean up the metadata
    _networkRequests.remove(requestKey);

    if (reply->error() != QNetworkReply::NoError)
    {
        qDebug() << "ERROR: Unable to read from host: " << hostIP << " Error: " << reply->errorString();
        return;
    }
    if (reply->bytesAvailable() == 0)
    {
        qDebug() << "ERROR: Received response from host: " << hostIP << " No data to read!";
        return;
    }

    QByteArray networkdata = reply->readAll();

    const char* buffer = networkdata.constData();

    quint32 crc = CRCMap::CalculateCRCForChunk(buffer, networkdata.size());

    if (crc == metadata.chunk.knownCRC)
    {
        qDebug() << "Received chunk data from host: " << hostIP << " Bytes:" << networkdata.size() << " Range: " << metadata.chunk.startOffset << "-" << metadata.chunk.endOffset << " - CRC OK";
    }
    else
    {
        _dataDumps[metadata.didx].cdnErrorCount++;

        qDebug() << "Received chunk data from host: " << hostIP << " Bytes:" << networkdata.size() << " Range: " << metadata.chunk.startOffset << "-" << metadata.chunk.endOffset << " - CRC FAIL";

        qDebug() << "CDN ERROR Host: " << hostIP << " Range: " << metadata.chunk.startOffset << "-" << metadata.chunk.endOffset << " Server CRC: " << crc << " Known Good CRC: " << metadata.chunk.knownCRC;
        //resultFile << "CDN ERROR Host: " << hostIP << " Range: " << metadata.chunk.startOffset << "-" << metadata.chunk.endOffset << " Server CRC: " << crc << " Known Good CRC: " << metadata.chunk.knownCRC;
        //qDebug() << "Received data CRC: " << crc << " Known correct CRC: " << metadata.chunk.knownCRC << " User log CRC: " << metadata.chunk.chunkCRC;
    }

    reply->close();

    checkCompleted(metadata.didx);
}

void Verifier::checkCompleted(int idx)
{
    if (_networkRequests.size() > 0)
        return;

    qDebug() << "Requests complete.";

    qDebug() << "Detected " << _dataDumps[idx].cdnErrorCount << " bad chunks on CDN.";
    qDebug() << "";

    // Keep checking the rest of the dumps
    QMetaObject::invokeMethod(this, "checkDataDumpsAgainstServer", Qt::QueuedConnection);
}

void Verifier::finish()
{
    qDebug() << "All tests completed.";
    qDebug() << "";

    printSummary();

    exit(0);
}
