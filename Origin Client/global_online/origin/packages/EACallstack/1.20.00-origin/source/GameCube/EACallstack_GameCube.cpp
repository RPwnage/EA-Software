///////////////////////////////////////////////////////////////////////////////
// EACallstack_GameCube.cpp
//
// Copyright (c) 2003-2009 Electronic Arts Inc.
// Created by Paul Pedriana
///////////////////////////////////////////////////////////////////////////////


#include <EACallstack/EACallstack.h>
#include <EACallstack/Context.h>
#include <EAStdC/EAString.h>
#include <stdio.h>
#include EA_ASSERT_HEADER

#if defined(EA_PLATFORM_REVOLUTION)
    #include <revolution/os.h>
    #include <revolution/os/OSContext.h>
    #include <revolution/rso.h>
#elif defined(EA_PLATFORM_GAMECUBE)
    #include <dolphin/os.h>
    #include <dolphin/os/OSContext.h>
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
    uint32_t gpr1;
    asm { stw r1, gpr1 }

    uint32_t* const pParentFrame = (uint32_t*)*((uint32_t*)gpr1);
    pInstruction = (void*)pParentFrame[1];

    // We need to call a dummy function here. The reason is that if we don't
    // then in an optimized build no stack frame will be created for this
    // function, as it can work on variables relative to the caller's frame.
    gpr1 = 0;
    sprintf((char*)&gpr1, (const char*)&gpr1);
}


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
        #if defined(EA_ASM_STYLE_ATT) || defined(EA_COMPILER_GNUC) || defined(EA_COMPILER_CLANG)
            // Load into pStack (%0) the contents of what is pointed to by register 1 with 0 offset.
            #if (EA_PLATFORM_PTR_SIZE == 8)
                __asm__ __volatile__("ld %0, 0(1)" : "=r" (pFrame) : );
            #else
                __asm__ __volatile__("lwz %0, 0(1)" : "=r" (pFrame) : );
            #endif
        #elif defined(__MWERKS__)
            asm { stw r1, pFrame }
        #else
            pFrame = NULL;
        #endif

        pInstruction = NULL;  // To do: implement this.
    }

    for(int nStackIndex = 0; nEntryIndex < (nReturnAddressArrayCapacity - 1); ++nStackIndex)
    {
        if(pContext || (nStackIndex > 0)) 
            pReturnAddressArray[nEntryIndex++] = (void*)((uintptr_t)pInstruction - 4);

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
EACALLSTACK_API size_t GetModuleFromAddress(const void* pAddress, char* pModuleFileName, size_t moduleNameCapacity)
{
    EA_ASSERT(moduleNameCapacity > 0);

    if(RSOModuleInfoManager::sRSOModuleInfoManager) // If the user registered a module info manager...
    {
        const RSOModuleInfo* pModuleInfo = RSOModuleInfoManager::sRSOModuleInfoManager->GetRSOModuleInfo(NULL, NULL, pAddress);

        if(pModuleInfo)
        {
            strncpy(pModuleFileName, pModuleInfo->mModuleFileName.c_str(), moduleNameCapacity);
            pModuleFileName[moduleNameCapacity - 1] = 0;

            return pModuleInfo->mModuleFileName.length();
        }
    }

    pModuleFileName[0] = 0;
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// GetModuleHandleFromAddress
//
EACALLSTACK_API ModuleHandle GetModuleHandleFromAddress(const void* pAddress)
{
    if(RSOModuleInfoManager::sRSOModuleInfoManager) // If the user registered a module info manager...
    {
        const RSOModuleInfo* pModuleInfo = RSOModuleInfoManager::sRSOModuleInfoManager->GetRSOModuleInfo(NULL, NULL, pAddress);

        if(pModuleInfo)
            return pModuleInfo->mpRSOObjectHeader;
    }

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



///////////////////////////////////////////////////////////////////////////////
// RSOModuleInfoManager
///////////////////////////////////////////////////////////////////////////////


// This is a pointer which the user sets at runtime, usually on startup.
RSOModuleInfoManager* RSOModuleInfoManager::sRSOModuleInfoManager = NULL;


struct RSOSectionInfo // This is an undocumented Wii RSO struct.
{
    uint32_t offset;
    uint32_t size;
};

static void GetRSOTextSectionAddress(RSOObjectHeader* pRSOObjectHeader, RSOSectionInfo& sectionInfo) 
{ 
    u32 sectionText; 

    // Use unresolved/epilog/prolog section to locate .text section.
    if(pRSOObjectHeader->unresolved)
        sectionText = pRSOObjectHeader->unresolvedSection;   // If unresolved section exists, it will be in .text (should hopefully always have this).
    else if(pRSOObjectHeader->epilog)
        sectionText = pRSOObjectHeader->epilogSection;       // If prolog section exists, it will be in .text (if we're desperate).
    else if(pRSOObjectHeader->prolog)
        sectionText = pRSOObjectHeader->prologSection;       // If prolog section exists, it will be in .text (if we're desperate).
    else
        sectionText = 1;                                     // Fallback.

    const RSOObjectInfo&  objectInfo    = pRSOObjectHeader->info; 
    const RSOSectionInfo* pSectionTable = (const RSOSectionInfo*)objectInfo.sectionInfoOffset; 

    sectionInfo = pSectionTable[sectionText];
}


const RSOModuleInfo* RSOModuleInfoManager::AddRSOModule(const char* pModuleFileName, RSOObjectHeader* pRSOObjectHeader)
{
    mRSOModuleInfoArray.push_back();
    RSOModuleInfo& info = mRSOModuleInfoArray.back();
    ReadRSOModuleInfo(pModuleFileName, pRSOObjectHeader, info);

    return &info;
}


void RSOModuleInfoManager::RemoveRSOModule(RSOObjectHeader* pRSOObjectHeader)
{
    const RSOModuleInfo* pRSOModuleInfo = GetRSOModuleInfo(NULL, pRSOObjectHeader, NULL);

    if(pRSOModuleInfo)
    {
        mRSOModuleInfoArray.erase(const_cast<RSOModuleInfo*>(pRSOModuleInfo)); // Assumes that vector::iterator is an element pointer.

        // A slow implementation:
        //for(RSOModuleInfoArray::iterator it = mRSOModuleInfoArray.begin(); it != mRSOModuleInfoArray.end(); ++it)
        //{
        //    if(*it == *pRSOModuleInfo)
        //    {
        //        mRSOModuleInfoArray.erase(it);
        //        break;
        //    }
        //}
    }
}


const RSOModuleInfo* RSOModuleInfoManager::GetRSOModuleInfo(const char* pModuleFileName, const RSOObjectHeader* pRSOObjectHeader, const void* pCodeAddress) const
{
    for(eastl_size_t i = 0, iEnd = mRSOModuleInfoArray.size(); i < iEnd; ++i)
    {
        const RSOModuleInfo& info = mRSOModuleInfoArray[i];

        if(pModuleFileName && (EA::StdC::Stristr(info.mModuleFileName.c_str(), pModuleFileName)))
            return &info;

        if(pRSOObjectHeader && (info.mpRSOObjectHeader == pRSOObjectHeader))
            return &info;

        if(pCodeAddress && (info.mModuleTextOffset <= (uint32_t)pCodeAddress) && ((uint32_t)pCodeAddress < (info.mModuleTextOffset + info.mModuleTextSize)))
            return &info;
    }

    return NULL;
}


void RSOModuleInfoManager::ReadRSOModuleInfo(const char* pModuleFileName, RSOObjectHeader* pRSOObjectHeader, RSOModuleInfo& moduleInfo)
{
    RSOSectionInfo sectionInfo;

    GetRSOTextSectionAddress(pRSOObjectHeader, sectionInfo);

    if(pModuleFileName)
        moduleInfo.mModuleFileName   = pModuleFileName;
    moduleInfo.mpRSOObjectHeader  = pRSOObjectHeader;
    moduleInfo.mModuleTextOffset = sectionInfo.offset;
    moduleInfo.mModuleTextSize   = sectionInfo.size;
}



} // namespace Callstack
} // namespace EA




