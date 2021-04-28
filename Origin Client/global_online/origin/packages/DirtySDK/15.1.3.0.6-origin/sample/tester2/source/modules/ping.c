/*H********************************************************************************/
/*!
    \File ping.c

    \Description
        Ping tester module.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 04/15/2005 (jbrookes) First Version, ported from Tester
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>
#include <stdlib.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/proto/protoname.h"
#include "DirtySDK/proto/protoping.h"

#include "libsample/zlib.h"

#include "testermodules.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct PingCmdRefT          // current module state
{
    ProtoPingRefT *pRef;
    HostentT      *pHost;           // host address
    uint32_t      uTick;            // number of ticks until death
    int32_t       iCt;              // number of iterations left
    int32_t       iNumSent;         // total number of packets sent
    int32_t       iTotalTicks;      // total number of ticks
    int32_t       iIdent;           // ident to set (zero=use default)
    int32_t       iMinTicks;        // min tick count
    int32_t       iMaxTicks;        // max tick count
    int32_t       iTimeout;         // timeout in ms
    uint32_t      uServiceId;       // the service id of the server to ping (0=default gateway)
    int32_t       uPort;            // qos/udp port
    uint8_t       bListen;          // listen mode
    uint8_t       bVerbose;         // verbose output?
    uint8_t       bServer;          // ping a server?
    uint8_t       bQos;             // if true, use QoS
    uint8_t       bUdp;             // if true, use UDP
    char          strHostname[256];
} PingCmdRefT;


/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _CmdPingUsage

    \Description
        Display usage information.

    \Input *pCmdName    - pointer to name of command

    \Output
        None.

    \Version 04/19/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _CmdPingUsage(const char *pCmdName){
    ZPrintf("   test the protoping module\n");
    ZPrintf("   usage: %s [-i<ident>] [-n<count>] [-l|-v] [-q|-u<port] [-s<serviceId>] [-t<type>] [-w<timeout>] hostname\n\n", pCmdName);
    ZPrintf("      -i<ident>        set two-character ping ident\n");
    ZPrintf("      -l               set up in listen mode\n");
    ZPrintf("      -n<count>        number of echo requests to send\n");
    ZPrintf("      -q<port>         use qos\n");
    ZPrintf("      -u<port>         use udp\n");
    ZPrintf("      -v               verbose output\n");
    ZPrintf("      -s<serviceId>    ping the server with the supplied service id\n");
    ZPrintf("      -w<timeout>      timeout, in ms\n");
}

/*F********************************************************************************/
/*!
    \Function _CmdPingGetArgs

    \Description
        Get command line arguments.

    \Input *pCmdRef - pointer to ping ref
    \Input argc     - argument count
    \Input *argv[]  - argument list
    
    \Output
        int32_t         - index past end of parsed arguments

    \Version 04/19/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdPingGetArgs(PingCmdRefT *pCmdRef, int32_t argc, char *argv[])
{
    int32_t iArg;
    char *pOpt;

    for (iArg = 1; iArg < argc; iArg++)
    {
        if (argv[iArg][0] == '-')
        {
            if ((pOpt = strchr(argv[iArg], 'i')) != NULL)
            {
                pCmdRef->iIdent = (pOpt[1] << 8) | pOpt[2];
            }
            if ((pOpt = strchr(argv[iArg], 'l')) != NULL)
            {
                pCmdRef->bListen = TRUE;
            }
            if ((pOpt = strchr(argv[iArg], 'n')) != NULL)
            {
                pCmdRef->iCt = (int32_t)strtol(pOpt + 1,NULL,10);
            }
            if ((pOpt = strchr(argv[iArg], 'q')) != NULL)
            {
                pCmdRef->uPort = (int32_t)strtol(pOpt+1, NULL, 10);
                pCmdRef->bQos = TRUE;
            }
            if ((pOpt = strchr(argv[iArg], 'u')) != NULL)
            {
                pCmdRef->uPort = (int32_t)strtol(pOpt+1, NULL, 10);
                pCmdRef->bUdp = TRUE;
            }
            if ((pOpt = strchr(argv[iArg], 'v')) != NULL)
            {
                pCmdRef->bVerbose = TRUE;
            }
            if ((pOpt = strchr(argv[iArg], 's')) != NULL)
            {
                pCmdRef->bServer = TRUE;
                pCmdRef->uServiceId = (int32_t)strtol(pOpt+1, NULL, 16);
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
    \Function _CmdPingDestroy

    \Description
        Destroy ref.

    \Input *pCmdRef  - pointer to ping ref
    
    \Output
        None.

    \Version 04/19/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _CmdPingDestroy(PingCmdRefT *pCmdRef)
{
    // release hostname resolve ref
    if (pCmdRef->pHost)
    {
        pCmdRef->pHost->Free(pCmdRef->pHost);
        pCmdRef->pHost = NULL;
    }

    // release ping ref
    if (pCmdRef->pRef)
    {
        ProtoPingDestroy(pCmdRef->pRef);
        pCmdRef->pRef = NULL;
    }
}

/*F********************************************************************************/
/*!
    \Function _CmdPingListenCB

    \Description
        Callback for ping listen.

    \Input Standard zlib stuff.
    
    \Output
        Standard result code.

    \Version 04/19/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdPingListenCB(ZContext *argz, int32_t argc, char **argv)
{
    PingCmdRefT *pCmdRef = (PingCmdRefT *)argz;
    ProtoPingResponseT PingResp;
    uint8_t aBuffer[256];
    int32_t iLen = sizeof(aBuffer);

    // check for kill
    if (argc == 0)
    {
        _CmdPingDestroy(pCmdRef);
        ZPrintf("%s: killed\n", argv[0]);
        return(0);
    }

    // listen for pings (but don't respond)
    ProtoPingResponse(pCmdRef->pRef, aBuffer, &iLen, &PingResp);

    // keep running
    return(ZCallback(&_CmdPingListenCB, 100));
}

/*F********************************************************************************/
/*!
    \Function _CmdPingCB

    \Description
        Callback for ping command.

    \Input Standard zlib stuff.
    
    \Output
        Standard result code.

    \Version 04/19/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdPingCB(ZContext *argz, int32_t argc, char **argv)
{
    int32_t iLen, iHost, iPing;
    const unsigned char strData[32] = "abcdefghijklmnopqrstuvwxyz01234";
    DirtyAddrT DirtyAddr;

    PingCmdRefT *pCmdRef = (PingCmdRefT *)argz;

    // check for kill
    if (argc == 0)
    {
        _CmdPingDestroy(pCmdRef);
        ZPrintf("%s: killed\n", argv[0]);
        return(0);
    }

    // check for timeout
    if (ZTick() > pCmdRef->uTick)
    {
        _CmdPingDestroy(pCmdRef);
        ZPrintf("%s: timeout\n", argv[0]);
        return(0);
    }

    if (pCmdRef->pRef == NULL)
    {
        // check for host addr resolve
        iHost = pCmdRef->pHost->Done(pCmdRef->pHost);
        if (iHost == -1)
        {
            _CmdPingDestroy(pCmdRef);
            ZPrintf("%s: unknown host\n",argv[0]);
            return(0);
        }
        else if (iHost != 0)
        {
            // echo resolved hostname in dot notation
            ZPrintf("ping: pinging %s [%a]\n", pCmdRef->strHostname, pCmdRef->pHost->addr);

            // allocate pingref
            if ((pCmdRef->pRef = ProtoPingCreate(0)) == NULL)
            {
                _CmdPingDestroy(pCmdRef);
                ZPrintf("ping: unable to create protoping ref; exiting\n");
                return(0);
            }

            // if we have an ident override, set it
            if (pCmdRef->iIdent != 0)
            {
                ProtoPingControl(pCmdRef->pRef, 'idnt', pCmdRef->iIdent, NULL);
            }

            // set verbosity
            ProtoPingControl(pCmdRef->pRef, 'spam', pCmdRef->bVerbose, NULL);

            if (pCmdRef->bQos)
            {
                ZPrintf("ping: using QoS with port %d\n", pCmdRef->uPort);
                ProtoPingControl(pCmdRef->pRef, 'type', PROTOPING_TYPE_QOS, &pCmdRef->uPort);
            }
            if (pCmdRef->bUdp)
            {
                ProtoPingControl(pCmdRef->pRef, 'prot', IPPROTO_UDP, NULL);
                if (pCmdRef->uPort != 0)
                {
                    ZPrintf("ping: using udp with port %d\n", pCmdRef->uPort);
                    ProtoPingControl(pCmdRef->pRef, 'port', pCmdRef->uPort, NULL);
                }
                else
                {
                    ZPrintf("ping: using udp with default port\n");
                }
            }

            // since we don't have a DirtyAddr, create one from the physical address
            DirtyAddrFromHostAddr(&DirtyAddr, &pCmdRef->pHost->addr);

            // issue ping request
            if (!pCmdRef->bServer)
            {
                if (ProtoPingRequest(pCmdRef->pRef, &DirtyAddr, strData, sizeof(strData), 0, 0) < 0)
                {
                    _CmdPingDestroy(pCmdRef);
                    ZPrintf("%s: error issuing ping request\n", argv[0]);
                    return(0);
                }
            }
            else
            {
                uint32_t uServerAddr = (pCmdRef->uServiceId != 0) ? pCmdRef->uServiceId : pCmdRef->pHost->addr;
                if (ProtoPingRequestServer(pCmdRef->pRef, uServerAddr, strData, sizeof(strData), 0, 0) < 0)
                {
                    _CmdPingDestroy(pCmdRef);
                    ZPrintf("%s: error issuing ping server request\n", argv[0]);
                    return(0);
                }
            }
            pCmdRef->iNumSent++;
        }
    }
    else
    {
        ProtoPingResponseT PingResp;
        uint8_t strBuffer[sizeof(strData)*2];

        iLen = sizeof(strBuffer);
        iPing = ProtoPingResponse(pCmdRef->pRef, strBuffer, &iLen, &PingResp);

        if (iPing != 0)
        {
            uint32_t uAddr, uSeqn;

            // get info from response structure
            uAddr = PingResp.uAddr;
            uSeqn = PingResp.uSeqn;

            // update stats
            pCmdRef->iTotalTicks += iPing;
            pCmdRef->iMinTicks = (iPing < pCmdRef->iMinTicks) ? iPing : pCmdRef->iMinTicks;
            pCmdRef->iMaxTicks = (iPing > pCmdRef->iMaxTicks) ? iPing : pCmdRef->iMaxTicks;

            // echo
            ZPrintf("%d bytes from %a: icmp_seq=%d time=%dms\n", sizeof(strBuffer), uAddr, uSeqn, iPing);

            if (--pCmdRef->iCt <= 0)
            {
                ZPrintf("Ping statistics for %a:\n", uAddr);
                ZPrintf("  Packets Sent: %d, Packets Received: %d, Packets Lost: %d (%2.2f%%)\n",
                    pCmdRef->iNumSent, pCmdRef->iNumSent, 0, 0.0f);
                ZPrintf("  Approximate round trip times in ms:\n");
                ZPrintf("    Min: %dms, Max: %dms, Avg: %dms\n",
                    pCmdRef->iMinTicks, pCmdRef->iMaxTicks, pCmdRef->iTotalTicks/pCmdRef->iNumSent);

                // shutdown module after ping received
                _CmdPingDestroy(pCmdRef);
                return(0);
            }
            else
            {
                // reset timeout
                pCmdRef->uTick = ZTick() + pCmdRef->iTimeout;

                // send next request
                if (!pCmdRef->bServer)
                {
                    // since we don't have a DirtyAddr, create one from the physical address
                    DirtyAddrFromHostAddr(&DirtyAddr, &pCmdRef->pHost->addr);
                    ProtoPingRequest(pCmdRef->pRef, &DirtyAddr, strData, sizeof(strData), 0, 0);
                }
                else
                {
                    uint32_t uServerAddr = (pCmdRef->uServiceId != 0) ? pCmdRef->uServiceId : pCmdRef->pHost->addr;
                    ProtoPingRequestServer(pCmdRef->pRef, uServerAddr, strData, sizeof(strData), 0, 0);
                }
                pCmdRef->iNumSent++;
            }
        }
    }
        
    // keep running
    return(ZCallback(&_CmdPingCB, 100));
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdPing

    \Description
        Initiate a ping command.

    \Input Standard zlib stuff.
    
    \Output
        Standard result code.

    \Version 04/19/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdPing(ZContext *argz, int32_t argc, char *argv[])
{
    PingCmdRefT *pCmdRef;
    int32_t         iName;

    // check usage
    if (argc < 2)
    {
        _CmdPingUsage(argv[0]);
        return(0);
    }

    // allocate context
    pCmdRef = (PingCmdRefT *) ZContextCreate(sizeof(*pCmdRef));
    ds_memclr(pCmdRef, sizeof(*pCmdRef));
    pCmdRef->iTimeout = 10*1000;

    // set default options
    pCmdRef->iCt = 3;
    pCmdRef->iNumSent = 0;
    pCmdRef->iTotalTicks = 0;
    pCmdRef->iMinTicks = 10*1000;
    pCmdRef->iMaxTicks = 0;

    // override options from commandline
    iName = _CmdPingGetArgs(pCmdRef, argc, argv);

    // if in listen mode, set it up now
    if (pCmdRef->bListen)
    {
        pCmdRef->pRef = ProtoPingCreate(0);
        if (pCmdRef->iIdent != 0)
        {
            ProtoPingControl(pCmdRef->pRef, 'idnt', pCmdRef->iIdent, NULL);
        }
        if (pCmdRef->uPort > 0)
        {
            ProtoPingControl(pCmdRef->pRef, 'type', PROTOPING_TYPE_QOS, &(pCmdRef->uPort));
            ProtoPingControl(pCmdRef->pRef, 'list', 0, NULL);
        }
        ProtoPingControl(pCmdRef->pRef, 'spam', pCmdRef->bVerbose, NULL);
        ZPrintf(("ping: listening for pings\n"));
        return(ZCallback(&_CmdPingListenCB, 100));
    }
    else if (iName >= argc)
    {
        _CmdPingUsage(argv[0]);
        return(0);
    }

    // copy hostname
    ds_strnzcpy(pCmdRef->strHostname, argv[iName], sizeof(pCmdRef->strHostname));

    // lookup host and set timeout
    pCmdRef->pHost = ProtoNameAsync(pCmdRef->strHostname, 5000);
    pCmdRef->uTick = ZTick() + pCmdRef->iTimeout;

    // let callback do the work
    return(ZCallback(&_CmdPingCB, 100));
}

