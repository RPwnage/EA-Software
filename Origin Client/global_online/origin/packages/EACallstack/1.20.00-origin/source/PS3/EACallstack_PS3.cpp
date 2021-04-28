///////////////////////////////////////////////////////////////////////////////
// EACallstack_PS3.cpp
//
// Copyright (c) 2003-2005 Electronic Arts Inc.
// Created by Paul Pedriana
///////////////////////////////////////////////////////////////////////////////


#include <EABase/eabase.h>

#if defined(EA_PLATFORM_PS3) && !defined(EA_PLATFORM_PS3_SPU)

#include <EACallstack/EACallstack.h>
#include <EACallstack/Context.h>
#include <string.h>
#include <sys/ppu_thread.h>
#include <sys/paths.h>
#include <sys/prx.h>
#include <sys/dbg.h>
#include <sdk_version.h>
#include <cell/sysmodule.h>
#include <sysutil/sysutil_common.h>
#include <sysutil/sysutil_sysparam.h>
#include <eathread/eathread_storage.h>
#include EA_ASSERT_HEADER



namespace EA
{
namespace Callstack
{


/* To consider: Enable usage of this below.
///////////////////////////////////////////////////////////////////////////////
// IsAddressReadable
//
static bool IsAddressReadable(const void* pAddress)
{
    sys_page_attr_t pageAccess;

    bool bPageReadable = (sys_memory_get_page_attribute((sys_addr_t)pAddress, &pageAccess) == CELL_OK);

    if(bPageReadable)
        bPageReadable = (pageAccess.access_right & SYS_MEMORY_ACCESS_RIGHT_PPU_THR) != 0;

    return bPageReadable;
}
*/


///////////////////////////////////////////////////////////////////////////////
// InitCallstack
//
EACALLSTACK_API void InitCallstack()
{
    // Nothing needed.
}


///////////////////////////////////////////////////////////////////////////////
// ShutdownCallstack
//
EACALLSTACK_API void ShutdownCallstack()
{
    // Nothing needed.
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstack
//
///////////////////////////////////////////////////////////////////////////////
// PowerPC64 Linux ABI Stack Frame Organiztion
// 
// High Addresses
//           +-> Back chain
//           |   Floating point register save area
//           |   General register save area
//           |   VRSAVE save word (32-bits)
//           |   Alignment padding (4 or 12 bytes)
//           |   Vector register save area (quadword aligned)
//           |   Local variable space
//           |   Parameter save area    (SP + 48)
//           |   TOC save area          (SP + 40)
//           |   link editor doubleword (SP + 32)
//           |   compiler doubleword    (SP + 24)
//           |   LR save area           (SP + 16)
//           |   CR save area           (SP + 8)
// SP  --->  +-- Back chain             (SP + 0)
// Low Addresses
//
// The LR (link register) holds the location of the return address for any
// given function.
//
// Another approach is to use __builtin_return_address with numeric literals.
// This will work even with -fomit-frame-pointer, but you must use literal constants.
//    stack[0] = __builtin_return_address(0);
//    stack[1] = __builtin_return_address(1);
//    stack[2] = __builtin_return_address(2);
// You cannot do:
//    for(size_t i = 0; i < depth; i++)
//        stack[i] = __builtin_return_address(i);
//
///////////////////////////////////////////////////////////////////////////////
//
EACALLSTACK_API size_t GetCallstack(void* pReturnAddressArray[], size_t nReturnAddressArrayCapacity, const CallstackContext* pContext)
{
    size_t    nEntryIndex(0);
    uint64_t* pFrame;           // The PS3 with 32 bit pointers still uses 64 bit pointers in its stack frames.
    void*     pInstruction;

    if(pContext)
    {
        pFrame       = (uint64_t*)pContext->mGPR1;
        pInstruction = (void*)pContext->mIAR;
    }
    else
    {
        // General PowerPC asm for reading the instruction pointer (program counter).
        // mflr r4                  // Save link register
        // bl NextLine              // Jump to the next line. Instruction pointer will be in the link register (LR)
        // NextLine:                //
        // mflr r3                  // Copy LR to some other register.
        // mtlr r4                  // Restore the original LR.
        // std r4, pInstruction     // std for 64 bit pointers, stw for 32 bit pointers

        // We make a bogus function call to ourselves that we know will have no effect. We do this because we 
        // need to force the generation of a stack frame for this function. Otherwise the optimizer might 
        // realize this function calls no others and eliminate the stack frame.
        CallstackContext context;
        context.mGPR1 = 0;
        context.mIAR  = 0;
        GetCallstack(pReturnAddressArray, 1, &context);

        #if defined(__SNC__)
            pFrame = (uint64_t*)__builtin_frame_address();
            pFrame = (uint64_t*)(uintptr_t)*pFrame; // Move to parent frame. To consider: Use IsAddressReadable(pFrame) before dereferencing this pointer.
        #else
            // The following statement causes the compiler to generate a stack frame
            // for this function, as if this function were going to need to call 
            // another function. We could call some arbitrary function here, but the
            // statement below has the same effect under GCC.
            __asm__ __volatile__("" : "=l" (pFrame) : );

            // Load into pFrame (%0) the contents of what is pointed to by register 1 with 0 offset.
            #if (EA_PLATFORM_PTR_SIZE == 8)
                __asm__ __volatile__("ld %0, 0(1)" : "=r" (pFrame) : );
            #else
                uint64_t  temp;
                __asm__ __volatile__("ld %0, 0(1)" : "=r" (temp) : );
                pFrame = (uint64_t*)(uintptr_t)temp;
            #endif
        #endif

        pInstruction = (void*)(uintptr_t)pFrame[2];
    }

    while(pFrame && (nEntryIndex < (nReturnAddressArrayCapacity - 1)))
    {
        // The LR save area of a stack frame holds the return address, and what
        // we really want is the address of the calling instruction, which is in
        // many cases the instruction located just before the return address.
        // On PPC, this is simple to calculate due to every instruction being 
        // four bytes in length. To do: Only do the -4 adjustment below if the 
        // instruction before pInstruction is a function-calling instruction.
        pInstruction = (void*)((uintptr_t)pInstruction - 4);

        pReturnAddressArray[nEntryIndex++] = pInstruction;

        const uint64_t* const pPrevFrame = pFrame;

        pFrame = (uint64_t*)(uintptr_t)*pFrame; // To consider: Use IsAddressReadable(pFrame) before dereferencing this pointer.

        // If pFrame is 0 or is not aligned as expected (16 bytes) or is 
        // a pointer to an address that's before pPrevFrame, we quit.
        if(!pFrame || ((uintptr_t)pFrame & 15) || (pFrame < pPrevFrame))
            break;

        pInstruction = (void*)(uintptr_t)pFrame[2];
    }

    pReturnAddressArray[nEntryIndex] = 0;
    return nEntryIndex;
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstackSPU
// 
// Gets an SPU callstack from the PPU, given an spuThreadId.
// This code is based on the SPU callstack from the JuiceConsole project.
// nReturnAddressArrayCapacity must be at least two.
//
EACALLSTACK_API size_t GetCallstackSPU(void* pReturnAddressArray[], size_t nReturnAddressArrayCapacity, intptr_t spuThreadId)
{
    size_t nEntryIndex(0);

    sys_dbg_spu_thread_context_t spu_context;
    memset(&spu_context, 0, sizeof(spu_context));

    if(sys_dbg_read_spu_thread_context(spuThreadId, &spu_context) == CELL_OK)
    {
        uint32_t pBackChain = spu_context.gpr[1].word[0];

        if(pBackChain)
        {
            pReturnAddressArray[0] = (void*)(spu_context.gpr[0].word[0] - sizeof(uint32_t)); // The call address is often one instruction prior to the return address. Sometimes it is more instructions prior; uncommonly it can be instructions later.

            // This code requires that nReturnAddressArrayCapacity be at least 2.
            for(nEntryIndex = 1; pBackChain && ((nEntryIndex < nReturnAddressArrayCapacity - 1)); ++nEntryIndex)
            {
                uint64_t lr, bch;

                if((sys_spu_thread_read_ls(spuThreadId, pBackChain + 4*sizeof(uint32_t), &lr,  sizeof(uint64_t)) == CELL_OK) && 
                   (sys_spu_thread_read_ls(spuThreadId, pBackChain,                      &bch, sizeof(uint64_t)) == CELL_OK))
                {
                    pReturnAddressArray[nEntryIndex] = (void*)(uint32_t)((lr >> 32) - sizeof(uint32_t)); // The call address is often one instruction prior to the return address. Sometimes it is more instructions prior; uncommonly it can be instructions later.
                    pBackChain = (bch >> 32);
                }
                else
                    break;
            }
        }
    }

    pReturnAddressArray[nEntryIndex] = 0;
    return nEntryIndex;
}



///////////////////////////////////////////////////////////////////////////////
// GetCallstackContext
//
EACALLSTACK_API bool GetCallstackContext(CallstackContext& context, intptr_t threadId)
{
    sys_ppu_thread_t threadIdCurrent;
    sys_ppu_thread_get_id(&threadIdCurrent);

    if(threadId == (intptr_t)threadIdCurrent)
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
                __asm__ __volatile__("lwz %0, 0(1)" : "=r" (pFrame) : );
            #endif
        #endif

        // This is some crazy GCC code that happens to work:
        pInstruction = ({ __label__ label; label: &&label; });

        context.mGPR1 = (uintptr_t)pFrame;
        context.mIAR  = (uintptr_t)pInstruction;
    }
    else
    {
        // Clear the context because sys_dbg_read_ppu_thread_context sometimes returns CELL_OK but actually does nothing.
        sys_dbg_ppu_thread_context_t ppu_context;
        memset(&ppu_context, 0, sizeof(ppu_context));

        // The following works only if the if the given thread is in a stopped or sleeping state,
        // and if in a sleeping state then some registers are not read correctly. The PS3 documentation
        // states that this function is usable only when called within an exception handler and returns
        // CELL_LV2DBG_ERROR_DEOPERATIONDENIED in any other case.
        int result = sys_dbg_read_ppu_thread_context(threadId, &ppu_context);

        if(result != CELL_OK)
            return false;

        context.mGPR1 = ppu_context.gpr[1];
        context.mIAR  = ppu_context.pc;
    }

    return true;
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
    if(pContext)
    {
        context.mGPR1 = (uintptr_t)pContext->mGpr[1];
        context.mIAR  = (uintptr_t)pContext->mIar;
    }
    else
    {
        context.mGPR1 = 0;
        context.mIAR  = 0;
    }
}


///////////////////////////////////////////////////////////////////////////////
// GetModuleFromAddress
//
EACALLSTACK_API size_t GetModuleFromAddress(const void* pAddress, char* pModuleName, size_t moduleNameCapacity)
{
    EA_ASSERT(moduleNameCapacity > 0);

    sys_prx_id_t id = sys_prx_get_module_id_by_address((void*)pAddress);

    if(id >= 0)
    {
        char                  fileNameBuffer[256];
        sys_prx_module_info_t info;

        info.size          = sizeof(info);
        info.filename      = fileNameBuffer;
        info.filename_size = sizeof(fileNameBuffer);
        info.segments      = NULL;
        info.segments_num  = 0;

        int result = sys_prx_get_module_info(id, 0, &info);

        if(result == 0)
        {
            strncpy(pModuleName, info.filename, moduleNameCapacity);
            pModuleName[moduleNameCapacity - 1] = 0;

            return info.filename_size;
        }
    }

    // The address likely belongs to the main ELF. But the PS3 doesn't provide 
    // any means to tell what the main ELF file name/path is. We might be able
    // to get away with iterating all files in the home directory and search
    // for *.self.
    strncpy(pModuleName, "unknown", moduleNameCapacity);
    pModuleName[moduleNameCapacity - 1] = 0;
    return 7; // strlen("unknown")
}


///////////////////////////////////////////////////////////////////////////////
// GetModuleHandleFromAddress
//
EACALLSTACK_API ModuleHandle GetModuleHandleFromAddress(const void* pAddress)
{
    sys_prx_id_t id = sys_prx_get_module_id_by_address((void*)pAddress);

    return (id >= 0) ? id : 0;
}


// To do: Remove the usage of this global variable if it turns out that the 
// sys_ppu_thread_get_stack_information function is reliable. If it's reliable
// then we don't need sStackBase as a user-set fallback. 
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
        // Load into pFrame (%0) the contents of what is pointed to by register 1 with 0 offset.
        #if defined(__SNC__)
            pStackBase = (uintptr_t*)__builtin_frame_address();
        #else
            #if (EA_PLATFORM_PTR_SIZE == 8)
                __asm__ __volatile__("ld %0, 0(1)" : "=r" (pStackBase) : );
            #else
                uint64_t  temp;
                __asm__ __volatile__("ld %0, 0(1)" : "=r" (temp) : );
                pStackBase = (void*)(uintptr_t)temp;
            #endif
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
    sys_ppu_thread_stack_t stackInfo;

    int result = sys_ppu_thread_get_stack_information(&stackInfo);

    if(result == CELL_OK)
        return (void*)(stackInfo.pst_addr + stackInfo.pst_size); // stackInfo.pst_addr refers to the lowest address (i.e. the stack limit)

    return sStackBase.GetValue();
}


///////////////////////////////////////////////////////////////////////////////
// GetStackLimit
//
EACALLSTACK_API void* GetStackLimit()
{
    // The PS3 doesn't provide a way to get the stack information for another 
    // thread's stack. This is unfortunate in face of the fact that PS3 exception
    // handling necessarily occurs in a different thread from the one with the 
    // exception.
    sys_ppu_thread_stack_t stackInfo;

    int result = sys_ppu_thread_get_stack_information(&stackInfo);

    if(result == CELL_OK)
        return (void*)stackInfo.pst_addr; // stackInfo.pst_addr refers to the stack limit, not the stack base.

    // Fallback implementation:
    void* pStack = NULL;
    
    #if defined(__SNC__)
        pStack = __builtin_frame_address();
    #else
        pStack = __builtin_frame_address(0);
    #endif
    
    return (void*)((uintptr_t)pStack & ~4095); // Round down to nearest page.
}



} // namespace Callstack
} // namespace EA


#endif // compile guard






