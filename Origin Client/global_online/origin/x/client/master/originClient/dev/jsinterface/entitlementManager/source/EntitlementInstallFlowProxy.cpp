#include "EntitlementInstallFlowProxy.h"

#include "engine/content/Entitlement.h"
#include "engine/content/LocalContent.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/content/ContentOperationQueueController.h"

#include "engine/downloader/IContentInstallFlow.h"
#include "engine/downloader/ContentInstallFlowState.h"
#include "engine/login/LoginController.h"
#include "TelemetryAPIDLL.h"
#include "utilities/LocalizedContentString.h"
#include "LocalizedDateFormatter.h"
#include "LocalContentViewController.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

EntitlementInstallFlowProxy::EntitlementInstallFlowProxy(QObject *parent, Engine::Content::EntitlementRef entitlement)
: EntitlementServerOperationProxy(parent)
, mEntitlement(entitlement)
, mEntitlementInstallFlow(entitlement->localContent()->installFlow())
, mLocalizedContentStrings(new LocalizedContentString(QLocale(), this))
{
}

void EntitlementInstallFlowProxy::pause()
{
    // TELEMETRY: Flow is being paused due to user action.
    GetTelemetryInterface()->Metric_DL_PAUSE(mEntitlement->contentConfiguration()->productId().toLocal8Bit().constData(), "user_action");
    if (mEntitlementInstallFlow)
    mEntitlementInstallFlow->pause(false);
}

void EntitlementInstallFlowProxy::resume()
{
    // TELEMETRY: Flow is being paused due to user action.
    GetTelemetryInterface()->Metric_DL_PAUSE(mEntitlement->contentConfiguration()->productId().toLocal8Bit().constData(), "user_resume");
    if (mEntitlementInstallFlow)
        mEntitlementInstallFlow->startTransferFromQueue(true);  // set as manually started/resumed by user (so we can clear pause-due-to-unretryable-errors flag)
}

void EntitlementInstallFlowProxy::cancel()
{
    if (mEntitlementInstallFlow)
	mEntitlementInstallFlow->requestCancel();
}

QString EntitlementInstallFlowProxy::specificPhaseDisplay(const QString& phase)
{
    return mEntitlementInstallFlow ? mLocalizedContentStrings->operationPhaseDiplay(mLocalizedContentStrings->operationTypeCode(mEntitlementInstallFlow->operationType()), phase) : QString();
}

bool EntitlementInstallFlowProxy::shouldLightFlag()
{
    using namespace Engine::Content;
    const LocalContent::PlayableStatus playableStatus = mEntitlement->localContent()->playableStatus(false);

    // If the game is playable or are playing
    // Resuming for when we are in between playable and playing.
    return (playableStatus == LocalContent::PS_Playable || playableStatus == LocalContent::PS_Playing || (mEntitlementInstallFlow && mEntitlementInstallFlow->getFlowState() == Downloader::ContentInstallFlowState::kResuming))
        // and the game is installed.
        && mEntitlement->localContent()->installed(true);
}

QVariant EntitlementInstallFlowProxy::progress()
{
    const float progress = mEntitlementInstallFlow ? mEntitlementInstallFlow->progressDetail().mProgress : -1.0f;
	return progress == -1.0f ? QVariant() : progress;
}

QString EntitlementInstallFlowProxy::progressState()
{
    return mEntitlementInstallFlow ? mEntitlementInstallFlow->progressDetail().progressStateCode(LocalContentViewController::possibleMaskInstallState(mEntitlement).getValue()) : QString();
}

QString EntitlementInstallFlowProxy::type()
{
    return mEntitlementInstallFlow ? mLocalizedContentStrings->operationTypeCode(mEntitlementInstallFlow->operationType()) : QString();
}

QString EntitlementInstallFlowProxy::typeDisplay()
{
    return mEntitlementInstallFlow ? mLocalizedContentStrings->operationDisplay(mEntitlementInstallFlow->operationType()) : QString();
}

QString EntitlementInstallFlowProxy::phase()
{
    return mLocalizedContentStrings->flowStateCode(LocalContentViewController::possibleMaskInstallState(mEntitlement).getValue());
}

QString EntitlementInstallFlowProxy::phaseDisplay()
{
    QString debugRet = "";
    Engine::Content::ContentOperationQueueController* queueController = Engine::LoginController::currentUser()->contentOperationQueueControllerInstance();
    if (mLocalizedContentStrings && mEntitlementInstallFlow && mEntitlement && queueController)
    {
        Downloader::ContentInstallFlowState flowState = LocalContentViewController::possibleMaskInstallState(mEntitlement).getValue();
        if (flowState == Downloader::ContentInstallFlowState::kPaused && queueController->indexInQueue(mEntitlement) > 0)
        {
            flowState = Downloader::ContentInstallFlowState::kEnqueued;
        }

        debugRet = mLocalizedContentStrings->operationPhaseDiplay(mEntitlementInstallFlow->operationType(), flowState.getValue());
    }
    return debugRet;
}

QVariant EntitlementInstallFlowProxy::bytesDownloaded()
{
    return mEntitlementInstallFlow ? mEntitlementInstallFlow->progressDetail().mBytesDownloaded : 0;
}

QVariant EntitlementInstallFlowProxy::bytesPerSecond()
{
    return mEntitlementInstallFlow ? mEntitlementInstallFlow->progressDetail().mBytesPerSecond : 0;
}

QVariant EntitlementInstallFlowProxy::totalFileSize()
{
    return mEntitlementInstallFlow ? mEntitlementInstallFlow->progressDetail().mTotalBytes : 0;
}

QVariant EntitlementInstallFlowProxy::secondsRemaining()
{
    //if the seconds remaining is -1, do not show anything yet, this means that the value is not yet valid
    const quint64 seconds = mEntitlementInstallFlow ? mEntitlementInstallFlow->progressDetail().mEstimatedSecondsRemaining : 0;
    return seconds == 0xffffffffffffffffULL ? 0 : seconds;
}

bool EntitlementInstallFlowProxy::cancellable()
{
    // Do not use queueController->isHeadBusy(). With Repair - you can cancel, but you can't pause or resume.
    if(!mEntitlementInstallFlow  || (mEntitlementInstallFlow->isDynamicDownload() && mEntitlement->localContent()->playing()))
        return false;
    return mEntitlementInstallFlow->canCancel();
}

bool EntitlementInstallFlowProxy::pausable()
{
    Engine::Content::ContentOperationQueueController* queueController = Engine::LoginController::currentUser()->contentOperationQueueControllerInstance();
    if(queueController->isHeadBusy())
        return false;
    return mEntitlementInstallFlow ? mEntitlementInstallFlow->canPause() : false;
}

bool EntitlementInstallFlowProxy::resumable()
{
    Engine::Content::ContentOperationQueueController* queueController = Engine::LoginController::currentUser()->contentOperationQueueControllerInstance();
    bool isHeadAndEnqueuedAndPaused = false;
    
    // handle case where an enqueued title is brought to the head when the previous head was paused and then cancelled
    // so the head item in the queue will be in the state kEnqueued but needs to be treated as Paused. (EBIBUGS-27115)
    // however, because a new download on an empty queue will also be at the head and kEnqueued, we need to check for isEnqueuedAndPaused() (which is
    // only set for when an enqueued title FALLS into the head and the queue was paused) to make sure we don't let the resume button be live while the
    // title is starting up the download (EBIBUGS-27556).
    if (queueController && mEntitlementInstallFlow)
    {
        int index = queueController->indexInQueue(mEntitlement);
        Downloader::ContentInstallFlowState flowState = mEntitlementInstallFlow->getFlowState();

        isHeadAndEnqueuedAndPaused = mEntitlementInstallFlow->isEnqueuedAndPaused() && index == 0 && (flowState == Downloader::ContentInstallFlowState::kEnqueued);
    }

    return mEntitlementInstallFlow ? (mEntitlementInstallFlow->canResume() || isHeadAndEnqueuedAndPaused) : false;
}

double EntitlementInstallFlowProxy::playableAt()
{
    if (isDynamicOperation() && mEntitlementInstallFlow)
    {
        const Downloader::InstallProgressState progressState = mEntitlementInstallFlow->progressDetail();
        if (progressState.mTotalBytes > 0 && progressState.mDynamicDownloadRequiredPortionSize != progressState.mTotalBytes)
        {
            return (static_cast<double>(progressState.mDynamicDownloadRequiredPortionSize) / progressState.mTotalBytes);
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }
}

bool EntitlementInstallFlowProxy::isDynamicOperation()
{
    bool isAllowedToPlay = true;
    if (mEntitlement && mEntitlement->localContent())
    {
        isAllowedToPlay = mEntitlement->localContent()->isAllowedToPlay();
    }
    return mEntitlementInstallFlow ? (mEntitlementInstallFlow->isDynamicDownload() && isAllowedToPlay) : false;
}

}
}
}