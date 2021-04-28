///////////////////////////////////////////////////////////////////////////////
// ExceptionHandlerPSP2.cpp
// 
// Copyright (c) 2005, Electronic Arts. All Rights Reserved.
// Written and maintained by Paul Pedriana.
//
// Exception handling and reporting facilities.
///////////////////////////////////////////////////////////////////////////////


#include <ExceptionHandler/PSP2/ExceptionHandlerPSP2.h>


// PSP2-specific implemenation. Make sure nobody accidentally sticks it into wrong project
#if defined(EA_PLATFORM_PSP2)

#include <EAStdC/EAString.h>
#include <sdk_version.h>
#include EA_ASSERT_HEADER



namespace EA {
namespace Debug {


///////////////////////////////////////////////////////////////////////////////
// ExceptionHandlerPSP2
//
ExceptionHandlerPSP2::ExceptionHandlerPSP2(ExceptionHandler* pOwner) :
    mpOwner(pOwner),
    mbEnabled(false),
    mbExceptionOccurred(false),
    mMode(ExceptionHandler::kModeDefault),
    mAction(ExceptionHandler::kActionDefault),
    mnTerminateReturnValue(ExceptionHandler::kDefaultTerminationReturnValue),
    mExceptionInfoPSP2()
{
}


///////////////////////////////////////////////////////////////////////////////
// ~ExceptionHandlerPSP2
//
ExceptionHandlerPSP2::~ExceptionHandlerPSP2()
{
    SetEnabled(false);
}


///////////////////////////////////////////////////////////////////////////////
// SetEnabled
//
bool ExceptionHandlerPSP2::SetEnabled(bool state)
{
    if(state && !mbEnabled)
    {
        #if (SCE_PSP2_SDK_VERSION >= 0x01800000)
            SceInt32 result = sceCoredumpRegisterCoredumpHandler(ExceptionCallbackStatic, 4096, this);

            EA_ASSERT(result == SCE_OK);
            mbEnabled = (result == SCE_OK);
        #endif
    }
    else if(!state && mbEnabled)
    {
        mbEnabled = false;

        #if (SCE_PSP2_SDK_VERSION >= 0x01800000)
            SceInt32 result = sceCoredumpUnregisterCoredumpHandler();

            EA_ASSERT(result == SCE_OK); EA_UNUSED(result);
        #endif
    }

    return (mbEnabled == state);
}


///////////////////////////////////////////////////////////////////////////////
// IsEnabled
//
bool ExceptionHandlerPSP2::IsEnabled() const
{
    return mbEnabled;
}


///////////////////////////////////////////////////////////////////////////////
// SetAction
//
void ExceptionHandlerPSP2::SetAction(ExceptionHandler::Action action, int returnValue)
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
ExceptionHandler::Action ExceptionHandlerPSP2::GetAction(int* pReturnValue) const
{
    if(pReturnValue)
        *pReturnValue = mnTerminateReturnValue;
    return mAction;
}


///////////////////////////////////////////////////////////////////////////////
// RunTrapped
//
intptr_t ExceptionHandlerPSP2::RunTrapped(ExceptionHandler::UserFunctionUnion userFunctionUnion, ExceptionHandler::UserFunctionType userFunctionType, void* pContext, bool& exceptionCaught)
{
    // The PSVita doesn't support try/catch style exception handling (aside from C++ try/catch). 
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
    }
    else
        returnValue = ExceptionHandler::ExecuteUserFunction(userFunctionUnion, userFunctionType, pContext);

    //mbExceptionOccurred = exceptionCaught;
    return returnValue;
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionCallstack
//
size_t ExceptionHandlerPSP2::GetExceptionCallstack(void* pCallstackArray, size_t capacity) const
{
    if(capacity > mExceptionInfoPSP2.mCallstackEntryCount)
        capacity = mExceptionInfoPSP2.mCallstackEntryCount;
    
    memmove(pCallstackArray, mExceptionInfoPSP2.mCallstack, capacity * sizeof(void*));

    return capacity;
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionContext
//
const EA::Callstack::Context* ExceptionHandlerPSP2::GetExceptionContext() const
{
    return &mExceptionInfoPSP2.mContext;
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionThreadId
//
EA::Thread::ThreadId ExceptionHandlerPSP2::GetExceptionThreadId(EA::Thread::SysThreadId& sysThreadId, char* threadName, size_t threadNameCapacity) const
{
    sysThreadId = mExceptionInfoPSP2.mSysThreadId;
    EA::StdC::Strlcpy(threadName, mExceptionInfoPSP2.mThreadName, threadNameCapacity);
    
    return mExceptionInfoPSP2.mThreadId;
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionDescription
//
size_t ExceptionHandlerPSP2::GetExceptionDescription(char* buffer, size_t capacity)
{
    EA::StdC::Snprintf(buffer, capacity, "Unknown exception");

    return EA::StdC::Strlen(buffer);
}


///////////////////////////////////////////////////////////////////////////////
// SimulateExceptionHandling
//
void ExceptionHandlerPSP2::SimulateExceptionHandling(int /*exceptionType*/)
{
    ExceptionCallback();
}


///////////////////////////////////////////////////////////////////////////////
// ExceptionCallbackStatic
//
int ExceptionHandlerPSP2::ExceptionCallbackStatic(void* pContext)
{
    if(pContext)
    {
        ExceptionHandlerPSP2* pThis = static_cast<ExceptionHandlerPSP2*>(pContext);
        pThis->ExceptionCallback();
    }

    return SCE_OK;
}


///////////////////////////////////////////////////////////////////////////////
// ExceptionCallback
//
void ExceptionHandlerPSP2::ExceptionCallback()
{
    memset(&mExceptionInfoPSP2.mContext, 0, sizeof(mExceptionInfoPSP2.mContext));

    // Save the thread id. The registered clients may want to use this.
    mExceptionInfoPSP2.mThreadId = EA::Thread::kThreadIdInvalid;

    // Record the exception cause for posterity.
    mExceptionInfoPSP2.mExceptionCause = 0; 

    // Record exception time.
    const time_t timeValue = time(NULL);
    mExceptionInfoPSP2.mTime = *localtime(&timeValue);

    // Record the exception callstack
    //EA::Callstack::CallstackContext callstackContext;
    //EA::Callstack::GetCallstackContext(callstackContext, GetExceptionContext());
    //mExceptionInfoPSP2.mCallstackEntryCount = EA::Thread::GetCallstack(mExceptionInfoPSP2.mCallstack, EAArrayCount(mExceptionInfoPSP2.mCallstack), &callstackContext);

    // Record the exception address
    mExceptionInfoPSP2.mpExceptionInstructionAddress = 0;
    mExceptionInfoPSP2.mpExceptionMemoryAddress      = 0;

    if(mpOwner)
    {
        mpOwner->NotifyClients(ExceptionHandler::kExceptionHandlingBegin);

        if(mpOwner->GetReportTypes() & ExceptionHandler::kReportTypeException)
        {
            // WriteExceptionReport is currently disabled because we can't write a generic 
            // report for this platform. To consider: Enable the writing of a basic embedded 
            // report by the only function Sony makes available for this:
            //     SceSSize sceCoredumpWriteUserData(const void* data, SceSize size); // Size limited to 16 K.
            // mpOwner->WriteExceptionReport();
        }

        mpOwner->NotifyClients(ExceptionHandler::kExceptionHandlingEnd);
    }
}


} // namespace Debug

} // namespace EA


#endif  // #if defined(EA_PLATFORM_PS3)

















