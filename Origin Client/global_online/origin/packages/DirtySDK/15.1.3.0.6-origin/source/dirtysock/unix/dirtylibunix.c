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
#include <time.h>
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

// global critical section
static NetCritT _NetLib_GlobalCrit;

// idle thread state
static volatile int32_t g_idlelife = -1;
static pthread_t g_idlethread = 0;
static uint8_t g_singlethreaded = 0;

#if DIRTYCODE_LOGGING
// static printf buffer to avoid using dynamic allocation
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

    // init common functionality
    NetLibCommonInit();

    // init idlelife tracker
    g_idlelife = -1;

    // single g_idlethread is also the "quit flag", set to non-zero before create to avoid bogus exit
    g_idlethread = (pthread_t)1;

    // create a worker thread
    if (!g_singlethreaded)
    {
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        #if defined(DIRTYCODE_LINUX)
        // set the thread's cpu affinity
        if (iThreadCpuAffinity != 0)
        {
            uint8_t uCpu;
            cpu_set_t CpuSetMask;
            CPU_ZERO(&CpuSetMask);

            for (uCpu = 0; uCpu < (sizeof(iThreadCpuAffinity) * 8); uCpu += 1)
            {
                if ((iThreadCpuAffinity >> uCpu) & 1)
                {
                    CPU_SET(uCpu, &CpuSetMask);
                }
            }

            if ((iResult = pthread_attr_setaffinity_np(&attr, sizeof(CpuSetMask), &CpuSetMask)) != 0)
            {
                NetPrintf(("dirtylibunix: failed to set idle thread's cpu affinity (err=%d)\n", iResult));
            }
        }
        #endif

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
    #endif

    #ifdef NEED_STATIC_ASSERT
        #define _Static_assert(expr, msg) typedef char DIRTY_CONCATENATE_HELPER(_NetCrit_too_small,__LINE__) [((expr) != 0) ? 1 : -1]
    #endif

    // validate NetCritT size
    _Static_assert(sizeof(NetCritT) >= sizeof(NetCritPrivT), "NetCritT is too small!");
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

    // shut down common functionality
    NetLibCommonShutdown();
}

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
    \Function NetTickUsec

    \Description
        Return increasing tick count in microseconds.  Used for performance timing
        purposes.

    \Output
        uint64_t    - microsecond tick count

    \Version 01/30/2015 (jbrookes)
*/
/********************************************************************************F*/
uint64_t NetTickUsec(void)
{
    struct timeval now;
    gettimeofday(&now, 0);
    return(((uint64_t)now.tv_sec*1000000) + (uint64_t)now.tv_usec);
}

/*F********************************************************************************/
/*!
    \Function NetLocalTime

    \Description
        This converts the input GMT time to the local time as specified by the
        system clock.  This function follows the re-entrant localtime_r function
        signature.

    \Input *pTm         - storage for localtime output
    \Input uElap        - GMT time

    \Output
        struct tm *     - pointer to localtime result

    \Version 04/23/2008 (jbrookes)
*/
/********************************************************************************F*/
struct tm *NetLocalTime(struct tm *pTm, uint64_t uElap)
{
    time_t uTimeT = (time_t)uElap;
    return(localtime_r(&uTimeT, pTm));
}

/*F********************************************************************************/
/*!
    \Function NetPlattimeToTime

    \Description
        This converts the input platform-specific time data structure to the
        generic time data structure.

    \Input *pTm         - generic time data structure to be filled by the function
    \Input *pPlatTime   - pointer to the platform-specific data structure

    \Output
        struct tm *     - NULL=failure; else pointer to user-provided generic time data structure

    \Notes
        pPlatTime is expected to point to a timespec on Unix platforms

    \Version 05/08/2010 (mclouatre)
*/
/********************************************************************************F*/
struct tm *NetPlattimeToTime(struct tm *pTm, void *pPlatTime)
{
    #if defined(DIRTYCODE_LINUX) || defined(DIRTYCODE_ANDROID)
    struct timespec timeSpec = *(struct timespec *)pPlatTime;
    localtime_r(&timeSpec.tv_sec, pTm);
    #elif defined(DIRTYCODE_APPLEIOS) || defined(DIRTYCODE_APPLEOSX)
    struct timeval timeVal = *(struct timeval *)pPlatTime;
    localtime_r(&timeVal.tv_sec, pTm);
    #else
    pTm = NULL;
    #endif

    return(pTm);
}

/*F********************************************************************************/
/*!
    \Function NetPlattimeToTimeMs
 
    \Description
        This function retrieves the current date time and fills in the
        generic time data structure prrovided. It has the option of returning millisecond
        which is not part of the generic time data structure

    \Input *pTm         - generic time data structure to be filled by the function
    \Input *pImSec      - output param for milisecond to be filled by the function (optional can be NULL)

    \Output
        struct tm *     - NULL=failure; else pointer to user-provided generic time data structure

    \Version 09/16/2014 (tcho)
*/
/********************************************************************************F*/
struct tm *NetPlattimeToTimeMs(struct tm *pTm , int32_t *pImSec)
{
    void *pPlatTime;
    int32_t iMsec;
    
    #if defined(DIRTYCODE_LINUX) || defined(DIRTYCODE_ANDROID)
    struct timespec timeSpec;
    clock_gettime(CLOCK_REALTIME, &timeSpec);
    iMsec = timeSpec.tv_nsec/1000000;
    pPlatTime = (void *)&timeSpec;
    #elif defined(DIRTYCODE_APPLEIOS) || defined(DIRTYCODE_APPLEOSX)
    struct timeval timeVal;
    gettimeofday(&timeVal, NULL);

    iMsec = timeVal.tv_usec/1000;
    pPlatTime = (void*)&timeVal;
    #else
    return (NULL);
    #endif

    if (pImSec != NULL)
    {
        *pImSec = iMsec;
    }
   
    if (pTm == NULL)
    {
        return(NULL);
    }

    return(NetPlattimeToTime(pTm, pPlatTime));
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
    ds_memclr(pPriv, sizeof(*pPriv));
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
