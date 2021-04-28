///////////////////////////////////////////////////////////////////////////////
// SubscriptionVaultServiceClient.cpp
//
// Author: Hans van Veenendaal
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QNetworkRequest>
#include <QLocale>

#include "services/subscription/SubscriptionVaultServiceClient.h"
#include "services/subscription/SubscriptionVaultServiceResponse.h"
#include "services/settings/SettingsManager.h"
#include "services/platform/PlatformService.h"
#include "services/session/SessionService.h"

namespace Origin
{
    namespace Services
    {
        namespace Subscription
        {
            SubscriptionVaultServiceClient::SubscriptionVaultServiceClient(const QUrl &baseUrl, NetworkAccessManager *nam)
                : OriginServiceClient(nam)
            {
                if (!baseUrl.isEmpty())
                    setBaseUrl(baseUrl);
                else
                {
                    QString strUrl = Origin::Services::readSetting(Origin::Services::SETTING_SubscriptionVaultServiceURL); 
                    setBaseUrl(QUrl(strUrl));
                }
            }


            SubscriptionVaultInfoResponse * SubscriptionVaultServiceClient::subscriptionVaultInfoPriv(const QString &group)
            {
                QString myUrl("/%1");

                Origin::Services::Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();

                myUrl = myUrl.arg(group);

                QUrl serviceUrl(urlForServicePath(myUrl));
                QUrlQuery serviceUrlQuery(serviceUrl);

                serviceUrl.setQuery(serviceUrlQuery);
                QNetworkRequest request(serviceUrl);

                request.setRawHeader("AuthToken", Origin::Services::Session::SessionService::accessToken(Origin::Services::Session::SessionService::currentSession()).toLatin1());

                return new SubscriptionVaultInfoResponse(getNonAuth(request));
            }	
        }
    }
}
