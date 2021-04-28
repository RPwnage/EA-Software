/*H********************************************************************************/
/*!
    \File protowebsocketxboxone.cpp

    \Description
        This class wraps Microsoft's WebSocket class provided for Xbox One.

    \Copyright
        Copyright (c) 2013 Electronic Arts Inc.

    \Version 02/23/2013 (cvienneau) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdlib.h>
#include <string.h>
#include <wrl.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/proto/protowebsocket.h"
#include "DirtySDK/proto/protohttputil.h"
#include "DirtySDK/util/utf8.h"

using namespace Platform;
using namespace Windows::Data;
using namespace Windows::Foundation;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Details;

/*** Defines **********************************************************************/

//! default protowebsocket buffer size
#define PROTOWEBSOCKET_BUFSIZE_MIN  (4*1024)

#define PROTO_WEBSOCKET_XBOXONE_MAX_URL_SIZE (256)
#define PROTO_WEBSOCKET_XBOXONE_MAX_CLOSE_REASON_SIZE (256)

/*** Type Definitions *************************************************************/
// state for a websocket connection
struct ProtoWebSocketRefT
{
    char                    strUrl[PROTO_WEBSOCKET_XBOXONE_MAX_URL_SIZE];   //!< storage for the url we connect to
    int32_t                 iMemGroup;          //!< memory meta data
    void*                   pMemGroupUserData;  //!< memory meta data
    NetCritT                recvcrit;           //!< receive critical section
    char*                   pRecvBuf;           //!< recieve buffer
    char                    strReason[PROTO_WEBSOCKET_XBOXONE_MAX_CLOSE_REASON_SIZE];     //!< Reason for disconnect
    int32_t                 iBuffMax;           //!< max amount of data that can be stored in the send/recv buffer
    int32_t                 iRecvLen;           //!< current amount of data in buffer
    int32_t                 iVerbose;           //!< current module print verbosity level
    int32_t                 iCloseReason;        //!< Reason Code
    StreamWebSocket^        pWebSocket;         //!< ref to MS's websocket class
    DataWriter^             pOutputDataWriter;  //!< writer used to send through the websocket
    bool                    bPendingRecv;       //!< flag indicating if we are waiting for data to arrive
    bool                    bPendingSend;       //!< un ack'ed data is on the wire
    
    enum
    {
        ST_DISC,            //!< disconnected
        ST_CONN,            //!< connecting
        ST_OPEN,            //!< connected
        ST_FAIL             //!< connection failed
    } eState;               //!< current state

};

/*** Variables ********************************************************************/

#if DIRTYCODE_LOGGING
static const char *_ProtoWebsocketState[] =
{
    "ST_DISC",      // 0
    "ST_CONN",      // 1
    "ST_OPEN",      // 2
    "ST_FAIL"       // 3
};
#endif

/*** Function Prototypes **********************************************************/


/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketAppendHeader

    \Description
        Add headers in supplied buffer to connect handshake headers.

    \Input *pWebSocket  - module state
    \Input *pStrReason  - headers
    
    \Output
        int32_t         - 0 success, negative failure.

    \Version 03/04/2013 (cvienneau)
*/
/********************************************************************************F*/
int32_t _ProtoWebSocketAppendHeader(ProtoWebSocketRefT *pWebSocket, const char *pStrReason)
{
    if (pWebSocket->eState == ProtoWebSocketRefT::ST_DISC)
    {
        char strHeaderName [128];
        char strHeaderValue[10*1024];

        for (const char *pHdrTmp = pStrReason; ProtoHttpGetNextHeader(pWebSocket, pHdrTmp, strHeaderName, sizeof(strHeaderName), strHeaderValue, sizeof(strHeaderValue), &pHdrTmp) == 0; )
        {
            //translation of data types from char -> wchar -> string
            wchar_t wstrHeaderName [128];
            wchar_t wstrHeaderValue[10*1024];
            MultiByteToWideChar(CP_UTF8, 0, strHeaderName, -1, wstrHeaderName, sizeof(wstrHeaderName));
            MultiByteToWideChar(CP_UTF8, 0, strHeaderValue, -1, wstrHeaderValue, sizeof(wstrHeaderValue));
            Platform::String ^ pstrHeaderName = ref new Platform::String(wstrHeaderName);
            Platform::String ^ pstrHeaderValue = ref new Platform::String(wstrHeaderValue);
            pWebSocket->pWebSocket->SetRequestHeader(pstrHeaderName, pstrHeaderValue);
        }
    }
    else
    {
        NetPrintf(( "protowebsocketxboxone: [%p] unable to append headers, must be disconnected but current state is %s\n", pWebSocket, _ProtoWebsocketState[pWebSocket->eState]));
        return(-1);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketClose

    \Description
        Close connection to server.

    \Input *pWebSocket  - module state
    \Input iReason      - (optional) close reason (PROTOWEBSOCKET_CLOSEREASON_*)
    \Input *pStrReason  - (optional) string close reason
    
    \Output
        int32_t         - 0 success

    \Version 03/04/2013 (cvienneau)
*/
/********************************************************************************F*/
int32_t _ProtoWebSocketClose(ProtoWebSocketRefT *pWebSocket, int32_t iReason, const char *pStrReason)
{
    // ignore request in states where we are already disconnected
    if ((pWebSocket->eState == ProtoWebSocketRefT::ST_DISC) || (pWebSocket->eState == ProtoWebSocketRefT::ST_FAIL))
    {
        NetPrintfVerbose((pWebSocket->iVerbose, 1, "protowebsocketxboxone: [%p] Disconnecting skipped because not in connected state\n", pWebSocket));
        return(0);
    }

    NetPrintf(( "protowebsocketxboxone: [%p] Disconnecting from %s\n", pWebSocket, pWebSocket->strUrl));

    Platform::String ^  strReason = nullptr;
    wchar_t buff[PROTO_WEBSOCKET_XBOXONE_MAX_CLOSE_REASON_SIZE];
    if(pStrReason != NULL)
    {
        MultiByteToWideChar(CP_UTF8, 0, pStrReason, -1, buff, sizeof(buff));
        strReason = ref new Platform::String(buff);
    }

    // an invalid close reason results in an exception
    if ((iReason < PROTOWEBSOCKET_CLOSEREASON_BASE) || (iReason > PROTOWEBSOCKET_CLOSEREASON_MAX))
    {
        NetPrintfVerbose((pWebSocket->iVerbose, 1, "protowebsocketxboxone: [%p] Invalid close reason provided %d, using %d instead.\n", pWebSocket, iReason, PROTOWEBSOCKET_CLOSEREASON_NORMAL));
        iReason = PROTOWEBSOCKET_CLOSEREASON_NORMAL;
    }
    
    pWebSocket->pWebSocket->Close((unsigned short)iReason, strReason);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketRecv

    \Description
        Receive any pending data and place it in our modules buffer

    \Input *pWebSocket  - module state

    \Output
        int32_t         - negative for an error, 0 recv skipped, 1 recv started

    \Version 03/04/2013 (cvienneau)
*/
/********************************************************************************F*/
int _ProtoWebSocketRecv(ProtoWebSocketRefT *pWebSocket)
{
    NetCritEnter(&pWebSocket->recvcrit);
    int32_t iRecvSize = pWebSocket->iBuffMax - pWebSocket->iRecvLen;

    // handle not connected states
    if ((pWebSocket->eState == ProtoWebSocketRefT::ST_DISC) || (pWebSocket->eState == ProtoWebSocketRefT::ST_FAIL))
    {
        NetCritLeave(&pWebSocket->recvcrit);
        return(SOCKERR_NOTCONN);
    }
    // handle states where we're not ready to recieve
    else if ((pWebSocket->eState == ProtoWebSocketRefT::ST_CONN) || (pWebSocket->bPendingRecv == true) || (iRecvSize <= 0))
    {
        NetCritLeave(&pWebSocket->recvcrit);
        return(0);
    }

    pWebSocket->bPendingRecv = true;
    Windows::Storage::Streams::Buffer^ buff = ref new Buffer(iRecvSize);  //receive a max of the amount of space left over
    auto readOp = pWebSocket->pWebSocket->InputStream->ReadAsync( buff, iRecvSize, Windows::Storage::Streams::InputStreamOptions::Partial );

    readOp->Completed = ref new AsyncOperationWithProgressCompletedHandler< IBuffer^, unsigned int >
        ( [ pWebSocket ]( IAsyncOperationWithProgress< IBuffer^, unsigned int >^ asyncOp, Windows::Foundation::AsyncStatus asyncStatus )
    {
        NetCritEnter(&pWebSocket->recvcrit);
        switch ( asyncStatus )
        {
        case Windows::Foundation::AsyncStatus::Completed:
            try
            {
                Windows::Storage::Streams::IBuffer^ localReadBuf = asyncOp->GetResults();
                int32_t bytesRead = localReadBuf->Length;
                if ((pWebSocket->iRecvLen + bytesRead) <= pWebSocket->iBuffMax)
                {
                    NetPrintfVerbose((pWebSocket->iVerbose, 1, "protowebsocketxboxone: [%p] Recv success %d bytes\n", pWebSocket, bytesRead));
                    DataReader^ chunkReader = DataReader::FromBuffer( localReadBuf );
                    Platform::Array<unsigned char> ^ dataArray = ref new Platform::Array<unsigned char>(bytesRead);
                    chunkReader->ReadBytes(dataArray);
                    ds_memcpy(pWebSocket->pRecvBuf + pWebSocket->iRecvLen, dataArray->Data, bytesRead);
                    pWebSocket->iRecvLen += bytesRead;
                }
                else
                {
                    NetPrintf(( "protowebsocketxboxone: [%p] Recv error insufficient space, Max %d, Current %d, New %d.\n", pWebSocket, pWebSocket->iBuffMax, pWebSocket->iRecvLen, bytesRead));
                }
            }
            catch( Exception^ e )
            {
                NetPrintf(( "protowebsocketxboxone: [%p] Recv error, completion failed: 0x%x\n", pWebSocket, e->HResult));
                pWebSocket->eState = ProtoWebSocketRefT::ST_FAIL;
            }
            break;
        case Windows::Foundation::AsyncStatus::Error:
            {
                NetPrintf(( "protowebsocketxboxone: [%p] Recv error: 0x%x\n", pWebSocket, asyncOp->ErrorCode));
                pWebSocket->eState = ProtoWebSocketRefT::ST_FAIL;
                break;
            }
        default:
            {
                NetPrintf(( "protowebsocketxboxone: [%p] Recv error: default\n", pWebSocket));            
                pWebSocket->eState = ProtoWebSocketRefT::ST_FAIL;
                break;
            }
        }
        pWebSocket->bPendingRecv = false;
        NetCritLeave(&pWebSocket->recvcrit);
    });

    NetCritLeave(&pWebSocket->recvcrit);
    return(1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoWebSocketRecv

    \Description
        Initialize MS's websocket class in the module state.

    \Input *pWebSocket  - module state

    \Output
        int32_t         - true for success

    \Version 03/04/2013 (cvienneau)
*/
/********************************************************************************F*/
bool _ProtoWebSocketInit(ProtoWebSocketRefT *pWebSocket)
{
    // clear off any old referances we might have
    pWebSocket->pOutputDataWriter = nullptr;
    pWebSocket->pWebSocket = nullptr;

    // allocate and init MS's websocket object
    if ((pWebSocket->pWebSocket = ref new StreamWebSocket()) == nullptr)
    {
        NetPrintf(( "protowebsocketxboxone: [%p] Error allocating StreamWebSocket\n", pWebSocket));
        ProtoWebSocketDestroy(pWebSocket);
        return(false);
    }
    pWebSocket->pWebSocket->Control->NoDelay = true;
    pWebSocket->pWebSocket->Control->OutboundBufferSizeInBytes = pWebSocket->iBuffMax;  //send/recv buffers are same size

    //Callback to get disconnect reason
    pWebSocket->pWebSocket->Closed += ref new TypedEventHandler<IWebSocket^, WebSocketClosedEventArgs^> ([pWebSocket] (IWebSocket^ sender, WebSocketClosedEventArgs^ args)
    {
        pWebSocket->iCloseReason = args->Code;
        memset(pWebSocket->strReason, 0, PROTO_WEBSOCKET_XBOXONE_MAX_CLOSE_REASON_SIZE);
        Utf8EncodeFromUCS2(pWebSocket->strReason, PROTO_WEBSOCKET_XBOXONE_MAX_CLOSE_REASON_SIZE, (uint16 *)args->Reason->Data());
    });

    // allocate and init the data writer for sending
    if ((pWebSocket->pOutputDataWriter = ref new DataWriter(pWebSocket->pWebSocket->OutputStream)) == nullptr)
    {
        NetPrintf(( "protowebsocketxboxone: [%p] Error allocating DataWriter\n", pWebSocket));
        ProtoWebSocketDestroy(pWebSocket);
        return(false);
    }

    return(true);
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ProtoWebSocketCreate

    \Description
        Allocate a WebSocket connection and prepare for use

    \Input iBufSize             - websocket buffer size (determines max message size)

    \Output
        ProtoWebSocketRefT *    - module state, or NULL on failure

    \Version 02/23/2013 (cvienneau)
*/
/********************************************************************************F*/
ProtoWebSocketRefT *ProtoWebSocketCreate(int32_t iBufSize)
{
    ProtoWebSocketRefT *pWebSocket;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // clamp the buffer size
    if (iBufSize < PROTOWEBSOCKET_BUFSIZE_MIN)
    {
        iBufSize = PROTOWEBSOCKET_BUFSIZE_MIN;
    }

    // allocate and init module state
    if ((pWebSocket = (ProtoWebSocketRefT*)DirtyMemAlloc(sizeof(*pWebSocket), PROTOWEBSOCKET_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(( "protowebsocketxboxone: Error could not allocate module state\n"));
        return(NULL);
    }
    else
    {
        NetPrintfVerbose((pWebSocket->iVerbose, 1, "protowebsocketxboxone: [%p] ProtoWebSocketCreate\n", pWebSocket));
    }
    memset(pWebSocket, 0, sizeof(*pWebSocket));
    pWebSocket->iMemGroup = iMemGroup;
    pWebSocket->pMemGroupUserData = pMemGroupUserData;

    // allocate recieve buffer
    if ((pWebSocket->pRecvBuf = (char *)DirtyMemAlloc(iBufSize, PROTOWEBSOCKET_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(( "protowebsocketxboxone: [%p] could not allocate receive buffer\n", pWebSocket));
        ProtoWebSocketDestroy(pWebSocket);
        return(NULL);
    }
    pWebSocket->iBuffMax = iBufSize;

    // initilaize MS's websocket class, in error cases this portion needs to be re-initialized
    if (_ProtoWebSocketInit(pWebSocket) == false)
    {
        return(NULL);
    }

    NetCritInit(&pWebSocket->recvcrit, "websocket-recv");

    // return module state to caller
    return(pWebSocket);
}

/*F********************************************************************************/
/*!
    \Function ProtoWebSocketCreate2

    \Description
        Allocate a WebSocket connection and prepare for use: ProtoSSL function
        signature compatible version.

    \Output
        ProtoWebSocketRefT *    - module state, or NULL on failure

    \Version 03/14/2013 (cvienneau)
*/
/********************************************************************************F*/
ProtoWebSocketRefT *ProtoWebSocketCreate2(void)
{
    return(ProtoWebSocketCreate(PROTOWEBSOCKET_BUFSIZE_MIN));
}

/*F********************************************************************************/
/*!
    \Function ProtoWebSocketDestroy

    \Description
        Destroy a connection and release state

    \Input *pWebSocket  - websocket module state

    \Version 02/23/2013 (cvienneau)
*/
/********************************************************************************F*/
void ProtoWebSocketDestroy(ProtoWebSocketRefT *pWebSocket)
{
    NetPrintfVerbose((pWebSocket->iVerbose, 1, "protowebsocketxboxone: [%p] ProtoWebSocketDestroy\n", pWebSocket));
    
    if (pWebSocket != NULL)
    {
        ProtoWebSocketDisconnect(pWebSocket);
        pWebSocket->pOutputDataWriter = nullptr;
        pWebSocket->pWebSocket = nullptr;
        if (pWebSocket->pRecvBuf != NULL)
        {
            DirtyMemFree(pWebSocket->pRecvBuf, PROTOWEBSOCKET_MEMID, pWebSocket->iMemGroup, pWebSocket->pMemGroupUserData);
        }
        NetCritKill(&pWebSocket->recvcrit);
        DirtyMemFree(pWebSocket, PROTOWEBSOCKET_MEMID, pWebSocket->iMemGroup, pWebSocket->pMemGroupUserData);
    }
}

/*F********************************************************************************/
/*!
    \Function ProtoWebSocketAccept

    \Description
        Accept an incoming connection

    \Input *pWebSocket          - websocket module state

    \Output
        ProtoWebSocketRefT *    - new connection, or NULL if no connection

    \Version 02/23/2013 (cvienneau)
*/
/********************************************************************************F*/
ProtoWebSocketRefT *ProtoWebSocketAccept(ProtoWebSocketRefT *pWebSocket)
{
    NetPrintf(( "protowebsocketxboxone: [%p] ProtoWebSocketAccept is not implemented.\n", pWebSocket));
    return(NULL); //$$TODO
}

/*F********************************************************************************/
/*!
    \Function ProtoWebSocketListen

    \Description
        Start listening for an incoming connection

    \Input *pWebSocket          - websocket module state

    \Output
        ProtoWebSocketRefT *    - ?

    \Version 02/23/2013 (cvienneau)
*/
/********************************************************************************F*/
ProtoWebSocketRefT *ProtoWebSocketListen(ProtoWebSocketRefT *pWebSocket)
{
    NetPrintf(( "protowebsocketxboxone: [%p] ProtoWebSocketListen is not implemented.\n", pWebSocket));
    return(NULL); //$$TODO
}

/*F********************************************************************************/
/*!
    \Function ProtoWebSocketConnect

    \Description
        Initiate a connection to a server

    \Input *pWebSocket          - websocket module state
    \Input *pUrl                - url containing host information; ws:// or wss://

    \Output
        int32_t                 - zero=success, negative failure

    \Version 02/23/2013 (cvienneau)
*/
/********************************************************************************F*/
int32_t ProtoWebSocketConnect(ProtoWebSocketRefT *pWebSocket, const char *pUrl)
{
    if (pWebSocket->eState == ProtoWebSocketRefT::ST_DISC)
    {
        wchar_t buff[PROTO_WEBSOCKET_XBOXONE_MAX_URL_SIZE];
        pWebSocket->eState = ProtoWebSocketRefT::ST_CONN;
        
        NetPrintfVerbose((pWebSocket->iVerbose, 2, "protowebsocketxboxone: [%p] Beginning connection to %s\n", pWebSocket, pUrl));
        ds_strnzcpy(pWebSocket->strUrl, pUrl, PROTO_WEBSOCKET_XBOXONE_MAX_URL_SIZE);
        MultiByteToWideChar(CP_UTF8, 0, pWebSocket->strUrl, -1, buff, sizeof(buff));
        Uri^ uri = ref new Uri( ref new String( buff ) );

        Windows::Foundation::IAsyncAction ^ connectOp = pWebSocket->pWebSocket->ConnectAsync( uri );
        connectOp->Completed = ref new AsyncActionCompletedHandler([pWebSocket]( IAsyncAction^ asyncAction, Windows::Foundation::AsyncStatus asyncStatus )
        {
            switch (asyncStatus)
            {
            case Windows::Foundation::AsyncStatus::Completed:
                {
                NetPrintfVerbose((pWebSocket->iVerbose, 1, "protowebsocketxboxone: [%p] Connect success to %s\n", pWebSocket, pWebSocket->strUrl));
                pWebSocket->eState = ProtoWebSocketRefT::ST_OPEN;
                break;
                }
            case Windows::Foundation::AsyncStatus::Error:
                {
                NetPrintf(( "protowebsocketxboxone: [%p] Connect error 0x%x to %s\n", pWebSocket, asyncAction->ErrorCode, pWebSocket->strUrl ));
                pWebSocket->eState = ProtoWebSocketRefT::ST_FAIL;
                break;
                }
            default:
                {
                NetPrintf(( "protowebsocketxboxone: [%p] Connect error default to %s\n", pWebSocket, pWebSocket->strUrl));
                pWebSocket->eState = ProtoWebSocketRefT::ST_FAIL;
                break;
                }
            }
        });
        return(0);
    }
    NetPrintf(( "protowebsocketxboxone: [%p] Unable to connect to %s, because current state is %s\n", pWebSocket, pUrl, _ProtoWebsocketState[pWebSocket->eState]));
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ProtoWebSocketConnect2

    \Description
        Initiate a connection to a server: ProtoSSL function signature compatible
        version.

    \Input *pWebSocket          - websocket module state
    \Input iUnused0             - unused
    \Input *pUrl                - url containing host information; ws:// or wss://
    \Input uUnused1             - unused
    \Input iUnused2             - unused

    \Output
        int32_t                 - zero=success, else failure

    \Version 03/14/2013 (cvienneau)
*/
/********************************************************************************F*/
int32_t ProtoWebSocketConnect2(ProtoWebSocketRefT *pWebSocket, int32_t iUnused0, const char *pUrl, uint32_t uUnused1, int32_t iUnused2)
{
    return(ProtoWebSocketConnect(pWebSocket, pUrl));
}

/*F********************************************************************************/
/*!
    \Function ProtoWebSocketDisconnect

    \Description
        Disconnect from the server.  

    \Input *pWebSocket          - websocket module state

    \Output
        int32_t                 - zero=success, else failure

    \Version 02/23/2013 (cvienneau)
*/
/********************************************************************************F*/
int32_t ProtoWebSocketDisconnect(ProtoWebSocketRefT *pWebSocket)
{
    _ProtoWebSocketClose(pWebSocket, PROTOWEBSOCKET_CLOSEREASON_NORMAL, NULL);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function ProtoWebSocketSend

    \Description
        Send data to a server over a WebSocket connection.  Stream-type send.

    \Input *pWebSocket          - websocket module state
    \Input *pBuffer             - data to send
    \Input iLength              - amount of data to send

    \Output
        int32_t                 - negative=failure, else number of bytes of data sent

    \Version 02/23/2013 (cvienneau)
*/
/********************************************************************************F*/
int32_t ProtoWebSocketSend(ProtoWebSocketRefT *pWebSocket, const char *pBuffer, int32_t iLength)
{
    // handle not connected states
    if ((pWebSocket->eState == ProtoWebSocketRefT::ST_DISC) || (pWebSocket->eState == ProtoWebSocketRefT::ST_FAIL))
    {
        return(SOCKERR_NOTCONN);
    }
    else if (pWebSocket->iBuffMax < iLength)
    {
        NetPrintf(( "protowebsocketxboxone: [%p] Send error, insufficient buffer space %d, for send size %d.\n", pWebSocket, pWebSocket->iBuffMax, iLength));
        return(SOCKERR_OTHER);
    }
    // handle states where we're not ready to send, not sure if we can have many pending sends out at once, but doing it for simplicity for now
    else if ((pWebSocket->eState == ProtoWebSocketRefT::ST_CONN) || (pWebSocket->bPendingSend == true))
    {
        return(0);
    }

    pWebSocket->bPendingSend = true;
    Platform::Array<unsigned char> ^ data = ref new Platform::Array<unsigned char>((unsigned char*)pBuffer, iLength);
    pWebSocket->pOutputDataWriter->WriteBytes(data);
    Windows::Foundation::IAsyncOperation<unsigned int> ^ sendOp = pWebSocket->pOutputDataWriter->StoreAsync();

    sendOp->Completed = ref new Windows::Foundation::AsyncOperationCompletedHandler<unsigned int>([pWebSocket]( IAsyncOperation<unsigned int> ^ asyncOp, Windows::Foundation::AsyncStatus asyncStatus )
    {
        switch (asyncStatus)
        {
        case Windows::Foundation::AsyncStatus::Completed:
            {
            NetPrintfVerbose((pWebSocket->iVerbose, 1, "protowebsocketxboxone: [%p] Send success\n", pWebSocket));
            break;
            }
        case Windows::Foundation::AsyncStatus::Error:
            {
            NetPrintf(( "protowebsocketxboxone: [%p] Send error 0x%x\n", pWebSocket, asyncOp->ErrorCode));
            pWebSocket->eState = ProtoWebSocketRefT::ST_FAIL;
            break;
            }
        default:
            {
            NetPrintf(( "protowebsocketxboxone: [%p] Send error default\n", pWebSocket));
            pWebSocket->eState = ProtoWebSocketRefT::ST_FAIL;
            break;
            }
        }
        pWebSocket->bPendingSend = false;
    });

    return(iLength);
}

/*F********************************************************************************/
/*!
    \Function ProtoWebSocketRecv

    \Description
        Receive data from a server over a WebSocket connection

    \Input *pWebSocket          - websocket module state
    \Input *pBuffer             - [out] buffer to receive data
    \Input iLength              - size of buffer

    \Output
        int32_t                 - negative=failure, else number of bytes of data received

    \Version 02/23/2013 (cvienneau)
*/
/********************************************************************************F*/
int32_t ProtoWebSocketRecv(ProtoWebSocketRefT *pWebSocket, char *pBuffer, int32_t iLength)
{
    NetCritEnter(&pWebSocket->recvcrit);
    // so long as there is data in the revieve buffer they can make this call
    if (pWebSocket->iRecvLen > 0)
    {
        //find the maximum amount of data we want to copy MIN(iLength, pWebSocket->iRecvLen)
        int32_t iByteCount = pWebSocket->iRecvLen;
        int32_t iRemainingBytes = 0;
        if(iLength < iByteCount)
        {
            iByteCount = iLength;
            iRemainingBytes = pWebSocket->iRecvLen - iLength;
        }

        //data is now in user's buffer
        ds_memcpy(pBuffer, pWebSocket->pRecvBuf, iByteCount);

        //move any remaining data to the beginning of the recv buffer
        if (iRemainingBytes > 0)
        {
            memmove(pWebSocket->pRecvBuf, pWebSocket->pRecvBuf + iByteCount, iRemainingBytes);
        }
        pWebSocket->iRecvLen -= iByteCount;
        NetPrintfVerbose((pWebSocket->iVerbose, 1, "protowebsocketxboxone: [%p] read %d bytes, %d remain buffered\n", pWebSocket, iByteCount, pWebSocket->iRecvLen));
        NetCritLeave(&pWebSocket->recvcrit);
        return iByteCount;
    }

    // if there is no data, and they're not connected its an error, otherwise there is just nothing left to read at the moment
    if ((pWebSocket->eState == ProtoWebSocketRefT::ST_DISC) || (pWebSocket->eState == ProtoWebSocketRefT::ST_FAIL))
    {
        NetCritLeave(&pWebSocket->recvcrit);
        return(SOCKERR_NOTCONN);
    }
    NetCritLeave(&pWebSocket->recvcrit);
    return (0);
}

/*F********************************************************************************/
/*!
    \Function ProtoWebSocketStatus

    \Description
        Get module status

    \Input *pWebSocket          - websocket module state
    \Input iSelect              - status selector
    \Input *pBuffer             - [in/out] buffer, selector specific
    \Input iBufSize             - size of buffer

    \Output
        int32_t                 - selector specific

    \Notes
        Selectors are:

    \verbatim
        SELECTOR    RETURN RESULT
        'crsn'      returns closed reason code (PROTOWEBSOCKET_CLOSEREASON_*) and copies the closed reason string into pBuffer (Optional).
        'host'      current host copied to output buffer
        'port'      current port
        'stat'      connection status (-1=not connected, 0=connection in progress, 1=connected)
    \endverbatim

    \Version 03/05/2013 (cvienneau)
*/
/********************************************************************************F*/
int32_t ProtoWebSocketStatus(ProtoWebSocketRefT *pWebSocket, int32_t iSelect, void *pBuffer, int32_t iBufSize)
{
    // return close reason
    if (iSelect == 'crsn')
    {
        if (pBuffer != NULL && iBufSize >= PROTO_WEBSOCKET_XBOXONE_MAX_CLOSE_REASON_SIZE)
        {
            ds_memcpy(pBuffer, pWebSocket->strReason, PROTO_WEBSOCKET_XBOXONE_MAX_CLOSE_REASON_SIZE);
        }
        
        return(pWebSocket->iCloseReason);
    }

    // return current host
    if (iSelect == 'host')
    {
        if (pBuffer != NULL)
        {
            char strKind[16];
            int32_t iPort, iSecure;
            uint8_t bPortSpecified;
            ProtoHttpUrlParse2(pWebSocket->strUrl, strKind, sizeof(strKind), (char *)pBuffer, iBufSize, &iPort, &iSecure, &bPortSpecified);
        }
        else
        {
            NetPrintf(( "protowebsocketxboxone: [%p] ProtoWebSocketStatus 'host' error, buffer required.\n", pWebSocket));
            return(-1);
        }
        return(0);
    }

    // return current port
    if (iSelect == 'port')
    {
        char strUrl[PROTO_WEBSOCKET_XBOXONE_MAX_URL_SIZE];
        char strKind[16];
        int32_t iPort, iSecure;
        uint8_t bPortSpecified;
        ProtoHttpUrlParse2(pWebSocket->strUrl, strKind, sizeof(strKind), strUrl, sizeof(strUrl), &iPort, &iSecure, &bPortSpecified);
        if (bPortSpecified == FALSE)
        {
            // do our own secure determination
            iSecure = ds_stricmp(strKind, "wss") ? 0 : 1;
            if (iSecure && (iPort == 80))
            {
                iPort = 443;
            }
        }        
        return(iPort);
    }

    // check connection status
    if (iSelect == 'stat')
    {
        if ((pWebSocket->eState == ProtoWebSocketRefT::ST_DISC) || (pWebSocket->eState == ProtoWebSocketRefT::ST_FAIL))
        {
            return(-1);
        }
        else if (pWebSocket->eState != ProtoWebSocketRefT::ST_OPEN)
        {
            return(0);
        }
        return(1);
    }

    // unhandled
    NetPrintf(("protowebsocketxboxone: [%p] ProtoWebSocketStatus('%c%c%c%c') unhandled\n", pWebSocket,
        (uint8_t)(iSelect>>24), (uint8_t)(iSelect>>16), (uint8_t)(iSelect>>8), (uint8_t)(iSelect)));

    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ProtoWebSocketControl

    \Description
        Control module behavior

    \Input *pWebSocket          - websocket module state
    \Input iSelect              - control selector
    \Input iValue               - selector specific
    \Input iValue2              - selector specific
    \Input *pValue              - selector specific

    \Output
        int32_t                 - selector specific

    \Notes
        Selectors are:

        \verbatim
            SELECTOR    DESCRIPTION
            'apnd'      The given buffer will be added to the request header, must be used before connecting.
            'clse'      Send a close message with optional reason (iValue) and data (string passed in pValue)
            'spam'      Sets debug output verbosity (0...n)
        \endverbatim

    \Version 03/05/2013 (cvienneau)
*/
/********************************************************************************F*/
int32_t ProtoWebSocketControl(ProtoWebSocketRefT *pWebSocket, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iSelect == 'apnd')
    {
        _ProtoWebSocketAppendHeader(pWebSocket, (const char *)pValue);
        return(0);
    }
    if (iSelect == 'clse')
    {
        return(_ProtoWebSocketClose(pWebSocket, iValue, (const char *)pValue));
    }
    if (iSelect == 'spam')
    {
        NetPrintf(( "protowebsocketxboxone: [%p] verbose set to %d\n", pWebSocket, iValue));
        pWebSocket->iVerbose = iValue;
        return(0);
    }

    // unhandled
    NetPrintf(("protowebsocketxboxone: [%p] ProtoWebSocketControl('%c%c%c%c') unhandled\n", pWebSocket,
        (uint8_t)(iSelect>>24), (uint8_t)(iSelect>>16), (uint8_t)(iSelect>>8), (uint8_t)(iSelect)));

    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ProtoWebSocketUpdate

    \Description
        Give life to module (should be called periodically to allow module to
        perform work).

    \Input *pWebSocket          - websocket module state

    \Version 03/05/2013 (cvienneau)
*/
/********************************************************************************F*/
void ProtoWebSocketUpdate(ProtoWebSocketRefT *pWebSocket)
{
    //if (pWebSocket->eState == ProtoWebSocketRefT::ST_DISC){}//initial state, nothing going on
    //if (pWebSocket->eState == ProtoWebSocketRefT::ST_CONN){}//connection in progress, can't do anything till completed

    if (pWebSocket->eState == ProtoWebSocketRefT::ST_OPEN)
    {
        //we are connected, we can send and recieve
        _ProtoWebSocketRecv(pWebSocket);
    }
    if (pWebSocket->eState == ProtoWebSocketRefT::ST_FAIL)
    {
        //we are in a bad state, do some clean up and return to disconnected
        if (_ProtoWebSocketInit(pWebSocket) == true)
        {
            pWebSocket->eState = ProtoWebSocketRefT::ST_DISC;
        }
    }    
}
