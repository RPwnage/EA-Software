/*H********************************************************************************/
/*!

    \File    dirtylibwinrt.cpp

    \Description
        Windows8/WindowsPhone8 specific support library for network code. Suppplies
        simple time, memory, and semaphore functions.

    \Copyright
        Copyright (c) Electronic Arts 2012.  ALL RIGHTS RESERVED.

    \Version 19/10/12 (mclouatre) Initial version

*/
/********************************************************************************H*/

#pragma warning(disable:4265) // 'class' : class has virtual functions, but destructor is not virtual
#pragma warning(disable:4548) // expression before comma has no effect; expected expression with side-effect
#pragma warning(disable:4625) //'derived class' : copy constructor could not be generated because a base class copy constructor is inaccessible
#pragma warning(disable:4626) // 'derived class' : assignment operator could not be generated because a base class assignment operator is inaccessible
#pragma warning(disable:4571) // Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught


/*** Include files ****************************************************************/
#include <windows.h>
#include <stdio.h>
#include <chrono>

#include "DirtySDK/platform.h"   // must be before <thread> for char16_t
#include <thread>

#include "DirtySDK/dirtysock/dirtylib.h"


using namespace Windows::Foundation;
using namespace Windows::System::Threading;

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! critical section type
typedef struct NetCritPrivT
{
    uint32_t thread;            //!< thread id
    uint32_t access;            //!< access count (zero if available)
    uint32_t locked;            //!< TRUE if locked, else FALSE
    CRITICAL_SECTION csect;     //!< windows critical section
} NetCritPrivT;


/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

// Private variables

extern "C"
{

// global critical section
static NetCritT _NetLib_GlobalCrit;

// idle critical section
static NetCritT _NetLib_IdleCrit;
NetCritT *_NetLib_pIdleCrit = &_NetLib_IdleCrit;

// queryperformancecounter frequency
static LARGE_INTEGER _NetLib_lFreq;

uint32_t _NetLib_bUseHighResTimer = TRUE;

// selected timer function
static uint32_t (*_NetLib_pTimerFunc)(void) = NULL;

// idle thread state
static volatile int32_t g_idlelife = -1;
static volatile int32_t g_exit = 0;
static Windows::Foundation::IAsyncAction^ g_workItem;

// debugging hooks
static void *_NetLib_pDebugParm = NULL;
static int32_t (*_NetLib_pDebugHook)(void *pParm, const char *pText) = NULL;

}

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _NetLibThread

    \Description
        Thread to handle special library tasks

    \Input uArg -   unused

    \Version 18/10/2012 (mclouatre)
*/
/********************************************************************************F*/
static void _NetLibThread(uint32_t uArg)
{
    // run while we have sema
    while (g_exit != 1)
    {
        // call idle functions
        NetIdleCall();

        // wait for next tick
        NetSleep(50);
    }

    // show we are dead
    g_idlelife = 0;
}

/*F********************************************************************************/
/*!
    \Function _NetLibGetTickCount

    \Description
        Millisecond-accurate tick counter.

    \Output
        uint32_t        - millisecond tick counter

    \Version 18/10/2012 (mclouatre)
*/
/********************************************************************************F*/
static uint32_t _NetLibGetTickCount(void)
{
    LARGE_INTEGER lCount;
    QueryPerformanceCounter(&lCount);
    return((uint32_t)(lCount.QuadPart/_NetLib_lFreq.QuadPart));
}

/*F********************************************************************************/
/*!
    \Function _NetLibGetTickCount2

    \Description
        Millisecond tick counter, with variable precision.

    \Output
        uint32_t        - millisecond tick counter

    \Version 18/10/2012 (mclouatre)
*/
/********************************************************************************F*/
static uint32_t _NetLibGetTickCount2(void)
{
    return((uint32_t)GetTickCount64());
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function NetLibCreate

    \Description
        Initialize the network library functions

    \Input iThreadPrio          - thread priority to start NetLib thread with (negative: low; 0:normal; positive:high)
    \Input iThreadStackSize     - stack size to start the _NetLibThread with (in bytes)
    \Input iThreadCpuAffinity   - cpu affinity to start the _NetLibThread with

    \Version 18/10/2012 (mclouatre)
*/
/********************************************************************************F*/
void NetLibCreate(int32_t iThreadPrio, int32_t iThreadStackSize, int32_t iThreadCpuAffinity)
{
    WorkItemPriority workItemPriority;

    // reset the idle list
    NetIdleReset();
    g_idlelife = -1;

    // initialize critical sections
    NetCritInit(NULL, "lib-global");
    NetCritInit(&_NetLib_IdleCrit, "lib-idle");

    g_exit = 0;

    // race-condition fix, refer to _NetLibThread & NetLibDestroy for more details
    g_idlelife = 1;

    WorkItemHandler^ refWorkItemHdlr = ref new WorkItemHandler([](Windows::Foundation::IAsyncAction^ operation)
    {
        _NetLibThread(0);
    });

    if (iThreadPrio > 0)
    {
        workItemPriority = WorkItemPriority::Low;
    }
    else if (iThreadPrio > 0)
    {
        workItemPriority = WorkItemPriority::High;
    }
    else
    {
        workItemPriority = WorkItemPriority::Normal;
    }
   g_workItem = ThreadPool::RunAsync(refWorkItemHdlr, workItemPriority, WorkItemOptions::TimeSliced);

    // set up high-performance timer, if available
    if (_NetLib_bUseHighResTimer && (QueryPerformanceFrequency(&_NetLib_lFreq) != 0))
    {
        _NetLib_pTimerFunc = _NetLibGetTickCount;
        _NetLib_lFreq.QuadPart /= 1000;
    }
    else // fall back on good old GetTickCount()
    {
        _NetLib_pTimerFunc = _NetLibGetTickCount2;
    }

    #if DIRTYCODE_LOGGING
    if (sizeof(NetCritT) < sizeof(NetCritPrivT))
    {
        NetPrintf(("dirtylibwinrt: warning - NetCritT is too small!\n"));
    }
    #endif
}

/*F********************************************************************************/
/*!
    \Function NetLibDestroy

    \Description
        Destroy the network lib

    \Input uShutdownFlags   - NET_SHUTDOWN_* flags

    \Version 18/10/2012 (mclouatre)
*/
/********************************************************************************F*/
void NetLibDestroy(uint32_t uShutdownFlags)
{
    // signal a shutdown
    g_exit = 1;
    
    // wait for thread to terminate
    while (g_idlelife > 0)
    {
        // wait for next tick
        NetSleep(1);
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
        Windows formatted output function

    \Input *pFormat - fmt string
    \Input ...      - varargs

    \Output
        int32_t     - zero

    \Version 18/10/2012 (mclouatre)
*/
/********************************************************************************F*/
int32_t NetPrintfCode(const char *pFormat, ...)
{
    va_list pFmtArgs;
    char strText[4096];
    const char *pText = strText;
    int32_t iOutput = 1;

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
        OutputDebugStringA(pText);
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

    \Version 18/10/2012 (mclouatre)
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
    \Function NetSleep

    \Description
        Suspends the execution of the current thread until the time-out
        interval elapses.

    \Input iMilliSecs    - number of milliseconds to block for

    \Version 18/10/2012 (mclouatre)
*/
/********************************************************************************F*/
void NetSleep(int32_t iMilliSecs)
{
    std::chrono::milliseconds duration(iMilliSecs);
    std::this_thread::sleep_for(duration);
}

/*F********************************************************************************/
/*!
    \Function NetTick

    \Description
        Return some kind of increasing tick count with millisecond scale (does
        not need to have millisecond precision, but higher precision is better).

    \Output
        uint32_t    - millisecond tick count

    \Version 18/10/2012 (mclouatre)
*/
/********************************************************************************F*/
uint32_t NetTick(void)
{
    return(_NetLib_pTimerFunc());
}

/*F********************************************************************************/
/*!
    \Function NetCritInit

    \Description
        Initialize a critical section for use. Call must have persistant storage
        defined for lifetime of critical section.

    \Input *pCrit       - critical section marker
    \Input *pCritName   - unused

    \Version 18/10/2012 (mclouatre)
*/
/********************************************************************************F*/
void NetCritInit(NetCritT *pCrit, const char *pCritName)
{
    NetCritPrivT *pPriv = (NetCritPrivT *)(pCrit ? pCrit : &_NetLib_GlobalCrit);

    // initialize the critical section
    pPriv->thread = 0;
    pPriv->access = 0;
    pPriv->locked = 0;

    InitializeCriticalSectionEx(&pPriv->csect, 20, 0);
}

/*F********************************************************************************/
/*!
    \Function NetCritKill

    \Description
        Release resources and destroy critical section. Not currently
        used (no resources to release)

    \Input *pCrit   - critical section marker

    \Version 18/10/2012 (mclouatre)
*/
/********************************************************************************F*/
void NetCritKill(NetCritT *pCrit)
{
    NetCritPrivT *pPriv = (NetCritPrivT *)(pCrit ? pCrit : &_NetLib_GlobalCrit);

    // kill the critical section
    pPriv->locked = 0;
    DeleteCriticalSection(&pPriv->csect);
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
        int32_t     - zero=unable to get access, non-zero=access granted

    \Version 18/10/2012 (mclouatre)
*/
/********************************************************************************F*/
int32_t NetCritTry(NetCritT *pCrit)
{
    NetCritPrivT *pPriv = (NetCritPrivT *)(pCrit ? pCrit : &_NetLib_GlobalCrit);

    // see if we own section already
    if (pPriv->thread == GetCurrentThreadId())
    {
        // just count the access and return
        pPriv->access += 1;
        return(1);
    }

    // try and gain access
    if (InterlockedExchange((volatile LONG *)&pPriv->locked, 1) != 0)
    {
        // someone else already has the lock
        return(0);
    }

    // grab the critical section
    EnterCriticalSection(&pPriv->csect);

    // we have lock+crit so mark us as owner
    pPriv->thread = GetCurrentThreadId();
    pPriv->access += 1;
    return(1);
}

/*F********************************************************************************/
/*!
    \Function NetCritEnter

    \Description
        Enter a critical section, blocking if needed. If implemented
        under a non-threaded O/S, then there should never be a reason
        for this routine to want to block.

    \Input *pCrit   - critical section marker

    \Version 18/10/2012 (mclouatre)
*/
/********************************************************************************F*/
void NetCritEnter(NetCritT *pCrit)
{
    NetCritPrivT *pPriv = (NetCritPrivT *)(pCrit ? pCrit : &_NetLib_GlobalCrit);

    // try to get access
    while (!NetCritTry(pCrit))
    {
        // wait for resource to become free
        EnterCriticalSection(&pPriv->csect);

        // now try and get the lock
        if (InterlockedExchange((volatile LONG *)&pPriv->locked, 1) == 0)
        {
            // we have crit+lock so we own it
            pPriv->thread = GetCurrentThreadId();
            pPriv->access += 1;
            break;
        }

        // we have critical section, but someone else has lock
        // release critical and try wait
        LeaveCriticalSection(&pPriv->csect);

        /* yield to lower-priority threads to give them a chance to acquire the
           critical section.  this avoids a race condition where a lower-priority
           thread has acquired the locked variable but was switched out before it
           could acquire the critical section. */
        NetSleep(1);
    }
}

/*F********************************************************************************/
/*!
    \Function NetCritLeave

    \Description
        Leave a critical section. Must be called once for every NetCritEnter (or
        successful NetCritTry).

    \Input *pCrit   - critical section marker

    \Version 18/10/2012 (mclouatre)
*/
/********************************************************************************F*/
void NetCritLeave(NetCritT *pCrit)
{
    NetCritPrivT *pPriv = (NetCritPrivT *)(pCrit ? pCrit : &_NetLib_GlobalCrit);

    // handle based on number of accesses
    if (pPriv->access > 1)
    {
        pPriv->access -= 1;
    }
    else
    {
        // reset thread and access count
        pPriv->thread = 0;
        pPriv->access = 0;
        // return the lock
        pPriv->locked = 0;
        // release the critical section (must happen after lock)
        LeaveCriticalSection(&pPriv->csect);
    }
}