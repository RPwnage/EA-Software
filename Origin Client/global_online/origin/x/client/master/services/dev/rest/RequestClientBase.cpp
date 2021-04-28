///////////////////////////////////////////////////////////////////////////////
// RequestClientBase.cpp
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "services/log/LogService.h"
#include "services/rest/RequestClientBase.h"
#include "services/settings/SettingsManager.h"

namespace Origin
{
    namespace Services
    {
        RequestClientBase::RequestClientBase(NetworkAccessManager *nam /*= NULL*/ )
            : OriginAuthServiceClient(nam)
        {
        }

        QNetworkRequest RequestClientBase::buildRequest(Session::SessionRef session, const QString& servicePath, const QMap<QString, QString>& queryItems)
        {
            QUrl serviceUrl(urlForServicePath(servicePath));
            QUrlQuery serviceUrlQuery(serviceUrl);
            
            // if there are any query items, add them to the request
            QMap<QString, QString>::const_iterator query = queryItems.constBegin();
            while (query != queryItems.constEnd()) {
                serviceUrlQuery.addQueryItem(query.key(), query.value());
                ++query;
            }

            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest request(serviceUrl);
            request.setRawHeader("AuthToken", Session::SessionService::accessToken(session).toUtf8());
            request.setRawHeader("X-AccessToken", Session::SessionService::accessToken(session).toUtf8());
            request.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");

#ifdef _DEBUG
            ORIGIN_LOG_EVENT << "GET Request: " << request.url().toString();
            foreach(QByteArray hdr, request.rawHeaderList())
            {
                ORIGIN_LOG_EVENT << hdr << " : " << request.rawHeader(hdr);
            }
#endif // _DEBUG

            return request;
        }

        RequestClientBase::~RequestClientBase()
        {

        }

        //////////////////////////////////////////////////////////////////////////
        // Privacy
    }
}
