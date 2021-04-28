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

#include "dirtysock.h"
#include "dirtynames.h"
#include "dirtymem.h"
#include "protomangle.h"
#include "prototunnel.h"
#include "protoupnp.h"
#include "netconn.h"
#include "voip.h"
#include "voipgroup.h"

#include "connapi.h"

/*** Defines **********************************************************************/

//! default connapi timeout
#define CONNAPI_TIMEOUT_DEFAULT     (15*1000)

//! connapi connection timeout
#define CONNAPI_CONNTIMEOUT_DEFAULT (10*1000)

//! connapi default demangler timeout (per user)
#define CONNAPI_DEMANGLER_TIMEOUT   (15*1000)

//! connapi maximum (total) demangler timeout
#define CONNAPI_DEMANGLER_MAXTIMEOUT (60*1000)

//! default GameLink buffer size
#define CONNAPI_LINKBUFDEFAULT      (1024)

//! test demangling
#define CONNAPI_DEMANGLE_TEST       (DIRTYCODE_DEBUG && FALSE)

//! random demangle local port?
#define CONNAPI_RNDLCLDEMANGLEPORT  (DIRTYCODE_PLATFORM == DIRTYCODE_REVOLUTION)

//! max number of registered callbacks
#define CONNAPI_MAX_CALLBACKS       (8)

//! connapi client flags
#define CONNAPI_CLIENTFLAG_REMOVE               (1) // remove client from clientlist
#define CONNAPI_CLIENTFLAG_TUNNELPORTDEMANGLED  (2) // tunnel port has been demangled

/*** Type Definitions *************************************************************/

struct ConnApiRefT
{
    //! connapi user callback info
    ConnApiCallbackT        *pCallback[CONNAPI_MAX_CALLBACKS];
    void                    *pUserData[CONNAPI_MAX_CALLBACKS];

    //! dirtymem memory group
    int32_t                 iMemGroup;
    void                    *pMemGroupUserData;

    //! next unique client identifier
    uint16_t                uClientId;

    //! game port to connect on
    uint16_t                uGamePort;

    //! game port to use when game connectivity falls back to gameserver
    uint16_t                uFallbackGamePort;

    //! remote tunnel port to use when game connectivity falls back to gameserver
    uint16_t                uFallbackTunnelPort;

    //! voip port to connect on
    uint16_t                uVoipPort;

    //! game connection flags
    uint16_t                uConnFlags;

    //! connection mode
    uint16_t                uGameServConnMode;

    //! fallback mode
    uint16_t                uGameServFallback;

    //! netmask, used for external address comparisons
    uint32_t                uNetMask;

    //! game name
    char                    strGameName[32];

    //! game server name; empty if not using a gameserver
    char                    strGameServName[32];

    //! game server id; zero if not using a gameserver
    uint32_t                uGameServId;

    //! game server address; zero if not using a gameserver
    uint32_t                uGameServAddr;

    //! game server mode
    ConnApiGameServModeE    eGameServMode;

    //! game link buffer size
    int32_t                 iLinkBufSize;

    //! master game util ref (used for advertising)
    NetGameUtilRefT         *pGameUtilRef;

    //! protomangle ref
    ProtoMangleRefT         *pProtoMangle;

    //! prototunnel ref
    ProtoTunnelRefT         *pProtoTunnel;

    //! prototunnel crypto base key
    char                    strTunnelKey[64];

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

    //! our username
    char                    strSelfName[32];

    //! our address
    DirtyAddrT              SelfAddr;

    //! index of ourself in client list
    int32_t                 iSelf;

    //! session identifier
    int32_t                 iSessId;

    //! connection timeout value
    int32_t                 iConnTimeout;

    //! timeout value
    int32_t                 iTimeout;

    //! demangler timeout
    int32_t                 iMnglTimeout;

    //! gamelink configuration - input packet queue size
    int32_t                 iGameMinp;

    //! gamelink configuration - output packet queue size
    int32_t                 iGameMout;

    //! gamelink configuration - max packet width
    int32_t                 iGameMwid;

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

    //! in peer-web mode?
    uint8_t                 bPeerWeb;

    //! inside of a callback?
    uint8_t                 bInCallback;

    //! disc callback on client removal?
    uint8_t                 bRemoveCallback;

    //! auto-update enabled?
    uint8_t                 bAutoUpdate;

    //! 'phxc'
    uint8_t                 bPcHostXenonClientMode;

    //! 'adve', TRUE if ProtoAdvt advertising enabled
    uint8_t                 bDoAdvertising;

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
        "CONNAPI_STATUS_CLSE",
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
        NetPrintf(("connapi:   %d) %s %a <%s> iVoipConnId: %d\n", iClient, pClient->UserInfo.strName,
            pClient->UserInfo.uAddr, pClient->UserInfo.DirtyAddr.strMachineAddr, pClient->iVoipConnId));
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
    \Function _ConnApiInitClient

    \Description
        Initialize a single client based on input user info.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - pointer to client to init
    \Input *pUserInfo   - pointer to user info to init client with
    \Input iClientIndex - index of client

    \Version 01/06/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiInitClient(ConnApiRefT *pConnApi, ConnApiClientT *pClient, ConnApiUserInfoT *pUserInfo, int32_t iClientIndex)
{
    // initialize new client structure and save input user info
    memset(pClient, 0, sizeof(*pClient));
    memcpy(&pClient->UserInfo,  pUserInfo, sizeof(pClient->UserInfo));
    
    // initialize default voip values
    pClient->iVoipConnId = VOIP_CONNID_NONE;

    // set up remote (connect) port info
    pClient->GameInfo.uMnglPort = (pClient->UserInfo.uGamePort == 0) ? pConnApi->uGamePort : pClient->UserInfo.uGamePort;
    pClient->VoipInfo.uMnglPort = (pClient->UserInfo.uVoipPort == 0) ? pConnApi->uVoipPort : pClient->UserInfo.uVoipPort;

    // set up local (bind) port info
    pClient->GameInfo.uLocalPort = (pClient->UserInfo.uLocalGamePort == 0) ? pConnApi->uGamePort : pClient->UserInfo.uLocalGamePort;
    pClient->VoipInfo.uLocalPort = (pClient->UserInfo.uLocalVoipPort == 0) ? pConnApi->uVoipPort : pClient->UserInfo.uLocalVoipPort;

    // create unique id from external and internal addresses if not already supplied
    if (pClient->UserInfo.strUniqueId[0] == '\0')
    {
        snzprintf(pClient->UserInfo.strUniqueId, sizeof(pClient->UserInfo.strUniqueId),
            "$%08x$%08x", pClient->UserInfo.uAddr, pClient->UserInfo.uLocalAddr);
    }
    
    // set unique client identifier if not already supplied
    if (pClient->UserInfo.uClientId == 0)
    {
        pClient->UserInfo.uClientId = NetHash(pClient->UserInfo.strName);
    }

    // if we're a dedicated server in phxc mode, set the client address to zero(listen mode),
    // as the traffic will be from the XLSP and we don't know where that is.
    if (pConnApi->bPcHostXenonClientMode)
    {
        pClient->UserInfo.uAddr = 0;
        pClient->UserInfo.uLocalAddr = 0;
    }
    
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

            if ((iClient != pConnApi->iHostIndex) && (!pConnApi->bPeerWeb))
            {
                pClient->uConnFlags &= ~CONNAPI_CONNFLAG_GAMECONN;
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _ConnApiFindClientByName

    \Description
        Find a client in the client list by name.

    \Input *pClientList - pointer to client list
    \Input *pClientName - pointer to name of client to find

    \Output
        int32_t         - index of client in list, or negative if not in list

    \Version 04/08/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ConnApiFindClientByName(ConnApiClientListT *pClientList, const char *pClientName)
{
    int32_t iClientIndex;
    
    // search for client by name
    for (iClientIndex = 0; iClientIndex < pClientList->iMaxClients; iClientIndex++)
    {
        if (!DirtyUsernameCompare(pClientList->Clients[iClientIndex].UserInfo.strName, pClientName))
        {
            return(iClientIndex);
        }
    }

    // did not find client
    return(-1);
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

    // if fallback mode is disabled
    if ((pConnApi->uGameServFallback & uGameServConnMode) == 0)
    {
        // use gameserver if address is not zero and connmode says it is enabled
        bUseGameServer = (pConnApi->uGameServAddr != 0) && (pConnApi->uGameServConnMode & uGameServConnMode);
    }
    else if (pClient != NULL)
    {
        // use gameserver only if peer connection has already failed
        ConnApiConnInfoT *pInfo = (uGameServConnMode == CONNAPI_CONNFLAG_GAMECONN) ? &pClient->GameInfo : &pClient->VoipInfo;
        bUseGameServer = (pInfo->uPeerConnFailed & uGameServConnMode);
    }
    else
    {
        // if we don't have access to a client, then return a generic "true" to indicate we are using the GameServer
        bUseGameServer = TRUE;
    }

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
    uint32_t uClientIdA, uClientIdB, uTemp;

    uClientIdA = pConnApi->ClientList.Clients[pConnApi->iSelf].UserInfo.uClientId;
    uClientIdB = pClient->UserInfo.uClientId;
    if (uClientIdB < uClientIdA)
    {
        uTemp = uClientIdA;
        uClientIdA = uClientIdB;
        uClientIdB = uTemp;
    }

    snzprintf(pSess, iSessSize, "%08x-%08x-%s-%08x", uClientIdA, uClientIdB, pSessType, pConnApi->iSessId);
}

/*F********************************************************************************/
/*!
    \Function _ConnApiGenerateTunnelKey

    \Description
        Generate tunnel key for prototunnel use.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - pointer to peer
    \Input iClientIndex - index of peer
    \Input *pSess       - [out] pointer to session buffer
    \Input iSessSize    - size of session buffer

    \Todo
        Implement a more secure key.

    \Version 02/20/2009 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiGenerateTunnelKey(ConnApiRefT *pConnApi, ConnApiClientT *pClient, int32_t iClientIndex, char *pSess, int32_t iSessSize)
{
    uint32_t uClientIdA, uClientIdB, uTemp;
    
    uClientIdA = pConnApi->ClientList.Clients[pConnApi->iSelf].UserInfo.uClientId;
    uClientIdB = pClient->UserInfo.uClientId;
    if (uClientIdB < uClientIdA)
    {
        uTemp = uClientIdA;
        uClientIdA = uClientIdB;
        uClientIdB = uTemp;
    }
    
    snzprintf(pSess, iSessSize, "%08x-%08x-%s", uClientIdA, uClientIdB, pConnApi->strTunnelKey);
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

    \Output
        int32_t         - tunnel id

    \Version 12/20/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ConnApiTunnelAlloc(ConnApiRefT *pConnApi, ConnApiClientT *pClient, int32_t iClientIndex, uint32_t uRemoteAddr, uint8_t bLocalAddr)
{
    ProtoTunnelInfoT TunnelInfo;
    char strKey[PROTOTUNNEL_MAXKEYLEN];

    // set up tunnel info
    memset(&TunnelInfo, 0, sizeof(TunnelInfo));
    TunnelInfo.uRemoteClientId =  pClient->UserInfo.uClientId;
    TunnelInfo.uRemoteAddr = uRemoteAddr;
    TunnelInfo.uRemotePort = bLocalAddr ? pClient->UserInfo.uLocalTunnelPort : pClient->UserInfo.uTunnelPort;
    TunnelInfo.aRemotePortList[0] = pClient->GameInfo.uMnglPort;
    TunnelInfo.aRemotePortList[1] = pClient->VoipInfo.uMnglPort;
    TunnelInfo.aPortFlags[0] = (pConnApi->uGameTunnelFlagOverride) ? (pConnApi->uGameTunnelFlag) : (PROTOTUNNEL_PORTFLAG_ENCRYPTED|PROTOTUNNEL_PORTFLAG_AUTOFLUSH);

    NetPrintf(("connapi: [0x%08x] setting client %d TunnelInfo.uRemotePort %d\n", pConnApi, iClientIndex, TunnelInfo.uRemotePort));

    // generate tunnel crypto key
    if (pConnApi->strTunnelKey[0] != '\0')
    {
        _ConnApiGenerateTunnelKey(pConnApi, pClient, iClientIndex, strKey, sizeof(strKey));
    }
    else
    {
        _ConnApiGenerateSessionKey(pConnApi, pClient, iClientIndex, strKey, sizeof(strKey), "tunl");
    }

    // allocate tunnel and return to caller
    return(ProtoTunnelAlloc(pConnApi->pProtoTunnel, &TunnelInfo, strKey));
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

    \Output
        int32_t         - tunnel id

    \Version 12/23/2008 (jrainy)
*/
/********************************************************************************F*/
static int32_t _ConnApiVoipTunnelAlloc(ConnApiRefT *pConnApi, ConnApiClientT *pClient, int32_t iClientIndex, uint32_t uRemoteAddr, uint8_t bLocalAddr)
{
    int32_t iTunnelId = 0;

    // if doing redirect via host, check for previously created tunnel for re-use
    if (pConnApi->bVoipRedirectViaHost)
    {
        iTunnelId = pConnApi->ClientList.Clients[pConnApi->iHostIndex].iTunnelId;
    }

    // if doing gameserver voip, check for previously created tunnel for re-use
    if (_ConnApiGameServerEnabled(pConnApi, NULL, CONNAPI_CONNFLAG_VOIPCONN) && (pConnApi->eGameServMode != CONNAPI_GAMESERV_BROADCAST))
    {
        iTunnelId = pConnApi->GameServer.iTunnelId;
    }

    // if no reused tunnel, create one
    if (iTunnelId == 0)
    {
        iTunnelId = _ConnApiTunnelAlloc(pConnApi, pClient, iClientIndex, uRemoteAddr, bLocalAddr);
    }

    return(iTunnelId);
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
    char strKey[PROTOTUNNEL_MAXKEYLEN];

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

    // generate tunnel crypto key 
    if (pConnApi->strTunnelKey[0] != '\0') 
    { 
        _ConnApiGenerateTunnelKey(pConnApi, pClient, 0, strKey, sizeof(strKey)); 
    } 
    else 
    { 
        _ConnApiGenerateSessionKey(pConnApi, pClient, 0, strKey, sizeof(strKey), "tunl"); 
    } 

    if (pClient->iTunnelId != 0)
    {
        ProtoTunnelFree2(pConnApi->pProtoTunnel, pClient->iTunnelId, strKey, pClient->UserInfo.uAddr); 
    }
    pClient->iTunnelId = 0;
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
            uAddr = pConnApi->ClientList.Clients[pConnApi->iHostIndex].UserInfo.uAddr;
            pClientUsed = &pConnApi->ClientList.Clients[pConnApi->iHostIndex];
            NetPrintf(("connapi: [0x%08x] [%s] using host address to connect to (or disconnect from) user '%s'\n", pConnApi, pConn, pClient->UserInfo.strName));
        }
        else
        {
            // if external addresses match, use local address
            if ((pSelf->UserInfo.uAddr & pConnApi->uNetMask) == (pClient->UserInfo.uAddr & pConnApi->uNetMask))
            {
                NetPrintf(("connapi: [0x%08x] [%s] using local address to connect to (or disconnect from) user '%s'\n", pConnApi, pConn, pClient->UserInfo.strName));
                uAddr = pClient->UserInfo.uLocalAddr;
                pClientUsed = pClient;
                bLocalAddr = TRUE;
            }
            else
            {
                NetPrintf(("connapi: [0x%08x] [%s] using peer address to connect to (or disconnect from) user '%s'\n", pConnApi, pConn, pClient->UserInfo.strName));
                uAddr = pClient->UserInfo.uAddr;

                #if DIRTYCODE_DEBUG
                if (pConnApi->bFailP2PConnect)
                {
                    uAddr = (127 << 24) + 1;
                }
                #endif

                pClientUsed = pClient;
            }
        }
    }
    else
    {
        NetPrintf(("connapi: [0x%08x] [%s] using game server address%sto connect to (or disconnect from) user '%s'\n", pConnApi, pConn,
            (pConnApi->uGameServFallback & uConnMode) ? " as fallback " : " ",
            pClient->UserInfo.strName));
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

    \Version 11/11/2009 (mclouatre)
*/
/********************************************************************************F*/
static void _ConnApiVoipGroupConnSharingCallback(VoipGroupRefT *pVoipGroup, ConnSharingCbTypeE eCbType, int32_t iConnId, void *pUserData)
{
    ConnApiRefT *pConnApi = (ConnApiRefT *)pUserData;
    ConnApiClientT *pClient = &pConnApi->ClientList.Clients[iConnId];
    ConnApiClientT *pClientUsed;
    int32_t iConnectAddr;
    uint8_t bLocalAddr;

    iConnectAddr = _ConnApiGetConnectAddr(pConnApi, pClient, &bLocalAddr, CONNAPI_CONNFLAG_VOIPCONN, &pClientUsed);

    // do we have a tunnel to this client?
    if (pClientUsed->iTunnelId > 0)
    {
        uint32_t uVoipPort;

        NetPrintf(("connapi: [0x%08x] we have a tunnel for client %d; using virtual address %a\n", pConnApi,
            iConnId, pClientUsed->iTunnelId));
        iConnectAddr = pClientUsed->iTunnelId;

        // check for pre-existing voip port binding
        uVoipPort = VoipGroupStatus(pConnApi->pVoipGroupRef, 'lprt', 0, NULL, 0);

        // if voip already has a different port, update ours to match
        if ((uVoipPort != 0) && (uVoipPort != pConnApi->uVoipPort))
        {
            // update port map
            ProtoTunnelInfoT TunnelInfo;
            memset(&TunnelInfo, 0, sizeof(TunnelInfo));
            TunnelInfo.aRemotePortList[1] = uVoipPort;
            ProtoTunnelUpdatePortList(pConnApi->pProtoTunnel, pClientUsed->iTunnelId, &TunnelInfo);

            // update conn/bind ports
            pClient->VoipInfo.uMnglPort = uVoipPort;
            pClient->VoipInfo.uLocalPort = uVoipPort;
        }
    }

    switch (eCbType)
    {
        case (VOIPGROUP_CBTYPE_CONNSUSPEND):
            NetPrintf(("connapi: [0x%08x] suspending voip connection to user %s:%a at %d\n", pConnApi, pClient->UserInfo.strName, iConnectAddr, NetTick()));
            VoipGroupSuspend(pVoipGroup, iConnId);
            break;

        case (VOIPGROUP_CBTYPE_CONNRESUME):
            NetPrintf(("connapi: [0x%08x] resuming voip connection to user %s:%a at %d\n", pConnApi, pClient->UserInfo.strName, iConnectAddr, NetTick()));
            VoipGroupResume(pVoipGroup, iConnId, pClient->UserInfo.strUniqueId, iConnectAddr,
                pClient->VoipInfo.uMnglPort, pClient->VoipInfo.uLocalPort, pClient->UserInfo.uClientId, pConnApi->iSessId);
            break;

        default:
            NetPrintf(("connapi: [0x%08x] critical error - unknown connection sharing event type\n", pConnApi));
            break;
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
    char *pAddrA, *pAddrB, *pAddrT;
    uint32_t bHosting = TRUE;
    int32_t iConnFlags;
    
    // reference unique address strings
    pAddrA = pClientA->UserInfo.strUniqueId;
    pAddrB = pClientB->UserInfo.strUniqueId;

    // if we're in gameserver/peerconn mode, connname is our username and we are always connecting
    if (_ConnApiGameServerEnabled(pConnApi, pClientA, CONNAPI_CONNFLAG_GAMECONN) && (pConnApi->eGameServMode == CONNAPI_GAMESERV_PEERCONN))
    {
        strnzcpy(pConnName, pConnApi->strSelfName, iNameSize);
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
        bHosting = strcmp(pAddrA, pAddrB) < 0;
    }

    /* set up parms based on whether we are "hosting" or not.  the connection name is the
       unique address of the "host" concatenated with the unique address of the "client" */
    if (bHosting == TRUE)
    {
        // swap names
        pAddrT = pAddrB;
        pAddrB = pAddrA;
        pAddrA = pAddrT;

        // set conn flags
        iConnFlags = NETGAME_CONN_LISTEN;
    }
    else
    {
        iConnFlags = NETGAME_CONN_CONNECT;
    }

    // format connection name and return connection flags
    snzprintf(pConnName, iNameSize, "%s-%s", pAddrA, pAddrB);
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
    NetPrintf(("connapi: [0x%08x] destroying game connection to user %s: %s at %d\n", pConnApi, pClient->UserInfo.strName, pReason, NetTick()));
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
        NetPrintf(("connapi: [0x%08x] destroying voip connection to user %s: %s at %d\n", pConnApi, pClient->UserInfo.strName, pReason, NetTick()));
        VoipGroupDisconnect(pConnApi->pVoipGroupRef, pClient->iVoipConnId);
        pClient->iVoipConnId = VOIP_CONNID_NONE;
    }
    pClient->VoipInfo.eStatus = CONNAPI_STATUS_DISC;
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
static void _ConnApiInitClientList(ConnApiRefT *pConnApi, ConnApiUserInfoT *pClientList, int32_t iNumClients)
{
    ConnApiClientT *pClient;
    int32_t iClient;

    // make sure client count is below max
    if (iNumClients > pConnApi->ClientList.iMaxClients)
    {
        NetPrintf(("connapi: [0x%08x] cannot host %d clients; clamping to %d\n", pConnApi, iNumClients, pConnApi->ClientList.iMaxClients));
        iNumClients = pConnApi->ClientList.iMaxClients;
    }
    // init so we can check after setup to make sure we're in the list
    pConnApi->iSelf = -1;
    
    // copy input client list
    for (iClient = 0, pConnApi->ClientList.iNumClients = 0; iClient < iNumClients; iClient++)
    {
        // ref client structure
        pClient = &pConnApi->ClientList.Clients[iClient];
        
        if (pClientList[iClient].strName[0] != 0)
        {
            // init client structure and copy user info
            _ConnApiInitClient(pConnApi, pClient, &pClientList[iClient], iClient);

            // remember our index in list
            if (!strcmp(pClient->UserInfo.strName, pConnApi->strSelfName))
            {
                // update dirtyaddr and save ref
                memcpy(&pConnApi->SelfAddr, &pClient->UserInfo.DirtyAddr, sizeof(pConnApi->SelfAddr));
                pConnApi->iSelf = iClient;
            }

            // increment client count
            pConnApi->ClientList.iNumClients += 1;
        }
        else
        {
            pClient->GameInfo.eStatus = CONNAPI_STATUS_DISC;
            pClient->VoipInfo.eStatus = CONNAPI_STATUS_DISC;
            pClient->bAllocated = FALSE;
        }
    }

    // ref local client
    pClient = &pConnApi->ClientList.Clients[pConnApi->iSelf];

    if (pConnApi->GameServer.UserInfo.uAddr == 0)
    {
        // if gameserver, set up gameserver user in case we need it
        memcpy(&pConnApi->GameServer, pClient, sizeof(pConnApi->GameServer));
        pConnApi->GameServer.UserInfo.uAddr = pConnApi->uGameServAddr;
        strnzcpy(pConnApi->GameServer.UserInfo.strName, pConnApi->strGameServName, sizeof(pConnApi->GameServer.UserInfo.strName));
        pConnApi->GameServer.UserInfo.uClientId = pConnApi->uGameServId;
        pConnApi->GameServer.UserInfo.strUniqueId[0] = '\0';

        // make sure to setup gameserver properly in case it is used as a fallback
        if (pConnApi->uFallbackTunnelPort)
        {
            pConnApi->GameServer.UserInfo.uTunnelPort = pConnApi->uFallbackTunnelPort;
            pConnApi->GameServer.UserInfo.uLocalTunnelPort = pConnApi->uFallbackTunnelPort;
        }
        else
        {
            pConnApi->GameServer.UserInfo.uTunnelPort = pConnApi->iTunnelPort;
            pConnApi->GameServer.UserInfo.uLocalTunnelPort = pConnApi->iTunnelPort;
        }

        NetPrintf(("connapi: [0x%08x] setting GameServer ProtoTunnel port %d\n", pConnApi, pConnApi->GameServer.UserInfo.uTunnelPort));

        if (pConnApi->uFallbackGamePort)
        {
            pConnApi->GameServer.GameInfo.uMnglPort = pConnApi->uFallbackGamePort;
            pConnApi->GameServer.GameInfo.uLocalPort = pConnApi->uFallbackGamePort;
        }
    }

    // set local user
    if (pConnApi->pVoipRef != NULL)
    {
        if (pConnApi->bVoipOwner == TRUE)
        {
            VoipGroupControl(pConnApi->pVoipGroupRef, 'lusr', TRUE, pClient->UserInfo.strUniqueId);
        }
        VoipGroupControl(pConnApi->pVoipGroupRef, 'clid', pClient->UserInfo.uClientId, NULL);
    }
    
    // set prototunnel user
    if (pConnApi->pProtoTunnel != NULL)
    {
        ProtoTunnelControl(pConnApi->pProtoTunnel, 'clid', pClient->UserInfo.uClientId, 0, NULL);
    }

    // set initial client flags
    _ConnApiUpdateClientFlags(pConnApi);

    #if DIRTYCODE_LOGGING
    // make sure we're in the list
    if (pConnApi->iSelf == -1)
    {
        NetPrintf(("connapi: [0x%08x] local user %s not in client list\n", pConnApi, pConnApi->strSelfName));
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

    strnzcpy(pOutBuf, pInpBuf, iOutSize);
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

        // extract address from addr field
        pTemp = strchr(strAddr, ':');
        *pTemp = '\0';
        uLocalAddr = SocketInTextGetAddr(strAddr);

        // does the name match one of our client's names?
        for (iClient = 0; iClient < pConnApi->ClientList.iMaxClients; iClient++)
        {
            pClient = &pConnApi->ClientList.Clients[iClient];

            if (!strcmp(pClient->UserInfo.strName, strName) && (pClient->UserInfo.uLocalAddr != uLocalAddr))
            {
                NetPrintf(("connapi: [0x%08x] updating local address of user %s from %a to %a\n", pConnApi, strName, pClient->UserInfo.uAddr, uLocalAddr));
                pClient->UserInfo.uLocalAddr = uLocalAddr;
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
        NetPrintf(("connapi: [0x%08x] aborting demangle of user %s being removed from client list\n", pConnApi, pClient->UserInfo.strName));
        ProtoMangleControl(pConnApi->pProtoMangle, 'abrt', 0, 0, NULL);
    }

    // decrement overall count
    pConnApi->ClientList.iNumClients -= 1;

    memset(&pConnApi->ClientList.Clients[iClientIndex], 0, sizeof(ConnApiClientT));

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
    \Function _ConnApiCheckGameServerFallback

    \Description
        Determine if the Game Server is to be used to connect the GameClient or
        VoipClient.

    \Input *pConnApi            - pointer to module state
    \Input *pClient             - pointer to client
    \Input uGameServConnMode    - game conn or voip conn
    \Input iClientIndex         - client index

    \Version 02/21/2007 (jbrookes)
*/
/********************************************************************************F*/
static ConnApiConnStatusE _ConnApiCheckGameServerFallback(ConnApiRefT *pConnApi, ConnApiClientT *pClient, uint32_t uGameServConnMode, int32_t iClientIndex)
{
    ConnApiConnStatusE eStatus;
    ConnApiConnInfoT *pInfo = (uGameServConnMode == CONNAPI_CONNFLAG_GAMECONN) ? &pClient->GameInfo : &pClient->VoipInfo;

    #if DIRTYCODE_LOGGING
    const char *pConn = (uGameServConnMode & CONNAPI_CONNFLAG_GAMECONN) ? "game" : "voip";
    #endif
        
    if (pConnApi->uGameServFallback & ~pInfo->uPeerConnFailed & pConnApi->uGameServConnMode & uGameServConnMode)
    {
        NetPrintf(("connapi: [0x%08x] [%s] status=init, connection failed, using gameserver fallback for connection to user '%s'\n", pConnApi, pConn, pClient->UserInfo.strName));
        pInfo->uPeerConnFailed |= uGameServConnMode;
        eStatus  = CONNAPI_STATUS_INIT;

        NetPrintf(("connapi: [0x%08x] [%s] game port fallback: %d -> %d\n", pConnApi, pConn, pClient->GameInfo.uMnglPort, pConnApi->uFallbackGamePort));
        pClient->GameInfo.uMnglPort = pConnApi->uFallbackGamePort;
        pClient->GameInfo.uLocalPort = pConnApi->uFallbackGamePort;

        NetPrintf(("connapi: [0x%08x] freeing previous tunnel for user '%s'\n", pConnApi, pClient->UserInfo.strName));
        _ConnApiTunnelFree(pConnApi, pClient);
    }
    else
    {
        NetPrintf(("connapi: [0x%08x] [%s] status=disc, connection failed%sfor connection to user %s\n", pConnApi, pConn,
            (pInfo->uPeerConnFailed & uGameServConnMode) ? " after gameserver fallback " : " ",
            pClient->UserInfo.strName));
        eStatus  = CONNAPI_STATUS_DISC;
    }
    return(eStatus);
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

        NetPrintf(("connapi: [0x%08x] establishing game connection to user %s:%a at %d\n", pConnApi,
            pClient->UserInfo.strName, iConnectAddr, NetTick()));

        // create tunnel?
        if ((pConnApi->bTunnelEnabled) && (pClientUsed->iTunnelId == 0))
        {
            pClientUsed->iTunnelId = _ConnApiTunnelAlloc(pConnApi, pClientUsed, iClientIndex, iConnectAddr, bLocalAddr);
        }

        // do we have a tunnel to this client?
        if (pClientUsed->iTunnelId > 0)
        {
            NetPrintf(("connapi: [0x%08x] tunnel allocated for client %d; switching to use virtual address %a\n", pConnApi,
                iClientIndex, pClientUsed->iTunnelId));
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

            // set our client id (used by gameserver to uniquely identify us)
            NetGameUtilControl(pClient->pGameUtilRef, 'clid', pConnApi->ClientList.Clients[pConnApi->iSelf].UserInfo.uClientId);

            NetGameUtilControl(pClient->pGameUtilRef, 'rcid', pClient->UserInfo.uClientId);

            // determine connection parameters
            iConnFlags = _ConnApiGetConnectParms(pConnApi, pClient, strConnName, sizeof(strConnName));

            // format connect string
            snzprintf(strConn, sizeof(strConn), "%a:%d:%d#%s", iConnectAddr,
                pClient->GameInfo.uLocalPort, pClient->GameInfo.uMnglPort, strConnName);

            // start the connection attempt
            NetGameUtilConnect(pClient->pGameUtilRef, iConnFlags, strConn, pConnApi->pCommConstruct);
            pClient->GameInfo.eStatus = CONNAPI_STATUS_CONN;

            // if tunnel is active, update port map with actual port that was bound, if differing from requested
            if (pClientUsed->iTunnelId > 0)
            {
                uint32_t uGamePort = NetGameUtilStatus(pClient->pGameUtilRef, 'hprt', NULL, 0);
                if (uGamePort != pClient->GameInfo.uMnglPort)
                {
                    ProtoTunnelInfoT TunnelInfo;
                    memset(&TunnelInfo, 0, sizeof(TunnelInfo));
                    TunnelInfo.aRemotePortList[0] = uGamePort;
                    TunnelInfo.aPortFlags[0] = (pConnApi->uGameTunnelFlagOverride) ? (pConnApi->uGameTunnelFlag) : (PROTOTUNNEL_PORTFLAG_ENCRYPTED|PROTOTUNNEL_PORTFLAG_AUTOFLUSH);
                    ProtoTunnelUpdatePortList(pConnApi->pProtoTunnel, pClientUsed->iTunnelId, &TunnelInfo);
                }
            }

            // set if this is a GameServer connection or not
            if ((iConnectAddr == (signed)pConnApi->uGameServAddr) && (pConnApi->uGameServAddr != 0))
            {
                pClient->GameInfo.uConnFlags |= CONNAPI_CONNFLAG_GAMESERVER;
            }

            // remember connection start time
            pClient->iConnStart = NetTick();

            NetGameUtilControl(pClient->pGameUtilRef, 'meta', ((pConnApi->uGameServAddr != 0) && (pConnApi->eGameServMode == CONNAPI_GAMESERV_BROADCAST)) ? 1 : 0);
        }
        else
        {
            NetPrintf(("connapi: [0x%08x] unable to allocate util ref for connection %d to user %s\n", pConnApi, iClientIndex, pClient->UserInfo.strName));
            pClient->GameInfo.eStatus = CONNAPI_STATUS_DISC;
        }
    }

    // waiting for connection
    if (pClient->GameInfo.eStatus == CONNAPI_STATUS_CONN)
    {
        void *pCommRef;

        // check for established connection
        if ((pCommRef = NetGameUtilComplete(pClient->pGameUtilRef)) != NULL)
        {
            DirtyMemGroupEnter(pConnApi->iMemGroup, pConnApi->pMemGroupUserData);
            pClient->pGameLinkRef = NetGameLinkCreate(pCommRef, FALSE, pConnApi->iLinkBufSize);
            DirtyMemGroupLeave();
            if (pClient->pGameLinkRef != NULL)
            {
                NetPrintf(("connapi: [0x%08x] game connection %d to user %s established at %d\n", pConnApi, iClientIndex, pClient->UserInfo.strName, NetTick()));

                // if we were demangling, report success
                _ConnApiDemangleReport(pConnApi, &pClient->GameInfo, PROTOMANGLE_STATUS_CONNECTED);

                // save socket ref for multi-demangle if we need it
                if (!pConnApi->bTunnelEnabled)
                {
                    NetGameUtilStatus(pClient->pGameUtilRef, 'sock', &pConnApi->uGameSockRef, sizeof(pConnApi->uGameSockRef));
                }
                
                // mark as active
                pClient->GameInfo.eStatus = CONNAPI_STATUS_ACTV;
                pClient->GameInfo.uConnFlags |= CONNAPI_CONNFLAG_CONNECTED;
            }
            else
            {
                NetPrintf(("connapi: [0x%08x] unable to allocate link ref for connection %d to user %s\n", pConnApi, iClientIndex, pClient->UserInfo.strName));
                pClient->GameInfo.eStatus = CONNAPI_STATUS_DISC;
            }
        }
        
        // check for connection timeout
        if (NetTickDiff(NetTick(), pClient->iConnStart) > pConnApi->iConnTimeout)
        {
            _ConnApiDestroyGameConnection(pConnApi, pClient, iClientIndex, "connection timeout");

            // initial attempt to connect failed
            if (pClient->GameInfo.bDemangling == FALSE)
            {
                if (pConnApi->bDemanglerEnabled)
                {
                    NetPrintf(("connapi: [0x%08x] game status=mngl for connection %d to user %s\n", pConnApi, iClientIndex, pClient->UserInfo.strName));
                    pClient->GameInfo.eStatus = CONNAPI_STATUS_MNGL;
                }
                else 
                {
                    // change status if fallback is enabled for game connections, otherwise, disconnect
                    pClient->GameInfo.eStatus = _ConnApiCheckGameServerFallback(pConnApi, pClient, CONNAPI_CONNFLAG_GAMECONN, iClientIndex);
                }
            }
            else
            {
                NetPrintf(("connapi: [0x%08x] game connection to user %s after demangle failed\n", pConnApi, pClient->UserInfo.strName));
                _ConnApiDemangleReport(pConnApi, &pClient->GameInfo, PROTOMANGLE_STATUS_FAILED);

                // change status if fallback is enabled for game connections, otherwise, disconnect
                pClient->GameInfo.eStatus = _ConnApiCheckGameServerFallback(pConnApi, pClient, CONNAPI_CONNFLAG_GAMECONN, iClientIndex);
            }
        } // timeout
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

        NetPrintf(("connapi: [0x%08x] establishing voip connection to user %s:%a at %d\n", pConnApi,
            pClient->UserInfo.strName, iConnectAddr, NetTick()));

        // create tunnel?
        if ((pConnApi->bTunnelEnabled) && (pClientUsed->iTunnelId == 0))
        {
            pClientUsed->iTunnelId = _ConnApiVoipTunnelAlloc(pConnApi, pClientUsed, iClientIndex, iConnectAddr, bLocalAddr);
        }

        // do we have a tunnel to this client?
        if (pClientUsed->iTunnelId > 0)
        {
            uint32_t uVoipPort;

            NetPrintf(("connapi: [0x%08x] tunnel allocated for client %d; switching to use virtual address %a\n", pConnApi,
                iClientIndex, pClientUsed->iTunnelId));
            iConnectAddr = pClientUsed->iTunnelId;

            // check for pre-existing voip port binding
            uVoipPort = VoipGroupStatus(pConnApi->pVoipGroupRef, 'lprt', 0, NULL, 0);

            // if voip already has a different port, update ours to match
            if ((uVoipPort != 0) && (uVoipPort != pConnApi->uVoipPort))
            {
                // update port map
                ProtoTunnelInfoT TunnelInfo;
                memset(&TunnelInfo, 0, sizeof(TunnelInfo));
                TunnelInfo.aRemotePortList[1] = uVoipPort;
                ProtoTunnelUpdatePortList(pConnApi->pProtoTunnel, pClientUsed->iTunnelId, &TunnelInfo);

                // update conn/bind ports
                pClient->VoipInfo.uMnglPort = uVoipPort;
                pClient->VoipInfo.uLocalPort = uVoipPort;
            }
        }

        // initiate connection attempt
        iVoipConnId = VoipGroupConnect(pConnApi->pVoipGroupRef, iClientIndex, pClient->UserInfo.strUniqueId, iConnectAddr,
                          pClient->VoipInfo.uMnglPort, pClient->VoipInfo.uLocalPort, pClient->UserInfo.uClientId, pConnApi->iSessId);
        if (iVoipConnId >= 0)
        {
            pClient->iVoipConnId = iVoipConnId;
            pClient->VoipInfo.eStatus = CONNAPI_STATUS_CONN;

            // set if this is a GameServer connection or not
            if ((iConnectAddr == (signed)pConnApi->uGameServAddr) && (pConnApi->uGameServAddr != 0))
            {
                pClient->VoipInfo.uConnFlags |= CONNAPI_CONNFLAG_GAMESERVER;
            }
        }
        else
        {
            NetPrintf(("connapi: [0x%08x] unable to init voip for connection %d to user %s\n", pConnApi, iClientIndex, pClient->UserInfo.strName));
            pClient->VoipInfo.eStatus = CONNAPI_STATUS_DISC;
        }
    }
    
    // get connection status
    if ((pClient->VoipInfo.eStatus == CONNAPI_STATUS_CONN) || (pClient->VoipInfo.eStatus == CONNAPI_STATUS_ACTV))
    {
        iVoipStatus = VoipGroupRemote(pConnApi->pVoipGroupRef, pClient->iVoipConnId);
    }
    
    // for both cases below, CONNAPI_STATUS_CONN and CONNAPI_STATUS_ACTV, when we detect a disconnection,
    // we won't set the iVoipConnId to NONE nor trigger a disconnection. This is because the voipgroup code,
    // in order to provide correct voip connection sharing, needs to be told only of authoritative "connect" 
    // and "disconnect" by the higher-level lobby (plasma, lobbysdk, blazesdk, ...) techonology.

    // update connection status during connect phase
    if (pClient->VoipInfo.eStatus == CONNAPI_STATUS_CONN)
    {
        // check for established connection
        if (iVoipStatus & VOIP_REMOTE_CONNECTED)
        {
            NetPrintf(("connapi: [0x%08x] voip connection %d to user %s established at %d\n", pConnApi, iClientIndex, pClient->UserInfo.strName, NetTick()));
            
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
        else if (iVoipStatus & VOIP_REMOTE_DISCONNECTED)
        {
            NetPrintf(("connapi: [0x%08x] voip connection attempt to user %s failed at %d\n", pConnApi, pClient->UserInfo.strName, NetTick()));
        
            if (pClient->VoipInfo.bDemangling == TRUE)
            {
                NetPrintf(("connapi: [0x%08x] voip connection attempt to user %s after demangle failed\n", pConnApi, pClient->UserInfo.strName));
                _ConnApiDemangleReport(pConnApi, &pClient->VoipInfo, PROTOMANGLE_STATUS_FAILED);

                // change status if fallback is enabled for voip connections, otherwise, disconnect
                pClient->VoipInfo.eStatus = _ConnApiCheckGameServerFallback(pConnApi, pClient, CONNAPI_CONNFLAG_VOIPCONN, iClientIndex);
            }
            else
            {
                if (pConnApi->bDemanglerEnabled)
                {
                    NetPrintf(("connapi: [0x%08x] voip status=mngl for connection %d to user %s\n", pConnApi, iClientIndex, pClient->UserInfo.strName));
                    pClient->VoipInfo.eStatus = CONNAPI_STATUS_MNGL;
                }
                else 
                {
                    // change status if fallback is enabled for voip connections, otherwise, disconnect
                    pClient->VoipInfo.eStatus = _ConnApiCheckGameServerFallback(pConnApi, pClient, CONNAPI_CONNFLAG_VOIPCONN, iClientIndex);
                }
            } // bDemangling = FALSE
        } // REMOTE_DISCONNECTED
    } // STATUS_CONN

    // update client in active state
    if (pClient->VoipInfo.eStatus == CONNAPI_STATUS_ACTV)
    {
        if (iVoipStatus & VOIP_REMOTE_DISCONNECTED)
        {
            NetPrintf(("connapi: [0x%08x] voip connection to user %s terminated at %d\n", pConnApi, pClient->UserInfo.strName, NetTick()));
            pClient->VoipInfo.eStatus = CONNAPI_STATUS_DISC;
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

    \Version 01/17/2005 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _ConnApiUpdateDemangleReport(ConnApiRefT *pConnApi)
{
    int32_t iUnused1, iUnused2;

    // if not reporting, don't process
    if (pConnApi->bReporting != FALSE)
    {
        // update client
        ProtoMangleUpdate(pConnApi->pProtoMangle);
    
        // check for completion
        if (ProtoMangleComplete(pConnApi->pProtoMangle, &iUnused1, &iUnused2) != 0)
        {
            pConnApi->bReporting = FALSE;
        }
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
    int32_t iClient;
    ConnApiCbTypeE eCbType;

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

    // tunnel-specific processing
    if (pConnApi->bTunnelEnabled)
    {
        // if we've already demangled the tunnel port, use previous demangle result
        if (pClient->uFlags & CONNAPI_CLIENTFLAG_TUNNELPORTDEMANGLED)
        {
            ConnApiConnStatusE eStatus = pConnInfo->eStatus;
            pConnInfo->eStatus = CONNAPI_STATUS_INIT;

            NetPrintf(("connapi: [0x%08x] reusing previously demangled tunnel port\n", pConnApi));

            // trigger callback if state change
            _ConnApiUpdateCallback(pConnApi, iClientIndex, eCbType, eStatus, (ConnApiConnStatusE)pConnInfo->eStatus);
            return;
        }
    }
    
    // show we're demangling
    pConnInfo->bDemangling = TRUE;
    
    // are we in an idle state?
    if (ProtoMangleStatus(pConnApi->pProtoMangle, 'idle', NULL, 0))
    {
        char strSess[64];
        intptr_t uSockRef;
        uint32_t uDemanglePort = 0;
        
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
        // before demangling, flush the tunnel to make sure there are no buffered packets
        if (pConnApi->bTunnelEnabled)
        {
            ProtoTunnelControl(pConnApi->pProtoTunnel, 'flsh', pClient->iTunnelId, 0, NULL);
        }
        
        // kick off demangling process
        if (uSockRef == 0)
        {
            NetPrintf(("connapi: [0x%08x] initiating %s:%d demangle of user %s at %d\n", pConnApi, _Types[iType], uDemanglePort, pClient->UserInfo.strName, NetTick()));
            ProtoMangleConnect(pConnApi->pProtoMangle, uDemanglePort, strSess);
        }
        else
        {        
            NetPrintf(("connapi: [0x%08x] initiating %s demangle of user %s using sockref 0x%08x at %d\n", pConnApi, _Types[iType], pClient->UserInfo.strName, uSockRef, NetTick()));
            ProtoMangleConnectSocket(pConnApi->pProtoMangle, uSockRef, strSess);
        }
    }
    else
    {
        int32_t iAddr, iPort, iResult;
        
        // update client
        ProtoMangleUpdate(pConnApi->pProtoMangle);
        
        // check for completion
        if ((iResult = ProtoMangleComplete(pConnApi->pProtoMangle, &iAddr, &iPort)) != 0)
        {
            ConnApiConnStatusE eStatus = pConnInfo->eStatus;

            if (eStatus != CONNAPI_STATUS_ACTV)
            {
                if (iResult > 0)
                {
                    NetPrintf(("connapi: [0x%08x] %s demangle of user %s successful port=%d at %d\n", pConnApi, _Types[iType], pClient->UserInfo.strName, iPort, NetTick()));
                    pClient->UserInfo.uAddr = pClient->UserInfo.uLocalAddr = iAddr;
                    if (pConnApi->bTunnelEnabled)
                    {
                        ProtoTunnelControl(pConnApi->pProtoTunnel, 'rprt', pClient->iTunnelId, iPort, NULL);
                        pClient->uFlags |= CONNAPI_CLIENTFLAG_TUNNELPORTDEMANGLED;
                    }
                    else
                    {
                        pConnInfo->uMnglPort = (uint16_t)iPort;
                    }
                    pConnInfo->eStatus = CONNAPI_STATUS_INIT;
                    pConnInfo->uConnFlags |= CONNAPI_CONNFLAG_DEMANGLED;
                }
                else
                {
                    NetPrintf(("connapi: [0x%08x] %s demangle of user %s failed (timeout=%s)\n", pConnApi, _Types[iType], pClient->UserInfo.strName,
                        ProtoMangleStatus(pConnApi->pProtoMangle, 'time', NULL, 0) ? "true" : "false"));
                    pConnInfo->eStatus = _ConnApiCheckGameServerFallback(pConnApi, pClient, _Flags[iType], iClientIndex);
                }
            }
            else
            {
                NetPrintf(("connapi: [0x%08x] %s demangle of user %s finished with %d but ignored because connection aleady active (timeout=%s)\n", pConnApi,
                    _Types[iType], pClient->UserInfo.strName, iResult, ProtoMangleStatus(pConnApi->pProtoMangle, 'time', NULL, 0) ? "true" : "false"));
            }

            // trigger callback if state change
            _ConnApiUpdateCallback(pConnApi, iClientIndex, eCbType, eStatus, (ConnApiConnStatusE)pConnInfo->eStatus);
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
    \Input *pClientName - pointer to name of client to remove (NULL if iClientIndex should be used)
    \Input iClientIndex - index of client to remove (used if pClientName is NULL)
    \Input uFlags       - client removal flags

    \Notes
        If this function is called inside of a ConnApi callback, the removal will
        be deferred until the next time NetConnIdle() is called.  Otherwise, the
        removal will happen immediately.

    \Version 04/08/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiRemoveClientSetup(ConnApiRefT *pConnApi, const char *pClientName, int32_t iClientIndex, uint16_t uFlags)
{
    ConnApiClientT *pClient;
    
    // get client index in list?
    if (pClientName != NULL)
    {
        if ((iClientIndex = _ConnApiFindClientByName(&pConnApi->ClientList, pClientName)) < 0)
        {
            NetPrintf(("connapi: [0x%08x] can't find client '%s' in list to remove them\n", pConnApi, pClientName));
            return;
        }
    }

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

    // update reporting
    bDemangling = _ConnApiUpdateDemangleReport(pConnApi);
    
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

        if ((pConnApi->iHostIndex == pConnApi->iSelf) || (pClient->uConnFlags & CONNAPI_CONNFLAG_GAMECONN))
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
            // ref connection
            if (pConnApi->bVoipRedirectViaHost)
            {
                if (iClientIndex == pConnApi->iHostIndex)
                {
                    continue;
                }
            }

            pClient = &pConnApi->ClientList.Clients[iClientIndex];
            
            // don't update if iClientIndex is us or unallocated
            if ((iClientIndex == pConnApi->iSelf) || !pClient->bAllocated)
            {
                continue;
            }
        
            // process voip connection
            iActive += _ConnApiUpdateVoipClient(pConnApi, pClient, iClientIndex);
        }
    }

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
        if ((pClient->GameInfo.eStatus == CONNAPI_STATUS_MNGL) && (bDemangling == FALSE))
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

    // update reporting
    bDemangling = _ConnApiUpdateDemangleReport(pConnApi);

    // process game connection to gameserver
    if (pConnApi->ClientList.iNumClients > 0)
    {
        _ConnApiUpdateGameClient(pConnApi, &pConnApi->GameServer, 0);
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

    // update game demangling
    if (pConnApi->ClientList.iNumClients > 0)
    {
        if ((pClient->GameInfo.eStatus == CONNAPI_STATUS_MNGL) && (bDemangling == FALSE))
        {
            _ConnApiUpdateDemangle(pConnApi, pClient, 0, &pClient->GameInfo, 0);
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

/*F********************************************************************************/
/*!
    \Function _ConnApiUpdateGameLink

    \Description
        Calls NetGameLinkUpdate on all active netgamelink channels. To be done regularly

    \Input *pRef    - pointer to module state

    \Version 01/11/10 (jrainy)
*/
/********************************************************************************F*/
static void _ConnApiUpdateGameLink(ConnApiRefT *pRef)
{
    int32_t iIndex;

    if (pRef->GameServer.pGameLinkRef)
    {
        NetGameLinkUpdate(pRef->GameServer.pGameLinkRef);
    }
    for(iIndex = 0; iIndex < pRef->ClientList.iMaxClients; iIndex++)
    {
        if (pRef->ClientList.Clients[iIndex].pGameLinkRef)
        {
            NetGameLinkUpdate(pRef->ClientList.Clients[iIndex].pGameLinkRef);
        }
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
    memset(pConnApi, 0, iSize);
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
    pConnApi->uClientId = 1;
    pConnApi->uConnFlags = CONNAPI_CONNFLAG_GAMEVOIP;
    pConnApi->iLinkBufSize = CONNAPI_LINKBUFDEFAULT;
    pConnApi->iConnTimeout = CONNAPI_CONNTIMEOUT_DEFAULT;
    pConnApi->iTimeout = CONNAPI_TIMEOUT_DEFAULT;
    pConnApi->iMnglTimeout = CONNAPI_DEMANGLER_TIMEOUT;
    pConnApi->iTunnelPort = 3658;
    pConnApi->bDemanglerEnabled = TRUE;
    pConnApi->bUpnpEnabled = TRUE;
    pConnApi->bVoipEnabled = TRUE;
    pConnApi->bVoipRedirectViaHost = FALSE;
    pConnApi->bAutoUpdate = TRUE;
    pConnApi->bDoAdvertising = TRUE;
    pConnApi->uGameServConnMode = CONNAPI_CONNFLAG_GAMEVOIP;
    pConnApi->uGameServFallback = 0; // default fallback to off
    pConnApi->uNetMask = 0xffffffff;

    // set default demangler server and create demangler
    strnzcpy(pConnApi->strDemanglerServer, PROTOMANGLE_SERVER, sizeof(pConnApi->strDemanglerServer));

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
    \Input *pSelfName   - pointer to our name
    \Input *pSelfAddr   - pointer to our address

    \Version 01/06/2005 (jbrookes)
*/
/********************************************************************************F*/
void ConnApiOnline(ConnApiRefT *pConnApi, const char *pGameName, const char *pSelfName, DirtyAddrT *pSelfAddr)
{
    // save info
    strnzcpy(pConnApi->strGameName, pGameName, sizeof(pConnApi->strGameName));
    strnzcpy(pConnApi->strSelfName, pSelfName, sizeof(pConnApi->strSelfName));
    memcpy(&pConnApi->SelfAddr, pSelfAddr, sizeof(pConnApi->SelfAddr));

    // set memory grouping
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
        NetPrintf(("connapi: [0x%08x] creating demangler ref with gamename=%s and server=%s\n", pConnApi, pConnApi->strGameName, pConnApi->strDemanglerServer));
        if ((pConnApi->pProtoMangle = ProtoMangleCreate(pConnApi->strDemanglerServer, PROTOMANGLE_PORT, pConnApi->strGameName, "")) == NULL)
        {
            NetPrintf(("connapi: [0x%08x] unable to create ProtoMangle module\n", pConnApi));
            pConnApi->bDemanglerEnabled = FALSE;
        }
    }

    // create tunnel module
    if ((pConnApi->bTunnelEnabled) && (pConnApi->pProtoTunnel == NULL))
    {
        if ((pConnApi->pProtoTunnel = ProtoTunnelCreate(pConnApi->ClientList.iMaxClients-1, pConnApi->iTunnelPort)) == NULL)
        {
            // unable to create, so diable the tunnel
            pConnApi->bTunnelEnabled = FALSE;
        }
        else
        {
            // we own the tunnel
            pConnApi->bTunnelOwner = TRUE;
        }
    }

    if ((pConnApi->bTunnelEnabled == TRUE) && pConnApi->bPcHostXenonClientMode)
    {
        ProtoTunnelControl(pConnApi->pProtoTunnel, 'mode', PROTOTUNNEL_MODE_XENON, 0, 0); //speak xenon
    }

    // get VoIP ref
    if ((pConnApi->bVoipEnabled) && (pConnApi->pVoipRef == NULL))
    {
        if ((pConnApi->pVoipRef = VoipGetRef()) == NULL)
        {
            NetPrintf(("connapi: [0x%08x] starting VoIP with iMaxClients=%d\n", pConnApi, pConnApi->ClientList.iMaxClients));
            pConnApi->pVoipRef = VoipStartup(pConnApi->ClientList.iMaxClients, 1);
            pConnApi->bVoipOwner = TRUE;
        }
    }

    // set voip/gamelink timeouts
    ConnApiControl(pConnApi, 'time', pConnApi->iTimeout, 0, NULL);

    // set demangler timeout
    ConnApiControl(pConnApi, 'dtim', pConnApi->iMnglTimeout, 0, NULL);

    // start advertising
    if (pConnApi->pGameUtilRef != NULL)
    {
        NetGameUtilAdvert(pConnApi->pGameUtilRef, pConnApi->strGameName, pConnApi->strSelfName, "");
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
        VoipShutdown(pConnApi->pVoipRef);
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
    \Input iPlatformHostIndex   - index in the client list who will be serving as platform host 
    \Input iSessId              - unique session identifier
    \Input uGameFlags           - CONNAPI_GAMEFLAG_*

    \Version 09/29/2009 (cvienneau)
*/
/********************************************************************************F*/
void ConnApiConnect(ConnApiRefT *pConnApi, ConnApiUserInfoT *pClientList, int32_t iClientListSize, int32_t iTopologyHostIndex, int32_t iPlatformHostIndex, int32_t iSessId, uint32_t uGameFlags)
{
    NetPrintf(("connapi: [0x%08X] connecting, listSize=%d, topologyHost=%d, platformHost=%d, sessionId=%d, flags=%x/n", pConnApi, iClientListSize, iTopologyHostIndex, iPlatformHostIndex, iSessId, uGameFlags));

    // make sure we're idle
    if (pConnApi->eState != ST_IDLE)
    {
        NetPrintf(("connapi: [0x%08x] can't host or connect to a game when not in idle state\n", pConnApi));
        return;
    }

    // save session identifier
    pConnApi->iSessId = iSessId;

    // init client list
    _ConnApiInitClientList(pConnApi, pClientList, iClientListSize);
    pConnApi->iHostIndex = iTopologyHostIndex;

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
            pClient->GameInfo.eStatus = CONNAPI_STATUS_INIT;
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
        Add a new client to a pre-existing game. Client added in first empty slot.

    \Input *pConnApi    - pointer to module state
    \Input *pUserInfo   - info on joining user

    \Output
        int32_t         - 0 if successful, error code otherwise.

    \Notes
        This function should be called by all current members of a game while
        ConnApiConnect() is called by the joining client.

    \Version 01/17/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ConnApiAddClient(ConnApiRefT *pConnApi, ConnApiUserInfoT *pUserInfo)
{
    ConnApiClientT *pClient;
    int32_t iClientIndex;

    for (iClientIndex = 0; iClientIndex < pConnApi->ClientList.iMaxClients; iClientIndex++)
    {
        pClient = &pConnApi->ClientList.Clients[iClientIndex];

        if (pClient->bAllocated == FALSE)
        {
            return(ConnApiAddClient2(pConnApi, pUserInfo, iClientIndex));
        }
    }

    return(CONNAPI_ERROR_CLIENTLIST_FULL);
}

/*F********************************************************************************/
/*!
    \Function ConnApiAddClient2

    \Description
        Add a new client to a pre-existing game at the specified index.

    \Input *pConnApi    - pointer to module state
    \Input *pUserInfo   - info on joining user
    \Input iClientIndex - index to add client to

    \Output
        0 if successful, error code otherwise.

    \Notes
        This function should be called by all current members of a game while
        ConnApiConnect() is called by the joining client.

    \Version 06/16/2008 (jbrookes)
*/
/********************************************************************************F*/
int32_t ConnApiAddClient2(ConnApiRefT *pConnApi, ConnApiUserInfoT *pUserInfo, int32_t iClientIndex)
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

    // get pointer to new client structure to fill in, and increment client count
    pClient = &pConnApi->ClientList.Clients[iClientIndex];

    // check slot and make sure it is uninitialized
    if (pClient->bAllocated == TRUE)
    {
        NetPrintf(("connapi: [0x%08x] slot %d already allocated; cannot add a new client in this slot\n", pConnApi, iClientIndex));
        return(CONNAPI_ERROR_SLOT_USED);
    }

    // add client to list
    _ConnApiInitClient(pConnApi, pClient, pUserInfo, iClientIndex);

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
        Returns the ConnApiClientT of a given client, if found by name.

    \Input *pConnApi    - pointer to module state
    \Input *pUserInfo   - info on searched user
    \Input *pOutClient  - used to return the ClientT structure of the client

    \Output
        uint8_t         - TRUE if the client is found, FALSE otherwise

    \Version 01/30/2008 (jrainy)
*/
/********************************************************************************F*/
uint8_t ConnApiFindClient(ConnApiRefT *pConnApi, ConnApiUserInfoT *pUserInfo, ConnApiClientT *pOutClient)
{
    int32_t iClient;
    
    if ((iClient = _ConnApiFindClientByName(&pConnApi->ClientList, pUserInfo->strName)) >= 0)
    {
        memcpy(pOutClient, &pConnApi->ClientList.Clients[iClient], sizeof(*pOutClient));
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}

/*F********************************************************************************/
/*!
    \Function ConnApiFindClientById

    \Description
        Returns the ConnApiClientT of a given client, if found by id.

    \Input *pConnApi    - pointer to module state
    \Input *pUserInfo   - info on searched user
    \Input *pOutClient  - used to return the ClientT structure of the client
    
    \Output
        uint8_t         - TRUE if the client is found, FALSE otherwise

    \Version 06/05/2008 (jbrookes)
*/
/********************************************************************************F*/
uint8_t ConnApiFindClientById(ConnApiRefT *pConnApi, ConnApiUserInfoT *pUserInfo, ConnApiClientT *pOutClient)
{
    int32_t iClient;

    for (iClient = 0; iClient < pConnApi->ClientList.iMaxClients; iClient++)
    {
        if (pConnApi->ClientList.Clients[iClient].UserInfo.uClientId == pUserInfo->uClientId)
        {
            memcpy(pOutClient, &pConnApi->ClientList.Clients[iClient], sizeof(*pOutClient));
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
    \Input *pClientName - pointer to name of client to remove (NULL if iClientIndex should be used)
    \Input iClientIndex - index of client to remove (used if pClientName is NULL)

    \Notes
        If this function is called inside of a ConnApi callback, the removal will
        be deferred until the next time NetConnIdle() is called.  Otherwise, the
        removal will happen immediately.

    \Version 06/14/2008 (jbrookes)
*/
/********************************************************************************F*/
void ConnApiRemoveClient(ConnApiRefT *pConnApi, const char *pClientName, int32_t iClientIndex)
{
    _ConnApiRemoveClientSetup(pConnApi, pClientName, iClientIndex, CONNAPI_CLIENTFLAG_REMOVE);
}

/*F********************************************************************************/
/*!
    \Function ConnApiStart

    \Description
        Start game session.

    \Input *pConnApi    - pointer to module state
    \Input uStartFlags  - CONNAPI_STRTFLAG_*

    \Notes
        This function is a stub on non-Xenon platforms.

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
void ConnApiStart(ConnApiRefT *pConnApi, uint32_t uStartFlags)
{
}

/*F********************************************************************************/
/*!
    \Function ConnApiStop

    \Description
        End game session.

    \Input *pConnApi    - pointer to module state

    \Notes
        This function is a stub on non-Xenon platforms.

    \Version 01/30/2007 (jbrookes)
*/
/********************************************************************************F*/
void ConnApiStop(ConnApiRefT *pConnApi)
{
}

/*F********************************************************************************/
/*!
    \Function ConnApiRematch

    \Description
        End game session.

    \Input *pConnApi    - pointer to module state

    \Notes
        This function is a stub on non-Xenon platforms.

    \Version 04/17/2009 (jrainy)
*/
/********************************************************************************F*/
void ConnApiRematch(ConnApiRefT *pConnApi)
{
}

/*F********************************************************************************/
/*!
    \Function ConnApiSetPresence

    \Description
        Stub: would switch to a presence enabled(or disabled) session, if the 
        platform had it.

    \Input *pConnApi        - pointer to module state
    \Input uPresenceEnabled - whether presence should be enabled or not

    \Version 05/17/2010 (jrainy)
*/
/********************************************************************************F*/
void ConnApiSetPresence(ConnApiRefT *pConnApi, uint8_t uPresenceEnabled)
{
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
        pClient = &pConnApi->ClientList.Clients[pConnApi->iSelf];
        VoipGroupControl(pConnApi->pVoipGroupRef, 'lusr', FALSE, pClient->UserInfo.strUniqueId);
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
    if ((pConnApi->uGameServAddr != 0) && (pConnApi->eGameServMode == CONNAPI_GAMESERV_PEERCONN))
    {
        _ConnApiDisconnectClient(pConnApi, &pConnApi->GameServer, 0, "disconnect");
        memset(&pConnApi->GameServer, 0, sizeof(pConnApi->GameServer));
    }
    else if ((pConnApi->uGameServAddr != 0) && (pConnApi->eGameServMode == CONNAPI_GAMESERV_BROADCAST))
    {
        _ConnApiTunnelFree(pConnApi, &pConnApi->GameServer);
        memset(&pConnApi->GameServer, 0, sizeof(pConnApi->GameServer));
    }

    // reset client list
    pConnApi->ClientList.iNumClients = 0;
    memset(&pConnApi->ClientList.Clients, 0, pConnApi->ClientList.iMaxClients * sizeof(ConnApiClientT));

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
            'dtim' - get demangler timeout (in milliseconds)
            'gprt' - return game port
            'gsmd' - return gameserver mode (ConnApiGameServModeE), or -1 if not using GameServer for game connections
            'gsrv' - return the game server ConnApiClientT
            'host' - returns whether hosting or not, plus copies host name to buffer
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
            'dtim' - get demangler timeout (in milliseconds)
            'gprt' - return game port
            'gsmd' - return gameserver mode (ConnApiGameServModeE), or -1 if not using GameServer for game connections
            'gsrv' - return the game server ConnApiClientT
            'host' - returns whether hosting or not, plus copies host name to buffer
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
int32_t ConnApiStatus2(ConnApiRefT *pConnApi, int32_t iSelect, void *pData, void *pBuf, int32_t iBufSize)
{
    if ((iSelect == 'cadr') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(uint32_t)))
    {
        uint8_t bLocalAddr;
        ConnApiClientT *pClientUsed;
        ConnApiClientT *pClient = (ConnApiClientT *)pData;

        *(uint32_t*)pBuf = _ConnApiGetConnectAddr(pConnApi, pClient, &bLocalAddr, CONNAPI_CONNFLAG_GAMECONN, &pClientUsed);

        if (pClientUsed->GameInfo.eStatus != CONNAPI_STATUS_ACTV)
        {
            return(-1);
        }

        return(0);
    }
    if ((iSelect == 'cbfp') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(pConnApi->pCallback[0])))
    {
        memcpy(pBuf, &(pConnApi->pCallback[0]), sizeof(pConnApi->pCallback[0]));
        return(0);
    }
    if ((iSelect == 'cbup') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(pConnApi->pUserData[0])))
    {
        memcpy(pBuf, &(pConnApi->pUserData[0]), sizeof(pConnApi->pUserData[0]));
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
        return(pConnApi->iMnglTimeout);
    }
    if (iSelect == 'gprt')
    {
        return(pConnApi->uGamePort);
    }
    if ((iSelect == 'gsmd') && (_ConnApiGameServerEnabled(pConnApi, NULL, CONNAPI_CONNFLAG_GAMECONN)))
    {
        return(pConnApi->eGameServMode);
    }
    if ((iSelect == 'gsrv') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(ConnApiClientT)))
    {
        memcpy(pBuf, &pConnApi->GameServer, sizeof(ConnApiClientT));
        return(0);
    }
    if ((iSelect == 'host') && (pBuf != NULL) && (pConnApi->ClientList.iNumClients > 0))
    {
        strnzcpy(pBuf, pConnApi->ClientList.Clients[pConnApi->iHostIndex].UserInfo.strName, iBufSize);
        return(pConnApi->iHostIndex);
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
        strnzcpy((char *)pBuf, pConnApi->strSession, iBufSize);
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
            memcpy(pBuf, &pConnApi->pProtoTunnel, sizeof(ProtoTunnelRefT*));
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
    if (iSelect == 'type')
    {
        return(pConnApi->uConnFlags);
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
            'ctim' - set connection timeout - iValue=timeout (minimum & default 10000 ms)
            'dist' - set dist ref - iValue=index of client or 'gsrv' for GameServer user, pValue=dist ref
            'dsrv' - set demangler server - pValue=pointer to demangler server name (default demangler.ea.com)
            'dtim' - set demangler timeout - iValue=timeout in milliseconds per max user
            'gprt' - set game port to use - iValue=port
            'gsid' - set gameserver id - iValue=id
            'gsrv' - set address of gameserver - iValue=address
            'gsv2' - set Game Server Connection Mode - iValue=mode
            'fbkp' - set fallback ports  iValue=fallback game port, iValu2=fallback remote tunnel port
            'lbuf' - set game link buffer size - iValue=size (default 1024)
            'minp' - set GameLink input buffer queue length - iValue=length (default 32)
            'mngl' - set demangler enable/disable - iValue=TRUE/FALSE (default TRUE)
            'mout' - set GameLink output buffer queue length - iValue=length (default 32)
            'mwid' - set GameLink max packet size - iValue=size (default 240, max 512)
            'nmsk' - set netmask used for external address comparisons - iValue=mask (default 0xffffffff)
            'peer' - set game conn peer-web enable/disable - iValue=TRUE/FALSE (default FALSE)
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
            'voig' - pass-through to VoipGroupControl()
            'voip' - pass-through to VoipControl() - iValue=VoIP iControl, iValue2= VoIP iValue
            'vprt' - set voip port to use - iValue=port
            'vset' - set voip enable/disable (default=TRUE; call before ConnApiOnline())
            '!res' - force secure address resolution to fail, to simulate p2p connection failure
        \endverbatim

    \Version 01/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ConnApiControl(ConnApiRefT *pConnApi, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iControl == 'adve')
    {
        NetPrintf(("connapi: [0x%08x] ProtoAdvt advertising %s\n", pConnApi, (iValue?"enabled":"disabled")));
        pConnApi->bDoAdvertising = iValue;
        return(0);
    }
    if (iControl == 'auto')
    {
        pConnApi->bAutoUpdate = iValue;
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
        strnzcpy(pConnApi->strDemanglerServer, (const char *)pValue, sizeof(pConnApi->strDemanglerServer));
        return(0);
    }
    if (iControl == 'dtim')
    {
        // set demangler timeout
        pConnApi->iMnglTimeout = iValue;
        if (pConnApi->pProtoMangle != NULL)
        {
            int32_t iTimeout = pConnApi->iMnglTimeout * pConnApi->ClientList.iMaxClients;
            if (iTimeout > CONNAPI_DEMANGLER_MAXTIMEOUT)
            {
                iTimeout = CONNAPI_DEMANGLER_MAXTIMEOUT;
            }
            NetPrintf(("connapi: [0x%08x] setting demangler timeout to %d\n", pConnApi, iTimeout));
            ProtoMangleControl(pConnApi->pProtoMangle, 'time', iTimeout, 0, NULL);
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
        NetPrintf(("connapi: [0x%08x] setting gameserver id=%d\n", pConnApi, iValue));
        pConnApi->uGameServId = (uint32_t)iValue;
        return(0);
    }
    if (iControl == 'gsrv')
    {
        if (pValue == NULL)
        {
            pValue = (void *)"GameServer";
        }
        NetPrintf(("connapi: [0x%08x] setting gameserver=%s/%a mode=%d\n", pConnApi, pValue, iValue, iValue2));
        pConnApi->uGameServAddr = iValue;
        pConnApi->eGameServMode = (ConnApiGameServModeE)iValue2;
        strnzcpy(pConnApi->strGameServName, pValue, sizeof(pConnApi->strGameServName));
        VoipGroupControl(pConnApi->pVoipGroupRef, 'serv', (iValue != 0) && (pConnApi->uGameServConnMode & CONNAPI_CONNFLAG_VOIPCONN), NULL);
        return(0);
    }
    if (iControl == 'gsv2')
    {
        NetPrintf(("connapi: [0x%08x] setting gameserver conn mode=%d fallback mode=%d\n", pConnApi, iValue, iValue2));
        pConnApi->uGameServConnMode = iValue;
        pConnApi->uGameServFallback = iValue2;
        return(0);
    }
    if (iControl == 'fbkp')
    {
        NetPrintf(("connapi: [0x%08x] setting fallback ports (game port = %d, tunnel port = %d)\n", pConnApi, iValue, iValue2));
        pConnApi->uFallbackGamePort = (uint16_t)iValue;
        pConnApi->uFallbackTunnelPort = (uint16_t)iValue2;
        return(0);
    }
    if (iControl == 'lbuf')
    {
        // set game link buffer size
        pConnApi->iLinkBufSize = iValue;
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
        // set demangler enable/disable
        pConnApi->bDemanglerEnabled = iValue;
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
    if (iControl == 'phxc')
    {
        NetPrintf(("connapi: [0x%08x] 'phxc' PC Host, Xenon client mode set: %d\n", pConnApi, iValue));
        pConnApi->bPcHostXenonClientMode = iValue;
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
        VoipGroupControl(pConnApi->pVoipGroupRef, 'time', iValue, NULL);
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
            VoipGroupControl(pConnApi->pVoipGroupRef, 'tunl', iValue, NULL);
        }
        if (iValue2 > 0)
        {
            pConnApi->iTunnelPort = iValue2;
        }
        if (pValue != NULL)
        {
            strnzcpy(pConnApi->strTunnelKey, pValue, sizeof(pConnApi->strTunnelKey)); 
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
    if (iControl == 'upnp')
    {
        // set demangler enable/disable
        pConnApi->bUpnpEnabled = iValue;
        return(0);
    }
    if (iControl == 'voig')
    {
        VoipGroupControl(pConnApi->pVoipGroupRef, iValue, iValue2, pValue);
        return(0);
    }
    if ((iControl == 'voip'))
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
            ConnApiControl(pConnApi, 'type', CONNAPI_CONNFLAG_GAMECONN, 0, NULL);
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
        NetPrintf(("connapi: [0x%08x] setting bFailP2PConnect to %d\n", pConnApi, iValue));
        pConnApi->bFailP2PConnect = iValue;
        return(0);
    }
    #endif

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
    _ConnApiUpdateGameLink(pConnApi);

    // update client flags
    _ConnApiUpdateClientFlags(pConnApi);

    // update connapi connections
    if (!_ConnApiGameServerEnabled(pConnApi, NULL, CONNAPI_CONNFLAG_GAMECONN) || (pConnApi->eGameServMode == CONNAPI_GAMESERV_BROADCAST))
    {
        _ConnApiUpdateConnections(pConnApi);
    }
    else
    {
        _ConnApiUpdateGameServConnection(pConnApi);
    }
    
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
