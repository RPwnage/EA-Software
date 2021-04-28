///////////////////////////////////////////////////////////////////////////////
// MainFlow.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "MainFlow.h"
#include "FirstLaunchFlow.h"
#include "ContentFlowController.h"
#include "LoginFlow.h"
#include "ClientFlow.h"
#include "LogoutExitFlow.h"
#include "SingleLoginFlow.h"
#include "NonOriginGameFlow.h"
#include "MOTDFlow.h"
#include "RTPFlow.h"
#include "SSOFlow.h"
#include "ITOFlow.h"
#include "PendingActionFlow.h"
#include "OriginApplication.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "services/log/LogService.h"
#include "services/network/GlobalConnectionStateMonitor.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/network/NetworkAccessManagerFactory.h"
#include "EbisuSystemTray.h"
#include "engine/content/ContentController.h"
#include "engine/social/SocialController.h"
#include "utilities/CommandLine.h"
#include "services/heartbeat/Heartbeat.h"
#include "TelemetryAPIDLL.h"
#include "chat/OriginConnection.h"
#include "engine/content/LocalContent.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/updater/UpdateController.h"
#include <QtWebKitWidgets/QWebView>
#include "FeatureCalloutController.h"
#include "DirtyBitsTrafficDebugView.h"
#include "originwindow.h"
#include "tasks/RestartGameTask.h"

#if defined(ORIGIN_DEBUG)
#define ENABLE_FLOW_TRACE   0
#else
#define ENABLE_FLOW_TRACE   0
#endif

namespace Origin
{
    namespace Client
    {
        static MainFlow* mInstance = NULL;

        void MainFlow::create()
        {
            ORIGIN_LOG_EVENT << "create MainFlow";
            ORIGIN_ASSERT(!mInstance);
            if (!mInstance)
            {
                qRegisterMetaType<ClientFlowResult>("ClientFlowResult");
                mInstance = new MainFlow();
            }
        }

        void MainFlow::destroy()
        {
            ORIGIN_LOG_EVENT << "destroy MainFlow";
            ORIGIN_ASSERT(mInstance);
            if (mInstance)
            {
                delete mInstance;
                mInstance = NULL;
            }
        }

        MainFlow* MainFlow::instance()
        {
            return mInstance;
        }

        void MainFlow::start()
        {
            ORIGIN_LOG_DEBUG_IF(ENABLE_FLOW_TRACE) << "Main flow started";

            //connect app close
            ORIGIN_VERIFY_CONNECT(&OriginApplication::instance(), SIGNAL(gotCloseEvent()), this, SLOT(onGotAppCloseEvent()));

            startFirstLaunchFlow();
            startMOTDFlow();
            startContentFlowController();
            startRTPFlow(); //this needs to wait across multiple logins in case the user decides to login to another account, so we need to retain the info extracted from the original command line
            mFeatureCalloutController.reset(new FeatureCalloutController);
        }

        bool MainFlow::handleMessage(QString const& message)
        {
            if (message.isEmpty())
                return false;

            // Check whether we are exiting the client.
            if (!mLogoutExitFlow.isNull() && mLogoutExitFlow->isExiting())
                return false;

            bool bFwdAppFocus = !message.contains("-Origin_NoAppFocus", Qt::CaseInsensitive);  // If passing command line to existing instance then fwd the application focus by default unless disabled on command line
            bool bOffline = message.contains("-Origin_Offline", Qt::CaseInsensitive);
            bool bOnline = message.contains("-Origin_Online", Qt::CaseInsensitive);

            // These are for automation
            if (bOffline)
                Services::Network::GlobalConnectionStateMonitor::setSimulateIsInternetReachable(false);

            if (bOnline)
                Services::Network::GlobalConnectionStateMonitor::setSimulateIsInternetReachable(true);


            EbisuSystemTray* sysTray = EbisuSystemTray::instance();
            if (sysTray && bFwdAppFocus && PendingActionFlow::instance()->shouldShowPrimaryWindow())
            {
                sysTray->showPrimaryWindow();
            }

            if (message.contains("/contentIds:"))
            {
                //we set the command line from the auto-run, but we only want to check and kick off the flow
                //if the user is already logged in, otherwise if they are logged out and then log in, it will kick off the flow
                //again DT#15645
                if (Origin::Engine::LoginController::instance()->isUserLoggedIn())
                {
                    //MY: if we're already running ITE and we get another call to it, ignore it
                    if (OriginApplication::instance().isITE())
                    {
                        //tell windows it's handled
                        return true;
                    }
                    else
                    {
                        Origin::Services::writeSetting(Origin::Services::SETTING_COMMANDLINE, message, Origin::Services::Session::SessionService::currentSession());
                        emit(startITOviaCommandLine());
                    }
                }
                else
                {
                    //we're saving it out because user hasn't logged in yet and will be picked up after the login
                    Origin::Services::writeSetting(Origin::Services::SETTING_COMMANDLINE, message, Origin::Services::Session::SessionRef());

                    // if login dialog is already up, check to make sure the user isn't currently logging in
                    // and add the green banner with the ITO message.

                    //if its a newUser, means we are running through/ran through the registration, in that case we don't want to make the setLoginBanner call as that kicks us out of the ITO Flow
                    if (!mLoginFlow.isNull() && !mLoginFlow->isLoginRequestInProcess() && mLoginFlow->isLoginDialogShowing() && !newUser())
                    {
                        // up!
                        mLoginFlow->setLoginBanner(UIToolkit::OriginBanner::Info, tr("ebisu_client_login_ito").arg(ITOFlow::getGameTitleFromManifest()));
                    }
                }
            }

            return true;
        }

        RTPFlow* MainFlow::rtpFlow()
        {
            if (&mRTPFlow == NULL)
            {
                ORIGIN_LOG_ERROR << "&mRTPFlow is NULL!";
                return NULL;
            }

            return mRTPFlow.data();
        }

        ContentFlowController* MainFlow::contentFlowController()
        {
            return mContentFlowController.data();
        }

        FeatureCalloutController* MainFlow::featureCalloutController()
        {
            return mFeatureCalloutController.data();
        }

        void MainFlow::showDirtyBitsTrafficTool()
        {
            ORIGIN_LOG_ERROR << "Showing dirty bits window";
            if (mDirtyBitsTrafficTool.isNull())
            {
                ORIGIN_LOG_ERROR << "DirtyBitsTrafficTool is NULL - creating";
                mDirtyBitsTrafficTool.reset(new DirtyBitsTrafficDebugView);
                ORIGIN_VERIFY_CONNECT(mDirtyBitsTrafficTool.data(), SIGNAL(finished()), this, SLOT(onDirtyBitsTrafficToolClosed()));
            }
            mDirtyBitsTrafficTool->showWindow();
        }

        void MainFlow::onDirtyBitsTrafficToolClosed()
        {
            ORIGIN_LOG_ERROR << "DirtyBitsTrafficTool - closing";
            ORIGIN_VERIFY_DISCONNECT(mDirtyBitsTrafficTool.data(), SIGNAL(finished()), this, SLOT(onDirtyBitsTrafficToolClosed()));
            mDirtyBitsTrafficTool.reset(NULL);
        }


        void MainFlow::scheduleRestart( Origin::Engine::Content::EntitlementRef entitlement, bool forceUpdateGame, bool forceUpdateDLC, QString commandLineArgs )
        {
            const int RESTART_GAME_TIMEOUT = 30000;

            Origin::Client::Tasks::RestartGameTask * task = new Origin::Client::Tasks::RestartGameTask(entitlement, commandLineArgs, forceUpdateGame, forceUpdateDLC, RESTART_GAME_TIMEOUT);
            task->run();
        }

        void MainFlow::logout(ClientLogoutExitReason reason)
        {
            startLogoutExitFlow(reason);
        }

        void MainFlow::exit(ClientLogoutExitReason reason /*= ClientLogoutExitReason_Exit */)
        {
            startLogoutExitFlow(reason);
        }

        void MainFlow::restart(ClientLogoutExitReason reason /*= ClientLogoutExitReason_Restart */)
        {
            startLogoutExitFlow(reason);
        }

        //Use this call to reenable the Origin Client window
        void MainFlow::reactivateClient()
        {
            EbisuSystemTray* sysTray = EbisuSystemTray::instance();
            if (sysTray)
            {
                sysTray->showPrimaryWindow();
            }
        }

        void MainFlow::removeNonOriginGame(Origin::Engine::Content::EntitlementRef game)
        {
            if (!game.isNull() && game->contentConfiguration()->isNonOriginGame())
            {
                if (mNonOriginGameFlowsMap.contains(NonOriginGameFlow::OpRemove))
                {
                    mNonOriginGameFlowsMap.value(NonOriginGameFlow::OpRemove)->focus();
                }
                else
                {
                    NonOriginGameFlow* flow = new NonOriginGameFlow();
                    if (flow)
                    {
                        mNonOriginGameFlowsMap.insert(NonOriginGameFlow::OpRemove, flow);

                        ORIGIN_VERIFY_CONNECT(flow, SIGNAL(finished(NonOriginGameFlowResult)), this, SLOT(onNonOriginGameFlowFinished(NonOriginGameFlowResult)));
                        flow->initialize(NonOriginGameFlow::OpRemove, game);
                        flow->start();
                    }
                }
            }
        }

        void MainFlow::testPrematureTermination()
        {
            // if we are starting up, this file shouldn't exist
            QString genericDataLocation = Origin::Services::PlatformService::commonAppDataPath();
            if (Origin::Engine::Backup::GuardFile::exists(genericDataLocation))
            {
                // ...but if it does, generate telemetry
                GetTelemetryInterface()->Metric_APP_PREMATURE_TERMINATION();
                ORIGIN_LOG_EVENT << "[warning] Guard file for application FOUND. Telemetry generated.";
            }
            mGuardfile.reset(new Origin::Engine::Backup::GuardFile(genericDataLocation));
            ORIGIN_LOG_EVENT << "[-----] Guard file for application generated.";
        }


        void MainFlow::onLoginFlowFinished(LoginFlowResult result)
        {
            ORIGIN_LOG_EVENT << "onLoginFlowFinished";

            switch (result.result)
            {
                case FLOW_SUCCEEDED:
                    {
                        mLoginFlow.take()->deleteLater();
                        if (result.transition == LoginFlowResult::DEFAULT)
                        {
                            // user hit the login button in the dialog
                            Engine::UserRef user = Origin::Engine::LoginController::currentUser();
                            ORIGIN_ASSERT(user);
                            if (!user)
                                return;

                            // Notify heartbeat server of successful login
                            Services::Heartbeat::instance()->onLogin(user->eaid());

                            ORIGIN_VERIFY_CONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(authenticationRequired()), this, SLOT(onFailedTokenRenewal()));
                            ORIGIN_VERIFY_CONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(reAuthenticated()), this, SLOT(onReAuthentication()));

                            // authentication is true if we have received a valid token
                            // from the server, i.e. we are online
                            bool authenticated = Origin::Services::Connection::ConnectionStatesService::isAuthenticated(user->getSession());
                            if (!authenticated)
                            {
                                socialGoOffline();
                                // we are authenticated against the offline credential cache,
                                // skipping single login check and starting the client flow
                                if (!isClientFlowStarted())
                                    startClientFlow();
                            }
                            else
                            {
                                // we are authenticated against the login registration server
                                startSingleLoginFlow();
                            }
                        }
                        else
                        {
                            ORIGIN_ASSERT(false);
                        }
                    }
                    break;
                case FLOW_FAILED:
                    {
                        if (result.transition == LoginFlowResult::RETRY_LOGIN)
                        {
                            //this will clear the remember me token but we want to do that in case the failure keeps occurring and we don't want to put the user
                            //into an infinite loop of retrying
                            signOutEADP(ClientLogoutExitReason_Logout_onLoginError);
                            LogoutExitFlow::clearRememberMe();
                            restartLoginFlow(tr("login_message_timeout"));
                        }
                        else
                        {
                            mLoginFlow.take()->deleteLater();
                            // Stop and report the active client timer
                            // We need this here incase the user closed the login window
                            GetTelemetryInterface()->Metric_ACTIVE_TIMER_END(true);
                            OriginApplication::instance().quit(ReasonLoginExit);
                        }
                    }
                    break;
                default:
                    mLoginFlow.take()->deleteLater();
                    break;
            }
        }

        void MainFlow::onClientFlowFinished(ClientFlowResult result)
        {
            ORIGIN_LOG_DEBUG_IF(ENABLE_FLOW_TRACE) << "Client flow finished";

            ORIGIN_VERIFY_DISCONNECT(mClientFlow, SIGNAL(clientFlowCreated()), this, SIGNAL(startPendingAction()));

            ClientFlow::destroy();

            //  let the logout/exit flow handle the remainder of logout/exit
        }

        void MainFlow::onSingleLoginFlowFinished(SingleLoginFlowResult result)
        {
            ORIGIN_LOG_DEBUG_IF(ENABLE_FLOW_TRACE) << "Single Login flow finished";

            // don't delete the single login flow - we'll need it to handle messages
            // from engine/SingleLoginController

            if (OriginApplication::instance().isAppShuttingDown() || (!mLogoutExitFlow.isNull() && mLogoutExitFlow->confirmedAndStartedLoggingOut()))
            {
                // User is exiting the application so there's nothing we need to do here
                ORIGIN_LOG_EVENT << "app is shutting down or LogoutExitFLow not NULL";
                return;
            }

            emit singleLoginFlowFinished();

            switch (result.result)
            {
                case FLOW_SUCCEEDED:
                    {
                        Engine::UserRef user = Origin::Engine::LoginController::currentUser();
                        ORIGIN_ASSERT(user);
                        if (user)
                        {
                            //this will just set the flag, it will NOT initiate any state changes
                            Origin::Services::Session::SessionService::setSingleLoginCkCompletedFlag(user->getSession(), true);

                            // update online state depending on the result of the login flow
                            // need single login check flag set above, and if it is set,
                            //      manual offline = false, it will emit that the user is online & good to go
                            //      manual offline = true, it will call onClientConnectionStateChanged and single login will get set back to false for next time
                            Origin::Services::Session::SessionService::setManualOfflineFlag(user->getSession(), result.offlineMode);

                            if (!result.offlineMode)
                            {
                                user->socialControllerInstance()->loginToChat();
                            }

                            if (!isClientFlowStarted())
                                startClientFlow();
                        }
                    }
                    break;

                case FLOW_FAILED:
                default:
                    {
                        // update the single login flag to indicate that single login check has been completed
                        Engine::UserRef user = Origin::Engine::LoginController::currentUser();
                        if (user)
                        {
                            Origin::Services::Session::SessionService::setSingleLoginCkCompletedFlag(user->getSession(), true);
                        }

                        //we could be here either of two ways -- the hit cancel or closed the dialog box
                        //and the dialog could have been shown either at initial login or after token refresh
                        //if it's initial login, then it should exit
                        //if it's after token refresh and the user isn't already offline, we should place them offline
                        //check for existence of clientFlow to differentiate between initial login and in-session login
                        if (isClientFlowStarted())
                        {
                            if (user)
                            {
                                Origin::Services::Session::SessionService::setManualOfflineFlag(user->getSession(), true);
                            }
                        }
                        else    //initial login
                        {
                            startLogoutExitFlow(ClientLogoutExitReason_Logout_Silent);
                        }
                    }
                    break;
            }
        }

        void MainFlow::onRTPFlowFinished(RTPFlowResult result)
        {
            ORIGIN_LOG_DEBUG_IF(ENABLE_FLOW_TRACE) << "RTP flow finished";
        }

        void MainFlow::onSSOFlowFinished(SSOFlowResult result)
        {
            ORIGIN_LOG_DEBUG_IF(ENABLE_FLOW_TRACE) << "SSO flow finished";

            emit ssoFlowComplete(result, !mSSOFlow->loginOnly());

            ORIGIN_VERIFY_DISCONNECT(
                mSSOFlow.data(), SIGNAL(finished(SSOFlowResult)),
                this, SLOT(onSSOFlowFinished(SSOFlowResult)));

            GetTelemetryInterface()->Metric_SSO_FLOW_RESULT(
                resultString(result.result).toUtf8().constData(),
                ssoFlowActionString(result.action).toUtf8().constData(),
                ssoFlowLoginTypeString(mSSOFlow->loginType()).toUtf8().constData());

            //even if the SSO flow failed, if it's an RTP launch then we want to prompt the user to login (if not logged in)
            //if user is already logged in, then PendingActionFlow will take care of the triggering the pending Action
            if (result.result == FLOW_SUCCEEDED || result.action == SSOFlowResult::SSO_LOGIN)
            {
                switch (result.action)
                {
                    case SSOFlowResult::SSO_LOGIN:
                        {
                            //we might be SSO-ing in while the user is sitting on the login window, in that case, restart the flow
                            if (mLoginFlow.isNull())
                            {
                                ORIGIN_LOG_EVENT << "startLoginFlow() due to SSO_LOGIN, login flow null";
                                startLoginFlow();
                            }
                            else
                            {
                                //check and see if we're in the middle of auto-logging in
                                if (!mLoginFlow->autoLoginInProgress())
                                {
                                    ORIGIN_LOG_EVENT << "startLoginFlow() due to SSO_LOGIN, restart login flow";
                                    restartLoginFlow();
                                }
                                else
                                {
                                    ORIGIN_LOG_EVENT << "startLoginFlow() due to SSO_LOGIN aborted, login already in progress";
                                }
                            }
                        }
                        break;

                    case SSOFlowResult::SSO_LOGOUT:
                        {
                            logout(ClientLogoutExitReason_Logout_Silent);
                        }
                        break;

                    default:
                        break;
                }
            }
        }

        void MainFlow::onNonOriginGameFlowFinished(NonOriginGameFlowResult result)
        {
            ORIGIN_LOG_DEBUG_IF(ENABLE_FLOW_TRACE) << "NOG flow finished";

            // Operation can be changed during the flow based on user action, so we can't use sender()->currentOperation().
            NonOriginGameFlow* flow = static_cast<NonOriginGameFlow*>(sender());

            if (flow)
            {
                for (NonOriginGameFlowMap::iterator it = mNonOriginGameFlowsMap.begin(); it != mNonOriginGameFlowsMap.end(); ++it)
                {
                    if (flow == *it)
                    {
                        mNonOriginGameFlowsMap.erase(it);
                        break;
                    }
                }
                flow->deleteLater();
            }
        }

        void MainFlow::onGotAppCloseEvent()
        {
            ORIGIN_VERIFY_DISCONNECT(&OriginApplication::instance(), SIGNAL(gotCloseEvent()), this, SLOT(onGotAppCloseEvent()));

            //if we're not logged in, then we don't need to go thru the logout flow
            if (Origin::Engine::LoginController::instance()->isUserLoggedIn())
            {
                //emit the signal so the logout flow will get started from clientFlow
                emit(gotCloseEvent());
            }
            else
            {
                //otherwise proceed to exit
                OriginApplication::instance().cancelTaskAndExit(ReasonOS);
            }

        }

        void MainFlow::onRequestLogout()
        {
            startLogoutExitFlow(ClientLogoutExitReason_Logout);
        }

        void MainFlow::onRequestExit()
        {
            if (OriginApplication::instance().isAppShuttingDown() || (!mLogoutExitFlow.isNull() && mLogoutExitFlow->confirmedAndStartedLoggingOut()))
                return; // already in exit flow

            ClientLogoutExitReason reason = ClientLogoutExitReason_Exit;
            if (OriginApplication::instance().GetReceivedQueryEndSession())
                reason = ClientLogoutExitReason_Exit_NoConfirmation;

            startLogoutExitFlow(reason);
        }

        void MainFlow::onRequestSilentExit()
        {
            if (OriginApplication::instance().isAppShuttingDown() || (!mLogoutExitFlow.isNull() && mLogoutExitFlow->confirmedAndStartedLoggingOut()))
                return; // already in exit flow
            startLogoutExitFlow(ClientLogoutExitReason_Exit_NoConfirmation);
        }

        void MainFlow::onLogoutExitFlowFinished(LogoutExitFlowResult reason)
        {
            mReason = reason;
            ORIGIN_LOG_DEBUG_IF(ENABLE_FLOW_TRACE) << "Logout/Exit flow finished";

            //we need to grab the reason before mLogoutExitFlow gets reset
            //this would be true if we need to log them out to re-authenticate them but couldn't be cause they couldn't log out
            ClientLogoutExitReason myReason = mLogoutExitFlow->reason();
            mLogoutExitFlow.reset();

            //we cancelled out of the exit flow so nothing else to do
            if (reason.action == LogoutExitFlowResult::USER_CANCELLED)
            {
                bool setManualOffline = myReason == ClientLogoutExitReason_Logout_Silent_BadEncryptedToken ||
                                        myReason == ClientLogoutExitReason_Logout_Silent_CredentialsChanged;

                if (setManualOffline)
                {
                    Engine::UserRef user = Origin::Engine::LoginController::currentUser();
                    if (!user.isNull())
                    {
                        Origin::Services::Session::SessionService::setManualOfflineFlag(user->getSession(), true);
                    }
                }
                OriginApplication::instance().ResetQueryEndSessionFlag();
                return;
            }
            signOutEADP(myReason);
        }

        void MainFlow::onLogoutExit()
        {
            mContentFlowController->endFlows();

            // Notify heartbeat server of logout
            Services::Heartbeat::instance()->onLogoff();

            //disconnect listening for credentials change
            ORIGIN_VERIFY_DISCONNECT(
                mClientFlow,
                SIGNAL(credentialsChanged()),
                this,
                SLOT(onCredentialsChanged()));

            ORIGIN_VERIFY_DISCONNECT(
                mClientFlow,
                SIGNAL(lostAuthentication()),
                this,
                SLOT(onLostAuthentication()));

            if (mReason.action == LogoutExitFlowResult::USER_EXITED)
            {
                if (!mClientFlow.isNull())
                    mClientFlow->exit();

                OriginApplication& originApp = OriginApplication::instance();
                originApp.cancelTaskAndExit(ReasonLogoutExit);
            }
            else if (mReason.action == LogoutExitFlowResult::USER_RESTART)
            {
                if (!mClientFlow.isNull())
                    mClientFlow->exit();
                OriginApplication::instance().restart();
            }
            else
            {
                Engine::UserRef user = Origin::Engine::LoginController::currentUser();
                ORIGIN_ASSERT(user);
                // we lose authentication during logout, but don't want to trigger re-authentication
                if (user)
                {
                    ORIGIN_VERIFY_DISCONNECT(Origin::Services::Connection::ConnectionStatesService::instance(),
                                             SIGNAL(authenticationRequired()),
                                             this,
                                             SLOT(onFailedTokenRenewal()));
                }

                if (!mClientFlow.isNull())
                {
                    mClientFlow->logout();
                }
                else
                {
                    Origin::Engine::LoginController::logoutAsync(Origin::Engine::LoginController::currentUser()->getSession());
                }

                ORIGIN_ASSERT(mLoginFlow.isNull());

                //check and see if an SSO token exists, if so, then it means we needed to logout and log back in
                QString ssoToken = Origin::Services::readSetting(Origin::Services::SETTING_UrlSSOtoken);
                if (!ssoToken.isEmpty() && !mSSOFlow->loginOnly())
                {
                    PendingActionFlow *pendingActionFlow = PendingActionFlow::instance();

                    if (pendingActionFlow)
                    {
                        pendingActionFlow->setPendingRTPLaunch();
                    }
                }

                if (mReason.action == LogoutExitFlowResult::USER_FORCED_LOGGED_OUT_BADTOKEN)
                {
                    //pass the message that will get shown on the login window ("login credentials have expired. please re-log in")
                    ORIGIN_LOG_EVENT << "startLoginFlow() due to USER_FORCED_LOGGED_OUT_BADTOKEN";
                    startLoginFlow(tr("ebisu_client_login_expired"), Origin::UIToolkit::OriginBanner::Info);
                }
                else if (mReason.action == LogoutExitFlowResult::USER_FORCED_LOGGED_OUT_CREDENTIALCHANGE)
                {
                    //pass the message that will get shown on the login window ("your login credentials have changed. please re-log in")
                    ORIGIN_LOG_EVENT << "startLoginFlow() due to USER_FORCED_LOGGED_OUT_CREDENTIALCHANGE";
                    startLoginFlow(tr("ebisu_client_login_credentials_changed"), Origin::UIToolkit::OriginBanner::Info);
                }
                else
                {
                    ORIGIN_LOG_EVENT << "startLoginFlow() due to logout";
                    startLoginFlow();
                }
            }
        }

        MainFlow::MainFlow()
            : mFirstTimeThroughLogin(true)
            , mNewUser(false)
            , mAgeUpFlowCompleted(false)
            , mReason(LogoutExitFlowResult(LogoutExitFlowResult::USER_NO_ACTION))
        {
            ORIGIN_LOG_EVENT << "construct MainFlow";
            testPrematureTermination();
        }

        MainFlow::~MainFlow()
        {
            ORIGIN_LOG_EVENT << "destruct MainFlow";
            onDirtyBitsTrafficToolClosed();
            mGuardfile.reset();
        }

        void MainFlow::startLoginFlow(QString const& initialLoginMessage, const int icon)
        {
            ORIGIN_LOG_EVENT << "Initial login msg: " << initialLoginMessage;
            if (mLoginFlow.isNull())
            {
                // Reset the age up flow flag which will get set to true by the login flow
                // if an underage user ages up.
                mAgeUpFlowCompleted = false;

                StartupState startupState = StartNormal;
                if (mFirstTimeThroughLogin)
                {
                    //check and see if we were started via auto-start
                    startupState = OriginApplication::instance().startupState();
                }

                mLoginFlow.reset(new LoginFlow(startupState, initialLoginMessage, static_cast<Origin::UIToolkit::OriginBanner::IconType>(icon)));
                ORIGIN_VERIFY_CONNECT(mLoginFlow.data(), SIGNAL(finished(LoginFlowResult)), this, SLOT(onLoginFlowFinished(LoginFlowResult)));
                ORIGIN_VERIFY_CONNECT(mLoginFlow.data(), SIGNAL(loginError()), this, SIGNAL(loginError()));
                ORIGIN_VERIFY_CONNECT(mLoginFlow.data(), SIGNAL(ageUpFlowCompleted()), this, SLOT(onAgeUpFlowCompleted()));

                mLoginFlow->start();
                // Set to false after we start the flow for the first time.
                mFirstTimeThroughLogin = false;
            }
            else
            {
                ORIGIN_LOG_ERROR << "mLoginFlow not NULL";
            }
        }

        void MainFlow::restartLoginFlow(QString const& restartLoginMessage)
        {
            if (mLoginFlow.isNull())
            {
                ORIGIN_LOG_ERROR << "Restarting login flow";
                startLoginFlow(restartLoginMessage);
            }
            else
            {
                mLoginFlow->restart(restartLoginMessage);
            }
        }

        bool MainFlow::isClientFlowStarted() const
        {
            // technically this only checks that ClientFlow::create()
            // has been called, not ClientFlow::start(), should be OK
            return ClientFlow::instance() != NULL;
        }

        void MainFlow::startClientFlow()
        {
            ORIGIN_LOG_DEBUG_IF(ENABLE_FLOW_TRACE) << "Client flow started";

            ClientFlow::create();

            mClientFlow = ClientFlow::instance();
            ORIGIN_VERIFY_CONNECT_EX(
                mClientFlow, SIGNAL(logoutConcluded()),
                this, SIGNAL(logoutConcluded()),
                Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(
                mClientFlow, SIGNAL(requestLogout()),
                this, SLOT(onRequestLogout()),
                Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mClientFlow, SIGNAL(requestSilentExit()), this, SLOT(onRequestSilentExit()), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(
                mClientFlow, SIGNAL(requestExit()),
                this, SLOT(onRequestExit()),
                Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(
                mClientFlow, SIGNAL(finished(ClientFlowResult)),
                this, SLOT(onClientFlowFinished(ClientFlowResult)),
                Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(
                mClientFlow, SIGNAL(addNonOriginGame()),
                this, SLOT(onAddNonOriginGame()),
                Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(
                mClientFlow, SIGNAL(lostAuthentication()),
                this, SLOT(onLostAuthentication()),
                Qt::QueuedConnection);

            //RTPFlow listens for this but since ClientFlow isn't created when RTPFlow is started, have it listen to the signal from MainFlow
            ORIGIN_VERIFY_CONNECT(mClientFlow,
                                  SIGNAL(clientFlowCreated()),
                                  this,
                                  SIGNAL(startPendingAction()));

            //listen for dirty bits connection changes
            ORIGIN_VERIFY_CONNECT(
                mClientFlow,
                SIGNAL(credentialsChanged()),
                this,
                SLOT(onCredentialsChanged()));

            //check and see if the client should start minimized or minimized to system tray
            StartupState appStartupState = OriginApplication::instance().startupState();
            ClientViewWindowShowState startShowState = Origin::Client::ClientWindow_SHOW_NORMAL;
            if (rtpFlow()->startClientMinimized())
            {
                startShowState = Origin::Client::ClientWindow_SHOW_MINIMIZED;    //minimized to task bar
            }
            // TODO: Make this work as StartInSystemTray
            else if ((appStartupState == StartInTaskbar || appStartupState == StartInSystemTray) && Engine::LoginController::isCurrentSessionAutoLogin())
            {
                startShowState = Origin::Client::ClientWindow_SHOW_MINIMIZED_SYSTEMTRAY; //minimized to system tray
            }
            else if (appStartupState == StartMaximized)
            {
                startShowState = Origin::Client::ClientWindow_SHOW_MAXIMIZED;
            }

            ClientFlow::instance()->setStartClientMinimized(startShowState);
            ClientFlow::instance()->start();

            if ((startShowState == Origin::Client::ClientWindow_SHOW_NORMAL) && (mMOTDFlow.isNull() == FALSE))
            {
                mMOTDFlow->raise();
            }

        }

        void MainFlow::startFirstLaunchFlow()
        {
            ORIGIN_LOG_DEBUG_IF(ENABLE_FLOW_TRACE) << "First Launch flow started";

            mFirstLaunchFlow.reset(new FirstLaunchFlow);
            mFirstLaunchFlow->start();
        }

        void MainFlow::startContentFlowController()
        {
            mContentFlowController.reset(new ContentFlowController);
        }

        void MainFlow::startMOTDFlow()
        {
            ORIGIN_LOG_DEBUG_IF(ENABLE_FLOW_TRACE) << "MOTD flow started";

            bool motdDisabled = Origin::Services::readSetting(Origin::Services::SETTING_DisableMotd);
            if (!motdDisabled)
            {
                mMOTDFlow.reset(new MOTDFlow);
                mMOTDFlow->start();
            }
        }

        void MainFlow::startRTPFlow()
        {
            ORIGIN_LOG_EVENT << "start RtP flow";
            mRTPFlow.reset(new RTPFlow);
            ORIGIN_VERIFY_CONNECT(
                mRTPFlow.data(), SIGNAL(finished(RTPFlowResult)),
                this, SLOT(onRTPFlowFinished(RTPFlowResult)));
            mRTPFlow->start();
        }

        void MainFlow::startSingleLoginFlow()
        {
            ORIGIN_LOG_DEBUG_IF(ENABLE_FLOW_TRACE) << "Single Login flow started";

            if (!mSingleLoginFlow)
            {
                // don't delete the single login flow - we'll need it to handle messages
                // from engine/SingleLoginController
                mSingleLoginFlow.reset(new SingleLoginFlow);
                ORIGIN_VERIFY_CONNECT(
                    mSingleLoginFlow.data(), SIGNAL(finished(SingleLoginFlowResult)),
                    this, SLOT(onSingleLoginFlowFinished(SingleLoginFlowResult)));
            }
            mSingleLoginFlow->start();
        }

        void MainFlow::startLogoutExitFlow(ClientLogoutExitReason reason)
        {
            ORIGIN_LOG_DEBUG_IF(ENABLE_FLOW_TRACE) << "Logout/Exit flow started";

            //only initiate a new one if we aren't already in a flow
            //this could occur because we have modal dialogs in the exit flow that wait for response but another exit could be initiated from the system tray
            if (mLogoutExitFlow.isNull())
            {
                mLogoutExitFlow.reset(new LogoutExitFlow(reason));
                ORIGIN_VERIFY_CONNECT(
                    mLogoutExitFlow.data(), SIGNAL(finished(LogoutExitFlowResult)),
                    this, SLOT(onLogoutExitFlowFinished(LogoutExitFlowResult)));

                onContinueExitFlow(true);
            }
            else
            {
                ORIGIN_LOG_EVENT << "LogoutExitFlow already in progress";
            }
        }

        void MainFlow::signOutEADP(ClientLogoutExitReason reason)
        {
            QWebView* view = new QWebView;

            if (reason != ClientLogoutExitReason_Logout_onLoginError)
            {
                ORIGIN_VERIFY_CONNECT(view, SIGNAL(loadFinished(bool)), this, SLOT(onLogoutExit()));
            }

            view->setHidden(true);

            // This will cause the sid cookie to be attached to the logout request
            view->page()->setNetworkAccessManager(Origin::Services::NetworkAccessManagerFactory::instance()->createNetworkAccessManager());

            QString connectPortalBaseUrl = Origin::Services::readSetting(Origin::Services::SETTING_ConnectPortalBaseUrl).toString();
            QString accessToken = Services::Session::SessionService::accessToken(Services::Session::SessionService::currentSession());

            QUrl url;

            // We don't want to call /connect/logout during exit because it will invalidate the remember me cookie
            if (reason != ClientLogoutExitReason_Exit &&
                    reason != ClientLogoutExitReason_Exit_NoConfirmation &&
                    reason != ClientLogoutExitReason_Logout_Silent_BadEncryptedToken)
            {
                QString clientId = Origin::Services::readSetting(Origin::Services::SETTING_ClientId).toString();
                QString logoutSuccessRedirectUrl = Origin::Services::readSetting(Origin::Services::SETTING_LogoutSuccessRedirectUrl).toString();

                url = QUrl(connectPortalBaseUrl + QString("/connect/logout"));
                QUrlQuery urlQuery(url);
                urlQuery.addQueryItem("client_id", clientId);
                urlQuery.addQueryItem("redirect_uri", logoutSuccessRedirectUrl);
                url.setQuery(urlQuery);
            }
            else
            {
                url = QUrl(connectPortalBaseUrl + QString("/connect/exit"));
            }

            QUrlQuery urlQuery(url);
            urlQuery.addQueryItem("access_token", accessToken);
            url.setQuery(urlQuery);

            view->load(url);
            view->deleteLater();
        }

        void MainFlow::onContinueExitFlow(bool)
        {
            mLogoutExitFlow->start();
        }

        void MainFlow::onAgeUpFlowCompleted()
        {
            mAgeUpFlowCompleted = true;
        }

        void MainFlow::startSSOFlow(Origin::Client::SSOFlow::SSOLoginType loginType)
        {
            ORIGIN_LOG_DEBUG_IF(ENABLE_FLOW_TRACE) << "SSO flow started";

            mSSOFlow.reset(new SSOFlow);
            ORIGIN_VERIFY_CONNECT(
                mSSOFlow.data(), SIGNAL(finished(SSOFlowResult)),
                this, SLOT(onSSOFlowFinished(SSOFlowResult)));

            ORIGIN_VERIFY_CONNECT(
                mSSOFlow.data(), SIGNAL(ssoIdentifyError()),
                this, SIGNAL(ssoIdentifyError()));

            mSSOFlow->setLoginType(loginType);
            mSSOFlow->start();
        }

        bool MainFlow::ssoFlowActive()
        {
            if (mSSOFlow)
            {
                return mSSOFlow->active();
            }
            return false;
        }

        void MainFlow::onLostAuthentication()
        {
            authenticationRequired(ClientLogoutExitReason_Logout_Silent_BadEncryptedToken);
        }

        void MainFlow::onFailedTokenRenewal()
        {
            // Will get reconnected at the end of a successful credentialed login. If not disconnected, the single login flow will be started too early when the access token
            // is received but prior to the creation of a new user object.
            ORIGIN_VERIFY_DISCONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(reAuthenticated()), this, SLOT(onReAuthentication()));

            authenticationRequired(ClientLogoutExitReason_Logout_Silent_BadEncryptedToken);
        }

        void MainFlow::onCredentialsChanged()
        {
            authenticationRequired(ClientLogoutExitReason_Logout_Silent_CredentialsChanged);
        }

        void MainFlow::authenticationRequired(ClientLogoutExitReason reason)
        {
            ORIGIN_LOG_EVENT << "Authentication required";

            //the encryption token is no longer valid, so we're going to log the user out
            //but if the user is one of the states where logout is not allowed, place him in manual offline mode instead
            //but have it go thru the logout exit flow so that the user is prompted with the dialog
            //"Please close all running games and retry to go online"
            //then when flow is finished (cancelled), then it will set the manualOffline flag (see onLogoutExitFlowFinished)
            startLogoutExitFlow(reason);
        }

        void MainFlow::onReAuthentication()
        {
            //this gets triggered from the ConnectionStateServices after reauthentication occurs after coming back online
            //but we're not truly back "online" until single login flow has completed

            //only start the single login flow if the single login completed check hasn't been done yet
            //in theory onReauthentication won't be called if it's just a timed token refresh but add the guard just in case

            //if we're coming back from being offline, then we should do a single login check
            //the singlelogincheckcompleted flag will get cleared at the point that LoginRegistrationSession service detects that we're going offline
            if (!Origin::Services::Connection::ConnectionStatesService::isSingleLoginCheckCompleted())
            {
                ORIGIN_LOG_EVENT << "Starting single login check";
                startSingleLoginFlow();
            }
            else
            {
                ORIGIN_LOG_EVENT << "Single Login Check already completed";
            }
        }

        void MainFlow::showAddGameWindow()
        {
            onAddNonOriginGame();
        }

        void MainFlow::onAddNonOriginGame()
        {
            if (mNonOriginGameFlowsMap.contains(NonOriginGameFlow::OpAdd))
            {
                mNonOriginGameFlowsMap.value(NonOriginGameFlow::OpAdd)->focus();
            }
            else
            {
                NonOriginGameFlow* flow = new NonOriginGameFlow();
                if (flow)
                {
                    mNonOriginGameFlowsMap.insert(NonOriginGameFlow::OpAdd, flow);

                    ORIGIN_VERIFY_CONNECT(flow, SIGNAL(finished(NonOriginGameFlowResult)), this, SLOT(onNonOriginGameFlowFinished(NonOriginGameFlowResult)));
                    flow->initialize(NonOriginGameFlow::OpAdd);
                    flow->start();
                }
            }
        }

        void MainFlow::socialGoOnline()
        {
            ORIGIN_LOG_EVENT << "Going online";
            Engine::UserRef user = Origin::Engine::LoginController::currentUser();
            ORIGIN_ASSERT(user);
            if (user)
            {
                // NOTE: Not logging into chat here. Once we finish logging in single login will do that
                // for us
                Origin::Services::Session::SessionService::setManualOfflineFlag(user->getSession(), false);
            }
            else
                ORIGIN_LOG_ERROR << "user is NULL!";
        }

        void MainFlow::socialGoOffline()
        {
            ORIGIN_LOG_EVENT << "Going offline";
            Engine::UserRef user = Origin::Engine::LoginController::currentUser();
            ORIGIN_ASSERT(user);
            if (user)
            {
                // update online state depending on the result of the login flow.
                Origin::Services::Session::SessionService::setManualOfflineFlag(user->getSession(), true);
            }
            else
                ORIGIN_LOG_ERROR << "user is NULL!";
        }

        void MainFlow::consumeUrlParameter(const Origin::Services::Setting &setting)
        {
            //we really need a way to unset the readonly settings so we can distinguish between whether the setting exists or not
            //but currently there isn't a way to do that, so just set it to the "empty" values for now
            // TODO: HACK - ReadOnly Settings - need a way to correctly clear and unset the setting
            if ((QVariant::Type)setting.type() == QVariant::String)
            {
                Origin::Services::writeSetting(setting, "");
            }
            else if ((QVariant::Type)setting.type() == QVariant::Bool)
            {
                Origin::Services::writeSetting(setting, false);
            }
            else
            {
                ORIGIN_ASSERT_MESSAGE(0, "Unhandled variant");
            }

        }

        void MainFlow::setNewUser(bool setting)
        {
            mNewUser = setting;
        }

        bool MainFlow::wasAgeUpFlowCompleted() const
        {
            return mAgeUpFlowCompleted;
        }

        void MainFlow::setShowAgeUpFlow()
        {
            //we need to not show Age up Flow during login for RTP game launch
            if (!mLoginFlow.isNull() && !mLoginFlow->isLoginRequestInProcess())
            {
                if (rtpFlow()->isPending())
                    mLoginFlow->setShowAgeUpFlow(false);
            }

        }

        Origin::Client::SingleLoginFlow* MainFlow::singleLoginFlow()
        {
            if (&mSingleLoginFlow == NULL)
            {
                ORIGIN_LOG_ERROR << "&mSingleLoginFlow is NULL!";
                return NULL;
            }

            return mSingleLoginFlow.data();
        }

    }
}
