// DcmtFileDownloader.h
// Copyright (c) 2014, Electronic Arts Inc. All rights reserved.

#ifndef DCMT_FILE_DOWNLOADER_H
#define DCMT_FILE_DOWNLOADER_H

#include "services/settings/Setting.h"
#include "services/downloader/DownloaderErrors.h"
#include "services/connection/ConnectionStates.h"
#include "services/plugin/PluginAPI.h"
#include "ContentProtocolSingleFile.h"

#include "DcmtDownloadUrlResponse.h"

namespace Origin
{

namespace Downloader
{
class IContentProtocol;
}

namespace Plugins
{

namespace ShiftDirectDownload
{

/// \brief Class to download individual files from DCMT/Shift.
class DcmtFileDownloader : public QObject
{
    Q_OBJECT

public:
    /// \brief The DcmtFileDownloader constructor.
    DcmtFileDownloader(const QString &buildId, const QString &filename);

    /// \brief The DcmtFileDownloader destructor; releases the resources of an instance of the DcmtFileDownloader class.
    virtual ~DcmtFileDownloader();

    /// \brief Cancels a content install flow.
    Q_INVOKABLE virtual void cancel();
    
	Q_PROPERTY(QString fileName READ fileName)
	QString fileName() const;
    
	Q_PROPERTY(QString status READ status)
    QString status() const { return mStatus; }

signals:
    /// \brief Emitted when the download URL response is waiting on Shift to push the build to an edge node.
    /// \param buildId The build ID associated with this download.
    void deliveryRequested(const QString& buildId);

    /// \brief Emitted when the content protocol has started processing a transfer.
    /// \param buildId The build ID associated with this download.
    void started(const QString& buildId);

    /// \brief Emitted when the content protocol has finished processing a transfer.
    /// \param buildId The build ID associated with this download.
    void finished(const QString& buildId);

    /// \brief Emitted when the user cancels a download.
    /// \param buildId The build ID associated with this download.
    void canceled(const QString& buildId);

    /// \brief Emitted when an error occurs
    /// \param buildId The build ID associated with this download.
    void error(const QString& buildId);
    
    /// \brief Emitted when the download has made progress.
    /// \param bytesDownloaded The number of bytes that have been downloaded.
    /// \param totalBytes The total number of bytes.
    /// \param bytesPerSecond The average number of bytes downloaded per second.
    /// \param estimatedSecondsRemaining The estimated time remaining in seconds.
    void downloadProgress(qint64 bytesDownloaded, qint64 totalBytes, qint64 bytesPerSecond, qint64 estimatedSecondsRemaining);

private slots:
    void onProtocolInitialized();
    void onProtocolStarted();
    void onProtocolFinished();
    void onProtocolPaused();
    void onProtocolCanceled();
    void onProtocolError(Origin::Downloader::DownloadErrorInfo errorInfo);

    void onUrlReady();
    void onDeliveryRequested();
    void begin(const QString &url);
    void cancelInternal();

private:
    void deleteDownloadUrl();
    void deleteDownloadProtocol();

private:
    QString mStatus;
    DcmtDownloadUrlResponse *mDownloadUrl;
    QString mBuildId;
    QString mLocalFileName;
    Downloader::IContentProtocol* mDownloadProtocol;
};


} // namespace ShiftDirectDownload

} // namespace Plugins

} // namespace Origin

#endif // DCMT_FILE_DOWNLOADER_H
