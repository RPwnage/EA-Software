/////////////////////////////////////////////////////////////////////////////
// DownloadDebugDataCollector.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "engine/debug/DownloadDebugDataCollector.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/session/SessionService.h"
#include "services/Settings/SettingsManager.h"
#include "services/platform/PlatformService.h"

namespace Origin
{
namespace Engine
{
namespace Debug
{
DownloadDebugDataCollector::DownloadDebugDataCollector(QObject *parent /*= NULL*/)
: QObject(parent)
{
}

DownloadDebugDataCollector::~DownloadDebugDataCollector()
{
    QWriteLocker lock(&mLock);
    mTrackedFiles.clear();
}

void DownloadDebugDataCollector::updateDownloadFile(const DownloadFileMetadata& fileData)
{
    QMap<QString, DownloadFileMetadata> map;
    map.insert(fileData.fileName(), fileData);
    updateDownloadFiles(map);
}

void DownloadDebugDataCollector::updateDownloadFiles(const QMap<QString, DownloadFileMetadata>& fileDataMap)
{
    mLock.lockForWrite();
    for(QMap<QString, DownloadFileMetadata>::const_iterator it = fileDataMap.constBegin(); it != fileDataMap.constEnd(); ++it)
    {
        DownloadFileMetadata& metadata = mTrackedFiles[it.value().fileName()];
        metadata = it.value();
    }
    mLock.unlock();

    emit downloadFileProgress(fileDataMap.keys());
}

void DownloadDebugDataCollector::removeDownloadFile(const QString& fileName)
{
    QWriteLocker lock(&mLock);
    mTrackedFiles.remove(fileName);
}

DownloadFileMetadata DownloadDebugDataCollector::getDownloadFile(const QString& fileName)
{
    QReadLocker lock(&mLock);
    return mTrackedFiles[fileName];
}

QMap<QString, DownloadFileMetadata> DownloadDebugDataCollector::getDownloadFiles()
{
    QReadLocker lock(&mLock);
    return mTrackedFiles;
}

void DownloadDebugDataCollector::exportProgress(const QString& path)
{
    QFile file(path);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        ORIGIN_LOG_ERROR << "Error exporting download debug data to: " << path << ". Error: " << file.error();
    }

    QTextStream stream(&file);

    QStringList headers;
    headers.append(tr("ebisu_client_notranslate_file_name"));
    headers.append(tr("ebisu_client_notranslate_bytes_saved"));
    headers.append(tr("ebisu_client_notranslate_file_size"));
    headers.append(tr("ebisu_client_notranslate_install_location"));
    headers.append(tr("ebisu_client_notranslate_status"));
    headers.append(tr("ebisu_client_notranslate_error"));
    stream << headers.join(",") << endl;

    mLock.lockForRead();
    QStringList fields;
    for(QMap<QString, DownloadFileMetadata>::const_iterator it = mTrackedFiles.constBegin(); it != mTrackedFiles.constEnd(); ++it)
    {
        fields.clear();
        fields.append(it.value().strippedFileName());
        fields.append(QString::number(it.value().bytesSaved()));
        fields.append(QString::number(it.value().totalBytes()));
        fields.append(it.value().fileName());
        fields.append(fileStatusAsString(it.value().fileStatus()));
        fields.append(QString::number(it.value().errorCode()));
        stream << fields.join(",") << endl;
    }
    mLock.unlock();

    file.close();
}

void DownloadDebugDataCollector::exportUpdateDiff(const QString& path)
{
    QFile file(path);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        ORIGIN_LOG_ERROR << "Error exporting update debug data to: " << path << ". Error: " << file.error();
    }

    QTextStream stream(&file);

    QStringList headers;
    headers.append(tr("ebisu_client_notranslate_file_name"));
    headers.append(tr("ebisu_client_notranslate_file_size"));
    headers.append(tr("ebisu_client_notranslate_install_location"));
    headers.append(tr("ebisu_client_notranslate_update_crc"));
    headers.append(tr("ebisu_client_notranslate_local_crc"));
    stream << headers.join(",") << endl;

    mLock.lockForRead();
    QStringList fields;
    for(QMap<QString, DownloadFileMetadata>::const_iterator it = mTrackedFiles.constBegin(); it != mTrackedFiles.constEnd(); ++it)
    {
        fields.clear();
        fields.append(it.value().strippedFileName());
        fields.append(QString::number(it.value().totalBytes()));
        fields.append(it.value().fileName());
        fields.append(QString("0x%1").arg(it.value().diskFileCrc(), 8, 16));
        fields.append(QString("0x%1").arg(it.value().packageFileCrc(), 8, 16));
        stream << fields.join(",") << endl;
    }
    mLock.unlock();

    file.close();
}

} // Debug
} // Engine
} // Origin
