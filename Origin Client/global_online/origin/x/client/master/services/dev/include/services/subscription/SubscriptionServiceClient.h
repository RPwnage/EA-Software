///////////////////////////////////////////////////////////////////////////////
// SubscriptionServiceClient.h
//
// Author: Hans van Veenendaal
// Copyright (c) 2014 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _SUBSCRIPTIONSERVICECLIENT_H
#define _SUBSCRIPTIONSERVICECLIENT_H

#include "services/rest/OriginServiceClient.h"
#include "services/subscription/SubscriptionServiceResponse.h"
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
            class ORIGIN_PLUGIN_API SubscriptionServiceClient : public OriginServiceClient
            {
            public:
                friend class OriginClientFactory<SubscriptionServiceClient>;

                ///
                /// \brief Returns subscription for the user
                ///
                static SubscriptionInfoResponse *subscriptionInfo(const QString &group, const QString &state)
                {
                    return OriginClientFactory<SubscriptionServiceClient>::instance()->subscriptionInfoPriv(group, state);
                }

                ///
                /// \brief Get subscription detail
                ///
                static SubscriptionDetailInfoResponse *subscriptionDetailInfo(const QString subscriptionURI)
                {
                    return OriginClientFactory<SubscriptionServiceClient>::instance()->subscriptionDetailInfoPriv(subscriptionURI);
                }

            private:

                /// \brief The actual implementation of the subscription info query.
                /// \param subscriptionGroup The subscription group
                SubscriptionInfoResponse *subscriptionInfoPriv(const QString &group, const QString &state);

                /// \brief The actual implementation of the subscription detail info query.
                /// \param subscriptionGroup The subscription group
                SubscriptionDetailInfoResponse *subscriptionDetailInfoPriv(const QString &uri);

                /// 
                /// \brief Creates a new achievement service client
                ///
                /// \param baseUrl       Base URL for the achievement service API.
                /// \param nam           QNetworkAccessManager instance to send requests through.
                ///
                explicit SubscriptionServiceClient(const QUrl &baseUrl = QUrl(), NetworkAccessManager *nam = NULL);
            };
        }
    }
}

#endif //_SUBSCRIPTIONSERVICECLIENT_H