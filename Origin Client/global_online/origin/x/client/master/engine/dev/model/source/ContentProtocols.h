///////////////////////////////////////////////////////////////////////////////
// ContentProtocols.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef CONTENTPROTOCOLS_H
#define CONTENTPROTOCOLS_H

#include "services/downloader/Common.h"

#include "services/downloader/DownloaderErrors.h"
#include "services/session/AbstractSession.h"
#include "services/platform/PlatformService.h"
#include "services/plugin/PluginAPI.h"

#include <QObject>
#include <QList>
#include <QPair>
#include <QThread>

class QUrl;

namespace Origin
{
namespace Downloader
{
    class IDownloadSession;
    class IContentInstallFlow;

    /// \brief Enumeration of the states that an IContentProtocol goes through.
	enum ContentProtocolState
	{
		kContentProtocolUnknown = 0, ///< The protocol has not been initialized.
        kContentProtocolInitializing, ///< The protocol is initializing.
		kContentProtocolInitialized, ///< The protocol has been successfully initialized.
		kContentProtocolStarting, ///< The protocol is running but has not begun.
		kContentProtocolResuming, ///< The protocol is resuming from the paused state.
		kContentProtocolRunning, ///< The protocol is running.
		kContentProtocolPausing, ///< The protocol is in the process of pausing.
		kContentProtocolPaused, ///< The protocol has been paused.
		kContentProtocolCanceling, ///< The protocol is in the process of canceling.
		kContentProtocolCanceled, ///< The protocol has been canceled.
		kContentProtocolFinished, ///< The protocol has finished successfully.
		kContentProtocolError ///< The protocol has encountered a critical error.
	};

    /// \brief Manages the download sessions.
    class ORIGIN_PLUGIN_API IContentProtocol : public QObject
    {
        Q_OBJECT
            signals:
                void Initialized(); ///< Emitted when the protocol has been successfully initialized.
                void Started(); ///< Emitted when the protocol has been started.
                void Resumed(); ///< Emitted when the protocol has been resumed from a paused state.
                void Paused(); ///< Emitted when the protocol has been paused.
                void Canceled(); ///< Emitted when the protocol has been canceled.
                void Finished(); ///< Emitted when the protocol has finished.
				void Finalized(); ///< Emitted when the protocol has completed all post-transfer tasks.

                /// \brief Emitted when the requested source has been found (disc change during ITO, etc).
                /// \param discEjected Specify whether the disc was ejected in the middle of an install.
                void SourceFound(bool discEjected = false);

                /// \brief Emitted when the protocol encounters a critical error.
                /// \param errorInfo A Downloader::DownloadErrorInfo object that contains information about the error.
                void IContentProtocol_Error(Origin::Downloader::DownloadErrorInfo errorInfo);

                /// \brief Emitted when the requested source could not be found (disc change during ITO, etc).
                /// \param currentDisc The disc needed to resume the ITO installation.
                /// \param numDiscs The total number of discs in the ITO installation.
                /// \param wrongDisc The disc that was inserted (-1 if no disc is inserted).
                /// \param discEjected The current disc was ejected in the middle of an install
				void CannotFindSource(int currentDisc, int numDiscs, int wrongDisc, bool discEjected = false);

                /// \brief Emitted when the download has made progress.
                /// \param bytesDownloaded The number of bytes that have been downloaded.
                /// \param totalBytes The total number of bytes.
                /// \param bytesPerSecond The average number of bytes downloaded per second.
                /// \param estimatedSecondsRemaining The estimated time remaining in seconds.
                void TransferProgress(qint64 bytesDownloaded , qint64 totalBytes, qint64 bytesPerSecond, qint64 estimatedSecondsRemaining);

                /// \brief Emitted when the CRC calcluation has made progress.
                /// \param bytesProcessed The number of bytes that have been processed.
                /// \param totalBytes The total number of bytes.
                /// \param bytesPerSecond The average number of bytes downloaded per second.
                /// \param estimatedSecondsRemaining The estimated time remaining in seconds.
				void NonTransferProgress(qint64 bytesProcessed, qint64 totalBytes, qint64 bytesPerSecond, qint64 estimatedSecondsRemaining);

                /// \brief Emitted when the content length has been successfully retrieved from the host.
                /// \param totalBytesOnDisk is the total number of uncompressed bytes saved to disk for this content.
                /// \param totalBytesToDownload is the total compressed bytes of this content needed to be downloaded
                void ContentLength ( qint64 totalBytesOnDisk, qint64 totalBytesToDownload );

            public:
                /// \brief The IContentProtocol constructor.
                /// \param productId The product ID of the content being downloaded.
                /// \param session Weak reference to the user's session.
                IContentProtocol(const QString& productId, Services::Session::SessionWRef session);

                /// \brief Initializes the protocol.
                /// \param url The url to retrieve files from.
                /// \param target The directory to download files to.
                /// \param isPhysicalMedia True if this is an ITO install.
                Q_INVOKABLE virtual void Initialize( const QString& url , const QString& target, bool isPhysicalMedia = false) = 0;

                /// \brief Starts the protocol.
                Q_INVOKABLE virtual void Start() = 0;

                /// \brief Resumes the protocol.
                Q_INVOKABLE virtual void Resume() = 0;
                
                /// \brief Pauses the protocol.
                /// \param forcePause True if the protocol should stop all synchronous requests, otherwise it will pause after the current operation.
                Q_INVOKABLE virtual void Pause( bool forcePause = false ) = 0;

                /// \brief Cancels the protocol.
                Q_INVOKABLE virtual void Cancel() = 0;

                /// \brief Retries the download.
				Q_INVOKABLE virtual void Retry() = 0;
                
                /// \brief Requests that the protocol retrieve the content length from the host.
                Q_INVOKABLE virtual void GetContentLength() = 0;

                /// \brief Requests that the protocol complete any post-transfer tasks (destaging, etc).
				Q_INVOKABLE virtual void Finalize() = 0;
                
                /// \brief Converts a ContentProtocolState to a human-readable string for logging.
                /// \param state The ContentProtocolState to convert.
                /// \return A string representation of the given state.
                static const QString& protocolStateString(ContentProtocolState state);

                /// \brief Set mGameTitle
                void setGameTitle(const QString &gameTitle);

                /// \brief Gets the game title
                /// \return The game title
                QString getGameTitle() const;

                /// \brief Tells the protocol to report debug information for various debugging features.
                /// \param reportDebugInfo If true, the protocol will report debug info.  If false, the protocol will respect the DownloadDebugEnabled override.
                void setReportDebugInfo(bool reportDebugInfo);
                
                /// \brief Gets the protocol's state.
                /// \return The protocol's state.
                ContentProtocolState protocolState();

                /// \brief Sets the job UUID that this protocol is currently associated with.
                /// \param uuid A UUID that uniquely identifies the download job.
                void setJobId(const QString& uuid);

                /// \brief Gets the job UUID that this protocol is currently associated with.
                /// \return The protocol's associated job UUID.
                QString getJobId();

            public:
                /// \brief The IContentProtocol destructor.
                virtual ~IContentProtocol() {}

            protected:
                Services::Session::SessionWRef mSession; ///< Weak reference to the user's session.
                QString mGameTitle; ///< Game Title derived from Content ID
                QString mProductId; ///< Product ID
                qint64 mTimeOfLastTransferProgress; ///< Keep time of last transfer progress to limit how often they are sent
                bool mReportDebugInfo;

                /// \brief Sets the protocol's state.
                /// \param newState The new state.
                void setProtocolState(ContentProtocolState newState);
                
                /// \brief Returns true if it's time to send another transfer progress update.
                /// \return true if it's time to sent another transfer progress update.
                bool checkToSendThrottledUpdate();

            private:
                ContentProtocolState mProtocolState; ///< Current state of the protocol.
                QString mJobId; ///< Current job UUID.
    };

    /// \brief Moves install flows and protocols to separate threads.
    class ORIGIN_PLUGIN_API ContentProtocols : public QObject
    {
        Q_OBJECT
            public:
                /// \brief Moves the given protocol to the protocol thread.
                /// \param protocol Protocol to move to the protocol thread.
                static void RegisterProtocol(IContentProtocol *protocol);
                static void RegisterWithProtocolThread(QObject *obj);

                /// \brief Moves the given install flow to the install flow thread.
                /// \param flow The install flow to move to the install flow thread.
                static void RegisterWithInstallFlowThread(QObject *flow);

            private:
                /// \brief The ContentProtocols constructor.
                ContentProtocols();

                /// \brief Gets the singleton ContentProtocols instance.
                static ContentProtocols* GetInstance();

                QThread _protocolThread; ///< The protocol thread.
                QThread _flowThread; ///< The install flow thread.
    };

	class PackageFile;

    // Utility typedefs (NOTE: These are here so that we can minimize cross-compilation unit includes just for typedef definitions)
    typedef qint64 DiffFileId;
    typedef QPair<DiffFileId, ContentDownloadError::type> DiffFileError;
    typedef QList<DiffFileError> DiffFileErrorList;

    int DownloadErrorFromEscalationError(int escalationError, int commandFailureError = 0);

    /// \brief Utility class that handles common protocol operations (preallocation, directory creation, etc).
	class ORIGIN_PLUGIN_API ContentProtocolUtilities : public QObject
	{
		Q_OBJECT
			public:
                /// \brief Creates a directory that all users can access.
                /// \param sDir The directory to create.
                /// \param escalationErrorOut Pointer to an int that will contain the error code returned by the Escalation Client, if any.
                /// \return The error code if one was encountered, otherwise ProtocolError::OK.
				static ContentDownloadError::type CreateDirectoryAllAccess(const QString& sDir, int* escalationErrorOut = NULL, int* escalationSysErrorOut = NULL);

                /// \brief Preallocates all files and directories in the given Downloader::PackageFile.
                /// \param packageFile The Downloader::PackageFile containing the files and directories to preallocate.
                /// \return True if all files and directories were preallocated successfully.
				static bool PreallocateFiles(const PackageFile& packageFile);

                /// \brief Reads the central directory of a zip file and populates packageFile with its contents.
                /// \param downloadSession The download session containing the zip data.
                /// \param contentLength The length of the content in bytes.
                /// \param gameTitle TBD.
                /// \param productId TBD.
                /// \param packageFile This function will populate this value with the contents of the central directory.
                /// \param numDiscs This function will populate this value with the number of discs.
                /// \param errInfo This function will populate this structure with error information if it fails.
                /// \param syncRequestCount TBD.
                /// \param waitCondition TBD.
                /// \return True if the cenrtal directory was read successfully.
                static bool ReadZipCentralDirectory(IDownloadSession *downloadSession, qint64 contentLength, const QString& gameTitle, const QString& productId, PackageFile &packageFile, int &numDiscs, DownloadErrorInfo& errInfo, QAtomicInt *syncRequestCount, QWaitCondition *waitCondition);

                /// \brief Verifies that the given packageFile is valid.
                /// \param unpackPath The path that the files will be unpacked to.
                /// \param contentLength The length of the content in bytes.
                /// \param gameTitle TBD.
                /// \param productId TBD.
                /// \param packageFile The package file to test.
                /// \return True if the given package file is valid.
                static bool VerifyPackageFile(QString unpackPath, qint64 contentLength, const QString& gameTitle, const QString& productId, PackageFile &packageFile);
	};

} // namespace Downloader
} // namespace Origin

#endif // CONTENTPROTOCOLS_H
