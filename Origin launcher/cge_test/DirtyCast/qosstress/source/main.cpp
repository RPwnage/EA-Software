/*H********************************************************************************/
/*!
    \File main.cpp

    \Description
        Harness for the QosStress server.

    \Copyright
        Copyright (c) 2006 Electronic Arts Inc.

    \Version 04/03/2006 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <EABase/eabase.h>

#ifndef WIN32
 #include <unistd.h>
 #include <errno.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/dirtysock/netconn.h"

#include "qosstress.h"

#if defined(DIRTYCODE_PC)
extern "C" {
extern uint32_t _NetLib_bUseHighResTimer;
}
#endif

/*** Defines **********************************************************************/

#define MAIN_MEMDEBUGLEVEL (4)  // if debuglevel is less than this, per-allocation mem alloc/free info will not be printed

/*** Variables ********************************************************************/

static QosStressC  _Server;
static uint32_t     _uShutdownFlags;
static uint32_t     _uDebugLevel = MAIN_MEMDEBUGLEVEL;  // start debug level high until we can read it in from the config file

/*** Private Functions ************************************************************/


/*F*************************************************************************************/
/*!
    \Function _QosStressPrintfHook

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
#if DIRTYCODE_DEBUG
static int32_t _QosStressPrintfHook(void *pParm, const char *pText)
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
        _uShutdownFlags |= QOSSTRESS_SHUTDOWN_IMMEDIATE;
    }

    // handle "ifempty" shutdown
    #if defined(DIRTYCODE_LINUX)
    if (iSignal == SIGUSR1)
    {
        _uShutdownFlags |= QOSSTRESS_SHUTDOWN_IFEMPTY;
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
        None.

    \Version 03/24/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _ServerProcess(int argc, const char *argv[], const char *pConfig)
{
    // use high-performance timer under windows, to get millisecond-accurate timing
    #if defined(DIRTYCODE_PC)
    _NetLib_bUseHighResTimer = TRUE;
    #endif

    // set up NetPrintf() debug hook
    #if DIRTYCODE_DEBUG
    NetPrintfHook(_QosStressPrintfHook, NULL);
    #endif

    // initialize the VoipTunnel server
    if (_Server.Initialize(argc, argv, pConfig) != 0)
    {
        _uShutdownFlags = 0;
        _uDebugLevel = _Server.GetConfig().uDebugLevel;

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
        while (_Server.Process(_uShutdownFlags))
            ;

        // shut down server
        _Server.Shutdown();
    }
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

    if (argc < 2)
    {
        printf("usage: %s configfile [options]\n", argv[0]);
        exit(1);
    }
    pConfig = argv[1];

    fprintf(stdout, "main: pid=%d\n", DirtyCastGetPid());
    fflush(stdout);
    
    // run the server
    _ServerProcess(argc, argv, pConfig);

    // done
    return(0);
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
extern "C"
{
    void *DirtyMemAlloc(int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
    {
        void *pMem;

        pMem = (void *)malloc(iSize);

        if (_uDebugLevel >= MAIN_MEMDEBUGLEVEL)
        {
            DirtyMemDebugAlloc(pMem, iSize, iMemModule, iMemGroup, pMemGroupUserData);
        }
        return(pMem);
    }
} // extern "C"

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
extern "C"
{
    void DirtyMemFree(void *pMem, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
    {
        if (_uDebugLevel >= MAIN_MEMDEBUGLEVEL)
        {
            DirtyMemDebugFree(pMem, 0, iMemModule, iMemGroup, pMemGroupUserData);
        }
        free(pMem);
    }
} // extern "C"

