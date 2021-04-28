//  SettingsManager.h
//  Copyright 2011-2012 Electronic Arts Inc. All rights reserved.

#ifndef _EBISUCOMMON_SETTINGSMANAGER_
#define _EBISUCOMMON_SETTINGSMANAGER_


#include <limits>
#include <QDateTime>
#include <QtCore>
#include <QString>
#include <QHash>

#include "Setting.h"
#include "services/session/SessionService.h"
#include "services/settings/Variant.h"
#include "services/plugin/PluginAPI.h"
#include <qnetworkcookie.h>

namespace Origin
{
    namespace Services
    {

        // URL environments
        namespace env
        {
          const static QString production("production");
          const static QString integration("integration");
          const static QString development("dev");
          const static QString qa_main("qa-main");
          const static QString qa("qa");
          const static QString ci("ci");
          const static QString fc_dev("fc.dev");
          const static QString fc_qa("fc.qa");
          const static QString fc_ci("fc.ci");
          const static QString hf_qa("hf.qa");
        }

        // TODO TS3_HARD_CODE_REMOVE
        namespace subs
        {
            const static QString Sims3MasterTitleId("55906");
        }


        namespace ServerUserSettings
        {
            class ServerUserSettingsManager;

            // Key names for the various server-side user settings
            // This needs to be defined in SettingsManager as it needs to build Setting instances from them 
            ORIGIN_PLUGIN_API extern const QString EnableIgoForAllGamesKey;
            ORIGIN_PLUGIN_API extern const QString EnableCloudSavesKey;
            ORIGIN_PLUGIN_API extern const QString EnableTelemetryKey;
            ORIGIN_PLUGIN_API extern const QString FavoriteProductIdsKey;
            ORIGIN_PLUGIN_API extern const QString HiddenProductIdsKey;
            ORIGIN_PLUGIN_API extern const QString BroadcastTokenKey;
			ORIGIN_PLUGIN_API extern const QString OthOfferKey;

            ORIGIN_PLUGIN_API extern const unsigned int ServerQueryTimeoutMs;
        }

        /// \brief read a setting
        /// \param settingName      the actual key of the setting
        /// \param session          the session req. only for a Scope::LocalPerUser setting
        ORIGIN_PLUGIN_API Variant readSetting(const QString &settingName, const Session::SessionRef session = Session::SessionRef());

        /// \brief read a setting
        /// \param setting      the actual setting
        /// \param session      the session req. only for a Scope::LocalPerUser setting
        ORIGIN_PLUGIN_API Variant readSetting(const Setting& setting, const Session::SessionRef session = Session::SessionRef());

        /// \brief read a setting -- to handle conversion of settings stored as QByteArray to Ascii (Qt4 used to convert that way but Qt5 converts to utf8
        /// \param setting      the actual setting
        /// \param session      the session req. only for a Scope::LocalPerUser setting
        ORIGIN_PLUGIN_API Variant readSettingAsAscii(const Setting& setting, const Session::SessionRef session = Session::SessionRef());

        /// \brief write a setting
        /// \param setting      the actual setting
        /// \param value        the value, if the value type does not match the setting type, an assert will fail
        /// \param session      the session req. only for a Scope::LocalPerUser setting
        ORIGIN_PLUGIN_API void writeSetting(const Setting& setting, Variant const& value, const Session::SessionRef session = Session::SessionRef());


        /// \brief delete a setting (currently not working for cloud settings aka. Setting::GlobalPerUser) 
        /// \param setting      the actual setting
        /// \param session      the session req. only for a Scope::LocalPerUser setting
        ORIGIN_PLUGIN_API void deleteSetting(const Setting& setting, const Session::SessionRef session = Session::SessionRef());


        /// \brief write the default value to the setting
        /// \param setting      the actual setting
        /// \param session      the session req. only for a Scope::LocalPerUser setting
        ORIGIN_PLUGIN_API void reset(const Setting& setting, const Session::SessionRef session = Session::SessionRef());

        /// \brief return true if the current value and default value of the given setting are equal
        /// \param setting      the actual setting
        /// \param session      the session req. only for a Scope::LocalPerUser setting
        ORIGIN_PLUGIN_API bool isDefault( const Setting& setting, const Session::SessionRef session = Session::SessionRef());

        /// \brief Parses initial raw data containing URLs.
        /// \param data The raw data containing URLs.
        ORIGIN_PLUGIN_API bool isParseURLRawData(const QString&);

        ORIGIN_PLUGIN_API bool parseURLSettings(const char* urlPayload, const QString& environmentName);

        /// \brief load/genereate all URLs
        ORIGIN_PLUGIN_API void initializeURLSettings();

        /// \brief update the default of all per login session settings.
        ORIGIN_PLUGIN_API void initializePerSessionSettings();

        /// \brief save the cookie to settings
        ORIGIN_PLUGIN_API void saveCookieToSetting (const Setting& setting, QNetworkCookie& cookie);

#ifdef DEBUG

        /// \brief returns data read from file (see code) to test parsing
        ORIGIN_PLUGIN_API QString fakeURLdata();
#endif

#define DECLARE_EACORE_SETTING(name) \
    ORIGIN_PLUGIN_API Setting SETTING_##name;
        
        /// \brief True if developer overrides are enabled
        extern const DECLARE_EACORE_SETTING(OverridesEnabled);

        /// \brief string that represents production envrionment
        ORIGIN_PLUGIN_API extern const QString SETTING_ENV_production;    // we now append the env name for some settings, e.g. autologin (except for production)

        /// \brief The size and position of the Ebisu app window.
        extern const DECLARE_EACORE_SETTING(APPSIZEPOSITION);

        /// \brief The screen number the client was on when the user logged out.
        extern const DECLARE_EACORE_SETTING(APPLOGOUTSCREENNUMBER);

        /// \brief The size and position of the Friends List window.
        extern const DECLARE_EACORE_SETTING(FRIENDSLISTWINDOWSIZEPOSITION);

        /// \brief The size and position of the the Chat Windows window.
        extern const DECLARE_EACORE_SETTING(CHATWINDOWSSIZEPOSITION);

        /// \brief User cache avatar image url
        extern const DECLARE_EACORE_SETTING(USERAVATARCACHEURL);

        /// \property SETTING_ACCEPTEDEULALOCATION
        /// \brief Override for the location of the EULA linked from the About dialog.
        extern const DECLARE_EACORE_SETTING(ACCEPTEDEULALOCATION);

        /// \property SETTING_ACCEPTEDEULAVERSION
        /// \brief Version number of EULA that was accepted by the user.
        extern const DECLARE_EACORE_SETTING(ACCEPTEDEULAVERSION);

        /// \property SETTING_AUTOPATCH.
        /// \brief Flag that determines whether Ebisu will automatically install patches.
        extern const DECLARE_EACORE_SETTING(AUTOPATCH);

		/// \property AUTOPATCHTIMESTAMP.
		/// \brief This is set to the epoch time (in seconds) when "Keep My Games Up To Date" setting is modified by the user.
		extern const DECLARE_EACORE_SETTING(AUTOPATCHTIMESTAMP);

        /// \property SETTING_AUTOPATCHGLOBAL.
        /// \brief Flag that determines whether Ebisu will automatically install patches. This is set in the Origin installer. It is the
        /// default for AUTOPATCH
        extern const DECLARE_EACORE_SETTING(AUTOPATCHGLOBAL);

        /// \property SETTING_INSTALLTIMESTAMP
        /// \brief This is set to the epoch time (in seconds) when Origin is first installed.
        extern const DECLARE_EACORE_SETTING(INSTALLTIMESTAMP);

        /// \property SETTING_AUTOUPDATE.
        /// \brief Flag that determines whether Ebisu will automatically self update.
        extern const DECLARE_EACORE_SETTING(AUTOUPDATE);


        /// \property SETTING_RUNONSYSTEMSTART.
        /// \brief Flag that determines whether Ebisu will automatically open on system start.
        extern const DECLARE_EACORE_SETTING(RUNONSYSTEMSTART);


        /// \property SETTING_BETAOPTIN
        /// \brief The flat that determines whether to automatically install beta versions of Ebisu.
        extern const DECLARE_EACORE_SETTING(BETAOPTIN);

		/// \property SETTING_VOICE_ENABLE
		/// \brief Flag to determine whether to force enable voice chat.
		extern const DECLARE_EACORE_SETTING(VoiceEnable);

        /// \property SETTING_DOWNLOAD_CACHEDIR
        /// \brief The directory to use for storing cached files.
        extern const DECLARE_EACORE_SETTING(DOWNLOAD_CACHEDIR);


        /// \property SETTING_DOWNLOAD_CACHEDIRREMOVAL
        /// \brief Flag that determines whether the cache directory is cleared.
        extern const DECLARE_EACORE_SETTING(DOWNLOAD_CACHEDIRREMOVAL);

        /// \property SETTING_COMMANDLINE
        /// \brief The command line used to invoke the Ebisu client.
        extern const DECLARE_EACORE_SETTING(COMMANDLINE);

        /// \property SETTING_FIRSTLAUNCH
        /// \brief Only actually used on Mac.  Initially true.  Set to false after the First Launch flow has run.
        extern const DECLARE_EACORE_SETTING(FIRSTLAUNCH);

        /// \property SETTING_FIRSTTIMESTARTED
        /// \brief The date and time the client was first run on this system.
        extern const DECLARE_EACORE_SETTING(FIRSTTIMESTARTED);

        /// \property SETTING_DOWNLOADINPLACEDIR
        /// \brief The directory to use for housing download-in-place content.
        extern const DECLARE_EACORE_SETTING(DOWNLOADINPLACEDIR);

        /// \property SETTING_EnableDownloaderSafeMode
        /// \brief The directory to use for housing download-in-place content.
        extern const DECLARE_EACORE_SETTING(EnableDownloaderSafeMode);

        /// \property SETTING_ENABLEINGAMELOGGING
        /// \brief Flag that determines whether logging takes place in IGO.
        extern const DECLARE_EACORE_SETTING(ENABLEINGAMELOGGING);


        /// \property SETTING_HIDEEXITCONFIRMATION
        /// \brief Flag that determines whether to ask the user to confirm when exiting the client.
        extern const DECLARE_EACORE_SETTING(HIDEEXITCONFIRMATION);


        /// \property SETTING_HIDEOFFLINELOGINMODEDUETOEASERVERSNOTICE
        /// \brief Flag that determines whether to notify the user when logging in off-line due to EA servers.
        extern const DECLARE_EACORE_SETTING(HIDEOFFLINELOGINMODEDUETOEASERVERSNOTICE);


        /// \property SETTING_HIDEOFFLINELOGINMODEDUETONETWORKNOTICE
        /// \brief Flag that determines whether to notify the user when logging in off-line due to network issues.
        extern const DECLARE_EACORE_SETTING(HIDEOFFLINELOGINMODEDUETONETWORKNOTICE);


        /// \property SETTING_INGAME_HOTKEY
        /// \brief The default IGO hot key to use.
        extern const DECLARE_EACORE_SETTING(INGAME_HOTKEY);


        /// \property SETTING_INGAME_HOTKEY
        /// \brief The string value of the OIG Hotkey
        extern const DECLARE_EACORE_SETTING(INGAME_HOTKEYSTRING);


        /// \property SETTING_INGAME_HOTKEY
        /// \brief The default IGO hot key to use.
        extern const DECLARE_EACORE_SETTING(PIN_HOTKEY);


        /// \property SETTING_INGAME_HOTKEY
        /// \brief The string value of the OIG Hotkey
        extern const DECLARE_EACORE_SETTING(PIN_HOTKEYSTRING);


        /// \property SETTING_EnableIgo
        /// \brief User flag that determines whether OIG is enabled.
        extern const DECLARE_EACORE_SETTING(EnableIgo);


        /// \property SETTING_ISBETA
        /// \brief The flag that determines if the last run version was a beta
        extern const DECLARE_EACORE_SETTING(ISBETA);


        /// \property SETTING_KEEPINSTALLERS
        /// \brief Flag that determines whether to keep the game installers on the local computer.
        extern const DECLARE_EACORE_SETTING(KEEPINSTALLERS);

        /// \property SETTING_LOCALHOSTENABLED
        /// \brief Flag that determines whether to have the local host running on Origin
        extern const DECLARE_EACORE_SETTING(LOCALHOSTENABLED);

        /// \property SETTING_LOCALHOSTRESPONDERENABLED
        /// \brief Flag that determines whether to have the local host responder service (Beacon) running on Origin
        extern const DECLARE_EACORE_SETTING(LOCALHOSTRESPONDERENABLED);

        /// \property SETTING_CREATEDESKTOPSHORTCUT
        /// \brief Flag of last user setting result for setting
        extern const DECLARE_EACORE_SETTING(CREATEDESKTOPSHORTCUT);

        /// \property SETTING_CREATESTARTMENUSHORTCUT
        /// \brief Flag of last user setting result for setting
        extern const DECLARE_EACORE_SETTING(CREATESTARTMENUSHORTCUT);

        /// \property SETTING_LOCALE
        /// \brief The user's language setting.
        extern DECLARE_EACORE_SETTING(LOCALE);  // not const, because the default value is not yet set!!!

        // \property SETTING_PromoBrowserOfferCache
        /// \brief The most recent list of offerIds owned by current user, for promo browser
        extern DECLARE_EACORE_SETTING(PromoBrowserOfferCache);
        ORIGIN_PLUGIN_API extern const QString PROMOBROWSEROFFERCACHE_NO_ENTITLEMENTS_SENTINEL;

        /// \property SETTING_OverridePromoBrowserOfferCache
        /// \brief Promo manager offer id list that has been overridden by the user (enabled with ODT entitlement only)
        extern DECLARE_EACORE_SETTING(OverridePromoBrowserOfferCache);       

        /// \property SETTING_StoreLocale
        /// \brief The most recent locale used by the store
        ///
        /// Note that this does not always correlate with SETTING_LOCALE because the store may not 
        /// be available in the client locale for the user's region.  For example, a user in Canada 
        /// would only ever see the store in French or English, even if the client locale is Korean.
        extern DECLARE_EACORE_SETTING(StoreLocale);


        /// \property SETTING_LOGIN_AS_INVISIBLE.
        /// \brief Flag that determines whether Ebisu will log in as invisible on startup.
        extern const DECLARE_EACORE_SETTING(LOGIN_AS_INVISIBLE);


        /// \property SETTING_LOGINEMAIL
        /// \brief The email address most recently used to log in.
        extern const DECLARE_EACORE_SETTING(LOGINEMAIL);


        /// \property SETTING_NOTIFY_FINISHEDDOWNLOAD
        /// \brief Flag that determines whether to notify the user when a download finishes.
        extern const DECLARE_EACORE_SETTING(NOTIFY_FINISHEDDOWNLOAD);


        /// \property SETTING_NOTIFY_DOWNLOADERROR
        /// \brief Flag that determines whether to notify the user when a download finishes.
        extern const DECLARE_EACORE_SETTING(NOTIFY_DOWNLOADERROR);


        /// \property SETTING_NOTIFY_FINISHEDINSTALL
        /// \brief Flag that determines whether to notify the user when an install finishes.
        extern const DECLARE_EACORE_SETTING(NOTIFY_FINISHEDINSTALL);


        /// \property SETTING_NOTIFY_FRIENDQUITSGAME
        /// \brief Flag that determines whether to notify the user when a friend quits a game.
        extern const DECLARE_EACORE_SETTING(NOTIFY_FRIENDQUITSGAME);


        /// \property SETTING_NOTIFY_FRIENDSIGNINGIN
        /// \brief Flag that determines whether to notify the user when a friend signs into the Ebisu service.
        extern const DECLARE_EACORE_SETTING(NOTIFY_FRIENDSIGNINGIN);


        /// \property SETTING_NOTIFY_FRIENDSIGNINGOUT
        /// \brief Flag that determines whether to notify the user when a friend signs out of the Ebisu service.
        extern const DECLARE_EACORE_SETTING(NOTIFY_FRIENDSIGNINGOUT);


        /// \property SETTING_NOTIFY_FRIENDSTARTSGAME
        /// \brief Flag that determines whether to notify the user when a friend starts a game.
        extern const DECLARE_EACORE_SETTING(NOTIFY_FRIENDSTARTSGAME);


        /// \property SETTING_NOTIFY_INCOMINGTEXTCHAT
        /// \brief Flag that determines whether to notify the user of incoming chat messages.
        extern const DECLARE_EACORE_SETTING(NOTIFY_INCOMINGTEXTCHAT);


        /// \property SETTING_NOTIFY_INVITETOCHATROOM
        /// \brief Flag that determines whether to notify the user of invitations to chat room.
        extern const Setting SETTING_NOTIFY_INVITETOCHATROOM;

        
         /// \property SETTING_NOTIFY_INCOMINGCHATROOMMESSAGE
        /// \brief Flag that determines whether to notify the user of incoming chat messages.
        extern const Setting SETTING_NOTIFY_INCOMINGCHATROOMMESSAGE;


        /// \property SETTING_NOTIFY_NONFRIENDINVITE
        /// \brief Flag that determines whether to notify the user of non friend invites.
        extern const DECLARE_EACORE_SETTING(NOTIFY_NONFRIENDINVITE);

        /// \property SETTING_NOTIFY_GAMEINVITE
        /// \brief Flag that determines whether to notify the user of game invites.
        extern const DECLARE_EACORE_SETTING(NOTIFY_GAMEINVITE);

        /// \property SETTING_NOTIFY_INCOMINGCECHAT
        /// \brief Flag that determines whether to notify the user of incoming CE chat messages.
        extern const DECLARE_EACORE_SETTING(NOTIFY_INCOMINGCECHAT);

        /// \property SETTING_NOTIFY_FRIENDSTARTBROADCAST
        /// \brief Flag that determines whether to notify the user their friends started broadcasting.
        extern const DECLARE_EACORE_SETTING(NOTIFY_FRIENDSTARTBROADCAST);

        /// \property SETTING_NOTIFY_FRIENDSTOPBROADCAST
        /// \brief Flag that determines whether to notify the user their friends stopped broadcasting.
        extern const DECLARE_EACORE_SETTING(NOTIFY_FRIENDSTOPBROADCAST);

        /// \property SETTING_NOTIFY_GROUPCHATINVITE
        /// \brief Flag that determines whether to notify the user they have received a group chat invite
        extern const DECLARE_EACORE_SETTING(NOTIFY_GROUPCHATINVITE);

        /// \property SETTING_NOTIFY_INCOMINGVOIPCALL
        /// \brief Flag that determines whether to notify the user they have received a VoIP call request
        extern const DECLARE_EACORE_SETTING(NOTIFY_INCOMINGVOIPCALL);

        /// \property SETTING_NOTIFY_ACHIEVEMENTUNLOCKED
        /// \brief Flag that determines whether to notify the user they have unlocked an achievement
        extern const DECLARE_EACORE_SETTING(NOTIFY_ACHIEVEMENTUNLOCKED);

        /// \property SETTING_OverrideRefreshEntitlementsOnDirtyBits
        /// \brief The number of milliseconds the client should wait before refreshing entitlements after a dirty bits entitlement updated notification.  Overrides value from config service.
        extern const DECLARE_EACORE_SETTING(OverrideRefreshEntitlementsOnDirtyBits);

        /// \property SETTING_OverrideDirtyBitsEntitlementRefreshTimeout
        /// \brief The number of milliseconds the client should wait before refreshing entitlements after a dirty bits entitlement updated notification.  Overrides value from config service.
        extern const DECLARE_EACORE_SETTING(OverrideDirtyBitsEntitlementRefreshTimeout);

        /// \property SETTING_OverrideDirtyBitsTelemetryDelay
        /// \brief The number of milliseconds the client should wait before sending telemetry after a dirty bits notification.  Overrides value from config service.
        extern const DECLARE_EACORE_SETTING(OverrideDirtyBitsTelemetryDelay);

        /// \property SETTING_OverrideFullEntitlementRefreshThrottle
        /// \brief The number of milliseconds the client will wait before allowing user to initiate a server based full refresh of entitlements.  Overrides value from config service.
        extern const DECLARE_EACORE_SETTING(OverrideFullEntitlementRefreshThrottle);

        /// \property SETTING_FakeDirtyBitsMessageFile
        /// \brief Points to a file on disk that contains a payload to deliver through standard dirty bits.  Used in conjection with debug fake dirty bit message from file menu item to test any DB payload in client.
        extern const DECLARE_EACORE_SETTING(FakeDirtyBitsMessageFile);

        /// \property SETTING_OverrideLoginPageRefreshTimer
        //// \brief How often the Login page refreshes. (In minutes)
        extern const DECLARE_EACORE_SETTING(OverrideLoginPageRefreshTimer);

        /// \property SETTING_PRODUCTINFO_MAX_IDS
        /// \brief The maximum number of product id's that can be requested in a product info rest call.
        extern const DECLARE_EACORE_SETTING(PRODUCTINFO_MAX_IDS);


        /// \property SETTING_PROMOBROWSER_ORIGINSTARTED_LASTSHOWN
        /// \brief The date the promo browser was last shown when Origin started.
        extern const DECLARE_EACORE_SETTING(PROMOBROWSER_ORIGINSTARTED_LASTSHOWN);


        /// \property SETTING_PROMOBROWSER_GAMEFINISHED_LASTSHOWN
        /// \brief The date the promo browser was last shown when a game shut down.
        extern const DECLARE_EACORE_SETTING(PROMOBROWSER_GAMEFINISHED_LASTSHOWN);


        /// \property SETTING_PROMOBROWSER_DOWNLOADUNDERWAY_LASTSHOWN
        /// \brief The date the promo browser was last shown when a download started.
        extern const DECLARE_EACORE_SETTING(PROMOBROWSER_DOWNLOADUNDERWAY_LASTSHOWN);

        /// \property SETTING_NETPROMOTER_LASTSHOWN
        /// \brief The date the net promoter was last shown.
        extern const DECLARE_EACORE_SETTING(NETPROMOTER_LASTSHOWN);

        /// \property SETTING_NETPROMOTER_LASTSHOWN
        /// \brief The date the game net promoter was last shown.
        extern const DECLARE_EACORE_SETTING(NETPROMOTER_GAME_LASTSHOWN);

        /// \property SETTING_ONTHEHOUSE_VERSIONS_SHOWN
        /// \brief List of versions (ids) of on the house offers user has closed.
        extern const DECLARE_EACORE_SETTING(ONTHEHOUSE_VERSIONS_SHOWN);

        /// \property OverrideOnTheHouseQueryIntervalMS
        /// \brief EACore override on the query interval time for QA to easily test
        extern const DECLARE_EACORE_SETTING(OverrideOnTheHouseQueryIntervalMS);

        /// \property SETTING_SUBSCRIPTION_ERROR_MESSAGE_LASTSHOWN
        /// \brief The date the subscription message area error was last shown.
        extern const DECLARE_EACORE_SETTING(SUBSCRIPTION_ERROR_MESSAGE_LASTSHOWN);


        /// \property SETTING_SELECTEDFILTER
        /// \brief The current filter setting in the 'My Games' screen.
        extern const DECLARE_EACORE_SETTING(SELECTEDFILTER);


        /// \property SETTING_3PDD_PLAYDIALOG_SHOW
        /// \brief The flag to determine whether or not to show the 3PDD play dialog
        extern const DECLARE_EACORE_SETTING(3PDD_PLAYDIALOG_SHOW);

        /// \property SETTING_JUMPLIST_RECENTLIST
        /// \brief A string with a list of recently played games in bottom up order separated by "|"
        extern const DECLARE_EACORE_SETTING(JUMPLIST_RECENTLIST);

        /// \property SETTING_SHOWIGONUX
        /// \brief Flag that determines whether to use IGO in game.
        extern const DECLARE_EACORE_SETTING(SHOWIGONUX);


        /// \property SETTING_SHOWIGONUXFIRSTTIME
        /// \brief Flag that determines whether the IGO 'first time' screen has been shown.
        extern const DECLARE_EACORE_SETTING(SHOWIGONUXFIRSTTIME);


        /// \property SETTING_MULTILAUNCHPRODUCTIDSANDTITLES
        /// \brief User's list of default launch titles with their respected product id.
        extern const DECLARE_EACORE_SETTING(MULTILAUNCHPRODUCTIDSANDTITLES);


        /// \property SETTING_OIGDISABLEDGAMES
        /// \brief User's list of games that where manually disabled.
        extern const DECLARE_EACORE_SETTING(OIGDISABLEDGAMES);

        /// \property SETTING_FIRSTLAUNCHMESSAGESHOWN
        /// \brief User's list of games whose first launch message has shown.
        extern const DECLARE_EACORE_SETTING(FIRSTLAUNCHMESSAGESHOWN);

        /// \property SETTING_UPGRADEDSAVEGAMESWARNED
        /// \brief User's list of games that we showed a warning about their saved games.
        extern const DECLARE_EACORE_SETTING(UPGRADEDSAVEGAMESWARNED);

        /// \property SETTING_GAMECOMMANDLINEARGUMENTS
        /// \brief List of game command line arguments.
        extern const DECLARE_EACORE_SETTING(GAMECOMMANDLINEARGUMENTS);

        /// \property SETTING_SHOWTIMESTAMPS
        /// \brief Flag to determine whether to show timestamps in user chat window.
        extern const DECLARE_EACORE_SETTING(SHOWTIMESTAMPS);

        /// \property SETTING_SAVECONVERSATIONHISTORY
        /// \brief Flag to determine whether to save chat conversation history
        extern const DECLARE_EACORE_SETTING(SAVECONVERSATIONHISTORY);

        /// \property SETTING_STOREDLOGINEMAIL
        /// \brief User's login email stored in encrypted form.
        extern const DECLARE_EACORE_SETTING(STOREDLOGINEMAIL);


        /// \property SETTING_TIMESHIDENOTIFICATION
        /// \brief Counter to indicate whether to show hidden user notifications.
        extern const DECLARE_EACORE_SETTING(TIMESHIDENOTIFICATION);


        /// \property SETTING_VIEWMODE
        /// \brief The layout of the Ebisu main view); typically grid or list.
        extern const DECLARE_EACORE_SETTING(VIEWMODE);

        /// \property SETTING_INVALID_REMEMBER_ME
        /// \brief If SETTING_REMEMBER_ME is this value, a remember me cookie has not been set.
        ORIGIN_PLUGIN_API extern const QString SETTING_INVALID_REMEMBER_ME;

        /// \property SETTING_REMEMBER_ME_INT
        /// \brief Contains the remember me cookie value, url domain and expiration date (format = Qt::TextDate) separated by ||. One cookie for integration and another for production.
        extern const DECLARE_EACORE_SETTING(REMEMBER_ME_INT);

        /// \property SETTING_REMEMBER_ME_PROD
        /// \brief Contains the remember me cookie value, url domain and expiration date (format = Qt::TextDate) separated by ||. One cookie for integration and another for production.
        extern const DECLARE_EACORE_SETTING(REMEMBER_ME_PROD);

        /// \property SETTING_INVALID_REMEMBER_ME_USERID
        /// \brief If SETTING_REMEMBER_ME_USERID is this value, a remember me cookie has not been set, and so no associated remember me UserId either
        ORIGIN_PLUGIN_API extern const QString SETTING_INVALID_REMEMBER_ME_USERID;

        /// \property SETTING_REMEMBER_ME_USERID_INT
        /// \brief the userID associated with the remember me cookie in non-production
        extern const DECLARE_EACORE_SETTING(REMEMBER_ME_USERID_INT);

        /// \property SETTING_REMEMBER_ME_USERID_PROD
        /// \brief the userID associated with the remember me cookie in production
        extern const DECLARE_EACORE_SETTING(REMEMBER_ME_USERID_PROD);

        /// \property SETTING_INVALID_MOST_RECENT_USER_NAME
        /// \brief If SETTING_MOST_RECENT_USER_NAME is this value, a username cookie does not exits.
        ORIGIN_PLUGIN_API extern const QString SETTING_INVALID_MOST_RECENT_USER_NAME;

        /// \property SETTING_MOST_RECENT_USER_NAME_INT
        /// \brief Contains the cookie with the last username that logged in, url domain and expiration date (format = Qt::TextDate) separated by ||. One cookie for integration and another for production.
        extern const DECLARE_EACORE_SETTING(MOST_RECENT_USER_NAME_INT);

        /// \property SETTING_MOST_RECENT_USER_NAME_PROD
        /// \brief Contains the cookie with the last username that logged in, url domain and expiration date (format = Qt::TextDate) separated by ||. One cookie for integration and another for production.
        extern const DECLARE_EACORE_SETTING(MOST_RECENT_USER_NAME_PROD);

        /// \property SETTING_INVALID_TFAID
        /// \brief If SETTING_TFAID is this value, a TFAID has not been set.
        ORIGIN_PLUGIN_API extern const QString SETTING_INVALID_TFAID;

        /// \property SETTING_TFAID_INT
        /// \brief Contains the TFAid cookie value, url domain and expiration date (format = Qt::TextDate) separated by ||. One cookie for integration and another for production.
        extern const DECLARE_EACORE_SETTING(TFAID_INT);

        /// \property SETTING_TFAID_PROD
        /// \brief Contains the TFAid cookie value, url domain and expiration date (format = Qt::TextDate) separated by ||. One cookie for integration and another for production.
        extern const DECLARE_EACORE_SETTING(TFAID_PROD);

        /// \property REMEMBERME_TFA_TIMESTAMP.
        /// \brief This is set to the epoch time (in seconds) when RememberMeProd/Int or TFAIdProd/Int setting is modified by the user.
        extern const DECLARE_EACORE_SETTING(REMEMBERME_TFA_TIMESTAMP);

        /// \property SETTING_DISABLE_PROMO_MANAGER
        /// \brief Forces the promo manager to never show.
        extern const DECLARE_EACORE_SETTING(DISABLE_PROMO_MANAGER);

        /// \property SETTING_IGNORE_NON_MANDATORY_GAME_UPDATES
        /// \brief Forces us to not prompt the update UI if it's not a mandatory update.
        extern const DECLARE_EACORE_SETTING(IGNORE_NON_MANDATORY_GAME_UPDATES);

        /// \property SETTING_SHUTDOWN_ORIGIN_ON_GAME_FINISHED
        /// \brief Hides all Origin UI when a game is launched from desktop and closes Origin once the play flow is complete.
        extern const DECLARE_EACORE_SETTING(SHUTDOWN_ORIGIN_ON_GAME_FINISHED);

        /// \property FEATURE CALLOUTS 
        /// TODO: There will be deleted and replaced each release for different feature callouts
        extern const Setting SETTING_CALLOUT_SOCIALUSERAREA_SHOWN;
        extern const Setting SETTING_CALLOUT_GROUPSTAB_SHOWN;
        extern const Setting SETTING_CALLOUT_VOIPBUTTON_ONE_TO_ONE_SHOWN;
        extern const Setting SETTING_CALLOUT_VOIPBUTTON_GROUP_SHOWN;

        // Telemetry opt-out

        /// \property SETTING_HW_SURVEY_OPTOUT
        /// \brief to allow/dis allow sending of HW survey for all users in the machine
        extern const DECLARE_EACORE_SETTING(HW_SURVEY_OPTOUT);

        /////////////////////// OriginX Settings /////////////////////////////////////////
        extern const DECLARE_EACORE_SETTING(ORIGINX_SPALOADED);
        extern const DECLARE_EACORE_SETTING(ORIGINX_SPAPRELOADSUCCESS);
        extern const DECLARE_EACORE_SETTING(ORIGINX_SPALOCALE);

        ////////////////////// Nucleus 2.0 Settings //////////////////////////////////////

        extern const DECLARE_EACORE_SETTING(ClientId);
        extern       DECLARE_EACORE_SETTING(ClientSecret);
        extern       DECLARE_EACORE_SETTING(ConnectPortalBaseUrl);
        extern       DECLARE_EACORE_SETTING(SignInPortalBaseUrl);
        extern       DECLARE_EACORE_SETTING(IdentityPortalBaseUrl);
        extern const DECLARE_EACORE_SETTING(LoginSuccessRedirectUrl);
        extern const DECLARE_EACORE_SETTING(LogoutSuccessRedirectUrl);
        extern       DECLARE_EACORE_SETTING(OnlineLoginServiceUrl);
        extern       DECLARE_EACORE_SETTING(OfflineLoginServiceUrl);
        extern       DECLARE_EACORE_SETTING(MaxOfflineWebApplicationCacheSize);

        ORIGIN_PLUGIN_API extern const QString ClientSecretProduction;
        ORIGIN_PLUGIN_API extern const QString ClientSecretIntegration;

        //////////////////////////////////////////////////////////////////////////////////

        enum CrashDataOptOut
        {
            Always,
            Never,
            AskMe,
            Default = AskMe
        };

        /// \property SETTING_CRASH_DATA_OPTOUT
        /// \brief to allow/dis allow sending of Crash data for all users in the machine
        extern const DECLARE_EACORE_SETTING(CRASH_DATA_OPTOUT);

        // make these match FilterViewCode
        typedef enum 
        {
            SETTINGVALUE_GAMES_FILTER_ALL = 0,
            SETTINGVALUE_GAMES_FILTER_RECENTLY_PLAYED = 1,
            SETTINGVALUE_GAMES_FILTER_FAVORITES = 2,
            SETTINGVALUE_GAMES_FILTER_DOWNLOADING = 3,
            SETTINGVALUE_GAMES_FILTER_HIDDEN = 4,
            SETTINGVALUE_GAMES_FILTER_READY_TO_PLAY = 5,
        } SettingEnumGamesFilter;

        typedef enum 
        {
            SETTINGVALUE_GAMES_VIEW_MODE_GRID = 0,
            SETTINGVALUE_GAMES_VIEW_MODE_LIST = 1
        }
        SettingEnumGamesViewMode;

        SettingEnumGamesViewMode const SETTING_VIEWMODE_DEFAULT = SETTINGVALUE_GAMES_VIEW_MODE_GRID;

         extern DECLARE_EACORE_SETTING(RegistrationURL);
         extern DECLARE_EACORE_SETTING(ActivationURL);
         extern DECLARE_EACORE_SETTING(ChangePasswordURL);
         extern DECLARE_EACORE_SETTING(NewUserExpURL);
         extern DECLARE_EACORE_SETTING(SupportURL);
         extern DECLARE_EACORE_SETTING(OriginFaqURL);
         extern DECLARE_EACORE_SETTING(EbisuForumURL);
         extern DECLARE_EACORE_SETTING(EbisuForumSSOURL);
         extern DECLARE_EACORE_SETTING(EbisuForumFranceURL);
         extern DECLARE_EACORE_SETTING(EbisuForumFranceSSOURL);
         extern DECLARE_EACORE_SETTING(GameDetailsURL);
         extern DECLARE_EACORE_SETTING(MotdURL);
         extern DECLARE_EACORE_SETTING(ReleaseNotesURL);
         extern DECLARE_EACORE_SETTING(EulaURL);
         extern DECLARE_EACORE_SETTING(ToSURL);
         extern DECLARE_EACORE_SETTING(PrivacyPolicyURL);
         extern DECLARE_EACORE_SETTING(PromoURL);
         extern DECLARE_EACORE_SETTING(FriendSearchURL);
         extern DECLARE_EACORE_SETTING(EmptyShelfURL);
         extern DECLARE_EACORE_SETTING(DlgFriendSearchURL);
         extern DECLARE_EACORE_SETTING(GamerProfileURL);
         extern DECLARE_EACORE_SETTING(WizardURL);
         extern DECLARE_EACORE_SETTING(HeartbeatURL);
         extern DECLARE_EACORE_SETTING(EnvironmentName);
         extern DECLARE_EACORE_SETTING(PdlcStoreLegacyURL);
         extern DECLARE_EACORE_SETTING(PdlcStoreURL);
         extern DECLARE_EACORE_SETTING(StoreEntitleFreeGameURL);
         extern DECLARE_EACORE_SETTING(EbisuLockboxURL);
         extern DECLARE_EACORE_SETTING(EbisuLockboxV3URL);
         extern DECLARE_EACORE_SETTING(CustomerSupportHomeURL);
         extern DECLARE_EACORE_SETTING(CustomerSupportURL);
         extern DECLARE_EACORE_SETTING(EbisuCustomerSupportChatURL);
         extern DECLARE_EACORE_SETTING(CloudSyncAPIURL);
         extern DECLARE_EACORE_SETTING(UserIdURL);
         extern DECLARE_EACORE_SETTING(ForgotPasswordURL);
         extern DECLARE_EACORE_SETTING(OriginAccountURL);
         extern DECLARE_EACORE_SETTING(CloudSavesBlacklistURL);
         extern DECLARE_EACORE_SETTING(UpdateURL);
         extern DECLARE_EACORE_SETTING(RedeemCodeReturnURL);
         extern DECLARE_EACORE_SETTING(AvatarURL);
         extern DECLARE_EACORE_SETTING(FriendsURL);
         extern DECLARE_EACORE_SETTING(ChatURL);
         extern DECLARE_EACORE_SETTING(LoginRegistrationURL);
         extern DECLARE_EACORE_SETTING(EcommerceURL);        // This is for ECommerce 2.0.
         extern DECLARE_EACORE_SETTING(EcommerceV1URL);      // Used for legacy checkout and ofb catalog dependencies
         extern DECLARE_EACORE_SETTING(OverrideInitURL); // Used for overriding the config response from the server, not part of the dynamic payload!
         extern DECLARE_EACORE_SETTING(ShowMyOrigin);
         extern DECLARE_EACORE_SETTING(MyOriginPageURL);
         extern DECLARE_EACORE_SETTING(GamesServiceURL);
         extern DECLARE_EACORE_SETTING(WebDispatcherURL);
         extern DECLARE_EACORE_SETTING(AchievementServiceURL);
         extern DECLARE_EACORE_SETTING(AchievementsBasePageURL);
         extern DECLARE_EACORE_SETTING(AchievementsEnabled);
        extern DECLARE_EACORE_SETTING(SubscriptionServiceURL);
        extern DECLARE_EACORE_SETTING(SubscriptionVaultServiceURL);
        extern DECLARE_EACORE_SETTING(SubscriptionTrialEligibilityServiceURL);
        extern DECLARE_EACORE_SETTING(SubscriptionEnabled);
        extern DECLARE_EACORE_SETTING(SubscriptionEntitlementOfferId);
        extern DECLARE_EACORE_SETTING(SubscriptionEntitlementRetiringTime);
        extern DECLARE_EACORE_SETTING(SubscriptionEntitlementMasterIdUpdateAvaliable);
        extern DECLARE_EACORE_SETTING(SubscriptionExpirationReason);
        extern DECLARE_EACORE_SETTING(SubscriptionExpirationDate);
        extern DECLARE_EACORE_SETTING(SubscriptionMaxOfflinePlay);
        extern DECLARE_EACORE_SETTING(SubscriptionLastModifiedKey);
         extern DECLARE_EACORE_SETTING(WebWidgetUpdateURL);
         extern DECLARE_EACORE_SETTING(NetPromoterURL);
         extern DECLARE_EACORE_SETTING(FeedbackPageURL);
         extern DECLARE_EACORE_SETTING(StoreOrderHistoryURL);
         extern DECLARE_EACORE_SETTING(IGODefaultURL);
         extern DECLARE_EACORE_SETTING(IGODefaultSearchURL);
         extern DECLARE_EACORE_SETTING(InicisSsoCheckoutBaseURL);
         extern DECLARE_EACORE_SETTING(GNUURL);
         extern DECLARE_EACORE_SETTING(GPLURL);
         extern DECLARE_EACORE_SETTING(SteamSupportURL);
         extern DECLARE_EACORE_SETTING(StoreSecurityURL);
        extern DECLARE_EACORE_SETTING(AccountSubscriptionURL);
         extern DECLARE_EACORE_SETTING(StorePrivacyURL);
         extern DECLARE_EACORE_SETTING(StorePaymentAndShippingURL);
         extern DECLARE_EACORE_SETTING(StoreRedemptionURL);
         extern DECLARE_EACORE_SETTING(VoiceURL);
         extern DECLARE_EACORE_SETTING(GroupsURL);
        extern DECLARE_EACORE_SETTING(TimedTrialServiceURL);    // This is the OOA service dealing with time limited trials.
         extern DECLARE_EACORE_SETTING(OriginXURL);

        //////////////////////////////////////////////////////////////////////////
        // Atom URLs
         extern DECLARE_EACORE_SETTING(AtomURL);

        //////////////////////////////////////////////////////////////////////////
        // Achievement sharing URLs
         extern DECLARE_EACORE_SETTING(AchievementSharingURL);

        //////////////////////////////////////////////////////////////////////////
        // COPPA Help URL
         extern DECLARE_EACORE_SETTING(COPPADownloadHelpURL);
         extern DECLARE_EACORE_SETTING(COPPAPlayHelpURL);
        //////////////////////////////////////////////////////////////////////////
        // Dirty bits service values
         extern DECLARE_EACORE_SETTING(DirtyBitsServiceHostname);
         extern DECLARE_EACORE_SETTING(DirtyBitsServiceResource);

        //////////////////////////////////////////////////////////////////////////
        // Automation EACore overrides
         extern DECLARE_EACORE_SETTING(NotificationExpiryTime);
         extern DECLARE_EACORE_SETTING(DisableNotifications);
         extern DECLARE_EACORE_SETTING(DisableIdling);

        //////////////////////////////////////////////////////////////////////////
        /// Navigation debugging: print out our navigation to our Log file
         extern DECLARE_EACORE_SETTING(NavigationDebugging);

         extern       DECLARE_EACORE_SETTING(StoreInitialURL);
         extern       DECLARE_EACORE_SETTING(StoreMasterTitleURL);
         extern       DECLARE_EACORE_SETTING(StoreProductURL);
         extern       DECLARE_EACORE_SETTING(FreeGamesURL);
         extern const DECLARE_EACORE_SETTING(DefaultTab);
         extern const DECLARE_EACORE_SETTING(EnableJTL);

         extern const DECLARE_EACORE_SETTING(UrlHost);           //either "store", "login", "library" or "launchgame"
         extern const DECLARE_EACORE_SETTING(CmdLineAuthToken);  //param authtoken is found in the command line, regardless of UrlHost or not
         extern const DECLARE_EACORE_SETTING(UrlStoreProductBarePath); //param when UrlHost = store
         extern const DECLARE_EACORE_SETTING(UrlLibraryGameIds); //may be param when UrlHost = library
         extern const DECLARE_EACORE_SETTING(UrlRefresh);        //may be param when UrlHost = library

         extern const DECLARE_EACORE_SETTING(UrlGameIds);        //string that has a list of contentIds or offerIds, param when UrlHost = launchgame
         extern const DECLARE_EACORE_SETTING(UrlGameTitle);      //may be param when UrlHost = launchgame
         extern const DECLARE_EACORE_SETTING(UrlSSOtoken);       //may be param when UrlHost = launchgame
         extern const DECLARE_EACORE_SETTING(UrlSSOauthCode);    //may be param when UrlHost = launchgame
         extern const DECLARE_EACORE_SETTING(UrlSSOauthRedirectUrl);     //may be param when UrlHost = launchgame
         extern const DECLARE_EACORE_SETTING(UrlCmdParams);      //may be param when UrlHost = launchgame
         extern const DECLARE_EACORE_SETTING(UrlAutoDownload);   //may be param when UrlHost = launchgame
         extern const DECLARE_EACORE_SETTING(UrlRestart);        //may be param when UrlHost = launchgame
         extern const DECLARE_EACORE_SETTING(UrlItemSubType);    //may be param when UrlHost = launchgamejump

         extern const DECLARE_EACORE_SETTING(LocalhostSSOtoken); //SSOtoken when authenticating from localhost call

         extern DECLARE_EACORE_SETTING(LSX_PORT);         ///< Setting for overriding the LSX_PORT for the OriginSDK.
		extern DECLARE_EACORE_SETTING(ENABLE_PROGRESSIVE_INSTALLER_SIMULATOR);
        extern DECLARE_EACORE_SETTING(PROGRESSIVE_INSTALLER_SIMULATOR_CONFIG);

        extern DECLARE_EACORE_SETTING(DisconnectSDKWhenNoEntitlement); //Override for QA to check Origin SDK disconnecting when connecting a game that has no entitlement. 

		extern const DECLARE_EACORE_SETTING(DownloadQueue);

        /// \enum SettingEnumBroadcastResolution
        /// \brief settings for the video broadcast resolution
        // note that this matches up 1:1 with the entries in the drop down menu
        typedef enum {
            SETTINGVALUE_320x240 = 0,
            SETTINGVALUE_480x360,
            SETTINGVALUE_640x480,
            SETTINGVALUE_1024x768,
            SETTINGVALUE_1280x720,
            SETTINGVALUE_1920x1080
        } SettingEnumBroadcastResolution;

        // \property SETTING_BROADCAST_RESOLUTION
        /// \brief what the setting for the resolution is - see above for mapping
        extern const DECLARE_EACORE_SETTING(BROADCAST_RESOLUTION);

        /// \enum SettingEnumFramerate
        /// \brief settings for the video broadcast framerate
        // note that this matches up 1:1 with the entries in the drop down menu
        typedef enum {
            SETTINGVALUE_5 = 0,
            SETTINGVALUE_10,
            SETTINGVALUE_15,
            SETTINGVALUE_20,
            SETTINGVALUE_25,
            SETTINGVALUE_30
        } SettingEnumFramerate;

        // \property SETTING_BROADCAST_FRAMERATE
        /// \brief what the setting for the resolution is
        extern const DECLARE_EACORE_SETTING(BROADCAST_FRAMERATE);

        // note that this matches up 1:1 with the entries
        typedef enum {
            SETTINGVALUE_HIGHEST = 0,
            SETTINGVALUE_HIGH,
            SETTINGVALUE_MID,
            SETTINGVALUE_LOWEST
        } SettingEnumPictureQuality;

        extern const DECLARE_EACORE_SETTING(BROADCAST_QUALITY);

        extern const DECLARE_EACORE_SETTING(BROADCAST_BITRATE);

        extern const DECLARE_EACORE_SETTING(BROADCAST_AUTOSTART);

		extern const Setting SETTING_BROADCAST_USE_OPT_ENCODER;
        extern const Setting SETTING_BROADCAST_SHOW_VIEWERS_NUM;

        extern const DECLARE_EACORE_SETTING(BROADCAST_MIC_VOL);
        extern const DECLARE_EACORE_SETTING(BROADCAST_MIC_MUTE);
        extern const DECLARE_EACORE_SETTING(BROADCAST_GAME_VOL);
        extern const DECLARE_EACORE_SETTING(BROADCAST_GAME_MUTE);
        extern const DECLARE_EACORE_SETTING(BROADCAST_TOKEN);
        extern const DECLARE_EACORE_SETTING(BROADCAST_GAMENAME);
        /// \property SETTING_BROADCAST_SAVE_VIDEO
        /// \brief whether or not the user wants to save their videos
        extern const DECLARE_EACORE_SETTING(BROADCAST_SAVE_VIDEO);
        extern const DECLARE_EACORE_SETTING(BROADCAST_SAVE_VIDEO_URL);
        extern const DECLARE_EACORE_SETTING(BROADCAST_CHANNEL_URL);
        
        /// \property SETTING_BROADCAST_FIRST_TIME_CONNECT
        /// \brief internal only setting that indicates that this is the first time this user has connected to twitch
        extern const DECLARE_EACORE_SETTING(BROADCAST_FIRST_TIME_CONNECT);

        // Mac Alpha Trial
        extern DECLARE_EACORE_SETTING(FIRST_RUN);
        extern const DECLARE_EACORE_SETTING(AlphaLogin);

		// On the House
		extern	     DECLARE_EACORE_SETTING(OnTheHousePromoURL);
		extern       DECLARE_EACORE_SETTING(OnTheHouseStoreURL);
		extern const DECLARE_EACORE_SETTING(OnTheHouseOverridePromoURL);
		extern const DECLARE_EACORE_SETTING(LastDismissedOtHPromoOffer);

        // Subscriptions
        extern       DECLARE_EACORE_SETTING(SubscriptionStoreURL);
        extern       DECLARE_EACORE_SETTING(SubscriptionVaultURL);
        extern       DECLARE_EACORE_SETTING(SubscriptionFaqURL);
        extern       DECLARE_EACORE_SETTING(SubscriptionInitialURL);
        extern       DECLARE_EACORE_SETTING(SubscriptionRedemptionURL);

        ///
        /// Server user side settings
        ///
        extern const DECLARE_EACORE_SETTING(EnableIgoForAllGames);
        extern const DECLARE_EACORE_SETTING(EnableCloudSaves);
        extern const DECLARE_EACORE_SETTING(ServerSideEnableTelemetry); // Disambiguate with the existing EACore.ini setting
        extern const DECLARE_EACORE_SETTING(FavoriteProductIds);
        extern const DECLARE_EACORE_SETTING(HiddenProductIds);

#if ENABLE_VOICE_CHAT
        extern const Setting SETTING_AutoGain;
        extern const Setting SETTING_EnablePushToTalk;
        extern const Setting SETTING_PushToTalkKey;
        extern const Setting SETTING_PushToTalkMouse;
        extern const Setting SETTING_PushToTalkKeyString;
        extern const Setting SETTING_MicrophoneGain;
        extern const Setting SETTING_SpeakerGain;
        extern const Setting SETTING_EchoQueuedDelay;
        extern const Setting SETTING_EchoTailMultiplier;
        extern const Setting SETTING_EchoCancellation;
        extern const Setting SETTING_VoiceInputDevice;
        extern const Setting SETTING_VoiceOutputDevice;
        extern const Setting SETTING_VoiceActivationThreshold;
        extern const Setting SETTING_EnableVoiceChatIndicator;
        extern const Setting SETTING_DeclineIncomingVoiceChatRequests;
        extern const Setting SETTING_NoWarnAboutVoiceChatConflict;
#endif //ENABLE_VOICE_CHAT
        
        /// value to obtain URLS
        extern DECLARE_EACORE_SETTING(OriginConfigServiceURL);

        // URLS
        extern const DECLARE_EACORE_SETTING(DebugLanguage);
        extern const DECLARE_EACORE_SETTING(OriginDeveloperToolEnabled);
        extern const DECLARE_EACORE_SETTING(DownloadDebugEnabled);
        extern const DECLARE_EACORE_SETTING(AddonPreviewAllowed);
        extern const DECLARE_EACORE_SETTING(OverrideCustomerSupportPageUrl);    //to overrie the pages to load when Help is selected

        // Certificates
        extern const DECLARE_EACORE_SETTING(IgnoreSSLCertError);
        // Telemetry
        extern const DECLARE_EACORE_SETTING(TelemetryEnabled);
        extern const DECLARE_EACORE_SETTING(TelemetryXML);
        extern const DECLARE_EACORE_SETTING(TelemetryServer); //159.153.235.32 (default - PRODUCTION)
        extern const DECLARE_EACORE_SETTING(TelemetryPort); //9988

        //Connection
        enum OverridePromos {
            PromosEnabled,
            PromosDisabledNonProduction,
            PromosDefault
        };

        extern const DECLARE_EACORE_SETTING(OverridePromos);
        extern const DECLARE_EACORE_SETTING(FlushWebCache);
        extern const DECLARE_EACORE_SETTING(DisableEmptyShelf);

        // QA
        extern const DECLARE_EACORE_SETTING(OriginConfigDebug);
        extern const DECLARE_EACORE_SETTING(CloudSavesDebug);
        extern const DECLARE_EACORE_SETTING(ContentDebug);
        extern const DECLARE_EACORE_SETTING(qaForcedUpdateCheckInterval);
        extern const DECLARE_EACORE_SETTING(ForceLockboxURL);
        extern const DECLARE_EACORE_SETTING(EnableDownloadOverrideTelemetry);
        extern const DECLARE_EACORE_SETTING(MinNetPromoterTestRange);
        extern const DECLARE_EACORE_SETTING(MaxNetPromoterTestRange);
        extern const DECLARE_EACORE_SETTING(ShowNetPromoter);
        extern const DECLARE_EACORE_SETTING(ShowGameNetPromoter);
        extern const DECLARE_EACORE_SETTING(EnableIGODebugMenu);
        extern const DECLARE_EACORE_SETTING(EnableIGOStressTest);
        extern const DECLARE_EACORE_SETTING(IGOReducedBrowser);
        extern const DECLARE_EACORE_SETTING(IGOMiniAppBrowser);
        extern const DECLARE_EACORE_SETTING(CredentialsUserName);
        extern const DECLARE_EACORE_SETTING(CredentialsPassword);
        extern const DECLARE_EACORE_SETTING(ShowDebugMenu);
        extern const DECLARE_EACORE_SETTING(PurgeAccessLicenses);
        extern const DECLARE_EACORE_SETTING(DisableTwitchBlacklist);
        extern const DECLARE_EACORE_SETTING(DisableEmbargoMode);
        extern const DECLARE_EACORE_SETTING(OverrideOSVersion);
        extern const DECLARE_EACORE_SETTING(ReduceDirtyBitsCappedTimeout);
        extern const DECLARE_EACORE_SETTING(OverrideLoadTimeoutError);
        extern const DECLARE_EACORE_SETTING(VoipAddressBefore);
        extern const DECLARE_EACORE_SETTING(VoipAddressAfter);
        extern const DECLARE_EACORE_SETTING(VoipForceSurvey);
        extern const DECLARE_EACORE_SETTING(VoipForceUnexpectedEnd);
        extern const DECLARE_EACORE_SETTING(ForceChatGroupDeleteFail);
        extern const DECLARE_EACORE_SETTING(SonarOneMinuteInactivityTimeout);
        extern const DECLARE_EACORE_SETTING(OverrideCatalogDefinitionLookupTelemetryInterval);
        extern const DECLARE_EACORE_SETTING(OverrideCatalogDefinitionLookupRetryInterval);
        extern const DECLARE_EACORE_SETTING(OverrideCatalogDefinitionRefreshInterval);
        extern const DECLARE_EACORE_SETTING(OverrideCatalogDefinitionMaxAgeDays);
        extern const DECLARE_EACORE_SETTING(OverrideCatalogIGODefaultURL);

        // CrashTesting
        extern const DECLARE_EACORE_SETTING(CrashOnStartup);
        extern const DECLARE_EACORE_SETTING(CrashOnExit);

        // DownloaderTuning
        extern const DECLARE_EACORE_SETTING(MaxWorkersForAllSessions);
        extern const DECLARE_EACORE_SETTING(SessionBaseTickRate);
        extern const DECLARE_EACORE_SETTING(RateDecreasePerActiveSession);
        extern const DECLARE_EACORE_SETTING(MinRequestSize);
        extern const DECLARE_EACORE_SETTING(MaxRequestSize);
        extern const DECLARE_EACORE_SETTING(MaxWorkersPerFileSession);
        extern const DECLARE_EACORE_SETTING(MaxWorkersPerHttpSession);
        extern const DECLARE_EACORE_SETTING(IdealFileRequestSize);
        extern const DECLARE_EACORE_SETTING(IdealHttpRequestSize);
        extern const DECLARE_EACORE_SETTING(MaxUnpackServiceThreads);
        extern const DECLARE_EACORE_SETTING(MaxUnpackCompressedChunkSize);
        extern const DECLARE_EACORE_SETTING(MaxUnpackUncompressedChunkSize);
        extern const DECLARE_EACORE_SETTING(EnableITOCRCErrors);
        extern const DECLARE_EACORE_SETTING(EnableDownloaderSafeMode);
        extern const DECLARE_EACORE_SETTING(EnableHTTPS);

        // Progressive Install
        extern const DECLARE_EACORE_SETTING(EnableProgressiveInstall);
        extern const DECLARE_EACORE_SETTING(EnableSteppedDownload);

        // EACore
        extern const DECLARE_EACORE_SETTING(HeartbeatLog);
        extern const DECLARE_EACORE_SETTING(DL_History); //gnDownloadHistoryLengthKey
        extern const DECLARE_EACORE_SETTING(ActivateLegacyContent); //gsActivateLegacyContentKey
        extern const DECLARE_EACORE_SETTING(AuthenticationApiURL); //gsAuthenticationAPIKey
        extern const DECLARE_EACORE_SETTING(autodecrypt);
        extern const DECLARE_EACORE_SETTING(autoPatch);
        extern const DECLARE_EACORE_SETTING(auto_patch); //g_sAutoPatchPrefId
        extern const DECLARE_EACORE_SETTING(AvatarApiURL); //gsAvatarAPIKey
        extern const DECLARE_EACORE_SETTING(ChatApiURL); //gsChatAPIKey
        extern const DECLARE_EACORE_SETTING(country);
        extern const DECLARE_EACORE_SETTING(current_path);
        extern const DECLARE_EACORE_SETTING(EmailPasswordKeyApiURL); //gsEmailPasswordKeyAPIKey
        extern const DECLARE_EACORE_SETTING(EnableIGOForAll); //gsEnableIGOForAllKey
        extern const DECLARE_EACORE_SETTING(ForceFullIGOLogging);
        extern const DECLARE_EACORE_SETTING(ForceMinIGOResolution);
        extern const DECLARE_EACORE_SETTING(FriendApiURL); //gsFriendAPIKey
        extern const DECLARE_EACORE_SETTING(full_exe_path);
        extern const DECLARE_EACORE_SETTING(GenerateStagedURL); //gsGenerateStagedURL
        extern const DECLARE_EACORE_SETTING(locale);
        extern const DECLARE_EACORE_SETTING(LogId); //g_sInstanceNamePrefId
        extern const DECLARE_EACORE_SETTING(LoginGCSApiURL); //gsLoginGCSAPIKey
        extern const DECLARE_EACORE_SETTING(Max_DL_Chunksize); //gnDownloadMaxChunkSizeKey
        extern const DECLARE_EACORE_SETTING(Min_DL_Chunksize); //gnDownloadMinChunkSizeKey
        extern const DECLARE_EACORE_SETTING(Max_DL_Threads); //gnDownloadMaxThreadsKey
        extern const DECLARE_EACORE_SETTING(Min_DL_Threads); //gnDownloadMinThreadsKey
        extern const DECLARE_EACORE_SETTING(OriginOfflineMode); //g_sOriginOfflineMode

        // EACore Overrides
        extern const DECLARE_EACORE_SETTING(OverrideDownloadPath); //::gsOverrideDownloadKeyPrefix + GetExtId()
        extern const DECLARE_EACORE_SETTING(OverrideDownloadSyncPackagePath); //::gsOverrideDownloadSyncPackageKeyPrefix + GetExtId()
        extern const DECLARE_EACORE_SETTING(OverrideCommerceProfile); //::gsOverrideCommerceProfilePrefix + GetExtId()
        extern const DECLARE_EACORE_SETTING(OverrideMonitoredInstall); //::gsOverrideMonitoredInstall + GetExtId()
        extern const DECLARE_EACORE_SETTING(OverrideMonitoredPlay); //::gsOverrideMonitoredPlay + GetExtId()
         extern const DECLARE_EACORE_SETTING(OverrideExternalInstallDialog); //::gsOverride externalInstallDialog + GetExtId()
         extern const DECLARE_EACORE_SETTING(OverrideExternalPlayDialog); //::gsOverride externalPlayDialog + GetExtId()
        extern const DECLARE_EACORE_SETTING(OverridePartnerPlatform); //::gsOverridePartnerPlatform + GetExtId()
        extern const DECLARE_EACORE_SETTING(OverridePreloadPath); //::gsOverridePreloadKeyPrefix
        extern const DECLARE_EACORE_SETTING(OverrideExecuteRegistryKey);
        extern const DECLARE_EACORE_SETTING(OverrideExecuteFilename);
        extern const DECLARE_EACORE_SETTING(OverrideInstallRegistryKey);
        extern const DECLARE_EACORE_SETTING(OverrideInstallFilename);
        extern const DECLARE_EACORE_SETTING(OverrideGameLauncherURL);
        extern const DECLARE_EACORE_SETTING(OverrideGameLauncherURLIntegration);
        extern const DECLARE_EACORE_SETTING(OverrideGameLauncherURLIdentityClientId);
        extern const DECLARE_EACORE_SETTING(OverrideGameLauncherSoftwareId);
        extern const DECLARE_EACORE_SETTING(OverrideVersion);
        extern const DECLARE_EACORE_SETTING(OverrideBuildReleaseVersion);
        extern const DECLARE_EACORE_SETTING(OverrideBuildIdentifier);
        extern const DECLARE_EACORE_SETTING(OverrideUseLegacyCatalog);
        extern const DECLARE_EACORE_SETTING(OverrideEnableDLCUninstall);
        extern const DECLARE_EACORE_SETTING(GeolocationIPAddress);
        extern const DECLARE_EACORE_SETTING(OriginGeolocationTest);
        extern const DECLARE_EACORE_SETTING(KGLTimer);
        extern const DECLARE_EACORE_SETTING(DebugMenuEntitlementID);
        extern const DECLARE_EACORE_SETTING(DebugMenuEulaPathOverride);
        extern const DECLARE_EACORE_SETTING(ForceContentWatermarking);
        extern const DECLARE_EACORE_SETTING(EnableGameLaunchCancel);
        extern const DECLARE_EACORE_SETTING(EnableFeatureCallouts);
        extern const DECLARE_EACORE_SETTING(OverrideIGOSetting);
        extern const DECLARE_EACORE_SETTING(OverrideSalePrice);
        extern const DECLARE_EACORE_SETTING(OverrideBundleContents);
        extern const DECLARE_EACORE_SETTING(OverrideThumbnailUrl);
        extern const DECLARE_EACORE_SETTING(OverrideReleaseDate);
        extern const DECLARE_EACORE_SETTING(OverrideIsForceKilledAtOwnershipExpiry);
        extern const DECLARE_EACORE_SETTING(OverrideAchievementSet);
        extern const DECLARE_EACORE_SETTING(OverrideSNOFolder);
        extern const DECLARE_EACORE_SETTING(OverrideSNOUpdateCheck);
        extern const DECLARE_EACORE_SETTING(OverrideSNOScheduledTime);
        extern const DECLARE_EACORE_SETTING(OverrideSNOConfirmBuild);

        // DEBUG only settings
#ifdef _DEBUG
        extern const DECLARE_EACORE_SETTING(IsolateProductIds);
        extern const DECLARE_EACORE_SETTING(DebugEscalationService);
#endif

        // Nucleus 2.0 Identity Portal
        ORIGIN_PLUGIN_API extern const QString OverrideConnectPortalBaseUrl;
        ORIGIN_PLUGIN_API extern const QString OverrideIdentityPortalBaseUrl;
        ORIGIN_PLUGIN_API extern const QString OverrideLoginSuccessRedirectUrl;
        ORIGIN_PLUGIN_API extern const QString OverrideSignInPortalBaseUrl;
        ORIGIN_PLUGIN_API extern const QString OverrideClientId;
        ORIGIN_PLUGIN_API extern const QString OverrideClientSecret;

         // Free Trials Overrides
        extern const DECLARE_EACORE_SETTING(OverrideTerminationDate);
        extern const DECLARE_EACORE_SETTING(OverrideUseEndDate);
        extern const DECLARE_EACORE_SETTING(OverrideItemSubType);
        extern const DECLARE_EACORE_SETTING(OverrideCloudSaveContentIDFallback);
        extern const DECLARE_EACORE_SETTING(OverrideTrialDuration);

        extern const DECLARE_EACORE_SETTING(ProfileServer); //gsProfileServerKey
        extern const DECLARE_EACORE_SETTING(RandomMachineHashTestMode); //gsRandomMachineHashTestModeKey
        extern const DECLARE_EACORE_SETTING(RetrieveJITURL); //gsRetrieveJITURL
        extern const DECLARE_EACORE_SETTING(ServerIdleAutoShutdown_ms);
        extern const DECLARE_EACORE_SETTING(TokenExtensionApiURL); //gsTokenExtensionAPIKey
        extern const DECLARE_EACORE_SETTING(UseStandAloneServer);
        extern const DECLARE_EACORE_SETTING(UseTestServer); //gsUseBugSentryTestServer
        extern const DECLARE_EACORE_SETTING(FullMemoryDump);
        extern const DECLARE_EACORE_SETTING(XMPPApiURL); //gsXMPPAPIKey

        // logging
        extern const DECLARE_EACORE_SETTING(LogConsoleDevice); // values: "file" or "window"
        extern const DECLARE_EACORE_SETTING(LogDebug);
        extern const DECLARE_EACORE_SETTING(LogCatalog);
        extern const DECLARE_EACORE_SETTING(LogDirtyBits);
        extern const DECLARE_EACORE_SETTING(LogDownload);
        extern const DECLARE_EACORE_SETTING(LogSonar);
        extern const DECLARE_EACORE_SETTING(LogSonarDirectSoundCapture);
        extern const DECLARE_EACORE_SETTING(LogSonarDirectSoundPlayback);
        extern const DECLARE_EACORE_SETTING(LogSonarJitterMetricsLevel);
        extern const DECLARE_EACORE_SETTING(LogSonarJitterTracing);
        extern const DECLARE_EACORE_SETTING(LogSonarPayloadTracing);
        extern const DECLARE_EACORE_SETTING(LogSonarSpeexCapture);
        extern const DECLARE_EACORE_SETTING(LogSonarSpeexPlayback);
        extern const DECLARE_EACORE_SETTING(LogSonarTiming);
        extern const DECLARE_EACORE_SETTING(LogSonarConnectionStats);
        extern const DECLARE_EACORE_SETTING(LogVoip);
        extern const DECLARE_EACORE_SETTING(LogXep0115);
        extern const DECLARE_EACORE_SETTING(LogXmpp);
		extern const DECLARE_EACORE_SETTING(LogGroupServiceREST);
		extern const DECLARE_EACORE_SETTING(LogVoiceServiceREST);

        // EACoreSession
        extern const DECLARE_EACORE_SETTING(GeoCountry);
        extern const DECLARE_EACORE_SETTING(CommerceCountry); // The country where commerce defaults to. Can be different from geoCountry 
        extern const DECLARE_EACORE_SETTING(CommerceCurrency);// The currency used in the commerce country.
        extern const DECLARE_EACORE_SETTING(LastGeoCheck); 
        extern const DECLARE_EACORE_SETTING(DIPAutoInstallTried); //::  gsDIPAutoInstallTriedKey + pItem->GetExtId()
        extern const DECLARE_EACORE_SETTING(SharedServerID);

        // EACoreUser
        extern const DECLARE_EACORE_SETTING(Connection);
        extern const DECLARE_EACORE_SETTING(OverrideEntitlementResponse);

        extern const DECLARE_EACORE_SETTING(WebDebugEnabled);
        extern const DECLARE_EACORE_SETTING(BlockingBehavior);
        extern const DECLARE_EACORE_SETTING(SingleLogin);
        extern const DECLARE_EACORE_SETTING(DisableNetPromoter);
        extern const DECLARE_EACORE_SETTING(DisableMotd);
        extern DECLARE_EACORE_SETTING(CdnOverride); // not const because this can be changed at runtime via Debug menu

        extern const DECLARE_EACORE_SETTING(DisableEntitlementFilter);

        // Sonar overrides
        extern const DECLARE_EACORE_SETTING(SonarClientPingInterval);
        extern const DECLARE_EACORE_SETTING(SonarClientTickInterval);
        extern const DECLARE_EACORE_SETTING(SonarJitterBufferSize);
        extern const DECLARE_EACORE_SETTING(SonarRegisterTimeout); // milliseconds
        extern const DECLARE_EACORE_SETTING(SonarRegisterMaxRetries);
        extern const DECLARE_EACORE_SETTING(SonarNetworkPingInterval); // milliseconds
        extern const DECLARE_EACORE_SETTING(SonarEnableRemoteEcho); // allow user to hear themselves in voice (only useful in testing)

        extern const DECLARE_EACORE_SETTING(SonarSpeexAgcEnable);
        extern const DECLARE_EACORE_SETTING(SonarSpeexAgcLevel);
        extern const DECLARE_EACORE_SETTING(SonarSpeexComplexity);
        extern const DECLARE_EACORE_SETTING(SonarSpeexQuality);
        extern const DECLARE_EACORE_SETTING(SonarSpeexVbrEnable);
        extern const DECLARE_EACORE_SETTING(SonarSpeexVbrQuality);

        extern const DECLARE_EACORE_SETTING(SonarTestingAudioError);
        extern const DECLARE_EACORE_SETTING(SonarTestingRegisterPacketLossPercentage);
        extern const DECLARE_EACORE_SETTING(SonarTestingEmptyConversationTimeoutDuration);
        extern const DECLARE_EACORE_SETTING(SonarTestingUserLocation);
        extern const DECLARE_EACORE_SETTING(SonarTestingVoiceServerRegisterResponse);

        extern const DECLARE_EACORE_SETTING(SonarNetworkGoodLoss);
        extern const DECLARE_EACORE_SETTING(SonarNetworkGoodJitter);
        extern const DECLARE_EACORE_SETTING(SonarNetworkGoodPing);
        extern const DECLARE_EACORE_SETTING(SonarNetworkOkLoss);
        extern const DECLARE_EACORE_SETTING(SonarNetworkOkJitter);
        extern const DECLARE_EACORE_SETTING(SonarNetworkOkPing);
        extern const DECLARE_EACORE_SETTING(SonarNetworkPoorLoss);
        extern const DECLARE_EACORE_SETTING(SonarNetworkPoorJitter);
        extern const DECLARE_EACORE_SETTING(SonarNetworkPoorPing);

        extern const DECLARE_EACORE_SETTING(SonarNetworkQualityPingStartupDuration);
        extern const DECLARE_EACORE_SETTING(SonarNetworkQualityPingShortInterval);
        extern const DECLARE_EACORE_SETTING(SonarNetworkQualityPingLongInterval);

        // OriginSDKJS session key
        extern const DECLARE_EACORE_SETTING(OriginSessionKey);

        class ORIGIN_PLUGIN_API SettingsKey
        {
        public:
            inline explicit SettingsKey(const QString &k = "") : mKey(k.toLower()) {}
            inline const QString& key() const { return mKey; }
            inline bool operator==(const SettingsKey &other) const { return (mKey == other.mKey); }
        private:
            QString mKey;
        };

        inline uint qHash(const SettingsKey &key, uint seed = 0)
        {
            return qHash(key.key(), seed);
        }

        /// \class SettingsManager
        /// \brief A singleton class that manages all Origin settings. 
        /// The SettingManager class is a ...
        /// TBD: thread-safe? re-entrant?
        class ORIGIN_PLUGIN_API SettingsManager : public QObject
        {
            Q_OBJECT

            friend class SettingSynchronizer;
            friend struct QScopedPointerDeleter<SettingsManager>;
            friend class Setting;

            friend ORIGIN_PLUGIN_API Variant readSetting(const QString& settingName, const Session::SessionRef session);
            friend ORIGIN_PLUGIN_API Variant readSetting(const Setting& setting, const Session::SessionRef session);
            friend ORIGIN_PLUGIN_API Variant readSettingAsAscii(const Setting& setting, const Session::SessionRef session);
            friend ORIGIN_PLUGIN_API void writeSetting(const Setting& setting, Variant const& value, const Session::SessionRef session);
            friend ORIGIN_PLUGIN_API void deleteSetting(const Setting& setting, const Session::SessionRef session);
            friend ORIGIN_PLUGIN_API bool isParseURLRawData(const QString& data);
            friend ORIGIN_PLUGIN_API void initializeURLSettings();
        public:

            /// \fn init
            /// \brief Initializes the SettingsManager in preparation of use. Should only be called once.
            static void init();

            /// \fn free
            /// \brief Terminates the SettingsManager and cleans up. Should only be called once. 
            static void free();

            /// \fn instance
            /// \brief  
            static SettingsManager* instance();

            /// \fn isLoaded
            /// \brief Returns true if the settings have finished being loaded from external storage.
            static bool isLoaded();

            /// \fn areServerUserSettingsLoaded
            /// \brief Returns true if the user server-side settings have finished loading
            static bool areServerUserSettingsLoaded();

            /// \fn loadOverrideSettings
            /// \brief Loads the locally-stored override settings
            void loadOverrideSettings();

            /// \fn unloadOverrideSettings
            /// \brief Unload the locally-stored override settings
            void unloadOverrideSettings();

            /// \fn reloadProductOverrideSettings
            /// \brief Reload eacore.ini product override settings, since they are allowed to be updated (do not require client re-start - eg, OverrideDownloadPath)
            void reloadProductOverrideSettings();

            /// \fn isProductOverrideSetting(const Setting &setting)
            /// \brief Returns true if setting is a product override setting - an eacore.ini override setting that can be changed without requiring a client re-start.
            bool isProductOverrideSetting(const Setting& setting);

            /// \fn isProductOverrideSetting(const QString &settingName)
            /// \brief Returns true if setting is a dynamic override setting - an eacore.ini override setting that can be changed without requiring a client re-start.
            bool isProductOverrideSetting(const QString& settingName);

            /// \fn activeMachineEnvironments
            /// \brief Returns the locally-stored account user account types (production, qa, etc)
            QStringList activeMachineEnvironments();

            /// \fn sync
            /// \brief Save the locally-stored settings to disk
            void sync();

            // settings support functions
            // TODO: move them somewhere safe/clean !?

            /// \fn inGameHotkeyDefault
            /// \brief generates the default IGO hotkey
            static qulonglong inGameHotkeyDefault();

            /// \fn pinHotkeyDefault
            /// \brief generates the default IGO pin window hotkey
            static qulonglong pinHotkeyDefault();

#if ENABLE_VOICE_CHAT
            /// \fn pushToTalkKeyDefault
            /// \brief generates the default Push To Talk key
            static qulonglong pushToTalkKeyDefault();
#endif

            /// \fn downloadInPlaceDefault
            /// \brief generates the default DIP folder
            static QString const downloadInPlaceDefault();

            /// \fn downloadCacheDefault
            /// \brief generates the default download cache folder
            static QString const downloadCacheDefault();

            /// \fn notifyError
            /// \brief Allows external clients to trigger a settingError.
            void notifyError(QString const& errorMessage);

            /// \fn unset
            /// \brief remove a setting
            static bool unset(Setting const& setting);

            /// \fn unset
            /// \brief Sees if the user has setting set in user local files
            bool isSettingSpecified(Setting const& setting);

            /// \fn settingsFor
            /// \brief Retrieve all settings that match the provided settings scope
            QSettings* settingsFor(Setting const& setting);

            /// \fn getSettingByName
            /// \brief Returns the setting with the matching name.
            static bool getSettingByName(QString const& settingName, const Setting **setting);

            typedef QMap<QString, QString> URLMap;

        public slots:
            void onLogout();

            void reportSettingFileError(const QString& settingsFilename, const QString& errorMessage,
                const bool readable, const bool writable, const unsigned int readSysError, const unsigned int writeSysError,
                const QString& windowsFileAttributes, const QString& permFlag) const;

        private:
            /// \fn get
            /// \brief 
            static QVariant get(Setting const& setting);

            /// \fn getAsAscii
            /// \brief when converting from byteArray to String, specifically does manual conversion instead of calling Qt's convert becuase Qt5 converts to utf8 while Qt4 converted as Ascii
            static QVariant getAsAscii(Setting const& setting);

            /// \fn set
            /// \brief 
            static bool set(Setting const& setting, QVariant const& value);

            /// \fn getMatchingSettings
            /// \brief Returns a list of settings whose names match the given regular expression.
            static QList<Setting const*> getMatchingSettings(QRegExp const& settingName);

            typedef enum { Set, Unset } SettingChange;
            typedef void (*SettingChangeCallback)(Setting const& setting, SettingChange change, Variant newValue);

            // \fn registerForChanges(QString const& settingName, SettingChangeCallback callback)
            // \brief Registers a callback for when the given setting changes (value is set or unset).
            //static void registerForChanges(QString const& settingName, SettingChangeCallback callback);

        signals:

            /// \fn settingsLoaded
            /// \brief Emitted when local settings have loaded
            void settingsLoaded();
            
            /// \fn serverUserSettingsLoaded
            /// \brief Emitted when user server-side settings have loaded
            void serverUserSettingsLoaded();

            /// \fn settingsSaved
            /// \brief A notification that is fired when the settings have finished saving.
            void settingsSaved();

            /// \fn settingChanged
            /// \brief A notification that is fired when a setting value is changed.
            void settingChanged(const QString& settingName, const Origin::Services::Variant& value);

            /// \fn settingError
            /// \brief A notification that is fired when an error occurs involving the settings (usually setting a setting value).
            void settingError(QString const& errorMessage);

            /// \fn writeSettingFailed
            /// \brief A notification that is fired when an error occurs involving the settings (usually setting a setting value).
            void writeSettingFailed(QString const& setting, QString const& errorMessage);

            /// \fn localPerAccountSettingFileError
            /// \brief A notification that is fired when the localPerAccountSettingFile doesn't sync.
            void localPerAccountSettingFileError(const QString settingsFilename, const QString errorMessage,
                const bool readable, const bool writable, const unsigned int readSysError, const unsigned int writeSysError,
                const QString windowsFileAttributes, const QString permFlag) const;
        
        private slots:
            void loadingFinished();
            void savingFinished();
        
            void initServerUserSettings();

            void serverUserValueChanged(QString key, QVariant value, bool fromServer);
            
        private:
            void registerServerUserSetting(const Setting &setting);

            typedef enum
            {
                StateUninitialized,
                StateLoading,
                StateReadyToUse,
                StateUnloading,
            }
            State;

            /// \fn isInitialized
            /// \brief Returns true if SettingsManager::init() has been called. NB: The SettingsManager may be
            /// initialized but not finished loading.
            static bool isInitialized();

            /// \fn ~SettingsManager
            /// \brief private destructor
            ~SettingsManager();

            /// \fn SettingsManager
            /// \brief Prevent SettingsManager from being used outside of init()/release() scope.
            SettingsManager();

            /// \fn SettingsManager
            /// \brief Prevent SettingsManager from being used outside of init()/release() scope.
            SettingsManager(SettingsManager const&);

            /// \fn SettingsManager
            /// \brief Prevent SettingsManager from being used copied.
            SettingsManager& operator=(SettingsManager const&);

            /// \fn internalGet
            /// \brief TBD
            QVariant internalGet(Setting const& setting) const;

            /// \fn internalSet
            /// \brief TBD
            bool internalSet(Setting const& setting, QVariant const& value);

            /// \fn importOldSettings
            /// \brief Import old Origin & EADM settings
            void importOldSettings(bool skipPerUserSettings, bool deleteOldSettings);

            /// \fn loadLocalMachineSettings
            /// \brief Loads the locally-stored, machine-specific settings
            void loadLocalMachineSettings();

            /// \fn loadLocalPerAccountSettings
            /// \brief Loads the locally-stored, per OS account-specific settings
            void loadLocalPerAccountSettings();

            /// \fn loadLocalUserSettings
            /// \brief Loads the locally-stored, user-specific settings
            void loadLocalUserSettings(const Session::SessionRef session);

            /// \fn unloadLocalUserSettings
            /// \brief TBD
            void unloadLocalUserSettings();

            /// \fn reportSyncFailure
            /// \brief Emits the localPerAccountSettingFileError signal; intended to 
            /// be called after a manual QSettings::sync() failure occurs.
            /// \sa SettingsManager::sync, SettingsManager::localPerAccountSettingFileError
            void reportSyncFailure(QSettings const& settings) const;

            static QList<QString> cloudSettingsFiles();

            static QScopedPointer<SettingsManager> s_instance;

            QSettings::Format const mXmlFormat;

            /// \fn addToInternalSettingsMap / removeFromInternalSettingsMap
            /// \brief add & remove a setting from our internal list
            static void addToInternalSettingsMap(QString settingName, Setting const *setting);
            static void removeFromInternalSettingsMap(QString settingName);
            static void initInternalSettingsMap();
            static void freeInternalSettingsMap();

            static QReadWriteLock *mInternalSettingsMapLock;
            typedef QHash<SettingsKey, Setting const *> InternalSettingsMap;
            static InternalSettingsMap *mInternalSettingsMap;

            /// \fn addToInternalSettingsOverrideMap / removeFromInternalSettingsOverrideMap
            /// \brief add & remove a setting from our internal override list
            static void addToInternalSettingsOverrideMap(QString settingName, Setting const *setting);
            static void removeFromInternalSettingsOverrideMap(QString settingName);
            static void initInternalSettingsOverrideMap();
            static void freeInternalSettingsOverrideMap();

            static QReadWriteLock *mInternalSettingsOverrideMapLock;
            typedef QHash<SettingsKey, Setting const *> InternalSettingsOverrideMap;
            static InternalSettingsOverrideMap *mInternalSettingsOverrideMap;

            static QReadWriteLock *mLocalMachineSettingsLock;
            QScopedPointer<QSettings> mLocalMachineSettings;
            static QReadWriteLock *mLocalUserSettingsLock;
            QScopedPointer<QSettings> mLocalUserSettings;
            static QReadWriteLock *mLocalPerAccountSettingsLock;
            QScopedPointer<QSettings> mLocalPerAccountSettings;

            static QReadWriteLock *mOverrideSettingsLock;
            QScopedPointer<QSettings> mOverrideSettings;

            static QReadWriteLock *mServerUserSettingsLock;
            ServerUserSettings::ServerUserSettingsManager *mServerUserSettings;
            QHash<QString, const Setting*> mServerUserSettingInstances; 

            Session::SessionRef mCurrentLocalSession;

            // all of the product override settings will have names that begin with certain prefixes - eg, "OverrideDownloadPath::",
            // as in "OverrideDownloadPath::71083=file:///C:/OriginGames/_cdn_tmp/tiger_2012.zip"
            static QSet<QString> mProductOverrideSettingsPrefixes;

            // legacy INI helper
            void legacyINIBySection(const QString& iniFile, const QString& sectionName);
            int legacyINIpreferenceGet(const QString& iniFile, const QString& sSection, const QString& sEntry, QString& sValue);
            void legacyINIPaths(const QString& iniFile, const QString& settingName);

        };

        // inline functions

        inline SettingsManager* SettingsManager::instance()
        {
            return s_instance.data();
        }

        inline bool SettingsManager::isInitialized()
        {
            return !s_instance.isNull();
        }

        inline bool SettingsManager::isLoaded()
        {
            if (s_instance.isNull())
                return false;
            
            // TBD: need to figure out how to report a meaningful value here
            return true;
        }

    } // namespace Services

} // namespace Origin

#endif // _EBISUCOMMON_SETTINGSMANAGER_
