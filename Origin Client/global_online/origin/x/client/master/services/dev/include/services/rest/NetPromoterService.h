///////////////////////////////////////////////////////////////////////////////
// NetPromoterService.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _NETPROMOTER_H_INCLUDED_
#define _NETPROMOTER_H_INCLUDED_

#include "OriginAuthServiceClient.h"
#include "OriginAuthServiceResponse.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {

        class ORIGIN_PLUGIN_API NetPromoterServiceResponse : public OriginAuthServiceResponse
        {
            Q_OBJECT
        public:
            explicit NetPromoterServiceResponse(AuthNetworkRequest, const QString &element);

            /// 
            /// Returns the SSOTicket info data
            /// 
            const bool response() const { return mResponse;}


        protected:

            virtual void processReply();
            ///
            /// NetPromoter display data container
            ///
            bool mResponse;
        };

        class ORIGIN_PLUGIN_API NetPromoterService :	public OriginAuthServiceClient
        {
        public:
            enum NetPromoterType
            {
                Origin_General,
                Game_PlayEnd
            };


            friend class OriginClientFactory<NetPromoterService>;

            ///
            /// \brief Get response for displaying Net Promoter Score survey
            ///
            static NetPromoterServiceResponse* displayCheck(Session::SessionRef session, const NetPromoterType type = Origin_General, const QString arg2 = "", int over_ride = -1)
            {
                return OriginClientFactory<NetPromoterService>::instance()->displayCheckPriv(session, type, arg2, over_ride);
            }

        private:
            /// 
            /// \brief Creates a new SSOTicket service client
            /// \param baseUrl base url for the service (QUrl)
            /// \param nam           QNetworkAccessManager instance to send requests through
            ///
            explicit NetPromoterService(const QUrl &baseUrl = QUrl(), NetworkAccessManager *nam = NULL);

            ///
            /// \brief response for displaying Net Promoter Score survey
            ///
            NetPromoterServiceResponse* displayCheckPriv(Session::SessionRef session, const NetPromoterType type, const QString arg2, int over_ride);

            /// \brief to store base url
            QString mUrl;
        };

    }
}


#endif

