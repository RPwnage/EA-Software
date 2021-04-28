/////////////////////////////////////////////////////////////////////////////
// LoginViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include <QDesktopServices>
#include <QtWebKitWidgets/QWebView>

#include "LoginViewController.h"
#include "LoginNetworkAccessManager.h"
#include "RTPFlow.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "originwindow.h"
#include "originwindowmanager.h"
#include "originwebview.h"

#include "OriginApplication.h"
#include "services/network/GlobalConnectionStateMonitor.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/config/OriginConfigService.h"
#include "TelemetryAPIDLL.h"
#include "OriginWebController.h" 
#include "services/settings/SettingsManager.h"
#include "services/rest/OriginServiceMaps.h"
#include "LoginWebPage.h"
#include "LoginSystemMenu.h"

#include <QDesktopWidget>
#include <QLayout>

#include "AuthenticationJsHelper.h"
#include "WebWidget/FeatureRequest.h"
#include "services/heartbeat/Heartbeat.h"

#include <qnetworkaccessmanager.h>
#include <qnetworkcookiejar.h>

#include <QDesktopWidget>
#include <QtWebKit/QWebHistory>
#include <QtWebKit/QWebHistoryItem>

#ifdef ORIGIN_MAC
#include "MainMenuControllerOSX.h"
#endif

#ifdef WIN32
#include <ShlObj.h>
#include "services/crypto/CryptoService.h"
#include "SystemInformation.h"
#include <QFile>
#endif


#include <QtWebKitWidgets/qwebframe.h>
#include <QtWebKitWidgets/qwebpage.h>

// Legacy
#include "EbisuSystemTray.h"
#include "CommonJsHelper.h"

#include "services/network/NetworkAccessManagerFactory.h"
#include <QDesktopWidget>

#include "services/session/SessionService.h"        //for cookie name definitions

#include "TelemetryManager.h"
#include "SystemInformation.h"


using namespace Origin::Services;

namespace
{
    /// \brief Error code and "description" text delimiter when we're expecting additional
    /// error info when login errors occur.
    const QString ADDITIONAL_INFO_DELIMITER("_");

    /// \brief Returns the "error description" text from a login error string.
    ///
    /// For example, given the following error string:
    /// "10003_someErrorText"
    ///
    /// Assuming underscore is the additional info delimiter, "someErrorText"
    /// is returned by this function. If no additional error text is included with the
    /// given error string, a "null" QString object is returned.
    ///
    /// \return QString containing the additional error text of the given error string.
    /// If the expected additional info delimiter isn't found, a "null" QString 
    /// object is returned.
    QString parseAdditionalErrorInfo(const QString& errorString)
    {
        int delimIndex = errorString.indexOf(ADDITIONAL_INFO_DELIMITER);
        static const int NOT_FOUND = -1;

        if (delimIndex == NOT_FOUND)
        {
            return QString();
        }

        return errorString.right(errorString.size() - delimIndex - 1);
    }

    /// \brief Parses and returns the error code of the given login error string.
    ///
    /// For example, given the following string:
    /// "10003_someErrorText"
    ///
    /// Assuming underscore is the additional info delimiter, "10003" is returned
    /// by this function. If no additional error text is provided, the error code
    /// is still returned. For example, given the following string:
    /// "10003"
    ///
    /// This function will return "10003".
    ///
    /// \return The error code of the given login error string. If an error occurs
    /// during parsing, a copy of the given string is returned.
    QString parseErrorCode(const QString& errorString)
    {
        int delimIndex = errorString.indexOf(ADDITIONAL_INFO_DELIMITER);
        return errorString.left(delimIndex);
    }
}

namespace Origin
{
    using namespace UIToolkit;
    namespace Client
    {
        const QString LoginViewController::sOfflineLoginUrlKey = "OfflineLoginUrl";

        // The amount of time before EADPs login flow id expires in min.
        const int LoginViewController::sLoginRefreshTimeout = 70;

        // amount of time we wait for the page to load before aborting
        // since the login page is a series of redirects, Qt resets the timer with each request
        const int LoginViewController::sLoginLoadTimeout = 2 * 60 * 1000;   //2 minutes

        // the amount of time we wait for the offline login cache to download before showing
        // the login page
        const unsigned LoginViewController::sOfflineCacheDownloadTimeout = 60; // secs

        LoginViewController::LoginViewController(QWidget *parent, const QString& ssoToken/*= QString()*/)
            : QObject(parent)
            , mLoginType("none")
            , mAutoLogin(false)
            , mCookieJar(NULL)
            , mAuthenticationWebController(NULL)
            , mLoginWindow(NULL)
            , mPopupWindow(NULL)
            , mSsoToken(ssoToken)
            , mBannerIconType(OriginBanner::IconUninitialized)
            , mIsNux(false)
            , mIsChangePassword(false)
            , mLoggingInOffline(false)
            , mInitialTelemetrySent(false)
            , mOfflineCacheStatus (Exists)	//assume success
            , mCacheValidationSecondsLeft(0)
            , mStartupState(StartNormal)
            , mOnlineLoginPageLoadSuccess(false)
            , mExecuteJSstatementToSkipAgeUp ("")
            , mAutoLoginInProgress (false)
        {
        }


        LoginViewController::~LoginViewController()
        {
            closeLoginWindow();
            closePopupWindow();
            EbisuSystemTray::instance()->setTrayMenu(NULL);
        }

        void LoginViewController::clear()
        {
            
        }

        void LoginViewController::initalizeWebApplicationCache()
        {
            QString offlineCachePath = QWebSettings::offlineWebApplicationCachePath();
            QString offlineCache (offlineCachePath + QDir::separator() + "ApplicationCache.db");

            if (validateOfflineWebApplicationCache() != Exists)
            {
                #if ORIGIN_PC
                    QString webApplicationCacheResource = ":Login/Windows/ApplicationCache.db";
                #else //ORIGIN_MAC
                    QString webApplicationCacheResource = ":Login/Mac/ApplicationCache.db";
                #endif

                QString error = QString("%1").arg(restErrorOfflineCacheTransfer);

                QFile webApplicationCacheFile(webApplicationCacheResource);
                if (webApplicationCacheFile.exists())
                {
                    if (webApplicationCacheFile.copy(offlineCache))
                    {
                        QString signinPortalBase = Origin::Services::readSetting(Services::SETTING_SignInPortalBaseUrl);
                        if (!signinPortalBase.isEmpty())
                        {
                            QUrl signinPortalUrl(signinPortalBase);
                            saveOfflineUrlToSettings(signinPortalUrl);
                        }
                    }
                    else
                    {
                        sendAuthenticationErrorTelemetry(error);
                        ORIGIN_LOG_ERROR << "Unable to copy offline login web application cache resource";
                    }
                }
                else
                {
                    sendAuthenticationErrorTelemetry(error);
                    ORIGIN_LOG_ERROR << "Unable to locate offline login web application cache resource.";
                }
            }

            QFile::setPermissions(offlineCache, QFile::ReadOwner | QFile::WriteOwner);
        }

        void LoginViewController::init(const StartupState& startupState, UIToolkit::OriginBanner::IconType bannerIcon /*= UIToolkit::OriginBanner::IconUninitialized*/,
            const QString& bannerText /*= ""*/)
        {
            mBannerIconType = bannerIcon;
            mBannerText = bannerText;

            QTime time = QTime::currentTime();
            qsrand((uint)time.msec());

            // Don't have a minimize or close icon in re-authentication mode
            OriginWindow::TitlebarItems titlebarItems(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close);
            mLoginWindow = new OriginWindow(titlebarItems, NULL, OriginWindow::WebContent);
            mLoginWindow->hide(); // keep the login window hidden until we're ready to show it
            mLoginWindow->setAttribute(Qt::WA_DeleteOnClose, false);
            
#ifdef ORIGIN_MAC
            //
            // Since Qt grants our app a standard OSX Application menu complete with
            // "About" and "Preferences" items if we do nothing, create an basic
            // menubar here with "Preferences" and "About" disabled.
            //
            // Preferences has been removed since most settings are account-specific
            // and without logging in, showing the settings dialog doesn't make any sense.
            //
            // "About" doesn't function because it's currently part of
            // the client view which doesn't exist yet during login.
            //
            // We should probably just move the about box functionality to someplace
            // like OriginApplication so we can hook up the "About" menu item even
            // during the login flow.
            
            // But - you can't view the about box on Windows without logging in
            // so for now I'm just removing it in the interest of parity (even though
            // it goes against the Apple HIG to not have an About menu item)
            //
           MainMenuControllerOSX::getBasicMenu(mLoginWindow);
#endif
            if(Origin::Services::readSetting(Origin::Services::SETTING_OverridesEnabled).toQVariant().toBool())
            {
                QString embargomodeLabel;
                // emabrgo mode (R&D mode) is not disabled, so show an indicator
                if (!Services::readSetting(Services::SETTING_DisableEmbargoMode))
                {
                    embargomodeLabel = "[R&D mode]";
                }
                const QString version = QString("%0 %1 %2 %3")
                    .arg(QCoreApplication::applicationName())
                    .arg(QCoreApplication::applicationVersion())
                    .arg(embargomodeLabel)
                    .arg(Services::readSetting(Origin::Services::SETTING_EnvironmentName).toString());
                mLoginWindow->setTitleBarText(version);
            }

            //only show the ODT notice if we're not already trying to show an error
            if(Origin::Services::readSetting(Origin::Services::SETTING_OriginDeveloperToolEnabled).toQVariant().toBool() && mBannerIconType != OriginBanner::Error)
            {
                mBannerIconType = OriginBanner::Info;
                mBannerText = tr("ebisu_client_notranslate_developer_mode").arg(QCoreApplication::applicationName());
            }

            mStartupState = startupState;

            OriginWebView* originLoginWebView = mLoginWindow->webview();
            originLoginWebView->setHasSpinner(true);
            originLoginWebView->setChangeLoadSizeAfterInitLoad(true);

            QSize preferredSize(360, 404);
            if (mStartupState == StartInTaskbar)
            {
                //MY: for some reason, with switch to Qt5, showMinimized doesn't work unless you call show first
                //and then wait until the next event loop to call showMinimized; but in doing that, the login window
                //will appear, usually flashes.
                //but in the case of auto-login, the page loads as invalid content and that sets the window to 100,100 by default
                //so to get around that we set the preferred size.
                //Also, it seems that when the window is not large enough, it actually won't minimize (100,100 won't minimize).
                originLoginWebView->setPreferredContentsSize (preferredSize);
            }
            originLoginWebView->setVScrollPolicy(Qt::ScrollBarAlwaysOff);
            originLoginWebView->setHScrollPolicy(Qt::ScrollBarAlwaysOff);
            // Set the initial loading size so we won't see any weird window sizing.
            // Hopefully the size of this web page won't change. If it does - we need to update these values.
            originLoginWebView->setWindowLoadingSize(preferredSize);
            LoginWebPage* parentLoginWebPage = new LoginWebPage(this);
            originLoginWebView->webview()->setPage(parentLoginWebPage);
            originLoginWebView->webview()->settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);

            mAuthenticationWebController = new OriginWebController(originLoginWebView->webview(), parentLoginWebPage, NULL, false);

            //Special case for a redirection loop - can only be detected by the inherited web page
            ORIGIN_VERIFY_CONNECT(parentLoginWebPage, SIGNAL(onRedirectionLoopDetected()), this, SLOT(onRedirectionLimitReached()));

            LoginNetworkAccessManager* enam = new LoginNetworkAccessManager();
            mCookieJar = new Services::NetworkCookieJar(enam);
            enam->cookieJar()->deleteLater();
            enam->setCookieJar(mCookieJar);
            mAuthenticationWebController->page()->setNetworkAccessManager(enam);
            parentLoginWebPage->setNetworkAccessManager(enam);

            mAuthenticationWebController->page()->settings()->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, true);
            mAuthenticationWebController->page()->settings()->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, true);
            mAuthenticationWebController->page()->settings()->setAttribute(QWebSettings::LocalStorageEnabled, true);

            if (QWebSettings::offlineWebApplicationCachePath().isEmpty())
            {
                QWebSettings::enablePersistentStorage();
            }

            mAuthenticationWebController->errorDetector()->setErrorCheckEnabled(Origin::Client::PageErrorDetector::MainFrameRequestFailed, true);
            WebWidget::FeatureRequest clientNavigation("http://widgets.dm.origin.com/features/interfaces/clientNavigation");
            WebWidget::FeatureRequest capsLock("http://widgets.dm.origin.com/features/interfaces/capsLock");
            
            mAuthenticationWebController->jsBridge()->addFeatureRequest(clientNavigation);
            mAuthenticationWebController->jsBridge()->addFeatureRequest(capsLock);

            mAuthenticationWebController->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
            ORIGIN_VERIFY_CONNECT(mAuthenticationWebController->page(), SIGNAL(linkClicked(const QUrl &)), 
                this, SLOT(onLinkClicked(const QUrl &)));

            ORIGIN_VERIFY_CONNECT(mAuthenticationWebController->page()->networkAccessManager(), SIGNAL(finished(QNetworkReply *)), 
                    this, SLOT(onRequestFinished(QNetworkReply *)));

            //we need this to be a queued connection because after forcing a stop on load due to timeout, we need to wait one event loop before
            //attempting to load the offline page
            ORIGIN_VERIFY_CONNECT_EX (this, SIGNAL (loadCanceled()), this, SLOT (processLoadCanceled()), Qt::QueuedConnection);

            AuthenticationJsHelper *authenticationJsHelper = new AuthenticationJsHelper(mAuthenticationWebController);
            ORIGIN_VERIFY_CONNECT(authenticationJsHelper, SIGNAL(onlineLoginComplete(const QString&, const QString&)), this, SLOT(onOnlineLoginComplete(const QString&, const QString&)));
            ORIGIN_VERIFY_CONNECT(authenticationJsHelper, SIGNAL(offlineLoginComplete(const QString&, const QString&)), this, SLOT(onOfflineLoginComplete(const QString&, const QString&)));
            ORIGIN_VERIFY_CONNECT(authenticationJsHelper, SIGNAL(authenticationError(QString)), this, SLOT(onAuthenticationError(QString)));
            ORIGIN_VERIFY_CONNECT(authenticationJsHelper, SIGNAL(ageUpBegin(const QString&)), this, SLOT(onAgeUpFlowBegin(const QString&)));
            ORIGIN_VERIFY_CONNECT(authenticationJsHelper, SIGNAL(ageUpCancel()), this, SLOT(onAgeUpFlowCanceled()));
            ORIGIN_VERIFY_CONNECT(authenticationJsHelper, SIGNAL(ageUpComplete()), this, SLOT(onAgeUpFlowComplete()));
            ORIGIN_VERIFY_CONNECT(authenticationJsHelper, SIGNAL(nuxBegin(const QString&)), this, SLOT(onRegistrationStepBegin(const QString&)));
            ORIGIN_VERIFY_CONNECT(authenticationJsHelper, SIGNAL(nuxCancel(const QString&)), this, SLOT(onRegistrationStepCanceled(const QString&)));
            ORIGIN_VERIFY_CONNECT(authenticationJsHelper, SIGNAL(nuxComplete(const QString&)), this, SLOT(onRegistrationStepComplete(const QString&)));
            ORIGIN_VERIFY_CONNECT(mAuthenticationWebController->page()->currentFrame(), SIGNAL(urlChanged ( const QUrl&)),this, SLOT(onUrlChanged(const QUrl&)));
            ORIGIN_VERIFY_CONNECT(mAuthenticationWebController->page(), SIGNAL(loadFinished(bool)), this, SLOT(resetCacheLoadControl()));
            ORIGIN_VERIFY_CONNECT(mAuthenticationWebController->page(), SIGNAL(loadFinished(bool)), this, SLOT(onPageLoaded()));

            mAuthenticationWebController->jsBridge()->addHelper("authenticationJsHelper", authenticationJsHelper);
            authenticationJsHelper->setOfflineBannerInfo (bannerTypeStr(UIToolkit::OriginBanner::Notice), tr("ebisu_client_login_down"));

            // If the user does not login within a certain amouont of time, the flow id on the login page expires and the user will not be able to login.
            // Reloading the page generates a new flow id.
            int loginRefreshTimeout = Origin::Services::readSetting(Origin::Services::SETTING_OverrideLoginPageRefreshTimer).toQVariant().toInt();
            if(loginRefreshTimeout == 0) {
                loginRefreshTimeout = sLoginRefreshTimeout;
            }
            mLoginFlowIdRefreshTimer.setInterval(loginRefreshTimeout * 60.0 * 1000.0);

            ORIGIN_VERIFY_CONNECT(&mLoginFlowIdRefreshTimer, SIGNAL(timeout()), this, SLOT(loadLoginPage()));

            mLoginLoadTimer.setInterval(sLoginLoadTimeout);
            ORIGIN_VERIFY_CONNECT(&mLoginLoadTimer, SIGNAL(timeout()), this, SLOT(onLoadTimeout()));
            ORIGIN_VERIFY_CONNECT(originLoginWebView->webview(), SIGNAL(loadFinished(bool)), this, SLOT(onInitialPageLoad()));

            initalizeWebApplicationCache();
            loadPersistentCookies();
            loadLoginPage();
        }

        void LoginViewController::onPageLoaded()
        {
            if (mAuthenticationWebController)
            {
                QString url = mAuthenticationWebController->page()->mainFrame()->url().toString();
                QString onlineServiceUrl = Origin::Services::readSetting(Services::SETTING_OnlineLoginServiceUrl);
                if (!url.isEmpty() && !onlineServiceUrl.isEmpty() && url.contains(onlineServiceUrl))
                {
                    mOnlineLoginPageLoadSuccess = true;
                }
            }
            mAutoLoginInProgress = false;

            if (mLoginWindow)
            {
                mLoginWindow->webview()->webview()->setFocus();
            }
        }

        void LoginViewController::resetCacheLoadControl()
        {
            if (mAuthenticationWebController)
            {
                LoginNetworkAccessManager* nam = dynamic_cast<LoginNetworkAccessManager*>(mAuthenticationWebController->page()->networkAccessManager());
                if (nam)
                {
                    nam->setPreferNetwork();
                }
            }
        }

        void LoginViewController::onAuthenticationError(QString error)
        {
            sendAuthenticationErrorTelemetry(error);
        }

        void LoginViewController::sendAuthenticationErrorTelemetry(const QString error, const QString& requestUrl /* = "" */, int httpResponseCode /* = 0 */,
                                                                   QNetworkReply::NetworkError qError /*= QNetworkReply::NoError*/, int errorSubType /* = 0 */,
                                                                   const QString& qtErrorString /* = "" */)
        {
            QString errorCode(parseErrorCode(error));
            const QString additionalErrorInfo(parseAdditionalErrorInfo(error));

            // If login failed because of an http error and the login page did not load from the network,
            // send ERROR_LOGINPAGELOAD_FAILURE
            if (errorCode.toInt() == restErrorAuthenticationHttp && !mOnlineLoginPageLoadSuccess)
            {
                errorCode = QString("%1").arg(restErrorLoginPageLoad);
            }

            Services::restError err = static_cast<Services::restError>(errorCode.toInt());
            QString errorStr = Services::getLoginErrorString(err);

            ORIGIN_LOG_EVENT << "Authentication error: " << errorStr << " subtype:" << errorSubType;

            if (!requestUrl.isEmpty())
            {
                GetTelemetryInterface()->Metric_LOGIN_FAIL(
                    errorCode.toLatin1().constData(),
                    errorStr.toLocal8Bit().constData(),
                    mLoginType.toUtf8().constData(),
                    httpResponseCode,
                    requestUrl.left(160).toLocal8Bit().constData(),
                    qError,
                    errorSubType,
                    additionalErrorInfo.toUtf8().constData(),
                    qtErrorString.toUtf8().constData());
            }
            else
            {
                GetTelemetryInterface()->Metric_LOGIN_FAIL(
                    errorCode.toLatin1().constData(),
                    errorStr.toLocal8Bit().constData(),
                    mLoginType.toUtf8().constData(),
                    0,
                    "",
                    QNetworkReply::NoError,
                    errorSubType,
                    additionalErrorInfo.toUtf8().constData(),
                    qtErrorString.toUtf8().constData());
            }

            //  Send to heartbeat
            Heartbeat::instance()->setLoginError(err);
            if(err == Services::restErrorAuthenticationRememberMe)
            {
                // Case is triggered on bad auto-login credentials
                clearRememberMeCookie();      //since the cookie was bad, we want to make sure we clear the persistence

                // Auto-login failed so type of login is now Manual
                mLoginType = "Manual";
                mAutoLogin = false;
                emit typeOfLogin(mLoginType, mAutoLogin);
            }
        }

        void LoginViewController::onLinkClicked(const QUrl& url)
        {
            if (Services::Network::GlobalConnectionStateMonitor::instance()->isOnline())
            {
                bool isShareLegalInfoLink = url.path().contains("shareInfoLegal", Qt::CaseInsensitive);

                if (isShareLegalInfoLink)
                {
                    showPopupDialog(url);
                }
                // If it's the same host we keep it within the login window, otherwise we open in system browser (TOS, Help, etc)
                else if(url.host().compare(mAuthenticationWebController->page()->mainFrame()->url().host(), Qt::CaseInsensitive) != 0)
                {
                    QDesktopServices::openUrl(url);
                }
                else
                {
                    mAuthenticationWebController->loadTrustedUrl(url);
                }
            }
        }

        void LoginViewController::onAgeUpFlowBegin (const QString& executeJSstmt)
        {
            mExecuteJSstatementToSkipAgeUp = executeJSstmt;

            // Disconnect all signals to the reject. On window close - treat it as a cancel
            ORIGIN_VERIFY_DISCONNECT(mLoginWindow, SIGNAL(rejected()), 0, 0);
            ORIGIN_VERIFY_CONNECT(mLoginWindow, SIGNAL(rejected()), this, SLOT(onAgeUpWindowClosed()));

            //during the age-up flow, losing connection should just take them back to the login page (for now)
            ORIGIN_VERIFY_CONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(globalConnectionStateChanged(bool)),
                this, SLOT(onConnectionChangeDuringAgeUp(bool)));
        }

        void LoginViewController::onAgeUpFlowCanceled ()
        {
            ORIGIN_VERIFY_DISCONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(globalConnectionStateChanged(bool)),
                this, SLOT(onConnectionChangeDuringAgeUp(bool)));
        }

        void LoginViewController::onAgeUpFlowComplete ()
        {
            ORIGIN_VERIFY_DISCONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(globalConnectionStateChanged(bool)),
                this, SLOT(onConnectionChangeDuringAgeUp(bool)));

            emit ageUpComplete();
        }

        void LoginViewController::onAgeUpWindowClosed()
        {
            ORIGIN_VERIFY_DISCONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(globalConnectionStateChanged(bool)),
                this, SLOT(onConnectionChangeDuringAgeUp(bool)));

            //reconnect the close window logic
            ORIGIN_VERIFY_DISCONNECT(mLoginWindow, SIGNAL(rejected()), 0, 0);
            ORIGIN_VERIFY_CONNECT(mLoginWindow, SIGNAL(rejected()), this, SLOT(onUserClosedWindow()));

            if (mExecuteJSstatementToSkipAgeUp.isEmpty())
            {
                //this shouldn't happen, but in case it does, reroute the user back to the login apge
                //Close the pop up dialog related to the registration flow if it's alive
                closePopupWindow();

                mAuthenticationWebController->errorHandler()->setRequestToRestore(QNetworkRequest(onlineUrl()));
                loadLoginPage();
            }
            else
            {
                mAuthenticationWebController->page()->mainFrame()->evaluateJavaScript(mExecuteJSstatementToSkipAgeUp);
            }
        }

        void LoginViewController::onRegistrationStepBegin (const QString& step)
        {
            // Set this flag so we do not show a promo browser to new users
            MainFlow::instance()->setNewUser(true);

            // Disconnect all signals to the reject. On window close - show the login page.
            ORIGIN_VERIFY_DISCONNECT(mLoginWindow, SIGNAL(rejected()), 0, 0);
            ORIGIN_VERIFY_CONNECT(mLoginWindow, SIGNAL(rejected()), this, SLOT(loadLoginPage()));

            if (step.toInt() == 0)
            {
                // Once we begin NUX, going offline should take us to login page
                ORIGIN_VERIFY_CONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(globalConnectionStateChanged(bool)),
                    this, SLOT(onConnectionChangeDuringRegistration(bool)));
            }
            // When we start the 3rd step (step="2" because it's zero based), the user's account has been created and they are on optional name steps.  
            // If we go online/offline we want to reset back to the login page once we pass this step
            else if(step.toInt() >= 2)
            {
                ORIGIN_LOG_DEBUG << "Setting URL to restore to login page because we moved to NUX step " << step;
                mAuthenticationWebController->errorHandler()->setRequestToRestore(QNetworkRequest(loginUrl()));
            }
        }

        void LoginViewController::onRegistrationStepCanceled (const QString& step)
        {
            ORIGIN_VERIFY_DISCONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(globalConnectionStateChanged(bool)),
                this, SLOT(onConnectionChangeDuringRegistration(bool)));

            // Close the pop up dialog related to the registration flow if it's alive
            closePopupWindow();

            // When the NUX cancel button is pressed, the login page is loaded internally, loadTrustedUrl never gets called, so the restore url will remain
            // as the NUX page. This sets it back to the login page.
            mAuthenticationWebController->errorHandler()->setRequestToRestore(QNetworkRequest(onlineUrl()));

            // Clear this flag because we are no longer creating a new user and headed back to login page
            MainFlow::instance()->setNewUser(false);
        }

        void LoginViewController::onRegistrationStepComplete (const QString& step)
        {
            if(step.toInt() == 1)
            {
                // Only set true when 'create account' is clicked on the 1st step
                // This flag is set for telemetry purposes because the client loads the AUTH URL and we do not want telemetry sent
                mIsNux = true;
                QString oemTag;
                getOemInformation(oemTag);
                if(!oemTag.isEmpty())
                {
                    // Send telemetry if the user clicks 'create account' and if oemTag is populated
                    GetTelemetryInterface()->Metric_NUX_OEM(oemTag.toUtf8().data());
                }
            }

        }

        //if going online, need to handle the manual loading of the online page so we can load the remid cookie first to allow for auto-login
        //most needed for when autoStart has been set and the network connection isn't detected until after the initial load has occurred
        void LoginViewController::onConnectionChange (bool online)
        {
            if (online)
            {
                //if it goes offline, the slot will get connected again
                if (Origin::Services::Connection::ConnectionStatesService::instance() != NULL)
                {
                    ORIGIN_VERIFY_DISCONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(globalConnectionStateChanged(bool)),
                        this, SLOT(onConnectionChange(bool)));
                }

                loadPersistentCookies();

                if(!mCurrentRememberMeCookie.isEmpty())
                {
                    mLoginType = "Auto-Login";
                    mAutoLogin = true;
                }
                else
                {
                    mLoginType = "Manual";
                    mAutoLogin = false;
                }
                emit typeOfLogin(mLoginType, mAutoLogin);

                mBannerIconType = OriginBanner::IconUninitialized;
                mBannerText.clear();
                loadLoginPage();
            }
        }

        void LoginViewController::onRedirectionLimitReached()
        {
            if (!mLoggingInOffline)
            {
                //Stop the load timer as we don't want it to go off during the following steps.
                //Load timer will be restarted
                mLoginLoadTimer.stop();

                ORIGIN_LOG_ERROR << "infinite login page redirection detected";

                GetTelemetryInterface()->Metric_LOGIN_FAIL(
                    QString::number(restErrorLoginRedirectLoop).toLatin1().data(),
                    Services::getLoginErrorString(restErrorLoginRedirectLoop).toLatin1().data(),
                    mLoginType.toUtf8().data(), restErrorLoginRedirectLoop);
                Heartbeat::instance()->setLoginError(restErrorLoginRedirectLoop);

                initiateCancelLoad();
            }
        }

        void LoginViewController::onConnectionChangeDuringAgeUp(bool online)
        {
            if (!online)
            {
                ORIGIN_VERIFY_DISCONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(globalConnectionStateChanged(bool)),
                    this, SLOT(onConnectionChangeDuringAgeUp(bool)));

                loadLoginPage(true);
            }
        }


        void LoginViewController::onConnectionChangeDuringRegistration(bool online)
        {
            if (!online)
            {
                ORIGIN_VERIFY_DISCONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(globalConnectionStateChanged(bool)),
                    this, SLOT(onConnectionChangeDuringRegistration(bool)));

                loadLoginPage(true);
            }
        }

        void LoginViewController::onUrlChanged(const QUrl& url)
        {
            const QString strUrl = url.toString();

#ifdef _DEBUG
            ORIGIN_LOG_EVENT << "onUrlChanged: url = [" << strUrl << "]";
#else
            ORIGIN_LOG_EVENT << "onUrlChanged: url = [" << url.scheme() << "://" << url.host() << url.path() << "]";
#endif

            //safe to look for the string because it is in our QRC resource file, if that changes this needs to be changed
            if (strUrl.contains("forgot_password")) 
            {
                mLoginLoadTimer.stop();
                ORIGIN_VERIFY_CONNECT(mAuthenticationWebController->page()->currentFrame(), SIGNAL(javaScriptWindowObjectCleared()),this, SLOT(addJSObject()));
            }
            else if (!(offlineLoginUrl().isEmpty()) && strUrl.contains (offlineLoginUrl()))
            {
                mLoginLoadTimer.stop();
                // Disconnect all signals to the reject. On window close - close Origin
                ORIGIN_VERIFY_DISCONNECT(mLoginWindow, SIGNAL(rejected()), 0, 0);
                ORIGIN_VERIFY_CONNECT(mLoginWindow, SIGNAL(rejected()), this, SLOT(onUserClosedWindow()));
            }
            else if (Services::Network::GlobalConnectionStateMonitor::instance()->isOnline())
            {
                mLoginLoadTimer.stop();
                mLoggingInOffline = false;

                QString onlineServiceUrl = Origin::Services::readSetting(Services::SETTING_OnlineLoginServiceUrl);
                if (onlineServiceUrl.isEmpty())
                {
                     ORIGIN_LOG_ERROR << "Login service url setting is empty.";
                }
                //need to distinguish between actual url and qrc that contains "login_success"
                else if(strUrl.contains(onlineServiceUrl) && !strUrl.contains ("qrc:"))
                {
                    // Disconnect all signals to the reject. On window close - close Origin
                    ORIGIN_VERIFY_DISCONNECT(mLoginWindow, SIGNAL(rejected()), 0, 0);
                    ORIGIN_VERIFY_CONNECT(mLoginWindow, SIGNAL(rejected()), this, SLOT(onUserClosedWindow()));

                    saveOfflineUrlToSettings(url);
                }
            }
        }

        void LoginViewController::saveOfflineUrlToSettings(const QUrl& onlineUrl)
        {
            QString offlineServiceUrl = Origin::Services::readSetting(Services::SETTING_OfflineLoginServiceUrl);
            if (!offlineServiceUrl.isEmpty())
            {
                QUrl offlineUrl;

                offlineUrl.setScheme(onlineUrl.scheme());
                offlineUrl.setHost(onlineUrl.host());
                offlineUrl.setPort(onlineUrl.port());

                if (!offlineServiceUrl.startsWith('/'))
                    offlineServiceUrl.insert(0, '/');
                offlineUrl.setPath(offlineServiceUrl);
                addLoginUrlQueryParameters(offlineUrl, true);

                QString key = sOfflineLoginUrlKey;
                QString envname = Origin::Services::readSetting(Services::SETTING_EnvironmentName, Services::Session::SessionRef());

                //we need to differentiate between PROD and non-PROD so that the it saves off the correct offline url based on environment
                //since we were already using sOfflineLoginUrlKey, leave that as is so that it won't affect live users
                //and just append _INT to the non-PROD use since that would be used by internal users
                if (envname != Services::SETTING_ENV_production)
                    key += "Int";

                Setting offlineLoginUrl(
                    key,
                    Variant::String,
                    Variant(""),
                    Setting::LocalAllUsers,
                    Setting::ReadWrite,
                    Setting::Unencrypted);
                Origin::Services::writeSetting(offlineLoginUrl, offlineUrl.toString());
            }
        }

        void LoginViewController::addJSObject() 
        {
            AuthenticationJsHelper *authenticationJsHelper = new AuthenticationJsHelper(mAuthenticationWebController);
            ORIGIN_VERIFY_CONNECT(authenticationJsHelper, SIGNAL(changePasswordUrl (const QString&)), this, SLOT(onChangePasswordUrl(const QString&)));
            mAuthenticationWebController->page()->mainFrame()->addToJavaScriptWindowObject("authenticationJsHelper2", authenticationJsHelper);
        }

        void LoginViewController::onChangePasswordUrl ( const QString& url)
        {
            mIsChangePassword = true;
            ORIGIN_LOG_EVENT << "Forgot password";
            QString signinPortalBaseUrl = Origin::Services::readSetting(Origin::Services::SETTING_SignInPortalBaseUrl).toString();
            
            QString finalUrl = signinPortalBaseUrl + url;
            QUrl myOpenUrl = QUrl::fromPercentEncoding(finalUrl.toLatin1());
            bool res = QDesktopServices::openUrl(myOpenUrl);
            ORIGIN_UNUSED(res);
            ORIGIN_ASSERT(res);
            loadLoginPage();
        }

        void LoginViewController::onLoadErrorFinished(bool ok)
        {
            ORIGIN_VERIFY_DISCONNECT(mAuthenticationWebController->page(), SIGNAL(loadFinished(bool)), this, SLOT(onLoadErrorFinished(bool)));
            if (ok)
            {
                mLoginWindow->show();
            }
        }
        
        void LoginViewController::loadLoginPage(bool forceOfflineLogin)
        {
            // Clear this flag at the beginning of each login
            MainFlow::instance()->setNewUser(false);

            mLoginFlowIdRefreshTimer.start();

            QString lUrl = loginUrl(forceOfflineLogin);

            mAutoLoginInProgress = (!mLoggingInOffline && mAutoLogin);

#ifdef _DEBUG 
            ORIGIN_LOG_EVENT << "loadLoginPage[" << lUrl << "]";
#else
            QString lUrlTrimmed = lUrl;
            int paramIndex = lUrl.indexOf ("?", 0, Qt::CaseInsensitive);
            if (paramIndex > 0)
            {
                lUrlTrimmed = lUrl.left (paramIndex);
            }
            ORIGIN_LOG_EVENT << "loadLoginPage[" << lUrlTrimmed << "]";
#endif

            if (!lUrl.isEmpty())
            {
                //mLoginWindow could be NULL from user choosing to close the window and something else will trigger the loadLoginPage
                //so put in a guard just in case
                if (mLoginWindow)
                {
                    //hook up the signal here for when user wants to close the window if the load is taking too long
                    //and we don't ever get to onUrlChanged
                    ORIGIN_VERIFY_DISCONNECT(mLoginWindow, SIGNAL(rejected()), 0, 0);
                    ORIGIN_VERIFY_CONNECT(mLoginWindow, SIGNAL(rejected()), this, SLOT(onUserClosedWindow()));

                    //also hook up the timeout for when the load is taking too long
                    mLoginLoadTimer.start();            
                }

                // For online login, if the web application cache is almost full we need to clear it. It will be repopulated with the offline
                // login page when the online page loads. The cache is cleared at 90% capacity because caching of the files will fail silenty
                // if their size exceeds the remaining space. QWebPage is supposed to emit the signal applicationCacheQuotaExceeded, but it
                // never seems to fire.
                if (!mLoggingInOffline)
                {
                    qulonglong webApplicationCacheQuota = Origin::Services::readSetting(Origin::Services::SETTING_MaxOfflineWebApplicationCacheSize).toQVariant().toULongLong();

                    QFileInfo webApplicationCacheDb(mAuthenticationWebController->page()->settings()->offlineWebApplicationCachePath()+ QDir::separator() + "ApplicationCache.db");
                    if (!webApplicationCacheDb.exists() || webApplicationCacheDb.size() > 0.9 * webApplicationCacheQuota)
                    {
                        if (webApplicationCacheDb.exists())
                        {
                            ORIGIN_LOG_EVENT << "Clearing offline web application cache";
                            GetTelemetryInterface()->Metric_WEB_APPLICATION_CACHE_CLEAR();
                        }
                        else
                        {
                            ORIGIN_LOG_EVENT << "Setting offline web application cache quota for the first time.";
                        }

                        mAuthenticationWebController->page()->settings()->setOfflineWebApplicationCacheQuota(webApplicationCacheQuota);
                    }
                }
                else
                {
                    if (mAuthenticationWebController)
                    {
                        LoginNetworkAccessManager* nam = dynamic_cast<LoginNetworkAccessManager*>(mAuthenticationWebController->page()->networkAccessManager());
                        if (nam)
                        {
                            nam->setAlwaysUseCache();
                        }
                    }
                }
                GetTelemetryInterface()->Metric_PERFORMANCE_TIMER_END(EbisuTelemetryAPI::Bootstrap);
                GetTelemetryInterface()->Metric_PERFORMANCE_TIMER_END(EbisuTelemetryAPI::ShowLoginPage, 0);
                
                // If we're not automatically logging in, start timer to see how long it takes the server to return the valid information to load the login page.
                if (!mAutoLogin)
                {
                    GetTelemetryInterface()->Metric_PERFORMANCE_TIMER_START(EbisuTelemetryAPI::LoginPageLoaded);                    
                }

                mAuthenticationWebController->loadTrustedUrl(lUrl);

                // If we are offline, when network connectivity is restored, the PageErrorHandler will automatically load whatever the restore url
                // is set to. The restore url gets set when mAuthenticationWebController->loadTrustedUrl(lUrl) is called; however, in this case
                // we do not want to restore the offline url we want to restore the online url.
                if (!Services::Network::GlobalConnectionStateMonitor::instance()->isOnline() || forceOfflineLogin)
                {
                    //manually handle loading the online login page when connection is restored so that we can load the remember me cookie
                    ORIGIN_VERIFY_CONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(globalConnectionStateChanged(bool)),
                        this, SLOT(onConnectionChange(bool)));
                }

            }
            else //assume that this means application cache does not exit
            {
                onErrorDetected (QNetworkReply::UnknownContentError, NULL);
            }
        }

        QString LoginViewController::loginUrl(bool getOfflineUrl)
        {
            bool isOnline = Services::Network::GlobalConnectionStateMonitor::instance()->isOnline();
            ORIGIN_LOG_EVENT << "LoginViewController: loginUrl - getOfflineUrl=" << getOfflineUrl << " global connection state =" << isOnline;

            if (getOfflineUrl || !isOnline)
            {
                mLoginType = getOfflineUrl ? "OfflineFallback" : "Offline";   
                mLoginFlowIdRefreshTimer.stop();

                //need to set this so it won't try to reload the page in onErrorDetected
                mLoggingInOffline = true;

                //user must have logged in online at least once previously
                //we need to first check and see if a valid cache exists, otherwise it will just fail
                mOfflineCacheStatus = validateOfflineWebApplicationCache();
                if (mOfflineCacheStatus == Missing || mOfflineCacheStatus == Corrupt)
                {
                    return QString();
                }

                QString offlineUrl = offlineLoginUrl();
                if (offlineUrl.isEmpty())
                {
                    //this can happen if settings is deleted (as in a test case for automation)
                    mOfflineCacheStatus = Missing;      //treat it as same as offline cache missing so that we'll get the "must be online first time" error
                    return QString();
                }

                 GetTelemetryInterface()->Metric_LOGIN_START(mLoginType.toUtf8().data());

                //show the offline one only if there isn't already a message defined
                //or if the ODT is enabled -- since that would have set mBannerText to show ODT, but make offline message take precedence
                //but if it's an error, let the error take highest precedence
                if (mBannerText.isEmpty() || (Services::readSetting(Services::SETTING_OriginDeveloperToolEnabled).toQVariant().toBool() && mBannerIconType !=UIToolkit::OriginBanner::Error ))
                {
                    mBannerText = tr("ebisu_client_login_down");
                    mBannerIconType = UIToolkit::OriginBanner::Notice;
                }

                QUrl url(offlineUrl);
                addOEMQueryParameter(url);
                addBannerQueryParameters(url);
                addAgeUpQueryParameter(url);
                emit typeOfLogin(mLoginType, mAutoLogin);

                // Remove remember me cookie from the jar for offline login because of a bug in the login page that prevents
                // a user not associated with the cookie from a successful login.
                if (!mCurrentRememberMeCookie.isEmpty())
                {
                    mCookieJar->removeCookieByValue(mCurrentRememberMeCookie);
                    mCurrentRememberMeCookie.clear();
                }

                return url.toString();
            }

            ORIGIN_LOG_EVENT << "Logging in online";
            mLoggingInOffline = false;

            sendLoginTypeTelemetry();
            return onlineUrl();
        }

        LoginViewController::OfflineCacheType LoginViewController::validateOfflineWebApplicationCache()
        {
            QString offlinecachePath = QWebSettings::offlineWebApplicationCachePath();
            QString offlineCache (offlinecachePath + QDir::separator() + "ApplicationCache.db");

            ORIGIN_LOG_EVENT << "Web application cache path: " << offlinecachePath;

            if (!QFile::exists(offlineCache))
            {
                return Missing;
            }
            else
            {
                //need to check the size and see if it might be corrupt
                #if ORIGIN_PC
                    const qint64 validCacheSizeMin = 100*1024;
                #else //ORIGIN_MAC
                    const qint64 validCacheSizeMin = 100*1024;
                #endif

                qint64 cacheSize = 0;
                QFile cacheFile(offlineCache);
                if(cacheFile.open(QIODevice::ReadOnly))
                {
                    cacheSize = cacheFile.size();
                    cacheFile.close();
                }
                if (cacheSize < validCacheSizeMin)
                {
                    //cache size is too small, assume corruption
                    return Corrupt;
                }
            }
            
            return Exists;
        }

        QString LoginViewController::onlineUrl() const
        {
            QString connectPortalBaseUrl = Origin::Services::readSetting(Origin::Services::SETTING_ConnectPortalBaseUrl).toString();

            QString myUrl = QString("%1/connect/auth").arg(connectPortalBaseUrl);
            QUrl loginUrl(myUrl);
            addLoginUrlQueryParameters(loginUrl, false);
            addBannerQueryParameters(loginUrl);
            addMachineIdParameter(loginUrl);
            QUrlQuery loginUrlQuery(loginUrl);
            if (!mSsoToken.isNull() && !mSsoToken.isEmpty())
            {
                loginUrlQuery.addQueryItem("access_token", mSsoToken);
            } 
            loginUrl.setQuery(loginUrlQuery);
            return loginUrl.toString();
        }

        void LoginViewController::sendLoginTypeTelemetry()
        {
            if(!mInitialTelemetrySent)
            {
                // If the client builds the NUX account creation URL or the password change URL
                // Do not send telemetry. Make sure we reset these variables for the next call of loginUrl()
                if (mIsNux == true)
                {
                    mIsNux = false;
                    return;
                }
                else if (mIsChangePassword == true)
                {
                    mIsChangePassword = false;
                    return;
                }

                // Otherwise this is a standard login and we should send telemetry
                if (!mSsoToken.isNull() && !mSsoToken.isEmpty())
                {
                    mLoginType = "SSO";
                    GetTelemetryInterface()->Metric_LOGIN_START(mLoginType.toUtf8().data());
                } 
                else if(!mCurrentRememberMeCookie.isNull() && !mCurrentRememberMeCookie.isEmpty())
                {
                    mLoginType = "Auto-Login";
                    mAutoLogin = true;
                    GetTelemetryInterface()->Metric_LOGIN_START(mLoginType.toUtf8().data());
                }
                else
                {
                    mLoginType = "Manual";
                    GetTelemetryInterface()->Metric_LOGIN_START(mLoginType.toUtf8().data());
                }

                emit typeOfLogin(mLoginType, mAutoLogin);
                mInitialTelemetrySent = true;
            }
        }

        void LoginViewController::addLoginUrlQueryParameters(QUrl& url, bool isOfflineUrl) const
        {
            QString clientId = Origin::Services::readSetting(Origin::Services::SETTING_ClientId).toString();
            QString loginSuccessRedirectUrl = Origin::Services::readSetting(Origin::Services::SETTING_LoginSuccessRedirectUrl).toString();

            QString locale = Origin::Services::readSetting(Origin::Services::SETTING_LOCALE);
            int nonce = qrand(); // The nonce is used by the connect server to generate a cid which is used to encrypt user data for offline mode.

            // We purposely do NOT define the "scope" parameter in order to rely on the default scope for our client ID (EBIBUGS-27433).
            QUrlQuery urlQuery(url);
            urlQuery.addQueryItem("client_id", clientId.toUtf8());
            urlQuery.addQueryItem("response_type", "code id_token");
            urlQuery.addQueryItem("redirect_uri", loginSuccessRedirectUrl.toUtf8());
            urlQuery.addQueryItem("display", "origin_client");
            urlQuery.addQueryItem("locale", locale.toUtf8());
            urlQuery.addQueryItem("nonce", QString::number(nonce).toUtf8());
            
            url.setQuery(urlQuery);
            addOEMQueryParameter (url);
            addAgeUpQueryParameter (url);
            urlQuery.setQuery(url.query()); 

            if (isOfflineUrl)
            {
                QString connectPortalBaseUrl = Origin::Services::readSetting(Origin::Services::SETTING_ConnectPortalBaseUrl).toString();

                QUrl connectUrl(connectPortalBaseUrl);
                QString port = QString::number(connectUrl.port());
                QUrl host;
                host.setScheme(connectUrl.scheme());
                host.setHost(connectUrl.host());

                urlQuery.addQueryItem("host", host.toString().toUtf8());
                if (port != "-1")
                {
                    urlQuery.addQueryItem("port", port.toUtf8());
                }
            }
            url.setQuery(urlQuery);
        }

        QString LoginViewController::bannerTypeStr(UIToolkit::OriginBanner::IconType type) const
        {
            QString typeValue;
            switch (type)
            {
                case UIToolkit::OriginBanner::Error:
                    typeValue = "error";
                    break;
                case UIToolkit::OriginBanner::Success:
                    typeValue = "success";
                    break;
                case UIToolkit::OriginBanner::Info:
                    typeValue = "info";
                    break;
                case UIToolkit::OriginBanner::Notice:
                    typeValue = "caution";
                    break;
                default:
                    break;
            }

            return typeValue;
        }

        void LoginViewController::addBannerQueryParameters(QUrl& url) const
        {
            if (!mBannerText.isEmpty())
            {
                QString typeValue = bannerTypeStr(mBannerIconType);
                if (!typeValue.isEmpty())
                {
                    QUrlQuery urlQuery(url);
                    urlQuery.addQueryItem("bannerType", typeValue);
                    urlQuery.addQueryItem("bannerText", mBannerText.toUtf8());
                    url.setQuery(urlQuery);
                }
            }
        }

        void LoginViewController::addOEMQueryParameter (QUrl & url) const
        {
            QString oemTag;
            getOemInformation(oemTag);
            if (!oemTag.isEmpty())
            {
                //make sure it isn't already there
                QString urlStr = url.toString();
                if (!urlStr.contains ("registration_source", Qt::CaseInsensitive))
                {
                    QUrlQuery urlQuery(url);
                    urlQuery.addQueryItem("registration_source", oemTag.toLatin1());
                    url.setQuery(urlQuery);
                }
            }
        }

        void LoginViewController::addAgeUpQueryParameter (QUrl& url) const
        {
            //if it's RTP launch then bypass ageup flow
            if (MainFlow::instance()->rtpFlow() && MainFlow::instance()->rtpFlow()->isPending())
            {
                QString urlStr = url.toString();
                if (!urlStr.contains ("showAgeUp", Qt::CaseInsensitive))
                {
                    QUrlQuery urlQuery(url);
                    urlQuery.addQueryItem("showAgeUp", "false");
                    url.setQuery(urlQuery);
                }
                setShowAgeUpFlow (false);
            }
            else
                setShowAgeUpFlow (true);
        }

        void LoginViewController::addMachineIdParameter(QUrl & url) const
        {
            QUrlQuery urlQuery(url);
            urlQuery.addQueryItem("pc_machine_id", NonQTOriginServices::SystemInformation::instance().GetUniqueMachineHash());
            url.setQuery(urlQuery);
        }

        QString LoginViewController::offlineLoginUrl() const
        {
            QString key = sOfflineLoginUrlKey;

            QString envname = Origin::Services::readSetting(Services::SETTING_EnvironmentName, Services::Session::SessionRef());
            if (envname != Services::SETTING_ENV_production)
                key += "Int";

            Setting offlineLoginUrl(
                key,
                Variant::String,
                Variant(""),
                Setting::LocalAllUsers,
                Setting::ReadWrite,
                Setting::Unencrypted);

            return Origin::Services::readSetting(offlineLoginUrl).toString();
        }

        void LoginViewController::onOnlineLoginComplete(const QString& authorizationCode, const QString& idToken)
        {
            mLoginFlowIdRefreshTimer.stop();
            mLoginLoadTimer.stop();

            closePopupWindow();
            if (mLoginWindow)
            {
                mLoginWindow->hide();
            }

            QString envname = Origin::Services::readSetting(Services::SETTING_EnvironmentName, Services::Session::SessionRef());
            const Services::Setting &usernameCookieSetting = envname == Services::SETTING_ENV_production ? Services::SETTING_MOST_RECENT_USER_NAME_PROD : Services::SETTING_MOST_RECENT_USER_NAME_INT;
            const Services::Setting &tfaIdCookieSetting = envname == Services::SETTING_ENV_production ? Services::SETTING_TFAID_PROD : Services::SETTING_TFAID_INT;

            //don't have the userID yet, but we need to save off the cookie (since the previous one is now invalidated) in the event that the user aborts out before the actual full login flow is finished
            //cookie will get saved out again along with corresponding userID once we're finished with the full login flow
            saveRememberMeCookie ("");

            //clear out the previous stale cookie first
            clearCookieIfStale (Session::LOGIN_COOKIE_NAME_LASTUSERNAME, mCurrentUsernameCookie, Origin::Services::NetworkCookieJar::cookieExpiration, usernameCookieSetting, false);
            // Persist username cookie
            persistCookie(Session::LOGIN_COOKIE_NAME_LASTUSERNAME, mCurrentUsernameCookie, usernameCookieSetting, Services::SETTING_INVALID_MOST_RECENT_USER_NAME);

            //clear out the previous stale cookie first
            clearCookieIfStale (Session::LOGIN_COOKIE_NAME_TFA_REMEMBERMACHINE, mCurrentTFAIdCookie, Origin::Services::NetworkCookieJar::cookieExpiration, tfaIdCookieSetting, false);
            // Persist TFA id cookie
            persistCookie(Session::LOGIN_COOKIE_NAME_TFA_REMEMBERMACHINE, mCurrentTFAIdCookie, tfaIdCookieSetting, Services::SETTING_INVALID_TFAID);

            mCookieJar->transferCookies(Origin::Services::NetworkAccessManagerFactory::instance()->sharedCookieJar());

            // When the login flow completes, the connection state will change, triggering the error handler to restore the initial url of the web controller.
            // We don't want a 2nd request to authenticate going out after a successful login, so clear the restore URL.
            mAuthenticationWebController->errorHandler()->setRequestToRestore(QNetworkRequest());

            settingsFileSyncHelper();

            emit loginComplete(authorizationCode, idToken);
        }

        void LoginViewController::clearCookieIfStale (const QString& name, QString& cookieValue, Origin::Services::NetworkCookieJar::cookieElementType byType, const Services::Setting& settingKey, bool forceClearCookie)
        {
            //find out how many cookies of "name" we have
            //normally there would be two, the old one and the new one, but if we're offline, there might only be one
            int numCookies = mCookieJar->cookieCountByName (name);

            //if there's only one (usually from offline login), then we normally don't want to clear it unless we are passed in forceClearCookie
            if (!cookieValue.isEmpty() && (numCookies > 1 || forceClearCookie))
            {
                switch (byType)
                {
                    //need this for clearing pcun since value would be the same for multiple versions of the cookie
                    case Origin::Services::NetworkCookieJar::cookieExpiration:
                        {
                            //need to extract out the previous expiration
                            QString settingsCookieValue(Services::readSetting(settingKey));
                            QStringList tokens = settingsCookieValue.split("||", QString::SkipEmptyParts);
                            if( (tokens.size() == 3)||(tokens.size() == 4))
                            {
                                QDateTime expiration = QDateTime::fromString(tokens.at(2), Qt::TextDate);
                                expiration.setTimeSpec(Qt::UTC);
                                mCookieJar->removeCookieByValueAndExpiration(cookieValue, expiration);
                            }
                        }
                        break;
                
                    case Origin::Services::NetworkCookieJar::cookieValue:
                    default:
                        mCookieJar->removeCookieByValue(cookieValue);
                        break;
                }
                cookieValue.clear();
            }
        }

        void LoginViewController::persistCookie(const QString& name, QString& cookieValue, const Services::Setting& settingKey, const QString& invalidSettingValue)
        {
            QNetworkCookie c;

            quint32 count = mCookieJar->cookieCountByName (name);

            //in case there are multiple cookies with the same name, grab the one with the expiration date furthest out (which would presumably be the most recently acquired cookie)
            if (mCookieJar->findCookieWithLatestExpirationByName(name, c))
            {
                quint32 errcode = 0;

                saveCookieToSetting (settingKey, c);

                if ((name == Session::LOGIN_COOKIE_NAME_REMEMBERME) || (name == Session::LOGIN_COOKIE_NAME_TFA_REMEMBERMACHINE))
                {
                    QString invalidSettingValue;
                    if (name == Session::LOGIN_COOKIE_NAME_REMEMBERME)
                    {
                        invalidSettingValue = Services::SETTING_INVALID_REMEMBER_ME;
                    }
                    else if (name == Session::LOGIN_COOKIE_NAME_TFA_REMEMBERMACHINE)
                    {
                        invalidSettingValue = Services::SETTING_INVALID_TFAID;
                    }

                    QString expirationDate(c.expirationDate().toUTC().toString());
                    QString cValue = c.value();
                    QString lTrimmedValue = cValue;

                    //grab the last 10 "digits" of the cookie
                    if (!cValue.isEmpty())
                    {
                        lTrimmedValue = cValue.right(10);
                    }

                    QString settingsCookieValue(Services::readSetting(settingKey));

                    if (!settingsCookieValue.isEmpty() &&
                        settingsCookieValue != invalidSettingValue)
                    {
                        ORIGIN_LOG_DEBUG << "cookie " << name << " saved off, Expires: " << expirationDate;
                    }
                    else if (settingsCookieValue.isEmpty())
                    {
                        ORIGIN_LOG_DEBUG << "settingsCookieValue is empty";
                        errcode = 1;
                    }
                    else
                    {
                        ORIGIN_LOG_DEBUG << "settingsCookieValue is invalid";
                        errcode = 2;
                    }

                    GetTelemetryInterface()->Metric_LOGIN_COOKIE_SAVE (name.toUtf8(), lTrimmedValue.toUtf8(), expirationDate.toUtf8(), errcode, count);
                }
            }
            else
            {
                ORIGIN_LOG_DEBUG << "Didn't find cookie named: " << name << ", setting to invalid value";
                Services::writeSetting(settingKey, invalidSettingValue);
            }
        }

        void LoginViewController::clearRememberMeCookie ()
        {
            //we want to store the rememberME cookie & the associated userID in tandem
            //this is to handle the following scenario:
            //1. user A logged in online with remember me set, exit Origin
            //2. user B logs into offline mode with no internet, later internet is restored, user B selects "go online"
            //if remember me cookie isn't cleared after user B is logged in offline, it will auto-log in user A when user B selects "go online"

            ORIGIN_LOG_DEBUG << "clearing Remember me";

            QString envname = Origin::Services::readSetting(Services::SETTING_EnvironmentName, Services::Session::SessionRef());
            const Services::Setting &rememberMeSetting = envname == Services::SETTING_ENV_production ? Services::SETTING_REMEMBER_ME_PROD : Services::SETTING_REMEMBER_ME_INT;
            const Services::Setting &rememberMeUserIdSetting = envname == Services::SETTING_ENV_production ? Services::SETTING_REMEMBER_ME_USERID_PROD : Services::SETTING_REMEMBER_ME_USERID_INT;

            //if we want to clearCookie, don't clear mCurrentRememberMeCookie just yet, since we want to use it to remove it from the cookie jar
            clearCookieIfStale (Session::LOGIN_COOKIE_NAME_REMEMBERME, mCurrentRememberMeCookie, Origin::Services::NetworkCookieJar::cookieValue, rememberMeSetting, true);

            mCurrentRememberMeCookie.clear();
            Services::writeSetting (rememberMeSetting, SETTING_INVALID_REMEMBER_ME);
            Services::writeSetting (rememberMeUserIdSetting, SETTING_INVALID_REMEMBER_ME_USERID);

            settingsFileSyncHelper();
        }

        void LoginViewController::saveRememberMeCookie (const QString userID, bool syncSettingsFile /*= true*/)
        {
            //we want to store the rememberME cookie & the associated userID in tandem
            //this is to handle the following scenario:
            //1. user A logged in online with remember me set, exit Origin
            //2. user B logs into offline mode with no internet, later internet is restored, user B selects "go online"
            //if remember me cookie isn't cleared after user B is logged in offline, it will auto-log in user A when user B selects "go online"
            //when userB is logged into offline mode, clearRememberMe cookie will be called

            QString envname = Origin::Services::readSetting(Services::SETTING_EnvironmentName, Services::Session::SessionRef());
            const Services::Setting &rememberMeSetting = envname == Services::SETTING_ENV_production ? Services::SETTING_REMEMBER_ME_PROD : Services::SETTING_REMEMBER_ME_INT;
            const Services::Setting &rememberMeUserIdSetting = envname == Services::SETTING_ENV_production ? Services::SETTING_REMEMBER_ME_USERID_PROD : Services::SETTING_REMEMBER_ME_USERID_INT;

            clearCookieIfStale (Session::LOGIN_COOKIE_NAME_REMEMBERME, mCurrentRememberMeCookie, Origin::Services::NetworkCookieJar::cookieValue, rememberMeSetting, false);

            // During offline login the remid cookie is removed from the jar because of a bug in the login page that prevents
            // a user not associated with the cookie from a successful login. persistCookie() invalidates the cookie in settings
            // if it is not in the jar. This causes the owner of the cookie to lose it when logging in offline which makes switching to
            // online mode without re-entering credentials impossible.
            // So only persist if we went thru the online login page
            // NOTE: if we logged in online via the offline page, remid is NOT returned by the EADP server even when "remember me" was checked on the offline login page
            // so we're safe to ignore persisting it if the login page was OFFLINE
            // also verified that the remid isn't validated by logging in online from the offline login page, so that next time, when logging in online from the ONLINE page
            // the user is auto-logged in using the saved remid
            if (!mLoginType.contains("Offline", Qt::CaseInsensitive))
            {
                // Persist remember me cookie
                persistCookie(Session::LOGIN_COOKIE_NAME_REMEMBERME, mCurrentRememberMeCookie, rememberMeSetting, Services::SETTING_INVALID_REMEMBER_ME);
                if (mCurrentRememberMeCookie.isEmpty())
                {
                    //we want to "clear it"
                    Services::writeSetting (rememberMeUserIdSetting, SETTING_INVALID_REMEMBER_ME_USERID);
                }
                else if (!userID.isEmpty())
                {
                    // save of userID associated with the remember me cookie
                    Services::writeSetting (rememberMeUserIdSetting, userID);
                }
                if( syncSettingsFile )
                {
                    settingsFileSyncHelper();
                }
            }
        }

        void LoginViewController::loadPersistentCookies()
        {
            QString envname = Origin::Services::readSetting(Services::SETTING_EnvironmentName, Services::Session::SessionRef());
            const Services::Setting &rememberMeSetting = envname == Services::SETTING_ENV_production ? Services::SETTING_REMEMBER_ME_PROD : Services::SETTING_REMEMBER_ME_INT;
            const Services::Setting &usernameCookieSetting = envname == Services::SETTING_ENV_production ? Services::SETTING_MOST_RECENT_USER_NAME_PROD : Services::SETTING_MOST_RECENT_USER_NAME_INT;
            const Services::Setting &tfaIdCookieSetting = envname == Services::SETTING_ENV_production ? Services::SETTING_TFAID_PROD : Services::SETTING_TFAID_INT;

            // remember me
            // If the offline web application cache doesn't exist we can't allow a remember-me login because
            // there won't be enough time for the cache to download.
            if (validateOfflineWebApplicationCache() == Exists)
            {
                loadPersistentCookie(Session::LOGIN_COOKIE_NAME_REMEMBERME, mCurrentRememberMeCookie, rememberMeSetting,
                    Services::SETTING_INVALID_REMEMBER_ME);
            }

            //if the rememberme cookie is clear, we should also clear the associated remembemeUser
            if (mCurrentRememberMeCookie.isEmpty())
            {
                const Services::Setting &rememberMeUserIdSetting = envname == Services::SETTING_ENV_production ? Services::SETTING_REMEMBER_ME_USERID_PROD : Services::SETTING_REMEMBER_ME_USERID_INT;
                Services::writeSetting(rememberMeUserIdSetting, SETTING_INVALID_REMEMBER_ME_USERID);
            }

            // last user to login
            loadPersistentCookie(Session::LOGIN_COOKIE_NAME_LASTUSERNAME, mCurrentUsernameCookie, usernameCookieSetting,
                Services::SETTING_INVALID_MOST_RECENT_USER_NAME);

            //TFA id
            loadPersistentCookie(Session::LOGIN_COOKIE_NAME_TFA_REMEMBERMACHINE, mCurrentTFAIdCookie, tfaIdCookieSetting,
                Services::SETTING_INVALID_TFAID);
        }

        void LoginViewController::loadPersistentCookie(const QString& cookieName, QString& cookieValue, const Services::Setting& settingKey, const QString& invalidSettingValue)
        {
            QString settingsCookieValue(Services::readSetting(settingKey));
            cookieValue.clear();

            if (!settingsCookieValue.isEmpty() &&
                settingsCookieValue != invalidSettingValue)
            {
                QStringList tokens = settingsCookieValue.split("||", QString::SkipEmptyParts);
                //token.size() == 3 is the old way, need to be backwards compatible, but once re-saved out it will save off the cookie.path()
                if (tokens.size() == 3 || tokens.size() == 4)
                {
                    //pre 9.4 latin locales used Qt::TextDate
                    //9.3.11 forced non-latin locales to use Qt::ISODate because Qt4 couldn't encrypt/decrypt the string in settings correctly
                    //9.4: switch to all locales using Qt::ISODate but need to be backwards compatible so we need to check both
                    QDateTime expiration = QDateTime::fromString(tokens.at(2), Qt::ISODate);
                    if (!expiration.isValid())
                    {
                        //try TextDate
                        expiration = QDateTime::fromString(tokens.at(2), Qt::TextDate);

                        //Qt4 converts QString from QByteArray as ascii
                        //Qt5 converts QString from QByteArray as utf8
                        //so now, the foreign latin characters (e.g.  in French) gets garbaged
                        if (!expiration.isValid())
                        {
                            //setting is stored as QByteArray, force it to be converted to Ascii (use Latin1() since Ascii is now deprecated with Qt5)
                            QString settingsCookieValueAscii(Services::readSettingAsAscii(settingKey));
                            tokens = settingsCookieValueAscii.split("||", QString::SkipEmptyParts);
                            if (tokens.size() == 3 || tokens.size() == 4)
                            {
                                expiration = QDateTime::fromString(tokens.at(2), Qt::TextDate);
                            }
                        }
                    }
                    expiration.setTimeSpec(Qt::UTC);

                    ORIGIN_LOG_DEBUG << "cookie loaded: " << cookieName << ". Expires: " << expiration.toUTC().toString();

                    if (cookieName == Session::LOGIN_COOKIE_NAME_REMEMBERME || cookieName == Session::LOGIN_COOKIE_NAME_TFA_REMEMBERMACHINE)
                    {
                        QString cvalue = tokens.at(0);
                        QString lTrimmedValue = cvalue;
                        if (!cvalue.isEmpty())
                        {
                            //get the last 10 digits of the cookie
                            lTrimmedValue = cvalue.right(10);
                        }
                        GetTelemetryInterface()->Metric_LOGIN_COOKIE_LOAD(cookieName.toUtf8(), lTrimmedValue.toUtf8(), expiration.toString().toUtf8());
                    }

                    if (expiration.isValid() && expiration.toUTC() > QDateTime::currentDateTimeUtc())
                    {
                        QNetworkCookie cookie(cookieName.toUtf8(), tokens.at(0).toUtf8());
                        cookie.setExpirationDate (expiration);

                        QUrl url;
                        url.setHost(tokens.at(1));

                        if (tokens.size() == 4)
                        {
                            QString cPath = tokens.at(3);
                            cookie.setPath (cPath);
                        }
                        else
                        if (cookieName == Session::LOGIN_COOKIE_NAME_REMEMBERME)  //kludge to handle old version where we weren't storing out the cookie path
                        {
                            cookie.setPath ("/connect");
                        }

                        QList<QNetworkCookie> cookieList;
                        cookieList.push_back(cookie);

                        bool set = mCookieJar->setCookiesFromUrl(cookieList, url);
                        if (set)
                            cookieValue = cookie.value();
                    }
                    else if (expiration.isValid())
                    {
                        reportLoadPersistanceError (cookieName, ClientCookieError_Expired);
                    }
                    else //expiration is invalid
                    {
                        reportLoadPersistanceError (cookieName, ClientCookieError_ExpirationInvalid);
                    }
                }
                else
                {
                    reportLoadPersistanceError (cookieName, ClientCookieError_CookieInvalid);
                }

                if (cookieValue.isEmpty())
                {
                    // clear the cookie from settings
                    Services::writeSetting(settingKey, invalidSettingValue);

                    ORIGIN_LOG_DEBUG << "clearing cookie: " << cookieName;
                }
            }
        }

        void LoginViewController::reportLoadPersistanceError (const QString& cookieName, ClientCookieErrorType cookieError)
        {
            if (cookieName != Session::LOGIN_COOKIE_NAME_REMEMBERME && cookieName != Session::LOGIN_COOKIE_NAME_TFA_REMEMBERMACHINE)
            {
                ORIGIN_LOG_EVENT << "Load errortype: " << cookieError;
                return; //no error codes set up for other cookies, just return
            }

            Origin::Services::restError errCode;
            if (cookieName == Session::LOGIN_COOKIE_NAME_REMEMBERME)
                errCode = restErrorAuthenticationClientRememberMe;
            else
                errCode = restErrorAuthenticatinClientTFARememberMachine;

            QString errorNumStr = QString("%1").arg(errCode);

            //sendAuthenticationErrorTelemetry will actually log an event with the error string as well
            sendAuthenticationErrorTelemetry (errorNumStr, "", 0, QNetworkReply::NoError, cookieError); 
        }

        void LoginViewController::onOfflineLoginComplete(const QString& primaryCid, const QString& backupCid)
        {
            mLoginFlowIdRefreshTimer.stop();
            mLoginLoadTimer.stop();

            closePopupWindow();
            if (mLoginWindow)
            {
                mLoginWindow->hide();
            }

            emit offlineLoginComplete(primaryCid, backupCid);
        }

        bool LoginViewController::isLoginWindowVisible()
        {
            if (mLoginWindow && mLoginWindow->isVisible())
                return true;

            return false;
        }

        void LoginViewController::pollOfflineCacheStatus()
        {
            OfflineCacheType cacheState = validateOfflineWebApplicationCache();

            if (cacheState != Exists)
            {
                // Has offline cache download timed out
                if (mCacheValidationSecondsLeft--)
                {
                    // Poll again in 1 second
                    QTimer::singleShot(1000, this, SLOT(pollOfflineCacheStatus()));
                    return;
                }
                else
                {
                    // Report offline cache download timeout
                    ORIGIN_LOG_WARNING << "The offline web application cache download timed out";
                    GetTelemetryInterface()->Metric_WEB_APPLICATION_CACHE_DOWNLOAD();
                }
            }

            show(mStartupState);
        }

        void LoginViewController::onInitialPageLoad()
        {
            // During destruction, the mLoginWindow pointer is assigned NULL, but since the login window
            // is deleted via deleteLater, it's possible for this signal to still fire after that.
            if (mLoginWindow)
            {

                // Disconnect the signal/slot here so we only get this fired once
                // We only want to track the first spinner for the login page loading not the second for logging in
                ORIGIN_VERIFY_DISCONNECT(mLoginWindow->webview()->webview(), SIGNAL(loadFinished(bool)), this, SLOT(onInitialPageLoad()));
                
                // If we're not automatically logging in, end timer for how long it took the server to return the valid information to load the login page. 
                if(!mAutoLogin)
                {
                    GetTelemetryInterface()->Metric_PERFORMANCE_TIMER_END(EbisuTelemetryAPI::LoginPageLoaded, 0);
                }

                // Goofy workaround for the following Jira tickets
                // EBIBUGS-25664
                // EBIBUGS-25665
                //
                // Now that the webview has finished loading we can safely minimize it, knowing that the
                // window's restore size is properly set to the size of the web content
                if (mStartupState == StartInTaskbar)
                {
                    // There's another bug that I suspect is in the UI Toolkit that causes an OriginWindow to freak out and become unresponsive
                    // if you call showMinimized() on a window before you call show. Additionally, you need to pump messages or return control
                    // to the app between calling show and showMinimized. I've entered a jira ticket against that issue.
                    // EBIBUGS-25830 - OriginWindow does not restore to the proper size if the size is changed while minimized

                    //MY: hack work around to remove the scrollbar & show spinner for the empty window that
                    //flashes for the case when remember me is set.  EVEN THO THESE CALLS ARE ALREADY MADE DURING INIT above
                    //for some reason, when logging into OAuth with remember me cookie, it doesn't return a valid page to load
                    //so the values that get set above seems to get reset.
                    OriginWebView* originLoginWebView = mLoginWindow->webview();
                    originLoginWebView->setHasSpinner(true);
                    originLoginWebView->setVScrollPolicy(Qt::ScrollBarAlwaysOff);
                    originLoginWebView->setHScrollPolicy(Qt::ScrollBarAlwaysOff);

                    //reset it for any other load
                    originLoginWebView->setPreferredContentsSize (QSize());

                    //Qt5 doesn't allow us to call showMinimize() directly; it seems to need us to call show()
                    //and then call showMinimized() in the next event loop
                    mLoginWindow->show();

                    QMetaObject::invokeMethod(this, "onShowMinimized", Qt::QueuedConnection);
                }
            }
        }

        void LoginViewController::onShowMinimized()
        {
            mLoginWindow->showMinimized();
        }

        void LoginViewController::settingsFileSyncHelper()
        {
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


        void LoginViewController::validateOfflineCacheAndShow()
        {
            mCacheValidationSecondsLeft = sOfflineCacheDownloadTimeout;
            pollOfflineCacheStatus();
        }

        void LoginViewController::show(const StartupState& startupState)
        {
            setupSystemTray();

            switch(startupState)
            {
            default:
            case StartNormal:
                {
                    // Try to show the window on the screen that the client closed was on.
                    int clientPreviousScreen = Services::readSetting(Services::SETTING_APPLOGOUTSCREENNUMBER);
                    // If the screen doesn't exist anymore (disconnected) - show on primary screen
                    if(clientPreviousScreen > QApplication::desktop()->screenCount())
                        clientPreviousScreen = QApplication::desktop()->primaryScreen();

                    if(mLoginWindow->manager())
                        mLoginWindow->manager()->centerWindow(clientPreviousScreen);

                    mLoginWindow->showWindow();
                    QApplication::alert(mLoginWindow);
                }
                break;
            case StartInTaskbar:
                // On Mac, if we showMinimized() then the login window will appear for a split second and
                // then minimize down to the dock, creating an additional dock icon (there's already a dock
                // icon for the Origin client).  Skipping this call creates the desired user experience -
                // only the Origin icon appears in the dock, and clicking on it brings up the login window.
#ifdef ORIGIN_PC
                //show gets called when page is loaded, which then invokes showminimized, but doing it as part of show here
                //causes flakiness to the window when we move the window around after unminimizing
//                    mLoginWindow->showMinimized();
#endif
                break;
            case StartInSystemTray:
                break;
            }
            ORIGIN_LOG_EVENT << "Show login window";
        }

        void LoginViewController::closeLoginWindow()
        {
            ORIGIN_LOG_EVENT << "Closing login window.";

            mLoginFlowIdRefreshTimer.stop();
            mLoginLoadTimer.stop();

            if(mLoginWindow)
            {
                if (mAuthenticationWebController)
                {
                    ORIGIN_VERIFY_DISCONNECT(mAuthenticationWebController->page()->currentFrame(), SIGNAL(urlChanged ( const QUrl&)),this, SLOT(onUrlChanged(const QUrl&)));
                    ORIGIN_VERIFY_DISCONNECT(mAuthenticationWebController->page(), SIGNAL(loadFinished(bool)), this, SLOT(resetCacheLoadControl()));
                    ORIGIN_VERIFY_DISCONNECT(mAuthenticationWebController->page(), SIGNAL(loadFinished(bool)), this, SLOT(onPageLoaded()));
                }

                if (Origin::Services::Connection::ConnectionStatesService::instance() != NULL)
                {
                    ORIGIN_VERIFY_DISCONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(globalConnectionStateChanged(bool)),
                        this, SLOT(onConnectionChange(bool)));
                }

                ORIGIN_VERIFY_DISCONNECT(mLoginWindow, SIGNAL(rejected()), 0, 0);
                mLoginWindow->hide();
                mLoginWindow->deleteLater();
                mLoginWindow = NULL;
            }
        }

        void LoginViewController::onUserClosedWindow()
        {
            if(mLoginWindow)
            {
                // Write the position of the primary window on logout/exit.
                Services::writeSetting(Services::SETTING_APPLOGOUTSCREENNUMBER, QApplication::desktop()->screenNumber(mLoginWindow));
                closeLoginWindow();
                emit userCancelled();
            }
        }

        void LoginViewController::showBanner(UIToolkit::OriginBanner::IconType icon, const QString& text)
        {
            mBannerText = text;
            mBannerIconType = icon;
            mLoginFlowIdRefreshTimer.stop();
            loadLoginPage();
        }

        void LoginViewController::showErrorMessage(const QString& errorMessage)
        {
            mCookieJar->clear();
            mBannerIconType = UIToolkit::OriginBanner::Error;
            mBannerText = errorMessage;

            if (!mCurrentRememberMeCookie.isEmpty())
            {
                QString envname = Origin::Services::readSetting(Services::SETTING_EnvironmentName, Services::Session::SessionRef());
                const Services::Setting &rememberMeSetting = envname == Services::SETTING_ENV_production ? Services::SETTING_REMEMBER_ME_PROD : Services::SETTING_REMEMBER_ME_INT;

                mCookieJar->removeCookieByValue(mCurrentRememberMeCookie);
                mCurrentRememberMeCookie.clear();
                Services::writeSetting(rememberMeSetting, Services::SETTING_INVALID_REMEMBER_ME);
            }

            ORIGIN_VERIFY_CONNECT(mAuthenticationWebController->page(), SIGNAL(loadFinished(bool)), this, SLOT(onLoadErrorFinished(bool)));
            loadLoginPage();

            if (!mLoginWindow->isVisible())
            {
                show();
            }
        }

        void LoginViewController::onRequestFinished(QNetworkReply* reply)
        {
            static QUrl redirectUrl;
            if (!reply) return;

            const QUrl replyUrl = reply->url();
            int httpResponse = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

            // Qt returns a response code of 0 for qrc resources and sometimes sets the response to 0 for http responses even though a 502 was returned by the network.
            // Don't consider a code of 0 for qrc responses an error.
            bool isHttpError = httpResponse >= 400 || (httpResponse == 0 && !replyUrl.toString().contains("qrc", Qt::CaseInsensitive));
            if (isHttpError && redirectUrl == replyUrl)
            {
                mOfflineCacheStatus = validateOfflineWebApplicationCache();
                if (mOfflineCacheStatus != Exists)
                {
                    ORIGIN_LOG_ERROR << "Qt error code: " << reply->error() << ", HTTP response: " << httpResponse;
                    ORIGIN_LOG_ERROR << "Qt error string: " << reply->errorString();
                    #ifdef _DEBUG 
                        ORIGIN_LOG_ERROR << "Error for request: " << reply->request().url().toString();
                    #else
                        ORIGIN_LOG_ERROR << "Error for request: " << reply->request().url().scheme() << "://" << reply->request().url().host() << reply->request().url().path();
                    #endif

                    QString telemUrlInfo = generateTelemtryUrlWithParamInfo(reply->request().url());
                    sendAuthenticationErrorTelemetry(QString("%1").arg(restErrorAuthenticationHttp), telemUrlInfo, httpResponse, reply->error());

                    showFatalErrorDialog(tr("ebisu_client_login_offline_error"));
                    return;
                }
            }
            redirectUrl = reply->header(QNetworkRequest::LocationHeader).toUrl();

            if ((replyUrl != mAuthenticationWebController->page()->mainFrame()->url()) &&
                (replyUrl != mAuthenticationWebController->page()->mainFrame()->requestedUrl()))
            {
                // This is for another resource on the page, AJAX request etc
                // Ignore any failures that are not thrown by the main frame
                return;
            }

            const QNetworkReply::NetworkError error = reply->error();

            //OperationCanceledError only happens when abort is called, this can happen when a page is reloaded. We want to ignore the error if its due to a page reload
            //a page reload can happen when the internet connection has changed

            if ((error != QNetworkReply::NoError) && (error != QNetworkReply::OperationCanceledError))
            {
                onErrorDetected(error, reply);
            }


            //if we get the canceled error we need to stop the loginloadtimer
            if(error == QNetworkReply::OperationCanceledError)
            {
                mLoginLoadTimer.start();
            }
        }

        void LoginViewController::onTriggerActionWebpageStop(QNetworkReply* reply)
        {
            Q_UNUSED(reply)

            ORIGIN_VERIFY_CONNECT(mAuthenticationWebController->page()->networkAccessManager(), SIGNAL(finished(QNetworkReply *)), 
                    this, SLOT(onRequestFinished(QNetworkReply *)));

            ORIGIN_VERIFY_DISCONNECT(mAuthenticationWebController->page()->networkAccessManager(), SIGNAL(finished(QNetworkReply *)), 
                    this, SLOT(onTriggerActionWebpageStop(QNetworkReply *)));

            emit loadCanceled();
        }


        void LoginViewController::processLoadCanceled()
        {
            emit loginError();

            // Force offline mode login if we aren't already attempting to
            if(!mLoggingInOffline)
            {
                ORIGIN_LOG_EVENT << "load offline login page";
                loadLoginPage(true);
            }
            else
            {
                showFatalErrorDialog(tr("ebisu_client_login_error"));
            }
        }

        void LoginViewController::onLoadTimeout()
        {
            mLoginLoadTimer.stop();

            ORIGIN_LOG_ERROR << "login page load timed out";

            GetTelemetryInterface()->Metric_LOGIN_FAIL(
                QString::number(restErrorNetwork_TimeoutError).toLatin1().data(),
                "LoadCanceled",
                mLoginType.toUtf8().data(), restErrorNetwork_TimeoutError);
            Heartbeat::instance()->setLoginError(restErrorNetwork_TimeoutError);

            initiateCancelLoad();
        }

        void LoginViewController::onErrorDetected(QNetworkReply::NetworkError error, QNetworkReply *reply)
        {   
            bool alwaysTreatHttpResponseCodeAsError = false;
            QString telemUrlInfo;

            int httpResponse = 0;
            QString requestUrl;

            emit loginError();

            if (reply)
            {
                httpResponse = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

                requestUrl = QString("%1%2").arg(reply->request().url().host()).arg(reply->request().url().path());

                // Used in telemetry.
               telemUrlInfo = generateTelemtryUrlWithParamInfo(reply->request().url());

                ORIGIN_LOG_EVENT << "Qt error code: " << error << ", HTTP response: " << httpResponse;
                ORIGIN_LOG_EVENT << "Qt error string: " << reply->errorString();
#ifdef _DEBUG
                ORIGIN_LOG_EVENT << "Error for request: " << reply->request().url().toString();
#else
                ORIGIN_LOG_EVENT << "Error for request: " << reply->request().url().scheme() << "://" << requestUrl;
#endif

                // Treat all http reponse codes >= 400 as errors. Sometimes 502s created with Fiddler are showing up with a response code of 0.
                alwaysTreatHttpResponseCodeAsError = httpResponse >= 400 || httpResponse <= 0;
            }

            if(!alwaysTreatHttpResponseCodeAsError &&
                (error == QNetworkReply::NoError || 
                error == QNetworkReply::OperationCanceledError ||
                error == QNetworkReply::ProtocolUnknownError))
            {
                ORIGIN_LOG_EVENT << "Ignoring error.";
                return;
            }

            ORIGIN_LOG_ERROR << "Network error. Failed to load login or registration page. error:" << error;

            if(error == QNetworkReply::TimeoutError)
            {
                GetTelemetryInterface()->Metric_LOGIN_FAIL(
                    QString::number(restErrorNetwork_TimeoutError).toLatin1().data(),
                    "LoadTimedOut",
                    mLoginType.toUtf8().data(),
                    httpResponse,
                    telemUrlInfo.left(160).toLocal8Bit().data(),
                    error);
                Heartbeat::instance()->setLoginError(restErrorNetwork_TimeoutError);
            }
            else
            {
                if (reply && (httpResponse == 400 || httpResponse == 500))
                {
                    //if it's SSO login and we're getting back a 400, assume that there was
                    //something bad with the SSO token, so request credentials
                    //(we aren't able to parse the response body to see if it's a bad SSO token)
                    if (mLoginType.compare("sso", Qt::CaseInsensitive) == 0)
                    {
                        sendAuthenticationErrorTelemetry(QString("%1").arg(restErrorAuthenticationSSO), telemUrlInfo, httpResponse, error);
                        mLoginType = "Manual";
                        emit typeOfLogin(mLoginType, mAutoLogin);
                        mSsoToken.clear();
                        //if we started out minimized, we need to unminimize so the user knows to enter credentials
                        if (mStartupState == StartInTaskbar || mStartupState == StartInSystemTray)
                        {
                            setupSystemTray();
                            mStartupState = StartNormal;
                        }
                        loadLoginPage();
                        return;
                    }
                    else if ((httpResponse == 500) && (mLoginType.compare ("auto-login", Qt::CaseInsensitive) == 0))
                    {
                        //the server consumes the remid cookie even when it returns a 500, so we need to remove it
                        //otherwise we'll get a invalid-rem-sto (reuse) error next time
                        clearRememberMeCookie();      //since the cookie was bad, we want to make sure we clear the persistence

                        // Auto-login failed so type of login is now Manual
                        mLoginType = "Manual";
                        mAutoLogin = false;
                        emit typeOfLogin(mLoginType, mAutoLogin);
                    }
                }

                if (reply)
                {  
                    //stupid Qt returns an error when trying to load the offline page even tho we tell it to load it from the cache
                    //so we don't want to report that as a login failure
                    //however, it will also prevent reporting of the inability to load the page when applicationCache exists but is missing something for offline login
                    //but hopefully that will be a rare case
                    //failures from trying to login online from the offline page should still go fall thru because the requestUrl should not be the offlineLoginUrl()

                    //we don't want to put it as part of the check with if (reply) above because then it will fall down to the else if statement and end up reporting
                    //restErrorAuthenticationOfflineAppCache

                    //since offlineLoginUrl() returns all the params and stuff, and requestUrl only has the host and path, check for requestUrl within offlineLoginUrl() instead of
                    //the other way around
                    if (offlineLoginUrl().isEmpty() || !offlineLoginUrl().contains (requestUrl))
                        sendAuthenticationErrorTelemetry(QString("%1").arg(restErrorAuthenticationHttp), telemUrlInfo, httpResponse, error, 0, reply->errorString());
                }
                else if (mLoggingInOffline)
                {
                    switch(mOfflineCacheStatus)
                    {
                        case Missing:
                            if(offlineLoginUrl().isEmpty())
                                sendAuthenticationErrorTelemetry(QString("%1").arg(restErrorAuthenticationOfflineUrlMissing));
                            else
                                sendAuthenticationErrorTelemetry(QString("%1").arg(restErrorAuthenticationOfflineAppCacheMissing));
                            break;
                        case Corrupt:
                            sendAuthenticationErrorTelemetry(QString("%1").arg(restErrorAuthenticationOfflineAppCacheCorrupt));
                            break;
                        default:
                            sendAuthenticationErrorTelemetry(QString("%1").arg(restErrorAuthenticationOfflineAppCache));
                            break;
                    }
                }
                else
                    sendAuthenticationErrorTelemetry(QString("%1").arg(restErrorAuthenticationClient));
            }

            // Force offline mode login if we aren't already attempting to
            if(!mLoggingInOffline)
            {
                ORIGIN_LOG_EVENT << "load offline login page";
                loadLoginPage(true);
                return;
            }

            if(mOfflineCacheStatus == Missing || mOfflineCacheStatus == Corrupt || (reply && reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool() == true))
            {
                // If we get to here, the login page failed to load over the network and it is not available in the application cache
                //need to connect this so the user can close the error dialog
                ORIGIN_VERIFY_DISCONNECT(mLoginWindow, SIGNAL(rejected()), 0, 0);	//disconnect any previous
                ORIGIN_VERIFY_CONNECT(mLoginWindow, SIGNAL(rejected()), this, SLOT(onUserClosedWindow()));

                QString errorMessage;
                if (reply)
                {
                    ORIGIN_LOG_ERROR << reply->errorString();

                    errorMessage = tr("ebisu_client_error_uppercase");
                    errorMessage = QString("%1 %2").arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString()).arg(errorMessage);
                }
                else
                {
                    switch (mOfflineCacheStatus)
                    {
                        case Missing:
                            errorMessage = tr("ebisu_client_login_offline_error");
                            break;
                        case Corrupt:
                            errorMessage = tr("ebisu_client_login_offline_error");
                            break;
                        default:
                            errorMessage = tr("ebisu_client_login_error");
                            break;
                    }
                }

                showFatalErrorDialog(errorMessage);
            }
        }

        void LoginViewController::showFatalErrorDialog(const QString& errorMessage)
        {
            QFile resourceFile(QString(":html/errorPage_dialog.html"));
            if (!resourceFile.open(QIODevice::ReadOnly))
            {
                ORIGIN_ASSERT(0);
                    return;
            }

            // Replace our stylesheet
            QString stylesheet = UIToolkit::OriginWebView::getErrorStyleSheet(false);

            QString p = QString::fromUtf8(resourceFile.readAll())
            .arg(stylesheet)
            .arg(tr("ebisu_client_login_error_uppercase"))
            .arg(errorMessage);

            ORIGIN_LOG_EVENT << "Loading generic login error page";
            mAuthenticationWebController->page()->mainFrame()->setHtml(p);

            if (mLoginWindow != NULL)
                mLoginWindow->show();
        }

        void LoginViewController::getOemInformation(QString& tag) const
        {
    #ifdef WIN32
            // Check for the presence of OEM files
            TCHAR sBaseDirBuffer[MAX_PATH+1]; 
            SHGetSpecialFolderPath(NULL, sBaseDirBuffer, CSIDL_COMMON_APPDATA, false);
            QString sOEMDir = QString::fromWCharArray(sBaseDirBuffer);
            if (!sOEMDir.endsWith("\\"))
            {
                sOEMDir.append("\\");
            }
            sOEMDir.append("Origin\\OEM\\");
            QFile originOEM(sOEMDir + "origin.oem");
            if (originOEM.open(QIODevice::ReadOnly))
            {
                QString oemvalueStr = originOEM.readAll();
                if (!oemvalueStr.isEmpty())
                {
                    tag = "eadm-origin-"+ oemvalueStr;
                }
            }
    #endif
        }

        void LoginViewController::showPopupDialog(const QUrl& url)
        {
            if(mPopupWindow == NULL)
            {
                mPopupWindow = new OriginWindow(OriginWindow::Icon|OriginWindow::Minimize|OriginWindow::Close, NULL, OriginWindow::WebContent);
                ORIGIN_VERIFY_CONNECT(mPopupWindow, SIGNAL(rejected()), this, SLOT(closePopupWindow()));
                ORIGIN_VERIFY_CONNECT(mLoginWindow, SIGNAL(rejected()), this, SLOT(closePopupWindow()));

                OriginWebView* popupWebView = mPopupWindow->webview();
                popupWebView->setHasSpinner(true);
                popupWebView->setChangeLoadSizeAfterInitLoad(true);
                popupWebView->setVScrollPolicy(Qt::ScrollBarAlwaysOff);
                popupWebView->setHScrollPolicy(Qt::ScrollBarAlwaysOff);

                OriginWebController* popupWebController = new OriginWebController(popupWebView->webview(), NULL, false);
                popupWebController->errorDetector()->setErrorCheckEnabled(Origin::Client::PageErrorDetector::MainFrameRequestFailed, true);

                AuthenticationJsHelper* popupJsHelper = new AuthenticationJsHelper(popupWebController);
                ORIGIN_VERIFY_CONNECT(popupJsHelper, SIGNAL(close()), this, SLOT(closePopupWindow()));
                popupWebController->jsBridge()->addHelper("authenticationJsHelper", popupJsHelper);

                popupWebController->page()->mainFrame()->load(url);
            }
            
            mPopupWindow->showWindow();
        }

        void LoginViewController::closePopupWindow()
        {
            if (mPopupWindow)
            {
                ORIGIN_VERIFY_DISCONNECT(mPopupWindow, SIGNAL(rejected()), this, SLOT(closePopupWindow()));
                ORIGIN_VERIFY_DISCONNECT(mLoginWindow, SIGNAL(rejected()), this, SLOT(closePopupWindow()));
                mPopupWindow->close();
                mPopupWindow = NULL;
            }
        }

        QString LoginViewController::generateTelemtryUrlWithParamInfo(const QUrl &url)
        {
            QString requestUrl = QString("%1%2").arg(url.host()).arg(url.path());
            QString telemUrlInfo = requestUrl + ">";
            
            QUrlQuery urlQuery(url);
            const QList<QPair<QString, QString> > &queryItems = urlQuery.queryItems();
            for(QList<QPair<QString, QString> >::ConstIterator it = queryItems.constBegin();
                it != queryItems.constEnd();
                it++)
            {
                QString  temp = it->first;
                temp.truncate(5);
                if(it != queryItems.constBegin())
                    telemUrlInfo.append("-");
                telemUrlInfo.append(temp);
            }

            return telemUrlInfo;
        }

        void LoginViewController::setShowAgeUpFlow (bool show) const
        {
            AuthenticationJsHelper *authenticationJsHelper = dynamic_cast<AuthenticationJsHelper *>(mAuthenticationWebController->jsBridge()->helper("authenticationJsHelper"));
            if (authenticationJsHelper)
            {
                authenticationJsHelper->setShowAgeUpFlow (show);
            }
        }

        void LoginViewController::initiateCancelLoad()
        {
            ORIGIN_VERIFY_DISCONNECT(mAuthenticationWebController->page()->networkAccessManager(), SIGNAL(finished(QNetworkReply *)),
                this, SLOT(onRequestFinished(QNetworkReply *)));

            ORIGIN_VERIFY_CONNECT(mAuthenticationWebController->page()->networkAccessManager(), SIGNAL(finished(QNetworkReply *)),
                this, SLOT(onTriggerActionWebpageStop(QNetworkReply *)));

            mAuthenticationWebController->page()->triggerAction(QWebPage::Stop);
        }

        void LoginViewController::setupSystemTray()
        {
            EbisuSystemTray* systemTray = EbisuSystemTray::instance();
            if (systemTray->trayMenu() == NULL)
            {
                LoginSystemMenu* trayMenu = new LoginSystemMenu();
                ORIGIN_VERIFY_CONNECT(trayMenu, SIGNAL(openTriggered()), systemTray, SLOT(showPrimaryWindow()));
                ORIGIN_VERIFY_CONNECT(trayMenu, SIGNAL(quitTriggered()), this, SLOT(onSystemMenuExit()));
                systemTray->setTrayMenu(trayMenu);
            }
            systemTray->setPrimaryWindow(mLoginWindow);
        }

        void LoginViewController::onSystemMenuExit()
        {
            OriginApplication::instance().cancelTaskAndExit(ReasonSystrayExit);
        }
    }
}
