#include "engine/content/Entitlement.h"
#include "engine/content/LocalContent.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/content/ContentOperationQueueController.h"
#include "engine/downloader/ContentInstallFlowState.h"
#include "engine/downloader/IContentInstallFlow.h"
#include "engine/downloader/ContentServices.h"
#include "services/debug/DebugService.h"
#include "services/downloader/DownloadService.h"
//#include "services/downloader/DownloaderErrors.h"
#include "services/log/LogService.h"
#include "services/settings/SettingsManager.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/common/NetUtil.h"
#include "services/heartbeat/Heartbeat.h"
#include "TelemetryAPIDLL.h"

#include <QMutexLocker>
#include <QState>

namespace Origin
{
namespace Downloader
{

quint16 IContentInstallFlow::sMaxDownloadRetries = 6;

IContentInstallFlow::IContentInstallFlow(Origin::Engine::Content::EntitlementRef content, Engine::UserRef user) :
    mContent(content),
    mUser(user),
    mIsOverrideUrl(false),
    mDownloadRetryCount(0),
    mSuppressDialogs(false),
	mPausedForUnretryableError(false),
    mErrorPauseQueue(false),
    mEnqueuedAndPaused(false)
{
    ORIGIN_VERIFY_CONNECT_EX(this, SIGNAL(stopped()), this, SLOT(onStopped()), Qt::QueuedConnection);
    initializeStates();
    initializeErrorTransitions();

    if (content)
    {
        initUrl();
    }
    
    ORIGIN_VERIFY_CONNECT_EX(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(userConnectionStateChanged(bool, Origin::Services::Session::SessionRef)), 
        this, SLOT(onConnectionStateChanged(bool)), Qt::QueuedConnection);
}

IContentInstallFlow::~IContentInstallFlow()
{
    Engine::UserRef user = mUser.toStrongRef();
    if( user )
    {
        ORIGIN_VERIFY_DISCONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(userConnectionStateChanged(bool, Origin::Services::Session::SessionRef)), 
            this, SLOT(onConnectionStateChanged(bool)));
    }
}

void IContentInstallFlow::initUrl()
{
    mUrl = "";

    //we make a place holder with the file link, because this url is not really used for downloading 
    //but rather just used in some logging.
    //downloader also checks the first few characters (http vs file) to determine if the source is only
    //the url is also parsed so that we can get the file name -- xxx.zip which the file link can still provide
    if(!mContent->contentConfiguration()->fileLink().isEmpty())
    {
        mUrl = "http:/" + mContent->contentConfiguration()->fileLink();
    }

    mIsOverrideUrl = mContent->contentConfiguration()->hasOverride();

    if(mIsOverrideUrl)
    {
        mUrl = mContent->contentConfiguration()->overrideUrl();
    }
}

void IContentInstallFlow::initializeStates()
{
    //When the state machine transitions to each state IContentInstallFlow::onStateChange will get called.
    for (unsigned i = 0; i <= static_cast<unsigned>(ContentInstallFlowState::kCompleted); ++i)
    {
        QState* state = new QState(this);
        ORIGIN_VERIFY_CONNECT(state, SIGNAL(entered()), this, SLOT(onStateChange()));
        mTypeToStateMap.insert(static_cast<ContentInstallFlowState>(i), state);
        mStateToTypeMap.insert(state, static_cast<ContentInstallFlowState>(i));
    }

    setStartState(ContentInstallFlowState::kReadyToStart);
}

void IContentInstallFlow::initializeErrorTransitions()
{
    QState* errorState = getState(ContentInstallFlowState::kError);
    if (errorState != NULL)
    {
        // Setup a transition to the error state from each state
        for (unsigned i = ContentInstallFlowState::kReadyToStart; i != ContentInstallFlowState::kCompleted; ++i)
        {
            QState* currentState = getState(static_cast<ContentInstallFlowState>(i));
            if (currentState != NULL)
            {
                ErrorTransition* errorTransition = new ErrorTransition(this);
                errorTransition->setTargetState(errorState);
                currentState->addTransition(errorTransition);
            }
        }
    }
}

void IContentInstallFlow::migrateLegacyFlowInformation()
{
    // Local content migrates when cache folder is created
    mContent->localContent()->cacheFolderPath(true, true, NULL);

    // creates install manifest directory if it doesn't exist
    QString installManifestPath;
    ContentServices::GetInstallManifestPath(mContent, installManifestPath, true);

    initializeInstallManifest();
    mInstallManifest.SetString("currentstate", ContentInstallFlowState(ContentInstallFlowState::kReadyToStart).toString());
    mInstallManifest.SetString("previousstate", ContentInstallFlowState(ContentInstallFlowState::kCompleted).toString());
    ContentServices::SaveInstallManifest(mContent, mInstallManifest);
}

void IContentInstallFlow::initializeFlow()
{
    // Transitions in the public state machine are visible to other layers through the stateChanged signal
    initializePublicStateMachine();

    QString installManifestPath;
    ContentServices::GetInstallManifestPath(mContent, installManifestPath, false);

    // If content is installed but has no install manifest, we have a migration step for CRCs and CorePreferences here
    if(!QFile(installManifestPath).exists() && mContent->localContent()->installed())
    {
        migrateLegacyFlowInformation();
    }

    // Attempt to load the content install manifest file.
    if (loadInstallManifest())
    {
        //An install manifest file was found. The content is installed or in the process of
        //downloading or installing.

        //Addon and bonus content do not have install check data in their xml. Once the install flow
        //determines what should be used, the check gets stored in the install manifest. Load the
        //install check from the manifest and inform the LocalContent class of the value so that
        //it can return the correct result for the installed() method.
        mContent->localContent()->setAddonInstallCheck(mInstallManifest.GetString("addoninstallcheck"));

        //Make sure the install manifest isn't in an invalid state which can happen during
        //crashes or shutting Origin down in the middle of the install flow.
        validateInstallManifest();

		// for download queue
		if (getFlowState() == ContentInstallFlowState::kPaused)
		{
            // will place on queue in proper queue order if it is found on queue, otherwise at the end
			// must protect against user logging out which results in a null queueController
			Engine::Content::ContentOperationQueueController* queueController = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController();
			if (queueController)
            {
                queueController->enqueue(mContent, false);
            }
		}
    }

    // There was no content install manifest file. The content is not installed or in the process
    // of downloading/installing so initialize an install manifest in memory. The install manifest
    // will get saved to disk if the user starts the install flow.
    else
    {
        initializeInstallManifest();
    }
}

void IContentInstallFlow::startFlow()
{
    Downloader::ContentInstallFlowState currentFlowState = getFlowState();
    onProgressUpdate(getDownloadedByteCount(), getContentTotalByteCount(), 0, 0);

    QStateMachine::start();
}

bool IContentInstallFlow::escalationServiceEnabled()
{
    QString uacReasonCode = "IContentInstallFlow escalationServiceEnabled (" + mContent->contentConfiguration()->productId() + ")";

    // If we can't get escalation for some reason, we bail out of the entire process
    int errCode = 0;
    QSharedPointer<Origin::Escalation::IEscalationClient> escalationClient;
    if (!Origin::Escalation::IEscalationClient::quickEscalate(uacReasonCode, errCode, escalationClient))
    {
        // Special handling for this "new" special case: the UAC request popup is showing, but the user hasn't accepted/denied the request yet
        if (errCode == Escalation::kCommandErrorUACRequestAlreadyPending)
        {
            QString errDescription = ErrorTranslator::Translate((ContentDownloadError::type)Downloader::InstallError::Escalation);
            emit IContentInstallFlow_error(Downloader::InstallError::Escalation, errCode, errDescription, mContent->contentConfiguration()->productId());

            return false;
        }
    }

    return true;
}

bool IContentInstallFlow::loadInstallManifest()
{
    // Attempt to load the content install manifest file.
    if (ContentServices::LoadInstallManifest(mContent, mInstallManifest))
    {
        // An install manifest file was found. The content is either installed or in the
        // process of downloading/installing. Set at which state the install flow should begin.
        setStartState(ContentInstallFlowState(mInstallManifest.GetString("currentstate")));

        // Set the locale that the user previously selected. It will be an empty string
        // if it had not been selected yet.
        QString installLocale(mInstallManifest.GetString("locale"));
        mContent->localContent()->setInstalledLocale(installLocale);
    }
    else
    {
        // There was no content install manifest file. The content is not installed or in the process
        // of downloading/installing so initialize an install manifest in memory. The install manifest
        // will get saved to disk if the user starts the install flow.
        ContentServices::InitializeInstallManifest(mContent, mInstallManifest);
        return false;
    }

    return true;
}

QState* IContentInstallFlow::getState(ContentInstallFlowState type)
{
    QState* state = NULL;
    StateMap::const_iterator i = mTypeToStateMap.find(type);

    if (i != mTypeToStateMap.constEnd())
    {
        state = i.value();
    }

    return state;
}

ContentInstallFlowState IContentInstallFlow::getStateType(QState* state)
{
    ContentInstallFlowState type = ContentInstallFlowState::kInvalid;
    ReverseStateMap::const_iterator i = mStateToTypeMap.find(state);
    if (i != mStateToTypeMap.constEnd())
    {
        type = i.value();
    }

    return type;
}

void IContentInstallFlow::setStartState(ContentInstallFlowState type)
{
    StateMap::const_iterator i = mTypeToStateMap.find(type);
    if (i != mTypeToStateMap.constEnd())
    {
        setInitialState(i.value());
    }
    else
    {
        ORIGIN_ASSERT_MESSAGE("IContentInstallFlow::setStartState: unknown start state -> this could cause a crash!!!", 0);
    }
}

void IContentInstallFlow::generateNewJobId()
{
    // Create a new Job ID
    mInstallManifest.SetString("jobID", QUuid::createUuid().toString());
}

void IContentInstallFlow::clearJobId()
{
    // Clear the Job ID
    mInstallManifest.SetString("jobID", "");
}

QString IContentInstallFlow::currentDownloadJobId() const
{
    return mInstallManifest.GetString("jobID");
}

void IContentInstallFlow::onStateChange()
{
    QState* qstate = dynamic_cast<QState*>(QObject::sender());
    if (qstate != NULL)
    {
        // Find the state type we are about to transition to. Log an event. Set the state
        // in the install manifest. Emit a stateChanged signal to whomever is listening.
        ReverseStateMap::const_iterator i = mStateToTypeMap.find(qstate);
        if (i != mStateToTypeMap.constEnd())
        {
            ContentInstallFlowState nextState = i.value();
            ContentInstallFlowState currentState = getFlowState();
            if (nextState == currentState)
            {
                ORIGIN_LOG_DEBUG << "Transition skipped as nextstate == currentState: " << currentState.toString();
            }
            else
            {
                if (currentState != ContentInstallFlowState::kInvalid)
                {
                    ORIGIN_LOG_EVENT << "Transitioning from install state " << currentState.toString() << " to install state " << nextState.toString() <<
                        ". Content = " <<  mContent->contentConfiguration()->productId();
                }

                storeFlowAction();
                setFlowState(nextState);
                // onProgressUpdate will work fine for this when Transferring. It's when we are in our weird states when we really need to
                // worry about context progress

				//check for current state != Invalid so we stop the flurry of progress updates at the beginning
                if((currentState != ContentInstallFlowState::kInvalid) && (nextState != ContentInstallFlowState::kTransferring))
                {
                    updateProgress(true);
                }
                emit stateChanged(nextState);
            }
        }
    }
}

void IContentInstallFlow::onProgressUpdate(qint64 bytesProgressed, qint64 totalBytes, qint64 bytesPerSecond, qint64 estimatedSecondsRemaining)
{
    QMutexLocker locker(&mInstallProgressLock);
    ContentInstallFlowState currentState(mInstallManifest.GetString("currentstate"));

    mCurrentProgress.mBytesDownloaded = bytesProgressed;
    mCurrentProgress.mTotalBytes = totalBytes;
    mCurrentProgress.mBytesPerSecond = bytesPerSecond;
    mCurrentProgress.mEstimatedSecondsRemaining = estimatedSecondsRemaining;
    updateProgress();

    // For dynamic downloads, grab the required portion size
    // This number shouldn't change after it's been decided.
    if (isDynamicDownload() && currentState == ContentInstallFlowState::kTransferring && mCurrentProgress.mDynamicDownloadRequiredPortionSize == 0)
    {
        mCurrentProgress.mDynamicDownloadRequiredPortionSize = mInstallManifest.Get64BitInt("DDRequiredPortion");
    }

    mDownloadRetryCount = 0;

    GetTelemetryInterface()->UpdateDownloadSession(mContent->contentConfiguration()->productId().toLocal8Bit().constData(), bytesProgressed, bytesPerSecond / 128.0); // convert to Kbps

    emit(progressUpdate(currentState, mCurrentProgress));
}

void IContentInstallFlow::updateProgress(const bool& forceUpdate)
{
    ContentInstallFlowState state(mInstallManifest.GetString("currentstate"));
    float progress = 0.0f;
    const ContentOperationType opt_type = operationType();

    if(opt_type == kOperation_Install)
    {
        mCurrentProgress.mTotalBytes = 0;
        mCurrentProgress.mBytesDownloaded = 0;
    }

    if(state != ContentInstallFlowState::kTransferring || opt_type == kOperation_ITO)
    {
        mCurrentProgress.mBytesPerSecond = 0;
    }

    switch(state)
    {
    case ContentInstallFlowState::kPostTransfer:
        if(isDynamicDownload())
        {
            progress = (mCurrentProgress.mTotalBytes > 0) ? static_cast<float>(mCurrentProgress.mBytesDownloaded) / mCurrentProgress.mTotalBytes : mCurrentProgress.mProgress;
        }
        else
        {
            progress = 1.0f;
        }
        break;
    case ContentInstallFlowState::kPaused:
    case ContentInstallFlowState::kPausing:
        mCurrentProgress.mEstimatedSecondsRemaining = 0;
        progress = (mCurrentProgress.mTotalBytes > 0) ? static_cast<float>(mCurrentProgress.mBytesDownloaded) / mCurrentProgress.mTotalBytes : mCurrentProgress.mProgress;
        break;
    case ContentInstallFlowState::kUnpacking:
    case ContentInstallFlowState::kDecrypting:
    case ContentInstallFlowState::kCanceling:
    case ContentInstallFlowState::kEnqueued:
    case ContentInstallFlowState::kReadyToInstall:
        progress = 0.0f;
        break;
    case ContentInstallFlowState::kInitializing:
    case ContentInstallFlowState::kPreTransfer:
    case ContentInstallFlowState::kPendingInstallInfo:
    case ContentInstallFlowState::kPendingEulaLangSelection:
    case ContentInstallFlowState::kPendingEula:
        if(opt_type != kOperation_Repair && opt_type != kOperation_Update)
        {
            // Only in repair, update, and resume have progress during PREPARING
            progress = 0.0f;
        }
        else
        {
            progress = (mCurrentProgress.mTotalBytes > 0) ? static_cast<float>(mCurrentProgress.mBytesDownloaded) / mCurrentProgress.mTotalBytes : mCurrentProgress.mProgress;
        }
        break;
    case ContentInstallFlowState::kInstalling:
        if(isDynamicDownload())
            progress = (mCurrentProgress.mTotalBytes > 0) ? static_cast<float>(mCurrentProgress.mDynamicDownloadRequiredPortionSize) / mCurrentProgress.mTotalBytes : mCurrentProgress.mProgress;
        else if(!isInstallProgressConnected())
            progress = 0.0f;
        else
            progress = (mCurrentProgress.mTotalBytes > 0) ? static_cast<float>(mCurrentProgress.mBytesDownloaded) / mCurrentProgress.mTotalBytes : mCurrentProgress.mProgress;
        break;
    default:
        progress = (mCurrentProgress.mTotalBytes > 0) ? static_cast<float>(mCurrentProgress.mBytesDownloaded) / mCurrentProgress.mTotalBytes : mCurrentProgress.mProgress;
        break;
    }

    mCurrentProgress.mProgress = progress;
    if(forceUpdate)
    {
        emit(progressUpdate(state, mCurrentProgress));
    }
}

void IContentInstallFlow::setFlowState(ContentInstallFlowState type)
{
    // In the install manifest, save the current state as the previous state and the new state
    // as the current state.

    QMutexLocker lock(&mMutex);
    ContentInstallFlowState currentState(mInstallManifest.GetString("currentstate"));
    if (currentState != type)
    {
        // We do not want these states to ever be saved as the previous state. They are transitional states that
        // exist only to let the UI know we are in the middle of a transition that might take some time.
        if (currentState != ContentInstallFlowState::kPausing && currentState != ContentInstallFlowState::kResuming &&
            currentState != ContentInstallFlowState::kCanceling && currentState != ContentInstallFlowState::kError)
        {
            mInstallManifest.SetString("previousstate", currentState.toString());
        }
        mInstallManifest.SetString("currentstate", type.toString());

        if (currentState != ContentInstallFlowState::kInvalid)
        {
            ContentServices::SaveInstallManifest(mContent, mInstallManifest);
        }
    }
}

ContentInstallFlowState IContentInstallFlow::getPreviousState()
{
    QMutexLocker lock(&mMutex);

    return ContentInstallFlowState(mInstallManifest.GetString("previousstate"));
}

const Engine::Content::EntitlementRef IContentInstallFlow::getContent() const
{
    return mContent;
}

ContentInstallFlowState IContentInstallFlow::getFlowState() const
{
    QMutexLocker lock(&mMutex);

    return ContentInstallFlowState(mInstallManifest.GetString("currentstate"));
}

bool IContentInstallFlow::isAutoResumeSet() const
{
    return mInstallManifest.GetBool("autoresume");
}

bool IContentInstallFlow::isAutoStartSet() const
{
    return mInstallManifest.GetBool("autostart");
}

void IContentInstallFlow::setAutoResumeFlag(bool autoResume)
{
    mInstallManifest.SetBool("autoresume", autoResume);
}

void IContentInstallFlow::setAutoStartFlag(bool autoStart)
{
    mInstallManifest.SetBool("autostart", autoStart);
}

bool IContentInstallFlow::suppressDialogs () const
{
    return mSuppressDialogs;
}

void IContentInstallFlow::setSuppressDialogs(bool suppress)
{
    mSuppressDialogs = suppress;
}

bool IContentInstallFlow::isUsingLocalSource() const
{
    return false;
}

bool IContentInstallFlow::isITO () const
{
    return false;
}

bool IContentInstallFlow::isDynamicDownload(bool *contentPlayableStatus) const
{
    Q_UNUSED(contentPlayableStatus);
    return false;
}

bool IContentInstallFlow::isInstallProgressConnected() const
{
    return false;
}

void IContentInstallFlow::requestCancel(bool isIGO)
{
    // If we are in the initial set of the flow, don't present the cancel prompt - just cancel.
    const ContentInstallFlowState currentState = getFlowState();
    bool showCancel = true;
    switch(currentState)
    {
    case ContentInstallFlowState::kInitializing:
        if(isRepairing() == false)
        {
            cancel();
            showCancel = false;
        }
        break;
    case ContentInstallFlowState::kPendingInstallInfo:
    case ContentInstallFlowState::kPendingEulaLangSelection:
    case ContentInstallFlowState::kPendingEula:
        {
            cancel();
            showCancel = false;
        }
        break;
    case ContentInstallFlowState::kEnqueued:
    case ContentInstallFlowState::kPausing:
    case ContentInstallFlowState::kPaused:
        if(mCurrentProgress.mProgress == 0.0f)
        {
            cancel();
            showCancel = false;
        }
        break;
    }

    // Initiate an "are you sure you want to cancel" dialog in the UI.
    if(showCancel)
    {
        CancelDialogRequest req;
        req.contentDisplayName = mContent->contentConfiguration()->displayName();
        req.productId = mContent->contentConfiguration()->productId();
        req.state = operationType();
        req.isIGO = isIGO;
        emit pendingCancelDialog(req);
    }
}

void IContentInstallFlow::storeFlowAction()
{
    const ContentInstallFlowState currentState = getFlowState();
    bool valid = false;

    switch (currentState.getValue())
    {
        case ContentInstallFlowState::kPaused:
        case ContentInstallFlowState::kPausing:
        case ContentInstallFlowState::kCanceling:
        case ContentInstallFlowState::kInitializing:
        case ContentInstallFlowState::kResuming:
        case ContentInstallFlowState::kPreTransfer:
        case ContentInstallFlowState::kPendingInstallInfo:
        case ContentInstallFlowState::kPendingEulaLangSelection:
        case ContentInstallFlowState::kPendingEula:
        case ContentInstallFlowState::kTransferring:
        case ContentInstallFlowState::kPendingDiscChange:
        case ContentInstallFlowState::kReadyToInstall:
        case ContentInstallFlowState::kPreInstall:
        case ContentInstallFlowState::kInstalling:
        case ContentInstallFlowState::kPostInstall:
            valid = true;
        break;
        default:
            break;
    }

    if(valid)
    {
        mLastFlowAction.flowState = currentState;
        mLastFlowAction.state = operationType();
        mLastFlowAction.totalBytes = mCurrentProgress.mTotalBytes;
        mLastFlowAction.isDynamicDownload = isDynamicDownload();
    }
}

void IContentInstallFlow::install(bool /*continueInstall*/)
{
    // do nothing
}

ContentOperationType IContentInstallFlow::operationType()
{
    const Downloader::ContentInstallFlowState::value state = getFlowState().getValue();
    // Is this a download-ish operation?
    switch(state)
    {
    case ContentInstallFlowState::kPaused:
    case ContentInstallFlowState::kPausing:
    case ContentInstallFlowState::kCanceling:
    case ContentInstallFlowState::kInitializing:
    case ContentInstallFlowState::kResuming:
    case ContentInstallFlowState::kEnqueued:
    case ContentInstallFlowState::kPreTransfer:
    case ContentInstallFlowState::kPendingInstallInfo:
    case ContentInstallFlowState::kPendingEulaLangSelection:
    case ContentInstallFlowState::kPendingEula:
    case ContentInstallFlowState::kTransferring:
    case ContentInstallFlowState::kPendingDiscChange:
    case ContentInstallFlowState::kPostTransfer:
        {
            if(isRepairing())
            {
                return kOperation_Repair;
            }

            if(isUpdate())
            {
                return kOperation_Update;
            }

            if(isUsingLocalSource())
            {
                return kOperation_ITO;
            }

            if(mContent->localContent()->releaseControlState() == Origin::Engine::Content::LocalContent::RC_Preload)
            {
                return kOperation_Preload;
            }

            return kOperation_Download;
        }
        break;

    case ContentInstallFlowState::kPreInstall:
    case ContentInstallFlowState::kInstalling:
    case ContentInstallFlowState::kPostInstall:
        if(isUsingLocalSource())
        {
            return kOperation_ITO;
        }
        else if(mContent->localContent()->releaseControlState() == Origin::Engine::Content::LocalContent::RC_Preload)
        {
            return kOperation_Preload;
        }
        else
        {
            return kOperation_Install;
        }
        break;

    case ContentInstallFlowState::kUnpacking:
        if(isUpdate() == false)
        {
            return kOperation_Unpack;
        }
        break;

    default:
        break;
    }
    return kOperation_None;
}

void IContentInstallFlow::onConnectionStateChanged(bool onlineAuth)
{
    if(!onlineAuth)
    {
        ContentInstallFlowState currentState = getFlowState();

        // Pause all install flows that are in the transferring state and
        // downloading over a network. Flows that have not reached the transferring
        // state will be canceled and have autostart set to false.
        switch (currentState)
        {
            case ContentInstallFlowState::kResuming:
            case ContentInstallFlowState::kTransferring:
            {
                if(DownloadService::IsNetworkDownload(mUrl))
                {
                    // TELEMETRY: Flow is being paused due to user going offline.
                    GetTelemetryInterface()->Metric_DL_PAUSE(mContent->contentConfiguration()->productId().toLocal8Bit().constData(), "user_offline");
                    pause(true);
                }
                break;
            }
            case ContentInstallFlowState::kInitializing:
            case ContentInstallFlowState::kPreTransfer:
            case ContentInstallFlowState::kPendingInstallInfo:
            case ContentInstallFlowState::kPendingEulaLangSelection:
            case ContentInstallFlowState::kPendingEula:
            {
                if(DownloadService::IsNetworkDownload(mUrl))
                {
                    suspend(false);
                }
                break;
            }
            default:;
        }
    }
    // else The content controller will initiate restart of the flows in ContentController::processAutoDownloads()
    // which gets called when My Games refreshes.
}

void IContentInstallFlow::onStopped()
{
    mCurrentProgress.mBytesDownloaded = 0;
    mCurrentProgress.mTotalBytes = 0;
    mCurrentProgress.mBytesPerSecond = 0;
    mCurrentProgress.mEstimatedSecondsRemaining = 0;
    mCurrentProgress.mProgress = 0.0f;
    mCurrentProgress.mDynamicDownloadRequiredPortionSize = 0;

    // for ITO from a local source we do not want to removeFromQueue called as it will screw up the queue for any 
    // DLC when the disc install ends and the normal downloaded update begins.
    // However, Free To Play uses ITO but downloads it so we must make sure when a download install ends during ITO we remove it from the queue (fixes EBIBUGS-28259)
    // Also, since testers can use a local override to test non-ITO builds, we want to make sure removeFromQueue gets called when that completes.
    // Finally, currently (10/10/14) ITO from disc always sets the install version to 0 so that it will check for a download update.  This assumption
    // is important here as we don't remove the entitlement from the queue because we expect a download on the queue directly following the disc install.
    // But if the disc version is the live current version and download update job isn't created, then the item on the queue will get orphaned and the queue will get stuck.
    if (!isUsingLocalSource() || !isITO())
    {
        removeFromQueue(true);
    }
}

void IContentInstallFlow::sendCriticalErrorTelemetry(Origin::Downloader::DownloadErrorInfo errorInfo)
{
    // If download is from web URL, report the vendor's IP address
    if (QUrl(mUrl).scheme().startsWith("http", Qt::CaseInsensitive))
    {
        QString vendorIp(Util::NetUtil::ipFromUrl(mUrl));

        GetTelemetryInterface()->Metric_DL_ERROR_VENDOR_IP(mContent->contentConfiguration()->productId().toLocal8Bit().constData(),
                                            vendorIp.toLocal8Bit().constData(),
                                            errorInfo.mErrorType,
                                            errorInfo.mErrorCode);

        ORIGIN_LOG_ERROR << "[<OERJIT>][" << mContent->contentConfiguration()->productId() << "] [" << errorInfo.mErrorType << ":" << 
            errorInfo.mErrorCode << "] vendor IP: " << vendorIp << ", URL: " << mUrl;
    }

    // Figure out what CDN the error occured on
    QString cdnVendor = "unknown";
    if(mUrl.contains("akamai"))
        cdnVendor = "akamai";
    else if(mUrl.contains("lvlt"))
        cdnVendor = "lvlt";

    // HEARTBEAT: Send the download error via heartbeat
    Origin::Services::Heartbeat::instance()->setDownloadError(errorInfo.mErrorType, cdnVendor);

    bool isPreload = (mContent->localContent()->releaseControlState() == Origin::Engine::Content::LocalContent::RC_Preload);

    // TELEMETRY: The downloader has encountered a critical error.
    // This slot can either be called directly from the flow level, or triggered by signals from the protocol or services.
    GetTelemetryInterface()->Metric_DL_ERROR(mContent->contentConfiguration()->productId().toLocal8Bit().constData(),
                                            Origin::Downloader::ErrorTranslator::ErrorString((Origin::Downloader::ContentDownloadError::type) errorInfo.mErrorType).toUtf8().data(),
                                            errorInfo.mErrorContext.toUtf8().data(),
                                            errorInfo.mErrorType,
                                            errorInfo.mErrorCode,
                                            errorInfo.mSourceFile.toLocal8Bit().constData(),
                                            errorInfo.mSourceLine,
                                            isPreload);
}

qint64 IContentInstallFlow::getDownloadedByteCount()
{
    qint64 savedBytes = mInstallManifest.Get64BitInt("savedbytes");
    return savedBytes;
}

qint64 IContentInstallFlow::getContentTotalByteCount()
{
    qint64 totalBytes = mInstallManifest.Get64BitInt("totalbytes");
    return totalBytes;
}

QString IContentInstallFlow::getLogSafeUrl() const
{
    QString url = mUrl.toLower();

    if (url.startsWith("file"))
    {
        // File override, so we can display as is.
        url = mUrl;
    }
    else
    {
        int delimiter = mUrl.lastIndexOf('?');
        if (delimiter > 0)
        {
            // Strip off the token
            url = mUrl.left(delimiter);
        }
    }

    return url;
}

QString IContentInstallFlow::getContentTypeString() const
{
    if(mContent->contentConfiguration()->isPDLC())
    {
        return "PDLC";
    }
    else
    {
        return "Game";
    }
}

QString IContentInstallFlow::getPackageTypeString() const
{
    using namespace Origin::Services;
    QString packageType;

    switch(mContent->contentConfiguration()->packageType())
    {
    case Publishing::PackageTypeDownloadInPlace:
        {
            // We have to be slightly more granular with DiPs since there are
            // so many different flows a user can go through to install a DiP.
            if(isUsingLocalSource())
                packageType = "DiPMedia";
            else if(isRepairing())
                packageType = "DiPVerify";
            else if(isUpdate())
                packageType = "DiPUpdate";
            else
                packageType = "DiP";
            break;
        }

    case Publishing::PackageTypeUnpacked:
        // There no longer is a "Viv" package type, so "Unpacked" is the only option for this case.  This is only used by telemetry.
        packageType = "Unpacked";
        break;

    case Publishing::PackageTypeSingle:
        packageType = "Single";
        break;

    case Publishing::PackageTypeOriginPlugin:
        packageType = "OriginPlugin";
        break;

    case Publishing::PackageTypeExternalUrl:
        packageType = "ExternalUrl";
        break;

    case Publishing::PackageTypeUnknown:
    default:
        packageType = "Unknown";
        break;
    }

    return packageType;
}

bool IContentInstallFlow::isPathSymlink(const QString &path) const
{
    QString tempPath(path);
    tempPath.replace("\\","/");
    int index = -1;
    do
    {
        //	Detect symlink
        if (QFileInfo(tempPath).isSymLink())
            return true;

        //	Remove last path element
        index = tempPath.lastIndexOf("/");
        if (index > 0)  
        {
            tempPath = tempPath.left(index);
        }
    }	
    while (index > 0);
    return false;
}


void IContentInstallFlow::removeFromQueue(const bool& removeChildren)
{
    // must protect against user logging out which results in a null queueController
    Engine::Content::ContentOperationQueueController* queueController = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController();
    if (queueController)
    {
        if(removeChildren)
        {
            // remove and cancel any children on the queue
            Engine::Content::EntitlementRef child;

            // we must remove all the children first
            do
            {
                child = queueController->first_child_of(mContent);
                if (!child.isNull())
                {
                    queueController->removeFromQueue(child, false);
                    child->localContent()->installFlow()->cancel();
                }
            } while (!child.isNull());
        }

        queueController->removeFromQueue(mContent, true);
    }
}


bool IContentInstallFlow::makeOnlineCheck(const QString& warningMsgIfOnline /*= ""*/) const
{
    bool online = true;

    Origin::Engine::UserRef user = mUser.toStrongRef();
    if (user.isNull())
    {
        return false;
    }
    else
    {
        Origin::Services::Session::SessionRef userSession = user->getSession();
        if (userSession.isNull() || !Origin::Services::Session::SessionService::isValidSession (userSession))
        {
            return false;
        }
        else
        {
            online = Origin::Services::Connection::ConnectionStatesService::isUserOnline (userSession);
        }
    }

    if (!online && DownloadService::IsNetworkDownload(mUrl))
    {
        if (!warningMsgIfOnline.isEmpty())
        {
            ORIGIN_LOG_WARNING << warningMsgIfOnline << ": " <<  (mContent ? mContent->contentConfiguration()->productId() : "");
        }

        return false;
    }

    return true;
}

LastActionInfo IContentInstallFlow::lastFlowAction() const
{
    return mLastFlowAction;
}

InstallProgressState IContentInstallFlow::progressDetail()
{
    QMutexLocker locker(&mInstallProgressLock);
    return mCurrentProgress;
}

void IContentInstallFlow::fakeDownloadErrorRetry(qint32 errorType, qint32 errorCode, QString errorContext, QString key)
{
    ++mDownloadRetryCount;
    if (mDownloadRetryCount < sMaxDownloadRetries)
    {
        emit IContentInstallFlow_errorRetry(errorType, errorCode, errorContext, key);
    }
}

ErrorTransition::ErrorTransition(IContentInstallFlow* flow) : QSignalTransition(flow, SIGNAL(IContentInstallFlow_error(qint32, qint32, QString, QString)))
{

}

void ErrorTransition::onTransition(QEvent* event)
{
    // An error transtion occurs from any state when the install flow emits an error signal.
    // This method calls the flow's handleError method when that happens.

    QStateMachine::SignalEvent* signalEvent = static_cast<QStateMachine::SignalEvent*>(event);
    if (signalEvent != NULL)
    {
        IContentInstallFlow* flow = dynamic_cast<IContentInstallFlow*>(senderObject());
        if (flow != NULL)
        {
            int type = signalEvent->arguments().at(0).toInt();
            int code = signalEvent->arguments().at(1).toInt();
            QString description(signalEvent->arguments().at(2).toString());

            ORIGIN_LOG_ERROR << "ContentInstallFlow transtioning to error state: type = " << type << ", code = " <<
                code << ", description = " << description;

            flow->handleError(type, code, description);
        }
    }
}

} // namespace Downloader
} // namespace Origin

