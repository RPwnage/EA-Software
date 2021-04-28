/*H********************************************************************************/
/*!
    \File dirtylibps3.c

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
#include <system_types.h>
#include <ppu_thread.h>
#include <synchronization.h>
#include <sys_time.h>
#include <time_util.h>
#include <timer.h>

#include <sdk_version.h>
#include <netex/net.h>

#include "dirtysock.h"
#include "dirtyerr.h"

/*** Defines **********************************************************************/


/*** Type Definitions *************************************************************/

//! critical section type
typedef struct NetCritPrivT
{
    sys_mutex_t uMutexId;       //!< sce mutex
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
static sys_ppu_thread_t g_idlethread = 0;

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

    \Input *pArg    - pointer to argument

    \Output
        None.

    \Version 06/21/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _NetLibThread(uint64_t pArg)
{
    NetPrintf(("dirtylibps3: idle thread running (thid=%d)\n", g_idlethread));

    // show we are running
    g_idlelife = 1;

    // run while we have sema
    while (g_idlethread != 0)
    {
        // call idle functions
        NetIdleCall();

        // wait for next tick
        sys_timer_usleep(50*1000);
    }

    // show we are dead
    g_idlelife = 0;

    // report termination
    NetPrintf(("dirtylibps3: idle thread exiting\n"));

    // terminate ourselves
    sys_net_free_thread_context(0, SYS_NET_THREAD_SELF);
    sys_ppu_thread_exit(0);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function NetLibCreate

    \Description
        Initialize the network library module.

    \Input iThreadPrio  - thread priority to start NetLib thread with

    \Output
        None.

    \Version 06/21/2006 (jbrookes)
*/
/********************************************************************************F*/
void NetLibCreate(int32_t iThreadPrio)
{
    // reset the idle list
    NetIdleReset();
    g_idlelife = -1;

    // initialize critical sections
    NetCritInit(NULL, "lib-global");
    NetCritInit(&_NetLib_IdleCrit, "lib-idle");

    // single g_idlethread is also the "quit flag", set to non-zero before create to avoid bogus exit
    g_idlethread = 1;

    // create a worker thread
    sys_ppu_thread_create(&g_idlethread, _NetLibThread, 0, iThreadPrio, 16*1024, 0, "NetLib Idle");
    if ((signed)g_idlethread <= 0)
    {
        NetPrintf(("dirtylibps3: unable to create idle thread (err=%s)\n", DirtyErrGetName((uint32_t)g_idlethread)));
    }

    #if DIRTYCODE_LOGGING
    // set explicit printf buffer to avoid dynamic malloc() use
    setvbuf(stdout, _NetLib_aPrintfMem, _IOLBF, sizeof(_NetLib_aPrintfMem));
    // validate NetCritT size
    if (sizeof(NetCritT) < sizeof(NetCritPrivT))
    {
        NetPrintf(("\ndirtylibps3: warning, NetCritT is too small!\n\n"));
    }
    #endif
}

/*F********************************************************************************/
/*!
    \Function NetLibDestroy -- DEPRECATE in V9

    \Description
        Destroy the network library module.

    \Version 06/21/2006 (jbrookes)
*/
/********************************************************************************F*/
void NetLibDestroy(void)
{
    NetLibDestroy2(0);
}

/*F********************************************************************************/
/*!
    \Function NetLibDestroy2

    \Description
        Destroy the network library module.

    \Input uShutdownFlags   - NET_SHUTDOWN_* flags

    \Version 06/21/2006 (jbrookes)
*/
/********************************************************************************F*/
void NetLibDestroy2(uint32_t uShutdownFlags)
{
    /* If SHUTDOWN_THREADSTARVE is passed in, instead of shutting down we simply
       lower the priority of the recv thread to starve the thread and prevent it from
       executing.  This functionality is expected to be used in an XMB exit situation
       where the main code can be interrupted at any time, in any state, and forced
       to exit.  In this situation, simply preventing the threads from executing
       further allows the game to safely exit. */
    if (uShutdownFlags & NET_SHUTDOWN_THREADSTARVE)
    {
        sys_ppu_thread_set_priority(g_idlethread, 3071);
        return;
    }

    // signal a shutdown
    g_idlethread = 0;

    // wait for thread to terminate
    while (g_idlelife > 0)
    {
        sys_timer_usleep(1);
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
    uint64_t CurTime;
    static uint64_t sTimeBaseFreqency = 0;

    if (!sTimeBaseFreqency)
    {
        sTimeBaseFreqency = (sys_time_get_timebase_frequency() / 1000);
    }

    SYS_TIMEBASE_GET(CurTime);
    CurTime /= sTimeBaseFreqency;

    return((uint32_t)CurTime);
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
    sys_mutex_attribute_t MutexAttr;
    int32_t iResult;

    // initialize the critical section
    memset(pPriv, 0, sizeof(*pPriv));
    pPriv->pName = (pCritName != NULL) ? pCritName : "unknown";

    // create mutex
    sys_mutex_attribute_initialize(MutexAttr);
    MutexAttr.attr_recursive = SYS_SYNC_RECURSIVE;
    if ((iResult = sys_mutex_create(&pPriv->uMutexId, &MutexAttr)) != CELL_OK)
    {
        NetPrintf(("dirtylibps3: unable to create mutex %s (err=%s)\n", pPriv->pName, DirtyErrGetName(iResult)));
        pPriv->uMutexId = (uint32_t)-1;
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
    if ((iResult = sys_mutex_destroy(pPriv->uMutexId)) != CELL_OK)
    {
        NetPrintf(("dirtylibps3: unable to delete mutex (sema=0x%08x (%s), err=%s)\n", pPriv->uMutexId, pPriv->pName, DirtyErrGetName(iResult)));
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
    if ((iResult = sys_mutex_trylock(pPriv->uMutexId)) < 0)
    {
        if (iResult != EBUSY)
        {
            NetPrintf(("dirtylibps3: sys_mutex_trylock() of sema %d returned %s\n", pPriv->uMutexId, DirtyErrGetName(iResult)));
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

    // try to get access
    if (!NetCritTry(pCrit))
    {
        // wait for mutex to become free for up to five seconds
        if ((iResult = sys_mutex_lock(pPriv->uMutexId, 5*1000*1000)) < 0)
        {
            if (iResult == ETIMEDOUT)
            {
                sys_ppu_thread_t iThreadId;
                sys_ppu_thread_get_id(&iThreadId);
                NetPrintf(("dirtylibps3: warning -- possible deadlock condition (sema=0x%08x (%s) thid=%d)\n", pPriv->uMutexId, pPriv->pName, iThreadId));
                if ((iResult = sys_mutex_lock(pPriv->uMutexId, 0)) < 0)
                {
                    NetPrintf(("dirtylibps3: error waiting on mutex (sema=0x%08x (%s), err=%s)\n", pPriv->uMutexId, pPriv->pName, DirtyErrGetName(iResult)));
                }
                else
                {
                    NetPrintf(("dirtylibps3: notice -- mutex acquired (sema=0x%08x (%s) thid=%d)\n", pPriv->uMutexId, pPriv->pName, iThreadId));
                }
            }
            else
            {
                NetPrintf(("dirtylibps3: error waiting on mutex (sema=0x%08x (%s), err=%s)\n", pPriv->uMutexId, pPriv->pName, DirtyErrGetName(iResult)));
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
    if ((iResult = sys_mutex_unlock(pPriv->uMutexId)) < 0)
    {
        NetPrintf(("dirtylibps3: error releasing mutex (sema=0x%08x (%s), err=%s)\n", pPriv->uMutexId, pPriv->pName, DirtyErrGetName(iResult)));
    }
}
