///////////////////////////////////////////////////////////////////////////////
// SubscriptionVaultServiceResponse.h
//
// Author: Hans van Veenendaal
// Copyright (c) 2014 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef __SUBSCRIPTIONVAULTSERVICERESPONSE_H__
#define __SUBSCRIPTIONVAULTSERVICERESPONSE_H__

#include "services/rest/OriginServiceResponse.h"
#include "services/rest/OriginServiceMaps.h"
#include "services/plugin/PluginAPI.h"

#include <QVector>
#include <QHash>
#include <server.h>

namespace Origin
{
    namespace Services
    {
        namespace Subscription
        {
            class ORIGIN_PLUGIN_API SubscriptionVaultInfoResponse : public OriginServiceResponse
            {
            public:
                explicit SubscriptionVaultInfoResponse( QNetworkReply *);
                const server::SubscriptionVaultInfoT &info() const { return mInfo; }

            protected:
                bool parseSuccessBody(QIODevice*);

            private:
                server::SubscriptionVaultInfoT mInfo;
            };
        }
    }
}

#endif //__SUBSCRIPTIONVAULTSERVICERESPONSE_H__
