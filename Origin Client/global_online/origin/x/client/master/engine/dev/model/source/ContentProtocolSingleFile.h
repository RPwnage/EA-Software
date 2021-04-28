///////////////////////////////////////////////////////////////////////////////
// ContentProtocolSingleFile.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef CONTENTPROTOCOLSINGLEFILE_H
#define CONTENTPROTOCOLSINGLEFILE_H

#include "ContentProtocols.h"

#include <QTimer>
#include <QUrl>
#include <QFile>
#include <QMap>
#include <QQueue>
#include <QWaitCondition>
#include <QMutex>

#include "services/downloader/DownloadService.h"
#include "services/downloader/UnpackService.h"

#include "services/downloader/TransferStats.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Downloader
{
    class IDownloadSession;
    class IDownloadRequest;
    class SingleFileChunkMap;

    /// \brief The status of a download request.
    typedef struct
    {
        quint64 mRequestSize; ///< The size of the request.
        quint64 mRequestCurrentOffset; ///< The current offset of the request.
        quint8  mRetryCount; ///< The number of retries that have been attempted.
    } RequestStatus;

    /// \brief Manages file writing on a separate thread.
    class ORIGIN_PLUGIN_API WriteFileThread : public QThread
    {
        Q_OBJECT
    public:
        /// \brief The WriteFileThread constructor.
        /// \param file A QFile object representing the file to write to.
        /// \param session A Downloader::IDownloadSession object containing the data to write.
        WriteFileThread(QFile& file, IDownloadSession* session);

        /// \brief Queues up data to be written.
        /// \param request The unique ID of the download request.
        /// \param reqInfo Downloader::RequestStatus object containing file data informaton such as offsets.
        void submitRequest(RequestId request, RequestStatus* reqInfo);

        /// \brief Shuts down the file writing thread.
        void shutdown();

signals:
        /// \brief Emitted when the file write thread has completed.
        /// \param request The unique ID of the download request.
        void requestWriteComplete(RequestId request);

        /// \brief Emitted when the file write thread has encountered an error.
        /// \param request The unique ID of the download request.
        /// \param error TBD.
        void requestWriteError(RequestId request, quint8 error);

    private:
        void run(); ///< Begins writing to the file.

        QFile& mFileIO; ///< The file to write to.
        IDownloadSession *mDownloadSession; ///< The Downloader::IDownloadSession object containing the data to write.

        /// \brief Checks the queue for unprocessed items.
        /// \param nextReq This function will populate this parameter with the next unprocessed item's request ID, if one exists.
        /// \param info This function will populate this parameter with the next unprocessed item's request status, if one exists.
        /// \return True if nextReq and info were populated.
        bool processQueue(RequestId* nextReq, RequestStatus** info);

        /// \brief Writes the given data to disk.
        /// \param req The unique ID of the download request.
        /// \param reqStatus Downloader::RequestStatus object containing file data informaton such as offsets.
        void writeRequest(RequestId req, RequestStatus* reqStatus);
        
        bool mExecuting; ///< True if the thread is currently writing to file.
        QWaitCondition mWaitCondition; ///< QWaitCondition object responsible for waking other operations once the thread has finished writing.
        QMutex mQueueLock; ///< Mutex to prevent concurrent access of the queue.
        QQueue<QPair<RequestId, RequestStatus*> > mSubmittedRequests; ///< Queue of submitted requests indexed by request ID.
    };
    
    /// \brief Manages the download sessions for a single file (DiP manifest, EULA, etc).
    ///
    /// Single-file implementation, this interface is not public.  Used only by IContentInstallFlow.
    class ORIGIN_PLUGIN_API ContentProtocolSingleFile : public IContentProtocol
    {
        Q_OBJECT
        public:
            /// \brief The ContentProtocolSingleFile constructor.
            /// \param productId The product ID associated with this content.
            // \param telemetryId The telemetry ID used to track telemetry for this download
            /// \param session A weak reference to the user's session.
            ContentProtocolSingleFile(const QString& productId, Services::Session::SessionWRef session);
            
            /// \brief The ContentProtocolSingleFile destructor.
            ~ContentProtocolSingleFile();

        // IContentProtocol members
        public:
            /// \brief Initializes the protocol.
            /// \param url The url to retrieve files from.
            /// \param target The location to download the file to.
            /// \param isPhysicalMedia True if this is an ITO install.
            Q_INVOKABLE virtual void Initialize( const QString& url , const QString& target, bool isPhysicalMedia = false);
            
            /// \brief Starts the protocol.
            Q_INVOKABLE virtual void Start();

            /// \brief Resumes the protocol.
            Q_INVOKABLE virtual void Resume();
                
            /// \brief Pauses the protocol.
            /// \param forcePause True if the protocol should stop all synchronous requests, otherwise it will pause after the current operation.
            Q_INVOKABLE virtual void Pause( bool forcePause = false );
                
            /// \brief Cancels the protocol.
            Q_INVOKABLE virtual void Cancel();
                
            /// \brief Retries the download.
            Q_INVOKABLE virtual void Retry();
                
            /// \brief Requests that the protocol retrieve the content length from the host.
            Q_INVOKABLE virtual void GetContentLength();
                
            /// \brief Requests that the protocol complete any post-transfer tasks (destaging, etc).
            Q_INVOKABLE virtual void Finalize();

        public:
            /// \brief Sets the target file to download to.
            void SetTargetFile(const QString& target);

        protected:
            void ClearRunState(); ///< Clears all data related to the current session.
            void ClearTransferState(); ///< Clears all data related to the current download session and stops all download activity.
            void SuspendAll(); ///< Shuts down the download session.
            void ResumeAll(); ///< Resumes the transfer.
            void CheckTransferState(); ///< Makes sure that if we need to put the brakes on everything, we wait for everything to complete and then clean up.
            void Cleanup(); ///< Cleans up.
            void InitializeStart(); ///< Creates the download session and initializes the protocol.

            /// \brief Finishes the initializing state.
            /// \param contentLength The length of the content in bytes.
            void InitializeEnd(qint64 contentLength);
            
            /// \brief Emits an error based on the given error code.
            /// \param errorCode The error encountered.
            void InitializeFail(qint32 errorCode);

            /// \brief Checks for completion.  Emits IContentProtocol::Finished if completed.
            void ScanForCompletion();
                
            void TransferIssueRequests(); ///< Issues download requests.
            void TransferStart();  ///< Completes pre-download tasks such as preallocation and directory creation.
            
            /// \brief Queues a chunk for processing.
            /// \param req The Downloader::IDownloadRequest to pull raw data from.
            void ProcessChunk(IDownloadRequest* req);

            /// \brief Sends the request back to the download service to attempt to download again
            /// \param reqId The request ID to be retried.
            bool RetryRequest(RequestId reqId);

            /// \brief Records and reports errors.
            /// \param errorInfo Downloader::DownloadErrorInfo object containing relevant information about the error.
            void reportError(Origin::Downloader::DownloadErrorInfo errorInfo);

        protected slots:
            /// \brief Slot that is triggered when the download session has been initialized.
            /// \param contentLength The length of the content in bytes.
            void onDownloadSessionInitialized(qint64 contentLength);
            
            /// \brief Slot that is triggered when the download session encounters an error.
            /// \param errorInfo Downloader::DownloadErrorInfo object containing relevant information about the error.
            void onDownloadSessionError(Origin::Downloader::DownloadErrorInfo errorInfo);
            
            /// \brief Slot that is triggered when a new chunk is available.
            /// \param req The unique ID of the download request.
            void onDownloadRequestChunkAvailable(RequestId req);
            
            /// \brief Slot that is triggered when the download request encounters an error.
            /// \param req The unique ID of the download request.
            /// \param errorType The general type of error encountered.
            /// \param errorCode The specific error code of the error encountered.
            void onDownloadRequestError(RequestId req, qint32 errorType, qint32 errorCode);

            /// \brief Slot that is triggered when the file writing thread has completed.
            /// \param reqId The unique ID of the download request.
            void onRequestWriteComplete(RequestId reqId);
            
            /// \brief Slot that is triggered when the file writing thread has encountered an error.
            /// \param reqId The unique ID of the download request.
            /// \param writeError The cause of the write error.
            void onRequestWriteError(RequestId reqId, quint8 writeError);

        private:
            void cleanupWriteThread(); ///< Destroys the WriteFileThread member object.

            QString mTargetFile; ///< Path to the file to write to.
            QString mUrl; ///< URL to retrieve files from.
            QFile   mFileIO; ///< The file to write to.
			bool    mPhysicalMedia; ///< True if this is an ITO install.

            // Run state
            bool mProtocolReady; ///< True if the protocol has been initialized.

            SingleFileChunkMap *mChunkMap; ///< Map that tracks each chunk of our single file.

            // Transfer state
            IDownloadSession *mDownloadSession; ///< Pointer to the Downloader::IDownloadSession that issues download requests.

            QMap<RequestId, RequestStatus> mRequestStatusMap; ///< Map of request status by request ID.

            WriteFileThread* mWriteFileThread; ///< Pointer to the Downloader::WriteFileThread object in charge of writing to files.
            TransferStats mTransferStats; ///< TransferStats object that keeps track of download rate, etc.
            bool mProtocolForcePaused; ///< True if the protocol has been suspended/force-paused.
    };

} // namespace Downloader
} // namespace Origin

#endif // CONTENTPROTOCOLSINGLEFILE_H
