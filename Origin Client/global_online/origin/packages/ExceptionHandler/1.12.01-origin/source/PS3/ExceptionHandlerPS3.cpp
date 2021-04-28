///////////////////////////////////////////////////////////////////////////////
// ExceptionHandlerPS3.cpp
// 
// Copyright (c) 2005, Electronic Arts. All Rights Reserved.
// Written and maintained by Paul Pedriana.
//
// Exception handling and reporting facilities.
///////////////////////////////////////////////////////////////////////////////


#include <ExceptionHandler/PS3/ExceptionHandlerPS3.h>


// PS3-specific implemenation. Make sure nobody accidentally sticks it into wrong project
#if defined(EA_PLATFORM_PS3)

#include <EACallstack/EAAddressRep.h>
#include <EACallstack/EACallstack.h>
#include <EACallstack/EADasm.h>
#include <EACallstack/Context.h>
#include <eathread/eathread.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EAStopwatch.h>
#include EA_ASSERT_HEADER

#include <sdk_version.h>
#include <sys/dbg.h>
#include <cell/sysmodule.h>
#include <sys/timer.h>



namespace EA {
namespace Debug {


// To consider: Make this not be a single global but instead some kind of 
// thread-specific stack of pointers. That way we could have more than one
// exception handler at a time.
static ExceptionHandlerPS3* mpThis = NULL;


///////////////////////////////////////////////////////////////////////////////
// GetCurrentCallstackContext
//
static void GetCurrentCallstackContext(EA::Callstack::Context& context)
{
    uintptr_t* pFrame;
    void*      pInstruction;

    // Load into pFrame (%0) the contents of what is pointed to by register 1 with 0 offset.
    #if defined(__SNC__)
        pFrame = (uintptr_t*)__builtin_frame_address();
    #else
        #if (EA_PLATFORM_PTR_SIZE == 8)
            __asm__ __volatile__("ld %0, 0(1)" : "=r" (pFrame) : );
        #else
            uint64_t  temp;
            __asm__ __volatile__("ld %0, 0(1)" : "=r" (temp) : );
            pFrame = (uintptr_t*)(uintptr_t)temp;
        #endif
    #endif

    pInstruction    = ({ __label__ label; label: &&label; }); // This is some crazy GCC code that happens to work.
    context.mGpr[1] = (uintptr_t)pFrame;
    context.mIar    = (uintptr_t)pInstruction;
}



///////////////////////////////////////////////////////////////////////////////
// ExceptionHandlerPS3
//
ExceptionHandlerPS3::ExceptionHandlerPS3(ExceptionHandler* pOwner) :
    mpOwner(pOwner),
    mbEnabled(false),
    mbExceptionOccurred(false),
    mbHandlingInProgress(0),
    mMode(ExceptionHandler::kModeDefault),
    mAction(ExceptionHandler::kActionDefault),
    mnTerminateReturnValue(ExceptionHandler::kDefaultTerminationReturnValue),
    mExceptionInfoPS3()
{
    EA_ASSERT(mpThis == NULL);
    mpThis = this;

    // For backward compatibility, we default to kModeVectored and we SetEnabled(true).
    SetMode(ExceptionHandler::kModeVectored);
    SetEnabled(true);
}


///////////////////////////////////////////////////////////////////////////////
// ~ExceptionHandlerPS3
//
ExceptionHandlerPS3::~ExceptionHandlerPS3()
{
    mpThis = NULL;
    SetEnabled(false);
}


///////////////////////////////////////////////////////////////////////////////
// SetEnabled
//
bool ExceptionHandlerPS3::SetEnabled(bool state)
{
    if(state && !mbEnabled)
    {
        // Question: What is the return value if the module is already loaded?
        int result = cellSysmoduleLoadModule(CELL_SYSMODULE_LV2DBG);
        EA_ASSERT(result == CELL_OK);

        if(result == CELL_OK) 
        {
            // To consider: Make the priority specified here configurable.
            result = sys_dbg_initialize_ppu_exception_handler(EA::Thread::kSysThreadPriorityDefault - 32); // Make it fairly high priority.

            if(result == CELL_OK)
            {
                // We have options of SYS_DBG_PPU_THREAD_STOP and SYS_DBG_SPU_THREAD_GROUP_STOP
                // here. It is potentially dangerous to stop other threads, as they may be holding
                // onto resources that will block our execution. So we set the flags to zero.
                result = sys_dbg_register_ppu_exception_handler(ExceptionCallbackStatic, 0);
                EA_ASSERT(result == CELL_OK);

                // We unilaterally set mbEnabled as true, regardless of results of the calls below.
                mbEnabled = (result == CELL_OK);
            }
            else
            {
                // On a retail consumer machine you will get result => CELL_LV2DBG_ERROR_DEOPERATIONDENIED.
                // Also, it turns out that even with development machines (DEH, DECR, DebugStation) you 
                // still need to manually enable Lv2Dbg functionality by running lv2dbgutil.self or booting
                // the dev machine with an lv2dbgutil.iso DVD. These are currently available from the Sony
                // developer PS3 site.
                // EA_ASSERT(result == CELL_OK); To consider: Enable this assert.
            }
        }
    }
    else if(!state && mbEnabled)
    {
        mbEnabled = false;

        sys_dbg_unregister_ppu_exception_handler();
        sys_dbg_finalize_ppu_exception_handler();
        cellSysmoduleUnloadModule(CELL_SYSMODULE_LV2DBG);
    }

    return (mbEnabled == state);
}


///////////////////////////////////////////////////////////////////////////////
// IsEnabled
//
bool ExceptionHandlerPS3::IsEnabled() const
{
    return mbEnabled;
}


///////////////////////////////////////////////////////////////////////////////
// SetMode
//
bool ExceptionHandlerPS3::SetMode(ExceptionHandler::Mode mode)
{
    // We support the following:
    //     - C++ exception handling.
    //     - Sony PS3-specific exception handling (kModeVectored).
    
    bool originallyEnabled = mbEnabled;

    // If the mode is being changed, disable the current mode first.
    if(mbEnabled && (mode != mMode))
        SetEnabled(false);
    
    if(!mbEnabled) // If we were already disabled or were able to disable it above...
    {
        // Sony doesn't provide a stack-based exception handling system like Microsoft
        // does, and so kModeStackBased isn't supported.
        if(mode == ExceptionHandler::kModeStackBased)
            mode = ExceptionHandler::kModeDefault;

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
void ExceptionHandlerPS3::SetAction(ExceptionHandler::Action action, int returnValue)
{
    // Due to the way that PS3 exception handling works, kActionTerminate is the 
    // only supported action. It is impossible to continue/recover from an exception
    // on the PS3 and there is no way to re-throw an exception at the system level.

    mAction = action;
    mnTerminateReturnValue = returnValue;
}


///////////////////////////////////////////////////////////////////////////////
// GetAction
//
ExceptionHandler::Action ExceptionHandlerPS3::GetAction(int* pReturnValue) const
{
    if(pReturnValue)
        *pReturnValue = mnTerminateReturnValue;
    return mAction;
}


///////////////////////////////////////////////////////////////////////////////
// RunTrapped
//
intptr_t ExceptionHandlerPS3::RunTrapped(ExceptionHandler::UserFunctionUnion userFunctionUnion, ExceptionHandler::UserFunctionType userFunctionType, void* pContext, bool& exceptionCaught)
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
            // The PS3 doesn't support try/catch style exception handling. We just call
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


void ExceptionHandlerPS3::HandleCaughtCPPException(std::exception& e)
{    
    if(mbHandlingInProgress.SetValueConditional(1, 0)) // Set it true (1) from false (0) atomically, evaluating as true if somebody else didn't do it first.
    {
        mbExceptionOccurred = true;
            
        // It's not possible to tell where the exception occurred with C++ exceptions, unless the caller provides info via 
        // a custom exception class or at least via the exception::what() function. To consider: Make a standardized 
        // Exception C++ class for this package which users can use and provides some basic info, including location 
        // and possibly callstack.
        mExceptionInfoPS3.mpExceptionInstructionAddress = NULL;
        mExceptionInfoPS3.mpExceptionMemoryAddress      = NULL;
        mExceptionInfoPS3.mExceptionDescription.assign(e.what());

        // There is currently no CPU context information for a C++ exception.
        memset(&mExceptionInfoPS3.mContext, 0, sizeof(mExceptionInfoPS3.mContext));

        // Save the thread id.
        mExceptionInfoPS3.mThreadId      = EA::Thread::GetThreadId(); // C++ exception handling occurs in the same thread as the exception occurred.
        mExceptionInfoPS3.mSysThreadId   = EA::Thread::GetSysThreadId(mExceptionInfoPS3.mThreadId);
        EA::Debug::GetThreadName(mExceptionInfoPS3.mThreadId, mExceptionInfoPS3.mSysThreadId, mExceptionInfoPS3.mThreadName, EAArrayCount(mExceptionInfoPS3.mThreadName));

        // Record exception time
        const time_t timeValue = time(NULL);
        mExceptionInfoPS3.mTime = *localtime(&timeValue);

        // Record the exception callstack
        // Unfortunately, this will return the callstack where we are now and not where the throw was.
        mExceptionInfoPS3.mCallstackEntryCount = EA::Thread::GetCallstack(mExceptionInfoPS3.mCallstack, EAArrayCount(mExceptionInfoPS3.mCallstack), NULL); // C++ exception handling occurs in the same thread as the exception occurred.

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
// GetExceptionCallstack
//
size_t ExceptionHandlerPS3::GetExceptionCallstack(void* pCallstackArray, size_t capacity) const
{
    if(capacity > mExceptionInfoPS3.mCallstackEntryCount)
        capacity = mExceptionInfoPS3.mCallstackEntryCount;
    
    memmove(pCallstackArray, mExceptionInfoPS3.mCallstack, capacity * sizeof(void*));

    return capacity;
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionContext
//
const EA::Callstack::Context* ExceptionHandlerPS3::GetExceptionContext() const
{
    return &mExceptionInfoPS3.mContext;
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionThreadId
//
EA::Thread::ThreadId ExceptionHandlerPS3::GetExceptionThreadId(EA::Thread::SysThreadId& sysThreadId, char* threadName, size_t threadNameCapacity) const
{
    sysThreadId = mExceptionInfoPS3.mSysThreadId;
    EA::StdC::Strlcpy(threadName, mExceptionInfoPS3.mThreadName, threadNameCapacity);

    return mExceptionInfoPS3.mThreadId;
}



///////////////////////////////////////////////////////////////////////////////
// GetExceptionDescription
//
void ExceptionHandlerPS3::GenerateExceptionDescription()
{
    const char* pDescription = NULL;

    switch(mExceptionInfoPS3.mExceptionCause)
    {
        case SYS_DBG_PPU_EXCEPTION_TRAP:
            pDescription = "SYS_DBG_PPU_EXCEPTION_TRAP";
            break;

        case SYS_DBG_PPU_EXCEPTION_PREV_INST:
            pDescription = "SYS_DBG_PPU_EXCEPTION_PREV_INST";
            break;

        case SYS_DBG_PPU_EXCEPTION_ILL_INST:
            pDescription = "SYS_DBG_PPU_EXCEPTION_ILL_INST";
            break;

        case SYS_DBG_PPU_EXCEPTION_TEXT_HTAB_MISS:
            pDescription = "SYS_DBG_PPU_EXCEPTION_TEXT_HTAB_MISS (code access violation)";
            break;

        case SYS_DBG_PPU_EXCEPTION_TEXT_SLB_MISS:
            pDescription = "SYS_DBG_PPU_EXCEPTION_TEXT_SLB_MISS (code access violation)";
            break;

        case SYS_DBG_PPU_EXCEPTION_DATA_HTAB_MISS:
            pDescription = "SYS_DBG_PPU_EXCEPTION_DATA_HTAB_MISS (data access violation)";
            break;

        case SYS_DBG_PPU_EXCEPTION_DATA_SLB_MISS:
            pDescription = "SYS_DBG_PPU_EXCEPTION_DATA_SLB_MISS (data access violation)";
            break;

        case SYS_DBG_PPU_EXCEPTION_FLOAT:
            pDescription = "SYS_DBG_PPU_EXCEPTION_FLOAT";
            break;

        case SYS_DBG_PPU_EXCEPTION_DABR_MATCH:
            pDescription = "SYS_DBG_PPU_EXCEPTION_DABR_MATCH";
            break;

        case SYS_DBG_PPU_EXCEPTION_ALIGNMENT:
            pDescription = "SYS_DBG_PPU_EXCEPTION_ALIGNMENT";
            break;

        default:
            pDescription = "SYS_DBG_PPU_EXCEPTION_???";
            break;
    }

    mExceptionInfoPS3.mExceptionDescription.sprintf("%s, Instruction: %#p, Memory: %#p", pDescription, mExceptionInfoPS3.mpExceptionInstructionAddress, mExceptionInfoPS3.mpExceptionMemoryAddress);
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionDescription
//
size_t ExceptionHandlerPS3::GetExceptionDescription(char* buffer, size_t capacity)
{
    return EA::StdC::Strlcpy(buffer, mExceptionInfoPS3.mExceptionDescription.c_str(), capacity);
}


///////////////////////////////////////////////////////////////////////////////
// SimulateExceptionHandling
//
void ExceptionHandlerPS3::SimulateExceptionHandling(int exceptionType)
{
    sys_ppu_thread_t threadIdCurrent;
    sys_ppu_thread_get_id(&threadIdCurrent);

    uint64_t exceptionCause          = 0;
    uint64_t dataAccessRegisterValue = 0;

    switch ((ExceptionType)exceptionType)
    {
        case kExceptionTypeNone:
        case kExceptionTypeOther:
        case kExceptionTypeAccessViolation:
            exceptionCause = SYS_DBG_PPU_EXCEPTION_TEXT_HTAB_MISS;
            break;
        case kExceptionTypeIllegalInstruction:
            exceptionCause = SYS_DBG_PPU_EXCEPTION_ILL_INST;
            break;
        case kExceptionTypeDivideByZero:
            exceptionCause = 0;             // PowerPC doesn't generate exceptions on div/0.
            break;
        case kExceptionTypeStackOverflow:
            exceptionCause = SYS_DBG_PPU_EXCEPTION_TEXT_HTAB_MISS;
            break;
        case kExceptionTypeStackCorruption:
            exceptionCause = SYS_DBG_PPU_EXCEPTION_DATA_HTAB_MISS;
            break;
        case kExceptionTypeAlignment:
            exceptionCause = SYS_DBG_PPU_EXCEPTION_ALIGNMENT;
            break;
        case kExceptionTypeFPU:
            exceptionCause = SYS_DBG_PPU_EXCEPTION_FLOAT;
            break;
        case kExceptionTypeTrap:
            exceptionCause = SYS_DBG_PPU_EXCEPTION_TRAP;
            break;
    }

    ExceptionCallback(exceptionCause, threadIdCurrent, dataAccessRegisterValue);
}


///////////////////////////////////////////////////////////////////////////////
// ExceptionCallbackStatic
//
void ExceptionHandlerPS3::ExceptionCallbackStatic(uint64_t exceptionCause, sys_ppu_thread_t ppuThreadId, uint64_t dataAccessRegisterValue)
{
    if(mpThis)
        mpThis->ExceptionCallback(exceptionCause, ppuThreadId, dataAccessRegisterValue);
}


///////////////////////////////////////////////////////////////////////////////
// ExceptionCallback
//
// This function will be called when any exception occured in a PPU thread.
//
// The 3rd argument specifies DAR(Data Address Register) value.
//     If the 1st argument is one of the two items below, 
//     then the 3rd argument is a valid value. Otherwise it is zero.
//        SYS_DBG_PPU_EXCEPTION_DABR_MATCH
//        SYS_DBG_PPU_EXCEPTION_ALIGNMENT
//
void ExceptionHandlerPS3::ExceptionCallback(uint64_t exceptionCause, uint64_t ppuThreadId, uint64_t dataAccessRegisterValue)
{
    if(mbHandlingInProgress.SetValueConditional(1, 0)) // Set it true (1) from false (0) atomically, evaluating as true if somebody else didn't do it first.
    {
        mbExceptionOccurred = true;

        // To consider: Replace the current thread's stack space with a new one that has more memory. 
        // The PS3 kernel gives you very little stack space in this exception handling thread, which
        // it created for you. We can swap out the stack by allocating a new block of memory, copying
        // all the memory from the old stack space to the new stack space, and then fix up our stack 
        // frames. This might not allow us to successfully return to the original caller in the kernel
        // but it will let us do all of our exception handling work.
    
        // Assert that our Context is equivalent to the Sony context. 
        // A sizeof comparison is the first step in such an assertion, but is not enough.
        EA_COMPILETIME_ASSERT(sizeof(mExceptionInfoPS3.mContext) == sizeof(sys_dbg_ppu_thread_context_t));
        union ContextU
        {
            EA::Callstack::Context       eacContext;
            sys_dbg_ppu_thread_context_t ps3Context;
        }u;

        // In the case of exception simulation, this function will be fake-called from the simulating thread.
        // So detect this case and skip some processing if so. It seems that the sys_dbg_read_ppu_thread_context
        // call below crashes when used on the current thread.
 
        sys_ppu_thread_t threadIdCurrent;
        sys_ppu_thread_get_id(&threadIdCurrent);

        if(threadIdCurrent != ppuThreadId)
        {
            // It turns out that there is an as-yet undocumented race between when the exception handler is 
            // called and when the exception causing thread is moved to the STOP state. You can get around 
            // this by sleeping in the exception handler until the thread state is moved to STOP:
            EA::StdC::LimitStopwatch limitStopwatch(EA::StdC::Stopwatch::kUnitsMilliseconds, 1000, true);

            while(!limitStopwatch.IsTimeUp())
            {
                sys_dbg_ppu_thread_status_t status;

                int result = sys_dbg_get_ppu_thread_status((sys_ppu_thread_t)ppuThreadId, &status);

                if((result != CELL_OK) || (status == PPU_THREAD_STATUS_STOP))
                    break;

                sys_timer_usleep(300000); // Wait a few msec until the target PPU thread enters STOP state.
            }
        }

        memset(&u.ps3Context, 0, sizeof(EA::Callstack::Context));

        if(threadIdCurrent != ppuThreadId)
            sys_dbg_read_ppu_thread_context(ppuThreadId, &u.ps3Context);

        if(u.eacContext.mIar == 0) // If the above failed, which would happen if we weren't actually in an exception handling state.
            GetCurrentCallstackContext(u.eacContext); // Get the current context, at least as much as we can get of it, for diagnostic purposes.

        memcpy(&mExceptionInfoPS3.mContext, &u.eacContext, sizeof(mExceptionInfoPS3.mContext));

        // Save the thread id. The registered clients may want to use this.
        mExceptionInfoPS3.mThreadId      = ppuThreadId; // C++ exception handling occurs in the same thread as the exception occurred.
        mExceptionInfoPS3.mSysThreadId   = EA::Thread::GetSysThreadId(mExceptionInfoPS3.mThreadId);
        EA::Debug::GetThreadName(mExceptionInfoPS3.mThreadId, mExceptionInfoPS3.mSysThreadId, mExceptionInfoPS3.mThreadName, EAArrayCount(mExceptionInfoPS3.mThreadName));

        // Record the exception cause for posterity. It will be one of SYS_DBG_PPU_EXCEPTION_TEXT_HTAB_MISS, etc.
        mExceptionInfoPS3.mExceptionCause = exceptionCause; 

        // Record exception time.
        const time_t timeValue = time(NULL);
        mExceptionInfoPS3.mTime = *localtime(&timeValue);

        // Record the exception callstack
        // On this platform we can retrieve the callstack by looking at the exception context (e.g. registers).
        EA::Callstack::CallstackContext callstackContext;
        EA::Callstack::GetCallstackContext(callstackContext, GetExceptionContext());
        mExceptionInfoPS3.mCallstackEntryCount = EA::Thread::GetCallstack(mExceptionInfoPS3.mCallstack, EAArrayCount(mExceptionInfoPS3.mCallstack), &callstackContext);

        // Record the exception address
        mExceptionInfoPS3.mpExceptionInstructionAddress = (void*)(uintptr_t)mExceptionInfoPS3.mContext.mIar;
        mExceptionInfoPS3.mpExceptionMemoryAddress      = (void*)(uintptr_t)dataAccessRegisterValue;

        GenerateExceptionDescription();

        if(mpOwner)
        {
            // In the case of other platforms, we disable the exception handler here.
            // That would do no good on the PS3 because exceptions are unilaterally
            // terminating events and if another exception occurred we would just be 
            // shut down by the system.

            mpOwner->NotifyClients(ExceptionHandler::kExceptionHandlingBegin);

            if(mpOwner->GetReportTypes() & ExceptionHandler::kReportTypeException)
                mpOwner->WriteExceptionReport();

            mpOwner->NotifyClients(ExceptionHandler::kExceptionHandlingEnd);
        }

        // We don't do anything more with mAction, as the kernel will kill the process after any real exceptions.
        // The following is currently disabled, as we intentionally let the kernel kill us and possibly record it for company usage.
        // if(mAction == ExceptionHandler::kActionTerminate)
        //     ::exit(mnTerminateReturnValue);

        mbHandlingInProgress = 0;
    }
}


} // namespace Debug

} // namespace EA


#endif  // #if defined(EA_PLATFORM_PS3)

















