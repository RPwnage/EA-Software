///////////////////////////////////////////////////////////////////////////////
// SubscriptionServiceResponse.h
//
// Author: Hans van Veenendaal
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef __SUBSCRIPTIONSERVICERESPONSE_H__
#define __SUBSCRIPTIONSERVICERESPONSE_H__

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
            class ORIGIN_PLUGIN_API SubscriptionInfoResponse : public OriginServiceResponse
            {
            public:
                explicit SubscriptionInfoResponse( QNetworkReply *);
                const server::SubscriptionInfoT &info() const { return mInfo; }

            protected:
                bool parseSuccessBody(QIODevice*);

            private:
                server::SubscriptionInfoT mInfo;
            };

            class ORIGIN_PLUGIN_API SubscriptionDetailInfoResponse : public OriginServiceResponse
            {
            public:
                explicit SubscriptionDetailInfoResponse( QNetworkReply *);
                const server::SubscriptionDetailInfoT &info() const { return mInfo; }

            protected:
                bool parseSuccessBody(QIODevice*);

            private:
                server::SubscriptionDetailInfoT mInfo;
            };

            class ORIGIN_PLUGIN_API SubscriptionEntitleResponse : public OriginServiceResponse
            {
            public:
                explicit SubscriptionEntitleResponse(const QString &offerId, QNetworkReply *reply);
                const QString &offerId() const { return mOfferId; }

            protected:
                virtual bool parseSuccessBody(QIODevice *);

            private slots:
                void onRequestTimeout();

            private:
                const QString mOfferId;
                QTimer mTimeout;
            };
        }
    }
}

#endif //__SUBSCRIPTIONSERVICERESPONSE_H__
