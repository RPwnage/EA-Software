///////////////////////////////////////////////////////////////////////////////
// OnTheHousePromoService.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "qdom.h"

#include "services/rest/OnTheHousePromoService.h"
#include "services/settings/SettingsManager.h"
#include "services/log/LogService.h"
#include "services/debug/DebugService.h"
#include "services/session/SessionService.h"


namespace Origin
{
    namespace Services
    {
        OnTheHousePromoService::OnTheHousePromoService(const QUrl &baseUrl, NetworkAccessManager *nam)
            : OriginAuthServiceClient(nam) 
        {
            if (baseUrl.isEmpty())
            {
				mUrl = Origin::Services::readSetting(Origin::Services::SETTING_OnTheHousePromoURL).toString();
            }
            else
            {
                mUrl = baseUrl.toString();
            }
        }
        
        OnTheHousePromoServiceResponse* OnTheHousePromoService::fetchPromoPriv(Session::SessionRef session, const bool& isUnderage)
        {
			QString overrideURL = Services::readSetting(Services::SETTING_OnTheHouseOverridePromoURL).toString();
			if (overrideURL.isEmpty())
			{
	            mUrl = Services::readSetting(Services::SETTING_OnTheHousePromoURL).toString();
			}
			else
			{
				mUrl = overrideURL;
			}

            Services::Session::AccessToken token = Services::Session::SessionService::accessToken(session);

            QNetworkRequest req(mUrl);

            req.setRawHeader("token", token.toLatin1());
			req.setRawHeader("context", "onthehouse");
            req.setRawHeader("underage", isUnderage ? "true" : "false");
            req.setRawHeader("clientVersion", QCoreApplication::applicationVersion().toLatin1());

            QString locale = Services::readSetting(Services::SETTING_LOCALE).toString().left(2);
                
            req.setRawHeader("suggestedLanguage", locale.toLatin1());
                
            req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

            QString ownedOffers = Origin::Services::readSetting(Origin::Services::SETTING_PromoBrowserOfferCache, session).toString();
            const QString othVersionsShown = Services::readSetting(Services::SETTING_ONTHEHOUSE_VERSIONS_SHOWN, session).toString().replace("#//", ",");
                
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

            QUrlQuery params;
            params.addQueryItem("offerIds", ownedOffers);
            params.addQueryItem("exclude", othVersionsShown);
            QByteArray networkRequestBody = params.toString(QUrl::FullyEncoded).toLocal8Bit();

            // New promo manager is always POST
			AuthNetworkRequest reply = postAuth(session, req, networkRequestBody);
			OnTheHousePromoServiceResponse* response = new OnTheHousePromoServiceResponse(reply, "" );
			ORIGIN_VERIFY_CONNECT(reply.first, SIGNAL(finished()), response, SLOT(onFinished()));
            return response;				
				
        }

        OnTheHousePromoServiceResponse::OnTheHousePromoServiceResponse(AuthNetworkRequest reply, const QString &element) : 
            OriginAuthServiceResponse(reply),
            mHtml(""),
			mOfferVersion(""),
			mNetworkReply(reply.first)
        {
        }

		void OnTheHousePromoServiceResponse::onFinished()
		{
			Origin::Services::HttpStatusCode status = (Origin::Services::HttpStatusCode) mNetworkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
			
            ORIGIN_LOG_EVENT << "OnTheHousePromoServiceResponse " << status;

            if (status == Http200Ok)
            {

				mHtml = mNetworkReply->readAll();
				mOfferVersion = QString(mNetworkReply->rawHeader("X-Origin-Promo-Name"));
				emit(successful());
            }
            else
                emit(error(status));

		}
    }
}