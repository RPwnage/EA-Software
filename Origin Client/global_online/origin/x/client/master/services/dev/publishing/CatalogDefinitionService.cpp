//    Copyright (c) 2014-2015, Electronic Arts
//    All rights reserved.
//    Author: Kevin Wertman

#include "services/publishing/CatalogDefinitionService.h"
#include "services/publishing/SignatureVerifier.h"
#include "services/network/NetworkAccessManager.h"
#include "services/network/NetworkAccessManagerFactory.h"
#include "services/rest/OriginAuthServiceResponse.h"
#include "services/rest/OriginCatalogPublicPrivateResponse.h"
#include "services/session/SessionService.h"
#include "services/settings/SettingsManager.h"
#include "services/connection/ConnectionStatesService.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

#include "TelemetryAPIDLL.h"

#include <QJsonDocument>

namespace Origin
{

namespace Services
{

namespace Publishing
{

const static QByteArray CONTENT_TYPE = "application/json";

CatalogDefinitionLookup::CatalogDefinitionLookup(const CatalogDefinitionServiceClient *parent, const QString& offerId,
    const QString &qualifyingOfferId, const QByteArray &lastEtag, qint64 batchTime, bool elevatedPermissions)
    : m_parent(parent)
    , m_reply(NULL)
    , m_offerId(offerId)
    , m_qualifyingOfferId(qualifyingOfferId)
    , m_clientLocale(readSetting(SETTING_LOCALE))
    , m_lastEtag(lastEtag)
    , m_batchTime(batchTime)
    , m_dataSource(DataSourcePublic)
    , m_elevatedPermissions(elevatedPermissions)
    , m_aborted(false)
{
    m_session = Session::SessionService::currentSession();
}

CatalogDefinitionLookup::~CatalogDefinitionLookup()
{
}

void CatalogDefinitionLookup::start()
{
    QUrl url(QString("%1/public/%2/%3").arg(readSetting(SETTING_EcommerceURL),
        QString(QUrl::toPercentEncoding(m_offerId)), m_clientLocale));

    QUrlQuery query;

    query.addQueryItem("country", Services::readSetting(Services::SETTING_CommerceCountry).toString());

    if (m_batchTime > 0)
    {
        query.addQueryItem("batchTime", QString::number(m_batchTime));
    }

    url.setQuery(query);

    QNetworkRequest req(url);

    req.setRawHeader("Accept", CONTENT_TYPE);
    if (!m_lastEtag.isEmpty())
    {
        req.setRawHeader("If-None-Match", m_lastEtag);
    }
    req.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);

    // There are a lot of catalog definition requests made by the client,
    // mark them low priority so we don't starve other parts of the system
    req.setPriority(QNetworkRequest::LowPriority);

    m_reply = new OriginCatalogPublicPrivateResponse(m_parent->getAuth(m_session, req));
    ORIGIN_VERIFY_CONNECT(m_reply, &OriginAuthServiceResponse::finished, this, &CatalogDefinitionLookup::onLookupFinished);
}

void CatalogDefinitionLookup::startPrivateLookup()
{
    QUrl url(QString("%1/private/%2/%3").arg(readSetting(SETTING_EcommerceURL),
        QString(QUrl::toPercentEncoding(m_offerId)), m_clientLocale));
    QUrlQuery query;

    query.addQueryItem("country", Services::readSetting(Services::SETTING_CommerceCountry).toString());
    query.addQueryItem("machine_hash", PlatformService::machineHashAsString().toUtf8());
    if (!m_qualifyingOfferId.isEmpty() && m_offerId != m_qualifyingOfferId)
    {
        query.addQueryItem("parentId", m_qualifyingOfferId);
    }
    url.setQuery(query);

    QNetworkRequest req(url);

    req.setRawHeader("Accept", CONTENT_TYPE);
    req.setRawHeader("AuthToken", sessionToken(m_session).toUtf8());
    if (!m_lastEtag.isEmpty())
    {
        req.setRawHeader("If-None-Match", m_lastEtag);
    }
    req.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);

    // There are a lot of catalog definition requests made by the client,
    // mark them low priority so we don't starve other parts of the system
    req.setPriority(QNetworkRequest::LowPriority);

    m_reply = new OriginCatalogPublicPrivateResponse(m_parent->getAuth(m_session, req));
    ORIGIN_VERIFY_CONNECT(m_reply, &OriginAuthServiceResponse::finished,
        this, &CatalogDefinitionLookup::onLookupFinished);
}

void CatalogDefinitionLookup::abort()
{
    m_aborted = true;
    if (m_reply)
    {
        m_reply->abort();
    }
}

void CatalogDefinitionLookup::onLookupFinished()
{
    OriginAuthServiceResponse *reply = dynamic_cast<OriginAuthServiceResponse *>(sender());
    ORIGIN_ASSERT(reply);

    processReply(reply->reply());
    reply->deleteLater();
}

void CatalogDefinitionLookup::processReply(QNetworkReply *reply)
{
    QNetworkReply::NetworkError error = reply->error();
    const QByteArray &eTag = reply->rawHeader("ETag");
    const int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    bool emitFinished = true;
    const bool isPublicOffer = m_dataSource == DataSourcePublic;

    // If we were able to successfully retrieve offer data from /public and we have permissions to 
    // retrieve from /private, use more detailed /private data instead.
    if ((304 == httpCode || QNetworkReply::NoError == error) &&
        isPublicOffer && m_elevatedPermissions && !m_qualifyingOfferId.isEmpty() && !m_session.isNull())
    {
        m_dataSource = DataSourcePrivatePermissions;
        startPrivateLookup();
        return;
    }

    if (m_aborted || !Connection::ConnectionStatesService::isUserOnline(m_session))
    {
        // Avoid sending error telemetry or heartbeat data if requests failed due to user logging out or entering offline mode.
        emit aborted();
    }
    // Sometimes Qt will transparently turn a 304 into a 200 and serve it from the cache
    else if (304 == httpCode ||
            (QNetworkReply::NoError == error && !eTag.isEmpty() && eTag == m_lastEtag))
    {
        // Nothing changed
        emit unchanged();
    }
    else if (QNetworkReply::NoError == error)
    {
        const QByteArray &signatureKey = (isPublicOffer ? m_offerId : PlatformService::machineHashAsString()).toUtf8();
        CatalogDefinitionRef definition = CatalogDefinition::UNDEFINED;

        m_responseBytes = reply->readAll();
        m_responseSignature = QByteArray::fromBase64(reply->rawHeader(SignatureVerifier::SIGNATURE_HEADER));
        if (SignatureVerifier::verify(signatureKey, m_responseBytes, m_responseSignature))
        {
            definition = CatalogDefinition::parseJsonData(m_offerId, m_responseBytes, false, m_dataSource);
        }

        if (definition != CatalogDefinition::UNDEFINED)
        {
            definition->setETag(eTag);
            definition->setSignature(m_responseSignature);
            definition->setLocale(m_clientLocale);
            definition->setRefreshedFromCdnDate(QDateTime::currentDateTimeUtc());
            definition->setDataSource(m_dataSource);
            definition->setQualifyingOfferId(m_qualifyingOfferId);

            if(m_batchTime > 0)
            {
                definition->setBatchTime(m_batchTime);
            }

            emit succeeded(definition);
        }
        else
        {
            // Invalid response.  Error logging and telemetry handled in CatalogDefinition::parseJsonData.
            emit failed();
        }
    }
    else if (Http403ClientErrorForbidden == httpCode || Http404ClientErrorNotFound == httpCode)
    {
        if (isPublicOffer && !m_qualifyingOfferId.isEmpty() && !m_session.isNull())
        {
            // For this one case, we know it's confidential data and we must make a JIT request
            m_dataSource = DataSourcePrivateConfidential;
            startPrivateLookup();
            emitFinished = false;
        }
        else
        {
            // Consider the offer confidential if we got a 403/404 and either aren't qualified to query /private or already tried /private.
            emit confidential();
        }
    }
    else
    {
        ORIGIN_LOG_ERROR << "Failed to look up catalog definition for " << m_offerId << ". HTTP = " << httpCode << "; Qt = " << error;
        GetTelemetryInterface()->Metric_CATALOG_DEFINTION_LOOKUP_ERROR(m_offerId.toLatin1(), error, httpCode, isPublicOffer);
        emit failed();
    }

    if (emitFinished)
    {
        emit finished();
    }
}

OfferUpdatedDateQuery::OfferUpdatedDateQuery(const CatalogDefinitionServiceClient *serviceClient, const QStringList &offerIds)
    : m_serviceClient(serviceClient)
    , m_reply(NULL)
    , m_offerIds(offerIds)
{
    foreach (const QString &offerId, offerIds)
    {
        m_offerUpdatedDates[offerId] = Publishing::INVALID_BATCH_TIME;
    }
}

OfferUpdatedDateQuery::~OfferUpdatedDateQuery()
{
}

void OfferUpdatedDateQuery::start()
{
    QUrl url(QString("%1/offerUpdatedDate").arg(readSetting(SETTING_EcommerceURL).toString()));
    QUrlQuery query;

    query.addQueryItem("offerIds", m_offerIds.join(','));
    url.setQuery(query);

    QNetworkRequest req(url);

    req.setRawHeader("Accept", CONTENT_TYPE);
    req.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);

    m_reply = new OriginAuthServiceResponse(m_serviceClient->getAuth(Session::SessionService::currentSession(), req));
    ORIGIN_VERIFY_CONNECT(m_reply, &OriginAuthServiceResponse::finished, this, &OfferUpdatedDateQuery::queryFinished);
}

void OfferUpdatedDateQuery::abort()
{
    if (m_reply)
    {
        m_reply->abort();
    }
}

void OfferUpdatedDateQuery::queryFinished()
{
    OriginAuthServiceResponse *resp = dynamic_cast<OriginAuthServiceResponse *>(sender());
    ORIGIN_ASSERT(resp);

    QNetworkReply *reply = resp->reply();
    if (reply && QNetworkReply::NoError == reply->error())
    {
        QJsonParseError jsonResult;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &jsonResult);

        if (QJsonParseError::NoError == jsonResult.error)
        {
            foreach (const QJsonValue &updatedDateValue, jsonDoc.object()["offer"].toArray())
            {
                const QJsonObject &updatedDateObj = updatedDateValue.toObject();
                const QString &offerId = updatedDateObj["offerId"].toString();
                const QString &updatedDateStr = updatedDateObj["updatedDate"].toString();
                const QDateTime &updatedDate = QDateTime::fromString(updatedDateStr, Qt::ISODate);

                ORIGIN_ASSERT(m_offerUpdatedDates.contains(offerId));
                if (!offerId.isEmpty() && updatedDate.isValid())
                {
                    m_offerUpdatedDates[offerId] = updatedDate.toMSecsSinceEpoch() / 1000;
                }
            }
        }
    }

    resp->deleteLater();

    emit finished();
}

CatalogDefinitionServiceClient::CatalogDefinitionServiceClient()
{
}

CatalogDefinitionServiceClient::~CatalogDefinitionServiceClient()
{
}

CatalogDefinitionLookup *CatalogDefinitionServiceClient::lookupCatalogDefinition(const QString &offerId,
    const QString &qualifyingOfferId, const QByteArray &lastEtag, qint64 batchTime, bool elevatedPermissions) const
{
    return new CatalogDefinitionLookup(this, offerId, qualifyingOfferId, lastEtag, batchTime, elevatedPermissions);
}

OfferUpdatedDateQuery *CatalogDefinitionServiceClient::queryOfferUpdatedDates(const QStringList &offerIds)
{
    return new OfferUpdatedDateQuery(this, offerIds);
}

NetworkAccessManager *CatalogDefinitionServiceClient::networkAccessManager() const
{
    static QThreadStorage<NetworkAccessManager *> instances;

    if (instances.hasLocalData())
    {
        return instances.localData();
    }
    else
    {
        NetworkAccessManagerFactory *factory = NetworkAccessManagerFactory::instance();
        NetworkAccessManager *instance = factory->createNetworkAccessManager();

        instances.setLocalData(instance);

        return instance;
    }
}

} // namespace Publishing

} // namespace Services

} // namespace Origin
