///////////////////////////////////////////////////////////////////////////////
// DX10Hook.h
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef DX10HOOK_H
#define DX10HOOK_H

#include "IGO.h"

#if defined(ORIGIN_PC)
#include <DXGI.h>
#include "../IGOProxy/IGOAPIInfo.h"
#include "eathread/eathread_futex.h"

namespace OriginIGO {

    class DX10Hook
    {
    public:
        DX10Hook();
        ~DX10Hook();

        static void Cleanup();
        static void CleanupLater();
        static HANDLE mIGODX10HookDestroyThreadHandle;
        //static bool IsReadyForRehook();

        static ULONG ReleaseHandler(IDXGISwapChain *pSwapChain, IGOAPIDXReleaseFcn ReleaseFcn);
        static bool PresentHandler(IDXGISwapChain *pSwapChain, UINT SyncInterval, UINT Flags, HRESULT* Result, IGOAPIDXPresentFcn PresentFcn);
        static bool ResizeHandler(IDXGISwapChain *pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags, HRESULT* Result, IGOAPIDXResizeFcn ResizeFcn);
        static bool IsValidDevice(IDXGISwapChain* swapChain);

        static ULONG_PTR mSwapChainInterface;
    private:
        static EA::Thread::Futex mInstanceHookMutex;
        static DWORD mLastHookTime;
    };
}
#endif

#endif
