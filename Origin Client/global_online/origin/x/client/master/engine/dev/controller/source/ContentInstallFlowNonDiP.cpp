/////////////////////////////////////////////////////////////////////////////
// ContentInstallFlowNonDiP.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "services/network/GlobalConnectionStateMonitor.h"
#include "services/debug/DebugService.h"
#include "services/downloader/Parameterizer.h"
#include "services/common/DiskUtil.h"
#include "services/log/LogService.h"
#include "services/publishing/DownloadUrlServiceClient.h"
#include "services/rest/OriginServiceMaps.h"
#include "services/settings/SettingsManager.h"
#include "services/escalation/IEscalationService.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/qt/QtUtil.h"

#include "engine/content/Entitlement.h"
#include "engine/content/LocalContent.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/content/ContentOperationQueueController.h"
#include "engine/downloader/ContentInstallFlowNonDiP.h"
#include "engine/downloader/ContentServices.h"
#include "engine/login/LoginController.h"

#include "ContentProtocols.h"
#include "ContentProtocolSingleFile.h"
#include "ContentProtocolPackage.h"
#include "services/platform/EnvUtils.h"
#include "ExecuteInstallerRequest.h"
#include "InstallSession.h"
#include "login/User.h"
#include "TelemetryAPIDLL.h"


namespace Origin
{
namespace Downloader
{
    const int kDelayBeforeJitRetry = 4000;

    ContentInstallFlowNonDiP::ContentInstallFlowNonDiP(Origin::Engine::Content::EntitlementRef content, Origin::Engine::UserRef user) :
        IContentInstallFlow(content, user),
        mDownloadProtocol(NULL),
        mInstaller(new InstallSession(content)),
        mUnmonitoredInstallInProgress(false),
        mResumeOnPaused(false),
        mProcessingSuspend (false)
    {
        initializeFlow();
        startFlow();

        ORIGIN_VERIFY_CONNECT(&mounter, SIGNAL(mountComplete(int, QString)), this, SLOT(OnMountComplete(int, QString)));
        ORIGIN_VERIFY_CONNECT(&mounter, SIGNAL(unmountComplete(int)), this, SLOT(OnUnmountComplete(int)));
        
        ORIGIN_VERIFY_CONNECT_EX(this, SIGNAL(stopped()), this, SLOT(OnStopped()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT(mInstaller, SIGNAL(installFailed(Origin::Downloader::DownloadErrorInfo)), this, SLOT(OnInstallerError(Origin::Downloader::DownloadErrorInfo)));
        ORIGIN_VERIFY_CONNECT(mInstaller, SIGNAL(installFinished()), this, SLOT(OnInstallerFinished()));
        ORIGIN_VERIFY_CONNECT(Origin::Services::Network::GlobalConnectionStateMonitor::instance(), SIGNAL(connectionStateChanged(Origin::Services::Connection::ConnectionStateField)), this,  SLOT(OnGlobalConnectionStateChanged(Origin::Services::Connection::ConnectionStateField)));

        ContentProtocols::RegisterWithInstallFlowThread(this);
    }

    ContentInstallFlowNonDiP::~ContentInstallFlowNonDiP()
    {
        delete mInstaller;
        if (mDownloadProtocol)
            mDownloadProtocol->deleteLater();
    }

    void ContentInstallFlowNonDiP::deleteAsync()
    {
        if (!mDownloadProtocol)
        {
            onProtocolDeleted();
            return;
        }

        Origin::Services::ThreadObjectReaper* reaper = new Origin::Services::ThreadObjectReaper(mDownloadProtocol);
        ORIGIN_VERIFY_CONNECT(reaper, SIGNAL(objectsDeleted()), this, SLOT(onProtocolDeleted()));
        Downloader::ContentProtocols::RegisterWithProtocolThread(reaper);
        reaper->start();

        mDownloadProtocol = NULL;
    }

    void ContentInstallFlowNonDiP::onProtocolDeleted()
    {
        deleteLater();
    }

    void ContentInstallFlowNonDiP::initializePublicStateMachine()
    {
        QState* readyToStart = getState(ContentInstallFlowState::kReadyToStart);
        ORIGIN_VERIFY_CONNECT(readyToStart, SIGNAL(entered()), this, SLOT(OnReadyToStart()));

        QState* starting = getState(ContentInstallFlowState::kInitializing);
        ORIGIN_VERIFY_CONNECT(starting, SIGNAL(entered()), this, SLOT(SetupDownloadProtocol()));

        QState* installInfo = getState(ContentInstallFlowState::kPendingInstallInfo);
        ORIGIN_VERIFY_CONNECT(installInfo, SIGNAL(entered()), this, SLOT(OnInstallInfo()));

		QState* enqueued = getState(ContentInstallFlowState::kEnqueued);
		ORIGIN_VERIFY_CONNECT(enqueued, SIGNAL(entered()), this, SLOT(onEnqueued()));

        QState* preTransfer = getState(ContentInstallFlowState::kPreTransfer);
        ORIGIN_VERIFY_CONNECT(preTransfer, SIGNAL(entered()), this, SLOT(OnPreTransfer()));

        QState* transferring = getState(ContentInstallFlowState::kTransferring);
        ORIGIN_VERIFY_CONNECT(transferring, SIGNAL(entered()), this, SLOT(OnTransferring()));

        QState* postTransfer = getState(ContentInstallFlowState::kPostTransfer);
        ORIGIN_VERIFY_CONNECT(postTransfer, SIGNAL(entered()), this, SLOT(OnPostTransfer()));

        QState* readyToInstall = getState(ContentInstallFlowState::kReadyToInstall);
		ORIGIN_VERIFY_CONNECT(readyToInstall, SIGNAL(entered()), this, SLOT(OnReadyToInstall()));

        QState* preInstall = getState(ContentInstallFlowState::kPreInstall);
        ORIGIN_VERIFY_CONNECT(preInstall, SIGNAL(entered()), this, SLOT(OnPreInstall()));

        QState* install = getState(ContentInstallFlowState::kInstalling);
        ORIGIN_VERIFY_CONNECT(install, SIGNAL(entered()), this, SLOT(OnInstall()));

        QState* postInstall = getState(ContentInstallFlowState::kPostInstall);
        ORIGIN_VERIFY_CONNECT(postInstall, SIGNAL(entered()), this, SLOT(OnPostInstall()));

        QState* complete = getState(ContentInstallFlowState::kCompleted);
        ORIGIN_VERIFY_CONNECT(complete, SIGNAL(entered()), this, SLOT(OnComplete()));

        QState* paused = getState(ContentInstallFlowState::kPaused);
        ORIGIN_VERIFY_CONNECT(paused, SIGNAL(entered()), this, SLOT(OnPaused()));
        
        QState* mounting = getState(ContentInstallFlowState::kMounting);
        ORIGIN_VERIFY_CONNECT(mounting, SIGNAL(entered()), this, SLOT(OnMounting()));
        
        QState* unmounting = getState(ContentInstallFlowState::kUnmounting);
        ORIGIN_VERIFY_CONNECT(unmounting, SIGNAL(entered()), this, SLOT(OnUnmounting()));

        QState* pausing = getState(ContentInstallFlowState::kPausing);
        QState* resuming = getState(ContentInstallFlowState::kResuming);
        QState* canceling = getState(ContentInstallFlowState::kCanceling);
        QState* error = getState(ContentInstallFlowState::kError);
        ORIGIN_VERIFY_CONNECT(error, SIGNAL(entered()), this, SLOT(OnFlowError()));

        readyToStart->addTransition(this, SIGNAL(BeginInitializeProtocol()), starting);
        readyToStart->addTransition(this, SIGNAL(InstallFor3pddIncomplete()), readyToInstall);
        readyToStart->addTransition(this, SIGNAL(ShowUnmonitoredInstallDialog()), preInstall);
        readyToStart->addTransition(this, SIGNAL(canceling()), canceling);

        starting->addTransition(this, SIGNAL(ReceivedContentSize(qint64, qint64)), installInfo);
        starting->addTransition(this, SIGNAL(canceling()), canceling);

        installInfo->addTransition(this, SIGNAL(InstallArgumentsSet()), enqueued);
        installInfo->addTransition(this, SIGNAL(canceling()), canceling);

		enqueued->addTransition(this, SIGNAL(StartPreTransfer()), preTransfer);
		enqueued->addTransition(this, SIGNAL(canceling()), canceling);

        preTransfer->addTransition(this, SIGNAL(DownloadProtocolStarted()), transferring);
        preTransfer->addTransition(this, SIGNAL(canceling()), canceling);
       
        transferring->addTransition(this, SIGNAL(DownloadProtocolFinished()), postTransfer);
        transferring->addTransition(this, SIGNAL(Pausing()), pausing);
        transferring->addTransition(this, SIGNAL(ProtocolPaused()), paused);
        transferring->addTransition(this, SIGNAL(canceling()), canceling);

        postTransfer->addTransition(this, SIGNAL(BeginInstall()), readyToInstall);
        postTransfer->addTransition(this, SIGNAL(finishedState()), complete);
        postTransfer->addTransition(this, SIGNAL(ProtocolPaused()), paused);
        postTransfer->addTransition(this, SIGNAL(canceling()), canceling);

        mounting->addTransition(this, SIGNAL(BeginInstall()), install);
        
        readyToInstall->addTransition(this, SIGNAL(WaitingToInstall()), preInstall);
        readyToInstall->addTransition(this, SIGNAL(ResumeInstall()), mounting);

        preInstall->addTransition(this, SIGNAL(BeginInstall()), mounting);
        preInstall->addTransition(this, SIGNAL(BackToReadyToInstall()), readyToInstall);

        install->addTransition(this, SIGNAL(ReceivedContentSize(qint64, qint64)), preTransfer);
        install->addTransition(this, SIGNAL(FinishInstall()), postInstall);
        install->addTransition(this, SIGNAL(canceling()), canceling);
        install->addTransition(this, SIGNAL(PauseInstall()), readyToInstall);
        install->addTransition(this, SIGNAL(BeginUnmount()), unmounting);
        
        unmounting->addTransition(this, SIGNAL(FinishInstall()), postInstall);

        postInstall->addTransition(this, SIGNAL(finishedState()), complete);
        postInstall->addTransition(this, SIGNAL(canceling()), canceling);

        pausing->addTransition(this, SIGNAL(ProtocolPaused()), paused);

        paused->addTransition(this, SIGNAL(Resuming()), resuming);
        paused->addTransition(this, SIGNAL(canceling()), canceling);

        resuming->addTransition(this, SIGNAL(ResumeTransfer()), transferring);

        error->addTransition(this, SIGNAL(ProtocolPaused()), paused);
        error->addTransition(this, SIGNAL(Pausing()), pausing);
        error->addTransition(this, SIGNAL(InstallFailed()), readyToInstall);
        error->addTransition(this, SIGNAL(resetForReadyToStart()), readyToStart);
    }

    void ContentInstallFlowNonDiP::initializeInstallManifest()
    {
        ContentServices::InitializeInstallManifest(mContent, mInstallManifest);

        mInstallManifest.SetString("serverfile", GetServerFile());
        mInstallManifest.SetString("downloadPath", mContent->localContent()->nonDipInstallerPath());
    }

    QString ContentInstallFlowNonDiP::GetServerFile() const
    {
        QString localFilename;
        ContentServices::GetContentDownloadCachePath(mContent, localFilename);

        localFilename.replace("\\", "/");
        if (!localFilename.endsWith("/"))
        {
            localFilename.append("/");
        }
        QUrl contentUrl(mUrl);
        QFileInfo fileInfo(contentUrl.path());
        localFilename.append(fileInfo.fileName());

        return localFilename;
    }

    qint64 ContentInstallFlowNonDiP::getContentTotalByteCount()
    {
        qint64 totalBytes = mInstallManifest.Get64BitInt("totalbytes");
        return totalBytes;
    }

    qint64 ContentInstallFlowNonDiP::getContentSizeToDownload()
    {
        qint64 totalBytes = mInstallManifest.Get64BitInt("totaldownloadbytes");
        return totalBytes;
    }

    qint64 ContentInstallFlowNonDiP::getDownloadedByteCount()
    {
        qint64 savedBytes = mInstallManifest.Get64BitInt("savedbytes");
        return savedBytes;
    }

    QString ContentInstallFlowNonDiP::getDownloadLocation()
    {
        // Retrieve it from the install manifest
        QString installPath = mInstallManifest.GetString("downloadPath");
        if (installPath.isEmpty())
        {
            installPath = mContent->localContent()->nonDipInstallerPath();
        }
        return installPath;
    }

    quint16 ContentInstallFlowNonDiP::getErrorRetryCount() const
    {
        return mDownloadRetryCount;
    }

    void ContentInstallFlowNonDiP::initInstallParameters (QString& installLocale, QString& srcFile, ContentInstallFlags flags)
    {
        Q_UNUSED(srcFile);
        Q_UNUSED(installLocale);

        mSuppressDialogs = flags & kSuppressDialogs;
    }

    void ContentInstallFlowNonDiP::setInstallArguments (const Origin::Downloader::InstallArgumentResponse& args)
    {
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(Origin::Downloader::InstallArgumentResponse, args));

        emit InstallArgumentsSet();
    }

    void ContentInstallFlowNonDiP::setEulaLanguage (const Origin::Downloader::EulaLanguageResponse& response)
    {
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(Origin::Downloader::EulaLanguageResponse, response))

        // Do nothing
    }

    void ContentInstallFlowNonDiP::setEulaState (const Origin::Downloader::EulaStateResponse& response)
    {
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(Origin::Downloader::EulaStateResponse, response))

        // Do nothing
    }

    void ContentInstallFlowNonDiP::sendDownloadStartTelemetry(const QString& status, qint64 savedBytes)
    {
        // Only send telemetry if this is not an overridden download! We only need to check here because the other download-related 
        // metric calls (i.e. Metric_DL_END, Metric_DL_REDOWNLOAD) will silently fail unless we have called Metric_DL_START first.
        // If the user has an override in EACore.ini but is installing via ITO, or if EACore.ini contains the appropriate lines,
        // we DO want to capture that telemetry.
        const bool enableOverrideTelemetry = Origin::Services::readSetting(Origin::Services::SETTING_EnableDownloadOverrideTelemetry, Origin::Services::Session::SessionService::currentSession()).toQVariant().toBool();
        const bool shouldSendTelemetry = !mContent->contentConfiguration()->hasOverride() || isUsingLocalSource() || enableOverrideTelemetry;
        if(shouldSendTelemetry)
        {
            QString cachePath;
            ContentServices::GetContentInstallCachePath(mContent, cachePath, false);
            const bool symlinkDetected = isPathSymlink(cachePath);

            bool isPreload = (mContent->localContent()->releaseControlState() == Origin::Engine::Content::LocalContent::RC_Preload);

            // TELEMETRY: Starting a download.
            GetTelemetryInterface()->Metric_DL_START(Origin::Services::readSetting(Origin::Services::SETTING_EnvironmentName, Services::Session::SessionRef()).toString().toUtf8().data(),
                mContent->contentConfiguration()->productId().toLocal8Bit().constData(), 
                status.toLocal8Bit().constData(),
                getContentTypeString().toLocal8Bit().constData(),
                getPackageTypeString().toLocal8Bit().constData(),
                getLogSafeUrl().toLocal8Bit().constData(),
                savedBytes,
                "NotPatch",
                "NonDiP",
                symlinkDetected,
                false,
                isPreload);
        }
    }

    void ContentInstallFlowNonDiP::begin()
    {
        ASYNC_INVOKE_GUARD;

        // Just in case there's a UI flow not doing it
        if (!escalationServiceEnabled())
            return;

        ORIGIN_LOG_EVENT << "Begin install flow for " << mContent->contentConfiguration()->displayName() << ", ctid: " << mContent->contentConfiguration()->productId();

        if (canBegin())
        {
			setPausedForUnretryableError(false);	// be sure to reset this for each new download

            // We are about to start the flow. If the autostart flag is set we need to
            // unset it so an autostart does not happen again unless the flow encounters
            // conditions were it needs to be set again.
            setAutoStartFlag(false);

            sendDownloadStartTelemetry("new",0);

            if (mUnmonitoredInstallInProgress)
            {
                if (QFile::exists(GetInstallerPath()))
                {
                    emit ShowUnmonitoredInstallDialog();
                    return;
                }
                else
                {
                    mUnmonitoredInstallInProgress = false;
                }
            }

            // Generate a new download job UUID
            generateNewJobId();

            ORIGIN_LOG_EVENT << "Begin Download " << mContent->contentConfiguration()->displayName() << ", ctid: " << mContent->contentConfiguration()->productId() << " with Job ID: " << currentDownloadJobId();

            Origin::Engine::UserRef user = mUser.toStrongRef();
            bool online = user && (Origin::Services::Connection::ConnectionStatesService::isUserOnline (user->getSession()));
            if ( online || !DownloadService::IsNetworkDownload(mUrl))
            {
                emit BeginInitializeProtocol();
            }
            else
            {
                setAutoStartFlag(true);
            }
        }
        else
        {
            QString warnMsg = QString("Unable to begin download. Download may already be in progress. Current state = %1").arg(getFlowState().toString());
            ORIGIN_LOG_WARNING << warnMsg;
            emit warn(FlowError::CommandInvalid, warnMsg);
        }
    }

    void ContentInstallFlowNonDiP::resume()
    {
        ASYNC_INVOKE_GUARD;

        // Just in case there's a UI flow not doing it
        if (!escalationServiceEnabled())
            return;

        ORIGIN_LOG_EVENT << "Resume install flow for " << mContent->contentConfiguration()->displayName() << ", ctid: " << mContent->contentConfiguration()->productId() << " with Job ID: " << currentDownloadJobId();

        if (canResume())
        {
            // We are about to resume the flow. If the autoresume flag is set we need to
            // unset it so an autoresume does not happen again unless the flow encounters
            // conditions were it needs to be set again.
            mInstallManifest.SetBool("autoresume", false);

			setPausedForUnretryableError(false);	// assumes the user manually is retrying so reset

            ContentInstallFlowState previousState = getPreviousState();
            ContentInstallFlowState currentState = getFlowState();

            if (currentState == ContentInstallFlowState::kReadyToInstall)
            {
				if(!mContent->contentConfiguration()->showKeyDialogOnInstall())
                {
                    emit ResumeInstall();
                }
                else
                {
                    emit WaitingToInstall();
                }
                return;
            }

            if (currentState == ContentInstallFlowState::kPreInstall)
            {
                return;
            }

            Origin::Engine::UserRef user = mUser.toStrongRef();
            bool online = user && (Origin::Services::Connection::ConnectionStatesService::isUserOnline (user->getSession()));
            if ((previousState == ContentInstallFlowState::kReadyToStart || previousState == ContentInstallFlowState::kTransferring) &&
                !online && 
                DownloadService::IsNetworkDownload(mUrl))
            {
                mInstallManifest.SetBool("autoresume", true);
                return;
            }

            switch (previousState)
            {
            case ContentInstallFlowState::kTransferring:
            case ContentInstallFlowState::kPaused:
            default:
                {
                    SetupDownloadProtocol();
                }
            }
        }
        else
        {
            QString warnMsg = QString("Unable to resume download. Current state = %1").arg(getFlowState().toString());
            ORIGIN_LOG_WARNING << warnMsg;
            emit warn(FlowError::CommandInvalid, warnMsg);
        }
    }

    void ContentInstallFlowNonDiP::pause(bool autoresume)
    {
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(bool, autoresume));

        ORIGIN_LOG_EVENT << "Pause install flow for " << mContent->contentConfiguration()->displayName() << ", ctid: " << mContent->contentConfiguration()->productId() << " with Job ID: " << currentDownloadJobId();

        if (canPause())
        {
            mInstallManifest.SetBool("autoresume", autoresume);
            emit Pausing();
            if (mDownloadProtocol)
                mDownloadProtocol->Pause();
        }
        // If autoresume is set to true, it means that the application and not the user
        // initiated the pause. If the application initiated the pause, and we are not
        // in a pausable state, we want to suspend the flow which will set the flow to
        // ready to install if currently installing and ready to start if the transfer
        // state has not been reached yet.
        else if (autoresume)
        {
            suspend(autoresume);
        }
        else
        {
            QString warnMsg = QString("Unable to pause download. Current state = %1").arg(getFlowState().toString());
            ORIGIN_LOG_WARNING << warnMsg;
            emit warn(FlowError::CommandInvalid, warnMsg);
        }
    }

    //This should only be called in rare instances, like when Origin is about to shutdown.
    //Flows which have not progressed to the transferring state may get canceled.
    // Sets flows that get paused to auto resume and those that get canceled to auto start.
    void ContentInstallFlowNonDiP::suspend(bool autostart)
    {
        ContentInstallFlowState currentState = getFlowState();

        switch (currentState)
        {
            case ContentInstallFlowState::kReadyToStart:
            case ContentInstallFlowState::kReadyToInstall:
            case ContentInstallFlowState::kPaused:
			case ContentInstallFlowState::kEnqueued:
            {
                ORIGIN_LOG_EVENT << "No suspend action required, content = " << mContent->contentConfiguration()->productId();

                // do nothing, but just emit that we're done
                emit (suspendComplete(mContent->contentConfiguration()->productId()));
                break;
            }

            case ContentInstallFlowState::kPausing:
            {
                ORIGIN_LOG_WARNING << "Already in pausing... = " << mContent->contentConfiguration()->productId();
                //since it's in the middle of pausing, just let it finish, it will emit protocolPaused() when done
                //and if for some reason, it doens't finish, the timeout in ContentController will just force quit
                break;
            }
            
            case ContentInstallFlowState::kCanceling:
            {
                ORIGIN_LOG_WARNING << "Already in canceling... = " << mContent->contentConfiguration()->productId();
                //since it's in the middle of canceling, just let it finish, it will rever to kReadyToStart when done
                //and if for some reason, it doens't finish, the timeout in ContentController will just force quit
                break;
            }

            case ContentInstallFlowState::kTransferring:
            {
                // pause normally
                pause(autostart);
                break;
            }
            case ContentInstallFlowState::kResuming:
            {
                // We are about to go or are offline so the download session will most likely encounter errors. We want to ignore them.
                ORIGIN_VERIFY_DISCONNECT(mDownloadProtocol, SIGNAL(IContentProtocol_Error(Origin::Downloader::DownloadErrorInfo)), this, SLOT(onProtocolError(Origin::Downloader::DownloadErrorInfo)));

                // intentional fall-through
            }
            case ContentInstallFlowState::kPostTransfer:
            {
                ORIGIN_LOG_WARNING << "Force pause " << mContent->contentConfiguration()->productId();

                mInstallManifest.SetBool("autoresume", autostart);

                if (mDownloadProtocol)
                {
                    mDownloadProtocol->Pause(true);
                }
                else
                {
                    emit (ProtocolPaused());
                }
                break;
            }
            case ContentInstallFlowState::kInstalling:
            {
                ORIGIN_LOG_WARNING << "Force pause install, content = " << mContent->contentConfiguration()->productId();

                QFileInfo localFile = QFileInfo(mInstallManifest.GetString("serverfile"));
                
                if (IsDiskImageFileExtension(localFile.suffix()))
                {
                    // If we are installing from a mounted location drop back to readyToInstall so we can re-mount when Origin comes back
                    emit BackToReadyToInstall();
                }
                else
                {
                    mInstallManifest.SetBool("autoresume", autostart);
                    
                    // go back to ready to install
                    emit PauseInstall();
                }
                
                break;
            }
            
            case ContentInstallFlowState::kPreInstall:
            {
                ORIGIN_LOG_WARNING << "Force pause pre-install, content = " << mContent->contentConfiguration()->productId();

                // go back to ready to install
                emit BackToReadyToInstall();
                break;
            }
            case ContentInstallFlowState::kMounting:
            {
                ORIGIN_LOG_WARNING << "Suspending while mounting...";

                emit BackToReadyToInstall();

                break;
            }
            case ContentInstallFlowState::kUnmounting:
            {
                ORIGIN_LOG_WARNING << "Suspending while unmounting...";

                emit BackToReadyToInstall();

                break;
            }
            case ContentInstallFlowState::kInitializing:
            case ContentInstallFlowState::kPreTransfer:
            case ContentInstallFlowState::kPendingEulaLangSelection:
            default:       
            {
                ORIGIN_LOG_WARNING << "Force cancel, content = " << mContent->contentConfiguration()->productId() <<
                    ", state = " << currentState.toString();

                setAutoStartFlag(false); // Clear the autostart flag for flows that have not yet reached the kTransferring state.

                if (mDownloadProtocol)
                {
                    // We are about to go or are offline so the download session will most likely encounter errors. We want to ignore them.
                    ORIGIN_VERIFY_DISCONNECT(mDownloadProtocol, SIGNAL(IContentProtocol_Error(Origin::Downloader::DownloadErrorInfo)), this, SLOT(onProtocolError(Origin::Downloader::DownloadErrorInfo)));

                    //in this case, we're manually setting the state back to kReadyToStart so there wouldn't be a signal sent out for state change that
                    //the ContentController is listening for (so that it can remove from the list of pending suspend jobs)
                    //so set a flag to indicate that when onProtocolPaused() is called, it will emit the suspendComplete signal
                    mProcessingSuspend = true;
                    mDownloadProtocol->Pause(true);
                }
                else
                {
                    stop();
                }
            }
        }

        ContentServices::SaveInstallManifest(mContent, mInstallManifest);
    }

	void ContentInstallFlowNonDiP::startTransferFromQueue (bool manual_start)
	{
		ASYNC_INVOKE_GUARD_ARGS(Q_ARG(bool, manual_start));

		ContentInstallFlowState currentState = getFlowState();

		if (currentState == ContentInstallFlowState::kInstalling)
		{
			ORIGIN_LOG_WARNING << "startTransferFromQueue called for " << mContent->contentConfiguration()->productId() << " but current state is kInstalling";
			return;
		}

		if ((currentState != ContentInstallFlowState::kEnqueued) && (currentState != ContentInstallFlowState::kPaused))
		{
			ORIGIN_LOG_ERROR << "startTransferFromQueue called for " << mContent->contentConfiguration()->productId() << " but we are not in kEnqueued state.  Current state is " << currentState.toString();
			return;			
		}

        // if manually starting (from UI) bypass this check - unretryable error flag will be reset in resume()
		if (!manual_start && pausedForUnretryableError())	// an error occurred that we can't retry on so leave it paused as we assume anything below the head if in error state is also in an error state.
		{
			ORIGIN_LOG_WARNING << "startTransferFromQueue called for " << mContent->contentConfiguration()->productId() << " but it is paused for a unretryable error";
			return;
		}

		ORIGIN_LOG_EVENT << "Start transferring off queue for " << mContent->contentConfiguration()->productId();

		if (currentState == ContentInstallFlowState::kPaused)
			resume();
		else
			emit StartPreTransfer();
	}

    void ContentInstallFlowNonDiP::cancel(bool forceCancelAndUninstall) // forceCancelAndUninstall not supported in NonDiP
    {
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(bool, forceCancelAndUninstall));

        ORIGIN_LOG_EVENT << "Cancel install flow for " << mContent->contentConfiguration()->displayName() << ", ctid: " << mContent->contentConfiguration()->productId() << " with Job ID: " << currentDownloadJobId();

        if (canCancel())
        {
            if(mContent->localContent()->previousInstalledLocale().count())
            {
                mContent->localContent()->setInstalledLocale(mContent->localContent()->previousInstalledLocale());
                mContent->localContent()->setPreviousInstalledLocale("");
            }

			setPausedForUnretryableError(false);	// reset

            // TELEMETRY: User has canceled the download.
            bool isPreload = (mContent->localContent()->releaseControlState() == Origin::Engine::Content::LocalContent::RC_Preload);
            GetTelemetryInterface()->Metric_DL_CANCEL(mContent->contentConfiguration()->productId().toLocal8Bit().constData(), isPreload);

            emit canceling();

            if (!mDownloadProtocol)
            {
                CreateDownloadProtocol();


                QString serverFile(GetServerFile());
                QFileInfo serverFileInfo(serverFile);        

                // If we are a package protocol we nee to set the unpack directory so it knows what files to remove
                ContentProtocolPackage* packageProtocol = dynamic_cast<ContentProtocolPackage*>(mDownloadProtocol);

                if(packageProtocol != NULL)
                {
                    packageProtocol->SetUnpackDirectory(serverFileInfo.path());
                }
                else
                {
                    ContentProtocolSingleFile* singleFileProtocol = dynamic_cast<ContentProtocolSingleFile*>(mDownloadProtocol);
                    if(singleFileProtocol)
                    {
                        singleFileProtocol->SetTargetFile(serverFileInfo.absoluteFilePath());
                    }
                }
            }

            mDownloadProtocol->Cancel();
        }
        else
        {
            QString warnMsg = QString("Unable to cancel download. Current state = %1").arg(getFlowState().toString());
            ORIGIN_LOG_WARNING << warnMsg;
            emit warn(FlowError::CommandInvalid, warnMsg);
        }
    }

    void ContentInstallFlowNonDiP::retry()
    {
        ASYNC_INVOKE_GUARD;

        // DO nothing
    }

    void ContentInstallFlowNonDiP::handleError(qint32 errorType, qint32 errorCode , QString errorMessage)
    {
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG (qint32, errorType), Q_ARG(qint32, errorCode), Q_ARG(QString, errorMessage))

        /*ContentInstallFlowState state = getFlowState();
        if (state != ContentInstallFlowState::kTransferring && state != ContentInstallFlowState::kInstalling &&
            state != ContentInstallFlowState::kReadyToInstall && state != ContentInstallFlowState::kPostTransfer &&
            state != ContentInstallFlowState::kResuming)
        {
            stop();
        }*/
    }

    void ContentInstallFlowNonDiP::contentConfigurationUpdated()
    {
        ASYNC_INVOKE_GUARD;
        
        // Be very careful with what you implement here.  This method will get called anytime a user refreshes
        // their mygames page.   Can happen anytime during the normal download/install flow.

        // Check if we are ready to install, make sure the cached installer is still valid and still exists
        // Make sure installer exists if content is ready to install and not already installed
        if (ContentInstallFlowState(mInstallManifest.GetString("currentstate")) == ContentInstallFlowState::kReadyToInstall)
        {
            if(!validateReadyToInstallState())
            {
                // Either the installer has changed or the file was removed by the user, either way we reset back to ready to start...
                initializeInstallManifest();
                setStartState(ContentInstallFlowState::kReadyToStart);
                QStateMachine::stop();
            }
        }

        // If overrides are enabled (an eacore.ini file is present), the download path may be updated by ODT.  Update the 
        // download path - as long as we are in the kReadyToStart state.
        if(Services::readSetting(Services::SETTING_OverridesEnabled).toQVariant().toBool())
        {
            if (getFlowState() == ContentInstallFlowState::kReadyToStart && mContent)
            {
                initUrl();
            }
        }
    }

    void ContentInstallFlowNonDiP::OnFlowError()
    {
        // We don't want to delete the protocol if the flow is depending on the protocol's Paused signal
        // to transition from kPausing to kPaused. In this case the protocol will be deleted in
        // onProtocolPaused()
        bool deleteProtocol = false;
        ContentInstallFlowState previousState = getPreviousState();

        switch (previousState)
        {
            case ContentInstallFlowState::kReadyToStart:
            {                
                // Special case: if there was already a UAC prompt pending and we had to immediately cancel the download (ie from a kReadyToStart)
                // we wanted to emit an error to force a state change so that LocalContent resets its 'in-use' path - but for that we must go back to the kReadyToStart
                // state
                emit resetForReadyToStart();
            }
            break;

            case ContentInstallFlowState::kReadyToInstall:
            case ContentInstallFlowState::kInstalling:
                emit InstallFailed();
                deleteProtocol = true;
                break;
            case ContentInstallFlowState::kPostTransfer:
            case ContentInstallFlowState::kPaused:
                emit ProtocolPaused();
                deleteProtocol = true;
                break;
            case ContentInstallFlowState::kTransferring:
                // On errors during transfer, the protocol executes its paused logic. We must enter the pausing state
                // and wait for the protocol to indicate that it has finished pausing by emitting its pause signal.
                emit Pausing();

                // For retriable errors that have exceeded the max retry count, the protocol has already
                // emitted its Paused signal during the first failure. We need to re-emit it to put the
                // main state machine into the paused state.
                if (mDownloadRetryCount >= sMaxDownloadRetries)
                {
                    deleteProtocol = true;
                    emit ProtocolPaused();
                }
                break;
            default:
                deleteProtocol = true;
                stop();
                break;
        }

        mDownloadRetryCount = 0;

        if (deleteProtocol && mDownloadProtocol)
        {
            // Delete the protocol on it's own thread (queue the destructor on the event loop)
            mDownloadProtocol->deleteLater();
            mDownloadProtocol = NULL;
        }

        if (!mErrorPauseQueue)
        {
            // move down the queue here
		    // must protect against user logging out which results in a null queueController
		    Engine::Content::ContentOperationQueueController* queueController = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController();
		    if (queueController)
            {
                queueController->moveToBottom(mContent);
            }
        }
        else
            mErrorPauseQueue = false;   // reset

		setPausedForUnretryableError(true);	// so new enqueued items will skip over this
    }

    bool ContentInstallFlowNonDiP::canBegin()
    {
        return true;
    }

    bool ContentInstallFlowNonDiP::canCancel()
    {
        ContentInstallFlowState currentState = getFlowState();
        return currentState != ContentInstallFlowState::kReadyToStart && currentState != ContentInstallFlowState::kInitializing && currentState != ContentInstallFlowState::kPausing &&
            currentState != ContentInstallFlowState::kResuming && currentState != ContentInstallFlowState::kCanceling && currentState != ContentInstallFlowState::kInstalling;
    }

    bool ContentInstallFlowNonDiP::canPause()
    {
        ContentInstallFlowState currentState = getFlowState();
        return currentState == ContentInstallFlowState::kTransferring;
    }

    bool ContentInstallFlowNonDiP::canResume()
    {
        ContentInstallFlowState state = getFlowState();
        Engine::UserRef user(mUser.toStrongRef());
        bool isOnlineOrLocalDownload = ((user && Origin::Services::Connection::ConnectionStatesService::isUserOnline (user->getSession())) || !DownloadService::IsNetworkDownload(mUrl));

        if (!isOnlineOrLocalDownload && state == ContentInstallFlowState::kPaused)
        {
            ORIGIN_LOG_WARNING << "Unable to resume download of " << mContent->contentConfiguration()->productId() << " while offline.";
        }

        //if we are online or its a local download and we are paused or ready to install, then we can resume
        return (isOnlineOrLocalDownload && (state == ContentInstallFlowState::kPaused)) || state == ContentInstallFlowState::kReadyToInstall || state == ContentInstallFlowState::kPreInstall;
    }

    bool ContentInstallFlowNonDiP::canRetry()
    {
        return false;
    }

    bool ContentInstallFlowNonDiP::isActive()
    {
        ContentInstallFlowState currentState = getFlowState();
        return (currentState != ContentInstallFlowState::kReadyToStart && currentState != ContentInstallFlowState::kCompleted &&
            currentState != ContentInstallFlowState::kInvalid);
    }

    bool ContentInstallFlowNonDiP::isRepairing() const
    {
        return false;
    }

    bool ContentInstallFlowNonDiP::isUpdate() const
    {
        return false;
    }

    void ContentInstallFlowNonDiP::MakeJitUrlRequest()
    {
        if (!makeOnlineCheck("User is offline. Unable to retrieve content url"))
        {
            // The flow will get suspended in IContentInstallFlow::onConnectionStateChanged
            return;
        }

        Engine::UserRef user(mUser.toStrongRef());

        bool useHTTPS = false;

        // Use SSL/TLS when in safe mode
        bool downloaderSafeMode = Origin::Services::readSetting(Origin::Services::SETTING_EnableDownloaderSafeMode);
        if (downloaderSafeMode)
        {
            useHTTPS = true;

            ORIGIN_LOG_EVENT << "Requesting HTTPS URL due to Downloader Safe Mode.";
        }
        else
        {
            // Use SSL/TLS when enabled via EACore.ini override
            bool httpsOverrideEnabled = Origin::Services::readSetting(Origin::Services::SETTING_EnableHTTPS);
            if (httpsOverrideEnabled)
            {
                useHTTPS = true;

                ORIGIN_LOG_EVENT << "Requesting HTTPS URL due to EACore.ini override.";
            }
        }

        Origin::Services::Publishing::DownloadUrlResponse* resp = Origin::Services::Publishing::DownloadUrlProviderManager::instance()->downloadUrl(Origin::Services::Session::SessionService::currentSession(),
            mContent->contentConfiguration()->productId(), mContent->contentConfiguration()->buildIdentifierOverride(), mContent->contentConfiguration()->buildReleaseVersionOverride(), "", useHTTPS);
        ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(CompleteSetupProtocolWithJitUrl()));
    }

    void ContentInstallFlowNonDiP::SetupDownloadProtocol()
    {
        if (!mDownloadProtocol)
        {
            CreateDownloadProtocol();
        }

        // creates the cache dir if it doesn't exist
		int contentDownloadErrorType = 0;
        int elevation_error = 0;
		QString cachePath;
		ContentServices::GetContentInstallCachePath(mContent, cachePath, true, &contentDownloadErrorType, &elevation_error);
        
		// if the install cache folder creation returned an escalation error
		// we cannot continue, so report the error and exit. (EBIBUGS-18168)
		if (contentDownloadErrorType != ContentDownloadError::OK)
		{
			CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
			errorInfo.mErrorType = contentDownloadErrorType;
			errorInfo.mErrorCode = elevation_error;
            errorInfo.mErrorContext = cachePath;
			OnProtocolError(errorInfo);
			return;
		}

        // create download cache path if it doesn't exist
        ContentServices::GetContentDownloadCachePath(mContent, cachePath, true);

        if (!mIsOverrideUrl) // JIT url generation
        {
            MakeJitUrlRequest();
            return; // we complete the Initialization in CompleteSetupProtocolWithJitUrl
        }

        BeginDownloadProtocolInitialization();
    }

    void ContentInstallFlowNonDiP::BeginDownloadProtocolInitialization()
    {
        if (mDownloadProtocol)
        {
            QString serverFile(GetServerFile());
            QFileInfo serverFileInfo(serverFile);
            mInstallManifest.SetString("serverfile", serverFile);
            ContentServices::SaveInstallManifest(mContent, mInstallManifest);

            // If the content is a packed file and not addon or bonus content, a package protocol was created which requires
            // a directory for the target. Otherwise, a single file protocol was created which requires a target file.
            if((mContent->contentConfiguration()->packageType() == Origin::Services::Publishing::PackageTypeUnpacked ||
                IsPackedFileExtension(serverFileInfo.suffix())) && !mContent->contentConfiguration()->isAddonOrBonusContent() 
               && !IsDiskImageFileExtension(serverFileInfo.suffix()))
            {
                mDownloadProtocol->Initialize(mUrl, serverFileInfo.path(), false);
            }
            else
            {
                mDownloadProtocol->Initialize(mUrl, serverFileInfo.absoluteFilePath(), false);
            }
        }
    }

    void ContentInstallFlowNonDiP::CreateDownloadProtocol()
    {
        QFileInfo localFile = QFileInfo(mInstallManifest.GetString("serverfile"));

        Engine::UserRef user(mUser.toStrongRef());
        ORIGIN_ASSERT(user);
        // We unpack viv, zip on the fly during download, and everything else is downloaded a single file and processed locally (i.e. encrypted vivs and .exes)
        // Addons and bonus material must use the single file protocol even if in zip format
        if((mContent->contentConfiguration()->packageType() == Origin::Services::Publishing::PackageTypeUnpacked ||
            IsPackedFileExtension(localFile.suffix())) && !mContent->contentConfiguration()->isAddonOrBonusContent() && !IsDiskImageFileExtension(localFile.suffix()))
        {
            QString cachePath;
            ContentServices::GetContentInstallCachePath(mContent, cachePath);

            mDownloadProtocol = new ContentProtocolPackage(mContent->contentConfiguration()->productId(),cachePath, "", user->getSession());
            dynamic_cast<ContentProtocolPackage*>(mDownloadProtocol)->SetCrcMapKey(mContent->contentConfiguration()->productId());
        }
        else
        {
            mDownloadProtocol = new ContentProtocolSingleFile(mContent->contentConfiguration()->productId(), user->getSession());
        }
        
        mDownloadProtocol->setGameTitle(mContent->contentConfiguration()->displayName());
        mDownloadProtocol->setJobId(currentDownloadJobId());

        ORIGIN_VERIFY_CONNECT_EX(mDownloadProtocol, SIGNAL(Initialized()), this, SLOT(OnDownloadProtocolInitialized()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(mDownloadProtocol, SIGNAL(Started()), this, SLOT(OnDownloadProtocolStarted()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(mDownloadProtocol, SIGNAL(Finished()), this, SLOT(OnDownloadProtocolFinished()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(mDownloadProtocol, SIGNAL(ContentLength(qint64, qint64)), this, SLOT(SetContentSize(qint64, qint64)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(mDownloadProtocol, SIGNAL(TransferProgress(qint64, qint64, qint64, qint64)), this, SLOT(OnTransferProgress(qint64, qint64, qint64, qint64)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(mDownloadProtocol, SIGNAL(Resumed()), this, SLOT(OnDownloadProtocolResumed()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(mDownloadProtocol, SIGNAL(Paused()), this, SLOT(OnProtocolPaused()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(mDownloadProtocol, SIGNAL(Canceled()), this, SLOT(OnCanceled()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(mDownloadProtocol, SIGNAL(IContentProtocol_Error(Origin::Downloader::DownloadErrorInfo)), this, SLOT(OnProtocolError(Origin::Downloader::DownloadErrorInfo)), Qt::QueuedConnection);

        ContentProtocols::RegisterProtocol(mDownloadProtocol);
    }

    void ContentInstallFlowNonDiP::CompleteSetupProtocolWithJitUrl()
    {
        if (!makeOnlineCheck("User is offline. Unable to setup protocol with Jit Url."))
        {
            //The flow will get suspended in IContentInstallFlow::onConnectionStateChanged
            return;
        }

        Engine::UserRef user(mUser.toStrongRef());

        Origin::Services::Publishing::DownloadUrlResponse* response = dynamic_cast<Origin::Services::Publishing::DownloadUrlResponse*>(sender());
        ORIGIN_ASSERT(response);
        ORIGIN_VERIFY_DISCONNECT(response, SIGNAL(finished()), this, SLOT(CompleteSetupProtocolWithJitUrl()));
        response->deleteLater();

        if(response && response->error() == Origin::Services::restErrorSuccess)
        {
            //ORIGIN_LOG_DEBUG << "Retrieved JIT URL " << response->downloadUrl().mURL << " for product ID " << mContent->contentConfiguration()->productId();
            mUrl = response->downloadUrl().mURL;

            BeginDownloadProtocolInitialization();
        }
        else
        {
            // Error state
            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = FlowError::JitUrlRequestFailed;
            errorInfo.mErrorCode = response == NULL ? 0 : response->error();
            OnProtocolError(errorInfo);
        }
    }

    void ContentInstallFlowNonDiP::OnCanceled()
    {
        ContentServices::DeleteInstallManifest(mContent);
        ContentServices::ClearDownloadCache(mContent);

        // Reset back to an clean install manifest
        initializeInstallManifest();
        QStateMachine::stop();

        // Clear the job ID
        clearJobId();

        if (mDownloadProtocol)
        {
            // Delete the protocol on it's own thread (queue the destructor on the event loop)
            mDownloadProtocol->deleteLater();
            mDownloadProtocol = NULL;
        }

        removeFromQueue(true);
    }

    void ContentInstallFlowNonDiP::OnComplete()
    {
#ifdef ORIGIN_MAC
        // Create a plist in the game bundle to save content data for EA Access.
        ContentServices::CreateAccessPlist (mContent, Origin::Engine::LoginController::instance()->currentUser()->locale(), Services::PlatformService::localDirectoryFromOSPath(mContent->contentConfiguration()->installCheck()));
#endif
     
        ORIGIN_LOG_EVENT << "Install flow completed for " << mContent->contentConfiguration()->displayName() << ", ctid: " << mContent->contentConfiguration()->productId() << " with Job ID: " << currentDownloadJobId();

        // Since it isn't present in the content xml, need to set the install check for addon and bonus
        // content in the install manifest.
        if (mContent->contentConfiguration()->isAddonOrBonusContent())
        {
            QString installCheck = mInstallManifest.GetString("serverfile");
            mContent->localContent()->setAddonInstallCheck(installCheck);
            mInstallManifest.SetString("addoninstallcheck", installCheck);
        }
        // If the keep installers setting is not set and the content is not 3PDD,
        // delete all files in the content's download cache.
        else if (!Origin::Services::readSetting(Origin::Services::SETTING_KEEPINSTALLERS, Origin::Services::Session::SessionService::currentSession()))
        {
            if(mContent->contentConfiguration()->monitorInstall()) // do only for non-3PDD
                ContentServices::ClearDownloadCache(mContent);
        }

        // Destroy the protocol
        if (mDownloadProtocol)
        {
            // Delete the protocol on it's own thread (queue the destructor on the event loop)
            mDownloadProtocol->deleteLater();
            mDownloadProtocol = NULL;
        }

        QStateMachine::stop();

        // Clear the job ID
        clearJobId();

		// signal UI that the content is now playable
		emit contentPlayable();
    }

	void ContentInstallFlowNonDiP::OnReadyToInstall()
	{
		// must protect against user logging out which results in a null queueController
		Engine::Content::ContentOperationQueueController* queueController = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController();
		if (queueController)
        {
            queueController->dequeue(mContent);
        }
	}

    void ContentInstallFlowNonDiP::OnPreInstall()
    {
        if(mContent->contentConfiguration()->showKeyDialogOnInstall())
        {
            ThirdPartyDDRequest req;
            req.displayName = mContent->contentConfiguration()->displayName();
            req.productId = mContent->contentConfiguration()->productId();
            req.cdkey = mContent->contentConfiguration()->cdKey(); 
            req.monitoredInstall = mContent->contentConfiguration()->monitorInstall();
            req.monitoredPlay = mContent->contentConfiguration()->monitorPlay();
            req.partnerPlatform = mContent->contentConfiguration()->partnerPlatform();

            emit pending3PDDDialog(req);
        }
    }

    void ContentInstallFlowNonDiP::OnInstall()
    {
        // This appears to be unused. Note that this field can be overridden
        // by eacore.ini.
        QString instCheck = mContent->contentConfiguration()->installCheck();

        mUnmonitoredInstallInProgress = !mContent->contentConfiguration()->monitorInstall();

        QString localFilename(mInstallManifest.GetString("serverfile"));
        QFileInfo localFileInfo(localFilename);

        //packedFilename will be empty if the install flow never went through the unpack state
        QString packedFilename(mInstallManifest.GetString("packedfile"));


        // TELEMETRY: A game has started installing.
        GetTelemetryInterface()->Metric_GAME_INSTALL_START(mContent->contentConfiguration()->productId().toLocal8Bit().constData(), getPackageTypeString().toLocal8Bit().constData());

        // in case we were at "Ready To Install" on the Completed queue (due to installer error)
		// must protect against user logging out which results in a null queueController
		Engine::Content::ContentOperationQueueController* queueController = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController();
		if (queueController)
        {
            queueController->enqueueAndTryToPushToTop(mContent);
        }

#ifdef ORIGIN_PC
        // There is no registry on the Mac.  Content data will be saved to a plist in the game bundle after installation.  See ContentInstallFlowNonDiP::onComplete().
        bool isHKCU = true;
        ContentServices::setHKeyCurrentUser(mContent, isHKCU);
        ContentServices::ExportStagingKeys();
        ContentServices::setHKeyLocalMachine(mContent, Origin::Engine::LoginController::instance()->currentUser()->locale());
        //if we are not using HKCU as the staging path we must write the staging path reg entry to the staging.reg file
        //first, and have the proxy installer import the reg settings in, otherwise with UAC on we cannot write the entries directly into the registry
        if(!isHKCU)
        {
            ContentServices::setContentHKey(mContent);
        }
#endif
        
        if (!packedFilename.isEmpty())
        {
            QString installerPath(GetInstallerPath());
            if (QFile::exists(installerPath))
            {
                Origin::Util::DiskUtil::DeleteFileWithRetry(localFilename);
                ExecuteInstallerRequest request(installerPath, mContent->contentConfiguration()->installerParams(),
                    QFileInfo(installerPath).path(),true, true, mContent->contentConfiguration()->monitorInstall());

                mInstaller->makeInstallRequest(request);
            }
            else
            {
                ORIGIN_LOG_ERROR << "Installer file does not exist: " << localFilename << ". Retry download";

                // after initialize completetion a ReceivedContentSize signal is emitted which
                // transitions the state machine from kInstalling to kPreTransfer
                SetupDownloadProtocol();
            }
        }
        else if (IsExecutableFileExtension(localFileInfo.suffix()))
        {
            // Make sure installer didn't get deleted somehow
            if (QFile::exists(localFilename))
            {
                ExecuteInstallerRequest request(localFilename, mContent->contentConfiguration()->installerParams(),
                    QFileInfo(localFilename).path(),true, true, mContent->contentConfiguration()->monitorInstall());

                mInstaller->makeInstallRequest(request);
            }
            // retry download
            else
            {
                ORIGIN_LOG_ERROR << "Installer file does not exist: " << localFilename << ". Retry download";

                // after initialize completetion a ReceivedContentSize signal is emitted which
                // transitions the state machine from kInstalling to kPreTransfer
                SetupDownloadProtocol();
            }
        }
        else if (IsDiskImageFileExtension(localFileInfo.suffix()))
        {
            QString mounted_setup_path = mounter.mountPath().trimmed() + "/" + mContent->contentConfiguration()->InstallerPath().trimmed();

            ORIGIN_LOG_EVENT << "Mounted setup path: " << mounted_setup_path;
            
            // Make sure installer didn't get deleted or unmounted
            if (QFile::exists(mounted_setup_path))
            {
                
                ExecuteInstallerRequest request(mounted_setup_path, mContent->contentConfiguration()->installerParams(),
                                                QFileInfo(localFilename).path(),true, true, mContent->contentConfiguration()->monitorInstall());
                
                mInstaller->makeInstallRequest(request);
            }
            // retry download
            else
            {
                // Note: is it appropriate to retry the download?
                //
                ORIGIN_LOG_ERROR << "Mounted path does not exist: " << mounted_setup_path << ". Unable to proceed with download.";
                
                // after initialize completetion a ReceivedContentSize signal is emitted which
                // transitions the state machine from kInstalling to kPreTransfer
                //SetupDownloadProtocol();
            }                        
        }
        else
        {
            ORIGIN_LOG_ERROR << "Corrupt install manifest for : " << mContent->contentConfiguration()->productId() << ". Retry download";

            // after initialize completetion a ReceivedContentSize signal is emitted which
            // transitions the state machine from kInstalling to kPreTransfer
            SetupDownloadProtocol();
        }
    }

    void ContentInstallFlowNonDiP::OnInstallerError(Origin::Downloader::DownloadErrorInfo errorInfo)
    {
        QString localFilename(mInstallManifest.GetString("serverfile"));
        QFileInfo localFileInfo(localFilename);
        
        if (IsDiskImageFileExtension(localFileInfo.suffix()))
        {
            // The Installing state already has a handler for the unmounter after a successful install, and there
            // isn't a great way to discriminate these two cases in the handler. The cleanest solution is to just
            // disconnect the handlers here and unmount without any signaling and behave synchronously.
            
            ORIGIN_VERIFY_DISCONNECT(&mounter, SIGNAL(unmountComplete(int)), this, SLOT(OnUnmountComplete(int)));
            mounter.unmount();
            ORIGIN_VERIFY_CONNECT(&mounter, SIGNAL(unmountComplete(int)), this, SLOT(OnUnmountComplete(int)));
        }
        
        // TELEMETRY: A game has encountered a fatal error during installation.
        GetTelemetryInterface()->Metric_GAME_INSTALL_ERROR(mContent->contentConfiguration()->productId().toLocal8Bit().constData(), getPackageTypeString().toLocal8Bit().constData(), errorInfo.mErrorType, errorInfo.mErrorCode);

        if (errorInfo.mErrorType == InstallError::InstallerBlocked)
        {
            // Another installer was running at the same time. Try again.
            ORIGIN_LOG_EVENT << "Installer could not start due to another executing installer. Pausing installation.";
            emit PauseInstall();

            emit warn(InstallError::InstallerBlocked, ErrorTranslator::Translate(
                (ContentDownloadError::type)InstallError::InstallerBlocked));
        }
        else if (errorInfo.mErrorType == InstallError::InstallCheckFailed)
        {
            ORIGIN_LOG_EVENT << "Installer reported success but the install check failed for " <<
                mContent->contentConfiguration()->productId() << ". User may have canceled the installer.";
            emit PauseInstall();

            emit warn(InstallError::InstallCheckFailed, ErrorTranslator::Translate(
                (ContentDownloadError::type)InstallError::InstallCheckFailed));
        }
        else
        {
            QString errorDescription = ErrorTranslator::Translate((ContentDownloadError::type)errorInfo.mErrorType);
            emit IContentInstallFlow_error(errorInfo.mErrorType, errorInfo.mErrorCode, errorDescription, mContent->contentConfiguration()->productId());
        }

		// move to completed so it can be installed later from there by user
		// must protect against user logging out which results in a null queueController
		Engine::Content::ContentOperationQueueController* queueController = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController();
		if (queueController)
        {
            queueController->dequeue(mContent);
        }
    }

    void ContentInstallFlowNonDiP::OnInstallerFinished()
    {
        if (mContent->localContent()->installed(true) || !mContent->contentConfiguration()->monitorInstall())
        {
            QString localFilename(mInstallManifest.GetString("serverfile"));
            QFileInfo localFileInfo(localFilename);

            // TELEMETRY: A game has been successfully installed.
            GetTelemetryInterface()->Metric_GAME_INSTALL_SUCCESS(mContent->contentConfiguration()->productId().toLocal8Bit().constData(), getPackageTypeString().toLocal8Bit().constData());

            if (IsDiskImageFileExtension(localFileInfo.suffix()))
            {
                emit BeginUnmount();
            }
            else
            {
                emit FinishInstall();
            }
        }
        else
        {
            ORIGIN_LOG_ERROR << "Installer process reported finished but the game is not ready to play. Content = " << mContent->contentConfiguration()->productId();

            emit PauseInstall();
        }
    }

    void ContentInstallFlowNonDiP::OnInstallInfo()
    {
        InstallArgumentRequest req;
        req.contentDisplayName = mContent->contentConfiguration()->displayName();
        req.productId = mContent->contentConfiguration()->productId();
        req.installedSize = getContentTotalByteCount();
        req.downloadSize = getContentSizeToDownload();
		req.isDip = false;
        req.isPreload = (mContent->localContent()->releaseControlState() == Origin::Engine::Content::LocalContent::RC_Preload);
        QFileInfo localFile = QFileInfo(mInstallManifest.GetString("serverfile"));
        req.installPath = localFile.absolutePath();

        req.showShortCut = false;

		emit(pendingInstallArguments(req));
    }

	void ContentInstallFlowNonDiP::onEnqueued()	// this should only be called when an entitlement is first put on the queue not when restoring a queue (after shutdown/logout)
	{
        // put on queue - never on top
		// must protect against user logging out which results in a null queueController
		Engine::Content::ContentOperationQueueController* queueController = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController();
		if (queueController)
        {
            queueController->enqueue(mContent, false);
        }
	}


    void ContentInstallFlowNonDiP::OnGlobalConnectionStateChanged(Origin::Services::Connection::ConnectionStateField field)
    {
        if (field == Origin::Services::Connection::GLOB_OPR_IS_COMPUTER_AWAKE 
            && Origin::Services::Network::GlobalConnectionStateMonitor::connectionField(field) == Origin::Services::Connection::CS_TRUE)
        {
            switch ( getFlowState() )
            {
            case ContentInstallFlowState::kTransferring:
            case ContentInstallFlowState::kPausing:
                // EBIBUGS-18553: if the computer just came back from sleep, depending on the state of the download, 
                // set the flag that makes the download immediately resume on completed pausing
                ORIGIN_LOG_EVENT << mContent->contentConfiguration()->productId() << " Download in state " << (int) getFlowState() << " when waking up, will resume automatically";
                mResumeOnPaused = true;
                break;
            default:
                // there may be other states that need to be included above, logging here to help determine which ones
                // EBIBUGS-29240: commented out the following log entry, because it was causing massive log spam, and because it's not very important.
                // ORIGIN_LOG_EVENT << mContent->contentConfiguration()->productId() << " Download in state " << (int) getFlowState() << " when waking up, ignored";
                break;
            }
        }
    }

    void ContentInstallFlowNonDiP::OnPaused()
    {
        mInstallManifest.SetBool("paused", true);
        ContentServices::SaveInstallManifest(mContent, mInstallManifest);

		emit paused();

        if (mResumeOnPaused)
        {
            mResumeOnPaused = false;
            resume();
        }
    }

    void ContentInstallFlowNonDiP::OnPostInstall()
    {
        emit finishedState();
    }

    void ContentInstallFlowNonDiP::OnPostTransfer()
    {
        mInstallManifest.Set64BitInt("savedbytes", mInstallManifest.Get64BitInt("totalbytes"));
        ContentServices::SaveInstallManifest(mContent, mInstallManifest);

        QString localFilename(mInstallManifest.GetString("serverfile"));
        QString fileExtension = QFileInfo(localFilename).suffix();

        bool isPreload = (mContent->localContent()->releaseControlState() == Origin::Engine::Content::LocalContent::RC_Preload);

        // TELEMETRY: Download has completed successfully.
        GetTelemetryInterface()->Metric_DL_SUCCESS(mContent->contentConfiguration()->productId().toLocal8Bit().constData(), isPreload);

        // Make sure the file is there after transfer.
        // Don't check packed files, unless addon or bonus content, because they would have been unpacked during transfer and don't exist anymore.
        bool checkForLocalFileExistance = !IsPackedFileExtension(fileExtension) || mContent->contentConfiguration()->isAddonOrBonusContent();
        if (checkForLocalFileExistance  && !QFile::exists(localFilename))
        {
            CREATE_DOWNLOAD_ERROR_INFO(errorInfo);
            errorInfo.mErrorType = FlowError::FileDoesNotExist;
            errorInfo.mErrorCode = -1;
            sendCriticalErrorTelemetry(errorInfo);

            emit IContentInstallFlow_error(errorInfo.mErrorType, errorInfo.mErrorCode,
                ErrorTranslator::Translate((ContentDownloadError::type)FlowError::DecryptionFailed).arg(localFilename),
                mContent->contentConfiguration()->productId());

            return;
        }

        if (mContent->contentConfiguration()->isAddonOrBonusContent())
        {
            emit finishedState();
        }
        else if (IsPackedFileExtension(fileExtension))
        {
            //Content was unpacked on the fly but need to set the packedfile install manifest param
            //so that onInstall is able to determine the correct installer file
            mInstallManifest.SetString("packedfile", localFilename);
            emit BeginInstall();
        }
        else if (IsExecutableFileExtension(fileExtension))
        {
            emit BeginInstall();
        }
        else if (IsDiskImageFileExtension(fileExtension))
        {
            emit BeginInstall();
        }
        else
        {
            emit finishedState();
        }
    }

    void ContentInstallFlowNonDiP::OnPreTransfer()
    {
        if (makeOnlineCheck("User is offline. Unable to start protocol"))
        {
            mDownloadProtocol->Start();
        }
    }

    void ContentInstallFlowNonDiP::OnProtocolError(Origin::Downloader::DownloadErrorInfo errorInfo)
    {
        QString errorDescription = ErrorTranslator::Translate((ContentDownloadError::type)errorInfo.mErrorType);
        ORIGIN_LOG_ERROR << "ContentInstallFlowNonDiP: errortype = " << errorInfo.mErrorType << ", errorDesc = " << errorDescription << ", errorcode = ," << errorInfo.mErrorCode;

        // EBIBUGS-18553: on all download errors, ensure that we will try to resume the download. Sometimes when going to 
        // sleep, network errors occur well before (2 seconds was seen) the sleep signal is received.
        mInstallManifest.SetBool("autoresume", true);

        if (!makeOnlineCheck())
        {
            // Do nothing. IContentInstallFlow::onConnectionStateChanged will pause or suspend the flow.
            return;
        }

        if (errorInfo.mErrorType == FlowError::JitUrlRequestFailed || (errorInfo.mErrorType == DownloadError::HttpError && errorInfo.mErrorCode == Services::Http403ClientErrorForbidden))
        {
            ++mDownloadRetryCount;
            if (mDownloadRetryCount < sMaxDownloadRetries)
            {
                if(errorInfo.mErrorType == FlowError::JitUrlRequestFailed)
                {
                    QTimer::singleShot(kDelayBeforeJitRetry, this, SLOT(MakeJitUrlRequest()));
                }
                else
                {
                    ContentInstallFlowState currentState = getFlowState();
                    if (currentState == ContentInstallFlowState::kInitializing || currentState == ContentInstallFlowState::kPaused)
                    {
                        ORIGIN_LOG_WARNING << "Attempting retry during initialize state.";
                        SetupDownloadProtocol();
                    }
                    else if (currentState == ContentInstallFlowState::kTransferring)
                    {
                        // Protocol emits a Paused signal after the error signal which will put
                        // the flow in the kPaused state.

                        ORIGIN_LOG_WARNING << "Attempting retry during tranfer state.";
                        mResumeOnPaused = true;
                    }
                }
                emit IContentInstallFlow_errorRetry(errorInfo.mErrorType, errorInfo.mErrorCode, errorInfo.mErrorContext, mContent->contentConfiguration()->productId());
                return;
            }
        }
        else if (errorInfo.mErrorType == DownloadError::NotConnectedError)
        {
            // Do nothing. IContentInstallFlow::onConnectionStateChanged will pause or suspend the flow.
            return;
        }
        else if (errorInfo.mErrorType == UnpackError::IOWriteFailed && errorInfo.mErrorCode == Services::PlatformService::ErrorDiskFull)
        {
            mErrorPauseQueue = true;
        }

        sendCriticalErrorTelemetry(errorInfo);
        emit IContentInstallFlow_error(errorInfo.mErrorType, errorInfo.mErrorCode, errorInfo.mErrorContext, mContent->contentConfiguration()->productId());
    }

    void ContentInstallFlowNonDiP::OnDownloadProtocolStarted()
    {
        emit DownloadProtocolStarted();
    }

    void ContentInstallFlowNonDiP::OnDownloadProtocolFinished()
    {
        emit DownloadProtocolFinished();
    }

    void ContentInstallFlowNonDiP::OnDownloadProtocolInitialized()
    {
        if (mDownloadProtocol)
        {
            // If we are in the paused state, a retriable protocol error must have occurred during
            // transfer and the protocol was reinitialized. Now we must tell the protocol to resume.
            if (getFlowState() == ContentInstallFlowState::kPaused)
            {
                emit Resuming();
                mDownloadProtocol->Resume();
            }
            else
            {
                mDownloadProtocol->GetContentLength();
            }
        }
    }

    void ContentInstallFlowNonDiP::OnDownloadProtocolResumed()
    {
        emit ResumeTransfer();
    }

    void ContentInstallFlowNonDiP::OnProtocolPaused()
    {
        // A suspend was called prior to the flow reaching the transferring state. This method gets invoked
        // after the protocol has finished cleaning up from the suspend. The flow will have to resume from
        // the beginning.
        if (mProcessingSuspend)
        {
            mProcessingSuspend = false;
            stop();
        }

        if (mDownloadProtocol)
        {
            // Delete the protocol on it's own thread (queue the destructor on the event loop)
            mDownloadProtocol->deleteLater();
            mDownloadProtocol = NULL;
        }

        emit ProtocolPaused();
    }

    void ContentInstallFlowNonDiP::OnReadyToStart()
    {
        // For content that has an unmonitored install, if the installer exists but the content is not
        // installed, jump to the ready to install state.
        if (!mContent->contentConfiguration()->monitorInstall())
        {
            if (QFile::exists(GetInstallerPath()) && !mContent->localContent()->installed(true))
            {
                emit InstallFor3pddIncomplete();
            }
        }
    }
    
    void ContentInstallFlowNonDiP::OnMounting()
    {
        QFileInfo localFile = QFileInfo(mInstallManifest.GetString("serverfile"));
        QString localFilename(mInstallManifest.GetString("serverfile"));
        QString fileExtension = QFileInfo(localFilename).suffix();
        
        // Mount if necessary, otherwise begin the installation
        //
        if (IsDiskImageFileExtension(fileExtension))
        {
            mounter.mount(localFile.absoluteFilePath());        
        }
        else
        {
            emit BeginInstall();
        }
        
    }

    void ContentInstallFlowNonDiP::OnUnmounting()
    {               
        mounter.unmount();
    }
                      
    void ContentInstallFlowNonDiP::OnMountComplete(int status, QString output)
    {
        if (status == 0)
        {
            emit BeginInstall();
        }
        else
        {
            ORIGIN_LOG_ERROR << "Failed to mount installer dmg";

            Origin::Downloader::DownloadErrorInfo errInfo;
            
            POPULATE_ERROR_INFO_TELEMETRY(errInfo, InstallError::InstallerFailed, 0, mContent->contentConfiguration()->productId().toLocal8Bit().constData());
            OnInstallerError(errInfo);
        }
    }
                      
    void ContentInstallFlowNonDiP::OnUnmountComplete(int status)
    {
        emit FinishInstall();
    }

    QString ContentInstallFlowNonDiP::GetInstallerPath() const
    {
        QString installerFile(mContent->contentConfiguration()->InstallerPath());
        if (installerFile.isEmpty())
        {
            installerFile = "setup.exe";
        }

        QString installerPath(QFileInfo(GetServerFile()).path());
        installerPath.replace("\\", "/");
        if (!installerPath.endsWith("/"))
        {
            installerPath.append("/");
        }
        installerPath.append(installerFile);

        return installerPath;
    }

    void ContentInstallFlowNonDiP::OnStopped()
    {
		if( mContent->contentConfiguration()->monitorInstall() || !mContent->localContent()->isNonMonitoredInstalling() )
			mInstallManifest.Clear("packedfile");

        setStartState(ContentInstallFlowState::kReadyToStart);
        QStateMachine::start();
    }

    void ContentInstallFlowNonDiP::OnTransferProgress(qint64 currentBytes, qint64 totalBytes, qint64 bytesPerSecond, qint64 estimateSecondsRemaining)
    {
        qint64 bytesDownloadedSinceLastSave = currentBytes - mInstallManifest.Get64BitInt("savedBytes");

        // Periodically save out the install manifest with autoresume = true, in case we get killed
        if (getFlowState() == ContentInstallFlowState::kTransferring && bytesDownloadedSinceLastSave > 5242880)
        {
            mInstallManifest.Set64BitInt("savedbytes", currentBytes);

            // Save out the autoresume flag before we save the manifest, this ensures if we crash or get killed, the download will auto-resume
            // This will get un-set during the course of normal operation.  We only do this for the transferring state.
            bool autoresume = mInstallManifest.GetBool("autoresume");
            if (!autoresume) 
                mInstallManifest.SetBool("autoresume", true);

            ContentServices::SaveInstallManifest(mContent, mInstallManifest);

            // Restore the autoresume flag to whatever it was previously, so we don't override the normal behavior
            mInstallManifest.SetBool("autoresume", autoresume);
        }

        onProgressUpdate(currentBytes, totalBytes, bytesPerSecond, estimateSecondsRemaining);
    }

    void ContentInstallFlowNonDiP::OnTransferring()
    {
        mInstallManifest.SetBool("downloading", true);
        mInstallManifest.SetBool("paused", false);

        // Save out the autoresume flag before we save the manifest, this ensures if we crash or get killed, the download will auto-resume
        // This will get un-set during the course of normal operation.  We only do this for the transferring state.
        bool autoresume = mInstallManifest.GetBool("autoresume");
        if (!autoresume) 
            mInstallManifest.SetBool("autoresume", true);

        ContentServices::SaveInstallManifest(mContent, mInstallManifest);

        // Restore the autoresume flag to whatever it was previously, so we don't override the normal behavior
        mInstallManifest.SetBool("autoresume", autoresume);
    }

    void ContentInstallFlowNonDiP::SetContentSize(qint64 numBytesInstalled, qint64 numBytesToDownload)
    {
        mInstallManifest.Set64BitInt("totalbytes", numBytesInstalled);
        mInstallManifest.Set64BitInt("totaldownloadbytes", numBytesToDownload);

        emit ReceivedContentSize(numBytesInstalled, numBytesToDownload);
    }

    bool ContentInstallFlowNonDiP::IsExecutableFileExtension(const QString& extension) const
    {
        return extension.compare("exe", Qt::CaseInsensitive) == 0 ||
            extension.compare("msi", Qt::CaseInsensitive) == 0;
    }
    
    bool ContentInstallFlowNonDiP::IsDiskImageFileExtension(const QString& extension) const
    {
        return extension.compare("dmg", Qt::CaseInsensitive) == 0 ||
        extension.compare("iso", Qt::CaseInsensitive) == 0;
    }

    bool ContentInstallFlowNonDiP::IsPackedFileExtension(const QString& extension) const
    {
        return extension.compare("viv", Qt::CaseInsensitive) == 0 ||
            extension.compare("zip", Qt::CaseInsensitive) == 0;
    }

    bool ContentInstallFlowNonDiP::validateReadyToInstallState()
    {
        QString installerPath;
        QString serverFile = GetServerFile();
        QFileInfo serverInfo(serverFile);
        // If we are dealing with a disk image file, then it is sufficient to check for its existence.
        if (IsDiskImageFileExtension(serverInfo.suffix()))
        {
            installerPath = serverFile;
        }
        else
        {
            installerPath = GetInstallerPath();
        }
            
        return QFile::exists(installerPath);
    }

    bool ContentInstallFlowNonDiP::validateInstallManifest()
    {
        ContentInstallFlowState currentState = getFlowState();
        ContentInstallFlowState previousState = getPreviousState();

        if (previousState == ContentInstallFlowState::kCompleted && !mContent->localContent()->installed())
        {
            ORIGIN_LOG_EVENT << "Repairing install manifest for " << mContent->contentConfiguration()->productId() <<
                ". Content was uninstalled.";

            QString installerPath(GetInstallerPath());
            if (QFile::exists(installerPath))
            {
                mInstallManifest.SetString("previousstate", ContentInstallFlowState(ContentInstallFlowState::kPostTransfer).toString());
                mInstallManifest.SetString("currentstate", ContentInstallFlowState(ContentInstallFlowState::kReadyToInstall).toString());
                setStartState(ContentInstallFlowState::kReadyToInstall);
                ContentServices::SaveInstallManifest(mContent, mInstallManifest);
                return true;
            }

            initializeInstallManifest();
            setStartState(ContentInstallFlowState::kReadyToStart);

            return false;
        }

        if (currentState != ContentInstallFlowState::kReadyToStart && currentState != ContentInstallFlowState::kPaused && currentState != ContentInstallFlowState::kReadyToInstall)
        {
            ORIGIN_LOG_EVENT << "Repairing install manifest for " << mContent->contentConfiguration()->productId() <<
                "Install flow was interrupted during install state " << currentState.toString() << ". Previous state was " << previousState.toString();

            switch (currentState)
            {
                case ContentInstallFlowState::kTransferring:
                    {
                        mInstallManifest.SetString("previousstate", currentState.toString());
                        mInstallManifest.SetString("currentstate", ContentInstallFlowState(ContentInstallFlowState::kPaused).toString());
                        setStartState(ContentInstallFlowState::kPaused);
                        if( currentState == ContentInstallFlowState::kTransferring )
                            mInstallManifest.SetBool("autoresume", true);
                        ContentServices::SaveInstallManifest(mContent, mInstallManifest);
                        break;
                    }
                case ContentInstallFlowState::kPreInstall:
                case ContentInstallFlowState::kInstalling:
                    {
                        if (!mContent->localContent()->installed())
                        {
                            mInstallManifest.SetString("currentstate", ContentInstallFlowState(ContentInstallFlowState::kReadyToInstall).toString());
                            setStartState(ContentInstallFlowState::kReadyToInstall);
                        }
                        else
                        {
                            mInstallManifest.SetString("currentstate", ContentInstallFlowState(ContentInstallFlowState::kReadyToStart).toString());
                            mInstallManifest.SetString("previousstate", ContentInstallFlowState(ContentInstallFlowState::kCompleted).toString());
                            setStartState(ContentInstallFlowState::kReadyToStart);
                        }
                        ContentServices::SaveInstallManifest(mContent, mInstallManifest);
                        break;
                    }
                case ContentInstallFlowState::kResuming:
                case ContentInstallFlowState::kPausing:
                    {
                        mInstallManifest.SetString("currentstate", ContentInstallFlowState(ContentInstallFlowState::kPaused).toString());
                        setStartState(ContentInstallFlowState::kPaused);
                        ContentServices::SaveInstallManifest(mContent, mInstallManifest);
                        break;
                    }
                case ContentInstallFlowState::kMounting:
                {
                    // Coming back from a mount, put the machine in a paused state
                    mInstallManifest.SetString("previousstate", currentState.toString());
                    mInstallManifest.SetString("currentstate", ContentInstallFlowState(ContentInstallFlowState::kPaused).toString());
                    setStartState(ContentInstallFlowState::kPaused);
                    if( currentState == ContentInstallFlowState::kMounting )
                        mInstallManifest.SetBool("autoresume", true);
                    ContentServices::SaveInstallManifest(mContent, mInstallManifest);                                       
                    
                    break;
                }
				case ContentInstallFlowState::kEnqueued:
				{
					mInstallManifest.SetString("currentstate", ContentInstallFlowState(ContentInstallFlowState::kPaused).toString());
					mInstallManifest.SetString("previousstate", ContentInstallFlowState(ContentInstallFlowState::kTransferring).toString());
					setStartState(ContentInstallFlowState::kPaused);
					mInstallManifest.SetBool("autoresume", true);
					ContentServices::SaveInstallManifest(mContent, mInstallManifest);
					break;
				}
                case ContentInstallFlowState::kError:
                    {
                        switch (previousState)
                        {
                            case ContentInstallFlowState::kPaused:
                            case ContentInstallFlowState::kTransferring:
                            case ContentInstallFlowState::kPostTransfer:
                                {
                                    mInstallManifest.SetString("currentstate", ContentInstallFlowState(ContentInstallFlowState::kPaused).toString());
                                    mInstallManifest.SetString("previousstate", ContentInstallFlowState(ContentInstallFlowState::kTransferring).toString());
                                    setStartState(ContentInstallFlowState::kPaused);
                                    ContentServices::SaveInstallManifest(mContent, mInstallManifest);
                                    break;
                                }

                            case ContentInstallFlowState::kReadyToInstall:
                            case ContentInstallFlowState::kInstalling:
                            case ContentInstallFlowState::kPreInstall:
                                {
                                    mInstallManifest.SetString("currentstate", ContentInstallFlowState(ContentInstallFlowState::kReadyToInstall).toString());
                                    mInstallManifest.SetString("previousstate", ContentInstallFlowState(ContentInstallFlowState::kInstalling).toString());
                                    setStartState(ContentInstallFlowState::kReadyToInstall);
                                    ContentServices::SaveInstallManifest(mContent, mInstallManifest);
                                    break;
                                }
                            default:;
                                {
                                    initializeInstallManifest();
                                    setStartState(ContentInstallFlowState::kReadyToStart);
                                    return false;
                                }
                        }
                    }
                default:
                    {
                        initializeInstallManifest();
                        setStartState(ContentInstallFlowState::kReadyToStart);
                        return false;
                    }
            }
        }

        // Make sure installer exists if content is ready to install and not already installed
        if (ContentInstallFlowState(mInstallManifest.GetString("currentstate")) == ContentInstallFlowState::kReadyToInstall)
        {
            if(!validateReadyToInstallState())
            {
                ORIGIN_LOG_EVENT << "Repairing install manifest for " << mContent->contentConfiguration()->productId() <<
                    "Install manifest indicates ready to install, but the installer does not exist";
                initializeInstallManifest();
                setStartState(ContentInstallFlowState::kReadyToStart);
                return false;
            }
        }

        if (ContentInstallFlowState(mInstallManifest.GetString("currentstate")) == ContentInstallFlowState::kPaused &&
            ContentInstallFlowState(mInstallManifest.GetString("previousstate")) == ContentInstallFlowState::kTransferring)
        {
            sendDownloadStartTelemetry("resume", mInstallManifest.Get64BitInt("savedbytes"));
        }

        return true;
    }

    void ContentInstallFlowNonDiP::install(bool continueInstall)
    {
		ASYNC_INVOKE_GUARD_ARGS(Q_ARG(bool,continueInstall))

		if(continueInstall)
	        emit BeginInstall();
		else
			emit BackToReadyToInstall();
    }

}
}



