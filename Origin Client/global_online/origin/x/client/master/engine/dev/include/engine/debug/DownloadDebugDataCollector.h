///////////////////////////////////////////////////////////////////////////////
// DownloadDebugDataCollector.h
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _DOWNLOADDEBUGDATACOLLECTOR_H_
#define _DOWNLOADDEBUGDATACOLLECTOR_H_

#include <QObject>
#include <QMap>
#include <QReadWriteLock>

#include "services/downloader/DownloaderErrors.h"
#include "services/plugin/PluginAPI.h"
#include "engine/debug/DownloadFileMetadata.h"

namespace Origin
{
namespace Engine
{
namespace Debug
{
/// \brief Collects data on individual files in a download and relays this data to the UI.
class ORIGIN_PLUGIN_API DownloadDebugDataCollector : public QObject
{
	Q_OBJECT

public:
    /// \brief Constructor
    /// \brief parent The object's parent, if any.
    DownloadDebugDataCollector(QObject *parent = NULL);

    /// \brief Destructor
    ~DownloadDebugDataCollector();

    /// \brief Updates data on a single download file.
    /// \param fileData The information used to update our tracker.
    void updateDownloadFile(const DownloadFileMetadata& fileData);
    
    /// \brief Updates data on a multiple download files.
    /// \param fileDataMap The map containing all information used to update our tracker.
    void updateDownloadFiles(const QMap<QString, DownloadFileMetadata>& fileDataMap);
    
    /// \brief Removes data for a single download file.
    /// \param fileName The name that identifies the file in the download.
    void removeDownloadFile(const QString& fileName);

    /// \brief Retrieves information about an individual file's download progress.
    /// \param fileName The name that identifies the file in the download.
    /// \return The data collected for the file.
    DownloadFileMetadata getDownloadFile(const QString& fileName);
    
    /// \brief Retrieves information about an all files' download progress.
    /// \return The data collected for all files.
    QMap<QString, DownloadFileMetadata> getDownloadFiles();

    /// \brief Exports in-memory progress information to disk.
    /// \param path Path to file to write.
    void exportProgress(const QString& path);

    /// \brief Exports update file diff to disk.
    /// \param path Path to file to write.
    void exportUpdateDiff(const QString& path);

signals:
    /// \brief Emitted when a tracked file has been updated.
    /// \param fileNames The list of file names that were updated.
    void downloadFileProgress(QList<QString> fileNames);

private:
    QMap<QString, DownloadFileMetadata> mTrackedFiles;
    QReadWriteLock mLock;
};

} // namespace Debug
} // namespace Engine
} // namespace Origin

#endif
