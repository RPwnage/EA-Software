#include "PendingActionFlow.h"
#include "PendingActionBaseViewController.h"
#include "PendingActionFactory.h"
#include "MainFlow.h"
#include "services/log/LogService.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "ClientFlow.h"
#include "RTPFlow.h"
#include "FlowResult.h"
#include "engine/social/SocialController.h"
#include "chat/Roster.h"
#include "chat/ConnectedUser.h"
#include "chat/OriginConnection.h"

namespace Origin
{
    namespace Client
    {
        static PendingActionFlow* mInstance = NULL;

        void PendingActionFlow::create()
        {
            ORIGIN_LOG_EVENT << "create PendingActionFlow";
            ORIGIN_ASSERT(!mInstance);
            if (!mInstance)
            {
                qRegisterMetaType<SSOFlowResult>("SSOFlowResult");
                mInstance = new PendingActionFlow();
            }
        }

        void PendingActionFlow::destroy()
        {
            ORIGIN_LOG_EVENT << "destroy PendingActionFlow";
            ORIGIN_ASSERT(mInstance);
            if (mInstance)
            {
                delete mInstance;
                mInstance = NULL;
            }
        }

        PendingActionFlow* PendingActionFlow::instance()
        {
            return mInstance;
        }

        PendingActionFlow::PendingActionFlow()
            : mPendingAction(NULL)
            , mPendingActionFlowStatus(Inactive)
            , mSuppressPromo(false)
        {
            PendingActionFactory::registerPendingActions();
        }

        PendingActionFlow::~PendingActionFlow()
        {

        }

        bool PendingActionFlow::startAction(QUrl url, bool launchedFromBrowser, QString uuid)
        {
            bool success = true;

            //if we can't start a new action just set success to false and exit out
            if(!canStartNewAction(url) || !url.isValid() ||  ((url.scheme() != "eadm") && (url.scheme().toLower() != "origin2")))
            {
                success = false;
            }
            else
            {
                //if we already have an action, kill it and start a new one
                if(mPendingAction)
                {
                    mPendingAction->deleteLater();
                }

                mPendingAction = PendingActionFactory::createPendingActionViewController(url, launchedFromBrowser, uuid);
                if(mPendingAction)
                {
                    mUUID = uuid;

                    if(mPendingAction->parseAndValidateParams())
                    {
                        //send pertinent telemetry
                        mPendingAction->sendTelemetry();

                        //clear previous connection
                        ORIGIN_VERIFY_DISCONNECT(Origin::Client::MainFlow::instance(), SIGNAL(ssoFlowComplete(SSOFlowResult, bool )),
                                                 this, SLOT(onSSOComplete(SSOFlowResult, bool)));


                        //this has to be a queued connection, because we want onSSOComplete to get triggered after all the logic in MainFlow::handleMessage
                        ORIGIN_VERIFY_CONNECT_EX(Origin::Client::MainFlow::instance(), SIGNAL(ssoFlowComplete(SSOFlowResult, bool)),
                                                 this, SLOT(onSSOComplete(SSOFlowResult, bool)), Qt::QueuedConnection);

                        //always start the sso login regardless of whether a user is logged in or not because it may not be the same user we are trying to sso in
                        mPendingActionFlowStatus = WaitingForLogin;
                        mPendingAction->ssoLogin();
                    }
                    else
                    {
                        resetAction(ErrorInvalidParams);
                        success = false;
                    }

                }
                else
                {
                    success = false;
                }
            }
            return success;
        }

        bool PendingActionFlow::canStartNewAction(const QUrl &url)
        {
            bool same = false;
            bool rtpActive = false;
            Origin::Client::MainFlow *mainFlow = Origin::Client::MainFlow::instance();
            Origin::Client::RTPFlow *rtpFlow = NULL;

            if(mainFlow)
            {
                rtpFlow = mainFlow->rtpFlow();
                if(rtpFlow && rtpFlow->isPending())
                {
                    rtpActive = true;
                }
            }

            if(mPendingAction)
            {
                same = (PendingActionFactory::lookupIdFromUrl(url).compare(mPendingAction->lookupId()) == 0);
            }
            //can start a new action if
            //1. There is no active pending action
            //2. There is an active pending action, but it is the same action
            //AND
            //RTP is NOT active
            return (!rtpActive && ((mPendingAction == NULL) || same));
        }

        void PendingActionFlow::resetAction(PendingActionFlowStatus status)
        {
            if(mPendingAction)
            {
                mPendingAction->deleteLater();
                mPendingAction = NULL;
                mPendingActionFlowStatus = status;
            }
        }

        void PendingActionFlow::onSSOComplete(SSOFlowResult  result, bool launchGame)
        {
            ORIGIN_VERIFY_DISCONNECT(Origin::Client::MainFlow::instance(), SIGNAL(ssoFlowComplete(SSOFlowResult, bool )),
                                     this, SLOT(onSSOComplete(SSOFlowResult, bool)));

            //success or failure of the SSOFlow doesn't actually matter with respect to what to do next...
            //because if SSOFlow failed &&
            // a) user is logged in, we just continue with the flow
            // b) SSO_LOGIN: user is not logged in, we'll bring up the login window
            // c) SSO_LOGOUT: shouldn't be getting failure + LOGOUT
            switch (result.action)
            {
                case SSOFlowResult::SSO_LOGGEDIN_USERMATCHES:
                case SSOFlowResult::SSO_LOGGEDIN_USERUNKNOWN:
                    {
                        if (mPendingAction && mPendingAction->shouldConsumeSSOToken())
                        {
                            //need to consume the SSO token since we don't need to login
                            MainFlow::instance()->consumeUrlParameter(Origin::Services::SETTING_UrlSSOtoken);
                            MainFlow::instance()->consumeUrlParameter(Origin::Services::SETTING_UrlSSOauthCode);
                            MainFlow::instance()->consumeUrlParameter(Origin::Services::SETTING_UrlSSOauthRedirectUrl);

                        }
                        if(mPendingAction)
                        {
                            // the call to start rtp is in performMainAction for the GameLaunch pending action
                            // if it is just SSO Only (storeopen/libraryopen), then we will show the popups at htis point
                            mPendingAction->setWindowMode();
                            mPendingAction->setQuietMode();
                            mPendingAction->performMainAction();
                            mPendingAction->showPopupWindow();
                            resetAction();
                        }
                    }
                    break;

                case SSOFlowResult::SSO_LOGIN:
                    {
                        if(launchGame)
                        {
                            if(mPendingAction)
                            {
                                //performMainAction setsup the RTP vars by writing them to settings
                                mPendingAction->performMainAction();
                                resetAction();
                            }
                        }
                        else
                        {
                            //clear previous connection
                            ORIGIN_VERIFY_DISCONNECT (Origin::Client::MainFlow::instance(), SIGNAL (startPendingAction()), this, SLOT(onStartPendingAction ()));
                            ORIGIN_VERIFY_CONNECT_EX (Origin::Client::MainFlow::instance(), SIGNAL (startPendingAction()), this, SLOT(onStartPendingAction ()), Qt::QueuedConnection);
                        }
                    }
                    break;
                case SSOFlowResult::SSO_LOGOUT:
                    {
                        if(!launchGame)
                        {
                            //clear previous connection
                            ORIGIN_VERIFY_DISCONNECT (Origin::Client::MainFlow::instance(), SIGNAL (startPendingAction()), this, SLOT(onStartPendingAction ()));
                            ORIGIN_VERIFY_CONNECT_EX (Origin::Client::MainFlow::instance(), SIGNAL (startPendingAction()), this, SLOT(onStartPendingAction ()), Qt::QueuedConnection);
                        }
                        else
                        {
                            //the main action in this case is triggered from on logout exit
                        }
                    }
                    break;

                default:
                    break;
            }
        }

        void PendingActionFlow::onStartPendingAction()
        {
            ORIGIN_VERIFY_DISCONNECT (Origin::Client::MainFlow::instance(), SIGNAL (startPendingAction()), this, SLOT(onStartPendingAction ()));

            if(mPendingAction)
            {
                mPendingAction->setQuietMode();
                mPendingAction->setWindowMode();
                if(mPendingAction->requiresEntitlements())
                {
                    mPendingActionFlowStatus = WaitingForMyGames;

                    Origin::Engine::Content::ContentController *content_controller = Origin::Engine::Content::ContentController::currentUserContentController();
                    if (content_controller)
                    {
                        ORIGIN_VERIFY_DISCONNECT(content_controller, SIGNAL(initialFullUpdateCompleted()), this, SLOT(onEntitlementsUpdated()));
                    }
                    ORIGIN_VERIFY_DISCONNECT(ClientFlow::instance(), SIGNAL(myGamesLoadFinished()), this, SLOT(onEntitlementsUpdated()));

                    if (mPendingAction->requiresExtraContent()) // if it might need DLC, make sure extra content has been at least initially loaded
                    {
                        if (content_controller)
                        {
                            ORIGIN_VERIFY_CONNECT(content_controller, SIGNAL(initialFullUpdateCompleted()), this, SLOT(onEntitlementsUpdated()));
                        }
                        else    // this should never happen but just in case it is better to have something connected than to leave it dangling
                        {
                            ORIGIN_VERIFY_CONNECT(ClientFlow::instance(), SIGNAL(myGamesLoadFinished()), this, SLOT(onEntitlementsUpdated()));
                        }
                    }
                    else
                    {
                        ORIGIN_VERIFY_CONNECT(ClientFlow::instance(), SIGNAL(myGamesLoadFinished()), this, SLOT(onEntitlementsUpdated()));
                    }
                }
                else
                {
                    mPendingAction->performMainAction();
                }

                if(mPendingAction->requiresSocial())
                {
                    Origin::Chat::Roster *roster = Origin::Engine::LoginController::instance()->currentUser()->socialControllerInstance()->chatConnection()->currentUser()->roster();
                    ORIGIN_VERIFY_DISCONNECT(roster, SIGNAL(loaded()), this, SLOT(onSocialRosterLoaded()));
                    ORIGIN_VERIFY_CONNECT(roster, SIGNAL(loaded()), this, SLOT(onSocialRosterLoaded()));
                    //set a 30 second time out to fire off the onSocialRosterLoaded slot anyways and disconnect signal (in case we can't log in to the chat server)
                    QTimer::singleShot(30000, this, SLOT(onSocialRosterLoaded()));
                }
                else
                {
                    mPendingAction->showPopupWindow();
                }

                if(mPendingAction->allComponentsExecuted())
                    resetAction();


            }
        }

        QString PendingActionFlow::activeUUID()
        {
            return mUUID;
        }

        void PendingActionFlow::onEntitlementsUpdated()
        {
            ORIGIN_VERIFY_DISCONNECT(ClientFlow::instance(), SIGNAL(myGamesLoadFinished()), this, SLOT(onEntitlementsUpdated()));
            if(mPendingAction)
            {
                mPendingAction->performMainAction();
                if(mPendingAction->allComponentsExecuted())
                    resetAction();
            }
        }

        void PendingActionFlow::onSocialRosterLoaded()
        {
            Origin::Chat::Roster *roster = Origin::Engine::LoginController::instance()->currentUser()->socialControllerInstance()->chatConnection()->currentUser()->roster();
            ORIGIN_VERIFY_DISCONNECT(roster, SIGNAL(loaded()), this, SLOT(onSocialRosterLoaded()));
            if(mPendingAction)
            {
                mPendingAction->showPopupWindow();
                if(mPendingAction->allComponentsExecuted())
                    resetAction();
            }
        }


        QString PendingActionFlow::startupTab()
        {
            QString startupTab;

            if(mPendingAction)
            {
                startupTab = mPendingAction->startupTab();
            }

            return startupTab;
        }

        QUrl PendingActionFlow::storeUrl()
        {
            QUrl storeUrl;

            if(mPendingAction)
            {
                storeUrl = mPendingAction->storeUrl();
            }

            return storeUrl;
        }

        int PendingActionFlow::windowsShowState()
        {
            int showState = ClientWindow_NO_ACTION;

            if(mPendingAction)
            {
                showState = mPendingAction->windowsShowState();
            }

            return showState;
        }

        bool PendingActionFlow::shouldShowPrimaryWindow()
        {
            //client window may be minimized but we want to unminimize it under the following circumstance
            //game launch but requires login (i.e. no SSO)
            bool showPrimary = true;
            RTPFlow* rtpFlow = MainFlow::instance()->rtpFlow();
            // There is case when we start an RTP through protocol the RTP information hasn't been set yet.
            const bool isRtpLaunch = (mPendingAction && mPendingAction->lookupId() == PendingAction::GameLaunchLookupId) || (rtpFlow && rtpFlow->getRtpLaunchActive());
            if (isRtpLaunch && rtpFlow && rtpFlow->shouldMainWindowMinimize())  //this would be RTP launchgame
            {
                //we want to double check to see if this requires manual login
                if(!Origin::Engine::LoginController::instance()->isUserLoggedIn())
                {
                    //user not logged in but if there's an SSO token then we can stay minimized
                    QString ssoToken = Origin::Services::readSetting(Origin::Services::SETTING_UrlSSOtoken);
                    QString ssoAuthCode = Origin::Services::readSetting(Origin::Services::SETTING_UrlSSOauthCode);
                    if (!ssoToken.isEmpty() || !ssoAuthCode.isEmpty())
                    {
                        //SSOtoken exists so we should be able to remain minimized
                        showPrimary = false;
                    }
                }
                else
                {
                    showPrimary = false;
                }
            }
            return showPrimary;
        }

        void PendingActionFlow::setPendingRTPLaunch()
        {
            //this function is used in the case where the user that is logged is different than the one we sso in with
            //we will log the current user out and on logging out we want set the pending RTP which gets done in the performMainAction function of the
            //GameLaunch pending action
            if(mPendingAction && (mPendingAction->lookupId() == PendingAction::GameLaunchLookupId))
            {
                mPendingAction->performMainAction();
                resetAction();
            }
        }
    }
}
