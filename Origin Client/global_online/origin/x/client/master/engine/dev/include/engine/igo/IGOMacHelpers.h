///////////////////////////////////////////////////////////////////////////////
// IGOMacHelpers.h
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef QtInjector4_MAC_GOODIES
#define QtInjector4_MAC_GOODIES

#include "IGOQWidget.h"

#if defined(ORIGIN_MAC)

#include <sys/types.h>

namespace Origin
{
    namespace Engine
    {
        void SetupIGOWindow(void* winId, float alphaValue);
        void ForwardKeyData(void* winId, const char* data, int dataSize);
        bool IsFrontProcess();
        bool IsOIGWindow(void* winId);
        
        // Helper to make sure we did setup the shared memory used to interact with the different games.
        bool checkSharedMemorySetup();
    }
}

#endif

#endif
