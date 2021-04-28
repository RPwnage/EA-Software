//    ContentConfiguration.cpp
//    Copyright © 2012, Electronic Arts
//    All rights reserved.

#include <QUrl>
#include <QString>
#include "engine/content/ContentConfiguration.h"
#include "engine/content/ContentController.h"
#include "engine/content/GamesController.h"
#include "services/debug/DebugService.h"
#include "engine/content/Entitlement.h"
#include "engine/login/User.h"
#include "services/settings/SettingsManager.h"
#include "engine/igo/IGOController.h"
#include "services/log/LogService.h"
#include "LocalInstallProperties.h"
#include "engine/content/RetrieveDiPManifestHelper.h"
#include "services/platform/PlatformService.h"
#include "services/publishing/DownloadUrlProviderManager.h"
#include "engine/subscription/SubscriptionManager.h"

#include "TelemetryAPIDLL.h"

using namespace Origin::Services;

static QString replaceOverriddenParts(const QString& base, const QString& key, const QString& file);

namespace
{
    // Any entitlements containing this is the download URL is assumed to have a test code
    const QString TestCodeServer = "cdntest.ea.com";
    
    //list of possible protocols that Origin may launch
    const QString ExecPathProtocols[] = 
    { 
        "steam://",
        "http://",
        "https://"
    };

    const qint64 HAS_EXPIRATION_THRESHOLD_SECS = 30 * 24 * 60 * 60;
}

namespace Origin
{
    namespace Engine
    {
        namespace Content
        {

#if defined(Q_OS_WIN)
            const QString kDiPManifestPath = "__Installer\\installerdata.xml";
#else
            const QString kDiPManifestPath = "__Installer/installerdata.xml";
#endif

            const QString ContentConfiguration::NO_VERSION_OVERRIDE_CONFIGURED = "LOCAL";
            QSharedPointer<ContentConfiguration> ContentConfiguration::INVALID_CONTENT_CONFIGURATION;

            ContentConfigurationRef ContentConfiguration::create(Services::Publishing::CatalogDefinitionRef ref, Services::Publishing::NucleusEntitlementRef nRef)
            {
                ContentConfigurationRef configuration(new ContentConfiguration(ref, nRef));
                configuration->setSelf(configuration);
                configuration->setupOverrides();
                return configuration;
            }

            ContentConfiguration::ContentConfiguration(Origin::Services::Publishing::CatalogDefinitionRef serverData, Origin::Services::Publishing::NucleusEntitlementRef nucleusEntitlement, QObject *parent)
                : QObject(parent)
                , m_propertyLock(QReadWriteLock::Recursive)
                , m_overrideLock(QReadWriteLock::Recursive)
                , m_pricingLock(QReadWriteLock::Recursive)
                , m_overrideSoftwareBuildMetadataPresent(false)
                , m_treatUpdatesAsMandatoryAlternate(false)
                , m_hasPricingData(false)
            {
                m_catalogDef = serverData;
                m_nucleusEntitlement = nucleusEntitlement;

                ORIGIN_VERIFY_CONNECT(m_catalogDef.data(), SIGNAL(definitionUpdated()), this, SIGNAL(configurationChanged()));
                ORIGIN_VERIFY_CONNECT(m_catalogDef.data(), SIGNAL(definitionUpdated()), this, SLOT(checkVersionMismatch()));
                ORIGIN_VERIFY_CONNECT(m_nucleusEntitlement.data(), SIGNAL(entitlementChanged()), this, SIGNAL(configurationChanged()));

                m_entitlementHandle = EntitlementRef();
            }

            ContentConfiguration::~ContentConfiguration()
            {
            }

            void ContentConfiguration::setNucleusEntitlement(Origin::Services::Publishing::NucleusEntitlementRef nent)
            {
                if (m_nucleusEntitlement != nent)
                {
                    {
                        QWriteLocker lock(&m_propertyLock);
                        if (!m_nucleusEntitlement.isNull())
                        {
                            ORIGIN_VERIFY_DISCONNECT(m_nucleusEntitlement.data(), SIGNAL(entitlementChanged()), this, SIGNAL(configurationChanged()));
                        }
                        m_nucleusEntitlement = nent;
                        ORIGIN_VERIFY_CONNECT(m_nucleusEntitlement.data(), SIGNAL(entitlementChanged()), this, SIGNAL(configurationChanged()));
                    }

                    setupOverrides();
                    emit configurationChanged();
                }
            }

            void ContentConfiguration::setEntitlement(EntitlementRef ref)
            {
                QWriteLocker locker(&m_propertyLock);
                m_entitlementHandle = ref;
            }

            EntitlementRef ContentConfiguration::entitlement() const 
            {
                QReadLocker locker(&m_propertyLock);
                return m_entitlementHandle;
            }

            void ContentConfiguration::setPricingData(const Services::Publishing::OfferPricingData &pricingData)
            {
                QWriteLocker locker(&m_pricingLock);
                if (!m_hasPricingData || pricingData != m_pricingData)
                {
                    m_hasPricingData = true;
                    m_pricingData = pricingData;
                    locker.unlock();

                    emit configurationChanged();
                }
            }

            bool ContentConfiguration::softwareBuildMetadataPresent() const
            {
                QReadLocker locker(&m_overrideLock);

                if(m_overrideSoftwareBuildMetadataPresent)
                {
                    return true;
                }
                else if(!m_overrideBuildIdentifier.isEmpty() && softwareBuildMap().contains(m_overrideBuildIdentifier))
                {
                    return softwareBuildMap()[m_overrideBuildIdentifier].buildMetadataPresent();
                }
                else
                {
                    return softwareBuildMap()[liveBuildId()].buildMetadataPresent();
                }
            }

            const Services::Publishing::SoftwareBuildMetadata ContentConfiguration::softwareBuildMetadata() const
            {
                QReadLocker locker(&m_overrideLock);

                if(m_overrideSoftwareBuildMetadataPresent)
                {
                    return m_overrideSoftwareBuildMetadata;
                }
                else if(!m_overrideBuildIdentifier.isEmpty() && softwareBuildMap().contains(m_overrideBuildIdentifier))
                {
                    return softwareBuildMap()[m_overrideBuildIdentifier].buildMetadata();
                }
                else
                {
                    return softwareBuildMap()[liveBuildId()].buildMetadata();
                }
            }

            const QString ContentConfiguration::installCheck(Services::PlatformService::OriginPlatform platform) const
            {
                QReadLocker overrideLocker(&m_overrideLock);

                if(!m_installCheckOverride.isEmpty())
                {
                    return m_installCheckOverride;
                }
                else
                {
                    platform = Services::PlatformService::resolvePlatform(platform);

                    const Services::Publishing::SoftwareServerData &softwareServerData = m_catalogDef->softwareServerData(platform);
                    const QString &installCheck = softwareServerData.installCheck();
                    const QString &bundleId = softwareServerData.macBundleId();

                    // Construct a synthetic installCheck from the bundleID if we're on Mac, there is a bundleID, and no explicit installCheck is set
                    if (Services::PlatformService::PlatformMacOS == platform && !bundleId.isEmpty() && installCheck.isEmpty())
                    {
                        return QString("[%1]").arg(bundleId);
                    }
                    else
                    {
                        return installCheck;
                    }
                }
            }

            const QString ContentConfiguration::executePath(Services::PlatformService::OriginPlatform platform) const
            {
                QReadLocker overrideLocker(&m_overrideLock);

                if(!m_executePathOverride.isEmpty())
                {
                    return m_executePathOverride;
                }
                else
                {
                    platform = Services::PlatformService::resolvePlatform(platform);

                    const Services::Publishing::SoftwareServerData &softwareServerData = m_catalogDef->softwareServerData(platform);
                    const QString &executePath = softwareServerData.executePath();
                    const QString &bundleId = softwareServerData.macBundleId();

                    // Construct a synthetic executePath for base games from the bundleID if we're on Mac, there is a bundleID, and no explicit executePath is set
                    if (Services::PlatformService::PlatformMacOS == platform && !bundleId.isEmpty() && executePath.isEmpty() && isBaseGame())
                    {
                        return QString("[%1]").arg(bundleId);
                    }
                    else
                    {
                        return executePath;
                    }
                }
            }

            const QDateTime ContentConfiguration::releaseDate(Services::PlatformService::OriginPlatform platform) const
            {
                QReadLocker overrideLocker(&m_overrideLock);

                if (!m_releaseDateOverride.isEmpty())
                {
                    QDateTime releaseDate = QDateTime::fromString(m_releaseDateOverride, "yyyy-MM-dd hh:mm:ss 'GMT'");
                    if (!releaseDate.isValid())
                    {
                        releaseDate = QDateTime::fromString(m_releaseDateOverride, "yyyy-MM-dd hh:mm:ss.z");
                        if (!releaseDate.isValid())
                        {
                            releaseDate = m_catalogDef->softwareServerData(platform).releaseDate();
                        }
                    }

                    return releaseDate;
                }
                else
                {
                    return m_catalogDef->softwareServerData(platform).releaseDate();
                }
            }

            bool ContentConfiguration::hasOverride() const
            {
                QReadLocker locker(&m_overrideLock);
                return !m_overrideUrl.isEmpty();
            }

            bool ContentConfiguration::overrideUsesJitService() const
            {
                QReadLocker locker(&m_overrideLock);
                return hasOverride() && Publishing::DownloadUrlProviderManager::instance()->overrideUsesJitService(m_overrideUrl);
            }

            bool ContentConfiguration::hasValidVersion() const
            {
                QReadLocker locker(&m_overrideLock);

                if(hasOverride())
                {
                    return (m_overrideVersion != NO_VERSION_OVERRIDE_CONFIGURED);
                }
                else
                {
                    return true;
                }
            }

            // True if the content being installed supports grey market, false otherwise.
            // This content supports grey market if the DiP version is at least 1.1 and 
            // the OFB greyMarket flag is set
            bool ContentConfiguration::supportsGreyMarket(const QString& version) const
            {
                VersionInfo testVer = VersionInfo(1, 0, 0, 0);
                VersionInfo manifestVersion = VersionInfo(version);

                // Less-than and equal are the only comparison operators for VersionInfo
                // so we have to do the test this way around.
                if (testVer < manifestVersion && greyMarketControls())
                    return true;

                return false;
            }
            
            const QString ContentConfiguration::buildReleaseVersionOverride() const
            {
                QReadLocker locker(&m_overrideLock);
                return m_overrideBuildReleaseVersion;
            }

            const QString ContentConfiguration::buildIdentifierOverride() const
            {
                QReadLocker locker(&m_overrideLock);

                if(!m_overrideBuildIdentifier.isEmpty())
                {
                    return m_overrideBuildIdentifier;
                }
                else if(isPlugin())
                {
                    return liveBuildId();
                }
                else
                {
                    return QString();
                }
            }

            QDateTime ContentConfiguration::terminationDate() const
            {
                QReadLocker overrideLocker(&m_overrideLock);

                if (m_terminationDateOverride.isValid())
                {
                    return m_terminationDateOverride;
                }
                else
                {
                    QReadLocker propertyLocker(&m_propertyLock);
                    return m_nucleusEntitlement->terminationDate();
                }
            }

            bool ContentConfiguration::purchasable() const
            {
                if (hasPricingData())
                {
                    QReadLocker locker(&m_pricingLock);
                    return m_pricingData.isPurchasable;
                }
                else
                {
                    return m_catalogDef->purchasable();
                }
            }

            const QString ContentConfiguration::overrideUrl() const
            {
                QReadLocker locker(&m_overrideLock);
                return m_overrideUrl;
            }

            const QString ContentConfiguration::overrideSyncPackageUrl() const
            {
                QReadLocker locker(&m_overrideLock);
                return m_overrideSyncPackageUrl;
            }

            const QString ContentConfiguration::overrideExecutePath() const
            {
                QReadLocker locker(&m_overrideLock);
                return m_executePathOverride;
            }

            void ContentConfiguration::setOverrideVersion(const QString& overrideVersion)
            {
                QWriteLocker locker(&m_overrideLock);
                m_overrideVersion = overrideVersion;
            }

            void ContentConfiguration::reloadOverrides()
            {
                m_catalogDef->processPublishingOverrides();
                setupOverrides();
                emit configurationChanged();
            }

            int ContentConfiguration::testCode() const
            {
                if (originPermissions() == Origin::Services::Publishing::OriginPermissions1102)
                {
                    // OFB configured 1102-like
                    return 1102;
                }
                else if (originPermissions() == Origin::Services::Publishing::OriginPermissions1103)
                {
                    // OFB configured 1103-like
                    return 1103;
                }
                else
                {
                    return NoTestCode;
                }
            }
            
            void ContentConfiguration::debugInfo(QVariantMap &summaryInfo, QVariantMap &detailedInfo)
            {
                QReadLocker locker(&m_propertyLock);

                if (Origin::Services::readSetting(Origin::Services::SETTING_ContentDebug).toQVariant().toBool())
                {
                    summaryInfo["Content ID"] = contentId();
                    summaryInfo["Offer ID"] = productId();
                    summaryInfo["Master Title ID"] = masterTitleId();

                    //--- Version and override information but only for Non-NOGS
                    if(!isNonOriginGame())
                    {
                        if(m_entitlementHandle)
                        {
                            if(m_entitlementHandle->localContent()->installState() == LocalContent::ContentInstalled)
                            {
                                if(isPlugin())
                                {
                                    VersionInfo pluginVersion, compatibleClientVersion;
                                    Services::PlatformService::getPluginVersionInfo(m_entitlementHandle->localContent()->executablePath(), pluginVersion, compatibleClientVersion);
                                    summaryInfo["Compatible Client (Installed)"] = compatibleClientVersion.ToStr();
                                    summaryInfo["Version Installed"] = pluginVersion.ToStr();
                                }
                                else
                                {
                                    summaryInfo["Version Installed"] = m_entitlementHandle->localContent()->installedVersion();
                                }
                            }

                            // We only show version available in certain contexts.
                            if(m_entitlementHandle->localContent()->downloadable() || 
                               m_entitlementHandle->localContent()->installable() || 
                               m_entitlementHandle->localContent()->playable())
                            {
                                summaryInfo["Version Available"] = version();

                                if(isPlugin())
                                {
                                    summaryInfo["Compatible Client (Available)"] = softwareBuildMap()[liveBuildId()].buildMetadata().mRequiredClientVersion.ToStr();
                                }
                            }
                        }

                        if(hasOverride())
                        {
                            summaryInfo["Override URL"] = overrideUrl();
                        }
                        
                        if(!buildReleaseVersionOverride().isEmpty())
                        {
                            summaryInfo["Override Build Version"] = buildReleaseVersionOverride();
                        }

                        if(!buildIdentifierOverride().isEmpty())
                        {
                            summaryInfo["Override Build ID"] = buildIdentifierOverride();
                        }

                    }

                    if(m_entitlementHandle)
                    {
                        //-----------------------------------------------------
                        //    Strings ops get the registry key and file paths
                        //-----------------------------------------------------
                        QString installCheckRegKeyString;
                        QString installCheckFileString;
                        QString launchPathFileString;

                        if(!installCheck().isEmpty())
                        {
                            QStringList strlist = installCheck().split("]");
                            if (strlist.length() >= 1)
                            {
                                installCheckRegKeyString = strlist[0];
                                installCheckRegKeyString += ']';
                            }
                            if (strlist.length() >= 2)
                                installCheckFileString = strlist[1];

                            strlist = executePath().split("]");
                            if (strlist.length() >= 2)
                                launchPathFileString = strlist[1];

                            QString installCheckRegKeyValue = protocolUsedAsExecPath() == false ? PlatformService::localDirectoryFromOSPath(installCheckRegKeyString) : QString();

                            //-----------------------------------------------------
                            //    Install Check Registry Key
                            //-----------------------------------------------------
                            if ((m_entitlementHandle->localContent()->state() == LocalContent::ReadyToPlay) || 
                                (m_entitlementHandle->localContent()->state() == LocalContent::Installed))
                            {
                                detailedInfo["Install Check Registry Key"] = installCheckRegKeyString;
                            }
                            else
                            {
                                //    If the install check path is empty, then the registry key was not found.
                                detailedInfo["Install Check Registry Key"] = QString("Missing key-") + installCheckRegKeyString;
                            }

                            //-----------------------------------------------------
                            //    Install Check Registry Key Value
                            //-----------------------------------------------------
                            if(!installCheckRegKeyValue.isEmpty())
                                detailedInfo["Install Check Registry Key Value"] = installCheckRegKeyValue;
                        }

                        //-----------------------------------------------------
                        //    Install Check File
                        //-----------------------------------------------------
                        if(!installCheckFileString.isEmpty() &&
                            (m_entitlementHandle->localContent()->state() == LocalContent::ReadyToPlay || 
                             m_entitlementHandle->localContent()->state() == LocalContent::Installed))
                        {
                            detailedInfo["Install Checked File"] = installCheckFileString;
                        }

                        //-----------------------------------------------------
                        //    Launch Path information
                        //-----------------------------------------------------
                        if(m_entitlementHandle->localContent()->state() == LocalContent::ReadyToPlay && !launchPathFileString.isEmpty())
                        {
                            //    If the state is ready to play then it's all good
                            detailedInfo["Launch Path File"] = launchPathFileString;        
                        }
                        else if (m_entitlementHandle->localContent()->state() == LocalContent::Installed)
                        {
                            //    If the state is installed then we know that launch path is missing.

                            //    Launch Path File
                            if (launchPathFileString.isEmpty() == true)
                            {
                                detailedInfo["Launch Path File"] = QString("Missing Key-") + installCheckRegKeyString;
                            }
                            else
                            {
                                detailedInfo["Launch Path File"] = QString("Missing File-") + launchPathFileString;
                            }
                        }
                    }
                }

                if (testCode() != ContentConfiguration::NoTestCode)
                {
                    summaryInfo["Test Code"] = QString::number(testCode());
                }
            }

            bool ContentConfiguration::isNonOriginGame() const
            {
                return (itemSubType() == Origin::Services::Publishing::ItemSubTypeNonOrigin);
            }

            bool ContentConfiguration::isPlugin() const
            {
                return (packageType() == Origin::Services::Publishing::PackageTypeOriginPlugin);
            }

            bool ContentConfiguration::isIGOEnabled(Origin::Services::PlatformService::OriginPlatform platform) const
            {
                // Cases this code covers:
				// 
                // 0. Free Trial -- IGO must always be enabled for a free trial regardless of config or setting
                //
                // 1. Entitlement-specific INI entry:
				// - blacklisted - return false
				// - defaulton - return true
				// - default off - return false
				// (Note: INI settings are for overriding server-supplied settings)

				// 2. Globally OIG enabled/disabled.
				// - if disabled return false

				// 3. Server-supplied blacklisted attribute
				// - If blacklisted return false

                // 4. Is OIG manually disabled by the user
                // - if so return false

				// 5. Is OIG enabled for all games
				// - if so return true

				// 6. Server-supplied Entitlement-specific OIG enabled
				// - if enabled return true

                //---------------------------------------------------------------
                // 0. Free Trial -- process killing code is in the DLL so we need the IGO to always load
                if(isFreeTrial())
                    return true;

                //---------------------------------------------------------------
                // 1. Globally OIG enabled/disabled.

                // UI has IGO enabled, and for all games - cloud settings are where the UI reads/writes
                // Global .ini entry
                bool isEnableOIG = IGOController::isEnabled();
                bool isEnableOIGForAlliniSetting = Services::readSetting(SETTING_EnableIGOForAll.name());
                bool isEnableOIGForAll = IGOController::isEnabledForAllGames();

                // OIG not enabled?
                if (!isEnableOIG)
                    return false;

                // EACore.ini override flag should override all settings!!!
                if (isEnableOIGForAlliniSetting)
                    return true;

                //---------------------------------------------------------------
                // 2. Entitlement-specific INI entry:
                bool isEnableOIGGameIniSetting = false;
                Services::Publishing::IGOPermission entitlementPermissionIniSetting = Publishing::IGOPermissionOptIn;
                // Game-specific .ini entry - we also need to know
                // if any valid setting was specified, because we
                // shouldn't test against a missing/invalid value.
                const QString &settingVal = m_catalogDef->getProductOverrideValue(Services::SETTING_OverrideIGOSetting.name()).toString();

                if (settingVal.compare("blacklist", Qt::CaseInsensitive) == 0)
                {
                    entitlementPermissionIniSetting = Publishing::IGOPermissionBlacklisted;
                    isEnableOIGGameIniSetting = true;
                }
                else if (settingVal.compare("defaulton", Qt::CaseInsensitive) == 0)
                {
                    entitlementPermissionIniSetting = Publishing::IGOPermissionSupported;
                    isEnableOIGGameIniSetting = true;
                }
                else if (settingVal.compare("defaultoff", Qt::CaseInsensitive) == 0)
                {
                    entitlementPermissionIniSetting = Publishing::IGOPermissionOptIn;
                    isEnableOIGGameIniSetting = true;
                }

                if (isEnableOIGGameIniSetting)
                {
                    switch(entitlementPermissionIniSetting)
                    {
                    default:
                    case Publishing::IGOPermissionBlacklisted:
                        return false;
                    case Publishing::IGOPermissionOptIn:
                        return isEnableOIGForAll;
                    case Publishing::IGOPermissionSupported:
                        return true;
                    }
                }

                //---------------------------------------------------------------
                // 3. Server-supplied blacklisted attribute
                // Is the entitlement blacklisted from OIG?
                if(m_catalogDef->softwareServerData(platform).igoPermission() == Publishing::IGOPermissionBlacklisted)
                    return false;

                //---------------------------------------------------------------
                // 4. Is OIG manually disabled by the user
                if(GamesController::currentUserGamesController()->gameHasOIGManuallyDisabled(productId()))
                    return false;

				//---------------------------------------------------------------
				// 5. Is OIG enabled for all games
				if (isEnableOIGForAll)
					return true;

				//---------------------------------------------------------------
				// 6. Server-supplied Entitlement-specific OIG enabled
				Publishing::IGOPermission entitlementOIGpermission = m_catalogDef->softwareServerData(platform).igoPermission();
                if (entitlementOIGpermission == Publishing::IGOPermissionSupported)
                    return true;


                return false;
            }

            bool ContentConfiguration::protocolUsedAsExecPath(Origin::Services::PlatformService::OriginPlatform platform) const
            {
                bool isProtocol = false;
                int i =0;


                //grab the execute path
                if(!executePath(platform).isEmpty())
                {
                    const int numProtocols = sizeof(ExecPathProtocols)/sizeof(ExecPathProtocols[0]);

                    for(i = 0; i < numProtocols; i++)
                    {
                        //checks the exec path to see if the first letters are the same as the protocol
                        if(ExecPathProtocols[i].compare(executePath(platform).left(ExecPathProtocols[i].length()), Qt::CaseInsensitive) == 0)
                        {
                            isProtocol = true;
                            break;
                        }
                    }

                }
                return isProtocol;

            }

            void ContentConfiguration::checkVersionMismatch()
            {
                QReadLocker locker(&m_propertyLock);
                if (m_entitlementHandle && m_entitlementHandle->localContent())
                {
                    // Check to see if the local version matches the config version
                    const Downloader::LocalInstallProperties *localInstallProperties = m_entitlementHandle->localContent()->localInstallProperties();
                    if (localInstallProperties)
                    {
                        const VersionInfo serverVersionInfo(version());
                        const VersionInfo &localVersion = localInstallProperties->GetGameVersion();

                        if (localVersion != serverVersionInfo)
                        {
                            ORIGIN_LOG_EVENT << "Manifest [" << productId() << "] Versions: Installed [" <<
                                localVersion.ToStr() << "] Server [" << serverVersionInfo.ToStr() << "]";

                            // send telemetry
                            GetTelemetryInterface()->Metric_AUTOPATCH_CONFIGVERSION_DIPVERSION_DONTMATCH(
                                productId().toLocal8Bit(), localVersion.ToStr().toLocal8Bit(),
                                serverVersionInfo.ToStr().toLocal8Bit());
                        }
                    }
                }
            }

            void ContentConfiguration::setupOverrides()
            {
                QReadLocker propertyLocker(&m_propertyLock);

                // clear any current overrides, since these product override settings can be updated by the ODT without a client restart
                {
                    QWriteLocker overrideLocker(&m_overrideLock);
                    m_overrideUrl = "";
                    m_overrideVersion = "";
                    m_installCheckOverride = "";
                    m_executePathOverride = "";
                    m_overrideBuildReleaseVersion = "";
                    m_overrideBuildIdentifier = "";
                    m_overrideSyncPackageUrl = "";
                }

                const QString &sharedNetworkOverride = m_catalogDef->getProductOverrideValue(Services::SETTING_OverrideSNOFolder.name());

                // Shared network override now takes precendence over OverrideBuildIdentifier, OverrideBuildReleaseVersion, and the old school overrides.
                if (!sharedNetworkOverride.isEmpty())
                {
                    ContentController* contentController = ContentController::currentUserContentController();
                    if (contentController)
                    {
                        m_overrideUrl = contentController->sharedNetworkOverrideManager()->downloadOverrideForOfferId(productId());
                    }
                    retrieveVersionFromManifest();
                }

                const QString &overrideBuildIdentifier = m_catalogDef->getProductOverrideValue(Services::SETTING_OverrideBuildIdentifier.name());

                if (sharedNetworkOverride.isEmpty() && !overrideBuildIdentifier.isEmpty())
                {
                    if(originPermissions() == Origin::Services::Publishing::OriginPermissionsNormal)
                    {
                        ORIGIN_LOG_WARNING << "Can only use \"OverrideBuildIdentifier\" setting with 1102/1103 entitlements.  Ignoring override for product [" << productId() << "]";
                    }
                    else
                    {
                        QWriteLocker overrideLocker(&m_overrideLock);
                        m_overrideBuildIdentifier = overrideBuildIdentifier;
                    }

                    // We're currently using buildLiveDate as the build identifier.  This is not ideal because the live date can change.
                    // We want to surface an error message when the user tries to download if he gets into a state where the specified live date 
                    // no longer matches a build, so don't prevent m_overrideBuildIdentifier from being set if we don't detect a matching build.
                    if(!m_catalogDef->softwareServerData(Origin::Services::PlatformService::PlatformThis).softwareBuildMap().contains(overrideBuildIdentifier))
                    {
                        ORIGIN_LOG_WARNING << "Specified \"OverrideBuildIdentifier\" setting [" << overrideBuildIdentifier << "] is not valid for available software builds.";
                    }
                }

                // OverrideBuildIdentifier is the replacement for OverrideBuildReleaseVersion, but in the meantime we should support OverrideBuildReleaseVersion 
                // if no OverrideBuildIdentifier is present (and no shared network override).
                if (sharedNetworkOverride.isEmpty() && m_overrideBuildIdentifier.isEmpty())
                {
                    const QString &overrideBuildReleaseVersion = m_catalogDef->getProductOverrideValue(Services::SETTING_OverrideBuildReleaseVersion.name());

                    if (!overrideBuildReleaseVersion.isEmpty())
                    {
                        if(originPermissions() == Origin::Services::Publishing::OriginPermissionsNormal)
                        {
                            ORIGIN_LOG_WARNING << "Can only use \"OverrideBuildReleaseVersion\" setting with 1102/1103 entitlements.  Ignoring override for product [" << productId() << "]";
                        }
                        else
                        {
                            bool validBuildReleaseVersion = false;
                            const Services::Publishing::SoftwareBuildMap& map = softwareBuildMap();
                            for(Services::Publishing::SoftwareBuildMap::const_iterator it = map.constBegin(); it != map.constEnd(); ++it)
                            {
                                if(it.value().buildReleaseVersion() == overrideBuildReleaseVersion)
                                {
                                    validBuildReleaseVersion = true;
                                    break;
                                }
                            }

                            if(!validBuildReleaseVersion)
                            {
                                ORIGIN_LOG_WARNING << "Specified \"OverrideBuildReleaseVersion\" setting [" << overrideBuildReleaseVersion << "] is not valid for available software builds.  Ignoring override for id [" << productId() << "]";
                            }
                            else
                            {
                                QWriteLocker overrideLocker(&m_overrideLock);
                                m_overrideBuildReleaseVersion = overrideBuildReleaseVersion;
                            }
                        }
                    }
                }

                // We only look for the old school overrides when there is no shared network override, build release version, or build identifier override present
                if (sharedNetworkOverride.isEmpty() && m_overrideBuildIdentifier.isEmpty() && m_overrideBuildReleaseVersion.isEmpty())
                {
                    const QString &overridePath = m_catalogDef->getProductOverrideValue(Services::SETTING_OverrideDownloadPath.name());

                    if (!overridePath.isEmpty())
                    {
                        {
                            QWriteLocker overrideLocker(&m_overrideLock);
                            m_overrideUrl = overridePath.trimmed();
                            m_overrideVersion = ContentConfiguration::NO_VERSION_OVERRIDE_CONFIGURED;
                        }

                        // Check for override version, otherwise set it to LOCAL
                        const QString &overrideVersion = m_catalogDef->getProductOverrideValue(Services::SETTING_OverrideVersion.name());

                        if(!overrideVersion.isEmpty())
                        {
                            QWriteLocker overrideLocker(&m_overrideLock);
                            m_overrideVersion = overrideVersion;
                        }
                        else
                        {
                            retrieveVersionFromManifest();
                        }
                    }

                    QWriteLocker overrideLocker(&m_overrideLock);
                    m_overrideSyncPackageUrl = m_catalogDef->getProductOverrideValue(Services::SETTING_OverrideDownloadSyncPackagePath.name()).toString().trimmed();
                }

                const QString &overrideInstallRegKey = m_catalogDef->getProductOverrideValue(Services::SETTING_OverrideInstallRegistryKey.name());
                const QString &overrideInstallFilename = m_catalogDef->getProductOverrideValue(Services::SETTING_OverrideInstallFilename.name());
                const QString &origInstallCheck = defaultInstallCheck();
                const QString &overriddenInstallCheck = replaceOverriddenParts(origInstallCheck, overrideInstallRegKey, overrideInstallFilename);

                // If nothing was replaced, we get back the original
                // string. That means there's no override.
                if (origInstallCheck.compare(overriddenInstallCheck, Qt::CaseInsensitive) != 0)
                {
                    QWriteLocker overrideLocker(&m_overrideLock);
                    m_installCheckOverride = overriddenInstallCheck;
                }

                const QString &overrideExecuteRegKey = m_catalogDef->getProductOverrideValue(Services::SETTING_OverrideExecuteRegistryKey.name());
                const QString &overrideExecuteFilename = m_catalogDef->getProductOverrideValue(Services::SETTING_OverrideExecuteFilename.name());
                const QString &origExecPath = defaultExecutePath();
                const QString &overriddenExecPath = replaceOverriddenParts(origExecPath, overrideExecuteRegKey, overrideExecuteFilename);

                Origin::Services::Variant overrideEnableDLCUninstall = m_catalogDef->getProductOverrideValue(Services::SETTING_OverrideEnableDLCUninstall.name());
                if (!overrideEnableDLCUninstall.isNull())
                {
                    QWriteLocker overrideLocker(&m_overrideLock);
                    m_overrideEnableDLCUninstall = overrideEnableDLCUninstall.toString();
                }

                // If nothing was replaced, we get back the original
                // string. That means there's no override.
                if (origExecPath.compare(overriddenExecPath, Qt::CaseInsensitive) != 0)
                {
                    QWriteLocker overrideLocker(&m_overrideLock);
                    m_executePathOverride = overriddenExecPath;
                }

                QWriteLocker overrideLocker(&m_overrideLock);
                m_releaseDateOverride = m_catalogDef->getProductOverrideValue(Services::SETTING_OverrideReleaseDate.name()).toString();

                // Only override Free Trial and Price related settings in non-Production environments
                const QString &environment = Services::readSetting(Services::SETTING_EnvironmentName).toString().toLower();
                if (environment != SETTING_ENV_production)
                {
                    m_reducedPriceOverride = m_catalogDef->getProductOverrideValue(Services::SETTING_OverrideSalePrice.name()).toString();

                    if (m_reducedPriceOverride.isEmpty())
                    {
                        m_reducedPriceOverride = m_catalogDef->getProductOverrideValue(Services::SETTING_OverrideSalePrice.name()).toString();
                    }

                    const QString &terminationDateStr = m_catalogDef->getProductOverrideValue(Services::SETTING_OverrideTerminationDate.name());
                    if (!terminationDateStr.isEmpty())
                    {
                        const QString dateFormat = "yyyy-MM-dd hh:mm:ss.z";

                        m_terminationDateOverride = QDateTime::fromString(terminationDateStr, dateFormat);
                        m_terminationDateOverride.setTimeSpec(Qt::UTC);

                        ORIGIN_LOG_ERROR_IF(!m_terminationDateOverride.isValid()) <<
                            "Termination date override error for [" << productId() << "] - (" << terminationDateStr << "); " << 
                            "Please check that the date is in this format (UTC TIME)--> yyyy-MM-dd hh:mm:ss.z";
                    }
                    else
                    {
                        m_terminationDateOverride = QDateTime();
                    }
                }
            }

            void ContentConfiguration::retrieveVersionFromManifest()
            {
                if (!dip())
                {
                    return;
                }
                
                ORIGIN_LOG_DEBUG << "Fetching DiP Manifest from download override for [" << productId() << "] to check for game version.";

                Origin::Downloader::RetrieveDiPManifestHelper dipHelper(m_overrideUrl, m_self.toStrongRef());

                // Previously parsed manifest get cached to a static container, however,
                // our manifest values may have changed, so we need to clear the cache
                // in order to force a fresh read.
                dipHelper.clearManifestCache();

                if (dipHelper.retrieveDiPManifestSync())
                {
                    QWriteLocker overrideLocker(&m_overrideLock);
                    if (dipHelper.dipManifest().GetUseGameVersionFromManifestEnabled())
                    {
                        m_overrideVersion = dipHelper.dipManifest().GetGameVersion().ToStr();
                    }

                    // Grab the buildMetaData block from the DiP manifest so we can replace whatever we had in the catalog data
                    m_overrideSoftwareBuildMetadataPresent = true;
                    m_overrideSoftwareBuildMetadata = dipHelper.dipManifest().GetSoftwareBuildMetadata();
                }
            }

            bool ContentConfiguration::isFreeProduct() const
            {
                QReadLocker locker(&m_pricingLock);
                return m_pricingData.isFreeProduct;
            }

            QString ContentConfiguration::currentPrice() const
            {
                QReadLocker overrideLocker(&m_overrideLock);

                if (!m_reducedPriceOverride.isEmpty())
                {
                    return m_reducedPriceOverride;
                }
                else
                {
                    QReadLocker propertyLocker(&m_pricingLock);
                    return m_pricingData.currentPrice;
                }
            }

            QString ContentConfiguration::originalPrice() const
            {
                QReadLocker locker(&m_pricingLock);
                return m_pricingData.originalPrice;
            }

            QString ContentConfiguration::priceDescription() const
            {
                QReadLocker locker(&m_pricingLock);
                return m_pricingData.priceDescription;
            }

            bool ContentConfiguration::blockUnderageUsers() const
            {
                if (hasPricingData())
                {
                    QReadLocker locker(&m_pricingLock);
                    return m_pricingData.blockUnderageUsers;
                }
                else
                {
                    return m_catalogDef->blockUnderageUsers();
                }
            }

            bool ContentConfiguration::isSupportedByThisMachine() const
            {
                // First of all, are we even on the right platform?
                return platformEnabled() &&
                    PlatformService::queryThisMachineCanExecute(availableArchitectures());
            }

            /// \brief Returns the relative path to the dip manifest within the DiP package for dip/PDLC content
            QString ContentConfiguration::dipManifestPath(Origin::Services::PlatformService::OriginPlatform platform) const
            {
                if(dip())
                {
                    QString manifestPath(kDiPManifestPath);

                    if(isPDLC())
                    {
                        // PDLC has additional pathing info stored in the content xml under the metadata install location key.
                        // This additional pathing is prepended to the default DiP manifest path.

                        QString metadataInstallLocation;

                        if(!m_catalogDef->softwareServerData(platform).metadataInstallLocation().isEmpty())
                        {
                            metadataInstallLocation = m_catalogDef->softwareServerData(platform).metadataInstallLocation();
                        }
                        else
                        {
                            metadataInstallLocation = m_catalogDef->softwareServerData(platform).InstallerPath();
                        }

                        if (!metadataInstallLocation.isEmpty())
                        {
                            // TODO: these substitutions seem unneccessary and anti-mac.  I'm loath to change them since it's been this way for awhile.
                            // For P4 history see ContentUtils::buildDipManifestPath that I removed with this same changelist.
                            metadataInstallLocation.replace("/", "\\");

                            if (!metadataInstallLocation.endsWith("\\"))
                            {
                                metadataInstallLocation.append("\\");
                            }

                            manifestPath.prepend(metadataInstallLocation);
                        }
                    }

                    return manifestPath;
                }
                else
                {
                    return "";
                }
            }

            bool ContentConfiguration::owned() const
            {
                QReadLocker locker(&m_propertyLock);
                return isNonOriginGame() || m_nucleusEntitlement != Origin::Services::Publishing::NucleusEntitlement::INVALID_ENTITLEMENT;
            }

            bool ContentConfiguration::dip(Origin::Services::PlatformService::OriginPlatform platform) const
            {
                return (packageType(platform) == Origin::Services::Publishing::PackageTypeDownloadInPlace ||
                    packageType(platform) == Origin::Services::Publishing::PackageTypeOriginPlugin);
            }

            const Origin::Services::Publishing::SoftwareBuildMap& ContentConfiguration::softwareBuildMap(Services::PlatformService::OriginPlatform platform) const
            {
                return m_catalogDef->softwareServerData(platform).softwareBuildMap();
            }

            const QString ContentConfiguration::version(Services::PlatformService::OriginPlatform platform) const
            {
                QReadLocker overrideLocker(&m_overrideLock);

                if(hasOverride())
                {
                    return m_overrideVersion;
                }
                else if(!m_overrideBuildIdentifier.isEmpty())
                {
                    return softwareBuildMap(platform).value(m_overrideBuildIdentifier).buildReleaseVersion();
                }
                else if(!m_overrideBuildReleaseVersion.isEmpty())
                {
                    return m_overrideBuildReleaseVersion;
                }
                else
                {
                    return m_catalogDef->softwareServerData(platform).liveBuildVersion();
                }
            }

            const QString ContentConfiguration::fileLink(Services::PlatformService::OriginPlatform platform) const
            {
                QReadLocker overrideLocker(&m_overrideLock);

                if(!m_overrideBuildIdentifier.isEmpty())
                {
                    return softwareBuildMap(platform).value(m_overrideBuildIdentifier).fileLink();
                }
                else if(!m_overrideBuildReleaseVersion.isEmpty())
                {
                    QString fileLink;
                    const Services::Publishing::SoftwareBuildMap& map = softwareBuildMap(platform);
                    for(Services::Publishing::SoftwareBuildMap::const_iterator it = map.constBegin(); it != map.constEnd(); ++it)
                    {
                        if(it.value().buildReleaseVersion() == m_overrideBuildReleaseVersion)
                        {
                            fileLink = it.value().fileLink();
                            break;
                        }
                    }
                    return fileLink;
                }
                else
                {
                    return softwareBuildMap(platform).value(m_catalogDef->softwareServerData(platform).liveBuildId()).fileLink();
                }
            }

            const bool ContentConfiguration::enableDLCUninstall(Origin::Services::PlatformService::OriginPlatform platform) const
            {
                QReadLocker overrideLocker(&m_overrideLock);

                if (m_overrideEnableDLCUninstall.isEmpty())
                {
                    return m_catalogDef->softwareServerData(platform).enableDLCUninstall();
                }
                else 
                {
                    if (m_overrideEnableDLCUninstall.compare("true", Qt::CaseInsensitive) == 0 || m_overrideEnableDLCUninstall.compare("1") == 0)
                    {
                        return true;
                    }
                }
                return false;
            }

            const QString ContentConfiguration::achievementSet(Origin::Services::PlatformService::OriginPlatform platform) const
            {
                const QString &achievementOverride = m_catalogDef->getProductOverrideValue(Services::SETTING_OverrideAchievementSet.name());

                if(achievementOverride.isEmpty())
                {
                    return m_catalogDef->softwareServerData(platform).achievementSet();
                }
                else
                {
                    return achievementOverride;
                }
            }

            void ContentConfiguration::setTreatUpdatesAsMandatoryAlternate( bool forceUpdateGame )
            {
                QWriteLocker locker(&m_propertyLock);
                m_treatUpdatesAsMandatoryAlternate = forceUpdateGame;
            }

            // TODO TS3_HARD_CODE_REMOVE
            bool ContentConfiguration::isSuppressedSims3Expansion() const
            {
                QStringList masterTitleIds(masterTitleId());
                masterTitleIds << alternateMasterTitleIds();

                const bool hasSims3MasterTitleId = masterTitleIds.contains(Services::subs::Sims3MasterTitleId);
                const bool isFGPE = originDisplayType() == Publishing::OriginDisplayTypeFullGamePlusExpansion;
                const bool userOwnsSims3 = !entitlement()->parentList().isEmpty();

                return (hasSims3MasterTitleId && isFGPE && userOwnsSims3);
            }

            bool ContentConfiguration::isCompletelySuppressed()
            {
                if (itemSubType() == Publishing::ItemSubTypeTimedTrial_Access || itemSubType() == Publishing::ItemSubTypeTimedTrial_GameTime)
                {
                    foreach(const EntitlementRef subEntitlement, entitlement()->contentController()->entitlementsByMasterTitleId(masterTitleId()))
                    {
                        if (subEntitlement->contentConfiguration()->owned() &&
                            subEntitlement->contentConfiguration()->isBaseGame() &&
                            (subEntitlement->contentConfiguration()->isReleased() || subEntitlement->contentConfiguration()->originPermissions() > Origin::Services::Publishing::OriginPermissionsNormal) &&
                            subEntitlement->contentConfiguration()->itemSubType() == Publishing::ItemSubTypeNormalGame)
                        {
                            return true;
                        }
                    }
                }
                return false;
            }

            const QString ContentConfiguration::cloudSavesId() const
            {
                return QString("%1_%2").arg(masterTitleId()).arg(multiplayerId());
            }

            Services::Variant ContentConfiguration::getProductOverrideValue(const QString& overrideSettingBaseName) const
            {
                return m_catalogDef->getProductOverrideValue(overrideSettingBaseName);
            }

            bool ContentConfiguration::hasExpiration() const
            {
                ORIGIN_LOG_DEBUG << "TODO: implement bool ContentConfiguration::hasExpiration() const";
                return QDateTime::currentDateTimeUtc().secsTo(originSubscriptionUseEndDate()) < HAS_EXPIRATION_THRESHOLD_SECS;
            }

            bool ContentConfiguration::isEntitledFromSubscription() const
            {
                using Origin::Services::Publishing::NucleusEntitlement;

                const QString debug = Services::readSetting(Services::SETTING_SubscriptionEntitlementOfferId);
                if(QString::compare(debug, productId(), Qt::CaseInsensitive) == 0)
                {
                    return true;
                }

                return NucleusEntitlement::ExternalTypeSubscription == m_nucleusEntitlement->externalType() ||
                    (Services::Publishing::DISABLED == m_nucleusEntitlement->status()  &&
                     NucleusEntitlement::StatusReasonCodeSubscriptionExpired == m_nucleusEntitlement->statusReasonCode());
            }
        } //namespace Content
    } //namespace Engine
}//namespace Origin

static QString replaceOverriddenParts(const QString& original, const QString& key, const QString& file)
{
    // Neither are overridden, we can pass-through the default.
    if (key.isEmpty() && file.isEmpty())
    {
        return original;
    }

    // The general format of the hybrid path is [Key]Path where the key is a
    // lookup in the registry or app bundle to find the full path to the
    // install directory. The path is then a relative path appended to that to
    // give the absolute path to a specific file.
    QString originalFile = "";
    QString originalRegKey = "";
	QStringList originalList = original.split("]");

    if (originalList.length() >= 1)
	{
		originalRegKey = originalList[0];
		originalRegKey += ']';
	}

    if (originalList.length() >= 2)
		originalFile = originalList[1];

    // If the first part is overridden, use it instead of the default.
    QString overriddenPath = (!key.isEmpty()) ? key : originalRegKey;

    // Parsers of hybrid paths expect the key to be in brackets.
    if (!overriddenPath.startsWith("["))
        overriddenPath.prepend("[");
    if (!overriddenPath.endsWith("]"))
        overriddenPath.append("]");

    // Attach the default or overridden relative path, giving us a complete
    // hybrid path.
    overriddenPath += ((!file.isEmpty()) ? file : originalFile);

    return overriddenPath;
}
