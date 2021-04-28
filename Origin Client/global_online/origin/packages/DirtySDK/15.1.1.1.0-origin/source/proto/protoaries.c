/*H********************************************************************************/
/*!
    \File protoaries.c

    \Description
        This module supports the Aries transfer protocol (a simple length/type
        prefixed TCP message type). It is a cross platform component and is used
        on all platforms. ProtoAries is used for communication with the Lobby and
        Buddy servers.

    \Copyright
        Copyright (c) Electronic Arts 2000-2005.  ALL RIGHTS RESERVED.

    \Version 02/28/1999 (gschaefer) First Version
    \Version 03/18/2002 (gschaefer) Renamed from netsess and cleanup
    \Version 07/10/2003 (gschaefer) Added listen support
    \Version 02/02/2004 (sbevan)    Added SSL support
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/crypt/cryptstp1.h"
#include "DirtySDK/proto/protossl.h"
#include "DirtySDK/proto/protoaries.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! module state
struct ProtoAriesRefT
{
    ProtoSSLRefT *pProtoSsl;        //!< ssl ref
    
    // module memory group
    int32_t iMemGroup;              //!< module mem group id
    void *pMemGroupUserData;        //!< user data associated with mem group
    
    struct sockaddr_in DestAddr;    //!< server address/port
    uint32_t uLocalAddr;            //!< our local ip address
    uint32_t uLocalPort;            //!< our local ip port
    char strServerName[64];         //!< server to connect to

    ProtoAriesStateE eState;        //!< module state
    uint32_t uTimeout;              //!< timeout marker

    int32_t iBufSize;               //!< size of input and output buffers

    int32_t iOutSize;               //!< output packet size
    int32_t iOutProg;               //!< output packet progress (0..size)
    int32_t iOutMaxm;               //!< output packet maximum buffer
    uint8_t *pOutData;              //!< output packet data

    int32_t iInpSize;               //!< input packet size
    int32_t iInpProg;               //!< input packet progress (0..size)
    uint8_t *pInpData;              //!< input packet data
    uint8_t aInpHead[sizeof(ProtoAriesMsgHdrT)]; //!< packet header storage

    int16_t bUseSSL;                //!< use SSL for connection
    int16_t bHaveWallet;            //!< have a wallet available
    CryptStp1WalletT Wallet;        //!< wallet containing secret+ticket
    CryptStp1StreamT Stream;        //!< session stream encryption state
    
    char aBufferData[1];            //!< buffers start here
};

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

// Private variables

// Public variables

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _ProtoAriesBuff

    \Description
        Allocate new output buffer

    \Input *pProtoAries - reference pointer
    \Input iSize        - bytes to allocate

    \Output
        int32_t         - <0=error, else number of bytes available

    \Version 11/01/2002 (gschaefer)
*/
/********************************************************************************F*/
static int32_t _ProtoAriesBuff(ProtoAriesRefT *pProtoAries, int32_t iSize)
{
    ProtoAriesMsgHdrT *pHdr;
    int32_t iDone, iStep;

    // if the buffer is empty
    if (pProtoAries->iOutSize == 0)
    {
        pProtoAries->iOutProg = 0;
        return(pProtoAries->iOutMaxm-pProtoAries->iOutSize);
    }

    // see how many at start of buffer
    for (iDone = iStep = 0; iDone < pProtoAries->iOutProg; iDone += iStep)
    {
        pHdr = (ProtoAriesMsgHdrT *) (pProtoAries->pOutData+iDone);
        iStep = (pHdr->size[0]<<24)|(pHdr->size[1]<<16)|(pHdr->size[2]<<8)|(pHdr->size[3]<<0);
    }
    if (iDone > pProtoAries->iOutProg)
    {
        iDone -= iStep;
    }

    // shift back data within existing buffer
    if (iDone > 0)
    {
        ds_memcpy(pProtoAries->pOutData, pProtoAries->pOutData+iDone, pProtoAries->iOutSize-iDone);
        pProtoAries->iOutSize -= iDone;
        pProtoAries->iOutProg -= iDone;
    }

    // error if it doesn't fit
    if (iSize > pProtoAries->iOutMaxm-pProtoAries->iOutSize)
    {
        NetPrintf(("protoaries: output buffer overflow\n"));
        return(-1);
    }

    // return number of bytes available
    return(pProtoAries->iOutMaxm-pProtoAries->iOutSize);
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ProtoAriesCreate

    \Description
        Create new virtual network session

    \Input iMaxBuf          - size of internal buffer to use

    \Output
        ProtoAriesRefT *    - reference pointer (NULL=failed)

    \Version 11/01/2002 (gschaefer)
*/
/********************************************************************************F*/
ProtoAriesRefT *ProtoAriesCreate(int32_t iMaxBuf)
{
    ProtoAriesRefT *pProtoAries;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pProtoAries = DirtyMemAlloc(sizeof(*pProtoAries)+(iMaxBuf*2), PROTOARIES_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protoaries: could not allocate module state\n"));
        return(NULL);
    }
    memset(pProtoAries, 0, sizeof(*pProtoAries));
    pProtoAries->iMemGroup = iMemGroup;
    pProtoAries->pMemGroupUserData = pMemGroupUserData;

    // set up buffers
    pProtoAries->iBufSize = iMaxBuf;
    pProtoAries->pInpData = (unsigned char *)pProtoAries->aBufferData;
    pProtoAries->pOutData = (unsigned char *)pProtoAries->aBufferData + iMaxBuf;
    pProtoAries->iOutMaxm = iMaxBuf;

    // init the socket
    pProtoAries->pProtoSsl = NULL;
    pProtoAries->eState = PROTOARIES_STATE_IDLE;
    pProtoAries->bHaveWallet = FALSE;

    // return the reference
    return(pProtoAries);
}

/*F********************************************************************************/
/*!
    \Function ProtoAriesDestroy

    \Description
        Destroy existing session

    \Input *pProtoAries     - reference pointer

    \Output
        None.

    \Version 11/01/2002 (gschaefer)
*/
/********************************************************************************F*/
void ProtoAriesDestroy(ProtoAriesRefT *pProtoAries)
{
    // close the socket
    if (pProtoAries->pProtoSsl != NULL)
    {
        ProtoSSLDestroy(pProtoAries->pProtoSsl);
    }
    // release ref
    DirtyMemFree(pProtoAries, PROTOARIES_MEMID, pProtoAries->iMemGroup, pProtoAries->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function ProtoAriesStatus

    \Description
        Return session status info

    \Input *pProtoAries - reference pointer
    \Input iSelect      - status selector
    \Input *pBuf        - output buffer
    \Input iBufLen      - output buffer length

    \Output
        int32_t         - selector specific

    \Notes
        iSelect can be one of the following:
    
        \verbatim
            'addr' - server ip address
            'cryp' - session crypt status
            'ibuf' - number of bytes queued in the input buffer
            'ladr' - local ip address
            'lptr' - local port number
            'obuf' - number of bytes queued in the output buffer
            'port' - server port number
            'secu' - security status (ProtoAriesSecureE)
            'sock' - aries socket ref
            'stat' - module state
        \endverbatim

    \Version 11/01/2002 (gschaefer)
*/
/********************************************************************************F*/
int32_t ProtoAriesStatus(ProtoAriesRefT *pProtoAries, int32_t iSelect, void *pBuf, int32_t iBufLen)
{
    // return the connect address
    if (iSelect == 'addr')
    {
        return(SocketNtohl(pProtoAries->DestAddr.sin_addr.s_addr));
    }
    // return session crypt status
    if (iSelect == 'cryp')
    {
        return(pProtoAries->Stream.bEncrypt);
    }
    // return input buffer size
    if (iSelect == 'ibuf')
    {
        return(pProtoAries->pInpData ? pProtoAries->iInpSize-pProtoAries->iInpProg : 0);
    }
    // return the local address
    if (iSelect == 'ladr')
    {
        return(pProtoAries->uLocalAddr);
    }
    // return the local port
    if (iSelect == 'lprt')
    {
        return(pProtoAries->uLocalPort);
    }
    // return output buffer size
    if (iSelect == 'obuf')
    {
        return(pProtoAries->iOutSize-pProtoAries->iOutProg);
    }
    // return the connect port
    if (iSelect == 'port')
    {
        return(SocketNtohs(pProtoAries->DestAddr.sin_port));
    }
    // return SSL status
    if (iSelect == 'secu')
    {
        return(pProtoAries->bUseSSL);
    }
    // return the module state
    if (iSelect == 'stat')
    {
        return(pProtoAries->eState);
    }
    // unrecognized selector -- let ProtoSSL have a crack at it
    return((pProtoAries->pProtoSsl != NULL) ? ProtoSSLStat(pProtoAries->pProtoSsl, iSelect, pBuf, iBufLen) : -1);
}

/*F********************************************************************************/
/*!
    \Function ProtoAriesConnect

    \Description
        Initiate an Aries protocol connection to a server.  This is a non blocking
        call and sessions completion is detected by ProtoAriesStatus('stat') or by
        "receiving" a special packet where code/kind are both 0xffffffff.

    \Input *pProtoAries - reference pointer
    \Input *pServer     - server name
    \Input uAddr        - server address
    \Input iPort        - server port

    \Output
        int32_t         - -1=error, zero=no error

    \Version  11/01/2002 (gschaefer)
*/
/********************************************************************************F*/
int32_t ProtoAriesConnect(ProtoAriesRefT *pProtoAries, const char *pServer, uint32_t uAddr, int32_t iPort)
{
    // see if socket in use
    if (pProtoAries->pProtoSsl != NULL)
    {
        return(-1);
    }

    // clear input buffer
    pProtoAries->iInpProg = pProtoAries->iInpSize = 0;

    // setup dest address
    memset(&pProtoAries->DestAddr, 0, sizeof(pProtoAries->DestAddr));
    pProtoAries->DestAddr.sin_family = AF_INET;
    pProtoAries->DestAddr.sin_addr.s_addr = SocketHtonl(uAddr);
    pProtoAries->DestAddr.sin_port = SocketHtons((uint16_t)iPort);
    if (pServer != NULL)
    {
        ds_strnzcpy(pProtoAries->strServerName, pServer, sizeof(pProtoAries->strServerName));
    }
    else
    {
        memset(pProtoAries->strServerName, 0, sizeof pProtoAries->strServerName);
    }
    // start the connect
    pProtoAries->eState = PROTOARIES_STATE_CONN;
    pProtoAries->uTimeout = 0;

    // if we have a wallet, send ticket to server
    if (pProtoAries->bHaveWallet)
    {
        CryptStp1SecretT Secret;
        CryptStp1TicketT Ticket;
    
        // extract private secret and public ticket
        if (CryptStp1OpenWallet(&pProtoAries->Wallet, &Secret, &Ticket) > 0)
        {
            // send ticket unencrypted
            ProtoAriesSend(pProtoAries, '?tic', 0, (char *)&Ticket, sizeof(Ticket));
            // enable stream encryption
            CryptStp1UseSecret(&pProtoAries->Stream, &Secret);
        }
        else
        {
            /* Could not open the wallet.  It must either be malformed
               or there is some kind of version mismatch.  Either way,
               ignore the wallet from now on. */
            pProtoAries->bHaveWallet = 0;
        }
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function ProtoAriesUnconnect

    \Description
        Terminate Aries connection to server.

    \Input *pProtoAries - reference pointer

    \Output
        int32_t         - zero=success, negative=error

    \Version 11/01/2002 (gschaefer)
*/
/********************************************************************************F*/
int32_t ProtoAriesUnconnect(ProtoAriesRefT *pProtoAries)
{
    // close if open
    if (pProtoAries->pProtoSsl != NULL)
    {
        ProtoSSLDestroy(pProtoAries->pProtoSsl);
        pProtoAries->pProtoSsl = NULL;
    }
    // clear output buffer
    pProtoAries->iOutProg = pProtoAries->iOutSize = 0;
    // set to disconnected state
    pProtoAries->eState = PROTOARIES_STATE_DISC;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function ProtoAriesListen

    \Description
        Listen from an incoming Aries connection. This is a non blocking call
        and sessions completion is detected by ProtoAriesStatus('stat') or by
        "receiving" a special packet where code/kind are both 0xffffffff.

    \Input *pProtoAries - reference pointer
    \Input uAddr        - ip address
    \Input iPort        - tcp port

    \Output
        int32_t         - -1=error, zero=no error

    \Version 11/01/2002 (gschaefer)
*/
/********************************************************************************F*/
int32_t ProtoAriesListen(ProtoAriesRefT *pProtoAries, uint32_t uAddr, int32_t iPort)
{
    // see if socket in use
    if (pProtoAries->pProtoSsl != 0)
    {
        return(-1);
    }

    // clear input buffer
    pProtoAries->iInpProg = pProtoAries->iInpSize = 0;

    // setup dest uAddress
    memset(&pProtoAries->DestAddr, 0, sizeof(pProtoAries->DestAddr));
    pProtoAries->DestAddr.sin_family = AF_INET;
    pProtoAries->DestAddr.sin_addr.s_addr = SocketHtonl(uAddr);
    pProtoAries->DestAddr.sin_port = SocketHtons((uint16_t)iPort);

    DirtyMemGroupEnter(pProtoAries->iMemGroup, pProtoAries->pMemGroupUserData);
    pProtoAries->pProtoSsl = ProtoSSLCreate();
    DirtyMemGroupLeave();
    if (pProtoAries->pProtoSsl == NULL)
    {
        return(-1);
    }
    if (ProtoSSLBind(pProtoAries->pProtoSsl, (const struct sockaddr *)&pProtoAries->DestAddr, sizeof pProtoAries->DestAddr) < 0)
    {
        return(-1);
    }
    if (ProtoSSLListen(pProtoAries->pProtoSsl, 2) < 0)
    {
        return(-1);
    }
    // start the connect
    pProtoAries->eState = PROTOARIES_STATE_LIST;
    pProtoAries->uTimeout = 0;
    pProtoAries->bHaveWallet = 0;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function ProtoAriesUnlisten

    \Description
        Terminate Aries server listening.

    \Input *pProtoAries - reference pointer

    \Output
        int32_t         - zero=success, negative=error

    \Version 11/01/2002 (gschaefer)
*/
/********************************************************************************F*/
int32_t ProtoAriesUnlisten(ProtoAriesRefT *pProtoAries)
{
    // close if open
    if (pProtoAries->pProtoSsl != NULL)
    {
        ProtoSSLDestroy(pProtoAries->pProtoSsl);
        pProtoAries->pProtoSsl = NULL;
    }
    // clear output buffer
    pProtoAries->iOutProg = pProtoAries->iOutSize = 0;
    pProtoAries->eState = PROTOARIES_STATE_DISC;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function ProtoAriesSecure

    \Description
        Define whether the Aries connection should be secure or insecure.

    \Input *pProtoAries - reference pointer
    \Input eSecure      - the type of security the connection should use.

    \Output
        None.

    \Notes
        If this is not called then the default is to use PROTOARIES_SECURE_NONE.

    \Version 03/03/2004 (sbevan)
*/
/********************************************************************************F*/
void ProtoAriesSecure(ProtoAriesRefT *pProtoAries, ProtoAriesSecureE eSecure)
{
    pProtoAries->bUseSSL = (eSecure == PROTOARIES_SECURE_SSL);
}

/*F********************************************************************************/
/*!
    \Function ProtoAriesSetKey

    \Description
        Initialize the session key for this connection.

    \Input *pProtoAries - Aries object
    \Input *pKey        - the session key information
    \Input uKeySize     - the size of the key in bytes

    \Output
        None.

    \Notes
        A NULL pKey or uKeySize of 0 mean don't use any encryption.

    \Version 03/03/2004 (sbevan)
    \Version 05/05/2004 (sbevan) changed to take key&length
*/
/********************************************************************************F*/
void ProtoAriesSetKey(ProtoAriesRefT *pProtoAries, const void *pKey, uint32_t uKeySize)
{
    const CryptStp1WalletT *pWallet = (const CryptStp1WalletT *)pKey;

    if ((pKey == NULL) || (uKeySize == 0) || (uKeySize != sizeof(*pWallet)))
    {
        pProtoAries->bHaveWallet = FALSE;
        CryptStp1UseSecret(&pProtoAries->Stream, 0);
    }
    else
    {
        pProtoAries->bHaveWallet = TRUE;
        ds_memcpy_s(&pProtoAries->Wallet, sizeof(pProtoAries->Wallet), pWallet, sizeof *pWallet);
    }
}

/*F********************************************************************************/
/*!
    \Function ProtoAriesUpdate

    \Description
        Perform actual "work" for protocol. This function should be called at least
        every 100ms to allow packets to be processed at a reasonable speed.

    \Input *pProtoAries - reference pointer

    \Output
        None.

    \Version 11/01/2002 (gschaefer)
*/
/********************************************************************************F*/
void ProtoAriesUpdate(ProtoAriesRefT *pProtoAries)
{
    ProtoAriesMsgHdrT *pHdr;
    struct sockaddr_in SockAddr;
    int32_t iLen;

    // handle disconnect
    if ((pProtoAries->eState == PROTOARIES_STATE_ONLN) && (pProtoAries->pProtoSsl == NULL))
    {
        // allocate connect packet
        pProtoAries->iInpProg = pProtoAries->iInpSize = sizeof(*pHdr);

        // populate disconnect packet
        pHdr = (ProtoAriesMsgHdrT *)pProtoAries->pInpData;
        pHdr->kind[0] = pHdr->kind[1] = pHdr->kind[2] = pHdr->kind[3] = 0xff;
        pHdr->code[0] = pHdr->code[1] = pHdr->code[2] = pHdr->code[3] = 0xfe;
        pHdr->size[0] = pHdr->size[1] = pHdr->size[2] = 0;
        pHdr->size[3] = sizeof(*pHdr);
        
        // mark as disconnected
        pProtoAries->eState = PROTOARIES_STATE_DISC;
    }

    // attempt auto reconnect
    if ((pProtoAries->eState == PROTOARIES_STATE_DISC) && (pProtoAries->iOutSize > 0))
    {
        pProtoAries->eState = PROTOARIES_STATE_CONN;
        pProtoAries->uTimeout = 0;
    }

    // wait for incoming connection
    if (pProtoAries->eState == PROTOARIES_STATE_LIST)
    {
        ProtoSSLRefT *pSsl;
        iLen = sizeof(SockAddr);

        // check for incoming connection
        pSsl = ProtoSSLAccept(pProtoAries->pProtoSsl, 0, (struct sockaddr *)&SockAddr, &iLen);
        if (pSsl == NULL)
        {
            return;
        }

        // setup the connection
        ProtoSSLDestroy(pProtoAries->pProtoSsl);
        pProtoAries->pProtoSsl = pSsl;

        // mark as online
        pProtoAries->eState = PROTOARIES_STATE_ONLN;
        // restart any previously pending send (in case it broke)
        pProtoAries->iOutProg = 0;

        // allocate connect packet
        pProtoAries->iInpProg = pProtoAries->iInpSize = sizeof(*pHdr);

        // populate connect packet
        pHdr = (ProtoAriesMsgHdrT *)pProtoAries->pInpData;
        pHdr->kind[0] = pHdr->kind[1] = pHdr->kind[2] = pHdr->kind[3] = 0xff;
        pHdr->code[0] = pHdr->code[1] = pHdr->code[2] = pHdr->code[3] = 0xff;
        pHdr->size[0] = pHdr->size[1] = pHdr->size[2] = 0;
        pHdr->size[3] = sizeof(*pHdr);
    }

    // deal with connection
    if (pProtoAries->eState == PROTOARIES_STATE_CONN)
    {
        int32_t iStat;
        
        // auto reconnect if timeout
        if (NetTick() > pProtoAries->uTimeout)
        {
            if (pProtoAries->pProtoSsl != NULL)
            {
                ProtoSSLDestroy(pProtoAries->pProtoSsl);
            }
            DirtyMemGroupEnter(pProtoAries->iMemGroup, pProtoAries->pMemGroupUserData);
            pProtoAries->pProtoSsl = ProtoSSLCreate();
            DirtyMemGroupLeave();
            if (pProtoAries->pProtoSsl == NULL)
            {
                return;
            }
            if (ProtoSSLConnect(pProtoAries->pProtoSsl, pProtoAries->bUseSSL != 0, pProtoAries->strServerName,
                SocketNtohl(pProtoAries->DestAddr.sin_addr.s_addr), SocketNtohs(pProtoAries->DestAddr.sin_port)) < 0)
            {
                return;
            }
            // reset the timer
            pProtoAries->uTimeout = NetTick()+30*1000;
            // default to encryption off
            CryptStp1UseSecret(&pProtoAries->Stream, NULL);
        }

        if (pProtoAries->pProtoSsl == NULL)
        {
            return;
        }

        ProtoSSLUpdate(pProtoAries->pProtoSsl);
        if ((iStat = ProtoSSLStat(pProtoAries->pProtoSsl, 'stat', NULL, 0)) < 0)
        {
            // disconnect!
            NetPrintf(("protoaries: socket disconnection during connect\n"));
            ProtoAriesUnconnect(pProtoAries);
            return;
        }
        else if (iStat == 0)
        {
            return;
        }
        if (pProtoAries->DestAddr.sin_addr.s_addr == 0)
        {
            ProtoSSLStat(pProtoAries->pProtoSsl, 'peer', &pProtoAries->DestAddr, sizeof(pProtoAries->DestAddr));
        }
        // get local ip address
        if (ProtoSSLStat(pProtoAries->pProtoSsl, 'bind', &SockAddr, sizeof(SockAddr)) == 0)
        {
            pProtoAries->uLocalAddr = SocketNtohl(SockAddr.sin_addr.s_addr);
            pProtoAries->uLocalPort = SocketNtohs(SockAddr.sin_port);
        }

        // mark as online
        pProtoAries->eState = PROTOARIES_STATE_ONLN;
        // restart any previously pending send (in case it broke)
        pProtoAries->iOutProg = 0;

        // allocate connect packet
        pProtoAries->iInpProg = pProtoAries->iInpSize = sizeof(*pHdr);

        // populate connect packet
        pHdr = (ProtoAriesMsgHdrT *)pProtoAries->pInpData;
        pHdr->kind[0] = pHdr->kind[1] = pHdr->kind[2] = pHdr->kind[3] = 0xff;
        pHdr->code[0] = pHdr->code[1] = pHdr->code[2] = pHdr->code[3] = 0xff;
        pHdr->size[0] = pHdr->size[1] = pHdr->size[2] = 0;
        pHdr->size[3] = sizeof(*pHdr);
    }

    // see if there is a socket ready
    if (pProtoAries->pProtoSsl == NULL)
    {
        return;
    }

    ProtoSSLUpdate(pProtoAries->pProtoSsl);

    // attempt to send data
    if ((pProtoAries->iOutSize > 0) && (pProtoAries->iOutProg != pProtoAries->iOutSize))
    {
        // do the send
        iLen = ProtoSSLSend(pProtoAries->pProtoSsl, (char *)pProtoAries->pOutData+pProtoAries->iOutProg, pProtoAries->iOutSize-pProtoAries->iOutProg);
        if (iLen < 0)
        {
            // close socket and force reconnect
            ProtoSSLDestroy(pProtoAries->pProtoSsl);
            pProtoAries->pProtoSsl = NULL;
            return;
        }
        // add to progress
        if (iLen > 0)
        {
            pProtoAries->iOutProg += iLen;
        }

        // flush packet once sent
        if (pProtoAries->iOutProg == pProtoAries->iOutSize)
        {
            pProtoAries->iOutProg = pProtoAries->iOutSize = 0;
        }
    }

    // attempt to receive new packet
    if (pProtoAries->iInpSize == 0)
    {
        // see if any data pending
        iLen = ProtoSSLRecv(pProtoAries->pProtoSsl, (char *)pProtoAries->aInpHead+pProtoAries->iInpProg, sizeof(ProtoAriesMsgHdrT)-pProtoAries->iInpProg);
        if (iLen < 0)
        {
            // close socket and force reconnect
            ProtoSSLDestroy(pProtoAries->pProtoSsl);
            pProtoAries->pProtoSsl = NULL;
            return;
        }
        // add to progress
        if (iLen > 0)
        {
            pProtoAries->iInpProg += iLen;
        }

        // see if header is complete
        if (pProtoAries->iInpProg == sizeof(ProtoAriesMsgHdrT))
        {
            // validate the header
            pHdr = (ProtoAriesMsgHdrT *) pProtoAries->aInpHead;
            CryptStp1DecryptData(&pProtoAries->Stream, (char *)pHdr, sizeof(*pHdr));
            pProtoAries->iInpSize = (pHdr->size[0]<<24)|(pHdr->size[1]<<16)|(pHdr->size[2]<<8)|(pHdr->size[3]<<0);
            if ((pProtoAries->iInpSize < (signed)sizeof(*pHdr)) || (pProtoAries->iInpSize > pProtoAries->iBufSize))
            {
                NetPrintf(("protoaries: recv error -- invalid header\n"));
                ProtoSSLDestroy(pProtoAries->pProtoSsl);
                pProtoAries->pProtoSsl = NULL;
                return;
            }

            // setup to receive rest of packet
            ds_memcpy(pProtoAries->pInpData, pProtoAries->aInpHead, pProtoAries->iInpProg);
            
            // if packet is complete, release any completed send packet
            if ((pProtoAries->iInpProg == pProtoAries->iInpSize) && (pProtoAries->iOutSize > 0) && (pProtoAries->iOutProg == pProtoAries->iOutSize))
            {
                pProtoAries->iOutSize = pProtoAries->iOutProg = 0;
            }
        }
    }

    // attempt to append to packet
    if ((pProtoAries->iInpSize > 0) && (pProtoAries->iInpProg < pProtoAries->iInpSize))
    {
        // see if any data pending
        iLen = ProtoSSLRecv(pProtoAries->pProtoSsl, (char *)pProtoAries->pInpData+pProtoAries->iInpProg, pProtoAries->iInpSize-pProtoAries->iInpProg);
        if (iLen < 0)
        {
            // close socket and force reconnect
            ProtoSSLDestroy(pProtoAries->pProtoSsl);
            pProtoAries->pProtoSsl = NULL;
            return;
        }
        // add to progress
        if (iLen > 0)
        {
            pProtoAries->iInpProg += iLen;
        }

        // if packet is complete, deal with decrypt
        if (pProtoAries->iInpProg == pProtoAries->iInpSize)
        {
            CryptStp1DecryptData(&pProtoAries->Stream, (char *)pProtoAries->pInpData+sizeof(ProtoAriesMsgHdrT), pProtoAries->iInpProg-sizeof(ProtoAriesMsgHdrT));
            if (CryptStp1DecryptHash(&pProtoAries->Stream, (char *)pProtoAries->pInpData, pProtoAries->iInpSize) < 0)
            {
                ProtoSSLDestroy(pProtoAries->pProtoSsl);
                pProtoAries->pProtoSsl = NULL;
                return;
            }
            pProtoAries->iInpSize = pProtoAries->iInpProg = CryptStp1DecryptSize(&pProtoAries->Stream, pProtoAries->iInpSize);
        }

        // if packet is complete, release any completed send packet
        if ((pProtoAries->iInpProg == pProtoAries->iInpSize) && (pProtoAries->iOutSize > 0) && (pProtoAries->iOutProg == pProtoAries->iOutSize))
        {
            pProtoAries->iOutSize = pProtoAries->iOutProg = 0;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function    ProtoAriesSend

    \Description
        Send an Aries packet to the server.

    \Input *pProtoAries - reference pointer
    \Input iKind        - packet kind
    \Input iCode        - packet subtype
    \Input *pBody       - packet data pointer
    \Input iBodyLen     - data length

    \Output
        int32_t         - <0=error, 0=no error

    \Version 11/01/2002 (gschaefer)
*/
/********************************************************************************F*/
int32_t ProtoAriesSend(ProtoAriesRefT *pProtoAries, int32_t iKind, int32_t iCode, const char *pBody, int32_t iBodyLen)
{
    ProtoAriesMsgHdrT *pHdr;
    unsigned char *pStart;
    int32_t iSize;

    // flush output packet if one is queued
    ProtoAriesUpdate(pProtoAries);

    // allow auto string sizing for conveniece
    if (iBodyLen < 0)
    {
        iBodyLen = (int32_t)strlen(pBody)+1;
    }

    // add in the header/crypto size
    iSize = CryptStp1EncryptSize(&pProtoAries->Stream, iBodyLen+sizeof(*pHdr));

    // at the end.
    if (_ProtoAriesBuff(pProtoAries, iSize) < iSize)
    {
        NetPrintf(("protoaries: send buffer overflow\n"));
        return(-1);
    }

    pStart = pProtoAries->pOutData+pProtoAries->iOutSize;
    pHdr = (ProtoAriesMsgHdrT *)pStart;

    // only iBodyLen if non-zero size (since pBody may be NULL)
    if (iBodyLen > 0)
    {
        ds_memcpy(pStart+sizeof(*pHdr), pBody, iBodyLen);
    }

    // add to size
    pProtoAries->iOutSize += iSize;

    // setup the packet header
    pHdr->size[0] = (unsigned char)(iSize >> 24);
    pHdr->size[1] = (unsigned char)(iSize >> 16);
    pHdr->size[2] = (unsigned char)(iSize >> 8);
    pHdr->size[3] = (unsigned char)(iSize >> 0);

    pHdr->kind[0] = (unsigned char)(iKind >> 24);
    pHdr->kind[1] = (unsigned char)(iKind >> 16);
    pHdr->kind[2] = (unsigned char)(iKind >> 8);
    pHdr->kind[3] = (unsigned char)(iKind >> 0);

    pHdr->code[0] = (unsigned char)(iCode >> 24);
    pHdr->code[1] = (unsigned char)(iCode >> 16);
    pHdr->code[2] = (unsigned char)(iCode >> 8);
    pHdr->code[3] = (unsigned char)(iCode >> 0);

    // encrypt packet if needed
    CryptStp1EncryptHash(&pProtoAries->Stream, (char *)pStart, iSize);
    CryptStp1EncryptData(&pProtoAries->Stream, (char *)pStart, iSize);

    // if user is requesting a ticket, discard the wallet
    if (iKind == '@tic')
    {
        pProtoAries->bHaveWallet = FALSE;
    }

    // call idle handler to process
    ProtoAriesUpdate(pProtoAries);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function ProtoAriesPeek

    \Description
        See if an incoming packet is waiting (but do not remove from queue).

    \Input *pProtoAries - reference pointer
    \Input *pKind       - [out] packet kind (from packet)
    \Input *pCode       - [out] packet subtype (from packet)
    \Input **ppBody     - [out] packet data pointer

    \Output
        int32_t             - <0=nothing pending, else size of data

    \Notes
        Pointer to data remains valid until call to ProtoAriesRecv

    \Version 11/01/2002 (gschaefer)
*/
/********************************************************************************F*/
int32_t ProtoAriesPeek(ProtoAriesRefT *pProtoAries, int32_t *pKind, int32_t *pCode, char **ppBody)
{
    ProtoAriesMsgHdrT *pHdr;
    char *pData;
    int32_t iLen;
    static char strNull[] = "";

    // see if anything waiting
    if ((pProtoAries->iInpSize == 0) || (pProtoAries->iInpProg != pProtoAries->iInpSize))
    {
        // dont update if this is a flush
        if ((pKind == NULL) && (pCode == NULL) && (ppBody == NULL))
        {
            return(-1);
        }
        // call idle handler
        ProtoAriesUpdate(pProtoAries);
        // return now if nothing changed
        if ((pProtoAries->iInpSize == 0) || (pProtoAries->iInpProg != pProtoAries->iInpSize))
        {
            return(-1);
        }
    }

    // point to packet header
    pHdr = (ProtoAriesMsgHdrT *) pProtoAries->pInpData;
    iLen = pProtoAries->iInpSize - sizeof(*pHdr);
    pData = (char *)pProtoAries->pInpData + sizeof(*pHdr);

    // always null terminate data
    if (iLen < 1)
    {
        pData = strNull;
    }
    else
    {
        pData[iLen] = '\0';
    }

    // peek the data
    if (pKind != NULL)
    {
        *pKind = (pHdr->kind[0]<<24)|(pHdr->kind[1]<<16)|(pHdr->kind[2]<<8)|(pHdr->kind[3]<<0);
    }
    if (pCode != NULL)
    {
        *pCode = (pHdr->code[0]<<24)|(pHdr->code[1]<<16)|(pHdr->code[2]<<8)|(pHdr->code[3]<<0);
    }
    if (ppBody != NULL)
    {
        *ppBody = pData;
    }

    // return data size
    return(iLen);
}

/*F********************************************************************************/
/*!
    \Function ProtoAriesRecv

    \Description
        Get a packet from the server (will copy packet to private buffer).

    \Input *pProtoAries - reference pointer
    \Input *pKind       - [out] packet kind
    \Input *pCode       - [out] packet subtype
    \Input *pBody       - [out] packet data buffer
    \Input iSize        - size of packet data buffer

    \Output
        int32_t             - <0=error, 0=no error

    \Notes
        Can call with body=NULL to remove packet seen with ProtoAriesPeek().

    \Version 11/01/2002 (gschaefer)
*/
/********************************************************************************F*/
int32_t ProtoAriesRecv(ProtoAriesRefT *pProtoAries, int32_t *pKind, int32_t *pCode, char *pBody, int32_t iSize)
{
    // see what is waiting
    int32_t iLen = ProtoAriesPeek(pProtoAries, pKind, pCode, NULL);

    // copy packet data if needed
    if (iLen >= 0)
    {
        // limit the copy size to buffer
        if (iSize > iLen)
        {
            iSize = iLen;
        }
        if (pBody != NULL)
        {
            ds_memcpy(pBody, pProtoAries->pInpData+sizeof(ProtoAriesMsgHdrT), iSize);
        }
        // remove from buffer
        pProtoAries->iInpProg = pProtoAries->iInpSize = 0;
    }

    // return actual size
    return(iLen);
}

/*F********************************************************************************/
/*!
    \Function ProtoAriesTick

    \Description
        Return timebase for easy client calculations.  Returns number of elapsed
        milliseconds from some point in time (typically system startup).  Overflow
        occurs in approximately 46 days.

    \Input *pProtoAries - reference pointer

    \Output
        uint32_t    - epoch tick in milliseconds

    \Version 11/01/2002 (gschaefer)
*/
/********************************************************************************F*/
uint32_t ProtoAriesTick(ProtoAriesRefT *pProtoAries)
{
    return(NetTick());
}
