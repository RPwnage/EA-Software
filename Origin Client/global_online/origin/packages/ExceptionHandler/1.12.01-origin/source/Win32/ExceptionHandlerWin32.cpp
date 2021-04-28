///////////////////////////////////////////////////////////////////////////////
// ExceptionHandlerWin32.cpp
// 
// Copyright (c) 2005, Electronic Arts. All Rights Reserved.
// Written and maintained by Paul Pedriana.
//
// References:
//  http://msdn.microsoft.com/en-us/library/windows/desktop/ms680657%28v=vs.85%29.aspx
//  http://msdn.microsoft.com/en-us/magazine/cc301714.aspx
//
///////////////////////////////////////////////////////////////////////////////


#include <ExceptionHandler/Win32/ExceptionHandlerWin32.h>


// Windows-specific implementation. Make sure nobody accidentally sticks it into wrong project
#if defined(EA_PLATFORM_MICROSOFT) && !defined(EA_PLATFORM_XENON)

#include <ExceptionHandler/ExceptionHandler.h>
#include <ExceptionHandler/ReportWriter.h>
#include <EASTL/fixed_string.h>
#include <EAStdC/EAString.h>
#include <EAIO/EAFileBase.h>
#include EA_ASSERT_HEADER

#if defined(_MSC_VER)
    #pragma warning(push, 0)
#endif
#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif

#include <malloc.h> // For _resetstkoflw
#include <string.h>
#include <stdexcept>

#if defined(EA_PLATFORM_CONSOLE)
    #include <xdk.h>

    #if (_XDK_VER < 9299) // To do: Remove this in August of 2013.
        // Declared here because the March Capilano SDK and earlier require this.
        typedef struct tagVS_FIXEDFILEINFO
        {
            DWORD   dwSignature;            /* e.g. 0xfeef04bd */
            DWORD   dwStrucVersion;         /* e.g. 0x00000042 = "0.42" */
            DWORD   dwFileVersionMS;        /* e.g. 0x00030075 = "3.75" */
            DWORD   dwFileVersionLS;        /* e.g. 0x00000031 = "0.31" */
            DWORD   dwProductVersionMS;     /* e.g. 0x00030010 = "3.10" */
            DWORD   dwProductVersionLS;     /* e.g. 0x00000031 = "0.31" */
            DWORD   dwFileFlagsMask;        /* = 0x3F for version "0.42" */
            DWORD   dwFileFlags;            /* e.g. VFF_DEBUG | VFF_PRERELEASE */
            DWORD   dwFileOS;               /* e.g. VOS_DOS_WINDOWS16 */
            DWORD   dwFileType;             /* e.g. VFT_DRIVER */
            DWORD   dwFileSubtype;          /* e.g. VFT2_DRV_KEYBOARD */
            DWORD   dwFileDateMS;           /* e.g. 0 */
            DWORD   dwFileDateLS;           /* e.g. 0 */
        } VS_FIXEDFILEINFO;
    #endif

    #define PMINIDUMP_CALLBACK_INFORMATION void*  // Required because this platform neglects to declare this.

    // Temporary while waiting for formal support:
    extern "C" WINBASEAPI LPTOP_LEVEL_EXCEPTION_FILTER WINAPI SetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER);
#endif

#include <Windows.h>
#include <DbgHelp.h>

#if defined(_MSC_VER)
    #pragma warning(pop)
    #pragma warning(disable: 4509) // nonstandard extension used: 'x' uses SEH and 'y' has destructor
#endif


//	Uncomment next line to enable message box to instruct user to send minidump to client dev team.
//#define NOTIFYCRASHDIALOG_ENABLE

//	Uncomment next line to enable message box to give user a chance to attach with debugger
//#define DEBUGGERCRASHDIALOG_ENABLE


namespace EA {
namespace Debug {


// To consider: Make this not be a single global but instead some kind of 
// thread-specific stack of pointers. That way we could have more than one
// exception handler at a time.
static ExceptionHandlerWin32* mpThis = NULL;


///////////////////////////////////////////////////////////////////////////////
// Win32ExceptionFilter
//
// We have a problem here in that we are limited to a single global exception 
// handler. This is primarily because Microsoft's APIs provide no means to 
// associate a context for the callback we get. We could probably work around 
// this by providing multiple Win32ExceptionFilter function implementations, 
// as odd as that may look.
//
LONG WINAPI Win32ExceptionFilter(LPEXCEPTION_POINTERS pExceptionPointers)
{
    if(mpThis)
        return (LONG)mpThis->ExceptionFilter(pExceptionPointers);
    return EXCEPTION_CONTINUE_SEARCH;
}


///////////////////////////////////////////////////////////////////////////////
// ExceptionHandlerWin32
//
ExceptionHandlerWin32::ExceptionHandlerWin32(ExceptionHandler* pOwner) :
    mpOwner(pOwner),
    mbEnabled(false),
    mbExceptionOccurred(false),
    mbHandlingInProgress(0),
    mMode(ExceptionHandler::kModeStackBased),
    mAction(ExceptionHandler::kActionDefault),
    mpVectoredHandle(NULL),
    mnTerminateReturnValue(ExceptionHandler::kDefaultTerminationReturnValue),
  //mMiniDumpPath[],
    mPreviousFilter(NULL)
{
    memset(mMiniDumpPath, 0, sizeof(mMiniDumpPath));

    EA_ASSERT(mpThis == NULL);
    mpThis = this;

    // For backward compatibility, we default to kModeStackBased and we SetEnabled(true).
    SetMode(ExceptionHandler::kModeStackBased);
    SetEnabled(true);
}


///////////////////////////////////////////////////////////////////////////////
// ~ExceptionHandlerWin32
//
ExceptionHandlerWin32::~ExceptionHandlerWin32()
{
    mpThis = NULL;
    SetEnabled(false);
}


///////////////////////////////////////////////////////////////////////////////
// SetEnabled
//
bool ExceptionHandlerWin32::SetEnabled(bool state)
{
    switch (mMode)
    {
        case ExceptionHandler::kModeVectored:
        #if EA_EXCEPTIONHANDLER_VECTORED_HANDLING_SUPPORTED
        {
            if(state && !mbEnabled)
            {
                EA_ASSERT(mpVectoredHandle == NULL);
                mpVectoredHandle = AddVectoredExceptionHandler(1, Win32ExceptionFilter);
                mbEnabled = (mpVectoredHandle != NULL);
                EA_ASSERT(mbEnabled);
                return mbEnabled;
            }
            else if(!state && mbEnabled)
            {
                EA_ASSERT(mpVectoredHandle != NULL);
                ULONG result = RemoveVectoredExceptionHandler(mpVectoredHandle);
                EA_UNUSED(result);
                EA_ASSERT(result != 0);
                mpVectoredHandle = NULL;
                mbEnabled = false;
                return true;
            }

            break;
        }
        #else
            return false;
        #endif

        case ExceptionHandler::kModeStackBased:
        {
            if(state && !mbEnabled)
            {
                mbEnabled = true;
                mPreviousFilter = SetUnhandledExceptionFilter(Win32ExceptionFilter);
            }
            else if(!state && mbEnabled)
            {
                mbEnabled = false;
                SetUnhandledExceptionFilter(mPreviousFilter);
                mPreviousFilter = NULL;
            }

            break;
        }

        case ExceptionHandler::kModeCPP:
        {
            // C++ try/catch exception handling is executed only via a user call to 
            // our RunTrapped functionand we don't need to do anything here.
            #if defined(EA_COMPILER_NO_EXCEPTIONS)
                EA_FAIL_MESSAGE("ExceptionHandlerWin32::SetEnabled: kModeCPP selected, but the exception handling compiler support is disabled.");
            #endif
            
            mbEnabled = state;
            return true;
        }

        case ExceptionHandler::kModeSignalHandling:
            EA_FAIL_MESSAGE("ExceptionHandlerWin32::SetEnabled: kModeSignalHandling selected but not supported by Windows.");
            // Fall through
        default:
            return false;
    }

    return (mbEnabled == state);
}


///////////////////////////////////////////////////////////////////////////////
// IsEnabled
//
bool ExceptionHandlerWin32::IsEnabled() const
{
    return mbEnabled;
}


///////////////////////////////////////////////////////////////////////////////
// SetMode
//
bool ExceptionHandlerWin32::SetMode(ExceptionHandler::Mode mode)
{
    bool originallyEnabled = mbEnabled;

    // If the mode is being changed, disable the current mode first.
    if(mbEnabled && (mode != mMode))
        SetEnabled(false);

    if(!mbEnabled) // If we were already disabled or were able to disable it above...
    {
        if(mode == ExceptionHandler::kModeDefault)
            mode = ExceptionHandler::kModeVectored;
        
        // The user needs to call SetEnabled(true/false) in order to enable/disable handling in this mode.
        mMode = mode;

        if(originallyEnabled)
            SetEnabled(true); // What if this fails?

        return true;
    }
    
    return false;
}


///////////////////////////////////////////////////////////////////////////////
// SetAction
//
void ExceptionHandlerWin32::SetAction(ExceptionHandler::Action action, int returnValue)
{
    mAction = action;
    mnTerminateReturnValue = returnValue;
}


///////////////////////////////////////////////////////////////////////////////
// GetAction
//
ExceptionHandler::Action ExceptionHandlerWin32::GetAction(int* pReturnValue) const
{
    if(pReturnValue)
        *pReturnValue = mnTerminateReturnValue;
    return mAction;
}


///////////////////////////////////////////////////////////////////////////////
// SimulateExceptionHandling
//
void ExceptionHandlerWin32::SimulateExceptionHandling(int exceptionType)
{
    if(mbHandlingInProgress.SetValueConditional(1, 0)) // Set it true (1) from false (0) atomically, evaluating as true if somebody else didn't do it first.
    {
        mbExceptionOccurred = true;

        // To do: enable usage of exceptionType.

        memset(&mExceptionInfoWin32.mContext, 0, sizeof(mExceptionInfoWin32.mContext));

        {
            EA::Callstack::CallstackContext callstackContext;
            EA::Callstack::GetCallstackContext(callstackContext, (intptr_t)EA::Thread::GetThreadId()); // In theory we could use EA::Thread::kThreadIdCurrent here, but older versions of EAThread can't deal with that. Need EATHREAD_VERSION_N >= 12006
        
            #if defined(EA_PROCESSOR_ARM) 
                mExceptionInfoWin32.mContext.mGpr[11] = callstackContext.mFP;   /// Frame pointer; register 11 for ARM instructions, register 7 for Thumb instructions.
                mExceptionInfoWin32.mContext.mGpr[13] = callstackContext.mSP;   /// Stack pointer; register 13
                mExceptionInfoWin32.mContext.mGpr[14] = callstackContext.mLR;   /// Link register; register 14
                mExceptionInfoWin32.mContext.mGpr[15] = callstackContext.mPC;   /// Program counter; register 15

            #elif defined(EA_PROCESSOR_X86) 
                mExceptionInfoWin32.mContext.Eip = callstackContext.mEIP;      /// Instruction pointer.
                mExceptionInfoWin32.mContext.Esp = callstackContext.mESP;      /// Stack pointer.
                mExceptionInfoWin32.mContext.Ebp = callstackContext.mEBP;      /// Base pointer.

            #elif defined(EA_PROCESSOR_X86_64)
                mExceptionInfoWin32.mContext.Rip = callstackContext.mRIP;      /// Instruction pointer.
                mExceptionInfoWin32.mContext.Rsp = callstackContext.mRSP;      /// Stack pointer.
                mExceptionInfoWin32.mContext.Rbp = callstackContext.mRBP;      /// Base pointer.

            #elif defined(EA_PROCESSOR_POWERPC)
                mExceptionInfoWin32.mContext.mGpr[1] = callstackContext.mGPR1; /// General purpose register 1.
                mExceptionInfoWin32.mContext.mIar    = callstackContext.mIAR;  /// Instruction address pseudo-register.
            #endif    
        }

        mExceptionInfoWin32.mThreadId    = EA::Thread::GetThreadId();
        mExceptionInfoWin32.mSysThreadId = EA::Thread::GetSysThreadId(mExceptionInfoWin32.mThreadId);  // Get self as a mach kernel thread type.
        EA::Debug::GetThreadName(mExceptionInfoWin32.mThreadId, mExceptionInfoWin32.mSysThreadId, mExceptionInfoWin32.mThreadName, EAArrayCount(mExceptionInfoWin32.mThreadName));

        const time_t timeValue = time(NULL);
        mExceptionInfoWin32.mTime = *localtime(&timeValue);

        // Record the exception callstack
        EA::Callstack::CallstackContext callstackContext;
        EA::Callstack::GetCallstackContext(callstackContext, GetExceptionContext());
        mExceptionInfoWin32.mCallstackEntryCount = EA::Thread::GetCallstack(mExceptionInfoWin32.mCallstack, EAArrayCount(mExceptionInfoWin32.mCallstack), &callstackContext);

        #if defined(EA_PROCESSOR_ARM) 
            mExceptionInfoWin32.mpExceptionInstructionAddress = (void*)(uintptr_t)mExceptionInfoWin32.mContext.mGpr[15];
        #elif defined(EA_PROCESSOR_X86) 
            mExceptionInfoWin32.mpExceptionInstructionAddress = (void*)(uintptr_t)mExceptionInfoWin32.mContext.Eip;
        #elif defined(EA_PROCESSOR_X86_64)
            mExceptionInfoWin32.mpExceptionInstructionAddress = (void*)(uintptr_t)mExceptionInfoWin32.mContext.Rip;
        #elif defined(EA_PROCESSOR_POWERPC)
            mExceptionInfoWin32.mpExceptionInstructionAddress = (void*)(uintptr_t)mExceptionInfoWin32.mContext.mIar;
        #endif    

        // To do: Fill out the following with appropriate fake values. This is not a high-priority to-do.
        //mCPUExceptionId          = 0;
        //mCPUExceptionDetail      = 0;
        //mpExceptionMemoryAddress = NULL;

        // We do nothing more than make an admittedly fake description.
        mExceptionInfoWin32.mExceptionDescription.sprintf("Simulated exception %d.", exceptionType);

        if(mpOwner)
        {
            // In the case of other platforms, we disable the exception handler here.
            // That would do no good on the Unix because exceptions are unilaterally
            // terminating events and if another exception occurred we would just be 
            // shut down by the system.

            mpOwner->NotifyClients(ExceptionHandler::kExceptionHandlingBegin);

            if(mpOwner->GetReportTypes() & ExceptionHandler::kReportTypeException)
                mpOwner->WriteExceptionReport();

            mpOwner->NotifyClients(ExceptionHandler::kExceptionHandlingEnd);
        }

        mbHandlingInProgress.SetValue(0);
    }

    if(mAction == ExceptionHandler::kActionTerminate)
        ::exit(mnTerminateReturnValue);

    if(mAction == ExceptionHandler::kActionThrow)
    {
        // To do: To propertly implement this depends on mMode. In practice this simulation
        // functionality is more meant for exercizing the above report writing and so it 
        // may not be important to implement this throw.
    }

    // kActionContinue, kActionDefault
    // Else let the execution continue from the catch function. This is an exception
    // simulation function so there's no need to do anything drastic.
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionCallstack
//
size_t ExceptionHandlerWin32::GetExceptionCallstack(void* pCallstackArray, size_t capacity) const
{
    if(capacity > mExceptionInfoWin32.mCallstackEntryCount)
        capacity = mExceptionInfoWin32.mCallstackEntryCount;
    
    memmove(pCallstackArray, mExceptionInfoWin32.mCallstack, capacity * sizeof(void*));

    return capacity;
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionContext
//
const EA::Callstack::Context* ExceptionHandlerWin32::GetExceptionContext() const
{
    return &mExceptionInfoWin32.mContext;
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionThreadId
//
EA::Thread::ThreadId ExceptionHandlerWin32::GetExceptionThreadId(EA::Thread::SysThreadId& sysThreadId, char* threadName, size_t threadNameCapacity) const
{
    sysThreadId = mExceptionInfoWin32.mSysThreadId;
    EA::StdC::Strlcpy(threadName, mExceptionInfoWin32.mThreadName, threadNameCapacity);

    return mExceptionInfoWin32.mThreadId;
}



///////////////////////////////////////////////////////////////////////////////
// RunModeCPPTrapped
//
intptr_t ExceptionHandlerWin32::RunModeCPPTrapped(ExceptionHandler::UserFunctionUnion userFunctionUnion, ExceptionHandler::UserFunctionType userFunctionType, void* pContext, bool& exceptionCaught)
{
    exceptionCaught = false; // False until proven true.

    // The user has compiler exceptions disabled, but is using kModeCPP. This is a contradiction
    // and there is no choice but to skip using C++ exception handling. We could possibly assert
    // here but it might be annoying to the user.
    #if defined(EA_COMPILER_NO_EXCEPTIONS)
        return ExceptionHandler::ExecuteUserFunction(userFunctionUnion, userFunctionType, pContext);
    #else
        try
        {
            return ExceptionHandler::ExecuteUserFunction(userFunctionUnion, userFunctionType, pContext);
        }
        catch(std::exception& e)
        {
            HandleCaughtCPPException(e);
        }
        // The following catch(...) will catch all C++ language exceptions (i.e. C++ throw), 
        // and with some compilers (e.g. VC++) it can catch processor or system exceptions 
        // as well, depending on the compile/link settings.
        catch(...)
        {
            std::runtime_error e(std::string("generic"));
            HandleCaughtCPPException(e);
        }

        exceptionCaught = true;
        return 0;
    #endif
}


///////////////////////////////////////////////////////////////////////////////
// RunTrapped
//
intptr_t ExceptionHandlerWin32::RunTrapped(ExceptionHandler::UserFunctionUnion userFunctionUnion, ExceptionHandler::UserFunctionType userFunctionType, void* pContext, bool& exceptionCaught)
{
    intptr_t returnValue = 0;

    exceptionCaught = false;

    if(mbEnabled)
    {
        mbExceptionOccurred = false;

        if(mMode == ExceptionHandler::kModeCPP)
        {
            // VC++ requires the following to be executed in a separate function, due to the user of __try below.
            returnValue = RunModeCPPTrapped(userFunctionUnion, userFunctionType, pContext, exceptionCaught);
        }
        else if(mMode == ExceptionHandler::kModeStackBased)
        {
            // The Win32ExceptionFilter call below returns one of:
            //    EXCEPTION_CONTINUE_EXECUTION  // This would be used by code to attempt to re-execute the failed instruction.
            //    EXCEPTION_CONTINUE_SEARCH     // If this is returned, it means to skip us and see if somebody else has a handler installed.
            //    EXCEPTION_EXECUTE_HANDLER     // If this is returned, then the code within the __except block is executed.

            __try
            {
                returnValue = ExceptionHandler::ExecuteUserFunction(userFunctionUnion, userFunctionType, pContext);
            }
            __except(Win32ExceptionFilter(GetExceptionInformation()))
            {
                const DWORD dwExceptionCode = GetExceptionCode();

                if(dwExceptionCode == EXCEPTION_STACK_OVERFLOW)
                {
                    // We call _resetstkoflw() in order that we can possibly 
                    // continue execution here without the app being in a broken state.
                    // The _resetstkoflw function recovers from a stack overflow 
                    // condition, allowing a program to continue instead of failing 
                    // with a fatal exception error. If the _resetstkoflw function 
                    // is not called, there are no guard page after the previous 
                    // exception. The next time that there is a stack overflow, there 
                    // are no exception at all and the process terminates without warning.

                    #if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
                        _resetstkoflw();
                    #endif
                }

                exceptionCaught = true;
            }
        }
        else // kModeVectored
        {
            // If there's an exception then we'll catch it asynchronously (kModeSignalHandling) or in another thread (kModeVectored).
            returnValue = ExceptionHandler::ExecuteUserFunction(userFunctionUnion, userFunctionType, pContext);
        }
    }
    else
        returnValue = ExceptionHandler::ExecuteUserFunction(userFunctionUnion, userFunctionType, pContext);

    mbExceptionOccurred = exceptionCaught;
    return returnValue;
}


void ExceptionHandlerWin32::HandleCaughtCPPException(std::exception& e)
{    
    if(mbHandlingInProgress.SetValueConditional(1, 0)) // Set it true (1) from false (0) atomically, evaluating as true if somebody else didn't do it first.
    {
        mbExceptionOccurred = true;

        // It's not possible to tell where the exception occurred with C++ exceptions, unless the caller provides info via 
        // a custom exception class or at least via the exception::what() function. To consider: Make a standardized 
        // Exception C++ class for this package which users can use and provides some basic info, including location 
        // and possibly callstack.
        mExceptionInfoWin32.mpExceptionInstructionAddress = NULL;
        mExceptionInfoWin32.mExceptionDescription.assign(e.what());

        // There is currently no CPU context information for a C++ exception.
        memset(&mExceptionInfoWin32.mContext, 0, sizeof(mExceptionInfoWin32.mContext));

        // Save the thread id.
        mExceptionInfoWin32.mThreadId      = EA::Thread::GetThreadId(); // C++ exception handling occurs in the same thread as the exception occurred.
        mExceptionInfoWin32.mSysThreadId   = EA::Thread::GetSysThreadId(mExceptionInfoWin32.mThreadId);
        EA::Debug::GetThreadName(mExceptionInfoWin32.mThreadId, mExceptionInfoWin32.mSysThreadId, mExceptionInfoWin32.mThreadName, EAArrayCount(mExceptionInfoWin32.mThreadName));

        // Record exception time
        const time_t timeValue = time(NULL);
        mExceptionInfoWin32.mTime = *localtime(&timeValue);

        // Record the exception callstack
        // Unfortunately, this will return the callstack where we are now and not where the throw was.
        mExceptionInfoWin32.mCallstackEntryCount = EA::Thread::GetCallstack(mExceptionInfoWin32.mCallstack, EAArrayCount(mExceptionInfoWin32.mCallstack), NULL); // C++ exception handling occurs in the same thread as the exception occurred.

        if(mpOwner)
        {
            // Notify clients about exception handling start.
            mpOwner->NotifyClients(ExceptionHandler::kExceptionHandlingBegin);

            // Write exception report
            if(mpOwner->GetReportTypes() & ExceptionHandler::kReportTypeException)
                mpOwner->WriteExceptionReport();

            // Notify clients about exception handling end.
            mpOwner->NotifyClients(ExceptionHandler::kExceptionHandlingEnd);
        }

        mbHandlingInProgress = 0;
    }

    if(mAction == ExceptionHandler::kActionTerminate)
        ::exit(mnTerminateReturnValue);

    if(mAction == ExceptionHandler::kActionThrow)
    {
        #if defined(EA_COMPILER_NO_EXCEPTIONS)
            EA_FAIL_MESSAGE("ExceptionHandlerApple::HandleCaughtCPPException: Unable to throw, as compiler C++ exception support is disabled.");
        #else
            throw(e);
        #endif
    }

    // Else let the execution continue from the catch function.
}


///////////////////////////////////////////////////////////////////////////////
// ExceptionFilter
//
int ExceptionHandlerWin32::ExceptionFilter(LPEXCEPTION_POINTERS pExceptionPointers)
{
    // If the exception is the special exception "0x406d1388" then it means the user is intentionally calling it 
    // to send a message to the debugger to name the thread. http://blogs.msdn.com/b/stevejs/archive/2005/12/19/505815.aspx
    if(pExceptionPointers->ExceptionRecord->ExceptionCode == 0x406d1388)
        return EXCEPTION_CONTINUE_SEARCH;

    #ifdef DEBUGGERCRASHDIALOG_ENABLE
        MessageBoxA(NULL, "Attached with debugger.", "A Crash Occurred", MB_OK);
    #endif

    if(mbHandlingInProgress.SetValueConditional(1, 0)) // Set it true (1) from false (0) atomically, evaluating as true if somebody else didn't do it first.
    {
        mbExceptionOccurred = true;

        mExceptionInfoWin32.mpExceptionPointers = pExceptionPointers;
        mExceptionInfoWin32.mpExceptionInstructionAddress  = pExceptionPointers->ExceptionRecord->ExceptionAddress;
        // Shouldn't ExceptionAddress == ExceptionPointers->ContextRecord->Eip?

        EA_ASSERT(sizeof(*pExceptionPointers->ContextRecord) >= sizeof(mExceptionInfoWin32.mContext)); 
        memcpy(&mExceptionInfoWin32.mContext, pExceptionPointers->ContextRecord, sizeof(mExceptionInfoWin32.mContext));

        // Save the thread id.
        mExceptionInfoWin32.mThreadId      = EA::Thread::GetThreadId();
        mExceptionInfoWin32.mSysThreadId   = EA::Thread::GetSysThreadId(mExceptionInfoWin32.mThreadId);
        EA::Debug::GetThreadName(mExceptionInfoWin32.mThreadId, mExceptionInfoWin32.mSysThreadId, mExceptionInfoWin32.mThreadName, EAArrayCount(mExceptionInfoWin32.mThreadName));

        // Record exception time
        const time_t timeValue = time(NULL);
        mExceptionInfoWin32.mTime = *localtime(&timeValue);

        // Record the exception callstack
        EA::Callstack::CallstackContext callstackContext;
        EA::Callstack::GetCallstackContext(callstackContext, GetExceptionContext());
        mExceptionInfoWin32.mCallstackEntryCount = EA::Thread::GetCallstack(mExceptionInfoWin32.mCallstack, EAArrayCount(mExceptionInfoWin32.mCallstack), &callstackContext);

        // Record the exception description
        GenerateExceptionDescription();

        if(mpOwner)
        {
            // Protect against recursive crash by temporarily uninstalling the exception handler.
            // pCurrentHandler is most likely this handler.
            LPTOP_LEVEL_EXCEPTION_FILTER pCurrentHandler = NULL;

          #if EA_EXCEPTIONHANDLER_VECTORED_HANDLING_SUPPORTED
            ULONG result = 0;
          #endif

            if(mMode == ExceptionHandler::kModeStackBased)
                pCurrentHandler = SetUnhandledExceptionFilter(mPreviousFilter);
          #if EA_EXCEPTIONHANDLER_VECTORED_HANDLING_SUPPORTED
            else if(mMode == ExceptionHandler::kModeVectored)
            {
                result = RemoveVectoredExceptionHandler(mpVectoredHandle);
                EA_ASSERT(result != 0);
            }
          #endif

            // Notify clients about exception handling start.
            mpOwner->NotifyClients(ExceptionHandler::kExceptionHandlingBegin);

            // Write exception report
            if(mpOwner->GetReportTypes() & ExceptionHandler::kReportTypeException)
                mpOwner->WriteExceptionReport();

            // Write minidump
            if(mpOwner->GetReportTypes() & ExceptionHandler::kReportTypeMinidump)
                WriteMiniDump(mpOwner->GetMiniDumpFlags());

            // Notify clients about exception handling end.
            mpOwner->NotifyClients(ExceptionHandler::kExceptionHandlingEnd);

            // Reinstate current handler
            if(mMode == ExceptionHandler::kModeStackBased)
                SetUnhandledExceptionFilter(pCurrentHandler);
          #if EA_EXCEPTIONHANDLER_VECTORED_HANDLING_SUPPORTED
            else if((mMode == ExceptionHandler::kModeVectored) && (result != 0))
                mpVectoredHandle = AddVectoredExceptionHandler(1, Win32ExceptionFilter);
          #endif

            #ifdef NOTIFYCRASHDIALOG_ENABLE
                MessageBoxA(NULL, "After closing this message box, please zip up the html/mdmp files in \"C:\\ProgramData\\Origin\\Logs\" and email to OriginClientDev@ea.com.  Thanks!", "A Crash Occurred", MB_OK);
            #endif
        }

        mbHandlingInProgress = 0;
    }

    if(mAction == ExceptionHandler::kActionTerminate)
    {
        TerminateProcess(GetCurrentProcess(), (UINT)(unsigned)mnTerminateReturnValue);
        return mnTerminateReturnValue; // It doesn't matter what we return here.
    }

    // See if a debugger want to take another shot at this one
    #if 0 // We may or may not want this functionality. The debugger normally just tries to re-execute the instruction. 
        if(IsDebuggerPresent())
        {
            if(mPreviousFilter)
                return mPreviousFilter(pExceptionPointers);

            return EXCEPTION_CONTINUE_SEARCH; // If the debugger is present, we ignore mAction and just always continue so the debugger can catch it.
        }
    #endif

    if(mAction == ExceptionHandler::kActionThrow)
        return EXCEPTION_CONTINUE_SEARCH;

    // ExceptionHandler::kActionContinue
    return EXCEPTION_EXECUTE_HANDLER;

    // We don't have a means to specify EXCEPTION_CONTINUE_EXECUTION, which means to restart the instruction.
}


///////////////////////////////////////////////////////////////////////////////
// WriteMiniDump
//
void ExceptionHandlerWin32::WriteMiniDump(int miniDumpType)
{
    #if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP) || defined(EA_PLATFORM_CONSOLE)

        typedef BOOL (WINAPI* MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE dumpType, CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);
        MINIDUMPWRITEDUMP pMiniDumpWriteDump = NULL;

        #if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
            HMODULE hModuleDbgHelp = LoadLibraryW(L"DbgHelp.dll");
        #else
            HMODULE hModuleDbgHelp = LoadLibraryW(L"toolhelpx.dll");
        #endif

        if(hModuleDbgHelp)
            pMiniDumpWriteDump = (MINIDUMPWRITEDUMP)(void*)GetProcAddress(hModuleDbgHelp, "MiniDumpWriteDump");

        if(pMiniDumpWriteDump)
        {
            mpOwner->GetCurrentReportFilePath(mMiniDumpPath, EAArrayCount(mMiniDumpPath), ExceptionHandler::kReportTypeMinidump);
            wchar_t pathW[EA::IO::kMaxPathLength];
            EA::StdC::Strlcpy(pathW, mMiniDumpPath, EAArrayCount(pathW));

            HANDLE hFile = CreateFileW(pathW, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_FLAG_WRITE_THROUGH, 0);

            if(hFile != INVALID_HANDLE_VALUE)
            {
                MINIDUMP_EXCEPTION_INFORMATION exceptionInfo = { GetCurrentThreadId(), mExceptionInfoWin32.mpExceptionPointers, TRUE };

                BOOL result = pMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile,
                                                 (MINIDUMP_TYPE)miniDumpType, &exceptionInfo, 
                                                 (CONST PMINIDUMP_USER_STREAM_INFORMATION)NULL, (CONST PMINIDUMP_CALLBACK_INFORMATION)NULL);
                EA_ASSERT_FORMATTED(result, ("ExceptionHandlerWin32::WriteMiniDump: Failed to write minidump file at %s", mMiniDumpPath));

                if(result)
                {
                    LARGE_INTEGER fileSize;
                    result = GetFileSizeEx(hFile, &fileSize); EA_UNUSED(result);

                    EA_ASSERT_FORMATTED(result && (fileSize.QuadPart > 0), ("ExceptionHandlerWin32::WriteMiniDump: Failed to write minidump file at %s", mMiniDumpPath));
                }

                FlushFileBuffers(hFile); // This is not likely needed for desktop Windows, as it gracefully closes files if an app doesn't do so.
                CloseHandle(hFile);
                hFile = 0;
            }
            else
                EA_FAIL_FORMATTED(("ExceptionHandlerWin32::WriteMiniDump: Failed to create minidump file at %s", mMiniDumpPath));
        }

        FreeLibrary(hModuleDbgHelp);
    #else
        EA_UNUSED(miniDumpType);
    #endif
}


// GenerateExceptionDescription
//
void ExceptionHandlerWin32::GenerateExceptionDescription()
{
    DWORD dwCode = (mExceptionInfoWin32.mpExceptionPointers && mExceptionInfoWin32.mpExceptionPointers->ExceptionRecord) ? mExceptionInfoWin32.mpExceptionPointers->ExceptionRecord->ExceptionCode : 0;

    // There is some extra information available for AV exception.
    if(dwCode == EXCEPTION_ACCESS_VIOLATION)
    {
        mExceptionInfoWin32.mExceptionDescription.sprintf("ACCESS_VIOLATION %s address 0x%08x",
                   mExceptionInfoWin32.mpExceptionPointers->ExceptionRecord->ExceptionInformation[0] ? "writing" : "reading", 
                   mExceptionInfoWin32.mpExceptionPointers->ExceptionRecord->ExceptionInformation[1]);
    }
    else
    {
        mExceptionInfoWin32.mExceptionDescription.clear();

        // Process "standard" exceptions, other than 'access violation'
        #define FORMAT_EXCEPTION(x)                             \
            case EXCEPTION_##x:                                 \
                mExceptionInfoWin32.mExceptionDescription = #x; \
                break;

        switch(dwCode)
        {
          //FORMAT_EXCEPTION(ACCESS_VIOLATION) Already handled above.
            FORMAT_EXCEPTION(DATATYPE_MISALIGNMENT)
            FORMAT_EXCEPTION(BREAKPOINT)
            FORMAT_EXCEPTION(SINGLE_STEP)
            FORMAT_EXCEPTION(ARRAY_BOUNDS_EXCEEDED)
            FORMAT_EXCEPTION(FLT_DENORMAL_OPERAND)
            FORMAT_EXCEPTION(FLT_DIVIDE_BY_ZERO)
            FORMAT_EXCEPTION(FLT_INEXACT_RESULT)
            FORMAT_EXCEPTION(FLT_INVALID_OPERATION)
            FORMAT_EXCEPTION(FLT_OVERFLOW)
            FORMAT_EXCEPTION(FLT_STACK_CHECK)
            FORMAT_EXCEPTION(FLT_UNDERFLOW)
            FORMAT_EXCEPTION(INT_DIVIDE_BY_ZERO)
            FORMAT_EXCEPTION(INT_OVERFLOW)
            FORMAT_EXCEPTION(PRIV_INSTRUCTION)
            FORMAT_EXCEPTION(IN_PAGE_ERROR)
            FORMAT_EXCEPTION(ILLEGAL_INSTRUCTION)
            FORMAT_EXCEPTION(NONCONTINUABLE_EXCEPTION)
            FORMAT_EXCEPTION(STACK_OVERFLOW)
            FORMAT_EXCEPTION(INVALID_DISPOSITION)
            FORMAT_EXCEPTION(GUARD_PAGE)
            FORMAT_EXCEPTION(INVALID_HANDLE)
          #if defined(EXCEPTION_POSSIBLE_DEADLOCK) && defined(STATUS_POSSIBLE_DEADLOCK) // This type seems to be non-existant in practice.
            FORMAT_EXCEPTION(POSSIBLE_DEADLOCK)
          #endif
        }

        // If not one of the "known" exceptions, try to get the string from NTDLL.DLL's message table.
        if(mExceptionInfoWin32.mExceptionDescription.empty())
        {
            #if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
                char  buffer[256];
                DWORD capacity = EAArrayCount(buffer);

                const size_t length = (size_t)FormatMessageA(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE, 
                                                             GetModuleHandleA("NTDLL.DLL"), dwCode, 0, buffer, capacity, NULL);
                if(length)
                    mExceptionInfoWin32.mExceptionDescription.sprintf("%s at instruction %#p", buffer, mExceptionInfoWin32.mpExceptionMemoryAddress);
            #endif

            // If everything else failed just show the hex code.
            if(mExceptionInfoWin32.mExceptionDescription.empty())
                mExceptionInfoWin32.mExceptionDescription.sprintf("Unknown exception 0x%08x at instruction %#p", dwCode, mExceptionInfoWin32.mpExceptionMemoryAddress);
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionDescription
//
size_t ExceptionHandlerWin32::GetExceptionDescription(char* buffer, size_t capacity)
{
    return EA::StdC::Strlcpy(buffer, mExceptionInfoWin32.mExceptionDescription.c_str(), capacity);
}


} // namespace Debug

} // namespace EA


#endif  // #if defined(EA_PLATFORM_WIN32)





