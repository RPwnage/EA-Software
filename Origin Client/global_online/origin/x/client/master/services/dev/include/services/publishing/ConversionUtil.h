//    Copyright � 2011-2012, Electronic Arts
//    All rights reserved.

#ifndef ORIGINENTITLEMENTCONVERSIONUTIL_H
#define ORIGINENTITLEMENTCONVERSIONUTIL_H

#include <QVector>
#include <QMap>
#include <QJsonObject>

#include "services/publishing/CatalogDefinition.h"
#include "ecommerce2.h"

namespace Origin
{

namespace Services
{

namespace Publishing
{

namespace ConversionUtil
{

QDateTime ConvertTime(OriginTimeT const& t);
OriginTimeT ConvertTime(QDateTime const& t);

QString FormatPrice(double value, const QString& decimalPattern, const QString& orientation, const QString& symbol, const QString& currency);

bool ConvertOffer(const QJsonObject& jsonRep, CatalogDefinitionRef& catalogDef);
bool ConvertOfferPricing(const QJsonObject &jsonRep, OfferPricingData &pricingData);
ItemSubType ConvertGameDistributionSubType(const QString& productId, const QString& type);

void CloudSaveWhiteListBlackListSetup();

} // namespace ConversionUtil

} // namespace Publishing

} // namespace Services

} // namespace Origin

#endif // ORIGINENTITLEMENTCONVERSIONUTIL_H