///////////////////////////////////////////////////////////////////////////////
// ExceptionHandler.cpp
// 
// Copyright (c) 2005, Electronic Arts. All Rights Reserved.
// Written and maintained by Vasyl Tsvirkunov and Paul Pedriana.
//
// Exception handling and reporting facilities.
///////////////////////////////////////////////////////////////////////////////


#include <ExceptionHandler/ExceptionHandler.h>
#include <EACallstack/Context.h>
#include <EACallstack/EAAddressRep.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EAProcess.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EAGlobal.h>
#include <EASTL/algorithm.h>
#include <EASTL/fixed_string.h>
#include <EAIO/PathString.h>
#include <EAIO/EAFileUtil.h>
#include <eathread/eathread.h>
#include <eathread/eathread_thread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include EA_ASSERT_HEADER

// Need to include proper platform implementation. If a header does not 
// exist, that means that the platform is not supported yet.
#if defined(EA_PLATFORM_XENON)
    #include "Xenon/ExceptionHandlerXenon.cpp"
    #define REAL_HANDLER static_cast<ExceptionHandlerXenon*>(mpPlatformHandler)

#elif defined(EA_PLATFORM_MICROSOFT)
    #if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0400)
        #undef  _WIN32_WINNT
        #define _WIN32_WINNT 0x0400
    #endif
    #pragma warning(push, 0)
    #if !defined(EA_PLATFORM_WINDOWS_PHONE) // Windows phone requires you to use WinRT-based socket API.
        #include <winsock2.h>   // You should always #include <winsock2.h> before <Windows.h>, otherwise Winsock2 will be broken.
        #include <ws2tcpip.h>
    #endif
    #include <Windows.h>
    #pragma warning(pop)
    #include "Win32/ExceptionHandlerWin32.cpp"
    #define REAL_HANDLER static_cast<ExceptionHandlerWin32*>(mpPlatformHandler)

#elif defined(EA_PLATFORM_PS3)
    #include <libsn.h>
    #include "PS3/ExceptionHandlerPS3.cpp"
    #define REAL_HANDLER static_cast<ExceptionHandlerPS3*>(mpPlatformHandler)

#elif defined(EA_PLATFORM_PSP2)
    #include <sdk_version.h>
    #include "PSP2/ExceptionHandlerPSP2.cpp"
    #define REAL_HANDLER static_cast<ExceptionHandlerPSP2*>(mpPlatformHandler)

#elif defined(EA_PLATFORM_REVOLUTION)
    #include <revolution\db.h>
    #include "Wii/ExceptionHandlerWii.cpp"
    #define REAL_HANDLER static_cast<ExceptionHandlerWii*>(mpPlatformHandler)

#elif defined(EA_PLATFORM_APPLE)
    #include "Apple/ExceptionHandlerApple.cpp"
    #define REAL_HANDLER static_cast<ExceptionHandlerApple*>(mpPlatformHandler)

#elif defined(EA_PLATFORM_UNIX)
    #include "Unix/ExceptionHandlerUnix.cpp"
    #define REAL_HANDLER static_cast<ExceptionHandlerUnix*>(mpPlatformHandler)

#else
    #include <time.h>

    namespace EA
    {
        namespace Debug
        {
            struct DummyHandler
            {
                ExceptionHandler::ExceptionInfo mExceptionInfo;

                bool SetEnabled(bool /*state*/)
                    { return false; }
                    
                bool IsEnabled() const
                    { return false; }
                    
                bool SetMode(EA::Debug::ExceptionHandler::Mode)
                    { return false; }

                EA::Debug::ExceptionHandler::Action GetAction(int* /*pReturnValue*/ = NULL) const
                    { return EA::Debug::ExceptionHandler::kActionContinue; }
                    
                void SetAction(EA::Debug::ExceptionHandler::Action /*action*/, int /*returnValue*/)
                    { }
                    
                void SimulateExceptionHandling(int /*exceptionType*/)
                    { }

                intptr_t RunTrapped(ExceptionHandler::UserFunctionUnion userFunctionUnion, ExceptionHandler::UserFunctionType userFunctionType, void* pContext, bool& exceptionCaught)
                    { exceptionCaught = false; return ExceptionHandler::ExecuteUserFunction(userFunctionUnion, userFunctionType, pContext); }

                const ExceptionHandler::ExceptionInfo* GetExceptionInfo() const
                    { return &mExceptionInfo; }

                // Deprecated functions:
                size_t GetExceptionCallstack(void* pCallstackArray, size_t capacity) const
                    { if(capacity > mExceptionInfo.mCallstackEntryCount) capacity = mExceptionInfo.mCallstackEntryCount; memmove(pCallstackArray, mExceptionInfo.mCallstack, capacity * sizeof(void*)); return capacity; }

                const EA::Callstack::Context* GetExceptionContext() const
                    { return &mExceptionInfo.mContext; }
                    
                EA::Thread::ThreadId GetExceptionThreadId(EA::Thread::SysThreadId& sysThreadId, char* threadName, size_t threadNameCapacity) const
                    { sysThreadId = mExceptionInfo.mSysThreadId; EA::StdC::Strlcpy(threadName, mExceptionInfo.mThreadName, threadNameCapacity); return mExceptionInfo.mThreadId; }
                    
                size_t GetExceptionDescription(char* buffer, size_t capacity)
                    { return EA::StdC::Strlcpy(buffer, mExceptionInfo.mExceptionDescription.c_str(), capacity); }

            };
        }
    }

    #define REAL_HANDLER static_cast<DummyHandler*>(mpPlatformHandler)
#endif

#if defined(__APPLE__)    // OS X, iPhone, iPad, etc.
    #include <stdbool.h>
    #include <sys/types.h>
    #include <unistd.h>
    #include <sys/sysctl.h>
#elif defined(EA_PLATFORM_UNIX) && !defined(__CYGWIN__)
    #include <sys/ptrace.h>
#elif defined(EA_COMPILER_MSVC) && defined(EA_PLATFORM_XENON)
    #include <ppcintrinsics.h>
#elif defined(EA_COMPILER_MSVC) && defined(EA_PLATFORM_MICROSOFT)
    #pragma warning(disable: 4985) // 'ceil': attributes not present on previous declaration.
    #include <math.h>              // VS2008 has an acknowledged bug that requires math.h to be #included before intrin.h.
    #include <intrin.h>
    #pragma warning(disable: 4530) // C++ exception handler used, but unwind semantics are not enabled.
    #pragma warning(disable: 4723) // potential divide by 0.
#endif



namespace EA
{
namespace Debug
{
    EA::StdC::AutoStaticOSGlobalPtr<ExceptionHandler*, 0x0fe2c3bb> gExceptionHandler;

    ExceptionHandler* GetDefaultExceptionHandler()
    {
        return *gExceptionHandler.get(); // get returns a pointer to the OSGlobal, so we need to dereference it.
    }

    void SetDefaultExceptionHandler(ExceptionHandler* pExceptionHandler)
    {
        *gExceptionHandler.get() = pExceptionHandler;
    }


    ///////////////////////////////////////////////////////////////////////////////
    // GetThreadName
    //
    // Returns the required strlen of the thread name.
    // To do: Move this or a generic variation of it to EAThread.
    //
    size_t GetThreadName(const EA::Thread::ThreadId& threadId, const EA::Thread::SysThreadId& sysThreadId, 
                          char* threadName, size_t threadNameCapacity)
    {
        size_t requiredStrlen = 0;

        if(threadNameCapacity)
            threadName[0] = 0; 

        #if (EATHREAD_VERSION_N >= 12006) // If EA::Thread::EnumerateThreads is available...
            EA_UNUSED(threadId);

            EA::Thread::ThreadEnumData enumData[32];
            size_t count = EA::Thread::EnumerateThreads(enumData, EAArrayCount(enumData));

            for(size_t i = 0; (i < count) && !threadName[0]; i++)
            {
                // We have a problem here in that there isn't a consistent way to get a ThreadId from ThreadDynamicData. It currently differs per platform.
                #if EA_USE_CPP11_CONCURRENCY
                    EA::Thread::ThreadId threadIdTemp = enumData[i].mpThreadDynamicData->mThread.get_id();
                #elif defined(EA_PLATFORM_MICROSOFT) && !EA_POSIX_THREADS_AVAILABLE
                    EA::Thread::ThreadId threadIdTemp = enumData[i].mpThreadDynamicData->mhThread;
                #else
                    EA::Thread::ThreadId threadIdTemp = enumData[i].mpThreadDynamicData->mThreadId;
                #endif

                if(threadIdTemp != EA::Thread::kThreadIdInvalid)
                {
                    EA::Thread::SysThreadId sysThreadIdTemp = EA::Thread::GetSysThreadId(threadIdTemp);

                    if(sysThreadIdTemp == sysThreadId)
                        requiredStrlen = EA::StdC::Strlcpy(threadName, enumData[i].mpThreadDynamicData->mName, threadNameCapacity);
                }

                enumData[i].Release();
            }
        #else
            // Windows: It's impossible to ask Windows what the thread name is, as only the debugger stores it. 
            // Unix: http://stackoverflow.com/questions/2369738/can-i-set-the-name-of-a-thread-in-pthreads-linux
            EA_UNUSED(threadId);
            EA_UNUSED(sysThreadId);
            EA_UNUSED(threadNameCapacity);
        #endif

        return requiredStrlen;
    }


    namespace Internal
    {
        const char* kMinidumpFileNameExtension = ".mdmp";

        ///////////////////////////////////////////////////////////////////////////////
        // CreateExceptionImpl
        //
        static void CreateExceptionImpl(ExceptionType exceptionType)
        {
            char buffer[1024]; buffer[0] = 0;

            switch(exceptionType)
            {
                case kExceptionTypeAccessViolation:
                {
                    char* const pData = (char*)(uintptr_t)0;
                    pData[0] = 0;
                    sprintf(buffer, buffer, pData); // Force compilation.
                    break;
                }

                case kExceptionTypeIllegalInstruction:
                {
                    #if defined(EA_PROCESSOR_X86)
                        #if defined(EA_ASM_STYLE_INTEL)
                            // The x86 ud2 instruction is the "undefined instruction".
                            // Its purpose is to test invalid opcode handling.
                            // The opcode for ud2 is 0F 0B, in case your assembler doesn't understand ud2.
                            __asm ud2
                        #else
                            asm volatile("ud2");
                        #endif
                    #elif defined(EA_PROCESSOR_POWERPC)
                        #if defined(EA_ASM_STYLE_INTEL)
                            __emit(0x00000000);
                        #endif
                    #elif defined(EA_PROCESSOR_X86_64) && defined(EA_PLATFORM_MICROSOFT) && defined(PAGE_EXECUTE_READWRITE)
                        // VC++ for x64 doesn't support inline assembly, so we can't use that to write invalid code.
                        // One solution involves using VirtualProtect with PAGE_EXECUTE_READWRITE to enable writing 
                        // to a page of code, but Windows security may balk at this on some computers.
                        // Another solution is to write a separate .asm file which has a function we call.

                        void*   pVoid          = _AddressOfReturnAddress();
                        void**  ppVoid         = reinterpret_cast<void**>(pVoid);
                        void*   pReturnAddress = *ppVoid;
                        DWORD   dwOldProtect   = 0;

                        if(VirtualProtect(pReturnAddress, 2, PAGE_EXECUTE_READWRITE, &dwOldProtect))
                        {
                            // Modify the code we return to.
                            unsigned char instructions[] = { 0x0F, 0x0B }; // __asm ud2 (undefined instruction)
                            memcpy(pReturnAddress, instructions, sizeof(instructions));
                            VirtualProtect(pReturnAddress, 2, dwOldProtect, &dwOldProtect);
                        }
                        else
                        {
                            // Until we figure out how to do the above reliably, generate an access violation exception.
                            CreateExceptionImpl(kExceptionTypeAccessViolation);
                        }

                    #else
                        // To do: Implement this. In the meantime generate a different exception.
                        CreateExceptionImpl(kExceptionTypeAccessViolation);
                    #endif
                    break;
                }

                case kExceptionTypeDivideByZero:
                {
                    int x(1);
                    int y(exceptionType);
                    int z(7 / (x / y));
                    sprintf(buffer, buffer, x, y, z);  // Force compilation.
                    break;
                }

                case kExceptionTypeStackOverflow:
                {
                    CreateExceptionImpl(exceptionType);     // Call ourselves recursively.
                    int x(1);
                    int y(exceptionType);
                    int z(7 / (x / y));
                    sprintf(buffer, buffer, x, y, z);  // Force compilation.
                    break;
                }

                case kExceptionTypeStackCorruption:
                {
                    size_t size         = (sizeof(buffer) * 8) - (rand() % 8); // "- exceptionType" in order to prevent compiler warnings.
                    char*  pBadLocation = buffer - ((sizeof(buffer) * 8) + (rand() % 8));

                    memset(buffer,       0, size); 
                    memset(pBadLocation, 0, size);
                    break;
                }

                case kExceptionTypeAlignment:
                {
                    // Not all platforms generate alignment exceptions. Some internally handle it.
                    // For example, the PS3 PowerPC will generate an alignment exception for the code below, while x86 won't.
                    // It's hard to generate code which will misalign a uint64_t with some compilers because they give
                    // compiler warnings about violating strict type aliasing.
                    void*     pNew     = malloc(16);
                    char*     pNewChar = (char*)pNew + 1;
                    uint64_t* pNew64   = (uint64_t*)pNewChar;
                    *pNew64 = 1;  // This should generate an alignment exception on platforms where it's possible and enabled.
                    free(pNew);
                    // If the above didn't throw an exeption, generate an access violation.
                    CreateExceptionImpl(kExceptionTypeAccessViolation);
                    break;
                }

                case kExceptionTypeOther:
                case kExceptionTypeNone:
                case kExceptionTypeTrap:
                case kExceptionTypeFPU:
                default:
                    break;
            }
        }

    } // namespace Internal



///////////////////////////////////////////////////////////////////////////////
// IsDebuggerPresent
//
bool ExceptionHandler::IsDebuggerPresent()
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
        sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);

        // We're being debugged if the P_TRACED flag is set.
        return ((info.kp_proc.p_flag & P_TRACED) != 0);

    #elif (defined(EA_PLATFORM_UNIX) && defined(PT_TRACE_ME) && !defined(__CYGWIN__) && !defined(EA_PLATFORM_ANDROID) && !defined(EA_PLATFORM_PALM))
        // See http://www.phiral.net/other/linux-anti-debugging.txt and http://linux.die.net/man/2/ptrace
        return (ptrace(PT_TRACE_ME, 0, 1, 0) < 0); // A synonym for this is PTRACE_TRACEME.

    #else
        return false;

    #endif
}


///////////////////////////////////////////////////////////////////////////////
// CreateException
//
void CreateException(ExceptionType exceptionType, ExceptionHandler* pExceptionHandler)
{
    bool bDebuggerPresent(EA::Debug::ExceptionHandler::IsDebuggerPresent());
    bool bReleaseBuild(true);

    #if EA_EXCEPTIONHANDLER_DEBUG
        bReleaseBuild = false;
    #endif

    // In a PC release build, we are likely running under SafeDisc and so we just 
    // go straight to creating the exception. SafeDisc acts as a debugger but 
    // doesn't let the code below act as desired.
    if(bReleaseBuild || !bDebuggerPresent || !pExceptionHandler)
        Internal::CreateExceptionImpl(exceptionType);
    else
    {
        // Note by Paul Pedriana:
        // Under Win32, we do a __try/__except pair instead of just executing the
        // crashing code below. The reason for this is that if we just executed
        // the crashing code then it would be hard to debug this system because
        // the VC++ debugger will not call your exception handling routime when
        // the debugger is active. Instead, the VC++ debugger just stops execution
        // at the point of the exception and won't continue. This is not what we
        // want during testing. Using __try/__except and then directly calling the
        // ProcessExceptionHandler handling function below lets us run under the
        // debugger and be able to debug the code.

        #ifndef EA_COMPILER_NO_EXCEPTIONS
            #if EA_EXCEPTIONHANDLER_DEBUG && defined(EA_PLATFORM_MICROSOFT)
                    __try {
            #else
                    try {
            #endif
        #endif

        Internal::CreateExceptionImpl(exceptionType);

        #ifndef EA_COMPILER_NO_EXCEPTIONS
            #if EA_EXCEPTIONHANDLER_DEBUG && defined(EA_PLATFORM_XENON)
                    }
                    __except(pExceptionHandler->GetPlatformHandler<ExceptionHandlerXenon*>()->ExceptionFilter(GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER) {
                        // Do nothing.
                    }
            #elif EA_EXCEPTIONHANDLER_DEBUG && defined(EA_PLATFORM_MICROSOFT)
                    }
                    __except(pExceptionHandler->GetPlatformHandler<ExceptionHandlerWin32*>()->ExceptionFilter(GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER) {
                        // Do nothing.
                    }
            #else
                    }
                    catch(...) {
                        // Do nothing.
                    }
            #endif
        #endif
    }
}




///////////////////////////////////////////////////////////////////////////////
// ExceptionHandler
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// ExceptionHandler
//
ExceptionHandler::ExceptionHandler() :
    mpCoreAllocator(EA::Callstack::GetAllocator()),
    mpPlatformHandler(NULL),
    mReportWriterDefault(),
    mpReportWriter(&mReportWriterDefault),
    mReportTypes(kReportTypeAll),
    mReportFields(kReportFieldAll),
    mCallstackAddressRepTypeFlags(Callstack::kARTFlagFileLine | Callstack::kARTFlagFunctionOffset | Callstack::kARTFlagAddress),
    mReportNameSpaceChar(' '),
    mMiniDumpFlags(0), // Note that 0 is the same as Microsoft's MiniDumpNormal enum.
    mbRunningTrapped(false),
    mHandlerList(),
    msBuildDescription()
  //mpReportFileName[]
  //mpReportDirectory[]
  //mBaseReportName[]
  //mReportFilePath[]
{
    mpReportFileName[0]  = 0;
    mpReportDirectory[0] = 0;
    mBaseReportName[0]   = 0;
    mReportFilePath[0]   = 0;

    // To do: Use EAIO GetSpecialDirectory or GetTempFileDirectory instead of the per-platform calls below. Need to make sure EAIO does what we want first.
    EA::StdC::Strlcpy(mBaseReportName, "xcpt", EAArrayCount(mBaseReportName)); // We use a short phrase because the file system supports limited file name lengths.

    #if defined(EA_PLATFORM_XENON)
        EA::StdC::Strlcpy(mpReportDirectory, L"game:\\", EAArrayCount(mpReportDirectory));
    #elif defined(EA_PLATFORM_PS3)
        EA::StdC::Strlcpy(mpReportDirectory, "/app_home/", EAArrayCount(mpReportDirectory));    // /app_home/ works only from runs started by the debugger or TargetManager.
    #elif defined(EA_PLATFORM_DESKTOP)
        EA::StdC::GetCurrentProcessDirectory(mpReportDirectory);
    #else
        EA::IO::GetTempDirectory(mpReportDirectory, EAArrayCount(mpReportDirectory));
    #endif

    AllocatePlatformHandler(true);

    SetAllocator(mpCoreAllocator);
}


///////////////////////////////////////////////////////////////////////////////
// ~ExceptionHandler
//
ExceptionHandler::~ExceptionHandler()
{
    AllocatePlatformHandler(false);
}


///////////////////////////////////////////////////////////////////////////////
// IsValid
//
bool ExceptionHandler::IsValid() const
{
    return (mpPlatformHandler != NULL);
}


///////////////////////////////////////////////////////////////////////////////
// GetAllocator
//
EA::Allocator::ICoreAllocator* ExceptionHandler::GetAllocator() const
{
    return mpCoreAllocator;
}


///////////////////////////////////////////////////////////////////////////////
// SetAllocator
//
void ExceptionHandler::SetAllocator(EA::Allocator::ICoreAllocator* pCoreAllocator)
{
    mpCoreAllocator = pCoreAllocator;

    mReportWriterDefault.SetAllocator(pCoreAllocator);

    // What if the user supplies their own ReportWriter via SetReportWriter and they want it to have a different allocator?
    // In practice this will probably never occur or the users won't care about it. But we can wait and see.
    if(mpReportWriter)
        mpReportWriter->SetAllocator(pCoreAllocator);

    #if EASTL_NAME_ENABLED
        msBuildDescription.set_overflow_allocator(EA::Allocator::EASTLICoreAllocator(EXCEPTIONHANDLER_ALLOC_PREFIX "BuildDescription", mpCoreAllocator));
    #endif
}


///////////////////////////////////////////////////////////////////////////////
// AllocatePlatformHandler
//
void ExceptionHandler::AllocatePlatformHandler(bool state)
{
    if(!REAL_HANDLER && state)
    {
        #if defined(EA_PLATFORM_XENON)
            mpPlatformHandler = reinterpret_cast<void*>(EA_EH_NEW(EXCEPTIONHANDLER_ALLOC_PREFIX "ExceptionHandler") ExceptionHandlerXenon(this));
        #elif defined(EA_PLATFORM_MICROSOFT)
            mpPlatformHandler = reinterpret_cast<void*>(EA_EH_NEW(EXCEPTIONHANDLER_ALLOC_PREFIX "ExceptionHandler") ExceptionHandlerWin32(this));
        #elif defined(EA_PLATFORM_PS3)
            mpPlatformHandler = reinterpret_cast<void*>(EA_EH_NEW(EXCEPTIONHANDLER_ALLOC_PREFIX "ExceptionHandler") ExceptionHandlerPS3(this));
        #elif defined(EA_PLATFORM_APPLE)
            mpPlatformHandler = reinterpret_cast<void*>(EA_EH_NEW(EXCEPTIONHANDLER_ALLOC_PREFIX "ExceptionHandler") ExceptionHandlerApple(this));
        #elif defined(EA_PLATFORM_UNIX)
            mpPlatformHandler = reinterpret_cast<void*>(EA_EH_NEW(EXCEPTIONHANDLER_ALLOC_PREFIX "ExceptionHandler") ExceptionHandlerUnix(this));
        #elif defined(EA_PLATFORM_GAMECUBE) || defined(EA_PLATFORM_REVOLUTION)
            mpPlatformHandler = reinterpret_cast<void*>(EA_EH_NEW(EXCEPTIONHANDLER_ALLOC_PREFIX "ExceptionHandler") ExceptionHandlerWii(this));
        #elif defined(EA_PLATFORM_PSP2)
            mpPlatformHandler = reinterpret_cast<void*>(EA_EH_NEW(EXCEPTIONHANDLER_ALLOC_PREFIX "ExceptionHandler") ExceptionHandlerPSP2(this));
        #endif

        EA_ASSERT(REAL_HANDLER);
        NotifyClients(kExceptionHandlingEnabled);
    }
    else if(REAL_HANDLER && !state && !mbRunningTrapped)
    {
        NotifyClients(kExceptionHandlingDisabled);
        EA_EH_DELETE REAL_HANDLER;
        mpPlatformHandler = NULL;
    }
}


bool ExceptionHandler::SetEnabled(bool enabled)
{
    bool bResult = false;
    
    if(REAL_HANDLER)
    {
        bResult = REAL_HANDLER->SetEnabled(enabled);
        
        if(bResult)
            NotifyClients(enabled ? kExceptionHandlingEnabled : kExceptionHandlingDisabled);
    }

    return bResult;
}


///////////////////////////////////////////////////////////////////////////////
// IsEnabled
//
bool ExceptionHandler::IsEnabled() const
{
    return REAL_HANDLER && REAL_HANDLER->IsEnabled();
}


///////////////////////////////////////////////////////////////////////////////
// SetBuildDescription
//
void ExceptionHandler::SetBuildDescription(const char* pBuildDescription)
{
    if(pBuildDescription)
        msBuildDescription = pBuildDescription;
    else
        msBuildDescription.clear();
}


///////////////////////////////////////////////////////////////////////////////
// GetBuildDescription
//
const char* ExceptionHandler::GetBuildDescription() const
{
     return msBuildDescription.c_str();
}


///////////////////////////////////////////////////////////////////////////////
// GetBuildDescription
//
bool ExceptionHandler::SetMode(Mode mode)
{
    if(REAL_HANDLER)
        return REAL_HANDLER->SetMode(mode);

    return false;
}


///////////////////////////////////////////////////////////////////////////////
// SetAction
//
void ExceptionHandler::SetAction(Action action, int returnValue)
{
    if(REAL_HANDLER)
        REAL_HANDLER->SetAction(action, returnValue);
}


///////////////////////////////////////////////////////////////////////////////
// GetAction
//
ExceptionHandler::Action ExceptionHandler::GetAction(int* pReturnValue) const
{
    if(REAL_HANDLER)
        return REAL_HANDLER->GetAction(pReturnValue);
    return kActionDefault;
}


///////////////////////////////////////////////////////////////////////////////
// SetTerminateOnException
//
void ExceptionHandler::SetTerminateOnException(bool state)
{
    SetAction(state ? kActionTerminate : kActionDefault);
}


///////////////////////////////////////////////////////////////////////////////
// SetTerminateOnException
//
bool ExceptionHandler::GetTerminateOnException() const
{
    const Action action = GetAction();
    return (action == kActionTerminate);
}


void ExceptionHandler::SetReportFileName(const char8_t* pReportFileName)
{
    EA::StdC::Strlcpy(mpReportFileName, pReportFileName, EAArrayCount(mpReportFileName));
}


void ExceptionHandler::SetReportFileName(const char16_t* pReportFileName)
{
    EA::StdC::Strlcpy(mpReportFileName, pReportFileName, EAArrayCount(mpReportFileName));
}


void ExceptionHandler::SetReportDirectory(const char8_t* pReportDirectory)
{
    EA::StdC::Strlcpy(mpReportDirectory, pReportDirectory, EAArrayCount(mpReportDirectory));
    EA::IO::Path::EnsureTrailingSeparator(mpReportDirectory, EAArrayCount(mpReportDirectory));
}


///////////////////////////////////////////////////////////////////////////////
// SetReportDirectory
//
void ExceptionHandler::SetReportDirectory(const char16_t* pReportDirectory)
{
    EA::StdC::Strlcpy(mpReportDirectory, pReportDirectory, EAArrayCount(mpReportDirectory));
    EA::IO::Path::EnsureTrailingSeparator(mpReportDirectory, EAArrayCount(mpReportDirectory));
}


///////////////////////////////////////////////////////////////////////////////
// GetReportDirectory
//
const char16_t* ExceptionHandler::GetReportDirectory() const
{
    return mpReportDirectory; 
}


///////////////////////////////////////////////////////////////////////////////
// GetReportWriter
//
ReportWriter* ExceptionHandler::GetReportWriter() const
{
    return mpReportWriter;
}


///////////////////////////////////////////////////////////////////////////////
// SetReportWriter
//
void ExceptionHandler::SetReportWriter(ReportWriter* pReportWriter)
{
    mpReportWriter = pReportWriter;
}


///////////////////////////////////////////////////////////////////////////////
// RunTrappedInternal
//
intptr_t ExceptionHandler::RunTrappedInternal(UserFunctionUnion userFunctionUnion, UserFunctionType userFunctionType, void* pContext, bool& exceptionCaught)
{
    intptr_t returnValue;

    if(REAL_HANDLER)
    {
        mbRunningTrapped = true;
        returnValue = REAL_HANDLER->RunTrapped(userFunctionUnion, userFunctionType, pContext, exceptionCaught);
        mbRunningTrapped = false;
    }
    else
    {
        returnValue = ExecuteUserFunction(userFunctionUnion, userFunctionType, pContext);
        exceptionCaught = false;
    }

    return returnValue;
}

intptr_t ExceptionHandler::ExecuteUserFunction(UserFunctionUnion userFunctionUnion, UserFunctionType userFunctionType, void* pContext)
{
    switch (userFunctionType)
    {
        default:
        case kEADebugUserFunction:
            userFunctionUnion.mEADebugUserFunction(pContext);
            return 0;

        case kEAThreadRunnableFunction:
            return userFunctionUnion.mEAThreadRunnableFunction(pContext);

        case kEAThreadRunnableClass:
            return userFunctionUnion.mpEAThreadRunnableClass->Run(pContext);
    }
}


///////////////////////////////////////////////////////////////////////////////
// RunTrapped
//
bool ExceptionHandler::RunTrapped(UserFunction userFunction, void* pContext)
{
    UserFunctionUnion userFunctionUnion;
    userFunctionUnion.mEADebugUserFunction = userFunction;

    UserFunctionType userFunctionType = kEADebugUserFunction;
    bool exceptionCaught;

    // In this version we ignore the return value, as it's void.
    RunTrappedInternal(userFunctionUnion, userFunctionType, pContext, exceptionCaught);

    return exceptionCaught;
}


///////////////////////////////////////////////////////////////////////////////
// RunTrapped - Compatible with EA::Thread::RunnableFunction
//
intptr_t ExceptionHandler::RunTrapped(EA::Thread::RunnableFunction runnableFunction, void* pContext)
{
    UserFunctionUnion userFunctionUnion;
    userFunctionUnion.mEAThreadRunnableFunction = runnableFunction;

    UserFunctionType userFunctionType = kEAThreadRunnableFunction;
    bool exceptionCaught;

    // In this version we ignore exceptionCaught.
    return RunTrappedInternal(userFunctionUnion, userFunctionType, pContext, exceptionCaught);
}


///////////////////////////////////////////////////////////////////////////////
// RunTrapped - Compatible with EA::Thread::IRunnable
//
intptr_t ExceptionHandler::RunTrapped(EA::Thread::IRunnable* pRunnableClass, void* pContext)
{
    UserFunctionUnion userFunctionUnion;
    userFunctionUnion.mpEAThreadRunnableClass = pRunnableClass;

    UserFunctionType userFunctionType = kEAThreadRunnableClass;
    bool exceptionCaught;

    // In this version we ignore exceptionCaught.
    return RunTrappedInternal(userFunctionUnion, userFunctionType, pContext, exceptionCaught);
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionCallstack
//
size_t ExceptionHandler::GetExceptionCallstack(void* pCallstackArray, size_t capacity) const
{
    if(REAL_HANDLER)
        return REAL_HANDLER->GetExceptionCallstack(pCallstackArray, capacity);
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionContext
//
const EA::Callstack::Context* ExceptionHandler::GetExceptionContext() const
{
    if(REAL_HANDLER)
        return REAL_HANDLER->GetExceptionContext();
    return NULL;
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionThreadId
//
EA::Thread::ThreadId ExceptionHandler::GetExceptionThreadId(EA::Thread::SysThreadId& sysThreadId, char* threadName, size_t threadNameCapacity) const
{
    if(REAL_HANDLER)
        return REAL_HANDLER->GetExceptionThreadId(sysThreadId, threadName, threadNameCapacity);

    sysThreadId = EA::Thread::kSysThreadIdInvalid;
    if(threadNameCapacity)
        threadName[0] = 0;
    return EA::Thread::kThreadIdInvalid;
}



ExceptionHandler::ExceptionInfo::ExceptionInfo()
  : mTime(),
  //mCallstack[]
    mCallstackEntryCount(0),
    mThreadId(EA::Thread::kThreadIdInvalid),
    mSysThreadId(EA::Thread::kSysThreadIdInvalid),
  //mThreadName[]
    mpExceptionInstructionAddress(NULL),
    mpExceptionMemoryAddress(NULL),
  //mContext(),
    mExceptionDescription()
{
    memset(&mCallstack[0], 0, sizeof(mCallstack));
    memset(mThreadName, 0, sizeof(mThreadName));
    memset(&mContext, 0, sizeof(mContext));
}


const ExceptionHandler::ExceptionInfo* ExceptionHandler::GetExceptionInfo() const
{
    if(REAL_HANDLER)
        return REAL_HANDLER->GetExceptionInfo();
    return NULL;
}


///////////////////////////////////////////////////////////////////////////////
// EnableReportTypes
//
void ExceptionHandler::EnableReportTypes(int reportTypes)
{
    mReportTypes = reportTypes;
}


///////////////////////////////////////////////////////////////////////////////
// GetReportTypes
//
int ExceptionHandler::GetReportTypes() const
{
    return mReportTypes;
}


///////////////////////////////////////////////////////////////////////////////
// EnableReportFields
//
void ExceptionHandler::EnableReportFields(int reportFields)
{
    mReportFields = reportFields;
}


///////////////////////////////////////////////////////////////////////////////
// GetReportFields
//
int ExceptionHandler::GetReportFields() const
{
    return mReportFields;
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstackFlags
//
int ExceptionHandler::GetCallstackFlags() const
{
    return mCallstackAddressRepTypeFlags;
}


///////////////////////////////////////////////////////////////////////////////
// SetCallstackFlags
//
void ExceptionHandler::SetCallstackFlags(int addressRepTypeFlags)
{
    mCallstackAddressRepTypeFlags = addressRepTypeFlags;
}


///////////////////////////////////////////////////////////////////////////////
// GetMiniDumpFlags
//
int ExceptionHandler::GetMiniDumpFlags() const
{
    return mMiniDumpFlags;
}


///////////////////////////////////////////////////////////////////////////////
// SetCallstackFlags
//
void ExceptionHandler::SetMiniDumpFlags(int miniDumpFlags)
{
    mMiniDumpFlags = miniDumpFlags;
}


///////////////////////////////////////////////////////////////////////////////
// ReadMinidumpBegin
//
bool ExceptionHandler::ReadMinidumpBegin()
{
    if(!mpFile)
    {
        char8_t filePath[EA::IO::kMaxPathLength];

        GetCurrentReportFilePath(filePath, EAArrayCount(filePath), kReportTypeMinidump);
        mpFile = fopen(filePath, "rb");
        return (mpFile != NULL);
    }

    return false;
}


///////////////////////////////////////////////////////////////////////////////
// ReadMinidumpPart
//
size_t ExceptionHandler::ReadMinidumpPart(char* buffer, size_t size)
{
    if(mpFile)
    {
        size_t result = fread(buffer, 1, size, mpFile);

        if((result == size) && !ferror(mpFile))
            return result;
    }

    return (size_t)(ssize_t)-1;
}


///////////////////////////////////////////////////////////////////////////////
// ReadMinidumpEnd
//
void ExceptionHandler::ReadMinidumpEnd()
{
    if(mpFile)
    {
        fclose(mpFile);
        mpFile = NULL;
    }
}


///////////////////////////////////////////////////////////////////////////////
// RegisterClient
//
void ExceptionHandler::RegisterClient(ExceptionHandlerClient* pClient, bool bRegister)
{
    EA_ASSERT(pClient);

    HandlerList::iterator it = eastl::find(mHandlerList.begin(), mHandlerList.end(), pClient);

    if(it == mHandlerList.end() && bRegister)
    {
        // Put the most recently registered clients at the front of the list, like a stack.
        mHandlerList.push_front(pClient);

        if(IsEnabled())
            pClient->Notify(this, kExceptionHandlingEnabled);
        else
            pClient->Notify(this, kExceptionHandlingDisabled);
    }
    else if((it != mHandlerList.end()) && !bRegister)
        mHandlerList.erase(it);
}


///////////////////////////////////////////////////////////////////////////////
// NotifyClients
//
void ExceptionHandler::NotifyClients(EventId eventId)
{
    for(HandlerList::iterator it = mHandlerList.begin(), itEnd = mHandlerList.end(); it != itEnd; ++it)
    {
        ExceptionHandlerClient* const pClient = *it;
        pClient->Notify(this, eventId);
    }
}


///////////////////////////////////////////////////////////////////////////////
// SimulateExceptionHandling
//
void ExceptionHandler::SimulateExceptionHandling(int exceptionType)
{
    if(REAL_HANDLER)
        REAL_HANDLER->SimulateExceptionHandling(exceptionType);
}


///////////////////////////////////////////////////////////////////////////////
// WriteExceptionReport
//
void ExceptionHandler::WriteExceptionReport()
{
    if(mReportTypes & kReportTypeException)
    {
        mpReportWriter = GetReportWriter();

        // Get the report file path.
        GenerateReportFilePath(mReportFilePath, EAArrayCount(mReportFilePath), kReportTypeException);

        // Get thread Ids.
        EA::Thread::SysThreadId sysThreadId;
        EA::Thread::ThreadId    threadId = GetExceptionThreadId(sysThreadId, NULL, 0);
        mpReportWriter->SetExceptionThreadIds(threadId, sysThreadId);

        // Get the exception callstack and pass it on to the report writer.
        // Work in progress: Some platforms don't have GetExceptionCallstack implemented yet and 
        // just return 0. However, we a workaround in ReportWriter::WriteCallStackFromCallstackContext
        // to deal with that until all platforms have an implementation of it.
        const void* callstack[32];
        size_t callstackSize = REAL_HANDLER->GetExceptionCallstack(callstack, EAArrayCount(callstack));
        mpReportWriter->SetExceptionCallstack(callstack, callstackSize);

        // Write the report
        if(mpReportWriter->OpenReport(mReportFilePath))
        {
            mpReportWriter->BeginReport("Exception Report");

            #if defined(EA_COMPILER_MSVC) && defined(EA_PLATFORM_MICROSOFT)
                #define LOCAL_PRE_PHASE_GUARD() __try {
                #define LOCAL_POST_PHASE_GUARD() } __except(1) { mpReportWriter->WriteText("<An exception was encountered while writing this section.>", true); }
            #else
                #define LOCAL_PRE_PHASE_GUARD()
                #define LOCAL_POST_PHASE_GUARD()
            #endif

            if(mReportFields & ExceptionHandler::kReportFieldHeader)
            {
                LOCAL_PRE_PHASE_GUARD()
                WriteHeader();
                LOCAL_POST_PHASE_GUARD()
            }

            if(mReportFields & ExceptionHandler::kReportFieldCallStack)
            {
                LOCAL_PRE_PHASE_GUARD()
                WriteCallStack();
                WriteThreadList();
                LOCAL_POST_PHASE_GUARD()
            }

            if(mReportFields & ExceptionHandler::kReportFieldRegisters)
            {
                LOCAL_PRE_PHASE_GUARD()
                WriteRegisters();
                LOCAL_POST_PHASE_GUARD()
            }

            if(mReportFields & ExceptionHandler::kReportFieldMemoryDumps)
            {
                LOCAL_PRE_PHASE_GUARD()
                WriteMemoryDumps();
                LOCAL_POST_PHASE_GUARD()
            }

            if(mReportFields & ExceptionHandler::kReportFieldLoadedModules)
            {
                LOCAL_PRE_PHASE_GUARD()
                WriteLoadedModules();
                LOCAL_POST_PHASE_GUARD()
            }
          
            if(mReportFields & ExceptionHandler::kReportFieldOpenFiles)
            {
                LOCAL_PRE_PHASE_GUARD()
                WriteOpenFileList();
                LOCAL_POST_PHASE_GUARD()
            }

            if(mReportFields & ExceptionHandler::kReportFieldProcessList)
            {                
                LOCAL_PRE_PHASE_GUARD()
                WriteProcessList();
                LOCAL_POST_PHASE_GUARD()
            }

            if(mReportFields & ExceptionHandler::kReportFieldExtraData)
            {
                LOCAL_PRE_PHASE_GUARD()
                WriteExtraData();
                LOCAL_POST_PHASE_GUARD()
            }

            mpReportWriter->EndReport();
            mpReportWriter->CloseReport();
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
// WriteHeader
//
void ExceptionHandler::WriteHeader()
{
    char buffer[512];

    /////////////////////////////////////////////
    // Build Info
    // This is an application-specific section which the user provides. 
    // It is expected to be in the same format as the rest of the sections
    // and thus have a 'key: value\n' entry per line.
    mpReportWriter->BeginSection("Build info");

    const char* const pBuildDescription = GetBuildDescription();
    if(pBuildDescription && *pBuildDescription)
        mpReportWriter->WriteText(pBuildDescription, true);

    mpReportWriter->EndSection("Build info");
    /////////////////////////////////////////////


    /////////////////////////////////////////////
    // System info
    mpReportWriter->BeginSection("This Report");
    mpReportWriter->WriteKeyValue("Report Path", mReportFilePath);
    mpReportWriter->WriteKeyValueFormatted("EACallstack version", EACALLSTACK_VERSION);
    mpReportWriter->WriteKeyValueFormatted("ExceptionHandler version", EXCEPTIONHANDLER_VERSION);
    mpReportWriter->EndSection("This Report");
    /////////////////////////////////////////////


    /////////////////////////////////////////////
    // System info
    mpReportWriter->BeginSection("System info");
    mpReportWriter->WriteSystemInfo();
    mpReportWriter->EndSection("System info");
    /////////////////////////////////////////////


    /////////////////////////////////////////////
    // Application info
    mpReportWriter->BeginSection("Application info");
    mpReportWriter->WriteApplicationInfo();
    mpReportWriter->EndSection("Application info");
    /////////////////////////////////////////////


    /////////////////////////////////////////////
    // Exception info
    //
    // To consider: Have ReportWriter completely handle this itself. The only problem with that is 
    // that ReportWriter is currently designed to not be aware of the ExceptionHandler class, 
    // though that doesn't necessarily need to be that way.
    // To do: Convert some of the functions below to use our new GetExceptionInfo API.
    const ExceptionInfo* pExceptionInfo = GetExceptionInfo();

    mpReportWriter->BeginSection("Exception info");

    mpReportWriter->GetDateString(buffer, sizeof(buffer), &pExceptionInfo->mTime);
    mpReportWriter->WriteKeyValue("Date", buffer);

    mpReportWriter->GetTimeString(buffer, sizeof(buffer), &pExceptionInfo->mTime, true);
    mpReportWriter->WriteKeyValue("Time", buffer);
    
    EA::Thread::SysThreadId sysThreadId;
    char8_t                 threadName[32];
    EA::Thread::ThreadId    threadId = GetExceptionThreadId(sysThreadId, threadName, EAArrayCount(threadName));
    
    #if (EATHREAD_VERSION_N >= 11702)
        mpReportWriter->WriteKeyValueFormatted("Thread", "id: %s, sys id: %s, name: %s", EAThreadThreadIdToString(threadId), EAThreadSysThreadIdToString(sysThreadId), threadName[0] ? threadName : "(unavailable)");
    #else
        mpReportWriter->WriteKeyValueFormatted("Thread", "id: %I64x, sys id: %I64d, name: %s", (uint64_t)threadId, (int64_t)sysThreadId, threadName[0] ? threadName : "(unavailable)");
    #endif

    REAL_HANDLER->GetExceptionDescription(buffer, sizeof(buffer));
    mpReportWriter->WriteKeyValue("Type", buffer);

    mpReportWriter->GetAddressLocationString(pExceptionInfo->mpExceptionInstructionAddress, buffer, sizeof(buffer));
    mpReportWriter->WriteKeyValue("Instruction address", buffer);

    mpReportWriter->EndSection("Exception info");
    /////////////////////////////////////////////
}


///////////////////////////////////////////////////////////////////////////////
// WriteCallStack
//
void ExceptionHandler::WriteCallStack()
{
    mpReportWriter->BeginSection("Call stack");
    mpReportWriter->WriteCallStack(GetExceptionContext(), mCallstackAddressRepTypeFlags);
    mpReportWriter->EndSection("Call stack");

    #if defined(EA_PROCESSOR_POWERPC)
        const uint8_t* const pStack = (uint8_t*)(uintptr_t)GetExceptionContext()->mGpr[1];
    #elif defined(EA_PROCESSOR_X86)
        const uint8_t* const pStack = (uint8_t*)(uintptr_t)GetExceptionContext()->Esp;
    #elif defined(EA_PROCESSOR_X86_64)
        const uint8_t* const pStack = (uint8_t*)(uintptr_t)GetExceptionContext()->Rsp;
    #elif defined(EA_PROCESSOR_ARM)
        const uint8_t* const pStack = (uint8_t*)(uintptr_t)GetExceptionContext()->mGpr[13];
    #endif

    mpReportWriter->BeginSection("Stack data");
    mpReportWriter->WriteStackMemoryView(pStack, 512);
    mpReportWriter->EndSection("Stack data");

    #if defined(EA_PROCESSOR_POWERPC)
        const uint8_t* const pInstruction = (uint8_t*)(uintptr_t)GetExceptionContext()->mIar;
    #elif defined(EA_PROCESSOR_X86)
        const uint8_t* const pInstruction = (uint8_t*)(uintptr_t)GetExceptionContext()->Eip;
    #elif defined(EA_PROCESSOR_X86_64)
        const uint8_t* const pInstruction = (uint8_t*)(uintptr_t)GetExceptionContext()->Rip;
    #elif defined(EA_PROCESSOR_ARM)
        const uint8_t* const pInstruction = (uint8_t*)(uintptr_t)GetExceptionContext()->mGpr[15];
    #endif

    mpReportWriter->BeginSection("Instruction data");
    mpReportWriter->WriteDisassembly(pInstruction - 128, 256, pInstruction);
    mpReportWriter->EndSection("Instruction data");
}


///////////////////////////////////////////////////////////////////////////////
// WriteThreadList
//
void ExceptionHandler::WriteThreadList()
{
    mpReportWriter->BeginSection("Thread list");
    mpReportWriter->WriteThreadList();
    mpReportWriter->EndSection("Thread list");
}


///////////////////////////////////////////////////////////////////////////////
// WriteProcessList
//
void ExceptionHandler::WriteProcessList()
{
    #if !defined(EA_PLATFORM_CONSOLE) // Console platforms are single-process-only.
        mpReportWriter->BeginSection("Process list");
        mpReportWriter->WriteProcessList();
        mpReportWriter->EndSection("Process list");
    #endif
}


///////////////////////////////////////////////////////////////////////////////
// WriteRegisters
//
void ExceptionHandler::WriteRegisters()
{
    mpReportWriter->BeginSection("Registers");
    mpReportWriter->WriteRegisterValues(GetExceptionContext());
    mpReportWriter->EndSection("Registers");
}


///////////////////////////////////////////////////////////////////////////////
// WriteLoadedModules
//
void ExceptionHandler::WriteLoadedModules()
{
    mpReportWriter->BeginSection("Modules");
    mpReportWriter->WriteModuleList();
    mpReportWriter->EndSection("Modules");
}


///////////////////////////////////////////////////////////////////////////////
// WriteOpenFileList
//
void ExceptionHandler::WriteOpenFileList()
{
    mpReportWriter->BeginSection("Open files");
    mpReportWriter->WriteOpenFileList();
    mpReportWriter->EndSection("Open files");
}


///////////////////////////////////////////////////////////////////////////////
// WriteMemoryDumps
//
void ExceptionHandler::WriteMemoryDumps()
{
    mpReportWriter->BeginSection("Register memory");
    mpReportWriter->WriteRegisterMemoryView(GetExceptionContext());
    mpReportWriter->EndSection("Register memory");
}



///////////////////////////////////////////////////////////////////////////////
// WriteExtraData
//
// We write something that looks like this (x86):
//     [Extra]
//     <arbitrary user data here>
//
void ExceptionHandler::WriteExtraData()
{
    mpReportWriter->BeginSection("Extra");

    const size_t posSaved = mpReportWriter->GetPosition();

    #if defined(_MSC_VER)
        __try {
            NotifyClients(ExceptionHandler::kReadyForExtraData);
            NotifyClients(ExceptionHandler::kExtraDataDone);
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            // In case if some client messes up and dies trying to provide 
            // extra data we just report that and continue.
            mpReportWriter->WriteText("Extra data is not complete due to an exception generated by some client.\r\n");
        }
    #else
        NotifyClients(ExceptionHandler::kReadyForExtraData);
        NotifyClients(ExceptionHandler::kExtraDataDone);
    #endif

    if(posSaved == mpReportWriter->GetPosition()) // If nothing was written...
        mpReportWriter->WriteText("(none)\r\n");

    mpReportWriter->EndSection("Extra");
}


///////////////////////////////////////////////////////////////////////////////
// GetCurrentReportFilePath
//
size_t ExceptionHandler::GetCurrentReportFilePath(char* buffer, size_t capacity, ReportTypeMask reportType)
{
    if(!mReportFilePath[0])
        GenerateReportFilePath(mReportFilePath, EAArrayCount(mReportFilePath), kReportTypeException);

    if(reportType == kReportTypeException)
    {
        // If the user called SetReportFileName with a full file path, then the string returned here will
        // be the same as that.

        // We could alternatively ask mpReportWriter for the path it used, though that should be 
        // the same as mReportFilePath.
        return EA::StdC::Strlcpy(buffer, mReportFilePath, capacity);
    }

    if(reportType == kReportTypeMinidump)
    {
        char miniDumpReportFilePath[sizeof(mReportFilePath)];
        EA::StdC::Strcpy(miniDumpReportFilePath, mReportFilePath);
        char* extensionPos = EA::StdC::Strchr(miniDumpReportFilePath, '.');
        if(extensionPos != NULL)
            *extensionPos = 0;
        EA::StdC::Strlcat(miniDumpReportFilePath, Internal::kMinidumpFileNameExtension, EAArrayCount(miniDumpReportFilePath));
        
        return EA::StdC::Strlcpy(buffer, miniDumpReportFilePath, capacity);
    }

    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// GenerateReportFilePath
//
// We write a report to a file path such as:
//     "game:\xcpt xenon_pkavan 06-10-27 19.25.43.txt"          XBox 360
//     "/baseball/xcpt xenon_pkavan 06-10-27 19.25.43.txt"      PS3
//     "C:\baseball\xcpt xenon_pkavan 06-10-27 19.25.43.txt"    Windows
//
size_t ExceptionHandler::GenerateReportFilePath(char8_t* buffer, size_t capacity, ReportTypeMask reportType)
{
    // Get the directory to use by default.
    char8_t reportDirectory8[EA::IO::kMaxDirectoryLength];
    EA::StdC::Strlcpy(reportDirectory8, GetReportDirectory(), EA::IO::kMaxDirectoryLength);

    if(mpReportFileName[0]) // If the user specified a report file name manually...
    {
        char8_t reportFileName[EA::IO::kMaxPathLength];
        EA::StdC::Strlcpy(reportFileName, mpReportFileName, EAArrayCount(reportFileName));

        if(reportType == kReportTypeMinidump)
        {
            char* extensionPos = EA::StdC::Strchr(reportFileName, '.');
            if(extensionPos != NULL)
                *extensionPos = 0;
            EA::StdC::Strlcat(reportFileName, Internal::kMinidumpFileNameExtension, EAArrayCount(reportFileName));
        }

        if(EA::IO::Path::IsRelative(reportFileName)) // If the user-supplied report file name is just a file name and not a full file path...
            EA::StdC::Snprintf(buffer, capacity, "%s%s", reportDirectory8, reportFileName);
        else
            EA::StdC::Strlcpy(buffer, reportFileName, capacity);
    }
    else // Else generate a file name based on some system info, and append it to the report directory.
    {
        const size_t kFileNameLengthLimit = 64;
        const size_t kMaxFileNameLength   = eastl::min_alt(kFileNameLengthLimit, (size_t)EA::IO::kMaxFileNameLength);

        // Get the base report name to use.
        // To consider: validate the base report name string for valid filename characters.
        char baseReportName[16];
        strncpy(baseReportName, mBaseReportName, sizeof(baseReportName));
        baseReportName[15] = 0;
        size_t baseReportStrlen = strlen(baseReportName);

        // Get the machine name to use.
        char machineName[32];
        mpReportWriter->GetMachineName(machineName, sizeof(machineName));
        if(kMaxFileNameLength < kFileNameLengthLimit)
            machineName[15] = 0; // Limit the name to strlen of 16.
        else
            machineName[23] = 0; // Limit the name to strlen of 24.
        size_t machineStrlen = strlen(machineName);

        // Get the time string to use.
        char timeString[16];
        const ExceptionInfo* pExceptionInfo = GetExceptionInfo();
        mpReportWriter->GetTimeString(timeString, sizeof(timeString), &pExceptionInfo->mTime, false);
        size_t timeStrlen = strlen(timeString);

        // Get the date string to use.
        char dateString[16];
        mpReportWriter->GetDateString(dateString, sizeof(dateString), &pExceptionInfo->mTime, false); // false means use 2 digit year (in order to save space).
        size_t dateStrlen = strlen(dateString);

        // Get the file name extension to use.
        char filenameExtension[16];
        if(reportType == kReportTypeMinidump)
            strcpy(filenameExtension, Internal::kMinidumpFileNameExtension);
        else
            mpReportWriter->GetFileNameExtension(filenameExtension, EAArrayCount(filenameExtension));
        size_t extensionStrlen = strlen(filenameExtension);

        // Write the full file name.
        // We have to deal with the fact that the file system can store no more than 
        // kMaxFileNameLength characters per file name. So we store the most important 
        // information in the file name first and then fit what we can of the less 
        // important information. The order of importance of file name components, 
        // in order of most important to least important is:
        //    - file extension
        //    - machine name
        //    - base report name
        //    - date
        //    - time
        size_t capacityRemaining = kMaxFileNameLength;
        eastl::fixed_string<char, 128, true> fileName;

        // We always write the extension, but we write it last.
        // So we subtract its size here but append it later.
        capacityRemaining -= extensionStrlen;

        if(machineStrlen && (machineStrlen < capacityRemaining))
        {
            fileName += machineName;
            fileName += ' ';
            capacityRemaining -= machineStrlen;
        }

        if(baseReportStrlen && (baseReportStrlen < capacityRemaining))
        {
            // We put the base report name in front of the machine name.
            fileName.insert(0, baseReportName);
            fileName.insert((eastl_size_t)baseReportStrlen, 1, ' ');
            capacityRemaining -= baseReportStrlen;
        }

        if(dateStrlen && (dateStrlen < capacityRemaining))
        {
            fileName += dateString;
            fileName += ' ';
            capacityRemaining -= dateStrlen;
        }

        if(timeStrlen && (timeStrlen < capacityRemaining))
        {
            fileName += timeString;
            fileName += ' ';
            capacityRemaining -= timeStrlen;
        }

        // The code above will always have left a trailing ' ' char, so remove it.
        fileName.pop_back();

        // Now we append the file name extension which we made room for above.
        fileName += filenameExtension;

        EA::StdC::Snprintf(buffer, capacity, "%s%s", reportDirectory8, fileName.c_str());
    }

    size_t length = strlen(buffer);

    // Convert all spaces to mReportNameSpaceChar, which may be space itself.
    for(size_t i = 0; i < length; i++)
    {
        if(buffer[i] == ' ')
            buffer[i] = mReportNameSpaceChar;
    }

    return length;
}


bool ExceptionHandler::RemoveReportFile(int reportTypeFlags)
{
    bool    success = true;
    char8_t filePath[EA::IO::kMaxPathLength];

    if(reportTypeFlags & kReportTypeException)
    {
        GetCurrentReportFilePath(filePath, EAArrayCount(filePath), kReportTypeException);
        success = (EA::IO::File::Remove(filePath) && success);
    }

    if(reportTypeFlags & kReportTypeMinidump)
    {
        GetCurrentReportFilePath(filePath, EAArrayCount(filePath), kReportTypeMinidump);
        success = (EA::IO::File::Remove(filePath) && success);
    }

    return success;
}


/// This function is *deprecated*. Use the GetExceptionInfo function instead, which works for all platforms.
void* ExceptionHandler::GetPlatformExceptionInfo()
{
    #if defined EA_PLATFORM_WINDOWS
        if(REAL_HANDLER)
            return REAL_HANDLER->mExceptionInfoWin32.mpExceptionPointers;
    #else
        // This could be done for some other platforms, though we may need to package them into     
        // a struct of our own definition, as some platforms don't present us with a single 
        // struct with this info.
    #endif
    
    return NULL;
}


} // namespace Debug

} // namespace EA


// Undefine any locally-defined variables
#undef REAL_HANDLER













