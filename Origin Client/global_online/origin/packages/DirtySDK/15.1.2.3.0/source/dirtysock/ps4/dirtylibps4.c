/*H********************************************************************************/
/*!
    \File dirtylibps4.c

    \Description
        Platform-specific support library for network code.  Supplies simple time,
        debug printing, and mutex functions.

    \Copyright
        Copyright (c) 2013 Electronic Arts Inc.

    \Version 01/17/2013 (amakoukji) First version; a vanilla port to ps4 from unix
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>

#include <kernel.h>
#include <net.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtyerr.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

//! critical section type
typedef struct NetCritPrivT
{
    ScePthread iThreadId;       //!< thread id
    ScePthreadMutex Mutex;      //!< unix mutex
    int32_t iAccessCt;          //!< access count (zero if available)
    const char *pName;          //!< critical section name
    uint32_t _pad;
} NetCritPrivT;

/*** Variables ********************************************************************/

// global critical section
static NetCritT _NetLib_GlobalCrit;

// idle thread state
static volatile int32_t g_idlelife = -1;
static ScePthread g_idlethread = 0;
static uint8_t g_singlethreaded = 0;

// Time Stamp Conuter Freuency
static uint64_t _NetLib_uTimeStampFreq = 0;

// static printf buffer to avoid using dynamic allocation
#if DIRTYCODE_LOGGING
static char _NetLib_aPrintfMem[4096];
#endif


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _NetTickCount

    \Description
        Return millisecond/microsecond tick count, scale based on input.

    \Input
        uScale      - output scale (1000 for ms, 1000*1000 for us)

    \Output
        uint64_t    - tick count

    \Version 12/18/2013 (tcho) rdtsc version
*/
/********************************************************************************F*/
static uint64_t _NetTickCount(uint32_t uScale)
{
    uint32_t eaxLow32, edxHigh32;
    uint64_t uCurUsec;

    if (_NetLib_uTimeStampFreq == 0)
    {
        _NetLib_uTimeStampFreq = sceKernelGetTscFrequency();
    }

    // read rdtsc register
    asm volatile("rdtsc" : "=a" (eaxLow32), "=d" (edxHigh32));

    // convert timestamp to microseconds/milliseconds based on output scale
    uCurUsec = ((uint64_t)edxHigh32 << 32) | ((uint64_t)eaxLow32);
    return((uCurUsec*uScale)/_NetLib_uTimeStampFreq);
}

/*F********************************************************************************/
/*!
    \Function _NetLibThread

    \Description
        Thread to handle special library tasks.

    \Input *pArg    - pointer to argument

    \Version 01/23/2013 (amakoukji)
*/
/********************************************************************************F*/
static void *_NetLibThread(void *pArg)
{
    NetPrintf(("dirtylibps4: idle thread running (thid=%d)\n", g_idlethread));

    // show we are running
    g_idlelife = 1;

    // run while we have sema
    while (g_idlethread != 0)
    {
        // call idle functions
        NetIdleCall();

        // wait for next tick
        sceKernelUsleep(50*1000);
    }

    // show we are dead
    g_idlelife = 0;

    // report termination
    NetPrintf(("dirtylibps4: idle thread exiting\n"));
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
    \Input iThreadCpuAffinity - cpu affinity to start the _NetLibThread with, 0=SCE_KERNEL_CPUMASK_USER_ALL otherwise a mask of cores to use

    \Version 01/23/2013 (amakoukji)
*/
/********************************************************************************F*/
void NetLibCreate(int32_t iThreadPrio, int32_t iThreadStackSize, int32_t iThreadCpuAffinity)
{
    ScePthreadAttr attr;
    int32_t iResult;

    if (iThreadPrio < 0)
    {
        NetPrintf(("dirtylibps4: starting in single-threaded mode\n"));
        g_singlethreaded = TRUE;
    }

    // init common functionality
    NetLibCommonInit();

    // init idlelife tracker
    g_idlelife = -1;

    // single g_idlethread is also the "quit flag", set to non-zero before create to avoid bogus exit
    g_idlethread = (ScePthread)1;

    // create a worker thread
    if (!g_singlethreaded)
    {
        if ((iResult = scePthreadAttrInit(&attr)) == SCE_OK)
        {
            if ((iResult = scePthreadAttrSetdetachstate(&attr, SCE_PTHREAD_CREATE_DETACHED)) != SCE_OK)
            {
                NetPrintf(("dirtylibps4: scePthreadAttrSetdetachstate failed (err=%s)\n", DirtyErrGetName(iResult)));
            }

            if (iThreadCpuAffinity == 0)
            {
                if ((iResult = scePthreadAttrSetaffinity(&attr, SCE_KERNEL_CPUMASK_USER_ALL)) != SCE_OK)
                {
                    NetPrintf(("dirtylibps4: scePthreadAttrSetaffinity SCE_KERNEL_CPUMASK_USER_ALL failed (err=%s)\n", DirtyErrGetName(iResult)));
                }
            }
            else
            {
                if ((iResult = scePthreadAttrSetaffinity(&attr, iThreadCpuAffinity)) != SCE_OK)
                {
                    NetPrintf(("dirtylibps4: scePthreadAttrSetaffinity %x failed (err=%s)\n", iThreadCpuAffinity, DirtyErrGetName(iResult)));
                }
            }

            if ((iResult = scePthreadCreate(&g_idlethread, &attr, _NetLibThread, NULL, "NetLib Idle")) != SCE_OK)
            {
                NetPrintf(("dirtylibps4: scePthreadCreate failed (err=%s)\n", DirtyErrGetName(iResult)));
            }

            if ((iResult = scePthreadAttrDestroy(&attr)) != SCE_OK)
            {
                NetPrintf(("dirtylibps4: scePthreadAttrDestroy failed (err=%s)\n", DirtyErrGetName(iResult)));
            }
        }
        else
        {
            NetPrintf(("dirtylibps4: scePthreadAttrInit failed (err=%s)\n", DirtyErrGetName(iResult)));
        }

        // wait for thread startup
        while (g_idlelife == -1)
        {
            sceKernelUsleep(100);
        }
    }

     #if DIRTYCODE_LOGGING
    // set explicit printf buffer to avoid dynamic malloc() use
    setvbuf(stdout, _NetLib_aPrintfMem, _IOLBF, sizeof(_NetLib_aPrintfMem));
    // validate NetCritT size
    if (sizeof(NetCritT) < sizeof(NetCritPrivT))
    {
        NetPrintf(("\ndirtylibps4: warning, NetCritT is too small! (%d/%d)\n\n", sizeof(NetCritT), sizeof(NetCritPrivT)));
    }
    #endif
}

/*F********************************************************************************/
/*!
    \Function NetLibDestroy

    \Description
        Destroy the network library module.

    \Input uShutdownFlags   - NET_SHUTDOWN_* flags

    \Version 01/23/2013 (amakoukji)
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
            sceKernelUsleep(1);
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

    \Version 12/18/2013 (tcho) rdtsc version
*/
/********************************************************************************F*/
uint32_t NetTick(void)
{
    return((uint32_t)(_NetTickCount(1000)));
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
    return(_NetTickCount(1000000));
}

/*F********************************************************************************/
/*!
    \Function NetCritInit

    \Description
        Initialize a critical section for use. Call must have persistant storage
        defined for lifetime of critical section.

    \Input *pCrit       - critical section marker
    \Input *pCritName   - unused

    \Version 01/23/2013 (amakoukji)
*/
/********************************************************************************F*/
void NetCritInit(NetCritT *pCrit, const char *pCritName)
{
    NetCritPrivT *pPriv = (NetCritPrivT *)(pCrit ? pCrit : &_NetLib_GlobalCrit);
    ScePthreadMutexattr attr;
    int32_t iResult;

    // initialize the critical section
    memset(pPriv, 0, sizeof(*pPriv));
    pPriv->pName = (pCritName != NULL) ? pCritName : "unknown";

    // create mutex
    scePthreadMutexattrInit(&attr);
    scePthreadMutexattrSettype(&attr, SCE_PTHREAD_MUTEX_RECURSIVE);
    if ((iResult = scePthreadMutexInit(&pPriv->Mutex, &attr, pCritName)) != SCE_OK)
    {
        NetPrintf(("dirtylibps4: unable to create mutex %s (err=%s)\n", pPriv->pName, DirtyErrGetName(iResult)));
    }
    scePthreadMutexattrDestroy(&attr);
}

/*F********************************************************************************/
/*!
    \Function NetCritKill

    \Description
        Release resources and destroy critical section.

    \Input *pCrit   - critical section marker

    \Version 01/23/2013 (amakoukji)
*/
/********************************************************************************F*/
void NetCritKill(NetCritT *pCrit)
{
    NetCritPrivT *pPriv = (NetCritPrivT *)(pCrit ? pCrit : &_NetLib_GlobalCrit);
    int32_t iResult;

    // kill the critical section
    if ((iResult = scePthreadMutexDestroy(&pPriv->Mutex)) != SCE_OK)
    {
        NetPrintf(("dirtylibps4: unable to delete mutex (sema=%p (%s), err=%s)\n", &pPriv->Mutex, pPriv->pName, DirtyErrGetName(iResult)));
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

    \Version 01/23/2013 (amakoukji)
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
    if ((iResult = scePthreadMutexTrylock(&pPriv->Mutex)) != SCE_OK)
    {
        if (iResult != SCE_KERNEL_ERROR_EBUSY)
        {
            NetPrintf(("dirtylibps4: error trying mutex (sema=%p (%s), err=%s)\n", &pPriv->Mutex, pPriv->pName, DirtyErrGetName(iResult)));
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

    \Version 01/23/2013 (amakoukji)
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
    if ((iResult = scePthreadMutexLock(&pPriv->Mutex)) != SCE_OK)
    {
        NetPrintf(("dirtylibps4: error waiting on mutex (sema=%p (%s), err=%s)\n", &pPriv->Mutex, pPriv->pName, DirtyErrGetName(iResult)));
    }
}

/*F********************************************************************************/
/*!
    \Function NetCritLeave

    \Description
        Leave a critical section.  Must be called once for every NetCritEnter() or
        successful NetCritTry().

    \Input *pCrit   - critical section marker

    \Version 01/23/2013 (amakoukji)
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
    if ((iResult = scePthreadMutexUnlock(&pPriv->Mutex)) != 0)
    {
        NetPrintf(("dirtylibps4: error releasing mutex (sema=%p (%s), err=%s)\n", &pPriv->Mutex, pPriv->pName, DirtyErrGetName(iResult)));
    }
}
