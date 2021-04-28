/*H*******************************************************************/
/*!
    \File protoudp.c

    \Description
        This module provides a generic UDP interface.

    \Notes
        This was initially written to support the TickerApi.  That has
        modest bandwidth requirements, only one packet every few seconds,
        and can accept moderate packet loss.   As such this API is only
        a relatively thin layer over a plain UDP socket with some buffering
        on packet reception. Historically so that on the PS2 the IOP can 
        read multiple packets without the EE having to be interrupted for
        each packet.

        The buffering is in the form of a queue of packets where the
        maximum size of the queue and the maximum size of each packet in 
        the queue are specified by the client.

        There are various ways to implement a queue of packets, but to keep
        the memory allocation and error handling as simple as possible a block
        of memory is allocated at startup that is large enough to hold the
        maximum number of packets.

        All the packets are continguous in memory with each packet having
        the form :-

             0     1     2     3            len+4
          +-----+-----+-----+------+.......+------+.....+------+
          |     |     |     |      |       |      |     |      |
          +-----+-----+-----+------+.......+------+.....+------+
          |          len           |<--- len ---->|            |
          |                        |<------ max packet size -->|

        That is all packets are the same length (whatever size the user
        specifies at startup) and the 4 bytes at the start of the packet
        indicate the actual number of bytes in the packet (4 bytes is
        rather generous, 2 is probably sufficient).

        The packets are treated as a queue with the head were the client
        reads a packet from and the tail where new packets are added.
        To reduce the number of (hidden) multiplications during indexing the
        head and tail values are the byte offset of the start of the packet.
        So for example, if the user requested a maximum packet size of
        40 bytes and a 5 element queue then 220 bytes would be allocated
        to hold the packets, that's 5 * (40 + 4) where the +4 is to cover
        the 4 bytes at the start of each packet to hold the length.
        The byte offsets of the 5 packets would be: 0, 44, 88, 132 and 176
        therefore these are the 5 possible values for the head and tail
        offsets in the queue.

    \Copyright
        Copyright (c) Electronic Arts 2003. ALL RIGHTS RESERVED.

    \Version 1.0 12/11/2003 (sbevan) First Version
*/
/*******************************************************************H*/

/*** Include files ***************************************************/

#include <string.h>

#include "dirtysock.h"
#include "dirtymem.h"
#include "protoudp.h"

/*** Defines *********************************************************/

//! size of data that prefixes packet data in packet buffer
#define PROTOUDP_PKTPREFIXSIZE      (sizeof(int32_t)+sizeof(struct sockaddr_in))

/*** Type Definitions ************************************************/

struct ProtoUdpT
{
    SocketT  *pSocket;
    NetCritT crit;

    // module memory group
    int32_t iMemGroup;          //!< module mem group id
    void *pMemGroupUserData;    //!< user data associated with mem group

    char     *pPackets;         //!< all the packets
    uint32_t uMaxPacketSize;    //!< max size in # bytes of a packet
    uint32_t uTotalPacketSpace; //!< total # bytes occupied by max packets
    uint32_t uPacketsHead;      //!< byte offset of head of packet queue
    uint32_t uPacketsTail;      //!< byte offset of tail of packet queue
    struct sockaddr_in RemoteAddr;
};


/*** Private Functions ***********************************************/


/*F*******************************************************************/
/*!
    \Function _ProtoUdpGetNextPacket

    \Description
        Calculate the byte offset of the next packet.

    \Input *pUdp   - the UDP object
    \Input uPacket - the packet to find the next packet after.

    \Output packet byte offset.

    \Version 1.0 12/11/2003 (sbevan) First Version
*/
/*******************************************************************F*/
static uint32_t _ProtoUdpGetNextPacket(ProtoUdpT *pUdp, uint32_t uPacket)
{
    uint32_t delta = pUdp->uMaxPacketSize+PROTOUDP_PKTPREFIXSIZE;
    return((uPacket+delta)%pUdp->uTotalPacketSpace);
}


/*** Public functions ************************************************/


/*F*******************************************************************/
/*!
    \Function ProtoUdpCreate

    \Description
        Create a proto udp instance.

    \Input iMaxPacketSize - max size in bytes of any packet that will received.
    \Input iMaxPackets    - max number of packets that should be buffered.

    \Output 0 if the UDP object cannot be created.

    \Notes
        To avoid creating lots of separate packet buffers, one large object
        is requested that can hold the sum of the size of the ProtoUdpT object
        and all the packets.  For reasons to do with alignment and how the
        packet buffers are defined iMaxPacketSize is rounded up to the nearest
        multiple of 4 so this should be taken into account in any space
        calculations.  More details on the packet format can be found in the
        file description.  

    \Version 1.0 12/11/2003 (sbevan) First Version
*/
/*******************************************************************F*/
ProtoUdpT *ProtoUdpCreate(int32_t iMaxPacketSize, int32_t iMaxPackets)
{
    ProtoUdpT *pUdp;
    int32_t uRoundedMaxPacketSize = ((iMaxPacketSize+3)>>2)<<2;
    int32_t uTotalPacketSpace = (uRoundedMaxPacketSize+PROTOUDP_PKTPREFIXSIZE)*iMaxPackets;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pUdp = DirtyMemAlloc(sizeof *pUdp + uTotalPacketSpace, PROTOUDP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protoudp: could not allocate module state\n"));
        return(NULL);
    }
    memset(pUdp, 0, sizeof(*pUdp));
    pUdp->iMemGroup = iMemGroup;
    pUdp->pMemGroupUserData = pMemGroupUserData;

    // init rest of state
    pUdp->pSocket = NULL;
    NetCritInit(&pUdp->crit, "protoudp");
    pUdp->pPackets = (char *)(pUdp+1);
    pUdp->uMaxPacketSize = iMaxPacketSize;
    pUdp->uTotalPacketSpace = uTotalPacketSpace;
    pUdp->uPacketsHead = 0;
    pUdp->uPacketsTail = _ProtoUdpGetNextPacket(pUdp, 0);
    memset(&pUdp->RemoteAddr, 0, sizeof pUdp->RemoteAddr);

    // return ref to caller
    return(pUdp);
}

/*F*******************************************************************/
/*!
    \Function ProtoUdpBind

    \Description
        Bind the source port for all outbound packets.

    \Input *pUdp    - the UDP object
    \Input iPort    - port

    \Output 0 if the connection is successful.

    \Notes
        Should not be mixed with ProtoUdpConnect()

    \Version 1.0 12/11/2003 (sbevan) First Version
*/
/*******************************************************************F*/
int32_t ProtoUdpBind(ProtoUdpT *pUdp, int32_t iPort)
{
    struct sockaddr LocalAddr;
    int32_t iResult;

    ProtoUdpDisconnect(pUdp);

    pUdp->pSocket = SocketOpen(AF_INET, SOCK_DGRAM, 0);
    if (pUdp->pSocket == NULL)
    {
        return(-1);
    }

    SockaddrInit(&LocalAddr, AF_INET);
    SockaddrInSetPort(&LocalAddr, iPort);
    
    iResult = SocketBind(pUdp->pSocket, &LocalAddr, sizeof(LocalAddr));
    return(iResult);
}

/*F*******************************************************************/
/*!
    \Function ProtoUdpConnect

    \Description
        Bind the remote address of the socket.

    \Input  *pUdp        - the UDP object
    \Input  *pRemoteAddr - the IPv4 address of the remote peer.

    \Output 0 if the address can be bound, non-zero if there is an error.

    \Version 1.0 12/11/2003 (sbevan) First Version
*/
/*******************************************************************F*/
int32_t ProtoUdpConnect(ProtoUdpT *pUdp, struct sockaddr *pRemoteAddr)
{
    struct sockaddr SockAddr;
    int32_t iResult;

    ProtoUdpDisconnect(pUdp);

    pUdp->pSocket = SocketOpen(AF_INET, SOCK_DGRAM, 0);
    if (pUdp->pSocket == NULL)
    {
        return(-1);
    }
    memcpy(&pUdp->RemoteAddr, pRemoteAddr, sizeof(*pRemoteAddr));

    SockaddrInit(&SockAddr, AF_INET);
    iResult = SocketBind(pUdp->pSocket, &SockAddr, sizeof(SockAddr));
    return(iResult);
}

/*F*******************************************************************/
/*!
    \Function ProtoUdpDestroy

    \Description
        Destroy the UDP instance.

    \Input  *pUdp - the UDP object

    \Output None

    \Version 1.0 12/11/2003 (sbevan) First Version
*/
/*******************************************************************F*/
void ProtoUdpDestroy(ProtoUdpT *pUdp)
{
    ProtoUdpDisconnect(pUdp);
    NetCritKill(&pUdp->crit);
    DirtyMemFree(pUdp, PROTOUDP_MEMID, pUdp->iMemGroup, pUdp->pMemGroupUserData);
}

/*F*******************************************************************/
/*!
    \Function ProtoUdpDisconnect

    \Description
        Undo the effect of any previous ProtoUdpConnect.

    \Input  *pUdp - the UDP object

    \Output None

    \Notes
        This is here in the unlikely event that you call ProtoUdpConnect
        send some packets and then later decide to re-use the same
        ProtoUdpT for traffic destined for multiple targets and so
        you do not want the remote address bound or the implicit address
        checking that occurs on inbound packets when ProtoUdpConnect has
        been called.

    \Version 1.0 12/11/2003 (sbevan) First Version
*/
/*******************************************************************F*/
void ProtoUdpDisconnect(ProtoUdpT *pUdp)
{
    if (pUdp->pSocket != NULL)
    {
        SocketClose(pUdp->pSocket);
        pUdp->pSocket = NULL;
    }
    memset(&pUdp->RemoteAddr, 0, sizeof pUdp->RemoteAddr);
}

/*F*******************************************************************/
/*!
    \Function ProtoUdpGetLocalAddr

    \Description
        Return the local IPv4 address and port that is on all outbound
        packets.  Only works if ProtoUdpConnect has been called to bind
        the remote address.

    \Input  *pUdp       - the UDP object
    \Input  *pLocalAddr - the local IPv4 address that the socket is bound to.

    \Output None

    \Notes
       Logically the SocketInfo 'bind' call is all that should be
       required and that does work on Linux and Windows.  However, on PS2
       the SocketInfo call always returns 0 as the address for a UDP socket
       (works correctly for TCP).  So, as a backup, SocketHost is called
       if the returned address is 0 and on PS2 that does determine the
       correct address.  In the process it gets the port wrong which the
       SocketInfo got right so the port is saved and then restored around
       the SocketHost call.  Perhaps should fix up PS2 SocketHost?

    \Version 1.0 12/11/2003 (sbevan) First Version
*/
/*******************************************************************F*/
void ProtoUdpGetLocalAddr(ProtoUdpT *pUdp, struct sockaddr_in *pLocalAddr)
{
    if (pUdp->pSocket == NULL)
    {
        memset(pLocalAddr, 0, sizeof *pLocalAddr);
        return;
    }
    SocketInfo(pUdp->pSocket, 'bind', 0, pLocalAddr, sizeof *pLocalAddr);
    if (pLocalAddr->sin_addr.s_addr == 0)
    {
        uint16_t port = pLocalAddr->sin_port;
        SocketHost((struct sockaddr *)pLocalAddr, sizeof *pLocalAddr,
                   (const struct sockaddr *)&pUdp->RemoteAddr,
                   sizeof pUdp->RemoteAddr);
        pLocalAddr->sin_port = port;
    }
}

/*F*******************************************************************/
/*!
    \Function ProtoUdpRecvFrom

    \Description
        Checks if there is a udp packet available and if so returns it.

    \Input *pUdp        - the UDP object
    \Input *pBuffer     - buffer where the received message should be written
    \Input uBufferLen   - the length of the buffer
    \Input pRemoteAddr  - remote address

    \Output negative on error, zero if nothing read, otherwise the length of the data read.

    \Version 1.0 12/11/2003 (sbevan) First Version
*/
/*******************************************************************F*/
int32_t ProtoUdpRecvFrom(ProtoUdpT *pUdp, char *pBuffer, uint32_t uBufferLen, struct sockaddr *pRemoteAddr)
{
    uint32_t uPacket;
    uint32_t *pPacket;
    uint32_t uPacketLen = 0;

    if (pUdp->pSocket == NULL)
    {
        return(-1);
    }
    NetCritEnter(&pUdp->crit);
    uPacket = _ProtoUdpGetNextPacket(pUdp, pUdp->uPacketsHead);
    if (uPacket != pUdp->uPacketsTail)
    {
        pPacket = (uint32_t*)&pUdp->pPackets[uPacket];
        uPacketLen = *pPacket;
        if (uBufferLen < uPacketLen)
        {
            uPacketLen = (unsigned)-1;
        }
        else
        {
            if (pRemoteAddr != NULL)
            {
                memcpy(pRemoteAddr, pPacket+1, sizeof(*pRemoteAddr));
            }
            pPacket = (uint32_t *)((intptr_t)pPacket + PROTOUDP_PKTPREFIXSIZE);
            memcpy(pBuffer, pPacket, uPacketLen);
            pUdp->uPacketsHead = uPacket;
        }
    }
    NetCritLeave(&pUdp->crit);
    return(uPacketLen);
}

/*F*******************************************************************/
/*!
    \Function ProtoUdpSend

    \Description
        Send data to the host that was connected to via ProtoUdpConnect.

    \Input *pUdp  - the UDP object
    \Input *pData - the data to send.
    \Input iSize  - the length of the data.

    \Output negative if an error occurred otherwise number of bytes sent.

    \Version 1.0 12/11/2003 (sbevan) First Version
*/
/*******************************************************************F*/
int32_t ProtoUdpSend(ProtoUdpT *pUdp, const char *pData, int32_t iSize)
{
    if (pUdp->pSocket == NULL)
    {
        return(-1);
    }
    return(SocketSendto(pUdp->pSocket, pData, iSize, 0, (struct sockaddr *)&pUdp->RemoteAddr, sizeof(pUdp->RemoteAddr)));
}

/*F*******************************************************************/
/*!
    \Function ProtoUdpSendTo

    \Description
        Send data to the specified address.

    \Input *pUdp       - the UDP object
    \Input *pData      - the data to send.
    \Input iSize       - the length of the data.
    \Input pRemoteAddr - the (IPv4) address to send the data to.

    \Output negative if an error occurred otherwise number of bytes sent.

    \Version 1.0 12/11/2003 (sbevan) First Version
*/
/*******************************************************************F*/
int32_t ProtoUdpSendTo(ProtoUdpT *pUdp, const char *pData, int32_t iSize, struct sockaddr *pRemoteAddr)
{
    if (pUdp->pSocket == NULL)
    {
        return(-1);
    }
    return(SocketSendto(pUdp->pSocket, pData, iSize, 0, pRemoteAddr, sizeof *pRemoteAddr));
}

/*F*******************************************************************/
/*!
    \Function ProtoUdpUpdate

    \Description
        Do any necessary internal processing.

    \Input *pUdp - the UDP object

    \Output None

    \Notes
        This takes care of reading a packet from the network and storing
        it in the packet queue.  If ProtoUdpConnect was called to bind the
        remote address then address filtering is done and the packet is
        thrown away if it does not have the correct source address and port.
        This is a pretty weak sort of filtering and could arguably be left out.

    \Version 1.0 12/11/2003 (sbevan) First Version
*/
/*******************************************************************F*/
void ProtoUdpUpdate(ProtoUdpT *pUdp)
{
    struct sockaddr_in RemoteAddr, *pRemoteAddr;
    int32_t iAddrLen;
    uint32_t *pPacketLen;
    char *pPacketData;
    int32_t iResult;

    if (pUdp->pSocket == NULL)
    {
        return;
    }
    if (!NetCritTry(&pUdp->crit))
    {
        return;
    }
    while (pUdp->uPacketsHead != pUdp->uPacketsTail)
    {
        pPacketLen = (uint32_t *)&pUdp->pPackets[pUdp->uPacketsTail];
        pRemoteAddr = (struct sockaddr_in *)(pPacketLen+1);
        pPacketData = (char *)(pRemoteAddr+1);
        iAddrLen = sizeof(*pRemoteAddr);
        iResult = SocketRecvfrom(pUdp->pSocket, pPacketData,
                                 pUdp->uMaxPacketSize, 0,
                                 (struct sockaddr *)&RemoteAddr, &iAddrLen);
        if (iResult <= 0)
        {
            break;
        }
        #if (DIRTYCODE_PLATFORM != DIRTYCODE_XENON)
        // Xbox traffic is encapsulated in IPsec and so cannot be altered
        // therefore the very primitive address checking is not required.
        // Also when in secure mode the Xbox does funky things with the
        // IP addresses and so the check, as written, doesn't work anyway.
        if ((RemoteAddr.sin_addr.s_addr != pUdp->RemoteAddr.sin_addr.s_addr)
            & (pUdp->RemoteAddr.sin_addr.s_addr != 0))
        {
            continue;
        }
        if ((RemoteAddr.sin_port != pUdp->RemoteAddr.sin_port)
            & (pUdp->RemoteAddr.sin_port != 0))
        {
            continue;
        }
        #endif
        *pPacketLen = iResult;
        memcpy(pRemoteAddr, &RemoteAddr, sizeof(*pRemoteAddr));
        pUdp->uPacketsTail = _ProtoUdpGetNextPacket(pUdp, pUdp->uPacketsTail);
    }
    NetCritLeave(&pUdp->crit);
}
