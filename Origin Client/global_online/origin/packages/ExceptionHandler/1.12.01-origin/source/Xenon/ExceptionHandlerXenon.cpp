///////////////////////////////////////////////////////////////////////////////
// ExceptionHandlerXenon.cpp
// 
// Copyright (c) 2005, Electronic Arts. All Rights Reserved.
// Written and maintained by Paul Pedriana.
//
///////////////////////////////////////////////////////////////////////////////


#include <ExceptionHandler/Xenon/ExceptionHandlerXenon.h>


// Xenon-specific implemenation. Make sure nobody accidentally sticks it into wrong project
#if !defined(EA_PLATFORM_XENON)
    #error Xenon platform is required for this implementation
#endif

#include <ExceptionHandler/ReportWriter.h>

#pragma warning(push, 0)
#include <malloc.h> // For _resetstkoflw
#include <string.h>
#include <stdexcept>
#include <xtl.h>
#if EA_XBDM_ENABLED
    #include <xbdm.h>
    // #pragma comment(lib, "xbdm.lib") The application is expected to provide this if needed. 
#endif
#pragma warning(pop)
#pragma warning(disable: 4509) // nonstandard extension used: 'x' uses SEH and 'y' has destructor



namespace EA {

namespace Debug {


// To consider: Make this not be a single global but instead some kind of 
// thread-specific stack of pointers. That way we could have more than one
// exception handler at a time.
static ExceptionHandlerXenon* mpThis = NULL;


///////////////////////////////////////////////////////////////////////////////
// Win32ExceptionFilter
//
// We have a problem here in that we are limited to a single global exception 
// handler. This is primarily because Microsoft's APIs provide no means to 
// associate a context for the callback we get. We could probably work around 
// this by providing multiple Win32ExceptionFilter function implementations, 
// as odd as that may look.
//
LONG WINAPI Win32ExceptionFilter(LPEXCEPTION_POINTERS excPtrs)
{
    if(mpThis)
        return (LONG)mpThis->ExceptionFilter(excPtrs);
    return EXCEPTION_CONTINUE_SEARCH;
}


///////////////////////////////////////////////////////////////////////////////
// ExceptionHandlerXenon
//
ExceptionHandlerXenon::ExceptionHandlerXenon(ExceptionHandler* pOwner) :
    mpOwner(pOwner),
    mbEnabled(false),
    mbExceptionOccurred(false),
    mbHandlingInProgress(0),
    mMode(ExceptionHandler::kModeStackBased),
    mAction(ExceptionHandler::kActionDefault),
    mnTerminateReturnValue(ExceptionHandler::kDefaultTerminationReturnValue),
    mPreviousFilter(NULL),
    mExceptionInfoXenon()
{
    EA_ASSERT(mpThis == NULL);
    mpThis = this;

    // For backward compatibility, we default to kModeStackBased and we SetEnabled(true).
    SetMode(ExceptionHandler::kModeStackBased);
    SetEnabled(true);
}


///////////////////////////////////////////////////////////////////////////////
// ~ExceptionHandlerXenon
//
ExceptionHandlerXenon::~ExceptionHandlerXenon()
{
    mpThis = NULL;
    SetEnabled(false);
}


///////////////////////////////////////////////////////////////////////////////
// SetEnabled
//
bool ExceptionHandlerXenon::SetEnabled(bool state)
{
    switch (mMode)
    {
        case ExceptionHandler::kModeVectored:
        {
            EA_FAIL_MESSAGE("ExceptionHandlerXenon::SetEnabled: kModeVectored selected but not supported by the OS.");
            return false;
        }

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
                EA_FAIL_MESSAGE("ExceptionHandlerXenon::SetEnabled: kModeCPP selected, but the exception handling compiler support is disabled.");
            #endif
            
            mbEnabled = state;
            return true;
        }

        case ExceptionHandler::kModeSignalHandling:
        {
            EA_FAIL_MESSAGE("ExceptionHandlerXenon::SetEnabled: kModeSignalHandling selected but not supported by the OS.");
            return false;
        }

        default:
            return false;
    }

    return (mbEnabled == state);
}


///////////////////////////////////////////////////////////////////////////////
// IsEnabled
//
bool ExceptionHandlerXenon::IsEnabled() const
{
    return mbEnabled;
}


///////////////////////////////////////////////////////////////////////////////
// SetMode
//
bool ExceptionHandlerXenon::SetMode(ExceptionHandler::Mode mode)
{
    bool originallyEnabled = mbEnabled;

    // If the mode is being changed, disable the current mode first.
    if(mbEnabled && (mode != mMode))
        SetEnabled(false);

    if(!mbEnabled) // If we were already disabled or were able to disable it above...
    {
        if(mode == ExceptionHandler::kModeDefault)
            mode = ExceptionHandler::kModeStackBased;
        
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
void ExceptionHandlerXenon::SetAction(ExceptionHandler::Action action, int returnValue)
{
     mAction = action;
     mnTerminateReturnValue = returnValue;
}


///////////////////////////////////////////////////////////////////////////////
// GetAction
//
ExceptionHandler::Action ExceptionHandlerXenon::GetAction(int* pReturnValue) const
{
     if(pReturnValue)
          *pReturnValue = mnTerminateReturnValue;
     return mAction;
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionCallstack
//
size_t ExceptionHandlerXenon::GetExceptionCallstack(void* pCallstackArray, size_t capacity) const
{
    if(capacity > mExceptionInfoXenon.mCallstackEntryCount)
        capacity = mExceptionInfoXenon.mCallstackEntryCount;
    
    memmove(pCallstackArray, mExceptionInfoXenon.mCallstack, capacity * sizeof(void*));

    return capacity;
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionContext
//
const EA::Callstack::Context* ExceptionHandlerXenon::GetExceptionContext() const
{
    return &mExceptionInfoXenon.mContext;
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionThreadId
//
EA::Thread::ThreadId ExceptionHandlerXenon::GetExceptionThreadId(EA::Thread::SysThreadId& sysThreadId, char* threadName, size_t threadNameCapacity) const
{
    sysThreadId = mExceptionInfoXenon.mSysThreadId;
    EA::StdC::Strlcpy(threadName, mExceptionInfoXenon.mThreadName, threadNameCapacity);

    return mExceptionInfoXenon.mThreadId;
}


///////////////////////////////////////////////////////////////////////////////
// SimulateExceptionHandling
//
void ExceptionHandlerXenon::SimulateExceptionHandling(int /*exceptionType*/)
{
    // To do: enable usage of exceptionType.

    memset(&mExceptionInfoXenon.mContext, 0, sizeof(mExceptionInfoXenon.mContext));

    {
        EA::Callstack::CallstackContext callstackContext;
        EA::Callstack::GetCallstackContext(callstackContext, (intptr_t)EA::Thread::GetThreadId()); // In theory we could use EA::Thread::kThreadIdCurrent here, but older versions of EAThread can't deal with that. Need EATHREAD_VERSION_N >= 12006
    
        mExceptionInfoXenon.mContext.mGpr[1] = callstackContext.mGPR1; /// General purpose register 1.
        mExceptionInfoXenon.mContext.mIar    = callstackContext.mIAR;  /// Instruction address pseudo-register.
    }

    mExceptionInfoXenon.mThreadId    = EA::Thread::GetThreadId();
    mExceptionInfoXenon.mSysThreadId = EA::Thread::GetSysThreadId(mExceptionInfoXenon.mThreadId);  // Get self as a mach kernel thread type.
    EA::Debug::GetThreadName(mExceptionInfoXenon.mThreadId, mExceptionInfoXenon.mSysThreadId, mExceptionInfoXenon.mThreadName, EAArrayCount(mExceptionInfoXenon.mThreadName));

    const time_t timeValue = time(NULL);
    mExceptionInfoXenon.mTime = *localtime(&timeValue);

    #if defined(EA_PROCESSOR_ARM) 
        mExceptionInfoXenon.mpExceptionInstructionAddress = (void*)(uintptr_t)mExceptionInfoXenon.mContext.mGpr[15];
    #elif defined(EA_PROCESSOR_X86) 
        mExceptionInfoXenon.mpExceptionInstructionAddress = (void*)(uintptr_t)mExceptionInfoXenon.mContext.Eip;
    #elif defined(EA_PROCESSOR_X86_64)
        mExceptionInfoXenon.mpExceptionInstructionAddress = (void*)(uintptr_t)mExceptionInfoXenon.mContext.Rip;
    #elif defined(EA_PROCESSOR_POWERPC)
        mExceptionInfoXenon.mpExceptionInstructionAddress = (void*)(uintptr_t)mExceptionInfoXenon.mContext.mIar;
    #endif    

    // To do: Fill out the following with appropriate fake values. This is not a high-priority to-do.
    //mCPUExceptionId          = 0;
    //mCPUExceptionDetail      = 0;
    //mpExceptionMemoryAddress = NULL;
            
    // To do
    // We do nothing more than make an admittedly fake description.
    //mExceptionInfoXenon.mExceptionDescription.sprintf("Simulated exception %d", exceptionType);

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
    
    if(mAction == ExceptionHandler::kActionTerminate)
        ::exit(mnTerminateReturnValue);

    if(mAction == ExceptionHandler::kActionThrow)
    {
        // To do: To propertly implement this depends on mMode. In practice this simulation
        // functionality is more meant for exercizing the above report writing and so it 
        // may not be important to implement this throw.
    }

    // kActionContinjue, kActionDefault
    // Else let the execution continue from the catch function. This is an exception
    // simulation function so there's no need to do anything drastic.
}


///////////////////////////////////////////////////////////////////////////////
// RunModeCPPTrapped
//
intptr_t ExceptionHandlerXenon::RunModeCPPTrapped(ExceptionHandler::UserFunctionUnion userFunctionUnion, ExceptionHandler::UserFunctionType userFunctionType, void* pContext, bool& exceptionCaught)
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


void ExceptionHandlerXenon::HandleCaughtCPPException(std::exception& e)
{    
    if(mbHandlingInProgress.SetValueConditional(1, 0)) // Set it true (1) from false (0) atomically, evaluating as true if somebody else didn't do it first.
    {
        mbExceptionOccurred = true;
            
        // It's not possible to tell where the exception occurred with C++ exceptions, unless the caller provides info via 
        // a custom exception class or at least via the exception::what() function. To consider: Make a standardized 
        // Exception C++ class for this package which users can use and provides some basic info, including location 
        // and possibly callstack.
        mExceptionInfoXenon.mpExceptionInstructionAddress = NULL;
        mExceptionInfoXenon.mExceptionDescription.assign(e.what());

        // There is currently no CPU context information for a C++ exception.
        memset(&mExceptionInfoXenon.mContext, 0, sizeof(mExceptionInfoXenon.mContext));

        // Save the thread id.
        mExceptionInfoXenon.mThreadId      = EA::Thread::GetThreadId(); // C++ exception handling occurs in the same thread as the exception occurred.
        mExceptionInfoXenon.mSysThreadId   = EA::Thread::GetSysThreadId(mExceptionInfoXenon.mThreadId);
        EA::Debug::GetThreadName(mExceptionInfoXenon.mThreadId, mExceptionInfoXenon.mSysThreadId, mExceptionInfoXenon.mThreadName, EAArrayCount(mExceptionInfoXenon.mThreadName));

        // Record exception time
        const time_t timeValue = time(NULL);
        mExceptionInfoXenon.mTime = *localtime(&timeValue);

        // Record the exception callstack
        EA::Callstack::CallstackContext callstackContext;
        EA::Callstack::GetCallstackContext(callstackContext, &mExceptionInfoXenon.mContext);
        mExceptionInfoXenon.mCallstackEntryCount = EA::Thread::GetCallstack(mExceptionInfoXenon.mCallstack, EAArrayCount(mExceptionInfoXenon.mCallstack), &callstackContext);

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
// RunTrapped
//
intptr_t ExceptionHandlerXenon::RunTrapped(ExceptionHandler::UserFunctionUnion userFunctionUnion, ExceptionHandler::UserFunctionType userFunctionType, void* pContext, bool& exceptionCaught)
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

                    _resetstkoflw();
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


///////////////////////////////////////////////////////////////////////////////
// ExceptionFilter
//
int ExceptionHandlerXenon::ExceptionFilter(LPEXCEPTION_POINTERS pExceptionPointers)
{
    // If the exception is the special exception "0x406d1388" then it means the user is intentionally calling it 
    // to send a message to the debugger to name the thread. http://blogs.msdn.com/b/stevejs/archive/2005/12/19/505815.aspx
    if(pExceptionPointers->ExceptionRecord->ExceptionCode == 0x406d1388)
        return EXCEPTION_CONTINUE_SEARCH;

    if(mbHandlingInProgress.SetValueConditional(1, 0)) // Set it true (1) from false (0) atomically, evaluating as true if somebody else didn't do it first.
    {
        mExceptionInfoXenon.mpExceptionPointers = pExceptionPointers;
        mExceptionInfoXenon.mpExceptionInstructionAddress  = pExceptionPointers->ExceptionRecord->ExceptionAddress;
        // Shouldn't ExceptionAddress == ExceptionPointers->ContextRecord->Eip?

        EA_ASSERT(sizeof(*pExceptionPointers->ContextRecord) >= sizeof(mExceptionInfoXenon.mContext)); 
        memcpy(&mExceptionInfoXenon.mContext, pExceptionPointers->ContextRecord, sizeof(mExceptionInfoXenon.mContext));

        // Save the thread id.
        mExceptionInfoXenon.mThreadId = EA::Thread::GetThreadId();
        mExceptionInfoXenon.mSysThreadId   = EA::Thread::GetSysThreadId(mExceptionInfoXenon.mThreadId);
        EA::Debug::GetThreadName(mExceptionInfoXenon.mThreadId, mExceptionInfoXenon.mSysThreadId, mExceptionInfoXenon.mThreadName, EAArrayCount(mExceptionInfoXenon.mThreadName));

        // Record exception time
        const time_t timeValue = time(NULL);
        mExceptionInfoXenon.mTime = *localtime(&timeValue);

        GenerateExceptionDescription();

        if(mpOwner)
        {
            // Protect against recursive crash
            // pCurrentHandler is most likely this handler
            LPTOP_LEVEL_EXCEPTION_FILTER pCurrentHandler = SetUnhandledExceptionFilter(mPreviousFilter);

            // Notify clients about exception handling start.
            mpOwner->NotifyClients(ExceptionHandler::kExceptionHandlingBegin);

            // Write exception report
            if(mpOwner->GetReportTypes() & ExceptionHandler::kReportTypeException)
                mpOwner->WriteExceptionReport();

            // Write minidump
            if(mpOwner->GetReportTypes() & ExceptionHandler::kReportTypeMinidump)
                WriteMiniDump();

            // Notify clients about exception handling end.
            mpOwner->NotifyClients(ExceptionHandler::kExceptionHandlingEnd);

            // Reinstate current handler
            SetUnhandledExceptionFilter(pCurrentHandler);
        }

        mbHandlingInProgress.SetValue(0);
    }

    if(mAction == ExceptionHandler::kActionTerminate)
    {
        #if EA_XBDM_ENABLED
            DmReboot(DMBOOT_COLD);
        #endif

        return mnTerminateReturnValue; // It doesn't matter what we return here.
    }

    #if 0 // We may or may not want this functionality. The debugger normally just tries to re-execute the instruction. 
        if(DmIsDebuggerPresent())
        {
            if(mPreviousFilter)
                return mPreviousFilter(pExceptionPointers);
            return EXCEPTION_CONTINUE_SEARCH;
        }
    #endif

    if(mAction == ExceptionHandler::kActionThrow)
        return EXCEPTION_CONTINUE_SEARCH;

    // ExceptionHandler::kActionContinue
    // We don't have a means to specify EXCEPTION_CONTINUE_EXECUTION, which means to restart the instruction.
    return EXCEPTION_EXECUTE_HANDLER;
}


///////////////////////////////////////////////////////////////////////////////
// WriteMiniDump
//
void ExceptionHandlerXenon::WriteMiniDump()
{
    // Creates a file on the console at e:\dumps\[main module]\[crashed module][i].dmp
    // On XBox 360 (Xenon) there are no flags to control how mini-dumps are created.
    #if EA_XBDM_ENABLED
        DmCrashDump(TRUE);
    #endif
}


void ExceptionHandlerXenon::GenerateExceptionDescription()
{
    DWORD dwCode = (mExceptionInfoXenon.mpExceptionPointers && mExceptionInfoXenon.mpExceptionPointers->ExceptionRecord) ? mExceptionInfoXenon.mpExceptionPointers->ExceptionRecord->ExceptionCode : 0;

    // There is some extra information available for AV exception.
    if(dwCode == EXCEPTION_ACCESS_VIOLATION)
    {
        mExceptionInfoXenon.mExceptionDescription.sprintf("ACCESS_VIOLATION %s address 0x%08x",
                   mExceptionInfoXenon.mpExceptionPointers->ExceptionRecord->ExceptionInformation[0] ? "writing" : "reading", 
                   mExceptionInfoXenon.mpExceptionPointers->ExceptionRecord->ExceptionInformation[1]);
    }
    else
    {
        mExceptionInfoXenon.mExceptionDescription.clear();

        // Process "standard" exceptions, other than 'access violation'
        #define FORMAT_EXCEPTION(x)                             \
            case EXCEPTION_##x:                                 \
                mExceptionInfoXenon.mExceptionDescription = #x; \
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
        if(mExceptionInfoXenon.mExceptionDescription.empty())
        {
            #if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
                char  buffer[256];
                DWORD capacity = EAArrayCount(buffer);

                const size_t length = (size_t)FormatMessageA(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE, 
                                                             GetModuleHandleA("NTDLL.DLL"), dwCode, 0, buffer, capacity, NULL);
                if(length)
                    mExceptionInfoXenon.mExceptionDescription.sprintf("%s at instruction %#p", buffer, mExceptionInfoXenon.mpExceptionMemoryAddress);
            #endif

            // If everything else failed just show the hex code.
            if(mExceptionInfoXenon.mExceptionDescription.empty())
                mExceptionInfoXenon.mExceptionDescription.sprintf("Unknown exception 0x%08x at instruction %#p", dwCode, mExceptionInfoXenon.mpExceptionMemoryAddress);
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionDescription
//
size_t ExceptionHandlerXenon::GetExceptionDescription(char* buffer, size_t capacity)
{
    return EA::StdC::Strlcpy(buffer, mExceptionInfoXenon.mExceptionDescription.c_str(), capacity);
}


} // namespace Debug

} // namespace EA











