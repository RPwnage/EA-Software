
///////////////////////////////////////////////////////////////////////////////
// EACallstack_Apple.cpp
//
// Copyright (c) 2010 Electronic Arts Inc.
// Created by Paul Pedriana
///////////////////////////////////////////////////////////////////////////////


#include "EAAssert/eaassert.h"
#include <EACallstack/Context.h>
#include <EACallstack/internal/PthreadInfo.h>
#include <EACallstack/Apple/EACallstackApple.h>
#include <EAThread/eathread.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EAScanf.h>
#include <EAIO/PathString.h>
#include <mach/thread_act.h>
#include <stdio.h>
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
    pInstruction = __builtin_return_address(0); // Works for all Apple platforms and compilers (gcc and clang).
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
// ARM
//      Apple defines a different ABI than the ARM eabi used by Linux and the ABI used
//      by Microsoft. It implements a predictable stack frame system using r7 as the 
//      frame pointer. Documentation:
//          http://developer.apple.com/library/ios/#documentation/Xcode/Conceptual/iPhoneOSABIReference/Articles/ARMv6FunctionCallingConventions.html
//
//      Basically, Apple uses r7 as a frame pointer. So for any function you are
//      executing, r7 + 4 is the LR passed to us by the caller and is the PC of 
//      the parent. And r7 + 0 is a pointer to the parent's r7. 
// x86/x64
//      The ABI is similar except using the different registers from the different CPU.
//
EACALLSTACK_API size_t GetCallstack(void* pReturnAddressArray[], size_t nReturnAddressArrayCapacity, const CallstackContext* pContext)
{
    #if defined(EA_DEBUG)
        memset(pReturnAddressArray, 0, nReturnAddressArrayCapacity * sizeof(void*));
    #endif
    
    #if defined(EA_PROCESSOR_ARM) || defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_X86_64) || defined(EA_PROCESSOR_POWERPC)
        
        struct StackFrame {
            StackFrame* mpParentStackFrame;
            #if defined(EA_PROCESSOR_POWERPC)
            void*       mpUnused;
            #endif
            void*       mpReturnPC;
        };
        
        StackFrame* pStackFrame;
        void*       pInstruction;
        size_t      index = 0;

        if(pContext)
        {
            #if defined(EA_PROCESSOR_ARM)
                pStackFrame  = (StackFrame*)pContext->mFP;
                pInstruction = (void*)      pContext->mPC;
                #define FrameIsAligned(pStackFrame) ((((uintptr_t)pStackFrame) & 0x1) == 0)
                                
            #elif defined(EA_PROCESSOR_X86_64)
                pStackFrame  = (StackFrame*)pContext->mRBP;
                pInstruction = (void*)      pContext->mRIP;
                #define FrameIsAligned(pStackFrame) ((((uintptr_t)pStackFrame) & 0xf) == 0)

            #elif defined(EA_PROCESSOR_X86)
                pStackFrame  = (StackFrame*)pContext->mEBP;
                pInstruction = (void*)      pContext->mEIP;
                #define FrameIsAligned(pStackFrame) ((((uintptr_t)pStackFrame) & 0xf) == 8)
                
            #elif defined(EA_PROCESSOR_POWERPC)
                pStackFrame  = (StackFrame*)pContext->mGPR1;
                pInstruction = (void*)      pContext->mIAR;
                #define FrameIsAligned(pStackFrame) ((((uintptr_t)pStackFrame) & 0xf) == 0)

            #endif

            // Write the instruction to pReturnAddressArray. In this case we have this thread 
            // reading the callstack from another thread.
            if(index < nReturnAddressArrayCapacity)
                pReturnAddressArray[index++] = pInstruction;
        }
        else // Else get the current values...
        {
            pStackFrame = (StackFrame*)__builtin_frame_address(0);
            GetInstructionPointer(pInstruction); // Intentionally don't call EAGetInstructionPointer, because it won't set the Thumb bit if this is Thumb code.

            // Don't write pInstruction to pReturnAddressArray, as pInstruction refers to the code in *this* function, whereas we want to start with caller's call frame.
        }

        // We can do some range validation if we have a pthread id.
        StackFrame* pStackBase;
        StackFrame* pStackLimit;
        const bool  bThreadIsCurrent = (pContext == NULL); // To do: allow this to also tell if the thread is current for the case that pContext is non-NULL. We can do that by reading the current frame address and walking it backwards a few times and seeing if any value matches pStackFrame. 
        
        if(bThreadIsCurrent)
        {
            pthread_t pthread = pthread_self(); // This makes the assumption that the current thread is a pthread and not just a kernel thread.
            pStackBase  = reinterpret_cast<StackFrame*>(pthread_get_stackaddr_np(pthread));
            pStackLimit = pStackBase - (pthread_get_stacksize_np(pthread) / sizeof(StackFrame));
        }
        else
        {   // Make a conservative guess.
            pStackBase  = pStackFrame + ((1024 * 1024) / sizeof(StackFrame));
            pStackLimit = pStackFrame - ((1024 * 1024) / sizeof(StackFrame));
        }

        // To consider: Do some validation of the PC. We can validate it by making sure it's with 20 MB 
        // of our PC and also verify that the instruction before it (be it Thumb or ARM) is a BL or BLX 
        // function call instruction (in the case of EA_PROCESSOR_ARM).
        // To consider: Verify that each successive pStackFrame is at a higher address than the last,
        // as otherwise the data must be corrupt.

        if((index < nReturnAddressArrayCapacity) && pStackFrame && FrameIsAligned(pStackFrame))
        {
            pReturnAddressArray[index++] = pStackFrame->mpReturnPC;  // Should happen to be equal to pContext->mLR.

            while(pStackFrame && pStackFrame->mpReturnPC && (index < nReturnAddressArrayCapacity)) 
            {
                pStackFrame = pStackFrame->mpParentStackFrame;

                if(pStackFrame && FrameIsAligned(pStackFrame) && pStackFrame->mpReturnPC && (pStackFrame < pStackBase) && (pStackFrame > pStackLimit))
                    pReturnAddressArray[index++] = pStackFrame->mpReturnPC;
                else
                    break;
            }
        }

        return index;

    
    #elif EACALLSTACK_GLIBC_BACKTRACE_AVAILABLE // Mac OS X with GlibC

        // One way to get the callstack of another thread, via signal handling:
        //     https://github.com/albertz/openlierox/blob/0.59/src/common/Debug_GetCallstack.cpp
        
        size_t count = 0;

        // The pContext option is not currently supported.
        if(pContext == NULL) // To do: || pContext refers to this thread.
        {
            count = (size_t)backtrace(pReturnAddressArray, (int)nReturnAddressArrayCapacity);
            if(count > 0)
            {
                --count; // Remove the first entry, because it refers to this function and by design we don't include this function.
                memmove(pReturnAddressArray, pReturnAddressArray + 1, count * sizeof(void*));
            }
        }
        // else fall through to code that manually reads stack frames?
        
        return count;

    #else
        EA_UNUSED(pReturnAddressArray);
        EA_UNUSED(nReturnAddressArrayCapacity);
        EA_UNUSED(pContext);

        return 0;
    #endif
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstackContext
//
// Convert a full Context to a CallstackContext (subset of context).
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
        context.mFP  = pContext->mGpr[7];   // Apple uses R7 for the frame pointer in both ARM and Thumb CPU modes.
        context.mSP  = pContext->mGpr[13];
        context.mLR  = pContext->mGpr[14];
        context.mPC  = pContext->mGpr[15];
        
    #elif defined(EA_PROCESSOR_POWERPC)
        context.mGPR1 = (uintptr_t)pContext->mGpr[1];
        context.mIAR  = (uintptr_t)pContext->mIar;
    #endif
}



///////////////////////////////////////////////////////////////////////////////
// GetModuleFromAddress
//
// Returns the required strlen of pModuleName.
//
EACALLSTACK_API size_t GetModuleFromAddress(const void* pCodeAddress, char* pModuleName, size_t moduleNameCapacity)
{
    if(moduleNameCapacity > 0)
        pModuleName[0] = 0;

    Dl_info dlInfo; memset(&dlInfo, 0, sizeof(dlInfo)); // Just memset because dladdr sometimes leaves dli_fname untouched.
    int     result = dladdr(pCodeAddress, &dlInfo);

    if((result == 0) && dlInfo.dli_fname) // It seems that usually this fails.
        return EA::StdC::Strlcpy(pModuleName, dlInfo.dli_fname, moduleNameCapacity);

    // To do: Use GetModuleInfoApple to get the information
    ModuleInfoAppleArray moduleInfoAppleArray;
    size_t               count = GetModuleInfoApple(moduleInfoAppleArray, "__TEXT", true); // To consider: Make this true (use cache) configurable.
    uint64_t             codeAddress = (uint64_t)(uintptr_t)pCodeAddress;
    
    for(size_t i = 0; i < count; i++)
    {
        const EA::Callstack::ModuleInfoApple& info = moduleInfoAppleArray[(eastl_size_t)i];
        
        if((info.mBaseAddress < codeAddress) && (codeAddress < (info.mBaseAddress + info.mSize)))
            return EA::StdC::Strlcpy(pModuleName, info.mPath.c_str(), moduleNameCapacity);
    }
        
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// GetModuleHandleFromAddress
//
EACALLSTACK_API ModuleHandle GetModuleHandleFromAddress(const void* pCodeAddress)
{
    Dl_info dlInfo; memset(&dlInfo, 0, sizeof(dlInfo)); // Just memset because dladdr sometimes leaves fields untouched.
    int     result = dladdr(pCodeAddress, &dlInfo);

    if(result == 0)
        return dlInfo.dli_fbase; // Is the object load base the same as the module handle? 

    // Try using GetModuleInfoApple to get the information.
    ModuleInfoAppleArray moduleInfoAppleArray;
    size_t               count = GetModuleInfoApple(moduleInfoAppleArray, "__TEXT", true); // To consider: Make this true (use cache) configurable.
    uint64_t             codeAddress = (uint64_t)(uintptr_t)pCodeAddress;
    
    for(size_t i = 0; i < count; i++)
    {
        const EA::Callstack::ModuleInfoApple& info = moduleInfoAppleArray[(eastl_size_t)i];
        
        if((info.mBaseAddress < codeAddress) && (codeAddress < (info.mBaseAddress + info.mSize)))
            return (ModuleHandle)info.mBaseAddress;
    }

    return 0;
}



///////////////////////////////////////////////////////////////////////////////
// GetCallstackContext
//
EACALLSTACK_API bool GetCallstackContext(CallstackContext& context, intptr_t threadId)
{
    // For Apple pthread_t is typedef'd to an internally defined _opaque_pthread_t*.
    
    bool threadIsSelf = (threadId == (intptr_t)EA::Thread::kThreadIdInvalid) || // Due to a specification mistake, this function 
                        (threadId == (intptr_t)EA::Thread::kThreadIdCurrent) || // accepts kThreadInvalid to mean current.
                        (threadId == (intptr_t)pthread_self());
    
    if(threadIsSelf)
    {
        context.mStackBase  = (uintptr_t)GetStackBase();
        context.mStackLimit = (uintptr_t)GetStackLimit();

        #if defined(EA_PROCESSOR_ARM)
            void* p;
            EAGetInstructionPointer(p);
            context.mPC = (uint32_t)p;
            context.mFP = (uint32_t)__builtin_frame_address(0);  // This data isn't exactly right. We want to return the registers as they 
            context.mSP = (uint32_t)__builtin_frame_address(0);  // are for the caller, not for us. Without doing that we end up reporting 
            context.mLR = (uint32_t)__builtin_return_address(0); // an extra frame (this one) on the top of callstacks.

        #elif defined(EA_PROCESSOR_X86_64)
            context.mRIP = (uint64_t)__builtin_return_address(0);
            context.mRSP = 0;
            context.mRBP = (uint64_t)__builtin_frame_address(1);

        #elif defined(EA_PROCESSOR_X86)
            context.mEIP = (uint32_t)__builtin_return_address(0);
            context.mESP = 0;
            context.mEBP = (uint32_t)__builtin_frame_address(1);
        #endif

        return true;
    }
    else
    {
        // Pause the thread, get its state, unpause it. 
        //
        // Question: Is it truly necessary to suspend a thread in Apple platforms in order to read
        // their state? It is usually so for other platforms doing the same kind of thing.
        //
        // Question: Is it dangerous to suspend an arbitrary thread? Often such a thing is dangerous
        // because that other thread might for example have some kernel mutex locked that we need.
        // We'll have to see, as it's a great benefit for us to be able to read callstack contexts.
        // Another solution would be to inject a signal handler into the thread and signal it and 
        // have the handler read context information, if that can be useful. There's example code
        // on the Internet for that.
        // Some documentation:
        //     http://www.linuxselfhelp.com/gnu/machinfo/html_chapter/mach_7.html
        
        mach_port_t   thread = pthread_mach_thread_np((pthread_t)threadId); // Convert pthread_t to kernel thread id.
        kern_return_t result = thread_suspend(thread);
        
        if(result == KERN_SUCCESS)
        {
            #if defined(EA_PROCESSOR_ARM)                            
                arm_thread_state_t threadState; memset(&threadState, 0, sizeof(threadState));
                mach_msg_type_number_t stateCount = MACHINE_THREAD_STATE_COUNT;
                result = thread_get_state(thread, MACHINE_THREAD_STATE, (natural_t*)(uintptr_t)&threadState, &stateCount);

                context.mFP = threadState.__r[7]; // Apple uses R7 for the frame pointer in both ARM and Thumb CPU modes.
                context.mPC = threadState.__pc;
                context.mSP = threadState.__sp;
                context.mLR = threadState.__lr;        

            #elif defined(EA_PROCESSOR_X86_64)
                // Note: This is yielding gibberish data for me, despite everything seemingly being done correctly.
                            
                x86_thread_state_t     threadState; memset(&threadState, 0, sizeof(threadState));
                mach_msg_type_number_t stateCount  = MACHINE_THREAD_STATE_COUNT;
                result = thread_get_state(thread, MACHINE_THREAD_STATE, (natural_t*)(uintptr_t)&threadState, &stateCount);

                context.mRIP = threadState.uts.ts64.__rip;
                context.mRSP = threadState.uts.ts64.__rsp;
                context.mRBP = threadState.uts.ts64.__rbp;

            #elif defined(EA_PROCESSOR_X86)
                // Note: This is yielding gibberish data for me, despite everything seemingly being done correctly.
                            
                x86_thread_state_t     threadState; memset(&threadState, 0, sizeof(threadState));
                mach_msg_type_number_t stateCount  = MACHINE_THREAD_STATE_COUNT;
                result = thread_get_state(thread, MACHINE_THREAD_STATE, (natural_t*)(uintptr_t)&threadState, &stateCount);

                context.mEIP = threadState.uts.ts32.__eip;
                context.mESP = threadState.uts.ts32.__esp;
                context.mEBP = threadState.uts.ts32.__ebp;
            #endif
            
            thread_resume(thread);
            
            return (result == KERN_SUCCESS);
        }
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
    pthread_t pthread = pthread_from_mach_thread_np((mach_port_t)sysThreadId);
    
    return GetCallstackContext(context, (intptr_t)pthread);
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



