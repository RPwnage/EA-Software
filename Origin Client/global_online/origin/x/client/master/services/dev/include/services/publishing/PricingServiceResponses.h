//    PricingServiceResponses.h
//    Copyright (c) 2014, Electronic Arts
//    All rights reserved.

#ifndef PRICING_SERVICE_RESPONSES_H
#define PRICING_SERVICE_RESPONSES_H

#include "services/publishing/CatalogDefinition.h"
#include "services/rest/OriginAuthServiceResponse.h"

namespace Origin
{

namespace Services
{

namespace Publishing
{

class PricingResponse : public OriginAuthServiceResponse
{
public:
    explicit PricingResponse(AuthNetworkRequest request);

    const QMap<QString, OfferPricingData> &prices() const { return m_prices; }

protected:
    bool parseSuccessBody(QIODevice *body);

private:
    QMap<QString, OfferPricingData> m_prices;
    OriginAuthServiceResponse *m_internalRequest;
};

} // namespace Publishing

} // namespace Services

} // namespace Origin

#endif //PRICING_SERVICE_RESPONSES_H
