/*H********************************************************************************/
/*!
    \File main.cpp

    \Description
        Harness for the Stress server.

    \Copyright
        Copyright (c) 2006 Electronic Arts Inc.

    \Version 04/03/2006 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/
#include <EABase/eabase.h>

#ifdef WIN32
 #include <windows.h>
#else
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
#include "EAStdC/EASprintf.h"

#include "dirtycast.h"
#include "stress.h"

#if defined(DIRTYCODE_PC)
extern "C" {
extern uint32_t _NetLib_bUseHighResTimer;
}
#endif

/*** Defines **********************************************************************/

#define MAIN_MEMDEBUGLEVEL (4)  // if debuglevel is less than this, per-allocation mem alloc/free info will not be printed

/*** Variables ********************************************************************/

// VoipTunnel server instance
static StressRefT  *_pServer = NULL;

static uint32_t     _uShutdownFlags;

static uint32_t     _uDebugLevel = MAIN_MEMDEBUGLEVEL;  // start debug level high until we can read it in from the config file

/*** Private Functions ************************************************************/


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
        _uShutdownFlags |= STRESS_SHUTDOWN_IMMEDIATE;
    }

    // handle "ifempty" shutdown
    #if defined(DIRTYCODE_LINUX)
    if (iSignal == SIGUSR1)
    {
        _uShutdownFlags |= STRESS_SHUTDOWN_IFEMPTY;
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

    // start DirtySock
    NetConnStartup("-noupnp -singlethreaded"); 

    // test some DirtySock stuff
    NetPrintf(("main: addr=%a\n", NetConnStatus('addr', 0, NULL, 0)));
    NetPrintf(("main: macx=%s\n", NetConnMAC()));

    // initialize the VoipTunnel server
    if ((_pServer = StressInitialize(argc, argv, pConfig)) != NULL)
    {
        _uShutdownFlags = 0;
        _uDebugLevel = StressStatus(_pServer, 'spam', 0, NULL, 0);

        // set up to handle immediate server shutdown
        signal(SIGTERM, _SignalProcess);
        signal(SIGINT, _SignalProcess);

        #if defined(DIRTYCODE_LINUX)
        // set up to handle ifempty server shutdown
        signal(SIGUSR1, _SignalProcess);

        //ignore broken pipes
        signal(SIGPIPE, SIG_IGN);
        #endif

        // start the server running
        while (StressProcess(_pServer, &_uShutdownFlags))
            ;

        // shut down server
        StressShutdown(_pServer);
        _pServer = NULL;
    }

    // shut down DirtySock
    NetConnShutdown(0);
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

//! memid for eastl
#define EASTL_MEMID            ('east')

/*F********************************************************************************/
/*!
    \Function operator new[] 

    \Description
    Created to link eastl calls DirtyMemAlloc


    \Input size         - size of memory to allocate
    \Input pName        - unused
    \Input flags        - unused
    \Input debugFlags   - unused
    \Input file         - unused
    \Input line         - unused

    \Output
    void *              - pointer to memory, or NULL
*/
/********************************************************************************F*/
void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    int32_t iMemGroup;
    void *pMemGroupUserData;
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);
    return DirtyMemAlloc((int32_t)size, EASTL_MEMID, iMemGroup, pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function operator new[] 

    \Description
    Created to link eastl calls DirtyMemAlloc


    \Input size             - size of memory to allocate
    \Input alignment        - unused
    \Input alignmentOffset  - unused
    \Input pName            - unused
    \Input flags            - unused
    \Input debugFlags       - unused
    \Input file             - unused
    \Input line             - unused

    \Output
    void *              - pointer to memory, or NULL
*/
/********************************************************************************F*/
void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    int32_t iMemGroup;
    void *pMemGroupUserData;
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);
    return DirtyMemAlloc((int32_t)size, EASTL_MEMID, iMemGroup, pMemGroupUserData);
}

//Vsnprintf8 for eastl
int Vsnprintf8 (char8_t*  pDestination, size_t n, const char8_t*  pFormat, va_list arguments); //necessary for stupid codewarrior warning.
int Vsnprintf8 (char8_t*  pDestination, size_t n, const char8_t*  pFormat, va_list arguments)
{
    return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments);
}

