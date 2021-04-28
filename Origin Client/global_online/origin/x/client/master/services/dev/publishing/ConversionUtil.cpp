
//    Copyright  2011-2012, Electronic Arts
//    All rights reserved.
//    Author: Hans van Veenendaal

#include <limits>
#include <QDateTime>
#include <QDomDocument>
#include <QDomNodeList>
#include <QTextStream>
#include <QFileInfo>
#include <QApplication>
#include <QMessageBox>
#include <QRegExp>

#include "services/platform/TrustedClock.h"
#include "services/publishing/ConversionUtil.h"
#include "services/log/LogService.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "LSXWrapper.h"
#include "ecommerce2.h"
#include "ecommerce2Reader.h"
#include "ecommerce2Writer.h"

#if ORIGIN_DEBUG
#define ENABLE_BOX_ART_TRACING 0
#else
#define ENABLE_BOX_ART_TRACING 0
#endif

namespace Origin
{

namespace Services
{

namespace Publishing
{

namespace ConversionUtil
{

QDateTime ConvertTime(OriginTimeT const& t)
{
    QDateTime parsedTime(QDate(t.wYear, t.wMonth, t.wDay), QTime(t.wHour, t.wMinute, t.wSecond, t.wMilliseconds));
    parsedTime.setTimeSpec(Qt::UTC);
    return parsedTime;
}

OriginTimeT ConvertTime(QDateTime const& t)
{
    OriginTimeT time;
    if(t.isValid() && (uint16_t)t.date().year() < 2500)
    {
        time.wYear = (uint16_t)t.date().year();
        time.wMonth = (uint16_t)t.date().month();
        time.wDay = (uint16_t)t.date().day();
        time.wDayOfWeek = (uint16_t)t.date().dayOfWeek();
        time.wHour = (uint16_t)t.time().hour();
        time.wMinute = (uint16_t)t.time().minute();
        time.wSecond = (uint16_t)t.time().second();
        time.wMilliseconds = 0;
    }
    else
    {
        time.wYear = 0;
        time.wMonth = 0;
        time.wDay = 0;
        time.wDayOfWeek = 0;
        time.wHour = 0;
        time.wMinute = 0;
        time.wSecond = 0;
        time.wMilliseconds = 0;
    }

    return time;
}

QString FormatPrice(double value, const QString& decimalPattern, const QString& orientation, const QString& symbol, const QString& currency)
{
    QString price, separator;
    QString modifiedDecimalPattern, modifiedOrientation;
    qint32 precision = 0;
    bool space = false;

    // EBIBUGS-19804
    // Euros are used by many different locales, and should be displayed differently based on locale.  However, the novacommerce
    // backend is configured per-currency, not per-locale.  This means that all currencies have the same formatting instructions.
    // In the short term, we need to account for that here, and display Euros differently based on locale.
    // Edit: it seems the same formatting issues apply for many European languages, such as Polish, Norwegian, and Danish...
    if (currency.compare("EUR", Qt::CaseInsensitive) == 0)
    {
        switch(QLocale().country())
        {
        case QLocale::Finland:
        case QLocale::Hungary:
        case QLocale::France:
        case QLocale::Poland:
        case QLocale::Denmark:
        case QLocale::Germany:
        case QLocale::Greece:
            {
                modifiedOrientation = "R";
                modifiedDecimalPattern = "0,00##";
                space = true;
                break;
            }

        case QLocale::Italy:
        case QLocale::CzechRepublic:
        case QLocale::Netherlands:
            {
                modifiedOrientation = "L";
                modifiedDecimalPattern = "0,00##";
                space = true;
                break;
            }

        default:
            {
                modifiedOrientation = "L";
                modifiedDecimalPattern = "0.00##";
                space = false;
                break;
            }
        }
    }
    else if (currency.compare("NOK", Qt::CaseInsensitive) == 0 ||
                currency.compare("PLN", Qt::CaseInsensitive) == 0 ||
                currency.compare("SEK", Qt::CaseInsensitive) == 0)
    {
        modifiedOrientation = "R";
        modifiedDecimalPattern = "0,00##";
        space = true;
    }
    else
    {
        modifiedOrientation = orientation;
        modifiedDecimalPattern = decimalPattern;
        space = false;
    }

    const QString decimalStr = modifiedDecimalPattern.split("#").front();
    separator = decimalStr;
    separator.remove("0", Qt::CaseInsensitive);
    precision = decimalStr.split(separator).at(1).length();

    price = QString::number(value, 'f', precision);
    price.replace(QRegExp("[.,]"), separator);
    QString priceFormat = (modifiedOrientation.compare("L", Qt::CaseInsensitive) == 0) ? "%1%2%3" : "%3%2%1";

    return priceFormat.arg(symbol).arg(space ? " " : "").arg(price);
}

static QHash<QString, QDomNode> whiteBlackListOverrides;

void CloudSaveWhiteListBlackListSetup()
{
    // Override Cloud Save White/Black list
    QFileInfo applicationPath(QCoreApplication::applicationFilePath());
    const QString whiteBlackListPath = applicationPath.absolutePath() + "/" + "WhiteBlackList.xml";

    QFile whiteBlackListFile(whiteBlackListPath);
    QDomDocument whiteBlackListXML;

    if (whiteBlackListFile.open(QIODevice::ReadOnly))
    {
        if(whiteBlackListXML.setContent(&whiteBlackListFile))
        {
            for (QDomElement overrideElement = whiteBlackListXML.documentElement().firstChildElement(); 
                !overrideElement.isNull(); 
                overrideElement = overrideElement.nextSiblingElement())
            {
                // This is the preferred way to do overrides as it can deal content IDs
                // that aren't valid XML names (such as numbers)
                if (overrideElement.tagName() == "override")
                {
                    QString contentId = overrideElement.attribute("contentId");
                    Q_ASSERT(!contentId.isNull());
                    whiteBlackListOverrides[contentId] = overrideElement;
                }
            }
        }
        else
        {
            QMessageBox::critical(NULL, "Unable to parse WhiteBlackList.xml", "Please check the xml format");
        }
    }
}

bool ConvertOfferPricing(const QJsonObject &srcData, OfferPricingData &pricingData)
{
    const double MINIMUM_OFFER_PRICE = 0.01;

    if (!srcData.contains("priceData"))
    {
        return false;
    }

    pricingData.isPurchasable = srcData["purchasable"].toBool();
    pricingData.blockUnderageUsers = srcData["blockUnderageUsers"].toBool();

    const QJsonObject &priceData = srcData["priceData"].toObject();
    double price = priceData["price"].toDouble();
    const QString &promotionTypes = priceData["promotionTypes"].toString();
    const QString &currency = priceData["currency"].toString();
    const QString &orientation = priceData["orientation"].toString();
    const QString &decimalPattern = priceData["decimalPattern"].toString();
    const QString &symbol = priceData["symbol"].toString();
    bool freeProduct = (price < MINIMUM_OFFER_PRICE);

    pricingData.isFreeProduct = freeProduct;
    if (freeProduct)
    {
        pricingData.currentPrice = QObject::tr("ebisu_client_free");
    }
    else
    {
        pricingData.currentPrice = FormatPrice(price, decimalPattern, orientation, symbol, currency);
        if (currency.compare("_BW", Qt::CaseInsensitive) == 0)
        {
            pricingData.priceDescription = QObject::tr("ebisu_client_bioware_points");
        }
        else if (currency.compare("_FF", Qt::CaseInsensitive) == 0)
        {
            pricingData.priceDescription = QObject::tr("ebisu_client_fifa_points");
        }
    }

    double originalPrice = priceData["originalPrice"].toDouble(price);
    if (originalPrice > price)
    {
        if (originalPrice < MINIMUM_OFFER_PRICE)
        {
            pricingData.originalPrice = QObject::tr("ebisu_client_free");
        }
        else
        {
            pricingData.originalPrice = FormatPrice(originalPrice, decimalPattern, orientation, symbol, currency);
        }
    }

    return true;
}

Origin::Services::PlatformService::OriginPlatform ConvertPlatform(const QString& platformString)
{
    QVector<Origin::Services::PlatformService::OriginPlatform> platforms = PlatformService::supportedPlatformsFromCatalogString(platformString);

    if(platforms.empty())
    {
        return Origin::Services::PlatformService::PlatformUnknown;
    }
    else
    {
        return platforms.front();
    }
}

ItemSubType ConvertGameDistributionSubType(const QString& productId, const QString& type)
{
    if (type.compare("Alpha", Qt::CaseInsensitive) == 0)
    {
        return Publishing::ItemSubTypeAlpha;
    }
    else if (type.compare("Beta", Qt::CaseInsensitive) == 0)
    {
        return Publishing::ItemSubTypeBeta;
    }
    else if (type.compare("Free Weekend Trial", Qt::CaseInsensitive) == 0)
    {
        return Publishing::ItemSubTypeFreeWeekendTrial;
    }
    else if (type.compare("Limited Free Weekend Trial", Qt::CaseInsensitive) == 0)
    {
        return Publishing::ItemSubTypeLimitedWeekendTrial;
    }
    else if (type.compare("Limited Trial", Qt::CaseInsensitive) == 0)
    {
        return Publishing::ItemSubTypeLimitedTrial;
    }
    // There are two sub types for timed trial. They are used for being to tell the difference on the store and minor UI places.
    // Access - is for subscription members only.
    // Game Time - the new kind of game time. Used to be "Limited Trial".
    else if (type.compare("Gameplay Timer Trial - Game Time", Qt::CaseInsensitive) == 0)
    {
        return Publishing::ItemSubTypeTimedTrial_GameTime;
    }
    else if (type.compare("Gameplay Timer Trial - Access", Qt::CaseInsensitive) == 0)
    {
        return Publishing::ItemSubTypeTimedTrial_Access;
    }
    else if (type.isEmpty() || type.compare("Normal Game", Qt::CaseInsensitive) == 0 || type.compare("3PDD Only", Qt::CaseInsensitive) == 0)
    {
        return Publishing::ItemSubTypeNormalGame;
    }
    else if (type.compare("Demo", Qt::CaseInsensitive) == 0)
    {
        return Publishing::ItemSubTypeDemo;
    }
    else if (type.compare("On the House", Qt::CaseInsensitive) == 0)
    {
        return Publishing::ItemSubTypeOnTheHouse;
    }
    else
    {
        ORIGIN_LOG_WARNING << "For offer [" << productId << "] found unexpected gameDistributionSubTypeValue [" << type << "] defaulting to ItemSubTypeNormalGame.";
        return Publishing::ItemSubTypeNormalGame;
    }
}

OriginDisplayType ConvertOriginDisplayType(const QString& type, bool isDownloadable)
{
    if ("Full Game" == type)
    {
        return OriginDisplayTypeFullGame;
    }
    else if ("Full Game Plus Expansion" == type)
    {
        return OriginDisplayTypeFullGamePlusExpansion;
    }
    else if ("Expansion" == type)
    {
        return OriginDisplayTypeExpansion;
    }
    else if ("Addon" == type)
    {
        return OriginDisplayTypeAddon;
    }
    else if ("None" == type)
    {
        return OriginDisplayTypeNone;
    }
    else if ("Game Only" == type)
    {
        return OriginDisplayTypeGameOnly;
    }
    else
    {
        if (!type.isEmpty())
        {
            ORIGIN_LOG_ERROR << "Unrecognized originDisplayType: " << type;
        }

        if (isDownloadable)
        {
            return OriginDisplayTypeAddon;
        }
        else
        {
            return OriginDisplayTypeGameOnly;
        }
    }
}

QString ConvertExtraContentDisplayGroup(const QString& extraContentDisplayGroup)
{
    // Not using enumerations because DGI wants the flexibility to add/modify
    // extracontent display groups out-of-cycle.
    QString group("");

    if (!extraContentDisplayGroup.isEmpty() && extraContentDisplayGroup != "No ExtraContent Display Group")
    {
        group = extraContentDisplayGroup;

        // HTML doesn't support spaces in IDs, so let's make the key HTML-friendly.
        group.replace(' ', '_');
    }

    return group;
}

void ConvertSoftwareControlDates(const QJsonObject& softwareControlDatesObj, CatalogDefinitionRef& catalogDef)
{
    const QJsonArray &softwareControlDates = softwareControlDatesObj["softwareControlDate"].toArray();
    QString geoCountry = Origin::Services::readSetting(Origin::Services::SETTING_GeoCountry);
    QString platformStr;

    // Extract information for each platform
    foreach (PlatformService::OriginPlatform platform, catalogDef->platformsSupported())
    {
        switch(platform)
        {
        case PlatformService::PlatformMacOS:
            platformStr = "MAC";
            break;

        case PlatformService::PlatformWindows:
            platformStr = "PCWIN";
            break;

        default:
            ORIGIN_LOG_WARNING << "Unrecognized supported platform in ConvertSoftwareControlDates.";
            platformStr = "PCWIN";
            break;
        }

        QJsonObject softwareControlForPlatform;
        foreach (const QJsonValue &controlDateVal, softwareControlDates)
        {
            const QJsonObject &controlDate = controlDateVal.toObject();
            if (platformStr == controlDate["platform"].toString())
            {
                softwareControlForPlatform = controlDate;
                break;
            }
        }

        if (softwareControlForPlatform.isEmpty())
        {
            ORIGIN_LOG_ERROR << "No software control dates found for supported platform " << platformStr << " for offer [" << catalogDef->productId() << "]";
            continue;
        }

        QDateTime releaseDate = QDateTime::fromString(softwareControlForPlatform["releaseDate"].toString(), Qt::ISODate);
        QDateTime downloadStartDate = QDateTime::fromString(softwareControlForPlatform["downloadStartDate"].toString(), Qt::ISODate);
        QDateTime useEndDate = QDateTime::fromString(softwareControlForPlatform["useEndDate"].toString(), Qt::ISODate);
        QDateTime originSubscriptionUseEndDate = QDateTime::fromString(softwareControlForPlatform["originSubscriptionUseEndDate"].toString(), Qt::ISODate);
        QDateTime useEndDateOverride;

        // Only override Use End Date related settings in non-Production environments
        const QString environment = Services::readSetting(Services::SETTING_EnvironmentName).toString().toLower();
        const QString endUseDateStr = catalogDef->getProductOverrideValue(Services::SETTING_OverrideUseEndDate.name());
        if (environment != SETTING_ENV_production && !endUseDateStr.isEmpty())
        {
            const QString dateFormat = "yyyy-MM-dd hh:mm:ss.z";
            useEndDateOverride = QDateTime::fromString(endUseDateStr, dateFormat);
            useEndDateOverride.setTimeSpec(Qt::UTC);
            ORIGIN_LOG_ERROR_IF(!useEndDateOverride.isValid()) <<
                "Use End Date override error for [" << catalogDef->productId() << "] - (" << endUseDateStr << "); " <<
                "Please check that the date is in this format (UTC TIME)--> yyyy-MM-dd hh:mm:ss.z";
        }

        SoftwareServerData &platformData = catalogDef->softwareServerData(platform);
        platformData.setReleaseDate(releaseDate.isValid() ? releaseDate : Publishing::CatalogDefinition::NULL_QDATE_TIME_FUTURE);
        platformData.setOriginSubscriptionUseEndDate(originSubscriptionUseEndDate.isValid() ? originSubscriptionUseEndDate : Publishing::CatalogDefinition::NULL_QDATE_TIME_FUTURE);
        if (catalogDef->downloadable())
        {
            platformData.setDownloadStartDate(downloadStartDate.isValid() ? downloadStartDate : Publishing::CatalogDefinition::NULL_QDATE_TIME_FUTURE);
            if (useEndDateOverride.isValid())
            {
                platformData.setUseEndDate(useEndDateOverride);
            }
            else
            {
                platformData.setUseEndDate(useEndDate);
            }
        }
    }
}

PackageType ConvertPackageType(const QString& packageType)
{
    if (packageType == "DownloadInPlace")
    {
        return PackageTypeDownloadInPlace;
    }
    else if (packageType == "Single")
    {
        return PackageTypeSingle;
    }
    else if (packageType == "Unpacked")
    {
        return PackageTypeUnpacked;
    }
    else if (packageType == "ExternalURL")
    {
        return PackageTypeExternalUrl;
    }
    else if (packageType == "OriginPlugin")
    {
        return PackageTypeOriginPlugin;
    }
    else if (packageType == "Unknown")
    {
        return PackageTypeUnknown;
    }
    else
    {
        ORIGIN_LOG_ERROR << "Unrecognized package type [" << packageType << "] defaulting to Single.";
        return PackageTypeSingle;
    }
}

void ConvertSoftware(const QJsonObject& software, SoftwareServerData& platformData, CatalogDefinitionRef& catalogDef)
{
    const QJsonObject &fulfillmentAttributes = software["fulfillmentAttributes"].toObject();

    platformData.setSoftwareId(software["softwareId"].toString());
    platformData.setAchievementSet(fulfillmentAttributes["achievementSetOverride"].toString());

     // Parse cloud save stuff
    QDomNode saveFileCriteriaNode;
    QDomDocument saveFileCriteriaDoc;

    //check for cloud save overrides
    if (whiteBlackListOverrides.contains(catalogDef->contentId()))
    {
        saveFileCriteriaNode = whiteBlackListOverrides[catalogDef->contentId()].firstChild();
    }
    else
    {
        if (!fulfillmentAttributes["cloudSaveConfigurationOverride"].toString().isEmpty() &&
            saveFileCriteriaDoc.setContent(fulfillmentAttributes["cloudSaveConfigurationOverride"].toString()))
        {
            saveFileCriteriaNode = saveFileCriteriaDoc.namedItem("saveFileCriteria");
        }
    }
        
    if(!saveFileCriteriaNode.isNull())
    {
        QString xmlString;
            
        QTextStream textStream(&xmlString);
        saveFileCriteriaNode.save(textStream, 0);
        platformData.setSaveFileCriteria(xmlString);
        platformData.setCloudSaveSupported(true);
    }

    const QString& commerceProfile = fulfillmentAttributes["commerceProfile"].toString();
    if (!commerceProfile.isEmpty())
    {
        platformData.setCommerceProfile(commerceProfile);
    }

    platformData.setPackageType(ConvertPackageType(fulfillmentAttributes["downloadPackageType"].toString()));

    platformData.setDownloadSize(fulfillmentAttributes["downloadSizeOverride"].toDouble());
    platformData.setExecuteParameters(fulfillmentAttributes["executeParams"].toString());
    platformData.setExecutePath(fulfillmentAttributes["executePathOverride"].toString());
    platformData.setInstallationDirectory(fulfillmentAttributes["installationDirectory"].toString());
    platformData.setInstallCheck(fulfillmentAttributes["installCheckOverride"].toString());
    platformData.setInstallerPath(fulfillmentAttributes["installerPath"].toString());
    platformData.setMetadataInstallLocation(fulfillmentAttributes["metadataInstallLocation"].toString());
    platformData.setUpdateNonDipInstall(fulfillmentAttributes["updateNonDipInstall"].toBool(false));
    platformData.setNetPromoterScoreBlacklist(fulfillmentAttributes["netPromoterScoreBlacklist"].toBool(false));
    platformData.setGameLauncherUrlClientId(fulfillmentAttributes["gameLauncherURLClientID"].toString());

    const QString envname = Origin::Services::readSetting(Services::SETTING_EnvironmentName, Services::Session::SessionRef());
    if (Services::SETTING_ENV_production == envname || fulfillmentAttributes["gameLauncherURL_Integration"].toString().isEmpty())
    {
        platformData.setGameLauncherUrl(fulfillmentAttributes["gameLauncherURL"].toString());
    }
    else
    {
        platformData.setGameLauncherUrl(fulfillmentAttributes["gameLauncherURL_Integration"].toString());
    }

    platformData.setAlternateLaunchSoftwareId(fulfillmentAttributes["alternateLaunchSoftwareId"].toString());

    if (catalogDef->isPDLC())
    {
        const QString &addonDeploymentStrategy = fulfillmentAttributes["addonDeploymentStrategy"].toString();
        if ("Hot Deployable" == addonDeploymentStrategy)
        {
            platformData.setAddonDeploymentStrategy(AddonDeploymentStrategyHotDeployable);
        }
        else if ("Reload Save" == addonDeploymentStrategy)
        {
            platformData.setAddonDeploymentStrategy(AddonDeploymentStrategyReloadSave);
        }
        else if ("Restart Game" == addonDeploymentStrategy)
        {
            platformData.setAddonDeploymentStrategy(AddonDeploymentStrategyRestartGame);
        }
        else
        {
            ORIGIN_LOG_WARNING_IF(Services::readSetting(Services::SETTING_LogCatalog).toQVariant().toBool()) << 
                "PDLC with product ID [" << catalogDef->productId() << "] does not have a valid addon deployment strategy [found: " << addonDeploymentStrategy << "].";
            platformData.setAddonDeploymentStrategy(AddonDeploymentStrategyRestartGame);
        }
    }

    platformData.setMonitorInstall(fulfillmentAttributes["monitorInstall"].toBool(true));
    platformData.setMonitorPlay(fulfillmentAttributes["monitorPlay"].toBool(true));
    platformData.setMultiplayerId(fulfillmentAttributes["multiPlayerId"].toString());

    const QString &oigClientBehavior = fulfillmentAttributes["oigClientBehavior"].toString().toLower();
    if ("blacklist" == oigClientBehavior)
    {
        platformData.setIgoPermission(IGOPermissionBlacklisted);
    }
    else if ("optin" == oigClientBehavior)
    {
        platformData.setIgoPermission(IGOPermissionOptIn);
    }
    else
    {
        platformData.setIgoPermission(IGOPermissionSupported);
    }

    const QString &partnerPlatform = fulfillmentAttributes["partnerPlatform"].toString();
    if ("GFWL" == partnerPlatform)
    {
        platformData.setPartnerPlatform(PartnerPlatformGamesForWindows);
    }
    else if ("Steam" == partnerPlatform)
    {
        platformData.setPartnerPlatform(PartnerPlatformSteam);
    }
    else if ("Other" == partnerPlatform)
    {
        platformData.setPartnerPlatform(PartnerPlatformOtherExternal);
    }
    else
    {
        platformData.setPartnerPlatform(PartnerPlatformNone);
    }

    const QString &processorArchitecture = fulfillmentAttributes["processorArchitecture"].toString();
    if ("32/64" == processorArchitecture)
    {
        platformData.setAvailableArchitectures(Origin::Services::PlatformService::ProcessorArchitecture3264bit);
    }
    else if ("64-bit" == processorArchitecture)
    {
        platformData.setAvailableArchitectures(Origin::Services::PlatformService::ProcessorArchitecture64bit);
    }
    else
    {
        platformData.setAvailableArchitectures(Origin::Services::PlatformService::ProcessorArchitecture32bit);
    }

    platformData.setShowKeyDialogOnInstall(fulfillmentAttributes["showKeyDialogOnInstall"].toBool());
    platformData.setShowKeyDialogOnPlay(fulfillmentAttributes["showKeyDialogOnPlay"].toBool());
    if (fulfillmentAttributes.contains("enableDLCuninstall"))
    {
        platformData.setEnableDLCUninstall(fulfillmentAttributes["enableDLCuninstall"].toBool());
    }
    platformData.setStagingKeyPath(fulfillmentAttributes["stagingKeyPath"].toString());
    platformData.setMacBundleId(fulfillmentAttributes["macBundleID"].toString());
    platformData.setIdentityClientIdOverride(fulfillmentAttributes["identityClientIdOverride"].toString());
    platformData.setShowSubsSaveGameWarning(fulfillmentAttributes["showSubsSaveGameWarning"].toBool(false));

    SoftwareBuildMap softwareMap;
    QPair<QString, QDateTime> liveBuild, stagedBuild;
    const QDateTime trustedNow = TrustedClock::instance()->nowUTC();
    const bool odtEnabled = readSetting(SETTING_OriginDeveloperToolEnabled).toQVariant().toBool();
    const bool isPlugin = platformData.packageType() == Services::Publishing::PackageTypeOriginPlugin;

    const QJsonArray &downloadURLs = software["downloadURLs"].toObject()["downloadURL"].toArray();
    foreach (const QJsonValue &downloadURLValue, downloadURLs)
    {
        const QJsonObject &downloadURL = downloadURLValue.toObject();
        const QString &downloadURLType = downloadURL["downloadURLType"].toString().toLower();
        SoftwareBuildType buildType;

        if ("live" == downloadURLType)
        {
            buildType = SoftwareBuildLive;
        }
        else if ("staged" == downloadURLType)
        {
            buildType = SoftwareBuildStaged;
        }
        else
        {
            buildType = SoftwareBuildOther;
        }

        // If ODT is not enabled and the offer is not a plug-in, ignore all but the current live and staged builds.
        if(buildType == SoftwareBuildOther && !odtEnabled && !isPlugin)
        {
            continue;
        }

        Services::Publishing::SoftwareBuildData buildData;
        buildData.setFileLink(downloadURL["downloadURL"].toString().trimmed());
        buildData.setLiveDate(QDateTime::fromString(downloadURL["effectiveDate"].toString(), Qt::ISODate));
        buildData.setBuildReleaseVersion(downloadURL["buildReleaseVersion"].toString());
        buildData.setNotes(downloadURL["buildReleaseNotes"].toString());
        buildData.setBuildId(buildData.liveDate().toString(Qt::ISODate));

        const QString &dataSource = QString("EC2:%1").arg(catalogDef->productId());
        const QString &buildMetadata = downloadURL["buildMetaData"].toString().trimmed();
        if (!buildMetadata.isEmpty() && buildData.buildMetadata().ParseSoftwareBuildMetadataBlock(buildMetadata, dataSource))
        {
            buildData.setBuildMetadataPresent(true);

            // If UseGameVersionFromManifestEnabled, ignore server build release version in favor of metadata game version.
            if (buildData.buildMetadata().mbUseGameVersionFromManifestEnabled)
            {
                buildData.setBuildReleaseVersion(buildData.buildMetadata().mGameVersion.ToStr());
            }
        }

        if (isPlugin && buildData.buildMetadataPresent())
        {
            // Ignore this build if it isn't compatible with the current client.
            const VersionInfo& requiredClientVersion = buildData.buildMetadata().mRequiredClientVersion;
            const VersionInfo& currentClientVersion = Services::PlatformService::currentClientVersion();
            if(requiredClientVersion != currentClientVersion)
            {
                continue;
            }

            // If it is compatible, update live/staged builds appropriately.
            if (buildData.liveDate() <= trustedNow)
            {
                // If the current build went live in the past but went live later than any other
                // past build, we flag it as live.
                if (buildData.liveDate() > liveBuild.second || liveBuild.second.isNull())
                {
                    liveBuild.first = buildData.buildId();
                    liveBuild.second = buildData.liveDate();
                }
            }
            else
            {
                // If the current build goes live in the future but goes live sooner than any other
                // future build, we flag it as staged.
                if (buildData.liveDate() < stagedBuild.second || stagedBuild.second.isNull())
                {
                    stagedBuild.first = buildData.buildId();
                    stagedBuild.second = buildData.liveDate();
                }
            }
        }
        else
        {
            switch (buildType)
            {
            case SoftwareBuildLive:
                liveBuild.first = buildData.buildId();
                liveBuild.second = buildData.liveDate();
                break;

            case SoftwareBuildStaged:
                stagedBuild.first = buildData.buildId();
                stagedBuild.second = buildData.liveDate();
                break;

            default:
                // Do nothing
                break;
            }
        }

        softwareMap.insert(buildData.buildId(), buildData);
    }

    if(!liveBuild.second.isNull())
    {
        SoftwareBuildData &buildData = softwareMap[liveBuild.first];

        buildData.setType(SoftwareBuildLive);
        platformData.setLiveBuildId(buildData.buildId());

        QString buildVersion = buildData.buildReleaseVersion().trimmed();
        if (buildVersion.isEmpty() || "0.0.0.0" == buildVersion)
        {
            buildVersion = "0";
        }

        platformData.setLiveBuildVersion(buildVersion);
    }

    if(!stagedBuild.second.isNull())
    {
        SoftwareBuildData &buildData = softwareMap[stagedBuild.first];

        buildData.setType(SoftwareBuildStaged);
        platformData.setStagedBuildId(buildData.buildId());

        QString buildVersion = buildData.buildReleaseVersion().trimmed();
        if (buildVersion.isEmpty() || "0.0.0.0" == buildVersion)
        {
            buildVersion = "0";
        }

        platformData.setStagedBuildVersion(buildVersion);
    }

    platformData.setSoftwareBuildMap(softwareMap);
}

bool ConvertPublishingData(const QJsonObject& publishing, CatalogDefinitionRef& catalogDef)
{
    const QJsonObject &pubAttributes = publishing["publishingAttributes"].toObject();
    if (pubAttributes.isEmpty())
    {
        ORIGIN_LOG_ERROR << "Failed to parse publishingAttributes for offer [" << catalogDef->productId() << "]";
        return false;
    }

    // Common publishing attribution
    const QString &cloudSaveContentIDFallback = pubAttributes["cloudSaveContentIDFallback"].toString();
    if (!cloudSaveContentIDFallback.isEmpty())
    {
        catalogDef->setCloudSaveContentIDFallback(cloudSaveContentIDFallback.split(","));
    }

    catalogDef->setContentId(pubAttributes["contentId"].toString());
    catalogDef->setGreyMarketControls(pubAttributes["greyMarketControls"].toBool());
    catalogDef->setDownloadable(pubAttributes["isDownloadable"].toBool());
    catalogDef->setPublished(pubAttributes["isPublished"].toBool(false));
    catalogDef->setUseLegacyCatalog(pubAttributes["useLegacyCatalog"].toBool());
    catalogDef->setForceKillAtOwnershipExpiry(pubAttributes["forceKillAtOwnershipExpiry"].toBool());
    catalogDef->setTwitchClientBlacklist(pubAttributes["twitchClientBlacklist"].toBool(false));
    catalogDef->setWatermarkDownload(pubAttributes["watermarkDownload"].toBool(false));

    const QString &publishedDate = pubAttributes["publishedDate"].toString();
    if (!publishedDate.isEmpty())
    {
        catalogDef->setPublishedDate(QDateTime::fromString(publishedDate, Qt::ISODate));
    }

    catalogDef->setTrialDuration(static_cast<int>(pubAttributes["trialLaunchDuration"].toDouble()));

    OriginDisplayType displayType = ConvertOriginDisplayType(pubAttributes["originDisplayType"].toString(), catalogDef->downloadable());
    catalogDef->setOriginDisplayType(displayType);

    // Business rule: full games and FGPE are not allowed to be preview content.
    bool isPreviewDownload = pubAttributes["isPreviewDownload"].toBool(false);
    if (isPreviewDownload && (OriginDisplayTypeFullGame == displayType || OriginDisplayTypeFullGamePlusExpansion == displayType))
    {
        ORIGIN_LOG_WARNING << "Detected full game or FGPE configured as preview download (" << catalogDef->productId() << ").  Ignoring preview download flag.";
        isPreviewDownload = false;
    }
    catalogDef->setPreviewContent(isPreviewDownload);

    catalogDef->setItemSubType(ConvertGameDistributionSubType(catalogDef->productId(), pubAttributes["gameDistributionSubType"].toString()));

    const QString &suppressedOfferIds = pubAttributes["suppressedOfferIds"].toString();
    if (!suppressedOfferIds.isEmpty())
    {
        catalogDef->setSuppressedOfferIds(suppressedOfferIds.split(",", QString::SkipEmptyParts));
    }

    ConvertSoftwareControlDates(publishing["softwareControlDates"].toObject(), catalogDef);

    const QJsonArray &softwareList = publishing["softwareList"].toObject()["software"].toArray();
    foreach (const QJsonValue &softwareValue, softwareList)
    {
        const QJsonObject &software = softwareValue.toObject();
        PlatformService::OriginPlatform platform = ConvertPlatform(software["softwarePlatform"].toString());

        if (catalogDef->softwareServerData(platform).platformEnabled())
        {
            ConvertSoftware(software, catalogDef->softwareServerData(platform), catalogDef);
        }
    }

    // Software locales
    QStringList supportedLocaleList;
    foreach (const QJsonValue &localeVal, publishing["softwareLocales"].toObject()["locale"].toArray())
    {
        const QString &locale = localeVal.toObject()["value"].toString();
        if (!locale.isEmpty())
        {
            supportedLocaleList.append(locale);
        }
    }
    catalogDef->setSupportedLocales(supportedLocaleList);

    return true;
}

void ConvertLocalizableAttributes(const QJsonObject &localizableAttributes, CatalogDefinitionRef &catalogDef)
{
    catalogDef->setDisplayName(localizableAttributes["displayName"].toString());
    catalogDef->setLongDescription(localizableAttributes["longDescription"].toString());
    catalogDef->setShortDescription(localizableAttributes["shortDescription"].toString());
    catalogDef->setGameLaunchMessage(localizableAttributes["gameLaunchMessage"].toString());
    catalogDef->setManualUrl(localizableAttributes["gameManualURL"].toString());
    catalogDef->setIGOBrowserDefaultURL(localizableAttributes["oigBrowserDefaultURL"].toString());
    catalogDef->setOriginSdkImage(localizableAttributes["originSdkImage"].toString());
    catalogDef->setBackgroundImage(localizableAttributes["backgroundImage"].toString());
    catalogDef->setPackArtSmall(localizableAttributes["packArtSmall"].toString());
    catalogDef->setPackArtMedium(localizableAttributes["packArtMedium"].toString());
    catalogDef->setPackArtLarge(localizableAttributes["packArtLarge"].toString());
    catalogDef->setPreAnnouncementDisplayDate(localizableAttributes["preAnnouncementDisplayDate"].toString());
    catalogDef->setExtraContentDisplayGroupDisplayName(localizableAttributes["extraContentDisplayGroupDisplayName"].toString());
}

bool ConvertOffer(const QJsonObject& offer, CatalogDefinitionRef& catalogDef)
{
    catalogDef->setProductId(offer["offerId"].toString());
    catalogDef->setItemId(offer["itemId"].toString());
    catalogDef->setFinanceId(offer["financeId"].toString());

    QString supportedPlatforms = offer["baseAttributes"].toObject()["platform"].toString();
    if (supportedPlatforms.isEmpty())
    {
        supportedPlatforms = "pc";
    }

    // Mark enabled platforms
    foreach (PlatformService::OriginPlatform platform, PlatformService::supportedPlatformsFromCatalogString(supportedPlatforms))
    {
        SoftwareServerData &platformData = catalogDef->softwareServerData(platform);
        platformData.setPlatformId(platform);
        platformData.setPlatformEnabled(true);
    }

    // Process publishing data
    ConvertPublishingData(offer["publishing"].toObject(), catalogDef);

    foreach (const QJsonValue &association, offer["associations"].toArray())
    {
        QStringList associatedProductIds;
        // There's a "sequenceNum" alongside the offer id - we should sort by that...
        foreach (const QJsonValue &offer, association.toObject()["offer"].toArray())
        {
            associatedProductIds.push_back(offer.toObject()["offerId"].toString());
            ORIGIN_LOG_DEBUG << "For offer [" << catalogDef->productId() << "] adding associated offer: " << associatedProductIds.back();
        }

        catalogDef->setAssociatedProductIds(associatedProductIds);
    }

    // MDM Hierarchy - get the mdm title Id and franchise Id
    QStringList alternateMasterTitleIds;
    foreach (const QJsonValue &mdmHierarchy, offer["mdmHierarchies"].toObject()["mdmHierarchy"].toArray())
    {
        const QJsonObject &hierarchy = mdmHierarchy.toObject();
        const QJsonObject &masterTitle = hierarchy["mdmMasterTitle"].toObject();
        const QString &masterTitleId = masterTitle["masterTitleId"].toVariant().toString();
        const QString &type = hierarchy["type"].toString().toLower();

        if ("primary" == type)
        {
            const QJsonObject &mdmFranchise = hierarchy["mdmFranchise"].toObject();

            catalogDef->setMasterTitleId(masterTitleId);
            catalogDef->setMasterTitle(masterTitle["masterTitle"].toString());
            catalogDef->setFranchiseId(mdmFranchise["franchiseId"].toVariant().toString());
        }
        else if (!masterTitleId.isEmpty())
        {
            alternateMasterTitleIds.push_back(masterTitleId);
        }
    }

    catalogDef->setAlternateMasterTitleIds(alternateMasterTitleIds);

    if (catalogDef->masterTitleId().isEmpty())
    {
        ORIGIN_LOG_ERROR << "No primary MDM hierarchy elements found for product: " << catalogDef->productId();
    }

    // Custom attributes
    const QJsonObject &customAttributes = offer["customAttributes"].toObject();
    catalogDef->setImageServer(customAttributes["imageServer"].toString());
    catalogDef->setRank(customAttributes["gameEditionTypeFacetKeyRankDesc"].toString().toInt());
    catalogDef->setSortOrderDescending(customAttributes["sortOrderDescending"].toDouble(-1));
    catalogDef->setExtraContentDisplayGroup(ConvertExtraContentDisplayGroup(customAttributes["extraContentDisplayGroup"].toString()));
    catalogDef->setSuppressVaultUpgrade(customAttributes["suppressVaultUpgrade"].toBool());

    int extraContentDisplayGroupSortAsc = customAttributes["extraContentDisplayGroupSortAsc"].toString().toInt();
    catalogDef->setExtraContentDisplayGroupSortOrderAscending(extraContentDisplayGroupSortAsc);

    // eCommerce attributes
    const QJsonObject &ecommerceAttributes = offer["ecommerceAttributes"].toObject();

    catalogDef->setCdnAssetRoot(ecommerceAttributes["cdnAssetRoot"].toString());
    catalogDef->setBlockUnderageUsers(ecommerceAttributes["blockUnderageUsers"].toBool());

    if (ecommerceAttributes.contains("isPurchasable"))
    {
        catalogDef->setPurchasable(ecommerceAttributes["isPurchasable"].toBool());
    }
    else
    {
        // TODO: Remove this block of code when CDN endpoint has been updated to respect country param in PROD.
        const QString& countryStr = Services::readSetting(Services::SETTING_CommerceCountry).toString();
        const QJsonArray &countryList = offer["countries"].toObject()["country"].toArray();
        foreach (const QJsonValue &countryValue, countryList)
        {
            const QJsonObject &country = countryValue.toObject();
            const QString &countryCode = country["countryCode"].toString();

            if (!countryCode.isEmpty())
            {
                if (countryCode.compare(countryStr, Qt::CaseInsensitive) == 0)
                {
                    catalogDef->setPurchasable(country["attributes"].toObject()["isPurchasable"].toBool());
                    break;
                }
                else if (countryCode.compare("DEFAULT", Qt::CaseInsensitive) == 0)
                {
                    catalogDef->setPurchasable(country["attributes"].toObject()["isPurchasable"].toBool());
                    // Don't break here because we may find the matching country value later in the list.
                }
            }
        }
    }

    // Alphas, Betas, and Demos cannot have child content, per https://developer.origin.com/support/browse/EBIBUGS-22753
    ItemSubType itemSubType = catalogDef->itemSubType();
    catalogDef->setAddonsAvailable(itemSubType != ItemSubTypeAlpha &&
        itemSubType != ItemSubTypeBeta &&
        itemSubType != ItemSubTypeDemo &&
        ecommerceAttributes["addonsAvailable"].toBool());

    if (catalogDef->addonsAvailable())
    {
        const QJsonArray &extraContentArray = ecommerceAttributes["availableExtraContent"].toArray();
        QStringList availableExtraContent;

        foreach (const QJsonValue &offerIdVal, extraContentArray)
        {
            availableExtraContent.append(offerIdVal.toString());
        }
        catalogDef->setAvailableExtraContent(availableExtraContent);
    }

    // Localizable attributes
    ConvertLocalizableAttributes(offer["localizableAttributes"].toObject(), catalogDef);

    ORIGIN_LOG_DEBUG_IF(ENABLE_BOX_ART_TRACING)
        << "XML: " << catalogDef->productId() 
        << "\n  bgi: " << catalogDef->backgroundImage()
        << "\n  sdk: " << catalogDef->originSdkImage()
        << "\n  pas: " << catalogDef->packArtSmall()
        << "\n  pam: " << catalogDef->packArtMedium()
        << "\n  pal: " << catalogDef->packArtLarge();

    // Any local overrides
    catalogDef->processPublishingOverrides();

    return true;
}

} // end ConversionUtil namespace
} // end Publishing namespace
} // end Services namespace
} // end Origin namespace

