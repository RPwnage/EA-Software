//  SettingsManager.cpp
//  Copyright 2011-2012 Electronic Arts Inc. All rights reserved.

#include "SettingsManager.h"
#include "EAIO/EAIniFile.h"     // legacy & override INI support
#include "services/debug/DebugService.h"
#include "services/crypto/CryptoService.h"
#include "services/platform/PlatformService.h"
#include "services/platform/TrustedClock.h"
#include "services/log/LogService.h"
#include "services/crypto/SimpleEncryption.h"
#include "services/config/OriginConfigService.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/voice/VoiceService.h"

#include "qdebug.h"

#include "ServerUserSettings.h"
#include "InGameHotKey.h"
//#include "services/settings/cloudsettings/FilesystemSupport.h"
#include "TelemetryAPIDLL.h"

//#include "SettingsSyncManager.h"

#include <QDesktopServices>
#include <QFile>

#if defined(ORIGIN_MAC)
#include <Carbon/Carbon.h>
//#include <HIToolbox/Events.h>
#include <cerrno>
#endif

using namespace Origin;
using namespace Origin::Services;

namespace
{
    /// \brief Takes the given settings object and attempts to set read/write
    /// permissions to the owner of the associated settings file and the
    /// current executing user.
    ///
    /// This is primarily to resolve settings files being mysteriously
    /// applied a read-only file attribute that appears to affect users (as
    /// observed via telemetry) (EBIBUGS-27578).
    void handleAccessError(QSettings& settings)
    {
        const QString settingsFilename(settings.fileName());
        QFile settingsFile(settingsFilename);

        // The AccessError may have been due to a read-only file attribute
        // on the settings file. Try and clear the read-only attribute.
        // :NOTE: the sync may have failed due to a variety of reasons (such
        // as lack of permissions)! For now, we simply try to remove the
        // read-only file attribute but may want to consider further restorative
        // measures for the future.
        const QFileDevice::Permissions existingPerms = settingsFile.permissions();
        const bool qtFixed = settingsFile.setPermissions(existingPerms | QFile::ReadOwner | QFile::WriteOwner | QFile::ReadUser | QFile::WriteUser);

        // On NTFS, there are cases where setPermissions will return true,
        // but there are still some conflicting Windows permissions in place
        // that prevent us from accessing the file.
        const bool readable = settingsFile.open(QIODevice::ReadOnly);
        settingsFile.close();
        const bool writable = settingsFile.open(QIODevice::Append);
        settingsFile.close();

        // Censor the settings filepath since it could contain the user's
        // windows user account name
        QString censoredFilename(settingsFilename);
        Services::Logger::CensorStringInPlace(censoredFilename);

        // Perform a re-sync if we now have read/write acces to the settings file
        const bool attemptResync = readable && writable;

        if (attemptResync)
        {
            GetTelemetryInterface()->Metric_SETTINGS_FILE_FIX_RESULT(EbisuTelemetryAPI::FixSuccess, readable, writable,
                    qtFixed, censoredFilename.toUtf8().constData());

            // Attempt to re-sync since we restored permissions back to a favorable
            // state (hopefully)
            settings.sync();
        }
        else
        {
            GetTelemetryInterface()->Metric_SETTINGS_FILE_FIX_RESULT(EbisuTelemetryAPI::FixFail,
                    readable, writable, qtFixed, censoredFilename.toUtf8().constData());
        }
    }
}

namespace Origin
{
    namespace Services
    {
        using namespace ServerUserSettings;

        Variant readSetting(const QString& settingName, const Session::SessionRef session)
        {
            // user setting ?
            if (!session.isNull())
            {
                ORIGIN_ASSERT_MESSAGE(Origin::Services::Session::SessionService::instance() && Origin::Services::Session::SessionService::isValidSession(session), "session is not valid!");
                Origin::Services::SettingsManager::instance()->loadLocalUserSettings(session);
            }

            const Setting *setting = NULL;
            if (Origin::Services::SettingsManager::instance()->getSettingByName(settingName, &setting))
            {
                return Variant(Origin::Services::SettingsManager::instance()->get(*setting));
            }

            return Variant();
        }

        Variant readSetting(const Setting& setting, const Session::SessionRef session)
        {
            // user setting ?
            if (!session.isNull())
            {
                ORIGIN_ASSERT_MESSAGE(Origin::Services::Session::SessionService::instance() && Origin::Services::Session::SessionService::isValidSession(session), "session is not valid!");
                Origin::Services::SettingsManager::instance()->loadLocalUserSettings(session);
            }

            return Variant(Origin::Services::SettingsManager::instance()->get(setting));
        }


        Variant readSettingAsAscii(const Setting& setting, const Session::SessionRef session)
        {
            // user setting ?
            if (!session.isNull())
            {
                ORIGIN_ASSERT_MESSAGE(Origin::Services::Session::SessionService::instance() && Origin::Services::Session::SessionService::isValidSession(session), "session is not valid!");
                Origin::Services::SettingsManager::instance()->loadLocalUserSettings(session);
            }

            return Variant(Origin::Services::SettingsManager::instance()->getAsAscii(setting));
        }


        void writeSetting(const Setting& setting, Variant const& value, const Session::SessionRef session)
        {
            // user setting ?
            if (!session.isNull())
            {
                ORIGIN_ASSERT_MESSAGE(Origin::Services::Session::SessionService::instance() && Origin::Services::Session::SessionService::isValidSession(session), "session is not valid!");
                Origin::Services::SettingsManager::instance()->loadLocalUserSettings(session);
            }
            QVariant data(value.toQVariant());

            // http://qt-project.org/doc/qt-4.8/QVariant.html#using-canconvert-and-convert-consecutively
            // Some type conversion may or may not work depending on the data, hence the two checks,
            // e.g. conversion from string to int: in general it works, hence canConvert() returns
            // true, but a particular string may not, hence convert() returns false, only if both
            // return true is the conversion valid.
            bool ok = false;
            QString  failReason = "InvalidConvert";

            if (data.canConvert((QVariant::Type)setting.type()))
            {
                failReason = "FailedConvert";
                if (data.convert((QVariant::Type)setting.type()))
                {
                    failReason = "SetFailed";
                    ok = Origin::Services::SettingsManager::instance()->set(setting, data);
                }
            }

            if (!ok)
            {
                ORIGIN_LOG_ERROR << "Writing setting '" << setting.name() << "' failed with error :" << failReason;
                ORIGIN_ASSERT_MESSAGE(false, "storing a setting failed!");
            }

        }

        void deleteSetting(const Setting& setting, const Session::SessionRef session)
        {
            // user setting ?
            if (!session.isNull())
            {
                ORIGIN_ASSERT_MESSAGE(Origin::Services::Session::SessionService::instance() && Origin::Services::Session::SessionService::isValidSession(session), "session is not valid!");
                Origin::Services::SettingsManager::instance()->loadLocalUserSettings(session);
            }
            Origin::Services::SettingsManager::instance()->unset(setting);
        }

        void reset(const Setting& setting, const Session::SessionRef session)
        {
            writeSetting(setting, setting.defaultValue(), session);
        }
        bool isDefault( const Setting& setting, const Session::SessionRef session)
        {
            return setting.defaultValue() == readSetting(setting, session);
        }

        void saveCookieToSetting(const Setting& setting, QNetworkCookie& cookie)
        {
            QString cookieValue = cookie.value();
            //save it out as ISODate so we don't have to worry about locales
            QString expirationDate(cookie.expirationDate().toUTC().toString(Qt::ISODate));
            QString settingValue = QString("%1||%2||%3||%4").arg(cookieValue).arg(cookie.domain()).arg(expirationDate).arg(cookie.path());
            Services::writeSetting(setting, settingValue);

            // write out timestamp for most recent setting of remember me or two-factor auth setting
            if ((setting.name() == Services::SETTING_REMEMBER_ME_PROD.name()) ||
                (setting.name() == Services::SETTING_REMEMBER_ME_INT.name()) ||
                (setting.name() == Services::SETTING_TFAID_PROD.name()) ||
                (setting.name() == Services::SETTING_TFAID_INT.name()))
            {
                qulonglong timestamp = Origin::Services::TrustedClock::instance()->nowUTC().toMSecsSinceEpoch() / 1000;
                Services::writeSetting(Services::SETTING_REMEMBERME_TFA_TIMESTAMP, timestamp);
            }
        }

        namespace ServerUserSettings
        {
            // These are keys both in our settings hash and in our server-side payload
            const QString EnableIgoForAllGamesKey("in_game_all_games");
            const QString EnableCloudSavesKey("cloud_saves_enabled");
            const QString EnableTelemetryKey("telemetry_enabled");

            // These are keys both in our settings hash and in our server-side payload
            const QString BroadcastTokenKey("video_broadcast_token");

            // These keys only exist in our settings hash
            const QString FavoriteProductIdsKey("favorite_product_ids");
            const QString HiddenProductIdsKey("hidden_product_ids");

            const QString OthOfferKey("last_dissmissed_oth_offer");

            // How long before we give up querying the server
            const unsigned int ServerQueryTimeoutMs(5000);
        }

        // URL hosts
        namespace urls
        {
            const static QString host("dm.origin.com");
            const static QString games(QString("atom") + QString(".") + host); // This should really move out to a separate Games service
            const static QString web(QString("web") + QString(".") + host);
            const static QString avatar(QString("avatar") + QString(".") + host);
            const static QString friends(QString("friends") + QString(".") + host);
            const static QString chat(QString("chat") + QString(".") + host);
            const static QString login(QString("loginregistration") + QString(".") + host);
            const static QString atom(QString("atom") + QString(".") + host);
            const static QString authentication(QString("loginregistration") + QString(".") + host);
            const static QString ecommerce(QString("ecommerce") + QString(".") + host + "/ecommerce");
            const static QString ecommerce2(QString("ecommerce2") + QString(".") + host + "/ecommerce2");
        }

#define STR_EXPAND(tok) #tok
        // string setting
#define DEFINE_EACORE_SETTING(name) \
    Setting SETTING_##name( STR_EXPAND(name) , Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);

        // bool setting
#define DEFINE_EACORE_SETTING_BOOL_FALSE(name) \
    Setting SETTING_##name( STR_EXPAND(name) , Variant::Bool, false, Setting::LocalAllUsers, Setting::ReadOnly);
#define DEFINE_EACORE_SETTING_BOOL_TRUE(name) \
    Setting SETTING_##name( STR_EXPAND(name) , Variant::Bool, true, Setting::LocalAllUsers, Setting::ReadOnly);

        // int setting
#define DEFINE_EACORE_SETTING_INT_VALUE(name, value) \
    Setting SETTING_##name( STR_EXPAND(name) , Variant::Int, value, Setting::LocalAllUsers, Setting::ReadOnly);

#define DEFINE_EACORE_SETTING_INT(name) DEFINE_EACORE_SETTING_INT_VALUE(name, 0)

#define DEFINE_EACORE_SETTING_FLOAT_VALUE(name, value) \
    Setting SETTING_##name( STR_EXPAND(name) , Variant::Float, value, Setting::LocalAllUsers, Setting::ReadOnly);

        /// define all Origin settings
        /// pre-define all available Origin settings
        const DEFINE_EACORE_SETTING_BOOL_FALSE(OverridesEnabled);

        /// OriginX settings
        /// stores whether we successfully loaded the SPA main page
        const Setting SETTING_ORIGINX_SPALOADED("SPALoaded", Variant::Bool, false, Setting::LocalPerAccount, Setting::ReadWrite);
        const Setting SETTING_ORIGINX_SPAPRELOADSUCCESS("SPAPreloadSuccess", Variant::Bool, false, Setting::LocalPerAccount, Setting::ReadWrite);
        const Setting SETTING_ORIGINX_SPALOCALE("SPALocale", Variant::String, "", Setting::LocalPerAccount, Setting::ReadWrite);

        /// legacy Origin settings
        const Setting SETTING_APPSIZEPOSITION("AppSizePosition", Variant::String, "0,0|0,0", Setting::LocalPerUser);
        const Setting SETTING_APPLOGOUTSCREENNUMBER("AppLogOutScreenNumber", Variant::Int, -1, Setting::LocalAllUsers);
        const Setting SETTING_FRIENDSLISTWINDOWSIZEPOSITION("FriendsListSizePosition", Variant::String, "0,0|0,0", Setting::LocalPerUser);
        const Setting SETTING_CHATWINDOWSSIZEPOSITION("ChatSizePosition", Variant::String, "-999,-999|-999,-999", Setting::LocalPerUser);
        const Setting SETTING_ACCEPTEDEULALOCATION("AcceptedEULALocation", Variant::String, "");
        const Setting SETTING_ACCEPTEDEULAVERSION("AcceptedEULAVersion", Variant::Int, -1);
        const Setting SETTING_AUTOPATCH("AutoPatch", Variant::Bool, true, Setting::LocalPerUser);
        const Setting SETTING_AUTOPATCHTIMESTAMP("AutoPatchTimestamp", Variant::ULongLong, 0, Setting::LocalPerUser);  
        const Setting SETTING_AUTOPATCHGLOBAL("AutoPatchGlobal", Variant::Bool, true, Setting::LocalAllUsers);
        const Setting SETTING_INSTALLTIMESTAMP("InstallTimestamp", Variant::ULongLong, 0, Setting::LocalAllUsers);    

        const Setting SETTING_AUTOUPDATE("AutoUpdate", Variant::Bool, false, Setting::LocalAllUsers, Setting::ReadWrite);
        const Setting SETTING_RUNONSYSTEMSTART("RunOnSystemStart", Variant::Bool, false, Setting::LocalAllUsers, Setting::ReadWrite);
        const Setting SETTING_BETAOPTIN("BetaOptIn", Variant::Bool, false, Setting::LocalAllUsers, Setting::ReadWrite);
        const Setting SETTING_USERAVATARCACHEURL("UserAvatarCacheURL", Variant::String, "", Setting::LocalPerUser);

        const Setting SETTING_DOWNLOADINPLACEDIR("DownloadInPlaceDir", Variant::String, SettingsManager::downloadInPlaceDefault(), Setting::LocalPerAccount);
        const Setting SETTING_EnableDownloaderSafeMode("EnableDownloaderSafeMode", Variant::Bool, false, Setting::LocalAllUsers);
        const Setting SETTING_DOWNLOAD_CACHEDIR("CacheDir", Variant::String, SettingsManager::downloadCacheDefault(), Setting::LocalPerAccount);
        const Setting SETTING_DOWNLOAD_CACHEDIRREMOVAL("CacheDirRemoval", Variant::Bool, false, Setting::LocalPerAccount);

        const Setting SETTING_OverrideRefreshEntitlementsOnDirtyBits("OverrideRefreshEntitlementsOnDirtyBits", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        const Setting SETTING_OverrideDirtyBitsEntitlementRefreshTimeout("OverrideDirtyBitsEntitlementRefreshTimeout", Variant::Int, 0, Setting::LocalAllUsers, Setting::ReadOnly);
        const Setting SETTING_OverrideDirtyBitsTelemetryDelay("OverrideDirtyBitsTelemetryDelay", Variant::Int, 0, Setting::LocalAllUsers, Setting::ReadOnly);
        const Setting SETTING_OverrideFullEntitlementRefreshThrottle("OverrideFullEntitlementRefreshThrottle", Variant::Int, -1, Setting::LocalAllUsers, Setting::ReadOnly);
        const Setting SETTING_AddonPreviewAllowed("AddonPreviewAllowed", Variant::Bool, false, Setting::LocalPerUser, Setting::ReadOnly);

        const Setting SETTING_FakeDirtyBitsMessageFile("FakeDirtyBitsMessageFile", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);

        const Setting SETTING_FIRSTLAUNCH("FirstLaunch", Variant::Bool, true, Setting::LocalAllUsers, Setting::ReadWrite);
        const Setting SETTING_COMMANDLINE("CommandLine", Variant::String, "", Setting::LocalAllUsers);  // remove after usage

        const Setting SETTING_ENABLEINGAMELOGGING("EnableInGameLogging", Variant::Bool, false, Setting::LocalPerUser);
        const Setting SETTING_HIDEEXITCONFIRMATION("HideExitConfirmation", Variant::Bool, false, Setting::LocalPerUser);

        const Setting SETTING_HIDEOFFLINELOGINMODEDUETOEASERVERSNOTICE("HideOfflineLoginModeDueToEAServersNotice", Variant::Bool, false, Setting::LocalPerUser);
        const Setting SETTING_HIDEOFFLINELOGINMODEDUETONETWORKNOTICE("HideOfflineLoginModeDueToNetworkNotice", Variant::Bool, false, Setting::LocalPerUser);
        const Setting SETTING_OverrideLoginPageRefreshTimer("OverrideLoginPageRefreshTimer", Variant::Int, 0, Setting::LocalAllUsers, Setting::ReadOnly);
        const Setting SETTING_INGAME_HOTKEY("HotKey", Variant::ULongLong, SettingsManager::inGameHotkeyDefault(), Setting::LocalPerUser);
        const Setting SETTING_INGAME_HOTKEYSTRING("HotKeyString", Variant::String, "", Setting::LocalPerUser);
        const Setting SETTING_PIN_HOTKEY("PinHotKey", Variant::ULongLong, SettingsManager::pinHotkeyDefault(), Setting::LocalPerUser);
        const Setting SETTING_PIN_HOTKEYSTRING("PinHotKeyString", Variant::String, "", Setting::LocalPerUser);
        const Setting SETTING_EnableIgo("EnableIgo", Variant::Bool, true, Setting::LocalPerUser);
        const Setting SETTING_ISBETA("IsBeta", Variant::Bool, false, Setting::LocalPerUser);
        const Setting SETTING_KEEPINSTALLERS("KeepInstallers", Variant::Bool, false, Setting::LocalPerAccount, Setting::ReadWrite);
        const Setting SETTING_LOCALHOSTENABLED("LocalHostEnabled", Variant::Bool, true, Setting::LocalPerAccount, Setting::ReadWrite);
        const Setting SETTING_LOCALHOSTRESPONDERENABLED("LocalHostResponderEnabled", Variant::Bool, true, Setting::LocalAllUsers, Setting::ReadWrite);
        const Setting SETTING_CREATEDESKTOPSHORTCUT("CreateDesktopShortcut", Variant::Bool, true, Setting::LocalPerUser, Setting::ReadWrite);
        const Setting SETTING_CREATESTARTMENUSHORTCUT("CreateStartMenuShortcut", Variant::Bool, true, Setting::LocalPerUser, Setting::ReadWrite);


        Setting SETTING_LOCALE("Locale", Variant::String, "", Setting::LocalAllUsers, Setting::ReadWrite);  // correct default value will be set later
        Setting SETTING_StoreLocale("StoreLocale", Variant::String, "", Setting::LocalAllUsers, Setting::ReadWrite);

        const QString SETTING_UNKNOWN_PROMO_CACHE = "unknown";
        const QString PROMOBROWSEROFFERCACHE_NO_ENTITLEMENTS_SENTINEL = "opm_none";
        Setting SETTING_PromoBrowserOfferCache("PromoBrowserOfferCache", Variant::String, SETTING_UNKNOWN_PROMO_CACHE, Setting::LocalPerUser, Setting::ReadWrite, Setting::Encrypted);
        Setting SETTING_OverridePromoBrowserOfferCache("OverridePromoBrowserOfferCache", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);

        // LoginAsInvisible needs to be Setting::LocalAllUsers instead of Setting::LocalPerUser because the value is needed
        // at the login window before a user has logged in.  In 8.6, starting up Origin the checkbox is already set if the last user
        // set it so we need to mimic that in 9.0 as well (EBIBUGS-16175).
        const Setting SETTING_LOGIN_AS_INVISIBLE("LoginAsInvisible", Variant::Bool, false, Setting::LocalPerUser);
        const Setting SETTING_LOGINEMAIL("LoginEmail", Variant::String, "", Setting::LocalAllUsers, Setting::ReadWrite, Setting::Encrypted);

        const Setting SETTING_NOTIFY_FINISHEDDOWNLOAD("FinishedDownloadNotification", Variant::Int, 3, Setting::LocalPerUser);
        const Setting SETTING_NOTIFY_DOWNLOADERROR("DownloadErrorNotification", Variant::Int, 3, Setting::LocalPerUser);
        const Setting SETTING_NOTIFY_FINISHEDINSTALL("FinishedInstallNotification", Variant::Int, 3, Setting::LocalPerUser);
        const Setting SETTING_NOTIFY_FRIENDQUITSGAME("FriendQuitsGameNotification", Variant::Int, 3, Setting::LocalPerUser);
        const Setting SETTING_NOTIFY_FRIENDSIGNINGIN("FriendSigningInNotification", Variant::Int, 3, Setting::LocalPerUser);
        const Setting SETTING_NOTIFY_FRIENDSIGNINGOUT("FriendSigningOutNotification", Variant::Int, 3, Setting::LocalPerUser);
        const Setting SETTING_NOTIFY_FRIENDSTARTSGAME("FriendStartsGameNotification", Variant::Int, 3, Setting::LocalPerUser);
        const Setting SETTING_NOTIFY_INCOMINGTEXTCHAT("IncomingTextChatNotification", Variant::Int, 3, Setting::LocalPerUser);
        const Setting SETTING_NOTIFY_INVITETOCHATROOM("InviteToChatRoomNotification", Variant::Int, 3, Setting::LocalPerUser);
        const Setting SETTING_NOTIFY_INCOMINGCHATROOMMESSAGE("IncomingChatRoomMessageNotification", Variant::Int, 3, Setting::LocalPerUser);
        const Setting SETTING_NOTIFY_NONFRIENDINVITE("NonFriendInviteNotification", Variant::Int, 3, Setting::LocalPerUser);
        const Setting SETTING_NOTIFY_GAMEINVITE("GameInviteNotification", Variant::Int, 3, Setting::LocalPerUser);
        const Setting SETTING_NOTIFY_INCOMINGCECHAT("IncomingCEChatNotification", Variant::Int, 3, Setting::LocalPerUser);
        const Setting SETTING_NOTIFY_FRIENDSTARTBROADCAST("FriendStartBroadcastNotification", Variant::Int, 3, Setting::LocalPerUser);
        const Setting SETTING_NOTIFY_FRIENDSTOPBROADCAST("FriendStopBroadcastNotification", Variant::Int, 3, Setting::LocalPerUser);
        const Setting SETTING_NOTIFY_GROUPCHATINVITE("GroupChatInviteNotification", Variant::Int, 3, Setting::LocalPerUser);
        const Setting SETTING_NOTIFY_INCOMINGVOIPCALL("IncomingVoIPCallNotification", Variant::Int, 3, Setting::LocalPerUser);
        const Setting SETTING_NOTIFY_ACHIEVEMENTUNLOCKED("AchievementNotification", Variant::Int, 3, Setting::LocalPerUser);

        const Setting SETTING_PROMOBROWSER_ORIGINSTARTED_LASTSHOWN("PromoBrowserOriginStartedLastShown", Variant::Int, -1, Setting::LocalPerUser);
        const Setting SETTING_PROMOBROWSER_GAMEFINISHED_LASTSHOWN("PromoBrowserGameFinishedLastShown", Variant::Int, -1, Setting::LocalPerUser);
        const Setting SETTING_PROMOBROWSER_DOWNLOADUNDERWAY_LASTSHOWN("PromoBrowserDownloadUnderwayLastShown", Variant::Int, -1, Setting::LocalPerUser);
        const Setting SETTING_NETPROMOTER_LASTSHOWN("NetPromoterLastShown", Variant::Int, -1, Setting::LocalPerUser);
        const Setting SETTING_NETPROMOTER_GAME_LASTSHOWN("NetPromoterGameLastShown", Variant::Int, -1, Setting::LocalPerUser);
        const Setting SETTING_ONTHEHOUSE_VERSIONS_SHOWN("OnTheHouseVersionsShown", Variant::String, "", Setting::LocalPerUser);
        const Setting SETTING_OverrideOnTheHouseQueryIntervalMS("OverrideOnTheHouseQueryIntervalMS", Variant::Int, 21600000, Setting::LocalAllUsers, Setting::ReadOnly);
        const Setting SETTING_SUBSCRIPTION_ERROR_MESSAGE_LASTSHOWN("SubscriptionErrorMessageLastShown", Variant::Int, -1, Setting::LocalPerUser);

        const Setting SETTING_SELECTEDFILTER("SelectedFilter", Variant::Int, SETTINGVALUE_GAMES_FILTER_ALL, Setting::LocalPerUser);
        const Setting SETTING_SHOWIGONUX("ShowIGONux", Variant::Bool, true, Setting::LocalPerUser);
        const Setting SETTING_SHOWIGONUXFIRSTTIME("ShowIGONuxFirstTime", Variant::Bool, true, Setting::LocalPerUser);
        const Setting SETTING_MULTILAUNCHPRODUCTIDSANDTITLES("MultiLaunchProductIdsAndTitles", Variant::String, "", Setting::LocalPerUser);
        const Setting SETTING_OIGDISABLEDGAMES("OigDisabledGames", Variant::String, "", Setting::LocalPerUser);
        const Setting SETTING_FIRSTLAUNCHMESSAGESHOWN("FirstLaunchMessageShown", Variant::String, "", Setting::LocalPerUser);
        const Setting SETTING_UPGRADEDSAVEGAMESWARNED("UpradedSaveGamesWarned", Variant::String, "", Setting::LocalPerUser);
        const Setting SETTING_GAMECOMMANDLINEARGUMENTS("GameCommandLineArguments", Variant::String, "", Setting::LocalPerUser);
        const Setting SETTING_SHOWTIMESTAMPS("ShowTimeStamps", Variant::Bool, true, Setting::LocalPerUser);
        const Setting SETTING_SAVECONVERSATIONHISTORY("SaveConversationHistory", Variant::Bool, false, Setting::LocalPerUser);
        const Setting SETTING_TIMESHIDENOTIFICATION("TimesHideNotification", Variant::Int, 0, Setting::LocalPerUser);
        const Setting SETTING_VIEWMODE("ViewMode", Variant::Int, SETTINGVALUE_GAMES_VIEW_MODE_GRID, Setting::LocalPerUser);
        const Setting SETTING_3PDD_PLAYDIALOG_SHOW("3PDDShowPlayDialog", Variant::Bool, true, Setting::LocalPerUser);
        const Setting SETTING_JUMPLIST_RECENTLIST("JumplistRecentList", Variant::String, "", Setting::LocalPerUser);
        const Setting SETTING_DISABLE_PROMO_MANAGER("DisablePromoManager", Variant::Bool, false, Setting::LocalPerUser, Setting::ReadWrite);
        const Setting SETTING_IGNORE_NON_MANDATORY_GAME_UPDATES("IgnoreNonMandatoryGameUpdates", Variant::Bool, false, Setting::LocalPerUser, Setting::ReadWrite);
        const Setting SETTING_SHUTDOWN_ORIGIN_ON_GAME_FINISHED("ShutDownOriginOnGameFinished", Variant::Bool, false, Setting::LocalPerUser, Setting::ReadWrite);

        // Feature callouts
        const Setting SETTING_CALLOUT_SOCIALUSERAREA_SHOWN("Callout_SocialUserAreaShown", Variant::Bool, false, Setting::LocalPerUser, Setting::ReadWrite);
        const Setting SETTING_CALLOUT_GROUPSTAB_SHOWN("Callout_GroupsTabShown", Variant::Bool, false, Setting::LocalPerUser, Setting::ReadWrite);
        const Setting SETTING_CALLOUT_VOIPBUTTON_ONE_TO_ONE_SHOWN("Callout_VoipButtonOneToOneShown", Variant::Bool, false, Setting::LocalPerUser, Setting::ReadWrite);
        const Setting SETTING_CALLOUT_VOIPBUTTON_GROUP_SHOWN("Callout_VoipButtonGroupShown", Variant::Bool, false, Setting::LocalPerUser, Setting::ReadWrite);

        // Telemetry HW Survey opt out
        const Setting SETTING_HW_SURVEY_OPTOUT("TelemetryHWSurveyOptOut", Variant::Bool, false, Setting::LocalAllUsers, Setting::ReadWrite);
        // Telemetry Crash data opt out (Possible values for this setting are: always (0), never (1) , ask me (2)
        const Setting SETTING_CRASH_DATA_OPTOUT("TelemetryCrashDataOptOut", Variant::Int, Origin::Services::Default, Setting::LocalAllUsers, Setting::ReadWrite);

        const Setting SETTING_OverrideIGOSetting("OverrideIGOSetting", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);

        const QString SETTING_INVALID_REMEMBER_ME = "invalid";
        // Contains the remember me cookie value and expiration date (format = Qt::TextDate) separated by a comma
        const Setting SETTING_REMEMBER_ME_INT("RememberMeInt", Variant::String, SETTING_INVALID_REMEMBER_ME, Setting::LocalPerAccount, Setting::ReadWrite, Setting::Encrypted);
        const Setting SETTING_REMEMBER_ME_PROD("RememberMeProd", Variant::String, SETTING_INVALID_REMEMBER_ME, Setting::LocalPerAccount, Setting::ReadWrite, Setting::Encrypted);

        const QString SETTING_INVALID_REMEMBER_ME_USERID = "invalid";
        // Contains the remember me cookie value and expiration date (format = Qt::TextDate) separated by a comma
        const Setting SETTING_REMEMBER_ME_USERID_INT("RememberMeUserIdInt", Variant::String, SETTING_INVALID_REMEMBER_ME_USERID, Setting::LocalPerAccount, Setting::ReadWrite, Setting::Encrypted);
        const Setting SETTING_REMEMBER_ME_USERID_PROD("RememberMeUserIdProd", Variant::String, SETTING_INVALID_REMEMBER_ME_USERID, Setting::LocalPerAccount, Setting::ReadWrite, Setting::Encrypted);

        const QString SETTING_INVALID_MOST_RECENT_USER_NAME = "invalid";
        // Contains the last username to login cookie value
        const Setting SETTING_MOST_RECENT_USER_NAME_INT("LastUsernameToLoginInt", Variant::String, SETTING_INVALID_MOST_RECENT_USER_NAME, Setting::LocalPerAccount, Setting::ReadWrite, Setting::Encrypted);
        const Setting SETTING_MOST_RECENT_USER_NAME_PROD("LastUsernameToLoginProd", Variant::String, SETTING_INVALID_MOST_RECENT_USER_NAME, Setting::LocalPerAccount, Setting::ReadWrite, Setting::Encrypted);

        const QString SETTING_INVALID_TFAID = "invalid";
        // Contains the TFAid cookie value and expiration date (format = Qt::TextDate) separated by a comma
        const Setting SETTING_TFAID_INT("TFAIdInt", Variant::String, SETTING_INVALID_TFAID, Setting::LocalPerAccount, Setting::ReadWrite, Setting::Encrypted);
        const Setting SETTING_TFAID_PROD("TFAIdProd", Variant::String, SETTING_INVALID_TFAID, Setting::LocalPerAccount, Setting::ReadWrite, Setting::Encrypted);

        // Used to track last time RememberMeProd/Int or TFAIdProd/Int was changed by the user so when re-installing we can invalidate (ORPLAT-1093)
        const Setting SETTING_REMEMBERME_TFA_TIMESTAMP("RememberMeTFATimestamp", Variant::ULongLong, 0, Setting::LocalPerAccount);

        //Environment specific settings
        //settings that are stored off as environment specific have the environment name appended to the setting name (except for production)
        const QString SETTING_ENV_production = env::production; // for comparison in cases where we need to append the environment name to the setting to store the setting as environment specific
        // for production, setting will not have environment appended
        //////////////////////////////////////////////////////////////////////////
        Setting SETTING_StoreInitialURL("StoreInitialURL", Variant::String, "", Setting::LocalPerUser, Setting::ReadOnly);
        Setting SETTING_SubscriptionRedemptionURL("SubscriptionRedemptionURL", Variant::String, "", Setting::LocalPerUser, Setting::ReadOnly);
        Setting SETTING_StoreProductURL("StoreProductURL", Variant::String, "", Setting::LocalPerUser, Setting::ReadOnly);
        Setting SETTING_StoreMasterTitleURL("StoreMasterTitleURL", Variant::String, "", Setting::LocalPerUser, Setting::ReadOnly);
        Setting SETTING_StoreEntitleFreeGameURL("StoreEntitleFreeGameURL", Variant::String, "", Setting::LocalPerUser, Setting::ReadOnly);

        const Setting SETTING_DefaultTab("DefaultTab", Variant::String, "decide", Setting::LocalPerUser);
        const Setting SETTING_EnableJTL("EnableJTL", Variant::String, "T", Setting::LocalPerUser, Setting::ReadOnly);
        Setting SETTING_FreeGamesURL("FreeGamesURL", Variant::String, "", Setting::LocalPerUser, Setting::ReadOnly);
        //////////////////////////////////////////////////////////////////////////
        /// connection URLs
        // TODO: make sure all of these are really in mURLs
        Setting SETTING_RegistrationURL("RegistrationURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_ActivationURL("ActivationURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_DirtyBitsServiceHostname("DirtyBitsServiceHostName", Variant::String, "dev.dirtybits.dm.origin.com", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_DirtyBitsServiceResource("DirtyBitsServiceResource", Variant::String, "/dirtybits/events/", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_ChangePasswordURL("ChangePasswordURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_NewUserExpURL("NewUserExpURL", Variant::String, "https://account.origin.com/cp-ui/avatar/index", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_SupportURL("SupportURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_OriginFaqURL("OriginFaqURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_EbisuForumURL("EbisuForumURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_EbisuForumSSOURL("EbisuForumSSOURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_EbisuForumFranceURL("EbisuForumFranceURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_EbisuForumFranceSSOURL("EbisuForumFranceSSOURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_GameDetailsURL("GameDetailURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_MotdURL("MotdURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_ReleaseNotesURL("ReleaseNotesURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_EulaURL("EulaURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_ToSURL("ToSURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_PrivacyPolicyURL("PrivacyPolicyKey", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_PromoURL("PromoURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_PromoV1URL("PromoV1URL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_PromoV2URL("PromoV2URL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_EmptyShelfURL("EmptyShelfURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_FriendSearchURL("FriendSearchURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_DlgFriendSearchURL("DlgFriendSearchURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_GamerProfileURL("GamerProfileURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_WizardURL("WizardURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_HeartbeatURL("HeartbeatURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_EnvironmentName("EnvironmentName", Variant::String, "production", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_PdlcStoreLegacyURL("PdlcStoreLegacyURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_PdlcStoreURL("PdlcStoreURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_EbisuLockboxURL("EbisuLockboxURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_EbisuLockboxV3URL("EbisuLockboxV3URL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_CustomerSupportHomeURL("CustomerSupportHomeURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_CustomerSupportURL("CustomerSupportURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_EbisuCustomerSupportChatURL("EbisuCustomerSupportChatURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_CloudSyncAPIURL("CloudSyncApiURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_UserIdURL("UserIdURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_ForgotPasswordURL("ForgotPasswordURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_OriginAccountURL("OriginAccountURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_CloudSavesBlacklistURL("CloudSavesBlacklistURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_UpdateURL("UpdateURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_RedeemCodeReturnURL("RedeemCode.ReturnUrl", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_AvatarURL("AvatarURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_FriendsURL("FriendsURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_ChatURL("ChatURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_EcommerceURL("EcommerceURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_EcommerceV1URL("EcommerceV1URL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_LoginRegistrationURL("LoginRegistrationURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_GamesServiceURL("GamesServiceURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_NetPromoterURL("NetPromoterURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_OnTheHousePromoURL("OnTheHousePromoURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_OnTheHouseStoreURL("OnTheHouseStoreURL", Variant::String, "", Setting::LocalPerUser, Setting::ReadOnly);
        Setting SETTING_SubscriptionStoreURL("SubscriptionStoreURL", Variant::String, "", Setting::LocalPerUser, Setting::ReadOnly);
        Setting SETTING_SubscriptionVaultURL("SubscriptionVaultURL", Variant::String, "", Setting::LocalPerUser, Setting::ReadOnly);
        Setting SETTING_SubscriptionFaqURL("SubscriptionFaqURL", Variant::String, "", Setting::LocalPerUser, Setting::ReadOnly);
        Setting SETTING_SubscriptionInitialURL("SubscriptionInitialURL", Variant::String, "", Setting::LocalPerUser, Setting::ReadOnly);
        Setting SETTING_FeedbackPageURL("FeedbackPageURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_StoreOrderHistoryURL("StoreOrderHistoryURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_StoreSecurityURL("StoreSecurityURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_AccountSubscriptionURL("AccountSubscriptionURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_StorePrivacyURL("StorePrivacyURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_StorePaymentAndShippingURL("StorePaymentAndShippingURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_StoreRedemptionURL("StoreRedemptionURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_VoiceURL("VoiceURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_GroupsURL("GroupsURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);

        Setting SETTING_IGODefaultURL("IGODefaultURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_IGODefaultSearchURL("IGODefaultSearchURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_InicisSsoCheckoutBaseURL("InicisSsoCheckoutBaseURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_GNUURL("GNUURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_GPLURL("GPLURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_SteamSupportURL("SteamSupportURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_OriginXURL("OriginXURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);

        //////////////////////////////////////////////////////////////////////////
        // Atom URL Settings
        Setting SETTING_AtomURL("AtomURL", Variant::String, "https://qa.atom.dm.origin.com/atom", Setting::LocalAllUsers, Setting::ReadOnly);

        //////////////////////////////////////////////////////////////////////////
        // Achievement sharing URL Settings
        Setting SETTING_AchievementSharingURL("AchievementSharingURL", Variant::String, "https://gateway.int.ea.com/proxy/identity/pids/%1/privacysettings", Setting::LocalAllUsers, Setting::ReadOnly);

        //////////////////////////////////////////////////////////////////////////
        // COPPA Help Urls
        Setting SETTING_COPPADownloadHelpURL("COPPADownloadHelpURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_COPPAPlayHelpURL("COPPAPlayHelpURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);



        //////////////////////////////////////////////////////////////////////////
        // Notification value
        Setting SETTING_NotificationExpiryTime("NotificationExpiryTime", Variant::Int, 720, Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_DisableNotifications("DisableNotifications", Variant::Bool, false, Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_DisableIdling("DisableIdling", Variant::Bool, false, Setting::LocalAllUsers, Setting::ReadOnly);

        //////////////////////////////////////////////////////////////////////////
        /// Navigation Debugging
        Setting SETTING_NavigationDebugging("NavigationDebugging", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);


        // Every environment except prod uses the same URL
        Setting SETTING_WebDispatcherURL("WebDispatcherURL", Variant::String, "https://integration.connect.origin.com/origin/web_dispatch", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_WebWidgetUpdateURL("WebWidgetUpdateURL", Variant::String, "https://stage.secure.download.dm.origin.com/widgets/{appVersion}/{widgetName}.wgt", Setting::LocalAllUsers, Setting::ReadOnly);

        // Setting to find where the MyOrigin server configuration file is stored.
        Setting SETTING_AchievementsBasePageURL("AchievementsBasePageURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);

        Setting SETTING_OverrideInitURL("OverrideInitURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);

        // Override for the MyOrigin Page. This setting has priority over all other settings.
        Setting SETTING_MyOriginPageURL("MyOriginPageURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);

        void initializeURLSettings()
        {
            // if you are adding new URLs, please do not use a setting to do so. Add the URL to the
            // OriginConfigService and add the necessary hacks in OriginConfigService::parseConfiguration(),
            // and use URI::get() to retrieve the URL.

            SETTING_DirtyBitsServiceHostname.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::dirtyBitsHostName));
            SETTING_ActivationURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::activationURL));
            SETTING_RegistrationURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::registrationURL));
            SETTING_NetPromoterURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::netPromoterURL));
            SETTING_NewUserExpURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::newUserExpURL));
            SETTING_FriendSearchURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::friendSearchURL));
            SETTING_GamerProfileURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::gamerProfileURL));
            SETTING_DlgFriendSearchURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::dlgFriendSearchURL));
            SETTING_ChangePasswordURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::changePasswordURL));
            SETTING_WizardURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::wizardURL));
            SETTING_PdlcStoreLegacyURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::pdlcStoreLegacyURL));
            SETTING_PdlcStoreURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::pdlcStoreURL));
            SETTING_PromoURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::promoURL));
            SETTING_EbisuLockboxURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::ebisuLockboxURL));
            SETTING_EbisuLockboxV3URL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::ebisuLockboxV3URL));
            SETTING_EbisuCustomerSupportChatURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::ebisuCustomerSupportChatURL));
            SETTING_CloudSyncAPIURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::cloudSyncAPIURL));
            SETTING_CustomerSupportHomeURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::customerSupportHomeURL));
            SETTING_CustomerSupportURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::customerSupportURL));
            SETTING_OriginFaqURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::originFaqURL));
            SETTING_UpdateURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::updateURL));
            SETTING_CloudSavesBlacklistURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::cloudSavesBlacklistURL));
#ifdef ORIGIN_MAC
            SETTING_MotdURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::macMotdURL));
#else
            SETTING_MotdURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::motdURL));
#endif
            SETTING_ReleaseNotesURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::releaseNotesURL));
            SETTING_EmptyShelfURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::emptyShelfURL));

#ifdef ORIGIN_MAC
            SETTING_EulaURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::eulaURL_mac));
#else
            SETTING_EulaURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::eulaURL));
#endif

            SETTING_ForgotPasswordURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::forgotPasswordURL));
            SETTING_WebDispatcherURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::webDispatcherURL));
            SETTING_GamesServiceURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::gamesServiceURL));
            SETTING_AvatarURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::avatarURL));
            SETTING_LoginRegistrationURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::loginRegistrationURL));

            SETTING_EcommerceV1URL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::ecommerceV1URL));
            SETTING_EcommerceURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::ecommerceV2URL));

            SETTING_PromoURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::promoURL));
            SETTING_FriendsURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::friendsURL));
            SETTING_ChatURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::chatURL));
            SETTING_AchievementServiceURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::achievementServiceURL));
            SETTING_TimedTrialServiceURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::timedTrialServiceURL));
            SETTING_RedeemCodeReturnURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::redeemCodeReturnURL));
            SETTING_SupportURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::supportURL));
            SETTING_EbisuForumURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::ebisuForumURL));
            SETTING_EbisuForumSSOURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::ebisuForumSSOURL));
            SETTING_EbisuForumFranceURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::ebisuForumFranceURL));
            SETTING_EbisuForumFranceSSOURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::ebisuForumFranceSSOURL));
            SETTING_ToSURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::toSURL));
            SETTING_PrivacyPolicyURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::privacyPolicyURL));
            SETTING_HeartbeatURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::heartbeatURL));

            SETTING_FreeGamesURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::freeGamesURLV2));
            SETTING_OnTheHouseStoreURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::onTheHouseStoreURL));
            SETTING_OnTheHousePromoURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::onTheHousePromoURL));
            SETTING_SubscriptionStoreURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::subscriptionStoreURL));
            SETTING_SubscriptionVaultURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::subscriptionVaultURL));
            SETTING_SubscriptionFaqURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::subscriptionFaqURL));
            SETTING_SubscriptionInitialURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::subscriptionInitialURL));
            SETTING_StoreProductURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::storeProductURLV2));
            SETTING_VoiceURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::voiceURL));
            SETTING_GroupsURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::groupsURL));
            SETTING_SubscriptionServiceURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::subscriptionServiceURL));
            SETTING_SubscriptionVaultServiceURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::subscriptionVaultServiceURL));
            SETTING_SubscriptionTrialEligibilityServiceURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::subscriptionTrialEligibilityServiceURL));
#ifdef ORIGIN_MAC
            SETTING_StoreInitialURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::storeInitialURLV2_mac));
#else
            SETTING_StoreInitialURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::storeInitialURLV2));
#endif
            SETTING_SubscriptionRedemptionURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::subscriptionRedemptionURL));

            SETTING_StoreMasterTitleURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::storeMasterTitleURL));
            SETTING_StoreEntitleFreeGameURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::storeEntitleFreeGameURL));

            SETTING_PromoURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::promoURL));
            SETTING_OriginAccountURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::originAccountURL));
            SETTING_StoreOrderHistoryURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::storeOrderHistoryURL));
            SETTING_StoreSecurityURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::storeSecurityURL));
            SETTING_AccountSubscriptionURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::accountSubscriptionURL));
            SETTING_StorePrivacyURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::storePrivacyURL));
            SETTING_StorePaymentAndShippingURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::storePaymentAndShippingURL));
            SETTING_StoreRedemptionURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::storeRedemptionURL));
            SETTING_WebWidgetUpdateURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::webWidgetUpdateURL));
            SETTING_AtomURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::atomServiceURL));
            SETTING_AchievementSharingURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::achievementSharingURL));
            SETTING_ConnectPortalBaseUrl.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::connectPortalBaseURL));
            SETTING_SignInPortalBaseUrl.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::signInPortalBaseURL));
            SETTING_IdentityPortalBaseUrl.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::identityPortalBaseURL));
            SETTING_OnlineLoginServiceUrl.updateDefaultValue(OriginConfigService::instance()->miscConfig().onlineLoginPath);
            SETTING_OfflineLoginServiceUrl.updateDefaultValue(OriginConfigService::instance()->miscConfig().offlineLoginPath);

            SETTING_COPPADownloadHelpURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::COPPADownloadHelpURL));
            SETTING_COPPAPlayHelpURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::COPPAPlayHelpURL));

            SETTING_IGODefaultURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::igoDefaultURL));
            SETTING_IGODefaultSearchURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::igoDefaultSearchURL));
            SETTING_InicisSsoCheckoutBaseURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::inicisSsoCheckoutBaseURL));
            SETTING_GNUURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::gnuURL));
            SETTING_GPLURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::gplURL));
            SETTING_SteamSupportURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::steamSupportURL));

            SETTING_OriginXURL.updateDefaultValue(OriginConfigService::instance()->serviceUrl(URI::OriginXURL));

        }

        void initializePerSessionSettings()
        {
            // We have to do this here because when we are setting the values of the settings - localization
            // hasn't been initialized yet.
            // OIG hot key
            InGameHotKey settingHotKeys = InGameHotKey::fromULongLong(readSetting(SETTING_INGAME_HOTKEY, Session::SessionService::currentSession()));
            writeSetting(SETTING_INGAME_HOTKEYSTRING, settingHotKeys.asNativeString(), Session::SessionService::currentSession());
            // OIG Pinning hot key
            settingHotKeys = InGameHotKey::fromULongLong(readSetting(SETTING_PIN_HOTKEY, Session::SessionService::currentSession()));
            writeSetting(SETTING_PIN_HOTKEYSTRING, settingHotKeys.asNativeString(), Session::SessionService::currentSession());
#if ENABLE_VOICE_CHAT
            bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
            if( isVoiceEnabled )
            {
                // Chat push to talk hot key
                qulonglong pushtotalk_setting = readSetting(SETTING_PushToTalkKey, Session::SessionService::currentSession());
                if (pushtotalk_setting) // only default write string if there is a value in it.  0 means mouse button.
                {
                    settingHotKeys = InGameHotKey::fromULongLong(pushtotalk_setting);
                    writeSetting(SETTING_PushToTalkKeyString, settingHotKeys.asNativeString(), Session::SessionService::currentSession());
                }
            }
#endif //ENABLE_VOICE_CHAT

            qulonglong install_ts   = readSetting(SETTING_INSTALLTIMESTAMP, Session::SessionService::currentSession());
            qulonglong autopatch_ts = readSetting(SETTING_AUTOPATCHTIMESTAMP, Session::SessionService::currentSession());

            if ((SettingsManager::instance()->isSettingSpecified(SETTING_AUTOPATCH) == false) ||
                (install_ts > autopatch_ts))    // if install timestamp is more recent than the last time user set AutoPatch(aka "Keep Games Up To Date") use the AutoPatchGlobal setting from the installer dialog. (ORPLAT-1034)
            {
                writeSetting(SETTING_AUTOPATCH, readSetting(SETTING_AUTOPATCHGLOBAL, Session::SessionService::currentSession()), Session::SessionService::currentSession());
            }

            // Reset OIG logging to default. We want this setting to be turned on per session.
            reset(SETTING_ENABLEINGAMELOGGING);

            // Reset our broadcast game name
            writeSetting(SETTING_BROADCAST_GAMENAME, SETTING_BROADCAST_GAMENAME.defaultValue(), Session::SessionService::currentSession());
        }

        // Override to show the MyOrigin Tab when there is no Configuration file.
        Setting SETTING_ShowMyOrigin("ShowMyOrigin", Variant::Bool, false, Setting::LocalAllUsers, Setting::ReadOnly);

        // parameters passed in as Url into the commmandline -- used mostly for RTP, but also for "store", "login", "library"
        const Setting SETTING_UrlHost("urlHost", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        const Setting SETTING_CmdLineAuthToken("cmdLineAuthToken", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        const Setting SETTING_UrlStoreProductBarePath("urlStoreProductBarePath", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        const Setting SETTING_UrlLibraryGameIds("urlGameIds", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        const Setting SETTING_UrlRefresh("urlRefresh", Variant::Bool, false, Setting::LocalAllUsers, Setting::ReadOnly);
        const Setting SETTING_UrlGameTitle("urlGameTitle", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        const Setting SETTING_UrlSSOtoken("urlSSOtoken", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        const Setting SETTING_UrlSSOauthCode("urlSSOauthcode", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        const Setting SETTING_UrlSSOauthRedirectUrl("urlSSOauthRedirectUrl", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        const Setting SETTING_UrlCmdParams("urlCmdParams", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        const Setting SETTING_UrlAutoDownload("urlAutoDownload", Variant::Bool, false, Setting::LocalAllUsers, Setting::ReadOnly);
        const Setting SETTING_UrlRestart("urlRestart", Variant::Bool, false, Setting::LocalAllUsers, Setting::ReadOnly);
        const Setting SETTING_UrlItemSubType("urlItemSubType", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);

        const Setting SETTING_LocalhostSSOtoken("localhostSSOtoken", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);

        Setting SETTING_LSX_PORT("LSXPort", Variant::Int, 3216, Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_ENABLE_PROGRESSIVE_INSTALLER_SIMULATOR("EnableProgressiveInstallerSimulator", Variant::Bool, false, Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_PROGRESSIVE_INSTALLER_SIMULATOR_CONFIG("ProgressiveInstallerSimulatorConfig", Variant::String, "PISimulator.xml", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_DisconnectSDKWhenNoEntitlement("DisconnectSDKWhenNoEntitlement", Variant::Bool, false, Setting::LocalAllUsers, Setting::ReadOnly);

        Setting SETTING_AchievementServiceURL("AchievementServiceURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_AchievementsEnabled("AchievementsEnabled", Variant::Bool, false, Setting::LocalAllUsers, Setting::ReadOnly);

        Setting SETTING_TimedTrialServiceURL("TimedTrialServiceURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);

        // Subscription Settings.
        Setting SETTING_SubscriptionServiceURL("SubscriptionServiceURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_SubscriptionVaultServiceURL("SubscriptionVaultServiceURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_SubscriptionTrialEligibilityServiceURL("SubscriptionTrialEligibilityServiceURL", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_SubscriptionEnabled("SubscriptionEnabled", Variant::Bool, false, Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_SubscriptionEntitlementOfferId("SubscriptionEntitlementOfferId", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_SubscriptionEntitlementRetiringTime("SubscriptionEntitlementRetiringTime", Variant::Int, -1, Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_SubscriptionEntitlementMasterIdUpdateAvaliable("SubscriptionEntitlementMasterIdUpdateAvaliable", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_SubscriptionExpirationReason("SubscriptionExpirationReason", Variant::Int, -1, Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_SubscriptionExpirationDate("SubscriptionExpirationDate", Variant::String, "", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_SubscriptionMaxOfflinePlay("SubscriptionMaxOfflinePlay", Variant::Int, -1, Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_SubscriptionLastModifiedKey("LeavingEarly", Variant::String, "", Setting::LocalPerUser, Setting::ReadWrite, Setting::Encrypted);

        // value to obtain URLS
        Setting SETTING_OriginConfigServiceURL("OriginConfigServiceURL", Variant::String, "https://loginregistration.dm.origin.com", Setting::LocalAllUsers, Setting::ReadOnly);

        const Setting SETTING_BROADCAST_RESOLUTION("BroadcastResolution", Variant::Int, 2, Setting::LocalPerUser, Setting::ReadWrite);
        // note that this is an index into the structure, not the value of the framerate. We may want to change this
        const Setting SETTING_BROADCAST_FRAMERATE("BroadcastFramerate", Variant::Int, 4, Setting::LocalPerUser, Setting::ReadWrite);
        const Setting SETTING_BROADCAST_QUALITY("BroadcastQuality", Variant::Int, 0, Setting::LocalPerUser, Setting::ReadWrite);
        const Setting SETTING_BROADCAST_BITRATE("BroadcastBitrate", Variant::Int, 1500, Setting::LocalPerUser, Setting::ReadWrite);
        const Setting SETTING_BROADCAST_AUTOSTART("BroadcastAutoStart", Variant::Bool, false, Setting::LocalPerUser, Setting::ReadWrite);
        const Setting SETTING_BROADCAST_SHOW_VIEWERS_NUM("BroadcastShowViewersNum", Variant::Bool, true, Setting::LocalPerUser, Setting::ReadWrite);
        const Setting SETTING_BROADCAST_USE_OPT_ENCODER("BroadcastUseOptimizedEncoder", Variant::Bool, true, Setting::LocalPerUser, Setting::ReadWrite);

        const Setting SETTING_BROADCAST_MIC_VOL("BroadcastMicVolume", Variant::Int, 50, Setting::LocalPerUser, Setting::ReadWrite);
        const Setting SETTING_BROADCAST_MIC_MUTE("BroadcastMicMute", Variant::Bool, false, Setting::LocalPerUser, Setting::ReadWrite);
        const Setting SETTING_BROADCAST_GAME_VOL("GameMicVolume", Variant::Int, 50, Setting::LocalPerUser, Setting::ReadWrite);
        const Setting SETTING_BROADCAST_GAME_MUTE("GameMicMute", Variant::Bool, false, Setting::LocalPerUser, Setting::ReadWrite);
        const Setting SETTING_BROADCAST_GAMENAME("BroadcastGameName", Variant::String, "", Setting::LocalPerUser, Setting::ReadWrite);

        // readonly because we cannot trust the previous connection!!!
        const Setting SETTING_BROADCAST_SAVE_VIDEO("BroadcastSaveVideo", Variant::Bool, true, Setting::LocalPerUser, Setting::ReadOnly);
        const Setting SETTING_BROADCAST_SAVE_VIDEO_URL("BroadcastSaveVideoFix", Variant::String, "", Setting::LocalPerUser, Setting::ReadOnly);
        const Setting SETTING_BROADCAST_CHANNEL_URL("BroadcastChannel", Variant::String, "", Setting::LocalPerUser, Setting::ReadOnly);

        // this is not a user controlled setting
        const Setting SETTING_BROADCAST_FIRST_TIME_CONNECT("BroadcastFirstTimeConnect", Variant::Bool, false, Setting::LocalPerUser, Setting::ReadWrite);

        // Nucleus 2.0 Settings used when accessing the Identitiy Portal API
        const Setting SETTING_ClientId("ClientId", Variant::String, "ORIGIN_PC", Setting::LocalAllUsers, Setting::ReadOnly);
        const QString ClientSecretProduction = "UIY8dwqhi786T78ya8Kna78akjcp0s";
        const QString ClientSecretIntegration = "ORIGIN_PC_SECRET";
        Setting SETTING_ClientSecret("ClientSecret", Variant::String, ClientSecretProduction, Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_ConnectPortalBaseUrl("ConnectPortalBaseUrl", Variant::String, "https://accounts.int.ea.com", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_SignInPortalBaseUrl("SignInPortalBaseURL", Variant::String, "https://signin.int.ea.com", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_MaxOfflineWebApplicationCacheSize("MaxOfflineWebApplicationCacheSize", Variant::ULongLong, 15 * 1024 * 1024, Setting::LocalAllUsers, Setting::ReadOnly);

        Setting SETTING_IdentityPortalBaseUrl("IdentityPortalBaseUrl", Variant::String, "https://gateway.int.ea.com/proxy", Setting::LocalAllUsers, Setting::ReadOnly);
        const Setting SETTING_LoginSuccessRedirectUrl("LoginSuccessRedirectUrl", Variant::String, "qrc:///html/login_successful.html", Setting::LocalAllUsers, Setting::ReadOnly);
        const Setting SETTING_LogoutSuccessRedirectUrl("LogoutSuccessRedirectUrl", Variant::String, "qrc:///html/logout.html", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_OnlineLoginServiceUrl("OnlineLoginServiceUrl", Variant::String, "p/pc/login", Setting::LocalAllUsers, Setting::ReadOnly);
        Setting SETTING_OfflineLoginServiceUrl("OfflineLoginServiceUrl", Variant::String, "p/pc/offline", Setting::LocalAllUsers, Setting::ReadOnly);

        // Used for Mac Alpha Trial to determine if origin if running for first time
        Setting SETTING_FIRST_RUN("FirstRun", Variant::Bool, true);
        const DEFINE_EACORE_SETTING_BOOL_TRUE(AlphaLogin);

        // Used to overide onTheHousePromoURL and point to a different html file.
        const DEFINE_EACORE_SETTING(OnTheHouseOverridePromoURL);

        // Used to check how long it's been since we were installed. Net Promoter is suppressed
        // for the first day, for example.
        const Setting SETTING_FIRSTTIMESTARTED("FirstTimeStarted", Variant::String, "");

        // Server side settings
        // These need to be registered with registerServerUserSetting() in initServerUserSettings()
        // ServerUserSettings.cpp will also need to be taught to serialize/deserialize it
        // do not use "Setting::Encrypted" for server side tokens, enc./dec. is machine dependant!!!
        const Setting SETTING_EnableIgoForAllGames(EnableIgoForAllGamesKey, Variant::Bool, false, Setting::GlobalPerUser, Setting::ReadWrite);
        const Setting SETTING_EnableCloudSaves(EnableCloudSavesKey, Variant::Bool, true, Setting::GlobalPerUser, Setting::ReadWrite);
        const Setting SETTING_ServerSideEnableTelemetry(EnableTelemetryKey, Variant::Bool, true, Setting::GlobalPerUser, Setting::ReadWrite);
        const Setting SETTING_FavoriteProductIds(FavoriteProductIdsKey, Variant::StringList, QStringList(), Setting::GlobalPerUser, Setting::ReadWrite);
        const Setting SETTING_HiddenProductIds(HiddenProductIdsKey, Variant::StringList, QStringList(), Setting::GlobalPerUser, Setting::ReadWrite);
        const Setting SETTING_BROADCAST_TOKEN(BroadcastTokenKey, Variant::String, "", Setting::GlobalPerUser, Setting::ReadWrite);
        const Setting SETTING_LastDismissedOtHPromoOffer(OthOfferKey, Variant::String, "", Setting::GlobalPerUser, Setting::ReadWrite);

        const Setting SETTING_DownloadQueue("DownloadQueue", Variant::String, "", Setting::LocalPerUser, Setting::ReadWrite);

        // VOIP settings
#if ENABLE_VOICE_CHAT
        const Setting SETTING_AutoGain("AutoGain", Variant::Bool, false, Setting::LocalPerUser);
        const Setting SETTING_EnablePushToTalk("EnablePushToTalk", Variant::Bool, false, Setting::LocalPerUser);
        const Setting SETTING_PushToTalkKey("PushToTalkKey", Variant::ULongLong, SettingsManager::pushToTalkKeyDefault(), Setting::LocalPerUser);
        const Setting SETTING_PushToTalkMouse("PushToTalkMouse", Variant::String, "mouse1", Setting::LocalPerUser);
        const Setting SETTING_PushToTalkKeyString("PushToTalkKeyString", Variant::String, InGameHotKey::fromULongLong(SettingsManager::pushToTalkKeyDefault()).asNativeString(), Setting::LocalPerUser);
        const Setting SETTING_MicrophoneGain("MicrophoneGain", Variant::Int, 60, Setting::LocalPerUser);
        const Setting SETTING_SpeakerGain("SpeakerGain", Variant::Int, 50, Setting::LocalPerUser);
        const Setting SETTING_EchoQueuedDelay("EchoQueuedDelay", Variant::Int, 5, Setting::LocalPerUser);
        const Setting SETTING_EchoTailMultiplier("EchoTailMultiplier", Variant::Int, 5, Setting::LocalPerUser);

        const Setting SETTING_VoiceInputDevice("VoiceInputDevice", Variant::String, "", Setting::LocalPerUser);
        const Setting SETTING_VoiceOutputDevice("VoiceOutputDevice", Variant::String, "", Setting::LocalPerUser);
        const Setting SETTING_VoiceActivationThreshold("VoiceActivationThreshold", Variant::Int, 5, Setting::LocalPerUser);
        const Setting SETTING_EnableVoiceChatIndicator("EnableVoiceChatIndicator", Variant::Int, 3, Setting::LocalPerUser);
        const Setting SETTING_DeclineIncomingVoiceChatRequests("DeclineIncomingVoiceChatRequests", Variant::Bool, false, Setting::LocalPerUser);
        const Setting SETTING_NoWarnAboutVoiceChatConflict("NoWarnAboutVoiceChatConflict", Variant::Bool, false, Setting::LocalPerUser);
#endif //ENABLE_VOICE_CHAT

        /// legacy EACore settings

        /*
        const QString g_sCorePrefGroupId = "EACore";
        const QString g_sCoreSessionPrefGroupId = "EACoreSession";
        const QString g_sCoreUserPrefGroupId = "EACoreUser";
        const QString g_sConnectionPrefGroupId = "Connection";
        const QString g_sServicesPrefGroupId = "Services";
        */
        /*
        */

        // URLS
#ifdef _DEBUG
        const DEFINE_EACORE_SETTING_BOOL_TRUE(WebDebugEnabled);
        const DEFINE_EACORE_SETTING_BOOL_TRUE(ShowDebugMenu);
#else
        const DEFINE_EACORE_SETTING_BOOL_FALSE(WebDebugEnabled);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(ShowDebugMenu);
#endif

        const DEFINE_EACORE_SETTING_BOOL_FALSE(DebugLanguage);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(OriginDeveloperToolEnabled);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(DownloadDebugEnabled);
        const DEFINE_EACORE_SETTING(OverrideCustomerSupportPageUrl);    //override default CE page to load

        // Certificates
        const DEFINE_EACORE_SETTING_BOOL_FALSE(IgnoreSSLCertError);
        // Feature
        const DEFINE_EACORE_SETTING_BOOL_FALSE(BlockingBehavior);
        const DEFINE_EACORE_SETTING_BOOL_TRUE(SingleLogin);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(DisableNetPromoter);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(DisableMotd);
        DEFINE_EACORE_SETTING(CdnOverride); // not const because this can be changed at runtime via Debug menu
        // Telemetry
        const DEFINE_EACORE_SETTING(TelemetryEnabled);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(TelemetryXML);
        const DEFINE_EACORE_SETTING(TelemetryServer); //159.153.235.32 (default - PRODUCTION)
        const DEFINE_EACORE_SETTING(TelemetryPort); //9988

        //Connection
        const Setting SETTING_OverridePromos("OverridePromos", Variant::Int, PromosDefault, Setting::LocalAllUsers, Setting::ReadOnly);
        const Setting SETTING_FlushWebCache("FlushWebCache", Variant::Bool, false, Setting::LocalAllUsers, Setting::ReadOnly);

        // QA
        const DEFINE_EACORE_SETTING_BOOL_FALSE(OriginConfigDebug);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(CloudSavesDebug);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(ContentDebug);
        const DEFINE_EACORE_SETTING_INT(qaForcedUpdateCheckInterval);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(ForceLockboxURL);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(EnableDownloadOverrideTelemetry);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(EnableIGODebugMenu);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(EnableIGOStressTest);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(IGOReducedBrowser);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(IGOMiniAppBrowser);
        const DEFINE_EACORE_SETTING_INT(MinNetPromoterTestRange);
        const DEFINE_EACORE_SETTING_INT(MaxNetPromoterTestRange);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(ShowNetPromoter);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(ShowGameNetPromoter);
        const DEFINE_EACORE_SETTING(CredentialsUserName);
        const DEFINE_EACORE_SETTING(CredentialsPassword);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(PurgeAccessLicenses);
        const DEFINE_EACORE_SETTING(DisableTwitchBlacklist);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(DisableEmbargoMode);
        const DEFINE_EACORE_SETTING(OverrideOSVersion);
        const DEFINE_EACORE_SETTING_INT_VALUE(OverrideCatalogDefinitionLookupTelemetryInterval, -1);
        const DEFINE_EACORE_SETTING_INT_VALUE(OverrideCatalogDefinitionLookupRetryInterval, -1);
        const DEFINE_EACORE_SETTING_INT_VALUE(OverrideCatalogDefinitionRefreshInterval, -1);
        const DEFINE_EACORE_SETTING_INT_VALUE(OverrideCatalogDefinitionMaxAgeDays, -1);
        const DEFINE_EACORE_SETTING(DebugMenuEntitlementID);
        const DEFINE_EACORE_SETTING(DebugMenuEulaPathOverride);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(ForceContentWatermarking);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(EnableGameLaunchCancel);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(DisableEntitlementFilter);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(ReduceDirtyBitsCappedTimeout);
        const DEFINE_EACORE_SETTING_INT_VALUE(OverrideLoadTimeoutError, -1);
        const DEFINE_EACORE_SETTING_BOOL_TRUE(EnableFeatureCallouts);
        const DEFINE_EACORE_SETTING(VoipAddressBefore);
        const DEFINE_EACORE_SETTING(VoipAddressAfter);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(VoipForceSurvey);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(VoipForceUnexpectedEnd);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(ForceChatGroupDeleteFail);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(SonarOneMinuteInactivityTimeout);
        const DEFINE_EACORE_SETTING(OverrideCatalogIGODefaultURL);

        // CrashTesting
        const DEFINE_EACORE_SETTING_BOOL_FALSE(CrashOnStartup);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(CrashOnExit);

        // DownloaderTuning
        const DEFINE_EACORE_SETTING_INT(MaxWorkersForAllSessions);
        const DEFINE_EACORE_SETTING_INT(SessionBaseTickRate);
        const DEFINE_EACORE_SETTING_INT(RateDecreasePerActiveSession);
        const DEFINE_EACORE_SETTING_INT(MinRequestSize);
        const DEFINE_EACORE_SETTING_INT(MaxRequestSize);
        const DEFINE_EACORE_SETTING_INT(MaxWorkersPerFileSession);
        const DEFINE_EACORE_SETTING_INT(MaxWorkersPerHttpSession);
        const DEFINE_EACORE_SETTING_INT(IdealFileRequestSize);
        const DEFINE_EACORE_SETTING_INT(IdealHttpRequestSize);
        const DEFINE_EACORE_SETTING_INT(MaxUnpackServiceThreads);
        const DEFINE_EACORE_SETTING_INT(MaxUnpackCompressedChunkSize);
        const DEFINE_EACORE_SETTING_INT(MaxUnpackUncompressedChunkSize);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(EnableITOCRCErrors);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(EnableHTTPS);

        // Progressive Installation
        const DEFINE_EACORE_SETTING_BOOL_TRUE(EnableProgressiveInstall);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(EnableSteppedDownload);

        // EACore
        const DEFINE_EACORE_SETTING_BOOL_FALSE(HeartbeatLog);
        const DEFINE_EACORE_SETTING(DL_History); //gnDownloadHistoryLengthKey
        const DEFINE_EACORE_SETTING(AuthenticationApiURL); //gsAuthenticationAPIKey
        const DEFINE_EACORE_SETTING(autodecrypt);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(auto_patch); //g_sAutoPatchPrefId
        const DEFINE_EACORE_SETTING(AvatarApiURL); //gsAvatarAPIKey
        const DEFINE_EACORE_SETTING(ChatApiURL); //gsChatAPIKey
        const DEFINE_EACORE_SETTING(country);
        const DEFINE_EACORE_SETTING(current_path);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(EnableIGOForAll); //gsEnableIGOForAllKey
        const DEFINE_EACORE_SETTING_BOOL_FALSE(ForceFullIGOLogging);
        const DEFINE_EACORE_SETTING(ForceMinIGOResolution);
        const DEFINE_EACORE_SETTING(FriendApiURL); //gsFriendAPIKey
        const DEFINE_EACORE_SETTING(full_exe_path);
        const DEFINE_EACORE_SETTING(GenerateStagedURL); //gsGenerateStagedURL
        const DEFINE_EACORE_SETTING(LogId); //g_sInstanceNamePrefId
        const DEFINE_EACORE_SETTING(LoginGCSApiURL); //gsLoginGCSAPIKey
        const DEFINE_EACORE_SETTING(Max_DL_Chunksize); //gnDownloadMaxChunkSizeKey
        const DEFINE_EACORE_SETTING(Min_DL_Chunksize); //gnDownloadMinChunkSizeKey
        const DEFINE_EACORE_SETTING(Max_DL_Threads); //gnDownloadMaxThreadsKey
        const DEFINE_EACORE_SETTING(Min_DL_Threads); //gnDownloadMinThreadsKey
        const DEFINE_EACORE_SETTING(NewUserExperienceURL); //gsNewUserExpURLKey
        const DEFINE_EACORE_SETTING(OriginOfflineMode); //g_sOriginOfflineMode

        // EACore Overrides
        const DEFINE_EACORE_SETTING(OverrideDownloadPath); //::gsOverrideDownloadKeyPrefix + GetExtId()
        const DEFINE_EACORE_SETTING(OverrideDownloadSyncPackagePath); //::gsOverrideDownloadSyncPackageKeyPrefix + GetExtId()
        const DEFINE_EACORE_SETTING(OverrideCommerceProfile); //::gsOverrideCommerceProfilePrefix + GetExtId()
        const DEFINE_EACORE_SETTING(OverrideMonitoredInstall); //::gsOverrideMonitoredInstall + GetExtId()
        const DEFINE_EACORE_SETTING(OverrideMonitoredPlay); //::gsOverrideMonitoredPlay + GetExtId()
        const DEFINE_EACORE_SETTING(OverrideExternalInstallDialog); //::gsOverrideExternalInstallDialog + GetExtId()
        const DEFINE_EACORE_SETTING(OverrideExternalPlayDialog); //::gsOverrideExternalPlayDialog + GetExtId()
        const DEFINE_EACORE_SETTING(OverridePartnerPlatform); //::gsOverridePartnerPlatform + GetExtId()
        const DEFINE_EACORE_SETTING(OverridePreloadPath); //::gsOverridePreloadKeyPrefix
        const DEFINE_EACORE_SETTING(OverrideExecuteRegistryKey);
        const DEFINE_EACORE_SETTING(OverrideExecuteFilename);
        const DEFINE_EACORE_SETTING(OverrideInstallRegistryKey);
        const DEFINE_EACORE_SETTING(OverrideInstallFilename);
        const DEFINE_EACORE_SETTING(OverrideGameLauncherURL);
        const DEFINE_EACORE_SETTING(OverrideGameLauncherURLIntegration);
        const DEFINE_EACORE_SETTING(OverrideGameLauncherURLIdentityClientId);
        const DEFINE_EACORE_SETTING(OverrideGameLauncherSoftwareId);
        const DEFINE_EACORE_SETTING(OverrideVersion);
        const DEFINE_EACORE_SETTING(OverrideBuildReleaseVersion);
        const DEFINE_EACORE_SETTING(OverrideBuildIdentifier);
        const DEFINE_EACORE_SETTING(OverrideUseLegacyCatalog); 
        const DEFINE_EACORE_SETTING(OverrideEnableDLCUninstall);
        const DEFINE_EACORE_SETTING(OverrideSalePrice);
        const DEFINE_EACORE_SETTING(OverrideBundleContents);
        const DEFINE_EACORE_SETTING(OverrideThumbnailUrl);
        const DEFINE_EACORE_SETTING(OverrideReleaseDate);
        const DEFINE_EACORE_SETTING(OverrideIsForceKilledAtOwnershipExpiry);
        const DEFINE_EACORE_SETTING(OverrideAchievementSet);
        const DEFINE_EACORE_SETTING(GeolocationIPAddress);
        const DEFINE_EACORE_SETTING(OriginGeolocationTest);
        const DEFINE_EACORE_SETTING(KGLTimer);
        const DEFINE_EACORE_SETTING(OverrideSNOFolder);
        const DEFINE_EACORE_SETTING(OverrideSNOUpdateCheck);
        const DEFINE_EACORE_SETTING(OverrideSNOScheduledTime);
        const DEFINE_EACORE_SETTING(OverrideSNOConfirmBuild);

        // Debug only settings
#ifdef _DEBUG
        const DEFINE_EACORE_SETTING(IsolateProductIds);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(DebugEscalationService);
#endif

        // Nucleus 2.0 Identity Portal
        const QString OverrideConnectPortalBaseUrl = "OverrideConnectPortalBaseUrl";
        const QString OverrideIdentityPortalBaseUrl = "OverrideIdentityPortalBaseUrl";
        const QString OverrideLoginSuccessRedirectUrl = "OverrideLoginSuccessRedirectUrl";
        const QString OverrideSignInPortalBaseUrl = "OverrideSignInPortalBaseUrl";
        const DEFINE_EACORE_SETTING(SETTING_ShowDebugMenu);
        const QString OverrideClientId = "OverrideClientId";
        const QString OverrideClientSecret = "OverrideClientSecret";
        const QString OverrideMaxOfflineWebApplicationCacheSize = "OverrideMaxOfflineWebApplicationCacheSize";

        // Free Trials Overrides
        const DEFINE_EACORE_SETTING(OverrideTerminationDate);
        const DEFINE_EACORE_SETTING(OverrideUseEndDate);
        const DEFINE_EACORE_SETTING(OverrideItemSubType);
        const DEFINE_EACORE_SETTING(OverrideCloudSaveContentIDFallback);
        const DEFINE_EACORE_SETTING(OverrideTrialDuration);

        const DEFINE_EACORE_SETTING(RandomMachineHashTestMode); //gsRandomMachineHashTestModeKey
        const DEFINE_EACORE_SETTING(RetrieveJITURL); //gsRetrieveJITURL
        const DEFINE_EACORE_SETTING(ServerIdleAutoShutdown_ms);
        const DEFINE_EACORE_SETTING(TokenExtensionApiURL); //gsTokenExtensionAPIKey
        const DEFINE_EACORE_SETTING(UseStandAloneServer);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(UseTestServer); //gsUseBugSentryTestServer
        const DEFINE_EACORE_SETTING_BOOL_FALSE(FullMemoryDump);
        const DEFINE_EACORE_SETTING(XMPPApiURL); //gsXMPPAPIKey

        // logging
        const DEFINE_EACORE_SETTING(LogConsoleDevice);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(LogDebug);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(LogCatalog);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(LogDirtyBits);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(LogDownload);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(LogSonar);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(LogSonarDirectSoundCapture);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(LogSonarDirectSoundPlayback);
        const DEFINE_EACORE_SETTING_INT_VALUE(LogSonarJitterMetricsLevel, 0);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(LogSonarJitterTracing);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(LogSonarPayloadTracing);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(LogSonarSpeexCapture);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(LogSonarSpeexPlayback);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(LogSonarTiming);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(LogSonarConnectionStats);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(LogVoip);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(LogXep0115);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(LogXmpp);
		const DEFINE_EACORE_SETTING_BOOL_FALSE(LogGroupServiceREST);
		const DEFINE_EACORE_SETTING_BOOL_FALSE(LogVoiceServiceREST);

        // EACoreSession
        const Setting SETTING_GeoCountry("LastGeoCountry", Variant::String, "", Setting::LocalAllUsers, Setting::ReadWrite, Setting::Encrypted);
        const Setting SETTING_CommerceCountry("LastCommerceCountry", Variant::String, "", Setting::LocalAllUsers, Setting::ReadWrite, Setting::Encrypted);
        const Setting SETTING_CommerceCurrency("LastCommerceCurrency", Variant::String, "", Setting::LocalAllUsers, Setting::ReadWrite, Setting::Encrypted);
        const DEFINE_EACORE_SETTING(LastGeoCheck);
        const DEFINE_EACORE_SETTING(DIPAutoInstallTried); //::  gsDIPAutoInstallTriedKey + pItem->GetExtId()
        const DEFINE_EACORE_SETTING(SharedServerID);


        // EACoreUser
        const DEFINE_EACORE_SETTING(Connection);
        const DEFINE_EACORE_SETTING(OverrideEntitlementResponse);

		// Voice
		const DEFINE_EACORE_SETTING(VoiceEnable);

        // Sonar overrides
        const DEFINE_EACORE_SETTING_INT_VALUE(SonarClientPingInterval, 20000);
        const DEFINE_EACORE_SETTING_INT_VALUE(SonarClientTickInterval, 100);
        const DEFINE_EACORE_SETTING_INT_VALUE(SonarJitterBufferSize, 7);
        const DEFINE_EACORE_SETTING_INT_VALUE(SonarRegisterTimeout, 10000);
        const DEFINE_EACORE_SETTING_INT_VALUE(SonarRegisterMaxRetries, 0);
        const DEFINE_EACORE_SETTING_INT_VALUE(SonarNetworkPingInterval, 100);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(SonarEnableRemoteEcho);

        const DEFINE_EACORE_SETTING_INT_VALUE(SonarSpeexAgcEnable, 0);
        const DEFINE_EACORE_SETTING_FLOAT_VALUE(SonarSpeexAgcLevel, 3500.0);
        const DEFINE_EACORE_SETTING_INT_VALUE(SonarSpeexComplexity, 4);
        const DEFINE_EACORE_SETTING_INT_VALUE(SonarSpeexQuality, 6);
        const DEFINE_EACORE_SETTING_INT_VALUE(SonarSpeexVbrEnable, 1);
        const DEFINE_EACORE_SETTING_FLOAT_VALUE(SonarSpeexVbrQuality, 6.0f);

        const DEFINE_EACORE_SETTING_INT_VALUE(SonarTestingAudioError, 0);
        const DEFINE_EACORE_SETTING_INT_VALUE(SonarTestingRegisterPacketLossPercentage, 0); // 0-100
        const DEFINE_EACORE_SETTING_INT_VALUE(SonarTestingEmptyConversationTimeoutDuration, 15 * 60 * 1000); // 15 minutes (in milliseconds) - https://developer.origin.com/support/browse/OFM-10461
		const DEFINE_EACORE_SETTING(SonarTestingUserLocation);
        const DEFINE_EACORE_SETTING(SonarTestingVoiceServerRegisterResponse);
        const DEFINE_EACORE_SETTING_BOOL_FALSE(EchoCancellation);

        const DEFINE_EACORE_SETTING_INT_VALUE(SonarNetworkGoodLoss, 1);
        const DEFINE_EACORE_SETTING_INT_VALUE(SonarNetworkGoodJitter, 2);
        const DEFINE_EACORE_SETTING_INT_VALUE(SonarNetworkGoodPing, 90);
        const DEFINE_EACORE_SETTING_INT_VALUE(SonarNetworkOkLoss, 2);
        const DEFINE_EACORE_SETTING_INT_VALUE(SonarNetworkOkJitter, 3);
        const DEFINE_EACORE_SETTING_INT_VALUE(SonarNetworkOkPing, 180);
        const DEFINE_EACORE_SETTING_INT_VALUE(SonarNetworkPoorLoss, 3);
        const DEFINE_EACORE_SETTING_INT_VALUE(SonarNetworkPoorJitter, 4);
        const DEFINE_EACORE_SETTING_INT_VALUE(SonarNetworkPoorPing, 300);

        const DEFINE_EACORE_SETTING_INT_VALUE(SonarNetworkQualityPingStartupDuration, 3000);
        const DEFINE_EACORE_SETTING_INT_VALUE(SonarNetworkQualityPingShortInterval, 200);
        const DEFINE_EACORE_SETTING_INT_VALUE(SonarNetworkQualityPingLongInterval, 2000);

        // OriginSDKJS session key
        const DEFINE_EACORE_SETTING(OriginSessionKey);

    }
}

#include <QtCore>
#include <QDomDocument>

#if defined(ORIGIN_PC)
#include <ShlObj.h>
#endif

namespace Origin
{
    namespace Engine
    {
        class SettingsSyncManager;
    }
}

namespace
{
    // prototypes
#if defined(ORIGIN_PC)
    QString GetDirectory(int csidl_id);
#endif
    QByteArray encryptionKey();

    /// \fn readXmlFile(QIODevice &device, QSettings::SettingsMap &map)
    /// \brief Reads QSettings data from an XML-formatted file.
    bool readXmlFile(QIODevice &device, QSettings::SettingsMap &map);

    /// \fn writeXmlFile(QIODevice &device, const QSettings::SettingsMap &map)
    /// \brief Writes QSettings data to an XML-formatted file.
    bool writeXmlFile(QIODevice &device, const QSettings::SettingsMap &map);

    CryptoService::Cipher sCipher = CryptoService::BLOWFISH;
    CryptoService::CipherMode sCipherMode = CryptoService::CIPHER_BLOCK_CHAINING;
}

namespace Origin
{
    namespace Services
    {

        // statics
        namespace
        {
            QString const ORGANIZATION("Origin");
            QString const SETTINGS_ELEMENT_NAME("Settings");
            QString const SETTING_ELEMENT_NAME("Setting");
            QString const LOCAL_SETTINGS_PREFIX("local");
            QString const CLOUD_SETTINGS_PREFIX("remote");
            QString const SETTINGS_FILE_EXTENSION("xml");
            QMutex sGuard(QMutex::Recursive); // marked as recursive to allow settings to be set during instance creation (import)
            QString const LoginTokenKeySecret("D4A848D57B26E005554D1062054CA094A1480BD514DC0026"); // random
        }
        QScopedPointer<SettingsManager> SettingsManager::s_instance;

        QReadWriteLock *SettingsManager::mInternalSettingsMapLock =  NULL;
        SettingsManager::InternalSettingsMap *SettingsManager::mInternalSettingsMap = NULL;
        QReadWriteLock *SettingsManager::mInternalSettingsOverrideMapLock =  NULL;
        SettingsManager::InternalSettingsMap *SettingsManager::mInternalSettingsOverrideMap = NULL;
        QReadWriteLock *SettingsManager::mLocalMachineSettingsLock =  NULL;
        QReadWriteLock *SettingsManager::mLocalUserSettingsLock =  NULL;
        QReadWriteLock *SettingsManager::mLocalPerAccountSettingsLock =  NULL;
        QReadWriteLock *SettingsManager::mOverrideSettingsLock = NULL;
        QReadWriteLock *SettingsManager::mServerUserSettingsLock = NULL;
        QSet<QString> SettingsManager::mProductOverrideSettingsPrefixes;

        void SettingsManager::init()
        {
            if (isInitialized())
            {
                ORIGIN_ASSERT_MESSAGE(false, "SettingsManager already initialized.");
                return;
            }

            SettingsManager* singleton = new SettingsManager();
            ORIGIN_ASSERT(singleton);
            s_instance.reset(singleton);

            // load machine and OS account specific settings
            s_instance->loadLocalMachineSettings();
            s_instance->loadLocalPerAccountSettings();
            // load the override settings
            s_instance->loadOverrideSettings();

            // import old settings, but keep the files until we have per user settings ready for import
            s_instance->importOldSettings(true, false);

            s_instance->initServerUserSettings();

            QString firstStartStr = readSetting(SETTING_FIRSTTIMESTARTED);
            if (firstStartStr.isEmpty())
            {
                // Store in ISO 8601 format.
                firstStartStr = QDateTime::currentDateTime().toString("yyyy-MM-ddThh:mm:ssZ");
                writeSetting(SETTING_FIRSTTIMESTARTED, firstStartStr);
            }

            // Re-init server user settings whenever we go online
            ORIGIN_VERIFY_CONNECT(Session::SessionService::instance(),
                                  SIGNAL(userOnlineAllChecksCompleted(bool, Origin::Services::Session::SessionRef)),
                                  singleton,
                                  SLOT(initServerUserSettings()));

            // Populate mProductOverrideSettingsPrefixes with the setting names that all product override settings will begin with - for example, "OverrideDownloadPath"
            mProductOverrideSettingsPrefixes << SETTING_OverrideDownloadPath.name()
                << SETTING_OverrideDownloadSyncPackagePath.name()
                << SETTING_OverrideCommerceProfile.name()
                << SETTING_OverrideUseLegacyCatalog.name()
                << SETTING_OverrideEnableDLCUninstall.name()
                << SETTING_OverrideMonitoredInstall.name()
                << SETTING_OverrideMonitoredPlay.name()
                << SETTING_OverrideExternalInstallDialog.name()
                << SETTING_OverrideExternalPlayDialog.name()
                << SETTING_OverridePartnerPlatform.name()
                << SETTING_OverridePreloadPath.name()
                << SETTING_OverrideExecuteRegistryKey.name()
                << SETTING_OverrideExecuteFilename.name()
                << SETTING_OverrideInstallRegistryKey.name()
                << SETTING_OverrideInstallFilename.name()
                << SETTING_OverrideGameLauncherURL.name()
                << SETTING_OverrideGameLauncherURLIntegration.name()
                << SETTING_OverrideGameLauncherURLIdentityClientId.name()
                << SETTING_OverrideGameLauncherSoftwareId.name()
                << SETTING_OverrideVersion.name()
                << SETTING_OverrideBuildReleaseVersion.name()
                << SETTING_OverrideBuildIdentifier.name()
                << SETTING_OverrideSalePrice.name()
                << SETTING_OverrideBundleContents.name()
                << SETTING_OverrideThumbnailUrl.name()
                << SETTING_OverrideReleaseDate.name()
                << SETTING_DisableTwitchBlacklist.name()
                << SETTING_OverrideAchievementSet.name()
                << SETTING_OverrideSNOFolder.name()
                << SETTING_OverrideSNOUpdateCheck.name()
                << SETTING_OverrideSNOScheduledTime.name()
                << SETTING_OverrideSNOConfirmBuild.name();

            // if install timestamp is more recent than the last time user set RememberMe or TFAId, invalidate all of them as we have re-installed since then. (ORPLAT-1093)
            // we must do this here because RememberMe and TFDAId are local per ACCOUNT but a re-install is local per MACHINE.
            qulonglong install_ts = readSetting(SETTING_INSTALLTIMESTAMP, Session::SessionService::currentSession());
            qulonglong rememberme_tfa_ts = readSetting(SETTING_REMEMBERME_TFA_TIMESTAMP, Session::SessionService::currentSession());

            if (install_ts > rememberme_tfa_ts)    
            {
                // clear out remember me and two-factor auth settings when installing (not updating)
                Origin::Services::writeSetting(Origin::Services::SETTING_REMEMBER_ME_INT, SETTING_INVALID_REMEMBER_ME);
                Origin::Services::writeSetting(Origin::Services::SETTING_REMEMBER_ME_PROD, SETTING_INVALID_REMEMBER_ME);
                Origin::Services::writeSetting(Origin::Services::SETTING_TFAID_INT, SETTING_INVALID_TFAID);
                Origin::Services::writeSetting(Origin::Services::SETTING_TFAID_PROD, SETTING_INVALID_TFAID);
            }
        }

        void SettingsManager::free()
        {
            if (!isInitialized())
            {
                ORIGIN_ASSERT_MESSAGE(false, "SettingsManager is not initialized.");
                return;
            }

            delete s_instance.take();
        }

        void SettingsManager::initServerUserSettings()
        {
            QWriteLocker lock(mServerUserSettingsLock);

            Session::SessionRef session = Session::SessionService::currentSession();

            if (mServerUserSettings)
            {
                if (mServerUserSettings->session() == session)
                {
                    // Already loaded for the correct session
                    return;
                }

                delete mServerUserSettings;
            }

            // Keep track of our settings so we can emit settingChanged() for them when the server side changes
            registerServerUserSetting(SETTING_EnableIgoForAllGames);
            registerServerUserSetting(SETTING_EnableCloudSaves);
            registerServerUserSetting(SETTING_ServerSideEnableTelemetry);
            registerServerUserSetting(SETTING_FavoriteProductIds);
            registerServerUserSetting(SETTING_HiddenProductIds);
            registerServerUserSetting(SETTING_BROADCAST_TOKEN);
            registerServerUserSetting(SETTING_LastDismissedOtHPromoOffer);

            // Create the new server-side settings manager
            mServerUserSettings = new ServerUserSettingsManager(session, this);

            ORIGIN_VERIFY_CONNECT(mServerUserSettings, SIGNAL(loaded()),
                                  this, SIGNAL(serverUserSettingsLoaded()));

            ORIGIN_VERIFY_CONNECT(mServerUserSettings, SIGNAL(valueChanged(QString, QVariant, bool)),
                                  this, SLOT(serverUserValueChanged(QString, QVariant, bool)));

        }

        void SettingsManager::registerServerUserSetting(const Setting &setting)
        {
            mServerUserSettingInstances[setting.name()] = &setting;
        }

        void SettingsManager::serverUserValueChanged(QString key, QVariant value, bool fromServer)
        {
            const Setting *changedSetting = NULL;

            // Did we get a change driven by the server?
            if (fromServer)
            {
                QReadLocker lock(mServerUserSettingsLock);
                changedSetting = mServerUserSettingInstances[key];
            }

            if (changedSetting)
            {
                emit settingChanged(changedSetting->name(), Variant(value));
            }
        }

        bool SettingsManager::areServerUserSettingsLoaded()
        {
            QReadLocker lock(s_instance->mServerUserSettingsLock);
            return s_instance->mServerUserSettings && s_instance->mServerUserSettings->isLoaded();
        }

        void SettingsManager::initInternalSettingsMap()
        {
            if (!mInternalSettingsMapLock)
            {
                mInternalSettingsMapLock = new QReadWriteLock();
                mInternalSettingsMap = new InternalSettingsMap();
            }
        }

        void SettingsManager::freeInternalSettingsMap()
        {
            if (mInternalSettingsMapLock)
            {
                delete mInternalSettingsMapLock;
                mInternalSettingsMapLock = NULL;
                delete mInternalSettingsMap;
                mInternalSettingsMap = NULL;
            }
        }

        void SettingsManager::initInternalSettingsOverrideMap()
        {
            if (!mInternalSettingsOverrideMapLock)
            {
                mInternalSettingsOverrideMapLock = new QReadWriteLock();
                mInternalSettingsOverrideMap = new InternalSettingsOverrideMap();
            }
        }

        void SettingsManager::freeInternalSettingsOverrideMap()
        {
            if (mInternalSettingsOverrideMapLock)
            {
                delete mInternalSettingsOverrideMapLock;
                mInternalSettingsOverrideMapLock = NULL;
                delete mInternalSettingsOverrideMap;
                mInternalSettingsOverrideMap = NULL;
            }
        }

        void SettingsManager::addToInternalSettingsMap(QString settingName, Setting const *setting)
        {
            initInternalSettingsMap();
            QWriteLocker lock(mInternalSettingsMapLock);
            SettingsKey key(settingName);
            ORIGIN_ASSERT(setting->scope() != Setting::LocalOverrideAllUsers);
            ORIGIN_ASSERT_MESSAGE(mInternalSettingsMap->contains(key) == false, "A setting can only exist one time!");
            mInternalSettingsMap->insert(key, setting);
        }

        void SettingsManager::notifyError(QString const& errorMessage)
        {
            emit settingError(errorMessage);
        }

        void SettingsManager::removeFromInternalSettingsMap(QString settingName)
        {
            initInternalSettingsMap();
            SettingsKey key(settingName);
            ORIGIN_ASSERT_MESSAGE(mInternalSettingsMap->contains(key) == true, "Found a stale setting!");

            QWriteLocker lock(mInternalSettingsMapLock);
            InternalSettingsMap::iterator iter = mInternalSettingsMap->find(key);
            if (iter != mInternalSettingsMap->end())
                mInternalSettingsMap->erase(iter);

            // destroy our internal settings list
            if (mInternalSettingsMap->empty())
            {
                lock.unlock();
                freeInternalSettingsMap();
            }
        }

        void SettingsManager::addToInternalSettingsOverrideMap(QString settingName, Setting const *setting)
        {
            initInternalSettingsOverrideMap();
            QWriteLocker lock(mInternalSettingsOverrideMapLock);

            ORIGIN_ASSERT(setting->scope() == Setting::LocalOverrideAllUsers);
            SettingsKey key(settingName);
            ORIGIN_ASSERT_MESSAGE(mInternalSettingsOverrideMap->contains(key) == false, "A setting can only exist one time!");

            mInternalSettingsOverrideMap->insert(key, setting);
        }

        void SettingsManager::removeFromInternalSettingsOverrideMap(QString settingName)
        {
            initInternalSettingsOverrideMap();

            SettingsKey key(settingName);
            ORIGIN_ASSERT_MESSAGE(mInternalSettingsOverrideMap->contains(key) == true, "Found a stale setting!");

            QWriteLocker lock(mInternalSettingsOverrideMapLock);
            InternalSettingsOverrideMap::iterator iter = mInternalSettingsOverrideMap->find(key);
            if (iter != mInternalSettingsOverrideMap->end())
            {
                mInternalSettingsOverrideMap->erase(iter);
            }
            // destroy our internal settings list
            if (mInternalSettingsOverrideMap->empty())
            {
                lock.unlock();
                freeInternalSettingsOverrideMap();
            }
        }

        bool SettingsManager::getSettingByName(QString const& settingName, const Setting **setting)
        {
            if (setting)
            {
                {
                    // override settings have the highest priority
                    initInternalSettingsOverrideMap();

                    QReadLocker lock(mInternalSettingsOverrideMapLock);
                    InternalSettingsOverrideMap::iterator iter = mInternalSettingsOverrideMap->find(SettingsKey(settingName));
                    if (iter != mInternalSettingsOverrideMap->end())
                    {
                        (*setting) = iter.value();
                        return true;
                    }
                }
                {
                    initInternalSettingsMap();

                    QReadLocker lock(mInternalSettingsMapLock);
                    InternalSettingsMap::iterator iter = mInternalSettingsMap->find(SettingsKey(settingName));
                    if (iter != mInternalSettingsMap->end())
                    {
                        (*setting) = iter.value();
                        return true;
                    }
                }
            }

            //comment out this assert for now as it fires when we check for overrides, since the override may or may not exist
            //  ORIGIN_ASSERT_MESSAGE(false, "Setting could not be found!");

            return false;
        }

        void SettingsManager::onLogout()
        {
            sync();
            unloadLocalUserSettings();

            // Get rid of the server-side user settings for the logged out user
            {
                QWriteLocker lock(mServerUserSettingsLock);
                delete mServerUserSettings;
                mServerUserSettings = NULL;
            }
        }

        QVariant SettingsManager::get(Setting const& setting)
        {
            ORIGIN_ASSERT(isLoaded());

            // safely grab the value from the appropriate container
            QVariant v = s_instance->internalGet(setting);

            // return default value, if not set
            if (v.isNull())
                return setting.defaultValue().toQVariant();

            // decrypt the value, if necessary. setting as read (i.e. v) must be a string
            // setting after decryption need not be a string.
            if (setting.encryption() != Setting::Unencrypted && v.type() == QVariant::String)
            {
                QByteArray encryptedData(QByteArray::fromHex(v.toByteArray()));
                QByteArray decryptedData;
                bool ok = CryptoService::decryptSymmetric(decryptedData, encryptedData, encryptionKey(), sCipher, sCipherMode);
                if (!ok)
                {

                    // convert to the appropriate type, if necessary
                    QVariant valueToSet = setting.defaultValue().toQVariant();
                    if ((valueToSet.type() != static_cast<QVariant::Type>(setting.type()))
                            && (!valueToSet.canConvert((QVariant::Type)setting.type()) || !valueToSet.convert((QVariant::Type)setting.type())))
                        return setting.defaultValue().toQVariant();

                    // encrypt the value, if necessary
                    if (setting.encryption() != Setting::Unencrypted)
                    {
                        QByteArray encryptedData;
                        bool wasEncrypted = CryptoService::encryptSymmetric(encryptedData, setting.defaultValue().toQVariant().toByteArray(), encryptionKey(), sCipher, sCipherMode);
                        ORIGIN_ASSERT(wasEncrypted);
                        Q_UNUSED(wasEncrypted);
                        valueToSet = QString(encryptedData.toHex());
                    }

                    s_instance->internalSet(setting, valueToSet);
                    return setting.defaultValue().toQVariant();
                }

                QVariant result(decryptedData);
                if (result.canConvert((QVariant::Type)setting.type()) && result.convert((QVariant::Type)setting.type()))
                {
                    return result;
                }
                else
                {
                    ORIGIN_ASSERT_MESSAGE(false, "failed to decrypt setting");
                    return setting.defaultValue().toQVariant();
                }
            }
            return v;
        }

        QVariant SettingsManager::getAsAscii(Setting const& setting)
        {
            ORIGIN_ASSERT(isLoaded());

            // safely grab the value from the appropriate container
            QVariant v = s_instance->internalGet(setting);

            // return default value, if not set
            if (v.isNull())
                return setting.defaultValue().toQVariant();

            // decrypt the value, if necessary. setting as read (i.e. v) must be a string
            // setting after decryption need not be a string.
            if (setting.encryption() != Setting::Unencrypted && v.type() == QVariant::String)
            {
                QByteArray encryptedData(QByteArray::fromHex(v.toByteArray()));
                QByteArray decryptedData;
                bool ok = CryptoService::decryptSymmetric(decryptedData, encryptedData, encryptionKey(), sCipher, sCipherMode);
                if (!ok)
                {

                    // convert to the appropriate type, if necessary
                    QVariant valueToSet = setting.defaultValue().toQVariant();
                    if ((valueToSet.type() != static_cast<QVariant::Type>(setting.type()))
                            && (!valueToSet.canConvert((QVariant::Type)setting.type()) || !valueToSet.convert((QVariant::Type)setting.type())))
                        return setting.defaultValue().toQVariant();

                    // encrypt the value, if necessary
                    if (setting.encryption() != Setting::Unencrypted)
                    {
                        QByteArray encryptedData;
                        bool wasEncrypted = CryptoService::encryptSymmetric(encryptedData, setting.defaultValue().toQVariant().toByteArray(), encryptionKey(), sCipher, sCipherMode);
                        ORIGIN_ASSERT(wasEncrypted);
                        Q_UNUSED(wasEncrypted);
                        valueToSet = QString(encryptedData.toHex());
                    }

                    s_instance->internalSet(setting, valueToSet);
                    return setting.defaultValue().toQVariant();
                }

                QVariant result(decryptedData);
                if (result.canConvert((QVariant::Type)setting.type()))
                {
                    if (setting.encryption() != Setting::Unencrypted && v.type() == QVariant::String)
                    {
                        //need to do manual conversion here
                        QString str = QString::fromLatin1(result.toByteArray());
                        result.setValue (str);
                        return result;
                    }
                    else if (result.convert((QVariant::Type)setting.type()))
                        return result;
                }
                else
                {
                    ORIGIN_ASSERT_MESSAGE(false, "failed to decrypt setting");
                    return setting.defaultValue().toQVariant();
                }
            }
            return v;
        }

        bool SettingsManager::set(Setting const& setting, QVariant const& value)
        {
            ORIGIN_ASSERT(isLoaded());

            // convert to the appropriate type, if necessary
            QVariant valueToSet(value);
            if ((valueToSet.type() != static_cast<QVariant::Type>(setting.type()))
                    && (!valueToSet.canConvert((QVariant::Type)setting.type()) || !valueToSet.convert((QVariant::Type)setting.type())))
                return false;

            // encrypt the value, if necessary
            if (setting.encryption() != Setting::Unencrypted)
            {
                QByteArray encryptedData;
                bool wasEncrypted = CryptoService::encryptSymmetric(encryptedData, value.toByteArray(), encryptionKey(), sCipher, sCipherMode);
                ORIGIN_ASSERT(wasEncrypted);
                Q_UNUSED(wasEncrypted);
                valueToSet = QString(encryptedData.toHex());
            }

            bool wasChanged = s_instance->internalSet(setting, valueToSet);

            // notify of change to setting
            if (wasChanged)
            {
                emit(s_instance->settingChanged(setting.name(), Variant(value)));
            }

            return true;
        }

        bool SettingsManager::unset(Setting const& setting)
        {
            ORIGIN_ASSERT(isLoaded());

            // TODO: verify implementation

            QSettings* settingsToUse = s_instance->settingsFor(setting);
            if (!settingsToUse)
                return false;

            {
                QMutexLocker guard(&sGuard);
                settingsToUse->remove(setting.name());
            }

            emit(s_instance->settingChanged(setting.name(), Variant()));

            return true;
        }

        bool SettingsManager::isSettingSpecified(Setting const& setting)
        {
            // immutable settings store override values in its default value
            if (setting.mutability() == Setting::ReadOnly)
            {
                return setting.hasDefaultValueUpdated();
            }

            QVariant current = QVariant();
            switch (setting.scope())
            {
            case Setting::LocalOverrideAllUsers:
                {
                    QReadLocker lock(mOverrideSettingsLock);
                    current = mOverrideSettings->value(setting.name());
                    break;
                }

            case Setting::LocalPerAccount:
                {
                    QReadLocker lock(mLocalPerAccountSettingsLock);
                    current = mLocalPerAccountSettings->value(setting.name());
                    break;
                }

            case Setting::LocalAllUsers:
                {
                    QReadLocker lock(mLocalMachineSettingsLock);
                    current = mLocalMachineSettings->value(setting.name());
                    break;
                }

            case Setting::LocalPerUser:
                {
                    if (mLocalUserSettings.isNull())
                    {
                        ORIGIN_ASSERT_MESSAGE(false, "Unable to set settings, local user settings not loaded.");
                        return false;
                    }

                    QReadLocker lock(mLocalUserSettingsLock);
                    current = mLocalUserSettings->value(setting.name());
                    break;
                }

            case Setting::GlobalPerUser:
            default:
                ORIGIN_ASSERT_MESSAGE(false, "Unsupported setting.");
                break;
            }
            return current.isNull() == false;
        }

        // private:

        SettingsManager::SettingsManager()
            : mXmlFormat(QSettings::registerFormat("xml", readXmlFile, writeXmlFile))
            , mServerUserSettings(NULL)
            , mCurrentLocalSession(NULL)
        {
#ifdef ORIGIN_MAC

            // put the system-scope preferences in /Library/Application Support/Origin
            {
                // to-do: use getSystemPreferencesPath() and useelevated privileges to make Origin directory
                QString systemPreferencesPath(PlatformService::userPreferencesPath());
//                systemPreferencesPath += "/Origin";
//                QDir systemPreferencesDir(systemPreferencesPath);
//                if (!systemPreferencesDir.exists())
//                {
//                    bool wasCreated = QDir::root().mkpath(systemPreferencesPath);
//                    ORIGIN_ASSERT(wasCreated);
//                }
                QSettings::setPath(mXmlFormat, QSettings::SystemScope, systemPreferencesPath);
            }

            // put the user-scope preferences in ~/Library/Application Support/Origin
            {
                QString userPreferencesPath(PlatformService::userPreferencesPath());
//                userPreferencesPath += "/Origin";
//                QDir userPreferencesDir(userPreferencesPath);
//                if (!userPreferencesDir.exists())
//                {
//                    bool wasCreated = QDir::root().mkpath(userPreferencesPath);
//                    ORIGIN_ASSERT(wasCreated);
//                }
                QSettings::setPath(mXmlFormat, QSettings::UserScope, userPreferencesPath);
            }

#endif

            mLocalMachineSettingsLock = new QReadWriteLock();
            mLocalUserSettingsLock =  new QReadWriteLock();
            mLocalPerAccountSettingsLock =  new QReadWriteLock();
            mOverrideSettingsLock = new QReadWriteLock();
            mServerUserSettingsLock = new QReadWriteLock();
        }

        // store settings on disc
        void SettingsManager::sync()
        {
            if (!mLocalMachineSettings.isNull())
                mLocalMachineSettings->sync();

            if (!mLocalUserSettings.isNull())
                mLocalUserSettings->sync();

            if (!mLocalPerAccountSettings.isNull())
            {
                mLocalPerAccountSettings->sync();
                QSettings::Status status = mLocalPerAccountSettings->status();

                // Note that if QSettings' status is in an error state, it will
                // always be in an error state (even if we resolve the issue).
                // Therefore, only attempt to handle the error once by
                // controlling execution with a boolean flag.
                static bool needToHandleSyncError = true;

                if( status != QSettings::NoError && needToHandleSyncError)
                {
                    // If we reach this point and this boolean is false,
                    // then our assumptions surrounding these conditions have
                    // changed and the code must be updated!
                    ORIGIN_ASSERT(needToHandleSyncError);

                    // Flip the boolean to false to ensure this code only
                    // executes once
                    needToHandleSyncError = false;

                    reportSyncFailure(*mLocalPerAccountSettings);

                    if (QSettings::AccessError == status)
                    {
                        handleAccessError(*mLocalPerAccountSettings);
                    }
                }
            }
            else
            {
                //Send telemetry if perAccountSettings didn't exist when saving the remember me cookies.
                const QString nullError("isNull");
                const bool ACCESS_UNKNOWN = false;
                const int NO_SYS_ERROR = 0;
                emit localPerAccountSettingFileError(
                    nullError,
                    nullError,
                    ACCESS_UNKNOWN,
                    ACCESS_UNKNOWN,
                    NO_SYS_ERROR,
                    NO_SYS_ERROR,
                    nullError,
                    nullError);
            }
        }

        void SettingsManager::reportSettingFileError(const QString& settingsFilename, const QString& errorMessage,
                const bool readable, const bool writable, const unsigned int readSysError, const unsigned int writeSysError,
                const QString& windowsFileAttributes, const QString& permFlag) const
        {
            // Censor the settings filepath since it could contain the user's
            // windows user account name
            QString censoredFilename(settingsFilename);
            Services::Logger::CensorStringInPlace(censoredFilename);

            GetTelemetryInterface()->Metric_SETTINGS_SYNC_FAILED(
                censoredFilename.toUtf8().constData(),
                errorMessage.toUtf8().constData(),
                readable,
                writable,
                readSysError,
                writeSysError,
                windowsFileAttributes.toUtf8().constData(),
                permFlag.toUtf8().constData());
        }



        SettingsManager::~SettingsManager()
        {
            delete mLocalMachineSettingsLock;
            delete mLocalUserSettingsLock;
            delete mLocalPerAccountSettingsLock;
            delete mOverrideSettingsLock;

            mLocalMachineSettingsLock = NULL;
            mLocalUserSettingsLock = NULL;
            mLocalPerAccountSettingsLock = NULL;
            mOverrideSettingsLock = NULL;

        }


        // Disable copying and assigning.
        //SettingsManager::SettingsManager(SettingsManager const&)
        //SettingsManager& SettingsManager::operator=(SettingsManager const&)

        QSettings* SettingsManager::settingsFor(Setting const& setting)
        {
            switch (setting.scope())
            {
                case Setting::LocalAllUsers:
                    return mLocalMachineSettings.data();

                case Setting::LocalPerAccount:
                    return mLocalPerAccountSettings.data();

                case Setting::LocalPerUser:
                    return mLocalUserSettings.data();

                case Setting::LocalOverrideAllUsers:
                    return mOverrideSettings.data();

                default:
                    ORIGIN_ASSERT_MESSAGE(false, "Unsupported setting.");
                    return NULL;
            }
        }

        QVariant SettingsManager::internalGet(Setting const& setting) const
        {
            // override settings have the highest priority !!!
            if (!mOverrideSettings.isNull())
            {
                QReadLocker lock(mOverrideSettingsLock);
                if (mOverrideSettings->contains(setting.name()))
                {
                    QVariant v = mOverrideSettings->value(setting.name(), setting.defaultValue().toQVariant());
                    if (v.isValid())
                        return v;
                }
            }
            // now try the other "scopes"
            switch (setting.scope())
            {

            case Setting::LocalOverrideAllUsers:
                {
                    QVariant v;
                    if (!mOverrideSettings.isNull())
                        v = mOverrideSettings->value(setting.name(), setting.defaultValue().toQVariant());
                    return v;
                }

            case Setting::LocalAllUsers:
                {
                    QVariant v;
                    if (!mLocalMachineSettings.isNull())
                    {
                        QReadLocker lock(mLocalMachineSettingsLock);
                        v = mLocalMachineSettings->value(setting.name(), setting.defaultValue().toQVariant());
                    }
                    return v;
                }

            case Setting::LocalPerAccount:
                {
                    if (mLocalPerAccountSettings.isNull())
                    {
                        ORIGIN_ASSERT_MESSAGE(false, "Unable to get settings, local account settings not loaded.");
                        return QVariant();
                    }

                    QReadLocker lock(mLocalPerAccountSettingsLock);
                    return mLocalPerAccountSettings->value(setting.name(), setting.defaultValue().toQVariant());
                }

            case Setting::LocalPerUser:
                {
                    if (mLocalUserSettings.isNull())
                    {
                        ORIGIN_ASSERT_MESSAGE(false, "Unable to get settings, local user settings not loaded.");
                        return QVariant();
                    }

                    QReadLocker lock(mLocalUserSettingsLock);
                    return mLocalUserSettings->value(setting.name(), setting.defaultValue().toQVariant());
                }

            case Setting::GlobalPerUser:
                {
                    QReadLocker lock(mServerUserSettingsLock);

                    if (mServerUserSettings)
                    {
                        return mServerUserSettings->get(setting.name(), setting.defaultValue().toQVariant());
                    }

                    return NULL;
                }

            default:
                ORIGIN_ASSERT_MESSAGE(false, "Unsupported setting.");
                return QVariant();
            }
        }

        bool SettingsManager::internalSet(Setting const& setting, QVariant const& value)
        {
            // read only setting won't go to the disc/cloud - so we keep them as default values!!!
            if (setting.mutability() == Setting::ReadOnly)
            {
                const_cast<Setting &>(setting).updateDefaultValue(Variant(value));
                return true;
            }

            switch (setting.scope())
            {
            case Setting::LocalOverrideAllUsers:
                {
                    {
                        QReadLocker lock(mOverrideSettingsLock);
                        QVariant current = mOverrideSettings->value(setting.name());
                        if (current == value)
                            return false;
                    }

                    QWriteLocker lock(mOverrideSettingsLock);
                    mOverrideSettings->setValue(setting.name(), value);
                    return true;
                }

            case Setting::LocalPerAccount:
                {
                    {
                        QReadLocker lock(mLocalPerAccountSettingsLock);
                        QVariant current = mLocalPerAccountSettings->value(setting.name());
                        if (current == value)
                            return false;
                    }

                    QWriteLocker lock(mLocalPerAccountSettingsLock);
                    mLocalPerAccountSettings->setValue(setting.name(), value);
                    return true;
                }

            case Setting::LocalAllUsers:
                {
                    {
                        QReadLocker lock(mLocalMachineSettingsLock);
                        QVariant current = mLocalMachineSettings->value(setting.name());
                        if (current == value)
                            return false;
                    }

                    QWriteLocker lock(mLocalMachineSettingsLock);
                    mLocalMachineSettings->setValue(setting.name(), value);
                    return true;
                }

            case Setting::LocalPerUser:
                {
                    if (mLocalUserSettings.isNull())
                    {
                        ORIGIN_ASSERT_MESSAGE(false, "Unable to set settings, local user settings not loaded.");
                        return false;
                    }

                    {
                        QReadLocker lock(mLocalUserSettingsLock);
                        QVariant current = mLocalUserSettings->value(setting.name());
                        if (current == value)
                            return false;
                    }

                    QWriteLocker lock(mLocalUserSettingsLock);
                    mLocalUserSettings->setValue(setting.name(), value);
                    return true;
                }

            case Setting::GlobalPerUser:
                {
                    QWriteLocker lock(mServerUserSettingsLock);

                    if (mServerUserSettings)
                    {
                        return mServerUserSettings->set(setting.name(), value);
                    }

                    return false;
                }
            default:
                ORIGIN_ASSERT_MESSAGE(false, "Unsupported setting.");
                return NULL;
            }
        }

        void SettingsManager::importOldSettings(bool skipPerUserSettings, bool deleteOldSettings)
        {
            QString rootDir;
#if defined(ORIGIN_PC)
            rootDir = GetDirectory(CSIDL_COMMON_APPDATA);
#elif defined (ORIGIN_MAC)
            rootDir = PlatformService::getStorageLocation(QStandardPaths::DataLocation);
#else
#error "Unsupported platform."
#endif
            if (rootDir.isEmpty())
                return;

            QString rootDir2;
#if defined(ORIGIN_PC)
            rootDir2 = GetDirectory(CSIDL_APPDATA);
#elif defined (ORIGIN_MAC)
            rootDir2 = PlatformService::getStorageLocation(QStandardPaths::DataLocation);
#else
#error "Unsupported platform."
#endif
            if (rootDir2.isEmpty())
                return;

            // settings hierarchy - latest to oldest
            QStringList oldSettings;
            oldSettings.append(rootDir2 + "\\Origin\\Settings.xml");
            oldSettings.append(rootDir  + "\\Origin\\Settings.xml");
            oldSettings.append(rootDir  + "\\Electronic Arts\\EADM\\Settings.xml");

            int oldSettingsIndex = 0;
            bool oldSettingsExist = false;
            while(oldSettingsIndex < oldSettings.count())
            {
                oldSettingsExist = QFile::exists(oldSettings[oldSettingsIndex]);
                if (oldSettingsExist)
                    break;
                else
                    oldSettingsIndex++;
            }

            // we have found old settings
            if (oldSettingsExist)
            {
                // load the dom into memory
                QFile settingsFile(oldSettings[oldSettingsIndex]);
                settingsFile.open(QIODevice::ReadOnly);
                QDomDocument document;
                document.setContent(settingsFile.readAll());

                static char const ROOT_NAME[] = "Settings";
                ORIGIN_UNUSED(ROOT_NAME);

                QDomElement root = document.documentElement();
                ORIGIN_ASSERT(root.tagName() == ROOT_NAME);

                initInternalSettingsMap();
                QReadLocker lock(mInternalSettingsMapLock);

                // add the individual elements to the settings
                // TBD: we really need to strengthen the error handling
                for (QDomNode n = root.firstChild(); !n.isNull(); n = n.nextSibling())
                {
                    QDomElement e = n.toElement(); // try to convert the node to an element.
                    if (!e.isNull())
                    {
                        QString key = e.attribute("key");
                        ORIGIN_ASSERT(!key.isEmpty());

                        int oldType = -1;
                        if(e.hasAttribute("type"))
                        {
                            oldType = e.attribute("type").toInt();
                        }

                        QString value = e.attribute("value");

                        InternalSettingsMap::const_iterator iter = mInternalSettingsMap->find(SettingsKey(key));
                        if (iter == mInternalSettingsMap->constEnd() || (skipPerUserSettings && iter.value()->scope() == Origin::Services::Setting::LocalPerUser))
                        {
                            // ignore obsolete settings and local per user settings
                            continue;
                        }

                        if (iter.value()->encryption() == Origin::Services::Setting::Encrypted)
                        {
                            // decrypt legacy setting
                            Crypto::SimpleEncryption encryption;
                            value = QString::fromStdString(encryption.decrypt(value.toStdString()));
                            if (value.isEmpty())    // skip empty settings
                                continue;
                        }

                        switch (iter.value()->type())
                        {
                            // string parameters
                            case Origin::Services::Variant::String:
                                set(*(iter.value()), value);
                                break;

                            // int parameters
                            case Origin::Services::Variant::Int:
                                set(*(iter.value()), value.toInt());
                                break;

                            // int64 parameters
                            case Origin::Services::Variant::ULongLong:
                                set(*(iter.value()), value.toULongLong());
                                break;

                            // bool parameters
                            case Origin::Services::Variant::Bool:
                                if(oldType == QVariant::Int)
                                {
                                    set(*(iter.value()), (value.toInt() != 0));
                                }
                                else
                                {
                                    set(*(iter.value()), value == "true");
                                }
                                break;

                            default:
                                ORIGIN_ASSERT(0);
                                break;
                        }
                    }
                }
                settingsFile.close();

                if (deleteOldSettings)
                {
                    oldSettingsIndex = 0;
                    while(oldSettingsIndex < oldSettings.count())
                    {
                        oldSettingsExist = QFile::exists(oldSettings[oldSettingsIndex]);
                        if (oldSettingsExist)
                            QFile::remove(oldSettings[oldSettingsIndex]);

                        oldSettingsIndex++;
                    }
                }
            }
        }

        void SettingsManager::loadLocalMachineSettings()
        {
            ORIGIN_ASSERT(isInitialized());
            ORIGIN_ASSERT_MESSAGE(mLocalMachineSettings.isNull(), "local machine settings already configured");

            if (!mLocalMachineSettings.isNull())
                mLocalMachineSettings->sync();

            // production means no prefix! Use Variant to handle conversion to QString with type check
            QString environment = get(SETTING_EnvironmentName).toString();
            if (!environment.compare(env::production, Qt::CaseInsensitive))
                environment.clear();

            QSettings* settings = new QSettings(mXmlFormat, QSettings::SystemScope, ORGANIZATION, environment + LOCAL_SETTINGS_PREFIX);
            ORIGIN_ASSERT(settings);
            // TODO add settings file version information

            mLocalMachineSettings.reset(settings);
        }

        void SettingsManager::loadLocalPerAccountSettings()
        {
            ORIGIN_ASSERT(isInitialized());
            ORIGIN_ASSERT_MESSAGE(mLocalPerAccountSettings.isNull(), "local account settings already configured");

            if (!mLocalPerAccountSettings.isNull())
                mLocalPerAccountSettings->sync();

            // production means no prefix! Use Variant to handle conversion to QString with type check
            QString environment = Variant(get(SETTING_EnvironmentName));
            if (!environment.compare(env::production, Qt::CaseInsensitive))
                environment.clear();

            QSettings* settings = new QSettings(mXmlFormat, QSettings::UserScope, ORGANIZATION, environment + LOCAL_SETTINGS_PREFIX);
            ORIGIN_ASSERT(settings);

            mLocalPerAccountSettings.reset(settings);
        }

        void SettingsManager::loadLocalUserSettings(const Session::SessionRef session)
        {
            // session data already cached
            if (mCurrentLocalSession == session)
                return;

            ORIGIN_ASSERT(isInitialized());

            unloadLocalUserSettings();
            ORIGIN_ASSERT_MESSAGE(mLocalUserSettings.isNull(), "local user settings already configured");

            mCurrentLocalSession = session;

            // production means no prefix! Use Variant to handle conversion to QString with type check
            QString environment = Variant(get(SETTING_EnvironmentName));
            if (!environment.compare(env::production, Qt::CaseInsensitive))
                environment.clear();

            // convert "1234567890" to "0123456789abcdef.xml"
            QString nucleusUserId(Session::SessionService::nucleusUser(session));
            QString hashedNucleusUserId(QCryptographicHash::hash(nucleusUserId.toLatin1(), QCryptographicHash::Md5).toHex());

            QString filenameWithoutExtension(QString("%1_%2").arg(environment + LOCAL_SETTINGS_PREFIX).arg(hashedNucleusUserId));

            QSettings* settings = new QSettings(mXmlFormat, QSettings::UserScope, ORGANIZATION, filenameWithoutExtension);
            ORIGIN_ASSERT(settings);

            mLocalUserSettings.reset(settings);

            // per user settings contains the locale, so we have to load the override settings again (they will load the localized URLs as well!!!)
            loadOverrideSettings();

            // import old settings and delete the old settings files
            importOldSettings(false, true);
        }

        QStringList SettingsManager::activeMachineEnvironments()
        {
            // We are cycling through to see what kind of user settings environment this machine has touched. We are
            // doing this instead of saving it out explicitly because of a first user experience. If a user had a override
            // and deleted the EACore.ini without running this first, we could not detect they had multiple environments.
            QStringList activeEnv;

            // Get the directory of the local user settings. fileName returns the full path + file name. We need to take off file name.
            // - Getting path, seperating it out so we can access the filename
            QStringList temp = QDir().toNativeSeparators(mLocalUserSettings->fileName()).split(QDir::separator());
            // - Removing file name
            temp.removeLast();
            // - Putting it back together
            const QDir settingsDir = QDir(temp.join(QDir::separator()));

            // Go through all the .xml files in the settings directory
            const QStringList fileList = settingsDir.entryList(QStringList("*." + SETTINGS_FILE_EXTENSION), QDir::Files);
            foreach(QString fileName, fileList)
            {
                // If it's a user settings (contains local_USERHASH)
                temp = fileName.split(QString(LOCAL_SETTINGS_PREFIX + "_"));

                // Only when the filename contains the LOCAL_SETTINGS_PREFIX + "_" it will split, so we 
                // don’t need to explicitly test for it.
                if(temp.size() == 2)
                {   
                    // Convert to the real names.
                    if(temp[0].isEmpty())
                        activeEnv.append(env::production);
                    else
                        activeEnv.append(temp[0]);
                }
            }
            activeEnv.removeDuplicates();

            return activeEnv;
        }

        void SettingsManager::unloadLocalUserSettings()
        {
            mCurrentLocalSession.clear();
            if (!mLocalUserSettings.isNull())
                mLocalUserSettings->sync();
            delete mLocalUserSettings.take();
        }

        int SettingsManager::legacyINIpreferenceGet(const QString& iniFile, const QString& sSection, const QString& sEntry, QString& sValue)
        {
            std::wstring from(iniFile.toStdWString());
            EA::IO::IniFile appIniFile(from.c_str());

            EA::IO::String8 value;
            int ret = appIniFile.ReadEntry( sSection.toUtf8().constData(), sEntry.toUtf8().constData(), value );

            quint32 valueLen = value.size();
            sValue = QString::fromUtf8( value.c_str(), valueLen );

            return ret;
        }

        void SettingsManager::legacyINIPaths(const QString& iniFile, const QString& settingName)
        {
            QFile appIniFile(iniFile);
            if (!appIniFile.open(QIODevice::ReadOnly))
                return;

            QTextStream stream(&appIniFile);
            QString content(stream.readAll());
            int pos = 0;

            QRegExp regExp(settingName + "::");
            regExp.setCaseSensitivity(Qt::CaseInsensitive);

            while ((pos = regExp.indexIn(content, pos)) != -1)
            {
                //check for ';' which would mean that this is commented out
                if (pos > 0)
                {
                    QString commentStr = content.mid (pos - 1, 1);
                    if (QString::compare (commentStr, ";") == 0)
                    {
                        // next pair
                        pos += regExp.matchedLength();
                        continue;
                    }
                }

                QString key = content.mid(pos);

                // extract the key
                int endPos = key.indexOf("=");
                key = key.mid(0, endPos);
                key = key.trimmed();
                // make sure the original capitalization is applied
                int startPos = key.indexOf("::");
                key = settingName + key.mid(startPos);

                // extract the value
                QString value = content.mid(pos + 1 + endPos);
                value = value.mid(0, value.indexOf("\n"));
                value = value.trimmed();    // remove any whitespace

                // create new override setting, this will be used to set the value,
                // an setting name owned by the ContentInstallFlow will be
                // used to read the same setting
                Setting *s = new Setting(key, Variant::String, value, Setting::LocalOverrideAllUsers, Setting::ReadOnly);
                internalSet(*s, value);

                ORIGIN_LOG_DEBUG << settingName << "::" << key << " : " << value;

                // next pair
                pos += regExp.matchedLength();
            }

        }

        void SettingsManager::legacyINIBySection(const QString& iniFile, const QString& sectionName)
        {
            QFile appIniFile(iniFile);
            if (!appIniFile.open(QIODevice::ReadOnly))
                return;

            QTextStream stream(&appIniFile);
            QString content(stream.readAll());
            int pos = 0;

            QRegExp regExp("[a-zA-Z0-9]+[\t ]*=");

            while ((pos = regExp.indexIn(content, pos)) != -1)
            {
                //check for ';' which would mean that this is commented out
                if (pos > 0)
                {
                    QString commentStr = content.mid (pos - 1, 1);
                    if (QString::compare (commentStr, ";") == 0)
                    {
                        // next pair
                        pos += regExp.matchedLength();
                        continue;
                    }
                }

                QString key = content.mid(pos);

                // extract the key
                int endPos = key.indexOf("=");
                key = key.mid(0, endPos);
                key = key.trimmed();

                // extract the value
                QString value = content.mid(pos + 1 + endPos);
                value = value.mid(0, value.indexOf("\n"));
                value = value.trimmed();    // remove any whitespace

                QString tempValue;

                // does this setting match our section?
                if (legacyINIpreferenceGet(iniFile, sectionName, key, tempValue) > 0)
                {
                    const Setting *setting = NULL;
                    // does this setting exist?
                    if (Origin::Services::SettingsManager::instance()->getSettingByName(key, &setting))
                    {
                        set(*setting, tempValue);
                        ORIGIN_LOG_DEBUG << key << " : " << tempValue;
                    }
                    else
                    {
                        ORIGIN_LOG_DEBUG << "Setting not found " << key << " so we can't override it";
                        /*  // no need to support that currently!!!
                        // create new override setting
                        Setting *s = new Setting(key, Variant::String, value, Setting::LocalOverrideAllUsers, Setting::ReadOnly);
                        internalSet(*s, value);
                        */
                    }
                }

                // next pair
                pos += regExp.matchedLength();
            }

        }

        void SettingsManager::loadOverrideSettings()
        {
            unloadOverrideSettings();
            mOverrideSettings.reset(new QSettings(QSettings::registerFormat("xml", NULL, NULL), QSettings::SystemScope, "", ""));
            ORIGIN_ASSERT(mOverrideSettings.data());

#if defined(Q_OS_WIN)
            const QString coreAppIni = QCoreApplication::applicationDirPath() + QDir::separator() + "EACore_App.ini";
#elif defined(Q_OS_MAC)
            const QString coreAppIni = PlatformService::commonAppDataPath() /*+ QDir::separator()*/ + "EACore_App.ini";
#else
#error "Needs platform-specific implementation."
#endif
            const QString eaCoreSection = "EACore";
            const QString urlSection = "URLS";
            const QString certSection = "Cert";
            const QString featureSection = "Feature";
            const QString automationSection = "Automation";
            const QString qaSection = "QA";

            EA::IO::IniFile coreAppIniFile(coreAppIni.toUtf8().constData());
            QString tempValue;

            QString eaCoreIniFilePath = PlatformService::eacoreIniFilename();
            QFile eaCoreIniFileInfo(eaCoreIniFilePath);
            writeSetting(SETTING_OverridesEnabled, (eaCoreIniFileInfo.exists() && eaCoreIniFileInfo.size() > 0));

            // Read our IgnoreSSLCertErrors key from EACore.ini
            // This has to be done before we read our client network config below

            //Read in what configuration we should be using
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Connection", SETTING_EnvironmentName.name(), tempValue) > 0)
                set(SETTING_EnvironmentName, tempValue.toLower());

            // Read whether we're supposed to flush the web cache on startup
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Connection", SETTING_FlushWebCache.name(), tempValue) > 0)
            {
                if (tempValue.compare("true", Qt::CaseInsensitive) == 0 ||
                        tempValue.compare("1", Qt::CaseInsensitive) == 0)
                    set(SETTING_FlushWebCache, true);
            }

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_StoreInitialURL.name(), tempValue) > 0)
                set(SETTING_StoreInitialURL, tempValue);

            if(legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_SubscriptionRedemptionURL.name(), tempValue) > 0)
                set(SETTING_SubscriptionRedemptionURL, tempValue);

            if(legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_SubscriptionTrialEligibilityServiceURL.name(), tempValue) > 0)
                set(SETTING_SubscriptionTrialEligibilityServiceURL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_EmptyShelfURL.name(), tempValue) > 0)
                set(SETTING_EmptyShelfURL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_StoreProductURL.name(), tempValue) > 0)
                set(SETTING_StoreProductURL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_FreeGamesURL.name(), tempValue) > 0)
                set(SETTING_FreeGamesURL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_OriginXURL.name(), tempValue) > 0)
                set(SETTING_OriginXURL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_StoreOrderHistoryURL.name(), tempValue) > 0)
                set(SETTING_StoreOrderHistoryURL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_StoreSecurityURL.name(), tempValue) > 0)
                set(SETTING_StoreSecurityURL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_AccountSubscriptionURL.name(), tempValue) > 0)
                set(SETTING_AccountSubscriptionURL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_StorePrivacyURL.name(), tempValue) > 0)
                set(SETTING_StorePrivacyURL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_StorePaymentAndShippingURL.name(), tempValue) > 0)
                set(SETTING_StorePaymentAndShippingURL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_StoreRedemptionURL.name(), tempValue) > 0)
                set(SETTING_StoreRedemptionURL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_AvatarURL.name(), tempValue) > 0)
                set(SETTING_AvatarURL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_SignInPortalBaseUrl.name(), tempValue) > 0)
                set(SETTING_SignInPortalBaseUrl, tempValue);


            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_NewUserExpURL.name(), tempValue) > 0)
                set(SETTING_NewUserExpURL, tempValue);

            // Show promos even if they were shown recently enough to suppress?
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Connection", SETTING_OverridePromos.name(), tempValue) > 0)
            {
                if (tempValue.compare("enabled", Qt::CaseInsensitive) == 0)
                    set(SETTING_OverridePromos, PromosEnabled);
                else if (tempValue.compare("disabled", Qt::CaseInsensitive) == 0)
                    set(SETTING_OverridePromos, PromosDisabledNonProduction);
                else
                    set(SETTING_OverridePromos, PromosDefault);
            }

            // Need to check for command-line environments for automation testing
            QStringList arguments = QCoreApplication::arguments();
            for(int i = 1; i < arguments.count(); ++i)
            {
                if(arguments.at(i).startsWith("-EnvironmentName:"))
                {
                    tempValue = arguments.at(i);
                    tempValue = tempValue.mid(tempValue.indexOf(":") + 1);
                    set(SETTING_EnvironmentName, tempValue.toLower());
                    break;
                }
            }

            // parse PATH override settings
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideDownloadPath.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideDownloadSyncPackagePath.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideCommerceProfile.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideUseLegacyCatalog.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideEnableDLCUninstall.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideMonitoredInstall.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideMonitoredPlay.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideExternalInstallDialog.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideExternalPlayDialog.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverridePartnerPlatform.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverridePreloadPath.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideExecuteRegistryKey.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideExecuteFilename.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideInstallRegistryKey.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideInstallFilename.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideGameLauncherURL.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideGameLauncherURLIntegration.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideGameLauncherURLIdentityClientId.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideGameLauncherSoftwareId.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideVersion.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideBuildReleaseVersion.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideBuildIdentifier.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideSalePrice.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideBundleContents.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideThumbnailUrl.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideReleaseDate.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideIsForceKilledAtOwnershipExpiry.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideAchievementSet.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideSNOFolder.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideSNOUpdateCheck.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideSNOScheduledTime.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideSNOConfirmBuild.name());

            // nucleus 2.0
            legacyINIPaths(eaCoreIniFilePath, OverrideConnectPortalBaseUrl);
            legacyINIPaths(eaCoreIniFilePath, OverrideIdentityPortalBaseUrl);
            legacyINIPaths(eaCoreIniFilePath, OverrideSignInPortalBaseUrl);
            legacyINIPaths(eaCoreIniFilePath, OverrideLoginSuccessRedirectUrl);

            legacyINIPaths(eaCoreIniFilePath, SETTING_ShowDebugMenu.name());

            legacyINIPaths(eaCoreIniFilePath, OverrideClientId);
            legacyINIPaths(eaCoreIniFilePath, OverrideClientSecret);

            //free trials
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideTerminationDate.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideUseEndDate.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideItemSubType.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideCloudSaveContentIDFallback.name());
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideTrialDuration.name());

            legacyINIBySection(eaCoreIniFilePath, urlSection);
            legacyINIPaths(eaCoreIniFilePath, SETTING_OverrideIGOSetting.name());


// Debug only settings
#ifdef _DEBUG
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Debug", SETTING_IsolateProductIds.name(), tempValue) > 0)
            {
                set(SETTING_IsolateProductIds, tempValue);
            }

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Debug", SETTING_DebugEscalationService.name(), tempValue) > 0)
            {
                set(SETTING_DebugEscalationService, tempValue.toLower() == "true");
            }
#endif

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Connection", SETTING_OverrideInitURL.name(), tempValue) > 0)
                set(SETTING_OverrideInitURL, tempValue);

            // Sensitive override capability, do not document widely, do not include in Confluence
            // Name: GeolocationIPAddress
            // Value Type: IP4 address string
            // Default Values: none
            // Availability: Origin 8.5 onward
            // IP address to supply in calls to user entitlement requests. Can be set to "101.102.103.104" to force a
            // specific IP address for QA testing.
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Services", SETTING_GeolocationIPAddress.name(), tempValue) > 0)
                set(SETTING_GeolocationIPAddress, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Services", SETTING_OriginGeolocationTest.name(), tempValue) > 0)
                set(SETTING_OriginGeolocationTest, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_MotdURL.name(), tempValue) > 0)
            {
                // if URL contains "%1" pattern, substitute it with a locale
                if(tempValue.contains("%1"))
                    tempValue.replace("%1", readSetting(Services::SETTING_LOCALE));
                set(SETTING_MotdURL, tempValue);
            }

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_OriginAccountURL.name(), tempValue) > 0)
                set(SETTING_OriginAccountURL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_UpdateURL.name(), tempValue) > 0)
                set(SETTING_UpdateURL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_OriginConfigServiceURL.name(), tempValue) > 0)
                set(SETTING_OriginConfigServiceURL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_AchievementsBasePageURL.name(), tempValue) > 0)
                set(SETTING_AchievementsBasePageURL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_MyOriginPageURL.name(), tempValue) > 0)
                set(SETTING_MyOriginPageURL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_EbisuLockboxURL.name(), tempValue) > 0)
                set(SETTING_EbisuLockboxURL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_EbisuLockboxV3URL.name(), tempValue) > 0)
                set(SETTING_EbisuLockboxV3URL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_OverrideCustomerSupportPageUrl.name(), tempValue) > 0)
                set (SETTING_CustomerSupportURL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_LSX_PORT.name(), tempValue) > 0)
                set(SETTING_LSX_PORT, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_ENABLE_PROGRESSIVE_INSTALLER_SIMULATOR.name(), tempValue) > 0)
                set(SETTING_ENABLE_PROGRESSIVE_INSTALLER_SIMULATOR, tempValue);

            if(legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_PROGRESSIVE_INSTALLER_SIMULATOR_CONFIG.name(), tempValue) > 0)
                set(SETTING_PROGRESSIVE_INSTALLER_SIMULATOR_CONFIG, tempValue);

            if(legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_DisconnectSDKWhenNoEntitlement.name(), tempValue) > 0)
                set(SETTING_DisconnectSDKWhenNoEntitlement, tempValue);

            if(legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_PdlcStoreURL.name(), tempValue) > 0)
                set(SETTING_PdlcStoreURL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_StoreProductURL.name(), tempValue) > 0)
                set(SETTING_StoreProductURL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_StoreMasterTitleURL.name(), tempValue) > 0)
                set(SETTING_StoreMasterTitleURL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_StoreEntitleFreeGameURL.name(), tempValue) > 0)
                set(SETTING_StoreEntitleFreeGameURL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_FreeGamesURL.name(), tempValue) > 0)
                set(SETTING_FreeGamesURL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_EcommerceV1URL.name(), tempValue) > 0)
                set(SETTING_EcommerceV1URL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_EcommerceURL.name(), tempValue) > 0)
                set(SETTING_EcommerceURL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_InicisSsoCheckoutBaseURL.name(), tempValue) > 0)
                set(SETTING_InicisSsoCheckoutBaseURL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, featureSection, SETTING_ShowMyOrigin.name(), tempValue) > 0)
                set(SETTING_ShowMyOrigin, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, featureSection, SETTING_AchievementsEnabled.name(), tempValue) > 0)
                set(SETTING_AchievementsEnabled, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, featureSection, SETTING_SubscriptionEnabled.name(), tempValue) > 0)
                set(SETTING_SubscriptionEnabled, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, featureSection, SETTING_SubscriptionEntitlementOfferId.name(), tempValue) > 0)
                set(SETTING_SubscriptionEntitlementOfferId, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, featureSection, SETTING_SubscriptionEntitlementRetiringTime.name(), tempValue) > 0)
                set(SETTING_SubscriptionEntitlementRetiringTime, tempValue.toInt());

            if (legacyINIpreferenceGet(eaCoreIniFilePath, featureSection, SETTING_SubscriptionEntitlementMasterIdUpdateAvaliable.name(), tempValue) > 0)
                set(SETTING_SubscriptionEntitlementMasterIdUpdateAvaliable, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, featureSection, SETTING_SubscriptionExpirationReason.name(), tempValue) > 0)
                set(SETTING_SubscriptionExpirationReason, tempValue.toInt());

            if (legacyINIpreferenceGet(eaCoreIniFilePath, featureSection, SETTING_SubscriptionExpirationDate.name(), tempValue) > 0)
                set(SETTING_SubscriptionExpirationDate, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, featureSection, SETTING_SubscriptionMaxOfflinePlay.name(), tempValue) > 0)
                set(SETTING_SubscriptionMaxOfflinePlay, tempValue.toInt());

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_OriginConfigDebug.name(), tempValue) > 0)
                set(SETTING_OriginConfigDebug, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_CloudSavesDebug.name(), tempValue) > 0)
                set(SETTING_CloudSavesDebug, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_ContentDebug.name(), tempValue) > 0)
                set(SETTING_ContentDebug, tempValue.toLower() == "true");
            else
                set(SETTING_ContentDebug, readSetting(SETTING_OverridesEnabled).toQVariant());

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_qaForcedUpdateCheckInterval.name(), tempValue) > 0)
                set(SETTING_qaForcedUpdateCheckInterval, tempValue.toInt());

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_OverrideRefreshEntitlementsOnDirtyBits.name(), tempValue) > 0)
                set(SETTING_OverrideRefreshEntitlementsOnDirtyBits, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_OverrideOnTheHouseQueryIntervalMS.name(), tempValue) > 0)
                set(SETTING_OverrideOnTheHouseQueryIntervalMS, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_OverrideDirtyBitsEntitlementRefreshTimeout.name(), tempValue) > 0)
                set(SETTING_OverrideDirtyBitsEntitlementRefreshTimeout, tempValue.toInt());

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_OverrideDirtyBitsTelemetryDelay.name(), tempValue) > 0)
                set(SETTING_OverrideDirtyBitsTelemetryDelay, tempValue.toInt());

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_FakeDirtyBitsMessageFile.name(), tempValue) > 0)
                set(SETTING_FakeDirtyBitsMessageFile, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_OverrideFullEntitlementRefreshThrottle.name(), tempValue) >= 0)
                set(SETTING_OverrideFullEntitlementRefreshThrottle, tempValue.toInt());

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_OverrideLoginPageRefreshTimer.name(), tempValue) >= 0)
                set(SETTING_OverrideLoginPageRefreshTimer, tempValue.toInt());

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_KGLTimer.name(), tempValue) > 0)
                set(SETTING_KGLTimer, tempValue);
            else
                set(SETTING_KGLTimer, "3600"); // default to 60 minutes

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_DebugMenuEntitlementID.name(), tempValue) > 0)
                set(SETTING_DebugMenuEntitlementID, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_DebugMenuEulaPathOverride.name(), tempValue) > 0)
                set(SETTING_DebugMenuEulaPathOverride, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_ForceContentWatermarking.name(), tempValue) > 0)
                set(SETTING_ForceContentWatermarking, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_EnableGameLaunchCancel.name(), tempValue) > 0)
                set(SETTING_EnableGameLaunchCancel, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_DisableEntitlementFilter.name(), tempValue) > 0)
                set(SETTING_DisableEntitlementFilter, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_EnableFeatureCallouts.name(), tempValue) > 0)
                set(SETTING_EnableFeatureCallouts, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_ForceLockboxURL.name(), tempValue) > 0)
                set(SETTING_ForceLockboxURL, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_EnableDownloadOverrideTelemetry.name(), tempValue) > 0)
                set(SETTING_EnableDownloadOverrideTelemetry, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_DisableEmbargoMode.name(), tempValue) > 0)
                set(SETTING_DisableEmbargoMode, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_OverrideOSVersion.name(), tempValue) > 0)
                set(SETTING_OverrideOSVersion, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_OverrideCatalogDefinitionLookupTelemetryInterval.name(), tempValue) > 0)
                set(SETTING_OverrideCatalogDefinitionLookupTelemetryInterval, tempValue.toLongLong());

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_OverrideCatalogDefinitionLookupRetryInterval.name(), tempValue) > 0)
                set(SETTING_OverrideCatalogDefinitionLookupRetryInterval, tempValue.toLongLong());

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_OverrideCatalogDefinitionRefreshInterval.name(), tempValue) > 0)
                set(SETTING_OverrideCatalogDefinitionRefreshInterval, tempValue.toLongLong());

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_OverrideCatalogDefinitionMaxAgeDays.name(), tempValue) > 0)
                set(SETTING_OverrideCatalogDefinitionMaxAgeDays, tempValue.toLongLong());
            
            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_OverrideLoadTimeoutError.name(), tempValue) > 0)
                set(SETTING_OverrideLoadTimeoutError, tempValue.toLongLong());
                
            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_EnableIGODebugMenu.name(), tempValue) > 0)
                set(SETTING_EnableIGODebugMenu, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_EnableIGOStressTest.name(), tempValue) > 0)
                set(SETTING_EnableIGOStressTest, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_IGOReducedBrowser.name(), tempValue) > 0)
                set(SETTING_IGOReducedBrowser, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_IGOMiniAppBrowser.name(), tempValue) > 0)
                set(SETTING_IGOMiniAppBrowser, tempValue.toLower() == "true" || tempValue.toLower() == "1");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_MinNetPromoterTestRange.name(), tempValue) > 0)
                set(SETTING_MinNetPromoterTestRange, tempValue.toInt());

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_MaxNetPromoterTestRange.name(), tempValue) > 0)
                set(SETTING_MaxNetPromoterTestRange, tempValue.toInt());

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_ShowNetPromoter.name(), tempValue) > 0)
                set(SETTING_ShowNetPromoter, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_ShowGameNetPromoter.name(), tempValue) > 0)
                set(SETTING_ShowGameNetPromoter, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_CredentialsUserName.name(), tempValue) > 0)
                set(SETTING_CredentialsUserName, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_CredentialsPassword.name(), tempValue) > 0)
                set(SETTING_CredentialsPassword, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, OverrideConnectPortalBaseUrl, tempValue) > 0)
                set(SETTING_ConnectPortalBaseUrl, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, OverrideIdentityPortalBaseUrl, tempValue) > 0)
                set(SETTING_IdentityPortalBaseUrl, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, OverrideLoginSuccessRedirectUrl, tempValue) > 0)
                set(SETTING_LoginSuccessRedirectUrl, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, OverrideSignInPortalBaseUrl, tempValue) > 0)
                set(SETTING_SignInPortalBaseUrl, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, OverrideClientId, tempValue) > 0)
                set(SETTING_ClientId, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, OverrideClientSecret, tempValue) > 0)
            {
                set(SETTING_ClientSecret, tempValue);
            }
            else
            {
                QString environment = get(SETTING_EnvironmentName).toString();
                if (environment.compare(env::production, Qt::CaseInsensitive) == 0)
                {
                    set(SETTING_ClientSecret, ClientSecretProduction);
                }
                else
                {
                    set(SETTING_ClientSecret, ClientSecretIntegration);
                }
            }

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_OverrideCatalogIGODefaultURL.name(), tempValue) > 0)
                set(SETTING_OverrideCatalogIGODefaultURL, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_MaxOfflineWebApplicationCacheSize.name(), tempValue) > 0)
                set(SETTING_MaxOfflineWebApplicationCacheSize, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_ReduceDirtyBitsCappedTimeout.name(), tempValue) > 0)
                set(SETTING_ReduceDirtyBitsCappedTimeout, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_ShowDebugMenu.name(), tempValue) > 0)
                set(SETTING_ShowDebugMenu, tempValue.toLower() == "true");

			if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_OverridePromoBrowserOfferCache.name(), tempValue) > 0)
				set(SETTING_OverridePromoBrowserOfferCache, tempValue);

			if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_VoiceEnable.name(), tempValue) > 0)
				set(SETTING_VoiceEnable, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "CrashTesting", SETTING_CrashOnStartup.name(), tempValue) > 0)
                set(SETTING_CrashOnStartup, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "CrashTesting", SETTING_CrashOnExit.name(), tempValue) > 0)
                set(SETTING_CrashOnExit, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(coreAppIni, eaCoreSection, SETTING_CrashOnExit.name(), tempValue) > 0)
                set(SETTING_CrashOnExit, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "DownloaderTuning", SETTING_MaxWorkersForAllSessions.name(), tempValue) > 0)
                set(SETTING_MaxWorkersForAllSessions, tempValue.toInt());

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "DownloaderTuning", SETTING_SessionBaseTickRate.name(), tempValue) > 0)
                set(SETTING_SessionBaseTickRate, tempValue.toInt());

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "DownloaderTuning", SETTING_RateDecreasePerActiveSession.name(), tempValue) > 0)
                set(SETTING_RateDecreasePerActiveSession, tempValue.toInt());

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "DownloaderTuning", SETTING_MinRequestSize.name(), tempValue) > 0)
                set(SETTING_MinRequestSize, tempValue.toInt());

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "DownloaderTuning", SETTING_MaxRequestSize.name(), tempValue) > 0)
                set(SETTING_MaxRequestSize, tempValue.toInt());

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "DownloaderTuning", SETTING_MaxWorkersPerFileSession.name(), tempValue) > 0)
                set(SETTING_MaxWorkersPerFileSession, tempValue.toInt());

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "DownloaderTuning", SETTING_MaxWorkersPerHttpSession.name(), tempValue) > 0)
                set(SETTING_MaxWorkersPerHttpSession, tempValue.toInt());

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "DownloaderTuning", SETTING_IdealFileRequestSize.name(), tempValue) > 0)
                set(SETTING_IdealFileRequestSize, tempValue.toInt());

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "DownloaderTuning", SETTING_IdealHttpRequestSize.name(), tempValue) > 0)
                set(SETTING_IdealHttpRequestSize, tempValue.toInt());

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "DownloaderTuning", SETTING_MaxUnpackServiceThreads.name(), tempValue) > 0)
                set(SETTING_MaxUnpackServiceThreads, tempValue.toInt());

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "DownloaderTuning", SETTING_MaxUnpackCompressedChunkSize.name(), tempValue) > 0)
                set(SETTING_MaxUnpackCompressedChunkSize, tempValue.toInt());

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "DownloaderTuning", SETTING_MaxUnpackUncompressedChunkSize.name(), tempValue) > 0)
                set(SETTING_MaxUnpackUncompressedChunkSize, tempValue.toInt());

            //  Heartbeat:  If flag is set, open the log file for QA testing
            if (legacyINIpreferenceGet(coreAppIni, eaCoreSection, SETTING_HeartbeatLog.name(), tempValue) > 0)
                set(SETTING_HeartbeatLog, tempValue.toLower() == "true");

            // IGO
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "IGO", SETTING_EnableIGOForAll.name(), tempValue) > 0)
                set(SETTING_EnableIGOForAll, tempValue == "1" || tempValue == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "IGO", SETTING_ForceFullIGOLogging.name(), tempValue) > 0)
                set(SETTING_ForceFullIGOLogging, tempValue == "1" || tempValue == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "IGO", SETTING_ForceMinIGOResolution.name(), tempValue) > 0)
                set(SETTING_ForceMinIGOResolution, tempValue);

            // These values are ones that we will set defaults for immediately since they are not URLs that
            // we store in the client

            if (legacyINIpreferenceGet(eaCoreIniFilePath, certSection, SETTING_IgnoreSSLCertError.name(), tempValue) > 0)
                set(SETTING_IgnoreSSLCertError, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, featureSection, SETTING_BlockingBehavior.name(), tempValue) > 0)
                set(SETTING_BlockingBehavior, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, featureSection, SETTING_SingleLogin.name(), tempValue) > 0)
                set(SETTING_SingleLogin, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Telemetry", SETTING_TelemetryXML.name(), tempValue) > 0)
                set(SETTING_TelemetryXML, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "BugSentry", SETTING_UseTestServer.name(), tempValue) > 0)
                set(SETTING_UseTestServer, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "BugSentry", SETTING_FullMemoryDump.name(), tempValue) > 0)
                set(SETTING_FullMemoryDump, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Logging", SETTING_LogConsoleDevice.name(), tempValue) > 0)
                set(SETTING_LogConsoleDevice, tempValue.toLower());

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Logging", SETTING_LogDebug.name(), tempValue) > 0)
                set(SETTING_LogDebug, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Logging", SETTING_LogCatalog.name(), tempValue) > 0)
                set(SETTING_LogCatalog, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Logging", SETTING_LogDirtyBits.name(), tempValue) > 0)
                set(SETTING_LogDirtyBits, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Logging", SETTING_LogDownload.name(), tempValue) > 0)
                set(SETTING_LogDownload, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Logging", SETTING_LogSonar.name(), tempValue) > 0)
                set(SETTING_LogSonar, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Logging", SETTING_LogSonarDirectSoundCapture.name(), tempValue) > 0)
                set(SETTING_LogSonarDirectSoundCapture, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Logging", SETTING_LogSonarDirectSoundPlayback.name(), tempValue) > 0)
                set(SETTING_LogSonarDirectSoundPlayback, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Logging", SETTING_LogSonarJitterMetricsLevel.name(), tempValue) > 0)
                set(SETTING_LogSonarJitterMetricsLevel, tempValue.toInt());

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Logging", SETTING_LogSonarJitterTracing.name(), tempValue) > 0)
                set(SETTING_LogSonarJitterTracing, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Logging", SETTING_LogSonarPayloadTracing.name(), tempValue) > 0)
                set(SETTING_LogSonarPayloadTracing, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Logging", SETTING_LogSonarSpeexCapture.name(), tempValue) > 0)
                set(SETTING_LogSonarSpeexCapture, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Logging", SETTING_LogSonarSpeexPlayback.name(), tempValue) > 0)
                set(SETTING_LogSonarSpeexPlayback, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Logging", SETTING_LogSonarTiming.name(), tempValue) > 0)
                set(SETTING_LogSonarTiming, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Logging", SETTING_LogSonarConnectionStats.name(), tempValue) > 0)
                set(SETTING_LogSonarConnectionStats, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Logging", SETTING_LogVoip.name(), tempValue) > 0)
                set(SETTING_LogVoip, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Logging", SETTING_LogXep0115.name(), tempValue) > 0)
                set(SETTING_LogXep0115, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Logging", SETTING_LogXmpp.name(), tempValue) > 0)
                set(SETTING_LogXmpp, tempValue.toLower() == "true");

			if (legacyINIpreferenceGet(eaCoreIniFilePath, "Logging", SETTING_LogGroupServiceREST.name(), tempValue) > 0)
				set(SETTING_LogGroupServiceREST, tempValue.toLower() == "true");

			if (legacyINIpreferenceGet(eaCoreIniFilePath, "Logging", SETTING_LogVoiceServiceREST.name(), tempValue) > 0)
				set(SETTING_LogVoiceServiceREST, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, featureSection, SETTING_DisableNetPromoter.name(), tempValue) > 0)
                set(SETTING_DisableNetPromoter, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, featureSection, SETTING_DisableMotd.name(), tempValue) > 0)
                set(SETTING_DisableMotd, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, featureSection, SETTING_CdnOverride.name(), tempValue) > 0)
                set(SETTING_CdnOverride, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, featureSection, SETTING_EnableITOCRCErrors.name(), tempValue) > 0)
                set(SETTING_EnableITOCRCErrors, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, featureSection, SETTING_EnableHTTPS.name(), tempValue) > 0)
                set(SETTING_EnableHTTPS, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, featureSection, SETTING_EnableDownloaderSafeMode.name(), tempValue) > 0)
                set(SETTING_EnableDownloaderSafeMode, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "ProgressiveInstall", SETTING_EnableProgressiveInstall.name(), tempValue) > 0)
                set(SETTING_EnableProgressiveInstall, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "ProgressiveInstall", SETTING_EnableSteppedDownload.name(), tempValue) > 0)
                set(SETTING_EnableSteppedDownload, tempValue.toLower() == "true");

            // Give dev's an available override to not use the new Alpha login URL
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "MacAlphaTrial", SETTING_AlphaLogin.name(), tempValue) > 0)
                set(SETTING_AlphaLogin, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, automationSection, SETTING_NotificationExpiryTime.name(), tempValue) > 0)
                set(SETTING_NotificationExpiryTime, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, automationSection, SETTING_DisableNotifications.name(), tempValue) > 0)
                set(SETTING_DisableNotifications, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, automationSection, SETTING_DisableIdling.name(), tempValue) > 0)
                set(SETTING_DisableIdling, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_NavigationDebugging.name(), tempValue) > 0)
                set(SETTING_NavigationDebugging, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_DownloadDebugEnabled.name(), tempValue) > 0)
                set(SETTING_DownloadDebugEnabled, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_PurgeAccessLicenses.name(), tempValue) > 0)
                set(SETTING_PurgeAccessLicenses, tempValue.toLower() == "true");


            // Gives server team the ability to point the client to an html file for OtH feature
            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_OnTheHouseOverridePromoURL.name(), tempValue) > 0)
                set(SETTING_OnTheHouseOverridePromoURL, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_OnTheHouseStoreURL.name(), tempValue) > 0)
                set(SETTING_OnTheHouseStoreURL, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_SubscriptionStoreURL.name(), tempValue) > 0)
                set(SETTING_SubscriptionStoreURL, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_SubscriptionVaultURL.name(), tempValue) > 0)
                set(SETTING_SubscriptionVaultURL, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_SubscriptionFaqURL.name(), tempValue) > 0)
                set(SETTING_SubscriptionFaqURL, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_SubscriptionInitialURL.name(), tempValue) > 0)
                set(SETTING_SubscriptionInitialURL, tempValue);

            // Voip
            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_VoipAddressBefore.name(), tempValue) > 0)
                set(SETTING_VoipAddressBefore, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_VoipAddressAfter.name(), tempValue) > 0)
                set(SETTING_VoipAddressAfter, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_VoipForceSurvey.name(), tempValue) > 0)
                set(SETTING_VoipForceSurvey, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_VoipForceUnexpectedEnd.name(), tempValue) > 0)
                set(SETTING_VoipForceUnexpectedEnd, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_ForceChatGroupDeleteFail.name(), tempValue) > 0)
                set(SETTING_ForceChatGroupDeleteFail, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_SonarOneMinuteInactivityTimeout.name(), tempValue) > 0)
                set(SETTING_SonarOneMinuteInactivityTimeout, tempValue.toLower() == "true");

            if (legacyINIpreferenceGet(eaCoreIniFilePath, urlSection, SETTING_VoiceURL.name(), tempValue) > 0)
                set(SETTING_VoiceURL, tempValue);

            // Sonar
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Sonar", SETTING_SonarClientPingInterval.name(), tempValue) > 0)
                set(SETTING_SonarClientPingInterval, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Sonar", SETTING_SonarClientTickInterval.name(), tempValue) > 0)
                set(SETTING_SonarClientTickInterval, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Sonar", SETTING_SonarJitterBufferSize.name(), tempValue) > 0)
                set(SETTING_SonarJitterBufferSize, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Sonar", SETTING_SonarRegisterTimeout.name(), tempValue) > 0)
                set(SETTING_SonarRegisterTimeout, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Sonar", SETTING_SonarRegisterMaxRetries.name(), tempValue) > 0)
                set(SETTING_SonarRegisterMaxRetries, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Sonar", SETTING_SonarNetworkPingInterval.name(), tempValue) > 0)
                set(SETTING_SonarNetworkPingInterval, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Sonar", SETTING_SonarEnableRemoteEcho.name(), tempValue) > 0)
                set(SETTING_SonarEnableRemoteEcho, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Sonar", SETTING_SonarSpeexAgcEnable.name(), tempValue) > 0)
                set(SETTING_SonarSpeexAgcEnable, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Sonar", SETTING_SonarSpeexAgcLevel.name(), tempValue) > 0)
                set(SETTING_SonarSpeexAgcLevel, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Sonar", SETTING_SonarSpeexComplexity.name(), tempValue) > 0)
                set(SETTING_SonarSpeexComplexity, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Sonar", SETTING_SonarSpeexQuality.name(), tempValue) > 0)
                set(SETTING_SonarSpeexQuality, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Sonar", SETTING_SonarSpeexVbrEnable.name(), tempValue) > 0)
                set(SETTING_SonarSpeexVbrEnable, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Sonar", SETTING_SonarSpeexVbrQuality.name(), tempValue) > 0)
                set(SETTING_SonarSpeexVbrQuality, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Sonar", SETTING_EchoCancellation.name(), tempValue) > 0)
                set(SETTING_EchoCancellation, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Sonar", SETTING_SonarNetworkGoodLoss.name(), tempValue) > 0)
                set(SETTING_SonarNetworkGoodLoss, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Sonar", SETTING_SonarNetworkGoodJitter.name(), tempValue) > 0)
                set(SETTING_SonarNetworkGoodJitter, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Sonar", SETTING_SonarNetworkGoodPing.name(), tempValue) > 0)
                set(SETTING_SonarNetworkGoodPing, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Sonar", SETTING_SonarNetworkOkLoss.name(), tempValue) > 0)
                set(SETTING_SonarNetworkOkLoss, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Sonar", SETTING_SonarNetworkOkJitter.name(), tempValue) > 0)
                set(SETTING_SonarNetworkOkJitter, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Sonar", SETTING_SonarNetworkOkPing.name(), tempValue) > 0)
                set(SETTING_SonarNetworkOkPing, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Sonar", SETTING_SonarNetworkPoorLoss.name(), tempValue) > 0)
                set(SETTING_SonarNetworkPoorLoss, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Sonar", SETTING_SonarNetworkPoorJitter.name(), tempValue) > 0)
                set(SETTING_SonarNetworkPoorJitter, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Sonar", SETTING_SonarNetworkPoorPing.name(), tempValue) > 0)
                set(SETTING_SonarNetworkPoorPing, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Sonar", SETTING_SonarNetworkQualityPingStartupDuration.name(), tempValue) > 0)
                set(SETTING_SonarNetworkQualityPingStartupDuration, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Sonar", SETTING_SonarNetworkQualityPingShortInterval.name(), tempValue) > 0)
                set(SETTING_SonarNetworkQualityPingShortInterval, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "Sonar", SETTING_SonarNetworkQualityPingLongInterval.name(), tempValue) > 0)
                set(SETTING_SonarNetworkQualityPingLongInterval, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, "QA", SETTING_SonarTestingAudioError.name(), tempValue) > 0)
                set(SETTING_SonarTestingAudioError, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "QA", SETTING_SonarTestingRegisterPacketLossPercentage.name(), tempValue) > 0)
                set(SETTING_SonarTestingRegisterPacketLossPercentage, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, "QA", SETTING_SonarTestingEmptyConversationTimeoutDuration.name(), tempValue) > 0)
                set(SETTING_SonarTestingEmptyConversationTimeoutDuration, tempValue);
			if (legacyINIpreferenceGet(eaCoreIniFilePath, "QA", SETTING_SonarTestingUserLocation.name(), tempValue) > 0)
				set(SETTING_SonarTestingUserLocation, tempValue);
            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_SonarTestingVoiceServerRegisterResponse.name(), tempValue) > 0)
                set(SETTING_SonarTestingVoiceServerRegisterResponse, tempValue);

            if (legacyINIpreferenceGet(eaCoreIniFilePath, qaSection, SETTING_OriginSessionKey.name(), tempValue) > 0)
                set(SETTING_OriginSessionKey, tempValue);
            else
                set(SETTING_OriginSessionKey, QUuid::createUuid().toString().toUpper());
        }

        void SettingsManager::unloadOverrideSettings()
        {
            initInternalSettingsOverrideMap();
            QReadLocker lock(mInternalSettingsOverrideMapLock);

            QList<const Setting*> deleteList;
            InternalSettingsOverrideMap::iterator iter = mInternalSettingsOverrideMap->begin();
            while(iter != mInternalSettingsOverrideMap->end())
            {
                deleteList.push_back(*iter);
                ++iter;
            }
            lock.unlock();

            for (int i = 0; i < deleteList.count(); ++i)
                delete deleteList[i];

            freeInternalSettingsOverrideMap();
            ORIGIN_ASSERT(mInternalSettingsOverrideMap == NULL);

            delete mOverrideSettings.take();
        }

        void SettingsManager::reloadProductOverrideSettings()
        {
            initInternalSettingsOverrideMap();

            // delete any existing dynamic overrides
            QReadLocker mapLock(mInternalSettingsOverrideMapLock);

            QList<const Setting*> deleteList;
            InternalSettingsOverrideMap::iterator iter = mInternalSettingsOverrideMap->begin();
            while(iter != mInternalSettingsOverrideMap->end())
            {
                if (isProductOverrideSetting(*(*iter)))
                {
                    // add the setting object to the delete list.  deleting it here directly is not allowed.
                    deleteList.push_back(*iter);
                }
                ++iter;
            }
            mapLock.unlock();

            // delete the setting objects
            QWriteLocker settingsLock(mOverrideSettingsLock);
            for (int i = 0; i < deleteList.count(); ++i)
            {
                // delete the setting entry from the QSettings object
                mOverrideSettings->remove(deleteList[i]->name());

                // the setting deconstructor will remove the setting's entry in mInternalSettingsOverrideMap for us
                delete deleteList[i];
            }
            settingsLock.unlock();

            // read/set dynamic overrides from eacore.ini
            QString eaCoreIniFilePath = PlatformService::eacoreIniFilename();

            foreach (QString overridePrefix, mProductOverrideSettingsPrefixes)
            {
                legacyINIPaths(eaCoreIniFilePath, overridePrefix);
            }
        }

        bool SettingsManager::isProductOverrideSetting(const Setting& setting)
        {
            return isProductOverrideSetting(setting.name());
        }

        bool SettingsManager::isProductOverrideSetting(const QString& settingName)
        {
            foreach (const QString prefix, mProductOverrideSettingsPrefixes)
            {
                if (settingName.startsWith(prefix, Qt::CaseInsensitive))
                {
                    // all of the product override settings names will begin with an element of mProductOverrideSettingsPrefixes.
                    // for example, "OverrideDownloadPath::".
                    return true;
                }
            }
            return false;
        }

        QString createURL(const QString& myEnv, const QString& myUrl)
        {
            if ( myEnv.compare("production", Qt::CaseInsensitive) == 0)
            {
                return QString("https://" + myUrl);
            }
            else
            {
                return QString("https://" + myEnv + "." + myUrl);
            }
        }

        // createURL helper
        QString createURL(const QString& myUrl)
        {
            return QString("https://" + myUrl);
        }

        void SettingsManager::loadingFinished()
        {
            emit settingsLoaded();
        }

        void SettingsManager::savingFinished()
        {
            emit settingsSaved();
        }


        /// \fn SettingsManager::inGameHotkeyDefault()
        /// \brief generates the default IGO hotkey
        qulonglong SettingsManager::inGameHotkeyDefault()
        {
            //EbisuCommon::InGameHotKey hotkey(Qt::ShiftModifier, Qt::Key_F1, VK_F1);
            //return hotkey.toULongLong();
#if defined(ORIGIN_PC)
            const qulonglong platformKey = VK_F1;
            return (static_cast<qulonglong>(Qt::ShiftModifier | Qt::Key_F1) << 32) | platformKey;
#elif defined(ORIGIN_MAC)
            const qulonglong platformKey = kVK_ANSI_1;
            return (static_cast<qulonglong>(Qt::ControlModifier | Qt::Key_1) << 32) | platformKey;
#else
#error Must specialize for other platform.
#endif
        }

        qulonglong SettingsManager::pinHotkeyDefault()
        {
            //EbisuCommon::InGameHotKey hotkey(Qt::ShiftModifier, Qt::Key_F2, VK_F2);
            //return hotkey.toULongLong();
#if defined(ORIGIN_PC)
            const qulonglong platformKey = VK_F2;
            return (static_cast<qulonglong>(Qt::ShiftModifier | Qt::Key_F2) << 32) | platformKey;
#elif defined(ORIGIN_MAC)
            const qulonglong platformKey = kVK_ANSI_2;
            return (static_cast<qulonglong>(Qt::ControlModifier | Qt::Key_2) << 32) | platformKey;
#else
#error Must specialize for other platform.
#endif
        }

#if ENABLE_VOICE_CHAT
        qulonglong SettingsManager::pushToTalkKeyDefault()
        {
            //EbisuCommon::InGameHotKey hotkey(Qt::NoModifier, Qt::Key_Control, 0x01000021);
            //return hotkey.toULongLong();
#if defined(ORIGIN_PC)
            const qulonglong platformKey = VK_CONTROL; // "Ctrl"
            return (static_cast<qulonglong>(Qt::Key_Control) << 32) | platformKey;
#else
#error Must specialize for other platform.
#endif
        }
#endif

#if defined(Q_OS_WIN)
        // The DIP folder drive default must coincide with the application
        // drive i.e. where it was installed.
        QString setCorrectDipFolder(const QString& dipFolder)
        {
            static const QString sep(":");
            const QStringList dipFolderList = dipFolder.split(sep);

            QString myAppFolder = QDir::currentPath();
            const QStringList myAppFolderList = myAppFolder.split(sep);

            QString myDipFolder = dipFolder;
            if (dipFolderList[0] != myAppFolderList[0])
            {
                // the application and the DIP folder are in different
                // drives: Lets make the DIP drive to be the same as the
                // application drive
                myDipFolder = myAppFolderList[0] + sep + dipFolderList[1];
            }

            return myDipFolder;
        }
#endif

        /// \fn SettingsManager::downloadInPlaceDefault()
        /// \brief generates the default DIP folder
        QString const SettingsManager::downloadInPlaceDefault()
        {
            QString dipFolder;

#if defined(ORIGIN_PC)
            dipFolder = GetDirectory(CSIDL_PROGRAM_FILES);
            dipFolder = setCorrectDipFolder(dipFolder);
            dipFolder += "\\Origin Games";
#elif defined(ORIGIN_MAC)
            dipFolder = PlatformService::getStorageLocation(QStandardPaths::ApplicationsLocation);
#else
#error Must specialize for other platform.
#endif

            return dipFolder;
        }

        /// \fn SettingsManager::downloadCacheDefault()
        /// \brief generates the default download cache folder
        QString const SettingsManager::downloadCacheDefault()
        {
            return PlatformService::machineStoragePath() + "DownloadCache";
        }

        void SettingsManager::reportSyncFailure(QSettings const& settings) const
        {
            const QSettings::Status status = settings.status();

            QString statusEnumString;
            switch(status)
            {
                case QSettings::AccessError:
                    statusEnumString = "AccessError";
                    break;
                case QSettings::FormatError:
                    statusEnumString = "FormatError";
                    break;
                default:
                    QTextStream(&statusEnumString) << "UNKOWN-" << status;
                    break;
            }

            const QString settingsFilename(settings.fileName());

            if (QFile::exists(settingsFilename))
            {
                static const int ERRNO_SUCCESS = 0;

                // Attempt to open the file for read
                const bool readable = QFile(settingsFilename).open(QIODevice::ReadOnly);

#if defined(Q_OS_WIN)
                // Obtain last OS error in case opening for read failed
                DWORD sysReadError = ::GetLastError();
#elif defined(ORIGIN_MAC)
                int sysReadError = readable ? ERRNO_SUCCESS : errno;
#else
#error "Platform not supported!"
#endif

                // Attempt to open the file for write
                const bool writable = QFile(settingsFilename).open(QIODevice::Append);

#if defined(Q_OS_WIN)
                // Obtain last OS error in case opening for write failed
                DWORD sysWriteError = ::GetLastError();
#elif defined(ORIGIN_MAC)
                int sysWriteError = writable ? ERRNO_SUCCESS : errno;
#else
#error "Platform not supported!"
#endif

                QString windowsFileAttr;

#if defined (Q_OS_WIN)
                // Because QFile's permissions flag is unreliable on NTFS drives, query Windows
                // file attributes for the file and report that also (this information only
                // applies to windows-based clients, obviously).
                DWORD fileAttributes = ::GetFileAttributesA(settingsFilename.toLocal8Bit().constData());
                windowsFileAttr = QString("0x%1").arg(QString::number(fileAttributes, 16));
#endif

                QString permString;

#if defined(ORIGIN_MAC)
                // Report QFile permissions flag for Mac only.
                // :WARN: as of Qt 5.3, this flag is unreliable for NTFS drives unless a flag
                // is enabled that could degrade performance.
                QFileDevice::Permissions permissions = QFile(settingsFilename).permissions();
                permString = QString("0x%1").arg(QString::number(permissions, 16));
#endif

                emit localPerAccountSettingFileError(
                    settingsFilename,
                    statusEnumString,
                    readable,
                    writable,
                    sysReadError,
                    sysWriteError,
                    windowsFileAttr,
                    permString);

            }

            else
            {
                emit localPerAccountSettingFileError(
                    settingsFilename,
                    statusEnumString,
                    false,
                    false,
                    0,
                    0,
                    "noExist",
                    "noExist");
            }
        }

    } // namespace Services
} // namespace Origin

namespace
{
    bool readXmlFile(QIODevice &device, QSettings::SettingsMap &map)
    {
        // load the dom into memory
        QDomDocument document;
        document.setContent(device.readAll());

        QDomElement root = document.documentElement();
        ORIGIN_ASSERT(root.tagName() == SETTINGS_ELEMENT_NAME);

        // add the individual elements to the settings
        // TBD: we really need to strengthen the error handling
        for (QDomNode n = root.firstChild(); !n.isNull(); n = n.nextSibling())
        {
            QDomElement e = n.toElement(); // try to convert the node to an element.
            if (!e.isNull())
            {
                QString key = e.attribute("key");
                if(key.length() != 0)
                {
                    QString type = e.attribute("type");
                    QString value = e.attribute("value");

                    switch (type.toInt())
                    {
                        case Variant::Int:
                            map.insert(key, value.toInt()); break;

                        case Variant::String:
                            map.insert(key, value); break;

                        case Variant::Bool:
                            map.insert(key, value == "true"); break;

                        case Variant::ULongLong:
                            map.insert(key, value.toULongLong()); break;

                        default:
                            ORIGIN_ASSERT(false);
                            break;
                    }
                }
            }
        }

        // TBD: we really need to strengthen the error handling
        return true;
    }

    bool writeXmlFile(QIODevice &device, const QSettings::SettingsMap &map)
    {
        // build the dom
        QDomDocument document;
        QDomProcessingInstruction header = document.createProcessingInstruction( "xml", "version=\"1.0\"" );
        document.appendChild( header );
        QDomElement root = document.createElement(SETTINGS_ELEMENT_NAME);
        document.appendChild(root);

        // add the individual elements
        foreach(QString key, map.keys())
        {
            QDomElement node = document.createElement(SETTING_ELEMENT_NAME);
            QVariant v = map.value(key);
            node.setAttribute("key", key);
            node.setAttribute("type", QString::number(v.type()));
            node.setAttribute("value", v.toString());
            root.appendChild(node);
        }

        // save the dom to the given file using a text stream
        QTextStream stream(&device);
        document.save(stream, 2);
        stream.flush();

        return true;
    }

    // local functions
#if defined(ORIGIN_PC)

    QString GetDirectory(int csidl_id)
    {
        QString dirPath;

        WCHAR buffer[MAX_PATH];
        SHGetFolderPathW(NULL, csidl_id, NULL, SHGFP_TYPE_CURRENT, buffer );
        dirPath = QString::fromWCharArray(buffer);

        return dirPath;
    }

#endif

    QByteArray encryptionKey()
    {
        static bool isInitialized = false;
        static QByteArray key;

        if (!isInitialized)
        {
            quint64 machineHash = PlatformService::machineHash();
            QByteArray keyData(QByteArray(reinterpret_cast<char*>(&machineHash), sizeof(machineHash)));
            key = CryptoService::padKeyFor(keyData, sCipher, sCipherMode);
            isInitialized = true;
        }

        return key;
    }
}
