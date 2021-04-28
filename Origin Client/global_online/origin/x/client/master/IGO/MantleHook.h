///////////////////////////////////////////////////////////////////////////////
// MantleHook.h
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2013 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef MantleHOOK_H
#define MantleHOOK_H

#include "IGO.h"

#if defined(ORIGIN_PC)
#include "eathread/eathread_futex.h"
#include "mantle.h"

class CommandBuffer;

namespace OriginIGO {

	class tRenderTarget;

    class MantleHook
    {
    public:
		typedef struct DevContext{
			int deviceIndex;
			CommandBuffer *cmdBuffer;
			tRenderTarget* rt;
			GR_QUEUE queue;
			bool fullscreen;
			GR_IMAGE_STATE oldState;
			GR_IMAGE_STATE newState;

		};
		
		
		MantleHook();
	    ~MantleHook();

        static void CleanupLater();

        static bool TryHook();
        static void Cleanup();
        static HANDLE mIGOMantleHookDestroyThreadHandle;
        static bool IsHooked() { return isHooked; };
    private:
        static EA::Thread::Futex mInstanceHookMutex;
        static bool isHooked;
    };
}
#endif

#endif
