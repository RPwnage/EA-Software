///////////////////////////////////////////////////////////////////////////////
// ChatServiceClient.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QNetworkRequest>

#include "services/rest/ChatServiceClient.h"
#include "services/settings/SettingsManager.h"

namespace Origin
{
    namespace Services
    {
        ChatServiceClient::ChatServiceClient(const QUrl &baseUrl, NetworkAccessManager *nam)
            : OriginAuthServiceClient(nam)
        {
            if (!baseUrl.isEmpty())
                setBaseUrl(baseUrl);
            else
            {
                QString strUrl = Origin::Services::readSetting(Origin::Services::SETTING_ChatURL);
                setBaseUrl(QUrl(strUrl + "/chat"));
            }
        }

        UserOnlineResponse* ChatServiceClient::isUserOnlinePriv(Session::SessionRef session)
        {
            QUrl serviceUrl(urlForServicePath("user/" + nucleusId(session) + "/online"));
            QUrlQuery serviceUrlQuery(serviceUrl);
            serviceUrlQuery.addQueryItem("resource", "origin");
            serviceUrl.setQuery(serviceUrlQuery);
            
            QNetworkRequest request(serviceUrl);
            request.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            // We are using the offline auth here because we might be in a manual offline state. We are unable to go
            // online because that would interrupt another session if there is one active.
            return new UserOnlineResponse(getAuthOffline(session, request));
        }	
    }
}
