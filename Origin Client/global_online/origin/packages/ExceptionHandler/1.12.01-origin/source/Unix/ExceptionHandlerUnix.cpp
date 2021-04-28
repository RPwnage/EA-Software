///////////////////////////////////////////////////////////////////////////////
// ExceptionHandlerUnix.cpp
// 
// Copyright (c) 2012, Electronic Arts. All Rights Reserved.
// Written and maintained by Paul Pedriana.
//
// Exception handling and reporting facilities.
///////////////////////////////////////////////////////////////////////////////


#include <ExceptionHandler/Unix/ExceptionHandlerUnix.h>


// Unix-specific implemenation. Make sure nobody accidentally sticks it into wrong project
#if defined(EA_PLATFORM_UNIX)

#include <EACallstack/EAAddressRep.h>
#include <EACallstack/EACallstack.h>
#include <EACallstack/EADasm.h>
#include <EACallstack/Context.h>
#include <eathread/eathread.h>
#include <EAStdC/EAString.h>
#include EA_ASSERT_HEADER


static const char* GetSigNumberString(int sigNumber)
{
    switch (sigNumber)
    {
        case SIGSEGV:
            return "SIGSEGV";

        case SIGILL:
            return "SIGILL";

        case SIGBUS:
            return "SIGBUS";

        case SIGFPE:
            return "SIGFPE";

        case SIGABRT:
            return "SIGABRT";

        case SIGQUIT:
            return "SIGQUIT";

        case SIGTERM:
            return "SIGTERM";

        case SIGSYS:
            return "SIGSYS";

        case SIGXCPU:
            return "SIGXCPU";

        default:
            return "SIG???";
    }
}


static const char* GetSIGILLCodeString(int siCode)
{
    switch (siCode)
    {
        case ILL_ILLOPC:
	        return "ILL_ILLOPC (illegal opcode)";
        case ILL_ILLOPN:
	        return "ILL_ILLOPN (illegal operand)";
        case ILL_ILLADR:
	        return "ILL_ILLADR (illegal addressing mode)";
        case ILL_ILLTRP:
	        return "ILL_ILLTRP (illegal trap)";
        case ILL_PRVOPC:
	        return "ILL_PRVOPC (privileged opcode)";
        case ILL_PRVREG:
	        return "ILL_PRVREG (privileged register)";
        case ILL_COPROC:
	        return "ILL_COPROC (coprocessor error)";
        case ILL_BADSTK:
    	    return "ILL_BADSTK (internal stack error)";
        default:
	        return "ILL_???";
    }
}


static const char* GetSIGFPECodeString(int siCode)
{
    switch (siCode)
    {
        case FPE_INTDIV:
    	    return "FPE_INTDIV (integer divide by zero)";
        case FPE_INTOVF:
	        return "FPE_INTOVF (integer overflow)";
        case FPE_FLTDIV:
	        return "FPE_FLTDIV (floating point divide by zero)";
        case FPE_FLTOVF:
	        return "FPE_FLTOVF (floating point overflow)";
        case FPE_FLTUND:
	        return "FPE_FLTUND (floating point underflow)";
        case FPE_FLTRES:
	        return "FPE_FLTRES (floating poing inexact result)";
        case FPE_FLTINV:
	        return "FPE_FLTINV (invalid Floating poing operation)";
        case FPE_FLTSUB:
	        return "FPE_FLTSUB (subscript out of range)";
        default:
	        return "FPE_???";
    }
}


static const char* GetSIGSEGVCodeString(int siCode)
{
    switch (siCode)
    {
        case SEGV_MAPERR:
    	    return "SEGV_MAPERR (address not mapped to object)";
        case SEGV_ACCERR:
	        return "SEGV_ACCERR (invalid permissions for mapped object)";
        default:
	        return "SEGV_???";
    }
}


static const char* GetSIGBUSCodeString(int siCode)
{
    switch (siCode)
    {
        case BUS_ADRALN:
    	    return "BUS_ADRALN (invalid address alignment)";
        case BUS_ADRERR:
	        return "BUS_ADRERR (non-existant physical address)";
        case BUS_OBJERR:
	        return "BUS_OBJERR (object specific hardware error)";
        default:
	        return "BUS_???";
    }
}


static const char* GetSIGTRAPCodeString(int siCode)
{
    switch (siCode)
    {
        case TRAP_BRKPT:
    	    return "TRAP_BRKPT (process breakpoint)";
        case TRAP_TRACE:
	        return "TRAP_TRACE (process trace trap)";
        default:
	        return "TRAP_???";
    }
}


static const char* GetCodeString(int sigNumber, int siCode)
{
    switch(sigNumber)
    {
        case SIGILL:
            return GetSIGILLCodeString(siCode);

        case SIGFPE:
            return GetSIGFPECodeString(siCode);

        case SIGSEGV:
            return GetSIGSEGVCodeString(siCode);

        case SIGBUS:
            return GetSIGBUSCodeString(siCode);

        case SIGTRAP:
            return GetSIGTRAPCodeString(siCode);

        // There are others, but they aren't relevant to us and we don't register for those values.
        default:
            return "<unknown>";
    }
}


#if defined(EA_PLATFORM_BSD)
    static const char* GetCPUTrapCodeString(int trapNumber)
    {
        #if defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_X86_64)
            switch (trapNumber)
            {
                // Hardware traps
                case 0:
                    return "integer divide by zero";
                case 1:
                    return "debug exception";
                case 2:
                    return "non-maskable hardware interrupt";
                case 3:
                    return "int 3 breakpoint";
                case 4:
                    return "overflow";
                case 5:
                    return "bounds check failure";
                case 6:
                    return "invalid instruction";
                case 7:
                    return "coprocessor unavailable";
                case 8:
                    return "exception within exception";
              //case 9: // No longer used in modern hardware.
              //    return "coprocessor segment overrun";
                case 10:
                    return "invalid task switch";
                case 11:
                    return "segment not present";
                case 12:
                    return "stack exception";
                case 13:
                    return "general protection fault";
                case 14:
                    return "page fault";
                case 16:
                    return "floating point error";
                case 17:
                    return "aligment check";
                case 18:
                    return "machine check";
                case 19:
                    return "SIMD floating point error";

                // OS traps
                case 0x80:
                    return "sys call";
                case 0x81:
                    return "inter-processor interrupt";
                case 0x82:
                    return "inter-processor interrupt DOS"; // I have no idea what this one means.
            }
        
        #else
            EA_UNUSED(trapNumber);
        #endif

        return "unknown";
    }
#endif


namespace EA {
namespace Debug {


ExceptionHandlerUnix::ExceptionInfoUnix::ExceptionInfoUnix()
  : mLWPId(0),
    mSigNumber(0),
    mSigContext(),
    mSigInfo()
{
    memset(&mSigContext, 0, sizeof(mSigContext));
    memset(&mSigInfo,    0, sizeof(mSigInfo));
}


// To consider: Make this not be a single global but instead some kind of 
// thread-specific stack of pointers. That way we could have more than one
// exception handler at a time.
static ExceptionHandlerUnix* mpThis = NULL;


///////////////////////////////////////////////////////////////////////////////
// ExceptionHandlerUnix
//
ExceptionHandlerUnix::ExceptionHandlerUnix(ExceptionHandler* pOwner) :
    mpOwner(pOwner),
    mbEnabled(false),
    mbExceptionOccurred(false),
    mbHandlingInProgress(0),
    mMode(ExceptionHandler::kModeDefault),
    mAction(ExceptionHandler::kActionDefault),
    mnTerminateReturnValue(ExceptionHandler::kDefaultTerminationReturnValue),
    mExceptionInfoUnix()
{
    EA_ASSERT(mpThis == NULL);
    mpThis = this;

    // For backward compatibility, we default to kModeSignalHandling and we SetEnabled(true).
    SetMode(ExceptionHandler::kModeSignalHandling);
    SetEnabled(true);
}


///////////////////////////////////////////////////////////////////////////////
// ~ExceptionHandlerUnix
//
ExceptionHandlerUnix::~ExceptionHandlerUnix()
{
    mpThis = NULL;
    SetEnabled(false);
}


///////////////////////////////////////////////////////////////////////////////
// SetEnabled
//
bool ExceptionHandlerUnix::SetEnabled(bool state)
{
    if(state && !mbEnabled)
    {
        // See our BSD code.
    }
    else if(!state && mbEnabled)
    {
        mbEnabled = false;

        // See our BSD code.
    }

    return (mbEnabled == state);
}


///////////////////////////////////////////////////////////////////////////////
// IsEnabled
//
bool ExceptionHandlerUnix::IsEnabled() const
{
    return mbEnabled;
}


///////////////////////////////////////////////////////////////////////////////
// SetMode
//
bool ExceptionHandlerUnix::SetMode(ExceptionHandler::Mode mode)
{
    // We support the following:
    //     - C++ exception handling.
    //     - Posix sigint exception handling.
    
    EA_ASSERT(!mbEnabled); // Verify that it isn't already enabled and active.
    
    if(!mbEnabled) // If we were already disabled or were able to disable it above...
    {
        if((mode == ExceptionHandler::kModeStackBased) || 
           (mode == ExceptionHandler::kModeVectored)   ||
           (mode == ExceptionHandler::kModeDefault))
        {
            mode = ExceptionHandler::kModeDefault;
        }

        if(mode == ExceptionHandler::kModeDefault)
            mode = ExceptionHandler::kModeSignalHandling;
        
        // The user needs to call SetEnabled(true/false) in order to enable/disable handling in this mode.
        mMode = mode;
        return true;
    }

    return false;
}


///////////////////////////////////////////////////////////////////////////////
// SetAction
//
void ExceptionHandlerUnix::SetAction(ExceptionHandler::Action action, int returnValue)
{
    // Due to the way that Unix exception handling works, kActionTerminate is the 
    // only supported action. It is impossible to continue/recover from an exception
    // on the Unix and there is no way to re-throw an exception at the system level.

    mAction = action;
    mnTerminateReturnValue = returnValue;
}


///////////////////////////////////////////////////////////////////////////////
// GetAction
//
ExceptionHandler::Action ExceptionHandlerUnix::GetAction(int* pReturnValue) const
{
    if(pReturnValue)
        *pReturnValue = mnTerminateReturnValue;
    return mAction;
}


///////////////////////////////////////////////////////////////////////////////
// SimulateExceptionHandling
//
void ExceptionHandlerUnix::SimulateExceptionHandling(int /*exceptionType*/)
{
    // To do.
}


///////////////////////////////////////////////////////////////////////////////
// RunTrapped
//
intptr_t ExceptionHandlerUnix::RunTrapped(ExceptionHandler::UserFunctionUnion userFunctionUnion, ExceptionHandler::UserFunctionType userFunctionType, void* pContext, bool& exceptionCaught)
{
    intptr_t returnValue = 0;

    exceptionCaught = false;

    if(mbEnabled)
    {
        mbExceptionOccurred = false;

        if(mMode == ExceptionHandler::kModeCPP)
        {
            // The user has compiler exceptions disabled, but is using kModeCPP. This is a contradiction
            // and there is no choice but to skip using C++ exception handling. We could possibly assert
            // here but it might be annoying to the user.
        
            #if defined(EA_COMPILER_NO_EXCEPTIONS)
                returnValue = ExceptionHandler::ExecuteUserFunction(userFunctionUnion, userFunctionType, pContext);
            #else
                try
                {
                    returnValue = ExceptionHandler::ExecuteUserFunction(userFunctionUnion, userFunctionType, pContext);
                }
                catch(std::exception& e)
                {
                    HandleCaughtCPPException(e);
                    exceptionCaught = true;
                }
                // The following catch(...) will catch all C++ language exceptions (i.e. C++ throw), 
                // and with some compilers (e.g. VC++) it can catch processor or system exceptions 
                // as well, depending on the compile/link settings.
                catch(...)
                {
                    std::runtime_error e(std::string("generic"));
                    HandleCaughtCPPException(e);
                    exceptionCaught = true;
                }
            #endif
        }
        else // kModeSignalHandling, kModeVectored
        {
            // If there's an exception then we'll catch it asynchronously (kModeSignalHandling) or in another thread (kModeVectored).
            // Unix doesn't support try/catch style exception handling. We just call
            // the user function and if an exception occurs then our callback will find
            // out about it and the user will also find out about it if the RegisterClient
            // function was called.
            returnValue = ExceptionHandler::ExecuteUserFunction(userFunctionUnion, userFunctionType, pContext);
        }
    }
    else
        returnValue = ExceptionHandler::ExecuteUserFunction(userFunctionUnion, userFunctionType, pContext);

    mbExceptionOccurred = exceptionCaught;
    return returnValue;
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionCallstack
//
size_t ExceptionHandlerUnix::GetExceptionCallstack(void* pCallstackArray, size_t capacity) const
{
    if(capacity > mExceptionInfoUnix.mCallstackEntryCount)
        capacity = mExceptionInfoUnix.mCallstackEntryCount;
    
    memmove(pCallstackArray, mExceptionInfoUnix.mCallstack, capacity * sizeof(void*));

    return capacity;
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionContext
//
const EA::Callstack::Context* ExceptionHandlerUnix::GetExceptionContext() const
{
    return &mExceptionInfoUnix.mContext;
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionThreadId
//
EA::Thread::ThreadId ExceptionHandlerUnix::GetExceptionThreadId(EA::Thread::SysThreadId& sysThreadId, char* threadName, size_t threadNameCapacity) const
{
    sysThreadId = mExceptionInfoUnix.mSysThreadId;
    EA::StdC::Strlcpy(threadName, mExceptionInfoUnix.mThreadName, threadNameCapacity);

    return mExceptionInfoUnix.mThreadId;
}


///////////////////////////////////////////////////////////////////////////////
// GetSignalExceptionDescription
//
void ExceptionHandlerUnix::GetSignalExceptionDescription(String& sDescription)
{
    sDescription.clear();

    // http://linux.die.net/man/2/sigaction
    // http://www.manualpages.de/NetBSD/NetBSD-5.1/man2/siginfo.2.html

    sDescription.append_sprintf("Signal received: %s, %s\r\n", GetSigNumberString(mExceptionInfoUnix.mSigNumber), GetCodeString(mExceptionInfoUnix.mSigNumber, mExceptionInfoUnix.mSigInfo.si_code));
    sDescription.append_sprintf("Exception instruction address: %016I64x\r\n", (uint64_t)mExceptionInfoUnix.mpExceptionInstructionAddress);
    sDescription.append_sprintf("Thread id: %016I64x, LWP id: %I32u, name: %s\r\n", (uint64_t)mExceptionInfoUnix.mThreadId, mExceptionInfoUnix.mLWPId, mExceptionInfoUnix.mThreadName[0] ? mExceptionInfoUnix.mThreadName : "<unknown>");

    if(mExceptionInfoUnix.mSigInfo.si_errno)
        sDescription.append_sprintf("errno: %d\r\n", mExceptionInfoUnix.mSigInfo.si_errno);
    else
        sDescription.append_sprintf("errno: (unapplicable)\r\n");

    if((mExceptionInfoUnix.mSigNumber == SIGILL) || (mExceptionInfoUnix.mSigNumber == SIGFPE) || (mExceptionInfoUnix.mSigNumber == SIGTRAP) || (mExceptionInfoUnix.mSigNumber == SIGBUS) || (mExceptionInfoUnix.mSigNumber == SIGSEGV))
    {
        sDescription.append_sprintf("Fault memory address: %016I64x\r\n", (uint64_t)mExceptionInfoUnix.mSigInfo.si_addr);

        #if defined(EA_PLATFORM_BSD) // si_trapno exists only on BSD and a couple old irrelevant platforms under Linux.
            sDescription.append_sprintf("CPU-specific exception (trap): %d (%s)\r\n", mExceptionInfoUnix.mSigInfo.si_trapno, GetCPUTrapCodeString(mExceptionInfoUnix.mSigInfo.si_trapno));
        #endif
    }

    //sDescription.append_sprintf("Process id: %d, user id: %u\r\n", (int)mExceptionInfoUnix.mSigInfo.si_pid, (unsigned)mExceptionInfoUnix.mSigInfo.si_uid);            // Not relevant to the types of signals we handle.
    //sDescription.append_sprintf("User time: %d, process time: %u\r\n", (clock_t)mExceptionInfoUnix.mSigInfo.si_utime, (clock_t)mExceptionInfoUnix.mSigInfo.si_stime); // Some Unixes don't have this.
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionDescription
//
size_t ExceptionHandlerUnix::GetExceptionDescription(char* buffer, size_t capacity)
{
    return EA::StdC::Strlcpy(buffer, mExceptionInfoUnix.mExceptionDescription.c_str(), capacity);
}


///////////////////////////////////////////////////////////////////////////////
// ExceptionCallbackStatic
//
void ExceptionHandlerUnix::ExceptionCallbackStatic(uint64_t exceptionCause /*, sys_ppu_thread_t ppuThreadId, uint64_t dataAccessRegisterValue*/)
{
    if(mpThis)
        mpThis->ExceptionCallback(exceptionCause /*, ppuThreadId, dataAccessRegisterValue*/);
}


///////////////////////////////////////////////////////////////////////////////
// ExceptionCallback
//
// This function will be called when any exception occured.
//
void ExceptionHandlerUnix::ExceptionCallback(uint64_t /*exceptionCause*/ /*, uint64_t ppuThreadId, uint64_t dataAccessRegisterValue */)
{
    GetSignalExceptionDescription(mExceptionInfoUnix.mExceptionDescription);

    /*
        // See our BSD code. Need to support Linux too.
    */
}


} // namespace Debug

} // namespace EA


#endif  // #if defined(EA_PLATFORM_Unix)

















