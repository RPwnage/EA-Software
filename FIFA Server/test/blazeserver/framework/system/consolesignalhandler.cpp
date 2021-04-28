/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class Filename

    <Describe the responsibility of the class>

    \notes
        <Any additional detail including implementation notes and references.  Delete this
        section if there are no notes.>

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/system/consolesignalhandler.h"
#include "framework/controller/processcontroller.h"
#ifdef EA_PLATFORM_LINUX
#include <signal.h>
#endif

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

static ShutdownHandler sShutdownHandler;
static void* sShutdownContext;
static uint32_t sKillCount = 0;
static ReloadHandler sReloadHandler;
static void* sReloadContext;
static SetServiceStateHandler sSetServiceStateHandler;

/*** Private Methods ******************************************************************************/

//IMPORTANT NOTE - its dangerous to do a lot from these functions, as they're pushed onto the stack at 
//any arbitrary point.  Please avoid using things like logging, and do logging downstream to avoid 
//things like locking the logging thread.

#ifdef EA_PLATFORM_LINUX
static void linuxSignalHandler(int sig)
{
    if ((sig == SIGINT) || (sig == SIGTERM))
    {        
        sKillCount++;

        if (sKillCount == 3)
        {
            // The server doesn't seem to be shutting down so force the shutdown instead
            abort();
        }
        else
        {
            sShutdownHandler(sShutdownContext);
        }
    }
    else if (sig == SIGHUP)
    {
        if (sReloadHandler != nullptr)
            sReloadHandler(sReloadContext);
    }
    else if (sig == SIGUSR1)
    {
        if (sSetServiceStateHandler != nullptr)
            sSetServiceStateHandler(false);
    }
    else if (sig == SIGUSR2)
    {
        if (sSetServiceStateHandler != nullptr)
            sSetServiceStateHandler(true);
    }
}
#else
static BOOL WINAPI windowsSignalHandler(DWORD dwCtrlType)
{
    BOOL result = FALSE;

    //Unlike linux this actually executes on a different thread.  We should initialize like we're in a 
    //thread startup here
    Fiber::FiberStorageInitializer fiberStorage;

    if (dwCtrlType == CTRL_C_EVENT)
    {
        sKillCount++;

        if (sKillCount == 3)
        {
            // The server doesn't seem to be shutting down so force the shutdown instead
            abort();
        }
        else
        {        
            sShutdownHandler(sShutdownContext);
        }
        
        result = TRUE;
    }
    else if (dwCtrlType == CTRL_BREAK_EVENT)
    {
        if (sReloadHandler != nullptr)
            sReloadHandler(sReloadContext);
        result = TRUE;
    }

    return result;
}
#endif


/*** Public Methods ******************************************************************************/

bool InstallConsoleSignalHandler(ShutdownHandler shutdownCb, void* shutdownContext,
        ReloadHandler reloadCb, void* reloadContext, SetServiceStateHandler setServiceStateCb)
{
    sShutdownHandler = shutdownCb;
    sShutdownContext = shutdownContext;
    sReloadHandler = reloadCb;
    sReloadContext = reloadContext;
    sSetServiceStateHandler = setServiceStateCb;

#ifdef EA_PLATFORM_LINUX
    // if fail, try again.
    if (signal(SIGINT, linuxSignalHandler) == SIG_ERR)
    {
        if (signal(SIGINT, linuxSignalHandler) == SIG_ERR)
        {
            return false;
        }
    }

    if (signal(SIGTERM, linuxSignalHandler) == SIG_ERR)
    {
        if (signal(SIGTERM, linuxSignalHandler) == SIG_ERR)
        {
            return false;
        }
    }

    if (signal(SIGHUP, linuxSignalHandler) == SIG_ERR)
    {
        if (signal(SIGHUP, linuxSignalHandler) == SIG_ERR)
        {
            return false;
        }
    }

    if (signal(SIGUSR1, linuxSignalHandler) == SIG_ERR)
    {
        if (signal(SIGUSR1, linuxSignalHandler) == SIG_ERR)
        {
            return false;
        }
    }

    if (signal(SIGUSR2, linuxSignalHandler) == SIG_ERR)
    {
        if (signal(SIGUSR2, linuxSignalHandler) == SIG_ERR)
        {
            return false;
        }
    }
#else
    // if fail, try again.
    if (SetConsoleCtrlHandler(windowsSignalHandler, TRUE) == 0)
    {
        if (SetConsoleCtrlHandler(windowsSignalHandler, TRUE) == 0)
        {
            return false;
        }
    }
#endif

    return true;
}

void ResetDefaultConsoleSignalHandler()
{
#ifdef EA_PLATFORM_LINUX
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGHUP, SIG_DFL);
    signal(SIGUSR1, SIG_DFL);
    signal(SIGUSR2, SIG_DFL);
#else
    SetConsoleCtrlHandler(windowsSignalHandler, FALSE);
#endif
}

} // namespace Blaze

