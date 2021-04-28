///////////////////////////////////////////////////////////////////////////////
// EACallstack_Cafe.cpp
//
// Copyright (c) 2003-2011 Electronic Arts Inc.
///////////////////////////////////////////////////////////////////////////////

#include <EABase/eabase.h>

#if defined(EA_PLATFORM_CAFE)

#include <EACallstack/EACallstack.h>
#include <EACallstack/Context.h>
#include <eathread/eathread_storage.h>
#include <stdio.h>
#include EA_ASSERT_HEADER

#if defined(EA_PLATFORM_CAFE)
    #include <cafe/os/OSContext.h>
    #include <cafe/os/OSThread.h>
    #include <cafe/os/OSDynLoad.h>
    #include <ppc/ppc_ghs.h>
#endif


namespace EA
{
namespace Callstack
{

///////////////////////////////////////////////////////////////////////////////
// GetInstructionPointer
//
// The returned instruction pointer is the instruction pointer of the caller
// upon returning from this function and is not the instruction pointer of 
// code within this GetInstructionPointer. What this function does is what
// the user almost certainly wants.
//
EACALLSTACK_API void GetInstructionPointer(void*& pInstruction)
{
    // We need to call a dummy function here. The reason is that if we don't
    // then in an optimized build no stack frame will be created for this
    // function, as it can work on variables relative to the caller's frame.
    char buffer[1] = {0};
    char format[2] = {0,0};
    sprintf(buffer, format, pInstruction); // Equivalent to sprintf(buffer, "", pInstruction);

    // The following is a simpler way to solve this than the asm below.
    pInstruction = __builtin_return_address(0);

    /*
    union RegisterAddress {
        uint32_t  mGPR1;
        uint32_t* mpParentFrame;
    } ra = { 0 };

    // Read general purpose register 1 (a.k.a. GPR1).
    // The following assumes that the ra variable is located in a register, which in practice it seems to be.
    __asm__ __volatile__ ("mr %0, 1" : "=r" (ra.mGPR1) );
    // Alternative that might be better, assuming it works as intended: ra.mGPR1 = __GETREG(1); // Call compiler intrinsic from <ppc/ppc_ghs.h>
    pInstruction = (void*)ra.mpParentFrame[1];
    */
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstack
//
EACALLSTACK_API size_t GetCallstack(void* pReturnAddressArray[], size_t nReturnAddressArrayCapacity, const CallstackContext* pContext)
{
    size_t     nEntryIndex(0);
    uintptr_t* pFrame;
    void*      pInstruction;

    if(pContext)
    {
        pFrame = (uintptr_t*)pContext->mGPR1;
        pInstruction = (void*)pContext->mIAR;
    }
    else
    {
        pFrame = (uintptr_t*)__GETREG(1);
        pInstruction = NULL;
    }

    for(int nStackIndex = 0; nEntryIndex < (nReturnAddressArrayCapacity - 1); ++nStackIndex)
    {
        if(pContext || (nStackIndex > 0)) 
            pReturnAddressArray[nEntryIndex++] = (void*)((uintptr_t)pInstruction - 4);

        if(pFrame == NULL)
            break;

        pFrame = (uintptr_t*)*pFrame;

        if(!pFrame || ((uintptr_t)pFrame == 0xffffffff))
            break;

        pInstruction = (void*)pFrame[1];
    }

    pReturnAddressArray[nEntryIndex] = 0;
    return nEntryIndex;
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstackContext
//
EACALLSTACK_API bool GetCallstackContext(CallstackContext& context, intptr_t threadId)
{
    // This function can currently get only the current context.
    if(threadId == (intptr_t)OSGetCurrentThread())
    {
        const OSContext* const pContext = OSGetCurrentContext();
        void* pInstruction;
        GetInstructionPointer(pInstruction);

        context.mGPR1 = (uintptr_t)pContext->gpr[1];
        context.mIAR  = (uintptr_t)pInstruction;

        return true;
    }

    return false;
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstackContextSysThreadId
//
EACALLSTACK_API bool GetCallstackContextSysThreadId(CallstackContext& context, intptr_t threadId)
{
    return GetCallstackContext(context, threadId);
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstackContext
//
EACALLSTACK_API void GetCallstackContext(CallstackContext& context, const Context* pContext)
{
    context.mGPR1 = (uintptr_t)pContext->mGpr[1];
    context.mIAR  = (uintptr_t)pContext->mIar;
}


///////////////////////////////////////////////////////////////////////////////
// GetModuleFromAddress
//
EACALLSTACK_API size_t GetModuleFromAddress(const void* /*pAddress*/, char* pModuleFileName, size_t moduleNameCapacity)
{
    // The Cafe API as of this writing (v1.5, May 2011) doesn't provide a means to enumerate modules.
    // There is the OSDynLoad_AddNofifyCallback function, which you could use on startup to receive 
    // notifications of loads and unloads and you can keep track of the current module set.
    EA_ASSERT(moduleNameCapacity > 0); (void)moduleNameCapacity;
    pModuleFileName[0] = 0;
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// GetModuleHandleFromAddress
//
EACALLSTACK_API ModuleHandle GetModuleHandleFromAddress(const void* /*pAddress*/)
{
    // The Cafe API as of this writing (v1.5, May 2011) doesn't provide a means to enumerate modules.
    // There is the OSDynLoad_AddNofifyCallback function, which you could use on startup to receive 
    // notifications of loads and unloads and you can keep track of the current module set.
    return 0;
}



///////////////////////////////////////////////////////////////////////////////
// SetStackBase
//
EACALLSTACK_API void SetStackBase(void* /*pStackBase*/)
{
    // Nothing to do, as GetStackBase always works on its own.
}


///////////////////////////////////////////////////////////////////////////////
// GetStackBase
//
EACALLSTACK_API void* GetStackBase()
{
    OSThread* const pOSThread = OSGetCurrentThread();
    return pOSThread->stackBase;
}


///////////////////////////////////////////////////////////////////////////////
// GetStackLimit
//
EACALLSTACK_API void* GetStackLimit()
{
    OSThread* const pOSThread = OSGetCurrentThread();
    return pOSThread->stackEnd;

    // Alternative:
    // We want to return the address of buffer, but don't trigger any compiler warnings
    // that we are returning the address of memory that will be invalid upon return.
    // char  buffer[16];
    // void* pStack = buffer;
    // sprintf(buffer, "%p", &pStack);
    // return pStack;
}


} // namespace Callstack
} // namespace EA

#endif
