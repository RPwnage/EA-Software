/////////////////////////////////////////////////////////////////////////////
// ClientViewController.cpp
//
// Copyright (c) 2012-2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "ClientViewController.h"
#include "NavBarViewController.h"
#include "ClientMessageAreaManager.h"
#include "ChangeAvatarViewController.h"
#include "FriendSearchViewController.h"
#include "CreateGroupViewController.h"
#include "DeleteGroupViewController.h"
#include "DeleteRoomViewController.h"
#include "EditGroupViewController.h"
#include "LeaveGroupViewController.h"
#include "RemoveGroupUserViewController.h"
#include "RemoveRoomUserViewController.h"
#include "TransferOwnerConfirmViewController.h"
#include "EnterRoomPasswordViewController.h"
#include "CreateRoomViewController.h"
#include "services/debug/DebugService.h"
#include "services/rest/SSOTicketServiceClient.h"
#include "ClientView.h"
#include "engine/content/ContentController.h"
#include "services/settings/SettingsManager.h"
#include "engine/login/LoginController.h"
#include "OriginApplication.h"
#include "services/platform/PlatformJumplist.h"
#include "engine/content/ContentController.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/content/CatalogDefinitionController.h"
#include "engine/achievements/achievementmanager.h"
#include "engine/updater/UpdateController.h"
#include "flows/RTPFlow.h"
#include "flows/PendingActionFlow.h"
#include "flows/ContentFlowController.h"
#include "engine/social/SocialController.h"
#include "engine/social/UserNamesCache.h"
#include "TelemetryAPIDLL.h"
#include "originwindow.h"
#include "originwindowmanager.h"
#include "originwebview.h"
#include "OriginNotificationDialog.h"
#include "originmsgbox.h"
#include "origincallout.h"
#include "originlabel.h"
#include "origincallouticontemplate.h"
#include "services/log/LogService.h"
#include "engine/login/LoginController.h"
#include "engine/login/User.h"
#include "engine/config/ConfigIngestController.h"
#include "engine/content/OnTheHouseController.h"
#include "PromoBrowserViewController.h"
#include "NetPromoterViewController.h"
#include "NetPromoterViewControllerGame.h"
#include "CdnOverrideViewController.h"
#include "StoreViewController.h"
#include "services/log/LogService.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/network/GlobalConnectionStateMonitor.h"
#include "services/config/OriginConfigService.h"
#include "services/platform/PlatformJumplist.h"
#include "services/qt/QtUtil.h"
#include "services/settings/InGameHotKey.h"
#include "WebDispatcherRequestBuilder.h"
#include "StoreUrlBuilder.h"
#include "MandatoryUpdateViewController.h"
#include "DirtyBitsTrafficDebugView.h"
#include "Service/SDKService/SDK_ServiceArea.h"
#include "chat/OriginConnection.h"
#include "chat/Roster.h"
#include "chat/RemoteUser.h"
#include "FeatureCalloutController.h"
#include "chat/ConnectedUser.h"
#include "chat/BlockList.h"
#include "OriginToastManager.h"
#include "ClientFlow.h"
#include "services/platform/TrustedClock.h"
#include "CustomQMenu.h"
#include "ClientSystemMenu.h"

#include "AchievementToastView.h"
#include "AccountSettingsViewController.h"
#include "engine/content/ContentOperationQueueController.h"
#include "services/rest/VoiceServiceClient.h"
#include "services/voice/VoiceService.h"
#include "engine/voice/VoiceController.h"
#include "engine/voice/VoiceClient.h"
#include "engine/plugin/PluginManager.h"

#include "AchievementToastView.h"
#include "OfflineCdnNetworkAccessManager.h"
#include "services/network/NetworkCookieJar.h"
#include "GamesManagerProxy.h"
#include "ContentOperationQueueControllerProxy.h"
#include "OnlineStatusProxy.h"
#include "OriginSocialProxy.h"
#include "ClientSettingsProxy.h"
#include "InstallDirectoryManager.h"
#include "DialogProxy.h"
#include "OriginUserProxy.h"
#include "OriginIGOProxy.h"
#include "VoiceProxy.h"
#include "DesktopServicesProxy.h"
#include "PopOutProxy.h"

#ifdef ORIGIN_MAC
#include "MainMenuControllerOSX.h"
#else
#include "MainMenuControllerWin.h"
#endif

#include <QLayout>
#include <QDesktopWidget>
#include <QProcess>
#include <QUrl>
#include <QKeyEvent>
#include <QMouseEvent>

// Legacy APP
#include "OriginWebController.h"
#include "SocialJsHelper.h"
#include "EbisuSystemTray.h"
#include "engine/igo/IGOController.h"

// Groups
#include "GroupInviteViewController.h"
#include "InviteFriendsWindowView.h"
#include "AcceptGroupInviteViewController.h"
#include "GroupMembersViewController.h"
#include "SocialViewController.h"
#include "services/rest/GroupServiceClient.h"
#include "DialogController.h"

#include <QJsonDocument>

static const int PreferredWidth = 1065;
static const int PreferredHeight = 766;

namespace Origin
{
    using namespace UIToolkit;
    namespace Client
    {

        // amount of time we wait for the page to load before aborting
        const int ClientViewController::sSPALoadTimeout = 2 * 60 * 1000;   //2 minutes

        QMap<QString, ShowProfileSource> sourceMap()
        {
            static QMap<QString, ShowProfileSource> staticMap;

            if (staticMap.isEmpty())
            {
                staticMap["FRIENDS_LIST"] = ShowProfile_FriendsList;
                staticMap["MY_PROFILE"] = ShowProfile_MyProfile;
                staticMap["FRIENDS_PROFILE"] = ShowProfile_FriendsProfile;
                staticMap["GAME_DETAILS"] = ShowProfile_GamesDetails;
                staticMap["CHAT"] = ShowProfile_Chat;
                staticMap["ACHIEVEMENTS"]  = ShowProfile_Achievements;
                staticMap["MYGAMES"]  = ShowProfile_MyGames;
                staticMap["MAIN_CLIENT_WINDOW"] = ShowProfile_MainWindow;
                staticMap["NON_FRIENDS_PROFILE"] = ShowProfile_NonFriendsProfile;
            }

            return staticMap;
        }

        QString profileSourceToString(ShowProfileSource src)
        {
            return sourceMap().key(src);
        }

        ShowProfileSource stringToProfileSource(const QString &str)
        {
            if (sourceMap().contains(str))
            {
                return sourceMap()[str];
            }
            else
            {
                return ShowProfile_Undefined;
            }
        }
    
        ClientViewController::ClientViewController(Origin::Engine::UserRef user, QWidget *parent)
            : QObject(parent)
            , mUser(user)
            , mMainWindow(NULL)
            , mClientView(NULL)
            , mNavBarViewController(NULL)
            , mClientMessageAreaManager(NULL)
            , mFriendsListViewController(NULL)
            , mMainMenuController(NULL)
            , mFeaturedPromoBrowserViewController(NULL)
            , mFreeTrialPromoBrowserViewController(NULL)
            , mNetPromoterViewController(NULL)
            , mCdnOverrideViewController(NULL)
            , mChangeAvatarViewController(NULL)
            , mFriendSearchViewController(NULL)
            , mCreateGroupViewController(NULL)
            , mMandatoryUpdateViewController(NULL)
            , mSocialViewController(NULL)
            , mShouldShowSocialContent(true)
            , mRedeemWindow (NULL)
            , mOfflineErrorDialogIGO(NULL)
            , mOfflineErrorDialog(NULL)
            , mCurrentTab(NO_TAB)
            , storeLoaded(false)
            , mWidgetParent(parent)
            , mWebController(NULL)
            , mHotkeyInputHasFocus(false)
            , mPinHotkeyInputHasFocus(false)
            , mPushToTalkHotKeyHasFocus(false)
            , mModifierKeyPressed(0)
        {
            init();
        }

        ClientViewController::~ClientViewController()
        {
            //save out the locale cookie if it exists
            persistCookie("locale", Origin::Services::SETTING_ORIGINX_SPALOCALE);            
            Origin::Services::NetworkCookieJar *cookieJar = static_cast<Origin::Services::NetworkCookieJar*>(mMainWindow->webview()->webview()->page()->networkAccessManager()->cookieJar());
            ORIGIN_VERIFY_DISCONNECT (cookieJar, SIGNAL(setCookie (QString)), this, SLOT(onCookieSet(QString)));

            if(mWebController)
            {
                delete mWebController;
                mWebController = NULL;
            }

            if (mMainMenuController)
            {
                delete mMainMenuController;
                mMainMenuController = NULL;
            }

            if (mFriendsListViewController)
                delete mFriendsListViewController;

            // These are created by the mainwindow and are deleted when it is. Set them to NULL
            mClientView = NULL;
            mNavBarViewController = NULL;
            mClientMessageAreaManager = NULL;
//          mSocialStatusBarViewController = NULL;

            if(mFeaturedPromoBrowserViewController)
            {
                delete mFeaturedPromoBrowserViewController;
                mFeaturedPromoBrowserViewController = NULL;
            }

            if(mFreeTrialPromoBrowserViewController)
            {
                delete mFreeTrialPromoBrowserViewController;
                mFreeTrialPromoBrowserViewController = NULL;
            }

            if (mNetPromoterViewController)
            {
                delete mNetPromoterViewController;
                mNetPromoterViewController = NULL;
            }

            if (mChangeAvatarViewController)
            {
                delete mChangeAvatarViewController;
                mChangeAvatarViewController = NULL;
            }

            if (mFriendSearchViewController)
            {
                mFriendSearchViewController->deleteLater();
                mFriendSearchViewController = NULL;
            }

            if (mCreateGroupViewController)
            {
                mCreateGroupViewController->deleteLater();
                mCreateGroupViewController = NULL;
            }

            if (mSocialViewController)
            {
                mSocialViewController->disconnect();
                mSocialViewController->deleteLater();
                mSocialViewController = NULL;
            }

            for (QMap<QString, DeleteGroupViewController*>::const_iterator i = mDeleteGroupViewControllers.cbegin();
                i != mDeleteGroupViewControllers.cend(); ++i)
            {
                (*i)->deleteLater();
            }

            for (QMap<QString, DeleteRoomViewController*>::const_iterator i = mDeleteRoomViewControllers.cbegin();
                i != mDeleteRoomViewControllers.cend(); ++i)
            {
                (*i)->deleteLater();
            }

            for (QMap<QString, EditGroupViewController*>::const_iterator i = mEditGroupViewControllers.cbegin();
                i != mEditGroupViewControllers.cend(); ++i)
            {
                (*i)->deleteLater();
            }

            for (QMap<QString, LeaveGroupViewController*>::const_iterator i = mLeaveGroupViewControllers.cbegin();
                i != mLeaveGroupViewControllers.cend(); ++i)
            {
                (*i)->deleteLater();
            }

            for (QMap<QString, EnterRoomPasswordViewController*>::const_iterator i = mEnterRoomPasswordViewController.cbegin();
                i != mEnterRoomPasswordViewController.cend(); ++i)
            {
                (*i)->deleteLater();
            }

            for (QMap<QString, GroupInviteViewController*>::const_iterator i = mGroupInviteViewControllers.cbegin();
                i != mGroupInviteViewControllers.cend(); ++i)
            {
                (*i)->deleteLater();
            }

            for (QMap<QString, AcceptGroupInviteViewController*>::const_iterator i = mAcceptGroupInviteViewControllers.cbegin();
                i != mAcceptGroupInviteViewControllers.cend(); ++i)
            {
                (*i)->deleteLater();
            }
            
            for (QMap<QString, GroupMembersViewController*>::const_iterator i = mGroupMembersViewControllers.cbegin();
                i != mGroupMembersViewControllers.cend(); ++i)
            {
                (*i)->deleteLater();
            }

            for (QMap<QString, RemoveGroupUserViewController*>::const_iterator i = mRemoveGroupUserViewControllers.cbegin();
                i != mRemoveGroupUserViewControllers.cend(); ++i)
            {
                (*i)->deleteLater();
            }

            for (QMap<QString, RemoveRoomUserViewController*>::const_iterator i = mRemoveRoomUserViewControllers.cbegin();
                i != mRemoveRoomUserViewControllers.cend(); ++i)
            {
                (*i)->deleteLater();
            }

            for (QMap<QString, TransferOwnerConfirmViewController*>::const_iterator i = mTransferOwnerConfirmViewControllers.cbegin();
                i != mTransferOwnerConfirmViewControllers.cend(); ++i)
            {
                (*i)->deleteLater();
            }

            for (QMap<QString, CreateRoomViewController*>::const_iterator i = mCreateRoomViewControllers.cbegin();
                i != mCreateRoomViewControllers.cend(); ++i)
            {
                (*i)->deleteLater();
            }

            if (mMandatoryUpdateViewController)
            {
                delete mMandatoryUpdateViewController;
                mMandatoryUpdateViewController = NULL;
            }

            if (mRedeemWindow)
            {
                ORIGIN_VERIFY_DISCONNECT((RedeemBrowser*)(mRedeemWindow->content()), SIGNAL(closeWindow()), this, SLOT(closeRedemptionPage()));
                ORIGIN_VERIFY_DISCONNECT(mRedeemWindow, SIGNAL(rejected()), this, SLOT(closeRedemptionPage()));
                mRedeemWindow->close();
                mRedeemWindow = NULL;
            }

            if (mMainWindow)
            {
                delete mMainWindow;
                mMainWindow = NULL;
            }

            // Remove items from System Menu so they won't display in login screen
            EbisuSystemTray::instance()->setTrayMenu(NULL);
        }

		void ClientViewController::determineStartupUrl(QUrl &url)
		{
			QString urlStr = Origin::Services::readSetting(Origin::Services::SETTING_OriginXURL).toString();
			//we don't have actual dynamic urls specified for non-prod environment yet, so it's possible that if there's no EACore.ini override (OriginXurl=http://...)
			//the default urlStr will be empty
			if (urlStr.isEmpty())
			{
				urlStr = "https://dev.x.origin.com";
			}

			QString defaultTabStr = Origin::Services::readSetting(Origin::Services::SETTING_DefaultTab).toString();
			if (defaultTabStr == "store")
				urlStr.append("/#/store");
			else
			if (defaultTabStr == "mygames")
				urlStr.append("/#/game-library");
			else
				urlStr.append("/#/home");

			url = QUrl(urlStr);
		}

        void ClientViewController::init()
        {           
            Services::initializePerSessionSettings();


            //not hooked into anything anymore, still initialize it for now, until we can clean up all references to it
            mClientView = new ClientView(mUser);

            //setup main window
            using namespace Origin::UIToolkit;
            mMainWindow = new OriginWindow(OriginWindow::AllItems | OriginWindow::FullScreen, NULL, OriginWindow::WebContent, QDialogButtonBox::NoButton, OriginWindow::ChromeStyle_Window);
            mMainWindow->setObjectName("Origin_MainWindow");
            mMainWindow->setWindowTitle(tr("application_name"));
            mMainWindow->setContentMargin(0);
            mMainWindow->manager()->setBorderWidth(3);
            mMainWindow->setAttribute(Qt::WA_DeleteOnClose, false);
            mMainWindow->setIgnoreEscPress(true);
            mMainWindow->webview()->setHasSpinner(true);
            mMainWindow->webview()->setIsContentSize(false);

            mMainWindow->installEventFilter(this);

            if(mMainWindow->manager())
                ORIGIN_VERIFY_CONNECT(mMainWindow->manager(), SIGNAL(customizeZoomedSize()), this, SLOT(onZoomed()));
            ORIGIN_VERIFY_CONNECT(mMainWindow, SIGNAL(rejected()), this, SLOT(onMinimizeToTray()));

            int screen = QApplication::desktop()->primaryScreen();
            QRect availableGeom = QApplication::desktop()->availableGeometry(screen);
            int height = availableGeom.height() - 50;
            int width = availableGeom.width() - 50;
            QPoint upperLeft(availableGeom.center() - (QPoint(width, height) / 2));
            QRect initRect (upperLeft, QSize(width, height));
            mMainWindow->setGeometry(initRect);
            mMainWindow->show();

			QUrl url;
			determineStartupUrl(url);

            mWebController = new OriginWebController(mMainWindow->webview()->webview(), NULL, false);
            mWebController->jsBridge()->addHelper("OriginGamesManager", new JsInterface::GamesManagerProxy(this));
            mWebController->jsBridge()->addHelper("OriginContentOperationQueueController", new JsInterface::ContentOperationQueueControllerProxy(this));
            mWebController->jsBridge()->addHelper("OriginOnlineStatus",new JsInterface::OnlineStatusProxy(this));
            mWebController->jsBridge()->addHelper("OriginClientSettings", new JsInterface::ClientSettingsProxy(this));
            mWebController->jsBridge()->addHelper("InstallDirectoryManager", new JsInterface::InstallDirectoryManager(this));
            mWebController->jsBridge()->addHelper("OriginUser", new JsInterface::OriginUserProxy(mUser));
            mWebController->jsBridge()->addHelper("OriginDialogs", new JsInterface::DialogProxy(this));
            mWebController->jsBridge()->addHelper("OriginVoice", new JsInterface::VoiceProxy(this));
            mWebController->jsBridge()->addHelper("DesktopServices", new JsInterface::DesktopServicesProxy(this));
            mWebController->jsBridge()->addHelper("OriginPopOut", new JsInterface::PopOutProxy(this));
            ORIGIN_VERIFY_CONNECT(DialogController::instance(), SIGNAL(linkClicked(const QJsonObject&)), this, SLOT(onDialogLinkClicked(const QJsonObject&)));

            JsInterface::OriginIGOProxy* oigProxy = new JsInterface::OriginIGOProxy();
            oigProxy->setCreateWindowHandler(mWebController->page());
            mWebController->jsBridge()->addHelper("OriginIGO", oigProxy);

            mWebController->jsBridge()->addHelper("OriginSocialManager", new JsInterface::OriginSocialProxy(this));

            mWebController->errorHandler()->setTrackCurrentUrl(false);

            //set up the network access manager so we can set it to cache the pages when online, and load from cache when offline
            OfflineCdnNetworkAccessManager* offlineNam = new OfflineCdnNetworkAccessManager(mMainWindow->webview()->webview()->page());
            Origin::Services::NetworkCookieJar *cookieJar = new Services::NetworkCookieJar(offlineNam);
            offlineNam->cookieJar()->deleteLater();
            offlineNam->setCookieJar(cookieJar);

            mMainWindow->webview()->webview()->page()->setNetworkAccessManager (offlineNam);
            ORIGIN_VERIFY_CONNECT(offlineNam, SIGNAL(finished(QNetworkReply*)), this, SLOT(onNetworkRequestFinished(QNetworkReply*)));

            mSPALoadTimer.setInterval(sSPALoadTimeout);
            ORIGIN_VERIFY_CONNECT(&mSPALoadTimer, SIGNAL(timeout()), this, SLOT(onSPALoadTimeout()));
            ORIGIN_VERIFY_CONNECT(mMainWindow->webview()->webview()->page(), SIGNAL(loadFinished(bool)), this, SLOT(onSPALoadFinished(bool)));

            //need to do this AFTER the NAM is set up, otherwise, in offline mode it will fail because it won't try to load it from the cache

            //if we're offline then we need to check and see if we were able to load the SPA successfully during the last online session
            bool online = Origin::Services::Connection::ConnectionStatesService::isUserOnline (mUser.toStrongRef()->getSession());

            bool loadSPA = true;
            if (!online)
            {
                bool spaLoadedOk = Origin::Services::readSetting(Origin::Services::SETTING_ORIGINX_SPALOADED);
                bool preloadOk = Origin::Services::readSetting(Origin::Services::SETTING_ORIGINX_SPAPRELOADSUCCESS); //preload success wil lcome across the bridge
                if (!spaLoadedOk || !preloadOk)
                {
                    onSPALoadFinished(false);
                    loadSPA = false;
                }
            }

            if (loadSPA)
            {
                //intialize the successfully-loaded flag in case the user just decides to exit before load has finished
                //if the load fails due to timeout or network error, then onShowFatalError will also set the flag to false
                Origin::Services::writeSetting(Origin::Services::SETTING_ORIGINX_SPALOADED, false);

                //retrieve the cookie if it exists
                loadPersistentCookie("locale", Origin::Services::SETTING_ORIGINX_SPALOCALE, url.host());

                //hook up to listen for any cookie changes; do this after loadPersistenCookie since that will trigger the signal
                Origin::Services::NetworkCookieJar *cookieJar = static_cast<Origin::Services::NetworkCookieJar*>(mMainWindow->webview()->webview()->page()->networkAccessManager()->cookieJar());
                ORIGIN_VERIFY_CONNECT (cookieJar, SIGNAL(setCookie (QString)), this, SLOT (onCookieSet(QString)));

                mWebController->loadTrustedUrl(url);
                mSPALoadTimer.start();
                qint64 startTS = QDateTime::currentMSecsSinceEpoch ();
                ORIGIN_LOG_EVENT << "SPA Load Start TS: " << startTS;
            }


            // MAIN MENU BAR
            const bool underage = Origin::Engine::LoginController::isUserLoggedIn() ? mUser.toStrongRef()->isUnderAge() : false;
            //online already retrieved above
            const bool debug = Origin::Services::readSetting(Origin::Services::SETTING_ShowDebugMenu, Origin::Services::Session::SessionRef());
            const bool developer = Origin::Services::readSetting(Origin::Services::SETTING_OriginDeveloperToolEnabled, Origin::Services::Session::SessionRef());

            // Jump List
            Services::PlatformJumplist::create_jumplist(underage);
            EbisuSystemTray* systemTray = EbisuSystemTray::instance();
            ClientSystemMenu* trayMenu = new ClientSystemMenu();
            ORIGIN_VERIFY_CONNECT(trayMenu, SIGNAL(logoutTriggered()), this, SLOT(onLogout()));
            ORIGIN_VERIFY_CONNECT(trayMenu, SIGNAL(redeemTriggered()), this, SLOT(onMainMenuRedeemProductCode()));
            ORIGIN_VERIFY_CONNECT(trayMenu, SIGNAL(quitTriggered()), this, SLOT(onSystemMenuExit()));
            ORIGIN_VERIFY_CONNECT(trayMenu, SIGNAL(openTriggered()), systemTray, SLOT(showPrimaryWindow()));
            systemTray->setTrayMenu(trayMenu);
            systemTray->setPrimaryWindow(mMainWindow);

            updateEnvronmentLabel();

#ifdef ORIGIN_MAC
            mMainWindow->setTitleBarText(tr("application_name"));
            bool fullscreen = mMainWindow->supportsFullscreen();
            mMainMenuController = new MainMenuControllerOSX(underage, online, debug, developer, mMainWindow);
            // With Qt on Mac we just have to instanciate a QMenuBar for the application menu
            // to show up in the titlebar. The Qt docs say to NOT assign them to a parent widget
            // if you want the menu to be global across all windows in the app.
            // http://qt-project.org/doc/qt-4.8/QMenuBar.html#qmenubar-on-mac-os-x

            MainMenuControllerOSX* menuControllerOSX = dynamic_cast<MainMenuControllerOSX*>(mMainMenuController);
            if (menuControllerOSX)
                menuControllerOSX->setFullscreenSupported(fullscreen);
#else
            mMainMenuController = new MainMenuControllerWin(underage, online, debug, developer);
            mMainWindow->menuBarLayout()->addWidget(mMainMenuController->menuBar());
            mMainWindow->setMenuBarVisible(true);
            // No titlebar text on Windows since we have a titlebar menu
            mMainWindow->setTitleBarText("");
#endif

            // Connect the menu actions
            ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionExit()), this, SLOT(onMainMenuExit()));
            ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionRedeemProductCode()), this, SLOT(onMainMenuRedeemProductCode()));
            ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionReloadMyGames()), this, SLOT(onMainMenuReloadMyGames()));
            ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionReloadSPA()), this, SLOT(onMainMenuReloadSPA()));
            ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionApplicationSettings()), this, SLOT(onMainMenuApplicationSettings()));
            ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionAccount()), this, SLOT(onMainMenuAccount()));
            ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionToggleOfflineOnline()), this, SLOT(onMainMenuToggleOfflineOnline()));

            ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionLogOut()), this, SLOT(onMainMenuLogOut()));
            ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionAddFriend()), this, SLOT(onMainMenuAddFriend()));
            ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionOriginHelp()), this, SLOT(onMainMenuOriginHelp()));
            ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionOriginForum()), this, SLOT(onMainMenuOriginForum()));
            ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionOrderHistory()), this, SLOT(onMainMenuOrderHistory()));
            ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionAbout()), this, SLOT(onMainMenuAbout()));
            ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionAddNonOriginGame()), this, SLOT(onMainMenuAddNonOriginGame()));

#if defined(ORIGIN_PC)
            ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionOER()), this, SLOT(onMainMenuOriginER()));
#endif

#if defined(ORIGIN_MAC)
            ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionMyGames()), this, SLOT(onMainMenuMyGames()));
            ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionStore()), this, SLOT(onMainMenuStore()));
            ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionDiscover()), this, SLOT(onMainMenuDiscover()));
            ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionToggleFullScreen()), this, SLOT(onMainMenuToggleFullScreen()));

            // Watch for changes in fullscreen mode
            ORIGIN_VERIFY_CONNECT(mMainWindow, SIGNAL(fullScreenChanged(bool)), mMainMenuController, SLOT(onMainWindowFullscreenChanged(bool)));
#endif

            if (debug)
            {
                // Debug menu actions
                ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionDEBUGCheckforUpdates()), this, SLOT(onMainMenuDEBUGCheckforUpdates()));
                ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionDEBUGtestDirtyBits()), this, SLOT(onMainMenuDEBUGtestDirtyBits()));
                ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionDEBUGtestDirtyBitsFakeMessages()), this, SLOT(onMainMenuDEBUGtestDirtyBitsFakeMessages()));
                ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionDEBUGsetWebSocketVerbosity()), this, SLOT(onMainMenuDEBUGsetWebSocketVerbosity()));
                ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionDEBUGtestDirtyBitsConnectHandlers()), this, SLOT(onMainMenuDEBUGtestDirtyBitsConnectHandlers()));
                ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionDEBUGtestDirtyBitsFakeMessageFromFile()), this, SLOT(onMainMenuDEBUGtestDirtyBitsFakeMessageFromFile()));
                ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionDEBUGshowDirtyBitsTrafficDebugViewer()), this, SLOT(onMainMenuDEBUGshowDirtyBitsViewer()));
                ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionDEBUGCdnOverride()), this, SLOT(onMainMenuDEBUGCdnOverride()));
                ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionDEBUGDisableEmbargoMode()), this, SLOT(onMainMenuDEBUGDisableEmbargoMode()));
                ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionDEBUGIncrementalRefresh()), this, SLOT(onMainMenuDEBUGIncrementalRefresh()));
                ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionDEBUGOtHSettingClear()), this, SLOT(onMainMenuDEBUGOthSettingClear()));
                ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionDEBUGIGOExtraLogging(bool)), this, SLOT(onMainMenuDEBUGIGOExtraLogging(bool)));
                ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionDEBUGJoinPendingGroups()), this, SLOT(onMainMenuDEBUGJoinPendingGroups()));
            }

            if (debug || developer)
            {
                ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionDEBUGRevertToDefault()), this, SLOT(onMainMenuDEBUGRevertToDefault()));
            }

            if (developer)
            {
                // Developer tool menu actions
                ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionDEBUGOriginDeveloperTool()), this, SLOT(onMainMenuDEBUGOriginDeveloperTool()));
                ORIGIN_VERIFY_CONNECT(mMainMenuController, SIGNAL(menuActionDEBUGIGOWebInspectors(bool)), this, SLOT(onMainMenuDEBUGIGOWebInspectors(bool)));
            }


            ORIGIN_VERIFY_CONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(userConnectionStateChanged(bool, Origin::Services::Session::SessionRef)), 
                this, SLOT(onOnlineStatusChangedAuth(bool)));

            Origin::Engine::Content::ContentController *contentController = Origin::Engine::Content::ContentController::currentUserContentController();
            // clear the download queue here so we don't have to do it in onInitialEntitlementLoad() as for now we are allowing user to add things before the queue is restored
            mUser.toStrongRef()->contentOperationQueueControllerInstance()->clearDownloadQueue();
            mUser.toStrongRef()->contentOperationQueueControllerInstance()->LoadInitialQueue();	// load the strings from the settings here in case something gets put on the queue and wipes out the setting

            if (contentController && contentController->initialEntitlementRefreshCompleted())
            {
                // Entitlements already loaded
                entitlementsLoaded();
            }
            else
            {
                //setup connections to intitialize the myGamesView
                //we eventually may not need to wait for the entitlements to be refreshed, but for now lets wait
                ORIGIN_VERIFY_CONNECT(contentController, SIGNAL(updateFinished(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>)), this, SLOT(entitlementsLoaded()));
                ORIGIN_VERIFY_CONNECT(contentController, SIGNAL(updateFailed()), this, SLOT(entitlementsLoaded()));         
            }
#if 0
            mClientView = new ClientView(mUser);
            ORIGIN_VERIFY_CONNECT(mClientView, SIGNAL(lostAuthentication()), this, SIGNAL(lostAuthentication()));
            ORIGIN_VERIFY_CONNECT(NavigationController::instance(), SIGNAL(currentId(QString)), mClientView, SLOT(showProfilePage(QString)));

            // Set name for automation
            mMainWindow->setContent(mClientView);

            updateEnvronmentLabel();

            ORIGIN_VERIFY_CONNECT(this, SIGNAL(tabChanged(NavigationItem, NavigationItem)), this, SLOT(onTabChanged(NavigationItem, NavigationItem)));

            ORIGIN_VERIFY_CONNECT(
                Origin::Services::SettingsManager::instance(), SIGNAL(settingChanged(const QString&, const Origin::Services::Variant&)),
                this, SLOT(onSettingChange(const QString&, const Origin::Services::Variant&)));

            Origin::Engine::UserRef user = mUser.toStrongRef();
            mNavBarViewController = new NavBarViewController(user, mClientView->getNavBar());
            ORIGIN_VERIFY_CONNECT(mNavBarViewController, SIGNAL(exit()), this, SLOT(onExit()));
            ORIGIN_VERIFY_CONNECT(mNavBarViewController, SIGNAL(logout()), this, SLOT(onLogout()));
            ORIGIN_VERIFY_CONNECT(mNavBarViewController, SIGNAL(showStore()), this, SLOT(onShowStore()));
            ORIGIN_VERIFY_CONNECT(mNavBarViewController, SIGNAL(showMyGames()), this, SLOT(onShowMyGames()));
            ORIGIN_VERIFY_CONNECT(mNavBarViewController, SIGNAL(addFriend()), this, SLOT(showFriendSearchDialog()));
            ORIGIN_VERIFY_CONNECT(mNavBarViewController, SIGNAL(showRedemption(RedeemBrowser::SourceType, RedeemBrowser::RequestorID)), this, SLOT (showRedemptionPage(RedeemBrowser::SourceType, RedeemBrowser::RequestorID)));
            ORIGIN_VERIFY_CONNECT(mNavBarViewController, SIGNAL(clickedShowFriendsList()), this, SLOT(onNavbarClickedShowFriendsList()));
            ORIGIN_VERIFY_CONNECT(mNavBarViewController, SIGNAL(clickedMyUsername()), this, SLOT(onNavbarClickedMyUsername()));
            ORIGIN_VERIFY_CONNECT(mNavBarViewController, SIGNAL(clickedMyAvatar()), this, SLOT(onNavbarClickedMyAvatar()));
            ORIGIN_VERIFY_CONNECT(mNavBarViewController, SIGNAL(clickedAchievements()), this, SLOT(onShowAchievementsHome()));
            ORIGIN_VERIFY_CONNECT(mNavBarViewController, SIGNAL(goOnline()), this, SLOT(requestOnlineMode()));
            ORIGIN_VERIFY_CONNECT(Origin::SDK::SDK_ServiceArea::instance(), SIGNAL(goOnline()), this, SLOT(requestOnlineMode()));

            ORIGIN_VERIFY_CONNECT(mNavBarViewController, SIGNAL(clickedAchievements())
                , NavigationController::instance(), SLOT(achievementHomeChosen()));

            mClientMessageAreaManager = new ClientMessageAreaManager(mClientView->getMessageArea());
            ORIGIN_VERIFY_CONNECT(mClientMessageAreaManager, SIGNAL(goOnline()), this, SLOT(requestOnlineMode()));
            ORIGIN_VERIFY_CONNECT(mClientMessageAreaManager, SIGNAL(showAccountSettings()), this, SLOT(onShowSettingsAccount()));
            ORIGIN_VERIFY_CONNECT(mClientMessageAreaManager, SIGNAL(changeMessageAreaVisibility(const bool&)), mClientView, SLOT(onChangeMessageAreaVisibility(const bool&)));
            mClientMessageAreaManager->init();

            Origin::Engine::Content::ContentController *contentController = Origin::Engine::Content::ContentController::currentUserContentController();

			// clear the download queue here so we don't have to do it in onInitialEntitlementLoad() as for now we are allowing user to add things before the queue is restored
			user->contentOperationQueueControllerInstance()->clearDownloadQueue();
			user->contentOperationQueueControllerInstance()->LoadInitialQueue();	// load the strings from the settings here in case something gets put on the queue and wipes out the setting

            if (contentController && contentController->initialEntitlementRefreshCompleted())
            {
                // Entitlements already loaded
                entitlementsLoaded();
            }
            else
            {
                //setup connections to intitialize the myGamesView
                //we eventually may not need to wait for the entitlements to be refreshed, but for now lets wait
                ORIGIN_VERIFY_CONNECT(contentController, SIGNAL(entitlementsInitialized()), this, SLOT(entitlementsLoaded()));
                ORIGIN_VERIFY_CONNECT(contentController, SIGNAL(updateFailed()), this, SLOT(entitlementsLoaded()));         
                ORIGIN_VERIFY_CONNECT(contentController, SIGNAL(playFinished(Origin::Engine::Content::EntitlementRef)), this, SLOT(onPlayFinished(Origin::Engine::Content::EntitlementRef)));         
            }

            // Settings
            ORIGIN_VERIFY_CONNECT(mClientView, SIGNAL(showSettings()), this, SLOT(onShowSettingsAccount()));
        
            // Navigation
            ORIGIN_VERIFY_CONNECT(mClientView, SIGNAL(recordSocialNavigation(const QString&)), this, SLOT(recordSocialNavigation(const QString&)));
            ORIGIN_VERIFY_CONNECT(mClientView, SIGNAL(recordFeedbackNavigation(const QString&)), this, SLOT(recordFeedbackNavigation(const QString&)));
            ORIGIN_VERIFY_CONNECT(NavigationController::instance(), SIGNAL(showStore(bool)), this, SLOT(onShowStore(bool)));
            ORIGIN_VERIFY_CONNECT(NavigationController::instance(), SIGNAL(showMyGames(bool)), this, SLOT(onShowMyGames(bool)));
            ORIGIN_VERIFY_CONNECT(NavigationController::instance(), SIGNAL(showSocial(bool)), this, SLOT(onShowSocial(bool)));
            ORIGIN_VERIFY_CONNECT(NavigationController::instance(), SIGNAL(showSearchPage(bool)), this, SLOT(onShowSearchPage(bool)));
            ORIGIN_VERIFY_CONNECT(NavigationController::instance(), SIGNAL(showFeedbackPage(bool)), this, SLOT(onShowFeedbackPage(bool)));
            ORIGIN_VERIFY_CONNECT(NavigationController::instance(), SIGNAL(showAchievements(bool)), this, SLOT(onShowAchievements(bool)));
            ORIGIN_VERIFY_CONNECT(NavigationController::instance(), SIGNAL(showAccountSettings(bool)), this, SLOT(onShowSettingsAccount(bool)));
            ORIGIN_VERIFY_CONNECT(NavigationController::instance(), SIGNAL(showApplicationSettings(bool)), this, SLOT(onShowSettingsApplication(bool)));
            ORIGIN_VERIFY_CONNECT(NavigationController::instance(), SIGNAL(clickStoreLayout()), this, SLOT(updateStoreTab()));
            ORIGIN_VERIFY_CONNECT(NavigationController::instance(), SIGNAL(clickGamesLayout()), this, SLOT(updateMyGamesTab()));


            ORIGIN_VERIFY_CONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(userConnectionStateChanged(bool, Origin::Services::Session::SessionRef)), 
                          this, SLOT(onOnlineStatusChangedAuth(bool)));

            // OTH is PC only
#ifdef ORIGIN_PC
            ORIGIN_VERIFY_CONNECT(Engine::Content::OnTheHouseController::instance(), SIGNAL(finished()), this, SLOT(showGroupChatCallout()));
#else
            showGroupChatCallout();
#endif
            ORIGIN_VERIFY_CONNECT(contentController, SIGNAL(loadingGamesTooSlow()), this, SLOT(onLoadingGamesTooSlow()));

            if(underage == true)
            {
                ORIGIN_VERIFY_DISCONNECT(Origin::Engine::Achievements::AchievementManager::instance(), 
                    SIGNAL(granted(Origin::Engine::Achievements::AchievementSetRef, Origin::Engine::Achievements::AchievementRef)), 
                    this, SLOT(showAchievementNotification(Origin::Engine::Achievements::AchievementSetRef, Origin::Engine::Achievements::AchievementRef)));
            }
            else
            {
                ORIGIN_VERIFY_CONNECT_EX(Origin::Engine::Achievements::AchievementManager::instance(), 
                    SIGNAL(granted(Origin::Engine::Achievements::AchievementSetRef, Origin::Engine::Achievements::AchievementRef)), 
                    this, SLOT(showAchievementNotification(Origin::Engine::Achievements::AchievementSetRef, Origin::Engine::Achievements::AchievementRef)), Qt::UniqueConnection);
            }

            restoreClientViewSizeAndPosition();

            // Startup Tab
            setStartupTab();


            mMandatoryUpdateViewController = new MandatoryUpdateViewController(NULL);
            ORIGIN_VERIFY_CONNECT(mMandatoryUpdateViewController, SIGNAL(requestUpdateCheck()), this, SLOT(checkForUpdates()));
            checkForUpdates();

            Chat::Connection* connection = Engine::LoginController::instance()->currentUser()->socialControllerInstance()->chatConnection();
            Chat::ConnectedUser* currentUser = connection->currentUser();
            ORIGIN_VERIFY_CONNECT(Engine::LoginController::instance()->currentUser()->socialControllerInstance(), SIGNAL(groupDestroyed(const QString&, qint64, const QString&)), this, SLOT(onGroupDeletedByRemoteUser(const QString&, qint64, const QString&))); 
            ORIGIN_VERIFY_CONNECT(Engine::LoginController::instance()->currentUser()->socialControllerInstance(), SIGNAL(kickedFromGroup(const QString&, const QString&, qint64)), this, SLOT(onKickedFromGroup(const QString&, const QString&, qint64))); 

            QSignalMapper* signalMapper =  new QSignalMapper(this);
            signalMapper->setMapping(currentUser->blockList(), AccountSettingsViewController::Privacy);
            ORIGIN_VERIFY_CONNECT_EX(currentUser->blockList(), SIGNAL(updated()), signalMapper, SLOT(map()), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT(signalMapper, SIGNAL(mapped(int)), mClientView, SLOT(refreshAccountSettingPage(const int&))); 
#endif
        }

        void ClientViewController::onNetworkRequestFinished(QNetworkReply *reply)
        {
//            QString urlString(reply->url().toString());

            // Cache control indicates if this was an offline/cached load
//            int cacheLoadControl = reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toInt();
//            QNetworkAccessManager::Operation op = reply->operation ();
//            ORIGIN_LOG_EVENT << "reply received for :" << urlString << "op = " << op << " cacheLoadControl = " << cacheLoadControl << " error = " << reply->error();
        }

        void ClientViewController::onSPALoadFinished(bool ok)
        {
            mSPALoadTimer.stop();
            ORIGIN_VERIFY_DISCONNECT(&mSPALoadTimer, SIGNAL(timeout()), this, SLOT(onSPALoadTimeout()));

            //user may have closed the window before the response came back
            if (mMainWindow && mMainWindow->webview() && mWebController)
            {
                ORIGIN_VERIFY_DISCONNECT(mMainWindow->webview()->webview()->page(), SIGNAL(loadFinished(bool)), this, SLOT(onSPALoadFinished(bool)));

                if (ok)
                {
                    //save off the we loaded ok
                    Origin::Services::writeSetting(Origin::Services::SETTING_ORIGINX_SPALOADED, true);
                    Origin::Services::SettingsManager::instance()->sync();
                } 
                else    //failure to load
                {
                    //we need this to be a queued connection because after forcing a stop on load due to timeout, we need to wait one event loop before
                    //attempting to show the fatal error dialog
                    QMetaObject::invokeMethod(this, "onShowFatalError", Qt::QueuedConnection);
                }
            }
        }

        void ClientViewController::onSPALoadTimeout()
        {
            mSPALoadTimer.stop();

            ORIGIN_VERIFY_DISCONNECT(&mSPALoadTimer, SIGNAL(timeout()), this, SLOT(onSPALoadTimeout()));

            //guard against the case when user may already be in the process of closing the window
            if (mMainWindow && mMainWindow->webview() && mWebController)
            {
                ORIGIN_VERIFY_DISCONNECT(mMainWindow->webview()->webview()->page(), SIGNAL(loadFinished(bool)), this, SLOT(onSPALoadFinished(bool)));

                ORIGIN_VERIFY_DISCONNECT(mMainWindow->webview()->webview()->page()->networkAccessManager(), 
                                        SIGNAL(finished(QNetworkReply*)), 
                                        this, 
                                        SLOT(onNetworkRequestFinished(QNetworkReply*)));

                ORIGIN_VERIFY_CONNECT(mMainWindow->webview()->webview()->page()->networkAccessManager(), 
                                      SIGNAL(finished(QNetworkReply *)), 
                                      this, 
                                      SLOT(onTriggerActionWebpageStop(QNetworkReply *)));

                mMainWindow->webview()->webview()->page()->triggerAction (QWebPage::Stop);
            }
        }

        void ClientViewController::onTriggerActionWebpageStop(QNetworkReply* reply)
        {
            Q_UNUSED(reply)

            //guard against the case when the use is closing the window
            if (mMainWindow && mMainWindow->webview() && mWebController)
            {
                ORIGIN_VERIFY_DISCONNECT(mMainWindow->webview()->webview()->page()->networkAccessManager(), 
                                      SIGNAL(finished(QNetworkReply *)), 
                                      this, 
                                      SLOT(onTriggerActionWebpageStop(QNetworkReply *)));

                ORIGIN_VERIFY_CONNECT(mMainWindow->webview()->webview()->page()->networkAccessManager(), 
                                        SIGNAL(finished(QNetworkReply*)), 
                                        this, 
                                        SLOT(onNetworkRequestFinished(QNetworkReply*)));

                //we need this to be a queued connection because after forcing a stop on load due to timeout, we need to wait one event loop before
                //attempting to show the fatal error dialog
                QMetaObject::invokeMethod(this, "onShowFatalError", Qt::QueuedConnection);
            }
        }

        void ClientViewController::onShowFatalError()
        {
            Origin::Services::writeSetting(Origin::Services::SETTING_ORIGINX_SPALOADED, false);
            Origin::Services::SettingsManager::instance()->sync();
            showFatalErrorDialog(tr("ebisu_x_fatal_offline"));
        }

        //triggered when cookie is changed/set in webview
        void ClientViewController::onCookieSet(QString cookieName)
        {
            if (cookieName == "locale")
            {
                persistCookie("locale", Origin::Services::SETTING_ORIGINX_SPALOCALE);            
            }
        }

        void ClientViewController::onDialogLinkClicked(const QJsonObject& obj)
        {
            const QString href = Services::JsonValueValidator::validate(obj["href"]).toString();
            if(href.contains("openOER", Qt::CaseInsensitive))
            {
#if defined(ORIGIN_PC)
                QString program = "OriginER.exe";
                QString objectKeyword = "source:";
                QString key = "";
                QString source = "";
                QStringList arguments;
                Engine::UserRef user = Engine::LoginController::currentUser();
                QString keyAndSource = sender()->objectName();

                int pos = sender()->objectName().indexOf(objectKeyword);
                key = keyAndSource.left(pos);
                if(keyAndSource.length()-pos-objectKeyword.length() > 0)
                    source = keyAndSource.right(keyAndSource.length()-pos-objectKeyword.length());
                else
                    source = "none";

                arguments << "-originId" << user->eaid() << "-category" << "5" << "-description" << key << "-source" << source;
                bool isStarted = QProcess::startDetached(program, arguments, ".");

                if (!isStarted)
                    ORIGIN_LOG_ACTION << "unable to launch OriginER";
#endif
            }
        }

        void ClientViewController::showFatalErrorDialog(const QString& errorMessage)
        {
            QFile resourceFile(QString(":html/errorPage.html"));
            if (!resourceFile.open(QIODevice::ReadOnly))
            {
                ORIGIN_ASSERT(0);
                    return;
            }

            // Replace our stylesheet
            QString stylesheet = UIToolkit::OriginWebView::getErrorStyleSheet(true);
            QString p = QString::fromUtf8(resourceFile.readAll())
            .arg(stylesheet)
            .arg(tr("ebisu_x_fatal_offline"))
            .arg(errorMessage);

            ORIGIN_LOG_EVENT << "Loading fatal offline load error page";
            mWebController->page()->mainFrame()->setHtml(p);
        }

        void ClientViewController::loadPersistentCookie(const QByteArray& cookieName, const Services::Setting& settingKey, const QString& urlHost)
        {
            QString settingValue = Services::readSetting(settingKey).toString();
            if (!settingValue.isEmpty())
            {
                QNetworkCookie cookie(cookieName, settingValue.toLatin1());
                
                QList<QNetworkCookie> cookieList;
                cookieList.push_back(cookie);

                QUrl url;
                url.setHost(urlHost);

                //use the cookie jar associated with the webview, since the cookie is one set by DOM and therefore is NOT in the shared cookie jar
                Origin::Services::NetworkCookieJar *cookieJar = static_cast<Origin::Services::NetworkCookieJar*>(mMainWindow->webview()->webview()->page()->networkAccessManager()->cookieJar());
                cookieJar->setCookiesFromUrl(cookieList, url);
            }
        }

        void ClientViewController::persistCookie(const QByteArray& name, const Services::Setting& settingKey)
        {
            QNetworkCookie c;
            //use the cookie jar associated with the webview, since the cookie is one set by DOM and therefore is NOT in the shared cookie jar
            Origin::Services::NetworkCookieJar *cookieJar = static_cast<Origin::Services::NetworkCookieJar*>(mMainWindow->webview()->webview()->page()->networkAccessManager()->cookieJar());
            
            if (cookieJar->findFirstCookieByName(name, c))
            {
                Services::writeSetting(settingKey, QString(c.value()));
            }
            else
            {
                // Clear the persisted value since no 'hl' cookie exists.
                Services::writeSetting(settingKey, "");
            }
        }

        void ClientViewController::loadTab(const NavigationItem& navItem)
        {
            QString navStr;
            switch (navItem)
            {
            case MY_GAMES_TAB:
                navStr = "/#/game-library";
                break;
            case STORE_TAB:
                navStr = "/#/store";
                break;
            case SETTINGS_APPLICATION_TAB:
            case SETTINGS_ACCOUNT_TAB:
                navStr = "/#/settings";
                break;
            case MY_ORIGIN_TAB:
                navStr = "/#/home";
                break;
            default:
                break;
            }

            QString urlStr = Origin::Services::readSetting(Origin::Services::SETTING_OriginXURL).toString();
            //we don't have actual dynamic urls specified for non-prod environment yet, so it's possible that if there's no EACore.ini override (OriginXurl=http://...)
            //the default urlStr will be empty
            if (urlStr.isEmpty())
            {
                urlStr = "https://dev.x.origin.com";
            }
            urlStr.append(navStr);
            QUrl url(urlStr);
            mWebController->loadTrustedUrl(url);
        }

        void ClientViewController::showGroupChatCallout(const bool& force)
        {
            bool okToShow = false;
            const QString settingName = Services::SETTING_CALLOUT_SOCIALUSERAREA_SHOWN.name();
            if(force)
            {
                if(Engine::LoginController::currentUser()->isUnderAge())
                    ORIGIN_LOG_ACTION << "Feature Callout: SocialUserArea - User is underage";
                else
                {
                okToShow = true;
            }
            }
            else
            {
                if(!(Services::readSetting(Services::SETTING_EnableFeatureCallouts, Services::Session::SessionService::currentSession())))
                    ORIGIN_LOG_ACTION << "Feature Callout: SocialUserArea - Callouts disabled";
                else if(OriginApplication::instance().isITE())
                    ORIGIN_LOG_ACTION << "Feature Callout: SocialUserArea - ITO active";
                else if(Origin::Services::Connection::ConnectionStatesService::isUserOnline(Engine::LoginController::currentUser()->getSession()) == false)
                    ORIGIN_LOG_ACTION << "Feature Callout: SocialUserArea - User offline";
                else if(Engine::LoginController::currentUser()->isUnderAge())
                    ORIGIN_LOG_ACTION << "Feature Callout: SocialUserArea - User is underage";
                else if(Services::readSetting(settingName, Services::Session::SessionService::currentSession()))
                    ORIGIN_LOG_ACTION << "Feature Callout: SocialUserArea - Shown";

// OTH is PC only
#ifdef ORIGIN_PC
                else if(Engine::Content::OnTheHouseController::instance()->isResponseFinished() == false)
                    ORIGIN_LOG_ACTION << "Feature Callout: SocialUserArea - OTH response not complete yet";
                else if(Engine::Content::OnTheHouseController::instance()->willPromoShow())
                    ORIGIN_LOG_ACTION << "Feature Callout: SocialUserArea - OTH Showing";
#endif
                else
                {
                    okToShow = true;
                }
            }

            UIToolkit::OriginCallout* callout = MainFlow::instance()->featureCalloutController()->callout(settingName);
            if(okToShow && callout == NULL)
            {
                QList<UIToolkit::OriginCalloutIconTemplateContentLine*> lines;
#if ENABLE_VOICE_CHAT
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( isVoiceEnabled )
                {
                    lines.append(new UIToolkit::OriginCalloutIconTemplateContentLine(":/GUI/icon-phone.png", tr("ebisu_client_call_your_origin_friends_directly_on_their_computers")));
                    lines.append(new UIToolkit::OriginCalloutIconTemplateContentLine(":/GUI/icon-friend.png", tr("ebisu_client_rooms_allow_you_to_chat_with_a_bunch")));
                    // Will be parented to MainWindow and will be killed with it.
                    MainFlow::instance()->featureCalloutController()->showCallout(settingName, tr("ebisu_client_new_title"), tr("ebisu_client_voice_chat_and_groups_title"), lines);
                }
                else
                {
#endif
                lines.append(new UIToolkit::OriginCalloutIconTemplateContentLine(":/GUI/icon-friend.png", tr("ebisu_client_rooms_allow_you_to_chat_with_a_bunch")));
                // Will be parented to MainWindow and will be killed with it.
                MainFlow::instance()->featureCalloutController()->showCallout(settingName, tr("ebisu_client_new_title"), tr("ebisu_client_chat_rooms_title"), lines);
#if ENABLE_VOICE_CHAT
                }
#endif
            }
        }


        void ClientViewController::onSettingChange(const QString& settingKey, const Origin::Services::Variant& val)
        {
            if (settingKey == Services::SETTING_CALLOUT_SOCIALUSERAREA_SHOWN.name())
            {
                if(!val)
                {
                    showGroupChatCallout(true);
                }
            }
        }


        void ClientViewController::updateEnvronmentLabel()
        {
            const bool isBeta = Origin::Services::PlatformService::isBeta();
            QString envirBetaStr = "";
            // If Beta add "Beta"
            if(isBeta)
                envirBetaStr += tr("ebisu_client_beta");
            // If we have an EACore.ini - write environment name
            if(Services::readSetting(Services::SETTING_OverridesEnabled).toQVariant().toBool())
            {
                // emabrgo mode (R&D mode) is not disabled, so show an indicator
                if (!Services::readSetting(Services::SETTING_DisableEmbargoMode))
                {
                    envirBetaStr += " [R&D mode] ";
                }

                // If it's Beta add " - "
                envirBetaStr += isBeta ? " - " : "";
                envirBetaStr += Services::readSetting(Services::SETTING_EnvironmentName).toString();

                // If we're in production but we failed to configure, annotate with a star
                if (Services::readSetting(Services::SETTING_EnvironmentName).toString().compare(Services::env::production) == 0 && !Services::OriginConfigService::instance()->configurationSuccessful())
                {
                    envirBetaStr += "*";
                }
            }

            if (mMainWindow)
                mMainWindow->setEnvironmentLabel(envirBetaStr);
        }

        void ClientViewController::showAchievementNotification(Origin::Engine::Achievements::AchievementSetRef set, Origin::Engine::Achievements::AchievementRef achievement)
        {
            if(!Origin::Engine::Achievements::AchievementManager::instance()->enabled())
                return;

            // Only show notifications for achievement sets that have entitlements associated with them.
            if(set->entitlement().isNull())
                return;

            ClientFlow* clientFlow = ClientFlow::instance();
            if (!clientFlow)
            {
                ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
                return;
            }
            OriginToastManager* otm = clientFlow->originToastManager();
            if (!otm)
            {
                ORIGIN_LOG_ERROR<<"OriginToastManager not available";
                return;
            }

            UIToolkit::OriginNotificationDialog* toast = otm->showAchievementToast("POPUP_ACHIEVEMENT", QString("<b>%1</b>").arg(tr("ebisu_client_achievement_unlocked")), achievement->name(),  achievement->xp(), achievement->getImageUrl(40), true);
            if (toast)
                ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(clicked()), this, SLOT(showAchievementsHome()), Qt::QueuedConnection);
        }

        void ClientViewController::show(ClientViewWindowShowState showState)
        {
            mMainWindow->setAttribute(Qt::WA_ShowWithoutActivating);
            mMainWindow->show();
            if (showState == ClientWindow_SHOW_MINIMIZED)
            {
                mClientView->showGlobalProgress(ClientWindow_SHOW_MINIMIZED);
                mMainWindow->showMinimized ();
            }
            else if (showState == ClientWindow_SHOW_NORMAL)
            {
                mClientView->showGlobalProgress(ClientWindow_SHOW_NORMAL_IF_CREATED);
#if defined(ORIGIN_PC)
                mMainWindow->manager() && mMainWindow->manager()->isManagedMaximized() ? mMainWindow->showNormal() : mMainWindow->showWindow();
#else
                //when we are maximized on Mac, and we want to unmaximize we want to call "showZoomed" which is a toggle
                if(mMainWindow->manager()->isManagedZoomed())
                    mMainWindow->showZoomed();
                else
                    mMainWindow->showWindow();
#endif

            }
            else if (showState == ClientWindow_SHOW_MAXIMIZED)
            {
                mClientView->showGlobalProgress(ClientWindow_SHOW_NORMAL_IF_CREATED);
#if defined(ORIGIN_PC)
                mMainWindow->showMaximized();
#else
                //on Mac, "maximizing" is actually zooming
                if(!mMainWindow->manager()->isManagedZoomed())
                    mMainWindow->showZoomed();
#endif
            }
            if (showState == ClientWindow_SHOW_MINIMIZED_SYSTEMTRAY)
            {
                minimizeClientToTray();
            }
            mMainWindow->setAttribute(Qt::WA_ShowWithoutActivating, false);
        }

        void ClientViewController::minimizeClientToTray()
        {
            onMinimizeToTray();
        }

        void ClientViewController::onMinimizeToTray()
        {
#if !defined(ORIGIN_MAC)
            using namespace Origin::Services;
            const int time = readSetting(SETTING_TIMESHIDENOTIFICATION, Session::SessionService::currentSession());
            if (time > 0)
            {
                writeSetting(SETTING_TIMESHIDENOTIFICATION, 0, Session::SessionService::currentSession());
                EbisuSystemTray::instance()->showMessage(tr("ebisu_client_hide_notification_title").arg(tr("application_name")),
                    tr("ebisu_client_hide_notification_message"), QSystemTrayIcon::MessageIcon(QSystemTrayIcon::Information),
                    20000);
            }
            else
            {
                // a hack to not let windows show it's default ballon message once the app goes into systray mode
                EbisuSystemTray::instance()->showMessage("", "", QSystemTrayIcon::MessageIcon(QSystemTrayIcon::Information), 0);
            }
#endif
            mMainWindow->hide();
        }

        void ClientViewController::onZoomed()
        {
            mMainWindow->setGeometry(QApplication::desktop()->availableGeometry(mMainWindow));
        }

        void ClientViewController::entitlementsLoaded()
        {
            //  TELEMETRY:  Send entitlement counts on login
            int numGameEntitlements = Origin::Engine::Content::ContentController::currentUserContentController()->baseGamesController()->lastFullRefresh().isValid() ? 
                Origin::Engine::Content::ContentController::currentUserContentController()->baseGamesController()->numberOfGames() : -1;
            int numNonOriginEntitlements = Origin::Engine::Content::ContentController::currentUserContentController()->numberOfNonOriginEntitlements();
            QStringList nonOriginIds = Origin::Engine::Content::ContentController::currentUserContentController()->nonOriginEntitlementIds();
            GetTelemetryInterface()->Metric_ENTITLEMENT_GAME_COUNT_LOGIN(numGameEntitlements, numNonOriginEntitlements);
            GetTelemetryInterface()->Metric_NON_ORIGIN_GAME_COUNT_LOGIN(numNonOriginEntitlements);
            GetTelemetryInterface()->Metric_PERFORMANCE_TIMER_END(EbisuTelemetryAPI::LoginToMainPage,numGameEntitlements);
            for (int i = 0; i < nonOriginIds.length(); ++i)
            {
                GetTelemetryInterface()->Metric_NON_ORIGIN_GAME_ID_LOGIN(nonOriginIds.at(i).toUtf8());
            }
            QStringList favoriteSettings = Services::readSetting(Services::SETTING_FavoriteProductIds, Origin::Services::Session::SessionService::currentSession()).toStringList();
            int favCount = favoriteSettings.count();
            GetTelemetryInterface()->Metric_LOGIN_FAVORITES(favCount);

            QStringList hiddenSettings = Services::readSetting(Services::SETTING_HiddenProductIds, Origin::Services::Session::SessionService::currentSession()).toStringList();
            int hiddenCount = hiddenSettings.count();
            GetTelemetryInterface()->Metric_LOGIN_HIDDEN(hiddenCount);

            //only want to intialize the mygames view after login
            ORIGIN_VERIFY_DISCONNECT(Origin::Engine::Content::ContentController::currentUserContentController(), SIGNAL(entitlementsInitialized()), this, SLOT(entitlementsLoaded()));
            ORIGIN_VERIFY_DISCONNECT(Origin::Engine::Content::ContentController::currentUserContentController(), SIGNAL(updateFailed()), this, SLOT(entitlementsLoaded()));

            ClientSystemMenu* systemMenu = dynamic_cast<ClientSystemMenu*>(EbisuSystemTray::instance()->trayMenu());
            if (systemMenu)
            {
                systemMenu->updateRecentPlayedGames();
            }

			// must protect against user logging out which results in a null queueController
			Engine::Content::ContentOperationQueueController* queueController = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController();
			if (queueController)
				queueController->onInitialEntitlementLoad();

#ifdef ORIGIN_MAC
            // Show a Welcome message if Origin has opened for the first time
            showWelcomeMessage();
#endif
        }

        void ClientViewController::saveClientViewSizeAndPosition()
        {
            Services::writeSetting(Services::SETTING_APPSIZEPOSITION, Origin::Services::getFormatedWidgetSize(mMainWindow));
            Services::writeSetting(Services::SETTING_APPLOGOUTSCREENNUMBER, QApplication::desktop()->screenNumber(mMainWindow));
        }

        void ClientViewController::restoreClientViewSizeAndPosition()
        {
            const QString formatedSizePos = Services::readSetting(Origin::Services::SETTING_APPSIZEPOSITION);
            mMainWindow->setGeometry(Services::setFormatedWidgetSizePosition(formatedSizePos, QSize(PreferredWidth, PreferredHeight), true));
#if defined(ORIGIN_PC)
            if(QString(formatedSizePos.split("|").first()).contains("MAXIMIZED",Qt::CaseInsensitive))
            {
                mMainWindow->showMaximized();
            }
#endif
        }

        void ClientViewController::onExit()
        {
            emit requestExit();
        }

        void ClientViewController::onLogout()
        {
            emit requestLogout();
        }


        void ClientViewController::onTabChanged(NavigationItem previousTab, NavigationItem newTab)
        {
            // Update the accessibility string
            QString contextString = "";
            QString init;
            if(previousTab == NO_TAB)
                init = "true";
            else
                init = "false";

            switch (newTab)
            {
                case MY_GAMES_TAB:
                    GetTelemetryInterface()->Metric_GUI_TAB_VIEW("MyGames", init.toStdString().c_str());
                    contextString = QString(tr("ebisu_client_my_games_title"));
                    break;
                case STORE_TAB:
                    GetTelemetryInterface()->Metric_GUI_TAB_VIEW("Store", init.toStdString().c_str());
                    contextString = QString(tr("ebisu_client_store_title"));
                    break;
                case MY_ORIGIN_TAB:
                    GetTelemetryInterface()->Metric_GUI_TAB_VIEW("OriginNow", init.toStdString().c_str());
                    contextString = QString(tr("home_title"));
                    break;
                case SOCIAL_TAB:
                    GetTelemetryInterface()->Metric_GUI_TAB_VIEW("Social", init.toStdString().c_str());
                    break;
                case ACHIEVEMENT_TAB:
                    GetTelemetryInterface()->Metric_GUI_TAB_VIEW("Achievement", init.toStdString().c_str());
                    break;
                case SETTINGS_ACCOUNT_TAB:
                    GetTelemetryInterface()->Metric_GUI_TAB_VIEW("Account", init.toStdString().c_str());
                    break;
                case SETTINGS_APPLICATION_TAB:
                    GetTelemetryInterface()->Metric_GUI_TAB_VIEW("Application", init.toStdString().c_str());
                    break;

                default:
                    // Do nothing here
                    break;
            }
            updateAccessibleName(contextString);
        }

        void ClientViewController::onShowMyGames(bool recordNavigation /* = true */)
        {
            // Adding this here helps ensure that calling showMyGames gives the client focus
            // If this causes any issues we need to make bringUpWindow() public
            bringUpWindow();
            loadTab(MY_GAMES_TAB);
            NavigationItem previousTab = mCurrentTab;

            if (recordNavigation && NO_TAB != mCurrentTab) // Record the navigation only if we have been elsewhere already.
            {
                NavigationController::instance()->recordNavigation(MY_GAMES_TAB);
            }

            mCurrentTab = MY_GAMES_TAB;


            if(previousTab != mCurrentTab)
            {
                emit tabChanged(previousTab, mCurrentTab);
            }
        }

        void ClientViewController::onShowStore(bool recordNavigation /* = true */)
        {
            NavigationItem previousTab = mCurrentTab;
            
            // show the top page if we are already on the store tab or if we are showing store for the first time
            // show the page that was previously left by the user if we are coming to store tab from elsewhare
            if((mCurrentTab == NO_TAB) || (mCurrentTab == STORE_TAB) || !storeLoaded)
            {
                loadTab(STORE_TAB);
            }

            if (recordNavigation && NO_TAB != mCurrentTab) // Record the navigation only if we have been elsewhere already.
                NavigationController::instance()->recordNavigation(STORE_TAB);

            mCurrentTab = STORE_TAB;

            if(previousTab != mCurrentTab)
            {
                emit tabChanged(previousTab, mCurrentTab);
            }
            storeLoaded = true;
        }

        void ClientViewController::onShowSocial(bool recordNavigation /* = true */)
        {
            if (recordNavigation && NO_TAB != mCurrentTab) // Record the navigation only if we have been elsewhere already.
                NavigationController::instance()->recordNavigation(SOCIAL_TAB);

            mCurrentTab = SOCIAL_TAB;
            mNavBarViewController->clearTabs();
            mClientView->showSocial();
        }

        void ClientViewController::onShowSearchPage(bool recordNavigation /* = true */)
        {
            if (recordNavigation && NO_TAB != mCurrentTab) // Record the navigation only if we have been elsewhere already.
                NavigationController::instance()->recordNavigation(SEARCH_CONTENT);

            mCurrentTab = SOCIAL_TAB;
            mNavBarViewController->clearTabs();
            mClientView->showSearchPage();
        }

        void ClientViewController::onShowAchievementsHome(bool recordNavigation)
        {
            bringUpWindow();
            mNavBarViewController->clearTabs();
            mClientView->showAchievementsHome(recordNavigation);

            mCurrentTab = ACHIEVEMENT_TAB;

        }

        void ClientViewController::onShowAchievements(bool recordNavigation)
        {
            bringUpWindow();
            mNavBarViewController->clearTabs();
            mClientView->showAchievements(recordNavigation);
            mCurrentTab = ACHIEVEMENT_TAB;
        }

        void ClientViewController::onShowSettingsAccount(bool recordNavigation /* = true */)
        {
            ORIGIN_LOG_EVENT << "Showing settings (account tab).";

            bringUpWindow();

            mCurrentTab = SETTINGS_ACCOUNT_TAB;
            loadTab(mCurrentTab);

            if (recordNavigation)  
                NavigationController::instance()->recordNavigation(SETTINGS_ACCOUNT_TAB);
        }

        void ClientViewController::onShowSettingsAccountSubscription(bool recordNavigation /* = true */)
        {
            ORIGIN_LOG_EVENT << "Showing settings (account tab - subscriptions).";

            bringUpWindow();

            mCurrentTab = SETTINGS_ACCOUNT_TAB;

//TODO FIXUP_ORIGINX
#if 0 
            mClientMessageAreaManager->removeMessage(MessageAreaViewControllerBase::Subscription);
            mNavBarViewController->clearTabs();
            mClientView->showAccountProfileSubscription();
#endif

            if (recordNavigation)  
                NavigationController::instance()->recordNavigation(SETTINGS_ACCOUNT_TAB);
        }

        void ClientViewController::onShowSettingsApplication(bool recordNavigation /* = true */)
        {
            ORIGIN_LOG_EVENT << "Showing settings (application tab).";
            bringUpWindow();

            mCurrentTab = SETTINGS_APPLICATION_TAB;
            loadTab(mCurrentTab);

            if (recordNavigation)  
                NavigationController::instance()->recordNavigation(SETTINGS_APPLICATION_TAB);
        }

        void ClientViewController::onShowFeedbackPage(bool recordNavigation /* = true */)
        {
            mCurrentTab = FEEDBACK_TAB;

            // If the client is minimized or hidden (in the system tray): 
            // Use consistent behavior of when we show the window from the system tray
            if(mMainWindow->isMinimized() || mMainWindow->isVisible() == false)
                EbisuSystemTray::instance()->showPrimaryWindow();

            // If we got here from clicking on the XP in the buddy list while there was a 
            // window stacked on top of Origin, raise Origin to the top of the screen
            if(mMainWindow)
                mMainWindow->activateWindow();

            mNavBarViewController->clearTabs();
            // Get users authToken
            QString accessToken = Origin::Services::Session::SessionService::accessToken(Origin::Services::Session::SessionService::currentSession());
            // Get users locale
            QString shortLocale("us");
            QString currentLocale = OriginApplication::instance().locale();

            if (currentLocale.compare("fr_FR", Qt::CaseInsensitive) == 0)
            {
                shortLocale = "fr";               
            }
            QString feedbackUrl = Origin::Services::readSetting(Origin::Services::SETTING_FeedbackPageURL).toString();
            feedbackUrl = feedbackUrl.arg(shortLocale);
            mClientView->showFeedbackPage((feedbackUrl + "?cem=" + accessToken), ClientScope);

            if (recordNavigation)  
                NavigationController::instance()->recordNavigation(FEEDBACK_TAB);
        }

        void ClientViewController::onNavbarClickedShowFriendsList()
        {
            //  TELEMETRY:  User viewed their profile by clicking in main.
            GetTelemetryInterface()->Metric_FRIEND_VIEWSOURCE("main");

            showFriendsList();
        }

        void ClientViewController::showUrlInStore(const QUrl& url)
        {
            if(mNavBarViewController)
                mNavBarViewController->updateStoreTab();
            mClientView->showStore(url);
        }

        void ClientViewController::showProductInStore(const QString& productID, const QString& masterTitleID)
        {
            if(mNavBarViewController)
                mNavBarViewController->updateStoreTab();
            mClientView->showStoreProduct(productID, masterTitleID);
        }

        void ClientViewController::showProductIDInStore(const QString& productID)
        {
            // New store only understands master title ID based links, so we do a product definition lookup.
            Origin::Engine::Content::CatalogDefinitionQuery* query = Origin::Engine::Content::CatalogDefinitionController::instance()->queryCatalogDefinition(productID);
            ORIGIN_VERIFY_CONNECT(query, SIGNAL(finished()), this, SLOT(onProductInfoLookupCompleteForStore()));
        }

        void ClientViewController::onProductInfoLookupCompleteForStore()
        {
            Origin::Engine::Content::CatalogDefinitionQuery* query = dynamic_cast<Origin::Engine::Content::CatalogDefinitionQuery*>(sender());

            if(query)
            {
                if(query->definition() != Origin::Services::Publishing::CatalogDefinition::UNDEFINED)
                {
                    if(Engine::IGOController::instance()->isActive())
                    {
                        QUrl newUrl = mClientView->storeViewController()->storeUrlBuilder()->productUrl(query->definition()->productId(), query->definition()->masterTitleId());
                        emit Engine::IGOController::instance()->showBrowser(newUrl.toString(), true);
                    }
                    else
                    {
                        loadTab(STORE_TAB);
                        mClientView->showStoreProduct(query->definition()->productId(), query->definition()->masterTitleId());
                        mMainWindow->showWindow();
                        mMainWindow->activateWindow();
                    }
                }

                query->deleteLater();
            }
        }
        
        void ClientViewController::showMasterTitleInStore(const QString& masterTitleID)
        {
            if(Engine::IGOController::instance()->isActive())
            {
                QUrl newUrl = mClientView->storeViewController()->storeUrlBuilder()->productUrl("", masterTitleID);
                
                emit Engine::IGOController::instance()->showBrowser(newUrl.toString(), true);
            }
            else
            {
                if(mNavBarViewController)
                    mNavBarViewController->updateStoreTab();
                mClientView->showStoreProduct("", masterTitleID);
                mMainWindow->showWindow();
                mMainWindow->activateWindow();
            }
        }

        void ClientViewController::showStoreFreeGames()
        {
            if(mMainWindow->isVisible() == false)
                mMainWindow->showWindow();
            if(mNavBarViewController)
                mNavBarViewController->updateStoreTab();
            mClientView->showStoreFreeGames();
            updateStoreTab();
        }

        void ClientViewController::showStoreOnTheHouse(const QString& trackingParam)
        {
            if(mMainWindow->isVisible() == false)
                mMainWindow->showWindow();
            if(mNavBarViewController)
                mNavBarViewController->updateStoreTab();
            mClientView->showStoreOnTheHouse(trackingParam);
            updateStoreTab();
        }

        void ClientViewController::showStoreHome()
        {
            if(mMainWindow->isVisible() == false)
                mMainWindow->showWindow();
            if(mNavBarViewController)
                mNavBarViewController->updateStoreTab();
            loadTab(STORE_TAB);
            updateStoreTab();
        }

        void ClientViewController::showMyGames()
        {
            if(mMainWindow->isVisible() == false)
                mMainWindow->showWindow();
            onShowMyGames();
            updateMyGamesTab();
        }

        void ClientViewController::showAchievementsHome()
        {
            if(Engine::IGOController::instance()->isActive())
            {
                Engine::IGOController::instance()->EbisuShowAchievementsUI(0, 0, "", true, Engine::IIGOCommandController::CallOrigin_CLIENT);
            }
            else
            {
                onShowAchievementsHome();
            }
            NavigationController::instance()->achievementHomeChosen();
        }

        void ClientViewController::showAchievementSetDetails(const QString& achievementSetId, const QString& userId, const QString& gameTitle)
        {
            if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
            {
                Engine::IGOController::instance()->EbisuShowAchievementsUI(userId.toULongLong(), 0, achievementSetId, true, Engine::IIGOCommandController::CallOrigin_CLIENT);
            }
            else
            {
                mCurrentTab = ACHIEVEMENT_TAB;

                bringUpWindow();

                if(mNavBarViewController)
                    mNavBarViewController->clearTabs();
                mClientView->showAchievementSetDetails(achievementSetId, userId, gameTitle);
            }
            
        }

        void ClientViewController::showFeedbackPage()
        {
            onShowFeedbackPage();
        }

        void ClientViewController::showAvatarSelect()
        {
            UIScope scope = ClientScope;
            if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
                scope = IGOScope;
            showAvatarSelect(scope, Engine::IIGOCommandController::CallOrigin_CLIENT);
        }

        void ClientViewController::showAvatarSelect(UIScope scope, Engine::IIGOCommandController::CallOrigin callOrigin)
        {
            if (!mChangeAvatarViewController)
                mChangeAvatarViewController = new ChangeAvatarViewController(this);

            mChangeAvatarViewController->show(scope, callOrigin);
        }

        void ClientViewController::showMyProfile(UIScope scope, ShowProfileSource source, Engine::IIGOCommandController::CallOrigin callOrigin)
        {
            using namespace ::Origin::Services;
            showProfile(Session::SessionService::nucleusUser(Session::SessionService::currentSession()).toULongLong(), scope, source, callOrigin);
        }

        void ClientViewController::showMyProfile(Engine::IIGOCommandController::CallOrigin callOrigin)
        {
            UIScope scope = ClientScope;
            if (Engine::IGOController::instance()->isActive())
                scope = IGOScope;
            using namespace ::Origin::Services;
            showProfile(Session::SessionService::nucleusUser(Session::SessionService::currentSession()).toULongLong(), scope, ShowProfile_MainWindow, callOrigin);
        }

        void ClientViewController::showProfile(quint64 nucleusId, UIScope scope, ShowProfileSource source, Engine::IIGOCommandController::CallOrigin callOrigin)
        {
            using namespace ::Origin::Services;
#ifdef ORIGIN_MAC
            if (scope != IGOScope)
            {
#endif
                bringUpWindow();
#ifdef ORIGIN_MAC
            }
#endif
            // Don't check for visibility of IGO if using scope, it's explicit
            if ((Engine::IGOController::instance()->isActive() || scope == IGOScope))
            {
                Engine::IGOController::instance()->igoShowProfile(nucleusId, callOrigin);
            }
            else
            {
                if(mNavBarViewController)
                    mNavBarViewController->clearTabs();
                mClientView->showProfilePage(nucleusId);
            }

            QString src = profileSourceToString(source);
            QString dst = "NON_FRIENDS_VIEW";
            Origin::Engine::UserRef user = Origin::Engine::LoginController::instance()->currentUser();
            if(nucleusId == (quint64) user->userId())
                dst = "MY_VIEW";
            else
            {
                Chat::Connection *connection = user->socialControllerInstance()->chatConnection();
                Origin::Chat::Roster *r = connection->currentUser()->roster();

                QSet<Origin::Chat::RemoteUser*> friendsSet = r->contacts();
                for(QSet<Origin::Chat::RemoteUser*>::iterator it = friendsSet.begin(); it != friendsSet.end(); it++)
                {
                    Origin::Chat::RemoteUser* aUser = *(it);
                    if(aUser->nucleusId() == nucleusId)
                    {
                        dst = "FRIENDS_VIEW";
                        break;
                    }
                }
            }

            switch (scope)
            {
            case ClientScope:
                GetTelemetryInterface()->Metric_USER_PROFILE_VIEW("Client", src.toLocal8Bit().data(), dst.toLocal8Bit().data());
                break;
            case IGOScope:
                GetTelemetryInterface()->Metric_USER_PROFILE_VIEW("IGO", src.toLocal8Bit().data(), dst.toLocal8Bit().data());
                break;
            default:
                GetTelemetryInterface()->Metric_USER_PROFILE_VIEW("unknown", src.toLocal8Bit().data(), dst.toLocal8Bit().data());
                break;
            }
        }

        void ClientViewController::showProfile(quint64 nucleusId)
        {
            UIScope scope = ClientScope;
            if (Engine::IGOController::instance()->isActive())
                scope = IGOScope;
            showProfile(nucleusId, scope, ShowProfile_MainWindow, Engine::IIGOCommandController::CallOrigin_CLIENT);
        }
            
        void ClientViewController::showProfileSearchResult(const QString &search, UIScope scope)
        {
            if (Engine::IGOController::instance()->isActive())
            {
                mFriendSearchViewController->closeIGOWindow();
                Engine::IGOController::instance()->igoShowSearch(search, Engine::IIGOCommandController::CallOrigin_CLIENT);
            }
            else
            {
                mFriendSearchViewController->closeWindow();
                mClientView->search(search);
            }
            if (scope != IGOScope)
            {
                bringUpWindow();
            }
        }

        void ClientViewController::updateMyGamesTab()
        {
            if(mNavBarViewController)
                mNavBarViewController->updateMyGamesTab();
        }

        void ClientViewController::updateStoreTab()
        {
            if(mNavBarViewController)
                mNavBarViewController->updateStoreTab();
        }

        void ClientViewController::recordSocialNavigation(const QString& url)
        {
            NavigationController::instance()->recordNavigation(SOCIAL_TAB, url);
        }

        void ClientViewController::recordFeedbackNavigation(const QString& url)
        {
            NavigationController::instance()->recordNavigation(FEEDBACK_TAB, url);
        }

        void ClientViewController::setInSocial(const QString& nucleusID)
        {
            mClientView->setInSocial(nucleusID);
            mCurrentTab = SOCIAL_TAB;

            Engine::UserRef user = mUser.toStrongRef();
            
            if (!user)
            {
                return;
            }

            // Try to look up their Origin ID
            // If we're friends with the user their Origin ID should be cached
            Engine::Social::UserNamesCache *userNames = user->socialControllerInstance()->userNames();
            const QString originId = userNames->namesForNucleusId(nucleusID.toULongLong()).originId();

            if (!originId.isNull())
            {
                // Update the accessibility string
                QString contextString = QString(tr("ebisu_client_user_profile_title")).arg(originId);
                this->updateAccessibleName(contextString);
            }
        }

        void ClientViewController::showFriendSearchDialog()
        {
            UIScope scope = ClientScope;
            if (Engine::IGOController::instance()->isActive())
                scope = IGOScope;
            showFriendSearchDialog(scope, Engine::IIGOCommandController::CallOrigin_CLIENT);
        }

        void ClientViewController::showDownloadProgressDialog()
        {
            mClientView->showGlobalProgress();
        }

        void ClientViewController::showFriendSearchDialog(UIScope scope, Engine::IIGOCommandController::CallOrigin callOrigin)
        {
            if (!mFriendSearchViewController)
                mFriendSearchViewController = new FriendSearchViewController(this);
            mFriendSearchViewController->show(scope, callOrigin);
        }

        void ClientViewController::showCreateGroupDialog()
        {
            UIScope scope = ClientScope;
            if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
                scope = IGOScope;
            showCreateGroupDialog(scope);
        }

        void ClientViewController::showCreateGroupDialog(UIScope scope)
        {
            if (!mCreateGroupViewController) {
                mCreateGroupViewController = new CreateGroupViewController();
                ORIGIN_VERIFY_CONNECT(mCreateGroupViewController, SIGNAL(createGroupSucceeded(const QString&)), this, SLOT(onCreateGroupSuccess(const QString&)));
            }
            mCreateGroupViewController->show(scope);
        }

        void ClientViewController::onCreateGroupSuccess(const QString& groupGuid)
        {
            UIScope scope = ClientScope;
            if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
                scope = IGOScope;

            Chat::Connection *connection = Engine::LoginController::instance()->currentUser()->socialControllerInstance()->chatConnection();
            Origin::Chat::Roster *r = connection->currentUser()->roster();

            if (r->hasFullySubscribedContacts())
            {
                this->showInviteFriendsToGroupDialog(groupGuid, scope);
            }
            else
            {
                this->showYouNeedFriendsDialog(scope);
            }
        }

        void ClientViewController::showEditGroupDialog(const QString& groupName, const QString& groupGuid)
        {
            UIScope scope = ClientScope;
            if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
                scope = IGOScope;
            showEditGroupDialog(groupName, groupGuid, scope);
        }

        void ClientViewController::showEditGroupDialog(const QString& groupName, const QString& groupGuid, UIScope scope)
        {
            EditGroupViewController* controller = mEditGroupViewControllers[groupGuid];
            if (!controller)
            {
                controller = new EditGroupViewController(groupName, groupGuid, this);
                mEditGroupViewControllers.insert(groupGuid, controller);
                ORIGIN_VERIFY_CONNECT(controller, SIGNAL(windowClosed()), this, SLOT(onCloseEditGroupDialog()));
            }
            controller->show(scope);
        }

        void ClientViewController::onCloseEditGroupDialog()
        {
            EditGroupViewController* controller = dynamic_cast<EditGroupViewController*>(sender());
            
            if (controller)
            {
                mEditGroupViewControllers.remove(controller->groupGuid());
                delete controller;
            }
        }

        void ClientViewController::showDeleteGroupDialog(const QString& groupName, const QString& groupGuid)
        {
            UIScope scope = ClientScope;
            if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
                scope = IGOScope;
            showDeleteGroupDialog(groupName, groupGuid, scope);
        }

        void ClientViewController::showDeleteGroupDialog(const QString& groupName, const QString& groupGuid, UIScope scope)
        {
            DeleteGroupViewController* controller = mDeleteGroupViewControllers[groupGuid];
            if (!controller)
            {
                controller = new DeleteGroupViewController(groupName, groupGuid, this);
                mDeleteGroupViewControllers.insert(groupGuid, controller);

                ORIGIN_VERIFY_CONNECT(controller, SIGNAL(groupDeleted(const QString&)), this, SLOT(onGroupDeleted(const QString&)));
                ORIGIN_VERIFY_CONNECT(controller, SIGNAL(groupDeleted(const QString&)), this, SIGNAL(groupDeleted(const QString&)));
                ORIGIN_VERIFY_CONNECT(controller, SIGNAL(windowClosed()), this, SLOT(onCloseDeleteGroupDialog()));
            }
            controller->show(scope);
        }

        void ClientViewController::onCloseDeleteGroupDialog()
        {
            DeleteGroupViewController* controller = dynamic_cast<DeleteGroupViewController*>(sender());

            if (controller)
            {
                mDeleteGroupViewControllers.remove(controller->groupGuid());
                delete controller;
            }
        }

        void ClientViewController::showLeaveGroupDialog(const QString& groupName, const QString& groupGuid)
        {
            UIScope scope = ClientScope;
            if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
                scope = IGOScope;
            showLeaveGroupDialog(groupName, groupGuid, scope);
        }

        void ClientViewController::showLeaveGroupDialog(const QString& groupName, const QString& groupGuid, UIScope scope)
        {
            LeaveGroupViewController* controller = mLeaveGroupViewControllers[groupGuid];
            if (!controller)
            {
                controller = new LeaveGroupViewController(groupName, groupGuid, this);
                mLeaveGroupViewControllers.insert(groupGuid, controller);

                ORIGIN_VERIFY_CONNECT(controller, SIGNAL(groupDeparted(const QString&)), this, SLOT(onDepartGroup(const QString&)));
                ORIGIN_VERIFY_CONNECT(controller, SIGNAL(windowClosed()), this, SLOT(onCloseLeaveGroupDialog()));
            }
            controller->show(scope);
        }

        void ClientViewController::onCloseLeaveGroupDialog()
        {
            LeaveGroupViewController* controller = dynamic_cast<LeaveGroupViewController*>(sender());

            if (controller)
            {
                mLeaveGroupViewControllers.remove(controller->groupGuid());
                delete controller;
            }
        }

        void ClientViewController::showDeleteRoomDialog(const QString& groupGuid, const QString& channelId, const QString& roomName)
        {
            UIScope scope = ClientScope;
            if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
                scope = IGOScope;
            showDeleteRoomDialog(groupGuid, channelId, roomName, scope);
        }

        void ClientViewController::showDeleteRoomDialog(const QString& groupGuid, const QString& channelId, const QString& roomName, UIScope scope)
        {
            DeleteRoomViewController* controller = mDeleteRoomViewControllers[channelId];
            if (!controller)
            {
                controller = new DeleteRoomViewController(groupGuid, channelId, roomName, this);
                mDeleteRoomViewControllers.insert(channelId, controller);

                ORIGIN_VERIFY_CONNECT(controller, SIGNAL(roomDeleted(const QString&)), this, SLOT(onRoomDeleted(const QString&)));
                ORIGIN_VERIFY_CONNECT(controller, SIGNAL(windowClosed()), this, SLOT(onCloseDeleteRoomDialog()));
            }
            controller->show(scope);
        }

        void ClientViewController::onCloseDeleteRoomDialog()
        {
            DeleteRoomViewController* controller = dynamic_cast<DeleteRoomViewController*>(sender());

            if (controller)
            {
                mDeleteRoomViewControllers.remove(controller->channelId());
                delete controller;
            }
        }

        void ClientViewController::showEnterRoomPasswordDialog(const QString& groupGuid, const QString& channelId, bool deleteRoom)
        {
            UIScope scope = ClientScope;
            if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
                scope = IGOScope;
            showEnterRoomPasswordDialog(groupGuid, channelId, scope, deleteRoom);
        }

        void ClientViewController::showEnterRoomPasswordDialog(const QString& groupGuid, const QString& channelId, UIScope scope, bool deleteRoom)
        {
            EnterRoomPasswordViewController* controller = mEnterRoomPasswordViewController[channelId];
            if (!controller)
            {
                controller = new EnterRoomPasswordViewController(groupGuid, channelId, deleteRoom, this);
                mEnterRoomPasswordViewController.insert(channelId, controller);
                ORIGIN_VERIFY_CONNECT(controller, SIGNAL(windowClosed()), this, SLOT(onCloseEnterRoomPasswordDialog()));
            }
            controller->show(scope);
        }

        void ClientViewController::onCloseEnterRoomPasswordDialog()
        {
            EnterRoomPasswordViewController* controller = dynamic_cast<EnterRoomPasswordViewController*>(sender());

            if (controller)
            {
                mEnterRoomPasswordViewController.remove(controller->channelId());
                delete controller;
            }
        }

        void ClientViewController::showCreateRoomDialog(const QString& groupName, const QString& groupGuid)
        {
            UIScope scope = ClientScope;
            if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
                scope = IGOScope;
            showCreateRoomDialog(groupName, groupGuid, scope);
        }

        void ClientViewController::showCreateRoomDialog(const QString& groupName, const QString& groupGuid, UIScope scope)
        {
            CreateRoomViewController* controller = mCreateRoomViewControllers[groupGuid];
            if (!controller)
            {
                controller = new CreateRoomViewController(groupName, groupGuid, this);
                mCreateRoomViewControllers.insert(groupGuid, controller);
            }
            controller->show(scope);
        }

        void ClientViewController::onCloseCreateRoomDialog()
        {
            CreateRoomViewController* controller = dynamic_cast<CreateRoomViewController*>(sender());

            if (controller)
            {
                mCreateRoomViewControllers.remove(controller->groupGuid());
                delete controller;
            }
        }

        void ClientViewController::showInviteFriendsToGroupDialog(const QString& groupGuid, UIScope scope)
        {
            GroupInviteViewController* controller = mGroupInviteViewControllers[groupGuid];
            if (!controller)
            {
                controller = new GroupInviteViewController(groupGuid, "", "", this->getWidgetParent(), scope);
                mGroupInviteViewControllers.insert(groupGuid, controller);
            }
            controller->show(scope);
        }

        void ClientViewController::showInviteFriendsToRoomDialog(const QString& groupGuid, const QString& channelId, const QString& conversationId, UIScope scope)
        {
            GroupInviteViewController* controller = mGroupInviteViewControllers[groupGuid + channelId + conversationId];
            if (!controller)
            {
                controller = new GroupInviteViewController(groupGuid, channelId, conversationId, this->getWidgetParent(), scope);
                mGroupInviteViewControllers.insert(groupGuid + channelId + conversationId, controller);

                if (Engine::Social::SocialController* socialController = Engine::LoginController::instance()->currentUser()->socialControllerInstance())
                {
                    ORIGIN_VERIFY_CONNECT(socialController, SIGNAL(kickedFromGroup(QString, QString, qint64)), controller, SLOT(onWindowClosed()));
                    ORIGIN_VERIFY_CONNECT(socialController, SIGNAL(groupDestroyed(QString, qint64, QString)), controller, SLOT(onWindowClosed()));

                    Chat::Connection* connection = socialController->chatConnection();
                    Chat::ConnectedUser* currentUser = connection->currentUser();
                    if (currentUser)
                    {
                        ORIGIN_VERIFY_CONNECT(currentUser, SIGNAL(userIsInvisible()), controller, SLOT(onWindowClosed()));
                        if (Chat::ChatGroups* groups = currentUser->chatGroups())
                        {
                            ORIGIN_VERIFY_CONNECT(groups, SIGNAL(groupRemoved(Origin::Chat::ChatGroup*)), controller, SLOT(onWindowClosed()));
                        }
                    }
                }
            }
            controller->show(scope);
        }

        void ClientViewController::showYouNeedFriendsDialog(UIScope scope)
        {
            QString header = tr("ebisu_client_add_some_friends_first");
            UIToolkit::OriginWindow* window = 
                UIToolkit::OriginWindow::promptNonModal(UIToolkit::OriginMsgBox::NoIcon, header, 
                tr("ebisu_client_chat_groups_are_for_you_and_your_friends_to_text"),
                tr("ebisu_client_add_friend_title"),
                tr("ebisu_client_cancel"), QDialogButtonBox::Yes);

            window->setTitleBarText(tr("ebisu_client_group_invite_friends"));

            ORIGIN_VERIFY_CONNECT(window, SIGNAL(accepted()), this, SLOT(showFriendSearchDialog()));
            ORIGIN_VERIFY_CONNECT(window, SIGNAL(rejected()), window, SLOT(close()));

            if (scope==IGOScope)
            {
                window->configForOIG(true);
                Engine::IGOController::instance()->igowm()->addPopupWindow(window, Engine::IIGOWindowManager::WindowProperties());
            }
            else
            {
                window->showWindow();
            }
        }

        void ClientViewController::showAllFriendsInGroupDialog(UIScope scope)
        {
            UIToolkit::OriginWindow* window = 
                UIToolkit::OriginWindow::promptNonModal(UIToolkit::OriginMsgBox::NoIcon, tr("ebisu_client_all_of_your_friends_are_in_this_group"), 
                tr("ebisu_client_you_can_only_invite_your_friends_who_are_not_already"),
                tr("ebisu_client_add_friend_title"),
                tr("ebisu_client_cancel"));

            window->setTitleBarText(tr("ebisu_client_group_invite_friends"));

            ORIGIN_VERIFY_CONNECT(window, SIGNAL(accepted()), this, SLOT(showFriendSearchDialog()));
            ORIGIN_VERIFY_CONNECT(window, SIGNAL(rejected()), window, SLOT(close()));

            if (scope==IGOScope)
            {
                window->configForOIG(true);
                Engine::IGOController::instance()->igowm()->addPopupWindow(window, Engine::IIGOWindowManager::WindowProperties());
            }
            else
            {
                window->showWindow();
            }

        }


        void ClientViewController::showGroupMembersDialog(const QString& groupName, const QString& groupGuid)
        {
            UIScope scope = ClientScope;
            if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
                scope = IGOScope;
            showGroupMembersDialog(groupName, groupGuid, scope);
        }

        void ClientViewController::showGroupMembersDialog(const QString& groupName, const QString& groupGuid, UIScope scope)
        {
            GroupMembersViewController* controller = mGroupMembersViewControllers[groupGuid];
            if (!controller)
            {
                controller = new GroupMembersViewController(groupGuid, this->getWidgetParent(), scope);
                mGroupMembersViewControllers.insert(groupGuid, controller);
            }

            controller->show(scope);
        }

        void ClientViewController::showGroupBannedMembersDialog(const QString& groupName, const QString& groupGuid)
        {
            UIScope scope = ClientScope;
            if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
                scope = IGOScope;
            showGroupBannedMembersDialog(groupName, groupGuid, scope);
        }

        void ClientViewController::showGroupBannedMembersDialog(const QString& groupName, const QString& groupGuid, UIScope scope)
        {

        }

        void ClientViewController::showRemoveGroupUserDialog(const QString& groupGuid, const QString& userId, const QString& userNickname)
        {
            UIScope scope = ClientScope;
            if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
                scope = IGOScope;
            showRemoveGroupUserDialog(groupGuid, userId, userNickname, scope);
        }

        void ClientViewController::showRemoveGroupUserDialog(const QString& groupGuid, const QString& userId, const QString& userNickname, UIScope scope)
        {
            RemoveGroupUserViewController* controller = mRemoveGroupUserViewControllers[groupGuid];

            if (!controller)
            {
                controller = new RemoveGroupUserViewController(groupGuid, userId, userNickname);
                mRemoveGroupUserViewControllers.insert(groupGuid, controller);
            }
            else
            {
                // Need to set the proper user up if we are re-using the controller
                controller->setUserId(userId);
                controller->setUserNickname(userNickname);
            }
            controller->show(scope);
        }

        void ClientViewController::showPromoteToAdminSuccessDialog(const QString& groupName, const QString& userNickname, UIScope scope)
        {
            QString header = tr("ebisu_client_is_now_an_admin_of").arg(userNickname).arg(groupName);
            UIToolkit::OriginWindow* window = 
                UIToolkit::OriginWindow::alertNonModal(UIToolkit::OriginMsgBox::Success, header, 
                QString(""),
                tr("ebisu_client_close"));
            if(window->msgBox() && window->msgBox()->getTitleLabel())
            {
                window->msgBox()->getTitleLabel()->setMaxNumLines(2);
                window->msgBox()->getTitleLabel()->setElideMode(Qt::ElideRight);
            }

            window->setTitleBarText(tr("ebisu_client_promote_to_admin"));

            if (scope==IGOScope)
            {
                window->configForOIG(true);
                Engine::IGOController::instance()->igowm()->addPopupWindow(window, Engine::IIGOWindowManager::WindowProperties());
            }
            else
            {
                window->showWindow();
            }
        }

        void ClientViewController::showPromoteToAdminFailureDialog(const QString& userNickname, UIScope scope)
        {
            QString description = tr("ebisu_client_username_is_no_longer_available").arg(userNickname);
            UIToolkit::OriginWindow* window = 
                UIToolkit::OriginWindow::alertNonModal(UIToolkit::OriginMsgBox::Error, tr("ebisu_client_unable_to_promote_member_to_admin"), 
                description,
                tr("ebisu_client_close"));

            window->setTitleBarText(tr("ebisu_client_promote_to_admin"));

            if (scope==IGOScope)
            {
                window->configForOIG(true);
                Engine::IGOController::instance()->igowm()->addPopupWindow(window, Engine::IIGOWindowManager::WindowProperties());
            }
            else
            {
                window->showWindow();
            }
        }


        void ClientViewController::showRoomIsFullDialog(UIScope scope)
        {
            UIToolkit::OriginWindow* window = 
                UIToolkit::OriginWindow::alertNonModal(UIToolkit::OriginMsgBox::Error, 
                    tr("ebisu_client_this_group_is_full_title"), 
                    tr("ebisu_client_this_room_has_reached_its_limit_of_text"),
                    tr("ebisu_client_close"));

            window->setTitleBarText(tr("application_name"));

            if (scope==IGOScope)
            {
                window->configForOIG(true);
                Engine::IGOController::instance()->igowm()->addPopupWindow(window, Engine::IIGOWindowManager::WindowProperties());
            }
            else
            {
                window->showWindow();
            }
        }

        void ClientViewController::showDemoteToMemberSuccessDialog(const QString& groupName, const QString& userNickname, UIScope scope)
        {
            QString header = tr("ebisu_client_is_no_longer_an_admin_of").arg(userNickname).arg(groupName);
            UIToolkit::OriginWindow* window = 
                UIToolkit::OriginWindow::alertNonModal(UIToolkit::OriginMsgBox::Success, header, 
                QString(""),
                tr("ebisu_client_close"));
            if(window->msgBox() && window->msgBox()->getTitleLabel())
            {
                window->msgBox()->getTitleLabel()->setMaxNumLines(2);
                window->msgBox()->getTitleLabel()->setElideMode(Qt::ElideRight);
            }
            window->setTitleBarText(tr("ebisu_client_demote_admin_to_member"));

            if (scope==IGOScope)
            {
                window->configForOIG(true);
                Engine::IGOController::instance()->igowm()->addPopupWindow(window, Engine::IIGOWindowManager::WindowProperties());
            }
            else
            {
                window->showWindow();
            }
        }

        void ClientViewController::showTransferOwnershipConfirmationDialog(const QString& groupGuid, Origin::Chat::RemoteUser* user, const QString& userNickname, const UIScope scope)
        {
            TransferOwnerConfirmViewController* controller = mTransferOwnerConfirmViewControllers[groupGuid];

            if (!controller)
            {
                controller = new TransferOwnerConfirmViewController(groupGuid, user, userNickname);
                mTransferOwnerConfirmViewControllers.insert(groupGuid, controller);
            }
            else 
            {
                controller->setUser(user, userNickname);
            }

            controller->show(scope);
        }

        void ClientViewController::showGroupInviteSentDialog(QList<Origin::Chat::RemoteUser*> users, UIScope scope)
        {
            QString heading;
            QString description;
            UIToolkit::OriginMsgBox::IconType icon = UIToolkit::OriginMsgBox::Success;

            if (users.length()==1) 
            {
                heading = tr("ebisu_client_invitations_sent_title");
                description = tr("ebisu_client_invitation_sent_to_X").arg(users[0]->originId());
            }
            else if (users.length()==0)
            {
                heading = tr("ebisu_client_unable_to_invite_friend");
                description = tr("ebisu_client_invitations_sent_to_X_friends").arg(users.length());
                icon = UIToolkit::OriginMsgBox::Error;
            }
            else
            {
                heading = tr("ebisu_client_invitations_sent_title");
                description = tr("ebisu_client_invitations_sent_to_X_friends").arg(users.length());
            }

            UIToolkit::OriginWindow* window = 
                UIToolkit::OriginWindow::alertNonModal(icon, heading, description, tr("ebisu_client_close"));

            window->setTitleBarText(tr("ebisu_client_group_invite_friends"));

            if (Engine::IGOController::instance()->isActive())
            {
                window->configForOIG(true);
                Engine::IGOController::instance()->igowm()->addPopupWindow(window, Engine::IIGOWindowManager::WindowProperties());
            }
            else
            {
                window->showWindow();
            }
        }

        void ClientViewController::showTransferOwnershipSuccessDialog(const QString& groupName, const QString& userNickname, UIScope scope)
        {
            QString header = tr("ebisu_client_is_the_new_owner_of").arg(userNickname).arg(groupName);
            UIToolkit::OriginWindow* window = 
                UIToolkit::OriginWindow::alertNonModal(UIToolkit::OriginMsgBox::Success, header, 
                QString(""),
                tr("ebisu_client_close"));
            if(window->msgBox() && window->msgBox()->getTitleLabel())
            {
                window->msgBox()->getTitleLabel()->setMaxNumLines(2);
                window->msgBox()->getTitleLabel()->setElideMode(Qt::ElideRight);
            }

            window->setTitleBarText(tr("ebisu_client_transfer_ownership"));

            if (scope==IGOScope)
            {
                window->configForOIG(true);
                Engine::IGOController::instance()->igowm()->addPopupWindow(window, Engine::IIGOWindowManager::WindowProperties());
            }
            else
            {
                window->showWindow();
            }
        }

        void ClientViewController::showTransferOwnershipFailureDialog(const QString& userNickname, UIScope scope)
        {
            QString description = tr("ebisu_client_is_no_longer_a_member_of_this_group").arg(userNickname);
            UIToolkit::OriginWindow* window = 
                UIToolkit::OriginWindow::alertNonModal(UIToolkit::OriginMsgBox::Error, tr("ebisu_client_unable_to_transfer_ownership"), 
                description,
                tr("ebisu_client_close"));

            window->setTitleBarText(tr("ebisu_client_transfer_ownership"));

            if (scope==IGOScope)
            {
                window->configForOIG(true);
                Engine::IGOController::instance()->igowm()->addPopupWindow(window, Engine::IIGOWindowManager::WindowProperties());
            }
            else
            {
                window->showWindow();
            }
        }

        void ClientViewController::showRemoveRoomUserDialog(const QString& groupGuid, const QString& channelId, const QString& userNickname)
        {
            UIScope scope = ClientScope;
            if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
                scope = IGOScope;
            showRemoveRoomUserDialog(groupGuid, channelId, userNickname, scope);
        }

        void ClientViewController::showRemoveRoomUserDialog(const QString& groupGuid, const QString& channelId, const QString& userNickname, UIScope scope)
        {
            RemoveRoomUserViewController* controller = mRemoveRoomUserViewControllers[channelId];

            if (!controller)
            {
                controller = new RemoveRoomUserViewController(groupGuid, channelId, userNickname);
                mRemoveRoomUserViewControllers.insert(channelId, controller);
            }
            else
                controller->setUserId(userNickname);

            controller->show(scope);
        }

        void ClientViewController::showFriendsList()
        {
            QString header = tr("No Friends list here");
            UIToolkit::OriginWindow* window =
                UIToolkit::OriginWindow::promptNonModal(UIToolkit::OriginMsgBox::NoIcon, header,
                tr("Please contact Social Scrum and let them know you found this dialog."),
                tr("No Friends list here"),
                tr("ebisu_client_cancel"), QDialogButtonBox::Yes);

            window->setTitleBarText(tr("No Friends List here"));

            ORIGIN_VERIFY_CONNECT(window, SIGNAL(accepted()), window, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(window, SIGNAL(rejected()), window, SLOT(close()));
        }

        void ClientViewController::showSettings()
        {
            onMainMenuApplicationSettings();
        }

        void ClientViewController::showSettingsGeneral()
        {
            bringUpWindow();

            if(mNavBarViewController)
                mNavBarViewController->clearTabs();

            if (mClientView)
                mClientView->showSettingsGeneral();
        }

        void ClientViewController::showSettingsNotifications()
        {
            bringUpWindow();

            if(mNavBarViewController)
                mNavBarViewController->clearTabs();

            if (mClientView)
                mClientView->showSettingsNotifications();
        }

        void ClientViewController::showSettingsInGame()
        {

            if(mNavBarViewController)
                mNavBarViewController->clearTabs();

            if (mClientView)
                mClientView->showSettingsInGame();
        }

        void ClientViewController::showSettingsAdvanced()
        {
            bringUpWindow();
           
            if(mNavBarViewController)
                mNavBarViewController->clearTabs();
    
            if (mClientView)
                mClientView->showSettingsAdvanced();
        }

        void ClientViewController::showSettingsVoice()
        {
            bringUpWindow();

            if(mNavBarViewController)
                mNavBarViewController->clearTabs();

            if (mClientView)
                mClientView->showSettingsVoice();
        }

        void ClientViewController::showHelp()
        {
            onMainMenuOriginHelp();
        }

        void ClientViewController::showOrderHistory()
        {
            bringUpWindow();
            loadTab(SETTINGS_ACCOUNT_TAB);
        }

        void ClientViewController::showAccountProfilePrivacy()
        {
            bringUpWindow();
            mNavBarViewController->clearTabs();
            mClientView->showAccountProfilePrivacy();
        }

        void ClientViewController::showAccountProfileSecurity()
        {
            bringUpWindow();
            mNavBarViewController->clearTabs();
            mClientView->showAccountProfileSecurity();
        }

        void ClientViewController::showAccountProfilePaymentShipping()
        {
            bringUpWindow();
            mNavBarViewController->clearTabs();
            mClientView->showAccountProfilePaymentShipping();
        }

        void ClientViewController::showAccountProfileSubscription(const QString& status)
        {
            bringUpWindow();
            mNavBarViewController->clearTabs();
            mClientView->showAccountProfileSubscription(status);
        }

        void ClientViewController::showAccountProfileRedeem()
        {
            bringUpWindow();
            mNavBarViewController->clearTabs();
            mClientView->showAccountProfileRedeem();
        }


        void ClientViewController::showPromoDialog(PromoBrowserContext context, const Engine::Content::EntitlementRef ent, const bool force)
        {
            switch (context.promoType)
            {
            case PromoContext::FreeTrialExited:
            case PromoContext::FreeTrialExpired:
                {
                    if (!mFreeTrialPromoBrowserViewController)
                    {
                        mFreeTrialPromoBrowserViewController = new PromoBrowserViewController();
                    }

                    mFreeTrialPromoBrowserViewController->show(context, ent, force);
                    break;
                }
            default:
                {
                    if (!mFeaturedPromoBrowserViewController)
                    {
                        mFeaturedPromoBrowserViewController = new PromoBrowserViewController();
                    }

                    mFeaturedPromoBrowserViewController->show(context, ent, force);
                    break;
                }
            }
        }

        void ClientViewController::showNetPromoterDialog()
        {
            if (Origin::Services::readSetting(Origin::Services::SETTING_DisableNetPromoter, Origin::Services::Session::SessionRef()))
            {
                emit netpromoter_notshown();    // so promo manager will still show
                return;
            }
#ifdef DISABLE_INITIAL_SURVEY_POPUP
            emit netpromoter_notshown();    // so promo manager will still show
            return;
#endif
            if(mNetPromoterViewController == NULL)
            {
                mNetPromoterViewController = new NetPromoterViewController();
                ORIGIN_VERIFY_CONNECT(mNetPromoterViewController, SIGNAL(onNoNetPromoter()), this, SIGNAL(netpromoter_notshown()));
            }
            mNetPromoterViewController->show();
        }

        void ClientViewController::showCdnOverrideDialog()
        {
            if(!mCdnOverrideViewController)
            {
                mCdnOverrideViewController = new CdnOverrideViewController();
            }

            mCdnOverrideViewController->show();
        }

        void ClientViewController::handleODTRevertToDefault()
        {
            using namespace Origin::Engine::Config;
            if (OriginWindow::prompt(OriginMsgBox::NoIcon,
                tr("ebisu_client_notranslate_revert_to_default_title"), tr("ebisu_client_notranslate_revert_to_default_text"),
                tr("ebisu_client_notranslate_yes"), tr("ebisu_client_notranslate_no")) == QDialog::Accepted)
            {
                ConfigIngestController controller;
                ConfigIngestController::ConfigIngestError error = controller.revertToDefaultConfig();

                switch(error)
                {
                case ConfigIngestController::kConfigIngestDeleteBackupFailed:
                    // Show eacore.ini.bak delete error dialog
                    Origin::UIToolkit::OriginWindow::alert(OriginMsgBox::Info, 
                        tr("ebisu_client_notranslate_ingest_config_file_title").toUpper(), 
                        tr("ebisu_client_notranslate_ingest_config_file_delete_backup_error").arg(tr("ebisu_client_notranslate_config_file")), 
                        tr("ebisu_client_notranslate_close"));
                        break;

                case ConfigIngestController::kConfigIngestRenameFailed:
                    // Show eacore.ini rename error dialog
                    Origin::UIToolkit::OriginWindow::alert(OriginMsgBox::Info, 
                        tr("ebisu_client_notranslate_ingest_config_file_title").toUpper(), 
                        tr("ebisu_client_notranslate_ingest_config_file_rename_error").arg(tr("ebisu_client_notranslate_config_file")), 
                        tr("ebisu_client_notranslate_close"));
                        break;
                   
                case ConfigIngestController::kConfigIngestNoError:
                default:
                    {
                        int result = Origin::UIToolkit::OriginWindow::prompt(OriginMsgBox::Info, 
                            tr("ebisu_client_notranslate_revert_complete_title"), 
                            tr("ebisu_client_notranslate_revert_complete_text").arg(tr("application_name")), 
                            tr("ebisu_client_notranslate_restart_now"), tr("ebisu_client_notranslate_not_now"));
                        if(result == QDialog::Accepted)
                        {
                            MainFlow::instance()->restart();
                        }
                    }
                    break;
                }
            }
        }

        void ClientViewController::onNavbarClickedMyUsername()
        {
            showMyProfile(Engine::IIGOCommandController::CallOrigin_CLIENT);
        }

        void ClientViewController::onNavbarClickedMyAvatar()
        {
            showMyProfile(ClientScope, ShowProfile_MainWindow, Engine::IIGOCommandController::CallOrigin_CLIENT); // nav bar
        }

        void ClientViewController::setStartupTab()
        {
            Origin::Engine::UserRef user = mUser.toStrongRef();

            Origin::Client::PendingActionFlow *actionFlow = Origin::Client::PendingActionFlow::instance();
            
            //check to see if the origin2:// is going to set the startup tab
            if(actionFlow && !actionFlow->startupTab().isEmpty())
            {
                if(actionFlow->startupTab() == PendingAction::StartupTabStoreId)     
                    OriginApplication::instance().SetServerStartupTab(TabStore);
                else if (actionFlow->startupTab() == PendingAction::StartupTabMyGamesId)
                    OriginApplication::instance().SetServerStartupTab(TabMyGames);

            }
            else
            if (user->overrideTab())
            {
                // Are we using these as an override?
                if (user->defaultTab() == "store")
                    OriginApplication::instance().SetServerStartupTab(TabStore);
                else if (user->defaultTab() == "mygames")
                    OriginApplication::instance().SetServerStartupTab(TabMyGames);
            }

            StartupTab startupTab = OriginApplication::instance().startupTab();
            const bool isUnderAge = Origin::Engine::LoginController::isUserLoggedIn() ? Origin::Engine::LoginController::currentUser()->isUnderAge() : false;
            const bool isOffline = !user || !(Origin::Services::Connection::ConnectionStatesService::isUserOnline (user->getSession()));

            if (startupTab == TabLetOriginDecide)
            {
                Origin::Engine::UserRef user = mUser.toStrongRef();
                QString tab = user->defaultTab();
                if (tab == "store")
                {
                    startupTab = TabStore;
                }
                else 
                {
                    // If we didn't understand what was in the response (or it was empty) we want My Games
                    startupTab = TabMyGames;
                }
            }

            if( (isOffline || isUnderAge) && (startupTab == TabStore) )
            {
                // Shouldn't show the store if offline or underage, show MyGames instead.
                startupTab = TabMyGames;
            }

            NavigationController::instance()->setStartupTab(startupTab);
            switch(startupTab)
            {
                case TabStore:
                    {
                        QString barePath = Origin::Services::readSetting (Origin::Services::SETTING_UrlStoreProductBarePath);
                        if (!barePath.isEmpty ())
                        {
                            // TODO: HACK - ReadOnly Settings - need a way to correctly clear and unset the setting
                            //consume it
                            Origin::Services::writeSetting (Origin::Services::SETTING_UrlStoreProductBarePath, "");
                            showProductIDInStore(barePath);                            
                        }
                        else
                            onShowStore();
                        updateStoreTab();
                        break;
                    }

                case TabMyGames:
                default:
                    {
                        QString ids = Origin::Services::readSetting (Origin::Services::SETTING_UrlLibraryGameIds);
                        if (!ids.isEmpty())  //start up by showing game details of this id
                        {
                            //consume the setting 
                            // TODO: HACK - ReadOnly Settings - need a way to correctly clear and unset the setting
                            Origin::Services::writeSetting (Origin::Services::SETTING_UrlLibraryGameIds, "");
                            
                            bool found = false;
                            QStringList idList = ids.split(',', QString::SkipEmptyParts);
                            for(QStringList::const_iterator it = idList.constBegin(); it != idList.constEnd(); ++it)
                            {
                                Origin::Engine::Content::EntitlementRef entRef = Origin::Engine::Content::ContentController::currentUserContentController()->entitlementById(*it);
                                if (!entRef.isNull())
                                {
                                    showGameDetails (entRef);
                                    found = true;
                                    break;
                                }
                            }

                            if(!found)
                                onShowMyGames();
                        }
                        else
                            onShowMyGames();
                        updateMyGamesTab();

                        break;
                    }
            }
        }
            
        void ClientViewController::showGameDetails(Origin::Engine::Content::EntitlementRef entitlement)
        {

        }

        void ClientViewController::showTileView()
        {

        }
        void ClientViewController::showRedemptionPage (RedeemBrowser::SourceType src /*= Origin*/, RedeemBrowser::RequestorID requestorID /*= OriginCodeClient*/, const QString &code)
        {
            using namespace Origin::UIToolkit;
            ORIGIN_LOG_ACTION << "User selected code redemption dialog.";
            
            if(mRedeemWindow == NULL)
            {
                mRedeemWindow = mClientView->createRedemptionWindow (src, requestorID, code);
                if (mRedeemWindow)
                {
                    ORIGIN_VERIFY_CONNECT((RedeemBrowser*)(mRedeemWindow->content()), SIGNAL(closeWindow()), this, SLOT(closeRedemptionPage()));
                    ORIGIN_VERIFY_CONNECT(mRedeemWindow, SIGNAL(rejected()), this, SLOT(closeRedemptionPage()));
                    
                    emit redemptionPageShown();
                }
            }
            mRedeemWindow->showWindow();

            //we need to have an activate window here otherwise when this is triggered via origin:// the redeem window will not show to the front
            //redeem is a window that is only triggered via user interaction, so it will not popup when the user is doing something else
            mRedeemWindow->activateWindow();
        }

        void ClientViewController::closeRedemptionPage()
        {
            if (mRedeemWindow)
            {
                GetTelemetryInterface()->Metric_ACTIVATION_WINDOW_CLOSE("window");

                ORIGIN_VERIFY_DISCONNECT((RedeemBrowser*)(mRedeemWindow->content()), SIGNAL(closeWindow()), this, SLOT(closeRedemptionPage()));
                ORIGIN_VERIFY_DISCONNECT(mRedeemWindow, SIGNAL(rejected()), this, SLOT(closeRedemptionPage()));
                mRedeemWindow->close();
                mRedeemWindow = NULL;
                
                emit redemptionPageClosed();
            }
            
            if(Origin::Client::MainFlow::instance()->rtpFlow()->getRtpLaunchActive() && Origin::Client::MainFlow::instance()->rtpFlow()->getCodeRedeemed())
            {
                Origin::Client::MainFlow::instance()->rtpFlow()->attemptPendingLaunch();
            }
        }

        void ClientViewController::onOnlineStatusChangedAuth(bool online)
        {
            if (online)
            {
                Engine::Subscription::SubscriptionManager *pSubsMgr = Engine::Subscription::SubscriptionManager::instance();

                GetTelemetryInterface()->Metric_SUBSCRIPTION_USER_GOES_ONLINE(QDateTime::fromMSecsSinceEpoch(pSubsMgr->lastKnownGoodTime()).toString().toUtf8(),
                    pSubsMgr->expiration().toString().toUtf8());
            }
            mMainMenuController->setOnline(online);
            setUserOnlineState();
        }

        void ClientViewController::onOfflineErrorDialogClose()
        {
            if( mOfflineErrorDialog )
            {
                mOfflineErrorDialog->close();
                mOfflineErrorDialog = NULL;
            }
        
            if( mOfflineErrorDialogIGO )
            {
                mOfflineErrorDialogIGO->close();
                mOfflineErrorDialogIGO = NULL;
            }
        }

        void ClientViewController::requestOnlineModeRetry()
        {
            onOfflineErrorDialogClose();
            // Use a timer so we don't recurse into requestOnlineMode and also to give
            // the connection problem a chance to resolve itself so the retry might actually work
            QTimer::singleShot(1000, this, SLOT(requestOnlineMode()));
        }

        void ClientViewController::requestOnlineMode(const int& friendOrigin)
        {
            GetTelemetryInterface()->Metric_SINGLE_LOGIN_GO_ONLINE_ACTION();

            Origin::Engine::UserRef user = Origin::Engine::LoginController::instance()->currentUser();

            //if already online then just ignore (UI may not be showing the friends list yet)
            if (Origin::Services::Connection::ConnectionStatesService::isUserOnline(user->getSession()))
                return;
    
            if ( Origin::Services::Connection::ConnectionStatesService::canGoOnline(user->getSession()) )
            {
                // set the client online
                MainFlow::instance()->socialGoOnline();
            }
            else
            {
                // create error dialog for required update pending
                const bool requiredUpdatePending = Origin::Services::Connection::ConnectionStatesService::isRequiredUpdatePending();

                // if we have no dialog, create one
                // if we have one, but it's not a requiredUpdatePending one, create a requiredUpdatePending one 
                if (mOfflineErrorDialog == NULL || (mOfflineErrorDialog->msgBox()->getTitle().compare(tr("ebisu_client_required_update_title")) != 0 && requiredUpdatePending == true))
                {
                    onOfflineErrorDialogClose();

                    if (requiredUpdatePending)
                    {
                        mOfflineErrorDialog = UIToolkit::OriginWindow::promptNonModal(UIToolkit::OriginMsgBox::Notice,
                            tr("ebisu_client_required_update_title"),
                            tr("ebisu_client_update_requires_restart_description").arg(QApplication::applicationName()),
                            tr("ebisu_client_restart_now"),
                            tr("ebisu_client_restart_later"));
                        ORIGIN_VERIFY_CONNECT(mOfflineErrorDialog, SIGNAL(accepted()), this, SLOT(restartOrigin()));
                        ORIGIN_VERIFY_CONNECT(mOfflineErrorDialog, SIGNAL(rejected()), this, SLOT(onOfflineErrorDialogClose()));

                        mOfflineErrorDialogIGO = UIToolkit::OriginWindow::promptNonModal(UIToolkit::OriginMsgBox::Notice,
                            tr("ebisu_client_required_update_title"),
                            tr("ebisu_client_update_requires_restart_description").arg(QApplication::applicationName()),
                            tr("ebisu_client_restart_now"),
                            tr("ebisu_client_restart_later"));
                        mOfflineErrorDialogIGO->configForOIG();
                        ORIGIN_VERIFY_CONNECT(mOfflineErrorDialogIGO, SIGNAL(accepted()), this, SLOT(restartOrigin()));
                        ORIGIN_VERIFY_CONNECT(mOfflineErrorDialogIGO, SIGNAL(rejected()), this, SLOT(onOfflineErrorDialogClose()));
                    }
                    else
                    {
                        // create generic error dialog for other reasons for being offline
                        mOfflineErrorDialog = UIToolkit::OriginWindow::promptNonModal(UIToolkit::OriginMsgBox::Error,
                            tr("ebisu_client_connection_error_title"),
                            tr("ebisu_client_connection_error_body"),
                            tr("ebisu_mainmenuitem_go_online"),
                            tr("ebisu_client_stay_offline"));
                        ORIGIN_VERIFY_CONNECT(mOfflineErrorDialog, SIGNAL(accepted()), this, SLOT(requestOnlineModeRetry()));
                        ORIGIN_VERIFY_CONNECT(mOfflineErrorDialog, SIGNAL(rejected()), this, SLOT(onOfflineErrorDialogClose()));

                        mOfflineErrorDialogIGO = UIToolkit::OriginWindow::promptNonModal(UIToolkit::OriginMsgBox::Error,
                            tr("ebisu_client_connection_error_title"),
                            tr("ebisu_client_connection_error_body"),
                            tr("ebisu_mainmenuitem_go_online"),
                            tr("ebisu_client_stay_offline"));
                        mOfflineErrorDialogIGO->configForOIG();
                        ORIGIN_VERIFY_CONNECT(mOfflineErrorDialogIGO, SIGNAL(accepted()), this, SLOT(requestOnlineModeRetry()));
                        ORIGIN_VERIFY_CONNECT(mOfflineErrorDialogIGO, SIGNAL(rejected()), this, SLOT(onOfflineErrorDialogClose()));
                    }

                    // show the dialog in OIG and on the desktop!!!
                    Engine::IGOController::instance()->igoShowError(mOfflineErrorDialogIGO);
                    mOfflineErrorDialog->showWindow();
                }
            }
        }

        //used by brige/remote to initiate logout from JS interface
        //could have called onLogout directly but want to use the same flow as if the user made the selection from the main menu
        void ClientViewController::requestLogoutFromJS()
        {
            onMainMenuLogOut();
        }

        void ClientViewController::restartOrigin()
        {
            // We need to make sure both dialogs are closed before starting the restart process
            onOfflineErrorDialogClose();
            MainFlow::instance()->restart();
        }

        void ClientViewController::requestManualOfflineMode()
        {
            GetTelemetryInterface()->Metric_SINGLE_LOGIN_GO_OFFLINE_ACTION();
            MainFlow::instance()->socialGoOffline();
            setUserOnlineState();
        }

        void ClientViewController::updateAccessibleName(QString& string)
        {
            // This is used by screenreaders, etc.
            // It is also the string that is placed in the Mac Window menu list
            // The purpose is to give the client window a more descriptive name than
            // what appears on the titlebar, which is typically just "Origin".
            if (mMainWindow) mMainWindow->setAccessibleName(string);
        }


        // *** BEGIN MAIN MENU HANDLERS
        //
        // FOR HYGIENE REASONS I AM PUTTING A SLOT FOR EVERY MENU ACTION AND NOT DOING
        // A TON OF CRAZY SIGNAL TO SIGNAL AND SIGNAL TO SLOT MIXTURES. EVERYTHING THAT
        // IS A MAIN MENU ACTION GETS A DEDICATED SLOT HERE AND THEN GETS PROPERLY ROUTED
        //
        // THIS ALSO CREATES A FIREWALL/DMZ/ABSTRACTION-LAYER BETWEEN NEW CODE AND THINGS
        // THAT STILL NEED TO REACH INTO THE LEGACY APP.
        //
        // Translation: Menu controller = good. Legacy code = bad. Route all menu actions
        // through a common place - these functions.
        //

        void ClientViewController::onMainMenuExit()
        {
            ORIGIN_LOG_ACTION << "User Action";
            onExit();
        }

        void ClientViewController::onMainMenuRedeemProductCode()
        {
            ORIGIN_LOG_ACTION << "User Action";
            showRedemptionPage();
        }

        void ClientViewController::onMainMenuReloadMyGames()
        {
            ORIGIN_LOG_ACTION << "User Action - Reload My Games";
            mClientView->refreshEntitlements();
        }

        void ClientViewController::onMainMenuReloadSPA()
        {
            ORIGIN_LOG_ACTION << "User Action - Reload SPA";
            if (mMainWindow && mMainWindow->webview() && mMainWindow->webview()->webview())
            {
                mSPALoadTimer.start();
                ORIGIN_LOG_EVENT << "SPA Load Start TS: " << QDateTime::currentMSecsSinceEpoch();
                mMainWindow->webview()->webview()->reload();
            }
            else
            {
                ORIGIN_LOG_ERROR << "User Action - Error: main window isn't valid";
            }
        }

        void ClientViewController::onMainMenuApplicationSettings()
        {
            ORIGIN_LOG_ACTION << "User Action - Application Settings";
            onShowSettingsApplication();
        }

        void ClientViewController::onMainMenuAccount()
        {
            ORIGIN_LOG_ACTION << "User Action - Account Settings";
            onShowSettingsAccount();
        }

        void ClientViewController::onMainMenuToggleOfflineOnline()
        {
            ORIGIN_LOG_ACTION << "User Action - Offline";
            Origin::Engine::UserRef user = mUser.toStrongRef();
            if (!user.isNull())
            {
                if (Origin::Services::Connection::ConnectionStatesService::isUserOnline (user->getSession()))
                {
                    // We're online - go offline
                    requestManualOfflineMode();
                }
                else
                {
                    // We're offline - go online
                    requestOnlineMode();
                }
            }
        }

        void ClientViewController::onMainMenuLogOut()
        {
            ORIGIN_LOG_ACTION << "User Action - Log out";
            onLogout();
        }

        void ClientViewController::onMainMenuAddFriend()
        {
            ORIGIN_LOG_ACTION << "User Action - Add friend";

            showFriendSearchDialog(ClientScope, Engine::IIGOCommandController::CallOrigin_CLIENT);
        }

        void ClientViewController::onMainMenuOriginHelp()
        {
            ORIGIN_LOG_ACTION << "User Action - Origin help";

            GetTelemetryInterface()->Metric_HELP_ORIGIN_HELP();

            mClientView->showCustomerSupport();
        }

        void ClientViewController::onMainMenuOriginER()
        {
            ORIGIN_LOG_ACTION << "User Action - OER";

            GetTelemetryInterface()->Metric_HELP_ORIGIN_ER();
            const bool isSubscriber = Engine::Subscription::SubscriptionManager::instance()->isActive();

            QString program = "OriginER.exe";
            QStringList arguments;
            arguments << "-originId" << mUser.toStrongRef()->eaid() << "-source" << "main_menu" << "-sub " << QString::number(isSubscriber);
            bool isStarted = QProcess::startDetached(program, arguments, ".");

            if (!isStarted)
                ORIGIN_LOG_ACTION << "unable to launch OriginER";
            
        }

        void ClientViewController::onMainMenuOriginForum()
        {
            ORIGIN_LOG_ACTION << "User Action - Origin Forum";

            GetTelemetryInterface()->Metric_HELP_ORIGIN_FORUM();

            mClientView->showEbisuForum();
        }

        void ClientViewController::onMainMenuOrderHistory()
        {
            ORIGIN_LOG_ACTION << "User Action - Order history";
            showOrderHistory();
        }

        void ClientViewController::onMainMenuAbout()
        {
            ORIGIN_LOG_ACTION << "User Action - About";

            mClientView->showAbout();
        }

        void ClientViewController::onMainMenuAddNonOriginGame()
        {
            ORIGIN_LOG_ACTION << "User Action - Add game";

            emit addNonOriginGame();
        }

        void ClientViewController::onPromoLoadCompleteExecuteUserAction(PromoBrowserContext context, bool promoLoadedSuccessfully)
        {
            // The "featured" promo browser should be non-NULL at this point, but just in case...
            if (mFeaturedPromoBrowserViewController)
            {
                ORIGIN_VERIFY_DISCONNECT(mFeaturedPromoBrowserViewController, SIGNAL(promoBrowserLoadComplete(PromoBrowserContext,bool)), 
                    this, SLOT(onPromoLoadCompleteExecuteUserAction(PromoBrowserContext,bool)));
            }

            if(context.promoType != PromoContext::MenuPromotions || !promoLoadedSuccessfully)
            {
                showPromoDialog(PromoContext::MenuPromotions, Engine::Content::EntitlementRef(), true);
            }
        }

        void ClientViewController::onMainMenuDEBUGCheckforUpdates()
        {
            ORIGIN_LOG_ACTION << "User Action";

            checkForUpdates();
        }

        void ClientViewController::onGameLibraryUpdated(QByteArray payload)
        {
            Q_UNUSED(payload);

            Origin::Engine::Content::ContentController *userContCtrl = Origin::Engine::Content::ContentController::currentUserContentController();
            if (userContCtrl)
                userContCtrl->refreshUserGameLibrary(Origin::Engine::Content::ContentUpdates);
        }

        void ClientViewController::onAvatarChanged(QByteArray /*payload*/)
        {
            Engine::UserRef user = mUser.toStrongRef();
            if (!user)
            {
                ORIGIN_ASSERT(NULL);
                return;
            }

            Engine::Social::SocialController *socialController = user->socialControllerInstance();
            socialController->suscribeToRefreshAvatars(user->userId());
        }

        void ClientViewController::onPlayFinished(Origin::Engine::Content::EntitlementRef entitlement)
        {
            if(entitlement.isNull())
                return;

            NetPromoterViewControllerGame* netPromoterViewControllerGame = new NetPromoterViewControllerGame(entitlement->contentConfiguration()->productId(), this);
            netPromoterViewControllerGame->show();
            ORIGIN_VERIFY_CONNECT(netPromoterViewControllerGame, SIGNAL(finish(const bool&)), netPromoterViewControllerGame, SLOT(deleteLater()));
        }

        void ClientViewController::onGroupsEvent(QByteArray payload)
        {
            Q_UNUSED(payload);
        }

        void ClientViewController::onPrivacySettingsUpdated(QByteArray payload)
        {
            Q_UNUSED(payload);
        }

        void ClientViewController::onMainMenuDEBUGtestDirtyBitsFakeMessages()
        {
            Origin::Engine::DirtyBitsClient::instance()->setFakeMessages();
        }

        void ClientViewController::onMainMenuDEBUGtestDirtyBitsConnectHandlers()
        {
            //add registering of new handlers here for testing (see ClientFlow::connectDirtyBitHandlers) 
        }

        void ClientViewController::onMainMenuDEBUGtestDirtyBitsFakeMessageFromFile()
        {
            if(Origin::Engine::DirtyBitsClient::instance() != NULL)
            {
                Origin::Engine::DirtyBitsClient::instance()->deliverFakeMessageFromFile();
            }
        }

        void ClientViewController::onMainMenuDEBUGsetWebSocketVerbosity()
        {
            Origin::Engine::DirtyBitsClient::instance()->setSocketVerbosity();
        }

        void ClientViewController::onMainMenuDEBUGshowDirtyBitsViewer()
        {
            MainFlow::instance()->showDirtyBitsTrafficTool();
        }

        void ClientViewController::onMainMenuDEBUGtestDirtyBits()
        {
            Origin::Engine::DirtyBitsClient::logActivity();
        }

        void ClientViewController::onMainMenuDEBUGOriginDeveloperTool()
        {
            ORIGIN_LOG_ACTION << "User Action";

            GetTelemetryInterface()->Metric_ODT_LAUNCH();

            const QString odtPluginOfferId(Services::OriginConfigService::instance()->odtPlugin().productId);

            Engine::Content::ContentController* pContentController = Origin::Engine::Content::ContentController::currentUserContentController();
            Origin::Engine::Content::EntitlementRef ent = pContentController->entitlementByProductId(odtPluginOfferId);

            if (ent)
            {
                Origin::Engine::Content::ContentConfigurationRef pContentConfig = ent->contentConfiguration();
                Origin::Engine::Content::LocalContent* pLocalContent = ent->localContent();

                if (pContentConfig && pLocalContent)
                {
                    if (pLocalContent->downloadable())
                    {
                        Origin::Engine::Content::ContentController::DownloadPreflightingResult result;
                        pContentController->doDownloadPreflighting(*pLocalContent, result);
                        if (result >= Origin::Engine::Content::ContentController::CONTINUE_DOWNLOAD)
                        {
                            pLocalContent->download("");
                        }
                    }
                    else if (pLocalContent->playable())
                    {
                        MainFlow::instance()->contentFlowController()->startPlayFlow(odtPluginOfferId, false);  // let playNow check for patches and bring up Update Progress bar
                    }
                    else
                    {
                        OriginWindow* messageBox = new OriginWindow(
                            OriginWindow::Icon | OriginWindow::Close, NULL, OriginWindow::MsgBox, QDialogButtonBox::Ok);
                        messageBox->msgBox()->setup(
                            OriginMsgBox::Notice,
                            "PLUG-IN UNAVAILABLE",
                            "The ODT plugin can't be launched right now.\n\n" 
                            "If the plugin is downloading, please wait for the download to finish. Otherwise, run the repair process on the entitlement.\n\n"
                            "If the issue persists, please uninstall then reinstall the plugin.");
                        messageBox->setButtonText(QDialogButtonBox::Ok, "Close");
                        messageBox->setDefaultButton(QDialogButtonBox::Ok);

                        ORIGIN_VERIFY_CONNECT(messageBox, SIGNAL(finished(int)), messageBox, SLOT(close()));
                        ORIGIN_VERIFY_CONNECT(messageBox, SIGNAL(rejected()), messageBox, SLOT(close()));
                        ORIGIN_VERIFY_CONNECT(messageBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), messageBox, SLOT(accept()));
                        messageBox->manager()->setupButtonFocus();
                        messageBox->execWindow();
                    }
                }
            }

            else
            {
                const bool pluginSuccess = Engine::PluginManager::instance()->runPlugin(odtPluginOfferId);

                if (!pluginSuccess)
                {
                    OriginWindow* messageBox = new OriginWindow(
                        OriginWindow::Icon | OriginWindow::Close, NULL, OriginWindow::MsgBox, QDialogButtonBox::Ok);
                    messageBox->msgBox()->setup(
                        OriginMsgBox::Notice,
                        "PLUG-IN ERROR",
                        "An error occurred trying to load the ODT plugin.\n\n" 
                        "Please log into an account entitled with the ODT and run repair process on the entitlement.\n\n"
                        "If the issue persists, please uninstall then reinstall the plugin.");
                    messageBox->setButtonText(QDialogButtonBox::Ok, "Close");
                    messageBox->setDefaultButton(QDialogButtonBox::Ok);

                    ORIGIN_VERIFY_CONNECT(messageBox, SIGNAL(finished(int)), messageBox, SLOT(close()));
                    ORIGIN_VERIFY_CONNECT(messageBox, SIGNAL(rejected()), messageBox, SLOT(close()));
                    ORIGIN_VERIFY_CONNECT(messageBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), messageBox, SLOT(accept()));
                    messageBox->manager()->setupButtonFocus();
                    messageBox->execWindow();
                }
            }
        }
        
        void ClientViewController::onMainMenuDEBUGRevertToDefault()
        {
            ORIGIN_LOG_ACTION << "User Action";

            handleODTRevertToDefault();
        }
        
        void ClientViewController::onMainMenuDEBUGCdnOverride()
        {
            ORIGIN_LOG_ACTION << "User Action";

            showCdnOverrideDialog();
        }

        void ClientViewController::onMainMenuDEBUGDisableEmbargoMode()
        {
            ORIGIN_LOG_ACTION << "User Action";
            Services::writeSetting(Services::SETTING_DisableEmbargoMode, true);
            updateEnvronmentLabel();
        }

        void ClientViewController::onMainMenuDEBUGIncrementalRefresh()
        {
            ORIGIN_LOG_ACTION << "User Action: Incremental Refresh.";
            Origin::Engine::Content::ContentController *contentController = Origin::Engine::Content::ContentController::currentUserContentController();
            if(contentController != NULL)
            {
                contentController->refreshUserGameLibrary(Origin::Engine::Content::ContentUpdates);
            }
        }

        void ClientViewController::onMainMenuDEBUGOthSettingClear()
        {
            ORIGIN_LOG_ACTION << "User Action: Clear Oth Setting.";
            Services::writeSetting(Services::SETTING_LastDismissedOtHPromoOffer, "");
            Services::writeSetting(Services::SETTING_ONTHEHOUSE_VERSIONS_SHOWN, "");
        }

        void ClientViewController::onMainMenuDEBUGIGOExtraLogging(bool checked)
        {
            ORIGIN_LOG_ACTION << "User Action: Extra IGO logs";
            Origin::Engine::IGOController::instance()->setExtraLogging(checked);
        }
        
        void ClientViewController::onMainMenuDEBUGIGOWebInspectors(bool checked)
        {
            ORIGIN_LOG_ACTION << "User Action: Toggle Web Inspectors";
            Origin::Engine::IGOController::instance()->setShowWebInspectors(checked);
        }
        
        void ClientViewController::onMainMenuDEBUGJoinPendingGroups()
        {
            Services::GroupPendingInviteResponse* response = Services::GroupServiceClient::getPendingGroupInvites(
                Services::Session::SessionService::currentSession());

            ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(onMainMenuDEBUGPendingGroups()));
        }

        void ClientViewController::onMainMenuDEBUGPendingGroups()
        {
            Services::GroupPendingInviteResponse* resp = dynamic_cast<Services::GroupPendingInviteResponse*>(sender());
            ORIGIN_ASSERT(resp);

            if (resp != NULL)
            {
                QVector<Origin::Services::GroupPendingInviteResponse::PendingGroupInfo> pendingGroups = resp->getPendingGroups();                

                for (QVector<Origin::Services::GroupPendingInviteResponse::PendingGroupInfo>::const_iterator i = pendingGroups.cbegin();
                    i != pendingGroups.cend(); ++i)
                {
                    Services::GroupJoinResponse* response = Services::GroupServiceClient::joinGroup(
                        Services::Session::SessionService::currentSession(), (*i).groupGuid);
                    (void) response;
                }
            }
        }

        void ClientViewController::onMainMenuDiscover()
        {
            ORIGIN_LOG_ACTION << "User Action";
            loadTab(MY_ORIGIN_TAB);
        }

        void ClientViewController::onMainMenuStore()
        {
            ORIGIN_LOG_ACTION << "User Action";
            
            showStoreHome();
        }
        
        void ClientViewController::onMainMenuMyGames()
        {
            ORIGIN_LOG_ACTION << "User Action";
            
            showMyGames();
        }
        
#if defined(ORIGIN_MAC)
        void ClientViewController::onMainMenuToggleFullScreen()
        {
            ORIGIN_LOG_ACTION << "User Action";
            
            mMainWindow->toggleFullScreen();
        }
#endif
        
        // *** END MAIN MENU ACTION HANDLERS

        void ClientViewController::setUserOnlineState()
        {
            Origin::Engine::UserRef user = Origin::Engine::LoginController::instance()->currentUser();
            bool isOnline = !user.isNull() ? Origin::Services::Connection::ConnectionStatesService::isUserOnline (user->getSession()) : false;
            mClientView->enableSocialTab(isOnline);
        }


        void ClientViewController::onLoadingGamesTooSlow()
        {
        }

        void ClientViewController::bringUpWindow()
        {
            Origin::Client::PendingActionFlow *actionFlow = Origin::Client::PendingActionFlow::instance();

            //if the pending action requests that we show minimized, don't bring up the window
            if(actionFlow && ((actionFlow->windowsShowState() == ClientWindow_SHOW_MINIMIZED) || (actionFlow->windowsShowState() == ClientWindow_SHOW_MINIMIZED_SYSTEMTRAY)))
            {
                return;
            }

            if(mMainWindow)
            {
                // If the client is minimized or hidden (in the system tray): 
                // Use consistent behavior of when we show the window from the system tray
                if(mMainWindow->isMinimized() || mMainWindow->isVisible() == false)
                    EbisuSystemTray::instance()->showPrimaryWindow();

                // If we got here from on from an external source that's on top of origin, raise 
                // Origin to the top of the screen
                mMainWindow->activateWindow();
            }
        }

        void ClientViewController::alertMainWindow()
        {
            if(mMainWindow)
            {
                QApplication::alert(mMainWindow);
            }
        }

        void ClientViewController::showWelcomeMessage()
        {
            // Disabled until further notice
            bool firstRun = false;// Origin::Services::readSetting(Services::SETTING_FIRST_RUN);
            if (firstRun)
            {
                using namespace Origin::UIToolkit;
                Services::writeSetting(Services::SETTING_FIRST_RUN, false);
            }
        }

        void ClientViewController::checkForUpdates()
        {
            QString locale = OriginApplication::instance().locale();
            QString type = Origin::Services::readSetting(Origin::Services::SETTING_BETAOPTIN, Origin::Services::Session::SessionService::currentSession()) ? "BETA" : "PROD";
            QString platform;

            switch(Origin::Services::PlatformService::runningPlatform())
            {
            case Origin::Services::PlatformService::PlatformWindows:
                platform = "PCWIN";
                break;
            case Origin::Services::PlatformService::PlatformMacOS:
                platform = "MAC";
                break;
            default:
                ORIGIN_LOG_WARNING << "Unknown platform detected...";
                platform = "UNKNOWN";
                break;
            }

            Engine::UpdateController::instance()->checkForUpdate(locale, type, platform);
        }

        bool ClientViewController::isMainWindowVisible ()
        {
            if (mMainWindow)
                return mMainWindow->isVisible();
            return false;
        }

        void ClientViewController::openJSWindow(const QString& requestType, const QString& requestExtraParams)
        {
            // requestType is mapped in popout.js
            // Make sure we can popup windows!
            mMainWindow->webview()->webview()->settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);
            
            // Run the JS SDK command to open a specific child window
            QString cmd = QString("window._openPopOutWindow(\"%1\", \"%2\")").arg(requestType, requestExtraParams);
            mMainWindow->webview()->webview()->page()->mainFrame()->evaluateJavaScript(cmd);
        }

        void ClientViewController::navigateJSWindow(const QString& requestType, const QString& requestExtraParams)
        {
            // Make sure we can popup windows!
            mMainWindow->webview()->webview()->settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);

            // Run the JS SDK command to open a specific child window
            QString cmd = QString("window._navigatePopOutWindow(\"%1\", \"%2\")").arg(requestType, requestExtraParams);
            mMainWindow->webview()->webview()->page()->mainFrame()->evaluateJavaScript(cmd);

        }

        void ClientViewController::showAcceptGroupInviteDialog(qint64 inviteFrom, const QString& groupName, const QString& groupGuid)
        {
            UIScope scope = ClientScope;
            if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
                scope = IGOScope;

            AcceptGroupInviteViewController* controller = mAcceptGroupInviteViewControllers[groupGuid];

            if (!controller)
            {
                Engine::UserRef user = mUser.toStrongRef();

                if (!user)
                {
                    return;
                }

                Engine::Social::UserNamesCache *userNames = user->socialControllerInstance()->userNames();
                const QString originId = userNames->namesForNucleusId(inviteFrom).originId();

                controller = new AcceptGroupInviteViewController(originId, groupName, groupGuid);
                mAcceptGroupInviteViewControllers.insert(groupGuid, controller);
            }

            controller->show(scope);

        }

        void ClientViewController::closeGroupInviteAndMembersDialogs(const QString& groupGuid)
        {
            GroupInviteViewController* givc = mGroupInviteViewControllers[groupGuid];
            if (givc)
            {
                givc->close();
                mGroupInviteViewControllers.remove(groupGuid);
            }

            GroupMembersViewController* gmvc = mGroupMembersViewControllers[groupGuid];
            if (gmvc)
            {
                gmvc->close();
                mGroupMembersViewControllers.remove(groupGuid);
            }

            TransferOwnerConfirmViewController* tocvc = mTransferOwnerConfirmViewControllers[groupGuid];
            if (tocvc)
            {
                tocvc->close();
                mTransferOwnerConfirmViewControllers.remove(groupGuid);
            }
        }

        void ClientViewController::onRoomDeleted(const QString& channelId)
        {
            EnterRoomPasswordViewController* epvc = mEnterRoomPasswordViewController[channelId];
            if (epvc)
            {
                epvc->close();
            }

            RemoveRoomUserViewController* rruvc = mRemoveRoomUserViewControllers[channelId];
            if (rruvc)
            {
                rruvc->close();
            }

            /* Need to delete these when a MUC_GROUP_ROOM_DESTROYED message comes in for each member
            mEnterRoomPasswordViewController  member
            */
        }

        void ClientViewController::onDepartGroup(const QString& groupGuid)
        {
            closeGroupInviteAndMembersDialogs(groupGuid);
        }

        void ClientViewController::onGroupDeleted(const QString& groupGuid)
        {
            closeGroupInviteAndMembersDialogs(groupGuid);

            CreateRoomViewController* crvc = mCreateRoomViewControllers[groupGuid];
            if (crvc)
            {
                crvc->close();
            }


            EditGroupViewController* egvc = mEditGroupViewControllers[groupGuid];
            if (egvc)
            {
                egvc->close();
            }

            RemoveGroupUserViewController* rguvc = mRemoveGroupUserViewControllers[groupGuid];
            if (rguvc)
            {
                rguvc->close();
            }

            /* Need to delete these when a GROUP_DESTROYED message comes in for each member
            mLeaveGroupViewControllers        member
            */

        }

        void ClientViewController::onKickedFromGroup(const QString& groupName, const QString& groupGuid, qint64 kickedBy)
        {
            closeGroupInviteAndMembersDialogs(groupGuid);
        }

        void ClientViewController::onGroupDeletedByRemoteUser(const QString& groupName, qint64 removedBy, const QString& groupId)
        {
            closeGroupInviteAndMembersDialogs(groupId);
        }

        void ClientViewController::onSystemMenuExit()
        {
            if (MainFlow::instance() && Origin::Engine::LoginController::isUserLoggedIn())
            {
                // If we are logged in, kick off the exit flow.  Skip the confirmation step.
                MainFlow::instance()->exit(ClientLogoutExitReason_Exit_NoConfirmation);
            }
        }
        static bool isModifier(int key)
        {
            switch (key)
            {
            case Qt::Key_Shift:
            case Qt::Key_Control:
            case Qt::Key_Alt:
            case Qt::Key_Meta:
                return true;
            }

            return false;
        }

        void ClientViewController::handleHotkeyInput(const Services::Setting& hotkey, const Services::Setting& hotkeyStr, QKeyEvent* keyEvent)
        {
            if (keyEvent)
            {
                Qt::KeyboardModifiers modifiers = keyEvent->modifiers();

#ifdef ORIGIN_MAC
                Qt::KeyboardModifiers save_modifiers = modifiers;
                // Swap the Qt::ControlModifier and the Qt::MetaModifier so that the control key would be the same if we were to share the hot key settings between platform one day
                Qt::KeyboardModifier ctrlModifier = (modifiers & Qt::MetaModifier) ? Qt::ControlModifier : Qt::NoModifier;
                Qt::KeyboardModifier metaModifier = (modifiers & Qt::ControlModifier) ? Qt::MetaModifier : Qt::NoModifier;
                modifiers = modifiers & ~(Qt::ControlModifier | Qt::MetaModifier);
                modifiers = modifiers | ctrlModifier | metaModifier;

                ORIGIN_LOG_EVENT << "handleHotkeyInput - mod before: " << save_modifiers << " mod after: " << modifiers;
#endif
                // to prevent cases of Ctrl+Shift - if the key() is the modifier then clear out the modifiers
                if ((modifiers != Qt::NoModifier) && isModifier(keyEvent->key()))
                {
                    modifiers = Qt::NoModifier;
                    ORIGIN_LOG_EVENT << "handleHotkeyInput - modifiers cleared";
                }

                Services::InGameHotKey settingHotKeys(modifiers, keyEvent->key(), keyEvent->nativeVirtualKey());
                ORIGIN_LOG_EVENT << settingHotKeys.toULongLong() << " " << settingHotKeys.asNativeString() << " status: " << settingHotKeys.status();
                switch (settingHotKeys.status())
                {

                case Services::InGameHotKey::Valid:
                    if (mTakenHotkeys.find(settingHotKeys.asNativeString()) != mTakenHotkeys.end())
                    {   // hot key conflict (don't write out the hotkey - send web-side event that an error occurred)
                        // pass the setting name of the hot key that conflicts as that will be the one we swap with
                        // do the swap:

                        // 1) read in current settings of key in focus
                        Services::InGameHotKey currentHotKey = Services::InGameHotKey::fromULongLong(Services::readSetting(hotkey, Services::Session::SessionService::currentSession()));
                        // 2) write current settings of key in focus to settings that conflict
                        Services::writeSetting(*mTakenHotkeys[settingHotKeys.asNativeString()][0], currentHotKey.toULongLong(), Services::Session::SessionService::currentSession());
                        Services::writeSetting(*mTakenHotkeys[settingHotKeys.asNativeString()][1], currentHotKey.asNativeString(), Services::Session::SessionService::currentSession());

                        emit hotkeySettingError("hotkey_taken", mTakenHotkeys[settingHotKeys.asNativeString()][1]->name());
                    }

                    ORIGIN_LOG_EVENT << "handleHotkeyInput - written " << settingHotKeys.asNativeString() << " status: " << settingHotKeys.status();
                    Services::writeSetting(hotkey, settingHotKeys.toULongLong(), Services::Session::SessionService::currentSession());
                    Services::writeSetting(hotkeyStr, settingHotKeys.asNativeString(), Services::Session::SessionService::currentSession());

                    break;

                case Services::InGameHotKey::NotValid_Revert: // If the user put in something bad: e.g. Esc
                    if (Services::isDefault(hotkey) == false)
                    {
                        Services::reset(hotkey);
                        settingHotKeys = Services::InGameHotKey::fromULongLong(Services::readSetting(hotkey, Services::Session::SessionService::currentSession()));
                        Services::writeSetting(hotkeyStr, settingHotKeys.asNativeString(), Services::Session::SessionService::currentSession());
                        ORIGIN_LOG_EVENT << "handleHotkeyInput - NotValid_Revert - written " << settingHotKeys.asNativeString() << " status: " << settingHotKeys.status();
                    }
                        
                    break;

                case Services::InGameHotKey::NotValid_Ignore:
                default:
                    break;
                }
            }
        }

        void ClientViewController::handlePushToTalkHotkeyInput(QKeyEvent* keyEvent)
        {
#if ENABLE_VOICE_CHAT
            bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
            if (!isVoiceEnabled)
                return;

            if (keyEvent)
            {
                Qt::KeyboardModifiers modifiers = keyEvent->modifiers();

                Origin::Services::InGameHotKey settingHotKeys(modifiers, keyEvent->key(), keyEvent->nativeVirtualKey());
                Origin::Services::InGameHotKey::HotkeyStatus status = settingHotKeys.status();

                // Bad key blacklist
                if ((keyEvent->key() == Qt::Key_NumLock) ||        // Num Lock
                    (modifiers & Qt::KeypadModifier)               // Any keypad key
                    )
                {
                    status = Origin::Services::InGameHotKey::NotValid_Ignore;
                }

                switch (status)
                {
                case Origin::Services::InGameHotKey::Valid:
                    Services::writeSetting(Services::SETTING_PushToTalkKey, settingHotKeys.toULongLong(), Origin::Services::Session::SessionService::currentSession());
                    Services::writeSetting(Services::SETTING_PushToTalkKeyString, settingHotKeys.asNativeString(), Origin::Services::Session::SessionService::currentSession());
                    break;

                case Origin::Services::InGameHotKey::NotValid_Revert: // If the user put in something bad: e.g. Esc
                    if (Services::isDefault(Services::SETTING_PushToTalkKey) == false)
                    {
                        Services::reset(Services::SETTING_PushToTalkKey);
                        settingHotKeys = Origin::Services::InGameHotKey::fromULongLong(Origin::Services::readSetting(Services::SETTING_PushToTalkKey, Origin::Services::Session::SessionService::currentSession()));
                        Services::writeSetting(Services::SETTING_PushToTalkKeyString, settingHotKeys.asNativeString(), Origin::Services::Session::SessionService::currentSession());
                    }
                    break;

                case Origin::Services::InGameHotKey::NotValid_Ignore:
                default:
                    break;
                }
            }
#endif
        }

        void ClientViewController::handlePushToTalkMouseInput(QMouseEvent* mouseEvent)
        {
#if ENABLE_VOICE_CHAT
            bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
            if (!isVoiceEnabled)
                return;

            if (mouseEvent)
            {
                Qt::MouseButton button = mouseEvent->button();
                if (button)
                {
                    int num = 0;
                    switch (button)
                    {
                    case Qt::LeftButton:
                        num = 1;
                        break;
                    case Qt::MidButton:
                        num = 2;
                        break;
                    case Qt::RightButton:
                        num = 3;
                        break;
                    case Qt::XButton1:
                        num = 4;
                        break;
                    case Qt::XButton2:
                        num = 5;
                        break;
                    }
                    if (num)
                    {
                        QString str = QString("mouse%1").arg(QString::number(num));
                        Services::writeSetting(Services::SETTING_PushToTalkKey, 0, Origin::Services::Session::SessionService::currentSession());
                        Services::writeSetting(Services::SETTING_PushToTalkMouse, str, Origin::Services::Session::SessionService::currentSession());
                        Services::writeSetting(Services::SETTING_PushToTalkKeyString, str, Origin::Services::Session::SessionService::currentSession());
                    }
                }
            }
#endif
        }

        void ClientViewController::onHotkeySettingKeyPress(QKeyEvent* keyEvent)
        {
            mTakenHotkeys.clear();

            // always add push to talk key (when available)
            Services::InGameHotKey settingHotKeys;
            QList < const Origin::Services::Setting *> hotkeySettingsPair;
#if ENABLE_VOICE_CHAT
            settingHotKeys = Services::InGameHotKey::fromULongLong(Services::readSetting(Services::SETTING_PushToTalkKey, Services::Session::SessionService::currentSession()));
            hotkeySettingsPair.append(&Services::SETTING_PushToTalkKey);
            hotkeySettingsPair.append(&Services::SETTING_PushToTalkKeyString);
            mTakenHotkeys[settingHotKeys.asNativeString()] = hotkeySettingsPair;
#endif
            if (mHotkeyInputHasFocus)
            {
                settingHotKeys = Services::InGameHotKey::fromULongLong(Services::readSetting(Services::SETTING_PIN_HOTKEY, Services::Session::SessionService::currentSession()));
                hotkeySettingsPair.clear();
                hotkeySettingsPair.append(&Services::SETTING_PIN_HOTKEY);
                hotkeySettingsPair.append(&Services::SETTING_PIN_HOTKEYSTRING);
                mTakenHotkeys[settingHotKeys.asNativeString()] = hotkeySettingsPair;
                handleHotkeyInput(Services::SETTING_INGAME_HOTKEY, Services::SETTING_INGAME_HOTKEYSTRING, keyEvent);
            }
            else
            {
                if (mPinHotkeyInputHasFocus)
                {
                    settingHotKeys = Services::InGameHotKey::fromULongLong(Services::readSetting(Services::SETTING_INGAME_HOTKEY, Services::Session::SessionService::currentSession()));
                    hotkeySettingsPair.clear();
                    hotkeySettingsPair.append(&Services::SETTING_INGAME_HOTKEY);
                    hotkeySettingsPair.append(&Services::SETTING_INGAME_HOTKEYSTRING);
                    mTakenHotkeys[settingHotKeys.asNativeString()] = hotkeySettingsPair;

                    handleHotkeyInput(Services::SETTING_PIN_HOTKEY, Services::SETTING_PIN_HOTKEYSTRING, keyEvent);
                }
            }
        }

        bool ClientViewController::eventFilter(QObject* obj, QEvent* event)
        {
            bool stopEvent = false;
            switch (event->type())
            {
            case QEvent::MouseButtonPress:
            {
                QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent*>(event);
                if (mouseEvent && mPushToTalkHotKeyHasFocus)
                {
                    mPushToTalkHotKeyHasFocus = false;
                    handlePushToTalkMouseInput(mouseEvent);
                }
            }
            break;
            case QEvent::KeyPress:
            {
                QKeyEvent* keyEvent = dynamic_cast<QKeyEvent*>(event);
                if (keyEvent && (mHotkeyInputHasFocus || mPinHotkeyInputHasFocus) && !keyEvent->isAutoRepeat())
                {
                    // if the user wants to set just the modifier key as the hotkey, we need to set it at release 
                    // otherwise a user trying to set a key with a modifier will, for example for Shift+F1, will set Shift
                    // first as the setting then Shift+F1.  But if another hotkey is set to Shift, then it will swap them incorrectly.
                    // ignore presses that are only modifiers - set modifier-only key settings in the KeyRelease event
                    if (isModifier(keyEvent->key()))
                    {
                        if (mModifierKeyPressed == 0)
                            mModifierKeyPressed = keyEvent->key();  // save the modifier key pressed - 
                        ORIGIN_LOG_EVENT << "eventFilter - KeyPress mod0: " << keyEvent->key() << " mod: " << keyEvent->modifiers() << " mkp: " << mModifierKeyPressed;
                        break;
                    }
                    else
                    //we don't want the escape key to run through the OIG logic as that forces code to revert the hotkey to the default
                    //fixes a bug where hitting ESC after setting a hotkey puts the hotkey back at Shift-f1
                    if (keyEvent->key() != Qt::Key_Escape)
                    {
                        ORIGIN_LOG_EVENT << "eventFilter - KeyPress: " << keyEvent->key() << " mod: " << keyEvent->modifiers() << " mkp: " << mModifierKeyPressed;
                        onHotkeySettingKeyPress(keyEvent);
                    }
                    // IF the user pressed Backspace eat the event. If we don't - the
                    // web page will think we are trying to navigate to the previous URL.
                    if (keyEvent->key() == Qt::Key_Backspace)
                        stopEvent = true;

                    mModifierKeyPressed = 0;    // reset if normal key was pressed
                }
                if (keyEvent && mPushToTalkHotKeyHasFocus)
                {
                    //we don't want the escape key to run through the OIG logic as that forces code to revert the hotkey to the default
                    //fixes a bug where hitting ESC after setting a hotkey puts the hotkey back at Ctrl
                    if (keyEvent->key() != Qt::Key_Escape)
                    {
                        handlePushToTalkHotkeyInput(keyEvent);
                    }
                    // IF the user pressed Backspace eat the event. If we don't - the
                    // web page will think we are trying to navigate to the previous URL.
                    if (keyEvent->key() == Qt::Key_Backspace)
                        stopEvent = true;
                }
            }
            break;
            case QEvent::KeyRelease:
            {
                QKeyEvent* keyEvent = dynamic_cast<QKeyEvent*>(event);
                if (keyEvent && !keyEvent->isAutoRepeat())
                {
                    ORIGIN_LOG_EVENT << "eventFilter - KeyRelease: " << keyEvent->key() << " mod: " << keyEvent->modifiers() << " mkp: " << mModifierKeyPressed;
                    if (mModifierKeyPressed == keyEvent->key())
                    {
                        mModifierKeyPressed = 0;

                        if (mHotkeyInputHasFocus || mPinHotkeyInputHasFocus)
                            onHotkeySettingKeyPress(keyEvent);  // here mainly to catch when user sets a modifier key as the hotkey
                    }
                }
                mPushToTalkHotKeyHasFocus = false;
            }
            break;

            default:
                break;
            }

            return stopEvent ? stopEvent : QObject::eventFilter(obj, event);
        }


    }
}
