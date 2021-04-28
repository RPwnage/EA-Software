// DcmtDownloadManager.h
// Copyright (c) 2014, Electronic Arts Inc. All rights reserved.

#ifndef DCMTDOWNLOADMANAGER_H
#define DCMTDOWNLOADMANAGER_H

#include "ShiftDirectDownload/DcmtFileDownloader.h"

#include <QMap>
#include <QObject>

namespace Origin
{

namespace Plugins
{

namespace ShiftDirectDownload
{

class DcmtFileDownloader;

/// \brief Class to manage DcmtFileDownloader objects.
class DcmtDownloadManager : public QObject
{
    Q_OBJECT
public:
    /// \brief Initializes the download manager.
    static void init() { sInstance = new DcmtDownloadManager(); }

    /// \brief De-initializes the download manager.
    static void destroy();

    /// \brief Returns a pointer to the download manager.
    static DcmtDownloadManager* instance();

    /// \brief Kicks off a download.
    /// \param buildId The build ID of the download.
    /// \param buildVersion The build version string, used for file name.
    void startDownload(const QString& buildId, const QString& buildVersion);

private slots:
    void removeDownload(const QString& buildId);

private:
    DcmtDownloadManager(QObject *parent = NULL);
    ~DcmtDownloadManager();
    Q_DISABLE_COPY(DcmtDownloadManager);

    /// \brief Hash of downloaders by build ID.
    typedef QHash<QString, DcmtFileDownloader*> DownloaderHash;
    DownloaderHash mDownloaders;
    
    static DcmtDownloadManager* sInstance;
};


} // namespace ShiftDirectDownload

} // namespace Plugins

} // namespace Origin

#endif // DCMTDOWNLOADMANAGER_H
