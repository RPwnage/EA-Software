///////////////////////////////////////////////////////////////////////////////
// TrialServiceClient.cpp
//
// Author: Hans van Veenendaal
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QNetworkRequest>
#include <QLocale>

#include "services/trials/TrialServiceClient.h"
#include "services/trials/TrialServiceResponse.h"
#include "services/settings/SettingsManager.h"
#include "services/platform/PlatformService.h"

namespace Origin
{
    namespace Services
    {
        namespace Trials
        {
            TrialServiceClient::TrialServiceClient(const QUrl &baseUrl, NetworkAccessManager *nam) : 
                OriginAuthServiceClient(nam)
            {
                if (!baseUrl.isEmpty())
                    setBaseUrl(baseUrl);
                else
                {
                    QString strUrl = Origin::Services::readSetting(Origin::Services::SETTING_TimedTrialServiceURL);
                    setBaseUrl(QUrl(strUrl));
                }
            }

            TrialBurnTimeResponse * TrialServiceClient::burnTimePriv(const QString &contentId, const QString &requestToken)
            {
                Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();

                const QString myUrl("/burntime");
                QUrl serviceUrl(urlForServicePath(myUrl));
                QUrlQuery serviceUrlQuery(serviceUrl);

                serviceUrlQuery.addQueryItem("userId", nucleusId(session));
                serviceUrlQuery.addQueryItem("contentId", contentId);

                if(!requestToken.isEmpty())
                {
                    serviceUrlQuery.addQueryItem("requestToken", requestToken);
                }

                serviceUrl.setQuery(serviceUrlQuery);
                QNetworkRequest request(serviceUrl);

                request.setRawHeader("Authorization", QByteArray("Bearer ") + Origin::Services::Session::SessionService::accessToken(session).toLatin1());
                request.setRawHeader("Accept", "application/json");

                return new TrialBurnTimeResponse(putAuth(session, request, QByteArray()));
            }	

            TrialCheckTimeResponse * TrialServiceClient::checkTimePriv(const QString &contentId)
            {
                Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();

                const QString myUrl("/checktime");
                QUrl serviceUrl(urlForServicePath(myUrl));
                QUrlQuery serviceUrlQuery(serviceUrl);

                serviceUrlQuery.addQueryItem("userId", nucleusId(session));
                serviceUrlQuery.addQueryItem("contentId", contentId);

                serviceUrl.setQuery(serviceUrlQuery);
                QNetworkRequest request(serviceUrl);

                request.setRawHeader("Authorization", QByteArray("Bearer ") + Origin::Services::Session::SessionService::accessToken(session).toLatin1());
                request.setRawHeader("Accept", "application/json");

                return new TrialCheckTimeResponse(getAuth(session, request));
            }

            TrialGrantTimeResponse * TrialServiceClient::grantTimePriv(const QString& contentId)
            {
                Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();
                const QString myUrl("http://access.integration.ea.com/1/granttime");
                QUrl serviceUrl(myUrl);
                QUrlQuery serviceUrlQuery(serviceUrl);

                serviceUrlQuery.addQueryItem("userId", nucleusId(session));
                serviceUrlQuery.addQueryItem("contentId", contentId);
                serviceUrlQuery.addQueryItem("grantedSec", "600");

                serviceUrl.setQuery(serviceUrlQuery);
                QNetworkRequest request(serviceUrl);

                request.setRawHeader("Authorization", QByteArray("Bearer ") + Origin::Services::Session::SessionService::accessToken(session).toLatin1());
                request.setRawHeader("Accept", "application/json");
                request.setRawHeader("X-Requester-Id", "dev");
                request.setRawHeader("X-Forwarded-For", "1.2.3.4");

                return new TrialGrantTimeResponse(putAuth(session, request, QByteArray()));
            }
        }
    }
}
