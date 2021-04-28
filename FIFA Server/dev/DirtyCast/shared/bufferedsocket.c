/*H********************************************************************************/
/*!
    \File bufferedsocket.c

    \Description
        Buffered socket read/write routines, aimed at reducing the number of
        send() and recv() calls.

    \Copyright
        Copyright (c) 2007 Electronic Arts Inc.

    \Version 04/23/2007 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"

#include "dirtycast.h"
#include "bufferedsocket.h"

/*** Defines **********************************************************************/

//! memid for gameserver module
#define BUFFEREDSOCKET_MEMID            ('bsoc')

//! is buffering enabled for sending
#define BUFFEREDSOCKET_SEND             (TRUE)

//! is buffering enabled for receiving
#define BUFFEREDSOCKET_RECV             (TRUE)

/*** Type Definitions *************************************************************/

//! module state
struct BufferedSocketT
{
    //! module memory group
    int32_t iMemGroup;
    void *pMemGroupUserData;

    //! SocketT pointer
    SocketT *pSocket;

    //! source and destination ports of the TCP connection the buffered socket is associated with
    uint16_t uSrcPort;
    uint16_t uDstPort;

    //! send/flush mutex
    NetCritT SendCrit;

    //! send buffer pointer
    char *pSendBuf;

    //! recv buffer pointer
    char *pRecvBuf;

    //! current offset in send buffer
    int32_t iSendBufPos;

    //! current offset in recv buffer
    int32_t iRecvBufPos;

    //! send buffer size
    int32_t iSendBufLen;

    //! recv buffer size
    int32_t iRecvBufLen;

    //! last write time
    uint32_t uLastWrite;

    //! module stats
    BufferedSocketInfoT BufferedSocketInfo;
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function BufferedSocketOpen

    \Description
        Create a buffered socket.

    \Input iType            - type of socket (SOCK_STREAM, SOCK_DGRAM)
    \Input uPort            - port socket should be bound to
    \Input iBufLen          - length of buffer
    \Input *pSocket         - socket to use, or NULL to create/bind one

    \Output
        BufferedSocketT *   - new buffered socket, or NULL

    \Version 04/23/2007 (jbrookes)
*/
/********************************************************************************F*/
BufferedSocketT *BufferedSocketOpen(int32_t iType, uint16_t uPort, int32_t iBufLen, SocketT *pSocket)
{
    BufferedSocketT *pBufferedSocket;
    int32_t iResult;
    struct sockaddr BindAddr, SrcAddr, DestAddr;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // did the user not supply a socket?
    if (pSocket == NULL)
    {
        // first, open the socket
        if ((pSocket = SocketOpen(AF_INET, iType, IPPROTO_IP)) == NULL)
        {
            DirtyCastLogPrintf("bufferedsocket: unable to open socket\n");
            return(NULL);
        }

        // bind socket to specified port
        SockaddrInit(&BindAddr, AF_INET);
        SockaddrInSetPort(&BindAddr, uPort);
        if ((iResult = SocketBind(pSocket, &BindAddr, sizeof(BindAddr))) != SOCKERR_NONE)
        {
            DirtyCastLogPrintf("bufferedsocket: error %d binding to port\n", iResult);
            SocketClose(pSocket);
            return(NULL);
        }
    }
 
    // allocate and clear module state
    if ((pBufferedSocket = (BufferedSocketT *)DirtyMemAlloc(sizeof(*pBufferedSocket), BUFFEREDSOCKET_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        DirtyCastLogPrintf("bufferedsocket: could not allocate module state\n");
        SocketClose(pSocket);
        return(NULL);
    }
    ds_memclr(pBufferedSocket, sizeof(*pBufferedSocket));

    // init send mutex
    NetCritInit(&pBufferedSocket->SendCrit, "bufferedsocket");

    // init module state
    pBufferedSocket->iMemGroup = iMemGroup;
    pBufferedSocket->pMemGroupUserData = pMemGroupUserData;
    pBufferedSocket->pSocket = pSocket;
    pBufferedSocket->BufferedSocketInfo.uBufSize = iBufLen;

    /* Query source and destination ports of TCP connection associated with the socket.
       For the scenario where the user does not specify a socket, the passed in port might
       have been zero to bind the new socket to a random port. 

       We will re-attempt this in the 'stat' selector in case the socket is no yet connected here.
       But we still have to do it here for sockets that are never polled with 'stat', like
       the ones that are created as a result of a call to accept(). */
    SocketInfo(pSocket, 'bind', 0, &SrcAddr, sizeof(SrcAddr));
    pBufferedSocket->uSrcPort = SockaddrInGetPort(&SrcAddr);
    SocketInfo(pSocket, 'peer', 0, &DestAddr, sizeof(DestAddr));
    pBufferedSocket->uDstPort = SockaddrInGetPort(&DestAddr);

    // allocate send and recv buffers
    if ((pBufferedSocket->pSendBuf = (char *)DirtyMemAlloc(iBufLen, BUFFEREDSOCKET_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        DirtyCastLogPrintf("bufferedsocket: could not allocate send buffer\n");
        BufferedSocketClose(pBufferedSocket);
        return(NULL);
    }
    pBufferedSocket->iSendBufLen = iBufLen;
    if ((pBufferedSocket->pRecvBuf = (char *)DirtyMemAlloc(iBufLen, BUFFEREDSOCKET_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        DirtyCastLogPrintf("bufferedsocket: could not allocate send buffer\n");
        BufferedSocketClose(pBufferedSocket);
        return(NULL);
    }
    pBufferedSocket->iRecvBufLen = iBufLen;

    // return module ref to caller
    return(pBufferedSocket);
}

/*F********************************************************************************/
/*!
    \Function BufferedSocketClose

    \Description
        Close a buffered socket

    \Input *pBufferedSocket     - module state

    \Version 04/23/2007 (jbrookes)
*/
/********************************************************************************F*/
void BufferedSocketClose(BufferedSocketT *pBufferedSocket)
{
    // close the socket
    if (pBufferedSocket->pSocket != NULL)
    {
        SocketClose(pBufferedSocket->pSocket);
    }

    // free send and recv buffers
    if (pBufferedSocket->pSendBuf != NULL)
    {
        DirtyMemFree(pBufferedSocket->pSendBuf, BUFFEREDSOCKET_MEMID, pBufferedSocket->iMemGroup, pBufferedSocket->pMemGroupUserData);
    }
    if (pBufferedSocket->pRecvBuf != NULL)
    {
        DirtyMemFree(pBufferedSocket->pRecvBuf, BUFFEREDSOCKET_MEMID, pBufferedSocket->iMemGroup, pBufferedSocket->pMemGroupUserData);
    }

    // release send mutex
    NetCritKill(&pBufferedSocket->SendCrit);

    // release module state
    DirtyMemFree(pBufferedSocket, BUFFEREDSOCKET_MEMID, pBufferedSocket->iMemGroup, pBufferedSocket->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function BufferedSocketConnect

    \Description
        Initiate a connection attempt to a remote host

    \Input *pBufferedSocket - module state
    \Input *pName           - address to connect to
    \Input iNameLen         - address length

    \Output
        int32_t             - result from SocketConnect()

    \Version 04/23/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t BufferedSocketConnect(BufferedSocketT *pBufferedSocket, struct sockaddr *pName, int32_t iNameLen)
{
    return(SocketConnect(pBufferedSocket->pSocket, pName, iNameLen));
}

/*F********************************************************************************/
/*!
    \Function BufferedSocketSend

    \Description
        Send data to a remote host

    \Input *pBufferedSocket - module state
    \Input *pBuffer         - data to send
    \Input iBufLen          - length of data to send

    \Output
        int32_t             - negative=failure, else success

    \Version 04/23/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t BufferedSocketSend(BufferedSocketT *pBufferedSocket, const char *pBuffer, int32_t iBufLen)
{
    int32_t iResult = 0;

    // validate input buffer length
    if (iBufLen <= 0)
    {
        DirtyCastLogPrintf("bufferedsocket: send buffer length of %d is not valid\n", iBufLen);
        return(0);
    }

    #if BUFFEREDSOCKET_SEND
    // not enough room to buffer data?
    if ((pBufferedSocket->iSendBufLen - pBufferedSocket->iSendBufPos) < iBufLen)
    {
        int32_t iBufLeft;
        // flush the buffer
        if ((iResult = BufferedSocketFlush(pBufferedSocket)) < 0)
        {
            DirtyCastLogPrintf("bufferedsocket: flush failed (err=%d)\n", iResult);
            return(iResult);
        }
        // if still not enough room after flush, realloc buffer bigger
        if ((iBufLeft = (pBufferedSocket->iSendBufLen - pBufferedSocket->iSendBufPos)) < iBufLen)
        {
            int32_t iBufAdd, iBufNeed;
            char *pNewSendBuf;

            // calculate how much space to add in 4k increments
            for (iBufAdd = 4096, iBufNeed = iBufLen-iBufLeft; iBufAdd < iBufNeed; iBufAdd *= 2)
                ;

            // log send buffer increase
            DirtyCastLogPrintf("bufferedsocket: adding %d bytes to send buffer (%d total)\n", iBufAdd, pBufferedSocket->iSendBufLen + iBufAdd);

            // alloc new buffer
            if ((pNewSendBuf = (char *)DirtyMemAlloc(pBufferedSocket->iSendBufLen + iBufAdd, BUFFEREDSOCKET_MEMID, pBufferedSocket->iMemGroup, pBufferedSocket->pMemGroupUserData)) == NULL)
            {
                DirtyCastLogPrintf("bufferedsocket: could not realloc send buffer\n");
                return(-1);
            }

            // copy old buffer data into new buffer, release old buffer, and update buffer size
            NetCritEnter(&pBufferedSocket->SendCrit);
            ds_memcpy(pNewSendBuf, pBufferedSocket->pSendBuf, pBufferedSocket->iSendBufPos);
            DirtyMemFree(pBufferedSocket->pSendBuf, BUFFEREDSOCKET_MEMID, pBufferedSocket->iMemGroup, pBufferedSocket->pMemGroupUserData);
            pBufferedSocket->pSendBuf = pNewSendBuf;
            pBufferedSocket->iSendBufLen = pBufferedSocket->iSendBufLen + iBufAdd;
            NetCritLeave(&pBufferedSocket->SendCrit);
        }
    }

    // append data to buffer
    if (iBufLen > 0)
    {
        NetCritEnter(&pBufferedSocket->SendCrit);
        ds_memcpy(pBufferedSocket->pSendBuf + pBufferedSocket->iSendBufPos, pBuffer, iBufLen);
        pBufferedSocket->iSendBufPos += iBufLen;
        pBufferedSocket->BufferedSocketInfo.uNumWrit += 1;
        if (pBufferedSocket->uLastWrite == 0)
        {
            pBufferedSocket->uLastWrite = NetTick();
        }
        if (pBufferedSocket->BufferedSocketInfo.uMaxSBuf < (unsigned)pBufferedSocket->iSendBufPos)
        {
            pBufferedSocket->BufferedSocketInfo.uMaxSBuf = (unsigned)pBufferedSocket->iSendBufPos;
        }
        iResult = iBufLen;
        NetCritLeave(&pBufferedSocket->SendCrit);
    }
    #else
    iResult = SocketSend(pBufferedSocket->pSocket, pBuffer, iBufLen, 0);
    #endif
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function BufferedSocketRecv

    \Description
        Read from a buffered socket

    \Input *pBufferedSocket - module state
    \Input *pBuffer         - buffer to receive into
    \Input iBufLen          - length of buffer

    \Output
        int32_t             - negative=failure, else success

    \Version 04/23/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t BufferedSocketRecv(BufferedSocketT *pBufferedSocket, char *pBuffer, int32_t iBufLen)
{
    int32_t iResult = 0;

    // validate input buffer length
    if (iBufLen <= 0)
    {
        DirtyCastLogPrintf("bufferedsocket: recv buffer length of %d is not valid\n", iBufLen);
        return(0);
    }

    #if BUFFEREDSOCKET_RECV
    // if we have less data than requested
    if (pBufferedSocket->iRecvBufPos < iBufLen)
    {
        // don't try to read if poll hint says we have no data
        if (SocketInfo(pBufferedSocket->pSocket, 'read', 0, NULL, 0) == 0)
        {
            return(0);
        }
        // grab as much data as we can
        if ((iResult = SocketRecv(pBufferedSocket->pSocket, pBufferedSocket->pRecvBuf + pBufferedSocket->iRecvBufPos, pBufferedSocket->iRecvBufLen - pBufferedSocket->iRecvBufPos, 0)) <= 0)
        {
            if (iResult < 0)
            {
                DirtyCastLogPrintf("bufferedsocket: SocketRecv() failed (err=%d/%d)\n", iResult, DirtyCastGetLastError());
            }
            return(iResult);
        }
        pBufferedSocket->iRecvBufPos += iResult;
        pBufferedSocket->BufferedSocketInfo.uDatRecv += iResult;
        pBufferedSocket->BufferedSocketInfo.uNumRecv += 1;
        if (pBufferedSocket->BufferedSocketInfo.uMaxRBuf < (unsigned)pBufferedSocket->iRecvBufPos)
        {
            pBufferedSocket->BufferedSocketInfo.uMaxRBuf = (unsigned)pBufferedSocket->iRecvBufPos;
        }
    }
    // copy up to requested amount
    if (pBufferedSocket->iRecvBufPos > 0)
    {
        if (iBufLen > pBufferedSocket->iRecvBufPos)
        {
            iBufLen = pBufferedSocket->iRecvBufPos;
        }
        ds_memcpy(pBuffer, pBufferedSocket->pRecvBuf, iBufLen);
        if (iBufLen < pBufferedSocket->iRecvBufPos)
        {
            memmove(pBufferedSocket->pRecvBuf, pBufferedSocket->pRecvBuf + iBufLen, pBufferedSocket->iRecvBufPos - iBufLen);
        }
        pBufferedSocket->iRecvBufPos -= iBufLen;
        pBufferedSocket->BufferedSocketInfo.uNumRead += 1;
        iResult = iBufLen;
    }
    #else
    iResult = SocketRecv(pBufferedSocket->pSocket, pBuffer, iBufLen, 0);
    #endif
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function BufferedSocketControl

    \Description
        Send a control message to the dirtysock layer

    \Input *pBufferedSocket - module state
    \Input iOption          - control option
    \Input iData1           - control data
    \Input *pData2          - control data
    \Input *pData3          - control data

    \Output
        int32_t             - control specific

    \Notes
        Controls are:

        \verbatim
            CONTROL         DESCRIPTION
            'rcvq'          Updates last sampled recv buf size for the socket.
            'rset'          Resets the send and recv buffers to their default sizes.
            'sndq'          Updates last sampled send buf size for the socket.
        \endverbatim

    \Version 04/23/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t BufferedSocketControl(BufferedSocketT *pBufferedSocket, int32_t iOption, int32_t iData1, void *pData2, void *pData3)
{
    if (iOption == 'rcvq')
    {
        pBufferedSocket->BufferedSocketInfo.uRcvQueue = iData1;
        // update high water
        pBufferedSocket->BufferedSocketInfo.uMaxRQueue = DS_MAX(pBufferedSocket->BufferedSocketInfo.uMaxRQueue, pBufferedSocket->BufferedSocketInfo.uRcvQueue);
        return(0);
    }
    if (iOption == 'rset')
    {
        char *pNewBuf;

        // if the remaining space in the buffer is greater than our default then shrink down
        if ((uint32_t)(pBufferedSocket->iSendBufLen - pBufferedSocket->iSendBufPos) > pBufferedSocket->BufferedSocketInfo.uBufSize)
        {
            if ((pNewBuf = (char *)DirtyMemAlloc(pBufferedSocket->BufferedSocketInfo.uBufSize, BUFFEREDSOCKET_MEMID, pBufferedSocket->iMemGroup, pBufferedSocket->pMemGroupUserData)) == NULL)
            {
                DirtyCastLogPrintf("bufferedsocket: unable to realloc send buffer\n");
                return(-1);
            }
            NetCritEnter(&pBufferedSocket->SendCrit);
            ds_memcpy(pNewBuf, pBufferedSocket->pSendBuf, pBufferedSocket->iSendBufPos);
            DirtyMemFree(pBufferedSocket->pSendBuf, BUFFEREDSOCKET_MEMID, pBufferedSocket->iMemGroup, pBufferedSocket->pMemGroupUserData);
            pBufferedSocket->pSendBuf = pNewBuf;
            pBufferedSocket->iSendBufLen = pBufferedSocket->BufferedSocketInfo.uBufSize;
            NetCritLeave(&pBufferedSocket->SendCrit);
        }
        if ((uint32_t)(pBufferedSocket->iRecvBufLen - pBufferedSocket->iRecvBufPos) > pBufferedSocket->BufferedSocketInfo.uBufSize)
        {
            if ((pNewBuf = (char *)DirtyMemAlloc(pBufferedSocket->BufferedSocketInfo.uBufSize, BUFFEREDSOCKET_MEMID, pBufferedSocket->iMemGroup, pBufferedSocket->pMemGroupUserData)) == NULL)
            {
                DirtyCastLogPrintf("bufferedsocket: unable to realloc recv buffer\n");
                return(-1);
            }
            NetCritEnter(&pBufferedSocket->SendCrit);
            ds_memcpy(pNewBuf, pBufferedSocket->pRecvBuf, pBufferedSocket->iRecvBufPos);
            DirtyMemFree(pBufferedSocket->pRecvBuf, BUFFEREDSOCKET_MEMID, pBufferedSocket->iMemGroup, pBufferedSocket->pMemGroupUserData);
            pBufferedSocket->pRecvBuf = pNewBuf;
            pBufferedSocket->iRecvBufLen = pBufferedSocket->BufferedSocketInfo.uBufSize;
            NetCritLeave(&pBufferedSocket->SendCrit);
        }
        return(0);
    }
    if (iOption == 'sndq')
    {
        pBufferedSocket->BufferedSocketInfo.uSndQueue = iData1;
        // update our high water
        pBufferedSocket->BufferedSocketInfo.uMaxSQueue = DS_MAX(pBufferedSocket->BufferedSocketInfo.uMaxSQueue, pBufferedSocket->BufferedSocketInfo.uSndQueue);
        return(0);
    }
    return(SocketControl(pBufferedSocket->pSocket, iOption, iData1, pData2, pData3));
}

/*F********************************************************************************/
/*!
    \Function BufferedSocketInfo

    \Description
        Return information about an existing socket

    \Input *pBufferedSocket - module state
    \Input iInfo            - info selector
    \Input iData            - selector data
    \Input *pBuf            - selector data buffer
    \Input iLen             - length of buffer

    Info selectors are:

        \verbatim
            INFO            DESCRIPTION
            'bnfo'          Resets the send and recv buffers to their default sizes.
            'dstp'          Returns destination port of TCP connection associated with buffered socket
            'srcp'          Returns source port of TCP connection associated with buffered socket
            'stat'          Returns socket status
        \endverbatim

    \Output
        int32_t             - SocketInfo() result

    \Version 04/23/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t BufferedSocketInfo(BufferedSocketT *pBufferedSocket, int32_t iInfo, int32_t iData, void *pBuf, int32_t iLen)
{
    if ((iInfo == 'bnfo') && (pBuf != NULL) && (iLen == sizeof(pBufferedSocket->BufferedSocketInfo)))
    {
        ds_memcpy(pBuf, &pBufferedSocket->BufferedSocketInfo, iLen);
        return(0);
    }
    if (iInfo == 'dstp')
    {
        return(pBufferedSocket->uDstPort);
    }
    if (iInfo == 'srcp')
    {
        return(pBufferedSocket->uSrcPort);
    }
    if (iInfo == 'stat')
    {
        int32_t iResult = SocketInfo(pBufferedSocket->pSocket, iInfo, iData, pBuf, iLen);

        // upon detection of 'connected state', update connection src port and dest port 
        if (iResult > 0)
        {
            struct sockaddr SrcAddr, DestAddr;
            SocketInfo(pBufferedSocket->pSocket, 'bind', 0, &SrcAddr, sizeof(SrcAddr));
            pBufferedSocket->uSrcPort = SockaddrInGetPort(&SrcAddr);
            SocketInfo(pBufferedSocket->pSocket, 'peer', 0, &DestAddr, sizeof(DestAddr));
            pBufferedSocket->uDstPort = SockaddrInGetPort(&DestAddr);
        }
        return(iResult);
    }
    return(SocketInfo(pBufferedSocket->pSocket, iInfo, iData, pBuf, iLen));
}

/*F********************************************************************************/
/*!
    \Function BufferedSocketFlush

    \Description
        Flush any pending writes on the given buffered socket.

    \Input *pBufferedSocket - module state

    \Output
        int32_t             - negative=failure, else success

    \Version 04/23/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t BufferedSocketFlush(BufferedSocketT *pBufferedSocket)
{
    int32_t iResult = 0;

    #if BUFFEREDSOCKET_SEND
    if (pBufferedSocket->iSendBufPos > 0)
    {
        if ((iResult = SocketSend(pBufferedSocket->pSocket, pBufferedSocket->pSendBuf, pBufferedSocket->iSendBufPos, 0)) > 0)
        {
            pBufferedSocket->BufferedSocketInfo.uWriteDelay += NetTick() - pBufferedSocket->uLastWrite;
            pBufferedSocket->uLastWrite = 0;

            NetCritEnter(&pBufferedSocket->SendCrit);

            if (iResult < pBufferedSocket->iSendBufPos)
            {
                memmove(pBufferedSocket->pSendBuf, pBufferedSocket->pSendBuf + iResult, pBufferedSocket->iSendBufPos - iResult);
            }

            pBufferedSocket->iSendBufPos -= iResult;
            pBufferedSocket->BufferedSocketInfo.uDatSent += iResult;
            pBufferedSocket->BufferedSocketInfo.uNumSent += 1;

            NetCritLeave(&pBufferedSocket->SendCrit);
        }
        else
        {
            DirtyCastLogPrintf("bufferedsocket: SocketSend() failed (err=%d/%d)\n", iResult, DirtyCastGetLastError());
        }
    }
    #endif
    return(iResult);
}
