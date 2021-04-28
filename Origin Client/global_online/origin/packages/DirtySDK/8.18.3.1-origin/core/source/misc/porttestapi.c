/*H********************************************************************************/
/*!
    \File porttestapi.c

    \Description
        PortTest is used for testing port connectivity between
        a central server and a client using UDP packets.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 03/09/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "dirtysock.h"
#include "dirtymem.h"
#include "netconn.h"
#include "protoudp.h"
#include "porttestpkt.h"
#include "porttestapi.h"

/*** Defines **********************************************************************/

// set this to TRUE if more output is desired
#define PORTTESTER_VERBOSE  (DIRTYCODE_DEBUG && FALSE)

#define PORTTESTER_MAXINCOMINGBUFFERREDPACKETS      (10)    //!< Max number of incoming pkts to buffer
#define PORTTESTER_MAXOUTGOINGBUFFERREDPACKETS      (10)    //!< Max number of outgoing pkts to buffer
#define PORTTESTER_MAXPACKETSIZE                    (1024)  //!< Maximum size of each packet
#define PORTTESTER_REQUESTPACKETSIZE                (1024)  //!< Size of the request packet

#define PORTTESTER_SERVERSTRINGSIZE                 (256)
#define PORTTESTER_TITLESTRINGSIZE                  (64)
#define PORTTESTER_USERSTRINGSIZE                   (16)

/*** Type Definitions *************************************************************/

struct PortTesterT
{
    // module memory group
    int32_t iMemGroup;                         //!< module mem group id
    void *pMemGroupUserData;                   //!< user data associated with mem group;
    
    int32_t iStatus;                           //!< module status
    uint8_t uConnected;                        //!< whethe we are connected
    ProtoUdpT *pOutgoingUDPRef;                //!< outgoing protoudp module pointer
    ProtoUdpT *pIncomingUDPRef;                //!< incoming protoudp module pointer
    int32_t iOutgoingPortNum;                  //!< port to send to
    int32_t iIncomingPortNum;                  //!< port to receive on
    int32_t iNumSendPackets;                   //!< number of packets to send
    int32_t iTotalPacketsToReceive;            //!< number of packets to receive - might be a clamped number from the server
    int32_t iNumReceivedPackets;               //!< number of packets received
    int32_t iSequenceNum;                      //!< current sequence number
    int32_t iServerIP;                         //!< ip address of the server
    int32_t iStatusLast;                       //!< module status the last time we updated
    int32_t iNumReceivedPacketsLast;           //!< the last number since we updated
    int32_t iSendStartTime;                    //!< time since the send started
    int32_t iCurrentTime;                      //!< last time the update was called
    int32_t iTimeout;                          //!< timeout in milliseconds
    PortTesterCallbackT *pPortTesterUserCB;    //!< user callback for status updates
    void *pUserData;                           //!< user data for user callback
    char strTitle[PORTTESTER_TITLESTRINGSIZE]; //!< game title
    char strUser[PORTTESTER_USERSTRINGSIZE];   //!< user name
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _PortTesterCallUserCallback

    \Description
        Call the user-supplied callback, if provided.

    \Input  *pRef - module state

    \Output None

    \Version 03/09/2005 (jfrank)
*/
/********************************************************************************F*/
static void _PortTesterCallUserCallback(PortTesterT *pRef)
{
    // call the user callback if it's available
    if (pRef->pPortTesterUserCB != NULL)
    {
        pRef->pPortTesterUserCB(pRef, pRef->iStatus, pRef->pUserData);
    }
}

/*F********************************************************************************/
/*!
    \Function _PortTesterSendPackets

    \Description
        Send packets to the server.

    \Input  *pRef - module state

    \Output None

    \Version 03/09/2005 (jfrank)
*/
/********************************************************************************F*/
static void _PortTesterSendPackets(PortTesterT *pRef)
{
    unsigned char strPkt[PORTTESTER_REQUESTPACKETSIZE];
    DirtyPortRequestPktT OutPkt;
    struct sockaddr_in ServerAddr;
    struct sockaddr *pServerAddr = (struct sockaddr *)&ServerAddr;
    int32_t iResult;

    // construct the outgoing packet structure
    OutPkt.iCount = pRef->iNumSendPackets;
    OutPkt.iPort = pRef->iIncomingPortNum;
    OutPkt.iSeqn = pRef->iSequenceNum;
    strnzcpy(OutPkt.strTitle, pRef->strTitle, sizeof(OutPkt.strTitle));
    strnzcpy(OutPkt.strUser, pRef->strUser, sizeof(OutPkt.strUser));
    
    // set up the server address to send to
    ServerAddr.sin_family = AF_INET;
    // must cast integers 
    SockaddrInSetAddr((struct sockaddr *)&ServerAddr, ((uint32_t)(pRef->iServerIP)));
    SockaddrInSetPort((struct sockaddr *)&ServerAddr, ((uint16_t)(pRef->iOutgoingPortNum)));

    #if PORTTESTER_VERBOSE
    NetPrintf(("\nporttester----> iServerIP [0x%08X] pServerAddr [0x%08X] pServerAddrPort[0x%X]\n\n",pRef->iServerIP, ServerAddr.sin_addr.s_addr, ServerAddr.sin_port));
    #endif

    // now create the packet to send
    DirtyPortRequestEncode(&OutPkt, strPkt, sizeof(strPkt));

    iResult = ProtoUdpSendTo(pRef->pOutgoingUDPRef, (char *)strPkt, DirtyPortRequestSize(), pServerAddr);
    if (iResult < 0)
    {
        NetPrintf(("porttester: _PortTesterSendPackets got error from ProtoUdpSendTo [%d]\n", iResult));
        pRef->iStatus = PORTTESTER_STATUS_ERROR;
        return;
    }
    else 
    {
        #if PORTTESTER_VERBOSE
        NetPrintf(("porttester: _PortTesterSendPackets successfully sent [%d] bytes.\n", iResult));
        #endif
    }
}

/*F********************************************************************************/
/*!
    \Function _PortTesterCheckForResponse

    \Description
        Check to see if the server's responded to our requests
        and unload any responses.

    \Input  *pRef - module state

    \Output None

    \Version 03/09/2005 (jfrank)
*/
/********************************************************************************F*/
static void _PortTesterCheckForResponse(PortTesterT *pRef)
{
    unsigned char strPkt[PORTTESTER_REQUESTPACKETSIZE];
    DirtyPortResponsePktT InPkt;
    int32_t iResult;

    // check for a response
    while((iResult = ProtoUdpRecvFrom(pRef->pIncomingUDPRef, (char *)strPkt, sizeof(strPkt), NULL)) > 0)
    {
        iResult = DirtyPortResponseDecode(&InPkt, strPkt, sizeof(strPkt));
        if (iResult == 0)
        {
            NetPrintf(("porttester: DirtyPortResponseDecode could not decode packet.\n"));
        }
        else
        {
            // only accept it if the sequence matches
            if(pRef->iSequenceNum == InPkt.iSeqn)
            {
                #if PORTTESTER_VERBOSE
                NetPrintf(("porttester: Incoming Packet --> index[%d] count[%d] seqn[%d] title[%s] user[%s]\n",
                    InPkt.iIndex, InPkt.iCount, InPkt.iSeqn, InPkt.strTitle, InPkt.strUser));
                #endif
                // set the number of packets to actually receive from the server
                // since the value we wanted might be clamped to a lower value
                pRef->iTotalPacketsToReceive = InPkt.iCount;
                pRef->iNumReceivedPackets++;
            }
            else
            {
                #if PORTTESTER_VERBOSE
                NetPrintf(("porttester: Ignoring packet with old sequence number [%d] because current sequence number is [%d]\n",
                    InPkt.iSeqn, pRef->iSequenceNum));
                #endif
            }
        }
    }

    // check for an error code
    if (iResult < 0)         // error code
    {
        NetPrintf(("porttester: _PortTesterCheckForResponse got error from ProtoUdpRecvFrom [%d]\n", iResult));
        pRef->iStatus = PORTTESTER_STATUS_ERROR;
        return;
    }

}

/*F********************************************************************************/
/*!
    \Function _PortTesterUpdate

    \Description
        _PortTesterUpdate checks the state of the incoming packets and sets
            the status of the PortTesterModule accordingly.  This is where the
            optional user callback will get called with any status changes.

    \Input *pData   - port tester state
    \Input uData    - unused

    \Output None

    \Version 1.0 10/11/2004 (jfrank) First Version
*/
/********************************************************************************F*/
static void _PortTesterUpdate(void *pData, uint32_t uData)
{
    PortTesterT *pRef = (PortTesterT *)pData;

    // pump the protoudp modules
    ProtoUdpUpdate(pRef->pIncomingUDPRef);
    ProtoUdpUpdate(pRef->pOutgoingUDPRef);

    // now do something based on the state of the module
    switch(pRef->iStatus)
    {
    case PORTTESTER_STATUS_IDLE:
        // nothing to do, but see if we have a title already
        if (pRef->strTitle[0] == '\0')
        {
            // $$ TODO : V6 offer an api to specify thoe two values.
            strnzcpy(pRef->strUser, "[no user]", sizeof(pRef->strUser));
            strnzcpy(pRef->strTitle, "[no slus]", sizeof(pRef->strTitle));
        }
        break;
    case PORTTESTER_STATUS_SENDING:
        // send a packet
        _PortTesterSendPackets(pRef);
        // advance the state
        pRef->iStatus = PORTTESTER_STATUS_SENT;
        _PortTesterCallUserCallback(pRef);  // we've changed, so call the callback
        break;
    case PORTTESTER_STATUS_SENT:
    case PORTTESTER_STATUS_RECEIVING:
        // get the current time
        pRef->iCurrentTime = NetTick();

        // look for any packets which have been received
        _PortTesterCheckForResponse(pRef);

        // see if we've gotten any packets in
        if (pRef->iNumReceivedPackets > pRef->iNumReceivedPacketsLast)
        {
            // and adjust the state if necessary
            if (pRef->iNumReceivedPackets > 0)
                pRef->iStatus = PORTTESTER_STATUS_RECEIVING;
            _PortTesterCallUserCallback(pRef);  // we've changed, so call the callback
        }

        // now see if we've gotten all the packets in
        if (pRef->iNumReceivedPackets >= pRef->iTotalPacketsToReceive)
        {
            // call the callback for the last packet
            _PortTesterCallUserCallback(pRef); 
            pRef->iStatus = PORTTESTER_STATUS_SUCCESS;
            // and call the callback for the success state
            _PortTesterCallUserCallback(pRef); 
        }

        // if we're not done and we're stalled, check for a timeout
        if ((pRef->iCurrentTime - pRef->iSendStartTime) > pRef->iTimeout)
        {
            // we've timed out, but see if we got any packets in
            if(pRef->iNumReceivedPackets > 0)
                pRef->iStatus = PORTTESTER_STATUS_PARTIALSUCCESS;
            // no packets came in - complete timeout, a failure condition
            else
                pRef->iStatus = PORTTESTER_STATUS_TIMEOUT;
            // call the callback in either case
            _PortTesterCallUserCallback(pRef);  // we've changed, so call the callback
        }
        break;
    case PORTTESTER_STATUS_SUCCESS:
        // got all the packets - success
        NetPrintf(("porttester: _PortTesterUpdate --> SUCCESS.\n"));
        pRef->iStatus = PORTTESTER_STATUS_IDLE;
        break;
    case PORTTESTER_STATUS_PARTIALSUCCESS:
        // got some the packets - partial success
        NetPrintf(("porttester: _PortTesterUpdate --> PARTIALSUCCESS.\n"));
        pRef->iStatus = PORTTESTER_STATUS_IDLE;
        break;
    case PORTTESTER_STATUS_TIMEOUT:
        // timeout
        NetPrintf(("porttester: _PortTesterUpdate --> TIMEOUT.\n"));
        pRef->iStatus = PORTTESTER_STATUS_IDLE;
        break;
    case PORTTESTER_STATUS_ERROR:
        // some sort of error
        NetPrintf(("porttester: _PortTesterUpdate --> ERROR.\n"));
        pRef->iStatus = PORTTESTER_STATUS_IDLE;
        break;
    default:
        // should not get here
        NetPrintf(("porttester: _PortTesterUpdate --> UNKNOWN State [%d].  Forcing ERROR state.\n", pRef->iStatus));
        pRef->iStatus = PORTTESTER_STATUS_ERROR;
        _PortTesterCallUserCallback(pRef);  // we've changed, so call the callback
    }


    // update the last counts
    pRef->iStatusLast = pRef->iStatus;
    pRef->iNumReceivedPacketsLast = pRef->iNumReceivedPackets;
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function PortTesterCreate

    \Description
        Create a port tester module and connect it to the lobby.

    \Input  iServerIP   - IP address of the server to use
    \Input  iServerPort - outgoing port to the server to use
    \Input  iTimeout    - timeout in milliseconds for all packets

    \Output PortTesterT *   - pointer to a port tester object

    \Version 03/09/2005 (jfrank)
*/
/********************************************************************************F*/
PortTesterT *PortTesterCreate(int32_t iServerIP, int32_t iServerPort, int32_t iTimeout)
{
    PortTesterT *pRef;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);
    
    // allocate and init module state
    if ((pRef = (PortTesterT *)DirtyMemAlloc(sizeof(*pRef), PORTTESTER_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("porttester: could not allocate module state\n"));
        return(NULL);
    }
    memset(pRef, 0, sizeof(*pRef));
    pRef->iMemGroup = iMemGroup;
    pRef->pMemGroupUserData = pMemGroupUserData;

    // copy the startup params to local variables
    pRef->iStatus = PORTTESTER_STATUS_IDLE;
    pRef->iServerIP = iServerIP;
    pRef->iOutgoingPortNum = iServerPort;
    pRef->iTimeout = iTimeout;

    // initialize the protoudp modules
    pRef->pIncomingUDPRef = ProtoUdpCreate(PORTTESTER_MAXPACKETSIZE, PORTTESTER_MAXINCOMINGBUFFERREDPACKETS);
    pRef->pOutgoingUDPRef = ProtoUdpCreate(PORTTESTER_MAXPACKETSIZE, PORTTESTER_MAXOUTGOINGBUFFERREDPACKETS);

    // register the idle function
    NetConnIdleAdd(_PortTesterUpdate, pRef);

    // return module reference
    return(pRef);
}

/*F********************************************************************************/
/*!
    \Function PortTesterConnect

    \Description
        Connect a port tester module to a lobby ref.

    \Input *pRef    - PortTester module reference
    \Input *unused  - unused

    \Output None

    \Version 03/09/2005 (jfrank)
*/
/********************************************************************************F*/
void PortTesterConnect(PortTesterT *pRef, void* unused)
{
    // check to see if we're already connected
    if (pRef->uConnected)
    {
        NetPrintf(("porttester: PortTester already connected, not reconnecting.\n"));
        return;
    }

    // keep a reference to the lobby around for use in _LobbyLoggerUpdate
    pRef->uConnected = TRUE;

    // advance the sequence number
    pRef->iSequenceNum++;
}

/*F********************************************************************************/
/*!
    \Function PortTesterStatus

    \Description
        Return PortTester info

    \Input *pRef    - reference pointer
    \Input iSelect  - output selector
    \Input *pBuf    - output buffer
    \Input iBufSize - output buffer size
    
    \Output
        int32_t     - selector specific

    \Notes
        iKind can be one of the following:

        \verbatim
            'call' - user callback pointer
            'ctme' - current time (last time the update function was called)
            'iprt' - incoming port number
            'oprt' - outgoing port number
            'recv' - number of packets received
            'seqn' - current sequence number
            'spkt' - number of server packets which will be coming back (set by incoming packets)
            'srvr' - server IP address
            'stat' - module status (PORTTESTER_STATUS_XXXX)
            'stme' - time when the packets were sent
            'time' - timeout value
            'titl' - game title
            'user' - username
        \endverbatim

    \Version 03/09/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t PortTesterStatus(PortTesterT *pRef, int32_t iSelect, void *pBuf, int32_t iBufSize)
{
    if (pRef == NULL)
    {
        return(-1);
    }

    // return module info
    if (iSelect == 'ctme')    return(pRef->iCurrentTime);
    if (iSelect == 'iprt')    return(pRef->iIncomingPortNum);
    if (iSelect == 'oprt')    return(pRef->iOutgoingPortNum);
    if (iSelect == 'recv')    return(pRef->iNumReceivedPackets);
    if (iSelect == 'seqn')    return(pRef->iSequenceNum);
    if (iSelect == 'spkt')    return(pRef->iTotalPacketsToReceive);
    if (iSelect == 'srvr')    return(pRef->iServerIP);
    if (iSelect == 'stat')    return(pRef->iStatus);
    if (iSelect == 'stme')    return(pRef->iSendStartTime);
    if (iSelect == 'time')    return(pRef->iTimeout);

    // following options require a valid buffer pointer to succeed
    if ((pBuf != NULL) && (iBufSize > 0))
    {
        if ((iSelect == 'call') && (iBufSize > (signed)sizeof(pRef->pPortTesterUserCB)))
        {
            memcpy(pBuf, &pRef->pPortTesterUserCB, sizeof(pRef->pPortTesterUserCB));
            return(0);
        }
        if (iSelect == 'titl')
        {
            strnzcpy(pBuf, pRef->strTitle, iBufSize);
            return(0);
        }
        if (iSelect == 'user')
        {
            strnzcpy(pBuf, pRef->strUser, iBufSize);
            return(0);
        }
    }

    // otherwise return an error code
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function PortTesterControl

    \Description
        Set various PortTester parameters

    \Input *pRef        - reference pointer
    \Input iSelect      - parameter selector
    \Input iValue       - value to set the parameter to

    \Output int32_t         - selector-specific

    \Notes
        iKind can be one of the following:

        \verbatim
            'srvr' - server IP address
            'oprt' - outgoing port number
            'iprt' - incoming port number
            'time' - timeout value
        \endverbatim

    \Version 03/09/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t PortTesterControl(PortTesterT *pRef, int32_t iSelect, int32_t iValue)
{
    if (pRef == NULL)
        return(PORTTESTER_ERROR_BADPTR);

    // only set if we're IDLE
    if (pRef->iStatus != PORTTESTER_STATUS_IDLE)
        return(PORTTESTER_ERROR_BUSY);

    if (iSelect == 'srvr')
        pRef->iServerIP = iValue;
    if (iSelect == 'oprt')
        pRef->iOutgoingPortNum = iValue;
    if (iSelect == 'iprt')
        pRef->iIncomingPortNum = iValue;
    if (iSelect == 'time')
        pRef->iTimeout = iValue;

    return(PORTTESTER_ERROR_NONE);
}

/*F********************************************************************************/
/*!
    \Function PortTesterTestPort

    \Description
        Do a PortTester test on the specified input port.  Use the callback,
        if non-NULL, to report status.  Status may also be polled with
        PortTesterStatus().

    \Input *pRef        - PortTester module reference
    \Input iPort        - incoming port to test
    \Input iNumPackets  - number of incoming packets requested
    \Input *pCallback   - Optional (NULL if not desired) callback to report status
    \Input *pUserData   - user data for user callback, can be NULL
    
    \Output int32_t       - Status code (PORTTESTER_ERROR_NONE)

    \Version 03/09/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t PortTesterTestPort(PortTesterT *pRef, int32_t iPort, int32_t iNumPackets, PortTesterCallbackT *pCallback, void *pUserData)
{
    int32_t iResult;

    #if PORTTESTER_VERBOSE
    NetPrintf(("porttester: PortTesterRequest on port [%d] user callback [0x%X]\n", iPort, pCallback));
    #endif

    // set the port to test and the callback to use
    pRef->iIncomingPortNum = iPort;
    pRef->pPortTesterUserCB = pCallback;
    pRef->pUserData = pUserData;

    // wipe out any previous results and times to begin the new test
    pRef->iNumReceivedPackets = 0;
    pRef->iNumReceivedPacketsLast = 0;
    pRef->iStatusLast = pRef->iStatus;
    
    // set the initial number of packets to receive to the number of packets
    // we're requesting to be sent, but the server may clamp this number
    // later and cause the number to be less
    pRef->iNumSendPackets = iNumPackets;
    pRef->iTotalPacketsToReceive = iNumPackets;

    // advance the sequence number
    pRef->iSequenceNum++;

    // call the user callback to signify that we're starting (in IDLE right now)
    _PortTesterCallUserCallback(pRef);

    #if PORTTESTER_VERBOSE
    NetPrintf(("porttester: Binding ports - OUTGOING [%6d] INCOMING [%6d] SERVER [%08X]\n",
        pRef->iOutgoingPortNum, pRef->iIncomingPortNum, pRef->iServerIP));
    #endif

    // incoming socket
    iResult = ProtoUdpBind(pRef->pIncomingUDPRef, pRef->iIncomingPortNum);
    if (iResult == 0)
    {
        #if PORTTESTER_VERBOSE
        NetPrintf(("porttester: PortTesterConnect Bind succeeded for incoming port, port num [%d]\n", pRef->iIncomingPortNum));
        #endif
    }
    else
    {
        NetPrintf(("porttester: PortTesterConnect Bind failed for incoming port with result [%d]\n", iResult));
        return(PORTTESTER_ERROR_BIND);
    }

    // outgoing socket
    iResult = ProtoUdpBind(pRef->pOutgoingUDPRef, pRef->iOutgoingPortNum);
    if (iResult == 0)
    {
        #if PORTTESTER_VERBOSE
        NetPrintf(("porttester: PortTesterConnect Bind succeeded for outgoing port, port num [%d]\n", pRef->iOutgoingPortNum));
        #endif
    }
    else
    {
        // BIND may return an error because the port may already be in use
        // which might be the case if running both server and client on the same PC
        NetPrintf(("porttester: PortTesterConnect Bind failed for outgoing port with result [%d]\n", iResult));
        return(PORTTESTER_ERROR_BIND);
    }

    // mark the time and force the state transition
    pRef->iSendStartTime = NetTick();
    pRef->iStatus = PORTTESTER_STATUS_SENDING;

    // call the user callback to signify that we're about to send
    _PortTesterCallUserCallback(pRef);

    // done - no error for now
    return(PORTTESTER_ERROR_NONE);
}

/*F********************************************************************************/
/*!
    \Function PortTesterOpenPort

    \Description
        Send a packet to server over the given port, to test "opening" that port
        to incoming traffic.

    \Input *pRef       - PortTester module reference
    \Input iPort       - incoming port to test

    \Output int32_t    - Status code (PORTTESTER_ERROR_*)

    \Version 02/10/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t PortTesterOpenPort(PortTesterT *pRef, int32_t iPort)
{
    struct sockaddr ServerAddr;
    const char strPkt[] = "open";
    int32_t iResult;
    
    // use "incoming" socket
    iResult = ProtoUdpBind(pRef->pIncomingUDPRef, iPort);
    if (iResult == 0)
    {
        #if PORTTESTER_VERBOSE
        NetPrintf(("porttester: PortTesterOpenPort Bind succeeded for incoming port, port num [%d]\n", iPort));
        #endif
    }
    else
    {
        NetPrintf(("porttester: PortTesterOpenPort Bind failed for incoming port with result [%d]\n", iResult));
        return(PORTTESTER_ERROR_BIND);
    }

    // set up the server address to send to
    SockaddrInit(&ServerAddr, AF_INET);
    SockaddrInSetAddr(&ServerAddr, pRef->iServerIP);
    SockaddrInSetPort(&ServerAddr, iPort);

    #if PORTTESTER_VERBOSE
    NetPrintf(("\nporttester----> ServerAddr [0x%08X] ServerPort[0x%X]\n\n", SockaddrInGetAddr(&ServerAddr), SockaddrInGetPort(&ServerAddr));
    #endif

    // send the packet
    iResult = ProtoUdpSendTo(pRef->pOutgoingUDPRef, strPkt, sizeof(strPkt), &ServerAddr);
    if (iResult < 0)
    {
        NetPrintf(("porttester: PortTesterOpenPort got error from ProtoUdpSendTo [%d]\n", iResult));
    }
    else 
    {
        #if PORTTESTER_VERBOSE
        NetPrintf(("porttester: PortTesterOpenPort successfully sent [%d] bytes.\n", iResult));
        #endif
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function PortTesterDisconnect

    \Description
        Disconnect a port tester object from a lobby ref pointer.

    \Input *pRef            - PortTester module reference
    
    \Output None

    \Version 03/09/2005 (jfrank)
*/
/********************************************************************************F*/
void PortTesterDisconnect(PortTesterT *pRef)
{
    // check to make sure we haven't already been destroyed
    if (pRef == NULL)
    {
        NetPrintf(("porttester: PortTesterDisconnect called with NULL ref.  Not doing anything.\n"));
        return;
    }

    // now signify that we have no lobby connection
    pRef->uConnected = FALSE;

    // and force the state to IDLE
    pRef->iStatus = PORTTESTER_STATUS_IDLE;
}

/*F********************************************************************************/
/*!
    \Function PortTesterDestroy

    \Description
        Destroy a port tester object.

    \Input *pRef            - PortTester module reference
    
    \Output None

    \Version 03/09/2005 (jfrank)
*/
/********************************************************************************F*/
void PortTesterDestroy(PortTesterT *pRef)
{
    // check to make sure we haven't already been destroyed
    if (pRef == NULL)
    {
        NetPrintf(("porttester: PortTesterDestroy called with NULL ref.  Not doing anything.\n"));
        return;
    }

    // stop the idle function
    NetConnIdleDel(_PortTesterUpdate, pRef);

    // disconnect first, if we haven't already
    PortTesterDisconnect(pRef);

    // kill the protoudp refs
    ProtoUdpDestroy(pRef->pIncomingUDPRef);
    ProtoUdpDestroy(pRef->pOutgoingUDPRef);

    // now free the memory
    DirtyMemFree(pRef, PORTTESTER_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
}

