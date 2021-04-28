//    PricingServiceClient.h
//    Copyright (c) 2014, Electronic Arts
//    All rights reserved.

#ifndef PRICING_SERVICE_CLIENT_H
#define PRICING_SERVICE_CLIENT_H

#include "ECommerceServiceClient.h"

#include "services/plugin/PluginAPI.h"
#include "services/publishing/PricingServiceResponses.h"

namespace Origin
{

namespace Services
{

namespace Publishing
{

/// \brief HTTP service client for dynamic offer pricing
class ORIGIN_PLUGIN_API PricingServiceClient : public ECommerceServiceClient
{
public:
    friend class OriginClientFactory<PricingServiceClient>;

    static PricingServiceClient *instance();

    QList<PricingResponse *> getPricing(Session::SessionRef session, const QStringList &offerIds, const QString &masterTitleId);

private:
    enum { MAX_OFFERS_PER_CALL = 20 };

    /// \brief Creates a dynamic pricing service client
    /// \param nam           QNetworkAccessManager instance to send requests through
    explicit PricingServiceClient(NetworkAccessManager *nam = NULL);
};

} // namespace Publishing

} // namespace Services

} // namespace Origin

#endif //PRICING_SERVICE_CLIENT_H
