/*H*************************************************************************************/
/*!
    \File    lobby.c

    \Description
        Reference application for the NetApi module.

    \Notes
        Base code from locker test function by jbrookes.

    \Copyright
        Copyright (c) Electronic Arts 2004.    ALL RIGHTS RESERVED.

    \Version    1.0        04/12/2005 (jfrank) First Version
*/
/*************************************************************************************H*/


/*** Include files *********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _XBOX
#include <xtl.h>
#endif

#include "platform.h"
#include "dirtysock.h"
#include "netconn.h"

#include "zlib.h"
#include "zmem.h"

#include "testerregistry.h"
#include "testerhostcore.h"
#include "testersubcmd.h"
#include "testermodules.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct NetAppT
{
    int32_t   iNetUp;
    int8_t    bExternalCleanupComplete;
} NetAppT;

/*** Function Prototypes ***************************************************************/

static void _NetStartup(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _NetQuery(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
#if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
static void _NetLogin(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _NetNameMap(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _NetFriends(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
#elif DIRTYCODE_PLATFORM == DIRTYCODE_PS3
static void _NetTicket(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
#endif
static void _NetConnect(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _NetDisconnect(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _NetShutdown(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _NetControl(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _NetStatus(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _NetLoss(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _NetPorts(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);

#if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
static void _NetLogin(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _NetAddrMap(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _NetNameMap(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _NetFriends(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
#endif

#if (DIRTYCODE_PLATFORM == DIRTYCODE_XENON) || (DIRTYCODE_PLATFORM == DIRTYCODE_PS3)
static void _NetCleanup(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
#endif

/*** Variables *************************************************************************/

static T2SubCmdT _Net_Commands[] =
{
    { "startup",        _NetStartup       },
    { "start",          _NetStartup       },
    { "query",          _NetQuery         },
    { "ctrl",           _NetControl       },
    { "status",         _NetStatus        },
    { "loss",           _NetLoss          },
    { "ports",          _NetPorts         },
#if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
    { "login",          _NetLogin         },
    { "xacl",           _NetAddrMap       },
    { "xapr",           _NetAddrMap       },
    { "+xam",           _NetAddrMap       },
    { "-xam",           _NetAddrMap       },
    { "xncl",           _NetNameMap       },
    { "xnpr",           _NetNameMap       },
    { "+xnm",           _NetNameMap       },
    { "-xnm",           _NetNameMap       },
    { "friends",        _NetFriends       },
#elif DIRTYCODE_PLATFORM == DIRTYCODE_PS3
    { "ticket",         _NetTicket        },
#endif
#if (DIRTYCODE_PLATFORM == DIRTYCODE_XENON) || (DIRTYCODE_PLATFORM == DIRTYCODE_PS3)
    { "cleanup",        _NetCleanup       },
#endif
    { "connect",        _NetConnect       },
    { "disconnect",     _NetDisconnect    },
    { "shutdown",       _NetShutdown      },
    { "stop",           _NetShutdown      },
    { "",               NULL              }
};

static NetAppT _Net_App = { 0, 0 };

/*** Private Functions *****************************************************************/

/*F*************************************************************************************************/
/*!
    \Function _NetExtCleanupCallback

    \Description
        To test external cleanup mechanism

    \Input *pCallbackData   - pointer to platform-specific dataspace

    \Output
        int32_t             - zero=success; -1=try again; other negative=error

    \Version 10/01/2011 (mclouatre)
*/
/*************************************************************************************************F*/
static int32_t _NetExtCleanupCallback(void *pCallbackData)
{
    NetAppT *pApp = (NetAppT *)pCallbackData;

    if (pApp->bExternalCleanupComplete)
    {
        return(0);  // complete
    }
    else
    {
        return(-1);  // try again
    }
}

/*F*************************************************************************************/
/*!
    \Function _NetStartup

    \Description
        Net subcommand - start dirtysock

    \Input *pApp    - pointer to lobby app
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 04/12/2005 (jfrank)
*/
/**************************************************************************************F*/
static void _NetStartup(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    NetAppT *pApp = (NetAppT *)pCmdRef;
    char strBuf[64];
    int32_t iLoop;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s [start|startup] \"<startup parameters>\"\n", argv[0], argv[1]);
        return;
    }

    if (pApp->iNetUp == 1)
    {
        ZPrintf("NET: Network already started.\n");
        return;
    }

    ZMemSet(strBuf, 0, sizeof(strBuf));
    for(iLoop = 2; iLoop < argc; iLoop++)
    {
        strncat(strBuf, argv[iLoop], sizeof(strBuf) - strlen(strBuf) - 1);
        strncat(strBuf, " ", sizeof(strBuf) - strlen(strBuf) - 1);
    }
    ZPrintf("NET: Starting dirtysock with params {%s}.\n", strBuf);
    NetConnStartup(strBuf);

    #if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
    // enable LSP service
    NetConnControl('xlsp', 0, 0, NULL, NULL);
    #endif

    pApp->iNetUp = 1;
}

/*F*************************************************************************************/
/*!
    \Function _NetQuery

    \Description
        Net subcommand - issue a NetConnQuery call

    \Input *pApp    - pointer to lobby app
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 05/12/2005 (jbrookes)
*/
/**************************************************************************************F*/
static void _NetQuery(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    if ((bHelp == TRUE) || (argc > 3))
    {
        ZPrintf("   usage: %s [query] \"<startup parameters>\"\n", argv[0], argv[1]);
        return;
    }
    
    if (argc == 2)
    {
        NetConnQuery(NULL, NULL, 0);
    }
    else if (argc == 3)
    {
        NetConnQuery(argv[2], NULL, 0);
    }
}

/*F*************************************************************************************/
/*!
    \Function _NetLogin

    \Description
        Net subcommand - log into Xbox Live [XENON only]

    \Input *pApp    - pointer to lobby app
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 05/06/2005 (jbrookes)
*/
/**************************************************************************************F*/
#if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
static void _NetLogin(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    DWORD dwResult = XShowSigninUI(4, XSSUI_FLAGS_SHOWONLYONLINEENABLED);
    ZPrintf("NET: XShowSigninUI() returned %d\n", dwResult);
}
static void _NetAddrMap(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    int32_t iCmd, iResult = -1;

    // check args
    if ((bHelp == TRUE) || (argc < 2))
    {
        ZPrintf("usage: net xacl (clear addr map) OR\n");
        ZPrintf("       net xapr (print addr map) OR\n");
        ZPrintf("       net +xam <addr> <mask> <serviceid> (add addr map entry) OR\n");
        ZPrintf("       net -xam <addr> <mask> <serviceid> (del addr map entry)\n");
        return;
    }

    // ref command
    iCmd  = argv[1][0] << 24;
    iCmd |= argv[1][1] << 16;
    iCmd |= argv[1][2] << 8;
    iCmd |= argv[1][3];

    if (iCmd == 'xapr')
    {
        NetConnStatus('xapr', 0, NULL, 0);
        return;
    }
    if (iCmd == 'xacl')
    {
        SocketAddrMapT aSocketAddrMap[32];
        memset(aSocketAddrMap, 0, sizeof(aSocketAddrMap));
        iResult = NetConnControl('xmap', sizeof(aSocketAddrMap)/sizeof(aSocketAddrMap[0]), 0, aSocketAddrMap, NULL);
    }
    if ((iCmd == '+xam') || (iCmd == '-xam'))
    {
        if (argc == 5)
        {
            SocketAddrMapT aSocketAddrEntry;
            // format addr map entry
            memset(&aSocketAddrEntry, 0, sizeof(aSocketAddrEntry));
            aSocketAddrEntry.uAddr = SocketInTextGetAddr(argv[2]);
            aSocketAddrEntry.uMask = SocketInTextGetAddr(argv[3]);
            aSocketAddrEntry.uServiceId = strtol(argv[4], NULL, 16);
            if (iCmd == '+xam')
            {
                if ((aSocketAddrEntry.uRemap = NetConnStatus('xsad', aSocketAddrEntry.uServiceId, NULL, 0)) == 0)
                {
                    ZPrintf("net: unknown serviceID %s; addr map entry will not be added\n", argv[4]);
                    return;
                }
            }
            iResult = NetConnControl(iCmd, 0, 0, &aSocketAddrEntry, NULL);
        }
        else
        {
            ZPrintf("net: incorrect argument count; see help\n");
            return;
        }
    }
    ZPrintf("net: NetConnControl('%s') returned %d\n", argv[1], iResult);
}
static void _NetNameMap(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    int32_t iCmd, iResult = -1;

    // check args
    if ((bHelp == TRUE) || (argc < 2))
    {
        ZPrintf("usage: net xncl (clear name map) OR\n");
        ZPrintf("       net xnpr (print name map) OR\n");
        ZPrintf("       net +xnm <dnsname> <remapname> (add name map entry) OR\n");
        ZPrintf("       net -xnm <dnsname> <remapname> (del name map entry)\n");
        return;
    }

    // ref command
    iCmd  = argv[1][0] << 24;
    iCmd |= argv[1][1] << 16;
    iCmd |= argv[1][2] << 8;
    iCmd |= argv[1][3];

    if (iCmd == 'xnpr')
    {
        NetConnStatus('xnpr', 0, NULL, 0);
        return;
    }
    if (iCmd == 'xncl')
    {
        SocketAddrMapT aSocketNameMap[32];
        memset(aSocketNameMap, 0, sizeof(aSocketNameMap));
        iResult = NetConnControl('xmap', sizeof(aSocketNameMap)/sizeof(aSocketNameMap[0]), 0, aSocketNameMap, NULL);
    }
    if ((iCmd == '+xnm') || (iCmd == '-xnm'))
    {
        if (argc == 4)
        {
            SocketNameMapT aSocketNameEntry;
            // format name map entry
            memset(&aSocketNameEntry, 0, sizeof(aSocketNameEntry));
            strnzcpy(aSocketNameEntry.strDnsName, argv[2], sizeof(aSocketNameEntry.strDnsName));
            strnzcpy(aSocketNameEntry.strRemap, argv[3], sizeof(aSocketNameEntry.strRemap));
            iResult = NetConnControl(iCmd, 0, 0, &aSocketNameEntry, NULL);
        }
        else
        {
            ZPrintf("net: incorrect argument count; see help\n");
            return;
        }
    }
    ZPrintf("net: NetConnControl('%s') returned %d\n", argv[1], iResult);
}
static void _NetFriends(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    DWORD dwResult = XShowFriendsUI(0);
    ZPrintf("NET: XShowFriendsUI() returned %d\n", dwResult);
}
#endif // DIRTYCODE_PLATFORM == DIRTYCODE_XENON

#if DIRTYCODE_PLATFORM == DIRTYCODE_PS3
/*F*************************************************************************************/
/*!
    \Function _NetTicket

    \Description
        Net subcommand - test ps3 ticketing system

    \Input *pApp    - pointer to lobby app
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 10/09/2009 (jbrookes)
*/
/**************************************************************************************F*/
static void _NetTicket(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    uint8_t aTicketBuf[1024];
    int32_t iResult;
    #if DIRTYCODE_LOGGING
    const char *_strPlatEnv[] = { "", "sp-int", "prod-qa", "np" };
    #endif
    
    // acquire ticket
    if ((iResult = NetConnStatus('tick', 0, aTicketBuf, sizeof(aTicketBuf))) < 1)
    {
        NetPrintf(("net: NetConnStatus('tick') returned %d\n", iResult));
        return;
    }
    // display environment
    if ((iResult = NetConnStatus('envi', 0, NULL, 0)) < 1)
    {
        NetPrintf(("net: NetConnStatus('envi') returned %d\n", iResult));
        return;
    }
    NetPrintf(("net: platform environment is %s\n", _strPlatEnv[iResult]));
}
#endif // DIRTYCODE_PLATFORM == DIRTYCODE_PS3

#if (DIRTYCODE_PLATFORM == DIRTYCODE_XENON) || (DIRTYCODE_PLATFORM == DIRTYCODE_PS3)
/*F*************************************************************************************/
/*!
    \Function _NetCleanup

    \Description
        Net subcommand - test ps3 ticketing system

    \Input *pApp    - pointer to lobby app
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 10/01/2011 (mclouatre)
*/
/**************************************************************************************F*/
static void _NetCleanup(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    NetAppT *pApp = (NetAppT *)pCmdRef;

    // mark cleanup done
    pApp->bExternalCleanupComplete = TRUE;
}
#endif // (DIRTYCODE_PLATFORM == DIRTYCODE_XENON) || (DIRTYCODE_PLATFORM == DIRTYCODE_PS3)

/*F*************************************************************************************/
/*!
    \Function _NetConnect

    \Description
        Net subcommand - connect the networking

    \Input *pApp    - pointer to lobby app
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 04/12/2005 (jfrank) First Version
*/
/**************************************************************************************F*/
static void _NetConnect(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    if ((bHelp == TRUE) || (argc < 2) || (argc > 4))
    {
        ZPrintf("   usage: %s connect \"<startup parameters>\"\n", argv[0]);
        return;
    }

    if (argc == 2)
    {
        NetConnConnect(NULL, NULL, 0);
    }
    else if (argc == 3)
    {
        NetConnConnect((const NetConfigRecT *)argv[2], NULL, 0);
    }
    else if (argv[2][0] == '-')
    {
        NetConnConnect(NULL, argv[3], 0);
    }
    else
    {
        NetConnConnect((const NetConfigRecT *)argv[2], argv[3], 0);
    }
}

/*F*************************************************************************************/
/*!
    \Function _NetDisconnect

    \Description
        Net subcommand - connect the networking

    \Input *pApp    - pointer to lobby app
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 04/12/2005 (jfrank) First Version
*/
/**************************************************************************************F*/
static void _NetDisconnect(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s disconnect\n", argv[0]);
        return;
    }

    NetConnDisconnect();
}

/*F*************************************************************************************/
/*!
    \Function _NetShutdown

    \Description
        Net subcommand - shut down dirtysock

    \Input *pApp    - pointer to lobby app
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 04/12/2005 (jfrank) First Version
*/
/**************************************************************************************F*/
static void _NetShutdown(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    NetAppT *pApp = (NetAppT *)pCmdRef;
    TesterHostCoreT *pCore;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s [stop|shutdown]\n", argv[0]);
        return;
    }

    if (pApp->iNetUp == 0)
    {
        ZPrintf("NET: Network already shut down.\n");
        return;
    }

    pApp->iNetUp = 0;

    // signal to the core that we want to shut everything down
    if ((pCore = (TesterHostCoreT *)TesterRegistryGetPointer("CORE")) != NULL)
    {
        TesterHostCoreShutdown(pCore);
    }
}

/*F********************************************************************************/
/*!
    \Function _NetControl

    \Description
        Execute NetConnControl()

    \Input *pApp    - pointer to upnp module
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false

    \Version 09/27/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _NetControl(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    NetAppT *pApp = (NetAppT *)pCmdRef;
    int32_t iCmd, iValue=0, iValue2=0;
    void *pValue = NULL;
    void *pValue2 = NULL;

    if ((bHelp == TRUE) || (argc < 3))
    {
        ZPrintf("   usage: %s ctrl <cmd> [val1] [val2] [strVal] [strVal]\n", argv[0]);
        return;
    }

    iCmd  = argv[2][0] << 24;
    iCmd |= argv[2][1] << 16;
    iCmd |= argv[2][2] << 8;
    iCmd |= argv[2][3];
    
    if (argc > 3)
    {
        iValue = strtol(argv[3], NULL, 10);
    }
    if (argc > 4)
    {
        iValue2 = strtol(argv[4], NULL, 10);
    }
    if (argc > 5)
    {
        pValue = argv[5];
    }
    if (argc > 6)
    {
        pValue2 = argv[6];
    }

    if (strcmp("recu", argv[2]) == 0)
    {
        pApp->bExternalCleanupComplete = FALSE;
        pValue = (void *)_NetExtCleanupCallback;
        pValue2 = (void *)pApp;
    }

    ZPrintf("net: executing NetConnControl('%s', %d, %d, %s, %s)\n", argv[2], iValue, iValue2, pValue ? "<ptr>" : "(null)", pValue2 ? "<ptr>" : "(null)");
    NetConnControl(iCmd, iValue, iValue2, pValue, pValue2);
}

/*F*************************************************************************************/
/*!
    \Function _NetStatus

    \Description
        Net subcommand - Execute NetConnStatus()

    \Input *pApp    - pointer to lobby app
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 04/12/2005 (jfrank) First Version
*/
/**************************************************************************************F*/
static void _NetStatus(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    unsigned char *pToken;
    uint32_t uResult, uToken;

    if ((bHelp == TRUE) || (argc != 3))
    {
        ZPrintf("   usage: %s status <4-char status token to get>\n", argv[0]);
        return;
    }

    pToken = (unsigned char *)argv[2];
    uToken = (pToken[0] << 24) | (pToken[1] << 16) | (pToken[2] << 8) | pToken[3];
    uResult = NetConnStatus(uToken, 0, NULL, 0);

    // if printable, display result as text
    if (isprint((uResult >> 24) & 0xff) && isprint((uResult >> 16) & 0xff) &&
        isprint((uResult >>  8) & 0xff) && isprint((uResult >>  0) & 0xff))
    {
        ZPrintf("NET: Status of ('%c%c%c%c') is {'%c%c%c%c')",
            (uToken >> 24) & 0xff, (uToken >> 16) & 0xff, (uToken >> 8) & 0xff, uToken & 0xff,
            (uResult >> 24) & 0xff, (uResult >> 16) & 0xff, (uResult >> 8) & 0xff, uResult & 0xff);
    }
    else // display result as decimal and hex
    {
        ZPrintf("NET: Status of ('%c%c%c%c') is {%d/0x%08X)", 
            (uToken >> 24) & 0xff, (uToken >> 16) & 0xff, (uToken >> 8) & 0xff, uToken & 0xff,
            uResult, uResult);
    }
}

/*F*************************************************************************************/
/*!
    \Function _NetLoss

    \Description
        Net subcommand - control packet loss simulation

    \Input *pCmdRef - pointer to ref
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHep     - TRUE if we should print usage info and return

    \Version 06/29/2009 (jbrookes) First Version
*/
/**************************************************************************************F*/
static void _NetLoss(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    uint32_t uFrequency, uDuration, bVerbose;
    
    // usage
    if ((bHelp == TRUE) || (argc != 5))
    {
        ZPrintf("   usage: %s loss <frequency [0...255]> <duration [0...255]> <verbose [0|1]>\n", argv[0]);
        return;
    }
    
    // get args
    uFrequency = strtol(argv[2], NULL, 10);
    uDuration  = strtol(argv[3], NULL, 10);
    bVerbose   = strtol(argv[4], NULL, 10);
    
    // set it up
    NetConnControl('loss', NETCONN_PACKETLOSSPARAM(uFrequency, uDuration, bVerbose), 0, NULL, NULL);
}

/*F********************************************************************************/
/*!
    \Function _NetPorts

    \Description
        Like "netstat" on unix

    \Input *pApp    - pointer to upnp module
    \Input argc     - argument count
    \Input *argv[]  - argument list
    \Input bHelp    - true if help request, else false

    \Version 09/25/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _NetPorts(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    SocketT *pSocket;
    int32_t iResult;
    uint32_t uPort;
    struct sockaddr SockName;

    SockaddrInit(&SockName, AF_INET);

    // find ports that are bound
    for (uPort = 1024; uPort < 65536; uPort++)
    {
        // create a UDP socket
        if ((pSocket = SocketOpen(AF_INET, SOCK_DGRAM, IPPROTO_IP)) == NULL)
        {
            NetPrintf(("net: error -- could not allocate socket resource\n"));
            return;
        }

        // bind the socket
        SockaddrInSetPort(&SockName, uPort);
        iResult = SocketBind(pSocket, &SockName, sizeof(SockName));
        if (iResult == SOCKERR_ADDRINUSE)
        {
            ZPrintf("net: %5s %5d\n", "UDP", uPort);
        }

        // close the socket
        if (SocketClose(pSocket) != 0)
        {
            NetPrintf(("net: error -- could not free socket resource\n"));
            return;
        }

        // give some time to network stack
        NetConnSleep(1);
        NetConnIdle();
    }
}


/*** Public Functions ******************************************************************/


/*F*************************************************************************************/
/*!
    \Function    CmdNet

    \Description
        Net command.

    \Input *argz    - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Output
        int32_t     - zero

    \Version 11/24/04 (jbrookes)
*/
/**************************************************************************************F*/
int32_t CmdNet(ZContext *argz, int32_t argc, char *argv[])
{
    void *pCmdRef = &_Net_App;
    unsigned char bHelp;
    T2SubCmdT *pCmd;

    // handle shutdown
    if(argc == 0)
    {
        // nothing to do
        return(0);
    }
    // handle basic help
    else if ((argc < 2) || (((pCmd = T2SubCmdParse(_Net_Commands, argc, argv, &bHelp)) == NULL)))
    {
        T2SubCmdUsage(argv[0], _Net_Commands);
        return(0);
    }

    // hand off to command
    pCmd->pFunc(pCmdRef, argc, argv, bHelp);
    return(0);
}

