/*H*************************************************************************************/
/*!

    \File T2Host.c

    \Description
        Tester2 Host Application Framework

    \Copyright
        Copyright (c) Electronic Arts 2004.  ALL RIGHTS RESERVED.

    \Version    1.0 03/22/2005 (jfrank) First Version
*/
/*************************************************************************************H*/

/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
//#include <termios.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/util/tagfield.h"

#include "zlib.h"
#include "zmemtrack.h"

#include "testerhostcore.h"
#include "testercomm.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/


/*** Type Definitions ******************************************************************/

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// tester host module
TesterHostCoreT *g_pHostCore;

static volatile uint8_t _bContinue = TRUE;

/*** Private Functions *****************************************************************/


/*F********************************************************************************/
/*!
    \Function _T2HostDisplayOutput

    \Description
        Take input from TesterConsole and display it somewhere

    \Input *pBuf    - string containing the debug output to display
    \Input  iLen    - length of buffer
    \Input  iRefcon - user-specified parameter (unused)
    \Input *pRefptr - user-specified parameter (window pointer)

    \Output None

    \Version 04/13/2005 (jfrank)
*/
/********************************************************************************F*/
static void _T2HostDisplayOutput(const char *pBuf, int32_t iLen, int32_t iRefcon, void *pRefptr)
{
    //printf("%s",pBuf);
}


/*F********************************************************************************/
/*!
    \Function _T2HostCommandlineProcess

    \Description
        Clear the console

    \Input  *argz   - environment
    \Input   argc   - num args
    \Input **argv   - arg list

    \Output  int32_t    - standard return code

    \Version 04/07/2005 (jfrank)
*/
/********************************************************************************F*/
static int32_t _TestHostProcessInput(char *pCommandLine, int32_t iCommandLen, int32_t *pCommandOff)
{
    struct pollfd PollFd;
    int32_t iResult;
    char cInput = 0;

    // poll for input, blocking up to 20ms
    PollFd.fd = 0;
    PollFd.events = POLLIN;
    if ((iResult = poll(&PollFd, 1, 20)) < 0)
    {
        NetPrintf(("t2host: poll() error %d trying to get input from stdin\n", errno));
    }

    // did we get any input?
    if (PollFd.revents & POLLIN)
    {
        if ((iResult = read(0, &cInput, sizeof(cInput))) > 0)
        {
            pCommandLine[*pCommandOff] = cInput;
            *pCommandOff += 1;
        }
        else if (iResult < 0)
        {
            NetPrintf(("t2host: read() error %d trying to read input from stdin\n", errno));
        }
    }

    // no update
    return(cInput == '\n');
}

/*F********************************************************************************/
/*!
    \Function _T2HostCmdClear

    \Description
        Clear the console

    \Input  *argz   - environment
    \Input   argc   - num args
    \Input **argv   - arg list

    \Output  int32_t    - standard return code

    \Version 04/07/2005 (jfrank)
*/
/********************************************************************************F*/
static int32_t _T2HostCmdClear(ZContext *argz, int32_t argc, char **argv)
{
    if (argc < 1)
    {
        ZPrintf("   clear the display.\n");
        ZPrintf("   usage: %s\n", argv[0]);
    }
    else
    {
#if 0
        // clear the console
        //TesterConsoleT *pConsole;
        if ((pConsole = (TesterConsoleT *)TesterRegistryGetPointer("CONSOLE")) != NULL)
        {
            TesterConsoleClear(pConsole);
        }
#endif
    }
    return(0);
}


/*F********************************************************************************/
/*!
    \Function _T2HostCmdExit

    \Description
        Quit

    \Input  *argz   - environment
    \Input   argc   - num args
    \Input **argv   - arg list

    \Output  int32_t    - standard return code

    \Version 04/05/2005 (jfrank)
*/
/********************************************************************************F*/
static int32_t _T2HostCmdExit(ZContext *argz, int32_t argc, char **argv)
{
    _bContinue = FALSE;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _T2HostRegisterModules

    \Description
        Register client commands (local commands, like exit, history, etc.)

    \Input None

    \Output None

    \Version 04/05/2005 (jfrank)
*/
/********************************************************************************F*/
static void _T2HostRegisterModules(void)
{
    TesterHostCoreRegister(g_pHostCore, "exit",     &_T2HostCmdExit);
    TesterHostCoreRegister(g_pHostCore, "clear",    &_T2HostCmdClear);
}


/*** Public Functions ******************************************************************/

int main(int32_t argc, char *argv[])
{
    char strParams[512], strCommandLine[256] = "", strBase[256] = "", strHostName[128] = "";
    int32_t iArg, iCommandOff = 0;

    // start the memtracker before we do anything else
    ZMemtrackStartup();

    ZPrintf(("\nStarting T2Host.\n\n"));

    // get arguments
    for (iArg = 0; iArg < argc; iArg++)
    {
        if (!strncmp(argv[iArg], "-path=", 6))
        {
            ds_strnzcpy(strBase, &argv[iArg][6], sizeof(strBase));
            ZPrintf("t2host: base path=%s\n", strBase);
        }
        if (!strncmp(argv[iArg], "-connect=", 9))
        {
            ds_strnzcpy(strHostName, &argv[iArg][9], sizeof(strHostName));
            ZPrintf("t2host: connect=%s\n", strHostName);
        }
    }

    // create the module
    TagFieldClear(strParams, sizeof(strParams));
    TagFieldSetNumber(strParams, sizeof(strParams), "PLATFORM", TESTER_PLATFORM_NORMAL);
    TagFieldSetString(strParams, sizeof(strParams), "INPUTFILE", TESTERCOMM_HOSTINPUTFILE);
    TagFieldSetString(strParams, sizeof(strParams), "OUTPUTFILE", TESTERCOMM_HOSTOUTPUTFILE);
    TagFieldSetString(strParams, sizeof(strParams), "CONTROLDIR", strBase);
    TagFieldSetString(strParams, sizeof(strParams), "HOSTNAME", strHostName);

    // startup dirtysdk (must come before TesterHostCoreCreate() if we are using socket comm)
    NetConnStartup("-servicename=tester2");

    NetPrintf(("t2host: ipaddr=%a\n", NetConnStatus('addr', 0, NULL, 0)));

    NetPrintf(("t2host: macaddr=%s\n", NetConnMAC()));

    g_pHostCore = TesterHostCoreCreate(strParams);

    // define the function which will show stuff on the screen
    TesterHostCoreDisplayFunc(g_pHostCore, _T2HostDisplayOutput, 0, NULL);

    // register basic functions
    _T2HostRegisterModules();

    ZPrintf("T2Host Unix Application Successfully started.\n");

    // check for command-line command
    for (iArg = 1; iArg < argc; iArg += 1)
    {
        // don't include -base or -connect arg, if present
        if (!strncmp(argv[iArg], "-base=", 6) || !strncmp(argv[iArg], "-connect=", 9))
        {
            continue;
        }
        // add to command-line
        ds_strnzcat(strCommandLine, argv[iArg], sizeof(strCommandLine));
        ds_strnzcat(strCommandLine, " ", sizeof(strCommandLine));
    }
    ZPrintf("> %s\n", strCommandLine);
    TesterHostCoreDispatch(g_pHostCore, strCommandLine);
    strCommandLine[0] = '\0';

    while(_bContinue)
    {
        // check for input
        if (_TestHostProcessInput(strCommandLine, sizeof(strCommandLine), &iCommandOff))
        {
            strCommandLine[iCommandOff-1] = '\0'; // remove lf
            TesterHostCoreDispatch(g_pHostCore, strCommandLine);
            strCommandLine[0] = '\0';
            iCommandOff = 0;
        }

        // pump the host core module
        TesterHostCoreUpdate(g_pHostCore, 1);

        // give time to zlib
        ZTask();
        ZCleanup();

        // give time to network
        NetConnIdle();

        // sleep for a int16_t while
        ZSleep(20);
    }

    // code is unreachable for now
    TesterHostCoreDisconnect(g_pHostCore);
    TesterHostCoreDestroy(g_pHostCore);

    // shut down the network
    NetConnShutdown(0);

    ZMemtrackShutdown();

    ZPrintf("\nQuitting T2Host.\n\n");

    return(0);
}

