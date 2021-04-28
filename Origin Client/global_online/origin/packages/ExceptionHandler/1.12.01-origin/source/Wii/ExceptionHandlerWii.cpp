///////////////////////////////////////////////////////////////////////////////
// ExceptionHandlerWii.cpp
// 
// Copyright (c) 2005, Electronic Arts. All Rights Reserved.
// Written and maintained by Paul Pedriana.
//
// Exception handling and reporting facilities.
///////////////////////////////////////////////////////////////////////////////


#include <ExceptionHandler/Wii/ExceptionHandlerWii.h>


// Wii/GameCube-specific implemenation. Make sure nobody accidentally sticks it into wrong project
#if defined(EA_PLATFORM_REVOLUTION) || defined(EA_PLATFORM_GAMECUBE)

#include <EACallstack/EAAddressRep.h>
#include <EACallstack/EACallstack.h>
#include <EACallstack/EADasm.h>
#include <EACallstack/Context.h>
#include <eathread/eathread.h>
#include <EAStdC/EAString.h>
#include EA_ASSERT_HEADER

#if defined(EA_PLATFORM_REVOLUTION)
    #include <revolution/os.h>
    #include <revolution/os/OSContext.h>
    #include <revolution/base/PPCArch.h>
#elif defined(EA_PLATFORM_GAMECUBE)
    #include <dolphin/os.h>
    #include <dolphin/os/OSContext.h>
    #include <dolphin/base/PPCArch.h>
#endif


namespace EA {
namespace Debug {



// To consider: Make this not be a single global but instead some kind of 
// thread-specific stack of pointers. That way we could have more than one
// exception handler at a time.
static ExceptionHandlerWii* mpThis = NULL;


///////////////////////////////////////////////////////////////////////////////
// ExceptionHandlerWii
//
ExceptionHandlerWii::ExceptionHandlerWii(ExceptionHandler* pOwner) :
    mpOwner(pOwner),
    mSavedOSExceptionHandler(NULL),
    mbEnabled(false),
    mbExceptionOccurred(false),
    mMode(ExceptionHandler::kModeDefault),
    mAction(ExceptionHandler::kActionDefault),
    mnTerminateReturnValue(ExceptionHandler::kDefaultTerminationReturnValue),
    mExceptionInfoWii()
{
    EA_ASSERT(mpThis == NULL);
    mpThis = this;
    SetEnabled(true);
}


///////////////////////////////////////////////////////////////////////////////
// ~ExceptionHandlerWii
//
ExceptionHandlerWii::~ExceptionHandlerWii()
{
    mpThis = NULL;
    SetEnabled(false);
}


///////////////////////////////////////////////////////////////////////////////
// SetEnabled
//
bool ExceptionHandlerWii::SetEnabled(bool state)
{
    if(state && !mbEnabled)
    {
        // We unilaterally set mbEnabled as true, regardless of results of the calls below.
        mbEnabled = true;

        EA_COMPILETIME_ASSERT(__OS_EXCEPTION_SYSTEM_RESET == 0);
        for(int i = 0; i < __OS_EXCEPTION_MAX; i++)
            mSavedOSExceptionHandler = __OSSetExceptionHandler(__OSException(i), ExceptionCallbackStatic);

        // Note that there is also the following somewhat parallel function:
        //     OSErrorHandler OSSetErrorHandler(OSError error, OSErrorHandler handler);
    }
    else if(!state && mbEnabled)
    {
        mbEnabled = false;

        for(int i = 0; i < __OS_EXCEPTION_MAX; i++)
            mSavedOSExceptionHandler = __OSSetExceptionHandler(__OSException(i), mSavedOSExceptionHandler); // Note that mSavedOSExceptionHandler might be NULL. Is this OK?
    }

    return true;
}


///////////////////////////////////////////////////////////////////////////////
// IsEnabled
//
bool ExceptionHandlerWii::IsEnabled() const
{
    return mbEnabled;
}


///////////////////////////////////////////////////////////////////////////////
// SetAction
//
void ExceptionHandlerWii::SetAction(ExceptionHandler::Action action, int returnValue)
{
    // Due to the way that Wii exception handling works, kActionTerminate is the 
    // only supported action. It is impossible to continue/recover from an exception
    // on the Wii and there is no way to re-throw an exception at the system level.

    mAction = action;
    mnTerminateReturnValue = returnValue;
}


///////////////////////////////////////////////////////////////////////////////
// GetAction
//
ExceptionHandler::Action ExceptionHandlerWii::GetAction(int* pReturnValue) const
{
    if(pReturnValue)
        *pReturnValue = mnTerminateReturnValue;
    return mAction;
}


///////////////////////////////////////////////////////////////////////////////
// SimulateExceptionHandling
//
void ExceptionHandlerWii::SimulateExceptionHandling(int /*exceptionType*/)
{
    // To do.
}


///////////////////////////////////////////////////////////////////////////////
// RunTrapped
//
bool ExceptionHandlerWii::RunTrapped(UserFunction pUserFunction, void* pContext)
{
    // The Wii and WiiU don't support try/catch style exception handling (aside from C++ try/catch). 
    // We just call the user function and if an exception occurs then our callback will find
    // out about it and the user will also find out about it if the RegisterClient
    // function was called.

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
    else
        returnValue = ExceptionHandler::ExecuteUserFunction(userFunctionUnion, userFunctionType, pContext);

    //mbExceptionOccurred = exceptionCaught;
    return returnValue;
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionCallstack
//
size_t ExceptionHandlerWii::GetExceptionCallstack(void* pCallstackArray, size_t capacity) const
{
    if(capacity > mCallstackEntryCount)
        capacity = mCallstackEntryCount;
    
    memmove(pCallstackArray, mCallstack, capacity * sizeof(void*));

    return capacity;
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionContext
//
const EA::Callstack::Context* ExceptionHandlerWii::GetExceptionContext() const
{
    return &mExceptionInfoWii.mContext;
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionThreadId
//
EA::Thread::ThreadId ExceptionHandlerWii::GetExceptionThreadId(EA::Thread::SysThreadId& sysThreadId, char* threadName, size_t threadNameCapacity) const
{
    sysThreadId = EA::Thread::GetSysThreadId(mExceptionInfoWii.mThreadId);

    if(threadNameCapacity)
        threadName[0] = 0; // To do: See if we can get the real thread name.

    return mExceptionInfoWii.mThreadId;
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionDescription
//
size_t ExceptionHandlerWii::GetExceptionDescription(char* buffer, size_t capacity)
{
    const char* pDescription = NULL;

    switch(mExceptionInfoWii.mExceptionCause)
    {
        case __OS_EXCEPTION_SYSTEM_RESET:
            pDescription = "OS_ERROR_SYSTEM_RESET";
            break;

        case __OS_EXCEPTION_MACHINE_CHECK:
            pDescription = "OS_ERROR_MACHINE_CHECK";
            break;

        case __OS_EXCEPTION_DSI:
            pDescription = "OS_ERROR_DSI (data access violation)";
            break;

        case __OS_EXCEPTION_ISI:
            pDescription = "OS_ERROR_ISI (code access violation)";
            break;

        case __OS_EXCEPTION_EXTERNAL_INTERRUPT:
            pDescription = "OS_ERROR_EXTERNAL_INTERRUPT";
            break;

        case __OS_EXCEPTION_ALIGNMENT:
            pDescription = "OS_ERROR_ALIGNMENT";
            break;

        case __OS_EXCEPTION_PROGRAM:
            pDescription = "OS_ERROR_PROGRAM";
            break;

        case __OS_EXCEPTION_FLOATING_POINT:
            pDescription = "OS_ERROR_FLOATING_POINT";
            break;

        case __OS_EXCEPTION_DECREMENTER:
            pDescription = "OS_ERROR_DECREMENTER";
            break;

        case __OS_EXCEPTION_SYSTEM_CALL:
            pDescription = "OS_ERROR_SYSTEM_CALL";
            break;

        case __OS_EXCEPTION_TRACE:
            pDescription = "OS_ERROR_TRACE";
            break;

        case __OS_EXCEPTION_PERFORMACE_MONITOR:
            pDescription = "OS_ERROR_PERFORMACE_MONITOR";
            break;

        case __OS_EXCEPTION_BREAKPOINT:
            pDescription = "OS_ERROR_BREAKPOINT";
            break;

        case __OS_EXCEPTION_SYSTEM_INTERRUPT:
            pDescription = "OS_ERROR_SYSTEM_INTERRUPT";
            break;

        case __OS_EXCEPTION_THERMAL_INTERRUPT:
            pDescription = "OS_ERROR_THERMAL_INTERRUPT";
            break;

        case OS_ERROR_PROTECTION:   // There isn't an __OS_EXCEPTION version of this.
            pDescription = "OS_ERROR_PROTECTION";
            break;

        case OS_ERROR_FPE:          // There isn't an __OS_EXCEPTION version of this.
            pDescription = "OS_ERROR_FPE";
            break;

        default:
            pDescription = "OS_ERROR_???";
            break;
    }

    EA::StdC::Snprintf(buffer, capacity, "%s", pDescription);
    buffer[capacity - 1] = 0;

    return strlen(buffer);
}


///////////////////////////////////////////////////////////////////////////////
// ExceptionCallbackStatic
//
void ExceptionHandlerWii::ExceptionCallbackStatic(__OSException exceptionCause, OSContext* pContext)
{
    if(mpThis)
        mpThis->ExceptionCallback(exceptionCause, pContext);
}


///////////////////////////////////////////////////////////////////////////////
// ExceptionCallback
//
// This function will be called when any exception occured in a PPU thread.
//
// The dar argument specifies DAR(Data Address Register) value. See the 
// Nintendo documentation for OSSetErrorHandler to see an explanation of 
// this stuff.
//
void ExceptionHandlerWii::ExceptionCallback(__OSException exceptionCause, OSContext* pContext)
{
    // Assert that our Context is equivalent to the OSContext.
    // A sizeof comparison is the first step in such an assertion, but is not enough.
    EA_COMPILETIME_ASSERT(sizeof(mExceptionInfoWii.mContext) >= sizeof(OSContext));
    memcpy(&mExceptionInfoWii.mContext, pContext, sizeof(OSContext));

    // The Wii OS has a design flaw related to how it uses FPU exceptions to detect that 
    // threads need to save FPU registers on context switches. The first time every  
    // thread executes FPU functions an FPU exception is triggered which the OS catches.
    // The problem is that this happens for an exception handling thread as well, which
    // results in an exception within an exception and thus a crash. The OS flaw is that
    // it should be disabling this FPU exception thing for exception handling threads.
    // However, there is a workaround which is to call OSDumpContext; it causes the 
    // system to disable that exception handling.
    OSDumpContext(pContext);

    // The context supplied in pContext doesn't include mIAR (instruction pointer) information.
    void* pInstruction;
    EA::Callstack::GetInstructionPointer(pInstruction);
    mExceptionInfoWii.mContext.mIar  = (uintptr_t)pInstruction;

    // Save the thread id. The registered clients may want to use this.
    mExceptionInfoWii.mThreadId = OSGetCurrentThread();

    // Record the exception cause for posterity. It will be one of OS_ERROR_PROTECTION (15), etc.
    mExceptionInfoWii.mExceptionCause = exceptionCause; 

    // Record exception time.
    const time_t timeValue = time(NULL);
    mExceptionInfoWii.mTime = *localtime(&timeValue);

    // Record the exception address
    mExceptionInfoWii.mpExceptionInstructionAddress = (void*)(uintptr_t)mExceptionInfoWii.mContext.mIar;

    if(mpOwner)
    {
        // In the case of other platforms, we disable the exception handler here.
        // That would do no good on the Wii because exceptions are unilaterally
        // terminating events and if another exception occurred we would just be 
        // shut down by the system.

        mpOwner->NotifyClients(ExceptionHandler::kExceptionHandlingBegin);

        if(mpOwner->GetReportTypes() & ExceptionHandler::kReportTypeException)
            mpOwner->WriteExceptionReport();

        mpOwner->NotifyClients(ExceptionHandler::kExceptionHandlingEnd);
    }
}


} // namespace Debug

} // namespace EA


#endif  // #if defined(EA_PLATFORM_REVOLUTION)

















