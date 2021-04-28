/*H*************************************************************************************/
/*!
    \File    protoadvt.c

    \Description
        This advertising module provides a relatively simple multi-protocol
        distributed name server architecture utilizing the broadcast capabilities
        of UDP and IPX. Once the module is instantiated, it can be used as both
        an advertiser (server) and watcher (client) simultaneously.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2002.  ALL RIGHTS RESERVED.

    \Version  1.0  02/17/99 (GWS) Original version
    \Version  1.1  02/25/99 (GWS) Alpha release
    \Version  1.2  07/27/99 (GWS) Initial release
    \Version  1.3  10/28/99 (GWS) Message queue elimination
    \Version  1.4  07/10/00 (GWS) Windock2 dependency removal
    \Version  1.5  12/11/00 (GWS) Ported to Dirtysock
    \Version  2.0  03/28/03 (GWS) Made more robust with double-broadcast sends
    \Version  2.1  06/16/03 (JLB) Made string comparisons case-insensitive
*/
/*************************************************************************************H*/

/*** Include files *********************************************************************/

#include <stdio.h>
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/proto/protoadvt.h"

#if defined(DIRTYCODE_PS4)
#include <sys/types.h>
#endif
/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! raw broadcast packet format
typedef struct ProtoAdvtPacketT
{
    //! static packet identifier
    uint8_t aIdent[3];
    //! single char freq (avoid byte ordering issues)
    uint8_t uFreq;
    //! advertisement sequence number
    uint8_t aSeqn[4];
    //! the kind of service
    char strKind[32];
    //! the name of this particular service
    char strName[32];
    //! misc notes about service
    char strNote[192];
    //! list of service addresses
    char strAddr[120];
} ProtoAdvtPacketT;

//! a list of services
typedef struct ServListT
{
    //! actual packet data
    ProtoAdvtPacketT Packet;
    //! timeout until item expires or is rebroadcast
    uint32_t uTimeout;
    //! last internet address associated with service
    char strInetDot[16];
    uint32_t uInetVal;
    uint32_t uHostVal;
    //! flag to indicate if packet has local origin
    int32_t iLocal;
    //! link to next item in list
    struct ServListT *pNext;
} ServListT;

//! local module reference
struct ProtoAdvtRef
{
    //! control access to resources
    NetCritT Crit;
    //! list of services to announce
    ServListT *pAnnounce;
    //! known service list
    ServListT *pSnoopBuf;
    ServListT *pSnoopEnd;
    //! dirtysock memory group
    int32_t iMemGroup;
    void *pMemGroupUserData;
    //! seeding is needed
    int32_t iSeed;
    //! number of users
    int32_t iUsage;
    //! active socket
    SocketT *pSock;
    //! broadcast address
    struct sockaddr Addr;
    //! indicate that snoop buffer changed
    int32_t iSnoop;
};


/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

static int32_t g_count = 0;
static ProtoAdvtRef *g_ref = NULL;

// Public variables


/*** Private Functions *****************************************************************/


/*F*************************************************************************************/
/*!
    \Function _ProtoAdvtSendPacket

    \Description
        Sends a ProtoAdvt packet.

    \Input *pRef    - module ref
    \Input *pPacket - packet to send

    \Output
        int32_t     - return result from SocketSendto()

    \Version 11/30/2006 (jbrookes)
*/
/*************************************************************************************F*/
static int32_t _ProtoAdvtSendPacket(ProtoAdvtRef *pRef, const ProtoAdvtPacketT *pPacket)
{
    int32_t iPacketSize = sizeof(*pPacket);
    return(SocketSendto(pRef->pSock, (const char *)pPacket, iPacketSize, 0, &pRef->Addr, sizeof(pRef->Addr)));
}

/*F*************************************************************************************/
/*!
    \Function _ProtoAdvtRecvPacket

    \Description
        Receives a ProtoAdvt packet.  Variable-length string fields are unpacked from
        the compacted sent packet to produce the fixed-length result packet.

    \Input *pRef    - module ref
    \Input *pPacket - packet buffer to receive to
    \Input *pFrom   - sender's address

    \Output
        int32_t     - return result from SocketRecvfrom()

    \Version 11/30/2006 (jbrookes)
*/
/*************************************************************************************F*/
static int32_t _ProtoAdvtRecvPacket(ProtoAdvtRef *pRef, ProtoAdvtPacketT *pPacket, struct sockaddr *pFrom)
{
    int32_t iFromLen = sizeof(*pFrom);
    return(SocketRecvfrom(pRef->pSock, (char *)pPacket, sizeof(*pPacket), 0, pFrom, &iFromLen));
}

/*F*************************************************************************************/
/*!
    \Function _ProtoAdvtRequestSeed

    \Description
        Send a seed request

    \Input *pRef    - module ref

    \Version 03/28/03 (GWS)
*/
/*************************************************************************************F*/
static void _ProtoAdvtRequestSeed(ProtoAdvtRef *pRef)
{
    ProtoAdvtPacketT Packet;

    // setup a query packet
    ds_memclr(&Packet, sizeof(Packet));
    Packet.aIdent[0] = ADVERT_PACKET_IDENTIFIER[0];
    Packet.aIdent[1] = ADVERT_PACKET_IDENTIFIER[1];
    Packet.aIdent[2] = ADVERT_PACKET_IDENTIFIER[2];
    Packet.strKind[0] = '?';

    // do the send
    _ProtoAdvtSendPacket(pRef, &Packet);
}


/*F*************************************************************************************/
/*!
    \Function _ProtoAdvtCallback

    \Description
        Main worker callback

    \Input *pSock - connection socket pointer
    \Input iFlags - unused
    \Input *_pRef - ProtoAdvtRef pointer

    \Output int32_t - zero

    \Version 11/01/02 (GWS)
*/
/*************************************************************************************F*/
static int32_t _ProtoAdvtCallback(SocketT *pSock, int32_t iFlags, void *_pRef)
{
    int32_t iLen;
    int32_t iLocal;
    struct sockaddr Base;
    struct sockaddr From;
    ServListT *pList, *pItem, **ppLink;
    ProtoAdvtPacketT Packet;
    ProtoAdvtRef *pRef = (ProtoAdvtRef *) _pRef;
    uint32_t uTick;

    // make sure we own resources
    if (!NetCritTry(&pRef->Crit))
    {
        NetPrintf(("protoadvt: could not acquire critical section\n"));
        return(0);
    }

    // process all pending packets
    while (pRef->pSock != NULL)
    {
        // request seeding if needed
        if (pRef->iSeed > 0)
        {
            pRef->iSeed = 0;
            _ProtoAdvtRequestSeed(pRef);
        }

        // try and receive a packet
        Packet.strKind[0] = '\0';
        SockaddrInit(&From, AF_INET);
        iLen = _ProtoAdvtRecvPacket(pRef, &Packet, &From);
        if ((iLen <= 0) || (Packet.strKind[0] == '\0'))
        {
            break;
        }

        // get local address we would have used
        SockaddrInit(&Base, AF_INET);
        SocketHost(&Base, sizeof(Base), &From, sizeof(From));

        // see if packet was sent locally
        iLocal = 0;
        if ((Base.sa_family == From.sa_family) && (From.sa_family == AF_INET))
            iLocal = (memcmp(&From.sa_data, &Base.sa_data, 6) == 0);

        // ignore packets without proper identifier
        if ((Packet.aIdent[0] != ADVERT_PACKET_IDENTIFIER[0]) ||
            (Packet.aIdent[1] != ADVERT_PACKET_IDENTIFIER[1]) ||
            (Packet.aIdent[2] != ADVERT_PACKET_IDENTIFIER[2]))
        {
            continue;
        }

        // handle seed request first
        if ((Packet.strKind[0] == '?') && (Packet.strKind[1] == 0))
        {
            uTick = NetTick() + 1000;
            // reduce timeouts for outgoing packets
            for (pList = pRef->pAnnounce; pList != NULL; pList = pList->pNext)
            {
                // reduce timeout down to 1 second
                // (helps prevent multiple seed requests from causing storm)
                if (pList->uTimeout > uTick)
                    pList->uTimeout = uTick;
            }
            // get next packet
            continue;
        }

        // make sure name & kind are set
        if (Packet.strName[0] == 0)
        {
            continue;
        }

        // process the packet
        pItem = NULL;
        for (pList = pRef->pSnoopBuf; pList != pRef->pSnoopEnd; ++pList)
        {
            // check for unused block
            if ((pItem == NULL) && (pList->Packet.strName[0] == 0))
            {
                pItem = pList;
            }
            // see if we already have this packet
            if ((ds_stricmp(Packet.strKind, pList->Packet.strKind) == 0) &&
                (ds_stricmp(Packet.strName, pList->Packet.strName) == 0) &&
                (memcmp(Packet.aSeqn, pList->Packet.aSeqn, sizeof(Packet.aSeqn)) == 0))
            {
                break;
            }
        }

        // see if this is a new packet
        if ((pList == pRef->pSnoopEnd) && (pItem != NULL))
        {
            pList = pItem;
            ds_memclr(pList, sizeof(*pList));
            // populate with strKind/strName/aSeqn (never change)
            ds_memcpy_s(pList->Packet.aSeqn, sizeof(pList->Packet.aSeqn), Packet.aSeqn, sizeof(Packet.aSeqn));
            ds_strnzcpy(pList->Packet.strKind, Packet.strKind, sizeof(pList->Packet.strKind));
            ds_strnzcpy(pList->Packet.strName, Packet.strName, sizeof(pList->Packet.strName));
            ds_strnzcpy(pList->Packet.strNote, Packet.strNote, sizeof(pList->Packet.strNote));
            // indicate snoop buffer changed
            pRef->iSnoop += 1;
            NetPrintf(("protoadvt: added new advert name=%s freq=%u addr=%a\n", pList->Packet.strName, Packet.uFreq, SockaddrInGetAddr(&From)));
        }

        // update the record (assuming we found/allocated)
        if (pList != pRef->pSnoopEnd)
        {
            ds_strnzcpy(pList->Packet.strAddr, Packet.strAddr, sizeof(pList->Packet.strAddr));
            ds_strnzcpy(pList->Packet.strNote, Packet.strNote, sizeof(pList->Packet.strNote));
            pList->uTimeout = NetTick()+2*(Packet.uFreq*1000)+1000;
            pList->iLocal = iLocal;
            pList->uInetVal = SockaddrInGetAddr(&From);
            pList->uHostVal = SockaddrInGetAddr(&Base);
            SockaddrInGetAddrText(&From, pList->strInetDot, sizeof(pList->strInetDot));
            // indicate snoop buffer changed
            pRef->iSnoop += 1;
        }
        // loop checks for another packet
    }

    // check for expired services
    uTick = NetTick();
    for (pList = pRef->pSnoopBuf; pList != pRef->pSnoopEnd; ++pList)
    {
        if (pList->Packet.strName[0] != 0)
        {
            // see if anything has expired
            if ((pList->Packet.strAddr[0] == 0) || (pList->uTimeout == 0) || (uTick > pList->uTimeout))
            {
                NetPrintf(("protoadvt: expiring advert name=%s\n", pList->Packet.strName));
                pList->Packet.strName[0] = 0;
                pList->Packet.strKind[0] = 0;
                // indicate snoop buffer changed
                pRef->iSnoop += 1;
            }
        }
    }

    // check for expired announcements
    for (ppLink = &pRef->pAnnounce; (*ppLink) != NULL;)
    {
        // see if it has expired
        if ((*ppLink)->uTimeout == 0)
        {
            pItem = *ppLink;
            *ppLink = (*ppLink)->pNext;
            // send out a cancelation notice
            NetPrintf(("protoadvt: canceling announcement name=%s\n", pItem->Packet.strName));
            pItem->Packet.strAddr[0] = '\0';
            // send out notices
            _ProtoAdvtSendPacket(pRef, &pItem->Packet);
            // done with the record
            DirtyMemFree(pItem, PROTOADVT_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        }
        else
        {
            // move to next item
            ppLink = &(*ppLink)->pNext;
        }
    }

    // see if anything needs to be sent
    uTick = NetTick();
    for (pList = pRef->pAnnounce; pList != NULL; pList = pList->pNext)
    {
        // see if its time to send
        if (uTick > pList->uTimeout)
        {
            // broadcast packet
            _ProtoAdvtSendPacket(pRef, &pList->Packet);
            // update send time
            pList->uTimeout = uTick + pList->Packet.uFreq*1000;
        }
    }

    // done with update
    NetCritLeave(&pRef->Crit);
    return(0);
}


/*** Public Functions ******************************************************************/


/*F*************************************************************************************/
/*!
    \Function ProtoAdvtAnnounce

    \Description
        Advertise a service as available

    \Input *pRef  - reference pointer
    \Input *pKind - service class (max 32 characters including NUL)
    \Input *pName - service name (usually game name, max 32 characters including NUL)
    \Input *pNote - service note (max 32 characters including NUL)
    \Input *pAddr - service address list (max 120 characters, including NUL)
    \Input iFreq  - announcement frequency, in seconds (can be in the range [2...250])

    \Output
        int32_t  - <0 = error, 0=ok

    \Version 11/01/02 (GWS)
*/
/*************************************************************************************F*/
int32_t ProtoAdvtAnnounce(ProtoAdvtRef *pRef, const char *pKind, const char *pName,
                      const char *pNote, const char *pAddr, int32_t iFreq)
{
    ServListT *pList;
    ServListT *pItem;

    // clamp the freqeuncy to valid range
    if (iFreq == 0)
        iFreq = 30;
    if (iFreq < 2)
        iFreq = 2;
    if (iFreq > 250)
        iFreq = 250;

    // validate input strings
    if ((pKind == NULL) || (pKind[0] == '\0'))
    {
        NetPrintf(("protoadvt: error, invalid kind\n"));
        return(-1);
    }
    if ((pName == NULL) || (pName[0] == '\0'))
    {
        NetPrintf(("protoadvt: error, invalid name\n"));
        return(-2);
    }
    if (pNote == NULL)
    {
        NetPrintf(("protoadvt: error, invalid note\n"));
        return(-3);
    }
    if (pAddr == NULL)
    {
        NetPrintf(("protoadvt: error, invalid addr\n"));
        return(-4);
    }

    // see if service is already listed
    for (pList = pRef->pAnnounce; pList != NULL; pList = pList->pNext)
    {
        // check for dupl
        if ((ds_stricmp(pKind, pList->Packet.strKind) == 0) &&
            (ds_stricmp(pName, pList->Packet.strName) == 0))
        {
            // update the addr field if necessary
            if (ds_stricmp(pAddr, pList->Packet.strAddr) != 0)
            {
                // update address & force immediate broadcast of new info
                ds_strnzcpy(pList->Packet.strAddr, pAddr, sizeof(pList->Packet.strAddr));
                pList->uTimeout = NetTick()-1;
            }

            // update the note field if necessary
            if (ds_stricmp(pNote, pList->Packet.strNote) != 0)
            {
                // update note & force immediate broadcast of new info
                ds_strnzcpy(pList->Packet.strNote, pNote, sizeof(pList->Packet.strNote));
                pList->uTimeout = NetTick()-1;
            }
            // all done
            return(0);
        }
    }

    // build a packet
    pItem = DirtyMemAlloc(sizeof(*pItem), PROTOADVT_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
    ds_memclr(pItem, sizeof(*pItem));
    pItem->uTimeout = NetTick();
    pItem->Packet.aIdent[0] = ADVERT_PACKET_IDENTIFIER[0];
    pItem->Packet.aIdent[1] = ADVERT_PACKET_IDENTIFIER[1];
    pItem->Packet.aIdent[2] = ADVERT_PACKET_IDENTIFIER[2];
    pItem->Packet.uFreq = (unsigned char) iFreq;
    pItem->Packet.aSeqn[0] = (unsigned char)(pItem->uTimeout >> 24);
    pItem->Packet.aSeqn[1] = (unsigned char)(pItem->uTimeout >> 16);
    pItem->Packet.aSeqn[2] = (unsigned char)(pItem->uTimeout >> 8);
    pItem->Packet.aSeqn[3] = (unsigned char)(pItem->uTimeout >> 0);
    ds_strnzcpy(pItem->Packet.strKind, pKind, sizeof(pItem->Packet.strKind));
    ds_strnzcpy(pItem->Packet.strName, pName, sizeof(pItem->Packet.strName));
    ds_strnzcpy(pItem->Packet.strAddr, pAddr, sizeof(pItem->Packet.strAddr));
    ds_strnzcpy(pItem->Packet.strNote, pNote, sizeof(pItem->Packet.strNote));
    NetPrintf(("protoadvt: broadcasting new announcement name=%s freq=%u\n", pItem->Packet.strName, pItem->Packet.uFreq));

    // send an immediate copy
    _ProtoAdvtSendPacket(pRef, &pItem->Packet);
    // schedule next send in 250ms
    pItem->uTimeout = NetTick()+250;

    // make sure we own resources
    NetCritEnter(&pRef->Crit);
    // add new item to list
    pItem->pNext = pRef->pAnnounce;
    pRef->pAnnounce = pItem;
    // release resources
    NetCritLeave(&pRef->Crit);
    return(0);
}

/*F*************************************************************************************/
/*!
    \Function ProtoAdvtCancel

    \Description
        Cancel server advertisement

    \Input *pRef  - reference pointer
    \Input *pKind - service kind
    \Input *pName - service name (usually game name, NULL=wildcard)

    \Output
        int32_t  - <0=error, 0=ok

    \Version 11/01/02 (GWS)
*/
/*************************************************************************************F*/
int32_t ProtoAdvtCancel(ProtoAdvtRef *pRef, const char *pKind, const char *pName)
{
    ServListT *pList;

    // make sure we own resources
    NetCritEnter(&pRef->Crit);

    // locate the item in the announcement list
    for (pList = pRef->pAnnounce; pList != NULL; pList = pList->pNext)
    {
        // see if we got a match
        if (((pKind == NULL) || (ds_stricmp(pKind, pList->Packet.strKind) == 0)) &&
            ((pName == NULL) || (ds_stricmp(pName, pList->Packet.strName) == 0)))
        {
            // mark as deleted
            pList->uTimeout = 0;
            break;
        }
    }

    // release resources
    NetCritLeave(&pRef->Crit);

    // was item found?
    return(pList == NULL ? -1 : 0);
}


/*F*************************************************************************************/
/*!
    \Function ProtoAdvtQuery

    \Description
        Query for available services

    \Input *pRef    - reference pointer
    \Input *pKind   - service kind
    \Input *pProto  - protocol
    \Input *pBuffer - target buffer
    \Input iBufLen  - target length
    \Input iLocal   - if zero, exclude local lists

    \Output
        int32_t     - number of matches

    \Version 11/01/02 (GWS)
*/
/*************************************************************************************F*/
int32_t ProtoAdvtQuery(ProtoAdvtRef *pRef, const char *pKind, const char *pProto,
                   char *pBuffer, int32_t iBufLen, int32_t iLocal)
{
    char *pS, *pT, *pU;
    char strAddr[256] = "";
    char strRecord[512];
    int32_t iCount = 0;
    ServListT *pList;

    // establish min buffer size
    if (iBufLen < 5)
        return(-1);

    // default to empty buffer
    pBuffer[0] = 0;

    // see what matching services we know about
    for (pList = pRef->pSnoopBuf; pList != pRef->pSnoopEnd; ++pList)
    {
        // skip empty entries
        if (pList->Packet.strName[0] == 0)
            continue;
        // see if the kind matches
        if (ds_stricmp(pKind, pList->Packet.strKind) != 0)
            continue;
        // exclude locals if not wanted
        if ((!iLocal) && (pList->iLocal))
            continue;
        // parse the address choices
        for (pS = pList->Packet.strAddr; *pS != 0;)
        {
            // parse out the address
            for (pT = strAddr; (*pS != 0) && (*pS != '\t');)
                *pT++ = *pS++;
            *pT++ = 0;
            if (*pS == '\t')
                ++pS;
            // make sure address looks valid
            if ((strlen(strAddr) < 5) || (strAddr[3] != ':'))
                continue;
            // see if this protocol type is wanted by caller
            strAddr[3] = 0;
            if ((pProto[0] != 0) && (strstr(pProto, strAddr) == NULL))
                continue;
            strAddr[3] = ':';
            // compile data into a record
            ds_strnzcpy(strRecord, pList->Packet.strName, sizeof(strRecord));
            for (pT = strRecord; *pT != 0; ++pT)
                ;
            *pT++ = '\t';
            // copy over notes field
            strcpy(pT, pList->Packet.strNote);
            while (*pT != 0)
                ++pT;
            *pT++ = '\t';
            // append the address
            for (pU = strAddr; *pU != 0; ++pU)
            {
                // check for inet substitution
                if ((pU[0] == '~') && (pU[1] == '1'))
                {
                    // make sure inet address is known
                    if (pList->strInetDot[0] == 0)
                        break;
                    // copy over the address
                    strcpy(pT, pList->strInetDot);
                    while (*pT != 0)
                        ++pT;
                    ++pU;
                    continue;
                }
                // check for ipx substitution
                if ((pU[0] == '~') && (pU[1] == '2'))
                {
                    // no ipx support
                    break;
                }
                // raw copy
                *pT++ = *pU;
            }
            // see if copy was canceled (substitution error)
            if (*pU != 0)
                continue;
            // terminate the record
            *pT++ = '\n';
            *pT = 0;
            // see if the record will fit into output buffer
            if (strlen(strRecord)+5 > (unsigned) iBufLen)
            {
                // indicate an overflow
                *pBuffer++ = '.';
                *pBuffer++ = '.';
                *pBuffer++ = '.';
                *pBuffer++ = '\n';
                *pBuffer = 0;
                return(iCount);
            }
            // append the record to the buffer
            strcpy(pBuffer, strRecord);
            pBuffer += (int32_t)strlen(strRecord);
            iBufLen -= (int32_t)strlen(strRecord);
            ++iCount;
        }
    }

    // return count of items
    return(iCount);
}


/*F*************************************************************************************/
/*!
    \Function ProtoAdvtLocate

    \Description
        Locate a specific advertisement and return advertisers address (UDP only)

    \Input *pRef   - reference pointer
    \Input *pKind  - service kind (NULL=any)
    \Input *pName  - service name (usually game name, NULL=wildcard)
    \Input *pHost  - pointer to buffer for host (our) address, or NULL
    \Input uDefVal - value returned if module is not set

    \Output
        uint32_t  - uDefVal if ref is NULL, else first matching internet address

    \Version 11/01/02 (GWS)
*/
/*************************************************************************************F*/
uint32_t ProtoAdvtLocate(ProtoAdvtRef *pRef, const char *pKind, const char *pName,
                             uint32_t *pHost, uint32_t uDefVal)
{
    ServListT *pList;
    
    // just return default if module is not set
    if (pRef == NULL)
        return(uDefVal);

    // see what matching services we know about
    for (pList = pRef->pSnoopBuf; pList != pRef->pSnoopEnd; ++pList)
    {
        // make sure service is valid
        if (pList->Packet.strName[0] == 0)
            continue;
        // exclude locals
        if (pList->iLocal)
            continue;
        // see if the kind matches
        if ((pKind != NULL) && (pKind[0] != 0) && (ds_stricmp(pKind, pList->Packet.strKind) != 0))
            continue;
        // make sure the name matches
        if ((pName != NULL) && (pName[0] != 0) && (ds_stricmp(pList->Packet.strName, pName) != 0))
            continue;
        // make sure there is an internet address
        if (pList->uInetVal == 0)
            continue;
        // return host (our) address if they want it
        if (pHost != NULL)
            *pHost = pList->uHostVal;
        // return the internet address
        uDefVal = pList->uInetVal;
        break;
    }

    // return default value
    return(uDefVal);
}


/*F*************************************************************************************/
/*!
    \Function ProtoAdvtConstruct

    \Description
        Construct an advertising agent

    \Input limit            - size of snoop buffer (min 4)

    \Output
        ProtoAdvtRef *      - construct ref (passed to other routines)

    \Version 11/01/02 (GWS)
*/
/*************************************************************************************F*/
ProtoAdvtRef *ProtoAdvtConstruct(int32_t limit)
{
    SocketT *sock;
    struct sockaddr bindaddr;
    struct sockaddr peeraddr;
    ProtoAdvtRef *ref;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // see if already allocated
    if (g_ref != NULL)
    {
        ++g_count;
        return(g_ref);
    }

    // allocate class storage
    ref = DirtyMemAlloc(sizeof(*ref), PROTOADVT_MEMID, iMemGroup, pMemGroupUserData);
    if (ref == NULL)
        return(NULL);
    ds_memclr(ref, sizeof(*ref));
    ref->iMemGroup = iMemGroup;
    ref->pMemGroupUserData = pMemGroupUserData;

    // allocate snooped advertisement buffer
    if (limit < 4)
        limit = 4;
    ref->pSnoopBuf = DirtyMemAlloc(limit*sizeof(ref->pSnoopBuf[0]), PROTOADVT_MEMID, ref->iMemGroup, ref->pMemGroupUserData);
    if (ref->pSnoopBuf == NULL)
    {
        DirtyMemFree(ref, PROTOADVT_MEMID, ref->iMemGroup, ref->pMemGroupUserData);
        return(NULL);
    }
    ref->pSnoopEnd = ref->pSnoopBuf+limit;
    ds_memclr(ref->pSnoopBuf, (int32_t)((char *)ref->pSnoopEnd-(char *)ref->pSnoopBuf));

    // set initial revision value
    ref->iSnoop = 1;

    // create the actual socket
    sock = SocketOpen(AF_INET, SOCK_DGRAM, 0);
    if (sock == NULL)
    {
        DirtyMemFree(ref, PROTOADVT_MEMID, ref->iMemGroup, ref->pMemGroupUserData);
        return(NULL);
    }

    // mark as available for sharing
    g_ref = ref;
    g_count = 1;

    // need to sync access to data
    NetCritInit(&ref->Crit, "protoadvt");
    // limit access during setup
    NetCritEnter(&ref->Crit);

    // bind with local address
    SockaddrInit(&bindaddr, AF_INET);
    SockaddrInSetPort(&bindaddr, ADVERT_BROADCAST_PORT_UDP);
    SocketBind(sock, &bindaddr, sizeof(bindaddr));

    // connect to remote address
    SockaddrInit(&peeraddr, AF_INET);
    SockaddrInSetPort(&peeraddr, ADVERT_BROADCAST_PORT_UDP);
    SockaddrInSetAddr(&peeraddr, INADDR_BROADCAST);

    // note: though it would be easier to use connect to bind the peer address
    // to the socket, it will not work because winsock will not deliver an
    // incoming packet to a socket connected to the sending port. therefore,
    // the address must be maintained separately and used with sendto.
    // (logic unchanged for dirtysock since it is already coded this way)
    ds_memcpy_s(&ref->Addr, sizeof(ref->Addr), &peeraddr, sizeof(peeraddr));

    // broadcast request for available server info (seed database)
    ref->iSeed = 1;
    // make the socket available
    ref->pSock = sock;
    // bind the callback function
    SocketCallback(ref->pSock, CALLB_RECV, 100, ref, &_ProtoAdvtCallback);

    // done with setup
    NetCritLeave(&ref->Crit);

    // immediately send a seed request
    // (the idle process will send a second within 100ms)
    _ProtoAdvtRequestSeed(ref);
    return(ref);
}


/*F*************************************************************************************/
/*!
    \Function ProtoAdvtDestroy

    \Description
        Destruct an advertising agent

    \Input *ref - construct ref

    \Version 11/01/02 (GWS)
*/
/*************************************************************************************F*/
void ProtoAdvtDestroy(ProtoAdvtRef *ref)
{
    SocketT *sock;

    // make sure what is valid
    if (ref == NULL)
    {
        return;
    }

    // see if we are last
    if (g_count > 1)
    {
        --g_count;
        return;
    }

    // destroy the class
    g_ref = NULL;
    g_count = 0;

    // cancel all announcements
    while (ref->pAnnounce != NULL)
    {
        ProtoAdvtPacketT *packet = &ref->pAnnounce->Packet;
        ProtoAdvtCancel(ref, packet->strKind, packet->strName);
        _ProtoAdvtCallback(ref->pSock, 0, ref);
    }

    // make sure we own resources
    NetCritEnter(&ref->Crit);

    // do the shutdown
    sock = ref->pSock;
    ref->pSock = NULL;

    // release resources
    NetCritLeave(&ref->Crit);

    // dispose of socket
    SocketClose(sock);

    // done with semaphore
    NetCritKill(&ref->Crit);

    // done with ref
    DirtyMemFree(ref->pSnoopBuf, PROTOADVT_MEMID, ref->iMemGroup, ref->pMemGroupUserData);
    DirtyMemFree(ref, PROTOADVT_MEMID, ref->iMemGroup, ref->pMemGroupUserData);
}

