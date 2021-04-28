
///////////////////////////////////////////////////////////////////////////////
// EACallstack_glibc.cpp
//
// Copyright (c) 2010 Electronic Arts Inc.
// Created by Paul Pedriana
///////////////////////////////////////////////////////////////////////////////


#include "EAAssert/eaassert.h"
#include <EACallstack/Context.h>
#include <EACallstack/internal/PthreadInfo.h>
#include <EAStdC/EAString.h>
#include <string.h>
#include <pthread.h>
#include <dlfcn.h>
#include EA_ASSERT_HEADER

#include <eathread/eathread_storage.h>

#if EACALLSTACK_GLIBC_BACKTRACE_AVAILABLE
    #include <signal.h>
    #include <execinfo.h>
#endif


namespace EA
{
namespace Callstack
{


///////////////////////////////////////////////////////////////////////////////
// GetInstructionPointer
//
EACALLSTACK_API void GetInstructionPointer(void*& pInstruction)
{
    pInstruction = __builtin_return_address(0);
}


///////////////////////////////////////////////////////////////////////////////
// InitCallstack
//
EACALLSTACK_API void InitCallstack()
{
}


///////////////////////////////////////////////////////////////////////////////
// ShutdownCallstack
//
EACALLSTACK_API void ShutdownCallstack()
{
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstack
//
// Capture up to nReturnAddressArrayCapacity elements of the call stack, 
// or the whole callstack, whichever is smaller. 
//
EACALLSTACK_API size_t GetCallstack(void* pReturnAddressArray[], size_t nReturnAddressArrayCapacity, const CallstackContext* pContext)
{
    EA_UNUSED(pContext);

    // One way to get the callstack of another thread, via signal handling:
    //     https://github.com/albertz/openlierox/blob/0.59/src/common/Debug_GetCallstack.cpp
    // For Apple platforms we have a separate source file which has more extensive support than this.
    
    #if EACALLSTACK_GLIBC_BACKTRACE_AVAILABLE
        size_t count = 0;

        // The pContext option is not currently supported.
        if(pContext == NULL)
        {
            count = (size_t)backtrace(pReturnAddressArray, (int)nReturnAddressArrayCapacity);
            if(count > 0)
            {
                --count; // Remove the first entry, because it refers to this function and by design we don't include this function.
                memmove(pReturnAddressArray, pReturnAddressArray + 1, count * sizeof(void*));
            }
        }
        
        return count;

    #else
        EA_UNUSED(pReturnAddressArray);
        EA_UNUSED(nReturnAddressArrayCapacity);

        return 0;
    #endif
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstackContext
//
EACALLSTACK_API void GetCallstackContext(CallstackContext& context, const Context* pContext)
{
    #if defined(EA_PROCESSOR_X86_64)
        context.mRIP = pContext->Rip;
        context.mRSP = pContext->Rsp;
        context.mRBP = pContext->Rbp;
    #elif defined(EA_PROCESSOR_X86)
        context.mEIP = pContext->Eip;
        context.mESP = pContext->Esp;
        context.mEBP = pContext->Ebp;
    #elif defined(EA_PROCESSOR_ARM)
        context.mSP  = pContext->mGpr[13];
        context.mLR  = pContext->mGpr[14];
        context.mPC  = pContext->mGpr[15];
    #elif defined(EA_PROCESSOR_POWERPC)
        context.mGPR1 = (uintptr_t)pContext->mGpr[1];
        context.mIAR  = (uintptr_t)pContext->mIar;
    #else
        // To do.
    #endif
}


///////////////////////////////////////////////////////////////////////////////
// GetModuleFromAddress
//
// Returns the required strlen of pModuleName.
//
EACALLSTACK_API size_t GetModuleFromAddress(const void* pAddress, char* pModuleName, size_t moduleNameCapacity)
{
    // May need to #define _GNU_SOURCE for the build for this to be usable.
    Dl_info dlInfo; memset(&dlInfo, 0, sizeof(dlInfo)); // Just memset because dladdr sometimes leaves dli_fname untouched.
    int     result = dladdr((void*)pAddress, &dlInfo);

    if((result == 0) && dlInfo.dli_fname) // It seems that usually this fails
        return EA::StdC::Strlcpy(pModuleName, dlInfo.dli_fname, moduleNameCapacity);

    // To do: take an approach like we do with GetModuleInfoApple.
    
    if(moduleNameCapacity > 0)
        pModuleName[0] = 0;
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// GetModuleHandleFromAddress
//
EACALLSTACK_API ModuleHandle GetModuleHandleFromAddress(const void* pAddress)
{
    // May need to #define _GNU_SOURCE for the build for this to be usable.
    Dl_info dlInfo; memset(&dlInfo, 0, sizeof(dlInfo));
    int     result = dladdr((void*)pAddress, &dlInfo);

    if(result == 0)
        return (ModuleHandle)dlInfo.dli_fbase; // Is the object load base the same as the module handle? 

    // To do: take an approach like we do with GetModuleInfoApple.

    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstackContext
//
// Under Windows, the threadId parameter is expected to be a thread HANDLE, 
// which is different from a windows integer thread id.
// On Unix the threadId parameter is expected to be a pthread id.
//
EACALLSTACK_API bool GetCallstackContext(CallstackContext& context, intptr_t /*threadId*/)
{
    //if(threadId == pthread_self()) // Note that at least for MacOS, it's possible to get other threads' info.
    {
        #if defined(EA_PROCESSOR_X86)
            context.mEIP = (uint32_t)__builtin_return_address(0);
            context.mESP = (uint32_t)__builtin_frame_address(1);
            context.mEBP = 0;
        #else
            // To do.
        #endif
        
        return true;
    }
    
    // Not currently implemented for the given platform.
    memset(&context, 0, sizeof(context));
    return false;
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstackContextSysThreadId
//
EACALLSTACK_API bool GetCallstackContextSysThreadId(CallstackContext& context, intptr_t sysThreadId)
{
    return GetCallstackContext(context, sysThreadId);
}


// To do: Remove the usage of sStackBase for the platforms that it's not needed,
// as can be seen from the logic below. For example Mac OSX probably doesn't need it.
static EA::Thread::ThreadLocalStorage sStackBase;

///////////////////////////////////////////////////////////////////////////////
// SetStackBase
//
EACALLSTACK_API void SetStackBase(void* pStackBase)
{
    if(pStackBase)
        sStackBase.SetValue(pStackBase);
    else
    {
        pStackBase = __builtin_frame_address(0);

        if(pStackBase)
            SetStackBase(pStackBase);
        // Else failure; do nothing.
    }
}


///////////////////////////////////////////////////////////////////////////////
// GetStackBase
//
EACALLSTACK_API void* GetStackBase()
{
    #if defined(EA_PLATFORM_UNIX)
        void* pBase;
        if(GetPthreadStackInfo(&pBase, NULL))
            return pBase;
    #endif

    // Else we require the user to have set this previously, usually via a call 
    // to SetStackBase() in the start function of this currently executing 
    // thread (or main for the main thread).
    return sStackBase.GetValue();
}


///////////////////////////////////////////////////////////////////////////////
// GetStackLimit
//
EACALLSTACK_API void* GetStackLimit()
{
    #if defined(EA_PLATFORM_UNIX)
        void* pLimit;
        if(GetPthreadStackInfo(NULL, &pLimit))
            return pLimit;
    #endif

    // If this fails then we might have an issue where you are using GCC but not 
    // using the GCC standard library glibc. Or maybe glibc doesn't support 
    // __builtin_frame_address on this platform. Or maybe you aren't using GCC but
    // rather a compiler that masquerades as GCC (common situation).
    void* pStack = __builtin_frame_address(0);
    return (void*)((uintptr_t)pStack & ~4095); // Round down to nearest page.

}




} // namespace Callstack
} // namespace EA



