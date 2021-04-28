/*H********************************************************************************/
/*!
    \File main.cpp

    \Description
        Harness for the VoipTunnel server.

    \Copyright
        Copyright (c) 2006 Electronic Arts Inc.

    \Version 03/24/2006 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/dirtysock/dirtylib.h"

#include "DirtySDK/dirtysock/netconn.h"

#ifdef WIN32
 #include "winmain.h"

 #define COBJMACROS // exposes the COM C macros
 #include <netfw.h>
#else
 #include <unistd.h>
 #include <errno.h>
#endif

#include "memtrack.h"
#include "dirtycast.h"

#include "voipserver.h"
#include "voipserverconfig.h"
#include "voipgameserver.h"

#if defined(DIRTYCODE_PC)
extern uint32_t _NetLib_bUseHighResTimer;
#endif

/*** Defines **********************************************************************/

/*** Variables ********************************************************************/

// VoipTunnel server instance
static VoipServerRefT *_Server;

static uint32_t     _uSignalFlags;

static uint32_t     _uDebugLevel = DIRTYCAST_DBGLVL_MEMORY+1;  // start debug level high until we can read it in from the config file

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _VoipServerPrintfHook

    \Description
        Print output to stdout so it is visible when run from the commandline as well
        as in the debugger output.

    \Input *pParm   - unused
    \Input *pText   - text to print

    \Output
        int32_t     - if zero, suppress OutputDebugString printing by NetPrintf()

    \Notes
        If the debuglevel is greater than two, we suppress OutputDebugString printing,
        as there is so much output, printing debug output impedes the functioning of
        the server.

    \Version 04/04/2007 (jbrookes)
*/
/********************************************************************************F*/
#if DIRTYCODE_LOGGING
static int32_t _VoipServerPrintfHook(void *pParm, const char *pText)
{
    DirtyCastLogDbgPrintf(_uDebugLevel, "%s", pText);
    return(0);
}
#endif

/*F********************************************************************************/
/*!
    \Function _SignalProcess

    \Description
        Process SIGTERM/SIGINT (graceful shutdown).

    \Input iSigNum  - Which signal was sent
    
    \Notes
        Avoid using DirtyCastLogPrintf() from this function. First, it is good 
        practice to do as less as possible in a signal handler. Second, we did log
        from here in that past and it resulted in deadlocks in rare scenarios where 
        the signal would happen while the main thread is also logging. See jira
        ticket GOS-32828.

    \Version 03/24/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _SignalProcess(int32_t iSignal)
{
    // handle config reload request
    if (iSignal == SIGHUP)
    {
        // indicate config reload should take place
        _uSignalFlags |= VOIPSERVER_SIGNAL_CONFIG_RELOAD;
        // re-set config reload
        signal(SIGHUP, &_SignalProcess);
    }

    // handle immediate shutdown
    if ((iSignal == SIGTERM) || (iSignal == SIGINT))
    {
        _uSignalFlags |= VOIPSERVER_SIGNAL_SHUTDOWN_IMMEDIATE;
    }

    // handle "ifempty" shutdown
    #if defined(DIRTYCODE_LINUX)
    if (iSignal == SIGUSR1)
    {
        _uSignalFlags |= VOIPSERVER_SIGNAL_SHUTDOWN_IFEMPTY; 
    }
    #endif
}

#if defined(DIRTYCODE_PC)
/*F********************************************************************************/
/*!
    \Function _SetupFirewallRules

    \Description
        Setup the firewall rules for the voipserver on windows

    \Version 06/18/2018 (eesponda)
*/
/********************************************************************************F*/
static void _SetupFirewallRules(void)
{
    HRESULT hResult;
    INetFwPolicy2 *pFwPolicy2;
    INetFwRules *pFwRules;

    // RPC_E_CHANGED_MODE means that it has already been initialized in a different mode
    hResult = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
    if (FAILED(hResult) && (hResult != RPC_E_CHANGED_MODE))
    {
        DirtyCastLogPrintf("_SetupFirewallRules: unable to initialize com\n");
        CoUninitialize();
        return;
    }

    // create the policy
    if (FAILED(CoCreateInstance(&CLSID_NetFwPolicy2, NULL, CLSCTX_INPROC_SERVER, &IID_INetFwPolicy2, (void **)&pFwPolicy2)))
    {
        DirtyCastLogPrintf("_SetupFirewallRules: unable to create instance of NetFwPolicy2 needed to query our rules\n");
        CoUninitialize();
        return;
    }

    // check if we have the rule to know what to do next
    if (SUCCEEDED(INetFwPolicy2_get_Rules(pFwPolicy2, &pFwRules)))
    {
        char strApplicationName[128], strPath[64], strFirewallCommand[256];
        wchar_t wstrApplicationName[128];
        INetFwRule *pFwRule;
        int64_t iResult;

        // get the executable path and build our required strings, select a name that is specific to the location of the application
        GetModuleFileName(NULL, strPath, sizeof(strPath));
        ds_snzprintf(strApplicationName, sizeof(strApplicationName), "DirtyCast (%s)", strPath);
        MultiByteToWideChar(CP_UTF8, 0, strApplicationName, -1, wstrApplicationName, sizeof(wstrApplicationName));

        // add the rule if not found
        if (INetFwRules_Item(pFwRules, wstrApplicationName, &pFwRule) == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            DirtyCastLogPrintf("_SetupFilewallRules: adding firewall rules for the voipserver\n");
            ds_snzprintf(strFirewallCommand, sizeof(strFirewallCommand),
                "advfirewall firewall add rule name=\"%s\" description=\"Allows remote connections to DirtyCast\" dir=in action=allow program=\"%s\" profile=domain",
                strApplicationName, strPath);

            // execute the addition of the firewall rule using netsh with admin privileges (runas)
            if ((iResult = (int64_t)ShellExecute(NULL, "runas", "netsh", strFirewallCommand, NULL, SW_HIDE)) <= 32)
            {
                DirtyCastLogPrintf("_SetupFirewallRules: ShellExecute for netsh failed with %lld\n", iResult);
            }
        }
        else if (pFwRule != NULL)
        {
            // we don't need the rule for anything, just need to know it exists
            INetFwRule_Release(pFwRule);
        }

        // cleanup for the rules
        INetFwRules_Release(pFwRules);
    }
    else
    {
        DirtyCastLogPrintf("_SetupFirewallRules: unable to get firewall rules to determine if our rules has been enabled yet\n");
    }

    // cleanup the resources
    INetFwPolicy2_Release(pFwPolicy2);
    CoUninitialize();
}
#endif

/*F********************************************************************************/
/*!
    \Function _ServerProcess

    \Description
        Start up networking and run the server.

    \Input argc - number of command line arguments
    \Input argv - command line arguments
    \Input *pConfig - server config file
    
    \Output
        None.

    \Version 03/24/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ServerProcess(int32_t argc, const char *argv[], const char *pConfig)
{
    int32_t iReturnValue = DIRTYCAST_EXIT_OK;
    // start memory tracking
    MemtrackStartup();

    #if defined(DIRTYCODE_PC)
    // use high-performance timer under windows, to get millisecond-accurate timing
    _NetLib_bUseHighResTimer = TRUE;

    // setup our firewall rules
    _SetupFirewallRules();
    #endif

    // setup the netprintf debug hook
    #if DIRTYCODE_LOGGING
    NetPrintfHook(_VoipServerPrintfHook, NULL);
    #endif
    
    // disable time stamp for dirtycast (it will double stamp if not)
    NetTimeStampEnable(FALSE);

    // start DirtySock with no UPnP and in single-threaded mode
    NetConnStartup("-noupnp -singlethreaded -servicename=voipserver");

    // initialize the VoipTunnel server
    if ((_Server = VoipServerCreate(argc, argv, pConfig)) == NULL)
    {
        DirtyCastLogPrintf("_ServerProcess: failed to create voipserver module\n");
        iReturnValue = DIRTYCAST_EXIT_ERROR;
    }
    else
    {
        _uSignalFlags = 0;
        _uDebugLevel = VoipServerGetConfig(_Server)->uDebugLevel;

        // config reload request
        signal(SIGHUP, &_SignalProcess);

        // immediate server shutdown
        signal(SIGTERM, &_SignalProcess);
        signal(SIGINT, &_SignalProcess);

        #if defined(DIRTYCODE_LINUX)
        // graceful (when empty) server shutdown
        signal(SIGUSR1, &_SignalProcess);

        // ignore broken pipes
        signal(SIGPIPE, SIG_IGN);
        #endif

        // start the server running
        while (VoipServerUpdate(_Server, &_uSignalFlags))
            ;

        // shut down the server
        VoipServerDestroy(_Server);
    }

    // shut down DirtySock
    NetConnShutdown(0);

    // shut down memory tracking
    MemtrackShutdown();

    // disable per-allocation memory printing before shutting down logger
    _uDebugLevel = 0;

    // shut down logging
    DirtyCastLoggerDestroy();

    return(iReturnValue);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
     \Function main
 
     \Description
          Entry point into VoipTunnel server.
  
     \Input argc - number of command line arguments
     \Input argv - command line arguments
 
     \Output int32_t - 0 on success, else failure
 
     \Version 03/24/2006 (jbrookes)
*/
/********************************************************************************F*/
int main(int32_t argc, const char *argv[])
{
    const char *pConfig;
    int32_t iReturnValue = DIRTYCAST_EXIT_OK;

    // make sure we have a configfile
    if (argc < 2)
    {
        printf("usage: %s configfile [options]\n", argv[0]);
        printf("exit: %d\n", DIRTYCAST_EXIT_ERROR);
        exit(DIRTYCAST_EXIT_ERROR);
    }
    pConfig = argv[1];

    // first thing we do is start up the logger
    if (DirtyCastLoggerCreate(argc, argv) < 0)
    {
        printf("exit: %d\n", DIRTYCAST_EXIT_ERROR);
        exit(DIRTYCAST_EXIT_ERROR);
    }

    // display version info
    DirtyCastLogPrintf("===============================================\n");
    DirtyCastLogPrintf("VoipServer build: %s (%s)\n", _SDKVersion, DIRTYCAST_BUILDTYPE); 
    DirtyCastLogPrintf("BuildTime: %s\n", _ServerBuildTime);
    DirtyCastLogPrintf("BuildLocation: %s\n", _ServerBuildLocation);
    DirtyCastLogPrintf("===============================================\n");
    DirtyCastLogPrintf("main: pid=%d\n", DirtyCastGetPid());

    // run the server
    iReturnValue = _ServerProcess(argc, argv, pConfig);
    printf("main completed exit: %d\n", iReturnValue);
    return iReturnValue;
}

/*F********************************************************************************/
/*!
     \Function DirtyMemAlloc
 
     \Description
        Required memory allocation function for DirtySock client code.
  
     \Input iSize               - size of memory to allocate
     \Input iMemModule          - memory module
     \Input iMemGroup           - memory group
     \Input *pMemGroupUserData  - user data associated with memory group
 
     \Output
        void *                     - pointer to memory, or NULL
 
     \Version 03/24/2006 (jbrookes)
     \Version 11/25/2008 (mclouatre) Introducing support for mem group user data
*/
/********************************************************************************F*/
void *DirtyMemAlloc(int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    void *pMem;
    if ((pMem = (void *)malloc(iSize)) != NULL)
    {
        MemtrackAlloc(pMem, iSize, iMemModule);
    }
    if (_uDebugLevel > DIRTYCAST_DBGLVL_MEMORY)
    {
        DirtyMemDebugAlloc(pMem, iSize, iMemModule, iMemGroup, pMemGroupUserData);
    }
    return(pMem);
}

/*F********************************************************************************/
/*!
     \Function DirtyMemFree
 
     \Description
        Required memory free function for DirtySock client code.
  
     \Input *pMem               - pointer to memory to dispose of
     \Input iMemModule          - memory module
     \Input iMemGroup           - memory group
     \Input *pMemGroupUserData  - user data associated with memory group
 
     \Output
        None.
 
     \Version 03/24/2006 (jbrookes)
     \Version 11/25/2008 (mclouatre) Introducing support for mem group user data
*/
/********************************************************************************F*/
void DirtyMemFree(void *pMem, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    uint32_t uSize;
    MemtrackFree(pMem, &uSize);
    if (_uDebugLevel > DIRTYCAST_DBGLVL_MEMORY)
    {
        DirtyMemDebugFree(pMem, uSize, iMemModule, iMemGroup, pMemGroupUserData);
    }
    free(pMem);
}

