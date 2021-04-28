 ---
 Blaze
 =====
 ---

Release Info
-------------

* Version: 15.1.1.10.1
* Date: January 27, 2021
* [Greenlight features]( https://eadpjira.ea.com/issues/?jql=status%20in%20(Completed)%20AND%20fixVersion%20%3D%20Urraca.1 )
* [Resolved Bugs ]( https://eadpjira.ea.com/issues/?jql=project%20%3D%20%22GOS%22%20and%20issuetype%20%3D%20%22Blaze%20Internal%20Bug%22%20and%20status%20%3D%20Closed%20and%20fixVersion%20%3D%20Urraca.1 )
* [2014 Spring --> 2015 Winter UPGRADE GUIDE]( https://developer.ea.com/display/blaze/Blaze+2015+Winter+1.x+Continuous+Feature+Upgrade+Guide )

Blaze Documentation
-------------------

* [Getting Started with Blaze]( https://developer.ea.com/display/blaze/Getting+Started )
* [Blaze Customer Portal]( https://developer.ea.com/display/blaze/Blaze+Customer+Portal )
* [Blaze Technical Designs]( https://developer.ea.com/display/TEAMS/Blaze )

Compatibility
-------------
This release's server has been tested with the following clients (in addition to this release's client):
* 15.1.1.9.6 (Trama.6)

Known Issues
------------
* About [GOSREQ-3535](https://eadpjira.ea.com/browse/GOSREQ-3535) (release notes are below): When blazeserver config tokenFormat is set to "TOKEN_FORMAT_JWT" in oauth.cfg, steam user login doesn't work normally that an incorrect value is used as steam user pid_id. For steam user, the pid_id in JWT access token is different from the value returned by Nucleus /tokeninfo API. This issue only happens when blazeserver is configured to use JWT format access token. Config tokenFormat is "TOKEN_FORMAT_OPAQUE" by default in oauth.cfg.

---
Needs Action
============
---
* As a result of [GOSREQ-3866](https://eadpjira.ea.com/browse/GOSREQ-3866) and [GOSREQ-3996](https://eadpjira.ea.com/browse/GOSREQ-3996), the dependency on EA Cloud's PIN Event Forwarder has been eliminated.  There is also a new config setting in titledefines.cfg called PIN_EA_GAME_ID that should be set to the 'EA Game ID' provided when doing your PIN onboarding.  See [Blaze PIN Integration](https://developer.ea.com/display/blaze/PIN+Integration) for more details.

General
------------
* [GOS-33153](https://eadpjira.ea.com/browse/GOS-33153) - Titles building NucleusSDK with the DirtySDK version this release was tested on (see masterconfig.xml), including those building Blaze's Ignition samples, should use at least version 20.4.1.0.0 of NucleusSDK, and include a preprocessor in their materconfig file's globaldefines DIRTYCODE_USE_NEW_CUSTOMHEADERCALLBACK=1, to avoid compilation issues. Reference this release's masterconfig.xml, and [GOS-33153](https://eadpjira.ea.com/browse/GOS-33153) for details.
* [GOS-32929](https://eadpjira.ea.com/browse/GOS-32929) - Authorization.cfg will not define GOS_EAC, GOS_EARS, GOS_IVC_SJC, GOS_BIOWARE, GOS_RELOADTOOL, ANIMALBOX Ip lists anymore. If these Ip Lists are needed, integrators will need to define them or use the GOS IpList from authorization.cfg.

UserSessionManager
------------
* [GOS-32895](https://eadpjira.ea.com/browse/GOS-32895) - For users with multiple same-platform 1st party accounts linked to their EA account, conccurent login attempts now boot the older user session offline. The new default more closely matches player expectations and also aligns with existing Blaze behavior for concurrent logins with the same persona. Setting forceMultiLinkedSessionOffline to 'false' in usersessions.cfg can revert to the Blaze Trama behavior, where the additional personas were blocked from authenticating while the original persona is still online.

---
 New Features
===============
---
Authentication
---------------
* [GOSREQ-3535](https://eadpjira.ea.com/browse/GOSREQ-3535) - Blazeserver now has the ability of decoding and verifying JWT format access token returned by C&I Nucleus service. Blazeserver can be configured to use JWT format access token and the login process would: (1) verify token is legit using Public Key provided by Nucleus; (2) decode token to get basic token info and skip one particular Nucleus API call. Changes of this GOSREQ only affect how server side handles token internally. This GOSREQ is NOT the support of login by sending JWT access token. By default, blazeserver uses OPAQUE format access token in oauth.cfg, not JWT. For more information about Blaze JWT support and its config, please visit [Blaze JWT Support](https://developer.ea.com/display/blaze/JWT+Support).

Framework
------------
* [GOSREQ-3294](https://eadpjira.ea.com/browse/GOSREQ-3294) - Server-to-server gRPC usage of the Blaze server via the [EADP Service Mesh](https://developer.ea.com/display/TEAMS/EADP+Service+Mesh+Standard+Proposal) is now available. For more details, see [Blaze In Service Mesh](https://developer.ea.com/display/blaze/EADP+Service+Mesh).
* [GOS-33113](https://eadpjira.ea.com/browse/GOS-33113) - At the request of EA Security, the $S "verbatim string" format specifier for the database Query class's append function has been deprecated via the BLAZE_ENABLE_DEPRECATED_VERBATIM_STRING_FORMAT_SPECIFIER_IN_DATABASE_QUERY define. This release comments out that define in the blazeserver/scripts/deprecations.xml file, thus disabling that format specifier. See more details [here](https://developer.ea.com/display/blaze/Urraca+Database+Query+Verbatim+Strings).
* [GOS-33261](https://eadpjira.ea.com/browse/GOS-33261) - The commondefines.cfg file now provides a NUCLEUS_ENV config definition.  For Blaze clusters running in the Cloud, this value will be provided automatically on the command line.  Outside the Cloud, if not overridden by boot files, it will be initialized based on the Blaze ENV definition (Blaze ENV of "prod" initializes NUCLEUS_ENV to "prod", all other Blaze ENV values initialize NUCLEUS_ENV to "int").  Integrators who set NUCLEUS_ENV to "lt" in any (on-prem) load test boot files can now take advantage of the common set of LT-related URL's that have newly been added to commondefines.cfg (where previously only int and prod definitions were provided).  Integrators are also free to use this definition in other configs for conditional configuration decisions that need to be based on Nucleus (as opposed to Blaze) environment.
* [GOSREQ-3866](https://eadpjira.ea.com/browse/GOSREQ-3866) - PIN events can now be sent to River's PIN API directly from Blaze, eliminating the dependency on the PIN Event Forwarder (Logstash), and enabling you to test custom PIN events while running on Windows.  See [Blaze PIN Integration](https://developer.ea.com/display/blaze/PIN+Integration) for more details.
* [GOSREQ-3996](https://eadpjira.ea.com/browse/GOSREQ-3996) - The PIN event data pipeline is now fully managed by the Blaze server.  Blaze will now read PIN event data from the 'ponevent' log files stored on disk, and deliver them directly to the River PIN API.  See [Blaze PIN Integration](https://developer.ea.com/display/blaze/PIN+Integration) for more details.

GameManager
------------
* [GOSREQ-3977](https://eadpjira.ea.com/browse/GOSREQ-3977) - Blaze has been updated to support Sony's Cross Gen SDK and APIs for PS4 and PS5, allowing titles' PS4 clients to view, invite and join into the same Blaze Games via the console shell UX as the title's PS5 clients, and vice versa. To enable this feature, configure your PS4 client title App to be PS4 crossgen enabled on PS5 DevNet, as described in [Sony's PS5 docs](https://p.siedev.net/resources/documents/SDK/2.000/PSN_Service_Setup-Guide/0003.html#__document_toc_00000008), install the latest crossgen enabled PS4 SDK package version (see the kettlesdk BlazeSDK's stock sample masterconfig.xml), build the BlazeSDK client with the '-D:package.kettlesdk.crossgen.enabled=true' on the NAnt Framework command line, and set the PSN_EXTERNAL_SESSION_XGEN_ENABLED "true" in your Blaze Server's titledefines.cfg. Reference [Blaze's PS5 Cross Gen docs](https://developer.ea.com/pages/viewpage.action?spaceKey=blaze&title=PS5+Cross+Gen), and [Sony's Cross Gen docs](https://p.siedev.net/resources/documents/SDK/2.000/Cross_Generation-Overview/0004.html) .
* [GOSREQ-3947](https://eadpjira.ea.com/browse/GOSREQ-3947) - Blaze now allows creating or resetting games without joining the caller as part of the player roster. To do this, ensure your client type has PERMISSION_OMIT_CALLER_FROM_ROSTER in your server's authorization.cfg, and specify the CreateGameRequest.gameCreationData.isExternalCaller flag to be true. When true, the request's caller will be omitted from the player roster. Reference [GameManager docs](https://developer.ea.com/display/blaze/Omit+Joining+Caller) for details.
* [GOSREQ-4066](https://eadpjira.ea.com/browse/GOSREQ-4066) - To support upcoming DirtySDK Connection Concierge concurrent connections feature, the 'connection set id' identifying the hosting server providing a game endpoint's hosted connectivity is now exposed via BlazeSDK's new GameManager::MeshEndpoint::getHostingServerConnSetId() method.
* [GOSREQ-3888](https://eadpjira.ea.com/browse/GOSREQ-3888) - For Russia legal compliance, added Player Settings Service integration to disable VoIP.  Blaze will not update this PSS setting (the responsibility is external to Blaze) and will only fetch the setting to determine whether or not to disable VoIP.  This is an additional check as VoIP can be disabled by other existing mechanisms, but this check takes precedence.

Matchmaker
------------
* [GOSREQ-3701](https://eadpjira.ea.com/browse/GOSREQ-3701) - added tot_players_matched & tot_players_potentially_matched members to mp_match_join events to provide additional telemetry for Blaze matchmaking.

UserSessionManager
-----------
* [GOS-32895](https://eadpjira.ea.com/browse/GOS-32895) - New configuration primaryPersonaUpdatableByWalAuthentication (defaulting to false) prevents WAL user sessions from being created for any persona but a user's current primary persona on a given platform. The primary persona is the most recent (per-platform) 1st party persona tied to an EA account that has authenticated with your Blaze cluster. Titles that wish the primary persona to be updatable via WAL sessions can set this flag to 'true.' In no case will a WAL session ever be permitted to force a standard persona offline, so conflicting 1:N logins are always resolved in favor of the console login, regardless of authentication order.

Inclusions/Dependencies
=======================
---


The default dependencies for packages and libraries used in this release of Blaze SDK are as follows:

Blaze SDK Dependencies
----------------------
```
ActivePython                    2.7.13.2716
AndroidAPI                      28
AndroidBuildTools               29.0.2
AndroidEmulator                 1.18.00
AndroidNDK                      15.2.4203891
AndroidPlatformTools            28
AndroidSDK                      100.00.00-pr1
AndroidTools                    26.1.1
antlr                           3.5.2
ApacheAnt                       1.9.7
BlazeSDKSamplesLibPack          1.0.0
CapilanoSDK                     180713-proxy
ConsoleRemoteServer             0.03.10
coreallocator                   1.04.02
DirtySDK                        15.1.7.0.1
DotNet                          4.72
DotNetSDK                       2.0-4
Doxygen                         1.8.14-1
EAAssert                        1.05.07
EABase                          2.09.13
EACallstack                     2.02.01
EAController                    1.09.01
EAControllerUserPairing         1.12.09
EACrypto                        1.03.04
EAIO                            3.01.11
EAJson                          1.06.05-maverick
EAMessage                       2.10.05
EAStdC                          1.26.07
EASTL                           3.16.01
EASystemEventMessageDispatcher  1.05.00
EATDF                           15.1.1.10.1
EAThread                        1.33.00
EATrace                         2.12.03
EAUser                          1.09.08
GDK                             200805-proxy
GradleWrapper                   1.00.00-pr1
GraphViz                        2.38.0
IncrediBuild                    3.61.1.1243
jdk                             azul-8.0.222-pc64
job_manager                     4.05.05
kettlesdk                       8.008.011-1-proxy
nantToVSTools                   3.19.09
NucleusSDK                      20.4.1.0.0
nx_config                       1.01.02
nx_vsi                          10.4.5.1-proxy
nxsdk                           10.4.1-proxy
opus                            1.00.06
OriginSDK                       10.6.2.13
PcLint                          9.00k-nonproxy
pclintconfig                    0.17.02
PinTaxonomySDK                  2.2.2
PlayStation4AppContentEx        8.008.011
PlayStation4AudioInEx           8.008.011
PPMalloc                        1.26.07
ProDG_VSI                       8.00.0.3-proxy
ps5sdk                          2.00.00.09-proxy
Ps5VSI                          2.00.0.6-proxy
PyroSDK                         15.1.1.9.3
rwmath                          3.01.01
Speex                           1.2rc1-8
StadiaSDK                       1.53-proxy
TelemetrySDK                    20.4.1.1.0
TypeDatabase                    1.04.02
UnixClang                       4.0.1
userdoc                         1.00.00
UTFXml                          3.08.09
vstomaketools                   2.06.09
WindowsSDK                      10.0.19041
XcodeProjectizer                3.07.02
zlib                            1.2.11
```

Blaze Server Dependencies
-------------------------
```
ActivePython            2.7.13.2716-blaze-1
antlr                   3.5.2
bison                   2.1
blaze_base              flattened
blazeserver_grpc_tools  15.1.1.10.1
coreallocator           1.04.02
DotNet                  4.72
EAAssert                1.05.07
EABase                  2.09.13
EACallstack             2.02.01
EAIO                    3.01.11
EAJson                  1.06.05-maverick
EARunner                1.09.04
EAStdC                  1.26.07
EASTL                   3.16.01
EATDF                   15.1.1.10.1
EATDF_Java              1.09.00
EAThread                1.33.00
flex                    2.5.4a
GamePacker              1.04.01
google_coredumper       container-proxy
google_perftools        container-proxy-1
gtest                   1.10.0.ea.6
hiredis                 0.14.0-custom-4
IncrediBuild            3.61.1.1243
jdk                     azul-8.0.222-pc64
libcurl                 7.62.0-3-ssh2
libopenssl              1.1.1d-prebuilt
libssh2                 1.8.0-3-pc64
mariadb                 3.0.6
nantToVSTools           3.19.09
PcLint                  9.00k-nonproxy
pclintconfig            0.17.02
PPMalloc                1.26.07
ptmalloc3               1.0-1-custom
UnixClang               9.0.0
UTFXml                  3.08.09
vstomaketools           2.06.09
WindowsSDK              10.0.19041-proxy
zlib                    1.2.11
```

These are the versions used in the available pre-compiled libs for Blaze SDK. If needed, the SDK can be built for older packages/libraries by modifying the masterconfig.xml file. Building using newer packages/libraries may not work depending on the changes in the newer versions; check with GOS if you are trying to build with newer versions and having problems.

# Blaze Release Info

* Version: 15.1.1.10.0
* Date: October 23, 2020
* [Greenlight features]( https://eadpjira.ea.com/issues/?jql=status%20in%20(Completed)%20AND%20fixVersion%20%3D%20Urraca.0 )
* [Resolved Bugs ]( https://eadpjira.ea.com/issues/?jql=project%20%3D%20%22GOS%22%20and%20issuetype%20%3D%20%22Blaze%20Internal%20Bug%22%20and%20status%20%3D%20Closed%20and%20fixVersion%20%3D%20Urraca.0 )
* [2014 Spring --> 2015 Winter UPGRADE GUIDE]( https://developer.ea.com/display/blaze/Blaze+2015+Winter+1.x+Continuous+Feature+Upgrade+Guide )
* [Urraca Upgrade Guide]( https://developer.ea.com/display/blaze/Urraca+Upgrade+Guide )

# Blaze Documentation

* [Getting Started with Blaze]( https://developer.ea.com/display/blaze/Getting+Started )
* [Blaze Customer Portal]( https://developer.ea.com/display/blaze/Blaze+Customer+Portal )
* [Blaze Technical Designs]( https://developer.ea.com/display/TEAMS/Blaze )

# Compatibility

### Clients
* 15.1.1.9.5 (Trama.5)

# Known Issues
* [EADPMPS-6187](https://eadpjira.ea.com/browse/EADPMPS-6187) - A build issue exists with NucleusSDK version 20.3.1.0.1, that will occur if using the new DirtySDK version (15.1.7.0.0) being released with Urraca.0. The issue is not in Blaze code, but would block building Blaze's Ignition samples, which use NucleusSDK. To complete our testing we fixed the compliation issues in the NucleusSDK code locally, as described in https://eadpjira.ea.com/browse/EADPMPS-6187. NucleusSDK team will be including these fixes in a later NucleusSDK version. Reference https://eadpjira.ea.com/browse/EADPMPS-6187 for details.

# Needs Action

### General
* [GOS-32375](https://eadpjira.ea.com/browse/GOS-32375) - Game Teams must obtain a custom address for STATSERVICEHOST from GS Support, and set the value in titledefines.cfg (for PROD).  The STATSERVICEHOST value is no longer hardcoded in PROD enviroments and must be obtained from GS Support in order to run the blazeserver in PROD.
* [GOS-32439](https://eadpjira.ea.com/browse/GOS-32439) - All blaze server service names over 48 characters need to be revised to stay under the updated max service name length. Doc: [Service Name Standard](https://developer.ea.com/display/blaze/Service+Name+Standard)
* [GOS-32907](https://eadpjira.ea.com/browse/GOS-32907) - The $S "verbatim string" format specifier for DB queries has been deprecated to increase protection against SQL Injection. See [Urraca Database Query Verbatim Strings](https://developer.ea.com/display/blaze/Urraca+Database+Query+Verbatim+Strings) for details.
* [GOS-33141](https://eadpjira.ea.com/browse/GOS-33141) - In titledefines.cfg: the blazeServerClientId has been moved out of the serviceNames map to a single blazeServerNucleusClientId value. Also, the releaseType value in serviceNames has been moved to a productNamesPINInfo map in usersessions.cfg that replaces the serviceNamesPINInfo config. If desired, titles can define the PIN_RELEASE_TYPE_DEFAULT value in their .boot files to override the PIN release type from commondefines.cfg
* [GOS-33145](https://eadpjira.ea.com/browse/GOS-33145) - Removed MyISAM usage from Blaze Stats component.  If you are connecting your blaze instance to MySQL DB, drop all of your lb_stats_% tables before your next restart.  Any needed lb_stats_% tables will be rebuilt (using InnoDB) on your next server start up.
* [GOSREQ-3757](https://eadpjira.ea.com/browse/GOSREQ-3757) - Blaze's S2S PS5 Match Activities integration has been updated to work with Blaze in the Cloud. As part of this, Blaze's PS5 Match Activities title onboarding procedures have also been updated/streamlined. Titles no longer setup an App Server Client Credential DevNet product, nor any vault/client-secret in oauth.cfg. Instead PS5 titles should send an email/ticket to Commerce & Identity (C&I), requesting to "setup a PS5 Matches Client-credential Service Label number for the title". Titles then set that number C&I provides as their Blaze Server titledefines.cfg's PS5_MATCHES_SERVICE_LABEL value. Reference Blaze's PS5 Matches docs for details.

### Authentication
* [GOS-32832](https://eadpjira.ea.com/browse/GOS-32832) - Users with more than one 1st-party account linked to their Nucleus account from the same platform will receive AUTH_ERR_DUPLICATE_LOGIN when trying to log in concurrently with their other personas. Titles should provide specific UI to end users in this situation so they can correct it by either logging their other persona out of Blaze, or delink their additional same-platform personas from their Nucleus account.

### GameManager
* [GOS-32213](https://eadpjira.ea.com/browse/GOS-32213), [GOS-32754](https://eadpjira.ea.com/browse/GOS-32754), [GOS-32767](https://eadpjira.ea.com/browse/GOS-32767), [GOS-32768](https://eadpjira.ea.com/browse/GOS-32768), [GOS-32769](https://eadpjira.ea.com/browse/GOS-32769) - enableNetworkAddressProtection in gamesession.cfg now defaults to 'true'. This configuration must be set to true for titles supporting cross-play. In dedicated server game sessions, player clients will only have access to the dedicated server's address. The dedicated server host will have full access to game member network addresses. In CCS_MODE_HOSTED_ONLY games, players will not get each other's network addresses. GameGroup members and NETWORK_DISABLED game session members will also not get network addresses for their fellow game members unless P2P VoIP is enabled either without CCS enabled or in CCS_MODE_HOSTED_FALLBACK mode.

### Matchmaker
* [GOS-32757](https://eadpjira.ea.com/browse/GOS-32757) - For performance, by default, Blaze's Matchmaking Diagnostics metrics are now disabled from being populated in getMatchmakingMetrics responses. To enable the Matchmaking Diagnostics, titles can specify trackDiagnostics = true within the matchmakerSettings section of Blaze Server's matchmaker_settings.cfg. Reference docs on Matchmaking Diagnostics here: https://developer.ea.com/display/blaze/Matchmaking+Diagnostics .

### UserSessionManager
* [GOS-32766](https://eadpjira.ea.com/browse/GOS-32766) - UserExtendedData will not provide a NetworkAddress for any non-local user updates, unless the recipient has PERMISSION_OTHER_NETWORK_ADDRESS. This permission is enabled by default for the DEDICATED_SERVER, and the TOOLS access groups, and disabled by default for end-user client AccessGroups. This permission does not impact transmission of NetworkAddresses to members of GameSessions as required for gameplay or VoIP connectivity. 
* [GOS-32379](https://eadpjira.ea.com/browse/GOS-32379) - (Minor)  When setting enableNetworkAddressProtection to true in gamesession.cfg, addresses are masked for peer-mesh & hosted topologies when CCS mode is set to CCS_MODE_HOSTEDONLY. Titles enabling networkAddressProtection and using CCS_MODE_HOSTEDONLY will no longer be able to access remote user IP addresses via Player object accessors in the SDK.
* [GOS-32367](https://eadpjira.ea.com/browse/GOS-32367) - the "dirtyCastGame" template has been moved from /etc/gamemanager/examples/create_game_template_examples.cfg to /etc/gamemanager/tools/create_game_template_dirtycast.cfg and included in /etc/gamemanager/create_game_templates.cfg seperately so titles can comment out example template includes without disabling the dirtycast game template.

# New Features

### Framework
* [GOS-32985](https://eadpjira.ea.com/browse/GOS-32985) - The [Leak Sanitizer](https://developer.ea.com/display/blaze/Using+Clang+Sanitizers+in+Blaze) can now be used independently from Address Sanitizer by using `-D:blazeserver.lsan_enabled` instead of `-D:blazeserver.asan_enabled`.
* [GOS-32301](https://eadpjira.ea.com/browse/GOS-32301) - (Minor)  Database configuration now only applies maxConnCount from the instanceOverrides map. No other parameter can be overriden by using instanceOverrides.
* [GOS-32386](https://eadpjira.ea.com/browse/GOS-32386) - Metrics now support an allowedlist for OI export, in addition to the existing blockedlist.  While the default blazeserver metrics are unchanged, any custom metrics should be added to the allowedlist in order to be exported. 
* [GOS-32764](https://eadpjira.ea.com/browse/GOS-32764) - (Minor) The localization file now supports the following escape characer sequences: '/t' '/r' '/n'.  These values will be replaced automatically, when the are read in.   For other special characters, simply enclose the string in double quotes.  (Ex. "String, with, commas")
* [GOS-33037](https://eadpjira.ea.com/browse/GOS-33037) - Gen 5 PINClientPlatform names are updated to "ps5"(from "balin") and "xbsx"(from "gemini"). The old deprecated platform names should no longer be used for any title/server in prod. 
* To support Blaze in the Cloud, blazeserver instances now obtain their instance ids from redis, rather than reading them from the commandline or boot config. As part of this change, the blazeserver RedisConfig was moved to blaze.boot.
* Support for redis sentinels has been removed, and the blazeserver now uses redis in cluster mode only. See [Blaze Deployment Scripts](https://developer.ea.com/display/blaze/Blaze+Deployment+Scripts) and [How to Use Redis in Cluster Mode](https://developer.ea.com/display/blaze/How+to+Use+Redis+in+Cluster+Mode) for more details about using redis in cluster mode for deployment and local blazeserver development.
* [GOS-32765](https://eadpjira.ea.com/browse/GOS-32765) - EATDF TdfStructVector::insert has been fixed to copy the inserted data type into a newly allocated struct, rather than simply recreating a reference to the original value.  Additionally, the range insert and multiple value insert functions that were available in TdfPrimitiveVector are now available in TdfStructVector as well.

### GameManager
* [GOS-32213](https://eadpjira.ea.com/browse/GOS-32213), [GOS-32754](https://eadpjira.ea.com/browse/GOS-32754) - CCS_MODE_HOSTED_FALLBACK can be used for cross-play game sessions. When game members are on different platforms, they will never attempt to connect directly to clients of a different platform. Additionally, player network addresses are not sent to users on a different platform. For the network addresses to be hidden, enableNetworkAddressProtection in gamesession.cfg must be set to 'true', which is the new default value.
* [GOS-32265](https://eadpjira.ea.com/browse/GOS-32265) - The XblBlockList can now be disabled at the Rule level, using the disableRule parameter.  This can be set via a Scneraio Rule. 
* [GOS-32297](https://eadpjira.ea.com/browse/GOS-32297) - Blaze's stock Ignition samples on Xbox One have been updated to use a new Xbox One sample title setup via Microsoft's new Partner Center site ('BlazeSDK Sample 2', title id 2056720033). The old 'BlazeSDK Sample' and title id, setup via MS's old XDP site, will only continue to be used with prior Blaze versions' samples going forward. Reference Blaze and Microsoft Partner Center and XDP Migration documentation for details.
* [GOS-32372](https://eadpjira.ea.com/browse/GOS-32372) - The privacy settings of PS4 games now automatically change to private when the Game is switched to disable join by player, browsing, and matchmaking.  This is done to allow invite only games on the PS4.
* [GOSREQ-3100](https://eadpjira.ea.com/browse/GOSREQ-3100) - A new set of Connection PIN events have been added.  These events add a large amount of addition connection data that can be used to diagnose issues in PIN.  The rate of data can be controlled by the following:  DisableDetailedConnectionStats - disables the metrics, FlushDetailedConnectionStatsOnExport - disables the metrics when a ZDT export occurs, ConnStatsMaxEntries - sets how many metric entries are included in each call. 
* [GOSREQ-3309](https://eadpjira.ea.com/browse/GOSREQ-3309) - Dedicated Server attributes can now be changed after the Game is created.  This can be done via a BlazeSDK call on the Dedicated Server itself, or by calling the GameManager setDedicatedServerAttributes RPC with the PERMISSION_BATCH_UPDATE_DEDICATED_SERVER_ATTRIBUTES permission.  The RPC call allows multiple servers to be updated simultaneously. 

### Matchmaker
* [GOS-32788](https://eadpjira.ea.com/browse/GOS-32788) - The AvoidPlayersRule has a new input parameter, mAvoidAccountList (avoidAccountList in scenarios.cfg's rule definition), that allows for Nucleus AccountIds to be used when trying to avoid specific players in matchmaking.
* [GOSREQ-3507](https://eadpjira.ea.com/browse/GOSREQ-3507) - Blaze Matchmaking re-architecture - Packer based functional feedback release. Initial implementation of the packer based matchmaking features for functional feedback from lighthouse partner.

### UserSessionManager
* [GOSREQ-3877](https://eadpjira.ea.com/browse/GOSREQ-3877) - Added configuration for 'nonCrossplayPlatformSets' to allow Blaze to treat groups of platforms as non-crossplay for purposes of users opted out of crossplay. Can be used to allow Gen4 & Gen5 versions of the same console family to play together without a crossplay opt-in or Origin and Steam PC users to play together without enabling crossplay.

# Inclusions/Dependencies

The default dependencies for packages and libraries used in this release of Blaze SDK are as follows:

Blaze SDK Dependencies
----------------------
```
ActivePython                    2.7.13.2716
AndroidAPI                      28
AndroidBuildTools               29.0.2
AndroidEmulator                 1.18.00
AndroidNDK                      15.2.4203891
AndroidPlatformTools            28
AndroidSDK                      100.00.00-pr1
AndroidTools                    26.1.1
antlr                           3.5.2
ApacheAnt                       1.9.7
BlazeSDKSamplesLibPack          1.0.0
CapilanoSDK                     180713-proxy
ConsoleRemoteServer             0.03.10
coreallocator                   1.04.02
DirtySDK                        15.1.7.0.0
DotNet                          4.0-2
DotNetSDK                       2.0-4
Doxygen                         1.8.14-1
EAAssert                        1.05.07
EABase                          2.09.13
EACallstack                     2.02.01
EAController                    1.09.01
EAControllerUserPairing         1.12.09
EACrypto                        1.03.04
EAIO                            3.01.11
EAJson                          1.06.05-maverick
EAMessage                       2.10.05
EAStdC                          1.26.07
EASTL                           3.16.01
EASystemEventMessageDispatcher  1.05.00
EATDF                           15.1.1.10.0
EAThread                        1.33.00
EATrace                         2.12.03
EAUser                          1.09.08
GDK                             200801-proxy
GradleWrapper                   1.00.00-pr1
GraphViz                        2.38.0
IncrediBuild                    3.61.1.1243
jdk                             azul-8.0.222-pc64
job_manager                     4.05.05
kettlesdk                       7.508.021-proxy
nantToVSTools                   3.19.09
NucleusSDK                      20.3.1.0.1-custom-1
nx_config                       1.01.02
nx_vsi                          10.4.5.1-proxy
nxsdk                           10.4.1-proxy
opus                            1.00.05
OriginSDK                       10.6.2.13
PcLint                          9.00k-nonproxy
pclintconfig                    0.17.02
PinTaxonomySDK                  2.2.2
PlayStation4AppContentEx        7.008.001
PlayStation4AudioInEx           7.008.001
PPMalloc                        1.26.07
ProDG_VSI                       7.50.0.5-proxy
ps5sdk                          1.00.00.50-proxy
Ps5VSI                          1.00.0.7-1-proxy
PyroSDK                         15.1.1.9.3
rwmath                          3.01.01
Speex                           1.2rc1-8
StadiaSDK                       1.50-proxy
TelemetrySDK                    20.4.1.1.0
TypeDatabase                    1.04.02
UnixClang                       4.0.1
userdoc                         1.00.00
UTFXml                          3.08.09
vstomaketools                   2.06.09
WindowsSDK                      10.0.19041
XcodeProjectizer                3.07.02
zlib                            1.2.11
```

Blaze Server Dependencies
-------------------------
```
```

These are the versions used in the available pre-compiled libs for Blaze SDK. If needed, the SDK can be built for older packages/libraries by modifying the masterconfig.xml file. Building using newer packages/libraries may not work depending on the changes in the newer versions; check with GOS if you are trying to build with newer versions and having problems.



# Blaze Release Info

* Version: 15.1.1.9.5
* Date: August 20, 2020
* [Greenlight features]( https://eadpjira.ea.com/issues/?jql=status%20in%20(Completed)%20AND%20fixVersion%20%3D%20Trama.5 )
* [Resolved Bugs ]( https://eadpjira.ea.com/issues/?jql=project%20%3D%20%22GOS%22%20and%20issuetype%20%3D%20%22Blaze%20Internal%20Bug%22%20and%20status%20%3D%20Closed%20and%20fixVersion%20%3D%20Trama.5 )
* [2014 Spring --> 2015 Winter UPGRADE GUIDE](https://developer.ea.com/display/blaze/Blaze+2015+Winter+1.x+Continuous+Feature+Upgrade+Guide)

# Blaze Documentation

* [Getting Started with Blaze](https://developer.ea.com/display/blaze/Getting+Started)
* [Blaze Customer Portal](https://developer.ea.com/display/blaze/Blaze+Customer+Portal)
* [Blaze Technical Designs](https://developer.ea.com/display/TEAMS/Blaze)

# Compatibility

### Clients
* 15.1.1.9.3 (Trama.3)
* 15.1.1.8.4 (Shrike.4)

# Known Issues
* [EADPMPS-6055](https://eadpjira.ea.com/browse/EADPMPS-6055) - A couple crash issues were found in the NucleusSDK version used in this release on XBSX. The issues are not in Blaze code, but may occur when using Blaze's Ignition samples, which use NucleusSDK. To complete our testing on XBSX, we fixed the issues in the NucleusSDK code locally. NucleusSDK team is planning to include the fixes in an upcoming NucleusSDK version (see https://eadpjira.ea.com/browse/EADPMPS-6055). To avoid the crashes, XBSX titles using NucleusSDK should ensure to upgrade to a NucleusSDK version/build including those fixes. Reference https://eadpjira.ea.com/browse/EADPMPS-6055 for details on the fixes.
* Gen 5 Crossplay: XBOX has been validated, but XBXS is not validated

# Needs Action
* None

# New Features

### Framework
* [GOS-33037](https://eadpjira.ea.com/browse/GOS-33037) - Gen 5 PINClientPlatform names are updated to "ps5"(from "balin") and "xbsx"(from "gemini"). The old deprecated platform names should no longer be used for any title/server in prod. 

# Inclusions/Dependencies

The default dependencies for packages and libraries used in this release of Blaze SDK are as follows:

Blaze SDK Dependencies
----------------------
```
ActivePython                    2.7.13.2716
AndroidAPI                      28
AndroidBuildTools               29.0.2
AndroidEmulator                 1.18.00
AndroidNDK                      15.2.4203891
AndroidPlatformTools            28
AndroidSDK                      100.00.00-pr1
AndroidTools                    26.1.1
antlr                           3.5.2
ApacheAnt                       1.9.7
BlazeSDKSamplesLibPack          1.0.0
CapilanoSDK                     180713-proxy
ConsoleRemoteServer             0.03.10
coreallocator                   1.04.02
DirtySDK                        15.1.6.0.5
DotNet                          4.0-2
DotNetSDK                       2.0-4
Doxygen                         1.8.14-1
EAAssert                        1.05.07
EABase                          2.09.13
EACallstack                     2.02.01
EAController                    1.09.01
EAControllerUserPairing         1.12.09
EACrypto                        1.03.04
EAIO                            3.01.11
EAJson                          1.06.05
EAMessage                       2.10.05
EAStdC                          1.26.07
EASTL                           3.16.01
EASystemEventMessageDispatcher  1.05.00
EATDF                           15.1.1.9.5
EAThread                        1.33.00
EATrace                         2.12.03
EAUser                          1.09.05
GDK                             200602-proxy
GradleWrapper                   1.00.00-pr1
GraphViz                        2.38.0
IncrediBuild                    3.61.1.1243
iphonesdk                       10.0-proxy-1
jdk                             azul-8.0.222-pc64
job_manager                     4.05.05
kettlesdk                       7.508.021-proxy
nantToVSTools                   3.19.09
NucleusSDK                      20.2.1.0.0-pr
nx_config                       1.00.05
nx_vsi                          9.0.6.27951
nxsdk                           9.0.0
opus                            1.00.05
OriginSDK                       10.6.2.13
PcLint                          9.00k-nonproxy
pclintconfig                    0.17.02
PinTaxonomySDK                  2.2.0
PlayStation4AppContentEx        7.008.001
PlayStation4AudioInEx           7.008.001
PPMalloc                        1.26.07
ProDG_VSI                       7.50.0.5-proxy
ps5sdk                          1.00.00.40-proxy
Ps5VSI                          1.00.0.7-1-proxy
PyroSDK                         15.1.1.9.3
rwmath                          3.01.01
Speex                           1.2rc1-8
StadiaSDK                       dev-proxy
TelemetrySDK                    19.3.1.0.0
TypeDatabase                    1.04.02
UnixClang                       4.0.1
userdoc                         1.00.00
UTFXml                          3.08.09
vstomaketools                   2.06.09
WindowsSDK                      10.0.19041
XcodeProjectizer                3.04.00
zlib                            1.2.11
```

Blaze Server Dependencies
-------------------------
```
ActivePython            2.7.13.2716-blaze-1
antlr                   3.5.2
bison                   2.1
blazeserver_grpc_tools  15.1.1.9.2
c_ares                  container-proxy-1
centos7_base            flattened
coreallocator           1.04.02
EAAssert                1.05.07
EABase                  2.09.13
EAIO                    2.23.01
EAJson                  1.06.05
EARunner                1.09.01
EAStdC                  1.23.00
EASTL                   3.12.01
EATDF                   15.1.1.9.5
EATDF_Java              1.09.00
EAThread                1.32.06
flex                    2.5.4a
GamePacker              1.03.04
google_coredumper       container-proxy
google_perftools        container-proxy
gtest                   1.8.0.ea.10
hiredis                 0.14.0-custom-4
IncrediBuild            3.61.1.1243
jdk                     azul-8.0.222-pc64
libcurl                 7.62.0-3-ssh2
libopenssl              1.1.1d-prebuilt
libssh2                 1.8.0-3-pc64
lua                     5.3.1.04
mariadb                 3.0.6
nantToVSTools           3.19.09
PcLint                  9.00k-nonproxy
pclintconfig            0.17.02
PPMalloc                1.26.00
ptmalloc3               1.0-1-custom
UnixClang               4.0.1
UTFXml                  3.08.09
vstomaketools           2.06.09
WindowsSDK              10.0.14393-proxy
zlib                    1.2.11
```

These are the versions used in the available pre-compiled libs for Blaze SDK. If needed, the SDK can be built for older packages/libraries by modifying the masterconfig.xml file. Building using newer packages/libraries may not work depending on the changes in the newer versions; check with GOS if you are trying to build with newer versions and having problems.



# Blaze Release Info

* Version: 15.1.1.9.4
* Date: June 3, 2020
* [Greenlight features]( https://eadpjira.ea.com/issues/?jql=status%20in%20(Completed)%20AND%20fixVersion%20%3D%20Trama.4 )
* [Resolved Bugs ]( https://eadpjira.ea.com/issues/?jql=project%20%3D%20%22GOS%22%20and%20issuetype%20%3D%20%22Blaze%20Internal%20Bug%22%20and%20status%20%3D%20Closed%20and%20fixVersion%20%3D%20Trama.4 )
* [2014 Spring --> 2015 Winter UPGRADE GUIDE]( https://developer.ea.com/display/blaze/Blaze+2015+Winter+1.x+Continuous+Feature+Upgrade+Guide )

# Blaze Documentation

* [Getting Started with Blaze]( https://developer.ea.com/display/blaze/Getting+Started )
* [Blaze Customer Portal]( https://developer.ea.com/display/blaze/Blaze+Customer+Portal )
* [Blaze Technical Designs]( https://developer.ea.com/display/TEAMS/Blaze )

# Compatibility

### Clients
* 15.1.1.9.3 (Trama.3)
* 15.1.1.8.4 (Shrike.4)

# Known Issues
* None

# Needs Action
* None

# New Features

### General
* Blaze now supports *steam* as a native platform. New [scopes](https://developer.ea.com/display/blaze/Nucleus+scopes+required+for+Blaze) are required to login to steam for the blazeserver's clientId. Build configuration for steam is same as PC but it is required to set the platform as steam. Check [ConnectionManager](https://developer.ea.com/display/blaze/ConnectionManager) guide to set the platform for steam. 
* Blaze now supports *stadia* as a native platform.

### GameManager
* [GOSREQ-3652](https://eadpjira.ea.com/browse/GOSREQ-3652) - Support for PS5 1st-party Match Team names UX integration has been added. By default Blaze now updates PS5 Match Team names based in the shell UX based off games' TeamIds and BlazeServer's new gamesession.cfg teamNameByTeamIdMap configuration setting. For details see Blaze docs on [TeamIds](https://developer.ea.com/display/blaze/Teams+and+Roles) and [Match Teams](https://developer.ea.com/display/blaze/PS5+Matches).

# Inclusions/Dependencies

The default dependencies for packages and libraries used in this release of Blaze SDK are as follows:

Blaze SDK Dependencies
----------------------
```
ActivePython                    2.7.13.2716
AndroidAPI                      28
AndroidBuildTools               29.0.2
AndroidEmulator                 1.18.00
AndroidNDK                      15.2.4203891
AndroidPlatformTools            28
AndroidSDK                      100.00.00-pr1
AndroidTools                    26.1.1
antlr                           3.5.2
ApacheAnt                       1.9.7
BlazeSDKSamplesLibPack          1.0.0
CapilanoSDK                     180712-1-proxy
ConsoleRemoteServer             0.03.10
coreallocator                   1.04.02
DirtySDK                        15.1.6.0.4
DotNet                          4.0-2
DotNetSDK                       2.0-4
Doxygen                         1.8.14-1
EAAssert                        1.05.07
EABase                          2.09.13
EACallstack                     2.02.01
EAController                    1.09.01
EAControllerUserPairing         1.12.05
EACrypto                        1.03.04
EAIO                            3.01.08
EAJson                          1.06.05
EAMessage                       2.10.05
EAStdC                          1.26.07
EASTL                           3.16.01
EASystemEventMessageDispatcher  1.05.00
EATDF                           15.1.1.9.4
EAThread                        1.33.00
EATrace                         2.12.03
EAUser                          1.09.02
GDK                             200200-proxy
GradleWrapper                   1.00.00-pr1
GraphViz                        2.38.0
IncrediBuild                    3.61.1.1243
iphonesdk                       10.0-proxy-1
jdk                             azul-8.0.222-pc64
job_manager                     4.05.05
kettlesdk                       7.508.001-proxy
nantToVSTools                   3.19.09
NucleusSDK                      20.2.1.0.0-pr
nx_config                       1.00.05
nx_vsi                          9.0.6.27951
nxsdk                           9.0.0
opus                            1.00.05
OriginSDK                       10.6.2.13
PcLint                          9.00k-nonproxy
pclintconfig                    0.17.02
PinTaxonomySDK                  2.2.0
PlayStation4AppContentEx        7.008.001
PlayStation4AudioInEx           7.008.001
PPMalloc                        1.26.07
ProDG_VSI                       7.50.0.5-proxy
ps5sdk                          0.95.00.36-proxy
Ps5VSI                          0.95.0.4-proxy
PyroSDK                         15.1.1.9.3
rwmath                          3.01.01
Speex                           1.2rc1-8
StadiaSDK                       dev-proxy
TelemetrySDK                    19.3.1.0.0
TypeDatabase                    1.04.02
UnixClang                       4.0.1
userdoc                         1.00.00
UTFXml                          3.08.09
vstomaketools                   2.06.09
WindowsSDK                      10.0.18362
XcodeProjectizer                3.04.00
zlib                            1.2.11
```

Blaze Server Dependencies
-------------------------
```
ActivePython            2.7.13.2716-blaze-1
antlr                   3.5.2
bison                   2.1
blazeserver_grpc_tools  15.1.1.9.2
c_ares                  container-proxy-1
centos7_base            flattened
coreallocator           1.04.02
EAAssert                1.05.07
EABase                  2.09.13
EAIO                    2.23.01
EAJson                  1.06.05
EARunner                1.09.01
EAStdC                  1.23.00
EASTL                   3.12.01
EATDF                   15.1.1.9.4
EATDF_Java              1.09.00
EAThread                1.32.06
flex                    2.5.4a
GamePacker              1.03.04
google_coredumper       container-proxy
google_perftools        container-proxy
gtest                   1.8.0.ea.10
hiredis                 0.14.0-custom-4
IncrediBuild            3.61.1.1243
jdk                     azul-8.0.222-pc64
libcurl                 7.62.0-3-ssh2
libopenssl              1.1.1d-prebuilt
libssh2                 1.8.0-3-pc64
lua                     5.3.1.04
mariadb                 3.0.6
nantToVSTools           3.19.09
PcLint                  9.00k-nonproxy
pclintconfig            0.17.02
PPMalloc                1.26.00
ptmalloc3               1.0-1-custom
UnixClang               4.0.1
UTFXml                  3.08.09
vstomaketools           2.06.09
WindowsSDK              10.0.14393-proxy
zlib                    1.2.11
```

These are the versions used in the available pre-compiled libs for Blaze SDK. If needed, the SDK can be built for older packages/libraries by modifying the masterconfig.xml file. Building using newer packages/libraries may not work depending on the changes in the newer versions; check with GOS if you are trying to build with newer versions and having problems.



# Blaze Release Info

* Version: 15.1.1.9.3
* Date: April 21, 2020
* [Greenlight features]( https://eadpjira.ea.com/issues/?jql=status%20in%20(Completed)%20AND%20fixVersion%20%3D%20Trama.3 )
* [Resolved Bugs ]( https://eadpjira.ea.com/issues/?jql=project%20%3D%20%22GOS%22%20and%20issuetype%20%3D%20%22Blaze%20Internal%20Bug%22%20and%20status%20%3D%20Closed%20and%20fixVersion%20%3D%20Trama.3 )
* [2014 Spring --> 2015 Winter UPGRADE GUIDE]( https://developer.ea.com/display/blaze/Blaze+2015+Winter+1.x+Continuous+Feature+Upgrade+Guide )

# Blaze Documentation

* [Getting Started with Blaze]( https://developer.ea.com/display/blaze/Getting+Started )
* [Blaze Customer Portal]( https://developer.ea.com/display/blaze/Blaze+Customer+Portal )
* [Blaze Technical Designs]( https://developer.ea.com/display/TEAMS/Blaze )

# Compatibility

### Clients
* 15.1.1.9.2 (Trama.2)

### Framework Versions
* Integrators not supporting Gen5 consoles can still build BlazeSDK and DirtySDK with Framework 7.  Gen5 console support requires Framework 8.

# Known Issues
* No Known Issues

# Needs Action

### General
* The Build Framework System has been updated in Trama.3 to support new platforms. The newer version has reversed the platform name changes made as part of Trama.2. 
    | Previous      | Now           |
    | :------------- |:-------------|
    | win64      | pc64 |
    | linux64      | unix64 |
    | win32      | pc |
    | ps4      | kettle |
    | xb1       | capilano |
 
    Unfortunately, for teams that have previously upgraded to Trama.2 and are now upgrading to Trama.3, build scripts and configurations will again need an adjustment. For details on upgrading to Framework 8, reference [Framework upgrade documentation](https://frostbite-ew.gitlab.ea.com/framework/fwdocs/upgrade-guides/framework8/).
* For teams doing Gen5 development, the following package versions or greater should be used with this release.
    * Framwork 8.03.02        (needed for Gen5 SDK packages compatability)
    * GDK 202000-proxy        (lastest GDK and it contains bug fixes)
    * PS5 0.95.00.36-proxy    (lastest SDK and is required for Blaze PS5 features added in this release)
    * With other packages, we recommend you match versions used in the BlazeSDK masterconfig, as those are the packages we tested this release with.

### GameManager
* [GOS-32819](https://eadpjira.ea.com/browse/GOS-32819) - For clarity, several old names of 'external sessions' typedefs have been deprecated in favor of their newer names:

    | Previous                      | Now                       |
    | :-----------------------------|:--------------------------|
    | Scid                          | XblScid                   |
    | ExternalSessionTemplateName   | XblSessionTemplateName    |
    | ExternalSessionName           | XblSessionName            |
    | ContractVersion               | XblContractVersion        |
    | ExternalSessionActivityHandleId | XblActivityHandleId     |
    | ExternalSessionCorrelationId  | XblCorrelationId          |
    | ExternalSessionInviteProtocol | XblSessionInviteProtocol  |
    | ExternalSessionActivityHandleId | XblSessionActivityHandleId |
    | ExternalSessionTournamentDefinitionName | XblTournamentDefinitionName |
    | ExternalSessionImage          | Ps4NpSessionImage         |
    | ExternalSessionStatusString   | Ps4NpSessionStatusString  |
    | ExternalSessionStatusStringLocaleMap | Ps4NpSessionStatusStringLocaleMap |
    | ExternalSessionServiceLabel   | PsnServiceLabel           |
    
    The previous names may be removed in future releases. To avoid future build breaks, replace instances of them with their new name in your title code.
* [GOSREQ-3441](https://eadpjira.ea.com/browse/GOSREQ-3441) - As part of maintenance, the GameCreationData.externalSessionTemplate member in create game, reset game and Matchmaking scenario requests, has been deprecated in favor of using the requests' new GameCreationData.externalSessionIdentSetup.xone.templateName member. Blaze's stock create_game_templates.cfg and scenarios.cfg files have been updated to use the new GameCreationData.externalSessionIdentSetup.xone.templateName instead. The old GameCreationData.externalSessionTemplate member may be removed in future releases. To avoid future build breaks, replace instances of it with using the new GameCreationData.externalSessionIdentSetup.xone.templateName in your title code.

### Matchmaker
* [GOS-32757](https://eadpjira.ea.com/browse/GOS-32757) - For performance, by default, Blaze's Matchmaking Diagnostics metrics are now disabled from being populated in getMatchmakingMetrics responses. To enable the Matchmaking Diagnostics, titles can specify trackDiagnostics = true within the matchmakerSettings section of Blaze Server's matchmaker_settings.cfg. Reference docs on Matchmaking Diagnostics here: https://developer.ea.com/display/blaze/Matchmaking+Diagnostics .

### Authentication
* [GOS-32832](https://eadpjira.ea.com/browse/GOS-32832) - Users with more than one 1st-party account linked to their Nucleus account from the same platform will receive AUTH_ERR_DUPLICATE_LOGIN when trying to log in concurrently with their other personas. Titles should provide specific UI to end users in this situation so they can correct it by either logging their other persona out of Blaze, or delink their additional same-platform personas from their Nucleus account.
* [GOSREQ-3342](https://eadpjira.ea.com/browse/GOSREQ-3342) - In accordance with the previous communication to GS-Blaze Community, the ID 1.0 access has been removed from Blaze Server. For details, visit https://developer.ea.com/display/blaze/Identity+1.0+Access+Status.

### Framework
* [GOSREQ-3296](https://eadpjira.ea.com/browse/GOSREQ-3296) - logWarnForHttpCodes from HttpServiceConfig has been removed and logErrForHttpCodes has been added to the configuration. The new member allows for error logging over a range of http codes. If you were previously using logWarnForHttpCodes in any custom http service configuration, you'd need to remove the it and use logErrForHttpCodes.  

# New Features

### GameManager
* [GOSREQ-3441](https://eadpjira.ea.com/browse/GOSREQ-3441) - Support for PS5 1st-party invites and UX integration has been added. By default Blaze manages games' PS5 PlayerSessions and Match Activities S2S. For details see: https://developer.ea.com/display/TEAMS/PS5+Upgrade+Guide+Sample .
* [GOSREQ-3299](https://eadpjira.ea.com/browse/GOSREQ-3299) - As part of the updates to support 1st-party PlayerSessions on PS5, the max character length of the GameManager::PersistedGameId define has been increased from 36 to 64. The extra length is only used for PS5. UUID based PersistedGameIds for non-PS5 titles and titles on older Blaze versions still only use 36 characters.
* [GOSREQ-3296](https://eadpjira.ea.com/browse/GOSREQ-3296) - Blaze Nucleus Integration Improvement
    * Blaze server now forces user logout if attempts to following operations result in an unrecoverable state (4xx). In such scenarios, BlazeStateEventHandler::onForcedDeAuthenticated callback is called. 
        * refresh user access token; 
        * obtaining first party access token
    * logErrForHttpCodes has been added to the http service configuration. The new member allows for automatic error logging over a range of http codes.
    * Blaze Stress Test Tool now supports Nucleus Login Flow. Both Stress Login Flow and Nucleus Login Flow can be used for testing single platform and shared cluster.
    * Logging of http request and response is improved with better arrangement.


### Framework
* [GOSREQ-3194](https://eadpjira.ea.com/browse/GOSREQ-3194) - As part of the 3M PSU scalability testing, we have added/changed the following features:
    * Added a new setting called listenerMatrixShapeModifier which is a property of a Sliver Namespace configured in ./etc/framework/slivers.cfg.  The effect of the setting is well described in slivers.cfg.  But briefly, you can fine tune this setting to get an optimal balance between CPU and Memory usage among searchSlaves.  If your searchSlaves are using very little CPU, but consuming a lot of memory, then try lowering this setting closer to 0.  If they are running hot, and you have a lot RAM available, try turning this setting up closer to 10.
    * Added a new setting called updateCoalescePeriod which is a property of a Sliver Namespace configured in ./etc/framework/slivers.cfg.  Use updateCoalescePeriod to delay replication of changes made to the state of a replicated object, such as a GameSession or UserSession.  Because these objects can change many many times in half a second, delaying for even 100ms and then replicating the sum of all change can save tens of thousands of replication messages from being sent and processed, resulting in very significant CPU saving.
    * The out-of-the-box default value for EXTERNAL_FIRE_MAX_CONNS (commondefines.cfg) has been increased to 15000 (was 10000).  The Blaze servers have been able to handle more than 10,000 connections for sometime.  We have tested up to 25,000 connections to a single coreSlave.
    * The out-of-the-box default value for UEDUpdateRateTimeDelay (usersessions.cfg) has been lowered to 250ms (was 1s).  This value was lowered because it was observed that bursts of changes to UED tend to occur in under 250ms.
    * The out-of-the-box default value for defaultIdlePeriod (gamebrowser.cfg) has been increased to 150ms (was 50ms).  This value was increased because of the observed savings in CPU usage was significant relative to the observed impact of waiting 100ms longer for game browser list updates.

# Inclusions/Dependencies

The default dependencies for packages and libraries used in this release of Blaze SDK are as follows:

Blaze SDK Dependencies
----------------------
```
ActivePython                    2.7.13.2716
android_config                  5.00.00-pr1
AndroidAPI                      28
AndroidBuildTools               29.0.2
AndroidEmulator                 1.18.00
AndroidNDK                      15.2.4203891
AndroidPlatformTools            28
AndroidSDK                      100.00.00-pr1
AndroidTools                    26.1.1
antlr                           3.5.2
ApacheAnt                       1.9.7
BlazeSDKSamplesLibPack          1.0.0
CapilanoSDK                     180712-1-proxy
ConsoleRemoteServer             0.03.08
coreallocator                   1.04.02
DirtySDK                        15.1.6.0.3
DotNet                          4.0-2
DotNetSDK                       2.0-4
Doxygen                         1.8.14-1
EAAssert                        1.05.07
EABase                          2.09.11
EACallstack                     2.02.01
EAController                    1.09.01
EAControllerUserPairing         1.12.01
EACrypto                        1.03.04
EAIO                            3.01.08
EAJson                          1.06.05
EAMessage                       2.10.05
EAStdC                          1.26.07
EASTL                           3.16.01
EASystemEventMessageDispatcher  1.05.00
EATDF                           15.1.1.9.3
EAThread                        1.33.00
EATrace                         2.12.03
EAUser                          1.09.01-fb
GDK                             200200-proxy
GradleWrapper                   1.00.00-pr1
GraphViz                        2.38.0
IncrediBuild                    3.61.1.1243
iphonesdk                       10.0-proxy-1
jdk                             azul-8.0.222-pc64
job_manager                     4.05.05
kettlesdk                       7.008.001-proxy
nantToVSTools                   3.19.09
NucleusSDK                      dev
nx_config                       1.00.05
nx_vsi                          9.0.6.27951
nxsdk                           9.0.0
opus                            1.00.05
OriginSDK                       10.6.2.13
PcLint                          9.00k-nonproxy
pclintconfig                    0.17.02
PinTaxonomySDK                  2.2.0
PlayStation4AppContentEx        6.508.010
PlayStation4AudioInEx           6.508.001
PPMalloc                        1.26.07
ProDG_VSI                       7.00.0.5-proxy
ps5sdk                          0.95.00.36-proxy
Ps5VSI                          0.95.0.4-proxy
PyroSDK                         15.1.1.9.3
rwmath                          3.01.01
Speex                           1.2rc1-7
StadiaSDK                       dev-proxy
TelemetrySDK                    19.3.1.0.0
TypeDatabase                    1.04.02
UnixClang                       4.0.1
userdoc                         1.00.00
UTFXml                          3.08.09
vstomaketools                   2.06.09
WindowsSDK                      10.0.18362
XcodeProjectizer                3.04.00
zlib                            1.2.11
```

Blaze Server Dependencies
-------------------------
```
ActivePython            2.7.13.2716-blaze-1
antlr                   3.5.2
bison                   2.1
blazeserver_grpc_tools  15.1.1.9.2
c_ares                  container-proxy-1
centos7_base            flattened
coreallocator           1.04.02
EAAssert                1.05.07
EABase                  2.08.04
EAIO                    2.23.01
EAJson                  1.06.05
EARunner                1.09.01
EAStdC                  1.23.00
EASTL                   3.12.01
EATDF                   15.1.1.9.3
EATDF_Java              1.09.00
EAThread                1.32.06
flex                    2.5.4a
GamePacker              1.03.04
google_coredumper       container-proxy
google_perftools        container-proxy
gtest                   1.8.0.ea.10
hiredis                 0.14.0-custom-4
IncrediBuild            3.61.1.1243
jdk                     azul-8.0.222-pc64
libcurl                 7.62.0-3-ssh2
libopenssl              1.1.1d-prebuilt
libssh2                 1.8.0-3-pc64
lua                     5.3.1.04
mariadb                 3.0.6
nantToVSTools           3.19.09
PcLint                  9.00k-nonproxy
pclintconfig            0.17.02
PPMalloc                1.26.00
ptmalloc3               1.0-1-custom
UnixClang               4.0.1
UTFXml                  3.08.09
vstomaketools           2.06.09
WindowsSDK              10.0.14393-proxy
zlib                    1.2.11
```

These are the versions used in the available pre-compiled libs for Blaze SDK. If needed, the SDK can be built for older packages/libraries by modifying the masterconfig.xml file. Building using newer packages/libraries may not work depending on the changes in the newer versions; check with GOS if you are trying to build with newer versions and having problems.



# Blaze Release Info

* Version: 15.1.1.9.2
* Date: February 14, 2020
* [Greenlight features]( https://eadpjira.ea.com/issues/?jql=status%20in%20(Completed)%20AND%20fixVersion%20%3D%20Trama.2 )
* [Resolved Bugs ]( https://eadpjira.ea.com/issues/?jql=project%20%3D%20%22GOS%22%20and%20issuetype%20%3D%20%22Blaze%20Internal%20Bug%22%20and%20status%20%3D%20Closed%20and%20fixVersion%20%3D%20Trama.2 )
* [2014 Spring --> 2015 Winter UPGRADE GUIDE]( https://developer.ea.com/display/blaze/Blaze+2015+Winter+1.x+Continuous+Feature+Upgrade+Guide )

# Blaze Documentation

* [Getting Started with Blaze]( https://developer.ea.com/display/blaze/Getting+Started )
* [Blaze Customer Portal]( https://developer.ea.com/display/blaze/Blaze+Customer+Portal )
* [Blaze Technical Designs]( https://developer.ea.com/display/TEAMS/Blaze )

# Compatibility

### Clients
This release's server has been tested with the following clients (in addition to this release's client):

* 15.1.1.9.0 (Trama.0)

### Framework Versions
* Integrators not supporting Gen5 consoles can still build BlazeSDK and DirtySDK with Framework 7.  Gen5 console support requires Framework 8.

# Known Issues

* [GOSOPS-190647](https://eadpjira.ea.com/browse/GOSOPS-190647)  This release uses Framework 8 which has a bug with case sensitivity in paths, which can cause compilation issues.
    * If you hit the issue described in the ticket ("error C2259: 'BlazeLexer': cannot instantiate abstract class"  or  "warning C4005: 'yyparse': macro redefinition") a workaround is available:
    * You can workaround this issue by running nant under the Windows Powershell.

# Needs Action

### General

* [GOSREQ-3180](https://eadpjira.ea.com/browse/GOSREQ-3180)  Early Gen5 features are included to facilitate integrators.  The list of what is included for Gen5 can be found on [Blaze Release Page](https://developer.ea.com/display/blaze/Release+Schedule)  Gen5 features should be considered as "integration ready," not "release ready."
    * To support building BlazeSDK on Gen 5 and future platforms, BlazeSDK's build scripts been updated to use Framework 8 by default. As part of these updates, several older/deprecated Framework build configuration names have been replaced in Blaze's build scripts by newer Framework names, as follows:
    | Previous      | Now           |
    | :------------- |:-------------|
    | pc64-vc-dev-debug      | win64-vc-debug |
    | pc64-vc-dev-debug-opt      | win64-vc-release      |
    | pc64-vc-dev-opt | win64-vc-final      |
    | pc64-vc-dll-dev-debug | win64-vc-dll-debug      |
    | pc64-vc-dll-dev-debug-opt | win64-vc-dll-release      |
    | pc64-vc-dll-dev-opt | win64-vc-dll-final      |
    | kettle-clang-dev-debug | ps4-clang-debug      |
    | kettle-clang-dev-debug-opt | ps4-clang-release      |
    | kettle-clang-dev-opt | ps4-clang-final      |
    | capilano-vc-dev-debug | xb1-vc-debug      |
    | capilano-vc-dev-debug-opt | xb1-vc-release      |
    | capilano-vc-dev-opt | xb1-vc-final      |
    | unix64-clang-dev-debug | linux64-clang-debug      |
    | unix64-clang-dev-debug-opt | linux64-clang-release      |
    | unix64-clang-dev-opt | linux64-clang-final      |

    For details on upgrading to Framework 8, reference [Framework upgrade documentation](https://frostbite-ew.gitlab.ea.com/framework/fwdocs/upgrade-guides/framework8/).

    * BlazeSDK's stock build scripts have been updated to use the latest versions of the EAIO in order to support file I/O on PS5. To avoid build issues, titles using this and later versions of EAIO should ensure using up-to-date job_manager and rwmath versions in their masterconfig file. Reference Frostbite package docs/info for details.

    * BlazeSDK's stock build scripts have been updated to use the latest versions of the EAUser, EASystemEventMessageDispatcher, EAControllerUserPairing and EAController (see BlazeSDK's masterconfig.xml). To prevent issues, when using these package versions, titles should remove extra dependencies/references on the old IEAUser, IEASystemEventMessageDispatcher, and IEAController packages in their builds, as their contents are now part of the EAUser, EASystemEventMessageDispatcher and EAController packages respectively. Reference Frostbite package docs for details.

* [GOSREQ-3181](https://eadpjira.ea.com/browse/GOSREQ-3181) - Blaze has been update to support building and running with PS5 and XBSX platforms. To start a Blaze Server using PS5 and XBSX platforms locally, manually add a new DB schemas of the form \<username\>_ps5 and \<username\>_xbsx respectively, with your Blaze Server mysql instance. For details, reference [db set up](https://developer.ea.com/display/blaze/Setting+Up+Your+Database).

### GameManager
* [GOS-32213](https://eadpjira.ea.com/browse/GOS-32213), [GOS-32754](https://eadpjira.ea.com/browse/GOS-32754), [GOS-32767](https://eadpjira.ea.com/browse/GOS-32767), [GOS-32768](https://eadpjira.ea.com/browse/GOS-32768), [GOS-32769](https://eadpjira.ea.com/browse/GOS-32769) - enableNetworkAddressProtection in gamesession.cfg now defaults to 'true'. This configuration must be set to true for titles supporting cross-play. In dedicated server game sessions, player clients will only have access to the dedicated server's address. The dedicated server host will have full access to game member network addresses. In CCS_MODE_HOSTED_ONLY games, players will not get each other's network addresses. GameGroup members and NETWORK_DISABLED game session members will also not get network addresses for their fellow game members unless P2P VoIP is enabled either without CCS enabled or in CCS_MODE_HOSTED_FALLBACK mode.


### Framework
* [GOS-32766](https://eadpjira.ea.com/browse/GOS-32766) - UserExtendedData will not provide a NetworkAddress for any non-local user updates, unless the recipient has PERMISSION_OTHER_NETWORK_ADDRESS. This permission is enabled by default for the DEDICATED_SERVER, and the TOOLS access groups, and disabled by default for end-user client AccessGroups. This permission does not impact transmission of NetworkAddresses to members of GameSessions as required for gameplay or VoIP connectivity.
* [GOSREQ-3296](https://eadpjira.ea.com/browse/GOSREQ-3296) - Removed Blaze::Authentication::MAX_AUTH_TOKEN_LENGTH define which was arbitrarily set to 1024 . The JWT tokens can be variable length. If you rely
on this define, you'll need to adjust the code (typically by using a string to hold the token rather than a char buffer).  

# New Features

### GameManager
* [GOS-32213](https://eadpjira.ea.com/browse/GOS-32213), [GOS-32754](https://eadpjira.ea.com/browse/GOS-32754) - CCS_MODE_HOSTED_FALLBACK can be used for cross-play game sessions. When game members are on different platforms, they will never attempt to connect directly to clients of a different platform. Additionally, player network addresses are not sent to users on a different platform. For the network addresses to be hidden, enableNetworkAddressProtection in gamesession.cfg must be set to 'true', which is the new default value.

### Matchmaker
* [GOS-32788](https://eadpjira.ea.com/browse/GOS-32788) - The AvoidPlayersRule has a new input parameter, mAvoidAccountList (avoidAccountList in scenarios.cfg's rule definition), that allows for Nucleus AccountIds to be used when trying to avoid specific players in matchmaking.

### Framework

* [GOS-32765](https://eadpjira.ea.com/browse/GOS-32765) - EATDF TdfStructVector::insert has been fixed to copy the inserted data type into a newly allocated struct, rather than simply recreating a reference to the original value.  Additionally, the range insert and multiple value insert functions that were available in TdfPrimitiveVector are now available in TdfStructVector as well.

* [GOS-32764](https://eadpjira.ea.com/browse/GOS-32764) - The localization file now supports the following escape characer sequences: '/t' '/r' '/n'.  These values will be replaced automatically, when the are read in.   For other special characters, simply enclose the string in double quotes.  (Ex. "String, with, commas")

* [GOSREQ-3299](https://eadpjira.ea.com/browse/GOSREQ-3299) - BlazeSDK has been updated to support building with Microsoft's November Xbox GDK and Sony's 0.90 PS5 SDKs. For details reference [Blaze Gen 5 build documentation](https://developer.ea.com/display/blaze/Gen5).

* [GOSREQ-3181](https://eadpjira.ea.com/browse/GOSREQ-3181) - On Gen 5 Xbox, BlazeSDK and DirtySDK have been updated to automatically make use of Microsoft's new 'Preferred Local UDP Multiplayer Port'. See Microsoft GDK docs for details on the feature. To streamline use, on Gen 5 Xbox, BlazeSDK will use the port automatically as the client's Game Port by default, and ignores the BlazeHub::InitParameters::GamePort parameter, even if specified. Reference Blaze docs: https://developer.ea.com/display/TEAMS/Preferred+Local+UDP+Multiplayer+Ports+Wiki+Doc .

* [GOSREQ-3100](https://eadpjira.ea.com/browse/GOSREQ-3100) - A new set of Connection PIN events have been added.  These events add a large amount of addition connection data that can be used to diagnose issues in PIN.  The rate of data can be controlled by the following:  DisableDetailedConnectionStats - disables the metrics, FlushDetailedConnectionStatsOnExport - disables the metrics when a ZDT export occurs, ConnStatsMaxEntries - sets how many metric entries are included in each call.

* [GOSREQ-3284](https://eadpjira.ea.com/browse/GOSREQ-3284) - Pin login/logout events now contain the user's crossplay opt out status (only meaningful if the title and platform actually supports cross play).

* [GOSREQ-3296](https://eadpjira.ea.com/browse/GOSREQ-3296) - Blaze can now be configured to request user access tokens in [JWT format](https://developer.ea.com/display/blaze/JWT+Support) to provide better resiliency against Nucleus storage outages.

* [GOSREQ-2913](https://eadpjira.ea.com/browse/GOSREQ-2913) - Oracle JDK is replaced by Azul Zulu JDK.

* [GOSREQ-3156](https://eadpjira.ea.com/browse/GOSREQ-3156) - Blazeserver and BlazeSDK support Visual Studio 2019. Use `-G:vsversion=2019` with nant to build solutions for Visual Studio 2019.

* [GOSREQ-3135](https://eadpjira.ea.com/browse/GOSREQ-3135) - Blaze supports OpenSSL-1.1.1d.

* [GOSREQ-3136](https://eadpjira.ea.com/browse/GOSREQ-3136) - Blaze supports TLS-1.3 (default is TLS-1.2).

# Inclusions/Dependencies

The default dependencies for packages and libraries used in this release of Blaze SDK are as follows:

Blaze SDK Dependencies
----------------------
```
ActivePython                    2.7.13.2716
android_config                  5.00.00-pr1
AndroidAPI                      28
AndroidBuildTools               29.0.2
AndroidEmulator                 1.18.00
AndroidNDK                      15.2.4203891
AndroidPlatformTools            28
AndroidSDK                      100.00.00-pr1
AndroidTools                    26.1.1
antlr                           3.5.2
ApacheAnt                       1.9.7
BlazeSDKSamplesLibPack          1.0.0
CapilanoSDK                     180712-1-proxy
ConsoleRemoteServer             0.03.02
coreallocator                   1.04.02
DirtySDK                        15.1.6.0.2
DotNet                          4.0-2
DotNetSDK                       2.0-4
Doxygen                         1.8.14-1
EAAssert                        1.05.07
EABase                          2.09.11
EACallstack                     2.02.01
EAController                    1.07.01
EAControllerUserPairing         1.11.08
EACrypto                        1.03.04
EAIO                            3.01.03
EAJson                          1.06.05
EAMessage                       2.10.05
EAStdC                          1.26.07
EASTL                           3.15.00
EASystemEventMessageDispatcher  1.05.00
EATDF                           15.1.1.9.2
EAThread                        1.33.00
EATrace                         2.12.03
EAUser                          1.08.01
GradleWrapper                   1.00.00-pr1
GraphViz                        2.38.0
GSDK                            191100-proxy
IncrediBuild                    3.61.1.1243
iphonesdk                       10.0-proxy-1
jdk                             azul-8.0.222-pc64
job_manager                     4.05.05
kettlesdk                       7.008.001-proxy
nantToVSTools                   3.19.09
NucleusSDK                      dev
nx_config                       1.00.05
nx_vsi                          9.0.6.27951
nxsdk                           9.0.0
opus                            1.00.05
OriginSDK                       10.6.2.13
PcLint                          9.00k-nonproxy
pclintconfig                    0.17.02
PinTaxonomySDK                  2.2.0
PlayStation4AppContentEx        6.508.010
PlayStation4AudioInEx           6.508.001
PPMalloc                        1.26.07
ProDG_VSI                       7.00.0.5-proxy
ps5sdk                          0.90.00.24-proxy
Ps5VSI                          0.90.0.3-proxy
PyroSDK                         15.1.1.9.2
rwmath                          3.01.01
Speex                           1.2rc1-7
TelemetrySDK                    19.3.1.0.0
TypeDatabase                    1.04.02
UnixClang                       4.0.1
userdoc                         1.00.00
UTFXml                          3.08.09
vstomaketools                   2.06.09
WindowsSDK                      10.0.18362
XcodeProjectizer                3.04.00
zlib                            1.2.11
```

Blaze Server Dependencies
-------------------------
```
ActivePython            2.7.13.2716-blaze-1
antlr                   3.5.2
bison                   2.1
blazeserver_grpc_tools  15.1.1.9.2
c_ares                  container-proxy-1
centos7_base            flattened
coreallocator           1.04.02
EAAssert                1.05.07
EABase                  2.08.04
EAIO                    2.23.01
EAJson                  1.06.05
EARunner                1.09.01
EAStdC                  1.23.00
EASTL                   3.12.01
EATDF                   15.1.1.9.2
EATDF_Java              1.09.00
EAThread                1.32.06
flex                    2.5.4a
GamePacker              1.03.04
google_coredumper       container-proxy
google_perftools        container-proxy
gtest                   1.8.0.ea.10
hiredis                 0.14.0-custom-3
IncrediBuild            3.61.1.1243
jdk                     azul-8.0.222-pc64
libcurl                 7.62.0-2-ssh2
libopenssl              1.1.1d-prebuilt
libssh2                 1.8.0-2-pc64
lua                     5.3.1.04
mariadb                 3.0.6
nantToVSTools           3.19.09
PcLint                  9.00k-nonproxy
pclintconfig            0.17.02
PPMalloc                1.26.00
ptmalloc3               1.0-1-custom
UnixClang               4.0.1
UTFXml                  3.08.09
vstomaketools           2.06.09
WindowsSDK              10.0.14393-proxy
zlib                    1.2.11
```

These are the versions used in the available pre-compiled libs for Blaze SDK. If needed, the SDK can be built for older packages/libraries by modifying the masterconfig.xml file. Building using newer packages/libraries may not work depending on the changes in the newer versions; check with GOS if you are trying to build with newer versions and having problems.



---
Blaze 
=====
---

Release Info
-------------

* Version: 15.1.1.9.1
* Date: October 18, 2019
* [Greenlight features]( https://eadpjira.ea.com/issues/?jql=status%20in%20(Completed)%20AND%20fixVersion%20%3D%20Trama.1 )
* [Resolved Bugs ]( https://eadpjira.ea.com/issues/?jql=project%20%3D%20%22GOS%22%20and%20issuetype%20%3D%20%22Blaze%20Internal%20Bug%22%20and%20status%20%3D%20Closed%20and%20fixVersion%20%3D%20Trama.1 )
* [2014 Spring --> 2015 Winter UPGRADE GUIDE]( https://developer.ea.com/display/blaze/Blaze+2015+Winter+1.x+Continuous+Feature+Upgrade+Guide )

Blaze Documentation
-------------------

* [Getting Started with Blaze]( https://developer.ea.com/display/blaze/Getting+Started )
* [Blaze Customer Portal]( https://developer.ea.com/display/blaze/Blaze+Customer+Portal )
* [Blaze Technical Designs]( https://developer.ea.com/display/TEAMS/Blaze )

Compatibility
-------------
This release's server has been tested with the following clients (in addition to this release's client):
15.1.1.9.0

Known Issues
------------
> [GOS-32478](https://eadpjira.ea.com/browse/GOS-32478) - Player fails to join game with P2P full mesh topology in crossplay unless there is a delay after the game is created.


---
Needs Action
============
> [GOS-32375](https://eadpjira.ea.com/browse/GOS-32375) - If your team uses stats service from Persistent Services team, you must obtain a custom address for STATSERVICEHOST from GS-Services Support and set the value in 
titledefines.cfg.

> [GOS-32406](https://eadpjira.ea.com/browse/GOS-32406) - When upgrading BlazeSDK to Trama.1 be sure to also upgrade DirtySDK to Trama.1, the BlazeSDK makes use of new VoipNarrateControl('ctsm') and 
VoipTranscribeControl('cstm') to clear stt/tts metrics between sessions.

> [GOS-32324](https://eadpjira.ea.com/browse/GOS-32324) - EATDF's tdf_ptr class now properly maintains const-ness when converting to a pointer.  This means that code that previously converted constant iterators into 
non-const pointers (ex. MyData* myData = constMapOfDataIterator->second;) will need to be updated (by adding const).  This functionality is controlled by the **EA_TDF_PTR_CONST_SAFETY** compiler define, which is enabled 
(set to 1) by default for both blaze server and BlazeSDK.  If needed, the compile define can be set to 0, but we highly recommend simply updating your code to use const pointers. 

> InstanceId size has increased by 2 bits, and INVALID_INSTANCE_ID has been changed to 0. The valid InstanceId range is now 1 through 4095 (inclusive). Be sure to adjust your server deploy configs 
(the server\_\<env>\_\<plat>.cfg files in the blazeserver/bin directory) to use a startId of 1 instead of 0. 

> If a user has more than one persona linked to their Nucleus account from the same platform (ie: two XBL GamerTags), they are blocked from being logged in at the same time. If the first persona is already logged in, 
the Login RPC will return ERR_DUPLICATE_LOGIN when the Nth linked persona on the same plaform attempts to log in. If the first persona logs out, the other persona(s) are able to log in to Blaze. Titles should handle this 
error by displaying an informative message. Please reach out to WWCE and Legal to get approvals for user-facing messaging.


---
New Features
============


Framework
------------

> Multiple logins on the same platform with different personas is always blocked by Blaze. The 'singleLoginCheckPerAccount' flag in usersessions.cfg will prevent a Nucleus account from logging into multiple personas 
across platforms if set to true, and only within the same platform if set to false.

> [GOS-32301](https://eadpjira.ea.com/browse/GOS-32301) - Database configuration now only applies maxConnCount from the instanceOverrides map. No other parameter can be overriden by using instanceOverrides.

> [GOS-32386](https://eadpjira.ea.com/browse/GOS-32386) - Metrics now support a whitelist for OI export, in addition to the existing blacklist.  While the default blazeserver metrics are unchanged, any custom metrics 
should be added to the whitelist in order to be exported. 

> [GOSREQ-2916](https://eadpjira.ea.com/browse/GOSREQ-2916) - Blaze now supports data type double for external data source. A scaling factor attribute is introduced in external source config to scale up/down the received 
integer/double values. Scaled double values are casted to int64 and hence used by Blaze. Precision loss is possible during the process.

> [GOSREQ-3069](https://eadpjira.ea.com/browse/GOSREQ-3069) - New metrics are surfaced to track configured outbound HTTP and gRPC calls(number of calls started, finished, failed, response code and time). These metrics can be viewed by calling blazecontroller::getStatus.



GameManager
------------
> In a cross play enabled blaze server, outage of the first party external session services do not block the gamemanager/matchmaking functionality for the players that don't belong to the first party platform suffering 
from the outage. 

> **assignPlatformHostForDedicatedServerGames** in gamesession.cfg is now set to false by default. This is a long deprecated functionality.  When set to false, attempts to retrieve the "platform host" for dedicated server 
game sessions in the GameManagerAPI will return NULL. Titles querying for the platform host to determine which player should take action to modify or update game settings / state or attributes should instead query the 
game's Admin list. If this value is set to true, a first party outage in a cross play cluster may end up affecting off platform players, and titles will also encounter host migrations in dedicated server game sessions as 
Blaze is forced to select a new 'PlatformHost' when the prior one departs a game.

> [GOS-32379](https://eadpjira.ea.com/browse/GOS-32379) - When setting enableNetworkAddressProtection to true in gamesession.cfg, addresses are masked for peer-mesh & hosted topologies when CCS mode is set to 
CCS_MODE_HOSTEDONLY.

> [GOS-32367](https://eadpjira.ea.com/browse/GOS-32367) - The "dirtyCastGame" template has been moved from /etc/gamemanager/examples/create_game_template_examples.cfg to 
/etc/gamemanager/tools/create_game_template_dirtycast.cfg and included in /etc/gamemanager/create_game_templates.cfg seperately so titles can comment out example template includes without disabling the dirtycast game template.

> [GOS-32265](https://eadpjira.ea.com/browse/GOS-32265) - The XblBlockList can now be disabled at the Rule level, using the disableRule parameter.  This can be set via a Scneraio Rule. 

> [GOS-32297](https://eadpjira.ea.com/browse/GOS-32297) - Blaze's stock Ignition samples on Xbox One have been updated to use a new Xbox One sample title setup via Microsoft's new Partner Center site 
('BlazeSDK Sample 2', title id 2056720033). The old 'BlazeSDK Sample' and title id, setup via MS's old XDP site, will only continue to be used with prior Blaze versions' samples going forward. Reference Blaze and 
Microsoft Partner Center and XDP Migration documentation for details.

> [GOS-32372](https://eadpjira.ea.com/browse/GOS-32372) - The privacy settings of PS4 games now automatically change to private when the Game is switched to disable join by player, browsing, and matchmaking. This is done 
to allow invite only games on the PS4.

> The preferredPlayersRule now supports matching by Nucleus account id. Set PreferredPlayersRuleCriteria::mPreferredAccountList to increase the chance of matching sessions/games containing players with the specified 
Nucleus account id(s).

> Presence mode can now be disabled per platform. Set PresenceModeDisabledList in the game creation data to specify platform(s) on which presence mode should be disabled for the game. Clients who join the game on these 
platforms will receive NotifyGameSetup notifications with PRESENCE_MODE_NONE, instead of the game's configured presence mode setting.


Inclusions/Dependencies
=======================
---


The default dependencies for packages and libraries used in this release of Blaze SDK are as follows:

Blaze SDK Dependencies
----------------------
```
ActivePython                     2.7.13.2716
android_config                   4.00.00
AndroidEmulator                  1.18.00
AndroidNDK                       r13b
AndroidSDK                       24.4.1-1
antlr                            3.5.2
ApacheAnt                        1.9.7
BlazeSDKSamplesLibPack           1.0.0
capilano_config                  2.07.03
CapilanoSDK                      180709-proxy
ConsoleRemoteServer              0.03.02
coreallocator                    1.04.02
DirtySDK                         15.1.6.0.1
DotNet                           4.0-2
DotNetSDK                        2.0-4
Doxygen                          1.8.14-1
EAAssert                         1.05.07
EABase                           2.09.02
EACallstack                      2.01.02
eaconfig                         5.14.04
EAController                     1.05.01
EAControllerUserPairing          1.08.02
EACrypto                         1.03.01
EAIO                             2.23.03
EAJson                           1.06.05
EAMessage                        2.10.03
EAStdC                           1.26.00
EASTL                            3.12.01
EASystemEventMessageDispatcher   1.04.03
EATDF                            2.30.00
EAThread                         1.32.01
EATrace                          2.12.03
EAUser                           1.05.01
GraphViz                         2.38.0
IEAController                    1.02.03
IEASystemEventMessageDispatcher  1.00.05
IEAUser                          1.03.00
IncrediBuild                     3.61.1.1243
ios_config                       1.09.00
iphonesdk                        10.0-proxy-1
jdk                              1.8.0_112-pc64
kettle_config                    2.11.00
kettlesdk                        6.008.001-proxy
nantToVSTools                    3.19.09
NucleusSDK                       19.2.1.0.0-pr
nx_config                        1.00.05
nx_vsi                           9.0.6.27951
nxsdk                            9.0.0
opus                             1.00.01
PcLint                           9.00k-nonproxy
pclintconfig                     0.17.02
PinTaxonomySDK                   2.2.0
PlayStation4AudioInEx            5.508.001
PPMalloc                         1.26.00
PyroSDK                          15.1.1.9.1
Speex                            1.2rc1-4
TelemetrySDK                     19.3.1.0.0
TypeDatabase                     1.04.02
UnixClang                        4.0.1
userdoc                          1.00.00
UTFXml                           3.08.09
VisualStudio                     15.4.27004.2002-2-proxy
vstomaketools                    2.06.09
WindowsSDK                       10.0.14393-proxy
XcodeProjectizer                 3.04.00
zlib                             1.2.11
```

Blaze Server Dependencies
-------------------------
```
ActivePython            2.7.13.2716-blaze-1
antlr                   3.5.2
bison                   2.1
blazeserver_grpc_tools  15.1.1.9.0
c_ares                  container-proxy
centos7_base            flattened
coreallocator           1.04.02
EAAssert                1.05.07
EABase                  2.08.04
eaconfig                5.14.04
EAIO                    2.23.01
EAJson                  1.06.05
EARunner                1.09.01
EAStdC                  1.23.00
EASTL                   3.12.01
EATDF                   2.30.00
EATDF_Java              1.09.00
EAThread                1.32.06
flex                    2.5.4a
GamePacker              1.03.04
google_coredumper       container-proxy
google_perftools        container-proxy
gtest                   1.8.0.ea.10
hiredis                 0.14.0-custom-1
IncrediBuild            3.61.1.1243
jdk                     1.7.0_67-1-pc
libcurl                 7.62.0-ssh2
libopenssl              1.0.2p-prebuilt
libssh2                 1.8.0-pc64
lua                     5.3.1.04
mariadb                 3.0.6
nantToVSTools           3.19.09
PcLint                  9.00k-nonproxy
pclintconfig            0.17.02
PPMalloc                1.26.00
ptmalloc3               1.0-1-custom
UnixClang               4.0.1
UTFXml                  3.08.09
VisualStudio            15.4.27004.2002-2-proxy
vstomaketools           2.06.09
WindowsSDK              10.0.14393-proxy
zlib                    1.2.11
```

These are the versions used in the available pre-compiled libs for Blaze SDK. If needed, the SDK can be built for older packages/libraries by modifying the masterconfig.xml file. Building using newer packages/libraries may not work depending on the changes in the newer versions; check with GOS if you are trying to build with newer versions and having problems.



---
Blaze 
=====
---

Release Info
-------------

* Version: 15.1.1.9.0
* Date: July 24, 2019
* [Greenlight features]( https://eadpjira.ea.com/issues/?jql=status%20in%20(Completed)%20AND%20fixVersion%20%3D%20Trama )
* [Resolved Bugs ]( https://eadpjira.ea.com/issues/?jql=project%20%3D%20%22GOS%22%20and%20issuetype%20%3D%20%22Blaze%20Internal%20Bug%22%20and%20status%20%3D%20Closed%20and%20fixVersion%20%3D%20Trama )
* [2014 Spring --> 2015 Winter UPGRADE GUIDE]( https://developer.ea.com/display/blaze/Blaze+2015+Winter+1.x+Continuous+Feature+Upgrade+Guide )

Blaze Documentation
-------------------

* [Getting Started with Blaze]( https://developer.ea.com/display/blaze/Getting+Started )
* [Blaze Customer Portal]( https://developer.ea.com/display/blaze/Blaze+Customer+Portal )
* [Blaze Technical Designs]( https://developer.ea.com/display/TEAMS/Blaze )

Compatibility
-------------
This release's server has been tested with the following clients (in addition to this release's client):
TBD

Known Issues
------------
None


---
Needs Action
============

General
------------
> Blaze server can now be deployed as a *Shared Cluster* that supports multiple platforms. In addition, such a shared cluster can enable *cross play* between players on different platforms. All integrators are encouraged to read the [Trama Upgrade Guide](https://developer.ea.com/display/TEAMS/Trama+Upgrade+Guide) to familiarize themselves with the steps to upgrade. 


Framework
------------

> [GOSREQ-2727](https://eadpjira.ea.com/browse/GOSREQ-2727) Blaze Vault configuration has been modified to support the integration with ESS, which will impact any team already using the Blaze vault feature.  Please see the Upgrade Guide for more details.

> [GOS-31611](https://eadpjira.ea.com/browse/GOS-31611) - Sliver namespaces no longer provide a default configuration.  If your custom components are using slivers, you will need to supply a config setting in slivers.cfg.

> [GOS-31873](https://eadpjira.ea.com/browse/GOS-31873) - Shrike.1 fixes the dbmig python script failures many developers experienced when running the Shrike.0 blazeserver on Windows. After integrating this fix, be sure to delete the blazeserver build folder before regenerating the solution. Also delete the Python27 folder from your local %USERPROFILE%\\AppData\\Roaming\\Python directory, if it exists (this prevents the dbmig script from attempting to use any incompatible old installations of the MySQLdb python module).

> [GOS-32188](https://eadpjira.ea.com/browse/GOS-32188) External non secure endpoint access is now only allowed on trusted Ips in all environments(previously, only prod).
If you access the external non secure endpoints in your title, you may need to adjust trust settings on your endpoint to appropriately whitelist IPs.

> [GOS-32197](https://eadpjira.ea.com/browse/GOS-32197) - (Minor)  The parameters of HttpProtocolUtil::printHttpRequest have changed from a char* to an eastl::string, to avoid size limitations.

> If you use grpc_cli for development purpose, please go over https://developer.ea.com/display/blaze/grpc_cli+with+Blaze for updated documentation on using the tool. The input directories for proto files have been consolidated.  


GameManager
------------
> [GOSREQ-2870](https://eadpjira.ea.com/browse/GOSREQ-2870) The gamesession.cfg enableAutomaticAdminSelection setting has been extended to work for virtual games, and games where all players reserved until a user actively joins. When the flag is set to true for a game's game permissions, when an active player joins a game with no other active player admins, Blaze Server will automatically set that player to be an admin. For details, see docs on enableAutomaticAdminSelection.

---
New Features
============
* Shared Cluster and Cross Play 
* Profanity Filtering - https://developer.ea.com/display/blaze/Profanity+Filtering , see references to Shrike.1
* mTLS Support - https://developer.ea.com/display/blaze/Server+to+server+login+via+standardized+trust  *Since Shrike.1*
* Support for Nintendo NX *Since Shrike.2*


BlazeSDK
------------

Framework
------------

>[GOSREQ-2636](https://eadpjira.ea.com/browse/GOSREQ-2636) - Blaze now supports the Nintendo Switch platform. Note that the Origin namespace is used for Switch persona names, since Nintendo "nicknames" are not guaranteed to be unique. This means that Switch users must have an Origin account in order to log in to Blaze. Consequently, the Identity [Progressive Registration](https://developer.ea.com/display/Identity/Identity+Progressive+Registration) login flow is not supported on the Switch.

> [GOSREQ-2727](https://eadpjira.ea.com/browse/GOSREQ-2727) - Added support for Blaze to integrate with the EA Secrets Service (ESS) to allow secrets to be moved out of source control and into Vault. Please refer to the documentation at https://developer.ea.com/display/blaze/EA+Secrets+Service for more details

> [GOSREQ-2903](https://eadpjira.ea.com/browse/GOSREQ-2903) - Added support for mTLS connections to Blaze to allow game servers to be trusted based on their client cert rather than IP whitelist. Please refer to the documentation at https://developer.ea.com/display/blaze/Server+to+server+login+via+standardized+trust for more details

> [GOS-31899](https://eadpjira.ea.com/browse/GOS-31899) - Blazeserver has been updated to stats service version 18.4.3.0.0.


> [GOS-31899](https://eadpjira.ea.com/browse/GOS-31899) - Blazeserver has been updated to stats service version 18.4.3.0.0.

> [GOS-31914](https://eadpjira.ea.com/browse/GOS-31914) - All references to the dbmig schema downgrade functionality have been removed. Schema downgrades have never worked correctly and the feature was officially removed from the dbmig python script in Blaze 3.09. Since then, the blazeserver continued to invoke the dbmig script whenever a schema's current version did not match the requested version, but this was a no-op unless a schema upgrade was required. Starting in Shrike.1, the blazeserver logs a warning and does not invoke dbmig for a component if the requested schema version is lower than the version currently in the database.

> [GOS-31916](https://eadpjira.ea.com/browse/GOS-31916) - Blazeserver link time settings for Windows has been optimized for debug and incremental build. The link time are now down to ~20s from ~48s earlier.

> [GOS-32197](https://eadpjira.ea.com/browse/GOS-32197) - Cache-Control header support has been expanded.  Various minor issues related to caching have been resolved.   See more details [here](https://developer.ea.com/display/blaze/HTTP+Services+Wrapper#HTTPServicesWrapper-Cache-Control).

> You can now split the bulk build by directories by passing blazeserver.SplitBulkBuildDir as true. Default is false to keep the current behavior.  

> Async DB connections (introduced in Shrike.0) are now enabled by default. They can still be disabled per db connection pool by setting asyncDbConns = false on the pool's master DbConnConfig in framework.databaseConfig.databaseConnections. For more details, please visit [Blaze FAQs - How To Use The Database](https://developer.ea.com/display/blaze/How+To+Use+The+Database).

GameBrowser
------------
> [NOBUG] Blaze support for player reputation has been removed for Blaze deployments other than single-platform clusters hosting exclusively Xbox One players. This means that a player's reputation value will be disregarded when determining which games are a match for a GameBrowser list.

GameManager
------------
> [GOSREQ-2897](https://eadpjira.ea.com/browse/GOSREQ-2897) - Blaze now allows clients to create empty game groups without joining the caller as part of the player roster. To do this, specify the CreateGameRequest.gameCreationData.isExternalOwner flag to be true. When true, the game's creator will be omitted from the player roster, and the game group persists as long as the creator's user session exists. Please see https://developer.ea.com/display/blaze/Game+Groups for details.

> [NOBUG] Blaze support for player reputation has been removed for Blaze deployments other than single-platform clusters hosting exclusively Xbox One players. This means that a player's reputation value will be disregarded when determining if a player may or may not join a game if the Blaze cluster hosts non-Xbox One players.

> [NOBUG] Sample create game template configs have been relocated to etc/gamemanager/example/... and the sample contents are now #included in the base configuration file. This should allow for easier integrations going forward, as changes to sample/example configurations won't create differences in the main file. Disabling the sample templates should be achieved by commenting out the #include(s) in etc/gamemanager/create_game_templates.cfg rather than editing the file(s) directly in etc/gamemanager/example/...

Matchmaker
-------------

> [GOSREQ-2978](https://eadpjira.ea.com/browse/GOSREQ-2978) - Blaze: Resolve 3v3/4v4 MM issues for NHL 20 Beta *Since Shrike.2*

> [NOBUG] Blaze support for player reputation has been removed for Blaze deployments other than single-platform clusters hosting exclusively Xbox One players. This means that a player's reputation value will be disregarded when determining which games or players are considered a match by the Blaze Matchmaker.

> [NOBUG] Sample matchmaking scenario configs have been relocated to etc/gamemanager/example/... and the sample contents are now #included in the base configuration file. This should allow for easier integrations going forward, as changes to sample/example configurations won't create differences in the main file. Disabling the sample scenarios and rules should be achieved by commenting out the #include statements in etc/gamemanager/scenarios.cfg rather than editing the files directly in etc/gamemanager/example/...

Util
------------

---

GDPR Compliance
------------
> [GOSREQ-2322](https://eadpjira.ea.com/browse/GOSREQ-2322) - Periodically delete expired GM connection metrics (which may contain IP info) and users for log audit (which may contain device ID info).  Data affected here is independent of the User Data Pull/Delete functionality. *Since Shrike.1*


Inclusions/Dependencies
=======================
---


The default dependencies for packages and libraries used in this release of Blaze SDK are as follows:

Blaze SDK Dependencies
----------------------
```
ActivePython                     2.7.13.2716
android_config                   4.00.00
AndroidEmulator                  1.18.00
AndroidNDK                       r13b
AndroidSDK                       24.4.1-1
antlr                            3.5.2
ApacheAnt                        1.9.7
BlazeSDKSamplesLibPack           1.0.0
capilano_config                  2.07.02
CapilanoSDK                      180706-proxy
ConsoleRemoteServer              0.03.02
coreallocator                    1.04.02
DirtySDK                         15.1.6.0.0
DotNet                           4.0-2
DotNetSDK                        2.0-4
Doxygen                          1.8.14-1
EAAssert                         1.05.07
EABase                           2.09.02
EACallstack                      2.01.02
eaconfig                         5.14.04
EAController                     1.05.01
EAControllerUserPairing          1.08.02
EACrypto                         1.03.01
EAIO                             2.23.03
EAJson                           1.06.05
EAMessage                        2.10.03
EAStdC                           1.26.00
EASTL                            3.12.01
EASystemEventMessageDispatcher   1.04.03
EATDF                            2.29.00
EAThread                         1.32.01
EATrace                          2.12.03
EAUser                           1.05.01
GraphViz                         2.38.0
IEAController                    1.02.03
IEASystemEventMessageDispatcher  1.00.05
IEAUser                          1.03.00
IncrediBuild                     3.61.1.1243
ios_config                       1.09.00
iphonesdk                        10.0-proxy-1
jdk                              1.8.0_112-pc64
kettle_config                    2.11.00
kettlesdk                        6.008.001-proxy
nantToVSTools                    3.19.09
NucleusSDK                       19.2.1.0.0-pr
nx_config                        1.00.04
nxsdk                            7.2.1
opus                             1.00.01
PcLint                           9.00k-nonproxy
pclintconfig                     0.17.02
PinTaxonomySDK                   2.1.0
PlayStation4AudioInEx            5.508.001
PPMalloc                         1.26.00
PyroSDK                          15.1.1.9.0
Speex                            1.2rc1-4
TelemetrySDK                     15.1.0
TypeDatabase                     1.04.02
UnixClang                        4.0.1
userdoc                          1.00.00
UTFXml                           3.08.09
VisualStudio                     15.4.27004.2002-2-proxy
vstomaketools                    2.06.09
WindowsSDK                       10.0.14393-proxy
XcodeProjectizer                 3.04.00
zlib                             1.2.11
```

Blaze Server Dependencies
-------------------------
```
ActivePython            2.7.13.2716-blaze-1
antlr                   3.5.2
bison                   2.1
blazeserver_grpc_tools  15.1.1.9.0
c_ares                  container-proxy
centos7_base            flattened
coreallocator           1.04.02
EAAssert                1.05.07
EABase                  2.08.04
eaconfig                5.14.04
EAIO                    2.23.01
EAJson                  1.06.05
EARunner                1.09.01
EAStdC                  1.23.00
EASTL                   3.12.01
EATDF                   2.29.00
EATDF_Java              1.09.00
EAThread                1.32.06
flex                    2.5.4a
GamePacker              1.03.04
google_coredumper       container-proxy
google_perftools        container-proxy
gtest                   1.8.0.ea.10
hiredis                 0.14.0-custom-1
IncrediBuild            3.61.1.1243
jdk                     1.7.0_67-1-pc
libcurl                 7.62.0-ssh2
libopenssl              1.0.2p-prebuilt
libssh2                 1.8.0-pc64
lua                     5.3.1.04
mariadb                 3.0.6
nantToVSTools           3.19.09
PcLint                  9.00k-nonproxy
pclintconfig            0.17.02
PPMalloc                1.26.00
ptmalloc3               1.0-1-custom
UnixClang               4.0.1
UTFXml                  3.08.09
VisualStudio            15.4.27004.2002-2-proxy
vstomaketools           2.06.09
WindowsSDK              10.0.14393-proxy
zlib                    1.2.11
```

These are the versions used in the available pre-compiled libs for Blaze SDK. If needed, the SDK can be built for older packages/libraries by modifying the masterconfig.xml file. Building using newer packages/libraries may not work depending on the changes in the newer versions; check with GOS if you are trying to build with newer versions and having problems.



---
Blaze 
=====
---

Release Info
-------------

* Version: 15.1.1.8.0
* Date: January 23, 2019
* [Resolved Issues]( https://eadpjira.ea.com/issues/?jql=status%20in%20(Completed%2C%20%22LOW%20RISK%22)%20AND%20fixVersion%20%3D%20Shrike.0 )
* [2014 Spring --> 2015 Winter UPGRADE GUIDE]( https://developer.ea.com/display/blaze/Blaze+2015+Winter+1.x+Continuous+Feature+Upgrade+Guide )

Blaze Documentation
-------------------

* [Getting Started with Blaze](https://developer.ea.com/display/blaze/Getting+Started)
* [Blaze Customer Portal](https://developer.ea.com/display/blaze/Blaze+Customer+Portal)
* [Blaze Technical Designs](https://developer.ea.com/display/TEAMS/Blaze)

Compatibility
-------------
This release's server has been tested with the following clients (in addition to this release's client):
TBD

Known Issues
------------
None


---
Needs Action
============
---

Framework
------------
> In an effort to reduce coupling between custom components and core Blaze components, the dependencies of Framework on core components have been removed. For expected build impact on custom components 
and any other tech built using Blaze Framework, see https://developer.ea.com/display/TEAMS/Framework+Dependency+Isolation+-+Build+changes+in+Shrike.x 

> [GOS-31556](https://eadpjira.ea.com/browse/GOS-31556) - UserSessionManager::getReputationServiceUtil() and ReputationServiceUtilFactory::getReputationServiceUtil() now return a ReputationServiceUtilPtr 
instead of a ReputationServiceUtil&.

> [GOS-31542](https://eadpjira.ea.com/browse/GOS-31542) - The EADP Data team has requested that custom PIN events sent from the Blaze server use the 'server' event type. If the event type field hasn't been
set, Blaze will set it to 'server_blaze', which can result in skewed PIN data.

> [GOS-31669](https://eadpjira.ea.com/browse/GOS-31669) - For efficiency, Blaze Server's metrics.cfg has been updated to reduce the amount of OI metrics exported by default. For details see the updated 
metrics.cfg metricsLoggingConfig.operationalInsightsExport.exclude section.

GameManager
------------
> [GOS-31657](https://eadpjira.ea.com/browse/GOS-31657) - Non-Scenario based matchmaking has been removed from the BlazeSDK and BlazeServer.  
> The GameManager startMatchmaking() and cancelMatchmaking() RPCs have been removed.
> The following classes and callbacks have been removed:  (Old non-Scenarios (Session) version --> Scenarios Alternative)
>  Blaze::GameManager::MatchmakingResultsHelper           --> Blaze::GameManager::MatchmakingScenario
>  Blaze::GameManager::MatchmakingSession                 --> Blaze::GameManager::MatchmakingScenario
>  GameManagerAPIListener::onMatchmakingAsyncStatus()     --> GameManagerAPIListener::onMatchmakingScenarioAsyncStatus()
>  GameManagerAPIListener::onMatchmakingSessionFinished() --> GameManagerAPIListener::onMatchmakingScenarioFinished()
>  StartMatchmakingSessionCb                              --> StartMatchmakingScenarioCb
>  cancelSession()                                        --> cancelScenario()

> Non-Templated CreateGame calls are now [DEPRECATED].  The RPCs still exist, but will be removed in a future version.  Please update your code to use CreateGameTemplates. 


---
New Features
============
---

BlazeSDK
------------
> [GOS-31772](https://eadpjira.ea.com/browse/GOS-31772) - BlazeSDK now includes profiling support hooks via the Debug::setPerfHooks() function.  This allows teams to add a start and stop callback, which will be triggered by the various parts of the BlazeHub::idle process.  All callbacks, jobs, scheduled functions, dispatches, and idlers have been named as part of this update, to make debugging easier.

Framework
------------
> [GOSREQ-1983](https://eadpjira.ea.com/browse/GOSREQ-1983) - "includeLocation" and "encodeLocation" now default to true, meaning that file and line numbers are encoded in Blaze logs by default.

> [GOSREQ-1955](https://eadpjira.ea.com/browse/GOSREQ-1955), [GOSREQ-2437](https://eadpjira.ea.com/browse/GOSREQ-2437), [GOSREQ-2626](https://eadpjira.ea.com/browse/GOSREQ-2626) -
* Added support for authenticating a stateless blaze component/service via a Nucleus/Identity access token. The support is only via grpc endpoint on services using 'grpcOnly' attribute. For an example, 
see */blazeserver/component/greeter*. 
* Added support for controlling authorization of a service via oauth scopes.  
    * Note that stateless grpc services are considered **experimental** until we establish concrete authorization policies around it. As a result, do **NOT** write/deploy services in prod that leverage 
this feature yet.
* Added support for detecting introduction of undesired dependencies on Framework via **depchecker** tool/module.
* Added support for **blazeserver.lite** nant property to allow building the lite version of Blaze from the regular 'blazeserver' directory. It is recommended that you use the "build_lite" as a build artifact
directory rather than "build" in order to avoid build conflicts between lite and regular version.
* Please refer to the documentation at https://developer.ea.com/display/TEAMS/Custom+Component+Separation for these features. 

> [GOSREQ-2390](https://eadpjira.ea.com/browse/GOSREQ-2390) - Address Sanitizer (ASan) support has been added to Blaze server. It can be used instead of Valgrind for the memory testing purpose. 
For details, please visit [Clang Sanitizers](https://developer.ea.com/display/blaze/Using+Clang+Sanitizers+in+Blaze).

> [GOSREQ-2497](https://eadpjira.ea.com/browse/GOSREQ-2497) - Blazeserver db connections can now use the MariaDB nonblocking client API instead of making blocking calls and using db threads. This feature is off by default and can be enabled per db connection pool by setting asyncDbConns = true on the pool's master DbConnConfig in framework.databaseConfig.databaseConnections. (Note that slave connections will always use the master's asyncDbConns setting.) The setting is not reconfigurable and changing it requires a rolling restart.

> [GOSREQ-2561](https://eadpjira.ea.com/browse/GOSREQ-2561) - The blazeserver now uses the EADP Cloud PIN forwarder to submit PIN events. (PIN events are no longer submitted to River by the blazeserver directly, but are instead logged to file and sent to River via logstash.) To support this, a new event category, "pinevent", has been added to logging.cfg.

> [GOSREQ-2583](https://eadpjira.ea.com/browse/GOSREQ-2583) - On Windows, the blazeserver and BlazeSDK will now build in VS 2017 by default. If you would like to use VS 2015 instead, add -D:vsversion=2015 to your nant build options.

> [GOS-31537](https://eadpjira.ea.com/browse/GOS-31537) - When enabling proto generation for custom components, a blacklist of custom components can now be specified.

> [GOS-31802](https://eadpjira.ea.com/browse/GOS-31802) -
* Any protos added to the `/blazeserver/outboundprotos` directory will be generated. Currently, it was limited to `outboundprotos/eadp` directory. This allows protos to be added for non-eadp services 
(including your custom services).
* Added support for passing any additional proto directories to inbound and outbound proto code generation. The integrators can use new line separated directories in `${blazeserver.protoinbound.additional_paths.custom}` 
and `${blazeserver.protooutbound.additional_paths.custom}` properties respectively (like you currently do for custom tdf include directories).

> [GOS-31690](https://eadpjira.ea.com/browse/GOS-31690) - messaging component can now be configured to use EADP Social Profanity Filtering Service. For more details, please visit 
[Blaze Profanity Filtering Using Social Profanity Service](https://developer.ea.com/display/blaze/Profanity+Filtering#ProfanityFiltering-ProfanityfilteringusingEADPSocialProfanityFilterService).

> Added unit tests for custom protobuf serialization/deserialization code.

> Added sample tests to show usage of gMock.

> [GOSREQ-2495](https://eadpjira.ea.com/browse/GOSREQ-2495) - To support future transitioning of Blaze to rely solely on Protobuf and GRPC for its code generation, Blaze Server's internal build scripts, and the blazeserver_grpc_tools, and EATDF packages, have been updated, allowing Blaze Server to now auto-generate equivalent .protos, from its components' existing .tdfs/.rpcs. This is done automatically under the hood, as part of Blaze Server's regular build process. To ensure Blaze Server builds, titles should ensure to sync the latest blazeserver_grpc_tools and EATDF updates. Note: To ensure Blaze Server builds properly, ensure to also sync the latest blazeserver_grpc_tools, and EATDF packages. For details, reference Blaze GRPC and 'Port Existing TDF and RPC Files To Proto Files' documentation: https://developer.ea.com/display/TEAMS/Part+1+-+Port+Existing+TDF+and+RPC+Files+To+Proto+Files.

> [GOSREQ-2546](https://eadpjira.ea.com/browse/GOSREQ-2546) - To allow titles/Ops to easily identify and deal with server instances or components that may be stalled or unable to complete reconfiguration, when a reconfigure request gets rejected due to server instances being still (potentially stuck) in a reconfiguring state from a prior request, the error response contains a new mPendingInstancesMap member, which gets filled with info on the culprit server instances. Additionally, the reconfigure request includes a new mForceReconfigure flag, which when set to true, causes Blaze to stop waiting for reconfiguration updates from server instances currently in a still reconfiguring state, to allow this request (and subsequent requests with mForceReconfigure true) to run (without getting a CTRL_ERROR_RECONFIGURE_IN_PROGRESS error). For details, see Blaze 'Reconfiguration Separation' documentation: https://developer.ea.com/display/TEAMS/Reconfiguration+Separation

GameBrowser
------------
> [GOS-31369](https://eadpjira.ea.com/browse/GOS-31369) - ListGameData TDF contains a new member, mQueueRoster, containing the full roster of the game queue. This enables the getFullGameData RPC to supply information about the player queue.
> SearchSlave metrics now include detailed information on the status of the GameBrowser.

GameManager
------------
> New create game template parameter in the 'gameCreationData' block, 'skipInitializing', allows a hostless, NETWORK_DISABLED GameSession or GameGroup to skip directly to the PRE_GAME/GAME_GROUP_INITIALIZED state rather than requiring an admin member to advance it from the INITIALIZING state. For this flag to work, 'assignTopologyHostForNetworklessGameSessions' in gamesession.cfg must be set to false. 

> Audit logging has undergone a major overhaul. Audit logging now supports additional fields; IP address and deviceId. Setting audits via the usersessions.cfg file is now deprecated. Users can still be added to the audit list via the config file, but they can no longer be removed via config file. The UserSessionManager RPCs for enabling/disabling audits for users as well as fetching user audit state have been deprecated. Generic enable/disable/fetch audit RPCs have been added to the Blaze controller (and are marked internal). Audits can now be set in a combinitoric manner. For instance, an audit could specify a blazeId, or it could be a blazeId from a specific IP (or IP range), or both could be specified (in which case, two audit files would be generated). All audits now persist across server restarts. Random sample auditing has been removed as this feature was not being used. For more information, please read the audit logging section at https://developer.ea.com/display/blaze/How+To+Use+Logging#HowToUseLogging-AuditLogging

> GameManager can now disregard reserved players when reporting 1st party session fullness for titles that create reservations for invited players on PS4. externalSessionFullnessExcludesReservations being set to 'true' (default 'false') causes sessions for the game types listed in externalSessionFullnessExcludesReservationsGameTypes to ignore reservations when checking if a game is full for 1st party reporting purposes. Since XBL MPSD sessions do handle reservations, this configuration option should ONLY be enabled for PS4 Blaze instances.

> [GOSREQ-2547](https://eadpjira.ea.com/browse/GOSREQ-2547) - CCS pool can now be configured per game mode. If a CCS pool isn't configured for a game mode, the default pool defined in gamemanager.cfg will be used. An API, GameBase::getCCSPool(), has been added to the BlazeSDK to fetch the CCS pool used for a game. Please see https://developer.ea.com/display/blaze/Configuring+Connection+Concierge for details.

> [GOSREQ-2652](https://eadpjira.ea.com/browse/GOSREQ-2652) - To allow clients to custom handle players, based on Matchmaking Scenario they joined the game via, title clients can now access the Scenario via the GameManagerAPI's Player::getScenarioName() method.

> [GOSREQ-2537](https://eadpjira.ea.com/browse/GOSREQ-2537) - Blaze now supports gRPC services as external data sources. As part of this work, the HTTP services config section in externaldatasources.cfg has been merged with the HTTP services defined in framework.cfg. Additionally, HTTP services can now be limited based on what local components are loaded on an instance. For details on using gRPC services as external data sources, see https://developer.ea.com/display/blaze/External+Data+Sources#ExternalDataSources-ExternalgRPCService.

Matchmaker
-------------
> If a create game template referenced in matchmaking defines a non-zero number of SLOT_PRIVATE_PARTICIPANT seats in its configuration, the resulting game will have private slots. No users participating in matchmaking will be placed into private slots. The TotalPlayerSlots rule's values only SET the number of public participant slots for a game in create game matchmaking. Games created via matchmaking with private slots will always have those slots unoccupied.

> Use of Create and Find mode for a Matchmaking Subsession has been deprecated, in favor of having a Scenario request use multiple Subsessions that each specify only one of Create or Find Mode. Future releases may remove ability to specify both Create and Find mode in the same Subsession.
> To simplify features, use of Dual Create and Find mode for a Matchmaking Subsession is not supported, when using new Modernized Matchmaking's Property Filters or Game Quality Factors. Attempting to start or configure Matchmaking sessions/Scenarios with these features and Dual Mode enabled and will fail, returning appropriate errors. Titles should use multiple individual Create, or else Find mode Subsessions instead.

> Player filtering rules are now implemented to use Rete for find game matchmaking and game browser requests. This new implementation is enabled by default, and can be disabled by setting 'runPlayerRulesInRete' to false in matchmaker_settings.cfg

Util
------------
> [GOSREQ-2605](https://eadpjira.ea.com/browse/GOSREQ-2605) - Added an rpc which by default will send tts/stt metrics to the blazeserver every 60 sec.  The rate can be configured with setting clientUserMetricsUpdateRate in util.cfg, set to 0 to disable.  These metrics are currently only available in the binary logs produced by the blazeserver.

---
Inclusions/Dependencies
=======================
---


The default dependencies for packages and libraries used in this release of Blaze SDK are as follows:

Blaze SDK Dependencies
----------------------
```
ActivePython                     2.7.13.2716
android_config                   4.00.00
AndroidEmulator                  1.18.00
AndroidNDK                       r13b
AndroidSDK                       24.4.1-1
antlr                            3.5.2
ApacheAnt                        1.9.7
BlazeSDKSamplesLibPack           1.0.0
capilano_config                  2.07.02
CapilanoSDK                      180702-proxy
coreallocator                    1.04.02
DirtySDK                         15.1.5.0.0
DotNet                           4.0-2
DotNetSDK                        2.0-4
Doxygen                          1.8.14-1
EAAssert                         1.05.07
EABase                           2.08.04
EACallstack                      2.01.02
eaconfig                         5.14.04
EAController                     1.04.06
EAControllerUserPairing          1.07.01
EACrypto                         1.03.00
EAIO                             2.23.01
EAJson                           1.06.05
EAMessage                        2.10.03
EAStdC                           1.23.00
EASTL                            3.12.01
EASystemEventMessageDispatcher   1.04.01
EATDF                            2.26.00
EAThread                         1.32.01
EAUser                           1.04.06
GraphViz                         2.38.0
IEAController                    1.02.03
IEASystemEventMessageDispatcher  1.00.04
IEAUser                          1.02.03
IncrediBuild                     3.61.1.1243
ios_config                       1.09.00
iphonesdk                        10.0-proxy-1
jdk                              1.8.0_112-pc64
kettle_config                    2.11.00
kettlesdk                        6.008.001-proxy
nantToVSTools                    3.19.09
opus                             1.00.00
PcLint                           9.00k-nonproxy
pclintconfig                     0.17.02
PinTaxonomySDK                   2.0.0
PlayStation4AudioInEx            5.508.001
PPMalloc                         1.26.00
PyroSDK                          15.1.1.8.0
Speex                            1.2rc1-3
TelemetrySDK                     15.0.0
TypeDatabase                     1.04.02
UnixClang                        4.0.1
userdoc                          1.00.00
UTFXml                           3.08.09
VisualStudio                     15.4.27004.2002-2-proxy
vstomaketools                    2.06.09
WindowsSDK                       10.0.14393-proxy
XcodeProjectizer                 3.04.00
zlib                             1.2.11
```

Blaze Server Dependencies
-------------------------
```
ActivePython            2.7.13.2716
antlr                   3.5.2
bison                   2.1
blazeserver_grpc_tools  15.1.1.8.0
c_ares                  container-proxy
centos7_base            flattened
coreallocator           1.04.02
EAAssert                1.05.07
EABase                  2.08.04
eaconfig                5.14.04
EAIO                    2.23.01
EAJson                  1.06.05
EARunner                1.09.01
EAStdC                  1.23.00
EASTL                   3.12.01
EATDF                   2.26.00
EATDF_Java              1.09.00
EAThread                1.32.01
flex                    2.5.4a
GamePacker              1.03.00
google_coredumper       container-proxy
google_perftools        container-proxy
gtest                   1.8.0.ea.10
hiredis                 0.14.0-custom-1
IncrediBuild            3.61.1.1243
jdk                     1.7.0_67-1-pc
libcurl                 7.62.0-ssh2
libopenssl              1.0.2p-prebuilt
libssh2                 1.8.0-pc64
lua                     5.3.1.01
mariadb                 3.0.6
nantToVSTools           3.19.09
PcLint                  9.00k-nonproxy
pclintconfig            0.17.02
PPMalloc                1.26.00
ptmalloc3               1.0-1-custom
UnixClang               4.0.1
UTFXml                  3.08.09
VisualStudio            15.4.27004.2002-2-proxy
vstomaketools           2.06.09
WindowsSDK              10.0.14393-proxy
zlib                    1.2.11
```

These are the versions used in the available pre-compiled libs for Blaze SDK. If needed, the SDK can be built for older packages/libraries by modifying the masterconfig.xml file. Building using newer packages/libraries may not work depending on the changes in the newer versions; check with GOS if you are trying to build with newer versions and having problems.


