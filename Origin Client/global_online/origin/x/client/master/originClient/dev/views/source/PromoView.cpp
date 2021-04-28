/////////////////////////////////////////////////////////////////////////////
// PromoView.cpp
//
// Copyright (c) 2015, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "PromoView.h"

#include "OriginWebController.h"
#include "PromoJsHelper.h"
#include "SettingsJsHelper.h"
#include "CommonJsHelper.h"

#include "engine/login/LoginController.h"
#include "engine/content/Entitlement.h"
#include "engine/subscription/SubscriptionManager.h"
#include "services/network/NetworkAccessManagerFactory.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "services/platform/TrustedClock.h"
#include "services/log/LogService.h"
#include <serverEnumStrings.h>

#include "originwindow.h"

#include <QWebView>
#include <QWebPage>
#include <QNetworkReply>
#include <QNetworkAccessManager>

namespace Origin
{
namespace Client
{
PromoView::PromoView(QWidget* parent)
    : OriginWebView(parent)
    , mWebController(NULL)
{
    setHasSpinner(true);
    setShowSpinnerAfterInitLoad(false);
	setChangeLoadSizeAfterInitLoad(true);
    setIsContentSize(true);
    setScaleSizeOnContentsSizeChanged(true);

    mWebController = new OriginWebController(webview(), NULL, true);
    mWebController->errorHandler()->setTrackCurrentUrl(false);

    PromoJsHelper *promoHelper = new PromoJsHelper(this);
	mWebController->jsBridge()->addHelper("commonHelper", new CommonJsHelper(this));
    mWebController->jsBridge()->addHelper("promoManager", promoHelper);
    ORIGIN_VERIFY_CONNECT(promoHelper, &PromoJsHelper::closeWindow, this, &PromoView::closeWindow);

    // TODO: Remove this helper when the change avatar promo is gone.
    mWebController->jsBridge()->addHelper("newUserExperienceHelper", new SettingsJsHelper(this));

    ORIGIN_VERIFY_CONNECT(mWebController->errorDetector(), &PageErrorDetector::errorDetected, this, &PromoView::onErrorDetected);
    ORIGIN_VERIFY_CONNECT(webview()->page()->networkAccessManager(), &QNetworkAccessManager::finished, this, &PromoView::onRequestFinished);
    ORIGIN_VERIFY_CONNECT(webview(), &QWebView::loadFinished, this, &PromoView::onLoadFinished);
}

PromoView::~PromoView()
{
    if (mWebController)
    {
        QObject::disconnect(mWebController);
        mWebController->deleteLater();
    }
}

void PromoView::load(const PromoBrowserContext& context, const Engine::Content::EntitlementRef ent)
{
    ORIGIN_LOG_EVENT << "Showing Promo browser.";
    mCurrentPromoUrl = QUrl(Services::readSetting(Services::SETTING_PromoURL).toString());

    Services::Session::AccessToken token = Services::Session::SessionService::accessToken(Engine::LoginController::currentUser()->getSession());

    if (Services::readSetting(Services::SETTING_OverridePromos).toQVariant().toInt() == Services::PromosEnabled)
    {
        ORIGIN_LOG_EVENT << "Initial URL: " << mCurrentPromoUrl.toString();
        ORIGIN_LOG_EVENT << "context: " << context.promoType << " token: " << token.toLatin1();
    }

    QNetworkRequest req(mCurrentPromoUrl);

    req.setRawHeader("token", token.toLatin1());

    switch (context.promoType)
    {
        case PromoContext::OriginStarted:
        default:
	        req.setRawHeader("context", "login");
            break;
	
	    case PromoContext::MenuPromotions:
		    req.setRawHeader("context", "menu_promotions");
		    break;

        case PromoContext::GameFinished:
        case PromoContext::FreeTrialExited:
        case PromoContext::FreeTrialExpired:
            req.setRawHeader("context", "endgame");
            break;

        case PromoContext::DownloadUnderway:
            req.setRawHeader("context", "download");
            break;
    }

    QDateTime loginTime;
    QString lastLogin;
    Services::Session::SessionRef session = Services::Session::SessionService::instance()->currentSession();

    if(!session.isNull())
    {
        loginTime = Services::Session::SessionService::instance()->loginTime(session);
    }

    if (loginTime.isValid())
    {
        lastLogin = loginTime.toString("yyyy-MM-ddThh:mm:ssZ");
    }

    QString subsStatus = "UNKNOWN";
    const server::SubscriptionStatusT status = Origin::Engine::Subscription::SubscriptionManager::instance()->status();
    if(status >= 0 && status < server::SUBSCRIPTIONSTATUS_ITEM_COUNT)
    {
        subsStatus = server::SubscriptionStatusStrings[status];
    }
    req.setRawHeader("subscriptionStatus", subsStatus.toLatin1());
    req.setRawHeader("lastLogin", lastLogin.toLatin1());
    req.setRawHeader("originId", Engine::LoginController::currentUser()->eaid().toLatin1());

#if defined(ENABLE_SUBS_TRIALS)
    req.setRawHeader("subscriptionTrialEligible", (Engine::Subscription::SubscriptionManager::instance()->isTrialEligible() == 1 ? "true" : "false"));
#endif 

    // Has the user been subscribed for less than 30 days?
    req.setRawHeader("subscriptionNewUser", (Engine::Subscription::SubscriptionManager::instance()->firstSignupDate().daysTo(Services::TrustedClock::instance()->nowUTC()) < 30 ? "true" : "false"));

    QString locale = Services::readSetting(Services::SETTING_LOCALE).toString().left(2);
                
    QNetworkCookie c;
    if(Services::NetworkAccessManagerFactory::instance()->sharedCookieJar()->findFirstCookieByName("hl", c))
    {
        locale = c.value();
    }
    req.setRawHeader("suggestedLanguage", locale.toLatin1());

    // These fields are content-specific. Even if this is a promo
    // context which is supposed to provide this info, we can't
    // if we don't have an entitlement.
    if (ent)
    {
        bool trial = (context.promoType == PromoContext::FreeTrialExited || context.promoType == PromoContext::FreeTrialExpired);
        QString offerId = ent->contentConfiguration()->productId();

        if(ent->contentConfiguration()->isEntitledFromSubscription())
            offerId.append("_vault");
        else if(context.promoType == PromoContext::FreeTrialExited)
            offerId.append("_trialexited");
        else if(context.promoType == PromoContext::FreeTrialExpired)
            offerId.append("_trialexpired");

        req.setRawHeader("trial", (trial ? "true" : "false"));
        req.setRawHeader("offerId", offerId.toLatin1());
        req.setRawHeader("entitlementTag", ent->contentConfiguration()->entitlementTag().toLatin1());
        req.setRawHeader("groupName", ent->contentConfiguration()->groupName().toLatin1());
    }

    QString ownedOffers = Origin::Services::readSetting(Origin::Services::SETTING_PromoBrowserOfferCache, session).toString();
                
    // We want to make sure only ODT users can possibly override their owned offers - don't want to give any loopholes to gain
    // unwarranted access to promos.
    if(Services::readSetting(Services::SETTING_OriginDeveloperToolEnabled).toQVariant().toBool())
    {
        QString overrideOffers = Origin::Services::readSetting(Origin::Services::SETTING_OverridePromoBrowserOfferCache).toString();
        if(!overrideOffers.isEmpty())
        {
            ORIGIN_LOG_ACTION << "Overriding owned offers.";
            ownedOffers = overrideOffers;
        }
    }

    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QUrlQuery params;
    params.addQueryItem("offerIds", ownedOffers);
    QByteArray body = params.toString(QUrl::FullyEncoded).toLocal8Bit();

    mPromoUrlStatusCode = 0;
    mCurrentPromoUrl = req.url();

    mWebController->loadTrustedRequest(req, QNetworkAccessManager::PostOperation, body);

    // Properties that have to be set after the web page is set
    setHScrollPolicy(Qt::ScrollBarAlwaysOff);
    setVScrollPolicy(Qt::ScrollBarAlwaysOff);
}

void PromoView::onRequestFinished(QNetworkReply *reply)
{
    int attrib = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QUrl url = reply->url();

    if(url == mCurrentPromoUrl)
    {
        mPromoUrlStatusCode = attrib;
        ORIGIN_LOG_EVENT << "Promo service returned " << mPromoUrlStatusCode << ".";

        QString location = reply->header(QNetworkRequest::LocationHeader).toString();

        // 3XX codes signify a redirect, and we want mPromoUrlStatusCode to signify the final page's status.
        if(attrib >= 300 && attrib < 400 && !location.isEmpty())
        {
            mCurrentPromoUrl = location;
        }
    }
    
    if (Services::readSetting(Services::SETTING_OverridePromos).toQVariant().toInt() == Services::PromosEnabled)
    {
        PromoTraffic pt;
        pt.statusCode = attrib;
        pt.url = url.toString();
        mDebugTraffic.append(pt);
    }
}

void PromoView::onLoadFinished(bool successful)
{
    ORIGIN_VERIFY_DISCONNECT(mWebController->errorDetector(), &PageErrorDetector::errorDetected, this, &PromoView::onErrorDetected);
    ORIGIN_VERIFY_DISCONNECT(webview()->page()->networkAccessManager(), &QNetworkAccessManager::finished, this, &PromoView::onRequestFinished);
    ORIGIN_VERIFY_DISCONNECT(webview(), &QWebView::loadFinished, this, &PromoView::onLoadFinished);

    // Only write the traffic to the log if ForcePromos=1 in eacore.ini so we don't spam everybody's logs all the time.
    if (Services::readSetting(Services::SETTING_OverridePromos).toQVariant().toInt() == Services::PromosEnabled)
    {
        ORIGIN_LOG_EVENT << "Promo traffic: " << mDebugTraffic.size();
        for (auto it = mDebugTraffic.constBegin(); it != mDebugTraffic.constEnd(); ++it)
        {
            PromoTraffic pt = *it;
            ORIGIN_LOG_EVENT << "Response: " << pt.statusCode << " URL: " << pt.url;
        }
        mDebugTraffic.clear();
    }

    ORIGIN_LOG_EVENT << "Page loaded: " << successful << "(status=" << mPromoUrlStatusCode << ")";

    emit loadFinished(mPromoUrlStatusCode, successful);
}

void PromoView::onErrorDetected(Origin::Client::PageErrorDetector::ErrorType errorType)
{
    if (errorType != PageErrorDetector::NoError)
    {
        // Report an unsuccessful load
        onLoadFinished(false);
    }
}

}

}
