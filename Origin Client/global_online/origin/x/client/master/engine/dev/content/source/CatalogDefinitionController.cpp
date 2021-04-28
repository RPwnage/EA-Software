//    Copyright (c) 2014-2015, Electronic Arts
//    All rights reserved.
//    Author: Kevin Wertman

#include <QJsonDocument>
#include <QJsonObject>

#include <EAStdC/EARandomDistribution.h>

#include "engine/content/CatalogDefinitionController.h"

#include "services/config/OriginConfigService.h"
#include "services/debug/DebugService.h"
#include "services/downloader/Common.h"
#include "services/downloader/StringHelpers.h"
#include "services/log/LogService.h"
#include "services/publishing/CatalogDefinitionService.h"
#include "services/publishing/SignatureVerifier.h"
#include "services/publishing/ConversionUtil.h"
#include "services/settings/SettingsManager.h"
#include "services/heartbeat/Heartbeat.h"

#include "TelemetryAPIDLL.h"

namespace Origin
{

namespace Engine
{

namespace Content
{

const unsigned int CatalogSerializationMagic = 0xb25a145e;
const unsigned int CatalogSerializationVersion = 7;
const QDataStream::Version CatalogSerializationDataStreamVersion = QDataStream::Qt_5_0;

const QString CatalogDefinitionFolder = "CatalogCache";
const QString CatalogDefinitionExtension = "cdef";

const Services::CryptoService::Cipher CatalogDefinitionCacheCipher = Services::CryptoService::BLOWFISH;
const Services::CryptoService::CipherMode CatalogDefinitionCacheCipherMode = Services::CryptoService::CIPHER_BLOCK_CHAINING;
const QByteArray CatalogDefinitionCacheEntropy = "0\nFKxD f-KSU%~vkt&0w} 9CS<K62k &vl,J^kk0(eVz/w_F`Kx8'^?2t]YbjY4n?%H$"
    "(TKh.Zt,E_Hae7z2;k8Jaa) {q7S(zSSKlgOv_nM$lR5RTpuT?f<qrJ\v|&x_<*334P^FXU%8TMaIcJY9./w+Uxrwc;y4zq=X0|h6d6sog/ck0mfE";

#ifdef _DEBUG
QStringList s_isolateProductIds;

bool CatalogDefinitionController::ignoreOfferViaIsolateProductIds(const QString& productId)
{
    return (!s_isolateProductIds.isEmpty() && !s_isolateProductIds.contains(productId, Qt::CaseInsensitive));
}
#endif

CatalogDefinitionQuery::CatalogDefinitionQuery(const QString& offerId, Origin::Services::Publishing::CatalogDefinitionLookup* lookup, Origin::Services::Publishing::CatalogDefinitionRef currentDef)
    : m_offerId(offerId)
    , m_definition(currentDef)
    , m_lookup(lookup)
    , m_confidential(false)
{
    if(m_lookup != NULL)
    {
        ORIGIN_VERIFY_CONNECT(m_lookup, &Services::Publishing::CatalogDefinitionLookup::failed,
            this, &CatalogDefinitionQuery::onDefLookupFailed);
        ORIGIN_VERIFY_CONNECT(m_lookup, &Services::Publishing::CatalogDefinitionLookup::unchanged,
            this, &CatalogDefinitionQuery::onDefLookupUnchanged);
        ORIGIN_VERIFY_CONNECT(m_lookup, &Services::Publishing::CatalogDefinitionLookup::succeeded,
            this, &CatalogDefinitionQuery::onDefLookupUpdated);
        ORIGIN_VERIFY_CONNECT(m_lookup, &Services::Publishing::CatalogDefinitionLookup::aborted,
            this, &CatalogDefinitionQuery::onDefLookupAborted);
        ORIGIN_VERIFY_CONNECT(m_lookup, &Services::Publishing::CatalogDefinitionLookup::confidential,
            this, &CatalogDefinitionQuery::onDefLookupConfidential);
        m_lookup->start();
    }
    else
    {
        QMetaObject::invokeMethod(this, "onDefLookupUnchanged", Qt::QueuedConnection);
    }
}

void CatalogDefinitionQuery::abort()
{
    m_lookup->abort();
}

void CatalogDefinitionQuery::onDefLookupUpdated(Origin::Services::Publishing::CatalogDefinitionRef catalogDef)
{
    // If we already had a handle to a valid catalog defintion, the controller has updated it in place with the new
    // definition information, so we just take the new definition reference if we didn't already have one.
    if(m_definition == Origin::Services::Publishing::CatalogDefinition::UNDEFINED)
    {
        m_definition = catalogDef;
    }

    emit catalogDefinition(m_definition);
    emit finished();
}

void CatalogDefinitionQuery::onDefLookupUnchanged()
{
    using Origin::Services::Publishing::CatalogDefinitionLookup;

    // It's possible that a previously private offer's visibility changed without its data changing
    CatalogDefinitionLookup *lookup = dynamic_cast<CatalogDefinitionLookup *>(sender());
    if (lookup && m_definition)
    {
        m_definition->setDataSource(lookup->dataSource());
    }

    emit catalogDefinition(m_definition);
    emit finished();
}

void CatalogDefinitionQuery::onDefLookupFailed()
{
    if(m_definition == Origin::Services::Publishing::CatalogDefinition::UNDEFINED)
    {
        emit catalogDefinitionNotFound(m_offerId);
    }
    else
    {
        emit catalogDefinition(m_definition);
    }

    emit finished();
}

void CatalogDefinitionQuery::onDefLookupAborted()
{
    emit finished();
}

void CatalogDefinitionQuery::onDefLookupConfidential()
{
    m_confidential = true;
    emit finished();
}

CatalogDefinitionController::DirtyBitsCatalogUpdate::DirtyBitsCatalogUpdate(const QString &id_, qint64 timestamp, UpdateType updateType_)
    : id(id_)
    , changeBatchTimestamp(timestamp)
    , updateType(updateType_)
{
}

CatalogDefinitionController* CatalogDefinitionController::sInstance = NULL;

void CatalogDefinitionController::init()
{
    // TODO: Move off of UI thread.
    if(sInstance == NULL)
    {
#ifdef _DEBUG
        s_isolateProductIds = Origin::Services::readSetting(Services::SETTING_IsolateProductIds).toString().split(",", QString::SkipEmptyParts);
#endif
        sInstance = new CatalogDefinitionController();
        sInstance->startup();
    }
}

void CatalogDefinitionController::release()
{
    if(sInstance != NULL)
    {
        sInstance->shutdown();
        sInstance->deleteLater();
        sInstance = NULL;
    }
}

CatalogDefinitionController* CatalogDefinitionController::instance()
{
    return sInstance;
}

CatalogDefinitionController::CatalogDefinitionController()
    : m_lookupSuccessCount(0)
    , m_lookupFailureCount(0)
    , m_lookupConfidentialCount(0)
    , m_lookupUnchangedCount(0)
    , m_catalogDefinitionLock(QMutex::Recursive)
    , m_refreshRequested(false)
    , m_catalogDefinitionRefreshTimer(this)
    , m_dirtyBitsTelemetryDelayTimer(this)
    , m_catalogDefLookupTelemetryTimer(this)
    , m_writeUserCacheDataTimer(this)
{
    qint64 refreshInterval = Services::OriginConfigService::instance()->ecommerceConfig().catalogDefinitionRefreshInterval;
    if (refreshInterval > 0)
    {
        m_catalogDefinitionRefreshTimer.setInterval(refreshInterval);
        ORIGIN_VERIFY_CONNECT(&m_catalogDefinitionRefreshTimer, &QTimer::timeout,
            this, &CatalogDefinitionController::onRefreshDefinitionTimerFired);
    }

    qint64 catalogDefLookupTelemetryInterval = Services::OriginConfigService::instance()->ecommerceConfig().catalogDefinitionLookupTelemetryInterval;
    if(catalogDefLookupTelemetryInterval > 0)
    {
        m_catalogDefLookupTelemetryTimer.setInterval(catalogDefLookupTelemetryInterval);
        ORIGIN_VERIFY_CONNECT(&m_catalogDefLookupTelemetryTimer, &QTimer::timeout,
            this, &CatalogDefinitionController::onCatalogDefinitionLookupTelemetryTimeout);
    }

    // Only write the private cache when we haven't received a new private catalog definition for
    // two seconds to avoid thrashing the private cache file when loading a lot of private definitions
    m_writeUserCacheDataTimer.setInterval(2000);
    m_writeUserCacheDataTimer.setSingleShot(true);
    ORIGIN_VERIFY_CONNECT(&m_writeUserCacheDataTimer, &QTimer::timeout,
        this, &CatalogDefinitionController::writeUserCacheData);

    ORIGIN_VERIFY_CONNECT(
        Services::Session::SessionService::instance(), &Services::Session::SessionService::endSessionComplete,
        this, &CatalogDefinitionController::onEndSessionComplete);

    const QString &env = Services::readSetting(Origin::Services::SETTING_EnvironmentName).toString();
    QString catalogDefinitionFolder = CatalogDefinitionFolder;
    if (Services::env::production != env)
    {
        catalogDefinitionFolder.append(QString(".%1").arg(env));
    }

    m_definitionCacheFolder = QString("%1%2").arg(Services::PlatformService::machineStoragePath(), catalogDefinitionFolder);
    if (!QDir("/").exists(m_definitionCacheFolder))
    {
        ORIGIN_LOG_ACTION << "Creating catalog cache folder: " << m_definitionCacheFolder;
        Services::PlatformService::createFolderElevated(m_definitionCacheFolder, "D:(A;OICI;GA;;;WD)");
    }

    EA::StdC::RandomQuality randomGenerator;
    // Offset by 30 seconds so that there's a delay even if the RNG gives us "0".
    int dirtyBitsTelemetryDelay = 30000 + static_cast<int>(randomGenerator.RandomUint32Uniform(Services::OriginConfigService::instance()->ecommerceConfig().dirtyBitsTelemetryDelay));
    ORIGIN_LOG_DEBUG << "Telemetry delay for catalog update dirty bit notifications: " << dirtyBitsTelemetryDelay;
    m_dirtyBitsTelemetryDelayTimer.setInterval(dirtyBitsTelemetryDelay);
    m_dirtyBitsTelemetryDelayTimer.setSingleShot(true);
    ORIGIN_VERIFY_CONNECT(&m_dirtyBitsTelemetryDelayTimer, &QTimer::timeout,
        this, &CatalogDefinitionController::onDirtyBitsTelemetryDelayTimeout);

    // Encrypted cache settings
    setCacheDataVersion(CatalogSerializationVersion);
    setCacheFolder(catalogDefinitionFolder);
    setCacheFileExtension(CatalogDefinitionExtension);
    setCacheCipher(CatalogDefinitionCacheCipher);
    setCacheCipherMode(CatalogDefinitionCacheCipherMode);
    setCacheEntropy(CatalogDefinitionCacheEntropy);
}

CatalogDefinitionController::~CatalogDefinitionController()
{
    ORIGIN_VERIFY_DISCONNECT(
        Services::Session::SessionService::instance(), &Services::Session::SessionService::endSessionComplete,
        this, &CatalogDefinitionController::onEndSessionComplete);
}

void CatalogDefinitionController::startup()
{
    // Start scanning timer...
    if (m_catalogDefinitionRefreshTimer.interval() > 0)
    {
        m_catalogDefinitionRefreshTimer.start();
    }
    
    if (m_catalogDefLookupTelemetryTimer.interval() > 0)
    {
        m_catalogDefLookupTelemetryTimer.start();
    }

    // Initialize cloud save white/black list...
    ORIGIN_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
    Services::Publishing::ConversionUtil::CloudSaveWhiteListBlackListSetup();
}

void CatalogDefinitionController::shutdown()
{
    // Stop it...
    m_catalogDefinitionRefreshTimer.stop();
}

CatalogDefinitionQuery* CatalogDefinitionController::queryCatalogDefinition(const QString& offerId,
    const QString &qualifyingOfferId, qint64 batchTime, RefreshType refreshType, bool elevatedPermissions)
{
    Origin::Services::Publishing::CatalogDefinitionRef currentDefinition = getCachedCatalogDefinition(offerId);
    Origin::Services::Publishing::CatalogDefinitionLookup* lookup = NULL;

    if(currentDefinition != Origin::Services::Publishing::CatalogDefinition::UNDEFINED)
    {
        if (batchTime < 0)
        {
            // If caller has not specified a batch time, use the most recent batch time in the request.
            batchTime = currentDefinition->batchTime();
        }
        else
        {
            // If caller *has* specified a batch time, persist it in case we get a 304 Not Modified.
            // If we get a 200, the batch time will be updated in CatalogDefinitionLookup::processReply.
            currentDefinition->setBatchTime(batchTime);
        }
    }

    if (currentDefinition == Origin::Services::Publishing::CatalogDefinition::UNDEFINED ||
        batchTime > 0 || RefreshAll == refreshType || currentDefinition->isCdnDefinitionStale())
    {
        QByteArray currentETag;

        if (currentDefinition != Origin::Services::Publishing::CatalogDefinition::UNDEFINED)
        {
            currentETag = currentDefinition->eTag();
        }

        lookup = lookupCatalogDefinition(offerId, qualifyingOfferId, currentETag, batchTime, elevatedPermissions);
    }

    return new CatalogDefinitionQuery(offerId, lookup, currentDefinition);
}

bool CatalogDefinitionController::hasCachedCatalogDefinition(const QString& offerId)
{
    QMutexLocker lock(&m_catalogDefinitionLock);
    return m_catalogDefinitions.contains(offerId) ||
        readDefinitionCache(offerId) != Services::Publishing::CatalogDefinition::UNDEFINED;
}

Origin::Services::Publishing::CatalogDefinitionRef CatalogDefinitionController::getCachedCatalogDefinition(const QString& offerId)
{
    QMutexLocker lock(&m_catalogDefinitionLock);
    if (m_catalogDefinitions.contains(offerId))
    {
        return m_catalogDefinitions[offerId];
    }
    else
    {
        return readDefinitionCache(offerId);
    }
}

Services::Publishing::OfferUpdatedDateQuery *CatalogDefinitionController::queryOfferUpdatedDates(const QStringList &offerIds)
{
    Services::Publishing::OfferUpdatedDateQuery *query = m_catalogDefinitionService.queryOfferUpdatedDates(offerIds);
    QMetaObject::invokeMethod(query, "start", Qt::QueuedConnection);
    return query;
}

void CatalogDefinitionController::onDirtyBitsCatalogUpdateNotification(const QByteArray &payload)
{
    // Make sure we do our payload processing on background thread and free up dirty bit thread to continue
    ASYNC_INVOKE_GUARD_ARGS(Q_ARG(QByteArray, payload));

    if (payload.isEmpty())
    {
        return;
    }

    QJsonParseError jsonResult;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &jsonResult);
    if (jsonResult.error != QJsonParseError::NoError)
    {
        GetTelemetryInterface()->Metric_ENTITLEMENT_CATALOG_UPDATE_DBIT_PARSE_ERROR();
        ORIGIN_LOG_ERROR << "Catalog Update payload contains invalid response format.  Can not parse.";
        return;
    }

    const QJsonObject &data = jsonDoc.object()["data"].toObject();
    qint64 changeBatchTimestamp = data["batchTime"].toVariant().toLongLong();
    if (changeBatchTimestamp <= 0)
    {
        ORIGIN_LOG_ERROR << "Failed to parse catalog update payload.";
        GetTelemetryInterface()->Metric_ENTITLEMENT_CATALOG_UPDATE_DBIT_PARSE_ERROR();
        return;
    }

    // List of offer ids for base games updated in batch.
    const QJsonArray &updatedBaseGames = data["baseGameOfferIds"].toArray();
    foreach (const QJsonValue &varOfferId, updatedBaseGames)
    {
        const QString &offerId = varOfferId.toString();
        if (m_catalogDefinitions.contains(offerId))
        {
            const Services::Publishing::CatalogDefinitionRef &def = m_catalogDefinitions[offerId];
            const bool elevatedPermissions = def->dataSource() == Services::Publishing::DataSourcePrivatePermissions;
            lookupCatalogDefinition(offerId, offerId, def->eTag(), changeBatchTimestamp, elevatedPermissions)->start();

            ORIGIN_LOG_EVENT << "Product update detected for offer [" << offerId << "] with modified batch time of "
                "[" << changeBatchTimestamp << "] last server refresh: [" << def->batchTime() << "]";

            m_catalogUpdatesTelemetryData.append(DirtyBitsCatalogUpdate(offerId, changeBatchTimestamp,
                DirtyBitsCatalogUpdate::OfferDefinitionUpdate));
        }
    }

    // Find updated extra content updated in batch.
    const QJsonObject &updatedMasterTitles = data["masterTitles"].toObject();
    for (QJsonObject::const_iterator it = updatedMasterTitles.constBegin(); it != updatedMasterTitles.constEnd(); ++it)
    {
        const QString &masterTitleId = it.key();
        ORIGIN_LOG_EVENT << "Master title update detected for master title ID [" << masterTitleId << "] with modified batch time of "
            "[" << changeBatchTimestamp << "]";

        m_catalogUpdatesTelemetryData.append(DirtyBitsCatalogUpdate(masterTitleId, changeBatchTimestamp,
            DirtyBitsCatalogUpdate::MasterTitleUpdate));

        // EA_TODO("jonkolb", "2014/06/19", "Do we still need to find all owned base game offers with the master title id and update them too?")
        const QJsonArray &extraContentOfferIds = it.value().toArray();
        foreach (const QJsonValue &varOfferId, extraContentOfferIds)
        {
            const QString &offerId = varOfferId.toString();
            if (m_catalogDefinitions.contains(offerId))
            {
                const Services::Publishing::CatalogDefinitionRef &def = m_catalogDefinitions[offerId];
                const bool elevatedPermissions = def->dataSource() == Services::Publishing::DataSourcePrivatePermissions;
                lookupCatalogDefinition(def->productId(), def->qualifyingOfferId(), def->eTag(), changeBatchTimestamp, elevatedPermissions)->start();
            }
        }
    }

    if (!m_catalogUpdatesTelemetryData.isEmpty() && !m_dirtyBitsTelemetryDelayTimer.isActive())
    {
        ORIGIN_LOG_DEBUG << "Starting delay timer for dirty bits catalog update telemetry...";
        m_dirtyBitsTelemetryDelayTimer.start();
    }
}

void CatalogDefinitionController::onLookupSucceeded(Origin::Services::Publishing::CatalogDefinitionRef updatedDef)
{
    using namespace Origin::Services::Publishing;
    using Origin::Services::Publishing::CatalogDefinitionLookup;
    using Origin::Services::Publishing::CatalogDefinitionRef;

    QMutexLocker lock(&m_catalogDefinitionLock);
    if (m_catalogDefinitions.contains(updatedDef->productId()))
    {
        CatalogDefinitionRef currentDef = m_catalogDefinitions[updatedDef->productId()];
        DataSource currentDataSource = currentDef->dataSource();

        currentDef->updateDefinition(updatedDef);

        if (currentDataSource != updatedDef->dataSource())
        {
            // If an offer's visibility has changed, we need to remove its current definition from the cache,
            // it will get added to the new cache below
            // Note that we only consider the visibility changed when the lookup was made to /private because
            // the offer is confidential.
            if (currentDataSource == DataSourcePublic && updatedDef->dataSource() == DataSourcePrivateConfidential)
            {
                QFile::remove(definitionCacheFile(updatedDef->productId()));
            }
            else
            {
                m_writeUserCacheDataTimer.start();
            }
        }
    }
    else
    {
        m_catalogDefinitions.insert(updatedDef->productId(), updatedDef);
    }

    if (updatedDef->dataSource() == DataSourcePublic)
    {
        writeDefinitionCache(updatedDef);
    }
    else
    {
        m_writeUserCacheDataTimer.start();
    }

    // Track successful CDN lookup count in heartbeat.
    Services::Heartbeat::instance()->catalogLookupSuccess();
    ++m_lookupSuccessCount;
}


void CatalogDefinitionController::onLookupUnchanged()
{
    using namespace Origin::Services::Publishing;
    using Origin::Services::Publishing::CatalogDefinitionLookup;
    using Origin::Services::Publishing::CatalogDefinitionRef;

    QMutexLocker lock(&m_catalogDefinitionLock);
    CatalogDefinitionLookup *lookup = dynamic_cast<CatalogDefinitionLookup*>(sender());
    if (lookup && m_catalogDefinitions.contains(lookup->offerId()))
    {
        CatalogDefinitionRef &def = m_catalogDefinitions[lookup->offerId()];
        DataSource currentDataSource = def->dataSource();

        if (currentDataSource != lookup->dataSource())
        {
            def->setSignature(lookup->responseSignature());
            def->setDataSource(lookup->dataSource());

            // If an offer's visibility has changed, we need to move its definition to the proper cache
            // Note that we only consider the visibility changed when the lookup was made to /private because
            // the offer is confidential.
            if (currentDataSource == DataSourcePublic && def->dataSource() == DataSourcePrivateConfidential)
            {
                QFile::remove(definitionCacheFile(lookup->offerId()));
            }
            else
            {
                writeDefinitionCache(def);
            }

            m_writeUserCacheDataTimer.start();
        }
    }

    // Track unchanged CDN lookup count in heartbeat.
    Services::Heartbeat::instance()->catalogLookupUnchanged();
    ++m_lookupUnchangedCount;
}


void CatalogDefinitionController::onLookupFailed()
{
    using Origin::Services::Publishing::CatalogDefinitionLookup;
    
    // Track failed CDN lookup count in heartbeat.
    Services::Heartbeat::instance()->catalogLookupFailed();
    ++m_lookupFailureCount;
}


void CatalogDefinitionController::onLookupConfidential()
{
    using Origin::Services::Publishing::CatalogDefinitionLookup;
    
    // Not tracking confidential CDN lookup count in heartbeat because it's redundant with EC2 zabbix graphs.
    ++m_lookupConfidentialCount;
}


void CatalogDefinitionController::onLookupComplete()
{
    Origin::Services::Publishing::CatalogDefinitionLookup* lookup = dynamic_cast<Origin::Services::Publishing::CatalogDefinitionLookup*>(sender());

    m_pendingLookups.remove(lookup->offerId());
    if (m_refreshRequested && !refreshInProgress())
    {
        m_refreshRequested = false;
        emit refreshComplete();
    }

    lookup->deleteLater();
}

void CatalogDefinitionController::refreshDefinitions(RefreshType refreshType)
{
    m_refreshRequested = true;

    {
        QMutexLocker lock(&m_catalogDefinitionLock);
        foreach(Services::Publishing::CatalogDefinitionRef def, m_catalogDefinitions.values())
        {
            if (refreshType == RefreshAll || (refreshType == RefreshStale && def->isCdnDefinitionStale()))
            {
                const bool elevatedPermissions = def->dataSource() == Services::Publishing::DataSourcePrivatePermissions;
                lookupCatalogDefinition(def->productId(), def->qualifyingOfferId(), def->eTag(), def->batchTime(), elevatedPermissions)->start();
            }
        }
    }

    // Just in case we are not actually refreshing anything.
    if (m_refreshRequested && !refreshInProgress())
    {
        m_refreshRequested = false;
        emit refreshComplete();
    }
}

bool CatalogDefinitionController::refreshInProgress() const
{
    return m_refreshRequested && !m_pendingLookups.isEmpty();
}

void CatalogDefinitionController::onRefreshDefinitionTimerFired()
{
    refreshDefinitions(RefreshStale);
}

void CatalogDefinitionController::onDirtyBitsTelemetryDelayTimeout()
{
    ORIGIN_LOG_DEBUG << "Sending " << m_catalogUpdatesTelemetryData.size() << " delayed dirty bits catalog update telemetry events.";

    for (auto update = m_catalogUpdatesTelemetryData.begin(); update != m_catalogUpdatesTelemetryData.end(); ++update)
    {
        switch (update->updateType)
        {
        case DirtyBitsCatalogUpdate::MasterTitleUpdate:
            GetTelemetryInterface()->Metric_ENTITLEMENT_CATALOG_MASTERTITLE_UPDATE_DETECTED(
                update->id.toLocal8Bit().constData(), update->changeBatchTimestamp);
            break;

        case DirtyBitsCatalogUpdate::OfferDefinitionUpdate:
            GetTelemetryInterface()->Metric_ENTITLEMENT_CATALOG_OFFER_UPDATE_DETECTED(
                update->id.toLocal8Bit().constData(), update->changeBatchTimestamp);
            break;
        }
    }

    m_catalogUpdatesTelemetryData.clear();
}

void CatalogDefinitionController::onCatalogDefinitionLookupTelemetryTimeout()
{
    GetTelemetryInterface()->Metric_CATALOG_DEFINTION_CDN_LOOKUP_STATS(m_lookupSuccessCount, m_lookupFailureCount, m_lookupConfidentialCount, m_lookupUnchangedCount);

    m_lookupSuccessCount = 0;
    m_lookupFailureCount = 0;
    m_lookupConfidentialCount = 0;
    m_lookupUnchangedCount = 0;
}

void CatalogDefinitionController::onEndSessionComplete(Origin::Services::Session::SessionError error)
{
    if (m_dirtyBitsTelemetryDelayTimer.isActive())
    {
        m_dirtyBitsTelemetryDelayTimer.stop();
        onDirtyBitsTelemetryDelayTimeout();
    }
    
    if (m_catalogDefLookupTelemetryTimer.isActive())
    {
        m_catalogDefLookupTelemetryTimer.stop();
        onCatalogDefinitionLookupTelemetryTimeout();
    }

    QMutexLocker lock(&m_catalogDefinitionLock);
    m_catalogDefinitions.clear();
}

void CatalogDefinitionController::serializeCacheData(QDataStream &cacheContentStream)
{
    QMutexLocker lock(&m_catalogDefinitionLock);
    foreach (const Services::Publishing::CatalogDefinitionRef &def, m_catalogDefinitions)
    {
        if (def != Services::Publishing::CatalogDefinition::UNDEFINED && def->dataSource() != Services::Publishing::DataSourcePublic)
        {
            if (!Services::Publishing::CatalogDefinition::serialize(cacheContentStream, def))
            {
                ORIGIN_LOG_ERROR << "Failed to persist offer to cache [" << def->productId() << "]";
            }
        }
    }
}

void CatalogDefinitionController::deserializeCacheData(QDataStream &cacheContentStream)
{
    QMutexLocker lock(&m_catalogDefinitionLock);
    while (!cacheContentStream.atEnd())
    {
        Services::Publishing::CatalogDefinitionRef def;

        // Load the cache data with a data source of "PrivatePermissions" to avoid wiping out any public cache.
        if (Services::Publishing::CatalogDefinition::deserialize(cacheContentStream, def, Services::Publishing::DataSourcePrivatePermissions))
        {
            if (!m_catalogDefinitions.contains(def->productId()))
            {
                m_catalogDefinitions[def->productId()] = def;
            }
        }
    }
}

QString CatalogDefinitionController::definitionCacheFile(const QString& productId)
{
    const QString &fileNameSafeId = QUrl::toPercentEncoding(productId);
    return QString("%1%2%3.%4").arg(m_definitionCacheFolder, QDir::separator(), fileNameSafeId, CatalogDefinitionExtension);
}

void CatalogDefinitionController::writeDefinitionCache(const Origin::Services::Publishing::CatalogDefinitionRef &def)
{
    QFile serializationFile(definitionCacheFile(def->productId()));
    serializationFile.open(QIODevice::WriteOnly);

    QDataStream stream(&serializationFile);
    stream.setVersion(CatalogSerializationDataStreamVersion);

    stream << CatalogSerializationMagic;
    stream << CatalogSerializationVersion;

    Services::Publishing::CatalogDefinition::serialize(stream, def);
}

Services::Publishing::CatalogDefinitionLookup *CatalogDefinitionController::lookupCatalogDefinition(
    const QString& offerId, const QString &qualifyingOfferId, const QByteArray &eTag, qint64 batchTime, bool elevatedPermissions)
{
    Services::Publishing::CatalogDefinitionLookup *lookup;
    lookup = m_catalogDefinitionService.lookupCatalogDefinition(offerId, qualifyingOfferId, eTag, batchTime, elevatedPermissions);
    m_pendingLookups.insert(offerId);

    // Subscribe to succeeded to update our reference
    ORIGIN_VERIFY_CONNECT(lookup, &Services::Publishing::CatalogDefinitionLookup::succeeded,
        this, &CatalogDefinitionController::onLookupSucceeded);
    ORIGIN_VERIFY_CONNECT(lookup, &Services::Publishing::CatalogDefinitionLookup::unchanged,
        this, &CatalogDefinitionController::onLookupUnchanged);
    ORIGIN_VERIFY_CONNECT(lookup, &Services::Publishing::CatalogDefinitionLookup::failed,
        this, &CatalogDefinitionController::onLookupFailed);
    ORIGIN_VERIFY_CONNECT(lookup, &Services::Publishing::CatalogDefinitionLookup::confidential,
        this, &CatalogDefinitionController::onLookupConfidential);
    ORIGIN_VERIFY_CONNECT(lookup, &Services::Publishing::CatalogDefinitionLookup::finished,
        this, &CatalogDefinitionController::onLookupComplete);

    return lookup;
}

Origin::Services::Publishing::CatalogDefinitionRef CatalogDefinitionController::readDefinitionCache(const QString& offerId)
{
    const QString& cacheFilePath = definitionCacheFile(offerId);

    QFile serializationFile(cacheFilePath);

    if(serializationFile.exists() && serializationFile.open(QIODevice::ReadOnly))
    {
        QDataStream stream(&serializationFile);
        stream.setVersion(CatalogSerializationDataStreamVersion);

        unsigned int magic;
        unsigned int version;
        stream >> magic;
        stream >> version;

        if (magic == CatalogSerializationMagic &&
            version == CatalogSerializationVersion &&
            stream.status() == QDataStream::Ok && !stream.atEnd())
        {
            Services::Publishing::CatalogDefinitionRef cachedDef;

            if (Services::Publishing::CatalogDefinition::deserialize(stream, cachedDef, Services::Publishing::DataSourcePublic) && !cachedDef.isNull())
            {
                QMutexLocker lock(&m_catalogDefinitionLock);
                m_catalogDefinitions.insert(cachedDef->productId(), cachedDef);
                return cachedDef;
            }
        }

        ORIGIN_LOG_ERROR << "Corrupt catalog cache file for [" << offerId << "], removing...";
        serializationFile.close();
        serializationFile.remove();
    }

    return Services::Publishing::CatalogDefinition::UNDEFINED;
}

} // namespace Content

} // namespace Engine

} // namespace Origin
