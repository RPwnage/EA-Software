/*H*************************************************************************************/
/*!
    \File    udp.c

    \Description
        Reference application for the udp sockets.

    \Copyright
        Copyright (c) Electronic Arts 2012.    ALL RIGHTS RESERVED.

    \Version 11/12/2012 (mclouatre)
*/
/*************************************************************************************H*/

/*** Include files *********************************************************************/
#include <string.h>
#include <stdlib.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/proto/protoudp.h"

#include "zlib.h"
#include "zfile.h"
#include "zmem.h"
#include "testersubcmd.h"
#include "testermodules.h"

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct UdpAppT
{
    ProtoUdpT *pProtoUdp;
    char aRecvBuf[2048];
    char aSendBuf[2048];

    int32_t iDummyCounter;

    uint8_t bStarted;

    unsigned char bZCallback;
} UdpAppT;

/*** Function Prototypes ***************************************************************/

static void _UdpCreate(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _UdpDestroy(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _UdpRecv(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _UdpSend(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _UdpSendFile(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);

/*** Variables *************************************************************************/

// Private variables
static T2SubCmdT _Udp_Commands[] =
{
    { "create",              _UdpCreate           },
    { "destroy",             _UdpDestroy          },
    { "recv",                _UdpRecv             },
    { "send",                _UdpSend             },
    { "fsend",               _UdpSendFile         },
    { "",                    NULL                 }
};

static UdpAppT _Udp_App;

// Public variables

/*** Private Functions *****************************************************************/

/*F********************************************************************************/
/*!
    \Function _CmdUdpCb

    \Description
        Udp callback, called after command has been issued.

    \Input *argz    - pointer to context
    \Input argc     - number of command-line arguments
    \Input *argv[]   - command-line argument list

    \Output
        int32_t      - result of zcallback, or zero to terminate

    \Version 11/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdUdpCb(ZContext *argz, int32_t argc, char *argv[])
{
    UdpAppT *pApp = &_Udp_App;

    // check for kill
    if (argc == 0)
    {
        ZPrintf("%s: killed\n", argv[0]);
        ProtoUdpDestroy(pApp->pProtoUdp);
        return(0);
    }

    // update module
    if (pApp->pProtoUdp != NULL)
    {
        ProtoUdpUpdate(pApp->pProtoUdp);
    }

    // keep recurring
    return(ZCallback(&_CmdUdpCb, 17));
}

/*F*************************************************************************************/
/*!
    \Function _UdpCreate

    \Description
        Udp subcommand - create udp socket

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 11/12/2012 (mclouatre)
*/
/**************************************************************************************F*/
static void _UdpCreate(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    UdpAppT *pApp = (UdpAppT *)_pApp;
    int32_t iBindPort;
    int32_t iResult;

    if ((bHelp == TRUE) || (argc != 2 && argc != 3))
    {
        ZPrintf("   usage: %s create [udp port]\n", argv[0]);
        return;
    }

    if (pApp->bStarted)
    {
        ZPrintf("%s: protoudp already created\n", argv[0]);
        return;
    }

    // get the bind port (use 0=ANY if not specified)
    iBindPort = (argc == 3) ? (int32_t)strtol(argv[2], NULL, 10) : 0;

    // allocate ProtoUdp module
    if ((pApp->pProtoUdp = ProtoUdpCreate(1024, 4)) == NULL)
    {
        ZPrintf("%s: unable to create protoudp module\n", argv[0]);
        return;
    }

    ZPrintf("%s: binding to port %d\n", argv[0], iBindPort);
    if ((iResult = ProtoUdpBind(pApp->pProtoUdp, iBindPort)) == SOCKERR_NONE)
    {
        struct sockaddr BindAddr;
        ProtoUdpGetLocalAddr(pApp->pProtoUdp, (struct sockaddr_in *)&BindAddr);
        ZPrintf("%s: successfully bound to port %d\n", argv[0], SockaddrInGetPort(&BindAddr));
    }
    else
    {
        ZPrintf("%s: failed to bind to port %d\n", argv[0], iBindPort);
        ProtoUdpDestroy(pApp->pProtoUdp);
        return;
    }

    // one-time install of periodic callback
    if (pApp->bZCallback == FALSE)
    {
        pApp->bZCallback = TRUE;
        ZCallback(_CmdUdpCb, 17);
    }

    pApp->bStarted = TRUE;
    pApp->iDummyCounter = 1;
}

/*F*************************************************************************************/
/*!
    \Function _UdpDestroy

    \Description
        Udp subcommand - destroy udp socket

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 11/12/2012 (mclouatre)
*/
/**************************************************************************************F*/
static void _UdpDestroy(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    UdpAppT *pApp = (UdpAppT *)_pApp;

    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s destroy\n", argv[0]);
        return;
    }

    if (!pApp->bStarted)
    {
        ZPrintf("%s: udp socket not yet created\n", argv[0]);
        return;
    }

    ProtoUdpDestroy(pApp->pProtoUdp);
    pApp->bStarted = FALSE;
}


/*F*************************************************************************************/
/*!
    \Function _UdpRecv

    \Description
        Udp subcommand - listen for udp datagrams on specified port

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 11/12/2012 (mclouatre)
*/
/**************************************************************************************F*/
static void _UdpRecv(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    UdpAppT *pApp = (UdpAppT *)_pApp;
    int32_t iResult;

    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s recv\n", argv[0]);
        return;
    }

    if (!pApp->bStarted)
    {
        ZPrintf("%s: udp socket not yet created\n", argv[0]);
        return;
    }

    iResult = ProtoUdpRecv(pApp->pProtoUdp, pApp->aRecvBuf, sizeof(pApp->aRecvBuf));
    ZPrintf("%s: ProtoUdpRecv() returned %d\n", argv[0], iResult);
    if (iResult > 0)
    {
        ZPrintf(" first byte = 0x%02x - last byte = 0x%02x\n", pApp->aRecvBuf[0], pApp->aRecvBuf[iResult-1]);
    }
}

/*F*************************************************************************************/
/*!
    \Function _UdpSend

    \Description
        Udp subcommand - send a udp datagram on a specified ippair/port

    \Input *_pApp   - pointer to FriendApi app
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 11/12/2012 (mclouatre)
*/
/**************************************************************************************F*/
static void _UdpSend(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    UdpAppT *pApp = (UdpAppT *)_pApp;
    struct sockaddr peerAddr;
    int32_t iPeerPort, iSize, iResult;
    char *pWrite, *pEnd;

    if ((bHelp == TRUE) || (argc < 4) || (argc > 5))
    {
        ZPrintf("   usage: %s send <ipaddr> <udp port> [size]\n", argv[0]);
        return;
    }

    if (!pApp->bStarted)
    {
        ZPrintf("%s: udp socket not yet created\n", argv[0]);
        return;
    }

    // get the peer port
    iPeerPort = (int32_t)strtol(argv[3], NULL, 10);

    // initialize peer addr
    SockaddrInit(&peerAddr, AF_INET);
    SockaddrInSetPort(&peerAddr, iPeerPort);
    SockaddrInSetAddrText(&peerAddr, argv[2]);

    // get size
    iSize = (argc == 5) ? (int32_t)strtol(argv[4], NULL, 10) : (signed)sizeof(pApp->aSendBuf);
    if (iSize > (signed)sizeof(pApp->aSendBuf))
    {
        ZPrintf("%s: limiting send size to %d\n", argv[0], sizeof(pApp->aSendBuf));
        iSize = sizeof(pApp->aSendBuf);
    }

    // initialize dummy buffer
    for (pWrite = (char *)&pApp->aSendBuf, pEnd = pWrite + iSize; pWrite != pEnd; pWrite += 1)
    {
        *pWrite = pApp->iDummyCounter;
    }
    pApp->iDummyCounter++;

    ZPrintf("%s: send %d byte udp datagram to %a:%d\n", argv[0], iSize, SockaddrInGetAddr(&peerAddr), SockaddrInGetPort(&peerAddr));
    iResult = ProtoUdpSendTo(pApp->pProtoUdp, pApp->aSendBuf, iSize, &peerAddr);
    ZPrintf("%s: ProtoUdpSendto() returned %d\n", argv[0], iResult);
}

/*F*************************************************************************************/
/*!
    \Function _UdpSendFile

    \Description
        Udp subcommand - send a udp datagram on a specified ippair/port with the data
        sourced from the specified file

    \Input *_pApp   - pointer to udp state
    \Input argc     - argument count
    \Input **argv   - argument list

    \Version 04/23/2015 (jbrookes)
*/
/**************************************************************************************F*/
static void _UdpSendFile(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    UdpAppT *pApp = (UdpAppT *)_pApp;
    struct sockaddr peerAddr;
    int32_t iFileSize, iResult;
    char *pFileBuf;

    if ((bHelp == TRUE) || (argc < 4) || (argc > 5))
    {
        ZPrintf("   usage: %s fsend <ipaddr> <udp port> [filepath]\n", argv[0]);
        return;
    }

    if (!pApp->bStarted)
    {
        ZPrintf("%s: udp socket not yet created\n", argv[0]);
        return;
    }

    // initialize peer addr&port
    SockaddrInit(&peerAddr, AF_INET);
    SockaddrInSetPort(&peerAddr, (int32_t)strtol(argv[3], NULL, 10));
    SockaddrInSetAddrText(&peerAddr, argv[2]);

    // load the file
    if ((pFileBuf = (char *)ZFileLoad(argv[4], &iFileSize, ZFILE_OPENFLAG_RDONLY|ZFILE_OPENFLAG_BINARY)) == NULL)
    {
        ZPrintf("%s: could not open file '%s' for sending\n", argv[4]);
        return;
    }
    // cap size and copy to send buf
    if (iFileSize > (signed)sizeof(pApp->aSendBuf))
    {
        ZPrintf("%s: limiting send size to %d (requested=%d)\n", argv[0], sizeof(pApp->aSendBuf), iFileSize);
        iFileSize = sizeof(pApp->aSendBuf);
    }
    memcpy(pApp->aSendBuf, pFileBuf, iFileSize);
    // free allocated file buffer
    ZMemFree(pFileBuf);

    ZPrintf("%s: send %d byte udp datagram to %a:%d\n", argv[0], iFileSize, SockaddrInGetAddr(&peerAddr), SockaddrInGetPort(&peerAddr));
    iResult = ProtoUdpSendTo(pApp->pProtoUdp, pApp->aSendBuf, iFileSize, &peerAddr);
    ZPrintf("%s: ProtoUdpSendto() returned %d\n", argv[0], iResult);
}


/*** Public Functions ******************************************************************/

/*F*************************************************************************************/
/*!
    \Function    CmdUdp

    \Description
        Udp command.

    \Input *argz    - unused
    \Input argc     - argument count
    \Input **argv   - argument list

    \Output
        int32_t         - zero

    \Version 12/11/2011 (mclouatre)
*/
/**************************************************************************************F*/
int32_t CmdUdp(ZContext *argz, int32_t argc, char *argv[])
{
    T2SubCmdT *pCmd;
    UdpAppT *pApp = &_Udp_App;
    unsigned char bHelp;

    // handle basic help
    if ((argc <= 1) || (((pCmd = T2SubCmdParse(_Udp_Commands, argc, argv, &bHelp)) == NULL)))
    {
        ZPrintf("   test the protoudp module\n");
        T2SubCmdUsage(argv[0], _Udp_Commands);
        return(0);
    }

    // if no ref yet, make one
    if ((pCmd->pFunc != _UdpCreate) && (pApp->pProtoUdp == NULL))
    {
        char *pCreate = "create";
        ZPrintf("   %s: ref has not been created - creating\n", argv[0]);
        _UdpCreate(pApp, 1, &pCreate, bHelp);
    }

    // hand off to command
    pCmd->pFunc(pApp, argc, argv, bHelp);

    return(0);
}
