/*H********************************************************************************/
/*!
    \File voipserverconfig.h

    \Description
        This module handles loading and parsing the configuration for
        the voipserver

    \Copyright
        Copyright (c) 2015 Electronic Arts Inc.

    \Version 09/17/2015 (eesponda)
*/
/********************************************************************************H*/

#ifndef _voipserverconfig_h
#define _voipserverconfig_h

/*** Include files ****************************************************************/

#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/proto/prototunnel.h"
#include "DirtySDK/misc/qoscommon.h"
#include "gameserverpkt.h"
#include "servermetrics.h"

/*** Defines **********************************************************************/

//! used to configure prototunnel and reference when opening virtual sockets
#define VOIPSERVERCONFIG_VOIPPORT               (7777)
#define VOIPSERVERCONFIG_GAMEPORT               (7778)

#define VOIPSERVERCONFIG_ENV_SIZE               (16)
#define VOIPSERVERCONFIG_SITE_SIZE              (64)
#define VOIPSERVERCONFIG_POOL_SIZE              (64)
#define VOIPSERVERCONFIG_DEPLOYINFO_SIZE        (128)
#define VOIPSERVERCONFIG_DEPLOYINFO2_SIZE       (128)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! server monitor configuration
typedef struct ServerMonitorConfigT
{
    uint32_t uPctGamesAvail;        //!< monitor threshold for percentage of gaems available
    uint32_t uPctClientSlot;        //!< monitor threshold for number of client slots available
    uint32_t uAvgClientLate;        //!< monitor threshold for average client latency
    uint32_t uPctServerLoad;        //!< monitor threshold for server cpu percentage
    uint32_t uAvgSystemLoad;        //!< monitor threshold for system load (5 minute average)
    uint32_t uFailedGameCnt;        //!< monitor threshold for number of failed games (no data received)
    uint32_t uUdpErrorRate;         //!< monitor threshold for udp errors
    uint32_t uUdpRecvQueueLen;      //!< monitor threshold for udp receive queue length
    uint32_t uUdpSendQueueLen;      //!< monitor threshold for udp send queue length
} ServerMonitorConfigT;

//! modes we support on the voipserver
typedef enum VoipServerModeE
{
    VOIPSERVER_MODE_GAMESERVER,
    VOIPSERVER_MODE_CONCIERGE,
    VOIPSERVER_MODE_QOS_SERVER,
    VOIPSERVER_MODE_NUMMODES
} VoipServerModeE;

//! config file items
typedef struct VoipServerConfigT
{
    uint32_t uRecvBufSize;              //!< udp receive buffer size
    uint32_t uSendBufSize;              //!< udp send buffer size
    uint32_t uPeerTimeout;              //!< gameserver timeout (gameserver mode) / hosted connection timeout (concierge mode) (in milliseconds)
    int32_t iMaxClients;                //!< maximum number of clients we support
    uint32_t uFrontAddr;                //!< front-end address
    int32_t iFinalMetricsReportDuration;//!< number of ms during wich exit is postponed to give a chance for a last metrics push to the aggregator
    uint16_t uApiPort;                  //!< gameserver port, or qos probe port
    uint16_t uTunnelPort;               //!< local port tunnel will be bound to
    uint16_t uDiagnosticPort;           //!< port voipserver listens on for diagnostic requests
    int16_t iMaxGameServers;            //!< max number of gameservers supported by this VoipServerConfigT
    uint8_t uDebugLevel;                //!< debug level
    ServerMonitorConfigT MonitorWarnings;   //!< thresholds for monitor warn events
    ServerMonitorConfigT MonitorErrors;     //!< thresholds for monitor error events
    ProtoTunnelInfoT TunnelInfo;        //!< tunnel configuration info
    GameServerLateBinT LateBinCfg;      //!< latency bin definition, read in from config file
    char strCertFilename[64];           //!< path to the certificate file
    char strKeyFilename[64];            //!< path to the key file
    //! voip config
    int32_t iMaxSimultaneousTalkers;
    int32_t iTalkTimeOut;
    VoipServerModeE uMode;              //!< which voipserver mode are we using (gameserver, etc)
    //! metrics aggregator config
    char strMetricsAggregatorHost[256]; //!< metrics aggregator host
    uint32_t uMetricsAggregatorPort;    //!< metrics aggregator port
    uint32_t uMetricsPushingRate;       //!< reporting rate
    uint32_t uMetricsFinalPushingRate;  //!< final reporting rate
    //! subspace config
    char strSubspaceSidekickAddr[256];  //!< subspace sidekick app host (probably localhost)
    uint32_t uSubspaceSidekickPort;     //!< subspace sidekick app port (0 is disabled, probably 5005)
    char strSubspaceLocalAddr[256];     //!< subspace provided us this address to use in place of our own
    uint32_t uSubspaceLocalPort;        //!< subspace provided us this port to use in place of our own (0 is disabled)
    //! process state/scaling config
    int32_t iProcessStateMinDuration;       //!< minimum number of ms to stay in any state (required for all state transitions to show up in monitoring dashboards, shall be larger than metrics agent flush rate)
    int32_t iCpuThresholdDetectionDelay;    //!< number of ms for load to be sustained to trigger a "threshold crossed" event
    int32_t iAutoScaleBufferConst;          //!< number of pods worth of capacity cloud fabric should always have ready, for scaleup demand
    //! ccs config
    char strHostName[128];
    char strPingSite[VOIPSERVERCONFIG_SITE_SIZE];
    char strPool[VOIPSERVERCONFIG_POOL_SIZE];
    char strCoordinatorAddr[256];
    char strEnvironment[VOIPSERVERCONFIG_ENV_SIZE];         //!< the environment to use: "dev", "test", "cert" or "prod"
    char strNucleusAddr[256];                               //!< DNS address of the Nucleus service to use
    char strDeployInfo[VOIPSERVERCONFIG_DEPLOYINFO_SIZE];   //!< Deployment Info  (defaults to "servulator" for non-EAcloud deployment; "kubernetes pod name" for EAcloud deployment)
    char strDeployInfo2[VOIPSERVERCONFIG_DEPLOYINFO2_SIZE]; //!< Deployment Info 2  (defaults to "servulator" for non-EAcloud deployment; "kubernetes namespace" for EAcloud deployment)
    //! qos config
    char strOverrideSiteKey[QOS_COMMON_SECURE_KEY_LENGTH];  //!< the secure key value if overridden

    uint8_t bOffloadInboundSSL;                             //!< TRUE if enabled, else FALSE
    uint8_t bAuthValidate;                                  //!< TRUE if we want to validate auth tokens, FALSE otherwise
    uint8_t bJwtEnabled;                                    //!< TRUE if we want to use JWT tokens
    uint32_t uFlushRate;                                    //!< tunnel flush rate in milliseconds
    ServerMetricsFormatE eMetricsFormat;
} VoipServerConfigT;

typedef struct ConfigFileT ConfigFileT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// start the load of the configuration file
int32_t VoipServerLoadConfigBegin(ConfigFileT **pConfig, const char *pConfigTagsFile);

// attempts to finish loading the configuration file
int32_t VoipServerLoadConfigComplete(ConfigFileT *pConfig, const char **ppConfigTags);

// parses the configuration into our VoipServerConfigT struct
void VoipServerSetConfigVars(VoipServerConfigT *pConfig, const char *pCommandTags, const char *pConfigTags);

// parse the monitor configuration into our ServerMonitorConfigT that reside within VoipServerConfigT
void VoipServerLoadMonitorConfiguration(VoipServerConfigT *pConfig, const char *pCommandTags, const char *pConfigTags);

#ifdef __cplusplus
}
#endif

#endif // _voipserverconfig_h
