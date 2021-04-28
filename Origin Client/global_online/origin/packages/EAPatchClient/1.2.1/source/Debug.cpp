///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/Debug.cpp
//
// Copyright (c) Electronic Arts. All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////


#include <EAPatchClient/Config.h>
#include <EAPatchClient/Error.h>
#include <EAPatchClient/Locale.h>
#include <EAPatchClient/Debug.h>
#include <EAStdC/EARandom.h>
#include <EAStdC/EASprintf.h>
#include <stdarg.h>

#if defined(EA_PLATFORM_XENON)
    #pragma warning(push, 0)
    #include <crtdbg.h>
    #include <xtl.h>
    #if EA_XBDM_ENABLED
        #include <xbdm.h>
        // #pragma comment(lib, "xbdm.lib") The application is expected to provide this if needed. 
    #endif
    #pragma warning(pop)
#elif defined(EA_PLATFORM_MICROSOFT)
    #pragma warning(push, 0)
    #include <Windows.h>
    extern "C" WINBASEAPI BOOL WINAPI IsDebuggerPresent();
    #pragma warning(pop)
#elif defined(EA_PLATFORM_PS3)
    #include <libsn.h>
#elif defined(__APPLE__)    // OS X, iPhone, iPad, etc.
    #include <stdbool.h>
    #include <sys/types.h>
    #include <unistd.h>
    #include <sys/sysctl.h>
#elif defined(EA_HAVE_SYS_PTRACE_H)
    #include <sys/ptrace.h>
#endif


namespace EA
{
namespace Patch
{


///////////////////////////////////////////////////////////////////////////////
// RandomizeMemory
///////////////////////////////////////////////////////////////////////////////

void RandomizeMemory(void* p, size_t size)
{
    EA::StdC::Random rand;
    uint8_t* p8 = static_cast<uint8_t*>(p);

    // To do: Make this fill in uint32_t or uint64_t blocks instead of uint8_t.
    //        In doing so watch out that the dest may not be aligned and the 
    //        size may not be aligned. One solution is to fill a memory of 
    //        uint32_t values with random data, and memcpy it to the dest here.

    for(size_t i = 0; i < size; i++)
        p8[i] = (uint8_t)rand.RandomUint32Uniform();
}



///////////////////////////////////////////////////////////////////////////////
// EAPATCH_TRACE
///////////////////////////////////////////////////////////////////////////////


// The implementation for EAIsDebuggerPresent is copied from the EATrace package.
static bool EAIsDebuggerPresent()
{
    #if defined(EA_PLATFORM_XENON)
        #if EA_XBDM_ENABLED
            return DmIsDebuggerPresent() != 0;
        #else
            return false;
        #endif

    #elif defined(EA_PLATFORM_MICROSOFT)
        return ::IsDebuggerPresent() != 0;

    #elif defined(EA_PLATFORM_PSP2)
        // This will be available around SDK 1.5. https://psvita.scedev.net/forums/thread/1351/
        return false;

    #elif defined(EA_PLATFORM_PS3) || defined(SN_TARGET_PS3)
        return snIsDebuggerRunning() != 0;

    #elif defined(EA_PLATFORM_REVOLUTION)
        return DBIsDebuggerPresent() != 0;

    #elif defined(__APPLE__) // OS X, iPhone, iPad, etc.
        // See http://developer.apple.com/mac/library/qa/qa2004/qa1361.html
        int               junk;
        int               mib[4];
        struct kinfo_proc info;
        size_t            size;

        // Initialize the flags so that, if sysctl fails for some bizarre reason, we get a predictable result.
        info.kp_proc.p_flag = 0;

        // Initialize mib, which tells sysctl the info we want, in this case we're looking for information about a specific process ID.
        mib[0] = CTL_KERN;
        mib[1] = KERN_PROC;
        mib[2] = KERN_PROC_PID;
        mib[3] = getpid();

        // Call sysctl.
        size = sizeof(info);
        junk = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);
        //assert(junk == 0);

        // We're being debugged if the P_TRACED flag is set.
        return ((info.kp_proc.p_flag & P_TRACED) != 0);

    #elif defined(EA_HAVE_SYS_PTRACE_H) && defined(PT_TRACE_ME)
        // See http://www.phiral.net/other/linux-anti-debugging.txt and http://linux.die.net/man/2/ptrace
        return (ptrace(PT_TRACE_ME, 0, 0, 0) < 0); // A synonym for this is PTRACE_TRACEME.

    #else
        return false;

    #endif
}


static void PrintDebugString(const String& sMessage)
{
    #if defined(EA_PLATFORM_MICROSOFT) && !defined(EA_PLATFORM_CONSOLE) // No need to do this for Microsoft console platforms, as the fputs below covers that.
        if(EAIsDebuggerPresent())
        {
            OutputDebugStringA("[EAPatch] ");
            OutputDebugStringA(sMessage.c_str());
        }
    #else
        if(EAIsDebuggerPresent())
            { }
    #endif

    // This printf may go to nowhere on some platforms, such as Windows when this is a GUI app as opposed to a text console app.
    EA::StdC::Printf("[EAPatch] %s", sMessage.c_str());

    #if defined(EA_PLATFORM_MOBILE)
        fflush(stdout); // Mobile platforms need this because otherwise you can easily lose output if the device crashes while not all the output has been written.
    #endif
}



EAPATCHCLIENT_API void EAPATCH_TRACE_FORMATTED_IMP(const char* pFile, int nLine, const char* pFormat, ...)
{
    String  sMessage;
    va_list arguments;
    va_start(arguments, pFormat);
    int result = EA::StdC::StringVcprintf(sMessage, pFormat, arguments);
    va_end(arguments);

    if(result >= 0)
    {
        intptr_t           userContext = 0;
        DebugTraceHandler* pDebugTraceHandler = GetUserDebugTraceHandler(&userContext);

        if(pDebugTraceHandler) // If there is a registered handler...
        {
            DebugTraceInfo debugTraceInfo;
            
            debugTraceInfo.mpFile    = pFile;
            debugTraceInfo.mLine     = nLine;
            debugTraceInfo.mpText    = sMessage.c_str();
            debugTraceInfo.mSeverity = 0;

            pDebugTraceHandler->HandleEAPatchDebugTrace(debugTraceInfo, userContext);
        }
        else
            PrintDebugString(sMessage);
    }
}



///////////////////////////////////////////////////////////////////////////////
// DebugTraceInfo
///////////////////////////////////////////////////////////////////////////////

DebugTraceInfo::DebugTraceInfo()
  : mpFile(NULL)
  , mLine(0)
  , mpText(NULL)
  , mSeverity(0)
{
}


///////////////////////////////////////////////////////////////////////////////
// RegisterUserDebugTraceHandler
///////////////////////////////////////////////////////////////////////////////

DebugTraceHandler* gpDebugTraceHandler = NULL;
intptr_t           gDebugTraceHandlerContext = 0;


EAPATCHCLIENT_API void RegisterUserDebugTraceHandler(DebugTraceHandler* pDebugTraceHandler, intptr_t userContext)
{
    gpDebugTraceHandler       = pDebugTraceHandler;
    gDebugTraceHandlerContext = userContext;
}


EAPATCHCLIENT_API DebugTraceHandler* GetUserDebugTraceHandler(intptr_t* pUserContext)
{
    if(pUserContext)
        *pUserContext = gDebugTraceHandlerContext;

    return gpDebugTraceHandler;
}




} // namespace Patch
} // namespace EA







