///////////////////////////////////////////////////////////////////////////////
// OnTheHousePromoService.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _ONTHEHOUSEPROMO_H_INCLUDED_
#define _ONTHEHOUSEPROMO_H_INCLUDED_

#include "OriginAuthServiceClient.h"
#include "OriginAuthServiceResponse.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {

		class ORIGIN_PLUGIN_API OnTheHousePromoServiceResponse :	public OriginAuthServiceResponse
		{
            Q_OBJECT
        public:
            explicit OnTheHousePromoServiceResponse(AuthNetworkRequest reply, const QString &element);

            const QString &offer() const { return mHtml; }
            const QString &offerVersion() const { return mOfferVersion; }
        signals:
            void successful();

		private slots:
			void onFinished();
		protected:
            ///
            ///  On the House Promotional offer display data container
            ///
			QString mHtml;
			QString mOfferVersion;
			QNetworkReply* mNetworkReply;
        };

        class ORIGIN_PLUGIN_API OnTheHousePromoService :	public OriginAuthServiceClient
        {
        public:
            friend class OriginClientFactory<OnTheHousePromoService>;

            ///
            /// \brief Get response for displaying On the House Promotional offer
            ///
            static OnTheHousePromoServiceResponse* fetchPromo(Session::SessionRef session, const bool& isUnderage)
            {
                return OriginClientFactory<OnTheHousePromoService>::instance()->fetchPromoPriv(session, isUnderage);
            }

        private:
            /// 
            /// \brief Creates a new SSOTicket service client
            /// \param baseUrl base url for the service (QUrl)
            /// \param nam QNetworkAccessManager instance to send requests through
            ///
            explicit OnTheHousePromoService(const QUrl &baseUrl = QUrl(), NetworkAccessManager *nam = NULL);

            ///
            /// \brief response for displaying  On the House Promotional offer
            ///
            OnTheHousePromoServiceResponse* fetchPromoPriv(Session::SessionRef session, const bool& isUnderage);

            /// \brief to store base url
            QString mUrl;
		};
	}
}
#endif