/*H********************************************************************************/
/*!
    \File connapi.c

    \Description
        ConnApi is a high-level connection manager, that packages the "connect to
        peer" process into a single module.  Both game connections and voice
        connections can be managed.  Multiple peers are supported in a host/client
        model for the game connection, and a peer/peer model for the voice
        connections.

    \Copyright
        Copyright (c) Electronic Arts 2005. ALL RIGHTS RESERVED.

    \Version 01/04/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtynames.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/proto/protomangle.h"
#include "DirtySDK/proto/prototunnel.h"
#include "DirtySDK/proto/protoupnp.h"
#include "DirtySDK/voip/voip.h"
#include "DirtySDK/voip/voipgroup.h"

#include "DirtySDK/game/connapi.h"

/*** Defines **********************************************************************/

//! default connapi timeout
#define CONNAPI_TIMEOUT_DEFAULT     (15*1000)

//! connapi connection timeout
#define CONNAPI_CONNTIMEOUT_DEFAULT (10*1000)

//! connapi default demangler timeout (per user)
#if defined(DIRTYCODE_XBOXONE)
#define CONNAPI_DEMANGLER_TIMEOUT               (2*CONNAPI_CONNTIMEOUT_DEFAULT)  // experimentation showed that creation of security association can be significantly long
#define CONNAPI_DEMANGLER_WITH_FAILOVER_TIMEOUT (CONNAPI_CONNTIMEOUT_DEFAULT)    // however for scenarios involving CC assistance, we don't wait that long
#else
#define CONNAPI_DEMANGLER_TIMEOUT               (CONNAPI_CONNTIMEOUT_DEFAULT)
#define CONNAPI_DEMANGLER_WITH_FAILOVER_TIMEOUT (CONNAPI_DEMANGLER_TIMEOUT)
#endif

//! default GameLink buffer size
#define CONNAPI_LINKBUFDEFAULT      (1024)

//! test demangling
#define CONNAPI_DEMANGLE_TEST       (DIRTYCODE_DEBUG && FALSE)

//! random demangle local port?
#define CONNAPI_RNDLCLDEMANGLEPORT  (0) // used to be enabled for WII revolution platform

//! max number of registered callbacks
#define CONNAPI_MAX_CALLBACKS       (8)

//! connapi client flags
#define CONNAPI_CLIENTFLAG_REMOVE               (1) // remove client from clientlist
#define CONNAPI_CLIENTFLAG_TUNNELPORTDEMANGLED  (2) // tunnel port has been demangled
#define CONNAPI_CLIENTFLAG_SECUREADDRRESOLVED   (4) // secure address has been resolved (xboxone-specific)

//! debug flags
#define CONNAPI_CLIENTFLAG_P2PFAILDBG           (128)  // set to force P2P connections to fail

/*** Type Definitions *************************************************************/
struct ConnApiRefT
{
    //! connapi user callback info
    ConnApiCallbackT        *pCallback[CONNAPI_MAX_CALLBACKS];
    void                    *pUserData[CONNAPI_MAX_CALLBACKS];

    //! dirtymem memory group
    int32_t                 iMemGroup;
    void                    *pMemGroupUserData;

    //! game port to connect on
    uint16_t                uGamePort;

    //! voip port to connect on
    uint16_t                uVoipPort;

    //! game connection flags
    uint16_t                uConnFlags;

    //! connection mode
    uint16_t                uGameServConnMode;

    //! netmask, used for external address comparisons
    uint32_t                uNetMask;

    //! game name
    char                    strGameName[32];

    //! game server name; empty if not using a gameserver
    char                    strGameServName[32];

    //! game server address; zero if not using a gameserver
    uint32_t                uGameServAddr;

    //! game link buffer size
    int32_t                 iLinkBufSize;

    //! master game util ref (used for advertising)
    NetGameUtilRefT         *pGameUtilRef;

    //! protomangle ref
    ProtoMangleRefT         *pProtoMangle;

    //! prototunnel ref
    ProtoTunnelRefT         *pProtoTunnel;

    //! prototunnel port
    int32_t                 iTunnelPort;

    //! do we own tunnel ref?
    int32_t                 bTunnelOwner;

    //! protoupnp ref
    ProtoUpnpRefT           *pProtoUpnp;

    //! protomangle server name
    char                    strDemanglerServer[48];

    //! voip ref
    VoipRefT                *pVoipRef;

    //! voipgroup ref
    VoipGroupRefT           *pVoipGroupRef;

    //! do we own voip ref?
    uint32_t                bVoipOwner;

    //! comm construct function
    CommAllConstructT       *pCommConstruct;

    //! our address
    DirtyAddrT              SelfAddr;

    //! our unique identifier
    uint32_t                uSelfId;

    //! index of ourself in client list
    int32_t                 iSelf;

    //! session identifier
    int32_t                 iSessId;

    //! connection timeout value
    int32_t                 iConnTimeout;

    //! timeout value
    int32_t                 iTimeout;

    //! demangler timeouts
    int32_t                 iConfigMnglTimeout;         // configured timeout for cases where CC assistance are not applicable
    int32_t                 iConfigMnglTimeoutFailover; // configured timeout for cases where CC assistance are applicable
    int32_t                 iCurrentMnglTimeout;        // effective timeout value to be passed down to ProtoMangle

    //! gamelink configuration - input packet queue size
    int32_t                 iGameMinp;

    //! gamelink configuration - output packet queue size
    int32_t                 iGameMout;

    //! gamelink configuration - max packet width
    int32_t                 iGameMwid;

    //! gamelink configuration - unacknowledged packet window
    int32_t                 iGameUnackLimit;

    //! Connection Concierge mode (CONNAPI_CCMODE_*)
    int32_t                 iCcMode;

    //! session information
    char                    strSession[128];

    //! demangler reporting?
    uint8_t                 bReporting;

    //! is demangler enabled?
    uint8_t                 bDemanglerEnabled;

    //! is tunnel enabled?
    uint8_t                 bTunnelEnabled;

    //! is upnp enabled?
    uint8_t                 bUpnpEnabled;

    //! is voip enabled?
    uint8_t                 bVoipEnabled;

    //! should the client establish a voip connection with a player via the host.
    uint8_t                 bVoipRedirectViaHost;

    //! should the client attemp a voip connection with the host
    uint8_t                 bVoipConnectHost;

    //! in peer-web mode?
    uint8_t                 bPeerWeb;

    //! inside of a callback?
    uint8_t                 bInCallback;

    //! disc callback on client removal?
    uint8_t                 bRemoveCallback;

    //! auto-update enabled?
    uint8_t                 bAutoUpdate;

#if defined(DIRTYCODE_XBOXONE)
    //! 'phcc'
    uint8_t                 bPcHostXboxOneClientMode;

    //! 'exsn', the external session name - passed to the demangler
    char                    strExternalSessionName[128];

    //! 'estn', the external session template name - passed to the demangler
    char                    strExternalSessionTemplateName[128];

    //! 'scid', the service configuration id - passed to the demangler
    char                    strScid[128];
#endif

    //! 'adve', TRUE if ProtoAdvt advertising enabled
    uint8_t                 bDoAdvertising;

    //! 'meta', TRUE if commudp metadata enabled
    uint8_t                 bCommUdpMetadata;

    //! game socket ref, if available
    uintptr_t               uGameSockRef;

    //! voip socket ref, if available
    uintptr_t               uVoipSockRef;

    //! tunnel socket ref, if available
    uintptr_t               uTunlSockRef;

    int32_t                 iHostIndex;

    #if DIRTYCODE_DEBUG
    //! force direct connection to fail ?
    uint8_t                 bFailP2PConnect;
    #endif

    uint32_t                uGameTunnelFlag;
    uint32_t                uGameTunnelFlagOverride;

    int32_t                 iQosDuration;
    int32_t                 iQosInterval;
    int32_t                 iQosPacketSize;

    //! client state
    enum
    {
        ST_IDLE,            //!< idle
        ST_INGAME           //!< hosting or joined a game
    } eState;

    //! gameserver "client" (used if gameserver is enabled and in PEERCONN mode)
    ConnApiClientT          GameServer;

    //! client list - must come last in ref as it is variable length
    ConnApiClientListT      ClientList;
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _ConnApiDefaultCallback

    \Description
        Default ConnApi user callback.  On a debug build, displays state transition
        information.

    \Input *pConnApi    - connection manager ref
    \Input *pCbInfo     - connection info
    \Input *pUserData   - user callback data

    \Version 01/17/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiDefaultCallback(ConnApiRefT *pConnApi, ConnApiCbInfoT *pCbInfo, void *pUserData)
{
    #if DIRTYCODE_LOGGING
    static const char *_StateNames[CONNAPI_NUMSTATUSTYPES] =
    {
        "CONNAPI_STATUS_INIT",
        "CONNAPI_STATUS_CONN",
        "CONNAPI_STATUS_MNGL",
        "CONNAPI_STATUS_ACTV",
        "CONNAPI_STATUS_DISC"
    };
    static const char *_TypeNames[CONNAPI_NUMCBTYPES] =
    {
        "CONNAPI_CBTYPE_GAMEEVENT",
        "CONNAPI_CBTYPE_DESTEVENT",
        "CONNAPI_CBTYPE_VOIPEVENT",
    };

    // display state change
    NetPrintf(("connapi: [0x%08x] client %d) [%s] %s -> %s\n", pConnApi, pCbInfo->iClientIndex, _TypeNames[pCbInfo->eType],
        _StateNames[pCbInfo->eOldStatus], _StateNames[pCbInfo->eNewStatus]));
    #endif
}

/*F********************************************************************************/
/*!
    \Function _ConnApiGenerateDemanglePort

    \Description
        Generate a "random" demangle port to use, actually generated based on the
        two lowest bytes of the MAC address and multiplied by a small prime number,
        and then modded into a small range of ports.  The intent here is to generate
        ports that will both not be reused on successive attempts and also be unlikely
        to collide with other client choices, in case there are other clients operating
        the same code behind the same NAT device.

    \Input *pConnApi    - connection manager ref

    \Output
        uint16_t        - generated demangle port to use

    \Version 03/02/2009 (jbrookes)
*/
/********************************************************************************F*/
#if CONNAPI_RNDLCLDEMANGLEPORT
static uint16_t _ConnApiGenerateDemanglePort(ConnApiRefT *pConnApi)
{
    static const uint16_t _uPortBase = 10000;
    static const uint16_t _uPortRange = 1000;
    static uint16_t _uPortOffset = 0xffff;

    // initialize port offset based on mac address to avoid collisions with other clients behind the same NAT
    if (_uPortOffset == 0xffff)
    {
        uint8_t aMacAddr[6];
        if (NetConnStatus('macx', 0, aMacAddr, sizeof(aMacAddr)) >= 0)
        {
            // generate mac-based port within specified range
            NetPrintf(("connapi: [0x%08x] generating port offset based on MAC addr '%s'\n", pConnApi, NetConnMAC()));
            _uPortOffset = aMacAddr[4];
            _uPortOffset = (_uPortOffset << 8) + aMacAddr[5];
            _uPortOffset *= 7;
        }
        else
        {
            NetPrintf(("connapi: [0x%08x] unable to acquire mac address\n", pConnApi));
            _uPortOffset = 0;
        }
    }

    // generate new port
    _uPortOffset = (_uPortOffset + 1) % _uPortRange;
    return(_uPortBase + _uPortOffset);
}
#endif

/*F********************************************************************************/
/*!
    \Function _ConnApiDisplayClientInfo

    \Description
        Debug-only function to print the given client info to debug output.

    \Input *pClient - pointer to client to display

    \Version 01/06/2005 (jbrookes)
*/
/********************************************************************************F*/
#if DIRTYCODE_LOGGING
static void _ConnApiDisplayClientInfo(ConnApiClientT *pClient, int32_t iClient)
{
    if (pClient->bAllocated)
    {
        NetPrintf(("connapi:   %d) id:0x%08x lid:0x%08x rid:0x%08x ip:%a hosted:%s qos:%s dirtyaddr:%s\n",
            iClient, pClient->ClientInfo.uId, pClient->ClientInfo.uLocalClientId, pClient->ClientInfo.uRemoteClientId,
            pClient->ClientInfo.uAddr, pClient->ClientInfo.bIsConnectivityHosted ? "TRUE" : "FALSE",
            pClient->ClientInfo.bEnableQos ? "TRUE" : "FALSE", pClient->ClientInfo.DirtyAddr.strMachineAddr));
    }
    else
    {
        NetPrintf(("connapi:   %d) empty\n", iClient));
    }
}
#endif

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function _ConnApiDisplayClientList

    \Description
        Debug-only function to print the given client list to debug output.

    \Input *pClientList - pointer to client list to display

    \Version 01/06/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiDisplayClientList(ConnApiClientListT *pClientList)
{
    int32_t iClient;

    NetPrintf(("connapi: clientlist display\n"));
    for (iClient = 0; iClient < pClientList->iMaxClients; iClient++)
    {
        _ConnApiDisplayClientInfo(&pClientList->Clients[iClient], iClient);
    }
}
#endif

/*F********************************************************************************/
/*!
    \Function _ConnApiClientReleaseProtoMangleResources

    \Description
        When both voip and game are in DISC state, let ProtoMangle know that it
        can release resources associated with this client.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - pointer to client to init

    \Notes
        1- Really needed for Xbox One only. Ends up being a no-op on other platform.

        2- For dirtycast-based scenarios, the client entry for the gameserver always has
           pClient->VoipInfo.eStatus = CONNAPI_STATUS_INIT, so the call to ProtoMangleContro()
           is never exercised.

    \Version 09/13/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _ConnApiClientReleaseProtoMangleResources(ConnApiRefT *pConnApi, ConnApiClientT *pClient)
{
    if ((pClient->GameInfo.eStatus == CONNAPI_STATUS_DISC) && (pClient->VoipInfo.eStatus == CONNAPI_STATUS_DISC))
    {
        ProtoMangleControl(pConnApi->pProtoMangle, 'remv', (int32_t)(pClient - pConnApi->ClientList.Clients), 0, NULL);
    }
}


#if defined(DIRTYCODE_XBOXONE)
/*F********************************************************************************/
/*!
    \Function _ConnApiInitClientConnectionState

    \Description
        Initialize client's game and voip connection states based on selected game and voip topology.

    \Input *pConnApi    - connection manager ref
    \Input *pClient     - pointer to client to init
    \Input iClientIndex - index of client
    \Input uConnMode    - bit mask to specify what connection state needs to be initialized (CONNAPI_CONNFLAG_XXXCONN)

    \Version 11/08/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _ConnApiInitClientConnectionState(ConnApiRefT *pConnApi, ConnApiClientT *pClient, int32_t iClientIndex, uint32_t uConnMode)
{
    /*
    $todo - revisit this logics to make it more explicit
    --> p2p game connections that do not need to be demangled are skipped in _ConnApiUpdateConnections() because
        they don't have the CONNAPI_CONNFLAG_GAMECONN flag set
    --> the same trick is not feasible with p2p voip connections because they need to reach the CONNAPI_STATUS_ACTV state
        even if routed through a server; so we make sure they do not get demangled by defaulting straight
        to CONNAPI_STATUS_INIT here.
    */

    if (uConnMode & CONNAPI_CONNFLAG_GAMECONN)
    {
        // if we're a xboxone client in phcc mode and we are currently dealing with the dedicated server client entry or we are connectivity hosted,
        // then make sure we do not attempt to demangle with protomanglexboxone
        if (pClient->ClientInfo.bIsConnectivityHosted || (pConnApi->bPcHostXboxOneClientMode) && (iClientIndex == pConnApi->iHostIndex || iClientIndex == pConnApi->iSelf))
        {
            pClient->GameInfo.eStatus = CONNAPI_STATUS_INIT;
            pClient->GameInfo.bDemangling = TRUE; // make sure demangling will not be attempted if first connection attempt to server fails
        }
        else
        {
            pClient->GameInfo.eStatus = CONNAPI_STATUS_MNGL;

            // if secure address is already known, don’t clear it – it will be reused
            if ((pClient->uFlags & CONNAPI_CLIENTFLAG_SECUREADDRRESOLVED) == 0)
            {
                pClient->ClientInfo.uAddr = 0;
                pClient->ClientInfo.uLocalAddr = 0;
            }
        }
    }

    if (uConnMode & CONNAPI_CONNFLAG_VOIPCONN)
    {
        // if voip is routed through a dedicated server (dirtycast or not) or when connectivity hosted, then make sure we do not attempt to demangle
        // voip connections with protomanglexboxone
        if (pClient->ClientInfo.bIsConnectivityHosted || (((pConnApi->uGameServConnMode & CONNAPI_CONNFLAG_VOIPCONN) || pConnApi->bVoipRedirectViaHost)) ||
            ((pConnApi->bPcHostXboxOneClientMode) && (iClientIndex == pConnApi->iHostIndex || iClientIndex == pConnApi->iSelf)))
        {
            pClient->VoipInfo.eStatus = CONNAPI_STATUS_INIT;
            pClient->VoipInfo.bDemangling = TRUE; // make sure demangling will not be attempted if first connection attempt to server fails
        }
        else
        {
            pClient->VoipInfo.eStatus = CONNAPI_STATUS_MNGL;

            // if secure address is already known, don’t clear it – it will be reused
            if ((pClient->uFlags & CONNAPI_CLIENTFLAG_SECUREADDRRESOLVED) == 0)
            {
                pClient->ClientInfo.uAddr = 0;
                pClient->ClientInfo.uLocalAddr = 0;
            }
        }
    }
}
#endif

/*F********************************************************************************/
/*!
    \Function _ConnApiInitClient

    \Description
        Initialize a single client based on input user info.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - pointer to client to init
    \Input *pClientInfo - pointer to user info to init client with
    \Input iClientIndex - index of client

    \Version 01/06/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiInitClient(ConnApiRefT *pConnApi, ConnApiClientT *pClient, ConnApiClientInfoT *pClientInfo, int32_t iClientIndex)
{
    // initialize new client structure and save input user info
    ds_memclr(pClient, sizeof(*pClient));
    ds_memcpy_s(&pClient->ClientInfo, sizeof(pClient->ClientInfo),  pClientInfo, sizeof(*pClientInfo));

    // initialize default voip values
    pClient->iVoipConnId = VOIP_CONNID_NONE;

    // set up remote (connect) port info
    pClient->GameInfo.uMnglPort = (pClient->ClientInfo.uGamePort == 0) ? pConnApi->uGamePort : pClient->ClientInfo.uGamePort;
    pClient->VoipInfo.uMnglPort = (pClient->ClientInfo.uVoipPort == 0) ? pConnApi->uVoipPort : pClient->ClientInfo.uVoipPort;

    // set up local (bind) port info
    pClient->GameInfo.uLocalPort = ((pClient->ClientInfo.uLocalGamePort == 0) || (pConnApi->bTunnelEnabled == TRUE)) ? pConnApi->uGamePort : pClient->ClientInfo.uLocalGamePort;
    pClient->VoipInfo.uLocalPort = ((pClient->ClientInfo.uLocalVoipPort == 0) || (pConnApi->bTunnelEnabled == TRUE)) ? pConnApi->uVoipPort : pClient->ClientInfo.uLocalVoipPort;

    // set unique client identifier if not already supplied
    if (pClient->ClientInfo.uId == 0)
    {
        pClient->ClientInfo.uId = (uint32_t)iClientIndex + 1;
    }

    if (pClient->ClientInfo.bEnableQos == TRUE && iClientIndex != pConnApi->iSelf)
    {
        NetPrintf(("connapi: [0x%08x] Delaying voip connection for player %d because of QoS validation\n", pConnApi, iClientIndex));
    }
    else
    {
        // Voip is not delayed with bEnableQos is FALSE
        pClient->bEstablishVoip = TRUE;
    }

#if defined(DIRTYCODE_XBOXONE)
    _ConnApiInitClientConnectionState(pConnApi, pClient, iClientIndex, CONNAPI_CONNFLAG_GAMECONN|CONNAPI_CONNFLAG_VOIPCONN);
#endif

    // mark as allocated
    pClient->bAllocated = TRUE;
}

/*F********************************************************************************/
/*!
    \Function _ConnApiUpdateClientFlags

    \Description
        Update client flags based on game mode and game flags.

    \Input *pConnApi    - pointer to module state

    \Version 05/16/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiUpdateClientFlags(ConnApiRefT *pConnApi)
{
    ConnApiClientT *pClient;
    int32_t iClient;

    for (iClient = 0; iClient < pConnApi->ClientList.iMaxClients; iClient++)
    {
        pClient = &pConnApi->ClientList.Clients[iClient];
        if (pClient->bAllocated)
        {
            pClient->uConnFlags = pConnApi->uConnFlags;

            if ((iClient != pConnApi->iHostIndex) && (pConnApi->iHostIndex != pConnApi->iSelf) && (!pConnApi->bPeerWeb))
            {
                pClient->uConnFlags &= ~CONNAPI_CONNFLAG_GAMECONN;
            }

            if (pClient->bEstablishVoip == FALSE)
            {
                pClient->uConnFlags &= ~CONNAPI_CONNFLAG_VOIPCONN;
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _ConnApiGameServerEnabled

    \Description
        Determine if the Game Server is to be used to connect the GameClient or
        VoipClient.

    \Input *pConnApi            - pointer to module state
    \Input *pClient             - client connection is to, or NULL for global state
    \Input uGameServConnMode    - game conn or voip conn

    \Output
        uint32_t                - TRUE if GameServer is enabled, else FALSE

    \Version 02/21/2007 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _ConnApiGameServerEnabled(ConnApiRefT *pConnApi, ConnApiClientT *pClient, uint32_t uGameServConnMode)
{
    uint32_t bUseGameServer;

    // use gameserver if address is not zero and connmode says it is enabled
    bUseGameServer = (pConnApi->uGameServAddr != 0) && (pConnApi->uGameServConnMode & uGameServConnMode);
    
    return(bUseGameServer);
}

/*F********************************************************************************/
/*!
    \Function _ConnApiGenerateSessionKey

    \Description
        Generate session key for demangling session.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - pointer to peer
    \Input iClientIndex - index of peer
    \Input *pSess       - [out] pointer to session buffer
    \Input iSessSize    - size of session buffer
    \Input *pSessType   - type of session - "game" or "voip"

    \Version 01/13/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiGenerateSessionKey(ConnApiRefT *pConnApi, ConnApiClientT *pClient, int32_t iClientIndex, char *pSess, int32_t iSessSize, const char *pSessType)
{
    uint32_t uIdA, uIdB, uTemp;

    uIdA = pConnApi->ClientList.Clients[pConnApi->iSelf].ClientInfo.uId;
    uIdB = pClient->ClientInfo.uId;
    if (uIdB < uIdA)
    {
        uTemp = uIdA;
        uIdA = uIdB;
        uIdB = uTemp;
    }

    ds_snzprintf(pSess, iSessSize, "%08x-%08x-%s-%08x", uIdA, uIdB, pSessType, pConnApi->iSessId);
}

/*F********************************************************************************/
/*!
    \Function _ConnApiTunnelAlloc

    \Description
        Allocate a tunnel for the given client.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - client to connect to
    \Input iClientIndex - index of client
    \Input uRemoteAddr  - remote address to tunnel to
    \Input bLocalAddr   - TRUE if we are using local address, else FALSE

    \Version 12/20/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiTunnelAlloc(ConnApiRefT *pConnApi, ConnApiClientT *pClient, int32_t iClientIndex, uint32_t uRemoteAddr, uint8_t bLocalAddr)
{
    ProtoTunnelInfoT TunnelInfo;

    // if connectivity is hosted and if the tunnel to the hosting server was already created in the connection flow of another client
    // then skip tunnel creation and reuse that tunnel
    if (pClient->ClientInfo.bIsConnectivityHosted)
    {
        int32_t iClient;

        for (iClient = 0; iClient < pConnApi->ClientList.iMaxClients; iClient++)
        {
            if ( (pClient != &pConnApi->ClientList.Clients[iClient]) &&
                 (pClient->ClientInfo.uHostingServerId == pConnApi->ClientList.Clients[iClient].ClientInfo.uHostingServerId) &&
                 (pConnApi->ClientList.Clients[iClient].iTunnelId != 0) )
            {
                pClient->iTunnelId = pConnApi->ClientList.Clients[iClient].iTunnelId;
                NetPrintf(("connapi: [0x%08x] client %d is reusing tunnel 0x%08x to hosting server 0x%08x\n", pConnApi, iClientIndex, pClient->iTunnelId, pClient->ClientInfo.uHostingServerId));
                return;
            }
        }
    }

    // set up tunnel info
    ds_memclr(&TunnelInfo, sizeof(TunnelInfo));

    TunnelInfo.uRemoteClientId =  pClient->ClientInfo.bIsConnectivityHosted ? pClient->ClientInfo.uHostingServerId : pClient->ClientInfo.uRemoteClientId;
    TunnelInfo.uRemoteAddr = uRemoteAddr;
    TunnelInfo.uRemotePort = bLocalAddr ? pClient->ClientInfo.uLocalTunnelPort : pClient->ClientInfo.uTunnelPort;
    TunnelInfo.aRemotePortList[0] = pClient->GameInfo.uMnglPort;
    TunnelInfo.aRemotePortList[1] = pClient->VoipInfo.uMnglPort;
    TunnelInfo.aPortFlags[0] = (pConnApi->uGameTunnelFlagOverride) ? (pConnApi->uGameTunnelFlag) : (PROTOTUNNEL_PORTFLAG_ENCRYPTED|PROTOTUNNEL_PORTFLAG_AUTOFLUSH);
    TunnelInfo.aPortFlags[1] = PROTOTUNNEL_PORTFLAG_ENCRYPTED;

    NetPrintf(("connapi: [0x%08x] setting client %d clientId=0x%08x TunnelInfo.uRemotePort %d %s\n", pConnApi, iClientIndex, TunnelInfo.uRemoteClientId, TunnelInfo.uRemotePort,
               pClient->ClientInfo.bIsConnectivityHosted ? "(hosted connection)" : "(direct connection)"));

    // allocate tunnel, set the local client id and return to caller
    pClient->iTunnelId = ProtoTunnelAlloc(pConnApi->pProtoTunnel, &TunnelInfo, pClient->ClientInfo.strTunnelKey);
    if (pClient->iTunnelId >= 0)
    {
        ProtoTunnelControl(pConnApi->pProtoTunnel, 'tcid', pClient->iTunnelId, (int32_t)pClient->ClientInfo.uLocalClientId, NULL);
    }
}

/*F********************************************************************************/
/*!
    \Function _ConnApiVoipTunnelAlloc

    \Description
        Allocate a tunnel for voip for the given client depending on voip settings.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - client to connect to
    \Input iClientIndex - index of client
    \Input uRemoteAddr  - remote address to tunnel to
    \Input bLocalAddr   - TRUE if we are using local address, else FALSE

    \Version 12/23/2008 (jrainy)
*/
/********************************************************************************F*/
static void _ConnApiVoipTunnelAlloc(ConnApiRefT *pConnApi, ConnApiClientT *pClient, int32_t iClientIndex, uint32_t uRemoteAddr, uint8_t bLocalAddr)
{
    int32_t iTunnelId = 0;

    // if doing redirect via host, check for previously created tunnel for re-use
    if (pConnApi->bVoipRedirectViaHost)
    {
        iTunnelId = pConnApi->ClientList.Clients[pConnApi->iHostIndex].iTunnelId;
    }

    // if doing gameserver voip, check for previously created tunnel for re-use
    if (_ConnApiGameServerEnabled(pConnApi, NULL, CONNAPI_CONNFLAG_VOIPCONN))
    {
        iTunnelId = pConnApi->GameServer.iTunnelId;
    }

    // if no reused tunnel, create one
    if (iTunnelId == 0)
    {
        _ConnApiTunnelAlloc(pConnApi, pClient, iClientIndex, uRemoteAddr, bLocalAddr);
    }
    else
    {
        pClient->iTunnelId = iTunnelId;
    }
}

/*F********************************************************************************/
/*!
    \Function _ConnApiTunnelFree

    \Description
        Free the given client's tunnel.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - pointer to client to allocate tunnel for

    \Version 12/20/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiTunnelFree(ConnApiRefT *pConnApi, ConnApiClientT *pClient)
{
    // tunnel active?
    if (!pConnApi->bTunnelEnabled)
    {
        return;
    }

    //  if voip to this client is redirected via the host in a C/S game,
    //  and we're trying to free a client, but not the host itself, skip tunnel destruction.
    if (pConnApi->bVoipRedirectViaHost)
    {
        if ((pConnApi->ClientList.Clients[pConnApi->iHostIndex].iTunnelId == pClient->iTunnelId) &&
            (&pConnApi->ClientList.Clients[pConnApi->iHostIndex] != pClient))
        {
            pClient->iTunnelId = 0;
            return;
        }
    }

    // if connectivity is hosted and if the tunnel to the hosting server is still used for another client
    // then skip tunnel destruction
    if (pClient->ClientInfo.bIsConnectivityHosted)
    {
        int32_t iClient;

        for (iClient = 0; iClient < pConnApi->ClientList.iMaxClients; iClient++)
        {
            if ( (pClient != &pConnApi->ClientList.Clients[iClient]) &&
                 (pClient->ClientInfo.uHostingServerId == pConnApi->ClientList.Clients[iClient].ClientInfo.uHostingServerId) &&
                 (pClient->iTunnelId == pConnApi->ClientList.Clients[iClient].iTunnelId) )
            {
                #if DIRTYCODE_LOGGING
                ConnApiClientT *pFirstClient = &pConnApi->ClientList.Clients[0];
                NetPrintf(("connapi: [0x%08x] freeing tunnel 0x%08x to hosting server 0x%08x is skipped for client %d because tunnel still used by at least client %d\n",
                    pConnApi, pClient->iTunnelId, pClient->ClientInfo.uHostingServerId, (pClient - pFirstClient), iClient));
                #endif
                pClient->iTunnelId = 0;
                return;
            }
        }
    }

    if (pClient->iTunnelId != 0)
    {
        ProtoTunnelFree2(pConnApi->pProtoTunnel, pClient->iTunnelId, pClient->ClientInfo.strTunnelKey, pClient->ClientInfo.uAddr);
        pClient->iTunnelId = 0;
    }
}

/*F********************************************************************************/
/*!
    \Function _ConnApiGetConnectAddr

    \Description
        Selects between internal and internal address to use for connection,
        based on whether the external addresses are equal or not.

    \Input *pConnApi        - pointer to module state
    \Input *pClient         - pointer to client to connect to
    \Input *pLocalAddr      - [out] storage for boolean indicating whether local address was used or not
    \Input uConnMode        - game conn or voip conn
    \Input **pClientUsedRet - the pointer to use to return the client actually used

    \Output
        int32_t             - ip address to be used to connnect to the specified client

    \Version 01/11/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ConnApiGetConnectAddr(ConnApiRefT *pConnApi, ConnApiClientT *pClient, uint8_t *pLocalAddr, const uint32_t uConnMode, ConnApiClientT **pClientUsedRet)
{
    ConnApiClientT *pSelf;
    int32_t uAddr;
    uint8_t bLocalAddr = FALSE;
    ConnApiClientT *pClientUsed = NULL;

    #if DIRTYCODE_LOGGING
    const char *pConn = (uConnMode == CONNAPI_CONNFLAG_GAMECONN) ? "game" : "voip";
    #endif

    // are we using the gameserver?
    if (!_ConnApiGameServerEnabled(pConnApi, pClient, uConnMode))
    {
        // ref local client info
        pSelf = &pConnApi->ClientList.Clients[pConnApi->iSelf];

        if ((uConnMode == CONNAPI_CONNFLAG_VOIPCONN) && pConnApi->bVoipRedirectViaHost)
        {
            uAddr = pConnApi->ClientList.Clients[pConnApi->iHostIndex].ClientInfo.uAddr;
            pClientUsed = &pConnApi->ClientList.Clients[pConnApi->iHostIndex];
            NetPrintf(("connapi: [%s] using host address to connect to (or disconnect from) client 0x%08x\n", pConn, pClient->ClientInfo.uId));
        }
        else
        {
            // if external addresses match, use local address
            if ((pSelf->ClientInfo.uAddr & pConnApi->uNetMask) == (pClient->ClientInfo.uAddr & pConnApi->uNetMask))
            {
                NetPrintf(("connapi: [%s] using local address to connect to (or disconnect from) client 0x%08x\n", pConn, pClient->ClientInfo.uId));
                uAddr = pClient->ClientInfo.uLocalAddr;
                pClientUsed = pClient;
                bLocalAddr = TRUE;
            }
            else
            {
                NetPrintf(("connapi: [%s] using peer address to connect to (or disconnect from) client 0x%08x\n", pConn, pClient->ClientInfo.uId));
                uAddr = pClient->ClientInfo.uAddr;

                pClientUsed = pClient;
            }

            #if DIRTYCODE_DEBUG
            if (pClient->ClientInfo.bIsConnectivityHosted == FALSE)
            {
                if (pConnApi->bFailP2PConnect)
                {
                    // global P2P fail flag is set
                    NetPrintf(("connapi: [%s] !! P2P CONNECTION FAILURE TRICK !! - global P2P fail - peer address for client 0x%08x replaced with unreachable value\n", pConn, pClient->ClientInfo.uId));
                    uAddr = 0;
                }
                else if (pClient->uFlags & CONNAPI_CLIENTFLAG_P2PFAILDBG)
                {
                    NetPrintf(("connapi: [%s] !! P2P CONNECTION FAILURE TRICK !! - remote P2P fail flag - peer address for client 0x%08x replaced with unreachable value\n", pConn, pClient->ClientInfo.uId));
                    uAddr = 0;
                }
                else if (pConnApi->ClientList.Clients[pConnApi->iSelf].uFlags & CONNAPI_CLIENTFLAG_P2PFAILDBG)
                {
                    NetPrintf(("connapi: [%s] !! P2P CONNECTION FAILURE TRICK !! - self P2P fail flag - peer address for client 0x%08x replaced with unreachable value\n", pConn, pClient->ClientInfo.uId));
                    uAddr = 0;
                }
            }
            #endif
        }
    }
    else
    {
        NetPrintf(("connapi: [0x%08x] [%s] using game server address to connect to (or disconnect from) client 0x%08x\n", pConnApi, pConn, pClient->ClientInfo.uId));
        uAddr = pConnApi->uGameServAddr;
        pClientUsed = &pConnApi->GameServer;
    }

    *pLocalAddr = bLocalAddr;

    if (pClientUsedRet)
    {
        *pClientUsedRet = pClientUsed;
    }

    return(uAddr);
}

/*F********************************************************************************/
/*!
    \Function _ConnApiVoipGroupConnSharingCallback

    \Description
        Use to invoke VoipGroupResume() and VoipGroupSuspend when notified
        by the VoipGroup module.

    \Input *pVoipGroup  - voip group ref
    \Input eCbType      - event identifier
    \Input iConnId      - connection ID
    \Input *pUserData   - user callback data
    \Input bSending     - client sending flag
    \Input bReceiving   - client receiving flag

    \Version 11/11/2009 (mclouatre)
*/
/********************************************************************************F*/
static void _ConnApiVoipGroupConnSharingCallback(VoipGroupRefT *pVoipGroup, ConnSharingCbTypeE eCbType, int32_t iConnId, void *pUserData, uint8_t bSending, uint8_t bReceiving)
{
    ConnApiRefT *pConnApi = (ConnApiRefT *)pUserData;
    ConnApiClientT *pClient = &pConnApi->ClientList.Clients[iConnId];
    ConnApiClientT *pClientUsed;
    int32_t iConnectAddr;
    uint8_t bLocalAddr;

    iConnectAddr = _ConnApiGetConnectAddr(pConnApi, pClient, &bLocalAddr, CONNAPI_CONNFLAG_VOIPCONN, &pClientUsed);

    if (eCbType == VOIPGROUP_CBTYPE_CONNSUSPEND)
    {
        NetPrintf(("connapi: [0x%08x] suspending voip connection to client 0x%08x:%a at %d\n", pConnApi, pClient->ClientInfo.uId, iConnectAddr, NetTick()));
        VoipGroupSuspend(pVoipGroup, iConnId);
    }
    else if (eCbType == VOIPGROUP_CBTYPE_CONNRESUME)
    {
         // do we have a tunnel to this client?
        if (pClientUsed->iTunnelId > 0)
        {
            NetPrintf(("connapi: [0x%08x] we have a tunnel for client %d; using virtual address %a\n", pConnApi,
                iConnId, pClientUsed->iTunnelId));
            iConnectAddr = pClientUsed->iTunnelId;
        }

        NetPrintf(("connapi: [0x%08x] resuming voip connection to client 0x%08x:%a at %d\n", pConnApi, pClient->ClientInfo.uId, iConnectAddr, NetTick()));
        VoipGroupResume2(pVoipGroup, iConnId, iConnectAddr, pClient->VoipInfo.uMnglPort, pClient->VoipInfo.uLocalPort, pClient->ClientInfo.uId, pConnApi->iSessId, pClient->ClientInfo.bIsConnectivityHosted);
        #if DIRTYCODE_LOGGING
        if (bSending != bReceiving)
        {
            NetPrintf(("connapi: [0x%08x] warning - send and receive mute flags are different for client 0x%08x:%a\n", pConnApi, pClient->ClientInfo.uId, iConnectAddr));
        }
        #endif
        VoipGroupMuteByConnId(pVoipGroup, iConnId, bSending);
    }
    else
    {
        NetPrintf(("connapi: [0x%08x] critical error - unknown connection sharing event type\n", pConnApi));
    }
}

/*F********************************************************************************/
/*!
    \Function _ConnApiGetConnectParms

    \Description
        Gets connection parameters.

    \Input *pConnApi    - pointer to module state
    \Input *pClientA    - pointer to client to connect to
    \Input *pConnName   - [out] storage for connection name
    \Input iNameSize    - size of output buffer

    \Output
        int32_t             - connection flags (NEGAME_CONN_*)

    \Version 04/21/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ConnApiGetConnectParms(ConnApiRefT *pConnApi, ConnApiClientT *pClientA, char *pConnName, int32_t iNameSize)
{
    ConnApiClientT *pClientB = &pConnApi->ClientList.Clients[pConnApi->iSelf];
    uint32_t uAddrA, uAddrB, uAddrT;
    uint32_t bHosting = TRUE;
    int32_t iConnFlags;

    // reference unique address strings
    uAddrA = pClientA->ClientInfo.uId;
    uAddrB = pClientB->ClientInfo.uId;

    // if we're in gameserver/peerconn mode, connname is our username and we are always connecting
    if (_ConnApiGameServerEnabled(pConnApi, pClientA, CONNAPI_CONNFLAG_GAMECONN))
    {
        ds_snzprintf(pConnName, iNameSize, "%08x", pConnApi->uSelfId);
        return(NETGAME_CONN_CONNECT);
    }

    /* determine if we are "hosting" or not, in NetGame terms (one peer
       must listen and one peer must connect for each connection) */
    if (pConnApi->bPeerWeb == FALSE)
    {
        // if we're client/server, server listens and clients connect
        bHosting = (pConnApi->iHostIndex == pConnApi->iSelf);
    }
    else
    {
        // if we're peer-web, compare addresses to choose listener and connector
        bHosting = uAddrA < uAddrB;
    }

    /* set up parms based on whether we are "hosting" or not.  the connection name is the
       unique address of the "host" concatenated with the unique address of the "client" */
    if (bHosting == TRUE)
    {
        // swap names
        uAddrT = uAddrB;
        uAddrB = uAddrA;
        uAddrA = uAddrT;

        // set conn flags
        iConnFlags = NETGAME_CONN_LISTEN;
    }
    else
    {
        iConnFlags = NETGAME_CONN_CONNECT;
    }

    // format connection name and return connection flags
    ds_snzprintf(pConnName, iNameSize, "%x-%x", uAddrA, uAddrB);
    return(iConnFlags);
}

/*F********************************************************************************/
/*!
    \Function _ConnApiUpdateCallback

    \Description
        Trigger user callback if the state has changed.

    \Input *pConnApi    - pointer to module state
    \Input iClientIndex - index of client
    \Input eType        - type of connection (CONNAPI_CBTYPE_*)
    \Input eOldStatus   - previous status
    \Input eNewStatus   - current status

    \Output
        int32_t         - one=active, zero=disconnected

    \Version 01/06/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiUpdateCallback(ConnApiRefT *pConnApi, int32_t iClientIndex, ConnApiCbTypeE eType, ConnApiConnStatusE eOldStatus, ConnApiConnStatusE eNewStatus)
{
    ConnApiCbInfoT CbInfo;
    ConnApiConnInfoT *pConnInfo = NULL;
    int32_t iIndex;

    // if no change, no callback
    if (eOldStatus == eNewStatus)
    {
        return;
    }

    // otherwise, fire off a callback
    CbInfo.iClientIndex = iClientIndex;
    CbInfo.eType = eType;
    CbInfo.eOldStatus = eOldStatus;
    CbInfo.eNewStatus = eNewStatus;

    if (iClientIndex == -1)
    {
        CbInfo.pClient = &pConnApi->GameServer;
    }
    else
    {
        CbInfo.pClient = &pConnApi->ClientList.Clients[iClientIndex];
    }

    // update connection timers
    if (eType == CONNAPI_CBTYPE_GAMEEVENT)
    {
        pConnInfo = (ConnApiConnInfoT *)&CbInfo.pClient->GameInfo;
    }
    else if (eType == CONNAPI_CBTYPE_VOIPEVENT)
    {
        pConnInfo = (ConnApiConnInfoT *)&CbInfo.pClient->VoipInfo;
    }

    if (pConnInfo != NULL)
    {
        // finished demangling
        if (eOldStatus == CONNAPI_STATUS_MNGL)
        {
            if (pConnInfo->iConnStart != 0)
            {
                #ifdef DIRTYCODE_XBOXONE
                    pConnInfo->ConnTimers.uCreateSATime = NetTickDiff(NetTick(), pConnInfo->iConnStart);
                #else
                    pConnInfo->ConnTimers.uDemangleTime = NetTickDiff(NetTick(), pConnInfo->iConnStart);
                #endif
            }
            else
            {
                #ifdef DIRTYCODE_XBOXONE
                    pConnInfo->ConnTimers.uCreateSATime = 0;
                #else
                    pConnInfo->ConnTimers.uDemangleTime = 0;
                #endif
            }
        }
        // it went from some state to active or disconnect log connect time
        else if ((eOldStatus == CONNAPI_STATUS_CONN) && ((eNewStatus == CONNAPI_STATUS_ACTV) || (eNewStatus == CONNAPI_STATUS_DISC)))
        {
            if (pConnInfo->uConnFlags & CONNAPI_CONNFLAG_DEMANGLED)
            {
                if (pConnInfo->iConnStart != 0)
                {
                    pConnInfo->ConnTimers.uDemangleConnectTime = NetTickDiff(NetTick(), pConnInfo->iConnStart);
                }
                else
                {
                    pConnInfo->ConnTimers.uDemangleConnectTime = 0;
                }
            }
            else
            {
                if (pConnInfo->iConnStart != 0)
                {
                    pConnInfo->ConnTimers.uConnectTime = NetTickDiff(NetTick(), pConnInfo->iConnStart);
                }
                else
                {
                    pConnInfo->ConnTimers.uConnectTime = 0;
                }
            }
        }
    }

    // call the callback
    pConnApi->bInCallback = TRUE;
    _ConnApiDefaultCallback(pConnApi, &CbInfo, NULL);
    for(iIndex = 0; iIndex < CONNAPI_MAX_CALLBACKS; iIndex++)
    {
        if (pConnApi->pCallback[iIndex] != NULL)
        {
            pConnApi->pCallback[iIndex](pConnApi, &CbInfo, pConnApi->pUserData[iIndex]);
        }
    }

    pConnApi->bInCallback = FALSE;
}


/*F********************************************************************************/
/*!
    \Function _ConnApiDestroyGameConnection

    \Description
        Destroy game link to given client.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - pointer to client to close game connection for
    \Input iClientIndex - index of client in client array
    \Input *pReason     - reason connection is being closed (for debug use)

    \Version 01/12/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiDestroyGameConnection(ConnApiRefT *pConnApi, ConnApiClientT *pClient, int32_t iClientIndex, const char *pReason)
{
    // if refs are about to be destroyed, notify application
    if ((pClient->pGameDistRef != NULL) || (pClient->pGameLinkRef != NULL))
    {
        _ConnApiUpdateCallback(pConnApi, iClientIndex, CONNAPI_CBTYPE_DESTEVENT, CONNAPI_STATUS_ACTV, CONNAPI_STATUS_DISC);
    }

    // destroy the refs
    NetPrintf(("connapi: [0x%08x] destroying game connection to client 0x%08x: %s at %d\n", pConnApi, pClient->ClientInfo.uId, pReason, NetTick()));
    if (pClient->pGameLinkRef != NULL)
    {
        NetGameLinkDestroy(pClient->pGameLinkRef);
        pClient->pGameLinkRef = NULL;
    }
    if (pClient->pGameUtilRef != NULL)
    {
        NetGameUtilDestroy(pClient->pGameUtilRef);
        pClient->pGameUtilRef = NULL;
    }

    pClient->GameInfo.eStatus = CONNAPI_STATUS_DISC;
    _ConnApiClientReleaseProtoMangleResources(pConnApi, pClient);
}

/*F********************************************************************************/
/*!
    \Function _ConnApiDestroyVoipConnection

    \Description
        Destroy voip link to given client.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - pointer to client to close game connection for
    \Input *pReason     - reason connection is being closed (for debug use)

    \Version 01/12/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiDestroyVoipConnection(ConnApiRefT *pConnApi, ConnApiClientT *pClient, const char *pReason)
{
    if (pClient->iVoipConnId >= 0)
    {
        NetPrintf(("connapi: destroying voip connection to client 0x%08x: %s at %d\n", pClient->ClientInfo.uId, pReason, NetTick()));
        VoipGroupDisconnect(pConnApi->pVoipGroupRef, pClient->iVoipConnId);
        pClient->iVoipConnId = VOIP_CONNID_NONE;
    }
    pClient->VoipInfo.eStatus = CONNAPI_STATUS_DISC;

    _ConnApiClientReleaseProtoMangleResources(pConnApi, pClient);
}

/*F********************************************************************************/
/*!
    \Function _ConnApiDisconnectClient

    \Description
        Disconnect a client.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - pointer to client to disconnect
    \Input iClientIndex - client index
    \Input *pReason     - reason for the close

    \Version 01/12/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiDisconnectClient(ConnApiRefT *pConnApi, ConnApiClientT *pClient, int32_t iClientIndex, const char *pReason)
{
    _ConnApiDestroyGameConnection(pConnApi, pClient, iClientIndex, pReason);
    _ConnApiDestroyVoipConnection(pConnApi, pClient, pReason);
    _ConnApiTunnelFree(pConnApi, pClient);
}

/*F********************************************************************************/
/*!
    \Function _ConnApiInitClientList

    \Description
        Initialize client list based on input client list.

    \Input *pConnApi    - pointer to module state
    \Input *pClientList - list of client addresses
    \Input iNumClients  - number of clients in list

    \Version 01/06/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiInitClientList(ConnApiRefT *pConnApi, ConnApiClientInfoT *pClientList, int32_t iNumClients)
{
    ConnApiClientT *pClient;
    int32_t iClient;

    // make sure client count is below max
    if (iNumClients > pConnApi->ClientList.iMaxClients)
    {
        NetPrintf(("connapi: [0x%08x] cannot host %d clients; clamping to %d\n", pConnApi, iNumClients, pConnApi->ClientList.iMaxClients));
        iNumClients = pConnApi->ClientList.iMaxClients;
    }

    // find our index
    pConnApi->iSelf = -1;   // init so we can check after setup to make sure we're in the list
    for (iClient = 0, pConnApi->ClientList.iNumClients = 0; iClient < iNumClients; iClient++)
    {
        // remember our index in list
        if (pClientList[iClient].uId == pConnApi->uSelfId)
        {
            pConnApi->iSelf = iClient;
        }
    }

    // copy input client list
    for (iClient = 0, pConnApi->ClientList.iNumClients = 0; iClient < iNumClients; iClient++)
    {
        // ref client structure
        pClient = &pConnApi->ClientList.Clients[iClient];

        // need to check to see if the client passed it is valid.
        if (pClientList[iClient].uId != 0)
        {
            // init client structure and copy user info
            _ConnApiInitClient(pConnApi, pClient, &pClientList[iClient], iClient);

            // if us, update dirtyaddr
            if (iClient == pConnApi->iSelf)
            {
                // update dirtyaddr and save ref
                ds_memcpy_s(&pConnApi->SelfAddr, sizeof(pConnApi->SelfAddr), &pClient->ClientInfo.DirtyAddr, sizeof(pClient->ClientInfo.DirtyAddr));
            }

            // increment client count
            pConnApi->ClientList.iNumClients += 1;
        }
    }

    // make sure iSelf is valid before we continue
    if (pConnApi->iSelf >= 0)
    {
        // ref local client
        pClient = &pConnApi->ClientList.Clients[pConnApi->iSelf];

        if (pConnApi->GameServer.ClientInfo.uAddr == 0)
        {
            // if gameserver, set up gameserver user in case we need it
            pConnApi->GameServer.ClientInfo.uAddr = pConnApi->uGameServAddr;

            // set up remote (connect) port info
            pConnApi->GameServer.GameInfo.uMnglPort = (pClient->ClientInfo.uGamePort == 0) ? pConnApi->uGamePort : pClient->ClientInfo.uGamePort;
            pConnApi->GameServer.VoipInfo.uMnglPort = (pClient->ClientInfo.uVoipPort == 0) ? pConnApi->uVoipPort : pClient->ClientInfo.uVoipPort;

            // set up local (bind) port info
            pConnApi->GameServer.GameInfo.uLocalPort = ((pClient->ClientInfo.uLocalGamePort == 0) || (pConnApi->bTunnelEnabled == TRUE)) ? pConnApi->uGamePort : pClient->ClientInfo.uLocalGamePort;
            pConnApi->GameServer.VoipInfo.uLocalPort = ((pClient->ClientInfo.uLocalVoipPort == 0) || (pConnApi->bTunnelEnabled == TRUE)) ? pConnApi->uVoipPort : pClient->ClientInfo.uLocalVoipPort;

            pConnApi->GameServer.ClientInfo.uTunnelPort = pConnApi->iTunnelPort;
            pConnApi->GameServer.ClientInfo.uLocalTunnelPort = pConnApi->iTunnelPort;
            
            NetPrintf(("connapi: [0x%08x] setting GameServer ProtoTunnel port %d\n", pConnApi, pConnApi->GameServer.ClientInfo.uTunnelPort));

            #if defined(DIRTYCODE_XBOXONE)
            //  make sure we do not attempt to demangle that entry with protomanglexboxone
            pConnApi->GameServer.GameInfo.eStatus = CONNAPI_STATUS_INIT;
            pConnApi->GameServer.VoipInfo.eStatus = CONNAPI_STATUS_INIT;
            pConnApi->GameServer.GameInfo.bDemangling = TRUE; // make sure demangling will not be attempted if first connection attempt to server fails
            pConnApi->GameServer.VoipInfo.bDemangling = TRUE; // make sure demangling will not be attempted if first connection attempt to server fails
            #endif

            // make it clear that we have no direct voip connection with the game server
            pConnApi->GameServer.iVoipConnId = VOIP_CONNID_NONE;

            // mark as allocated
            pConnApi->GameServer.bAllocated = TRUE;
        }

        // set local user
        if (pConnApi->pVoipRef != NULL)
        {
            if (pConnApi->bVoipOwner == TRUE)
            {
                int32_t iUserIndex = 0;        //$$todo do foreach local user?
                VoipGroupControl(pConnApi->pVoipGroupRef, 'lusr', iUserIndex, TRUE, NULL);
            }
            VoipGroupControl(pConnApi->pVoipGroupRef, 'clid', pClient->ClientInfo.uId, 0, NULL);
        }

        // set prototunnel user
        if (pConnApi->pProtoTunnel != NULL)
        {
            ProtoTunnelControl(pConnApi->pProtoTunnel, 'clid', pClient->ClientInfo.uId, 0, NULL);
        }
    }

    // set initial client flags
    _ConnApiUpdateClientFlags(pConnApi);

    #if DIRTYCODE_LOGGING
    // make sure we're in the list
    if (pConnApi->iSelf == -1)
    {
        NetPrintf(("connapi: local client 0x%08x not found in client list\n", pConnApi->uSelfId));
    }
    // display client list
    _ConnApiDisplayClientList(&pConnApi->ClientList);
    #endif
}

/*F********************************************************************************/
/*!
    \Function _ConnApiParseAdvertField

    \Description
        Parse a field from an advertisement.

    \Input *pOutBuf     - output buffer for field
    \Input iOutSize     - size of output buffer
    \Input *pInpBuf     - pointer to start of field in advertisement buffer
    \Input cTerminator  - field termination character

    \Output
        char *          - pointer to next field

    \Version 01/10/2005 (jbrookes)
*/
/********************************************************************************F*/
static char *_ConnApiParseAdvertField(char *pOutBuf, int32_t iOutSize, char *pInpBuf, char cTerminator)
{
    char *pEndPtr;

    pEndPtr = strchr(pInpBuf, cTerminator);
    *pEndPtr = '\0';

    ds_strnzcpy(pOutBuf, pInpBuf, iOutSize);
    return(pEndPtr+1);
}

/*F********************************************************************************/
/*!
    \Function _ConnApiCheckAdvert

    \Description
        Scan current adverts for any adverts that are broadcast by clients we are
        connecting to.

    \Input *pConnApi    - pointer to module state

    \Version 01/10/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiCheckAdvert(ConnApiRefT *pConnApi)
{
    char strAdvtList[512], *pAdvt, *pTemp;
    char strName[32], strNote[32], strAddr[32];
    ConnApiClientT *pClient;
    int32_t iAdvt, iNumAdvt, iClient;
    uint32_t uAdvtId;
    uint32_t uLocalAddr;

    // see if there are any advertisements
    iNumAdvt = NetGameUtilQuery(pConnApi->pGameUtilRef, pConnApi->strGameName, strAdvtList, sizeof(strAdvtList));
    NetPrintf(("connapi: [0x%08x] found %d advertisements\n", pConnApi, iNumAdvt));

    // parse any advertisements
    for (pAdvt = strAdvtList, iAdvt = 0; iAdvt < iNumAdvt; iAdvt++)
    {
        // extract info from advertisement
        pAdvt = _ConnApiParseAdvertField(strName, sizeof(strName), pAdvt, '\t');
        pAdvt = _ConnApiParseAdvertField(strNote, sizeof(strNote), pAdvt, '\t');
        pAdvt = _ConnApiParseAdvertField(strAddr, sizeof(strAddr), pAdvt+4, '\n');

        sscanf(strName, "%u", &uAdvtId);

        // extract address from addr field
        pTemp = strchr(strAddr, ':');
        *pTemp = '\0';
        uLocalAddr = SocketInTextGetAddr(strAddr);

        // does the name match one of our client's names?
        for (iClient = 0; iClient < pConnApi->ClientList.iMaxClients; iClient++)
        {
            pClient = &pConnApi->ClientList.Clients[iClient];

            if ((uAdvtId == pClient->ClientInfo.uId) && (pClient->ClientInfo.uLocalAddr != uLocalAddr))
            {
                NetPrintf(("connapi: updating local address of machine Id %u from %a to %a\n", uAdvtId, pClient->ClientInfo.uAddr, uLocalAddr));
                pClient->ClientInfo.uLocalAddr = uLocalAddr;
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _ConnApiSetGamelinkOpt

    \Description
        Set a gamelink option, if it isn't the default.

    \Input *pUtilRef    - pointer to util ref for game link
    \Input iOpt         - gamelink option to set
    \Input iValue       - value to set

    \Version 06/03/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiSetGamelinkOpt(NetGameUtilRefT *pUtilRef, int32_t iOpt, int32_t iValue)
{
    if (iValue != 0)
    {
        NetGameUtilControl(pUtilRef, iOpt, iValue);
    }
}

/*F********************************************************************************/
/*!
    \Function _ConnApiDemangleReport

    \Description
        Initiate a demangler report to indicate connection success or failure.

    \Input *pConnApi    - pointer to module state
    \Input *pInfo       - connection info
    \Input eStatus      - connection result (success/fail)

    \Version 01/17/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiDemangleReport(ConnApiRefT *pConnApi, ConnApiConnInfoT *pInfo, ProtoMangleStatusE eStatus)
{
    if (pInfo->bDemangling != TRUE)
    {
        // not demangling, nothing to report
        return;
    }

    ProtoMangleReport(pConnApi->pProtoMangle, eStatus, -1);
    pConnApi->bReporting = TRUE;
    pInfo->bDemangling = FALSE;
}

/*F********************************************************************************/
/*!
    \Function _ConnApiRemoveClient

    \Description
        Remove a current client from a game.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - pointer to client to remove
    \Input iClientIndex - index of client to remove

    \Version 04/08/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiRemoveClient(ConnApiRefT *pConnApi, ConnApiClientT *pClient, int32_t iClientIndex)
{
    ConnApiConnStatusE eGameStatus = pClient->GameInfo.eStatus;
    ConnApiConnStatusE eVoipStatus = pClient->VoipInfo.eStatus;
    ConnApiConnStatusE eNewGameStatus;
    ConnApiConnStatusE eNewVoipStatus;

    // disconnect them
    _ConnApiDisconnectClient(pConnApi, pClient, iClientIndex, "removal");

    eNewGameStatus = pClient->GameInfo.eStatus;
    eNewVoipStatus = pClient->VoipInfo.eStatus;

    // if we were demangling them, abort demangling
    if ((pClient->GameInfo.bDemangling == TRUE) || (pClient->VoipInfo.bDemangling == TRUE))
    {
        NetPrintf(("connapi: aborting demangle of client 0x%08x - being removed from client list\n", pClient->ClientInfo.uId));
        ProtoMangleControl(pConnApi->pProtoMangle, 'abrt', iClientIndex, 0, NULL);
    }

    // decrement overall count
    pConnApi->ClientList.iNumClients -= 1;

    ds_memclr(&pConnApi->ClientList.Clients[iClientIndex], sizeof(ConnApiClientT));

    // send a disconnect event
    if (pConnApi->bRemoveCallback == TRUE)
    {
        if ((pConnApi->uConnFlags & CONNAPI_CONNFLAG_GAMECONN) != 0)
        {
            _ConnApiUpdateCallback(pConnApi, iClientIndex, CONNAPI_CBTYPE_GAMEEVENT, eGameStatus, eNewGameStatus);
        }
        if ((pConnApi->uConnFlags & CONNAPI_CONNFLAG_VOIPCONN) != 0)
        {
            _ConnApiUpdateCallback(pConnApi, iClientIndex, CONNAPI_CBTYPE_VOIPEVENT, eVoipStatus, eNewVoipStatus);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _ConnApiUpdateGameClient

    \Description
        Process game connection associated with this client.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - pointer to client to update
    \Input iClientIndex - index of client

    \Output
        int32_t         - one=active, zero=disconnected

    \Version 01/06/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ConnApiUpdateGameClient(ConnApiRefT *pConnApi, ConnApiClientT *pClient, int32_t iClientIndex)
{
    ConnApiConnStatusE eStatus = pClient->GameInfo.eStatus;
    uint8_t bLocalAddr;
    int32_t iConnTimeout;

    // are game connections disabled?
    if ((pConnApi->uConnFlags & CONNAPI_CONNFLAG_GAMECONN) == 0)
    {
        // if we're not connected, just bail
        if ((pClient->GameInfo.eStatus == CONNAPI_STATUS_INIT) || (pClient->GameInfo.eStatus == CONNAPI_STATUS_DISC))
        {
            return(0);
        }

        // if we're voip only and not already disconnected, kill the connection
        _ConnApiDestroyGameConnection(pConnApi, pClient, iClientIndex, "connection closed (voiponly)");
    }

    // handle initial connection state
    if (pClient->GameInfo.eStatus == CONNAPI_STATUS_INIT)
    {
        ConnApiClientT *pClientUsed;

        // get address to connect with
        int32_t iConnectAddr = _ConnApiGetConnectAddr(pConnApi, pClient, &bLocalAddr, CONNAPI_CONNFLAG_GAMECONN, &pClientUsed);

        //Save the physical address because iConnectAddr becomes prototunnel id if prototunnel is used.
        int32_t iPhysicalAddr = iConnectAddr;

        NetPrintf(("connapi: [0x%08x] establishing game connection to client 0x%08x:%a at %d\n", pConnApi, pClient->ClientInfo.uId, iConnectAddr, NetTick()));

        // create tunnel?
        if ((pConnApi->bTunnelEnabled) && (pClientUsed->iTunnelId == 0))
        {
            _ConnApiTunnelAlloc(pConnApi, pClientUsed, iClientIndex, iConnectAddr, bLocalAddr);
        }

        // do we have a tunnel to this client?
        if (pClientUsed->iTunnelId > 0)
        {
            NetPrintf(("connapi: [0x%08x] tunnel allocated for client %d (local id 0x%08x, remote id 0x%08x); switching to use virtual address %a\n", pConnApi,
                iClientIndex, pClient->ClientInfo.uLocalClientId,
                (pClient->ClientInfo.bIsConnectivityHosted ? pClient->ClientInfo.uHostingServerId : pClient->ClientInfo.uRemoteClientId), pClientUsed->iTunnelId));
            iConnectAddr = pClientUsed->iTunnelId;
        }

        // try to create game connection
        DirtyMemGroupEnter(pConnApi->iMemGroup, pConnApi->pMemGroupUserData);
        pClient->pGameUtilRef = NetGameUtilCreate();
        DirtyMemGroupLeave();
        if (pClient->pGameUtilRef != NULL)
        {
            char strConn[128], strConnName[64];
            int32_t iConnFlags;

            // set game link options
            _ConnApiSetGamelinkOpt(pClient->pGameUtilRef, 'minp', pConnApi->iGameMinp);
            _ConnApiSetGamelinkOpt(pClient->pGameUtilRef, 'mout', pConnApi->iGameMout);
            _ConnApiSetGamelinkOpt(pClient->pGameUtilRef, 'mwid', pConnApi->iGameMwid);
            if (pConnApi->iGameUnackLimit != 0)
            {
                _ConnApiSetGamelinkOpt(pClient->pGameUtilRef, 'ulmt', pConnApi->iGameUnackLimit);
            }

            // set our client id (used by gameserver to uniquely identify us)
            NetGameUtilControl(pClient->pGameUtilRef, 'clid', pClient->ClientInfo.uLocalClientId);
            NetGameUtilControl(pClient->pGameUtilRef, 'rcid', pClient->ClientInfo.uRemoteClientId);

            // determine connection parameters
            iConnFlags = _ConnApiGetConnectParms(pConnApi, pClient, strConnName, sizeof(strConnName));

            // format connect string
            ds_snzprintf(strConn, sizeof(strConn), "%a:%d:%d#%s", iConnectAddr,
                pClient->GameInfo.uLocalPort, pClient->GameInfo.uMnglPort, strConnName);

            // start the connection attempt
            NetGameUtilConnect(pClient->pGameUtilRef, iConnFlags, strConn, pConnApi->pCommConstruct);
            pClient->GameInfo.eStatus = CONNAPI_STATUS_CONN;

            // set if this is a GameServer connection or not
            if ((iPhysicalAddr == (signed)pConnApi->uGameServAddr) && (pConnApi->uGameServAddr != 0))
            {
                pClient->GameInfo.uConnFlags |= CONNAPI_CONNFLAG_GAMESERVER;
            }

            // remember connection start time
            pClient->GameInfo.iConnStart = NetTick();

            NetGameUtilControl(pClient->pGameUtilRef, 'meta', (pConnApi->bCommUdpMetadata || (pClient->ClientInfo.bIsConnectivityHosted)) ? 1 : 0);
        }
        else
        {
            NetPrintf(("connapi: unable to allocate util ref for connection %d to client 0x%08x\n", iClientIndex, pClient->ClientInfo.uId));
            pClient->GameInfo.eStatus = CONNAPI_STATUS_DISC;
            _ConnApiClientReleaseProtoMangleResources(pConnApi, pClient);
        }
    }

    // waiting for connection
    if (pClient->GameInfo.eStatus == CONNAPI_STATUS_CONN)
    {
        void *pCommRef;

        if (pClient->pGameLinkRef == NULL)
        {
            // check for established connection
            if ((pCommRef = NetGameUtilComplete(pClient->pGameUtilRef)) != NULL)
            {
                DirtyMemGroupEnter(pConnApi->iMemGroup, pConnApi->pMemGroupUserData);
                pClient->pGameLinkRef = NetGameLinkCreate(pCommRef, FALSE, pConnApi->iLinkBufSize);
                DirtyMemGroupLeave();
                if (pClient->pGameLinkRef != NULL)
                {
                    NetPrintf(("connapi: game connection %d to client 0x%08x established at %d\n", iClientIndex, pClient->ClientInfo.uId, NetTick()));

                    if (pClient->ClientInfo.bEnableQos)
                    {
                        NetPrintf(("connapi: enabling QoS in NetGameLink on connection %d to client 0x%08x\n", iClientIndex, pClient->ClientInfo.uId));
                        NetGameLinkControl(pClient->pGameLinkRef, 'sqos', pConnApi->iQosDuration, &pConnApi->iQosInterval);
                        NetGameLinkControl(pClient->pGameLinkRef, 'lqos', pConnApi->iQosPacketSize, NULL);
                    }

                    // if we were demangling, report success
                    _ConnApiDemangleReport(pConnApi, &pClient->GameInfo, PROTOMANGLE_STATUS_CONNECTED);

                    // save socket ref for multi-demangle if we need it
                    if (!pConnApi->bTunnelEnabled)
                    {
                        NetGameUtilStatus(pClient->pGameUtilRef, 'sock', &pConnApi->uGameSockRef, sizeof(pConnApi->uGameSockRef));
                    }

                    // indicate we've connected
                    pClient->GameInfo.uConnFlags |= CONNAPI_CONNFLAG_CONNECTED;
                }
                else
                {
                    NetPrintf(("connapi: unable to allocate link ref for connection %d to client 0x%08x\n", iClientIndex, pClient->ClientInfo.uId));
                    pClient->GameInfo.eStatus = CONNAPI_STATUS_DISC;
                    _ConnApiClientReleaseProtoMangleResources(pConnApi, pClient);
                }
            }
        }

        // check for gamelink saying we're connected
        if (pClient->pGameLinkRef != NULL)
        {
            NetGameLinkStatT Stat;

            // give time to NetGameLink to run any connection-related processes
            NetGameLinkUpdate(pClient->pGameLinkRef);

            // get link stats
            NetGameLinkStatus(pClient->pGameLinkRef, 'stat', 0, &Stat, sizeof(NetGameLinkStatT));

            // see if we're open
            if (Stat.isopen == TRUE)
            {
                // mark as active
                NetPrintf(("connapi: game connection %d to client 0x%08x is active at %d\n", iClientIndex, pClient->ClientInfo.uId, NetTick()));
                pClient->GameInfo.eStatus = CONNAPI_STATUS_ACTV;
            }
        }

        // check for connection timeout
        iConnTimeout = pConnApi->iConnTimeout;

#if defined(DIRTYCODE_XBOXONE)
        // on xboxone, we want the fallback connection timeout to be twice the p2p connection timeout to cover for possible cases
        // where p2p connectivity windows for both ends of connection do not overlap
        if (pClient->GameInfo.uPeerConnFailed & CONNAPI_CONNFLAG_GAMECONN)
        {
            iConnTimeout = iConnTimeout * 2;

            #if DIRTYCODE_LOGGING
            if (pClient->GameInfo.bFallbackTimeoutLogged == FALSE)
            {
                NetPrintf(("connapi: [0x%08x] connection timeout while attempting fallback is %d ms\n", pConnApi, iConnTimeout));
                pClient->GameInfo.bFallbackTimeoutLogged = TRUE;
            }
            #endif
        }
#endif

        // check for connection timeout
        // The connection timeout, and a subsequent disconnection, should only occur if we still have not connected to the peer.
        // If we have a pGameLinkRef, then that means we MUST have established a connection to the peer, but are still doing QoS.
        if ((pClient->GameInfo.eStatus == CONNAPI_STATUS_CONN) && (pClient->pGameLinkRef == NULL) && (NetTickDiff(NetTick(), pClient->GameInfo.iConnStart) > iConnTimeout))
        {
            _ConnApiDestroyGameConnection(pConnApi, pClient, iClientIndex, "connection timeout");

// on xboxone, we never want to go back to MNGL state from CONN state
#if defined(DIRTYCODE_XBOXONE)
            NetPrintf(("connapi: game connection to client 0x%08x failed\n", pClient->ClientInfo.uId));
            if (pClient->GameInfo.bDemangling)
            {
                _ConnApiDemangleReport(pConnApi, &pClient->GameInfo, PROTOMANGLE_STATUS_FAILED);
            }

            pClient->GameInfo.eStatus = CONNAPI_STATUS_DISC;
#else
            // initial attempt to connect failed
            if (pClient->GameInfo.bDemangling == FALSE)
            {
                if (pConnApi->bDemanglerEnabled && pClient->ClientInfo.bIsConnectivityHosted == FALSE)
                {
                    NetPrintf(("connapi: game status=mngl for connection %d to client 0x%08x\n", iClientIndex, pClient->ClientInfo.uId));
                    pClient->GameInfo.eStatus = CONNAPI_STATUS_MNGL;
                }
                else
                {
                    if (pConnApi->bDemanglerEnabled && pClient->ClientInfo.bIsConnectivityHosted == TRUE)
                    {
                        NetPrintf(("connapi: demangling skipped for connection %d to client 0x%08x because the client is connectivity hosted\n", iClientIndex, pClient->ClientInfo.uId));
                    }

                    pClient->GameInfo.eStatus = CONNAPI_STATUS_DISC;
                }
            }
            else
            {
                NetPrintf(("connapi: game connection to client 0x%08x after demangle failed\n", pClient->ClientInfo.uId));
                _ConnApiDemangleReport(pConnApi, &pClient->GameInfo, PROTOMANGLE_STATUS_FAILED);

                pClient->GameInfo.eStatus = CONNAPI_STATUS_DISC;
            }
#endif
        } // timeout

        // set the packet receive flag if a packet was received
        if ((pClient->pGameUtilRef != NULL) && (NetGameUtilStatus(pClient->pGameUtilRef, 'pkrc', NULL, 0) == TRUE))
        {
            pClient->GameInfo.uConnFlags |= CONNAPI_CONNFLAG_PKTRECEIVED;
        }
    } // waiting for connection

    // update connection status during active phase
    if (pClient->GameInfo.eStatus == CONNAPI_STATUS_ACTV)
    {
        // get link stats
        NetGameLinkStatT Stat;
        NetGameLinkStatus(pClient->pGameLinkRef, 'stat', 0, &Stat, sizeof(NetGameLinkStatT));

        // make sure connection is still open
        if (Stat.isopen == FALSE)
        {
            _ConnApiDestroyGameConnection(pConnApi, pClient, iClientIndex, "connection closed");
        }
        // see if we've timed out
        else if (NetTickDiff(Stat.tick, Stat.rcvdlast) > pConnApi->iTimeout)
        {
            _ConnApiDestroyGameConnection(pConnApi, pClient, iClientIndex, "connection timed out");
        }
    }

    // trigger callback if state change
    _ConnApiUpdateCallback(pConnApi, iClientIndex, CONNAPI_CBTYPE_GAMEEVENT, eStatus, (ConnApiConnStatusE)pClient->GameInfo.eStatus);

    // return active or inactive
    return(pClient->GameInfo.eStatus != CONNAPI_STATUS_DISC);
}

/*F********************************************************************************/
/*!
    \Function _ConnApiUpdateVoipClient

    \Description
        Process voip connection associated with this connection.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - pointer to connection to update
    \Input iClientIndex - index of connection

    \Output
        int32_t         - one=active, zero=disconnected

    \Version 01/06/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ConnApiUpdateVoipClient(ConnApiRefT *pConnApi, ConnApiClientT *pClient, int32_t iClientIndex)
{
    ConnApiConnStatusE eStatus = pClient->VoipInfo.eStatus;
    int32_t iVoipConnId, iVoipStatus = 0;
    uint8_t bLocalAddr;

    // are voip connections disabled?
    if ((pConnApi->uConnFlags & CONNAPI_CONNFLAG_VOIPCONN) == 0)
    {
        // if we're not connected, just bail
        if ((pClient->VoipInfo.eStatus == CONNAPI_STATUS_INIT) || (pClient->VoipInfo.eStatus == CONNAPI_STATUS_DISC))
        {
            return(0);
        }

        // if we're game only and not already disconnected, close the connection
        _ConnApiDestroyVoipConnection(pConnApi, pClient, "connection closed (gameonly)");
    }

    // handle initial connection state
    if (pClient->VoipInfo.eStatus == CONNAPI_STATUS_INIT)
    {
        ConnApiClientT *pClientUsed;

        // get address to connect with
        int32_t iConnectAddr = _ConnApiGetConnectAddr(pConnApi, pClient, &bLocalAddr, CONNAPI_CONNFLAG_VOIPCONN, &pClientUsed);

        //Save the physical address because iConnectAddr becomes prototunnel id if prototunnel is used.
        int32_t iPhysicalAddr = iConnectAddr;

        NetPrintf(("connapi: [0x%08x] establishing voip connection to client 0x%08x:%a at %d\n", pConnApi,
            pClient->ClientInfo.uId, iConnectAddr, NetTick()));

        // create tunnel?
        if ((pConnApi->bTunnelEnabled) && (pClientUsed->iTunnelId == 0))
        {
            _ConnApiVoipTunnelAlloc(pConnApi, pClientUsed, iClientIndex, iConnectAddr, bLocalAddr);
        }

        // do we have a tunnel to this client?
        if (pClientUsed->iTunnelId > 0)
        {
            NetPrintf(("connapi: [0x%08x] tunnel allocated for client %d; switching to use virtual address %a\n", pConnApi,
                iClientIndex, pClientUsed->iTunnelId));
            iConnectAddr = pClientUsed->iTunnelId;
        }

        // initiate connection attempt
        VoipGroupControl(pConnApi->pVoipGroupRef, 'vcid', iClientIndex, 0, &pClient->ClientInfo.uLocalClientId);        
        iVoipConnId = VoipGroupConnect2(pConnApi->pVoipGroupRef, iClientIndex, iConnectAddr, pClient->VoipInfo.uMnglPort, pClient->VoipInfo.uLocalPort,
                                        pClient->ClientInfo.uId, pConnApi->iSessId, pClient->ClientInfo.bIsConnectivityHosted, pClient->ClientInfo.uRemoteClientId);

        if (iVoipConnId >= 0)
        {
            pClient->iVoipConnId = iVoipConnId;
            pClient->VoipInfo.eStatus = CONNAPI_STATUS_CONN;
            pClient->VoipInfo.iConnStart = NetTick();

            // set if this is a GameServer connection or not
            if ((iPhysicalAddr == (signed)pConnApi->uGameServAddr) && (pConnApi->uGameServAddr != 0))
            {
                pClient->VoipInfo.uConnFlags |= CONNAPI_CONNFLAG_GAMESERVER;
            }
        }
        else
        {
            NetPrintf(("connapi: unable to init voip for connection %d to client 0x%08x\n", iClientIndex, pClient->ClientInfo.uId));
            pClient->VoipInfo.eStatus = CONNAPI_STATUS_DISC;
            _ConnApiClientReleaseProtoMangleResources(pConnApi, pClient);
        }
    }

    // get connection status
    if ((pClient->VoipInfo.eStatus == CONNAPI_STATUS_CONN) || (pClient->VoipInfo.eStatus == CONNAPI_STATUS_ACTV))
    {
        iVoipStatus = VoipGroupConnStatus(pConnApi->pVoipGroupRef, pClient->iVoipConnId);
    }

    // for both cases below, CONNAPI_STATUS_CONN and CONNAPI_STATUS_ACTV, when we detect a disconnection,
    // we won't set the iVoipConnId to NONE nor trigger a disconnection. This is because the voipgroup code,
    // in order to provide correct voip connection sharing, needs to be told only of authoritative "connect"
    // and "disconnect" by the higher-level lobby (plasma, lobbysdk, blazesdk, ...) techonology.

    // update connection status during connect phase
    if (pClient->VoipInfo.eStatus == CONNAPI_STATUS_CONN)
    {
        // check for established connection
        if (iVoipStatus & VOIP_CONN_CONNECTED)
        {
            NetPrintf(("connapi: voip connection %d to client 0x%08x established at %d\n", iClientIndex, pClient->ClientInfo.uId, NetTick()));

            // if we were demangling, report success
            _ConnApiDemangleReport(pConnApi, &pClient->VoipInfo, PROTOMANGLE_STATUS_CONNECTED);

            // save socket ref for multi-demangle if we need it
            if (!pConnApi->bTunnelEnabled)
            {
                VoipGroupStatus(pConnApi->pVoipGroupRef, 'sock', 0, &pConnApi->uVoipSockRef, sizeof(pConnApi->uVoipSockRef));
            }

            // mark as active
            pClient->VoipInfo.eStatus = CONNAPI_STATUS_ACTV;
            pClient->VoipInfo.uConnFlags |= CONNAPI_CONNFLAG_CONNECTED;
        }
        else if (iVoipStatus & VOIP_CONN_STOPPED)
        {
            NetPrintf(("connapi: voip connection attempt to client 0x%08x failed at %d\n", pClient->ClientInfo.uId, NetTick()));

// on xboxone, we never want to go back to MNGL state from CONN state
#if defined(DIRTYCODE_XBOXONE)
            NetPrintf(("connapi: voip connection attempt to client 0x%08x failed\n", pClient->ClientInfo.uId));
            if (pClient->VoipInfo.bDemangling == TRUE)
            {
                _ConnApiDemangleReport(pConnApi, &pClient->VoipInfo, PROTOMANGLE_STATUS_FAILED);
            }

            // change status if fallback is enabled for voip connections, otherwise, disconnect
            pClient->VoipInfo.eStatus = CONNAPI_STATUS_DISC;
#else
            if (pClient->VoipInfo.bDemangling == TRUE)
            {
                NetPrintf(("connapi: voip connection attempt to client 0x%08x after demangle failed\n", pClient->ClientInfo.uId));
                _ConnApiDemangleReport(pConnApi, &pClient->VoipInfo, PROTOMANGLE_STATUS_FAILED);

                // change status if fallback is enabled for voip connections, otherwise, disconnect
                pClient->VoipInfo.eStatus = CONNAPI_STATUS_DISC;
            }
            else
            {
                if (pConnApi->bDemanglerEnabled && pClient->ClientInfo.bIsConnectivityHosted == FALSE)
                {
                    NetPrintf(("connapi: voip status=mngl for connection %d to client 0x%08x\n", iClientIndex, pClient->ClientInfo.uId));
                    pClient->VoipInfo.eStatus = CONNAPI_STATUS_MNGL;
                }
                else
                {
                    if (pConnApi->bDemanglerEnabled && pClient->ClientInfo.bIsConnectivityHosted == TRUE)
                    {
                        NetPrintf(("connapi: demangling skipped for voip connection %d to client 0x%08x because the client is connectivity hosted\n", iClientIndex, pClient->ClientInfo.uId));
                    }

                    // change status if fallback is enabled for voip connections, otherwise, disconnect
                    pClient->VoipInfo.eStatus = CONNAPI_STATUS_DISC;
                }
            } // bDemangling = FALSE
#endif
        } // REMOTE_DISCONNECTED
    } // STATUS_CONN

    // update client in active state
    if (pClient->VoipInfo.eStatus == CONNAPI_STATUS_ACTV)
    {
        if (iVoipStatus & VOIP_CONN_STOPPED)
        {
            NetPrintf(("connapi: voip connection to client 0x%08x terminated at %d\n", pClient->ClientInfo.uId, NetTick()));
            pClient->VoipInfo.eStatus = CONNAPI_STATUS_DISC;
            _ConnApiClientReleaseProtoMangleResources(pConnApi, pClient);
        }
    }

    // trigger callback if state change
    _ConnApiUpdateCallback(pConnApi, iClientIndex, CONNAPI_CBTYPE_VOIPEVENT, eStatus, (ConnApiConnStatusE)pClient->VoipInfo.eStatus);

    // return active or inactive
    return(pClient->VoipInfo.eStatus != CONNAPI_STATUS_DISC);
}

/*F********************************************************************************/
/*!
    \Function _ConnApiUpdateDemangleReport

    \Description
        Update demangler during the Report phase.

    \Input *pConnApi    - pointer to module state

    \Output
        uint32_t        - TRUE if reporting is in progress, FALSE otherwise

    \Version 01/17/2005 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _ConnApiUpdateDemangleReport(ConnApiRefT *pConnApi)
{
    // if not reporting, don't process
    if (pConnApi->bReporting != FALSE)
    {
        // update client
        ProtoMangleUpdate(pConnApi->pProtoMangle);

        #if defined(DIRTYCODE_XBOXONE)
        // there is no reporting phase on xbox one... so we alway fake that reporting is complete
        pConnApi->bReporting = FALSE;
        #else
        // check for completion
        if (ProtoMangleComplete(pConnApi->pProtoMangle, NULL, NULL) != 0)
        {
            pConnApi->bReporting = FALSE;
        }
        #endif
    }

    return(pConnApi->bReporting);
}

/*F********************************************************************************/
/*!
    \Function _ConnApiUpdateDemangle

    \Description
        Update client connection in CONNAPI_STATUS_MNGL state.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - pointer to client to update
    \Input iClientIndex - index of client
    \Input *pConnInfo   - pointer to connection info (game or voip)
    \Input iType        - type of connection (zero=game, one=voip)

    \Version 01/13/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiUpdateDemangle(ConnApiRefT *pConnApi, ConnApiClientT *pClient, int32_t iClientIndex, ConnApiConnInfoT *pConnInfo, int32_t iType)
{
    static const char _Types[2][5] = { "game", "voip" };
    static uint32_t _Flags[2] = { CONNAPI_CONNFLAG_GAMECONN, CONNAPI_CONNFLAG_VOIPCONN };
    ConnApiCbTypeE eCbType;

#if !defined(DIRTYCODE_XBOXONE)
    int32_t iClient;
#endif

    // initialize eType
    if (iType == 0)
    {
        eCbType = CONNAPI_CBTYPE_GAMEEVENT;
    }
    else
    {
        eCbType = CONNAPI_CBTYPE_VOIPEVENT;
    }

    // ignore game/voip demangle if we're not doing game/voip connections
    if ((_Flags[iType] & pConnApi->uConnFlags) == 0)
    {
        return;
    }

#if !defined(DIRTYCODE_XBOXONE)
    // if anyone is in a connecting state, wait to demangle
    for (iClient = 0; iClient < pConnApi->ClientList.iMaxClients; iClient++)
    {
        if ((pConnApi->ClientList.Clients[iClient].GameInfo.eStatus == CONNAPI_STATUS_CONN) ||
            (pConnApi->ClientList.Clients[iClient].VoipInfo.eStatus == CONNAPI_STATUS_CONN))
        {
            NetPrintf(("connapi: [0x%08x] deferring demangle until there are no ongoing connection attempts\n", pConnApi));
            return;
        }
    }
#endif

    // tunnel-specific processing
    if (pConnApi->bTunnelEnabled)
    {
        // if we've already demangled the tunnel port, use previous demangle result
        if (pClient->uFlags & CONNAPI_CLIENTFLAG_TUNNELPORTDEMANGLED)
        {
            ConnApiConnStatusE eStatus = pConnInfo->eStatus;
            pConnInfo->eStatus = CONNAPI_STATUS_INIT;
            pConnInfo->uConnFlags |= CONNAPI_CONNFLAG_DEMANGLED;
            
            NetPrintf(("connapi: [0x%08x] reusing previously demangled tunnel port\n", pConnApi));

            // trigger callback if state change
            _ConnApiUpdateCallback(pConnApi, iClientIndex, eCbType, eStatus, (ConnApiConnStatusE)pConnInfo->eStatus);
            return;
        }
    }
    else
    {
        // if we've already resolved the secure address, use it
        if (pClient->uFlags & CONNAPI_CLIENTFLAG_SECUREADDRRESOLVED)
        {
            ConnApiConnStatusE eStatus = pConnInfo->eStatus;
            pConnInfo->eStatus = CONNAPI_STATUS_INIT;
            pConnInfo->uConnFlags |= CONNAPI_CONNFLAG_DEMANGLED;

            NetPrintf(("connapi: [0x%08x] reusing previously resolved secure address\n", pConnApi));

            // trigger callback if state change
            _ConnApiUpdateCallback(pConnApi, iClientIndex, eCbType, eStatus, (ConnApiConnStatusE)pConnInfo->eStatus);
            return;
        }
    }

    // are we in an idle state?
    if (ProtoMangleStatus(pConnApi->pProtoMangle, 'idle', NULL, iClientIndex))
    {
        char strSess[64];
        intptr_t uSockRef;
        uint32_t uDemanglePort = 0;

        // show we're demangling
        pConnInfo->bDemangling = TRUE;

        // create session id
        _ConnApiGenerateSessionKey(pConnApi, pClient, iClientIndex, strSess, sizeof(strSess), _Types[iType]);

        // get socket ref
        if (pConnApi->bTunnelEnabled)
        {
            uSockRef = pConnApi->uTunlSockRef;
        }
        else if (iType == 0)
        {
            uSockRef = pConnApi->uGameSockRef;
        }
        else
        {
            uSockRef = pConnApi->uVoipSockRef;
        }

        // if socket ref is null, connect normally
        if (uSockRef == 0)
        {
            #if !CONNAPI_RNDLCLDEMANGLEPORT
            uDemanglePort = (iType == 0) ? pConnApi->uGamePort : pConnApi->uVoipPort;
            uDemanglePort = (pConnApi->bTunnelEnabled) ? (unsigned)pConnApi->iTunnelPort : uDemanglePort;
            #else
            uDemanglePort = _ConnApiGenerateDemanglePort(pConnApi);
            if (pConnApi->bTunnelEnabled)
            {
                // if we're tunneling, we need to recreate the tunnel socket bound to the new port
                ProtoTunnelControl(pConnApi->pProtoTunnel, 'bind', uDemanglePort, 0, NULL);
            }
            else
            {
                // if not tunneling, we need to update the local port info with the new local (bind) demangle port
                pConnInfo->uLocalPort = (uint16_t)uDemanglePort;
            }
            #endif
            // if tunneling we always want to use the tunnel socket ref for demangling
            if (pConnApi->bTunnelEnabled)
            {
                ProtoTunnelStatus(pConnApi->pProtoTunnel, 'sock', 0, &pConnApi->uTunlSockRef, sizeof(pConnApi->uTunlSockRef));
                uSockRef = pConnApi->uTunlSockRef;
            }
        }

        if (iType == 0)
        {
            pClient->GameInfo.iConnStart = NetTick();
        }
        else
        {
            pClient->VoipInfo.iConnStart = NetTick();
        }

#if defined(DIRTYCODE_XBOXONE)
        {
            // since the SecureDeviceAddr fits in the DirtyAddrT, it is safe to use a buffer as large as a DirtyAddrT
            char aSecureDeviceAddressBlob[DIRTYADDR_MACHINEADDR_MAXLEN];
            int32_t iBlobSize;

            NetPrintf(("connapi: initiating %s secure address resolution of client 0x%08x at %d\n", _Types[iType], pClient->ClientInfo.uId, NetTick()));
            DirtyAddrGetInfoXboxOne((DirtyAddrT *)(pConnApi->ClientList.Clients[iClientIndex].ClientInfo.DirtyAddr.strMachineAddr), NULL, aSecureDeviceAddressBlob, &iBlobSize);

            ProtoMangleConnect2(pConnApi->pProtoMangle, iClientIndex, aSecureDeviceAddressBlob, iBlobSize);
        }
#else
        // before demangling, flush the tunnel to make sure there are no buffered packets
        if (pConnApi->bTunnelEnabled)
        {
            ProtoTunnelControl(pConnApi->pProtoTunnel, 'flsh', pClient->iTunnelId, 0, NULL);
        }

        // kick off demangling process
        if (uSockRef == 0)
        {
            NetPrintf(("connapi: initiating %s:%d demangle of client 0x%08x at %d\n", _Types[iType], uDemanglePort, pClient->ClientInfo.uId, NetTick()));
            ProtoMangleConnect(pConnApi->pProtoMangle, uDemanglePort, strSess);

        }
        else
        {
            NetPrintf(("connapi: initiating %s demangle of client 0x%08x using sockref 0x%08x at %d\n", _Types[iType], pClient->ClientInfo.uId, uSockRef, NetTick()));
            ProtoMangleConnectSocket(pConnApi->pProtoMangle, uSockRef, strSess);
        }
#endif
    }
    else
    {
        if (pConnInfo->bDemangling != FALSE)
        {
            int32_t iAddr, iPort, iResult;

            // update client
            ProtoMangleUpdate(pConnApi->pProtoMangle);

            // check for completion
            #if defined(DIRTYCODE_XBOXONE)
            iPort = iClientIndex;
            #endif
            if ((iResult = ProtoMangleComplete(pConnApi->pProtoMangle, &iAddr, &iPort)) != 0)
            {
                ConnApiConnStatusE eStatus = pConnInfo->eStatus;

                if (eStatus != CONNAPI_STATUS_ACTV)
                {
                    if (iResult > 0)
                    {
    #if defined(DIRTYCODE_XBOXONE)
                        NetPrintf(("connapi: %s secure address resolution for user 0x%08x is successful ipaddr=%a at %d\n",
                            _Types[iType], pClient->ClientInfo.uId, iAddr, NetTick()));
                        iPort = pConnInfo->uMnglPort;
    #else
                        NetPrintf(("connapi: %s demangle of client 0x%08x successful port=%d at %d\n", _Types[iType], pClient->ClientInfo.uId, iPort, NetTick()));
    #endif

                        pClient->ClientInfo.uAddr = pClient->ClientInfo.uLocalAddr = iAddr;
                        if (pConnApi->bTunnelEnabled)
                        {
                            // for xboxone, clients do not yet have a valid tunnel id at this point because client enters to MNGL state before INIT state (unlike other platforms)
                            #if !defined(DIRTYCODE_XBOXONE)
                            ProtoTunnelControl(pConnApi->pProtoTunnel, 'rprt', pClient->iTunnelId, iPort, NULL);
                            #endif

                            pClient->uFlags |= CONNAPI_CLIENTFLAG_TUNNELPORTDEMANGLED;
                        }
                        else
                        {
                            pConnInfo->uMnglPort = (uint16_t)iPort;

                            #if defined(DIRTYCODE_XBOXONE)
                            pClient->uFlags |= CONNAPI_CLIENTFLAG_SECUREADDRRESOLVED;
                            #endif
                        }
                        pConnInfo->eStatus = CONNAPI_STATUS_INIT;
                        pConnInfo->uConnFlags |= CONNAPI_CONNFLAG_DEMANGLED;
                    }
                    else
                    {
                        NetPrintf(("connapi: %s demangle of client 0x%08x failed (timeout=%s)\n", _Types[iType], pClient->ClientInfo.uId,
                            ProtoMangleStatus(pConnApi->pProtoMangle, 'time', NULL, iClientIndex) ? "true" : "false"));
                        pConnInfo->eStatus = CONNAPI_STATUS_DISC;
                    }
                }
                else
                {
                    NetPrintf(("connapi: [0x%08x] %s demangle of client 0x%08x finished with %d but ignored because connection aleady active (timeout=%s)\n", pConnApi,
                        _Types[iType], pClient->ClientInfo.uId, iResult, ProtoMangleStatus(pConnApi->pProtoMangle, 'time', NULL, iClientIndex) ? "true" : "false"));
                }

                // trigger callback if state change
                _ConnApiUpdateCallback(pConnApi, iClientIndex, eCbType, eStatus, (ConnApiConnStatusE)pConnInfo->eStatus);
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _ConnApiUpdateRemoval

    \Description
        Scan through client list and remove clients marked for removal.

    \Input *pConnApi    - pointer to module state

    \Version 04/11/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiUpdateRemoval(ConnApiRefT *pConnApi)
{
    ConnApiClientT *pClient;
    int32_t iClientIndex;

    for (iClientIndex = 0; iClientIndex < pConnApi->ClientList.iMaxClients; iClientIndex++)
    {
        // ref client
        pClient = &pConnApi->ClientList.Clients[iClientIndex];

        // if client needs to be removed, remove them
        if (pClient->uFlags & CONNAPI_CLIENTFLAG_REMOVE)
        {
            _ConnApiRemoveClient(pConnApi, pClient, iClientIndex);
        }
    }
}


/*F********************************************************************************/
/*!
    \Function _ConnApiRemoveClientSetup

    \Description
        Set up a client for removal from the game.

    \Input *pConnApi    - pointer to module state
    \Input iClientIndex - index of client to remove (used if pClientName is NULL)
    \Input uFlags       - client removal flags

    \Notes
        If this function is called inside of a ConnApi callback, the removal will
        be deferred until the next time NetConnIdle() is called.  Otherwise, the
        removal will happen immediately.

    \Version 04/08/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiRemoveClientSetup(ConnApiRefT *pConnApi, int32_t iClientIndex, uint16_t uFlags)
{
    ConnApiClientT *pClient;

    // don't allow self removal
    if (iClientIndex == pConnApi->iSelf)
    {
        NetPrintf(("connapi: [0x%08x] can't remove self from game\n", pConnApi));
        return;
    }

    // ref the client and mark them for removal
    pClient = &pConnApi->ClientList.Clients[iClientIndex];
    pClient->uFlags |= uFlags;

    // if we're not in a callback, do the removal immediately
    if (pConnApi->bInCallback == FALSE)
    {
        _ConnApiUpdateRemoval(pConnApi);
    }
}

/*F********************************************************************************/
/*!
    \Function _ConnApiUpdateConnections

    \Description
        Update ConnApi connections.

    \Input *pConnApi    - pointer to module state

    \Output
        int32_t         - number of connections that are not in the DISC state

    \Version 01/06/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ConnApiUpdateConnections(ConnApiRefT *pConnApi)
{
    ConnApiClientT *pClient;
    int32_t iActive, iClientIndex;
    uint32_t bDemangling;

    // update game connections
    for (iActive = 0, iClientIndex = 0; iClientIndex < pConnApi->ClientList.iMaxClients; iClientIndex++)
    {
        // ref connection
        pClient = &pConnApi->ClientList.Clients[iClientIndex];

        // don't update if iClientIndex is us or unallocated
        if ((iClientIndex == pConnApi->iSelf) || !pClient->bAllocated)
        {
            continue;
        }

        if (pClient->uConnFlags & CONNAPI_CONNFLAG_GAMECONN)
        {
            // process game connection
            iActive += _ConnApiUpdateGameClient(pConnApi, pClient, iClientIndex);
        }
    }

    // update voip connections
    if (pConnApi->pVoipRef != NULL)
    {
        for (iClientIndex = 0; iClientIndex < pConnApi->ClientList.iMaxClients; iClientIndex++)
        {
            // do not proceed with the voip connection to the host if voip is redirected through
            // the host or we specified not too
            if (pConnApi->bVoipRedirectViaHost || !pConnApi->bVoipConnectHost)
            {
                if (iClientIndex == pConnApi->iHostIndex)
                {
                    continue;
                }
            }

            // ref connection
            pClient = &pConnApi->ClientList.Clients[iClientIndex];

            // don't update if iClientIndex is us or unallocated
            if ((iClientIndex == pConnApi->iSelf) || !pClient->bAllocated)
            {
                continue;
            }

            if (pClient->uConnFlags & CONNAPI_CONNFLAG_VOIPCONN)
            {
                // process voip connection
                iActive += _ConnApiUpdateVoipClient(pConnApi, pClient, iClientIndex);
            }
        }
    }

    // update reporting
    bDemangling = _ConnApiUpdateDemangleReport(pConnApi);

    // update game demangling
    for (iClientIndex = 0; iClientIndex < pConnApi->ClientList.iMaxClients; iClientIndex++)
    {
        // ref connection
        pClient = &pConnApi->ClientList.Clients[iClientIndex];

        // don't update if iClientIndex is us or unallocated
        if ((iClientIndex == pConnApi->iSelf) || !pClient->bAllocated)
        {
            continue;
        }

        // demangle game connection?
#if defined(DIRTYCODE_XBOXONE)
        // on xboxone, we don't serialize demangling of different clients, we want them occurring in parallel
        if ((pClient->GameInfo.eStatus == CONNAPI_STATUS_MNGL) && (pClient->uConnFlags & CONNAPI_CONNFLAG_GAMECONN))
#else
        if ((pClient->GameInfo.eStatus == CONNAPI_STATUS_MNGL) && (pClient->uConnFlags & CONNAPI_CONNFLAG_GAMECONN) && (bDemangling == FALSE))
#endif

        {
            _ConnApiUpdateDemangle(pConnApi, pClient, iClientIndex, &pClient->GameInfo, 0);
        }
        bDemangling |= pClient->GameInfo.bDemangling;
    }

    // update voip demangling
    for (iClientIndex = 0; iClientIndex < pConnApi->ClientList.iMaxClients; iClientIndex++)
    {
        if (pConnApi->bVoipRedirectViaHost)
        {
            if (iClientIndex == pConnApi->iHostIndex)
            {
                continue;
            }
        }

        // ref connection
        pClient = &pConnApi->ClientList.Clients[iClientIndex];

        // don't update if iClientIndex is us or unallocated
        if ((iClientIndex == pConnApi->iSelf) || !pClient->bAllocated)
        {
            continue;
        }

        // demangle voip connection?
#if defined(DIRTYCODE_XBOXONE)
        // on xboxone, we don't serialize demangling of different clients, we want them occurring in parallel
        if ((pClient->VoipInfo.eStatus == CONNAPI_STATUS_MNGL))
#else
        if ((pClient->VoipInfo.eStatus == CONNAPI_STATUS_MNGL) && (bDemangling == FALSE))
#endif

        {
            _ConnApiUpdateDemangle(pConnApi, pClient, iClientIndex, &pClient->VoipInfo, 1);
        }
    }

    // update tunnel
    if ((pConnApi->bTunnelEnabled != 0) && (pConnApi->pProtoTunnel != NULL))
    {
        ProtoTunnelUpdate(pConnApi->pProtoTunnel);
    }

    return(iActive);
}

/*F********************************************************************************/
/*!
    \Function _ConnApiUpdateGameServConnection

    \Description
        Update ConnApi connection to GameServer in PEERCONN mode.

    \Input *pConnApi    - pointer to module state

    \Output
        int32_t         - one

    \Version 02/22/2007 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ConnApiUpdateGameServConnection(ConnApiRefT *pConnApi)
{
    ConnApiClientT *pClient;
    uint32_t bDemangling;
    int32_t iClientIndex;

    // process game connection to gameserver
    if (pConnApi->ClientList.iNumClients > 0)
    {
        _ConnApiUpdateGameClient(pConnApi, &pConnApi->GameServer, -1);
    }

    // update voip connections
    if (pConnApi->pVoipRef != NULL)
    {
        for (iClientIndex = 0; iClientIndex < pConnApi->ClientList.iMaxClients; iClientIndex++)
        {
            // ref connection
            pClient = &pConnApi->ClientList.Clients[iClientIndex];

            // don't update if iClientIndex is us or unallocated
            if ((iClientIndex == pConnApi->iSelf) || !pClient->bAllocated)
            {
                continue;
            }

            // process voip connection
            _ConnApiUpdateVoipClient(pConnApi, pClient, iClientIndex);
        }
    }

    // ref gameserver client
    pClient = &pConnApi->GameServer;

    // update reporting
    bDemangling = _ConnApiUpdateDemangleReport(pConnApi);

    // update game demangling
    if (pConnApi->ClientList.iNumClients > 0)
    {
        if ((pClient->GameInfo.eStatus == CONNAPI_STATUS_MNGL) && (bDemangling == FALSE))
        {
            _ConnApiUpdateDemangle(pConnApi, pClient, -1, &pClient->GameInfo, 0);
        }
        bDemangling |= pClient->GameInfo.bDemangling;
    }

    // update voip demangling
    for (iClientIndex = 0; iClientIndex < pConnApi->ClientList.iMaxClients; iClientIndex++)
    {
        // ref connection
        pClient = &pConnApi->ClientList.Clients[iClientIndex];

        // don't update if iClientIndex is us or unallocated
        if ((iClientIndex == pConnApi->iSelf) || !pClient->bAllocated)
        {
            continue;
        }

        // demangle voip connection?
        if ((pClient->VoipInfo.eStatus == CONNAPI_STATUS_MNGL) && (bDemangling == FALSE))
        {
            _ConnApiUpdateDemangle(pConnApi, pClient, iClientIndex, &pClient->VoipInfo, 1);
        }
        bDemangling |= pClient->VoipInfo.bDemangling;
    }

    // update tunnel
    if ((pConnApi->bTunnelEnabled != 0) && (pConnApi->pProtoTunnel != NULL))
    {
        ProtoTunnelUpdate(pConnApi->pProtoTunnel);
    }

    return(1);
}


/*F********************************************************************************/
/*!
    \Function _ConnApiIdle

    \Description
        NetConn idle function to update the ConnApi module.

    \Input *pData   - pointer to module state
    \Input uTick    - current tick count

    \Notes
        This function is installed as a NetConn Idle function.  NetConnIdle()
        must be regularly polled for this function to be called.

    \Version 01/06/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiIdle(void *pData, uint32_t uTick)
{
    ConnApiRefT *pConnApi = (ConnApiRefT *)pData;

    if (pConnApi->bAutoUpdate == TRUE)
    {
        ConnApiUpdate(pConnApi);
    }
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ConnApiCreate2

    \Description
        Create the module state.

    \Input iGamePort    - game connection port
    \Input iMaxClients  - maximum number of clients allowed
    \Input *pCallback   - pointer to user callback
    \Input *pUserData   - pointer to user data
    \Input *pConstruct  - comm construct function

    \Output
        ConnApiRefT *   - pointer to module state, or NULL

    \Version 01/04/2005 (jbrookes)
*/
/********************************************************************************F*/
ConnApiRefT *ConnApiCreate2(int32_t iGamePort, int32_t iMaxClients, ConnApiCallbackT *pCallback, void *pUserData, CommAllConstructT *pConstruct)
{
    ConnApiRefT *pConnApi;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    int32_t iSize;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // calculate size of module state
    iSize = sizeof(*pConnApi) + sizeof(ConnApiClientT) * iMaxClients;

    // allocate and init module state
    if ((pConnApi = DirtyMemAlloc(iSize, CONNAPI_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("connapi: could not allocate module state... connapi initialization aborted!\n"));
        return(NULL);
    }
    ds_memclr(pConnApi, iSize);
    pConnApi->iMemGroup = iMemGroup;
    pConnApi->pMemGroupUserData = pMemGroupUserData;

    if ((pConnApi->pVoipGroupRef = VoipGroupCreate()) == NULL)
    {
        // release module memory
        DirtyMemFree(pConnApi, CONNAPI_MEMID, pConnApi->iMemGroup, pConnApi->pMemGroupUserData);

        NetPrintf(("connapi: [0x%08x] no more voip groups available... connapi initialization aborted!\n", pConnApi));
        return(NULL);
    }

    // register connection sharing callback with underlying voip group instance
    VoipGroupSetConnSharingEventCallback(pConnApi->pVoipGroupRef, _ConnApiVoipGroupConnSharingCallback, pConnApi);

    // save info
    pConnApi->uGamePort = (uint16_t)iGamePort;
    pConnApi->uVoipPort = VOIP_PORT;
    pConnApi->ClientList.iMaxClients = iMaxClients;
    pConnApi->pCallback[0] = (pCallback != NULL) ? pCallback :  _ConnApiDefaultCallback;
    pConnApi->pUserData[0] = pUserData;
    pConnApi->pCommConstruct = pConstruct;

    // set default values
    pConnApi->uConnFlags = CONNAPI_CONNFLAG_GAMEVOIP;
    pConnApi->iLinkBufSize = CONNAPI_LINKBUFDEFAULT;
    pConnApi->iConnTimeout = CONNAPI_CONNTIMEOUT_DEFAULT;
    pConnApi->iTimeout = CONNAPI_TIMEOUT_DEFAULT;
    pConnApi->iConfigMnglTimeout = CONNAPI_DEMANGLER_TIMEOUT;
    pConnApi->iConfigMnglTimeoutFailover = CONNAPI_DEMANGLER_WITH_FAILOVER_TIMEOUT;
    pConnApi->iCurrentMnglTimeout = 0;
    pConnApi->iTunnelPort = 3658;
    pConnApi->bDemanglerEnabled = TRUE;
    pConnApi->bUpnpEnabled = TRUE;
    pConnApi->bVoipEnabled = TRUE;
    pConnApi->bVoipRedirectViaHost = FALSE;
    pConnApi->bVoipConnectHost = TRUE;
    pConnApi->bAutoUpdate = TRUE;
#if !defined(DIRTYCODE_XBOXONE)
    pConnApi->bDoAdvertising = TRUE;
#endif

    pConnApi->uGameServConnMode = CONNAPI_CONNFLAG_GAMEVOIP;
    pConnApi->uNetMask = 0xffffffff;

    pConnApi->iQosDuration = 0; // QoS is disabled by default
    pConnApi->iQosInterval = NETGAME_QOS_INTERVAL_MIN;
    pConnApi->iQosPacketSize = NETGAME_QOS_PACKETSIZE_MIN;

    // set default demangler server and create demangler
    ds_strnzcpy(pConnApi->strDemanglerServer, PROTOMANGLE_SERVER, sizeof(pConnApi->strDemanglerServer));

    // add update function to netconn idle handler
    NetConnIdleAdd(_ConnApiIdle, pConnApi);

    // return module state to caller
    return(pConnApi);
}

/*F********************************************************************************/
/*!
    \Function ConnApiOnline

    \Description
        This function should be called once the user has logged on and the input
        parameters are available

    \Input *pConnApi    - pointer to module state
    \Input *pGameName   - pointer to game resource string (eg cso/NCAA-2006/na)
    \Input uSelfId      - unique identifier for the local connapi client

    \Version 01/06/2005 (jbrookes)
*/
/********************************************************************************F*/
void ConnApiOnline(ConnApiRefT *pConnApi, const char *pGameName, uint32_t uSelfId)
{
    char strAdvt[32];

    NetPrintf(("connapi: [0x%08x] ConnApiOnline() invoked with uSelfId=0x%08x and pGameName=%s\n", pConnApi, uSelfId, pGameName));

    // save info
    ds_strnzcpy(pConnApi->strGameName, pGameName, sizeof(pConnApi->strGameName));
    pConnApi->uSelfId = uSelfId;

    // set memory grouping, this requires DirtyMemGroupLeave() to be called before return
    DirtyMemGroupEnter(pConnApi->iMemGroup, pConnApi->pMemGroupUserData);

    // create upnp (first)
    if ((pConnApi->bUpnpEnabled) && (pConnApi->pProtoUpnp == NULL))
    {
        // if upnp ref already exists, use it
        if (ProtoUpnpGetRef() != NULL)
        {
            // "create" module (get ref and increment ref count)
            pConnApi->pProtoUpnp = ProtoUpnpCreate();

            // set default port
            if (pConnApi->bTunnelEnabled)
            {
                ProtoUpnpControl(pConnApi->pProtoUpnp, 'port', pConnApi->iTunnelPort, 0, NULL);
            }
            else
            {
                ProtoUpnpControl(pConnApi->pProtoUpnp, 'port', pConnApi->uGamePort, 0, NULL);
            }

            // set verbose operation
            ProtoUpnpControl(pConnApi->pProtoUpnp, 'spam', 1, 0, NULL);

            // try to add a port mapping
            ProtoUpnpControl(pConnApi->pProtoUpnp, 'macr', 'addp', 0, NULL);
        }
        else
        {
            NetPrintf(("connapi: [0x%08x] warning - protoupnp module not available; UPnP functionality disabled\n", pConnApi));
            pConnApi->bUpnpEnabled = FALSE;
        }
    }

    // create util ref for subnet advertising
    if (pConnApi->bDoAdvertising)
    {
        NetPrintf(("connapi: [0x%08x] creating NetGameUtil ref used for advertising purposes\n", pConnApi));
        if (pConnApi->pGameUtilRef == NULL)
        {
            if ((pConnApi->pGameUtilRef = NetGameUtilCreate()) == NULL)
            {
                NetPrintf(("connapi: [0x%08x] failed to create the NetGameUtil ref used for advertising purposes\n", pConnApi));
                DirtyMemGroupLeave();
                return;
            }
        }
        else
        {
            NetPrintf(("connapi: [0x%08x] can't create the NetGameUtil ref used for advertising purposes because there already exists one\n", pConnApi));
            DirtyMemGroupLeave();
            return;
        }
    }
    else
    {
        NetPrintf(("connapi: [0x%08x] skipped creation of the NetGameUtil ref used for advertising purposes\n", pConnApi));
    }

    // create demangler
    if ((pConnApi->bDemanglerEnabled) && (pConnApi->pProtoMangle == NULL))
    {
#if defined(DIRTYCODE_XBOXONE)
        NetPrintf(("connapi: [0x%08x] creating demangler ref with max clients = %d\n", pConnApi, pConnApi->ClientList.iMaxClients));
        if ((pConnApi->pProtoMangle = ProtoMangleCreate(pConnApi->strDemanglerServer, pConnApi->ClientList.iMaxClients, pConnApi->strGameName, "")) == NULL)
#else
        NetPrintf(("connapi: [0x%08x] creating demangler ref with gamename=%s and server=%s\n", pConnApi, pConnApi->strGameName, pConnApi->strDemanglerServer));
        if ((pConnApi->pProtoMangle = ProtoMangleCreate(pConnApi->strDemanglerServer, PROTOMANGLE_PORT, pConnApi->strGameName, "")) == NULL)
#endif
        {
            NetPrintf(("connapi: [0x%08x] unable to create ProtoMangle module\n", pConnApi));
            pConnApi->bDemanglerEnabled = FALSE;
        }
        else
        {
            // now that we have a valid ProtoMangle, immediately make sure that ConnApi-driven demangler timeout is passed down to it
            ConnApiControl(pConnApi, 'dtim', pConnApi->iConfigMnglTimeout, 0, NULL);
            ConnApiControl(pConnApi, 'dtif', pConnApi->iConfigMnglTimeoutFailover, 0, NULL);
        }
    }

    // create tunnel module
    if ((pConnApi->bTunnelEnabled) && (pConnApi->pProtoTunnel == NULL))
    {
        if ((pConnApi->pProtoTunnel = ProtoTunnelCreate(pConnApi->ClientList.iMaxClients-1, pConnApi->iTunnelPort)) == NULL)
        {
            // unable to create, so disable the tunnel
            pConnApi->bTunnelEnabled = FALSE;
        }
        else
        {
            // we own the tunnel
            pConnApi->bTunnelOwner = TRUE;
        }
    }

    // get VoIP ref
    if ((pConnApi->bVoipEnabled) && (pConnApi->pVoipRef == NULL))
    {
        if ((pConnApi->pVoipRef = VoipGetRef()) == NULL)
        {
            NetPrintf(("connapi: [0x%08x] starting VoIP with iMaxClients=%d\n", pConnApi, pConnApi->ClientList.iMaxClients));
            pConnApi->pVoipRef = VoipStartup(pConnApi->ClientList.iMaxClients, 0);
            pConnApi->bVoipOwner = TRUE;
        }
    }

    // set voip/gamelink timeouts
    ConnApiControl(pConnApi, 'time', pConnApi->iTimeout, 0, NULL);

#if defined(DIRTYCODE_XBOXONE)
    // set external session name and scid (calls into ProtoMangle)
    ConnApiControl(pConnApi, 'exsn', 0, 0, pConnApi->strExternalSessionName);
    ConnApiControl(pConnApi, 'exst', 0, 0, pConnApi->strExternalSessionTemplateName);
    ConnApiControl(pConnApi, 'scid', 0, 0, pConnApi->strScid);
#endif

    ds_snzprintf(strAdvt, sizeof(strAdvt), "%u", pConnApi->uSelfId);

    // start advertising
    if (pConnApi->pGameUtilRef != NULL)
    {
        NetGameUtilAdvert(pConnApi->pGameUtilRef, pConnApi->strGameName, strAdvt, "");
    }

    // leave memory group
    DirtyMemGroupLeave();
}

/*F********************************************************************************/
/*!
    \Function ConnApiDestroy

    \Description
        Destroy the module state.

    \Input *pConnApi    - pointer to module state

    \Version 01/04/2005 (jbrookes)
*/
/********************************************************************************F*/
void ConnApiDestroy(ConnApiRefT *pConnApi)
{
    // if upnp is enabled, delete current mapping
    if (pConnApi->pProtoUpnp != NULL)
    {
        ProtoUpnpControl(pConnApi->pProtoUpnp, 'dprt', 0, 0, NULL);
    }

    // disconnect
    ConnApiDisconnect(pConnApi);

    // remove idle handler
    NetConnIdleDel(_ConnApiIdle, pConnApi);

    VoipGroupDestroy(pConnApi->pVoipGroupRef);

    // destroy voip if we are the owner
    if ((pConnApi->pVoipRef != NULL) && (pConnApi->bVoipOwner == TRUE))
    {
        VoipShutdown(pConnApi->pVoipRef, 0);
    }

    // destroy advertising gameutil ref
    if (pConnApi->pGameUtilRef != NULL)
    {
        NetGameUtilDestroy(pConnApi->pGameUtilRef);
    }

    // destroy tunnel, if present and we are the owner
    if ((pConnApi->pProtoTunnel != NULL) && (pConnApi->bTunnelOwner == TRUE))
    {
        ProtoTunnelDestroy(pConnApi->pProtoTunnel);
    }

    // destroy demangler
    if (pConnApi->pProtoMangle != NULL)
    {
        ProtoMangleDestroy(pConnApi->pProtoMangle);
    }

    // destroy UPnP
    if (pConnApi->pProtoUpnp != NULL)
    {
        ProtoUpnpDestroy(pConnApi->pProtoUpnp);
    }

    // release module memory
    DirtyMemFree(pConnApi, CONNAPI_MEMID, pConnApi->iMemGroup, pConnApi->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function ConnApiConnect

    \Description
        Connect to a game.

    \Input *pConnApi            - pointer to module state
    \Input *pClientList         - list of clients in game session
    \Input iClientListSize      - number of clients in list
    \Input iTopologyHostIndex   - index in the client list who will be serving as topology host
    \Input iPlatformHostIndex   - (ignored)
    \Input iSessId              - unique session identifier
    \Input uGameFlags           - CONNAPI_GAMEFLAG_*

    \Notes
        ConnApi supports invalid entries in the client list. Invalid clients are detected with a DirtyAddr that is zeroed out

    \Version 09/29/2009 (cvienneau)
*/
/********************************************************************************F*/
void ConnApiConnect(ConnApiRefT *pConnApi, ConnApiClientInfoT *pClientList, int32_t iClientListSize, int32_t iTopologyHostIndex, int32_t iPlatformHostIndex, int32_t iSessId, uint32_t uGameFlags)
{
    NetPrintf(("connapi: [0x%08x] ConnApiConnect() called with listSize=%d, topologyHost=%d, sessionId=%d, flags=0x%08x\n", pConnApi, iClientListSize, iTopologyHostIndex, iSessId, uGameFlags));

    // make sure we're idle
    if (pConnApi->eState != ST_IDLE)
    {
        NetPrintf(("connapi: [0x%08x] can't host or connect to a game when not in idle state\n", pConnApi));
        return;
    }

    // save session identifier
    pConnApi->iSessId = iSessId;

    // virtualize ports if tunneling
    if (pConnApi->bTunnelEnabled == TRUE)
    {
        NetConnControl('vadd', pConnApi->uGamePort, 0, NULL, NULL);

        if (pConnApi->bVoipEnabled == TRUE)
        {
            NetConnControl('vadd', pConnApi->uVoipPort, 0, NULL, NULL);
        }
    }

    // init client list
    _ConnApiInitClientList(pConnApi, pClientList, iClientListSize);
    pConnApi->iHostIndex = iTopologyHostIndex;

#if defined(DIRTYCODE_XBOXONE)
    // let protomanglexboxone know what our index is
    ProtoMangleControl(pConnApi->pProtoMangle, 'self', pConnApi->iSelf, 0, NULL);
#endif

    // check for advertisement if necessary
    if (pConnApi->pGameUtilRef != NULL)
    {
        _ConnApiCheckAdvert(pConnApi);
    }

    pConnApi->eState = ST_INGAME;
}

/*F********************************************************************************/
/*!
    \Function ConnApiMigrate

    \Description
        Reopen all connections, using the host specified.
        This is for host migration to a different host in non-peerweb, needing new
        connections for everyone.

    \Input *pConnApi            - pointer to module state
    \Input newTopologyHostIndex - index of the new topology host

    \Version 09/21/2007 (jrainy)
*/
/********************************************************************************F*/
void ConnApiMigrateTopologyHost(ConnApiRefT *pConnApi, int32_t newTopologyHostIndex)
{
    ConnApiClientT *pClient;
    int32_t iClientIndex;
    ConnApiConnStatusE eStatus;

    pConnApi->iHostIndex = newTopologyHostIndex;
    pConnApi->eState = ST_INGAME;

    for (iClientIndex = 0; iClientIndex < pConnApi->ClientList.iMaxClients; iClientIndex++)
    {
        pClient = &pConnApi->ClientList.Clients[iClientIndex];
        if (pClient->bAllocated && (pClient->GameInfo.eStatus != CONNAPI_STATUS_ACTV))
        {
            eStatus = pClient->GameInfo.eStatus;

            #if defined(DIRTYCODE_XBOXONE)
            _ConnApiInitClientConnectionState(pConnApi, pClient, iClientIndex, CONNAPI_CONNFLAG_GAMECONN);
            #else
            pClient->GameInfo.eStatus = CONNAPI_STATUS_INIT;
            #endif

            _ConnApiUpdateCallback(pConnApi, iClientIndex, CONNAPI_CBTYPE_GAMEEVENT, eStatus, (ConnApiConnStatusE)pClient->GameInfo.eStatus);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function ConnApiMigratePlatformHost

    \Description
        Specified client index becomes responsible for managing platform (session owner)

    \Input *pConnApi                - pointer to module state
    \Input iNewPlatformHostIndex    - index of new platform host
    \Input pPlatformData            - platform data

    \Version     09/30/2009 (cvienneau)
*/
/********************************************************************************F*/
void ConnApiMigratePlatformHost(ConnApiRefT *pConnApi, int32_t iNewPlatformHostIndex, void *pPlatformData)
{
}

/*F********************************************************************************/
/*!
    \Function ConnApiAddClient

    \Description
        Add a new client to a pre-existing game at the specified index.

    \Input *pConnApi    - pointer to module state
    \Input *pClientInfo - info on joining user
    \Input iClientIndex - index to add client to

    \Output
        0 if successful, error code otherwise.

    \Notes
        This function should be called by all current members of a game while
        ConnApiConnect() is called by the joining client.

    \Version 06/16/2008 (jbrookes)
*/
/********************************************************************************F*/
int32_t ConnApiAddClient(ConnApiRefT *pConnApi, ConnApiClientInfoT *pClientInfo, int32_t iClientIndex)
{
    ConnApiClientT *pClient;

    // make sure we're not idle
    if (pConnApi->eState == ST_IDLE)
    {
        NetPrintf(("connapi: [0x%08x] can't add a connection to a game in idle state\n", pConnApi));
        return(CONNAPI_ERROR_INVALID_STATE);
    }

    // make sure there is room
    if (pConnApi->ClientList.iNumClients == pConnApi->ClientList.iMaxClients)
    {
        NetPrintf(("connapi: [0x%08x] can't add a connection to the game because it is full\n", pConnApi));
        return(CONNAPI_ERROR_CLIENTLIST_FULL);
    }

    // make sure the selected slot is valid
    if ((iClientIndex < 0) || (iClientIndex >= pConnApi->ClientList.iMaxClients))
    {
        NetPrintf(("connapi: [0x%08x] can't add a connection to the game in slot %d because valid slot range 0-%d\n", pConnApi, iClientIndex, pConnApi->ClientList.iMaxClients-1));
        return(CONNAPI_ERROR_SLOT_OUT_OF_RANGE);
    }

    // get pointer to new client structure to fill in, and increment client count
    pClient = &pConnApi->ClientList.Clients[iClientIndex];

    // check slot and make sure it is uninitialized
    if (pClient->bAllocated == TRUE)
    {
        NetPrintf(("connapi: [0x%08x] slot %d already allocated; cannot add a new client in this slot\n", pConnApi, iClientIndex));
        return(CONNAPI_ERROR_SLOT_USED);
    }

    // add client to list
    _ConnApiInitClient(pConnApi, pClient, pClientInfo, iClientIndex);

    // display client info
    #if DIRTYCODE_LOGGING
    NetPrintf(("connapi: [0x%08x] adding client to clientlist\n", pConnApi));
    _ConnApiDisplayClientInfo(&pConnApi->ClientList.Clients[iClientIndex], iClientIndex);
    #endif

    // increment client count
    pConnApi->ClientList.iNumClients += 1;

    // check for advertisement if necessary
    if (pConnApi->pGameUtilRef != NULL)
    {
        _ConnApiCheckAdvert(pConnApi);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function ConnApiFindClient

    \Description
        Returns the ConnApiClientT of a given client, if found by id.

    \Input *pConnApi    - pointer to module state
    \Input *pClientInfo - info on searched user
    \Input *pOutClient  - used to return the ClientT structure of the client

    \Output
        uint8_t         - TRUE if the client is found, FALSE otherwise

    \Version 06/05/2008 (jbrookes)
*/
/********************************************************************************F*/
uint8_t ConnApiFindClient(ConnApiRefT *pConnApi, ConnApiClientInfoT *pClientInfo, ConnApiClientT *pOutClient)
{
    int32_t iClient;

    for (iClient = 0; iClient < pConnApi->ClientList.iMaxClients; iClient++)
    {
        if (pConnApi->ClientList.Clients[iClient].ClientInfo.uId == pClientInfo->uId)
        {
            ds_memcpy_s(pOutClient, sizeof(*pOutClient), &pConnApi->ClientList.Clients[iClient], sizeof(pConnApi->ClientList.Clients[iClient]));
            return(TRUE);
        }
    }
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function ConnApiRemoveClient

    \Description
        Remove a current client from a game.

    \Input *pConnApi    - pointer to module state
    \Input iClientIndex - index of client to remove (used if pClientName is NULL)

    \Notes
        If this function is called inside of a ConnApi callback, the removal will
        be deferred until the next time NetConnIdle() is called.  Otherwise, the
        removal will happen immediately.

    \Version 06/14/2008 (jbrookes)
*/
/********************************************************************************F*/
void ConnApiRemoveClient(ConnApiRefT *pConnApi, int32_t iClientIndex)
{
    // make sure the select slot is valid
    if ((iClientIndex < 0) || (iClientIndex >= pConnApi->ClientList.iMaxClients))
    {
        NetPrintf(("connapi: [0x%08x] can't remove a connection from the game in slot %d because valid slot range is 0-%d\n", pConnApi, iClientIndex, pConnApi->ClientList.iMaxClients-1));
        return;
    }

    _ConnApiRemoveClientSetup(pConnApi, iClientIndex, CONNAPI_CLIENTFLAG_REMOVE);
}

/*F********************************************************************************/
/*!
    \Function ConnApiDisconnect

    \Description
        Stop game, disconnect from clients, and reset client list.

    \Input *pConnApi    - pointer to module state

    \Notes
        Any NetGameDistRefs created by the application that references a NetGameUtil/
        NeGameLink combination created by ConnApi must destroy the DistRef(s) before
        calling this function.

    \Version 01/04/2005 (jbrookes)
*/
/********************************************************************************F*/
void ConnApiDisconnect(ConnApiRefT *pConnApi)
{
    ConnApiClientT *pClient;
    int32_t iClient;

    NetPrintf(("connapi: [0x%08x] disconnecting\n", pConnApi));

    // make sure we're not idle
    if (pConnApi->eState == ST_IDLE)
    {
        NetPrintf(("connapi: [0x%08x] can't disconnect when in idle state\n", pConnApi));
        return;
    }

    // clear the local user
    if (pConnApi->pVoipRef != NULL && pConnApi->bVoipOwner == TRUE)
    {
        int32_t iUserIndex =  0;        //$$todo do foreach local user?
        VoipGroupControl(pConnApi->pVoipGroupRef, 'lusr', iUserIndex, FALSE, NULL);
    }

    // walk client list
    for (iClient = 0; iClient < pConnApi->ClientList.iMaxClients; iClient++)
    {
        // ref client
        pClient = &pConnApi->ClientList.Clients[iClient];

        // if it's not us and was allocated, disconnect from them
        if ((iClient != pConnApi->iSelf) && pClient->bAllocated)
        {
            _ConnApiDisconnectClient(pConnApi, &pConnApi->ClientList.Clients[iClient], iClient, "disconnect");
        }
    }
    // if in peerconn mode, disconnect from GameServer
    if (pConnApi->uGameServAddr != 0)
    {
        _ConnApiDisconnectClient(pConnApi, &pConnApi->GameServer, -1, "disconnect");
        ds_memclr(&pConnApi->GameServer, sizeof(pConnApi->GameServer));
    }

    // reset client list
    pConnApi->ClientList.iNumClients = 0;
    ds_memclr(&pConnApi->ClientList.Clients, pConnApi->ClientList.iMaxClients * sizeof(ConnApiClientT));

    // devirtualize ports if tunneling is enabled
    if (pConnApi->bTunnelEnabled == TRUE)
    {
        NetConnControl('vdel', pConnApi->uGamePort, 0, NULL, NULL);

        if (pConnApi->bVoipEnabled == TRUE)
        {
            NetConnControl('vdel', pConnApi->uVoipPort, 0, NULL, NULL);
        }
    }

    // clear socket refs
    pConnApi->uTunlSockRef = 0;
    pConnApi->uGameSockRef = 0;
    pConnApi->uVoipSockRef = 0;

    // go to idle state
    pConnApi->eState = ST_IDLE;
}

/*F********************************************************************************/
/*!
    \Function ConnApiGetClientList

    \Description
        Get a list of current connections.

    \Input *pConnApi            - pointer to module state

    \Output
        ConnApiClientListT *    - pointer to client list

    \Version 01/04/2005 (jbrookes)
*/
/********************************************************************************F*/
const ConnApiClientListT *ConnApiGetClientList(ConnApiRefT *pConnApi)
{
    return(&pConnApi->ClientList);
}

/*F********************************************************************************/
/*!
    \Function ConnApiStatus

    \Description
        Get status information.

    \Input *pConnApi    - pointer to module state
    \Input iSelect      - status selector
    \Input *pBuf        - [out] storage for selector-specific output
    \Input iBufSize     - size of output buffer

    \Output
        int32_t         - selector specific

    \Notes
        iSelect can be one of the following:

        \verbatim
            'cbfp' - return current callback function pointer in output buffer
            'cbup' - return current callback data pointer in output buffer
            'dtim' - get effective demangler timeout (in milliseconds)
            'gprt' - return game port
            'gsmd' - return gameserver mode (ConnApiGameServModeE), or -1 if not using GameServer for game connections
            'gsrv' - return the game server ConnApiClientT
            'ingm' - currently 'in game' (connecting to or connected to one or more peers)
            'lbuf' - returns GameLink buffer allocation size
            'minp' - returns GameLink input buffer queue length (zero=default)
            'mngl' - returns whether demangler is enabled or not
            'mout' - returns GameLink output buffer queue length (zero=default)
            'mplr' - returns one past the index of the last allocated player
            'mwid' - returns GameLink max packet size (zero=default)
            'nmsk' - returns current netmask
            'peer' - returns game conn peer-web enable/disable status
            'self' - returns index of local user in client list
            'sock' - copy socket ref to pBuf
            'sess' - copies session information into output buffer
            'tprt' - returns port tunnel has been bound to (if available)
            'tref' - returns the prototunnel ref used
            'tunl' - returns whether tunnel is enabled or not
            'type' - returns current connection type (CONNAPI_CONNFLAG_*)
            'vprt' - return voip port
        \endverbatim

    \Version 01/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ConnApiStatus(ConnApiRefT *pConnApi, int32_t iSelect, void *pBuf, int32_t iBufSize)
{
    return(ConnApiStatus2(pConnApi, iSelect, NULL, pBuf, iBufSize));
}

/*F********************************************************************************/
/*!
    \Function ConnApiStatus2

    \Description
        Get status information.

    \Input *pConnApi    - pointer to module state
    \Input iSelect      - status selector
    \Input *pData       - input data
    \Input *pBuf        - [out] storage for selector-specific output
    \Input iBufSize     - size of output buffer

    \Output
        int32_t         - selector specific

    \Notes
        iSelect can be one of the following:

        \verbatim
            'cadr' - stands for Connection Address, ip address used at the platform socket level to reach this client (regardless of tunneling being used or not)
            'cbfp' - return current callback function pointer in output buffer
            'cbup' - return current callback data pointer in output buffer
            'cprt' - stands for Connection Port, UDP port used at the platform socket level to reach this client
            'dtim' - get effective demangler timeout (in milliseconds)
            'gprt' - return game port
            'gsrv' - return the game server ConnApiClientT
            'host' - returns whether hosting or not, plus copies host name to buffer
            'ingm' - currently 'in game' (connecting to or connected to one or more peers)
            'lbuf' - returns GameLink buffer allocation size
            'minp' - returns GameLink input buffer queue length (zero=default)
            'mngl' - returns whether demangler is enabled or not
            'mout' - returns GameLink output buffer queue length (zero=default)
            'mplr' - returns one past the index of the last allocated player
            'mvtm' - return true if multiple virtual machine mode is active
            'mwid' - returns GameLink max packet size (zero=default)
            'nmsk' - returns current netmask
            'peer' - returns game conn peer-web enable/disable status
            'self' - returns index of local user in client list
            'sock' - copy socket ref to pBuf
            'sess' - copies session information into output buffer
            'tprt' - returns port tunnel has been bound to (if available)
            'tref' - returns the prototunnel ref used
            'tunl' - returns whether tunnel is enabled or not
            'tunr' - returns receive protunnel stats as a ProtoTunnelStatT in pBuf for a given client pData
            'tuns' - returns send prototunnel stats as a ProtoTunnelStatT in pBuf for a given client pData
            'type' - returns current connection type (CONNAPI_CONNFLAG_*)
            'vprt' - return voip port
        \endverbatim

    \Version 01/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ConnApiStatus2(ConnApiRefT *pConnApi, int32_t iSelect, void *pData, void *pBuf, int32_t iBufSize)
{
    if ((iSelect == 'cadr') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(uint32_t)))
    {
        uint8_t bLocalAddr;
        ConnApiClientT *pClientUsed;
        ConnApiClientT *pClient = (ConnApiClientT *)pData;
        struct sockaddr addr;

        *(uint32_t*)pBuf = _ConnApiGetConnectAddr(pConnApi, pClient, &bLocalAddr, CONNAPI_CONNFLAG_GAMECONN, &pClientUsed);

        // if we are using the tunnel return the tunnel address
        if (pConnApi->pProtoTunnel != NULL)
        {
            ProtoTunnelStatus(pConnApi->pProtoTunnel, 'vtop', pClient->iTunnelId, &addr, sizeof(addr));
             *(uint32_t*)pBuf = SockaddrInGetAddr(&addr);
        }

        if (pClientUsed->GameInfo.eStatus != CONNAPI_STATUS_ACTV)
        {
            return(-1);
        }

        return(0);
    }
    if ((iSelect == 'cbfp') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(pConnApi->pCallback[0])))
    {
        ds_memcpy(pBuf, &(pConnApi->pCallback[0]), sizeof(pConnApi->pCallback[0]));
        return(0);
    }
    if ((iSelect == 'cbup') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(pConnApi->pUserData[0])))
    {
        ds_memcpy(pBuf, &(pConnApi->pUserData[0]), sizeof(pConnApi->pUserData[0]));
        return(0);
    }
    if ((iSelect == 'cprt') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(uint16_t)))
    {
        uint8_t bLocalAddr;
        ConnApiClientT *pClientUsed;
        ConnApiClientT *pClient = (ConnApiClientT *)pData;

        _ConnApiGetConnectAddr(pConnApi, pClient, &bLocalAddr, CONNAPI_CONNFLAG_GAMECONN, &pClientUsed);

        if (pClientUsed->GameInfo.eStatus != CONNAPI_STATUS_ACTV)
        {
            return(-1);
        }

        if (pClientUsed->iTunnelId > 0)
        {
            ProtoTunnelStatus(pConnApi->pProtoTunnel, 'rprt', pClientUsed->iTunnelId, pBuf, iBufSize);
        }
        else
        {
            *(uint16_t*)pBuf = pClientUsed->GameInfo.uMnglPort;
        }
        return(0);
    }
    if (iSelect == 'dtim')
    {
        return(pConnApi->iCurrentMnglTimeout);
    }
    if (iSelect == 'gprt')
    {
        return(pConnApi->uGamePort);
    }
    if ((iSelect == 'gsrv') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(ConnApiClientT)))
    {
        ds_memcpy(pBuf, &pConnApi->GameServer, sizeof(ConnApiClientT));
        return(0);
    }
    if (iSelect == 'ingm')
    {
        return(pConnApi->eState != ST_IDLE);
    }
    if (iSelect == 'lbuf')
    {
        return(pConnApi->iLinkBufSize);
    }
    if (iSelect == 'minp')
    {
        return(pConnApi->iGameMinp);
    }
    if (iSelect == 'mngl')
    {
        return(pConnApi->bDemanglerEnabled);
    }
    if (iSelect == 'mout')
    {
        return(pConnApi->iGameMout);
    }
    if (iSelect == 'mplr')
    {
        int32_t iClient;
        int32_t iNumberPlayers = 0;
        for (iClient = 0; iClient < pConnApi->ClientList.iMaxClients; iClient++)
        {
            if (pConnApi->ClientList.Clients[iClient].bAllocated)
            {
                iNumberPlayers = iClient + 1;
            }
        }
        return(iNumberPlayers);
    }
    if (iSelect == 'mwid')
    {
        return(pConnApi->iGameMwid);
    }
    if (iSelect == 'nmsk')
    {
        return(pConnApi->uNetMask);
    }
    if (iSelect == 'peer')
    {
        return(pConnApi->bPeerWeb);
    }
    if (iSelect == 'self')
    {
        return(pConnApi->iSelf);
    }
    if (iSelect == 'sess')
    {
        ds_strnzcpy((char *)pBuf, pConnApi->strSession, iBufSize);
        return(0);
    }
    if (iSelect == 'sock')
    {
        if (iBufSize >= (signed)sizeof(intptr_t))
        {
            if (pConnApi->bTunnelEnabled)
            {
                ProtoTunnelStatus(pConnApi->pProtoTunnel, 'sock', 0, pBuf, iBufSize);
            }
            else if (pConnApi->ClientList.Clients[pConnApi->iHostIndex].pGameUtilRef)
            {
                NetGameUtilStatus(pConnApi->ClientList.Clients[pConnApi->iHostIndex].pGameUtilRef, 'sock', pBuf, iBufSize);
            }
            else
            {
                NetPrintf(("connapi: [0x%08x] ConnApiStatus('sock') failed because game socket not yet available (tunnel enabled = %s)\n", pConnApi,
                    pConnApi->bTunnelEnabled?"true":"false"));
                return(-2);
            }
            return(0);
        }
        else
        {
            NetPrintf(("connapi: [0x%08x] ConnApiStatus('sock') failed because size (%d) of user-provided buffer is too small (required: %d)\n", pConnApi,
                iBufSize, sizeof(intptr_t)));
            return(-1);
        }
    }
    if ((iSelect == 'tprt') && (pConnApi->pProtoTunnel != NULL))
    {
        return(ProtoTunnelStatus(pConnApi->pProtoTunnel, 'lprt', 0, NULL, 0));
    }
    if (iSelect == 'tref' && (pConnApi->pProtoTunnel != NULL))
    {
        if (iBufSize >= (signed)sizeof(ProtoTunnelRefT*))
        {
            ds_memcpy(pBuf, &pConnApi->pProtoTunnel, sizeof(ProtoTunnelRefT*));
            return(0);
        }
        else
        {
            NetPrintf(("connapi: [0x%08x] ConnApiStatus('tref') failed because size (%d) of user-provided buffer is too small (required: %d)\n", pConnApi,
                iBufSize, sizeof(ProtoTunnelRefT*)));
            return(-1);
        }
    }
    if (iSelect == 'tunl')
    {
        return(pConnApi->bTunnelEnabled);
    }
    if (iSelect == 'tunr')
    {
        if ((pData != NULL) && (pBuf != NULL) && (iBufSize == sizeof(ProtoTunnelStatT)))
        {
            ConnApiClientT *pClient = (ConnApiClientT *)pData;
            ProtoTunnelStatus(pConnApi->pProtoTunnel, 'rcvs', pClient->iTunnelId, pBuf, iBufSize);
            return(0);
        }
        else
        {
            NetPrintf(("connapi: [0x%08x] ConnApiStatus('tunr') failed due to invalid arguments\n", pConnApi));
            return(-1);
        }
    }
    if (iSelect == 'tuns')
    {
        if ((pData != NULL) && (pBuf != NULL) && (iBufSize == sizeof(ProtoTunnelStatT)))
        {
            ConnApiClientT *pClient = (ConnApiClientT *)pData;
            ProtoTunnelStatus(pConnApi->pProtoTunnel, 'snds', pClient->iTunnelId, pBuf, iBufSize);
            return(0);
        }
        else
        {
            NetPrintf(("connapi: [0x%08x] ConnApiStatus('tuns') failed due to invalid arguments\n", pConnApi));
            return(-1);
        }
    }
    if (iSelect == 'type')
    {
        return(pConnApi->uConnFlags);
    }
    if (iSelect == 'ulmt')
    {
        return(pConnApi->iGameUnackLimit);
    }
    if (iSelect == 'vprt')
    {
        return(pConnApi->uVoipPort);
    }
    // unhandled
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ConnApiControl

    \Description
        Control behavior of module.

    \Input *pConnApi    - pointer to module state
    \Input iControl     - status selector
    \Input iValue       - control value
    \Input iValue2      - control value
    \Input *pValue      - control value

    \Output
        int32_t             - selector specific

    \Notes
        iControl can be one of the following:

        \verbatim
            'adve' - set to enable ProtoAdvt advertising
            'auto' - set auto-update enable/disable - iValue=TRUE or FALSE (default TRUE)
            'cbfp' - set callback function pointer - pValue=pCallback
            'cbup' - set callback user data pointer - pValue=pUserData
            'ccmd' - set the CC mode  (CONNAPI_CCMODE_*)
            'ctim' - set connection timeout - iValue=timeout (minimum & default 10000 ms)
            'dist' - set dist ref - iValue=index of client or 'gsrv' for GameServer user, pValue=dist ref
            'dsrv' - set demangler server - pValue=pointer to demangler server name (default demangler.ea.com)
            'dtif' - set demangler timeout for scenarios involving or CC assistance - iValue=timeout in milliseconds
            'dtim' - set demangler timeout - iValue=timeout in milliseconds
            'estv' - enable flag to establish voip for a client after it was delayed, iValue=client index
            'gprt' - set game port to use - iValue=port
            'gsid' - set gameserver id - iValue=id
            'gsrv' - set address of gameserver - iValue=address
            'gsv2' - set Game Server Connection Mode - iValue=mode
            'lbuf' - set game link buffer size - iValue=size (default 1024)
            'meta' - enable/disable commudp metadata
            'minp' - set GameLink input buffer queue length - iValue=length (default 32)
            'mngl' - set demangler enable/disable - iValue=TRUE/FALSE (default TRUE)
            'mout' - set GameLink output buffer queue length - iValue=length (default 32)
            'mvtm' - set multiple virtual machine mode enable/disable - iValue=TRUE/FALSE (default FALSE)
            'mwid' - set GameLink max packet size - iValue=size (default 240, max 512)
            'nmsk' - set netmask used for external address comparisons - iValue=mask (default 0xffffffff)
            'peer' - set game conn peer-web enable/disable - iValue=TRUE/FALSE (default FALSE)
            'phcc' - enable/disable PcHostXboxOneClient mode
            'rcbk' - set enable of disc callback on removal - iValue=TRUE/FALSE (default FALSE)
            'stun' - set prototunnel ref
            'tctl' - set prototunnel control data
            'time' - set timeout - iValue=timeout in ms (default 15 seconds)
            'tgam' - enable the override of game tunnel flags - iValue=falgs, iValue2=boolean(overrides or not)
            'tunl' - set tunnel parms:
                         iValue = TRUE/FALSE to enable/disable or negative to ignore
                         iValue2 = tunnel port, or negative to ignore
                         pValue = tunnel crypto key, or NULL to ignore
            'type' - set connection type - iValue = CONNAPI_CONNFLAG_*
            'upnp' - set UPnP enable - iValue = TRUE to enable, FALSE to disable
            'vcon' - set the conapi to voip only with no game
            'vhos' - set the connapi to attempt a voip connection with the hpost or not
            'voig' - pass-through to VoipGroupControl()
            'voip' - pass-through to VoipControl() - iValue=VoIP iControl, iValue2= VoIP iValue
            'vprt' - set voip port to use - iValue=port
            'vset' - set voip enable/disable (default=TRUE; call before ConnApiOnline())
            'exsn' - set the external session name for the MultiplayerSessionReference (xbox one only, not yet implemented)
            'exst' - set the external session template name for the MultiplayerSessionReference (xbox one only, not yet implemented)
            'scid' - set the service configuration id for the MultiplayerSessionReference (xbox one only, not yet implemented)
            'sqos' - set QoS settings used when creating NetGameLinks. iValue=QoS duration (0 disables QoS), iValue2=QoS packet interval
            'lqos' - set QoS packet size used when creating NetGameLinks. iValue=packet size in bytes
            '!res' - force secure address resolution to fail, to simulate p2p connection failure.
                         iValue  = TRUE/FALSE to enable/disable
                         iValue2 = index of user to force fail or -1 for all users or local index for all users
        \endverbatim

    \Version 01/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ConnApiControl(ConnApiRefT *pConnApi, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iControl == 'adve')
    {
#if defined(DIRTYCODE_XBOXONE)
        NetPrintf(("connapi: ProtoAdvt advertising cannot be enabled on Xbox One\n"));
#else
        NetPrintf(("connapi: [0x%08x] ProtoAdvt advertising %s\n", pConnApi, (iValue?"enabled":"disabled")));
        pConnApi->bDoAdvertising = iValue;
        // if disabling advertising and we already have an advertising ref, kill it
        if ((pConnApi->bDoAdvertising == FALSE) && (pConnApi->pGameUtilRef != NULL))
        {
            NetGameUtilDestroy(pConnApi->pGameUtilRef);
            pConnApi->pGameUtilRef = NULL;
        }
#endif
        return(0);
    }
    if (iControl == 'auto')
    {
        pConnApi->bAutoUpdate = iValue;
        return(0);
    }
    if (iControl == 'ccmd')
    {
        #if DIRTYCODE_LOGGING
        static const char *_ConnApiCcModeNames[] =
        {
            "CONNAPI_CCMODE_PEERONLY",
            "CONNAPI_CCMODE_HOSTEDONLY",
            "CONNAPI_CCMODE_HOSTEDFALLBACK"
        };
        #endif

        switch (iValue)
        {
            case (CONNAPI_CCMODE_PEERONLY):
                VoipGroupControl(pConnApi->pVoipGroupRef, 'ccmd', VOIPGROUP_CCMODE_PEERONLY, 0, NULL);
                break;
            case (CONNAPI_CCMODE_HOSTEDONLY):
                VoipGroupControl(pConnApi->pVoipGroupRef, 'ccmd', VOIPGROUP_CCMODE_HOSTEDONLY, 0, NULL);
                break;
            case (CONNAPI_CCMODE_HOSTEDFALLBACK):
                VoipGroupControl(pConnApi->pVoipGroupRef, 'ccmd', VOIPGROUP_CCMODE_HOSTEDFALLBACK, 0, NULL);
                break;
            default:
                NetPrintf(("connapi: [0x%08x] unsupported CC mode %d\n", pConnApi, iValue));
                return(-1);
        }
        NetPrintf(("connapi: [0x%08x] CC mode = %d (%s)\n", pConnApi, iValue, _ConnApiCcModeNames[iValue]));
        pConnApi->iCcMode = iValue;
        return(0);
    }
    if (iControl == 'cbfp')
    {
        // set callback function pointer
        pConnApi->pCallback[0] = ((pValue != NULL) ? (ConnApiCallbackT *)pValue : _ConnApiDefaultCallback);
        return(0);
    }
    if (iControl == 'cbup')
    {
        // set callback user data pointer
        pConnApi->pUserData[0] = pValue;
        return(0);
    }
    if ((iControl == 'ctim') && (iValue >= CONNAPI_CONNTIMEOUT_DEFAULT))
    {
        NetPrintf(("connapi: [0x%08x] setting connection timeout to %d\n", pConnApi, iValue));
        pConnApi->iConnTimeout = iValue;
        return(0);
    }
    if (iControl == 'dist')
    {
        // set dist ref for specified client
        if (iValue == 'gsrv')
        {
            pConnApi->GameServer.pGameDistRef = (NetGameDistRefT *)pValue;
            return(0);
        }
        else if (iValue < pConnApi->ClientList.iMaxClients)
        {
            pConnApi->ClientList.Clients[iValue].pGameDistRef = (NetGameDistRefT *)pValue;
            return(0);
        }
    }
    if (iControl == 'dsrv')
    {
        // set demangler server
        ds_strnzcpy(pConnApi->strDemanglerServer, (const char *)pValue, sizeof(pConnApi->strDemanglerServer));
        return(0);
    }
    if ((iControl == 'dtim') || (iControl == 'dtif'))
    {
        // set demangler timeout
        int32_t iNewMnglTimeout;
        #if DIRTYCODE_LOGGING
        uint8_t bIsFailoverPossible = FALSE;
        #endif

        // special case to allow using a different timeout value for connapi involving CC assistance
        if (iControl == 'dtif')
        {
            NetPrintf(("connapi: [0x%08x] changing demangler_with_failover timeout config (%d ms --> %d ms)\n", pConnApi, pConnApi->iConfigMnglTimeoutFailover, iValue));
            pConnApi->iConfigMnglTimeoutFailover = iValue;
        }
        else
        {
            NetPrintf(("connapi: [0x%08x] changing demangler timeout config (%d ms --> %d ms)\n", pConnApi, pConnApi->iConfigMnglTimeout, iValue));
            pConnApi->iConfigMnglTimeout = iValue;
        }

        // check if the special timeout value for cc scenarios is applicable
        if (pConnApi->iCcMode != CONNAPI_CCMODE_PEERONLY)
        {
            #if DIRTYCODE_LOGGING
            bIsFailoverPossible = TRUE;
            #endif
            iNewMnglTimeout = pConnApi->iConfigMnglTimeoutFailover;
        }
        else
        {
            iNewMnglTimeout = pConnApi->iConfigMnglTimeout;
        }

        // pass new effective demangler timeout value down to ProtoMangle
        if ((pConnApi->pProtoMangle != NULL) && (iNewMnglTimeout != pConnApi->iCurrentMnglTimeout))
        {
            NetPrintf(("connapi: [0x%08x] applying new demangler timeout (%d ms --> %d ms) for a demangling scenario %s\n",
                pConnApi, pConnApi->iCurrentMnglTimeout, iNewMnglTimeout, (bIsFailoverPossible?"with":"without")));
            pConnApi->iCurrentMnglTimeout = iNewMnglTimeout;
            ProtoMangleControl(pConnApi->pProtoMangle, 'time', pConnApi->iCurrentMnglTimeout, 0, NULL);
        }

        return(0);
    }
    if (iControl == 'gprt')
    {
        NetPrintf(("connapi: [0x%08x] using game port %d\n", pConnApi, iValue));
        pConnApi->uGamePort = (uint16_t)iValue;
        return(0);
    }
    if (iControl == 'gsid')
    {
        NetPrintf(("connapi: [0x%08x] setting gameserver id=%d rid=%d lid=%d\n", pConnApi, iValue, iValue, iValue2));
        pConnApi->GameServer.ClientInfo.uId = (uint32_t)iValue;
        pConnApi->GameServer.ClientInfo.uRemoteClientId = (uint32_t)iValue;
        pConnApi->GameServer.ClientInfo.uLocalClientId = (uint32_t)iValue2;
        return(0);
    }
    if (iControl == 'gsrv')
    {
        if (pValue == NULL)
        {
            pValue = (void *)"GameServer";
        }
        NetPrintf(("connapi: [0x%08x] setting gameserver=%s/%a", pConnApi, pValue, iValue));
        pConnApi->uGameServAddr = iValue;
        ds_strnzcpy(pConnApi->strGameServName, pValue, sizeof(pConnApi->strGameServName));
        VoipGroupControl(pConnApi->pVoipGroupRef, 'serv', (iValue != 0) && (pConnApi->uGameServConnMode & CONNAPI_CONNFLAG_VOIPCONN), 0, NULL);
        return(0);
    }
    if (iControl == 'gsv2')
    {
        NetPrintf(("connapi: [0x%08x] setting gameserver conn mode=%d\n", pConnApi, iValue));
        pConnApi->uGameServConnMode = iValue;
        return(0);
    }
    if (iControl == 'lbuf')
    {
        // set game link buffer size
        pConnApi->iLinkBufSize = iValue;
        return(0);
    }
    if (iControl == 'meta')
    {
        // enable/disable commudp metadata
        NetPrintf(("connapi: [0x%08x] commudp metadata %s\n", pConnApi, iValue ? "enabled" : "disabled"));
        pConnApi->bCommUdpMetadata = iValue;
        return(0);
    }
    if (iControl == 'minp')
    {
        // set gamelink input packet queue length
        pConnApi->iGameMinp = iValue;
        return(0);
    }
    if (iControl == 'mngl')
    {
#if defined(DIRTYCODE_XBOXONE)
        NetPrintf(("connapi: [0x%08x] demangler cannot be disabled on Xbox One\n", pConnApi));
#else
        // set demangler enable/disable
        NetPrintf(("connapi: [0x%08x] demangling %s\n", pConnApi, iValue ? "enabled" : "disabled"));
        pConnApi->bDemanglerEnabled = iValue;
#endif
        return(0);
    }
    if (iControl == 'mout')
    {
        // set gamelink output packet queue length
        pConnApi->iGameMout = iValue;
        return(0);
    }
    if (iControl == 'mwid')
    {
        // set gamelink packet length
        pConnApi->iGameMwid = iValue;
        return(0);
    }
    if (iControl == 'nmsk')
    {
        // set netmask
        pConnApi->uNetMask = (unsigned)iValue;
        return(0);
    }
    if (iControl == 'peer')
    {
        // set peer-web status
        NetPrintf(("connapi: [0x%08x] peerweb %s\n", pConnApi, iValue ? "enabled" : "disabled"));
        pConnApi->bPeerWeb = iValue;
        return(0);
    }
#if defined(DIRTYCODE_XBOXONE)
    if (iControl == 'phcc')
    {
        NetPrintf(("connapi: [0x%08x] 'phcc' PC Host, Xbox One client mode set: %d\n", pConnApi, iValue));
        pConnApi->bPcHostXboxOneClientMode = iValue;
        return(0);
    }
    if (iControl == 'exsn')
    {
        if ((pValue == NULL) || (*(char*)pValue == '\0'))
        {
            NetPrintf(("connapi: [0x%08x] 'exsn', invalid external session name\n", pConnApi));
        }
        else
        {
            if (pConnApi->strExternalSessionName != pValue)
            {
                ds_strnzcpy(pConnApi->strExternalSessionName, (char*)pValue, sizeof(pConnApi->strExternalSessionName));
                NetPrintf(("connapi: [0x%08x] 'exsn', external session name saved as (%s)\n", pConnApi, pConnApi->strExternalSessionName));
            }
            if (pConnApi->pProtoMangle != NULL)
            {
                return(ProtoMangleControl(pConnApi->pProtoMangle, 'exsn', 0, 0, pConnApi->strExternalSessionName));
            }
            return(0);
        }
    }
    if (iControl == 'exst')
    {
        if ((pValue == NULL) || (*(char*)pValue == '\0'))
        {
            NetPrintf(("connapi: [0x%08x] 'exst', invalid external session template name\n", pConnApi));
        }
        else
        {
            if (pConnApi->strExternalSessionTemplateName != pValue)
            {
                ds_strnzcpy(pConnApi->strExternalSessionTemplateName, (char*)pValue, sizeof(pConnApi->strExternalSessionTemplateName));
                NetPrintf(("connapi: [0x%08x] 'exst', external session template name saved as (%s)\n", pConnApi, pConnApi->strExternalSessionTemplateName));
            }
            if (pConnApi->pProtoMangle != NULL)
            {
                return(ProtoMangleControl(pConnApi->pProtoMangle, 'exst', 0, 0, pConnApi->strExternalSessionTemplateName));
            }
            return(0);
        }
    }
    if (iControl == 'scid')
    {
        if ((pValue == NULL) || (*(char*)pValue == '\0'))
        {
            NetPrintf(("connapi: [0x%08x] 'scid', invalid service configuration id\n", pConnApi));
        }
        else
        {
            if (pConnApi->strScid != pValue)
            {
                ds_strnzcpy(pConnApi->strScid, (char*)pValue, sizeof(pConnApi->strScid));
                NetPrintf(("connapi: [0x%08x] 'scid', service configuration name saved as (%s)\n", pConnApi, pConnApi->strScid));
            }
            if (pConnApi->pProtoMangle != NULL)
            {
                return(ProtoMangleControl(pConnApi->pProtoMangle, 'scid', 0, 0, pConnApi->strScid));
            }
            return(0);
        }
    }
#endif
    if (iControl == 'phxc')
    {
        //$$DEPRECATE: but for now, don't return an error
        return(0);
    }
    if (iControl == 'rcbk')
    {
        // enable disc callback on removal
        pConnApi->bRemoveCallback = iValue;
        return(0);
    }
    if ((iControl == 'stun') && (pConnApi->pProtoTunnel == NULL) && (pValue != NULL))
    {
        // set prototunnel ref
        pConnApi->pProtoTunnel = (ProtoTunnelRefT *)pValue;
        return(0);
    }
    if (iControl == 'time')
    {
        // set timeout
        pConnApi->iTimeout = iValue;
        VoipGroupControl(pConnApi->pVoipGroupRef, 'time', iValue, 0, NULL);
        return(0);
    }
    if ((iControl == 'tctl') && (pConnApi->pProtoTunnel != NULL))
    {
        return(ProtoTunnelControl(pConnApi->pProtoTunnel, iValue, iValue2, 0, pValue));
    }
    if (iControl == 'tgam')
    {
        pConnApi->uGameTunnelFlag = iValue;
        pConnApi->uGameTunnelFlagOverride = iValue2;
    }
    if (iControl == 'tunl')
    {
        // set tunnel status
        if (iValue >= 0)
        {
            pConnApi->bTunnelEnabled = iValue;
            VoipGroupControl(pConnApi->pVoipGroupRef, 'tunl', iValue, 0, NULL);
        }
        if (iValue2 > 0)
        {
            pConnApi->iTunnelPort = iValue2;
        }
        if (pValue != NULL)
        {
            ds_strnzcpy(pConnApi->GameServer.ClientInfo.strTunnelKey, pValue, sizeof(pConnApi->GameServer.ClientInfo.strTunnelKey));
        }
        return(0);
    }
    if (iControl == 'type')
    {
        // set connection flags (CONNAPI_CONNFLAG_*)
        NetPrintf(("connapi: [0x%08x] connflag change from 0x%02x to 0x%02x\n", pConnApi, pConnApi->uConnFlags, iValue));
        pConnApi->uConnFlags = (uint16_t)iValue;
        return(0);
    }
    if (iControl == 'ulmt')
    {
        // set gamelink unack window size
        pConnApi->iGameUnackLimit = iValue;
        return(0);
    }
    if (iControl == 'upnp')
    {
        // set demangler enable/disable
        pConnApi->bUpnpEnabled = iValue;
        return(0);
    }
    if (iControl == 'vcon')
    {
        pConnApi->uConnFlags &= ~CONNAPI_CONNFLAG_GAMECONN;
        pConnApi->uConnFlags |= CONNAPI_CONNFLAG_VOIPCONN;
    }
    if (iControl == 'vhos')
    {
        pConnApi->bVoipConnectHost = (uint8_t) iValue;
        return(0);
    }
    if (iControl == 'voig')
    {
        VoipGroupControl(pConnApi->pVoipGroupRef, iValue, iValue2, 0, pValue);
        return(0);
    }
    if (iControl == 'voip')
    {
        if(pConnApi->pVoipRef != NULL)
        {
            VoipControl(pConnApi->pVoipRef, iValue, iValue2, pValue);
            return(0);
        }

        NetPrintf(("connapi: [0x%08x] - WARNING - ConnApiControl(): processing of 'voip' selector failed because of an uninitialized VOIP module reference!\n", pConnApi));
    }
    if (iControl == 'vprt')
    {
        NetPrintf(("connapi: [0x%08x] using voip port %d\n", pConnApi, iValue));
        pConnApi->uVoipPort = (uint16_t)iValue;
        return(0);
    }
    if (iControl == 'vset')
    {
        // if disabling VoIP, set game flags appropriately
        if ((pConnApi->bVoipEnabled = iValue) == FALSE)
        {
            pConnApi->uConnFlags &= ~CONNAPI_CONNFLAG_GAMEVOIP;
            pConnApi->uConnFlags |= CONNAPI_CONNFLAG_GAMECONN;
        }
        return(0);
    }
    if (iControl == 'vsrv')
    {
        NetPrintf(("connapi: [0x%08x] voip connections for player made via host slot = %d\n", pConnApi, iValue));
        pConnApi->bVoipRedirectViaHost = (uint8_t)iValue;
        return(0);
    }
    #if DIRTYCODE_DEBUG
    if (iControl == '!res')
    {
        ConnApiClientT *pClient;
        if (iValue2 >= 0 && iValue2 < pConnApi->ClientList.iMaxClients)
        {
            pClient = &pConnApi->ClientList.Clients[iValue2];
            if (pClient->bAllocated == TRUE)
            {
                if (iValue > 0)
                {
                    NetPrintf(("connapi: [0x%08x] setting debug FailP2P flag for user at index %d\n", pConnApi, iValue2));
                    pConnApi->ClientList.Clients[iValue2].uFlags |= CONNAPI_CLIENTFLAG_P2PFAILDBG;
                    if (iValue2 == pConnApi->iSelf)
                    {
                        NetPrintf(("connapi: [0x%08x] setting debug FailP2P flag with our own index, disabling P2P for all users for all users\n", pConnApi));
                    }
                }
                else
                {
                    NetPrintf(("connapi: [0x%08x] resetting debug FailP2P flag for user at index %d\n", pConnApi, iValue2));
                    pConnApi->ClientList.Clients[iValue2].uFlags &= ~CONNAPI_CLIENTFLAG_P2PFAILDBG;
                }
            }
            else
            {
                NetPrintf(("connapi: [0x%08x] failed to set debug FailP2P flag for user at index %d\n", pConnApi, iValue2));
                return(-1);
            }
        }
        else if (iValue2 < 0)
        {
            pConnApi->bFailP2PConnect = iValue;
            NetPrintf(("connapi: [0x%08x] setting debug FailP2P flag to %d for all users\n", pConnApi, iValue));
        }
        else
        {
            NetPrintf(("connapi: [0x%08x] cannot set debug FailP2P flag to %d for user at index %d\n", pConnApi, iValue, iValue2));
        }

        return(0);
    }
    #endif
    if (iControl == 'sqos')
    {
        if ((iValue < NETGAME_QOS_DURATION_MIN) || (iValue > NETGAME_QOS_DURATION_MAX))
        {
            NetPrintf(("connapi: [0x%08x] Invalid QoS duration period provided (%d). QoS duration must be >= %d and <= %s ms\n", pConnApi, iValue, NETGAME_QOS_DURATION_MIN, NETGAME_QOS_DURATION_MAX));
            iValue = (iValue < NETGAME_QOS_DURATION_MIN ? NETGAME_QOS_DURATION_MIN : iValue);
            iValue = (iValue > NETGAME_QOS_DURATION_MAX ? NETGAME_QOS_DURATION_MAX : iValue);
        }
        if ((iValue != 0) && ((iValue2 < NETGAME_QOS_INTERVAL_MIN) || (iValue2 > iValue)))
        {
            NetPrintf(("connapi: [0x%08x] Invalid QoS interval provided (%d). QoS interval must be >= %d and <= QoS duration period\n", pConnApi, iValue2, NETGAME_QOS_INTERVAL_MIN));
            iValue2 = (iValue2 < NETGAME_QOS_INTERVAL_MIN ? NETGAME_QOS_INTERVAL_MIN : iValue2);
            iValue2 = (iValue2 > iValue ? iValue : iValue2);
        }
        if (iValue == 0)
        {
            NetPrintf(("connapi: [0x%08x] QoS for NetGameLink connections is disabled\n", pConnApi));
        }
        NetPrintf(("connapi: [0x%08x] QoS duration period set to %d ms\n", pConnApi, iValue));
        NetPrintf(("connapi: [0x%08x] QoS interval set to %d ms\n", pConnApi, iValue2));

        pConnApi->iQosDuration = iValue;
        pConnApi->iQosInterval = iValue2;
        return(0);
    }
    if (iControl == 'lqos')
    {
        if ((iValue < NETGAME_QOS_PACKETSIZE_MIN) || (iValue > NETGAME_DATAPKT_MAXSIZE))
        {
            NetPrintf(("connapi: [0x%08x] Invalid QoS packet size specified (%d). QoS packets must be >= %d and <= %d\n", pConnApi, iValue, NETGAME_QOS_PACKETSIZE_MIN, NETGAME_DATAPKT_MAXSIZE));
            iValue = (iValue < NETGAME_QOS_PACKETSIZE_MIN ? NETGAME_QOS_PACKETSIZE_MIN : iValue);
            iValue = (iValue > NETGAME_DATAPKT_MAXSIZE ? NETGAME_DATAPKT_MAXSIZE : iValue);
        }
        NetPrintf(("connapi: [0x%08x] QoS packet size set to %d\n", pConnApi, iValue));
        pConnApi->iQosPacketSize = iValue;
        return(0);
    }
    if (iControl == 'estv')
    {
        ConnApiClientT *pClient;
        if (iValue < pConnApi->ClientList.iMaxClients)
        {
            pClient = &pConnApi->ClientList.Clients[iValue];
            if (pClient->bAllocated == TRUE)
            {
                // no-op if it is already established
                // in the case of the local user, he will always have bEstablishVoip == TRUE so let's not spam with extra logs about it
                if (pClient->bEstablishVoip == FALSE)
                {
                    NetPrintf(("connapi: [0x%08x] Activating delayed voip for client %d\n", pConnApi, iValue));
                    pClient->bEstablishVoip = TRUE;
                    return(0);
                }
            }
            else
            {
                NetPrintf(("connapi: [0x%08x] Activating delayed voip failed because client %d not allocated\n", pConnApi, iValue));
                return(-1);
            }
        }
        else
        {
            NetPrintf(("connapi: [0x%08x] Activating delayed voip failed because of client index of range\n", pConnApi));
            return(-1);
        }
    }

    // unhandled
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ConnApiUpdate

    \Description
        Update the ConnApi module (must be called directly if auto-update is disabled)

    \Input *pConnApi    - pointer to module state

    \Notes
        By default, ConnApiUpdate() is called internally via a NetConnIdle() callback
        (auto-update).  If auto-update is disabled via ConnApiControl('auto'),
        ConnApiUpdate must be polled by the application instead.

    \Version 01/06/2005 (jbrookes)
*/
/********************************************************************************F*/
void ConnApiUpdate(ConnApiRefT *pConnApi)
{
    uint8_t bGameServEnabled;

    // update client flags
    _ConnApiUpdateClientFlags(pConnApi);

    // check if gameserver enabled
    bGameServEnabled = _ConnApiGameServerEnabled(pConnApi, NULL, CONNAPI_CONNFLAG_GAMECONN);

    // update connapi connections
    if (!bGameServEnabled)
    {
        _ConnApiUpdateConnections(pConnApi);
    }
    else
    {
        _ConnApiUpdateGameServConnection(pConnApi);
    }

    #if DIRTYCODE_XBOXONE
    /*
       update protomangle outside the demangling phase context
       required for protomangle to be pumped after a call to ProtoMangleControl('remv')
    */
    ProtoMangleUpdate(pConnApi->pProtoMangle);
    #endif

    // handle removal of clients from client list, if requested
    _ConnApiUpdateRemoval(pConnApi);
}

/*F********************************************************************************/
/*!
    \Function ConnApiAddCallback

    \Description
        Register a new callback

    \Input *pConnApi    - pointer to module state
    \Input *pCallback   - the callback to add
    \Input *pUserData   - the user data that will be passed back

    \Output
        int32_t         - negative means error. 0 or greater: slot used

    \Version 09/18/2008 (jrainy)
*/
/********************************************************************************F*/
int32_t ConnApiAddCallback(ConnApiRefT *pConnApi, ConnApiCallbackT *pCallback, void *pUserData)
{
    int32_t iIndex;

    // skip the first (0th, which is reserved for 'cbfp' and 'cbup' backward compatibility.
    for(iIndex = 1; iIndex < CONNAPI_MAX_CALLBACKS; iIndex++)
    {
        if (pConnApi->pCallback[iIndex] == NULL)
        {
            pConnApi->pCallback[iIndex] = pCallback;
            pConnApi->pUserData[iIndex] = pUserData;

            return(iIndex);
        }
    }
    return(CONNAPI_CALLBACKS_FULL);
}

/*F********************************************************************************/
/*!
    \Function ConnApiRemoveCallback

    \Description
        Unregister a callback

    \Input *pConnApi    - pointer to module state
    \Input *pCallback   - the callback to remove
    \Input *pUserData   - the user data that was originally passed in

    \Output
        int32_t         - negative means error. 0 or greater: slot freed

    \Version 09/18/2008 (jrainy)
*/
/********************************************************************************F*/
int32_t ConnApiRemoveCallback(ConnApiRefT *pConnApi, ConnApiCallbackT *pCallback, void *pUserData)
{
    int32_t iIndex;

    // skip the first (0th, which is reserved for 'cbfp' and 'cbup' backward compatibility.
    for(iIndex = 1; iIndex < CONNAPI_MAX_CALLBACKS; iIndex++)
    {
        if ((pConnApi->pCallback[iIndex] == pCallback) && (pConnApi->pUserData[iIndex] == pUserData))
        {
            pConnApi->pCallback[iIndex] = NULL;
            pConnApi->pUserData[iIndex] = NULL;
            return(iIndex);
        }
    }
    return(CONNAPI_CALLBACK_NOT_FOUND);
}
