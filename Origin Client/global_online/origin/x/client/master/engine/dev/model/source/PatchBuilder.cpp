///////////////////////////////////////////////////////////////////////////////
// PatchBuilder.cpp
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////


#include "PatchBuilder.h"

#include "services/downloader/Common.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/downloader/MemBuffer.h"
#include "services/downloader/DownloaderErrors.h"
#include "services/common/DiskUtil.h"
#include "services/platform/EnvUtils.h"

#include "PackageFile.h"

#include <QMutex>
#include <QMutexLocker>
#include <QUrl>

#include <EAPatchClient/Base.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EAString.h>
#include <EAIO/PathString.h>
#include <EAIO/EAFileUtil.h>
#include <EAIO/EAFileDirectory.h>

#if !defined(ORIGIN_PC)
#include <unistd.h>
#include <sys/stat.h>
#endif

///////////////////////////////////////////////////////////////////////////
// log functionality
///////////////////////////////////////////////////////////////////////////

#ifndef PATCHBUILDER_DEVELOPER_DEBUG
    #if defined(_DEBUG) || defined(EA_DEBUG)
        #define PATCHBUILDER_DEVELOPER_DEBUG 2
    #else
        #define PATCHBUILDER_DEVELOPER_DEBUG 0
    #endif
#endif

#define PATCHBUILDER_MAX_OPEN_FILE_COUNT 64 // This is somewhat arbitrary.

#define PATCHBUILDER_LOG_ERROR(msg)             PatchBuilderLogEvent (Origin::Services::kLogError,   _logPrefix, __FILE__, __FUNCTION__, __LINE__, msg)
#define PATCHBUILDER_LOG_ERROR_F(format, ...)   PatchBuilderLogEventF(Origin::Services::kLogError,   _logPrefix, __FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)
#define PATCHBUILDER_LOG_WARNING(msg)           PatchBuilderLogEvent (Origin::Services::kLogWarning, _logPrefix, __FILE__, __FUNCTION__, __LINE__, msg)
#define PATCHBUILDER_LOG_WARNING_F(format, ...) PatchBuilderLogEventF(Origin::Services::kLogWarning, _logPrefix, __FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)
#define PATCHBUILDER_LOG_EVENT(msg)             PatchBuilderLogEvent (Origin::Services::kLogEvent,   _logPrefix, __FILE__, __FUNCTION__, __LINE__, msg)
#define PATCHBUILDER_LOG_EVENT_F(format, ...)   PatchBuilderLogEventF(Origin::Services::kLogEvent,   _logPrefix, __FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)


void PatchBuilderLogEvent(Origin::Services::LogMsgType logMsgType, const QString& prefix, const char* pFile, const char* pFunction, int nLine, const char* pMsg)
{
    Origin::Services::Logger::NewLogStatement(logMsgType, pFile, pFunction, nLine) << prefix;
    Origin::Services::Logger::NewLogStatement(logMsgType, pFile, pFunction, nLine) << pMsg;
}


void PatchBuilderLogEventF(Origin::Services::LogMsgType logMsgType, const QString& prefix, const char* pFile, const char* pFunction, int nLine, const char* pFormat, ...)
{
    Origin::Services::Logger::NewLogStatement(logMsgType, pFile, pFunction, nLine) << prefix;

    // Write the log entry.
    const int kBufferSize = 2048;
    char      buffer[kBufferSize];
    va_list   arguments;

    va_start(arguments, pFormat);

    if(EA::StdC::Vsnprintf(buffer, kBufferSize, pFormat, arguments) >= 0)
        Origin::Services::Logger::NewLogStatement(logMsgType, pFile, pFunction, nLine) << buffer;
    // else consider handling the case.

    va_end(arguments);
}


static void QStringToString(EA::Patch::String& str, const QString& qstr)
{
    EA::StdC::Strlcpy(str, reinterpret_cast<const char16_t*>(qstr.utf16()), qstr.length());
}

static void StringToQString(QString& qstr, const EA::Patch::String& str)
{
    qstr = QString::fromUtf8(str.data(), str.length()); // To consider: Make this more efficient.
}

// Replace any forward path separators with file system-specific path separators.
static void ForwardSlashesToFileSystemSlashes(EA::Patch::String& str)
{
    if(EA_FILE_PATH_SEPARATOR_8 != '/') // If the file system path separator isn't already a forward slash...
    {
        for(eastl_size_t i = 0, iEnd = str.size(); i != iEnd; ++i)
        {
            if(str[i] == '/')
                str[i] = EA_FILE_PATH_SEPARATOR_8;
        }
    }
}

static void EnsureTrailingPathSeparator(EA::Patch::String& str)
{
    if(str.empty() || !EA::IO::IsFilePathSeparator(str.back()))
        str += EA_FILE_PATH_SEPARATOR_8;
}



namespace Origin
{
namespace Downloader
{
    ///////////////////////////////////////////////////////////////////////////
    // PatchBuilderFileMetadata
    ///////////////////////////////////////////////////////////////////////////

    PatchBuilderFileMetadata::PatchBuilderFileMetadata() : 
        diffId(-1),
        originalFilename(),
        diskFilename(),
        fileCompressedSize(0),
        fileUncompressedSize(0),
        fileCRC(0)
    {
    }


    ///////////////////////////////////////////////////////////////////////////
    // PatchBuilderFactory
    ///////////////////////////////////////////////////////////////////////////

    static PatchBuilderFactory *g_instPatchBuilderFactory = NULL;
    static QMutex gPatchBuilderFactoryMutex; // Question: Is there a single project-wide mutex for this kind of thing, or does each subsystem needs its own like this?


    PatchBuilderFactory::PatchBuilderFactory()
    {
        qRegisterMetaType<PackageFileEntry*> ("PackageFileEntry*");
        qRegisterMetaType<PatchBuilderFileMetadataQList>("PatchBuilderFileMetadataQList");
        qRegisterMetaType<DiffFileId>("DiffFileId");
        qRegisterMetaType<DiffFileError>("DiffFileError");
        qRegisterMetaType<DiffFileErrorList>("DiffFileErrorList");
    }

    PatchBuilderFactory* PatchBuilderFactory::GetInstance()
    {
        QMutexLocker autoMutex(&gPatchBuilderFactoryMutex);

        if (NULL == g_instPatchBuilderFactory)
            g_instPatchBuilderFactory = new PatchBuilderFactory();

        return g_instPatchBuilderFactory;
    }

    PatchBuilder* PatchBuilderFactory::CreatePatchBuilder(QString rootPath, QString productId, QString gameTitle, Services::Session::SessionWRef curSession)
    {
        PatchBuilderFactory *instance = GetInstance();

        PatchBuilder *patchBuilder = new PatchBuilder(rootPath, productId, gameTitle, curSession);

        // Create and start a QThread for the new object
        QThread* newThread = new QThread();
        newThread->setObjectName("PatchBuilder Thread");
        instance->_patchBuilderThreads[patchBuilder] = newThread;
        newThread->start();

        // Move the new object to the thread
        patchBuilder->moveToThread(newThread);

        return patchBuilder;
    }

    void PatchBuilderFactory::DestroyPatchBuilder(PatchBuilder* patchBuilder)
    {
        PatchBuilderFactory *factory = GetInstance();

        // Signal the patch builder to shut down
        patchBuilder->_shutdown = true;

        // Hook up the deletion signal
        ORIGIN_VERIFY_CONNECT_EX(patchBuilder, SIGNAL(destroyed(QObject*)), factory, SLOT(onPatchBuilderInstanceDestroyed(QObject*)), Qt::QueuedConnection);

        // Tell it to destroy itself on its own event loop
        patchBuilder->deleteLater();
    }

    void PatchBuilderFactory::onPatchBuilderInstanceDestroyed(QObject* obj)
    {
        PatchBuilder *patchBuilder = static_cast<PatchBuilder*>(obj);

        ORIGIN_ASSERT(patchBuilder);

        if (patchBuilder)
        {
            // Retrieve the thread associated with this PatchBuilder isntance
            QThread* removeThread = _patchBuilderThreads[patchBuilder];

            // Tell the thread to exit the event loop and wait for it to finish
            removeThread->exit();
            removeThread->wait();

            // Destroy the thread
            _patchBuilderThreads.remove(patchBuilder);
            delete removeThread;
        }
        else
        {
            ORIGIN_LOG_WARNING << "Unable to clean up patch builder thread!";
        }
    }


    ///////////////////////////////////////////////////////////////////////////
    // PatchBuilder::DownloadRequestData
    ///////////////////////////////////////////////////////////////////////////

    PatchBuilder::DownloadRequestData::DownloadRequestData()
      : mpJob(NULL),
        mpBuiltFileSpan(),
        mRequestId(0),
        mDestFileStart(0),
        mServerFileStart(0),
        mServerFileEnd(0),
        mBytesWritten(0)
    {
    }


    ///////////////////////////////////////////////////////////////////////////
    // PatchBuilder::Job
    ///////////////////////////////////////////////////////////////////////////

    PatchBuilder::Job::Job()
      : mDiffFileMetadata(),
        mSyncPackageDiffFileEntry(NULL),
        mSyncPackageDiffFileRelativePath(),
        mDestinationFileRelativePath(),
        mDiffFileDownloadData(),
        mDiffFilePath(),
        mDiffFileTempPath(),
        mDiffFileTempStream(),
        mDiffData(),

        mSyncPackageFileEntry(NULL),
        mSyncPackageFileRelativePath(),
        mDestBFIFilePath(),
        mFileDownloadDataList(),
        mBuiltFileInfo()
    {
    }


    void PatchBuilder::Job::Cleanup()
    {
        // Should we do this? Need to make sure it doesn't interfere with other usage of these variables.
        //mDiffFileMetadata.diffId = 0;
        //mDiffFileDownloadData.mRequestId = 0;
        //mFileDownloadDataList.clear();

        // Most of the Job members don't require special handling.
        // Close files and clear errors without examining them. We assume that 
        // we would have already reported errors upon first encountering them and so
        // during shutdown they don't matter to us any more.

        if(mDiffFileTempStream.IsOpen())            // If file is still open, close it and ignore any error state it may have.
        {
            mDiffFileTempStream.Close();            // It's possible that we were Canceled while writing files. So we make sure they are gracefully closed, so they don't complain about being closed in the File destructor.
            mDiffFileTempStream.ClearError();       // 
        }

        if(mBuiltFileInfo.mFileStreamTemp.IsOpen()) // If file is still open, close it and ignore any error state it may have.
        {
            mBuiltFileInfo.mFileStreamTemp.Close();
            mBuiltFileInfo.mFileStreamTemp.ClearError();
        }

        ClearError();
    }


    ///////////////////////////////////////////////////////////////////////////
    // PatchBuilder
    ///////////////////////////////////////////////////////////////////////////

    static const uint64_t kBFISerializeIncrement = 1048576; // We serialize .eaPatchBFI files every so many bytes downloaded.

    PatchBuilder::PatchBuilder(const QString& rootPath, const QString& productId, const QString& gameTitle, Services::Session::SessionWRef curSession) : 
        _state(kStateNone),
        _shutdown(false),
        _shutdownCancel(false),
        _curSession(curSession),
        _downloadSession(NULL),
        _rootPath(rootPath),
        _productId(productId),
        _gameTitle(gameTitle),
        _logPrefix(),
        _syncPackageUrl(),
        _syncPackageFile(NULL),
        _nextDiffFileId(0),
        _jobList(),
        _jobSizeMetrics(),
        _downloadRequestDataMap()
    {
        _logPrefix = QString("[Game Title: %1] [ProductID: %2]").arg(_productId, _gameTitle);

        // In case the QString code above doesn't work in a portable way.
        //char buffer[128];
        //EA::StdC::Snprintf(buffer, EAArrayCount(buffer), "[Game Title: %I16s] [ProductID: %I16s]", _productId.utf16(), _gameTitle.utf16());
        //_logPrefix = buffer;

        EA::Patch::RegisterUserDebugTraceHandler(this, 0);

        PATCHBUILDER_LOG_EVENT("Constructed patch builder.");
    }


    PatchBuilder::~PatchBuilder()
    {
        Shutdown();

        PATCHBUILDER_LOG_EVENT("Destroyed patch builder.");
    }

    DiffFileId PatchBuilder::CreateDiffFileId()
    {
        // Note: This function is only used by clients of this API on a single thread, and therefore this doesn't need to really be threadsafe
        return _nextDiffFileId++;
    }


    // Debug function which prints a list of all jobs.
    #if defined(_DEBUG) || defined(EA_DEBUG)
        void PatchBuilder::TraceJobList()
        {
            PATCHBUILDER_LOG_EVENT_F("%s", "-- Job list ------------------\n");

            for(JobList::iterator it = _jobList.begin(); it != _jobList.end(); ++it)
            {
                Job& job = *it;

                EA::StdC::Sprintf(_debugBuffer,
                                    "Job for %s, "
                                    "    mDestinationFileRelativePath: %s\n",
                                    job.mSyncPackageFileRelativePath.c_str(), 
                                    job.mDestinationFileRelativePath.c_str());

                PATCHBUILDER_LOG_EVENT_F("%s", _debugBuffer);
            }
        }
    #endif

    // Initialize and begin processing.
    // This function is the first function called by the downloader after constructing us. The calling
    // of this function initiates this class to do a data patch.
    void PatchBuilder::Initialize(QString syncPackageUrl, PatchBuilderFileMetadataQList basePackageFilesToPatch)
    {
        // Ensure this can only be called asynchronously. If this is being called from a thread other than the PatchBuilder thread, 
        // then queue this call to be executed on the PatchBuilder thread and return.
        // Note on ASYNC_INVOKE_GUARD - This is just a macro that hides doing QMetaObject::invoke(), which is fairly laborious.  
        // What we are saying here is, let the API client code call this as a normal function call, and we will invoke ourselves 
        // on our owner thread (queue an event) and return control to the calling thread.
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(QString, syncPackageUrl), Q_ARG(PatchBuilderFileMetadataQList, basePackageFilesToPatch));

        // At this point, we are executing in the PatchBuilder thread.
        PATCHBUILDER_LOG_EVENT_F("Initialized patch builder, files to patch: %d", basePackageFilesToPatch.count());

        // To consider: Move this variable clearing to a standalone function.
        SetState(kStateBegin);
        _shutdown        = false;
      //_curSession.clear();        // Don't clear this because it was set in our ctor and should be constant for the lifetime of this class.
        _downloadSession = NULL;
      //_rootPath                   // Already set in our constructor.
      //_productId                  // Already set in our constructor.
      //_gameTitle                  // Already set in our constructor.
      //_logPrefix                  // Already set in our constructor.
        _syncPackageUrl = syncPackageUrl;
        _syncPackageFile = NULL;
        _jobList.clear();
        _jobSizeMetrics = JobSizeMetrics();
        _downloadRequestDataMap.clear();

        //if(_syncPackageUrl.length() == 0) // Debug code.
        //    _syncPackageUrl = "file:D:\\Temp\\DiP3\\tiger_2012_dip3test_v2_SyncPackage.zip";

        // Copy the basePackageFilesToPatch argument to our JobList.
        for(PatchBuilderFileMetadataQList::const_iterator it = basePackageFilesToPatch.begin(); it != basePackageFilesToPatch.end(); ++it)
        {
            PatchBuilderFileMetadata diffFile = *it;

            _jobList.push_back(Job());

            Job& job = _jobList.back();

            job.mDiffFileMetadata = diffFile;

            QStringToString(job.mSyncPackageFileRelativePath, diffFile.originalFilename);    // e.g. SomeDir/SomeFile.dat_DiP_Staged
            QStringToString(job.mDestinationFileRelativePath, diffFile.diskFilename);        // e.g. SomeDir/SomeFile.dat

            // We'll fill in the rest of the Job in our PatchBuilder::onDownloadSessionInitialized function.
        }

        // Create a download source session. 
        _downloadSession = DownloadService::CreateDownloadSession(_syncPackageUrl, _productId, false, _curSession);

        // Hookup slots for download events we process at runtime. 
        ORIGIN_VERIFY_CONNECT_EX(_downloadSession, SIGNAL(initialized(qint64)),                                           this, SLOT(onDownloadSessionInitialized(qint64)),                          Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(_downloadSession, SIGNAL(IDownloadSession_error(Origin::Downloader::DownloadErrorInfo)), this, SLOT(onDownloadSessionError(Origin::Downloader::DownloadErrorInfo)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(_downloadSession, SIGNAL(shutdownComplete()),                                            this, SLOT(onDownloadSessionShutdown()),                                   Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(_downloadSession, SIGNAL(requestChunkAvailable(RequestId)),                              this, SLOT(onDownloadRequestChunkAvailable(RequestId)),                    Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(_downloadSession, SIGNAL(requestError(RequestId, qint32, qint32)),                       this, SLOT(onDownloadRequestError(RequestId, qint32, qint32)),             Qt::QueuedConnection);

        // This will trigger events we receive, such as onDownloadSessionInitialized.
        _downloadSession->initialize();
    }


    // Start actual download/patching
    void PatchBuilder::Start()
    {
        // Ensure this can only be called asynchronously.
        ASYNC_INVOKE_GUARD;

        if(ShouldContinue())
        {
            emit Started();

            ORIGIN_ASSERT(_state == kStateBuildingDestinationFileInfo); // Verify that the last state was as-expected.
            BeginStateDownloadingDestinationFiles();
        }
        else
        {
            // See if we can shutdown
            ShutdownIfAble();
        }
    }


    // Pause download/patching.
    // This function is called by Origin when it needs us to pause processing. We stop what we are doing, 
    // but we don't destroy our serialized files.
    void PatchBuilder::Pause()
    {
        // Set the shutdown flags
        _shutdown = true;           // We are shutting down.
        _shutdownCancel = false;    // We are shutting down, but not nuking our file workspace.
        EAWriteBarrier();           // Make sure other threads see the _shutdown write.

        // Ensure this can only be called asynchronously
        ASYNC_INVOKE_GUARD;

        // Cancel pending download requests.
        StopAllTransfers();

        // See if we can shutdown
        ShutdownIfAble();
    }


    // Cancel download/patching.
    // This function is called by Origin when it needs us to cancel processing. We stop what we are doing 
    // and destroy any serialized/temp files we may have created.
    void PatchBuilder::Cancel()
    {
        // Set the shutdown flags
        _shutdown = true;           // We are shutting down.
        _shutdownCancel = true;     // We are shutting down and also nuking our file workspace.
        EAWriteBarrier();           // Make sure other threads see the _shutdown write.

        // Ensure this can only be called asynchronously
        ASYNC_INVOKE_GUARD;

        // Cancel pending download requests.
        StopAllTransfers();

        // Remove any temporary files that we may be working on, such as .eaPatchTemp files.
        RemoveAllTempFiles();

        // See if we can shutdown
        ShutdownIfAble();
    }


    // This function is called by the downloader when it is finished initializing and was able to download the header for the sync package zip file.
    // The contentLength is the length of the sync package zip file; we need it for some calls below.
    void PatchBuilder::onDownloadSessionInitialized(qint64 contentLength)
    {
        // If we are forced shutdown or otherwise, clear our state and exit here
        if(ShouldContinue())
        {
            PATCHBUILDER_LOG_EVENT("Download Session intialized");

            // At this point we've connected to the downloadSession and received a content length for our sync package on the server.
            // Here we will request our TOC, parse it, and issue download requests for the appropriate files

            // Create a new Central Directory structure
            _syncPackageFile = new PackageFile;

            // This will be ignored, we assume this would be reported back as 1
            int numDiscs = 0;

            // Make a blocking call to download the sync package .zip header and parse it.
            DownloadErrorInfo errInfo;
            if (!ContentProtocolUtilities::ReadZipCentralDirectory(_downloadSession, contentLength, _gameTitle, _productId, *_syncPackageFile, numDiscs, errInfo, NULL, NULL))
            {
                // Report some error here?
                ORIGIN_LOG_ERROR << "Couldn't read central directory for sync package";
                HandleDownloaderError(NULL, ProtocolError::ContentInvalid, 0, __FILE__, __LINE__);
                return;
            }

            // Make sure the package file directory is valid
            if (!ContentProtocolUtilities::VerifyPackageFile(_rootPath, contentLength, _gameTitle, _productId, *_syncPackageFile))
            {
                ORIGIN_LOG_ERROR << "Sync Package file failed validation.";
                HandleDownloaderError(NULL, ProtocolError::ZipOffsetCalculationFailed, 0, __FILE__, __LINE__);
                //POPULATE_ERROR_INFO_TELEMETRY(errInfo, ProtocolError::ZipOffsetCalculationFailed, 0, mProductId.toLocal8Bit().constData());
                return;
            }

            BeginStateBegin();
            BeginStateDownloadingPatchDiffFiles();
        }
        else
        {
            Shutdown();
        }
    }


    // Called by the downloader when it encounters an error.
    void PatchBuilder::onDownloadSessionError(Origin::Downloader::DownloadErrorInfo errInfo)
    {
        // If we're shutting down, we may get errors, just ignore them
        if (!_shutdown)
        {
            PATCHBUILDER_LOG_EVENT_F("Download Session error; Type %d , Code %d", errInfo.mErrorType, errInfo.mErrorCode);

            // As it currently stands, we don't have information about what Job the error occurred with, 
            // so we don't have information needed for perhaps killing just that Job. However, there is a 
            // separate onDownloadRequestError call that we get when there's an error associated with just 
            // a single request.
            HandleDownloaderError(NULL, errInfo);
        }

        ShutdownIfAble();
    }


    // Callsed by the downloader when it is shut down.
    void PatchBuilder::onDownloadSessionShutdown()
    {
        // Just in case, this signal happens when the download session is done.  We should be totally done tracking requests at this point, but this is a failsafe
        ORIGIN_ASSERT(_downloadRequestDataMap.empty());
        _downloadRequestDataMap.clear();
    }


    // This function is called by the downloader when some block of data that we requested has been received.
    void PatchBuilder::onDownloadRequestChunkAvailable(RequestId requestId)
    {
        // Currently disabled because if we cancel a single Job, the requestId might have been removed while this call was in flight, thus triggering this assertion failure.
        // ORIGIN_ASSERT(_downloadRequestDataMap.contains(requestId));

        if(_downloadRequestDataMap.contains(requestId))
        {
            // Grab our cached metadata
            // DownloadRequestData* pDownloadRequestData = _downloadRequestDataMap[requestId];
            
            // Get the actual request object (we don't manage this)
            IDownloadRequest * downloadRequest = _downloadSession->getRequestById(requestId);
            ORIGIN_ASSERT(downloadRequest && downloadRequest->chunkIsAvailable());

            if(downloadRequest && downloadRequest->chunkIsAvailable())
            {
                DownloadRequestData* pDownloadRequestData = _downloadRequestDataMap[requestId];
                const MemBuffer*     pReceivedData        = downloadRequest->getDataBuffer();

                // Process our received data (may only be part of the full request)
                if(ShouldContinue())
                {
                    if(_state == kStateDownloadingPatchDiffFiles)
                        ProcessReceivedDiffData(pDownloadRequestData, pReceivedData);
                    else if(_state == kStateDownloadingDestinationFiles)
                        ProcessReceivedFileSpanData(pDownloadRequestData, pReceivedData);
                }

                // Tell the download service we're done with the chunk!
                downloadRequest->chunkMarkAsRead();

                // Has this request been completed? There will usually be multiple such requests, each of which must complete before we move on from this state.
                if(downloadRequest->isComplete())
                {
                    // Clean up after ourselves
                    _downloadRequestDataMap.remove(requestId);
                    _downloadSession->closeRequest(requestId);
                }

                if(ShouldContinue())
                {
                    // We currently execute these CheckState functions here instead of in the ProcessReceived functions above because
                    // these functions may trigger a significant amount of processing, and we want to make sure that the above closeRequest
                    // occurs before executing this, as it may be holding onto something that we'd like to see freed before we proceed.
                    if(_state == kStateDownloadingPatchDiffFiles)
                        CheckStateDownloadingPatchDiffFiles();              // See if we're done receiving all of our .eaPatchDiff data for all files.
                    else if(_state == kStateDownloadingDestinationFiles)
                        CheckStateDownloadingDestinationFiles();            // See if we're done downloading all files that we expect to be downloading.
                }
                else
                {
                    ShutdownIfAble();
                }
            }
        }
    }


    void PatchBuilder::onDownloadRequestError(RequestId requestId, qint32 errType, qint32 errCode)
    {
        // Question: Should we assert that the following checks are valid? Or can they reasonably occur at runtime and not be a bug in our code?
        if (!_downloadSession)
            return;

        IDownloadRequest *req = _downloadSession->getRequestById(requestId);
        if (!req)
            return;

        if (!_downloadRequestDataMap.contains(requestId))
            return;

        // Get the download request metadata
        DownloadRequestData* pDownloadRequestData = _downloadRequestDataMap[requestId];
        ORIGIN_ASSERT(pDownloadRequestData); EA_UNUSED(pDownloadRequestData);

        // Only bother logging if we're not shutting down and the error isn't that the request was cancelled.
        if (!_shutdown && (errType != DownloadError::DownloadWorkerCancelled))
        {
            PATCHBUILDER_LOG_EVENT_F("Download Session error; RequestId %lld, Type %d , Code %d", requestId, errType, errCode);
            HandleDownloaderError(pDownloadRequestData->mpJob, errType, errCode, __FILE__, __LINE__);
        }

        // Clean up after ourselves
        _downloadRequestDataMap.remove(requestId);
        _downloadSession->closeRequest(requestId);

        ShutdownIfAble();
    }


    void PatchBuilder::Shutdown()
    {
        PATCHBUILDER_LOG_EVENT("PatchBuilder Shutdown.");

        // Destroy everything here!
        _downloadRequestDataMap.clear();

        // Clear the job list, taking care to shut down the jobList gracefully instead of just calling clear on it.
        // To consider: Make a standalone function for clearing the job list.
        {
            for(JobList::iterator it = _jobList.begin(); it != _jobList.end(); ++it)
            {
                Job& job = *it;

                // We don't currently call CancelJob, as that deletes temp files, which we may not want to do here.
                job.Cleanup();
            }

            _jobList.clear();
        }

        if(_syncPackageFile)
        {
            delete _syncPackageFile;
            _syncPackageFile = NULL;
        }

        // Kill the download session
        if(_downloadSession)
        {
            DownloadService::DestroyDownloadSession(_downloadSession);
            _downloadSession = NULL;
        }
    }

    void PatchBuilder::StopAllTransfers()
    {
        // If there are outstanding requests
        if(!_downloadRequestDataMap.empty())
        {
            // If there is a download session
            if (_downloadSession != NULL)
            {
                // Cancel all outstanding requests
                PATCHBUILDER_LOG_EVENT("Stopping all transfers...");
                _downloadSession->shutdown();
            }
        }
    }

    // This function is called during execution in our PatchBuilder thread if 
    void PatchBuilder::ShutdownIfAble()
    {
        // We're not done if there are pending download requests.  Those requests will call back to this function when they complete.
        if(_downloadRequestDataMap.empty())
        {
            // The actual shutdown happens in this call to Shutdown.
            Shutdown();

            // Emit the appropriate signal
            // ORIGIN_ASSERT(_shutdown); I believe currently this assertion would be valid, but keep it disabled until we are sure.

            if (_shutdown)
            {
                if (_shutdownCancel)
                {
                    emit Canceled();
                }
                else
                {
                    emit Paused();
                }
            }
        }
    }

    void PatchBuilder::BeginStateBegin()
    {
        if(ShouldContinue())
        {
            // We don't need to set kStateBegin because it was set earlier, in our Init.
            // SetState(kStateBegin);

            // Set up JobList
            // The job list is a list of all files that we will be building. It it simply all the files that we were given to patch by 
            // our owner. It may be that there are other files in the app we are patching which weren't given to us. This can happen if
            // it was determined that they weren't worth the effort of doing differential patching with for various possible reasons.
            #if (PATCHBUILDER_DEVELOPER_DEBUG >= 2)
                PATCHBUILDER_LOG_EVENT_F("PatchBuilder Job list (%d files):", _syncPackageFile->GetNumberOfFiles());
            #else
                PATCHBUILDER_LOG_EVENT_F("PatchBuilder Job list: %d files.", _syncPackageFile->GetNumberOfFiles());
            #endif

            QString sTempW;

            for(JobList::iterator it = _jobList.begin(); it != _jobList.end(); ++it)
            {
                Job& job = *it;

                // job.mBasePackageEntry
                // This is already set up.

                // job.mSyncPackageFileRelativePath (e.g. GameData/SomeFile.dat) Should be the same as job.mBasePackageEntry->GetFileName.
                // This was set earlier in Initialize.

                // job.mSyncPackageFileEntry
                job.mSyncPackageFileEntry = _syncPackageFile->GetEntryByName(job.mDiffFileMetadata.originalFilename);
                ORIGIN_ASSERT(job.mSyncPackageFileEntry);

                // If we can't find the package file entry
                if (!job.mSyncPackageFileEntry)
                {
                    PATCHBUILDER_LOG_EVENT_F("PatchBuilder shutting down due to invalid sync package.  Can't find base file entry.  File %s, Reason %d", job.mSyncPackageFileRelativePath.c_str(), 0);

                    HandleDownloaderError(&job, PatchBuilderError::SyncPackageInvalid, 0, __FILE__, __LINE__);
                    return;
                }

                ORIGIN_ASSERT(job.mSyncPackageFileEntry->GetFileCRC() == job.mDiffFileMetadata.fileCRC); // This MUST be true or the file doesn't match

                // If the sync package doesn't match the base package
                if (job.mSyncPackageFileEntry->GetFileCRC() != job.mDiffFileMetadata.fileCRC)
                {
                    PATCHBUILDER_LOG_EVENT_F("PatchBuilder shutting down due to invalid sync package.  CRC mismatch.  File %s, Reason %d", job.mSyncPackageFileRelativePath.c_str(), 1);

                    HandleDownloaderError(&job, PatchBuilderError::SyncPackageInvalid, 1, __FILE__, __LINE__);
                    return;
                }

                ORIGIN_ASSERT(job.mSyncPackageFileEntry->GetCompressionMethod() == 0); // Files must be uncompressed!

                // If the file is compressed, the package is invalid
                if (job.mSyncPackageFileEntry->GetCompressionMethod() != 0)
                {
                    PATCHBUILDER_LOG_EVENT_F("PatchBuilder shutting down due to invalid sync package.  Base File compressed.  File %s, Reason %d", job.mSyncPackageFileRelativePath.c_str(), 2);

                    HandleDownloaderError(&job, PatchBuilderError::SyncPackageInvalid, 2, __FILE__, __LINE__);
                    return;
                }

                // job.mSyncPackageDiffFileRelativePath (e.g. GameData/SomeFile.dat.eaPatchDiff)
                job.mSyncPackageDiffFileRelativePath = job.mSyncPackageFileRelativePath;
                job.mSyncPackageDiffFileRelativePath += kPatchDiffFileNameExtension;

                // job.mSyncPackageDiffFileEntry
                StringToQString(sTempW, job.mSyncPackageDiffFileRelativePath);
                job.mSyncPackageDiffFileEntry = _syncPackageFile->GetEntryByName(sTempW);
                ORIGIN_ASSERT(job.mSyncPackageDiffFileEntry);

                // If we can't find the diff file entry
                if (!job.mSyncPackageDiffFileEntry)
                {
                    PATCHBUILDER_LOG_EVENT_F("PatchBuilder shutting down due to invalid sync package.  Can't find diff file entry.  File %s, Reason %d", job.mSyncPackageFileRelativePath.c_str(), 3);

                    HandleDownloaderError(&job, PatchBuilderError::SyncPackageInvalid, 3, __FILE__, __LINE__);
                    return;
                }

                ORIGIN_ASSERT(job.mSyncPackageDiffFileEntry->GetCompressionMethod() == 0); // Files must be uncompressed!

                // If the file is compressed, the package is invalid
                if (job.mSyncPackageDiffFileEntry->GetCompressionMethod() != 0)
                {
                    PATCHBUILDER_LOG_EVENT_F("PatchBuilder shutting down due to invalid sync package.  Diff file compressed.  File %s, Reason %d", job.mSyncPackageFileRelativePath.c_str(), 4);

                    HandleDownloaderError(&job, PatchBuilderError::SyncPackageInvalid, 4, __FILE__, __LINE__);
                    return;
                }

                // job.mDiffFilePath     (e.g. C:\SomeApp\SomeDir\SomeFile.dat.eaPatchDiff)
                QStringToString(job.mDiffFilePath, _rootPath);
                EnsureTrailingPathSeparator(job.mDiffFilePath);
                job.mDiffFilePath += job.mSyncPackageDiffFileRelativePath;
                ForwardSlashesToFileSystemSlashes(job.mDiffFilePath);

                // job.mDiffFileTempPath (e.g. C:\SomeApp\SomeDir\SomeFile.dat.eaPatchDiff.eaPatchTemp)
                job.mDiffFileTempPath = job.mDiffFilePath;
                job.mDiffFileTempPath += kPatchTempFileNameExtension;

                // job.mBuiltFileInfo.mLocalFilePath (e.g. C:\SomeApp\SomeDir\SomeFile.dat)
                QStringToString(job.mBuiltFileInfo.mLocalFilePath, _rootPath);
                EnsureTrailingPathSeparator(job.mBuiltFileInfo.mLocalFilePath);
                job.mBuiltFileInfo.mLocalFilePath += job.mSyncPackageFileRelativePath;
                ForwardSlashesToFileSystemSlashes(job.mBuiltFileInfo.mLocalFilePath);

                // job.mBuiltFileInfo.mDestinationFilePath     (e.g. C:\SomeApp\SomeDir\SomeFile.dat_DiP_Staged)
                QStringToString(job.mBuiltFileInfo.mDestinationFilePath, _rootPath);
                EnsureTrailingPathSeparator(job.mBuiltFileInfo.mDestinationFilePath);
                job.mBuiltFileInfo.mDestinationFilePath += job.mDestinationFileRelativePath;
                ForwardSlashesToFileSystemSlashes(job.mBuiltFileInfo.mDestinationFilePath);

                // job.mBuiltFileInfo.mDestinationFilePathTemp (e.g. C:\SomeApp\SomeDir\SomeFile.dat_Staged.eaPatchTemp)
                job.mBuiltFileInfo.mDestinationFilePathTemp = job.mBuiltFileInfo.mDestinationFilePath;
                job.mBuiltFileInfo.mDestinationFilePathTemp += kPatchTempFileNameExtension;

                // job.mDestBFIFilePath (e.g. C:\SomeApp\SomeDir\SomeFile.dat_Staged.eaPatchBFI)
                job.mDestBFIFilePath = job.mBuiltFileInfo.mDestinationFilePath;
                job.mDestBFIFilePath += kPatchBFIFileNameExtension;

                // job.mDiffFileDownloadData
                // job.mDiffFileTempStream
                // job.mDiffData
                // job.mFileDownloadDataList
                // job.mBuiltFileInfo
                // job.mFileStream 
                // job.mError
                // There is nothing to do with these yet.

                #if (PATCHBUILDER_DEVELOPER_DEBUG >= 2)
            
                    EA::StdC::Sprintf(_debugBuffer,
                                      "Job for %s\n"
                                      "    size: %I64u\n"
                                      "    mSyncPackageDiffFileRelativePath: %s\n"
                                      "    mDiffFilePath: %s\n"
                                      "    mDiffFileTempPath: %s\n"
                                      "    mSyncPackageFileRelativePath: %s\n"
                                      "    mDestFilePath: %s\n"
                                      "    mDestFileTempPath: %s\n"
                                      "    mLocalPrevFilePath: %s\n"
                                      "    mDestBFIFilePath: %s\n",
                                       job.mSyncPackageFileRelativePath.c_str(), 
                                       job.mDiffFileMetadata.fileUncompressedSize, 
                                       job.mSyncPackageDiffFileRelativePath.c_str(),
                                       job.mDiffFilePath.c_str(),
                                       job.mDiffFileTempPath.c_str(),
                                       job.mSyncPackageFileRelativePath.c_str(),
                                       job.mBuiltFileInfo.mDestinationFilePath.c_str(),
                                       job.mBuiltFileInfo.mDestinationFilePathTemp.c_str(),
                                       job.mBuiltFileInfo.mLocalFilePath.c_str(),
                                       job.mDestBFIFilePath.c_str());

                    PATCHBUILDER_LOG_EVENT_F("%s", _debugBuffer);
                #endif
            }
        }
    }


    void PatchBuilder::CompleteStateBegin()
    {
        // Nothing to do, currently.
    }


    void PatchBuilder::BeginStateDownloadingPatchDiffFiles()
    {
        if(ShouldContinue())
        {
            // Begin download all the relevant required .eaPatchDiff files via IDownloadSession.
            SetState(kStateDownloadingPatchDiffFiles);

            QVector<RequestId> requestIdArray;
            qint64 dataToDownload = 0;

            for(JobList::iterator it = _jobList.begin(); it != _jobList.end(); ++it)
            {
                Job& job = *it;
                bool shouldDownload = true; // True until proven false.

                // We download any required .eaPatchDiff files. It may be that we are resuming a previously started
                // job and some portion (or even all) of the .eaPatchDiff files are already downloaded. Some of them
                // may be only partially downloaded and we need to resume where we left off for them.

                // Set up some initial download data, even if we don't end up downloading it (due to already being downloaded). We use the numbers anyway.
                job.mDiffFileDownloadData.mpJob            = &job;
                job.mDiffFileDownloadData.mServerFileStart = job.mSyncPackageDiffFileEntry->GetOffsetToFileData();
                job.mDiffFileDownloadData.mServerFileEnd   = job.mSyncPackageDiffFileEntry->GetEndOffset();

                // Update metrics
                // We are updating the jobDiffLocalReadSize metric with the server file size and not the local
                // file size. The reason for this is that we know the server size for free upon start of
                // initialization without opening any files, but can't know the size of local files without taking 
                // time opening them. All that matters is that we measure relative progress, and the server files 
                // are typically going to be of similar size to the local files.
                _jobSizeMetrics.jobDiffLocalReadSize += job.mSyncPackageFileEntry->GetUncompressedSize(); // The sync package files are always uncompressed, so compressed size == uncompressed size.

                if(EA::IO::File::Exists(job.mDiffFilePath.c_str())) // If this .eaPatchDiff file has not already been fully downloaded and renamed...
                {
                    job.mDiffFileDownloadData.mBytesWritten = (job.mDiffFileDownloadData.mServerFileEnd - job.mDiffFileDownloadData.mServerFileStart);

                    if(EA::IO::File::Exists(job.mDiffFileTempPath.c_str()))  // This should rarely if ever be the case, because we should already have done this in a previous session.
                        Origin::Util::DiskUtil::DeleteFileWithRetry(job.mDiffFileTempPath.c_str());

                    // Do a size check, and if the size mis-matches then delete and re-do the file.
                    // To consider: Do a CRC check, which is more accurate but slower.
                    uint64_t fileSize = EA::IO::File::GetSize(job.mDiffFilePath.c_str());
                    ORIGIN_ASSERT(fileSize == (uint64_t)(job.mDiffFileDownloadData.mServerFileEnd - job.mDiffFileDownloadData.mServerFileStart));
                    if(fileSize == (uint64_t)(job.mDiffFileDownloadData.mServerFileEnd - job.mDiffFileDownloadData.mServerFileStart))
                        shouldDownload = false; // The file looks good, at least from a size standpoint.
                    else
                        Origin::Util::DiskUtil::DeleteFileWithRetry(job.mDiffFilePath.c_str());
                }

                if(shouldDownload)
                {
                    if(EA::IO::File::Exists(job.mDiffFileTempPath.c_str()))
                    {
                        uint64_t fileSize = EA::IO::File::GetSize(job.mDiffFilePath.c_str()); // See how much has already been downloaded...

                        if(fileSize > (uint64_t)(job.mDiffFileDownloadData.mServerFileEnd - job.mDiffFileDownloadData.mServerFileStart)) // If the file is too big... we assume it's corrupted.
                        {
                            // To consider: Write code which validates the already downloaded data. Actually this isn't an easy problem, since the data doesn't have info we could use for this purpose.
                            Origin::Util::DiskUtil::DeleteFileWithRetry(job.mDiffFileTempPath.c_str());
                        }
                        else
                        {   // Continue downloading this file where we last left off. We pretend that we've already written fileSize bytes.
                            job.mDiffFileDownloadData.mServerFileStart += fileSize;
                            job.mDiffFileDownloadData.mBytesWritten     = fileSize;
                        }
                    }

                    if(job.mDiffFileDownloadData.mServerFileEnd != job.mDiffFileDownloadData.mServerFileStart) // If there is anything to download...
                    {
                        dataToDownload += (job.mDiffFileDownloadData.mServerFileEnd - job.mDiffFileDownloadData.mServerFileStart);
                        job.mDiffFileDownloadData.mRequestId = _downloadSession->createRequest(job.mDiffFileDownloadData.mServerFileStart, job.mDiffFileDownloadData.mServerFileEnd);

                        requestIdArray.push_back(job.mDiffFileDownloadData.mRequestId);
                        _downloadRequestDataMap[job.mDiffFileDownloadData.mRequestId] = &job.mDiffFileDownloadData;

                        // Update metrics.
                        _jobSizeMetrics.jobDiffFileDownloadSize += (job.mDiffFileDownloadData.mServerFileEnd - job.mDiffFileDownloadData.mServerFileStart);
                    }
                    else
                        ProcessCompletedDiffFileDownload(job);
                }
            }

            // Notify our current progress. (Start from 0)
            emit InitializeMetadataProgress(0, _jobSizeMetrics.jobDiffFileDownloadSize + _jobSizeMetrics.jobDiffLocalReadSize);

            if(requestIdArray.size()) // If the setup done above resulted in any downloads that need to be done (usually this will be so)...
            {
                ORIGIN_LOG_EVENT << "[Patch Builder] Downloading " << requestIdArray.size() << " eaPatchDiff files. (" << dataToDownload << " bytes)";
                _downloadSession->submitRequests(requestIdArray);
                // We'll get called back as data is downloaded. During the final callback we'll call CompleteStateDownloadingPatchDiffFiles() 
                // and then BeginStateBuildingDestinationFileInfo() to move onto the next stage.
            }
            else            
                CompleteStateDownloadingPatchDiffFiles(); // We are already done.
        }
    }


    void PatchBuilder::ProcessReceivedDiffData(DownloadRequestData* pDownloadRequestData, const MemBuffer * pReceivedData)
    {
        Job* pJob = pDownloadRequestData->mpJob;
        ORIGIN_ASSERT(pJob && (pDownloadRequestData == &pJob->mDiffFileDownloadData));

        if(!pJob->HasError())
        {
            if(!pJob->mDiffFileTempStream.IsOpen())
            {
                // As of this writing, the only way we guarantee that the open file count < PATCHBUILDER_MAX_OPEN_FILE_COUNT is that
                // it happens to be that the Downloader service won't have more than a few files being downloaded at a time. If that 
                // doesn't work out in the future then we may want to come up with a scheme for throttling the open file count.
                ORIGIN_ASSERT(EA::Patch::File::GetOpenFileCount() < PATCHBUILDER_MAX_OPEN_FILE_COUNT);

                pJob->mDiffFileTempStream.Open(pJob->mDiffFileTempPath.c_str(), EA::IO::kAccessFlagReadWrite, EA::IO::kCDOpenAlways); // kCDOpenAlways means create if not already present, leave contents if present.

                if(pJob->mDiffFileTempStream.HasError()) // If the open failed...
                {
                    pJob->TransferError(pJob->mDiffFileTempStream); // Copy the error to pJob.
                    ORIGIN_LOG_ERROR << GetEAPatchContextString(pJob->GetError());
                    CREATE_DOWNLOAD_ERROR_INFO(errInfo);
                    HandleEAPatchError(pJob, pJob->GetError(), errInfo);
                }
                else
                {
                    ORIGIN_ASSERT(EA::IO::File::GetSize(pJob->mDiffFileTempPath.c_str()) == pDownloadRequestData->mBytesWritten);
                    ORIGIN_ASSERT(pDownloadRequestData->mDestFileStart == 0);           // This is just a sanity check.

                    pJob->mDiffFileTempStream.SetPosition(0, EA::IO::kPositionTypeEnd); // Move to the end of the file, which is the same as the beginning of the file if we are writing this for the first time.
                }
            }

            if(!pJob->HasError())
            {
                const quint8* pData    = pReceivedData->GetBufferPtr();
                const quint32 dataSize = pReceivedData->TotalSize();

                if(pJob->mDiffFileTempStream.Write(pData, dataSize))
                {
                    pDownloadRequestData->mBytesWritten += dataSize;

                    // Update metrics
                    _jobSizeMetrics.jobDiffFileDownloadCurrent += dataSize;

                    // See if we are done.
                    const bool currentFileDone = pDownloadRequestData->mBytesWritten == (pDownloadRequestData->mServerFileEnd - pDownloadRequestData->mServerFileStart);

                    if(currentFileDone)
                    {
                        pJob->mDiffFileDownloadData.mRequestId = 0;
                        pJob->mDiffFileTempStream.Close();
                        ProcessCompletedDiffFileDownload(*pJob);
                    }

                    // To consider: Finish the checksum calculation for the file, and examine it.

                    // Notify our current progress.
                    emit InitializeMetadataProgress(_jobSizeMetrics.jobDiffFileDownloadCurrent + _jobSizeMetrics.jobDiffLocalReadCurrent, _jobSizeMetrics.jobDiffFileDownloadSize + _jobSizeMetrics.jobDiffLocalReadSize);
                }
                else
                {
                    pJob->TransferError(pJob->mDiffFileTempStream); // Copy the error to pJob.
                    ORIGIN_LOG_ERROR << GetEAPatchContextString(pJob->GetError());
                    CREATE_DOWNLOAD_ERROR_INFO(errInfo);
                    HandleEAPatchError(pJob, pJob->GetError(), errInfo);
                }
            }
        }
    }


    void PatchBuilder::ProcessCompletedDiffFileDownload(Job& job)
    {
        // The .eaPatchDiff.eaPatchTemp file is done downloading and is ready to be renamed to .eaPatchDiff.
        if(!EA::IO::File::Rename(job.mDiffFileTempPath.c_str(), job.mDiffFilePath.c_str(), true))
        {
            // The following will result in the error reported and the Job cancelled.
            ORIGIN_LOG_ERROR << "Unable to rename: " << job.mDiffFileTempPath.c_str() << " to " << job.mDiffFilePath.c_str();
            HandleEAPatchError(&job, EA::Patch::kECFatal, EA::Patch::GetLastSystemErrorId(), EA::Patch::kEAPatchErrorIdFileRenameError, __FILE__, __LINE__);
        }

        // We don't read the .eaPatchDiff file contents into job.mDiffData here; we do it in the 
        // processing for kStateBuildingDestinationFileInfo. However, there's an opportunity to 
        // do an optimization whereby we save the job.mDiffData in memory as we are receiving it
        // during download. That would avoid us having to re-open the file and read the contents. 
    }


    void PatchBuilder::CheckStateDownloadingPatchDiffFiles()
    {
        if(ShouldContinue())
        {
            // To do: Handle the case of us being in a state of error.
            if(_downloadRequestDataMap.empty()) // If there are no outstanding download requests...
                CompleteStateDownloadingPatchDiffFiles();
        }
    }


    void PatchBuilder::CompleteStateDownloadingPatchDiffFiles()
    {
        // We implement code to verify the .eaPatchDiff files in the BeginStateBuildingDestinationFileInfo function, 
        // as it needs to load and process that .eaPatchDiff data.
        if(ShouldContinue())
            BeginStateBuildingDestinationFileInfo();
    }


    void PatchBuilder::GenerateBuiltFileInfoProgress(const EA::Patch::DiffData& /*diffData*/, const EA::Patch::BuiltFileInfo& /*builtFileInfo*/, 
                                                     const EA::Patch::Error& /*error*/, intptr_t userContext, uint64_t byteProgress)
    {
        const Job* pJob = reinterpret_cast<const Job*>(userContext);

        // Some info we have at hand which is related to what we might want to report:
        // pJob->mBuiltFileInfo.mLocalFilePath;             // The path to the local file we are patching from, empty if it's not present.
        // pJob->mBuiltFileInfo.mLocalFileSize;             // Size of the local file we are patching from, 0 if it's not present.
        // pJob->mDestinationFileRelativePath;              // The path to the file we are patching to. Will typically have something like DiP_STAGED appended to it.
        // pJob->mDiffFileMetadata.diskFilename;            // The path to the file we are patching to, minus the appended name. 
        // pJob->mDiffFileMetadata.fileUncompressedSize;    // Size of the file we are patching to.
        // pJob->mDiffFileMetadata.diffId;                  // The downloader diffId of the file being patched.

        // What we do here is convert the amount of byte progress on the local file to the amount of byte progress on the destination file.
        // We are doing this because our progress events are in terms of building the destination file and not about reading the local file
        // However, the GenerateBuiltFileInfo function works by reading the local file and the only way for it to report its progress is in 
        // terms of that file. All we care about is reporting a percent completion, and so we can just roughly translate the local file progress
        // amount to what it equates to in the destination file progress amount.
        double   fractionalLocalFileProgress = ((double)byteProgress / (double)pJob->mBuiltFileInfo.mLocalFileSize);
        uint64_t destFileByteProgress        = (uint64_t)(fractionalLocalFileProgress * (double)pJob->mDiffFileMetadata.fileUncompressedSize);

        // _jobSizeMetrics.jobDiffLocalReadCurrent is the current number of bytes we've read from local files in 
        // order to finish the Initialize (diff processing) phase. It's the sum of the pJob->mDiffFileMetadata.fileUncompressedSize
        // values for all the Jobs. We report the current stats plus the stats for this current file we are working on.

        emit InitializeMetadataProgress(_jobSizeMetrics.jobDiffFileDownloadCurrent + _jobSizeMetrics.jobDiffLocalReadCurrent + destFileByteProgress, 
                                        _jobSizeMetrics.jobDiffFileDownloadSize    + _jobSizeMetrics.jobDiffLocalReadSize);
    }


    void PatchBuilder::BeginStateBuildingDestinationFileInfo()
    {
        if(ShouldContinue())
        {
            SetState(kStateBuildingDestinationFileInfo);

            DiffFileErrorList rejectedList;

            // Tasks to do:
            //     Load all the files into diff data (job.mDiffFilePath -> job.mDiffData).
            //     Convert the diff data into BuiltFileInfo (job.mDiffData + mLocalPrevFilePath -> job.mBuiltFileInfo).
            // This function could take a while to execute, as it reads every byte from every local file being patched.

            EA::Patch::DiffDataSerializer ddSerializer;

            JobList::iterator it = _jobList.begin();
            while ((it != _jobList.end()) && ShouldContinue())
            {
                Job& job = *it;

                ddSerializer.Deserialize(job.mDiffData, job.mDiffFilePath.c_str());

                if(ddSerializer.HasError())
                {
                    job.TransferError(ddSerializer);
                    ORIGIN_LOG_ERROR << GetEAPatchContextString(job.GetError());
                    CREATE_DOWNLOAD_ERROR_INFO(errInfo);
                    HandleEAPatchError(&job, job.GetError(), errInfo);
                }
                else
                {
                    // Assert that the diff data's expectation of the final patched file size is what the server says it is. If this fails then the "sync package" is out of sync with the "base package."
                    ORIGIN_ASSERT(job.mDiffData.mFileSize == (uint64_t)job.mDiffFileMetadata.fileUncompressedSize);
                    ORIGIN_ASSERT(job.mSyncPackageFileEntry->GetFileCRC() == job.mDiffFileMetadata.fileCRC); // TODO: Do something here if this is not true!

                    // Set up job.mBuiltFileInfo, so we can call the GenerateBuiltFileInfo function, which is a key engine function.
                    // This is the first use of job.mBuiltFileInfo, so its members are reset. We don't need to initialize every member
                    // of the struct here, as some members are fine with their initial cleared values, and the GenerateBuiltFileInfo
                    // call below will generate a number of the member values. A couple of the members (e.g. mRemoteFilePathURL) are
                    // not relevant to us in this Origin implementation, as we are using a different system for downloading.

                    job.mBuiltFileInfo.mDestinationFilePathTemp = job.mBuiltFileInfo.mDestinationFilePathTemp;
                    job.mBuiltFileInfo.mLocalFilePath           = job.mBuiltFileInfo.mLocalFilePath;

                    // The following call will determine all the segments, in order, that need to be retrieved from the server
                    // to build this patched file, as well as the ordered segments from the previous file that can be used to 
                    // build this patched file. The runtime of this function is proportional to the size of the previous file, 
                    // as it needs to read every byte from the previous file.
                    EA::Patch::Error error;

                    // We don't need to call GenerateBuiltFileInfo if it's already been built and serialized to disk.
                    bool generateRequired = true;

                    if(EA::IO::File::Exists(job.mDestBFIFilePath.c_str()))
                    {
                        EA::Patch::BuiltFileInfoSerializer bfiSerializer;

                        bfiSerializer.Deserialize(job.mBuiltFileInfo, job.mDestBFIFilePath.c_str());

                        if(bfiSerializer.HasError())
                        {
                            if (!Origin::Util::DiskUtil::DeleteFileWithRetry(job.mDestBFIFilePath.c_str())) // If we can't even delete the file...
                            {
                                ORIGIN_LOG_ERROR << "Unable to delete file: " << job.mDestBFIFilePath.c_str();
                                HandleEAPatchError(&job, EA::Patch::kECFatal, EA::Patch::GetLastSystemErrorId(), EA::Patch::kEAPatchErrorIdFileRemoveError, __FILE__, __LINE__);
                                continue;
                            }
                        }
                        else
                        {
                            // To do: Verify that ((job.mBuiltFileInfo.mLocalFileSize == file size of job.mBuiltFileInfo.mLocalFilePath) or (LocalFileSize == 0 and mLocalFilePath doesn't exist)).

                            generateRequired = false;
                        }
                    }

                    if(generateRequired)
                    {
                        // We call GenerateBuiltFileInfo with ourself being a listener to the progress events it generates. We handle those events in
                        // our GenerateBuiltFileInfoProgress function and use the progress events we receive to notify our owner of the progress of this
                        // patching and/or of the progress of the given Job. This reporting is useful because GenerateBuiltFileInfo can be a slow function
                        // for very large files.
                        if(EA::Patch::PatchBuilder::GenerateBuiltFileInfo(job.mDiffData, job.mBuiltFileInfo, error, this, reinterpret_cast<intptr_t>(&job)))
                        {
                            // Write the .eaPatchBFI (built file info file), which is simply bfi serialized to disk.
                            EA::Patch::BuiltFileInfoSerializer bfiSerializer;

                            bfiSerializer.Serialize(job.mBuiltFileInfo, job.mDestBFIFilePath.c_str());

                            if(bfiSerializer.HasError())
                            {
                                job.TransferError(bfiSerializer);
                                ORIGIN_LOG_ERROR << GetEAPatchContextString(job.GetError());
                                CREATE_DOWNLOAD_ERROR_INFO(errInfo);
                                HandleEAPatchError(&job, job.GetError(), errInfo);
                                continue;
                            }

                            // We are updating the jobDiffLocalReadCurrent metric with the server file size and not the local
                            // file size. The reason we do this is that jobDiffLocalReadSize is also the server file size and 
                            // not the local size. The reason for this is that we know the server size for free upon start of
                            // initialization without opening any files, but can't know the size of local files without taking 
                            // time opening them. All that matters is that we measure relative progress, and the server files 
                            // are typically going to be of similar size to the local files.
                            _jobSizeMetrics.jobDiffLocalReadCurrent += job.mSyncPackageFileEntry->GetUncompressedSize(); // The sync package files are always uncompressed, so compressed size == uncompressed size.

                            // Notify our current progress.
                            emit InitializeMetadataProgress(_jobSizeMetrics.jobDiffFileDownloadCurrent + _jobSizeMetrics.jobDiffLocalReadCurrent, 
                                                            _jobSizeMetrics.jobDiffFileDownloadSize    + _jobSizeMetrics.jobDiffLocalReadSize);

                            // If it isn't efficient to bother diff updating this file
                            if (job.mBuiltFileInfo.mRemoteSpanCount > static_cast<uint64_t>(job.mDiffFileMetadata.fileCompressedSize))
                            {
                                // Mark this as a rejected file
                                rejectedList.push_back(DiffFileError(job.mDiffFileMetadata.diffId, (ContentDownloadError::type)PatchBuilderError::RejectFileNonOptimal));

                                // Kill this job.
                                CancelJob(&job);         // Removes .eaPatchDiff file, closes any open files for the job (there should be none), cancels any currently downloads for the job (there should be none).
                                it = _jobList.erase(it); // Kill this item and get the new iterator (next item)

                                continue;
                            }

                            // Add to the job counters
                            _jobSizeMetrics.jobFileDownloadSize  += static_cast<qint64>(job.mBuiltFileInfo.mRemoteSpanCount);
                            _jobSizeMetrics.jobFileLocalCopySize += job.mBuiltFileInfo.mLocalSpanCount;

                            // Currently we emit a per-file stats update. To consider: Make a signal that reports the progress of 
                            // the entire job (e.g. _jobSizeMetrics.jobFileDownloadCurrent/_jobSizeMetrics.jobFileDownloadSize).
                            emit PatchFileProgress(job.mDiffFileMetadata.diffId, job.mBuiltFileInfo.mRemoteSpanWritten, job.mBuiltFileInfo.mRemoteSpanCount);
                        }
                        else
                        {
                            ORIGIN_LOG_ERROR << GetEAPatchContextString(error);
                            CREATE_DOWNLOAD_ERROR_INFO(errInfo);
                            HandleEAPatchError(&job, error, errInfo);
                        }
                    }
                    else
                    {
                        // Add to the job counters
                        _jobSizeMetrics.jobDiffLocalReadCurrent += job.mSyncPackageFileEntry->GetUncompressedSize(); // The sync package files are always uncompressed, so compressed size == uncompressed size.
                        _jobSizeMetrics.jobFileDownloadSize  += static_cast<qint64>(job.mBuiltFileInfo.mRemoteSpanCount);
                        _jobSizeMetrics.jobFileLocalCopySize += job.mBuiltFileInfo.mLocalSpanCount;

                        // Currently we emit a per-file stats update. To consider: Make a signal that reports the progress of 
                        // the entire job (e.g. _jobSizeMetrics.jobFileDownloadCurrent/_jobSizeMetrics.jobFileDownloadSize).
                        emit PatchFileProgress(job.mDiffFileMetadata.diffId, job.mBuiltFileInfo.mRemoteSpanWritten, job.mBuiltFileInfo.mRemoteSpanCount);
                    }
                }

                job.mDiffData.Reset(); // We don't need this information any more, so free it.

                ++it; // Next item. We increment here instead of in a for loop statement because above we may erase it from the job list.

            } // for each job...

            if (!rejectedList.empty())
            {
                // Notify the caller that we are unable to patch certain files
                emit InitializeRejectFiles(rejectedList);
            }
            
            CompleteStateBuildingDestinationFileInfo();
        }
    }


    void PatchBuilder::CompleteStateBuildingDestinationFileInfo()
    {
        // To do: We may want to rename the Initialized signal to something else, or at add some additional info to its arguments.
        //        I'm not sure the term "Initialized" is the best way to describe the process of setting up the job list, downloading
        //        diff files, and processing them against local files. That's a bit more than initialization, and is more like "phase 1" (of 2).
        if(ShouldContinue())
        {
            // Report the full download size, the total number of bytes we need to download of the files we're building.
            emit Initialized(_jobSizeMetrics.jobFileDownloadSize);

            // We don't execute any more. Rather we wait until the Origin downloader calls our Start function, which 
            // will trigger us to go into the next state.
        }
    }


    void PatchBuilder::BeginStateDownloadingDestinationFiles()
    {
        if(ShouldContinue())
        {
            SetState(kStateDownloadingDestinationFiles);

            // We do something similar to the EAPatch package's PatchBuilder::BuildDestinationFiles.
            using namespace EA::Patch;

            QVector<RequestId> requestIdArray;
            qint64 dataToDownload = 0;
            qint64 minSpanSize = (qint64)std::numeric_limits<int64_t>::max();
            qint64 maxSpanSize = 0;

            for(JobList::iterator it = _jobList.begin(); (it != _jobList.end()) && ShouldContinue(); ++it)
            {
                Job&           job = *it;
                BuiltFileInfo& bfi = job.mBuiltFileInfo;

                // See if there is a serialized .eaPatchBFI file on disk we can use. If so load it here using BuiltFileInfoSerializer.
                if(EA::IO::File::Exists(job.mDestBFIFilePath.c_str()))
                {
                    BuiltFileInfoSerializer bfiSerializer;
                    BuiltFileInfo           bfiTemp;

                    if(bfiSerializer.Deserialize(bfiTemp, job.mDestBFIFilePath.c_str()) &&      // If we can deserialize the info into our bfiTemp... and
                       (bfiTemp.mBuiltFileSpanList.size() == bfi.mBuiltFileSpanList.size()))    // if a basic size test passes...
                    {
                        // We copy the portions of bfiTemp that relate to download progress. We leave the other 
                        // portions (e.g. paths) as-is, since the paths may theoretically have changed since the last session.
                        // To do: Move the code below to a standlone member function of BuiltFileInfo.
                        bfi.mLocalSpanWritten  = bfiTemp.mLocalSpanWritten;
                        bfi.mRemoteSpanWritten = bfiTemp.mRemoteSpanWritten;

                        for(BuiltFileSpanList::iterator itTemp = bfiTemp.mBuiltFileSpanList.begin(), it = bfi.mBuiltFileSpanList.begin(); 
                            itTemp != bfiTemp.mBuiltFileSpanList.end(); 
                            ++itTemp, ++it)
                        {
                            BuiltFileSpan& bfsTemp = *itTemp;
                            BuiltFileSpan& bfs     = *it;

                            ORIGIN_ASSERT((bfs.mSourceFileType == bfsTemp.mSourceFileType) && // To consider: Bail on this BFI load if these mis-match.
                                          (bfs.mSourceStartPos == bfsTemp.mSourceStartPos) &&
                                          (bfs.mDestStartPos   == bfsTemp.mDestStartPos)   &&
                                          (bfs.mCount          == bfsTemp.mCount));

                            bfs.mCountWritten        = bfsTemp.mCountWritten;
                            bfs.mSpanWrittenChecksum = bfsTemp.mSpanWrittenChecksum;
                        }
                    }
                    else
                    {
                        // Rather than throw an error and bail, we simply delete the .eaPatchBFI file and move on.
                        // To consider: Replace the code below and similar code else where with Downloader file system and error calls and usage.
                        if (!Origin::Util::DiskUtil::DeleteFileWithRetry(job.mDestBFIFilePath.c_str())) // If we can't even delete the file...
                            HandleEAPatchError(&job, EA::Patch::kECFatal, EA::Patch::GetLastSystemErrorId(), EA::Patch::kEAPatchErrorIdFileRemoveError, __FILE__, __LINE__);
                    } 
                }

                // Possibly download segments for this file we are building.
                if(bfi.mRemoteSpanWritten < bfi.mRemoteSpanCount) // If there is more to download for this file...
                {
                    const uint64_t serverZipFileDataBegin = job.mSyncPackageFileEntry->GetOffsetToFileData();
                    const uint64_t serverZipFileDataEnd   = job.mSyncPackageFileEntry->GetEndOffset(); EA_UNUSED(serverZipFileDataEnd);

                    // Go through bfi.mBuiltFileSpanList and add jobs for all uncompleted downloads.
                    for(BuiltFileSpanList::iterator it = bfi.mBuiltFileSpanList.begin(); (it != bfi.mBuiltFileSpanList.end()) && ShouldContinue(); ++it) // For each span in the file...
                    {
                        BuiltFileSpan& bfs = *it;

                        if((bfs.mSourceFileType == kFileTypeRemote) && (bfs.mCountWritten < bfs.mCount)) // If this span refers to data to download from the server file, and if there is more to write for this span...
                        {
                            job.mFileDownloadDataList.push_back(DownloadRequestData());
                            DownloadRequestData& drd = job.mFileDownloadDataList.back();

                            drd.mpJob            = &job;
                            drd.mpBuiltFileSpan  = &bfs;
                            drd.mDestFileStart   = bfs.mDestStartPos + bfs.mCountWritten;       // Usually mCountWritten is 0, but it can be > 0 if we are resuming from a previous patch session.
                            drd.mServerFileStart = serverZipFileDataBegin + drd.mDestFileStart; // For downloading from the server file, the server and dest positions are the same.
                            drd.mServerFileEnd   = drd.mServerFileStart + (bfs.mCount - bfs.mCountWritten);
                          //drd.mBytesWritten    = Leave as-is, which is usually 0, but may be otherwise if we loaded a pre-existing .eaPatchBFI file above.
                            drd.mRequestId       = _downloadSession->createRequest(drd.mServerFileStart, drd.mServerFileEnd);

                            int remoteSpanSize = (drd.mServerFileEnd - drd.mServerFileStart);
                            if (remoteSpanSize < minSpanSize)
                                minSpanSize = remoteSpanSize;
                            if (remoteSpanSize > maxSpanSize)
                                maxSpanSize = remoteSpanSize;
                            dataToDownload += remoteSpanSize;

                            ORIGIN_ASSERT((drd.mServerFileStart >= (int64_t)serverZipFileDataBegin) && 
                                          (drd.mServerFileEnd   <= (int64_t)serverZipFileDataEnd)); // Sanity check.

                            requestIdArray.push_back(drd.mRequestId);
                            _downloadRequestDataMap[drd.mRequestId] = &drd;

                            // Metrics upkeep
                            // To do: mFileDownloadVolumeFinal += (drd.mServerFileEnd - drd.mServerFileStart); // This is the download volume required to complete the patch build.

                            #if (PATCHBUILDER_DEVELOPER_DEBUG >= 2)
                                PATCHBUILDER_LOG_EVENT_F("PatchBuilder::BeginStateDownloadingDestinationFiles: Job added. RangeBegin: %9I64u, RangeCount: %9I64u, DestPath: %s\n", drd.mDestFileStart, drd.mServerFileEnd - drd.mServerFileStart, job.mBuiltFileInfo.mDestinationFilePath.c_str());
                            #endif
                        }
                        else if((bfs.mSourceFileType == kFileTypeLocal) && (bfs.mCountWritten < bfs.mCount))
                        {
                            // To do: mFileCopyVolumeFinal += (bfs.mCount - bfs.mCountWritten); // This is the copied file volume required to complete the patch build.
                            // We'll do the actual copying of local file data in a later step, after downloading (though we may add code to parallelize the two).
                        }

                    } // for each span in this file that we need to download...

                } // If this job still requires downloading...

            } // for each job...

            if(ShouldContinue())
            {
                if(requestIdArray.size()) // If the setup done above resulted in any downloads that need to be done (usually this will be so)...
                {
                    ORIGIN_LOG_EVENT << "[Patch Builder] Downloading " << requestIdArray.size() << " remote spans. (Total: " << dataToDownload << " bytes  Min: " << minSpanSize << " Max: " << maxSpanSize << ")";
                    _downloadSession->submitRequests(requestIdArray);
                    // We'll get called back as data is downloaded.
                }
                else            
                {
                    // We can immediately move on to copying local files.
                    CompleteStateDownloadingDestinationFiles();
                }
            }
        }
    }


    void PatchBuilder::ProcessReceivedFileSpanData(DownloadRequestData* pDownloadRequestData, const MemBuffer * pReceivedData)
    {
        Job* pJob = pDownloadRequestData->mpJob;
        ORIGIN_ASSERT(pJob);

        const quint8*  pData           = pReceivedData->GetBufferPtr();
        const quint32  dataSize        = pReceivedData->TotalSize();
        const uint64_t prevWriteTotal  = pJob->mBuiltFileInfo.mLocalSpanWritten + pJob->mBuiltFileInfo.mRemoteSpanWritten;
        const uint64_t newWriteTotal   = prevWriteTotal + dataSize;

        if(dataSize && !pJob->HasError())
        {
            #if defined(_DEBUG) // This data too can be changed from the downloader thread as we execute.
                IDownloadRequest* pRequest = _downloadSession->getRequestById(pDownloadRequestData->mRequestId);

                qint64 requestStartOffset    = pRequest->getStartOffset();
                qint64 requestEndOffset      = pRequest->getEndOffset();
            #endif

            // Get the iterator to Job::mFileDownloadDataList that corresponds to pDownloadRequestData.
            DownloadRequestDataList::iterator it = pJob->mFileDownloadDataList.begin();
            while((it != pJob->mFileDownloadDataList.end()) && (&*it != pDownloadRequestData))
                ++it;
            ORIGIN_ASSERT(it != pJob->mFileDownloadDataList.end()); // Assert that it was found.

            // Open the dest file if not open already.
            if(!pJob->mBuiltFileInfo.mFileStreamTemp.IsOpen())
            {
                // As of this writing, the only way we guarantee that the open file count < PATCHBUILDER_MAX_OPEN_FILE_COUNT is that
                // it happens to be that the Downloader service won't have more than a few files being downloaded at a time. If that 
                // doesn't work out in the future then we may want to come up with a scheme for throttling the open file count.
                ORIGIN_ASSERT(EA::Patch::File::GetOpenFileCount() < PATCHBUILDER_MAX_OPEN_FILE_COUNT);

                pJob->mBuiltFileInfo.mFileStreamTemp.Open(pJob->mBuiltFileInfo.mDestinationFilePathTemp.c_str(), EA::IO::kAccessFlagReadWrite, EA::IO::kCDOpenAlways); // kCDOpenAlways means create if not already present, leave contents if present.

                if(pJob->mBuiltFileInfo.mFileStreamTemp.HasError()) // If the open failed...
                {
                    pJob->TransferError(pJob->mBuiltFileInfo.mFileStreamTemp); // Copy the error to pJob.
                    ORIGIN_LOG_ERROR << GetEAPatchContextString(pJob->GetError());
                    CREATE_DOWNLOAD_ERROR_INFO(errInfo);
                    HandleEAPatchError(pJob, pJob->GetError(), errInfo);
                }
                else
                    pJob->mBuiltFileInfo.mFileUseCount++;
            }

            if(!pJob->HasError())
            {
                if ((pDownloadRequestData->mDestFileStart + pDownloadRequestData->mBytesWritten) < 0)
                {
                    ORIGIN_LOG_WARNING << QString("[Patch Buider] Negative offset detected! deststart=%1 byteswritten=%2").arg(pDownloadRequestData->mDestFileStart).arg(pDownloadRequestData->mBytesWritten);
                }

                if(pJob->mBuiltFileInfo.mFileStreamTemp.WritePosition(pData, dataSize, pDownloadRequestData->mDestFileStart + pDownloadRequestData->mBytesWritten)) // This will have no effect of the file couldn't be opened.
                {
                    pDownloadRequestData->mBytesWritten += dataSize;

                    // Assert that the following are in sync.
                    ORIGIN_ASSERT((requestStartOffset    == pDownloadRequestData->mServerFileStart) &&
                                  (requestEndOffset      == pDownloadRequestData->mServerFileEnd));

                    // We need to maintain pJob->mBuiltFileInfo.mBuiltFileSpanList. pDownloadRequestData stores information about the download request we 
                    // made to the downloader. BuiltFileSpan is the data struct we use to store what we need to download from the server. Given that our
                    // requests have a 1:1 correspondence with BuiltFileSpans, it's somewhat redundant to have both the BuiltFileSpan struct to maintain
                    // at the same time as the DownloadRequestData we maintain. There may be a way to reduce or remove 
                    pDownloadRequestData->mpBuiltFileSpan->mCountWritten += dataSize;
                  //pDownloadRequestData->mpBuiltFileSpan->mSpanWrittenChecksum; // To do: Update this:
                    pJob->mBuiltFileInfo.mRemoteSpanWritten += dataSize;

                    // We periodically serialize the .eaPatchBFI (built file info file). We do this because if we get interrupted
                    // during a large file download then we can pick up near where we left off if we save our state to disk here 
                    // and re-load it upon resuming.
                    const bool currentSpanDone = (pDownloadRequestData->mBytesWritten == (pDownloadRequestData->mServerFileEnd - pDownloadRequestData->mServerFileStart));

                    // Sanity check:
                    // Another way of telling that the download is done is to test BuilfFileSpan::mCountWritten == mBuiltFileSpan::mCount. We assert here that these are the same, as expected.
                    ORIGIN_ASSERT(currentSpanDone == (pDownloadRequestData->mpBuiltFileSpan->mCountWritten == pDownloadRequestData->mpBuiltFileSpan->mCount));

                    if(((prevWriteTotal / kBFISerializeIncrement) != (newWriteTotal / kBFISerializeIncrement)) || currentSpanDone) // If it's been kBFISerializeIncrement bytes since we've last saved off a copy of the BFI to disk or if we've finished the current span....
                    {
                        pJob->mBuiltFileInfo.mFileStreamTemp.Flush(); // This isn't necessarily required on all platforms.

                        // Write the .eaPatchBFI (built file info file), which is simply bfi serialized to disk.
                        EA::Patch::BuiltFileInfoSerializer bfiSerializer;

                        bfiSerializer.Serialize(pJob->mBuiltFileInfo, pJob->mDestBFIFilePath.c_str());

                        // Report progress whenever we've committed the BFI file
                        if (!pJob->HasError())
                        {
                            // Currently we emit a per-file stats update. To consider: Make a signal that reports the progress of 
                            // the entire job (e.g. _jobSizeMetrics.jobFileDownloadCurrent/_jobSizeMetrics.jobFileDownloadSize).
                            emit PatchFileProgress(pJob->mDiffFileMetadata.diffId, pJob->mBuiltFileInfo.mRemoteSpanWritten, pJob->mBuiltFileInfo.mRemoteSpanCount);
                        }

                        if(bfiSerializer.HasError())
                        {
                            pJob->TransferError(bfiSerializer);
                            ORIGIN_LOG_ERROR << GetEAPatchContextString(pJob->GetError());
                            CREATE_DOWNLOAD_ERROR_INFO(errInfo);
                            HandleEAPatchError(pJob, pJob->GetError(), errInfo);
                        }
                    }

                    if(!pJob->HasError())
                    {
                        //  Update metrics.
                        _jobSizeMetrics.jobFileDownloadCurrent += dataSize;

                        // If this segment download is complete, remove this pDownloadRequestData. Note that a file being built may have more than one such segment.
                        if(currentSpanDone)
                        {
                            it->mRequestId = 0;                    // Not currently necessary because we erase it below. 
                            pJob->mFileDownloadDataList.erase(it); // To consider: Move this to a pJob->mFileDownloadDataListCompleted member (doesn't currently exist) instead of erasing it. If we do so then we should probably set mRequestId to 0 while doing so.

                            // If the entire file download is complete (i.e. all its segments are complete), close the file and process it.
                            // We could conceivably leave the file open for the next phase whereby we write kFileTypeLocal data to it, 
                            // but that would risk resulting in too many files open for the operating system to allow.
                            if(pJob->mFileDownloadDataList.empty())
                            {
                                pJob->mBuiltFileInfo.mFileUseCount--;
                                pJob->mBuiltFileInfo.mFileStreamTemp.Close();
                                ProcessCompletedFileDownload(*pJob);
                            }
                        }
                    }
                }
                else
                {
                    pJob->TransferError(pJob->mBuiltFileInfo.mFileStreamTemp); // Copy the error to pJob.
                    ORIGIN_LOG_ERROR << GetEAPatchContextString(pJob->GetError());
                    CREATE_DOWNLOAD_ERROR_INFO(errInfo);
                    HandleEAPatchError(pJob, pJob->GetError(), errInfo);
                }
            }
        }
    }


    void PatchBuilder::ProcessCompletedFileDownload(Job& job)
    {
        // To do. It's not clear if we need to do anything here, as we aren't done with the file yet.
        // Maybe emit some message that we are done with the server download portion of this file.
        ORIGIN_ASSERT( job.mFileDownloadDataList.empty());
        ORIGIN_ASSERT(!job.mBuiltFileInfo.mFileStreamTemp.IsOpen());

        if(job.mBuiltFileInfo.mFileStreamTemp.IsOpen()) // We shouldn't need this Close here, as it should be done in ProcessReceivedFileSpanData, but right now we're working on debugging this code 
            job.mBuiltFileInfo.mFileStreamTemp.Close(); // for the first time and for some reason it's not always closed as it should be. This Close here lets us at least proceed with the process.
    }


    void PatchBuilder::CheckStateDownloadingDestinationFiles()
    {
        if(ShouldContinue())
        {
            // To do: Handle the case of us being in a state of error.
            // To do: Implement an assert here which verifies that if _downloadRequestDataMap is empty then so are all the Job::mFileDownloadDataList containers.

            if(_downloadRequestDataMap.empty()) // If there are no outstanding download requests...
            {
                // To do: Implement some state sanity check assertions here.
                CompleteStateDownloadingDestinationFiles();
            }
        }
        else
        {
            ShutdownIfAble();
        }
    }


    void PatchBuilder::CompleteStateDownloadingDestinationFiles()
    {
        if(ShouldContinue())
        {
            BeginStateCopyingLocalFiles();

            // To consider: generate some telemetry and/or status update reporting.
        }
        else
        {
            ShutdownIfAble();
        }
    }


    void PatchBuilder::BeginStateCopyingLocalFiles()
    {
        if(ShouldContinue())
        {
            SetState(kStateCopyingLocalFiles);

            // We do something similar to the EAPatch package's PatchBuilder::BuildDestinationFiles.
            using namespace EA::Patch;

            for(JobList::iterator it = _jobList.begin(); (it != _jobList.end()) && ShouldContinue(); ++it)
            {
                Job&           job = *it;
                BuiltFileInfo& bfi = job.mBuiltFileInfo;

                // There is no need to load any disk-based .eaPatchBFI files, as we did that in kStateDownloadingDestinationFiles.
                if(bfi.mLocalSpanWritten < bfi.mLocalSpanCount) // If there is more to copy for this file...
                {
                    EA::Patch::FileSpanCopier fileSpanCopier;

                    if(fileSpanCopier.Open(bfi.mLocalFilePath.c_str(), bfi.mDestinationFilePathTemp.c_str()))
                    {
                        for(BuiltFileSpanList::iterator it = bfi.mBuiltFileSpanList.begin(); (it != bfi.mBuiltFileSpanList.end()) && ShouldContinue(); ++it) // For each local span in the file...
                        {
                            BuiltFileSpan& bfs = *it;

                            if((bfs.mSourceFileType == kFileTypeLocal) && (bfs.mCountWritten < bfs.mCount)) // If there is more to write for this span...
                            {
                                const uint64_t sourceStartPos = bfs.mSourceStartPos + bfs.mCountWritten;
                                const uint64_t destStartPos   = bfs.mDestStartPos + bfs.mCountWritten;
                                const uint64_t copyCount      = bfs.mCount - bfs.mCountWritten;

                                fileSpanCopier.CopySpan(sourceStartPos, destStartPos, copyCount);

                                if(fileSpanCopier.HasError())
                                {
                                    ORIGIN_LOG_WARNING << QString("FileSpanCopier error ; spos=%1 dpos=%2 count=%3 ; ").arg(sourceStartPos).arg(destStartPos).arg(copyCount) << GetEAPatchContextString(fileSpanCopier.GetError());
                                    //TransferError(fileSpanCopier);
                                }
                                else
                                { 
                                    bfs.mCountWritten     += copyCount;
                                    bfi.mLocalSpanWritten += copyCount;

                                    // Update metrics
                                    _jobSizeMetrics.jobFileLocalCopyCurrent += copyCount;

                                    //mFileCopyVolume += copyCount;
                                    //const double stateCompletion = (double)(mFileDownloadVolume + mFileCopyVolume) / (double)(mFileDownloadVolumeFinal + mFileCopyVolumeFinal); // Yields a value between 0 and 1, which indicates how far through the current state alone we are.
                                    //mProgressValue = kStateCompletionValue[kStateBuildingDestinationFiles] + (stateCompletion * (kStateCompletionValue[kStateBuildingDestinationFiles + 1] - kStateCompletionValue[kStateBuildingDestinationFiles]));
                                    //NotifyProgress();

                                    // To do (must do): For large files, serialize the BuiltFileInfo periodically. That way if 
                                    // we are interrupted during this copying then we can continue it later.
                                }
                            }
                        } // For each local span in the file.

                        fileSpanCopier.Close();

                        // Write the .eaPatchBFI (built file info file), which is simply bfi serialized to disk.
                        const String            sDestinationBFIFilePath(bfi.mDestinationFilePath + kPatchBFIFileNameExtension);    // e.g. /GameData/Data1.big.eaPatchBFI
                        BuiltFileInfoSerializer bfiSerializer;

                        bfiSerializer.Serialize(bfi, sDestinationBFIFilePath.c_str());

                        if(bfiSerializer.HasError())
                        {
                            ORIGIN_LOG_WARNING << QString("BFI Serializer error ; dest=%1 ; ").arg(sDestinationBFIFilePath.c_str()) << GetEAPatchContextString(bfiSerializer.GetError());
                            //TransferError(bfiSerializer);
                        }
                    }

                    if(fileSpanCopier.HasError())
                    {
                        //if(mbSuccess) // Use the first reported error if it exists, not this one.
                        //{
                        //    TransferError(fileSpanCopier);
                        //}
                        fileSpanCopier.ClearError();
                    }

                } // If there was more to copy for this file.
                // Else the file is fully copied and the .eaPatchBFI file on disk should be up to date.

                // We should have caught any errors already, but we do this to make sure.
                if(bfi.mFileStreamTemp.HasError())
                {
                    ORIGIN_LOG_WARNING << "FileStreamTemp error ; " << GetEAPatchContextString(bfi.mFileStreamTemp.GetError());
                    // TransferError(bfi.mFileStreamTemp);
                }

                // To do: Verify the disk file's hash against patchEntry.mFileHashValue.
                // Is there a way to calculate the hash as it's being written to disk instead of afterwards?               
                // DiffData::mBlockHashArray could provide a way to calculate or validate the hash as we go.
            } // For each file...

        }  

        CompleteStateCopyingLocalFiles();
    }


    void PatchBuilder::CompleteStateCopyingLocalFiles()
    {
        if(ShouldContinue())
        {
            BeginStateRenamingDestinationFiles();
        }
        else
        {
            ShutdownIfAble();
        }
    }


    bool PatchBuilder::RemoveAllTempFiles(Job* pJob)
    {
        int failureCount = 0;

        //    Delete .eaPatchTemp files.
        //    Delete .eaPatchBFI files
        //    Delete .eaPatchDiff files.
        //    Delete .eaPatchDiff.eaPatchTemp files.
        // To consider: Do we delete any files that have been converted from .eaPatchTemp to their final names.

        // To consider: Should we remove this too?:
        // Delete job.mBuiltFileInfo.mDestinationFilePath
        // EA::IO::File::Remove(EA::IO::File::Exists(job.mBuiltFileInfo.mDestinationFilePath.c_str()));

        // Delete job.mBuiltFileInfo.mDestinationFilePathTemp
        if (EnvUtils::GetFileExistsNative(pJob->mBuiltFileInfo.mDestinationFilePathTemp.c_str()) && !Origin::Util::DiskUtil::DeleteFileWithRetry(pJob->mBuiltFileInfo.mDestinationFilePathTemp.c_str()))
        {
            failureCount++; // We use the result of Exists to determine failure and not a return value from Remove, because another entity may have removed it as we are executing.
            HandleError(EA::Patch::kECBlocking, EA::Patch::GetLastSystemErrorId(), EA::Patch::kEAPatchErrorIdFileRemoveError, pJob->mBuiltFileInfo.mDestinationFilePathTemp.c_str());                
        }

        // Delete pJob->mDestBFIFilePath
        if (EnvUtils::GetFileExistsNative(pJob->mDestBFIFilePath.c_str()) && !Origin::Util::DiskUtil::DeleteFileWithRetry(pJob->mDestBFIFilePath.c_str()))
        {
            failureCount++;
            HandleError(EA::Patch::kECBlocking, EA::Patch::GetLastSystemErrorId(), EA::Patch::kEAPatchErrorIdFileRemoveError, pJob->mDestBFIFilePath.c_str());                
        }

        // Delete pJob->mDiffFilePath
        if (EnvUtils::GetFileExistsNative(pJob->mDiffFilePath.c_str()) && !Origin::Util::DiskUtil::DeleteFileWithRetry(pJob->mDiffFilePath.c_str()))
        {
            failureCount++;
            HandleError(EA::Patch::kECBlocking, EA::Patch::GetLastSystemErrorId(), EA::Patch::kEAPatchErrorIdFileRemoveError, pJob->mDiffFilePath.c_str());                
        }

        // Delete pJob->mDiffFileTempPath
        if (EnvUtils::GetFileExistsNative(pJob->mDiffFileTempPath.c_str()) && !Origin::Util::DiskUtil::DeleteFileWithRetry(pJob->mDiffFileTempPath.c_str()))
        {
            failureCount++;
            HandleError(EA::Patch::kECBlocking, EA::Patch::GetLastSystemErrorId(), EA::Patch::kEAPatchErrorIdFileRemoveError, pJob->mDiffFileTempPath.c_str());                
        }

        return (failureCount == 0);
    }


    bool PatchBuilder::RemoveAllTempFiles()
    {
        int failureCount = 0;

        for(JobList::iterator it = _jobList.begin(); it != _jobList.end(); ++it)
        {
            Job& job = *it;

            if(!RemoveAllTempFiles(&job))
                failureCount++;
        }


        /* Alternative implementation:
        // Walk the tree recursively and remove files that match *.eaPatch*
        EA::IO::DirectoryIterator di;
        EA::IO::DirectoryIterator::EntryList entryList;

        di.Read(_rootPath.utf16(), entryList, L"*.eaPatch*", EA::IO::kDirectoryEntryFile, 9999999, false);

        for(EA::IO::DirectoryIterator::EntryList::iterator it = entryList.begin(); it != entryList.end(); ++it)
        {
            EA::IO::DirectoryIterator::Entry& entry = *it;

            EA::IO::File::Remove(entry.msName.c_str());
            if(EA::IO::File::Exists(entry.msName.c_str()))
            {
                failureCount++;  // We use the result of Exists to determine failure and not a return value from Remove, because another entity may have removed it as we are executing.
                HandleError(EA::Patch::kECBlocking, EA::Patch::GetLastSystemErrorId(), EA::Patch::kEAPatchErrorIdFileRemoveError, entry.msName.c_str() (Need to convert this to UTF8));               
            }
        }
        */

        /* Alternative QDirIterator implementation:
        // Walk the tree recursively and remove files that match *.eaPatch*
        QDirIterator dirIterator(_rootPath, QDir::Files, QDirIterator::Subdirectories);
        QString      eaPatchPattern = ".eaPatch";

        while(dirIterator.hasNext())
        {
            QString path = dirIterator.next();

            if(path.size())
            {
                QString fileName = dirIterator.fileName();
                
                if(fileName.contains(eaPatchPattern)) // If it's a .eaPatchXXX file...
                {
                    QFile::remove(path);
                    if(QFile::exists(path))
                    {
                        failureCount++;  // We use the result of Exists to determine failure and not a return value from Remove, because another entity may have removed it as we are executing.
                        HandleError(EA::Patch::kECBlocking, EA::Patch::GetLastSystemErrorId(), EA::Patch::kEAPatchErrorIdFileRemoveError, path.utf8());               
                    }
                }
            }
        }
        */

        return (failureCount == 0);
    }


    void PatchBuilder::ClearTempFilesForEntry(QString root, QString fileRelativePath, QString stagedFileRelativePath)
    {
        // For this staged file:
        //    Delete the .eaPatchDiff.eaPatchTemp file if present.
        //    Delete the .eaPatchDiff file if present.
        //    Delete the .eaPatchBFI file if present.
        //    Delete the .eaPatchTemp files.
        //   ?Delete the staged file itself if present.

        QString filePath = (root + stagedFileRelativePath + ".eaPatchDiff.eaPatchTemp");
        if (EnvUtils::GetFileExistsNative(filePath) && !Origin::Util::DiskUtil::DeleteFileWithRetry(filePath))
            ORIGIN_LOG_ERROR << "PatchBuilder: Unable to remove file: " << filePath;

        // eaPatchDiff files aren't based on the staged path
        filePath = (root + fileRelativePath + ".eaPatchDiff");
        if (EnvUtils::GetFileExistsNative(filePath) && !Origin::Util::DiskUtil::DeleteFileWithRetry(filePath))
            ORIGIN_LOG_ERROR << "PatchBuilder: Unable to remove file: " << filePath;

        filePath = (root + stagedFileRelativePath + ".eaPatchBFI");
        if (EnvUtils::GetFileExistsNative(filePath) && !Origin::Util::DiskUtil::DeleteFileWithRetry(filePath))
            ORIGIN_LOG_ERROR << "PatchBuilder: Unable to remove file: " << filePath;

        filePath = (root + stagedFileRelativePath + ".eaPatchTemp");
        if (EnvUtils::GetFileExistsNative(filePath) && !Origin::Util::DiskUtil::DeleteFileWithRetry(filePath))
            ORIGIN_LOG_ERROR << "PatchBuilder: Unable to remove file: " << filePath;

        // filePath = (root + stagedFileRelativePath);
        // if(QFile::exists(filePath) && !QFile::remove(fullPath))
        //     ORIGIN_LOG_ERROR << "PatchBuilder: Unable to remove file: " << filePath;
    }

/*
    if (fileExists && removeExistingFile)
    {
        // Zero out the file size we think we have for this file
        fileSize = 0;

        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Removing existing file due to CRC mismatch: " << fullPath;
        if (!QFile::remove(fullPath))
        {
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Unable to remove CRC mismatched file: " << fullPath;
        }

        if (fileIsBeingDiffUpdated)
        {
            // TODO: Maybe tell EAPatchBuilderOrigin we need to clear out any partial data for this file?
        }
    }
*/


    void PatchBuilder::BeginStateRenamingDestinationFiles()
    {
        if(ShouldContinue())
        {
            SetState(kStateRenamingDestinationFiles);

            using namespace EA::Patch;

            // Rename .eaPatchTemp files to the final file names.
            // Delete .eaPatchBFI files
            // Delete .eaPatchDiff files.
            // There should be no .eaPatchDiff.eaPatchTemp files.

            for(JobList::iterator it = _jobList.begin(); (it != _jobList.end()) && ShouldContinue(); ++it)
            {
                Job& job = *it;

                // Rename  job.mBuiltFileInfo.mDestinationFilePathTemp to job.bfi.mDestinationFilePath;
                ORIGIN_ASSERT(EA::IO::File::Exists(job.mBuiltFileInfo.mDestinationFilePathTemp.c_str()) !=  // Assert that one or the other exists, and not both.
                              EA::IO::File::Exists(job.mBuiltFileInfo.mDestinationFilePath.c_str()));
                if(EA::IO::File::Exists(job.mBuiltFileInfo.mDestinationFilePathTemp.c_str()))
                    EA::IO::File::Rename(job.mBuiltFileInfo.mDestinationFilePathTemp.c_str(), job.mBuiltFileInfo.mDestinationFilePath.c_str());

#if !defined(ORIGIN_PC)
                // Get the file attributes for the target file
                quint32 fileAttributes = job.mSyncPackageFileEntry->GetFileAttributes();

                // Set the attributes, this is particularly important since Unix-based OSs (like MacOS) depend on the execute bit
                if (fileAttributes != 0)
                {
                    // Strip everything but the execute permission bits.
                    quint32 unixPermissions = fileAttributes & 0777;
            
                    // Explicitly set relaxed permissions to allow resuming downloads by other users.
                    unixPermissions |= 0666;
            
                    chmod(job.mBuiltFileInfo.mDestinationFilePath.c_str(), unixPermissions);
                }
#endif

                // Delete job.mDestBFIFilePath
                ORIGIN_ASSERT(EA::IO::File::Exists(job.mDestBFIFilePath.c_str()));
                Origin::Util::DiskUtil::DeleteFileWithRetry(job.mDestBFIFilePath.c_str());

                // Delete job.mDiffFilePath; 
                ORIGIN_ASSERT(EA::IO::File::Exists(job.mDiffFilePath.c_str()));  // We will need to remove this assert if we change the code to remove this at an earlier stage.
                Origin::Util::DiskUtil::DeleteFileWithRetry(job.mDiffFilePath.c_str());

                // Verify there is no job.mDiffFileTempPath file.
                ORIGIN_ASSERT(!EA::IO::File::Exists(job.mDiffFileTempPath.c_str()));

                // Let our clients know the file is done
                emit PatchFileComplete(job.mDiffFileMetadata.diffId);
            }

            CompleteStateRenamingDestinationFiles();
        }
        else
        {
            ShutdownIfAble();
        }
    }


    void PatchBuilder::CompleteStateRenamingDestinationFiles()
    {
        if(ShouldContinue())
        {
            BeginStateEnd();
        }
        else
        {
            ShutdownIfAble();
        }
    }


    void PatchBuilder::BeginStateEnd()
    {
        if(ShouldContinue())
        {
            SetState(kStateEnd);

            emit Complete();

            CompleteStateEnd();
        }
        else
        {
            ShutdownIfAble();
        }
    }


    void PatchBuilder::CompleteStateEnd()
    {
        if(ShouldContinue())
        {
            // Currently nothing to do.
        }
    }


    void PatchBuilder::HandleEAPatchDebugTrace(const EA::Patch::DebugTraceInfo& info, intptr_t /*userContex*/)
    {
        PATCHBUILDER_LOG_EVENT(info.mpText);
    }


    struct EAPatchErrorToContentDownloadError
    {
        EA::Patch::EAPatchErrorId               mEAPatchErrorId;
        Origin::Downloader::ContentDownloadError::type mDownloaderErrorId;
    };

    static EAPatchErrorToContentDownloadError gErrorArray[] = 
    {
        { EA::Patch::kEAPatchErrorIdNone,                       ContentDownloadError::OK                                          },
        { EA::Patch::kEAPatchErrorIdUnknown,                    (ContentDownloadError::type)PatchBuilderError::UNKNOWN            },
        { EA::Patch::kEAPatchErrorIdFileOpenFailure,            (ContentDownloadError::type)PatchBuilderError::FileOpenFailure    },
        { EA::Patch::kEAPatchErrorIdFileReadFailure,            (ContentDownloadError::type)PatchBuilderError::FileReadFailure    },
        { EA::Patch::kEAPatchErrorIdFileReadTruncate,           (ContentDownloadError::type)PatchBuilderError::FileReadTruncate   }, // Not an exact match. Maybe like DownloadError::ContentLength
        { EA::Patch::kEAPatchErrorIdFileWriteFailure,           (ContentDownloadError::type)PatchBuilderError::FileWriteFailure   },
        { EA::Patch::kEAPatchErrorIdFilePosFailure,             (ContentDownloadError::type)PatchBuilderError::FilePosFailure     },
        { EA::Patch::kEAPatchErrorIdFileCloseFailure,           (ContentDownloadError::type)PatchBuilderError::FileCloseFailure   }, // Unknown matches.
        { EA::Patch::kEAPatchErrorIdFileCorrupt,                (ContentDownloadError::type)PatchBuilderError::FileCorrupt        },
        { EA::Patch::kEAPatchErrorIdFileRemoveError,            (ContentDownloadError::type)PatchBuilderError::FileRemoveError    },
        { EA::Patch::kEAPatchErrorIdFileNameLength,             (ContentDownloadError::type)PatchBuilderError::FileNameLength     },
        { EA::Patch::kEAPatchErrorIdFilePathLength,             (ContentDownloadError::type)PatchBuilderError::FilePathLength     },
        { EA::Patch::kEAPatchErrorIdFileNotFound,               (ContentDownloadError::type)PatchBuilderError::FileNotFound       },
        { EA::Patch::kEAPatchErrorIdFileNotSpecified,           (ContentDownloadError::type)PatchBuilderError::FileNotSpecified   },
        { EA::Patch::kEAPatchErrorIdFileRenameError,            (ContentDownloadError::type)PatchBuilderError::FileRenameError    },
        { EA::Patch::kEAPatchErrorIdFileCreateError,            (ContentDownloadError::type)PatchBuilderError::FileCreateError    },
        { EA::Patch::kEAPatchErrorIdDirCreateError,             (ContentDownloadError::type)PatchBuilderError::DirCreateError     },
        { EA::Patch::kEAPatchErrorIdDirReadError,               (ContentDownloadError::type)PatchBuilderError::DirReadError       },
        { EA::Patch::kEAPatchErrorIdDirRenameError,             (ContentDownloadError::type)PatchBuilderError::DirRenameError     },
        { EA::Patch::kEAPatchErrorIdDirRemoveError,             (ContentDownloadError::type)PatchBuilderError::DirRemoveError     },
        { EA::Patch::kEAPatchErrorIdXMLError,                   (ContentDownloadError::type)PatchBuilderError::XMLError           },
        { EA::Patch::kEAPatchErrorIdMemoryFailure,              (ContentDownloadError::type)PatchBuilderError::MemoryFailure      },
        { EA::Patch::kEAPatchErrorIdURLError,                   (ContentDownloadError::type)PatchBuilderError::URLError           },
        { EA::Patch::kEAPatchErrorIdThreadError,                (ContentDownloadError::type)PatchBuilderError::ThreadError        },
        { EA::Patch::kEAPatchErrorIdServerError,                (ContentDownloadError::type)PatchBuilderError::ServerError        },
        { EA::Patch::kEAPatchErrorIdHttpInitFailure,            (ContentDownloadError::type)DownloadError::HttpError              }, // HTTP matches.
        { EA::Patch::kEAPatchErrorIdHttpDNSFailure,             (ContentDownloadError::type)DownloadError::HttpError              },
        { EA::Patch::kEAPatchErrorIdHttpConnectFailure,         (ContentDownloadError::type)DownloadError::HttpError              },
        { EA::Patch::kEAPatchErrorIdHttpSSLSetupFailure,        (ContentDownloadError::type)DownloadError::HttpError              },
        { EA::Patch::kEAPatchErrorIdHttpSSLCertInvalid,         (ContentDownloadError::type)DownloadError::HttpError              },
        { EA::Patch::kEAPatchErrorIdHttpSSLCertHostMismatch,    (ContentDownloadError::type)DownloadError::HttpError              },
        { EA::Patch::kEAPatchErrorIdHttpSSLCertUntrusted,       (ContentDownloadError::type)DownloadError::HttpError              },
        { EA::Patch::kEAPatchErrorIdHttpSSLCertRequestFailure,  (ContentDownloadError::type)DownloadError::HttpError              },
        { EA::Patch::kEAPatchErrorIdHttpSSLConnectionFailure,   (ContentDownloadError::type)DownloadError::HttpError              },
        { EA::Patch::kEAPatchErrorIdHttpSSLUnknown,             (ContentDownloadError::type)DownloadError::HttpError              },
        { EA::Patch::kEAPatchErrorIdHttpDisconnect,             (ContentDownloadError::type)DownloadError::HttpError              },
        { EA::Patch::kEAPatchErrorIdHttpUnknown,                (ContentDownloadError::type)DownloadError::HttpError              },
        { EA::Patch::kEAPatchErrorIdHttpBusyFailure,            (ContentDownloadError::type)DownloadError::HttpError              }
    };

    static Origin::Downloader::ContentDownloadError::type ConvertEAPatchErrorToContentDownloadError(EA::Patch::EAPatchErrorId eaPatchErrorId)
    {
        // This is a slowish linear search, but in practice it's rarely ever called.
        for(size_t i = 0; i < EAArrayCount(gErrorArray); i++)
        {
            if(gErrorArray[i].mEAPatchErrorId == eaPatchErrorId)
                return gErrorArray[i].mDownloaderErrorId;
        }

        return (ContentDownloadError::type)PatchBuilderError::UNKNOWN;
    }

    static EA::Patch::EAPatchErrorId ConvertContentDownloadErrorToEAPatchError(Origin::Downloader::ContentDownloadError::type downloadErrorType)
    {
        // This is a slowish linear search, but in practice it's rarely ever called.
        for(size_t i = 0; i < EAArrayCount(gErrorArray); i++)
        {
            if(gErrorArray[i].mDownloaderErrorId == downloadErrorType)
                return gErrorArray[i].mEAPatchErrorId;
        }

        return EA::Patch::kEAPatchErrorIdUnknown;
    }


    void PatchBuilder::HandleEAPatchError(Job* pJob, EA::Patch::ErrorClass errorClass, EA::Patch::SystemErrorId systemErrorId, 
                                            EA::Patch::EAPatchErrorId eaPatchErrorId, const char* pFile, int line)
    {
        EA::Patch::Error eaPatchError(errorClass, systemErrorId, eaPatchErrorId, pFile, line);
        Origin::Downloader::DownloadErrorInfo errInfo;
        errInfo.mSourceFile = pFile;
        errInfo.mSourceLine = line;
        HandleEAPatchError(pJob, eaPatchError, errInfo);
    }


    void PatchBuilder::HandleEAPatchError(Job* pJob, const EA::Patch::Error& eaPatchError, const Origin::Downloader::DownloadErrorInfo& errInfo)
    {
        // This will set us into an error state which will cause further processing to stop.
        HandleError(eaPatchError); // To consider: Is this going to result in a redundant debug printf of the error description?

        // Tell our owner about this.
        PropogateEAPatchError(pJob, eaPatchError, errInfo);
    }


    void PatchBuilder::HandleDownloaderError(Job* pJob, const Origin::Downloader::DownloadErrorInfo& errInfo)
    {
        // We create an EAPatch error for this solely for the purpose of putting us in an error state.
        // The actual reporting of the error still still use the DownloadErrorInfo.
        EA::Patch::Error eaPatchError((EA::Patch::ErrorClass)EA::Patch::kECFatal, 
                                      (EA::Patch::SystemErrorId)errInfo.mErrorCode,
                                      ConvertContentDownloadErrorToEAPatchError((Origin::Downloader::ContentDownloadError::type)errInfo.mErrorType), 
                                      errInfo.mSourceFile.toUtf8().constData(), (int)errInfo.mSourceLine);

        // This will set us into an error state which will cause further processing to stop.
        HandleError(eaPatchError); // To consider: Is this going to result in a redundant debug printf of the error description?

        // Tell our owner about this.
        PropogateDownloaderError(pJob, eaPatchError, errInfo);
    }


    void PatchBuilder::HandleDownloaderError(Job* pJob, qint32 downloaderErrorType, qint32 downloaderErrorCode, const char* pFile, int line)
    {
        // We just pass this on to the other version of HandleDownloaderError.
        Origin::Downloader::DownloadErrorInfo errInfo;

        errInfo.mErrorType  = downloaderErrorType;
        errInfo.mErrorCode  = downloaderErrorCode;
        errInfo.mSourceFile = pFile;
        errInfo.mSourceLine = line;
        
        HandleDownloaderError(pJob, errInfo);
    }

    QString PatchBuilder::GetEAPatchContextString(const EA::Patch::Error& eaPatchError)
    {
        // mErrorType / mErrorCode
        EA::Patch::ErrorClass     eaPatchErrorClass;
        EA::Patch::SystemErrorId  eaPatchSystemErrorId;
        EA::Patch::EAPatchErrorId eaPatchErrorId;
        eaPatchError.GetErrorId(eaPatchErrorClass, eaPatchSystemErrorId, eaPatchErrorId);

        // We want to preserve the internal EAPatch file/line info in the context field
        // mSourceFile / mSourceLine
        EA::Patch::String eaPatchSourceFile;
        int               eaPatchSourceLine;
        eaPatchError.GetErrorSource(eaPatchSourceFile, eaPatchSourceLine);
        EA::Patch::String eaPatchErrorContext;
        EA::Patch::String eaPatchErrorString;
        eaPatchError.GetErrorContext(eaPatchErrorContext);
        eaPatchError.GetErrorString(eaPatchErrorString);
        return QString("EAPatch error: %1 [Class: %2 SysErr: %3 ErrId: %4] at (%5:%6) Ctx: %7").arg(eaPatchErrorString.c_str()).arg((int)eaPatchErrorClass).arg((int)eaPatchSystemErrorId).arg((int)eaPatchErrorId).arg(eaPatchSourceFile.c_str()).arg((quint32)eaPatchSourceLine).arg(eaPatchErrorContext.c_str());
    }
    
    // We report the error to our owner. Since we are a sub-module within Origin::Downloader, we don't  
    // try to gracefully deal with the error ourselves but rather let our owner decide what to do.
    void PatchBuilder::PropogateEAPatchError(Job* pJob, const EA::Patch::Error& eaPatchError, const Origin::Downloader::DownloadErrorInfo& errInfo)
    {
        // The File/Line info needs to come from the caller location, we will include the EAPatchError details in the context       
        Origin::Downloader::DownloadErrorInfo propogatedErrInfo = errInfo;

        // mErrorType / mErrorCode
        EA::Patch::ErrorClass     eaPatchErrorClass;
        EA::Patch::SystemErrorId  eaPatchSystemErrorId;
        EA::Patch::EAPatchErrorId eaPatchErrorId;
        eaPatchError.GetErrorId(eaPatchErrorClass, eaPatchSystemErrorId, eaPatchErrorId);
        propogatedErrInfo.mErrorType = ConvertEAPatchErrorToContentDownloadError(eaPatchErrorId);
        propogatedErrInfo.mErrorCode = eaPatchSystemErrorId;

        // We want to preserve the internal EAPatch file/line info in the context field
        propogatedErrInfo.mErrorContext = GetEAPatchContextString(eaPatchError);

        // EAPatch also has a function called GetErrorContext(String& sErrorContext) which returns a string
        // that's relevant to the error and provides more info. For example, if the error is a file open 
        // error, GetErrorContext returns the file path. There isn't currently an analogue to this in Downloader.

        // Call the primary PropogateError function.
        PropogateDownloaderError(pJob, eaPatchError, propogatedErrInfo);
    }


    // All of the Error functions eventually call this one, so this is where we do things that are 
    // common to handling of all error sources.
    void PatchBuilder::PropogateDownloaderError(Job* pJob, const EA::Patch::Error& eaPatchError, const Origin::Downloader::DownloadErrorInfo& errInfo)
    {
        if(pJob) // If the error was associated with a specific job...
        {
            if(!pJob->HasError())                       // Make sure pJob is set to have the error.
                pJob->HandleError(eaPatchError, false); // False means don't do a printf of the error state, as we're just copying the error into the job.

            CancelJob(pJob);
        }

        // Propagate the error up to the protocol
        Origin::Downloader::DownloadErrorInfo propogatedErrInfo = errInfo;
        emit PatchFatalError(propogatedErrInfo); // Generate PatchFatalError signal. TODO: Map the EAPatch errors into the DownloadErrors, also supply a 'result code' if possible
    }


    /*
    void PatchBuilder::PropogateDownloaderError(Job* pJob, qint32 downloaderErrorType, qint32 downloaderErrorCode, const char* pFile, int line)
    {
        Origin::Downloader::DownloadErrorInfo errInfo;

        errInfo.mErrorType  = downloaderErrorType;
        errInfo.mErrorCode  = downloaderErrorCode;
        errInfo.mSourceFile = pFile;
        errInfo.mSourceLine = line;

        PropogateDownloaderError(pJob, eaPatchError, errInfo);
    }
    */

    void PatchBuilder::CancelJob(Job* pJob)
    {
        CancelJobDownloads(pJob);

        // Close any open files, etc.
        pJob->Cleanup();

        // We won't be needing any temp files any more.
        if(!RemoveAllTempFiles(pJob))
        {
            // Error occurred. To do: We may need to cancel the entire process.
        }

        // To consider: Should we remove the Job from our job list or leave it there in an error state? 
        // Currently we leave it there in an error state, as it might be useful for debug/trace information later.
        // The caller can always remove it if needed.
    }


    void PatchBuilder::CancelJobDownloads(Job* pJob)
    {
        if(pJob->mDiffFileDownloadData.mRequestId)
        {
            _downloadSession->closeRequest(pJob->mDiffFileDownloadData.mRequestId);
            _downloadRequestDataMap.remove(pJob->mDiffFileDownloadData.mRequestId);
            pJob->mDiffFileDownloadData.mRequestId = 0;
        }

        for(DownloadRequestDataList::iterator it = pJob->mFileDownloadDataList.begin(); it != pJob->mFileDownloadDataList.end(); ++it)
        {
            DownloadRequestData& drd = *it;

            _downloadSession->closeRequest(drd.mRequestId);
            _downloadRequestDataMap.remove(drd.mRequestId);
            drd.mRequestId = 0;
        }
    }


    bool PatchBuilder::ShouldContinue()
    {
        EAReadBarrier();
        return !_shutdown && !HasError();
    }


    PatchBuilder::State PatchBuilder::GetState() const
    {
        EAReadBarrier();
        return _state;
    }


    void PatchBuilder::SetState(PatchBuilder::State state)
    {
        _state = state;
        EAWriteBarrier();
    }


    static const char8_t* kPatchBuilderStateTable[] = 
    {
        "None",
        "Begin",
        "Downloading PatchDiff files",
        "Building destination file info",
        "Downloading destination files",
        "Copying local files",
        "Renaming destination files",
        "End"
    };

    const char8_t* PatchBuilder::GetStateString(State state)
    {
        EA_COMPILETIME_ASSERT((kStateNone == 0) && (kStateEnd == (EAArrayCount(kPatchBuilderStateTable) - 1))); // This ensures that the enum and the string array are in sync.

        if(((int)state < 0) || ((int)state >= (int)EAArrayCount(kPatchBuilderStateTable)))
            state = kStateNone;

        return kPatchBuilderStateTable[state];
    }


}//namespace Downloader

}//namespace Origin
