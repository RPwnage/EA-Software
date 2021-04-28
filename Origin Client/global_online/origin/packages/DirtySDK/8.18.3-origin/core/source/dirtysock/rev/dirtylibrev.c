/*H********************************************************************************/
/*!
    \File dirtylibrev.c

    \Description
        Platform-specific support library for network code.  Supplies simple time,
        debug printing, and mutex functions.

    \Copyright
        Copyright (c) 2006 Electronic Arts Inc.

    \Version 11/27/2006 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>

#include <revolution/os.h>

#include "platform.h"
#include "dirtylib.h"
#include "dirtyerr.h"

/*** Defines **********************************************************************/


/*** Type Definitions *************************************************************/

//! critical section type
typedef struct NetCritPrivT
{
    OSMutex Mutex;      //!< system mutex (24 bytes)
    OSThread *pThread;  //!< thread pointer
    int32_t iAccessCt;  //!< access count (zero if available) 
    const char *pName;  //!< critical section name
} NetCritPrivT;

/*** Variables ********************************************************************/

// idle critical section
static NetCritT _NetLib_IdleCrit;
NetCritT *_NetLib_pIdleCrit = &_NetLib_IdleCrit;

// global critical section
static NetCritT _NetLib_GlobalCrit;

// idle thread state
static volatile int32_t g_idlelife = -1;
static int32_t  g_idlethread = 0;

// stack
#define NETLIB_THREADSTACKSIZE (8192)
static uint64_t _NetLib_aStack[NETLIB_THREADSTACKSIZE/sizeof(uint64_t)];
#define NETLIB_THREADSTACKSIZE_WORD  (sizeof(_NetLib_aStack)/sizeof(_NetLib_aStack[0]))

// debug printf hook
#if DIRTYCODE_LOGGING
static void *_NetLib_pDebugParm = NULL;
static int32_t (*_NetLib_pDebugHook)(void *pParm, const char *pText) = NULL;
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
static void _NetLibThread(void *pArg)
{
    // get thread id
    #if DIRTYCODE_LOGGING
    OSThread *pThread = OSGetCurrentThread();
    NetPrintf(("dirtylibrev: netlib thread running (thread=0x%08x)\n", pThread));
    #endif

    // show we are running
    g_idlelife = 1;

    // run while we have sema
    while (g_idlethread != 0)
    {
        // call idle functions
        NetIdleCall();

        // wait for next tick
        OSSleepMilliseconds(50);
    }

    // show we are dead
    g_idlelife = 0;

    // report termination
    NetPrintf(("dirtylibrev: idle thread exiting\n"));
    OSExitThread(NULL);
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
    static OSThread Thread;
    
    // reset the idle list
    NetIdleReset();
    g_idlelife = -1;

    // initialize critical sections
    NetCritInit(NULL, "lib-global");
    NetCritInit(&_NetLib_IdleCrit, "lib-idle");

    // single g_idlethread is also the "quit flag", set to non-zero before create to avoid bogus exit
    g_idlethread = 1;

    // create a worker thread
    OSCreateThread(&Thread, _NetLibThread, NULL, _NetLib_aStack+NETLIB_THREADSTACKSIZE_WORD, NETLIB_THREADSTACKSIZE, iThreadPrio, OS_THREAD_ATTR_DETACH);
    OSResumeThread(&Thread);
    
    #if DIRTYCODE_LOGGING
    if (sizeof(NetCritT) < sizeof(NetCritPrivT))
    {
        NetPrintf(("\ndirtylibrev: warning, NetCritT is too small!\n\n"));
    }
    #endif
}

/*F********************************************************************************/
/*!
    \Function NetLibDestroy

    \Description
        Destroy the network library module.

    \Version 06/21/2006 (jbrookes)
*/
/********************************************************************************F*/
void NetLibDestroy(void)
{
    // signal a shutdown
    g_idlethread = 0;

    // wait for thread to terminate
    while (g_idlelife > 0)
    {
        OSSleepMilliseconds(1);
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
    char strText[2048];
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
        OSReport("%s", pText);
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
    return(OSTicksToMilliseconds(OSGetTime()));
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

    // initialize the critical section
    memset(pPriv, 0, sizeof(*pPriv));
    pPriv->pName = (pCritName != NULL) ? pCritName : "unknown";

    // initialize the mutex
    OSInitMutex(&pPriv->Mutex);
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
    memset(pPriv, 0, sizeof(*pPriv));
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
    OSThread *pThread;
    int32_t iResult;
    
    // get thread id
    pThread = OSGetCurrentThread();

    // see if we own section already
    if (pPriv->pThread == pThread)
    {
        // just count the access and return
        pPriv->iAccessCt += 1;
        return(1);
    }

    // see if someone else owns it
    if (pPriv->pThread != NULL)
    {
        return(0);
    }

    // try and gain access
    if ((iResult = OSTryLockMutex(&pPriv->Mutex)) == 0)
    {
        return(0);
    }

    // we have acquired the mutex, so mark us as owner
    pPriv->pThread = pThread;
    pPriv->iAccessCt += 1;
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

    // try to get access
    if (!NetCritTry(pCrit))
    {
        OSThread *pThread = OSGetCurrentThread();
        
        // lock the mutex
        OSLockMutex(&pPriv->Mutex);

        // mark as ours
        pPriv->pThread = pThread;
        pPriv->iAccessCt += 1;
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

    // handle based on number of accesses
    if (pPriv->iAccessCt > 1)
    {
        pPriv->iAccessCt -= 1;
    }
    else
    {
        // reset thread and access count
        pPriv->pThread = NULL;
        pPriv->iAccessCt = 0;

        // release the critical section (must happen after lock)
        OSUnlockMutex(&pPriv->Mutex);
    }
}
