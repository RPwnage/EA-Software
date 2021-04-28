///////////////////////////////////////////////////////////////////////////////
// DownloadDebugDataManager.h
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _DOWNLOADDEBUGDATAMANAGER_H_
#define _DOWNLOADDEBUGDATAMANAGER_H_

#include <QObject>
#include <QMap>
#include <QReadWriteLock>

#include "engine/debug/DownloadDebugDataCollector.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Engine
{
namespace Debug
{

/// \brief Manages all DownloadDebugDataCollector objects for all downloads.
class ORIGIN_PLUGIN_API DownloadDebugDataManager : public QObject
{
	Q_OBJECT

public:
    /// \brief Creates the current DownloadDebugDataManager instance.
    static void init() { sInstance = new DownloadDebugDataManager(); }

    /// \brief Destroys the current DownloadDebugDataManager instance.
    static void destroy() { sInstance->deleteLater(); sInstance = NULL; }
            
    /// \brief Returns the current DownloadDebugDataManager instance.
    /// \return The current DownloadDebugDataManager instance.
    static DownloadDebugDataManager* instance();
    
    /// \brief Starts collecting data on a download.
    /// \param productId The productId that identifies the download.
    /// \return The newly created data collector object.
    DownloadDebugDataCollector* addDownload(const QString& productId);
    
    /// \brief Stops collecting data on a download.
    /// \param productId The productId that identifies the download.
    void removeDownload(const QString& productId);
    
    /// \brief Retrieves the data collector for the given download.
    /// \param productId The productId that identifies the download.
    /// \return The data collector object.
    DownloadDebugDataCollector* getDataCollector(const QString& productId);

signals:
    /// \brief Emitted when a new download has started being tracked.
    /// \param productId The productId that identifies the download.
    void downloadAdded(const QString& productId);
    
    /// \brief Emitted when a download has stopped being tracked.
    /// \param productId The productId that identifies the download.
    void downloadRemoved(const QString& productId);

private:
    /// \brief Constructor
    /// \brief parent The object's parent, if any.
    DownloadDebugDataManager(QObject *parent = NULL);

    /// \brief Destructor
    ~DownloadDebugDataManager();
    
    Q_DISABLE_COPY(DownloadDebugDataManager);
    
    static DownloadDebugDataManager* sInstance;

    QMap<QString, DownloadDebugDataCollector*> mDatabase;
    QReadWriteLock mLock;
};

} // namespace Debug
} // namespace Engine
} // namespace Origin

#endif
