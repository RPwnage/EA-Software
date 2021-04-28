/*H********************************************************************************/
/*!
    \File protowebsocket.h

    \Description
        This module implements a WebSocket client as documented in RFC 6455
        (http://tools.ietf.org/html/rfc6455).  Note that implementation details
        may vary when a platform-specific implementation is required.  The API
        as currently implemented offers stream-like operations; message-based
        operations are currently not supported.

    \Todo
        - Message-based APIs
        - Server support (Accept, Listen, etc)

    \Copyright
        Copyright (c) 2012 Electronic Arts Inc.

    \Version 11/26/2012 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _protowebsocket_h
#define _protowebsocket_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

// close reasons
#define PROTOWEBSOCKET_CLOSEREASON_BASE             (1000)
#define PROTOWEBSOCKET_CLOSEREASON_NORMAL           (PROTOWEBSOCKET_CLOSEREASON_BASE+0)
#define PROTOWEBSOCKET_CLOSEREASON_GOINGAWAY        (PROTOWEBSOCKET_CLOSEREASON_BASE+1)
#define PROTOWEBSOCKET_CLOSEREASON_PROTOCOLERROR    (PROTOWEBSOCKET_CLOSEREASON_BASE+2)
#define PROTOWEBSOCKET_CLOSEREASON_UNSUPPORTEDDATA  (PROTOWEBSOCKET_CLOSEREASON_BASE+3)
#define PROTOWEBSOCKET_CLOSEREASON_NOSTATUSRECVD    (PROTOWEBSOCKET_CLOSEREASON_BASE+5)
#define PROTOWEBSOCKET_CLOSEREASON_ABNORMALCLOSURE  (PROTOWEBSOCKET_CLOSEREASON_BASE+6)
#define PROTOWEBSOCKET_CLOSEREASON_INVALIDFRAMEDATA (PROTOWEBSOCKET_CLOSEREASON_BASE+7)
#define PROTOWEBSOCKET_CLOSEREASON_POLICYVIOLATION  (PROTOWEBSOCKET_CLOSEREASON_BASE+8)
#define PROTOWEBSOCKET_CLOSEREASON_MESSAGETOOBIG    (PROTOWEBSOCKET_CLOSEREASON_BASE+9)
#define PROTOWEBSOCKET_CLOSEREASON_MANDATORYEXT     (PROTOWEBSOCKET_CLOSEREASON_BASE+10)
#define PROTOWEBSOCKET_CLOSEREASON_INTERNALERR      (PROTOWEBSOCKET_CLOSEREASON_BASE+11)
#define PROTOWEBSOCKET_CLOSEREASON_SERVICERESTART   (PROTOWEBSOCKET_CLOSEREASON_BASE+12)
#define PROTOWEBSOCKET_CLOSEREASON_TRYAGAINLATER    (PROTOWEBSOCKET_CLOSEREASON_BASE+13)
#define PROTOWEBSOCKET_CLOSEREASON_TLSHANDSHAKE     (PROTOWEBSOCKET_CLOSEREASON_BASE+15)
#define PROTOWEBSOCKET_CLOSEREASON_MAX              (PROTOWEBSOCKET_CLOSEREASON_TLSHANDSHAKE)
#define PROTOWEBSOCKET_CLOSEREASON_COUNT            (PROTOWEBSOCKET_CLOSEREASON_MAX-PROTOWEBSOCKET_CLOSEREASON_BASE+1)
// unidentified close reason
#define PROTOWEBSOCKET_CLOSEREASON_UNKNOWN          (PROTOWEBSOCKET_CLOSEREASON_BASE+100)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! opaque module state definition
typedef struct ProtoWebSocketRefT ProtoWebSocketRefT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// allocate a websocket connection and prepare for use
ProtoWebSocketRefT *ProtoWebSocketCreate(int32_t iBufSize);

// allocate a websocket connection and prepare for use (protossl function signature compatible)
ProtoWebSocketRefT *ProtoWebSocketCreate2(void);

// destroy a connection and release state
void ProtoWebSocketDestroy(ProtoWebSocketRefT *pWebSocket);

// accept an incoming connection
ProtoWebSocketRefT *ProtoWebSocketAccept(ProtoWebSocketRefT *pWebSocket);

// start listening for an incoming connection
ProtoWebSocketRefT *ProtoWebSocketListen(ProtoWebSocketRefT *pWebSocket);

// initiate a connection to a server
int32_t ProtoWebSocketConnect(ProtoWebSocketRefT *pWebSocket, const char *pUrl);

// initiate a connection to a server (protossl function signature compatible)
int32_t ProtoWebSocketConnect2(ProtoWebSocketRefT *pWebSocket, int32_t iUnused0, const char *pUrl, uint32_t uUnused1, int32_t iUnused2);

// disconnect from the server
int32_t ProtoWebSocketDisconnect(ProtoWebSocketRefT *pWebSocket);

// send data to a server over a websocket connection
int32_t ProtoWebSocketSend(ProtoWebSocketRefT *pWebSocket, const char *pBuffer, int32_t iLength);

// recv data from a server over a websocket connection
int32_t ProtoWebSocketRecv(ProtoWebSocketRefT *pWebSocket, char *pBuffer, int32_t iLength);

// get module status
int32_t ProtoWebSocketStatus(ProtoWebSocketRefT *pWebSocket, int32_t iSelect, void *pBuffer, int32_t iBufSize);

// control module behavior
int32_t ProtoWebSocketControl(ProtoWebSocketRefT *pWebSocket, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue);

// give time to module to do its thing (should be called periodically to allow module to perform work)
void ProtoWebSocketUpdate(ProtoWebSocketRefT *pWebSocket);

#ifdef __cplusplus
}
#endif

#endif // _protowebsocket_h

