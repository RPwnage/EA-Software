///////////////////////////////////////////////////////////////////////////////
// StoreUrlBuilder.cpp
//
// Copyright (c) 2012-2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "StoreUrlBuilder.h"

#include "engine/subscription/SubscriptionManager.h"
#include "engine/login/LoginController.h"
#include "services/settings/SettingsManager.h"
#include "services/config/OriginConfigService.h"
#include "services/session/SessionService.h"

namespace Origin
{
namespace Client
{

QUrl StoreUrlBuilder::productUrl(QString productId, QString masterTitleId) const
{
    if(!masterTitleId.isEmpty())
    {
        return masterTitleUrl(masterTitleId);
    }
    else
    {
        // Remove any percent encoding that was previously applied to the product ID.
        productId = QUrl::fromPercentEncoding(productId.toUtf8());

        QString productUrl = Services::readSetting(Services::SETTING_StoreProductURL).toString();
        productUrl.replace("{offerId}", QUrl::toPercentEncoding(productId));
	    return QUrl::fromEncoded(productUrl.toUtf8());
    }
}

QUrl StoreUrlBuilder::masterTitleUrl(QString masterTitleId) const
{
    // Remove any percent encoding that was previously applied to the master title ID.
    masterTitleId = QUrl::fromPercentEncoding(masterTitleId.toUtf8());

    QString masterTitleUrl = Services::readSetting(Services::SETTING_StoreMasterTitleURL).toString();
    masterTitleUrl.replace("{masterTitleId}", QUrl::toPercentEncoding(masterTitleId));
	return QUrl::fromEncoded(masterTitleUrl.toUtf8());
}

QUrl StoreUrlBuilder::homeUrl() const
{
    QString homeUrl = Services::readSetting(Services::SETTING_StoreInitialURL).toString();
    QString subsHomeUrl = Services::readSetting(Services::SETTING_SubscriptionInitialURL).toString();

    // If a subscription home URL is configured and the user has a subscription, use that URL instead.
    if (!subsHomeUrl.isEmpty() && Engine::Subscription::SubscriptionManager::instance()->isActive())
    {
        homeUrl = subsHomeUrl;
    }

	return QUrl::fromEncoded(homeUrl.toUtf8());
}

QUrl StoreUrlBuilder::freeGamesUrl() const
{
    QString freeGamesUrl = Services::readSetting(Services::SETTING_FreeGamesURL).toString();
    return QUrl::fromEncoded(freeGamesUrl.toUtf8());
}

QUrl StoreUrlBuilder::onTheHouseStoreUrl(const QString & trackingParam) const
{
    QString onTheHouseStoreUrl = Services::readSetting(Services::SETTING_OnTheHouseStoreURL).toString();
    onTheHouseStoreUrl.replace("{trackingParam}", QUrl::toPercentEncoding(trackingParam));
    return QUrl::fromEncoded(onTheHouseStoreUrl.toUtf8());
}

QUrl StoreUrlBuilder::emptyShelfUrl() const
{
    QString emptyShelfUrl = Services::readSetting(Services::SETTING_EmptyShelfURL).toString();
    return QUrl::fromEncoded(emptyShelfUrl.toUtf8());
}

QUrl StoreUrlBuilder::subscriptionUrl() const
{
    QString url = Services::readSetting(Services::SETTING_SubscriptionStoreURL).toString();
    return QUrl::fromEncoded(url.toUtf8());
}

QUrl StoreUrlBuilder::subscriptionVaultUrl() const
{
    QString url = Services::readSetting(Services::SETTING_SubscriptionVaultURL).toString();
    return QUrl::fromEncoded(url.toUtf8());
}

QUrl StoreUrlBuilder::entitleFreeGamesUrl(QString productId) const
{
    // Remove any percent encoding that was previously applied to the product ID.
    productId = QUrl::fromPercentEncoding(productId.toUtf8());

    QString entitleFreeGameUrl = Services::readSetting(Services::SETTING_StoreEntitleFreeGameURL).toString();
    entitleFreeGameUrl.replace("{offerId}", QUrl::toPercentEncoding(productId));
    entitleFreeGameUrl.replace("{userId}", QString::number(Engine::LoginController::instance()->currentUser()->userId()));
    entitleFreeGameUrl.replace("{authToken}", Services::Session::SessionService::accessToken(Services::Session::SessionService::currentSession()));
    return QUrl::fromEncoded(entitleFreeGameUrl.toUtf8());
}

QString StoreUrlBuilder::storeEnvironment() const
{
    QString storeEnv;
    const QString storeHomeUrl = Services::readSetting(Services::SETTING_StoreInitialURL).toString();

    // It's valid for us to be pointing to a QA environment on FCQA so we can't split it out in client environments.
    // -- FC.QA --
    if(storeHomeUrl.contains("fc.qa.preview"))
    {
        storeEnv = "ebisu-fcqa-preview";
    }
    else if(storeHomeUrl.contains("fc.qa.review"))
    {
        storeEnv = "ebisu-fcqa-review";
    }
    else if(storeHomeUrl.contains("fc.qa.approve"))
    {
        storeEnv = "ebisu-fcqa-approve";
    }
    else if(storeHomeUrl.contains("fc.qa"))
    {
        storeEnv = "ebisu-fcqa-live";
    }
    // -- QA --
    else if(storeHomeUrl.contains("qa.preview"))
    {
        storeEnv = "ebisu-qa-preview";
    }
    else if(storeHomeUrl.contains("qa.review"))
    {
        storeEnv = "ebisu-qa-review";
    }
    else if(storeHomeUrl.contains("qa.approve"))
    {
        storeEnv = "ebisu-qa-approve";
    }
    else if(storeHomeUrl.contains("qa"))
    {
        storeEnv = "ebisu-qa";
    }
    // -- Integration --
    else if(storeHomeUrl.contains("dev.preview"))
    {
        storeEnv = "ebisu-dev-preview";
    }
    else if(storeHomeUrl.contains("dev.review"))
    {
        storeEnv = "ebisu-dev-review";
    }
    else if(storeHomeUrl.contains("dev.approve"))
    {
        storeEnv = "ebisu-dev-approve";
    }
    else if(storeHomeUrl.contains("dev"))
    {
        storeEnv = "ebisu-dev-live";
    }
    // -- Production --
    else if(storeHomeUrl.contains("preview"))
    {
        storeEnv = "ebisu-preview";
    }
    else if(storeHomeUrl.contains("review"))
    {
        storeEnv = "ebisu-review";
    }
    else if(storeHomeUrl.contains("approve"))
    {
        storeEnv = "ebisu-approve";
    }
    else
    {
        storeEnv = "ebisu-prod";
    }

    if(storeHomeUrl.contains("thor"))
    {
        storeEnv.append("-thor");
    }

    return storeEnv;
}

}
}
