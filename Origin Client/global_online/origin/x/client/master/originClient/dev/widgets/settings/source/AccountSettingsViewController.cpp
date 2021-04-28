/////////////////////////////////////////////////////////////////////////////
// AccountSettingsViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#include "AccountSettingsViewController.h"
#include "OriginWebController.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/settings/SettingsManager.h"
#include "WebWidget/FeatureRequest.h"
#include "NativeBehaviorManager.h"
#include "NavigationController.h"
#include "originwebview.h"
#include <QtWebKitWidgets/QWebView>
#include "SettingsJsHelper.h"
#include "TelemetryAPIDLL.h"
#include "SystemInformation.h"
#include "StoreUrlBuilder.h"

namespace Origin
{
namespace Client
{
AccountSettingsViewController::AccountSettingsViewController(QWidget* parent)
: QObject(parent)
, mWebController(NULL)
, mWebViewContainer(new UIToolkit::OriginWebView(parent))
, mCurrentPath(Unset)
{
    // Give the web view a gray background by default
    mWebViewContainer->setPalette(QPalette(QColor(238,238,238)));
    mWebViewContainer->setAutoFillBackground(true);
    mWebViewContainer->setIsContentSize(false);
    mWebViewContainer->setHasSpinner(true);
    mWebViewContainer->setShowSpinnerAfterInitLoad(false);
    auto webview = new QWebView();
    mWebViewContainer->setWebview(webview);

    // Act and look more native
    NativeBehaviorManager::applyNativeBehavior(webview);

    // Make sure you pass it in as a QWebView! Otherwise, it won't load on Mac.
    mWebController = new OriginWebController(mWebViewContainer->webview());
    WebWidget::FeatureRequest clientNavigation("http://widgets.dm.origin.com/features/interfaces/clientNavigation");
    mWebController->jsBridge()->addFeatureRequest(clientNavigation);
    // Verify email clicked
    SettingsJsHelper* settingsJsHelper = new SettingsJsHelper(this);
    mWebController->jsBridge()->addHelper("accountSettingHelper", settingsJsHelper);
    ORIGIN_VERIFY_CONNECT(settingsJsHelper, SIGNAL(emailClicked()), this, SLOT(onEmailClicked()));


    mWebViewContainer->webview()->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    mWebController->errorHandler()->setErrorPage(DefaultErrorPage::centerAlignedPage());
    mWebController->errorHandler()->setHandleOfflineAsError(true);
    mWebController->errorDetector()->disableErrorCheck(PageErrorDetector::FinishedWithFailure);
    mWebController->errorDetector()->disableErrorCheck(PageErrorDetector::MainFrameRequestFailed);
    ORIGIN_VERIFY_CONNECT(mWebController->page()->currentFrame(), SIGNAL(urlChanged(const QUrl&)), this, SLOT(onUrlChanged(const QUrl&)));
    NavigationController::instance()->addWebHistory(SETTINGS_ACCOUNT_TAB, webview->page()->history());
        
    QString locale = Origin::Services::readSetting(Origin::Services::SETTING_LOCALE).toString();

    QUrl homeUrl(Services::readSetting(Services::SETTING_OriginAccountURL).toString());
    QUrl orderHistoryUrl(Services::readSetting(Services::SETTING_StoreOrderHistoryURL).toString());
    QUrl privacyUrl(Services::readSetting(Services::SETTING_StorePrivacyURL).toString());
    QUrl securityUrl(Services::readSetting(Services::SETTING_StoreSecurityURL).toString());
    QUrl subscriptionUrl(Services::readSetting(Services::SETTING_AccountSubscriptionURL).toString());
    QUrl paymentAndShippingUrl(Services::readSetting(Services::SETTING_StorePaymentAndShippingURL).toString());
    QUrl redemptionUrl(Services::readSetting(Services::SETTING_StoreRedemptionURL).toString());
    QString machineHash(NonQTOriginServices::SystemInformation::instance().GetUniqueMachineHash());
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("gameId", StoreUrlBuilder().storeEnvironment());
    urlQuery.addQueryItem("locale", locale); 
    urlQuery.addQueryItem("sourceType", "client");
	urlQuery.addQueryItem("pc_machine_id",machineHash);

    homeUrl.setQuery(urlQuery);
    orderHistoryUrl.setQuery(urlQuery);
    privacyUrl.setQuery(urlQuery);
    securityUrl.setQuery(urlQuery);
    subscriptionUrl.setQuery(urlQuery);
    paymentAndShippingUrl.setQuery(urlQuery);
    redemptionUrl.setQuery(urlQuery);

    mHomePage = homeUrl.toString();
    mOrderHistoryPage = orderHistoryUrl.toString();
    mPrivacyPage = privacyUrl.toString();
    mSecurityPage = securityUrl.toString();
    mSubscriptionPage = subscriptionUrl.toString();
    mPaymentShippingPage = paymentAndShippingUrl.toString();
    mRedemptionPage = redemptionUrl.toString();

    QStringList hosts;
    hosts << ".origin.com";
    hosts << ".ea.com";
    mAuthenticationMonitor.setRelevantHosts(hosts);
    mAuthenticationMonitor.setWebPage(mWebController->page());
    ORIGIN_VERIFY_CONNECT(&mAuthenticationMonitor, SIGNAL(lostAuthentication()), this, SLOT(onLostAuthentication()));
    mAuthenticationMonitor.start();
}


AccountSettingsViewController::~AccountSettingsViewController()
{
    mAuthenticationMonitor.shutdown();
    delete mWebController;
    delete mWebViewContainer;
}


void AccountSettingsViewController::onLostAuthentication()
{
    ORIGIN_LOG_ERROR << "Account settings page lost authentication.";
    mWebController->page()->triggerAction(QWebPage::Stop);
    emit lostAuthentication();
}

void AccountSettingsViewController::onUrlChanged(const QUrl& url)
{
    const QString urlStr = url.toString(QUrl::RemoveQuery);
    if(urlStr.compare(QUrl(mHomePage).toString(QUrl::RemoveQuery), Qt::CaseInsensitive) == 0)
        mCurrentPath = Home;
    else if(urlStr.compare(QUrl(mPrivacyPage).toString(QUrl::RemoveQuery), Qt::CaseInsensitive) == 0)
        mCurrentPath = Privacy;
    else if(urlStr.compare(QUrl(mSecurityPage).toString(QUrl::RemoveQuery), Qt::CaseInsensitive) == 0)
        mCurrentPath = Security;
    else if(urlStr.compare(QUrl(mSubscriptionPage).toString(QUrl::RemoveQuery), Qt::CaseInsensitive) == 0)
        mCurrentPath = Subscription;
    else if(urlStr.compare(QUrl(mOrderHistoryPage).toString(QUrl::RemoveQuery), Qt::CaseInsensitive) == 0)
        mCurrentPath = OrderHistory;
    else if(urlStr.compare(QUrl(mPaymentShippingPage).toString(QUrl::RemoveQuery), Qt::CaseInsensitive) == 0)
        mCurrentPath = PaymentShipping;
    else if(urlStr.compare(QUrl(mRedemptionPage).toString(QUrl::RemoveQuery), Qt::CaseInsensitive) == 0)
        mCurrentPath = Redemption;
    else
        mCurrentPath = Unset;
    NavigationController::instance()->recordNavigation(SETTINGS_ACCOUNT_TAB, url.toString());
#if defined(ORIGIN_MAC)
    // Hack to force a redraw to work around the background clearing hack in ClientView (which was introduced to
    // work around the the transparency issue when switching views).
    mWebViewContainer->update();
#endif
}


QWidget* AccountSettingsViewController::view() const
{
    return mWebViewContainer;
}


void AccountSettingsViewController::loadWebPage(AccountSettingsViewController::Path path /*= AccountSettingsViewController::Home*/, const QString& status /* = "" */)
{
    // Load the page if the path is different
    // or if there is currently an error being displayed. (EBIBUGS-26229)
    if (path != currentPath() || mWebController->errorDetector()->currentErrorType() != PageErrorDetector::NoError)
    {
        switch(path)
        {
            case AccountSettingsViewController::Home:
                mWebController->loadTrustedUrl(mHomePage);
            break;
            case AccountSettingsViewController::Privacy:
            {
                // Clear the current frame URL to force the 'order history' to fully reload.
                // Currently it's broken on the EADP server side.
                mWebController->page()->currentFrame()->setHtml(QString());
                mWebController->loadTrustedUrl(mPrivacyPage);
                break;
            }
            case AccountSettingsViewController::Security:
            {
                // Clear the current frame URL to force the 'order history' to fully reload.
                // Currently it's broken on the EADP server side.
                mWebController->page()->currentFrame()->setHtml(QString());
                mWebController->loadTrustedUrl(mSecurityPage);
                break;
            }
            case AccountSettingsViewController::Subscription:
                {
                    mWebController->page()->currentFrame()->setHtml(QString());
                    QUrl subsUrl = mSubscriptionPage;
                    if(status.count())
                    {
                        QUrlQuery urlQuery;
                        urlQuery.addQueryItem("status", status);
                        subsUrl.setQuery(urlQuery);
                    }
                    mWebController->loadTrustedUrl(subsUrl);
                    break;
                }
            case AccountSettingsViewController::OrderHistory:
            {
                // Clear the current frame URL to force the 'order history' to fully reload.
                // Currently it's broken on the EADP server side.
                mWebController->page()->currentFrame()->setHtml(QString());
                mWebController->loadTrustedUrl(mOrderHistoryPage);
                break;
            }
            case AccountSettingsViewController::PaymentShipping:
                {
                    // Clear the current frame URL to force the 'order history' to fully reload.
                    // Currently it's broken on the EADP server side.
                    mWebController->page()->currentFrame()->setHtml(QString());
                    mWebController->loadTrustedUrl(mPaymentShippingPage);
                    break;
                }
            case AccountSettingsViewController::Redemption:
            {
                // Clear the current frame URL to force the 'order history' to fully reload.
                // Currently it's broken on the EADP server side.
                mWebController->page()->currentFrame()->setHtml(QString());
                mWebController->loadTrustedUrl(mRedemptionPage);
                break;
            }
            default:
                break;

        }

        mCurrentPath = path;
    }
}

void AccountSettingsViewController::onEmailClicked()
{
    GetTelemetryInterface()->Metric_EMAIL_VERIFICATION_STARTS_BANNER();
}

void AccountSettingsViewController::refreshPage(const AccountSettingsViewController::Path& path)
{
    if(path == Unset || mCurrentPath == path)
    {
        // Show the spinner if we are doing a manual refresh
        mWebController->page()->triggerAction(QWebPage::Reload);
    }
}

}
}
