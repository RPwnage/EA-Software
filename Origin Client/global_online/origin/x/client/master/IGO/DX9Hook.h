///////////////////////////////////////////////////////////////////////////////
// DX9Hook.h
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef DX9HOOK_H
#define DX9HOOK_H

#include "IGO.h"

#if defined(ORIGIN_PC)
#include "eathread/eathread_futex.h"

namespace OriginIGO {

    class DX9Hook
    {
    public:
        DX9Hook();
        ~DX9Hook();
    
        static bool TryHook(bool calculateOffsetsOnly = false);
        static void Cleanup();

        static bool IsHooked() { return isHooked; };
        static bool IsReadyForRehook();
        static bool IsPrecalculationDone() { return mDX9Release && mDX9TestCooperativeLevel && mDX9Reset && mDX9SwapChainPresent && mDX9Present && mDX9SetGammaRamp; };
        static bool IsBroken();

        enum ShaderSupport
        {
            ShaderSupport_UNKNOWN = 0,
            ShaderSupport_FIXED,
            ShaderSupport_FULL
        };
        static enum ShaderSupport ShaderSupport();
		
        static bool mDX9Initialized;
        static LONG_PTR mDX9Release;
        static LONG_PTR mDX9TestCooperativeLevel;
        static LONG_PTR mDX9Reset;
        static LONG_PTR mDX9SwapChainPresent;
        static LONG_PTR mDX9Present;
        static LONG_PTR mDX9SetGammaRamp;
        static EA::Thread::Futex mInstanceHookMutex;

    private:
        static bool isHooked;
        static DWORD mLastHookTime;
    };
}
#endif

#endif
