/*H********************************************************************************/
/*!
    \File netconnrev.c

    \Description
        Provides network setup and teardown support. Does not actually create any
        kind of network connections.

    \Copyright
        Copyright (c) 2006 Electronic Arts Inc.

    \Version 11/27/2006 (jbrookes) First Version
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#include <string.h>
#include <stdlib.h>
#include <revolution/os.h>

#include "dirtysock.h"
#include "dirtyvers.h"
#include "dirtyerr.h"
#include "dirtymem.h"
#include "netconn.h"
#include "protoupnp.h"

/*** Defines **********************************************************************/

//! rate at which socket idling takes place, in hz
#define NETCONN_IDLERATE        (2)

//! netconn idle interval
#define NETCONN_IDLEINTERVAL    ((signed)(1000/NETCONN_IDLERATE))

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! private module state
typedef struct NetConnRefT
{
    //! module memory group
    int32_t         iMemGroup;
    void            *pMemGroupUserData;

    uint32_t        uGameCode;
    
    enum
    {
        ST_INIT,
        ST_CONN,
        ST_IDLE,
    } eState;
    
    char            strAuthCode[8];

    int32_t         iPeerPort;          //!< peer port to be opened by upnp; if zero, still find upnp router but don't open a port
    uint32_t        uLastIdleTick;      //!< most recent idle update tick

    ProtoUpnpRefT   *pProtoUpnp;
} NetConnRefT;

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

static NetConnRefT  *_NetConn_pRef = NULL;

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _NetConnCopyParam

    \Description
        Copy a command-line parameter.
        
    \Input *pDst        - output buffer
    \Input iDstLen      - output buffer length
    \Input *pParamName  - name of parameter to check for
    \Input *pSrc        - input string to look for parameters in
    \Input *pDefault    - default string to use if paramname not found
    
    \Output
        None.

    \Version 1.0 07/18/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetConnCopyParam(char *pDst, int32_t iDstLen, const char *pParamName, const char *pSrc, const char *pDefault)
{
    int32_t iIndex;

    // find parameter
    if ((pSrc = strstr(pSrc, pParamName)) == NULL)
    {
        // copy in default
        strnzcpy(pDst, pDefault, iDstLen);
        return(0);
    }
    
    // skip parameter name
    pSrc += strlen(pParamName);
    
    // make sure buffer has enough room
    if (--iDstLen < 0)
    {
        return(0);
    }
    
    // copy the string
    for (iIndex = 0; (iIndex < iDstLen) && (pSrc[iIndex] != '\0') && (pSrc[iIndex] != ' '); iIndex++)
    {
        pDst[iIndex] = pSrc[iIndex];
    }
    
    // write null terminator and return number of bytes written
    pDst[iIndex] = '\0';
    return(iIndex);
}

/*F********************************************************************************/
/*!
    \Function    _NetConnParseParams
    
    \Description
        Parse and handle input parameters to NetConnStartup().
    
    \Input *pRef    - module state
    \Input *pParams - startup parameters
    
    \Output
        None.
        
    \Version 02/23/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _NetConnParseParams(NetConnRefT *pRef, const char *pParams)
{
    char strTemp[32];
    
    // get game code
    _NetConnCopyParam(strTemp, sizeof(strTemp), "-gamecode=", pParams, "RABA");
    pRef->uGameCode = (strTemp[0] << 24) | (strTemp[1] << 16) | (strTemp[2] << 8) | strTemp[3];
    NetPrintf(("netconnrev: setting game code=%c%c%c%c\n", 
        (pRef->uGameCode >> 24) & 0xff, (pRef->uGameCode >> 16) & 0xff,
        (pRef->uGameCode >> 8) & 0xff, (pRef->uGameCode >> 0) & 0xff));

    // get authentication code
    _NetConnCopyParam(pRef->strAuthCode, sizeof(pRef->strAuthCode), "-authcode=", pParams, "9000");
    NetPrintf(("netconnrev: setting auth code=%s\n", pRef->strAuthCode));
}

/*F********************************************************************************/
/*!
    \Function _NetConnUpdate

    \Description
        Update status of NetConn module.  This function is called by NetConnIdle.
        
    \Input *pData   - pointer to NetConn module ref
    \Input uTick    - current tick count
    
    \Output None.

    \Version 1.0 09/15/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _NetConnUpdate(void *pData, uint32_t uTick)
{
    NetConnRefT *pRef = _NetConn_pRef;
    
    // perform socket-level idle processing
    if (NetTickDiff(uTick, pRef->uLastIdleTick) >= NETCONN_IDLEINTERVAL)
    {
        SocketControl(NULL, 'idle', uTick, NULL, NULL);
        pRef->uLastIdleTick = uTick;
    }

    // if connecting, check for connection success
    if (pRef->eState == ST_CONN)
    {
        if (SocketInfo(NULL, 'conn', 0, NULL, 0) == '+onl')
        {
            if (pRef->pProtoUpnp != NULL)
            {
                // discover upnp router information
                if (pRef->iPeerPort != 0)
                {
                    ProtoUpnpControl(pRef->pProtoUpnp, 'port', pRef->iPeerPort, 0, NULL);
                    ProtoUpnpControl(pRef->pProtoUpnp, 'macr', 'upnp', 0, NULL);
                }
                else
                {
                    ProtoUpnpControl(pRef->pProtoUpnp, 'macr', 'dscg', 0, NULL);
                }
            }
            // transition to idle state
            pRef->eState = ST_IDLE;
        }
    }
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function NetConnStartup
    
    \Description
        Creates the basic DirtySock networking module.  Must be called before other
        DirtySock calls are made.
    
    \Input *pParams    - startup parameters
    
    \Output
        int32_t        - zero=success, negative=failure

    \Notes
        pParams can contain the following terms:

        \verbatim
            -gamecode=ABCD      - specify title's game code (default=RABA)
            -authcode=WXYZ      - specify title's auth code (default=9000)
        \endverbatim

        Note that for testing purposes gamecode and authcode can be omitted, but
        every shipping title must specify a real game and auth code for their title.
            
    \Version 09/15/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnStartup(const char *pParams)
{
    NetConnRefT *pRef = _NetConn_pRef;
    OSThread *pCurrThread;
    int32_t iMode, iPriority, iResult;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allow NULL params
    if (pParams == NULL)
    {
        pParams = "";
    }
    
    // error see if already started
    if (pRef != NULL)
    {
        NetPrintf(("netconnrev: NetConnStartup() called while module is already active\n"));
        return(-1);
    }

    // calculate netlib and socket thread priorities as current - 1
    pCurrThread = OSGetCurrentThread();
    iPriority = OSGetThreadPriority(pCurrThread) - 1;
    NetPrintf(("netconnrev: starting dirtysock with threadprio=%d\n", iPriority));

    // select inet or adhoc
    iMode = !strstr(pParams, "-inet") ? 'ahoc' : 'inet';
    SocketControl(NULL, iMode, 0, NULL, 0);

    // create network instance
    SocketCreate(iPriority);

    // alloc and init ref
    if ((pRef = DirtyMemAlloc(sizeof(*pRef), NETCONN_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("netconnrev: unable to allocate module state\n"));
        return(-2);
    }
    memset(pRef, 0, sizeof(*pRef));
    pRef->iMemGroup = iMemGroup;
    pRef->pMemGroupUserData = pMemGroupUserData;
    pRef->eState = ST_INIT;
    pRef->uLastIdleTick = NetTick();

    // parse params
    _NetConnParseParams(pRef, pParams);
    
    // init DWC library
    if ((iResult = SocketControl(NULL, 'init', pRef->uGameCode, pRef->strAuthCode, NULL)) != 0)
    {
        SocketDestroy(0);
        return(iResult);
    }

    // add netconn task handle
    NetConnIdleAdd(_NetConnUpdate, pRef);

    // create the upnp module
    pRef->pProtoUpnp = ProtoUpnpCreate();
    
    // save ref
    _NetConn_pRef = pRef;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function NetConnQuery
    
    \Description
        Query the list of available connection configurations. This list is loaded
        from the specified device. The list is returned in a simple fixed width
        array with one string per array element. The caller can find the user portion
        of the config name via strchr(item, '#')+1.
    
    \Input *pDevice - device to scan (mc0:, mc1:, pfs0:, pfs1:)
    \Input *pList   - buffer to store output array in
    \Input iSize    - length of buffer in bytes
    
    \Output
        int32_t     - negative=error, else number of configurations
            
    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnQuery(const char *pDevice, NetConfigRecT *pList, int32_t iSize)
{
    return(0);
}

/*F********************************************************************************/
/*!
    \Function NetConnConnect

    \Description
        Used to bring the networking online with a specific configuration. Uses a
        configuration returned by NetConnQuery.
    
    \Input *pConfig - the configuration entry from NetConnQuery
    \Input *pOption - asciiz list of config parameters (tbd)
    \Input iData    - platform-specific

    \Output
        int32_t     - negative=error, zero=success

    \Version 09/15/2006 (jbrookes)  Initial version
    \Version 06/06/2009 (mclouatre) Early returns success if not in ST_INIT state
*/
/********************************************************************************F*/
int32_t NetConnConnect(const NetConfigRecT *pConfig, const char *pOption, int32_t iData)
{
    NetConnRefT *pRef = _NetConn_pRef;
    int32_t iResult = 0;

    // check connection options, if present
    if (pOption != NULL)
    {
        const char *pOpt;
        // check for specification of peer port
        if ((pOpt = strstr(pOption, "peerport=")) != NULL)
        {
            pRef->iPeerPort = strtol(pOpt+9, NULL, 10);
            NetPrintf(("netconnrev: peerport=%d\n", pRef->iPeerPort));
        }
    }

    // initiate the connect
    if (pRef->eState == ST_INIT)
    {
        if ((iResult = SocketControl(NULL, 'conn', 0, (void *)pConfig, (void *)pOption)) == 0)
        {    
            // connecting
            pRef->eState = ST_CONN;
        }
    }
    else
    {
        NetPrintf(("netconnrev: NetConnConnect() ignored because already connected!\n"));
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function NetConnControl

    \Description
        Set module behavior based on input selector.

    \Input  iControl    - input selector
    \Input  iValue      - selector input
    \Input  iValue2     - selector input
    \Input *pValue      - selector input
    \Input *pValue2     - selector input
    
    \Output
        int32_t         - selector result

    \Notes
        iControl can be one of the following:

        \verbatim
        \endverbatim
        
        Unhandled selectors are passed through to SocketControl()
    
    \Version 1.0 04/27/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnControl(int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue, void *pValue2)
{
    // make sure module is started before allowing any other control calls
    if (_NetConn_pRef == NULL)
    {
        NetPrintf(("netconnrev: warning - calling NetConnControl() while module is not initialized\n"));
        return(-1);
    }
    // pass through unhandled selectors to SocketControl()
    return(SocketControl(NULL, iControl, iValue, pValue, pValue2));
}

/*F********************************************************************************/
/*!
    \Function NetConnStatus
    
    \Description
        Check general network connection status. Different selectors return
        different status attributes.
    
    \Input iKind    - status selector ('open', 'conn', 'onln')
    \Input iData    - (optional) selector specific
    \Input *pBuf    - (optional) pointer to output buffer
    \Input iBufSize - (optional) size of output buffer
    
    \Output
        int32_t     - selector specific
            
    \Notes
        iKind can be one of the following:
        
        \verbatim
            addr: ip address of client
            athc: authentication code
            bbnd: TRUE if broadband, else FALSE
            conn: connection status: +onl=online, ~<code>=in progress, -<err>=NETCONN_ERROR_*
            envi: EA back-end environment type in use
            gamc: game code
            macx: mac address of client (returned in pBuf)
            onln: true/false indication of whether network is operational
            open: true/false indication of whether network code running
            plug: SCE/Ethernet plug status (negative=error/unknown, 0=disconnected, 1=connected)
            type: connection type: NETCONN_IFTYPE_* bitfield
            upnp: return protoupnp port map info, if available
            vers: return DirtySDK version
        \endverbatim
            
    \Version 09/15/2006 (jbrookes)  initial version
    \Version 06/11/2006 (mclouatre) added support for 'envi' selector
*/
/********************************************************************************F*/
int32_t NetConnStatus(int32_t iKind, int32_t iData, void *pBuf, int32_t iBufSize)
{
    NetConnRefT *pRef = _NetConn_pRef;
    
    // initialize output buffer
    if (pBuf != NULL)
    {
        memset(pBuf, 0, iBufSize);
    }

    // see if network code is initialized
    if (iKind == 'open')
    {
        return(pRef != NULL);
    }
    // return DirtySDK version
    if (iKind == 'vers')
    {
        return(DIRTYVERS);
    }

    // make sure module is started before allowing any other status calls    
    if (pRef == NULL)
    {
        NetPrintf(("netconnrev: warning - calling NetConnStatus() while module is not initialized\n"));
        return(-1);
    }

    // return authcode
    if ((iKind == 'athc') && (pBuf != NULL))
    {
        strnzcpy(pBuf, pRef->strAuthCode, iBufSize);
        return(0);
    }
    // return broadband (TRUE/FALSE)
    if (iKind == 'bbnd')
    {
        return(TRUE);
    }
    // return EA backend environment (dev or prod)
    if (iKind == 'envi')
    {
        // Note: WII currently has no authentication server type matching NETCONN_PLATENV_CERT. There is 
        // one called DWC_AUTHSERVER_TEST in the DWCAUTHSERVER, but it is flagged as "Do not use" for now.

        int32_t rc = SocketInfo(NULL, 'aths', 0, NULL, 0);

        if (rc == 0)
        {
             return(NETCONN_PLATENV_TEST);
        }
        else if (rc == 1)
        {
             return(NETCONN_PLATENV_PROD);
        }
        else
        {
            NetPrintf(("netconnrev: - WARNING - processing of 'envi' selector in NetConnStatus() failed!\n"));
            return(-1);
        }
    }
    // return gamecode
    if ((iKind == 'gamc') && (pBuf != NULL) && (iBufSize == 4))
    {
        memcpy(pBuf, &pRef->uGameCode, iBufSize);
        return(0);
    }
    // see if connected to ISP/LAN
    if (iKind == 'onln')
    {
        return(NetConnStatus('conn', 0, NULL, 0) == '+onl');
    }
    // return interface type (more verbose)
    if (iKind == 'type')
    {
        uint32_t uIfType = NETCONN_IFTYPE_ETHER;
        return(uIfType);
    }
    // return upnp addportmap info, if available
    if (iKind == 'upnp')
    {
        // if protoupnp is available, and we've added a port map, return the external port for the port mapping
        if ((pRef->pProtoUpnp != NULL) && (ProtoUpnpStatus(pRef->pProtoUpnp, 'stat', NULL, 0) & PROTOUPNP_STATUS_ADDPORTMAP))
        {
            return(ProtoUpnpStatus(pRef->pProtoUpnp, 'extp', NULL, 0));
        }
    }
    // pass unrecognized options to SocketInfo
    return(SocketInfo(NULL, iKind, 0, pBuf, iBufSize));
}

/*F********************************************************************************/
/*!
    \Function NetConnDisconnect

    \Description
        Used to bring down the network connection. After calling this, it would
        be necessary to call NetConnConnect to bring the connection back up or
        NetConnShutdown to completely shutdown all network support.

    \Output
        int32_t     - negative=error, zero=success

    \Version 09/15/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnDisconnect(void)
{
    int32_t iResult = 0;
    if (_NetConn_pRef->eState != ST_INIT)
    {
        iResult = SocketControl(NULL, 'disc', 0, NULL, NULL);
        _NetConn_pRef->eState = ST_INIT;
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function NetConnShutdown
    
    \Description
        Shutdown the network code and return to idle state.
    
    \Input uShutdownFlags   - shutdown configuration flags
    
    \Output
        int32_t             - negative=error, zero=success

    \Version 09/15/2006 (jbrookes)
    \Version 08/11/2009 (mclouatre) ProtoUpnpDestroy() now called before NetConnDisconnect()
*/
/********************************************************************************F*/
int32_t NetConnShutdown(uint32_t uShutdownFlags)
{
    NetConnRefT *pRef = _NetConn_pRef;
    int32_t iResult;
    
    // make sure we've been started
    if (pRef == NULL)
    {
        return(-1);
    }

    // destroy the upnp ref
    if (_NetConn_pRef->pProtoUpnp != NULL)
    {
        ProtoUpnpDestroy(_NetConn_pRef->pProtoUpnp);
        _NetConn_pRef->pProtoUpnp = NULL;
    }

    // disconnect the interfaces
    if ((iResult = NetConnDisconnect()) != 0)
    {
        return(iResult);
    }

    // shut down socket lib
    if ((iResult = SocketControl(NULL, 'shut', 0, NULL, NULL)) != 0)
    {
        return(iResult);
    }

    // remove netconn idle task
    NetConnIdleDel(_NetConnUpdate, pRef);
    
    // shut down Idle handler
    NetConnIdleShutdown();

    // shutdown the network code
    SocketDestroy(0);

    // dispose of ref
    DirtyMemFree(pRef, NETCONN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
    _NetConn_pRef = NULL;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function NetConnSleep
    
    \Description
        Sleep the application for some number of milliseconds.
    
    \Input iMilliSecs   - number of milliseconds to sleep
    
    \Output
        None.
            
    \Version 09/15/2006 (jbrookes)
*/
/********************************************************************************F*/
void NetConnSleep(int32_t iMilliSecs)
{
    OSSleepMilliseconds(iMilliSecs);
}

