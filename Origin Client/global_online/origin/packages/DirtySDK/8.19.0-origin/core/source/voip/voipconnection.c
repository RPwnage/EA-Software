/*H********************************************************************************/
/*!
    \File voipconnection.c

    \Description
        VoIP virtual connection manager.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 03/17/2004 (jbrookes)  First Version
    \Version 1.1 01/06/2008 (mlcouatre) Created VoipConnectionAddRef(), VoipConnectionRemoveRef(), VoipConnectionResetRef(), VoipConnectionGetFallbackAddr()
    \Version 1.2 10/26/2009 (mlcouatre) Renamed from xbox/voipconnection.c to xenon/voipconnectionxenon.c
    \Version 1.3 11/13/2009 (jbrookes)  Merged Xenon back into common version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#ifdef _XBOX
#include <xtl.h>
#include <xonline.h>
#endif

#include <string.h>

#include "platform.h"
#include "dirtysock.h"
#include "dirtymem.h"

#include "voipdef.h"
#include "voippriv.h"
#include "voipcommon.h"

#include "voipconnection.h"

/*** Defines **********************************************************************/

#define VOIP_PACKET_VERSION     ('e')           //!< current packet version
#define VOIP_PING_RATE          ( 500)          //!< send ping packets twice a second
#define VOIP_TIMEOUT            (15*1000)       //!< default voip data timeout
#define VOIP_CONNTIMEOUT        (10*1000)       //!< connection timeout
#define VOIP_MSPERPACKET        (100)           //!< number of ms per network packet (100ms = 10hz)
#define VOIP_TALKTIMEOUT        ( 100)          //!< clear talking flag after 1/10th sec of no voice

//! enable for verbose debugging
#define VOIP_CONNECTION_DEBUG   (DIRTYCODE_DEBUG && FALSE)

#if (defined(DIRTYCODE_XENON))
 #define VOIP_IPPROTO        (IPPROTO_VDP)
#else
 #define VOIP_IPPROTO        (IPPROTO_IP)
#endif

/*** Macros ***********************************************************************/

//! compare packet types
#define VOIP_SamePacketType(_packetHead1, _packetHead2)  \
    (!memcmp((_packetHead1)->aType, (_packetHead2)->aType, sizeof((_packetHead1)->aType)))

//! get connection ID
#define VOIP_ConnID(_pConnectionlist, _pConnection) ((uint32_t)((_pConnection)-(_pConnectionlist)->pConnections))

/*** Type Definitions *************************************************************/

/*** Function Prototypes **********************************************************/

static void _VoipConnectionStop(VoipConnectionlistT *pConnectionlist, VoipConnectionT *pConnection, int32_t iConnID, int32_t bSendDiscMsg);

/*** Variables ********************************************************************/

//! VoIP conn packet header
static VoipPacketHeadT  _Voip_ConnPacket =
{
    #if defined(DIRTYCODE_XENON)
    sizeof(VoipConnPacketT)-2,
    #endif
    { 'C', 'O', VOIP_PACKET_VERSION }, 0, { 0, 0, 0, 0 }, {0, 0}
};

//! VoIP disc packet header
static VoipPacketHeadT  _Voip_DiscPacket =
{
    #if defined(DIRTYCODE_XENON)
    sizeof(VoipDiscPacketT)-2,
    #endif
    { 'D', 'S', 'C' }, 0, { 0, 0, 0, 0 }, {0, 0}
};

//! VoIP ping packet header
static VoipPacketHeadT  _Voip_PingPacket =
{
    #if defined(DIRTYCODE_XENON)
    sizeof(VoipPingPacketT)-2,
    #endif
    { 'P', 'N', 'G' }, 0, { 0, 0, 0, 0 }, {0, 0}
};

//! VoIP mic data packet header
static VoipPacketHeadT  _Voip_MicrPacket =
{
    #if defined(DIRTYCODE_XENON)
    // we set the size of data to be encrypted to the size of the packet minus the VDP header and voice data
    sizeof(VoipMicrPacketT)-2 - sizeof(((VoipMicrPacketT *)0)->aData),
    #endif
    { 'M', 'I', 'C' }, 0, { 0, 0, 0, 0 }, {0, 0}
};

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function   _VoipEncodeFlags

    \Description
        Encode the aFlags field in the packet header

    \Input *pConnectionlist - information read from here to be written to the packet header
    \Input *pHead           - packet header to write to

    \Version 11/20/2008 (cvienneau)
*/
/********************************************************************************F*/
static void _VoipEncodeFlags(VoipConnectionlistT *pConnectionlist, VoipPacketHeadT *pHead)
{
    uint16_t uFlags = 0;

    if (pConnectionlist->uLocalStatus & VOIP_LOCAL_HEADSETOK)
    {
        uFlags |= VOIP_PACKET_STATUS_FLAG_HEADSETOK;
    }

    pHead->aFlags[0] = (uint8_t)(uFlags >> 8);
    pHead->aFlags[1] = (uint8_t)uFlags;
}

/*F********************************************************************************/
/*!
    \Function   _VoipDecodeFlags

    \Description
        Reads aFlags field from an incoming packet and sets appropriate state about the
        remote connection.

    \Input *pConnection     - connection to write state to
    \Input *pHead           - packet header to read information from

    \Version 11/20/2008 (cvienneau)
*/
/********************************************************************************F*/
static void _VoipDecodeFlags(VoipConnectionT *pConnection, VoipPacketHeadT *pHead)
{
    uint16_t uFlags = pHead->aFlags[0] << 8;
    uFlags |= pHead->aFlags[1];

    if (uFlags & VOIP_PACKET_STATUS_FLAG_HEADSETOK)
    {
        pConnection->uRemoteStatus |= VOIP_REMOTE_HEADSETOK;    //on
    }
    else
    {
        pConnection->uRemoteStatus &= ~VOIP_REMOTE_HEADSETOK;   //off
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipDecodeU32

    \Description
        Decode uint32_t from given packet structure.

    \Input *pValue      - network-order data field

    \Output
        uint32_t        - decoded value

    \Version 06/02/2006 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _VoipDecodeU32(const uint8_t *pValue)
{
    uint32_t uValue = pValue[0] << 24;
    uValue |= pValue[1] << 16;
    uValue |= pValue[2] << 8;
    uValue |= pValue[3];
    return(uValue);
}

/*F********************************************************************************/
/*!
    \Function   _VoipEncodeU32

    \Description
        Encode given uint32_t in network order

    \Input *pValue      - storage for network-order u32
    \Input uValue       - u32 value to encode

    \Version 06/02/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipEncodeU32(uint8_t *pValue, uint32_t uValue)
{
    pValue[0] = (uint8_t)(uValue >> 24);
    pValue[1] = (uint8_t)(uValue >> 16);
    pValue[2] = (uint8_t)(uValue >> 8);
    pValue[3] = (uint8_t)uValue;
}

/*F********************************************************************************/
/*!
    \Function   _VoipPrepareVoipServerSendMask

    \Description
        Convert local send mask to voip-server ready send mask

    \Input *pConnectionlist - connection list

    \Output
        uint32_t            - send mask to be packed in voip packet sent to voip server

    \Version 10/27/2006 (mclouatre)
*/
/********************************************************************************F*/
static uint32_t _VoipPrepareVoipServerSendMask(VoipConnectionlistT *pConnectionlist)
{
    int32_t iLocalConnId, iVoipServerConnId;
    uint32_t uLocalSendMask = pConnectionlist->uSendMask;
    uint32_t uVoipServerSendMask = 0;

    for (iLocalConnId = 0; iLocalConnId < pConnectionlist->iMaxConnections; iLocalConnId++)
    {
        iVoipServerConnId = pConnectionlist->pConnections[iLocalConnId].iVoipServerConnId;

        // make sure connection is used by voip server
        if (iVoipServerConnId != VOIP_CONNID_NONE)
        {
            // make sure local connection has send bit set
            if (uLocalSendMask & (1 << iLocalConnId))
            {
                // set the proper bit in the send mask for the voip server
                uVoipServerSendMask |= (1 << iVoipServerConnId);
            }
        }
    }

    return(uVoipServerSendMask);
}

/*F********************************************************************************/
/*!
    \Function   _VoipConnectionAllocate

    \Description
        Allocate an empty connection for use.

    \Input  *pConnectionlist    - pointer to connectionlist
    \Input  iConnID             - connection to allocate (or VOIP_CONNID_*)
    \Input  *pUserID            - userID of user we're allocating connection for

    \Output
        VoipConnectionT *       - pointer to connection to use or NULL

    \Version 03/17/2004 (jbrookes)
*/
/********************************************************************************F*/
static VoipConnectionT *_VoipConnectionAllocate(VoipConnectionlistT *pConnectionlist, int32_t iConnID, VoipUserT *pUserID)
{
    VoipConnectionT *pConnection = NULL;
    int32_t iConnTest;

    // make sure they aren't already connected to this address
    for (iConnTest = 0; iConnTest < pConnectionlist->iMaxConnections; iConnTest++)
    {
        pConnection = &pConnectionlist->pConnections[iConnTest];

        // see if we are already connected to this user
        if (pConnectionlist->pConnections[iConnTest].eState != ST_DISC)
        {
            if (VOIP_SameUser(&pConnectionlist->pConnections[iConnTest].RemoteUsers[0], pUserID))
            {
                NetPrintf(("voipconnection: [%d] already connected to user '%s'\n", iConnTest, pUserID->strUserId));
                return(NULL);
            }
        }
    } // End of FOR loop [iConnTest]

    // get a connection ID
    pConnection = NULL;
    if (iConnID == VOIP_CONNID_NONE)
    {
        // find a free connection slot
        for (iConnID = 0; iConnID < pConnectionlist->iMaxConnections; iConnID++)
        {
            // if this is an unallocated connection
            if (pConnectionlist->pConnections[iConnID].eState == ST_DISC)
            {
                pConnection = &pConnectionlist->pConnections[iConnID];
                break;
            }
        }

        // make sure we found a connection
        if (pConnection == NULL)
        {
            NetPrintf(("voipconnection: out of connections\n"));
        }
    }
    else if ((iConnID >= 0) && (iConnID < pConnectionlist->iMaxConnections))
    {
        // if we're currently connected, complain
        if (pConnectionlist->pConnections[iConnID].eState != ST_DISC)
        {
            NetPrintf(("voipconnection: [%d] connection not available, currently used for client id %d\n", iConnID, pConnectionlist->pConnections[iConnID].uClientId));
            return(NULL);
        }

        // ref connection
        pConnection = &pConnectionlist->pConnections[iConnID];
    }
    else
    {
        NetPrintf(("voipconnection: %d is an invalid connection id\n", iConnID));
    }

    // return connection ref to caller
    return(pConnection);
}

/*F********************************************************************************/
/*!
    \Function   _VoipSocketSendto

    \Description
        Send data with the given socket.

    \Input *pSocket     - socket to send on
    \Input uClientId    - client identifier
    \Input *pBuf        - data to send
    \Input iLen         - size of data to send
    \Input iFlags       - send flags
    \Input *pTo         - address to send data to
    \Input iToLen       - length of address structure

    \Output
        int32_t         - amount of data sent

    \Version 11/30/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipSocketSendto(SocketT *pSocket, uint32_t uClientId, VoipPacketHeadT *pBuf, int32_t iLen, int32_t iFlags, struct sockaddr *pTo, int32_t iToLen)
{
    // make sure socket exists
    if (pSocket == NULL)
    {
        return(0);
    }

    // set connection identifier (goes in all packets)
    _VoipEncodeU32(pBuf->aClientId, uClientId);

    // send the packet
    return(SocketSendto(pSocket, (char *)pBuf, iLen, iFlags, pTo, iToLen));
}


/*F********************************************************************************/
/*!
    \Function _VoipConnectionMatchSessionId

    \Description
        Check whether specified session ID matchs the voip connection.

    \Input *pConnectionlist - connectionlist
    \Input iConnId          - connection ID
    \Input uSessionId       - session ID

    \Output
        uint8_t             - TRUE for match, FALSE for no match

    \Version 08/30/2011 (mclouatre)
*/
/********************************************************************************F*/
static uint8_t _VoipConnectionMatchSessionId(VoipConnectionlistT *pConnectionlist, int32_t iConnId, uint32_t uSessionId)
{
    int32_t iSessionIndex;
    int32_t bMatch = FALSE;
    VoipConnectionT *pConnection = &pConnectionlist->pConnections[iConnId];

    NetCritEnter(&pConnectionlist->NetCrit);

    for (iSessionIndex = 0; iSessionIndex < VOIP_MAXSESSIONIDS; iSessionIndex++)
    {
        if (pConnection->uSessionId[iSessionIndex] == uSessionId)
        {
            bMatch = TRUE;
        }
    }

    NetCritLeave(&pConnectionlist->NetCrit);

    return(bMatch);
}


#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function   _VoipConnectionPrintSessionIds

    \Description
        Prints all session IDs sharing this voip connection

    \Input *pConnectionlist - connectionlist
    \Input iConnId          - connection id

    \Version 08/30/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipConnectionPrintSessionIds(VoipConnectionlistT *pConnectionlist, int32_t iConnId)
{
    int32_t iSessionIndex;
    VoipConnectionT *pConnection = &pConnectionlist->pConnections[iConnId];

    NetCritEnter(&pConnectionlist->NetCrit);

    if (pConnection->uSessionId[0] != 0)
    {
        NetPrintf(("voipconnection: [%d] IDs of sessions sharing this voip connection are: ", iConnId));
        for (iSessionIndex = 0; iSessionIndex < VOIP_MAXSESSIONIDS; iSessionIndex++)
        {
            if (pConnection->uSessionId[iSessionIndex] != 0)
            {
                NetPrintf(("0x%08x\n", pConnection->uSessionId[iSessionIndex]));
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        NetPrintf(("voipconnection: [%d] there is no session ID associated with this connection\n", iConnId));
    }

    NetCritLeave(&pConnectionlist->NetCrit);
}
#endif  // DIRTYCODE_LOGGING

#if defined(DIRTYCODE_XENON)
#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function   _VoipConnectionPrintSendAddrFallbacks

    \Description
        Prints all send addr fallbacks of a given voip connection

    \Input *pConnectionlist - connectionlist
    \Input iConnId          - connection id

    \Version 11/27/2009 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipConnectionPrintSendAddrFallbacks(VoipConnectionlistT *pConnectionlist, int32_t iConnId)
{
    int32_t iAddrIndex;
    VoipConnectionT *pConnection = &pConnectionlist->pConnections[iConnId];

    NetCritEnter(&pConnectionlist->NetCrit);

    if (pConnection->SendAddrFallbacks[0].SendAddr != 0)
    {
        NetPrintf(("voipconnection: [%d] send address fallbacks are: ", iConnId));
        for (iAddrIndex = 0; iAddrIndex < VOIP_MAXADDRFALLBACKS; iAddrIndex++)
        {
            if (pConnection->SendAddrFallbacks[iAddrIndex].SendAddr != 0)
            {
                NetPrintf(("%a(%s)  ", pConnection->SendAddrFallbacks[iAddrIndex].SendAddr, (pConnection->SendAddrFallbacks[iAddrIndex].bReachable ? "reachable" : "unreachable")));
            }
            else
            {
                break;
            }
        }
        NetPrintf(("\n"));
    }
    else
    {
        NetPrintf(("voipconnection: [%d] there is no send fallback address for this connection\n", iConnId));
    }

    NetCritLeave(&pConnectionlist->NetCrit);
}
#endif  // DIRTYCODE_LOGGING

/*F********************************************************************************/
/*!
    \Function   _VoipConnectionReplaceActiveSendAddr

    \Description
        Replace currently active SendAddr with an address from the fallback list
        that is not yet know as unreachable.

    \Input *pConnectionlist - connectionlist
    \Input iConnId          - connection id

    \Output
        int32_t             - 0 for success, negative for failure

    \Version 11/27/2009 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipConnectionReplaceActiveSendAddr(VoipConnectionlistT *pConnectionlist, int32_t iConnId)
{
    int32_t iAddrIndex;
    int32_t iRetCode = -1;
    uint32_t uReplacementAddr = 0;
    VoipConnectionT *pConnection = &pConnectionlist->pConnections[iConnId];

    NetCritEnter(&pConnectionlist->NetCrit);

    // find an address in the fallback list that is not yet known as unreachable
    for (iAddrIndex = 0; iAddrIndex < VOIP_MAXADDRFALLBACKS; iAddrIndex++)
    {
        if ( (pConnection->SendAddrFallbacks[iAddrIndex].SendAddr != SocketNtohl(pConnection->SendAddr.sin_addr.s_addr)) &&
             (pConnection->SendAddrFallbacks[iAddrIndex].SendAddr != 0) &&
             (pConnection->SendAddrFallbacks[iAddrIndex].bReachable == TRUE) )
        {
            uReplacementAddr = pConnection->SendAddrFallbacks[iAddrIndex].SendAddr;
            break;
        }
    }

    if (uReplacementAddr != 0)
    {
        NetPrintf(("voipconnection: [%d] replacing active send addr %a with %a\n", iConnId, SocketNtohl(pConnection->SendAddr.sin_addr.s_addr), uReplacementAddr));
        pConnection->SendAddr.sin_addr.s_addr = SocketHtonl(uReplacementAddr);
        iRetCode = 0;
    }
    else
    {
        NetPrintf(("voipconnection: [%d] no success replacing active send addr with a fallback\n", iConnId));
        iRetCode = -1;
    }

    NetCritLeave(&pConnectionlist->NetCrit);

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function   _VoipConnectionSendAddrFallbackMarkUnreachable

    \Description
        Mark an address as unreachable.

    \Input *pConnectionlist - connectionlist
    \Input iConnId          - connection id
    \Input uAddr            - address to be marked as unreachable

    \Output
        int32_t             - 0 for success, negative for failure

    \Version 11/27/2009 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipSendAddrFallbackMarkUnreachable(VoipConnectionlistT *pConnectionlist, int32_t iConnId, uint32_t uAddr)
{
    int32_t iAddrIndex;
    int32_t iRetCode = -1;
    VoipConnectionT *pConnection = &pConnectionlist->pConnections[iConnId];

    NetCritEnter(&pConnectionlist->NetCrit);

    // find an address in the fallback list that is not yet known as unreachable
    for (iAddrIndex = 0; iAddrIndex < VOIP_MAXADDRFALLBACKS; iAddrIndex++)
    {
        if (pConnection->SendAddrFallbacks[iAddrIndex].SendAddr == uAddr)
        {
            NetPrintf(("voipconnection: [%d] fallback address %a marked as unreachable\n", iConnId, uAddr));
            pConnection->SendAddrFallbacks[iAddrIndex].bReachable = FALSE;
            iRetCode = 0;
            break;
        }
    }

    if (iRetCode !=0 )
    {
        NetPrintf(("voipconnection: [%d] warning - fallback address %a to be marked as unreachable could not be found\n", iConnId, uAddr));
    }

    NetCritLeave(&pConnectionlist->NetCrit);
    return(iRetCode);
}

#endif  // defined(DIRTYCODE_XENON)


/*F********************************************************************************/
/*!
    \Function   _VoipConnectionConn

    \Description
        Send a connection packet to peer

    \Input *pConnectionlist - connectionlist ref
    \Input *pConnection     - pointer to connection to connect on
    \Input uTick            - current tick count

    \Version 03/20/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipConnectionConn(VoipConnectionlistT *pConnectionlist, VoipConnectionT *pConnection, uint32_t uTick)
{
    VoipConnPacketT ConnPacket;

    // output diagnostic info
    NetPrintf(("voipconnection: [%d] sending connect packet to user '%s' (%a:%d)\n", VOIP_ConnID(pConnectionlist, pConnection),
        pConnection->RemoteUsers[pConnection->iPrimaryRemoteUser].strUserId,
        SocketNtohl(pConnection->SendAddr.sin_addr.s_addr), SocketNtohs(pConnection->SendAddr.sin_port)));

    // send a connection packet
    memcpy(&ConnPacket.Head, &_Voip_ConnPacket, sizeof(ConnPacket.Head));
    _VoipEncodeU32(ConnPacket.aRemoteClientId, pConnection->uClientId);
    _VoipEncodeU32(ConnPacket.aSessionId, pConnection->uSessionId[0]);
   _VoipEncodeFlags(pConnectionlist, &(ConnPacket.Head));
    memcpy(ConnPacket.LocalUsers, pConnectionlist->LocalUsers, sizeof(ConnPacket.LocalUsers));
    memcpy(ConnPacket.RemoteUsers, pConnection->RemoteUsers, sizeof(ConnPacket.RemoteUsers));
    ConnPacket.iPrimaryLocalUser = pConnectionlist->iPrimaryLocalUser;
    ConnPacket.iPrimaryRemoteUser = pConnection->iPrimaryRemoteUser;
    ConnPacket.bConnected = pConnection->bConnPktRecv;
    ConnPacket.pad = 0;
    #if VOIP_CONNECTION_DEBUG
    NetPrintMem(&ConnPacket, sizeof(ConnPacket), "connection packet data");
    #endif
    pConnection->iSendResult = _VoipSocketSendto(pConnectionlist->pSocket, pConnectionlist->uClientId, &ConnPacket.Head, sizeof(ConnPacket), 0, (struct sockaddr *)&pConnection->SendAddr, sizeof(pConnection->SendAddr));

    // update timestamp
    pConnection->uLastSend = uTick;
}

/*F********************************************************************************/
/*!
    \Function   _VoipConnectionDisc

    \Description
        Send a disconnection request to peer.

    \Input iConnId          - connection id
    \Input uClientId        - client id
    \Input uRemoteClientId  - remote client id
    \Input uSessionId       - session identifier
    \Input *pSendAddr       - address to send disconnect to
    \Input *pSocket         - socket to send request with

    \Version 03/21/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipConnectionDisc(int32_t iConnId, uint32_t uClientId, uint32_t uRemoteClientId, uint32_t uSessionId, struct sockaddr_in *pSendAddr, SocketT *pSocket)
{
    VoipDiscPacketT DiscPacket;

    // send a disconnect packet
    NetPrintf(("voipconnection: [%d] sending disconnect packet to %a:%d\n",
        iConnId, SocketNtohl(pSendAddr->sin_addr.s_addr), SocketNtohs(pSendAddr->sin_port)));

    // set up disc packet
    memcpy(&DiscPacket.Head, &_Voip_DiscPacket, sizeof(DiscPacket.Head));
    _VoipEncodeU32(DiscPacket.aRemoteClientId, uRemoteClientId);
    _VoipEncodeU32(DiscPacket.aSessionId, uSessionId);

    // send disc packet
    _VoipSocketSendto(pSocket, uClientId, &DiscPacket.Head, sizeof(DiscPacket), 0, (struct sockaddr *)pSendAddr, sizeof(*pSendAddr));
}

/*F********************************************************************************/
/*!
    \Function   _VoipConnectionInit

    \Description
        Transition from connecting to connected.

    \Input *pConnectionlist - connectionlist connection is a part of
    \Input *pConnection     - connection to transition
    \Input iConnID          - index of connection

    \Version 03/21/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipConnectionInit(VoipConnectionlistT *pConnectionlist, VoipConnectionT *pConnection, int32_t iConnID)
{
    NetPrintf(("voipconnection: [%d] connection established; talking to user '%s'\n", iConnID,
        pConnection->RemoteUsers[pConnection->iPrimaryRemoteUser].strUserId));

    // update state and connection flags
    pConnection->eState = ST_ACTV;
    pConnection->uRemoteStatus = VOIP_REMOTE_CONNECTED;

    // add connection identifier to send/recv masks
    VoipConnectionSetSendMask(pConnectionlist, pConnectionlist->uSendMask | (1 << iConnID));
    VoipConnectionSetRecvMask(pConnectionlist, pConnectionlist->uRecvMask | (1 << iConnID));
}

/*F********************************************************************************/
/*!
    \Function   _VoipConnectionPing

    \Description
        Ping a connected remote peer.

    \Input *pConnectionlist - connectionlist ref
    \Input *pConnection     - pointer to connection to send on
    \Input uTick            - current tick count

    \Version 03/20/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipConnectionPing(VoipConnectionlistT *pConnectionlist, VoipConnectionT *pConnection, uint32_t uTick)
{
    VoipPingPacketT PingPacket;

    #if VOIP_CONNECTION_DEBUG
    NetPrintf(("voipconnection: [%d] pinging %a:%d\n", VOIP_ConnID(pConnectionlist, pConnection),
        SocketNtohl(pConnection->SendAddr.sin_addr.s_addr),
        SocketNtohs(pConnection->SendAddr.sin_port)));
    #endif

    // set up ping packet
    memcpy(&PingPacket.Head, &_Voip_PingPacket, sizeof(PingPacket.Head));
    _VoipEncodeFlags(pConnectionlist, &(PingPacket.Head));
    _VoipEncodeU32(PingPacket.aRemoteClientId, pConnection->uClientId);
    _VoipEncodeU32(PingPacket.aChannelId, pConnectionlist->uChannelId);

    // send a 'ping' packet
    pConnection->iSendResult = _VoipSocketSendto(pConnectionlist->pSocket, pConnectionlist->uClientId, &PingPacket.Head, sizeof(PingPacket), 0, (struct sockaddr *)&pConnection->SendAddr, sizeof(pConnection->SendAddr));

    // update send timestamp and local status flag
    pConnection->uLastSend = uTick;
    pConnectionlist->uLocalStatus &= ~VOIP_LOCAL_SENDVOICE;
}

/*F********************************************************************************/
/*!
    \Function   _VoipConnectionIsSendPktReady

    \Description
        Determine if we should send the current packet.

    \Input *pConnectionlist - connectionlist ref
    \Input *pConnection - connection pointer
    \Input uTick        - current tick count

    \Output
        uint32_t        - TRUE if ready, FALSE otherwise.

    \Version 10/01/2011 (mclouatre)
*/
/********************************************************************************F*/
static uint32_t _VoipConnectionIsSendPktReady(VoipConnectionlistT *pConnectionlist, VoipConnectionT *pConnection, uint32_t uTick)
{
    // if packet is full, send it
    if (pConnection->VoipMicrPacket.MicrInfo.uNumSubPackets == VOIP_MAXSUBPKTS_PER_PKT)
    {
        return(TRUE);
    }

    // if an additional sub-packet would cause us to overflow the sub-packet buffer, send packet
    if (((pConnection->VoipMicrPacket.MicrInfo.uNumSubPackets + 1) * pConnection->VoipMicrPacket.MicrInfo.uSubPacketSize) > (signed)sizeof(pConnection->VoipMicrPacket.aData))
    {
        return(TRUE);
    }

    // if 10hz timer has elapsed, send packet
    if ((pConnection->VoipMicrPacket.MicrInfo.uNumSubPackets > 0) && (NetTickDiff(uTick, pConnection->uVoiceSendTimer) >= 0))
    {
        return(TRUE);
    }

    // not yet ready to send
    return(FALSE);
}

#if VOIP_CONNECTION_DEBUG
/*F********************************************************************************/
/*!
    \Function   _VoipConnectionCountSubPktsPerUser

    \Description
        Count the number of sub-packets per user in a given voip packet

    \Input *pPacket             - packet data that was received
    \Input *pUser0SubPktCount   - [OUT] to be filled with sub-packets count for user 0
    \Input *pUser1SubPktCount   - [OUT] to be filled with sub-packets count for user 1
    \Input *pUser2SubPktCount   - [OUT] to be filled with sub-packets count for user 2
    \Input *pUser3SubPktCount   - [OUT] to be filled with sub-packets count for user 3

    \Version 01/27/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipConnectionCountSubPktsPerUser(VoipMicrPacketT *pPacket, int32_t *pUser0SubPktCount, int32_t *pUser1SubPktCount, int32_t *pUser2SubPktCount, int32_t *pUser3SubPktCount)
{
    uint32_t uDataMap, uRemoteUserIndex, uSubPktIndex;

    // initialize counters
    *pUser0SubPktCount = 0;
    *pUser1SubPktCount = 0;
    *pUser2SubPktCount = 0;
    *pUser3SubPktCount = 0;

    // extract data map from the packet
    uDataMap = (pPacket->MicrInfo.aDataMap[0] << 24) | (pPacket->MicrInfo.aDataMap[1] << 16) |
        (pPacket->MicrInfo.aDataMap[2] << 8) | pPacket->MicrInfo.aDataMap[3];

    // inspect voip packet and update the sub-packet counters appropriately
    for (uSubPktIndex=0; uSubPktIndex < pPacket->MicrInfo.uNumSubPackets; uSubPktIndex++)
    {
        // get remote user index from data map
        uRemoteUserIndex = uDataMap >> ((pPacket->MicrInfo.uNumSubPackets - uSubPktIndex - 1) * 2) & 0x3;

        if (uRemoteUserIndex == 0)
        {
            (*pUser0SubPktCount)++;
        }
        else if (uRemoteUserIndex == 1)
        {
            (*pUser1SubPktCount)++;
        }
        else if (uRemoteUserIndex == 2)
        {
            (*pUser2SubPktCount)++;
        }
        else if (uRemoteUserIndex == 3)
        {
            (*pUser3SubPktCount)++;
        }
    }
}
#endif


/*F********************************************************************************/
/*!
    \Function   _VoipConnectionSendSingle

    \Description
        Send a single buffered voice packet, if it is time to send it.

    \Input *pConnectionlist - connectionlist ref
    \Input *pConnection     - pointer to connection to send on
    \Input uTick            - current tick count
    \Input bFlush           - flush data

    \Version 03/20/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipConnectionSendSingle(VoipConnectionlistT *pConnectionlist, VoipConnectionT *pConnection, uint32_t uTick, uint32_t bFlush)
{
    // ref current buffered packet
    VoipMicrPacketT *pPacket = &pConnection->VoipMicrPacket;
    uint32_t bSendPacket = FALSE;

    // should we send?
    if (_VoipConnectionIsSendPktReady(pConnectionlist, pConnection, uTick) || (bFlush == TRUE))
    {
        bSendPacket = TRUE;
    }

    // send packet?
    if (bSendPacket == TRUE)
    {
        int32_t iPacketSize;
        uint32_t uVoipServerReadySendMask = 0;

        // set packet sequence
        pConnection->VoipMicrPacket.MicrInfo.uSeqn = pConnection->uSendSeqn++;

        // set the voip group id
        _VoipEncodeU32(pConnection->VoipMicrPacket.MicrInfo.aChannelId, pConnectionlist->uChannelId);

        // set send mask for VoipServer use
        if (pConnection->iVoipServerConnId != VOIP_CONNID_NONE)
        {
            uVoipServerReadySendMask = _VoipPrepareVoipServerSendMask(pConnectionlist);
        }
        _VoipEncodeU32(pConnection->VoipMicrPacket.MicrInfo.aSendMask, uVoipServerReadySendMask);
        _VoipEncodeFlags(pConnectionlist, &pPacket->Head);

        #if VOIP_CONNECTION_DEBUG
        // display verbose status (metered to about once every three seconds)
        if ((pConnection->uSendSeqn % 30) == 0)
        {
            int32_t iUser0SubPktCount, iUser1SubPktCount, iUser2SubPktCount, iUser3SubPktCount;
            _VoipConnectionCountSubPktsPerUser(pPacket, &iUser0SubPktCount, &iUser1SubPktCount, &iUser2SubPktCount, &iUser3SubPktCount);
            NetPrintf(("voipconnection: [%d] sending voip pkt (seq#: %d) with %d sub-pkt(s) of size %d (bytes) to %a:%d\n",
                VOIP_ConnID(pConnectionlist, pConnection), pPacket->MicrInfo.uSeqn, pPacket->MicrInfo.uNumSubPackets, pPacket->MicrInfo.uSubPacketSize,
                SocketNtohl(pConnection->SendAddr.sin_addr.s_addr), SocketNtohs(pConnection->SendAddr.sin_port)));
            NetPrintf(("voipconnection:         (number of sub-pkt(s) per remote user -> user0:%d, user1:%d, user2:%d, user3:%d)\n",
                iUser0SubPktCount, iUser1SubPktCount, iUser2SubPktCount, iUser3SubPktCount));
        }
        #endif

        // send voice packet
        iPacketSize = sizeof(pConnection->VoipMicrPacket) - sizeof(pConnection->VoipMicrPacket.aData) + (pPacket->MicrInfo.uNumSubPackets * pPacket->MicrInfo.uSubPacketSize);
        pConnection->iSendResult = _VoipSocketSendto(pConnectionlist->pSocket, pConnectionlist->uClientId, &pConnection->VoipMicrPacket.Head,
            iPacketSize, 0, (struct sockaddr *)&pConnection->SendAddr, sizeof(pConnection->SendAddr));

        // update send info -- don't update uLastSend in server mode, to force pinging even when voice is being sent
        if (pConnection->iVoipServerConnId == VOIP_CONNID_NONE)
        {
            pConnection->uLastSend = uTick;
        }
        pConnection->uVoiceSendTimer = uTick + VOIP_MSPERPACKET;
        pConnectionlist->uLocalStatus |= VOIP_LOCAL_SENDVOICE;

        // reset packet info $$TODO - reset entire MicrInfo structure?
        pPacket->MicrInfo.uNumSubPackets = 0;
        pPacket->MicrInfo.uSubPacketSize = 0;
        memset(&pConnection->aUserSubPktCount, 0, sizeof(pConnection->aUserSubPktCount));
        memset(pPacket->MicrInfo.aDataMap, 0, sizeof(pPacket->MicrInfo.aDataMap));
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipConnectionRecvSingle

    \Description
        Handle data incoming on a connection.

    \Input *pConnectionlist - connectionlist connection belongs to
    \Input *pConnection     - pointer to connection to receive on
    \Input *pSendAddr       - remote address data was received from
    \Input *pPacket         - packet data that was received
    \Input iSize            - size of data received

    \Version 03/21/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipConnectionRecvSingle(VoipConnectionlistT *pConnectionlist, VoipConnectionT *pConnection, struct sockaddr_in *pSendAddr, VoipPacketBufferT *pPacket, int32_t iSize)
{
    int32_t iConnID = VOIP_ConnID(pConnectionlist, pConnection);
    #if DIRTYCODE_LOGGING
    uint32_t uClientId = _VoipDecodeU32(pPacket->VoipPacketHead.aClientId);
    #endif
    uint32_t uTick;

    // note time of arrival
    uTick = NetTick();

    // count the packet
    pConnection->iRecvPackets += 1;
    pConnection->iRecvData += iSize;

    // did port change?
    if (pConnection->SendAddr.sin_port != pSendAddr->sin_port)
    {
        NetPrintf(("voipconnection: [%d] port remap #%d: %d bytes from %d instead of %d\n", iConnID,
            pConnection->iChangePort++, iSize,
            SocketNtohs(pSendAddr->sin_port),
            SocketNtohs(pConnection->SendAddr.sin_port)));

        // send future to remapped port
        pConnection->SendAddr.sin_port = pSendAddr->sin_port;

        // reset counter of packets received
        pConnection->iRecvPackets = 0;
    }

    // handle incoming packet
    if (VOIP_SamePacketType(&pPacket->VoipPacketHead, &_Voip_ConnPacket))
    {
        VoipConnPacketT *pConnPacket = &pPacket->VoipConnPacket;
        uint32_t uSessionId = _VoipDecodeU32(pConnPacket->aSessionId);

        // got a connection packet
        NetPrintf(("voipconnection: [%d] received conn packet bConnected=%d eState=%d uClientId=0x%08x, uSessionId=0x%08x, user=%s\n",
            iConnID, pConnPacket->bConnected, pConnection->eState, uClientId, uSessionId,
            pConnPacket->LocalUsers[pConnPacket->iPrimaryLocalUser].strUserId));

        // copy remote userID and broadband status
        if (_VoipConnectionMatchSessionId(pConnectionlist, iConnID, uSessionId))
        {
            // copy remote user's info
            memcpy(pConnection->RemoteUsers, pConnPacket->LocalUsers, sizeof(pConnection->RemoteUsers));
            pConnection->iPrimaryRemoteUser = pConnPacket->iPrimaryLocalUser;
            _VoipDecodeFlags(pConnection, &pConnPacket->Head);

            // flag that we received Conn packet from remote party
            pConnection->bConnPktRecv = TRUE;

            // if they've received data from us, consider the connection established
            if ((pConnPacket->bConnected) && (pConnection->eState == ST_CONN))
            {
                _VoipConnectionInit(pConnectionlist, pConnection, iConnID);
            }

            // if peer no more sees us as connected... go back to ST_CONN state and hope for connection to go back up
            if ((!pConnPacket->bConnected) && (pConnection->eState == ST_ACTV))
            {
                NetPrintf(("voipconnection: [%d] voip connection lost! try to re-establish. connection back to ST_CONN state (from ST_ACTV)... \n", iConnID));
                pConnection->eState = ST_CONN;
            }

            // update last received time
            pConnection->uLastRecv = uTick;
        }
        else
        {
            #if DIRTYCODE_LOGGING
            NetPrintf(("voipconnection: [%d] ignoring conn packet with sessionId=0x%08x because of session IDs mismatch\n",
                iConnID, uSessionId));

            // print set of sessions sharing this connection
            _VoipConnectionPrintSessionIds(pConnectionlist, iConnID);

            NetPrintMem(pConnPacket, sizeof(*pConnPacket), "connection packet data");
            #endif
        }
    }
    else if (VOIP_SamePacketType(&pPacket->VoipPacketHead, &_Voip_PingPacket))
    {
        VoipPingPacketT* pPingPacket = &pPacket->VoipPingPacket;

        #if VOIP_CONNECTION_DEBUG
        NetPrintf(("voipconnection: [%d] received ping packet from clientId=0x%08x\n", iConnID, uClientId));
        #endif

        if (pConnection->bConnPktRecv)
        {
            // if we receive a ping in connection state, that means we're connected
            if (pConnection->eState == ST_CONN)
            {
                _VoipConnectionInit(pConnectionlist, pConnection, iConnID);
            }

            // update connection info
            _VoipDecodeFlags(pConnection, &pPingPacket->Head);
            pConnection->uRemoteStatus &= ~VOIP_REMOTE_RECVVOICE;
            pConnection->uRemoteStatus |= VOIP_REMOTE_BROADCONN;
            pConnection->uRecvChannelId = _VoipDecodeU32(pPingPacket->aChannelId);
            pConnection->uLastRecv = uTick;
        }
        else
        {
            NetPrintf(("voipconnection: [%d] ping packet from clientId=0x%08x ignored because conn packet not yet received\n", iConnID, uClientId));
        }
    }
    else if (VOIP_SamePacketType(&pPacket->VoipPacketHead, &_Voip_MicrPacket))
    {
        VoipMicrPacketT *pMicrPacket = &pPacket->VoipMicrPacket;

        #if VOIP_CONNECTION_DEBUG
        if ((pConnection->iRecvPackets % 30) == 0)
        {
            int32_t iUser0SubPktCount, iUser1SubPktCount, iUser2SubPktCount, iUser3SubPktCount;
            _VoipConnectionCountSubPktsPerUser(pMicrPacket, &iUser0SubPktCount, &iUser1SubPktCount, &iUser2SubPktCount, &iUser3SubPktCount);
            NetPrintf(("voipconnection: [%d] received voip pkt with %d sub-pkt(s) of size %d (bytes)\n",
                iConnID, pMicrPacket->MicrInfo.uNumSubPackets, pMicrPacket->MicrInfo.uSubPacketSize));
            NetPrintf(("voipconnection:         (number of sub-pkt(s) per remote user -> user0:%d, user1:%d, user2:%d, user3:%d)\n",
                iUser0SubPktCount, iUser1SubPktCount, iUser2SubPktCount, iUser3SubPktCount));
            NetPrintMem(&pMicrPacket->Head, iSize, "mic packet data");
        }
        #endif

        if (pConnection->bConnPktRecv)
        {
            // if we receive voice data in connection state, that means we're connected
            if (pConnection->eState == ST_CONN)
            {
                _VoipConnectionInit(pConnectionlist, pConnection, iConnID);
            }

            // update connection info
            _VoipDecodeFlags(pConnection, &pMicrPacket->Head);
            pConnection->uRemoteStatus |= (VOIP_REMOTE_ACTIVE|VOIP_REMOTE_BROADCONN|VOIP_REMOTE_RECVVOICE);
            pConnection->uRecvChannelId = _VoipDecodeU32(pMicrPacket->MicrInfo.aChannelId);
            pConnection->uLastRecv = uTick;

            // validate and update packet sequence number
            if (pMicrPacket->MicrInfo.uSeqn != (uint8_t)(pConnection->uRecvSeqn+1))
            {
                NetPrintf(("voipconnection: [%d] got packet seq # %d while expecting packet seq # %d\n",
                    iConnID, pMicrPacket->MicrInfo.uSeqn, (uint8_t)pConnection->uRecvSeqn+1));
            }
            pConnection->uRecvSeqn = pMicrPacket->MicrInfo.uSeqn;

            // if we don't have them muted, forward voice data to registered callback
            if (pConnectionlist->uRecvMask & (1 << iConnID))
            {
                pConnectionlist->pVoiceCb(pConnection->RemoteUsers, pMicrPacket, pConnectionlist->pCbUserData);
            }
        }
        else
        {
            NetPrintf(("voipconnection: [%d] voip data packet from clientId=0x%08x ignored because conn packet not yet received\n", iConnID, uClientId));
        }
    }
    else if (VOIP_SamePacketType(&pPacket->VoipPacketHead, &_Voip_DiscPacket))
    {
        VoipDiscPacketT *pDiscPacket = &pPacket->VoipDiscPacket;
        uint32_t uSessionId = _VoipDecodeU32(pDiscPacket->aSessionId);
        uint32_t uRemoteClientId = _VoipDecodeU32(pDiscPacket->aRemoteClientId);

        if (_VoipConnectionMatchSessionId(pConnectionlist, iConnID, uSessionId))
        {
            if (uRemoteClientId == pConnectionlist->uClientId)
            {
                NetPrintf(("voipconnection: [%d] remote peer 0x%08x has disconnected from 0x%08x; sessionid in disc packet is %d\n", iConnID, uClientId, uRemoteClientId, uSessionId));

                // In this specific case, we know that remote peer is already in 'disconnected' state.
                // No need to send back a DISC msg. Therefore, set bSendDiscMsg parameter to FALSE.
                _VoipConnectionStop(pConnectionlist, &pConnectionlist->pConnections[iConnID], iConnID, FALSE);
            }
            else
            {
                // this should only occurs with multiple machines on the same PC and voip being routed to the incorrect client
                NetPrintf(("voipconnection: [%d] remote peer 0x%08x has disconnected from 0x%08x; but we are 0x%08x\n", iConnID, uClientId, uRemoteClientId, pConnectionlist->uClientId));
            }
        }
        else
        {
            #if DIRTYCODE_LOGGING
            NetPrintf(("voipconnection: [%d] ignoring disc packet with sessionId=0x%08x because of session IDs mismatch\n",
                iConnID, uSessionId));

            // print set of sessions sharing this connection
            _VoipConnectionPrintSessionIds(pConnectionlist, iConnID);
            #endif
        }
    }
    else
    {
        NetPrintf(("voipconnection: [%d] received unknown packet type 0x%02x%02x%02x\n", iConnID,
            pPacket->VoipPacketHead.aType[0], pPacket->VoipPacketHead.aType[1], pPacket->VoipPacketHead.aType[2]));
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipConnectionUpdateSingle

    \Description
        Update a virtual connection.

    \Input *pConnectionlist - connectionlist
    \Input *pConnection     - pointer to connection to update
    \Input iConnId          - index of connection to update
    \Input uTick            - current tick count

    \Output
        uint32_t            - nonzero=receiving voice, zero=not receiving voice

    \Version 03/20/2004 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _VoipConnectionUpdateSingle(VoipConnectionlistT *pConnectionlist, VoipConnectionT *pConnection, int32_t iConnId, uint32_t uTick)
{
    int32_t iTimeout;

    // make sure it's a valid connection
    if (pConnection->eState == ST_DISC)
    {
        return(0);
    }

    // see if we need to send a conn or ping packet
    if ((uTick - pConnection->uLastSend) > VOIP_PING_RATE)
    {
        // if we're connecting, send connection packet
        if (pConnection->eState == ST_CONN)
        {
            _VoipConnectionConn(pConnectionlist, pConnection, uTick);
        }
        else
        {
            _VoipConnectionPing(pConnectionlist, pConnection, uTick);
        }
    }

    // see if we need to send any buffered voice data
    _VoipConnectionSendSingle(pConnectionlist, pConnection, uTick, FALSE);

    // see if we need to time out the connection
    iTimeout = (pConnection->eState == ST_ACTV) ? pConnectionlist->iDataTimeout : VOIP_CONNTIMEOUT;
    if (NetTickDiff(uTick, pConnection->uLastRecv) > iTimeout)
    {
        NetPrintf(("voipconnection: [%d] timing out connection due to inactivity\n", iConnId));
        _VoipConnectionStop(pConnectionlist, &pConnectionlist->pConnections[iConnId], iConnId, TRUE);
    }

    #if defined(DIRTYCODE_XENON)
    // unreachable send error on xenon are not fatal if we can switch to a reachable fallback address
    if (pConnection->iSendResult == SOCKERR_UNREACH)
    {
        NetPrintf(("voipconnection: [%d] error %d when sending to %a\n", iConnId, pConnection->iSendResult, SocketNtohl(pConnection->SendAddr.sin_addr.s_addr)));
        pConnection->iSendResult = 0;

        // mark active SendAddr as unreachable
        _VoipSendAddrFallbackMarkUnreachable(pConnectionlist, iConnId, SocketNtohl(pConnection->SendAddr.sin_addr.s_addr));

        // try to replace active SendAddr with one from the fallback list
        if (_VoipConnectionReplaceActiveSendAddr(pConnectionlist, iConnId) != 0)
        {
            // no success replacing the active SendAddr, so just close the connection
            // parameter bSendDiscMsg set to FALSE because there is no need to send a DISC msg as the remote addr is unreachable.
            _VoipConnectionStop(pConnectionlist, &pConnectionlist->pConnections[iConnId], iConnId, FALSE);
        }
    }
    #endif

    // return if we are receiving voice or not
    return((pConnection->uRemoteStatus & VOIP_REMOTE_RECVVOICE) ? (1 << iConnId) : 0);
}

/*F********************************************************************************/
/*!
    \Function _VoipConnectionRecv

    \Description
        Receive data and forward it to the appropriate connection.

    \Input *pConnectionlist - connectionlist

    \Output
        int32_t             - amount of data received

    \Version 03/21/2004 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipConnectionRecv(VoipConnectionlistT *pConnectionlist)
{
    struct sockaddr_in RecvAddr;
    int32_t iAddrLen, iRecvSize;

    VoipPacketBufferT RecvPacket;

    // no socket?  no way to receive.
    if (pConnectionlist->pSocket == NULL)
    {
        return(0);
    }

    // try and receive from a peer
    if ((iRecvSize = SocketRecvfrom(pConnectionlist->pSocket, (char *)&RecvPacket, sizeof(RecvPacket), 0, (struct sockaddr *)&RecvAddr, (iAddrLen=sizeof(RecvAddr),&iAddrLen))) > 0)
    {
        VoipConnectionT *pConnection;
        int32_t iConnID;

        // extract client identifier
        uint32_t uClientId = _VoipDecodeU32(RecvPacket.VoipPacketHead.aClientId);

        // find the matching connection
        for (iConnID = 0, pConnection = NULL; iConnID < pConnectionlist->iMaxConnections; iConnID++)
        {
            pConnection = &pConnectionlist->pConnections[iConnID];

            // if the clientID matches, this is the connection
            if (uClientId == pConnection->uClientId)
            {
                break;
            }
        }

        // if we found a matching connection
        if (iConnID < pConnectionlist->iMaxConnections)
        {
            // give the data to the connection
            _VoipConnectionRecvSingle(pConnectionlist, pConnection, &RecvAddr, &RecvPacket, iRecvSize);
        }
        else if (!VOIP_SamePacketType(&RecvPacket.VoipPacketHead, &_Voip_ConnPacket) && !VOIP_SamePacketType(&RecvPacket.VoipPacketHead, &_Voip_DiscPacket))
        {
            NetPrintf(("voipconnection: ignoring 0x%02x%02x%02x packet from address %a:%d clientId=0x%08x\n",
                RecvPacket.VoipPacketHead.aType[0], RecvPacket.VoipPacketHead.aType[1], RecvPacket.VoipPacketHead.aType[2],
                SocketNtohl(RecvAddr.sin_addr.s_addr), SocketNtohs(RecvAddr.sin_port),
                uClientId));
            NetPrintMem(&RecvPacket, iRecvSize, "packet data");
        }
    }

    // return data length
    return(iRecvSize);
}

/*F********************************************************************************/
/*!
    \Function _VoipConnectionRecvCallback

    \Description
        Voip socket recv event callback.

    \Input *pSocket - voip socket
    \Input iFlags   - unused
    \Input *pRef    - connectionlist ref

    \Output
        int32_t     - zero

    \Version 12/16/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipConnectionRecvCallback(SocketT *pSocket, int32_t iFlags, void *pRef)
{
    VoipConnectionlistT *pConnectionlist = (VoipConnectionlistT *)pRef;

    // see if we have exclusive access
    if (NetCritTry(&pConnectionlist->NetCrit))
    {
        // try and receive data
        _VoipConnectionRecv(pConnectionlist);

        // free access
        NetCritLeave(&pConnectionlist->NetCrit);
    }
    else
    {
        NetPrintf(("voipconnection: _VoipConnectionRecvCallback() could not acquire connectionlist critical section\n"));
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipConnectionTrySocketClose

    \Description
        Check if the voip socket can be closed. The close will occur when all the
        connections are closed. Moved from the voip loop, to also be done immediately
        upon connection stop.

    \Input *pConnectionlist - connectionlist ref

    \Output
        int32_t     - zero

    \Version 06/18/2009 (jrainy)
*/
/********************************************************************************F*/
static void _VoipConnectionTrySocketClose(VoipConnectionlistT *pConnectionlist)
{
    int32_t iConnId, iNumConnections;

    NetCritEnter(&pConnectionlist->NetCrit);

    // count number of active connections
    for (iConnId = 0, iNumConnections = 0; iConnId < pConnectionlist->iMaxConnections; iConnId++)
    {
        if (pConnectionlist->pConnections[iConnId].eState != ST_DISC)
        {
            iNumConnections++;
        }
    }

    // if no connections left, kill socket
    if ((iNumConnections == 0) && (pConnectionlist->pSocket != NULL))
    {
        NetPrintf(("voipconnection: closing socket in _VoipConnectionTrySocketClose()\n"));
        SocketClose(pConnectionlist->pSocket);
        pConnectionlist->pSocket = NULL;
        pConnectionlist->uBindPort = 0;
    }
    NetCritLeave(&pConnectionlist->NetCrit);
}

/*F********************************************************************************/
/*!
    \Function _VoipConnectionStop

    \Description
        Stop a connection.

    \Input *pConnectionlist - connectionlist ref
    \Input *pConnection     - connection to stop
    \Input iConnID          - index of connection to stop, or VOIP_CONNID_ALL to stop all connections
    \Input bSendDiscMsg     - TRUE - send DISC msg to peer; FALSE - do not send DISC msg to peer

    \Notes
        Loss of the single unreliable disconnect packet sent in this function
        is not critical, as the connection manager will respond to unexpected
        packets that are not connect/disconnect packets (ie pings and voice
        packets) with disconnection requests.

    \Version 05/10/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipConnectionStop(VoipConnectionlistT *pConnectionlist, VoipConnectionT *pConnection, int32_t iConnID, int32_t bSendDiscMsg)
{
    NetCritEnter(&pConnectionlist->NetCrit);

    // make sure we're not already disconnected
    if (pConnection->eState == ST_DISC)
    {
        NetPrintf(("voipconnection: [%d] disconnection attempt canceled because connection already in state ST_DISC!\n", iConnID));
        NetCritLeave(&pConnectionlist->NetCrit);
        return;
    }

    if (bSendDiscMsg)
    {
        // send a disconnection packet
        _VoipConnectionDisc(iConnID, pConnectionlist->uClientId, pConnection->uClientId, pConnection->uSessionId[0], &pConnection->SendAddr, pConnectionlist->pSocket);
    }
    else
    {
        NetPrintf(("voipconnection: [%d] no need to send disconnect message to %a:%d\n", iConnID,
            SocketNtohl(pConnection->SendAddr.sin_addr.s_addr), SocketNtohs(pConnection->SendAddr.sin_port)));
    }

    // set to disconnected before unregister
    pConnection->eState = ST_DISC;
    pConnection->bConnPktRecv = FALSE;

    // subtract connection identifier from send/recv masks
    VoipConnectionSetSendMask(pConnectionlist, pConnectionlist->uSendMask & ~(1 << iConnID));
    VoipConnectionSetRecvMask(pConnectionlist, pConnectionlist->uRecvMask & ~(1 << iConnID));

    // clear connection and mark as disconnected
    memset(pConnection, 0, sizeof(*pConnection));
    pConnection->uRemoteStatus = VOIP_REMOTE_DISCONNECTED;
    pConnection->iVoipServerConnId = VOIP_CONNID_NONE;
    pConnectionlist->uLocalStatus &= ~VOIP_LOCAL_SENDVOICE;

    NetCritLeave(&pConnectionlist->NetCrit);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function VoipConnectionStartup

    \Description
        Startup a connectionlist.

    \Input *pConnectionlist - connectionlist to init
    \Input iMaxPeers        - max number of connections to support

    \Output
        int32_t             - zero=success, negative=error

    \Version 03/20/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipConnectionStartup(VoipConnectionlistT *pConnectionlist, int32_t iMaxPeers)
{
    int32_t iConnId;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    VoipMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init connectionlist
    if ((pConnectionlist->pConnections = (VoipConnectionT *)DirtyMemAlloc(sizeof(VoipConnectionT) * iMaxPeers, VOIP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voipconnection: error, unable to allocate connectionlist\n"));
        return(-1);
    }
    memset(pConnectionlist->pConnections, 0, sizeof(VoipConnectionT) * iMaxPeers);

    pConnectionlist->iFriendConnId = VOIP_CONNID_NONE;

    // reset local-to-server conn id mappings
    for (iConnId = 0; iConnId < iMaxPeers; iConnId++)
    {
        pConnectionlist->pConnections[iConnId].iVoipServerConnId = VOIP_CONNID_NONE;
    }

    // init critical section
    NetCritInit(&pConnectionlist->NetCrit, "voipconnection");

    // set default timeout
    pConnectionlist->iDataTimeout = VOIP_TIMEOUT;

    // set max peers
    pConnectionlist->iMaxConnections = iMaxPeers;

    NetPrintf(("voipconnection: iMaxConnections=%d\n", pConnectionlist->iMaxConnections));

    return(0);
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionSetCallbacks

    \Description
        Set required recv voice and reg user callbacks.

    \Input *pConnectionlist - connectionlist to register callbacks for
    \Input *pVoiceCb        - voice callback to register
    \Input *pRegUserCb      - user reg callback to register
    \Input *pUserData       - pointer to user data

    \Version 03/20/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipConnectionSetCallbacks(VoipConnectionlistT *pConnectionlist, VoipConnectRecvVoiceCbT *pVoiceCb, VoipConnectRegUserCbT *pRegUserCb, void *pUserData)
{
    // save callbacks and callback user data
    pConnectionlist->pRegUserCb = pRegUserCb;
    pConnectionlist->pVoiceCb = pVoiceCb;
    pConnectionlist->pCbUserData = pUserData;
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionClose

    \Description
        Shutdown a connectionlist

    \Input *pConnectionlist - connectionlist to close

    \Version 03/20/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipConnectionShutdown(VoipConnectionlistT *pConnectionlist)
{
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    VoipMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // dispose of connectionlist
    DirtyMemFree(pConnectionlist->pConnections, VOIP_MEMID, iMemGroup, pMemGroupUserData);

    // dispose of socket
    if (pConnectionlist->pSocket)
    {
        NetPrintf(("voipconnection: closing socket in VoipConnectionShutdown()\n"));
        SocketClose(pConnectionlist->pSocket);
    }

    // release critical section
    NetCritKill(&pConnectionlist->NetCrit);

    // clear connectionlist
    memset(pConnectionlist, 0, sizeof(*pConnectionlist));
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionCanAllocate

    \Description
        Check whether a given Connection ID can be allocated in this Connection List

    \Input *pConnectionlist - connection list ref
    \Input iConnID          - connection index

    \Output
        uint8_t             - Whether a connection can be allocated with given ConnID

    \Version 02/19/2008 (jrainy)
*/
/********************************************************************************F*/
uint8_t VoipConnectionCanAllocate(VoipConnectionlistT *pConnectionlist, int32_t iConnID)
{
    if ((iConnID < 0) || (iConnID >= pConnectionlist->iMaxConnections))
    {
        return(FALSE);
    }

    return(pConnectionlist->pConnections[iConnID].eState == ST_DISC);
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionStart

    \Description
        Start a connection to a peer.

    \Input *pConnectionlist - connection list ref
    \Input iConnID          - connection index
    \Input *pUserID         - userID of user we are connecting to
    \Input uAddr            - address to connect to
    \Input uConnPort        - connection port
    \Input uBindPort        - local bind port
    \Input uClientId        - id of client to connect to (cannot be 0)
    \Input uSessionId       - session id (optional)

    \Output
        int32_t             - connection identifier on success, negative=failure

    \Version 03/18/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipConnectionStart(VoipConnectionlistT *pConnectionlist, int32_t iConnID, VoipUserT *pUserID, uint32_t uAddr, uint32_t uConnPort, uint32_t uBindPort, uint32_t uClientId, uint32_t uSessionId)
{
    VoipConnectionT *pConnection;
    struct sockaddr BindAddr;

    // make sure we have local and remote clientIDs
    if ((pConnectionlist->uClientId == 0) || (uClientId == 0))
    {
        NetPrintf(("voipconnection: non-zero local client id (%d) and remote client id (%d) required for connection\n",
            pConnectionlist->uClientId, uClientId));
        return(-1);
    }

    // if we haven't allocated a socket yet, do so now
    if (pConnectionlist->pSocket == NULL)
    {
        int32_t iResult;

        // open the socket
        if ((pConnectionlist->pSocket = SocketOpen(AF_INET, SOCK_DGRAM, VOIP_IPPROTO)) == NULL)
        {
            NetPrintf(("voipconnection: error creating socket\n"));
            return(-2);
        }

        // bind the socket
        SockaddrInit(&BindAddr, AF_INET);
        SockaddrInSetPort(&BindAddr, uBindPort);
        if ((iResult = SocketBind(pConnectionlist->pSocket, &BindAddr, sizeof(BindAddr))) != SOCKERR_NONE)
        {
            NetPrintf(("voipconnection: error %d binding socket to port %d, trying random\n", iResult, uBindPort));
            SockaddrInSetPort(&BindAddr, 0);
            if ((iResult = SocketBind(pConnectionlist->pSocket, &BindAddr, sizeof(BindAddr))) != SOCKERR_NONE)
            {
                NetPrintf(("voipconnection: error %d binding socket\n", iResult));
                SocketClose(pConnectionlist->pSocket);
                pConnectionlist->pSocket = NULL;
                return(-3);
            }
        }

        // retrieve bound port
        SocketInfo(pConnectionlist->pSocket, 'bind', 0, &BindAddr, sizeof(BindAddr));
        uBindPort = SockaddrInGetPort(&BindAddr);
        NetPrintf(("voipconnection: bound socket to port %d\n", uBindPort));

        // save local port
        pConnectionlist->uBindPort = uBindPort;

        // setup for socket events
        SocketCallback(pConnectionlist->pSocket, CALLB_RECV, 5000, pConnectionlist, &_VoipConnectionRecvCallback);
    }

    // make sure bind matches our previous bind
    if (uBindPort != pConnectionlist->uBindPort)
    {
        NetPrintf(("voipconnection: warning, only one global bind port is currently supported, using previously specified port\n"));
    }

    // allocate a connection
    if ((pConnection = _VoipConnectionAllocate(pConnectionlist, iConnID, pUserID)) == NULL)
    {
        NetPrintf(("voipconnection: alloc failed\n"));
        return(-4);
    }
    iConnID = VOIP_ConnID(pConnectionlist, pConnection);

    if (pConnectionlist->iFriendConnId == VOIP_CONNID_NONE)
    {
        pConnectionlist->iFriendConnId = iConnID;
    }
    else
    {
        pConnectionlist->iFriendConnId = VOIP_CONNID_ALL;
    }


    // convert address and ports to network form
    uAddr = SocketHtonl(uAddr);
    uConnPort = SocketHtons(uConnPort);

    // init the connection
    memset(pConnection, 0, sizeof(*pConnection));
    pConnection->iVoipServerConnId = VOIP_CONNID_NONE;
    pConnection->SendAddr.sin_family = AF_INET;
    pConnection->SendAddr.sin_addr.s_addr = uAddr;
    pConnection->SendAddr.sin_port = uConnPort;
    pConnection->uClientId = uClientId;

    // add specified session ID to the set of sessions sharing this connection
    VoipConnectionAddSessionId(pConnectionlist, iConnID, uSessionId);

    #if (defined(DIRTYCODE_XENON))
        strnzcpy(pConnection->RemoteUsers[0].strUserId, pUserID->strUserId, 18);
    #else
        memcpy(&pConnection->RemoteUsers[0], pUserID, sizeof(pConnection->RemoteUsers[0]));
    #endif

    pConnection->uRecvSeqn = 0xFFFFFFFF;   // intialize last receive sequence number with -1.
    pConnection->uRemoteStatus = 0;
    pConnection->eState = ST_CONN;
    pConnection->uLastRecv = NetTick();

    #if (defined(DIRTYCODE_XENON))
    // add this address in the list of fallbacks
    VoipConnectionAddFallbackAddr(pConnectionlist, iConnID, uAddr);
    #endif

    // set up to transmit mic data
    memcpy(&pConnection->VoipMicrPacket.Head, &_Voip_MicrPacket, sizeof(VoipPacketHeadT));

    // when a connection is created, the corresponding bit in application-specified send/recv masks always defaults to ON
    // it is the game code responsibility to alter this upon voip connection establishment if needed
    VoipCommonAddMask(&pConnectionlist->uUserSendMask, 1 << iConnID, "usendmask");
    VoipCommonAddMask(&pConnectionlist->uUserRecvMask, 1 << iConnID, "urecvmask");

    // output connect message
    NetPrintf(("voipconnection: [%d] connecting to user=%s %a:%d clientId=0x%08x\n", iConnID,
        pConnection->RemoteUsers[pConnection->iPrimaryRemoteUser].strUserId,
        SocketNtohl(pConnection->SendAddr.sin_addr.s_addr), SocketNtohs(pConnection->SendAddr.sin_port),
        pConnection->uClientId));

    // return connection ID to caller
    return(iConnID);
}

/*F********************************************************************************/
/*!
    \Function   VoipConnectionUpdate

    \Description
        Update all connections

    \Input *pConnectionlist - connection list

    \Version 03/18/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipConnectionUpdate(VoipConnectionlistT *pConnectionlist)
{
    int32_t iConnId;
    uint32_t uTick;

    // get sole access
    NetCritEnter(&pConnectionlist->NetCrit);

    // receive any incoming data
    while(_VoipConnectionRecv(pConnectionlist) > 0)
        ;

    _VoipConnectionTrySocketClose(pConnectionlist);

    // relinquish sole access
    NetCritLeave(&pConnectionlist->NetCrit);

    // update connection status for all connections, keeping track of who we are receiving voice from
    for (iConnId = 0, uTick = NetTick(), pConnectionlist->uRecvVoice = 0; iConnId < pConnectionlist->iMaxConnections; iConnId++)
    {
        pConnectionlist->uRecvVoice |= _VoipConnectionUpdateSingle(pConnectionlist, &pConnectionlist->pConnections[iConnId], iConnId, uTick);
    }

    // check if we need to time out talking status
    if ((pConnectionlist->uLocalStatus & VOIP_LOCAL_TALKING) && (NetTickDiff(uTick, pConnectionlist->uLastVoiceTime) > VOIP_TALKTIMEOUT))
    {
        pConnectionlist->uLocalStatus &= ~VOIP_LOCAL_TALKING;
    }
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionStop

    \Description
        Stop a connection with a peer.

    \Input *pConnectionlist - connection list ref
    \Input iConnID          - connection to stop, or VOIP_CONNID_ALL to stop all connections
    \Input bSendDiscMsg     - TRUE - send DISC msg to peer; FALSE - do not send DISC msg to peer

    \Version 03/18/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipConnectionStop(VoipConnectionlistT *pConnectionlist, int32_t iConnID, int32_t bSendDiscMsg)
{
    if (iConnID == VOIP_CONNID_ALL)
    {
        // disconnect from all current connections
        for (iConnID = 0; iConnID < pConnectionlist->iMaxConnections; iConnID++)
        {
            _VoipConnectionStop(pConnectionlist, &pConnectionlist->pConnections[iConnID], iConnID, bSendDiscMsg);
        }
    }
    else if ((iConnID >= 0) && (iConnID < pConnectionlist->iMaxConnections))
    {
        // disconnect from the given connection
        _VoipConnectionStop(pConnectionlist, &pConnectionlist->pConnections[iConnID], iConnID, bSendDiscMsg);
    }
    else
    {
        NetPrintf(("voipconnection: disconnect with iConnID=%d is invalid\n", iConnID));
    }

    _VoipConnectionTrySocketClose(pConnectionlist);
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionRemove

    \Description
        Remove specified connection.

    \Input *pConnectionlist - connection list ref
    \Input iConnID          - connection to remove

    \Version 05/24/2007 (jbrookes)
*/
/********************************************************************************F*/
void VoipConnectionRemove(VoipConnectionlistT *pConnectionlist, int32_t iConnID)
{
    VoipConnectionT * pConnection;

    if ((iConnID >= 0) && (iConnID < pConnectionlist->iMaxConnections))
    {
        uint32_t uConnMask = 1 << iConnID;

        pConnection = &pConnectionlist->pConnections[iConnID];

        // call VoipConnectionSetRecvMask() to unregister the user
        VoipConnectionSetRecvMask(pConnectionlist, pConnectionlist->uRecvMask & uConnMask);

        // remove entry from masks
        VoipCommonDelMask(&pConnectionlist->uSendMask, uConnMask, "sendmask");
        VoipCommonDelMask(&pConnectionlist->uRecvMask, uConnMask, "recvmask");
        VoipCommonDelMask(&pConnectionlist->uRecvVoice, uConnMask, "recvvoice");

        // remove entry from application-specified masks
        VoipCommonDelMask(&pConnectionlist->uUserSendMask, uConnMask, "usendmask");
        VoipCommonDelMask(&pConnectionlist->uUserRecvMask, uConnMask, "urecvmask");
    }
    else
    {
        NetPrintf(("voipconnection: removal with iConnID=%d is invalid\n", iConnID));
    }
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionSend

    \Description
        Send data to peer.

    \Input *pConnectionlist - connectionlist to send to
    \Input *pVoiceData      - pointer to data to send
    \Input iDataSize        - size of data to send
    \Input uLocalIndex      - local user index
    \Input uSendSeqn        - seq nb

    \Version 03/17/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipConnectionSend(VoipConnectionlistT *pConnectionlist, const uint8_t *pVoiceData, int32_t iDataSize, uint32_t uLocalIndex, uint8_t uSendSeqn)
{
    uint32_t uCurTick = NetTick(), uDataMap;
    int32_t iConnID;
    uint8_t bSentToVoipServer = FALSE;

    // set talking flag
    pConnectionlist->uLocalStatus |= VOIP_LOCAL_TALKING;
    pConnectionlist->uLastVoiceTime = uCurTick;

    // loop through all connections
    for (iConnID = 0; iConnID < pConnectionlist->iMaxConnections; iConnID++)
    {
        // ref the connection
        VoipConnectionT *pConnection = &pConnectionlist->pConnections[iConnID];

        // for connections via voip server, send only to the first connection
        if ((pConnection->iVoipServerConnId != VOIP_CONNID_NONE) && (bSentToVoipServer != FALSE))
        {
            continue;
        }

        // is the connection active, and are we sending to it?
        if ((pConnection->eState == ST_ACTV) && (pConnectionlist->uSendMask & (1 << iConnID)))
        {
            // ref packet
            VoipMicrPacketT *pMicrPacket = &pConnection->VoipMicrPacket;

            // if no voice packets are queued and the voice timer has elapsed, reset it
            if ((pMicrPacket->MicrInfo.uNumSubPackets == 0) && (NetTickDiff(uCurTick, pConnection->uVoiceSendTimer) >= 0))
            {
                pConnection->uVoiceSendTimer = uCurTick + VOIP_MSPERPACKET;
            }

            // queue the data up if we have room for it
            if (pMicrPacket->MicrInfo.uNumSubPackets < VOIP_MAXSUBPKTS_PER_PKT)
            {
                // make sure sub-packet size is initialized
                #if DIRTYCODE_DEBUG
                if (pMicrPacket->MicrInfo.uSubPacketSize != 0 && pMicrPacket->MicrInfo.uSubPacketSize != iDataSize)
                {
                    NetPrintf(("voipconnection: critical error - sub-packets have different size -> %d vs %d\n", pMicrPacket->MicrInfo.uSubPacketSize, iDataSize));
                }
                #endif
                pMicrPacket->MicrInfo.uSubPacketSize = (unsigned)iDataSize;

                // copy data into packet buffer
                memcpy(&pMicrPacket->aData[pMicrPacket->MicrInfo.uNumSubPackets*pMicrPacket->MicrInfo.uSubPacketSize], pVoiceData, iDataSize);
                pMicrPacket->MicrInfo.uNumSubPackets += 1;
                pConnection->aUserSubPktCount[uLocalIndex] += 1;

                // add local user index to aDataMap bitfield
                uDataMap = (uint32_t)((pMicrPacket->MicrInfo.aDataMap[0] << 24) | (pMicrPacket->MicrInfo.aDataMap[1] << 16) |
                    (pMicrPacket->MicrInfo.aDataMap[2] << 8) | pMicrPacket->MicrInfo.aDataMap[3]);
                uDataMap <<= 2;
                uDataMap |= uLocalIndex;
                pMicrPacket->MicrInfo.aDataMap[0] = (uDataMap >> 24) & 0xff;
                pMicrPacket->MicrInfo.aDataMap[1] = (uDataMap >> 16) & 0xff;
                pMicrPacket->MicrInfo.aDataMap[2] = (uDataMap >> 8) & 0xff;
                pMicrPacket->MicrInfo.aDataMap[3] = uDataMap & 0xff;
            }
            else
            {
                NetPrintf(("voipconnection: critical error - no more space for a sub-packet in the voip packet (sub-packet count = %d)\n", pMicrPacket->MicrInfo.uNumSubPackets));
            }

            // see if we need to send any buffered voice data
            _VoipConnectionSendSingle(pConnectionlist, pConnection, uCurTick, FALSE);

            if (pConnection->iVoipServerConnId != VOIP_CONNID_NONE)
            {
                // mark game server as served
                bSentToVoipServer = TRUE;
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionFlush

    \Description
        Send currently queued voice data, if any.

    \Input *pConnectionlist - connectionlist to send to
    \Input iConnID          - connection ident of connection to flush

    \Output
        int32_t             - number of voice packets flushed

    \Notes
        Currently only useful on Xenon

    \Version 01/04/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipConnectionFlush(VoipConnectionlistT *pConnectionlist, int32_t iConnID)
{
    // ref the connection
    VoipConnectionT *pConnection = &pConnectionlist->pConnections[iConnID];
    int32_t iNumPackets = (signed)pConnection->VoipMicrPacket.MicrInfo.uNumSubPackets;

    // is the connection active, and are we sending to it?
    if ((pConnection->eState == ST_ACTV) && (pConnectionlist->uSendMask & (1 << iConnID)) && (iNumPackets > 0))
    {
        // see if we need to send any buffered voice data
        _VoipConnectionSendSingle(pConnectionlist, pConnection, NetTick(), TRUE);
    }
    else
    {
        iNumPackets = 0;
    }

    // return count to caller
    return(iNumPackets);
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionRegisterRemoteTalkers

    \Description
        Register/unregister remote users associated with the given connection.

    \Input *pConnectionlist - connectionlist to send to
    \Input iConnID          - connection ident of connection to flush
    \Input bRegister        - if TRUE register talkers, else unregister them

    \Version 01/04/2006 (jbrookes)
*/
/********************************************************************************F*/
void VoipConnectionRegisterRemoteTalkers(VoipConnectionlistT *pConnectionlist, int32_t iConnID, uint32_t bRegister)
{
    VoipConnectionT *pConnection = &pConnectionlist->pConnections[iConnID];
    int32_t iUser;

    // register any remote users that are part of the connection
    for (iUser = 0; iUser < VOIP_MAXLOCALUSERS; iUser++)
    {
        // if no user, don't register
        if (VOIP_NullUser(&pConnection->RemoteUsers[iUser]))
        {
            continue;
        }

        // register the user
        pConnectionlist->pRegUserCb(&pConnection->RemoteUsers[iUser], iUser, bRegister, pConnectionlist->pCbUserData);
        // set mute list to be updated
        pConnectionlist->bUpdateMute = TRUE;
    }
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionSetSendMask

    \Description
        Set connections to send to.

    \Input *pConnectionlist - connectionlist to send to
    \Input uSendMask        - connection send mask

    \Version 03/22/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipConnectionSetSendMask(VoipConnectionlistT *pConnectionlist, uint32_t uSendMask)
{
    if (pConnectionlist->uSendMask != uSendMask)
    {
        VoipCommonSetMask(&pConnectionlist->uSendMask, uSendMask, "sendmask");
    }
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionSetRecvMask

    \Description
        Set connections to receive from.

    \Input *pConnectionlist - connectionlist to send to
    \Input uRecvMask        - connection receive mask

    \Version 03/22/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipConnectionSetRecvMask(VoipConnectionlistT *pConnectionlist, uint32_t uRecvMask)
{
    if (pConnectionlist->uRecvMask != uRecvMask)
    {
        uint32_t uConnMask;
        uint8_t bRegister;
        int32_t iConnID;

        for (iConnID = 0; iConnID < pConnectionlist->iMaxConnections; iConnID++)
        {
            uConnMask = (unsigned)(1 << iConnID);

            // if it's changed
            if (uConnMask & (pConnectionlist->uRecvMask ^ uRecvMask))
            {
                // ref the connection
                VoipConnectionT *pConnection = &pConnectionlist->pConnections[iConnID];
                bRegister = (uConnMask & uRecvMask) != 0;

                // update user registration (only if we're connected or if we're unregistering)
                if ((pConnection->eState == ST_CONN) && (bRegister == TRUE))
                {
                    NetPrintf(("voipconnection: [%d] setrecvmask skip register during connect\n", VOIP_ConnID(pConnectionlist, pConnection)));
                    uRecvMask &= ~(1 << iConnID);
                }
                else if ((pConnection->eState != ST_DISC) || (bRegister == FALSE))
                {
                    VoipConnectionRegisterRemoteTalkers(pConnectionlist, iConnID, bRegister);
                }
                else
                {
                    uRecvMask &= ~(1 << iConnID);
                }
            }
        }

        // update mask
        VoipCommonSetMask(&pConnectionlist->uRecvMask, uRecvMask, "recvmask");
    }
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionAddSessionId

    \Description
        Add a session ID the set of higher level sessions sharing the specified VoIP connection.

    \Input *pConnectionlist - connectionlist
    \Input iConnId          - connection ID
    \Input uSessionId       - session ID to be added

    \Output
        int32_t             - 0 for success, negative for failure

    \Notes
        Maintaning a set of session IDs per voip connection is required to support cases
        where two consoles are involved in a P2P game and a pg simultaneously. Both of these
        constructs will have a different session ID over the same shared voip connection.
        Under some specific race conditions affecting the order at which Blaze messages are
        processed by each game client, it is very possible that the pg session id is setup
        first on one side and second on the other side thus leading to VOIP connectivity
        failures if the voip connection construct is not supporting multiple concurrent
        session IDs.

    \Version 08/30/2011 (mclouatre)
*/
/********************************************************************************F*/
int32_t VoipConnectionAddSessionId(VoipConnectionlistT *pConnectionlist, int32_t iConnId, uint32_t uSessionId)
{
    int32_t iSessionIndex;
    int32_t iRetCode = -1;
    VoipConnectionT *pConnection = &pConnectionlist->pConnections[iConnId];

    NetCritEnter(&pConnectionlist->NetCrit);

    for (iSessionIndex = 0; iSessionIndex < VOIP_MAXSESSIONIDS; iSessionIndex++)
    {
        if (pConnection->uSessionId[iSessionIndex] == 0)
        {
            // free spot, insert session ID here
            pConnection->uSessionId[iSessionIndex] = uSessionId;

            NetPrintf(("voipconnection: [%d] added 0x%08x to set of sessions sharing this voip connection\n", iConnId, uSessionId));

            iRetCode = 0;
            break;
        }
    }

    if (iRetCode != 0)
    {
        NetPrintf(("voipconnection: [%d] warning - 0x%08x could not be added to set of sessions sharing this voip connection because the list is full.\n", iConnId, uSessionId));
    }

    #if DIRTYCODE_LOGGING
    // print set of sessions sharing this connection
    _VoipConnectionPrintSessionIds(pConnectionlist, iConnId);
    #endif

    NetCritLeave(&pConnectionlist->NetCrit);

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionDeleteSessionId

    \Description
        Delete a session ID from the set of sessions sharing this voip connection.

    \Input *pConnectionlist - connectionlist
    \Input iConnId          - connection id
    \Input uSessionId       - session ID to be deleted

    \Output
           int32_t          - 0 for success, negative for failure

    \Version 08/30/2011 (mclouatre)
*/
/********************************************************************************F*/
int32_t VoipConnectionDeleteSessionId(VoipConnectionlistT *pConnectionlist, int32_t iConnId, uint32_t uSessionId)
{
    int32_t iSessionIndex;
    int32_t iRetCode = -1;
    VoipConnectionT *pConnection = &pConnectionlist->pConnections[iConnId];

    NetCritEnter(&pConnectionlist->NetCrit);

    // now delete this address from the list of fallbacks
    for(iSessionIndex = 0; iSessionIndex < VOIP_MAXSESSIONIDS; iSessionIndex++)
    {
        if (pConnection->uSessionId[iSessionIndex] == uSessionId)
        {
            int32_t iSessionIndex2;

            // move all following session IDs one cell backward in the array
            for(iSessionIndex2 = iSessionIndex; iSessionIndex2 < VOIP_MAXSESSIONIDS; iSessionIndex2++)
            {
                if (iSessionIndex2 == VOIP_MAXSESSIONIDS-1)
                {
                    // last entry, reset to 0
                    pConnection->uSessionId[iSessionIndex2] = 0;
                }
                else
                {
                    pConnection->uSessionId[iSessionIndex2] = pConnection->uSessionId[iSessionIndex2+1];
                }
            }

            NetPrintf(("voipconnection: [%d] removed 0x%08x from set of sessions sharing this voip connection\n", iConnId, uSessionId));

            iRetCode = 0;
            break;
        }
    }

    if (iRetCode != 0)
    {
        NetPrintf(("voipconnection: [%d] warning - 0x%08x not deleted because not found in set of  sessions sharing this voip connection\n", iConnId, uSessionId));
    }

    #if DIRTYCODE_LOGGING
    // print set of sessions sharing this connection
    _VoipConnectionPrintSessionIds(pConnectionlist, iConnId);
    #endif

    NetCritLeave(&pConnectionlist->NetCrit);

    return(iRetCode);
}

#if defined(DIRTYCODE_XENON)
/*F********************************************************************************/
/*!
    \Function VoipConnectionAddFallbackAddr

    \Description
        Add an address to the list of send address fallbacks

    \Input *pConnectionlist - connectionlist
    \Input iConnId          - connection ID
    \Input uAddr            - address to be added

    \Output
        int32_t             - 0 for success, negative for failure

    \Notes
        On Xenon, a single voip connection can be used by several higher-level
        ConnApis. Each one of them has a different secure address for the same client
        (uniquely identified by the client id) that they are sharing. These secure
        addresses are obtained on a per session basis. If one these sessions is
        terminated, we want the voip connection to fallback to the other secure address
        instead of terminating the connection. This is feasible because both secure
        addresses are in fact virtual encrypted tunnels to the exact same unsecure IP
        address.

    \Version 11/27/2009 (mclouatre)
*/
/********************************************************************************F*/
int32_t VoipConnectionAddFallbackAddr(VoipConnectionlistT *pConnectionlist, int32_t iConnId, uint32_t uAddr)
{
    int32_t iAddrIndex;
    int32_t iRetCode = -1;
    VoipConnectionT *pConnection = &pConnectionlist->pConnections[iConnId];

    NetCritEnter(&pConnectionlist->NetCrit);

    for (iAddrIndex = 0; iAddrIndex < VOIP_MAXADDRFALLBACKS; iAddrIndex++)
    {
        if (pConnection->SendAddrFallbacks[iAddrIndex].SendAddr == 0)
        {
            // free spot, insert address here
            pConnection->SendAddrFallbacks[iAddrIndex].SendAddr = uAddr;
            pConnection->SendAddrFallbacks[iAddrIndex].bReachable = TRUE;

            NetPrintf(("voipconnection: [%d] added %a to send addr fallbacks list for this voip connection\n", iConnId, uAddr));

            iRetCode = 0;
            break;
        }
    }

    if (iRetCode != 0)
    {
        NetPrintf(("voipconnection: [%d] warning - %a could not be added to send addr fallbacks list because the list is full.\n", iConnId, uAddr));
    }

    #if DIRTYCODE_LOGGING
    // print list of send address fallbacks for this connection
    _VoipConnectionPrintSendAddrFallbacks(pConnectionlist, iConnId);
    #endif

    NetCritLeave(&pConnectionlist->NetCrit);

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function VoipConnectionDeleteFallbackAddr

    \Description
        Delete an address from the list of send address fallbacks

    \Input *pConnectionlist - connectionlist
    \Input iConnId          - connection id
    \Input uAddr            - address to be deleted

    \Output
           int32_t          - 0 for success, negative for failure

    \Version 11/27/2009 (mclouatre)
*/
/********************************************************************************F*/
int32_t VoipConnectionDeleteFallbackAddr(VoipConnectionlistT *pConnectionlist, int32_t iConnId, uint32_t uAddr)
{
    int32_t iAddrIndex;
    int32_t iRetCode = -1;
    VoipConnectionT *pConnection = &pConnectionlist->pConnections[iConnId];

    NetCritEnter(&pConnectionlist->NetCrit);

    // if this is the send addr currently in use, replace it with another one if possible
    if (SocketNtohl(pConnection->SendAddr.sin_addr.s_addr) == uAddr)
    {
        _VoipConnectionReplaceActiveSendAddr(pConnectionlist, iConnId);
    }

    // now delete this address from the list of fallbacks
    for(iAddrIndex = 0; iAddrIndex < VOIP_MAXADDRFALLBACKS; iAddrIndex++)
    {
        if (pConnection->SendAddrFallbacks[iAddrIndex].SendAddr == uAddr)
        {
            int32_t iAddrIndex2;

            // move all following addresses one cell backward in the array
            for(iAddrIndex2 = iAddrIndex; iAddrIndex2 < VOIP_MAXADDRFALLBACKS; iAddrIndex2++)
            {
                if (iAddrIndex2 == VOIP_MAXADDRFALLBACKS-1)
                {
                    // last entry, reset to 0
                    pConnection->SendAddrFallbacks[iAddrIndex2].SendAddr = 0;
                    pConnection->SendAddrFallbacks[iAddrIndex2].bReachable = FALSE;
                }
                else
                {
                    pConnection->SendAddrFallbacks[iAddrIndex2].SendAddr = pConnection->SendAddrFallbacks[iAddrIndex2+1].SendAddr;
                    pConnection->SendAddrFallbacks[iAddrIndex2].bReachable = pConnection->SendAddrFallbacks[iAddrIndex2+1].bReachable;
                }
            }

            NetPrintf(("voipconnection: [%d] removed %a from send addr fallbacks list for this voip connection\n", iConnId, uAddr));

            iRetCode = 0;
            break;
        }
    }

    if (iRetCode != 0)
    {
        NetPrintf(("voipconnection: [%d] warning - %a not deleted because not found in send addr fallbacks list for this voip connection\n", iConnId, uAddr));
    }

    #if DIRTYCODE_LOGGING
    // print list of send address fallbacks for this connection
    _VoipConnectionPrintSendAddrFallbacks(pConnectionlist, iConnId);
    #endif

    NetCritLeave(&pConnectionlist->NetCrit);

    return(iRetCode);
}
#endif

