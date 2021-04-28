/*H********************************************************************************/
/*!
    \File voipserverconfig.c

    \Description
        This module handles loading and parsing the configuration for
        the voipserver

    \Copyright
        Copyright (c) 2015 Electronic Arts Inc.

    \Version 09/17/2015 (eesponda)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>
#include <stdio.h>

#include "DirtySDK/platform.h"

#if defined(DIRTYCODE_LINUX)
  #include <ifaddrs.h>
  #include <net/if.h>
#endif

#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/protossl.h"

#include "configfile.h"
#include "dirtycast.h"
#include "ipmask.h"
#include "voipserverconfig.h"

/*** Defines **********************************************************************/

//! default metrics pushing rate
#define VOIPSERVER_CONFIG_METRICSAGGR_DEFAULT_RATE  (300*1000)  // every 5 min

//! final metrics pushing rate (used when server is about to explicitly exit - best effort to increase chance of final metrics snapshot to make it to the aggregator)
#define VOIPSERVER_CONFIG_METRICSAGGR_FINAL_RATE  (3*1000)  // every 3 sec

//! default host we use for stastd reporting
#define VOIPSERVER_CONFIG_METRICSAGGR_DEFAULT_HOST  ("127.0.0.1")

//! default host we use for subspace configuration
#define VOIPSERVER_CONFIG_SUBSPACE_DEFAULT_HOST     ("127.0.0.1")

//! nucleus endpoints by environment
#define VOIPSERVER_CONFIG_NUCLEUS_ENDPOINT_PROD     ("https://accounts2s.ea.com")
#define VOIPSERVER_CONFIG_NUCLEUS_ENDPOINT_INT      ("https://accounts2s.int.ea.com")

/*** Type Definitions *************************************************************/

/*** Private functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _VoipServerLoadMonitorConfigEntry

    \Description
        Load a monitor config entry.

    \Input *pCommandTags    - command-line tag buffer
    \Input *pConfigTags     - config file tag buffer
    \Input *pTagName        - tagfield entry to get
    \Input *pWrnVal         - [out] warning threshold
    \Input *pErrVal         - [out] error threshold
    \Input *pDefVal         - default value

    \Version 08/17/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerLoadMonitorConfigEntry(const char *pCommandTags, const char *pConfigTags, const char *pTagName, uint32_t *pWrnVal, uint32_t *pErrVal, const char *pDefVal)
{
    char strTemp[32];
    DirtyCastConfigGetString(pCommandTags, pConfigTags, pTagName, strTemp, sizeof(strTemp), pDefVal);
    if (sscanf(strTemp, "%u,%u", pWrnVal, pErrVal) != 2)
    {
        DirtyCastLogPrintf("voipserverconfig: could not read %s setting from config file\n", pTagName);
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipServerConfigGetFrontAddr

    \Description
        Gets the frontend address for the voipserver

    \Input *pCommandTags    - command-line tag buffer
    \Input *pConfigTags     - config file tag buffer

    \Output
        uint32_t            - address found

    \Notes
        On Linux: Will try to use the ip mask specified in the configuration. If that
        is not available it will either select an address depending on the
        address space (prefers public addresses)

        On PC: It will just return the local address, if we need anything in the
        future we can revisit this implementation

    \Version 04/20/2016 (eesponda)
*/
/********************************************************************************F*/
static uint32_t _VoipServerConfigGetFrontAddr(const char *pCommandTags, const char *pConfigTags)
{
#if defined(DIRTYCODE_LINUX)
    IPMaskT Mask;
    uint32_t uAddr = 0;
    struct ifaddrs *pInterfaces, *pCurrentInterface;

    if (getifaddrs(&pInterfaces) != 0)
    {
        DirtyCastLogPrintf("voipserverconfig: cannot query interfaces, using default %a\n", SocketGetLocalAddr());
        return(SocketGetLocalAddr());
    }
    IPMaskInit(&Mask, "voipserver.", "frontinterface", "", pCommandTags, pConfigTags, FALSE);

    /* loop through the interfaces to try to find the address that matches our mask
       if a mask has not been specified either grab a public address or private address (prefer public) */
    for (pCurrentInterface = pInterfaces; pCurrentInterface != NULL; pCurrentInterface = pCurrentInterface->ifa_next)
    {
        if ((pCurrentInterface->ifa_addr->sa_family == AF_INET) && ((pCurrentInterface->ifa_flags & (IFF_LOOPBACK | IFF_UP)) == IFF_UP))
        {
            const uint32_t uCurrentAddr = SockaddrInGetAddr(pCurrentInterface->ifa_addr);
            if (IPMaskMatch(&Mask, uCurrentAddr) == TRUE)
            {
                uAddr = uCurrentAddr;
                break;
            }
            // if in a private address space space (10.0.0.0/8 or 192.168.0.0/16) and nothing else found
            if (((uCurrentAddr & 0xff000000) == 0x0a000000) || ((uCurrentAddr & 0xffff0000) == 0xc0a80000) || ((uCurrentAddr & 0xfff00000) == 0xac100000))
            {
                if (uAddr == 0)
                {
                    uAddr = uCurrentAddr;
                }
            }
            // always use a public address
            else
            {
                uAddr = uCurrentAddr;
            }
        }
    }

    // cleanup and return result
    freeifaddrs(pInterfaces);
    return(uAddr);
#elif defined(DIRTYCODE_PC)
    // we just return the local address
    return(SocketGetLocalAddr());
#else
    #error "Unsupported platform"
#endif
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function VoipServerLoadConfigBegin

    \Description
        Starting loading the configuration file

    \Input **pConfig        - [out] config file module, gets created by the module
    \Input *pConfigTagsFile - full path of config file

    \Output
        int32_t             - negative=failure, else zero

    \Version 11/29/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipServerLoadConfigBegin(ConfigFileT **pConfig, const char *pConfigTagsFile)
{
    // if a previous config instance exists, kill it
    if (*pConfig != NULL)
    {
        ConfigFileDestroy(*pConfig);
        *pConfig = NULL;
    }

    // create config file instance
    if ((*pConfig = ConfigFileCreate()) == NULL)
    {
        DirtyCastLogPrintf("voipserverconfig: unable to create config file instance\n");
        return(-1);
    }

    // load the config file
    DirtyCastLogPrintf("voipserverconfig: loading config file %s\n", pConfigTagsFile);
    ConfigFileLoad(*pConfig, pConfigTagsFile, "");

    return(0);
}

/*F********************************************************************************/
/*!
    \Function VoipServerLoadConfigComplete

    \Description
        Check for config load completion.

    \Input *pConfig         - config file module ref
    \Input **ppConfigTags   - [out] parsed config file data

    \Output
        int32_t             - negative=failure, zero=pending, else success

    \Version 11/30/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipServerLoadConfigComplete(ConfigFileT *pConfig, const char **ppConfigTags)
{
    // update config file loading
    ConfigFileUpdate(pConfig);

    // if load is pending, return pending status
    if (ConfigFileStatus(pConfig) == CONFIGFILE_STAT_BUSY)
    {
        return(0);
    }

    // make sure we got a config file
    *ppConfigTags = ConfigFileData(pConfig, "");
    if ((*ppConfigTags == NULL) || (**ppConfigTags == '\0'))
    {
        DirtyCastLogPrintf("voipserverconfig: error loading config file (%s)\n", ConfigFileError(pConfig));
        return(-1);
    }

    return(1);
}

/*F********************************************************************************/
/*!
    \Function VoipServerSetConfigVars

    \Description
        Load the config into variables.

    \Input *pConfig         - [out] config file data
    \Input *pCommandTags    - command-line tagbuffer
    \Input *pConfigTags     - command-file tagbuffer

    \Version 05/02/2012 (cvienneau)
*/
/********************************************************************************F*/
void VoipServerSetConfigVars(VoipServerConfigT *pConfig, const char *pCommandTags, const char *pConfigTags)
{
    //for tunnel port map parsing
    int32_t iMaxGameSize;
    char strTag[32];
    char strFrontAddr[32];
    uint32_t uConfiguredApiPort;
    const char *pDefault;

    // environment
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "voipserver.environment", pConfig->strEnvironment, sizeof(pConfig->strEnvironment), "prod");

    // get the front-end address
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "voipserver.frontaddr", strFrontAddr, sizeof(strFrontAddr), "invalid");
    if (strcmp("invalid", strFrontAddr) == 0)
    {
        // find out the front-end address from the interfaces on the box
        pConfig->uFrontAddr = _VoipServerConfigGetFrontAddr(pCommandTags, pConfigTags);
    }
    else
    {
        // use the specified front end address
        pConfig->uFrontAddr = SocketInTextGetAddr(strFrontAddr);
    }

    // get diagnostic port
    pConfig->uDiagnosticPort = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.diagnosticport", 0);

    // get external metrics aggregator settings
    pConfig->uMetricsAggregatorPort = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.metricsaggregator.port", 0);   // when port is 0, metrics pushing is disabled
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "voipserver.metricsaggregator.addr", pConfig->strMetricsAggregatorHost, sizeof(pConfig->strMetricsAggregatorHost), VOIPSERVER_CONFIG_METRICSAGGR_DEFAULT_HOST);
    pConfig->uMetricsPushingRate = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.metricsaggregator.rate", VOIPSERVER_CONFIG_METRICSAGGR_DEFAULT_RATE);
    pConfig->uMetricsFinalPushingRate = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.metricsaggregator.finalrate", VOIPSERVER_CONFIG_METRICSAGGR_FINAL_RATE);
    pConfig->eMetricsFormat = SERVERMETRICS_FORMAT_DATADOG;

    // get subspace sidekick app settings
    pConfig->uSubspaceSidekickPort = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.subspace.sidekick.port", 0);   // when port is 0, subspace sidekick is disabled
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "voipserver.subspace.sidekick.addr", pConfig->strSubspaceSidekickAddr, sizeof(pConfig->strSubspaceSidekickAddr), VOIPSERVER_CONFIG_SUBSPACE_DEFAULT_HOST);
    // get the local address information provided by the sidekick app (if not using SubspaceApi)
    pConfig->uSubspaceLocalPort = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.subspace.local.port", 0);   // when port is 0, subspace is disabled
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "voipserver.subspace.local.addr", pConfig->strSubspaceLocalAddr, sizeof(pConfig->strSubspaceLocalAddr), "");

    // port for for gameservers
    uConfiguredApiPort = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.port", 0);
    // alias
    uConfiguredApiPort = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.gameserverport", uConfiguredApiPort);
    uConfiguredApiPort = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.qosprobeport", uConfiguredApiPort);

    // tunnel port
    pConfig->uTunnelPort = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.localport", 4096);
    // alias
    pConfig->uTunnelPort = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.tunnelport", pConfig->uTunnelPort);

    // debug level
    pConfig->uDebugLevel = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.debuglevel", 1);

    // GS timeout
    pConfig->uPeerTimeout = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.gameservtimeout", 40) * 1000;
    // alias
    pConfig->uPeerTimeout = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.peertimeout", pConfig->uPeerTimeout);

    // clear tunnel configurations, before intializing them below
    ds_memclr(&pConfig->TunnelInfo, sizeof(pConfig->TunnelInfo));

    // using tunnel overrides
    pConfig->uTunnelPort = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.port", pConfig->uTunnelPort);

    // upd send/recv buffer size
    pConfig->uRecvBufSize = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.udprbufsize", 212992);
    pConfig->uSendBufSize = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.udpsbufsize", 212992);

    // the game port is flagged as auto-flush to ensure these packets have the minimal latency end-to-end
    pConfig->TunnelInfo.aRemotePortList[0] = VOIPSERVERCONFIG_GAMEPORT;
    pConfig->TunnelInfo.aPortFlags[0] = PROTOTUNNEL_PORTFLAG_ENCRYPTED|PROTOTUNNEL_PORTFLAG_AUTOFLUSH;

    // the voip port is not tagged as auto-flush as we want to try to bundle these with other packets for more 
    // efficient transfers, since these packets generally occur less frequently and are more tolerant to latency 
    // we allow them to wait on the server for up to the configured tunnel flush rate to increase the odds of 
    // packet bundling, see "voipserver.tunnelflushrate" to set this value
    pConfig->TunnelInfo.aRemotePortList[1] = VOIPSERVERCONFIG_VOIPPORT;
    pConfig->TunnelInfo.aPortFlags[1] = PROTOTUNNEL_PORTFLAG_ENCRYPTED;

    // use the config override if specified, otherwise voipserver.port is the tunnel port and the api port
    pConfig->uApiPort = uConfiguredApiPort != 0 ? uConfiguredApiPort : pConfig->uTunnelPort;

    // get duration of "final metrics report" phase  (default: 10s - assuming that flush rate is accelerated to once every 3 sec during that phase)
    pConfig->iFinalMetricsReportDuration = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.finalmetricsreportduration", 10000);

    // get voip throttling configuration
    pConfig->iMaxSimultaneousTalkers = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.maxsimultaneoustalkers", 4);
    pConfig->iTalkTimeOut = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.talktimeout", 750);

    // get latency bin configuration
    GameServerPacketReadLateBinConfig(&pConfig->LateBinCfg, pCommandTags, pConfigTags, "voipserver.latebins");

    // load monitor threshold levels
    VoipServerLoadMonitorConfiguration(pConfig, pCommandTags, pConfigTags);

    // what mode are we operating
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "voipserver.mode", strTag, sizeof(strTag), "gameserver");
    if (ds_stricmp(strTag, "gameserver") == 0)
    {
        pConfig->uMode = VOIPSERVER_MODE_GAMESERVER;
    }
    else if (ds_stricmp(strTag, "concierge") == 0)
    {
        pConfig->uMode = VOIPSERVER_MODE_CONCIERGE;
    }
    else if (ds_stricmp(strTag, "qosserver") == 0)
    {
        pConfig->uMode = VOIPSERVER_MODE_QOS_SERVER;
    }
    else
    {
        pConfig->uMode = (VoipServerModeE)DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.mode", VOIPSERVER_MODE_GAMESERVER);
    }

    /* -- common auto-scaling config -- */
    pConfig->iProcessStateMinDuration = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.processstateminduration", 120000);          // in ms. default = 120000 ms (2 min)
    pConfig->iCpuThresholdDetectionDelay = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.cputhresholddetectiondelay", 30000);    // in ms. default = 30000 ms (30 s)
    pConfig->iAutoScaleBufferConst = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.autoscale.bufferconst", 0);

    // total numbers of tunnels needed
    pConfig->iMaxClients = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.maxtunnels", 8);

    // maximum number of GS's for this VS
    pConfig->iMaxGameServers = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.maxservers", 16);
    // alias
    pConfig->iMaxGameServers = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.maxgameservers", pConfig->iMaxGameServers);


    if (pConfig->uMode == VOIPSERVER_MODE_CONCIERGE)
    {
        // in concierge mode we calculate our worst case needed for games in the voiptunnel based on expected game size (assumes 2 players for now)
        if ((iMaxGameSize = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.maxgamesize", 2)) != 0)
        {
            pConfig->iMaxGameServers = pConfig->iMaxClients / iMaxGameSize;
        }
    }
    else if (pConfig->uMode == VOIPSERVER_MODE_GAMESERVER)
    {
        #if defined(DIRTYCODE_PC)
        // this limitation only applies to how gameserver communication works
        if (pConfig->iMaxGameServers > (FD_SETSIZE-8))
        {
            DirtyCastLogPrintf("voipserverconfig: WARNING -- Win32 build supports a maximum of %d GameServers due to select() limitations\n", FD_SETSIZE-8);
            pConfig->iMaxGameServers = FD_SETSIZE-8;
        }
        #endif

        // maximum number of people in each GS
        if ((iMaxGameSize = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.maxgamesize", 0)) != 0)
        {
            // updated total number of tunnels needed
            pConfig->iMaxClients = iMaxGameSize * pConfig->iMaxGameServers;
        }
    }
    else if (pConfig->uMode == VOIPSERVER_MODE_QOS_SERVER)
    {
        // QOS capacity
        pConfig->iMaxClients = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.qosmaxclients", 10);
    }

    if (pConfig->uMode != VOIPSERVER_MODE_QOS_SERVER)
    {
        DirtyCastLogPrintf("voipserverconfig: configured capacity with maxtunnels=%d maxgameservers=%d\n", pConfig->iMaxClients, pConfig->iMaxGameServers);
    }
    else
    {
        DirtyCastLogPrintf("voipserverconfig: configured capacity with %d Qos Tests/sec.\n", pConfig->iMaxClients);
    }

    // s2s configuration
    if (pConfig->uMode == VOIPSERVER_MODE_GAMESERVER)
    {
        DirtyCastConfigGetString(pCommandTags, pConfigTags, "voipserver.certfilename", pConfig->strCertFilename, sizeof(pConfig->strCertFilename), "certs/default.crt");
        DirtyCastConfigGetString(pCommandTags, pConfigTags, "voipserver.keyfilename", pConfig->strKeyFilename, sizeof(pConfig->strKeyFilename), "certs/default.key");
    }
    else if (pConfig->uMode == VOIPSERVER_MODE_CONCIERGE)
    {
        DirtyCastConfigGetString(pCommandTags, pConfigTags, "voipserver.certfilename", pConfig->strCertFilename, sizeof(pConfig->strCertFilename), "certs/GS-DS-CCS.int.crt");
        DirtyCastConfigGetString(pCommandTags, pConfigTags, "voipserver.keyfilename", pConfig->strKeyFilename, sizeof(pConfig->strKeyFilename), "certs/GS-DS-CCS.int.key");
    }
    else if (pConfig->uMode == VOIPSERVER_MODE_QOS_SERVER)
    {
        DirtyCastConfigGetString(pCommandTags, pConfigTags, "voipserver.certfilename", pConfig->strCertFilename, sizeof(pConfig->strCertFilename), "certs/GS-DS-QOS.int.crt");
        DirtyCastConfigGetString(pCommandTags, pConfigTags, "voipserver.keyfilename", pConfig->strKeyFilename, sizeof(pConfig->strKeyFilename), "certs/GS-DS-QOS.int.key");
    }

    // pool / data center information for registering with the CCS
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "voipserver.pool", pConfig->strPool, sizeof(pConfig->strPool), "global");
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "voipserver.pingsite", pConfig->strPingSite, sizeof(pConfig->strPingSite), "");

    // hostname override for registering with CCS (required when running in the cloud where hostname id passed in from deployment infrastructure)
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "voipserver.hostname", pConfig->strHostName, sizeof(pConfig->strHostName), "");

    // CCS/Qos coordinator address
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "voipserver.ccsaddr", pConfig->strCoordinatorAddr, sizeof(pConfig->strCoordinatorAddr), "");
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "voipserver.qoscaddr", pConfig->strCoordinatorAddr, sizeof(pConfig->strCoordinatorAddr), pConfig->strCoordinatorAddr);

    // Qos secure key override, used for debug purposes
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "voipserver.overridesitekey", pConfig->strOverrideSiteKey, sizeof(pConfig->strOverrideSiteKey), "");

    // Nucleus server address
    if (ds_strnicmp(pConfig->strEnvironment, "prod", 4) == 0)
    {
        pDefault = VOIPSERVER_CONFIG_NUCLEUS_ENDPOINT_PROD;
    }
    else
    {
        pDefault = VOIPSERVER_CONFIG_NUCLEUS_ENDPOINT_INT;
    }
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "voipserver.nucleusaddr", pConfig->strNucleusAddr, sizeof(pConfig->strNucleusAddr), pDefault);

    /* Deployment Info  (defaults to "servulator" for non-EAcloud deployment; "kubernetes pod name" for EAcloud deployment)
       Deployment Info 2 (defaults to "servulator" for non-EAcloud deployment; "kubernetes namespace" for EAcloud deployment)
       Note: Because these config params are checked for early in DirtyCastLoggerCreate(), they are meant to be always passed as command line
       arguments, and never as entries in the config file. */
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "voipserver.deployinfo", pConfig->strDeployInfo, sizeof(pConfig->strDeployInfo), "servulator");
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "voipserver.deployinfo2", pConfig->strDeployInfo2, sizeof(pConfig->strDeployInfo2), "servulator");

    // SSL handshaking offload
    pConfig->bOffloadInboundSSL = (uint8_t)DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.offloadInboundSSL", 0);

    // auth token validation options
    pConfig->bAuthValidate = (uint8_t)DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.authvalidate", TRUE);

    // tunnel flush rate
    pConfig->uFlushRate = (uint32_t)DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.tunnelflushrate", 0);

    // jwt configuration
    pConfig->bJwtEnabled = (uint8_t)DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipserver.jwt", TRUE);
}

/*F********************************************************************************/
/*!
    \Function VoipServerLoadMonitorConfiguration

    \Description
        Load a monitor config entry.

    \Input *pConfig         - [out] configuration data
    \Input *pCommandTags    - command-line tag buffer
    \Input *pConfigTags     - config file tag buffer

    \Version 08/17/2007 (jbrookes)
*/
/********************************************************************************F*/
void VoipServerLoadMonitorConfiguration(VoipServerConfigT *pConfig, const char *pCommandTags, const char *pConfigTags)
{
    ServerMonitorConfigT *pWrn = &pConfig->MonitorWarnings;
    ServerMonitorConfigT *pErr = &pConfig->MonitorErrors;

    // get pct games avail
    _VoipServerLoadMonitorConfigEntry(pCommandTags, pConfigTags, "monitor.pctgamesavail", &pWrn->uPctGamesAvail, &pErr->uPctGamesAvail, "6,3");

    // get pct client slots
    _VoipServerLoadMonitorConfigEntry(pCommandTags, pConfigTags, "monitor.pctclientslot", &pWrn->uPctClientSlot, &pErr->uPctClientSlot, "6,3");

    // get avg client latency
    _VoipServerLoadMonitorConfigEntry(pCommandTags, pConfigTags, "monitor.avgclientlate", &pWrn->uAvgClientLate, &pErr->uAvgClientLate, "100,150");

    // get pct server load
    _VoipServerLoadMonitorConfigEntry(pCommandTags, pConfigTags, "monitor.pctserverload", &pWrn->uPctServerLoad, &pErr->uPctServerLoad, "75,85");

    // get avg system load
    _VoipServerLoadMonitorConfigEntry(pCommandTags, pConfigTags, "monitor.avgsystemload", &pWrn->uAvgSystemLoad, &pErr->uAvgSystemLoad, "4,8");

    // get failed game count
    _VoipServerLoadMonitorConfigEntry(pCommandTags, pConfigTags, "monitor.failedgamecnt", &pWrn->uFailedGameCnt, &pErr->uFailedGameCnt, "4,9");

    // get udp error rate
    _VoipServerLoadMonitorConfigEntry(pCommandTags, pConfigTags, "monitor.udperrorrate",  &pWrn->uUdpErrorRate,  &pErr->uUdpErrorRate,  "100000,150000");

    // get udp recv queue length
    _VoipServerLoadMonitorConfigEntry(pCommandTags, pConfigTags, "monitor.udprecvqlen",   &pWrn->uUdpRecvQueueLen, &pErr->uUdpRecvQueueLen, "100000,120000");

    // get udp send queue length
    _VoipServerLoadMonitorConfigEntry(pCommandTags, pConfigTags, "monitor.udpsendqlen",   &pWrn->uUdpSendQueueLen, &pErr->uUdpSendQueueLen, "100000,120000");
}
