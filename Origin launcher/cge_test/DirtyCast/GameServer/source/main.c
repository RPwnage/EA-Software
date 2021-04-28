/*H********************************************************************************/
/*!
    \File main.c

    \Description
        Harness for the GameServer server.

    \Copyright
        Copyright (c) 2006 Electronic Arts Inc.

    \Version 04/03/2006 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtysock/netconn.h"

#if defined(DIRTYCODE_PC)
 #include "winmain.h"
#else
 #include <unistd.h>
#endif

#include "dirtycast.h"
#include "gameserver.h"
#include "gameserverconfig.h"
#include "memtrack.h"

/*** Defines **********************************************************************/

/*** Variables ********************************************************************/

#if defined(DIRTYCODE_PC)
extern uint32_t _NetLib_bUseHighResTimer;
#endif

// VoipTunnel server instance
static GameServerRefT  *_pServer = NULL;

static uint32_t     _uShutdownFlags;

static uint32_t     _uDebugLevel = DIRTYCAST_DBGLVL_MEMORY+1;  // start debug level high until we can read it in from the config file

/*** Private Functions ************************************************************/

/*F*************************************************************************************/
/*!
    \Function _GameServerPrintfHook

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
/**************************************************************************************F*/
#if DIRTYCODE_LOGGING
static int32_t _GameServerPrintfHook(void *pParm, const char *pText)
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
    
    \Output
        None.

    \Version 03/24/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _SignalProcess(int32_t iSignal)
{
    // handle graceful shutdown
    if ((iSignal == SIGTERM) || (iSignal == SIGINT))
    {
        DirtyCastLogPrintf("_SignalProcess: signal=%s (immediate)\n",
            (iSignal == SIGTERM) ? "SIGTERM" : "SIGINT");
        _uShutdownFlags |= GAMESERVER_SHUTDOWN_IMMEDIATE;
    }

    // handle "ifempty" shutdown
    #if defined(DIRTYCODE_LINUX)
    if (iSignal == SIGUSR1)
    {
        DirtyCastLogPrintf("_SignalProcess: signal=SIGUSR1 (ifempty)\n");
        _uShutdownFlags |= GAMESERVER_SHUTDOWN_IFEMPTY;
    }
    #endif
}

/*F********************************************************************************/
/*!
    \Function _ServerProcess

    \Description
        Start up networking and run the server.

    \Input argc - number of command line arguments
    \Input argv - command line arguments
    \Input *pConfig - server config file
    
    \Output
        Program return value.

    \Version 03/24/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ServerProcess(int32_t argc, const char *argv[], const char *pConfig)
{
    // start memory tracking
    int32_t iReturnValue = DIRTYCAST_EXIT_OK;
    MemtrackStartup();

    // use high-performance timer under windows, to get millisecond-accurate timing
    #if defined(DIRTYCODE_PC)
    _NetLib_bUseHighResTimer = TRUE;
    #endif

    // setup the netprintf debug hook
    #if DIRTYCODE_LOGGING
    NetPrintfHook(_GameServerPrintfHook, NULL);
    #endif

    // disable time stamp for dirtycast (it will double stamp if not)
    NetTimeStampEnable(FALSE);

    // start DirtySock with no UPnP and in single-threaded mode
    NetConnStartup("-noupnp -singlethreaded"); 

    // initialize the VoipTunnel server
    if ((_pServer = GameServerCreate(argc, argv, pConfig)) != NULL)
    {
        _uShutdownFlags = 0;
        _uDebugLevel = GameServerGetConfig(_pServer)->uDebugLevel;

        // set up to handle immediate server shutdown
        signal(SIGTERM, _SignalProcess);
        signal(SIGINT, _SignalProcess);

        #if defined(DIRTYCODE_LINUX)
        // set up to handle ifempty server shutdown
        signal(SIGUSR1, _SignalProcess);

        // ignore broken pipes
        signal(SIGPIPE, SIG_IGN);
        #endif

        // start the server running
        while (GameServerUpdate(_pServer, _uShutdownFlags))
            ;

        // shut down server
        GameServerDestroy(_pServer);
        _pServer = NULL;
    }
    else
    {
        DirtyCastLogPrintf("_ServerProcess: _Server.Initialize failed returning %d\n", DIRTYCAST_EXIT_ERROR);
        iReturnValue = DIRTYCAST_EXIT_ERROR;
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
    DirtyCastLogPrintf("GameServer build: %s (%s)\n", _SDKVersion, DIRTYCAST_BUILDTYPE); 
    DirtyCastLogPrintf("BuildTime: %s\n", _ServerBuildTime);
    DirtyCastLogPrintf("BuildLocation: %s\n", _ServerBuildLocation);
    DirtyCastLogPrintf("===============================================\n");
    DirtyCastLogPrintf("main: pid=%d\n", DirtyCastGetPid());

    // run the server
    iReturnValue = _ServerProcess(argc, argv, pConfig);
    printf("main completed exit: %d\n", iReturnValue);
    return(iReturnValue);
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
        void *                  - pointer to memory, or NULL
 
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
