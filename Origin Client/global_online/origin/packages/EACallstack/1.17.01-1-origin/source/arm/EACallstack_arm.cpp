///////////////////////////////////////////////////////////////////////////////
// EACallstack_arm.cpp
//
// Copyright (c) 2010 Electronic Arts Inc.
// Created by Paul Pedriana
///////////////////////////////////////////////////////////////////////////////


#include <EACallstack/EACallstack.h>
#include <EACallstack/Context.h>
#include <EACallstack/internal/PthreadInfo.h>
#include <EAAssert/eaassert.h>
#include <eathread/eathread_storage.h>
#include <string.h>
#include EA_ASSERT_HEADER
#include "arm_unwind.h"

#if EACALLSTACK_DEBUG_DETAIL_ENABLED
    #include <EAStdC/EASprintf.h>
#endif

#ifdef _MSC_VER
    #pragma warning(push, 0)
    #include <Windows.h>
    #include <winternl.h>
    #pragma warning(pop)
#endif

#if defined(EA_PLATFORM_UNIX)
    #include <pthread.h>
    #include <eathread/eathread.h>
#elif defined(EA_PLATFORM_PSP2)
    #include <kernel/backtrace.h>
#endif



namespace EA
{
namespace Callstack
{


#if defined(_MSC_VER)
    EACALLSTACK_API void GetInstructionPointer(void*& pInstruction)
    {
        CONTEXT context;

        // Apparently there is no need to memset the context struct.
        context.ContextFlags = CONTEXT_ALL;
        RtlCaptureContext(&context);

        // Possibly use the __emit intrinsic. http://msdn.microsoft.com/en-us/library/ms933778.aspx
        pInstruction = (void*)(uintptr_t)context.___; // To do.
    }
#elif defined(EA_PLATFORM_PSP2)
    EACALLSTACK_API void GetInstructionPointer(void*& pInstruction)
    {
        //Un-implemented
    }
#else
    EACALLSTACK_API void GetInstructionPointer(void*& pInstruction)
    {
        // __builtin_return_address returns the address with the Thumb bit set
        // if it's a return to Thumb code. We intentionally preserve this and 
        // don't try to mask it away.
        pInstruction = (void*)(uintptr_t)__builtin_return_address(0);
    }
#endif


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
///////////////////////////////////////////////////////////////////////////////

// The PSVita (a.k.a. PSP2) platform provides an sceKernelBacktrace function.
// For cases where that doesn't work well, we might want to consider using the
// manual code we have in the PSP2 directory.

#if defined(EA_PLATFORM_PSP2)

    EACALLSTACK_API size_t GetCallstack(void* pReturnAddressArray[], size_t nReturnAddressArrayCapacity, const CallstackContext*)
    {
        SceKernelCallFrame buffer[64];
        SceUInt32          frameCount = 0;

        // sceKernelBacktrace indeed is documented as accepting the buffer byte size and not the element capacity.
        sceKernelBacktrace(SCE_KERNEL_BACKTRACE_CONTEXT_CURRENT, buffer, sizeof(buffer), 
                            &frameCount, SCE_KERNEL_BACKTRACE_MODE_USER /*| SCE_KERNEL_BACKTRACE_MODE_DONT_EXCEED*/); // To consider: enable don't-exceed mode.
        
        // The first two frames appear to be within the kernel and the third 
        // is the GetCallstack function itself, so we omit them.
        const SceUInt32 frameDiscardCount = (frameCount > 3) ? 3 : frameCount;
        frameCount -= frameDiscardCount;

        const size_t resultCopyCount = ((size_t)frameCount < nReturnAddressArrayCapacity) ? (size_t)frameCount : nReturnAddressArrayCapacity;

        for(int i = 0; i < resultCopyCount; i++)
            pReturnAddressArray[i] = (void*)buffer[i + frameDiscardCount].pc;

        return resultCopyCount;
    }

#else

    EACALLSTACK_API size_t GetCallstack(void* pReturnAddressArray[], size_t nReturnAddressArrayCapacity, const CallstackContext* pContext)
    {
        #if defined(EA_PLATFORM_CTR) // CTR == Nintendo 3DS hand-held.
            // EASetStackBottom() or SetStackBase() must be called early in each thread's execution (e.g. in main function).
            // Otherwise we won't know where the callstack limit is and will probably crash. To do: find a way to guess what 
            // the callstack bottom is with enough accuracy to be useful to users. At the least we can assume that the bottom
            // is the base + a megabyte or similar.
            EA_ASSERT(GetStackBase() != NULL);
        #endif

        void* p;
        CallstackContext context;
        size_t entryCount = 0;

        if(pContext)
            context = *pContext;
        else
        {
            #if defined(__ARMCC_VERSION)
                context.mFP = 0; // We don't currently have a simple way to read fp (which is r7 (Thumb) or r11 (ARM)).
                context.mSP = (uintptr_t)__current_sp();
                context.mLR = (uintptr_t)__return_address();
                GetInstructionPointer(p); // Intentionally don't call __current_pc() or EAGetInstructionPointer, because these won't set the Thumb bit it this is Thumb code.
                context.mPC = (uintptr_t)p;

            #elif defined(__GNUC__) || defined(EA_COMPILER_CLANG) // Including Apple iOS.
                void* spAddress = &context.mSP;
                void* sp;
                asm volatile(
                    "add %0, sp, #0\n"
                    "str %0, [%1, #0]\n"
                         : "=l"(sp), "+l"(spAddress) :: "memory");

                context.mFP = (uintptr_t)__builtin_frame_address(0);
                context.mLR = (uintptr_t)__builtin_return_address(0);
                GetInstructionPointer(p); // Intentionally don't call EAGetInstructionPointer, because it won't set the Thumb bit it this is Thumb code.
                context.mPC = (uintptr_t)p;

            #elif defined(_MSC_VER)
                // Possibly use the __emit intrinsic. Do this by making a __declspec(naked) function that 
                // does nothing but return r14 (move r14 to r0). Need to know the opcode for that.
                // http://msdn.microsoft.com/en-us/library/ms933778.aspx
                #error Need to complete this somehow.
                context.mFP = 0; 
                context.mLR = 0;
                context.mSP = 0;
                GetInstructionPointer(p); // Intentionally don't call EAGetInstructionPointer, because it won't set the Thumb bit it this is Thumb code.
                context.mPC = (uintptr_t)p;
            #endif
        }

        CliStack results;
        results.frameCount = 0;

        /*const UnwResult r = */ UnwindStart(context.mFP, context.mSP, context.mLR , context.mPC, &cliCallbacks, &results);

        // We ignore the return value and see if there was any results.
        // It isn't useful to check the return value because an error might occur after we've already successfully 
        // decoded a number of useful stack frames. And the user just wants as many stack frames as possible.
        if(results.frameCount > 0)
        {
            for(entryCount = 0; (entryCount < results.frameCount) && (entryCount < nReturnAddressArrayCapacity); entryCount++)
            {
                // The low bit of a returned address indicates whether the address refers to thumb code (1) or regular arm code (0).
                pReturnAddressArray[entryCount] = (void*)(uintptr_t)results.address[entryCount];

                #if EACALLSTACK_DEBUG_DETAIL_ENABLED
                    EA::StdC::Printf("Stack #%u: 0x%08x\n", (unsigned)entryCount, pReturnAddressArray[entryCount]);
                #endif
            }
        }

        return entryCount;
    }

#endif


///////////////////////////////////////////////////////////////////////////////
// GetCallstackContext
//
EACALLSTACK_API void GetCallstackContext(CallstackContext& context, const Context* pContext)
{
    context.mSP = pContext->mGpr[13];
    context.mLR = pContext->mGpr[14];
    context.mPC = pContext->mGpr[15];
}


///////////////////////////////////////////////////////////////////////////////
// GetModuleFromAddress
//
EACALLSTACK_API size_t GetModuleFromAddress(const void* /*address*/, char* pModuleName, size_t /*moduleNameCapacity*/)
{
    pModuleName[0] = 0;
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// GetModuleHandleFromAddress
//
EACALLSTACK_API ModuleHandle GetModuleHandleFromAddress(const void* /*pAddress*/)
{
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstackContext
//
// Under Windows, the threadId parameter is expected to be a thread HANDLE, 
// which is different from a windows integer thread id.
// On Unix the threadId parameter is expected to be a pthread id.
//
EACALLSTACK_API bool GetCallstackContext(CallstackContext& context, intptr_t threadId)
{
    memset(&context, 0, sizeof(context));

    // True Linux-based ARM platforms (usually tablets and phones) can use pthread_attr_getstack.
    #if defined(EA_PLATFORM_ANDROID) || defined(EA_PLATFORM_PALM)
        if ((threadId == 0) || (threadId == (intptr_t)EA::Thread::GetThreadId()))
        {
            void* p;

            // TODO: make defines of this so that the implementation between us and GetCallstack remains the same
            #if defined(__ARMCC_VERSION)
                context.mSP = (uint32_t)__current_sp();
                context.mLR = (uint32_t)__return_address();
                context.mPC = (uint32_t)__current_pc();

            #elif defined(__GNUC__) || defined(EA_COMPILER_CLANG)
                // register uintptr_t current_sp asm ("sp");
                p = __builtin_frame_address(0);
                context.mSP = (uintptr_t)p;

                p = __builtin_return_address(0);
                context.mLR = (uint32_t)p;

                EAGetInstructionPointer(p);
                context.mPC = reinterpret_cast<uintptr_t>(p);

            #elif defined(_MSC_VER)
                context.mSP = 0;

                #error EACallstack::GetCallstack: Need a way to get the return address (register 14)
                // Possibly use the __emit intrinsic. Do this by making a __declspec(naked) function that 
                // does nothing but return r14 (move r14 to r0). Need to know the opcode for that.
                // http://msdn.microsoft.com/en-us/library/ms933778.aspx
                context.mLR = 0;

                EAGetInstructionPointer(p);
                context.mPC = reinterpret_cast<uintptr_t>(p);
            #endif

            context.mStackBase    = (uintptr_t)GetStackBase();
            context.mStackLimit   = (uintptr_t)GetStackLimit();
            context.mStackPointer = context.mSP;

            return true;
        }
        // Else haven't implemented getting the stack info for other threads

    #else
        // Not currently implemented for the given platform.
        EA_UNUSED(threadId);

    #endif

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
// as can be seen from the logic below. For example iPhone probably doesn't need it.
EA::Thread::ThreadLocalStorage sStackBase;

///////////////////////////////////////////////////////////////////////////////
// SetStackBase
//
EACALLSTACK_API void SetStackBase(void* pStackBase)
{
    if(pStackBase)
        sStackBase.SetValue(pStackBase);
    else
    {
        // Can't call GetStackLimit() because doing so would disturb the stack. 
        // As of this writing, we don't have an EAGetStackTop macro which could do this.
        // So we implement it inline here.
        #if defined(EA_PLATFORM_CTR)
            pStackBase = (void*)__current_sp();
        #elif defined(__ARMCC_VERSION)
            pStackBase = (void*)__current_sp();
        #elif defined(__GNUC__) || defined(EA_COMPILER_CLANG)
            pStackBase = __builtin_frame_address(0);
        #endif

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
    // In practice the following Unix test will cover most cases, as ARM platforms are usually
    // Unix or Apple and provide this functionality. Examples of cases that don't are the 
    // PlayBook tablet platform which uses a QNX-based OS and standard library.
    #if defined(EA_PLATFORM_UNIX)
        void* pLimit;
        if(GetPthreadStackInfo(NULL, &pLimit))
            return pLimit;
    #endif

    #if defined(EA_PLATFORM_CTR)
        void* pStack = (void*)__current_sp();
    #elif defined(__ARMCC_VERSION)
        void* pStack = (void*)__current_sp();
    #elif defined(__GNUC__) || defined(EA_COMPILER_CLANG)
        void* pStack = __builtin_frame_address(0);
    #elif defined(EA_PLATFORM_PSP2)
        void* pStack = (void*)__current_sp();
    #endif

    return (void*)((uintptr_t)pStack & ~4095); // Round down to nearest page.
}



} // namespace Callstack
} // namespace EA





