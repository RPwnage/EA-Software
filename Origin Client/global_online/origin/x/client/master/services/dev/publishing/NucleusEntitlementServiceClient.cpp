///////////////////////////////////////////////////////////////////////////////
// ChatServiceClient.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QNetworkRequest>

#include "services/publishing/NucleusEntitlementServiceClient.h"
#include "services/publishing/NucleusEntitlementServiceResponse.h"
#include "services/settings/SettingsManager.h"
#include "services/platform/PlatformService.h"
#include "services/debug/DebugService.h"

#include "TelemetryAPIDLL.h"

namespace Origin
{

namespace Services
{

namespace Publishing
{

NucleusEntitlementServiceClient::NucleusEntitlementServiceClient(NetworkAccessManager *nam)
    : ECommerceServiceClient(nam)
{
  
}

/// references: http://opqa-online.rws.ad.ea.com/test/Origin/serverAPIDocs/ecommerce/
NucleusEntitlementServiceResponse* NucleusEntitlementServiceClient::entitlementInfoPriv(Session::SessionRef session, quint64 entitlementId )
{
    QString myUrl("entitlements/" + nucleusId(session));
    QUrl serviceUrl(urlForServicePath(myUrl));
    QUrlQuery serviceUrlQuery(serviceUrl);

    serviceUrlQuery.addQueryItem("machine_hash", PlatformService::machineHashAsString());
    serviceUrlQuery.addQueryItem("entitlementId", QString::number(entitlementId));

    serviceUrl.setQuery(serviceUrlQuery);
    QNetworkRequest request(serviceUrl);
    request.setRawHeader("AuthToken", sessionToken(session).toUtf8());
    request.setRawHeader("Accept", "application/vnd.origin.v2+json");
    request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);

    return new NucleusEntitlementServiceResponse(getAuth(session, request), EntitlementRefreshTypeSingle);
}

/// references: http://opqa-online.rws.ad.ea.com/test/Origin/serverAPIDocs/ecommerce/
NucleusEntitlementServiceResponse* NucleusEntitlementServiceClient::baseGamesPriv(Session::SessionRef session)
{
    QString myUrl("basegames/" + nucleusId(session));
    QUrl serviceUrl(urlForServicePath(myUrl));
    QUrlQuery serviceUrlQuery(serviceUrl);

    serviceUrlQuery.addQueryItem("machine_hash", PlatformService::machineHashAsString());

    serviceUrl.setQuery(serviceUrlQuery);
    QNetworkRequest request(serviceUrl);
    request.setRawHeader("AuthToken", sessionToken(session).toUtf8());
    request.setRawHeader("Accept", "application/vnd.origin.v2+json");
    request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);

    return new NucleusEntitlementServiceResponse(getAuth(session, request), EntitlementRefreshTypeFull);
}

/// references: http://opqa-online.rws.ad.ea.com/test/Origin/serverAPIDocs/ecommerce/
NucleusEntitlementServiceResponse* NucleusEntitlementServiceClient::extraContentPriv(Session::SessionRef session, const QString& masterTitleId )
{
    QString myUrl("extracontent/" + nucleusId(session));

    QUrl serviceUrl(urlForServicePath(myUrl));
    QUrlQuery serviceUrlQuery(serviceUrl);

    serviceUrlQuery.addQueryItem("machine_hash", PlatformService::machineHashAsString());
    serviceUrlQuery.addQueryItem("masterTitleId", masterTitleId);

    serviceUrl.setQuery(serviceUrlQuery);
    QNetworkRequest request(serviceUrl);
    request.setRawHeader("AuthToken", sessionToken(session).toUtf8());
    request.setRawHeader("Accept", "application/vnd.origin.v2+json");
    request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);

    return new NucleusEntitlementServiceResponse(getAuth(session, request), EntitlementRefreshTypeExtraContent, masterTitleId);
}

/// references: http://opqa-online.rws.ad.ea.com/test/Origin/serverAPIDocs/ecommerce/
NucleusEntitlementServiceResponse* NucleusEntitlementServiceClient::contentUpdatesPriv(Session::SessionRef session, const QDateTime& lastModifiedDate )
{
    QString myUrl("contentupdates/" + nucleusId(session));
    QUrl serviceUrl(urlForServicePath(myUrl));
    QUrlQuery serviceUrlQuery(serviceUrl);

    serviceUrlQuery.addQueryItem("lastModifiedDate", lastModifiedDate.toString(QString("yyyy-MM-dd'T'HH:mm'Z'")));
    serviceUrlQuery.addQueryItem("machine_hash", PlatformService::machineHashAsString());

    serviceUrl.setQuery(serviceUrlQuery);
    QNetworkRequest request(serviceUrl);

    request.setRawHeader("AuthToken", sessionToken(session).toUtf8());
    request.setRawHeader("Accept", "application/vnd.origin.v2+json");
    request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);

    return new NucleusEntitlementServiceResponse(getAuth(session, request), EntitlementRefreshTypeIncremental);
}

/// references: http://opqa-online.rws.ad.ea.com/test/Origin/serverAPIDocs/ecommerce/
NucleusEntitlementServiceResponse* NucleusEntitlementServiceClient::sdkEntitlementsPriv(Session::SessionRef session, const QStringList& filterGroups, const QStringList& filterOffers, const QStringList& filterItems, const QStringList& filterEntitlementIds, const bool includeChildGroups)
{
    QString myUrl("entitlements/" + nucleusId(session));
    QUrl serviceUrl(urlForServicePath(myUrl));
    QUrlQuery serviceUrlQuery(serviceUrl);

    if (!filterGroups.isEmpty())
    {
        serviceUrlQuery.addQueryItem("groupName", QUrl::toPercentEncoding(filterGroups.join(',')));
    }

    if (!filterOffers.isEmpty())
    {
        serviceUrlQuery.addQueryItem("productId", QUrl::toPercentEncoding(filterOffers.join(',')));
    }

    if (!filterEntitlementIds.isEmpty())
    {
        serviceUrlQuery.addQueryItem("entitlementId", filterEntitlementIds.at(0));
    }

    if (!filterItems.isEmpty())
    {
        serviceUrlQuery.addQueryItem("entitlementTag", filterItems.at(0));
    }

    if (includeChildGroups)
    {
        serviceUrlQuery.addQueryItem("includeChildGroups", "true");
    }

    serviceUrlQuery.addQueryItem("machine_hash", PlatformService::machineHashAsString());

    serviceUrl.setQuery(serviceUrlQuery);
    QNetworkRequest request(serviceUrl);

    request.setRawHeader("AuthToken", sessionToken(session).toUtf8());
    request.setRawHeader("Accept", "application/vnd.origin.v2+json");
    request.setRawHeader("X-Origin-SDK", "true");
    request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);

    return new NucleusEntitlementServiceResponse(getAuth(session, request), EntitlementRefreshTypeSDK);
}

} // namespace Publishing

} // namespace Services

} // namespace Origin
