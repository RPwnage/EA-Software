///////////////////////////////////////////////////////////////////////////////
// ClientFlow.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef CLIENTFLOW_H
#define CLIENTFLOW_H

#include "widgets/client/source/ClientViewController.h"

#include "flows/AbstractFlow.h"

#include "engine/content/Entitlement.h"
#include "engine/content/ContentController.h"
#include "engine/igo/IIGOCommandController.h"
#include "services/plugin/PluginAPI.h"


//LEGACY
#include "RedeemBrowser.h"

namespace Origin
{
    namespace Chat
    {
        class JabberID;
    }

    namespace Services
    {
        class AuthPortalAuthorizationCodeResponse;
    }

    namespace Client
    {
        class ITOFlow;
        class IGOFlow;
        class PDLCViewController;
        class LogoutExitFlow;
        class SocialViewController;
        class OriginToastManager;
        struct PromoBrowserContext;
        
        /// \brief Handles all high-level actions related to the client flow.
        class ORIGIN_PLUGIN_API ClientFlow : public AbstractFlow
        {
            Q_OBJECT

        public:
            /// \brief The ClientFlow constructor.
            ClientFlow();

            /// \brief The ClientFlow destructor.
            ~ClientFlow();
            
            /// \brief Public interface for starting the client flow.
            virtual void start();

            /// \brief Creates a new ClientFlow object.
            static void create();

            /// \brief Destroys the current ClientFlow object if any.
            static void destroy();

            /// \brief Gets the current ClientFlow object.
            /// \return The current ClientFlow object.
            static ClientFlow* instance();

            /// \brief Cleans up and emits finished with a "logout" transition.
            void logout();

            /// \brief Cleans up and emits finished with an "exit" transition.
            void exit();

            /// \brief Shows the promo dialog.
            /// \param context Reason the promo browser is being displayed.
            /// \param ent Which entitlement (if any) is the cause of the promo.
            /// \param force True if the promo dialog should show regardless of other factors.
            void showPromoDialog(const PromoBrowserContext& context, const Engine::Content::EntitlementRef ent, const bool force = false);

            /// \brief Switches to the store tab and shows the game with the given product ID.
            /// \param productID The product ID of the game to show.
            void showProductIDInStore(const QString& productID);

            /// \brief Switches to the store tab and shows the game with the given master title ID.
            /// \param masterTitleID The master title ID of the game to show.
            void showMasterTitleInStore(const QString& masterTitleID);

            /// \brief Switches to the store tab and shows the given store page.
            /// \param url The url to the store page.
            void showUrlInStore(const QUrl& url);
            
            /// \brief Switches to the store tab and shows the store free games page.
            void showStoreFreeGames();

            /// \brief Switches to the store tab and shows the store giveaway games page.
            /// \param trackingParam Value used in GA tracking
            void showStoreOnTheHouse(const QString& trackingParam);

            /// \brief Switches to the store tab and shows the store home page.
            void showStoreHome();

            /// \brief Shows the PDLC store for the game with the given content ID.
            /// \param entitlement The entitlement to show PDLC for.
            void showPDLCStore(Engine::Content::EntitlementRef entitlement);

            /// \brief Shows the detailed download progress dialog.
            void showDownloadProgressDialog();

            /// \brief Shows the game details page of the given entitlement.
            /// \param entitlement The entitlement for which to show the details page.
            void showGameDetails(Engine::Content::EntitlementRef entitlement);

            /// \brief Shows the entitlements tile view
            void showTileView();

            /// \brief Switches to the my games tab.
            Q_INVOKABLE void showMyGames();

            /// \brief Shows the current user's profile.
            void showMyProfile(Engine::IIGOCommandController::CallOrigin callOrigin);

            /// \brief Shows the current user's profile in the given scope
            /// \param scope The scope of the Show Profile command
            /// \param source The source of the Show Profile command
            void showMyProfile(const UIScope scope, ShowProfileSource source, Engine::IIGOCommandController::CallOrigin callOrigin);

            /// \brief Shows the given user's profile.
            void showProfile(qint64 nucleusId);

            /// \brief Shows the given user's profile in the given scope
            void showProfile(qint64 nucleusId, const UIScope scope, ShowProfileSource source, Engine::IIGOCommandController::CallOrigin callOrigin);

            /// \brief Searches for profiles with the given strings
            void showProfileSearchResult(const QString &search, UIScope scope);

            /// \brief Shows the achievements front page
            void showAchievementsHome();

            /// \brief Shows an achievement set's details based on the parameters passed.
            void showAchievementSetDetails(const QString& achievementSetId, const QString& userId = "", const QString& gameTitle = "");

            /// \brief Shows the feedback page
            void showFeedbackPage();

            /// \brief Shows the order history page
            void showOrderHistory();

            /// \brief Shows the Account and Profile privacy page
            void showAccountProfilePrivacy();

            /// \brief Shows the Account and Profile security page
            void showAccountProfileSecurity();
            
            /// \brief Shows the Account and Profile Payment and Shipping page
            void showAccountProfilePaymentShipping();

            /// \brief Shows the Account and Profile Subscription page
            void showAccountProfileSubscription(const QString& status = "");
           
            /// \brief Shows the Account and Profile redemption page
            void showAccountProfileRedeem();

            /// \brief Records that the client is in social.
            /// \param nucleusID Nucleus ID of the person been viewed in social.
            void setInSocial(const QString& nucleusID);

            /// \brief Shows the detachable friends list.
            void showMyFriends();

            /// \brief Shows the select avatar window in the given scope
            void showSelectAvatar(const UIScope scope, Engine::IIGOCommandController::CallOrigin callOrigin);

            /// \brief Shows the select avatar window
            void showSelectAvatar();

            /// \brief Shows the redemption page.
            /// \param src The method of redemption.
            /// \param requestorID The ID of the requestor.
            void showRedemptionPage (RedeemBrowser::SourceType src /*= Origin*/, RedeemBrowser::RequestorID requestorID /*= OriginCodeClient*/, const QString &code);

            /// \brief Shows a dialog for searching for friends
            void showFriendSearchDialog();

            /// \brief Shows a dialog for searching for friends in the given scope
            void showFriendSearchDialog(const UIScope scope, Engine::IIGOCommandController::CallOrigin callOrigin);

            /// \brief Shows a dialog for creating groups
            void showCreateGroupDialog();

            /// \brief Shows a dialog for creating groups in the given scope
            void showCreateGroupDialog(const UIScope scope);

            /// \brief Shows a dialog for deleting groups
            void showDeleteGroupDialog(const QString& groupName, const QString& groupGuid);

            /// \brief Shows a dialog for deleting groups in the given scope
            void showDeleteGroupDialog(const QString& groupName, const QString& groupGuid, const UIScope scope);

            /// \brief Shows a dialog for leaving a group
            void showLeaveGroupDialog(const QString& groupName, const QString& groupGuid);

            /// \brief Shows a dialog for leaving a group in the given scope
            void showLeaveGroupDialog(const QString& groupName, const QString& groupGuid, const UIScope scope);

            /// \brief Shows a dialog for deleting rooms
            void showDeleteRoomDialog(const QString& groupGuid, const QString& channelId, const QString& roomName);

            /// \brief Shows a dialog for deleting rooms in the given scope
            void showDeleteRoomDialog(const QString& groupGuid, const QString& channelId, const QString& roomName, const UIScope scope);

            /// \brief Shows a dialog for entering a room password
            void showEnterRoomPasswordDialog(const QString& groupGuid, const QString& channelId);

            /// \brief Shows a dialog for entering a room passwords in the given scope
            void showEnterRoomPasswordDialog(const QString& groupGuid, const QString& channelId, const UIScope scope);

            /// \brief Shows a dialog for creating a room
            void showCreateRoomDialog(const QString& groupName, const QString& groupGuid);

            /// \brief Shows a dialog for creating a room in the given scope
            void showCreateRoomDialog(const QString& groupName, const QString& groupGuid, const UIScope scope);

            /// \brief Shows a dialog for editing a group
            void showEditGroupDialog(const QString& groupName, const QString& groupGuid);

            /// \brief Shows a dialog for editing a group in the given scope
            void showEditGroupDialog(const QString& groupName, const QString& groupGuid, const UIScope scope);

            /// \brief Shows a dialog for inviting friends to a group in the given scope
            void showInviteFriendsToGroupDialog(const QString& groupGuid, const UIScope scope = ClientScope);

            /// \brief Shows a dialog for inviting friends to a room in the given scope
            void showInviteFriendsToRoomDialog(const QString& groupGuid, const QString& channelId, const QString& conversationId, const UIScope scope = ClientScope);

            /// \brief Show a dialog explaining that chat groups are better with friends
            void showYouNeedFriendsDialog(const UIScope scope = ClientScope);

            /// \brief Shows a dialog for group members
            void showGroupMembersDialog(const QString& groupName, const QString& groupGuid);

            /// \brief Shows a dialog for group members in the given scope
            void showGroupMembersDialog(const QString& groupName, const QString& groupGuid, const UIScope scope);

            /// \brief Shows a dialog for banned group members
            void showGroupBannedMembersDialog(const QString& groupName, const QString& groupGuid);

            /// \brief Shows a dialog for banned group members in the given scope
            void showGroupBannedMembersDialog(const QString& groupName, const QString& groupGuid, const UIScope scope);

            /// \brief Shows a dialog to remove a group user
            void showRemoveGroupUserDialog(const QString& groupGuid, const QString& userId, const QString& userNickname);

            /// \brief Shows a dialog to remove a group user in the given scope
            void showRemoveGroupUserDialog(const QString& groupGuid, const QString& userId, const QString& userNickname, const UIScope scope);

            /// \brief Shows a dialog to remove a room user
            void showRemoveRoomUserDialog(const QString& groupGuid, const QString& channelId, const QString& userNickname);

            /// \brief Shows a dialog to remove a room user in the given scope
            void showRemoveRoomUserDialog(const QString& groupGuid, const QString& channelId, const QString& userNickname, const UIScope scope);

            /// \brief show promote to admin success dialog
            void showPromoteToAdminSuccessDialog (const QString& groupName, const QString& userNickname, const UIScope scope=ClientScope);

            /// \brief show promote to admin success dialog
            void showPromoteToAdminFailureDialog (const QString& userNickname, const UIScope scope=ClientScope);

            /// \brief show demote to member success dialog
            void showDemoteToMemberSuccessDialog (const QString& groupName, const QString& userNickname, const UIScope scope=ClientScope);

            /// \brief show transfer owner confirmation dialog
            void showTransferOwnershipConfirmationDialog (const QString& groupGuid, Origin::Chat::RemoteUser* user, const QString& username, const UIScope scope=ClientScope);

            /// \brief show transfer owner success dialog
            void showTransferOwnershipSuccessDialog (const QString& groupName, const QString& userNickname, const UIScope scope=ClientScope);

            /// \brief show transfer owner failure dialog
            void showTransferOwnershipFailureDialog (const QString& userNickname, const UIScope scope);

            /// \brief show group invites sent dialog, overloaded
            void showGroupInviteSentDialog(const QObjectList& users);

            /// \brief show group invites sent dialog
            void showGroupInviteSentDialog(QList<Origin::Chat::RemoteUser*> users, const UIScope scope=ClientScope);

            /// \brief Shows the application settings dialog.
            void showSettings();

            /// \brief Shows the Origin Developer Tool
            void showDeveloperTool();

            /// \brief Shows the application settings dialog - general tab
            void showSettingsGeneral();

            /// \brief Shows the application settings dialog - notifications tab
            void showSettingsNotifications();

            /// \brief Shows the application settings dialog - in game tab
            void showSettingsInGame();

            /// \brief Shows the application settings dialog - advanced tab
            void showSettingsAdvanced();

            /// \brief Shows the application settings dialog - voice tab
            void showSettingsVoice();

            /// \brief Shows the help page.
            void showHelp();

            /// \brief Shows the Origin Now tab.
            void showOriginNow();

            /// \brief Shows the Origin About popup
            void showAbout();

            /// \brief Checks if the My Origin tab is visible.
            /// \return True if the My Origin tab is visible.
            bool isMyOriginVisible();

            /// \brief Sets whether the client should start minimized.
            /// \param showState True if the client should start minimized.
            void setStartClientMinimized (ClientViewWindowShowState showState) { mStartShowState = showState; }

            /// \brief Toggles the "minimized" state of the client window.
            /// \param minimize True if the client should be minimized.
            void minimizeClientWindow (bool minimize);

            /// \brief Minimizes the client to tray.
            void minimizeClientToTray();

            /// \brief Sets user's online status.
            void setUserOnline();

            /// \brief Closes the ODT window.
            /// \return True if the window was closed.
            bool closeDeveloperTool();

            /// \brief flashes main window in taskbar.
            void alertMainWindow();

            /// \brief Returns the SocialViewController instance for the flow
            SocialViewController* socialViewController() const;

            /// \brief Returns the FriendsListViewController instance for the flow
            FriendsListViewController* friendsListViewController() const;

            /// \brief Returns the OriginToastManager instance for the flow
            OriginToastManager* originToastManager() const;

            /// \brief Returns the ClientViewController instance for the flow
            ClientViewController* view() const;

            /// \brief requests that client log out
            void requestLogoutFromJS();

            /// \brief Requests that the client enter manual offline mode
            void requestManualOfflineMode();
            
            ///
            /// \brief Starts a join multiplayer flow on the main thread
            ///
            /// This is intended for users on other threads such as the SDK. Main thread callers can construct
            /// JoinMultiplayerFlow themselves and receive flow feedback.
            ///
            void startJoinMultiplayerFlow(const Chat::JabberID &jid);

            ///
            /// \brief returns true if main client window is visible
            ///
            bool isMainWindowVisible ();

            ///
            /// \brief Launches an external browser to SSO the user into an Inicis-supported checkout flow.
            ///
            void launchInicisSsoCheckout(const QString& urlparams);

        public slots:
            /// \brief Requests that the client enter online mode
            void requestOnlineMode();

            /// \brief Silently requests that the client enter online mode. No UI shown.
            void silentRequestOnlineMode();

        signals:

            /// \brief Emitted when the user has logged out.
            void logoutConcluded();

            /// \brief Emitted when the user has requested to logout.
            void requestLogout();

            /// \brief Emitted when the user has requested to exit.
            void requestExit();

            /// \brief Emitted when we want to exit without prompting the user. E.g. Quiet mode
            void requestSilentExit();

            /// \brief Emitted when the client flow has finished.
            /// \param result The result of the client flow.
            void finished(ClientFlowResult result);

            /// \brief Emitted when the client flow has been created.
            void clientFlowCreated();

            /// \brief Emitted when the user elects to add a non origin game to the library
            void addNonOriginGame();

            /// \brief Emitted when we receive notification from dirtybit server that the user's password has changed
            void credentialsChanged();

            /// \brief Emitted when the redemption page is successfully launched
            void redemptionPageShown();
            
            /// \brief Emitted when the redemption page is closed
            void redemptionPageClosed();
            
            /// \brief Emitted when the load of the mygames view is finished
            void myGamesLoadFinished();

            /// \brief Emitted when the user loses authentication.
            void lostAuthentication();

            /// \brief Used to determine whether logout/exit should continue (if ODT is loaded).
            bool odtCloseWindowRequest();

        protected:
            /// \brief Starts the client flow.
            void startClient();

            /// \brief Starts the ITO flow.
            void startITOFlow();

            /// \brief Starts the IGO flow.
            void startIGOFlow();

            /// \brief Cleans up.
            void endCurrentSession();

        protected slots:
            /// \brief Slot that is triggered when the application is closing.
            void onGotAppCloseEvent();

            /// \brief Slot that is triggered when a game finishes playing.
            /// \param entRef TBD, was: The content ID of the game that has finished playing.
            void onPlayFinished(Origin::Engine::Content::EntitlementRef entRef);

            /// \brief Slot that is triggered when a Net Promoter doesn't show.
            void onNetPromoterNotShowing();

            /// \brief Slot that is triggered when the subscription trial eligibility response is complete
            void onSubscriptionTrialEligibilityInfoReceived();

            /// \brief Perform any required download preflighting.
            void onPreflightDownload(
                Origin::Engine::Content::LocalContent const& content,
                Origin::Engine::Content::ContentController::DownloadPreflightingResult& result);

        private slots:
            /// \brief slot connected to DirtyBits e-mail changed message
            void onEmailChanged(QByteArray payload);
            /// \brief slot conneected to DirtyBits password changed message
            void onPasswordChanged(QByteArray payload);
            /// \brief slot connected to DirtyBits originId changed message
            void onOriginIdChanged(QByteArray payload);
            /// \brief slot connected to Inicis SSO auth code request.
            void onInicisSsoCheckoutAuthCodeRetrieved();

        private:
            /// \brief register functions that will listen for DirtyBits change notices
            void connectDirtyBitsHandlers();

            Q_INVOKABLE void startJoinMultiplayerFlowAsync(const Origin::Chat::JabberID &jid);

            QScopedPointer<ClientViewController>        mClientViewController; ///< Pointer to the view controller.
            QScopedPointer<PDLCViewController>          mPDLCViewController; ///< Pointer to the PDLC view controller.

            QPointer<ITOFlow>                           mITOFlow; ///< Pointer to the ITO flow.
            QScopedPointer<IGOFlow>                     mIGOFlow; ///< Pointer to the IGO flow.

            QScopedPointer<SocialViewController>        mSocialViewController;
            QScopedPointer<OriginToastManager>         mOriginToastManager;

            ClientViewWindowShowState mStartShowState; ///< initial show state of client window - SHOW_NORMAL, SHOW_MINIMIZED, SHOW_MINIMIZED_SYSTEMTRAY

            QTimer mInicisAuthCodeRetrieveTimer; ///< used for timing out requests for auth code during Inicis SSO checkout

            QHash<Services::AuthPortalAuthorizationCodeResponse *, QString> mInicisParamMapper; ///< stores the URL parameters assocaited with a given auth code request for Inicis SSO checkout flow
        };
    }
}

#endif // CLIENTFLOW_H
