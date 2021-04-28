///////////////////////////////////////////////////////////////////////////////
// DX8Hook.h
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef DX8HOOK_H
#define DX8HOOK_H

#ifndef _WIN64
#include "IGO.h"

#if defined(ORIGIN_PC)
#include "eathread/eathread_futex.h"
namespace OriginIGO {

    class DX8Hook
    {
    public:
        DX8Hook();
        ~DX8Hook();

        static bool TryHook(bool calculateOffsetsOnly = false);
        static void Cleanup();

        static bool IsHooked() { return isHooked; };
        static bool IsReadyForRehook();
        static bool IsPrecalculationDone();
        static bool IsBroken();

        static bool mDX8Initialized;
        static LONG_PTR mDX8Reset;
        static LONG_PTR mDX8Present;
        static LONG_PTR mDX8CreateDevice;
        static EA::Thread::Futex mInstanceHookMutex;

    private:
        static bool isHooked;
        static DWORD mLastHookTime;
    };
}
#endif

#endif // _WIN64

#endif