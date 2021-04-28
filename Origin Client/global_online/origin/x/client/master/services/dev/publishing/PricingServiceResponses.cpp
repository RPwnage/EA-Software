//    PricingServiceResponses.cpp
//    Copyright (c) 2014, Electronic Arts
//    All rights reserved.

#include <QNetworkRequest>

#include "services/publishing/ConversionUtil.h"
#include "services/publishing/PricingServiceResponses.h"

#include "services/log/LogService.h"

namespace Origin
{

namespace Services
{

namespace Publishing
{

PricingResponse::PricingResponse(AuthNetworkRequest request)
    : OriginAuthServiceResponse(request)
{
}

bool PricingResponse::parseSuccessBody(QIODevice *body)
{
    QByteArray entitlementsPayload = body->readAll();

    QJsonParseError jsonResult;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(entitlementsPayload, &jsonResult);
    if (QJsonParseError::NoError != jsonResult.error)
    {
        return false;
    }

    const QJsonObject &doc = jsonDoc.object();
    foreach (const QJsonValue &offerPriceVal, doc["offerPriceList"].toArray())
    {
        const QJsonObject &offerPrice = offerPriceVal.toObject();
        OfferPricingData pricingData;

        if (ConversionUtil::ConvertOfferPricing(offerPrice, pricingData))
        {
            const QString &offerId = offerPrice["offerId"].toString();
            m_prices[offerId] = pricingData;
        }
    }

    return true;
}

} // namespace Publishing

} // namespace Services

} // namespace Origin
