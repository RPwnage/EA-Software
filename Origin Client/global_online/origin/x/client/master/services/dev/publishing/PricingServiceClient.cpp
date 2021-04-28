//    PricingServiceClient.cpp
//    Copyright (c) 2014, Electronic Arts
//    All rights reserved.

#include <QNetworkRequest>

#include "services/publishing/PricingServiceClient.h"
#include "services/settings/SettingsManager.h"

namespace Origin
{

namespace Services
{

namespace Publishing
{

PricingServiceClient *PricingServiceClient::instance()
{
    return OriginClientFactory<PricingServiceClient>::instance();
}

PricingServiceClient::PricingServiceClient(NetworkAccessManager *nam)
    : ECommerceServiceClient(nam)
{
}

QList<PricingResponse *> PricingServiceClient::getPricing(Session::SessionRef session, const QStringList &offerIds, const QString &masterTitleId)
{
    QList<PricingResponse *> responses;

    for (int start = 0; start < offerIds.size(); start += MAX_OFFERS_PER_CALL)
    {
        QUrl serviceUrl = urlForServicePath(QString("pricing/%1").arg(nucleusId(session)));
        QUrlQuery serviceUrlQuery;
        QStringList requestIds = offerIds.mid(start, MAX_OFFERS_PER_CALL);

        serviceUrlQuery.addQueryItem("masterTitleId", masterTitleId);
        serviceUrlQuery.addQueryItem("offerIds", requestIds.join(','));

        serviceUrl.setQuery(serviceUrlQuery);

        QNetworkRequest request(serviceUrl);

        request.setRawHeader("Accept", "application/json");
        request.setRawHeader("AuthToken", sessionToken(session).toUtf8());

        responses.append(new PricingResponse(getAuth(session, request)));
    }

    return responses;
}

} // namespace Publishing

} // namespace Services

} // namespace Origin
