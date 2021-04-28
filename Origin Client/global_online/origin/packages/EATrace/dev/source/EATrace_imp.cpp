/////////////////////////////////////////////////////////////////////////////
// EATrace_imp.cpp
//
// Copyright (c) 2003, Electronic Arts Inc. All rights reserved.
// Created by Paul Pedriana, Maxis
/////////////////////////////////////////////////////////////////////////////


#include <EATrace/internal/Config.h>
#include <EATrace/internal/CoreAllocatorNew.h>
#include <EATrace/Allocator.h>
#include <EATrace/EATrace_imp.h>
#include <EATrace/internal/RefCount.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EASprintf.h>
#include <EASTL/algorithm.h>
#include <stdio.h>
#include <stdarg.h>

#if EATRACE_GLOBAL_ENABLED
    #include <EAStdC/EAGlobal.h>
#endif

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
    #include <sys/ppu_thread.h>
    #include <sys/timer.h>
    #include <libsn.h>
#elif defined(EA_PLATFORM_REVOLUTION)
    #include <revolution\db.h>
#elif defined(EA_PLATFORM_CTR)
    #include <nn/dbg.h>
#elif defined(EA_PLATFORM_AIRPLAY)
    #include <s3eDebug.h>
#elif defined(__APPLE__)    // OS X, iPhone, iPad, etc.
    #include <stdbool.h>
    #include <sys/types.h>
    #include <unistd.h>
    #include <sys/sysctl.h>
#elif defined(EA_HAVE_SYS_PTRACE_H)
    #include <sys/ptrace.h>
#endif

#if defined(EA_PLATFORM_ANDROID)
    #include <android/log.h>
#endif

#if defined(EA_PLATFORM_PS3)
    #include <libsn.h>
#endif


/// EA_INIT_PRIORITY_AVAIALABLE
///
/// Defined if the compiler supports __attribute__ ((init_priority (n))).
/// Matches the EABase definition, in the case that the user is using an older version 
/// of EABase that doesn't already handle this.
///
#if (EABASE_VERSION_N < 20026) // If EABase isn't already defining this for us...
    #if defined(__GNUC__)
        #if defined(__SNC__)                        // EDG typically #defines __GNUC__ but doesn't
            #define EA_INIT_PRIORITY_AVAILABLE      // implement init_priority. Except some variants 
        #elif !defined(__EDG__)                     // of EDG (e.g. SN) do implement it anyway.
            #define EA_INIT_PRIORITY_AVAILABLE
        #endif
    #endif
#endif



// To do: Provide tracer filtering of all dialogs, all pText, and certain types of 
//        messages (log, trace, assert) even when the statements are embeded in the code.  
//        Do not force user to recompile the codebase. Can change the filter on the 
//        fly and trace/log code kicks in immediately.

///////////////////////////////////////////////////////////////////////////////
// EATrueDummyFunction
//
// This is a dummy function for the purpose of supporting EA_TRACE_TRUE / EA_TRACE_FALSE
//
#if !(defined(__GNUC__) && (__GNUC__ >= 4)) // The GCC version of this is defined inline in EATraceBase.h
    void EATrueDummyFunction()
    {
        // Intentionally empty.
    }
#endif


namespace EA
{
    namespace Trace
    {
        //////////////////////////////////////////////////////////////////////////
        // EATraceShutdownValue
        //
        // Shared variable denoting if the trace system is alive or shut down.
        //
        class EATraceShutdownValue
        {
        public:
            bool mbShutdown;
            EATraceShutdownValue() : mbShutdown(false) { }
        };

        #if !EATRACE_GLOBAL_ENABLED
            static EATraceShutdownValue  sShutdownValue;
            static EATraceShutdownValue* gShutdownValue = &sShutdownValue;
        #elif defined(EA_INIT_PRIORITY_AVAIALABLE)
            static EA::StdC::AutoStaticOSGlobalPtr<EATraceShutdownValue, 0x0b97383a> gShutdownValue __attribute__ ((init_priority (11000)));
        #else
            static EA::StdC::AutoStaticOSGlobalPtr<EATraceShutdownValue, 0x0b97383a> gShutdownValue;
        #endif


        void Shutdown()
        {
            gShutdownValue->mbShutdown = true;
            SetTraceHelperTable(NULL);
            SetTracer(NULL);            
        }
        

        // only support the default helper table for now
        ITraceHelperTable* CreateDefaultTraceHelperTable(Allocator::ICoreAllocator* pAllocator)
        {
            return new(pAllocator, EATRACE_ALLOC_PREFIX "TraceHelperTable") TraceHelperTable;
        }


        // If EA_CUSTOM_TRACER_ENABLED is defined to 1, then we expect that 
        // the user will implement their own version of CreateDefaultTracer.
        // Check for EA_LOG_ENABLED because if it is enabled then EALog will 
        // implement the default tracer instead.
        #if !EA_CUSTOM_TRACER_ENABLED && !EA_LOG_ENABLED
            ITracer* CreateDefaultTracer(Allocator::ICoreAllocator* pAllocator)
            {
                return new(pAllocator, EATRACE_ALLOC_PREFIX "Tracer") Tracer;
            }
        #else // Else the user is expected to provide this function.
            extern ITracer* CreateDefaultTracer(Allocator::ICoreAllocator* pAllocator);
        #endif
    }
}


///////////////////////////////////////////////////////////////////////////////
/// EAIsDebuggerPresent
///
/// Returns true if the app appears to be running under a debugger.
///
bool EAIsDebuggerPresent()
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




///////////////////////////////////////////////////////////////////////////////
// EAOutputDebugString / EAOutputDebugStringF
//
#if EAOUTPUTDEBUGSTRING_DEFINED && EAOUTPUTDEBUGSTRING_ENABLED
    #if !defined(EAOutputDebugString) // If we are implementing this as a function instead of a macro...
        #include <stdarg.h>

        void EAOutputDebugString(const char* pStr)
        {
            #if defined(EA_PLATFORM_MICROSOFT)
                OutputDebugStringA(pStr);
            #elif defined(EA_PLATFORM_CTR)
                NN_LOG("EATrace: %s", pStr);
            #elif defined(EA_PLATFORM_AIRPLAY)
                if(s3eDebugIsDebuggerPresent())
                    s3eDebugOutputString(pStr);
            #elif defined(EA_PLATFORM_ANDROID)
                __android_log_write(ANDROID_LOG_INFO, "EATrace", pStr);
            #else
                fprintf(stdout, "%s", pStr);

                #if EATRACE_FLUSH_ON_WRITE_ENABLED_BY_DEFAULT
                    fflush(stdout);
                #endif
            #endif
        }

        void EAOutputDebugStringF(const char* pFormat, ...)
        {
            char    buf[4096];
            va_list arguments;
            va_start(arguments, pFormat);

            // We don't have support for dealing with the case whereby the buffer
            // isn't big enough. We can fix this if it becomes an issue.
            if((unsigned)EA::StdC::Vsnprintf(buf, sizeof(buf), pFormat, arguments) < (unsigned)sizeof(buf))
            {
                #if defined(EA_PLATFORM_MICROSOFT)
                    OutputDebugStringA(buf);
                #elif defined(EA_PLATFORM_CTR)
                    NN_LOG("EATrace: %s", buf);
                #elif defined(EA_PLATFORM_AIRPLAY)
                    if(s3eDebugIsDebuggerPresent())
                        s3eDebugOutputString(buf);
                #elif defined(EA_PLATFORM_ANDROID)
                    __android_log_write(ANDROID_LOG_INFO, "EATrace", buf);
                #else
                    fprintf(stdout, "%s", buf);

                    #if EATRACE_FLUSH_ON_WRITE_ENABLED_BY_DEFAULT
                        fflush(stdout);
                    #endif
                #endif
            }

            va_end(arguments);
        }

    #endif
#endif






#if defined(_MSC_VER) // If using VC++...
    // Make sure the tracer and helper table are initialized as early as possible
    #pragma warning(disable: 4073) // "initializers put in library initialization area"
    #pragma init_seg(lib)
#else
    // GCC equivilent is __attribute__ ((init_priority (###))); where ### is between
    // 101 and 64k, the lower being higher priority. This is defined for each 
    // varible, so we need to add the attribute to the static objects themselves.
    // The memory manager is set at priority 10000. Static objects in this file 
    // are set at 11000, so they will be constructed after the memory manager.
#endif


//////////////////////////////////////////////////////////////////////////
// GetTraceDialogEnabled / SetTraceDialogEnabled
//
// We set it up so that trace dialogs don't get enabled until at least 
// after this module is initialized. The result is that early startup
// code will not trigger dialogs unless that code specifically enables
// dialogs. 
//
class EATraceDialogEnabledValue
{
public:
    int mTraceDialogEnabled; // -1 means not yet specified and disabled, 0 means disabled, 1 means enabled.

    EATraceDialogEnabledValue() 
        #if defined(EA_PLATFORM_DESKTOP) // Windows, Linux, OSX, etc.
          : mTraceDialogEnabled(1)
        #else
          : mTraceDialogEnabled(0)
        #endif
    {
    }
};


#if !EATRACE_GLOBAL_ENABLED
    static EATraceDialogEnabledValue  sTraceDialogEnabled;
    static EATraceDialogEnabledValue* gTraceDialogEnabled = &sTraceDialogEnabled;
#elif defined(EA_INIT_PRIORITY_AVAIALABLE)
    static EA::StdC::AutoStaticOSGlobalPtr<EATraceDialogEnabledValue, 0x00ee4655> gTraceDialogEnabled __attribute__ ((init_priority (11000)));
#else
    static EA::StdC::AutoStaticOSGlobalPtr<EATraceDialogEnabledValue, 0x00ee4655> gTraceDialogEnabled;
#endif


bool EA::Trace::GetTraceDialogEnabled()
{
    return (gTraceDialogEnabled->mTraceDialogEnabled == 1);
}

void EA::Trace::SetTraceDialogEnabled(bool bEnabled)
{
    gTraceDialogEnabled->mTraceDialogEnabled = (bEnabled ? 1 : 0);
}
//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
// GetTraceDialogCallback / SetTraceDialogCallback
//
struct EATraceDialogCallbackValue
{
    EATraceDialogCallbackValue() :
        mpFunction(NULL),
        mpContext(NULL)
    { }

    EA::Trace::TraceDialogCallbackFunction  mpFunction;
    void *                                  mpContext;
};


#if !EATRACE_GLOBAL_ENABLED
    static EATraceDialogCallbackValue  sTraceDialogCallback;
    static EATraceDialogCallbackValue* gTraceDialogCallback = &sTraceDialogCallback;
#elif defined(EA_COMPILER_GNUC)
    static EA::StdC::AutoStaticOSGlobalPtr<EATraceDialogCallbackValue, 0x9209ae98> gTraceDialogCallback __attribute__ ((init_priority (11000)));
#else
    static EA::StdC::AutoStaticOSGlobalPtr<EATraceDialogCallbackValue, 0x9209ae98> gTraceDialogCallback;
#endif


void EA::Trace::SetTraceDialogCallback(TraceDialogCallbackFunction pFunction, void *pContext)
{
    gTraceDialogCallback->mpFunction = pFunction;
    gTraceDialogCallback->mpContext  = pContext;
}
//////////////////////////////////////////////////////////////////////////




//////////////////////////////////////////////////////////////////////////
// GetTraceHelperTable / SetTraceHelperTable
//
class EATraceHelperTablePtr
{
public:
    EATraceHelperTablePtr(EA::Allocator::ICoreAllocator* pAllocator = NULL)
        : mpCoreAllocator(pAllocator)
    {
    }

    EA::Trace::ITraceHelperTable* Create()
    {
        // This function is not thread-safe. Making it thread-safe would be more 
        // tedious that it is probably worth. If the user is concerned about two
        // threads calling this function simultaneously, then the user should 
        // simply explicitly call this function on startup. That way it won't be
        // lazy-called by arbitrary threads.
        mPtr = EA::Trace::CreateDefaultTraceHelperTable(mpCoreAllocator ? mpCoreAllocator : EA::Trace::GetAllocator());
        return mPtr;
    }

    EA::Trace::AutoRefCount<EA::Trace::ITraceHelperTable> mPtr;
    EA::Allocator::ICoreAllocator*                        mpCoreAllocator;
};


#if !EATRACE_GLOBAL_ENABLED
    static EATraceHelperTablePtr  sTraceHelperTable;
    static EATraceHelperTablePtr* gTraceHelperTable = &sTraceHelperTable;
#elif defined(EA_COMPILER_GNUC)
    static EA::StdC::AutoStaticOSGlobalPtr<EATraceHelperTablePtr, 0xeea485e3> gTraceHelperTable __attribute__ ((init_priority (11000)));
#else
    static EA::StdC::AutoStaticOSGlobalPtr<EATraceHelperTablePtr, 0xeea485e3> gTraceHelperTable;
#endif


// GetTraceHelperTable
// This function gets the OS global trace helper table. 
// Startup dll should maintain the global.
// 
EA::Trace::ITraceHelperTable* EA::Trace::GetTraceHelperTable()
{
    // I think we have a possible thread race situation here if one thread is shutting down the 
    // trace system while another is calling this function. Adding thread safety to this would
    // be pretty complicated and not needed most of the time. So for now the solution is to 
    // not create a situation where this problem could occur.
    if(EA::Trace::gShutdownValue->mbShutdown)
        return NULL;
    else
    {
        EA::Trace::ITraceHelperTable* const pTable = gTraceHelperTable->mPtr;

        if(pTable)
            return pTable;

        return gTraceHelperTable->Create();
    }
}


void EA::Trace::SetTraceHelperTable(EA::Trace::ITraceHelperTable* table)
{
    gTraceHelperTable->mPtr = table;
}





///////////////////////////////////////////////////////////////////////////////
// GetTracer / SetTracer
//
class EATracerPtr
{
public:
    EATracerPtr(EA::Allocator::ICoreAllocator* pAllocator = NULL)
        : mpCoreAllocator(pAllocator)
    {
    }

    EA::Trace::ITracer* Create()
    {
        // This function is not thread-safe. Making it thread-safe would be more 
        // tedious that it is probably worth. If the user is concerned about two
        // threads calling this function simultaneously, then the user should 
        // simply explicitly call this function on startup. That way it won't be
        // lazy-called by arbitrary threads.
        if(!gTraceHelperTable->mPtr)
            gTraceHelperTable->Create();
        mPtr = EA::Trace::CreateDefaultTracer(mpCoreAllocator ? mpCoreAllocator : EA::Trace::GetAllocator());
        gTraceHelperTable->mPtr->UpdateTracer(mPtr); // Update the helper table.
        return mPtr;
    }

    EA::Trace::AutoRefCount<EA::Trace::ITracer> mPtr;
    EA::Allocator::ICoreAllocator*       mpCoreAllocator;
};


#if !EATRACE_GLOBAL_ENABLED
    static EATracerPtr  sTracer;
    static EATracerPtr* gTracer = &sTracer;
#elif defined(EA_COMPILER_GNUC)
    static EA::StdC::AutoStaticOSGlobalPtr<EATracerPtr, 0x0ea48606> gTracer __attribute__ ((init_priority (11000)));
#else
    static EA::StdC::AutoStaticOSGlobalPtr<EATracerPtr, 0x0ea48606> gTracer;
#endif


EA::Trace::ITracer* EA::Trace::GetTracer()
{
    // Potential problem: This is not thread-safe. The user should call 
    // EA::Trace::SetTracer or EA::Trace::SetServer on app startup or else
    // implement a mutex around calls to GetTracer and GetServer.

    if(EA::Trace::gShutdownValue->mbShutdown)
        return NULL;

    EA::Trace::ITracer* const pTracer = gTracer->mPtr;

    if(pTracer)
        return pTracer;

    return gTracer->Create();
}

void EA::Trace::SetTracer(EA::Trace::ITracer* tracer)
{
    // Potential problem: This is not thread-safe. The user should call 
    // EA::Trace::SetTracer or EA::Trace::SetServer on app startup or else
    // implement a mutex around calls to GetTracer and GetServer.

    // This will release the old tracer.
    gTracer->mPtr = tracer;

    // Update the helper table, so all helpers point to the correct tracer.
    if(GetTraceHelperTable())
        GetTraceHelperTable()->UpdateTracer(tracer);
}




namespace EA
{
namespace Trace
{

//////////////////////////////////////////////////////////////////////////
// DisplayTraceDialog
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// EATraceDisplayDialogValue
//
class EATraceDisplayDialogValue
{
public:
    DisplayTraceDialogFunc mpDisplayTraceDialogFunc;
    void*                  mpDisplayTraceDialogContext;

    EATraceDisplayDialogValue() : mpDisplayTraceDialogFunc(DisplayTraceDialog), mpDisplayTraceDialogContext(NULL) { }
};

#if !EATRACE_GLOBAL_ENABLED
    static EATraceDisplayDialogValue  sDisplayTraceDialogValue;
    static EATraceDisplayDialogValue* gDisplayTraceDialogValue = &sDisplayTraceDialogValue;
#elif defined(EA_INIT_PRIORITY_AVAIALABLE)
    static EA::StdC::AutoStaticOSGlobalPtr<EATraceDisplayDialogValue, 0x0b973acd> gDisplayTraceDialogValue __attribute__ ((init_priority (11000)));
#else
    static EA::StdC::AutoStaticOSGlobalPtr<EATraceDisplayDialogValue, 0x0b973acd> gDisplayTraceDialogValue;
#endif


void SetDisplayTraceDialogFunc(DisplayTraceDialogFunc pDisplayTraceDialogFunc, void* pContext)
{
    gDisplayTraceDialogValue->mpDisplayTraceDialogFunc    = pDisplayTraceDialogFunc;
    gDisplayTraceDialogValue->mpDisplayTraceDialogContext = pContext;
}


DisplayTraceDialogFunc GetDisplayTraceDialogFunc(void*& pContext)
{
    pContext = gDisplayTraceDialogValue->mpDisplayTraceDialogContext;
    return gDisplayTraceDialogValue->mpDisplayTraceDialogFunc;
}


DisplayTraceDialogFunc GetDefaultDisplayTraceDialogFunc(void*& pContext)
{
    pContext = NULL;
    return DisplayTraceDialog;
}


#if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP) || defined(EA_PLATFORM_XENON)

//////////////////////////////////////////////////////////////////////////
// TracerDialogThread
//
// This launches the message box in a separate thread. This avoids problems
// in the past with paint and other windows messages occuring in the main thread,
// and saves having to send a message to the main thread to disable paints.
//////////////////////////////////////////////////////////////////////////
class TracerDialogThread
{
public:
    TracerDialogThread(const char* pTitle, const char* pText);

    // This call is not re-entrant, so make sure to lock a mutex before calling into this.
    EA::Trace::tAlertResult Run();

    static unsigned long __stdcall MessageBoxThreadStart(void* pContext);

private:
    const char* mText;
    const char* mTitle;
    EA::Trace::tAlertResult mAlertResult;
};


TracerDialogThread::TracerDialogThread(const char* pTitle, const char* pText)
    : mTitle(pTitle), mText(pText), mAlertResult(EA::Trace::kAlertResultNone)
{
    // Empty
}


// This call is not re-entrant, so make sure to lock a mutex before calling into this.
EA::Trace::tAlertResult TracerDialogThread::Run()
{
    mAlertResult = EA::Trace::kAlertResultNone;

    HANDLE handle = CreateThread(NULL, 64*1024, MessageBoxThreadStart, this, 0, NULL);

    if(handle)
    {
        for(DWORD dwResult = WAIT_TIMEOUT; dwResult == WAIT_TIMEOUT; )
        {
            dwResult = WaitForSingleObject(handle, 100);

            if(gTraceDialogCallback->mpFunction)
                (*gTraceDialogCallback->mpFunction)(gTraceDialogCallback->mpContext);

            Sleep(100);
        }

        CloseHandle(handle);
    }

    return mAlertResult;
}

unsigned long __stdcall TracerDialogThread::MessageBoxThreadStart(void* pContext)
{
    TracerDialogThread& data = *(TracerDialogThread*)pContext;

    #if defined(EA_PLATFORM_WINDOWS)
        const size_t kBufferSize = 1024;
        char szText[kBufferSize];

        // Potentially truncate the text to fit within our fixed-size buffer.
        // We don't want to overflow our dialogs.
        const size_t nTextLength = EA::StdC::Strlcpy(szText, data.mText, kBufferSize);

        // If there's still enough room left within our text buffer, we append
        // some instructional documentation.
        static const char docs[] = "\n\nSelect Abort to break, Retry to continue, Ignore to remove assert.";
        const size_t kDocsLength = sizeof(docs) / sizeof(docs[0]);
        EA_COMPILETIME_ASSERT(kDocsLength != 0);

        if(nTextLength <= kBufferSize - kDocsLength)
            memcpy(szText + nTextLength, docs, kDocsLength);

        const int nMessageBoxResult = MessageBoxA(NULL, szText, data.mTitle, 
                                                  MB_ABORTRETRYIGNORE | MB_DEFBUTTON2 | // TODO: TOPMOST too ?
                                                  MB_ICONWARNING | MB_TASKMODAL | MB_SETFOREGROUND); 
        switch (nMessageBoxResult)
        {
            case IDABORT:
                data.mAlertResult  =  EA::Trace::kAlertResultBreak;    // Break the execution. 
                break;

            case IDRETRY: default:
                data.mAlertResult  =  EA::Trace::kAlertResultNone;     // Continue the execution.
                break;

            case IDIGNORE:
                data.mAlertResult  =  EA::Trace::kAlertResultDisable; // Continue the execution and don't complain about this again.
                break;  
        }

    #elif defined(EA_PLATFORM_XENON)
        const WCHAR*      pButtonText[3] = { L"Break", L"Continue", L"Ignore" };
        DWORD             dwResult = 0;
        MESSAGEBOX_RESULT result = { 0 };
        WCHAR             title[256];
        WCHAR             text[1024];
        XOVERLAPPED       overlapped;
        ZeroMemory(&overlapped, sizeof(overlapped));

        EA::StdC::Snprintf(title, 256, L"%hs", data.mTitle);
        title[255] = 0;

        EA::StdC::Snprintf(text, 1024, L"%hs", data.mText);
        text[1023] = 0;

        // Attempt to show the message box UI.  We allow for the case where a
        // previous message box may have not completely finished "animating"
        // off of the screen (hence, the retry loop).
        for (int nAttempts = 0; nAttempts < 5; ++nAttempts)
        {
            dwResult = XShowMessageBoxUI(XUSER_INDEX_ANY, // DWORD dwUserIndex,
                                         title,           // LPCWSTR wszTitle,
                                         text,            // LPCWSTR wszText,
                                         3,               // DWORD cButtons,
                                         pButtonText,     // LPCWSTR* pwszButtons,
                                         0,               // DWORD dwFocusButton,
                                         XMB_ALERTICON,   // DWORD dwFlags,
                                         &result,         // PMESSAGEBOX_RESULT pResult,
                                         &overlapped);    // XOVERLAPPED* pOverlapped

            if(dwResult == ERROR_IO_PENDING)
                break;

            // We've received an unexpected result (such as ERROR_ACCESS_DENIED)
            // so we sleep a little bit before retrying.
            Sleep(500);
        }

        // If we've failed to display the message box, give up.
        if(dwResult != ERROR_IO_PENDING)
            return 0;

        // What we need to do is poll the result of this message box via XHasOverlappedIoCompleted(&overlapped).
        // We also need to implement a mechanism whereby the user's main thread polls us or we
        // create a sleeping thread that waits on the overlapped IO result.

        while(!XHasOverlappedIoCompleted(&overlapped))
            Sleep(100);

        dwResult = XGetOverlappedResult(&overlapped, NULL, TRUE);
        (void)dwResult; //dwResult should be equal to ERROR_SUCCESS or ERROR_IO_INCOMPLETE.

        if(result.dwButtonPressed == 0)
            data.mAlertResult  =  EA::Trace::kAlertResultBreak;
        else if(result.dwButtonPressed == 1)
            data.mAlertResult  =  EA::Trace::kAlertResultNone;
        else
            data.mAlertResult  =  EA::Trace::kAlertResultDisable;
    #endif

    return 0;
}

#endif // EA_PLATFORM_WINDOWS || defined(EA_PLATFORM_XENON)


// DisplayTraceDialog
//
// This function primarily gets called in the case that an assertion failure occured.
// It displays an assertion failure dialog on platforms that can support it, and possibly
// breaks execution in the debugger (causes the debugger to stop the app on the current
// line of code). 
//
tAlertResult DisplayTraceDialog(const char* pTitle, const char* pText, void* /*pContext*/)
{
    (void)pTitle; (void)pText;

    if(GetTraceDialogEnabled())
    {
        #if EATRACE_DEBUG_BREAK_ABORT_ENABLED
            return kAlertResultAbort;

        #elif EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP) || defined(EA_PLATFORM_XENON)
            TracerDialogThread tracerDialogThread(pTitle, pText);
            const tAlertResult result = tracerDialogThread.Run();
            return result;

        #elif defined(EA_PLATFORM_XENON)
            #if defined(EA_DEBUG) || defined(_DEBUG)
                // The _CrtDbgReport() implementation offers the user "Break" and "Continue" options.
                // If they "Break", _CrtDbgReport() processes the break itself and doesn't allow us
                // to catch the result.  This precludes us return kAlertResultBreak, which unfortunately
                // means that we can't roll back the debugger's callstack to the source of the trace.
                //
                // Ideally, we'd be able to use _CrtDbgReportT(), but that symbol doesn't appear to be
                // available / exported in the current XDK.
                static bool bAlwaysBreak = false;

                if(bAlwaysBreak)
                {
                     // Don't post a message box; just break into debugger at code location.
                     return kAlertResultBreak;
                }
                else
                {
                    _CrtDbgReport(_CRT_ERROR, NULL, 0, NULL, "%s", pText);
                    return kAlertResultNone;
                }
            #else
                return kAlertResultBreak;
            #endif

        #else
            return kAlertResultBreak;
        #endif
    }

    // If dialogs are disabled then we break, as otherwise there would be no way to notify the user of the event.
    return kAlertResultBreak;
}



//////////////////////////////////////////////////////////////////////////
// Tracer
//////////////////////////////////////////////////////////////////////////

Tracer::Tracer()
  : Trace::ZoneObject(),
    mbIsReporting(false),
   #if EATRACE_FLUSH_ON_WRITE_ENABLED_BY_DEFAULT
    mbFlushOnWrite(true),
   #else
    mbFlushOnWrite(false),
   #endif
    mnRefCount(0),
    mMutex()
{
    // Empty
}


Tracer::~Tracer()
{
    // assert(mnRefCount == 0);
}


int Tracer::AddRef()
{
    return mnRefCount.Increment();
}


int Tracer::Release()
{
    int32_t rc = mnRefCount.Decrement();
    if(rc)
        return rc;
    mnRefCount.SetValue(1); // Prevent double destroy if AddRef/Release
    FinalRelease();         // is called while in destructor. This can happen
    return 0;               // if the destructor does things like debug traces.
}

void* Tracer::AsInterface(InterfaceId iid)
{
    switch((InterfaceIdInt)iid)
    {
        case IUnknown32::kIID: // As of this writing the 32 bit IUnknown32 GUID doesn't have its own name.
            return static_cast<IUnknown32*>(static_cast<ITracer*>(this));

        case ITracer::kIID:
            return static_cast<ITracer*>(this);
    }

    return NULL;
}


void Tracer::FinalRelease()
{
    // We normally put shutdown code here.
    // You can subclass this function and add your own shutdown code to it.
    delete this;    // Tracer is a ZoneObject-deriving class
}


tAlertResult Tracer::TraceV(const TraceHelper& helper, const char* pFormat, va_list argList)
{
    int nReturnValue = kAlertResultNone;

    if(pFormat)
    {
        Thread::AutoMutex auto_mutex(mMutex);

        if(!mbIsReporting) // If not in this function re-entrantly...
        {
            const int32_t nIntendedStrlen = EA::StdC::Vsnprintf(mTraceBuffer, kTraceBufferSize, pFormat, argList);
            if((nIntendedStrlen >= 0) && (nIntendedStrlen < kTraceBufferSize)) // If the input buffer to vsnprintf above was sufficiently large.
                nReturnValue = Trace(helper, mTraceBuffer);
        }
    }

    return nReturnValue;
}


tAlertResult Tracer::Trace(const TraceHelper& helper, const char* pText)
{
    int alertResult = kAlertResultNone;

    if(pText)
    {
        Thread::AutoMutex auto_mutex(mMutex);

        if(!mbIsReporting)
        {
            // This guards against re-entrancy into this code, especially to avoid re-used of shared mTraceBuffer.
            mbIsReporting = true;
         
            tOutputType outputType = helper.GetOutputType();

            #if defined(EA_PLATFORM_MICROSOFT)
                if(outputType & kOutputTypeText)
                {
                    // Done: Problems with multiple threads hitting this code, OutputDebugString will be out of order
                    //       code had relied on log system to cause a lock, and then call back into this, but
                    //       the log code goes out to reporters with a record now, and not back into the tracer.
                    // ToDo: There is a limit to how much pText (32K bytes) can be funneled through ODS in a single call
                    //       this impl will lose the additional data
                    // ToDo: Consolidate OutputDebugString calls using temp buffer (mTraceBuffer?)
                    // Done: Make sure last character is always a newline
                    OutputDebugStringA(pText);
                    
                    const size_t textLength = strlen(pText);
                    if(textLength && (pText[textLength-1] != '\n'))
                        OutputDebugStringA("\n");

                    // Default tracer outputs the file/line number if level warrants it
                    if(helper.GetLevel() >= kLevelWarn)
                    {
                        const ReportingLocation* const loc = helper.GetReportingLocation();
                        
                        char8_t buffer[256];
                        EA::StdC::Snprintf(buffer, 256, "%s(%d): %s\n", loc->GetFilename(), loc->GetLine(), loc->GetFunction());
                        buffer[256-1] = '\0';

                        OutputDebugStringA(buffer);
                    }
                }

            #else

                if(outputType & kOutputTypeText) 
                {
                    // ToDo: coalesce these into fewer calls, just want it to match OutputDebugString for now.
                    EA::StdC::Printf("%s", pText);

                    const size_t textLength = strlen(pText);
                    if(textLength && (pText[textLength-1] != '\n'))
                        EA::StdC::Printf("\n");

                    // Default tracer outputs the file/line number if level warrants it
                    if(helper.GetLevel() >= kLevelWarn)
                    {
                        const ReportingLocation* const loc = helper.GetReportingLocation();
                        EA::StdC::Printf("%s(%d): %s\n", loc->GetFilename(), loc->GetLine(), loc->GetFunction());
                    }

                    if(mbFlushOnWrite)
                        fflush(stdout);
                }
            #endif

            // handle dialogs
            if(outputType & kOutputTypeAlert)
                alertResult = gDisplayTraceDialogValue->mpDisplayTraceDialogFunc("Alert", pText, gDisplayTraceDialogValue->mpDisplayTraceDialogContext);
        }
    }

    mbIsReporting = false;

    return alertResult;
}


bool Tracer::IsFiltered(const TraceHelper& /*helper*/) const
{
    return false;
}


void Tracer::SetFlushOnWrite(bool flushOnWrite)
{
    mbFlushOnWrite = flushOnWrite;
}

        
bool Tracer::GetFlushOnWrite() const
{
    return mbFlushOnWrite;
}



//////////////////////////////////////////////////////////////////////////
// TraceHelperTable
//////////////////////////////////////////////////////////////////////////

TraceHelperTable::TraceHelperTable(Allocator::ICoreAllocator* pAllocator)
  : ITraceHelperTable(), 
    Trace::ZoneObject(),
    mTracer(NULL),
    mTraceHelperTable(Allocator::EASTLICoreAllocator(EATRACE_ALLOC_PREFIX "TraceHelperTable", 
        pAllocator ? pAllocator : EA::Trace::GetAllocator())),
    mMutex(),
    mnRefCount(0)
{
    // This is disabled because it results in memory being allocated before
    // the application main() is encountered, and that violates our policies.
    // mTraceHelperTable.reserve(4 * 1024);
}


TraceHelperTable::TraceHelperTable(const TraceHelperTable& x)
  : ITraceHelperTable(), 
    Trace::ZoneObject(),
    mTracer(NULL),
    mTraceHelperTable(Allocator::EASTLICoreAllocator(EATRACE_ALLOC_PREFIX "TraceHelperTable", 
                        EA::Trace::GetAllocator())),
    mMutex(),
    mnRefCount(0)
{ 
    mTraceHelperTable = x.mTraceHelperTable;
    SetAllocator(x.GetAllocator());
}


TraceHelperTable::~TraceHelperTable()
{
    // Empty
}


void TraceHelperTable::SetAllocator(Allocator::ICoreAllocator* pAllocator)
{
    mTraceHelperTable.get_allocator().set_allocator(pAllocator); // Get the EASTL allocator, and with that allocator set its ICoreAllocator.
}


Allocator::ICoreAllocator* TraceHelperTable::GetAllocator() const
{
    return const_cast<tTraceHelperTable&>(mTraceHelperTable).get_allocator().get_allocator();    // Get the EASTL allocator, and with that allocator get its ICoreAllocator.
}


int TraceHelperTable::AddRef()
{
    return mnRefCount.Increment();
}


int TraceHelperTable::Release()
{
    int32_t rc = mnRefCount.Decrement();
    if(rc)
        return rc;

    // Prevent double-destroy if AddRef/Release is called while 
    // in destructor. This can happenif the destructor does things 
    // like debug traces.
    mnRefCount.SetValue(1); 
    
    delete this;    // TracerHelperTable is a ZoneObject-deriving class
    return 0;               
}


void* TraceHelperTable::AsInterface(InterfaceId iid)
{
    switch((InterfaceIdInt)iid)
    {
        case IUnknown32::kIID: // As of this writing the 32 bit IUnknown32 GUID doesn't have its own name.
            return static_cast<IUnknown32*>(static_cast<ITraceHelperTable*>(this));

        case ITraceHelperTable::kIID:
            return static_cast<ITraceHelperTable*>(this);
    }

    return NULL;
}

// This is called by the tracer to update the tracer and filtering. 
// Updates all existing tracers.  Call with same ptr if config change.
void TraceHelperTable::UpdateTracer()
{
    UpdateTracer(mTracer);
}


void TraceHelperTable::UpdateTracer(ITracer* tracer)
{
    Thread::AutoMutex auto_mutex(mMutex);

    mTracer = tracer;
    for (tTraceHelperTable::iterator it = mTraceHelperTable.begin(), itEnd(mTraceHelperTable.end()); it != itEnd; ++it)
    {
        TraceHelper* helper = (*it);
        helper->UpdateTracer(mTracer);
    }
}

void TraceHelperTable::SetAllEnabled(bool enabled)
{
    Thread::AutoMutex auto_mutex(mMutex);

    for (tTraceHelperTable::iterator it = mTraceHelperTable.begin(), itEnd(mTraceHelperTable.end()); it != itEnd; ++it)
    {
        TraceHelper* helper = (*it);
        helper->SetEnabled(enabled);
    }
}

// each individual helper registers with the table
void TraceHelperTable::RegisterHelper(TraceHelper& helper)
{
    Thread::AutoMutex auto_mutex(mMutex);

    helper.UpdateTracer(mTracer);

    if(mTraceHelperTable.empty()) // If this is the first time we are being called...
        mTraceHelperTable.reserve(4 * 1024);

    mTraceHelperTable.push_back(&helper);
}


void TraceHelperTable::UnregisterHelper(TraceHelper& helper)
{
    Thread::AutoMutex auto_mutex(mMutex);

    helper.UpdateTracer(NULL);
    
    // may not want to erase because ptrs shift down, just mark as NULL ?
    //  need to cleanup before next update because the dll may have gone away.
    tTraceHelperTable::iterator iter = eastl::find(mTraceHelperTable.begin(), mTraceHelperTable.end(), &helper);
    if(iter != mTraceHelperTable.end())
        mTraceHelperTable.erase(iter);
}


TraceHelper* TraceHelperTable::ReserveHelper(TraceType traceType, const char8_t* groupName, tLevel level, const ReportingLocation& reportingLocation)
{
    Thread::AutoMutex auto_mutex(mMutex);

    // safely assume that trace helper is unique, external map should ensure this

    // this automatically registers the helper (which dll heap is this allocation in?)
    //   the tracer lives for a long time, so have to release these before that dll goes away
    TraceHelper* helper = EA_TRACE_CA_NEW(TraceHelper, EA::Trace::GetAllocator(),
                                            EATRACE_ALLOC_PREFIX "TraceHelper")(traceType, groupName, level, reportingLocation);
    
    // To consider: Set the id into the helper itself, also matchup co-id?
    // tTraceHelperId id = (tTraceHelperId)mTraceHelperTable.size();
    // return id;

    return helper; 
}


//////////////////////////////////////////////////////////////////////////
// TempTraceHelperMap
//////////////////////////////////////////////////////////////////////////

TempTraceHelperMap::TempTraceHelperMap(Allocator::ICoreAllocator* pAllocator)
    : mFileToHelperMap(Allocator::EASTLICoreAllocator(EATRACE_ALLOC_PREFIX "TempTraceHelperMap", 
                        pAllocator ? pAllocator : EA::Trace::GetAllocator()))
    , mIsEnabled(true)
{
}

TempTraceHelperMap::~TempTraceHelperMap()
{
    ReleaseHelpers();
}


void TempTraceHelperMap::SetAllocator(Allocator::ICoreAllocator* pAllocator)
{
    mFileToHelperMap.get_allocator().set_allocator(pAllocator); // Get the EASTL allocator, and with that allocator set its ICoreAllocator.
}


Allocator::ICoreAllocator* TempTraceHelperMap::GetAllocator() const
{
    return const_cast<tFileToHelperMap&>(mFileToHelperMap).get_allocator().get_allocator();    // Get the EASTL allocator, and with that allocator get its ICoreAllocator.
}


void TempTraceHelperMap::ReleaseHelpers()
{
    EA::Thread::AutoMutex auto_mutex(mMutex);

    // delete all of the temp helpers
    for (tFileToHelperMap::iterator it = mFileToHelperMap.begin(), itEnd = mFileToHelperMap.end(); it != itEnd; ++it)
    {    
        TraceHelper* const helper = (*it).second;
        EA_TRACE_CA_DELETE(helper, EA::Trace::GetAllocator());
    }
    mFileToHelperMap.clear();

    mIsEnabled = false;
}


TraceHelper* TempTraceHelperMap::ReserveHelper(TraceType traceType, const char8_t* groupName, tLevel level, const ReportingLocation& reportingLocation)
{
    EA::Thread::AutoMutex auto_mutex(mMutex);

    TraceHelper* helper = NULL;

    // Cannot reserve helpers once table shutsdown (on first release call).
    if(mIsEnabled)
    {
        // Note that this code copies the reporting location into the heap-based helper
        //   file/function name are still aliased to minimize heap memory allocation

        // lookup file/line
        tFileToHelperMap::iterator iter = mFileToHelperMap.find(reportingLocation);

        if(iter != mFileToHelperMap.end())
        {
            helper = (*iter).second;
        }
        else
        {
            ITraceHelperTable* const helperTable = GetTraceHelperTable();

            // table creates the heap helper, but our map holds onto it
            if(helperTable)
            {
                helper = helperTable->ReserveHelper(traceType, groupName, level, reportingLocation);
                mFileToHelperMap[*helper->GetReportingLocation()] = helper;
            }
        }
    }

    return helper;
}






//////////////////////////////////////////////////////////////////////////
// TraceHelper
//////////////////////////////////////////////////////////////////////////

bool TraceHelper::sTracingEnabled = true;

TraceHelper::TraceHelper(TraceType traceType, const char8_t* groupName, 
                         tLevel level, const ReportingLocation& reportingLocation)
  : mIsEnabled(true),
    mIsFiltered(true), 
    mIsFilterEvaluated(false),
    mTraceType(traceType),
    mOutputType(kOutputTypeNone),
    mLevel(level),
    mGroupName(groupName), 
    mReportingLocation(reportingLocation),
    mTracer(NULL)
{
    // this defers output and level decisions into the C++ code
    switch(traceType)
    {
        case kTraceTypeAssert:
            if(mGroupName == NULL)
                mGroupName = "Assert";
            mOutputType = kOutputTypeText | kOutputTypeAlert;
            if(mLevel == kLevelUndefined)
                mLevel = kLevelError;
            break;

        case kTraceTypeVerify:
            if(mGroupName == NULL)
                mGroupName = "Verify";
            mOutputType = kOutputTypeText | kOutputTypeAlert;
            if(mLevel == kLevelUndefined)
                mLevel = kLevelError;
            break;

        case kTraceTypeTrace:
            mOutputType = kOutputTypeText;
            if(mLevel == kLevelUndefined)
                mLevel = kLevelDebug;
            break;

        case kTraceTypeFail:
            if(mGroupName == NULL)
                mGroupName = "Fail";
            mOutputType = kOutputTypeText | kOutputTypeAlert;
            mLevel = kLevelError;
            break;

        case kTraceTypeLog:
        default:
            // Honor the group, level
            mOutputType = kOutputTypeText;
            break;
    }

    // fix up the group name if none
    if(mGroupName == NULL)
        mGroupName = "<Unknown>";

    // register with the helper table, tracer will update this table during reconfig
    ITraceHelperTable* const pTable = GetTraceHelperTable();
    if(pTable)
    {
        pTable->RegisterHelper(*this);
    }
    else
    {
        // disable this helper
        mIsEnabled         = false;
        mIsFilterEvaluated = true;
    }
}


TraceHelper::~TraceHelper()
{
    ITraceHelperTable* const pTable = GetTraceHelperTable();
    if(pTable)
        pTable->UnregisterHelper(*this);
}


bool TraceHelper::IsTracing()
{
    if(!mIsFilterEvaluated)
    {
        // If you are getting no trace output and mTracer is NULL, it could be because 
        // EA::Trace::SetTracer(ITracer* pTracer) was not called on startup.
        ITracer* const pTracer = mTracer;
        
        mIsFiltered = (pTracer == NULL) || pTracer->IsFiltered(*this); 
        mIsFilterEvaluated = true;
    }

    return mIsEnabled && !mIsFiltered; 
}


// This function is intentionally not inlined because some users want to use EATrace as a header 
// file and manually provide their own implementations of some of the functions, in particular 
// these two. Their reasons for doing this are to allow a non-DLL savvy library (such as this)
// be in a DLL; the mechanism for this isn't very important; what's important is that the user 
// be able to override all functions in this library. These two were a problem because they 
// reference sTracingEnabled, and sTracingEnabled needs to come from the DLL and not the app.
// Making the usage of sTracingEnabled be in this .cpp file (which the app replaces) allows this.
// This mostly isn't a performance issue because this function isn't called by our ASSERT macros 
// unless the assertion expression has failed, which is uncommon. The primary cost is a small 
// increase in code generated at the ASSERT macro invocation sites.
// An alternative solution to all this may be to make EATrace be fully DLL-savvy and have the 
// app use it as such.
bool TraceHelper::GetTracingEnabled()
{
    return sTracingEnabled;
}


void TraceHelper::SetTracingEnabled(bool bEnabled)
{
    sTracingEnabled = bEnabled;
}


// do not inline these, only one path is taken per macro instantiation
//  but multiple threads may take that same path
tAlertResult TraceHelper::Trace(const char8_t* pText)
{
    // get a copy of tracer in case an UpdateTracer call happens
    ITracer* const tracer = mTracer;
    if(tracer)
    {
        // output the pText
        const tAlertResult result = tracer->Trace(*this, pText);
         
        if(result & kAlertResultDisable)
            mIsEnabled = false;

        return result;
    }
    return kAlertResultNone;
}


tAlertResult TraceHelper::TraceFormatted(const char8_t* pFormat, ...)
{
    va_list argList;
    va_start(argList, pFormat);
    tAlertResult result = TraceFormattedV(pFormat, argList);
    va_end(argList);

    return result;
}


tAlertResult TraceHelper::TraceFormattedV(const char8_t* pFormat, va_list argList)
{
    // get a copy of tracer in case an UpdateTracer call happens
    ITracer* const tracer = mTracer;
    if(tracer)
    {
        // ouput the pText
        tAlertResult result = tracer->TraceV(*this, pFormat, argList);

        if(result & kAlertResultDisable)
            mIsEnabled = false;

        return result;
    }
    return kAlertResultNone;
}


void TraceHelper::UpdateTracer(ITracer* tracer)
{
    mTracer            = tracer; // if passed null, may be better to just mark mIsFilterEvaluated to false in case tracer in use ?  
    mIsFiltered        = true;
    mIsFilterEvaluated = false;  // better approach ?, wait for helper to update it's own filter on it's next call
}


void TraceHelper::SetGroupNameAndLevel(const char8_t* groupName, tLevel level)
{
    // This routine is solely for dynamic groups/levels.  These should be
    // be avoided for performance, but can occur in an interpreter that can log.
    // In that scenario, lock a mutex, contruct a static helper, use this call, 
    // call IsFiltered() and then call trace.
    // Caller can check to see if the group name/level has changed, and 
    // avoid calling this if so.

    mIsFilterEvaluated = false;
    mGroupName         = groupName;
    mLevel             = level;
}

} // namespace Trace

} // namespace EA




