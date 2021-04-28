///////////////////////////////////////////////////////////////////////////////
// PlayFlow.cpp
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "PlayFlow.h"

#include "engine/content/ContentConfiguration.h"
#include "engine/content/ContentController.h"
#include "engine/content/LocalContent.h"
#include "engine/content/PlayFlow.h"
#if defined(ORIGIN_MAC)
#include "engine/igo/IGOSetupOSX.h"
#endif

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#if defined(ORIGIN_MAC)
#include "services/escalation/IEscalationClient.h"
#include "services/qt/QtUtil.h"
#include "services/settings/SettingsManager.h"
#include "services/session/SessionService.h"
#endif

#include "widgets/play/source/PlayViewController.h"
#include "Service/SDKService/SDK_ServiceArea.h"

#include "MainFlow.h"
#include "RTPFlow.h"
#include "ContentFlowController.h"
#include "TelemetryAPIDLL.h"

#include <QDialog>

namespace
{

/// \brief Handles play flows that have an alternate launcher by software ID configured.
///
/// 
void handlePendingAlternateLaunchSoftwareId(
    Origin::Engine::Content::PlayFlow* pFlow, 
    const QString &alternateSoftwareId, 
    const QString &launchParams)
{
    using namespace Origin;

    ORIGIN_LOG_EVENT << "handlePendingAlternateLaunchSoftwareId";

    if (pFlow)
    {
        ORIGIN_LOG_EVENT << "handlePendingAlternateLaunchSoftwareId: alternateLaunchSoftwareId = " << alternateSoftwareId;

        if (!alternateSoftwareId.isEmpty())
        {
            Client::ContentFlowController*  pController = Client::MainFlow::instance()->contentFlowController();

            if (pController)
            {
                ORIGIN_LOG_EVENT << "handlePendingAlternateLaunchSoftwareId: startPlayFlow";
                // Queue a new play flow to execute
                QMetaObject::invokeMethod(
                    pController, 
                    "startPlayFlow", 
                    Qt::QueuedConnection,
                    Q_ARG(const QString, alternateSoftwareId),
                    Q_ARG(const QString, launchParams));
            }
        }
    }
}

}

namespace Origin
{
namespace Client
{

PlayFlow::PlayFlow(const QString& productId)
: mPlayViewController(NULL)
, mWithoutUpdates(false)
, mLaunchParameters("")
, mPlayGameWhileUpdatingWindowVisible(false)
{
    //create the PlayView and PlayViewController 
    mPlayViewController = new PlayViewController(productId, false, this);
    mEntitlement = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(productId);
}


PlayFlow::~PlayFlow()
{
}


void PlayFlow::start()
{
    //can't find the entitlement, so can't play, just exit
    if(!mEntitlement)
    {
        onGameLaunchCanceled();
        return;
    }

    Engine::Content::PlayFlow* playFlow = mEntitlement->localContent()->playFlow().data();
    playFlow->setLaunchedFromRTP(MainFlow::instance()->rtpFlow()->getRtpLaunchActive(mEntitlement->contentConfiguration()->contentId()));
    const bool userOnline = Services::Connection::ConnectionStatesService::isUserOnline(Engine::LoginController::currentUser()->getSession());
    // If this is RTP AND we're offline (which means downlaod should be paused), we don't want to show the update progress dialog, we just want to go to launch
    if(userOnline == false)
        playFlow->setForceSkipUpdate();
    playFlow->setLaunchParameters(mLaunchParameters); //make sure to set the launch parameters before starting

    ORIGIN_VERIFY_CONNECT(mPlayViewController, SIGNAL(stopFlow()), playFlow, SIGNAL(launchCanceled()));
    ORIGIN_VERIFY_CONNECT(mPlayViewController, SIGNAL(play3PDDGameAnswer(bool)), this, SLOT(onPlay3PDDGameAnswered(bool)));
    ORIGIN_VERIFY_CONNECT(mPlayViewController, SIGNAL(pdlcDownloadingWarningAnswer(int)), this, SLOT(onPdlcDownloadingWarningAnswered(int)));
    ORIGIN_VERIFY_CONNECT(mPlayViewController, SIGNAL(playGameWhileUpdatingAnswer(int)), this, SLOT(onPlayGameWhileUpdatingAnswered(int)));
    ORIGIN_VERIFY_CONNECT(mPlayViewController, SIGNAL(updateExistAnswer(const Origin::Client::PlayViewController::UpdateExistAnswers&)), this, SLOT(onUpdatePromptAnswered(const Origin::Client::PlayViewController::UpdateExistAnswers&)));
    ORIGIN_VERIFY_CONNECT(mPlayViewController, SIGNAL(launchQueueEmpty()), playFlow, SIGNAL(pendingLaunchComplete()));
    ORIGIN_VERIFY_CONNECT(mPlayViewController, SIGNAL(trialCheckComplete()), playFlow, SIGNAL(trialCheckComplete()));
    ORIGIN_VERIFY_CONNECT(mPlayViewController, SIGNAL(upgradeSaveWarningComplete()), playFlow, SIGNAL(upgradeSaveGameWarningComplete()));
    ORIGIN_VERIFY_CONNECT(mPlayViewController, SIGNAL(firstLaunchMessageComplete()), playFlow, SIGNAL(firstLaunchMessageCheckComplete()));

    //listen for signals from the engine::PlayFlow
    ORIGIN_VERIFY_CONNECT(playFlow, SIGNAL(promptUserGeneralUpdateExists(const QString&)), mPlayViewController, SLOT(showUpdateExistsDialog()));
    ORIGIN_VERIFY_CONNECT(playFlow, SIGNAL(promptUserRequiredUpdateExists(const QString&)), mPlayViewController, SLOT(showUpdateRequiredDialog()));
    ORIGIN_VERIFY_CONNECT(playFlow, SIGNAL(promptUserPlayWhileUpdating(const QString&)), this, SLOT(onPromptUserPlayWhileUpdating()));
    ORIGIN_VERIFY_CONNECT(playFlow, SIGNAL(show3PDDPlayDialog(const QString&)), mPlayViewController, SLOT(show3PDDPlayDialog()));
    ORIGIN_VERIFY_CONNECT(playFlow, SIGNAL(showPDLCDownloadingWarning(const QString&)), mPlayViewController, SLOT(showPDLCDownloadingWarning()));
    ORIGIN_VERIFY_CONNECT(playFlow, SIGNAL(showStartingLimitedTrialDialog(const QString&)), mPlayViewController, SLOT(showStartingLimitedTrialDialog()));
    ORIGIN_VERIFY_CONNECT(playFlow, SIGNAL(showStartingTimedTrialDialog(const QString&)), mPlayViewController, SLOT(showStartingTimedTrialDialog()));
    ORIGIN_VERIFY_CONNECT(playFlow, SIGNAL(showFirstLaunchMessage()), mPlayViewController, SLOT(showFirstLaunchMessageDialog()));
    ORIGIN_VERIFY_CONNECT(playFlow, SIGNAL(showOnlineRequiredForTrialDialog(const QString&)), mPlayViewController, SLOT(showOnlineRequiredForTrialDialog()));
    ORIGIN_VERIFY_CONNECT(playFlow, SIGNAL(showMultiLaunchDialog(const QString&)), mPlayViewController, SLOT(showMultipleLaunchDialog()));
    ORIGIN_VERIFY_CONNECT(playFlow, SIGNAL(showLaunchingDialog(const QString&)), mPlayViewController, SLOT(showDefaultLaunchDialog()));
    ORIGIN_VERIFY_CONNECT(playFlow, SIGNAL(showInstallFlowResuming()), mPlayViewController, SLOT(showInstallFlowResuming()));
    ORIGIN_VERIFY_CONNECT(playFlow, SIGNAL(showUpgradeSaveGameWarning()), mPlayViewController, SLOT(showUpgradeSaveGameWarning()));

    if(mEntitlement->localContent()->installFlow())
    {
        // I don't want to include ContentInstallFlowState in our .h
        ORIGIN_VERIFY_CONNECT(mEntitlement->localContent()->installFlow(), SIGNAL(stateChanged(Origin::Downloader::ContentInstallFlowState)), this, SLOT(onInstallFlowStateChanged()));
    }

#if defined(ORIGIN_MAC)
    ORIGIN_VERIFY_CONNECT(playFlow, SIGNAL(checkIGOSettings()), this, SLOT(onCheckIGOSettings()));
    ORIGIN_VERIFY_CONNECT(mPlayViewController, SIGNAL(oigSettingsAnswer(int, bool)), this, SLOT(onOigSettingsAnswer(int, bool)));
#endif

    //check to see if the statemachine is already running
    Engine::Content::PlayFlowState pState = playFlow->getFlowState();
    // can't really do anything at this point
    if(pState == Engine::Content::kPendingCloudSync)
        return;

    //we could be here
    //1. selected PlayNow (no auto-update happening) - go thru normal flow
    //2. selected PlayNow (with auto-update happening and first time PlayNow selected, brought up updateprogress, 2nd time, same as playnow without update
    //   but engine::playFlow hasn't be started so start that.  engine::playFlow will suppress checking for update and advance to launch
    //3. selected Play after auto-update completed, running via RTP so engine::playFlow wouldn't have been started since it will have just returned from above
    //   after bringing up the updateprogressdialog. similar to 2. above, it will start but since we're updated, the updatecheck will find nothing and advance to launch

    //connect the signals so we know when it's done
    ORIGIN_VERIFY_CONNECT(
        playFlow, SIGNAL(launched(Origin::Engine::Content::LocalContent::PlayResult)),
        this, SLOT(onGameLaunched(Origin::Engine::Content::LocalContent::PlayResult)));
    ORIGIN_VERIFY_CONNECT(playFlow, SIGNAL(canceled(const QString&)), this, SLOT(onGameLaunchCanceled()));

    ORIGIN_VERIFY_CONNECT(playFlow, SIGNAL(launchedAlternate(const QString&, const QString&, const QString&)), this, SLOT(onAlternateGameLaunched(const QString&, const QString&, const QString&)));
    playFlow->begin();
    ORIGIN_LOG_EVENT << "started PlayFlow state machine for " << mEntitlement->contentConfiguration()->productId();
}


void PlayFlow::setCloudSyncFinished(const bool& proceedToPlay)
{
    mEntitlement->localContent()->playFlow()->setCloudSyncFinished(proceedToPlay);
}


void PlayFlow::onGameLaunched(Origin::Engine::Content::LocalContent::PlayResult playResult)
{
    if (mEntitlement.isNull())
    {
        ORIGIN_LOG_EVENT << "Entitlement no longer valid";
        return;
    }
    ORIGIN_LOG_EVENT << "PlayFlow::onGameLaunched: [" << mEntitlement->contentConfiguration()->productId() << "] - result:" << playResult.result;

    if(MainFlow::instance()->rtpFlow()->getRtpLaunchActive(mEntitlement->contentConfiguration()->contentId()))
    {
        //clearRtpLaunchInfo will chekc that contentId matches
        MainFlow::instance()->rtpFlow()->clearRtpLaunchInfo(mEntitlement->contentConfiguration()->contentId());
    }

    Engine::Content::ContentController::currentUserContentController()->prepareIGO(mEntitlement->contentConfiguration());
    GetTelemetryInterface()->SetGameLaunchSource(EbisuTelemetryAPI::Launch_Client);

    // TODO: For the love of god we need to clean up LocalContent::launch to get in here. This should be signal from the playflow, not a signal from
    // local content to playflow to here. wtf.
    // Most of the logic doesn't belong there. The layering makes zero sense.
    if (playResult.result == Origin::Engine::Content::LocalContent::PlayResult::PLAY_FAILED_MIN_OS)
    {
        mPlayViewController->showSystemRequirementsNotMet(playResult.info);
    }
    else if (playResult.result == Origin::Engine::Content::LocalContent::PlayResult::PLAY_FAILED_PLUGIN_INCOMPATIBLE)
    {
        mPlayViewController->showPluginIncompatible();
    }

    // If this PlayFlow was caused by an alternate launch software, restore
    // the alternate launch software to let it know that all the conflicts
    // requiring the user's attention have been resolved (it's fine to fire
    // this event even when no corresponding minimize request was fired).
    Origin::SDK::SDK_ServiceArea::instance()->RestoreRequest();
    ORIGIN_LOG_DEBUG << "RestoreRequest";

    PlayFlowResult result;
    result.result = (playResult.result == Origin::Engine::Content::LocalContent::PlayResult::PLAY_SUCCESS) ? FLOW_SUCCEEDED : FLOW_FAILED;
    result.productId = mEntitlement->contentConfiguration()->productId();
    
    emit finished(result);
}

void PlayFlow::onAlternateGameLaunched(const QString& productId, const QString &alternateSoftwareId, const QString &launchParams)
{
    ORIGIN_LOG_EVENT << "onAlternateGameLaunched";

    if (!mEntitlement.isNull() && productId == mEntitlement->contentConfiguration()->productId())
    {
        ORIGIN_LOG_EVENT << "PlayFlow: onAlternateLaunched: [" << productId << "]";

        Engine::Content::PlayFlow * const pEnginePlayFlow = mEntitlement->localContent()->playFlow().data();
        ORIGIN_VERIFY_DISCONNECT(pEnginePlayFlow, SIGNAL(launchedAlternate(const QString&, const QString&, const QString&)), this, SLOT(onAlternateGameLaunched(const QString&)));

        mEntitlement->localContent()->alternateGameLaunched();  //this will trigger UI update to undim the game tile

        if (MainFlow::instance()->rtpFlow()->getRtpLaunchActive (mEntitlement->contentConfiguration()->contentId()))
        {
            //clearRtpLaunchInfo will chekc that contentId matches
            MainFlow::instance()->rtpFlow()->clearRtpLaunchInfo (mEntitlement->contentConfiguration()->contentId());
        }

        // Start a new play flow if we have a pending alternate launcher software ID
        // that we need to launch.
        handlePendingAlternateLaunchSoftwareId(pEnginePlayFlow, alternateSoftwareId, launchParams);

        PlayFlowResult result;
        result.result = FLOW_SUCCEEDED;
        result.productId = productId;
        emit finished(result);
    }
    else
    {
        ORIGIN_LOG_ERROR << "unable to retrieve entitlement by mProductId:[" << productId << "]";
    }
}

void PlayFlow::onGameLaunchCanceled()
{
    QString productId = "";
    if(!mEntitlement.isNull())
    {
        productId = mEntitlement->contentConfiguration()->productId();

        ORIGIN_VERIFY_DISCONNECT(mEntitlement->localContent()->playFlow().data(), SIGNAL(canceled(const QString&)), this, SLOT(onGameLaunchCanceled()));

        if(MainFlow::instance()->rtpFlow()->getRtpLaunchActive(mEntitlement->contentConfiguration()->contentId()))
        {
            //clearRtpLaunchInfo will chekc that contentId matches
            MainFlow::instance()->rtpFlow()->clearRtpLaunchInfo(mEntitlement->contentConfiguration()->contentId());
        }

        sendCancelTelemetry();
    }
    else
    {
        ORIGIN_LOG_ERROR << "unable to retrieve entitlement by productId = [" << productId << "]";
    }

    // If this PlayFlow was caused by an alternate launch software, restore
    // the alternate launch software to let it know that all the conflicts
    // requiring the user's attention have been resolved (it's fine to fire
    // this event even when no corresponding minimize request was fired).
    Origin::SDK::SDK_ServiceArea::instance()->RestoreRequest();
    ORIGIN_LOG_DEBUG << "RestoreRequest";

    PlayFlowResult result;
    result.result = FLOW_FAILED;
    result.productId = productId;
    emit finished(result);
}


#if defined(ORIGIN_MAC)
void PlayFlow::onCheckIGOSettings()
{
    char errorMsg[256];
    // Check if the assistive devices setting is on (needed to capture game events)
    if(!Engine::CheckIGOSystemPreferences(errorMsg, sizeof(errorMsg)))
    {
        if(errorMsg[0])
        {
            ORIGIN_LOG_ERROR << errorMsg;
        }
        mPlayViewController->showOIGSettings();
    }
    else
    {
        // Setting is already set correctly - skip the prompt
        mEntitlement->localContent()->playFlow()->setCheckIGOSettingsResponse(true);
    }
}


void PlayFlow::onOigSettingsAnswer(int result, bool oigEnabled)
{
    if(result == QDialog::Accepted)
    {
        if(oigEnabled)
        {
            // Use the escalation services not to request the user's credentials
            QString escalationReasonStr = "enableAssistiveDevices";
            int escalationError = 0;
            QSharedPointer<Origin::Escalation::IEscalationClient> escalationClient;
            if (!Origin::Escalation::IEscalationClient::quickEscalate(escalationReasonStr, escalationError, escalationClient))
                return;

            escalationError = escalationClient->enableAssistiveDevices();
            if (!Origin::Escalation::IEscalationClient::evaluateEscalationResult(escalationReasonStr, "createDirectory", escalationError, escalationClient->getSystemError()))
            {
                ORIGIN_LOG_ERROR << "Unable to enable assistive devices setting";
            }
        }
        
        // We need to always save user's choice since SETTING_EnableIgo defaults to true
        // This fixes EBIBUGS-23184
        // https://developer.origin.com/support/browse/EBIBUGS-23184
        // Turn on IGO if they've checked the setting
        // Turn off IGO if they've unchecked the setting
        Services::writeSetting(Services::SETTING_EnableIgo, oigEnabled);

        // Proceed with game launch
        mEntitlement->localContent()->playFlow()->setCheckIGOSettingsResponse(true);
    }
    else
    {
        // Cancel the game launch
        mEntitlement->localContent()->playFlow()->setCheckIGOSettingsResponse(false);
    }
}
#endif


void PlayFlow::onPromptUserPlayWhileUpdating()
{
    if(mWithoutUpdates)
    {
        //go straight to launching
        Engine::Content::PlayFlow* playFlow = mEntitlement->localContent()->playFlow().data();
        playFlow->startPlayNowWithoutUpdate(); 
    }
    else
    {
        mPlayGameWhileUpdatingWindowVisible = true;
        ORIGIN_VERIFY_CONNECT(mEntitlement->localContent(), SIGNAL(stateChanged(Origin::Engine::Content::EntitlementRef)), this, SLOT(onLocalStateChanged()));
        mPlayViewController->showGameUpdatingWarning();
    }
}


void PlayFlow::onPlayGameWhileUpdatingAnswered(int result)
{
    ORIGIN_VERIFY_DISCONNECT(mEntitlement->localContent(), SIGNAL(stateChanged(Origin::Engine::Content::EntitlementRef)), this, SLOT(onLocalStateChanged()));
    mPlayGameWhileUpdatingWindowVisible = false;
    Engine::Content::PlayFlow* playFlow = mEntitlement->localContent()->playFlow().data();
    if(result == QDialog::Accepted)
    {
        playFlow->playWhileUpdating();
    }
    else
    {
        playFlow->cancel();
    }
}


void PlayFlow::onUpdatePromptAnswered(const Origin::Client::PlayViewController::UpdateExistAnswers& result)
{
    switch(result)
    {
    case PlayViewController::PlayWithoutUpdate:
        mEntitlement->localContent()->playFlow()->pauseAndSkipUpdate();
        break;

    case PlayViewController::PlayWhileUpdating:
        mEntitlement->localContent()->update();
        onPromptUserPlayWhileUpdating();
        break;

    case PlayViewController::UpdateCancelLaunch:
        mEntitlement->localContent()->update();
        mEntitlement->localContent()->playFlow()->cancel();
        break;

    default:
    case PlayViewController::CancelLaunch:
        mEntitlement->localContent()->playFlow()->cancel();
        break;
    }
}


void PlayFlow::onRestoreBackupSaveFiles()
{
    Client::MainFlow::instance()->contentFlowController()->startSaveRestore(mEntitlement->contentConfiguration()->productId());
    mEntitlement->localContent()->playFlow()->cancel();
}


void PlayFlow::onLocalStateChanged()
{
    Origin::Downloader::IContentInstallFlow* installFlow = mEntitlement->localContent()->installFlow();
    if(installFlow && !installFlow->isActive())
    {
        if(mEntitlement->localContent()->playFlow()->getFlowState() == Engine::Content::kCheckForUpdate
           && installFlow->isUpdate())
        {
            ORIGIN_VERIFY_DISCONNECT(mEntitlement->localContent(), SIGNAL(stateChanged(Origin::Engine::Content::EntitlementRef)), this, SLOT(onLocalStateChanged()));
            mPlayViewController->showUpdateCompleteDialog();
        }
    }
}


void PlayFlow::onPlay3PDDGameAnswered(bool result)
{
    mEntitlement->localContent()->playFlow()->setResponseTo3PDDPlayDialog(result);
}


void PlayFlow::onPdlcDownloadingWarningAnswered(int result)
{
    if(result == QDialog::Accepted)
    {
        mEntitlement->localContent()->playFlow()->setSkipPDLCCheck();
    }
    else
    {
        mEntitlement->localContent()->playFlow()->cancel();
    }
}


void PlayFlow::onInstallFlowStateChanged()
{
    const Downloader::ContentInstallFlowState flowState = mEntitlement->localContent()->installFlow() ? mEntitlement->localContent()->installFlow()->getFlowState() : Downloader::ContentInstallFlowState::kInvalid;
    if(flowState == Downloader::ContentInstallFlowState::kTransferring
        && mEntitlement->localContent()->playFlow()->getFlowState() == Engine::Content::kCheckForOperation)
    {
        mPlayViewController->closeWindow();
        Engine::Content::PlayFlow* playFlow = mEntitlement->localContent()->playFlow().data();
        playFlow->startOperationStarted();
    }
}


void PlayFlow::sendCancelTelemetry()
{
    QString playState = "";
    using namespace Engine::Content;
    switch(mEntitlement->localContent()->playFlow()->getFlowState())
    {
    case kInvalid:
        playState = "State-Invalid";
        break;
    case kError:
        playState = "State-Error";
        break;
    case kCanceling:
        playState = "State-Canceling";
        break;
    case kReadyToStart:
        playState = "State-ReadyToStart";
        break;
    case kNonMonitoredInstall:
        playState = "State-NonMonitoredInstall";
        break;
    case kCheckForOperation:
        playState = "State-CheckForOperation";
        break;
    case kCheckForUpdate:
        playState = "State-CheckForUpdate";
        break;
    case kCheckIGOSettings:
        playState = "State-CheckIGOSettings";
        break;
    case kPendingCloudSync:
        playState = "State-PendingCloudSync";
        break;
    case kPendingMultiLaunch:
        playState = "State-PendingMultiLaunch";
        break;
    case kPendingLaunch:
        playState = "State-PendingLaunch";
        break;
    case kShow3PDDPlayDialogCheck:
        GetTelemetryInterface()->Metric_3PDD_PLAY_DIALOG_CANCEL();
        playState = "State-Show3PDDPlayDialogCheck";
        break;
    case kPDLCDownloadingCheck:
        playState = "State-PDLCDownloadingCheck";
        break;
    case kTrialCheck:
        playState = "State-TrialCheck";
        break;
    case kLaunchGame:
        playState = "State-LaunchGame";
        break;
    case kFinalState:
        playState = "State-FinalState";
        break;
    default:
        playState = "State-Unknown";
        break;
    }
    GetTelemetryInterface()->Metric_GAME_LAUNCH_CANCELLED(mEntitlement->contentConfiguration()->productId().toUtf8().data(), playState.toUtf8().data());
}

}
}
