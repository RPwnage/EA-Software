///////////////////////////////////////////////////////////////////////////////
// SearchFriendServiceClient.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QNetworkRequest>

#include "services/rest/SearchFriendServiceClient.h"
#include "services/rest/SearchFriendServiceResponses.h"
#include "services/settings/SettingsManager.h"
#include "services/log/LogService.h"
#include <QNetworkCookie>
#include "services/session/SessionService.h"


namespace Origin
{
    namespace Services
    {

        SearchFriendServiceClient::SearchFriendServiceClient(const QUrl &baseUrl, NetworkAccessManager *nam)
            : OriginAuthServiceClient(nam)
        {
            if (!baseUrl.isEmpty())
                setBaseUrl(baseUrl);
            else
            {
                QString strUrl = Origin::Services::readSetting(Origin::Services::SETTING_FriendSearchURL); 
                QUrl myUrl(strUrl);
                myUrl.setPath("/web");
                setBaseUrl(myUrl);
            }
        }

         SearchFriendServiceResponse* SearchFriendServiceClient::searchFriendPriv(const QByteArray& at, const QString &searchKeyword, const QString& pageNumber)
         {
             QUrl serviceUrl(urlForServicePath("searchInXml"));
             
             QUrlQuery queryUrl(serviceUrl);
             queryUrl.addQueryItem("searchkeyword", searchKeyword);
             queryUrl.addQueryItem("currentPageNo", pageNumber);

             serviceUrl.setQuery(queryUrl);
             QNetworkRequest request(serviceUrl);
             QVariant myVar;
             auto cookies = getMeMyCookies(at);
             setNetworkAccessManagerCookieJar(cookies, serviceUrl);
             debugRequest(request);
             return new SearchFriendServiceResponse(getAuth(Session::SessionService::currentSession(), request));
         }

    }
}

