///////////////////////////////////////////////////////////////////////////////
// SubscriptionTrialEligibilityServiceClient.cpp
//
// Copyright (c) 2015 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "services/subscription/SubscriptionTrialEligibilityServiceClient.h"
#include "services/subscription/SubscriptionTrialEligibilityServiceResponse.h"
#include "services/session/SessionService.h"
#include "services/settings/SettingsManager.h"
#include <QNetworkRequest>
#include <QLocale>

namespace Origin
{
namespace Services
{
namespace Subscription
{

SubscriptionTrialEligibilityServiceClient::SubscriptionTrialEligibilityServiceClient(const QUrl& baseUrl, NetworkAccessManager* nam)
: OriginServiceClient(nam)
{
    if (!baseUrl.isEmpty())
        setBaseUrl(baseUrl);
    else
    {
        const QString locale = QLocale().name().toLower().replace("_", "-");
        const QString strUrl = Services::readSetting(Services::SETTING_SubscriptionTrialEligibilityServiceURL).arg(locale);
        setBaseUrl(QUrl(strUrl));
    }
}


SubscriptionTrialEligibilityServiceResponse* SubscriptionTrialEligibilityServiceClient::subscriptionTrialEligibilityInfoPriv()
{
    const QString myUrl = baseUrl().toString() + QString("/%1").arg(Services::Session::SessionService::nucleusUser(Services::Session::SessionService::currentSession()));
    const QUrl serviceUrl(myUrl);
    QNetworkRequest request(serviceUrl);
    request.setRawHeader("skip-client-check", "true");

    return new SubscriptionTrialEligibilityServiceResponse(getNonAuth(request));
}

}
}
}