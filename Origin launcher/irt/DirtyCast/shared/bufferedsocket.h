/*H********************************************************************************/
/*!
    \File bufferedsocket.h

    \Description
        Buffered socket read/write routiines, aimed at reducing the number of
        send() and recv() calls.

    \Copyright
        Copyright (c) 2007 Electronic Arts Inc.

    \Version 04/23/2007 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _bufferedsocket_h
#define _bufferedsocket_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! buffered socket stat info
typedef struct BufferedSocketInfoT
{
    uint32_t uBufSize;  //!< buffered socket buffer size

    uint32_t uNumRead;  //!< number of reads against buffered socket
    uint32_t uNumRecv;  //!< number of recv calls made by buffered socket
    uint32_t uDatRecv;  //!< amount of data received by buffered socket
    uint32_t uMaxRBuf;  //!< high-water data in receive buffer
    uint32_t uRcvQueue; //!< amount of data in the TCP recv queue
    uint32_t uMaxRQueue;//!< high-water data in socket receive queue

    uint32_t uNumWrit;  //!< number of writes against buffered socket
    uint32_t uNumSent;  //!< number of send calls made by buffered socket
    uint32_t uDatSent;  //!< amount of data sent by buffered socket
    uint32_t uMaxSBuf;  //!< high-water data in send buffer
    uint32_t uSndQueue; //!< amount of data in the TCP send queue
    uint32_t uMaxSQueue;//!< high-water data in socket send queue

    uint32_t uWriteDelay; //!< accumulated delay between write/flush
} BufferedSocketInfoT;

//! opaque module state
typedef struct BufferedSocketT BufferedSocketT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// open a buffered socket
BufferedSocketT *BufferedSocketOpen(int32_t iType, uint16_t uPort, int32_t iBufLen, SocketT *pSocket);

// close a buffered socket
void BufferedSocketClose(BufferedSocketT *pBufferedSocket);

// initiate a connection attempt to a remote host
int32_t BufferedSocketConnect(BufferedSocketT *pBufferedSocket, struct sockaddr *pName, int32_t iNameLen);

// send data to a remote host
int32_t BufferedSocketSend(BufferedSocketT *pBufferedSocket, const char *pBuffer, int32_t iBufLen);

// read from a buffered socket
int32_t BufferedSocketRecv(BufferedSocketT *pBufferedSocket, char *pBuffer, int32_t iBufLen);

// send a control message to the dirtysock layer
int32_t BufferedSocketControl(BufferedSocketT *pBufferedSocket, int32_t iOption, int32_t iData1, void *pData2, void *pData3);

// return information about an existing socket
int32_t BufferedSocketInfo(BufferedSocketT *pBufferedSocket, int32_t iInfo, int32_t iData, void *pBuf, int32_t iLen);

// flush a buffered socket
int32_t BufferedSocketFlush(BufferedSocketT *pBufferedSocket);

#ifdef __cplusplus
}
#endif

#endif // _bufferedsocket_h

