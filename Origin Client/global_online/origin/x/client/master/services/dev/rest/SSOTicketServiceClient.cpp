///////////////////////////////////////////////////////////////////////////////
// SSOTicketServiceClient.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "services/rest/SSOTicketServiceClient.h"
#include "services/settings/SettingsManager.h"

namespace Origin
{
    namespace Services
    {
        SSOTicketServiceClient::SSOTicketServiceClient(const QUrl &baseUrl,NetworkAccessManager *nam)
            :OriginAuthServiceClient(nam) 
        {
            if (baseUrl.isEmpty())
            {
                mUrl = Origin::Services::readSetting(Origin::Services::SETTING_LoginRegistrationURL).toString();
            }
            else
            {
                mUrl = baseUrl.toString();
            }
        }

        SSOTicketServiceResponse* SSOTicketServiceClient::ssoTicketPriv(Session::SessionRef session)
        {
            mUrl = mUrl.arg(nucleusId(session));
            QNetworkRequest request;
            
            QString serviceUrl = QString(mUrl + "/loginregistration/user/%1/originSSOToken").arg(nucleusId(session));

            request.setUrl(serviceUrl);
            request.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new SSOTicketServiceResponse(getAuth(session, request), "originSSOToken");
        }
        
        SSOTicketServiceResponse* SSOTicketServiceClient::exchangeTicketPriv(Session::SessionRef session)
        {
            mUrl = mUrl.arg(nucleusId(session));
            QNetworkRequest request;

            QString serviceUrl = QString(mUrl + "/loginregistration/user/%1/originSSOExchangeTicket").arg(nucleusId(session));
            request.setUrl(serviceUrl);
            request.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new SSOTicketServiceResponse(getAuth(session, request), "exchangeTicket");
        }
    }
}


