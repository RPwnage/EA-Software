///////////////////////////////////////////////////////////////////////////////
// RTPFlow.cpp (Required To Play)
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "RTPFlow.h"
#include "OriginApplication.h"
#include "engine/content/ContentController.h"
#include "engine/content/LocalContent.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/login/LoginController.h"
#include "engine/subscription/SubscriptionManager.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include <services/platform/TrustedClock.h>
#include "RedeemBrowser.h"
#include "RTPViewController.h"
#include "TelemetryAPIDLL.h"
#include "LocalizedDateFormatter.h"

#include "MainFlow.h"
#include "ClientFlow.h"
#include "PlayFlow.h"
#include "ContentFlowController.h"

namespace Origin
{
    namespace Client
    {
        QString UnicodeUnescapeQString(QString sStringToDecode);
    }
}

#define MAX_RETRY_COUNT 2

namespace Origin
{
    namespace Client
    {

        //#pragma message( "HACK: RtP 8.3.7 handles case where storage quota or other cloud error occurs and Entitlement::createCancelLaunchDialog logic calls directlyLaunchContent without RtP CmdLineParams")
        QString RTPFlow::sRtpLaunchCmdParams = "";
        bool RTPFlow::sRtpLaunchActive = false;
        QString RTPFlow::sRtpLaunchGameContentId = "";

        RTPFlow::RTPFlow()
            : mRTPViewController (NULL)
            , mShowProductPage(false)
            , mRedemptionActive(false)
            , mRTPStatus(GameLaunchInactive)
            , mPendingEntitlementFlag(RTP_NOTWAITING)
            , mForceOnline(false)
            , mRetryCount(0)
        {
            //create the PlayView and PlayViewController
            mRTPViewController = new RTPViewController(this);
            ORIGIN_VERIFY_CONNECT(mRTPViewController, SIGNAL(cancel()), this, SLOT(onCancelLaunch()));
            ORIGIN_VERIFY_CONNECT_EX(mRTPViewController, SIGNAL(redeemGame()), this, SLOT(onRedeemButton()), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mRTPViewController, SIGNAL(notThisUser()), this, SLOT(onNotThisUserButton()), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mRTPViewController, SIGNAL(purchaseGame(const QString&)), this, SLOT(onPurchaseGame(const QString&)), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mRTPViewController, SIGNAL(renewSubscriptionMembership()), this, SLOT(onRenewSubscriptionMembership()), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mRTPViewController, SIGNAL(goOnline()), this, SLOT(onGoOnline()), Qt::QueuedConnection);
            // We have to have a go-between here. When RTP is created, ClientFlow isn't alive
            ORIGIN_VERIFY_CONNECT_EX(mRTPViewController, SIGNAL(showDeveloperTool()), this, SLOT(onShowDeveloperTool()), Qt::QueuedConnection);
            mAutoUpdating = false;
            mLaunchGameRestart = false;
            mAutoDownload = false;
        }

        RTPFlow::~RTPFlow ()
        {
        }

        bool RTPFlow::getRtpLaunchActive(const QString contentId)
        {
            //no active RTP flow in progress
            if (!sRtpLaunchActive)
                return false;

            if (contentId.isEmpty ())   //just return whether an RTP flow is in progress or not
                return sRtpLaunchActive;

            //here if RTP is in progress, check if it's for contentId
            return (contentId == sRtpLaunchGameContentId);
        }


        void RTPFlow::clearRtpLaunchInfo(const QString contentId)
        {
            // must match content id, otherwise cancelling another game's download/update can wipe the params
            if (sRtpLaunchGameContentId != contentId)
                return;

            sRtpLaunchActive = false;
            sRtpLaunchCmdParams = "";

            ORIGIN_LOG_EVENT << "RTP Info CLEARED";
        }

        QString RTPFlow::getRtpLaunchParams(const QString contentId)
        {
            QString temp = "";

            // must match content id, otherwise another game can launch with these params
            if (sRtpLaunchGameContentId == contentId)
            {
                temp = sRtpLaunchCmdParams;
                clearRtpLaunchInfo(contentId);
            }

            return temp;
        }

        void RTPFlow::setPendingLaunch(const QStringList& idList, const QString& gameTitle, const QString& cmdParams, bool autoDownload, bool restartEntitlement, const QString& itemSubType, const QString &id, bool forceOnline)
        {
            ORIGIN_LOG_EVENT << "--------------------- RTP SetPendingLaunch ---- ";
            //	CRASH POINT:  2EEF
            //
            ORIGIN_LOG_EVENT << "Game Title: " << gameTitle;
            ORIGIN_LOG_EVENT << "Auto Download: " << autoDownload;
            Origin::Services::LoggerFilter::DumpCommandLineToLog("RTP Param", cmdParams);

            if (mLaunchGameIdList.count() > 0)
            {
                ORIGIN_LOG_EVENT << "An RTP launch, " << mLaunchGameIdList[0] << ", is already pending!";
                return;
            }

            mUUID = id;

            mRTPStatus = GameLaunchCheckingLogin;
            mLaunchGameIdList = idList;
            mLaunchGameTitle = gameTitle;
            mLaunchGameCmdParams = 	UnicodeUnescapeQString(cmdParams);  // Remove any URL escaping done from Access/RtP wrapper
            mbCodeRedeemed = false;
            mRetryOnceAfterInitialRefresh = false;
            mLaunchGameRestart = restartEntitlement;
            mAutoDownload = autoDownload;
            mForceOnline = forceOnline;
            mRetryCount = 0;

            // Try the launch. If the game isn't ready we can either try downloading it
            // or just tell the user they need to fix the problem.
            if (autoDownload)
            {
                mLaunchGameFailedAction = ifLaunchFailsTryDownload;
            }
            else
            {
                // For Non Origin games, the default behavior is to do nothing if the user no longer has nog in their game library
                // Item sub type will only be present when launched via a jumplist entry
                if(!itemSubType.isEmpty() && itemSubType.toInt() == Origin::Services::Publishing::ItemSubTypeNonOrigin)
                {
                    mLaunchGameFailedAction = ifLaunchFailsDoNothing;
                }
                else
                {
                    mLaunchGameFailedAction = ifLaunchFailsShowDialog;
                }
            }

            attemptGameLaunch(mLaunchGameFailedAction);
        }

        bool RTPFlow::startClientMinimized()
        {
            //first check and see if there's a pending RTP
            //and if it's not autodownload, then start the client minimized
            if (isPending())
            {
                if (mLaunchGameFailedAction != ifLaunchFailsTryDownload)
                    return true;
            }
            return false;
        }

        void RTPFlow::start()
        {
        }

        //need to wait for the ClientFlow to start, not login, because it single login check happens AFTER login so we need to make sure we wait for the result of that
        //before starting RTP
        void RTPFlow::onClientFlowStarted ()
        {
            ORIGIN_VERIFY_DISCONNECT (Origin::Client::MainFlow::instance(), SIGNAL (startPendingAction()), this, SLOT(onClientFlowStarted()));

            Origin::Engine::Content::ContentController *contentController = Origin::Engine::Content::ContentController::currentUserContentController();
            //check and see if we've already finished receiving the entitlements
            if(contentController && contentController->initialEntitlementRefreshCompleted())
            {
                //since we won't have the autoupdate list, check and see if the flow for this content is active to see if it is autoupdating
                QString id;
                foreach(id, mLaunchGameIdList)
                {
                    Origin::Engine::Content::EntitlementRef entRef = contentController->entitlementById(id);
                    if (!entRef.isNull() && entRef->localContent()->installFlow() && entRef->localContent()->installFlow()->isActive())
                    {
                        mAutoUpdating = true;
                        break;
                    }
                }
                onEntitlementContentUpdated();
            }
            else
            {
                ORIGIN_VERIFY_CONNECT_EX(contentController, SIGNAL(autoUpdateCheckCompleted(bool, QStringList)), this, SLOT(onAutoUpdateCheckCompleted(bool, QStringList)), Qt::QueuedConnection);
                ORIGIN_VERIFY_CONNECT_EX(contentController, SIGNAL(updateFailed()), this, SLOT(onEntitlementContentUpdated()), Qt::QueuedConnection);
            }
        }

        void RTPFlow::onAutoUpdateCheckCompleted (bool autoUpdating, QStringList autoUpdateList)
        {
            ORIGIN_LOG_EVENT << "onAutoUpdateCheckCompleted : " << mPendingEntitlementFlag;

            QString contentId;
            foreach(contentId, mLaunchGameIdList)
            {
                if (autoUpdateList.contains (contentId))
                {
                    mAutoUpdating =  true;
                    break;
                }
            }

            mPendingEntitlementFlag &= ~RTP_WAITINGFOR_UPDATE;

            if (mPendingEntitlementFlag != RTP_NOTWAITING)
            {
                return; // wait for onExtraContentUpdate() to trigger the next attemptGameLaunch()
            }

            if(!mRedemptionActive)
                onEntitlementContentUpdated();
        }

        void RTPFlow::onExtraContentUpdate()
        {
            ORIGIN_LOG_EVENT << "onExtraContentUpdate : " << mPendingEntitlementFlag;

            if (mPendingEntitlementFlag == RTP_NOTWAITING)
                return; // we don't need to do anything now

            mPendingEntitlementFlag &= ~RTP_WAITINGFOR_EXTRACONTENT;

            if (mPendingEntitlementFlag != RTP_NOTWAITING)
                return; // still waiting for refresh(new) so do nothing

            if (!mRedemptionActive)
                onEntitlementContentUpdated();
        }

        void RTPFlow::onEntitlementContentUpdated()
        {
            attemptGameLaunch(mLaunchGameFailedAction);
        }

        void RTPFlow::onRetryGameStart(Origin::Engine::Content::EntitlementRef entitlementRef)
        {
            ORIGIN_VERIFY_DISCONNECT(entitlementRef->localContent(), NULL, this, SLOT(onRetryGameStart(Origin::Engine::Content::EntitlementRef)));

            if(mLaunchGameIdList.size() > 0)
            {
                attemptGameLaunch(mLaunchGameFailedAction);
            }
        }

        void RTPFlow::onRedeemButton()
        {
            using namespace Origin::Services::Session;
            SessionRef session = SessionService::currentSession();
            bool online = Origin::Services::Connection::ConnectionStatesService::isUserOnline(session);
            if ( online )
            {

                // EBIBUGS-20151
                //
                //  The client cycling online-offline-online triggers a content controller refresh, so unless we track that we're
                //  already in the redemption "flow" we'll wind up showing both the redemption page _and_ the activate dialog at the same time.
                //
                //  These signals track the lifetime of the redemption window and help us prevent that from happening.
                //
                QObject::connect(Origin::Client::ClientFlow::instance(), SIGNAL(redemptionPageShown()), this,
                                 SLOT(onRedemptionPageShown()), Qt::UniqueConnection);

                QObject::connect(Origin::Client::ClientFlow::instance(), SIGNAL(redemptionPageClosed()), this,
                                 SLOT(onRedemptionPageClosed()), Qt::UniqueConnection);

                // Take user to the redeem dialog
                Origin::Client::ClientFlow::instance()->showRedemptionPage(RedeemBrowser::RTP, RedeemBrowser::OriginCodeRTP, QString());

                //make these queued connections to allow for onImportComplete to finish before we go thru the bulk of the RTP flow
                ORIGIN_VERIFY_CONNECT_EX(Origin::Engine::Content::ContentController::currentUserContentController(), SIGNAL(autoUpdateCheckCompleted(bool, QStringList)), this, SLOT(onAutoUpdateCheckCompleted(bool, QStringList)), Qt::QueuedConnection);
                ORIGIN_VERIFY_CONNECT_EX(Origin::Engine::Content::ContentController::currentUserContentController(), SIGNAL(updateFailed()), this, SLOT(onEntitlementContentUpdated()), Qt::QueuedConnection);
            }
            else
            {
                mRTPStatus = GameLaunchFailed;
                clearPendingLaunch();
                mRTPViewController->showOfflineErrorDialog();
            }
        }

        void RTPFlow::onNotThisUserButton()
        {
            // Keep game info and monitor entitlements for requested game to perform delayed launch
            mLaunchGameFailedAction = ifLaunchFailsShowDialog;
            //connect signals for logging back in
            ORIGIN_VERIFY_CONNECT_EX (Origin::Client::MainFlow::instance(), SIGNAL (startPendingAction()), this, SLOT(onClientFlowStarted ()), Qt::QueuedConnection);
            MainFlow::instance()->logout(ClientLogoutExitReason_Logout_Silent);
        }

        void RTPFlow::onPurchaseGame(const QString& masterTitleId)
        {
            mRTPStatus = GameLaunchFailed;
            clearPendingLaunch();
            Client::ClientFlow::instance()->showMasterTitleInStore(masterTitleId);
        }

        void RTPFlow::onRenewSubscriptionMembership()
        {
            mRTPStatus = GameLaunchFailed;
            clearPendingLaunch();
            Origin::Client::ClientFlow::instance()->showAccountProfileSubscription();
        }

        void RTPFlow::onGoOnline()
        {
            mRTPStatus = GameLaunchFailed;
            clearPendingLaunch();
            Origin::Client::ClientFlow::instance()->requestOnlineMode();
        }

        void RTPFlow::onShowDeveloperTool()
        {
            // Closed / Cancelled?
            clearPendingLaunch();
            mRTPStatus = GameLaunchFailed;
            ORIGIN_LOG_EVENT << "Stopping RTP and showing developer tool";
            ClientFlow::instance()->showDeveloperTool();
        }

        //this listens for state change to READY_TO_PLAY so we can launch the game
        void RTPFlow::onDownloadStateChanged (Origin::Engine::Content::EntitlementRef entitlementRef)
        {
            //this should never happen since we're specifically listening for state change of the local content of the RTP download title but...
            if (entitlementRef->contentConfiguration()->contentId().compare (sRtpLaunchGameContentId) != 0)
            {
                //now check offer id too
                if(entitlementRef->contentConfiguration()->productId().compare (sRtpLaunchGameContentId) != 0)
                    return;
            }
            Origin::Engine::Content::LocalContent::States lstate = entitlementRef->localContent()->state();
            if (lstate == Origin::Engine::Content::LocalContent::ReadyToPlay)
            {
                //disconnect the signal
                ORIGIN_VERIFY_DISCONNECT (entitlementRef->localContent(), SIGNAL(stateChanged(Origin::Engine::Content::EntitlementRef)), this, SLOT (onDownloadStateChanged (Origin::Engine::Content::EntitlementRef)));

                //ok we're done downloading so now go to launch the game
                //restore mLaunchGameIdList and mLaunchGameCmdParams it got cleared the first time we went thru to start the download
                mLaunchGameIdList.append (sRtpLaunchGameContentId);
                mLaunchGameCmdParams = sRtpLaunchCmdParams;
                attemptGameLaunch(mLaunchGameFailedAction);
            }
            else if (lstate == Origin::Engine::Content::LocalContent::ReadyToDownload)
            {
                //an error occurred or the user canceled the download
                mRTPStatus = GameLaunchFailed;
            }
        }

        //this listens for kCanceling so we can disconnect the signals
        void RTPFlow::onInstallFlowStateChanged (Origin::Downloader::ContentInstallFlowState newState)
        {
            if (newState == Origin::Downloader::ContentInstallFlowState::kCanceling)
            {
                Origin::Engine::Content::EntitlementRef theEntitlement = Origin::Engine::Content::ContentController::currentUserContentController()->entitlementByContentId((sRtpLaunchGameContentId));
                if (theEntitlement)
                {
                    if (theEntitlement->localContent())
                        ORIGIN_VERIFY_DISCONNECT (theEntitlement->localContent(), SIGNAL(stateChanged(Origin::Engine::Content::EntitlementRef)), this, SLOT (onDownloadStateChanged (Origin::Engine::Content::EntitlementRef)));
                    if (theEntitlement->localContent()->installFlow())
                        ORIGIN_VERIFY_DISCONNECT (theEntitlement->localContent()->installFlow(), SIGNAL (stateChanged (Origin::Downloader::ContentInstallFlowState)), this, SLOT (onInstallFlowStateChanged(Origin::Downloader::ContentInstallFlowState)));
                }
                clearRtpLaunchInfo(sRtpLaunchGameContentId);
            }
        }

        // Counterpart specifically for StringHelpers of UnicodeEscapeCString used in Access/RtP wrapper
        QString UnicodeUnescapeQString(QString sStringToDecode)
        {
            // We only supported a handful of characters so spare the RegExp etc. and just call 'em out with simple string replacements
            QString sProcessed = sStringToDecode;

            const QString sCharSet = " <>#%{}|\\^~[]';/?:@=&$+\"";
            for (int i = sCharSet.length(); i--; )
            {
                QChar cChr = sCharSet.at(i);
                QString sSrch = QString("%%1").arg(cChr.unicode(), 2, 16);
                sProcessed.replace(sSrch, cChr);
            }
            return sProcessed;
        }

        void RTPFlow::clearPendingLaunch()
        {
            // Clear current objective
            mLaunchGameIdList.clear();
            mLaunchGameTitle.clear();
            mbCodeRedeemed = false;
            mAutoUpdating = false;
            mAutoDownload = false;
            mForceOnline = false;
            ORIGIN_VERIFY_DISCONNECT(Origin::Engine::Content::ContentController::currentUserContentController(), SIGNAL(autoUpdateCheckCompleted(bool, QStringList)), this, SLOT(onAutoUpdateCheckCompleted(bool, QStringList)));
            ORIGIN_VERIFY_DISCONNECT(Origin::Engine::Content::ContentController::currentUserContentController(), SIGNAL(updateFailed()), this, SLOT(onEntitlementContentUpdated()));
            //Clear SSO Token in case it's still there
            MainFlow::instance()->consumeUrlParameter(Origin::Services::SETTING_UrlSSOtoken);
            MainFlow::instance()->consumeUrlParameter(Origin::Services::SETTING_UrlSSOauthCode);
            MainFlow::instance()->consumeUrlParameter(Origin::Services::SETTING_UrlSSOauthRedirectUrl);
        }

        void RTPFlow::showGamesTabNowOrOnLogin( const QString offerId )
        {
            // TODO: Need a way to make mygames page scroll to show the particular content
            if (Engine::LoginController::isUserLoggedIn() && ClientFlow::instance())
            {
                ClientFlow::instance()->showMyGames();
            }
            else
            {
                //have it show the mygames on client startup
                OriginApplication::instance().SetExternalLaunchStartupTab(TabMyGames);
            }
        }

        bool RTPFlow::shouldMainWindowMinimize() const
        {
            return !mAutoDownload && !getShowProductPage();
        }

        void RTPFlow::onUserForcedOnline(bool isOnline, Origin::Services::Session::SessionRef session)
        {
            if (isOnline) {
                ORIGIN_VERIFY_DISCONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(userConnectionStateChanged(bool, Origin::Services::Session::SessionRef)),
                                         this, SLOT(onUserForcedOnline(bool, Origin::Services::Session::SessionRef)));
                attemptGameLaunch(mLaunchGameFailedAction);
            }
        }

        bool RTPFlow::attemptGameLaunch(LaunchFailedAction aLaunchFailedAction)
        {
            ORIGIN_LOG_EVENT << "RTP: in AttemptGameLaunch " << aLaunchFailedAction;

            mShowProductPage = false;
            Origin::Engine::Content::ContentController *contentController = Origin::Engine::Content::ContentController::currentUserContentController();

            //If the force online flag is on and the RTP flow starts with the user being offline, attempt to go online at this point.
            if (mForceOnline)
            {
                Origin::Services::Connection::ConnectionStatesService* connectService = Origin::Services::Connection::ConnectionStatesService::instance();
                if (Engine::LoginController::isUserLoggedIn() && !connectService->isUserOnline(Engine::LoginController::instance()->currentUser()->getSession()))
                {
                    ORIGIN_LOG_EVENT << "RTP Launch requested force online, user is offline, forcing online";
                    MainFlow::instance()->socialGoOnline();
                    ORIGIN_VERIFY_CONNECT(connectService, SIGNAL(userConnectionStateChanged(bool, Origin::Services::Session::SessionRef)), this, SLOT(onUserForcedOnline(bool, Origin::Services::Session::SessionRef)));
                    return false;
                }
            }
            //we should check for user's entitlements only if we're actually logged in AND clientFlow has started
            if (Engine::LoginController::isUserLoggedIn() && Origin::Client::ClientFlow::instance())
            {
                mRTPStatus = GameLaunchPending;
                ORIGIN_LOG_EVENT << "User is logged in";

                Q_ASSERT(!mLaunchGameIdList.isEmpty());

                //another safety check, if the list is empty, we're not going to find the entitlement so don't do anything
                if (mLaunchGameIdList.isEmpty())
                {
                    ORIGIN_LOG_EVENT << "Content list is empty";
                    mRTPStatus = GameLaunchFailed;
                    return false;
                }

                // Search current content for requested game
                Origin::Engine::Content::EntitlementRef theEntitlement;
                QString id;

                //MY: the user may own multiple entitlements in the mLaunchGameIdList
                //pre-order, different locale, etc.
                //and they may each be in non-playable() state, so we want to make sure to grab the one that is playable()
                //if there are none in the list, then we want to display the error message in order of precedence
                //a. installed() but not playable()
                //b. not installed()
                //c. not owned
                Origin::Engine::Content::EntitlementRef installedEntRef = Origin::Engine::Content::EntitlementRef();    //installed but not playable()
                Origin::Engine::Content::EntitlementRef installableEntRef = Origin::Engine::Content::EntitlementRef();  //installable, i.e. not pre-order, etc.
                Origin::Engine::Content::EntitlementRef fallbackEntRef = Origin::Engine::Content::EntitlementRef(); //neither of the cases, not sure when that would happen but just in case
                Origin::Engine::Content::EntitlementRef trialEntRef = Origin::Engine::Content::EntitlementRef(); //playable trial available

                bool exitOuterLoop = false;

                //presearch the list for one that is actually playable()
                foreach(id, mLaunchGameIdList)
                {
                    // Lookup an owned entitlement matching this content ID.
                    // if contentId is passed, it's possible that there may be more than one entitlement with the same contentId (unlikely, but possible)
                    // so retrieve a list of entitlements with the same contentId and iterate thru that
                    const QList<Origin::Engine::Content::EntitlementRef > &entitlementList = contentController->entitlementsById(id);
                    const QList<Origin::Engine::Content::EntitlementRef > &entitlementTrialList = contentController->entitlementsByTimedTrialAssociation(id);

                    theEntitlement.clear();

                    foreach(auto ent, entitlementTrialList)
                    {
                        bool isBlockedByReleaseControl = ent->localContent()->playableStatus(true) == Origin::Engine::Content::LocalContent::PS_ReleaseControl;
                        bool isTrialTimeRemainingUpdated = ent->localContent()->isTimeRemainingUpdated();
                        bool isInstalled = ent->localContent()->installed();
                        bool hasInvalidDuration = ent->localContent()->trialTimeRemaining(true) < 0;

                        if(ent->localContent()->playable() || ent->localContent()->playing() || (isBlockedByReleaseControl && (!isTrialTimeRemainingUpdated || hasInvalidDuration) && isInstalled))
                        {
                            trialEntRef = ent;
                            exitOuterLoop = true; //break out of the outer loop too
                            break;
                        }
                    }

                    foreach (auto ent, entitlementList)
                    {
                        // If the entitlement is non-null,
                        if ( ! ent.isNull() )
                        {
                            // If this platform is supported by this entitlement,
                            if ( ent->contentConfiguration()->platformEnabled( Services::PlatformService::PlatformThis ) )
                            {
                                if (ent->localContent()->playing()) //if we're already playing then break out of the search
                                {
                                    theEntitlement = ent;
                                    exitOuterLoop = true; //break out of the outer loop too
                                    break;
                                }
                                else if (ent->localContent()->playable()) //found one that is playable
                                {
                                    // Break out of the search.
                                    theEntitlement = ent;
                                    exitOuterLoop = true; //break out of the outer loop too
                                    break;
                                }
                                else    //need to keep track of why it's not playable so we can provide the correct error dialog in the event we don't find one that's playable
                                {
                                    bool isTrial = (ent->contentConfiguration()->itemSubType() == Services::Publishing::ItemSubTypeTimedTrial_Access ||
                                                    ent->contentConfiguration()->itemSubType() == Services::Publishing::ItemSubTypeTimedTrial_GameTime);
                                    bool isInstalled = ent->localContent()->installed();
                                    bool isBlockedByReleaseControl = ent->localContent()->playableStatus(true) == Origin::Engine::Content::LocalContent::PS_ReleaseControl;
                                    bool isTrialTimeRemainingUpdated = ent->localContent()->isTimeRemainingUpdated();

                                    if(isTrial && isInstalled && isBlockedByReleaseControl && (!isTrialTimeRemainingUpdated))
                                    {
                                        // Retry for when the entitlement has been updated.
                                        mRetryCount++;

                                        if(mRetryCount < MAX_RETRY_COUNT)
                                        {
                                            aLaunchFailedAction = ifLaunchFailsDoNothing;

                                            // Try again when the entitlement changes.
                                            ORIGIN_VERIFY_CONNECT(ent->localContent(), SIGNAL(stateChanged(Origin::Engine::Content::EntitlementRef)),
                                                                  this, SLOT(onRetryGameStart(Origin::Engine::Content::EntitlementRef)));

                                            exitOuterLoop = true; //break out of the outer loop too
                                        }
                                        else
                                        {
                                            // fail the entitlement
                                            mRetryCount = 0;
                                            theEntitlement = ent;
                                        }
                                    }
                                    else
                                    {
                                        //check for the status to allow us to choose precedence when showing the error
                                        Origin::Engine::Content::LocalContent::PlayableStatus status = ent->localContent()->playableStatus(true);

                                        //currently, the check for PS_ReleaseControll happens after the check for installed() but in case that is changed
                                        //just double check that it's installed
                                        if (ent->localContent()->installed(true) && status == Origin::Engine::Content::LocalContent::PS_ReleaseControl)
                                        {
                                            installedEntRef = ent;
                                        }
                                        else if (status == Origin::Engine::Content::LocalContent::PS_NotInstalled)
                                        {
                                            installableEntRef = ent;
                                        }
                                        else
                                            fallbackEntRef = ent;
                                    }
                                }
                            }
                            // Otherwise,
                            else
                            {
                                ORIGIN_LOG_WARNING << "RTPFlow: Entitlement found for ID [" << id << "] that is not supported on this platform.  Continuing search.";

                                // EBIBUGS-24139
                                // If the user was granted an entitlement from an external source (i.e. Demos page of store), there's no guarantee that they
                                // can launch it on the active platform.  If the entitlement was granted successfully but it's not supported, don't show the
                                // activation dialog.
                                aLaunchFailedAction = ifLaunchFailsDoNothing;
                            }
                        }
                    } //foreach entitlementList

                    if (exitOuterLoop)
                        break;
                } //foreach mLaunchGameList

                if (theEntitlement.isNull())    //didn't find one that could be launched
                {
                    if(!trialEntRef.isNull())   // There is a trial, and it is installed.
                    {
                        if(!trialEntRef->localContent()->isTimeRemainingUpdated())  // Are we waiting for the timeRemaining to be updated from the server?
                        {
                            mRetryCount++;

                            if(mRetryCount < MAX_RETRY_COUNT)
                            {
                                // Retry for when the entitlement has been updated.
                                aLaunchFailedAction = ifLaunchFailsDoNothing;

                                // Try again when the entitlement changes.
                                ORIGIN_VERIFY_CONNECT(trialEntRef->localContent(), SIGNAL(stateChanged(Origin::Engine::Content::EntitlementRef)),
                                                      this, SLOT(onRetryGameStart(Origin::Engine::Content::EntitlementRef)));
                            }
                            else
                            {
                                theEntitlement = trialEntRef; // let the trial fail on not being playable.
                            }
                        }
                        else
                        {
                            theEntitlement = trialEntRef;	//use the launchable trial instead.
                        }
                    }
                    else if (!installedEntRef.isNull()) //use the one that we found was installed
                        theEntitlement = installedEntRef;
                    else if (!installableEntRef.isNull()) //use the one that can be installed
                        theEntitlement = installableEntRef;
                    else
                        theEntitlement = fallbackEntRef;
                }

                if (!theEntitlement.isNull())
                {
                    emit entitlementFound();

                    ORIGIN_VERIFY_DISCONNECT(contentController, SIGNAL(autoUpdateCheckCompleted(bool, QStringList)), this, SLOT(onAutoUpdateCheckCompleted(bool, QStringList)));
                    ORIGIN_VERIFY_DISCONNECT(contentController, SIGNAL(updateFailed()), this, SLOT(onEntitlementContentUpdated()));
                    ORIGIN_VERIFY_DISCONNECT(contentController, SIGNAL(initialFullUpdateCompleted()), this, SLOT(onExtraContentUpdate()));

                    ORIGIN_LOG_EVENT << "Entitlement found, signal disconnected";

                    bool saveOffautoUpdateStatus = mAutoUpdating; //since clearPendingLaunch resets mAutoUpdating

                    clearPendingLaunch();

                    //since we found the entitlement and cleared the pending launch flags
                    //we also want to clear the redemption states and dialogs slots
                    //otherwise we will end up with two activate dialogs
                    clearRedemptionStates();

                    if (theEntitlement->localContent()->playing() && mLaunchGameRestart)
                    {
                        ORIGIN_LOG_EVENT << "RTP: Game already playing, detected restarting so detaching from current process to allow creation of new process.";
                        theEntitlement->localContent()->prepareForRestart();
                    }

                    // Prevent launching the game if it's already playing, unless it's
                    // specifically configured to allow multiple launches via the
                    // allowMultipleInstances DiP manifest flag.
                    const bool allowMultipleInstances = theEntitlement->localContent()->allowMultipleInstances();
                    if (theEntitlement->localContent()->playing() && !allowMultipleInstances)
                    {
                        ORIGIN_LOG_EVENT << "RTP: Game already playing";
                        clearRtpLaunchInfo(id);	// make sure we clear out the launch info
                        mRTPStatus = GameLaunchAlreadyPlaying;
                        return false;
                    }

                    //bool is_patching = (theEntitlement->IsPatchingForRTP();	// we need to always check this because if an update is available
                    // we need to set the is_patching flag so the pop-up appears to the user
                    bool is_patching = false;
                    bool is_repairing_or_installing = false;

                    if (theEntitlement->localContent()->installFlow())
                    {
                        if (theEntitlement->localContent()->installFlow()->isActive())
                        {
                            if (theEntitlement->localContent()->installFlow()->isInstallProgressConnected() || theEntitlement->localContent()->installFlow()->isRepairing())
                                is_repairing_or_installing = true;
                            else
                                is_patching = theEntitlement->localContent()->installFlow()->isUpdate();
                        }
                    }

                    //mAutoUpdating could be false if it's a resume of a previously started download
                    //but we should never have the came where mAutoUpdating is true but is_patching is false because it means we're going
                    //to miss the fact that we need to wait for the flow to start
                    if (saveOffautoUpdateStatus && !is_patching)
                    {
                        ORIGIN_ASSERT_MESSAGE (0, "AutoUpdating but flow doesn't think so");
                    }

                    // In addition to while patching and playable state, we will allow launching the
                    // content if it's already playing and the content is configured to allow
                    // multiple instances.
                    const bool playingAndAllowsMultipleInstances = theEntitlement->localContent()->playing() && allowMultipleInstances;
                    if (is_patching || theEntitlement->localContent()->playable() || playingAndAllowsMultipleInstances)
                    {
                        RTPFlow::sRtpLaunchActive = true;
                        RTPFlow::sRtpLaunchCmdParams = mLaunchGameCmdParams;
                        RTPFlow::sRtpLaunchGameContentId = id;

                        // Check to see if the "Game is updating, do you still want to play?" window is up - don't launch if it is. Doing so will bypass the Play Now w/o Updates.
                        // but put this check after the parameter setup so when the user decides to Play Now, the RtP params are setup.
                        PlayFlow* iFlow = MainFlow::instance()->contentFlowController()->playFlow(theEntitlement->contentConfiguration()->productId());
                        if (iFlow && iFlow->isPlayGameWhileUpdatingWindowVisible())
                        {
                            ORIGIN_LOG_EVENT << "RTP: Updating already with dialog up, don't launch";
                            mRTPStatus = GameLaunchFailed;
                            return false;
                        }

                        if (shouldMainWindowMinimize())
                        {
                            ClientFlow *clientFlow = ClientFlow::instance();
                            if (clientFlow)
                            {
                                //check and see if we're already minimized to the system tray, if so, don't do anything
                                if (clientFlow->isMainWindowVisible())
                                    clientFlow->minimizeClientWindow(true);
                            }
                        }

                        ORIGIN_LOG_EVENT << "RTP: startPlayFlow: " << theEntitlement->contentConfiguration()->productId();
                        GetTelemetryInterface()->SetGameLaunchSource(EbisuTelemetryAPI::Launch_Desktop);
                        MainFlow::instance()->contentFlowController()->startPlayFlow (theEntitlement->contentConfiguration()->productId(), false, mLaunchGameCmdParams);  // let playNow check for patches and bring up Update Progress bar
                        // btw, ClearRtpLaunchInfo() gets called in Entitlement::directlyLaunchContent()
                        mRTPStatus = GameLaunchSuccess;

                        // EBIBUGS-14999 - crash on cancel and logout during RtP launch -
                        // if the user logs out before we get back from playNow, theEntitlement will no longer be valid.
                        // but we use it below in the call to ShowGamesTabNowOrOnLogin().  So if the user is logged out, just return.
                        if (!Engine::LoginController::isUserLoggedIn())
                        {
                            mRTPStatus = GameLaunchFailed;
                            return false;
                        }
                    }
                    else if (aLaunchFailedAction == ifLaunchFailsTryDownload)
                    {
                        //first we want to start the download and listen for the finish
                        //when that finishes, then we want to autolaunch
                        RTPFlow::sRtpLaunchGameContentId = id;
                        //make sure it is ready to download
                        if (!theEntitlement->localContent()->downloadable())
                        {
                            ORIGIN_LOG_ERROR << "RTP: autodownload: " << id << " - state not ready to download! [ " << theEntitlement->localContent()->state() << "]";
                            mRTPStatus = GameLaunchFailed;
                            return false;
                        }

                        //we want to wait for READY_TO_PLAY
                        ORIGIN_VERIFY_CONNECT (theEntitlement->localContent(), SIGNAL(stateChanged(Origin::Engine::Content::EntitlementRef)), this, SLOT (onDownloadStateChanged (Origin::Engine::Content::EntitlementRef)));
                        //but we also need to check for download cancel
                        if (theEntitlement->localContent()->installFlow())
                            ORIGIN_VERIFY_CONNECT (theEntitlement->localContent()->installFlow(), SIGNAL (stateChanged (Origin::Downloader::ContentInstallFlowState)), this, SLOT (onInstallFlowStateChanged(Origin::Downloader::ContentInstallFlowState)));

                        theEntitlement->localContent()->download(Origin::Engine::LoginController::instance()->currentUser()->locale());
                        mRTPStatus = GameLaunchDownloading;
                        showGamesTabNowOrOnLogin( theEntitlement->contentConfiguration()->productId() );
                    }
                    else
                    {
                        //we could fall into here even if the game is installed, can't launch for ORC reasons
                        //but we only want to show the dialog if the game isn't installed
                        ORIGIN_LOG_EVENT << "R2P error";
                        if (!theEntitlement->localContent()->installed(true))
                        {
                            mRTPViewController->showGameShouldBeInstalledErrorDialog (theEntitlement->contentConfiguration()->displayName());
                        }
                        else if (!theEntitlement->localContent()->playable())
                        {
                            //pretty sure this would be because of ORC but double check
                            if (theEntitlement->localContent()->releaseControlState() == Origin::Engine::Content::LocalContent::RC_Preload ||
                                    theEntitlement->localContent()->releaseControlState() == Origin::Engine::Content::LocalContent::RC_Unreleased)
                            {
                                QString releaseDate;
                                const int daysToReveal = -365;

                                QDateTime localDateTime = theEntitlement->contentConfiguration()->releaseDate().toLocalTime();

                                //see if we are a year out, if we are then don't show a date
                                QDateTime localDateTimeCheckTime = localDateTime.addDays(daysToReveal);
                                if(localDateTimeCheckTime <= QDateTime::currentDateTime())
                                {
                                    releaseDate = LocalizedDateFormatter().format(localDateTime.date(), LocalizedDateFormatter::LongFormat);
                                }

                                mRTPViewController->showGameUnreleasedErroDialog(theEntitlement->contentConfiguration()->displayName(), releaseDate);
                            }
                            // If this is an timed trial game and it was disabled by admin (indicated by date in past)
                            else if ((theEntitlement->contentConfiguration()->itemSubType() == Origin::Services::Publishing::ItemSubTypeTimedTrial_Access 
                                      || theEntitlement->contentConfiguration()->itemSubType() == Origin::Services::Publishing::ItemSubTypeTimedTrial_GameTime)
                                && theEntitlement->localContent()->trialTimeRemaining() == Engine::Content::LocalContent::TrialError_DisabledByAdmin)
                            {
                                mRTPViewController->showEarlyTrialDisabledByAdminDialog(theEntitlement);
                            }
                            // could be that the free trial expired
                            else if (theEntitlement->contentConfiguration()->isFreeTrial() && theEntitlement->localContent()->releaseControlState() == Origin::Engine::Content::LocalContent::RC_Expired)
                            {
                                if (theEntitlement->contentConfiguration()->itemSubType() == Origin::Services::Publishing::ItemSubTypeTimedTrial_Access
                                    || theEntitlement->contentConfiguration()->itemSubType() == Origin::Services::Publishing::ItemSubTypeTimedTrial_GameTime)
                                {
                                    // show timed trial dialog
                                    mRTPViewController->showEarlyTrialExpiredDialog(theEntitlement);
                                }
                                else
                                {
                                    // show free trial expired dialog
                                    mRTPViewController->showFreeTrialExpiredDialog(theEntitlement);
                                }
                            }
                            // could this be a subscription issue?
                            else if(theEntitlement->contentConfiguration()->isEntitledFromSubscription())
                            {
                                // Has the user's subscription offline play expired?
                                if(Engine::Subscription::SubscriptionManager::instance()->offlinePlayRemaining() <= 0)
                                {
                                    mRTPViewController->showSubscriptionOfflinePlayExpired();
                                }
#ifdef ORIGIN_MAC
                                else if(Engine::Subscription::SubscriptionManager::instance()->status() == server::SUBSCRIPTIONSTATUS_ENABLED 
                                     || Engine::Subscription::SubscriptionManager::instance()->status() == server::SUBSCRIPTIONSTATUS_PENDINGEXPIRED)
                                {
                                    QString masterTitleId = theEntitlement->contentConfiguration()->purchasable() ? theEntitlement->contentConfiguration()->masterTitleId() : "";
                                    mRTPViewController->showSubscriptionNotAvailableOnPlatform(Origin::Engine::LoginController::currentUser()->eaid(),
                                        getLaunchGameTitle(), Origin::Engine::LoginController::currentUser()->isUnderAge(), masterTitleId);
                                }
#endif
                                // Has the user's subscription membership expired?
                                else if(!Engine::Subscription::SubscriptionManager::instance()->isActive())
                                {
#ifdef ORIGIN_MAC
                                    // OFM-11003: Show the subscription activate since subs is disabled on mac
                                    QString masterTitleId = theEntitlement->contentConfiguration()->purchasable() ? theEntitlement->contentConfiguration()->masterTitleId() : "";
                                    mRTPViewController->showSubscriptionNotAvailableOnPlatform(Origin::Engine::LoginController::currentUser()->eaid(),
                                        getLaunchGameTitle(), Origin::Engine::LoginController::currentUser()->isUnderAge(), masterTitleId);
#else
                                    mRTPViewController->showSubscriptionMembershipExpired(theEntitlement, Origin::Engine::LoginController::currentUser()->isUnderAge());
#endif
                                }
                                // Has the entitlement retired from the subscriptions?
                                else if(Services::TrustedClock::instance()->nowUTC().secsTo(theEntitlement->contentConfiguration()->originSubscriptionUseEndDate()) <= 0)
                                {
                                    mRTPViewController->showEntitlementRetiredFromSubscription(theEntitlement, Origin::Engine::LoginController::currentUser()->isUnderAge());
                                }
                            }
                            else if(theEntitlement->localContent()->playableStatus() == Engine::Content::LocalContent::PS_WrongArchitecture)
                            {
                                mRTPViewController->showSystemRequirementsNotMetDialog(theEntitlement);
                            }
                            else if(theEntitlement->localContent()->playableStatus() == Engine::Content::LocalContent::PS_TrialNotOnline)
                            {
                                if (theEntitlement->contentConfiguration()->itemSubType() == Origin::Services::Publishing::ItemSubTypeTimedTrial_Access
                                    || theEntitlement->contentConfiguration()->itemSubType() == Origin::Services::Publishing::ItemSubTypeTimedTrial_GameTime)
                                {
                                    mRTPViewController->showOnlineRequiredForEarlyTrialDialog(theEntitlement->contentConfiguration()->displayName());
                                }
                                else
                                {
                                    mRTPViewController->showOnlineRequiredForTrialDialog();
                                }
                            }
                            else if (is_repairing_or_installing)
                            {
                                mRTPViewController->showBusyDialog();
                            }
                        }

                        mRTPStatus = GameLaunchFailed;
                        return false;
                    }

                    return true;
                }
                else if (aLaunchFailedAction != ifLaunchFailsDoNothing)
                {
                    //we're here because we can't find the RTP entitlement
                    ORIGIN_LOG_EVENT << "RTP: content " << id << " not found";

                    //if the initial refresh has happend, then we issue a second one just in case the new entitlement has been added since then
                    if(contentController && contentController->initialEntitlementRefreshCompleted())
                    {
                        ORIGIN_LOG_EVENT << "RTP: initial refresh completed ";
                        ORIGIN_LOG_DEBUG << "RTP: mRetryOnceAfterInitialRefresh: " << mRetryOnceAfterInitialRefresh;

                        if (!mRetryOnceAfterInitialRefresh)
                        {
                            //disconnect previous signal just in case
                            ORIGIN_VERIFY_DISCONNECT(contentController, SIGNAL(autoUpdateCheckCompleted(bool, QStringList)), this, SLOT(onAutoUpdateCheckCompleted(bool, QStringList)));
                            ORIGIN_VERIFY_DISCONNECT(contentController, SIGNAL(updateFailed()), this, SLOT(onEntitlementContentUpdated()));
                            //reconnect
                            ORIGIN_VERIFY_CONNECT_EX(contentController, SIGNAL(autoUpdateCheckCompleted(bool, QStringList)), this, SLOT(onAutoUpdateCheckCompleted(bool, QStringList)), Qt::QueuedConnection);
                            ORIGIN_VERIFY_CONNECT_EX(contentController, SIGNAL(updateFailed()), this, SLOT(onEntitlementContentUpdated()), Qt::QueuedConnection);
                            mPendingEntitlementFlag = RTP_WAITINGFOR_UPDATE;

                            ORIGIN_VERIFY_DISCONNECT(contentController, SIGNAL(initialFullUpdateCompleted()), this, SLOT(onExtraContentUpdate()));
                            ORIGIN_VERIFY_CONNECT_EX(contentController, SIGNAL(initialFullUpdateCompleted()), this, SLOT(onExtraContentUpdate()), Qt::QueuedConnection);
                            if (!contentController->initialFullEntitlementRefreshed())
                            {
                                ORIGIN_LOG_EVENT << "Extra content response pending";
                                mPendingEntitlementFlag |= RTP_WAITINGFOR_EXTRACONTENT;
                            }

                            contentController->refreshUserGameLibrary(Origin::Engine::Content::ContentUpdates);
                            mRetryOnceAfterInitialRefresh = true;
                            return false;
                        }
                    }
                    else //otherwise, wait for the updatefinished signal
                    {
                        //disconnect previous signal just in case
                        ORIGIN_VERIFY_DISCONNECT(contentController, SIGNAL(autoUpdateCheckCompleted(bool, QStringList)), this, SLOT(onAutoUpdateCheckCompleted(bool, QStringList)));
                        ORIGIN_VERIFY_DISCONNECT(contentController, SIGNAL(updateFailed()), this, SLOT(onEntitlementContentUpdated()));

                        ORIGIN_LOG_EVENT << "RTP: waiting for autoUpdateCheckComplete signal";
                        ORIGIN_VERIFY_CONNECT_EX(contentController, SIGNAL(autoUpdateCheckCompleted(bool, QStringList)), this, SLOT(onAutoUpdateCheckCompleted(bool, QStringList)), Qt::QueuedConnection);
                        ORIGIN_VERIFY_CONNECT_EX(contentController, SIGNAL(updateFailed()), this, SLOT(onEntitlementContentUpdated()), Qt::QueuedConnection);
                        return false;
                    }

                    ORIGIN_VERIFY_DISCONNECT(contentController, SIGNAL(initialFullUpdateCompleted()), this, SLOT(onExtraContentUpdate()));

                    // Present message to user
                    // Add a model user dialog to prompt user to switch accounts if logged into a non-entitled account
                    if(mbCodeRedeemed)
                    {
                        ORIGIN_LOG_EVENT << "Code redeemed error";

                        ORIGIN_VERIFY_DISCONNECT(contentController, SIGNAL(autoUpdateCheckCompleted(bool, QStringList)), this, SLOT(onAutoUpdateCheckCompleted(bool, QStringList)));
                        ORIGIN_VERIFY_DISCONNECT(contentController, SIGNAL(updateFailed()), this, SLOT(onEntitlementContentUpdated()));

                        mRTPViewController->showWrongCodeDialog (mLaunchGameTitle);
                        mbCodeRedeemed = false;
                        onRedeemButton();
                    }
                    else if (!mRedemptionActive)
                    {
                        ORIGIN_LOG_EVENT << "RTP: Unable to find entitlement, launching Activate dialog";

                        //we're here because our entitlement doesn't contain the contentID with which RTP was launched
                        ORIGIN_VERIFY_DISCONNECT(contentController, SIGNAL(autoUpdateCheckCompleted(bool, QStringList)), this, SLOT(onAutoUpdateCheckCompleted(bool, QStringList)));
                        ORIGIN_VERIFY_DISCONNECT(contentController, SIGNAL(updateFailed()), this, SLOT(onEntitlementContentUpdated()));

                        showActivateDialog();
                    }
                }
            }
            else if (!mLaunchGameIdList.isEmpty())
            {
                ORIGIN_LOG_EVENT << "User not yet logged in, Setup delayed launch";
                mRTPStatus = GameLaunchWaitingToLogin;
                //wait for user to login
                ORIGIN_VERIFY_CONNECT (Origin::Client::MainFlow::instance(), SIGNAL (startPendingAction()), this, SLOT(onClientFlowStarted ()));
            }
            // Game not launched
            ORIGIN_LOG_EVENT << "RTP: Game not launched";
            return false;
        }

        void RTPFlow::showActivateDialog()
        {
            const QString environment = Services::readSetting(Services::SETTING_EnvironmentName).toString();
            const QStringList environmentTypes = Services::SettingsManager::instance()->activeMachineEnvironments();

            // User has only ever logged into one environment
            if(environmentTypes.count() < 2)
            {
                ORIGIN_LOG_EVENT << QString("Current env: %0 / MACHINE ENV: (%1)")
                                        .arg(environment)
                                        .arg(environmentTypes.join(", "));

                const RTPViewController::rtpHiddenUI hiddenUI = Engine::LoginController::currentUser()->isUnderAge() ? RTPViewController::ShowNothing : RTPViewController::ShowOfAgeContent;
                mRTPViewController->showNotEntitledDialog(Engine::LoginController::currentUser()->eaid(),
                    getLaunchGameTitle(), hiddenUI, "");
            }
            else
            {
                const bool devModeEnabled = Services::readSetting(Services::SETTING_OriginDeveloperToolEnabled, Services::Session::SessionRef());
                const RTPViewController::rtpHiddenUI hiddenUI = devModeEnabled ? RTPViewController::ShowDeveloperMode : RTPViewController::ShowNothing;
                ORIGIN_LOG_EVENT << "Dev mode enabled: " << (devModeEnabled ? "true" : "false");
                mRTPViewController->showIncorrectEnvironment(getLaunchGameTitle(), environment, hiddenUI);
            }

            mRetryOnceAfterInitialRefresh = false; //want to be sure to force refresh again if user decides to get entitled
        }

        void RTPFlow::onCancelLaunch()
        {
            ORIGIN_LOG_EVENT << "Cancelled";

            // Closed / Cancelled?
            clearPendingLaunch();
            mRTPStatus = GameLaunchFailed;
        }

        void RTPFlow::onRedemptionPageShown()
        {
            mRedemptionActive = true;
        }

        void RTPFlow::clearRedemptionStates()
        {
            mRedemptionActive = false;

            Origin::Client::ClientFlow *clientFlow = Origin::Client::ClientFlow::instance();

            if(clientFlow)
            {
                ORIGIN_VERIFY_DISCONNECT(clientFlow, SIGNAL(redemptionPageShown()), this,
                                         SLOT(onRedemptionPageShown()));

                ORIGIN_VERIFY_DISCONNECT(clientFlow, SIGNAL(redemptionPageClosed()), this,
                                         SLOT(onRedemptionPageClosed()));
            }
        }

        void RTPFlow::onRedemptionPageClosed()
        {
            //onRedemptionPageClosed gets called when you complete the redeem process and when you exit early
            clearRedemptionStates();

            //we want to give the users another chance to see the activate dialog if they close the dialog.

            if(!mbCodeRedeemed)
            {
                showActivateDialog();
            }
        }

        void RTPFlow::setAutoDownload(bool flag)
        {
            mAutoDownload = flag;
        }

        void RTPFlow::clearManualOnlineSlot()
        {
            if (mForceOnline) //Only for if forceOnline is active
            {
                //Swap the Forced user online connection with the client flow started
                //This occurs when we try to put a user online but their access/refresh token is expired so they're logged out and in again instead
                ORIGIN_VERIFY_DISCONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(userConnectionStateChanged(bool, Origin::Services::Session::SessionRef)),
                                         this, SLOT(onUserForcedOnline(bool, Origin::Services::Session::SessionRef)));
                //As long as mForceOnline is true (which also implies the RTP Flow is still pending launch)
                //and the user is logged out, the RTP Flow should wait for the user to be logged in again in all cases.
                ORIGIN_VERIFY_DISCONNECT(Origin::Client::MainFlow::instance(), SIGNAL(startPendingAction()), this, SLOT(onClientFlowStarted()));
                ORIGIN_VERIFY_CONNECT(Origin::Client::MainFlow::instance(), SIGNAL(startPendingAction()), this, SLOT(onClientFlowStarted()));

            }
        }

    }
}
