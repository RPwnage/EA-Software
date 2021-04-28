//    ContentConfiguration.h
//    Copyright (c) 2012, Electronic Arts
//    All rights reserved.

#ifndef ORIGINCONTENTCONFIGURATION_H
#define ORIGINCONTENTCONFIGURATION_H

#include <limits>
#include <QDateTime>
#include <QVector>
#include <QFlags>
#include <QStringList>

#include "services/publishing/NucleusEntitlement.h"
#include "services/publishing/CatalogDefinition.h"
#include "services/publishing/PricingServiceResponses.h"
#include "services/plugin/PluginAPI.h"
#include "engine/content/ContentTypes.h"
#include "engine/login/User.h"
#include "services/common/VersionInfo.h"

#define CATALOG_GET( type, name )\
public:\
    const type name() const\
    {\
        return m_catalogDef->name();\
    }

#define SOFTWARESERVERDATA_GET( type, name )\
public:\
    const type name(Origin::Services::PlatformService::OriginPlatform platform = Origin::Services::PlatformService::PlatformThis) const\
    {\
        return m_catalogDef->softwareServerData(platform).name();\
    }

#define NUCLEUS_GET( type, name, getter )\
public:\
    const type name() const\
    {\
        QReadLocker locker(&m_propertyLock);\
        return m_nucleusEntitlement->getter();\
    }

namespace Origin
{
    namespace Engine
    {
        namespace Content
        {
            /// \brief TBD.
            class ORIGIN_PLUGIN_API ContentConfiguration : public QObject
            {
                Q_OBJECT

                friend class ContentController;

            public:
                /// \brief TBD.
                static const int NoTestCode = -1;
                static const QString NO_VERSION_OVERRIDE_CONFIGURED;
                static ContentConfigurationRef INVALID_CONTENT_CONFIGURATION;

                static ContentConfigurationRef create(Services::Publishing::CatalogDefinitionRef ref, Services::Publishing::NucleusEntitlementRef nRef = Services::Publishing::NucleusEntitlement::INVALID_ENTITLEMENT);

                /// \brief The ContentConfiguration destructor; releases the resources of a ContentConfiguration instance.
                virtual ~ContentConfiguration();

                void setNucleusEntitlement(Origin::Services::Publishing::NucleusEntitlementRef nent);
                const Origin::Services::Publishing::NucleusEntitlementRef nucleusEntitlement() { QReadLocker lock(&m_propertyLock); return m_nucleusEntitlement; }

                //use the accessors to access the serverData to ensure thread safety
                //the accessors use mutexs to prevent accessing data while changing out the 
                //server data on updates

                SOFTWARESERVERDATA_GET(Origin::Services::PlatformService::OriginPlatform, platformId)
                SOFTWARESERVERDATA_GET(bool, platformEnabled)
                CATALOG_GET(QString, displayName)
                CATALOG_GET(QString, detailImage)
                SOFTWARESERVERDATA_GET(Origin::Services::PlatformService::OriginProcessorArchitecture, availableArchitectures)
                CATALOG_GET(QString, manualUrl)
                CATALOG_GET(QString, contentId)
                CATALOG_GET(QString, itemId)

                CATALOG_GET(QStringList, cloudSaveContentIDFallback)
                SOFTWARESERVERDATA_GET(bool, isCloudSaveSupported)
                SOFTWARESERVERDATA_GET(Origin::Services::Publishing::IGOPermission, igoPermission);
                NUCLEUS_GET(QString, cdKey, cdKey)

                SOFTWARESERVERDATA_GET(QDateTime, downloadStartDate)
                SOFTWARESERVERDATA_GET(QDateTime, useEndDate)
                SOFTWARESERVERDATA_GET(QDateTime, originSubscriptionUseEndDate)

                /// \todo unlockExpiration missing here?

                NUCLEUS_GET(QDateTime, entitleDate, grantDate)
                CATALOG_GET(bool, greyMarketControls)
                SOFTWARESERVERDATA_GET(Origin::Services::Publishing::PackageType, packageType)
                SOFTWARESERVERDATA_GET(QString, installationDirectory)

                /// \brief TBD.
                CATALOG_GET(QString, financeId);

                /// \brief TBD.
                SOFTWARESERVERDATA_GET(QString, executeParameters);

                /// \brief TBD.

                SOFTWARESERVERDATA_GET(QString, saveFileCriteria);

                /// \brief Check whether PDLC can be used without
                /// restarting base game.
                SOFTWARESERVERDATA_GET(Origin::Services::Publishing::AddonDeploymentStrategy, addonDeploymentStrategy);

                /// \brief TBD.
                SOFTWARESERVERDATA_GET(QString, multiplayerId);

                /// \brief TBD.
                SOFTWARESERVERDATA_GET(QString, InstallerPath);

                /// \brief TBD.
                SOFTWARESERVERDATA_GET(QString, installerParams);

                /// \brief TBD.
                SOFTWARESERVERDATA_GET(QString, stagingKeyPath);

                /// \brief TBD.
                CATALOG_GET(bool, addonsAvailable);

                CATALOG_GET(QStringList, availableExtraContent);

                CATALOG_GET(QStringList, suppressedOfferIds);

                /// \brief TBD.
                NUCLEUS_GET(QString, groupName, groupName);

                /// \brief TBD.
                NUCLEUS_GET(qint64, entitlementId, id);

                /// \brief TBD.
                NUCLEUS_GET(QString, entitlementTag, tag);

                /// \brief TBD.
                NUCLEUS_GET(QString, entitlementSource, source);

                /// \brief TBD.
                QDateTime terminationDate() const;

                /// \brief TBD.
                NUCLEUS_GET(Origin::Services::Publishing::NucleusEntitlementStatus, status, status);

                /// \brief TBD.
                NUCLEUS_GET(int, useCount, useCount);

                /// \brief TBD.
                CATALOG_GET(QString, productId);

                /// \brief TBD.
                SOFTWARESERVERDATA_GET(bool, monitorInstall);

                /// \brief TBD.
                SOFTWARESERVERDATA_GET(bool, monitorPlay);

                /// \brief TBD.
                SOFTWARESERVERDATA_GET(Origin::Services::Publishing::PartnerPlatformType, partnerPlatform);

                /// \brief TBD.
                SOFTWARESERVERDATA_GET(bool, showKeyDialogOnInstall);

                /// \brief TBD.
                SOFTWARESERVERDATA_GET(bool, showKeyDialogOnPlay);

                /// \brief TBD.
                const bool enableDLCUninstall(Origin::Services::PlatformService::OriginPlatform platform = Origin::Services::PlatformService::PlatformThis) const;

                /// \brief TBD.
                CATALOG_GET(QStringList, supportedLocales);

                /// \brief TBD.
                CATALOG_GET(QString, cdnAssetRoot);

                /// \brief The server used to retrieve secure box art from.
                CATALOG_GET(QString, imageServer);

                /// \brief The query portion of the URL for the small (thumbnail) secure box art.
                CATALOG_GET(QString, packArtSmall);

                /// \brief The query portion of the URL for the medium secure box art.
                CATALOG_GET(QString, packArtMedium);

                /// \brief The query portion of the URL for the large secure box art.
                CATALOG_GET(QString, packArtLarge);

                /// \brief The pre announcment display date to show if real release date is unknown.
                CATALOG_GET(QString, preAnnouncementDisplayDate);

                /// \brief The query portion of the URL for the background (banner) secure box art.
                CATALOG_GET(QString, backgroundImage);

                /// \brief The query portion of the URL for the SDK secure box art.
                CATALOG_GET(QString, originSdkImage);

                /// \brief TBD.
                SOFTWARESERVERDATA_GET(QString, identityClientIdOverride);

                /// \brief TBD.
                NUCLEUS_GET(Origin::Services::Publishing::OriginPermissions, originPermissions, originPermissions);

                /// \brief TBD.
                CATALOG_GET(Origin::Services::Publishing::OriginDisplayType, originDisplayType);

                /// \brief TBD.
                CATALOG_GET(bool, downloadable);

                /// \brief TBD.
                CATALOG_GET(bool, published);

                /// \brief TBD.
                CATALOG_GET(QDateTime, publishedDate);

                /// \brief use as a way for server side to override computed downloadSize
                SOFTWARESERVERDATA_GET(qint64, downloadSize);

                /// \brief defines the achievementSet identifier for a game.
                const QString achievementSet(Origin::Services::PlatformService::OriginPlatform platform = Origin::Services::PlatformService::PlatformThis) const;

                /// \brief defines the gameType identifier for a game (Normal, free trials, etc).
                CATALOG_GET(Origin::Services::Publishing::ItemSubType, itemSubType);

                /// \brief the length in HOURS of a trial
                CATALOG_GET(int, trialDuration);

                /// \brief the master title id for this product, it is used to identify a group of related products
                CATALOG_GET(QString, masterTitleId);

                /// \brief the master title for this product, it is used to identify a group of related products
                CATALOG_GET(QString, masterTitle);

                /// \brief the alternate master title ids for this product, used to handle alternate parenting
                CATALOG_GET(QStringList, alternateMasterTitleIds)

                /// \brief the franchise id for this product, identifies the franchise that the title is a part of
                CATALOG_GET(QString, franchiseId);

                /// \brief the software id for this product
                SOFTWARESERVERDATA_GET(QString, softwareId);

                /// \brief A long description of the content. May contain html tags.
                CATALOG_GET(QString, longDescription);

                /// \brief A brief description of the content. May contain html tags.
                CATALOG_GET(QString, shortDescription);

                /// \brief A brief message describing the limitations of a trial.
                CATALOG_GET(QString, gameLaunchMessage);

                /// \brief The OFB-configured product type.
                CATALOG_GET(QString, productType);

                /// \brief Can the content be purchased
                bool purchasable() const;

                /// \brief Can the content be downloaded without an entitlement
                CATALOG_GET(bool, previewContent);

                /// \brief the default url for the IGO browser
                CATALOG_GET(QString, IGOBrowserDefaultURL);

                /// \brief the bundle id for Mac app bundle
                SOFTWARESERVERDATA_GET(QString, macBundleId);

                /// \brief Has this content been blacklisted from twitch broadcasting.
                CATALOG_GET(bool, twitchClientBlacklist);

                /// \brief Should downloads of this offer be watermarked
                CATALOG_GET(bool, watermarkDownload);

                /// \brief Should non-dip installs of  this content be attempted to be updated to DiP installs
                SOFTWARESERVERDATA_GET(bool, updateNonDipInstall);

                /// \brief Should this content be excluded from net promoter surveys
                SOFTWARESERVERDATA_GET(bool, netPromoterScoreBlacklist);

                /// \brief Owned offer id making this offer eligible to retrieve confidential definition
                CATALOG_GET(QString, qualifyingOfferId);

                bool isFreeProduct() const;
                QString currentPrice() const;
                QString originalPrice() const;
                QString priceDescription() const;
                bool blockUnderageUsers() const;

                CATALOG_GET(qint64, sortOrderDescending);

                CATALOG_GET(QString, extraContentDisplayGroup);
                CATALOG_GET(QString, extraContentDisplayGroupDisplayName);
                CATALOG_GET(int, extraContentDisplayGroupSortOrderAscending);
                CATALOG_GET(bool, suppressVaultUpgrade);
                CATALOG_GET(int, rank);

                /// \brief TBD.
                CATALOG_GET(QStringList, associatedProductIds);

                CATALOG_GET(bool, useLegacyCatalog);
                SOFTWARESERVERDATA_GET(QString, commerceProfile);

                CATALOG_GET(bool, forceKillAtOwnershipExpiry);
                CATALOG_GET(QSet<Origin::Services::PlatformService::OriginPlatform>, platformsSupported);

                SOFTWARESERVERDATA_GET(QString, liveBuildVersion);
                SOFTWARESERVERDATA_GET(QString, stagedBuildVersion);
                
                SOFTWARESERVERDATA_GET(QString, liveBuildId);
                SOFTWARESERVERDATA_GET(QString, stagedBuildId);

                SOFTWARESERVERDATA_GET(QString, gameLauncherUrl);
                SOFTWARESERVERDATA_GET(QString, gameLauncherUrlClientId);
                SOFTWARESERVERDATA_GET(bool, showSubsSaveGameWarning);

                SOFTWARESERVERDATA_GET(QString, alternateLaunchSoftwareId);

                /// \brief Returns true if any of this content's available processor architectures are supported by this machine
                bool isSupportedByThisMachine() const;

                /// \brief Returns the relative path of the dip manifest for DiP and PDLC content
                QString dipManifestPath(Origin::Services::PlatformService::OriginPlatform platform = Origin::Services::PlatformService::PlatformThis) const;

                bool owned() const;

                // Returns true if a staged build live date indicates that should now be the live build.
                CATALOG_GET(bool, isBuildVersionStale);

                // Returns true if the last fetched CDN definition is stale for any reason and should be refreshed.
                CATALOG_GET(bool, isCdnDefinitionStale);

                /// \brief Returns true if content is a full game
                CATALOG_GET(bool,isBaseGame);

                /// \brief Returns true if content is PDLC
                CATALOG_GET(bool,isPDLC);

                /// \brief Returns true if content is PULC
                CATALOG_GET(bool,isPULC);

                /// \brief Returns true if Origin Display type is Addon, Expansion, or Full Game Plus Expansion and therefore can be displayed in game details page.
                CATALOG_GET(bool,canBeExtraContent);

                /// \brief Returns true if Origin Display type is Expansion or Full Game Plus Expansion and therefore can be displayed in game details page.
                CATALOG_GET(bool, canBeExpansion);

                CATALOG_GET(bool,isBrowserGame);

                CATALOG_GET(bool,isAddonOrBonusContent);

                /// \brief is this content a pre-order entitlement
                NUCLEUS_GET(bool, isPreorder, isPreorder);

                CATALOG_GET(bool, isUnreleased);
                CATALOG_GET(bool, isPreload);
                CATALOG_GET(bool, isReleased);
                CATALOG_GET(bool, isDownloadExpired);

                /// \brief A flag indicating whether there is build metadata present for the current build.
                bool softwareBuildMetadataPresent() const;

                /// \brief A custom implementation to allow softwareBuildMetadata() to be overridden.
                const Services::Publishing::SoftwareBuildMetadata softwareBuildMetadata() const;

                /// \brief A custom implementation to allow installCheck() to be overridden.
                const QString installCheck(Origin::Services::PlatformService::OriginPlatform platform = Origin::Services::PlatformService::PlatformThis) const;

                /// \brief Returns the version (live or build release override version).
                const QString version(Origin::Services::PlatformService::OriginPlatform platform = Origin::Services::PlatformService::PlatformThis) const;

                /// \brief Returns the fileLink (live or build release override version).
                const QString fileLink(Origin::Services::PlatformService::OriginPlatform platform = Origin::Services::PlatformService::PlatformThis) const;

                /// \brief Get the un-overridden version of installCheck() from serverData.
                inline const QString defaultInstallCheck(Origin::Services::PlatformService::OriginPlatform platform = Origin::Services::PlatformService::PlatformThis) const { return m_catalogDef->softwareServerData(platform).installCheck(); }

                /// \brief A custom implementation to allow executePath() to be overridden.
                const QString executePath(Origin::Services::PlatformService::OriginPlatform platform = Origin::Services::PlatformService::PlatformThis) const;

                /// \brief Get the un-overridden version of executePath() from serverData.
                inline const QString defaultExecutePath(Origin::Services::PlatformService::OriginPlatform platform = Origin::Services::PlatformService::PlatformThis) const { return m_catalogDef->softwareServerData(platform).executePath(); }

                /// \brief A custom implementation to allow releaseDate() to be overriden
                const QDateTime releaseDate(Origin::Services::PlatformService::OriginPlatform platform = Origin::Services::PlatformService::PlatformThis) const;

                /// \brief Get the un-overridden version of releaseDate() from serverData.
                inline const QDateTime defaultReleaseDate(Origin::Services::PlatformService::OriginPlatform platform = Origin::Services::PlatformService::PlatformThis) const { return m_catalogDef->softwareServerData(platform).releaseDate(); }
                
                void reloadOverrides();

                /// \brief Returns the test code for the entitlement or NoTestCode for no test code.
                int testCode() const;

                /// \brief Returns whether this content is a non-origin game (based on itemSubType check)
                bool isNonOriginGame() const;

                /// \brief Returns whether this content is an Origin plug-in (based on packageType check)
                bool isPlugin() const;

                /// \brief TBD.
                bool hasOverride() const;

                /// \brief TBD.
                bool overrideUsesJitService() const;

                /// \Brief TDB
                bool hasValidVersion() const;
                /// \brief Detemines if the content associated with this install flow supports grey market.
                ///
                /// \return bool Returns true if the content being installed supports grey market; otherwise, false.
                bool supportsGreyMarket(const QString& version) const;

                /// \brief Returns the build release version override value.  Deprecated in 9.5.  Use buildIdentifierOverride() instead.
                const QString buildReleaseVersionOverride() const;

                /// \brief Returns the build ID override value.
                const QString buildIdentifierOverride() const;

                /// \brief TBD.
                const QString overrideUrl() const;

                /// \brief TBD.
                const QString overrideSyncPackageUrl() const;

                /// \brief Returns the overridden execute path (if one is set).
                const QString overrideExecutePath() const;

                /// \brief TDB.
                void setOverrideVersion(const QString& overrideVersion);

                /// \brief TBD.
                bool isIGOEnabled(Origin::Services::PlatformService::OriginPlatform platform = Origin::Services::PlatformService::PlatformThis) const;

                /// \brief TBD.
                bool protocolUsedAsExecPath(Origin::Services::PlatformService::OriginPlatform platform = Origin::Services::PlatformService::PlatformThis) const;

                /// \brief returns true if entitlement is a free trial.
                CATALOG_GET(bool, isFreeTrial);

                /// \brief Returns true if this entitlement was published less than 28 days ago.
                CATALOG_GET(bool,newlyPublished);

                /// \brief returns a list of contentIds related to this entitlement (used for things like games playing)
                CATALOG_GET(QStringList, relatedGameContentIds);

                /// \brief True if this is a DiP configured item
                bool dip(Origin::Services::PlatformService::OriginPlatform platform = Origin::Services::PlatformService::PlatformThis) const;

                /// \brief Returns underlying software build information for this content
                const Origin::Services::Publishing::SoftwareBuildMap& softwareBuildMap(Origin::Services::PlatformService::OriginPlatform platform = Origin::Services::PlatformService::PlatformThis) const;

                EntitlementRef entitlement() const;
                void setEntitlement(EntitlementRef ref);

                void setPricingData(const Services::Publishing::OfferPricingData &pricingData);
                bool hasPricingData() const { QReadLocker locker(&m_pricingLock); return m_hasPricingData; }

                /// \brief Returns debug info.
                ///
                /// \param summaryInfo TBD.
                /// \param detailedInfo TBD.
                void debugInfo(QVariantMap &summaryInfo, QVariantMap &detailedInfo);

                bool treatUpdatesAsMandatoryAlternate() const { QReadLocker locker(&m_propertyLock); return m_treatUpdatesAsMandatoryAlternate; } 
                void setTreatUpdatesAsMandatoryAlternate( bool forceUpdateGame );

                // TODO TS3_HARD_CODE_REMOVE
                bool isSuppressedSims3Expansion() const;

                /// \brief Return true if we doing a HARD suppress of this game? Meaning, it shouldn't show up in the my games library.
                /// \param entitlement The entitlement to check.
                bool isCompletelySuppressed();

                const QString cloudSavesId() const;

                /// \brief Pass-through function for reading product overrides, without exposing m_catalogDef
                Services::Variant getProductOverrideValue(const QString& overrideSettingBaseName) const;

                //////////////////////////////////////////////////////////////////////////
                // Subscription functions

                /// \brief returns true when the entitlement has expiration
                bool hasExpiration() const;

                /// \brief indicates whether the entitlement was granted by a subscription.
                bool isEntitledFromSubscription() const;

                // Subscription functions
                //////////////////////////////////////////////////////////////////////////
            signals:
                /// \brief TBD.
                void configurationChanged();

            public slots:
                void checkVersionMismatch();

            private:
                void setSelf(ContentConfigurationRef ref) { QWriteLocker locker(&m_propertyLock); m_self = ref; }

            private:
                ContentConfigurationWRef m_self;

                mutable QReadWriteLock m_propertyLock;
                Origin::Services::Publishing::CatalogDefinitionRef m_catalogDef;
                Origin::Services::Publishing::NucleusEntitlementRef m_nucleusEntitlement;
                EntitlementRef m_entitlementHandle;

                explicit ContentConfiguration(Origin::Services::Publishing::CatalogDefinitionRef serverData, Origin::Services::Publishing::NucleusEntitlementRef nucleusEntitlement = Origin::Services::Publishing::NucleusEntitlement::INVALID_ENTITLEMENT, QObject *parent = NULL);

                void setupOverrides();
                void retrieveVersionFromManifest();
                mutable QReadWriteLock m_overrideLock;
                QString m_overrideUrl;
                QString m_overrideSyncPackageUrl;
                QString m_overrideVersion;
                QString m_overrideBuildReleaseVersion;
                QString m_overrideBuildIdentifier;
                bool m_overrideSoftwareBuildMetadataPresent;
                Services::Publishing::SoftwareBuildMetadata m_overrideSoftwareBuildMetadata;
                QString m_installCheckOverride;
                QString m_executePathOverride;
                QString m_releaseDateOverride;
                QString m_reducedPriceOverride;
                QDateTime m_terminationDateOverride;
                QString m_overrideEnableDLCUninstall;

                bool m_treatUpdatesAsMandatoryAlternate;

                mutable QReadWriteLock m_pricingLock;
                bool m_hasPricingData;
                Services::Publishing::OfferPricingData m_pricingData;
            }; 
        }//ContentConfiguration
    } //Engine
} //Origin

#endif // ORIGINCONTENTCONFIGURATION_H
