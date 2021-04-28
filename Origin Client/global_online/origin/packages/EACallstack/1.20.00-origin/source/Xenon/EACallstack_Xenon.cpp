///////////////////////////////////////////////////////////////////////////////
// EACallstack_Xenon.cpp
//
// Copyright (c) 2003-2005 Electronic Arts Inc.
// Created by Paul Pedriana
//
// This implementation is based on previous work by:
//     Simon Everett
//     Vasyl Tsvirkunov
//     Avery Lee
//     Paul Pedriana
///////////////////////////////////////////////////////////////////////////////


#include <EACallstack/EACallstack.h>
#include <EACallstack/Context.h>
#include <eathread/eathread_thread.h>
#include <eathread/eathread_storage.h>
#include <stdio.h>
#include EA_ASSERT_HEADER

#if defined(EA_PLATFORM_XENON)
    #pragma warning(push, 1)
    #include <xtl.h>
    #if EA_XBDM_ENABLED
        #include <xbdm.h>
        // #pragma comment(lib, "xbdm.lib") The application is expected to provide this if needed. 
    #endif
    #pragma warning(pop)

    extern "C" LONG ObReferenceObjectByHandle(HANDLE Handle, DWORD DesiredAccess, void** Object);
#endif


namespace EA
{
namespace Thread
{
    // Older versions of EAThread don't expose this function, so we do so here.
    EAThreadDynamicData* FindThreadDynamicData(EA::Thread::ThreadId threadId);
}

namespace Callstack
{


/* To consider: Enable usage of this below.
///////////////////////////////////////////////////////////////////////////////
// IsAddressReadable
//
static bool IsAddressReadable(const void* pAddress)
{
    // Cannot use VirtualQuery here as it will fail on call stack. Fortunately
    // there is functional replacement for it.
    // Side note: I have no idea why the parameter is declared as LPVOID even
    // if both documentation and common sense call for LPCVOID.
    const DWORD pageAccess = XQueryMemoryProtect((LPVOID)pAddress);

    return (pageAccess & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) != 0;
}
*/


///////////////////////////////////////////////////////////////////////////////
// GetInstructionPointer
//
EACALLSTACK_API void GetInstructionPointer(void*& pInstruction)
{
    uint64_t gpr1;
    __asm std r1, gpr1;

    uintptr_t* const pParentFrame = (uintptr_t*)*((uintptr_t*)gpr1);
    pInstruction = (void*)pParentFrame[-2];

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
        uint64_t gpr1;
        __asm std r1, gpr1;
        pFrame = (uintptr_t*)gpr1;

        GetInstructionPointer(pInstruction);
    }

    for(int nStackIndex = 0; nEntryIndex < (nReturnAddressArrayCapacity - 1); ++nStackIndex)
    {
        if(pContext || (nStackIndex > 0)) 
            pReturnAddressArray[nEntryIndex++] = pInstruction;

        if(pFrame == NULL)
            break;

        const void* const pPrevFrame = pFrame;
        pFrame = (uintptr_t*)*pFrame;           // To consider: Use IsAddressReadable(pFrame) before dereferencing this pointer.

        if((pContext != NULL) && (pContext->mStackBase != 0))
        {
            //do the uber safety check
            //Check if frame is within the expected range
            //Frame is not a multiple of 16 (should be 16 byte aligned)
            if(!(((uintptr_t)pFrame >= (uintptr_t)pContext->mStackLimit ) && ((uintptr_t)pFrame <= (uintptr_t)pContext->mStackBase )) || (!((uintptr_t)pFrame % 0x10 == 0)))
                break;
        }
        else if(!pFrame || ((uintptr_t)pFrame & 15) || (pFrame < pPrevFrame))
        {
            //standard safety check
            // If pFrame is 0 or is not aligned as expected (4 bytes) or is 
            // a pointer to an address thats before pPrevFrame, we quit.
            break;
        }

        pInstruction = (void*)pFrame[-2];
    }

    pReturnAddressArray[nEntryIndex] = 0;
    return nEntryIndex;
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstackContext
//
// A sysThreadId is a Microsoft DWORD thread id, which can be obtained from 
// the currently running thread via GetCurrentThreadId. It can be obtained from
// a Microsoft thread HANDLE via EA::Callstack::GetThreadIdFromThreadHandle();
// A DWORD thread id can be converted to a thread HANDLE via the Microsoft OpenThread
// system function.
//
EACALLSTACK_API bool GetCallstackContext(CallstackContext& context, intptr_t threadId)
{
    // Verify that our Context struct matches Microsoft's.
    #if defined(EA_PROCESSOR_POWERPC)
        EA_COMPILETIME_ASSERT(offsetof(EA::Callstack::Context, mIar) == offsetof(CONTEXT, Iar));
        EA_COMPILETIME_ASSERT(offsetof(EA::Callstack::Context, mVscr) == offsetof(CONTEXT, Vscr));
    #endif

    bool bReturnValue = false;

    // The following is dependent on the thread being created by EAThread.
    // An alternative approach suggested by Toan Pham might be to use 
    // DmGetThreadList to help us find the thread id.

    EA::Thread::ThreadId currentThreadId = EA::Thread::GetThreadId();

    if(currentThreadId == (EA::Thread::ThreadId)threadId)
    {
        uint64_t gpr1;
        __asm std r1, gpr1;
        context.mGPR1 = (uintptr_t)gpr1;

        void* pIP;
        GetInstructionPointer(pIP);
        context.mIAR        = (uintptr_t)pIP;
      //context.mGPR1       = ___; // To do.
      //context.mStackBase  = ___;
      //context.mStackLimit = ___;

        bReturnValue = true;
    }
    else
    {
        #if EA_XBDM_ENABLED
            EAThreadDynamicData* pThreadDynamicData = EA::Thread::FindThreadDynamicData((EA::Thread::ThreadId)threadId);

            if(pThreadDynamicData)
            {
                // In this case we are working with a separate thread, so we suspend it
                // and read information about it and then resume it.
                SuspendThread((HANDLE)threadId);

                CONTEXT xenonCONTEXT;
                xenonCONTEXT.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
                HRESULT resultThreadContext = DmGetThreadContext(pThreadDynamicData->mnThreadId, &xenonCONTEXT);

                uint32_t sysThreadId = GetThreadIdFromThreadHandle(threadId);
                DM_THREADINFOEX tempThreadInfo;
                tempThreadInfo.Size = sizeof(tempThreadInfo);
                HRESULT resultThreadInfo = DmGetThreadInfoEx((DWORD)sysThreadId, &tempThreadInfo);

                if((resultThreadContext == XBDM_NOERR) && (resultThreadInfo == XBDM_NOERR))
                {
                    context.mGPR1       = (uintptr_t)xenonCONTEXT.Gpr1;
                    context.mIAR        = (uintptr_t)xenonCONTEXT.Iar;
                    context.mStackBase  = (uintptr_t)tempThreadInfo.StackBase;
                    context.mStackLimit = (uintptr_t)tempThreadInfo.StackLimit;

                    bReturnValue = true;
                }

                ResumeThread((HANDLE)threadId);
            }
        #else
            // Not much we can do.
        #endif
    }

    return bReturnValue;


    /* This simpler version is not always working. Would be nice to find out why.
    bool bReturnValue = false;

    if(threadId == 0) // If the user wants to get the current thread's context...
    {
        uint64_t gpr1;
        __asm std r1, gpr1;
        context.mGPR1 = (uintptr_t)gpr1;

        void* pIP;
        GetInstructionPointer(pIP);
        context.mIAR = (uintptr_t)pIP;

        bReturnValue = true;
    }
    else
    {
        const uint32_t sysThreadId = GetThreadIdFromThreadHandle(threadId);

        bReturnValue = GetCallstackContextSysThreadId(context, sysThreadId);
    }

    return bReturnValue;
    */
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstackContextSysThreadId
//
// A sysThreadId is a Microsoft DWORD thread id, which can be obtained from 
// the currently running thread via GetCurrentThreadId. It can be obtained from
// a Microsoft thread HANDLE via EA::Callstack::GetThreadIdFromThreadHandle();
// A DWORD thread id can be converted to a thread HANDLE via the Microsoft OpenThread
// system function.
//
EACALLSTACK_API bool GetCallstackContextSysThreadId(CallstackContext& context, intptr_t sysThreadId)
{
    bool  bReturnValue       = false;
    DWORD sysThreadIdCurrent = ::GetCurrentThreadId();

    if(sysThreadId == (intptr_t)EA::Thread::kSysThreadIdInvalid)
        sysThreadId = (intptr_t)::GetCurrentThreadId();

    if(sysThreadIdCurrent == (DWORD)sysThreadId)
    {
        uint64_t gpr1;
        __asm std r1, gpr1;
        context.mGPR1 = (uintptr_t)gpr1;

        void* pIP;
        GetInstructionPointer(pIP);
        context.mIAR        = (uintptr_t)pIP;
      //context.mGPR1       = ___; // To do.
      //context.mStackBase  = ___;
      //context.mStackLimit = ___;

        bReturnValue = true;
    }
    else
    {
        #if EA_XBDM_ENABLED
            // In this case we are working with a separate thread, so we suspend it
            // and read information about it and then resume it.
            HANDLE threadId = ::OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT, TRUE, (DWORD)sysThreadId);

            if(threadId)
            {
                SuspendThread(threadId);

                CONTEXT xenonCONTEXT;
                xenonCONTEXT.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
                HRESULT resultThreadContext = DmGetThreadContext((DWORD)sysThreadId, &xenonCONTEXT);

                DM_THREADINFOEX tempThreadInfo;
                tempThreadInfo.Size = sizeof(tempThreadInfo);
                HRESULT resultThreadInfo = DmGetThreadInfoEx((DWORD)sysThreadId, &tempThreadInfo);

                if((resultThreadContext == XBDM_NOERR) && (resultThreadInfo == XBDM_NOERR))
                {
                    context.mGPR1       = (uintptr_t)xenonCONTEXT.Gpr1;
                    context.mIAR        = (uintptr_t)xenonCONTEXT.Iar;
                    context.mStackBase  = (uintptr_t)tempThreadInfo.StackBase;
                    context.mStackLimit = (uintptr_t)tempThreadInfo.StackLimit;

                    bReturnValue = true;
                }

                ResumeThread(threadId);
                CloseHandle(threadId);
            }
        #endif
    }

    return bReturnValue;
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
EACALLSTACK_API size_t GetModuleFromAddress(const void* address, char* pModuleName, size_t moduleNameCapacity)
{
    pModuleName[0] = 0;

    #if EA_XBDM_ENABLED
        DMN_MODLOAD      ModLoad;
        PDM_WALK_MODULES pWalkMod = NULL;
   
        while(DmWalkLoadedModules(&pWalkMod, &ModLoad) == XBDM_NOERR)
        {
            if((address >= ModLoad.BaseAddress) && (address < ((BYTE*)ModLoad.BaseAddress + ModLoad.Size)))
            {
                strncpy(pModuleName, ModLoad.Name, moduleNameCapacity);
                pModuleName[moduleNameCapacity - 1] = 0;
                break;
            }
        }

        DmCloseLoadedModules(pWalkMod);
    #else
        (void)address;
        (void)pModuleName;
        (void)moduleNameCapacity;
    #endif

    return strlen(pModuleName);
}


///////////////////////////////////////////////////////////////////////////////
// GetModuleHandleFromAddress
//
EACALLSTACK_API ModuleHandle GetModuleHandleFromAddress(const void* pAddress)
{
    #if EA_XBDM_ENABLED
        DMN_MODLOAD      ModLoad;
        PDM_WALK_MODULES pWalkMod = NULL;
   
        while(DmWalkLoadedModules(&pWalkMod, &ModLoad) == XBDM_NOERR)
        {
            if((pAddress >= ModLoad.BaseAddress) && (pAddress < ((BYTE*)ModLoad.BaseAddress + ModLoad.Size)))
                return (ModuleHandle)ModLoad.BaseAddress;
        }

        DmCloseLoadedModules(pWalkMod);
    #else
        (void)pAddress;
    #endif

    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// ThreadHandlesAreEqual
//
// Due to the way Windows works, you can't compare thread handles for equality.
// You instead need to somehow detect that they refer to the same thing.
// The Xenon thread API is highly resistant to the user being able to detect 
// if two thread handles refer to the same thread. 
//
EACALLSTACK_API bool ThreadHandlesAreEqual(intptr_t threadId1, intptr_t threadId2)
{
    bool   bReturnValue = false;
    HANDLE threadId1Dup, threadId2Dup;

    BOOL bResult1 = DuplicateHandle(GetCurrentProcess(), (HANDLE)threadId1, GetCurrentProcess(), &threadId1Dup, 0, true, DUPLICATE_SAME_ACCESS);
    BOOL bResult2 = DuplicateHandle(GetCurrentProcess(), (HANDLE)threadId2, GetCurrentProcess(), &threadId2Dup, 0, true, DUPLICATE_SAME_ACCESS);

    if(bResult1 && bResult2)
    {
        uint32_t id1 = GetThreadIdFromThreadHandle((intptr_t)threadId1Dup);
        uint32_t id2 = GetThreadIdFromThreadHandle((intptr_t)threadId2Dup);

        bReturnValue = (id1 == id2);
    }
    else
    {
        // The only way I can think of right now to possibly test if two threads
        // are equivalent by thread handle is to see if their ThreadTimes are 
        // equivalent. Xenon provides no documented way to convert a thread handle 
        // to a thread id.
        FILETIME ftCT1, ftET1, ftKT1, ftUT1;
        FILETIME ftCT2, ftET2, ftKT2, ftUT2;

        if(GetThreadTimes((HANDLE)threadId1, &ftCT1, &ftET1, &ftKT1, &ftUT1) &&
           GetThreadTimes((HANDLE)threadId2, &ftCT2, &ftET2, &ftKT2, &ftUT2))
        {
            if((ftCT1.dwHighDateTime == ftCT2.dwHighDateTime) && 
               (ftCT1.dwLowDateTime  == ftCT2.dwLowDateTime))
            {
                // To consider: Perhaps we need to compare UT here for approximate
                // equality instead of absolute equality. Perhaps there isn't much
                // we can do at all here in a reliable way.
                if((ftUT1.dwHighDateTime == ftUT2.dwHighDateTime) && 
                   (ftUT1.dwLowDateTime  == ftUT2.dwLowDateTime))
                {
                    bReturnValue = true;
                }
            }
        }
    }

    // The Xenon XDK documentation says that we don't need to close the handle returned by DuplicateHandle.
    CloseHandle(threadId1Dup);
    CloseHandle(threadId2Dup);
    
    return bReturnValue;
}


EACALLSTACK_API uint32_t GetThreadIdFromThreadHandle(intptr_t threadId)
{
    // What we do here is use the XBox 360 (a.k.a. Xenon) ObReferenceObjectByHandle 
    // kernel function to convert a HANDLE to a KIP_KTHREAD struct. Both the function
    // and the struct are not documented and are subject to change in future SDKs.
    // This function should not be called in release build code, as it isn't guaranteed
    // to work with future XBox 360 machines. It may also be a violation of the Microsoft
    // TRC (technical requirements checklist) to use it in shipping code.
    void* pVoidKThread = NULL;
    LONG lResult = ObReferenceObjectByHandle((HANDLE)threadId, 0, &pVoidKThread);

    if(lResult == 0)
    {
        const uint32_t* const pKThread = (uint32_t*)pVoidKThread;
        return pKThread[83];
    }

    return 0;
}


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
        // Code copied from GetStackLimit(). Can't call GetStackLimit because doing so would disturb the stack.
        uint64_t gpr1;
        __asm std r1, gpr1;
        pStackBase = (void*)(uint32_t)gpr1;

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
    #if EA_XBDM_ENABLED
        DM_THREADINFOEX tempThreadInfo;
        tempThreadInfo.Size = sizeof(tempThreadInfo);
        DWORD dwThread = GetCurrentThreadId();
        HRESULT resultThreadInfo = DmGetThreadInfoEx(dwThread, &tempThreadInfo);

        if(resultThreadInfo == XBDM_NOERR)
            return tempThreadInfo.StackBase;
        // Else fall through.
    #endif

    return sStackBase.GetValue();
}


///////////////////////////////////////////////////////////////////////////////
// GetStackLimit
//
EACALLSTACK_API void* GetStackLimit()
{
    #if EA_XBDM_ENABLED
        DM_THREADINFOEX tempThreadInfo;
        tempThreadInfo.Size = sizeof(tempThreadInfo);
        DWORD dwThread = GetCurrentThreadId();
        HRESULT resultThreadInfo = DmGetThreadInfoEx(dwThread, &tempThreadInfo);

        if(resultThreadInfo == XBDM_NOERR)
            return tempThreadInfo.StackLimit;
        // Else fall through.
    #endif

    void* pStack = NULL;

    uint64_t gpr1;
    __asm std r1, gpr1;
    pStack = (void*)(uint32_t)gpr1;

    return (void*)((uintptr_t)pStack & ~4095); // Round down to nearest page.
}


} // namespace CallStack
} // namespace EA




