/*H********************************************************************************/
/*!
    \File    protohttp2.c

    \Description
        This module implements an HTTP/2 client that can perform basic transactions
        with an HTTP/2 server.  It conforms to but does not fully implement
        the 2.0 HTTP spec (https://tools.ietf.org/html/rfc7540), and
        allows for secure HTTP transactions as well as insecure transactions.

    \Copyright
        Copyright (c) Electronic Arts 2016. ALL RIGHTS RESERVED.

    \Version 09/27/2016 (eesponda)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>
#include <stdlib.h>

#include "DirtySDK/dirtydefs.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/proto/protossl.h"
#include "DirtySDK/util/hpack.h"
#include "DirtySDK/proto/protohttp2.h"

/*** Defines **********************************************************************/

//! size of a http2 header (24-bit length, 8-bit flag, 8-bit type and 31-bit stream id)
#define PROTOHTTP2_HEADER_SIZE      (9)

//! size of http2 setting (16-bit key and 32-bit value)
#define PROTOHTTP2_SETTING_SIZE     (6)

//! size of http2 ping (8 octets of opaque data)
#define PROTOHTTP2_PING_SIZE        (8)

//! size of http2 goaway (31-bit stream id and 32-bit error code)
#define PROTOHTTP2_GOAWAY_SIZE      (8)

//! size of http2 rst_stream (32-bit error code)
#define PROTOHTTP2_RST_STREAM_SIZE  (4)

//! size of http2 window update (31-bit window size increment)
#define PROTOHTTP2_WINDOW_UPDATE_SIZE (4)

//! size of the http2 priority frame (1 bit exclusive, 31-bit stream dependency, 8-bit weight)
#define PROTOHTTP2_PRIORITY_SIZE    (5)

//! reserved size in the buffer for a header + important frames (ping/settings/rst_stream)
#define PROTOHTTP2_RESERVED_SIZE    \
    ((PROTOHTTP2_HEADER_SIZE*4)+(PROTOHTTP2_SETTING_SIZE*SETTINGS_NUM)+PROTOHTTP2_PING_SIZE+PROTOHTTP2_RST_STREAM_SIZE)

//! flags for http frames
#define PROTOHTTP2_FLAG_ACK         (1 << 0) /* acknowledge ping / settings */
#define PROTOHTTP2_FLAG_END_STREAM  (1 << 0) /* indicates end data chunk */
#define PROTOHTTP2_FLAG_END_HEADERS (1 << 2) /* indicates end headers, ie: no continuation frames */
#define PROTOHTTP2_FLAG_PADDED      (1 << 3) /* indicates data / header is padded so look for pad length */
#define PROTOHTTP2_FLAG_PRIORITY    (1 << 5) /* indicates priority information */

//! size of sent/received header cache
#define PROTOHTTP2_HDRCACHESIZE     (1024)

//! size of the receive window
#define PROTOHTTP2_WINDOWSIZE       (1024*16)

//! module version (maj.min)
#define PROTOHTTP2_VERSION          (0x0200)

//! maximum number of concurrent streams supported
#define PROTOHTTP2_MAX_STREAMS      (100)

//! default protocol timeout
#define PROTOHTTP2_TIMEOUT_DEFAULT  (30*1000)

/*** Macros ***********************************************************************/

//! calculates how much free space we have for encoding data/header frames (ie: user frames)
#define PROTOHTTP2_CalculateFreeSpace(pState)                   \
    (pState->iOutMax-pState->iOutLen-PROTOHTTP2_RESERVED_SIZE)

//! clamps the maximum frame size (16k-16m)
#define PROTOHTTP2_ClampFrameSize(iValue)   \
    iValue = DS_MAX(iValue, 16384);         \
    iValue = DS_MIN(iValue, 16777215);

/*** Type Definitions *************************************************************/

//! settings supported on the http2 connection
enum
{
    SETTINGS_HEADER_TABLE_SIZE = 1,
    SETTINGS_ENABLE_PUSH,
    SETTINGS_MAX_CONCURRENT_STREAM,
    SETTINGS_INITIAL_WINDOW_SIZE,
    SETTINGS_MAX_FRAME_SIZE,
    SETTINGS_MAX_HEADER_LIST_SIZE,

    SETTINGS_NUM
};

//! the different types of http2 frames we expect
typedef enum FrameTypeE
{
    FRAMETYPE_DATA,
    FRAMETYPE_HEADERS,
    FRAMETYPE_PRIORITY,
    FRAMETYPE_RST_STREAM,
    FRAMETYPE_SETTINGS,
    FRAMETYPE_PUSH_PROMISE,
    FRAMETYPE_PING,
    FRAMETYPE_GOAWAY,
    FRAMETYPE_WINDOW_UPDATE,
    FRAMETYPE_CONTINUATION,
    FRAMETYPE_LAST = FRAMETYPE_CONTINUATION
} FrameTypeE;

//! the different types of errors we send in RST_STREAM/GOAWAY frames
typedef enum ErrorTypeE
{
    ERRORTYPE_NO_ERROR,
    ERRORTYPE_PROTOCOL_ERROR,
    ERRORTYPE_INTERNAL_ERROR,
    ERRORTYPE_FLOW_CONTROL_ERROR,
    ERRORTYPE_SETTINGS_TIMEOUT,
    ERRORTYPE_STREAM_CLOSED,
    ERRORTYPE_FRAME_SIZE_ERROR,
    ERRORTYPE_REFUSED_STREAM,
    ERRORTYPE_CANCEL,
    ERRORTYPE_COMPRESSION_ERROR,
    ERRORTYPE_CONNECT_ERROR,
    ERRORTYPE_ENHANCE_YOUR_CALM,
    ERRORTYPE_INADEQUATE_SECURITY,
    ERRORTYPE_HTTP_1_1_REQUIRED
} ErrorTypeE;

//! header embedded into every http2 frame
typedef struct FrameHeaderT
{
    uint32_t uLength;   //!< length of the payload (24 bits)
    uint8_t  uType;     //!< the frame type corresponds to the FrameTypeE enum (8 bits)
    uint8_t  uFlags;    //!< the frame flags, this value depends on the frame type (8 bits)
    uint8_t  uPadding;  //!< amount of padding (8 bits)
    uint8_t  _pad;
    int32_t  iStreamId; //!< identifier of the stream (31 bits)
} FrameHeaderT;

//! settings information
typedef struct SettingsT
{
    uint32_t uHeaderTableSize;      //!< SETTINGS_HEADER_TABLE_SIZE
    uint32_t uEnablePush;           //!< SETTINGS_ENABLE_PUSH
    uint32_t uMaxConcurrentStream;  //!< SETTINGS_MAX_CONCURRENT_STREAM
    uint32_t uInitialWindowSize;    //!< SETTINGS_INITIAL_WINDOW_SIZE
    uint32_t uMaxFrameSize;         //!< SETTINGS_MAX_FRAME_SIZE
    uint32_t uMaxHeaderListSize;    //!< SETTINGS_MAX_HEADER_LIST_SIZE
} SettingsT;

//! stream information
typedef struct StreamInfoT
{
    int32_t iStreamId;                              //!< stream identifier
    ProtoHttp2StreamStateE eState;                  //!< current state of the stream

    ProtoHttpRequestTypeE eRequestType;             //!< request type of current request
    ProtoHttpResponseE eResponseCode;               //!< HTTP error code from the response
    ErrorTypeE eErrorType;                          //!< cached error code from a rst_stream

    char *pHeader;                                  //!< storage for response header (uncompressed)
    int32_t iHeaderLen;                             //!< length of the response header

    char *pRequestHeader;                           //!< storage for cached request header (uncompressed)
    int32_t iRequestHeaderLen;                      //!< length of the request header

    uint8_t *pData;                                 //!< storage for data frames used for polling
    int32_t iDataLen;                               //!< length of the data frame storage
    int32_t iDataMax;                               //!< maximum size of the data frame storage

    int32_t iLocalWindow;                           //!< size of the stream wide receive window (local)
    int32_t iPeerWindow;                            //!< size of the stream wide receive window (remote)

    ProtoHttp2WriteCbT *pWriteCb;                   //!< user write callback pointer
    void *pUserData;                                //!< user data to pass along with callback

    int64_t iBodySize;                              //!< size of body data
    int64_t iBodyReceived;                          //!< size of body data received by caller
    int32_t iRecvSize;                              //!< amount of data received by ProtoHttp2RecvAll
    int32_t iHdrDate;                               //!< last modified date
} StreamInfoT;

//! http2 module state
struct ProtoHttp2RefT
{
    ProtoSSLRefT *pSsl;     //!< ssl module used for communication

    //! memgroup data
    int32_t iMemGroup;
    void *pMemGroupUserData;

    uint8_t *pOutBuf;       //!< send buffer
    int32_t iOutMax;        //!< size of send buffer
    int32_t iOutLen;        //!< length in send buffer
    int32_t iOutOff;        //!< offset into the send buffer
    uint8_t *pInpBuf;       //!< recv buffer
    int32_t iInpMax;        //!< size of recv buffer
    int32_t iInpLen;        //!< length in recv buffer

    int32_t iLocalWindow;   //!< size of the connection wide receive window (local)
    int32_t iPeerWindow;    //!< size of the connection wide receive window (remote)

    //! callback data
    ProtoHttp2CustomHeaderCbT *pCustomHeaderCb;
    ProtoHttp2ReceiveHeaderCbT *pReceiveHeaderCb;
    void *pCallbackRef;

    int32_t iStreamId;          //!< identifier of the current stream id for new requests
    int32_t iVerbose;           //!< logging verbosity
    int32_t iSslFail;           //!< ssl failure code, if any
    int32_t iHresult;           //!< ssl hresult code, if any
    uint32_t uSettingsTimer;    //!< timeout for the peer to acknowledge our settings
    uint32_t uTimer;            //!< timeout timer
    uint32_t uTimeout;          //!< protocol timeout
    NetCritT HttpCrit;          //!< critical section for guarding update from send/recv

    //! settings based on ProtoHttp2SettingsE
    SettingsT PeerSettings;    //!< settings advertised by our peer
    SettingsT TempSettings;    //!< settings we will advertise to our peer
    SettingsT LocalSettings;   //!< settings we have advertised and were ack'd

    char strHost[256];          //!< server name
    char strBaseHost[256];      //!< base server name (used for partial urls)
    char strProxy[256];         //!< proxy server name/address (including port)
    int32_t iPort;              //!< server port
    int32_t bSecure;            //!< secure connection?
    int32_t iBasePort;          //!< base port (used for partial urls)
    int32_t bBaseSecure;        //!< base security settings (used for partial urls)

    enum
    {
        ST_IDLE,                //!< default state
        ST_CONN,                //!< connecting to the server
        ST_ACTIVE,              //!< active connection
        ST_FAIL                 //!< connection failure
    } eState;                   //!< current state of the module
    ErrorTypeE eErrorType;      //!< cached error when handling peer frames (goaway related)

    HpackRefT *pEncoder;        //!< encoder context
    HpackRefT *pDecoder;        //!< decoder context
    uint8_t bHuffman;           //!< use huffman encoding? (default=FALSE)
    uint8_t bSettingsRecv;      //!< have we received any settings from our peer?
    uint8_t bTimeout;           //!< timeout indicator
    uint8_t _pad;

    char *pAppendHdr;           //!< append header ('apnd' control) buffer
    int32_t iAppendLen;         //!< size of append header

    int32_t iNumStreams;        //!< current number of active streams
    StreamInfoT Streams[PROTOHTTP2_MAX_STREAMS];    //!< list of stream info
};

/*** Variables ********************************************************************/

//! used when establishing a connection to try to cause failure with non http2 endpoints
static const uint8_t _ProtoHttp2_ConnectionPreface[24] =
{
    0x50, 0x52, 0x49, 0x20, 0x2a, 0x20, 0x48, 0x54,
    0x54, 0x50, 0x2f, 0x32, 0x2e, 0x30, 0x0d, 0x0a,
    0x0d, 0x0a, 0x53, 0x4d, 0x0d, 0x0a, 0x0d, 0x0a
};

#if DIRTYCODE_LOGGING
//! frame types for logging purposes
static const char *_ProtoHttp2_strFrameType[] =
{
    "DATA",
    "HEADERS",
    "PRIORITY",
    "RST_STREAM",
    "SETTINGS",
    "PUSH_PROMISE",
    "PING",
    "GOAWAY",
    "WINDOW_UPDATE",
    "CONTINUATION"
};

//! settings names for logging purposes
static const char *_ProtoHttp2_strSettingName[SETTINGS_NUM] =
{
    "SETTINGS_HEADER_TABLE_SIZE",
    "SETTINGS_ENABLE_PUSH",
    "SETTINGS_MAX_CONCURRENT_STREAM",
    "SETTINGS_INITIAL_WINDOW_SIZE",
    "SETTINGS_MAX_FRAME_SIZE",
    "SETTINGS_MAX_HEADER_LIST_SIZE"
};

//! protohttp2 state for logging
static const char *_ProtoHttp2_strState[] =
{
    "ST_IDLE",
    "ST_CONN",
    "ST_ACTIVE",
    "ST_FAIL"
};
#endif

//! error types to string mapping
static const char *_ProtoHttp2_strErrorType[] =
{
    "NO_ERROR",
    "PROTOCOL_ERROR",
    "INTERNAL_ERROR",
    "FLOW_CONTROL_ERROR",
    "SETTINGS_TIMEOUT",
    "STREAM_CLOSED",
    "FRAME_SIZE_ERROR",
    "REFUSED_STREAM",
    "CANCEL",
    "COMPRESSION_ERROR",
    "CONNECT_ERROR",
    "ENHANCE_YOUR_CALM",
    "INADEQUATE_SECURITY",
    "HTTP_1_1_REQUIRED"
};

//! default settings based on rfc
static const SettingsT _ProtoHttp2_DefaultSettings =
{
    0x1000,         // SETTINGS_HEADER_TABLE_SIZE
    0x1,            // SETTINGS_ENABLE_PUSH
    0x7fffffff,     // SETTINGS_MAX_CONCURRENT_STREAM
    0xffff,         // SETTINGS_INITIAL_WINDOW_SIZE
    0x4000,         // SETTINGS_MAX_FRAME_SIZE
    0x7fffffff      // SETTINGS_MAX_HEADER_LIST_SIZE
};

//! http request names
static const char *_ProtoHttp2_strRequestNames[PROTOHTTP_NUMREQUESTTYPES] =
{
    "HEAD", "GET", "POST", "PUT", "DELETE", "OPTIONS"
};

/*** Private Functions ************************************************************/

static void _ProtoHttp2StreamInfoCleanup(ProtoHttp2RefT *pState, StreamInfoT *pStreamInfo);

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2Close

    \Description
        Closes the connection to the peer

    \Input *pState  - module state
    \Input *pReason - string representation of why we are closing the connection

    \Version 09/28/2016 (eesponda)
*/
/********************************************************************************F*/
static void _ProtoHttp2Close(ProtoHttp2RefT *pState, const char *pReason)
{
    if (ProtoSSLStat(pState->pSsl, 'stat', NULL, 0) >= 0)
    {
        NetPrintfVerbose((pState->iVerbose, 0, "protohttp2: [%p] closing connection: %s\n", pState, pReason));
        ProtoSSLDisconnect(pState->pSsl);
        pState->eState = ST_FAIL;
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2StreamCloseOnError

    \Description
        Sets the stream state to closed and fires a failure write callback

    \Input *pState      - module state
    \Input *pStreamInfo - stream information we are updating

    \Version 01/05/2017 (eesponda)
*/
/********************************************************************************F*/
static void _ProtoHttp2StreamCloseOnError(ProtoHttp2RefT *pState, StreamInfoT *pStreamInfo)
{
    // if the stream already closed nothing left to do
    if (pStreamInfo->eState == STREAMSTATE_CLOSED)
    {
        return;
    }
    pStreamInfo->eState = STREAMSTATE_CLOSED;

    // if we have a write callback, let the user know that we had an error occur
    if (pStreamInfo->pWriteCb != NULL)
    {
        ProtoHttp2WriteCbInfoT CbInfo;
        CbInfo.iStreamId = pStreamInfo->iStreamId;
        CbInfo.eRequestType = pStreamInfo->eRequestType;
        CbInfo.eRequestResponse = pStreamInfo->eResponseCode;

        pStreamInfo->pWriteCb(pState, &CbInfo, NULL, pState->bTimeout ? PROTOHTTP2_TIMEOUT : PROTOHTTP2_RECVFAIL, pStreamInfo->pUserData);
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2Send

    \Description
        Try and send some data

    \Input  *pState     - reference pointer
    \Input  *pBuf       - pointer to buffer to send from
    \Input  iBufSize    - amount of data to try and send

    \Output
        int32_t         - negative=error, else number of bytes sent

    \Version 11/02/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoHttp2Send(ProtoHttp2RefT *pState, const uint8_t *pBuf, int32_t iBufSize)
{
    int32_t iResult, iStream;

    // try and send some data
    iResult = ProtoSSLSend(pState->pSsl, (const char *)pBuf, iBufSize);
    if (iResult > 0)
    {
        #if DIRTYCODE_LOGGING
        NetPrintfVerbose((pState->iVerbose, 2, "protohttp2: [%p] sent %d bytes\n", pState, iResult));
        if (pState->iVerbose > 3)
        {
            NetPrintMem(pBuf, iResult, "http2-send");
        }
        #endif
    }
    else if (iResult < 0)
    {
        NetPrintf(("protohttp2: [%p] error %d sending %d bytes\n", pState, iResult, iBufSize));
        pState->eState = ST_FAIL;
        pState->iSslFail = ProtoSSLStat(pState->pSsl, 'fail', NULL, 0);
        pState->iHresult = ProtoSSLStat(pState->pSsl, 'hres', NULL, 0);

        // close all the streams that are currently active
        for (iStream = 0; iStream < pState->iNumStreams; iStream += 1)
        {
            _ProtoHttp2StreamCloseOnError(pState, &pState->Streams[iStream]);
        }
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2Recv

    \Description
        Try and recv some data

    \Input  *pState     - reference pointer
    \Input  *pBuf       - pointer to buffer to recv to
    \Input  iBufSize    - amount of data we can accept

    \Output
        int32_t         - negative=error, else number of bytes recv

    \Version 11/02/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoHttp2Recv(ProtoHttp2RefT *pState, uint8_t *pBuf, int32_t iBufSize)
{
    int32_t iResult, iStream;

    // try and send some data
    iResult = ProtoSSLRecv(pState->pSsl, (char *)pBuf, iBufSize);
    if (iResult > 0)
    {
        #if DIRTYCODE_LOGGING
        NetPrintfVerbose((pState->iVerbose, 2, "protohttp2: [%p] recv %d bytes\n", pState, iResult));
        if (pState->iVerbose > 3)
        {
            NetPrintMem(pBuf, iResult, "http2-recv");
        }
        #endif
    }
    else if (iResult < 0)
    {
        NetPrintf(("protohttp2: [%p] %s got ST_FAIL (err=%d)\n", pState, _ProtoHttp2_strState[pState->eState], iResult));
        pState->eState = ST_FAIL;
        pState->iSslFail = ProtoSSLStat(pState->pSsl, 'fail', NULL, 0);
        pState->iHresult = ProtoSSLStat(pState->pSsl, 'hres', NULL, 0);

        // close all the streams that are currently active
        for (iStream = 0; iStream < pState->iNumStreams; iStream += 1)
        {
            _ProtoHttp2StreamCloseOnError(pState, &pState->Streams[iStream]);
        }
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2ApplyBaseUrl

    \Description
        Apply base url elements (if set) to any url elements not specified (relative
        url support).

    \Input *pState      - module state
    \Input *pKind       - parsed http kind ("http" or "https")
    \Input *pHost       - [in/out] parsed URL host
    \Input iHostSize    - size of pHost buffer
    \Input *pPort       - [in/out] parsed port
    \Input *pSecure     - [in/out] parsed security (0 or 1)
    \Input bPortSpecified - TRUE if a port is explicitly specified in the url, else FALSE

    \Output
        uint8_t        - non-zero if changed, else zero

    \Version 02/03/2010 (jbrookes)
*/
/********************************************************************************F*/
static uint8_t _ProtoHttp2ApplyBaseUrl(ProtoHttp2RefT *pState, const char *pKind, char *pHost, int32_t iHostSize, int32_t *pPort, int32_t *pSecure, uint8_t bPortSpecified)
{
    uint8_t bChanged = FALSE;
    if ((*pHost == '\0') && (pState->strBaseHost[0] != '\0'))
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttp2: [%p] host not present; setting to %s\n", pState, pState->strBaseHost));
        ds_strnzcpy(pHost, pState->strBaseHost, iHostSize);
        bChanged = TRUE;
    }
    if ((bPortSpecified == FALSE) && (pState->iBasePort != 0))
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttp2: [%p] port not present; setting to %d\n", pState, pState->iBasePort));
        *pPort = pState->iBasePort;
        bChanged = TRUE;
    }
    if (*pKind == '\0')
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttp2: [%p] kind (protocol) not present; setting to %d\n", pState, pState->bBaseSecure));
        *pSecure = pState->bBaseSecure;
        // if our port setting is default and incompatible with our security setting, override it
        if (((*pPort == 80) && (*pSecure == TRUE)) || ((*pPort == 443) && (*pSecure == FALSE)))
        {
            *pPort = *pSecure ? 443 : 80;
        }
        bChanged = TRUE;
    }
    return(bChanged);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2Reset

    \Description
        Reset the internal state to before we connected to a peer

    \Input *pState  - module state

    \Version 10/31/2016 (eesponda)
*/
/********************************************************************************F*/
static void _ProtoHttp2Reset(ProtoHttp2RefT *pState)
{
    int32_t iStream;

    // set peer's default setting values (these are overwritten on connection establishment)
    ds_memcpy(&pState->PeerSettings, &_ProtoHttp2_DefaultSettings, sizeof(pState->PeerSettings));
    // set my local default setting values
    ds_memcpy(&pState->LocalSettings, &_ProtoHttp2_DefaultSettings, sizeof(pState->LocalSettings));

    // set my updated local settings that we will advertise during connection establishment
    ds_memcpy(&pState->TempSettings, &_ProtoHttp2_DefaultSettings, sizeof(pState->TempSettings));
    pState->TempSettings.uEnablePush = 0;          // tell the peer that it cannot push

    /* tell peer the size of header we are willing to support (uncompressed)
       note: if the size is larger than the size of max header list you will
             need to look into supporting continuation frames */
    pState->TempSettings.uMaxHeaderListSize = PROTOHTTP2_HDRCACHESIZE;

    // update the size of the recv window
    pState->iLocalWindow = pState->TempSettings.uInitialWindowSize = PROTOHTTP2_WINDOWSIZE;
    pState->iPeerWindow = pState->PeerSettings.uInitialWindowSize;

    // update the maximum frame size based on our input buffer
    if (pState->TempSettings.uMaxFrameSize < (uint32_t)(pState->iInpMax-PROTOHTTP2_RESERVED_SIZE))
    {
        pState->TempSettings.uMaxFrameSize = (uint32_t)(pState->iInpMax-PROTOHTTP2_RESERVED_SIZE);
    }

    // clear the dynamic tables
    HpackClear(pState->pDecoder);
    HpackClear(pState->pEncoder);

    // set the stream id to 1 (initial state)
    pState->iStreamId = 1;

    // clean up the number of tracked streams
    for (iStream = 0; iStream < pState->iNumStreams; iStream += 1)
    {
        _ProtoHttp2StreamInfoCleanup(pState, &pState->Streams[iStream]);
    }
    pState->iNumStreams = 0;

    // set the default state
    pState->eState = ST_IDLE;
    pState->eErrorType = ERRORTYPE_NO_ERROR;

    // reset the sizes of the buffers
    pState->iOutLen = pState->iInpLen = 0;
    // reset the offset
    pState->iOutOff = 0;

    // reset variables dealing with settings
    pState->bSettingsRecv = FALSE;
    pState->uSettingsTimer = 0;
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2StreamInfoGet

    \Description
        Retrieves the stream information by identifier

    \Input *pState      - module state
    \Input iStreamId    - identifier used for the lookup

    \Output
        StreamInfoT *   - pointer to stream information or NULL if not found

    \Version 10/26/2016 (eesponda)
*/
/********************************************************************************F*/
static StreamInfoT *_ProtoHttp2StreamInfoGet(ProtoHttp2RefT *pState, int32_t iStreamId)
{
    StreamInfoT *pStreamInfo = NULL;
    int32_t iStream;

    // iterate through stream info and look for a match
    for (iStream = 0; iStream < pState->iNumStreams; iStream += 1)
    {
        if (pState->Streams[iStream].iStreamId == iStreamId)
        {
            pStreamInfo = &pState->Streams[iStream];
            break;
        }
    }

    // return info to caller, or NULL if no match
    return(pStreamInfo);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2StreamInfoCleanup

    \Description
        Cleanup any dynamic memory consumed by the stream info struct

    \Input *pState      - module state
    \Input *pStreamInfo - pointer to the structure needing cleanup

    \Version 10/26/2016 (eesponda)
*/
/********************************************************************************F*/
static void _ProtoHttp2StreamInfoCleanup(ProtoHttp2RefT *pState, StreamInfoT *pStreamInfo)
{
    if (pStreamInfo->pHeader != NULL)
    {
        DirtyMemFree(pStreamInfo->pHeader, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        pStreamInfo->pHeader = NULL;
    }
    if (pStreamInfo->pRequestHeader != NULL)
    {
        DirtyMemFree(pStreamInfo->pRequestHeader, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        pStreamInfo->pRequestHeader = NULL;
    }
    if (pStreamInfo->pData != NULL)
    {
        DirtyMemFree(pStreamInfo->pData, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        pStreamInfo->pData = NULL;
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2DecodeHeader

    \Description
        Decodes the header of a http2 frame

    \Input *pState  - module state
    \Input *pBuf    - the buffer we are pasing the data out of
    \Input iBufLen  - the size of the buffer
    \Input *pHeader - [out] location we are parsing into

    \Output
        const uint8_t *   - the new location of the buffer after parsing or NULL

    \Version 09/28/2016 (eesponda)
*/
/********************************************************************************F*/
static const uint8_t *_ProtoHttp2DecodeFrameHeader(ProtoHttp2RefT *pState, const uint8_t *pBuf, int32_t iBufLen, FrameHeaderT *pHeader)
{
    // make sure we can parse the whole header
    if (iBufLen < PROTOHTTP2_HEADER_SIZE)
    {
        return(NULL);
    }

    // decode 24-bit length
    pHeader->uLength      = *pBuf++ << 16;
    pHeader->uLength     |= *pBuf++ <<  8;
    pHeader->uLength     |= *pBuf++;
    // decode 8-bit type
    pHeader->uType        = *pBuf++;
    // decode 8-bit flags
    pHeader->uFlags       = *pBuf++;
    // decode 31-bit stream id (ignoring the high bit)
    pHeader->iStreamId    = (*pBuf++ << 24) & 0x7f;
    pHeader->iStreamId   |= *pBuf++ << 16;
    pHeader->iStreamId   |= *pBuf++ << 8;
    pHeader->iStreamId   |= *pBuf++;

    /* decode 8-bit pad length if present, add 1 for the length field itself
       this is technically part of the frame payload but we do it here at a
       central location */
    if ((pHeader->uFlags & PROTOHTTP2_FLAG_PADDED) != 0)
    {
        pHeader->uPadding = (*pBuf++) + 1;
        pHeader->uLength -= pHeader->uPadding;
    }
    else
    {
        pHeader->uPadding = 0;
    }

    // validate the stream id of the frame
    if ((pHeader->iStreamId == 0) && (pHeader->uType != FRAMETYPE_SETTINGS) && (pHeader->uType != FRAMETYPE_PING) && (pHeader->uType != FRAMETYPE_GOAWAY) && (pHeader->uType != FRAMETYPE_WINDOW_UPDATE))
    {
        NetPrintf(("protohttp2: [%p] received frame with stream id 0 which type requires an associated stream, closing connection\n", pState));
        pState->eErrorType = ERRORTYPE_PROTOCOL_ERROR;
        return(NULL);
    }
    // validate the frame header size
    if ((pHeader->uLength > pState->LocalSettings.uMaxFrameSize) && ((pHeader->iStreamId == 0) || (pHeader->uType == FRAMETYPE_HEADERS)))
    {
        NetPrintf(("protohttp2: [%p] received frame with invalid size on connection impacting frame, closing the connection\n", pState));
        pState->eErrorType = ERRORTYPE_FRAME_SIZE_ERROR;
        return(NULL);
    }
    if ((pHeader->uType == FRAMETYPE_SETTINGS) && ((pHeader->uLength % PROTOHTTP2_SETTING_SIZE) != 0))
    {
        NetPrintf(("protohttp2: [%p] received settings frame with invalid size, closing the connection\n", pState));
        pState->eErrorType = ERRORTYPE_FRAME_SIZE_ERROR;
        return(NULL);
    }
    // validate that padding isn't present on frames that don't support it
    if ((pHeader->uPadding > 0) && ((pHeader->uType != FRAMETYPE_HEADERS) && (pHeader->uType != FRAMETYPE_DATA) && (pHeader->uType != FRAMETYPE_PUSH_PROMISE)))
    {
        NetPrintf(("protohttp2: [%p] received padding in %s frame that doesn't support it, closing connection\n", pState, _ProtoHttp2_strFrameType[pHeader->uType]));
        pState->eErrorType = ERRORTYPE_PROTOCOL_ERROR;
        return(NULL);
    }

    // make sure the whole frame has arrived
    if ((iBufLen-PROTOHTTP2_HEADER_SIZE) < (int32_t)pHeader->uLength)
    {
        return(NULL);
    }

    #if DIRTYCODE_LOGGING
    if (pHeader->uType <= FRAMETYPE_LAST)
    {
        NetPrintfVerbose((pState->iVerbose, 2, "protohttp2: [%p] received frame (len=%u, type=%s, flags=%u, stream=0x%08x, padding=%u)\n",
            pState, pHeader->uLength, _ProtoHttp2_strFrameType[pHeader->uType], pHeader->uFlags, pHeader->iStreamId, pHeader->uPadding));
    }
    #endif
    return(pBuf);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2EncodeFrameHeader

    \Description
        Encodes the header of a http2 frame

    \Input *pState  - module state
    \Input *pBuf    - the buffer we are writing the data to
    \Input *pHeader - the header data we are writing

    \Version 09/29/2016 (eesponda)
*/
/********************************************************************************F*/
static void _ProtoHttp2EncodeFrameHeader(ProtoHttp2RefT *pState, uint8_t *pBuf, const FrameHeaderT *pHeader)
{
    // encode 24-bit length
    *pBuf++ = (uint8_t)(pHeader->uLength >> 16);
    *pBuf++ = (uint8_t)(pHeader->uLength >> 8);
    *pBuf++ = (uint8_t)(pHeader->uLength);
    // encode 8-bit type
    *pBuf++ = pHeader->uType;
    // encode 8-bit flags
    *pBuf++ = pHeader->uFlags;
    // encode 31-bit stream id
    *pBuf++ = (uint8_t)(pHeader->iStreamId >> 24);
    *pBuf++ = (uint8_t)(pHeader->iStreamId >> 16);
    *pBuf++ = (uint8_t)(pHeader->iStreamId >> 8);
    *pBuf++ = (uint8_t)(pHeader->iStreamId);

    NetPrintfVerbose((pState->iVerbose, 2, "protohttp2: [%p] encoding frame (len=%u, type=%s, flags=%u, stream=0x%08x)\n",
        pState, pHeader->uLength, _ProtoHttp2_strFrameType[pHeader->uType], pHeader->uFlags, pHeader->iStreamId));
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2DecodeSettings

    \Description
        Decodes the settings frame and saves the settings

    \Input *pState  - module state
    \Input *pBuf    - the buffer we are parsing the data from
    \Input *pHeader - the header data for the frame

    \Version 09/29/2016 (eesponda)
*/
/********************************************************************************F*/
static void _ProtoHttp2DecodeSettings(ProtoHttp2RefT *pState, const uint8_t *pBuf, const FrameHeaderT *pHeader)
{
    // if we received an ACK, commit our temp settings
    if ((pHeader->uFlags & PROTOHTTP2_FLAG_ACK) != 0)
    {
        ds_memcpy(&pState->LocalSettings, &pState->TempSettings, sizeof(pState->LocalSettings));
    }
    else
    {
        int32_t iOffset;
        for (iOffset = 0; iOffset < (int32_t)pHeader->uLength; iOffset += PROTOHTTP2_SETTING_SIZE)
        {
            uint16_t uKey;
            uint32_t uValue;

            // decode settings key
            uKey     = *pBuf++ << 8;
            uKey    |= *pBuf++;
            // decode settings value
            uValue   = *pBuf++ << 24;
            uValue  |= *pBuf++ << 16;
            uValue  |= *pBuf++ << 8;
            uValue  |= *pBuf++;

            switch (uKey)
            {
                case SETTINGS_ENABLE_PUSH: pState->PeerSettings.uEnablePush = uValue; break;
                case SETTINGS_HEADER_TABLE_SIZE: pState->PeerSettings.uHeaderTableSize = uValue; break;
                case SETTINGS_INITIAL_WINDOW_SIZE: pState->PeerSettings.uInitialWindowSize = uValue; break;
                case SETTINGS_MAX_CONCURRENT_STREAM: pState->PeerSettings.uMaxConcurrentStream = uValue; break;
                case SETTINGS_MAX_FRAME_SIZE: pState->PeerSettings.uMaxFrameSize = uValue; break;
                case SETTINGS_MAX_HEADER_LIST_SIZE: pState->PeerSettings.uMaxHeaderListSize = uValue; break;
                default: NetPrintf(("protohttp2: [%p] unhandled settings key %u\n", pState, uKey)); break;
            }
            if ((uKey >= SETTINGS_HEADER_TABLE_SIZE) && (uKey <= SETTINGS_MAX_HEADER_LIST_SIZE))
            {
                NetPrintfVerbose((pState->iVerbose, 1, "protohttp2: [%p] received setting (%s=%u)\n",
                    pState, _ProtoHttp2_strSettingName[uKey-1], uValue));
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2WriteSetting

    \Description
        Writes a setting into the payload

    \Input *pState          - module state
    \Input *pBuf            - [out] the buffer we are writing the data to
    \Input *pHeader         - the frame header we are updating while writing
    \Input uSettingKey      - the key for the setting
    \Input uSettingValue    - the value for the setting

    \Output
        int32_t             - amount of data written

    \Version 10/24/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoHttp2WriteSetting(ProtoHttp2RefT *pState, uint8_t *pBuf, FrameHeaderT *pHeader, uint16_t uSettingKey, uint32_t uSettingValue)
{
    pHeader->uLength += PROTOHTTP2_SETTING_SIZE;

    *pBuf++ = (uint8_t)(uSettingKey >> 8);
    *pBuf++ = (uint8_t)(uSettingKey);
    *pBuf++ = (uint8_t)(uSettingValue >> 24);
    *pBuf++ = (uint8_t)(uSettingValue >> 16);
    *pBuf++ = (uint8_t)(uSettingValue >> 8);
    *pBuf++ = (uint8_t)(uSettingValue);

    NetPrintfVerbose((pState->iVerbose, 1, "protohttp2: [%p] encode setting (%s=%u)\n",
        pState, _ProtoHttp2_strSettingName[uSettingKey-1], uSettingValue));

    return(PROTOHTTP2_SETTING_SIZE);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2EncodeSettings

    \Description
        Encodes the a settings frame

    \Input *pState  - module state
    \Input bAck     - are we acknowledging or updating settings?
    \Input *pBuf    - [out] the buffer we are writing the data to
    \Input iBufLen  - the length of the buffer

    \Output
        int32_t     - amount of data written

    \Version 09/29/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoHttp2EncodeSettings(ProtoHttp2RefT *pState, uint8_t bAck, uint8_t *pBuf, int32_t iBufLen)
{
    FrameHeaderT Header;
    uint8_t *pFrameHeader;

    // make sure we have enough room for the maximum amount of settings
    if (iBufLen < (PROTOHTTP2_HEADER_SIZE + (PROTOHTTP2_SETTING_SIZE * SETTINGS_NUM)))
    {
        return(0);
    }

    // setup defaults
    ds_memclr(&Header, sizeof(Header));
    Header.uType = FRAMETYPE_SETTINGS;

    // save the header location and advance the buffer
    pFrameHeader = pBuf;
    pBuf += PROTOHTTP2_HEADER_SIZE;

    // if we need to acknowledge the peer's setting and our settings have not changed set the appropriate flags
    if (bAck == TRUE)
    {
        Header.uFlags |= PROTOHTTP2_FLAG_ACK;
    }
    // otherwise, send our settings to the peer
    else
    {
        if (pState->TempSettings.uHeaderTableSize != pState->LocalSettings.uHeaderTableSize)
        {
            pBuf += _ProtoHttp2WriteSetting(pState, pBuf, &Header, SETTINGS_HEADER_TABLE_SIZE, pState->TempSettings.uHeaderTableSize);
        }
        if (pState->TempSettings.uEnablePush != pState->LocalSettings.uEnablePush)
        {
            pBuf += _ProtoHttp2WriteSetting(pState, pBuf, &Header, SETTINGS_ENABLE_PUSH, pState->TempSettings.uEnablePush);
        }
        if (pState->TempSettings.uMaxConcurrentStream != pState->LocalSettings.uMaxConcurrentStream)
        {
            pBuf += _ProtoHttp2WriteSetting(pState, pBuf, &Header, SETTINGS_MAX_CONCURRENT_STREAM, pState->TempSettings.uMaxConcurrentStream);
        }
        if (pState->TempSettings.uInitialWindowSize != pState->LocalSettings.uInitialWindowSize)
        {
            pBuf += _ProtoHttp2WriteSetting(pState, pBuf, &Header, SETTINGS_INITIAL_WINDOW_SIZE, pState->TempSettings.uInitialWindowSize);
        }
        if (pState->TempSettings.uMaxFrameSize != pState->LocalSettings.uMaxFrameSize)
        {
            pBuf += _ProtoHttp2WriteSetting(pState, pBuf, &Header, SETTINGS_MAX_FRAME_SIZE, pState->TempSettings.uMaxFrameSize);
        }
        if (pState->TempSettings.uMaxHeaderListSize != pState->LocalSettings.uMaxHeaderListSize)
        {
            pBuf += _ProtoHttp2WriteSetting(pState, pBuf, &Header, SETTINGS_MAX_HEADER_LIST_SIZE, pState->TempSettings.uMaxHeaderListSize);
        }

        // ensure we actually wrote any settings
        if (Header.uLength == 0)
        {
            return(0);
        }
    }

    // encode the frame header, by this point we know we have enough space
    _ProtoHttp2EncodeFrameHeader(pState, pFrameHeader, &Header);

    return(Header.uLength+PROTOHTTP2_HEADER_SIZE);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2EncodeHeaders

    \Description
        Encodes the a headers frame

    \Input *pState      - module state
    \Input *pStreamInfo - information about the stream (id, etc)
    \Input bEndStream   - does this request contain data?
    \Input *pHeader     - the header we are using to encode

    \Output
        int32_t         - zero=success, negative=failure

    \Version 10/26/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoHttp2EncodeHeaders(ProtoHttp2RefT *pState, StreamInfoT *pStreamInfo, uint8_t bEndStream, const char *pHeader)
{
    FrameHeaderT Header;
    uint8_t *pFrameHeader, *pBuf;
    int32_t iBufMax, iBufLen, iResult;

    pBuf = pState->pOutBuf+pState->iOutLen;
    iBufLen = PROTOHTTP2_CalculateFreeSpace(pState);
    iBufMax = iBufLen-PROTOHTTP2_HEADER_SIZE;

    // make sure we at least have enough space for frame header
    if (iBufMax <= 0)
    {
        return(PROTOHTTP2_MINBUFF);
    }

    // setup the frame header
    ds_memclr(&Header, sizeof(Header));
    Header.uType = FRAMETYPE_HEADERS;
    Header.iStreamId = pStreamInfo->iStreamId;
    Header.uFlags |= PROTOHTTP2_FLAG_END_HEADERS;
    if (bEndStream == TRUE)
    {
        Header.uFlags |= PROTOHTTP2_FLAG_END_STREAM;
    }

    // save the header location and advance the buffer
    pFrameHeader = pBuf;
    pBuf += PROTOHTTP2_HEADER_SIZE;

    // encode the header info the buffer
    iResult = HpackEncode(pState->pEncoder, pHeader, pBuf, iBufMax, pState->bHuffman);
    if ((iResult > 0) && (iResult < iBufMax))
    {
        Header.uLength = (uint32_t)iResult;

        // encode the frame header, by this point we know we have enough space
        _ProtoHttp2EncodeFrameHeader(pState, pFrameHeader, &Header);

        // update the stream state
        if (pStreamInfo->eState == STREAMSTATE_IDLE)
        {
            pStreamInfo->eState = (bEndStream == TRUE) ? STREAMSTATE_HALF_CLOSED_LOCAL : STREAMSTATE_OPEN;
        }

        // advance the buffer
        pState->iOutLen += Header.uLength+PROTOHTTP2_HEADER_SIZE;
        return(0);
    }

    return(-1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2EncodeData

    \Description
        Encodes the a data frame

    \Input *pState      - module state
    \Input *pStreamInfo - information about the stream (id, etc)
    \Input *pInput      - the payload data we are sending
    \Input iInpLen      - the length of the payload data
    \Input bEndStream   - will we be sending any more data frames?

    \Output
        int32_t     - amount of data written

    \Version 10/26/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoHttp2EncodeData(ProtoHttp2RefT *pState, StreamInfoT *pStreamInfo, const uint8_t *pInput, int32_t iInpLen, uint8_t bEndStream)
{
    FrameHeaderT Header;
    uint8_t *pOutput = pState->pOutBuf+pState->iOutLen;
    int32_t iOutLen = PROTOHTTP2_CalculateFreeSpace(pState);

    // make sure we have enough space to encode the frame header plus some more
    if (iOutLen <= PROTOHTTP2_HEADER_SIZE)
    {
        return(PROTOHTTP2_MINBUFF);
    }
    iOutLen -= PROTOHTTP2_HEADER_SIZE;

    /* if we cannot fit the whole user payload, then fill the remainder of the buffer
       don't set the end stream flag as more data needs to be sent */
    if (iInpLen > iOutLen)
    {
        iInpLen = iOutLen;
        bEndStream = FALSE;
    }
    // make sure we have enough space in the window
    if ((pState->iPeerWindow-iInpLen < 0) || (pStreamInfo->iPeerWindow-iInpLen < 0))
    {
        return(0);
    }

    // setup the frame header
    ds_memclr(&Header, sizeof(Header));
    Header.uType = FRAMETYPE_DATA;
    Header.iStreamId = pStreamInfo->iStreamId;
    Header.uLength = iInpLen;

    // end the stream if necessary
    if (bEndStream == TRUE)
    {
        Header.uFlags |= PROTOHTTP2_FLAG_END_STREAM;
    }

    // encode the frame header
    _ProtoHttp2EncodeFrameHeader(pState, pOutput, &Header);
    pOutput += PROTOHTTP2_HEADER_SIZE;

    // encode the data
    ds_memcpy_s(pOutput, iOutLen, pInput, iInpLen);

    // update the stream state
    if (bEndStream == TRUE)
    {
        if (pStreamInfo->eState == STREAMSTATE_OPEN)
        {
            pStreamInfo->eState = STREAMSTATE_HALF_CLOSED_LOCAL;
        }
        else if (pStreamInfo->eState == STREAMSTATE_HALF_CLOSED_REMOTE)
        {
            pStreamInfo->eState = STREAMSTATE_CLOSED;
        }
    }

    // advance the windows
    pState->iPeerWindow -= iInpLen;
    pStreamInfo->iPeerWindow -= iInpLen;

    // advance buffer
    pState->iOutLen += iInpLen+PROTOHTTP2_HEADER_SIZE;

    // return amount of user payload data written to the output buffer
    return(iInpLen);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2EncodeWindowUpdate

    \Description
        Encodes the a window update frame

    \Input *pState      - module state
    \Input iStreamId    - stream we are sending the window update for
    \Input iIncrement   - window increment we are sending for the stream
    \Input *pWindow     - [out] window we are incrementing

    \Output
        int32_t         - zero=success, negative=error

    \Notes
        StreamId of 0 is reserved for updating the window of the entire
        connection

    \Version 10/26/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoHttp2EncodeWindowUpdate(ProtoHttp2RefT *pState, int32_t iStreamId, int32_t iIncrement, int32_t *pWindow)
{
    FrameHeaderT Header;
    const int32_t iPayloadSize = PROTOHTTP2_WINDOW_UPDATE_SIZE+PROTOHTTP2_HEADER_SIZE;
    uint8_t *pBuf = pState->pOutBuf+pState->iOutLen;
    int32_t iBufLen = pState->iOutMax-pState->iOutLen;

    // make sure we have enough space to encode
    if (iBufLen < iPayloadSize)
    {
        return(PROTOHTTP2_MINBUFF);
    }

    // setup the frame header
    ds_memclr(&Header, sizeof(Header));
    Header.uType = FRAMETYPE_WINDOW_UPDATE;
    Header.iStreamId = iStreamId;
    Header.uLength = PROTOHTTP2_WINDOW_UPDATE_SIZE;

    // encode the frame header
    _ProtoHttp2EncodeFrameHeader(pState, pBuf, &Header);
    pBuf += PROTOHTTP2_HEADER_SIZE;

    // encode the data
    *pBuf++ = (uint8_t)(iIncrement >> 24);
    *pBuf++ = (uint8_t)(iIncrement >> 16);
    *pBuf++ = (uint8_t)(iIncrement >> 8);
    *pBuf++ = (uint8_t)(iIncrement);

    NetPrintfVerbose((pState->iVerbose, 2, "protohttp2: [%p] incrementing the window (stream=0x%08x, size=%d)\n", pState, iStreamId, iIncrement));
    *pWindow += iIncrement;

    // advance the buffer
    pState->iOutLen += iPayloadSize;

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2EncodePing

    \Description
        Encodes a ping frame

    \Input *pState      - module state
    \Input bAck         - are we acknowledging a ping?
    \Input *pInput      - the opaque data we send
    \Input iInpLen      - the size of the opaque data

    \Output
        int32_t         - zero=success, negative=error

    \Version 10/26/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoHttp2EncodePing(ProtoHttp2RefT *pState, uint8_t bAck, const uint8_t *pInput, int32_t iInpLen)
{
    FrameHeaderT Header;
    const int32_t iPayloadSize = iInpLen+PROTOHTTP2_HEADER_SIZE;
    uint8_t *pOutput = pState->pOutBuf+pState->iOutLen;
    int32_t iOutLen = pState->iOutMax-pState->iOutLen;

    // make sure the input is the correct size
    if (iInpLen != PROTOHTTP2_PING_SIZE)
    {
        NetPrintf(("protohttp2: [%p] wrong size of data given for ping\n", pState));
        return(-1);
    }
    // make sure we have enough space to encode
    if (iOutLen < iPayloadSize)
    {
        return(PROTOHTTP2_MINBUFF);
    }

    // setup the frame header
    ds_memclr(&Header, sizeof(Header));
    Header.uType = FRAMETYPE_PING;
    Header.uLength = iInpLen;
    if (bAck == TRUE)
    {
        Header.uFlags |= PROTOHTTP2_FLAG_ACK;
    }

    // encode the frame header
    _ProtoHttp2EncodeFrameHeader(pState, pOutput, &Header);
    pOutput += PROTOHTTP2_HEADER_SIZE;

    // encode the opaque data
    ds_memcpy(pOutput, pInput, iInpLen);
    // advance the buffer
    pState->iOutLen += iPayloadSize;

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2EncodeGoAway

    \Description
        Encodes a goaway frame

    \Input *pState      - module state
    \Input uErrorCode   - error code being sent with frame

    \Notes
        This function cannot fail since we know that a goaway frame will always
        fit within our output buffer. Since we are closing the connection right
        after we can disregard any data already encoded there within.

    \Version 10/31/2016 (eesponda)
*/
/********************************************************************************F*/
static void _ProtoHttp2EncodeGoAway(ProtoHttp2RefT *pState, uint32_t uErrorCode)
{
    FrameHeaderT Header;
    uint8_t *pBuf = pState->pOutBuf;
    const int32_t iPayloadSize = PROTOHTTP2_GOAWAY_SIZE+PROTOHTTP2_HEADER_SIZE;
    // the variable points to the next stream id so deduct 2 to get last assigned stream id
    const int32_t iLastStreamId = pState->iStreamId - 2;

    // setup the frame header
    ds_memclr(&Header, sizeof(Header));
    Header.uType = FRAMETYPE_GOAWAY;
    Header.uLength = PROTOHTTP2_GOAWAY_SIZE;

    // encode the frame header
    _ProtoHttp2EncodeFrameHeader(pState, pBuf, &Header);
    pBuf += PROTOHTTP2_HEADER_SIZE;

    // encode last-stream-id
    *pBuf++ = (uint8_t)(iLastStreamId >> 24);
    *pBuf++ = (uint8_t)(iLastStreamId >> 16);
    *pBuf++ = (uint8_t)(iLastStreamId >> 8);
    *pBuf++ = (uint8_t)(iLastStreamId);
    // encode error code
    *pBuf++ = (uint8_t)(uErrorCode >> 24);
    *pBuf++ = (uint8_t)(uErrorCode >> 16);
    *pBuf++ = (uint8_t)(uErrorCode >> 8);
    *pBuf++ = (uint8_t)(uErrorCode);

    pState->iOutLen = iPayloadSize;
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2EncodeRstStream

    \Description
        Encodes a rst_stream frame

    \Input *pState      - module state
    \Input iStreamId    - the stream we are resetting
    \Input uErrorCode   - the error that caused the reset

    \Output
        int32_t         - zero=success, negative=failure

    \Version 11/03/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoHttp2EncodeRstStream(ProtoHttp2RefT *pState, int32_t iStreamId, uint32_t uErrorCode)
{
    FrameHeaderT Header;
    const int32_t iPayloadSize = PROTOHTTP2_RST_STREAM_SIZE+PROTOHTTP2_HEADER_SIZE;
    uint8_t *pBuf = pState->pOutBuf+pState->iOutLen;
    int32_t iBufSize = pState->iOutMax-pState->iOutLen;

    // make sure we have enough space to encode
    if (iBufSize < iPayloadSize)
    {
        return(PROTOHTTP2_MINBUFF);
    }

    // setup the frame header
    ds_memclr(&Header, sizeof(Header));
    Header.uType = FRAMETYPE_RST_STREAM;
    Header.uLength = PROTOHTTP2_RST_STREAM_SIZE;
    Header.iStreamId = iStreamId;

    // encode the frame header
    _ProtoHttp2EncodeFrameHeader(pState, pBuf, &Header);
    pBuf += PROTOHTTP2_HEADER_SIZE;

    // encode the error code
    *pBuf++ = (uint8_t)(uErrorCode >> 24);
    *pBuf++ = (uint8_t)(uErrorCode >> 16);
    *pBuf++ = (uint8_t)(uErrorCode >> 8);
    *pBuf++ = (uint8_t)(uErrorCode);

    // advance the buffer
    pState->iOutLen += iPayloadSize;

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2SendGoAway

    \Description
        Sends goaway frame to peer to signal closing the connection

    \Input *pState      - module state
    \Input eErrorType   - the error we are sending in the goaway
    \Input *pReason     - logging reason we are closing the connection

    \Version 10/31/2016 (eesponda)
*/
/********************************************************************************F*/
static void _ProtoHttp2SendGoAway(ProtoHttp2RefT *pState, ErrorTypeE eErrorType, const char *pReason)
{
    // only send this error if still connected
    if (ProtoSSLStat(pState->pSsl, 'stat', NULL, 0) >= 0)
    {
        int32_t iStream;

        // encode and send go away
        _ProtoHttp2EncodeGoAway(pState, eErrorType);
        _ProtoHttp2Send(pState, pState->pOutBuf, pState->iOutLen);

        // close all the streams that are currently active
        for (iStream = 0; iStream < pState->iNumStreams; iStream += 1)
        {
            _ProtoHttp2StreamCloseOnError(pState, &pState->Streams[iStream]);
        }

        // close the connection
        _ProtoHttp2Close(pState, pReason);
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2PrepareRstStream

    \Description
        Prepares the rst_stream to be sent on the next update

    \Input *pState      - module state
    \Input *pStreamInfo - information about stream we are resetting
    \Input eErrorType   - reason for closing the connection
    \Input *pMessage    - logging message about why we are resetting

    \Version 11/21/2016 (eesponda)
*/
/********************************************************************************F*/
static void _ProtoHttp2PrepareRstStream(ProtoHttp2RefT *pState, StreamInfoT *pStreamInfo, ErrorTypeE eErrorType, const char *pMessage)
{
    NetPrintf(("protohttp2: [%p] %s, sending %s to peer\n", pState, pMessage, _ProtoHttp2_strErrorType[eErrorType]));
    if (_ProtoHttp2EncodeRstStream(pState, pStreamInfo->iStreamId, eErrorType) != 0)
    {
        NetPrintf(("protohttp2: [%p] not enough space in output buffer to encode rst_stream\n", pState));
    }
    _ProtoHttp2StreamCloseOnError(pState, pStreamInfo);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2ParseHeader

    \Description
        Decodes and parses the header frame information

    \Input *pState      - module state
    \Input *pStreamInfo - stream information
    \Input *pBuf        - the buffer we are parsing the data from
    \Input iBufLen      - length of the buffer

    \Output
        int32_t         - success=zero, failure=negative

    \Version 10/27/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoHttp2ParseHeader(ProtoHttp2RefT *pState, StreamInfoT *pStreamInfo, const uint8_t *pBuf, int32_t iBufLen)
{
    char strValue[128], *pHeader;
    int32_t iResult, iHeaderMax;
    const uint8_t bParse = pStreamInfo->iHeaderLen == 0; /* if this is the first header, we need to parse */

    // allocate memory for uncompressed headers
    if ((pStreamInfo->pHeader == NULL) && ((pStreamInfo->pHeader = (char *)DirtyMemAlloc(pState->LocalSettings.uMaxHeaderListSize, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData)) == NULL))
    {
        NetPrintf(("protohttp2: [%p] could not allocate space for header list\n", pState));
        return(-1);
    }
    // point to the memory we are updating
    pHeader = pStreamInfo->pHeader+pStreamInfo->iHeaderLen;
    iHeaderMax = pState->LocalSettings.uMaxHeaderListSize-pStreamInfo->iHeaderLen;

    // decode the header and write the information to our cache
    if ((iResult = HpackDecode(pState->pDecoder, pBuf, iBufLen, pHeader, iHeaderMax)) >= 0)
    {
        pStreamInfo->iHeaderLen += iResult;
    }
    else
    {
        NetPrintf(("protohttp2: [%p] could not decode the incoming headers\n", pState));
        return(-1);
    }

    /* if we already received an initial header, we don't need to reparse this data as
       we are just handling trailing header fields after the last data chunk.
       these fields _should_ not contain any of the headers we are parsing for */
    if (bParse == TRUE)
    {
        // parse header code
        pStreamInfo->eResponseCode = ProtoHttpParseHeaderCode(pStreamInfo->pHeader);

        // parse content-length field
        if (ProtoHttpGetHeaderValue(pState, pStreamInfo->pHeader, "content-length", strValue, sizeof(strValue), NULL) != -1)
        {
            pStreamInfo->iBodySize = ds_strtoll(strValue, NULL, 10);
        }
        else
        {
            pStreamInfo->iBodySize = -1;
        }

        // parse last-modified header
        if (ProtoHttpGetHeaderValue(pState, pStreamInfo->pHeader, "last-modified", strValue, sizeof(strValue), NULL) != -1)
        {
            pStreamInfo->iHdrDate = (int32_t)ds_strtotime(strValue);
        }

        // if this is a redirect, we don't support it log something
        if (PROTOHTTP_GetResponseClass(pStreamInfo->eResponseCode) == PROTOHTTP_RESPONSE_REDIRECTION)
        {
            NetPrintf(("protohttp2: [%p] auto-redirection not supported, if you want to follow the redirect please issue a new request\n", pState));
        }

        NetPrintfVerbose((pState->iVerbose, 0, "protohttp2: [%p] received %d response (%d bytes)\n", pState, pStreamInfo->eResponseCode, pStreamInfo->iHeaderLen));
    }

    #if DIRTYCODE_LOGGING
    if (pState->iVerbose > 1)
    {
        NetPrintf(("protohttp2: [%p] received response header:\n", pState));
        NetPrintWrap(pHeader, 80);
    }
    #endif

    // if we have a receive header callback, invoke it
    if (pState->pReceiveHeaderCb != NULL)
    {
        pState->pReceiveHeaderCb(pState, pStreamInfo->iStreamId, pHeader, iResult, pState->pCallbackRef);
    }

    // header successfully parsed
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2HandleData

    \Description
        Handles the incoming data frame

    \Input *pState      - module state
    \Input *pBuf        - the buffer we are parsing the data from
    \Input *pHeader     - the header data for the frame
    \Input *pStreamInfo - information about associated stream

    \Version 11/04/2016 (eesponda)
*/
/********************************************************************************F*/
static void _ProtoHttp2HandleData(ProtoHttp2RefT *pState, const uint8_t *pBuf, const FrameHeaderT *pHeader, StreamInfoT *pStreamInfo)
{
    uint8_t bEndStream = FALSE;

    // check the state of the stream to make sure we are in correct state
    if ((pStreamInfo->eState != STREAMSTATE_OPEN) && (pStreamInfo->eState != STREAMSTATE_HALF_CLOSED_LOCAL))
    {
        _ProtoHttp2PrepareRstStream(pState, pStreamInfo, ERRORTYPE_STREAM_CLOSED, "received data frame when in unexpected state");
        return;
    }

    // update the stream data based on the flags
    if ((pHeader->uFlags & PROTOHTTP2_FLAG_END_STREAM) != 0)
    {
        if (pStreamInfo->eState == STREAMSTATE_OPEN)
        {
            pStreamInfo->eState = STREAMSTATE_HALF_CLOSED_REMOTE;
        }
        else if (pStreamInfo->eState == STREAMSTATE_HALF_CLOSED_LOCAL)
        {
            pStreamInfo->eState = STREAMSTATE_CLOSED;
        }
        bEndStream = TRUE;
    }

    // if we have a write callback, invoke it
    if (pStreamInfo->pWriteCb != NULL)
    {
        ProtoHttp2WriteCbInfoT CbInfo;
        CbInfo.iStreamId = pStreamInfo->iStreamId;
        CbInfo.eRequestType = pStreamInfo->eRequestType;
        CbInfo.eRequestResponse = pStreamInfo->eResponseCode;

        pStreamInfo->pWriteCb(pState, &CbInfo, pBuf, pHeader->uLength, pStreamInfo->pUserData);
        pState->iLocalWindow -= pHeader->uLength;
        pStreamInfo->iLocalWindow -= pHeader->uLength;

        // update amount of data received
        pStreamInfo->iBodyReceived += pHeader->uLength;

        if (bEndStream == TRUE)
        {
            // update body size now that stream is complete
            pStreamInfo->iBodySize = pStreamInfo->iBodyReceived;
            pStreamInfo->pWriteCb(pState, &CbInfo, NULL, PROTOHTTP2_RECVDONE, pStreamInfo->pUserData);
        }
    }
    else
    {
        // allocate space if necessary (size of the window)
        if ((pStreamInfo->pData == NULL) && ((pStreamInfo->pData = (uint8_t *)DirtyMemAlloc(pStreamInfo->iDataMax, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData)) == NULL))
        {
            _ProtoHttp2PrepareRstStream(pState, pStreamInfo, ERRORTYPE_INTERNAL_ERROR, "failed to allocate space for storing data");
            return;
        }
        // make sure the peer did not exceed the window
        if (pStreamInfo->iDataLen+(int32_t)pHeader->uLength <= pStreamInfo->iDataMax)
        {
            ds_memcpy(pStreamInfo->pData+pStreamInfo->iDataLen, pBuf, pHeader->uLength);
            pStreamInfo->iDataLen += pHeader->uLength;
            pState->iLocalWindow -= pHeader->uLength;
            pStreamInfo->iLocalWindow -= pHeader->uLength;
        }
        else
        {
            _ProtoHttp2PrepareRstStream(pState, pStreamInfo, ERRORTYPE_FLOW_CONTROL_ERROR, "peer exceeded the receive window");
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2HandleHeaders

    \Description
        Handles the incoming headers frame

    \Input *pState      - module state
    \Input *pBuf        - the buffer we are parsing the data from
    \Input *pHeader     - the header data for the frame
    \Input *pStreamInfo - information about associated stream

    \Version 11/04/2016 (eesponda)
*/
/********************************************************************************F*/
static void _ProtoHttp2HandleHeaders(ProtoHttp2RefT *pState, const uint8_t *pBuf, const FrameHeaderT *pHeader, StreamInfoT *pStreamInfo)
{
    uint8_t bEndStream = FALSE;
    int32_t iBufRead = 0;

    if ((pStreamInfo->eState != STREAMSTATE_OPEN) && (pStreamInfo->eState != STREAMSTATE_HALF_CLOSED_LOCAL))
    {
        _ProtoHttp2PrepareRstStream(pState, pStreamInfo, ERRORTYPE_STREAM_CLOSED, "received header frames when stream is in unexpected state");
        return;
    }

    // update the stream data based on the flags
    if ((pHeader->uFlags & PROTOHTTP2_FLAG_END_STREAM) != 0)
    {
        if (pStreamInfo->eState == STREAMSTATE_OPEN)
        {
            pStreamInfo->eState = STREAMSTATE_HALF_CLOSED_REMOTE;
        }
        else if (pStreamInfo->eState == STREAMSTATE_HALF_CLOSED_LOCAL)
        {
            pStreamInfo->eState = STREAMSTATE_CLOSED;
        }
        bEndStream = TRUE;
    }

    // skip the priority frame data if present
    if ((pHeader->uFlags & PROTOHTTP2_FLAG_PRIORITY) != 0)
    {
        iBufRead += PROTOHTTP2_PRIORITY_SIZE;
    }

    // if this is the end of the stream we can parse our headers
    if ((pHeader->uFlags & PROTOHTTP2_FLAG_END_HEADERS) != 0)
    {
        if (_ProtoHttp2ParseHeader(pState, pStreamInfo, pBuf+iBufRead, pHeader->uLength-iBufRead) != 0)
        {
            _ProtoHttp2PrepareRstStream(pState, pStreamInfo, ERRORTYPE_COMPRESSION_ERROR, "failed to decompress header");
        }
    }
    else
    {
        NetPrintf(("protohttp2: [%p] expected end of the header as our maximum header list fits within a single frame, closing connection\n", pState));
        pState->eErrorType = ERRORTYPE_PROTOCOL_ERROR;
    }

    // if we have a write callback, invoke it
    if ((pStreamInfo->pWriteCb != NULL) && (bEndStream == TRUE))
    {
        ProtoHttp2WriteCbInfoT CbInfo;
        CbInfo.iStreamId = pStreamInfo->iStreamId;
        CbInfo.eRequestType = pStreamInfo->eRequestType;
        CbInfo.eRequestResponse = pStreamInfo->eResponseCode;

        // update body size now that stream is complete
        pStreamInfo->iBodySize = pStreamInfo->iBodyReceived;
        pStreamInfo->pWriteCb(pState, &CbInfo, NULL, PROTOHTTP2_RECVDONE, pStreamInfo->pUserData);
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2HandleFrame

    \Description
        Handles the incoming http2 frame

    \Input *pState  - module state
    \Input *pBuf    - the buffer we are parsing the data from
    \Input *pHeader - the header data for the frame

    \Version 09/29/2016 (eesponda)
*/
/********************************************************************************F*/
static void _ProtoHttp2HandleFrame(ProtoHttp2RefT *pState, const uint8_t *pBuf, const FrameHeaderT *pHeader)
{
    StreamInfoT *pStreamInfo = NULL;
    int32_t iOffset;

    // validate we know about the stream (if non-zero)
    if ((pHeader->iStreamId > 0) && ((pStreamInfo = _ProtoHttp2StreamInfoGet(pState, pHeader->iStreamId)) == NULL))
    {
        const ErrorTypeE eErrorType = (pHeader->iStreamId < pState->iStreamId) ? ERRORTYPE_STREAM_CLOSED : ERRORTYPE_REFUSED_STREAM;
        NetPrintf(("protohttp2: [%p] received stream id that is no longer tracked by module, sending %s to peer\n", pState, _ProtoHttp2_strErrorType[eErrorType]));

        if (_ProtoHttp2EncodeRstStream(pState, pHeader->iStreamId, eErrorType) != 0)
        {
            NetPrintf(("protohttp2: [%p] not enough space in output buffer to encode rst_stream\n", pState));
        }
        return;
    }
    // validate the length in the header, if invalid then reset the stream
    if (pHeader->uLength > pState->LocalSettings.uMaxFrameSize)
    {
        _ProtoHttp2PrepareRstStream(pState, pStreamInfo, ERRORTYPE_FRAME_SIZE_ERROR, "received frame header with invalid size");
        return;
    }

    if (pHeader->uType == FRAMETYPE_SETTINGS)
    {
        const int32_t iPrevWindow = pState->PeerSettings.uInitialWindowSize; /* save previous window setting for calculation */

        // decode the settings and update what we have saved
        _ProtoHttp2DecodeSettings(pState, pBuf, pHeader);

        // if not an acknowledgement, apply settings and send acknowledgement
        if ((pHeader->uFlags & PROTOHTTP2_FLAG_ACK) == 0)
        {
            int32_t iStream;

            // encode the acknowledge
            if ((iOffset = _ProtoHttp2EncodeSettings(pState, TRUE, pState->pOutBuf+pState->iOutLen, pState->iOutMax-pState->iOutLen)) != 0)
            {
                pState->iOutLen += iOffset;
            }

            // resize the encoder dynamic table to match the peer, send dynamic table update if needed
            HpackResize(pState->pEncoder, pState->PeerSettings.uHeaderTableSize, pState->bSettingsRecv);
            pState->bSettingsRecv = TRUE;

            // recalculate the stream windows based on new settings and how much we consumed
            for (iStream = 0; iStream < pState->iNumStreams; iStream += 1)
            {
                StreamInfoT *pStream = &pState->Streams[iStream];
                if ((pStream->eState == STREAMSTATE_OPEN) || (pStream->eState == STREAMSTATE_HALF_CLOSED_LOCAL))
                {
                    pStream->iPeerWindow = pState->PeerSettings.uInitialWindowSize - (iPrevWindow - pStream->iPeerWindow);
                }
            }
        }
        else
        {
            // received an acknowledgement, disable timer
            pState->uSettingsTimer = 0;
        }
    }
    else if (pHeader->uType == FRAMETYPE_RST_STREAM)
    {
        uint32_t uErrorCode;

        // decode error code
        uErrorCode =  *pBuf++ << 24;
        uErrorCode |= *pBuf++ << 16;
        uErrorCode |= *pBuf++ << 8;
        uErrorCode |= *pBuf++;

        NetPrintf(("protohttp2: [%p] received rst stream (error=%s)\n", pState, _ProtoHttp2_strErrorType[uErrorCode]));

        // cache the error code
        pStreamInfo->eErrorType = (ErrorTypeE)uErrorCode;

        // close the stream and fire any callbacks needed
        _ProtoHttp2StreamCloseOnError(pState, pStreamInfo);
    }
    else if (pHeader->uType == FRAMETYPE_GOAWAY)
    {
        int32_t iStreamId, iStream;
        uint32_t uErrorCode;

        // decode last-stream-id
        iStreamId  = *pBuf++ << 24;
        iStreamId |= *pBuf++ << 16;
        iStreamId |= *pBuf++ << 8;
        iStreamId |= *pBuf++;
        // decode error code
        uErrorCode  = *pBuf++ << 24;
        uErrorCode |= *pBuf++ << 16;
        uErrorCode |= *pBuf++ << 8;
        uErrorCode |= *pBuf++;

        NetPrintf(("protohttp2: [%p] received goaway frame (stream=0x%08x, error=%s)\n", pState, iStreamId, _ProtoHttp2_strErrorType[uErrorCode]));

        // close all the streams
        for (iStream = 0; iStream < pState->iNumStreams; iStream += 1)
        {
            _ProtoHttp2StreamCloseOnError(pState, &pState->Streams[iStream]);
        }

        // peer told us to go away, so go into the failure state
        pState->eState = ST_FAIL;

        // cache the error the peer sent
        pState->eErrorType = (ErrorTypeE)uErrorCode;
    }
    else if ((pHeader->uType == FRAMETYPE_PING) && ((pHeader->uFlags & PROTOHTTP2_FLAG_ACK) == 0))
    {
        // encode ping acknowledgement
        if ((iOffset = _ProtoHttp2EncodePing(pState, TRUE, pBuf, pHeader->uLength)) != 0)
        {
            NetPrintfVerbose((pState->iVerbose, 1, "protohttp2: [%p] not enough space to encode ping frame into output buffer\n", pState));
        }
    }
    else if (pHeader->uType == FRAMETYPE_HEADERS)
    {
        _ProtoHttp2HandleHeaders(pState, pBuf, pHeader, pStreamInfo);
    }
    else if (pHeader->uType == FRAMETYPE_DATA)
    {
        _ProtoHttp2HandleData(pState, pBuf, pHeader, pStreamInfo);
    }
    else if (pHeader->uType == FRAMETYPE_WINDOW_UPDATE)
    {
        int32_t iIncrement;

        // decode 31-bit window size increment (ignoring the high bit)
        iIncrement  = (*pBuf++ << 24) & 0x7f;
        iIncrement |= *pBuf++ << 16;
        iIncrement |= *pBuf++ << 8;
        iIncrement |= *pBuf++;

        // make sure the window size increment is valid
        if (iIncrement != 0)
        {
            NetPrintfVerbose((pState->iVerbose, 1, "protohttp2: [%p] received window update (stream=0x%08x, increment=%d)\n", pState, pHeader->iStreamId, iIncrement));
            if (pHeader->iStreamId == 0)
            {
                pState->iPeerWindow += iIncrement;
            }
            else
            {
                pStreamInfo->iPeerWindow += iIncrement;
            }
        }
        else
        {
            if (pHeader->iStreamId == 0)
            {
                pState->eErrorType = ERRORTYPE_PROTOCOL_ERROR;
                NetPrintf(("protohttp2: [%p] received invalid connection wide window size increment, closing connection\n", pState));
            }
            else
            {
                _ProtoHttp2PrepareRstStream(pState, pStreamInfo, ERRORTYPE_PROTOCOL_ERROR, "received invalid stream window size increment");
            }
        }
    }
    else if (pHeader->uType == FRAMETYPE_CONTINUATION)
    {
        /* we should never receive continuation as our header list maximum
           is well below what we can send in a frame */
        NetPrintf(("protohttp2: [%p] received continuation frame when not expected, closing the connection\n", pState));
        pState->eErrorType = ERRORTYPE_PROTOCOL_ERROR;
    }
    else if (pHeader->uType == FRAMETYPE_PUSH_PROMISE)
    {
        /* we disabled push in our settings so if we receive this type of frame
           it is a protocol level error */
        NetPrintf(("protohttp2: [%p] received push promise frame when push was disabled, closing the connection\n", pState));
        pState->eErrorType = ERRORTYPE_PROTOCOL_ERROR;
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2FormatRequestHeader

    \Description
        Format a request header based on given input data.

    \Input *pState      - module state
    \Input *pStreamInfo - stream information for the request
    \Input *pUrl        - pointer to user-supplied url
    \Input *pData       - pointer to included data, or NULL if none
    \Input iDataLen     - size of included data; zero if none, negative if streaming put/post

    \Output
        int32_t         - zero=success, else error

    \Version 10/24/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoHttp2FormatRequestHeader(ProtoHttp2RefT *pState, StreamInfoT *pStreamInfo, const char *pUrl, const uint8_t *pData, int32_t iDataLen)
{
    const char *pUrlSlash;
    char *pHeader;
    int32_t iHeaderLen = 0, iHeaderMax;

    // allocate memory for request header cache, if peer didn't set it we use our local limits
    iHeaderMax = DS_MIN(pState->PeerSettings.uMaxHeaderListSize, PROTOHTTP2_HDRCACHESIZE);
    if ((pHeader = (char *)DirtyMemAlloc(iHeaderMax, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protohttp2: [%p] could not allocate space for request header cache\n", pState));
        return(-1);
    }
    pStreamInfo->pRequestHeader = pHeader;
    pStreamInfo->iRequestHeaderLen = iHeaderMax;

    // if url is empty or isn't preceded by a slash, put one in
    pUrlSlash = (*pUrl != '/') ? "/" : "";

    // format request header
    iHeaderLen += ds_snzprintf(pHeader+iHeaderLen, iHeaderMax-iHeaderLen, ":method: %s\r\n", _ProtoHttp2_strRequestNames[pStreamInfo->eRequestType]);
    iHeaderLen += ds_snzprintf(pHeader+iHeaderLen, iHeaderMax-iHeaderLen, ":path: %s%s\r\n", pUrlSlash, pUrl);
    iHeaderLen += ds_snzprintf(pHeader+iHeaderLen, iHeaderMax-iHeaderLen, ":scheme: %s\r\n", pState->bSecure ? "https" : "http");
    if ((pState->bSecure && (pState->iPort == 443)) || (pState->iPort == 80))
    {
        iHeaderLen += ds_snzprintf(pHeader+iHeaderLen, iHeaderMax-iHeaderLen, ":authority: %s\r\n", pState->strHost);
    }
    else
    {
        iHeaderLen += ds_snzprintf(pHeader+iHeaderLen, iHeaderMax-iHeaderLen, ":authority: %s:%d\r\n", pState->strHost, pState->iPort);
    }
    if (iDataLen > 0)
    {
        iHeaderLen += ds_snzprintf(pHeader+iHeaderLen, iHeaderMax-iHeaderLen, "content-Length: %qd\r\n", iDataLen);
    }
    if ((pState->pAppendHdr == NULL) || !ds_stristr(pState->pAppendHdr, "user-agent:"))
    {
        iHeaderLen += ds_snzprintf(pHeader+iHeaderLen, iHeaderMax-iHeaderLen, "user-agent: ProtoHttp %d.%d/DS %d.%d.%d.%d.%d (" DIRTYCODE_PLATNAME ")\r\n",
            (PROTOHTTP2_VERSION >> 8) & 0xFF, PROTOHTTP2_VERSION & 0xFF, DIRTYSDK_VERSION_YEAR, DIRTYSDK_VERSION_SEASON, DIRTYSDK_VERSION_MAJOR, DIRTYSDK_VERSION_MINOR, DIRTYSDK_VERSION_PATCH);
    }
    if ((pState->pAppendHdr == NULL) || (pState->pAppendHdr[0] == '\0'))
    {
        iHeaderLen += ds_snzprintf(pHeader+iHeaderLen, iHeaderMax-iHeaderLen, "accept: */*\r\n");
    }
    else
    {
        iHeaderLen += ds_snzprintf(pHeader+iHeaderLen, iHeaderMax-iHeaderLen, "%s", pState->pAppendHdr);
    }

    // call custom header format callback, if specified
    if (pState->pCustomHeaderCb != NULL)
    {
        if ((iHeaderLen = pState->pCustomHeaderCb(pState, pHeader, iHeaderMax, pData, iDataLen, pState->pCallbackRef)) < 0)
        {
            NetPrintfVerbose((pState->iVerbose, 0, "protohttp2: [%p] custom header callback error %d\n", pState, iHeaderLen));
            return(iHeaderLen);
        }
        if (iHeaderLen == 0)
        {
            iHeaderLen = (int32_t)strlen(pHeader);
        }
    }

    // make sure we were able to complete the header
    if (iHeaderLen > iHeaderMax)
    {
        NetPrintfVerbose((pState->iVerbose, 0, "protohttp: not enough buffer to format request header (have %d, need %d)\n", iHeaderMax, iHeaderLen));
        return(PROTOHTTP2_MINBUFF);
    }

    #if DIRTYCODE_LOGGING
    if (pState->iVerbose > 1)
    {
        NetPrintf(("protohttp2: [%p] sending request:\n", pState));
        NetPrintWrap(pHeader, 80);
    }
    #endif

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2SetAppendHeader

    \Description
        Set given string as append header, allocating memory as required

    \Input *pState          - module state
    \Input *pAppendHdr      - append header string

    \Output
        int32_t             - zero=success, else error

    \Version 10/27/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoHttp2SetAppendHeader(ProtoHttp2RefT *pState, const char *pAppendHdr)
{
    int32_t iAppendBufLen, iAppendStrLen;

    // check for empty append string, in which case we free the buffer
    if ((pAppendHdr == NULL) || (*pAppendHdr == '\0'))
    {
        if (pState->pAppendHdr != NULL)
        {
            DirtyMemFree(pState->pAppendHdr, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
            pState->pAppendHdr = NULL;
        }
        pState->iAppendLen = 0;
        return(0);
    }

    // check to see if append header is already set
    if ((pState->pAppendHdr != NULL) && (strncmp(pAppendHdr, pState->pAppendHdr, strlen(pState->pAppendHdr)) == 0))
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttp: ignoring set of append header '%s' that is already set\n", pAppendHdr));
        return(0);
    }

    // get append header length
    iAppendStrLen = (int32_t)strlen(pAppendHdr);
    // append buffer size includes null and space for \r\n if not included by submitter
    iAppendBufLen = iAppendStrLen + 3;

    // see if we need to allocate a new buffer
    if (iAppendBufLen > pState->iAppendLen)
    {
        if (pState->pAppendHdr != NULL)
        {
            DirtyMemFree(pState->pAppendHdr, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        }
        if ((pState->pAppendHdr = DirtyMemAlloc(iAppendBufLen, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData)) != NULL)
        {
            pState->iAppendLen = iAppendBufLen;
        }
        else
        {
            NetPrintf(("protohttp: could not allocate %d byte buffer for append header\n", iAppendBufLen));
            pState->iAppendLen = 0;
            return(-1);
        }
    }

    // copy append header
    ds_strnzcpy(pState->pAppendHdr, pAppendHdr, iAppendStrLen+1);

    // if append header is not \r\n terminated, do it here
    if (pState->pAppendHdr[iAppendStrLen-2] != '\r' || pState->pAppendHdr[iAppendStrLen-1] != '\n')
    {
        ds_strnzcat(pState->pAppendHdr, "\r\n", pState->iAppendLen);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2ResizeInputBuffer

    \Description
        Resize the input buffer

    \Input *pState  - module state
    \Input iBufMax  - new buffer size

    \Output
        int32_t     - zero=success, else error

    \Version 11/14/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoHttp2ResizeInputBuffer(ProtoHttp2RefT *pState, int32_t iBufMax)
{
    int32_t iCopySize;
    uint8_t *pInpBuf;

    NetPrintfVerbose((pState->iVerbose, 1, "protohttp2: [%p] resizing input buffer from %d to %d bytes\n", pState, pState->iInpMax, iBufMax));
    if ((pInpBuf = (uint8_t *)DirtyMemAlloc(iBufMax, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protohttp: [%p] could not resize input buffer\n", pState));
        return(-1);
    }

    // calculate size of data to copy from old buffer to new
    if ((iCopySize = pState->iInpLen) > iBufMax)
    {
        NetPrintf(("protohttp: [%p] warning; resize of input buffer is losing %d bytes of data\n", pState, iCopySize - iBufMax));
        iCopySize = iBufMax;
    }
    // copy valid contents of input buffer, if any, to new buffer
    ds_memcpy(pInpBuf, pState->pInpBuf, iCopySize);

    // dispose of old buffer
    DirtyMemFree(pState->pInpBuf, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);

    // update buffer variables
    pState->pInpBuf = pInpBuf;
    pState->iInpLen = iCopySize;
    pState->iInpMax = iBufMax;

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2CheckSettings

    \Description
        Handle all setting related synchronization

    \Input *pState  - module state

    \Notes
        This function will check to make sure that we get our settings
        acknowledgement from our peer in time. It also checks to see
        if we need to send a new settings frame to our peer, in this
        case it will handle the encoding into our output buffer.

    \Version 11/15/2016 (eesponda)
*/
/********************************************************************************F*/
static void _ProtoHttp2CheckSettings(ProtoHttp2RefT *pState)
{
    // check for settings timeout
    if (pState->uSettingsTimer != 0)
    {
        if (NetTickDiff(NetTick(), pState->uSettingsTimer) > 0)
        {
            NetPrintf(("protohttp2: [%p] peer did not send settings in time, closing connection\n", pState));
            pState->eErrorType = ERRORTYPE_SETTINGS_TIMEOUT;
        }
    }
    // otherwise check if we need to synchronize our settings
    else if (memcmp(&pState->LocalSettings, &pState->TempSettings, sizeof(pState->TempSettings)) != 0)
    {
        int32_t iOffset;
        if ((iOffset = _ProtoHttp2EncodeSettings(pState, FALSE, pState->pOutBuf+pState->iOutLen, pState->iOutMax-pState->iOutLen)) != 0)
        {
            pState->iOutLen += iOffset;
            pState->uSettingsTimer = NetTick() + PROTOHTTP2_TIMEOUT;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2CheckWindows

    \Description
        Check the connection and stream wide windows to send increments when
        necessary

    \Input *pState  - module state

    \Version 11/21/2016 (eesponda)
*/
/********************************************************************************F*/
static void _ProtoHttp2CheckWindows(ProtoHttp2RefT *pState)
{
    int32_t iStream;

    // check the connection wide window, if so increment a large amount to handle our multiple streams
    if (pState->iLocalWindow <= 0)
    {
        if (_ProtoHttp2EncodeWindowUpdate(pState, 0, PROTOHTTP2_MAX_STREAMS*PROTOHTTP2_WINDOWSIZE, &pState->iLocalWindow) != 0)
        {
            NetPrintfVerbose((pState->iVerbose, 1, "protohttp2: [%p] not enough space in the output buffer to encode window update for connection\n", pState));
        }
    }

    // check the stream wide windows
    for (iStream = 0; iStream < pState->iNumStreams; iStream += 1)
    {
        StreamInfoT *pStreamInfo = &pState->Streams[iStream];

        if (pStreamInfo->iLocalWindow <= 0)
        {
            if (_ProtoHttp2EncodeWindowUpdate(pState, pStreamInfo->iStreamId, PROTOHTTP2_WINDOWSIZE, &pStreamInfo->iLocalWindow) != 0)
            {
                NetPrintfVerbose((pState->iVerbose, 1, "protohttp2: [%p] not enough space in the output buffer to encode window update for stream\n", pState));
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2Read

    \Description
        Read data out of the stream's buffer

    \Input *pState      - module state
    \Input *pStreamInfo - stream we are reading from
    \Input *pBuffer     - buffer to store data in
    \Input iBufMin      - minimum number of bytes to return
    \Input iBufMax      - maximum number of bytes to return (buffer size)

    \Output
        int32_t         - negative=error, zero=no data available or bufmax <= 0, positive=number of bytes read

    \Version 11/23/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoHttp2Read(ProtoHttp2RefT *pState, StreamInfoT *pStreamInfo, uint8_t *pBuffer, int32_t iBufMin, int32_t iBufMax)
{
    int32_t iLen;

    // early out for failure result
    if ((pState->eState == ST_FAIL) || ((pStreamInfo->eState == STREAMSTATE_CLOSED) && (pStreamInfo->eErrorType != ERRORTYPE_NO_ERROR)))
    {
        return(pState->bTimeout ? PROTOHTTP2_TIMEOUT : PROTOHTTP2_RECVFAIL);
    }
    // if they only wanted head that is what we will return
    if ((pStreamInfo->iHeaderLen > 0) && (pStreamInfo->eRequestType == PROTOHTTP_REQUESTTYPE_HEAD))
    {
        return(PROTOHTTP2_RECVHEAD);
    }
    // if we haven't sent the headers yet we cannot expect to receive any data
    if (pStreamInfo->eState == STREAMSTATE_IDLE)
    {
        return(PROTOHTTP2_RECVWAIT);
    }

    // if they are querying only for done state when no more data is available to be read
    if ((iBufMax == 0) && ((pStreamInfo->eState == STREAMSTATE_HALF_CLOSED_REMOTE) || (pStreamInfo->eState == STREAMSTATE_CLOSED)) && (pStreamInfo->iBodyReceived == pStreamInfo->iBodySize))
    {
        return(PROTOHTTP2_RECVDONE);
    }

    // make sure the range is valid
    if (iBufMax < 1)
    {
        return(0);
    }
    // clamp the range
    iBufMin = DS_MAX(1, iBufMin);
    iBufMax = DS_MAX(iBufMin, iBufMax);
    iBufMin = DS_MIN(iBufMin, pStreamInfo->iDataMax);
    iBufMax = DS_MIN(iBufMax, pStreamInfo->iDataMax);

    // figure out how much data is available
    iLen = DS_MIN(pStreamInfo->iDataLen, iBufMax);

    // check for end of data
    if ((iLen == 0) && ((pStreamInfo->eState == STREAMSTATE_HALF_CLOSED_REMOTE) || (pStreamInfo->eState == STREAMSTATE_CLOSED)))
    {
        // update body size now that stream is complete
        pStreamInfo->iBodySize = pStreamInfo->iBodyReceived;
        return(PROTOHTTP2_RECVDONE);
    }

    // see if there is enough to return
    if (iLen >= iBufMin)
    {
        // return data to caller
        if (pBuffer != NULL)
        {
            ds_memcpy(pBuffer, pStreamInfo->pData, iLen);

            #if DIRTYCODE_LOGGING
            NetPrintfVerbose((pState->iVerbose, 2, "protohttp2: [%p] read %d bytes\n", pState, iLen));
            if (pState->iVerbose > 3)
            {
                NetPrintMem(pBuffer, iLen, "http2-read");
            }
            #endif
        }
        pStreamInfo->iBodyReceived += iLen;

        memmove(pStreamInfo->pData, pStreamInfo->pData+iLen, pStreamInfo->iDataLen-iLen);
        pStreamInfo->iDataLen -= iLen;

        // return bytes read
        return(iLen);
    }

    // nothing available
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp2EncodeRequest

    \Description
        Encode the request into the output buffer (headers / data)

    \Input *pState      - module state
    \Input *pStreamInfo - stream we are reading from
    \Input *pData       - data we are encoding
    \Input iDataSize    - size of the data we are encoding

    \Output
        int32_t         - negative=error, otherwise amount of user data encoded

    \Version 11/23/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoHttp2EncodeRequest(ProtoHttp2RefT *pState, StreamInfoT *pStreamInfo, const uint8_t *pData, int32_t iDataSize)
{
    int32_t iResult, iOutLen;

    // save the outlen in case of error
    iOutLen = pState->iOutLen;

    // encode the request headers
    if ((iResult = _ProtoHttp2EncodeHeaders(pState, pStreamInfo, iDataSize == 0, pStreamInfo->pRequestHeader)) < 0)
    {
        NetPrintf(("protohttp2: [%p] not enough room in the output buffer to encode headers\n", pState));
    }
    // encode the data if any exist
    else if ((pData != NULL) && (iDataSize > 0))
    {
        if ((iResult = _ProtoHttp2EncodeData(pState, pStreamInfo, pData, iDataSize, TRUE)) < 0)
        {
            NetPrintf(("protohttp2: [%p] not enough room in the output buffer to encode data\n", pState));
            pState->iOutLen = iOutLen;
        }
    }

    // return result back to user
    return(iResult);
}

/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ProtoHttp2Create

    \Description
        Allocate module state and prepare for use

    \Input iBufSize     - length of recv buffer

    \Output
        ProtoHttp2RefT *- pointer to module state, or NULL

    \Version 09/27/2016 (eesponda)
*/
/********************************************************************************F*/
ProtoHttp2RefT *ProtoHttp2Create(int32_t iBufSize)
{
    ProtoHttp2RefT *pState;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    uint32_t uCiphers;

    // query memgroup data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // clamp the buffer size based on the default frame size and maximum frame size
    PROTOHTTP2_ClampFrameSize(iBufSize);

    // allocate state
    if ((pState = (ProtoHttp2RefT *)DirtyMemAlloc(sizeof(*pState), PROTOHTTP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protohttp2: unable to allocate module state\n"));
        return(NULL);
    }
    ds_memclr(pState, sizeof(*pState));
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;
    pState->iVerbose = 1;
    pState->uTimeout = PROTOHTTP2_TIMEOUT_DEFAULT;

    // set the buffer sizes to the defaults, adding the additional space for a frame header
    pState->iInpMax = iBufSize+PROTOHTTP2_RESERVED_SIZE;
    pState->iOutMax = _ProtoHttp2_DefaultSettings.uMaxFrameSize+PROTOHTTP2_RESERVED_SIZE;

    // allocate ssl state
    if ((pState->pSsl = ProtoSSLCreate()) == NULL)
    {
        NetPrintf(("protohttp2: [%p] unable to allocate ssl module state\n", pState));
        ProtoHttp2Destroy(pState);
        return(NULL);
    }
    ProtoSSLControl(pState->pSsl, 'alpn', 0, 0, (void *)"h2");  // tell protossl to advertise the http2 protocol
    ProtoSSLControl(pState->pSsl, 'snod', TRUE, 0, NULL);       // set TCP_NODELAY on the SSL socket

    // configure the supported ciphers
    uCiphers = PROTOSSL_CIPHER_ALL;
    uCiphers |= (PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_128_GCM_SHA256 | PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_256_GCM_SHA384);
    uCiphers &= ~(PROTOSSL_CIPHER_RSA_WITH_RC4_128_SHA | PROTOSSL_CIPHER_RSA_WITH_RC4_128_MD5);
    ProtoSSLControl(pState->pSsl, 'ciph', uCiphers, 0, NULL);

    // allocate input buffer
    if ((pState->pInpBuf = (uint8_t *)DirtyMemAlloc(pState->iInpMax, PROTOHTTP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protohttp2: [%p] unable to allocate protohttp2 input buffer\n", pState));
        ProtoHttp2Destroy(pState);
        return(NULL);
    }
    ds_memclr(pState->pInpBuf, pState->iInpMax);

    // allocate output buffer
    if ((pState->pOutBuf = (uint8_t *)DirtyMemAlloc(pState->iOutMax, PROTOHTTP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protohttp2: [%p] unable to allocate protohttp2 output buffer\n", pState));
        ProtoHttp2Destroy(pState);
        return(NULL);
    }
    ds_memclr(pState->pOutBuf, pState->iOutMax);

    // create the encoder context
    if ((pState->pEncoder = HpackCreate(_ProtoHttp2_DefaultSettings.uHeaderTableSize, FALSE)) == NULL)
    {
        NetPrintf(("protohttp2: [%p] unable to create encoder context\n", pState));
        ProtoHttp2Destroy(pState);
        return(NULL);
    }
    // create the decoder context
    if ((pState->pDecoder = HpackCreate(_ProtoHttp2_DefaultSettings.uHeaderTableSize, TRUE)) == NULL)
    {
        NetPrintf(("protohttp2: [%p] unable to create decoder context\n", pState));
        ProtoHttp2Destroy(pState);
        return(NULL);
    }

    // init crit
    NetCritInit(&pState->HttpCrit, "ProtoHttp2");

    // reset the state to default
    _ProtoHttp2Reset(pState);

    // return the state
    return(pState);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttp2Update

    \Description
        Give time to module to do its thing (should be called periodically to
        allow module to perform work)

    \Input *pState      - module state

    \Version 09/27/2016 (eesponda)
*/
/********************************************************************************F*/
void ProtoHttp2Update(ProtoHttp2RefT *pState)
{
    int32_t iResult;

    // give time to ssl module
    ProtoSSLUpdate(pState->pSsl);

    // acquire sole access to http crit
    NetCritEnter(&pState->HttpCrit);

    // see if the connection is complete
    if (pState->eState == ST_CONN)
    {
        int32_t iStream;

        iResult = ProtoSSLStat(pState->pSsl, 'stat', NULL, 0);
        if (iResult > 0)
        {
            /* just use some stack space to send this connection preface instead
               of having to juggle with our output buffer (that might have data) */
            uint8_t aConnectionPreface[128];
            int32_t iOffset = 0;

            // add the connection preface to the buffer
            ds_memcpy(aConnectionPreface, _ProtoHttp2_ConnectionPreface, sizeof(_ProtoHttp2_ConnectionPreface));
            iOffset += sizeof(_ProtoHttp2_ConnectionPreface);

            // encode our settings
            iOffset += _ProtoHttp2EncodeSettings(pState, FALSE, aConnectionPreface+iOffset, sizeof(aConnectionPreface)-iOffset);

            // send the connection preface+settings and wait for settings from server
            _ProtoHttp2Send(pState, aConnectionPreface, iOffset);
            pState->eState = ST_ACTIVE;
            pState->uSettingsTimer = NetTick() + PROTOHTTP2_TIMEOUT_DEFAULT;
        }
        else if (iResult < 0)
        {
            NetPrintf(("protohttp2: [%p] ST_CONN got ST_FAIL (err=%d)\n", pState, iResult));
            pState->eState = ST_FAIL;
            pState->iSslFail = ProtoSSLStat(pState->pSsl, 'fail', NULL, 0);
            pState->iHresult = ProtoSSLStat(pState->pSsl, 'hres', NULL, 0);
        }
        else if (NetTickDiff(NetTick(), pState->uTimer) >= 0)
        {
            NetPrintf(("protohttp2: [%p] timed out while establishing connection\n", pState));
            _ProtoHttp2Close(pState, "timeout");
            pState->bTimeout = TRUE;
        }

        // if a failure occured when establishing the connection handle closing our streams
        for (iStream = 0; (pState->eState == ST_FAIL) && (iStream < pState->iNumStreams); iStream += 1)
        {
            _ProtoHttp2StreamCloseOnError(pState, &pState->Streams[iStream]);
        }
    }
    /* otherwise we are connected so let's try to send / receive data from our peer
       we will act on the data depending our state */
    else if ((pState->eState > ST_CONN) && (pState->eState < ST_FAIL))
    {
        // send any data if we have any
        if (pState->iOutLen > 0)
        {
            // send as much data as we can and update our offset accordingly
            if ((iResult = _ProtoHttp2Send(pState, pState->pOutBuf+pState->iOutOff, pState->iOutLen-pState->iOutOff)) > 0)
            {
                pState->iOutOff += iResult;
            }
            // if we sent all our data, clear our length/offset
            if (pState->iOutOff == pState->iOutLen)
            {
                pState->iOutLen = pState->iOutOff = 0;
            }
        }

        // receive new data from our peer
        if ((iResult = _ProtoHttp2Recv(pState, pState->pInpBuf+pState->iInpLen, pState->iInpMax-pState->iInpLen)) > 0)
        {
            FrameHeaderT Header;
            const uint8_t *pBuf;

            // increment the offset into the input buffer
            pState->iInpLen += iResult;

            // decode as many frames as possible
            while ((pBuf = _ProtoHttp2DecodeFrameHeader(pState, pState->pInpBuf, pState->iInpLen, &Header)) != NULL)
            {
                const uint32_t uFrameLength = Header.uLength+Header.uPadding+PROTOHTTP2_HEADER_SIZE;

                // handle the frame
                _ProtoHttp2HandleFrame(pState, pBuf, &Header);

                /* advance the buffer past the frame and update new length
                   note: we need to take advantage of as much space as possible in our buffer.
                   for that reason we cannot just use an offset into the buffer like we
                   do on the send side. */
                memmove(pState->pInpBuf, pState->pInpBuf+uFrameLength, pState->iInpLen-uFrameLength);
                pState->iInpLen -= uFrameLength;
            }
        }

        // handle settings synchronization
        _ProtoHttp2CheckSettings(pState);

        // check our receive windows and send updates if needed
        _ProtoHttp2CheckWindows(pState);

        // attempt to handle any connection wide error
        if (pState->eErrorType != ERRORTYPE_NO_ERROR)
        {
            _ProtoHttp2SendGoAway(pState, pState->eErrorType, _ProtoHttp2_strErrorType[pState->eErrorType]);
        }
    }

    // release access to http crit
    NetCritLeave(&pState->HttpCrit);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttp2Destroy

    \Description
        Destroy the module and release its state

    \Input *pState      - module state

    \Version 09/27/2016 (eesponda)
*/
/********************************************************************************F*/
void ProtoHttp2Destroy(ProtoHttp2RefT *pState)
{
    int32_t iStream;

    // try to gracefully close the connection
    ProtoHttp2Close(pState);

    // clean up append header memory
    _ProtoHttp2SetAppendHeader(pState, NULL);

    // cleanup stream info memory
    for (iStream = 0; iStream < pState->iNumStreams; iStream += 1)
    {
        _ProtoHttp2StreamInfoCleanup(pState, &pState->Streams[iStream]);
    }

    NetCritKill(&pState->HttpCrit);

    if (pState->pDecoder != NULL)
    {
        HpackDestroy(pState->pDecoder);
        pState->pDecoder = NULL;
    }
    if (pState->pEncoder != NULL)
    {
        HpackDestroy(pState->pEncoder);
        pState->pEncoder = NULL;
    }
    if (pState->pInpBuf != NULL)
    {
        DirtyMemFree(pState->pInpBuf, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        pState->pInpBuf = NULL;
    }
    if (pState->pOutBuf != NULL)
    {
        DirtyMemFree(pState->pOutBuf, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        pState->pOutBuf = NULL;
    }
    if (pState->pSsl != NULL)
    {
        ProtoSSLDestroy(pState->pSsl);
        pState->pSsl = NULL;
    }
    DirtyMemFree(pState, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttp2Callback

    \Description
        Set header callbacks.

    \Input *pState          - module state
    \Input *pCustomHeaderCb - pointer to custom send header callback (may be NULL)
    \Input *pReceiveHeaderCb- pointer to recv header callback (may be NULL)
    \Input *pUserData       - user-supplied callback ref (may be NULL)

    \Notes
        The ProtoHttpCustomHeaderCbT callback is used to allow customization of
        the HTTP header before sending.  It is more powerful than the append
        header functionality, allowing to make changes to any part of the header
        before it is sent.  The callback should return a negative code if an error
        occurred, zero can be returned if the application wants ProtoHttp to
        calculate the header size, or the size of the header can be returned if
        the application has already calculated it.  The header should *not* be
        terminated with the double \\r\\n that indicates the end of the entire
        header, as protohttp appends itself.

        The ProtoHttpReceiveHeaderCbT callback is used to view the header
        immediately on reception.  It can be used when the built-in header
        cache (retrieved with ProtoHttpStatus('htxt') is too small to hold
        the entire header received.  It is also possible with this method
        to view redirection response headers that cannot be retrieved
        normally.  This can be important if, for example, the application
        wishes to attach new cookies to a redirection response.  The
        custom response header and custom header callback can be used in
        conjunction to implement this type of functionality.

    \Version 09/27/2016 (eesponda)
*/
/********************************************************************************F*/
void ProtoHttp2Callback(ProtoHttp2RefT *pState, ProtoHttp2CustomHeaderCbT *pCustomHeaderCb, ProtoHttp2ReceiveHeaderCbT *pReceiveHeaderCb, void *pUserData)
{
    pState->pCustomHeaderCb = pCustomHeaderCb;
    pState->pReceiveHeaderCb = pReceiveHeaderCb;
    pState->pCallbackRef = pUserData;
}

/*F********************************************************************************/
/*!
    \Function ProtoHttp2Request

    \Description
        Initiate an HTTP transfer without callback. Pass in a URL and the module
        starts a transfer from the appropriate server.

    \Input *pState      - module state
    \Input *pUrl        - the url to retrieve
    \Input *pData       - user data to send to server (PUT and POST only)
    \Input iDataSize    - size of user data to send to server (PUT and POST only)
    \Input eRequestType - request type to make
    \Input *pStreamId   - [out] identifier tied to the request

    \Output
        int32_t         - negative=failure, zero=success, >0=number of data bytes sent (PUT and POST only)

    \Version 09/27/2016 (eesponda)
*/
/********************************************************************************F*/
int32_t ProtoHttp2Request(ProtoHttp2RefT *pState, const char *pUrl, const uint8_t *pData, int32_t iDataSize, ProtoHttpRequestTypeE eRequestType, int32_t *pStreamId)
{
    return(ProtoHttp2RequestCb(pState, pUrl, pData, iDataSize, eRequestType, pStreamId, NULL, NULL));
}

/*F********************************************************************************/
/*!
    \Function ProtoHttp2RequestCb

    \Description
        Initiate an HTTP transfer with callback. Pass in a URL and the module starts
        a transfer from the appropriate server.

    \Input *pState      - module state
    \Input *pUrl        - the url to retrieve
    \Input *pData       - user data to send to server (PUT and POST only)
    \Input iDataSize    - size of user data to send to server (PUT and POST only)
    \Input eRequestType - request type to make
    \Input *pStreamId   - [out] identifier tied to the request
    \Input *pWriteCb    - write callback (optional)
    \Input *pUserData   - write callback user data (optional)

    \Output
        int32_t         - negative=failure, zero=success, >0=number of data bytes sent (PUT and POST only)

    \Version 09/27/2016 (eesponda)
*/
/********************************************************************************F*/
int32_t ProtoHttp2RequestCb(ProtoHttp2RefT *pState, const char *pUrl, const uint8_t *pData, int32_t iDataSize, ProtoHttpRequestTypeE eRequestType, int32_t *pStreamId, ProtoHttp2WriteCbT *pWriteCb, void *pUserData)
{
    char strKind[8], strHost[sizeof(pState->strHost)];
    int32_t iPort, bSecure, iResult = 0;
    uint8_t bPortSpecified;
    StreamInfoT StreamInfo;

    // make sure we can create a new stream
    if (pState->iNumStreams >= PROTOHTTP2_MAX_STREAMS)
    {
        NetPrintf(("protohttp2: [%p] exceeded maximum number of concurrent stream\n", pState));
        return(-1);
    }
    if (pStreamId == NULL)
    {
        NetPrintf(("protohttp2: [%p] stream identifier output is NULL (required to save identifier)\n", pState));
        return(-2);
    }
    // make sure we can even attempt to fit the frame
    if ((iDataSize > 0) && (uint32_t)iDataSize > pState->LocalSettings.uMaxFrameSize)
    {
        NetPrintf(("protohttp2: [%p] data size exceeds size of the frame, use streaming instead\n", pState));
        return(-3);
    }

    // parse the url for kind, host and port
    if (pState->strProxy[0] == '\0')
    {
        pUrl = ProtoHttpUrlParse2(pUrl, strKind, sizeof(strKind), strHost, sizeof(strHost),
            &iPort, &bSecure, &bPortSpecified);
    }
    else
    {
        NetPrintfVerbose((pState->iVerbose, 0, "protohttp2: [%p] using proxy server %s\n", pState->strProxy));
        ProtoHttpUrlParse2(pState->strProxy, strKind, sizeof(strKind), strHost, sizeof(strHost),
            &iPort, &bSecure, &bPortSpecified);
    }

    // fill in any missing info (relative url) if available
    if (_ProtoHttp2ApplyBaseUrl(pState, strKind, strHost, sizeof(strHost), &iPort, &bSecure, bPortSpecified) != 0)
    {
        NetPrintfVerbose((pState->iVerbose, 0, "protohttp2: [%p] %s %s://%s:%d%s\n", pState, _ProtoHttp2_strRequestNames[eRequestType],
            bSecure ? "https" : "http", strHost, iPort, pUrl));
    }

    // are we still connected, connected to same endpoint and have valid stream id?
    if ((pState->eState > ST_IDLE) && (pState->eState < ST_FAIL) && (bSecure == pState->bSecure) && (ds_stricmp(strHost, pState->strHost) == 0) && (pState->iStreamId > 0))
    {
        NetPrintfVerbose((pState->iVerbose, 0, "protohttp2: [%p] reusing previous connection\n", pState));
    }
    else
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttp2: [%p] request new connection -- url change to %s\n", pState, strHost));

        // save new server/port/security state
        ds_strnzcpy(pState->strHost, strHost, sizeof(pState->strHost));
        pState->iPort = iPort;
        pState->bSecure = bSecure;

        // send goaway if necessary & close
        _ProtoHttp2SendGoAway(pState, ERRORTYPE_NO_ERROR, "new connection");

        // reset the state to default
        _ProtoHttp2Reset(pState);

        // start connect
        NetPrintfVerbose((pState->iVerbose, 0, "protohttp2: [%p] connect start (tick=%u)\n", pState, NetTick()));
        ProtoSSLConnect(pState->pSsl, pState->bSecure, pState->strHost, 0, pState->iPort);
        pState->eState = ST_CONN;
        pState->uTimer = NetTick() + pState->uTimeout;
    }

    // if using a proxy server, parse original url to get target host and port for authority header
    if (pState->strProxy[0] != '\0')
    {
        pUrl = ProtoHttpUrlParse2(pUrl, strKind, sizeof(strKind), strHost, sizeof(strHost),
            &iPort, &bSecure, &bPortSpecified);
    }

    // setup the stream info
    ds_memclr(&StreamInfo, sizeof(StreamInfo));
    StreamInfo.iStreamId = *pStreamId = pState->iStreamId;
    StreamInfo.eResponseCode = PROTOHTTP_RESPONSE_PENDING;
    StreamInfo.eRequestType = eRequestType;
    StreamInfo.pWriteCb = pWriteCb;
    StreamInfo.pUserData = pUserData;
    StreamInfo.iDataMax = StreamInfo.iLocalWindow = PROTOHTTP2_WINDOWSIZE; /* we never update our window size at runtime so assume this value */
    StreamInfo.iPeerWindow = pState->PeerSettings.uInitialWindowSize;

    NetCritEnter(&pState->HttpCrit);
    // attempt to format and encode request
    if (((iResult = _ProtoHttp2FormatRequestHeader(pState, &StreamInfo, pUrl, pData, iDataSize)) != 0) ||
        ((iResult = _ProtoHttp2EncodeRequest(pState, &StreamInfo, pData, iDataSize)) < 0))
    {
        _ProtoHttp2StreamInfoCleanup(pState, &StreamInfo);
        *pStreamId = PROTOHTTP2_INVALID_STREAMID;
    }
    else
    {
        // add the stream information for tracking
        ds_memcpy(&pState->Streams[pState->iNumStreams], &StreamInfo, sizeof(StreamInfo));
        pState->iNumStreams += 1;

        // increment to next stream id
        pState->iStreamId += 2;
    }
    NetCritLeave(&pState->HttpCrit);

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttp2Send

    \Description
        Send data during an ongoing request

    \Input *pState      - module state
    \Input iStreamId    - identifier of the stream to send the data on
    \Input *pData       - pointer to data to send
    \Input iDataSize    - size of data being sent

    \Output
        int32_t         - negative=PROTOHTTP2_MINBUFF or failure otherwise number of data
                          bytes sent

    \Notes
        In the case of PROTOHTTP2_MINBUFF we do not have enough space to encode
        the frame header. We need to return a special code due to the fact when sending
        a zero sized end stream we need to have a way to tell if it was actually encoded
        into the buffer. In this case you should retry to send on the next frame.

    \Version 09/27/2016 (eesponda)
*/
/********************************************************************************F*/
int32_t ProtoHttp2Send(ProtoHttp2RefT *pState, int32_t iStreamId, const uint8_t *pData, int32_t iDataSize)
{
    int32_t iResult = -1;
    StreamInfoT *pStreamInfo;

    if ((pStreamInfo = _ProtoHttp2StreamInfoGet(pState, iStreamId)) != NULL)
    {
        // make sure the stream is open to allow us to send data
        if (pStreamInfo->eState < STREAMSTATE_OPEN)
        {
            // not ready to send data yet
            return(0);
        }
        else if ((pStreamInfo->eState != STREAMSTATE_OPEN) && (pStreamInfo->eState != STREAMSTATE_HALF_CLOSED_REMOTE))
        {
            // we have already finished sending, an error occured
            return(-1);
        }

        NetCritEnter(&pState->HttpCrit);
        // encode the data into the output buffer
        if ((iResult = _ProtoHttp2EncodeData(pState, pStreamInfo, pData, iDataSize, (iDataSize == PROTOHTTP2_STREAM_END))) < 0)
        {
            NetPrintf(("protohttp2: [%p] not enough room in the output buffer to encode data\n", pState));
        }
        NetCritLeave(&pState->HttpCrit);
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttp2Recv

    \Description
        Return the actual url data.

    \Input *pState      - module state
    \Input iStreamId    - identifier of the stream to recv data from
    \Input *pBuffer     - buffer to store data in
    \Input iBufMin      - minimum number of bytes to return
    \Input iBufMax      - maximum number of bytes to return (buffer size)

    \Output
        int32_t     - negative=error, zero=no data available or bufmax <= 0, positive=number of bytes read

    \Version 09/27/2016 (eesponda)
*/
/********************************************************************************F*/
int32_t ProtoHttp2Recv(ProtoHttp2RefT *pState, int32_t iStreamId, uint8_t *pBuffer, int32_t iBufMin, int32_t iBufMax)
{
    StreamInfoT *pStreamInfo;
    int32_t iResult = PROTOHTTP2_RECVFAIL;

    if ((pStreamInfo = _ProtoHttp2StreamInfoGet(pState, iStreamId)) != NULL)
    {
        NetCritEnter(&pState->HttpCrit);
        iResult = _ProtoHttp2Read(pState, pStreamInfo, pBuffer, iBufMin, iBufMax);
        NetCritLeave(&pState->HttpCrit);
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttp2RecvAll

    \Description
        Return all of the url data.

    \Input *pState      - module state
    \Input iStreamId    - identifier of the stream to recv data from
    \Input *pBuffer     - buffer to store data in
    \Input iBufSize     - size of buffer

    \Output
        int32_t     - PROTOHTTP_RECV*, or positive=bytes in response

    \Version 09/27/2016 (eesponda)
*/
/********************************************************************************F*/
int32_t ProtoHttp2RecvAll(ProtoHttp2RefT *pState, int32_t iStreamId, uint8_t *pBuffer, int32_t iBufSize)
{
    StreamInfoT *pStreamInfo;
    int32_t iRecvMax = iBufSize-1, iRecvResult = PROTOHTTP2_RECVFAIL;

    if ((pStreamInfo = _ProtoHttp2StreamInfoGet(pState, iStreamId)) != NULL)
    {
        NetCritEnter(&pState->HttpCrit);
        // try to receive as much as possible, adding to amount received
        while ((iRecvResult = _ProtoHttp2Read(pState, pStreamInfo, pBuffer+pStreamInfo->iRecvSize, 1, iRecvMax-pStreamInfo->iRecvSize)) > 0)
        {
            pStreamInfo->iRecvSize += iRecvResult;
        }
        NetCritLeave(&pState->HttpCrit);
    }

    // check the response code
    if (iRecvResult == PROTOHTTP2_RECVDONE)
    {
        pBuffer[pStreamInfo->iRecvSize] = 0;
        iRecvResult = pStreamInfo->iRecvSize;
    }
    else if ((iRecvResult < 0) && (iRecvResult != PROTOHTTP2_RECVWAIT))
    {
        // an error occured
        NetPrintf(("protohttp2: [%p] error %d receiving response\n", pState, iRecvResult));
    }
    else if (iRecvResult == 0)
    {
        iRecvResult = (pStreamInfo->iRecvSize < iRecvMax) ? PROTOHTTP2_RECVWAIT : PROTOHTTP2_RECVBUFF;
    }

    // return result to caller
    return(iRecvResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttp2Abort

    \Description
        Abort current operation, if any.

    \Input *pState      - module state
    \Input iStreamId    - identifier of the stream to cancel

    \Notes
        This will send a RST_STREAM frame to the peer endpoint with the
        CANCEL (0x8) error code

    \Version 11/03/2016 (eesponda)
*/
/********************************************************************************F*/
void ProtoHttp2Abort(ProtoHttp2RefT *pState, int32_t iStreamId)
{
    StreamInfoT *pStreamInfo;

    // find stream id and make sure the stream is open to allow us to send data
    if (((pStreamInfo = _ProtoHttp2StreamInfoGet(pState, iStreamId)) != NULL) && (pStreamInfo->eState > STREAMSTATE_IDLE) && (pStreamInfo->eState < STREAMSTATE_CLOSED))
    {
        NetCritEnter(&pState->HttpCrit);
        _ProtoHttp2PrepareRstStream(pState, pStreamInfo, ERRORTYPE_CANCEL, "cancelling the stream");
        NetCritLeave(&pState->HttpCrit);
    }
}

/*F********************************************************************************/
/*!
    \Function ProtoHttp2Close

    \Description
        Close the connection to the server

    \Input *pState      - module state

    \Notes
        This will send a GOAWAY frame to the peer endpoint with the
        NO_ERROR (0x0) error code

    \Version 11/22/2016 (eesponda)
*/
/********************************************************************************F*/
void ProtoHttp2Close(ProtoHttp2RefT *pState)
{
    // if not connected then nothing left to do
    if ((pState->eState == ST_IDLE) || (pState->eState == ST_FAIL))
    {
        return;
    }

    _ProtoHttp2SendGoAway(pState, ERRORTYPE_NO_ERROR, "user request");
}

/*F********************************************************************************/
/*!
    \Function ProtoHttp2Status

    \Description
        Get status information

    \Input *pState      - module state
    \Input iStreamId    - stream identifier used by certain selectors
    \Input iSelect      - info selector (see Notes)
    \Input *pBuffer     - [out] storage for selector-specific output
    \Input iBufSize     - size of buffer

    \Output
        int32_t     - selector specific

    \Notes
        Selectors are:

    \verbatim
        SELECTOR    RETURN RESULT
        'addr'      returns address peer we are connecting/connected to, if available
        'alrt'      returns alert status 0=no alert, 1=recv alert, 2=sent alert; alert desc copied to pBuffer
        'body'      negative=failed or pending, else size of body (for 64bit size, get via pBuffer)
        'cert'      returns SSL cert information
        'code'      negative=no response, else server response code (ProtoHttpResponseE)
        'cfip'      returns TRUE/FALSE indication if a CA fetch is in progress
        'date'      returns last-modified data, if available
        'done'      returns status of request negative=failed, zero=pending or positive=done
        'essl'      returns protossl error state
        'head'      returns header size negative=failed or pending, otherwise header size
        'host'      current host copied to output buffer
        'hres'      returns hResult containing either the socket error, ssl error, or http status code
        'htxt'      response header for stream identified by iStreamId via output buffer
        'imax'      returns size of input buffer
        'nstm'      returns the number of active streams
        'port'      returns the current port
        'rtxt'      most http request header text copied to output buffer for stream
        'serr'      returns socket error
        'strm'      returns the state of the stream identified by iStreamId as ProtoHttp2StreamStateE
        'time'      TRUE if the client timed out the connection, else FALSE
    \endverbatim

    \Version 09/27/2016 (eesponda)
*/
/********************************************************************************F*/
int32_t ProtoHttp2Status(ProtoHttp2RefT *pState, int32_t iStreamId, int32_t iSelect, void *pBuffer, int32_t iBufSize)
{
    const StreamInfoT *pStreamInfo;

    // return protossl address
    if (iSelect == 'addr')
    {
        return(ProtoSSLStat(pState->pSsl, iSelect, pBuffer, iBufSize));
    }
    // return protossl alert status
    if (iSelect == 'alrt')
    {
        return(ProtoSSLStat(pState->pSsl, iSelect, pBuffer, iBufSize));
    }
    // return cert info
    if (iSelect == 'cert')
    {
        return(ProtoSSLStat(pState->pSsl, iSelect, pBuffer, iBufSize));
    }
    // return if a CA fetch request is in progress
    if (iSelect == 'cfip')
    {
        return(ProtoSSLStat(pState->pSsl, iSelect, pBuffer, iBufSize));
    }
    // return protossl error state (we cache this since we reset the state when we disconnect an error)
    if (iSelect == 'essl')
    {
        return(pState->iSslFail);
    }
    // return current host
    if (iSelect == 'host')
    {
        ds_strnzcpy(pBuffer, pState->strHost, iBufSize);
        return(0);
    }
    // return hresult containing either the socket error, ssl error or http status code
    if (iSelect == 'hres')
    {
        // validate stream id, stream information and status code
        if ((iStreamId > 0) && ((pStreamInfo = _ProtoHttp2StreamInfoGet(pState, iStreamId)) != NULL) && (pStreamInfo->eResponseCode > 0))
        {
            return(DirtyErrGetHResult(DIRTYAPI_PROTO_HTTP, pStreamInfo->eResponseCode, (pStreamInfo->eResponseCode >= PROTOHTTP_RESPONSE_CLIENTERROR) ? TRUE : FALSE));
        }
        else
        {
            return(pState->iHresult);
        }
    }
    // return size of input buffer
    if (iSelect == 'imax')
    {
        return(pState->iInpMax);
    }
    // return number of active streams
    if (iSelect == 'nstm')
    {
        return(pState->iNumStreams);
    }
    // return current port
    if (iSelect == 'port')
    {
        return(pState->iPort);
    }
    // return current socket errors
    if (iSelect == 'serr')
    {
        return(ProtoSSLStat(pState->pSsl, 'serr', NULL, 0));
    }
    // return timeout indicator
    if (iSelect == 'time')
    {
        return(pState->bTimeout);
    }

    // attempt to get the stream information for the below selectors where valid stream is required
    if ((iStreamId <= 0) || ((pStreamInfo = _ProtoHttp2StreamInfoGet(pState, iStreamId)) == NULL))
    {
        return(-1);
    }

    // return response code
    if (iSelect == 'code')
    {
        return(pStreamInfo->eResponseCode);
    }
    // return the header data
    if (iSelect == 'date')
    {
        return(pStreamInfo->iHdrDate);
    }
    // done check: negative=failed, zero=pending, positive=done
    if (iSelect == 'done')
    {
        if (pState->eState == ST_FAIL)
        {
            return(-1);
        }
        if (pStreamInfo->eState == STREAMSTATE_CLOSED)
        {
            return(1);
        }
        return(0);
    }
    // return the request header text
    if (iSelect == 'rtxt')
    {
        ds_strnzcpy(pBuffer, pStreamInfo->pRequestHeader, iBufSize);
        return(0);
    }
    // return the current state of the stream
    if (iSelect == 'strm')
    {
        return(pStreamInfo->eState);
    }

    /* check the state:
       if failure then nothing is happening, thus nothing left to do
       otherwise if we have yet to receive our headers then the remaining data has
       not been filled out yet */
    if (pState->eState == ST_FAIL)
    {
        return(-1);
    }
    if (pStreamInfo->eResponseCode == PROTOHTTP_RESPONSE_PENDING)
    {
        return(-2);
    }

    // negative=failed or pending, else size of body (for 64bit size, get via pBuffer)
    if (iSelect == 'body')
    {
        if ((pBuffer != NULL) && (iBufSize == sizeof(pStreamInfo->iBodySize)))
        {
            ds_memcpy(pBuffer, &pStreamInfo->iBodySize, iBufSize);
        }
        return((int32_t)pStreamInfo->iBodySize);
    }
    // return size of the header
    if (iSelect == 'head')
    {
        return(pStreamInfo->iHeaderLen);
    }
    // return the response header text
    if (iSelect == 'htxt')
    {
        ds_strnzcpy(pBuffer, pStreamInfo->pHeader, iBufSize);
        return(0);
    }

    // unhandled selector
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttp2Control

    \Description
        ProtoHttp2 control function.  Different selectors control different behaviors.

    \Input *pState      - module state
    \Input iStreamId    - stream identifier used by certain controls
    \Input iControl     - control selector (see Notes)
    \Input iValue       - control specific
    \Input iValue2      - control specific
    \Input *pValue      - control specific

    \Output
        int32_t     - control specific

    \Notes
        Selectors are:

        \verbatim
            SELECTOR    DESCRIPTION
            'apnd'      The given buffer will be appended to future headers sent
                        by ProtoHttp2. Note that the User-Agent and Accept lines
                        in the default header will be replaced, so if these lines
                        are desired, they should be supplied to the append header.
            'huff'      Sets the use of huffman encoding for strings (default=TRUE)
            'ires'      Resize input buffer
            'prxy'      Sets the proxy server
            'spam'      Sets debug output verbosity (0..n)
            'time'      Sets ProtoHttp client timeout in milliseconds (default=30s)
        \endverbatim

        Unhandled selectors are passed on to ProtoSSL.

    \Version 09/27/2016 (eesponda)
*/
/*******************************************************************************F*/
int32_t ProtoHttp2Control(ProtoHttp2RefT *pState, int32_t iStreamId, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iControl == 'apnd')
    {
        return(_ProtoHttp2SetAppendHeader(pState, (const char *)pValue));
    }
    if (iControl == 'huff')
    {
        pState->bHuffman = (uint8_t)iValue;
        return(0);
    }
    if (iControl == 'ires')
    {
        // clamp the input buffer size
        PROTOHTTP2_ClampFrameSize(iValue);

        // attempt to resize the buffer if necessary
        if (((uint32_t)iValue != pState->TempSettings.uMaxFrameSize) && (_ProtoHttp2ResizeInputBuffer(pState, iValue+PROTOHTTP2_RESERVED_SIZE) == 0))
        {
            // set the temp settings to be sent to our peer
            pState->TempSettings.uMaxFrameSize = (uint32_t)iValue;
            return(0);
        }
        return(-1);
    }
    if (iControl == 'prxy')
    {
        NetPrintfVerbose((pState->iVerbose, 0, "protohttp2: [%p] setting %s as proxy server\n", pState, (const char *)pValue));
        ds_strnzcpy(pState->strProxy, (const char *)pValue, sizeof(pState->strProxy));
        return(0);
    }
    if (iControl == 'spam')
    {
        pState->iVerbose = iValue;
        // fall through to protossl
    }
    if (iControl == 'time')
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttp2: [%p] setting timeout to %d ms\n", pState, iValue));
        pState->uTimeout = (unsigned)iValue;
        return(0);
    }

    // unhandled control, fallthrough to protossl
    return(ProtoSSLControl(pState->pSsl, iControl, iValue, iValue2, pValue));
}

/*F********************************************************************************/
/*!
    \Function ProtoHttp2SetBaseUrl

    \Description
        Set base url that will be used for any relative url references.

    \Input *pState      - module state
    \Input *pUrl        - base url

    \Version 09/27/2016 (eesponda)
*/
/********************************************************************************F*/
void ProtoHttp2SetBaseUrl(ProtoHttp2RefT *pState, const char *pUrl)
{
    char strKind[8];
    uint8_t bPortSpecified;

    // parse the url for kind, host and port
    ProtoHttpUrlParse2(pUrl, strKind, sizeof(strKind), pState->strBaseHost, sizeof(pState->strBaseHost),
        &pState->iBasePort, &pState->bBaseSecure, &bPortSpecified);

    NetPrintfVerbose((pState->iVerbose, 1, "protohttp2: [%p] setting base url to %s://%s:%d\n",
        pState, pState->bBaseSecure ? "https" : "http", pState->strBaseHost, pState->iBasePort));
}

/*F********************************************************************************/
/*!
    \Function ProtoHttp2GetLocationHeader

    \Description
        Get location header from the input buffer.  The Location header requires
        some special processing.

    \Input *pState  - reference pointer
    \Input *pInpBuf - buffer holding header text
    \Input *pBuffer - [out] output buffer for parsed location header, null for size request
    \Input iBufSize - size of output buffer, zero for size request
    \Input **pHdrEnd- [out] pointer past end of parsed header (optional)

    \Output
        int32_t     - negative=not found or not enough space, zero=success, positive=size query result

    \Version 09/27/2016 (eesponda)
*/
/********************************************************************************F*/
int32_t ProtoHttp2GetLocationHeader(ProtoHttp2RefT *pState, const char *pInpBuf, char *pBuffer, int32_t iBufSize, const char **pHdrEnd)
{
    const char *pLocHdr;
    int32_t iLocLen, iLocPreLen=0;

    // get a pointer to header
    if ((pLocHdr = ProtoHttpFindHeaderValue(pInpBuf, "location")) == NULL)
    {
        return(-1);
    }

    /* according to RFC location headers should be absolute, but some webservers respond with relative
       URL's.  we assume any url that does not include "://" is a relative url, and if we find one, we
       assume the request keeps the same security, port, and host as the previous request */
    if ((pState != NULL) && (!strstr(pLocHdr, "://")))
    {
        char strTemp[288]; // space for max DNS name (253 chars) plus max http url prefix

        // format http url prefix
        if ((pState->bSecure && (pState->iPort == 443)) || (pState->iPort == 80))
        {
            ds_snzprintf(strTemp, sizeof(strTemp), "%s://%s", pState->bSecure ? "https" : "http", pState->strHost);
        }
        else
        {
            ds_snzprintf(strTemp, sizeof(strTemp), "%s://%s:%d", pState->bSecure ? "https" : "http", pState->strHost, pState->iPort);
        }

        // make sure relative URL includes '/' as the first character, required when we format the redirection url
        if (*pLocHdr != '/')
        {
            ds_strnzcat(strTemp, "/", sizeof(strTemp));
        }

        // calculate url prefix length
        iLocPreLen = (int32_t)strlen(strTemp);

        // copy url prefix text if a buffer is specified
        if (pBuffer != NULL)
        {
            ds_strnzcpy(pBuffer, strTemp, iBufSize);
            pBuffer = (char *)((uint8_t *)pBuffer + iLocPreLen);
            iBufSize -= iLocPreLen;
        }
    }

    // extract location header and return size
    iLocLen = ProtoHttpExtractHeaderValue(pLocHdr, pBuffer, iBufSize, pHdrEnd);
    // if it's a size request add in (possible) url header length
    if ((pBuffer == NULL) && (iBufSize == 0))
    {
        iLocLen += iLocPreLen;
    }

    // return to caller
    return(iLocLen);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttp2StreamFree

    \Description
        Removes the stream information for the list by identifier

    \Input *pState      - module state
    \Input iStreamId    - identifier of the stream info to remove

    \Version 11/01/2016 (eesponda)
*/
/********************************************************************************F*/
void ProtoHttp2StreamFree(ProtoHttp2RefT *pState, int32_t iStreamId)
{
    StreamInfoT *pStreamInfo;

    if ((pStreamInfo = _ProtoHttp2StreamInfoGet(pState, iStreamId)) != NULL)
    {
        // get index based on pointer
        int32_t iStream = (int32_t)(pStreamInfo - pState->Streams);

        #if DIRTYCODE_LOGGING
        if (pStreamInfo->eState != STREAMSTATE_CLOSED)
        {
            NetPrintf(("protohttp2: [%p] warning: freeing stream state for a stream that is not yet closed\n", pState));
        }
        #endif

        // cleanup any dynamic memory
        _ProtoHttp2StreamInfoCleanup(pState, pStreamInfo);

        // remove entry from stream list
        if (iStream != (pState->iNumStreams-1))
        {
            int32_t iNumMove = (pState->iNumStreams-1) - iStream;

            // move the stream info to remove the gap
            memmove(pStreamInfo, pStreamInfo+1, iNumMove * sizeof(*pStreamInfo));
        }
        // decrement count
        pState->iNumStreams -= 1;
    }
}
