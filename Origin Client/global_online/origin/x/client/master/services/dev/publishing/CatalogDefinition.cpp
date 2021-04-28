
//    Copyright (c) 2011-2012, Electronic Arts
//    All rights reserved.
//    Author: Hans van Veenendaal


#include "CatalogDefinition.h"

#include <limits>
#include <QDateTime>
#include <QJsonObject>

#include "services/config/OriginConfigService.h"
#include "services/debug/DebugService.h"
#include "services/publishing/ConversionUtil.h"
#include "services/log/LogService.h"
#include "services/platform/TrustedClock.h"
#include "services/publishing/SignatureVerifier.h"

#include "SettingsManager.h"
#include "TelemetryAPIDLL.h"

namespace Origin
{

namespace Services
{

namespace Publishing
{

// NULL dates
const QDateTime CatalogDefinition::NULL_QDATE_TIME_FUTURE = QDateTime::fromString(QDATESTRING_FUTURE_DEFAULTVALUE, QDATESTRING_FORMAT);

const CatalogDefinitionRef CatalogDefinition::UNDEFINED;

IMPLEMENT_PROPERTY_CONTAINER(SoftwareBuildData)
{
    setLiveDate(CatalogDefinition::NULL_QDATE_TIME_FUTURE);
}

QDataStream &operator << (QDataStream &out, const SoftwareServerData &serverData)
{
    return out << serverData.properties();
}

QDataStream &operator >> (QDataStream &in, SoftwareServerData &serverData)
{
    return in >> serverData.properties();
}

IMPLEMENT_PROPERTY_CONTAINER(SoftwareServerData)
{
    setUseEndDate(CatalogDefinition::NULL_QDATE_TIME_FUTURE);
    setDownloadStartDate(CatalogDefinition::NULL_QDATE_TIME_FUTURE);
    setReleaseDate(CatalogDefinition::NULL_QDATE_TIME_FUTURE);
    setOriginSubscriptionUseEndDate(CatalogDefinition::NULL_QDATE_TIME_FUTURE);
}

const SoftwareBuildMap& SoftwareServerData::softwareBuildMap() const
{
    return m_softwareBuildMap;
}

void SoftwareServerData::setSoftwareBuildMap(const SoftwareBuildMap& newMap) 
{
    m_softwareBuildMap = newMap;
}

OfferPricingData::OfferPricingData()
    : isPurchasable(false)
    , isFreeProduct(false)
    , blockUnderageUsers(false)
{
}

bool OfferPricingData::operator == (const OfferPricingData &other) const
{
    return isPurchasable == other.isPurchasable &&
        isFreeProduct == other.isFreeProduct &&
        blockUnderageUsers == other.blockUnderageUsers &&
        currentPrice == other.currentPrice &&
        originalPrice == other.originalPrice &&
        priceDescription == other.priceDescription;
}

bool OfferPricingData::operator != (const OfferPricingData &other) const
{
    return !(*this == other);
}

IMPLEMENT_PROPERTY_CONTAINER(CatalogDefinition)
{
    // Create a default platform server data for each platform
    m_softwareServerDataMap.insert(Origin::Services::PlatformService::PlatformWindows, SoftwareServerData());
    m_softwareServerDataMap.insert(Origin::Services::PlatformService::PlatformMacOS, SoftwareServerData());
}

void CatalogDefinition::updateDefinition(CatalogDefinitionRef def)
{
    if(def.data() != NULL && def.data() != this)
    {
        setproperties(*def);
        setSoftwareServerDataMap(def->softwareServerDataMap());
        emit definitionUpdated();
    }
}

const SoftwareServerDataMap& CatalogDefinition::softwareServerDataMap() const
{
    QReadLocker lock(&m_softwareServerDataLock);
    return m_softwareServerDataMap;
}

void CatalogDefinition::setSoftwareServerDataMap(const SoftwareServerDataMap& newMap) 
{
    QWriteLocker lock(&m_softwareServerDataLock);
    m_softwareServerDataMap = newMap;
}

SoftwareServerDataMap& CatalogDefinition::softwareServerDataMap() 
{
    QReadLocker lock(&m_softwareServerDataLock);
    return m_softwareServerDataMap;
}

SoftwareServerData& CatalogDefinition::softwareServerData(Origin::Services::PlatformService::OriginPlatform platform)
{
    QReadLocker lock(&m_softwareServerDataLock);
    return m_softwareServerDataMap[resolvePlatform(platform)];
}

const SoftwareServerData& CatalogDefinition::softwareServerData(Origin::Services::PlatformService::OriginPlatform platform) const
{
    platform = resolvePlatform(platform);

    QReadLocker lock(&m_softwareServerDataLock);
    ORIGIN_ASSERT(m_softwareServerDataMap.contains(platform));
    return m_softwareServerDataMap.constFind(platform).value();
}

QSet<Origin::Services::PlatformService::OriginPlatform> CatalogDefinition::platformsSupported() const
{
    QReadLocker lock(&m_softwareServerDataLock);

    QSet<Origin::Services::PlatformService::OriginPlatform> supported;

    foreach(Origin::Services::PlatformService::OriginPlatform platform, m_softwareServerDataMap.keys())
    {
        if(m_softwareServerDataMap[platform].platformEnabled())
        {
            supported.insert(platform);
        }
    }

    return supported;
}

bool CatalogDefinition::isBuildVersionStale() const
{
    // Check to see if staged build has become live...
    QString stagedBuildId = softwareServerData(PlatformService::PlatformThis).stagedBuildId();
    if(!stagedBuildId.isEmpty())
    {
        const Origin::Services::Publishing::SoftwareBuildData& stagedBuildInfo = softwareServerData(PlatformService::PlatformThis).softwareBuildMap()[stagedBuildId];

        if(stagedBuildInfo.liveDate().isValid())
        {
            qint64 timeTilReleaseChanges = Origin::Services::TrustedClock::instance()->nowUTC().msecsTo(stagedBuildInfo.liveDate());
            return (timeTilReleaseChanges <= 0);
        }
    }

    return false;
}

bool CatalogDefinition::isCdnDefinitionStale() const
{
    int maxAgeDays = Services::OriginConfigService::instance()->ecommerceConfig().catalogDefinitionMaxAgeDays;
    const QString &currentLocale = Services::readSetting(Services::SETTING_LOCALE);

    return ((maxAgeDays > 0 && refreshedFromCdnDate().daysTo(QDateTime::currentDateTimeUtc()) >= maxAgeDays) ||
            isBuildVersionStale() || locale() != currentLocale);
}

bool CatalogDefinition::canBeExtraContent() const
{
    return (originDisplayType() == Origin::Services::Publishing::OriginDisplayTypeFullGamePlusExpansion ||
            originDisplayType() == Origin::Services::Publishing::OriginDisplayTypeAddon ||
            originDisplayType() == Origin::Services::Publishing::OriginDisplayTypeExpansion);
}

bool CatalogDefinition::canBeExpansion() const
{
    return (originDisplayType() == Origin::Services::Publishing::OriginDisplayTypeFullGamePlusExpansion ||
            originDisplayType() == Origin::Services::Publishing::OriginDisplayTypeExpansion);
}

bool CatalogDefinition::isBaseGame() const
{
    return (originDisplayType() == Origin::Services::Publishing::OriginDisplayTypeFullGame ||
            originDisplayType() == Origin::Services::Publishing::OriginDisplayTypeFullGamePlusExpansion);
}

bool CatalogDefinition::isPDLC() const
{
    return (downloadable() && 
            (originDisplayType() == Origin::Services::Publishing::OriginDisplayTypeAddon ||
             originDisplayType() == Origin::Services::Publishing::OriginDisplayTypeExpansion));
}
            
bool CatalogDefinition::isPULC() const
{
    return (!downloadable() && 
            (originDisplayType() == Origin::Services::Publishing::OriginDisplayTypeAddon ||
             originDisplayType() == Origin::Services::Publishing::OriginDisplayTypeExpansion));
}

bool CatalogDefinition::newlyPublished() const
{
    // considered new only if less than 4 weeks (28 days) old
    return (publishedDate() >= QDateTime::currentDateTime().addDays(-28) &&
            publishedDate() <= QDateTime::currentDateTime());
}

bool CatalogDefinition::isFreeTrial() const
{
    switch (itemSubType())
    {
    case Publishing::ItemSubTypeFreeWeekendTrial:
    case Publishing::ItemSubTypeLimitedWeekendTrial:
    case Publishing::ItemSubTypeLimitedTrial:
        // We get a lot of things for free by saying timed trial is a free trial.
        // In the UI (only place that it is different) we will check for timed trial before free trial to get around this complication.
    case Publishing::ItemSubTypeTimedTrial_GameTime:
    case Publishing::ItemSubTypeTimedTrial_Access:
        return true;
    default:
        return false;
    }
}

bool CatalogDefinition::isBrowserGame() const
{
    return (softwareServerData(Origin::Services::PlatformService::runningPlatform()).packageType() == Origin::Services::Publishing::PackageTypeExternalUrl);
}

bool CatalogDefinition::isAddonOrBonusContent() const
{
    return softwareServerData(Origin::Services::PlatformService::runningPlatform()).installCheck().isEmpty() &&
        softwareServerData(Origin::Services::PlatformService::runningPlatform()).executePath().isEmpty();
}

bool CatalogDefinition::isPreload() const
{
    QDateTime downloadStartDate = softwareServerData(Origin::Services::PlatformService::runningPlatform()).downloadStartDate();

    // We are only preload if past download start date but not yet released
    return (downloadable() && downloadStartDate.isValid() && (downloadStartDate <= TrustedClock::instance()->nowUTC()) && isUnreleased());
}

bool CatalogDefinition::isReleased() const
{
    QDateTime releaseDate = softwareServerData(Origin::Services::PlatformService::runningPlatform()).releaseDate();
    return (releaseDate.isValid() && (releaseDate <= TrustedClock::instance()->nowUTC()));
}

bool CatalogDefinition::isUnreleased() const
{
    return !isReleased();
}

bool CatalogDefinition::isDownloadExpired() const
{
    QDateTime useEndDate = softwareServerData(Origin::Services::PlatformService::runningPlatform()).useEndDate();
    return (useEndDate.isValid() && (useEndDate <= TrustedClock::instance()->nowUTC()));
}

QStringList CatalogDefinition::relatedGameContentIds() const
{
    QStringList relatedGameIds(contentId());
    if(cloudSaveContentIDFallback().size())
    {
        relatedGameIds += cloudSaveContentIDFallback();
    }
    return relatedGameIds;
}

Variant CatalogDefinition::getProductOverrideValue(const QString& overrideSettingBaseName) const
{
    Variant setting = readSetting(QString("%1::%2").arg(overrideSettingBaseName, productId()));

    if(setting.isNull())
    {
        setting = readSetting(QString("%1::%2").arg(overrideSettingBaseName, contentId())); 
    }

    return setting;
}

void CatalogDefinition::processPublishingOverrides()
{
    //handle overrides
    SoftwareServerData& platformDataMap = softwareServerData(PlatformService::PlatformThis);

    platformDataMap.clearOverrides();

    Variant overrideMonitoredInstallSetting = getProductOverrideValue(SETTING_OverrideMonitoredInstall.name());
    if(!overrideMonitoredInstallSetting.isNull())
    {
        platformDataMap.setMonitorInstallOverride(overrideMonitoredInstallSetting.toQVariant().toBool());
    }

    Variant overrideMonitoredPlaySetting = getProductOverrideValue(SETTING_OverrideMonitoredPlay.name());
    if(!overrideMonitoredPlaySetting.isNull())
    {
        platformDataMap.setMonitorPlayOverride(overrideMonitoredPlaySetting.toQVariant().toBool());
    }

    Variant overrideExternalInstallDialogSetting = getProductOverrideValue(SETTING_OverrideExternalInstallDialog.name());
    if(!overrideExternalInstallDialogSetting.isNull())
    {
        platformDataMap.setShowKeyDialogOnInstallOverride(overrideExternalInstallDialogSetting.toQVariant().toBool());
    }

    Variant overrideExternalPlayDialogSetting = getProductOverrideValue(SETTING_OverrideExternalPlayDialog.name());
    if(!overrideExternalPlayDialogSetting.isNull())
    {
        platformDataMap.setShowKeyDialogOnPlayOverride(overrideExternalPlayDialogSetting.toQVariant().toBool());
    }

    const QString &overridePartnerPlatformSetting = getProductOverrideValue(SETTING_OverridePartnerPlatform.name()).toString().trimmed().toLower();
    if (!overridePartnerPlatformSetting.isEmpty())
    {
        if (overridePartnerPlatformSetting == "steam")
        {
            platformDataMap.setPartnerPlatformOverride(Publishing::PartnerPlatformSteam);
        }
        else if (overridePartnerPlatformSetting == "gfwl")
        {
            platformDataMap.setPartnerPlatformOverride(Publishing::PartnerPlatformGamesForWindows);
        }
        else if (overridePartnerPlatformSetting.length() > 0)
        {
            platformDataMap.setPartnerPlatformOverride(Publishing::PartnerPlatformOtherExternal);
        }
    }

    const QString &overrideGameLauncherUrl = getProductOverrideValue(SETTING_OverrideGameLauncherURL.name()).toString();
    if (!overrideGameLauncherUrl.isEmpty())
    {
        platformDataMap.setGameLauncherUrlOverride(overrideGameLauncherUrl);
    }

    const QString &overrideGameLauncherSoftwareId = getProductOverrideValue(SETTING_OverrideGameLauncherSoftwareId.name()).toString();
    if (!overrideGameLauncherSoftwareId.isEmpty())
    {
        platformDataMap.setAlternateLaunchSoftwareId(overrideGameLauncherSoftwareId);
    }

    const QString &overrideGameLauncherUrlClientId = getProductOverrideValue(SETTING_OverrideGameLauncherURLIdentityClientId.name()).toString();
    if (!overrideGameLauncherUrlClientId.isEmpty())
    {
        platformDataMap.setGameLauncherUrlClientIdOverride(overrideGameLauncherUrlClientId);
    }

    Variant overrideUseLegacyCatalog = getProductOverrideValue(SETTING_OverrideUseLegacyCatalog.name());
    if(!overrideUseLegacyCatalog.isNull())
    {
        setUseLegacyCatalogOverride(overrideUseLegacyCatalog.toQVariant().toBool());
    }

    const QString &overrideCommerceProfile = getProductOverrideValue(SETTING_OverrideCommerceProfile.name()).toString();
    if (!overrideCommerceProfile.isEmpty())
    {
        platformDataMap.setCommerceProfileOverride(overrideCommerceProfile);
    }

    Variant overrideForceKilledAtOwnershipExpiry = getProductOverrideValue(SETTING_OverrideIsForceKilledAtOwnershipExpiry.name());
    if(!overrideForceKilledAtOwnershipExpiry.isNull())
    {
        setForceKillAtOwnershipExpiryOverride(overrideForceKilledAtOwnershipExpiry.toQVariant().toBool());
    }

    const QString &associationBundleOverride = getProductOverrideValue(SETTING_OverrideBundleContents.name()).toString();
    if (!associationBundleOverride.isEmpty())
    {
        setAssociatedProductIdsOverride(associationBundleOverride.split(","));
    }

    // Only override Free Trial and Price related settings in non-Production environments
    const QString &environment = readSetting(SETTING_EnvironmentName).toString().toLower();
    if (environment != SETTING_ENV_production)
    {
        const QString &overrideItemSubTypeSetting = getProductOverrideValue(SETTING_OverrideItemSubType.name()).toString();
        if (!overrideItemSubTypeSetting.isEmpty())
        {
            Publishing::ItemSubType itemSubType = Services::Publishing::ConversionUtil::ConvertGameDistributionSubType(productId(), overrideItemSubTypeSetting);
#ifdef DISABLE_FREE_TRIALS_FOR_NON_PC
            bool isFreeTrial = false;
            switch (itemSubType)
            {
            case Publishing::ItemSubTypeLimitedWeekendTrial:
            case Publishing::ItemSubTypeFreeWeekendTrial:
            case Publishing::ItemSubTypeLimitedTrial:
            case Publishing::ItemSubTypeTimedTrial_GameTime:
            case Publishing::ItemSubTypeTimedTrial_Access:
                isFreeTrial = true;
                break;
            default:
                break;
            }
            if (isFreeTrial)
                itemSubType = Publishing::ItemSubTypeNormalGame;
#endif

            setItemSubTypeOverride(itemSubType);
        }

        Variant overrideTrialDuration = getProductOverrideValue(SETTING_OverrideTrialDuration.name());
        if(!overrideTrialDuration.isNull())
        {
            setTrialDurationOverride(overrideTrialDuration.toQVariant().toInt());
        }

        const QString &overrideCloudSaveContentIDFallbackSetting = getProductOverrideValue(SETTING_OverrideCloudSaveContentIDFallback.name()).toString();
        if (!overrideCloudSaveContentIDFallbackSetting.isEmpty())
        {
            setCloudSaveContentIDFallbackOverride(overrideCloudSaveContentIDFallbackSetting.split(","));
        }

        const QString &overrideGameLauncherUrlIntegration = getProductOverrideValue(SETTING_OverrideGameLauncherURLIntegration.name()).toString();
        if (!overrideGameLauncherUrlIntegration.isEmpty())
        {
            platformDataMap.setGameLauncherUrlOverride(overrideGameLauncherUrlIntegration);
        }
    }
}

CatalogDefinitionRef CatalogDefinition::createEmptyDefinition()
{
    CatalogDefinitionRef def(new CatalogDefinition());
    return def;
}

CatalogDefinitionRef CatalogDefinition::parseJsonData(const QString& offerId, const QByteArray& jsonResponseData, const bool fromCache, const DataSource dataSource)
{
    TIME_BEGIN("CatalogDefinition::parseJsonData")

    const bool isPublicOffer = dataSource == DataSourcePublic;

    QJsonParseError jsonResult;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonResponseData, &jsonResult);

    //check if its a valid JSON response
    if(jsonResult.error != QJsonParseError::NoError)
    {
        ORIGIN_LOG_ERROR << "Encountered json error while parsing catalog definition: " << jsonResult.error << "; Desc: " << jsonResult.errorString();
        GetTelemetryInterface()->Metric_CATALOG_DEFINTION_PARSE_ERROR(offerId.toLatin1(), "Json parse error", fromCache, isPublicOffer, jsonResult.error);
        TIME_END("CatalogDefinition::parseJsonData")
        return CatalogDefinition::UNDEFINED;
    }

    QJsonObject jsonObj = jsonDoc.object();

    if (!jsonObj.contains("offerId"))
    {
        ORIGIN_LOG_ERROR << "Json did not contain offerId node";
        GetTelemetryInterface()->Metric_CATALOG_DEFINTION_PARSE_ERROR(offerId.toLatin1(), "Missing offerId node", fromCache, isPublicOffer);
        TIME_END("CatalogDefinition::parseJsonData")
        return CatalogDefinition::UNDEFINED;
    }

    CatalogDefinitionRef def(new CatalogDefinition());
   
    if(!Origin::Services::Publishing::ConversionUtil::ConvertOffer(jsonObj, def))
    {
        ORIGIN_LOG_ERROR << "Failed to parse offer definition JSON from CDN for offer [" << offerId << "]";
        GetTelemetryInterface()->Metric_CATALOG_DEFINTION_PARSE_ERROR(offerId.toLatin1(), "Conversion failed", fromCache, isPublicOffer);
        TIME_END("CatalogDefinition::parseJsonData")
        return CatalogDefinition::UNDEFINED;
    }

    def->setJsonData(qCompress(jsonResponseData));

#if LOG_CATALOG_DEFINITIONS
    def->logCatalogDef();
#endif

    TIME_END("CatalogDefinition::parseJsonData")

    return def;
}

bool CatalogDefinition::serialize(QDataStream& outputStream, CatalogDefinitionRef def)
{
    if (def && def != CatalogDefinition::UNDEFINED)
    {
        ORIGIN_ASSERT(!def->jsonData().isEmpty());

        outputStream << def->productId();
        outputStream << def->eTag();
        outputStream << def->signature();
        outputStream << def->locale();
        outputStream << def->refreshedFromCdnDate();

        // Only persist qualifyingOfferId if we're persisting to the private cache.
        const QString qualifyingOfferId = (def->dataSource() != DataSourcePublic) ? def->qualifyingOfferId() : QString();
        outputStream << qualifyingOfferId;

        outputStream << def->jsonData();

        return outputStream.status() == QDataStream::Ok;
    }
    else
    {
        return false;
    }
}

bool CatalogDefinition::deserialize(QDataStream& inputStream, CatalogDefinitionRef &ref, DataSource dataSource)
{
    TIME_BEGIN("CatalogDefinition::deserialize")

    if (!inputStream.atEnd())
    {
        CatalogDefinitionRef def;
        QString offerId;
        QByteArray eTag;
        QByteArray signature;
        QString locale;
        QDateTime refreshedFromCdnDate;
        QString qualifyingOfferId;
        QByteArray jsonData;

        inputStream >> offerId;
        inputStream >> eTag;
        inputStream >> signature;
        inputStream >> locale;
        inputStream >> refreshedFromCdnDate;
        inputStream >> qualifyingOfferId;
        inputStream >> jsonData;

        // EBIBUGS-29097: Avoid loading this cached data if it is in a different language.
        if (locale != Services::readSetting(Services::SETTING_LOCALE).toString())
        {
            TIME_END("CatalogDefinition::deserialize")
            return false;
        }

        jsonData = qUncompress(jsonData);

        const QByteArray &signatureKey = (dataSource == DataSourcePublic ? offerId : PlatformService::machineHashAsString()).toUtf8();
        if (Services::Publishing::SignatureVerifier::verify(signatureKey, jsonData, signature))
        {
            def = Services::Publishing::CatalogDefinition::parseJsonData(offerId, jsonData, true, dataSource);
        }

        if (!def.isNull())
        {
            ORIGIN_ASSERT(def->productId() == offerId);

            def->setDataSource(dataSource);
            def->setETag(eTag);
            def->setSignature(signature);
            def->setLocale(locale);
            def->setRefreshedFromCdnDate(refreshedFromCdnDate);
            def->setQualifyingOfferId(qualifyingOfferId);

            ref = def;

            TIME_END("CatalogDefinition::deserialize")

            return true;
        }
    }

    TIME_END("CatalogDefinition::deserialize")

    return false;
}

#if LOG_CATALOG_DEFINITIONS
void CatalogDefinition::logCatalogDef()
{
    for (auto iter = properties().constBegin(); iter != properties().constEnd(); ++iter) 
    {
        ORIGIN_LOG_DEBUG << iter.key() << ":" << iter.value().toString();
    }

    foreach (const Services::Publishing::SoftwareServerData& softwareData, softwareServerDataMap().values())
    {
        if (!softwareData.platformEnabled())
        {
            ORIGIN_LOG_DEBUG << "  No software enabled for " << Services::PlatformService::stringForPlatformId(softwareData.platformId()) << ".";
            continue;
        }

        ORIGIN_LOG_DEBUG << "  Software enabled for " << Services::PlatformService::stringForPlatformId(softwareData.platformId()) << ":";

        for (auto iter = softwareData.properties().constBegin(); iter != softwareData.properties().constEnd(); iter++)
        {
            ORIGIN_LOG_DEBUG << "    " << iter.key() << ":" << iter.value().toString();
        }

        foreach (const Services::Publishing::SoftwareBuildData& buildData, softwareData.softwareBuildMap().values())
        {
            ORIGIN_LOG_DEBUG << "    Software Build Data for ID " << buildData.buildId() << ":";

            for (auto iter = buildData.properties().constBegin(); iter != buildData.properties().constEnd(); iter++)
            {
                ORIGIN_LOG_DEBUG << "      " << iter.key() << ":" << iter.value().toString();
            }

            if (buildData.buildMetadataPresent())
            {
                ORIGIN_LOG_DEBUG << "      Software BuildMetaData:";
                ORIGIN_LOG_DEBUG << "        gameVersion:" << buildData.buildMetadata().mGameVersion.ToStr();
                ORIGIN_LOG_DEBUG << "        minimumRequiredOS:" << buildData.buildMetadata().mMinimumRequiredOS.ToStr();
                ORIGIN_LOG_DEBUG << "        requiredClientVersion:" << buildData.buildMetadata().mRequiredClientVersion.ToStr();
                ORIGIN_LOG_DEBUG << "        autoUpdateEnabled:" << buildData.buildMetadata().mbAutoUpdateEnabled;
                ORIGIN_LOG_DEBUG << "        ignoresEnabled:" << buildData.buildMetadata().mbIgnoresEnabled;
                ORIGIN_LOG_DEBUG << "        deletesEnabled:" << buildData.buildMetadata().mbDeletesEnabled;
                ORIGIN_LOG_DEBUG << "        consolidatedEULAsEnabled:" << buildData.buildMetadata().mbConsolidatedEULAsEnabled;
                ORIGIN_LOG_DEBUG << "        shortcutsEnabled:" << buildData.buildMetadata().mbShortcutsEnabled;
                ORIGIN_LOG_DEBUG << "        languageExcludesEnabled:" << buildData.buildMetadata().mbLanguageExcludesEnabled;
                ORIGIN_LOG_DEBUG << "        languageIncludesEnabled:" << buildData.buildMetadata().mbLanguageIncludesEnabled;
                ORIGIN_LOG_DEBUG << "        useGameVersionFromManifestEnabled:" << buildData.buildMetadata().mbUseGameVersionFromManifestEnabled;
                ORIGIN_LOG_DEBUG << "        forceTouchupInstallerAfterUpdate:" << buildData.buildMetadata().mbForceTouchupInstallerAfterUpdate;
                ORIGIN_LOG_DEBUG << "        executeGameElevated:" << buildData.buildMetadata().mbExecuteGameElevated;
                ORIGIN_LOG_DEBUG << "        treatUpdatesAsMandatory:" << buildData.buildMetadata().mbTreatUpdatesAsMandatory;
                ORIGIN_LOG_DEBUG << "        allowMultipleInstances:" << buildData.buildMetadata().mbAllowMultipleInstances;
                ORIGIN_LOG_DEBUG << "        enableDifferentialUpdate:" << buildData.buildMetadata().mbEnableDifferentialUpdate;
                ORIGIN_LOG_DEBUG << "        dynamicContentSupportEnabled:" << buildData.buildMetadata().mbDynamicContentSupportEnabled;
                ORIGIN_LOG_DEBUG << "        gameRequires64BitOS:" << buildData.buildMetadata().mbGameRequires64BitOS;
                ORIGIN_LOG_DEBUG << "        languageChangeSupportEnabled:" << buildData.buildMetadata().mbLanguageChangeSupportEnabled;
                ORIGIN_LOG_DEBUG << "        DLCRequiresParentUpToDate:" << buildData.buildMetadata().mbDLCRequiresParentUpToDate;
                ORIGIN_LOG_DEBUG << "        autoRepairVersionMin:" << buildData.buildMetadata().mAutoRepairVersionMin.ToStr();
                ORIGIN_LOG_DEBUG << "        autoRepairVersionMax:" << buildData.buildMetadata().mAutoRepairVersionMax.ToStr();
                ORIGIN_LOG_DEBUG << "        minimumRequiredVersionForOnlineUse:" << buildData.buildMetadata().mMinimumRequiredVersionForOnlineUse.ToStr();
            }
        }
    }
}
#endif

} // namespace Publishing

} // namespace Services

} // namespace Origin
