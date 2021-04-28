/*H********************************************************************************/
/*!
    \File voipserver.h

    \Description
        This is module handles creation of all the shared data that the
        different modes of the voipserver. It acts as the main entry point
        into functionality for the voipserver, modes specialize logic by
        taking advantage of the callbacks provided here.

    \Copyright
        Copyright (c) Electronic Arts 2015. ALL RIGHTS RESERVED.

    \Version 09/18/2015 (eesponda)
*/
/********************************************************************************H*/

#ifndef _voipserver_h
#define _voipserver_h

/*** Includes *********************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/voip/voiptunnel.h"

#include "dirtycast.h"
#include "gameserverpkt.h"

/*** Defines **********************************************************************/

//! voipserver stats are recalculated every second
#define VOIPSERVER_VS_STATRATE          (1*1000)
//! voipserver max rate metrics will accumulate for before being reset
#define VOIPSERVER_MAXMETRICSRESETRATE  (6*60*1000)

#define VOIPSERVER_LOAD_FSHIFT             (16)                                        // number of bits of precision
#define VOIPSERVER_LOAD_FIXED_1            (1 << VOIPSERVER_LOAD_FSHIFT)               // 1.0 as fixed-point

/*** Macros ***********************************************************************/

// macros for extracting load average from 16.16 fixed-point representation
#define VOIPSERVER_LoadInt(__x)           ((__x) >> VOIPSERVER_LOAD_FSHIFT)                                 // extract int portion
#define VOIPSERVER_LoadFrac(__x)          VOIPSERVER_LoadInt(((__x) & (VOIPSERVER_LOAD_FIXED_1-1)) * 100)   // extract frac portion and convert to pct
#define VOIPSERVER_LoadPct(__x)           (uint64_t)(((float)__x) / 655.36f)                                // convert fixed-point to fractional decimal integer with two digits of mantissa

/*** Type Definitions *************************************************************/

//! opaque module ref
typedef struct ServerMetricsT ServerMetricsT;
typedef struct VoipServerRefT VoipServerRefT;
typedef struct ProtoTunnelRefT ProtoTunnelRefT;
typedef struct VoipServerConfigT VoipServerConfigT;
typedef struct TokenApiRefT TokenApiRefT;

// enumeration used in conjuction with the signal flags
enum
{
    VOIPSERVER_SIGNAL_SHUTDOWN_IMMEDIATE    = 1 << 0,
    VOIPSERVER_SIGNAL_SHUTDOWN_IFEMPTY      = 1 << 1,
    VOIPSERVER_SIGNAL_CONFIG_RELOAD         = 1 << 2
};

//! process states
typedef enum {
    VOIPSERVER_PROCESS_STATE_RUNNING = 0,   //!< running under normal operation
    VOIPSERVER_PROCESS_STATE_DRAINING,      //<! VS received a signal to close, clean up
    VOIPSERVER_PROCESS_STATE_EXITING,       //<! we are about to exit, last chance to do final reporting of metrics
    VOIPSERVER_PROCESS_STATE_EXITED,        //<! ok to exit
    VOIPSERVER_PROCESS_STATE_NUMSTATES
} VoipServerProcessStateE;

extern const char * _VoipServer_strProcessStates[VOIPSERVER_PROCESS_STATE_NUMSTATES];

//! voipserver game event types
typedef enum VoipServerGameEventE
{
    VOIPSERVER_EVENT_RECVGAME,     //!< received game data from client (return 0 when the packet should be reflected)
    VOIPSERVER_EVENT_SENDGAME,     //!< sending game data to client
    VOIPSERVER_NUMEVENTS           //!< total number of events
} VoipServerGameEventE;

// event data for the gametunnel callbacks
typedef struct GameEventDataT
{
    const VoipTunnelClientT *pClient;
    VoipServerGameEventE eEvent;
    const char *pData;
    int32_t iDataSize;
    uint32_t uCurTick;
} GameEventDataT;

// callback to allow modes to initialize their mode specific data, expects a pointer to the mode initialized or NULL if failure
typedef void *(VoipServerInitializeCbT)(VoipServerRefT *pVoipServer, const char *pCommandTags, const char *pConfigTags);

// callback to allow modes to do any processing, expects TRUE/FALSE based on success
typedef uint8_t (VoipServerProcessCbT)(void *pRef, uint32_t *pSignalFlags, uint32_t uCurTick);

// status function to allow us to call our mode specific functions in a generic way
typedef int32_t (VoipServerStatusCbT)(void *pRef, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize);

// control function to allow us to call our mode specific function in a generic way
typedef int32_t (VoipServerControlCbT)(void *pRef, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);
 
// callback to allow modes to drain
typedef uint32_t (VoipServerDrainingCbT)(void *pRef);

// callback to allow modes to cleanup
typedef void (VoipServerShutdownCbT)(void *pRef);

// callback for game event
typedef int32_t (GameEventCallbackT)(const GameEventDataT *pEventData, void *pUserData);

// common stats shared between all modules
typedef struct VoipServerStatT
{
    //! server info
    DirtyCastServerInfoT ServerInfo;
    time_t uStartTime;

    //! latency stats
    GameServerStatInfoT StatInfo;       //!< most recent stat info

    //! kernel-induced latency stats
    struct {
        GameServerLateT Game;           //!< kernel-induced packet latency stats for game traffic  (included in StatInfo.InbLate)
        GameServerLateT Voip;           //!< kernel-induced packet latency stats for voip traffic
    } InbKernLate;

    //! total number of packets of game data received
    uint32_t       uTotalRecv;
    //! total number of sub-packets of game data received (tunneling only)
    uint32_t       uTotalRecvSub;
    //! total number of _VoipServerGameRecvCallback() calls made to get game packets
    uint32_t       uRecvCalls;
    //! total number of out-of-order packet discards (tunneling only)
    uint32_t       uTotalPktsDiscard;

    //! overall "talking clients" stats
    int32_t         iTalkingClientsAvg; //!< active stat buffer  (last calculated average over the interval)
    int32_t         iTalkingClientsCur; //!< most recently collected stats
    int32_t         iTalkingClientsSum; //!< accumulated stats
    int32_t         iTalkingClientsMax; //!< max stats over an interval
    int32_t         iTalkingClientsSamples; //!< number of samples collected over the interval

    //! overall traffic stats
    GameServerStatsT GameStats;     //!< active stat buffer
    GameServerStatsT GameStatsCur;  //!< most recently collected stats
    GameServerStatsT GameStatsSum;  //!< accumulated stats
    GameServerStatsT GameStatsMax;  //!< max stats over an interval

    //! total number of received udp packets (as last read from the system)
    uint64_t uTotalPktsUdpReceivedCount;
    //! total number of send udp packets (as last read from the system)
    uint64_t uTotalPktsUdpSentCount;
    //! total number of error udp packets (as last read from the system)
    uint64_t uTotalPktsUdpErrorCount;

    uint32_t uTotalClientsAdded;                //!< total number of clients added
    uint32_t uTotalGamesCreated;                //!< total number of games created
    uint32_t uLastTotalClientsAdded;            //!< last snapshot of total number of clients added
    uint32_t uLastTotalGamesCreated;            //!< last snapshot of total number of games created
} VoipServerStatT;

//! base monitor flags, custom flags can be added after last
enum
{
    VOIPSERVER_MONITORFLAG_PCTGAMESAVAIL    = 1 << 0,
    VOIPSERVER_MONITORFLAG_PCTCLIENTSLOT    = 1 << 1,
    VOIPSERVER_MONITORFLAG_AVGCLIENTLATE    = 1 << 2,
    VOIPSERVER_MONITORFLAG_PCTSERVERLOAD    = 1 << 3,
    VOIPSERVER_MONITORFLAG_AVGSYSTEMLOAD    = 1 << 4,
    VOIPSERVER_MONITORFLAG_FAILEDGAMECNT    = 1 << 5,
    VOIPSERVER_MONITORFLAG_UDPRECVQULEN     = 1 << 6,
    VOIPSERVER_MONITORFLAG_UDPSENDQULEN     = 1 << 7,
    VOIPSERVER_MONITORFLAG_LAST             = VOIPSERVER_MONITORFLAG_UDPSENDQULEN
};

//! diagnostic monitor information
typedef struct VoipServerDiagnosticMonitorT
{
    uint32_t uMonitorFlag;
    const char *pMonitorMessage;
} VoipServerDiagnosticMonitorT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the module
VoipServerRefT *VoipServerCreate(int32_t iArgCt, const char *pArgs[], const char *pConfigTagFile);

// update the module
uint8_t VoipServerUpdate(VoipServerRefT *pVoipServer, uint32_t *pSignalFlags);

// get voiptunnel
VoipTunnelRefT *VoipServerGetVoipTunnel(const VoipServerRefT *pVoipServer);

// get prototunnel
ProtoTunnelRefT *VoipServerGetProtoTunnel(const VoipServerRefT *pVoipServer);

// get tokenapi
TokenApiRefT *VoipServerGetTokenApi(const VoipServerRefT *pVoipServer);

// get configuration
const VoipServerConfigT *VoipServerGetConfig(const VoipServerRefT *pVoipServer);

// let the voipserver send traffic
void VoipServerSend(VoipServerRefT *pVoipServer, uint32_t uSrcClient, uint32_t uDstClient, const char *pData, int32_t iDataSize);

// move the voip server process through its lifetime
void VoipServerProcessMoveToNextState(VoipServerRefT *pVoipServer, VoipServerProcessStateE eNewState);

// status function
int32_t VoipServerStatus(VoipServerRefT *pVoipServer, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize);

// control function
int32_t VoipServerControl(VoipServerRefT *pVoipServer, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// set the game/voip callback
void VoipServerCallback(VoipServerRefT *pVoipServer, GameEventCallbackT *pGameCallback, VoipTunnelCallbackT *pVoipCallback, void *pUserData);

// update monitor flags
void VoipServerUpdateMonitorFlag(VoipServerRefT *pVoipServer, uint32_t bFailWrn, uint32_t bFailErr, uint32_t uFailFlag, uint32_t bClearOnly);

// checks the monitor flags add warnings/errors to buffer
int32_t VoipServerMonitorFlagCheck(VoipServerRefT *pVoipServer, const VoipServerDiagnosticMonitorT *pDiagnostic, uint32_t *pStatus, char *pBuffer, int32_t iBufSize);

// destroy the module
void VoipServerDestroy(VoipServerRefT *pVoipServer);

// get the ServerMetricsT reference
ServerMetricsT* VoipServerGetServerMetrics(VoipServerRefT *pVoipServer);

#ifdef __cplusplus
}
#endif

#endif // _voipserver_h
