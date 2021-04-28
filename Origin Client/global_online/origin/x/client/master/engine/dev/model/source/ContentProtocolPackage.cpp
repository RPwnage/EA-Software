
#include "ContentProtocolPackage.h"
#include "services/downloader/Common.h"
#include "engine/downloader/ContentServices.h"
#include "engine/utilities/ContentUtils.h"
#include "engine/debug/DownloadDebugDataManager.h"
#include "services/common/DiskUtil.h"
#include "services/downloader/DownloadService.h"
#include "services/downloader/LogHelpers.h"
#include "services/log/LogService.h"
#include "services/debug/DebugService.h"
#include "services/downloader/Timer.h"
#include "services/settings/SettingsManager.h"
#include "services/downloader/UnpackStreamFile.h"
#include "EAIO/FnMatch.h"

#include <QDir>
#include <QFile>
#include <QDebug>
#include <QWaitCondition>
#include <QObject>

#include <set>

#if defined(ORIGIN_MAC)
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "ZipFileInfo.h"
#include "BigFileInfo.h"
#include "PackageFile.h"
#include "PatchBuilder.h"
#include "services/downloader/FilePath.h"
#include "services/platform/envutils.h"
#include "services/downloader/StringHelpers.h"
#include "DateTime.h"
#include "services/downloader/DownloaderErrors.h"
#include "TelemetryAPIDLL.h"
#include "services/platform/PlatformService.h"

#define MAX_RESTART_COUNT 1
#define MAX_RETRY_COUNT 16

#define PKGLOG_PREFIX "[Game Title: " << getGameTitle() << "][ProductID:" << mProductId << "]"

// Wait up to a minute before giving up on waiting for the protocol
// to clean itself up gracefully
#define CANCEL_PAUSE_TIMEOUT (60*1000)

// Don't diff update files with a compressed size smaller than this
#define DIFF_UPDATE_MIN_SIZE_THRESHOLD (64*1024)

// Number of refresh cycles of inactivity that a group can have before being demoted from active status
#define PRIORITY_GROUP_INACTIVITY_COUNT 20

// Number of milliseconds between re-sampling the instantaneous download speeds
#define DOWNLOAD_SPEED_SAMPLING_INTERVAL 1000

// Number of milliseconds between periodic saves of the CRC map (currently 60s)
#define PERIODIC_CRC_MAP_SAVE_INTERVAL 60000

namespace Origin
{
namespace Downloader
{
    ContentProtocolPackage::ContentProtocolPackage(const QString& productId, const QString& cachePath, const QString& crcMapPrefix, Services::Session::SessionWRef session) :
        IContentProtocol(productId, session),
        _cachePath(cachePath),
        _crcMapPrefix(crcMapPrefix),
        _packageType(ContentPackageType::Unknown),
        _enableITOCRCErrors(false),
        _skipCrcVerification(false),
        mTransferStatsDecompressed(100),
        mTransferStatsDownload(100),
        mAverageTransferRateDecompressed(50), // More smoothing on the decompressed rate, which is used for ETA
        mAverageTransferRateDownload(20),
        mLastUpdateTick(0),
        mLastCRCSaveTick(0),
        mTransferStats("Package Protocol", productId),
        mPatchBuilderTransferStats("Patch Builder", productId),
        _protocolForcePaused(false),
        _protocolReady(false),
		_protocolShutdown(false),
        _dataSaved(0),
        _dataToSave(0),
        _dataAdded(0),
        _dataDownloaded(0),
        _dataToDownload(0),
        _filesToSave(0),
        _filesPatchRejected(0),
        _fIsUpdate(false),
        _fIsRepair(false),
        _fSafeCancel(false),
        _fIsDynamicDownloadingAvailable(false),
        _fIsDiffUpdatingAvailable(false),
        _patchBuilder(NULL),
        _patchBuilderInitialized(false),
        _packageFileDirectory(NULL),
        _crcMap(NULL),
        _crcMapVerifyProxy(NULL),
		_downloadSession(NULL),
        _unpackStream(NULL),
        _anyPriorityGroupsUpdated(false),
        _fSteppedDownloadEnabled(false),
        _allRequiredGroupsDownloaded(false),
        _checkDiscTimer(this),
		_cancelPauseTimer(this),
        _progressUpdateTimer(this),
        _requestsCompleted(0),
        _numDiscs(1),
		_currentDisc(0),
        _requestedDisc(0),
        _currentDiscSize(0),
        _discEjected(false),
        _activeSyncRequestCount(0)
    {
        // Note: the directory must have a separator on the end or the package file code gets confused
        // This is relatively safe since Windows collapses duplicate separators
		_cachePath = _cachePath.trimmed();	// remove whitespace from the ends (fixes EBIBUGS-18058)
		if (!_cachePath.endsWith(QDir::separator()))
            _cachePath.append(QDir::separator());

        // Clean up and fix the slashes in the metadata path
        _crcMapPrefix = _crcMapPrefix.trimmed();
        _crcMapPrefix.replace("\\","/");
        if (_crcMapPrefix.startsWith("/"))
            _crcMapPrefix.remove(0,1);

        // We don't want just an empty slash, that will screw us up
        if (_crcMapPrefix == "/")
            _crcMapPrefix = "";
        if (!_crcMapPrefix.isEmpty() && !_crcMapPrefix.endsWith("/"))
            _crcMapPrefix.append("/");

        //we only want to hook this up once
        ORIGIN_VERIFY_CONNECT_EX(&_checkDiscTimer, SIGNAL(timeout()), this, SLOT(onCheckDiskTimeOut()),Qt::UniqueConnection);
		ORIGIN_VERIFY_CONNECT_EX(&_cancelPauseTimer, SIGNAL(timeout()), this, SLOT(onPauseCancelTimeOut()), Qt::UniqueConnection);
        ORIGIN_VERIFY_CONNECT_EX(&_progressUpdateTimer, SIGNAL(timeout()), this, SLOT(onProgressUpdateTimeOut()), Qt::QueuedConnection);
    }

    ContentProtocolPackage::~ContentProtocolPackage()
    {
        ClearRunState();
    }

    void ContentProtocolPackage::Initialize( const QString& url, const QString& target, bool isPhysicalMedia )
    {
        // Ensure this can only be called asynchronously
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(QString, url), Q_ARG(QString, target), Q_ARG(bool, isPhysicalMedia));
        
        if(Origin::Services::readSetting(Origin::Services::SETTING_LogDownload))
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << " Initializing this protocol with URL (" << url << ").";

        // Lets make sure we can initialize the protocol
        //
        switch(protocolState())
        {
            case kContentProtocolInitializing:
            {
                ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Already initializing this protocol with URL (" << Downloader::LogHelpers::logSafeDownloadUrl(_url) << "), cannot reinitialize.";
                return;
            }
            case kContentProtocolInitialized:
            case kContentProtocolStarting:
            case kContentProtocolResuming:
            case kContentProtocolRunning:
            case kContentProtocolPausing:
            case kContentProtocolCanceling:
            {
                ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Protocol is in a state [" << protocolStateString(protocolState()) << "] where it cannot be initialized.";
                return;
            }            
            case kContentProtocolPaused:
            case kContentProtocolUnknown:
            case kContentProtocolCanceled:
            case kContentProtocolFinished:
            case kContentProtocolError:
            {
                // these are the only states where we can actually successfully initialize.
                setProtocolState(kContentProtocolInitializing);
                break;
            }
            default:
                ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Protocol is in an unhandled state [" << protocolStateString(protocolState()) << "] for initialization check.";
                return;
        }

        _physicalMedia = isPhysicalMedia;
        _enableITOCRCErrors = Origin::Services::readSetting(Origin::Services::SETTING_EnableITOCRCErrors, Origin::Services::Session::SessionRef());
        _fSteppedDownloadEnabled = Origin::Services::readSetting(Origin::Services::SETTING_EnableSteppedDownload, Origin::Services::Session::SessionRef());

		//if this is ITO and we are not on disc one lets figure out the correct URL
		if(_physicalMedia && (_currentDisc != 0))
		{
			_url = getPathForDisc(_currentDisc);
		}
		else
		{
			_url = url; 
		}

        SetUnpackDirectory(target);

        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Initialize( " << Downloader::LogHelpers::logSafeDownloadUrl(_url) << " )";

        // Start the initialization
        InitializeStart();
    }

    void ContentProtocolPackage::SetUnpackDirectory(const QString& dir)
    {
        _unpackPath = dir;
        // Note: the directory must have a separator on the end or the package file code gets confused
        // This is relatively safe since Windows collapses duplicate separators
        if (!_unpackPath.endsWith(QDir::separator()))
            _unpackPath.append(QDir::separator());
    }

    void ContentProtocolPackage::SetSkipCrcVerification(bool skipCrcVerification)
    {
        _skipCrcVerification = skipCrcVerification;
    }

    void ContentProtocolPackage::SetContentTagBlock(QMap<QString,QString> contentTagBlock)
    {
        _contentTagBlock = contentTagBlock;
    }

    void ContentProtocolPackage::SetCrcMapKey(const QString& key)
    {
        if (key.isEmpty())
        {
            ORIGIN_LOG_ERROR << "CRC map key was empty for content: " << mProductId; 
        }
        _crcMapKey = key;
    }

    void ContentProtocolPackage::Start()
    {
        // Ensure this can only be called asynchronously
        ASYNC_INVOKE_GUARD;

        if (_protocolForcePaused)
        {
            ORIGIN_LOG_EVENT << "Unable to start after forced pause.";
            ClearRunState();
            return;
        }

        if (!_protocolReady || _protocolShutdown)
        {
            ORIGIN_ASSERT(false);
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Can't start.  Protocol not ready.";
            
            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = ProtocolError::CommandInvalid;
            errorInfo.mErrorCode = 0;
            emit (IContentProtocol_Error(errorInfo));
            return;
        }

        if (protocolState() == kContentProtocolRunning || protocolState() == kContentProtocolStarting || protocolState() == kContentProtocolResuming)
        {
            ORIGIN_LOG_WARNING << PKGLOG_PREFIX << "Can't start.  Protocol already running.";
            
            // This has no effect
            return;
        }

        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Start()";

        // Mark ourselves as starting
        setProtocolState(kContentProtocolStarting);

        //_unpack Path may have changed after the download started (via ITO Prepare Download) so we want to wait til just before transfer to set up the unpackstream
        // Create an unpack stream
        _unpackStream = UnpackService::GetUnpackStream(mProductId, _unpackPath, _contentTagBlock);
        ORIGIN_VERIFY_CONNECT_EX(_unpackStream, SIGNAL(UnpackFileChunkProcessed(UnpackStreamFileId, qint64, qint64, bool)), this, SLOT(onUnpackFileChunkProcessed(UnpackStreamFileId, qint64, qint64, bool)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(_unpackStream, SIGNAL(UnpackFileError(UnpackStreamFileId, Origin::Downloader::DownloadErrorInfo)), this, SLOT(onUnpackFileError(UnpackStreamFileId, Origin::Downloader::DownloadErrorInfo)), Qt::QueuedConnection);

        //update the destination directory
        _packageFileDirectory->SetDestinationDirectory(_unpackPath);

        // Do the actual work
        TransferStart();
    }

    void ContentProtocolPackage::Resume()
    {
        ASYNC_INVOKE_GUARD;

        if (!_protocolReady || _protocolShutdown)
        {
            ORIGIN_ASSERT(false);
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Can't resume.  Protocol " << (_protocolShutdown ? "was shutdown." : "not ready.");

            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = ProtocolError::CommandInvalid;
            errorInfo.mErrorCode = 0;
            emit (IContentProtocol_Error(errorInfo));
            return;
        }

        if (protocolState() == kContentProtocolInitializing || protocolState() == kContentProtocolRunning || protocolState() == kContentProtocolStarting || protocolState() == kContentProtocolResuming)
        {
            ORIGIN_LOG_WARNING << PKGLOG_PREFIX << "Can't resume.  Protocol already running.";
            
            // This has no effect
            return;
        }

        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Resume()";

        // Create an unpack stream
        _unpackStream = UnpackService::GetUnpackStream(mProductId, _unpackPath, _contentTagBlock);
        ORIGIN_VERIFY_CONNECT_EX(_unpackStream, SIGNAL(UnpackFileChunkProcessed(UnpackStreamFileId, qint64, qint64, bool)), this, SLOT(onUnpackFileChunkProcessed(UnpackStreamFileId, qint64, qint64, bool)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(_unpackStream, SIGNAL(UnpackFileError(UnpackStreamFileId, Origin::Downloader::DownloadErrorInfo)), this, SLOT(onUnpackFileError(UnpackStreamFileId, Origin::Downloader::DownloadErrorInfo)), Qt::QueuedConnection);


        // Do the work
        ResumeAll();
    }
    void ContentProtocolPackage::Pause(bool forcePause)
    {
        // Set our shutdown flag
        _protocolShutdown = true;

        // Make sure we only do this once, because the Pause will execute again 
        // on the protocol thread with the same forcePause parameter
        if(!_protocolForcePaused)
        {
            _protocolForcePaused = forcePause;

            if(_protocolForcePaused && _downloadSession != NULL)
            {
                _downloadSession->cancelSyncRequests();
            }
        }

        // Everything after this will happen on another thread
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(bool, forcePause));

        // Make sure we're not in a busy state
        if (protocolState() == kContentProtocolPausing || protocolState() == kContentProtocolCanceling)
        {
            return;
        }

        if (protocolState() != kContentProtocolRunning)
        {
            ORIGIN_LOG_WARNING << PKGLOG_PREFIX << "Can't pause.  Protocol not running.";
            
            // We're not running, so just give up and say we're paused anyway
            setProtocolState(kContentProtocolPaused);
            emit (Paused());

            return;
        }

        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Pause()";

        // Update the state
        setProtocolState(kContentProtocolPausing);
		_cancelPauseTimer.start(CANCEL_PAUSE_TIMEOUT);

        SuspendAll();

        // If we have a patch builder object running, tell it to pause
        if (_patchBuilder != NULL)
        {
            _patchBuilder->Pause();
        }

		CheckTransferState();
    }

    void ContentProtocolPackage::Finalize()
    {
        ASYNC_INVOKE_GUARD;

        // Make sure we're ready to go
        if (!_protocolReady)
        {
            ORIGIN_ASSERT(false);
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Can't finalize, protocol has not been initialized.";
            
            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = ProtocolError::CommandInvalid;
            errorInfo.mErrorCode = 0;
            emit IContentProtocol_Error(errorInfo);
            return;
        }

        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Finalize()";

        // Try to finalize
        int errorCode = 0;
        if (FinalizeDestageFiles(errorCode))
        {
            // Clear all state
            ClearRunState();

            emit Finalized();
        }
        else
        {
            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = ProtocolError::DestageFailed;
            errorInfo.mErrorCode = errorCode;
            emit IContentProtocol_Error(errorInfo);
        }
    }

    void ContentProtocolPackage::Cancel()
    {
        // Everything after this will happen on another thread
        ASYNC_INVOKE_GUARD;

        // If a synchronous network request is active we must delay the cancel
        // until it is finished.
        int count = _activeSyncRequestCount.loadAcquire();
        if (count > 0)
        {
            ORIGIN_LOG_EVENT << "Delay cancel until after an asynchronous network request.";
            DelayedCancel* delayedCancel = new DelayedCancel(this, _delayedCancelWaitCondition);
            delayedCancel->setAutoDelete(true);
            QThreadPool::globalInstance()->start(delayedCancel);
            return;
        }

        // Set our shutdown flag
        _protocolShutdown = true;

        // Make sure we're not in a busy state
        if (protocolState() == kContentProtocolPausing || protocolState() == kContentProtocolCanceling)
        {
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Cannot cancel protocol that is in pausing or canceling state.";
            return;
        }

        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Cancel()";

        if(_fIsRepair)
        {
            if(_dataToSave == 0)
            {
                // TELEMETRY: User has canceled a repair job before or during the CRC calculation.
                GetTelemetryInterface()->Metric_REPAIRINSTALL_VERIFYCANCELED(mProductId.toUtf8().data());
            }
            else
            {
                // TELEMETRY: User has canceled a repair job after the CRC calculation.
                GetTelemetryInterface()->Metric_REPAIRINSTALL_REPLACECANCELED(mProductId.toUtf8().data());
            }
        }

        // If we're running, first we pause
        if (protocolState() == kContentProtocolRunning)
        {
            // Update the state
            setProtocolState(kContentProtocolCanceling);
			_cancelPauseTimer.start(CANCEL_PAUSE_TIMEOUT);

            SuspendAll();

            // If we have a patch builder object running, tell it to cancel
            if (_patchBuilder != NULL)
            {
                _patchBuilder->Cancel();
            }
        }
        else
        {
            // We're not running, just kill it all
            Cleanup();
        }
    }

	void ContentProtocolPackage::onPauseCancelTimeOut()
	{
		if(protocolState() != kContentProtocolPausing && protocolState() != kContentProtocolCanceling)
			return;

		ORIGIN_LOG_ERROR << PKGLOG_PREFIX << " " << (protocolState() == kContentProtocolPausing ? "Pause" : "Cancel") << " timed out, forcing the issue.";
		ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Timed out protocol state: [DownloadSession running: " << (_downloadSession != NULL && _downloadSession->isRunning() ? "yes" : "no") << "] [Unpack Metadata Count: " << _unpackFiles.size() << "] [Download Metadata Count: " << _downloadReqMetadata.size() << "]";
	
		ClearTransferState();
	}

    void ContentProtocolPackage::onProgressUpdateTimeOut()
    {
        if (protocolState() != kContentProtocolRunning)
        {
            if (_progressUpdateTimer.isActive())
            {
                _progressUpdateTimer.stop();
            }
            return;
        }

        // Update dynamic download group stats
        UpdatePriorityGroupAggregateStats();

        // Trim outdated download rate samples (max age 10000ms)
        // This allows the download rate to use a larger samples window but react quickly when the flow stops
        mTransferStatsDecompressed.trimSamples(10000);
        mTransferStatsDownload.trimSamples(10000);

        // Send progress (even if it may not have changed)
        SendProgressUpdate();

        // Save CRC map periodically
        SaveCRCMapPeriodically();
    }

    void ContentProtocolPackage::SendProgressUpdate()
    {
        // Only re-sample the current speed once a second, even if we update progress more often
        qint64 thisTick = GetTick();
        if ((thisTick - mLastUpdateTick) > DOWNLOAD_SPEED_SAMPLING_INTERVAL)
        {
            // Update the last tick
            mLastUpdateTick = thisTick;

            // Add the current instantaneous BPS into the moving averages
            mAverageTransferRateDecompressed.Add(mTransferStatsDecompressed.averageBPS());
            mAverageTransferRateDownload.Add(mTransferStatsDownload.averageBPS());
        }

        // Send the progress update signal
        quint64 smoothedAverageDownloadBps = (quint64)mAverageTransferRateDownload.GetAverage();
        quint64 estimatedTimeRemaining = (smoothedAverageDownloadBps > 0) ? (_dataToDownload-_dataDownloaded) / smoothedAverageDownloadBps : 0;

        //ORIGIN_LOG_DEBUG << "smoothedAverageDownloadBps:" << smoothedAverageDownloadBps << "\n";
        emit (TransferProgress(_dataDownloaded, _dataToDownload, smoothedAverageDownloadBps, estimatedTimeRemaining));
    }

    void ContentProtocolPackage::SaveCRCMapPeriodically()
    {
        if (!_crcMap)
            return;

        if (mLastCRCSaveTick == 0)
        {
            mLastCRCSaveTick = GetTick();
            return;
        }

        qint64 thisTick = GetTick();
        if ((thisTick - mLastCRCSaveTick) > PERIODIC_CRC_MAP_SAVE_INTERVAL)
        {
            // Update the last tick
            mLastCRCSaveTick = thisTick;

            if (_crcMap->HasChanges())
            {
                if (!_crcMap->Save())
                {
                    ORIGIN_LOG_WARNING << PKGLOG_PREFIX << "Periodic CRC Map save failure.";
                }
            }
        }
    }

    void ContentProtocolPackage::Retry()
    {
        ASYNC_INVOKE_GUARD;

        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Retry()";

        if( _discEjected )
        {
            // show 'insert disc' dialog
            emit(CannotFindSource(_requestedDisc + 1, _numDiscs, -1, true));
            //every 5 seconds check for the disc
            _checkDiscTimer.start(5000);
        }
        else
        {
            ChangeDiscIfNeeded();
        }
        return;
    }

    void ContentProtocolPackage::InstallDynamicChunk(int priorityGroupId)
    {
        // Everything after this will happen on another thread
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(int, priorityGroupId));

        if (!_protocolReady)
        {
            // TODO: Handle this error
            return;
        }

        // This is only valid while we're running or finished (and waiting to destage)
        // However, for an initial download, once the transfer has finished, everything is going to get destaged anyway
        if(protocolState() != kContentProtocolRunning && protocolState() != kContentProtocolFinished)
			return;

        // Do we even know about this group id?
        if (!_downloadReqPriorityGroups.contains(priorityGroupId))
            return;

        // Destage the files in this priority group
        int destageError = 0;
        FinalizeDestageFilesForGroup(priorityGroupId, destageError);
        Q_UNUSED(destageError);
    }

    void ContentProtocolPackage::GetContentLength()
    {
        // Ensure this can only be called asynchronously
        ASYNC_INVOKE_GUARD;

        // If it hasn't yet been calculated, figure out the content length
        if (_dataToSave == 0)
        {
            if (_protocolForcePaused)
            {
                ORIGIN_LOG_EVENT << "Unable to retrieve content length after forced pause.";
                ClearRunState();
                return;
            }

            if (!ComputeDownloadJobSizeBegin())
            {
                ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Unable to setup package file entry map";

                ClearRunState();
                
                CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
                errorInfo.mErrorType = UnpackError::IOCreateOpenFailed;
                errorInfo.mErrorCode = Services::PlatformService::lastSystemResultCode();
                emit(IContentProtocol_Error(errorInfo));
                return;
            }
        }
        else
        {
            // Skip the asynchronous bit because we already have the content length
            FinishGetContentLength();
        }

        // DO NOT put any code here, ComputeDownloadJobSizeBegin can return asynchronously
    }

    void ContentProtocolPackage::FinishGetContentLength()
    {
        if (_protocolReady)
        {
            ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "GetContentLength() - Returned: " << _dataToSave;

            emit(ContentLength(_dataToSave, _dataToDownload));
        }
        else
        {
            ORIGIN_ASSERT(false);
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Protocol is not initialized, cannot request content length.";
            
            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = ProtocolError::CommandInvalid;
            errorInfo.mErrorCode = 0;
            emit(IContentProtocol_Error(errorInfo));
            return;
        }
    }

    void ContentProtocolPackage::SetUpdateFlag(bool flag)
    {
        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Setting update flag to " << flag;
        _fIsUpdate = flag;
    }

    void ContentProtocolPackage::SetRepairFlag(bool flag)
    {
        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Setting repair flag to " << flag;
        _fIsRepair = flag;
    }

    void ContentProtocolPackage::SetSafeCancelFlag(bool flag)
    {
        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Setting safe cancel flag to " << flag;
        _fSafeCancel = flag;
    }

    void ContentProtocolPackage::SetDifferentialFlag(bool flag, const QString& syncPackageUrl)
    {
        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Setting differential update flag to " << flag;

        // If they want diff updating
        if (flag)
        {
            // If there is no sync package URL, we can't do it!
            if (syncPackageUrl.isEmpty())
            {
                flag = false;
            }
            else
            {
                _syncPackageUrl = syncPackageUrl;
            }
        }
        else
        {
            // No diff updating, clear the sync package URL
            _syncPackageUrl = "";
        }

        _fIsDiffUpdatingAvailable = flag;
    }

    void ContentProtocolPackage::SetDynamicDownloadFlag(bool flag)
    {
        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Setting dynamic download flag to " << flag;

        _fIsDynamicDownloadingAvailable = flag;
    }

    bool ContentProtocolPackage::IsStaged(const QString& archivePath, QString& stagedFilename, quint32& newFileCRC)
    {
        newFileCRC = 0;

        if (!_protocolReady)
        {
            ORIGIN_ASSERT(false);
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Protocol can't track staged files when not initialized.";
            return false;
        }

        // Retrieve the file from the central directory
        const PackageFileEntry *pEntry = _packageFileDirectory->GetEntryByName(archivePath);
        if (NULL == pEntry)
            return false;

		// make sure entry actually exists in the map, otherwise the meta data info will just be the default values and not valid
		if (!_packageFiles.contains(const_cast<PackageFileEntry*>(pEntry)))
			return false;

        // Retrieve the metadata
        PackageFileEntryMetadata& fileMetadata = _packageFiles[const_cast<PackageFileEntry*>(pEntry)];

        // If it isn't staged, bail out
        if (!fileMetadata.isStaged)
            return false;

        // Don't return it if it isn't complete
        if (fileMetadata.bytesSaved != pEntry->GetUncompressedSize())
            return false;

        // Get the actual staged filename
        QString tempStagedFilename = GetFileFullPath(GetStagedFilename(pEntry->GetFileName()));

        if (EnvUtils::GetFileExistsNative(tempStagedFilename))
        {
            newFileCRC = pEntry->GetFileCRC();
            stagedFilename = tempStagedFilename;
            return true;
        }

        return false;
    }

    bool ContentProtocolPackage::IsFileInJob(const QString& archivePath, quint32& newFileCRC)
    {
        newFileCRC = 0;

        if (!_protocolReady)
        {
            ORIGIN_ASSERT(false);
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Protocol can't track files when not initialized.";
            return false;
        }

        // We need the CRC map
        if (!_crcMap)
        {
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Protocol can't track files when not initialized. (no CRC Map)";
            return false;
        }

        // Retrieve the file from the central directory
        const PackageFileEntry *pEntry = _packageFileDirectory->GetEntryByName(archivePath);
        if (NULL == pEntry)
            return false;

        // If the file is excluded, it isn't modified
        if (!pEntry->IsIncluded() || !pEntry->IsFile())
            return false;

        newFileCRC = pEntry->GetFileCRC();

        // If the CRC map doesn't already track this entry, it's new
        QString sFilename = pEntry->GetFileName();
        //QString sStagedFilename = GetStagedFilename(sFilename);


        QString crcMapFilePath = GetCRCMapFilePath(sFilename);
        if (!_crcMap->Contains(crcMapFilePath))
            return true;

        // We're tracking this file, so see if the CRC matches
        const Downloader::CRCMapData crcData = _crcMap->GetCRC(crcMapFilePath);
        if (crcData.crc == pEntry->GetFileCRC())
            return false;

        // CRC didn't match
        return true;
    }

    void ContentProtocolPackage::GetSingleFile(const QString& archivePath, const QString& diskPath)
    {
        // Ensure this can only be called asynchronously
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(QString, archivePath), Q_ARG(QString, diskPath));

        if (_protocolForcePaused)
        {
            ORIGIN_LOG_EVENT << "Unable to retrieve network file after forced pause.";
            ClearRunState();
            return;
        }

        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "GetSingleFile( archivePath: " << archivePath << " , diskPath: " << diskPath << " )";
        
        if (!_protocolReady)
        {
            ORIGIN_ASSERT(false);
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Protocol is not initialized, cannot get single file: " << archivePath;
            
            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = ProtocolError::NotReady;
            errorInfo.mErrorCode = 0;
            emit (IContentProtocol_Error(errorInfo));
            return;
        }

        TransferSingleFile(archivePath, diskPath);
    }

    void ContentProtocolPackage::onCRCVerifyProgress(qint64 bytesProcessed, qint64 totalBytes, qint64 bytesPerSecond, qint64 estimatedSecondsRemaining)
    {
        bool shutdown = (protocolState() == kContentProtocolPausing || protocolState() == kContentProtocolCanceling);
        if (shutdown)
        {
            if (_crcMapVerifyProxy)
            {
                // Tell the verify to cancel
                _crcMapVerifyProxy->Cancel();
            }
        }

        emit NonTransferProgress(bytesProcessed, totalBytes, bytesPerSecond, estimatedSecondsRemaining);
    }

    void ContentProtocolPackage::onCRCVerifyCompleted()
    {
        // Dispose of our asynchronous completion object
        if (_crcMap && _crcMapVerifyProxy)
        {
            _crcMap->VerifyEnd(_crcMapVerifyProxy);
            _crcMapVerifyProxy = NULL;
        }

        // Continue the initialization
        InitializeVerified();
    }

    void ContentProtocolPackage::onPatchBuilderInitializeMetadataProgress(qint64 bytesProcessed, qint64 bytesTotal)
    {
        if (mPatchBuilderTransferStats.totalReceived() == 0)
        {
            mPatchBuilderTransferStats.onStart(bytesTotal, 0);
        }
        qint64 totalReceived = bytesProcessed - mPatchBuilderTransferStats.totalReceived();
        if (totalReceived < 0)
            return;
        mPatchBuilderTransferStats.onDataReceived(totalReceived);

        ORIGIN_LOG_DEBUG << PKGLOG_PREFIX << "Patch Builder Initialize Progress ; Processed: " << bytesProcessed << " Total: " << bytesTotal ;

        // Emit the computed progress statistics to the flow
        emit NonTransferProgress(bytesProcessed, bytesTotal, (qint64)mPatchBuilderTransferStats.bps(), mPatchBuilderTransferStats.estimatedTimeToCompletion(true));
    }

    // This function is a slot which PatchBuilder calls when certain files are not eligible (or non-optimal) for differential updating.
    // That situation might occur if a file's compressed size is smaller than the amount of uncompressed data we would otherwise download.
    // We primarily handle this situation by removing the job from the diff file list and updating the counters that would ordinarily be used
    // when a file is downloaded conventionally.  (Steps we skipped because we thought we would differentially update the file)
    void ContentProtocolPackage::onPatchBuilderInitializeRejectFiles(DiffFileErrorList rejectedFiles)
    {
        for (DiffFileErrorList::iterator it = rejectedFiles.begin(); it != rejectedFiles.end(); ++it)
        {
            DiffFileError &error = *it;

            // If this is a file we're tracking
            if (_diffPatchFiles.contains(error.first))
            {
                // Fetch the non-diff metadata
                PackageFileEntry* packageFileMetadata = _diffPatchFiles[error.first].packageFileEntry;
                if (_packageFiles.contains(packageFileMetadata))
                {
                    ++_filesPatchRejected;

                    PackageFileEntryMetadata& fileMetadata = _packageFiles[packageFileMetadata];

                    // Turn off the diff updating for this file
                    fileMetadata.isBeingDiffUpdated = false;
                    fileMetadata.diffId = -1;
                    fileMetadata.bytesSaved = 0;
                    fileMetadata.bytesDownloaded = 0;

                    // Fix the counters
                    _dataToSave += packageFileMetadata->GetUncompressedSize();
                    _dataToDownload += packageFileMetadata->GetCompressedSize();
                }

                // Remove the diff file metadata
                _diffPatchFiles.remove(error.first);

                ORIGIN_LOG_DEBUG << PKGLOG_PREFIX << "Removed file rejected by PatchBuilder: " << packageFileMetadata->GetFileName() << " Reason: " << (int)error.second;
            }
        }
    }

    void ContentProtocolPackage::onPatchBuilderInitialized(qint64 totalBytesToPatch)
    {
        _patchBuilderInitialized = true;

        mPatchBuilderTransferStats.onFinished();
        ComputeDownloadJobSizeEnd(true, totalBytesToPatch);
    }

    void ContentProtocolPackage::onPatchBuilderFileProgress(DiffFileId diffFileId, qint64 bytesProcessed, qint64 bytesTotal)
    {
        if (_diffPatchFiles.contains(diffFileId))
        {
            DiffPatchMetadata& diffFileMetadata = _diffPatchFiles[diffFileId];

            ORIGIN_LOG_DEBUG << PKGLOG_PREFIX << "Patch Builder Progress for File: " << diffFileMetadata.packageFileEntry->GetFileName() << " Processed: " << bytesProcessed << " Total: " << bytesTotal ;

            qint64 deltaBytes = bytesProcessed - diffFileMetadata.bytesBuilt;

            diffFileMetadata.bytesBuilt = bytesProcessed;
            diffFileMetadata.bytesToBuild = bytesTotal;

            if (deltaBytes > 0)
            {
                // Update the progress bars
                _dataSaved += deltaBytes;
                _dataDownloaded += deltaBytes;

                // Fire progress bar events if the job is running, otherwise we might be initializing (patch builder needs to report partial file progress)
                if (protocolState() == kContentProtocolRunning)
                {
                    mTransferStatsDecompressed.addSample(deltaBytes);
                    mTransferStatsDownload.addSample(deltaBytes);
                    mTransferStats.onDataReceived(deltaBytes);

                    if(_dataSaved == _dataToSave || checkToSendThrottledUpdate())
                    {
                        SendProgressUpdate();
                    }
                }
            }
        }
    }

    void ContentProtocolPackage::onPatchBuilderFileComplete(DiffFileId diffFileId)
    {
        if (_diffPatchFiles.contains(diffFileId))
        {
            DiffPatchMetadata& diffFileMetadata = _diffPatchFiles[diffFileId];

            if (diffFileMetadata.packageFileEntry)
            {
                PackageFileEntryMetadata& packageFileMetadata = _packageFiles[diffFileMetadata.packageFileEntry];

                // Make sure the file is actually there!
                QString fullStagedPath = GetFileFullPath(packageFileMetadata.diskFilename);
                bool fileExists = false;
                bool isSymlink = false;
                qint64 fileSize = 0;
                GetFileDetails(fullStagedPath, false, fileExists, fileSize, isSymlink);

                if (fileExists)
                {
                    // Update the bytes saved
                    diffFileMetadata.bytesBuilt = diffFileMetadata.bytesToBuild;
                    packageFileMetadata.bytesSaved = fileSize;
                    packageFileMetadata.bytesDownloaded = diffFileMetadata.packageFileEntry->GetCompressedSize();

                    ORIGIN_ASSERT(fileSize == diffFileMetadata.packageFileEntry->GetUncompressedSize());

                    ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Patch Builder COMPLETED for File: " << diffFileMetadata.packageFileEntry->GetFileName();

                    diffFileMetadata.isComplete = true;

                    // Update the CRC map
                    QString crcMapFilePath = GetCRCMapFilePath(packageFileMetadata.diskFilename);
                    _crcMap->AddModify(crcMapFilePath, diffFileMetadata.packageFileEntry->GetFileCRC(), diffFileMetadata.packageFileEntry->GetUncompressedSize(), _crcMapKey);

                    // Tag it with this job ID since we touched the file
                    _crcMap->SetJobId(crcMapFilePath, getJobId());
                }
                else
                {
                    ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Patch Builder could not find file on disk for file: " << diffFileMetadata.packageFileEntry->GetFileName() << " at full path: " << fullStagedPath;
                }
            }
            else
            {
                ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Patch Builder could not find PackageFileEntry for DiffID: " << (qint64)(diffFileMetadata.diffId);
            }
        }
    }

    void ContentProtocolPackage::onPatchBuilderComplete()
    {
        // Do something less dumb here later... probably we should be accumulating this info periodically
        _diffPatchFiles.clear();

        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Patch Builder Finished";

        // Send telemetry indicating that we successfully completed the patch building process
        GetTelemetryInterface()->Metric_DL_DIP3_SUCCESS(mProductId.toLocal8Bit().constData());

        // Check and see if we're done
        ScanForCompletion();
        CheckTransferState();
    }

    void ContentProtocolPackage::onPatchBuilderPaused()
    {
        // Do something less dumb here later... probably we should be accumulating this info periodically
        _diffPatchFiles.clear();

        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Patch Builder Paused";

        // Check and see if we're done
        ScanForCompletion();
        CheckTransferState();
    }

    void ContentProtocolPackage::onPatchBuilderCanceled()
    {
        // Do something less dumb here later... probably we should be accumulating this info periodically
        _diffPatchFiles.clear();

        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Patch Builder Canceled";

        // Check and see if we're done
        ScanForCompletion();
        CheckTransferState();
    }

    void ContentProtocolPackage::onPatchBuilderFatalError(Origin::Downloader::DownloadErrorInfo errorInfo)
    {
        // The PatchBuilder might have spit out a bunch of errors, but we only need/want to handle the first one, ignore the rest
        if (!_patchBuilder)
        {
            return;
        }

        if (!_patchBuilderInitialized)
        {
            mPatchBuilderTransferStats.onFinished();

            // Send telemetry indicating that we're canceling the patching process
            GetTelemetryInterface()->Metric_DL_DIP3_CANCELED(mProductId.toLocal8Bit().constData(), (uint32_t)errorInfo.mErrorType, (uint32_t)errorInfo.mErrorCode);

            // Abort using the PatchBuilder altogether, go finish computing the download job size manually
            ComputeDownloadJobSizeEnd(false, 0);
        }
        else
        {
            // Send telemetry indicating that we had a fatal patch builder error
            GetTelemetryInterface()->Metric_DL_DIP3_ERROR(mProductId.toLocal8Bit().constData(), (uint32_t)errorInfo.mErrorType, (uint32_t)errorInfo.mErrorCode, errorInfo.mErrorContext.toLocal8Bit().constData());

            // Kill the Patch Builder
            DestroyPatchBuilder();

            // Do something less dumb here later... but for now we need these to be gone so we can shut down
            _diffPatchFiles.clear();

            // Stop the protocol
            Pause();

            // We're running already, so this is a fatal error for us too
            emit(IContentProtocol_Error(errorInfo));
            return;
        }
    }

    void ContentProtocolPackage::onDownloadSessionInitialized(qint64 contentLength)
    {
        // If we are forced shutdown or otherwise, clear our state and exit here
        if(_protocolForcePaused || protocolState() != kContentProtocolInitializing)
        {
            ClearRunState();
            return;
        }

        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Download Session intialized; Content Length: " << contentLength;

        //the size of the current zip
        _currentDiscSize = contentLength;

		//store the disc size for multi disc packages
		_discIndexToDiscSizeMap[_currentDisc] = _currentDiscSize;

        InitializeConnected(contentLength);
    }

	void ContentProtocolPackage::onDownloadSessionShutdown()
	{
        // Kill the download session
        if (_downloadSession)
        {
            DownloadService::DestroyDownloadSession(_downloadSession);
        }
        _downloadSession = NULL;

		CheckTransferState();
	}

    void ContentProtocolPackage::onDownloadSessionInitializedForDiscChange(qint64 contentLength)
    {
        // If we are forced shutdown or otherwise, clear our state and exit here
        if(_protocolForcePaused || protocolState() != kContentProtocolInitializing)
        {
            ClearRunState();
            return;
        }

        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Download Session re-initialized for disc change; Content length of new disc: " << contentLength;

        _currentDiscSize = contentLength;

		//store the disc size for multi disc packages
		_discIndexToDiscSizeMap[_currentDisc] = _currentDiscSize;

		if(_currentDisc >= 1)
        {
            setProtocolState(kContentProtocolInitialized);
            ResumeAll();
        }
    }

    void ContentProtocolPackage::onDownloadSessionError(Origin::Downloader::DownloadErrorInfo errorInfo)
    { 
        bool emitError = false;
        bool wasInitializing = (protocolState() == kContentProtocolInitializing);

        if(!_protocolForcePaused)
        {
            ContentDownloadError::type errType = static_cast<ContentDownloadError::type>(errorInfo.mErrorType);
            QString errStringType(ErrorTranslator::ErrorString(errType));
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Download Session error ; Type: " << errorInfo.mErrorType << " (" << errStringType << ") Code: " << errorInfo.mErrorCode;

            QString errorStr = QString("type=%1;code=%2").arg(errorInfo.mErrorType).arg(errorInfo.mErrorCode);
            GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mProductId.toLocal8Bit().constData(), "onDownloadSessionError", errorStr.toUtf8().data(), errorInfo.mSourceFile.toLocal8Bit().constData(), errorInfo.mSourceLine);

            // Call the appropriate failure handler here depending on what we are doing
            if (protocolState() == kContentProtocolUnknown || protocolState() == kContentProtocolPaused || protocolState () == kContentProtocolInitializing )
            {
                setProtocolState(kContentProtocolUnknown);
                emitError = true;
            }
            else if (protocolState() == kContentProtocolRunning)
            {
                // Halt everything and report an error
                Pause();
                emitError = true;
            }
        }

        if (emitError)
        {
            if (errorInfo.mErrorType > 0)
            {
                if(_physicalMedia) // for ITO only
                {
                    bool discEjected = !IsDiscAvailable(_currentDisc) && !wasInitializing;
                    if( discEjected )
                    {
                        if(errorInfo.mErrorType == DownloadError::NotConnectedError)// happens when disc is ejected before 'prepare' is finished
                        {
                            // Just in case not all requests were allowed to finish with errors, which is what cleans up the transfer-related meta maps
                            // This forces the cleanup and resets the _protocolShutdown flag without having to wait for the pauseCancelTimer to expire
                            ClearRunState(false);
                        }

                        //  TELEMETRY:  Track when disk is ejected
                        GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mProductId.toLocal8Bit().constData(), "ITOEjectedDisk", mProductId.toUtf8().data(), __FILE__, __LINE__);

                        _discEjected = true;
                        errorInfo.mErrorType = ProtocolError::DiscEjected;
                    }
                    else if(errorInfo.mErrorType == DownloadError::FileRead)// happens when reading a bad disc
                    {
                        // When reading a bad disc, the 'IsDiscAvailable()' call above can take minutes to complete.
                        // This prevents the Pause() from completing fully (i.e., shut down the download session).
                        // If and when we get here, the _cancelPauseTimer may have already been expired but not yet
                        // acted upon, so let's force a cleanup and reset the _protocolShutdown flag. If we don't clean up now,
                        // the user may be able to 'Try Again' from the 'Read Disc Error' dialog without
                        // the download session being cleaned up first, resulting in EBIBUGS-20369.
                        ClearRunState(false);
                    }
                }

                emit (IContentProtocol_Error(errorInfo));
            }
            else
            {
                // errorInfo.mErrorType is invalid, so pass up some valid info.
                CREATE_DOWNLOAD_ERROR_INFO(validErrorInfo);
                validErrorInfo.mErrorType = ProtocolError::DownloadSessionFailure;
                validErrorInfo.mErrorCode = errorInfo.mErrorCode;
                emit (IContentProtocol_Error(validErrorInfo));
            }
        }
    }

    void ContentProtocolPackage::onDownloadRequestChunkAvailable(RequestId reqId)
    {
        if (!_downloadSession)
            return;

        // We have an early return here to cover the case where we need to shut everything down
        bool shutdown = (protocolState() == kContentProtocolPausing || protocolState() == kContentProtocolCanceling);

        if (shutdown)
        {
            // Get the download request metadata
            DownloadRequestMetadata& downloadRequestMetadata = _downloadReqMetadata[reqId];

            // Clean up after ourselves
            UnpackStreamFileId unpackId = downloadRequestMetadata.unpackStreamId;
            if (_unpackFiles.contains(unpackId))
            {
                _unpackStream->closeUnpackFile(unpackId);
                _unpackFiles.remove(unpackId);
            }
            _downloadReqMetadata.remove(reqId);
            _downloadSession->closeRequest(reqId);
        }
        else
        {
            IDownloadRequest *req = _downloadSession->getRequestById(reqId);
            if (!req)
                return;

            // Read the actual chunk
            ProcessChunk(req);
        }

        ScanForCompletion();

        CheckTransferState();
    }

    void ContentProtocolPackage::onDownloadRequestError(RequestId reqId, qint32 errtype, qint32 errcode)
    {
        if (!_downloadSession)
            return;

        IDownloadRequest *req = _downloadSession->getRequestById(reqId);
        if (!req)
            return;

        if (!_downloadReqMetadata.contains(reqId))
            return;

        // Get the download request metadata
        const DownloadRequestMetadata downloadRequestMetadata = _downloadReqMetadata[reqId];

        // Clean up after ourselves
        UnpackStreamFileId unpackId = downloadRequestMetadata.unpackStreamId;
        if (_unpackFiles.contains(unpackId))
        {
            _unpackStream->closeUnpackFile(unpackId);
            _unpackFiles.remove(unpackId);
        }
        _downloadReqMetadata.remove(reqId);
        _downloadSession->closeRequest(reqId);

        // If the whole protocol is shutting down, just return
        if (protocolState() == kContentProtocolPausing || protocolState() == kContentProtocolCanceling)
        {
			CheckTransferState();
            return;
        }

        if (!_protocolForcePaused && (errtype != DownloadError::DownloadWorkerCancelled || protocolState() == kContentProtocolRunning))
        {
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Request error: " << downloadRequestMetadata.packageFileEntry->GetFileName() << " Error: " << ErrorTranslator::Translate((ContentDownloadError::type) errtype);
        
            QString telemStr = QString("Type=%1,Code=%2").arg(errtype).arg(errcode);
            GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mProductId.toLocal8Bit().constData(), "DownloadRequestError", qPrintable(telemStr), __FILE__, __LINE__);

            // Try to re-issue the request (don't clobber the file for network-related errors)
            ContentDownloadError::type rerequestErrCode = TransferReIssueRequest(downloadRequestMetadata.packageFileEntry, static_cast<ContentDownloadError::type>(errtype), false);
		    if (ContentDownloadError::OK != rerequestErrCode)
            {
                ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Failed to re-request file with error condition, halting download session";

                // Send an actual redownload error
                CREATE_DOWNLOAD_ERROR_INFO(redownloadErrorInfo);
                redownloadErrorInfo.mErrorType = errtype; // The actual network error
                redownloadErrorInfo.mErrorCode = rerequestErrCode;
                
                emit (IContentProtocol_Error(redownloadErrorInfo));

                // We weren't able to reissue the request, we need to halt the entire job
                Pause();

                return;
            }
            else
            {
                // Send progress because the byte totals may have changed
                if(checkToSendThrottledUpdate())
                {
                    SendProgressUpdate();
                }
            }
        }

        ScanForCompletion();

        CheckTransferState();
    }

    void ContentProtocolPackage::onDownloadRequestPostponed(RequestId reqId)
    {
        if (!_downloadSession)
            return;

        IDownloadRequest *req = _downloadSession->getRequestById(reqId);
        if (!req)
            return;

        if (!_downloadReqMetadata.contains(reqId))
            return;

        // Get the download request metadata
        const DownloadRequestMetadata downloadRequestMetadata = _downloadReqMetadata[reqId];

        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "[!!] Request postponed: " << reqId << " file: " << downloadRequestMetadata.packageFileEntry->GetFileName();

        // Tell the priority group we're no longer processing
        UpdatePriorityGroupChunkMap(downloadRequestMetadata.packageFileEntry, downloadRequestMetadata.totalBytesRead, downloadRequestMetadata.totalBytesRequested, kEntryStatusQueued);
    }

    void ContentProtocolPackage::onUnpackFileChunkProcessed(UnpackStreamFileId id, qint64 downloadedByteCount, qint64 processedByteCount, bool completed)
    {
        if (!_unpackStream)
            return;

        // If we're no longer tracking this request, bail
        if (!_unpackFiles.contains(id))
        {
            return;
        }

        IUnpackStreamFile *unpackFile = _unpackStream->getUnpackFileById(id);
        if (!unpackFile)
            return;

        bool error = false;

        // Retrieve the unpack file metadata
        const UnpackFileMetadata unpackFileMetadata = _unpackFiles[id];

        // Retrieve the package file entry metadata
        PackageFileEntryMetadata& fileMetadata = _packageFiles[unpackFileMetadata.packageFileEntry];
        fileMetadata.bytesSaved += processedByteCount;
        fileMetadata.bytesDownloaded += downloadedByteCount;

        // Update the CRC map
        QString crcMapFilePath = GetCRCMapFilePath(fileMetadata.diskFilename);
        _crcMap->AddModify(crcMapFilePath, unpackFileMetadata.packageFileEntry->GetFileCRC(), unpackFileMetadata.packageFileEntry->GetUncompressedSize(), _crcMapKey);

        // Tag it with this job ID since we touched the file
        _crcMap->SetJobId(crcMapFilePath, getJobId());

        // Retrieve the download request
        IDownloadRequest* req = _downloadSession->getRequestById(unpackFileMetadata.downloadRequestId);
        if (req != NULL)
        {
            // Mark the chunk as read
            req->chunkMarkAsRead();

            if (req->isComplete())
            {
                //QDEBUGEX << "Request completed: " << pPackageFile->GetFileName();
                ++_requestsCompleted;

                if ((_requestsCompleted % 100) == 0)
                {
                    ORIGIN_LOG_DEBUG << PKGLOG_PREFIX << "Requests completed: " << _requestsCompleted;
                }
                
                // Make sure we got everything we requested, so check the metadata counter
                DownloadRequestMetadata downloadMetadata = _downloadReqMetadata[unpackFileMetadata.downloadRequestId];
                if (downloadMetadata.totalBytesRead != downloadMetadata.totalBytesRequested)
                {
                    // We didn't get everything we asked for, there is some problem here!
                    ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Incomplete/partial request data for file: " << downloadMetadata.packageFileEntry->GetFileName() << " Read: " << downloadMetadata.totalBytesRead << " Requested: " << downloadMetadata.totalBytesRequested;
                    QString telemStr = QString("Read=%1,Reqd=%2").arg(downloadMetadata.totalBytesRead).arg(downloadMetadata.totalBytesRequested);
                    GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mProductId.toLocal8Bit().constData(), "IncompletePartialDownloadRequest", qPrintable(telemStr), __FILE__, __LINE__);

                    error = true;
                }

                // Clean up after ourselves
                fileMetadata.downloadId = -1;
                _downloadReqMetadata.remove(unpackFileMetadata.downloadRequestId);
                _downloadSession->closeRequest(unpackFileMetadata.downloadRequestId);

				// Set it to completed so that it persists the unpack stream to this point.
				completed = true;
            }
        }

        // Update the progress bars
        _dataSaved += processedByteCount;
        _dataDownloaded += downloadedByteCount;
        mTransferStatsDecompressed.addSample(processedByteCount);
        mTransferStats.onDataReceived(downloadedByteCount);

        // Update the priority group chunk counters
        UpdatePriorityGroupChunkMap( unpackFileMetadata.packageFileEntry, fileMetadata.bytesSaved, unpackFileMetadata.packageFileEntry->GetUncompressedSize(), (completed ? kEntryStatusCompleted : kEntryStatusDownloading) );
        
        // Update our debug table if download debugging is enabled.
        if(mReportDebugInfo || Origin::Services::readSetting(Origin::Services::SETTING_DownloadDebugEnabled).toQVariant().toBool())
        {
            Engine::Debug::DownloadFileMetadata file;
            file.setFileName(unpackFileMetadata.packageFileEntry->GetFileName());
            file.setBytesSaved(fileMetadata.bytesSaved);
            file.setTotalBytes(unpackFileMetadata.packageFileEntry->GetUncompressedSize());
            file.setErrorCode(unpackFile->getErrorCode());
            file.setIncluded(unpackFileMetadata.packageFileEntry->IsIncluded());

            Engine::Debug::DownloadDebugDataCollector *collector = Engine::Debug::DownloadDebugDataManager::instance()->getDataCollector(mProductId);
            if(collector)
            {
                collector->updateDownloadFile(file);
            }
        }

        // Make sure to do a final progress update if we've reached 100%
        if(_dataSaved == _dataToSave)
        {
            // Update dynamic download chunk stats
            UpdatePriorityGroupAggregateStats();

            SendProgressUpdate();
        }

        // If the whole protocol is shutting down, just kill everything
        if (protocolState() == kContentProtocolPausing || protocolState() == kContentProtocolCanceling)
        {
            // Retire the unpack file request
            _unpackStream->closeUnpackFile(id);
            _unpackFiles.remove(id);
        }
        else if (completed)
        {
            // Mark it complete
            //finalEntry.mFileState = kFileProgressCompleted;

            // Record the written file time in the fileProgressEntry just in case we ever want to look at it.
            //finalEntry.mCreationTime = finalEntry.mWriteTime = finalEntry.mAccessTime = QDateTime(unpackFileMetadata.packageFileEntry->GetFileModificationDate(), unpackFileMetadata.packageFileEntry->GetFileModificationTime());

            // Retire the unpack file request
            _unpackStream->closeUnpackFile(id);
            _unpackFiles.remove(id);

        }
        else if (_downloadSession->getRequestById(unpackFileMetadata.downloadRequestId) == 0)
        {
            // Retire the unpack file request, this case is if we've only requested a part of the file 
            // as in the case of ITO
            _unpackStream->closeUnpackFile(id);
            _unpackFiles.remove(id);
        }

        // If there was an unexpected error (e.g. partial/incomplete request detected)
        if (error)
        {
            // Try to re-issue the request (don't clobber the existing file for network-related errors)
            ContentDownloadError::type rerequestErrCode = TransferReIssueRequest(unpackFileMetadata.packageFileEntry, static_cast<ContentDownloadError::type>(ProtocolError::PartialDataReceived), false);
		    if (ContentDownloadError::OK != rerequestErrCode)
            {
                ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Failed to re-request file with error condition, halting download session";

                // Send an actual redownload error
                CREATE_DOWNLOAD_ERROR_INFO(redownloadErrorInfo);
                redownloadErrorInfo.mErrorType = ProtocolError::PartialDataReceived; // The actual error we're reporting
                redownloadErrorInfo.mErrorCode = rerequestErrCode; // The reason we couldn't redownload
                
                emit (IContentProtocol_Error(redownloadErrorInfo));

                // We weren't able to reissue the request, we need to halt the entire job
                Pause();

                return;
            }
            else
            {
                // Send progress because the byte totals may have changed
                if(checkToSendThrottledUpdate())
                {
                    SendProgressUpdate();
                }
            }
        }

        ScanForCompletion();

        CheckTransferState();

        if((_numDiscs > 1) && (_downloadReqMetadata.count() <= 0) && (_requestedDisc <= (_numDiscs -1)) && (protocolState() == kContentProtocolRunning))
        {
            _requestedDisc++;
            Retry();
        }
    }

    void ContentProtocolPackage::onUnpackFileError(UnpackStreamFileId id, Origin::Downloader::DownloadErrorInfo errorInfo)
    {
        ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Unpack File Error [type=" << errorInfo.mErrorType << " code=" << errorInfo.mErrorCode << "]:" << ErrorTranslator::Translate((ContentDownloadError::type)errorInfo.mErrorType);

        if (!_unpackStream)
            return;

        // If we're no longer tracking this request, bail
        if (!_unpackFiles.contains(id))
        {
            return;
        }

        IUnpackStreamFile *unpackFile = _unpackStream->getUnpackFileById(id);
        if (!unpackFile)
            return;

        // Retrieve the unpack file metadata
        UnpackFileMetadata& unpackFileMetadata = _unpackFiles[id];

        // Retrieve the package file entry metadata
		PackageFileEntry* pEntry = unpackFileMetadata.packageFileEntry;
        PackageFileEntryMetadata& fileMetadata = _packageFiles[pEntry];
        qint64 bytesSaved = fileMetadata.bytesSaved;
        qint64 bytesDownloaded = fileMetadata.bytesDownloaded;
        fileMetadata.bytesSaved = 0;
        fileMetadata.bytesDownloaded = 0;

        // Retrieve the download request
        IDownloadRequest* req = _downloadSession->getRequestById(unpackFileMetadata.downloadRequestId);
        if (req != NULL)
        {
            // Cancel the request
            req->markCanceled();

            // Clean up after ourselves
            _downloadSession->closeRequest(unpackFileMetadata.downloadRequestId);
        }

		fileMetadata.downloadId = -1;
        _downloadReqMetadata.remove(unpackFileMetadata.downloadRequestId);

        // Fix the progress bars to take away the progress
        _dataSaved -= bytesSaved;
        _dataDownloaded -= bytesDownloaded;
        
        // Update our debug table if download debugging is enabled.
        if(mReportDebugInfo || Origin::Services::readSetting(Origin::Services::SETTING_DownloadDebugEnabled).toQVariant().toBool())
        {
            Engine::Debug::DownloadFileMetadata file;
            file.setFileName(pEntry->GetFileName());
            file.setBytesSaved(fileMetadata.bytesSaved);
            file.setTotalBytes(pEntry->GetUncompressedSize());
            file.setErrorCode(unpackFile->getErrorCode());
            file.setIncluded(pEntry->IsIncluded());
            
            Engine::Debug::DownloadDebugDataCollector *collector = Engine::Debug::DownloadDebugDataManager::instance()->getDataCollector(mProductId);
            if(collector)
            {
                collector->updateDownloadFile(file);
            }
        }

        if(checkToSendThrottledUpdate())
        {
            SendProgressUpdate();
        }

        // Retire the unpack file request
        _unpackStream->closeUnpackFile(id);
		_unpackFiles.remove(id);

        // If the whole protocol is shutting down, just return
        if (protocolState() == kContentProtocolPausing || protocolState() == kContentProtocolCanceling)
        {
			CheckTransferState();
            return;
        }

        // Retry on retryable errors
        if (errorInfo.mErrorType == UnpackError::StreamStateInvalid 
            || errorInfo.mErrorType == UnpackError::StreamUnpackFailed 
            || errorInfo.mErrorType == UnpackError::CRCFailed)
		{
			// for any UnpackError - lets report the file and the offset info to see if we are loading from the correct place (11/9/12)
			QString errorStr = QString("file: %1 zlibErr: %2").arg(pEntry->GetFileName().right(180)).arg(errorInfo.mErrorCode);
			errorStr += QString("offset start: %1 end: %2 disc: %3").arg(pEntry->GetOffsetToFileData()).arg(pEntry->GetEndOffset()).arg(pEntry->GetDiskNoStart());
			GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mProductId.toLocal8Bit().constData(), "DecompFail Range", errorStr.toUtf8().data(), __FILE__, __LINE__);
			
            // Ask the download session to send host-IP telemetry, even though this isn't ncessarily a network-related error
            if (_downloadSession)
            {
                _downloadSession->sendHostIPTelemetry(errorInfo.mErrorType, errorInfo.mErrorCode);
            }

            // Re-request the file, but make sure to delete the existing file (for stream-related problems, the file is corrupt) (also enable diagnostics)
            ContentDownloadError::type errCode = TransferReIssueRequest(pEntry, static_cast<ContentDownloadError::type>(errorInfo.mErrorType), true, true);
			if (ContentDownloadError::OK != errCode)
            {
                ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Failed to re-request file with error condition, halting download session";

                // Send an actual redownload error
                CREATE_DOWNLOAD_ERROR_INFO(redownloadErrorInfo);
                redownloadErrorInfo.mErrorType = errorInfo.mErrorType;
                redownloadErrorInfo.mErrorCode = errCode; // The code will be the reason we couldn't redownload
                redownloadErrorInfo.mErrorContext = errorInfo.mErrorContext; // Preserve the underlying error context
                
                emit (IContentProtocol_Error(redownloadErrorInfo));

                // We weren't able to reissue the request, we need to halt the entire job
                Pause();

                return;
            }
        }
        else
        {            
            // Update the priority group tracking counters with the error state information
            UpdatePriorityGroupChunkMap(pEntry, 0, pEntry->GetUncompressedSize(), kEntryStatusError, (int)errorInfo.mErrorType);

            if (errorInfo.mErrorType > 0)
            {
                emit (IContentProtocol_Error(errorInfo));
            }
            else
            {
                // errorInfo.mErrorType is invalid, so pass up some valid info.
                CREATE_DOWNLOAD_ERROR_INFO(validErrorInfo);
                validErrorInfo.mErrorType = ProtocolError::DownloadSessionFailure;
                validErrorInfo.mErrorCode = errorInfo.mErrorCode;
                validErrorInfo.mErrorContext = errorInfo.mErrorContext;
                emit (IContentProtocol_Error(validErrorInfo));
            }

            // These errors are not retryable, we need to halt the entire job
            Pause();
            return;
        }
    }

    void ContentProtocolPackage::UpdatePriorityGroupAggregateStats()
    {
        // Safety check, don't compute metadata for non-dynamic downloads
        if (!_fIsDynamicDownloadingAvailable)
            return;

        bool hadAnyUpdates = _anyPriorityGroupsUpdated;
        _anyPriorityGroupsUpdated = false;

        // Go over all the priority groups
        for (QMap<int, DownloadPriorityGroupMetadata>::iterator groupIter = _downloadReqPriorityGroups.begin(); groupIter != _downloadReqPriorityGroups.end(); ++groupIter)
        {
            DownloadPriorityGroupMetadata& groupMetadata = groupIter.value();

            // No need to process completed groups further
            if (groupMetadata.status == Engine::Content::kDynamicDownloadChunkState_Installing || groupMetadata.status == Engine::Content::kDynamicDownloadChunkState_Installed)
                continue;

            // Ignore groups that haven't started processing yet
            if (groupMetadata.inactivityCount == -1)
                continue;

            // Decrement the inactivity counter to indicate that we processed this group
            if (hadAnyUpdates && groupMetadata.inactivityCount > 0)
                --groupMetadata.inactivityCount;

            // This group didn't change, don't bother processing it if it's within the allowed period of inactivity
            if (!groupMetadata.hadUpdates && groupMetadata.inactivityCount > 0)
                continue;

            // Iterate over all the entries in the group
            qint64 bytesRead = 0;
            qint64 bytesRequested = 0;

            bool readyToInstall = false;
            bool allInstalled = true;
            bool allComplete = true;
            bool anyActive = false;
            bool anyError = false;
            for (QHash<PackageFileEntry*, DownloadPriorityGroupEntryMetadata>::iterator entryIter = groupMetadata.entries.begin(); entryIter != groupMetadata.entries.end(); ++entryIter)
            {
                DownloadPriorityGroupEntryMetadata& fileMetadata = entryIter.value();

                // If we've had no activity, make sure to un-set the downloading status
                if (groupMetadata.inactivityCount == 0 && fileMetadata.status == kEntryStatusDownloading)
                {
                    fileMetadata.status = kEntryStatusQueued;
                }

                // First handle 0-byte files, they are implicitly complete since they are created at download start
                if (entryIter.key()->GetUncompressedSize() == 0)
                {
                    fileMetadata.status = kEntryStatusInstalled;
                }
                if (fileMetadata.status == kEntryStatusError)
                {
                    anyError = true;
                }
                if (fileMetadata.status != kEntryStatusInstalled)
                {
                    allInstalled = false;
                }
                if (fileMetadata.status != kEntryStatusCompleted && fileMetadata.status != kEntryStatusInstalled) // Installed is complete too, for our purposes
                {
                    allComplete = false;
                }
                if (fileMetadata.status == kEntryStatusDownloading)
                {
                    anyActive = true;
                }

                bytesRead += fileMetadata.totalBytesRead;
                bytesRequested += fileMetadata.totalBytesRequested;
            }

            if (anyError)
            {
                ORIGIN_LOG_EVENT << "[DEBUG] Priority Group #" << groupMetadata.groupId << " has errors.";
                groupMetadata.status = Engine::Content::kDynamicDownloadChunkState_Error;
            }
            else if (allInstalled)
            {
                ORIGIN_LOG_EVENT << "[DEBUG] Priority Group #" << groupMetadata.groupId << " is already installed.";
                groupMetadata.status = Engine::Content::kDynamicDownloadChunkState_Installed;
            }
            else if (allComplete)
            {
                ORIGIN_LOG_EVENT << "[DEBUG] Priority Group #" << groupMetadata.groupId << " downloading complete.";
                groupMetadata.status = Engine::Content::kDynamicDownloadChunkState_Installing;
                readyToInstall = true;
            }
            else if (anyActive)
            {
                groupMetadata.status = Engine::Content::kDynamicDownloadChunkState_Downloading;
            }
            // If the group was previously downloading but there are no longer any active entries, it's back to queued if the inactivity counter ran out
            else if (groupMetadata.status == Engine::Content::kDynamicDownloadChunkState_Downloading && !anyActive && groupMetadata.inactivityCount == 0)
            {
                ORIGIN_LOG_EVENT << "[DEBUG] Priority Group #" << groupMetadata.groupId << " re-queued due to inactivity.";
                groupMetadata.status = Engine::Content::kDynamicDownloadChunkState_Queued;
                groupMetadata.inactivityCount = -1; // Stop this group from being processed until it receives further progress updates
            }
            else
            {
                // We're probably back to being queued
                groupMetadata.inactivityCount = -1; // Stop this group from being processed until it receives further progress updates

                // Unknown groups become queued at least, since they had some activity
                if (groupMetadata.status == Engine::Content::kDynamicDownloadChunkState_Unknown)
                {
                    groupMetadata.status = Engine::Content::kDynamicDownloadChunkState_Queued;
                }
            }

            // Compute the progress
            double progress = (static_cast<double>(bytesRead) / static_cast<double>(bytesRequested));
            ORIGIN_LOG_EVENT << "[DEBUG] Priority Group #" << groupMetadata.groupId << " progress - " << (progress*100.0) << "%.";
            groupMetadata.pctComplete = progress;
            

            // TODO: Compute the ETA based off the last bytesRead/bytesRequested
            groupMetadata.totalBytesRead = bytesRead;
            groupMetadata.totalBytesRequested = bytesRequested;

            // Turn off the hadUpdates flag since we've already aggregated the relevant data
            groupMetadata.hadUpdates = false;

            // Emit a signal indicating that this chunk changed
            emit DynamicChunkUpdate(groupMetadata.groupId, groupMetadata.pctComplete, groupMetadata.totalBytesRead, groupMetadata.totalBytesRequested, groupMetadata.status);

            if (readyToInstall)
            {
                emit DynamicChunkReadyToInstall(groupMetadata.groupId);
            }
            else if (allInstalled)
            {
                // Normally this is emitted from FinalizeDestageFiles, but this group was already installed
                emit DynamicChunkInstalled(groupMetadata.groupId);
            }
        }
    }

    void ContentProtocolPackage::UpdatePriorityGroupChunkMap( PackageFileEntry *pEntry, qint64 totalBytesRead, qint64 totalBytesRequested, PriorityGroupEntryStatus entryStatus, qint32 errorCode )
    {
        // Safety check, don't compute metadata for non-dynamic downloads
        if (!_fIsDynamicDownloadingAvailable)
            return;

        // Set the global activity flag
        _anyPriorityGroupsUpdated = true;

        // Find which chunks we belong to
        for (QMap<int, DownloadPriorityGroupMetadata>::iterator groupIter = _downloadReqPriorityGroups.begin(); groupIter != _downloadReqPriorityGroups.end(); ++groupIter)
        {
            // If we belong to this group
            if (groupIter.value().entries.contains(pEntry))
            {
                DownloadPriorityGroupMetadata& groupMetadata = groupIter.value();
                DownloadPriorityGroupEntryMetadata& fileMetadata = groupMetadata.entries[pEntry];

                // Flag this group as having had updates
                groupMetadata.hadUpdates = true;

                fileMetadata.totalBytesRead = totalBytesRead;
                fileMetadata.totalBytesRequested = totalBytesRequested;
                fileMetadata.status = entryStatus;

                if (entryStatus == kEntryStatusError)
                {
                    // Reset the inactvitiy counter so we know this group was seeing activity
                    groupMetadata.inactivityCount = PRIORITY_GROUP_INACTIVITY_COUNT;

                    emit DynamicChunkError(groupMetadata.groupId, errorCode);
                }
                else if (entryStatus == kEntryStatusInstalled)
                {
                    // The file is already installed, make sure the group gets re-processed
                    groupMetadata.inactivityCount = PRIORITY_GROUP_INACTIVITY_COUNT;
                }
                else if (entryStatus == kEntryStatusCompleted)
                {    
                    // We may be finishing small files in one cycle, so none of them ever have a chance to be 'active'
                    // Therefore, if the group is queued and we just completed a file for it, we'll call it 'downloading'
                    // When the group aggregate stats are counted periodically, this will be fixed if it isn't correct,
                    // for instance if this group got re-queued (postponed)
                    if (groupMetadata.status == Engine::Content::kDynamicDownloadChunkState_Queued)
                    {
                        groupMetadata.status = Engine::Content::kDynamicDownloadChunkState_Downloading;
                    }

                    // Reset the inactvitiy counter so we know this group was seeing activity
                    groupMetadata.inactivityCount = PRIORITY_GROUP_INACTIVITY_COUNT;
                }
                else if (entryStatus == kEntryStatusDownloading)
                {
                    // If the file is downloading, it follows that the group is downloading also
                    groupMetadata.status = Engine::Content::kDynamicDownloadChunkState_Downloading;

                    // Reset the inactvitiy counter so we know this group was seeing activity
                    groupMetadata.inactivityCount = PRIORITY_GROUP_INACTIVITY_COUNT;
                }
                else
                {
                    // Not active, not completed, not installed this file is postponed or not started yet
                    // Nothing to do for now
                }
            }
        }

        // In case of an error, force recomputation of the aggregate stats
        if (entryStatus == kEntryStatusError)
        {
            UpdatePriorityGroupAggregateStats();
        }
    }

    void ContentProtocolPackage::ActivateNonRequiredDynamicChunks()
    {
        // Ensure we can only be invoked on our correct thread
        ASYNC_INVOKE_GUARD;

        // Everything below here runs on the protocol thread
        ORIGIN_LOG_EVENT << "[Dynamic Download] Activating non-required dynamic chunks.";

        // If we are started and haven't yet determined that all required groups are downloaded
        if (protocolState() == kContentProtocolRunning && !_allRequiredGroupsDownloaded)
        {
            // Go over all the priority groups again to see their new status
            bool allRequiredDownloaded = true;
            for (QMap<int, DownloadPriorityGroupMetadata>::iterator groupIter = _downloadReqPriorityGroups.begin(); groupIter != _downloadReqPriorityGroups.end(); ++groupIter)
            {
                DownloadPriorityGroupMetadata& groupMetadata = groupIter.value();

                // No need to process completed groups further
                if (groupMetadata.status == Engine::Content::kDynamicDownloadChunkState_Installing || groupMetadata.status == Engine::Content::kDynamicDownloadChunkState_Installed)
                    continue;

                // If we got to this point and the chunk is required, that means it isn't installed
                if (groupMetadata.requirement == Engine::Content::kDynamicDownloadChunkRequirementRequired)
                    allRequiredDownloaded = false;
            }

            if (allRequiredDownloaded)
            {
                // Make sure all the non-required groups are enabled
                CheckNonRequiredGroupsEnabled();

                _allRequiredGroupsDownloaded = true;

                return;
            }
        }

        // We should not get here unless this function was called inappropriately (required groups are not yet complete)
        ORIGIN_LOG_ERROR << "[Dynamic Download] Failed to enable non-required chunks due to one or more required chunks not being complete.";
    }

    void ContentProtocolPackage::CheckNonRequiredGroupsEnabled()
    {
        // If we are using stepped-download mode, we don't want to do this automatically
        if (_fSteppedDownloadEnabled)
        {
            ORIGIN_LOG_EVENT << "[DEBUG] Required groups downloaded.  Stepped download enabled, will not enable non-required groups automatically.";
            return;
        }

        ORIGIN_LOG_EVENT << "[Dynamic Download] Required groups downloaded.  Enabling non-required groups.";

        // We can only do this with an active download session
        if (!_downloadSession)
            return;

        for (QMap<Engine::Content::DynamicDownloadChunkRequirementT, bool>::iterator requirementIter = _downloadReqPriorityGroupsEnableState.begin(); requirementIter != _downloadReqPriorityGroupsEnableState.end(); ++requirementIter)
        {
            // We already know required chunks are enabled
            if (requirementIter.key() == Engine::Content::kDynamicDownloadChunkRequirementRequired)
                continue;

            // If the requirement group isn't enabled
            if (!requirementIter.value())
            {
                for (QMap<int, DownloadPriorityGroupMetadata>::iterator groupIter = _downloadReqPriorityGroups.begin(); groupIter != _downloadReqPriorityGroups.end(); ++groupIter)
                {
                    DownloadPriorityGroupMetadata& groupMetadata = groupIter.value();

                    // Only interested in groups that match the current requirement group
                    if (groupMetadata.requirement != requirementIter.key())
                        continue;

                    // This group is already enabled
                    if (groupMetadata.groupEnabled)
                        continue;

                    // Enable the group
                    _downloadSession->prioritySetGroupEnableState(groupMetadata.groupId, true);
                    groupMetadata.groupEnabled = true;
                }

                // Mark the whole requirement group as enabled
                requirementIter.value() = true;
            }
        }
    }

    QSet<int> ContentProtocolPackage::GetEntryPriorityGroups(PackageFileEntry *pEntry)
    {
        QSet<int> groups;

        // Find which chunks we belong to
        for (QMap<int, DownloadPriorityGroupMetadata>::iterator groupIter = _downloadReqPriorityGroups.begin(); groupIter != _downloadReqPriorityGroups.end(); ++groupIter)
        {
            // If we belong to this group
            if (groupIter.value().entries.contains(pEntry))
            {
                groups.insert(groupIter.key());
            }
        }

        return groups;
    }

    void ContentProtocolPackage::ClearRunState(bool resetProtocolState)
    {
        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Clearing all state.";

        // We're no longer ready!
        _protocolReady = false;
    
        if(resetProtocolState)
            setProtocolState(kContentProtocolUnknown);

        // Reset our shutdown flag
        _protocolShutdown = false;

        // Turn of stepped downloading in case we get reused
        _fSteppedDownloadEnabled = false;

        // Ensure that the patch builder is destroyed before the package file metadata
        DestroyPatchBuilder();

        // Clear the transfer state
        ClearTransferState();

        // Turn off dynamic downloading after we've cleared the installation data (if cancel, happens in ClearTransferState)
        _fIsDynamicDownloadingAvailable = false;

        // Reset the counters and flags
        _dataSaved = 0;
        _dataToSave = 0;
        _dataAdded = 0;
        _dataDownloaded = 0;
        _dataToDownload = 0;
        _filesToSave = 0;
        _filesPatchRejected = 0;
        _requestsCompleted = 0;

        // Reset our download debug database
        if(mReportDebugInfo || Origin::Services::readSetting(Origin::Services::SETTING_DownloadDebugEnabled).toQVariant().toBool())
        {
            Engine::Debug::DownloadDebugDataManager::instance()->removeDownload(mProductId);
        }

        // Kill the Central Directory
        if (_packageFileDirectory)
        {
            delete _packageFileDirectory;
        }
        _packageFileDirectory = NULL;

        // Kill any CRC proxy progress object, if we have one
        if (_crcMapVerifyProxy)
        {
            _crcMapVerifyProxy->Cleanup();
            _crcMapVerifyProxy = NULL;
        }

        // Kill the CRC Map
        if (_crcMap)
        {
            if (_crcMap->HasChanges())
            {
                // Save it first!
                _crcMap->Save();
            }

            CRCMapFactory::instance()->releaseCRCMap(_crcMap);
        }
        _crcMap = NULL;

        // Kill the metadata maps
        _packageFiles.clear();
        _deletePatterns.clear();
        _diffPatchFiles.clear();

        // Kill our sorted list
        _packageFilesSorted.clear();
    }

    void ContentProtocolPackage::ClearTransferState()
    {
		ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Clearing protocol metadata...";
        // Kill the transfer-related metadata maps
        _downloadReqMetadata.clear();
        _unpackFiles.clear();

        _allRequiredGroupsDownloaded = false;

        // Kill the download session
        if (_downloadSession)
        {
			ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Destroying download session...";
            DownloadService::DestroyDownloadSession(_downloadSession);
        }
        _downloadSession = NULL;

        // Kill the unpack stream
        if (_unpackStream)
        {
			ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Destroying unpack stream...";
            UnpackService::DestroyUnpackStream(_unpackStream);
        }
        _unpackStream = NULL;

        // If we're pausing, just emit that signal
        if (protocolState() == kContentProtocolPausing)
        {
			ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Transitioning to Paused...";
			_cancelPauseTimer.stop();
            setProtocolState(kContentProtocolPaused);
            emit (Paused());
        }
        else if (protocolState() == kContentProtocolCanceling)
        {
			ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Cleaning up canceled transfer...";
			_cancelPauseTimer.stop();
            // We're canceling, so hit the cleanup function
            Cleanup();
        }
        else if (protocolState() == kContentProtocolRunning)
        {
            // We're done transferring
            ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Finished!";
            setProtocolState(kContentProtocolUnknown);
        }
        else
        {
			// We probably got here in some kind of error handler, or are clearing out state at protocol startup
            setProtocolState(kContentProtocolUnknown);
        }
    }

    void ContentProtocolPackage::SuspendAll()
    {
        if (_downloadSession != NULL)
        {
            ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Stopping all transfers...";
            _downloadSession->shutdown();
        }
    }

    void ContentProtocolPackage::ResumeAll()
    {
        setProtocolState(kContentProtocolResuming);

        // We are resuming from cold pause, so just transition to starting
        TransferStart();
    }

    void ContentProtocolPackage::CheckTransferState()
    {
        // The purpose of this function is to make sure that if we need to put the brakes on everything, that we wait for everything to complete and then clean up
        if (protocolState() == kContentProtocolRunning)
        {
            return;
        }

        // if there are still download requests pending or if we are not on the last disc don't clean up
        if (_downloadReqMetadata.count() > 0 || _unpackFiles.count() > 0)
        {
            return;
        }

        // If we're diff updating, see if the PatchBuilder is still busy
        if (_fIsDiffUpdatingAvailable && _diffPatchFiles.count() > 0)
        {
            return;
        }

		// Still waiting on download session to shutdown
		if(_downloadSession != NULL && _downloadSession->isRunning())
		{
			return;
		}

        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "All transfers stopped.  Clearing state.";

        // We don't have anything to do anymore
        ClearRunState(false);
    }

    bool string_length_greater_predicate( const QString& dir1, const QString& dir2 )
    {
        return dir1.length() > dir2.length();
    }

    void ContentProtocolPackage::DeleteStagedFileData(PackageFile& zipFileDirectory)
    {
        // First delete all associated files in the package
        for (PackageFileEntryList::iterator it = zipFileDirectory.begin(); it != zipFileDirectory.end(); it++)
        {
            PackageFileEntry* pPackageEntry = (*it);
            if (pPackageEntry->IsIncluded())
            {
                CFilePath fullPath = CFilePath::Absolutize( _unpackPath, pPackageEntry->GetFileName() );

                if (pPackageEntry->IsFile())
                {
                    QString sFilename = pPackageEntry->GetFileName();
                        
                    // Kill any staged files that may exist
                    if (ShouldStageFile(sFilename))
                    {
                        QString sStagedFilename = GetStagedFilename(sFilename);
                        QString sFullStagedFilePath = GetFileFullPath(sStagedFilename);

                        if (EnvUtils::GetFileExistsNative(sFullStagedFilePath))
                            Origin::Util::DiskUtil::DeleteFileWithRetry(sFullStagedFilePath);

                        QString streamFileName( UnpackStreamFile::GetDecompressorFilename(sFullStagedFilePath) );
                        if (EnvUtils::GetFileExistsNative(streamFileName))
                            Origin::Util::DiskUtil::DeleteFileWithRetry(streamFileName);

                        // Ask the PatchBuilder to clear its own temp files
                        PatchBuilder::ClearTempFilesForEntry(_unpackPath, sFilename, sStagedFilename);
                    }
                }
            }
        }
    }

    void ContentProtocolPackage::DeleteEmptyFolders(PackageFile& zipFileDirectory)
    {
        // Build a list of unique folder names from the files/folders in the package
        std::set<QString> uniqueFolders;
        for (PackageFileEntryList::iterator it = zipFileDirectory.begin(); it != zipFileDirectory.end(); it++)
        {
            PackageFileEntry* pPackageEntry = (*it);
            if (pPackageEntry->IsIncluded())
            {
                CFilePath fullPath = CFilePath::Absolutize( _unpackPath, pPackageEntry->GetFileName() );
                uniqueFolders.insert(fullPath.GetDirectory());
            }
        }

        // Note:  Non-empty folders will be left behind 
        std::list<QString> folders;

        // Move the list of unique folders into a sortable list
        for (std::set<QString>::iterator it = uniqueFolders.begin(); it != uniqueFolders.end(); it++)
        {
            //QDEBUGEX << "Unique Folder \"" << *it << "\"";
            folders.push_back(*it);
        }

        // We'll sort the list of folders from longest to shortest (as that will let us delete children before parents.)
        folders.sort(string_length_greater_predicate);

        // remove all empty folders up to the parent
        QDir qdir;

        // Now do the removals
        for (std::list<QString>::iterator it = folders.begin(); it != folders.end(); it++)
        {
            QString sFolder(*it);
        
            do 
            {
                // Note:  We can't use qdir.rmpath because it could also remove the Origin Games folder (eg. "c:/program files/origin games/") which causes the settings dialog to think that the folder is invalid.
                if (qdir.rmdir(sFolder))
                {
                    ORIGIN_LOG_DEBUG << PKGLOG_PREFIX << "Removed folder \"" << sFolder << "\"";
                }

                QString tempFolder = sFolder + QDir::separator() + "..";
				sFolder = qdir.cleanPath( tempFolder );		// gets the parent folder
            } while (sFolder.length() > _unpackPath.length());  // until we get to our unpack folder
        }

        // Now remove the unpack folder if it's empty
        if (qdir.rmdir(_unpackPath))
        {
            ORIGIN_LOG_DEBUG << PKGLOG_PREFIX << "Removed folder \"" << _unpackPath << "\"";
        }
        else
        {
            ORIGIN_LOG_DEBUG << PKGLOG_PREFIX << "Couldn't remove folder \"" << _unpackPath << "\"";
        }
    }

    void ContentProtocolPackage::CleanupUpdateRepairData()
    {
        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Cleaning all staged data from " << (_fIsUpdate ? "update" : "repair");
        ORIGIN_ASSERT(_fIsUpdate || _fIsRepair);

        // if we haven't started anything, then there is no need for deleting folders.  Fixes OFM-4313 where DLCs, when canceled off the queue can delete
        // folders the main game is still downloading into causing a disc write error.
        if (protocolState() == kContentProtocolInitialized)
            return;

        PackageFile zipFileDirectory;
        
        if(!zipFileDirectory.LoadFromManifest( GetPackageManifestFilename() ))
        {
            ORIGIN_LOG_ERROR << "Failed to load package manifest [" << GetPackageManifestFilename() << "] for staged file cleanup.";

			QString errStr = QString("Failed to load package manifest %1").arg(GetPackageManifestFilename());
			GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mProductId.toLocal8Bit().constData(), "Cleanup UpdateRepair Failed", errStr.toUtf8().data(), __FILE__, __LINE__);
        }
        else
        {
            // We only delete Staged files and their _stream data.
            DeleteStagedFileData(zipFileDirectory);

            // Delete any empty folders left behind. See EBIBUGS-20743.
            DeleteEmptyFolders(zipFileDirectory);

            ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "CleanupUpdateRepairData complete.";
        }
    }

    void ContentProtocolPackage::CleanupFreshInstallData()
    {
        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Cleaning all installation data from fresh install.";
        ORIGIN_ASSERT(!(_fIsUpdate || _fIsRepair));

		//clear ITO specific vars
		_numDiscs = 1;
		_currentDisc = 0;
		_requestedDisc = 0;
		_fileNameToDiscStartMap.clear(); 
		_discIndexToDiscSizeMap.clear();
		_headerOffsetTofileDataOffsetMap.clear();


		try  // TODO: THIS IS A HACK, REMOVE THIS STUFF AND HANDLE ERRORS PROPERLY
        {
            // Wipe everything out
            PackageFile zipFileDirectory;

            if(!zipFileDirectory.LoadFromManifest( GetPackageManifestFilename() ))
            {
                ORIGIN_LOG_ERROR << "Failed to load package manifest [" << GetPackageManifestFilename() << "] for installation file cleanup.";

				QString errStr = QString("Failed to load package manifest %1").arg(GetPackageManifestFilename());
				GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mProductId.toLocal8Bit().constData(), "Cleanup Failed", errStr.toUtf8().data(), __FILE__, __LINE__);
            }
            else
            {
                // Delete all staged files and their _stream data.
                DeleteStagedFileData(zipFileDirectory);

                // note that this function works also when mPackageFile contains zero entries,
                // which can happen following failed downloads, in which case this function 
                // reduces to just a call to QDir::rmdir(GetUnpackDir())
                ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Cleaning up - Package contains " << zipFileDirectory.GetNumberOfEntries() << " entries\n";
                
                // Kill the manifest
                QString packageManifestFilename = GetPackageManifestFilename();
                if (EnvUtils::GetFileExistsNative(packageManifestFilename))
                    Origin::Util::DiskUtil::DeleteFileWithRetry(packageManifestFilename);

                // Kill the CRC map
                QString crcMapFilename = GetCRCMapFilename();
                if (EnvUtils::GetFileExistsNative(crcMapFilename))
                    Origin::Util::DiskUtil::DeleteFileWithRetry(crcMapFilename);

                // First delete all associated files in the package, and if its not an update, we're clobbering regular installation files
                for (PackageFileEntryList::iterator it = zipFileDirectory.begin(); it != zipFileDirectory.end(); it++)
                {
                    PackageFileEntry* pPackageEntry = (*it);
                    if (pPackageEntry->IsIncluded() && pPackageEntry->IsFile())
                    {
                        QString sFilename = pPackageEntry->GetFileName();
                        QString sFullFilename = GetFileFullPath(sFilename);

                        if (EnvUtils::GetFileExistsNative(sFullFilename))
                            Origin::Util::DiskUtil::DeleteFileWithRetry(sFullFilename);

                        // Clean up any stream files that may be laying around
                        QString streamFileName( UnpackStreamFile::GetDecompressorFilename(sFullFilename) );
                        if (EnvUtils::GetFileExistsNative(streamFileName))
                            Origin::Util::DiskUtil::DeleteFileWithRetry(streamFileName);
                    }
                }

                DeleteEmptyFolders(zipFileDirectory);
            }

            ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "CleanupFreshInstallData complete.";
        }
        catch (...)
        {
            // TODO: THIS IS A HACK, REMOVE THIS STUFF AND HANDLE ERRORS PROPERLY
        }
    }

    void ContentProtocolPackage::Cleanup()
    {
        if(_fIsUpdate || _fIsRepair)
        {
            CleanupUpdateRepairData();
        }
        else if (_fSafeCancel) // Safe cancel flag inhibits us from blowing away game files before we've gotten to a transferring state
        {
            ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Skip delete of base game files due to safe-cancel flag being set.";
        }
        else 
        {
            CleanupFreshInstallData();
        }

        // Clear our state
        ClearRunState();

        setProtocolState(kContentProtocolCanceled);
        emit (Canceled());
    }

    // Creates a PatchBuilder object if necessary.
    void ContentProtocolPackage::CreatePatchBuilder()
    {
        if (_patchBuilder == NULL)
        {
            ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Creating PatchBuilder.";

            // Clear patching-related counters
            _filesPatchRejected = 0;

            _patchBuilder = PatchBuilderFactory::CreatePatchBuilder(_unpackPath, mProductId, getGameTitle(), mSession);

            ORIGIN_VERIFY_CONNECT_EX(_patchBuilder, SIGNAL(InitializeMetadataProgress(qint64, qint64)), this, SLOT(onPatchBuilderInitializeMetadataProgress(qint64, qint64)), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(_patchBuilder, SIGNAL(InitializeRejectFiles(DiffFileErrorList)), this, SLOT(onPatchBuilderInitializeRejectFiles(DiffFileErrorList)), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(_patchBuilder, SIGNAL(Initialized(qint64)), this, SLOT(onPatchBuilderInitialized(qint64)), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(_patchBuilder, SIGNAL(PatchFileProgress(DiffFileId, qint64, qint64)), this, SLOT(onPatchBuilderFileProgress(DiffFileId, qint64, qint64)), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(_patchBuilder, SIGNAL(PatchFileComplete(DiffFileId)), this, SLOT(onPatchBuilderFileComplete(DiffFileId)), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(_patchBuilder, SIGNAL(Complete()), this, SLOT(onPatchBuilderComplete()), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(_patchBuilder, SIGNAL(Paused()), this, SLOT(onPatchBuilderPaused()), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(_patchBuilder, SIGNAL(Canceled()), this, SLOT(onPatchBuilderCanceled()), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(_patchBuilder, SIGNAL(PatchFatalError(Origin::Downloader::DownloadErrorInfo)), this, SLOT(onPatchBuilderFatalError(Origin::Downloader::DownloadErrorInfo)), Qt::QueuedConnection);
        }
    }

    void ContentProtocolPackage::DestroyPatchBuilder()
    {
        if (_patchBuilder)
        {
            ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Destroying PatchBuilder.";

            PatchBuilderFactory::DestroyPatchBuilder(_patchBuilder);

            _patchBuilder = NULL;
            _patchBuilderInitialized = false;
        }
    }

    // Gets a DiffFileId from PatchBuilder.  (Creates it if necessary)
    DiffFileId ContentProtocolPackage::GetNextPatchBuilderId()
    {
        // This only happens if necessary
        CreatePatchBuilder();

        return _patchBuilder->CreateDiffFileId();
    }

    bool ContentProtocolPackage::ChangeDiscIfNeeded()
    {
        bool discChangeNeeded = false;
        if(_requestedDisc == _currentDisc)
            return true;

        if((_numDiscs > 1) && (_currentDisc < (_numDiscs - 1)) && (_downloadReqMetadata.count() <= 0) && (_requestedDisc != _currentDisc))
        {
            _checkDiscTimer.stop();
            discChangeNeeded = true;
            if(!ChangeDisc())
            {
                //  TELEMETRY:  Detect disk change prompt
                QString errorCode = QString("RequestDisk=%1,NumDisk=%2").arg(_requestedDisc).arg(_numDiscs);
                GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mProductId.toLocal8Bit().constData(), "ChangeDiscIfNeeded", errorCode.toUtf8().data(), __FILE__, __LINE__);

                emit(CannotFindSource(_requestedDisc + 1, _numDiscs, -1));

                //every 5 seconds check for the disc
                _checkDiscTimer.start(5000);
            }
        }
        return discChangeNeeded;
    }

    void ContentProtocolPackage::onCheckDiskTimeOut()
    {
        if(!ChangeDisc())
        {
            int wrongSource = -1;
            for(int i = 0; i < _numDiscs; i++)
            {
                if(i == _currentDisc)
                    continue;

                if(IsDiscAvailable(i))
                {
                    wrongSource = i;
                    break;
                }
            }

            //  TELEMETRY:  Detect wrong disk
            QString errorCode = QString("RequestDisk=%1,NumDisk=%2,WrongSrc=%3").arg(_requestedDisc).arg(_numDiscs).arg(wrongSource);
            GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mProductId.toLocal8Bit().constData(), "onCheckDiskTimeOut", errorCode.toUtf8().data(), __FILE__, __LINE__);

            emit(CannotFindSource(_requestedDisc + 1, _numDiscs, wrongSource));
        }
    }

    void ContentProtocolPackage::InitializeStart()
    {
        if(_protocolForcePaused)
        {
            ClearRunState();
            return;
        }

        // Determine the package type
        QUrl url(_url);
        QFileInfo finfo(url.path());
        QString extension = finfo.suffix().toLower();

        if (extension == "viv")
        {
            _packageType = ContentPackageType::Viv;
        }
        else // Default to zip, since there are a lot of different file extensions for zip.  Validation will fail if it can't read a zip central directory later if it turns out not to be a zip file.
        {
            _packageType = ContentPackageType::Zip;
        }
        
        // Setup download debug data collector
        if(mReportDebugInfo || Origin::Services::readSetting(Origin::Services::SETTING_DownloadDebugEnabled).toQVariant().toBool())
        {
            Engine::Debug::DownloadDebugDataManager::instance()->addDownload(mProductId);
        }

        // Create a download source
        _downloadSession = DownloadService::CreateDownloadSession(_url, mProductId, _physicalMedia, mSession );

        ORIGIN_VERIFY_CONNECT_EX(_downloadSession, SIGNAL(initialized(qint64)), this, SLOT(onDownloadSessionInitialized(qint64)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(_downloadSession, SIGNAL(IDownloadSession_error(Origin::Downloader::DownloadErrorInfo)), this, SLOT(onDownloadSessionError(Origin::Downloader::DownloadErrorInfo)), Qt::QueuedConnection);
		ORIGIN_VERIFY_CONNECT_EX(_downloadSession, SIGNAL(shutdownComplete()), this, SLOT(onDownloadSessionShutdown()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(_downloadSession, SIGNAL(requestChunkAvailable(RequestId)), this, SLOT(onDownloadRequestChunkAvailable(RequestId)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(_downloadSession, SIGNAL(requestError(RequestId, qint32, qint32)), this, SLOT(onDownloadRequestError(RequestId, qint32, qint32)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(_downloadSession, SIGNAL(requestPostponed(RequestId)), this, SLOT(onDownloadRequestPostponed(RequestId)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(_downloadSession, SIGNAL(priorityQueueOrderUpdate(QVector<int>)), this, SIGNAL(DynamicChunkOrderUpdated(QVector<int>)), Qt::QueuedConnection);
		_downloadSession->initialize();
    }

    QString ContentProtocolPackage::getPathForDisc(const int &disc)
    {
        //parse the current url so that we can remove the extension
        QString newURL;

        int index = _url.lastIndexOf('.');
        if (index >= 0)
        {
            newURL = _url.left(index+1);
        }

        //the last disk always contains the .zip file other wise the extension is .z01, .z02 etc
        if ((disc + 1) == _numDiscs)
            newURL.append("zip");
        else
            newURL.append(QString("z%1").arg(disc + 1, 2, 10, QChar('0')));

        return newURL;
    }

	bool ContentProtocolPackage::IsPathAvailable(const QString& fileURL)
	{
		QString discPath = fileURL;
		discPath.remove("file:///");
        discPath.remove("file://");
		discPath.remove("file:");

		bool pathExists = false;
        QFile file(discPath);
        if(file.open(QIODevice::ReadOnly))
        {
            pathExists = true;
            file.close();
        }

        return pathExists;
	}

    bool ContentProtocolPackage::IsDiscAvailable(const int &disc)
    {
		return IsPathAvailable(getPathForDisc(disc));
    }

    bool ContentProtocolPackage::ChangeDisc()
    {
        bool changeDiscSuccess = false;

        //only if we are multi-disc, if we are zip do nothing
        if(_numDiscs > 1 || _discEjected)
        {
            if(IsDiscAvailable(_requestedDisc))
            {
                _currentDisc = _requestedDisc;
                changeDiscSuccess = true;

                _checkDiscTimer.stop();

                if( _discEjected )
                {
                    _discEjected = false;
                    emit (SourceFound(true /* discEjected */));
                }
                else
                {
                    emit (SourceFound());
                    //change the download source, we first delete the current _downloadSession
                    //then create a new one with the new url
                    if (_downloadSession)
                        DownloadService::DestroyDownloadSession(_downloadSession);

                    _url = getPathForDisc(_currentDisc);
                    _downloadSession = DownloadService::CreateDownloadSession(_url, mProductId, _physicalMedia, mSession );

                    //connect to a different slot for disc change for the initialized signal
                    //in onDownloadSessionInitializedForDiscChange we set the current disc size as well as start the next transfer
                    ORIGIN_VERIFY_CONNECT_EX(_downloadSession, SIGNAL(initialized(qint64)), this, SLOT(onDownloadSessionInitializedForDiscChange(qint64)), Qt::QueuedConnection);
                    ORIGIN_VERIFY_CONNECT_EX(_downloadSession, SIGNAL(IDownloadSession_error(Origin::Downloader::DownloadErrorInfo)), this, SLOT(onDownloadSessionError(Origin::Downloader::DownloadErrorInfo)), Qt::QueuedConnection);
				    ORIGIN_VERIFY_CONNECT_EX(_downloadSession, SIGNAL(shutdownComplete()), this, SLOT(onDownloadSessionShutdown()), Qt::QueuedConnection);
                    ORIGIN_VERIFY_CONNECT_EX(_downloadSession, SIGNAL(requestChunkAvailable(RequestId)), this, SLOT(onDownloadRequestChunkAvailable(RequestId)), Qt::QueuedConnection);
                    ORIGIN_VERIFY_CONNECT_EX(_downloadSession, SIGNAL(requestError(RequestId, qint32, qint32)), this, SLOT(onDownloadRequestError(RequestId, qint32, qint32)), Qt::QueuedConnection);

                    // Initialize the download source
                    _downloadSession->initialize();
                    setProtocolState(kContentProtocolInitializing);
                }
            }
        }
        else
            changeDiscSuccess = false;

        return changeDiscSuccess;
    }

    void ContentProtocolPackage::InitializeConnected(qint64 contentLength)
    {
        if(_protocolForcePaused)
        {
            ClearRunState();
            return;
        }

        // Create the book-keeping objects
        _packageFileDirectory = new PackageFile;
        _crcMap = CRCMapFactory::instance()->getCRCMap(GetCRCMapFilename());

        // Read the central directory from the data source if we're downloading from server or reading first disk of ITO
        if (!_physicalMedia || (_physicalMedia && (_currentDisc == 0)))
        { 
            // Just to be safe, clobber the cached one
            QString packageManifestFilename = GetPackageManifestFilename();
            if (EnvUtils::GetFileExistsNative(packageManifestFilename))
                Origin::Util::DiskUtil::DeleteFileWithRetry(packageManifestFilename);

            // Read it from the data source
            ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Reading central directory from data source.";

            bool readDirectory = false;
            DownloadErrorInfo errInfo;

            if (_packageType == ContentPackageType::Zip)
            {
                if (ContentProtocolUtilities::ReadZipCentralDirectory(_downloadSession, contentLength, getGameTitle(), mProductId, *_packageFileDirectory, _numDiscs, errInfo, &_activeSyncRequestCount, &_delayedCancelWaitCondition))
                {
                    readDirectory = true;
                }
            }

            if (readDirectory)
            {
                // Make sure the package file directory is valid
                if (!ContentProtocolUtilities::VerifyPackageFile(_unpackPath, contentLength, getGameTitle(), mProductId, *_packageFileDirectory))
                {
                    ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Package file failed validation.";
                    POPULATE_ERROR_INFO_TELEMETRY(errInfo, ProtocolError::ZipOffsetCalculationFailed, 0, mProductId.toLocal8Bit().constData());
                    readDirectory = false;
                }
				else
				{
					_fileNameToDiscStartMap.clear(); 
					_discIndexToDiscSizeMap.clear();
					_headerOffsetTofileDataOffsetMap.clear();

					//we change disk from data.cd to data.z01 with this call
					//the central directory is store in its own data.cd file, once we read it, we can switch to the file with actual data
					ChangeDisc();
				}
            }

            if(!readDirectory)
            {
                ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Unable to load package file directory!";

                ClearRunState();
                emit(IContentProtocol_Error(errInfo));
                return;
            }
            
            // Persist the Central Directory, we might need it later if canceling
            if (!_packageFileDirectory->SaveManifest( GetPackageManifestFilename() ))
            {
                ClearRunState();
            
                CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
                errorInfo.mErrorType = UnpackError::IOWriteFailed;
                errorInfo.mErrorCode = Services::PlatformService::lastSystemResultCode();

                ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Unable to save package manifest, error code " << errorInfo.mErrorCode;

                emit(IContentProtocol_Error(errorInfo));
                return;
            }
        }
        else if(_physicalMedia && (!EnvUtils::GetFileExistsNative(GetPackageManifestFilename()) || !_packageFileDirectory->LoadFromManifest( GetPackageManifestFilename())))
        {
            GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mProductId.toLocal8Bit().constData(), "InitializeConnectedFileITORead", GetPackageManifestFilename().toUtf8().data(), __FILE__, __LINE__);

            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Failed to read ITO central directory from cache.";
            CREATE_DOWNLOAD_ERROR_INFO(errInfo);
            errInfo.mErrorType = ProtocolError::ContentInvalid;
            errInfo.mErrorCode = 0;
            emit(IContentProtocol_Error(errInfo));
            return;
        }
        else
        {
            ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Successfully read ITO central directory from cache.";
        }


        // Set the proper destination directory
        _packageFileDirectory->SetDestinationDirectory(_unpackPath);

        // Set the product ID
        _packageFileDirectory->SetProductId(mProductId);

        // Go to the verify stage
        InitializeVerify();
    }

    void ContentProtocolPackage::InitializeVerify()
    {
        if(_protocolForcePaused)
        {
            ClearRunState();
            return;
        }

        if(_skipCrcVerification)
        {
            // Finish our initialization
            InitializeEnd();
            return;
        }

        // Setup the CRC map
        ORIGIN_LOG_DEBUG << PKGLOG_PREFIX << "Loading CRC map";
        if (!ProcessCRCMap())
        {
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Unable to setup CRC map";

            ClearRunState();
            
            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = UnpackError::IOCreateOpenFailed;
            errorInfo.mErrorCode = Services::PlatformService::lastSystemResultCode();
            emit (IContentProtocol_Error(errorInfo));
            return;
        }
    }

    void ContentProtocolPackage::InitializeVerified()
    {
        if(_protocolForcePaused)
        {
            ClearRunState();
            return;
        }

        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "CRC Calculation Complete.";

        bool shutdown = ( protocolState() == kContentProtocolPausing || protocolState() == kContentProtocolCanceling || protocolState() == kContentProtocolCanceled);

        if (shutdown)
        {
            ORIGIN_LOG_WARNING << PKGLOG_PREFIX << "User canceled CRC verify";

            ClearRunState();
            
            return;
        }

        // Finish our initialization
        InitializeEnd();
    }

    void ContentProtocolPackage::InitializeEnd()
    {
        if(_protocolForcePaused)
        {
            ClearRunState();
            return;
        }

        //we change disk from data.cd to data.z01 with this call
        //the central directory is store in its own data.cd file, once we read it, we can switch to the file with actual data
        if (_physicalMedia && !IsDiscAvailable(_requestedDisc))
		{
			ClearRunState();	// set this to unknown so that we can retry

            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = DownloadError::ContentUnavailable;
            errorInfo.mErrorCode = 0;
            emit(IContentProtocol_Error(errorInfo));

			return;
		}

        // Mark ourselves ready
        _protocolReady = true;
        setProtocolState(kContentProtocolInitialized);

        if(_fIsRepair)
        {
            // TELEMETRY: The user has requested to repair their install.
            GetTelemetryInterface()->Metric_REPAIRINSTALL_REQUESTED(mProductId.toUtf8().data());
        }

        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Initialization complete.";

        emit(Initialized());
    }
    void ContentProtocolPackage::InitializeFail(qint32 errorCode)
    {
        setProtocolState(kContentProtocolUnknown);
        
        CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
        errorInfo.mErrorType = ProtocolError::DownloadSessionFailure;
        errorInfo.mErrorCode = errorCode;
        emit (IContentProtocol_Error(errorInfo));
    }

    // This function is analogous to the InitializerGetRange function; synchronous I/O of a single file only
    void ContentProtocolPackage::TransferSingleFile(const QString& archivePath, const QString& diskPath)
    {
        TIME_BEGIN("ContentProtocolPackage::TransferSingleFile");
        bool result = false;
        DownloadErrorInfo errorInfo;

        const PackageFileEntry* fileEntry = _packageFileDirectory->GetEntryByName(archivePath, false);
        if (fileEntry != NULL)
        {
            // Increment active sync requests to delay cancel requests from the client
            _activeSyncRequestCount.ref();

            qint64 offStart = fileEntry->GetOffsetToFileData();
            qint64 offEnd = fileEntry->GetEndOffset();

            //qDebug() << "Start: " << offStart << " End: " << offEnd << " File: " << fileEntry->GetFileName();

            qint64 reqId = _downloadSession->createRequest(offStart, offEnd);
            IDownloadRequest *request = _downloadSession->getRequestById(reqId);
            if (_downloadSession->requestAndWait(reqId) && request->isComplete() && request->chunkIsAvailable() && request->getErrorState() == 0)
            {
                MemBuffer *fileData = request->getDataBuffer();

                IUnpackStreamFile *unpackFile = UnpackService::GetStandaloneUnpackStreamFile(mProductId,
					                                                                         diskPath, 
                                                                                             fileEntry->GetCompressedSize(), 
                                                                                             fileEntry->GetUncompressedSize(), 
                                                                                             DateTime::QDateToMsDosDate(fileEntry->GetFileModificationDate()), 
                                                                                             DateTime::QTimeToMsDosTime(fileEntry->GetFileModificationTime()),
                                                                                             fileEntry->GetFileAttributes(),
                                                                                             (UnpackType::code)fileEntry->GetCompressionMethod(), 
                                                                                             fileEntry->GetFileCRC()
                                                                                             );

                quint32 bytesProcessed = 0;
                bool fileComplete = false;
                if (!unpackFile->processChunk(fileData->GetBufferPtr(), fileData->TotalSize(), QSharedPointer<DownloadChunkDiagnosticInfo>(NULL), bytesProcessed, fileComplete))
                {
                    ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Unable to unpack single file: " << fileEntry->GetFileName();
                    POPULATE_ERROR_INFO_TELEMETRY(errorInfo, unpackFile->getErrorType(), unpackFile->getErrorCode(), mProductId.toLocal8Bit().constData());
                }
                else
                {
                    // Mark the request as read
                    request->chunkMarkAsRead();
                    result = true;
                }

                // Clean up the unpack file
                UnpackService::DestroyStandaloneUnpackStreamFile(unpackFile);
            }
            else
            {
                ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Unable to read bytes from download session";           
                POPULATE_ERROR_INFO_TELEMETRY(errorInfo, request->getErrorState(), request->getErrorCode(), mProductId.toLocal8Bit().constData());
            }

            // Dispatch the request
            _downloadSession->closeRequest(reqId);

            //Wake cancel requests that were delayed because of an sync network request
            _activeSyncRequestCount.deref();
            int count = _activeSyncRequestCount.loadAcquire();
            if (count <= 0)
            {
                _delayedCancelWaitCondition.wakeAll();
            }
        }
        else
        {
            GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mProductId.toLocal8Bit().constData(), "TransferSingleFileNotFound", archivePath.toUtf8().data(), __FILE__, __LINE__);

            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Couldn't find file: " << archivePath;
            POPULATE_ERROR_INFO_TELEMETRY(errorInfo, ProtocolError::ContentInvalid, 0, mProductId.toLocal8Bit().constData());
        }

        if (result)
        {
            emit (FileDataReceived(archivePath, diskPath));
        }
        else
        {
            emit (IContentProtocol_Error(errorInfo));
        }
        TIME_END("ContentProtocolPackage::TransferSingleFile");
    }

    void ContentProtocolPackage::TransferStart()
    {
        if (_fIsUpdate)
        {
            ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Running update job.";
        }

        // Create the directories if needed. 
        // If the folder already exists, the escalation client will ensure the correct permissions are set
        int escalationError = 0;
        int escalationSysError = 0;
        ContentDownloadError::type returnVal = ContentProtocolUtilities::CreateDirectoryAllAccess(_unpackPath, &escalationError, &escalationSysError);
        if (returnVal != ContentDownloadError::OK)
        {
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Failed to create unpack directory: " << _unpackPath;

            ClearRunState();

            int sysError = escalationError;

            // For command failures, we should send the sys error code back instead since it tells us more
            if ((int)returnVal == (int)ProtocolError::ContentFolderError)
                sysError = escalationSysError;

            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = returnVal;
            errorInfo.mErrorCode = sysError;
            errorInfo.mErrorContext = _unpackPath;
            emit(IContentProtocol_Error(errorInfo));
            return;
        }

#if defined(ORIGIN_MAC)
        // Hide the bundle from Finder
        if (!_fIsUpdate && !_fIsRepair)
        {
            struct stat fileStat;
            stat(qPrintable(_unpackPath), &fileStat);
            
            if ((fileStat.st_flags & UF_HIDDEN) == 0)
            {
                ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Hiding bundle.";
                
                chflags(qPrintable(_unpackPath), (fileStat.st_flags | UF_HIDDEN));
            }
        }
#endif
        
        // Preallocate the files from the zipfile
        if (! ContentProtocolUtilities::PreallocateFiles(*_packageFileDirectory))
        {
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Unable to preallocate directories!";

            ClearRunState();
            
            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = UnpackError::IOCreateOpenFailed;
            // TODO: Create a means to return system error codes from the escalation client.
            errorInfo.mErrorCode = 0;
            emit(IContentProtocol_Error(errorInfo));
            return;
        }

        // If we haven't calculated the work to do yet
        if (_dataToSave == 0)
        {
            // Setup the package file entry metadata map
            ORIGIN_LOG_DEBUG << PKGLOG_PREFIX << "Setting up package file entry map";
            if (!ComputeDownloadJobSizeBegin())
            {
                ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Unable to setup package file entry map";

                ClearRunState();
                
                CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
                errorInfo.mErrorType = UnpackError::IOCreateOpenFailed;
                errorInfo.mErrorCode = Services::PlatformService::lastSystemResultCode();
                emit(IContentProtocol_Error(errorInfo));
                return;
            }
        }
        else
        {
            // Skip the asynchronous bit and just continue on
            TransferStartJobReady();
        }

        // DO NOT put any code here, ComputeDownloadJobSizeBegin can return asynchronously
    }

    void ContentProtocolPackage::TransferStartJobReady()
    {
        // Save off the CRC map
        if (!_crcMap->Save())
        {
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Unable to write CRC Map to: " << _crcMap->getCRCMapPath();

            ClearRunState();
            
            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = UnpackError::IOWriteFailed;
            errorInfo.mErrorCode = Services::PlatformService::lastSystemResultCode();
            emit(IContentProtocol_Error(errorInfo));
            return;
        }

        // There's nothing to do here, transition right to finished.
        // Exception(s):
        // https://developer.origin.com/support/browse/OFM-2069
        // always run the repair flow through all transitions to trigger the touchup installer execution at the end
		// https://developer.origin.com/support/browse/EBIBUGS-27226
		// if we are not a pure update (repairs can become updates) then run the full flow as we are not installed

        if(!checkIfWorkToDo() && _fIsRepair == false && _fIsUpdate == true)
        {
            emit Finished();
        }

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

        if(_fIsRepair)
        {
            // TELEMETRY: We've started downloading the replacement files.
            GetTelemetryInterface()->Metric_REPAIRINSTALL_REPLACINGFILES(mProductId.toUtf8().data());
        }

        // Start the Patch Builder going if necessary
        if (_fIsDiffUpdatingAvailable && _patchBuilder)
        {
            _patchBuilder->Start();
        }

        // Send out the actual requests
        TransferIssueRequests();
    }

    void ContentProtocolPackage::TransferIssueRequests()
    {
        QMap<int, QVector<RequestId> > requestGroups;

        // Go over all the files in the archive and issue requests for them if necessary
        int c = 0;
        bool endOfDisc = false;

        // If the priority group list doesn't contain any groups, insert the default 0 group
        if (!_downloadReqPriorityGroups.contains(0))
        {
            _downloadReqPriorityGroups[0].groupId = 0;
        }

        // Group 0 is required by definition
        _downloadReqPriorityGroups[0].requirement = Engine::Content::kDynamicDownloadChunkRequirementRequired;
        
        //use the sorted package file list so that we can preserve the files by disk order for multi disc zips
        for (QList<PackageFileEntry*>::Iterator it = _packageFilesSorted.begin(); it != _packageFilesSorted.end() && !endOfDisc; it++)
        {
            PackageFileEntry *pPackageEntry = *it;
            PackageFileEntryMetadata &fileMetadata = _packageFiles[pPackageEntry];

            // See if this file is being diff updated
            bool fileIsBeingDiffUpdated = fileMetadata.isBeingDiffUpdated;

            QString sFilename(pPackageEntry->GetFileName());

            //RetrieveOffsetToFileData guarantees that our offsets are valid
            //For multi file zips, the offsets for the last files in  those zips can not be calculated ahead of time
            //RetrieveOffsetToFileData gets the file offset for these files when needed
            if (_packageType == ContentPackageType::Zip)
            {
                RetrieveOffsetToFileData(pPackageEntry);
            }

            if (fileMetadata.bytesSaved != pPackageEntry->GetUncompressedSize())
            {
                ++c;

#ifdef DOWNLOADER_CRC_DEBUG
                ORIGIN_LOG_EVENT << "Job Start - File in job - " << pPackageEntry->GetFileName();
#endif

                if (!fileIsBeingDiffUpdated)
                {
                    RequestId downloadId;
                    UnpackStreamFileId unpackId;
                    ContentDownloadError::type errCode = CreateDownloadUnpackRequest(pPackageEntry, downloadId, unpackId, false, endOfDisc);
                    if (ContentDownloadError::OK == errCode)
                    {
                        // Save the download request Id in case we need to refer to it later
                        fileMetadata.downloadId = downloadId;

                        bool groupFound = false;
                        if (_fIsDynamicDownloadingAvailable)
                        {
                            for (QMap<int, DownloadPriorityGroupMetadata>::iterator groupIter = _downloadReqPriorityGroups.begin(); groupIter != _downloadReqPriorityGroups.end(); ++groupIter)
                            {
                                // If this entry belongs to this group
                                if (groupIter.value().entries.contains(pPackageEntry))
                                {
                                    groupFound = true;

                                    // Initialize the per-file data
                                    DownloadPriorityGroupEntryMetadata& entryMetadata = groupIter.value().entries[pPackageEntry];
                                    entryMetadata.status = kEntryStatusQueued;
                                    entryMetadata.totalBytesRequested = pPackageEntry->GetUncompressedSize();

                                    requestGroups[groupIter.key()].push_back(downloadId);
                                }
                            }
                        }

                        // Fallback is group 0
                        if (!groupFound)
                        {
                            // Initialize the per-file data
                            DownloadPriorityGroupEntryMetadata& fileMetadata = _downloadReqPriorityGroups[0].entries[pPackageEntry];
                            fileMetadata.status = kEntryStatusQueued;
                            fileMetadata.totalBytesRequested = pPackageEntry->GetUncompressedSize();

                            requestGroups[0].push_back(downloadId);
                        }
                    }
                    else if ((ContentDownloadError::type)ProtocolError::EndOfDisc != errCode)
                    {
                        ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Failed to create Download/Unpack Request for file: " << sFilename << " Error: " << Downloader::ErrorTranslator::ErrorString(errCode);
    
                        GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mProductId.toLocal8Bit().constData(), "TransferIssueRequests_Failure", sFilename.toLocal8Bit().constData(), __FILE__, __LINE__);
    
                        // Something went wrong and we weren't able to create a download/unpack request, so bail out
                        ClearRunState();
            
                        CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
                        errorInfo.mErrorType = errCode;
                        errorInfo.mErrorCode = 0;
                        emit(IContentProtocol_Error(errorInfo));

                        // Stop the download, the flow will restart us if appropriate
                        Pause();

                        return;
                   }
                }
            }
        }

        // Start the progress update timer
        if (!_progressUpdateTimer.isActive())
        {
            // Update progress twice a second
            _progressUpdateTimer.start(500);
        }

        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Total requests: " << c;

        // Go through all the priority groups and submit the requests
        // For non-dynamic downloads, this will only be group 0, which basically means everything is equal priority
        // Submit the requests in order of requirement priority (required->recommended->etc)
        QVector<Engine::Content::DynamicDownloadChunkRequirementT> chunkRequirementPriority = Engine::Content::DynamicDownloadChunkRequirementDefaultOrder();
        for (QVector<Engine::Content::DynamicDownloadChunkRequirementT>::iterator requirementIter = chunkRequirementPriority.begin(); requirementIter != chunkRequirementPriority.end(); ++requirementIter)
        {
            Engine::Content::DynamicDownloadChunkRequirementT curRequirement = *requirementIter;

            ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Submitting " << Engine::Content::DynamicDownloadChunkRequirementToString(curRequirement) << " groups.";

            // Only required groups are enabled by default
            bool groupEnabled = (curRequirement == Engine::Content::kDynamicDownloadChunkRequirementRequired) ? true : false;

            // Save the state of the group
            _downloadReqPriorityGroupsEnableState[curRequirement] = groupEnabled;

            for (QMap<int, QVector<RequestId> >::iterator reqIter = requestGroups.begin(); reqIter != requestGroups.end(); ++reqIter)
            {
                DownloadPriorityGroupMetadata& groupMetadata = _downloadReqPriorityGroups[reqIter.key()];

                // Skip groups that aren't at the current requirement level
                if (curRequirement != groupMetadata.requirement)
                    continue;

                ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Priority Group #" << reqIter.key() << " - Submitting: " << reqIter.value().size();

                groupMetadata.groupEnabled = groupEnabled;
                groupMetadata.status = Engine::Content::kDynamicDownloadChunkState_Queued;
                groupMetadata.hadUpdates = true;
                groupMetadata.inactivityCount = 1;

                // Submit the requests
                _downloadSession->submitRequests(reqIter.value(), reqIter.key(), groupEnabled);
            }
        }


        // If we're tracking priority groups
        if (_fIsDynamicDownloadingAvailable)
        {
            // Update the stats (pushes them out to the DDC)
            UpdatePriorityGroupAggregateStats();

            // Send an empty order list to generate a queue order update event (this has no effect on the queue order)
            _downloadSession->prioritySetQueueOrder(QVector<int>());
        }

        mTransferStatsDecompressed.clear();
        mTransferStatsDownload.clear();
        mTransferStats.onStart(_dataToSave, _dataSaved);

        ScanForCompletion();

        CheckTransferState();
    }

    ContentDownloadError::type ContentProtocolPackage::TransferReIssueRequest(PackageFileEntry* pEntry, ContentDownloadError::type reasonErrType, bool clearExistingFile, bool enableDiagnostics)
    {
        // Update the package file entry metadata
        PackageFileEntryMetadata& fileMetadata = _packageFiles[pEntry];

        // Don't overwrite the last error type if we don't know what the error is (keep the last known)
        if (reasonErrType != static_cast<ContentDownloadError::type>(ProtocolError::UNKNOWN))
        {
            // Store the reason code why we are re-requesting
            fileMetadata.lastErrorType = reasonErrType;
        }

        // If we're wiping the existing file, it counts against the restart count, not the retry count (which is more permissive)
        if (clearExistingFile)
        {
            fileMetadata.restartCount++;

            // Reset the priority group tracking counters
            UpdatePriorityGroupChunkMap(pEntry, 0, pEntry->GetUncompressedSize(), kEntryStatusQueued);
        }
        else
        {
            fileMetadata.retryCount++;
        }

        ORIGIN_LOG_WARNING << PKGLOG_PREFIX << "Redownloading: " << pEntry->GetFileName() << " Restart: " << (clearExistingFile ? QString("true") : QString("false")) << " RetryCount: " << fileMetadata.retryCount << " RestartCount: " << fileMetadata.restartCount;
        QString telemStr = QString("File=%1,ClearExisting=%2,RetryCount=%3,RestartCount=%4").arg(pEntry->GetFileName()).arg(clearExistingFile ? QString("true") : QString("false")).arg(fileMetadata.retryCount).arg(fileMetadata.restartCount);
        GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mProductId.toLocal8Bit().constData(), "TransferReIssueRequest", qPrintable(telemStr), __FILE__, __LINE__);

        // Don't retry forever
        int maxRetryCount = MAX_RETRY_COUNT;
        int maxRestartCount = MAX_RESTART_COUNT;
        if (_fIsDynamicDownloadingAvailable)
        {
            // More permissive error counts for dynamic downloads
            maxRetryCount *= 2;
            maxRestartCount *= 2;
        }
        if (fileMetadata.retryCount > maxRetryCount || fileMetadata.restartCount > maxRestartCount)
        {
            // Update the priority group tracking counters with the error state information
            UpdatePriorityGroupChunkMap(pEntry, 0, pEntry->GetUncompressedSize(), kEntryStatusError, (int)fileMetadata.lastErrorType);

            QString retryCountString = QString("retryCount=%1,restartCount=%2").arg(fileMetadata.retryCount).arg(fileMetadata.restartCount);
            GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mProductId.toLocal8Bit().constData(), "TransferReIssueRequestRetry", retryCountString.toLocal8Bit().constData(), __FILE__, __LINE__);
            return (ContentDownloadError::type)ProtocolError::RedownloadTooManyRetries;
        }

        // Create and submit a new request
        UnpackStreamFileId newUnpackId;
        RequestId newReqId;
        bool endOfDisc;
        ContentDownloadError::type errCode = CreateDownloadUnpackRequest(pEntry, newReqId, newUnpackId, clearExistingFile, endOfDisc);
        if (ContentDownloadError::OK == errCode)
        {
            // Save the download request Id in case we need to refer to it later
            fileMetadata.downloadId = newReqId;

            // If diagnostics are enabled for this request, turn it on
            if (enableDiagnostics)
            {
                Downloader::IDownloadRequest* downloadReq = _downloadSession->getRequestById(newReqId);
                if (downloadReq)
                {
                    downloadReq->setDiagnosticMode(true);
                }
            }

            // Resubmit the request
            _downloadSession->submitRequest(newReqId, GetEntryPriorityGroups(pEntry));
            return ContentDownloadError::OK;
        }
        GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mProductId.toLocal8Bit().constData(), "TransferReIssueRequestUnpack", "none", __FILE__, __LINE__);
        return errCode;
    }

    ContentDownloadError::type ContentProtocolPackage::CreateDownloadUnpackRequest(PackageFileEntry* pEntry, RequestId &downloadReqId, UnpackStreamFileId &unpackFileId, bool clearExistingFiles, bool &endOfDisc)
    {
        TIME_BEGIN("ContentProtocolPackage::CreateDownloadUnpackRequest");
        // Get the package file entry metadata
        PackageFileEntryMetadata& fileMetadata = _packageFiles[pEntry];

        // Hack for History
        // Unfortunately there have been a few titles published (on DVD and digitally) that have particular files within them with CRC discrepancies.
        // Because they're already live and we can't exactly reissue new DVDs we have to handle it ourselves.  Turning OFF CRC validation is not desirable.
        // Writing a data-driven system to handle these files is overkill as we're not planning on having more of these.
        // The simplest thing to do is to be aware of the individual files that exhibit these issues and simply do not do CRC checking for them.
        // Here we'll check against these specific files and pass a 0 CRC value into createUnpackFile (which is an indicator to NOT perform CRC validation.)
        // Note: Even though it's possible for there to be a name collision with these file paths in other content, the possibility is so unlikely that it's
        // not worth attempting to check both file path AND content ID.

        quint32 nCRCToPass = pEntry->GetFileCRC();

        // Only perform a possible CRC check bypass if this an ITO installation
        if (_physicalMedia)
        {
            // Here is the array of files to ignore CRC checks
            static const char* badCRCFileArray[] = {   "Data/bom.fb2",                                                              // BF3
                                                    "Data/initfs_Win32",                                                               // BF3
                                                    "BIOGame/CookedPCConsole/Wwise_Crt_Cerb_Nem_SFX.afc",                              // ME3
                                                    "BIOGame/CookedPCConsole/Wwise_Crt_Reap_Husk_SFX.afc",                             // ME3
												    "Game/Bin/GraphicsRules.sgr",							                            // Sims3 MasterSuite stuff
                                                    "Game/data7.big",                                                                  // FIFA 12 & FIFA 13
                                                    "Data/Win32/_c4/Levels/Level_0400_SierraPass/Level_0400_SierraPass.toc",           // NFS:TR
                                                    "Data/Win32/_c4/Levels/Level_0500_DesertHills/Level_0500_DesertHills_Terrain.toc", // NFS:TR
                                                    "BioGame/CookedPC/Packages/GameObjects/Characters/BIOG_Female_Player_C.upk",       // ME1 : trilogy disc 1
                                                    "BioGame/CookedPC/Packages/GameObjects/Characters/BIOG_Male_Player_C.upk",         // ME1 : trilogy disc 2
                                                    NULL                                                                               // for loop terminator
                                                };

            QString entryFilename(pEntry->GetFileName());
            entryFilename.replace('\\', '/');
            for (int i = 0; badCRCFileArray[i]; i++)
            {
                QString badFile(badCRCFileArray[i]);
                if (entryFilename.compare(badFile, Qt::CaseInsensitive) == 0)
                {
                    ORIGIN_LOG_WARNING << PKGLOG_PREFIX << "Detected known invalid CRC: " << fileMetadata.diskFilename;
                    GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mProductId.toLocal8Bit().constData(), "SkipValidation", entryFilename.toLocal8Bit().constData(), __FILE__, __LINE__);
                    nCRCToPass = 0;
                    break;
                }
            }
        }

        // Create a new unpack file
        unpackFileId = _unpackStream->getNewId();
        UnpackError::type err = _unpackStream->createUnpackFile(unpackFileId, mProductId,
                                                                fileMetadata.diskFilename, 
                                                                pEntry->GetCompressedSize(), 
                                                                pEntry->GetUncompressedSize(), 
                                                                DateTime::QDateToMsDosDate(pEntry->GetFileModificationDate()), 
                                                                DateTime::QTimeToMsDosTime(pEntry->GetFileModificationTime()),
                                                                pEntry->GetFileAttributes(),
                                                                (UnpackType::code)pEntry->GetCompressionMethod(),
                                                                nCRCToPass,
                                                                (_physicalMedia) ? !_enableITOCRCErrors : false,    // ignore CRC errors if _enableITOCRCChecking is false
                                                                clearExistingFiles
                                                                );
        if (err != UnpackError::OK)
        {
            QString errorCode = QString("ErrorCode=%1").arg(err);
            GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mProductId.toLocal8Bit().constData(), "FileUnpackStream", errorCode.toUtf8().data(), __FILE__, __LINE__);

            if (err != UnpackError::AlreadyCompleted)
            {
                GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mProductId.toLocal8Bit().constData(), "FileUnpackStream", fileMetadata.diskFilename.toUtf8().data(), __FILE__, __LINE__);
                ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Couldn't create unpack stream for file: " << fileMetadata.diskFilename;
            }
            return (ContentDownloadError::type)err;
        }
        IUnpackStreamFile* unpackFile = _unpackStream->getUnpackFileById(unpackFileId);

        // Fix the counters
        _dataSaved -= fileMetadata.bytesSaved;
        _dataSaved += unpackFile->getSavedBytes();
        _dataDownloaded -= fileMetadata.bytesDownloaded;
        _dataDownloaded += unpackFile->getProcessedBytes();

        fileMetadata.bytesSaved = unpackFile->getSavedBytes();
        fileMetadata.bytesDownloaded = unpackFile->getProcessedBytes();

        bool headerConsumed = false;
        qint64 headerOffset = pEntry->GetOffsetToHeader();

        // If there is no header, adjust for that
        if (headerOffset == 0)
        {
            headerOffset = pEntry->GetOffsetToFileData();
            headerConsumed = true;
        }

        qint64 requestStart = headerOffset;
        qint64 requestEnd = pEntry->GetEndOffset();

        // Sanity check
        if (requestStart > requestEnd)
        {
            GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mProductId.toLocal8Bit().constData(), "CreateDownloadUnpackRequest_CentralDirInvalid", pEntry->GetFileName().toUtf8().data(), __FILE__, __LINE__);

            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << " Invalid offsets in central directory [" << requestStart << " to " << requestEnd << "]";

            return (ContentDownloadError::type)ProtocolError::ZipFileOffsetsInvalid;
        }

        endOfDisc = false;

        // If this file already has progress, adjust the start offset and mark the header as already consumed
        if (unpackFile->getProcessedBytes() > 0)
        {
            qint64 headerSize = pEntry->GetOffsetToFileData() - headerOffset;

            // TODO: Fix the offsets here for ITO?
            requestStart += headerSize + unpackFile->getProcessedBytes();

            headerConsumed = true;
        }

        if (_physicalMedia && (_packageType == ContentPackageType::Zip))
        {
            // Setup a disk offset value since spanned files user header information that is based from 
            // the disk it originates on
            // We need to subtract this value from the spanned file when we start on a new disc
            QString sFilename(pEntry->GetFileName());

			//see if this file spans
            if(_fileNameToDiscStartMap.contains(sFilename))
            {
				qint64 diskOffsetForSpannedFile = 0;

				//add up the offsets, starting from the start disc to our current disc
				for(int i = _fileNameToDiscStartMap[sFilename]; i < _currentDisc ; i++)
				{
					diskOffsetForSpannedFile += _discIndexToDiscSizeMap[i];
				}

				//subtract the offset from our request values
                requestStart -= diskOffsetForSpannedFile;
                requestEnd -= diskOffsetForSpannedFile;
            }

            // If a request is larger than the disc size we will truncate to the end of disc
            if(requestEnd >= _currentDiscSize)
            {
                requestEnd = _currentDiscSize;

                //store the file start disc for files that span disks
                if(!_fileNameToDiscStartMap.contains(sFilename))
                    _fileNameToDiscStartMap[sFilename] = _currentDisc;

                endOfDisc = true;
            }

            //If the last file on disc (both spanned or not spanned) is excluded, the above check for end of disc will never succeed.
            //This check here will make sure we notify the TransferIssueRequest that we have finished all requests on disk
            //By returning false here, we do not process the request and setting the endOfDisc breaks out of the request loop in TransferIssueRequest for this disc.
            //The package entries are sorted by disc when before passed into this function

            if(pEntry->GetDiskNoStart() > _currentDisc)
            {
                GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mProductId.toLocal8Bit().constData(), "CreateDownloadUnpackRequest_LastDisk", "none", __FILE__, __LINE__);
                endOfDisc = true;
                return (ContentDownloadError::type)ProtocolError::EndOfDisc;
            }
        }

		// We have one condition where a request spans multiple discs and we successfully downloaded the request to the end of the disc prior to resuming
		if(requestEnd == requestStart && endOfDisc)
		{
            GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mProductId.toLocal8Bit().constData(), "CreateDownloadUnpackRequest_SpanMultDisk", "none", __FILE__, __LINE__);
			return (ContentDownloadError::type)ProtocolError::EndOfDisc;
		}

		// check for and log invalid request size
		if(requestStart < 0 || requestEnd < 0 || requestEnd <= requestStart)
		{
            QString errorCode = QString("Strt=%1,End=%2").arg(requestStart).arg(requestEnd);
            GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mProductId.toLocal8Bit().constData(), "CreateDownloadUnpackRequest_Range", errorCode.toUtf8().data(), __FILE__, __LINE__);

			ORIGIN_LOG_ERROR << PKGLOG_PREFIX << " invalid request range formulated in CreateDownloadUnpackRequest [" << requestStart << " to " << requestEnd << "]";
			return (ContentDownloadError::type)ProtocolError::ComputedFileOffsetsInvalid;
		}

        // Create a new download request
        downloadReqId = _downloadSession->createRequest(requestStart, requestEnd);

        // Update the metadata maps
        DownloadRequestMetadata &downloadRequestMetadata = _downloadReqMetadata[downloadReqId];
        downloadRequestMetadata.unpackStreamId = unpackFileId;
        downloadRequestMetadata.headerConsumed = headerConsumed;
        downloadRequestMetadata.packageFileEntry = pEntry;
        downloadRequestMetadata.totalBytesRequested = requestEnd - requestStart;
        UnpackFileMetadata &unpackFileMetadata = _unpackFiles[unpackFileId];
        unpackFileMetadata.downloadRequestId = downloadReqId;
        unpackFileMetadata.packageFileEntry = pEntry;

        TIME_END("ContentProtocolPackage::CreateDownloadUnpackRequest");
        return ContentDownloadError::OK;
    }
    
    bool ContentProtocolPackage::checkIfWorkToDo()
    {
        TIME_BEGIN("ContentProtocolPackage::checkIfWorkToDo")
        // We can only know if there's work to do if we've been initialized and are resuming or starting up
        if(protocolState() != kContentProtocolStarting && protocolState() != kContentProtocolResuming)
        {
            ORIGIN_ASSERT(true);
            ORIGIN_LOG_WARNING << "Cannot check for completion of protocol work if protocol has not been initialized.";
            TIME_END("ContentProtocolPackage::checkIfWorkToDo")
            return true;
        }

        // We can't really bail on a multi-disc ITO install early and we can't make user switch disks and then switch
        // again to verify, so we always have more work to do.
        if(_physicalMedia && _numDiscs > 1)
        {
            TIME_END("ContentProtocolPackage::checkIfWorkToDo")
            return true;
        }

        // The most obvious check, have we downloaded everything?
        if(_dataSaved < _dataToSave)
        {
            TIME_END("ContentProtocolPackage::checkIfWorkToDo")
            return true;
        }

        // Finally, are there any files that need to be destaged?
        for (QMap<PackageFileEntry*, PackageFileEntryMetadata>::Iterator it = _packageFiles.begin(); it != _packageFiles.end(); it++)
        {
            PackageFileEntryMetadata &fileMetadata = it.value();
            if (fileMetadata.isStaged)
            {
                TIME_END("ContentProtocolPackage::checkIfWorkToDo")
                return true;
            }
        }

        // Nope, nothing to do homey enjoy your day off!
        TIME_END("ContentProtocolPackage::checkIfWorkToDo")
        return false;
    }

    qint64 ContentProtocolPackage::stagedFileCount()
    {
        qint64 nCount = 0;
        for (QMap<PackageFileEntry*, PackageFileEntryMetadata>::Iterator it = _packageFiles.begin(); it != _packageFiles.end(); it++)
        {
            PackageFileEntryMetadata &fileMetadata = it.value();
            if (fileMetadata.isStaged)
                nCount++;
        }

        return nCount;
    }

    bool ContentProtocolPackage::FinalizeDestageFiles(int& errorCode)
    {
        // Reset the error code
        errorCode = 0;

        // This gets called when the job is done.  Maybe we didn't get a chance to individually destage each group because it happened too quickly
        // Therefore, let's go over each group and do it one by one

        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Destaging all files!";

        // Go over all the priority groups
        for (QMap<int, DownloadPriorityGroupMetadata>::iterator groupIter = _downloadReqPriorityGroups.begin(); groupIter != _downloadReqPriorityGroups.end(); ++groupIter)
        {
            DownloadPriorityGroupMetadata& groupMetadata = groupIter.value();

            // No need to process completed groups further
            if (groupMetadata.status == Engine::Content::kDynamicDownloadChunkState_Installed)
                continue;

            // This group isn't processed yet, so destage it
            if (!FinalizeDestageFilesForGroup(groupIter.key(), errorCode))
                return false;
        }

        // Go over all the file deletions from the manifest if updating or repairing
        if (_fIsUpdate || _fIsRepair)
        {
            ProcessDeletes();
			RemoveUnusedFiles();

            // We should have destaged everything already, so clear out all the cruft (stream files, leftover staged files, patch builder files)
            if (_packageFileDirectory)
            {
                DeleteStagedFileData(*_packageFileDirectory);
            }
        }

        return true;
    }

    bool ContentProtocolPackage::FinalizeDestageFilesForGroup(int priorityGroup, int& errorCode)
    {
        // Reset the error code
        errorCode = 0;

        bool fDeleteSuccess = true;
        bool fDestageSuccess = false;

		// If we're in windows, the files to be deleted will all need to be locked first - requiring file handles with delete access.
#ifdef ORIGIN_PC
        QMap<PackageFileEntry*, HANDLE> fileHandleCache;

        const int kMaxRetriesForDelete = 40;			// for a total of 10 seconds
        const int kRetrieSleepTimeForLockForDelete = 250;	// in milliseconds
#endif
        QList<PackageFileEntry*> filesToDestage;

        // If we don't know about this group, bail out
        if (!_downloadReqPriorityGroups.contains(priorityGroup))
        {
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Unknown priority group! - " << priorityGroup;
            return false;
        }

        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Destaging files for priority group: " << priorityGroup;

        for (QMap<PackageFileEntry*, PackageFileEntryMetadata>::Iterator it = _packageFiles.begin(); it != _packageFiles.end(); it++)
        {
            PackageFileEntry *pEntry = it.key();
            PackageFileEntryMetadata &fileMetadata = it.value();

            // Only destage files from the group we requested
            if (!_downloadReqPriorityGroups[priorityGroup].entries.contains(pEntry))
                continue;

            // Count the files that we need to destage
            if (fileMetadata.isStaged)
            {
                filesToDestage.push_back(pEntry);
            }

            QString sFinalFilename = pEntry->GetFileName();
            QString sFullPath(CFilePath::Absolutize( _unpackPath, sFinalFilename ).ToString());
            
            // Lock all existing files that are going to be replaced
            if (pEntry->IsFile() && fileMetadata.isStaged && EnvUtils::GetFileExistsNative(sFullPath))
            {
                QString sFullStagedPath(CFilePath::Absolutize( _unpackPath, fileMetadata.diskFilename ).ToString());
                if (!EnvUtils::GetFileExistsNative(sFullStagedPath))
                {
                    ORIGIN_LOG_WARNING << PKGLOG_PREFIX << "Staged file did not exist, skipping replacement. ; " << sFullStagedPath;
                    continue;
                }

#ifdef ORIGIN_PC // In Windows, attempt to lock the file and add it to fileHandleCache
                
				::SetFileAttributes(sFullPath.utf16(), FILE_ATTRIBUTE_NORMAL);       // ensure file is not read-only just in case.

                int nRetriesLeft = kMaxRetriesForDelete;         // maximum times to retry locking a file for delete before erroring out
                bool bLocked = FALSE;
                while (bLocked == FALSE && nRetriesLeft-- > 0)
                {
                    HANDLE hFile = ::CreateFile(sFullPath.utf16(), GENERIC_WRITE|GENERIC_READ, FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                    if (hFile == INVALID_HANDLE_VALUE)
                    {
                        DWORD nLastError = ::GetLastError();
                        errorCode = (int)nLastError;

                        if (nLastError != ERROR_FILE_NOT_FOUND)
                        {
                            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "File couldn't be locked for deletion!: " << sFinalFilename << " - OS error: " << nLastError;
                        }

                        switch (nLastError)
                        {
                        case ERROR_FILE_NOT_FOUND:	// file isn't there so it doesn't need to be deleted.
                            bLocked = true;
                            nRetriesLeft = 0;
                            break;
                        case ERROR_SHARING_VIOLATION:
                            {
                                QString rebootReason;
                                QString lockingProcessesSummary;
                                QList<EnvUtils::FileLockProcessInfo> processes;

                                // This API only works for Vista+
                                if (EnvUtils::GetFileInUseDetails(sFullPath, rebootReason, lockingProcessesSummary, processes))
                                {
                                    if (!lockingProcessesSummary.isEmpty())
                                    {
                                        QString inUseError = QString("File (%1) is in use. %2 ; %3").arg(sFullPath).arg(rebootReason).arg(lockingProcessesSummary);
                                        ORIGIN_LOG_ERROR << inUseError;

                                        GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(qPrintable(mProductId), "DestageFileInUse", qPrintable(inUseError), __FILE__, __LINE__);
                                    }
                                }
                            }
                        case ERROR_ACCESS_DENIED:
                        case ERROR_BUSY:
                        case ERROR_LOCK_VIOLATION:
                        case ERROR_NETWORK_BUSY:
                            // wait and try again
                            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Failed to lock file for delete ....OS error code: " << nLastError << "\n";
                            Origin::Services::PlatformService::sleep(kRetrieSleepTimeForLockForDelete); // delay before trying again
                            ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Waiting and trying again. Attempt #" << (kMaxRetriesForDelete - nRetriesLeft) << "\n";
                            break;

                        default:
                            nRetriesLeft = 0;	// auto-retries are not going to work for these cases so just fail out and toss up the update error message
                            break;
                        }
                    }
                    else
                    {
                        fileHandleCache[pEntry] = hFile;
                        bLocked = true;
                    }
                }

                if (!bLocked)
                {
                    fDeleteSuccess = false;
                    break;
                }
#else// Mac implementation
				// On Mac, instead of locking the file, simply delete the file
				QFile fileToDelete(sFullPath);
				bool bSuccess = fileToDelete.remove();
				if (!bSuccess)
				{
					ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "File could not be deleted!: " << sFinalFilename << " - QFile error: " << fileToDelete.error();
					fDeleteSuccess = false;
                    GetTelemetryInterface()->Metric_ERROR_DELETE_FILE("ContentProtocolPackage_FinalizeDestageFiles", sFullPath.toUtf8().data(), fileToDelete.error());
				}
#endif

            }
        }

		
#ifdef ORIGIN_PC // In windows, delete the files (if all were locked successfully), release the handles.
        
		ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Destaging; Files locked for deletion: " << fileHandleCache.size();

        // Go over all the files we locked
        for (QMap<PackageFileEntry*, HANDLE>::Iterator it = fileHandleCache.begin(); it != fileHandleCache.end(); it++)
        {
            PackageFileEntry *pEntry = it.key();
            HANDLE hFile = it.value();

            // If we had succeeded in locking all of them, delete them
            if (fDeleteSuccess)
            {
                QString sFullPath(CFilePath::Absolutize( _unpackPath, pEntry->GetFileName() ).ToString());

                if (::DeleteFile(sFullPath.utf16()) == FALSE)
                {
                    if (::GetLastError() != ERROR_FILE_NOT_FOUND)
                    {
                        // if for whatever reason the file couldn't be deleted even though the file was locked, something went horribly wrong
                        ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "File has been locked for deletion but deletion failed!: " << pEntry->GetFileName();
                    }
                }
            }

            // Either way, close the handle
            CloseHandle(hFile);

            // Remove the handle from the map
            it.value() = NULL;
        }

        // Erase the map
        fileHandleCache.clear();

        const int kMaxRetriesForMove = 40;			// for a total of 10 seconds
        const int kRetrieSleepTimeForMove = 250;	// in milliseconds

#endif // ORIGIN_PC


        // If we were successful in locking everything
        if (fDeleteSuccess)
        {
            // Set our initial state to successful
            fDestageSuccess = true;

            // TODO: Figure out how to handle it if this thing fails somewhere in the middle

            ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Destaging; Staged files: " << filesToDestage.count();

            // Go over all the files we will destage
            for (QList<PackageFileEntry*>::Iterator it = filesToDestage.begin(); it != filesToDestage.end(); it++)
            {
                PackageFileEntry *pEntry = *it;
                PackageFileEntryMetadata &fileMetadata = _packageFiles[pEntry];

                // Make sure we only operate on staged files
                if (pEntry->IsFile() && fileMetadata.isStaged)
                {
                    QString sFullStagedPath(CFilePath::Absolutize( _unpackPath, fileMetadata.diskFilename ).ToString());
                    QString sFullFinalPath(CFilePath::Absolutize( _unpackPath, pEntry->GetFileName() ).ToString());

                    if (EnvUtils::GetFileExistsNative(sFullFinalPath))
                    {
                        ORIGIN_LOG_WARNING << PKGLOG_PREFIX << "File existed in final location, can't de-stage. ; " << sFullFinalPath;
                        continue;
                    }

                    if (!EnvUtils::GetFileExistsNative(sFullStagedPath))
                    {
                        ORIGIN_LOG_WARNING << PKGLOG_PREFIX << "Staged file missing. ; " << sFullStagedPath;
                        continue;
                    }

#ifdef ORIGIN_PC // In Windows, it may require many attempts to move(rename) the file

                    int nRetriesLeft = kMaxRetriesForMove;         // maximum times to retry renaming a file before erroring out
                    bool bMoved = FALSE;
                    DWORD nLastError = ERROR_SUCCESS;
                    while (bMoved == FALSE && nRetriesLeft-- > 0)
                    {
                        // Try to move the file
                        if (::MoveFile(sFullStagedPath.utf16(), sFullFinalPath.utf16()) == FALSE)
                        {
                            nLastError = ::GetLastError();
                            errorCode = (int)nLastError;

                            // if for whatever reason the file couldn't be deleted even though the file was locked, something went horribly wrong
                            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "File could not be renamed to final name!: " << pEntry->GetFileName() << "; GetLastError: " << nLastError;

                            switch (nLastError)
                            {
                            case ERROR_ACCESS_DENIED:
                            case ERROR_BUSY:
                            case ERROR_SHARING_VIOLATION:
                            case ERROR_LOCK_VIOLATION:
                            case ERROR_NETWORK_BUSY:
                                // wait and try again
                                ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Failed to rename file....OS error code: " << nLastError << "\n";
                                Origin::Services::PlatformService::sleep(kRetrieSleepTimeForMove); // delay before trying again
                                ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Waiting and trying again. Attempt #" << (kMaxRetriesForMove - nRetriesLeft) << "\n";
                                break;

                            default:
                                nRetriesLeft = 0;	// auto-retries are not going to work for these cases so just fail out and toss up the update error message
                                break;
                            }
                        }
                        else
                        {
                            bMoved = true;
                        }
                    }

#else  // Mac implementation.  Rename the file from the staged name to the final name.

					bool bMoved = false;
					QDir qdir;
					bMoved = qdir.rename(sFullStagedPath, sFullFinalPath);
#endif

                    if (!bMoved)	// failed
                    {
                        fDestageSuccess = false;

                        // TODO: Add some telemetry here
                        // TELEMETRY: We were not able to de-stage the file.

                        ORIGIN_LOG_ERROR << "Unable to destage file.  Staged: " << sFullStagedPath << " Final: " << sFullFinalPath;
                    }
                    // We only want to commit these changes to the CRC map if destaging succeeds!
                    else
                    {
                        QString sFinalFilename = pEntry->GetFileName();
                        QString sFullPath(CFilePath::Absolutize( _unpackPath, sFinalFilename ).ToString());

                        // Update the CRC Map
                        QString crcMapFilePath = GetCRCMapFilePath(sFinalFilename);
                        _crcMap->AddModify(crcMapFilePath, pEntry->GetFileCRC(), pEntry->GetUncompressedSize(), _crcMapKey);
                        _crcMap->SetJobId(crcMapFilePath, getJobId());

                        // Remove the staged entry if it existed
                        QString stagedCrcMapFilePath = GetCRCMapFilePath(fileMetadata.diskFilename);
                        if (fileMetadata.isStaged && _crcMap->Contains(stagedCrcMapFilePath))
                        {
                            _crcMap->Remove(stagedCrcMapFilePath);
                        }

                        // We succeeded, turn off the staging marker since this file is no longer staged
                        fileMetadata.isStaged = false;
                        fileMetadata.diskFilename = pEntry->GetFileName(); // Restore the final (unstaged) filename
                    }
                }
            }
        }

        // Success or failure, we may have made modifications to the disk, so we must save the CRC map here
        ORIGIN_LOG_DEBUG << PKGLOG_PREFIX << "Saving CRC Map...";
        if (!_crcMap->Save())
        {
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Unable to write CRC Map to: " << _crcMap->getCRCMapPath();
                
            // Arbitrarily assign this case to -1 (doesn't overlap Win32 errors)
            errorCode = -1;

            return false;
        }

        if (fDestageSuccess)
        {
            if(_fIsRepair)
            {
                // TELEMETRY: We've finished repairing the broken files.
                GetTelemetryInterface()->Metric_REPAIRINSTALL_REPLACINGFILESCOMPLETED(mProductId.toUtf8().data());
            }

            ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "De-staging complete!";

            DownloadPriorityGroupMetadata& groupMetadata = _downloadReqPriorityGroups[priorityGroup];

            groupMetadata.status = Origin::Engine::Content::kDynamicDownloadChunkState_Installed;

            if (_fIsDynamicDownloadingAvailable)
            {
                // Emit the relevant signals for our listeners
                emit DynamicChunkUpdate(priorityGroup, groupMetadata.pctComplete, groupMetadata.totalBytesRead, groupMetadata.totalBytesRequested, groupMetadata.status);
		        emit DynamicChunkInstalled(priorityGroup);
            }

            return true;
        }
        return false;
    }

    bool ContentProtocolPackage::ProcessDeletes()
    {
        if (_deletePatterns.isEmpty())
        {
            ORIGIN_LOG_EVENT << "Processing Deletes - Nothing specified to delete.";
            return true;
        }

        ORIGIN_LOG_EVENT << "Processing Deletes";

        QString unpackPathWithTrailingSlash(_unpackPath);
        unpackPathWithTrailingSlash.replace("\\", "/");		// ensure only forward slashes
        if (unpackPathWithTrailingSlash.right(1) != "/")
            unpackPathWithTrailingSlash += "/";

        long filesDeleted = 0;
        long foldersDeleted = 0;

        // 1 Scan the game structure only if there are any wildcards specified
        QList<QFileInfo> entries;
        QDir directory(_unpackPath);

        // Do any patterns contain wildcards?
        for (QStringList::iterator patternIt = _deletePatterns.begin(); patternIt != _deletePatterns.end(); patternIt++)
        {
            QString pattern = unpackPathWithTrailingSlash + *patternIt;
            // Is this a wildcard pattern?
            if (pattern.contains(QChar('*')) || pattern.contains(QChar('?')) || pattern.contains(QChar('[')))
            {
                ORIGIN_LOG_EVENT << "Delete patterns contain wildcards.  Scanning game folder: " << _unpackPath;
                EnvUtils::ScanFolders(_unpackPath, entries);
                break;
            }
        }

        for (QStringList::iterator patternIt = _deletePatterns.begin(); patternIt != _deletePatterns.end(); patternIt++)
        {
            QString pattern = unpackPathWithTrailingSlash + *patternIt;
            pattern.replace("\\", "/");		// ensure only forward slashes

            // Wildcard deletion
            if (pattern.contains(QChar('*')) || pattern.contains(QChar('?')) || pattern.contains(QChar('[')))
            {
                // 2 For each pattern in _deletePatterns
                //   If the filename or folder matches delete it
                for (QList<QFileInfo>::iterator it = entries.begin(); it != entries.end(); it++)
                {
                    QFileInfo& fileInfo = *it;

                    if (fileInfo.isFile())
                    {
                        QString filePath(fileInfo.absoluteFilePath());
                        pattern.replace("\\", "/");		// ensure only forward slashes

                        if (EA::IO::FnMatch(pattern.toStdWString().c_str(), filePath.toStdWString().c_str(), EA::IO::kFNMCaseFold|EA::IO::kFNMPathname|EA::IO::kFNMUnixPath))
                        {
                            // Potentially this matched and was deleted by another pattern.
                            if(directory.exists(filePath))
                            {
                                ORIGIN_LOG_EVENT << "Deleting File: \"" << filePath << "\"";
                                if(!directory.remove(filePath))
                                {
                                    ORIGIN_LOG_ERROR << "Failed to delete file: \"" << filePath << "\"";
                                }
                                else
                                {
                                    filesDeleted++;
                                }
                            }
                        }
                    }
                }

                for (QFileInfoList::Iterator it = entries.begin(); it != entries.end(); it++)
                {
                    QFileInfo& fileInfo = *it;
                    if (fileInfo.isDir())
                    {
                        QString folderPath(fileInfo.absoluteFilePath());
                        pattern.replace("\\", "/");		// ensure only forward slashes

                        if (EA::IO::FnMatch(pattern.toStdWString().c_str(), folderPath.toStdWString().c_str(), EA::IO::kFNMCaseFold|EA::IO::kFNMPathname|EA::IO::kFNMUnixPath))
                        {
                            // Potentially this matched and was deleted by another pattern.
                            if(directory.exists(folderPath))
                            {
                                ORIGIN_LOG_EVENT << "Deleting Folder: \"" << folderPath << "\"";
                                if(!directory.rmdir(folderPath))
                                {
                                    ORIGIN_LOG_ERROR << "Failed to remove folder \"" << folderPath << "\"";
                                }
                                else
                                {
                                    foldersDeleted++;
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                // Straight forward deletion
                if (pattern.right(1) == "/")
                {
                    if(QDir(pattern).exists())
                    {
                        // Delete the folder
                        ORIGIN_LOG_EVENT << "Deleting Folder:" << pattern;
                        if(!EnvUtils::DeleteFolderIfPresent(pattern, false, true))
                        {
                            ORIGIN_LOG_ERROR << "Failed to delete folder: " << pattern;
                        }
                        else
                        {
                            foldersDeleted++;
                        }
                    }
                }
                else if(EnvUtils::GetFileExistsNative(pattern))
                {
                    // Delete the file
                    ORIGIN_LOG_EVENT << "Deleting File:" << pattern;
                    if(Origin::Util::DiskUtil::DeleteFileWithRetry(pattern))
                    {
                        filesDeleted++;
                    }
                }
            }
        }

        ORIGIN_LOG_EVENT << "Processing Deletes Completed.  Files Deleted:" << filesDeleted << " Folders Deleted:" << foldersDeleted;

        return true;
    }

    void ContentProtocolPackage::RemoveUnusedFiles()
    {
        ORIGIN_LOG_EVENT << "Deleting Unused Files";

        bool updatedCRCMap = false;

        QList<QString> localFiles = _crcMap->GetAllKeys();
        foreach (QString baseGameRelativeLocalFile, localFiles)
        {
            // Do we have a valid product Id (could be an older CRCMap format) and does it match the current download?
            // If not, skip this file
            CRCMapData crcData = _crcMap->GetCRC(baseGameRelativeLocalFile);
            if (crcData.id.isEmpty() || crcData.id != _crcMapKey)
            {
                continue;
            }

            QString packageRelativeLocalFile = baseGameRelativeLocalFile;
            if (!_crcMapPrefix.isEmpty())
            {
                // CRC Map entries are relative to the base game always, however so-called 'Type A' DLC packages are *not* relative to the base game
                // In order to transform the CRC map entries into entries that match the DiP package, we must remove the prefix, which is usually
                // something like '/DLC/EP1/' or another subfolder which contains DLC.
                // Note: Base games and 'Type B' DLC (most content) do not have a prefix value
                if (packageRelativeLocalFile.startsWith(_crcMapPrefix, Qt::CaseInsensitive))
                {
                    packageRelativeLocalFile.remove(0, _crcMapPrefix.length());
                }
            }

            // Are we still using this file?
            const PackageFileEntry* entry = _packageFileDirectory->GetEntryByName(packageRelativeLocalFile, false);
            if (!entry)
            {
                // This file is no longer included in the package, so we are good to remove the entry
                QString fullFileName(GetFileFullPath(packageRelativeLocalFile));

                ORIGIN_LOG_EVENT << "Deleting unused file:" << fullFileName;
                Origin::Util::DiskUtil::DeleteFileWithRetry(fullFileName);
                _crcMap->Remove(baseGameRelativeLocalFile);

                updatedCRCMap = true;
            }
        }

        // Make sure to save our changes to the map
        if (updatedCRCMap)
        {
            if (!_crcMap->Save())
                ORIGIN_LOG_ERROR << "Unable to save CRCMap after removing unused files";
        }
    }

    void ContentProtocolPackage::ScanForCompletion()
    {
        TIME_BEGIN("ContentProtocolPackage::ScanForCompletion")

        if (_downloadReqMetadata.count() > 0 || _unpackFiles.count() > 0)
        {
            TIME_END("ContentProtocolPackage::ScanForCompletion")
            return;
        }

        if (protocolState() != kContentProtocolRunning)
        {
            TIME_END("ContentProtocolPackage::ScanForCompletion")
            return;
        }

        // DEBUG
        UpdatePriorityGroupAggregateStats();

        // If we're diff updating, see if the PatchBuilder is still busy
        if (_fIsDiffUpdatingAvailable && _diffPatchFiles.count() > 0)
        {
            TIME_END("ContentProtocolPackage::ScanForCompletion")
            return;
        }

        //if we are on the last disc or a regular dip download end the download, if not, pause protocol and change the disc
        if(_currentDisc >= (_numDiscs - 1))
        {
            // Walk over all files in the archive and check that the scanned FileProgressMap is up to date
            int redownloadCount = 0;
            for (QMap<PackageFileEntry*, PackageFileEntryMetadata>::Iterator it = _packageFiles.begin(); it != _packageFiles.end(); it++)
            {
                PackageFileEntry *pPackageEntry = it.key();
                PackageFileEntryMetadata &fileMetadata = it.value();

                // Don't bother with diff updatable files if we're diff updating
                if (_fIsDiffUpdatingAvailable && fileMetadata.isBeingDiffUpdated)
                    continue;

                //	If the bytes saved matches the uncompressed bytes in the source file, the file is complete
                qint64 nFullSize = pPackageEntry->GetUncompressedSize();

                if (nFullSize > 0)
                {
                    qint64 nSaved = fileMetadata.bytesSaved;

                    if (nSaved != nFullSize)
                    {
                        // We missed a file
                        ++redownloadCount;
                        
                        ORIGIN_LOG_WARNING << PKGLOG_PREFIX << "WARNING: File needs to be redownloaded: " << pPackageEntry->GetFileName();

                        GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mProductId.toLocal8Bit().constData(), "MismatchSizeRedownloadRequest", pPackageEntry->GetFileName().toUtf8().data(), __FILE__, __LINE__);
                            
                        // Try to re-issue the request if we can (default to clobbering the file since we don't know how we got here)
                        ContentDownloadError::type errCode = TransferReIssueRequest(pPackageEntry, static_cast<ContentDownloadError::type>(ProtocolError::UNKNOWN), true);
                        if (ContentDownloadError::OK != errCode)
                        {
                            GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mProductId.toLocal8Bit().constData(), "MismatchSizeRedownloadRequestFailed", pPackageEntry->GetFileName().toUtf8().data(), __FILE__, __LINE__);

                            // We can't reissue it, there is a more serious problem, we need to shutdown

                            // Issue an invalid content error
                            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
                            errorInfo.mErrorType = errCode;
                            errorInfo.mErrorCode = it.value().lastErrorType;
                            
                            emit IContentProtocol_Error(errorInfo);

                            // Halt everything and report an error
                            Pause();

                            TIME_END("ContentProtocolPackage::ScanForCompletion")
                            return;
                        }
                    }
                }
            }

            // If we didn't catch everything, we're not done yet
            if (redownloadCount > 0)
            {
                GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mProductId.toLocal8Bit().constData(), "ScanForCompletion", "IncompleteFilesDetected", __FILE__, __LINE__);

                ORIGIN_LOG_WARNING << PKGLOG_PREFIX << "WARNING: " << redownloadCount << " Incomplete files detected.  Attempting to redownload.";

                TIME_END("ContentProtocolPackage::ScanForCompletion")
                return;
            }

            mTransferStats.onFinished();

            // Finalize and destage files if we weren't an update
            if (!_fIsUpdate)
            {
                int errorCode = 0;
                bool finalized = FinalizeDestageFiles(errorCode);

                // Clear our state
                ClearRunState();

                if (!finalized)
                {
                    CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
                    errorInfo.mErrorType = ProtocolError::DestageFailed;
                    errorInfo.mErrorCode = errorCode;
                    emit IContentProtocol_Error(errorInfo);
                    emit Paused();
                    TIME_END("ContentProtocolPackage::ScanForCompletion")
                    return;
                }
            }

            // Mark ourselves as finished
            setProtocolState(kContentProtocolFinished);

			//seems like the best place to reset these if we completed the install
            _currentDisc = 0;
            _requestedDisc = 0;

            emit (Finished());
        }

        TIME_END("ContentProtocolPackage::ScanForCompletion")
    }

    bool ContentProtocolPackage::RetrieveOffsetToFileData( PackageFileEntry *pEntry )
    {
        if (!pEntry)
            return false;

        //Skip already verified entries
        if ( pEntry->IsVerified() )
            return true;

        if ( pEntry->IsDirectory() )
            return true;

		//see if our map has an entry and use that offset 
		//when trying to RetrieveOffsetToFile data will fail, if the file you are trying
		//to get the offset for does not begin on the current disc
		if(_headerOffsetTofileDataOffsetMap.contains(pEntry->GetOffsetToHeader()) && !pEntry->IsVerified())
		{
			pEntry->setOffsetToFileData(_headerOffsetTofileDataOffsetMap[pEntry->GetOffsetToHeader()]);
			pEntry->setIsVerified(true);
			return true;
		}

        //Buffer used to fetch headers
        MemBuffer headerBuffer(ZipFileInfo::LocalFileHeaderFixedPartSize);

        qint64 startOffset = pEntry->GetOffsetToHeader();

        //Fetch the actual data
        qint64 reqId = _downloadSession->createRequest(startOffset, startOffset + headerBuffer.TotalSize());
        IDownloadRequest *request = _downloadSession->getRequestById(reqId);
        bool result = _downloadSession->requestAndWait(reqId);

        if ( result && request->chunkIsAvailable() && request->getErrorState() == 0 )
        {
            // Copy the data
            MemBuffer *reqBuffer = request->getDataBuffer();
            memcpy((char*)headerBuffer.GetBufferPtr(), (char*)reqBuffer->GetBufferPtr(), headerBuffer.TotalSize());
            request->chunkMarkAsRead();
            _downloadSession->closeRequest(reqId);
            LocalFileHeader localHeaderInfo;

            headerBuffer.Rewind();
            result = ZipFileInfo::readLocalFileHeader(localHeaderInfo, headerBuffer);

            if ( result )
            {
                qint64 dataOffset = startOffset + ZipFileInfo::LocalFileHeaderFixedPartSize +
                    localHeaderInfo.filenameLength + localHeaderInfo.extraFieldLength;
				_headerOffsetTofileDataOffsetMap[pEntry->GetOffsetToHeader()] = dataOffset;
                pEntry->setOffsetToFileData( dataOffset );
                pEntry->setIsVerified(true);
                return true;
            }

            GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mProductId.toLocal8Bit().constData(), "RetrieveOffsetToFileData", "ReadLocalFileHeaderError", __FILE__, __LINE__);

            //ORIGIN_LOG_ERROR << L"Could not load local file header for " << sLocalFilename << L".";
            return false;
        }
        else
        {
            _downloadSession->closeRequest(reqId);
        }

        return false;
    }

    bool ContentProtocolPackage::ShouldStageFile(const QString& file) const
    {
        if (_fIsUpdate)
            return true;
        if (_fIsDynamicDownloadingAvailable) // Always stage dynamic downloads
            return true;
        if (file.endsWith(".exe"))
            return true;
#if defined(ORIGIN_MAC)
        // Stage plist files on Mac to prevent launch services from getting ahead of us with bundles!
        if (file.endsWith(".plist"))
            return true;
#endif
        return false;
    }

    bool ContentProtocolPackage::IsFileNew(const PackageFileEntry* pEntry, qint64 &existingSize) const
    {
        // First, determine if this is a file that we already have
        QString sFilename = pEntry->GetFileName();
        QString crcMapFilePath = GetCRCMapFilePath(sFilename);
        if (_crcMap->Contains(crcMapFilePath))
        {
            const CRCMapData crcData = _crcMap->GetCRC(crcMapFilePath);

            // See if we're going to diff update it
            QString baseFileFullPath = GetFileFullPath(sFilename);

            bool basePathExists = false;
            qint64 basePathFileSize = 0;
            bool isSymlink = false;

            // Get the file details from disk
            GetFileDetails(baseFileFullPath, false, basePathExists, basePathFileSize, isSymlink);

            // We can only differentially update files if we have the complete original file already present
            if (basePathExists && basePathFileSize == crcData.size)
            {
                existingSize = basePathFileSize;
                return false;
            }
        }

        // We couldn't find it, it must be new
        existingSize = 0;
        return true;
    }

    bool ContentProtocolPackage::ShouldDiffUpdateFile(const PackageFileEntry* pEntry, qint64 existingSize) const
    {
        // Are differential updates disabled globally?
        if (!_fIsDiffUpdatingAvailable)
            return false;

        // For now, let's avoid the train wreck of diff updating and dynamic downloading
        // However, in practice, this shouldn't be an issue.  Dynamic 'updates' will become two-stage operations, an
        // explicit update step (in which diff patching would be available and no dynamic download is available), and
        // a dynamic download step (in which diff patching would not be available, because it only pertains to new files).
        if (_fIsDynamicDownloadingAvailable)
            return false;

        // If the file is excluded from differential updating
        if (!pEntry->IsDiffUpdatable())
            return false;

        // Check for files which don't meet the minimum size threshold
        if (pEntry->GetCompressedSize() < DIFF_UPDATE_MIN_SIZE_THRESHOLD)
            return false;

        // Check for files which are unlikely to benefit from patching (file sizes differ too greatly)
        if (abs(pEntry->GetUncompressedSize() - existingSize) > pEntry->GetCompressedSize())
            return false;

        return true;
    }

    QString ContentProtocolPackage::GetFileFullPath(const QString& file) const
    {
        return CFilePath::Absolutize( _unpackPath,  file ).ToString();
    }

    QString ContentProtocolPackage::GetStagedFilename(const QString& file) const
    {
        return file + "_DiP_STAGED";
    }

    QString ContentProtocolPackage::GetPackageManifestFilename() const
    {
        QString strippedProductId = mProductId;
        Origin::Downloader::StringHelpers::StripReservedCharactersFromFilename(strippedProductId);
        return CFilePath::Absolutize( _cachePath,  QString(strippedProductId + ".pkg") ).ToString();
    }

    QString ContentProtocolPackage::GetCRCMapFilename() const
    {
        return CFilePath::Absolutize(_cachePath, QString("map.crc") ).ToString();
    }

    QString ContentProtocolPackage::GetCRCMapFilePath(const QString& filename) const
    {
        QString filepath = filename;
        if (!_crcMapPrefix.isEmpty())
        {
            filepath.prepend(_crcMapPrefix);
        }
        return filepath;
    }

    bool ContentProtocolPackage::ProcessCRCMap()
    {
        TIME_BEGIN("ContentProtocolPackage::ProcessCRCMap")
            
        bool quickUpdate = !_fIsRepair;

        bool loadedCRCs = false;
        if (!_crcMap->IsLoaded())
        {
            // Try to load it.  If we fail, just give up and delete it anyway
            if (!_crcMap->Load())
            {
                ORIGIN_LOG_WARNING << PKGLOG_PREFIX << "WARNING: Unable to load CRC Map from: " << _crcMap->getCRCMapPath();
                _crcMap->Clear();
                _crcMap->ClearDiskFile();
            }
            else
            {
                loadedCRCs = true;
            }
        }
        else
        {
            // They were already loaded
            loadedCRCs = true;
        }
        
        // If we couldn't load the CRC map, we'll just treat the existing central directory as the truth
        if (!loadedCRCs)
        {
            // If we're doing an update, this means we need to treat it like a repair
            if (_fIsUpdate)
            {
                ORIGIN_LOG_WARNING << PKGLOG_PREFIX << "Upgrading update to repair due to CRC map load failure.";
                quickUpdate = false;
                SetRepairFlag(true);
            }
        }

        // First check for if files exist on disk but are missing from the CRC map (this can indicate we need to do a repair!)
        bool filesExisted = false;
        for (PackageFileEntryList::iterator it = _packageFileDirectory->begin();it != _packageFileDirectory->end(); it++)
        {
            PackageFileEntry* pPackageEntry = (*it);
            if (pPackageEntry->IsFile() && pPackageEntry->IsIncluded())
            {
                QString sFilename(pPackageEntry->GetFileName());

				// If the CRC map doesn't already track this entry, add it
                QString crcMapFilePath = GetCRCMapFilePath(sFilename);
				if (!_crcMap->Contains(crcMapFilePath))
				{
					// If the file already existed on disk, we missed it somehow
                    qint64 fileSize = 0;
					if (EnvUtils::GetFileDetailsNative(GetFileFullPath(sFilename), fileSize))
					{
                        ORIGIN_LOG_WARNING << PKGLOG_PREFIX << "File exists on disk but not in CRC map:" << crcMapFilePath;

                        // Add it to the map (we don't yet know the CRC)
                        _crcMap->AddModify(crcMapFilePath, 0, fileSize, _crcMapKey);

						filesExisted = true;
					}
                }
                else
                {
                    // If the file exists on disk
                    if (EnvUtils::GetFileExistsNative(GetFileFullPath(sFilename)))
                    {
                        // We do track the file in the CRC map, so make sure the stored content ID reflects that this file belongs to this content
                        _crcMap->SetContentId(crcMapFilePath, _crcMapKey);
                    }
                    else
                    {
                        // It doesn't exist on disk, kill it from the map
                        _crcMap->Remove(crcMapFilePath);
                    }
                }

                // Add an entry for whatever staged file might already exist
                if (ShouldStageFile(sFilename))
                {
				    QString sStagedFilename = GetStagedFilename(sFilename);
                    QString stagedCrcMapFilePath = GetCRCMapFilePath(sStagedFilename);
                    qint64 stagedFileSize = 0;
				    if (EnvUtils::GetFileDetailsNative(GetFileFullPath(sStagedFilename), stagedFileSize))
				    {
                        // If it wasn't in the CRC map
                        if (!_crcMap->Contains(stagedCrcMapFilePath))
                        {
                            ORIGIN_LOG_WARNING << PKGLOG_PREFIX << "File exists on disk but not in CRC map:" << stagedCrcMapFilePath;

                            // Add it to the map (we don't yet know the CRC)
                            _crcMap->AddModify(stagedCrcMapFilePath, 0, stagedFileSize, _crcMapKey);

                            filesExisted = true;
                        }
				    }
                    else
                    {
                        if (_crcMap->Contains(stagedCrcMapFilePath))
                        {
                            // It doesn't exist on disk, kill it from the map
                            _crcMap->Remove(stagedCrcMapFilePath);
                        }
                    }
                }
            }
        }

        // If we found stray files, we need to make this a repair to be safe
		if (filesExisted)
		{
            ORIGIN_LOG_WARNING << PKGLOG_PREFIX << "Upgrading to repair due to untracked files being found  (see previous messages).";
			quickUpdate = false;
			SetRepairFlag(true);
		}

        // If we're doing a repair, verify everything.  This returns immediately, as this happens asynchronously
        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Verifying CRC Map...";
        if (_crcMapKey.isEmpty())
        {
            ORIGIN_LOG_WARNING << "CRC map key is an empty string. Content = " << mProductId;
        }

        
        // We need to remove the metadata prefix from the unpack path in order for the CRC map to find the files, since the CRC map is relative to the parent content
        QString crcMapVerifyRoot = _unpackPath;
        crcMapVerifyRoot.replace("\\","/");
        // Ensure separator termination
        if (!crcMapVerifyRoot.endsWith("/"))
            crcMapVerifyRoot.append("/");
        // Remove the DLC path
        if (!_crcMapPrefix.isEmpty() && crcMapVerifyRoot.endsWith(_crcMapPrefix, Qt::CaseInsensitive))
        {
            crcMapVerifyRoot = crcMapVerifyRoot.left(crcMapVerifyRoot.count() - _crcMapPrefix.count());
        }
        // Ensure separator termination
        if (!crcMapVerifyRoot.endsWith("/"))
            crcMapVerifyRoot.append("/");

        // Start the asynchronous verify operation
        _crcMapVerifyProxy = _crcMap->VerifyBegin(quickUpdate, crcMapVerifyRoot, _crcMapKey);
        ORIGIN_VERIFY_CONNECT_EX(_crcMapVerifyProxy, SIGNAL(verifyProgress(qint64, qint64, qint64, qint64)), this, SLOT(onCRCVerifyProgress(qint64, qint64, qint64, qint64)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(_crcMapVerifyProxy, SIGNAL(verifyCompleted()), this, SLOT(onCRCVerifyCompleted()), Qt::QueuedConnection);
        _crcMapVerifyProxy->Start(); // Start the actual thread now that the signals have been connected

        TIME_END("ContentProtocolPackage::ProcessCRCMap")
        return true;
    }

    bool ContentProtocolPackage::ComputeDownloadJobSizeBegin()
    {
        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Computing download job size.";
        TIME_BEGIN("ContentProtocolPackage::ComputeDownloadJobSize")

        if (!_packageFileDirectory)
        {
            TIME_END("ContentProtocolPackage::ComputeDownloadJobSize")
            return false;
        }

        // Process the actual TOC
        ProcessPackageFileEntryMap();

        return true;
    }

    void ContentProtocolPackage::ComputeDownloadJobSizeEnd(bool usePatchBuilder, qint64 totalBytesToPatch)
    {
        // We were using a PatchBuilder class, but it failed for some reason, so we need to recompute everything locally DiP <=2.2 style
        if (!usePatchBuilder && _patchBuilder != NULL)
        {
            // Disable differential updates
            _fIsDiffUpdatingAvailable = false;

            // Kill the Patch Builder
            DestroyPatchBuilder();

            // Process the actual TOC (again)
            ProcessPackageFileEntryMap();

            return;
        }

        if (usePatchBuilder)
        {
            // Adjust the progress counters
            _dataToSave += totalBytesToPatch;
            _dataToDownload += totalBytesToPatch;

            // Counters for statistical use only
            qint64 updateBytesToDownloadNonPatchablePortion = 0; // This number represents the total compressed size of the files we can't/won't patch
            qint64 updateBytesToDownloadPatchablePortionActual = totalBytesToPatch; // This number is given to us by PatchBuilder, it represents only the bytes we actually need to download
            qint64 updateBytesToDownloadPatchablePortionTotal = 0; // This number represents the total size of the files we WILL patch, even though we may only download a portion
            qint64 updateBytesToDownloadDiP2 = 0; // Total job over-the-wire size when not patching (DiP 2)
            qint64 updateBytesToDownloadDiP3 = 0; // Total job over-the-wire size with patching (DiP 2)
            qint64 updateBytesToDownloadDiP3Savings = 0;  // Savings from using DiP 3

            // Calculate how much data we're downloading (or would download) over the actual wire
            for (QMap<PackageFileEntry*, PackageFileEntryMetadata>::iterator it = _packageFiles.begin(); it != _packageFiles.end(); ++it)
            {
                PackageFileEntry* pEntry = it.key();
                PackageFileEntryMetadata& fileMetadata = it.value();

                // If its being differentially updated
                if (fileMetadata.isBeingDiffUpdated)
                {
                    // Accumulate the compressed sizes of all the files we actually will patch
                    updateBytesToDownloadPatchablePortionTotal += pEntry->GetCompressedSize();
                }
                else
                {
                    // Accumulate the compressed sizes of all the files we can't actually patch
                    updateBytesToDownloadNonPatchablePortion += pEntry->GetCompressedSize();
                }
            }

            updateBytesToDownloadDiP2 = updateBytesToDownloadNonPatchablePortion + updateBytesToDownloadPatchablePortionTotal; // DiP 2 is Non-Patchable Portion + Patchable Portion (Total)
            updateBytesToDownloadDiP3 = updateBytesToDownloadNonPatchablePortion + updateBytesToDownloadPatchablePortionActual; // DiP 3 is Non-Patchable Portion + Patchable Portion (Actual)
            updateBytesToDownloadDiP3Savings = updateBytesToDownloadPatchablePortionTotal - updateBytesToDownloadPatchablePortionActual; // Savings is Patchable Total - Patchable Actual

            ORIGIN_LOG_EVENT << PKGLOG_PREFIX << " Patch Builder Statistics - Total Patch Size (DiP3): " << updateBytesToDownloadDiP3 << " Total Size (DiP2): " << updateBytesToDownloadDiP2 << " Non-Patched Portion: " << updateBytesToDownloadNonPatchablePortion << " Patched Portion (Actual): " << updateBytesToDownloadPatchablePortionActual << " Patched Portion (Total): " << updateBytesToDownloadPatchablePortionTotal << " Savings: " << updateBytesToDownloadDiP3Savings;

            // Send telemetry indicating that we calculated the patch building statistics
            GetTelemetryInterface()->Metric_DL_DIP3_SUMMARY( mProductId.toLocal8Bit().constData(),
                                                            (uint32_t)(_filesToSave),                               /* Total changed files       */
                                                            (uint32_t)(_diffPatchFiles.count()),                    /* Num of files diff patch   */
                                                            (uint32_t)(_filesPatchRejected),                        /* Num of files can't patch  */
                                                            (uint64_t)(_dataAdded),                                 /* Update added data (can't patch) */
                                                            (uint64_t)(updateBytesToDownloadDiP2),                  /* DiP 2 Total Update size      */
                                                            (uint64_t)(updateBytesToDownloadDiP3),                  /* DiP 3 Total Update size      */
                                                            (uint64_t)(updateBytesToDownloadNonPatchablePortion),   /* DiP 3 Update Non-Patched size   */
                                                            (uint64_t)(updateBytesToDownloadPatchablePortionActual),/* DiP 3 Update Patched size       */
                                                            (uint64_t)(updateBytesToDownloadDiP3Savings),           /* DiP 3 Update patched savings    */
                                                            (uint64_t)mPatchBuilderTransferStats.historicBps()      /* Diff Calculation Bitrate (bps) */
                                                            );
        }

        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Computed Download Job Size: " << _dataToSave;

        if(_fIsRepair)
        {
            ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Repairing " << _filesToSave << " files of " << _packageFileDirectory->GetNumberOfFiles() << " total files.";

            // TELEMETRY: We've calculated how many files we need to repair.
            GetTelemetryInterface()->Metric_REPAIRINSTALL_VERIFYFILESCOMPLETED(mProductId.toUtf8().data());
            GetTelemetryInterface()->Metric_REPAIRINSTALL_FILES_STATS(mProductId.toUtf8().data(), _filesToSave, _packageFileDirectory->GetNumberOfFiles());
        }

        TIME_END("ContentProtocolPackage::ComputeDownloadJobSize");

        // Save the CRC map, in case changes were made
        _crcMap->Save();

        // This is the tricky bit, we need to return to the proper place since we could have been called from one of two places
        if (protocolState() == kContentProtocolInitialized)
        {
            // We were in the middle of GetContentLength, so finish that
            FinishGetContentLength();
        }
        else
        {
            // We were in the middle of TransferStart, so finish that
            TransferStartJobReady();
        }
    }

    void ContentProtocolPackage::GetFileDetails(QString fullPath, bool followSymlinks, bool &fileExists, qint64 &fileSize, bool &isSymlink)
    {
        // Qt handles shortcut (.lnk) in an utterly stupid way on Windows, it treats them as symlinks
        // Furthermore none of the QFile objects can operate on the .lnk file itself, they all go to the file it points to, which is incorrect for our purposes
        // Therefore the only safe way to get the file size is using the native APIs
        if (EnvUtils::GetFileDetailsNative(fullPath, fileSize))
        {
            fileExists = true;
#ifdef ORIGIN_MAC
            // Need to check if file is a symbolic link
            QFileInfo finfo (fullPath);
            isSymlink = finfo.isSymLink();
            if( !finfo.isSymLink() || followSymlinks ) // If not get path size normally
            {
                return;
            }
            else // Make sure we get the correct size of the relative link
            {
                char linktarget[512];
                memset(linktarget,0,sizeof(linktarget));
                fileSize = readlink(qPrintable(fullPath), linktarget, sizeof(linktarget)-1);
            }
#endif
#ifdef ORIGIN_PC
            // Windows doesn't have real symlinks (or at least we don't really support them)
            isSymlink = false;
#endif
        }
        else
        {
            fileExists = false;
            fileSize = 0;
            isSymlink = false;
        }
    }

    void ContentProtocolPackage::ProcessPackageFileEntryMap()
    {
        // Blank out all the counters
        _dataToSave = 0;
        _dataSaved = 0;
        _dataAdded = 0;
        _dataDownloaded = 0;
        _dataToDownload = 0;
        _filesToSave = 0;
        _filesPatchRejected = 0;

        qint64 requiredPortionSize = 0;

        // Clear the metadata maps
        _packageFiles.clear();
        _packageFilesSorted.clear();
        _diffPatchFiles.clear();

        ORIGIN_ASSERT(_patchBuilder == NULL);

        // The list of files that will be differentially updated (if any)
        PatchBuilderFileMetadataQList diffFileList;

        // Turn off diff updating globally for ITO, and for operations that aren't updates
        if (_physicalMedia || !_fIsUpdate)
        {
            _fIsDiffUpdatingAvailable = false;
        }

        Engine::Debug::DownloadFileMetadata debugFile;
        QMap<QString, Engine::Debug::DownloadFileMetadata> debugFiles;

        // Walk over all files in the archive and create metadata entries and check the size on disk
        for (PackageFileEntryList::iterator it = _packageFileDirectory->begin();it != _packageFileDirectory->end(); it++)
        {
            PackageFileEntry* pPackageEntry = (*it);
            
            // Update our debug record if download debugging is enabled and this entry is a file.
            if (pPackageEntry->IsFile() && (mReportDebugInfo || Origin::Services::readSetting(Origin::Services::SETTING_DownloadDebugEnabled).toQVariant().toBool()))
            {
                debugFile.reset();
                debugFile.setFileName(pPackageEntry->GetFileName());
                debugFile.setTotalBytes(pPackageEntry->GetUncompressedSize());
                debugFile.setIncluded(pPackageEntry->IsIncluded());
                debugFile.setPackageFileCrc(pPackageEntry->GetFileCRC());
            }

            if (pPackageEntry->IsFile() && pPackageEntry->IsIncluded())
            {
                // Don't process 0-byte files
                if (pPackageEntry->GetUncompressedSize() == 0)
                    continue;

                // Flag to track whether this file will be handed to EAPatchBuilderOrigin (default false)
                bool fileIsBeingDiffUpdated = false;

                // Determine if the file is added (new)
                qint64 existingSize = 0;
                if (IsFileNew(pPackageEntry, existingSize))
                {
                    _dataAdded += pPackageEntry->GetUncompressedSize();
                }
                else
                {
                    // File already exists, so we might be able to differentially update it
                    if (ShouldDiffUpdateFile(pPackageEntry, existingSize))
                    {
                        fileIsBeingDiffUpdated = true;
                    }
                    else
                    {
                        // Disable differential updating for files that don't meet the criteria
                        pPackageEntry->SetIsDiffUpdatable(false);
                        fileIsBeingDiffUpdated = false;
                    }
                }

                bool shouldStage = false;
                QString sFilename(pPackageEntry->GetFileName());
                QString sDiskFilename(sFilename);
                if (ShouldStageFile(sFilename))
                {
                    shouldStage = true;
                    sDiskFilename = GetStagedFilename(sFilename);
                }

                qint64 fileSize = 0;
                QString fullPath = GetFileFullPath(sDiskFilename);

                // Track what we know about this file
                bool removeExistingFile = false;
                bool fileExists = false;
                bool fileComplete = false;
                bool isSymlink = false;

                // Determine whether the file exists on disk and if its complete
                GetFileDetails(fullPath, false, fileExists, fileSize, isSymlink);
                if (fileSize == pPackageEntry->GetUncompressedSize())
                        fileComplete = true;

                // See which priority groups this entry belongs to
                bool groupFound = false;
                bool fileInRequiredPortion = false;
                if (_fIsDynamicDownloadingAvailable)
                {
                    for (QMap<int, DownloadPriorityGroupMetadata>::iterator groupIter = _downloadReqPriorityGroups.begin(); groupIter != _downloadReqPriorityGroups.end(); ++groupIter)
                    {
                        // If this entry belongs to this group
                        if (groupIter.value().entries.contains(pPackageEntry))
                        {
                            groupFound = true;

                            // Initialize the per-file data
                            DownloadPriorityGroupEntryMetadata& fileMetadata = groupIter.value().entries[pPackageEntry];
                            fileMetadata.status = fileComplete ? kEntryStatusCompleted : kEntryStatusQueued;
                            fileMetadata.totalBytesRead = fileExists ? fileSize : 0;
                            fileMetadata.totalBytesRequested = pPackageEntry->GetUncompressedSize();

                            // Mark the group as having activity
                            DownloadPriorityGroupMetadata& groupMetadata = groupIter.value();
                            groupMetadata.hadUpdates = true;
                            groupMetadata.inactivityCount = 1;

                            if (groupMetadata.requirement == Engine::Content::kDynamicDownloadChunkRequirementRequired)
                            {
                                fileInRequiredPortion = true;
                            }
                        }
                    }
                }

                // Fallback is group 0
                if (!groupFound)
                {
                    // Initialize the per-file data
                    DownloadPriorityGroupEntryMetadata& fileMetadata = _downloadReqPriorityGroups[0].entries[pPackageEntry];
                    fileMetadata.status = fileComplete ? kEntryStatusCompleted : kEntryStatusQueued;
                    fileMetadata.totalBytesRead = fileExists ? fileSize : 0;
                    fileMetadata.totalBytesRequested = pPackageEntry->GetUncompressedSize();

                    // Mark the group as having activity
                    DownloadPriorityGroupMetadata& groupMetadata = _downloadReqPriorityGroups[0];
                    groupMetadata.groupId = 0; // Ensure this gets set
                    groupMetadata.hadUpdates = true;
                    groupMetadata.inactivityCount = 1;

                    // Group 0 is always required
                    fileInRequiredPortion = true;
                }

                // If we're staging, check first whether we tracked the staged file already
                QString diskCrcMapFilePath = GetCRCMapFilePath(sDiskFilename);
                if (shouldStage && _crcMap->Contains(diskCrcMapFilePath))
                {
                    const CRCMapData stagedCRCData = _crcMap->GetCRC(diskCrcMapFilePath);

                    // If the CRC changed since we last operated on this staged file, or if its complete and the size is mismatched, we just need to remove it
                    if (stagedCRCData.crc != pPackageEntry->GetFileCRC() || (fileComplete && stagedCRCData.size != pPackageEntry->GetUncompressedSize()))
                    {
                        // Whatever staged file we have needs to go away
                        removeExistingFile = true;

#ifdef DOWNLOADER_CRC_DEBUG
                        ORIGIN_LOG_EVENT << "Download Job Setup - Staged file existed, but CRC or size mismatch - " << sDiskFilename << " CRC - " << stagedCRCData.crc << " size - " << stagedCRCData.size;
#endif
                    }
                }

                // See whether we already tracked this file in the CRC map
                QString crcMapFilePath = GetCRCMapFilePath(sFilename);
                if (_crcMap->Contains(crcMapFilePath))
                {
                    // This is the CRC data for the UN-staged file (i.e. the final destination file)
                    const CRCMapData crcData = _crcMap->GetCRC(crcMapFilePath);

                    // Get the file details for the UN-staged file
                    bool finalFileExists = false;
                    bool finalFileIsSymlink = false;
                    qint64 finalFileSize = 0;

                    // Determine whether the file exists on disk and its size
                    GetFileDetails(GetFileFullPath(sFilename), false, finalFileExists, finalFileSize, finalFileIsSymlink);
                    
                    // If the CRC changed since we last tracked it, we always want to include it in the job
                    if (crcData.crc != pPackageEntry->GetFileCRC())
                    {
                        if(mReportDebugInfo || Origin::Services::readSetting(Origin::Services::SETTING_DownloadDebugEnabled).toQVariant().toBool())
                        {
                            debugFile.setDiskFileCrc(crcData.crc);
                        }

                        // If we're not staging the file, we should just remove whatever file may have already existed
                        if (!shouldStage)
                        {
                            removeExistingFile = true;
                        }

#ifdef DOWNLOADER_CRC_DEBUG
                        ORIGIN_LOG_EVENT << "Download Job Setup - CRC does not match package, including in job - " << sFilename << " stored CRC - " << crcData.crc;
#endif
                    }
                    // If the file existed in the final location, and we calculated that the CRC value matches the target, we don't need to bother with it again
                    else if (finalFileExists && crcData.size == pPackageEntry->GetUncompressedSize() && crcData.crc == pPackageEntry->GetFileCRC() && finalFileSize == crcData.size)
                    {
                        pPackageEntry->SetIsIncluded(false);

                        // If we're not doing an update, we'll count this as progress we've already made
                        // Additionally, if we are updating and this item was part of the same job, we likely already installed it, so count it again
                        if (!_fIsUpdate || (_fIsUpdate && !crcData.jobId.isEmpty() && crcData.jobId == getJobId()))
                        {
                            _dataToSave += pPackageEntry->GetUncompressedSize();
                            _dataSaved += pPackageEntry->GetUncompressedSize();
                            _dataDownloaded += pPackageEntry->GetCompressedSize();
                            _dataToDownload += pPackageEntry->GetCompressedSize();

                            if (fileInRequiredPortion)
                            {
                                requiredPortionSize += pPackageEntry->GetCompressedSize();
                            }
                        }

                        // We already have the correct file, mark it completed
                        UpdatePriorityGroupChunkMap(pPackageEntry, pPackageEntry->GetUncompressedSize(), pPackageEntry->GetUncompressedSize(), kEntryStatusInstalled);

                        // We don't need to process it
                        continue;
                    }

                    // If we're not staging and the file is 'complete', but couldn't be validated, we'll remove it
                    if (!shouldStage && fileComplete)
                    {
                        removeExistingFile = true;
                    }
                }

                // If the file exists, but we want to remove it.  This may happen if the CDN package version has a different CRC than we were
                // expecting.  This probably indicates that we're not looking at a totally new file.  The best option here is to actually remove
                // partially downloaded files.  We could *possibly* keep some of these for differential updating later, but this is safest for now.
                if (fileExists && removeExistingFile)
                {
                    // Zero out the file size we think we have for this file
                    fileSize = 0;

                    // Clear the flags
                    fileExists = false;
                    fileComplete = false;

                    // Remove the file that exists in the CRC map since we're deleting it
                    _crcMap->Remove(diskCrcMapFilePath);

                    ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Removing existing file due to CRC mismatch: " << fullPath;
                    Origin::Util::DiskUtil::DeleteFileWithRetry(fullPath);

                    if (fileIsBeingDiffUpdated)
                    {
                        // Tell EAPatchBuilderOrigin we need to clear out any partial data for this file
                        PatchBuilder::ClearTempFilesForEntry(_unpackPath, sFilename, sDiskFilename);
                    }

                    // Oops, we may have thought we had progress or had even already completed this file, but it's bogus and we need to restart it
                    // This will blank out that metadata for the priority group tracking
                    UpdatePriorityGroupChunkMap(pPackageEntry, 0, pPackageEntry->GetUncompressedSize(), kEntryStatusQueued);
                }

                // Don't attempt differential update partially or completely downloaded files, just handle them conventionally
                if (fileExists)
                {
                    fileIsBeingDiffUpdated = false;
                }
                
                //keep a sorted list of package files
                //we need this mainly for multi disc zips. we need the zips to be sorted by disc order in order for the
                //logic to work. the _packageFileDirectory is already properly sorted in PackageFile.cpp so we are just preserving this with
                //this list
                _packageFilesSorted.append(pPackageEntry);

                // Setup the metadata map
                PackageFileEntryMetadata& fileMetadata = _packageFiles[pPackageEntry];
                
                fileMetadata.diskFilename = sDiskFilename;
                fileMetadata.isStaged = shouldStage;
                
                if (fileIsBeingDiffUpdated)
                {
                    // Obtain a new diff file ID for tracking purposes
                    DiffFileId diffId = GetNextPatchBuilderId();

                    // Track it with the regular filemetadata
                    fileMetadata.isBeingDiffUpdated = true;
                    fileMetadata.diffId = diffId;
                    fileMetadata.bytesSaved = 0;
                    fileMetadata.bytesDownloaded = 0;

                    // Create the metadata structures for our own internal use
                    DiffPatchMetadata& diffPatchMetadata = _diffPatchFiles[diffId];
                    diffPatchMetadata.diffId = diffId;
                    diffPatchMetadata.packageFileEntry = pPackageEntry;

                    // Create the metadata structures for PatchBuilder's use
                    diffFileList.push_back(PatchBuilderFileMetadata());
                    PatchBuilderFileMetadata& patchBuilderMetadata = diffFileList.back();
                    patchBuilderMetadata.diffId = diffId;
                    patchBuilderMetadata.originalFilename = sFilename;
                    patchBuilderMetadata.diskFilename = sDiskFilename;
                    patchBuilderMetadata.fileCRC = pPackageEntry->GetFileCRC();
                    patchBuilderMetadata.fileCompressedSize = pPackageEntry->GetCompressedSize();
                    patchBuilderMetadata.fileUncompressedSize = pPackageEntry->GetUncompressedSize();
                }
                else
                {
                    // Grab the existing file size
                    fileMetadata.bytesSaved = fileSize;

                    if (fileComplete)
                    {
                        // The file is complete, so we know we already downloaded the full compressed size
                        fileMetadata.bytesDownloaded = pPackageEntry->GetCompressedSize();
                    }
                    else
                    {
                        // The file is present, but not complete
                        // See if the Unpack Service can give us an early 'peek' at the probable number of bytes we've processed (might require looking at the stream file, if it exists)
                        qint64 bytesDownloaded = 0;
                        if (fileExists && UnpackStreamFile::PeekProcessedBytes(fullPath, (UnpackType::code)pPackageEntry->GetCompressionMethod(), bytesDownloaded))
                        {
                            fileMetadata.bytesDownloaded = bytesDownloaded;
                        }
                    }

                    // Track the counters
                    _dataToSave += pPackageEntry->GetUncompressedSize();
                    _dataSaved += fileMetadata.bytesSaved;
                    _dataDownloaded += fileMetadata.bytesDownloaded;
                    _dataToDownload += pPackageEntry->GetCompressedSize();
                    

                    if (fileInRequiredPortion)
                    {
                        requiredPortionSize += pPackageEntry->GetCompressedSize();
                    }
                }

                ++_filesToSave;

                // Include this file in our table if download debugging is enabled.
                if(mReportDebugInfo || Origin::Services::readSetting(Origin::Services::SETTING_DownloadDebugEnabled).toQVariant().toBool())
                {
                    if(fileMetadata.bytesSaved < pPackageEntry->GetUncompressedSize())
                    {
                        debugFile.setBytesSaved(fileMetadata.bytesSaved);
                        debugFile.setIncluded(true);
                        debugFiles.insert(debugFile.fileName(), debugFile);
                    }
                }
            }
        }
        
        // Create our debug table if download debugging is enabled.
        if(mReportDebugInfo || Origin::Services::readSetting(Origin::Services::SETTING_DownloadDebugEnabled).toQVariant().toBool())
        {
            Engine::Debug::DownloadDebugDataCollector *collector = Engine::Debug::DownloadDebugDataManager::instance()->getDataCollector(mProductId);

            if(collector)
            {
                collector->updateDownloadFiles(debugFiles);
            }
        }

        // Make sure that our initial group states get sent out
        if (_fIsDynamicDownloadingAvailable)
        {
            // Assuming there is something to even do
            if (_dataToSave > 0)
            {
                float requiredPortionPct = ((float) requiredPortionSize / _dataToSave) * 100.0f;

                ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Dynamic download required portion size computed: " << requiredPortionSize << " bytes (" << requiredPortionPct << "% of job)";

                emit DynamicRequiredPortionSizeComputed(requiredPortionSize);
            }

            UpdatePriorityGroupAggregateStats();
        }

        // If there are files that we need to hand off to EAPatchBuilder
        if (_patchBuilder && diffFileList.count() > 0)
        {
            // Send telemetry indicating that we're preparing a patch
            GetTelemetryInterface()->Metric_DL_DIP3_PREPARE(mProductId.toLocal8Bit().constData(), (uint32_t)diffFileList.count());

            // Initialize the patch builder (when we get a signal back from here, we'll move on to finishing the calculation)
            _patchBuilder->Initialize(_syncPackageUrl, diffFileList);
        }
        else
        {
            // No need to bother with any of the other checks later
            _fIsDiffUpdatingAvailable = false;

            // Just call the finish function directly
            ComputeDownloadJobSizeEnd(false, 0);
        }
    }

    bool ContentProtocolPackage::ProcessChunk(IDownloadRequest* req)
    {
        TIME_BEGIN("ContentProtocolPackage::ProcessChunk")
        // Mark the start time for profiling
        quint32 nStartTime;
        nStartTime = GetTick();

        // Make sure we're still tracking this request
        RequestId downloadRequestId = req->getRequestId();
        if (!_downloadReqMetadata.contains(downloadRequestId))
        {
            return false;
        }

        // Make sure there is in fact still data available
        if (!req->chunkIsAvailable())
        {
            return false;
        }

        // Get the actual data from the request
        MemBuffer *chunkData = req->getDataBuffer();
        uchar* pBuffer = (uchar*) chunkData->GetBufferPtr();
        quint32 dataLen = chunkData->TotalSize();

        // Update the counter in the map before we copy the data down
        _downloadReqMetadata[downloadRequestId].totalBytesRead += dataLen;

        // Get the request metadata from the map
        const DownloadRequestMetadata requestMetadata = _downloadReqMetadata[downloadRequestId];

        // Did we get back more than we were supposed to get back?  (Note: This would never trigger until the request is 'done', but check it constantly anyhow)
        if (requestMetadata.totalBytesRead > requestMetadata.totalBytesRequested)
        {
            // Oops!  Something went wrong!
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Received too many bytes for Download Request.  ReqID = " << downloadRequestId << " Filename: " << requestMetadata.packageFileEntry->GetFileName()
                                              << " Recvd = " << requestMetadata.totalBytesRead << " Expected = " << requestMetadata.totalBytesRequested;

            QString telemStr = QString("File=%1,Recvd=%2,Request=%3").arg(requestMetadata.packageFileEntry->GetFileName()).arg(requestMetadata.totalBytesRead).arg(requestMetadata.totalBytesRequested);
            GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mProductId.toLocal8Bit().constData(), "ProcessChunkSizeMismatch", qPrintable(telemStr), __FILE__, __LINE__);

            // Close the unpack request if one exists
            if (_unpackFiles.contains(requestMetadata.unpackStreamId))
            {
                _unpackStream->closeUnpackFile(requestMetadata.unpackStreamId);
                _unpackFiles.remove(requestMetadata.unpackStreamId);
            }

            // Cancel the existing download request
            _downloadReqMetadata.remove(downloadRequestId);
            _downloadSession->closeRequest(downloadRequestId);

            // Too much data inevitably means corruption, to be safe we should clobber and re-request the file
            ContentDownloadError::type rerequestErrCode = TransferReIssueRequest(requestMetadata.packageFileEntry, static_cast<ContentDownloadError::type>(ProtocolError::ExcessDataReceived), true);
		    if (ContentDownloadError::OK != rerequestErrCode)
            {
                ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Failed to re-request file with error condition, halting download session";

                // Send an actual redownload error
                CREATE_DOWNLOAD_ERROR_INFO(redownloadErrorInfo);
                redownloadErrorInfo.mErrorType = ProtocolError::ExcessDataReceived;
                redownloadErrorInfo.mErrorCode = rerequestErrCode;
                
                emit (IContentProtocol_Error(redownloadErrorInfo));

                // We weren't able to reissue the request, we need to halt the entire job
                Pause();
            }
            else
            {
                // Send progress because the byte totals may have changed
                if(checkToSendThrottledUpdate())
                {
                    SendProgressUpdate();
                }
            }

            TIME_END("ContentProtocolPackage::ProcessChunk")
            return false;
        }

        // If this is the first read for this request, we need to consume the header
        if (!requestMetadata.headerConsumed)
        {
            // Figure out the new buffer
            int headerSize = requestMetadata.packageFileEntry->GetOffsetToFileData() - requestMetadata.packageFileEntry->GetOffsetToHeader();
            pBuffer = pBuffer + headerSize;
            dataLen -= headerSize;

            _downloadReqMetadata[downloadRequestId].headerConsumed = true;
        }

        // Log the data with the download rate sampler
        mTransferStatsDownload.addSample(dataLen);

        // Queue the actual chunk with the unpack service
        UnpackStreamFileId unpackId = requestMetadata.unpackStreamId;

        _unpackStream->queueChunk(unpackId, pBuffer, dataLen, req->getChunkDiagnosticInfo());		

        //int32_t nStartToFinishTime = ::GetTickCount()-pWorker->GetStartTime();
        // LogDebug(L">>>Start->ProcessStart:%d  ProcessChunk:%d  Start->Finish:%d\n", nStartTime-pWorker->GetStartTime(), ::GetTickCount()-nStartTime, nStartToFinishTime );

        if (_packageFiles.contains(requestMetadata.packageFileEntry))
        {
            // If data is successfully received, lets reset the retry counter
            _packageFiles[requestMetadata.packageFileEntry].retryCount = 0;
        }

        TIME_END("ContentProtocolPackage::ProcessChunk")
        return true;
    }

    void ContentProtocolPackage::SetExcludes(const QStringList& excludePatterns)
    {
        //QDEBUGEX << "excludeDirectory: " << directoryName;
        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Setting Excludes";

        QSet<QString> singleExcludeFiles;

        for (QStringList::const_iterator excludeIt = excludePatterns.begin(); excludeIt != excludePatterns.end(); excludeIt++)
        {
            QString pattern = *excludeIt;
            pattern.replace('\\', '/');  // forward slashes required to match the formatting of the manifest
            if (pattern.left(1) == "/")  // manifest sometimes contains a leading slash but the package listings don't have that
                pattern.remove(0,1);

            // Is this a wildcard pattern?
            if (pattern.contains(QChar('*')) || pattern.contains(QChar('?')) || pattern.contains(QChar('[')))
            {
                for (PackageFileEntryList::const_iterator it = _packageFileDirectory->begin();  it != _packageFileDirectory->end(); it++)
                {
                    QString filename = (*it)->GetFileName();
                    filename.replace('\\', '/');  // forward slashes required to match the formatting of the manifest

                    if (EA::IO::FnMatch(pattern.toStdWString().c_str(), filename.toStdWString().c_str(), EA::IO::kFNMCaseFold|EA::IO::kFNMPathname|EA::IO::kFNMUnixPath))
                    {
                        ORIGIN_LOG_DEBUG << "Pattern: \"" << pattern << "\" matches filename: \"" << filename << "\" - Excluding.";
                        (*it)->SetIsIncluded(false);
                    }
                }
            }
            else
            {
                // not a wildcard, just find the entry and set it
                if (pattern.right(1) == "/")     // this is a folder so add a * for wildcard matching 
                {
                    ORIGIN_LOG_DEBUG << "Excluding Whole Directory: \"" << pattern << "\"";
                    int32_t charsToMatch = pattern.length();

                    for (PackageFileEntryList::const_iterator it = _packageFileDirectory->begin();  it != _packageFileDirectory->end(); it++)
                    {
                        QString filename = (*it)->GetFileName();
                        filename.replace('\\', '/');  // forward slashes required to match the formatting of the manifest

                        if (pattern.compare(filename.left(charsToMatch), Qt::CaseInsensitive) == 0)
                        {
                            (*it)->SetIsIncluded(false);
                        }
                    }
                }
                else
                {
                    // no wildcards and not a directory, just exclude the single file
                    singleExcludeFiles.insert(pattern.toLower());
                }
            }
        }

        if (singleExcludeFiles.size() > 0)
        {
            for (PackageFileEntryList::iterator it = _packageFileDirectory->begin(); it != _packageFileDirectory->end(); it++)
            {
                PackageFileEntry *pEntry = *it;

                // Skip already excluded files
                if (!pEntry->IsIncluded())
                    continue;

                QString filename = pEntry->GetFileName().toLower();

                // Try to find it in the excludes list
                QSet<QString>::Iterator entry = singleExcludeFiles.find(filename);
                if (entry != singleExcludeFiles.end())
                {
                    pEntry->SetIsIncluded(false);
                    singleExcludeFiles.erase(entry);
                }
            }

            for (QSet<QString>::Iterator it = singleExcludeFiles.begin(); it != singleExcludeFiles.end(); it++)
            {
                ORIGIN_LOG_WARNING << PKGLOG_PREFIX << "Unable to exclude: " << *it;
            }
        }
    }
/*    void ContentProtocolPackage::excludeFiles(QList<QString> filenames)
    {
        int count = 0;
        QMap<QString, int> excludeFiles;
        
		for(QList<QString>::iterator it = filenames.begin(); it != filenames.end(); it++)
        {
            // Clean up the filename
            QString sFilenameTemp = *it;

            // Fix the slashes
            sFilenameTemp.replace(L'\\', L'/');  // forward slashes required to match the formatting of the manifest

            // Capture the fact that manifest filenames sometimes prepend the path separator
            if (sFilenameTemp.startsWith("/"))
            {
                sFilenameTemp.remove(0,1);
            }

            excludeFiles.insert(sFilenameTemp.toLower(), ++count);
        }

        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Excluding " << count << " files.";

        for (PackageFileEntryList::iterator it = _packageFileDirectory->begin(); it != _packageFileDirectory->end(); it++)
        {
            PackageFileEntry *pEntry = *it;

            QString filename = pEntry->GetFileName().toLower();

            // Try to find it in the excludes list
            QMap<QString, int>::Iterator entry = excludeFiles.find(filename);
            if (entry != excludeFiles.end())
            {
                pEntry->SetIsIncluded(false);
                excludeFiles.erase(entry);
            }
        }

        for (QMap<QString, int>::Iterator it = excludeFiles.begin(); it != excludeFiles.end(); it++)
        {
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Unable to exclude: " << it.key();
        }
    }*/

    void ContentProtocolPackage::SetDeletes(const QStringList& deletePatterns)
    {
        _deletePatterns = deletePatterns;
    }

    void ContentProtocolPackage::SetDiffExcludes(const QStringList& diffExcludePatterns)
    {
        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Setting Diff Excludes";

        QSet<QString> singleExcludeFiles;

        for (QStringList::const_iterator excludeIt = diffExcludePatterns.begin(); excludeIt != diffExcludePatterns.end(); excludeIt++)
        {
            QString pattern = *excludeIt;
            pattern.replace('\\', '/');  // forward slashes required to match the formatting of the manifest
            if (pattern.left(1) == "/")  // manifest sometimes contains a leading slash but the package listings don't have that
                pattern.remove(0,1);

            // Is this a wildcard pattern?
            if (pattern.contains(QChar('*')) || pattern.contains(QChar('?')) || pattern.contains(QChar('[')))
            {
                for (PackageFileEntryList::const_iterator it = _packageFileDirectory->begin();  it != _packageFileDirectory->end(); it++)
                {
                    QString filename = (*it)->GetFileName();
                    filename.replace('\\', '/');  // forward slashes required to match the formatting of the manifest

                    if (EA::IO::FnMatch(pattern.toStdWString().c_str(), filename.toStdWString().c_str(), EA::IO::kFNMCaseFold|EA::IO::kFNMPathname|EA::IO::kFNMUnixPath))
                    {
                        ORIGIN_LOG_DEBUG << "Pattern: \"" << pattern << "\" matches filename: \"" << filename << "\" - Excluding.";

                        (*it)->SetIsDiffUpdatable(false);
                    }
                }
            }
            else
            {
                if (pattern.right(1) == "/")     // this is a folder so add a * for wildcard matching WITHOUT directory separator terminators
                {
                    ORIGIN_LOG_DEBUG << "Excluding Whole Directory: \"" << pattern << "\"";
                    int32_t charsToMatch = pattern.length();
                    for (PackageFileEntryList::const_iterator it = _packageFileDirectory->begin();  it != _packageFileDirectory->end(); it++)
                    {
                        QString filename = (*it)->GetFileName();
                        filename.replace('\\', '/');  // forward slashes required to match the formatting of the manifest

                        if (pattern.compare(filename.left(charsToMatch), Qt::CaseInsensitive) == 0)
                        {
                            (*it)->SetIsDiffUpdatable(false);
                        }
                    }
                }
                else
                {

                    //                ORIGIN_LOG_DEBUG << "Excluding File: \"" << pattern << "\"";
                    // not a wildcard and not a directory, just find the entry and set it

                    singleExcludeFiles.insert(pattern.toLower());
                }
            }
        }

        if (singleExcludeFiles.size() > 0)
        {
            for (PackageFileEntryList::iterator it = _packageFileDirectory->begin(); it != _packageFileDirectory->end(); it++)
            {
                PackageFileEntry *pEntry = *it;

                // Skip already diff update excluded files
                if (!pEntry->IsDiffUpdatable())
                    continue;

                QString filename = pEntry->GetFileName().toLower();

                // Try to find it in the excludes list
                QSet<QString>::Iterator entry = singleExcludeFiles.find(filename);
                if (entry != singleExcludeFiles.end())
                {
                    pEntry->SetIsDiffUpdatable(false);
                    singleExcludeFiles.erase(entry);
                }
            }

            for (QSet<QString>::Iterator it = singleExcludeFiles.begin(); it != singleExcludeFiles.end(); it++)
            {
                ORIGIN_LOG_WARNING << PKGLOG_PREFIX << "Unable to diff-update exclude: " << *it;
            }
        }
    }

    void ContentProtocolPackage::SetAutoExcludesFromIncludeList(const QStringList& allIncludePatterns, const QStringList& curIncludes)
    {
        QSet<QString> curMatchingFiles;

        // For each pattern
        for (QStringList::const_iterator excludeIt = curIncludes.begin(); excludeIt != curIncludes.end(); excludeIt++)
        {
            QString pattern = *excludeIt;
            pattern.replace('\\', '/');  // forward slashes required to match the formatting of the manifest
            if (pattern.left(1) == "/")  // manifest sometimes contains a leading slash but the package listings don't have that
                pattern.remove(0,1);

            bool patternIsWildcard = (pattern.contains(QChar('*')) || pattern.contains(QChar('?')) || pattern.contains(QChar('[')));
            bool patternIsFolder = (pattern.right(1) == "/");

            // For each file in the package
            for (PackageFileEntryList::const_iterator it = _packageFileDirectory->begin();  it != _packageFileDirectory->end(); it++)
            {
                QString filename = (*it)->GetFileName();
                filename.replace('\\', '/');  // forward slashes required to match the formatting of the manifest

                // Is this a wildcard pattern?
                if (patternIsWildcard)
                {
                    if (EA::IO::FnMatch(pattern.toStdWString().c_str(), filename.toStdWString().c_str(), EA::IO::kFNMCaseFold|EA::IO::kFNMPathname|EA::IO::kFNMUnixPath))
                    {
                        if (!curMatchingFiles.contains(filename.toLower()))
                            curMatchingFiles.insert(filename.toLower());
                    }
                }
                else
                {
                    if (patternIsFolder)     // this is a folder so add a * for wildcard matching WITHOUT directory separator terminators
                    {
                        // Grab the whole directory
                        int32_t charsToMatch = pattern.length();
                    
                        if (pattern.compare(filename.left(charsToMatch), Qt::CaseInsensitive) == 0)
                        {
                            if (!curMatchingFiles.contains(filename.toLower()))
                                curMatchingFiles.insert(filename.toLower());
                        }
                    }
                    else if (pattern.compare(filename, Qt::CaseInsensitive) == 0)
                    {
                        // not a wildcard and not a directory, just add the entry
                        if (!curMatchingFiles.contains(filename.toLower()))
                            curMatchingFiles.insert(filename.toLower());
                    }
                }
            }
        }

        // Now iterate the superset of all language include patterns.  If any files match which don't appear in the previously computed list, we exclude them
        // For each pattern
        for (QStringList::const_iterator excludeIt = allIncludePatterns.begin(); excludeIt != allIncludePatterns.end(); excludeIt++)
        {
            QString pattern = *excludeIt;
            pattern.replace('\\', '/');  // forward slashes required to match the formatting of the manifest
            if (pattern.left(1) == "/")  // manifest sometimes contains a leading slash but the package listings don't have that
                pattern.remove(0,1);

            bool patternIsWildcard = (pattern.contains(QChar('*')) || pattern.contains(QChar('?')) || pattern.contains(QChar('[')));
            bool patternIsFolder = (pattern.right(1) == "/");

            // For each file in the package
            for (PackageFileEntryList::const_iterator it = _packageFileDirectory->begin();  it != _packageFileDirectory->end(); it++)
            {
                QString filename = (*it)->GetFileName();
                filename.replace('\\', '/');  // forward slashes required to match the formatting of the manifest

                // Is this a wildcard pattern?
                if (patternIsWildcard)
                {
                    if (EA::IO::FnMatch(pattern.toStdWString().c_str(), filename.toStdWString().c_str(), EA::IO::kFNMCaseFold|EA::IO::kFNMPathname|EA::IO::kFNMUnixPath))
                    {
                        if (!curMatchingFiles.contains(filename.toLower()))
                        {
                            (*it)->SetIsIncluded(false);
                        }
                    }
                }
                else
                {
                    if (patternIsFolder)     // this is a folder so add a * for wildcard matching WITHOUT directory separator terminators
                    {
                        // Check the whole directory
                        int32_t charsToMatch = pattern.length();
                    
                        if (pattern.compare(filename.left(charsToMatch), Qt::CaseInsensitive) == 0)
                        {
                            if (!curMatchingFiles.contains(filename.toLower()))
                            {
                                (*it)->SetIsIncluded(false);
                            }
                        }
                    }
                    else if (pattern.compare(filename, Qt::CaseInsensitive) == 0)
                    {
                        // not a wildcard and not a directory, just exclude the entry
                        if (!curMatchingFiles.contains(filename.toLower()))
                        {
                            (*it)->SetIsIncluded(false);
                        }
                    }
                }
            }
        }
    }

    void ContentProtocolPackage::CreateRuntimeDynamicChunk(int priorityGroupId, QString internalName, QStringList includePatterns)
    {
        // Everything past this executes on the protocol thread
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(int, priorityGroupId), Q_ARG(QString, internalName), Q_ARG(QStringList, includePatterns));

        // We are safe to call this only on the Protocol thread once the protocol is running
        SetPriorityGroup(priorityGroupId, Origin::Engine::Content::kDynamicDownloadChunkRequirementDynamic, internalName, includePatterns);

        // Holds the request Ids of the files we're interested in, if any
        QVector<RequestId> requestIds;

        // Grab our newly created group
        DownloadPriorityGroupMetadata& groupMetadata = _downloadReqPriorityGroups[priorityGroupId];

        // Go over each individual entry
        for (QHash<PackageFileEntry*, DownloadPriorityGroupEntryMetadata>::iterator entryIter = groupMetadata.entries.begin(); entryIter != groupMetadata.entries.end(); ++entryIter)
        {
            PackageFileEntry *pEntry = entryIter.key();
            DownloadPriorityGroupEntryMetadata& fileMetadata = entryIter.value();

            // If we have no record of this file, skip it
            if (!_packageFiles.contains(pEntry))
            {
                // Files that are already de-staged (installed), or perhaps unchanged, don't appear in the job list (_packageFiles)
                // These files can be considered to be already installed if the CRC map says they are valid and match the package CRC
                QString crcMapFilePath = GetCRCMapFilePath(pEntry->GetFileName());
                if (_crcMap->Contains(crcMapFilePath))
                {
                    const CRCMapData crcData = _crcMap->GetCRC(crcMapFilePath);
                    if (crcData.size == pEntry->GetUncompressedSize() && crcData.crc == pEntry->GetFileCRC())
                    {
                        // Completed files just copy the size reported by the package
                        fileMetadata.totalBytesRead = pEntry->GetUncompressedSize();
                        fileMetadata.totalBytesRequested = pEntry->GetUncompressedSize();
                        fileMetadata.status = kEntryStatusInstalled;

                        continue;
                    }
                }

                // TODO: When we support on-demand chunks, we may need to handle this situation
                ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Dynamic Group Creation - Unable to find file in download job: " << pEntry->GetFileName();
                continue;
            }

            const PackageFileEntryMetadata packageFileMetadata = _packageFiles[pEntry];
            
            bool complete = (packageFileMetadata.bytesSaved == pEntry->GetUncompressedSize()); // If the file size matches the target size, it's complete

            fileMetadata.totalBytesRead = packageFileMetadata.bytesSaved;
            fileMetadata.totalBytesRequested = pEntry->GetUncompressedSize();
            fileMetadata.status = (complete) ? kEntryStatusCompleted : kEntryStatusQueued; // If it isn't complete, it starts as queued

            if (!complete)
            {
                // If there is an open download request
                if (packageFileMetadata.downloadId != -1 && _downloadReqMetadata.contains(packageFileMetadata.downloadId))
                {
                    // Save the request, we will submit it
                    requestIds.push_back(packageFileMetadata.downloadId);
                }
                else
                {
                    // TODO: There is no open download request and we may need to make one
                    // Don't worry about this now because we don't currently support on-demand chunks
                }
            }
        }

        ORIGIN_LOG_EVENT << PKGLOG_PREFIX << "Priority Group #" << priorityGroupId << " - Total entries: " << groupMetadata.entries.size() << " - Submitting: " << requestIds.size();

        groupMetadata.status = Engine::Content::kDynamicDownloadChunkState_Queued;
        groupMetadata.hadUpdates = true;
        groupMetadata.inactivityCount = 1;

        if (!requestIds.empty())
        {
            // Submit the requests again (they are likely already submitted) so that these request Ids get associated with our newly created priority group
            _downloadSession->submitRequests(requestIds, priorityGroupId);

            // Since we just created this group, we want to move it to the front right away
            _downloadSession->priorityMoveToTop(priorityGroupId);
        }

        // Refresh the aggregate stats so we get accurate accounting of our new group's status
        UpdatePriorityGroupAggregateStats();
    }

    void ContentProtocolPackage::SetPriorityGroup(int priorityGroupId, Engine::Content::DynamicDownloadChunkRequirementT requirement, const QString& internalName, const QStringList& includePatterns)
    {
        // We consider it a non-fatal error if Group 0 is not set as required (which it is, by definition)
        // This will get overridden later to be required
        if (priorityGroupId == 0 && requirement != Engine::Content::kDynamicDownloadChunkRequirementRequired)
        {
            ORIGIN_LOG_ERROR << PKGLOG_PREFIX << "Group 0 is not set to 'Required'.";
        }

        // Fetch/create the priority group metadata
        DownloadPriorityGroupMetadata& priorityGroup = _downloadReqPriorityGroups[priorityGroupId];
        priorityGroup.requirement = requirement;
        priorityGroup.groupId = priorityGroupId;
        priorityGroup.internalName = internalName;

        QSet<QString> singleFiles;
        QSet<QString> allMatchingFiles;

        for (QStringList::const_iterator includeIt = includePatterns.begin(); includeIt != includePatterns.end(); includeIt++)
        {
            QString pattern = *includeIt;
            pattern.replace('\\', '/');  // forward slashes required to match the formatting of the manifest
            if (pattern.left(1) == "/")  // manifest sometimes contains a leading slash but the package listings don't have that
                pattern.remove(0,1);

            // Is this a wildcard pattern?
            if (pattern.contains(QChar('*')) || pattern.contains(QChar('?')) || pattern.contains(QChar('[')))
            {
                for (PackageFileEntryList::const_iterator it = _packageFileDirectory->begin();  it != _packageFileDirectory->end(); it++)
                {
                    // Don't match non-included files or directories
                    if (!(*it)->IsIncluded() || !(*it)->IsFile())
                        continue;

                    QString filename = (*it)->GetFileName();
                    filename.replace('\\', '/');  // forward slashes required to match the formatting of the manifest

                    if (EA::IO::FnMatch(pattern.toStdWString().c_str(), filename.toStdWString().c_str(), EA::IO::kFNMCaseFold|EA::IO::kFNMPathname|EA::IO::kFNMUnixPath))
                    {
                        ORIGIN_LOG_DEBUG << "Pattern: \"" << pattern << "\" matches filename: \"" << filename << "\" - Including in Priority Group: " << priorityGroupId;

                        // Save it to the list
                        priorityGroup.entries.insert(*it, DownloadPriorityGroupEntryMetadata());
                        allMatchingFiles.insert(filename);
                    }
                }
            }
            else
            {
                if (pattern.right(1) == "/")     // this is a folder so add a * for wildcard matching WITHOUT directory separator terminators
                {
                    ORIGIN_LOG_DEBUG << "Including Whole Directory: \"" << pattern << "\" in Priority Group: " << priorityGroupId;
                    int32_t charsToMatch = pattern.length();
                    for (PackageFileEntryList::const_iterator it = _packageFileDirectory->begin();  it != _packageFileDirectory->end(); it++)
                    {
                        // Don't match non-included files or directories
                        if (!(*it)->IsIncluded()|| !(*it)->IsFile())
                            continue;

                        QString filename = (*it)->GetFileName();
                        filename.replace('\\', '/');  // forward slashes required to match the formatting of the manifest

                        if (pattern.compare(filename.left(charsToMatch), Qt::CaseInsensitive) == 0)
                        {
                            // Save it to the list
                            priorityGroup.entries.insert(*it, DownloadPriorityGroupEntryMetadata());
                            allMatchingFiles.insert(filename);
                        }
                    }
                }
                else
                {
                    singleFiles.insert(pattern.toLower());
                }
            }
        }

        if (singleFiles.size() > 0)
        {
            for (PackageFileEntryList::iterator it = _packageFileDirectory->begin(); it != _packageFileDirectory->end(); it++)
            {
                PackageFileEntry *pEntry = *it;

                QString filename = pEntry->GetFileName().toLower();

                bool addFile = true;

                // Don't match non-included files or directories
                if (!pEntry->IsIncluded() || !(*it)->IsFile())
                {
                    addFile = false;
                }

                if (priorityGroup.entries.contains(pEntry))
                {
                    addFile = false;
                }

                // Try to find it in the excludes list
                QSet<QString>::Iterator entry = singleFiles.find(filename);
                if (entry != singleFiles.end())
                {
                    // If we still want to add it, do so now
                    if (addFile)
                    {
                        ORIGIN_LOG_DEBUG << "Filename: \"" << filename << "\" exact match. - Including in Priority Group: " << priorityGroupId;

                        // Save it to the list
                        priorityGroup.entries.insert(*it, DownloadPriorityGroupEntryMetadata());
                        allMatchingFiles.insert(filename);
                    }

                    // Always erase it from this list so we don't report a warning
                    singleFiles.erase(entry);
                }
            }

            for (QSet<QString>::Iterator it = singleFiles.begin(); it != singleFiles.end(); it++)
            {
                ORIGIN_LOG_WARNING << PKGLOG_PREFIX << "No matching package entry for: " << *it << " in Priority Group: " << priorityGroupId;
            }
        }

        // If this is an empty group, we must 'prime' the states because it won't get hit otherwise
        if (allMatchingFiles.count() == 0)
        {
            priorityGroup.status = Engine::Content::kDynamicDownloadChunkState_Queued;
            priorityGroup.hadUpdates = true;
            priorityGroup.inactivityCount = 1;
        }

        emit DynamicChunkDefined(priorityGroupId, requirement, internalName, allMatchingFiles);
    }

    void ContentProtocolPackage::SetPriorityGroupOrder(QVector<int> priorityGroupIds)
    {
        // Make sure we have a valid download session, and then submit the new queue order
        if (_downloadSession && _downloadSession->isRunning())
        {
            _downloadSession->prioritySetQueueOrder(priorityGroupIds);
        }
    }

    QVector<int> ContentProtocolPackage::GetPriorityGroupOrder()
    {
        if (protocolState() != kContentProtocolRunning)
        {
            ORIGIN_LOG_WARNING << PKGLOG_PREFIX << "Can't send priority group queue update, protocol not running.";

            // No valid data available
            return QVector<int>();
        }

        // Make sure we have a valid download session, and then submit the new queue order
        if (_downloadSession && _downloadSession->isRunning())
        {
            return _downloadSession->priorityGetQueueOrder();
        }

        // No valid data available
        return QVector<int>();
    }

    ContentProtocolPackage::DelayedCancel::DelayedCancel(ContentProtocolPackage* parent, QWaitCondition& waitCondition) :
        mParent(parent),
        mWaitCondition(waitCondition)
    {
        
    }

    void ContentProtocolPackage::DelayedCancel::run()
    {
        mMutex.lock();
        mWaitCondition.wait(&mMutex);

        ORIGIN_LOG_EVENT << "Asynchronous network request complete, resume cancel.";
        mParent->Cancel();
        mMutex.unlock();
    }

} // namespace Downloader
} // namespace Origin
