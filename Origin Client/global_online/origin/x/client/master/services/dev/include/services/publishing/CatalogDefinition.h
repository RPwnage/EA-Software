//    Copyright © 2011-2012, Electronic Arts
//    All rights reserved.
//    Author: Hans van Veenendaal

#ifndef CATALOGDEFINITION_H
#define CATALOGDEFINITION_H

#include "services/common/Property.h"
#include "services/platform/PlatformService.h"
#include "services/publishing/SoftwareBuildMetadata.h"
#include "services/settings/Variant.h"
#include <QStringList>
#include <QSharedPointer>

#ifndef Q_OS_WIN
#define DISABLE_FREE_TRIALS_FOR_NON_PC 
#endif

#define LOG_CATALOG_DEFINITIONS 0

namespace Origin
{

namespace Services
{

namespace Publishing
{

const qint64 INVALID_BATCH_TIME = -1;

enum SoftwareBuildType
{
    SoftwareBuildLive,
    SoftwareBuildStaged,
    SoftwareBuildOther
};

class ORIGIN_PLUGIN_API SoftwareBuildData
{
    DECLARE_PROPERTY_CONTAINER(SoftwareBuildData)

    DECLARE_PROPERTY_STRING(buildReleaseVersion, "", setBuildReleaseVersion);
    DECLARE_PROPERTY_DATETIME(liveDate, QDATESTRING_FUTURE_DEFAULTVALUE, QDATESTRING_FORMAT, setLiveDate);
    DECLARE_PROPERTY_STRING(fileLink, "", setFileLink);
    DECLARE_PROPERTY_STRING(notes, "", setNotes);
    DECLARE_PROPERTY_ENUM(SoftwareBuildType, type, SoftwareBuildOther, setType);
    DECLARE_PROPERTY_BOOL(buildMetadataPresent, false, setBuildMetadataPresent);
    DECLARE_PROPERTY_STRING(buildId, "", setBuildId);

    SoftwareBuildMetadata& buildMetadata() { return m_buildMetadata; }
    const SoftwareBuildMetadata& buildMetadata() const { return m_buildMetadata; }

private:
    SoftwareBuildMetadata m_buildMetadata;
};

typedef QMap<QString, SoftwareBuildData> SoftwareBuildMap;

enum IGOPermission
{
    IGOPermissionBlacklisted, // Blacklisted games are configured to ensure that IGO is never enabled
    IGOPermissionOptIn, // Unsupported games will allow IGO to be turned on if user enables IGO and selects the "All Games" option
    IGOPermissionSupported    // Supported games are known to work with IGO and allow IGO to be used if user has IGO enabled at all
};

enum AddonDeploymentStrategy
{
    AddonDeploymentStrategyHotDeployable,
    AddonDeploymentStrategyReloadSave,
    AddonDeploymentStrategyRestartGame
};

enum PackageType
{
    PackageTypeDownloadInPlace,
    PackageTypeSingle,
    PackageTypeUnpacked,
    PackageTypeExternalUrl,
    PackageTypeOriginPlugin,
    PackageTypeUnknown
};

enum PartnerPlatformType
{
    PartnerPlatformNone,
    PartnerPlatformGamesForWindows,
    PartnerPlatformSteam,
    PartnerPlatformOtherExternal
};

class ORIGIN_PLUGIN_API SoftwareServerData
{
    DECLARE_PROPERTY_CONTAINER(SoftwareServerData)

    DECLARE_PROPERTY_ENUM(Services::PlatformService::OriginPlatform, platformId, Services::PlatformService::PlatformUnknown, setPlatformId)
    DECLARE_PROPERTY_BOOL(platformEnabled, false, setPlatformEnabled)
    DECLARE_PROPERTY_STRING(softwareId, "", setSoftwareId)

    // FulfillmentAttributes
    DECLARE_PROPERTY_STRING(achievementSet, "", setAchievementSet)
    DECLARE_PROPERTY_ENUM(AddonDeploymentStrategy, addonDeploymentStrategy, AddonDeploymentStrategyRestartGame, setAddonDeploymentStrategy)

    DECLARE_PROPERTY_ENUM(Services::PlatformService::OriginProcessorArchitecture, availableArchitectures, Services::PlatformService::ProcessorArchitecture3264bit, setAvailableArchitectures);

    // field set based on cloudSaveConfigurationOverride != ""
    DECLARE_PROPERTY_BOOL(isCloudSaveSupported, false, setCloudSaveSupported)
    DECLARE_PROPERTY_STRING(saveFileCriteria, "", setSaveFileCriteria)

    DECLARE_PROPERTY_STRING(commerceProfile, "oig-real", setCommerceProfile)
    DECLARE_PROPERTY_LONGLONG(downloadSize, 0, setDownloadSize)
    DECLARE_PROPERTY_ENUM(PackageType, packageType, PackageTypeUnknown, setPackageType)
    DECLARE_PROPERTY_STRING(executePath, "", setExecutePath)
    DECLARE_PROPERTY_STRING(executeParameters, "", setExecuteParameters)
    DECLARE_PROPERTY_STRING(installCheck, "", setInstallCheck)

    DECLARE_PROPERTY_STRING(installationDirectory, "", setInstallationDirectory)

    // SoftwareControlDates
    DECLARE_PROPERTY_DATETIME(downloadStartDate, QDATESTRING_FUTURE_DEFAULTVALUE, QDATESTRING_FORMAT, setDownloadStartDate)
    DECLARE_PROPERTY_DATETIME(releaseDate, QDATESTRING_FUTURE_DEFAULTVALUE, QDATESTRING_FORMAT, setReleaseDate)
    DECLARE_PROPERTY_DATETIME(useEndDate, QDATESTRING_FUTURE_DEFAULTVALUE, QDATESTRING_FORMAT, setUseEndDate)
    DECLARE_PROPERTY_DATETIME(originSubscriptionUseEndDate, QDATESTRING_FUTURE_DEFAULTVALUE, QDATESTRING_FORMAT, setOriginSubscriptionUseEndDate)

    DECLARE_PROPERTY_STRING(metadataInstallLocation, "", setMetadataInstallLocation);
    DECLARE_PROPERTY_STRING(InstallerPath, "", setInstallerPath)
    DECLARE_PROPERTY_STRING(installerParams, "", setInstallerParams)
    DECLARE_PROPERTY_ENUM(IGOPermission, igoPermission, IGOPermissionOptIn, setIgoPermission)
    DECLARE_PROPERTY_STRING(multiplayerId, "", setMultiplayerId)

    DECLARE_PROPERTY_STRING(stagingKeyPath, "", setStagingKeyPath)

    DECLARE_PROPERTY_BOOL(monitorInstall, true, setMonitorInstall)
    DECLARE_PROPERTY_BOOL(monitorPlay, true, setMonitorPlay)
    DECLARE_PROPERTY_ENUM(PartnerPlatformType, partnerPlatform, PartnerPlatformNone, setPartnerPlatform)
    DECLARE_PROPERTY_BOOL(showKeyDialogOnInstall, false, setShowKeyDialogOnInstall)
    DECLARE_PROPERTY_BOOL(showKeyDialogOnPlay, false, setShowKeyDialogOnPlay)
    DECLARE_PROPERTY_BOOL(enableDLCUninstall, false, setEnableDLCUninstall)

    DECLARE_PROPERTY_STRING(macBundleId, "", setMacBundleId);
    DECLARE_PROPERTY_BOOL(updateNonDipInstall, false, setUpdateNonDipInstall);
    DECLARE_PROPERTY_BOOL(netPromoterScoreBlacklist, false, setNetPromoterScoreBlacklist);

    DECLARE_PROPERTY_STRING(identityClientIdOverride, "", setIdentityClientIdOverride);
    DECLARE_PROPERTY_STRING(gameLauncherUrl, "", setGameLauncherUrl);
    DECLARE_PROPERTY_STRING(gameLauncherUrlClientId, "", setGameLauncherUrlClientId);
    DECLARE_PROPERTY_STRING(alternateLaunchSoftwareId, "", setAlternateLaunchSoftwareId);
    DECLARE_PROPERTY_BOOL(showSubsSaveGameWarning, false, setShowSubsSaveGameWarning);

    // This is a map of SoftwareBuildData above, keyed by live date
    // use liveBuildId to access the current release version
    // use stagedBuildId to access staged version info. 
    //
    // Iterate on map for all other info, which will only be populated if
    // user has ODT entitlement
    const SoftwareBuildMap& softwareBuildMap() const;
    void setSoftwareBuildMap(const SoftwareBuildMap& newBuildMap);

    DECLARE_PROPERTY_STRING(liveBuildVersion, "0", setLiveBuildVersion);
    DECLARE_PROPERTY_STRING(stagedBuildVersion, "", setStagedBuildVersion);

    DECLARE_PROPERTY_STRING(liveBuildId, "", setLiveBuildId);
    DECLARE_PROPERTY_STRING(stagedBuildId, "", setStagedBuildId);

private:
    SoftwareBuildMap m_softwareBuildMap;

    friend QDataStream &operator << (QDataStream &out, const SoftwareServerData &serverData);
    friend QDataStream &operator >> (QDataStream &in, SoftwareServerData &serverData);
};

typedef QMap<Services::PlatformService::OriginPlatform, SoftwareServerData> SoftwareServerDataMap;

QDataStream &operator << (QDataStream &out, const SoftwareServerData &serverData);
QDataStream &operator >> (QDataStream &in, SoftwareServerData &serverData);

enum ItemSubType
{
    ItemSubTypeNormalGame,
    ItemSubTypeAlpha,
    ItemSubTypeBeta,
    ItemSubTypeTimedTrial_GameTime,
    ItemSubTypeTimedTrial_Access,
    ItemSubTypeFreeWeekendTrial,
    ItemSubTypeLimitedTrial,
    ItemSubTypeLimitedWeekendTrial,
    ItemSubTypeNonOrigin,
    ItemSubTypeDemo,
    ItemSubTypeOnTheHouse,
    ItemSubTypeUnknown
};

/// \brief Location that the content should be shown.
enum OriginDisplayType
{
    OriginDisplayTypeNone,                   ///< Should not be shown at all.
    OriginDisplayTypeFullGame,              ///< Should be shown on the My Games page.
    OriginDisplayTypeExpansion,           ///< Should be shown in the Expansions section of the Details page.
    OriginDisplayTypeFullGamePlusExpansion, ///< Should appear both on the MyGames page and in the Expansions section.
    OriginDisplayTypeAddon,                 ///< Should be shown in the Addons table of the Details page.
    OriginDisplayTypeGameOnly,              ///< Only display in game.
};

enum DataSource
{
    DataSourcePublic,                   ///< Definition was retrieved via /public.
    DataSourcePrivateConfidential,      ///< Definition was retrieved via /private because the offer is confidential.
    DataSourcePrivatePermissions,       ///< Definition was retrieved via /private due to user permissions.
};

struct ORIGIN_PLUGIN_API OfferPricingData
{
    OfferPricingData();
    bool operator == (const OfferPricingData &other) const;
    bool operator != (const OfferPricingData &other) const;

    bool isPurchasable;
    bool isFreeProduct;
    bool blockUnderageUsers;
    QString currentPrice;
    QString originalPrice;
    QString priceDescription;
};

class CatalogDefinition;
typedef QSharedPointer<CatalogDefinition> CatalogDefinitionRef;

class ORIGIN_PLUGIN_API CatalogDefinition : public QObject
{
    Q_OBJECT

public:
    static const CatalogDefinitionRef UNDEFINED;
    static const QDateTime NULL_QDATE_TIME_FUTURE;

    void updateDefinition(CatalogDefinitionRef updatedDef);

public:
    DECLARE_PROPERTY_STRING(itemId, "", setItemId);
    DECLARE_PROPERTY_STRING(contentId, "", setContentId)
    DECLARE_PROPERTY_STRING(displayName, "", setDisplayName)
    DECLARE_PROPERTY_STRING(detailImage, "", setDetailImage)

    DECLARE_PROPERTY_STRING(manualUrl, "", setManualUrl)

    DECLARE_PROPERTY_DATETIME(publishedDate, QDATESTRING_FUTURE_DEFAULTVALUE, QDATESTRING_FORMAT, setPublishedDate)
    DECLARE_PROPERTY_BOOL(greyMarketControls, false, setGreyMarketControls)
    DECLARE_PROPERTY_STRING(cdnAssetRoot, "", setCdnAssetRoot)
    DECLARE_PROPERTY_STRING(financeId, "", setFinanceId)

    DECLARE_PROPERTY_ENUM(OriginDisplayType, originDisplayType, OriginDisplayTypeNone, setOriginDisplayType)
    DECLARE_PROPERTY_BOOL(downloadable, false, setDownloadable)
    DECLARE_PROPERTY_BOOL(published, true, setPublished)
    DECLARE_PROPERTY_BOOL(useLegacyCatalog, false, setUseLegacyCatalog)
    DECLARE_PROPERTY_BOOL(forceKillAtOwnershipExpiry, false, setForceKillAtOwnershipExpiry)
    DECLARE_PROPERTY_STRING(productId, "", setProductId)
    DECLARE_PROPERTY_STRINGLIST(parentProductIds, setParentProductIds)

    DECLARE_PROPERTY_BOOL(addonsAvailable, false, setAddonsAvailable)
    DECLARE_PROPERTY_STRINGLIST(availableExtraContent, setAvailableExtraContent)

    DECLARE_PROPERTY_LONGLONG(sortOrderDescending, -1, setSortOrderDescending)
    DECLARE_PROPERTY_STRING(extraContentDisplayGroup, "", setExtraContentDisplayGroup);
    DECLARE_PROPERTY_STRING(extraContentDisplayGroupDisplayName, "", setExtraContentDisplayGroupDisplayName);
    DECLARE_PROPERTY_INT(extraContentDisplayGroupSortOrderAscending, 0, setExtraContentDisplayGroupSortOrderAscending);
    DECLARE_PROPERTY_BOOL(suppressVaultUpgrade, false, setSuppressVaultUpgrade);

    DECLARE_PROPERTY_STRINGLIST(associatedProductIds, setAssociatedProductIds)

    DECLARE_PROPERTY_STRINGLIST(supportedLocales, setSupportedLocales)

    DECLARE_PROPERTY_STRING(IGOBrowserDefaultURL, "", setIGOBrowserDefaultURL)
    DECLARE_PROPERTY_ENUM(ItemSubType, itemSubType, ItemSubTypeNormalGame, setItemSubType)

    DECLARE_PROPERTY_STRINGLIST(cloudSaveContentIDFallback, setCloudSaveContentIDFallback)
    DECLARE_PROPERTY_INT(trialDuration, -1, setTrialDuration)
    DECLARE_PROPERTY_STRING(masterTitleId, "", setMasterTitleId)
    DECLARE_PROPERTY_STRING(masterTitle, "", setMasterTitle)
    DECLARE_PROPERTY_STRINGLIST(alternateMasterTitleIds, setAlternateMasterTitleIds)
    DECLARE_PROPERTY_STRING(franchiseId, "", setFranchiseId)
    DECLARE_PROPERTY_STRING(longDescription, "", setLongDescription);
    DECLARE_PROPERTY_STRING(shortDescription, "", setShortDescription);
    DECLARE_PROPERTY_STRING(gameLaunchMessage, "", setGameLaunchMessage);
    DECLARE_PROPERTY_STRING(productType, "", setProductType);
    DECLARE_PROPERTY_BOOL(previewContent, false, setPreviewContent);
    DECLARE_PROPERTY_BOOL(purchasable, false, setPurchasable);

    DECLARE_PROPERTY_BOOL(twitchClientBlacklist, false, setTwitchClientBlacklist);
    DECLARE_PROPERTY_BOOL(watermarkDownload, false, setWatermarkDownload);

    DECLARE_PROPERTY_STRING(originSdkImage, "", setOriginSdkImage);
    DECLARE_PROPERTY_STRING(backgroundImage, "", setBackgroundImage);
    DECLARE_PROPERTY_STRING(packArtSmall, "", setPackArtSmall);
    DECLARE_PROPERTY_STRING(packArtMedium, "", setPackArtMedium);
    DECLARE_PROPERTY_STRING(packArtLarge, "", setPackArtLarge);
    DECLARE_PROPERTY_STRING(preAnnouncementDisplayDate, "", setPreAnnouncementDisplayDate);
    DECLARE_PROPERTY_STRING(imageServer, "", setImageServer);
    DECLARE_PROPERTY_INT(rank, -1, setRank);

    DECLARE_PROPERTY_BOOL(blockUnderageUsers, false, setBlockUnderageUsers);

    DECLARE_PROPERTY_STRINGLIST(suppressedOfferIds, setSuppressedOfferIds);

    DECLARE_PROPERTY_BYTEARRAY(jsonData, QByteArray(), setJsonData);
    DECLARE_PROPERTY_BYTEARRAY(eTag, QByteArray(), setETag);
    DECLARE_PROPERTY_BYTEARRAY(signature, QByteArray(), setSignature);
    DECLARE_PROPERTY_STRING(locale, "", setLocale);
    DECLARE_PROPERTY_LONGLONG(batchTime, INVALID_BATCH_TIME, setBatchTime);
    DECLARE_PROPERTY_DATETIME(refreshedFromCdnDate, QDATESTRING_PAST_DEFAULTVALUE, QDATESTRING_FORMAT, setRefreshedFromCdnDate);
    DECLARE_PROPERTY_ENUM(DataSource, dataSource, DataSourcePublic, setDataSource);
    DECLARE_PROPERTY_STRING(qualifyingOfferId, "", setQualifyingOfferId);

    bool isBuildVersionStale() const;
    bool isCdnDefinitionStale() const;

    SoftwareServerData& softwareServerData(Services::PlatformService::OriginPlatform platform);
    const SoftwareServerData& softwareServerData(Services::PlatformService::OriginPlatform platform) const;

    QSet<Origin::Services::PlatformService::OriginPlatform> platformsSupported() const;

    bool isBaseGame() const;

    // Believe it or not, these are all subtly different
    bool isPDLC() const; // addons or expansions, downloadable
    bool isPULC() const; // addons or expansions, not downloadable (built into base game somehow)  
    bool isAddonOrBonusContent() const; // addons or expansions, downloadable but no installCheck or executePath set on the software. (think folder of MP3s)
    bool canBeExtraContent() const; // addons, expansions, or full games + expansion (some sims content shows up as full game tiles and expansions under Sims 3 for example)
    bool canBeExpansion() const; // expansions or full games + expansion (some sims content shows up as full game tiles and expansions under Sims 3 for example)

    bool isBrowserGame() const;
    bool newlyPublished() const;
    bool isFreeTrial() const;
    QStringList relatedGameContentIds() const;

    bool isUnreleased() const;
    bool isPreload() const;
    bool isReleased() const;
    bool isDownloadExpired() const;

signals:
    void definitionUpdated();

public:
    Variant getProductOverrideValue(const QString& overrideSettingBaseName) const;
    void processPublishingOverrides();

    static CatalogDefinitionRef createEmptyDefinition();
    static CatalogDefinitionRef parseJsonData(const QString& offerId, const QByteArray& jsonResponseData, const bool fromCache, const DataSource source);

    static bool serialize(QDataStream& outputStream, CatalogDefinitionRef def);
    static bool deserialize(QDataStream& inputStream, CatalogDefinitionRef &def, DataSource source);

#if LOG_CATALOG_DEFINITIONS
    void logCatalogDef();
#endif

private:
    DECLARE_PROPERTY_CONTAINER(CatalogDefinition)

public:
    const SoftwareServerDataMap& softwareServerDataMap() const;
    SoftwareServerDataMap& softwareServerDataMap();
    void setSoftwareServerDataMap(const SoftwareServerDataMap& newBuildMap);

private:
    mutable QReadWriteLock m_softwareServerDataLock;
    SoftwareServerDataMap m_softwareServerDataMap;
};

} // namespace Publishing

} // namespace Services

} // namespace Origin

#endif // CATALOGDEFINITION_H
