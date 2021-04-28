///////////////////////////////////////////////////////////////////////////////
// EbisuTelemetryAPI.h
//
// Implements a Origin Client specific telemetry interface
//
// Owner: Origin Dev Support
// Copyright (c) Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef EBISUTELEMETRYAPI_H
#define EBISUTELEMETRYAPI_H



///////////////////////////////////////////////////////////////////////////////
//  INCLUDES
///////////////////////////////////////////////////////////////////////////////

#include <EASTL/string.h>
#include <EASTL/fixed_string.h>
#include "EASTL/map.h"

#include "TelemetryPluginAPI.h"
#include "GenericTelemetryElements.h"
#include "EbisuMetrics.h"

///////////////////////////////////////////////////////////////////////////////
//  FORWARD DECLARATIONS
///////////////////////////////////////////////////////////////////////////////



class GameSession;
class InstallSession;
class DownloadSession;
class TelemetryLoginSession;

template<class T> class SessionListHelper;

namespace NonQTOriginServices
{
    class SystemInformation;
}


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251) // C4251: 'EbisuTelemetryAPI::lock' : class 'EA::Thread::Futex' needs to have dll-interface to be used by clients of class 'EbisuTelemetryAPI'
#endif


///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////

#define TELEMETRYPOWERMODEDEFS\
    TELEMETRYPOWERMODEDEF(PowerSleep    /* OS is entering sleep mode */)\
    TELEMETRYPOWERMODEDEF(PowerWake     /* OS is entering wake mode  */)\
    TELEMETRYPOWERMODEDEF(PowerShutdown /* OS is shutting down       */)


///////////////////////////////////////////////////////////////////////////////
//  CLASS
///////////////////////////////////////////////////////////////////////////////

class TELEMETRY_PLUGIN_API EbisuTelemetryAPI 
{

public:
    EbisuTelemetryAPI();
    virtual ~EbisuTelemetryAPI();

private:
    //Disable copying
    EbisuTelemetryAPI( const EbisuTelemetryAPI&);
    EbisuTelemetryAPI& operator=(const EbisuTelemetryAPI &);

public:

    ///////////////////////////////////////////////////////////////////////////////
    //  Super Amazing Enum Section
    ///////////////////////////////////////////////////////////////////////////////



    enum PowerModeEnum
    {
#define TELEMETRYPOWERMODEDEF(e) e,
        TELEMETRYPOWERMODEDEFS
#undef TELEMETRYPOWERMODEDEF
    };

    enum ViewGameSourceEnum {
        Hovercard = 0,
        Details,
        Expansion
    };

    enum IGOEnableSetting // must match IGOEnableSettingStrings[]
    {
        IGODisabled,
        IGOEnableGamesAll, 
        IGOEnableGamesSupported
    };

    enum DefaultTabSetting // must match DefaultTabSettingStrings[]
    {
        MyOrigin,
        MyGames, 
        Store,
        Decide
    };

    enum LaunchSource // must match LaunchStrings[]
    {
        Launch_Unknown,
        Launch_Desktop,
        Launch_Client, 
        Launch_MPInvite,
        Launch_ITO,
        Launch_SDK,
        Launch_SoftwareID
    };

    enum SettingFixResult
    {
        FixFail = 0,
        FixSuccess = 1
    };

    //  STORE PAGES
    enum OnTheHouseEnum
    {
        OtHNoGames,            //  0 - User navigated from Empty shelf page
        OtHMyGames,            //  1 - User navigated from My Games page
    };

    enum EntitlementNoneEnum {
        ENTITLEMENTS_NONE_GOTO_STOREHOME,
        ENTITLEMENTS_NONE_GOTO_FREEGAMES
    };

    enum GUISettingsViewEnum
    {
        GUISettingsView_Application,   //  0 - Application settings
        GUISettingsView_Account,       //  1 - Account settings
    };

    enum PerformanceTimerEnum
    {
        Authentication,                 //  Time from login 'ok' to token retrieval.
        Bootstrap,                      //  Launch of app to load of OriginClient DLL
        ContentRetrieval,               //  How long to retrieve user's entitlement list.  Context = # of games
        LoginToFriendsList,     //  After successful login to friends list availability.  Context = # of friends
        LoginToMainPage,        //  After successful login to my games availability.  Context = # of games
        LoginToMainPageLoaded,  //  After successful login to my games rendered.  Context = none
        RTPGameLaunch_launching,        //  Total time from client launch to game launch during RTP
        RTPGameLaunch_running,          //  Total time from RTP request to game launch during RTP
        ShowLoginPage,                  //  Load of OriginClient DLL to showing login page.  Context = none
        LoginPageLoaded,                //  After successful login page loads
        NUM_PERF_TIMERS
    };


    // Set the current user data on login
    virtual void setPersona(uint64_t nucleusid, const char8_t *country, bool isUnderage ) = 0;

    virtual void logout() = 0;

    void SetBootstrapVersion(const char8_t *versionString);

    /// \brief sets telemetry endpoint returns true if the setting was made -it hasn't been overriden- otherwise false
    /// \brief This function is called from OriginApplication right after receiving the configuration values.
    void setTelemetryServer(const eastl::string8&);

    /// \brief sets the telemetry hooks blacklist
    /// \param string that contains the list of hooks
    void setHooksBlacklist(const eastl::string8&);

    /// \brief sets the telemetry countries blacklist
    /// \param string that contains the list of countries
    void setCountriesBlacklist(const eastl::string8&);
    
    /// \brief stores the alternate launcher product ids (source and target) for future telemetry.
    ///   Origin client calls this function when an alternate launcher is about to be used.
    void setAlternateSoftwareIDLaunch(const eastl::string8& source_product_id, const eastl::string8& target_product_id);

    /// \brief stores the SDK OriginStartGame product ids (source and target) for future telemetry.
    ///   Origin client calls this function when an alternate launcher is about to be used.
    void setSDKStartGameLaunch(const eastl::string8& source_product_id, const eastl::string8& target_product_id);


    virtual void setIsBOI( bool isBOI ) = 0;

    virtual void startSendingTelemetry() = 0;

    /// Set if we are sending non-critical telemtry or not.
    virtual void setUserSettingSendNonCriticalTelemetry(bool isSendNonCriticalTelemetry) = 0;
    /// Set if the logged in user in underage or not.
    virtual void setUnderage(bool8_t b) = 0;
    /// set the current users subscriber status
    virtual void setSubscriberStatus(OriginTelemetrySubscriberStatus val) = 0;

    //TODO can we get rid of this and just get the client to use utilities directly?
    uint64_t CalculateIdFromString8(const char8_t *str);
    uint64_t CalculateIdFromString16(const char16_t *str);

    /// \brief Add listener to telemetry events
    virtual void AddEventListener(TelemetryEventListener* listener) = 0;
    /// \brief Remove listener to telemetry events
    virtual void RemoveEventListener(TelemetryEventListener* listener) = 0;

    // Helper for updating DownloadSession members
    void UpdateDownloadSession(const TYPE_S8 product_id, TYPE_U64 bytes_downloaded, TYPE_U64 DL_bitrate_kbps);

#if DEBUG
    /// \brief Sends test telemetry to help verify 4K payloads functionality
    //TODO new max size is now 8k.  We could update this and test, but so far we have no need of anything near that size.
    void Test4kPayload();
#endif

    //  kMETRIC_GROUP_APP
    void Metric_APP_INSTALL(const TYPE_S8 client_version_number);
    void Metric_APP_UNINSTALL(const TYPE_S8 client_version_number);
    void Metric_APP_MAC_ESCALATION_DENIED();
    void Metric_APP_ESCALATION_ERROR(const TYPE_S8 escalationCmd, const TYPE_I32 errorType, const TYPE_I32 sysErrorCode, const TYPE_S8 escalationReason, const TYPE_S8 errorDescrip);
    void Metric_APP_ESCALATION_SUCCESS(const TYPE_S8 escalationCmd, const TYPE_S8 escalationReason);
    void Metric_APP_SESSION_ID(const TYPE_S8 sessionId);
    void Metric_APP_START();
    void Metric_APP_SETTINGS(bool isBeta, bool isElevatedUser, bool mWasStartedFromAutoRun, bool mIsFreeToPlay);
    void Metric_APP_CMD(const TYPE_S8 cmdLine);
    void Metric_APP_CONFIG(const TYPE_S8 configName, const TYPE_S8 configValue, bool isOverride);
    void Metric_APP_END();
    void Metric_APP_CONNECTION_LOST();
    void Metric_APP_PREMATURE_TERMINATION();

    void Metric_APP_POWER_MODE(PowerModeEnum mode);
    void Metric_APP_MOTD_SHOW(const TYPE_S8 url, const TYPE_S8 text);
    void Metric_APP_MOTD_CLOSE();
    void Metric_APP_DYNAMIC_URL_LOAD(const TYPE_S8 file, const TYPE_S8 revision, bool isOverride);
    void Metric_APP_ADDITIONAL_CLIENT_LOG(const TYPE_S8 fileName);

    //  kMETRIC_GROUP_USER
    void Metric_LOGIN_START(const TYPE_S8 type);
    void Metric_LOGIN_OK(bool mode, bool underage, const TYPE_S8 type, bool isHwOptOut);
    void Metric_LOGIN_FAIL(const TYPE_S8 svcd, const TYPE_S8 code, const TYPE_S8 type, const TYPE_I32 http = 0, const TYPE_S8 url = "",
    const TYPE_U32 qt = 0, const TYPE_U32 subtype = 0, const TYPE_S8 description = "", const TYPE_S8 qtErrorString = "");
    void Metric_LOGIN_COOKIE_LOAD(const TYPE_S8 name, const TYPE_S8 value, const TYPE_S8 expiration);
    void Metric_LOGIN_COOKIE_SAVE(const TYPE_S8 name, const TYPE_S8 value, const TYPE_S8 expiration, const TYPE_U32 ecode, const TYPE_U32 count);
    void Metric_LOGIN_OEM(const TYPE_S8 oem_type);
    void Metric_WEB_APPLICATION_CACHE_CLEAR();
    void Metric_WEB_APPLICATION_CACHE_DOWNLOAD();
    void Metric_LOGOUT(const TYPE_S8 reason);
    void Metric_NUX_START(const TYPE_S8 step, bool autoStart);
    void Metric_NUX_CANCEL(const TYPE_S8 step);
    void Metric_NUX_END(const TYPE_S8 step);
    void Metric_NUX_PROFILE(bool fName = false, bool lName = false, bool avatarWindow = false, bool avatarGallery = false, bool avatarUpload = false);
    void Metric_NUX_EMAILDUPLICATE();
    void Metric_PRIVACY_START();
    void Metric_PRIVACY_CANCEL();
    void Metric_PRIVACY_END();
    void Metric_LOGIN_UNKNOWN(const TYPE_S8 type);
    void Metric_NUX_OEM(const TYPE_S8 oem_type);
    void Metric_PASSWORD_SENT();
    void Metric_PASSWORD_RESET();
    void Metric_CAPTCHA_START();
    void Metric_CAPTCHA_FAIL();
    void Metric_LOGIN_FAVORITES(const TYPE_I32 favorite = 0);
    void Metric_LOGIN_HIDDEN(const TYPE_I32 hidden = 0);
    void Metric_AUTHENTICATION_LOST(const TYPE_I32 qNetworkError, const TYPE_I32 httpStatus, const TYPE_S8 url);

    //  kMETRIC_GROUP_SETTINGS
    void Metric_SETTINGS(bool autoLogin, bool autoPatch, bool autoStart, bool autoClientUpdate, bool isBetaOptIn, 
        bool cloudSaveEnabled, bool hardwareOptOut, DefaultTabSetting defaultTabSetting, IGOEnableSetting igoEnableSetting );
    void Metric_PRIVACY_SETTINGS();
    void Metric_EMAIL_VERIFICATION_STARTS_CLIENT();
    void Metric_EMAIL_VERIFICATION_DISMISSED();
    void Metric_EMAIL_VERIFICATION_STARTS_BANNER();

    void Metric_QUIETMODE_ENABLED(bool disablePromoManager, bool ignoreNonMandatoryGameUpdates, bool shutdownOriginOnGameFinished);
    void Metric_INVALID_SETTINGS_PATH_CHAR(const TYPE_S8 path);

    void Metric_SETTINGS_SYNC_FAILED(const TYPE_S8 settingsFile, const TYPE_S8 failReason,
    const TYPE_U32 readable, const TYPE_U32 writable, const TYPE_U32 readSysError, const TYPE_U32 writeSysError,
    const TYPE_S8 winFileAttrs, const TYPE_S8 permFlag);

    void Metric_SETTINGS_FILE_FIX_RESULT(const SettingFixResult result, const TYPE_U32 read, const TYPE_U32 write, const TYPE_U32 qtFixSuccess, const TYPE_S8 settingsFile);

    //  kMETRIC_GROUP_HARDWARE
    void Metric_HW_OS();
    void Metric_HW_CPU();
    void Metric_HW_VIDEO();
    void Metric_HW_WEBCAM();
    void Metric_HW_DISPLAY();
    void Metric_HW_RESOLUTION();
    void Metric_HW_MEM();
    void Metric_HW_HDD();
    void Metric_HW_ODD();
    void Metric_HW_MICROPHONE();

    //  kMETRIC_GROUP_INSTALL
    void Metric_GAME_INSTALL_START(const TYPE_S8 product_id, const TYPE_S8 download_type);
    void Metric_GAME_INSTALL_SUCCESS(const TYPE_S8 product_id, const TYPE_S8 download_type);
    void Metric_GAME_INSTALL_ERROR(const TYPE_S8 product_id, const TYPE_S8 download_type, TYPE_I32 error_type, TYPE_I32 error_code);

    void Metric_GAME_UNINSTALL_CLICKED(const TYPE_S8 product_id);
    void Metric_GAME_UNINSTALL_CANCEL(const TYPE_S8 product_id);
    void Metric_GAME_UNINSTALL_START(const TYPE_S8 product_id);

    void Metric_GAME_ZERO_LENGTH_UPDATE(const TYPE_S8 product_id);

    void Metric_ITE_CLIENT_RUNNING(bool running);
    void Metric_ITE_CLIENT_SUCCESS();
    void Metric_ITE_CLIENT_FAILED(const TYPE_S8 reason);
    void Metric_ITE_CLIENT_CANCELLED(const TYPE_S8 reason);

    //  kMETRIC_GROUP_GAME_LAUNCH
    void Metric_GAME_LAUNCH_CANCELLED(const TYPE_S8 product_id, TYPE_S8 reason);

    //  kMETRIC_GROUP_GAME_SESSION
    void Metric_GAME_SESSION_START(const TYPE_S8 product_id, const TYPE_S8 launch_type, const TYPE_S8 children_content, const TYPE_S8 game_version, bool nonOrigin, const TYPE_S8 last_played, const TYPE_S8 sub_first_start_date, const TYPE_S8 sub_instance_start_date, bool isSubscriptionEntitlement);
    void Metric_GAME_SESSION_END(const TYPE_S8 product_id, TYPE_I32 error_code, bool nonOrigin, const TYPE_S8 last_played, const TYPE_S8 sub_first_start_date, const TYPE_S8 sub_instance_start_date, bool isSubscriptionEntitlement);
    void Metric_GAME_SESSION_UNMONITORED(const TYPE_S8 product_id, const TYPE_S8 launch_type, const TYPE_S8 children_content, const TYPE_S8 game_version);
    void Metric_ALTLAUNCHER_SESSION_FAIL(const TYPE_S8 product_id, const TYPE_S8 launch_type, const TYPE_S8 reas, const TYPE_S8 valu);
    void Metric_GAME_ALTLAUNCHER_FAIL(const TYPE_S8 launcher_product_id, const TYPE_S8 target_product_id, const TYPE_S8 game_state);

    //  kMETRIC_GROUP_DOWNLOAD
    void Metric_DL_START(const TYPE_S8 environment, const TYPE_S8 product_id, const TYPE_S8 DL_start_status, const TYPE_S8 content_type, const TYPE_S8 package_type, const TYPE_S8 url,
    TYPE_U64 bytes_downloaded, const TYPE_S8 auto_patch, const TYPE_S8 target_path, bool symlink_detected, bool isNonDipUpgrade, bool isPreload);
    void Metric_DL_IMMEDIATE_ERROR(const TYPE_S8 product_id, const TYPE_S8 error_string1, const TYPE_S8 error_string2, const TYPE_S8 source_file, TYPE_U32 source_line);
    void Metric_DL_DATA_ERROR(const TYPE_S8 product_id, const TYPE_S8 error_category, const TYPE_S8 error_details, const TYPE_S8 source_file, TYPE_U32 source_line);
    void Metric_DL_POPULATE_ERROR(const TYPE_S8 product_id, const TYPE_S8 error_type, const TYPE_I32 error_code, const TYPE_S8 source_file, TYPE_U32 source_line);
    void Metric_DL_ERROR(const TYPE_S8 product_id, const TYPE_S8 dler_error_string, const TYPE_S8 dler_error_context, TYPE_I32 dler_error_code, TYPE_I32 sys_error_code, const TYPE_S8 source_file, TYPE_U32 source_line, bool isPreload);

    void Metric_DL_ERROR_PREALLOCATE(const TYPE_S8 product_id, const TYPE_S8 error_type, const TYPE_S8 error_path, TYPE_U32 error_code, TYPE_U32 file_size);
    void Metric_DL_ERRORBOX(const TYPE_S8 product_id, const TYPE_I32 error_code, TYPE_I32 error_type);
    void Metric_DL_ERROR_VENDOR_IP(const TYPE_S8 product_id, const TYPE_S8 vendor_ip, TYPE_I32 dler_error_code, TYPE_I32 sys_error_code);
    void Metric_DL_CANCEL(const TYPE_S8 product_id, bool isPreload);
    void Metric_DL_SUCCESS(const TYPE_S8 product_id, bool isPreload);
    void Metric_DL_REDOWNLOAD(const TYPE_S8 product_id, const TYPE_S8 filename, const TYPE_S8 redl_error_code, TYPE_I32 sys_error_code, TYPE_U32 extra1, TYPE_U32 extra2);
    void Metric_DL_CRC_FAILURE(const TYPE_S8 product_id, const TYPE_S8 filename, TYPE_U32 unpack_type, TYPE_U32 file_crc, TYPE_U32 calculated_crc, TYPE_U32 file_date, TYPE_U32 file_time);
    void Metric_DL_PAUSE(const TYPE_S8 product_id, const TYPE_S8 reason);
    void Metric_DL_CONTENT_LENGTH(const TYPE_S8 product_id, const TYPE_S8 link, const TYPE_S8 type, const TYPE_S8 result, const TYPE_S8 startEndbyte, const TYPE_S8 totalbytes, const TYPE_S8 vendor_ip);
    void Metric_DL_OPTI_DATA(const TYPE_S8 product_id, const TYPE_S8 final_dl_rate, const TYPE_S8 avg_ip_dl_rate, const TYPE_S8 worker_saturation, const TYPE_S8 channel_saturation, const TYPE_U32 avg_chunk_bytes, const TYPE_S8 host, const TYPE_I16 num_ips_used, const TYPE_I16 num_ip_errors, const TYPE_U32 num_request_in_clusters, const TYPE_U32 num_single_file_chunks);
    void Metric_DL_PROXY_DETECTED();
    void Metric_DL_CONNECTION_STATS(const TYPE_S8 product_id, const TYPE_S8 uuid, const TYPE_S8 dest_ip, const TYPE_S8 url, TYPE_U64 bytes_downloaded, TYPE_U64 time_elapsed_ms, TYPE_U64 times_used, TYPE_U16 error_count);
    void Metric_DL_DIP3_PREPARE(const TYPE_S8 product_id, const TYPE_U32 filesToPatch);
    void Metric_DL_DIP3_SUMMARY(const TYPE_S8 product_id, const TYPE_U32 totalFiles, const TYPE_U32 filesToPatch, const TYPE_U32 filesRejected, const TYPE_U64 addedDataSz, const TYPE_U64 origUpdateSz, const TYPE_U64 dip3TotPatchSz, const TYPE_U64 dip3NonPatchSz, const TYPE_U64 dip3PatchedSz, const TYPE_U64 dip3PatchSavingsSz, const TYPE_U64 diffCalcBitrate);
    void Metric_DL_DIP3_CANCELED(const TYPE_S8 product_id, const TYPE_U32 reason_type, const TYPE_U32 reason_code);
    void Metric_DL_DIP3_ERROR(const TYPE_S8 product_id, const TYPE_U32 error_type, const TYPE_U32 error_code, const TYPE_S8 error_context);
    void Metric_DL_DIP3_SUCCESS(const TYPE_S8 product_id);

    void Metric_DL_ELAPSEDTIME_TO_READYTOPLAY(const TYPE_S8 product_id, TYPE_U64 timeElapsed, bool isITO, bool isLocalSource, bool isUpdate, bool isRepair);

    void Metric_DYNAMICDOWNLOAD_GAMELAUNCH(const TYPE_S8 product_id, const TYPE_S8 uuid, const TYPE_S8 launchSource, const TYPE_U64 bytesDownloaded, const TYPE_U64 bytesRequired, const TYPE_U64 bytesTotal);

    //  kMETRIC_GROUP_ERROR
    void Metric_ERROR_CRASH(const TYPE_S8 error_categoryid, const TYPE_S8 user_report_preference, bool onShutdown);

    //  kMetric_GROUP_BUGS
    void Metric_BUG_REPORT(const TYPE_S8 sessionid, const TYPE_S8 error_categoryid);
    void Metric_ERROR_DELETE_FILE(const TYPE_S8 context, const TYPE_S8 path, const TYPE_I32 error_code);
    void Metric_ERROR_NOTIFY(const TYPE_S8 error_reason, const TYPE_S8 error_code);
    void Metric_ERROR_REST(const TYPE_I32 restError, const TYPE_I32 qnetworkReply, const TYPE_I32 httpStatus, const TYPE_S8 url, const TYPE_S8 qErrorString);


    //  kMETRIC_GROUP_SOCIAL
    // Chat session start hook. 0 indicates that the local user is the sender. 
    void Metric_CHAT_SESSION_START(bool isInIgo, const uint64_t sendersNucleusId);
    void Metric_CHAT_SESSION_END(bool isInIgo);
    void Metric_FRIEND_VIEWSOURCE(const TYPE_S8 source);
    void Metric_FRIEND_COUNT(int count);
    void Metric_FRIEND_IMPORT();
    void Metric_FRIEND_INVITE_ACCEPTED(bool isInIgo);
    void Metric_FRIEND_INVITE_IGNORED(bool isInIgo);
    void Metric_FRIEND_INVITE_SENT(const TYPE_S8 source, bool isInIgo);
    void Metric_FRIEND_BLOCK_ADD_SENT();
    void Metric_FRIEND_BLOCK_REMOVE_SENT();
    void Metric_FRIEND_REPORT_SENT();
    void Metric_FRIEND_REMOVAL_SENT();
    void Metric_FRIEND_USERNAME_MISSING(int count);
    void Metric_GAME_INVITE_ACCEPTED(const TYPE_S8 offerId);
    void Metric_GAME_INVITE_DECLINE_SENT();
    void Metric_GAME_INVITE_DECLINE_RECEIVED();
    void Metric_GAME_INVITE_SENT(const TYPE_S8 offerId);
    void Metric_LOGIN_INVISIBLE(bool invisible);
    void Metric_USER_PROFILE_VIEW(const TYPE_S8 scope, const TYPE_S8 source, const TYPE_S8 view);
    void Metric_USER_PROFILE_VIEWGAMESOURCE(const TYPE_S8 source);
    void Metric_USER_PRESENCE_SET(const TYPE_S8 presence);
    void Metric_GC_CREATE_BORN_MUC_ROOM();
    void Metric_GC_RECEIVE_MUC_INVITE();
    void Metric_GC_DECLINING_MUC_INVITE(const TYPE_S8 reason);
    void Metric_GC_ACCEPTING_MUC_INVITE();
    void Metric_GC_ACCEPTING_CR_INVITE();
    void Metric_GC_MUC_UPGRADE_INITIATING(bool lastThreadEmpty);
    void Metric_GC_MUC_UPGRADE_AUTO_ACCEPT();
    void Metric_GC_MUC_EXIT(TYPE_I32 durationSeconds, TYPE_I32 messagesSent, TYPE_I32 maximumParticipants);
    void Metric_VC_CHANNEL_ERROR(const TYPE_S8 type, const TYPE_S8 errorMessage, const TYPE_I32 errorCode);

    // 9.5-style Chatroom telemetry
    void Metric_CHATROOM_ENTER_ROOM_FAIL(const TYPE_S8 xmppNode, const TYPE_S8 roomId, const TYPE_I32 errorCode);
    void Metric_CHATROOM_CREATE_GROUP(const TYPE_S8 groupId, const TYPE_I32 count);
    void Metric_CHATROOM_LEAVE_GROUP(const TYPE_S8 groupId, const TYPE_I32 count);
    void Metric_CHATROOM_DELETE_GROUP(const TYPE_S8 groupId, const TYPE_I32 count);
    void Metric_CHATROOM_DELETE_GROUP_FAILED(const TYPE_S8 groupId, const TYPE_I32 count);
    void Metric_CHATROOM_DEACTIVATED(const TYPE_S8 groupId, const TYPE_S8 roomId, const TYPE_I32 deactivationType, const TYPE_S8 by);
    void Metric_CHATROOM_GROUP_WAS_DELETED(const TYPE_S8 groupId, const TYPE_I32 count);
    void Metric_CHATROOM_KICKED_FROM_GROUP(const TYPE_S8 groupId, const TYPE_I32 count);
    void Metric_CHATROOM_GROUP_COUNT(const TYPE_I32 groupId);
    void Metric_CHATROOM_ENTER_ROOM(const TYPE_S8 roomId, const TYPE_S8 groupId, bool isInIgo);
    void Metric_CHATROOM_EXIT_ROOM(const TYPE_S8 roomId, const TYPE_S8 groupId, bool isInIgo);
    void Metric_CHATROOM_INVITE_SENT(const TYPE_S8 groupId, const TYPE_S8 invitee);
    void Metric_CHATROOM_INVITE_RECEIVED(const TYPE_S8 groupId);
    void Metric_CHATROOM_INVITE_ACCEPTED(const TYPE_S8 groupId, const TYPE_I32 count);
    void Metric_CHATROOM_INVITE_DECLINED(const TYPE_S8 groupId);

    //  kMETRIC_GROUP_IGO
    void Metric_IGO_BROWSER_START();
    void Metric_IGO_BROWSER_END();
    void Metric_IGO_BROWSER_ADDTAB();
    void Metric_IGO_BROWSER_CLOSETAB();
    void Metric_IGO_BROWSER_PAGELOAD();
    void Metric_IGO_BROWSER_URLSHORTCUT(const TYPE_S8 shortcut_id);
    void Metric_IGO_BROWSER_MAPP(const TYPE_S8 product_id);
    void Metric_IGO_OVERLAY_START();
    void Metric_IGO_OVERLAY_END();
    void Metric_IGO_OVERLAY_3RDPARTY_DLL(bool host, bool game, const TYPE_S8 name);
    void Metric_IGO_INJECTION_FAIL(const TYPE_S8 productId, bool elevated, bool freetrial);
    void Metric_IGO_HOOKING_BEGIN(const TYPE_S8 productId, TYPE_I64 timestamp, TYPE_U32 pid, bool showHardwareSpecs);
    void Metric_IGO_HOOKING_INFO(const TYPE_S8 productId, TYPE_I64 timestamp, TYPE_U32 pid, const TYPE_S8 context, const TYPE_S8 renderer, const TYPE_S8 message);
    void Metric_IGO_HOOKING_FAIL(const TYPE_S8 productId, TYPE_I64 timestamp, TYPE_U32 pid, const TYPE_S8 context, const TYPE_S8 renderer, const TYPE_S8 message);
    void Metric_IGO_HOOKING_END(const TYPE_S8 productId, TYPE_I64 timestamp, TYPE_U32 pid);

    void Metric_STORE_NAVIGATE_URL(const TYPE_S8 url);
    void Metric_STORE_LOAD_STATUS(const TYPE_S8 url, const TYPE_U32 durationInMs, const TYPE_U32 success);
    void Metric_STORE_NAVIGATE_OTH(OnTheHouseEnum type);

    //  HOME PAGES
    void Metric_HOME_NAVIGATE_URL(const TYPE_S8 url);

    //  PATCH
    void Metric_AUTOPATCH_DOWNLOAD_SKIP(const TYPE_S8 product_id);
    void Metric_AUTOPATCH_DOWNLOAD_START(const TYPE_S8 product_id, const TYPE_S8 version_installed, const TYPE_S8 version_server);
    void Metric_MANUALPATCH_DOWNLOAD_START(const TYPE_S8 product_id, const TYPE_S8 version_installed, const TYPE_S8 version_server);
    void Metric_AUTOPATCH_CONFIGVERSION_DIPVERSION_DONTMATCH(const TYPE_S8 entitl, const TYPE_S8 installedVersion, const TYPE_S8 serverVersion);

    // AUTO-REPAIR
    void Metric_AUTOREPAIR_TASK_ACCEPTED();
    void Metric_AUTOREPAIR_TASK_DECLINED();
    void Metric_AUTOREPAIR_NEEDS_REPAIR(const TYPE_S8 product_id, const TYPE_S8 version_installed, const TYPE_S8 version_server);
    void Metric_AUTOREPAIR_DOWNLOAD_START(const TYPE_S8 product_id, const TYPE_S8 version_installed, const TYPE_S8 version_server);

    //  SELF PATCH (aka Repair)
    void Metric_SELFPATCH_FOUND(const TYPE_S8 new_version, TYPE_U16 required, TYPE_U16 emergency);
    void Metric_SELFPATCH_DECLINED(const TYPE_S8 new_version, TYPE_U16 required, TYPE_U16 emergency);
    void Metric_SELFPATCH_DL_START(const TYPE_S8 new_version, TYPE_U16 required, TYPE_U16 emergency);
    void Metric_SELFPATCH_DL_FINISHED(const TYPE_S8 new_version, TYPE_U16 required, TYPE_U16 emergency);
    void Metric_SELFPATCH_LAUNCHED(const TYPE_S8 new_version, TYPE_U16 required, TYPE_U16 emergency);
    void Metric_SELFPATCH_OFFLINE_MODE(const TYPE_S8 new_version, TYPE_U16 required, TYPE_U16 emergency);
    void Metric_SELFPATCH_SIGNATURE_VALIDATION_ERROR(const TYPE_S8 new_version, TYPE_U16 required, TYPE_U16 emergency);
    void Metric_SELFPATCH_SIGNATURE_INVALID_HASH(const TYPE_S8 new_version, TYPE_U16 required, TYPE_U16 emergency);
    void Metric_SELFPATCH_SIGNATURE_UNTRUSTED_SIGNER(const TYPE_S8 new_version, TYPE_U16 required, TYPE_U16 emergency);

    //  NETWORK
    void Metric_NETWORK_SSL_ERROR(const TYPE_S8 url, TYPE_U32 code, const TYPE_S8 cert_sha1, const TYPE_S8 cert_name, const TYPE_S8 issuer_name);

    //  ACTIVATION
    void Metric_ACTIVATION_CODE_REDEEM_SUCCESS();
    void Metric_ACTIVATION_CODE_REDEEM_ERROR(TYPE_I32 errorCode);
    void Metric_ACTIVATION_REDEEM_PAGE_REQUEST();
    void Metric_ACTIVATION_REDEEM_PAGE_SUCCESS();
    void Metric_ACTIVATION_REDEEM_PAGE_ERROR(TYPE_I32 errorCode, TYPE_I32 httpCode);
    void Metric_ACTIVATION_FAQ_PAGE_REQUEST();
    void Metric_ACTIVATION_WINDOW_CLOSE(const TYPE_S8 eventString);

    //  REPAIR INSTALL
    void Metric_REPAIRINSTALL_FILES_STATS(const TYPE_S8 product_id, TYPE_U32 replaced, TYPE_U32 total_files);
    void Metric_REPAIRINSTALL_REQUESTED(const TYPE_S8 product_id);
    void Metric_REPAIRINSTALL_VERIFYCANCELED(const TYPE_S8 product_id);
    void Metric_REPAIRINSTALL_REPLACECANCELED(const TYPE_S8 product_id);
    void Metric_REPAIRINSTALL_REPLACINGFILES(const TYPE_S8 product_id);
    void Metric_REPAIRINSTALL_VERIFYFILESCOMPLETED(const TYPE_S8 product_id);
    void Metric_REPAIRINSTALL_REPLACINGFILESCOMPLETED(const TYPE_S8 product_id);

    //  CLOUD SYNC
    void Metric_CLOUD_WARNING_SPACE_CLOUD_LOW(const TYPE_S8 product_id, TYPE_U32 space);
    void Metric_CLOUD_WARNING_SPACE_CLOUD_MAX_CAPACITY(const TYPE_S8 product_id, TYPE_U32);
    void Metric_CLOUD_WARNING_SYNC_CONFLICT(const TYPE_S8 product_id);
    void Metric_CLOUD_ACTION_OBJECT_DELETION(const TYPE_S8 product_id, TYPE_U32 num_obj_deleted);
    void Metric_CLOUD_ACTION_GAME_NEW_CONTENT_DOWNLOADED(const TYPE_S8 product_id);
    void Metric_CLOUD_MANUAL_RECOVERY(const TYPE_S8 product_id);
    void Metric_CLOUD_AUTOMATIC_SAVE_SUCCESS(const TYPE_S8 product_id);
    void Metric_CLOUD_AUTOMATIC_RECOVERY_SUCCESS(const TYPE_S8 product_id);
    void Metric_CLOUD_ERROR_SYNC_LOCKING_UNSUCCESFUL(const TYPE_S8 product_id);
    void Metric_CLOUD_ERROR_PUSH_FAILED(const TYPE_S8 product_id);
    void Metric_CLOUD_ERROR_PULL_FAILED(const TYPE_S8 product_id);
    void Metric_CLOUD_PUSH_MONITORING(const TYPE_S8 product_id, TYPE_U32 numFiles, TYPE_U32 totalUploadSize);
    void Metric_CLOUD_MIGRATION_SUCCESS(const TYPE_S8 product_id);

    // Subscription 
    void Metric_SUBSCRIPTION_INFO(const TYPE_I32 isTrial, const TYPE_S8 subscription_id, const TYPE_S8 machine_id);

    // OFM-8399: SUBSCRIPTION user interaction events
    void Metric_SUBSCRIPTION_ENT_UPGRADED(const TYPE_S8 ownedOfferId, const TYPE_S8 subscriptionOfferId, const TYPE_S8 subscriptionElapsedTime, const bool successful, const TYPE_S8 context);
    void Metric_SUBSCRIPTION_ENT_REMOVE_START(const TYPE_S8 subscriptionOfferId, const TYPE_S8 subscriptionElapsedTime, const bool successful, const TYPE_S8 context);
    void Metric_SUBSCRIPTION_ENT_REMOVED(const TYPE_S8 subscriptionOfferId, const bool successful, const TYPE_S8 context);
    void Metric_SUBSCRIPTION_UPSELL(const TYPE_S8 context, const bool isSubscriptionActive);
    void Metric_SUBSCRIPTION_FAQ_PAGE_REQUEST(const TYPE_S8 context);
    void Metric_SUBSCRIPTION_USER_GOES_ONLINE(const TYPE_S8 lastKnownGoodTime, const TYPE_S8 subscriptionEndTime);


    //  MEMORY FOOTPRINT
    void Metric_METRIC_FOOTPRINT_INFO_STORE(TYPE_U32 unloadmemory_data, TYPE_U32 reloadmemory_data);

    //  SINGLE LOGIN
    void Metric_SINGLE_LOGIN_SECOND_USER_LOGGING_IN(const TYPE_S8 loginType);
    void Metric_SINGLE_LOGIN_FIRST_USER_LOGGING_OUT();
    void Metric_SINGLE_LOGIN_GO_OFFLINE_ACTION();
    void Metric_SINGLE_LOGIN_GO_ONLINE_ACTION();

    // SSO
    void Metric_SSO_FLOW_START(const TYPE_S8 authCode, const TYPE_S8 authToken, const TYPE_S8 loginType, const TYPE_U32 userOnline);
    void Metric_SSO_FLOW_RESULT(const TYPE_S8 result, const TYPE_S8 action, const TYPE_S8 loginType);
    void Metric_SSO_FLOW_ERROR(const TYPE_S8 reason, const TYPE_S8 restString, const TYPE_I32 rest, const TYPE_U32 http, const TYPE_S8 offerIds);
    void Metric_SSO_FLOW_INFO(const TYPE_S8 info);

    //  3PDD INSTALL
    void Metric_3PDD_INSTALL_DIALOG_SHOW();
    void Metric_3PDD_INSTALL_DIALOG_CANCEL();
    void Metric_3PDD_INSTALL_TYPE(const TYPE_S8 installType);
    void Metric_3PDD_PLAY_DIALOG_SHOW();
    void Metric_3PDD_PLAY_DIALOG_CANCEL();
    void Metric_3PDD_PLAY_TYPE(const TYPE_S8 playType);
    void Metric_3PDD_INSTALL_COPYCDKEY();
    void Metric_3PDD_PLAY_COPYCDKEY();

    //  FREE TRIALS
    void Metric_FREETRIAL_PURCHASE(const TYPE_S8 product_id, const TYPE_S8 expired, const TYPE_S8 source);

    //  CONTENT SALES
    void Metric_CONTENTSALES_PURCHASE(const TYPE_S8 id, const TYPE_S8 productType, const TYPE_S8 context);
    void Metric_CONTENTSALES_VIEW_IN_STORE(const TYPE_S8 id, const TYPE_S8 productType, const TYPE_S8 context);

    //  HELP
    void Metric_HELP_ORIGIN_HELP();
    void Metric_HELP_WHATS_NEW();
    void Metric_HELP_ORIGIN_FORUM();
    void Metric_HELP_ORIGIN_ER();

    // OER
    void Metric_OER_LAUNCHED(bool fromClient);
    void Metric_OER_ERROR();
    void Metric_OER_REPORT_SENT(const TYPE_S8 reportId);

    //  GUI
    void Metric_GUI_MYGAMESDETAILSPAGEVIEW(const TYPE_S8 product_id);
    void Metric_GUI_MYGAMES_CLOUD_STORAGE_TAB_VIEW(const TYPE_S8 product_id);
    void Metric_GUI_MYGAMES_MANUAL_LINK_CLICK(const TYPE_S8 product_id);
    void Metric_GUI_MYGAMEVIEWSTATETOGGLE(int viewstate);
    void Metric_GUI_MYGAMEVIEWSTATEEXIT(int viewstate);
    void Metric_GUI_TAB_VIEW(const TYPE_S8 newtab, const TYPE_S8 init);
    void Metric_GUI_SETTINGS_VIEW(GUISettingsViewEnum type);
    void Metric_GUI_MYGAMES_PLAY_SOURCE(const TYPE_S8 product_id, const TYPE_S8 source);
    void Metric_GUI_DETAILS_UPDATE_CLICKED(const TYPE_S8 product_id);
    void Metric_GUI_MYGAMES_HTTP_ERROR(const TYPE_I32 qnetworkerror, const TYPE_I32 httpstatus, const TYPE_I32 cachecontrol, const TYPE_S8 url);


    //  PERFORMANCE
    void Metric_PERFORMANCE_TIMER_START(PerformanceTimerEnum area, uint64_t startTime = 0);
    void Metric_PERFORMANCE_TIMER_END(PerformanceTimerEnum area, TYPE_U32 extraInfo = 0, TYPE_U32 extraInfo2 = 0);
    void Metric_PERFORMANCE_TIMER_STOP(PerformanceTimerEnum area);
    void Metric_PERFORMANCE_TIMER_CLEAR(PerformanceTimerEnum area);
    void Metric_ACTIVE_TIMER_START();
    void Metric_ACTIVE_TIMER_END(bool closed);
    void RUNNING_TIMER_START(uint64_t startTime = 0);
    void RUNNING_TIMER_CLEAR();

    //  GREY MARKET
    void Metric_GREYMARKET_LANGUAGE_SELECTION(const TYPE_S8 offerID, const TYPE_S8 langSelection, const TYPE_S8 langList, const TYPE_S8 manifestLangList);
    void Metric_GREYMARKET_LANGUAGE_SELECTION_BYPASS(const TYPE_S8 offerID, const TYPE_S8 langSelection, const TYPE_S8 langList, const TYPE_S8 manifestLangList);
    void Metric_GREYMARKET_BYPASS_FILTER(const TYPE_S8 offerID, const TYPE_S8 langList, const TYPE_S8 manifestLangList);
    void Metric_GREYMARKET_LANGUAGE_SELECTION_ERROR(const TYPE_S8 offerID, const TYPE_S8 availableLanguages, const TYPE_S8 manifestLanguages);
    void Metric_GREYMARKET_LANGUAGE_LIST(const TYPE_S8 offerID, const TYPE_S8 availableLanguages);

    //  DEVELOPER MODE
    void Metric_DEVMODE_ACTIVATE_SUCCESS(const TYPE_S8 tool_version, const TYPE_S8 platform);
    void Metric_DEVMODE_ACTIVATE_ERROR(const TYPE_S8 tool_version, const TYPE_S8 platform, const TYPE_U32 error_code);
    void Metric_DEVMODE_DEACTIVATE_SUCCESS(const TYPE_S8 tool_version, const TYPE_S8 platform);
    void Metric_DEVMODE_DEACTIVATE_ERROR(const TYPE_S8 tool_version, const TYPE_S8 platform, const TYPE_U32 error_code);

    //  NET PROMOTER SCORE
    void Metric_NET_PROMOTER_SCORE(const TYPE_S8 score, const TYPE_S8 feedback, const TYPE_S8 locale, bool canContact);
    void Metric_NET_PROMOTER_GAME_SCORE(const TYPE_S8 offerId, const TYPE_S8 score, const TYPE_S8 feedback, const TYPE_S8 locale, bool canContact);

    //  JUMPLIST
    void Metric_JUMPLIST_ACTION(const TYPE_S8 type, const TYPE_S8 source, const TYPE_S8 extra);

    //  BROADCAST (Twitch)
    void Metric_BROADCAST_STREAM_START(const TYPE_S8 product_id, const TYPE_S8 channelid, const TYPE_U64 streamid, const TYPE_U16 fromSDK, const TYPE_S8 settings);
    void Metric_BROADCAST_STREAM_STOP(const TYPE_S8 product_id, const TYPE_S8 channelid, const TYPE_U64 streamid, const TYPE_U32 minViewers, const TYPE_U32 maxViewers, const TYPE_U32 duration);
    void Metric_BROADCAST_ACCOUNT_LINKED(const TYPE_U16 fromSDK);
    void Metric_BROADCAST_STREAM_ERROR(const TYPE_S8 product_id, const TYPE_S8 error_reason, const TYPE_S8 error_code, const TYPE_S8 settings);
    void Metric_BROADCAST_JOINED(const TYPE_S8 source);

    //  ACHIEVEMENTS
    void Metric_ACHIEVEMENT_ACH_POST_SUCCESS(const TYPE_S8 product_id, const TYPE_S8 achievement_set_id, const TYPE_S8 achievementId);
    void Metric_ACHIEVEMENT_ACH_POST_FAIL(const TYPE_S8 product_id, const TYPE_S8 achievement_set_id, const TYPE_S8 achievementId, const TYPE_I32 code, const TYPE_S8 reason);
    void Metric_ACHIEVEMENT_WC_POST_FAIL(const TYPE_S8 product_id, const TYPE_S8 achievement_set_id, const TYPE_S8 wincode, const TYPE_I32 code, const TYPE_S8 reason);
    void Metric_ACHIEVEMENT_WIDGET_PAGE_VIEW(const TYPE_S8 page_name, const TYPE_S8 product_id, bool in_igo); 
    void Metric_ACHIEVEMENT_WIDGET_PURCHASE_CLICK(const TYPE_S8 product_id);
    void Metric_ACHIEVEMENT_ACHIEVED(TYPE_I32 achieved, TYPE_I32 xp, TYPE_I32 rp);
    void Metric_ACHIEVEMENT_ACHIEVED_PER_GAME(const TYPE_S8 productId, const TYPE_S8 achievementSetId, TYPE_I32 achieved, TYPE_I32 xp, TYPE_I32 rp);
    void Metric_ACHIEVEMENT_GRANT_DETECTED(const TYPE_S8 productId, const TYPE_S8 achievementSetId, const TYPE_S8 achievementId);
    void Metric_ACHIEVEMENT_SHARE_ACHIEVEMENT_SHOW(const TYPE_S8 source);
    void Metric_ACHIEVEMENT_SHARE_ACHIEVEMENT_DISMISSED(const TYPE_S8 source);
    void Metric_ACHIEVEMENT_COMPARISON_VIEW(const TYPE_S8 product_id, bool user_owns_product);

    // Dirty Bits
    void Metric_DIRTYBITS_NETWORK_CONNECTED();
    void Metric_DIRTYBITS_NETWORK_DISCONNECTED(const TYPE_S8 reason);
    void Metric_DIRTYBITS_MESSAGE_COUNTER(const TYPE_S8 totl, const TYPE_S8 emal, const TYPE_S8 orid, const TYPE_S8 pass, const TYPE_S8 dblc, const TYPE_S8 achi, const TYPE_S8 avat, const TYPE_S8 entl, const TYPE_S8 catl, const TYPE_S8 unkn, const TYPE_S8 dupl);
    void Metric_DIRTYBITS_RETRIES_CAPPED(const TYPE_S8 tout);
    void Metric_DIRTYBITS_NETWORK_SUDDEN_DISCONNECTION(const TYPE_I64 tcon);

    //  GAME PROPERTIES
    void Metric_GAMEPROPERTIES_APPLY_CHANGES(bool is_nog, const TYPE_S8 product_id, const TYPE_S8 install_path, const TYPE_S8 command_args, bool disable_igo);
    void Metric_GAMEPROPERTIES_LANG_CHANGES(const TYPE_S8 product_id, const TYPE_S8 locale);
    void Metric_CUSTOMIZE_BOXART_START(bool isNog);
    void Metric_CUSTOMIZE_BOXART_APPLY(bool isNog);
    void Metric_CUSTOMIZE_BOXART_CANCEL(bool isNog);
    void Metric_GAMEPROPERTIES_UPDATE_COUNT_LOGOUT(TYPE_I32 update_count);
    void Metric_GAMEPROPERTIES_ADDON_DESCR_EXPANDED(const TYPE_S8 addon_id);
    void Metric_GAMEPROPERTIES_ADDON_BUY_CLICK(const TYPE_S8 addon_id);
	void Metric_GAMEPROPERTIES_AUTO_DOWNLOAD_EXTRA_CONTENT(const TYPE_S8 masterTitleId, const bool& enabled, const TYPE_S8 context);

    //  ORIGIN DEVELOPER TOOL (ODT)
    void Metric_ODT_LAUNCH();
    void Metric_ODT_APPLY_CHANGES();
    void Metric_ODT_OVERRIDE_MODIFIED(const TYPE_S8 section, const TYPE_S8 entry, const TYPE_S8 value);
    void Metric_ODT_BUTTON_PRESSED(const TYPE_S8 button);
    void Metric_ODT_NAVIGATE_URL(const TYPE_S8 url);
    void Metric_ODT_RESTART_CLIENT();
    void Metric_ODT_TELEMETRY_VIEWER_OPENED();
    void Metric_ODT_TELEMETRY_VIEWER_CLOSED();

    //  LOCAL HOST
    void Metric_LOCALHOST_AUTH_SSO_START(const TYPE_S8 authType, const TYPE_S8 uuid, const TYPE_S8 origin);
    void Metric_LOCALHOST_SERVER_START(const TYPE_U32 port, const TYPE_U32 success);
    void Metric_LOCALHOST_SERVER_STOP();
    void Metric_LOCALHOST_SERVER_DISABLED();
    void Metric_LOCALHOST_SECURITY_FAILURE(const TYPE_S8 reason);

    // HOVERCARD
    void Metric_HOVERCARD_BUYNOW_CLICK(const TYPE_S8 gameid);
    void Metric_HOVERCARD_BUY_CLICK(const TYPE_S8 gameid);
    void Metric_HOVERCARD_DOWNLOAD_CLICK(const TYPE_S8 gameid);
    void Metric_HOVERCARD_DETAILS_CLICK(const TYPE_S8 gameid);
    void Metric_HOVERCARD_ACHIEVEMENTS_CLICK(const TYPE_S8 gameid);
    void Metric_HOVERCARD_UPDATE_CLICK(const TYPE_S8 gameid);

    //  PINNED IGO WIDGETS
    void Metric_PINNED_WIDGETS_STATE_CHANGED(const TYPE_S8 product_id, const TYPE_U16 widget_id, const TYPE_U16 pinnedState, const TYPE_U16 fromSDK);
    void Metric_PINNED_WIDGETS_HOTKEY_TOGGLED(const TYPE_S8 product_id, const TYPE_U16 pinnedState, const TYPE_U16 fromSDK);
    void Metric_PINNED_WIDGETS_COUNT(const TYPE_S8 product_id, const TYPE_U16 pinnedWindowsCount, const TYPE_U16 fromSDK);

    //  PLUGINS
    void Metric_PLUGIN_LOAD_SUCCESS(const TYPE_S8 product_id, const TYPE_S8 plugin_version, const TYPE_S8 compatible_client_version);
    void Metric_PLUGIN_LOAD_FAILED(const TYPE_S8 product_id, const TYPE_S8 plugin_version, const TYPE_S8 compatible_client_version, const TYPE_I32 error_code, const TYPE_I32 os_error_code = -1);
    void Metric_PLUGIN_OPERATION(const TYPE_S8 product_id, const TYPE_S8 plugin_version, const TYPE_S8 compatible_client_version, const TYPE_S8 operation, const TYPE_I32 exit_code);

    //  DOWNLOAD QUEUE
    void Metric_QUEUE_ENTITLMENT_COUNT(const TYPE_I32 count);
    void Metric_QUEUE_WINDOW_OPENED(const TYPE_S8 context);
    void Metric_QUEUE_ORDER_CHANGED(const TYPE_S8 old_product_id, const TYPE_S8 new_product_id, const TYPE_S8 context);
    void Metric_QUEUE_ITEM_REMOVED(const TYPE_S8 product_id, const TYPE_S8 wasHead, const TYPE_S8 context);
    void Metric_QUEUE_CALLOUT_SHOWN(const TYPE_S8 product_id, const TYPE_S8 context);
    void Metric_QUEUE_HEAD_BUSY_WARNING_SHOWN(const TYPE_S8 head_product_id, const TYPE_S8 head_operation, const TYPE_S8 ent_product_id, const TYPE_S8 ent_operation);
    void Metric_QUEUE_GUI_HTTP_ERROR(const TYPE_I32 qnetworkerror, const TYPE_I32 httpstatus, const TYPE_I32 cachecontrol, const TYPE_S8 url);

    // FEATURE CALLOUT
    void Metric_FEATURE_CALLOUT_DISMISSED(const TYPE_S8 seriesName, const TYPE_S8 objectName, const bool result);
    void Metric_FEATURE_CALLOUT_SERIES_COMPLETE(const TYPE_S8 seriesName, const bool result);

    //  NON-ORIGIN GAMES
    void Metric_NON_ORIGIN_GAME_ID_LOGIN(const TYPE_S8 product_id);
    void Metric_NON_ORIGIN_GAME_ID_LOGOUT(const TYPE_S8 product_id);
    void Metric_NON_ORIGIN_GAME_COUNT_LOGIN(int total);
    void Metric_NON_ORIGIN_GAME_COUNT_LOGOUT(int total);
    void Metric_NON_ORIGIN_GAME_ADD(const TYPE_S8 product_id);

    // SECURITY QUESTION
    void Metric_SECURITY_QUESTION_SHOW();
    void Metric_SECURITY_QUESTION_SET();
    void Metric_SECURITY_QUESTION_CANCEL();

    //  ENTITLEMENTS
    void Metric_ENTITLEMENTS_NONE_GOTO_STORE(EntitlementNoneEnum e);
    void Metric_ENTITLEMENT_GAME_COUNT_LOGIN(int games, int nogs);
    void Metric_ENTITLEMENT_GAME_COUNT_LOGOUT(int total, int games, int favs, int hidden, int nogs);
    void Metric_ENTITLEMENT_HIDE(TYPE_S8 product_id);
    void Metric_ENTITLEMENT_UNHIDE(TYPE_S8 product_id);
    void Metric_ENTITLEMENT_FAVORITE(TYPE_S8 product_id);
    void Metric_ENTITLEMENT_UNFAVORITE(TYPE_S8 product_id);
    void Metric_ENTITLEMENT_RELOAD(const TYPE_S8 product_id);
    void Metric_ENTITLEMENT_FREE_TRIAL_ERROR(const TYPE_I32 restError, const TYPE_I32 httpStatus);
    void Metric_ENTITLEMENT_DIRTYBIT_NOTIFICATION();
    void Metric_ENTITLEMENT_DIRTYBIT_REFRESH();
    void Metric_ENTITLEMENT_CATALOG_UPDATE_DBIT_PARSE_ERROR();
    void Metric_ENTITLEMENT_CATALOG_OFFER_UPDATE_DETECTED(const TYPE_S8 offerId, const TYPE_U64 catalogBatchTime);
    void Metric_ENTITLEMENT_CATALOG_MASTERTITLE_UPDATE_DETECTED(const TYPE_S8 masterTitleId, const TYPE_U64 catalogBatchTime);
    void Metric_ENTITLEMENT_REFRESH_COMPLETED(const TYPE_U32 serverResponseTime, const TYPE_U32 entitlementCount, const TYPE_I32 entitlementDelta); 
    void Metric_ENTITLEMENT_REFRESH_ERROR(const TYPE_I32 restError, const TYPE_I32 httpStatus, const TYPE_I32 clientErrorCode);
    void Metric_ENTITLEMENT_EXTRACONTENT_REFRESH_COMPLETED(const TYPE_S8 masterTitleId, const TYPE_U32 serverResponseTime, const TYPE_U32 entitlementCount); 
    void Metric_ENTITLEMENT_EXTRACONTENT_REFRESH_ERROR(const TYPE_S8 masterTitleId, const TYPE_I32 restError, const TYPE_I32 httpStatus, const TYPE_I32 clientErrorCode);
    void Metric_ENTITLEMENT_REFRESH_REQUEST_REFUSED(const TYPE_U32 requestDelta, const TYPE_U32 clientRefused);
    void Metric_ENTITLEMENT_OFFER_VERSION_UPDATE_CHECK(const TYPE_S8 offerId, const TYPE_S8 installedVersion);
    void Metric_ENTITLEMENT_OFFER_VERSION_UPDATE_CHECK_COMPLETED(const TYPE_S8 offerId, const TYPE_S8 installedVersion, const TYPE_S8 catalogVersion);
    void Metric_ENTITLEMENT_OFFER_VERSION_UPDATE_CHECK_TIMEOUT(const TYPE_S8 offerId, const TYPE_S8 installedVersion);

    // SONAR
    void Metric_SONAR_MESSAGE(const TYPE_S8 category, const TYPE_S8 message);
    void Metric_SONAR_STAT(const TYPE_S8 category, const TYPE_S8 name, const TYPE_U32 value);
    void Metric_SONAR_ERROR(const TYPE_S8 category, const TYPE_S8 message, const TYPE_U32 error);
    
    void Metric_SONAR_CLIENT_CONNECTED(
        const TYPE_S8 channelId,
        const TYPE_S8 inputDeviceId,
        const TYPE_I8 inputDeviceAGC,
        const TYPE_I16 inputDeviceGain,
        const TYPE_I16 inputDeviceThreshold,
        const TYPE_S8 outputDeviceId,
        const TYPE_I16 outputDeviceGain,
        const TYPE_U32 voiceServerIP,
        const TYPE_S8 captureMode);
    
    void Metric_SONAR_CLIENT_DISCONNECTED(
        const TYPE_S8 channelId,
        const TYPE_S8 reason,
        const TYPE_S8 reasonDesc,
        const TYPE_U32 messagesSent,
        const TYPE_U32 messagesReceived,
        const TYPE_U32 messagesLost,
        const TYPE_U32 messagesOutOfSequence,
        const TYPE_U32 badCid,
        const TYPE_U32 messagesTruncated,
        const TYPE_U32 audioFramesClippedInMixer,
        const TYPE_U32 socketSendError,
        const TYPE_U32 socketReceiveError,
        const TYPE_U32 receiveToPlaybackLatencyMean,
        const TYPE_U32 receiveToPlaybackLatencyMax,
        const TYPE_U32 playbackToPlaybackLatencyMean,
        const TYPE_U32 playbackToPlaybackLatencyMax,
        const TYPE_I32 jitterArrivalMean,
        const TYPE_I32 jitterArrivalMax,
        const TYPE_I32 jitterPlaybackMean,
        const TYPE_I32 jitterPlaybackMax,
        const TYPE_U32 playbackUnderrun,
        const TYPE_U32 playbackOverflow,
        const TYPE_U32 playbackDeviceLost,
        const TYPE_U32 dropNotConnected,
        const TYPE_U32 dropReadServerFrameCounter,
        const TYPE_U32 dropReadTakeNumber,
        const TYPE_U32 dropTakeStopped,
        const TYPE_U32 dropReadCid,
        const TYPE_U32 dropNullPayload,
        const TYPE_U32 dropReadClientFrameCounter,
        const TYPE_U32 jitterbufferUnderrun,
        const TYPE_U32 networkLoss,
        const TYPE_U32 networkJitter,
        const TYPE_U32 networkPing,
        const TYPE_U32 networkReceived,
        const TYPE_U32 networkQuality,
        const TYPE_U32 voiceServerIP,
        const TYPE_S8  captureMode,
        const TYPE_I32 jitterbufferOccupancyMean,
        const TYPE_I32 jitterbufferOccupancyMax
        );

    void Metric_SONAR_CLIENT_STATS(
        const TYPE_S8 channelId,
        // Avg, max, min latency(RTT)
        const TYPE_U32 networkLatencyMin,
        const TYPE_U32 networkLatencyMax,
        const TYPE_U32 networkLatencyMean,
        // bandwidth(bytes received and bytes sent)
        const TYPE_U32 bytesSent,
        const TYPE_U32 bytesReceived,
        // packet loss
        const TYPE_U32 messagesLost,
        // Out of order messages received
        const TYPE_U32 messagesReceivedOutOfSequence,
        // jitter(between successively received packets)
        const TYPE_U32 messagesReceivedJitterMin,
        const TYPE_U32 messagesReceivedJitterMax,
        const TYPE_U32 messagesReceivedJitterMean,
        const TYPE_U32 cpuLoad
        );

    //  CATALOG DEFINITIONS
    void Metric_CATALOG_DEFINTION_LOOKUP_ERROR(const TYPE_S8 product_id, const TYPE_I32 qt_network_code, const TYPE_I32 http_code, const bool public_offer);
    void Metric_CATALOG_DEFINTION_PARSE_ERROR(const TYPE_S8 product_id, const TYPE_S8 context, const bool from_cache, const bool public_offer, const TYPE_I32 code = -1);
    void Metric_CATALOG_DEFINTION_CDN_LOOKUP_STATS(const TYPE_U32 successCount, const TYPE_U32 failureCount, const TYPE_U32 confidentialCount, const TYPE_U32 unchangedCount);
    void Metric_ORIGIN_SDK_VERSION_CONNECTED(TYPE_S8 product_id, TYPE_S8 sdk_version);
    void Metric_ORIGIN_SDK_UNFILTERED_ENTITLEMENTS_REQUEST(TYPE_S8 product_id, TYPE_S8 sdk_version);


    void Metric_WEBWIDGET_DL_START(TYPE_S8 etag);
    void Metric_WEBWIDGET_DL_SUCCESS(TYPE_S8 etag, TYPE_U32 duration);
    void Metric_WEBWIDGET_DL_ERROR(TYPE_S8 etag, TYPE_I32 error_code);
    void Metric_WEBWIDGET_LOADED(TYPE_S8 widget_name, TYPE_S8 current_version);

    //Promo Manager
    void Metric_PROMOMANAGER_START(const TYPE_S8 offer_id, const TYPE_S8 primary_context, const TYPE_S8 secondary_context);
    void Metric_PROMOMANAGER_END(const TYPE_S8 offer_id, const TYPE_S8 primary_context, const TYPE_S8 secondary_context);
    void Metric_PROMOMANAGER_MANUAL_OPEN(const TYPE_S8 offer_id, const TYPE_S8 primary_context, const TYPE_S8 secondary_context);
    void Metric_PROMOMANAGER_MANUAL_RESULT(const TYPE_U32 result, const TYPE_S8 offer_id, const TYPE_S8 primary_context, const TYPE_S8 secondary_context);
    void Metric_PROMOMANAGER_MANUAL_VIEW_IN_STORE(const TYPE_S8 offer_id, const TYPE_S8 primary_context, const TYPE_S8 secondary_context);
    void Metric_OTH_BANNER_LOADED();

    //  SETTINGS
    void SetIsFreeToPlayITO(bool b);
    void SetLocale(const char* pLocale);
    void SetUsingAutoStart();
    void SetGameLaunchSource(LaunchSource source);
    bool IsComputerSurfacePro() const;

    const char8_t* GetMacAddr() const;
    const eastl::string8 GetMacHash() const;

protected:
    void HandleTelemetryBreakpoint(const TelemetryDataFormatTypes *format, va_list vl);

    /// pointer to SiystemInformation instance.
    NonQTOriginServices::SystemInformation *m_sysInfo;


private:
    ///Stores a Telemetry Event call as TelemetryEvent Object and puts it in a buffer to be sent to the Telemetry server.
    virtual void CaptureTelemetryData(const TelemetryDataFormatTypes *format, ...) = 0;
    /// gets the telemetry sending setting from the current telemetry writer.
    virtual bool isSendingNonCriticalTelemetry() const = 0;
    virtual TELEMETRY_SEND_SETTING userSettingSendNonCriticalTelemetry() const = 0;
    /// gets the underage setting from the current telemetry writer.
    virtual TELEMETRY_UNDERAGE_SETTING isTelemetryUserUnderaged() const = 0; 

    void IgnoreTelemetryBreakpoint(int id);
    int GetTelemetryEnum( va_list vl );

    uint64_t m_start_time;
    uint64_t m_login_time;
    uint64_t m_active_time;
    uint64_t m_running_time;
    uint64_t m_total_active_time;
    bool8_t m_active_timer_started;
    bool8_t m_running_timer_started;

    typedef SessionListHelper<GameSession> GameSessionList;
    typedef SessionListHelper<DownloadSession> DownloadSessionList;
    typedef SessionListHelper<InstallSession> InstallSessionList;

    GameSessionList *gameSessionList;
    DownloadSessionList *downloadSessionList;
    InstallSessionList *installSessionList;
    TelemetryLoginSession *loginSession;
    // \brief to test whether the API has been started

    int m_start_non_origin_games;
    int m_start_games;

    eastl::pair<eastl::string8,eastl::string8>* m_alt_launcher_info = NULL; // source product id / target product id
    eastl::pair<eastl::string8, eastl::string8>* m_sdk_launching_info = NULL; // source product id / target product id

    typedef eastl::map<eastl::string, uint64_t> PromoStartMap;

    /// \brief Maps a promo and its context to the timestamp when promo dialog was displayed under that context.
    PromoStartMap m_promoMgrDialogTime;

    bool mIsFreeToPlayITO;
    bool mUsingAutoStart;
    LaunchSource mGameLaunchSource;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // EBISUTELEMETRYAPI_H
