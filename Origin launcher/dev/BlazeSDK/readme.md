# Blaze Release Info

* Version: 15.1.1.10.2
* Date: April 20, 2021
* [Greenlight features]( https://eadpjira.ea.com/issues/?jql=status%20in%20(Completed)%20AND%20fixVersion%20%3D%20Urraca.2 )
* [Resolved Bugs ]( https://eadpjira.ea.com/issues/?jql=project%20%3D%20%22GOS%22%20and%20issuetype%20%3D%20%22Blaze%22%20and%20status%20%3D%20Closed%20and%20fixVersion%20%3D%20Urraca.2 )
* [2014 Spring --> 2015 Winter UPGRADE GUIDE]( https://developer.ea.com/display/blaze/Blaze+2015+Winter+1.x+Continuous+Feature+Upgrade+Guide )

# Blaze Documentation

* [Getting Started with Blaze]( https://developer.ea.com/display/blaze/Getting+Started )
* [Blaze Customer Portal]( https://developer.ea.com/display/blaze/Blaze+Customer+Portal )
* [Blaze Technical Designs]( https://developer.ea.com/display/TEAMS/Blaze )

# Compatibility

### Clients
* This release's server has been tested with the following clients (in addition to this release's client):
* 15.1.1.10.0 (Urraca.0)
* 15.1.1.9.6 (Trama.6)

# Known Issues
* TBD

# Needs Action

### General
* [GOS-32929](https://eadpjira.ea.com/browse/GOS-32929) - IP addresses for DEDICATED_SERVER_NETWORKS removed as we have moved to mTLS. IP addresses for WAL_PROXY2_NETWORKS removed as WAL is now running on RS4 version. If you continue to need these IP addresses, please reach out to GS Support. See [details](https://developer.ea.com/display/blaze/IP+addresses+removed+from+stock+config).
* [GOS-33368](https://eadpjira.ea.com/browse/GOS-33368) - Blaze gRPC endpoint and trusted gRPC client changes for Blaze's ServiceMesh integration. See the JIRA ticket for more details.
* [GOS-33385](https://eadpjira.ea.com/browse/GOS-33385) - Blaze now tracks UserInfo based on 3rd-party service (XBL/PSN), rather than platform, to avoid issues in shared clusters.  This requires the creation of _xbl and _psn Schemas to hold the new DB Tables used.  When migrating from Urraca.1, be aware that all UserInfo based data, like the crossplay opt-in, will be reset. 
* [GOSREQ-3869](https://eadpjira.ea.com/browse/GOSREQ-3869) - Four configuration items are removed from oauth.cfg: tokenFormat, minValidDurationForServerAccessToken, minValidDurationForUserSessionRefreshToken, minValidDurationForUserSessionAccessToken. If game client has customized oauth.cfg, please remove all the mentioned configuration items.

### PlaceHolderComponent
* TBD

# New Features

### General
* TBD

### Authentication
* [GOSREQ-3869](https://eadpjira.ea.com/browse/GOSREQ-3869) - Blaze now accepts JWT format access token for user login. Blaze server verifies JWT token authenticity and decodes it to obtain the user information needed for the login flow. When C&I Nucleus service is unavailable, users can still login to Blaze using JWT token cached on their consoles/PCs. Implementation of [Dual Token Support](https://developer.ea.com/display/CI/GOPFR-6143+%5BIdentity+Resilience%5D+Dual+Token+Strategy+-+JWT+token+across+EADP) in both core Blaze code (done) and custom component code (integrator to-do) is required for adopting JWT token login. Also, game client will be responsible for refreshing JWT token and sending new token to Blaze server. For technical details and integration guide of JWT token login, please refer to [Blaze JWT Support](https://developer.ea.com/display/blaze/JWT+Support).

### GameManager
* [GOSREQ-3970](https://eadpjira.ea.com/browse/GOSREQ-3970) - Blaze has been updated to support Microsoft's Large setting for creating Xbox MPSD sessions with max members > 100. To use the feature, see [documentation](https://developer.ea.com/pages/viewpage.action?pageId=310510981).
* [GOSREQ-3301](https://eadpjira.ea.com/browse/GOSREQ-3301) - GameBrowser functionality can now be accessed via Blaze's ServiceMesh integration. See [documentation](https://developer.ea.com/display/blaze/GameBrowser+Service) for more details.

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
CapilanoSDK                     180716-proxy
ConsoleRemoteServer             0.03.10
continuable                     4.0.0-5
coreallocator                   1.04.02
DirtySDK                        15.1.7.0.2
DotNet                          4.72
DotNetSDK                       2.0-4
Doxygen                         1.8.14-1
EAAssert                        1.05.07
EABase                          2.09.13
EACallstack                     2.02.01
EAController                    1.09.06
EAControllerUserPairing         1.12.15
EACrypto                        1.03.04
EadpAuthentication              1.9.0
EadpBrowserProvider             1.7.0
EadpFoundation                  1.11.0
EAIO                            3.01.11
EAJson                          1.06.05-maverick
EAMessage                       2.10.05
EAOrigin                        1.2.0
EAStdC                          1.26.07
EASTL                           3.17.06
EASystemEventMessageDispatcher  1.05.00
EATDF                           15.1.1.10.2
EAThread                        1.33.00
EATrace                         2.12.03
EAUser                          1.09.08
function2                       4.0.0-3
GDK                             201101-proxy
GradleWrapper                   1.00.00-pr1
GraphViz                        2.38.0
IncrediBuild                    3.61.1.1243
jdk                             azul-8.0.222-pc64
job_manager                     4.05.05
kettlesdk                       8.008.781-proxy
nantToVSTools                   3.19.09
NucleusSDK                      20.4.1.0.0
nx_config                       1.01.02
nx_vsi                          12.2.2.0-proxy
nxsdk                           12.2.0-proxy
opus                            1.00.06
PcLint                          9.00k-nonproxy
pclintconfig                    0.17.02
PinTaxonomySDK                  2.2.2
PlayStation4AppContentEx        8.008.011
PlayStation4AudioInEx           8.008.011
PPMalloc                        1.26.07
ProDG_VSI                       8.00.0.3-proxy
ps5sdk                          2.00.00.26-proxy
Ps5VSI                          2.00.0.6-proxy
PyroSDK                         15.1.1.9.3
rwmath                          3.01.01
Speex                           1.2rc1-8
StadiaSDK                       1.56-proxy
SteamSDK                        1.49.01
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
blazeserver_grpc_tools  15.1.1.10.2
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
EATDF                   15.1.1.10.2
EATDF_Java              1.09.00
EAThread                1.33.00
flex                    2.5.4a
GamePacker              1.04.02
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
