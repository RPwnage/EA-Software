/*H********************************************************************************/
/*!
    \File voipserver.c

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

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/prototunnel.h"
#include "DirtySDK/voip/voiptunnel.h"
#include "DirtySDK/dirtyvers.h"

#include "dirtycast.h"
#include "configfile.h"
#include "tokenapi.h"
#include "serverdiagnostic.h"
#include "servermetrics.h"
#include "voipserverconfig.h"
#include "voipservercomm.h"
#include "gameserverdiagnostic.h"
#include "conciergediagnostic.h"
#include "gameservermetrics.h"
#include "conciergemetrics.h"
#include "qosmetrics.h"
#include "qosdiagnostic.h"
#include "qosserver.h"
#include "subspaceapi.h"
#include "udpmetrics.h"
#include "voipgameserver.h"
#include "voipconcierge.h"
#include "voipserver.h"

/*** Defines **********************************************************************/

//! identifier for allocations
#define VOIPSERVER_MEMID ('vsrv')

/*** Type Definitions *************************************************************/

//! used to represent the functions of our different modes
typedef struct VoipServerDispatchT
{
    VoipServerInitializeCbT *pInitializeCb;
    VoipServerProcessCbT *pProcessCb;
    VoipServerStatusCbT *pStatusCb;
    VoipServerControlCbT *pControlCb;
    VoipServerDrainingCbT *pDrainingCb;
    VoipServerShutdownCbT *pShutdownCb;
} VoipServerDispatchT;

//! voipserver internal module data
typedef struct VoipServerRefT
{
    //! config file items
    VoipServerConfigT Config;
    ConfigFileT *pConfig;
    const char *pConfigTags;
    uint8_t bConfigReloading;
    uint8_t _pad1[3];

    //! config filename
    char strCfgFileName[256];
    //! command line
    char strCmdTagBuf[2048];

    //! prototunnel module
    ProtoTunnelRefT *pProtoTunnel;

    //! voiptunnel module
    VoipTunnelRefT *pVoipTunnel;

    //! tokenapi module
    TokenApiRefT *pTokenRef;

    //! virtual game socket
    SocketT *pGameSocket;

    //! pid filename
    char strPidFilename[256];

    //! hostname
    char strHostname[256];

    //! override callbacks
    const VoipServerDispatchT* pDispatch;
    void *pUserData;

    //! game/voip callback
    GameEventCallbackT *pGameCallback;
    VoipTunnelCallbackT *pVoipCallback;
    void *pCallbackUserData;

    //! memgroup data
    int32_t iMemGroup;
    void *pMemGroupUserdata;

    //! current monitor warning flags
    uint32_t uMonitorFlagWarnings;
    uint32_t uMonitorFlagErrors;

    //! signal flag state
    uint32_t uSignalFlags;

    //! server diagnostic module
    ServerDiagnosticT *pServerDiagnostic;

    //! metrics reporting module
    ServerMetricsT *pServerMetrics;

    //! subspace sidekick communication module
    SubspaceApiRefT *pSubspaceApi;

    //! process lifetime managment
    VoipServerProcessStateE eProcessState;      //!< state of the process
    uint32_t uCurrentStateStart;                //!< time when current state was entered
    uint8_t bCurrentStateMinDurationDone;       //!< TRUE when current state has last for the required minimal duration
    uint8_t bReady;                             //!< TRUE when readiness file is created, FALSE otherwise (not meant to be reset once set)
    uint8_t _pad2[2];

    //! metrics update tracking info
    uint32_t uLastMetricsUpdate;        //!< last time metrics accumulators were updated
    uint32_t uLastMetricsReset;         //!< last time metrics accumulators were reset
    uint32_t uFinalMetricsReportStart;  //!< time at which "final metrics report" was started
    uint8_t bResetMetrics;              //!< if true, clear metrics accumulators
    uint8_t bModuleExited;              //!< voipserver specialization module (otp, cc or qos) signaled the need to quit
    uint8_t _pad3[2];

    //! udp metrics socket
    int32_t iUdpMetricsSocket;

    //! voipserver stats
    UdpMetricsT UdpMetrics;
    VoipServerStatT Stats;

#if DIRTYCODE_DEBUG
    /* For testing only.
        CPU% : [-1,100] where -1 is the default and means "debug feature disabled"
    */
    int iFakeCpu;
#endif
} VoipServerRefT;

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

// must be aligned with VoipServerProcessStateE
const char * _VoipServer_strProcessStates[VOIPSERVER_PROCESS_STATE_NUMSTATES] =
{
    "running    ",   // VOIPSERVER_PROCESS_STATE_RUNNING
    "draining   ",   // VOIPSERVER_PROCESS_STATE_DRAINING
    "exiting    ",   // VOIPSERVER_PROCESS_STATE_EXITING
    "exited     "    // VOIPSERVER_PROCESS_STATE_EXITED
};

// Explicitly cast here to tell the compiler it is okay to upgrade from the void * to the correct module ref *
// This makes it so we don't have to cast back and forth between the two types
static const VoipServerDispatchT _VoipServer_ModeDispatch[VOIPSERVER_MODE_NUMMODES] =
{
    { (VoipServerInitializeCbT *)VoipGameServerInitialize, (VoipServerProcessCbT *)VoipGameServerProcess, (VoipServerStatusCbT *)VoipGameServerStatus, (VoipServerControlCbT *)VoipGameServerControl, (VoipServerDrainingCbT *)VoipGameServerDrain, (VoipServerShutdownCbT *)VoipGameServerShutdown },
    { (VoipServerInitializeCbT *)VoipConciergeInitialize, (VoipServerProcessCbT *)VoipConciergeProcess, (VoipServerStatusCbT *)VoipConciergeStatus, (VoipServerControlCbT *)VoipConciergeControl, (VoipServerDrainingCbT *)VoipConciergeDrain, (VoipServerShutdownCbT *)VoipConciergeShutdown },
    { (VoipServerInitializeCbT *)QosServerInitialize, (VoipServerProcessCbT *)QosServerProcess, (VoipServerStatusCbT *)QosServerStatus, (VoipServerControlCbT *)QosServerControl, (VoipServerDrainingCbT *)QosServerDrain, (VoipServerShutdownCbT *)QosServerShutdown }
};


/*** Private functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _VoipServerUpdateConfig

    \Description
        Tries to complete reloading, if successful loads the monitor config

    \Input *pVoipServer     - voipserver module state

    \Version 11/30/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerUpdateConfig(VoipServerRefT *pVoipServer)
{
    int32_t iResult;

    // not reloading, nothing to do
    if (pVoipServer->bConfigReloading == FALSE)
    {
        return;
    }

    // if reloading is in progress, wait for it to complete
    if ((iResult = VoipServerLoadConfigComplete(pVoipServer->pConfig, &pVoipServer->pConfigTags)) == 0)
    {
        return;
    }
    else if (iResult > 0) // if reload succeeded, update config entries that are reloadable
    {
        VoipServerLoadMonitorConfiguration(&pVoipServer->Config, pVoipServer->strCmdTagBuf, pVoipServer->pConfigTags);
    }

    // reloading done
    pVoipServer->bConfigReloading = FALSE;
}

/*F********************************************************************************/
/*!
    \Function _VoipServerGetAddr

    \Description
        Get physical (and virtual, if tunneling) addresses for the specified client.

    \Input *pProtoTunnel    - pointer to prototunnel module ref
    \Input *pClient         - pointer to client to get addresses for
    \Input *pPhysAddr       - [out] storage for physical address
    \Input *pVirtAddr       - [out] storage for virtual address, if tunneling

    \Version 10/22/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerGetAddr(ProtoTunnelRefT *pProtoTunnel, const VoipTunnelClientT *pClient, struct sockaddr *pPhysAddr, struct sockaddr *pVirtAddr)
{
    SockaddrInit(pPhysAddr, AF_INET);
    SockaddrInit(pVirtAddr, AF_INET);

    ProtoTunnelStatus(pProtoTunnel, 'vtop', pClient->uRemoteAddr, pPhysAddr, sizeof(struct sockaddr));
    SockaddrInSetAddr(pVirtAddr, pClient->uRemoteAddr);
    SockaddrInSetPort(pVirtAddr, pClient->uRemoteGamePort);
}

/*F********************************************************************************/
/*!
    \Function _VoipServerLogGetAddr

    \Description
        Get physical (and virtual, if tunneling) addresses for the specified client
        and logs it

    \Input *pProtoTunnel    - pointer to prototunnel module ref
    \Input *pClient         - pointer to client to get addresses for

    \Version 06/08/2011 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerLogGetAddr(ProtoTunnelRefT *pProtoTunnel, const VoipTunnelClientT *pClient)
{
    struct sockaddr PhysAddr, VirtAddr;
    uint32_t uVirtAddr;

    // get the addresses
    _VoipServerGetAddr(pProtoTunnel, pClient, &PhysAddr, &VirtAddr);

    if ((uVirtAddr = SockaddrInGetAddr(&VirtAddr)) != 0)
    {
        DirtyCastLogPrintf("voipserver: matching clientid=0x%08x to address %a:%d (virtual address %a)\n", pClient->uClientId,
            SockaddrInGetAddr(&PhysAddr), SockaddrInGetPort(&PhysAddr), uVirtAddr);
    }
    else
    {
        DirtyCastLogPrintf("voipserver: matching clientid=0x%08x to address %a:%d\n", pClient->uClientId,
            SockaddrInGetAddr(&PhysAddr), SockaddrInGetPort(&PhysAddr));
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipServerProtoTunnelEventCallback

    \Description
        ProtoTunnel event callback handler.

    \Input *pProtoTunnel    - tunnel module state
    \Input eEvent           - tunnel event
    \Input *pData           - received data
    \Input iDataSize        - size of received data
    \Input *pRecvAddr       - address data was received from
    \Input *pUserData       - voiptunnel module

    \Version 03/26/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerProtoTunnelEventCallback(ProtoTunnelRefT *pProtoTunnel, ProtoTunnelEventE eEvent, const char *pData, int32_t iDataSize, struct sockaddr *pRecvAddr, void *pUserData)
{
    // only handle unmapped receive events
    if (eEvent == PROTOTUNNEL_EVENT_RECVNOMATCH)
    {
        NetPrintf(("voipserver: warning, got unmatched data from source %a:%d\n", SockaddrInGetAddr(pRecvAddr), SockaddrInGetPort(pRecvAddr)));
    }
    else
    {
        NetPrintf(("voipserver: got unhandled event %d\n", eEvent));
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipServerRecvEvent

    \Description
        Fires the recv'd data event

    \Input *pVoipServer     - module state
    \Input *pClient         - the client associated with the event
    \Input eEvent           - the type of event
    \Input *pData           - packet data
    \Input iDataSize        - the size of the data
    \Input uCurTick         - current tick

    \Output
        int32_t - based on the event type, see voipserver.h

    \Version 10/06/2015 (eesponda)
*/
/********************************************************************************F*/
static int32_t _VoipServerGameEvent(VoipServerRefT *pVoipServer, VoipServerGameEventE eEvent, const VoipTunnelClientT *pClient, const char *pData, int32_t iDataSize, uint32_t uCurTick)
{
    GameEventDataT EventData;
    ds_memclr(&EventData, sizeof(EventData));

    EventData.pClient = pClient;
    EventData.eEvent = eEvent;
    EventData.pData = pData;
    EventData.iDataSize = iDataSize;
    EventData.uCurTick = uCurTick;

    return(pVoipServer->pGameCallback(&EventData, pVoipServer->pCallbackUserData));
}

/*F********************************************************************************/
/*!
    \Function _VoipServerReflect

    \Description
        Parses the destination information and sends data

    \Input *pVoipServer     - module state
    \Input *pClient         - source of the data
    \Input *pData           - packet data
    \Input iDataSize        - size of data

    \Version 10/06/2015 (eesponda)
*/
/********************************************************************************F*/
static void _VoipServerReflect(VoipServerRefT *pVoipServer, const VoipTunnelClientT *pClient, const char *pData, int32_t iDataSize)
{
    const VoipServerConfigT *pConfig = &pVoipServer->Config;
    CommUDPPacketMeta1T Meta1Data;
    int32_t iResult;

    #if DIRTYVERS <= DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
    // display incoming packet
    NetPrintfVerbose((pConfig->uDebugLevel, 2, "voipserver: [gs%02d-inp] %d byte packet from %a:%d clientid=0x%08x\n",
        pClient->iGameIdx, iDataSize, pClient->uRemoteAddr, pClient->uRemoteGamePort, pClient->uClientId));
    #else
    /* display incoming packet (game index not displayed because this is a src-to-dest unicasting and
       the game traffic does not contain data allowing us to associate it with any games the clients
       belong to */
    NetPrintfVerbose((pConfig->uDebugLevel, 2, "voipserver: [game-inp] %d byte packet from %a:%d clientid=0x%08x\n",
        iDataSize, pClient->uRemoteAddr, pClient->uRemoteGamePort, pClient->uClientId));
    #endif

    if (pConfig->uDebugLevel > 3)
    {
        int32_t iPrintSize = iDataSize;
        if ((iPrintSize > 64) && (pConfig->uDebugLevel < 5))
        {
            iPrintSize = 64;
        }
        NetPrintMem(pData, iPrintSize, "voipserver<-client");
    }

    // extract destination info for routing purposes
    if ((iResult = VoipServerExtractMeta1Data((const CommUDPPacketHeadT *)pData, iDataSize, &Meta1Data)) > 0)
    {
        NetPrintfVerbose((pConfig->uDebugLevel, 2, "voipserver: [voip-inp] meta1data=0x%08x/0x%08x\n",
            Meta1Data.uSourceClientId, Meta1Data.uTargetClientId));

        // unicast send to remote client identifier
        VoipServerSend(pVoipServer, Meta1Data.uSourceClientId, Meta1Data.uTargetClientId, pData, iDataSize);
    }
    // extraction failed
    else
    {
        DirtyCastLogPrintf("voipserver: could not extract metadata from packet\n");
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipServerGameRecvCallback

    \Description
        Callback when we receive data

    \Input *pSocket     - where we received data only
    \Input iFlags       - unused
    \Input *pUserData   - module ref pointer

    \Output
        int32_t - returns 0

    \Version 03/31/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipServerGameRecvCallback(SocketT *pSocket, int32_t iFlags, void *pUserData)
{
    VoipServerRefT *pVoipServer = (VoipServerRefT *)pUserData;
    int32_t iRecv;
    VoipTunnelRefT *pVoipTunnel = pVoipServer->pVoipTunnel;
    ProtoTunnelRefT *pProtoTunnel = pVoipServer->pProtoTunnel;
    uint32_t uCurTick = NetTick();

    // handle up to 64 recvs
    for (iRecv = 0; iRecv < 64; iRecv += 1)
    {
        int32_t iRecvLen, iAddrLen = sizeof(struct sockaddr);
        char aPacketData[SOCKET_MAXUDPRECV];
        struct sockaddr RecvAddr;
        CommUDPPacketHeadT *pPacketHead;
        VoipTunnelClientT *pClient;
        uint32_t uKernelRecvTick;
        GameServerLateT InbKernLate;

        // get any input?
        if ((iRecvLen = SocketRecvfrom(pSocket, aPacketData, sizeof(aPacketData), 0, &RecvAddr, &iAddrLen)) <= 0)
        {
            break;
        }

        // first try to match based on address and port
        if ((pClient = VoipTunnelClientListMatchSockaddr(pVoipTunnel, &RecvAddr)) == NULL)
        {
            uint32_t uClientId;

            // see if it is a commudp packet
            if ((pPacketHead = VoipServerValidateCommUDPPacket(aPacketData, iRecvLen)) == NULL)
            {
                NetPrintf(("voipserver: ignoring game packet from %a:%d with no client registered\n",
                    SockaddrInGetAddr(&RecvAddr), SockaddrInGetPort(&RecvAddr)));
                continue;
            }

            // extract clientid
            uClientId = VoipServerDecodeU32(pPacketHead->uClientId);

            // if we don't have a client with this id yet, bail
            if ((pClient = VoipTunnelClientListMatchId(pVoipTunnel, uClientId)) == NULL)
            {
                NetPrintf(("voipserver: ignoring game packet from %a:%d with unregistered id=0x%08x\n",
                    SockaddrInGetAddr(&RecvAddr), SockaddrInGetPort(&RecvAddr)));
                continue;
            }
        }

        // if the address isn't set yet, set it now
        if (pClient->uRemoteAddr != (uint32_t)SockaddrInGetAddr(&RecvAddr))
        {
            // get source client Address
            pClient->uRemoteAddr = SockaddrInGetAddr(&RecvAddr);
            // log address on match
            _VoipServerLogGetAddr(pProtoTunnel, pClient);
        }
        // if the port isn't set yet, set it now
        if (pClient->uRemoteGamePort != SockaddrInGetPort(&RecvAddr))
        {
            pClient->uRemoteGamePort = SockaddrInGetPort(&RecvAddr);
            DirtyCastLogPrintf("voipserver: matching clientid=0x%08x to game port %d\n", pClient->uClientId, pClient->uRemoteGamePort);
        }

        // detect a stall (>1.5s without receiving packets)
        if (pClient->uLastRecvGame != 0)
        {
            int32_t iLastRecv;
            if ((iLastRecv = NetTickDiff(uCurTick, pClient->uLastRecvGame)) > 1500)
            {
                pClient->iUserData0 = iLastRecv;        //!< we use iUserData0 to track the stall window
            }
            else if (iLastRecv < pClient->iUserData0)   // if our last recv is less than our stall window, we've received new data and report the stall
            {
                struct sockaddr PhysAddr, VirtAddr;
                _VoipServerGetAddr(pProtoTunnel, pClient, &PhysAddr, &VirtAddr);
                DirtyCastLogPrintf("voipserver: %dms stall detected on receive for clientid=0x%08x (%a:%d)\n", pClient->iUserData0, pClient->uClientId,
                    SockaddrInGetAddr(&PhysAddr), SockaddrInGetPort(&PhysAddr));
                pClient->iUserData0 = 0;
            }
        }
        // track last recv time (and reserve zero as uninitialized)
        if ((pClient->uLastRecvGame = uCurTick) == 0)
        {
            pClient->uLastRecvGame = 0xffffffff;
        }

        // accumulate data received information
        pVoipServer->Stats.GameStats.GameStat.uByteRecv += (uint32_t)VOIPSERVER_EstimateNetworkPacketSize(iRecvLen);
        pVoipServer->Stats.GameStats.GameStat.uPktsRecv += 1;

        /* unix: extract kernel-applied received timestamp
           windows: extract dirtynet-applied received timestamp */
        uKernelRecvTick = SockaddrInGetMisc(&RecvAddr);

        /* accumulate kernel-induced inbound latency for game traffic
           (should evaluate to 0 on windows because uKernelRecvTick will always be same as NetTick() */
        ds_memclr(&InbKernLate, sizeof(InbKernLate));
        GameServerPacketUpdateLateStatsSingle(&InbKernLate, NetTickDiff(NetTick(), uKernelRecvTick));
        GameServerPacketUpdateLateStats(&pVoipServer->Stats.InbKernLate.Game, &InbKernLate);

        // fire event
        if (_VoipServerGameEvent(pVoipServer, VOIPSERVER_EVENT_RECVGAME, pClient, aPacketData, iRecvLen, uKernelRecvTick) == 0)
        {
            // if the packet was not consumed by the module by default we will reflect the packet
            _VoipServerReflect(pVoipServer, pClient, aPacketData, iRecvLen);
        }

        // update source client timestamp
        pClient->uLastUpdate = uCurTick;
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipServerVoipTunnelEventCallback

    \Description
        ProtoTunnel event callback handler.

    \Input *pVoipTunnel     - voiptunnel module state
    \Input *pEventData      - event data
    \Input *pUserData       - user callback ref (voipserver module state)

    \Version 03/01/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerVoipTunnelEventCallback(VoipTunnelRefT *pVoipTunnel, const VoipTunnelEventDataT *pEventData, void *pUserData)
{
    VoipServerRefT *pVoipServer = (VoipServerRefT *)pUserData;

    // accumulate recv stats
    if (pEventData->eEvent == VOIPTUNNEL_EVENT_RECVVOICE)
    {
        GameServerLateT InbKernLate;
        const uint32_t uNetworkBytes = VOIPSERVER_EstimateNetworkPacketSize(pEventData->iDataSize);
        pVoipServer->Stats.GameStats.VoipStat.uByteRecv += uNetworkBytes;
        pVoipServer->Stats.GameStats.VoipStat.uPktsRecv += 1;

        /* accumulate kernel-induced inbound latency for voip traffic
           (should evaluate to 0 on windows because uKernelRecvTick will always be same as NetTick() */
        ds_memclr(&InbKernLate, sizeof(InbKernLate));
        GameServerPacketUpdateLateStatsSingle(&InbKernLate, NetTickDiff(NetTick(), pEventData->uTick));
        GameServerPacketUpdateLateStats(&pVoipServer->Stats.InbKernLate.Voip, &InbKernLate);

        NetPrintfVerbose((pVoipServer->Config.uDebugLevel, 2, "voipserver: [voip-inp] %d byte packet fm %a:%d clientId=0x%08x\n",
            pEventData->iDataSize, pEventData->pClient->uRemoteAddr,
            pEventData->pClient->uRemoteVoipPort, pEventData->pClient->uClientId));
    }
    // accumulate send stats
    if (pEventData->eEvent == VOIPTUNNEL_EVENT_SENDVOICE)
    {
        const uint32_t uNetworkBytes = VOIPSERVER_EstimateNetworkPacketSize(pEventData->iDataSize);
        pVoipServer->Stats.GameStats.VoipStat.uByteSent += uNetworkBytes;
        pVoipServer->Stats.GameStats.VoipStat.uPktsSent += 1;

       NetPrintfVerbose((pVoipServer->Config.uDebugLevel, 2, "voipserver: [voip-inp] %d byte packet fm %a:%d clientId=0x%08x\n",
            pEventData->iDataSize, pEventData->pClient->uRemoteAddr,
            pEventData->pClient->uRemoteVoipPort, pEventData->pClient->uClientId));
    }
    // debug logging for client match addr event
    if ((pEventData->eEvent == VOIPTUNNEL_EVENT_MATCHADDR) && (pEventData->pClient != NULL))
    {
        _VoipServerLogGetAddr(pVoipServer->pProtoTunnel, pEventData->pClient);
    }
    // debug logging for client match port event
    if ((pEventData->eEvent == VOIPTUNNEL_EVENT_MATCHPORT) && (pEventData->pClient != NULL))
    {
        DirtyCastLogPrintf("voipserver: matching clientId=0x%08x to voip port %d\n", pEventData->pClient->uClientId, pEventData->pClient->uRemoteVoipPort);
    }
    // debug logging for dead voice event
    if ((pEventData->eEvent == VOIPTUNNEL_EVENT_DEADVOICE) && (pEventData->pClient != NULL))
    {
        DirtyCastLogPrintf("voipserver: voice for clientId=0x%08x has gone dead\n", pEventData->pClient->uClientId);
    }
    // debug logging for VOIPTUNNEL_EVENT_MAXDVOICE
    if ((pEventData->eEvent == VOIPTUNNEL_EVENT_MAXDVOICE) && (pEventData->pClient != NULL))
    {
        #if DIRTYVERS <= DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
        DirtyCastLogPrintf("voipserver: limit of voip broadcasters reached for clientId 0x%08x in game %d\n", pEventData->pClient->uClientId, pEventData->pClient->iGameIdx);
        #else
        DirtyCastLogPrintf("voipserver: limit of voip broadcasters reached for clientId 0x%08x in game %d\n", pEventData->pClient->uClientId, pEventData->iGameIdx);
        #endif
    }
    // debug logging for VOIPTUNNEL_EVENT_AVLBVOICE
    if ((pEventData->eEvent == VOIPTUNNEL_EVENT_AVLBVOICE) && (pEventData->pClient != NULL))
    {
        #if DIRTYVERS <= DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
        DirtyCastLogPrintf("voipserver: voip broadcast slots are available for clientId 0x%08x in game %d\n", pEventData->pClient->uClientId, pEventData->pClient->iGameIdx);
        #else
        DirtyCastLogPrintf("voipserver: voip broadcast slots are available for clientId 0x%08x in game %d\n", pEventData->pClient->uClientId, pEventData->iGameIdx);
        #endif
    }
    if ((pEventData->eEvent == VOIPTUNNEL_EVENT_ADDCLIENT) && (pEventData->pClient != NULL))
    {
        pVoipServer->Stats.uTotalClientsAdded += 1;
    }
    // debug logging for VOIPTUNNEL_EVENT_DELCLIENT
    if ((pEventData->eEvent == VOIPTUNNEL_EVENT_DELCLIENT) && (pEventData->pClient != NULL))
    {
        // check for a stall on removal
        if (pEventData->pClient->iUserData0 > 0)
        {
            struct sockaddr PhysAddr, VirtAddr;
            ProtoTunnelRefT *pProtoTunnel = pVoipServer->pProtoTunnel;
            const VoipTunnelClientT *pClient = pEventData->pClient;

            _VoipServerGetAddr(pProtoTunnel, pClient, &PhysAddr, &VirtAddr);
            DirtyCastLogPrintf("voipserver: %dms stall detected on removal for clientid=0x%08x (%a:%d)\n", pEventData->pClient->iUserData0, pClient->uClientId,
                SockaddrInGetAddr(&PhysAddr), SockaddrInGetPort(&PhysAddr));
            pEventData->pClient->iUserData0 = 0;
        }
    }
    if (pEventData->eEvent == VOIPTUNNEL_EVENT_ADDGAME)
    {
        pVoipServer->Stats.uTotalGamesCreated += 1;
    }

    // let the operation module do any processing it needs
    pVoipServer->pVoipCallback(pVoipTunnel, pEventData, pVoipServer->pCallbackUserData);
}

/*F********************************************************************************/
/*!
    \Function _VoipServerCreateVoipTunnel

    \Description
        Helper to create and do basic configuration on VoipTunnelRefT

    \Input *pConfig         - configuration we base our control calls on

    \Output
        VoipTunnelRefT *    - ref, or NULL

    \Version 09/18/2015 (eesponda)
*/
/********************************************************************************F*/
static VoipTunnelRefT *_VoipServerCreateVoipTunnel(const VoipServerConfigT *pConfig)
{
    VoipTunnelRefT *pVoipTunnel;
    if ((pVoipTunnel = VoipTunnelCreate(VOIPSERVERCONFIG_VOIPPORT, pConfig->iMaxClients, pConfig->iMaxGameServers)) == NULL)
    {
        DirtyCastLogPrintf("voipserver: unable to create voiptunnel module\n");
        return(NULL);
    }

    // set debug level
    VoipTunnelControl(pVoipTunnel, 'spam', pConfig->uDebugLevel, 0, NULL);

    // set throttling configuration
    VoipTunnelControl(pVoipTunnel, 'bvma', pConfig->iMaxSimultaneousTalkers, 0, NULL);
    DirtyCastLogPrintf("voipserver: setting maximum number of talkers per game to %d\n", pConfig->iMaxSimultaneousTalkers);
    VoipTunnelControl(pVoipTunnel, 'rtim', pConfig->iTalkTimeOut, 0, NULL);
    DirtyCastLogPrintf("voipserver: setting receiving voice timeout to %dms\n", pConfig->iTalkTimeOut);

    return(pVoipTunnel);
}

/*F********************************************************************************/
/*!
    \Function _VoipServerCreateProtoTunnel

    \Description
        Helper to create and do basic configuration on ProtoTunnelRefT

    \Input *pConfig - Configuration we base our creation on

    \Output
        ProtoTunnelRefT *       - ref, or NULL

    \Version 09/18/2015 (eesponda)
*/
/********************************************************************************F*/
static ProtoTunnelRefT *_VoipServerCreateProtoTunnel(const VoipServerConfigT *pConfig)
{
    ProtoTunnelRefT *pProtoTunnel;
    uint16_t uTunnelPort;
    int32_t iPortMap;
    const ProtoTunnelInfoT *pTunnelInfo = &pConfig->TunnelInfo;

    if ((pProtoTunnel = ProtoTunnelCreate(pConfig->iMaxClients, pConfig->uTunnelPort)) == NULL)
    {
        DirtyCastLogPrintf("voipserver: unable to create prototunnel module\n");
        return(NULL);
    }

    if ((uTunnelPort = ProtoTunnelStatus(pProtoTunnel, 'lprt', 0, NULL, 0)) != pConfig->uTunnelPort)
    {
        DirtyCastLogPrintf("voipserver: unable to create prototunnel on the desired port. Desired is %d. 'lprt' is %d\n", pConfig->uTunnelPort, uTunnelPort);
        ProtoTunnelDestroy(pProtoTunnel);
        return(NULL);
    }

    // make ports virtual
    for (iPortMap = 0; iPortMap < PROTOTUNNEL_MAXPORTS; iPortMap++)
    {
        uint16_t uMapPort;
        if ((uMapPort = pTunnelInfo->aRemotePortList[iPortMap]) != 0)
        {
            SocketControl(NULL, 'vadd', uMapPort, NULL, NULL);
        }
    }

    // update debug logging
    ProtoTunnelControl(pProtoTunnel, 'spam', pConfig->uDebugLevel, 0, NULL);

    // set the flush rate for packets, higher flush rates increases the chances that packets will bundle together for efficiency
    ProtoTunnelControl(pProtoTunnel, 'rate', pConfig->uFlushRate, 0, NULL);

    // set the socket rbuf size
    if (pConfig->uRecvBufSize != 0)
    {
        ProtoTunnelControl(pProtoTunnel, 'rbuf', pConfig->uRecvBufSize, 0, NULL);
    }

    // set the socket sbuf size
    if (pConfig->uSendBufSize != 0)
    {
        ProtoTunnelControl(pProtoTunnel, 'sbuf', pConfig->uSendBufSize, 0, NULL);
    }

    // set default event callback
    ProtoTunnelCallback(pProtoTunnel, _VoipServerProtoTunnelEventCallback, NULL);

    return(pProtoTunnel);
}

/*F********************************************************************************/
/*!
    \Function _VoipServerCreateTokenApi

    \Description
        Helper to create and do basic configuration on TokenApiRefT

    \Input *pConfig - Configuration we base our creation on

    \Output
        TokenApiRefT *       - ref, or NULL

    \Version 09/18/2015 (eesponda)
*/
/********************************************************************************F*/
static TokenApiRefT *_VoipServerCreateTokenApi(const VoipServerConfigT *pConfig)
{
    TokenApiRefT *pTokenRef;
    if ((pTokenRef = TokenApiCreate(pConfig->strCertFilename, pConfig->strKeyFilename)) == NULL)
    {
        return(NULL);
    }

    // configure tokenapi
    TokenApiControl(pTokenRef, 'nuca', 0, 0, (void*)pConfig->strNucleusAddr);
    TokenApiControl(pTokenRef, 'spam', pConfig->uDebugLevel, 0, NULL);
    TokenApiControl(pTokenRef, 'jwts', pConfig->bJwtEnabled, 0, NULL);

    return(pTokenRef);
}

/*F********************************************************************************/
/*!
    \Function _VoipServerAttemptFQDNLookup

    \Description
        Determine FQDN (Fully Qualified Domain Name) for the host. If FQDN is specified
        in config file, we use that value. If not, we perfrom a reverse DSN lookup
        using the public ip address (internet reachable address) associated with the host.

    \Input *pVoipServer     - module state

    \Notes
       There are situations where this function is not successful because it gets executed
       when host network is not fully initialized. (We observed such race conditions when 
       exectuting as newly instantiated pods on newly instantiated EADP cloud nodes). When
       that happens, pVoipServer->strHostname is left empty and it is good practice to 
       re-attempt using this function later in time when the network stack is fully up. 

    \Version 19/09/2019 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipServerAttemptFQDNLookup(VoipServerRefT *pVoipServer)
{
    // use hostname specified in config, otherwise query from system
    if (pVoipServer->Config.strHostName[0] != '\0')
    {
        ds_strnzcpy(pVoipServer->strHostname, pVoipServer->Config.strHostName, sizeof(pVoipServer->strHostname));
        DirtyCastLogPrintf("voipserver: host FQDN inherited from config -> %s\n", pVoipServer->strHostname);
    }
    else
    {
        struct sockaddr Addr;

        // initialize the socket structure required to initiate the reverse lookup
        SockaddrInit(&Addr, AF_INET);
        SockaddrInSetAddr(&Addr, pVoipServer->Config.uFrontAddr);
        SockaddrInSetPort(&Addr, pVoipServer->Config.uApiPort);

        if (DirtyCastGetHostNameByAddr(&Addr, sizeof(Addr), pVoipServer->strHostname, sizeof(pVoipServer->strHostname), NULL, 0) == 0)
        {
            DirtyCastLogPrintf("voipserver: reverse lookup succeeded for %a, host FQDN is %s\n",  pVoipServer->Config.uFrontAddr, pVoipServer->strHostname);
        }
        else
        {
            DirtyCastLogPrintf("voipserver: reverse lookup failed for %a - retry later under the assumption that network is not yet fully initialized\n", pVoipServer->Config.uFrontAddr);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipServerInitialize

    \Description
        Initializes the shared data and calls upon the mode specific code
        to initialize its data

    \Input *pVoipServer     - module state
    \Input iArgCt           - number of arguments
    \Input *pArgs           - command line arguments
    \Input *pConfigTagsFile - configuration filename

    \Output
        uint8_t - TRUE for success, FALSE otherwise

    \Version 09/18/2015 (eesponda)
*/
/********************************************************************************F*/
static uint8_t _VoipServerInitialize(VoipServerRefT *pVoipServer, int32_t iArgCt, const char *pArgs[], const char *pConfigTagsFile)
{
    const char *pConfigTags, *pCommandTags;
    const VoipServerDispatchT *pDispatch;
    int32_t iResult;

    // parse command line parameters from tagfields
    pCommandTags = DirtyCastCommandLineProcess(pVoipServer->strCmdTagBuf, sizeof(pVoipServer->strCmdTagBuf), 2, iArgCt, pArgs, "");

    // log startup and build options
    DirtyCastLogPrintf("voipserver: started debug=%d profile=%d strCmdTagBuf=%s\n", DIRTYCODE_DEBUG, DIRTYCODE_PROFILE, pVoipServer->strCmdTagBuf);

    pVoipServer->uCurrentStateStart = NetTick();

    // save config filename and load config filename
    ds_strnzcpy(pVoipServer->strCfgFileName, pConfigTagsFile, sizeof(pVoipServer->strCfgFileName));
    if (VoipServerLoadConfigBegin(&pVoipServer->pConfig, pVoipServer->strCfgFileName) < 0)
    {
        return(FALSE);
    }
    while ((iResult = VoipServerLoadConfigComplete(pVoipServer->pConfig, &pVoipServer->pConfigTags)) == 0)
        ;

    if (iResult < 0)
    {
        DirtyCastLogPrintf("voipserver: config loaded failed -- exiting\n");
        return(FALSE);
    }

    pConfigTags = pVoipServer->pConfigTags;

    // parse out all the config settings
    VoipServerSetConfigVars(&pVoipServer->Config, pCommandTags, pConfigTags);

    // create pid file using diagnostic port in name
    if (DirtyCastPidFileCreate(pVoipServer->strPidFilename, sizeof(pVoipServer->strPidFilename), pArgs[0], pVoipServer->Config.uDiagnosticPort) < 0)
    {
        DirtyCastLogPrintf("voipserver: error creating pid file -- exiting\n");
        return(FALSE);
    }

    // configure logger
    DirtyCastLoggerConfig(pConfigTags);

    // set the debug level
    SocketControl(NULL, 'spam', pVoipServer->Config.uDebugLevel, NULL, NULL);

    // log the front addr resolved by the config
    DirtyCastLogPrintfVerbose(pVoipServer->Config.uDebugLevel, 0, "voipserver: resolved front address to %a\n", pVoipServer->Config.uFrontAddr);

    /* Early attempt to find out the FQDN associated with this host.
       If this attempt fails internally, another attempt will be re-attempted from
       within VoipServerStatus('host') when that gets used. */
    _VoipServerAttemptFQDNLookup(pVoipServer);

    // create module to communicate with the subspace sidekick app
    if (pVoipServer->Config.uSubspaceSidekickPort != 0)
    {
        if ((pVoipServer->pSubspaceApi = SubspaceApiCreate(pVoipServer->Config.strSubspaceSidekickAddr, pVoipServer->Config.uSubspaceSidekickPort)) == NULL)
        {
            DirtyCastLogPrintf("voipserver: unable to create module subspaceapi.\n");
            return(FALSE);
        }
    }

    if (pVoipServer->Config.uMode != VOIPSERVER_MODE_QOS_SERVER)
    {
        // create prototunnel if enabled
        if ((pVoipServer->pProtoTunnel = _VoipServerCreateProtoTunnel(&pVoipServer->Config)) == NULL)
        {
            DirtyCastLogPrintf("voipserver: unable to create prototunnel module\n");
            return(FALSE);
        }

        // create voiptunnel module
        if ((pVoipServer->pVoipTunnel = _VoipServerCreateVoipTunnel(&pVoipServer->Config)) == NULL)
        {
            DirtyCastLogPrintf("voipserver: unable to create voiptunnel module\n");
            return(FALSE);
        }
        VoipTunnelCallback(pVoipServer->pVoipTunnel, _VoipServerVoipTunnelEventCallback, pVoipServer);

        // create virtual game socket
        if ((pVoipServer->pGameSocket = VoipServerSocketOpen(SOCK_DGRAM, 0, VOIPSERVERCONFIG_GAMEPORT, FALSE, _VoipServerGameRecvCallback, pVoipServer)) == NULL)
        {
                DirtyCastLogPrintf("voipserver: unable to virtual game socket; exiting\n");
            return(FALSE);
        }
    }

    // create token module
    if ((pVoipServer->pTokenRef = _VoipServerCreateTokenApi(&pVoipServer->Config)) == NULL)
    {
        DirtyCastLogPrintf("voipserver: unable to create tokenapi module\n");
        return(FALSE);
    }

    // set the callbacks based on mode
    if (pVoipServer->Config.uMode >= VOIPSERVER_MODE_NUMMODES)
    {
        DirtyCastLogPrintf("voipserver: mode specified out of range\n");
        return(FALSE);
    }
    pDispatch = &_VoipServer_ModeDispatch[pVoipServer->Config.uMode];

    // set the user data to the memory allocated by initialize
    if ((pVoipServer->pUserData = pDispatch->pInitializeCb(pVoipServer, pCommandTags, pConfigTags)) == NULL)
    {
        DirtyCastLogPrintf("voipserver: unable to initialize mode\n");
        return(FALSE);
    }
    pVoipServer->pDispatch = pDispatch;

    // create module to listen for diagnostic requests
    if (pVoipServer->Config.uDiagnosticPort != 0)
    {
        ServerDiagnosticGetResponseTypeCallbackT *pResponseCb = NULL;
        ServerDiagnosticCallbackT *pCallback = NULL;

        // get the callbacks based on mode
        if (pVoipServer->Config.uMode == VOIPSERVER_MODE_GAMESERVER)
        {
            pResponseCb = &GameServerDiagnosticGetResponseTypeCb;
            pCallback = &GameServerDiagnosticCallback;
        }
        else if (pVoipServer->Config.uMode == VOIPSERVER_MODE_CONCIERGE)
        {
            pResponseCb = &VoipConciergeDiagnosticGetResponseTypeCb;
            pCallback = &VoipConciergeDiagnosticCallback;
        }
        else if (pVoipServer->Config.uMode == VOIPSERVER_MODE_QOS_SERVER)
        {
            pResponseCb = &QosServerDiagnosticGetResponseTypeCb;
            pCallback = &QosServerDiagnosticCallback;
        }
        else
        {
            DirtyCastLogPrintf("voipserver: unable to get callbacks for diagnostics\n");
            return(FALSE);
        }

        if ((pVoipServer->pServerDiagnostic = ServerDiagnosticCreate(pVoipServer->Config.uDiagnosticPort, pCallback, pVoipServer->pUserData, "VoipServer", 4096, "voipserver.", pCommandTags, pConfigTags)) != NULL)
        {
            DirtyCastLogPrintf("voipserver: created diagnostic socket bound to port %d\n", pVoipServer->Config.uDiagnosticPort);

            // set additional callback type
            ServerDiagnosticCallback(pVoipServer->pServerDiagnostic, pCallback, pResponseCb);

            // let the serverdiagnostic module know what our tunnel port is, it needs it to add it as an http header to http responses to the vmapp
            ServerDiagnosticControl(pVoipServer->pServerDiagnostic, 'tprt', pVoipServer->Config.uTunnelPort, 0, NULL);
        }
        else
        {
            DirtyCastLogPrintf("voipserver: unable to create serverdiagnostic module\n");
            return(FALSE);
        }
    }

    // create module to report metrics to an external metrics aggregator
    if (pVoipServer->Config.uMetricsAggregatorPort != 0)
    {
        ServerMetricsCallbackT *pCallback = NULL;

        // get the callbacks based on mode
        if (pVoipServer->Config.uMode == VOIPSERVER_MODE_GAMESERVER)
        {
            pCallback = &GameServerMetricsCallback;
        }
        else if (pVoipServer->Config.uMode == VOIPSERVER_MODE_CONCIERGE)
        {
            pCallback = &ConciergeMetricsCallback;
        }
        else if (pVoipServer->Config.uMode == VOIPSERVER_MODE_QOS_SERVER)
        {
            pCallback = &QosMetricsCallback;
        }
        else
        {
            DirtyCastLogPrintf("voipserver: unable to get callbacks for ServerMetrics\n");
            return(FALSE);
        }

        if ((pVoipServer->pServerMetrics = ServerMetricsCreate(pVoipServer->Config.strMetricsAggregatorHost, pVoipServer->Config.uMetricsAggregatorPort, pVoipServer->Config.uMetricsPushingRate, pVoipServer->Config.eMetricsFormat, pCallback, pVoipServer->pUserData)) != NULL)
        {
            DirtyCastLogPrintf("voipserver: created module for reporting metrics to external aggregator (report rate = %d ms)\n", pVoipServer->Config.uMetricsPushingRate);
            ServerMetricsControl(pVoipServer->pServerMetrics, 'spam', pVoipServer->Config.uDebugLevel, 0, NULL);
        }
        else
        {
            DirtyCastLogPrintf("voipserver: unable to create module for reporting metrics to external aggregator\n");
            return(FALSE);
        }
    }

    // create AF_NETLINK socket, if successful kick off a query
    if ((pVoipServer->iUdpMetricsSocket = UdpMetricsSocketOpen()) >= 0)
    {
        const uint16_t uPort = (pVoipServer->Config.uMode != VOIPSERVER_MODE_QOS_SERVER) ? pVoipServer->Config.uTunnelPort : pVoipServer->Config.uApiPort;
        UdpMetricsQuerySend(pVoipServer->iUdpMetricsSocket, uPort);
    }

    // success!
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function _VoipServerClientSendto

    \Description
        Send data to specified destination client.

    \Input *pSocket     - socket to send on
    \Input *pClient     - pointer to client that we're sending the data to
    \Input *pPacketData - pointer to packet data
    \Input iPacketSize  - size of packet data
    \Input uDebugLevel  - used for debugging more verbose information

    \Output
        uint32_t    - amount of data sent

    \Notes
        This function is only exercised from VoipServerSend() which is only used
        with the OTP operational mode. Therefore, with DirtySDK 15.1.7.0.1 and
        beyond, we assume only the first entry of the aGameMappings[] array is used.

    \Version 03/08/2007 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _VoipServerClientSendto(SocketT *pSocket, const VoipTunnelClientT *pClient, const char *pPacketData, int32_t iPacketSize, uint8_t uDebugLevel)
{
    struct sockaddr SendAddr;
    int32_t iResult;

    if (pSocket == NULL)
    {
        NetPrintf(("voipserver: ignoring send request to clientId=0x%08x on NULL socket\n", pClient->uClientId));
        return(0);
    }

    // don't send if we don't have a remote address to send to
    if ((pClient->uRemoteAddr == 0) || (pClient->uRemoteGamePort == 0))
    {
        #if DIRTYVERS <= DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
        NetPrintfVerbose((uDebugLevel, 2, "voipserver: [gs%02d-out] ignoring send of %d byte packet to %a:%d clientid=0x%08x\n",
            pClient->iGameIdx, iPacketSize, pClient->uRemoteAddr, pClient->uRemoteGamePort, pClient->uClientId));
        #else
        NetPrintfVerbose((uDebugLevel, 2, "voipserver: [game-out-%04d] ignoring send of %d-byte packet to %a:%d clientid=0x%08x\n",
            pClient->aGameMappings[0].iGameIdx, iPacketSize, pClient->uRemoteAddr, pClient->uRemoteGamePort, pClient->uClientId));
        #endif
        return(0);
    }
    else
    {
        #if DIRTYVERS <= DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
        NetPrintfVerbose((uDebugLevel, 2, "voipserver: [gs%02d-out] %d byte packet to %a:%d clientid=0x%08x\n",
            pClient->iGameIdx, iPacketSize, pClient->uRemoteAddr, pClient->uRemoteGamePort, pClient->uClientId));
        #else
        NetPrintfVerbose((uDebugLevel, 2, "voipserver: [game-out-%04d] %d-byte packet to %a:%d clientid=0x%08x\n",
            pClient->aGameMappings[0].iGameIdx, iPacketSize, pClient->uRemoteAddr, pClient->uRemoteGamePort, pClient->uClientId));
        #endif
    }

    if (uDebugLevel > 3)
    {
        int32_t iPrintSize = iPacketSize;
        if ((iPrintSize > 64) && (uDebugLevel < 5))
        {
            iPrintSize = 64;
        }
        NetPrintMem(pPacketData, iPrintSize, "voipserver->client");
    }

    // set address to send to this client
    SockaddrInit(&SendAddr, AF_INET);
    SockaddrInSetAddr(&SendAddr, pClient->uRemoteAddr);
    SockaddrInSetPort(&SendAddr, pClient->uRemoteGamePort);
    if ((iResult = SocketSendto(pSocket, pPacketData, iPacketSize, 0, &SendAddr, sizeof(SendAddr))) == iPacketSize)
    {
        return(iPacketSize);
    }
    else
    {
        DirtyCastLogPrintf("voipserver: send of %d byte packet to clientId=0x%08x failed (err=%d)\n", iPacketSize, pClient->uClientId, iResult);
        return(0);
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipServerUpdateLoad

    \Description
        Update load average calculations (like top).

    \Input *pVoipServer - module state
    \Input uCurTick     - current millisecond tick count
    \Input bResetMetrics- if true, reset metrics accumulator
    \Input uResetTicks  - ticks since last metrics reset

    \Version 04/11/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerUpdateLoad(VoipServerRefT *pVoipServer, uint32_t uCurTick, uint8_t bResetMetrics, uint32_t uResetTicks)
{
    DirtyCastGetServerInfo(&pVoipServer->Stats.ServerInfo, uCurTick, bResetMetrics, uResetTicks);
}

/*F********************************************************************************/
/*!
    \Function _VoipServerUpdateLate

    \Description
        Calculate summed latency for all GameServers

    \Input *pVoipServer     - module state
    \Input uCurTick         - current tick count
    \Input bResetMetrics    - if true, reset metrics accumulator

    \Version 06/28/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerUpdateLate(VoipServerRefT *pVoipServer, uint32_t uCurTick, uint8_t bResetMetrics)
{
    // reset latency accumulator?
    if (bResetMetrics == FALSE)
    {
        return;
    }
    // no-op: in configurations where we don't have a tunnel, we don't report these metrics
    if (pVoipServer->pProtoTunnel == NULL)
    {
        return;
    }

    GameServerPacketUpdateLateStats(&pVoipServer->Stats.StatInfo.E2eLate, NULL);
    GameServerPacketUpdateLateStats(&pVoipServer->Stats.StatInfo.InbLate, NULL);
    GameServerPacketUpdateLateStats(&pVoipServer->Stats.StatInfo.OtbLate, NULL);
    GameServerPacketUpdateLateBinStats(&pVoipServer->Stats.StatInfo.LateBin, NULL);

    GameServerPacketUpdateLateStats(&pVoipServer->Stats.InbKernLate.Game, NULL);
    GameServerPacketUpdateLateStats(&pVoipServer->Stats.InbKernLate.Voip, NULL);

    pVoipServer->Stats.uRecvCalls = ProtoTunnelStatus(pVoipServer->pProtoTunnel, 'rcal', 0, NULL, 0);
    pVoipServer->Stats.uTotalRecv = ProtoTunnelStatus(pVoipServer->pProtoTunnel, 'rtot', 0, NULL, 0);
    pVoipServer->Stats.uTotalRecvSub = ProtoTunnelStatus(pVoipServer->pProtoTunnel, 'rsub', 0, NULL, 0);
    pVoipServer->Stats.uTotalPktsDiscard = ProtoTunnelStatus(pVoipServer->pProtoTunnel, 'dpkt', 0, NULL, 0);
}

/*F********************************************************************************/
/*!
    \Function _VoipServerUpdateTalkingClients

    \Description
        Calculate average number of talking clietns

    \Input *pVoipServer     - module state
    \Input uCurTick         - current tick count
    \Input bResetMetrics    - if true, reset metrics accumulator

    \Version 04/10/2017 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipServerUpdateTalkingClients(VoipServerRefT *pVoipServer, uint32_t uCurTick, uint8_t bResetMetrics)
{
    // no-op: in configurations where we don't have a tunnel, we don't report these metrics
    if (pVoipServer->pVoipTunnel == NULL)
    {
        return;
    }

    // reset accumulator?
    if (bResetMetrics)
    {
        if (pVoipServer->Stats.iTalkingClientsSamples != 0)
        {
            pVoipServer->Stats.iTalkingClientsAvg = pVoipServer->Stats.iTalkingClientsSum / pVoipServer->Stats.iTalkingClientsSamples;
        }

        pVoipServer->Stats.iTalkingClientsCur = 0;
        pVoipServer->Stats.iTalkingClientsSum = 0;
        pVoipServer->Stats.iTalkingClientsMax = 0;
        pVoipServer->Stats.iTalkingClientsSamples = 0;
    }
    else
    {
        pVoipServer->Stats.iTalkingClientsCur = VoipTunnelStatus(pVoipServer->pVoipTunnel, 'talk', -1, NULL, 0);
        pVoipServer->Stats.iTalkingClientsSum += pVoipServer->Stats.iTalkingClientsCur;
        if (pVoipServer->Stats.iTalkingClientsCur >= pVoipServer->Stats.iTalkingClientsMax)
        {
            pVoipServer->Stats.iTalkingClientsMax = pVoipServer->Stats.iTalkingClientsCur;
        }
        pVoipServer->Stats.iTalkingClientsSamples++;
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipServerUpdateMetrics

    \Description
        Update various server metrics, surfaced by diagnostic pages.

    \Input *pVoipServer     - module state
    \Input uCurTick         - current tick count

    \Version 06/02/2010 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerUpdateMetrics(VoipServerRefT *pVoipServer, uint32_t uCurTick)
{
    int32_t iResetTicks;
    const uint16_t uPort = (pVoipServer->Config.uMode != VOIPSERVER_MODE_QOS_SERVER) ? pVoipServer->Config.uTunnelPort : pVoipServer->Config.uApiPort;
    UdpMetricsT UdpMetrics;

    // rate limit metrics update to once a second
    if (NetTickDiff(uCurTick, pVoipServer->uLastMetricsUpdate) < VOIPSERVER_VS_STATRATE)
    {
        return;
    }
    pVoipServer->uLastMetricsUpdate = uCurTick;

    // if metrics accumulators have not been reset for a while, do it now
    if ((iResetTicks = NetTickDiff(uCurTick, pVoipServer->uLastMetricsReset)) > VOIPSERVER_MAXMETRICSRESETRATE)
    {
        pVoipServer->bResetMetrics = TRUE;
    }

    // update load calcs
    _VoipServerUpdateLoad(pVoipServer, uCurTick, pVoipServer->bResetMetrics, (uint32_t)iResetTicks);

    // update latency calcs
    _VoipServerUpdateLate(pVoipServer, uCurTick, pVoipServer->bResetMetrics);

    // update talking clients calcs
    _VoipServerUpdateTalkingClients(pVoipServer, uCurTick, pVoipServer->bResetMetrics);

    // update stat calcs
    GameServerUpdateGameStats(&pVoipServer->Stats.GameStats, &pVoipServer->Stats.GameStatsCur, &pVoipServer->Stats.GameStatsSum, &pVoipServer->Stats.GameStatsMax, TRUE);

    // update udp stats
    if (pVoipServer->iUdpMetricsSocket >= 0)
    {
        ds_memclr(&UdpMetrics, sizeof(UdpMetrics));

        UdpMetricsQueryRecv(pVoipServer->iUdpMetricsSocket, &UdpMetrics);
        UdpMetricsQuerySend(pVoipServer->iUdpMetricsSocket, uPort);

        // capture the maximum for reporting
        pVoipServer->UdpMetrics.uSendQueueLen = DS_MAX(pVoipServer->UdpMetrics.uSendQueueLen, UdpMetrics.uSendQueueLen);
        pVoipServer->UdpMetrics.uRecvQueueLen = DS_MAX(pVoipServer->UdpMetrics.uRecvQueueLen, UdpMetrics.uRecvQueueLen);
    }

    // if we reset metrics, update our tracking
    if (pVoipServer->bResetMetrics)
    {
        if (pVoipServer->pVoipTunnel != NULL)
        {
            pVoipServer->Stats.uLastTotalClientsAdded = pVoipServer->Stats.uTotalClientsAdded - VoipTunnelStatus(pVoipServer->pVoipTunnel, 'nusr', -1, NULL, 0);
            pVoipServer->Stats.uLastTotalGamesCreated = pVoipServer->Stats.uTotalGamesCreated - VoipTunnelStatus(pVoipServer->pVoipTunnel, 'ngam', 0, NULL, 0);
        }
        ds_memclr(&pVoipServer->UdpMetrics, sizeof(pVoipServer->UdpMetrics));
        pVoipServer->bResetMetrics = FALSE;
        pVoipServer->uLastMetricsReset = uCurTick;
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipServerUpdateProcessState

    \Description
        Pump prcoess state transitions

    \Input *pVoipServer  - module ref

    \Version 01/06/2020 (cvienneau)
*/
/********************************************************************************F*/
static void _VoipServerUpdateProcessState(VoipServerRefT *pVoipServer)
{
    /* enforce minimal state duration to make sure all recently entered states have time to propagate 
       to the metrics back-end before the subsequent state transition */
    if (pVoipServer->bCurrentStateMinDurationDone == FALSE)
    {
        const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer);
        int32_t iMinDuration = (pVoipServer->eProcessState == VOIPSERVER_PROCESS_STATE_EXITED) ? 0 : pConfig->iProcessStateMinDuration;

        if (NetTickDiff(NetTick(), pVoipServer->uCurrentStateStart) < iMinDuration)
        {
            return;
        }
        else
        {
            DirtyCastLogPrintf("voipserver: [autoscale] minimal duration (%d ms) complete for state %s\n",
                iMinDuration, _VoipServer_strProcessStates[pVoipServer->eProcessState]);
            pVoipServer->bCurrentStateMinDurationDone = TRUE;
        }
    }

    switch (pVoipServer->eProcessState)
    {
        case VOIPSERVER_PROCESS_STATE_RUNNING:
            // noop
            break;

        case VOIPSERVER_PROCESS_STATE_DRAINING:
            // wait for draining to complete
            if (pVoipServer->pDispatch->pDrainingCb(pVoipServer->pUserData))
            {
                DirtyCastLogPrintf("voipserver: draining complete (duration: ~ %d s)\n",
                    (NetTickDiff(NetTick(), pVoipServer->uCurrentStateStart) / 1000));

                VoipServerProcessMoveToNextState(pVoipServer, VOIPSERVER_PROCESS_STATE_EXITING);
            }
            break;

        case VOIPSERVER_PROCESS_STATE_EXITING:
            if (pVoipServer->bReady == TRUE)
            {
                // signal ourself as 'not ready' to the cloud fabric (kubernetes readiness probe)
                DirtyCastReadinessFileRemove();
            }

            /* minimal duration for 'exiting' completed
               we can assume that final metrics have made it to the metrics agent
               we can now move on to 'exited' which has minimal duration set to 0 ms */
            VoipServerProcessMoveToNextState(pVoipServer, VOIPSERVER_PROCESS_STATE_EXITED);
            break;

        case VOIPSERVER_PROCESS_STATE_EXITED:
            // noop
            break;

        default:
            DirtyCastLogPrintf("voipserver: fatal error - unknown process state %s ... explicitly exiting\n", pVoipServer->eProcessState);
            break;
    }
}


/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function VoipServerCreate

    \Description
        Allocate module state and prepare for use

    \Output
        VoipServerRefT *   - pointer to module state, or NULL

    \Version 09/18/2015 (eesponda)
*/
/********************************************************************************F*/
VoipServerRefT *VoipServerCreate(int32_t iArgCt, const char *pArgs[], const char *pConfigTagsFile)
{
    VoipServerRefT *pVoipServer;
    int32_t iMemGroup;
    void *pMemGroupUserdata;

    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserdata);

    if ((pVoipServer = (VoipServerRefT *)DirtyMemAlloc(sizeof(VoipServerRefT), VOIPSERVER_MEMID, iMemGroup, pMemGroupUserdata)) == NULL)
    {
        DirtyCastLogPrintf("voipserver: could not allocate module ref\n");
        return(NULL);
    }

    ds_memclr(pVoipServer, sizeof(VoipServerRefT));
    pVoipServer->iMemGroup = iMemGroup;
    pVoipServer->pMemGroupUserdata = pMemGroupUserdata;

    // save startup time
    pVoipServer->Stats.uStartTime = time(NULL);
    // metrics update tracking info
    pVoipServer->uLastMetricsUpdate = pVoipServer->uLastMetricsReset = NetTick();

#if DIRTYCODE_DEBUG
    pVoipServer->iFakeCpu = -1; // default to disabled
#endif

    if (!_VoipServerInitialize(pVoipServer, iArgCt, pArgs, pConfigTagsFile))
    {
        VoipServerDestroy(pVoipServer);
        return(NULL);
    }

    return(pVoipServer);
}

/*F********************************************************************************/
/*!
    \Function VoipServerUpdate

    \Description
        Updates the shared modules and calls upon the process callback tokenapi
        do its work

    \Input *pVoipServer     - module state
    \Input *pSignalFlags    - flags passed to the app

    \Output
        uint8_t - TRUE for success, FALSE otherwise

    \Version 09/18/2015 (eesponda)
*/
/********************************************************************************F*/
uint8_t VoipServerUpdate(VoipServerRefT *pVoipServer, uint32_t *pSignalFlags)
{
    uint8_t bDone = FALSE;

    uint32_t uCurTick, uLoopTick;
    const VoipServerDispatchT *pDispatch = pVoipServer->pDispatch;

    // if we have not yet signaled ourself as ready, check if we should
    if (pVoipServer->bReady == FALSE)
    {
        if (pVoipServer->pDispatch->pStatusCb(pVoipServer->pUserData, 'read', 0, NULL, 0) == TRUE)
        {
            // signal ourself as 'ready' to the cloud fabric (kubernetes readiness probe)
            if (DirtyCastReadinessFileCreate() < 0)
            {
                return(FALSE);
            }
            pVoipServer->bReady = TRUE;
        }
    }

    _VoipServerUpdateProcessState(pVoipServer);

    // update the tokenapi
    TokenApiUpdate(pVoipServer->pTokenRef);

    // determine what poll timeout to use.
    if (pVoipServer->bModuleExited == FALSE)
    {
        /* check if the modules are performing work that we need to drop the poll time
        down as not to have them starved */
        bDone = pDispatch->pStatusCb(pVoipServer->pUserData, 'done', 0, NULL, 0);

        // execute a blocking poll on all of our possible inputs
        NetConnControl('poll', bDone ? 1000 : 1, 0, NULL, NULL);
    }
    else
    {
        /* when the "final metrics report" phase is initiated, we can't use
           NetConnControl('poll') because it may result in high cpu usage
           caused by the poll no longer blocking because of receive-ready
           sockets no longer being processed by the main module */
        NetConnSleep(30);
    }

    // update tick count after poll
    uCurTick = uLoopTick = NetTick();

    // update idle processing
    NetConnIdle();

    // update voiptunnel
    if (pVoipServer->pVoipTunnel != NULL)
    {
        VoipTunnelUpdate(pVoipServer->pVoipTunnel);
    }

    // update config reload
    _VoipServerUpdateConfig(pVoipServer);

    // update server diagnostic
    if (pVoipServer->pServerDiagnostic != NULL)
    {
        ServerDiagnosticUpdate(pVoipServer->pServerDiagnostic);
    }

    // update ServerMetrics module
    if (pVoipServer->pServerMetrics != NULL)
    {
        ServerMetricsUpdate(pVoipServer->pServerMetrics);
    }

    // update the SubspaceApi module
    if (pVoipServer->pSubspaceApi != NULL)
    {
        SubspaceApiUpdate(pVoipServer->pSubspaceApi);
    }

    // update signal flags (may be set by diagnostic action)
    if (pVoipServer->uSignalFlags != 0)
    {
        *pSignalFlags = pVoipServer->uSignalFlags;
    }

    // handle the reload configuration signal
    if (*pSignalFlags & VOIPSERVER_SIGNAL_CONFIG_RELOAD)
    {
        pVoipServer->bConfigReloading = (VoipServerLoadConfigBegin(&pVoipServer->pConfig, pVoipServer->strCfgFileName) == 0);
        *pSignalFlags &= ~VOIPSERVER_SIGNAL_CONFIG_RELOAD;
    }
    // handle immediate shutdown
    if (*pSignalFlags & VOIPSERVER_SIGNAL_SHUTDOWN_IMMEDIATE)
    {
        DirtyCastLogPrintf("voipserver: executing immediate shutdown\n");
        return(FALSE);
    }

    if (pVoipServer->bModuleExited == FALSE)
    {
        // update mode module
        if (pDispatch->pProcessCb(pVoipServer->pUserData, pSignalFlags, uCurTick) == FALSE)
        {
            if (pVoipServer->pServerMetrics != NULL)
            {
                DirtyCastLogPrintfVerbose(pVoipServer->Config.uDebugLevel, 0, "voipserver: initiating final best effort to push latest metrics to aggregator\n");

                // the mode has told us to stop
                pVoipServer->bModuleExited = TRUE;

                // initiate "final metrics report" phase - i.e. best effort to update metrics aggregator with most recent metric values

                // make metrics reporting rate much faster during the "final metrics report" phase
                ServerMetricsControl(pVoipServer->pServerMetrics, 'rate', pVoipServer->Config.uMetricsFinalPushingRate, 0, NULL);

                // force an immediate metrics report to make sure updated metrics propagate quick to the metrics backend
                ServerMetricsControl(pVoipServer->pServerMetrics, 'push', 0, 0, NULL);

                // save time at wich the "final metrics report" phase starts.
                pVoipServer->uFinalMetricsReportStart = NetTick();
            }
            else
            {
                return(FALSE);
            }
        }
    }
    else
    {
        if (NetTickDiff(NetTick(), pVoipServer->uFinalMetricsReportStart) > pVoipServer->Config.iFinalMetricsReportDuration)
        {
            DirtyCastLogPrintfVerbose(pVoipServer->Config.uDebugLevel, 0, "voipserver: best effort to push latest metrics to aggregator complete (%d s)... exiting for good!\n", (pVoipServer->Config.iFinalMetricsReportDuration / 1000));
            return(FALSE);
        }
    }

    // cache current signal flags
    pVoipServer->uSignalFlags = *pSignalFlags;

    // flush any packets queued during the mode module update
    if (pVoipServer->pProtoTunnel != NULL)
    {
        ProtoTunnelUpdate(pVoipServer->pProtoTunnel);
    }

    // update metrics
    _VoipServerUpdateMetrics(pVoipServer, uCurTick);

    // warn if main loop takes too long (don't print when doing known expensive operations)
    if ((bDone == TRUE) && ((uLoopTick = NetTickDiff(NetTick(), uLoopTick)) > 3))
    {
        DirtyCastLogPrintfVerbose(pVoipServer->Config.uDebugLevel, 1, "voipserver: main loop took %dms\n", uLoopTick);
    }

    // continue processing
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function VoipServerGetVoipTunnel

    \Description
        Gets the VoipTunnel instance

    \Input *pVoipServer     - module state

    \Output
        VoipTunnelRefT * - The instance of the VoipTunnel

    \Version 09/21/2015 (eesponda)
*/
/********************************************************************************F*/
VoipTunnelRefT *VoipServerGetVoipTunnel(const VoipServerRefT *pVoipServer)
{
    return(pVoipServer->pVoipTunnel);
}

/*F********************************************************************************/
/*!
    \Function VoipServerGetProtoTunnel

    \Description
        Gets the ProtoTunnel instance

    \Input *pVoipServer     - module state

    \Output
        ProtoTunnelRefT * - The instance of the ProtoTunnel

    \Version 09/21/2015 (eesponda)
*/
/********************************************************************************F*/
ProtoTunnelRefT *VoipServerGetProtoTunnel(const VoipServerRefT *pVoipServer)
{
    return(pVoipServer->pProtoTunnel);
}

/*F********************************************************************************/
/*!
    \Function VoipServerGetTokenApi

    \Description
        Gets the TokenApi instance

    \Input *pVoipServer     - module state

    \Output
        TokenApiRefT * - The instance of TokenApi

    \Version 09/21/2015 (eesponda)
*/
/********************************************************************************F*/
TokenApiRefT *VoipServerGetTokenApi(const VoipServerRefT *pVoipServer)
{
    return(pVoipServer->pTokenRef);
}

/*F********************************************************************************/
/*!
    \Function VoipServerGetConfig

    \Description
        Gets the configuration for the VoipServer

    \Input *pVoipServer     - module state

    \Output
        const VoipServerConfigT *   - All the configuration for the voipserver

    \Version 09/21/2015 (eesponda)
*/
/********************************************************************************F*/
const VoipServerConfigT *VoipServerGetConfig(const VoipServerRefT *pVoipServer)
{
    return(&pVoipServer->Config);
}

/*F********************************************************************************/
/*!
    \Function VoipServerSend

    \Description
        Sends packet data to the destination

    \Input *pVoipServer     - module state
    \Input uSrcClientId     - who sent the data
    \Input uDstClientId     - where the data is going to (0 for broadcast)
    \Input *pData           - packet data
    \Input iDataSize        - size of the data

    \Version 03/31/2006 (jbrookes)
*/
/********************************************************************************F*/
void VoipServerSend(VoipServerRefT *pVoipServer, uint32_t uSrcClientId, uint32_t uDstClientId, const char *pData, int32_t iDataSize)
{
    VoipTunnelClientT *pDstClient;

    // send to specified dest client
    if ((pDstClient = VoipTunnelClientListMatchId(pVoipServer->pVoipTunnel, uDstClientId)) != NULL)
    {
        uint32_t uByteSent;
        // send to dst client
        if ((uByteSent = _VoipServerClientSendto(pVoipServer->pGameSocket, pDstClient, pData, iDataSize, pVoipServer->Config.uDebugLevel)) > 0)
        {
            // accumulate data sent information
            pVoipServer->Stats.GameStats.GameStat.uByteSent += VOIPSERVER_EstimateNetworkPacketSize(uByteSent);
            pVoipServer->Stats.GameStats.GameStat.uPktsSent += 1;

            _VoipServerGameEvent(pVoipServer, VOIPSERVER_EVENT_SENDGAME, pDstClient, pData, uByteSent, NetTick());
        }
    }
    else
    {
        NetPrintf(("voipserver: unable to resolve clientId=0x%08x for send\n", uDstClientId));
    }
}

/*F********************************************************************************/
/*!
    \Function VoipServerProcessMoveToNextState

    \Description
        Perform autoscale state transition

    \Input *pVoipServer     - mode state
    \Input eNewState        - new state

    \Version 01/06/2020 (cvienneau)
*/
/********************************************************************************F*/
void VoipServerProcessMoveToNextState(VoipServerRefT *pVoipServer, VoipServerProcessStateE eNewState)
{
    uint32_t uCurrTick = NetTick();
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer);

    if (eNewState != pVoipServer->eProcessState)
    {
        DirtyCastLogPrintf("voipgameserver: [autoscale] state transition %s -> %s (min dur %dms).\n",
            _VoipServer_strProcessStates[pVoipServer->eProcessState],
            _VoipServer_strProcessStates[eNewState],
            pConfig->iProcessStateMinDuration);

        pVoipServer->uCurrentStateStart = uCurrTick;
        pVoipServer->bCurrentStateMinDurationDone = FALSE;

        // force an immediate metrics report to make sure state transitions propagate quick to the metrics backend
        VoipServerControl(pVoipServer, 'push', 0, 0, NULL);
    }

    // state transition
    pVoipServer->eProcessState = eNewState;
}

/*F********************************************************************************/
/*!
    \Function VoipServerStatus

    \Description
        Get module status

    \Input *pVoipServer     - pointer to module state
    \Input iSelect          - status selector
    \Input iValue           - selector specific
    \Input *pBuf            - [out] - selector specific
    \Input iBufSize         - size of output buffer

    \Output
        int32_t             - selector specific

    \Notes
        iSelect can be one of the following:

        \verbatim
            'epct' - return ratio representing how much of our allowed CPU budget is consumed (stands for 'Efficient CPU')
            'fcpu' - [debug only] get fake CPU% consumed by VS
            'host' - return our hostname via pBuf
            'merr' - return monitor error flags
            'mwrn' - return monitor warning flags
            'pmti' - return true if the process state has elapsed its min time
            'psta' - return the process's current state
            'sgnl' - return the cached signal flags
            'stat' - return the voipserver stats via pBuf
            'udpm' - return the udpmetrics stats via pBuf
        \endverbatim

    \Version 09/18/2015 (eesponda)
*/
/********************************************************************************F*/
int32_t VoipServerStatus(VoipServerRefT *pVoipServer, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    if (iSelect == 'epct')
    {
        const VoipServerConfigT *pConfig = &pVoipServer->Config;
        float *pConsumedCpuBudget = (float *)pBuf;

        /* Fill the caller's buffer with a value between 0 and 1 representing the ratio of CPU used in our CPU budget
           below the WARNING threshold.
           If running above warning level, then report full. */
        *pConsumedCpuBudget = pVoipServer->Stats.ServerInfo.fCpu / (float)pConfig->MonitorWarnings.uPctServerLoad;
        if (*pConsumedCpuBudget > 1.0f)
        {
            *pConsumedCpuBudget = 1.0f;
        }

        return(0);
    }
#if DIRTYCODE_DEBUG
    if (iSelect == 'fcpu')
    {
        return(pVoipServer->iFakeCpu);
    }
#endif
    if (iSelect == 'host')
    {
        if (pBuf != NULL)
        {
            // if FQDN not yet know, then re-attempt getting it
            if (pVoipServer->strHostname[0] == '\0')
            {
                _VoipServerAttemptFQDNLookup(pVoipServer);
            }

            ds_strnzcpy((char *)pBuf, pVoipServer->strHostname, iBufSize);
            return(0);
        }
        return(-1);
    }
    if (iSelect == 'merr')
    {
        return(pVoipServer->uMonitorFlagErrors);
    }
    if (iSelect == 'mwrn')
    {
        return(pVoipServer->uMonitorFlagWarnings);
    }
    if (iSelect == 'pmti')
    {
        return(pVoipServer->bCurrentStateMinDurationDone);
    }
    if (iSelect == 'psta')
    {
        return(pVoipServer->eProcessState);
    }
    if (iSelect == 'sgnl')
    {
        return(pVoipServer->uSignalFlags);
    }
    if (iSelect == 'stat')
    {
        if ((pBuf != NULL) && (iBufSize >= (int32_t)sizeof(pVoipServer->Stats)))
        {
            ds_memcpy(pBuf, &pVoipServer->Stats, sizeof(pVoipServer->Stats));
            return(0);
        }
        return(-1);
    }
    if (iSelect == 'udpm')
    {
        if ((pBuf != NULL) && (iBufSize >= (int32_t)sizeof(pVoipServer->UdpMetrics)))
        {
            ds_memcpy(pBuf, &pVoipServer->UdpMetrics, sizeof(pVoipServer->UdpMetrics));
            return(0);
        }
        return(-1);
    }
    // selector not supported
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function VoipServerControl

    \Description
        Module control function

    \Input *pVoipServer     - pointer to module state
    \Input iControl         - control selector
    \Input iValue           - selector specific
    \Input iValue2          - selector specific
    \Input *pValue          - selector specific

    \Output
        int32_t             - selector specific

    \Notes
        iControl can be one of the following:

        \verbatim
            'e2el' - adds new latency info for E2E latency bucket (via pValue)
            'fcpu' - [debug only] set fake CPU% consumed by VS
            'gsta' - reset game stats
            'inbl' - adds new latency info for inbound latency bucket (via pValue)
            'lbin' - adds new latency info for binary latency bucket (via pValue)
            'otbl' - adds new latency info for outbound latency bucket (via pValue)
            'push' - force an immediate metrics report
            'sgnl' - sets the cached signal flags
            'spam' - sets the debug level
            'stat' - queue resetting of stats based on iValue (TRUE/FALSE)
            'uerr' - update last reported udp error count (to be used by the metrics reporting layer)
            'urcv' - update last reported udp receive count (to be used by the metrics reporting layer)
            'usnt' - update last reported udp sent count (to be used by the metrics reporting layer)
        \endverbatim

        Unhandled controls will fallthrough to the mode specific control function

    \Version 12/02/2015 (eesponda)
*/
/********************************************************************************F*/
int32_t VoipServerControl(VoipServerRefT *pVoipServer, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iControl == 'e2el')
    {
        GameServerPacketUpdateLateStats(&pVoipServer->Stats.StatInfo.E2eLate, (const GameServerLateT *)pValue);
        return(0);
    }
#if DIRTYCODE_DEBUG
    if (iControl == 'fcpu')
    {
        DirtyCastLogPrintf("voipserver: [debug] FAKE CPU changed from %d to %d\n", pVoipServer->iFakeCpu, iValue);
        pVoipServer->iFakeCpu = iValue;
        return(0);
    }
#endif
    if (iControl == 'gsta')
    {
        pVoipServer->Stats.uTotalClientsAdded = (uint32_t)VoipTunnelStatus(pVoipServer->pVoipTunnel, 'nusr', -1, NULL, 0);
        pVoipServer->Stats.uTotalGamesCreated = (uint32_t)VoipTunnelStatus(pVoipServer->pVoipTunnel, 'ngam', 0, NULL, 0);
        return(0);
    }
    if (iControl == 'inbl')
    {
        GameServerPacketUpdateLateStats(&pVoipServer->Stats.StatInfo.InbLate, (const GameServerLateT *)pValue);
        return(0);
    }
    if (iControl == 'lbin')
    {
        GameServerPacketUpdateLateBinStats(&pVoipServer->Stats.StatInfo.LateBin, (const GameServerLateBinT *)pValue);
        return(0);
    }
    if (iControl == 'otbl')
    {
        GameServerPacketUpdateLateStats(&pVoipServer->Stats.StatInfo.OtbLate, (const GameServerLateT *)pValue);
        return(0);
    }
    if (iControl == 'push')
    {
        if (pVoipServer->pServerMetrics != NULL)
        {
            return(ServerMetricsControl(pVoipServer->pServerMetrics, iControl, 0, 0, NULL));
        }
        return(-1);
    }
    if (iControl == 'sgnl')
    {
        pVoipServer->uSignalFlags = (uint32_t)iValue;
        return(0);
    }
    if (iControl == 'spam')
    {
        pVoipServer->Config.uDebugLevel = (uint32_t)iValue;

        // update internal modules debug level
        if (pVoipServer->pVoipTunnel != NULL)
        {
            VoipTunnelControl(pVoipServer->pVoipTunnel, iControl, iValue, 0, NULL);
        }
        if (pVoipServer->pProtoTunnel != NULL)
        {
            ProtoTunnelControl(pVoipServer->pProtoTunnel, iControl, iValue, 0, NULL);
        }
        if (pVoipServer->pServerMetrics != NULL)
        {
            ServerMetricsControl(pVoipServer->pServerMetrics, iControl, iValue, 0, NULL);
        }
        TokenApiControl(pVoipServer->pTokenRef, iControl, iValue, 0, NULL);
        SocketControl(NULL, iControl, iValue, 0, NULL);

        // fallthrough to mode specific control
    }
    if (iControl == 'stat')
    {
        pVoipServer->bResetMetrics = (uint8_t)iValue;
        return(0);
    }
    if (iControl == 'uerr')
    {
        pVoipServer->Stats.uTotalPktsUdpErrorCount = *((uint64_t *)pValue);
        return(0);
    }
    if (iControl == 'urcv')
    {
        pVoipServer->Stats.uTotalPktsUdpReceivedCount = *((uint64_t *)pValue);
        return(0);
    }
    if (iControl == 'usnt')
    {
        pVoipServer->Stats.uTotalPktsUdpSentCount = *((uint64_t *)pValue);
        return(0);
    }

    // unhandled, fallthrough to mode specific control function
    return(pVoipServer->pDispatch->pControlCb(pVoipServer->pUserData, iControl, iValue, iValue2, pValue));
}

/*F********************************************************************************/
/*!
    \Function VoipServerCallback

    \Description
        Set the callback for data coming in

    \Input *pVoipServer     - pointer to module state
    \Input *pCallback       - pointer to the callback
    \Input *pUserData       - user specific data

    \Version 09/18/2015 (eesponda)
*/
/********************************************************************************F*/
void VoipServerCallback(VoipServerRefT *pVoipServer, GameEventCallbackT *pGameCallback, VoipTunnelCallbackT *pVoipCallback, void *pUserData)
{
    pVoipServer->pGameCallback = pGameCallback;
    pVoipServer->pVoipCallback = pVoipCallback;
    pVoipServer->pCallbackUserData = pUserData;
}

/*F********************************************************************************/
/*!
    \Function VoipServerUpdateMonitorFlag

    \Description
        Update monitor flag state based on input criteria.

    \Input *pVoipServer     - voipserver module state
    \Input bFailWrn         - TRUE/FALSE - did test fail warning threshold?
    \Input bFailErr         - TRUE/FALSE - did test fail error threshold?
    \Input uFailFlag        - flag to set if a test fails
    \Input bClearOnly       - only allow clearing flag, not setting it

    \Output
        None.

    \Version 10/31/2007 (jbrookes)
*/
/********************************************************************************F*/
void VoipServerUpdateMonitorFlag(VoipServerRefT *pVoipServer, uint32_t bFailWrn, uint32_t bFailErr, uint32_t uFailFlag, uint32_t bClearOnly)
{
    // check whether we should set a warning or error
    if (!bClearOnly)
    {
        if (bFailErr)
        {
            pVoipServer->uMonitorFlagErrors |= uFailFlag;
        }
        else if (bFailWrn)
        {
            pVoipServer->uMonitorFlagWarnings |= uFailFlag;
        }
    }

    // check if we should clear a warning or error
    if (!bFailErr)
    {
        pVoipServer->uMonitorFlagErrors &= ~uFailFlag;
    }
    if (!bFailWrn)
    {
        pVoipServer->uMonitorFlagWarnings &= ~uFailFlag;
    }
}

/*F********************************************************************************/
/*!
    \Function VoipServerMonitorFlagCheck

    \Description
        Check monitor warning and error flags and outputs monitor strings as
        appropriate.  Also keeps track of overall warning/error state.

    \Input *pVoipServer     - voipserver module state
    \Input *pDiagnostic     - monitor info
    \Input *pStatus         - [in/out] overall failure/warning status tracker
    \Input *pBuffer         - [out] output for diagnostic text
    \Input iBufSize         - size of diagnostic text buffer

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 08/17/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipServerMonitorFlagCheck(VoipServerRefT *pVoipServer, const VoipServerDiagnosticMonitorT *pDiagnostic, uint32_t *pStatus, char *pBuffer, int32_t iBufSize)
{
    int32_t iOffset = 0;
    if (pVoipServer->uMonitorFlagErrors & pDiagnostic->uMonitorFlag)
    {
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "ERR: %s\n", pDiagnostic->pMonitorMessage);
        *pStatus = 2;
    }
    else if (pVoipServer->uMonitorFlagWarnings & pDiagnostic->uMonitorFlag)
    {
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "WRN: %s\n", pDiagnostic->pMonitorMessage);
        if (*pStatus == 0)
        {
            *pStatus = 1;
        }
    }
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function VoipServerDestroy

    \Description
        Calls the shutdown callback and cleans up the shared data

    \Input *pVoipServer     - module state

    \Version 09/18/2015 (eesponda)
*/
/********************************************************************************F*/
void VoipServerDestroy(VoipServerRefT *pVoipServer)
{
    // close the AF_NETLINK socket
    if (pVoipServer->iUdpMetricsSocket >= 0)
    {
        UdpMetricsSocketClose(pVoipServer->iUdpMetricsSocket);
        pVoipServer->iUdpMetricsSocket = -1;
    }

    // destroy serverdiagnostic module
    if (pVoipServer->pServerDiagnostic != NULL)
    {
        ServerDiagnosticDestroy(pVoipServer->pServerDiagnostic);
        pVoipServer->pServerDiagnostic = NULL;
    }

    // destroy SubspaceApi module
    if (pVoipServer->pSubspaceApi != NULL)
    {
        SubspaceApiDestroy(pVoipServer->pSubspaceApi);
        pVoipServer->pSubspaceApi = NULL;
    }

    // destroy ServerMetrics module
    if (pVoipServer->pServerMetrics != NULL)
    {
        ServerMetricsDestroy(pVoipServer->pServerMetrics);
        pVoipServer->pServerMetrics = NULL;
    }

    // cleanup mode
    if (pVoipServer->pUserData != NULL)
    {
        const VoipServerDispatchT *pDispatch = pVoipServer->pDispatch;
        pDispatch->pShutdownCb(pVoipServer->pUserData);
        pVoipServer->pUserData = NULL;
    }

    // destroy virtual game socket
    if (pVoipServer->pGameSocket != NULL)
    {
        SocketClose(pVoipServer->pGameSocket);
        pVoipServer->pGameSocket = NULL;
    }

    // destroy tokenapi
    if (pVoipServer->pTokenRef != NULL)
    {
        TokenApiDestroy(pVoipServer->pTokenRef);
        pVoipServer->pTokenRef = NULL;
    }

    // destroy voiptunnel module
    if (pVoipServer->pVoipTunnel != NULL)
    {
        VoipTunnelDestroy(pVoipServer->pVoipTunnel);
        pVoipServer->pVoipTunnel = NULL;
    }

    // destroy prototunnel module
    if (pVoipServer->pProtoTunnel != NULL)
    {
        ProtoTunnelDestroy(pVoipServer->pProtoTunnel);
        pVoipServer->pProtoTunnel = NULL;
    }

    // destroy config file
    if (pVoipServer->pConfig != NULL)
    {
        ConfigFileDestroy(pVoipServer->pConfig);
        pVoipServer->pConfig = NULL;
    }

    // remove the pid file
    DirtyCastPidFileRemove(pVoipServer->strPidFilename);

    // cleanup memory
    DirtyMemFree(pVoipServer, VOIPSERVER_MEMID, pVoipServer->iMemGroup, pVoipServer->pMemGroupUserdata);
}

/*F********************************************************************************/
/*!
    \Function VoipServerGetServerMetrics

    \Description
        Return the ServerMetricsT reference

    \Input *pVoipServer     - module state

    \Output
        ServerMetricsT*     - reference to associated ServerMetricsT

    \Version 02/14/2018 (amakoukji)
*/
/********************************************************************************F*/
ServerMetricsT *VoipServerGetServerMetrics(VoipServerRefT *pVoipServer)
{
    return(pVoipServer->pServerMetrics);
}

