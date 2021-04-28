//  ClientFlow.cpp
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#include "flows/ClientFlow.h"
#include "flows/IGOFlow.h"
#include "flows/ITOFlow.h"
#include "flows/JoinMultiplayerFlow.h"
#include "flows/MainFlow.h"
#include "flows/RTPFlow.h"
#include "flows/PendingActionFlow.h"

#include "widgets/client/source/ClientViewController.h"
#include "widgets/client/source/PDLCViewController.h"
#include "widgets/promoBrowser/source/PromoBrowserViewController.h"
#include "widgets/social/source/SocialViewController.h"

#include "engine/content/CloudContent.h"
#include "engine/content/ContentController.h"
#include "engine/dirtybits/DirtyBitsClient.h"
#include "engine/login/LoginController.h"
#include "engine/subscription/SubscriptionManager.h"
#include "engine/plugin/PluginManager.h"
#include "engine/login/LoginController.h"
#include "engine/login/User.h"
#include "engine/social/SocialController.h"
#include "engine/multiplayer/InviteController.h"
#include "engine/multiplayer/JoinOperation.h"

#include "services/common/VersionInfo.h"
#include "services/common/JsonUtil.h"
#include "services/config/OriginConfigService.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/platform/EnvUtils.h"
#include "services/platform/PlatformService.h"
#include "services/session/SessionService.h"
#include "services/rest/AuthPortalServiceClient.h"

#include "ContentFlowController.h"
#include "NavigationController.h"
#include "OriginApplication.h"
#include "TelemetryAPIDLL.h"
#include "engine/content/PlayFlow.h"
#include "PlayViewController.h"
#include "OriginToastManager.h"
#include "Qt/originwindow.h"
#include "Qt/originwindowmanager.h"

#include <QDesktopServices>

namespace
{
    const QString INICIS_SSO_URL_AUTHCODE_REPLACE_TOKEN = "{code}";

    /// \brief Constructs the Inicis SSO checkout URL and launches the URL in the user's external browser.
    void launchBrowserWithInicisSsoCheckoutUrl(const QString& urlParams)
    {
        QString fullUrl(Origin::Services::readSetting(Origin::Services::SETTING_InicisSsoCheckoutBaseURL).toString());

        if (!urlParams.isEmpty())
        {
            fullUrl += "?" + urlParams;
        }
        
        QDesktopServices::openUrl(fullUrl);
    }
}

namespace Origin
{
    namespace Client
    {
        static ClientFlow* mInstance = NULL;

        void ClientFlow::create()
        {
            ORIGIN_ASSERT(!mInstance);
            if (!mInstance)
            {
                mInstance = new ClientFlow();
            }
        }

        void ClientFlow::destroy()
        {
            ORIGIN_ASSERT(mInstance);
            if (mInstance)
            {
                mInstance->deleteLater();
                mInstance = NULL;
            }
        }

        ClientFlow* ClientFlow::instance()
        {
            return mInstance;
        }

        void ClientFlow::showPromoDialog(const PromoBrowserContext& context, const Engine::Content::EntitlementRef ent, const bool force /* = false */)
        {
            if(mClientViewController)
            {
                mClientViewController->showPromoDialog(context, ent, force);
            }
        }
        
        void ClientFlow::onPlayFinished(Origin::Engine::Content::EntitlementRef entRef)
        {
            const bool shutdown = static_cast<bool>(Services::readSetting(Services::SETTING_SHUTDOWN_ORIGIN_ON_GAME_FINISHED));
            const bool stillPlaying = !Engine::Content::ContentController::currentUserContentController()->entitlementByState(0, Engine::Content::LocalContent::Playing).isEmpty();
            const bool joinInProgress = Engine::LoginController::currentUser()->socialControllerInstance()->multiplayerInvites()->activeJoinOperation() != NULL;

            if(shutdown && !stillPlaying && !joinInProgress)
            {
                // With some games playFinished is called twice. Since we don't want to request to exit twice lets just disconnect the signal.
                // Hopefully the exit won't fail...
                ORIGIN_VERIFY_DISCONNECT(Engine::Content::ContentController::currentUserContentController(), SIGNAL(playFinished(Origin::Engine::Content::EntitlementRef)), this, SLOT(onPlayFinished(Origin::Engine::Content::EntitlementRef)));
                emit requestSilentExit();
            }
            else if(!entRef->localContent()->playing())
            {
                PromoBrowserContext context = PromoContext::GameFinished;

                // determine context for showing PromoBrowser
                bool isFreeTrial = entRef->contentConfiguration()->isFreeTrial();
                if (isFreeTrial)
                {
                    qint64 playedSecs = 0;
                    qint64 remainingSecs = 0;
                    QElapsedTimer const& timer = entRef->localContent()->freeTrialElapsedTimer();
                    if (timer.isValid())
                    {
                        playedSecs = timer.elapsed() / 1000;
                        remainingSecs = entRef->localContent()->trialTimeRemaining() / 1000;
                    }
                    if (playedSecs < 60)
                        context = PromoContext::NoType;        // the trial was not started correctly
                    else if (remainingSecs <= 0)
                        context = PromoContext::FreeTrialExpired; // the trial is over
                    else
                        context = PromoContext::FreeTrialExited;
                }

                // Don't show promos for NOGs.
                if(entRef->contentConfiguration()->isNonOriginGame())
                {
                    context.promoType = PromoContext::NoType;
                }

                if (context.promoType != PromoContext::NoType)
                {
                    // Should a promo dialog after the game has exited
                    mClientViewController->showPromoDialog(context, entRef);
                }
            }
        }

        void ClientFlow::showProductIDInStore(const QString& productID)
        {
            if (mClientViewController)
                mClientViewController->showProductIDInStore(productID);
        }

        void ClientFlow::showMasterTitleInStore(const QString& masterTitleID)
        {
            if (mClientViewController)
                mClientViewController->showMasterTitleInStore(masterTitleID);
        }

        void ClientFlow::showUrlInStore(const QUrl& url)
        {
            if (mClientViewController)
                mClientViewController->showUrlInStore(url);
        }
        
        void ClientFlow::showStoreFreeGames()
        {
            if (mClientViewController)
                mClientViewController->showStoreFreeGames();
        }

        void ClientFlow::showStoreOnTheHouse(const QString& trackingParam)
        {
            if (mClientViewController)
                mClientViewController->showStoreOnTheHouse(trackingParam);
        }

        void ClientFlow::showStoreHome()
        {
            if (mClientViewController)
                mClientViewController->showStoreHome();
        }

        void ClientFlow::showMyGames()
        {
            if (mClientViewController)
                mClientViewController->showMyGames();
        }

        void ClientFlow::showTileView()
        {
            if (mClientViewController)
                mClientViewController->showTileView();
        }

        void ClientFlow::showSettingsGeneral()
        {
            if (mClientViewController)
                mClientViewController->showSettingsGeneral();
        }

        void ClientFlow::showSettingsNotifications()
        {
            if (mClientViewController)
                mClientViewController->showSettingsNotifications();
        }

        void ClientFlow::showSettingsInGame()
        {
            if (mClientViewController)
                mClientViewController->showSettingsInGame();
        }

        void ClientFlow::showSettingsAdvanced()
        {
            if (mClientViewController)
                mClientViewController->showSettingsAdvanced();
        }

        void ClientFlow::showSettingsVoice()
        {
            if (mClientViewController)
                mClientViewController->showSettingsVoice();
        }

        void ClientFlow::showPDLCStore(Engine::Content::EntitlementRef entitlement)
        {
            mPDLCViewController.reset(new PDLCViewController);

            if (Engine::IGOController::instance()->isActive())
            {
                Engine::IGOController::instance()->igoShowStoreUI(entitlement);
            }
            else
            {
                mPDLCViewController->show(entitlement);
            }
        }

        void ClientFlow::showDownloadProgressDialog()
        {
            if (mClientViewController)
                mClientViewController->showDownloadProgressDialog();
        }
           
        void ClientFlow::showGameDetails(Engine::Content::EntitlementRef entitlement)
        {
            if (mClientViewController)
                mClientViewController->showGameDetails(entitlement);
        }

        void ClientFlow::showRedemptionPage (RedeemBrowser::SourceType src /*= Origin*/, RedeemBrowser::RequestorID requestorID /*= OriginCodeClient*/, const QString &code)
        {
            if (mClientViewController)
                mClientViewController->showRedemptionPage (src, requestorID, code);
        }
        
        void ClientFlow::showFriendSearchDialog()
        {
            if (mClientViewController)
                mClientViewController->showFriendSearchDialog();
        }

        void ClientFlow::showFriendSearchDialog(const UIScope scope, Engine::IIGOCommandController::CallOrigin callOrigin)
        {
            if (mClientViewController)
                mClientViewController->showFriendSearchDialog(scope, callOrigin);
        }

        void ClientFlow::showCreateGroupDialog()
        {
            if (mClientViewController)
                mClientViewController->showCreateGroupDialog();
        }

        void ClientFlow::showCreateGroupDialog(const UIScope scope)
        {
            if (mClientViewController)
                mClientViewController->showCreateGroupDialog(scope);
        }

        void ClientFlow::showDeleteGroupDialog(const QString& groupName, const QString& groupGuid)
        {
            if (mClientViewController)
                mClientViewController->showDeleteGroupDialog(groupName, groupGuid);
        }

        void ClientFlow::showDeleteGroupDialog(const QString& groupName, const QString& groupGuid, const UIScope scope)
        {
            if (mClientViewController)
                mClientViewController->showDeleteGroupDialog(groupName, groupGuid, scope);
        }

        void ClientFlow::showLeaveGroupDialog(const QString& groupName, const QString& groupGuid)
        {
            if (mClientViewController)
                mClientViewController->showLeaveGroupDialog(groupName, groupGuid);
        }

        void ClientFlow::showLeaveGroupDialog(const QString& groupName, const QString& groupGuid, const UIScope scope)
        {
            if (mClientViewController)
                mClientViewController->showLeaveGroupDialog(groupName, groupGuid, scope);
        }

        void ClientFlow::showDeleteRoomDialog(const QString& groupGuid, const QString& channelId, const QString& roomName)
        {
            if (mClientViewController)
                mClientViewController->showDeleteRoomDialog(groupGuid, channelId, roomName);
        }

        void ClientFlow::showDeleteRoomDialog(const QString& groupGuid, const QString& channelId, const QString& roomName, const UIScope scope)
        {
            if (mClientViewController)
                mClientViewController->showDeleteRoomDialog(groupGuid, channelId, roomName, scope);
        }

        void ClientFlow::showEnterRoomPasswordDialog(const QString& groupGuid, const QString& channelId)
        {
            if (mClientViewController)
                mClientViewController->showEnterRoomPasswordDialog(groupGuid, channelId);
        }

        void ClientFlow::showEnterRoomPasswordDialog(const QString& groupGuid, const QString& channelId, const UIScope scope)
        {
            if (mClientViewController)
                mClientViewController->showEnterRoomPasswordDialog(groupGuid, channelId, scope);
        }

        void ClientFlow::showCreateRoomDialog(const QString& groupName, const QString& groupGuid)
        {
            if (mClientViewController)
                mClientViewController->showCreateRoomDialog(groupName, groupGuid);
        }

        void ClientFlow::showCreateRoomDialog(const QString& groupName, const QString& groupGuid, const UIScope scope)
        {
            if (mClientViewController)
                mClientViewController->showCreateRoomDialog(groupName, groupGuid, scope);
        }

        void ClientFlow::showEditGroupDialog(const QString& groupName, const QString& groupGuid)
        {
            if (mClientViewController)
                mClientViewController->showEditGroupDialog(groupName, groupGuid);
        }

        void ClientFlow::showEditGroupDialog(const QString& groupName, const QString& groupGuid, const UIScope scope)
        {
            if (mClientViewController)
                mClientViewController->showEditGroupDialog(groupName, groupGuid, scope);
        }

        void ClientFlow::showInviteFriendsToGroupDialog(const QString& groupGuid, const UIScope scope)
        {
            if (mClientViewController)
                mClientViewController->showInviteFriendsToGroupDialog(groupGuid, scope);
        }

        void ClientFlow::showInviteFriendsToRoomDialog(const QString& groupGuid, const QString& channelId, const QString& conversationId, const UIScope scope)
        {
            if (mClientViewController)
                mClientViewController->showInviteFriendsToRoomDialog(groupGuid, channelId, conversationId, scope);
        }

        void ClientFlow::showYouNeedFriendsDialog(const UIScope scope)
        {
            if (mClientViewController)
                mClientViewController->showYouNeedFriendsDialog(scope);
        }

        void ClientFlow::showGroupMembersDialog(const QString& groupName, const QString& groupGuid)
        {
            if (mClientViewController)
                mClientViewController->showGroupMembersDialog(groupName, groupGuid);
        }

        void ClientFlow::showGroupMembersDialog(const QString& groupName, const QString& groupGuid, const UIScope scope)
        {
            if (mClientViewController)
                mClientViewController->showGroupMembersDialog(groupName, groupGuid, scope);
        }

        void ClientFlow::showGroupBannedMembersDialog(const QString& groupName, const QString& groupGuid)
        {
            if (mClientViewController)
                mClientViewController->showGroupBannedMembersDialog(groupName, groupGuid);
        }

        void ClientFlow::showGroupBannedMembersDialog(const QString& groupName, const QString& groupGuid, const UIScope scope)
        {
            if (mClientViewController)
                mClientViewController->showGroupBannedMembersDialog(groupName, groupGuid, scope);
        }

        void ClientFlow::showRemoveGroupUserDialog(const QString& groupGuid, const QString& userId, const QString& userNickname)
        {
            if (mClientViewController)
                mClientViewController->showRemoveGroupUserDialog(groupGuid, userId, userNickname);
        }

        void ClientFlow::showRemoveGroupUserDialog(const QString& groupGuid, const QString& userId, const QString& userNickname, const UIScope scope)
        {
            if (mClientViewController)
                mClientViewController->showRemoveGroupUserDialog(groupGuid, userId, userNickname, scope);
        }

        void ClientFlow::showRemoveRoomUserDialog(const QString& groupGuid, const QString& channelId, const QString& userNickname)
        {
            if (mClientViewController)
                mClientViewController->showRemoveRoomUserDialog(groupGuid, channelId, userNickname);
        }

        void ClientFlow::showRemoveRoomUserDialog(const QString& groupGuid, const QString& channelId, const QString& userNickname, const UIScope scope)
        {
            if (mClientViewController)
                mClientViewController->showRemoveRoomUserDialog(groupGuid, channelId, userNickname, scope);
        }

        void ClientFlow::showPromoteToAdminSuccessDialog (const QString& groupName, const QString& userNickname, const UIScope scope)
        {
            if (mClientViewController)
                mClientViewController->showPromoteToAdminSuccessDialog(groupName, userNickname, scope);
        }

        void ClientFlow::showPromoteToAdminFailureDialog (const QString& userNickname, const UIScope scope)
        {
            if (mClientViewController)
                mClientViewController->showPromoteToAdminFailureDialog(userNickname, scope);
        }

        void ClientFlow::showDemoteToMemberSuccessDialog (const QString& groupName, const QString& userNickname, const UIScope scope)
        {
            if (mClientViewController)
                mClientViewController->showDemoteToMemberSuccessDialog(groupName, userNickname, scope);
        }

        void ClientFlow::showTransferOwnershipConfirmationDialog (const QString& groupGuid, Origin::Chat::RemoteUser* user, const QString& username, const UIScope scope)
        {
            if (mClientViewController)
                mClientViewController->showTransferOwnershipConfirmationDialog(groupGuid, user, username, scope);
        }

        void ClientFlow::showTransferOwnershipSuccessDialog (const QString& groupName, const QString& userNickname, const UIScope scope)
        {
            if (mClientViewController)
                mClientViewController->showTransferOwnershipSuccessDialog(groupName, userNickname, scope);
        }

        void ClientFlow::showTransferOwnershipFailureDialog (const QString& userNickname, const UIScope scope)
        {
            if (mClientViewController)
                mClientViewController->showTransferOwnershipFailureDialog(userNickname, scope);
        }

        void ClientFlow::showGroupInviteSentDialog(QList<Origin::Chat::RemoteUser*> users, const UIScope scope)
        {
            if (mClientViewController)
                mClientViewController->showGroupInviteSentDialog(users, scope);
        }

        void ClientFlow::showMyProfile(Engine::IIGOCommandController::CallOrigin callOrigin)
        {
            if (mClientViewController)
                mClientViewController->showMyProfile(callOrigin);
        }

        void ClientFlow::showMyProfile(const UIScope scope, ShowProfileSource source, Engine::IIGOCommandController::CallOrigin callOrigin)
        {
            if (mClientViewController)
                mClientViewController->showMyProfile(scope, source, callOrigin);
        }

        void ClientFlow::showProfile(qint64 nucleusId)
        {
            if (mClientViewController)
                mClientViewController->showProfile(nucleusId);
        }

        void ClientFlow::showProfile(qint64 nucleusId, const UIScope scope, ShowProfileSource source, Engine::IIGOCommandController::CallOrigin callOrigin)
        {
            if (mClientViewController)
                mClientViewController->showProfile(nucleusId, scope, source, callOrigin);
        }
            
        void ClientFlow::showProfileSearchResult(const QString &search, UIScope scope)
        {
            if (mClientViewController)
                mClientViewController->showProfileSearchResult(search, scope);
        }

        void ClientFlow::showAchievementsHome()
        {
            if (mClientViewController)
                mClientViewController->showAchievementsHome();
        }

        void ClientFlow::showAchievementSetDetails(const QString& achievementSetId, const QString& userId, const QString& gameTitle)
        {
            if (mClientViewController)
                mClientViewController->showAchievementSetDetails(achievementSetId, userId, gameTitle);
        }

        void ClientFlow::showFeedbackPage()
        {
            if (mClientViewController)
                mClientViewController->showFeedbackPage();
        }
        
        void ClientFlow::showOrderHistory()
        {
            if (mClientViewController)
                mClientViewController->showOrderHistory();
        }

        void ClientFlow::showAccountProfilePrivacy()
        {
            if (mClientViewController)
                mClientViewController->showAccountProfilePrivacy();
        }

        void ClientFlow::showAccountProfileSecurity()
        {
            if (mClientViewController)
                mClientViewController->showAccountProfileSecurity();
        }

        void ClientFlow::showAccountProfilePaymentShipping()
        {
            if (mClientViewController)
                mClientViewController->showAccountProfilePaymentShipping();
        }

        void ClientFlow::showAccountProfileSubscription(const QString& status)
        {
            if (mClientViewController)
                mClientViewController->showAccountProfileSubscription(status);
        }

        void ClientFlow::showAccountProfileRedeem()
        {
            if (mClientViewController)
                mClientViewController->showAccountProfileRedeem();
        }

        void ClientFlow::setInSocial(const QString& nucleusID)
        {
            if (mClientViewController)
                mClientViewController->setInSocial(nucleusID);
        }

        void ClientFlow::showMyFriends()
        {
            if (mClientViewController)
                mClientViewController->showFriendsList();
        }

        void ClientFlow::showSettings()
        {
            if (mClientViewController)
                mClientViewController->showSettings();
        }

        void ClientFlow::showDeveloperTool()
        {
            if(mClientViewController)
                mClientViewController->onMainMenuDEBUGOriginDeveloperTool();
        }

        void ClientFlow::showHelp()
        {
            if (mClientViewController)
                mClientViewController->showHelp();
        }

        void ClientFlow::showAbout()
        {
            if (mClientViewController)
                mClientViewController->onMainMenuAbout();
        }

        void ClientFlow::showSelectAvatar()
        {
            if (mClientViewController)
                mClientViewController->showAvatarSelect();
        }

        void ClientFlow::showSelectAvatar(const UIScope scope, Engine::IIGOCommandController::CallOrigin callOrigin)
        {
            if (mClientViewController)
                mClientViewController->showAvatarSelect(scope, callOrigin);
        }

        void ClientFlow::minimizeClientWindow (bool minimize)
        {
            if (mClientViewController)
                mClientViewController->show (minimize ? ClientWindow_SHOW_MINIMIZED : ClientWindow_SHOW_NORMAL);
        }

        void ClientFlow::minimizeClientToTray()
        {
            if (mClientViewController)
                mClientViewController->minimizeClientToTray();
        }


        void ClientFlow::setUserOnline()
        {
            if (mClientViewController)
                mClientViewController->setUserOnlineState();
        }

        bool ClientFlow::closeDeveloperTool()
        {
            bool developerToolClosed = true;

            QString odtPluginOffer(Services::OriginConfigService::instance()->odtPlugin().productId);
            const bool odtLoaded = Engine::PluginManager::instance()->pluginLoaded(odtPluginOffer);

            const QMetaMethod closeWindowRequestSignal = QMetaMethod::fromSignal(&ClientFlow::odtCloseWindowRequest);
            const bool signalConnected = isSignalConnected(closeWindowRequestSignal);

            // Don't emit the signal if we're not connected. We perform this 
            // check because the emit call will return false if nothing is
            // connected to the signal, which could also be the return value
            // if the user decides to not close ODT.
            if (odtLoaded && signalConnected)
            {
                developerToolClosed = emit odtCloseWindowRequest();
            }

            return developerToolClosed;
        }

        void ClientFlow::start()
        {
            mOriginToastManager.reset(new OriginToastManager());

            //need this started ASAP to get ahead of RTP
            startClient();

            emit(clientFlowCreated());

            // The settings dialog revalidates the content folder locations when it is shown, so
            // it generates the correct error dialogs after the content controller throws the
            // invalid content folders error.
            ORIGIN_VERIFY_CONNECT(
                Origin::Engine::Content::ContentController::currentUserContentController(),
                SIGNAL(invalidContentFolders()),
                mClientViewController.data(),
                SLOT(onShowSettingsApplication()));

            MainFlow::instance()->contentFlowController()->connectEntitlements();

            //if the RTP flow is active, lets not show the promo browser
            // Also if the user is now we do not want to show the promo browser
            // fixes EBIBUGS-22201 https://developer.origin.com/support/browse/EBIBUGS-22201
            // Also, don't show promo browser if the user just completed the age up flow.
            // https://developer.origin.com/support/browse/EBIBUGS-24233
            bool newUser = MainFlow::instance()->newUser();
            if (!newUser)
            {
                //the showNetPromoter dialog actually also kicks of the start promo browser
                //if ITE is active we don't want to show the net promoter dialog or promo browser
                if(!OriginApplication::instance().isITE() && !MainFlow::instance()->rtpFlow()->isPending() && !MainFlow::instance()->wasAgeUpFlowCompleted())
                {
                    mClientViewController->showNetPromoterDialog();
                }
            }
            // Set the newUser flag to false so that if the user logs out and back in
            // without closeing Origin they see the promo browser
            else
            {
                MainFlow::instance()->setNewUser(false);
            }
            startITOFlow();
            startIGOFlow();

            if(!Engine::LoginController::currentUser().isNull())
            {
                mSocialViewController.reset(new SocialViewController(mClientViewController->window(), Engine::LoginController::currentUser()));
            }

            // TODO: Create a new home for showing the promo dialog after a game has exited
            ORIGIN_VERIFY_CONNECT_EX(Engine::Content::ContentController::currentUserContentController(),SIGNAL(playFinished(Origin::Engine::Content::EntitlementRef)),
                this,SLOT(onPlayFinished(Origin::Engine::Content::EntitlementRef)), Qt::QueuedConnection);

            ORIGIN_VERIFY_CONNECT(
                Engine::Content::ContentController::currentUserContentController(), SIGNAL(preflightDownload(Origin::Engine::Content::LocalContent const&, Origin::Engine::Content::ContentController::DownloadPreflightingResult&)),
                this, SLOT(onPreflightDownload(Origin::Engine::Content::LocalContent const&, Origin::Engine::Content::ContentController::DownloadPreflightingResult&)));
        }

        void ClientFlow::startClient()
        {
            mClientViewController.reset(new ClientViewController(Engine::LoginController::currentUser()));
            Origin::Client::PendingActionFlow *actionFlow = Origin::Client::PendingActionFlow::instance();

            //see if there was a windows state requested by the pending action
            if(actionFlow && (actionFlow->windowsShowState() != ClientWindow_NO_ACTION))
            {
                mStartShowState = static_cast<ClientViewWindowShowState>(actionFlow->windowsShowState());
            }
            mClientViewController->show(mStartShowState);
            mStartShowState = ClientWindow_SHOW_NORMAL;      //reset it

            //ORIGIN_VERIFY_CONNECT(mClientViewController.data(), SIGNAL(success(ClientViewSuccessReason)), this, SLOT(onClientFlowFinished(ClientViewSuccessReason)));
            ORIGIN_VERIFY_CONNECT(
                mClientViewController.data(), SIGNAL(requestLogout()),
                this, SIGNAL(requestLogout()));
            ORIGIN_VERIFY_CONNECT(
                mClientViewController.data(), SIGNAL(requestExit()),
                this, SIGNAL(requestExit()));
            ORIGIN_VERIFY_CONNECT(
                mClientViewController.data(), SIGNAL(netpromoter_notshown()),
                this, SLOT(onNetPromoterNotShowing()));
            ORIGIN_VERIFY_CONNECT(
                mClientViewController.data(), SIGNAL(addNonOriginGame()),
                this, SIGNAL(addNonOriginGame()));
            ORIGIN_VERIFY_CONNECT(
                mClientViewController.data(), SIGNAL(redemptionPageShown()),
                this, SIGNAL(redemptionPageShown()));
            ORIGIN_VERIFY_CONNECT(
                mClientViewController.data(), SIGNAL(redemptionPageClosed()),
                this, SIGNAL(redemptionPageClosed()));
            ORIGIN_VERIFY_CONNECT(
                mClientViewController.data(), SIGNAL(lostAuthentication()),
                this, SIGNAL(lostAuthentication()));

            ORIGIN_VERIFY_CONNECT(
                Origin::Engine::Subscription::SubscriptionManager::instance(), SIGNAL(trialEligibilityInfoReceived()),
                this, SLOT(onSubscriptionTrialEligibilityInfoReceived()));
#if defined(ENABLE_SUBS_TRIALS)
            Engine::Subscription::SubscriptionManager::instance()->refreshSubscriptionTrialEligibility();
#endif
            
            ORIGIN_VERIFY_CONNECT(
                Origin::Client::MainFlow::instance(), SIGNAL(gotCloseEvent()),
                this, SLOT(onGotAppCloseEvent()) );

            ORIGIN_VERIFY_CONNECT(
                mClientViewController.data(), SIGNAL(myGamesLoadFinished()),
                this, SIGNAL(myGamesLoadFinished()));

            //hook up to listen for dirty bits changes
            connectDirtyBitsHandlers();
        }

        void ClientFlow::startITOFlow()
        {
            ITOFlow::create();
            ITOFlow::instance()->start();
        }

        void ClientFlow::startIGOFlow()
        {
            mIGOFlow.reset(new IGOFlow());
            mIGOFlow->start();
        }

        void ClientFlow::connectDirtyBitsHandlers()
        {
            if (mClientViewController)
            {
                Origin::Engine::DirtyBitsClient::instance()->registerHandler(this, "onEmailChanged", "email");
                Origin::Engine::DirtyBitsClient::instance()->registerHandler(this, "onPasswordChanged", "password");
                Origin::Engine::DirtyBitsClient::instance()->registerHandler(this, "onOriginIdChanged", "originid");

                Origin::Engine::DirtyBitsClient::instance()->registerHandler(mClientViewController.data(), "onGameLibraryUpdated", "gamelib");
                Origin::Engine::DirtyBitsClient::instance()->registerHandler(mClientViewController.data(), "onPrivacySettingsUpdated", "privacy");
                Origin::Engine::DirtyBitsClient::instance()->registerHandler(mClientViewController.data(), "onAvatarChanged", "avatar");

                if(Origin::Services::OriginConfigService::instance()->ecommerceConfig().refreshEntitlementsOnDirtyBits)
                {
                    Origin::Engine::DirtyBitsClient::instance()->registerHandler(Origin::Engine::Content::ContentController::currentUserContentController()->baseGamesController(), "onDirtyBitsEntitlementUpdate", "entitlement");
                }

                if(Origin::Engine::Content::CatalogDefinitionController::instance() != NULL)
                {   
                    Origin::Engine::DirtyBitsClient::instance()->registerHandler(Origin::Engine::Content::CatalogDefinitionController::instance(), "onDirtyBitsCatalogUpdateNotification", "catalog");
                }

                // Groip handlers
                Origin::Engine::DirtyBitsClient::instance()->registerHandler(mClientViewController.data(), "onGroupsEvent", "group");
            }
        }

        void ClientFlow::onEmailChanged(QByteArray payload)
        {
            ORIGIN_LOG_EVENT << "User changed email";

            // payload structure
            // {"ver":1,"uid":12306792511,"ts":1377285598636,"ctx":"email","verb":"upd","data":{"old":"t626s71_5@gmail.com","new":"t626s71_5a@gmail.com"}}

            Services::JsonUtil::JsonWrapper dom;
            dom.parse(payload);

            QVariant data = dom.search("data");
            if (!data.toMap().isEmpty() && data.toMap().contains("new"))
            {
                QString newEmail = data.toMap().find("new").value().toString();

                if (!newEmail.isEmpty())
                {
                    Origin::Services::Session::SessionService::instance()->updateCurrentUserEmail(
                        Origin::Engine::LoginController::currentUser()->getSession(), newEmail);
                }
            }
        }

        void ClientFlow::onPasswordChanged(QByteArray payload)
        {
            ORIGIN_LOG_EVENT << "User changed password";
            emit (credentialsChanged());
        }

        void ClientFlow::onOriginIdChanged (QByteArray payload)
        {
            ORIGIN_LOG_EVENT << "User changed Origin ID";

            // payload structure
            // {"ver":1,"uid":1000001520859,"ts":1371217132577,"ctx":"originid","verb":"upd","data":{"pid":1000001520859,"old":"jwadebuytest12b","new":"jwade_buy_test12"}}

            Services::JsonUtil::JsonWrapper dom;
            dom.parse(payload);

            QVariant data = dom.search("data");
            if (!data.toMap().isEmpty() && data.toMap().contains("new"))
            {
                QString newOriginId = data.toMap().find("new").value().toString();

                if (!newOriginId.isEmpty())
                {
                    Origin::Services::Session::SessionService::instance()->updateCurrentUserOriginId(
                        Origin::Engine::LoginController::currentUser()->getSession(), newOriginId);

                    // Update the last user to login cookie so that the login page will auto fill the new id.
                    QString envname = Origin::Services::readSetting(Services::SETTING_EnvironmentName, Services::Session::SessionRef());
                    const Services::Setting &usernameCookieSetting = envname == Services::SETTING_ENV_production ? Services::SETTING_MOST_RECENT_USER_NAME_PROD : Services::SETTING_MOST_RECENT_USER_NAME_INT;

                    QString settingsCookieValue(Services::readSetting(usernameCookieSetting));
                    if (!settingsCookieValue.isEmpty() &&
                        settingsCookieValue != Services::SETTING_INVALID_MOST_RECENT_USER_NAME)
                    {
                        QStringList tokens = settingsCookieValue.split("||", QString::SkipEmptyParts);
                        if (tokens.size() == 3)
                        {
                            tokens[0] = newOriginId;

                            QString newSettingsCookieValue = tokens.join("||");
                            Services::writeSetting(usernameCookieSetting, newSettingsCookieValue);
                        }
                    }
                }
            }
        }

        void ClientFlow::onInicisSsoCheckoutAuthCodeRetrieved()
        {
            Origin::Services::AuthPortalAuthorizationCodeResponse* response = dynamic_cast<Origin::Services::AuthPortalAuthorizationCodeResponse*>(sender());
            ORIGIN_ASSERT(response);

            // Note that we support multiple auth code requests simultaneously via the
            // mInicisParamMapper object, however, we only have one timer (mInicisAuthCodeRetrieveTimer).
            // This is incorrect - along with mapping the request, we should also map the individual 
            // timeout timers to each request. By having a single timer that we stop here, we 
            // effectively turn off timeout timers for any other requests that are still pending. This
            // means that those requests will fallback on the default timeout value for the network 
            // request (OFM-9040). Having multiple requests active should be a rare edge case, so we
            // avoid the additional complexity of creating multiple-mapped timers for now.
            mInicisAuthCodeRetrieveTimer.stop();

            if ( !response )
            {
                //just ignore since it really shouldn't happen (abort() won't make it null)
                return;
            }
            else
            {
                ORIGIN_VERIFY_DISCONNECT(response, SIGNAL(finished()), this, SLOT(onInicisSsoCheckoutAuthCodeRetrieved()));
                ORIGIN_VERIFY_DISCONNECT(&mInicisAuthCodeRetrieveTimer, SIGNAL (timeout()), response, SLOT(abort())); 
            }
                
            bool success = ((response->error() == Origin::Services::restErrorSuccess) && (response->httpStatus() == Origin::Services::Http302Found));

            if (!success)   //we need to log it
            {
                ORIGIN_LOG_ERROR << "Inicis checkout: auth code retrieval failed with: restError = " << response->error() << " httpStatus = " << response->httpStatus();
                GetTelemetryInterface()->Metric_ERROR_NOTIFY("InicisAuthCodeRetrievalFail", QString("%1").arg(response->httpStatus()).toUtf8().constData());
            }

            // Launch the checkout URL even if we fail to obtain the auth code. This requires
            // the user to log in manually but at least they can continue checkout.
            {
                // Grab the parameter string associated with this request
                QString urlParams(mInicisParamMapper.value(response));
                mInicisParamMapper.remove(response);

                // Update the parameter string with the actual authorization code
                QString authCode(response->authorizationCode());
                urlParams.replace(INICIS_SSO_URL_AUTHCODE_REPLACE_TOKEN, authCode);

                launchBrowserWithInicisSsoCheckoutUrl(urlParams);
            }
        }

        // because net promoter is supposed to supercede the starting promo dialog
        void ClientFlow::onNetPromoterNotShowing()
        {
            mClientViewController->showPromoDialog(PromoContext::OriginStarted, Engine::Content::EntitlementRef(), false);

            ORIGIN_VERIFY_DISCONNECT(
                mClientViewController.data(), SIGNAL(netpromoter_notshown()),
                this, SLOT(onNetPromoterNotShowing()));
        }

        void ClientFlow::onSubscriptionTrialEligibilityInfoReceived()
        {
            mClientViewController->showPromoDialog(PromoContext::OriginStarted, Engine::Content::EntitlementRef(), false);

            ORIGIN_VERIFY_DISCONNECT(
                Origin::Engine::Subscription::SubscriptionManager::instance(), SIGNAL(trialEligibilityInfoReceived()),
                this, SLOT(onSubscriptionTrialEligibilityInfoReceived()));
        }

        void ClientFlow::onGotAppCloseEvent()
        {
            //onClientFlowFinished(ClientViewSuccessReason_Exit);
            emit requestExit();
        }

        void ClientFlow::logout()
        {
            // Shut down navigation
            NavigationController::instance()->setEnabled(false);
            NavigationController::instance()->clearHistory();

            endCurrentSession();
            
            emit finished(ClientFlowResult(FLOW_SUCCEEDED, ClientFlowResult::LOGOUT));
            emit logoutConcluded();
        }

        void ClientFlow::exit()
        {
            //clear the ITE commandline
            Origin::Services::SettingsManager::instance()->unset(Origin::Services::SETTING_COMMANDLINE);

            endCurrentSession();

            emit finished(ClientFlowResult(FLOW_SUCCEEDED, ClientFlowResult::EXIT));
        }
        
        void ClientFlow::endCurrentSession()
        {
            // release our notification manager
            if (!mOriginToastManager.isNull())
                delete mOriginToastManager.take();

            // Make sure to close all IGO windows before we clean up the session so as to save their locations/sizes
            Origin::Engine::IGOController::instance()->igowm()->clear();
            
            // Make sure we delete our view controller before we log out
            // At login we don't create the view until a successful login so we don't want the UI code having
            // to special case for not having a session briefly during logout. This was causing crashes with 
            // WebWidgetController where it attempts to unregister all of its user-specific native interfaces on logout
            // but My Games was still running
            if (!mClientViewController.isNull())
            {
                mClientViewController->saveClientViewSizeAndPosition();
                delete mClientViewController.take();
            }

            if (!mSocialViewController.isNull())
                delete mSocialViewController.take();

            if (!mIGOFlow.isNull())
                delete mIGOFlow.take();

            ITOFlow::destroy();
            
            MainFlow::instance()->contentFlowController()->endUserSpecificFlows();

            if((Origin::Engine::LoginController::currentUser() != NULL) && (Origin::Engine::LoginController::currentUser()->getSession() != NULL))
                Origin::Engine::LoginController::logoutAsync(Origin::Engine::LoginController::currentUser()->getSession());

            QStringList excludedWindows;
            excludedWindows << "DirtyBitsTrafficTool";
            // Close any stray windows
            foreach(QWidget* widget, QApplication::topLevelWidgets())
            {
                UIToolkit::OriginWindow* window = dynamic_cast<UIToolkit::OriginWindow*>(widget);
                if (window && excludedWindows.contains(window->objectName(), Qt::CaseInsensitive) == false)
                {
                    window->deleteLater();
                }
            }
        }
            
        SocialViewController* ClientFlow::socialViewController() const
        {
            return mSocialViewController.data();
        }

        OriginToastManager* ClientFlow::originToastManager() const
        {
            return mOriginToastManager.data();
        }
        
        FriendsListViewController* ClientFlow::friendsListViewController() const
        {
            if(mClientViewController)
            {
                return mClientViewController->friendsListViewController();
            }
            return NULL;
        }

        ClientViewController* ClientFlow::view() const
        {
            return mClientViewController.data();
        }

        void ClientFlow::requestManualOfflineMode()
        {
            if(mClientViewController)
                mClientViewController->requestManualOfflineMode();
        }
        
        void ClientFlow::requestOnlineMode()
        {
            if(mClientViewController)
                mClientViewController->requestOnlineMode();
        }

        void ClientFlow::silentRequestOnlineMode()
        {
            Engine::UserRef user = Engine::LoginController::instance()->currentUser();

            // If already online then just ignore (UI may not be showing the friends list yet)
            if (Services::Connection::ConnectionStatesService::isUserOnline(user->getSession()))
                return;

            if (Services::Connection::ConnectionStatesService::canGoOnline(user->getSession()))
            {
                // Set the client online
                MainFlow::instance()->socialGoOnline();
            }
        }

        void ClientFlow::requestLogoutFromJS()
        {
            if (mClientViewController)
                mClientViewController->requestLogoutFromJS();
        }

        ClientFlow::ClientFlow()
            :mStartShowState (ClientWindow_SHOW_NORMAL)
        {
        }

        ClientFlow::~ClientFlow() 
        {
        }

        void ClientFlow::alertMainWindow()
        {
            if(mClientViewController)
                mClientViewController->alertMainWindow();
        }

        void ClientFlow::startJoinMultiplayerFlow(const Chat::JabberID &jid)
        {
            // Bounce to the main thread
            QMetaObject::invokeMethod(this, "startJoinMultiplayerFlowAsync", Q_ARG(Origin::Chat::JabberID, jid));
        }

        void ClientFlow::startJoinMultiplayerFlowAsync(const Chat::JabberID &jid)
        {
            Engine::UserRef user = Engine::LoginController::currentUser();

            if (!user)
            {
                return;
            }
            
            JoinMultiplayerFlow *flow = new JoinMultiplayerFlow;

            ORIGIN_VERIFY_CONNECT(flow, SIGNAL(finished(bool)), flow, SLOT(deleteLater()));
            flow->start(user->socialControllerInstance(), jid);
        }

        bool ClientFlow::isMainWindowVisible ()
        {
            if (mClientViewController)
                return mClientViewController->isMainWindowVisible();

            return false;
        }

        void ClientFlow::launchInicisSsoCheckout(const QString& urlparams)
        {
            static const int NETWORK_TIMEOUT_MSECS = 30000;
            static const QString LOCKBOX_CLIENTID = "lockbox";

            ORIGIN_LOG_EVENT << "Inicis checkout: retrieving authorization code";

            // Only perform auth-code request if the URL parameter string contains
            // the auth code replacement token.
            if (urlparams.contains(INICIS_SSO_URL_AUTHCODE_REPLACE_TOKEN))
            {
                QString accessToken = Origin::Services::Session::SessionService::accessToken(Origin::Services::Session::SessionService::currentSession());
                Origin::Services::AuthPortalAuthorizationCodeResponse *resp = Origin::Services::AuthPortalServiceClient::retrieveAuthorizationCode(accessToken, LOCKBOX_CLIENTID);

                // Map this request to its parameters in case the user decides navigate to another
                // checkout flow and put in another Inicis checkout request.
                mInicisParamMapper.insert(resp, urlparams);
                ORIGIN_ASSERT(mInicisParamMapper.contains(resp));

                ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(onInicisSsoCheckoutAuthCodeRetrieved()));
                ORIGIN_VERIFY_CONNECT(&mInicisAuthCodeRetrieveTimer, SIGNAL (timeout()), resp, SLOT(abort())); 

                mInicisAuthCodeRetrieveTimer.setSingleShot(true);
                mInicisAuthCodeRetrieveTimer.setInterval (NETWORK_TIMEOUT_MSECS);
                mInicisAuthCodeRetrieveTimer.start();

                resp->setDeleteOnFinish(true);
            }
            else
            {
                // Report that we're missing the auth-code replacement token (probably an indication
                // that something is wrong/misconfigured)
                GetTelemetryInterface()->Metric_ERROR_NOTIFY("InicisAuthCodeUrlTokenMissing", urlparams.toUtf8().constData());

                // If the replacement token isn't specified, just launch the
                // URL with the given params without requesting an auth code.
                launchBrowserWithInicisSsoCheckoutUrl(urlparams);
            }
        }

        void ClientFlow::onPreflightDownload(
            Origin::Engine::Content::LocalContent const& content,
            Origin::Engine::Content::ContentController::DownloadPreflightingResult& result)
        {
#ifdef ORIGIN_MAC

            // default to abort download in case of early exit
            result = Origin::Engine::Content::ContentController::ABORT_DOWNLOAD;
        
            const Origin::Engine::Content::EntitlementRef entitlement = content.entitlement();
            if (!entitlement)
                return;
        
            const Origin::Engine::Content::ContentConfigurationRef config = entitlement->contentConfiguration();
            if (!config)
                return;
            
            // change default result to continue
            result = Origin::Engine::Content::ContentController::CONTINUE_DOWNLOAD;
        
            // do our ugly SimCity version check hack
            QString productId = config->productId();
            if (Origin::Services::PlatformService::isSimCityProductId(productId))
            {
                Origin::VersionInfo localOSVersion(EnvUtils::GetOSVersion());
                Origin::VersionInfo requiredOSVersion(10, 7, 5);
                
                // if localOSVersion is empty, we don't know what OS we are on so notify user
                if (localOSVersion < requiredOSVersion)
                {
                    using namespace Origin::UIToolkit;
                    OriginWindow* messageBox = new OriginWindow(
                        Origin::UIToolkit::OriginWindow::Icon | Origin::UIToolkit::OriginWindow::Close,
                        NULL, OriginWindow::MsgBox, QDialogButtonBox::Yes|QDialogButtonBox::No);
                    QString gameName(config->displayName());
                    messageBox->msgBox()->setup(
                        OriginMsgBox::Notice,
                        tr("ebisu_client_system_upgrade_recommended_caps"),
                        tr("ebisu_client_system_upgrade_recommended_description").arg(gameName).arg(tr("ebisu_client_osx").append(" ").append(requiredOSVersion.ToStr(Origin::VersionInfo::ABBREVIATED_VERSION))));
                    messageBox->setButtonText(QDialogButtonBox::Yes, tr("ebisu_client_download_game"));
                    messageBox->setButtonText(QDialogButtonBox::No, tr("ebisu_client_cancel"));
                    messageBox->setDefaultButton(QDialogButtonBox::No);

                    ORIGIN_VERIFY_CONNECT(messageBox, SIGNAL(finished(int)), messageBox, SLOT(close()));
                    ORIGIN_VERIFY_CONNECT(messageBox, SIGNAL(rejected()), messageBox, SLOT(close()));
                    ORIGIN_VERIFY_CONNECT(messageBox->button(QDialogButtonBox::Yes), SIGNAL(clicked()), messageBox, SLOT(accept()));
                    ORIGIN_VERIFY_CONNECT(messageBox->button(QDialogButtonBox::No), SIGNAL(clicked()), messageBox, SLOT(reject()));
                    messageBox->manager()->setupButtonFocus();
                    
                    result = (messageBox->execWindow() == QDialog::Accepted) ? Origin::Engine::Content::ContentController::USER_OVERRIDE : Origin::Engine::Content::ContentController::ABORT_DOWNLOAD;
                }
            }
#else
                // No preflight required
                result = Origin::Engine::Content::ContentController::CONTINUE_DOWNLOAD;
#endif
        }

    }
}
