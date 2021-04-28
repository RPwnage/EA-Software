///////////////////////////////////////////////////////////////////////////////
// SSOTicketServiceClient.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _SSOTICKETSERVICECLIENT_H_INCLUDED_
#define _SSOTICKETSERVICECLIENT_H_INCLUDED_

#include "OriginAuthServiceClient.h"
#include "SSOTicketServiceResponse.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        class ORIGIN_PLUGIN_API SSOTicketServiceClient :	public OriginAuthServiceClient
        {
        public:
            friend class OriginClientFactory<SSOTicketServiceClient>;

            ///
            /// \brief Get SSOTicket for user
            ///
            static SSOTicketServiceResponse* ssoTicket(Session::SessionRef session)
            {
                return OriginClientFactory<SSOTicketServiceClient>::instance()->ssoTicketPriv(session);
            }

            ///
            /// \brief Get exchange ticket for user
            ///
            static SSOTicketServiceResponse* exchangeTicket(Session::SessionRef session)
            {
                return OriginClientFactory<SSOTicketServiceClient>::instance()->exchangeTicketPriv(session);
            }

        private:
            /// 
            /// \brief Creates a new SSOTicket service client
            /// \param baseUrl base url for the service (QUrl)
            /// \param nam           QNetworkAccessManager instance to send requests through
            ///
            explicit SSOTicketServiceClient(const QUrl &baseUrl = QUrl(), NetworkAccessManager *nam = NULL);

            ///
            /// \brief Get SSOTicket for user
            ///
            SSOTicketServiceResponse* ssoTicketPriv(Session::SessionRef session);
            
            ///
            /// \brief Get exchange ticket for user
            ///
            SSOTicketServiceResponse* exchangeTicketPriv(Session::SessionRef session);

            /// \brief to store base url
            QString mUrl;
        };
    }
}


#endif

