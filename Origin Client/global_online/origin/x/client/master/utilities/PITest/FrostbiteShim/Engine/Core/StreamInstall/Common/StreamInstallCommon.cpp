#include "StreamInstallCommon.h"

#include <QApplication>
#include <QFile>
#include <QJsonDocument>

namespace fb
{
    void MallocScope()
    {
        // Do nothing
    }

    StreamInstallCommon::StreamInstallCommon()
    {
        
    }

    bool StreamInstallCommon::manifestExist()
    {
        if (_manifestPath.isEmpty())
        {
            initManifestPath();
        }

        return QFile::exists(_manifestPath);
    }

    const char * StreamInstallCommon::getManifestFilename()
    {
        static char buffer[512];
        memset(buffer, 0, sizeof(buffer));

        if (_manifestPath.isEmpty())
        {
            initManifestPath();
        }

        strncpy_s<sizeof(buffer)>(buffer, _manifestPath.toLatin1().constData(), sizeof(buffer));

        return buffer;
    }


    bool StreamInstallCommon::superbundleExists(const char* superbundle, MediaHint hint) { return false; }
    void StreamInstallCommon::mountSuperbundle(const char* superbundle, MediaHint hint, bool optional) { }
    void StreamInstallCommon::unmountSuperbundle(const char* superbundle) { }
    bool StreamInstallCommon::isInstalling() { return false; }
    bool StreamInstallCommon::isSuperbundleInstalled(const char* fileName) { return false; }
    bool StreamInstallCommon::isGroupInstalled(const char* groupName) { return false; }

    void StreamInstallCommon::parseManifest(ManifestContent& manifest)
    {
        if (_manifestPath.isEmpty())
        {
            initManifestPath();
        }

        FB_INFO_FORMAT("StreamInstallCommon::parseManifest - Loading chunk manifest from: %s", _manifestPath.toLatin1().constData());

        QFile chunkManifest(_manifestPath);
        if (!chunkManifest.open(QIODevice::ReadOnly))
        {
            FB_WARNING_FORMAT("StreamInstallCommon::parseManifest - Unable to open manifest.");
            return;
        }

        QJsonParseError parseError;
        QJsonDocument manifestDoc = QJsonDocument::fromJson(chunkManifest.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError)
        {
            FB_WARNING_FORMAT("StreamInstallCommon::parseManifest - JSON parse error: %d ; %s", (int)parseError.error, parseError.errorString().toLatin1().constData());
            return;
        }

        QJsonObject rootObj = manifestDoc.object();
        manifest.m_startChunkId = rootObj["StartChunkId"].toInt();
        manifest.m_intialChunkCount = rootObj["StartChunks"].toInt();

        QJsonArray allChunks = rootObj["AllChunks"].toArray();
        for (int idx = 0; idx < allChunks.size(); ++idx)
        {
            QJsonValue chunkId = allChunks.at(idx);
            manifest.m_chunkIds.push_back(chunkId.toInt());
        }

        QJsonObject allFiles = rootObj["AllFiles"].toObject();
        QStringList fileKeys = allFiles.keys();
        for (int idx = 0; idx < fileKeys.count(); ++idx)
        {
            QString fileKey = fileKeys[idx];
            int chunkId = allFiles[fileKey].toInt();

            //FB_INFO_FORMAT("Found file: %s for chunk %d.", fileKey.toLatin1().constData(), chunkId);
            manifest.m_chunks[chunkId].push_back(fileKey.toLatin1().constData());
        }

        QJsonArray groupKeyArr = rootObj["GroupKeys"].toArray();
        for (int idx = 0; idx < groupKeyArr.count(); ++idx)
        {
            QString groupKey = groupKeyArr.at(idx).toString();

            eastl::string stlGroupKey = groupKey.toLatin1().constData();

            if (rootObj.contains(groupKey))
            {
                QJsonArray chunkList = rootObj[groupKey].toArray();
                FB_INFO_FORMAT("StreamInstallCommon::parseManifest - Found group key: %s with %d chunks.", groupKey.toLatin1().constData(), chunkList.size());
                for (int chunkIdx = 0; chunkIdx < chunkList.size(); ++chunkIdx)
                {
                    int chunkId = chunkList.at(chunkIdx).toInt();

                    manifest.m_groups[stlGroupKey].push_back(chunkId);
                }
            }
            else
            {
                FB_WARNING_FORMAT("StreamInstallCommon::parseManifest - Found group key %s but no matching values.", groupKey.toLatin1().constData());
            }
        }

        FB_INFO_FORMAT("StreamInstallCommon::parseManifest - Found %d non-required chunks. (%d required)", (int)manifest.m_chunkIds.size(), (int)manifest.m_intialChunkCount);
    }

    void StreamInstallCommon::setupData(ManifestContent& manifest)
    {

    }

    void StreamInstallCommon::cleanupData()
    {

    }

    void StreamInstallCommon::chunkInstalled(uint chunkId)
    {

    }

    void StreamInstallCommon::groupInstalled(const char * groupName)
    {

    }

    void StreamInstallCommon::initManifestPath()
    {
        QString appDir = QApplication::applicationDirPath();
        QString chunkManifestPath = QString("%1/%2").arg(appDir).arg("chunkmanifest.json");

        _manifestPath = chunkManifestPath;
    }
}//fb