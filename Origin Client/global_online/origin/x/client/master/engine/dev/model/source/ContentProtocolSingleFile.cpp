
#include "ContentProtocolSingleFile.h"
#include "services/downloader/Common.h"
#include "engine/downloader/ContentServices.h"
#include "engine/debug/DownloadDebugDataManager.h"
#include "services/downloader/DownloadService.h"
#include "services/debug/DebugService.h"
#include "services/common/DiskUtil.h"
#include "SingleFileChunkMap.h"
#include "services/log/LogService.h"

#include "TelemetryAPIDLL.h"

#include <QDir>
#include <QFile>
#include <QDebug>
#include <QWaitCondition>
#include <QObject>

#include <set>

#include "ZipFileInfo.h"
#include "PackageFile.h"
#include "services/downloader/FilePath.h"
#include "services/platform/EnvUtils.h"
#include "services/downloader/StringHelpers.h"
#include "DateTime.h"
#include "services/downloader/DownloaderErrors.h"

#include "engine/content/ContentController.h"
#include "engine/utilities/ContentUtils.h"

//#define DEBUG_PROTOCOL 1

#ifdef DEBUG_PROTOCOL
#include <QTextStream>
static QMutex sDebugMutex;
static QFile sDebugFile("debugSingleFile.csv");
static QTextStream sDebugStream;
#endif

#define PKGLOG_PREFIX "[Game Title: " << getGameTitle() << "][ProductID:" << mProductId << "]"
#define WRITETHREAD_PREFIX "[" << mDownloadSession->id() << "] "

namespace Origin
{
namespace Downloader
{		
    static const quint8 MAX_RETRY_COUNT = 3;
    
    static const quint8 FileNotWriteable = 0;
    static const quint8 WriteIOFailure = 1;
    static const quint8 InvalidDownloadRequest = 2;
    static const quint8 WriteThreadShutdown = 3;

    WriteFileThread::WriteFileThread(QFile& file, IDownloadSession* session) 
        : QThread(), mFileIO(file), mDownloadSession(session), mExecuting(true)
    {
        setObjectName("Downloader::WriteFile Thread");
        moveToThread(this);
    }

    void WriteFileThread::submitRequest(RequestId request, RequestStatus* reqInfo)
    {
        QMutexLocker lock(&mQueueLock);
        mSubmittedRequests.enqueue(QPair<RequestId, RequestStatus*>(request, reqInfo));
        mWaitCondition.wakeAll();
    }

    void WriteFileThread::shutdown()
    {
        ORIGIN_ASSERT(this != QThread::currentThread());

        mExecuting = false;
        mWaitCondition.wakeAll();

        while(isRunning())
        {
            Origin::Services::PlatformService::sleep(10);
        }
    }

    void WriteFileThread::run()
    {
        RequestId req;
        RequestStatus *reqInfo;
        QMutex waitMutex;

        while(mExecuting)
        {
            while(mExecuting && processQueue(&req, &reqInfo))
            {
                writeRequest(req, reqInfo);
            }

            if(mExecuting)
            {
                waitMutex.lock();
                mWaitCondition.wait(&waitMutex, 500);	
                waitMutex.unlock();
            }
        }

        ORIGIN_LOG_EVENT << WRITETHREAD_PREFIX << "Write request queue processing complete, clearing remaining [" << mSubmittedRequests.size() << "] entries.";

        while(!mSubmittedRequests.empty())
        {
            emit(requestWriteError(mSubmittedRequests.head().first, WriteThreadShutdown));
            mSubmittedRequests.dequeue();
        }
    }

    void WriteFileThread::writeRequest(RequestId reqId, RequestStatus* reqStatus)
    {
        IDownloadRequest* req = mDownloadSession->getRequestById(reqId);
        if(req == NULL)
        {
            ORIGIN_LOG_ERROR << WRITETHREAD_PREFIX << "Write error, request no longer tracked by download session!";
            emit(requestWriteError(reqId, InvalidDownloadRequest));
            return;
        }

        // Get the actual data from the request
        MemBuffer *chunkData = req->getDataBuffer();
        char* pBuffer = (char*) chunkData->GetBufferPtr();
        qint64 dataLen = chunkData->TotalSize();
        qint64 dataWritten = 0;

        qint64 fileOffset = req->getStartOffset() + reqStatus->mRequestCurrentOffset;

        // Save data to disk
        if(mFileIO.isWritable())
        {
            if(mFileIO.pos() == fileOffset || mFileIO.seek(fileOffset))
            {
                dataWritten = mFileIO.write(pBuffer, dataLen);
                if(dataWritten == dataLen)
                {
                    reqStatus->mRequestCurrentOffset += dataWritten;

#ifdef DEBUG_PROTOCOL
                    sDebugMutex.lock();
                    sDebugStream << "Write," << reqId << "," << QString::number(fileOffset) << "," << QString::number((fileOffset+reqStatus->mRequestCurrentOffset)) << "\n";
                    sDebugMutex.unlock();
#endif
                }
                else
                {
                    ORIGIN_LOG_ERROR << WRITETHREAD_PREFIX << "Write error, dataWritten != dataLen [" << dataWritten << " != " << dataLen << "] Error: " << mFileIO.errorString();
                    emit(requestWriteError(reqId, WriteIOFailure));
                    return;
                }
            }
            else
            {
                ORIGIN_LOG_ERROR << WRITETHREAD_PREFIX << "Write error, failed to seek to necessary offset [" << fileOffset << "] Error: " << mFileIO.errorString();
                emit(requestWriteError(reqId, WriteIOFailure));
                return;
            }
        }
        else
        {
            ORIGIN_LOG_ERROR << WRITETHREAD_PREFIX << "Single file target is not writable, aborting request save to target.";
            emit(requestWriteError(reqId, FileNotWriteable));
            return;
        }

        ORIGIN_ASSERT(dataWritten == dataLen);
        emit(requestWriteComplete(reqId));
    }

    bool WriteFileThread::processQueue(RequestId* nextReq, RequestStatus** info)
    {
        QMutexLocker lock(&mQueueLock);
        if(mSubmittedRequests.isEmpty())
        {
            return false;
        }
        else
        {
            *nextReq = mSubmittedRequests.head().first;
            *info = mSubmittedRequests.head().second;
            mSubmittedRequests.dequeue();
            return true;
        }
    }

    ContentProtocolSingleFile::ContentProtocolSingleFile(const QString& productId, Services::Session::SessionWRef session) :
        IContentProtocol(productId, session),
		mPhysicalMedia(false),
        mProtocolReady(false),
		mChunkMap(NULL),
        mDownloadSession(NULL),
        mWriteFileThread(NULL),
        mTransferStats("Single File Protocol", productId),
        mProtocolForcePaused(false)
    {

    }

    ContentProtocolSingleFile::~ContentProtocolSingleFile()
    {
        ClearRunState();
    }

    void ContentProtocolSingleFile::SetTargetFile(const QString& target)
    {
        // check to make sure we aren't already transferring
        mTargetFile = target;
    }

    void ContentProtocolSingleFile::Initialize( const QString& url, const QString& target, bool isPhysicalMedia )
    {
        // Ensure this can only be called asynchronously
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(QString, url), Q_ARG(QString, target), Q_ARG(bool, isPhysicalMedia));

        mUrl = url;
        SetTargetFile(target);
		mPhysicalMedia = isPhysicalMedia;
		
        ORIGIN_LOG_DEBUG << PKGLOG_PREFIX << "ContentProtocolSingleFile::Initialize( " << url << " )";

        // Start the initialization
        InitializeStart();
    }

    void ContentProtocolSingleFile::Start()
    {
        // Ensure this can only be called asynchronously
        ASYNC_INVOKE_GUARD;

        ORIGIN_LOG_DEBUG << PKGLOG_PREFIX << "ContentProtocolSingleFile::Start()";

        if (!mProtocolReady)
        {
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Cannot start.  Protocol not ready.";
            
            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = ProtocolError::CommandInvalid;
            errorInfo.mErrorCode = 0;
            reportError(errorInfo);
            return;
        }

        // Mark ourselves as starting
        setProtocolState(kContentProtocolStarting);

        // Do the actual work
        TransferStart();
    }

    void ContentProtocolSingleFile::Resume()
    {
        ASYNC_INVOKE_GUARD;

        if (!mProtocolReady)
        {
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Can't resume.  Protocol not ready.";
            
            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = ProtocolError::CommandInvalid;
            errorInfo.mErrorCode = 0;
            reportError(errorInfo);
            return;
        }

        // Do the work
        ResumeAll();
    }

    void ContentProtocolSingleFile::Pause(bool forcePause)
    {
        // Make sure we only do this once, because the Pause will execute again 
        // on the protocol thread with the same forcePause parameter
        if(!mProtocolForcePaused)
        {
            mProtocolForcePaused = forcePause;

            if(mProtocolForcePaused && mDownloadSession != NULL)
            {
                mDownloadSession->cancelSyncRequests();
            }
        }

        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(bool, forcePause));

        // Make sure we're not in a busy state
        if (protocolState() == kContentProtocolPausing || protocolState() == kContentProtocolCanceling)
        {
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Can't cancel.  Waiting for protocol to stop.";
            
            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = ProtocolError::CommandInvalid;
            errorInfo.mErrorCode = 0;
            reportError(errorInfo);
            return;
        }

        if (protocolState() != kContentProtocolRunning)
        {
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Can't pause.  Protocol not running.";
            
            // We're not running, so just give up and say we're paused anyway
            setProtocolState(kContentProtocolPaused);
            emit (Paused());

            return;
        }

        // Update the state
        setProtocolState(kContentProtocolPausing);

        // Shut down the file IO thread
        cleanupWriteThread();

        SuspendAll();
    }

    void ContentProtocolSingleFile::Finalize()
    {
        ASYNC_INVOKE_GUARD;

        // Make sure we're ready to go
        if (!mProtocolReady)
        {
            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = ProtocolError::CommandInvalid;
            errorInfo.mErrorCode = 0;
            reportError(errorInfo);
            return;
        }

        // Clear all state
        ClearRunState();
        emit Finalized();
    }

    void ContentProtocolSingleFile::Cancel()
    {
        // Everything after this will happen on another thread (fixes EBIBUGS-29078 - crash where a qt timer is not stopped due to being on a different thread)
        ASYNC_INVOKE_GUARD;

        // Make sure we're not in a busy state
        if (protocolState() == kContentProtocolPausing || protocolState() == kContentProtocolCanceling)
        {
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Can't cancel.  Waiting for protocol to " << (protocolState() == kContentProtocolPausing ? "pause" : "cancel") << " from previous command.";

            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = ProtocolError::CommandInvalid;
            errorInfo.mErrorCode = 0;
            reportError(errorInfo);
            return;
        }

        // If we're running, first we pause
        if (protocolState() == kContentProtocolRunning)
        {
            // Update the state
            setProtocolState(kContentProtocolCanceling);

            // Shut down the file IO thread
            cleanupWriteThread();

            SuspendAll();
        }
        else
        {
            // We're not running, just kill it all
            Cleanup();
        }
    }

    void ContentProtocolSingleFile::Retry()
    {
        return;
    }

    void ContentProtocolSingleFile::GetContentLength()
    {
        // Ensure this can only be called asynchronously
        ASYNC_INVOKE_GUARD;

        ORIGIN_LOG_DEBUG << PKGLOG_PREFIX << "ContentProtocolSingleFile::GetContentLength()";

        if (mProtocolReady)
        {
            emit(ContentLength(mChunkMap->GetFilesize(), mChunkMap->GetFilesize()));
        }
        else
        {
            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = ProtocolError::CommandInvalid;
            errorInfo.mErrorCode = 0;
            reportError(errorInfo);
            return;
        }
    }

    void ContentProtocolSingleFile::cleanupWriteThread()
    {
        // close the write thread and the file
        if(mWriteFileThread != NULL)
        {
            ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Shutting down write thread.";
            mWriteFileThread->shutdown();
            ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Write thread shutdown complete.";
            mWriteFileThread->deleteLater();
            mWriteFileThread = NULL;
        }
    }

    void ContentProtocolSingleFile::onDownloadSessionInitialized(qint64 contentLength)
    {
        ORIGIN_LOG_DEBUG << PKGLOG_PREFIX << "content length: " << contentLength;

        InitializeEnd(contentLength);
    }

    void ContentProtocolSingleFile::onDownloadSessionError(Origin::Downloader::DownloadErrorInfo errorInfo)
    {
        bool emitError = false;

        if(!mProtocolForcePaused)
        {
            // Call the appropriate failure handler here depending on what we are doing
            if (protocolState() == kContentProtocolUnknown || protocolState() == kContentProtocolPaused)
            {
                setProtocolState(kContentProtocolUnknown);
                emitError = true;
            }
            else if(protocolState() == kContentProtocolRunning)
            {
                ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "DownloadSession error type: " << errorInfo.mErrorType << " error code: " << errorInfo.mErrorCode;

                // Halt everything
                Pause();
                emitError = true;
            }
        }

        if (emitError)
        {
            if (errorInfo.mErrorType > 0)
            {
                reportError(errorInfo);
            }
            else
            {
                // errorInfo.mErrorType is invalid, so pass up some valid info.
                CREATE_DOWNLOAD_ERROR_INFO(validErrorInfo);
                validErrorInfo.mErrorType = ProtocolError::DownloadSessionFailure;
                validErrorInfo.mErrorCode = errorInfo.mErrorCode;
                reportError(errorInfo);
            }
        }
    }

    void ContentProtocolSingleFile::onDownloadRequestChunkAvailable(RequestId reqId)
    {
        if (!mDownloadSession)
            return;

        // We have an early return here to cover the case where we need to shut everything down
        bool shutdown = (protocolState() == kContentProtocolPausing || protocolState() == kContentProtocolCanceling);

        if (shutdown)
        {
            // Get the download request metadata
            if(mRequestStatusMap.contains(reqId))
            {
                mRequestStatusMap.remove(reqId);
            }
    
            mDownloadSession->closeRequest(reqId);
        }
        else
        {
            IDownloadRequest *req = mDownloadSession->getRequestById(reqId);

            if (!req)
                return;

            // Read the actual chunk
            ProcessChunk(req);
        }

        ScanForCompletion();
        CheckTransferState();
    }

    void ContentProtocolSingleFile::onDownloadRequestError(RequestId reqId, qint32 errorType, qint32 errorCode)
    {
        if ((!mProtocolForcePaused && errorType != DownloadError::DownloadWorkerCancelled) || protocolState() == kContentProtocolRunning)
        {
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Request error for " << reqId << " Error type: " << errorType << " Error code: " << errorCode << " Message: " << ErrorTranslator::Translate((ContentDownloadError::type) errorType);
        }

        // Get the download request metadata
        if(mRequestStatusMap.contains(reqId))
        {
            mRequestStatusMap.remove(reqId);
        }
    
        if(mDownloadSession)
        {
            mDownloadSession->closeRequest(reqId);
        }

        ScanForCompletion();
        CheckTransferState();
    }

    void ContentProtocolSingleFile::ScanForCompletion()
    {
        if (protocolState() != kContentProtocolRunning)
        {
            return;
        }

        if(mRequestStatusMap.empty() && mChunkMap->CountUnmarkedChunks() == 0)
        {
            mTransferStats.onFinished();

            // close the write thread and the file
            cleanupWriteThread();

            ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Download complete. Closing file handle.";
            mFileIO.close();
            ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "File handle closed.";

            // Mark ourselves as finished
            setProtocolState(kContentProtocolFinished);
            emit (Finished());
        }
    }

    void ContentProtocolSingleFile::ClearRunState()
    {
        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Clearing all state.";

        // We're no longer ready!
        mProtocolReady = false;
        setProtocolState(kContentProtocolUnknown);

        // Clear the transfer state
        ClearTransferState();
        
        // Reset our download debug database
        if(mReportDebugInfo || Origin::Services::readSetting(Origin::Services::SETTING_DownloadDebugEnabled).toQVariant().toBool())
        {
            Engine::Debug::DownloadDebugDataManager::instance()->removeDownload(mProductId);
        }

        // Reset the counters and flags
        if(mChunkMap != NULL)
        {
            ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Deleting chunk map.";
            delete mChunkMap;
            mChunkMap = NULL;
        }
    }

    void ContentProtocolSingleFile::ClearTransferState()
    {
        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Clearing transfer state.";

        // Kill the transfer-related metadata maps
        mRequestStatusMap.clear();

		// Clean up the currently active writing thread
		cleanupWriteThread();

        // Kill the download session
        if (mDownloadSession)
        {
            DownloadService::DestroyDownloadSession(mDownloadSession);
        }

        mDownloadSession = NULL;

        if(mFileIO.isOpen())
        {
            ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Closing file handle.";
            mFileIO.close();
            ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "File handle closed.";
        }

#ifdef DEBUG_PROTOCOL
        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Closing debug file";
        sDebugStream.flush();
        sDebugFile.close();
#endif
        // If we're pausing, just emit that signal
        if (protocolState() == kContentProtocolPausing)
        {
            if(mChunkMap)
            {
                ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Saving chunk map.";
                mChunkMap->SaveSingleFileChunkMap();
            }

            setProtocolState(kContentProtocolPaused);
            emit (Paused());
        }
        else if (protocolState() == kContentProtocolCanceling)
        {
            // We're canceling, so hit the cleanup function
            Cleanup();
        }
        else if (protocolState() == kContentProtocolRunning)
        {
            // We're done transferring
            // TODO: Go to the state where we wait for finalizing (if its an update)
            ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Finished!";
            setProtocolState(kContentProtocolUnknown);
        }
        else
        {
            // We probably got here in some kind of error handler
            setProtocolState(kContentProtocolUnknown);
        }
    }

    void ContentProtocolSingleFile::SuspendAll()
    {
        if (mDownloadSession != NULL)
        {
            ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Stopping all transfers...";
            mDownloadSession->shutdown();
        }
    }

    void ContentProtocolSingleFile::ResumeAll()
    {
        setProtocolState(kContentProtocolResuming);

        // We are resuming from cold pause, so just transition to starting
        TransferStart();
    }

    void ContentProtocolSingleFile::CheckTransferState()
    {
        // The purpose of this function is to make sure that if we need to put the brakes on everything, that we wait for everything to complete and then clean up
        if (protocolState() == kContentProtocolRunning)
        {
            return;
        }

        if(!mRequestStatusMap.empty())
        {
            return;
        }

        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "All transfers stopped.  Clearing state.";

        // We don't have anything to do anymore
        ClearTransferState();
    }

    void ContentProtocolSingleFile::Cleanup()
    {
        // Kill the chunk map
        Origin::Util::DiskUtil::DeleteFileWithRetry(SingleFileChunkMap::GetChunkMapFilename(mTargetFile));
            
        // Kill the target file
        Origin::Util::DiskUtil::DeleteFileWithRetry(mTargetFile);

        // Clear our state
        ClearRunState();

        setProtocolState(kContentProtocolCanceled);
        emit (Canceled());
    }

    void ContentProtocolSingleFile::InitializeStart()
    {
        // Clear all state
        ClearRunState();

        // Create a download source
        mDownloadSession = DownloadService::CreateDownloadSession(mUrl, mProductId, mPhysicalMedia, mSession );

        ORIGIN_VERIFY_CONNECT_EX(mDownloadSession, SIGNAL(initialized(qint64)), this, SLOT(onDownloadSessionInitialized(qint64)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(mDownloadSession, SIGNAL(IDownloadSession_error(Origin::Downloader::DownloadErrorInfo)), this, SLOT(onDownloadSessionError(Origin::Downloader::DownloadErrorInfo)), Qt::QueuedConnection);

        ORIGIN_VERIFY_CONNECT_EX(mDownloadSession, SIGNAL(requestChunkAvailable(RequestId)), this, SLOT(onDownloadRequestChunkAvailable(RequestId)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(mDownloadSession, SIGNAL(requestError(RequestId, qint32, qint32)), this, SLOT(onDownloadRequestError(RequestId, qint32, qint32)), Qt::QueuedConnection);

        // Initialize the download source
        mDownloadSession->initialize();
    }

    void ContentProtocolSingleFile::InitializeEnd(qint64 contentLength)
    {
        // Mark ourselves ready
        setProtocolState(kContentProtocolInitialized);
        mChunkMap = new SingleFileChunkMap(SingleFileChunkMap::GetChunkMapFilename(mTargetFile), contentLength, mDownloadSession->getMinRequestSize(), mProductId);
        
        if(mChunkMap->Init())
        {
            mProtocolReady = true;
            mTransferStats.onStart(mChunkMap->GetFilesize(), mChunkMap->GetMarkedSize());
            emit(Initialized());
        }
        else
        {
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Failed to initialize Chunk Map";

            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = ProtocolError::ProtocolErrorStart;
            errorInfo.mErrorCode = 0;
            reportError(errorInfo);
        }
    }

    void ContentProtocolSingleFile::InitializeFail(qint32 errorCode)
    {
        setProtocolState(kContentProtocolUnknown);
        
        CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
        errorInfo.mErrorType = ProtocolError::DownloadSessionFailure;
        errorInfo.mErrorCode = errorCode;
        reportError(errorInfo);
    }

    void ContentProtocolSingleFile::TransferStart()
    {
        // Make sure the directory in mTargetFile exists, if not create it.
        QFileInfo targetFileInfo(mTargetFile);

        // Create the directories, etc.
        if (!targetFileInfo.dir().exists())
        {
            int escalationError = 0;
            int escalationSysError = 0;
            QString targetPath = targetFileInfo.dir().absolutePath();
            ContentDownloadError::type returnVal = ContentProtocolUtilities::CreateDirectoryAllAccess(targetPath, &escalationError, &escalationSysError);

            if (returnVal != ContentDownloadError::OK)
            {
                ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Unable to create directory [" << targetPath << "] due to escalation error: " << escalationError;

                ClearRunState();

                int sysError = escalationError;

                // For command failures, we should send the sys error code back instead since it tells us more
                if ((int)returnVal == (int)ProtocolError::ContentFolderError)
                    sysError = escalationSysError;
                
                CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
                errorInfo.mErrorType = returnVal;
                errorInfo.mErrorCode = sysError;
                errorInfo.mErrorContext = targetPath;
                reportError(errorInfo);
                return;
            }
        }

        mFileIO.setFileName(mTargetFile);

        if (mFileIO.exists())
        {
            if(!mFileIO.open(QIODevice::ReadWrite))
            {
                ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Failed to open file for download resume...";
                ClearRunState();
                
                CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
                errorInfo.mErrorType = DownloadError::FileOpen;
                errorInfo.mErrorCode = mFileIO.error();
                reportError(errorInfo);
                return;
            }

#ifdef DEBUG_PROTOCOL
            sDebugFile.open(QIODevice::WriteOnly);
#endif
        }
        else
        {

#ifdef DEBUG_PROTOCOL
            sDebugMutex.lock();
            sDebugFile.open(QIODevice::WriteOnly);
            sDebugStream.setDevice(&sDebugFile);
            sDebugMutex.unlock();
#endif

            if(!mFileIO.open(QIODevice::WriteOnly))
            {
                ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Failed to open file for download start...";
                ClearRunState();
                
                CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
                errorInfo.mErrorType = DownloadError::FileOpen;
                errorInfo.mErrorCode = mFileIO.error();
                reportError(errorInfo);
                return;
            }

            // If we read in a chunk map, it's no longer valid as the target file is no longer on disk.  Reset it.
            mChunkMap->ResetMarkedChunkMap();
        }

        mWriteFileThread = new WriteFileThread(mFileIO, mDownloadSession);
        ORIGIN_VERIFY_CONNECT_EX(mWriteFileThread, SIGNAL(requestWriteComplete(RequestId)), this, SLOT(onRequestWriteComplete(RequestId)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(mWriteFileThread, SIGNAL(requestWriteError(RequestId, quint8)), this, SLOT(onRequestWriteError(RequestId, quint8)), Qt::QueuedConnection);
        mWriteFileThread->start();
        Services::ThreadNameSetter::setThreadName(mWriteFileThread, "ContentProtocolSingleFile Write File Thread");

        if (protocolState() == kContentProtocolResuming)
        {
            // Mark ourselves as running
            setProtocolState(kContentProtocolRunning);

            emit Resumed();
        }
        else
        {
            // Mark ourselves as running
            setProtocolState(kContentProtocolRunning);

            emit Started();
        }
        
        // Create our debug table if download debugging is enabled.
        if(mReportDebugInfo || Origin::Services::readSetting(Origin::Services::SETTING_DownloadDebugEnabled).toQVariant().toBool())
        {
            Engine::Debug::DownloadFileMetadata file;
            file.setFileName(mTargetFile);
            file.setBytesSaved(mChunkMap->GetMarkedSize());
            file.setTotalBytes(mChunkMap->GetFilesize());
            file.setErrorCode(ProtocolError::OK);
            file.setIncluded(true);

            Engine::Debug::DownloadDebugDataCollector *collector = Engine::Debug::DownloadDebugDataManager::instance()->addDownload(mProductId);
            if(collector)
            {
                collector->updateDownloadFile(file);
            }
        }  

        // Send out the actual requests
        TransferIssueRequests();
    }

    void ContentProtocolSingleFile::TransferIssueRequests()
    {
        QVector<RequestId> requests;

        qint64 mNextChunkStartOffset = mChunkMap->FindNextUnmarkedByte(0);
        qint64 requestEndOffset = 0;
        qint64 requestStartOffset = 0;

        while(mNextChunkStartOffset >= 0)
        {
            requestStartOffset = mNextChunkStartOffset;
            requestEndOffset = requestStartOffset + mDownloadSession->getRequestSize();

            if(requestEndOffset > mChunkMap->GetFilesize())
            {
                requestEndOffset = mChunkMap->GetFilesize();
            }


            requests.push_back(mDownloadSession->createRequest(requestStartOffset, requestEndOffset));

#ifdef DEBUG_PROTOCOL
            sDebugMutex.lock();
            sDebugStream << "Request," << requests.back() << "," << QString::number(requestStartOffset) << "," << QString::number(requestEndOffset) << "\n";
            sDebugMutex.unlock();
#endif
            RequestStatus& status = mRequestStatusMap[requests.back()];
            status.mRequestSize = requestEndOffset - requestStartOffset;
            status.mRequestCurrentOffset = 0;
            status.mRetryCount = 0;
            mNextChunkStartOffset = mChunkMap->FindNextUnmarkedByte(requestEndOffset+1);
        }

        // Submit the request
        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Submitting total requests: " << requests.size();
        mDownloadSession->submitRequests(requests);
        
        ScanForCompletion();
        CheckTransferState();
    }

    void ContentProtocolSingleFile::ProcessChunk(IDownloadRequest* req)
    {
        // Make sure there is in fact still data available
        if (req->chunkIsAvailable())
        {
#ifdef DEBUG_PROTOCOL
            sDebugMutex.lock();
            sDebugStream << "Chunk," << req->getRequestId() << "," << req->getStartOffset() << "," << req->getEndOffset() << "\n";
            sDebugMutex.unlock();
#endif
            RequestStatus& reqStatus = mRequestStatusMap[req->getRequestId()];
            mWriteFileThread->submitRequest(req->getRequestId(), &reqStatus);
        }
    }

    void ContentProtocolSingleFile::reportError(Origin::Downloader::DownloadErrorInfo errorInfo)
    {
        // Update our debug table if download debugging is enabled.
        if(mReportDebugInfo || Origin::Services::readSetting(Origin::Services::SETTING_DownloadDebugEnabled).toQVariant().toBool())
        {
            Engine::Debug::DownloadFileMetadata file;
            file.setFileName(mTargetFile);
            if (mChunkMap)
            {
                file.setBytesSaved(mChunkMap->GetMarkedSize());
                file.setTotalBytes(mChunkMap->GetFilesize());
            }
            file.setErrorCode(errorInfo.mErrorType);
            file.setIncluded(true);
            
            Engine::Debug::DownloadDebugDataCollector *collector = Engine::Debug::DownloadDebugDataManager::instance()->getDataCollector(mProductId);
            if(collector)
            {
                collector->updateDownloadFile(file);
            }
        }

        emit (IContentProtocol_Error(errorInfo));
    }

    void ContentProtocolSingleFile::onRequestWriteError(RequestId reqId, quint8 writeError)
    {
        if(protocolState() != kContentProtocolPausing && protocolState() != kContentProtocolCanceling)
        {
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Write error for request: " << reqId;
        }

        // If we're no longer tracking this request, bail
        if(!mRequestStatusMap.contains(reqId))
        {
            return;
        }

        RequestStatus& reqStatus = mRequestStatusMap[reqId];

        if(protocolState() == kContentProtocolRunning && 
           mDownloadSession->getRequestById(reqId) != NULL && 
           reqStatus.mRetryCount < MAX_RETRY_COUNT)
        {
            reqStatus.mRetryCount++;
            mWriteFileThread->submitRequest(reqId, &reqStatus);
        }
        else
        {
            if(mDownloadSession)
            {
                mDownloadSession->closeRequest(reqId);
            }

            // Give up on this request
            mRequestStatusMap.remove(reqId);

            if(protocolState() != kContentProtocolPausing && protocolState() != kContentProtocolCanceling)
            {
                CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
                errorInfo.mErrorType = DownloadError::FileWrite;
                errorInfo.mErrorCode = writeError;
                reportError(errorInfo);

                if(protocolState() == kContentProtocolRunning)
                {
                    Pause();
                }
            }

            ScanForCompletion();
            CheckTransferState();
        }
    }

    bool ContentProtocolSingleFile::RetryRequest(RequestId reqId)
    {
        IDownloadRequest* req = mDownloadSession->getRequestById(reqId);

        // If we can't access the request info, we can't retry it.
        if(req == NULL)
        {
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Requested to retry request that is not recognized.";
            return false;
        }
        
        // Clean up the old request metadata
        if(mRequestStatusMap.contains(reqId))
        {
            mRequestStatusMap.remove(reqId);
        }

        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Redownloading single file chunk [" << req->getStartOffset() << " : " << req->getEndOffset() << "]";

        RequestId newRequest = mDownloadSession->createRequest(req->getStartOffset(), req->getEndOffset());

#ifdef DEBUG_PROTOCOL
        sDebugMutex.lock();
        sDebugStream << "Rerequest," << newRequest << "," << QString::number(req->getStartOffset()) << "," << QString::number(req->getEndOffset()) << "\n";
        sDebugMutex.unlock();
#endif

        RequestStatus& status = mRequestStatusMap[newRequest];
        status.mRequestSize = req->getTotalBytesRequested();
        status.mRequestCurrentOffset = 0;
        status.mRetryCount = 0;

        // TELEMETRY: Could not resume existing file, redownload.
        QString telemetryFileName = QString("%1[%2-%3]").arg(mTargetFile, QString::number(req->getStartOffset()), QString::number(req->getEndOffset()));

        GetTelemetryInterface()->Metric_DL_REDOWNLOAD(mProductId.toLocal8Bit().constData(), telemetryFileName.toLocal8Bit().constData(), "SINGLE_FILE_CHUNK_RETRY", 0, req->getStartOffset(), req->getEndOffset());

        // Close the old request and submit the new one for download
        mDownloadSession->closeRequest(reqId);
        mDownloadSession->submitRequest(newRequest);
        return true;
    }

    void ContentProtocolSingleFile::onRequestWriteComplete(RequestId reqId)
    {			
        // If we're no longer tracking this request, bail
        if(!mRequestStatusMap.contains(reqId))
        {
            return;
        }
        
        IDownloadRequest* req = mDownloadSession->getRequestById(reqId);

        // We have an early return here to cover the case where we need to shut everything down
        if (req == NULL || protocolState() == kContentProtocolPausing || protocolState() == kContentProtocolCanceling)
        {
            // Clear the request metadata
            mRequestStatusMap.remove(reqId);

            if(mDownloadSession)
                mDownloadSession->closeRequest(reqId);
        }
        else
        {
            RequestStatus& reqStatus = mRequestStatusMap[reqId];
            qint32 buffer_size = 0;
            
            if (req->getDataBuffer())
                buffer_size = req->getDataBuffer()->TotalSize();    // grab it here as chunkMarkAsRead() will free the buffer
            
            if (!req->isComplete())
            {
                // Mark the chunk as read so it can start download the next one
                req->chunkMarkAsRead();
            }

            if (req->isComplete())
            {	
                if(reqStatus.mRequestCurrentOffset != reqStatus.mRequestSize)
                {
                    ORIGIN_ASSERT(false);
                    ORIGIN_LOG_WARNING << PKGLOG_PREFIX << "Request claims it has completed but it was not successfully written to disk. [size: " << reqStatus.mRequestSize << ", offset: " << reqStatus.mRequestCurrentOffset << ", bytes: " << req->getTotalBytesRead() << "]";
                    
                    if(reqStatus.mRetryCount > MAX_RETRY_COUNT || !RetryRequest(reqId))
                    {
                        // Clear the request metadata
                        mRequestStatusMap.remove(reqId);

                        if(mDownloadSession)
                            mDownloadSession->closeRequest(reqId);

                        CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
                        errorInfo.mErrorType = DownloadError::FileWrite;
                        errorInfo.mErrorCode = 0;
                        reportError(errorInfo);
                    }
                }
                else
                {
                    // Update the transfer stats
                    mTransferStats.onDataReceived(buffer_size);

#ifdef DEBUG_PROTOCOL
                    sDebugMutex.lock();
                    sDebugStream << "Marking," << reqId << "," << QString::number(req->getStartOffset()) << "," << QString::number(req->getEndOffset()) << "\n";
                    sDebugMutex.unlock();
#endif
                    // Mark this chunk as complete within the chunk map
                    mChunkMap->MarkRange(req->getStartOffset(), req->getEndOffset());

                    // Clean up after ourselves
                    mRequestStatusMap.remove(reqId);
                    mDownloadSession->closeRequest(reqId);
                    
                    // Update our debug table if download debugging is enabled.
                    if(mReportDebugInfo || Origin::Services::readSetting(Origin::Services::SETTING_DownloadDebugEnabled).toQVariant().toBool())
                    {
                        Engine::Debug::DownloadFileMetadata file;
                        file.setFileName(mTargetFile);
                        file.setBytesSaved(mChunkMap->GetMarkedSize());
                        file.setTotalBytes(mChunkMap->GetFilesize());
                        file.setErrorCode(ProtocolError::OK);
                        file.setIncluded(true);

                        Engine::Debug::DownloadDebugDataCollector *collector = Engine::Debug::DownloadDebugDataManager::instance()->getDataCollector(mProductId);
                        if(collector)
                        {
                            collector->updateDownloadFile(file);
                        }
                    }


                    // Always send last update, otherwise obey throttling in base protocol
                    if(mChunkMap->CountUnmarkedChunks() == 0 || checkToSendThrottledUpdate())
                    {
                        emit (TransferProgress(mChunkMap->GetMarkedSize(), mChunkMap->GetFilesize(), (qint64)mTransferStats.bps(), mTransferStats.estimatedTimeToCompletion(true)));
                    }
                }
            }
        }

        ScanForCompletion();
        CheckTransferState();
    }
    
} // namespace Downloader
} // namespace Origin
