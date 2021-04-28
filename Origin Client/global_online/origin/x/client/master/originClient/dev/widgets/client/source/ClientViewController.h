/////////////////////////////////////////////////////////////////////////////
// ClientViewController.h
//
// Copyright (c) 2012-2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef CLIENT_VIEW_CONTROLLER_H
#define CLIENT_VIEW_CONTROLLER_H

#include <QObject>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKit/QWebHistoryItem>
#include <qmenubar.h>
#include "widgets/promoBrowser/source/PromoBrowserViewController.h"
#include "services/session/SessionService.h"
#include "services/settings/SettingsManager.h"
#include "engine/content/Entitlement.h"
#include "engine/igo/IIGOCommandController.h"
#include "server.h"
#include "../../navigation/source/NavigationController.h"
#include "engine/achievements/achievementset.h"
#include "services/plugin/PluginAPI.h"

#include "UIScope.h"

// Legacy APP
#include "RedeemBrowser.h"


namespace Origin
{
    namespace Chat
    {
        class RemoteUser;
    }
    namespace UIToolkit
    {
        class OriginWindow;
    }
    namespace Client
    {
        class ClientView;
        class NavBarViewController;
        class ClientMessageAreaManager;
        class FriendsListViewController;
        class NetPromoterViewController;
        class CdnOverrideViewController;
        class MainMenuController;
        class OriginWebController;
        class ChangeAvatarViewController;
        class GroupInviteViewController;
        class AcceptGroupInviteViewController;
        class GroupMembersViewController;
        class FriendSearchViewController;
        class CreateGroupViewController;
        class EditGroupViewController;
        class DeleteGroupViewController;
        class LeaveGroupViewController;
        class DeleteRoomViewController;
        class EnterRoomPasswordViewController;
        class CreateRoomViewController;
        class RemoveGroupUserViewController;
        class RemoveRoomUserViewController;
        class MandatoryUpdateViewController;
        class SocialViewController;
        class TransferOwnerConfirmViewController;
        class ClientSettingsProxy;

        enum ClientViewSuccessReason
        {
            ClientViewSuccessReason_Logout,
            ClientViewSuccessReason_Exit,
        };

        /// \brief - defines where the "showProfile" request comes from
        enum ShowProfileSource
        {
            ShowProfile_Undefined,
            ShowProfile_FriendsList,
            ShowProfile_MyProfile,
            ShowProfile_FriendsProfile,
            ShowProfile_GamesDetails,
            ShowProfile_Chat,
            ShowProfile_Achievements,
            ShowProfile_MyGames,
            ShowProfile_MainWindow,
            ShowProfile_NonFriendsProfile
        };

        ORIGIN_PLUGIN_API ShowProfileSource stringToProfileSource(const QString &str);

        class ORIGIN_PLUGIN_API ClientViewController : public QObject
        {
            Q_OBJECT

        public:

            ClientViewController(Engine::UserRef user, QWidget *parent = 0);
            ~ClientViewController();

            void init();
			void determineStartupUrl(QUrl &url);

			void show(Origin::Client::ClientViewWindowShowState showState);

            void showUrlInStore(const QUrl& url);
            void showProductInStore(const QString& productID, const QString& masterTitleID);

            // If you already know the master title ID of the product you are showing, use showProductInStore to avoid
            // the ProductInfo lookup.
            void showProductIDInStore(const QString& productID);
            void showMasterTitleInStore(const QString& masterTitleID);
            void showStoreFreeGames();
            void showStoreOnTheHouse(const QString& trackingParam);
            void showStoreHome();
            void showGameDetails(Engine::Content::EntitlementRef entitlement);
            void showTileView();
            void showMyGames();
            void showMyProfile(UIScope scope, ShowProfileSource source, Engine::IIGOCommandController::CallOrigin callOrigin);
            void showMyProfile(Engine::IIGOCommandController::CallOrigin callOrigin);
            void showProfile(quint64 nucleusId, UIScope scope, ShowProfileSource source, Engine::IIGOCommandController::CallOrigin callOrigin);
            void showProfile(quint64 nucleusId);
            void showAvatarSelect(UIScope scope, Engine::IIGOCommandController::CallOrigin callOrigin);
            void showAvatarSelect();
            void showProfileSearchResult(const QString &search, UIScope scope);

            /// \brief Shows an achievement set's details based on the parameters passed
            void showAchievementSetDetails(const QString& achievementSetId, const QString& userId = "", const QString& gameTitle = "");
            void showFeedbackPage();
            void showFriendsList();
            void showSettings();
            void showSettingsGeneral();
            void showSettingsNotifications();
            void showSettingsInGame();
            void showSettingsAdvanced();
            void showSettingsVoice();
            void showHelp();
            void showOrderHistory();
            void showAccountProfilePrivacy();
            void showAccountProfileSecurity();
            void showAccountProfilePaymentShipping();
            void showAccountProfileSubscription(const QString& status = "");
            void showAccountProfileRedeem();
            void showPromoDialog(PromoBrowserContext context, const Engine::Content::EntitlementRef ent, const bool force = false);
            void showNetPromoterDialog();
            void showCdnOverrideDialog();
            void updateEnvronmentLabel();


            void minimizeClientToTray();
        
            /// \brief saves the client size and position
            void saveClientViewSizeAndPosition();

            void alertMainWindow();

            /// \brief shows the specail Welcome to Alpha message on first launch of Origin
            void showWelcomeMessage();

            UIToolkit::OriginWindow* window() { return mMainWindow; }
            ClientView* view() { return mClientView; }

            /// \brief Returns the FriendsListViewController instance
            FriendsListViewController* friendsListViewController() const { return mFriendsListViewController; }

            ///
            /// \brief returns true if window is visible
            ///
            bool isMainWindowVisible ();

            QWidget* getWidgetParent() { return mWidgetParent; }

            // Request opening a child window of the SPA (first and foremost for OIG)
            void openJSWindow(const QString& requestType, const QString& requestExtraParams);
            void navigateJSWindow(const QString& requestType, const QString& requestExtraParams);

            /// \brief Sets whether we should listen for the hotkey input. Called from ClientSettingsProxy.cpp
            /// via Javascript when the hotkey line edit focuses-in and out.
            void hotkeyInputHasFocus(const bool& hasFocus) { mHotkeyInputHasFocus = hasFocus; if (hasFocus) mModifierKeyPressed = 0; }
            void pinHotkeyInputHasFocus(const bool& hasFocus) { mPinHotkeyInputHasFocus = hasFocus; if (hasFocus) mModifierKeyPressed = 0; }
            void pushToTalkHotKeyInputHasFocus(const bool& hasFocus) { mPushToTalkHotKeyHasFocus = hasFocus; if (hasFocus) mModifierKeyPressed = 0; }

            /// \brief Called when a keypress is detected while setting a hotkey - handles conflict detection
            void onHotkeySettingKeyPress(QKeyEvent* event);

            /// \brief Handles the OIG hotkey input.
            void handleHotkeyInput(const Services::Setting& hotkey, const Services::Setting& hotkeyStr, QKeyEvent* event);

            /// \brief Handles the VOIP Push to Talk hotkey event
            void handlePushToTalkHotkeyInput(QKeyEvent* event);

            /// \brief Handles the VOIP Push to Talk mouse event
            void handlePushToTalkMouseInput(QMouseEvent* mouseEvent);

        signals:

            //void success(ClientViewSuccessReason reason);
            void requestLogout();
            void requestExit();
            void requestShowMyProfile();
            void tabChanged(NavigationItem previousTab, NavigationItem newTab);
            void netpromoter_notshown();
            void addNonOriginGame();
            void lostAuthentication();
            
            /// \brief Emitted when the redemption page is successfully launched
            void redemptionPageShown();
            
            /// \brief Emitted when the redemption page is closed
            void redemptionPageClosed();
                        
            /// \brief emitted when the load of the mygames view is finished
            void myGamesLoadFinished();

            /// \brief emitted when the user deletes a chat group
            void groupDeleted(const QString&);

            void hotkeySettingError(const QString&, const QString&);

        public slots:
            void showRedemptionPage (RedeemBrowser::SourceType src = RedeemBrowser::Origin, RedeemBrowser::RequestorID requestorID = RedeemBrowser::OriginCodeClient, const QString &code = QString());
            void showFriendSearchDialog();
            void showFriendSearchDialog(UIScope scope, Engine::IIGOCommandController::CallOrigin callOrigin);
            void showCreateGroupDialog();
            void showCreateGroupDialog(UIScope scope);
            void showDeleteGroupDialog(const QString& groupName, const QString& groupGuid);
            void showDeleteGroupDialog(const QString& groupName, const QString& groupGuid, UIScope scope);
            void onCloseDeleteGroupDialog();
            void showLeaveGroupDialog(const QString& groupName, const QString& groupGuid);
            void showLeaveGroupDialog(const QString& groupName, const QString& groupGuid, UIScope scope);
            void onCloseLeaveGroupDialog();
            void showDeleteRoomDialog(const QString& groupGuid, const QString& channelId, const QString& roomName);
            void showDeleteRoomDialog(const QString& groupGuid, const QString& channelId, const QString& roomName, UIScope scope);
            void onCloseDeleteRoomDialog();
            void showEnterRoomPasswordDialog(const QString& groupGuid, const QString& channelId, bool deleteRoom = false);
            void showEnterRoomPasswordDialog(const QString& groupGuid, const QString& channelId, UIScope scope, bool deleteRoom = false);
            void onCloseEnterRoomPasswordDialog();
            void showCreateRoomDialog(const QString& groupName, const QString& groupGuid);
            void showCreateRoomDialog(const QString& groupName, const QString& groupGuid, UIScope scope);
            void onCloseCreateRoomDialog();
            void showEditGroupDialog(const QString& groupName, const QString& groupGuid);
            void showEditGroupDialog(const QString& groupName, const QString& groupGuid, UIScope scope);
            void onCloseEditGroupDialog();
            void showInviteFriendsToGroupDialog(const QString& groupGuid, UIScope scope);
            void showInviteFriendsToRoomDialog(const QString& groupGuid, const QString& channelId, const QString& conversationId, UIScope scope);
            void showYouNeedFriendsDialog(UIScope scope);
            void showAllFriendsInGroupDialog(UIScope scope);
            void showGroupMembersDialog(const QString& groupName, const QString& groupGuid);
            void showGroupMembersDialog(const QString& groupName, const QString& groupGuid, UIScope scope);
            void showGroupBannedMembersDialog(const QString& groupName, const QString& groupGuid);
            void showGroupBannedMembersDialog(const QString& groupName, const QString& groupGuid, UIScope scope);
            void showRemoveGroupUserDialog(const QString& groupGuid, const QString& userId, const QString& userNickname );
            void showRemoveGroupUserDialog(const QString& groupGuid, const QString& userId, const QString& userNickname, UIScope scope);
            void showRemoveRoomUserDialog(const QString& groupGuid, const QString& channelId, const QString& userNickname);
            void showRemoveRoomUserDialog(const QString& groupGuid, const QString& channelId, const QString& userNickname, UIScope scope);
            void showPromoteToAdminSuccessDialog(const QString& groupName, const QString& userNickname, UIScope scope);
            void showPromoteToAdminFailureDialog(const QString& userNickname, UIScope scope);
            void showRoomIsFullDialog(UIScope scope);
            void showDemoteToMemberSuccessDialog(const QString& groupName, const QString& userNickname, UIScope scope);
            void showTransferOwnershipSuccessDialog(const QString& groupName, const QString& userNickname, UIScope scope);            
            void showTransferOwnershipFailureDialog(const QString& userNickname, UIScope scope);            
            void showTransferOwnershipConfirmationDialog(const QString& groupGuid, Origin::Chat::RemoteUser* user, const QString& userNickname, const UIScope scope);            
            void showGroupInviteSentDialog(QList<Origin::Chat::RemoteUser*> users, UIScope scope);
            void showDownloadProgressDialog();
            void showAchievementNotification(Origin::Engine::Achievements::AchievementSetRef /*set*/, Origin::Engine::Achievements::AchievementRef achievement);
            void showAchievementsHome();
            void onLogout();
            
            /// \brief Lets the client know that it is in social. This can be triggered from several places.
            /// \param nucleusID The Nucleus ID of the person being viewed in social.
            void setInSocial(const QString& nucleusID);

            /// \brief set user online status
            void setUserOnlineState();

            void onLoadingGamesTooSlow();

            void requestLogoutFromJS();
            void requestOnlineMode(const int& friendOrigin = 0);
            void requestManualOfflineMode();
            void checkForUpdates();

            /// \brief shows privacy and account on new portal. Made public since it has sto be accesible from the web
            void onMainMenuAccount();

            void onMainMenuAbout();
            void onMainMenuDEBUGOriginDeveloperTool();

        protected:  
            // for Settings pages capturing hot key assignments
            bool eventFilter(QObject* obj, QEvent* event);

        private slots:
            //ORIGIN X
            void onNetworkRequestFinished(QNetworkReply *reply);
            void onSPALoadFinished(bool ok);
            void onSPALoadTimeout();
            void onTriggerActionWebpageStop(QNetworkReply *reply);
            void onShowFatalError();
            void onCookieSet(QString cookieName);
            void onDialogLinkClicked(const QJsonObject& args);


            //OLD CALLS
            /// \brief slot connected to dirty bits to update avatars
            void onAvatarChanged(QByteArray payload);
            void onPlayFinished(Origin::Engine::Content::EntitlementRef);


            void onPromoLoadCompleteExecuteUserAction(PromoBrowserContext context, bool promoLoadedSuccessfully);

            void onGameLibraryUpdated(QByteArray payload);
            /// \brief slot connected to dirty bits privacy settings update 
            void onPrivacySettingsUpdated(QByteArray payload);

            void onGroupsEvent(QByteArray payload);

            void onProductInfoLookupCompleteForStore();

            void entitlementsLoaded();
            void onExit();
            void onMinimizeToTray();
            /// \brief Called when the window should zoom. Customizes the zoomed size of the window.
            void onZoomed();
            void onShowMyGames(bool recordNavigation = true);
            void onShowStore(bool isLoadStoreHome = true);
            void onShowSocial(bool recoordNavigation = true);
            void onShowSearchPage(bool recordNavigation = true);
            void onShowAchievementsHome(bool recordNavigation = true);
            void onShowAchievements(bool recordNavigation = true);
            void onShowFeedbackPage(bool recoordNavigation = true);
            void onNavbarClickedShowFriendsList();
            void onNavbarClickedMyUsername();
            void onNavbarClickedMyAvatar();
            void onSettingChange(const QString&, const Origin::Services::Variant&);

            void onTabChanged(NavigationItem previousTab, NavigationItem newTab);

            void recordSocialNavigation(const QString& url);
            void recordFeedbackNavigation(const QString& url);

            void updateStoreTab();
            void updateMyGamesTab();

            void closeRedemptionPage ();

            void onSystemMenuExit();
            void restartOrigin();

            void onShowSettingsAccount(bool recordNavigation = true);
            void onShowSettingsAccountSubscription(bool recordNavigation = true);
            void onShowSettingsApplication(bool recordNavigation = true);

            /// \brief Triggered when user connection status changes 
            void onOnlineStatusChangedAuth(bool online);

            // *** BEGIN MAIN MENU HANDLERS
            void onMainMenuExit();
            void onMainMenuRedeemProductCode();
            void onMainMenuReloadMyGames();
            void onMainMenuReloadSPA();
            void onMainMenuApplicationSettings();
            void onMainMenuToggleOfflineOnline();
            void onMainMenuLogOut();
            void onMainMenuAddFriend();
            void onMainMenuOriginHelp();
            void onMainMenuOriginForum();
            void onMainMenuOrderHistory();
            void onMainMenuAddNonOriginGame();
            void onMainMenuStore();
            void onMainMenuDiscover();
            void onMainMenuMyGames();
            void onMainMenuOriginER();
#if defined(ORIGIN_MAC)
            void onMainMenuToggleFullScreen();
#endif
            
            // Debug menu
            void onMainMenuDEBUGCheckforUpdates();
            void onMainMenuDEBUGRevertToDefault();
            void onMainMenuDEBUGCdnOverride();
            void onMainMenuDEBUGDisableEmbargoMode();
            void onMainMenuDEBUGIncrementalRefresh();
			void onMainMenuDEBUGOthSettingClear();
            void onMainMenuDEBUGIGOExtraLogging(bool checked);
            void onMainMenuDEBUGIGOWebInspectors(bool checked);
            void onMainMenuDEBUGJoinPendingGroups();
            void onMainMenuDEBUGPendingGroups();

            /// \brief dirty bits test
            void onMainMenuDEBUGtestDirtyBits();
            void onMainMenuDEBUGtestDirtyBitsFakeMessages();
            void onMainMenuDEBUGtestDirtyBitsConnectHandlers();
            void onMainMenuDEBUGtestDirtyBitsFakeMessageFromFile();
            void onMainMenuDEBUGshowDirtyBitsViewer();
            void onMainMenuDEBUGsetWebSocketVerbosity();

            // *** END MAIN MENU HANDLERS

            // online mode request
            void requestOnlineModeRetry();
            void onOfflineErrorDialogClose();
            void showGroupChatCallout(const bool& force = false);

            void onRoomDeleted(const QString& channelId);
            void onDepartGroup(const QString& groupGuid);
            void onGroupDeleted(const QString& groupGuid);
            void onKickedFromGroup(const QString& groupName, const QString& groupGuid, qint64 kickedBy);
            void onGroupDeletedByRemoteUser(const QString& groupName, qint64 removedBy, const QString& groupId);
            void onCreateGroupSuccess(const QString& groupGuid);

        private:

            static const int sSPALoadTimeout;
            void showFatalErrorDialog(const QString& errorMessage);

            void bringUpWindow();

            void restoreClientViewSizeAndPosition();

            void setStartupTab();
            void handleODTRevertToDefault();

            void updateAccessibleName(QString&);
            void showAcceptGroupInviteDialog(qint64 inviteFrom, const QString& groupName, const QString& groupGuid);
            void closeGroupInviteAndMembersDialogs(const QString& groupGuid);

            void loadPersistentCookie(const QByteArray& cookieName, const Services::Setting& settingKey,const QString& urlHost);
            void persistCookie(const QByteArray& name, const Services::Setting& settingKey);
            void loadTab(const NavigationItem& navItem);

            static bool                         sWasOfflinePreviously;

            Engine::UserWRef                    mUser;
            UIToolkit::OriginWindow*            mMainWindow;
            ClientView*                         mClientView;
            NavBarViewController*               mNavBarViewController;
            ClientMessageAreaManager*           mClientMessageAreaManager;
            FriendsListViewController*          mFriendsListViewController;
            MainMenuController*                 mMainMenuController;
            PromoBrowserViewController*         mFeaturedPromoBrowserViewController;
            PromoBrowserViewController*         mFreeTrialPromoBrowserViewController;
            NetPromoterViewController*          mNetPromoterViewController;
            CdnOverrideViewController*          mCdnOverrideViewController;
            ChangeAvatarViewController*         mChangeAvatarViewController;
            FriendSearchViewController*         mFriendSearchViewController;
            CreateGroupViewController*          mCreateGroupViewController;
            MandatoryUpdateViewController*      mMandatoryUpdateViewController;
            SocialViewController*               mSocialViewController;

            QMap<QString, CreateRoomViewController*>                    mCreateRoomViewControllers;
            QMap<QString, EditGroupViewController*>                     mEditGroupViewControllers;
            QMap<QString, DeleteGroupViewController*>                   mDeleteGroupViewControllers;
            QMap<QString, LeaveGroupViewController*>                    mLeaveGroupViewControllers;
            QMap<QString, DeleteRoomViewController*>                    mDeleteRoomViewControllers;
            QMap<QString, EnterRoomPasswordViewController*>             mEnterRoomPasswordViewController;
            QMap<QString, GroupInviteViewController*>                   mGroupInviteViewControllers;
            QMap<QString, AcceptGroupInviteViewController*>             mAcceptGroupInviteViewControllers;
            QMap<QString, GroupMembersViewController*>                  mGroupMembersViewControllers;
            QMap<QString, RemoveGroupUserViewController*>               mRemoveGroupUserViewControllers;
            QMap<QString, RemoveRoomUserViewController*>                mRemoveRoomUserViewControllers;
            QMap<QString, TransferOwnerConfirmViewController*>          mTransferOwnerConfirmViewControllers;
           
            bool                                mShouldShowSocialContent;

            Origin::UIToolkit::OriginWindow*    mRedeemWindow;
            Origin::UIToolkit::OriginWindow*    mOfflineErrorDialogIGO;
            Origin::UIToolkit::OriginWindow*    mOfflineErrorDialog;

            /// \brief buffer to store last selected tab
            NavigationItem mCurrentTab;

            bool storeLoaded;
            QWidget* mWidgetParent;

            OriginWebController*                mWebController;
            QTimer                              mSPALoadTimer;

            bool mHotkeyInputHasFocus;
            bool mPinHotkeyInputHasFocus;
            bool mPushToTalkHotKeyHasFocus;
            QMap<QString, QList<const Origin::Services::Setting *>> mTakenHotkeys;  // when focus is set for hotkey, also set list of taken hotkeys so error can be sent
            int mModifierKeyPressed;
        };
    }
}

#endif //CLIENT_VIEW_CONTROLLER_H
