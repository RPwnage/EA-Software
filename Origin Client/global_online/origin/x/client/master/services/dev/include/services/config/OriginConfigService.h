#ifndef ORIGINCONFIGSERVICE_H
#define ORIGINCONFIGSERVICE_H

#include <QObject>
#include <QMutex>
#include <QMap>
#include <QString>
#include <QPair>
#include <QDateTime>

#include "LSXWrapper.h"
#include "serverReader.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        enum ConfigurationError
        {
            NoError
            ,ConfigFileIOError
            ,ParsingError
            ,DecryptionFailedRijndaelError
            ,DecryptionFailedAnyDataError
            ,DynamicConfigDownloadFailedError
        };
        /** 
         * This class retrieves and stores configuration data from a static server side 
         * configuration file. It contains default values for all settings, and these
         * will apply if we fail to receive the configuration from the server. As far as
         * the API goes, this class will therefore never fail, it will always contain a valid
         * configuration.
         *
         * Unlike other services, the REST call is made by the bootstrapper, not the client.
         * The REST reply is passed from the bootstrapper to the client through shared memory.
         * There is therefore no REST call in the client for retrieving the configuration.
         */
        class ORIGIN_PLUGIN_API OriginConfigService : public QObject
        {
            Q_OBJECT
        public:

            static bool init();
            static void release();
            static OriginConfigService* instance();

            bool parseConfiguration();

            /// Make the server request for the config file
            void request();

            /// Return the version info, returned by value for thread safety
            server::VersionT version() const;
            
            /// Return the achievements configuration, returned by value for thread safety
            server::AchievementConfigT achievementsConfig() const;

            /// Return the misc configuration, returned by value for thread safety
            server::MiscConfigT miscConfig() const;

            /// Return the client access whitelist, returned by value for thread safety
            server::ClientAccessWhiteListUrlsT clientAccessWhiteListUrls() const;

            /// Return the eCommerce configuration, returned by value for thread safety
            server::ECommerceConfigT ecommerceConfig() const;

            /// Return the telemetry configuration, returned by value for thread safety
            server::TelemetryConfigT telemetryConfig() const;

            /// Return the ODT plugin configuration, returned by value for thread safety
            server::ODTPluginT odtPlugin() const;

            /// Return the subscription configuration, returned by value for thread safety
            server::SubscriptionConfigT subscriptionConfig() const;

            /// Return the service URL for the given service as defined in the URI namespace
            QString serviceUrl(QString const& key) const;

            /// Return the status of parseConfiguration
            bool configurationSuccessful() const { return mConfigured; }

            /// Return description of encountered error
            QPair<QString, QString> configurationErrors() const { return mConfigurationErrors; }
            
            /// Let's the config service know that the live fetch of config data failed, so we are 
            /// currently running on stale cached data from some previous run of the client.
            /// Once trigged, the client will attempt to download the config file itself in the background
            void setDynamicConfigDownloadFailed(const QString& errorInfo);

            /// \brief Reports the successful download of config data from the bootstrap to telemetry.
            void reportBootstrapDownloadConfigSuccess(const QString& successInfo);

            ConfigurationError configurationError() const { return mConfigurationError;}
            bool isConfigOk() const { return mIsConfigOk; }
            QString configFilePath() const { return mConfigFilePath; }
            QString parsingError() const { return mParsingError; }

            // reports telemetry about configuration service success.
            void reportDynamicConfigLoadTelemetry();

        private slots:
            void onGlobalConnectionStateChanged(bool isOnline);
            void onConfigDownloadFinished();
            void downloadConfig();

        private:
            OriginConfigService(bool production);
            
            /// Replaces config service responses with local overrides, if present.
            void processOverrides();

            bool processConfigFile();
            bool decryptConfigFileData(const QString& configFilePath, const QByteArray& encrypted, QByteArray& decrypted);

            static OriginConfigService* sInstance;

            QMutex mutable mMutex;
            server::OriginConfigT mConfig;
            // the server format for the service urls is not suitable for returning
            // individual urls efficiently, so we copy that data to a map
            QMap<QString, QString> mServiceUrls;

            bool mConfigured;
            QPair<QString, QString> mConfigurationErrors;
            bool mDownloadConfigFailed;

            QString mConfigFilePath;
            QString mParsingError;
            ConfigurationError mConfigurationError;
            bool mIsConfigOk;

        };
    }
    namespace URI
    {
        // return the URL for the given key, which should be one of the QStrings below
        ORIGIN_PLUGIN_API QString get(QString const& key);

        // these values must match the ones sent to us from the server in the name attribute e.g.:
        // <URL name = "ActivationURL" value="https://..." />.
        const static QString achievementConfigurationURL("AchievementConfigurationURL");
        const static QString achievementServiceURL("AchievementServiceURL");
        const static QString timedTrialServiceURL("TimedTrialServiceURL");
        const static QString subscriptionServiceURL("SubscriptionServiceURL");
        const static QString subscriptionVaultServiceURL("SubscriptionVaultServiceURL");
        const static QString subscriptionTrialEligibilityServiceURL("SubscriptionTrialEligibilityServiceURL");
        const static QString activationURL("ActivationURLV2");
        const static QString avatarURL("AvatarURL");
        const static QString changePasswordURL("ChangePasswordURL");
        const static QString chatURL("ChatURL");
        const static QString clientConfigURL("ClientConfigURL");
        const static QString cloudSavesBlacklistURL("CloudSavesBlacklistURL");
        const static QString cloudSyncAPIURL("CloudSyncAPIURL");
        const static QString customerSupportHomeURL("CustomerSupportHomeURL");
        const static QString customerSupportURL("CustomerSupportURL");
        const static QString dirtyBitsHostName("DirtyBitsServiceHostName");
        const static QString dlgFriendSearchURL("DlgFriendSearchURL");
        const static QString ecommerceV1URL("ECommerceV1URL");
        const static QString ecommerceV2URL("ECommerceV2URL");
        const static QString ebisuCustomerSupportChatURL("EbisuCustomerSupportChatURL");
        const static QString ebisuForumURL("EbisuForumURL");
        const static QString ebisuForumSSOURL("EbisuForumSSOURL");
        const static QString ebisuForumFranceURL("EbisuForumFranceURL");
        const static QString ebisuForumFranceSSOURL("EbisuForumFranceSSOURL");
        const static QString ebisuLockboxURL("EbisuLockboxURL");
        const static QString ebisuLockboxV3URL("EbisuLockboxV3URL");
        const static QString emptyShelfURL("EmptyShelf");
        const static QString eulaURL("EulaURL");
        const static QString eulaURL_mac("EulaURL_mac");
        const static QString feedbackPageURL("FeedbackPageURL");
        const static QString forgotPasswordURL("ForgotPasswordURL");
        const static QString freeGamesURLV2("FreeGamesURLV2");
        const static QString onTheHouseStoreURL("OnTheHouseStoreURL");
        const static QString subscriptionStoreURL("SubscriptionStoreURL");
        const static QString subscriptionVaultURL("SubscriptionVaultURL");
        const static QString subscriptionFaqURL("SubscriptionFaqURL");
        const static QString subscriptionInitialURL("SubscriptionInitialURL");
        const static QString subscriptionRedemptionURL("SubscriptionRedemptionURL");
        const static QString friendSearchURL("FriendSearchURL");
        const static QString friendsURL("FriendsURL");
        const static QString gamerProfileURL("GamerProfileURL");
        const static QString gamesServiceURL("GamesServiceURL");
        const static QString gnuURL("GNUURL");
        const static QString gplURL("GPLURL");
        const static QString heartbeatURL("HeartbeatURL");
        const static QString igoDefaultURL("IGODefaultURL");
        const static QString igoDefaultSearchURL("IGODefaultSearchURL");
        const static QString inicisSsoCheckoutBaseURL("InicisSsoCheckoutBaseURL");
        const static QString loginRegistrationURL("LoginRegistrationURL");
        const static QString motdURL("MotdURL");
        const static QString macMotdURL("MacMotdURL");
        const static QString netPromoterURL("NetPromoterURL");
        const static QString newUserExpURL("NewUserExpURL");
        const static QString onTheHousePromoURL("OnTheHousePromoURL");
        const static QString originAccountURL("OriginAccountURLV2");
        const static QString pdlcStoreLegacyURL("PdlcStoreV1URL");
        const static QString pdlcStoreURL("PdlcStoreV2URL");
        const static QString privacyPolicyURL("PrivacyPolicyURL");
        const static QString promoURL("PromoV2URL");
        const static QString redeemCodeReturnURL("RedeemCodeReturnURL");
        const static QString registrationURL("RegistrationURL");
        const static QString releaseNotesURL("ReleaseNotesURL");
        const static QString signInPortalBaseURL("SignInPortalBaseURL");
        const static QString steamSupportURL("SteamSupportURL");
        const static QString storeInitialURLV2("StoreInitialURLV2");
        const static QString storeInitialURLV2_mac("StoreInitialURLV2_mac");
        const static QString storeOrderHistoryURL("StoreOrderHistoryURLV2");
        const static QString storeSecurityURL("StoreSecurityURL");
        const static QString accountSubscriptionURL("AccountSubscriptionURL");
        const static QString storePrivacyURL("StorePrivacyURL");
        const static QString storePaymentAndShippingURL("StorePaymentAndShippingURL");
        const static QString storeRedemptionURL("StoreRedemptionURL");
        const static QString storeProductURLV2("StoreProductURLV2");
        const static QString storeMasterTitleURL("StoreMasterTitleURL");
        const static QString storeEntitleFreeGameURL("StoreEntitleFreeGameURL");
        const static QString supportURL("SupportURL");
        const static QString originFaqURL("OriginFaqURL");
        const static QString toSURL("ToSURL");
        const static QString updateURL("UpdateURL");
        const static QString webDispatcherURL("WebDispatcherURL");
        const static QString webWidgetUpdateURL("WebWidgetUpdateURL");
        const static QString wizardURL("WizardURL");
        const static QString connectPortalBaseURL("ConnectPortalBaseURL");
        const static QString identityPortalBaseURL("IdentityPortalBaseURL");
        const static QString atomServiceURL("AtomServiceURL");
        const static QString achievementSharingURL("AchievementSharingURL");
        const static QString COPPADownloadHelpURL("COPPADownloadHelpURL");
        const static QString COPPAPlayHelpURL("COPPAPlayHelpURL");
        const static QString voiceURL("VoiceURL");
        const static QString groupsURL("GroupsURL");
        const static QString OriginXURL ("OriginXURL");
    }
}

#endif // ORIGINCONFIGSERVICE_H
