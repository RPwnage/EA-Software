/*H********************************************************************************/
/*!
    \File connapixenon.c

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

#include <xtl.h>
#include <stdio.h>
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtynames.h"
#include "DirtySDK/dirtysock/xenon/dirtysessionmanagerxenon.h"
#include "DirtySDK/dirtysock/xenon/dirtylspxenon.h"
#include "DirtySDK/game/xenon/connapixenonsessionmanager.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/prototunnel.h"
#include "DirtySDK/voip/voip.h"
#include "DirtySDK/voip/voipgroup.h"


#include "DirtySDK/game/connapi.h"

/*** Defines **********************************************************************/

//! default connapi timeout
#define CONNAPI_TIMEOUT_DEFAULT     (15*1000)

//! connapi connection timeout
#define CONNAPI_CONNTIMEOUT         (10*1000)

//! default GameLink buffer size
#define CONNAPI_LINKBUFDEFAULT      (1024)

//! max number of registered callbacks
#define CONNAPI_MAX_CALLBACKS       (8)

//! connapi client flags
#define CONNAPI_CLIENTFLAG_REMOVE   (1) // remove client from clientlist

#if DIRTYCODE_LOGGING
static const char *_ConnApiStateNames[CONNAPI_NUMSTATUSTYPES] =
{
    "CONNAPI_STATUS_INIT",
    "CONNAPI_STATUS_CONN",
    "CONNAPI_STATUS_MNGL",
    "CONNAPI_STATUS_ACTV",
    "CONNAPI_STATUS_CLSE",
    "CONNAPI_STATUS_DISC"
};
#endif

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

    //! game flags
    uint32_t                uGameFlags;

    //! game link buffer size
    int32_t                 iLinkBufSize;

    //! connapixenonsessionmanager ref
    ConnApiXenonSessionManagerRefT *pSessionManager;

    //! prototunnel ref
    ProtoTunnelRefT         *pProtoTunnel;

    //! prototunnel crypto base key
    char                    strTunnelKey[64];

    //! prototunnel port
    int32_t                 iTunnelPort;

    //! do we own tunnel ref?
    int32_t                 bTunnelOwner;

    //! voip ref
    VoipRefT                *pVoipRef;

    //! voipgroup ref
    VoipGroupRefT           *pVoipGroupRef;

    //! comm construct function
    CommAllConstructT       *pCommConstruct;

    //! our address
    DirtyAddrT              SelfAddr;

    //! index of ourself in client list
    int32_t                 iSelf;

    //! do we own voip ref?
    uint8_t                 bVoipOwner;

    //! in peer-web mode?
    uint8_t                 bPeerWeb;

    //! is voip enabled?
    uint8_t                 bVoipEnabled;

    //! should the client establish a voip connection with a player via the host.
    uint8_t                 bVoipRedirectViaHost;

    //! inside of a callback?
    uint8_t                 bInCallback;

    //! disc callback on client removal?
    uint8_t                 bRemoveCallback;

    //! auto-update enabled?
    uint8_t                 bAutoUpdate;

    //! microsoft slot count overriden by user?
    uint8_t                 bSlotOverride;

    //! tunnel enabled?
    uint8_t                 bTunnelEnabled;

    //! TRUE if performing tasks of topology host, else FALSE
    uint8_t                 bTopologyHost;
    
    //! host index in the client list
    int32_t                 iTopologyHostIndex;

    //! TRUE if performing tasks of platform host (session managment), else FALSE
    uint8_t                 bPlatformHost;
    
    //! index in the client list who will be responsible for creating the session
    int32_t                 iPlatformHostIndex;

    //! 'phxc'
    uint8_t                 bPcHostXenonClientMode;

    //! 'jinp', TRUE if join in progress is enabled
    uint8_t                 bJoinInProgress;

    //! session identifier
    int32_t                 iSessId;

    //! session flags
    int32_t                 iSessionFlags;

    //! timeout value
    int32_t                 iTimeout;

    //! gamelink configuration - input packet queue size
    int32_t                 iGameMinp;

    //! gamelink configuration - output packet queue size
    int32_t                 iGameMout;

    //! gamelink configuration - max packet width
    int32_t                 iGameMwid;

    //! gamelink configuration - unacknowledged packet window
    int32_t                 iGameUnackLimit;

    //! session information
    char                    strSession[256];

    //! nonce information
    char                    strNonce[16];

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
        ST_SESSION,         //!< waiting for session 
        ST_MIGRATE,         //!< waiting for migration 
        ST_INGAME,
        ST_DELETING,        //!< deleting session
        ST_SESSION_UPDATE,  //!< waiting for the session update after a 'stop'
    } eState; 

    uint8_t                 uSessionJoinPending;

    //! gameserver "client" (used if gameserver is enabled and in PEERCONN mode)
    ConnApiClientT          GameServer;

    //! client list - must come last in ref as it is variable length
    ConnApiClientListT      ClientList;
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function _ConnApiUserNames

    \Description
        Returns a string made of the user names of all users on the specified client.

    \Input *pConnApi    - connection manager ref
    \Input *pClientInf  - client

    \Output
        char *          - concatenated user names

    \Version 03/04/2011 (jrainy)
*/
/********************************************************************************F*/
static char* _ConnApiUserNames(ConnApiRefT *pConnApi, ConnApiClientInfoT *pClientInfo)
{
    static char sStrRet[NETCONN_MAXLOCALUSERS * 32];
    int32_t iIndex;

    sStrRet[0] = 0;

    for(iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; iIndex++)
    {
        if (pClientInfo->UserInfo[iIndex].strName[0] != 0)
        {
            if (sStrRet[0] != 0)
            {
                strcat(sStrRet, " ");
            }
            strcat(sStrRet, pClientInfo->UserInfo[iIndex].strName);
        }
    }
    return(sStrRet);
}
#endif

/*F********************************************************************************/
/*!
    \Function _ConnApiSetStatus

    \Description
        Set status of a given game or voip connection

    \Input *pConnApi    - connection manager ref
    \Input *pClient     - client being modified
    \Input *pStatus     - pointer to the status field
    \Input uStatus      - new status

    \Version 03/14/2011 (jrainy)
*/
/********************************************************************************F*/
void _ConnApiSetStatus(ConnApiRefT *pConnApi, ConnApiClientT *pClient, ConnApiConnStatusE *pStatus, uint8_t uStatus)
{
    int32_t iClient;
    char* pType = "other";

    iClient = pClient - pConnApi->ClientList.Clients;

    if (pStatus == &pClient->GameInfo.eStatus)
    {
        pType = "game";
    }
    else if (pStatus == &pClient->VoipInfo.eStatus)
    {
        pType = "voip";
    }

    NetPrintf(("connapixenon: [0x%08x] Changing client %d %s status from %s to %s\n", pConnApi, iClient, pType, _ConnApiStateNames[*pStatus], _ConnApiStateNames[uStatus]));

    *pStatus = uStatus;
}

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
    static const char *_TypeNames[CONNAPI_NUMCBTYPES] =
    {
        "CONNAPI_CBTYPE_GAMEEVENT",
        "CONNAPI_CBTYPE_DESTEVENT",
        "CONNAPI_CBTYPE_VOIPEVENT",
        "CONNAPI_CBTYPE_SESSEVENT",
        "CONNAPI_CBTYPE_RANKEVENT"
    };

    // display state change
    NetPrintf(("connapixenon: [0x%08x] client %d) [%s] %s -> %s\n", pConnApi, pCbInfo->iClientIndex, _TypeNames[pCbInfo->eType],
        _ConnApiStateNames[pCbInfo->eOldStatus], _ConnApiStateNames[pCbInfo->eNewStatus]));
    #endif
}

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function _ConnApiDisplayClientInfo

    \Description
        Debug-only function to print the given client info to debug output.

    \Input *pClient - pointer to client to display
    \Input iClient  - client index

    \Version 01/06/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiDisplayClientInfo(ConnApiClientT *pClient, int32_t iClient)
{
    int32_t iIndex;

    if (pClient->bAllocated)
    {
        NetPrintf(("connapixenon:   %d) %a <%s> iVoipConnId: %d\n", iClient, pClient->ClientInfo.uAddr, pClient->ClientInfo.DirtyAddr.strMachineAddr, pClient->iVoipConnId));
        for(iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; iIndex++)
        {
            if (pClient->ClientInfo.UserInfo[iIndex].strName[0])
            {
                NetPrintf(("connapixenon:       %s\n", pClient->ClientInfo.UserInfo[iIndex].strName));
            }
        }
    }
    else
    {
        NetPrintf(("connapixenon:   %d) empty\n", iClient));
    }
}

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

    NetPrintf(("connapixenon: clientlist display\n"));
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
    \Input *pClientInfo - pointer to user info to init client with
    \Input iClientIndex - index of client

    \Version 01/06/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiInitClient(ConnApiRefT *pConnApi, ConnApiClientT *pClient, ConnApiClientInfoT *pClientInfo, int32_t iClientIndex)
{
    // initialize new client structure and save input user info
    memset(pClient, 0, sizeof(*pClient));
    memcpy(&pClient->ClientInfo, pClientInfo, sizeof(pClient->ClientInfo));

    // initialize default voip values
    pClient->iVoipConnId = VOIP_CONNID_NONE;

    // set up remote (connect) port info
    pClient->GameInfo.uMnglPort = pConnApi->uGamePort;
    pClient->VoipInfo.uMnglPort = pConnApi->uVoipPort;

    // set up local (bind) port info
    pClient->GameInfo.uLocalPort = (pClient->ClientInfo.uLocalGamePort == 0) ? pConnApi->uGamePort : pClient->ClientInfo.uLocalGamePort;
    pClient->VoipInfo.uLocalPort = (pClient->ClientInfo.uLocalVoipPort == 0) ? pConnApi->uVoipPort : pClient->ClientInfo.uLocalVoipPort;

    // here we wipe the external address, as it is irrelevant - we need to generate a secure address instead
    // for phxc mode, do not wipe the address for the PC host as we do not want to call ConnApiXenonSessionManagerConnect() on it.
    if (!pConnApi->bPcHostXenonClientMode || iClientIndex != pConnApi->iTopologyHostIndex)
    {
        pClient->ClientInfo.uAddr = 0;
    }

    // set unique client identifier if not already supplied
    if (pClient->ClientInfo.uMachineId == 0)
    {
        pClient->ClientInfo.uMachineId = (uint32_t)iClientIndex + 1;
    }

    // copy Xuid portion of DirtyAddr to UniqueId
    if (!pConnApi->bPcHostXenonClientMode)
    {
        ds_strnzcpy(pClient->ClientInfo.strUniqueId, pClient->ClientInfo.DirtyAddr.strMachineAddr, 18);
    }
    else if (pClient->ClientInfo.strUniqueId[0] == '\0')
    {
        // create unique id from external and internal addresses if not already supplied (same as pc)
        ds_snzprintf(pClient->ClientInfo.strUniqueId, sizeof(pClient->ClientInfo.strUniqueId),
            "$%08x$%08x", pClient->ClientInfo.uAddr, pClient->ClientInfo.uLocalAddr);
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
            if ((iClient != pConnApi->iTopologyHostIndex) && (pConnApi->iTopologyHostIndex != pConnApi->iSelf) && (!pConnApi->bPeerWeb))
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

    \Version 1.0 04/08/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ConnApiFindClientByName(ConnApiClientListT *pClientList, const char *pClientName)
{
    int32_t iClientIndex;
    int32_t iUserIndex;

    // search for client by name
    for (iClientIndex = 0; iClientIndex < pClientList->iMaxClients; iClientIndex++)
    {
        for(iUserIndex = 0; iUserIndex < NETCONN_MAXLOCALUSERS; iUserIndex++)
        {
            if (!strcmp(pClientList->Clients[iClientIndex].ClientInfo.UserInfo[iUserIndex].strName, pClientName))
            {
                return(iClientIndex);
            }
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
        uint32_t            -    TRUE if GameServer is to be used, else FALSE

    \Version 1.0 02/21/2007 (jbrookes)
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
    uint32_t uMachineIdA, uMachineIdB, uTemp;

    uMachineIdA = pConnApi->ClientList.Clients[pConnApi->iSelf].ClientInfo.uMachineId;
    uMachineIdB = pClient->ClientInfo.uMachineId;
    if (uMachineIdB < uMachineIdA)
    {
        uTemp = uMachineIdA;
        uMachineIdA = uMachineIdB;
        uMachineIdB = uTemp;
    }

    ds_snzprintf(pSess, iSessSize, "%08x-%08x-%s-%08x", uMachineIdA, uMachineIdB, pSessType, pConnApi->iSessId);
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
    uint32_t uMachineIdA, uMachineIdB, uTemp;

    uMachineIdA = pConnApi->ClientList.Clients[pConnApi->iSelf].ClientInfo.uMachineId;
    uMachineIdB = pClient->ClientInfo.uMachineId;
    if (uMachineIdB < uMachineIdA)
    {
        uTemp = uMachineIdA;
        uMachineIdA = uMachineIdB;
        uMachineIdB = uTemp;
    }

    ds_snzprintf(pSess, iSessSize, "%08x-%08x-%s", uMachineIdA, uMachineIdB, pConnApi->strTunnelKey);
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

    \Output
        int32_t         - tunnel id

    \Version 12/20/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ConnApiTunnelAlloc(ConnApiRefT *pConnApi, ConnApiClientT *pClient, int32_t iClientIndex, uint32_t uRemoteAddr)
{
    ProtoTunnelInfoT TunnelInfo;
    char strKey[PROTOTUNNEL_MAXKEYLEN];

    // set up tunnel info
    memset(&TunnelInfo, 0, sizeof(TunnelInfo));
    TunnelInfo.uRemoteClientId =  pClient->ClientInfo.uMachineId;
    TunnelInfo.uRemoteAddr = uRemoteAddr;
    TunnelInfo.uRemotePort = pClient->ClientInfo.uTunnelPort;
    TunnelInfo.aRemotePortList[0] = pClient->GameInfo.uMnglPort;
    TunnelInfo.aRemotePortList[1] = pClient->VoipInfo.uMnglPort;
    TunnelInfo.aPortFlags[0] = (pConnApi->uGameTunnelFlagOverride) ? (pConnApi->uGameTunnelFlag) : (PROTOTUNNEL_PORTFLAG_ENCRYPTED|PROTOTUNNEL_PORTFLAG_AUTOFLUSH);
    TunnelInfo.uTunnelMode = PROTOTUNNEL_MODE_XENON; // tunnel mode is always XENON on XENON regardless of remote endpoint type
    NetPrintf(("connapixenon: [0x%08x] setting client %d TunnelInfo.uRemotePort TunnelInfo.uTunnelMode %d \n", pConnApi, iClientIndex, TunnelInfo.uRemotePort, TunnelInfo.uTunnelMode));

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

    \Output
        int32_t         - tunnel id

    \Version 12/20/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ConnApiVoipTunnelAlloc(ConnApiRefT *pConnApi, ConnApiClientT *pClient, int32_t iClientIndex, uint32_t uRemoteAddr)
{
    int32_t iTunnelId = 0;

    // if doing redirect via host, check for previously created tunnel for re-use
    if (pConnApi->bVoipRedirectViaHost)
    {
        iTunnelId = pConnApi->ClientList.Clients[pConnApi->iTopologyHostIndex].iTunnelId;
    }

    // if doing gameserver voip, check for previously created tunnel for re-use
    if (_ConnApiGameServerEnabled(pConnApi, NULL, CONNAPI_CONNFLAG_VOIPCONN) && (pConnApi->eGameServMode != CONNAPI_GAMESERV_BROADCAST))
    {
        iTunnelId = pConnApi->GameServer.iTunnelId;
    }
    
    // if no reused tunnel, create one    
    if (iTunnelId == 0)
    {
        iTunnelId = _ConnApiTunnelAlloc(pConnApi, pClient, iClientIndex, uRemoteAddr);
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
        if ((pConnApi->ClientList.Clients[pConnApi->iTopologyHostIndex].iTunnelId == pClient->iTunnelId) &&
            (&pConnApi->ClientList.Clients[pConnApi->iTopologyHostIndex] != pClient ))
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
        ProtoTunnelFree2(pConnApi->pProtoTunnel, pClient->iTunnelId, strKey, pClient->ClientInfo.uAddr);
    }
    pClient->iTunnelId = 0;
}

/*F********************************************************************************/
/*!
    \Function _ConnApiGetConnectAddr

    \Description
         Gets the connect address.

    \Input *pConnApi        - pointer to module state
    \Input *pClient         - pointer to client to connect to
    \Input uConnMode        - game conn or voip conn
    \Input **pClientUsedRet - where to return the pointer to the client actually used

    \Output
        uint32_t            - connect address

    \Version 02/14/2007 (danielcarter)
*/
/********************************************************************************F*/
static uint32_t _ConnApiGetConnectAddr(ConnApiRefT *pConnApi, ConnApiClientT *pClient, uint32_t uConnMode, ConnApiClientT **pClientUsedRet)
{
    uint32_t uAddr;
    ConnApiClientT *pClientUsed = NULL;

    #if DIRTYCODE_LOGGING
    const char *pConn = (uConnMode & CONNAPI_CONNFLAG_GAMECONN) ? "game" : "voip";
    #endif

    // are we using the gameserver?
    if (!_ConnApiGameServerEnabled(pConnApi, pClient, uConnMode))
    {
        if ((uConnMode == CONNAPI_CONNFLAG_VOIPCONN) && pConnApi->bVoipRedirectViaHost)
        {
            uAddr = pConnApi->ClientList.Clients[pConnApi->iTopologyHostIndex].ClientInfo.uAddr;
            pClientUsed = &pConnApi->ClientList.Clients[pConnApi->iTopologyHostIndex];
            NetPrintf(("connapixenon: [%s] using host address to connect to (or disconnect from) user '%s'\n", pConn, _ConnApiUserNames(pConnApi, &pClient->ClientInfo)));
        }
        else
        {
            uAddr = pClient->ClientInfo.uAddr;
            pClientUsed = pClient;
            NetPrintf(("connapixenon: [%s] using peer address to connect to (or disconnect from) user '%s'\n", pConn, _ConnApiUserNames(pConnApi, &pClient->ClientInfo)));
        }
    }
    else
    {
        NetPrintf(("connapixenon: [0x%08x] [%s] using game server address%sto connect to (or disconnect from) user '%s' \n", pConnApi, pConn,
            (pConnApi->uGameServFallback & uConnMode) ? " as fallback " : " ",
            _ConnApiUserNames(pConnApi, &pClient->ClientInfo)));
        uAddr = pConnApi->uGameServAddr;
        pClientUsed = &pConnApi->GameServer;
    }

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
    uint32_t uConnectAddr;

    uConnectAddr = _ConnApiGetConnectAddr(pConnApi, pClient, CONNAPI_CONNFLAG_VOIPCONN, &pClientUsed);

    // do we have a tunnel to this client?
    if (pClientUsed->iTunnelId > 0)
    {
        uint32_t uVoipPort;

        NetPrintf(("connapixenon: [0x%08x] we have a tunnel for client %d; using virtual address %a\n", pConnApi,
            iConnId, pClientUsed->iTunnelId));
        uConnectAddr = pClientUsed->iTunnelId;

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
            NetPrintf(("connapixenon: suspending voip connection to user %s:%a at %d\n", _ConnApiUserNames(pConnApi, &pClient->ClientInfo), uConnectAddr, NetTick()));
            VoipGroupSuspend(pVoipGroup, iConnId);
            break;

        case (VOIPGROUP_CBTYPE_CONNRESUME):
            NetPrintf(("connapixenon: resuming voip connection to user %s:%a at %d\n", _ConnApiUserNames(pConnApi, &pClient->ClientInfo), uConnectAddr, NetTick()));
            VoipGroupResume(pVoipGroup, iConnId, uConnectAddr, pClient->VoipInfo.uMnglPort, pClient->VoipInfo.uLocalPort, pClient->ClientInfo.uMachineId, pConnApi->iSessId);
            break;

        default:
            NetPrintf(("connapixenon: [0x%08x] critical error - unknown connection sharing event type\n", pConnApi));
            break;
    }
}

/*F********************************************************************************/
/*!
    \Function _ConnApiListening

    \Description
        Determine whether we are listening or not.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - pointer to client to connect to

    \Output
        uint32_t        - TRUE if we are listening, else FALSE.

    \Version 10/13/2005 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _ConnApiListening(ConnApiRefT *pConnApi, ConnApiClientT *pClient)
{
    uint32_t bListening;

    /* determine if we are listening or not -- due to broken Xbox 360 peer
       networking, one side must only listen and one peer must connect */
    if (pConnApi->bPeerWeb == FALSE)
    {
        // if we're client/server, server listens and clients connect
        bListening = pConnApi->bTopologyHost;
    }
    else
    {
        // if we're peer-web, compare unique ids to choose listener and connector
        bListening = strcmp(pClient->ClientInfo.strUniqueId, pConnApi->ClientList.Clients[pConnApi->iSelf].ClientInfo.strUniqueId) > 0;
    }
 
    return(bListening);
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
        int32_t         - connection flags (NEGAME_CONN_*)

    \Version 04/21/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ConnApiGetConnectParms(ConnApiRefT *pConnApi, ConnApiClientT *pClientA, char *pConnName, int32_t iNameSize)
{
    ConnApiClientT *pClientB = &pConnApi->ClientList.Clients[pConnApi->iSelf];
    uint32_t uAddrA, uAddrB, uAddrT;
    uint32_t bListening;
    int32_t iConnFlags;

    // if we're in gameserver/peerconn mode, connname is our username and we are always connecting
    if (_ConnApiGameServerEnabled(pConnApi, pClientA, CONNAPI_CONNFLAG_GAMECONN) && (pConnApi->eGameServMode == CONNAPI_GAMESERV_PEERCONN))
    {
        sprintf(pConnName, "%08x", NetConnMachineId());
        return(NETGAME_CONN_CONNECT);
    }

    // determine who listens
    bListening = _ConnApiListening(pConnApi, pClientA);

    // reference unique id strings
    uAddrA = pClientA->ClientInfo.uMachineId;
    uAddrB = pClientB->ClientInfo.uMachineId;

    /* set up parms based on whether we are listening or not.  the connection name is the
       unique address of the listener concatenated with the unique address of the connector */
    if (bListening == TRUE)
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

    if ((eType == CONNAPI_CBTYPE_GAMEEVENT) || (eType == CONNAPI_CBTYPE_VOIPEVENT) || (eType == CONNAPI_CBTYPE_DESTEVENT))
    {
        if (iClientIndex == -1)
        {
            CbInfo.pClient = &pConnApi->GameServer;
        }
        else
        {
            CbInfo.pClient = &pConnApi->ClientList.Clients[iClientIndex];
        }
    }
    else
    {
        CbInfo.pClient = NULL;
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
    NetPrintf(("connapixenon: destroying game connection to user %s: %s\n", _ConnApiUserNames(pConnApi, &pClient->ClientInfo), pReason));
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

    _ConnApiSetStatus(pConnApi, pClient, &pClient->GameInfo.eStatus, CONNAPI_STATUS_DISC);
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
    NetPrintf(("connapixenon: destroying voip connection to user %s: %s at %d\n", _ConnApiUserNames(pConnApi, &pClient->ClientInfo), pReason, NetTick()));

    if (pClient->iVoipConnId >= 0)
    {
        _ConnApiGetConnectAddr(pConnApi, pClient, CONNAPI_CONNFLAG_VOIPCONN, NULL);

        VoipGroupDisconnect(pConnApi->pVoipGroupRef, pClient->iVoipConnId);
        pClient->iVoipConnId = VOIP_CONNID_NONE;
    }

    _ConnApiSetStatus(pConnApi, pClient, &pClient->VoipInfo.eStatus, CONNAPI_STATUS_DISC);
}

/*F********************************************************************************/
/*!
    \Function _ConnApiDisconnectClient

    \Description
        Disconnect a client.

    \Input *pConnApi    - pointer to module state
    \Input *pClient     - pointer to client to disconnect
    \Input iClientIndex - index of client
    \Input *pReason     - reason for the close (for debug use)

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
    int32_t iClient, iUser;

    // make sure client count is below max
    if (iNumClients > pConnApi->ClientList.iMaxClients)
    {
        NetPrintf(("connapixenon: [0x%08x] cannot host %d clients; clamping to %d\n", pConnApi, iNumClients, pConnApi->ClientList.iMaxClients));
        iNumClients = pConnApi->ClientList.iMaxClients;
    }
    // init so we can check after setup to make sure we're in the list
    pConnApi->iSelf = -1;

    // copy input client list
    for (iClient = 0, pConnApi->ClientList.iNumClients = 0; iClient < iNumClients; iClient++)
    {
        // ref client structure
        pClient = &pConnApi->ClientList.Clients[iClient];

        // check for the presence of any name
        for (iUser = 0; iUser < NETCONN_MAXLOCALUSERS; iUser++)
        {
            if (pClientList[iClient].UserInfo[iUser].strName[0] != 0)
            {
                break;
            }
        }

        if (iUser != NETCONN_MAXLOCALUSERS)
        {
            // init client structure and copy user info
            _ConnApiInitClient(pConnApi, pClient, &pClientList[iClient], iClient);

            // remember our index in list
            if (pClient->ClientInfo.uMachineId == NetConnMachineId())
            {
                // update dirtyaddr and save ref
                memcpy(&pConnApi->SelfAddr, &pClient->ClientInfo.DirtyAddr, sizeof(pConnApi->SelfAddr));
                pConnApi->iSelf = iClient;
            }

            // increment client count
            pConnApi->ClientList.iNumClients += 1;
        }
        else
        {
            _ConnApiSetStatus(pConnApi, pClient, &pClient->GameInfo.eStatus, CONNAPI_STATUS_DISC);
            _ConnApiSetStatus(pConnApi, pClient, &pClient->VoipInfo.eStatus, CONNAPI_STATUS_DISC);
            pClient->bAllocated = FALSE;
        }
    }

    // ref local client
    pClient = &pConnApi->ClientList.Clients[pConnApi->iSelf];

    if (pConnApi->GameServer.ClientInfo.uAddr == 0)
    {
        // if gameserver, set up gameserver user in case we need it
        memcpy(&pConnApi->GameServer, pClient, sizeof(pConnApi->GameServer));
        pConnApi->GameServer.ClientInfo.uAddr = pConnApi->uGameServAddr;

        //we assume the Game Server will use UserInfo[0]
        ds_strnzcpy(pConnApi->GameServer.ClientInfo.UserInfo[0].strName, pConnApi->strGameServName, sizeof(pConnApi->GameServer.ClientInfo.UserInfo[0].strName));
        pConnApi->GameServer.ClientInfo.uMachineId = pConnApi->uGameServId;
        pConnApi->GameServer.ClientInfo.strUniqueId[0] = '\0';

        // make sure to setup gameserver properly in case it is used as a fallback
        if (pConnApi->uFallbackTunnelPort)
        {
            pConnApi->GameServer.ClientInfo.uTunnelPort = pConnApi->uFallbackTunnelPort;
            pConnApi->GameServer.ClientInfo.uLocalTunnelPort = pConnApi->uFallbackTunnelPort;
        }
        else
        {
            pConnApi->GameServer.ClientInfo.uTunnelPort = pConnApi->iTunnelPort;
            pConnApi->GameServer.ClientInfo.uLocalTunnelPort = pConnApi->iTunnelPort;
        }

        NetPrintf(("connapixenon: [0x%08x] setting GameServer ProtoTunnel port %d\n", pConnApi, pConnApi->GameServer.ClientInfo.uTunnelPort));

        if (pConnApi->uFallbackGamePort)
        {
            pConnApi->GameServer.GameInfo.uMnglPort = pConnApi->uFallbackGamePort;
            pConnApi->GameServer.GameInfo.uLocalPort = pConnApi->uFallbackGamePort;
        }
    }

    // set local user
    VoipGroupControl(pConnApi->pVoipGroupRef, 'clid', pClient->ClientInfo.uMachineId, NULL);

    // set prototunnel user
    if (pConnApi->pProtoTunnel != NULL)
    {
        ProtoTunnelControl(pConnApi->pProtoTunnel, 'clid', pClient->ClientInfo.uMachineId, 0, NULL);
    }

    // set initial client flags
    _ConnApiUpdateClientFlags(pConnApi);

    #if DIRTYCODE_LOGGING
    // make sure we're in the list
    if (pConnApi->iSelf == -1)
    {
        NetPrintf(("connapixenon: local machine %08x not found in client list\n", NetConnMachineId()));
    }
    // display client list
    _ConnApiDisplayClientList(&pConnApi->ClientList);
    #endif
}

/*F********************************************************************************/
/*!
    \Function _ConnApiInitDemangler

    \Description
        Initialize demangler.

    \Input *pConnApi    - pointer to module state

    \Version 01/12/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiInitDemangler(ConnApiRefT *pConnApi)
{
    DirtyAddrT *pHostAddr;

    // set host xuid
    pHostAddr = &pConnApi->ClientList.Clients[pConnApi->iTopologyHostIndex].ClientInfo.DirtyAddr;
    ConnApiXenonSessionManagerControl(pConnApi->pSessionManager, 'host', 0, 0, pHostAddr->strMachineAddr);

    // if client, set nonce for ranked match support
    if (!pConnApi->bPlatformHost)
    {
        ConnApiXenonSessionManagerControl(pConnApi->pSessionManager, 'nonc', 0, 0, pConnApi->strNonce);
    }
    else
    {
        NetPrintf(("connapixenon: [0x%08x] skipping 'nonc' because I'm the platform host\n", pConnApi));
        NetPrintf(("connapixenon: [0x%08x] pConnApi->bTopologyHost is %d\n", pConnApi, pConnApi->bPlatformHost));
    }

    // set session slot count?
    if (pConnApi->bSlotOverride == FALSE)
    {
        int32_t iMaxPublic, iMaxPrivate;
        if (pConnApi->uGameFlags & CONNAPI_GAMEFLAG_PRIVATE)
        {
            iMaxPublic = 0;
            iMaxPrivate = pConnApi->ClientList.iMaxClients;
        }
        else
        {
            iMaxPublic = pConnApi->ClientList.iMaxClients;
            iMaxPrivate = 0;
        }
        ConnApiXenonSessionManagerControl(pConnApi->pSessionManager, 'slot', iMaxPublic, iMaxPrivate, NULL);
    }

    // if it's a private game, disable join via presense
    if (pConnApi->uGameFlags & CONNAPI_GAMEFLAG_PRIVATE)
    {
        ConnApiXenonSessionManagerControl(pConnApi->pSessionManager, 'sflg', pConnApi->iSessionFlags | XSESSION_CREATE_JOIN_VIA_PRESENCE_DISABLED, 0, NULL);
    }
    else
    {
        ConnApiXenonSessionManagerControl(pConnApi->pSessionManager, 'sflg', pConnApi->iSessionFlags, 0, NULL);
    }
}

/*F********************************************************************************/
/*!
    \Function _ConnApiDestroyDemangler

    \Description
        Destroy demangler.

    \Input *pConnApi    - pointer to module state

    \Version 01/24/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiDestroyDemangler(ConnApiRefT *pConnApi)
{
    int32_t iClientIndex;

    // destroy the demangler
    if (pConnApi->pSessionManager != NULL)
    {
        ConnApiXenonSessionManagerDestroy(pConnApi->pSessionManager);
        pConnApi->pSessionManager = NULL;
    }

    // mark all clients as not having a secure address
    for (iClientIndex = 0; iClientIndex < pConnApi->ClientList.iMaxClients; iClientIndex++)
    {
        pConnApi->ClientList.Clients[iClientIndex].ClientInfo.uAddr = 0;
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

    \Version 1.0 06/03/2005 (jbrookes)
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
    const char *strMachineAddrs[NETCONN_MAXLOCALUSERS];
    int32_t iIndex, iCount = 0;

    // remove the clients from the Microsoft session
    for(iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; iIndex++)
    {
        if (pClient->ClientInfo.UserInfo[iIndex].DirtyAddr.strMachineAddr[0] != 0)
        {
            strMachineAddrs[iCount] = pClient->ClientInfo.UserInfo[iIndex].DirtyAddr.strMachineAddr;
            iCount++;
        }
    }

    ConnApiXenonSessionManagerControl(pConnApi->pSessionManager, 'remv', iCount, 0, (void *)strMachineAddrs);

    // disconnect them
    _ConnApiDisconnectClient(pConnApi, pClient, iClientIndex, "removal");

    eNewGameStatus = pClient->GameInfo.eStatus;
    eNewVoipStatus = pClient->VoipInfo.eStatus;

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

    \Output
        ConnApiConnStatusE      - connect status

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
        NetPrintf(("connapixenon: [%s] status=init, connection failed, using gameserver fallback for connection to user '%s'\n", pConn, _ConnApiUserNames(pConnApi, &pClient->ClientInfo)));
        pInfo->uPeerConnFailed |= uGameServConnMode;
        eStatus  = CONNAPI_STATUS_INIT;

        NetPrintf(("connapixenon: [0x%08x] [%s] game port fallback: %d -> %d\n", pConnApi, pConn, pClient->GameInfo.uMnglPort, pConnApi->uFallbackGamePort));
        pClient->GameInfo.uMnglPort = pConnApi->uFallbackGamePort;
        pClient->GameInfo.uLocalPort = pConnApi->uFallbackGamePort;

        NetPrintf(("connapixenon: freeing previous tunnel for user '%s'\n", _ConnApiUserNames(pConnApi, &pClient->ClientInfo)));
        _ConnApiTunnelFree(pConnApi, pClient);
    }
    else
    {
        NetPrintf(("connapixenon: [0x%08x] [%s] status=disc, connection failed%sfor connection to user %s\n", pConnApi, pConn,
            (pInfo->uPeerConnFailed & uGameServConnMode) ? " after gameserver fallback " : " ",
            _ConnApiUserNames(pConnApi, &pClient->ClientInfo)));
        eStatus = CONNAPI_STATUS_DISC;
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
        uint32_t uConnectAddr = _ConnApiGetConnectAddr(pConnApi, pClient, CONNAPI_CONNFLAG_GAMECONN, &pClientUsed);

        NetPrintf(("connapixenon: [0x%08x] establishing game connection to user %s:%a at %d\n", pConnApi,
            _ConnApiUserNames(pConnApi, &pClient->ClientInfo), uConnectAddr, NetTick()));

        // create tunnel?
        if ((pConnApi->bTunnelEnabled) && (pClientUsed->iTunnelId == 0))
        {
            pClientUsed->iTunnelId = _ConnApiTunnelAlloc(pConnApi, pClientUsed, iClientIndex, uConnectAddr);
        }

        // do we have a tunnel to this client?
        if (pClientUsed->iTunnelId > 0)
        {
            NetPrintf(("connapixenon: [0x%08x] tunnel allocated for client %d; switching to use virtual address %a\n", pConnApi,
                iClientIndex, pClientUsed->iTunnelId));
            uConnectAddr = pClientUsed->iTunnelId;
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
            NetGameUtilControl(pClient->pGameUtilRef, 'clid', pConnApi->ClientList.Clients[pConnApi->iSelf].ClientInfo.uMachineId);

            NetGameUtilControl(pClient->pGameUtilRef, 'rcid', pClient->ClientInfo.uMachineId);

            // determine connection parameters
            iConnFlags = _ConnApiGetConnectParms(pConnApi, pClient, strConnName, sizeof(strConnName));

            #if DIRTYCODE_DEBUG
            if ((pConnApi->bFailP2PConnect) && (uConnectAddr != pConnApi->uGameServAddr))
            {
                uConnectAddr = (127 << 24) + 1;
            }
            #endif

            ds_snzprintf(strConn, sizeof(strConn), "%a:%d:%d#%s", uConnectAddr,
                    pConnApi->uGamePort, pConnApi->uGamePort, strConnName);

            // start the connection attempt
            NetGameUtilConnect(pClient->pGameUtilRef, iConnFlags, strConn, pConnApi->pCommConstruct);
            _ConnApiSetStatus(pConnApi, pClient, &pClient->GameInfo.eStatus, CONNAPI_STATUS_CONN);

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
            if ((uConnectAddr == pConnApi->uGameServAddr) && (pConnApi->uGameServAddr != 0))
            {
                pClient->GameInfo.uConnFlags |= CONNAPI_CONNFLAG_GAMESERVER;
            }

            // remember connection start time
            pClient->iConnStart = NetTick();

            NetGameUtilControl(pClient->pGameUtilRef, 'meta', ((pConnApi->uGameServAddr != 0) && (pConnApi->eGameServMode == CONNAPI_GAMESERV_BROADCAST)) ? 1 : 0);
        }
        else
        {
            NetPrintf(("connapixenon: unable to allocate util ref for connection %d to user %s\n", iClientIndex, _ConnApiUserNames(pConnApi, &pClient->ClientInfo)));
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
                NetPrintf(("connapixenon: game connection %d to user %s established\n", iClientIndex, _ConnApiUserNames(pConnApi, &pClient->ClientInfo)));

                // set to active status                
                _ConnApiSetStatus(pConnApi, pClient, &pClient->GameInfo.eStatus, CONNAPI_STATUS_ACTV);
                pClient->GameInfo.uConnFlags |= CONNAPI_CONNFLAG_CONNECTED;
            }
            else
            {
                NetPrintf(("connapixenon: unable to allocate link ref for connection %d to user %s\n", iClientIndex, _ConnApiUserNames(pConnApi, &pClient->ClientInfo)));
                pClient->GameInfo.eStatus = CONNAPI_STATUS_DISC;
            }
        }

        // check for connection timeout
        if ((pClient->GameInfo.eStatus == CONNAPI_STATUS_CONN) && (NetTickDiff(NetTick(), pClient->iConnStart) > CONNAPI_CONNTIMEOUT))
        {
            _ConnApiDestroyGameConnection(pConnApi, pClient, iClientIndex, "connection timeout");

            // change status if fallback is enabled for game connections, otherwise, disconnect
            _ConnApiSetStatus(pConnApi, pClient, &pClient->GameInfo.eStatus, _ConnApiCheckGameServerFallback(pConnApi, pClient, CONNAPI_CONNFLAG_GAMECONN, iClientIndex));
        }
    }

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
    _ConnApiUpdateCallback(pConnApi, iClientIndex, CONNAPI_CBTYPE_GAMEEVENT, eStatus, pClient->GameInfo.eStatus);

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
    int32_t iVoipConnId;

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
        uint32_t uConnectAddr = _ConnApiGetConnectAddr(pConnApi, pClient, CONNAPI_CONNFLAG_VOIPCONN, &pClientUsed);

        NetPrintf(("connapixenon: [0x%08x] establishing voip connection to user %s:%a at %d\n", pConnApi,
            _ConnApiUserNames(pConnApi, &pClient->ClientInfo), uConnectAddr, NetTick()));

        // create tunnel?
        if ((pConnApi->bTunnelEnabled) && (pClientUsed->iTunnelId == 0))
        {
            pClientUsed->iTunnelId = _ConnApiVoipTunnelAlloc(pConnApi, pClientUsed, iClientIndex, uConnectAddr);
        }

        // do we have a tunnel to this client?
        if (pClientUsed->iTunnelId > 0)
        {
            uint32_t uVoipPort;

            NetPrintf(("connapixenon: [0x%08x] tunnel allocated for client %d; switching to use virtual address %a\n", pConnApi,
                iClientIndex, pClientUsed->iTunnelId));
            uConnectAddr = pClientUsed->iTunnelId;

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
        iVoipConnId = VoipGroupConnect(pConnApi->pVoipGroupRef, iClientIndex, uConnectAddr, pClient->VoipInfo.uMnglPort, pClient->VoipInfo.uLocalPort, pClient->ClientInfo.uMachineId, pConnApi->iSessId);
        if (iVoipConnId >= 0)
        {
            pClient->iVoipConnId = iVoipConnId;
            _ConnApiSetStatus(pConnApi, pClient, &pClient->VoipInfo.eStatus, CONNAPI_STATUS_CONN);

            // set if this is a GameServer connection or not
            if ((uConnectAddr == pConnApi->uGameServAddr) && (pConnApi->uGameServAddr != 0))
            {
                pClient->VoipInfo.uConnFlags |= CONNAPI_CONNFLAG_GAMESERVER;
            }
        }
        else
        {
            NetPrintf(("connapixenon: unable to init voip for connection %d to user %s\n", iClientIndex, _ConnApiUserNames(pConnApi, &pClient->ClientInfo)));
            pClient->VoipInfo.eStatus = CONNAPI_STATUS_DISC;
        }
    }
    
    // update connection status during connect phase
    if (pClient->VoipInfo.eStatus == CONNAPI_STATUS_CONN)
    {
        // check for established connection
        if (VoipGroupRemote(pConnApi->pVoipGroupRef, pClient->iVoipConnId) & VOIP_REMOTE_CONNECTED)
        {
            NetPrintf(("connapixenon: voip connection %d to user %s established\n", iClientIndex, _ConnApiUserNames(pConnApi, &pClient->ClientInfo)));
            pClient->VoipInfo.eStatus = CONNAPI_STATUS_ACTV;
            pClient->VoipInfo.uConnFlags |= CONNAPI_CONNFLAG_CONNECTED;
        }
    }

    // for both cases below, CONNAPI_STATUS_CONN and CONNAPI_STATUS_ACTV, when we detect a disconnection,
    // we won't set the iVoipConnId to NONE nor trigger a disconnection. This is because the voipgroup code,
    // in order to provide correct voip connection sharing, needs to be told only of authoritative "connect" 
    // and "disconnect" by the higher-level lobby (plasma, lobbysdk, blazesdk, ...) techonology.

    // check to make sure connection is still active
    if (pClient->VoipInfo.eStatus == CONNAPI_STATUS_CONN)
    {
        if (VoipGroupRemote(pConnApi->pVoipGroupRef, pClient->iVoipConnId) & VOIP_REMOTE_DISCONNECTED)
        {
            NetPrintf(("connapixenon: voip connection to user %s closed\n", _ConnApiUserNames(pConnApi, &pClient->ClientInfo)));

            // change status if fallback is enabled for voip connections, otherwise, disconnect
            _ConnApiSetStatus(pConnApi, pClient, &pClient->VoipInfo.eStatus, _ConnApiCheckGameServerFallback(pConnApi, pClient, CONNAPI_CONNFLAG_VOIPCONN, iClientIndex));
        }
    }

    if (pClient->VoipInfo.eStatus == CONNAPI_STATUS_ACTV)
    {
        if (VoipGroupRemote(pConnApi->pVoipGroupRef, pClient->iVoipConnId) & VOIP_REMOTE_DISCONNECTED)
        {
            NetPrintf(("connapixenon: voip connection to user %s closed\n", _ConnApiUserNames(pConnApi, &pClient->ClientInfo)));

            pClient->VoipInfo.eStatus = CONNAPI_STATUS_DISC;
        }
    }

    // trigger callback if state change
    _ConnApiUpdateCallback(pConnApi, iClientIndex, CONNAPI_CBTYPE_VOIPEVENT, eStatus, pClient->VoipInfo.eStatus);

    // return active or inactive
    return(pClient->VoipInfo.eStatus != CONNAPI_STATUS_DISC);
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
        NetPrintf(("connapixenon: [0x%08x] can't remove self from game\n", pConnApi));
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
        Update ConnApi module in ST_INGAME state.

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
    for (iClientIndex = 0; iClientIndex < pConnApi->ClientList.iMaxClients; iClientIndex++)
    {
        // ref connection
        if (pConnApi->bVoipRedirectViaHost)
        {
            if (iClientIndex == pConnApi->iTopologyHostIndex)
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

    // update tunnel
    if ((pConnApi->bTunnelEnabled != 0) && (pConnApi->pProtoTunnel != NULL))
    {
        ProtoTunnelUpdate(pConnApi->pProtoTunnel);    
    }

    return(iActive);
}

/*F********************************************************************************/
/*!
    \Function _ConnApiCreateSession

    \Description
        Creates or joines the session, depending on if I'am the session owner.

    \Input *pConnApi    - pointer to module state

    \Version 04/25/2008 (cvienneau)
*/
/********************************************************************************F*/
static void _ConnApiCreateSession(ConnApiRefT *pConnApi)
{
    uint32_t uSlots[NETCONN_MAXLOCALUSERS];
    int32_t iIndex;
    DirtyAddrT aDirtyAddr[NETCONN_MAXLOCALUSERS];

    for(iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; iIndex++)
    {
        ConnApiUserInfoT *pUserInfo = &pConnApi->ClientList.Clients[pConnApi->iSelf].ClientInfo.UserInfo[iIndex];

        aDirtyAddr[iIndex].strMachineAddr[0] = 0;
        uSlots[iIndex] = 0;

        if (pUserInfo->uClientFlags & CONNAPI_FLAG_PUBLICSLOT)
        {
            uSlots[iIndex] |= DIRTYSESSIONMANAGER_FLAG_PUBLICSLOT;
        }
        if (pUserInfo->uClientFlags & CONNAPI_FLAG_PRIVATESLOT)
        {
            uSlots[iIndex] |= DIRTYSESSIONMANAGER_FLAG_PRIVATESLOT;
        }

        if (pUserInfo->DirtyAddr.strMachineAddr[0])
        {
            NetPrintf(("connapixenon: [0x%08x] Creating session with local player %d\n", pConnApi, iIndex));

            memcpy(&aDirtyAddr[iIndex], &pUserInfo->DirtyAddr, sizeof(pUserInfo->DirtyAddr));
        }
    }

    if (pConnApi->iPlatformHostIndex == pConnApi->iSelf)
    {
        // create session
        NetPrintf(("CONNAPI SESSION 0x%08X creating a new session\n", pConnApi));

        ConnApiXenonSessionManagerControl(pConnApi->pSessionManager, 'clrs', 0, 0, NULL);
        if (ConnApiXenonSessionManagerCreateSess(pConnApi->pSessionManager, (pConnApi->uGameFlags & CONNAPI_GAMEFLAG_RANKED) != 0, uSlots, NULL, aDirtyAddr) == 0)
        {
            // set to hosting state
            pConnApi->eState = ST_SESSION;
            pConnApi->bPlatformHost = TRUE;
        }
    }
    else
    {
        // join session
        NetPrintf(("CONNAPI SESSION 0x%08X creating existing session %s\n", pConnApi, pConnApi->strSession));
        if (ConnApiXenonSessionManagerCreateSess(pConnApi->pSessionManager, (pConnApi->uGameFlags & CONNAPI_GAMEFLAG_RANKED) != 0, uSlots, pConnApi->strSession, aDirtyAddr) == 0)
        {
            // set state
            pConnApi->eState = ST_SESSION;
            pConnApi->bPlatformHost = FALSE;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _ConnApiUpdateSession

    \Description
        Update ConnApi module in ST_SESSION or ST_MIGRATE state.

    \Input *pConnApi    - pointer to module state

    \Version 05/12/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ConnApiUpdateSession(ConnApiRefT *pConnApi)
{
    int32_t iResult;

    // check session creation status
    if ((iResult = ConnApiXenonSessionManagerStatus(pConnApi->pSessionManager, 'sess', pConnApi->strSession, sizeof(pConnApi->strSession))) > 0)
    {
        NetPrintf(("connapixenon: [0x%08x] session creation successful %s\n", pConnApi, pConnApi->strSession));
        
        // transition to next state
        if (pConnApi->eState == ST_MIGRATE)
        {
            pConnApi->eState = ST_INGAME;
            _ConnApiUpdateClientFlags(pConnApi);
        }
        else if ((pConnApi->eState == ST_SESSION) || (pConnApi->eState == ST_SESSION_UPDATE))
        {
            pConnApi->eState = ST_INGAME;
            pConnApi->uSessionJoinPending = TRUE;
        }

        _ConnApiUpdateCallback(pConnApi, pConnApi->iPlatformHostIndex, CONNAPI_CBTYPE_SESSEVENT, CONNAPI_STATUS_INIT, CONNAPI_STATUS_ACTV);

    }
    else if (iResult < 0)
    {
        NetPrintf(("connapixenon: [0x%08x] session creation failed\n", pConnApi));
        pConnApi->eState = ST_IDLE;
    }
}

/*F********************************************************************************/
/*!
    \Function _ConnApiUpdateDemangle

    \Description
        Update ConnApi module in ST_DEMANGLE state.

    \Input *pConnApi    - pointer to module state

    \Output
        int32_t         - number of connections that are not in the DISC state

    \Version 01/06/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ConnApiUpdateDemangle(ConnApiRefT *pConnApi)
{
    ConnApiConnStatusE aOldGameStatus[CONNAPI_MAXCLIENTS], aOldVoipStatus[CONNAPI_MAXCLIENTS];
    int32_t iActive = 1, iClientIndex, iResult, iDemangled;
    ConnApiClientT *pClient;

    // make sure we're done with create first
    if (ConnApiXenonSessionManagerStatus(pConnApi->pSessionManager, 'idle', NULL, 0) == FALSE)
    {
        NetPrintf(("connapixenon: [0x%08x] waiting for async session create to complete\n", pConnApi));
        return(0);
    }

    // save entry status of all connections
    for (iClientIndex = 0; iClientIndex < pConnApi->ClientList.iMaxClients; iClientIndex++)
    {
        aOldGameStatus[iClientIndex] = pConnApi->ClientList.Clients[iClientIndex].GameInfo.eStatus;
        aOldVoipStatus[iClientIndex] = pConnApi->ClientList.Clients[iClientIndex].VoipInfo.eStatus;
    }

    // update connection list
    for (iClientIndex = 0, iDemangled = 0; iClientIndex < pConnApi->ClientList.iMaxClients; iClientIndex++)
    {
        // dereference client
        pClient = &pConnApi->ClientList.Clients[iClientIndex];

        // don't update if iClientIndex is unallocated
        // this needs to be done for self, as opposed to similar case in this file, for session management
        if (!pClient->bAllocated)
        {
            continue;
        }

        // demangle only if we don't already have a secure address
        if (pClient->ClientInfo.uAddr == 0)
        {
            // handle initial state
            if ((pClient->GameInfo.eStatus == CONNAPI_STATUS_INIT) || (pClient->VoipInfo.eStatus == CONNAPI_STATUS_INIT))
            {
                //avoid demangling the clients we don't connect to
                uint8_t uDemangle = FALSE;

                if ((pClient->uConnFlags & CONNAPI_CONNFLAG_VOIPCONN) &&  
                    (!_ConnApiGameServerEnabled(pConnApi, NULL, CONNAPI_CONNFLAG_VOIPCONN)))
                {
                    //needed for voip
                    uDemangle = TRUE;
                }

                if ((pClient->uConnFlags & CONNAPI_CONNFLAG_GAMECONN) &&  
                    (!_ConnApiGameServerEnabled(pConnApi, NULL, CONNAPI_CONNFLAG_GAMECONN)))
                {
                    //needed for game
                    uDemangle = TRUE;
                }

                if (pConnApi->uGameServFallback)
                {
                    // if we use fallback mode, we need to demangle.
                    uDemangle = TRUE;
                }

                if (uDemangle && (pClient->ClientInfo.DirtyAddr.strMachineAddr[0] != '\0'))
                {
                    if ((iResult = ConnApiXenonSessionManagerComplete(pConnApi->pSessionManager, (int32_t *)&pClient->ClientInfo.uAddr, pClient->ClientInfo.DirtyAddr.strMachineAddr)) > 0)
                    {
                        iDemangled += 1;
                    }
                    else if (iResult < 0)
                    {
                        _ConnApiSetStatus(pConnApi, pClient, &pClient->GameInfo.eStatus, CONNAPI_STATUS_DISC);
                        _ConnApiSetStatus(pConnApi, pClient, &pClient->VoipInfo.eStatus, CONNAPI_STATUS_DISC);
                    }
                    else // allow only one demangle operation at a time
                    {
                        break;
                    }
                }
                else
                {
                    //lie, though we didn't demagle the address, lets say we did
                    NetPrintf(("connapixenon: [0x%08x] Skip resolving addr for client (%d), %s %s\n",
                    pConnApi, iClientIndex,
                    !uDemangle ? "we don't connect to it," : "",
                    !pClient->ClientInfo.DirtyAddr.strMachineAddr[0] ? "it has no strMachineAddr," : ""));

                    // continue to session join for consoles.
                    if (pClient->ClientInfo.DirtyAddr.strMachineAddr[0] != '\0')
                    {
                        //set a fake address so that we don't attempt demangling again
                        pClient->ClientInfo.uAddr = (uint32_t)(-1);
                    }
                    iDemangled++;
                }
            }
        }
        else
        {
            iDemangled += 1;
        }

        // update in demangling state
        if (pConnApi->uSessionJoinPending)
        {
            // kick demangling of all users on a given machine.
            const char *strMachineAddrs[NETCONN_MAXLOCALUSERS];
            uint32_t uSlots[NETCONN_MAXLOCALUSERS];
            int32_t iIndex, iCount;

            iCount = 0;

            for(iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS;iIndex++)
            {
                if ((pClient->ClientInfo.UserInfo[iIndex].DirtyAddr.strMachineAddr[0] != 0) &&
                    (!pClient->ClientInfo.UserInfo[iIndex].bUserJoined))
                {
                    NetPrintf(("connapixenon: [0x%08x] [api] adding client index %d, user %d to session\n", pConnApi, iClientIndex, iIndex));

                    uSlots[iCount] = 0;

                    if (pClient->ClientInfo.UserInfo[iIndex].uClientFlags & CONNAPI_FLAG_PUBLICSLOT)
                    {
                        uSlots[iCount] |= DIRTYSESSIONMANAGER_FLAG_PUBLICSLOT;
                    }
                    if (pClient->ClientInfo.UserInfo[iIndex].uClientFlags & CONNAPI_FLAG_PRIVATESLOT)
                    {
                        uSlots[iCount] |= DIRTYSESSIONMANAGER_FLAG_PRIVATESLOT;
                    }

                    strMachineAddrs[iCount] = pClient->ClientInfo.UserInfo[iIndex].DirtyAddr.strMachineAddr;
                    iCount++;

                    pClient->ClientInfo.UserInfo[iIndex].bUserJoined = TRUE;
                }
            }

            if (iCount)
            {
                // start demangling
                ConnApiXenonSessionManagerConnect(pConnApi->pSessionManager, strMachineAddrs, uSlots, iCount);
            }

            // advance to mangle state
            //    _ConnApiSetStatus(pConnApi, pClient, &pClient->VoipInfo.eStatus, CONNAPI_STATUS_INIT);
            //    _ConnApiSetStatus(pConnApi, pClient, &pClient->GameInfo.eStatus, CONNAPI_STATUS_INIT);
        }
    }

    pConnApi->uSessionJoinPending = FALSE;

    // are we done?
    if (iClientIndex == pConnApi->ClientList.iMaxClients)
    {
        // did we demangle anyone?
        if (iDemangled > 0)
        {
            NetPrintf(("connapixenon: [0x%08x] resolved %d secure addresses\n", pConnApi, iDemangled));
            pConnApi->eState = ST_INGAME;
        }
        else
        {
            NetPrintf(("connapixenon: [0x%08x] unable to demangle any clients\n", pConnApi));
            pConnApi->eState = ST_IDLE;
        }
    }

    // update transitions
    for (iClientIndex = 0; iClientIndex < pConnApi->ClientList.iMaxClients; iClientIndex++)
    {
        // trigger callbacks if state changed
        pClient = &pConnApi->ClientList.Clients[iClientIndex];
        if (pClient->bAllocated)
        {
            _ConnApiUpdateCallback(pConnApi, iClientIndex, CONNAPI_CBTYPE_GAMEEVENT, aOldGameStatus[iClientIndex], pClient->GameInfo.eStatus);
            _ConnApiUpdateCallback(pConnApi, iClientIndex, CONNAPI_CBTYPE_VOIPEVENT, aOldVoipStatus[iClientIndex], pClient->VoipInfo.eStatus);
        }
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
    int32_t iClientIndex;

    // process game connection to gameserver
    if (pConnApi->ClientList.iNumClients > 0)
    {
        _ConnApiUpdateGameClient(pConnApi, &pConnApi->GameServer, -1);
    }

    // update voip connections
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
    \Function _ConnApiResetSession

    \Description
        re-estbalish new sessions for the current connapi. Used by rematch and for 
        presence changes

    \Input *pConnApi    - pointer to module state

    \Version 05/17/10 (jrainy)
*/
/********************************************************************************F*/
static void _ConnApiResetSession(ConnApiRefT *pConnApi)
{
    int32_t iClientIndex;
    ConnApiClientT *pClient;

    ConnApiXenonSessionManagerControl(pConnApi->pSessionManager, 'clrs', 0, 0, NULL);
    ConnApiXenonSessionManagerControl(pConnApi->pSessionManager, 'rest', 0, 0, pConnApi->strSession);

    // ...the nonc has to make its way too...
    if (!pConnApi->bPlatformHost)
    {
        ConnApiXenonSessionManagerControl(pConnApi->pSessionManager, 'nonc', 0, 0, pConnApi->strNonce);
    }
    else
    {
        NetPrintf(("connapixenon: [0x%08x] skipping 'nonc' because I'm the platform host\n", pConnApi));
        NetPrintf(("connapixenon: [0x%08x] pConnApi->bTopologyHost is %d\n", pConnApi, pConnApi->bTopologyHost));
    }

    // re-register client to the session.
    for (iClientIndex = 0; iClientIndex < pConnApi->ClientList.iMaxClients; iClientIndex++)
    {
        uint32_t uSlots[NETCONN_MAXLOCALUSERS];
        const char *strMachineAddrs[NETCONN_MAXLOCALUSERS];

        int32_t iIndex, iCount = 0;

        // dereference client
        pClient = &pConnApi->ClientList.Clients[iClientIndex];

        // don't update if
        //   this DOES need to be done for self, as opposed other connection updates in this file
        //   client is unallocated, or
        //   phxc mode is on and the client is the host pc
        if ( !pClient->bAllocated ||
             ((pConnApi->bPcHostXenonClientMode) && (iClientIndex == pConnApi->iTopologyHostIndex)) )
        {
            continue;
        }

        for(iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; iIndex++)
        {
            if (pClient->ClientInfo.UserInfo[iIndex].DirtyAddr.strMachineAddr[0] != 0)
            {
                strMachineAddrs[iCount] = pClient->ClientInfo.UserInfo[iIndex].DirtyAddr.strMachineAddr;
                uSlots[iCount] = 0;
                if (pClient->ClientInfo.UserInfo[iIndex].uClientFlags & CONNAPI_FLAG_PUBLICSLOT)
                {
                    uSlots[iCount] |= DIRTYSESSIONMANAGER_FLAG_PUBLICSLOT;
                }
                if (pClient->ClientInfo.UserInfo[iIndex].uClientFlags & CONNAPI_FLAG_PRIVATESLOT)
                {
                    uSlots[iCount] |= DIRTYSESSIONMANAGER_FLAG_PRIVATESLOT;
                }
                iCount++;
            }
        }
        ConnApiXenonSessionManagerConnect2(pConnApi->pSessionManager, strMachineAddrs, uSlots, iCount);
    }

    pConnApi->eState = ST_SESSION_UPDATE;
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ConnApiCreate2

    \Description
        Create the module state.

    \Input iGamePort    - port to connect over
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
    int32_t iSize;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // calculate size of module state
    iSize = sizeof(*pConnApi) + sizeof(ConnApiClientT) * iMaxClients;

    // allocate and init module state
    if ((pConnApi = DirtyMemAlloc(iSize, CONNAPI_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("connapixenon: [0x%08x] could not allocate module state... connapi initialization aborted!\n", pConnApi));
        return(NULL);
    }
    memset(pConnApi, 0, iSize);
    pConnApi->iMemGroup = iMemGroup;
    pConnApi->pMemGroupUserData = pMemGroupUserData;

    if ((pConnApi->pVoipGroupRef = VoipGroupCreate()) == NULL)
    {
        // release module memory
        DirtyMemFree(pConnApi, CONNAPI_MEMID, pConnApi->iMemGroup, pConnApi->pMemGroupUserData);

        NetPrintf(("connapixenon: [0x%08x] no more voip groups available... connapi initialization aborted!\n", pConnApi));
        return(NULL);
    }

    // register connection sharing callback with underlying voip group instance
    VoipGroupSetConnSharingEventCallback(pConnApi->pVoipGroupRef, _ConnApiVoipGroupConnSharingCallback, pConnApi);

    // create protomangle ref
    if ((pConnApi->pSessionManager = ConnApiXenonSessionManagerCreate(NULL, NULL)) == NULL)
    {
        // release newly allocated voip group
        VoipGroupDestroy(pConnApi->pVoipGroupRef);

        // release module memory
        DirtyMemFree(pConnApi, CONNAPI_MEMID, pConnApi->iMemGroup, pConnApi->pMemGroupUserData);

        NetPrintf(("connapixenon: [0x%08x] unable to create ConnApiXenonSessionManager ref... connapi initialization aborted!\n", pConnApi));
        return(NULL);
    }

    pConnApi->iSessionFlags = ConnApiXenonSessionManagerStatus(pConnApi->pSessionManager, 'sflg', NULL, 0);

    // save info
    pConnApi->uGamePort = (uint16_t)iGamePort;
    pConnApi->uVoipPort = VOIP_PORT;
    pConnApi->ClientList.iMaxClients = iMaxClients;
    pConnApi->pCallback[0] = (pCallback != NULL) ? pCallback :  _ConnApiDefaultCallback;
    pConnApi->pUserData[0] = pUserData;
    pConnApi->pCommConstruct = pConstruct;

    // set default values
    pConnApi->iLinkBufSize = CONNAPI_LINKBUFDEFAULT;
    pConnApi->iTimeout = CONNAPI_TIMEOUT_DEFAULT;
    pConnApi->iTunnelPort = 1000;
    pConnApi->uConnFlags = CONNAPI_CONNFLAG_GAMEVOIP;
    pConnApi->uGameServConnMode = CONNAPI_CONNFLAG_GAMEVOIP;
    pConnApi->uGameServFallback = 0; // default fallback to off
    pConnApi->bVoipEnabled = TRUE;
    pConnApi->bVoipRedirectViaHost = FALSE;
    pConnApi->bAutoUpdate = TRUE;

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
    // set memory grouping
    DirtyMemGroupEnter(pConnApi->iMemGroup, pConnApi->pMemGroupUserData);

    // create tunnel module
    if ((pConnApi->bTunnelEnabled) && (pConnApi->pProtoTunnel == NULL))
    {
        NetPrintf(("connapixenon: [0x%08x] Calling ProtoTunnelCreate with port %d\n", pConnApi, pConnApi->iTunnelPort));
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

    // get VoIP ref
    if ((pConnApi->bVoipEnabled) && (pConnApi->pVoipRef == NULL))
    {
        if ((pConnApi->pVoipRef = VoipGetRef()) == NULL)
        {
            NetPrintf(("connapixenon: [0x%08x] starting VoIP with iMaxClients=%d\n", pConnApi, pConnApi->ClientList.iMaxClients));
            pConnApi->pVoipRef = VoipStartup(pConnApi->ClientList.iMaxClients, 1, 0);
            pConnApi->bVoipOwner = TRUE;
        }
    }

    // set timeout for voip's benefit
    ConnApiControl(pConnApi, 'time', pConnApi->iTimeout, 0, NULL);

    // tell session manager our local dirty addr
    ConnApiXenonSessionManagerControl(pConnApi->pSessionManager, 'self', 0, 0, pSelfAddr->strMachineAddr);
    
    if (pConnApi->pVoipRef && pConnApi->bVoipOwner == TRUE)
    {
        // tell voip our local address
        VoipGroupControl(pConnApi->pVoipGroupRef, 'lusr', TRUE, pSelfAddr->strMachineAddr);
    }

    // save info
    ds_strnzcpy(pConnApi->strGameName, pGameName, sizeof(pConnApi->strGameName));
    memcpy(&pConnApi->SelfAddr, pSelfAddr, sizeof(pConnApi->SelfAddr));

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
    // if we're not disconnected yet, disconnect now
    if ((pConnApi->eState != ST_IDLE) && (pConnApi->eState != ST_DELETING))
    {
        ConnApiDisconnect(pConnApi);
    }

    // remove idle handler
    NetConnIdleDel(_ConnApiIdle, pConnApi);

    // destroy protomangle
    _ConnApiDestroyDemangler(pConnApi);

     VoipGroupDestroy(pConnApi->pVoipGroupRef);

    // destroy voip if we are the owner
    if ((pConnApi->pVoipRef != NULL) && (pConnApi->bVoipOwner == TRUE))
    {
        VoipShutdown(pConnApi->pVoipRef, 0);
    }

    // destroy tunnel, if present
    if ((pConnApi->pProtoTunnel != NULL) && (pConnApi->bTunnelOwner == TRUE))
    {
        ProtoTunnelDestroy(pConnApi->pProtoTunnel);
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
void ConnApiConnect(ConnApiRefT *pConnApi, ConnApiClientInfoT *pClientList, int32_t iClientListSize, int32_t iTopologyHostIndex, int32_t iPlatformHostIndex, int32_t iSessId, uint32_t uGameFlags)
{
    NetPrintf(("connapixenon: [0x%08x] [api] ConnApiConnect, listSize=%d, topologyHost=%d, platformHost=%d, sessionId=%d, flags=%x\n", pConnApi, iClientListSize, iTopologyHostIndex, iPlatformHostIndex, iSessId, uGameFlags));

    // make sure we're idle
    if (pConnApi->eState != ST_IDLE)
    {
        NetPrintf(("connapixenon: [0x%08x] can't connect or host a game when not in idle state\n", pConnApi));
        return;
    }

    // save session identifier
    pConnApi->iSessId = iSessId;

    //if the user is using join in progress they can NOT use ranked matches
    if ((pConnApi->bJoinInProgress) && (uGameFlags & CONNAPI_GAMEFLAG_RANKED))
    {
        //zero out the ranked bit
        uGameFlags &= ~CONNAPI_GAMEFLAG_RANKED;
        NetPrintf(("connapixenon: **********************************************************\n"));
        NetPrintf(("connapixenon: WARNING - 'Ranked' game with 'Join In Progress' requested\n" ));
        NetPrintf(("connapixenon: This feature is not supported by the Microsoft Xbox\n"));
        NetPrintf(("connapixenon: 360 SDK.  The created session will be unranked,\n"));
        NetPrintf(("connapixenon: a waiver is required to use this functionality.\n"));
        NetPrintf(("connapixenon: **********************************************************\n"));
    }

    // save game flags
    pConnApi->uGameFlags = uGameFlags;

    // init client list
    _ConnApiInitClientList(pConnApi, pClientList, iClientListSize);

    pConnApi->iTopologyHostIndex = iTopologyHostIndex;
    pConnApi->bTopologyHost = (pConnApi->iSelf == iTopologyHostIndex);

    pConnApi->iPlatformHostIndex = iPlatformHostIndex;
    pConnApi->bPlatformHost = (pConnApi->iSelf == iPlatformHostIndex);

    //if we have a pc host, get rid of its dirtyaddr, this will help us in demangling
    if (pConnApi->bPcHostXenonClientMode)
    {
        memset(pClientList[pConnApi->iTopologyHostIndex].DirtyAddr.strMachineAddr, 0, sizeof(pClientList[pConnApi->iTopologyHostIndex].DirtyAddr.strMachineAddr));       
    }

    // init demangler
    _ConnApiInitDemangler(pConnApi);

    _ConnApiCreateSession(pConnApi);
}

/*F********************************************************************************/
/*!
    \Function ConnApiMigrateTopologyHost

    \Description
        Reopen all connections, using the topology host index specified.
        This is for host migration to a different host in non-peerweb, needing new 
        connections for everyone.

    \Input *pConnApi                - pointer to module state
    \Input iNewTopologyHostIndex    - index of client to become to new topology host

    \Version 09/30/2009 (jrainy)
*/
/********************************************************************************F*/
void ConnApiMigrateTopologyHost(ConnApiRefT *pConnApi, int32_t iNewTopologyHostIndex)
{
    ConnApiClientT *pClient;
    int32_t iClientIndex;
    ConnApiConnStatusE eStatus;

    pConnApi->iTopologyHostIndex = iNewTopologyHostIndex;
    pConnApi->bTopologyHost = (pConnApi->iTopologyHostIndex == pConnApi->iSelf);
    pConnApi->eState = ST_MIGRATE;

    for (iClientIndex = 0; iClientIndex < pConnApi->ClientList.iMaxClients; iClientIndex++)
    {
        pClient = &pConnApi->ClientList.Clients[iClientIndex];

        if (pClient->bAllocated && (pClient->GameInfo.eStatus != CONNAPI_STATUS_ACTV))
        {
            eStatus = pClient->GameInfo.eStatus;
            _ConnApiSetStatus(pConnApi, pClient, &pClient->GameInfo.eStatus, CONNAPI_STATUS_INIT);
            _ConnApiUpdateCallback(pConnApi, iClientIndex, CONNAPI_CBTYPE_GAMEEVENT, eStatus, pClient->GameInfo.eStatus);
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
    \Input *pPlatformData           - platform-specific migration data

    \Version 09/30/2009 (cvienneau)
*/
/********************************************************************************F*/
void ConnApiMigratePlatformHost(ConnApiRefT *pConnApi, int32_t iNewPlatformHostIndex, void *pPlatformData)
{
    if (iNewPlatformHostIndex == pConnApi->iPlatformHostIndex)
    {
        //Microsoft won't let us migrate to ourselves.
        // This can happen if host does A->B but B dies before sending session back, and lobby re-select A.
        // Instead of Migrating we must take the existing session
        return;
    }

    // migrate the xsession host
    pConnApi->iPlatformHostIndex = iNewPlatformHostIndex;
    pConnApi->bPlatformHost = (iNewPlatformHostIndex == pConnApi->iSelf);
    pConnApi->eState = ST_MIGRATE;

    ConnApiXenonSessionManagerControl(pConnApi->pSessionManager, 'clrs', 0, 0, NULL);
    ConnApiXenonSessionManagerControl(pConnApi->pSessionManager, 'migr', pConnApi->bPlatformHost, 0, pPlatformData);
}

/*F********************************************************************************/
/*!
    \Function ConnApiAddClient

    \Description
        Add a new client to a pre-existing game.

    \Input *pConnApi    - pointer to module state
    \Input *pClientInfo - info on joining user
    \Input iClientIndex - index to add client to

    \Output
        int32_t         - 0 if successful, error code otherwise.

    \Notes
        This function should be called by all current members of a game while
        ConnApiConnect() is called by the joining client.

    \Version 01/17/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ConnApiAddClient(ConnApiRefT *pConnApi, ConnApiClientInfoT *pClientInfo, int32_t iClientIndex)
{
    ConnApiClientT *pClient;

    NetPrintf(("connapixenon: [0x%08x] [api] ConnApiAddClient %d\n", pConnApi, iClientIndex));

    // make sure we're not idle
    if (pConnApi->eState == ST_IDLE)
    {
        NetPrintf(("connapixenon: [0x%08x] can't add a connection to a game in idle state\n", pConnApi));
        return(CONNAPI_ERROR_INVALID_STATE);
    }

    // make sure there is room
    if (pConnApi->ClientList.iNumClients == pConnApi->ClientList.iMaxClients)
    {
        NetPrintf(("connapixenon: [0x%08x] can't add a connection to the game because it is full\n", pConnApi));
        return(CONNAPI_ERROR_CLIENTLIST_FULL);
    }

    // make sure the selected slot is valid
    if ((iClientIndex < 0) || (iClientIndex >= pConnApi->ClientList.iMaxClients))
    {
        NetPrintf(("connapixenon: [0x%08x] can't add a connection to the game in slot %d because valid slot range 0-%d\n", pConnApi, iClientIndex, pConnApi->ClientList.iMaxClients-1));
        return(CONNAPI_ERROR_SLOT_OUT_OF_RANGE);
    }

    // get pointer to new client structure to fill in, and increment client count
    pClient = &pConnApi->ClientList.Clients[iClientIndex];

    // check slot and make sure it is uninitialized
    if (pClient->bAllocated == TRUE)
    {
        NetPrintf(("connapixenon: [0x%08x] slot %d already allocated; cannot add a new client in this slot\n", pConnApi, iClientIndex));
        return(CONNAPI_ERROR_SLOT_USED);
    }

    // add client to list
    _ConnApiInitClient(pConnApi, pClient, pClientInfo, iClientIndex);

    // display client info
    #if DIRTYCODE_LOGGING
    NetPrintf(("connapixenon: [0x%08x] adding client to clientlist\n", pConnApi)); 
    _ConnApiDisplayClientInfo(&pConnApi->ClientList.Clients[iClientIndex], iClientIndex);
    #endif

    // increment client count
    pConnApi->ClientList.iNumClients += 1;

    // go to demangling state
    if (pConnApi->eState != ST_SESSION)
    {
        pConnApi->uSessionJoinPending = TRUE;
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function ConnApiAddUser

    \Description
        Add a new user to a specific client in a pre-existing game.

    \Input *pConnApi    - pointer to module state
    \Input iClientIndex - index of client to add user to
    \Input *pUserInfo   - info on joining user

    \Output
        int32_t         - 0 if successful, error code otherwise.

    \Version 04/07/2011 (jrainy)
*/
/********************************************************************************F*/
int32_t ConnApiAddUser(ConnApiRefT *pConnApi, int32_t iClientIndex, ConnApiUserInfoT *pUserInfo)
{
    int32_t iUserIndex;
    ConnApiClientT *pClient;

    NetPrintf(("connapixenon: [0x%08x] [api] ConnApiAddUser %d\n", pConnApi, iClientIndex));

    pClient = &pConnApi->ClientList.Clients[iClientIndex];

    for(iUserIndex = 0; iUserIndex < NETCONN_MAXLOCALUSERS; iUserIndex++)
    {
        // if we've got an empty spot for a user
        if (pClient->ClientInfo.UserInfo[iUserIndex].DirtyAddr.strMachineAddr[0] == 0)
        {
            break;
        }
    }

    if (iUserIndex == NETCONN_MAXLOCALUSERS)
    {
        NetPrintf(("connapixenon: unable to add user as client %d is already full\n", iClientIndex));
        return(-1);
    }

    NetPrintf(("connapixenon: adding user %d to client %d\n", iUserIndex, iClientIndex));

    memcpy(&pClient->ClientInfo.UserInfo[iUserIndex], pUserInfo, sizeof(pClient->ClientInfo.UserInfo[iUserIndex]));
    pConnApi->uSessionJoinPending = TRUE;

    pClient->ClientInfo.UserInfo[iUserIndex].uClientFlags = 0;
    pClient->ClientInfo.UserInfo[iUserIndex].bUserJoined = FALSE;

    return(0);
}

/*F********************************************************************************/
/*!
    \Function ConnApiFindClient

    \Description
        Returns the ConnApiClientT of a given client, if found by name.

    \Input *pConnApi    - pointer to module state
    \Input *pClientInfo - info on searched user
    \Input *pOutClient  - used to return the ClientT structure of the client

    \Output
        uint8_t         - TRUE if the client is found, FALSE otherwise

    \Version 01/30/2008 (jrainy)
*/
/********************************************************************************F*/
uint8_t ConnApiFindClient(ConnApiRefT *pConnApi, ConnApiClientInfoT *pClientInfo, ConnApiClientT *pOutClient)
{
    int32_t iClient;
    int32_t iUserIndex;

    for(iUserIndex = 0; iUserIndex < NETCONN_MAXLOCALUSERS; iUserIndex++)
    {
        if (pClientInfo->UserInfo[iUserIndex].strName[0] != 0)
        {
            if ((iClient = _ConnApiFindClientByName(&pConnApi->ClientList, pClientInfo->UserInfo[iUserIndex].strName)) >= 0)
            {
                memcpy(pOutClient, &pConnApi->ClientList.Clients[iClient], sizeof(*pOutClient));
                return(TRUE);
            }
        }
    }
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function ConnApiFindClientById

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
uint8_t ConnApiFindClientById(ConnApiRefT *pConnApi, ConnApiClientInfoT *pClientInfo, ConnApiClientT *pOutClient)
{
    int32_t iClient;

    for (iClient = 0; iClient < pConnApi->ClientList.iMaxClients; iClient++)
    {
        if (pConnApi->ClientList.Clients[iClient].ClientInfo.uMachineId == pClientInfo->uMachineId)
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
    \Input iClientIndex    - index of client to remove (used if pClientName is NULL)

    \Notes
        If this function is called inside of a ConnApi callback, the removal will
        be deferred until the next time NetConnIdle() is called.  Otherwise, the
        removal will happen immediately.

    \Version 04/08/2005 (jbrookes)
*/
/********************************************************************************F*/
void ConnApiRemoveClient(ConnApiRefT *pConnApi, int32_t iClientIndex)
{
    _ConnApiRemoveClientSetup(pConnApi, iClientIndex, CONNAPI_CLIENTFLAG_REMOVE);
}

/*F********************************************************************************/
/*!
    \Function ConnApiRemoveUser

    \Description
        Remove a user from a specific client in a game.

    \Input *pConnApi    - pointer to module state
    \Input iClientIdx   - index of client to add user to
    \Input iUserIndex   - index of the user to remove

    \Version 04/07/2011 (jrainy)
*/
/********************************************************************************F*/
void ConnApiRemoveUser(ConnApiRefT *pConnApi, int32_t iClientIdx, int32_t iUserIndex)
{
    ConnApiClientT *pClient = &pConnApi->ClientList.Clients[iClientIdx];
    char* strMachineAddr[NETCONN_MAXLOCALUSERS];

    strMachineAddr[0] = pClient->ClientInfo.UserInfo[iUserIndex].DirtyAddr.strMachineAddr;

    if (!pClient->ClientInfo.UserInfo[iUserIndex].bUserJoined)
    {
        NetPrintf(("connapixenon: skipping removal of user %d (%s) from client %d as it has not joined session yet\n", iUserIndex, strMachineAddr[0], iClientIdx));
        return;
    }

    if (strMachineAddr[0][0] != 0)
    {
        NetPrintf(("connapixenon: removing user %d (%s) from client %d\n", iUserIndex, strMachineAddr[0], iClientIdx));
        ConnApiXenonSessionManagerControl(pConnApi->pSessionManager, 'remv', 1, 0, (void *)strMachineAddr);
    }
    else
    {
        NetPrintf(("connapixenon: unable to remove user %d from client %d. It has empty strMachineAddr\n", iUserIndex, iClientIdx));
    }

    pClient->ClientInfo.UserInfo[iUserIndex].bUserJoined = FALSE;
}

/*F********************************************************************************/
/*!
    \Function ConnApiStart

    \Description
        Start game session.

    \Input *pConnApi    - pointer to module state
    \Input uStartFlags  - game start flags

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
void ConnApiStart(ConnApiRefT *pConnApi, uint32_t uStartFlags)
{
    int32_t numClientsForArbitration = pConnApi->ClientList.iNumClients;

    // make sure we're not idle
    if (pConnApi->eState != ST_INGAME)
    {
        NetPrintf(("connapixenon: [0x%08x] can't start game when not in game\n", pConnApi));
        return;
    }

    //if we're in phxc mode the PC host must not be included in the count
    if(pConnApi->bPcHostXenonClientMode)
    {
        numClientsForArbitration--;
    }

    // start game session
    ConnApiXenonSessionManagerControl(pConnApi->pSessionManager, 'strt', numClientsForArbitration, 0, NULL);

    // for debugging, output True Skill
    ConnApiXenonSessionManagerStatus(pConnApi->pSessionManager, 'skil', NULL, 0);
}

/*F********************************************************************************/
/*!
    \Function ConnApiStop

    \Description
        End game session.

    \Input *pConnApi    - pointer to module state

    \Notes
        This function is useful when it is desired to end the current game session
        but not destroy it, with the assumption that the session will be restarted
        for a new game at a later point.

    \Version 10/27/2005 (jbrookes)
*/
/********************************************************************************F*/
void ConnApiStop(ConnApiRefT *pConnApi)
{
    ConnApiCbInfoT CbInfo;
    ConnApiCbRankT RankInfo;
    int32_t iIndex, iClient;

    // make sure we're not idle
    if (pConnApi->eState == ST_IDLE)
    {
        NetPrintf(("connapixenon: [0x%08x] can't disconnect when in idle state\n", pConnApi));
        return;
    }
    // make sure we're not already disconnecting
    if (pConnApi->eState == ST_DELETING)
    {
        NetPrintf(("connapixenon: [0x%08x] can't disconnect when already disconnecting\n", pConnApi));
        return;
    }
    NetPrintf(("connapixenon: [0x%08x] getting stats for current game\n", pConnApi));

    // is the game ranked?
    if (pConnApi->uGameFlags & CONNAPI_GAMEFLAG_RANKED)
    {
        // signal session termination
        memset(&CbInfo, 0, sizeof(CbInfo));
        memset(&RankInfo, 0, sizeof(RankInfo));

        CbInfo.eType = CONNAPI_CBTYPE_RANKEVENT;
        CbInfo.eOldStatus = CONNAPI_STATUS_ACTV;
        CbInfo.eNewStatus = CONNAPI_STATUS_ACTV;
        CbInfo.TypeData.pRankData = &RankInfo;

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

        if (RankInfo.uReportValid)
        {
            for (iClient = 0; iClient < pConnApi->ClientList.iMaxClients; iClient++)
            {
                if (pConnApi->ClientList.Clients[iClient].bAllocated)
                {
                    //if either the client didn't specify a partial table,
                    //or if it did and the current client is valid
                    if ((!RankInfo.uPartialTableValid) ||
                        ((RankInfo.uPartialTableValid) && RankInfo.uScoreValid[iClient]))
                    {                
                        // skip PC host in phxc mode
                        if (!pConnApi->bPcHostXenonClientMode || (iClient != pConnApi->iTopologyHostIndex))
                        {
                            ConnApiControl(pConnApi, 'skil', iClient, RankInfo.iScore[iClient], &(RankInfo.iTeam[iClient]));
                        }
                    }
                }
            }
        }
        else
        {
            NetPrintf(("connapixenon: [0x%08x] WARNING - No stats provided for TrueSkill at end of game.\n", pConnApi));
        }

        NetPrintf(("connapixenon: [0x%08x] ending current ranked game now that stats have been submitted\n", pConnApi));
    }
    else
    {
        NetPrintf(("connapixenon: [0x%08x] ending current unranked game (no stats submission)\n", pConnApi));
    }

    // stop game session
    ConnApiXenonSessionManagerControl(pConnApi->pSessionManager, 'stop', 0, 0, NULL);
}

/*F********************************************************************************/
/*!
    \Function ConnApiRematch

    \Description
        Rematch the game session.

    \Input *pConnApi    - pointer to module state

    \Version 04/28/2009 (jrainy)
*/
/********************************************************************************F*/
void ConnApiRematch(ConnApiRefT *pConnApi)
{
    // make sure we're not idle
    if (pConnApi->eState == ST_IDLE)
    {
        NetPrintf(("connapixenon: [0x%08x] can't rematch when in idle state\n", pConnApi));
        return;
    }
    // make sure we're not already disconnecting
    if (pConnApi->eState == ST_DELETING)
    {
        NetPrintf(("connapixenon: [0x%08x] can't rematch when already disconnecting\n", pConnApi));
        return;
    }
    NetPrintf(("connapixenon: [0x%08x] rematching current game\n", pConnApi));

    _ConnApiResetSession(pConnApi);
}

/*F********************************************************************************/
/*!
    \Function ConnApiSetPresence

    \Description
        Switch to a presence enabled(or disabled) session. Two steps:
            1- Set/Reset presence flag to be used at 'next' secondary session creation,
               because the presence attribute cannot be changed after a session is created.
            2- Force a rematch to destroy current secondary session and create a new one.
        Note: The secondary session is the one that is created with or without presence
              support enabled.

    \Input *pConnApi        - pointer to module state
    \Input uPresenceEnabled - whether presence should be enabled or not

    \Version 05/17/2010 (jrainy)
*/
/********************************************************************************F*/
void ConnApiSetPresence(ConnApiRefT *pConnApi, uint8_t uPresenceEnabled)
{
    if (uPresenceEnabled != (uint8_t)ConnApiStatus(pConnApi, 'pres', NULL, 0))
    {
        ConnApiControl(pConnApi, 'pres', uPresenceEnabled, 0, NULL);

        ConnApiRematch(pConnApi);
    }
}

/*F********************************************************************************/
/*!
    \Function ConnApiDisconnect

    \Description
        Stop game, disconnect from peers, reset client list, and disconnect from
        the game server. Added so that ConnApiDisconnectPartial could be called 
        directly when connection to the game server was to be kept

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

    // make sure we're not idle
    if (pConnApi->eState == ST_IDLE)
    {
        NetPrintf(("connapixenon: [0x%08x] can't disconnect when in idle state\n", pConnApi));
        return;
    }
    // make sure we're not already disconnecting
    if (pConnApi->eState == ST_DELETING)
    {
        NetPrintf(("connapixenon: [0x%08x] can't disconnect when already disconnecting\n", pConnApi));
        return;
    }
    NetPrintf(("connapixenon: [0x%08x] disconnecting\n", pConnApi));

    if (pConnApi->pVoipRef && pConnApi->bVoipOwner == TRUE)
    {
        // clear the local user information
        VoipGroupControl(pConnApi->pVoipGroupRef, 'lusr', FALSE, pConnApi->SelfAddr.strMachineAddr);
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
        _ConnApiDisconnectClient(pConnApi, &pConnApi->GameServer, -1, "disconnect");
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

    // delete game session
    ConnApiXenonSessionManagerControl(pConnApi->pSessionManager, 'dele', 1, 0, NULL);

    // set to deleting state, to handle async session deletion
    pConnApi->eState = ST_DELETING;
}

/*F********************************************************************************/
/*!
    \Function ConnApiGetClientList

    \Description
        Get a list of current connections.

    \Input *pConnApi    - pointer to module state

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
        int32_t             - selector specific

    \Notes
        iSelect can be one of the following:

        \verbatim
            'cbfp' - returns current callback function pointer in output buffer
            'cbup' - returns current callback data pointer in output buffer
            'gprt' - returns game port
            'gsmd' - returns gameserver mode (ConnApiGameServModeE), or -1 if not using GameServer for game connections
            'gsrv' - returns the game server ConnApiClientT
            'ingm' - currently 'in game' (connecting to or connected to one or more peers)
            'lbuf' - returns current GameLink buffer allocation size
            'minp' - returns GameLink input buffer queue length (zero=default)
            'mout' - returns GameLink output buffer queue length (zero=default) 
            'mplr' - returns one past the index of the last allocated player 
            'mwid' - returns GameLink max packet size (zero=default)
            'peer' - returns game conn peer-web enable/disable status
            'self' - returns index of local user in client list
            'sess' - copies session information into output buffer
            'tprt' - returns port tunnel has been bound to (if available)
            'tunl' - returns whether tunnel is enabled or not
            'type' - returns current connection type (CONNAPI_CONNFLAG_*)
            'vprt' - returns voip port
        \endverbatim

        Any unregognized selector is forwarded to the ConnApiXenonSessionManager ref created
        internally by ConnApi, via the ConnApiXenonSessionManagerStatus() call.

    \Version 01/04/2005 (jbrookes) first version
*/
/********************************************************************************F*/
int32_t ConnApiStatus(ConnApiRefT *pConnApi, int32_t iSelect, void *pBuf, int32_t iBufSize)
{
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
    if (iSelect == 'gprt')
    {
        return(pConnApi->uGamePort);
    }
    if ((iSelect == 'gsmd') && (_ConnApiGameServerEnabled(pConnApi, NULL, CONNAPI_CONNFLAG_GAMECONN)))
    {
        return(pConnApi->eGameServMode);
    }
    if ((iSelect == 'gsrv') && (pBuf != NULL) && (iBufSize >= sizeof(ConnApiClientT)))
    {
        memcpy(pBuf, &pConnApi->GameServer, sizeof(ConnApiClientT));
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
    if (iSelect == 'mout')
    {
        return(pConnApi->iGameMwid);
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
        return(pConnApi->iGameMout);
    }
    if (iSelect == 'peer')
    {
        return(pConnApi->bPeerWeb);
    }
    if (iSelect == 'self')
    {
        return(pConnApi->iSelf);
    }
    if ((iSelect == 'sess') && (pConnApi->strSession[0] != '\0'))
    {
        ds_strnzcpy((char *)pBuf, pConnApi->strSession, iBufSize);

        NetPrintf(("CONNAPI SESSION 0x%08X session queried %s\n", pConnApi, pConnApi->strSession));

        return(0);
    }
    if (iSelect == 'sflg')
    {
        return(ConnApiXenonSessionManagerStatus(pConnApi->pSessionManager, iSelect, pBuf, iBufSize) | pConnApi->iSessionFlags);
    }
    if ((iSelect == 'tprt') && (pConnApi->pProtoTunnel != NULL))
    {
        return(ProtoTunnelStatus(pConnApi->pProtoTunnel, 'lprt', 0, NULL, 0));
    }
    if (iSelect == 'tunl')
    {
        return(pConnApi->bTunnelEnabled);
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
    // let ConnApiXenonSessionManager handle any unrecognized selectors
    return(ConnApiXenonSessionManagerStatus(pConnApi->pSessionManager, iSelect, pBuf, iBufSize));
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

    \Notes
        iControl can be one of the following:

        \verbatim
            'auto' - set auto-update enable/disable - iValue=TRUE or FALSE (default TRUE)
            'cbfp' - set callback function pointer - pValue=pCallback
            'cbup' - set callback user data pointer - pValue=pUserData
            'dist' - set dist ref - iValue=index of client or 'gsrv' for GameServer user, pValue=dist ref
            'gdsc' - disconnect game link for given client - iValue=index of client to disconnect
            'gprt' - set game port to use - iValue=port
            'gsid' - set gameserver id - iValue=id
            'gsrv' - set address of gameserver - iValue=address
            'gsv2' - set Game Server Connection Mode - iValue=mode and iValue2=fallback
            'fbkp' - set fallback ports  iValue=fallback game port, iValu2=fallback remote tunnel port
            'jinp' - stores join in progress setting and passes through to other control methods
            'lbuf' - set game link buffer size - iValue=size (default 1024)
            'minp' - set GameLink input buffer queue length - iValue=length (default 32)
            'mout' - set GameLink output buffer queue length - iValue=length (default 32)
            'mwid' - set GameLink max packet size - iValue=size (default 240, max 512)
            'nonc' - set session nonce - pValue=pointer to binary7-encoded nonce
            'peer' - set game conn peer-web enable/disable - iValue=TRUE/FALSE
            'rcbk' - set enable of disc callback on removal - iValue=TRUE/FALSE (default FALSE)
            'sess' - set session identifier - pValue=pointer to session string
            'skil' - write to skill leaderboard - iValue=index of user in client list, 
                     iValue2=user relative score, pValue=pointer to int32_t set to the team index.
            'stat' - write stats for a user - iValue=index of user in client list,
                     iValue2=number of views, pValue=XSESSION_VIEW_PROPERTIES ptr
            'stun' - set prototunnel ref
            'time' - set timeout - iValue=timeout in ms
            'tctl' - set prototunnel control data
            'tgam' - enable the override of game tunnel flags - iValue=falgs, iValue2=boolean(overrides or not)
            'tunl' - set tunnel enable - iValue = TRUE to enable and iValue2 = tunnel port
                     (or zero for default), FALSE to disable
            'type' - set connection type - iValue = CONNAPI_CONNFLAG_*
            'uses' - set the session as "utility", for uses like Lobby's usersets
            'voig' - pass-through to VoipGroupControl()
            'voip' - pass-through to VoipControl() - iValue=VoIP iControl, iValue2= VoIP iValue
            '!res' - force secure address resolution to fail, to simulate p2p connection failure
        \endverbatim

        Any unregognized selector is forwarded to the ConnApiXenonSessionManager ref created
        internally by ConnApi, via the ConnApiXenonSessionManagerControl() call.

    \Version 01/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ConnApiControl(ConnApiRefT *pConnApi, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
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
    if (iControl == 'gdsc')
    {
        // disconnect game link for given client
        if (iValue < pConnApi->ClientList.iMaxClients)
        {
            _ConnApiDestroyGameConnection(pConnApi, &pConnApi->ClientList.Clients[iValue], iValue, "user disconnect");
        }
        return(0);
    }
    if (iControl == 'gprt')
    {
        NetPrintf(("connapixenon: [0x%08x] using game port %d\n", pConnApi, iValue));
        pConnApi->uGamePort = (uint16_t)iValue;
        return(0);
    }
    if (iControl == 'gsid')
    {
        NetPrintf(("connapixenon: [0x%08x] setting gameserver id=%d\n", pConnApi, iValue));
        pConnApi->uGameServId = (uint32_t)iValue;
        return(0);        
    }
    if (iControl == 'gsrv')
    {
        if (pValue == NULL)
        {
            pValue = (void *)"GameServer";
        }
        /* if gameserver address is nonzero and we're in secure mode, remap to secure address
           so lower-layer protocols don't get confused when they get back data from a secure
               address */
        if ((iValue != 0) && (NetConnStatus('secu', 0, NULL, 0) != 0))
        {
            struct sockaddr SrcAddr, DstAddr;

            SockaddrInit(&SrcAddr, AF_INET);
            SockaddrInSetAddr(&SrcAddr, iValue);
            if (SocketControl(NULL, 'madr', 0, &SrcAddr, &DstAddr) == 0)
            {
                iValue = SockaddrInGetAddr(&DstAddr);
                NetPrintf(("connapixenon: [0x%08x] remapping GameServer addr=%a to secure addr=%a\n", pConnApi, SockaddrInGetAddr(&SrcAddr), iValue));
            }
        }
        NetPrintf(("connapixenon: [0x%08x] setting gameserver=%s/%a mode=%d\n", pConnApi, pValue, iValue, iValue2));
        pConnApi->uGameServAddr = iValue;
        pConnApi->eGameServMode = (ConnApiGameServModeE)iValue2;
        ds_strnzcpy(pConnApi->strGameServName, pValue, sizeof(pConnApi->strGameServName));
        VoipGroupControl(pConnApi->pVoipGroupRef, 'serv', (iValue != 0) && (pConnApi->uGameServConnMode & CONNAPI_CONNFLAG_VOIPCONN), NULL);
        return(0);
    }
    if (iControl == 'gsv2')
    {
        NetPrintf(("connapixenon: [0x%08x] setting gameserver conn mode=%d fallback mode=%d\n", pConnApi, iValue, iValue2));
        pConnApi->uGameServConnMode = iValue;
        pConnApi->uGameServFallback = iValue2;
        return(0);
    }
    if (iControl == 'fbkp')
    {
        NetPrintf(("connapixenon: [0x%08x] setting fallback ports (game port = %d, tunnel port = %d)\n", pConnApi, iValue, iValue2));
        pConnApi->uFallbackGamePort = (uint16_t)iValue;
        pConnApi->uFallbackTunnelPort = (uint16_t)iValue2;
        return(0);
    }
    if (iControl == 'jinp')
    {
        pConnApi->bJoinInProgress = iValue;
        //pass through
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
    if (iControl == 'nonc')
    {
        ds_strnzcpy(pConnApi->strNonce, (char *)pValue, sizeof(pConnApi->strNonce));
        return(0);
    }
    if (iControl == 'peer')
    {
        // set peer-web status
        NetPrintf(("connapixenon: [0x%08x] peerweb %s\n", pConnApi, iValue ? "enabled" : "disabled"));
        pConnApi->bPeerWeb = iValue;
        return(0);
    }
    if (iControl == 'phxc')
    {
        NetPrintf(("connapixenon: [0x%08x] 'phxc' PC Host, Xenon client mode set: %d\n", pConnApi, iValue));
        pConnApi->bPcHostXenonClientMode = iValue;
        return(0);
    }
    if (iControl == 'rcbk')
    {
        // enable disc callback on removal
        pConnApi->bRemoveCallback = iValue;
        return(0);
    }
    if (iControl == 'sess')
    {
        // set session identifier
        ds_strnzcpy(pConnApi->strSession, (char *)pValue, sizeof(pConnApi->strSession));

        NetPrintf(("CONNAPI SESSION 0x%08X session set %s\n", pConnApi, pConnApi->strSession));

        return(0);
    }
    if (iControl == 'sflg')
    {
        // remember session flags, and pass through to protomangle
        pConnApi->iSessionFlags = iValue;
    }
    if (iControl == 'skil')
    {
        // set the xuid of the user whose stats we are going to write
        ConnApiXenonSessionManagerControl(pConnApi->pSessionManager, 'suid', 0, 0, pConnApi->ClientList.Clients[iValue].ClientInfo.DirtyAddr.strMachineAddr);

        //if the team is specified, set it in iValue
        //otherwise, the players' indices are used for team.
        if (pValue)
        {
            iValue = *(int32_t*)pValue;
        }

        // fall through to let ConnApiXenonSessionManagerControl handle 'skil'
    }
    if (iControl == 'slot')
    {
        // snoop 'slot' command - note fall-through is deliberate to pass 'slot' on to ConnApiXenonSessionManager
        pConnApi->bSlotOverride = TRUE;
    }
    if (iControl == 'stat')
    {
        // set the xuid of the user whose stats we are going to write
        ConnApiXenonSessionManagerControl(pConnApi->pSessionManager, 'suid', 0, 0, pConnApi->ClientList.Clients[iValue].ClientInfo.DirtyAddr.strMachineAddr);

        // write the stats
        return(ConnApiXenonSessionManagerControl(pConnApi->pSessionManager, 'stat', iValue2, 0, pValue));
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
            ds_strnzcpy(pConnApi->strTunnelKey, pValue, sizeof(pConnApi->strTunnelKey)); 
        }
        return(0);
    }
    if (iControl == 'type')
    {
        // set connection flags (CONNAPI_CONNFLAG_*)
        NetPrintf(("connapixenon: [0x%08x] connflag change from 0x%02x to 0x%02x\n", pConnApi, pConnApi->uConnFlags, iValue));
        pConnApi->uConnFlags = iValue;
        return(0);
    }
    // set us up as a utility session,
    if (iControl == 'uses')
    {
        pConnApi->iSessionFlags = XSESSION_CREATE_GROUP_GAME;
        return(0);
    }
    if (iControl == 'ulmt')
    {
        // set gamelink unack window size 
        pConnApi->iGameUnackLimit = iValue;
        return(0);
    }
    if (iControl == 'voig')
    {
        VoipGroupControl(pConnApi->pVoipGroupRef, iValue, iValue2, pValue);
        return(0);
    }
    if (iControl == 'voip')
    {
        if(pConnApi->pVoipRef != NULL)
        {      
            VoipControl(pConnApi->pVoipRef, iValue, iValue2, pValue);
            return(0);
        }  
        
        NetPrintf(("connapixenon: [0x%08x] - WARNING - ConnApiControl(): processing of 'voip' selector failed because of an uninitialized VOIP module reference!\n", pConnApi));
    }
    if (iControl == 'vprt')
    {
        NetPrintf(("connapixenon: [0x%08x] using voip port %d\n", pConnApi, iValue));
        pConnApi->uVoipPort = (uint16_t)iValue;
        return(0);
    }
    if (iControl == 'vsrv')
    {
        NetPrintf(("connapixenon: [0x%08x] voip connections for player made via host slot = %d\n", pConnApi, iValue));
        pConnApi->bVoipRedirectViaHost = (uint8_t)iValue;
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
    #if DIRTYCODE_DEBUG
    if (iControl == '!res')
    {
        NetPrintf(("connapixenon: [0x%08x] setting bFailP2PConnect to %d\n", pConnApi, iValue));
        pConnApi->bFailP2PConnect = iValue;
        return(0);
    }
    #endif

    // let ConnApiXenonSessionManager handle any unrecognized selectors
    return(ConnApiXenonSessionManagerControl(pConnApi->pSessionManager, iControl, iValue, iValue2, pValue));
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
    // update client flags
    _ConnApiUpdateClientFlags(pConnApi);

    // update protomangle
    if (pConnApi->pSessionManager != NULL)
    {
        ConnApiXenonSessionManagerUpdate(pConnApi->pSessionManager);
    }

    if ( (pConnApi->eState != ST_IDLE) &&
         (ConnApiXenonSessionManagerStatus(pConnApi->pSessionManager, 'fail', NULL, 0) == TRUE) )
    {
        _ConnApiUpdateCallback(pConnApi, 0, CONNAPI_CBTYPE_SESSEVENT, CONNAPI_STATUS_ACTV, CONNAPI_STATUS_DISC);

        NetPrintf(("connapixenon: [0x%08x] disconnecting due to DirtySessionManager failure\n", pConnApi));

        ConnApiDisconnect(pConnApi);

        // reset to idle state
        pConnApi->eState = ST_IDLE;
        pConnApi->uSessionJoinPending = FALSE;
    }

    // update demangler in session state
    if ((pConnApi->eState == ST_SESSION) || (pConnApi->eState == ST_MIGRATE) || (pConnApi->eState == ST_SESSION_UPDATE))
    {
        _ConnApiUpdateSession(pConnApi);
    }

    // update demangler in demangling state
    if (pConnApi->uSessionJoinPending)
    {
        _ConnApiUpdateDemangle(pConnApi);
    }

    // update connections
    if (pConnApi->eState == ST_INGAME)
    {
        uint8_t bGameServEnabled = _ConnApiGameServerEnabled(pConnApi, NULL, CONNAPI_CONNFLAG_GAMECONN);
        if (!bGameServEnabled || (pConnApi->eGameServMode == CONNAPI_GAMESERV_BROADCAST))
        {
            _ConnApiUpdateConnections(pConnApi);
        }
        else
        {
            _ConnApiUpdateGameServConnection(pConnApi);
        }
        
        /* update GameServer connection to prevent it from being timed out, even if there is no traffic
           (this can happen in failover mode with only one client, as an example) */
        if (bGameServEnabled)
        {
            DirtyLSPConnectionUpdate(pConnApi->uGameServAddr);
        }
    }

    // update demangler in deleting state
    if (pConnApi->eState == ST_DELETING)
    {
        if (ConnApiXenonSessionManagerStatus(pConnApi->pSessionManager, 'idle', NULL, 0) == TRUE)
        {
            _ConnApiUpdateCallback(pConnApi, 0, CONNAPI_CBTYPE_SESSEVENT, CONNAPI_STATUS_ACTV, CONNAPI_STATUS_DISC);

            // reset to idle state
            pConnApi->eState = ST_IDLE;
        }
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

/*F********************************************************************************/
/*!
    \Function ConnApiGetSessionPattern

    \Description
        Return a string and range showing what a session string, with a specific
        first-party session in it, would look like. This is to allow an invitee
        getting the invite from the system HUD to check if a given game session string
        matches it

    \Input *pFirstPartyStruct   - first-party Session Object. Actually an XSESSION_INFO
    \Input *pFrom               - output param. The first character in the return value that has to match
    \Input *pLength             - output param. The length that has to match in the return value

    \Output
        char*                   - the correponding session string

    \Version 02/20/2011 (jrainy)
*/
/********************************************************************************F*/
char* ConnApiGetSessionPattern(const void *pFirstPartyStruct, int32_t *pFrom, int32_t *pLength)
{
    static char strSession[256];
    int32_t iLen;

    memset(strSession, 0, sizeof(strSession));

    // The +2 and logic for copying are due to how ConnApiXenonSessionManager does it.
    // We want to place the session at the same place in the buffer as it would have. 
    DirtySessionManagerEncodeSession(strSession + 2, sizeof(strSession), pFirstPartyStruct);
    iLen = strlen(strSession + 2);

    // Copy the session after the first one, as what we want is the secondary session
    memcpy(strSession + 2 + iLen, strSession + 2, iLen);

    *pFrom = iLen + 2;
    *pLength = iLen;

    return(strSession);
}
