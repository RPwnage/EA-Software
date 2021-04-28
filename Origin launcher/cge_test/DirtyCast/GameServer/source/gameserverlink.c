/*H********************************************************************************/
/*!
    \File gameserverlink.c

    \Description
        Provides optional link connections for gameservers running in <blah> mode.

    \Copyright
        Copyright (c) 2007 Electronic Arts Inc.

    \Version 02/01/2007 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/comm/commudp.h"

#include "dirtycast.h"
#include "gameserverconfig.h"
#include "gameserverlink.h"

/*** Defines **********************************************************************/

//! memid for gameserver module
#define GSLINK_MEMID            ('gslk')

//! port reserved for game traffic (virtual)
#define GSLINK_PORT             (3600)

/*** Type Definitions *************************************************************/

//! module state
struct GameServerLinkT
{
    //! module memory group
    int32_t iMemGroup;
    void *pMemGroupUserData;

    //! game port
    uint16_t uGamePort;

    //! next free virtual port
    uint16_t uVirtPort;

    //! peer connect timeout (in milliseconds)
    int32_t iPeerConnTimeout;

    //! peer idle timeout (in milliseconds)
    int32_t iPeerIdleTimeout;

    //! maximum output packet count
    uint32_t uMaxOutPkts;

    //! verbose debug level
    int32_t iVerbosity;
    int32_t iStreamId;

    //!< count of redundancy limit changes (across all clients during lifetime of gameserverlink module)
    uint32_t uTotalRlmtChangeCount;
    uint32_t uReportedRlmtChangeCount;

    // gameserverlink->gameserver functions
    GameServerLinkSendT *pSendFunc;             //!< send function pointer
    GameServerLinkDiscT *pDiscFunc;             //!< function to notify gameserver of peer disconnect
    void *pUserData;                            //!< user data for send and disc funcs

    /* callbacks to update upper layer module, e.g.
       gameserverlink-cust or gameserverdist) */
    GameServerLinkClientEventT  *pEventFunc;    //!< client event callback
    GameServerLinkUpdateClientT *pUpdateFunc;   //!< update specified client or all clients (-1)
    void *pUpdateData;                          //!< update functions user data

    //! game started status
    uint32_t bGameStarted;

    //! comm redundancy config
    float fRedundancyThreshold;                 //!< unreasonable server-to-client packet loss threshold (percentage) - When packet loss exceeds this threshold, redundancylimit is applied.
    uint32_t uRedundancyThreshold;              //!< unreasonable server-to-client packet loss threshold (in number of packets per 100000) - When packet loss exceeds this threshold, redundancylimit is applied.
    uint32_t uRedundancyLimit;                  //!< max limit (in bytes) to be used when default limit results in unreasonable packet loss
    int32_t iRedundancyTimeout;                 //!< time limit (in sec) without exceeding server-to-client packet loss threshold before backing down to default redundancy config

    //! last update of game latency
    uint32_t uLastLateTick;

    //!< latency bin definition, read in from config file
    GameServerLateBinT LateBinCfg;

    //! stat info, returned to caller of GameServerLinkUpdate()
    GameServerStatInfoT StatInfo;

    //! late info, storing per-client latency data
    GameServerLateT ClientLateInfo[32];

    //! inbound late stat
    GameServerLateT InbLate;

    //! netgamelink buffer size (default 128k)
    uint32_t uLinkBufSize;

    //! commudp unack limit
    int32_t iUnackLimit;

    //! variable-length array of clients (must come last!)
    GameServerLinkClientListT ClientList;
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _GameServerLinkPoorConnQuality

    \Description
        Categorize link quality as poor or not; used to determine if we should
        log individual client latency.

    \Input *pLateStat       - late stats to quantify
    \Input fLPackLostPct    - Remote->Local packet loss percentage
    \Input fRPackLostPct    - Local->Remote packet loss percentage
    \Input iRNakSent        - Remote->Local NAKs sent

    \Output
        uint32_t            - TRUE if connection is deemed poor; else FALSE

    \Version 11/09/2010 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _GameServerLinkPoorConnQuality(GameServerLateT *pLateStat, float fLPackLostPct, float fRPackLostPct, int32_t iRNakSent)
{
    uint32_t bPoorConnection = FALSE;  // default to good connection
    uint32_t uAvgLate = (pLateStat->uNumStat != 0) ? pLateStat->uSumLate/pLateStat->uNumStat : 0;

    // verify latency threshold
    if ((pLateStat->uMinLate > 100) || // minimum end-to-end latency > 200
        (           uAvgLate > 125) || // average end-to-end latency > 250
        (pLateStat->uMaxLate > 250))   // maximum end-to-end latency > 500
    {
        bPoorConnection = TRUE;
    }
    else
    {
        // verify packet loss threshold
        if ((fLPackLostPct > 2.5f) || (fRPackLostPct > 2.5f))
        {
            bPoorConnection = TRUE;
        }
        else
        {
            if (iRNakSent > 0)
            {
                bPoorConnection = TRUE;
            }
        }
    }

    return(bPoorConnection);
}

/*F********************************************************************************/
/*!
    \Function _GameServerLinkStreamRecv

    \Description
        Receive and rebroadcast netgamelink streams

    \Input *pStream     - the stream we receive on
    \Input iSubchan     - the sub channel we received from (client index)
    \Input iKind        - the message kind
    \Input *pBuffer     - the message itself
    \Input iLen         - the message length

    \Version 04/12/2010 (jrainy)
*/
/********************************************************************************F*/
static void _GameServerLinkStreamRecv(NetGameLinkStreamT *pStream, int32_t iSubchan, int32_t iKind, void *pBuffer, int32_t iLen)
{
    int32_t iClient, iResult;
    GameServerLinkClientT *pClient;

    GameServerLinkT *pLinkRef = (GameServerLinkT *)pStream->pRefPtr;

    for (iClient = 0; iClient < pLinkRef->ClientList.iMaxClients; iClient++)
    {
        pClient = &pLinkRef->ClientList.Clients[iClient];

        // don't send back to ourselves
        if (iClient == pStream->iRefNum)
        {
            continue;
        }

        if (pClient->eStatus == GSLINK_STATUS_ACTV)
        {
            if (pClient->pGameLinkStream)
            {
                if ((iResult = pClient->pGameLinkStream->Send(pClient->pGameLinkStream, pStream->iRefNum, iKind, pBuffer, iLen)) == iLen)
                {
                    NetPrintfVerbose((pLinkRef->iVerbosity, 2, "gameserverlink: broadcast %d byte stream packet to client %d (kind='%c%c%c%c'/%d)\n", iLen, iClient,
                        (uint8_t)(iKind>>24), (uint8_t)(iKind>>16), (uint8_t)(iKind>>8), (uint8_t)(iKind), pStream->iRefNum));
                }
                else
                {
                    NetPrintf(("gameserverlink: stream send to client %d failed (err=%d)\n", iClient, iResult));
                }
            }
            else
            {
                NetPrintf(("gameserverlink: unable to send stream packet to client %d (no ref)\n", iClient));
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _GameServerLinkFindClientById

    \Description
        Find a client by clientId in the client list.

    \Input *pLink       - module state
    \Input iClientId    - unique client identifier

    \Output
        int32_t         - negative=failure, else success

    \Version 02/01/2007 (jbrookes)
*/
/********************************************************************************F*/
static GameServerLinkClientT *_GameServerLinkFindClientById(GameServerLinkT *pLink, int32_t iClientId)
{
    GameServerLinkClientT *pClient;
    int32_t iClient;

    // search list for client
    for (iClient = 0, pClient = NULL; iClient < pLink->ClientList.iMaxClients; iClient++)
    {
        if (pLink->ClientList.Clients[iClient].iClientId == iClientId)
        {
            pClient = &pLink->ClientList.Clients[iClient];
            break;
        }
    }

    // return result to caller;
    return(pClient);
}

/*F********************************************************************************/
/*!
    \Function _GameServerLinkSend

    \Description
        Socket send callback handler to intercept sends over the virtual game port
        and forward them back to the gameserver to handle sending.

    \Input *pSocket     - socket ref
    \Input iType        - socket type
    \Input *pData       - data to send
    \Input iDataSize    - size of data to send
    \Input *pTo         - remote address
    \Input *pCallref    - gameserverlink module state

    \Output
        int32_t         - positive=handled, else unhandled

    \Version 02/01/2007 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _GameServerLinkSend(SocketT *pSocket, int32_t iType, const uint8_t *pData, int32_t iDataSize, const struct sockaddr *pTo, void *pCallref)
{
    GameServerLinkT *pLink = (GameServerLinkT *)pCallref;
    GameServerPacketT GamePacket;
    GameServerLinkClientT *pClient;
    int32_t iClient;
    uint32_t uPort;

    // only handle sends with a destination address and port
    if ((pTo == NULL) || ((uPort = SockaddrInGetPort(pTo)) == 0))
    {
        return(0);
    }

    // try and find client in list based on port
    for (iClient = 0, pClient = NULL; iClient < pLink->ClientList.iMaxClients; iClient++)
    {
        if (pLink->ClientList.Clients[iClient].uVirt == uPort)
        {
            pClient = &pLink->ClientList.Clients[iClient];
            break;
        }
    }
    // did we find a client?
    if (pClient == NULL)
    {
        NetPrintfVerbose((pLink->iVerbosity, 2, "gameserverlink: could not find client with virtual port %d in client list\n", uPort));
        return(0);
    }

    // format the packet
    ds_memclr(&GamePacket, sizeof(GamePacket));
    GamePacket.Header.uKind = 'data';
    GamePacket.Header.uCode = 0;
    GamePacket.Header.uSize = sizeof(GamePacket.Header) + sizeof(GamePacket.Packet.GameData) - sizeof(GamePacket.Packet.GameData.aData);
    GamePacket.Packet.GameData.iClientId = -1;
    GamePacket.Packet.GameData.iDstClientId = pClient->iClientId;
    GamePacket.Packet.GameData.uTimestamp = SockaddrInGetMisc(pTo);

    // copy the data
    if (iDataSize > (signed)sizeof(GamePacket.Packet.GameData.aData))
    {
        DirtyCastLogPrintf("gameserverlink: error -- packet of size %d is too large (max=%d) and is being discarded\n", iDataSize, sizeof(GamePacket.Packet.GameData.aData));
        return(iDataSize);
    }
    ds_memcpy(GamePacket.Packet.GameData.aData, pData, iDataSize);
    GamePacket.Header.uSize += iDataSize;

    // forward back to gameserver
    pLink->pSendFunc(&GamePacket, pLink->pUserData);

    // tell dirtysock we handled the send
    return(iDataSize);
}

/*F********************************************************************************/
/*!
    \Function _GameServerLinkAddClient

    \Description
        Add a client to the client list.

    \Input *pLink        - module state
    \Input iClientIndex  - client index
    \Input iClientId     - unique client identifier
    \Input iGameServerId - game console identifier for dedicated server endpoint
    \Input uClientAddr   - client address
    \Input *pTunnelKey   - key used for establishing the tunnel

    \Output
        int32_t         - negative=failure, else success

    \Version 02/01/2007 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _GameServerLinkAddClient(GameServerLinkT *pLink, int32_t iClientIndex, int32_t iClientId, int32_t iGameServerId, uint32_t uClientAddr, const char *pTunnelKey)
{
    GameServerLinkClientT *pClient;
    char strConn[64];
    char strAddrText[20];

    // validate clientindex
    if (iClientIndex >= pLink->ClientList.iMaxClients)
    {
        DirtyCastLogPrintf("gameserverlink: client index of %d is too large for max game size of %d\n", iClientIndex, pLink->ClientList.iMaxClients);
        return(-2);
    }
    if (pLink->ClientList.Clients[iClientIndex].iClientId != 0)
    {
        DirtyCastLogPrintf("gameserverlink: client index %d is already used\n", iClientIndex);
        return(-3);
    }

    // allocate and clear a new client record
    pClient = &pLink->ClientList.Clients[iClientIndex];
    ds_memclr(pClient, sizeof(*pClient));

    // create util ref
    DirtyMemGroupEnter(pLink->iMemGroup, pLink->pMemGroupUserData);
    pClient->pGameUtil = NetGameUtilCreate();
    DirtyMemGroupLeave();

    // make sure it worked
    if (pClient->pGameUtil == NULL)
    {
        DirtyCastLogPrintf("gameserverlink: error creating gameutil ref for client 0x%08x\n", iClientId);
        return(-2);
    }

    // allocate a port to virtualize
    pClient->uVirt = pLink->uVirtPort++;
    // if we've wrapped, reset to base
    if (pLink->uVirtPort == 0)
    {
        pLink->uVirtPort = GSLINK_PORT;
    }
    // make port virtual
    NetConnControl('vadd', pClient->uVirt, 0, NULL, NULL);

    // support up to max-sized packets
    NetGameUtilControl(pClient->pGameUtil, 'mwid', NETGAME_DATAPKT_MAXSIZE);
    // set commudp output buffer size
    NetGameUtilControl(pClient->pGameUtil, 'mout', pLink->uMaxOutPkts);

    // set client id
    NetGameUtilControl(pClient->pGameUtil, 'clid', iClientId);

    // set the unack limit if set
    if (pLink->iUnackLimit != 0)
    {
        NetGameUtilControl(pClient->pGameUtil, 'ulmt', pLink->iUnackLimit);
    }

    // init and add to client list
    pClient->iClientId = iClientId;
    pClient->iGameServerId = iGameServerId;
    pClient->uAddr = uClientAddr;
    ds_strnzcpy(pClient->strTunnelKey, pTunnelKey, sizeof(pClient->strTunnelKey));
    pLink->ClientList.iNumClients += 1;
    DirtyCastLogPrintfVerbose(pLink->iVerbosity, DIRTYCAST_DBGLVL_CONNINFO, "gameserverlink: adding client 0x%08x/%a at slot #%d (client count = %d)\n", iClientId, uClientAddr, iClientIndex, pLink->ClientList.iNumClients);

    // format connect string
    ds_snzprintf(strConn, sizeof(strConn), "%s:%d:%d#%x-%x", SocketInAddrGetText(pClient->uAddr, strAddrText, sizeof(strAddrText)), pClient->uVirt, pClient->uVirt, iGameServerId, pClient->iClientId);

    // start the connection attempt
    NetGameUtilConnect(pClient->pGameUtil, NETGAME_CONN_LISTEN, strConn, (CommAllConstructT *)CommUDPConstruct);
    pClient->eStatus = GSLINK_STATUS_CONN;
    // start connection timer
    pClient->uConnStart = NetTick();

    // get socket ref from util
    NetGameUtilStatus(pClient->pGameUtil, 'sock', &pClient->pSocket, sizeof(pClient->pSocket));
    if (pClient->pSocket == NULL)
    {
        DirtyCastLogPrintf("gameserverlink: error, could not acquire socket for client 0x%08x\n", pClient->iClientId);
    }

    // call upper layer to notify when new client was added
    if (pLink->pEventFunc != NULL)
    {
        pLink->pEventFunc(&pLink->ClientList, GSLINK_CLIENTEVENT_ADD, iClientIndex, pLink->pUpdateData);
    }
    return(0);
}
/*F********************************************************************************/
/*!
    \Function _GameServerLinkDscClient

    \Description
        Disconnects a client and destroys game link and util refs.

    \Input *pLink       - module state
    \Input *pClient     - client pointer
    \Input iClient      - client index

    \Version 02/01/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _GameServerLinkDscClient(GameServerLinkT *pLink, GameServerLinkClientT *pClient, int32_t iClient, int32_t iReason)
{
    //set the disconnection reason
    pLink->ClientList.Clients[iClient].uDisconnectReason = iReason;

    // call upper layer to notify them of pending disconnection
    if (iReason != GMLINK_DIRTYCAST_DELETED)
    {
        if (pLink->pEventFunc != NULL)
        {
            pLink->pEventFunc(&pLink->ClientList, GSLINK_CLIENTEVENT_DSC, iClient, pLink->pUpdateData);
        }
    }
    // destroy refs
    if (pClient->pGameLinkStream != NULL)
    {
        NetGameLinkDestroyStream(pClient->pGameLink, pClient->pGameLinkStream);
        pClient->pGameLinkStream = NULL;
    }
    if (pClient->pGameLink != NULL)
    {
        NetPrintf(("gameserverlink: deleting gamelink for client 0x%08x slot#%d\n", pClient->iClientId, iClient));
        NetGameLinkDestroy(pClient->pGameLink);
        pClient->pGameLink = NULL;
    }
    if (pClient->pGameUtil != NULL)
    {
        NetPrintf(("gameserverlink: deleting gameutil for client 0x%08x slot#%d\n", pClient->iClientId, iClient));
        NetGameUtilDestroy(pClient->pGameUtil);
        pClient->pGameUtil = NULL;
    }
    // notifiy gameserver of peer disconnect?
    if (pClient->eStatus != GSLINK_STATUS_DISC)
    {
        pLink->pDiscFunc(pClient, pLink->pUserData);
    }
    // mark as disconnected
    pClient->eStatus = GSLINK_STATUS_DISC;
}

/*F********************************************************************************/
/*!
    \Function _GameServerLinkDelClient

    \Description
        Removes a client from the client list.

    \Input *pLink       - module state
    \Input *pClient     - client pointer
    \Input iClient      - client index

    \Version 02/01/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _GameServerLinkDelClient(GameServerLinkT *pLink, GameServerLinkClientT *pClient, int32_t iClient)
{
    // ignore delete request of unallocated client
    if (pClient->eStatus == GSLINK_STATUS_FREE)
    {
        return;
    }

    DirtyCastLogPrintfVerbose(pLink->iVerbosity, DIRTYCAST_DBGLVL_CONNINFO, "gameserverlink: removing client 0x%08x/%a at slot #%d (client count = %d)\n", pClient->iClientId, pClient->uAddr, iClient, pLink->ClientList.iNumClients-1);

    // log if outstanding stall exists
    if (pClient->iStallTime > 0)
    {
        DirtyCastLogPrintf("gameserverlink: %dms stall detected on removal for client 0x%08x\n", pClient->iStallTime, pClient->iClientId);
    }

    // disconnect from client
    _GameServerLinkDscClient(pLink, pClient, iClient, GMLINK_DIRTYCAST_DELETED);

    // call upper layer to notify them of pending deletion
    if (pLink->pEventFunc != NULL)
    {
        pLink->pEventFunc(&pLink->ClientList, GSLINK_CLIENTEVENT_DEL, iClient, pLink->pUpdateData);
    }

    // unvirtualize port
    NetConnControl('vdel', pClient->uVirt, 0, NULL, NULL);

    // remove entry from client list
    ds_memclr(&pLink->ClientList.Clients[iClient], sizeof(*pClient));

    // decrement client count
    pLink->ClientList.iNumClients -= 1;
}

/*F********************************************************************************/
/*!
    \Function _GameServerLinkUpdate

    \Description
        Find a client in the client list.

    \Input *pLink       - module state
    \Input *pClient     - client to update
    \Input uCurTick     - current tick count

    \Version 02/01/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _GameServerLinkUpdate(GameServerLinkT *pLink, GameServerLinkClientT *pClient, uint32_t uCurTick)
{
    int32_t iClient = pClient - pLink->ClientList.Clients;
    char strDebugBuffer[GSLINK_DEBUGBUFFERLENGTH] = "\0";
    int32_t iElapsed;

    // in connecting state?
    if (pClient->eStatus == GSLINK_STATUS_CONN)
    {
        void *pCommRef;

        // check for established connection
        if ((pCommRef = NetGameUtilComplete(pClient->pGameUtil)) != NULL)
        {
            // create gamelink ref
            DirtyMemGroupEnter(pLink->iMemGroup, pLink->pMemGroupUserData);
            pClient->pGameLink = NetGameLinkCreate(pCommRef, FALSE, pLink->uLinkBufSize);

            if (pLink->iStreamId)
            {
                pClient->pGameLinkStream = NetGameLinkCreateStream(pClient->pGameLink, 0, pLink->iStreamId, 10000, 10000, 100);
                pClient->pGameLinkStream->Recv = _GameServerLinkStreamRecv;
                pClient->pGameLinkStream->pRefPtr = pLink;
                pClient->pGameLinkStream->iRefNum = iClient;
            }

            DirtyMemGroupLeave();

            // transition to active state
            pClient->eStatus = GSLINK_STATUS_ACTV;

            // log amount of time it took for connection to succeed
            pClient->uConnTime = NetTickDiff(uCurTick, pClient->uConnStart);
            DirtyCastLogPrintfVerbose(pLink->iVerbosity, DIRTYCAST_DBGLVL_CONNINFO, "gameserverlink: client 0x%08x at slot #%d connected after %d ms\n", pClient->iClientId, iClient, pClient->uConnTime);

            // remember connection start time
            pClient->uConnStart = uCurTick;

            // call upper layer to notify them of new connection
            if (pLink->pEventFunc != NULL)
            {
                pLink->pEventFunc(&pLink->ClientList, GSLINK_CLIENTEVENT_CONN, iClient, pLink->pUpdateData);
            }
        }

        // check for connection timeout
        if ((iElapsed = NetTickDiff(uCurTick, pClient->uConnStart)) > pLink->iPeerConnTimeout)
        {
            // connection timed out
            DirtyCastLogPrintf("gameserverlink: unable to connect to client 0x%08x after %dms\n", pClient->iClientId, iElapsed);
            // disconnect from client
            _GameServerLinkDscClient(pLink, pClient, iClient, GMLINK_DIRTYCAST_CONNECTION_FAILURE);
        }
    }

    // in active state?
    if ((pClient->eStatus == GSLINK_STATUS_ACTV) && (pLink->pUpdateFunc != NULL))
    {
        NetGameLinkStatT Stat;
        int32_t iLastRecv;

        // get link stats
        NetGameLinkStatus(pClient->pGameLink, 'stat', 0, &Stat, sizeof(NetGameLinkStatT));

        // calculate time since last packet was received
        iLastRecv = NetTickDiff(Stat.tick, Stat.rcvdlast);

        // update netgamelink
        NetGameLinkUpdate(pClient->pGameLink);

        // make sure connection is still open
        if (Stat.isopen == FALSE)
        {
            // connection closed
            DirtyCastLogPrintf("gameserverlink: connection to client 0x%08x closed\n", pClient->iClientId);
            /* the connection is closed, which happens when the client has sent us a DISC packet. due to this
               we assume that the blazeserver is going to remove this player from the game. instead of disconnecting
               and waiting for that to happen, we can just delete the client now (GMLINK_DIRTYCAST_DELETED). when
               the remove event does come it will just no-op as the client has already been removed.

               GMLINK_DIRTYCAST_DELETED fires the DisconnectedFromEndpoint on the blaze side instead of the
               ConnectionFromEndpointLost, which is called with the other disconnect cases. how this is handled allows
               it to be treated as graceful disconnect as expected. */
            _GameServerLinkDelClient(pLink, pClient, iClient);
        }
        // see if we've timed out
        else if (iLastRecv > pLink->iPeerIdleTimeout)
        {
            // connection timed out
            DirtyCastLogPrintf("gameserverlink: connection to client 0x%08x timed out after %dms\n", pClient->iClientId, iLastRecv);
            // disconnect from client
            _GameServerLinkDscClient(pLink, pClient, iClient, GMLINK_DIRTYCAST_CONNECTION_TIMEDOUT);
        }
        else
        {
            int32_t iUpdateRet = pLink->pUpdateFunc(&pLink->ClientList, iClient, pLink->pUpdateData, strDebugBuffer);
            if (iUpdateRet < 0)
            {
                // upper layer has disconnected us
                DirtyCastLogPrintf("gameserverlink: upper layer disconnection of client 0x%08x with error %d\n", pClient->iClientId, iUpdateRet);
                DirtyCastLogPrintf("gameserverlink: disconnection buffer is \"%s\"\n", strDebugBuffer);

                // disconnect from client
                _GameServerLinkDscClient(pLink, pClient, iClient, iUpdateRet);
            }
            else
            {
                // detect a stall (>1.5s without receiving packets)
                if (iLastRecv > 1500)
                {
                    pClient->iStallTime = iLastRecv;
                }
                else if (iLastRecv < pClient->iStallTime)  // report a stall
                {
                    DirtyCastLogPrintf("gameserverlink: %dms stall detected on receive for client 0x%08x\n", pClient->iStallTime, pClient->iClientId);
                    pClient->iStallTime = 0;
                }

                // update current latency and packet sent lost
                pClient->uLate = Stat.late;
                pClient->PacketLostSentStat.uLpackLost = Stat.lpacklost;
                pClient->PacketLostSentStat.uLpackSent = Stat.lpacksent;
                pClient->PacketLostSentStat.uRpackLost = Stat.rpacklost;
                pClient->PacketLostSentStat.uRpackSent = Stat.rpacksent;
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _GameServerLinkUpdateStat

    \Description
        Update stats

    \Input *pLink       - module state
    \Input *pStatInfo   - buffer to accumulate latency info into
    \Input uCurTick     - current tick count

    \Output
        uint32_t        - zero if no stats, else non-zero

    \Version 06/28/2007 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _GameServerLinkUpdateStat(GameServerLinkT *pLink, GameServerStatInfoT *pStatInfo, uint32_t uCurTick)
{
    GameServerLinkClientT *pClient;
    int32_t iClient;

    // clear previous packet lost sent info
    GameServerPacketUpdatePackLostSentStatSingle(&pStatInfo->PackLostSent, NULL);

    // init end-to-end latency accumulator
    GameServerPacketUpdateLateStats(&pStatInfo->E2eLate, NULL);
    // init latency bin accumulator
    GameServerPacketUpdateLateBinStats(&pStatInfo->LateBin, NULL);

    // accumulate latency info from all active clients
    for (iClient = 0; iClient < pLink->ClientList.iMaxClients; iClient++)
    {
        // ref client and skip them if they aren't connected
        pClient = &pLink->ClientList.Clients[iClient];
        if (pClient->eStatus != GSLINK_STATUS_ACTV)
        {
            continue;
        }
        // wait ten seconds for latency calculations to settle
        if ((signed)(uCurTick - pClient->uConnStart) < 10000)
        {
            continue;
        }

        // accumulate latency info for this client
        GameServerPacketUpdateLateStatsSingle(&pLink->ClientLateInfo[iClient], pClient->uLate);

        // accumulate latency stats
        GameServerPacketUpdateLateStatsSingle(&pStatInfo->E2eLate, pClient->uLate);

        // update latency bin info
        GameServerPacketUpdateLateBin(&pLink->LateBinCfg, &pStatInfo->LateBin, pClient->uLate);

        // accumulate packet lost sent stats
        GameServerPacketUpdatePackLostSentStatSingle(&pStatInfo->PackLostSent, &pClient->PacketLostSentStat);
    }

    // copy inbound latency accumulator
    GameServerPacketUpdateLateStats(&pStatInfo->InbLate, NULL);
    GameServerPacketUpdateLateStats(&pStatInfo->InbLate, &pLink->InbLate);
    // reset inbound latency accumulator
    GameServerPacketUpdateLateStats(&pLink->InbLate, NULL);

    // return if we generated any stats
    return(pStatInfo->E2eLate.uNumStat + pStatInfo->InbLate.uNumStat);
}

/*F********************************************************************************/
/*!
    \Function _GameServerLinkL2RPktLossDetection

    \Description
        Check if local->remote packet loss is detected

    \Input *pLink       - module state
    \Input *pClient     - client
    \Input iClientId    - client index

    \Output
        uint32_t        - TRUE if packet loss detected, FALSE otherwise

    \Version 03/20/2014 (mclouatre)
*/
/********************************************************************************F*/
static uint32_t _GameServerLinkL2RPktLossDetection(GameServerLinkT *pLink, GameServerLinkClientT *pClient, int32_t iClientId)
{
    uint32_t bPacketLossDetected = FALSE;   // default to FALSE

    if (pLink->uRedundancyThreshold != 0)
    {
        int32_t iRPackRcvd, iLPackSent, iRPackLost;
        float fRPackLostPct;
        NetGameLinkStatT Stat;

        // get link stats
        NetGameLinkStatus(pClient->pGameLink, 'stat', 0, &Stat, sizeof(NetGameLinkStatT));

        // calculate local->remote packets lost since last update
        iRPackRcvd = (int32_t)(Stat.rpackrcvd - pClient->uRPackRcvd2);
        iRPackLost = (int32_t)(Stat.rpacklost - pClient->uRPackLost2);
        iLPackSent = iRPackRcvd + iRPackLost;

        // calculate local->remote packet loss percentage
        fRPackLostPct = ((iRPackLost > 0) && (iLPackSent > 0)) ? ((float)iRPackLost * 100.0f / (float)iLPackSent) : 0.0f;

        // update packet loss counters
        pClient->uRPackRcvd2 = Stat.rpackrcvd;
        pClient->uRPackLost2 = Stat.rpacklost;

        /*
        Were there enough packets lost to flag packet loss detection?
        Note: Due to rounding errors, most floating-point numbers end up being slightly imprecise.
        Typically, an error margin is used when comparing float. Here we don't really bother doing so.
        The feature being incorrectly activated or de-activated within that error margin is not really
        a big deal.
        */
        if (fRPackLostPct > pLink->fRedundancyThreshold)
        {
            DirtyCastLogPrintfVerbose(pLink->iVerbosity, DIRTYCAST_DBGLVL_CONNINFO, "gameserverlink: pkt loss detected for clnt %d (iLPackSent=%d, iRPackRcvd=%d, fRPackLostPct=%2.5f%%, thld=%2.5f%%)\n",
                iClientId, iLPackSent, iRPackRcvd, fRPackLostPct, pLink->fRedundancyThreshold);
            bPacketLossDetected = TRUE;
        }
    }
    else
    {
        DirtyCastLogPrintfVerbose(pLink->iVerbosity, DIRTYCAST_DBGLVL_CONNINFO, "gameserverlink: faking pkt loss detection for clnt %d because redundancy threshold is 0\n", iClientId);
        bPacketLossDetected = TRUE;
    }

    return(bPacketLossDetected);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function GameServerLinkCreate

    \Description
        Create the GameServerLink module.

    \Input *pCommandTags        - command-line config overrides
    \Input *pConfigTags         - config parameters
    \Input iMaxClients          - maximum number of clients this module will handle
    \Input iVerbosity           - debug level
    \Input uRedundancyThreshold - redundancy threshold
    \Input uRedundancyLimit     - redundancy limit
    \Input iRedundancyTimeout   - redundancy timeout
    \Input *pSendFunc           - send function to send data back to gameserver with
    \Input *pDiscFunc           - disconnection function used for peer disconnection notification
    \Input *pUserData           - user ref for send and disc funcs

    \Output
        GameServerLinkT *       - module state, or NULL if module creation failed;

    \Version 02/01/2007 (jbrookes)
*/
/********************************************************************************F*/
GameServerLinkT *GameServerLinkCreate(const char *pCommandTags, const char *pConfigTags, int32_t iMaxClients, int32_t iVerbosity, uint32_t uRedundancyThreshold,
                                      uint32_t uRedundancyLimit, int32_t iRedundancyTimeout, GameServerLinkSendT *pSendFunc, GameServerLinkDiscT *pDiscFunc, void *pUserData)
{
    GameServerLinkT *pLink;
    int32_t iMemSize;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    char strStream[5];

    // Query mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // calculate memory size based on number of clients
    iMemSize = sizeof(*pLink) + (sizeof(pLink->ClientList.Clients[0]) * (iMaxClients - 1));

    // allocate and init module state
    if ((pLink = (GameServerLinkT *)DirtyMemAlloc(iMemSize, GSLINK_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        DirtyCastLogPrintf("gameserverlink: could not allocate module state\n");
        return(NULL);
    }
    ds_memclr(pLink, iMemSize);
    pLink->iMemGroup = iMemGroup;
    pLink->pMemGroupUserData = pMemGroupUserData;

    // init other module state
    pLink->uVirtPort = GSLINK_PORT;
    pLink->pSendFunc = pSendFunc;
    pLink->pDiscFunc = pDiscFunc;
    pLink->pUserData = pUserData;
    pLink->ClientList.iMaxClients = iMaxClients;
    pLink->iPeerConnTimeout = (signed)DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.conntimeout", 10) * 1000;
    pLink->iPeerIdleTimeout = (signed)DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.peertimeout", 15) * 1000;
    pLink->uMaxOutPkts = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.maxoutpkts", 3*32);
    pLink->iVerbosity = iVerbosity;
    pLink->uLinkBufSize = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.linkbufsize", 128*1024);
    pLink->iUnackLimit = (signed)DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.unacklimit", 3*1024);
    pLink->uLastLateTick = NetTick();

    pLink->uRedundancyThreshold = uRedundancyThreshold;
    pLink->fRedundancyThreshold = (float)pLink->uRedundancyThreshold * 100.0f / 100000.0f;
    pLink->uRedundancyLimit = uRedundancyLimit;
    pLink->iRedundancyTimeout = iRedundancyTimeout;

    // read stream ident
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "gameserver.linkstream", strStream, sizeof(strStream), "");
    if (strStream[0] != '\0')
    {
        DirtyCastLogPrintf("gameserverlink: allocating link stream with id '%s'\n", strStream);
        pLink->iStreamId = (strStream[0] << 24) | (strStream[1] << 16) | (strStream[2] << 8) | strStream[3];
    }
    else
    {
        pLink->iStreamId = 0;
    }

    // get latency bin configuration
    GameServerPacketReadLateBinConfig(&pLink->LateBinCfg, pCommandTags, pConfigTags, "gameserver.latebins");

    // log module start and params
    DirtyCastLogPrintf("gameserverlink: starting (conntimeout=%dms, idletimeout=%dms, maxoutpkts=%d, redundancylimit=%d, redundancythreshold=%d, redundancytimeout=%d)\n",
                       pLink->iPeerConnTimeout, pLink->iPeerIdleTimeout, pLink->uMaxOutPkts, pLink->uRedundancyLimit,  pLink->uRedundancyThreshold, pLink->iRedundancyTimeout);

    /* set global DirtySock send callback to intercept NetGame sends and send
       them through the gameserver instead of directly */
    NetConnControl('sdcb', TRUE, 0, (void *)_GameServerLinkSend, pLink);

    // return module ref to caller
    return(pLink);
}

/*F********************************************************************************/
/*!
    \Function GameServerLinkCallback

    \Description
        Set update callbacks.

    \Input *pLink       - module state
    \Input *pEventFunc  - clientlist update func
    \Input *pUpdateFunc - client update function
    \Input *pStartFunc  - start game callback
    \Input *pStopFunc   - stop game callback
    \Input *pUpdateData - user ref for update functions

    \Version 02/01/2007 (jbrookes)
*/
/********************************************************************************F*/
void GameServerLinkCallback(GameServerLinkT *pLink, GameServerLinkClientEventT *pEventFunc, GameServerLinkUpdateClientT *pUpdateFunc, void *pUpdateData)
{
    pLink->pEventFunc = pEventFunc;
    pLink->pUpdateFunc = pUpdateFunc;
    pLink->pUpdateData = pUpdateData;
}

/*F********************************************************************************/
/*!
    \Function GameServerLinkDestroy

    \Description
        Destroy the GameServerLink module.

    \Input *pLink   - module state

    \Version 02/01/2007 (jbrookes)
*/
/********************************************************************************F*/
void GameServerLinkDestroy(GameServerLinkT *pLink)
{
    // delete any remaining clients
    GameServerLinkDelClient(pLink, -1);

    // clear the global DirtySock send callback
    NetConnControl('sdcb', FALSE, 0, (void *)_GameServerLinkSend, pLink);

    // release module state
    DirtyMemFree(pLink, GSLINK_MEMID, pLink->iMemGroup, pLink->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function GameServerLinkAddClient

    \Description
        Add a new client

    \Input *pLink        - module state
    \Input iClientIndex  - client index
    \Input iClientId     - unique client identifier
    \Input iGameServerId - game console identifier for dedicated server endpoint
    \Input uClientAddr   - client address
    \Input *pTunnelKey   - key used for establishing the tunnel

    \Output
        int32_t         - negative=failure, else success

    \Version 02/01/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t GameServerLinkAddClient(GameServerLinkT *pLink, int32_t iClientIndex, int32_t iClientId, int32_t iGameServerId, uint32_t uClientAddr, const char *pTunnelKey)
{
    GameServerLinkClientT *pClient;
    int32_t iResult;

    // make sure we're not adding a duplicate
    if (((pClient = _GameServerLinkFindClientById(pLink, iClientId)) != NULL) && (pClient->eStatus != GSLINK_STATUS_DISC))
    {
        DirtyCastLogPrintf("gameserverlink: attempt to add client with duplicate client 0x%08x ignored\n", iClientId);
        return(-1);
    }

    // add 'em to client list
    iResult = _GameServerLinkAddClient(pLink, iClientIndex, iClientId, iGameServerId, uClientAddr, pTunnelKey);

    // return add result
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function GameServerLinkDelClient

    \Description
        Remove a client.

    \Input *pLink       - module state
    \Input iClientId    - unique client identifier, or GSLINK_DEL*CLIENTS to delete multiple clients

    \Output
        int32_t         - negative=failure, else success

    \Version 02/01/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t GameServerLinkDelClient(GameServerLinkT *pLink, int32_t iClientId)
{
    GameServerLinkClientT *pClient;
    int32_t iClient;

    // delete multiple clients?
    if ((iClientId == GSLINK_DELALLCLIENTS) || (iClientId == GSLINK_DELDSCCLIENTS))
    {
        // consider all clients for deletion
        for (iClient = 0; iClient < pLink->ClientList.iMaxClients; iClient++)
        {
            pClient = &pLink->ClientList.Clients[iClient];
            if ((iClientId == GSLINK_DELALLCLIENTS) || (pClient->eStatus == GSLINK_STATUS_DISC))
            {
                _GameServerLinkDelClient(pLink, pClient, iClient);
            }
        }
    }
    else
    {
        // make sure there are clients
        if (pLink->ClientList.iNumClients == 0)
        {
            DirtyCastLogPrintf("gameserverlink: attempt to del client 0x%08x when client list is empty - ignored\n", iClientId);
            return(-1);
        }
        // ref specified client
        if ((pClient = _GameServerLinkFindClientById(pLink, iClientId)) == NULL)
        {
            DirtyCastLogPrintf("gameserverlink: attempt to del client 0x%08x when client is not in list - ignored\n", iClientId);
            return(-2);
        }
        // delete the client
        iClient = pClient - pLink->ClientList.Clients;
        _GameServerLinkDelClient(pLink, pClient, iClient);
    }

    // return success
    return(0);
}

/*F********************************************************************************/
/*!
    \Function GameServerLinkRecv

    \Description
        Receive a packet from VoipServer.

    \Input *pLink       - module state
    \Input iClientId    - unique client identifier

    \Output
        int32_t         - negative=failure, else success

    \Version 02/01/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t GameServerLinkRecv(GameServerLinkT *pLink, GameServerPacketT *pPacket)
{
    GameServerLinkClientT *pClient;
    struct sockaddr RecvAddr;
    int32_t iPacketSize;
    uint32_t uCurTick = NetTick(), uInbLate;

    // make sure there are clients
    if (pLink->ClientList.iNumClients == 0)
    {
        NetPrintfVerbose((pLink->iVerbosity, 0, "gameserverlink: attempt to receive data from client 0x%08x when client list is empty ignored\n", pPacket->Packet.GameData.iClientId));
        return(-1);
    }

    // find the client
    if ((pClient = _GameServerLinkFindClientById(pLink, pPacket->Packet.GameData.iClientId)) == NULL)
    {
        NetPrintfVerbose((pLink->iVerbosity, 0, "gameserverlink: attempt to recv data from client 0x%08x when client is not in list - ignored\n", pPacket->Packet.GameData.iClientId));
        return(-2);
    }
    // ignore recv if client is disconnected
    if (pClient->eStatus == GSLINK_STATUS_DISC)
    {
        NetPrintfVerbose((pLink->iVerbosity, 0, "gameserverlink: ignoring recv on disconnected client 0x%08x\n", pPacket->Packet.GameData.iClientId));
        return(-3);
    }

    // calculate size
    iPacketSize = pPacket->Header.uSize - sizeof(pPacket->Header) - sizeof(pPacket->Packet.GameData) + sizeof(pPacket->Packet.GameData.aData);

    // create source address
    SockaddrInit(&RecvAddr, AF_INET);
    SockaddrInSetAddr(&RecvAddr, pClient->uAddr);
    SockaddrInSetPort(&RecvAddr, pClient->uVirt);
    SockaddrInSetMisc(&RecvAddr, pPacket->Packet.GameData.uTimestamp);

    // push data into client's socket
    if (SocketControl(pClient->pSocket, 'push', iPacketSize, pPacket->Packet.GameData.aData, &RecvAddr) != 0)
    {
        DirtyCastLogPrintf("gameserverlink: error pushing incoming data into client 0x%08x\n", pClient->iClientId);
    }

    // update them to give them an immediate shot at the data
    _GameServerLinkUpdate(pLink, pClient, uCurTick);

    // update inbound latency stat
    uInbLate = uCurTick - pPacket->Packet.GameData.uTimestamp;
    if ((signed)uInbLate < 0)
    {
        if ((signed)uInbLate < -1)
        {
            DirtyCastLogPrintfVerbose(pLink->iVerbosity, DIRTYCAST_DBGLVL_MISCINFO, "gameserverlink: clamping inbound latency measurement of %d to zero\n", uInbLate);
        }
        uInbLate = 0;
    }
    GameServerPacketUpdateLateStatsSingle(&pLink->InbLate, uInbLate);

    // return success
    return(0);
}

/*F********************************************************************************/
/*!
    \Function GameServerLinkUpdate

    \Description
        Update the GameServerLink module.

    \Input *pLink               - module state

    \Output
        GameServerStatInfoT *   - NULL if no new latency info, or a pointer to new late in

    \Version 02/01/2007 (jbrookes)
*/
/********************************************************************************F*/
GameServerStatInfoT *GameServerLinkUpdate(GameServerLinkT *pLink)
{
    GameServerLinkClientT *pClient;
    uint32_t uCurTick = NetTick();
    int32_t iClient;

    // update individual client connections
    for (iClient = 0; iClient < pLink->ClientList.iMaxClients; iClient++)
    {
        // ref client
        pClient = &pLink->ClientList.Clients[iClient];

        // update non-disconnected clients
        if ((pClient->eStatus != GSLINK_STATUS_FREE) && (pClient->eStatus != GSLINK_STATUS_DISC))
        {
            _GameServerLinkUpdate(pLink, pClient, uCurTick);
        }
    }

    // update latency info once a second
    if ((signed)(uCurTick - pLink->uLastLateTick) > 1000)
    {
        // update timer
        pLink->uLastLateTick = uCurTick;
        // update latency
        if (_GameServerLinkUpdateStat(pLink, &pLink->StatInfo, uCurTick) > 0)
        {
            // report recent increase in RlmtChangeCount
            pLink->StatInfo.uRlmtChangeCount = pLink->uTotalRlmtChangeCount - pLink->uReportedRlmtChangeCount;
            pLink->uReportedRlmtChangeCount = pLink->uTotalRlmtChangeCount;

            // return latency info to caller
            return(&pLink->StatInfo);
        }
    }
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function GameServerLinkUpdateFixed

    \Description
        Update the GameServerLink module at a fixed rate, as defined in config file.
        It is the callers' responsibility to ensure that this function is called
        at a fixed rate.

    \Input *pLink   - module state

    \Version 02/21/2007 (jbrookes)
*/
/********************************************************************************F*/
void GameServerLinkUpdateFixed(GameServerLinkT *pLink)
{
    // update module
    if (pLink->pUpdateFunc != NULL)
    {
        pLink->pUpdateFunc(&pLink->ClientList, -1, pLink->pUpdateData, NULL);
    }
}

/*F********************************************************************************/
/*!
    \Function GameServerLinkStartGame

    \Description
        Handle start game notification.

    \Input *pLink   - module state

    \Version 03/05/2007 (jbrookes)
*/
/********************************************************************************F*/
void GameServerLinkStartGame(GameServerLinkT *pLink)
{
    pLink->bGameStarted = TRUE;
    DirtyCastLogPrintfVerbose(pLink->iVerbosity, DIRTYCAST_DBGLVL_MISCINFO, "gameserverlink: game start\n");
}

/*F********************************************************************************/
/*!
    \Function GameServerLinkStopGame

    \Description
        Handle stop game notification.

    \Input *pLink   - module state

    \Version 05/20/2008 (jbrookes)
*/
/********************************************************************************F*/
void GameServerLinkStopGame(GameServerLinkT *pLink)
{
    pLink->bGameStarted = FALSE;
    DirtyCastLogPrintfVerbose(pLink->iVerbosity, DIRTYCAST_DBGLVL_MISCINFO, "gameserverlink: game stop\n");
}

/*F********************************************************************************/
/*!
    \Function GameServerLinkLogClientLatencies

    \Description
        Log per-client latency info

    \Input *pLink   - module state

    \Version 09/20/2010 (jbrookes)
*/
/********************************************************************************F*/
void GameServerLinkLogClientLatencies(GameServerLinkT *pLink)
{
    GameServerLinkClientT *pClient;
    int32_t iClient;

    // update individual client connections
    for (iClient = 0; iClient < pLink->ClientList.iMaxClients; iClient++)
    {
        // ref client
        pClient = &pLink->ClientList.Clients[iClient];

        // update non-disconnected clients
        if ((pClient->pGameLink != NULL) && (pClient->eStatus != GSLINK_STATUS_FREE) && (pClient->eStatus != GSLINK_STATUS_DISC))
        {
            int32_t iLPackLost, iRPackLost, iRPackSent, iLPackSent, iRPackRcvd, iRNakSent, iRlmt;
            char strBuf1[32], strBuf2[96];
            NetGameLinkStatT Stat;
            float fLPackLostPct, fRPackLostPct;

            // get link stats
            NetGameLinkStatus(pClient->pGameLink, 'stat', 0, &Stat, sizeof(NetGameLinkStatT));

            // calculate remote->local packets lost since last update
            iRPackSent = (int32_t)(Stat.rpacksent - pClient->uRPackSent);
            iLPackLost = (int32_t)(Stat.lpacklost - pClient->uLPackLost);

            // calculate remote->local NAKs sent since last update
            iRNakSent = (int32_t)(Stat.rnaksent - pClient->uRNakSent);

            // calculate remote->local packet loss percentage
            fLPackLostPct = ((iLPackLost > 0) && (iRPackSent > 0)) ? ((float)iLPackLost * 100.0f / (float)iRPackSent) : 0.0f;

            // calculate local->remote packets lost since last update
            iRPackRcvd = (int32_t)(Stat.rpackrcvd - pClient->uRPackRcvd);
            iRPackLost = (int32_t)(Stat.rpacklost - pClient->uRPackLost);
            iLPackSent = iRPackRcvd + iRPackLost;

            // calculate local->remote packet loss percentage
            fRPackLostPct = ((iRPackLost > 0) && (iLPackSent > 0)) ? ((float)iRPackLost * 100.0f / (float)iLPackSent) : 0.0f;

            // calculate redundancy limit change since last update
            iRlmt = pClient->uRlmtChangeCount - pClient->uRLmt;

            // per-client logging if client has a poor connection, redundancy limit has changed, or verbosity is greater than zero
            if ((_GameServerLinkPoorConnQuality(&pLink->ClientLateInfo[iClient], fLPackLostPct, fRPackLostPct, iRNakSent) == TRUE) ||
                (pClient->bRlmtChanged == TRUE) || (pLink->iVerbosity > 0))
            {
                ds_snzprintf(strBuf1, sizeof(strBuf1), "[     0x%08x]", pClient->iClientId);
                ds_snzprintf(strBuf2, sizeof(strBuf2), "     lpacklost: %2d/%2.2f%%   rpacklost: %2d/%2.2f%%   rnaksent: %2d   rlmt:%2d/%s\n",
                    iLPackLost, fLPackLostPct, iRPackLost, fRPackLostPct, iRNakSent, iRlmt, (pClient->bRlmtEnabled?"ON":"OFF"));
                GameServerPacketLogLateStats(&pLink->ClientLateInfo[iClient], strBuf1, strBuf2);
                pClient->bRlmtChanged = FALSE;
            }

            // update counters
            pClient->uRNakSent = Stat.rnaksent;
            pClient->uRPackSent = Stat.rpacksent;
            pClient->uRPackRcvd = Stat.rpackrcvd;
            pClient->uLPackRcvd = Stat.lpackrcvd;
            pClient->uRPackLost = Stat.rpacklost;
            pClient->uLPackLost = Stat.lpacklost;
            pClient->uRLmt = pClient->uRlmtChangeCount;
        }

        // clear latency accumulator
        GameServerPacketUpdateLateStats(&pLink->ClientLateInfo[iClient], NULL);
    }
}

/*F********************************************************************************/
/*!
    \Function GameServerLinkUpdateRedundancy

    \Description
        Check if client link redundancy needs to be modified

    \Input *pLink   - module state

    \Version 03/20/2014 (mclouatre)
*/
/********************************************************************************F*/
void GameServerLinkUpdateRedundancy(GameServerLinkT *pLink)
{
    GameServerLinkClientT *pClient;
    int32_t iClient;
    uint32_t uCurTick = NetTick();

    // update individual client connections
    for (iClient = 0; iClient < pLink->ClientList.iMaxClients; iClient++)
    {
        // ref client
        pClient = &pLink->ClientList.Clients[iClient];

        // update non-disconnected clients
        if ((pClient->pGameLink != NULL) && (pClient->eStatus != GSLINK_STATUS_FREE) && (pClient->eStatus != GSLINK_STATUS_DISC))
        {
            uint32_t bPacketLossDetected = _GameServerLinkL2RPktLossDetection(pLink, pClient, iClient);

            // detect packet loss
            if (bPacketLossDetected)
            {
                pClient->iLastPktLoss = uCurTick;
            }

            // check if underlying comm redundancy limit needs to be changed
            if (pClient->bRlmtEnabled)
            {
                // is it time to return to default redundancy limit value ?
                if (NetTickDiff(uCurTick, pClient->iLastPktLoss) > pLink->iRedundancyTimeout * 1000)
                {
                    NetGameLinkControl(pClient->pGameLink, 'rlmt', 0, NULL);
                    pClient->bRlmtEnabled = FALSE;
                    pClient->bRlmtChanged = TRUE;
                    DirtyCastLogPrintfVerbose(pLink->iVerbosity, DIRTYCAST_DBGLVL_CONNINFO, "gameserverlink: rlmt for clnt %d reset to default (rlmt timeout = %d secs)\n",
                        iClient, pLink->iRedundancyTimeout);
                }
            }
            else
            {
                // is it time to move away from the default redundancy limit value ?
                if (bPacketLossDetected)
                {
                    NetGameLinkControl(pClient->pGameLink, 'rlmt', pLink->uRedundancyLimit, NULL);
                    pClient->bRlmtEnabled = TRUE;
                    pClient->bRlmtChanged = TRUE;
                    pClient->uRlmtChangeCount++;
                    pLink->uTotalRlmtChangeCount++;
                    DirtyCastLogPrintfVerbose(pLink->iVerbosity, DIRTYCAST_DBGLVL_CONNINFO, "gameserverlink: rlmt for clnt %d modified due to L->R packet loss detection (count = %d)\n",
                        iClient, pClient->uRlmtChangeCount);
                }
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function GameServerLinkDiagnosticStatus

    \Description
        Format gameserverlink stats for diagnostic request.

    \Input *pLink       - module state
    \Input *pBuffer     - pointer to buffer to format data into
    \Input iBufSize     - size of buffer to format data into
    \Input uCurTime     - current time

    \Output
        int32_t                 - number of bytes written to output buffer

    \Version 03/27/2014 (mclouatre)
*/
/********************************************************************************F*/
int32_t GameServerLinkDiagnosticStatus(GameServerLinkT *pLink, char *pBuffer, int32_t iBufSize, time_t uCurTime)
{
    int32_t iOffset = 0;
    GameServerLinkClientT *pClient;
    int32_t iClient;

    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "stats of connected clients:\n");

    // update individual client connections
    for (iClient = 0; iClient < pLink->ClientList.iMaxClients; iClient++)
    {
        // ref client
        pClient = &pLink->ClientList.Clients[iClient];

        // update non-disconnected clients
        if ((pClient->pGameLink != NULL) && (pClient->eStatus != GSLINK_STATUS_FREE) && (pClient->eStatus != GSLINK_STATUS_DISC))
        {
            iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "  %2d:[     0x%08x]  rpacksent:%10d  lpackrcvd:%10d  rpackrcvd:%10d  rpacklost:%10d  rnaksent:%10d  rlmt:%10d/%s\n",
                iClient, pClient->iClientId, pClient->uRPackSent, pClient->uLPackRcvd, pClient->uRPackRcvd, pClient->uRPackLost, pClient->uRNakSent, pClient->uRlmtChangeCount, (pClient->bRlmtEnabled?"ON":"OFF"));
        }
    }

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function GameServerLinkStatus

    \Description
        Status function

    \Input *pLink       - module state
    \Input iSelect      - selector
    \Input iValue       - input value
    \Input pBuf         - output buffer
    \Input iBufSize     - output buffer size

    \Output
        int32_t         - selector specific

    \Notes
        iSelect can be one of the following:

        \verbatim
            'qlat' - return the netgamelinkstatus 'qlat' for the client (index via iValue)
            'rlmt' - returns count of redundancy limit changes (across all clients during lifetime of gameserverlink module)
            'stat' - return the netgamelinkstatus 'stat' for the client (index via iValue)
        \endverbatim

    \Version 03/27/2014 (mclouatre)
*/
/********************************************************************************F*/
int32_t GameServerLinkStatus(GameServerLinkT *pLink, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    if (iSelect == 'rlmt')
    {
        return(pLink->uTotalRlmtChangeCount);
    }
    if ((iSelect == 'stat') || (iSelect == 'qlat'))
    {
        GameServerLinkClientT *pClient;

        // make sure it is a valid index
        if (iValue >= pLink->ClientList.iMaxClients)
        {
            return(-1);
        }
        pClient = &pLink->ClientList.Clients[iValue];

        // validate the client
        if (pClient->pGameLink == NULL)
        {
            return(-1);
        }

        return(NetGameLinkStatus(pClient->pGameLink, iSelect, 0, pBuf, iBufSize));
    }
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function GameServerLinkClientList

    \Description
        Status function

    \Input *pLink       - module state

    \Output
       GameServerLinkClientListT*    - pointer to game server link client list

    \Version 09/23/2016 (tcho)
*/
/********************************************************************************F*/
GameServerLinkClientListT* GameServerLinkClientList(GameServerLinkT* pLink)
{
    return(&pLink->ClientList);
}
