///////////////////////////////////////////////////////////////////////////////
// LogoutExitFlow.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "flows/LogoutExitFlow.h"
#include "widgets/logoutExit/source/LogoutExitViewController.h"
#include "widgets/social/source/SocialViewController.h"
#include "flows/FlowResult.h"
#include "flows/ClientFlow.h"
#include "flows/SingleLoginFlow.h"
#include "flows/MainFlow.h"
#include "flows/RTPFlow.h"
#include "OriginApplication.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/platform/PlatformJumplist.h"
#include "engine/ito/InstallFlowManager.h"
#include "engine/content/CloudContent.h"
#include "engine/content/ContentController.h"
#include "engine/content/ContentConfiguration.h"
#include "services/settings/SettingsManager.h"
#include "TelemetryAPIDLL.h"
#include "NativeBehaviorManager.h"
#include "engine/igo/IGOController.h"
#include "engine/dirtybits/DirtyBitsClient.h"
#include "engine/cloudsaves/RemoteSyncManager.h"
#include "engine/social/SocialController.h"
#include "engine/multiplayer/InviteController.h"

//NOTE: if you add any checks to the logout flow and it is a check that should block the user from logging out, please also update canLogout()
/*

    Logout/Exit flow is as follows:
        1. Check for an in-progress installation
        2. Check for the user playing a game
        3. Check for ODT editor active.
        4. Check for and clear any pending cloud saves, if cloud save exists, show the cloud sync in a dialog and allow cancelling
        5. application is minimized to the taskbar
        6. Pause all install flows if the user decides to exit / logout
        7. Logout / exit

*/

// Anonymous namespace for some helper functions
namespace
{
    /// \return String representation of given ClientLogoutExitReason enum value.
    /// If the value isn't an expected enum value, the string "Not mapped %d" with the
    /// integer-cast value is returned.
    QString exitReasonToString(Origin::Client::ClientLogoutExitReason reason)
    {
#define RETURN_LOGOUT_EXIT_REASON_STRING(exitReasonEnum) if (Origin::Client::exitReasonEnum == reason) { return #exitReasonEnum; }
        RETURN_LOGOUT_EXIT_REASON_STRING(ClientLogoutExitReason_Logout);
        RETURN_LOGOUT_EXIT_REASON_STRING(ClientLogoutExitReason_Logout_Silent);
        RETURN_LOGOUT_EXIT_REASON_STRING(ClientLogoutExitReason_Logout_Silent_BadEncryptedToken);
        RETURN_LOGOUT_EXIT_REASON_STRING(ClientLogoutExitReason_Logout_Silent_CredentialsChanged);
        RETURN_LOGOUT_EXIT_REASON_STRING(ClientLogoutExitReason_Logout_onLoginError);
        RETURN_LOGOUT_EXIT_REASON_STRING(ClientLogoutExitReason_Exit);
        RETURN_LOGOUT_EXIT_REASON_STRING(ClientLogoutExitReason_Exit_NoConfirmation);
        RETURN_LOGOUT_EXIT_REASON_STRING(ClientLogoutExitReason_Restart);
#undef RETURN_LOGOUT_EXIT_REASON_STRING

        // Enum value not mapped. Return the integer string representation
        return QString("Not mapped: %1").arg(reason);
    }
}

namespace Origin
{
    namespace Client
    {
        using namespace Origin::Engine::CloudSaves;

        LogoutExitFlow::LogoutExitFlow(ClientLogoutExitReason aReason)
            : mReason(aReason)
            , mIsActuallyLoggingOut (false)
        {
        }

        void LogoutExitFlow::start()
        {
            ORIGIN_LOG_EVENT << "starting LogoutExit flow";
            mLogoutExitViewController.reset(new LogoutExitViewController());

            // Start with the first step of the flow...check for installations
            checkForInstallations();
        }

        ///
        /// \brief checks whether we are currently performing an operation that would block the user from logging out
        /// 1.  currently installing game or in middle of ITO Flow
        /// 2.  if playing a game
        ///  if either of above is happening, function will return false
        ///
        bool LogoutExitFlow::canLogout ()
        {
            //1.  check and see if installing or ITO
            Origin::Engine::Content::ContentController * c = Origin::Engine::Content::ContentController::currentUserContentController();
            // Just in case we've been activated at some bad time where there's
            // no current user, or they haven't been set up.
            if (c)
            {
                Engine::Content::EntitlementRefList installing = c->entitlementByState(0, Engine::Content::LocalContent::Installing, 0, Engine::Content::AllContent);
                if (installing.size())  //there's something installing
                    return false;
            }

            if (OriginApplication::instance().isITE())      //in middle of ITO flow
                return false;


            //2. check and see if a game is being played -- for logout doesn't matter if its SDK or non-SDK
            if (c && c->isPlaying())
            {
                return false;
            }

            //3. if the user is in the middle of cloudsync --
            //for our current purposes, we don't need to wait; since the authtoken is invalid, cloud sync won't be able to release the lock
            //so it will just fail

            //here if nothing is blocking our way for logging out
            return true;
        }

        void LogoutExitFlow::checkForInstallations()
        {
            QString gamename;

            Origin::Engine::Content::ContentController * c = Origin::Engine::Content::ContentController::currentUserContentController();
            // Just in case we've been activated at some bad time where there's
            // no current user, or they haven't been set up.
            if (c)
            {
                Engine::Content::EntitlementRefList installing = c->entitlementByState(0, Engine::Content::LocalContent::Installing, 0, Engine::Content::AllContent);

                if (installing.size())
                {
                    gamename = installing.first()->contentConfiguration()->displayName();
                }
                else if (OriginApplication::instance().isITE())
                {
                    // We are currently in the ITE flow
                    gamename = Engine::InstallFlowManager::GetInstance().GetVariable("GameName").toString();
                }

                if (!gamename.isEmpty())
                {
                    // Prompt the user to exit the installation
                    if (mReason == ClientLogoutExitReason_Logout)
                    {
                        // Logout while installing is not allowed
                        mLogoutExitViewController->promptCancelInstallationOnLogout(gamename);
                        logoutExitCancelled ();
                    }
                    else if (mReason == ClientLogoutExitReason_Exit ||
                             mReason == ClientLogoutExitReason_Exit_NoConfirmation ||
                             mReason == ClientLogoutExitReason_Restart)
                    {
                        // Exit while installing is not allowed
                        mLogoutExitViewController->promptCancelInstallationOnExit(gamename);
                        logoutExitCancelled ();
                    }
                }
                else
                {
                    // Not installing anything
                    // NEXT STEP - check for playing a game
                    checkForPlayingGame();
                }
            }
        }

        bool LogoutExitFlow::isOriginRequiredGameBeingPlayed(QString& gamename, const QList<Origin::Engine::Content::EntitlementRef>& playing)
        {
            // CHECK FOR SDK
            Origin::Engine::Content::ContentController * controller = Origin::Engine::Content::ContentController::currentUserContentController();
            ORIGIN_ASSERT(controller);
            if ( controller )
            {
                // JS - leaving these contentIds in place 'cause of SDK
                QStringList productIds = controller->entitlementsConnectedToSDK();

                // We might have more than one SDK-enabled game playing at the same time but reporting the first one must be enough
                for ( QStringList::const_iterator i = productIds.begin(); i != productIds.end(); ++i )
                {
                    Engine::Content::EntitlementRef entitlement = controller->entitlementByProductId(*i);
                    if ( entitlement && entitlement->contentConfiguration() )
                    {
                        gamename = entitlement->contentConfiguration()->displayName();
                        return true;
                    }
                }
            }

            // CHECK FOR PI
            foreach(Origin::Engine::Content::EntitlementRef ent, playing)
            {
                if((ent.isNull() == false && ent->localContent() && ent->localContent()->installFlow())
                        // Is a incomplete downloaded PI game - that's currently being played.
                        && (ent->localContent()->installFlow()->isDynamicDownload()))
                {
                    gamename = ent->contentConfiguration()->displayName();
                    return true;
                }
            }
            return false;
        }

        void LogoutExitFlow::checkForPlayingGame()
        {
            // Make sure there are no conflicting join operations
            // If the user is trying to join a game we don't want to close Origin
            // Related to https://developer.origin.com/support/browse/EBIBUGS-28285
            Origin::Engine::Social::SocialController *socialController = Engine::LoginController::currentUser()->socialControllerInstance();
            Origin::Engine::MultiplayerInvite::JoinOperation *existingOp = socialController->multiplayerInvites()->activeJoinOperation();
            
            if (existingOp)
            {
                logoutExitCancelled();
                return;
            }

            QString gamename;
            Origin::Engine::Content::ContentController * c = Origin::Engine::Content::ContentController::currentUserContentController();
            QList<Origin::Engine::Content::EntitlementRef> playing = c->entitlementByState(0, Origin::Engine::Content::LocalContent::Playing);

            if (isOriginRequiredGameBeingPlayed(gamename, playing))
            {
                if (mReason == ClientLogoutExitReason_Logout_Silent_BadEncryptedToken || mReason == ClientLogoutExitReason_Logout_Silent_CredentialsChanged)
                    mLogoutExitViewController->promptPlayingGameOnLogoutBadCredentials();
                else
                    mLogoutExitViewController->promptCannotCloseSDKGamePlaying(gamename);
                logoutExitCancelled();
                return;
            }

            if (playing.size())
            {
                gamename = playing.first()->contentConfiguration()->displayName();
            }

            if (!gamename.isEmpty())
            {
                switch(mReason)
                {
                    case ClientLogoutExitReason_Logout:
                    case ClientLogoutExitReason_Logout_Silent:
                        // Logout while playing a game is not allowed
                        mLogoutExitViewController->promptPlayingGameOnLogout(gamename);
                        logoutExitCancelled();
                        break;
                    case ClientLogoutExitReason_Logout_Silent_BadEncryptedToken:
                        //trying to log them out as a result of user wanting to go Online (and requires logging back in)
                        mLogoutExitViewController->promptPlayingGameOnLogoutBadCredentials();
                        logoutExitCancelled();
                        break;
                    case ClientLogoutExitReason_Exit:
                        if (mLogoutExitViewController->promptPlayingGameOnExit(gamename))
                        {
                            // NEXT STEP - check for ODT
                            checkForODT();
                        }
                        else
                        {
                            logoutExitCancelled();
                        }
                        break;
                    case ClientLogoutExitReason_Exit_NoConfirmation:
                    case ClientLogoutExitReason_Restart:
                        checkForODT();
                        break;
                    default:
                        logoutExitCancelled();
                        break;
                }
            }
            else
            {
                // Not playing a game
                // NEXT STEP - check for ODT
                checkForODT();
            }
        }

        void LogoutExitFlow::checkForODT()
        {
            // Ask the user if he wants to close the ODT window prior to logout
            // make sure clientFlow exists tho (could be forced logged out from losing connection while in NUX Flow)
            if(!ClientFlow::instance() || ClientFlow::instance()->closeDeveloperTool())
            {
                // ODT window has been closed.
                //now check for cloud syncs
                checkForCloudSaves ();
            }
            else
            {
                logoutExitCancelled();
            }
        }

        void LogoutExitFlow::checkForCloudSaves()
        {
            if (RemoteSyncManager::instance()->syncBeingPerformed())
            {
                bool showProgress = true;

                //1.check and see if this is an exit or restart
                //2.check and see if it's a pull (i.e. Launching... dialog is showing), and if so, bring up the dialog that prompts the user
                // whether they should cancel the exit or exit anyways
                if (mReason == ClientLogoutExitReason_Exit || mReason == ClientLogoutExitReason_Exit_NoConfirmation || mReason == ClientLogoutExitReason_Restart)
                {
                    bool pulling = false;

                    //look thru the list of syncs and see if any of them are pull
                    RemoteSyncManager::RemoteSyncList *syncList = RemoteSyncManager::instance()->remoteSyncInProgressList ();
                    if (!syncList->isEmpty ())
                    {
                        RemoteSyncManager::RemoteSyncList::const_iterator it;
                        for (it = syncList->constBegin(); it != syncList->constEnd(); it++)
                        {
                            RemoteSync* syncContent = it.value();
                            if (syncContent)
                            {
                                if (syncContent->direction () == RemoteSync::PullFromCloud)
                                {
                                    pulling = true;
                                    break;
                                }
                            }
                        }
                    }

                    if (pulling)
                    {
                        showProgress = false;
                        if (!mLogoutExitViewController->promptCancelCloudSavesonExit ()) //user confirmed Exit Anyways
                        {
                            //cancelled exit
                            logoutExitCancelled ();
                            return;
                        }
                    }
                }

                if (showProgress)
                {
                    //connect signal when all syncs have finished
                    ORIGIN_VERIFY_CONNECT(RemoteSyncManager::instance(),
                                          SIGNAL(lastSyncFinished()),
                                          mLogoutExitViewController.data(),
                                          SLOT(onAllCloudSyncsComplete()));

                    //connect signal when all syncs have finished
                    ORIGIN_VERIFY_CONNECT(mLogoutExitViewController.data(),
                                          SIGNAL (lastSyncFinished()),
                                          this,
                                          SLOT (beginLogoutExit()));

                    //closed the cloud sync so cancel exit flow
                    ORIGIN_VERIFY_CONNECT(mLogoutExitViewController.data(),
                                          SIGNAL (cancelLogout()),
                                          this,
                                          SLOT (onCancelLogoutFromCloudSync()));

                    //hit "Cancel" on the cloud sync progress dialog, so cancel sync and proceed with logout
                    ORIGIN_VERIFY_CONNECT(mLogoutExitViewController.data(),
                                          SIGNAL (cloudSyncCancelled()),
                                          this,
                                          SLOT (onCancelCloudSync()));
                }

                //minimze the client
                if (ClientFlow::instance())
                {
                    ClientFlow::instance()->minimizeClientWindow(true);
                }

                if (showProgress)
                {
                    // bring up the cloud sync progress with cancel button on it
                    if (!mLogoutExitViewController->showCancelCloudSaves())
                    {
                        //for whatever reason, the cloud sync progress dialog wasn't created so proceed with logout/exit
                        beginLogoutExit();
                    }
                }
                else    //"Launching..." dialog was already showing, they decided to exit anyways
                {
                    beginLogoutExit ();
                }
            }
            else
            {
                // No syncs in-progress or already completed
                beginLogoutExit();
            }
        }

        /// user chose to cancel out of cloud sync, so proceed with logout
        void LogoutExitFlow::onCancelCloudSync()
        {
            // Cancel syncs and wait 2 seconds
            RemoteSyncManager::instance()->cancelRemoteSyncs(2000);
            //we only need to initiate the cancel because when cancel is done, it will emit lastSyncFinished() which we're already listening for
        }

        /// user closed the cloud sync dialog, so cancel the logout/exit flow
        void LogoutExitFlow::onCancelLogoutFromCloudSync()
        {
            //we need to restore the client window that we minimized
            if (ClientFlow::instance())
            {
                ClientFlow::instance()->minimizeClientWindow(false);
            }
            logoutExitCancelled();
        }

        void LogoutExitFlow::beginLogoutExit()
        {
            ORIGIN_VERIFY_DISCONNECT(RemoteSyncManager::instance(), SIGNAL(lastSyncFinished()), this, SLOT(beginLogoutExit()));

            //initiate the actual logout/exit process
            //minimize the client and social windows
            if (ClientFlow::instance())
            {
                ClientFlow::instance()->minimizeClientToTray();
                // Do we need to handle pop out social windows here??
                if(ClientFlow::instance()->socialViewController() && ClientFlow::instance()->socialViewController()->chatWindowManager())
                {
                    ClientFlow::instance()->socialViewController()->chatWindowManager()->hideMainChatWindow();
                }
            }

            // NEXT STEP - pause all active install flows so we don't lose progress
            pauseActiveInstallFlows();
        }

        void LogoutExitFlow::clearRememberMe ()
        {
            QString envname = Origin::Services::readSetting(Services::SETTING_EnvironmentName, Services::Session::SessionRef());

            // clear remember me cookie
            const Services::Setting &rememberMeSetting = envname == Services::SETTING_ENV_production ? Services::SETTING_REMEMBER_ME_PROD : Services::SETTING_REMEMBER_ME_INT;
            Services::writeSetting(rememberMeSetting, Services::SETTING_INVALID_REMEMBER_ME);

            //clear who the remember me belongs to
            const Origin::Services::Setting &rememberMeUserIdSetting = envname == Services::SETTING_ENV_production ? Services::SETTING_REMEMBER_ME_USERID_PROD : Services::SETTING_REMEMBER_ME_USERID_INT;
            Services::writeSetting(rememberMeUserIdSetting, Services::SETTING_INVALID_REMEMBER_ME_USERID);

            //setup a slot to catch if the settings file fails to write so we can log telemetry.
            ORIGIN_VERIFY_CONNECT(
                Origin::Services::SettingsManager::instance(),
                SIGNAL(localPerAccountSettingFileError(const QString, const QString, const bool, const bool, const unsigned int, const unsigned int, const QString, const QString)),
                Origin::Services::SettingsManager::instance(),
                SLOT(reportSettingFileError(const QString&, const QString&, const bool, const bool, const unsigned int, const unsigned int, const QString&, const QString&)));
            //Make sure the cookies get written out to disk.
            Origin::Services::SettingsManager::instance()->sync();
            ORIGIN_VERIFY_DISCONNECT(
                Origin::Services::SettingsManager::instance(),
                SIGNAL(localPerAccountSettingFileError(const QString, const QString, const bool, const bool, const unsigned int, const unsigned int, const QString, const QString)),
                Origin::Services::SettingsManager::instance(),
                SLOT(reportSettingFileError(const QString&, const QString&, const bool, const bool, const unsigned int, const unsigned int, const QString&, const QString&)));

        }

        void LogoutExitFlow::doLogoutTasks()
        {

            // Send telemetry before clearing Remember Me cookie. We want to know if this session had auto-login enabled or not.
            sendTelemetry();

            if(mReason != ClientLogoutExitReason_Logout_Silent_BadEncryptedToken)
            {
                clearRememberMe();
            }

            if (MainFlow::instance()->rtpFlow())
            {
                MainFlow::instance()->rtpFlow()->clearManualOnlineSlot();
            }

            // Clear the start up tab setting
            OriginApplication::instance().SetExternalLaunchStartupTab(TabNone);
            OriginApplication::instance().SetServerStartupTab(TabNone);

            Services::PlatformJumplist::clear_jumplist();

            if (MainFlow::instance()->singleLoginFlow())
            {
                MainFlow::instance()->singleLoginFlow()->closeSingleLoginDialogs();
            }
        }

        void LogoutExitFlow::pauseActiveInstallFlows()
        {
            //here if we've gone thru all the checks/confirmation dialogs
            mIsActuallyLoggingOut = true;

            Origin::Engine::Content::ContentController * c = Origin::Engine::Content::ContentController::currentUserContentController();

            if (c)
            {
                // NEXT STEP - delete all the install flows to avoid race conditions
                ORIGIN_VERIFY_CONNECT(c, SIGNAL(lastActiveFlowPaused(bool)), this, SLOT(cleanupInstallFlows()));

                c->suspendAllActiveFlows(true, 15000); // TODO: is 15 seconds reasonable?  Should we show a 'Please wait, Origin is shutting down...' GUI?
            }
        }

        void LogoutExitFlow::doExitTasks()
        {
            sendTelemetry();

            //clear the ITE commandline
            Origin::Services::SettingsManager::instance()->unset(Origin::Services::SETTING_COMMANDLINE);
        }

        void LogoutExitFlow::cleanupInstallFlows()
        {
            Origin::Engine::Content::ContentController * c = Origin::Engine::Content::ContentController::currentUserContentController();
            if (c)
            {
                // NEXT STEP - prompt the user for logout / exit
                ORIGIN_VERIFY_CONNECT(c, SIGNAL(lastContentInstallFlowDeleted()), this, SLOT(logoutExitSuccess()));

                c->deleteAllContentInstallFlows();
            }
        }

        void LogoutExitFlow::logoutExitSuccess()
        {
            Origin::Engine::Content::ContentController *contentController = Origin::Engine::Content::ContentController::currentUserContentController();

            Origin::Engine::DirtyBitsClient::finish();

            GetTelemetryInterface()->Metric_ACTIVE_TIMER_END(true);
            // We want to stop the running timer if the user logs out
            GetTelemetryInterface()->RUNNING_TIMER_CLEAR();

            //  TELEMETRY:  Record number of user entitlements on logout
            int numEntitlements = contentController->nucleusEntitlementController()->numberOfEntitlements();
            int numGameEntitlements = contentController->baseGamesController()->numberOfGames();
            int numNonOriginEntitlements = contentController->numberOfNonOriginEntitlements();

            // EBIBUGS-28393
            // If we never successfully refreshed, report entitlement/game count as -1
            if (!contentController->baseGamesController()->lastFullRefresh().isValid())
            {
                numEntitlements = -1;
                numGameEntitlements = -1;
            }

            int favEntitlements = Services::readSetting(Services::SETTING_FavoriteProductIds).toStringList().length();
            int hidEntitlements = Services::readSetting(Services::SETTING_HiddenProductIds).toStringList().length();

            QStringList nonOriginIds = contentController->nonOriginEntitlementIds();

            // do not report the number of entitlements if we haven't managed to load them
            if(contentController->initialEntitlementRefreshCompleted())
            {
                GetTelemetryInterface()->Metric_ENTITLEMENT_GAME_COUNT_LOGOUT(numEntitlements, numGameEntitlements, favEntitlements, hidEntitlements, numNonOriginEntitlements);
                GetTelemetryInterface()->Metric_NON_ORIGIN_GAME_COUNT_LOGOUT(numNonOriginEntitlements);
            }

            for (int i = 0; i < nonOriginIds.length(); ++i)
            {
                GetTelemetryInterface()->Metric_NON_ORIGIN_GAME_ID_LOGOUT(nonOriginIds.at(i).toUtf8());
            }

            // Reset value since it is based on user's entitlements and may vary between users.
            Services::writeSetting(Services::SETTING_AddonPreviewAllowed, false, Services::Session::SessionService::currentSession());

            // EBIBUGS-20535
            // If there are web pages still loading, we want to remove native behavior from them.
            // QWebPage::loadFinished returns a "success" signal if the web page is deleted immediately after loading successfully, which is
            // causing crashes in our NativeBehaviorManager.  The right fix would be a Qt patch which is too risky for a beta opt-in.
            NativeBehaviorManager::shutdown();

            if (mReason == ClientLogoutExitReason_Logout ||
                    mReason == ClientLogoutExitReason_Logout_Silent ||
                    mReason == ClientLogoutExitReason_Logout_Silent_BadEncryptedToken ||
                    mReason == ClientLogoutExitReason_Logout_Silent_CredentialsChanged)
            {
                using namespace Services;

                ORIGIN_LOG_EVENT << "Logout reason: " << mReason;

                doLogoutTasks();

                // Signal logout
                //emit (onLogoutSuccess());
                if (mReason == ClientLogoutExitReason_Logout_Silent_BadEncryptedToken)
                {
                    emit finished(LogoutExitFlowResult(LogoutExitFlowResult::USER_FORCED_LOGGED_OUT_BADTOKEN));
                }
                else if (mReason == ClientLogoutExitReason_Logout_Silent_CredentialsChanged)
                {
                    emit finished(LogoutExitFlowResult(LogoutExitFlowResult::USER_FORCED_LOGGED_OUT_CREDENTIALCHANGE));
                }
                else
                {
                    emit finished(LogoutExitFlowResult(LogoutExitFlowResult::USER_LOGGED_OUT));
                }
            }
            else if (mReason == ClientLogoutExitReason_Exit || mReason == ClientLogoutExitReason_Exit_NoConfirmation)
            {
                doExitTasks();

                // Just signal exit
                //emit (onExitSuccess());
                emit finished(LogoutExitFlowResult(LogoutExitFlowResult::USER_EXITED));
            }
            else if (mReason == ClientLogoutExitReason_Restart)
            {
                doExitTasks();

                // Just signal exit
                //emit (onExitSuccess());
                emit finished(LogoutExitFlowResult(LogoutExitFlowResult::USER_RESTART));
            }

            // END OF FLOW
        }

        //
        void LogoutExitFlow::logoutExitCancelled ()
        {
            mIsActuallyLoggingOut = false;
            emit finished(LogoutExitFlowResult(LogoutExitFlowResult::USER_CANCELLED));
        }

        void LogoutExitFlow::sendTelemetry()
        {
            QString reasonString(exitReasonToString(mReason));
            GetTelemetryInterface()->Metric_LOGOUT(reasonString.toUtf8().constData());

            Services::Session::SessionRef session = Services::Session::SessionService::currentSession();
            QString envname = Origin::Services::readSetting(Origin::Services::SETTING_EnvironmentName, Services::Session::SessionRef());
            bool auto_login = 0;

            if(envname != Origin::Services::SETTING_ENV_production && !Origin::Services::readSetting(Origin::Services::SETTING_REMEMBER_ME_INT).toString().isEmpty() && Origin::Services::readSetting(Origin::Services::SETTING_REMEMBER_ME_INT).toString() != Origin::Services::SETTING_INVALID_REMEMBER_ME)
                auto_login = 1;

            else if(envname == Origin::Services::SETTING_ENV_production && !Origin::Services::readSetting(Origin::Services::SETTING_REMEMBER_ME_PROD).toString().isEmpty() && Origin::Services::readSetting(Origin::Services::SETTING_REMEMBER_ME_PROD).toString() != Origin::Services::SETTING_INVALID_REMEMBER_ME)
                auto_login = 1;

            //  TELEMETRY:  Send the settings state
            EbisuTelemetryAPI::DefaultTabSetting defaultTab = EbisuTelemetryAPI::Decide;
            const QString defaultSetting = Services::readSetting(Services::SETTING_DefaultTab, session);
            if(defaultSetting.compare("myorigin", Qt::CaseInsensitive) == 0)
                defaultTab = EbisuTelemetryAPI::MyOrigin;
            else if(defaultSetting.compare("mygames", Qt::CaseInsensitive) == 0)
                defaultTab = EbisuTelemetryAPI::MyGames;
            else if(defaultSetting.compare("store", Qt::CaseInsensitive) == 0)
                defaultTab = EbisuTelemetryAPI::Store;
            else if(defaultSetting.compare("decide", Qt::CaseInsensitive) == 0)
                defaultTab = EbisuTelemetryAPI::Decide;

            EbisuTelemetryAPI::IGOEnableSetting igoEnable = EbisuTelemetryAPI::IGODisabled;
            if (Origin::Engine::IGOController::isEnabled())
            {
                igoEnable = Origin::Engine::IGOController::isEnabledForAllGames() ? EbisuTelemetryAPI::IGOEnableGamesAll : EbisuTelemetryAPI::IGOEnableGamesSupported;
            }

            GetTelemetryInterface()->Metric_SETTINGS(
                auto_login,
                Services::readSetting(Services::SETTING_AUTOPATCH, session),
                Services::readSetting(Services::SETTING_RUNONSYSTEMSTART, session),
                Services::readSetting(Services::SETTING_AUTOUPDATE, session),
                Services::readSetting(Services::SETTING_BETAOPTIN, session),
                Services::readSetting(Services::SETTING_EnableCloudSaves, session),
                Services::readSetting(Services::SETTING_HW_SURVEY_OPTOUT, session),
                defaultTab,
                igoEnable
            );
            GetTelemetryInterface()->Metric_PRIVACY_SETTINGS();
            GetTelemetryInterface()->Metric_QUIETMODE_ENABLED(Services::readSetting(Services::SETTING_DISABLE_PROMO_MANAGER),
                                                              Services::readSetting(Services::SETTING_IGNORE_NON_MANDATORY_GAME_UPDATES),
                                                              Services::readSetting(Services::SETTING_SHUTDOWN_ORIGIN_ON_GAME_FINISHED));

            // If the user is exiting the client, send the "end session"
            // telemetry event now, before we reset the user's telemetry
            // privacy settings. Since this is a non-critical event, the
            // event will never be sent after user logout, so we attempt to
            // send the telemetry now (since we know their privacy settings).
            if (mReason == ClientLogoutExitReason_Exit ||
                    mReason == ClientLogoutExitReason_Exit_NoConfirmation ||
                    mReason == ClientLogoutExitReason_Restart)
            {
                ORIGIN_LOG_EVENT << "Tel app end";
                GetTelemetryInterface()->Metric_APP_END();
            }

            // log how many games still need an update at logout
            Origin::Engine::Content::ContentController * c = Origin::Engine::Content::ContentController::currentUserContentController();
            QList<Origin::Engine::Content::EntitlementRef> eList = c->entitlements();
            int updateCount = 0;
            for (int i = 0; i < eList.size(); ++i)
            {
                if (eList.at(i)->localContent()->updateAvailable())
                    updateCount++;
            }
            GetTelemetryInterface()->Metric_GAMEPROPERTIES_UPDATE_COUNT_LOGOUT(updateCount);
        }
    }
}
