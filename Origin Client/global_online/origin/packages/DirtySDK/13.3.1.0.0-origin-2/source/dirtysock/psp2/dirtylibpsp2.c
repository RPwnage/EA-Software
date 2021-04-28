/*H********************************************************************************/
/*!
    \File dirtylibpsp2.c

    \Description
        Platform-specific support library for network code.  Supplies simple time,
        debug printing, and mutex functions.

    \Copyright
        Copyright (c) 2010 Electronic Arts Inc.

    \Version 11/02/2010 (jbrookes) Ported from PS3
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>

#include <sdk_version.h>
#include <sceerror.h>
#include <kernel.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtyerr.h"

/*** Defines **********************************************************************/

#define NETCRIT_LOCK_COUNT  (4)

/*** Type Definitions *************************************************************/

//! critical section type
typedef struct NetCritPrivT
{
    SceKernelLwMutexWork     SceMutex;          //!< Sce LW mutex work area
    SceKernelLwMutexOptParam SceMutexOptParam;  //!< Sce LW mutex opt param
    SceUID uMutexId;                            //!< sce mutex id
    
    const char *pName;          //!< critical section name
} NetCritPrivT;

/*** Variables ********************************************************************/

// idle critical section
static NetCritT _NetLib_IdleCrit;
NetCritT *_NetLib_pIdleCrit = &_NetLib_IdleCrit;

// global critical section
static NetCritT _NetLib_GlobalCrit;

// idle thread state
static volatile int32_t g_idlelife = -1;
static SceUID g_idlethread = 0;

// global timer (required for NetTick implementation)
static SceUID _NetLib_Timer = 0;

// debug printf hook
#if DIRTYCODE_LOGGING
static void *_NetLib_pDebugParm = NULL;
static int32_t (*_NetLib_pDebugHook)(void *pParm, const char *pText) = NULL;
static char _NetLib_aPrintfMem[4096];
#endif

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _NetLibThread

    \Description
        Thread to handle special library tasks.

    \Input iArgCt   - size of argument
    \Input *pArgc   - pointer to argument
    
    \Output
        None.

    \Version 06/21/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetLibThread(SceSize iArgCt, void *pArgc)
{
    NetPrintf(("dirtylibpsp2: idle thread running (thid=%d)\n", g_idlethread));

    // show we are running
    g_idlelife = 1;

    // run while we have sema
    while (g_idlethread != 0)
    {
        // call idle functions
        NetIdleCall();

        // wait for next tick
        sceKernelDelayThread(50*1000);
    }

    // show we are dead
    g_idlelife = 0;

    // report termination
    NetPrintf(("dirtylibpsp2: idle thread exiting\n"));

    // terminate ourselves
    sceKernelExitDeleteThread(0);
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
    int32_t iResult;

    // reset the idle list
    NetIdleReset();
    g_idlelife = -1;

    // initialize critical sections
    NetCritInit(NULL, "lib-global");
    NetCritInit(&_NetLib_IdleCrit, "lib-idle");

    // create global timer
    if ((_NetLib_Timer = sceKernelCreateTimer("_NetLib_Timer", 0, NULL)) > 0)
    {
        if ((iResult = sceKernelStartTimer(_NetLib_Timer)) < 0)
        {
            NetPrintf(("dirtylibpsp2: unable to start netlib timer (err=%s)\n", DirtyErrGetName(iResult)));
        }
    }
    else
    {
        NetPrintf(("dirtylibpsp2: unable to create netlib timer (err=%s)\n", DirtyErrGetName(_NetLib_Timer)));
    }

    // single g_idlethread is also the "quit flag", set to non-zero before create to avoid bogus exit
    g_idlethread = 1;

    // create a worker thread
    // note: thread within priority range 64 to 127 cannot be created with SCE_KERNEL_CPU_MASK_USER_ALL (see psp2 doc - Kernel-Overview_e.pdf)
    if ((g_idlethread = sceKernelCreateThread("NetLib Idle", _NetLibThread, SCE_KERNEL_HIGHEST_PRIORITY_USER, (iThreadStackSize?iThreadStackSize:(16*1024)), 0, iThreadCpuAffinity, NULL)) < 0)
    {
        NetPrintf(("dirtylibpsp2: unable to create idle thread (err=%s)\n", DirtyErrGetName(g_idlethread)));
    }
    if ((iResult = sceKernelStartThread(g_idlethread, 0, NULL)) < 0)
    {
        NetPrintf(("dirtylibpsp2: unable to start idle thread (err=%s)\n", DirtyErrGetName(iResult)));
    }

    #if DIRTYCODE_LOGGING
    // set explicit printf buffer to avoid dynamic malloc() use
    setvbuf(stdout, _NetLib_aPrintfMem, _IOLBF, sizeof(_NetLib_aPrintfMem));
    // validate NetCritT size    
    if (sizeof(NetCritT) < sizeof(NetCritPrivT))
    {
        NetPrintf(("\ndirtylibpsp2: warning, NetCritT is too small!\n\n"));
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
    int32_t iResult;

    // signal a shutdown
    g_idlethread = 0;

    // wait for thread to terminate
    while (g_idlelife > 0)
    {
        sceKernelDelayThread(10);
    }

    // destroy global timer
    if ((iResult = sceKernelStopTimer(_NetLib_Timer)) < 0)
    {
        NetPrintf(("dirtylibpsp2: unable to stop netlib timer (err=%s)\n", DirtyErrGetName(iResult)));
    }
    if ((iResult = sceKernelDeleteTimer(_NetLib_Timer)) < 0)
    {
        NetPrintf(("dirtylibpsp2: unable to delete netlib timer (err=%s)\n", DirtyErrGetName(iResult)));
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
    uint64_t uTick = sceKernelGetTimerTimeWide(_NetLib_Timer)/1000;
    return((uint32_t)uTick);
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
    SceUID iResult;

    // initialize the critical section
    memset(pPriv, 0, sizeof(*pPriv));
    pPriv->pName = (pCritName != NULL) ? pCritName : "netlib";

    // create mutex
    if ((iResult = sceKernelCreateLwMutex(&pPriv->SceMutex, pPriv->pName, 
                                          SCE_KERNEL_LW_MUTEX_ATTR_TH_FIFO|SCE_KERNEL_LW_MUTEX_ATTR_RECURSIVE, 
                                          0, &pPriv->SceMutexOptParam)) < 0)
    {
        NetPrintf(("dirtylibpsp2: unable to create mutex %s (err=%s)\n", pPriv->pName, DirtyErrGetName(iResult)));
        pPriv->uMutexId = -1;
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
    if ((iResult = sceKernelDeleteLwMutex(&pPriv->SceMutex)) < 0)
    {
        NetPrintf(("dirtylibpsp2: unable to delete mutex (sema=0x%08x (%s), err=%s)\n", pPriv->uMutexId, pPriv->pName, DirtyErrGetName(iResult)));
    }
    pPriv->uMutexId = (unsigned)-1;
}

/*F********************************************************************************/
/*!
    \Function NetCritTry

    \Description
        Attempt to gain access to critical section. Always returns immediately
        regadless of access status. A thread that already has access to a critical
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
    
    // try and gain access
    if ((iResult = sceKernelTryLockLwMutex(&pPriv->SceMutex, NETCRIT_LOCK_COUNT)) < 0)
    {
        if (iResult != SCE_KERNEL_ERROR_LW_MUTEX_FAILED_TO_OWN)
        {
            NetPrintf(("dirtylibpsp2: sceKernelTryLockLwMutex() of sema %d returned %s\n", pPriv->uMutexId, DirtyErrGetName(iResult)));
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
    SceUInt32 uTimeout = 5*1000*1000; // five seconds in microseconds
    int32_t iResult;

    // try to get access
    if (!NetCritTry(pCrit))
    {
        // wait for mutex to become free for up to five seconds
        if ((iResult = sceKernelLockLwMutex(&pPriv->SceMutex, NETCRIT_LOCK_COUNT, &uTimeout)) < 0)
        {
            if (iResult == SCE_KERNEL_ERROR_WAIT_TIMEOUT)
            {
                SceUID iThreadId = sceKernelGetThreadId();
                NetPrintf(("dirtylibpsp2: warning -- possible deadlock condition (sema=0x%08x (%s) thid=%d)\n", pPriv->uMutexId, pPriv->pName, iThreadId));
                
                /* wait for mutex to become free - wake up once every five seconds so we can put a
                   breakpoint here if we are debugging critical section bugs */
                do
                {
                    uTimeout = 5*1000*1000; // five seconds in microseconds 
                }
                while ((iResult = sceKernelLockLwMutex(&pPriv->SceMutex, NETCRIT_LOCK_COUNT, &uTimeout)) == SCE_KERNEL_ERROR_WAIT_TIMEOUT);

                // lock error
                if (iResult < 0)
                {
                    NetPrintf(("dirtylibpsp2: error waiting on mutex (sema=0x%08x (%s), err=%s)\n", pPriv->uMutexId, pPriv->pName, DirtyErrGetName(iResult)));
                }
                else
                {
                    NetPrintf(("dirtylibpsp2: notice -- mutex acquired (sema=0x%08x (%s) thid=%d)\n", pPriv->uMutexId, pPriv->pName, iThreadId));
                }
            }
            else
            {
                NetPrintf(("dirtylibpsp2: error waiting on mutex (sema=0x%08x (%s), err=%s)\n", pPriv->uMutexId, pPriv->pName, DirtyErrGetName(iResult)));
            }
        }
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

    // release the critical section (must happen after lock)
    if ((iResult = sceKernelUnlockLwMutex(&pPriv->SceMutex, NETCRIT_LOCK_COUNT)) < 0)
    {
        NetPrintf(("dirtylibpsp2: error releasing mutex (sema=0x%08x (%s), err=%s)\n", pPriv->uMutexId, pPriv->pName, DirtyErrGetName(iResult)));
    }
}
