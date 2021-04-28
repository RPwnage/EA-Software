#include "OriginConfigService.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "services/connection/ConnectionStatesService.h"
#include "version/version.h"
#include "services/log/LogService.h"
#include "TelemetryAPIDLL.h"


#include "services/network/NetworkAccessManager.h"
#include "services/crypto/CryptoService.h"
#include "services/log/LogService.h"
#include "services/platform/PlatformService.h"

#include <QMutexLocker>

namespace
{

    QString normalize_environment(QString env)
    {
        // first ensure lower case
        QString lower = env.toLower();

        // replace any dots or dashes with underscores
        lower = lower.replace(".", "_");
        lower = lower.replace("-", "_");

        return lower;
    }

    /// \brief Reports success or failure telemetry surrounding configuration download.
    /// If the configuration download succeeded, we report the time (in milliseconds) that
    /// it took to download the configuration. Otherwise, we report additional info 
    /// surrounding the error.
    void reportConfigDownloadTelemetry(const bool configDownloadSuccess, const QString& infoString)
    {
        QString telemReason;
        QString telemInfo;
        if (configDownloadSuccess)
        {
            telemReason = "configServiceSuccess";
        }
        else
        {
            telemReason = "configServiceFailed";
            telemInfo = "Bootstrap Failed to Download Dynamic Config.";
        }

        int infoIndex = infoString.indexOf(':');
        bool paramsExist = infoIndex > -1;

        if (paramsExist)
        {
            // Subtract an additional character from the error info string to
            // remove the initial delimiter (':')
            int errorInfoLength = infoString.size() - infoIndex - 1;
            telemInfo = infoString.right(errorInfoLength);
        }
        GetTelemetryInterface()->Metric_ERROR_NOTIFY(telemReason.toLocal8Bit().constData(), telemInfo.toLocal8Bit().constData());
    }

}

namespace Origin
{

namespace URI
{

QString get(QString const& key)
{
    return Services::OriginConfigService::instance()->serviceUrl(key.toLower());
}

}

namespace Services
{

// We want to wait until client has been up and running for 2 minutes before we attempt to download. Gives plenty of time
// for whatever network condition that caused bootstrap to fail to clear up.
const int DELAY_BEFORE_FIRST_CONFIG_DOWNLOAD_ATTEMPT = (2*60*1000);

// Let's only try once every 2 hours or so.  The first attempt is after client startup and if it fails it'll wait this long
// before trying again.
const int DELAY_BETWEEN_CONFIG_DOWNLOAD_ATTEMPTS = (2*60*60*1000);

OriginConfigService* OriginConfigService::sInstance = NULL;

bool OriginConfigService::init()
{
    QString environmentName = Origin::Services::readSetting(Origin::Services::SETTING_EnvironmentName);
    bool production = (0 == environmentName.compare(env::production, Qt::CaseInsensitive));

    sInstance = new OriginConfigService(production);
    sInstance->mIsConfigOk = sInstance->parseConfiguration() || production;
    if (sInstance->mIsConfigOk)
    {
        Origin::Services::initializeURLSettings();
    }
    return sInstance->mIsConfigOk;
}

void OriginConfigService::release()
{
    delete sInstance;
    sInstance = NULL;
}

OriginConfigService* OriginConfigService::instance()
{
    return sInstance;
}

OriginConfigService::OriginConfigService(bool production)
    : 
     mConfigured(false)
    ,mDownloadConfigFailed(false)
    ,mConfigurationError(NoError)
    ,mIsConfigOk(production)
{
    // Do not set dynamic config defaults here...for example, attempts to initialize members of mConfig.TelemetryConfig or mConfig.ECommerceConfig will not be respected.
    // They will be replaced by the SrvProtocolSchema.xsd defaults during parsing.  So all defaults should be maintained in SrvProtocolSchema.xsd.  Config defaults that
    // cannot be set in the .xsd should be set in the else block (config file processing failed) toward the bottom of OriginConfigService::parseConfiguration().
    
    mServiceUrls.clear();

    if (production)
    {
        mServiceUrls[URI::achievementConfigurationURL.toLower()]="https://download.dm.origin.com/MyOrigin/achieve.json";
        mServiceUrls[URI::achievementServiceURL.toLower()]="https://achievements.gameservices.ea.com/achievements";
        mServiceUrls[URI::achievementSharingURL.toLower()] = "https://gateway.ea.com/proxy/identity/pids/%1/privacysettings";
        mServiceUrls[URI::timedTrialServiceURL.toLower()] = "https://gateway.int.ea.com/proxy/access";
        mServiceUrls[URI::activationURL.toLower()]="https://checkout.origin.com/lb-ui/redeemcode/startRedeemCode";
        mServiceUrls[URI::avatarURL.toLower()]="https://avatar.dm.origin.com";
        mServiceUrls[URI::changePasswordURL.toLower()]="https://web.dm.origin.com/web/changePassword";
        mServiceUrls[URI::chatURL.toLower()]="https://chat.dm.origin.com";
        mServiceUrls[URI::clientConfigURL.toLower()]="http://www.dm.origin.com/ClientConfig";
        mServiceUrls[URI::cloudSavesBlacklistURL.toLower()]="https://secure.download.dm.origin.com/cloudsync/blacklist.txt";
        mServiceUrls[URI::cloudSyncAPIURL.toLower()]="https://cloudsync.dm.origin.com/cloudsync";
        mServiceUrls[URI::customerSupportHomeURL.toLower()]="https://help.ea.com/{loc2letter}/origin/origin";
        mServiceUrls[URI::customerSupportURL.toLower()]="https://help.ea.com/sso/login?code={code}&returnUrl=/{loc2letter}/origin/origin";
        mServiceUrls[URI::dirtyBitsHostName.toLower()]="dirtybits.dm.origin.com";
        mServiceUrls[URI::dlgFriendSearchURL.toLower()]="https://web.dm.origin.com/web/newImportInviteFriends";
        mServiceUrls[URI::ecommerceV1URL.toLower()]="https://ecommerce.dm.origin.com/ecommerce";
        mServiceUrls[URI::ecommerceV2URL.toLower()]="https://ecommerce2.dm.origin.com/ecommerce2";
        mServiceUrls[URI::ebisuCustomerSupportChatURL.toLower()]="https://customersupport.ea.com/lpchat/chat.do";
        mServiceUrls[URI::ebisuForumURL.toLower()]="http://answers.ea.com/t5/Origin/bd-p/O-2";
        mServiceUrls[URI::ebisuForumSSOURL.toLower()]="https://help.ea.com/sso/login?code={code}&communityUrl=/t5/Origin/bd-p/O-2/&WWCETokenType=OriginClient";
        mServiceUrls[URI::ebisuForumFranceURL.toLower()]="http://answers.ea.com/t5/Origin-Francais/bd-p/Origin-FR";
        mServiceUrls[URI::ebisuForumFranceSSOURL.toLower()]="https://help.ea.com/sso/login?code={code}&communityUrl=/t5/Origin-Francais/bd-p/Origin-FR/&WWCETokenType=OriginClient";
        mServiceUrls[URI::ebisuLockboxURL.toLower()]="https://origin.checkout.ea.com/lockbox-ui/checkout";
        mServiceUrls[URI::ebisuLockboxV3URL.toLower()]="https://www.origin.com/store/odc/widget/checkout/empty/redirect/{profile}/{offers}?profile={profile}&auto-login=true";
        mServiceUrls[URI::emptyShelfURL.toLower()]="";
        mServiceUrls[URI::eulaURL.toLower()]="http://download.dm.origin.com/eula/pc/3/%1.html";
        mServiceUrls[URI::eulaURL_mac.toLower()]="http://download.dm.origin.com/eula/mac/1/%1.html";
        mServiceUrls[URI::feedbackPageURL.toLower()]="https://www.origin.com/%1/mac-trial-feedback";
        mServiceUrls[URI::forgotPasswordURL.toLower()]="http://connect.origin.com/account/reset-password";
        mServiceUrls[URI::freeGamesURLV2.toLower()]="https://www.origin.com/store/shop/free";
        mServiceUrls[URI::friendSearchURL.toLower()]="https://web.dm.origin.com/web/searchPeople";
        mServiceUrls[URI::friendsURL.toLower()]="https://friends.dm.origin.com";
        mServiceUrls[URI::gamerProfileURL.toLower()]="https://web.dm.origin.com/web/judgeProfile";
        mServiceUrls[URI::gamesServiceURL.toLower()]="https://atom.dm.origin.com/atom";
        mServiceUrls[URI::gnuURL.toLower()]="http://www.gnu.org/licenses/old-licenses/lgpl-2.1.%1.html";
        mServiceUrls[URI::gplURL.toLower()]="http://gpl.ea.com/origin.html";
        mServiceUrls[URI::heartbeatURL.toLower()]="http://heartbeat.dm.origin.com";
        mServiceUrls[URI::igoDefaultURL.toLower()]="http://www.google.com";
        mServiceUrls[URI::igoDefaultSearchURL.toLower()]="http://www.google.com/search?q=%1";
        mServiceUrls[URI::inicisSsoCheckoutBaseURL.toLower()]="https://checkout.origin.com/lb-ui/cartcheckout/payment";
        mServiceUrls[URI::loginRegistrationURL.toLower()]="https://loginregistration.dm.origin.com";
        mServiceUrls[URI::macMotdURL.toLower()]="https://secure.download.dm.origin.com/motd_mac/%1/%2.html";
        mServiceUrls[URI::motdURL.toLower()]="https://secure.download.dm.origin.com/motd/%1/%2.html";
        mServiceUrls[URI::netPromoterURL.toLower()]="https://atom.dm.origin.com/atom/netPromoter";
        mServiceUrls[URI::newUserExpURL.toLower()]="https://account.origin.com/cp-ui/avatar/index";
        mServiceUrls[URI::onTheHouseStoreURL.toLower()]="https://www.origin.com/store/free-games/on-the-house?intcmp={trackingParam}";
        mServiceUrls[URI::subscriptionStoreURL.toLower()] = "https://www.origin.com/store/origin-access";
        mServiceUrls[URI::subscriptionVaultURL.toLower()] = "https://www.origin.com/store/browse/origin-membership";
        mServiceUrls[URI::subscriptionFaqURL.toLower()] = "https://www.origin.com/OriginAcess/FAQ";
        mServiceUrls[URI::subscriptionInitialURL.toLower()] = "https://www.origin.com/store/origin-access/home";
        mServiceUrls[URI::subscriptionRedemptionURL.toLower()] = "https://thor.qa.preview.www.cms.origin.com/en-us/store/v1/subscription/checkout";// TODO: This need to be replaced with the actual URL once we get it
        mServiceUrls[URI::subscriptionTrialEligibilityServiceURL.toLower()] = "https://thor.qa.preview.www.cms.origin.com/%1/store/v1/subscriptionTrialEligible";// TODO: This need to be replaced with the actual URL once we get it
        mServiceUrls[URI::onTheHousePromoURL.toLower()]="https://promomanager.dm.origin.com/promo/promotionManager2";
        mServiceUrls[URI::originAccountURL.toLower()]="https://account.origin.com/cp-ui/aboutme/index";
        mServiceUrls[URI::pdlcStoreLegacyURL.toLower()]="https://web.dm.origin.com/web/ecommerce/showStore";
        mServiceUrls[URI::pdlcStoreURL.toLower()]="https://www.origin.com/store/addonstore/{masterTitleId}";
        mServiceUrls[URI::privacyPolicyURL.toLower()]="http://tos.ea.com/legalapp/WEBPRIVACY/US/%1/PC/";
        mServiceUrls[URI::promoURL.toLower()]="https://promomanager.dm.origin.com/promo/promotionManager2";
        mServiceUrls[URI::redeemCodeReturnURL.toLower()]="https://download.dm.origin.com/checkout/%1/%2.html";
        mServiceUrls[URI::registrationURL.toLower()]="https://web.dm.origin.com/web/registration";
        mServiceUrls[URI::releaseNotesURL.toLower()]="https://secure.download.dm.origin.com/releasenotes/%1/%2.html";
        mServiceUrls[URI::signInPortalBaseURL.toLower()] = "https://signin.ea.com";
        mServiceUrls[URI::steamSupportURL.toLower()] = "http://support.steampowered.com";
        mServiceUrls[URI::storeEntitleFreeGameURL.toLower()]="https://www.origin.com/store/v1/checkout/{userId}/{offerId}?authToken={authToken}";
        mServiceUrls[URI::storeInitialURLV2.toLower()]="https://www.origin.com/store/";
        mServiceUrls[URI::storeInitialURLV2_mac.toLower()]="https://www.origin.com/store/shop/mac";
        mServiceUrls[URI::storeMasterTitleURL.toLower()]="https://www.origin.com/store/buy/{masterTitleId}";
        mServiceUrls[URI::storeOrderHistoryURL]="https://account.origin.com/cp-ui/orderhistory/index";
        mServiceUrls[URI::accountSubscriptionURL] = "https://account.origin.com/cp-ui/subscription/index";
        mServiceUrls[URI::storeSecurityURL]="https://account.origin.com/cp-ui/security/index";
        mServiceUrls[URI::storePrivacyURL]="https://account.origin.com/cp-ui/privacy/index";
        mServiceUrls[URI::storePaymentAndShippingURL]="https://account.origin.com/cp-ui/paymentandshipping/index";
        mServiceUrls[URI::storeRedemptionURL]="https://account.origin.com/cp-ui/redemption/index";
        mServiceUrls[URI::storeProductURLV2.toLower()]="https://www.origin.com/store/sku/{offerId}";
        mServiceUrls[URI::supportURL.toLower()]="http://help.origin.com/";
        mServiceUrls[URI::originFaqURL.toLower()]="http://www.origin.com/%1/faq/";
        mServiceUrls[URI::toSURL.toLower()]="http://tos.ea.com/legalapp/WEBTERMS/US/%1/PC/";
        mServiceUrls[URI::updateURL.toLower()]="https://secure.download.dm.origin.com/autopatch/2/upgradeFrom/%1/%2/%3?platform=%4";
        mServiceUrls[URI::webDispatcherURL.toLower()]="https://www.origin.com/origin/web_dispatch";
        mServiceUrls[URI::webWidgetUpdateURL.toLower()] = "https://secure.download.dm.origin.com/widgets/{appVersion}/{widgetName}.wgt";
        mServiceUrls[URI::wizardURL.toLower()]="https://web.dm.origin.com/web/profileSetup";        
        mServiceUrls[URI::connectPortalBaseURL.toLower()] = "https://accounts.ea.com";
        mServiceUrls[URI::identityPortalBaseURL.toLower()] = "https://gateway.ea.com/proxy";
        mServiceUrls[URI::atomServiceURL.toLower()] = "https://atom.dm.origin.com/atom";
        mServiceUrls[URI::COPPADownloadHelpURL.toLower()] = "";
        mServiceUrls[URI::COPPAPlayHelpURL.toLower()] = "";
        mServiceUrls[URI::OriginXURL.toLower()] = "https://dev.x.origin.com";

        if (Origin::Services::readSetting(Origin::Services::SETTING_OriginConfigDebug))
            mServiceUrls[URI::promoURL.toLower()] = "http://www.google.com";
        mServiceUrls[URI::voiceURL.toLower()] = "https://voice.dm.origin.com";
        mServiceUrls[URI::groupsURL.toLower()] = "https://groups.gameservices.ea.com";
    }

}

bool OriginConfigService::parseConfiguration()
{
    QMutexLocker locker(&mMutex);

    mConfigurationErrors = qMakePair(QString(), QString());
    mConfigured = processConfigFile();

    if ( mConfigured )
    {
        std::vector<server::URLT> const& urls = mConfig.ConnectionURLs.URLs.URLs;
        // if we have default URLs loaded, log errors if they do not match what came from the server
        bool check_keys = !mServiceUrls.isEmpty();
        if (check_keys && static_cast<int>(urls.size()) != mServiceUrls.size())
        {
            ORIGIN_LOG_ERROR << "Configuration contains wrong number of values";
        }

        for ( std::vector<server::URLT>::const_iterator i = urls.begin(); i != urls.end(); ++i )
        {
            if (check_keys && !mServiceUrls.contains(i->name.toLower()))
            {
                ORIGIN_LOG_ERROR << "Configuration contains an unrecognized key " << i->name;
            }

            mServiceUrls[i->name.toLower()] = i->value;
        }

        if(Services::readSetting(Services::SETTING_OverridesEnabled).toQVariant().toBool())
        {
            processOverrides();
        }
    }

    else
    {
        // if we failed to process the config file, set dynamic config fall-backs that cannot be set as defaults in SrvProtocolSchema.xsd
        mConfig.ClientAccessWhiteListUrls.url.push_back("http://opqa-online.rws.ad.ea.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("http://online.ea.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("http://www.battlefield.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("http://battlelog.battlefield.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("http://www.commandandconquer.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("http://www.battlefieldheroes.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("http://battlefield.play4free.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("http://world.needforspeed.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("http://www.ea.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("http://www.origin.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("https://www.origin.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("http://origin.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("http://store.origin.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("https://local.store.origin.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("https://approve.www.cms.origin.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("https://online.ea.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("https://www.easportsfifaworld.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("http://www.easportsfifaworld.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("http://fifaw.dev.play4free.ea.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("https://fifaw.dev.play4free.ea.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("http://battlelog.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("https://battlelog.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("http://cte.battlelog.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("https://cte.battlelog.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("http://cte.test.battlelog.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("https://cte.test.battlelog.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("http://battlefield.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("https://battlefield.com");
        mConfig.ClientAccessWhiteListUrls.url.push_back("https://billing.boacompra.com");

    }

    // URL overrides and hacks go here
    return mConfigured;
}

void OriginConfigService::processOverrides()
{
    // Process ECommerceConfig overrides
    QString dirtyBitsEnabledOverride = Services::readSetting(SETTING_OverrideRefreshEntitlementsOnDirtyBits);

    if(!dirtyBitsEnabledOverride.isEmpty())
    {
        mConfig.ECommerceConfig.refreshEntitlementsOnDirtyBits = (dirtyBitsEnabledOverride == "true" || dirtyBitsEnabledOverride.toInt() == 1);
    }
 
    int dirtyBitsRefreshOverride = Services::readSetting(SETTING_OverrideDirtyBitsEntitlementRefreshTimeout);

    if(dirtyBitsRefreshOverride > 0)
    {
        mConfig.ECommerceConfig.dirtyBitsEntitlementRefreshTimeout = dirtyBitsRefreshOverride;
    }
 
    int dirtyBitsTelemetryDelayOverride = Services::readSetting(SETTING_OverrideDirtyBitsTelemetryDelay);

    if(dirtyBitsTelemetryDelayOverride > 0)
    {
        mConfig.ECommerceConfig.dirtyBitsTelemetryDelay = dirtyBitsTelemetryDelayOverride;
    }

    int fullEntitlementRefreshThrottleOverride = Services::readSetting(SETTING_OverrideFullEntitlementRefreshThrottle);

    if(fullEntitlementRefreshThrottleOverride >= 0)
    {
        mConfig.ECommerceConfig.fullEntitlementRefreshThrottle = fullEntitlementRefreshThrottleOverride;
    }
 
    qint64 catalogDefinitionLookupTelemetryIntervalOverride = Services::readSetting(SETTING_OverrideCatalogDefinitionLookupTelemetryInterval).toQVariant().toLongLong();
    if(catalogDefinitionLookupTelemetryIntervalOverride > 0)
    {
        mConfig.ECommerceConfig.catalogDefinitionLookupTelemetryInterval = catalogDefinitionLookupTelemetryIntervalOverride;
    }

    qint64 catalogDefinitionLookupRetryIntervalOverride = Services::readSetting(SETTING_OverrideCatalogDefinitionLookupRetryInterval).toQVariant().toLongLong();
    if (catalogDefinitionLookupRetryIntervalOverride > 0)
    {
        mConfig.ECommerceConfig.catalogDefinitionLookupRetryInterval = catalogDefinitionLookupRetryIntervalOverride;
    }

    qint64 catalogDefinitionRefreshIntervalOverride = Services::readSetting(SETTING_OverrideCatalogDefinitionRefreshInterval).toQVariant().toLongLong();
    if (catalogDefinitionRefreshIntervalOverride >= 0)
    {
        mConfig.ECommerceConfig.catalogDefinitionRefreshInterval = catalogDefinitionRefreshIntervalOverride;
    }

    qint64 catalogDefinitionMaxAgeDaysOverride = Services::readSetting(SETTING_OverrideCatalogDefinitionMaxAgeDays).toQVariant().toLongLong();
    if (catalogDefinitionMaxAgeDaysOverride >= 0)
    {
        mConfig.ECommerceConfig.catalogDefinitionMaxAgeDays = catalogDefinitionMaxAgeDaysOverride;
    }
}

server::VersionT OriginConfigService::version() const
{
    QMutexLocker locker(&mMutex); 
    return mConfig.Version;
}

server::AchievementConfigT OriginConfigService::achievementsConfig() const
{
    QMutexLocker locker(&mMutex); 
    return mConfig.AchievementConfig;
}

server::MiscConfigT OriginConfigService::miscConfig() const 
{ 
    QMutexLocker locker(&mMutex); 
    return mConfig.MiscConfig; 
}

server::ClientAccessWhiteListUrlsT OriginConfigService::clientAccessWhiteListUrls() const 
{ 
    QMutexLocker locker(&mMutex); 
    return mConfig.ClientAccessWhiteListUrls; 
}

server::ECommerceConfigT OriginConfigService::ecommerceConfig() const 
{ 
    QMutexLocker locker(&mMutex); 
    return mConfig.ECommerceConfig; 
}

server::TelemetryConfigT OriginConfigService::telemetryConfig() const 
{ 
    QMutexLocker locker(&mMutex); 
    return mConfig.TelemetryConfig; 
}

server::ODTPluginT OriginConfigService::odtPlugin() const
{
    QMutexLocker locker(&mMutex); 
    return mConfig.ODTPlugin; 
}

server::SubscriptionConfigT OriginConfigService::subscriptionConfig() const
{
    QMutexLocker locker(&mMutex);
    return mConfig.SubscriptionConfig;
}

QString OriginConfigService::serviceUrl(QString const& key) const
{
    QMutexLocker locker(&mMutex); 
    return mServiceUrls[key.toLower()]; 
}

bool OriginConfigService::decryptConfigFileData(const QString& configFilePath, const QByteArray& encrypted, QByteArray& decrypted)
{
    QByteArray key = QByteArray::fromBase64("ePy+KACMX8MQ7GPCDAlx0w==");
    QByteArray base64 = QByteArray::fromBase64(encrypted);
    
    int ret = Origin::Services::CryptoService::decryptAES128(decrypted, base64, key);
    if(ret < 0)
    {
        mConfigurationErrors = qMakePair(QString("configServiceFailedDecryption"), QString("Rijndael error code: %1").arg(ret));
        mParsingError = QString("Failed to decrypt config service response: %1.  Rijndael error code: %2").arg(configFilePath).arg(ret);
        ORIGIN_LOG_ERROR << "Failed to decrypt configuration [" << configFilePath << "].  Rijndael error code [" << ret << "].  Falling back on hard-coded values if on production, otherwise exiting...";
        mConfigurationError = DecryptionFailedRijndaelError;
        return false;
    }
    else if (ret == 0)
    {
        mConfigurationErrors = qMakePair(QString("configServiceFailedDecryption"), QString("Failed to decrypt any data").arg(ret));

        mParsingError = QString("Failed to decrypt config service response: %1").arg(configFilePath);
        ORIGIN_LOG_ERROR << "Failed to decrypt configuration [" << configFilePath << "].  Falling back on hard-coded values if on production, otherwise exiting...";
        mConfigurationError = DecryptionFailedAnyDataError;
        return false;
    }
    else
    {
        return true;
    }
}

bool OriginConfigService::processConfigFile()
{
    QString env = normalize_environment(Origin::Services::readSetting(Origin::Services::SETTING_EnvironmentName).toString());
    QString configOverride = Origin::Services::readSetting(SETTING_OverrideInitURL, Services::Session::SessionRef());

    QString configFilePath;
    if (configOverride.isEmpty())
    {
        mConfigFilePath = Services::PlatformService::pathToConfigurationFile() + QDir::separator() + env + ".wad";
    }
    else
    {
        mConfigFilePath = configOverride.replace("{environment}", env);
    }
        
    QFile configFile(mConfigFilePath);

    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        mConfigurationError = ConfigFileIOError;
        return false;
    }

    QByteArray lump;
    QByteArray decrypted;

    QTextStream ts(&configFile);
    while (!ts.atEnd()) 
    {
        QString line = ts.readLine();
        lump.append(line);
    }
    configFile.close();

    if (configOverride.isEmpty() || !configOverride.endsWith(".xml"))
    {
        // Not using an override means we have to decrypt the data we just read
        if(!decryptConfigFileData(mConfigFilePath, lump, decrypted))
        {
            // We already logged the failures and updated the mConfigurationErrors in decryptConfigFileData, so just bail out.
            return false;
        }
    }
    else
    {
        decrypted = lump;
    }

#ifdef DEBUG
#if 0 // Enable for debugging config service responses
    QString filename = QString("%1configserviceresponse_%2.xml").arg(PlatformService::commonAppDataPath()).arg(Origin::Services::readSetting(Origin::Services::SETTING_EnvironmentName).toString().replace(".",""));
    QFile configDebug(filename);
    configDebug.open(QIODevice::ReadWrite | QIODevice::Truncate);
    configDebug.write(decrypted.data(), decrypted.size());
    configDebug.close();
#endif
#endif

    QDomDocument doc;

    QString errorMsg;
    int errLine = 0;
    int errColumn = 0;

    if(doc.setContent(decrypted, false, &errorMsg, &errLine, &errColumn))
    {
        LSXWrapper resp(doc, doc.firstChildElement());
        return server::Read(&resp, mConfig);
    }
    
    QString error = QString("Unable to parse XML: %1 | %2 | %3").arg(errColumn).arg(errLine).arg(errorMsg);
    error.chop(error.size() - 200);
    mConfigurationErrors = qMakePair(QString("configServiceFailedParse"), error);

    mParsingError = QString("Failed to parse config service response XML: %1.  ErrCol: %2.  ErrLine: %3.  ErrorMsg: %4").arg(mConfigFilePath).arg(errColumn).arg(errLine).arg(errorMsg);
    mParsingError.chop(mParsingError.size() - 200);
    mConfigurationError = ParsingError;
    errorMsg.chop(errorMsg.size() - 200);
    ORIGIN_LOG_ERROR << "Failed to parse configuration [" << mConfigFilePath << "].  ErrCol [" << errColumn << "].  ErrLine [" << errLine << "].  ErrorMsg [" << errorMsg << "].  Falling back on hard-coded values if on production, otherwise exiting...";
    return false;
}



void OriginConfigService::setDynamicConfigDownloadFailed(const QString& errorInfo)
{
    mDownloadConfigFailed = true;

    reportConfigDownloadTelemetry(false, errorInfo);

    mConfigurationError = DynamicConfigDownloadFailedError;
    if(Origin::Services::Connection::ConnectionStatesService::instance()->isGlobalStateOnline())
    {
        QTimer::singleShot(DELAY_BEFORE_FIRST_CONFIG_DOWNLOAD_ATTEMPT, this, SLOT(downloadConfig()));
    }
    else
    {
        ORIGIN_VERIFY_CONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(globalConnectionStateChanged(bool)),
            this, SLOT(onGlobalConnectionStateChanged(bool)));
    }
}

void OriginConfigService::reportBootstrapDownloadConfigSuccess(const QString& successInfo)
{
    reportConfigDownloadTelemetry(true, successInfo);
}


void OriginConfigService::onGlobalConnectionStateChanged(bool isOnline)
{
    if(isOnline && mDownloadConfigFailed)
    {
        // We don't care about this signal anymore, we try once and if we fail or go offline we reconnect.
        ORIGIN_VERIFY_DISCONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(globalConnectionStateChanged(bool)),
            this, SLOT(onGlobalConnectionStateChanged(bool)));
        downloadConfig();
    }
}

void OriginConfigService::downloadConfig()
{
    if(!Origin::Services::Connection::ConnectionStatesService::instance()->isGlobalStateOnline())
    {
        ORIGIN_VERIFY_CONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(globalConnectionStateChanged(bool)),
            this, SLOT(onGlobalConnectionStateChanged(bool)));
    }
    else
    {
        QString dynamicConfigURL;

        // convert to lower case and replace '.' with '-', it is valid to specify either FC-QA or fc.qa in EACore.ini
        QString environment = Origin::Services::readSetting(Origin::Services::SETTING_EnvironmentName).toString().toLower();
        environment = environment.replace(".", "-");

        if (environment.compare(Origin::Services::SETTING_ENV_production, Qt::CaseInsensitive) == 0)
        {
            dynamicConfigURL = QString("https://secure.download.dm.origin.com/production/autopatch/2/init/%1").arg(EALS_VERSION_P_DELIMITED);
        }
        else
        {
            dynamicConfigURL = QString("https://stage.secure.download.dm.origin.com/%1/autopatch/2/init/%2").arg(environment).arg(EALS_VERSION_P_DELIMITED); 
        }

        ORIGIN_LOG_EVENT << "Starting background download of dynamic config";

        QNetworkReply* downloadReply = Origin::Services::NetworkAccessManager::threadDefaultInstance()->get(QNetworkRequest(QUrl(dynamicConfigURL)));
        ORIGIN_VERIFY_CONNECT(downloadReply, SIGNAL(finished()), this, SLOT(onConfigDownloadFinished()));
    }
}

void OriginConfigService::onConfigDownloadFinished()
{
    QNetworkReply* downloadReply = dynamic_cast<QNetworkReply*>(sender());

    if(downloadReply != NULL)
    {
        if(downloadReply->error() == QNetworkReply::NoError &&
           downloadReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200)
        {
            QByteArray encrypted = downloadReply->readAll();
            QByteArray decrypted;

            // Make sure we don't have corrupted config data by attempting to decrypt the response.
            if(decryptConfigFileData("Server Response", encrypted, decrypted))
            {
                // Save to file on disk.
                QString env = normalize_environment(Origin::Services::readSetting(Origin::Services::SETTING_EnvironmentName).toString());
                QString configFilePath = Services::PlatformService::pathToConfigurationFile() + QDir::separator() + env + ".wad";
                QString stagedConfigFilePath = configFilePath + ".STAGED";

                QFile configFile(stagedConfigFilePath);
                if(configFile.open(QIODevice::WriteOnly))
                {
                    if(configFile.write(encrypted) == encrypted.length())
                    {                    
                        configFile.close();

                        // Remove any old cached dynamic config file
                        QFile::remove(configFilePath);
                        if(configFile.rename(configFilePath))
                        {
                            ORIGIN_LOG_EVENT << "Successfully downloaded dynamic config file in background.";
                            mDownloadConfigFailed = false;
                        }
                        else
                        {
                            ORIGIN_LOG_ERROR << "Failed to rename dynamic config file from [" << stagedConfigFilePath << "] to [" << configFilePath << "].";
                        }
                    }
                    else
                    {
                        ORIGIN_LOG_ERROR << "Failed to write config service response to disk.";
                    }
                }
                else
                {
                    ORIGIN_LOG_ERROR << "Failed to open staged config file for write.";
                }
            }
        }
        else
        {
            ORIGIN_LOG_EVENT << "Failed background download of dynamic config [error: " << downloadReply->error() << " statusCode: " << downloadReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() << "]";
        }
    }

    if(mDownloadConfigFailed)
    {
        QTimer::singleShot(DELAY_BETWEEN_CONFIG_DOWNLOAD_ATTEMPTS, this, SLOT(downloadConfig()));;
    }
}

void OriginConfigService::reportDynamicConfigLoadTelemetry()
{
    if (Origin::Services::OriginConfigService::instance()->isConfigOk())
    {
        // Capture telemetry for the OriginConfig.xml file
        QString configOverride = Origin::Services::readSetting(Origin::Services::SETTING_OverrideInitURL, Origin::Services::Session::SessionRef());
        GetTelemetryInterface()->Metric_APP_DYNAMIC_URL_LOAD(
            Origin::Services::OriginConfigService::instance()->version().file.toLocal8Bit().constData()
            ,Origin::Services::OriginConfigService::instance()->version().revision.toLocal8Bit().constData(),
            !configOverride.isEmpty());
    }

}


}

}
