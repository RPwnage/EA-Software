/*H********************************************************************************/
/*!
    \File tracert.c

    \Description
        An implementation of 'tracert' using protoping

    \Copyright
        Copyright (c) 2002-2005 Electronic Arts Inc.

    \Version 10/31/2002 (jbrookes) First Version
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#if defined(_WIN32) && !defined(EA_PLATFORM_CAPILANO)
#pragma warning(push,0)
#include <windows.h>
#pragma warning(pop)

#include <winsock.h>
#endif

#include <string.h>
#include <stdlib.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/proto/protoping.h"
#include "DirtySDK/proto/protoname.h"

#include "zlib.h"
#include "testermodules.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct TraceCmdRefT             // current module state
{
    ProtoPingRefT   *pProtoPing;        // ping ref
    HostentT        *pHost;             // dns lookup of hostname
    uint32_t        uTick;              // number of ticks until death
    int32_t         iMaxHops;           // max number of hops to trace through
    int32_t         iNumHops;           // number of hops navigated
    int32_t         iTimeout;           // timeout in ms
    int32_t         iTtl;               // time to live
    char            strHostname[256];   // hostname to tracert to
    uint32_t        uTargetAddress;     // address to tracert to
    int32_t         iPingsPerHop;       // pings per ttl
    int32_t         iNumPings;          // number of pings left for this ttl
    uint32_t        bResolve;           // whether to resolve hostname or not
    uint32_t        bUdpPing;           // if TRUE, use udp ping instead of icmp
    uint32_t        uPort;              // port to use for non-icmp pinging
    uint32_t        bVerbose;           // verbose or not?
    uint32_t        iHopPings;          // how many pings on this hop?
    uint32_t        uHopAddress;        // address of current hop
} TraceCmdRefT;

/*** Function Prototypes ***************************************************************/


/*** Variables *************************************************************************/


// Private variables

// Public variables


/*** Private Functions *****************************************************************/


/*F********************************************************************************/
/*!
    \Function _CmdTracertUsage

    \Description
        Update demangling process.

    \Input *pProgramName    - pointer to name of program

    \Output
        None,

    \Version 10/21/2002 (jbrookes)
*/
/********************************************************************************F*/
static void _CmdTracertUsage(char *pProgramName)
{
    ZPrintf("   usage: %s [-d] [-m<count>] [-n<count>] [-u<port>] [-v] [-w<timeout>] hostname\n", pProgramName);
    ZPrintf("      -d                do not resolve hostname\n");
    ZPrintf("      -m<count>         maximum number of hops to search\n");
    ZPrintf("      -n<count>         number of echo requests to send\n");
    ZPrintf("      -u<port>          use udp instead of icmp\n");
    ZPrintf("      -v                enable verbose mode\n");
    ZPrintf("      -w<timeout>       timeout, in ms\n");
}

/*F********************************************************************************/
/*!
    \Function _CmdTracertGetArgs

    \Description
        Parse command-line arguments

    \Input *pCmdRef - pointer to module state
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Output
        int32_t     - index past parsed arguments

    \Version 10/21/2002 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdTracertGetArgs(TraceCmdRefT *pCmdRef, int32_t argc, char *argv[])
{
    int32_t iArg;
    char *pOpt;

    for (iArg = 1; iArg < argc; iArg++)
    {
        if (argv[iArg][0] == '-')
        {
            if ((pOpt = strchr(argv[iArg], 'd')) != NULL)
            {
                pCmdRef->bResolve = FALSE;
            }
            if ((pOpt = strchr(argv[iArg], 'm')) != NULL)
            {
                pCmdRef->iMaxHops = (int32_t)strtol(pOpt + 1, NULL, 10);
            }
            if ((pOpt = strchr(argv[iArg], 'n')) != NULL)
            {
                pCmdRef->iPingsPerHop = (int32_t)strtol(pOpt + 1, NULL, 10);
            }
            if ((pOpt = strchr(argv[iArg], 'u')) != NULL)
            {
                pCmdRef->uPort = (int32_t)strtol(pOpt + 1, NULL, 10);
                pCmdRef->bUdpPing = TRUE;
            }
            if ((pOpt = strchr(argv[iArg], 'v')) != NULL)
            {
                pCmdRef->bVerbose = TRUE;
            }
            if ((pOpt = strchr(argv[iArg], 'w')) != NULL)
            {
                pCmdRef->iTimeout = (int32_t)strtol(pOpt + 1, NULL, 10);
            }
        }
        else
        {
            break;
        }
    }

    return(iArg);
}

/*F********************************************************************************/
/*!
    \Function _CmdTracertDestroy

    \Description
        Destroy module.

    \Input *pCmdRef - pointer to module state

    \Output
        None.

    \Version 10/21/2002 (jbrookes)
*/
/********************************************************************************F*/
static void _CmdTracertDestroy(TraceCmdRefT *pCmdRef)
{
    if (pCmdRef->pHost)
    {
        // release hostname resolve socket
        pCmdRef->pHost->Free(pCmdRef->pHost);
        pCmdRef->pHost = NULL;
    }

    if (pCmdRef->pProtoPing)
    {
        ProtoPingDestroy(pCmdRef->pProtoPing);
        pCmdRef->pProtoPing = NULL;
    }
}

/*F********************************************************************************/
/*!
    \Function _CmdTracertPrintHop

    \Description
        Print hop address (and name on some platforms).

    \Input *pCmdRef - pointer to module state

    \Output
        None.

    \Version 12/15/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _CmdTracertPrintHop(TraceCmdRefT *pCmdRef)
{
    char strHostname[256] = "";

    if (pCmdRef->bResolve)
    {
        #if defined(DIRTYCODE_PC)
        uint32_t uAddr = SocketHtonl(pCmdRef->uHopAddress);
        struct hostent *pHostent = NULL;
        if ((pHostent = gethostbyaddr((char *)&uAddr, sizeof(uAddr), AF_INET)) != NULL)
        {
            ds_strnzcpy(strHostname, pHostent->h_name, sizeof(strHostname));
        }
        #endif
    }

    if (strHostname[0] != '\0')
    {
        ZPrintf(" %-15a [%s]\n", pCmdRef->uHopAddress, strHostname);
    }
    else
    {
        ZPrintf(" %-15a\n", pCmdRef->uHopAddress);
    }
}

/*F********************************************************************************/
/*!
    \Function _CmdTracertCB

    \Description
        Send trace requests and wait for responses.

    \Input *argz    - module context
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Output
        None.

    \Version 10/21/2002 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdTracertCB(ZContext *argz, int32_t argc, char *argv[])
{
    DirtyAddrT DirtyAddr;

    TraceCmdRefT *pCmdRef = (TraceCmdRefT *)argz;

    // check for kill
    if (argc == 0)
    {
        _CmdTracertDestroy(pCmdRef);
        ZPrintf("%s: killed\n", argv[0]);
        return(0);
    }

    // handle startup
    if (pCmdRef->pProtoPing == NULL)
    {
        // check for host addr resolve
        int32_t iHost = pCmdRef->pHost->Done(pCmdRef->pHost);
        if (iHost == -1)
        {
            _CmdTracertDestroy(pCmdRef);
            ZPrintf("%s: unknown host\n", argv[0]);
            return(0);
        }
        else if (iHost != 0)
        {
            // get address
            pCmdRef->uTargetAddress = pCmdRef->pHost->addr;

            // release hostname resolve socket
            pCmdRef->pHost->Free(pCmdRef->pHost);
            pCmdRef->pHost = NULL;

            // allocate and configure pingref
            if ((pCmdRef->pProtoPing = ProtoPingCreate(0)) != NULL)
            {
                ProtoPingControl(pCmdRef->pProtoPing, 'spam', pCmdRef->bVerbose, NULL);
                if (pCmdRef->bUdpPing)
                {
                    ProtoPingControl(pCmdRef->pProtoPing, 'prot', 0x11, NULL);
                }

                // echo resolved hostname in dot notation
                ZPrintf("Tracing route to %s [%a] (%d hops max)\n", pCmdRef->strHostname,
                    pCmdRef->uTargetAddress, pCmdRef->iMaxHops);

                // since we don't have a DirtyAddr, create one from the physical address
                DirtyAddrFromHostAddr(&DirtyAddr, &pCmdRef->uTargetAddress);

                // issue ping request
                if (ProtoPingRequest(pCmdRef->pProtoPing, &DirtyAddr, NULL, 0, pCmdRef->iTimeout, pCmdRef->iTtl) < 0)
                {
                    _CmdTracertDestroy(pCmdRef);
                    ZPrintf("%s: error issuing ping request\n", argv[0]);
                    return(0);
                }
            }
            else
            {
                _CmdTracertDestroy(pCmdRef);
                ZPrintf("tracert: could not create protoping module\n");
                return(0);
            }
        }
    }
    else
    {
        ProtoPingResponseT PingResp;
        uint32_t uAddr;
        int32_t iPing;

        if ((iPing = ProtoPingResponse(pCmdRef->pProtoPing, NULL, 0, &PingResp)) != 0)
        {
            // get addr from response structure
            uAddr = PingResp.uAddr;

            // print start of output
            if (pCmdRef->iNumPings == pCmdRef->iPingsPerHop)
            {
                ZPrintf("%2d ", pCmdRef->iNumHops);
            }

            pCmdRef->iNumPings--;
            if (iPing > 0)
            {
                pCmdRef->uHopAddress = uAddr;
                if (iPing > 999)
                {
                    ZPrintf("%d.%ds ", iPing/1000, (iPing%1000)/10);
                }
                else
                {
                    ZPrintf("%3dms ", iPing);
                }
                pCmdRef->iHopPings += 1;
            }
            else
            {
                ZPrintf("  *   ");
            }

            // done pinging this hop?
            if (pCmdRef->iNumPings == 0)
            {
                if (pCmdRef->iHopPings > 0)
                {
                    _CmdTracertPrintHop(pCmdRef);
                }
                else
                {
                    ZPrintf("Request timed out.\n");
                }

                if (pCmdRef->iNumHops >= pCmdRef->iMaxHops)
                {
                    // done - finished before reaching target
                    ZPrintf("\nTrace completed without reaching target.\n");

                    // shutdown module
                    _CmdTracertDestroy(pCmdRef);
                    return(0);
                }
                if ((pCmdRef->uHopAddress == pCmdRef->uTargetAddress) && (pCmdRef->iHopPings > 0))
                {
                    ZPrintf("\nTrace complete in %d hops\n", pCmdRef->iNumHops);

                    // shutdown module after ping received
                    _CmdTracertDestroy(pCmdRef);
                    return(0);
                }

                pCmdRef->iNumPings = pCmdRef->iPingsPerHop;
                pCmdRef->iNumHops += 1;
                pCmdRef->iTtl += 1;
                pCmdRef->iHopPings = 0;
                pCmdRef->uHopAddress = 0;
            }

            // since we don't have a DirtyAddr, create one from the physical address
            DirtyAddrFromHostAddr(&DirtyAddr, &pCmdRef->uTargetAddress);

            // issue ping request
            ProtoPingRequest(pCmdRef->pProtoPing, &DirtyAddr, NULL, 0, pCmdRef->iTimeout, pCmdRef->iTtl);
        }
    }

    // keep running
    return(ZCallback(&_CmdTracertCB, 10));
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdTracert

    \Description
        Initiate a traceroute command.

    \Input *argz    - module context
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Output
        None.

    \Version 10/21/2002 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdTracert(ZContext *argz, int32_t argc, char *argv[])
{
    TraceCmdRefT *pCmdRef;
    int32_t         iName;

    // check usage
    if (argc < 2)
    {
        ZPrintf("   use protoping to execute a traceroute\n");
        _CmdTracertUsage(argv[0]);
        return(0);
    }

    // allocate context
    pCmdRef = (TraceCmdRefT *) ZContextCreate(sizeof(*pCmdRef));
    memset(pCmdRef, 0, sizeof(*pCmdRef));
    pCmdRef->iTimeout = 3*1000;

    // set default options
    pCmdRef->iMaxHops = 30;
    pCmdRef->iNumHops = 1;
    pCmdRef->iTtl = 1;
    pCmdRef->iPingsPerHop = 3;
    pCmdRef->bResolve = TRUE;

    // override options from commandline
    iName = _CmdTracertGetArgs(pCmdRef, argc, argv);
    if (iName == argc)
    {
        _CmdTracertUsage(argv[0]);
        return(0);
    }

    // copy hostname
    strcpy(pCmdRef->strHostname,argv[iName]);

    // lookup host and set timeout
    pCmdRef->pHost = ProtoNameAsync(pCmdRef->strHostname, 5000);
    pCmdRef->uTick = ZTick()+pCmdRef->iTimeout;
    pCmdRef->iNumPings = pCmdRef->iPingsPerHop;

    // let callback do the work
    return(ZCallback(&_CmdTracertCB, 10));
}



