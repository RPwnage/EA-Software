/*H********************************************************************************/
/*!
    \File testercomm_socket.c

    \Description
        This module provides a communication layer between the host and the client
        using socket I/O.  Typical operations are SendLine() and GetLine(), which
        send and receive lines of text, commands, debug output, etc.  Each platform
        may implement its own way of communicating - through files, debugger API
        calls, etc.

    \Copyright
        Copyright 2011 Electronic Arts Inc.

    \Version 10/17/2011 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/netconn.h"   // for NetConnSleep()
#include "DirtySDK/util/tagfield.h"
#include "zmem.h"
#include "zlib.h"
#include "zlist.h"
#include "zfile.h"

#include "testerprofile.h"
#include "testerregistry.h"
#include "testercomm.h"

/*** Defines **********************************************************************/

#define TESTERCOMM_OUTPUTTIMEOUT_DEFAULT    (5000) //!< timeout (ms) for a output message
#define T2COMM_PORT                         (3232) //!< communications port
#define T2COMM_USESOCKCALLBACK              (SOCKET_ASYNCRECVTHREAD) //!< TRUE=use socket callback for reading, FALSE=use polling

/*** Type Definitions *************************************************************/

//! module state
typedef struct TesterCommSocketT
{
    SocketT *pSocket;       //!< comm socket
    SocketT *pListen;       //!< listen socket used by host
    int32_t iPort;          //!< port to bind to

    #if T2COMM_USESOCKCALLBACK
    NetCritT zlistCrit;     //!< critical section used to make zlist access thread-safe
    #endif

    char strHostName[TESTERPROFILE_HOSTNAME_SIZEDEFAULT];   //!< host address to connect to
    enum
    {
        ST_INIT,
        ST_CONN,
        ST_LIST,
        ST_OPEN,
        ST_FAIL
    }eState;                    //!< module state
    uint8_t bIsHost;            //!< TRUE if host else FALSE
    char strInputData[16*1024]; //!< input buffer
} TesterCommSocketT;

/*** Private Function Prototypes **************************************************/

static int32_t _TesterCommDisconnect(TesterCommT *pState);
static int32_t _TesterCommCheckInput(TesterCommT *pState);

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _TesterCommConnect2

    \Description
        Connect the host client communication module.

    \Input *pSocketState    - module state

    \Output
        int32_t             - 0=success, else failure

    \Version 10/17/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _TesterCommConnect2(TesterCommSocketT *pSocketState)
{
    struct sockaddr SockAddr;
    uint32_t uHostAddr;
    int32_t iResult;

    // set up sockaddr
    SockaddrInit(&SockAddr, AF_INET);
    SockaddrInSetPort(&SockAddr, pSocketState->iPort);

    #if T2COMM_USESOCKCALLBACK
    NetCritInit(&pSocketState->zlistCrit, "zlist-crit");
    #endif

    // if we have somewhere to connect to, do it
    if (pSocketState->strHostName[0] != '\0')
    {
        // create the socket
        if ((pSocketState->pSocket = SocketOpen(AF_INET, SOCK_STREAM, 0)) == NULL)
        {
            ZPrintf("testercomm_socket: cannot create socket\n");
            pSocketState->eState = ST_FAIL;
            return(-1);
        }

        // check for invalid addr
        if ((uHostAddr = SocketInTextGetAddr(pSocketState->strHostName)) == 0)
        {
            ZPrintf("testercomm_socket: connect got invalid host address %s\n", pSocketState->strHostName);
            pSocketState->eState = ST_FAIL;
            return(-2);
        }

        // initiate connection
        SockaddrInSetAddr(&SockAddr, uHostAddr);
        if ((iResult = SocketConnect(pSocketState->pSocket, &SockAddr, sizeof(SockAddr))) != SOCKERR_NONE)
        {
            ZPrintf("testercomm_socket: error %d initiating connection to %s\n", iResult, pSocketState->strHostName);
            SocketClose(pSocketState->pSocket);
            pSocketState->pSocket = NULL;
            pSocketState->eState = ST_FAIL;
            return(-3);
        }
        pSocketState->eState = ST_CONN;
        ZPrintf("testercomm_socket: connect hostname=%a\n", uHostAddr);
    }
    else
    {
        struct sockaddr BindAddr;

        // create the socket
        if ((pSocketState->pListen = SocketOpen(AF_INET, SOCK_STREAM, 0)) == NULL)
        {
            ZPrintf("testercomm_socket: cannot create socket\n");
            pSocketState->eState = ST_FAIL;
            return(-4);
        }
        // bind the socket
        if ((iResult = SocketBind(pSocketState->pListen, &SockAddr, sizeof(SockAddr))) != SOCKERR_NONE)
        {
            ZPrintf("testercomm_socket: error %d binding to port %d\n", iResult, pSocketState->iPort);
            SocketClose(pSocketState->pListen);
            pSocketState->pListen = NULL;
            pSocketState->eState = ST_FAIL;
            ZPrintf("testercomm_socket: next bind will use a random port\n");
            pSocketState->iPort = 0;
            return(-5);
        }
        // listen on the socket
        if ((iResult = SocketListen(pSocketState->pListen, 2)) != SOCKERR_NONE)
        {
            ZPrintf("testercomm_socket: error %d listening on socket\n", iResult);
            SocketClose(pSocketState->pListen);
            pSocketState->pListen = NULL;
            pSocketState->eState = ST_FAIL;
            return(-6);
        }
        pSocketState->eState = ST_LIST;

        SocketInfo(pSocketState->pListen, 'bind', 0, &BindAddr, sizeof(BindAddr));
        ZPrintf("testercomm_socket: waiting for connection on port %d\n", SockaddrInGetPort(&BindAddr));
    }

    // done for now
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _TesterCommDisconnect2

    \Description
        Disconnect the host client communication module.

    \Input *pSocketState    - pointer to host client comm module

    \Output
        int32_t             - 0=success, error code otherwise

    \Version 10/17/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _TesterCommDisconnect2(TesterCommSocketT *pSocketState)
{
    ZPrintf("testercomm_socket: disconnect\n");
    // dispose of socket(s)
    if (pSocketState->pSocket != NULL)
    {
        SocketClose(pSocketState->pSocket);
        pSocketState->pSocket = NULL;
    }
    if (pSocketState->pListen != NULL)
    {
        SocketClose(pSocketState->pListen);
        pSocketState->pListen = NULL;
    }

    #if T2COMM_USESOCKCALLBACK
    NetCritKill(&pSocketState->zlistCrit);
    #endif

    // return success
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _TesterCommCallback

    \Description
        Socket callback

    \Input *pSocket     - Tester socket
    \Input iFlags       - callback flags
    \Input *pRef        - TesterComm ref

    \Output
        int32_t         - zero

    \Version 11/07/2012 (jbrookes)
*/
/********************************************************************************F*/
#if T2COMM_USESOCKCALLBACK
static int32_t _TesterCommCallback(SocketT *pSocket, int32_t iFlags, void *pRef)
{
    _TesterCommCheckInput((TesterCommT *)pRef);
    return(0);
}
#endif

/*F********************************************************************************/
/*!
    \Function _TesterCommCheckConnect

    \Description
        Check to see if a connection has been established

    \Input *pState      - module state

    \Output
        int32_t         - 1=open, 0=connecting, negative=error

    \Version 10/17/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _TesterCommCheckConnect(TesterCommT *pState)
{
    TesterCommSocketT *pSocketState = pState->pInterface->pData;
    int32_t iResult = 0;

    if (pSocketState->eState == ST_INIT)
    {
        //??
    }
    else if (pSocketState->eState == ST_OPEN)
    {
        // connected
        iResult = 1;
    }
    else if (pSocketState->eState == ST_CONN)
    {
        // waiting for connection to be complete
        if ((iResult = SocketInfo(pSocketState->pSocket, 'stat', 0, NULL, 0)) > 0)
        {
            ZPrintf("testercomm_socket: connection complete\n");
            pSocketState->eState = ST_OPEN;
            // set the tick time so we don't automatically get a timeout
            pState->uLastSendTime = ZTick();

            // set up socket callback, if enabled, and enable TCP socket for async recv (disabled by default)
            #if T2COMM_USESOCKCALLBACK
            SocketCallback(pSocketState->pSocket, CALLB_RECV, 0, pState, _TesterCommCallback);
            SocketControl(pSocketState->pSocket, 'arcv', 1, NULL, NULL);
            #endif
        }
        else if (iResult < 0)
        {
            pSocketState->eState = ST_FAIL;
            iResult = -1;
        }
    }
    else if (pSocketState->eState == ST_LIST)
    {
        struct sockaddr SockAddr;
        int32_t iSockLen = sizeof(SockAddr);

        if ((pSocketState->pSocket = SocketAccept(pSocketState->pListen, &SockAddr, &iSockLen)) != NULL)
        {
            ZPrintf("testercomm_socket: accepted incoming connection request\n");
            // transition to connect state
            pSocketState->eState = ST_CONN;
        }
    }
    else // ST_FAIL
    {
        if (pSocketState->strHostName[0] == '\0')
        {
            ZPrintf("testercomm_socket: connection failed; setting up for reconnect\n");
            _TesterCommDisconnect2(pSocketState);
            _TesterCommConnect2(pSocketState);
        }
        else
        {
            ZPrintf("testercomm_socket: connection failed\n");
            pSocketState->eState = ST_INIT;
        }
        iResult = -1;
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _TesterCommCheckInput

    \Description
        Check for data coming from the other side (host or client) and pull
        any data into our internal buffer, if possible.

    \Input *pState - pointer to host client comm module

    \Output
        int32_t     - 0 = no data, >0 = data, error code otherwise

    \Version 10/17/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _TesterCommCheckInput(TesterCommT *pState)
{
    TesterCommSocketT *pSocketState = pState->pInterface->pData;
    int32_t iBytesRecv, iLinesRecv, iResult;
    TesterCommDataT LineData;
    char *pInputLoop;

    // check for data
    if ((iResult = SocketRecv(pSocketState->pSocket, pSocketState->strInputData, sizeof(pSocketState->strInputData), 0)) == 0)
    {
        // no data to process
        return(0);
    }
    else if (iResult < 0)
    {
        // an error occurred
        ZPrintf("testercomm_socket: error %d receiving data\n", iResult);
        pSocketState->eState = ST_FAIL;
        return(-1);
    }

    // remember that we got input
    pState->bGotInput = TRUE;
    iBytesRecv = iResult;

    // push each line onto the list
    for (iLinesRecv = 0, pInputLoop = strtok(pSocketState->strInputData, "\r\n"); pInputLoop != NULL; pInputLoop = strtok(NULL, "\r\n"), iLinesRecv += 1)
    {
        ZMemSet(&LineData, 0, sizeof(LineData));
        LineData.iType = TagFieldGetNumber(TagFieldFind(pInputLoop, "TYPE"), 0);
        TagFieldGetString(TagFieldFind(pInputLoop, "TEXT"), LineData.strBuffer, sizeof(LineData.strBuffer) - 1, "");
        // add to the back of the list; remember to add one for the terminator

        #if T2COMM_USESOCKCALLBACK
        NetCritEnter(&pSocketState->zlistCrit);
        #endif

        ZListPushBack(pState->pInputData, &LineData);

        #if T2COMM_USESOCKCALLBACK
        NetCritLeave(&pSocketState->zlistCrit);
        #endif
    }

    //ZPrintf2("testercomm_socket: recv %d lines (%d bytes)\n", iLinesRecv, iBytesRecv);

    // done - return how much we got from the file
    return(iLinesRecv);
}

/*F********************************************************************************/
/*!
    \Function _TesterCommCheckOutput

    \Description
        Check and send data from the output buffer, if possible.

    \Input *pState  - pointer to host client comm module

    \Output
        int32_t     - 0=success, error code otherwise

    \Version 10/17/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _TesterCommCheckOutput(TesterCommT *pState)
{
    TesterCommSocketT *pSocketState = pState->pInterface->pData;
    char strLineOut[TESTERCOMM_COMMANDSIZE_DEFAULT];
    int32_t iLineLength, iLinesSent, iBytesSent, iResult, iTickDiff;
    TesterCommDataT LineData;
    char cDivider;
    void *pPeekResult;

    #if T2COMM_USESOCKCALLBACK
    NetCritEnter(&pSocketState->zlistCrit);
    #endif

    pPeekResult = ZListPeekFront(pState->pOutputData);

    #if T2COMM_USESOCKCALLBACK
    NetCritLeave(&pSocketState->zlistCrit);
    #endif

    // see if there's any data to send
    if (pPeekResult == NULL)
    {
        // reset the last send time - we're not timing out because there's no data
        pState->uLastSendTime = ZTick();
        return(0);
    }

    // see if we've timed out
    if ((iTickDiff = NetTickDiff(ZTick(), pState->uLastSendTime)) > TESTERCOMM_OUTPUTTIMEOUT_DEFAULT)
    {
        #if T2COMM_USESOCKCALLBACK
        NetCritEnter(&pSocketState->zlistCrit);
        #endif

        // timeout occurred - dump all pending messages and pop a warning
        ZListClear(pState->pOutputData);

        #if T2COMM_USESOCKCALLBACK
        NetCritLeave(&pSocketState->zlistCrit);
        #endif
        
        ZPrintf("testercomm_socket: TIMEOUT in sending data (%dms).  List cleared.  Comms Lost?\n", iTickDiff);
        pSocketState->eState = ST_FAIL;
    }

    // push output queue stuff by sending commands
    cDivider = TagFieldDivider('\t');

    #if T2COMM_USESOCKCALLBACK
    NetCritEnter(&pSocketState->zlistCrit);
    #endif

    // timeout occurred - dump all pending messages and pop a warning
    pPeekResult = ZListPeekFront(pState->pOutputData);

    #if T2COMM_USESOCKCALLBACK
    NetCritLeave(&pSocketState->zlistCrit);
    #endif

    for (iLinesSent = 0, iBytesSent = 0, cDivider = TagFieldDivider('\t'); pPeekResult; iLinesSent += 1)
    {
        char *pWrite;

        #if T2COMM_USESOCKCALLBACK
        NetCritEnter(&pSocketState->zlistCrit);
        #endif

        // snag the data into the local buffer
        ZListPopFront(pState->pOutputData, &LineData);

        #if T2COMM_USESOCKCALLBACK
        NetCritLeave(&pSocketState->zlistCrit);
        #endif


        // create the stuff to write
        TagFieldClear(strLineOut, sizeof(strLineOut));
        TagFieldSetNumber(strLineOut, sizeof(strLineOut)-1, "TYPE", LineData.iType);
        iResult = TagFieldSetString(strLineOut, sizeof(strLineOut)-1, "TEXT", LineData.strBuffer);
        if (iResult < 0)
        {
            // Sometimes, the text we get in will be too big.  Make sure this is an easy error to diagnose.
            ZPrintf("testercomm_socket: text too large to send in TESTERCOMM_COMMANDSIZE_DEFAULT.  Discarding.\n");
        }
        iLineLength = (int32_t)strlen(strLineOut);
        strLineOut[iLineLength++] = '\r';
        strLineOut[iLineLength++] = '\n';

        // send it
        pWrite = strLineOut;
        while (iLineLength != 0)
        {
            if ((iResult = SocketSend(pSocketState->pSocket, pWrite, iLineLength, 0)) < 0)
            {
                ZPrintf("testercomm_socket: error %d sending command to remote target\n", iResult);
                pSocketState->eState = ST_FAIL;
                return(-1);
            }
            else
            {
                iLineLength -= iResult;
                pWrite += iResult;
                iBytesSent += iResult;
            }

            if (iLineLength != 0)
            {
                NetConnSleep(5);
            }
        }

        #if T2COMM_USESOCKCALLBACK
        NetCritEnter(&pSocketState->zlistCrit);
        #endif

        // timeout occurred - dump all pending messages and pop a warning
        pPeekResult = ZListPeekFront(pState->pOutputData);

        #if T2COMM_USESOCKCALLBACK
        NetCritLeave(&pSocketState->zlistCrit);
        #endif
    }

    //ZPrintf2("testercomm_socket: sent %d lines (%d bytes)\n", iLinesSent, iBytesSent);

    // close out the send operation
    TagFieldDivider(cDivider);

    // mark the last send time
    pState->uLastSendTime = ZTick();
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _TesterCommConnect

    \Description
        Connect the host client communication module.

    \Input *pState  - pointer to host client comm module
    \Input *pParams - startup parameters
    \Input bIsHost  - TRUE=host, FALSE=client

    \Output
        int32_t     - 0 for success, error code otherwise

    \Version 10/17/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _TesterCommConnect(TesterCommT *pState, const char *pParams, uint32_t bIsHost)
{
    TesterCommSocketT *pSocketState = pState->pInterface->pData;

    // remember host status
    pSocketState->bIsHost = (uint8_t)bIsHost;
    pSocketState->iPort = T2COMM_PORT;

    // get startup parameters
    TagFieldGetString(TagFieldFind(pParams, "HOSTNAME"),  pSocketState->strHostName, sizeof(pSocketState->strHostName), "");

    // do the connect
    return(_TesterCommConnect2(pSocketState));
}

/*F********************************************************************************/
/*!
    \Function _TesterCommUpdate

    \Description
        Give the host/client interface module some processor time.  Call this
        once in a while to pump the input and output pipes.

    \Input *pState  - module state

    \Output
        int32_t     - 0 for success, error code otherwise

    \Version 10/17/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _TesterCommUpdate(TesterCommT *pState)
{
    #if T2COMM_USESOCKCALLBACK
    TesterCommSocketT *pSocketState = pState->pInterface->pData;
    #endif

    TesterCommDataT Entry;
    int32_t iResult;

    // quit if we are suspended (don't do any more commands)
    if (pState->uSuspended)
    {
        return(0);
    }

    // check for accepted connection
    if (_TesterCommCheckConnect(pState) <= 0)
    {
        return(0);
    }

    // check for outgoing and incoming data
    _TesterCommCheckOutput(pState);
    #if !T2COMM_USESOCKCALLBACK
    _TesterCommCheckInput(pState);
    #endif

    #if T2COMM_USESOCKCALLBACK
    NetCritEnter(&pSocketState->zlistCrit);
    #endif

    // now call the callbacks for incoming messages
    iResult = ZListPopFront(pState->pInputData, &Entry);

    #if T2COMM_USESOCKCALLBACK
    NetCritLeave(&pSocketState->zlistCrit);
    #endif

    while(iResult > 0)
    {
        // try to access the message map
        if (pState->MessageMap[Entry.iType] != NULL)
        {
            // protect against recursion by suspending commands until this one completes
            TesterCommSuspend(pState);
            (pState->MessageMap[Entry.iType])(pState, Entry.strBuffer, pState->pMessageMapUserData[Entry.iType]);
            TesterCommWake(pState);
        }

        #if T2COMM_USESOCKCALLBACK
        NetCritEnter(&pSocketState->zlistCrit);
        #endif

        // try to get the next chunk
        iResult = ZListPopFront(pState->pInputData, &Entry);

        #if T2COMM_USESOCKCALLBACK
        NetCritLeave(&pSocketState->zlistCrit);
        #endif
    }

    // done
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _TesterCommDisconnect

    \Description
        Disconnect the host client communication module.

    \Input *pState  - pointer to host client comm module

    \Output
        int32_t     - 0=success, error code otherwise

    \Version 10/17/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _TesterCommDisconnect(TesterCommT *pState)
{
    return(_TesterCommDisconnect2(pState->pInterface->pData));
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function TesterCommAttachSocket

    \Description
        Attach file module function pointers to a tester comm module.

    \Input *pState      - pointer to host client comm module

    \Version 10/17/2011 (jbrookes)
*/
/********************************************************************************F*/
void TesterCommAttachSocket(TesterCommT *pState)
{
    ZPrintf("testercomm_socket: attaching socket interface methods\n");
    pState->pInterface->CommConnectFunc = &_TesterCommConnect;
    pState->pInterface->CommUpdateFunc = &_TesterCommUpdate;
    pState->pInterface->CommDisconnectFunc = &_TesterCommDisconnect;
    pState->pInterface->pData = (TesterCommSocketT *)ZMemAlloc(sizeof(TesterCommSocketT));
    ZMemSet(pState->pInterface->pData, 0, sizeof(TesterCommSocketT));
}
