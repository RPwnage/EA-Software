///////////////////////////////////////////////////////////////////////////////
// InputHook.h
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef INPUTHOOK_H
#define INPUTHOOK_H

#include "IGO.h"

#if defined(ORIGIN_PC)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include "eathread/eathread_futex.h"

namespace OriginIGO {

    enum MessageHookType
    {
        MessageHookType_MHOOK = 0,
        MessageHookType_WIN32
    };

    class MemoryUnProtector{

    public:
        MemoryUnProtector(LPVOID address, DWORD size);
        ~MemoryUnProtector();

        bool isOkay() { return mCallSucceeded; };

    private:
        DWORD mProtectionState;
        bool mCallSucceeded;
        DWORD mSize;
        PVOID mAddress;
    };


    class InputHook
    {
    public:
        // async hooking/unhooking
        static void TryHookLater(bool* isHooked = NULL); // do not block, hook using a new thread
        static void CleanupLater(); // do not block, cleanup using a new thread
        static void FinishCleanup();    // give chance to cleanup completely before exiting

        static void EnableWindowHooking();  // we need to re-enable the hooking of a window (which should happen on its UI thread) after a reset/cleanup
        static void HookWindow(HWND hwnd);
        static bool IsMessagHookActive();    

        // never call those from hooked DirectX or OpenGL calls!!! Use the async ones
        static bool TryHook();
        static void Cleanup();
        
        static bool IsDestroyed() { return mInputHookState==-1; }
        static HANDLE mIGOInputhookDestroyThreadHandle;
        static HANDLE mIGOInputhookCreateThreadHandle;

        static EA::Thread::Futex mInstanceHookMutex;

    public:
        // THIS SECTION IS PUBLIC BUT REALLY IS ONLY TO BE USED INTERNALLY BY INPUTHOOK.CPP
        static void HookWindowImpl(HWND hwnd);
        static MessageHookType volatile mMessageHookType;

    private:
        static EA::Thread::Futex mInstanceCleanupMutex;
        static int volatile mInputHookState;
        static bool volatile mWindowHooked;

    };

	// Helpers
	bool IsSystemWindow(HWND hWnd);

    #endif
}
#endif
