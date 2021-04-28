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
