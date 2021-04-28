/*H********************************************************************************/
/*!
    \File    protoping.c

    \Description
        This module implements a Dirtysock ping routine. It can both generate
        ping requests and record their responses. Because ICMP access (ping)
        is o/s specific, different code is used for each environment. This code
        was derived from the Winsock version.

    \Copyright
        Copyright (c) Electronic Arts 2002-2005.

    \Version 04/03/2002 (gschaefer) First Version
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/proto/protoping.h"
#include "DirtySDK/misc/qosapi.h"

/*** Defines **********************************************************************/

#define PROTOPING_VERBOSE           (DIRTYCODE_DEBUG && FALSE)

#define IDENT_TOKEN                 ('gS')      //!< some unique identifier (client ping)

#define PROTOPING_FLAG_RECEIVED     (1)         //!< indicates a response to request has been received
#define PROTOPING_FLAG_SERVER       (2)         //!< indicates request was a server ping

#define PROTOPING_DEFAULT_TTL       (64)        //!< default ttl is 64 hops
#define PROTOPING_DEFAULT_TIMEOUT   (5 * 1000)  //!< default timeout is five seconds
#define PROTOPING_DEFAULT_MAXPINGS  (32)        //!< maximum number of pings that can be queued

#define PROTOPING_DEFAULT_PORT      (31338)     //!< default port when not using ICMP

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! standard ipv4 packet header (see RFC791)
typedef struct HeaderIpv4
{
    unsigned char verslen;      //!< version and length fields (4 bits each)
    unsigned char service;      //!< type of service field
    unsigned char length[2];    //!< total packet length (header+data)
    unsigned char ident[2];     //!< packet sequence number
    unsigned char frag[2];      //!< fragmentation information
    unsigned char time;         //!< time to live (remaining hop count)
    unsigned char proto;        //!< transport protocol number
    unsigned char check[2];     //!< header checksum
    unsigned char srcaddr[4];   //!< source ip address
    unsigned char dstaddr[4];   //!< dest ip address
} HeaderIpv4;

//! standard icmp packet header
typedef struct HeaderIcmp
{
    unsigned char type;         //!< packet type
    unsigned char code;         //!< sub-type
    unsigned char check[2];     //!< checksum
    unsigned char idn[2];       //!< identifier
    unsigned char seq[2];       //!< sequence number
} HeaderIcmp;

//! standard udp packet header
typedef struct HeaderUdp
{
    unsigned char srcport[2];   //!< src port
    unsigned char dstport[2];   //!< dst port
    unsigned char length[2];    //!< packet length
    unsigned char check[2];     //!< checksum
} HeaderUdp;

//! standard ipv4+icmp packet header
typedef struct EchoHeaderT
{
    HeaderIpv4 ipv4;            //!< ip header
    union
    {
        HeaderIcmp icmp;            //!< icmp header
        HeaderUdp  udp;             //!< udp header
    } proto;
} EchoHeaderT;

//! user data sent in ping
typedef struct EchoDataT
{
    char userdata[PROTOPING_MAXUSERDATA];    //!< user data
} EchoDataT;

//! complete ping request packet
typedef struct EchoRequestT
{
    EchoHeaderT header;         //!< generic ICMP response header
    EchoDataT data;
} EchoRequestT;

//! complete ping response packet
typedef struct EchoResponseT
{
    EchoHeaderT header;         //!< generic ICMP response header
    union
    {
        EchoDataT data;
        EchoHeaderT header_org;
    } body;
} EchoResponseT;

//! packet queue entry
typedef struct ProtoPingQueueT
{
    uint32_t uAddr;     //!< address pinged/responding address
    uint32_t uTime;     //!< timeout, in milliseconds
    uint32_t uTick;     //!< time received
    void    *pData;     //!< pointer to user data, or NULL if none
#if DIRTYCODE_LOGGING
    const char *pType;  //!< type name
#endif
    uint16_t uSize;     //!< size of user data, or zero if none
    uint16_t uSeqn;     //!< ping sequence number
    uint16_t uPing;     //!< ping time, or zero if timeout
    uint8_t  uType;     //!< ICMP type of response
    uint8_t  uFlag;     //!< flags
} ProtoPingQueueT;

//! module state
struct ProtoPingRefT
{
    SocketT *pSocket;           //!< socket pointer
    NetCritT ThreadCrit;        //!< critical section
    
    // module memory group
    int32_t iMemGroup;          //!< module mem group id
    void *pMemGroupUserData;    //!< user data associated with mem group
    
    ePingType eType;            //!< current ping type
    QosApiRefT *pQosApi;        //!< qosapi ref

    uint32_t uLocalAddr;        //!< local address
    int32_t iSeqn;              //!< current sequence number
    int32_t iRefCount;          //!< instance count
    int32_t iQueueSize;         //!< size of request queue
    int32_t iQueueTail;         //!< end of queue   
    int16_t iIdentToken;        //!< ident token (defaults to IDENT_TOKEN)
    uint16_t uPingPort;         //!< ping port to use if not using ICMP
    uint8_t bVerbose;           //!< verbose or not?
    uint8_t uProtocol;          //!< protocol to use (default ICMP)
    uint8_t _pad[2];
    ProtoPingQueueT RequestQueue[1];  //!< variably-sized request queue (must come last)
};

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

// Private variables

#if DIRTYCODE_LOGGING
static const char *_ProtoPing_strResponseTypes[] =
{
    "ICMP_ECHOREPLY",
    "",
    "",
    "ICMP_DESTUNREACH",
    "ICMP_SOURCEQUENCH",
    "ICMP_REDIRECT",
    "",
    "",
    "ICMP_ECHO",
    "",
    "",
    "ICMP_TIMEEXCEEDED",
    "ICMP_PARAMPROBLEM",
    "ICMP_TIMESTAMP",
    "ICMP_TIMESTAMPREPLY",
    "ICMP_INFOREQUEST",
    "ICMP_INFOREPLY",
};
#endif

static ProtoPingRefT *_ProtoPing_pRef = NULL;

// Public variables


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _ParseResponse

    \Description
        Parse an incoming ICMP packet.

    \Input *pProtoPing  - ping module state
    \Input *pQueueEntry - to be completed
    \Input *pResponse   - to be completed
    \Input *pSockAddr   - to be completed
    \Input iLen         - to be completed

    \Output
        int32_t         - ident token

    \Version 11/18/2002 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoPingParseResponse(ProtoPingRefT *pProtoPing, ProtoPingQueueT *pQueueEntry, EchoResponseT *pResponse, struct sockaddr *pSockAddr, int32_t iLen)
{
    int32_t iIdentToken;
    
    memset(pQueueEntry, 0, sizeof(*pQueueEntry));
    
    pQueueEntry->uAddr = (pResponse->header.ipv4.srcaddr[0]<<24)|(pResponse->header.ipv4.srcaddr[1]<<16)|(pResponse->header.ipv4.srcaddr[2]<<8)|(pResponse->header.ipv4.srcaddr[3]<<0);
    pQueueEntry->uTick = SockaddrInGetMisc(pSockAddr);
    pQueueEntry->uType = pResponse->header.proto.icmp.type;
    
    #if DIRTYCODE_LOGGING
    pQueueEntry->pType = (pQueueEntry->uType <= 16) ? _ProtoPing_strResponseTypes[pQueueEntry->uType] : "invalid ICMP";
    #endif
    
    if (pQueueEntry->uType == ICMP_ECHOREPLY)
    {
        pQueueEntry->uSeqn = (pResponse->header.proto.icmp.seq[0]<<8)|(pResponse->header.proto.icmp.seq[1]<<0);
        iLen = iLen - (sizeof(*pResponse) - sizeof(pResponse->body.data.userdata));
        if (iLen > 0)
        {
            pQueueEntry->uSize = (unsigned)iLen;
        }
        iIdentToken = (pResponse->header.proto.icmp.idn[0]<<8)|(pResponse->header.proto.icmp.idn[1]<<0);
    }
    else
    {
        pQueueEntry->uSeqn = (pResponse->body.header_org.proto.icmp.seq[0]<<8)|(pResponse->body.header_org.proto.icmp.seq[1]<<0);
        iIdentToken = (pResponse->body.header_org.proto.icmp.idn[0]<<8)|(pResponse->body.header_org.proto.icmp.idn[1]<<0);
    }

    return(iIdentToken);
}

/*F********************************************************************************/
/*!
    \Function _ProtoPingIcmpCallback

    \Description
        Callback to process ICMP response

    \Input *pSocket - pointer to connection
    \Input iFlags   - unused
    \Input *pData   - ProtoPingRefT pointer

    \Output
        int32_t     - zero

    \Version 04/03/2002 (gschaefer)
*/
/********************************************************************************F*/
static int32_t _ProtoPingIcmpCallback(SocketT *pSocket, int32_t iFlags, void *pData)
{
    ProtoPingRefT *pProtoPing = (ProtoPingRefT *)pData;
    char strResponse[sizeof(EchoResponseT)];
    ProtoPingQueueT Response, *pRequest;
    struct sockaddr SockAddr;
    int32_t iLen, iIdentToken, iQueue;
    EchoResponseT *pEchoResponse = (EchoResponseT *)strResponse;

    // process all pending packets
    while ((iLen = SocketRecvfrom(pProtoPing->pSocket, strResponse, sizeof(strResponse), 0, &SockAddr, (iLen=sizeof(SockAddr), &iLen))) > 0)
    {
        // parse the packet
        if ((iIdentToken = _ProtoPingParseResponse(pProtoPing, &Response, pEchoResponse, &SockAddr, iLen)) != pProtoPing->iIdentToken)
        {
            if (pProtoPing->uProtocol == IPPROTO_ICMP)
            {
                #if DIRTYCODE_DEBUG
                if (pProtoPing->bVerbose)
                {
                    NetPrintf(("protoping: received a non-dirtysock ping (ident=0x%02x%02x) of type %s from %a\n",
                        (uint8_t)(iIdentToken>>8), (uint8_t)iIdentToken, Response.pType, Response.uAddr));
                }
                #endif
                continue;
            }
        }
        if (pProtoPing->uProtocol == IPPROTO_UDP)
        {
            #if DIRTYCODE_LOGGING
            if ((Response.uType != ICMP_TIMEEXCEEDED) && (Response.uType != ICMP_DESTUNREACH))
            {
                NetPrintf(("protoping: ignoring ICMP of type %s from address %a\n", Response.pType, Response.uAddr));
                continue;
            }
            else if (pProtoPing->bVerbose)
            {
                NetPrintf(("protoping: received ICMP of type %s from address %a\n", Response.pType, Response.uAddr));
            }
            #endif
        }

        #if PROTOPING_VERBOSE
        NetPrintf(("protoping: received ping response from %a\n", Response.uAddr));
        #endif
    
        // make sure we own resources
        NetCritEnter(&pProtoPing->ThreadCrit);
        
        // find request in request queue
        for (iQueue = 0; iQueue < pProtoPing->iQueueTail; iQueue++)
        {
            uint32_t uSeqnMatch;

            pRequest = &pProtoPing->RequestQueue[iQueue];

            // calculate sequence match if ICMP, otherwise we do not have a sequence to match
            uSeqnMatch = (pProtoPing->uProtocol == IPPROTO_ICMP) ? (pRequest->uSeqn == Response.uSeqn) : !(pRequest->uFlag & PROTOPING_FLAG_RECEIVED); 
            if (uSeqnMatch != 0)
            {
                if (pRequest->uFlag & PROTOPING_FLAG_RECEIVED)
                {
                    NetPrintf(("protoping: got redundant response for sequence %d\n"));
                    break;
                }
                pRequest->uAddr = Response.uAddr;
                pRequest->uPing = Response.uTick - pRequest->uTick;
                if (pRequest->uPing == 0)
                {
                    pRequest->uPing = 1;
                }
                if ((pRequest->uSize = Response.uSize) != 0)
                {
                    if ((pRequest->pData = (char *)DirtyMemAlloc(pRequest->uSize, PROTOPING_MEMID, pProtoPing->iMemGroup, pProtoPing->pMemGroupUserData)) != NULL)
                    {
                        ds_memcpy(pRequest->pData, pEchoResponse->body.data.userdata, pRequest->uSize);
                    }
                    else
                    {
                        NetPrintf(("protoping: unable to allocate memory for ping response data\n"));
                        pRequest->uSize = 0;
                    }
                }
                pRequest->uType = Response.uType;
                pRequest->uFlag |= PROTOPING_FLAG_RECEIVED;
                break;
            }
        }

        // unclaimed?
        #if DIRTYCODE_LOGGING
        if (iQueue == pProtoPing->iQueueTail)
        {
            NetPrintf(("protoping: received unclaimed %s from %a\n", Response.pType, Response.uAddr));
        }
        else if (pProtoPing->bVerbose)
        {
            NetPrintf(("protoping: received %s from %a\n", Response.pType, Response.uAddr));
        }
        #endif

        // release resources
        NetCritLeave(&pProtoPing->ThreadCrit);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoPingSocketAlloc

    \Description
        Callback to process QoS status changes

    \Input *pProtoPing  - ping module state

    \Output
        SocketT *       - pointer to new socket, or NULL

    \Version 05/18/2010 (jbrookes) Extracted from ProtoPingCreate()
*/
/********************************************************************************F*/
static SocketT *_ProtoPingSocketAlloc(ProtoPingRefT *pProtoPing)
{
    struct sockaddr BindAddr;
    uint32_t uBindPort;
    int32_t iResult;

    // do we already have a socket?
    if (pProtoPing->pSocket != NULL)
    {
        return(pProtoPing->pSocket);
    }

    // allocate raw socket (note that protocol must be ICMP)
    if ((pProtoPing->pSocket = SocketOpen(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == NULL)
    {
        NetPrintf(("protoping: could not allocate socket\n"));
        return(NULL);
    }

    // bind for ping
    uBindPort = (pProtoPing->uProtocol == IPPROTO_ICMP) ? IPPROTO_ICMP : pProtoPing->uPingPort;
    SockaddrInit(&BindAddr, AF_INET);
    SockaddrInSetPort(&BindAddr, uBindPort);
    if ((iResult = SocketBind(pProtoPing->pSocket, &BindAddr, sizeof(BindAddr))) != SOCKERR_NONE)    
    {
        NetPrintf(("protoping: error %d binding socket to port %d\n", iResult, uBindPort));
        SocketClose(pProtoPing->pSocket);
        return(NULL);
    }

    // set the callback
    SocketCallback(pProtoPing->pSocket, CALLB_RECV, 5000, pProtoPing, &_ProtoPingIcmpCallback);

    // return socket to caller
    return(pProtoPing->pSocket);
}

/*F********************************************************************************/
/*!
    \Function _ProtoPingQoSCallback

    \Description
        Callback to process QoS status changes

    \Input *pQosApi   - QosApiRefT pointer
    \Input *pCBInfo   - pointer to qosapi info
    \Input eCBType    - callback type
    \Input *pUserData - ProtoPingRefT pointer

    \Version 06/09/2009 (cadam)
*/
/********************************************************************************F*/
static void _ProtoPingQosCallback(QosApiRefT *pQosApi, QosApiCBInfoT *pCBInfo, QosApiCBTypeE eCBType, void *pUserData)
{
    int32_t iQueue;
    ProtoPingRefT *pProtoPing = (ProtoPingRefT *)pUserData;
    ProtoPingQueueT *pRequest;

    // make sure we own resources
    NetCritEnter(&pProtoPing->ThreadCrit);
        
    // find request in request queue
    for (iQueue = 0; iQueue < pProtoPing->iQueueTail; iQueue++)
    {
        pRequest = &pProtoPing->RequestQueue[iQueue];
        if (pRequest->uSeqn == pCBInfo->pQosInfo->uRequestId)
        {
            if (pRequest->uFlag & QOSAPI_STATFL_COMPLETE)
            {
                NetPrintf(("protoping: got redundant response for sequence %d\n"));
                break;
            }
            pRequest->uAddr = pCBInfo->pQosInfo->uAddr;
            pRequest->uPing = pCBInfo->pQosInfo->iMinRTT;
            if (pRequest->uPing == 0)
            {
                pRequest->uPing = 1;
            }
            if (pCBInfo->pQosInfo->uFlags & QOSAPI_STATFL_FAILED)
            {
                pRequest->uType = ICMP_DESTUNREACH;
            }
            else
            {
                pRequest->uType = ICMP_ECHOREPLY;
            }
            pRequest->uFlag |= pCBInfo->pQosInfo->uFlags;
            break;
        }
    }

    // unclaimed?
    #if DIRTYCODE_LOGGING
    if (iQueue == pProtoPing->iQueueTail)
    {
        NetPrintf(("protoping: received unclaimed probe from %a\n", pCBInfo->pQosInfo->uAddr));
    }
    else if (pProtoPing->bVerbose)
    {
        NetPrintf(("protoping: received probe from %a\n", pCBInfo->pQosInfo->uAddr));
    }
    #endif

    // release resources
    NetCritLeave(&pProtoPing->ThreadCrit);
}

/*F********************************************************************************/
/*!
    \Function _ProtoPingCalcChecksum

    \Description
        Calculate checksum of specified data

    \Input *pData   - pointer to data to checksum
    \Input iLen     - pointer to length of data to checksum

    \Version 05/18/2010 (jbrookes) Extracted from _ProtoPingRequest()
*/
/********************************************************************************F*/
static uint32_t _ProtoPingCalcChecksum(uint8_t *pData, int32_t iLen)
{
    uint32_t uCheckVal;

    // calc the checksum
    for (uCheckVal = 0; iLen > 1; iLen -= 2)
    {
        uCheckVal += (*pData++)<<8;
        uCheckVal += (*pData++)<<0;
    }
    if (iLen > 0)
    {
        uCheckVal += (*pData++)<<8;
    }

    // handle rollover
    uCheckVal = (uCheckVal >> 16) + (uCheckVal & 65535);
    uCheckVal = (uCheckVal >> 16) + (uCheckVal & 65535);
    uCheckVal = ~uCheckVal;

    // return to caller
    return(uCheckVal);
}

/*F********************************************************************************/
/*!
    \Function _ProtoPingRequest

    \Description
        Send a ping request,

    \Input *pProtoPing  - reference pointer
    \Input uAddress     - target address
    \Input *pData       - user data
    \Input iDataLen     - length of data
    \Input iTimeout     - timeout in ms, or zero for default
    \Input iTtl         - time to live, or zero for default
    \Input bServer      - TRUE if a server ping, else false

    \Output
        int32_t         - sequence number of packet (zero=failed)

    \Version 04/03/2002 (gschaefer)
*/
/********************************************************************************F*/
static int32_t _ProtoPingRequest(ProtoPingRefT *pProtoPing, uint32_t uAddress, const unsigned char *pData, int32_t iDataLen, int32_t iTimeout, int32_t iTtl, uint32_t bServer)
{
    EchoRequestT EchoRequest;
    struct sockaddr SockAddr;
    uint32_t uCheckVal;
    int32_t iLen;

    #if PROTOPING_VERBOSE
    NetPrintf(("protoping: pinging address %a\n", uAddress));
    #endif

    if (pProtoPing->eType == PROTOPING_TYPE_ICMP)
    {
        // make sure we have a socket
        if ((pProtoPing->pSocket = _ProtoPingSocketAlloc(pProtoPing)) == NULL)
        {
            return(-1);
        }

        // easy string handling
        if (iDataLen < 0)
        {
            iDataLen = (int32_t)strlen((const char *)pData)+1;
        }
    
        // handle defaults
        if ((iTtl <= 0) || (iTtl > 255))
        {
            iTtl = PROTOPING_DEFAULT_TTL;
        }
        if (iTimeout <= 0)
        {
            iTimeout = PROTOPING_DEFAULT_TIMEOUT;
        }

        // setup the target address
        SockaddrInit(&SockAddr, AF_INET);
        SockaddrInSetAddr(&SockAddr, uAddress);

        // advance the sequence number and wrap at 16bits
        if (++pProtoPing->iSeqn > 65535)
        {
            pProtoPing->iSeqn = 1;
        }
        memset(&EchoRequest, 0, sizeof(EchoRequest));

        // setup the ip packet
        EchoRequest.header.ipv4.verslen = 0x45;
        EchoRequest.header.ipv4.srcaddr[0] = (unsigned char)(pProtoPing->uLocalAddr>>24);
        EchoRequest.header.ipv4.srcaddr[1] = (unsigned char)(pProtoPing->uLocalAddr>>16);
        EchoRequest.header.ipv4.srcaddr[2] = (unsigned char)(pProtoPing->uLocalAddr>>8);
        EchoRequest.header.ipv4.srcaddr[3] = (unsigned char)(pProtoPing->uLocalAddr>>0);
        EchoRequest.header.ipv4.dstaddr[0] = (unsigned char)(uAddress>>24);
        EchoRequest.header.ipv4.dstaddr[1] = (unsigned char)(uAddress>>16);
        EchoRequest.header.ipv4.dstaddr[2] = (unsigned char)(uAddress>>8);
        EchoRequest.header.ipv4.dstaddr[3] = (unsigned char)(uAddress>>0);
        EchoRequest.header.ipv4.proto = pProtoPing->uProtocol;
        EchoRequest.header.ipv4.time = (unsigned char)iTtl;

        // set up the icmp packet
        if (pProtoPing->uProtocol == IPPROTO_ICMP)
        {
            EchoRequest.header.proto.icmp.type = ICMP_ECHOREQ;
            EchoRequest.header.proto.icmp.idn[0] = (uint8_t)(pProtoPing->iIdentToken>>8);
            EchoRequest.header.proto.icmp.idn[1] = (uint8_t)(pProtoPing->iIdentToken>>0);
            EchoRequest.header.proto.icmp.seq[0] = (uint8_t)(pProtoPing->iSeqn>>8);
            EchoRequest.header.proto.icmp.seq[1] = (uint8_t)(pProtoPing->iSeqn>>0);
        }
        // set up the udp packet
        if (pProtoPing->uProtocol == IPPROTO_UDP)
        {
            EchoRequest.header.proto.udp.srcport[0] = (uint8_t)(pProtoPing->uPingPort >> 8);
            EchoRequest.header.proto.udp.srcport[1] = (uint8_t)(pProtoPing->uPingPort);
            EchoRequest.header.proto.udp.dstport[0] = (uint8_t)(pProtoPing->uPingPort >> 8);
            EchoRequest.header.proto.udp.dstport[1] = (uint8_t)(pProtoPing->uPingPort);
            EchoRequest.header.proto.udp.length[0] = 0;
            EchoRequest.header.proto.udp.length[1] = 8;
            //$$tmp - udp packet length currently hard-coded at 8
        }

        // clamp the data size
        if (iDataLen > (int32_t)sizeof(EchoRequest.data.userdata))
        {
            NetPrintf(("protoping: clamping data (length=%d) to %d bytes\n", iDataLen, sizeof(EchoRequest.data.userdata)));
            iDataLen = sizeof(EchoRequest.data.userdata);
        }
        // copy over the data
        ds_memcpy(EchoRequest.data.userdata, pData, iDataLen);
        // align the size
        iDataLen = (iDataLen + 3) & 0x7ffc;

        // adjust iLength
        pData = (const unsigned char *)&EchoRequest;
        iDataLen += sizeof(EchoRequest)-sizeof(EchoRequest.data.userdata);

        // now that we know the size, fill in the IPV4 length header
        EchoRequest.header.ipv4.length[0] = (uint8_t)(iDataLen >> 8);
        EchoRequest.header.ipv4.length[1] = (uint8_t)(iDataLen >> 0);

        // calc the checksum for the ipv4 header
        uCheckVal = _ProtoPingCalcChecksum((uint8_t *)&EchoRequest.header, sizeof(EchoRequest.header.ipv4));

        // save the result
        EchoRequest.header.ipv4.check[0] = (unsigned char)(uCheckVal>>8);
        EchoRequest.header.ipv4.check[1] = (unsigned char)(uCheckVal>>0);

        if (pProtoPing->uProtocol == IPPROTO_ICMP)
        {
            // calc the checksum for the icmp header
            uCheckVal = _ProtoPingCalcChecksum((uint8_t *)&EchoRequest.header.proto.icmp, iDataLen-sizeof(EchoRequest.header.ipv4));

            // save the result
            EchoRequest.header.proto.icmp.check[0] = (unsigned char)(uCheckVal>>8);
            EchoRequest.header.proto.icmp.check[1] = (unsigned char)(uCheckVal>>0);
        }
        if (pProtoPing->uProtocol == IPPROTO_UDP)
        {
            // create psuedo-ipv4 header required for udp checksum calculation
            uint8_t aHeader[1024];

            // source address
            aHeader[ 0] = (uint8_t)(pProtoPing->uLocalAddr >> 24);
            aHeader[ 1] = (uint8_t)(pProtoPing->uLocalAddr >> 16);
            aHeader[ 2] = (uint8_t)(pProtoPing->uLocalAddr >> 8);
            aHeader[ 3] = (uint8_t)(pProtoPing->uLocalAddr >> 0);
            // destination address
            aHeader[ 4] = (uint8_t)(uAddress >> 24);
            aHeader[ 5] = (uint8_t)(uAddress >> 16);
            aHeader[ 6] = (uint8_t)(uAddress >> 8);
            aHeader[ 7] = (uint8_t)(uAddress >> 0);
            // zero, protocol, udp packet length
            aHeader[ 8] = 0;
            aHeader[ 9] = 0x11;
            aHeader[10] = EchoRequest.header.proto.udp.length[0];
            aHeader[11] = EchoRequest.header.proto.udp.length[1];
            // udp header
            ds_memcpy(&aHeader[12], &EchoRequest.header.proto.udp, sizeof(EchoRequest.header.proto.udp));
            // udp payload
            //$$ temp -- currently nothing

            // calc the checksum for the udp packet
            uCheckVal = _ProtoPingCalcChecksum(aHeader, 20);

            // save the result
            EchoRequest.header.proto.udp.check[0] = (unsigned char)(uCheckVal>>8);
            EchoRequest.header.proto.udp.check[1] = (unsigned char)(uCheckVal>>0);
        }

        // add request queue entry
        if (pProtoPing->iQueueTail >= pProtoPing->iQueueSize)
        {
            NetPrintf(("protoping: request queue overflow\n"));
            return(-1);
        }

        // send the packet
        iLen = SocketSendto(pProtoPing->pSocket, (char *)&EchoRequest, iDataLen, 0, &SockAddr, sizeof(SockAddr));
        if (iLen >= 0)
        {
            ProtoPingQueueT *pRequest = &pProtoPing->RequestQueue[pProtoPing->iQueueTail++];
            memset(pRequest, 0, sizeof(*pRequest));
            pRequest->uAddr = uAddress;
            pRequest->uTime = (uint32_t)iTimeout;
            pRequest->uSeqn = (uint16_t)pProtoPing->iSeqn;
            pRequest->uFlag = bServer ? PROTOPING_FLAG_SERVER : 0;
            pRequest->uTick = NetTick();
        }
        else
        {
            NetPrintf(("protoping: error %d sending icmp ping\n", iLen));
            return(-1);
        }
    }
    else
    {
        ProtoPingQueueT *pRequest;

        if (iTimeout <= 0)
        {
            iTimeout = QosApiStatus(pProtoPing->pQosApi, 'time', 0, NULL, 0);
        }
        else
        {
            QosApiControl(pProtoPing->pQosApi, 'time', iTimeout, NULL);
            iTimeout = QosApiStatus(pProtoPing->pQosApi, 'time', 0, NULL, 0);
        }

        if (bServer)
        {
            char strAddrText[20];
            pProtoPing->iSeqn = QosApiServiceRequest(pProtoPing->pQosApi, SocketInAddrGetText(uAddress, strAddrText, sizeof(strAddrText)), 0, QOSAPI_REQUEST_LATENCY);
        }
        else
        {
            NetPrintf(("protoping: error client pings are unsported\n"));
            return(-1);
        }


        if (pProtoPing->iSeqn == 0)
        {
            NetPrintf(("protoping: error sending udp ping\n"));
            return(-1);
        }

        pRequest = &pProtoPing->RequestQueue[pProtoPing->iQueueTail++];
        memset(pRequest, 0, sizeof(*pRequest));
        pRequest->uAddr = uAddress;
        pRequest->uTime = (uint32_t)iTimeout;
        pRequest->uSeqn = (uint16_t)pProtoPing->iSeqn;
        pRequest->uFlag = PROTOPING_FLAG_SERVER;
        pRequest->uTick = NetTick();
    }

    // return sequence number
    return(pProtoPing->iSeqn);
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ProtoPingCreate

    \Description
        Create new ping module

    \Input iMaxPings    - maximum number of ping requests that can be issued at one time

    \Output
        ProtoPingRefT * - reference pointer (needed for all other calls)

    \Version 04/03/2002 (gschaefer)
*/
/********************************************************************************F*/
ProtoPingRefT *ProtoPingCreate(int32_t iMaxPings)
{
    ProtoPingRefT *pProtoPing;
    int32_t iStateSize;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // if already created, just ref it
    if (_ProtoPing_pRef != NULL)
    {
        _ProtoPing_pRef->iRefCount += 1;
        return(_ProtoPing_pRef);
    }

    // iMaxPings of zero means use the default
    if (iMaxPings == 0)
    {
        iMaxPings = PROTOPING_DEFAULT_MAXPINGS;
    }
    
    // calculate size of ref
    iStateSize = sizeof(*pProtoPing) + ((iMaxPings-1) * sizeof(ProtoPingQueueT));

    // allocate and init module state
    if ((pProtoPing = DirtyMemAlloc(iStateSize, PROTOPING_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protoping: could not allocate module state\n"));
        return(NULL);
    }
    memset(pProtoPing, 0, iStateSize);
    pProtoPing->iMemGroup = iMemGroup;
    pProtoPing->pMemGroupUserData = pMemGroupUserData;

    // set defaults
    pProtoPing->iIdentToken = IDENT_TOKEN;
    pProtoPing->uProtocol = IPPROTO_ICMP;
    pProtoPing->uPingPort = PROTOPING_DEFAULT_PORT;
    
    // get local address
    pProtoPing->uLocalAddr = SocketGetLocalAddr();

    // save queue size
    pProtoPing->iQueueSize = iMaxPings;
    // need to sync access to data
    NetCritInit(&pProtoPing->ThreadCrit, "protoping");

    // save ref and return state pointer
    pProtoPing->iRefCount = 1;
    _ProtoPing_pRef = pProtoPing;
    return(pProtoPing);
}

/*F********************************************************************************/
/*!
    \Function ProtoPingDestroy

    \Description
        Destroy the module

    \Input *pProtoPing    - reference pointer

    \Version 04/03/2002 (gschaefer)
*/
/********************************************************************************F*/
void ProtoPingDestroy(ProtoPingRefT *pProtoPing)
{
    ProtoPingQueueT *pQueue;
    int iQueue;

    // if we aren't the last, just decrement the refcount and return
    if (--pProtoPing->iRefCount > 0)
    {
        return;
    }

    // dispose of socket
    if (pProtoPing->pSocket != NULL)
    {
        SocketClose(pProtoPing->pSocket);
    }

    // free any unclaimed response data
    for (iQueue = 0; iQueue < pProtoPing->iQueueTail; iQueue++)
    {
        pQueue = &pProtoPing->RequestQueue[iQueue];
        if (pQueue->pData != NULL)
        {
            DirtyMemFree(pQueue->pData, PROTOPING_MEMID, pProtoPing->iMemGroup, pProtoPing->pMemGroupUserData);
        }
    }

    // free the qos ref if there is one
    if (pProtoPing->pQosApi != NULL)
    {
        QosApiDestroy(pProtoPing->pQosApi);
    }

    // done with semaphore
    NetCritKill(&pProtoPing->ThreadCrit);
    
    // done with state
    DirtyMemFree(pProtoPing, PROTOPING_MEMID, pProtoPing->iMemGroup, pProtoPing->pMemGroupUserData);

    // clear ref
    _ProtoPing_pRef = NULL;
}

/*F********************************************************************************/
/*!
    \Function ProtoPingControl

    \Description
        Configure ProtoPing module

    \Input *pProtoPing  - module state ref
    \Input iControl      - config selector
    \Input iValue       - selector specific
    \Input *pValue      - selector specific
        
    \Output
        int32_t         - selector specific

    \Notes
        iControl can be:

        \verbatim
            'idnt' - set ping packet identifier (16 bits)
            'spam' - set verbose debug output
            'type' - set the ping type to use
        \endverbatim

    \Version 11/18/2002 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoPingControl(ProtoPingRefT *pProtoPing, int32_t iControl, int32_t iValue, void *pValue)
{
    if (iControl == 'idnt')
    {
        pProtoPing->iIdentToken = (int16_t)iValue;
        return(0);
    }
    if (iControl == 'port')
    {
        NetPrintf(("protoping: setting non-icmp ping port to %d\n", iValue));
        pProtoPing->uPingPort = (uint16_t)iValue;
        return(0);
    }
    if (iControl == 'prot')
    {
        if ((iValue == IPPROTO_ICMP) || (iValue == IPPROTO_UDP))
        {
            NetPrintf(("protoping: using protocol %d for pings\n", iValue));
            pProtoPing->uProtocol = (uint8_t)iValue;
            return(0);
        }
        else
        {
            NetPrintf(("protoping: unsupported protocol %d ignored\n", iValue));
            return(-1);
        }
    }
    if (iControl == 'spam')
    {
        pProtoPing->bVerbose = iValue;
        if (pProtoPing->pQosApi != NULL)
        {
            return(QosApiControl(pProtoPing->pQosApi, iControl, iValue, pValue));
        }
    }
    if (iControl == 'type')
    {
        pProtoPing->eType = iValue;
        if (pProtoPing->eType == PROTOPING_TYPE_QOS)
        {
            if (pProtoPing->pQosApi == NULL)
            {
                DirtyMemGroupEnter(pProtoPing->iMemGroup, pProtoPing->pMemGroupUserData);
                pProtoPing->pQosApi = QosApiCreate(_ProtoPingQosCallback, pProtoPing, *(int32_t *)pValue);
                DirtyMemGroupLeave();
                // fail if the ref was not created
                if (pProtoPing->pQosApi == NULL)
                {
                    pProtoPing->eType = PROTOPING_TYPE_ICMP;
                    NetPrintf(("protoping: error, qosapicreate failed\n"));
                    return(-1);
                }
                else
                {
                    //configure latency request to use only 1 probe
                    QosApiControl(pProtoPing->pQosApi, 'lpco', 1, NULL);
                }
            }
            else
            {
                NetPrintf(("protoping: warning, qosapi ref already exists\n"));
            }
        }
        return(0);
    }
    if (pProtoPing->eType == PROTOPING_TYPE_QOS)
    {
        // pass-through to QosApiControl
        return(QosApiControl(pProtoPing->pQosApi, iControl, iValue, pValue));
    }
    else
    {
        return(-1);
    }
}

/*F********************************************************************************/
/*!
    \Function ProtoPingRequest

    \Description
        Send a ping request uSockAddrg opaque address form.

    \Input *pRef        - reference pointer
    \Input *pAddr       - address to ping
    \Input *pData       - pointer to user data
    \Input iDataLen     - iLength of user data
    \Input iTimeout     - timeout in ms, or zero for default
    \Input iTtl         - time to live, or zero for default

    \Output
        int32_t         - sequence number of packet (zero=failed)

    \Version 04/12/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoPingRequest(ProtoPingRefT *pRef, DirtyAddrT *pAddr, const unsigned char *pData, int32_t iDataLen, int32_t iTimeout, int32_t iTtl)
{
    uint32_t uAddress;

    // convert opaque address type to 32bit address type
    DirtyAddrToHostAddr(&uAddress, sizeof(uAddress), pAddr);

    // make the request
    return(_ProtoPingRequest(pRef, uAddress, pData, iDataLen, iTimeout, iTtl, FALSE));
}

/*F********************************************************************************/
/*!
    \Function ProtoPingRequestServer

    \Description
        Ping the given server address.

    \Input *pRef            - reference pointer
    \Input uServerAddress   - address of server to ping
    \Input *pData           - pointer to user data
    \Input iDataLen         - iLength of user data
    \Input iTimeout         - timeout in ms, or zero for default
    \Input iTtl             - time to live, or zero for default

    \Output
        int32_t             - sequence number of packet (zero=failed)

    \Version 04/12/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoPingRequestServer(ProtoPingRefT *pRef, uint32_t uServerAddress, const unsigned char *pData, int32_t iDataLen, int32_t iTimeout, int32_t iTtl)
{
    return(_ProtoPingRequest(pRef, uServerAddress, pData, iDataLen, iTimeout, iTtl, TRUE));
}

/*F********************************************************************************/
/*!
    \Function ProtoPingResponse

    \Description
        Get ping response from queue

    \Input *pProtoPing  - reference pointer
    \Input *pBuffer     - space for user data
    \Input *pBufLen     - size of buffer on entry, actual data size on exit
    \Input *pResponse   - pointer to ping response structure

    \Output
        int32_t         - zero if nothing pending, else ping (negative=timeout)

    \Version 04/03/2002 (gschaefer)
*/
/********************************************************************************F*/
int32_t ProtoPingResponse(ProtoPingRefT *pProtoPing, unsigned char *pBuffer, int32_t *pBufLen, ProtoPingResponseT *pResponse)
{
    ProtoPingQueueT *pRequest;
    int32_t iPing, iQueue;
    uint32_t uCurTime;
    uint8_t bTimeout;
        
    // if empty queue, bail
    if (pProtoPing->iQueueTail == 0)
    {
        return(0);
    }

    // make sure we own resources
    NetCritEnter(&pProtoPing->ThreadCrit);
    
    // scan request queue for a response
    for (iPing=0, iQueue=0, uCurTime=NetTick(); iQueue < pProtoPing->iQueueTail; iQueue++)
    {
        pRequest = &pProtoPing->RequestQueue[iQueue];
        bTimeout = (uCurTime - pRequest->uTick) > pRequest->uTime;
        if ((pProtoPing->eType == PROTOPING_TYPE_QOS) && (pRequest->uFlag & QOSAPI_STATFL_FAILED))
        {
            // timeout early if the request failed
            bTimeout = TRUE;
        }
        if ((pRequest->uFlag & PROTOPING_FLAG_RECEIVED) || (bTimeout == TRUE))
        {
            // save ping for return to caller
            iPing = (!bTimeout) ? (signed)pRequest->uPing : -1;
        
            // fill in response struct
            if (pResponse != NULL)
            {
                memset(pResponse, 0, sizeof(*pResponse));
                DirtyAddrFromHostAddr(&pResponse->Addr, &pRequest->uAddr);
                pResponse->uAddr = pRequest->uAddr;
                pResponse->iPing = iPing;
                pResponse->uType = pRequest->uType;
                pResponse->uSeqn = pRequest->uSeqn;
                pResponse->bServer = (pRequest->uFlag & PROTOPING_FLAG_SERVER) ? TRUE : FALSE;
            }

            // copy response data, if any, to caller
            if (pRequest->pData != NULL)
            {
                if (pBufLen != NULL)
                {
                    // fill in buffer iLength
                    if (*pBufLen > pRequest->uSize)
                    {
                        *pBufLen = pRequest->uSize;
                    }

                    // fill in buffer
                    if (pBuffer != NULL)
                    {
                        ds_memcpy(pBuffer, pRequest->pData, *pBufLen);
                    }
                }
                
                // dispose of user data buffer
                DirtyMemFree(pRequest->pData, PROTOPING_MEMID, pProtoPing->iMemGroup, pProtoPing->pMemGroupUserData);
            }
            
            // remove from queue
            memmove(pRequest, pRequest+1, (pProtoPing->iQueueTail - iQueue) * sizeof(*pRequest));
            pProtoPing->iQueueTail -= 1;
            break;
        }
    }

    // release resources
    NetCritLeave(&pProtoPing->ThreadCrit);

    // return ping
    return(iPing);
}

