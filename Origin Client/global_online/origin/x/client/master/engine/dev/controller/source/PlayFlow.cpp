///////////////////////////////////////////////////////////////////////////////
// Engine::PlayFlow.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "engine/content/CloudContent.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/content/ContentController.h"
#include "engine/content/ContentOperationQueueController.h"
#include "engine/content/Entitlement.h"
#include "engine/content/GamesController.h"
#include "engine/content/LocalContent.h"
#include "engine/content/LocalInstallProperties.h"
#include "engine/content/PlayFlow.h"
#include "engine/cloudsaves/LocalStateBackup.h"
#include "engine/cloudsaves/LocalStateCalculator.h"
#include "engine/downloader/IContentInstallFlow.h"
#include "engine/login/LoginController.h"
#include "engine/login/User.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/platform/EnvUtils.h"
#include "services/platform/PlatformJumplist.h"
#include "services/settings/SettingsManager.h"

#include "services/rest/AuthPortalServiceClient.h"
#include "services/rest/AuthPortalServiceResponses.h"

#include "qfinalstate.h"
#include "TelemetryAPIDLL.h"

#include <QApplication>

using namespace Origin::Services;

namespace
{

/// \brief Determines whether the given content has an alternate launcher configured by software ID.
/// \return True if the given content has an alternate launcher by software ID configured and should
/// be launched, false otherwise.
/// Telemetry does fire from this function
const bool shouldLaunchAlternateSoftwareId(const Origin::Engine::Content::EntitlementRef& content)
{
    bool shouldLaunch = false;

    // Check if alternate launcher software ID is present in the offer config and whether we're entitled to it.
    Origin::Engine::Content::ContentController* pContentController = Origin::Engine::Content::ContentController::currentUserContentController();

    QString alternateLaunchSoftwareId(content->contentConfiguration()->alternateLaunchSoftwareId());

    if (pContentController && !alternateLaunchSoftwareId.isEmpty())
    {
        Origin::Engine::Content::EntitlementRef ent = pContentController->entitlementBySoftwareId(alternateLaunchSoftwareId);
        const char8_t* launchtype = "software_id";

        if (ent)
        {
            using namespace Origin::Engine::Content;

            LocalContent* pLocalContent = ent->localContent();
            const LocalContent::RCState releaseState = pLocalContent->releaseControlState();

            const bool contentExpired = releaseState == LocalContent::RC_Expired;
            const bool contentReleased = 
                pLocalContent->releaseControlState() == LocalContent::RC_Released || 
                ent->contentConfiguration()->originPermissions() >= Origin::Services::Publishing::OriginPermissions1102;

            // We only want to launch this content if it's installed, released, and ready to be played,
            // or if the user happens to already be playing it. If the content isn't installed/released,
            // then this alternate launcher shouldn't be launched (that is, don't force the user to
            // have the alternate software installed to play their game - FOR NOW...).
            const bool contentReady = pLocalContent->installed(true) && contentReleased && !contentExpired;
                
            ORIGIN_LOG_EVENT << "shouldLaunchAlternateSoftwareId: alternate launch content ready = " << contentReady; 

            if (contentReady)
            {
                ORIGIN_LOG_EVENT << "shouldLaunchAlternateSoftwareId: shouldLaunch is true!"; 
                shouldLaunch = true;
                GetTelemetryInterface()->setAlternateSoftwareIDLaunch(content->contentConfiguration()->productId().toStdString().c_str(), ent->contentConfiguration()->productId().toStdString().c_str());
            }
            else
            {
                QString reason = "Unknown";
                if (!pLocalContent->installed(true))
                {
                    reason = "Not Installed";
                }
                else if (!contentReleased) 
                {
                    reason = "Content Not Released";
                }
                else if (contentExpired)
                {
                    reason = "Content Expired";
                }
                GetTelemetryInterface()->Metric_ALTLAUNCHER_SESSION_FAIL(content->contentConfiguration()->productId().toLatin1(),
                    launchtype,reason.toLatin1(),alternateLaunchSoftwareId.toLatin1());
            }
        }
        else
        {
            GetTelemetryInterface()->Metric_ALTLAUNCHER_SESSION_FAIL(content->contentConfiguration()->productId().toLatin1(),
                launchtype, "Not Found", alternateLaunchSoftwareId.toLatin1());

        }
    }

    return shouldLaunch;
}

/// \brief Determines whether the given content has an alternate launcher configured by URL.
/// \return True if the given content has an alternate launcher by URL configured and should
/// be launched, false otherwise.
const bool shouldLaunchAlternateUrl(const Origin::Engine::Content::EntitlementRef& content)
{
    bool shouldLaunch = false;

    const bool userOnline = 
        Origin::Services::Connection::ConnectionStatesService::isUserOnline (Origin::Engine::LoginController::instance()->currentUser()->getSession());

    if (userOnline)
    {
        QString alternateUrl(content->contentConfiguration()->gameLauncherUrl());
        if (!alternateUrl.isEmpty())
        {
            shouldLaunch = true;
        }
    }

    return shouldLaunch;
}

}

namespace Origin
{
    namespace Engine
    {
        namespace Content
        {
            using namespace Origin::Services;
            
            PlayFlow::PlayFlow(Origin::Engine::Content::EntitlementRef content)
                : mContent(content)
                , mLaunchedFromRTP(false)
                , mShowDefaultLaunch(false)
                , mUpdateCheckFlow(content)
            {
                ORIGIN_VERIFY_CONNECT(
                    Session::SessionService::instance(),
                    SIGNAL(endSessionComplete(Origin::Services::Session::SessionError, Origin::Services::Session::SessionRef)),
                    this, SLOT(onLogout())
                );

                initializePlayFlowStates();
                initializePlayFlow();
                setInitialStartStates ();
                mForceSkipUpdate = false;
                mbLaunchElevated = false;

                ORIGIN_VERIFY_CONNECT(&mUpdateCheckFlow, SIGNAL(noUpdateAvailable()), this, SIGNAL(skipUpdate()));
                ORIGIN_VERIFY_CONNECT(&mUpdateCheckFlow, SIGNAL(updateAvailable()), this, SLOT(onUpdateExists()));

                // ContentFlowController indexes and deletes PlayFlows from the main thread, so this object needs to live on the main thread.
                moveToThread(QApplication::instance()->thread());
            }

            PlayFlow::~PlayFlow ()
            {
                if ( mFinalState )
                {
                    delete mFinalState;
                }
                for ( StateMap::const_iterator i = mTypeToStateMap.begin(); i != mTypeToStateMap.end(); ++i ) 
                {
                    delete i.value();
                }
            }

            PlayFlowState PlayFlow::getFlowState()
            {
                return mCurrentState;
            }

            void PlayFlow::begin()
            {
				//double check to make sure that the item is installed and is playable
				//tho presumably this shouldn't be called if it isn't installed yet
				//  - need to set as flag because if we call localContent()->playable() inside the flow, it will return IS_BUSY
				mWasPlayableAtStart = mContent->localContent()->playable ();

                // Content can be configured with the "allowMultipleInstance" DiP manifest flag
                // that allows us to launch the software even if it's already playing. For reason
                // similar to mWasPlayableAtStart, we need to determine if the content state was 
                // "playing" at the beginning of the flow.
                mWasPlayingAtStart = mContent->localContent()->playing();

				start();    //start the StateMachine
            }

            void PlayFlow::beginFromJoin()
            {
                // If the user is joining a game with a launch URL but the .exe is running
                // mContent->localContent()->playable () will return false preventing us from launching the URL
                // This function should only be called for the explicit purpose of joining a game with a launch URL
                mWasPlayableAtStart = true;

                start();    //start the StateMachine
            }

            void PlayFlow::cancel ()
            {
                emit launchCanceled();
            }

            void PlayFlow::setForceSkipUpdate ()
            {
                mForceSkipUpdate = true;
            }

            void PlayFlow::pauseAndSkipUpdate()
            {
                // Pause the update if there is one in progress.
                if(mContent->localContent()->installFlow() && mContent->localContent()->installFlow()->canPause())
                    mContent->localContent()->installFlow()->pause(false);
                emit skipUpdate();
            }

            void PlayFlow::playWhileUpdating()
            {
                emit updatePlayNow();
            }

            void PlayFlow::setSkipPDLCCheck()
            {
                emit pdlcDownloadCheckComplete();
            }

            void PlayFlow::setResponseTo3PDDPlayDialog (bool doPlay)
            {
                if(doPlay)
                    emit show3PDDPlayDialogComplete();
                else
                {
                    emit launchCanceled();
                }
            }

#if defined(ORIGIN_MAC)
            void PlayFlow::setCheckIGOSettingsResponse(bool doPlay)
            {
                if (doPlay)
                {
                    emit (playIGOConfirmed());
                }
                else
                {
                    emit (launchCanceled());
                }
            }
#endif

            void PlayFlow::startPlayNowWithoutUpdate ()
            {
                emit playNowWithoutUpdate();
            }

            void PlayFlow::startOperationStarted()
            {
                emit operationStarted();
            }

            void PlayFlow::setCloudSyncFinished(const bool& proceedToPlay)
            {
                if (proceedToPlay)
                    emit cloudSyncComplete();
                else
                    emit launchCanceled();
            }

            void PlayFlow::setLaunchParameters (const QString& launchParameters)
            {
                mLaunchParameters = launchParameters;
            }

            void PlayFlow::clearLaunchParameters ()
            {
                // Don't clear mLaunchedFromRTP at the end of a new launch. On the session end we need to know
                // how the game play session started.

                //clear the launch parameters
                mLaunchParameters.clear();
            }

            void PlayFlow::clearLauncher()
            {
                mLauncher.clear();
            }

            QState* PlayFlow::getState(PlayFlowState type)
            {
                QState* state = NULL;
                StateMap::const_iterator i = mTypeToStateMap.find(type);

                if (i != mTypeToStateMap.constEnd())
                {
                    state = i.value();
                }

                return state;
            }

            PlayFlowState PlayFlow::getStateType(QState* state)
            {
                PlayFlowState type = kInvalid;
                ReverseStateMap::const_iterator i = mStateToTypeMap.find(state);
                if (i != mStateToTypeMap.constEnd())
                {
                    type = i.value();
                }

                return type;
            }

            void PlayFlow::setStartState(PlayFlowState type)
            {
                StateMap::const_iterator i = mTypeToStateMap.find(type);
                if (i != mTypeToStateMap.constEnd())
                {
                    setInitialState(i.value());
                }
            }

            void PlayFlow::setFlowState(PlayFlowState type)
            {
                mCurrentState = type;
            }

            void PlayFlow::initializePlayFlow()
            {
                QState* readyToStart = getState(kReadyToStart);
                ORIGIN_VERIFY_CONNECT(readyToStart, SIGNAL(entered()), this, SLOT(onReadyToStart()));

                //entered if its a non-monitored install
                QState* nonMonitoredInstall = getState(kNonMonitoredInstall);
                ORIGIN_VERIFY_CONNECT(nonMonitoredInstall, SIGNAL(entered()), this, SLOT(onNonMonitoredInstall()));

                //check if update exists, emit signal to prompt user for response update/play now
                QState* checkForOperation = getState(kCheckForOperation);
                ORIGIN_VERIFY_CONNECT(checkForOperation, SIGNAL(entered()), this, SLOT(onCheckForOperation()));

                //check if update exists, emit signal to prompt user for response update/play now
                QState* checkForUpdate = getState(kCheckForUpdate);
                ORIGIN_VERIFY_CONNECT(checkForUpdate, SIGNAL(entered()), this, SLOT(onCheckForUpdate()));

                //see if we need to show the 3PDD dialog
                QState* show3PDDPlayDialogCheck = getState(kShow3PDDPlayDialogCheck);
                ORIGIN_VERIFY_CONNECT(show3PDDPlayDialogCheck, SIGNAL(entered()), this, SLOT(onShow3PDDPlayDialogCheck()));

                //check if Mac has IGO properties, for PC just automatically advance
                QState *checkIGOSettings = getState (kCheckIGOSettings);
                ORIGIN_VERIFY_CONNECT (checkIGOSettings, SIGNAL(entered()), this, SLOT (onCheckIGOSettings()));

                QState* upgradeSaveGameWarning = getState(kUpgradeSaveGameWarning);
                ORIGIN_VERIFY_CONNECT(upgradeSaveGameWarning, SIGNAL(entered()), this, SLOT(onUpgradeSaveGameWarning()));

                // // SHOULD WE PUT THIS BACK? SUBSCRIPTION LOCAL BACKUP https://developer.origin.com/support/browse/ORSUBS-1523
                //QState* checkForLocalSaveBackup = getState(kCheckForLocalSaveBackup);
                //ORIGIN_VERIFY_CONNECT(checkForLocalSaveBackup, SIGNAL(entered()), this, SLOT(onCheckForLocalSaveBackup()));

                //launching dialog shown (and cloud sync may also be happening), wait for cancel or cloud sync to finish
                QState* pendingCloudSync = getState(kPendingCloudSync);
                ORIGIN_VERIFY_CONNECT(pendingCloudSync, SIGNAL(entered()), this, SLOT(onPendingCloudSync()));

                // SHOULD WE PUT THIS BACK? SUBSCRIPTION LOCAL BACKUP https://developer.origin.com/support/browse/ORSUBS-1523
                ////launching dialog shown (and cloud sync may also be happening), wait for cancel or cloud sync to finish
                //QState* pendingCloudSaveBackup = getState(kPendingCloudSaveBackup);
                //ORIGIN_VERIFY_CONNECT(pendingCloudSaveBackup, SIGNAL(entered()), this, SLOT(onPendingCloudSaveBackup()));

                // multi launch dialog shown
                QState* pendingMultiLaunch = getState(kPendingMultiLaunch);
                ORIGIN_VERIFY_CONNECT(pendingMultiLaunch, SIGNAL(entered()), this, SLOT(onPendingMultiLaunch()));

                // multi launch or default dialog shown
                QState* pendingLaunch = getState(kPendingLaunch);
                ORIGIN_VERIFY_CONNECT(pendingLaunch, SIGNAL(entered()), this, SLOT(onPendingLaunch()));

                //warn the user that downloading PDLC while playing may cause issues, launch if user chooses to continue
                QState* pdlcDownloadingCheck = getState(kPDLCDownloadingCheck);
                ORIGIN_VERIFY_CONNECT(pdlcDownloadingCheck, SIGNAL(entered()), this, SLOT(onPDLCDownloadingCheck()));

                //inform user that a timed trial is about to start, launch if user chooses to continue
                QState* trialCheck = getState(kTrialCheck);
                ORIGIN_VERIFY_CONNECT(trialCheck, SIGNAL(entered()), this, SLOT(onTrialCheck()));

                // show the user the first launch message if we haven't already
                QState* firstLaunchMessageCheck = getState(kFirstLaunchMessage);
                ORIGIN_VERIFY_CONNECT(firstLaunchMessageCheck, SIGNAL(entered()), this, SLOT(onFirstLaunchMessageCheck()));

                //actually launch the game
                QState* launchGame = getState(kLaunchGame);
                ORIGIN_VERIFY_CONNECT(launchGame, SIGNAL(entered()), this, SLOT(onLaunchGame()));

                //canceling the launch of the game, via either canceling of downloading the update or on the launch dialog
                QState* canceling = getState(kCanceling);
                ORIGIN_VERIFY_CONNECT(canceling, SIGNAL(entered()), this, SLOT(onCanceling()));

                //launching an alterate url
                QState* launchAlternateUrl = getState(kLaunchAlternateUrl);
                ORIGIN_VERIFY_CONNECT (launchAlternateUrl, SIGNAL(entered()), this, SLOT (onLaunchAlternateUrl()));

                //launching an alterate software id
                QState* launchAlternateSoftwareId = getState(kLaunchAlternateSoftwareId);
                ORIGIN_VERIFY_CONNECT (launchAlternateSoftwareId, SIGNAL(entered()), this, SLOT (onLaunchAlternateSoftwareId()));

                //create the final state
                QFinalState *finalState = new QFinalState(this);

                readyToStart->addTransition(this, SIGNAL(beginFlow()), show3PDDPlayDialogCheck);
                readyToStart->addTransition(this, SIGNAL(nonMonitoredInstallCheck()), nonMonitoredInstall);
                readyToStart->addTransition (this, SIGNAL (launchCanceled()), canceling);

                nonMonitoredInstall->addTransition (this, SIGNAL (launchCanceled()), canceling);
                nonMonitoredInstall->addTransition (this, SIGNAL(nonMonitoredInstalled()), show3PDDPlayDialogCheck);

                show3PDDPlayDialogCheck->addTransition(this, SIGNAL(show3PDDPlayDialogComplete()), checkForOperation);
                show3PDDPlayDialogCheck->addTransition (this, SIGNAL (launchCanceled()), canceling);

                checkForOperation->addTransition(this, SIGNAL(operationStarted()), checkIGOSettings);
                checkForOperation->addTransition(this, SIGNAL(noOperation()), checkForUpdate);
                checkForOperation->addTransition(this, SIGNAL (launchCanceled()), canceling);

                checkForUpdate->addTransition (this, SIGNAL(skipUpdate()), checkIGOSettings);
                checkForUpdate->addTransition (this, SIGNAL(updatePlayNow()), checkIGOSettings);
                checkForUpdate->addTransition (this, SIGNAL (launchCanceled()), canceling);

    //TODO: need to handle cancellation based on update download error

                checkIGOSettings->addTransition (this, SIGNAL (playIGOConfirmed()), pdlcDownloadingCheck);
                checkIGOSettings->addTransition (this, SIGNAL (launchCanceled()), canceling);
                checkIGOSettings->addTransition (this, SIGNAL (launchAlternateUrl()), launchAlternateUrl);
                checkIGOSettings->addTransition (this, SIGNAL (launchAlternateSoftwareId()), launchAlternateSoftwareId);

                launchAlternateUrl->addTransition (this, SIGNAL(launchCanceled()), canceling);
                launchAlternateSoftwareId->addTransition (this, SIGNAL(launchCanceled()), canceling);

                pdlcDownloadingCheck->addTransition( this, SIGNAL(pdlcDownloadCheckComplete()), trialCheck);
                pdlcDownloadingCheck->addTransition (this, SIGNAL(launchCanceled()), canceling);

                trialCheck->addTransition(this, SIGNAL(trialCheckComplete()), firstLaunchMessageCheck);
                trialCheck->addTransition( this, SIGNAL(launchCanceled()), canceling);

                firstLaunchMessageCheck->addTransition(this, SIGNAL(firstLaunchMessageCheckComplete()), upgradeSaveGameWarning);
                firstLaunchMessageCheck->addTransition(this, SIGNAL(launchCanceled()), canceling);

                upgradeSaveGameWarning->addTransition(this, SIGNAL(upgradeSaveGameWarningComplete()), pendingCloudSync);
                upgradeSaveGameWarning->addTransition(this, SIGNAL(launchCanceled()), canceling);

                //SHOULD WE PUT THIS BACK? SUBSCRIPTION LOCAL BACKUP https://developer.origin.com/support/browse/ORSUBS-1523
                //checkForLocalSaveBackup->addTransition(this, SIGNAL(localBackupSaveCheckComplete()), pendingCloudSync);
                //checkForLocalSaveBackup->addTransition(this, SIGNAL(launchCanceled()), canceling);

                pendingCloudSync->addTransition(this, SIGNAL(cloudSyncComplete()), pendingMultiLaunch);
                pendingCloudSync->addTransition(this, SIGNAL(launchCanceled()), canceling);

                // SHOULD WE PUT THIS BACK? SUBSCRIPTION LOCAL BACKUP https://developer.origin.com/support/browse/ORSUBS-1523
                // pendingCloudSaveBackup->addTransition(this, SIGNAL(cloudSaveBackupComplete()), pendingMultiLaunch);

                pendingMultiLaunch->addTransition (this, SIGNAL(multiLaunchCheckComplete()), pendingLaunch);
                pendingMultiLaunch->addTransition (this, SIGNAL(pendingLaunchComplete()), launchGame);
                pendingMultiLaunch->addTransition (this, SIGNAL(launchCanceled()), canceling);

                pendingLaunch->addTransition (this, SIGNAL(pendingLaunchComplete()), launchGame);
                pendingLaunch->addTransition (this, SIGNAL(launchCanceled()), canceling);

                launchGame->addTransition (this, SIGNAL(endplayflow()), finalState);
                canceling->addTransition (this, SIGNAL (endplayflow()), finalState);
                launchAlternateUrl->addTransition (this, SIGNAL(endplayflow()), finalState);
                launchAlternateSoftwareId->addTransition (this, SIGNAL(endplayflow()), finalState);

                if(GamesController::currentUserGamesController() == NULL)
                    return;

                //we want to make sure that, if able, the default launcher is initialized to what will work on the current system

                //retrieve default launcher from settings
                QString sDefaultLauncherSetting = GamesController::currentUserGamesController()->gameMultiLaunchDefault(mContent->contentConfiguration()->productId());

                //does the OS check and returns the default launcher that will work if launchers are specified, if no launcher is specified, it will return empty
                QString sDefaultLauncher = mContent->localContent()->defaultLauncher(); 

                // sDefaultLauncherSetting == empty means no launcher was specified, so we'll initialize to sDefaultLauncher
                // sDefaultLauncherSetting == SHOW_ALL_TITILES, it will initialize to sDefaultLauncher
                // sDefaultLauncherSetting == game title, it will check to make sure that that version can be launched
                if (!sDefaultLauncherSetting.isEmpty())  //something was saved off
                {
                    //make sure that the one that was saved off will work on this system
                    if (sDefaultLauncherSetting != GamesController::SHOW_ALL_TITLES) //if all_games is selected, it will just use sDefaultLauncher
                    {
                        if (mContent->localContent()->validateLauncher (sDefaultLauncherSetting))
                            sDefaultLauncher = sDefaultLauncherSetting;
                    }
                }
                    
                //if sDefaultLauncher is empty, then we'll have to rely on the configuration default
                if (!sDefaultLauncher.isEmpty())
                    setLauncherByTitle(sDefaultLauncher);
            }

            void PlayFlow::onLogout()
            {
                // disconnect all the signals from the state machine at logout, EBIBUGS-20521
                for ( StateMap::const_iterator e = mTypeToStateMap.begin(); e != mTypeToStateMap.end(); ++e )
                {
                    e.value()->disconnect(this);
                }
            }
            
            void PlayFlow::initializePlayFlowStates()
            {
                for (unsigned i = 0; i <= static_cast<unsigned>(kLaunchAlternateSoftwareId); ++i)
                {
                    QState* state = new QState(this);
                    ORIGIN_VERIFY_CONNECT(state, SIGNAL(entered()), this, SLOT(onStateChange()));
                    mTypeToStateMap.insert(static_cast<PlayFlowState>(i), state);
                    mStateToTypeMap.insert(state, static_cast<PlayFlowState>(i));
                }
                //also create a final state
                mFinalState = new QFinalState (this);
            }

            void PlayFlow::setInitialStartStates()
            {
                setFlowState(kReadyToStart);
                setStartState(kReadyToStart);
                setInitialState (getState (kReadyToStart));
            }

            void PlayFlow::finishFlow ()
            {
                clearLauncher();
                clearLaunchParameters();
                clearAlternateLaunchInviteParameters();
                emit (endplayflow());
                stop();
            }

            void PlayFlow::onStateChange()
            {
                QState* qstate = dynamic_cast<QState*>(QObject::sender());
                if (qstate != NULL)
                {
                    ReverseStateMap::const_iterator i = mStateToTypeMap.find(qstate);
                    if (i != mStateToTypeMap.constEnd())
                    {
                        PlayFlowState state = i.value();
                        ORIGIN_LOG_DEBUG << "PlayFlow: state changed to: " << state;
                        setFlowState(state);
                        emit stateChanged(state);
                    }
                }
            }


            void PlayFlow::onReadyToStart()
            {
                const bool shouldLaunchPlayingInstance = mWasPlayingAtStart && mContent->localContent()->allowMultipleInstances();
                if (!mWasPlayableAtStart && !shouldLaunchPlayingInstance)
				{
					cancel();	// we must kill the flow otherwise we can't start this game again (EBIBUGS-27261)
					return;
				}

                if(mContent->localContent()->shouldRestrictUnderageUser())
                {
                    mContent->localContent()->notifyCOPPAError(false);
                    emit(launchCanceled());
                }
                else
                if(mContent->contentConfiguration()->monitorInstall())
                    emit beginFlow();
                else
                    emit nonMonitoredInstallCheck();
            }

            void PlayFlow::onNonMonitoredInstall()
            {
                //if the local content is installed lets go to the launch flow
                if(mContent->localContent()->installed(true))
                    emit nonMonitoredInstalled();
                else
                {
                    //else run the installer again and quit out of play flow
                    mContent->localContent()->install();
                    emit (launchCanceled());
                }
            }

            void PlayFlow::onCheckForOperation()
            {
                Engine::Content::ContentOperationQueueController* queueController = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController();
                // If entitlement enqueued and is a dynamic download
                if (queueController && mContent && mContent->localContent() && mContent->localContent()->installFlow() && mContent->localContent()->installFlow()->isDynamicDownload())
                {
                    const int index = queueController->indexInQueue(mContent->contentConfiguration()->productId());
                    switch(index)
                    {
                        // Not in queue
                    case -1:
                        break;

                        // Head of queue
                    case 0:
                        // We can play if we are offline. Only resume when we are online
                        if(Origin::Services::Connection::ConnectionStatesService::isUserOnline(LoginController::instance()->currentUser()->getSession()) && mContent->localContent()->installFlow()->canResume())
                        {
                            mContent->localContent()->installFlow()->resume();
                            emit showInstallFlowResuming();
                        }
                        else
                        {
                            emit operationStarted();
                        }
                        return;

                        // In queue
                    default:
                        {
                            const bool queueSkippingEnabled = queueController->queueSkippingEnabled(mContent);
                            queueController->pushToTop(mContent->contentConfiguration()->productId());
                            if(queueSkippingEnabled)
                            {
                                if(Origin::Services::Connection::ConnectionStatesService::isUserOnline(LoginController::instance()->currentUser()->getSession()))
                                {
                                    emit showInstallFlowResuming();
                                }
                                else
                                {
                                    emit operationStarted();
                                }
                            }
                            else
                            {
                                emit launchCanceled();
                            }
                        }
                        return;
                    }
                }

                emit noOperation();
            }

            void PlayFlow::onCheckForUpdate()
            { 
                if (mForceSkipUpdate)
                {
                    mForceSkipUpdate = false;       //reset it for next time
                    emit skipUpdate();
                }
                else if (!mContent->localContent()->updatable() || !mContent->localContent()->installFlow())  //checking whether > dip 2.0
                {
                    emit skipUpdate();
                }
                //if already updating then don't prompt for update
                else if (mContent->localContent()->installFlow()->operationType() == Downloader::kOperation_Update)
                {
                    if(mContent->localContent()->installFlow()->getFlowState() == Downloader::ContentInstallFlowState::kPaused ||
                       mContent->localContent()->installFlow()->getFlowState() == Downloader::ContentInstallFlowState::kPausing)
                    {
                        emit skipUpdate();
                    }
                    else
                    {
                        emit promptUserPlayWhileUpdating(mContent->contentConfiguration()->productId());
                    }
                }
                else
                {
                    mUpdateCheckFlow.begin();
                }
            }

            void PlayFlow::onUpdateExists()
            {
                if(mContent->localContent()->treatUpdatesAsMandatory() || 
                    (Origin::Services::Connection::ConnectionStatesService::isUserOnline (LoginController::instance()->currentUser()->getSession()) && !mContent->localContent()->hasMinimumVersionRequiredForOnlineUse()))
                {
                    // prompt user that update is available
                    emit promptUserRequiredUpdateExists(mContent->contentConfiguration()->productId());
                }
                else if(static_cast<bool>(Services::readSetting(Services::SETTING_IGNORE_NON_MANDATORY_GAME_UPDATES)))
                {
                    emit skipUpdate();
                }
                else
                {
                    // prompt user that update is available
                    emit promptUserGeneralUpdateExists(mContent->contentConfiguration()->productId());
                }
            }

            void PlayFlow::onCheckIGOSettings ()
            {
                //MY: here after update check/update install has finished
                //1. check and see if an alternate url exists
                //2. if so, AND launch parameter is empty, then launch the alternate URL
                if (!LoginController::instance()->currentUser().isNull() && mLaunchParameters.isEmpty())
                {
                    // Note that an alternate launcher configured by software ID has precedence
                    // over a URL-based launcher (such as Battlelog).
                    if (shouldLaunchAlternateSoftwareId(mContent))
                    {
                        ORIGIN_LOG_EVENT << "PlayFlow::onCheckIGOSettings: shouldLaunchAlternateSoftwareId";

                        // :TODO: Not sure we really need to emit here. Not a direct call, so
                        // this emit gets queued. Maybe we can just call the logic
                        // directly since there aren't any intermediate steps to perform (unlike
                        // Battlelog, which requires an auth code request for SSO)?
                        emit (launchAlternateSoftwareId());
                        return;
                    }
                    
                    //if launching alternate URL then needs to be online. software id doesn't have this requirement
                    if (shouldLaunchAlternateUrl(mContent))
                    {
                        ORIGIN_LOG_EVENT << "PlayFlow::onCheckIGOSettings: shouldLaunchAlternateUrl";
                        emit (launchAlternateUrl ());
                        return;
                    }

                    ORIGIN_LOG_EVENT << "PlayFlow::onCheckIGOSettings: launching normally";
                }

#if defined(ORIGIN_MAC)
                // Mac needs to verify the IGO settings
                emit checkIGOSettings();
#else
                //for pc just emit the signal to advance
                emit (playIGOConfirmed());
#endif
            }

            void PlayFlow::onUpgradeSaveGameWarning()
            {
                GamesController* gamesController = GamesController::currentUserGamesController();
                if (mContent->contentConfiguration()->isEntitledFromSubscription() &&
                    mContent->contentConfiguration()->isBaseGame() &&
                    mContent->contentConfiguration()->showSubsSaveGameWarning() &&
                    gamesController)
                {
                    const bool haveShownWarning = gamesController->upgradedSaveGameWarningShown(mContent->contentConfiguration()->productId());
                    if (haveShownWarning)
                    {
                        emit upgradeSaveGameWarningComplete();
                    }
                    else
                    {
                        emit showUpgradeSaveGameWarning();
                    }
                }
                else
                {
                    emit upgradeSaveGameWarningComplete();
                }
            }

            void PlayFlow::onCheckForLocalSaveBackup()
            {
                // Same as: SaveBackupRestoreFlow::isUpgradeBackupSaveAvailable()
                if (mContent->contentConfiguration()->isEntitledFromSubscription())
                {
                    ORIGIN_LOG_ACTION << "isUpgradeBackupSaveAvailable: Entitlement is in upgraded state";
                    emit localBackupSaveCheckComplete();
                }
                else if (mContent->contentController()->hasEntitlementUpgraded(mContent))
                {
                    ORIGIN_LOG_ACTION << "isUpgradeBackupSaveAvailable: Upgraded state in game library";
                    emit localBackupSaveCheckComplete();
                }
                else
                {
                    Engine::CloudSaves::LocalStateBackup localStateBackup(mContent, Engine::CloudSaves::LocalStateBackup::BackupForSubscriptionUpgrade);
                    const bool fileExists = QFile::exists(localStateBackup.backupPath());
                    if (fileExists)
                    {
                        emit showLocalBackupSaveWarning();
                    }
                    else
                    {
                        emit localBackupSaveCheckComplete();
                    }
                }
            }

            void PlayFlow::onPendingCloudSync()
            {
                bool cloudSaves = mContent->localContent()->cloudContent()->shouldCloudSync();
                if ( cloudSaves )
                {
                    // This will trigger CloudSyncFlow to start. Once it is done it will signal the (client side) PlayFlow
                    // which will allow us to move on in this flow.
                    mContent->localContent()->cloudContent()->pullFromCloud();
                    mShowDefaultLaunch = false;
                }
                else
                    emit cloudSyncComplete();
            }

            void PlayFlow::onPendingCloudSaveBackup()
            {
                using Origin::Engine::CloudSaves::LocalStateCalculator;

                bool runBackup = (mContent->contentController() &&
                    mContent->contentConfiguration()->isEntitledFromSubscription() &&
                    mContent->localContent()->cloudContent()->hasCloudSaveSupport());

                if (runBackup)
                {
                    runBackup = false;
                    foreach (const Content::EntitlementRef &ent, mContent->contentController()->entitlementsByMasterTitleId(mContent->contentConfiguration()->masterTitleId()))
                    {
                        if (ent->contentConfiguration()->owned() &&
                            ent->contentConfiguration()->isBaseGame() &&
                            !ent->contentConfiguration()->isEntitledFromSubscription())
                        {
                            runBackup = true;
                            break;
                        }
                    }
                }

                if (runBackup)
                {
                    emit startSaveBackup(mContent->contentConfiguration()->productId());
                }
                else
                {
                    emit cloudSaveBackupComplete();
                }
            }

            void PlayFlow::onPendingMultiLaunch()
            {
                GamesController* gamesController = GamesController::currentUserGamesController();
                const QStringList launchTitles = mContent->localContent()->launchTitleList();

                // Does this game have multiple launchers?
                if(launchTitles.count())
                {
                    // If there is only one
                    if(launchTitles.count() == 1)
                    {
                        setLauncherByTitle(launchTitles[0]);
                        mShowDefaultLaunch = Services::readSetting(Services::SETTING_EnableGameLaunchCancel);
                        emit multiLaunchCheckComplete();
                    }
                    else if(gamesController)
                    {
                        const QString defaultLaunchTitle = gamesController->gameMultiLaunchDefault(mContent->contentConfiguration()->productId());
                        // If the user has never selected a multi-launcher setting, default to the first title that matches the 32/64 bit of the OS
                        if (defaultLaunchTitle == "")
                        {
                            //defaultLauncher() will return title to the launcher that matches the OS, and if unable to find OS match
                            //will just return first launcher defined
                            const QString launchTitle = mContent->localContent()->defaultLauncher();
                            setLauncherByTitle(launchTitle);
                            mShowDefaultLaunch = Services::readSetting(Services::SETTING_EnableGameLaunchCancel);
                            emit multiLaunchCheckComplete();
                        }
                        // User selected a default title
                        else if(defaultLaunchTitle != GamesController::SHOW_ALL_TITLES) 
                        {
                            if (!setLauncherByTitle(defaultLaunchTitle))    //unable to find launcher that matches the saved-off user selected title (maybe title changed?)
                            {
                                ORIGIN_LOG_EVENT << "saved title mismatch, use defaultLauncher";
                                //defaultLauncher() will return title to the launcher that matches the OS, and if unable to find OS match
                                //will just return first launcher defined
                                const QString launchTitle = mContent->localContent()->defaultLauncher();
                                setLauncherByTitle (launchTitle);
                            }
                            mShowDefaultLaunch = Services::readSetting(Services::SETTING_EnableGameLaunchCancel);
                            emit multiLaunchCheckComplete();
                        }
                        // Else the default is SHOW_ALL_TITLES, in which case we should show it.
                        else
                        {
                            mShowDefaultLaunch = false;
                            emit showMultiLaunchDialog(mContent->contentConfiguration()->productId());
                        }
                    }
                }
                else
                {
                    emit multiLaunchCheckComplete();
                }
            }

            void PlayFlow::onPendingLaunch()
            {
                if(mShowDefaultLaunch && mLaunchedFromRTP == false)
                    emit showLaunchingDialog(mContent->contentConfiguration()->productId());
                else
                    emit pendingLaunchComplete();
            }

            void PlayFlow::onLaunchGame()
            {
                ORIGIN_LOG_EVENT << "PlayFlow: Launching : " << mContent->contentConfiguration()->productId();
                
                //wait to listen for the result of the launch
                ORIGIN_VERIFY_CONNECT(
                    mContent->localContent(), SIGNAL(launched(Origin::Engine::Content::LocalContent::PlayResult)),
                    this, SLOT (onLaunchResult(Origin::Engine::Content::LocalContent::PlayResult)));

                mContent->localContent()->play();

            }

            void PlayFlow::onLaunchResult (LocalContent::PlayResult playResult)
            {
                bool wasLaunched = (playResult.result == LocalContent::PlayResult::PLAY_SUCCESS);
                ORIGIN_LOG_EVENT << "PlayFlow: launch result: " << wasLaunched;

                //listen for result of launch
                ORIGIN_VERIFY_DISCONNECT(
                    mContent->localContent(), SIGNAL(launched(Origin::Engine::Content::LocalContent::PlayResult)),
                    this, SLOT (onLaunchResult(Origin::Engine::Content::LocalContent::PlayResult)));

                if (mLaunchedFromRTP)
                {
                    GetTelemetryInterface()->Metric_PERFORMANCE_TIMER_END(EbisuTelemetryAPI::RTPGameLaunch_launching, wasLaunched);
                    GetTelemetryInterface()->Metric_PERFORMANCE_TIMER_END(EbisuTelemetryAPI::RTPGameLaunch_running, wasLaunched);
                }
                
                emit (launched(playResult));
                
                finishFlow ();
            }

            void PlayFlow::onLaunchAlternateUrl ()
            {
                QString alternateUrl = mContent->contentConfiguration()->gameLauncherUrl();

                if (!alternateUrl.isEmpty())
                {
                    //check and see if it is looking for an authcode
                    if (alternateUrl.contains("{authcode}", Qt::CaseInsensitive))
                    {
                        const int NETWORK_TIMEOUT_MSECS = 30000;
                        QString client_id = mContent->contentConfiguration()->gameLauncherUrlClientId();
                        if (client_id.isEmpty())
                        {
                            ORIGIN_ASSERT_MESSAGE (!client_id.isEmpty(), "launch client_id is empty!");
                            ORIGIN_LOG_ERROR << "PlayFlow: launcher client_id is empty!";
                            //since can't retrieve authcode, set it to empty and just go straight to launch
                            alternateUrl.replace("{authcode}", "", Qt::CaseInsensitive);
                            doLaunchAlternateUrl (alternateUrl);
                        }
                        else
                        {
                            ORIGIN_LOG_EVENT << "PlayFlow: retrieving authorization code";

                            QString accessToken = Origin::Services::Session::SessionService::accessToken(Origin::Services::Session::SessionService::currentSession());
                            Origin::Services::AuthPortalAuthorizationCodeResponse *resp = Origin::Services::AuthPortalServiceClient::retrieveAuthorizationCode(accessToken, client_id);
                            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(onAuthCodeRetrieved()));
                            QTimer::singleShot(NETWORK_TIMEOUT_MSECS, resp, SLOT(abort()));

                            resp->setDeleteOnFinish(true);
                        }
                    }
                    else
                    {
                        //don't need authcode, just go straight to launch
                        doLaunchAlternateUrl (alternateUrl);
                    }
                }
                else    //just quit the flow
                {
                    emit (launchCanceled ());
                }
            }

            void PlayFlow::onLaunchAlternateSoftwareId ()
            {
                QString alternateLaunchSoftwareId(mContent->contentConfiguration()->alternateLaunchSoftwareId());

                ORIGIN_LOG_EVENT << "onLaunchAlternateSoftwareId: " << alternateLaunchSoftwareId;

                if (!alternateLaunchSoftwareId.isEmpty())
                {
                    // :TODO: how should mAltLaunchParameters be passed to sparta for game invites?
                    // See doLaunchAlternateUrl().

                    QString launchParams = 
                        "/mastertitleid=" + mContent->contentConfiguration()->masterTitleId() + " " +
                        "/multiplayerid=" + mContent->contentConfiguration()->multiplayerId();

                    // This is performed for game launcher URL (battlelog) to update UI
                    // states. I'm assuming we probably need to do this for the software
                    // ID play flow since it's a similar process.
                    emit (launchedAlternate(
                        mContent->contentConfiguration()->productId(), 
                        alternateLaunchSoftwareId,
                        launchParams));

                    finishFlow();
                }
                else
                {
                    ORIGIN_LOG_EVENT << "launchCanceled";

                    // We shouldn't have reached this point if the alternate
                    // launcher ID hasn't been configured...
                    emit (launchCanceled ());
                }
            }

            void PlayFlow::onAuthCodeRetrieved ()
            {
                AuthPortalAuthorizationCodeResponse* response = dynamic_cast<AuthPortalAuthorizationCodeResponse*>(sender());
                ORIGIN_ASSERT(response);
                if ( !response )
                {
                    //just exit the flow
                    ORIGIN_LOG_ERROR << "PlayFlow: auth code retrieval failed, response = NULL";
                    emit (launchCanceled());
                    return;
                }
                else
                {
                    ORIGIN_VERIFY_DISCONNECT(response, SIGNAL(finished()), this, SLOT(onAuthCodeRetrieved()));
                }
                
                bool success = ((response->error() == restErrorSuccess) && (response->httpStatus() == Origin::Services::Http302Found));
                if (!success)   //we need to log it
                {
                    ORIGIN_LOG_ERROR << "PlayFlow: auth code retrieval failed with: restError = " << response->error() << " httpStatus = " << response->httpStatus();
                }

                //launch battlelog regardless of whether we have authCode or not so that the user will have feedback that something happened
                //they will need to log in to battlelog manually tho
                {
                    QString authCode = response->authorizationCode();
                    ORIGIN_LOG_EVENT << "PlayFlow: Launching Alternate : " << mContent->contentConfiguration()->productId();

                    //URLS for BF4 provided by DICE, now defined under dynamic URLs
                    //<play-url authType=”code” clientId=”battlelog”>
                    //    <env name=”production”>https://battlelog.battlefield.com/bf4/sso/{token}</env>
                    //   <env name=”integration”>https://develop.battlelog.battlefield.com/bf4/sso/{token}</env>
                    //</play-url>

                    QString launchUrl = mContent->contentConfiguration()->gameLauncherUrl();

                    if (!launchUrl.isEmpty())
                    {
                        launchUrl = launchUrl.replace("{authcode}", authCode, Qt::CaseInsensitive);
                    }

                    doLaunchAlternateUrl (launchUrl);
                }
            }

            void PlayFlow::doLaunchAlternateUrl (QString launchUrl)
            {
                if (launchUrl.isEmpty())
                {
                    ORIGIN_ASSERT_MESSAGE (!launchUrl.isEmpty(), "alternate launch URL is empty");
                    ORIGIN_LOG_ERROR << "PlayFlow: launchUrl is empty, launch canceled";
                    emit (launchCanceled());
                    return;
                }
                else
                {
                    //replace {locale} if it exists
                    launchUrl.replace("{locale}", mContent->localContent()->installedLocale(), Qt::CaseInsensitive);

                    // Check to see if we have any Alternate launch parameters and append to the URL
                    // This was added for the Battlelog URL and join game

                    if (!mAltLaunchParameters.isEmpty())
                    {
                        launchUrl.append(launchUrl.contains('?') ? "&" + mAltLaunchParameters : "?" + mAltLaunchParameters);
                    }

                    Origin::Services::IProcess::runProtocol(launchUrl, QString(), QString());
                    emit (launchedAlternate(mContent->contentConfiguration()->productId()));
                    finishFlow();
                    return;
                }
            }

            void PlayFlow::onCanceling()
            {
                emit (canceled (mContent->contentConfiguration()->productId()));
                finishFlow ();
            }

            void PlayFlow::onShow3PDDPlayDialogCheck()
            {
                //if this entitlement is supposed to show the 3PDD play dialog let the UI know, otherwise lets move on
                bool showDialog = Origin::Services::readSetting(Origin::Services::SETTING_3PDD_PLAYDIALOG_SHOW, LoginController::currentUser()->getSession());
                if(mContent->contentConfiguration()->showKeyDialogOnPlay() && showDialog)
                {
                    emit show3PDDPlayDialog(mContent->contentConfiguration()->productId());
                }
                else
                    emit show3PDDPlayDialogComplete();
            }

            void PlayFlow::onPDLCDownloadingCheck()
            {
                QString activeChildContent;

                //if we are downloading any of this entitlement's child PDLC/expansions, warn the user before launching.
                //if not, just continue with the launch. Need to check the child list that allows multiple parents
                //in case more than one version of the game is installed.
                foreach(EntitlementRef entRef, mContent->children())
                {
                    if(!entRef.isNull())
                    {
                        if(entRef->contentConfiguration()->isPDLC() &&
                            entRef->localContent()->installFlow() && 
                            entRef->localContent()->installFlow()->isActive() &&
                            entRef->localContent()->installFlow()->getFlowState() != Downloader::ContentInstallFlowState::kPaused &&
                            entRef->localContent()->installFlow()->getFlowState() != Downloader::ContentInstallFlowState::kPausing)
                        {
                            activeChildContent += entRef->contentConfiguration()->productId();
                            activeChildContent += "; ";
                        }
                    }
                }

                if(!activeChildContent.isEmpty())
                {
                    ORIGIN_LOG_WARNING << "User attempting to launch [" << mContent->contentConfiguration()->productId() + "] although child content is active: " << activeChildContent;
                    emit showPDLCDownloadingWarning(mContent->contentConfiguration()->productId());
                }
                else
                {
                    emit pdlcDownloadCheckComplete();
                }
            }

            void PlayFlow::onTrialCheck()
            {
                using namespace Origin::Services;

                if(mContent->localContent()->isOnlineRequiredForFreeTrial())
                {
                    emit showOnlineRequiredForTrialDialog(mContent->contentConfiguration()->productId());
                }
                else
                {
                    const bool startedFreeTrial = mContent->contentConfiguration()->terminationDate().isValid();
                    const QString alternateUrl = mContent->contentConfiguration()->gameLauncherUrl();
                    const bool hasCustomMessage = (bool)mContent->contentConfiguration()->gameLaunchMessage().count();
                    switch (mContent->contentConfiguration()->itemSubType())
                    {
                    case Publishing::ItemSubTypeLimitedTrial:
                    case Publishing::ItemSubTypeLimitedWeekendTrial:
                        if (!startedFreeTrial && alternateUrl.isEmpty() && !hasCustomMessage)
                        {
                            // show the 'Starting Timed Trial' dialog
                            emit showStartingLimitedTrialDialog(mContent->contentConfiguration()->productId());
                        }
                        else
                        {
                            emit trialCheckComplete();
                        }
                        break;
                    case Publishing::ItemSubTypeTimedTrial_Access:
                    case Publishing::ItemSubTypeTimedTrial_GameTime:
                        if (mContent->localContent()->hasEarlyTrialStarted() == false && !hasCustomMessage)
                        {
                            // Show the 'Start Trial' dialog
                            emit showStartingTimedTrialDialog(mContent->contentConfiguration()->productId());
                        }
                        else
                        {
                            emit trialCheckComplete();
                        }
                        break;
                    default:
                        emit trialCheckComplete();
                        break;
                    }
                }
            }

            void PlayFlow::onFirstLaunchMessageCheck()
            {
                GamesController* gamesController = GamesController::currentUserGamesController();
                if (gamesController && mContent->contentConfiguration()->gameLaunchMessage().count())
                {
                    // todo: 0 is the current id of the first launch string. When we are ready to scale, if the id is different than one stored, compare against last id.
                    const QString idOfLastMessageShown = gamesController->firstLaunchMessageShown(mContent->contentConfiguration()->productId());
                    if (idOfLastMessageShown.isEmpty())
                    {
                        emit showFirstLaunchMessage();
                    }
                    else
                    {
                        emit firstLaunchMessageCheckComplete();
                    }
                }
                else
                {
                    emit firstLaunchMessageCheckComplete();
                }
            }

            bool PlayFlow::setLauncherByTitle(const QString& launcherTitle)
            {
                bool bIs64BitOS = EnvUtils::GetIS64BitOS();

                // As far as launchers are concerned, if this is XP or earlier we only support 32 bit
                if (EnvUtils::GetISWindowsXPOrOlder())
                    bIs64BitOS = false;


                // Multi-launcher selection
                Downloader::LocalInstallProperties* pLocalProps = mContent->localContent()->localInstallProperties();
                if (pLocalProps)
                {
                    Engine::Content::tLaunchers launchers = pLocalProps->GetLaunchers();
                    if (launchers.size() > 0)
                    {
                        QString locale = readSetting(Services::SETTING_LOCALE);
                        for (Engine::Content::tLaunchers::iterator it = launchers.begin(); it != launchers.end(); it++)
                        {
                            // Only add the file to the list if it exists.
                            Engine::Content::LauncherDescription& launcherDescription = *it;

                            //this probably isn't needed since we had to retrieve the launchertitle from contentconfiguration::launchTitleList() which also does
                            //the OS check but just to be doubly sure...
                            if (launcherDescription.mbRequires64BitOS && !bIs64BitOS)
                            {
                                ORIGIN_LOG_DEBUG << "Launcher requires 64 bit OS but current OS is not 64 bit.";
                                continue;
                            }

                            const bool bIsTimedTrial = mContent->contentConfiguration()->itemSubType() == Services::Publishing::ItemSubTypeTimedTrial_Access || mContent->contentConfiguration()->itemSubType() == Services::Publishing::ItemSubTypeTimedTrial_GameTime;
                            if (launcherDescription.mbIsTimedTrial != bIsTimedTrial)
                            {
                                ORIGIN_LOG_DEBUG << "Launcher trial setting does not match offer trial setting. Launcher requires trial setting to match.";
                                continue;
                            }

                            QString launcherDescriptionTitle;

                            if (launcherDescription.mLanguageToCaptionMap.contains (locale))
                            {
                                launcherDescriptionTitle = launcherDescription.mLanguageToCaptionMap[locale];
                            }

                            if (launcherDescriptionTitle.isEmpty()) //this locale isn't supported by the game, so fall back on en_US or en_GB
                            {
                                ORIGIN_LOG_EVENT << "Looking for EN";
                                if (launcherDescription.mLanguageToCaptionMap.contains("en_US"))
                                {
                                    launcherDescriptionTitle = launcherDescription.mLanguageToCaptionMap["en_US"];
                                }
                                if (launcherDescriptionTitle.isEmpty() && launcherDescription.mLanguageToCaptionMap.contains("en_GB"))
                                {
                                    launcherDescriptionTitle = launcherDescription.mLanguageToCaptionMap["en_GB"];
                                }
                                //no en_US or en_GB; use the first one
                                if (launcherDescriptionTitle.isEmpty())
                                {
                                    ORIGIN_LOG_EVENT << "EN not found, use first one";
                                    launcherDescriptionTitle = launcherDescription.mLanguageToCaptionMap.begin().value();
                                }
                            }

                            if (launcherDescriptionTitle == launcherTitle)
                            {
                                // this is the one, use it
                                // EBIBUGS-27127: if the user has specified an execute path override, use that instead 
                                // of path specified in DiP manifest, but keep parameters/elevation settings
                                const QString overrideExecutePath = mContent->contentConfiguration()->overrideExecutePath();
                                mLauncher = overrideExecutePath.isEmpty() ? launcherDescription.msPath : overrideExecutePath;
                                mLaunchParameters += " " + launcherDescription.msParameters;
                                mbLaunchElevated = launcherDescription.mbExecuteElevated;
                                ORIGIN_LOG_EVENT << "Launcher selected. Title: " << launcherTitle << " productId: " << mContent->contentConfiguration()->productId() << " launcher: " << mLauncher << " launchParameters: " << mLaunchParameters << "executeElevated: " << mbLaunchElevated;
                                return true;
                            }
                        }
                    }
                }

                ORIGIN_LOG_ERROR << "Unable to find launcher associated with launch title: " << launcherTitle;
                return false;
            }
        } //namespace Content
    } //namespace Engine
} //namespace Origin



