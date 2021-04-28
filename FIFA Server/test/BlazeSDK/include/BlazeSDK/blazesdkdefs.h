/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SDKDEFS_H
#define BLAZE_SDKDEFS_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/* This file should be used to define constants, etc that are BlazeSDK specific
 */

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

// The name of the util component's config section that is used for BlazeSDK-specific configuration
#define BLAZESDK_CONFIG_SECTION "BlazeSDK"
#define IDENTITY_CONFIG_SECTION "IdentityParams"

//
// Config property names in the BlazeSDK config section
//

// Maximum number of milliseconds between ping requests to the server
#define BLAZESDK_CONFIG_KEY_PING_PERIOD "pingPeriod"

// Default number of milliseconds an RPC will wait before timing out.
#define BLAZESDK_CONFIG_KEY_DEFAULT_REQUEST_TIMEOUT "defaultRequestTimeout"

// Default number of milliseconds the connection will stay open without any traffic
#define BLAZESDK_CONFIG_KEY_DEFAULT_CONN_IDLE_TIMEOUT "connIdleTimeout"

// Used to enable or disable the auto reconnect functionality
#define BLAZESDK_CONFIG_KEY_AUTO_RECONNECT_ENABLED "autoReconnectEnabled"

// Maximum number of reconnect attempts to make
#define BLAZESDK_CONFIG_KEY_MAX_RECONNECT_ATTEMPTS "maxReconnectAttempts"

// Voip headset update rate, in milliseconds
#define BLAZESDK_CONFIG_KEY_VOIP_HEADSET_UPDATE_RATE "voipHeadsetUpdateRate"

// ClientUserMetrics rate, in milliseconds
#define BLAZESDK_CONFIG_KEY_CLIENT_METRICS2_UPDATE_RATE "clientUserMetricsUpdateRate"

// Voip STT Transcribe settings
#define BLAZESDK_CONFIG_KEY_VOIP_STT_PROFILE "voipSTTProfile"
#define BLAZESDK_CONFIG_KEY_VOIP_STT_URL "voipSTTUrl"
#define BLAZESDK_CONFIG_KEY_VOIP_STT_CREDENTIAL "voipSTTKey"

// Voip TTS narrate settings
#define BLAZESDK_CONFIG_KEY_VOIP_TTS_PROVIDER "voipTTSProvider"
#define BLAZESDK_CONFIG_KEY_VOIP_TTS_URL "voipTTSUrl"
#define BLAZESDK_CONFIG_KEY_VOIP_TTS_CREDENTIAL "voipTTSKey"

// ByteVault server connection parameters
#define BLAZESDK_CONFIG_KEY_BYTEVAULT_HOSTNAME "bytevaultHostname"
#define BLAZESDK_CONFIG_KEY_BYTEVAULT_PORT "bytevaultPort"
#define BLAZESDK_CONFIG_KEY_BYTEVAULT_SECURE "bytevaultSecure"

// Identity 2.0 parameters
#define BLAZESDK_CONFIG_KEY_IDENTITY_CONNECT_HOST "nucleusConnect"
#define BLAZESDK_CONFIG_KEY_IDENTITY_CONNECT_HOST_TRUSTED "nucleusConnectTrusted"
#define BLAZESDK_CONFIG_KEY_IDENTITY_PROXY_HOST "nucleusProxy"
#define BLAZESDK_CONFIG_KEY_IDENTITY_PORTAL_HOST "nucleusPortal"
#define BLAZESDK_CONFIG_KEY_IDENTITY_XBL_TOKEN_URN "xblTokenUrn"
#define BLAZESDK_CONFIG_KEY_IDENTITY_REDIRECT "redirect_uri"
#define BLAZESDK_CONFIG_KEY_IDENTITY_SERVER_CLIENT_ID "client_id"
#define BLAZESDK_CONFIG_KEY_IDENTITY_DISPLAY "display"
#define BLAZESDK_CONFIG_KEY_XBOXONE_STRING_VALIDATION_URI "xboxOneStringValidationUri"

// Association List
#define BLAZESDK_CONFIG_KEY_ASSOCIATIONLIST_SKIP_INITIAL_SET "associationListSkipInitialSet"

// UserManager
#define BLAZESDK_CONFIG_KEY_USERMANAGER_CACHED_USERS "userManagerMaxCachedUsers"
#define BLAZESDK_CONFIG_KEY_CACHED_USER_REFRESH_INTERVAL "cachedUserRefreshInterval"

// QoSManager
#define BLAZESDK_CONFIG_KEY_QOS_FIREWALL_TEST  "enableQosFirewallTest"
#define BLAZESDK_CONFIG_KEY_QOS_BANDWIDTH_TEST "enableQosBandwidthTest"

// GameManager
#define BLAZESDK_CONFIG_KEY_GAMEMANAGER_PREVENT_MULTI_GAME_MEMBERSHIP "gameManagerPreventMultipleGameMembership"

#endif // BLAZE_SDKDEFS_H

