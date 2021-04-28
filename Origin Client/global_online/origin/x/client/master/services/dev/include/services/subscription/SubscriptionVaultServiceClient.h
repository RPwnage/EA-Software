///////////////////////////////////////////////////////////////////////////////
// SubscriptionVaultServiceClient.h
//
// Author: Hans van Veenendaal
// Copyright (c) 2014 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _SUBSCRIPTIONVAULTSERVICECLIENT_H
#define _SUBSCRIPTIONVAULTSERVICECLIENT_H

#include "services/rest/OriginServiceClient.h"
#include "services/subscription/SubscriptionVaultServiceResponse.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        namespace Subscription
        {

            ///
            /// \brief HTTP service client for the communication with the EADP subscription endpoint.
            ///
            class ORIGIN_PLUGIN_API SubscriptionVaultServiceClient : public OriginServiceClient
            {
            public:
                friend class OriginClientFactory<SubscriptionVaultServiceClient>;

                ///
                /// \brief Returns subscription for the user
                ///
                static SubscriptionVaultInfoResponse *subscriptionVaultInfo(const QString &group)
                {
                    return OriginClientFactory<SubscriptionVaultServiceClient>::instance()->subscriptionVaultInfoPriv(group);
                }

            private:

                /// \brief The actual implementation of the subscription info query.
                /// \param subscriptionGroup The subscription group
                SubscriptionVaultInfoResponse *subscriptionVaultInfoPriv(const QString &group);

                /// 
                /// \brief Creates a new achievement service client
                ///
                /// \param baseUrl       Base URL for the achievement service API.
                /// \param nam           QNetworkAccessManager instance to send requests through.
                ///
                explicit SubscriptionVaultServiceClient(const QUrl &baseUrl = QUrl(), NetworkAccessManager *nam = NULL);
            };
        }
    }
}

#endif //_SUBSCRIPTIONVAULTSERVICECLIENT_H