#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "dirtyerr.h"
#include "netconn.h"
#include "DirtySDK/util/tagfield.h"
#include "zlib.h"
#include "zmemtrack.h"
#include "testerhostcore.h"
#include "testercomm.h"
#include "ZFile.h"

#include "SampleCore.h"


/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// tester host module
static TesterHostCoreT *g_pHostCore;

static volatile int iContinue=1;

static char strSnapshotName[32] = "";

/*** Private Functions *****************************************************************/

/*F********************************************************************************/
/*!
    \Function _T2HostCmdExit

    \Description
        Quit

    \Input  *argz   - environment
    \Input   argc   - num args
    \Input **argv   - arg list

    \Output  int32_t    - standard return code

    \Version 05/25/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _T2HostCmdExit(ZContext *argz, int32_t argc, char **argv)
{
    if (argc >= 1)
    {
        iContinue = 0;
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _T2HostCmdSnapshot

    \Description
        Set up to take a snapshot.

    \Input  *argz   - environment
    \Input   argc   - num args
    \Input **argv   - arg list

    \Output  int32_t    - standard return code

    \Version 05/25/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _T2HostCmdSnapshot(ZContext *argz, int32_t argc, char **argv)
{
    if (argc == 2)
    {
        ds_strnzcpy(strSnapshotName, argv[1], sizeof(strSnapshotName));
    }
    else
    {
        ZPrintf("usage: snap <snapshotname>\n");
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _T2HostNetStart

    \Description
        Connect network using "net" commands, wait for network to initialize.

    \Input *pCore   - TesterHostCoreT ref

    \Output
        None.

    \Version 08/11/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _T2HostNetConnect(TesterHostCoreT *pCore)
{
    int32_t iStatus, iTimeout;

    // connect the network using "net" module
    TesterHostCoreDispatch(pCore, "net connect");

    // wait for network interface activation
    for (iTimeout = NetTick()+15*1000 ; ; )
    {
        // update network
        NetConnIdle();

        // get current status
        iStatus = NetConnStatus('conn', 0, NULL, 0);
        if ((iStatus == '+onl') || ((iStatus >> 24) == '-'))
        {
            break;
        }

        // check for timeout
        if (iTimeout < (signed)NetTick())
        {
            NetPrintf(("t2host: timeout waiting for interface activation\n"));
            break;
        }

        // give time to other threads
        NetConnSleep(500);
    }

    // check result code
    if ((iStatus = NetConnStatus('conn', 0, NULL, 0)) == '+onl')
    {
        NetPrintf(("t2host: interface active\n"));
    }
    else if ((iStatus >> 14) == '-')
    {
        NetPrintf(("t2host: error bringing up interface\n"));
    }
}



/*** Public Functions ******************************************************************/

/*F********************************************************************************/
/*!
    \Function InitializeT2

    \Description
        Initialize state before running the sample.

    \Input      None

    \Output     None

    \Version 29/05/2013 (tcho)
*/
/********************************************************************************F*/
void InitializeT2()
{
    char strBase[128] = "", strHostName[128] = "", strParams[512];

    // start the memtracker before we do anything else
    ZMemtrackStartup();

    // create the module
    TagFieldClear(strParams, sizeof(strParams));
    TagFieldSetNumber(strParams, sizeof(strParams), "PLATFORM", TESTER_PLATFORM_NORMAL);
    TagFieldSetString(strParams, sizeof(strParams), "INPUTFILE", TESTERCOMM_HOSTINPUTFILE);
    TagFieldSetString(strParams, sizeof(strParams), "OUTPUTFILE", TESTERCOMM_HOSTOUTPUTFILE);
    TagFieldSetString(strParams, sizeof(strParams), "CONTROLDIR", strBase);
    TagFieldSetString(strParams, sizeof(strParams), "HOSTNAME", strHostName);

    // startup dirtysdk (must come before TesterHostCoreCreate() if we are using socket comm)
    NetConnStartup("-servicename=tester2");

    // create tester2 host core
    g_pHostCore = TesterHostCoreCreate(strParams);

    // connect to the network (blocking; waits for connection success)
    _T2HostNetConnect(g_pHostCore);

    // register some basic commands
    TesterHostCoreRegister(g_pHostCore, "exit", _T2HostCmdExit);
    TesterHostCoreRegister(g_pHostCore, "snap", _T2HostCmdSnapshot);

    ZPrintf("\n Started T2Host. \n\n");
}

/*F********************************************************************************/
/*!
    \Function UpdateT2

    \Description
        Update tester2 processing

    \Input  timeTotal   total time
    \Input  timeDelta   time interval

    \Version 29/05/2013 (tcho)
*/
/********************************************************************************F*/
void UpdateT2 (float timeTotal, float timeDelta)
{
    // pump the hostcore module
    TesterHostCoreUpdate(g_pHostCore, 1);

    // give time to zlib
    ZTask();
    ZCleanup();

    // give time to network
    NetConnIdle();
}


/*F********************************************************************************/
/*!
    \Function UninitializeT2

    \Description
        UninitializeT2 (shut it down)

    \Input      None

    \Output     None

    \Version 29/05/2013 (tcho)
*/
/********************************************************************************F*/
void UninitializeT2()
{
    // clean up and exit
    TesterHostCoreDisconnect(g_pHostCore);
    TesterHostCoreDestroy(g_pHostCore);

    // shut down the network
    NetConnShutdown(0);

    ZMemtrackShutdown();

    ZPrintf("\nQuitting T2Host.\n\n");
}

/*F********************************************************************************/
/*!
    \Function main

    \Description
        Main routine for WinRT T2Host application.

    \Input ^argc    - argument list

    \Output int32_t - zero

    \Version 11/11/2012 (mclouatre)
*/
/********************************************************************************F*/
int main(Platform::Array<Platform::String^>^ args)
{
    auto sampleFactory = ref new SampleFactory();
    Windows::ApplicationModel::Core::CoreApplication::Run(sampleFactory);
}
