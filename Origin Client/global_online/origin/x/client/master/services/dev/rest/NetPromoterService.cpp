///////////////////////////////////////////////////////////////////////////////
// NetPromoterService.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "services/rest/NetPromoterService.h"
#include "services/settings/SettingsManager.h"
#include "services/log/LogService.h"

namespace Origin
{
    namespace Services
    {
        NetPromoterService::NetPromoterService(const QUrl &baseUrl, NetworkAccessManager *nam)
            : OriginAuthServiceClient(nam) 
            , mUrl(baseUrl.toString())
        {
        }

        NetPromoterServiceResponse* NetPromoterService::displayCheckPriv(Session::SessionRef session, const NetPromoterType type, const QString arg2, int over_ride)
        {
            QString serviceUrlStr = "";
            switch(type)
            {
            case Origin_General:
                if (mUrl.isEmpty())
                {
                    mUrl = Origin::Services::readSetting(Origin::Services::SETTING_NetPromoterURL).toString();
                }
                serviceUrlStr = mUrl + QString("/%1").arg(nucleusId(session));
                break;
            case Game_PlayEnd:
                if (mUrl.isEmpty())
                {
                    mUrl = Origin::Services::readSetting(Origin::Services::SETTING_NetPromoterURL).toString();
                }
                serviceUrlStr = mUrl + QString("/%1/offer/%2").arg(nucleusId(session)).arg(arg2);
                break;
            default:
                break;
            }

            QUrl serviceUrl(serviceUrlStr);
            QUrlQuery serviceUrlQuery(serviceUrl);
            if (over_ride != -1)
            {
                serviceUrlQuery.addQueryItem("offset", QString::number(over_ride));
            }
            else
                serviceUrlQuery.addQueryItem("offset", QString("0"));

            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest request(serviceUrl);

            request.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new NetPromoterServiceResponse(getAuth(session, request), "" );
        }

        NetPromoterServiceResponse::NetPromoterServiceResponse(AuthNetworkRequest reply, const QString &element) : 
        OriginAuthServiceResponse(reply),
            mResponse(false)
        {
        }

        void NetPromoterServiceResponse::processReply()
        {
            // Default in case we don't get a response back
            mResponse = false;

            QNetworkReply* networkReply = reply();
            Origin::Services::HttpStatusCode status = (Origin::Services::HttpStatusCode) networkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

            ORIGIN_LOG_EVENT << "NetPromoterServiceResponse " << status;

            if (status == Http200Ok)
            {
                mResponse = true;
                emit(success());
            }
            else
                emit(error(status));

        }
    }
}