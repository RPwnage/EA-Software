///////////////////////////////////////////////////////////////////////////////
// DXGIHook.h
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef DXGIHOOK_H
#define DXGIHOOK_H

#include "IGO.h"

#if defined(ORIGIN_PC)
#include "eathread/eathread_futex.h"

namespace OriginIGO {

    class DXGIHook
    {
    public:
        static void Setup();

        static bool IsDX10Hooked();
        static bool IsDX11Hooked();
        static bool IsDX12Hooked();
    };
}
#endif

#endif
