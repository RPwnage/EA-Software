///////////////////////////////////////////////////////////////////////////////
// StoreServiceClient.h
//
// Author: Jonathan Kolb
// Copyright (c) 2014 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _STORESERVICECLIENT_H
#define _STORESERVICECLIENT_H

#include "services/rest/OriginServiceClient.h"
#include "services/rest/StoreServiceResponses.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{

    namespace Services
    {

///
/// \brief HTTP service client for the communication with the Origin store.
///
        class ORIGIN_PLUGIN_API StoreServiceClient : public OriginServiceClient
        {
        public:
            friend class OriginClientFactory<StoreServiceClient>;

            static StoreServiceClient *instance()
            {
                return OriginClientFactory<StoreServiceClient>::instance();
            }

            /// \brief Redeems a subscription offer for the user
            /// \param subscriptionId The subscription id you want the redemption for.
            /// \param offerId The offer to redeem.
            /// \param transactionId Some string to identify the transaction when the response comes back.
            RedeemSubscriptionOfferResponse * redeemSubscriptionOffer(const QString &subscriptionId, const QString &offerId, const QString &transactionId);

            /// \brief Revoke a subscription offer for the user
            /// \param subscriptionId The subscription id you want the redemption for.
            /// \param offerId The offer to revoke.
            /// \param transactionId Some string to identify the transaction when the response comes back.
            RedeemSubscriptionOfferResponse * revokeSubscriptionOffer(const QString &subscriptionId, const QString &offerId, const QString &transactionId);

            /// \brief Redirect the redemption
            RedeemSubscriptionOfferResponse * redeemSubscriptionOfferByUrl(Services::RedeemSubscriptionOfferResponse * response);

            /// \brief Redirect the redemption
            RedeemSubscriptionOfferResponse * revokeSubscriptionOfferByUrl(Services::RedeemSubscriptionOfferResponse * response);
        private:

            ///
            /// \brief Creates a new store service client
            ///
            /// \param baseUrl       Base URL for the store service API.
            /// \param nam           QNetworkAccessManager instance to send requests through.
            ///
            explicit StoreServiceClient(const QUrl &baseUrl = QUrl(), NetworkAccessManager *nam = NULL);
        };

    } // namespace Services

} // namespace Origin

#endif //_SUBSCRIPTIONSERVICECLIENT_H