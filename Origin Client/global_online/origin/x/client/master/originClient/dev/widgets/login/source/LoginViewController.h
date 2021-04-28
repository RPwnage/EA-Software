/////////////////////////////////////////////////////////////////////////////
// LoginViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef LOGIN_VIEW_CONTROLLER_H
#define LOGIN_VIEW_CONTROLLER_H

#include <QObject>

#include "OriginApplication.h"
#include "FlowResult.h"
#include "originbanner.h"
#include "PageErrorDetector.h"

#include "services/network/NetworkCookieJar.h"
#include "services/settings/Setting.h"
#include "services/rest/OriginServiceValues.h"
#include "services/plugin/PluginAPI.h"

#include <QNetworkCookie>
#include <QNetworkReply>
#include <QTimer>

namespace Origin
{
    namespace UIToolkit
    {
        class OriginWindow;
    }
    namespace Client
    {
        class OriginWebController;

        class ORIGIN_PLUGIN_API LoginViewController : public QObject
        {
            Q_OBJECT

        public:
            LoginViewController(QWidget *parent = 0, const QString& ssoToken= QString());
            ~LoginViewController();

            void init(const StartupState& startupState, UIToolkit::OriginBanner::IconType bannerIcon = UIToolkit::OriginBanner::IconUninitialized,
                const QString& bannerText = "");

            void resetLoginState(bool clearUserName /*= true*/);

            /// \brief Don't allow the user to see the login window until the offline login web application cache has downloaded.
            /// If the user is logged in prior to the cache completing download, the cache will not be created, making offline login impossible.
            void validateOfflineCacheAndShow();

            void show(const StartupState& startupState = StartNormal);
            void showBanner(UIToolkit::OriginBanner::IconType icon, const QString& text);

            bool isLoginWindowVisible();

            bool autoLoginInProgress() { return mAutoLoginInProgress;}     //checks to see if we're in the middle of auto-login

            QString loginUrl(bool getOfflineUrl = false);
            void clear();

            void showErrorMessage(const QString& errorMessage);

            /// fn saveRememberMeCookie
            /// \brief saves off the remember me cookie and the userID associated with that cookie into settings
            void saveRememberMeCookie (const QString userID, bool syncSettingsFile = true);

            /// fn clearRememberMeCookie
            /// \brief clears the saves off the remember me cookie and the userID associated with that cookie into settings
            void clearRememberMeCookie ();

            /// fn setShowAgeUpFlow
            /// \brief accessor to set the member var to bypass Age Up Flow during login
            void setShowAgeUpFlow (bool show) const;

            QString mLoginType;
            bool mAutoLogin;

            /// \brief Gets OEM information.
            /// \param tag This function will populate this parmeters.  TBD.
            void getOemInformation(QString& tag) const;

            enum OfflineCacheType
            {
                Exists = 0,
                Missing,
                Corrupt
            };

            enum ClientCookieErrorType
            {
                ClientCookieError_None = 0,
                ClientCookieError_Expired,
                ClientCookieError_ExpirationInvalid,
                ClientCookieError_CookieInvalid
            };

            static OfflineCacheType validateOfflineWebApplicationCache();

        signals:

            void userCancelled();
            void loginComplete(const QString& authorizationCode, const QString& idToken);
            void typeOfLogin(const QString& loginType, bool autoLogin);
            void offlineLoginComplete(const QString& primaryOfflineKey, const QString& backupOfflineKey);
            void loadCanceled();
            void loginError();

            // Emitted if an underage user has just completed the age up flow
            void ageUpComplete();

        public slots:

            void closeLoginWindow();

        protected slots:
            void onOnlineLoginComplete(const QString& authorizationCode, const QString& idToken);
            void onOfflineLoginComplete(const QString& primaryCid, const QString& backupCid);
            void loadLoginPage(bool forceOfflineLogin = false);
            void onUserClosedWindow();
            void onLoadErrorFinished(bool);
            void onRequestFinished(QNetworkReply*);
            void onErrorDetected(QNetworkReply::NetworkError, QNetworkReply*);
            void onRedirectionLimitReached();
            /// \brief Opens default browser when password url is received
            void onChangePasswordUrl ( const QString& );
            
            /// \brief connected to signal from js helper when ageup flow is started
            void onAgeUpFlowBegin (const QString& executeJSstmt);
            void onAgeUpFlowCanceled ();
            void onAgeUpFlowComplete ();

            /// \brief connected to signal from js helper when registration flow started
            void onRegistrationStepBegin (const QString&);
            void onRegistrationStepCanceled (const QString&);
            void onRegistrationStepComplete(const QString&);

            /// \brief detects URL change in login window. Used to establish bridge between client and forgot_password.html's JS
            void onUrlChanged(const QUrl&);

            /// \brief sets the cache load control to preferNetwork on the network access manager
            void resetCacheLoadControl();

            /// \brief called when a webpage has completed loading
            void onPageLoaded();

            /// \brief Establishes bridge between client and forgot_password.html's JS
            void addJSObject();
            void onLinkClicked(const QUrl &);

            /// \brief called when an authentication error was detected
            void onAuthenticationError(QString error);

            /// \brief Called when the login page takes too long to load
            void onLoadTimeout();

            /// \brief Called after QWebPage finishes canceling login page load after a timeout
            void onTriggerActionWebpageStop(QNetworkReply* reply);

            /// \brief Puts login into offline mode if it takes too long to load the login page, or displays a fatal error message if a timeout occurs during offline mode as well
            void processLoadCanceled();

            /// \brief connected to when user closes the window during age up flow
            void onAgeUpWindowClosed ();

            /// \brief slot called when connection change is detected
            void onConnectionChange (bool online);

            /// \brief If the client goes offline during ageUp, loads the login page using the offline url. 
            void onConnectionChangeDuringAgeUp(bool online);

            /// \brief If the client goes offline during registration, loads the login page using the offline url. 
            void onConnectionChangeDuringRegistration(bool online);

            /// \brief If a popup window is showing, close it.
            void closePopupWindow();

            /// \brief used to check if the offline login web application cache has finished downloading
            void pollOfflineCacheStatus();

            /// \brief slot called after the initial load of the login window page content
            void onInitialPageLoad();

            /// \brief used to minimize the window using a queued connection
            void onShowMinimized();

            void onSystemMenuExit();

        private:

            static const QString sOfflineLoginUrlKey;
            static const int sLoginRefreshTimeout;
            static const int sLoginLoadTimeout;
            static const unsigned sOfflineCacheDownloadTimeout;

            Services::NetworkCookieJar* mCookieJar;
            OriginWebController *mAuthenticationWebController;
            UIToolkit::OriginWindow* mLoginWindow;
            UIToolkit::OriginWindow* mPopupWindow;
            QTimer mLoginFlowIdRefreshTimer;
            QTimer mLoginLoadTimer;
            
            /// \brief SSO Token from command line for RTP.
            QString mSsoToken;
            QString mBannerText;
            UIToolkit::OriginBanner::IconType mBannerIconType;
            QString mCurrentRememberMeCookie;
            QString mCurrentUsernameCookie;
            QString mCurrentTFAIdCookie;
            bool mIsNux;
            bool mIsChangePassword;
            bool mLoggingInOffline;
            bool mInitialTelemetrySent;
            OfflineCacheType mOfflineCacheStatus;

            unsigned mCacheValidationSecondsLeft;
            StartupState mStartupState;

            bool mOnlineLoginPageLoadSuccess;

            QString mExecuteJSstatementToSkipAgeUp;

            bool mAutoLoginInProgress;        //to determine whether we're in the process of loading accounts.ea.com with remid

            void settingsFileSyncHelper();
            void addLoginUrlQueryParameters(QUrl& url, bool isOfflineUrl) const;
            void addBannerQueryParameters(QUrl& url) const;
            QString bannerTypeStr(UIToolkit::OriginBanner::IconType type) const;
            void addOEMQueryParameter (QUrl & url) const;
            void addAgeUpQueryParameter (QUrl& url) const;
            void addMachineIdParameter(QUrl & url) const;
            void initiateCancelLoad();

            QString offlineLoginUrl() const;
            QString onlineUrl() const;

            //cookie management
            void persistCookie(const QString& name, QString& cookieValue, const Services::Setting& settingKey, const QString& invalidSettingValue);
            void loadPersistentCookies();
            void loadPersistentCookie(const QString& cookieName, QString& cookieValue, const Services::Setting& settingKey, const QString& invalidSettingValue);

            void clearCookieIfStale (const QString& name, QString& cookieValue, Origin::Services::NetworkCookieJar::cookieElementType byType, const Services::Setting& settingKey, bool forceClearCookie);

            /// \brief Show a fatal error message in the login window.
            void showFatalErrorDialog(const QString& errorMessage);

            /// \brief Send telemetry about the type of login the client is about to attempt.
            void sendLoginTypeTelemetry();

            /// \brief Creates a popup window and displays a specified url. 
            void showPopupDialog(const QUrl& url);

            void sendAuthenticationErrorTelemetry(QString error, const QString& requestUrl = "", int httpResponseCode = 0,
                QNetworkReply::NetworkError qError = QNetworkReply::NoError, int errorSubType = 0, const QString& qtErrorString = "");

            QString generateTelemtryUrlWithParamInfo(const QUrl &url);

            /// \brief If the file does not exist, copies the offline web application cache from resources 
            void initalizeWebApplicationCache();

            /// \brief Stores the url that will be used when loading the login page from the offline web application cache.
            void saveOfflineUrlToSettings(const QUrl& onlineUrl);

            /// \brief separate function to log cookie errors on the client side -- named such because we don't want to log any mention of "cookie"
            void reportLoadPersistanceError (const QString& cookieName, ClientCookieErrorType cookieError);
            void setupSystemTray();
        };
    }
}
#endif // LOGIN_VIEW_CONTROLLER_H
