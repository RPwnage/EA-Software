//  IGOController.h
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#ifndef IGOCONTROLLER_H
#define IGOCONTROLLER_H

#include <QtCore>
#include <QObject>
#include <QPointer>
#include <QWebPage>

#include "EASTL/vector.h"

#include "originwindow.h"

#include "engine/igo/IIGOCommandController.h"
#include "engine/igo/IGOQWidget.h"
#include "engine/igo/IGOGameTracker.h"
#include "engine/igo/IIGOWindowManager.h"
#ifdef ORIGIN_PC
#include "engine/igo/IGOTwitch.h"
#include <windows.h>
#endif
#include "engine/login/LoginController.h"
#include "engine/content/ContentTypes.h"
#include "services/process/IProcess.h"
#include "services/settings/SettingsManager.h"
#include "services/plugin/PluginAPI.h"
#include "services/exception/ExceptionSystem.h"


// PLEASE READ:
// We are trying to limit the number of scenarios to handle object destructions.
// Here are the 3 separate scenarios we support that would be checked in turn:
// 1) IGO wraps a UIToolkit::OriginWindow:
//      - The destroy is based on the user closing the window -> window->close() is called (or we will call it ourselves when automatically closing a window)
//      - The controller gets a notification (hopefully all those cases have a controller) and kicks off a controller->deleteLater()
//      - In the controller destructor, we call window->deleteLater()
//      - OIG will detect the deleteLater on the window object, and properly remove the associated IGOQWidget && IGOQtContainer
// 2) IGO wraps a QWidget with a controller:
//      - Call controller->deleteLater(), which should handle the proper deletion of its window
//      - Same as previously stated: OIG will detect the deleteLater on embedded window and remove the associated IGOQWidget && IGOQtContainer
// 3) The user has to directly call removeWindow(window, true) which will automatically delete the embedded widget
//      - Remember that when you call removeWindow(window, false), you are responsible for cleanup your own window (but the internal wrappers will be wiped out)
namespace Origin
{
    namespace Chat
    {
        class RemoteUser;
    }
    namespace Engine
    {
        // Fwd decls
        class IGOController;
        class SearchProfileViewController;
        class IGOWindowManager;
        class IGOWindowManagerIPC;
        class IGOTwitch;
#ifdef ORIGIN_MAC
        class IGOTelemetryIPC;
#endif
        
        // Interfaces to the window types that we are able to create using a IIGOUIFactory
        class ORIGIN_PLUGIN_API IViewController
        {
        public:
            // Set of window identifiers already in use - if the client needs specific IDs, it should start from WI_USER_IDS
            enum WindowIdentifiers
            {
                WI_INVALID = -10000,
                WI_BROWSER = -1000, // We can have multiple browsers open at once
                WI_GENERIC = 0,
                WI_SETTINGS,
                WI_CHAT,
                WI_FRIENDS,
                WI_ACHIEVEMENTS,
                WI_CS,
                WI_CUSTOMERCHAT,
                WI_DOWNLOADPROGRESS,
                WI_SEARCH_AND_PROFILE,
                WI_PDLC,
                WI_ERROR,
                WI_GROUPCHAT,
                WI_INVITEFRIENDS,
                WI_KOREANWARNING,
                WI_INTRO,
                WI_BROADCAST,
                WI_CODEREDEMPTION,
                WI_CLOCK,
                WI_TITLE,
                WI_LOGO,
                WI_NAVIGATION,
                WI_SHARED_WINDOW,

                WI_CLIENT_IDS = 1000
            };

        public:
            virtual ~IViewController();
            
            virtual QWidget* ivcView() = 0;
            
            virtual bool ivcPreSetup(IIGOWindowManager::WindowProperties* properties, IIGOWindowManager::WindowBehavior* behavior) = 0;    // automatically called before adding the controller/view to the IGO virtual desktop - returning false will cancel the operation.
            virtual void ivcPostSetup() = 0;   // automatically called after adding the controller/view to the IGO virtual desktop
        };
        
        class IASPViewController
        {
        public:
            virtual QWebPage* createJSWindow(const QUrl& url) = 0;    
        };

        class ORIGIN_PLUGIN_API IGlobalProgressViewController : public IViewController
        {
        public:
            virtual void showWindow() = 0;
            static WindowIdentifiers windowID() { return WI_DOWNLOADPROGRESS; }
        };

        class ORIGIN_PLUGIN_API IDesktopClockViewController : public IViewController
        {
        public:
            static WindowIdentifiers windowID() { return WI_CLOCK; }
        };

        class ORIGIN_PLUGIN_API IDesktopNavigationViewController : public IViewController
        {
        public:
            static WindowIdentifiers windowID() { return WI_NAVIGATION; }
        };

        class ORIGIN_PLUGIN_API IDesktopTitleViewController : public IViewController
        {
        public:
            static WindowIdentifiers windowID() { return WI_TITLE; }
        };

        class ORIGIN_PLUGIN_API IDesktopLogoViewController : public IViewController
        {
        public:
            static WindowIdentifiers windowID() { return WI_LOGO; }
        };
        
        class ORIGIN_PLUGIN_API IWebBrowserViewController : public IViewController
        {
        public:
            enum BrowserFlag
            {
                IGO_BROWSER_SHOW_VIEW       = 0x00,
                IGO_BROWSER_SHOW_NAV        = 0x01,
                IGO_BROWSER_ADD_TAB         = 0x02,
                IGO_BROWSER_ACTIVE          = 0x04, // here to match SDK flags - default behavior when creating a browser (being the new active window)
                IGO_BROWSER_MINI_APP        = 0x08, // ie a simple browser (no tabs, no navigation) used for example by the Sims4 app
                IGO_BROWSER_RESOLVE_URL     = 0x8000,
            };
            
            Q_DECLARE_FLAGS(BrowserFlags, BrowserFlag)
            
        public:
            virtual QWebPage* page() = 0;
            
            virtual int tabCount() const = 0;
            virtual bool isTabBlank(int idx) const = 0;
            virtual void setTabInitialUrl(int idx, const QString& url) = 0;
            virtual bool isAddTabAllowed() const = 0;
            virtual void addTab(const QString& url, bool blank) = 0;
            virtual void showNavigation(bool flag) = 0;
            virtual void setID(const QString& id) = 0;
            virtual QString id() = 0;
            virtual BrowserFlags creationFlags() const = 0;
        };
        
        class ORIGIN_PLUGIN_API IAchievementsViewController : public IViewController
        {
        public:
            virtual void show(Origin::Engine::UserRef user, uint64_t userId, const QString& achievementSet) = 0;
            static WindowIdentifiers windowID() { return WI_ACHIEVEMENTS; }
        };
        
        class ORIGIN_PLUGIN_API ISearchViewController : public IViewController
        {
        public:
            virtual bool isLoadSearchPage(const QString keyword) = 0;
        };
        
        class ORIGIN_PLUGIN_API IProfileViewController : public IViewController
        {
        public:
            virtual void resetNucleusId() = 0;
            virtual quint64 nucleusId() const = 0;
            virtual bool showProfileForNucleusId(quint64 nucleusId) = 0;
        };
        
        class ORIGIN_PLUGIN_API ISocialHubViewController : public IViewController
        {
        public:
            static WindowIdentifiers windowID() { return WI_FRIENDS; }
        };
        
        class ORIGIN_PLUGIN_API ISocialConversationViewController : public IViewController
        {
        public:
            static WindowIdentifiers windowID() { return WI_CHAT; }
        };

        class ORIGIN_PLUGIN_API ISharedSPAViewController : public IViewController
        {
        public:
            virtual void navigate(QString id) = 0;
            static WindowIdentifiers windowID() { return WI_SHARED_WINDOW; }
        };

        class ORIGIN_PLUGIN_API IPDLCStoreViewController : public IViewController
        {
        public:
            virtual QUrl productUrl(const QString& productId, const QString& masterTitleId) = 0;
            
            virtual void show(const Origin::Engine::Content::EntitlementRef entRef) = 0;
            virtual void purchase(const Origin::Engine::Content::EntitlementRef entRef) = 0;
            virtual void showInIGO(const QString& url) = 0;
            virtual void showInIGOByCategoryIds(const QString& contentId, const QString& categoryIds, const QString& offerIds) = 0;
            virtual void showInIGOByMasterTitleId(const QString& contentId, const QString& masterTitleId, const QString& offerIds) = 0;
            static WindowIdentifiers windowID() { return WI_PDLC; }
        };
        
        class ORIGIN_PLUGIN_API ITwitchBroadcastIndicator : public IViewController
        {
        };

        class ORIGIN_PLUGIN_API ISettingsViewController : public IViewController
        {
        public:
            enum Tab
            {
                Tab_GENERAL = 0,
                Tab_NOTIFICATIONS,
                Tab_VOICE,
                Tab_OIG,
            };
            static WindowIdentifiers windowID() { return WI_SETTINGS; }
        };
        
        class ORIGIN_PLUGIN_API ICustomerSupportViewController : public IViewController
        {
        public:
            static WindowIdentifiers windowID() { return WI_CS; }
        };
        
        class ORIGIN_PLUGIN_API IGroupChatViewController : public IViewController
        {
        public:
            static WindowIdentifiers windowID() { return WI_GROUPCHAT; }
        };
        
        class ORIGIN_PLUGIN_API IInviteFriendsToGameViewController : public IViewController
        {
        public:
            static WindowIdentifiers windowID() { return WI_INVITEFRIENDS; }
        };
        
        class ORIGIN_PLUGIN_API IKoreanTooMuchFunViewController : public IViewController
        {
        public:
            static WindowIdentifiers windowID() { return WI_KOREANWARNING; }
        };
        
        class ORIGIN_PLUGIN_API ITwitchViewController : public IViewController
        {
        public:
            virtual IIGOCommandController::CallOrigin startedFrom() const = 0;
            static WindowIdentifiers windowID() { return WI_BROADCAST; }
        };
        
        class ORIGIN_PLUGIN_API IErrorViewController : public IViewController
        {
        public:
            virtual void setView(UIToolkit::OriginWindow* view) = 0;
            virtual void setView(const QString& title, const QString& text, const QString& okBtnText, QObject* okBtnObj, const QString& okBtnSlot) = 0;
            static WindowIdentifiers windowID() { return WI_ERROR; }
        };
        
        class ORIGIN_PLUGIN_API IPinButtonViewController : public IViewController
        {
        public:
            virtual void setPinned(bool pinned) = 0;
            virtual bool pinned() const = 0;
            virtual void setVisibility(bool visible) = 0;
            virtual void setSliderValue(int percent) = 0;
            virtual int  sliderValue() const = 0;

            virtual void raiseSlider() = 0;
            virtual void hideSlider() = 0;
            virtual void showSlider() = 0;
        };
        
        class IWebInspectorController : public IViewController
        {
        public:
            static WindowIdentifiers windowID() { return WI_GENERIC; }
        };
        
        class ORIGIN_PLUGIN_API ICodeRedemptionViewController : public IViewController
        {
        public:
            static WindowIdentifiers windowID() { return WI_CODEREDEMPTION; }
        };

        // Interface to a factory responsible for creating actual windows to be embedded in IGO.
        class ORIGIN_PLUGIN_API IIGOUIFactory
        {
        public:
            virtual ~IIGOUIFactory();
            
            virtual IDesktopTitleViewController* createTitle() = 0;
            virtual IDesktopNavigationViewController* createNavigation() = 0;
            virtual IDesktopClockViewController* createClock() = 0;
            virtual IDesktopLogoViewController* createLogo() = 0;
            virtual IWebBrowserViewController* createWebBrowser(const QString& url, IWebBrowserViewController::BrowserFlags flags) = 0;
            virtual IAchievementsViewController* createAchievements() = 0;
            virtual ISearchViewController* createSearch() = 0;
            virtual IProfileViewController* createProfile(quint64 nucleusId) = 0;
            virtual ISocialHubViewController* createSocialHub() = 0;
            virtual ISocialConversationViewController* createSocialChat() = 0;
            virtual ISharedSPAViewController* createSharedSPAWindow(const QString& id) = 0;
            virtual IPDLCStoreViewController* createPDLCStore() = 0;
            virtual IGlobalProgressViewController* createGlobalProgress() = 0;
            virtual ITwitchBroadcastIndicator* createTwitchBroadcastIndicator() = 0;
            virtual ISettingsViewController* createSettings(ISettingsViewController::Tab displayTab = ISettingsViewController::Tab_GENERAL) = 0;
            virtual ICustomerSupportViewController* createCustomerSupport() = 0;
            virtual IInviteFriendsToGameViewController* createInviteFriendsToGame(const QString& gameName) = 0;
            virtual IKoreanTooMuchFunViewController* createKoreanTooMuchFun() = 0;
            virtual IErrorViewController* createError() = 0;
            virtual IPinButtonViewController* createPinButton(QWidget* window) = 0;
            virtual IWebInspectorController* createWebInspector(QWebPage* page) = 0;
#ifdef ORIGIN_PC
            virtual ITwitchViewController* createTwitchBroadcast() = 0;
#endif           
            virtual Origin::UIToolkit::OriginWindow* createSharedWindow(QSize defaultSize, QSize minimumSize) = 0;
            virtual ICodeRedemptionViewController* createCodeRedemption() = 0;
        };

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // Response once a URL has been "dynamically" resolved/expanded
        class IGOURLUnwinder;
        class ORIGIN_PLUGIN_API IGOURLUnwinderResponse : public QObject
        {
            Q_OBJECT

        signals:
            // Signal a client needs needs to listen to get notified once the URL is ready for us
            void resolved(QString url);

        private slots:
            // Internal callback from requesting a new auth code
            void onAuthCodeRetrieved();
            // Internal callback that checks the current state of the URL/see if all pending queries have been completed
            void checkResolveStatus();

        private:
            IGOURLUnwinderResponse(const QString& url);
            bool resolve();



            QString mUrl;
            struct PendingRequest
            {
                PendingRequest(void* r, int startIdx, int endIdx) : key(r), tokenStartIdx(startIdx), tokenEndIdx(endIdx), completed(false) {}

                void* key;
                int tokenStartIdx;
                int tokenEndIdx;

                bool completed;
                QString value;
            };
            typedef eastl::vector<PendingRequest> PendingRequests;
            PendingRequests mPendingRequests;

            friend IGOURLUnwinder;
        };

        // Helper class to resolve URLs, for example expanding parameters like {game_locale} (static/known on game startup) or
        // {auth_code[ORIGIN_PC]} (dynamic)
        class ORIGIN_PLUGIN_API IGOURLUnwinder : public QObject
        {
            Q_OBJECT

        public:
            // Resolving/expanding URLs is a two-step process:
            // 1) Immediate resolution of parameters that may use data we have on game startup (then we can store the URL in the
            // game shared memory partition and re-use it when we re-attach to a game after Origin closes)
            QString staticResolve(const QString& url, const QString& locale);

            // 2) Resolution at the time we're ready to use the URL, useful for parameters like auth code that are only valid
            // for a few minutes. Using an async pattern also allows us to open up a browser and give the user an immediate feedback something
            // is happening, even if it takes a few seconds to retrieve the data
            IGOURLUnwinderResponse* dynamicResolve(const QString& url);

        public:
            // Extract oig explicit properties from the URL that we can use to tune up the browser to create
            // and return a 'clean' URL
            struct BrowserProperties
            {
                BrowserProperties() : noPinning(false), noRestore(false), centered(false) {}

                QString id;
                QSize size;
                bool noPinning;
                bool noRestore;
                bool centered;
            };
            QString requestedBrowserProperties(const QString& url, BrowserProperties* props);
        };

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // Helper class to manage/recycle the set of visible browsers
        class ORIGIN_PLUGIN_API WebBrowserManager : public QObject, public IIGOWindowManager::IWindowListener
        {
            Q_OBJECT
            
        public:
            WebBrowserManager(IGOController* controller, IIGOUIFactory* factory, IIGOWindowManager* windowManager);
            ~WebBrowserManager();
            
            IWebBrowserViewController* createWebBrowser(const QString& url);
            IWebBrowserViewController* createWebBrowser(const QString& url, IWebBrowserViewController::BrowserFlags flags, const IIGOWindowManager::WindowProperties& settings, const IIGOWindowManager::WindowBehavior& baseBehavior);

        public slots:
            // IWindowListener impl
            virtual void onFocusChanged(QWidget* window, bool hasFocus);

        private slots:
            virtual void onURLResolved(QString url);

        private:
            IGOController* mController;
            IIGOUIFactory* mUIFactory;
            IIGOWindowManager* mWindowManager;

            bool eventFilter(QObject* obj, QEvent* evt);
            void onWebBrowserDestroyed(QObject* object);

            // list of open web browsers
            typedef eastl::vector<IWebBrowserViewController*> WebBrowserList;
            WebBrowserList mWebBrowserList;
            
            // last or currently in-focus web browser
            IWebBrowserViewController* mCurrentWebBrowser;

            // Pending URL expansions
            struct PendingURL
            {
                IGOURLUnwinderResponse* key;

                IWebBrowserViewController* browser;
                int tabIdx;
            };
            typedef eastl::vector<PendingURL> PendingURLs;
            PendingURLs mPendingURLs;
        };
        
        // Helper class to use a single window to show different content from the client space (for example Search/Profile views)
        class ORIGIN_PLUGIN_API SharedViewController : public QObject, public IViewController
        {
            Q_OBJECT
            
        public:
            SharedViewController(QObject* parent = 0);
            virtual ~SharedViewController();
        
            virtual QWidget* ivcView() { return mWindow; }
            virtual bool ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior) { return true; }
            virtual void ivcPostSetup() {}

        protected:
            void setWindow(Origin::UIToolkit::OriginWindow* window);
            
            void add(int cid, IViewController* controller);
            IViewController* get(int cid);
            void show(int cid);
            
        private:
            typedef eastl::vector<eastl::pair<int, IViewController*>> ManagedControllers;
            ManagedControllers mControllers;
            
            QPointer<Origin::UIToolkit::OriginWindow> mWindow;
        };
        
        // Specialization of SharedViewController to manager Search and Profile pages
        class ORIGIN_PLUGIN_API SearchProfileViewController : public SharedViewController
        {
            Q_OBJECT
            
        public:
            SearchProfileViewController(QObject* parent=0);
            ~SearchProfileViewController();
            
            static WindowIdentifiers windowID() { return WI_SEARCH_AND_PROFILE; }
            
            void addSearch(ISearchViewController* controller) { add(ControllerID_SEARCH, controller); }
            void addProfile(IProfileViewController* controller) { add(ControllerID_PROFILE, controller); }
            
            ISearchViewController* search() { return static_cast<ISearchViewController*>(get(ControllerID_SEARCH)); }
            IProfileViewController* profile() { return static_cast<IProfileViewController*>(get(ControllerID_PROFILE)); }
            
            void showSearch() { show(ControllerID_SEARCH); }
            void showProfile() { show(ControllerID_PROFILE); }
            
        private:
            enum ControllerID
            {
                ControllerID_NONE = 0,
                ControllerID_SEARCH,
                ControllerID_PROFILE
            };
        };
        
        // Handles the initial notification that IGO is available (user should have focus on the game + the resolution must follow the
        // proper restriction)
        class ORIGIN_PLUGIN_API IGOAvailableNotifier : public QObject
        {
            Q_OBJECT
            
        public:
            IGOAvailableNotifier(IGOWindowManager* igowm, IGOGameTracker* tracker, uint32_t gameId);
            ~IGOAvailableNotifier();
            
        private slots:
            void onGameRemoved(uint32_t gameId);
            void onScreenSizeChanged(uint32_t width, uint32_t height, uint32_t gameId);
            
        private:
            IGOWindowManager* mWindowManager;
            IGOGameTracker* mTracker;
            uint32_t mGameId;
        };

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef ORIGIN_PC
        // Run the IGOProxy/IGOProxy64 exe multiple times to collect offsets from the different graphics API we support so 
        // that IGO can directly access COM objects 
        class OIGOffsetsLookupWorker : public QObject
        {
            Q_OBJECT

            public:
                static OIGOffsetsLookupWorker* createWorker(QString appPath, QString exePath, QString apiOption);
                void start();

            signals:
                void finished();

            private slots:
                void started();
                void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
                void onProcessError(QProcess::ProcessError error);

            private:
                OIGOffsetsLookupWorker(QThread* thread, QString appPath, QString exePath, QString apiOption);
                OIGOffsetsLookupWorker(const OIGOffsetsLookupWorker&);
                ~OIGOffsetsLookupWorker();

                QThread* mThread;
                QString mAppPath;
                QString mExePath;
                QString mApiOption;

                volatile bool mDone;
        };
#endif
        
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        
        /// \brief Defines the API for creating and ending the IGO system.
        class ORIGIN_PLUGIN_API IGOController : public QObject, public Origin::Engine::IIGOCommandController
        {
            Q_OBJECT

            friend class IGOWindowManager;

        public:
            // Little helper for sharing code when in need of a simple callback "injection"
            template<typename T> class ORIGIN_PLUGIN_API ICallback
            {
            public:
                virtual void operator() (T* controller) {};
            };
                     
            // Instance handling
        public:
            static bool instantiated(); // only used when the user immediately closes the client(never logs in) - otherwise there should always be an instance available
            static IGOController* instance();
            
            static void init(IIGOUIFactory* factory);
            static void release();

            // start process and injects IGO dll
            void startProcessWithIGO(Origin::Services::IProcess *process, const QString& gameProductId, 
                const QString& gameTitle, const QString& gameMasterTitle, const QString& gameFolder, 
                const QString& gameLocale, const QString& executePath, bool forceKillAtOwnershipExpiry, 
                bool isUnreleased, const qint64 timeRemaining, QString achievementSetID, const QString& IGOBrowserDefaultURL);

            // Unload the IGO dll from the client
            void unloadIGODll();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            // Common state setters/queries
        public:
            // Client-side "enhanced" logging triggered from Debug menu to show IGO object lifecycle
            void setExtraLogging(bool flag) { mExtraLogging = flag; }
            bool extraLogging() const { return mExtraLogging; }
            
            // Show web inspectors for Origin windows with web content
            void setShowWebInspectors(bool flag) { mShowWebInspectors = flag; }
            bool showWebInspectors() const { return mShowWebInspectors; }
            

            QString hotKey() const;
            QString pinHotKey() const;
			void setOriginKeyboardLanguage(void *language);
            
            // those are static, so we can use them even if no user has been logged in
            // for example in the SDK_ServiceArea, when a non RTP game which uses the SDK is started, the SDK connections are made before any user has been logged in!
            static bool isEnabled();
            static bool isEnabledForAllGames();
            static bool isEnabledForSupportedGames();

            bool isActive() const;
            bool isVisible() const;
            bool isConnected() const;
            bool isAvailable() const;
            
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            // Access to managed control components
        public:
            // Returns current window manager that handles IGO windows.
            IIGOWindowManager* igowm() const { return mWindowManager; }
            
            // Returns current UI factory used to create the embedded/core windows.
            IIGOUIFactory* uiFactory() const { return mUIFactory; }
            
            // Returns game data tracker
            IGOGameTracker* gameTracker() const { return mGameTracker; }

#ifdef ORIGIN_PC
            // Returns the twitch controller (engine)
            IGOTwitch* twitch() const { return mTwitch; }
#endif           
            IGOURLUnwinder* urlUnwinder() const { return mURLUnwinder; }

            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            
            // Game-side control
        public:
            void setIGOEnable(bool enable);
            void setIGOStressTest(bool enable);
            void setIGODebugMenu(bool eanble);
            void setIGOEnableLogging(bool enable);
            bool igoEnableLogging() const;
            void signalIGO(uint32_t processId);

            void resetHotKeys();
            
            void putGameIntoBackground();
            bool isGameInForeground() const;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            // Support for window creation from SPA
        public:
            QWebPage* createJSWindow(const QUrl& url);
            QUrlQuery createJSRequestParams(IViewController* controller);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            // Windows creation
        public:
            void setupDesktop(Origin::Services::Exception::ExceptionSystem* exceptionSystem);
            QRect desktopWorkArea();
#ifdef ORIGIN_PC
            IIGOCommandController::CallOrigin twitchStartedFrom() const; // unfortunate helper to distinguish whether Twitch was started from the SDK (to help with closing IGO automatically)
#endif           
            IWebBrowserViewController* createWebBrowser(const QString& url);
            IWebBrowserViewController* createWebBrowser(const QString& url, IWebBrowserViewController::BrowserFlags flags, const IIGOWindowManager::WindowProperties& settings, const IIGOWindowManager::WindowBehavior& behavior);
            
            IPDLCStoreViewController* createPdlcStore(ICallback<IPDLCStoreViewController>* callback, CallOrigin callOrigin);

            void alert(Origin::UIToolkit::OriginWindow* window);
            void showWebInspector(QWebPage* page);

            IIGOWindowManager::AnimPropertySet defaultOpenAnimation();
            IIGOWindowManager::AnimPropertySet defaultCloseAnimation();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            // SDK API - Do not call these from the Origin client code as the behavior is different compared
            // to when the functionality is called from the SDK (for example close IGO when closing the window)
        public:
            int32_t EbisuShowIGO(bool visible);
            int32_t EbisuShowProfileUI(uint64_t user);
            int32_t EbisuShowFriendsProfileUI(uint64_t user, uint64_t target);
            int32_t EbisuShowRecentUI(uint64_t user);
            int32_t EbisuShowFeedbackUI(uint64_t user, uint64_t target);
            int32_t EbisuShowBrowserUI(IWebBrowserViewController::BrowserFlags flags, const char* url);
            int32_t EbisuShowCheckoutUI(const char* pURL);
            int32_t EbisuShowStoreUIByCategoryIds(const QString& gamerId, const QString& contentId, const QString& categoryIds, const QString& offerIds);
            int32_t EbisuShowStoreUIByMasterTitleId(const QString& gamerId, const QString& contentId, const QString& masterTitleIds, const QString& offerIds);
            int32_t EbisuShowFriendsUI(uint64_t user);
            int32_t EbisuShowFindFriendsUI(uint64_t user);
            int32_t EbisuRequestFriendUI(uint64_t user, uint64_t target, const char* source);
            int32_t EbisuShowComposeChatUI(uint64_t user, const uint64_t* pUserList, uint32_t iUserCount, const char* pMessage);
            
            int32_t EbisuShowInviteUI();
            int32_t EbisuShowAchievementsUI(uint64_t userId, uint64_t personaId, const QString& achievementSet, bool showIgo, CallOrigin callOrigin);
            int32_t EbisuShowSelectAvatarUI(uint64_t user);
            int32_t EbisuShowGameDetailsUI(const QString& productId, const QString& masterTitleId="");
            int32_t EbisuShowBroadcastUI();
            int32_t EbisuShowReportUserUI(uint64_t user, uint64_t target, const QString& masterTitle);
            int32_t EbisuShowCodeRedemptionUI();
            int32_t EbisuShowErrorUI(const QString& title, const QString& text, const QString& okBtnText = "", QObject* okBtnObj = NULL, const QString& okBtnSlot = "");
            
            int32_t showError(const QString& title, const QString& text, const QString& okBtnText = "", QObject* okBtnObj = NULL, const QString& okBtnSlot = "", IIGOCommandController::CallOrigin callOrigin = IIGOCommandController::CallOrigin_CLIENT);

            bool isBroadcasting();
            bool isBroadcastingPending();
            bool isBroadcastTokenValid();
            bool isBroadcastingBlocked();
            void setBroadcastOriginatedFromSDK(bool flag);
            
        public slots: // Pretty much SDK implementation / internally called from commands (ie top/bottom bars)
            void igoShowSearch(const QString& keyword, Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
            void igoShowProfile(quint64 nucleusId, Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
            void igoShowFriendsList(Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
            void igoShowChatWindow(Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
            void igoShowSharedWindow(const QString& id, Origin::Engine::IIGOCommandController::CallOrigin callOrigin);

            void igoShowSettings(Engine::ISettingsViewController::Tab displayTab = ISettingsViewController::Tab_GENERAL);
            void igoShowCustomerSupport();
            void igoShowInviteFriendsToGame(const QString& gameName, CallOrigin callOrigin);
            void igoShowAchievements(uint64_t userId, uint64_t personaId, const QString &achievementSet, Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
            void igoShowError(Origin::UIToolkit::OriginWindow* window);
            void igoShowError(const QString& title, const QString& text, const QString& okBtnText, QObject* okBtnObj, const QString& okBtnSlot, Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
            void igoShowDownloads();
            void igoShowBroadcast();
            void igoShowStoreUI(Origin::Engine::Content::EntitlementRef entRef);
            void igoShowCheckoutUI(Origin::Engine::Content::EntitlementRef entRef);
            void igoShowCodeRedemption(Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
            void igoHideUI();

            bool attemptBroadcastStart(Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
            bool attemptBroadcastStop(Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
            
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        signals:
            void showMyProfile(Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
            void showFriendsProfile(qint64 user, Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
            void showAvatarSelect(Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
            void showFindFriends(Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
            void showChat(QString from, QStringList to, QString message, Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
            void showBrowser(const QString& url, bool useTabs); // helper that creates a browser/new tab on the main thread + maximize the window automatically
            void showBroadcastSettings();
            void requestGoOnline();

            void broadcastStarted(const QString& broadcastUrl);
            void broadcastStopped();

            void openingWebBrowser();
            void closingWebBrowser();

            void stateChanged(bool visible);
            void hotKeyChanged(QString key);
            
            void igoStop(); // no more games are running
            bool igoCommand(int cmd, Origin::Engine::IIGOCommandController::CallOrigin callOrigin); // from IGOFlow
            void igoShowToast(const QString& toasteeSetting, const QString& title, const QString& message); // from IGOFlow
            void igoShowOpenOIGToast();
            void igoShowTrialWelcomeToast(const QString& title, const qint64& timeRemaining);
            void showReportUser(qint64 user, const QString& masterTitleId, Origin::Engine::IIGOCommandController::CallOrigin callOrigin);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        private slots:
            void igoShowBrowser(const QString& url, bool useTabs);
            void igoShowBrowserForGame();
            void igoStateChanged(bool visible, bool sdkCall); // from IGOWindowManager when IGO is switching on/off
            void igoShowWebInspector(QWebPage* page);
            
            // Notifications from IGOGameTracker
            void onGameAdded(uint32_t gameId);
            void onGameFocused(uint32_t gameId);
            void onGameRemoved(uint32_t gameId);
            
            void onUserLoggedIn(Origin::Engine::UserRef user);
            void onUserLoggedOut(Origin::Engine::UserRef user);

            void onSettingChanged(const QString& name, const Origin::Services::Variant& value);
            
            void onEntitlementsLoaded();

#ifdef ORIGIN_PC
            // Notifications associated with the pre-computation of OIG offsets by IGOProxy
            void onOIGOffsetsComputed();
#endif            
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        private:
            IGOController(IIGOUIFactory* factory);
            ~IGOController();

            IGOController(IGOController const& from);
            IGOController& operator=(IGOController const& from);
            
            void removeOldLogs();

#ifdef ORIGIN_PC
            bool isIGODLLValid(QString igoName);
            void dispatchOffsetsLookup();
#endif

            IGOWindowManager* WindowManager() const;
            WebBrowserManager* WBManager();
            SearchProfileViewController* SPViewController(CallOrigin callOrigin);
            IErrorViewController* ErrorController(ICallback<IErrorViewController>* callback, CallOrigin callOrigin);
            bool isIGOWindow(const QUrl&, int32_t&);
            
            //
            
            enum ControllerWindowType
            {
                ControllerWindowType_NORMAL,
                ControllerWindowType_POPUP,
            };
            
            template<typename T> class ORIGIN_PLUGIN_API IControllerFunctor
            {
            public:
                virtual ~IControllerFunctor() {}
                virtual T* operator() () const = 0;
            };
            
            template<typename T> class ORIGIN_PLUGIN_API ControllerFunctor : public IControllerFunctor<T>
            {
            public:
                ControllerFunctor(T* controller) : mController(controller) {}
                virtual T* operator() () const { return mController; }
                
            private:
                T* mController;
            };
            
            template<typename T, typename TArg> class ORIGIN_PLUGIN_API ControllerFunctorNew : public IControllerFunctor<T>
            {
            public:
                ControllerFunctorNew(TArg arg) : mArg(arg) {}
                virtual T* operator() () const { return new T(mArg); }
                
            private:
                TArg mArg;
            };
            
            template<typename T> class ORIGIN_PLUGIN_API ControllerFunctorFactoryNoArg : public IControllerFunctor<T>
            {
            public:
                typedef T* (IIGOUIFactory::*CreateFct)();

                ControllerFunctorFactoryNoArg(CreateFct create) : mCreate(create) {}
                virtual T* operator() () const { return (IGOController::instance()->uiFactory()->*mCreate)(); }
                
            private:
                CreateFct mCreate;
            };

            template<typename T, typename TArg> class ORIGIN_PLUGIN_API ControllerFunctorFactoryOneArg : public IControllerFunctor<T>
            {
            public:
                typedef T* (IIGOUIFactory::*CreateFct)(TArg arg);
                
                ControllerFunctorFactoryOneArg(CreateFct create, TArg arg) : mCreate(create), mArg(arg) {}
                virtual T* operator() () const { return (IGOController::instance()->uiFactory()->*mCreate)(mArg); }
                
            private:
                CreateFct mCreate;
                TArg mArg;
            };

            template<class T> T* createController(enum ControllerWindowType winType, const IControllerFunctor<T>& functor
                                                  , IIGOWindowManager::WindowProperties* settings, IIGOWindowManager::WindowBehavior* behavior
                                                  , ICallback<T>* callback = NULL);
        
            //
            
            IIGOUIFactory* mUIFactory;
            IGOGameTracker* mGameTracker;
            IIGOWindowManager* mWindowManager;
            IGOWindowManagerIPC* mIPCServer;
#if defined(ORIGIN_MAC)
            IGOTelemetryIPC* mTelemetryServer;
#elif defined(ORIGIN_PC)
            IGOTwitch* mTwitch;
#endif
            WebBrowserManager* mWebBrowserManager;
            IGOURLUnwinder* mURLUnwinder;
            Origin::Services::Exception::ExceptionSystem* mExceptionSystem;
            
            IDesktopClockViewController* mClock;
            IDesktopTitleViewController* mTitle;
            IDesktopLogoViewController* mLogo;
            IDesktopNavigationViewController* mNavigation;
            IGlobalProgressViewController* mGlobalProgress;
            IKoreanTooMuchFunViewController* mKoreanController;
#ifdef ORIGIN_PC
            ITwitchBroadcastIndicator* mTwitchBroadcastIndicator;
            ITwitchViewController* mTwitchViewController;
            typedef eastl::hash_map<uint32_t, HANDLE> SDKSignalMap;
            SDKSignalMap mSDKSignals;
#endif
            ///

            struct ORIGIN_PLUGIN_API IGOGameInfo // Internal info per game
            {
                explicit IGOGameInfo()
                {
                    welcomedUser = false;
                    notifyIGOAvailable = false;
                }
                
                bool welcomedUser;          // did we show the initial welcome dialog? (can be disabled from preferences)
                bool notifyIGOAvailable;    // did we show the initial dialog/notification that IGO is now available in the game?
            };
            typedef eastl::hash_map<uint32_t, IGOGameInfo> IGOGameInstances;
            IGOGameInstances mGames;

            ///

            bool mDesktopSetupCompleted;
            bool mUserIsLoggedIn;
            bool mExtraLogging;
            bool mShowWebInspectors;
#ifdef ORIGIN_PC
			volatile int mOffsetsLookupCount;
            int mMaxOffsetsLookupCount;
#endif
        };
    }
}

// Interfaces
Q_DECLARE_INTERFACE(Origin::Engine::IWebBrowserViewController, "Origin.IWebBrowserViewController")


#endif // IGOCONTROLLER_H
