///////////////////////////////////////////////////////////////////////////////
// SubscriptionServiceClient.cpp
//
// Author: Hans van Veenendaal
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QNetworkRequest>
#include <QLocale>

#include "services/subscription/SubscriptionServiceClient.h"
#include "services/subscription/SubscriptionServiceResponse.h"
#include "services/settings/SettingsManager.h"
#include "services/platform/PlatformService.h"
#include "services/session/SessionService.h"

namespace Origin
{
    namespace Services
    {
        namespace Subscription
        {
            SubscriptionServiceClient::SubscriptionServiceClient(const QUrl &baseUrl, NetworkAccessManager *nam)
                : OriginServiceClient(nam)
            {
                if (!baseUrl.isEmpty())
                    setBaseUrl(baseUrl);
                else
                {
                    QString strUrl = Origin::Services::readSetting(Origin::Services::SETTING_SubscriptionServiceURL); 
                    setBaseUrl(QUrl(strUrl));
                }
            }


            SubscriptionInfoResponse * SubscriptionServiceClient::subscriptionInfoPriv(const QString &group, const QString &state)
            {
                QString myUrl("/pids/%1/subscriptionsv2/groups/%2");

                Origin::Services::Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();

                myUrl = myUrl.arg(Origin::Services::Session::SessionService::nucleusUser(session)).arg(group);

                QUrl serviceUrl(urlForServicePath(myUrl));
                QUrlQuery serviceUrlQuery(serviceUrl);

                serviceUrlQuery.addQueryItem("state", state);

                serviceUrl.setQuery(serviceUrlQuery);
                QNetworkRequest request(serviceUrl);

                request.setRawHeader("Authorization", QByteArray("Bearer ") + Origin::Services::Session::SessionService::accessToken(Origin::Services::Session::SessionService::currentSession()).toLatin1());

                return new SubscriptionInfoResponse(getNonAuth(request));
            }	

            SubscriptionDetailInfoResponse * SubscriptionServiceClient::subscriptionDetailInfoPriv(const QString &uri)
            {
                QString myUrl("/pids/%1%2");
                
                Origin::Services::Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();

                myUrl = myUrl.arg(Origin::Services::Session::SessionService::nucleusUser(session)).arg(uri);

                QUrl serviceUrl(urlForServicePath(myUrl));

                QNetworkRequest request(serviceUrl);

                request.setRawHeader("Authorization", QByteArray("Bearer ") + Origin::Services::Session::SessionService::accessToken(Origin::Services::Session::SessionService::currentSession()).toLatin1());

                return new SubscriptionDetailInfoResponse(getNonAuth(request));
            }	
        }
    }
}
