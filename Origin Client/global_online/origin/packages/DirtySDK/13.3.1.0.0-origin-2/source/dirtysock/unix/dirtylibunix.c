/*H********************************************************************************/
/*!
    \File dirtylibunix.c

    \Description
        Platform-specific support library for network code.  Supplies simple time,
        debug printing, and mutex functions.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 10/04/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtyerr.h"

/*** Defines **********************************************************************/


/*** Type Definitions *************************************************************/

//! critical section type
typedef struct NetCritPrivT
{
    pthread_t iThreadId;        //!< thread id
    pthread_mutex_t Mutex;      //!< unix mutex
    int32_t iAccessCt;          //!< access count (zero if available)
    const char *pName;          //!< critical section name
    uint32_t _pad;
} NetCritPrivT;

/*** Variables ********************************************************************/

// idle critical section
static NetCritT _NetLib_IdleCrit;
NetCritT *_NetLib_pIdleCrit = &_NetLib_IdleCrit;

// global critical section
static NetCritT _NetLib_GlobalCrit;

// idle thread state
static volatile int32_t g_idlelife = -1;
static pthread_t g_idlethread = 0;
static uint8_t g_singlethreaded = 0;

// debug printf hook
#if DIRTYCODE_LOGGING
static void *_NetLib_pDebugParm = NULL;
static int32_t (*_NetLib_pDebugHook)(void *pParm, const char *pText) = NULL;
static char _NetLib_aPrintfMem[4096];
#endif

// $$tmp -- for compatibility with old dirty*unix
void NetTickSet(void);
uint32_t _NetLib_bCritEnabled = 1;
void NetTickSet(void)
{
}
// $$tmp -- for compatibility with old dirty*unix


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _NetLibThread

    \Description
        Thread to handle special library tasks.

    \Input *pArg    - pointer to argument

    \Output
        None.

    \Version 06/21/2005 (jbrookes)
*/
/********************************************************************************F*/
static void *_NetLibThread(void *pArg)
{
    NetPrintf(("dirtylibunix: idle thread running (thid=%d)\n", g_idlethread));

    // show we are running
    g_idlelife = 1;

    // run while we have sema
    while (g_idlethread != 0)
    {
        // call idle functions
        NetIdleCall();

        // wait for next tick
        usleep(50*1000);
    }

    // show we are dead
    g_idlelife = 0;

    // report termination
    NetPrintf(("dirtylibunix: idle thread exiting\n"));
    return(0);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function NetLibCreate

    \Description
        Initialize the network library module.

    \Input iThreadPrio        - priority to start the _NetLibThread with
    \Input iThreadStackSize   - stack size to start the _NetLibThread with (in bytes)
    \Input iThreadCpuAffinity - cpu affinity to start the _NetLibThread with

    \Version 06/21/2006 (jbrookes)
*/
/********************************************************************************F*/
void NetLibCreate(int32_t iThreadPrio, int32_t iThreadStackSize, int32_t iThreadCpuAffinity)
{
    pthread_attr_t attr;
    int32_t iResult;

    if (iThreadPrio < 0)
    {
        NetPrintf(("dirtylibunix: starting in single-threaded mode\n"));
        g_singlethreaded = TRUE;
    }

    // reset the idle list
    NetIdleReset();
    g_idlelife = -1;

    // initialize critical sections
    NetCritInit(NULL, "lib-global");
    NetCritInit(&_NetLib_IdleCrit, "lib-idle");

    // single g_idlethread is also the "quit flag", set to non-zero before create to avoid bogus exit
    g_idlethread = (pthread_t)1;

    // create a worker thread
    if (!g_singlethreaded)
    {
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if ((iResult = pthread_create(&g_idlethread, &attr, _NetLibThread, NULL)) != 0)
        {
            NetPrintf(("dirtylibunix: unable to create idle thread (err=%s)\n", DirtyErrGetName(iResult)));
        }

        // wait for thread startup
        while (g_idlelife == -1)
        {
            usleep(100);
        }
    }

     #if DIRTYCODE_LOGGING
    // set explicit printf buffer to avoid dynamic malloc() use
    setvbuf(stdout, _NetLib_aPrintfMem, _IOLBF, sizeof(_NetLib_aPrintfMem));
    // validate NetCritT size
    if (sizeof(NetCritT) < sizeof(NetCritPrivT))
    {
        NetPrintf(("\ndirtylibunix: warning, NetCritT is too small! (%d/%d)\n\n", sizeof(NetCritT), sizeof(NetCritPrivT)));
    }
    #endif
}

/*F********************************************************************************/
/*!
    \Function NetLibDestroy

    \Description
        Destroy the network library module.

    \Input uShutdownFlags   - NET_SHUTDOWN_* flags

    \Version 06/21/2006 (jbrookes)
*/
/********************************************************************************F*/
void NetLibDestroy(uint32_t uShutdownFlags)
{
    // signal a shutdown
    if (!g_singlethreaded)
    {
        g_idlethread = 0;

        // wait for thread to terminate
        while (g_idlelife > 0)
        {
            usleep(1);
        }
    }

    // shutdown global critical section
    NetCritKill(NULL);
    NetCritKill(&_NetLib_IdleCrit);
}

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function NetPrintfCode

    \Description
        A printf function that works with both dev/fw-test systems.

    \Input *pFormat - pointer to format string
    \Input ...      - variable argument list

    \Output
        None.

    \Version 06/21/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetPrintfCode(const char *pFormat, ...)
{
    va_list pFmtArgs;
    char strText[4096];
    const char *pText = strText;
    int32_t iOutput=1;

    va_start(pFmtArgs, pFormat);
    // check for simple formatting
    if ((pFormat[0] == '%') && (pFormat[1] == 's') && (pFormat[2] == 0))
    {
        pText = va_arg(pFmtArgs, const char *);
    }
    else
    {
        ds_vsnprintf(strText, sizeof(strText), pFormat, pFmtArgs);
    }
    va_end(pFmtArgs);

    // send to debug hook if set
    if (_NetLib_pDebugHook != NULL)
    {
        iOutput = _NetLib_pDebugHook(_NetLib_pDebugParm, pText);
    }

    // if debug hook didn't override output, print here
    if (iOutput != 0)
    {
        printf("%s", pText);
    }
    return(0);
}
#endif

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function NetPrintfHook

    \Description
        Hook into debug output.

    \Input *pPrintfDebugHook    - pointer to function to call with debug output
    \Input *pParm               - user parameter

    \Output
        None.

    \Version 06/21/2006 (jbrookes)
*/
/********************************************************************************F*/
void NetPrintfHook(int32_t (*pPrintfDebugHook)(void *pParm, const char *pText), void *pParm)
{
    _NetLib_pDebugHook = pPrintfDebugHook;
    _NetLib_pDebugParm = pParm;
}
#endif

/*F********************************************************************************/
/*!
    \Function NetTick

    \Description
        Return some kind of increasing tick count with millisecond scale (does
        not need to have millisecond precision, but higher precision is better).

    \Output
        uint32_t    - millisecond tick count

    \Version 06/21/2006 (jbrookes)
*/
/********************************************************************************F*/
uint32_t NetTick(void)
{
    uint32_t uCurTick;
    struct timeval now;
    gettimeofday(&now, 0);
    uCurTick = (uint32_t)(((uint64_t)now.tv_sec*1000) + (uint64_t)now.tv_usec/1000);
    return(uCurTick);
}

/*F********************************************************************************/
/*!
    \Function NetCritInit

    \Description
        Initialize a critical section for use. Call must have persistant storage
        defined for lifetime of critical section.

    \Input *pCrit       - critical section marker
    \Input *pCritName   - unused

    \Output
        None

    \Version 06/21/2006 (jbrookes)
*/
/********************************************************************************F*/
void NetCritInit(NetCritT *pCrit, const char *pCritName)
{
    NetCritPrivT *pPriv = (NetCritPrivT *)(pCrit ? pCrit : &_NetLib_GlobalCrit);
    pthread_mutexattr_t attr;
    int32_t iResult;

    // initialize the critical section
    memset(pPriv, 0, sizeof(*pPriv));
    pPriv->pName = (pCritName != NULL) ? pCritName : "unknown";

    // create mutex
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if ((iResult = pthread_mutex_init(&pPriv->Mutex, &attr)) != 0)
    {
        NetPrintf(("dirtylibunix: unable to create mutex %s (err=%s)\n", pPriv->pName, DirtyErrGetName(iResult)));
    }
}

/*F********************************************************************************/
/*!
    \Function NetCritKill

    \Description
        Release resources and destroy critical section.

    \Input *pCrit   - critical section marker

    \Output
        None

    \Version 06/21/2006 (jbrookes)
*/
/********************************************************************************F*/
void NetCritKill(NetCritT *pCrit)
{
    NetCritPrivT *pPriv = (NetCritPrivT *)(pCrit ? pCrit : &_NetLib_GlobalCrit);
    int32_t iResult;

    // kill the critical section
    if ((iResult = pthread_mutex_destroy(&pPriv->Mutex)) != 0)
    {
        NetPrintf(("dirtylibunix: unable to delete mutex (sema=%p (%s), err=%s)\n", &pPriv->Mutex, pPriv->pName, DirtyErrGetName(iResult)));
    }
}

/*F********************************************************************************/
/*!
    \Function NetCritTry

    \Description
        Attempt to gain access to critical section. Always returns immediately
        regardless of access status. A thread that already has access to a critical
        section can always receive repeated access to it.

    \Input *pCrit   - critical section marker

    \Output
        int32_t         - zero=unable to get access, non-zero=access granted

    \Version 06/21/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetCritTry(NetCritT *pCrit)
{
    NetCritPrivT *pPriv = (NetCritPrivT *)(pCrit ? pCrit : &_NetLib_GlobalCrit);
    int32_t iResult;

    // in single-threaded mode, skip mutex
    if (g_singlethreaded)
    {
        return(1);
    }

    // try and gain access
    if ((iResult = pthread_mutex_trylock(&pPriv->Mutex)) != 0)
    {
        if (iResult != EBUSY)
        {
            NetPrintf(("dirtylibunix: error trying mutex (sema=%p (%s), err=%s)\n", &pPriv->Mutex, pPriv->pName, DirtyErrGetName(iResult)));
        }
        return(0);
    }
    return(1);
}

/*F********************************************************************************/
/*!
    \Function NetCritEnter

    \Description
        Enter a critical section, blocking if needed.

    \Input *pCrit   - critical section marker

    \Output
        None.

    \Version 06/21/2006 (jbrookes)
*/
/********************************************************************************F*/
void NetCritEnter(NetCritT *pCrit)
{
    NetCritPrivT *pPriv = (NetCritPrivT *)(pCrit ? pCrit : &_NetLib_GlobalCrit);
    int32_t iResult;

    // in single-threaded mode, skip mutex
    if (g_singlethreaded)
    {
        return;
    }

    // wait for mutex to become free for up to five seconds
    if ((iResult = pthread_mutex_lock(&pPriv->Mutex)) != 0)
    {
        NetPrintf(("dirtylibunix: error waiting on mutex (sema=%p (%s), err=%s)\n", &pPriv->Mutex, pPriv->pName, DirtyErrGetName(iResult)));
    }
}

/*F********************************************************************************/
/*!
    \Function NetCritLevae

    \Description
        Leave a critical section.  Must be called once for every NetCritEnter() or
        successful NetCritTry().

    \Input *pCrit   - critical section marker

    \Output
        None.

    \Version 06/21/2006 (jbrookes)
*/
/********************************************************************************F*/
void NetCritLeave(NetCritT *pCrit)
{
    NetCritPrivT *pPriv = (NetCritPrivT *)(pCrit ? pCrit : &_NetLib_GlobalCrit);
    int32_t iResult;

    // in single-threaded mode, skip mutex
    if (g_singlethreaded)
    {
        return;
    }

    // release the critical section (must happen after lock)
    if ((iResult = pthread_mutex_unlock(&pPriv->Mutex)) != 0)
    {
        NetPrintf(("dirtylibunix: error releasing mutex (sema=%p (%s), err=%s)\n", &pPriv->Mutex, pPriv->pName, DirtyErrGetName(iResult)));
    }
}
