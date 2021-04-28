/*H********************************************************************************/
/*!
    \File netconnunix.c

    \Description
        Provides network setup and teardown support. Does not actually create any
        kind of network connections.

    \Copyright
        Copyright (c) 2010 Electronic Arts Inc.

    \Version 04/05/2010 (jbrookes) First version; a vanilla port to Unix from PS3
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtycert.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/protossl.h"
#include "DirtySDK/proto/protoupnp.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! private module state
typedef struct NetConnRefT
{
    // module memory group
    int32_t         iMemGroup;          //!< module mem group id
    void            *pMemGroupUserData; //!< user data associated with mem group

    enum
    {
        ST_INIT,                        //!< initialization
        ST_CONN,                        //!< bringing up network interface
        ST_IDLE,                        //!< active
    } eState;                           //!< internal connection state

    uint32_t        uConnStatus;        //!< connection status (surfaced to user)

    ProtoUpnpRefT   *pProtoUpnp;        //!< protoupnp module state
    int32_t         iPeerPort;          //!< peer port to be opened by upnp; if zero, still find upnp router but don't open a port
    int32_t         iNumProcCores;      //!< number of processor cores on the system
} NetConnRefT;

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

static NetConnRefT  *_NetConn_pRef = NULL;          //!< module state pointer

/*** Private Functions ************************************************************/


#if defined(DIRTYCODE_LINUX)
/*F********************************************************************************/
/*!
    \Function _NetConnGetProcLine

    \Description
        Parse a single line of the processor entry file.

    \Input *pLine   - pointer to line to parse
    \Input *pName   - [out] buffer to store parsed field name
    \Input iNameLen - size of name buffer
    \Input *pData   - [out] buffer to store parsed field data
    \Input iDataLen - size of data buffer

    \Version 04/26/2010 (jbrookes)
*/
/********************************************************************************F*/
static void _NetConnGetProcLine(const char *pLine, char *pName, int32_t iNameLen, char *pData, int32_t iDataLen)
{
    const char *pParse;

    // skip to end of field name
    for (pParse = pLine;  (*pParse != '\t') && (*pParse != ':'); pParse += 1)
        ;

    // copy field name
    ds_strsubzcpy(pName, iNameLen, pLine, pParse - pLine);

    // skip to field value
    for ( ; (*pParse == ' ') || (*pParse == '\t') || (*pParse == ':'); pParse += 1)
        ;

    // find end of field value
    for (pLine = pParse; *pParse != '\n'; pParse += 1)
        ;

    // copy field value
    ds_strsubzcpy(pData, iDataLen, pLine, pParse - pLine);
}
#endif

#if defined(DIRTYCODE_LINUX)
/*F********************************************************************************/
/*!
    \Function _NetConnGetProcRecord

    \Description
        Parse a single proc record.

    \Input *pProcFile   - proc record file pointer

    \Output
        int32_t         - one=success, zero=no more entries

    \Version 04/26/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetConnGetProcRecord(FILE *pProcFile)
{
    char strBuf[512], strName[32], strValue[128], *pLine;
    while (((pLine = fgets(strBuf, sizeof(strBuf), pProcFile)) != NULL) && (strBuf[0] != '\n'))
    {
        _NetConnGetProcLine(strBuf, strName, sizeof(strName), strValue, sizeof(strValue));
    }
    return(pLine != NULL);
}
#endif

/*F********************************************************************************/
/*!
    \Function _NetConnGetNumProcs

    \Description
        Get the number of processors on the system.

    \Output
        int32_t     - number of processors in system, or <=0 on failure

    \Version 04/26/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetConnGetNumProcs(void)
{
    #if defined(DIRTYCODE_LINUX)
    FILE *pFile = fopen("/proc/cpuinfo", "r");
    int32_t iNumProcs = -1;

    if (pFile != NULL)
    {
        for (iNumProcs = 0; _NetConnGetProcRecord(pFile) != 0; iNumProcs += 1)
            ;
        fclose(pFile);

        NetPrintf(("netconnunix: parsed %d processor cores from cpuinfo\n", iNumProcs));
    }
    else
    {
        NetPrintf(("netconnunix: could not open proc file\n"));
    }
    return(iNumProcs);
    #else
    return(-1);
    #endif
}

/*F********************************************************************************/
/*!
    \Function _NetConnGetInterfaceType

    \Description
        Get interface type and return it to caller.

    \Input *pRef    - module state

    \Output
        uint32_t    - interface type bitfield (NETCONN_IFTYPE_*)

    \Version 10/08/2009 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _NetConnGetInterfaceType(NetConnRefT *pRef)
{
    uint32_t uIfType = NETCONN_IFTYPE_ETHER;
    return(uIfType);
}

/*F********************************************************************************/
/*!
    \Function _NetConnGetPlatformEnvironment

    \Description
        Uses ticket issuer to determine platform environment.

    \Input *pRef    - module state

    \Output
        uint32_t    - platform environment (NETCONN_PLATENV_*)

    \Version 10/07/2009 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _NetConnGetPlatformEnvironment(NetConnRefT *pRef)
{
    uint32_t uPlatEnv = NETCONN_PLATENV_PROD;   // default to production environment
    //$$TODO - get platform environment?
    return(uPlatEnv);
}

/*F********************************************************************************/
/*!
    \Function _NetConnUpdate

    \Description
        Update status of NetConn module.  This function is called by NetConnIdle.

    \Input *pData   - pointer to NetConn module ref
    \Input uTick    - current tick counter

    \Version 07/18/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _NetConnUpdate(void *pData, uint32_t uTick)
{
    NetConnRefT *pRef = (NetConnRefT *)pData;

    // perform idle processing
    SocketControl(NULL, 'idle', uTick, NULL, NULL);

    // wait for network active status
    if (pRef->eState == ST_CONN)
    {
        pRef->uConnStatus = SocketInfo(NULL, 'conn', 0, NULL, 0);
        if (pRef->uConnStatus == '+onl')
        {
            // discover upnp router information
            if (pRef->pProtoUpnp != NULL)
            {
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

            pRef->eState = ST_IDLE;
        }
    }

    // update connection status while idle
    if (pRef->eState == ST_IDLE)
    {
        // update connection status if not already in an error state
        if ((pRef->uConnStatus >> 24) != '-')
        {
            pRef->uConnStatus = SocketInfo(NULL, 'conn', 0, NULL, 0);
        }
    }

    // if error status, go to idle state from any other state
    if ((pRef->eState != ST_IDLE) && (pRef->uConnStatus >> 24 == '-'))
    {
        pRef->eState = ST_IDLE;
    }
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function    NetConnStartup

    \Description
        Bring the network connection module to life. Creates connection with IOP
        resources and gets things ready to go. Puts all device drivers into "probe"
        mode so they look for appropriate hardware. Does not actually start any
        network activity.

    \Input *pParams - startup parameters

    \Output
        int32_t     - zero=success, negative=failure

    \Notes
        pParams can contain the following terms:

        \verbatim
            -noupnp             - disable upnp support
            -servicename        - set servicename <game-year-platform> required for SSL use
            -singlethreaded     - start DirtySock in single-threaded mode (typically when used in servers)
        \endverbatim

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnStartup(const char *pParams)
{
    NetConnRefT *pRef = _NetConn_pRef;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    int32_t iThreadPrio = 10;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allow NULL params
    if (pParams == NULL)
    {
        pParams = "";
    }

    // error if already started
    if (pRef != NULL)
    {
        NetPrintf(("netconnunix: NetConnStartup() called while module is already active\n"));
        return(-1);
    }

    // alloc and init ref
    if ((pRef = DirtyMemAlloc(sizeof(*pRef), NETCONN_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("netconnunix: unable to allocate module state\n"));
        return(-2);
    }
    memset(pRef, 0, sizeof(*pRef));
    pRef->iMemGroup = iMemGroup;
    pRef->pMemGroupUserData = pMemGroupUserData;
    pRef->eState = ST_INIT;

    // check for singlethreaded mode
    if (strstr(pParams, "-singlethreaded"))
    {
        iThreadPrio = -1;
    }

    // create network instance
    SocketCreate(iThreadPrio, 0, 0);

    // create and configure dirtycert
    if (NetConnDirtyCertCreate(pParams))
    {
        NetConnShutdown(0);
        return(-3);
    }

    // start up protossl
    if (ProtoSSLStartup() < 0)
    {
        NetConnShutdown(0);
        NetPrintf(("netconnunix: unable to startup protossl\n"));
        return(-4);
    }

    // create the upnp module
    if (!strstr(pParams, "-noupnp"))
    {
        pRef->pProtoUpnp = ProtoUpnpCreate();
    }

    // add netconn task handle
    NetConnIdleAdd(_NetConnUpdate, pRef);

    // save ref
    _NetConn_pRef = pRef;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function    NetConnQuery

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
    \Function    NetConnConnect

    \Description
        Used to bring the networking online with a specific configuration. Uses a
        configuration returned by NetConnQuery.

    \Input *pConfig - unused
    \Input *pOption - optionally provide silent=true/false to prevent/enable sysutils from displaying.
    \Input iData    - platform-specific

    \Output
        int32_t     - negative=error, zero=success

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnConnect(const NetConfigRecT *pConfig, const char *pOption, int32_t iData)
{
    int32_t iResult = 0;
    NetConnRefT *pRef = _NetConn_pRef;

    // check connection options, if present
    if (pRef->eState == ST_INIT)
    {
        // check for connect options
        if (pOption != NULL)
        {
            const char *pOpt;

            // check for specification of peer port
            if ((pOpt = strstr(pOption, "peerport=")) != NULL)
            {
                pRef->iPeerPort = strtol(pOpt+9, NULL, 10);
                NetPrintf(("netconnunix: peerport=%d\n", pRef->iPeerPort));
            }
        }

        // start up network interface
        SocketControl(NULL, 'conn', 0, NULL, NULL);

        // transition to connecting state
        pRef->eState = ST_CONN;
    }
    else
    {
        NetPrintf(("netconnunix: NetConnConnect() ignored because already connected!\n"));
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function    NetConnDisconnect

    \Description
        Used to bring down the network connection. After calling this, it would
        be necessary to call NetConnConnect to bring the connection back up or
        NetConnShutdown to completely shutdown all network support.

    \Output
        int32_t     - negative=error, zero=success

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnDisconnect(void)
{
    NetConnRefT *pRef = _NetConn_pRef;

    // shut down networking
    if (pRef->eState != ST_INIT)
    {
        // bring down network interface
        SocketControl(NULL, 'disc', 0, NULL, NULL);

        // reset status
        pRef->eState = ST_INIT;
        pRef->uConnStatus = 0;
    }
    // done
    return(0);
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
            snam: set DirtyCert service name
        \endverbatim

        Unhandled selectors are passed through to SocketControl()

    \Version 1.0 04/27/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnControl(int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue, void *pValue2)
{
    NetConnRefT *pRef = _NetConn_pRef;

    // make sure module is started before allowing any other control calls
    if (pRef == NULL)
    {
        NetPrintf(("netconnunix: warning - calling NetConnControl() while module is not initialized\n"));
        return(-1);
    }
    // set dirtycert service name
    if (iControl == 'snam')
    {
        return(DirtyCertControl('snam', 0, 0, pValue));
    }
    // pass through unhandled selectors to SocketControl()
    return(SocketControl(NULL, iControl, iValue, pValue, pValue2));
}

/*F********************************************************************************/
/*!
    \Function    NetConnStatus

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
            bbnd: TRUE if broadband, else FALSE
            conn: connection status: +onl=online, ~<code>=in progress, -<err>=NETCONN_ERROR_*
            envi: EA back-end environment type in use (-1=not available, 0=pending)
            macx: MAC address of client (returned in pBuf)
            onln: true/false indication of whether network is operational
            open: true/false indication of whether network code running
            plug: network plug status (TRUE)
            proc: number of processor cores on the system (Linux only)
            type: connection type: NETCONN_IFTYPE_* bitfield
            upnp: return protoupnp external port info, if available
            vers: return DirtySDK version
        \endverbatim

    \Version 10/04/2005 (jbrookes)
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
        return(DIRTYSDK_VERSION);
    }

    // make sure module is started before allowing any other status calls
    if (pRef == NULL)
    {
        NetPrintf(("netconnunix: warning - calling NetConnStatus() while module is not initialized\n"));
        return(-1);
    }

    // return broadband (TRUE/FALSE)
    if (iKind == 'bbnd')
    {
        return(TRUE);
    }
    // connection status
    if (iKind == 'conn')
    {
        return(pRef->uConnStatus);
    }
    // EA back-end environment type in use
    if (iKind == 'envi')
    {
        return(_NetConnGetPlatformEnvironment(pRef));
    }
    // see if connected to ISP/LAN
    if (iKind == 'onln')
    {
        return(pRef->uConnStatus == '+onl');
    }
    // return status of plug (UNIX is assumed to be an "always-connected" platform)
    if (iKind == 'plug')
    {
        return(TRUE);
    }
    // return number of processor cores
    if (iKind == 'proc')
    {
        if (pRef->iNumProcCores == 0)
        {
            pRef->iNumProcCores = _NetConnGetNumProcs();
        }
        return(pRef->iNumProcCores);
    }
    // return interface type (more verbose)
    if (iKind == 'type')
    {
        return(_NetConnGetInterfaceType(pRef));
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
    \Function    NetConnShutdown

    \Description
        Shutdown the network code and return to idle state.

    \Input  uShutdownFlags  - shutdown configuration flags

    \Output
        int32_t             - negative=error, else zero

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnShutdown(uint32_t uShutdownFlags)
{
    // make sure we've been started
    if (_NetConn_pRef == NULL)
    {
        return(-1);
    }

    // destroy the upnp ref
    if (_NetConn_pRef->pProtoUpnp != NULL)
    {
        ProtoUpnpDestroy(_NetConn_pRef->pProtoUpnp);
        _NetConn_pRef->pProtoUpnp = NULL;
    }

    // shut down protossl
    ProtoSSLShutdown();

    // destroy the dirtycert module
    DirtyCertDestroy();

    // remove netconn idle task
    NetConnIdleDel(_NetConnUpdate, _NetConn_pRef);

    // shut down Idle handler
    NetConnIdleShutdown();

    // disconnect the interfaces
    NetConnDisconnect();

    // shutdown the network code
    SocketDestroy(0);

    // dispose of ref
    DirtyMemFree(_NetConn_pRef, NETCONN_MEMID, _NetConn_pRef->iMemGroup, _NetConn_pRef->pMemGroupUserData);
    _NetConn_pRef = NULL;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function    NetConnSleep

    \Description
        Sleep the application for some number of milliseconds.

    \Input iMilliSecs    - number of milliseconds to block for

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
void NetConnSleep(int32_t iMilliSecs)
{
    usleep(iMilliSecs*1000);
}

