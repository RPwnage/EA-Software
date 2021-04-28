///////////////////////////////////////////////////////////////////////////////
// HookAPI.h
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef HOOKAPI_H
#define HOOKAPI_H

#include "IGOTrace.h"
#if !defined(ORIGIN_MAC)
#include "eathread/eathread_futex.h"
#include "eathread/eathread_rwspinlock.h"
#endif

#ifdef ORIGIN_MAC

namespace OriginIGO
{
    bool HookAPI(LPCSTR moduleName, LPCSTR apiName, LPVOID pCallback, LPVOID *pNextHook);
    bool UnhookAPI(LPCSTR moduleName, LPCSTR apiName, LPVOID* pNextHook);

    // Returns whether the Steam overlay is running
    bool IsSteamRunning();
}
#else

#pragma optimize( "", off )

namespace OriginIGO
{
#ifndef _WIN64

#ifndef DLL_EXPORT
#define DLL_EXPORT __declspec(dllexport)
#endif
    
    extern "C" DLL_EXPORT bool HookAPI(LPCSTR moduleName, LPCSTR apiName, LPVOID pCallback, LPVOID *pNextHook);
    int _HookCode(LPVOID pCode, LPVOID pCallback, LPVOID *pNextHook, bool noPreCheck);
    extern "C" DLL_EXPORT bool UnhookAPI(LPVOID *pNextHook, LPVOID pCallback);
    extern "C" DLL_EXPORT bool CheckHook(LPVOID *pNextHook, LPVOID pCallback, bool checkOriginalCode = false, bool insideHookCheck = false);
    extern "C" DLL_EXPORT bool IsHook(LPVOID *pNextHook, LPVOID unknownFunction);
    extern "C" DLL_EXPORT bool IsTargetModified(LPVOID pCode, DWORD opcodeSize);
    extern "C" DLL_EXPORT bool GetLastHookErrorDetails(char* buffer, size_t bufferSize);

#else

    bool HookAPI(LPCSTR moduleName, LPCSTR apiName, LPVOID pCallback, LPVOID *pNextHook);
    int _HookCode(LPVOID pCode, LPVOID pCallback, LPVOID *pNextHook, bool noPreCheck);
    bool UnhookAPI(LPVOID *pNextHook, LPVOID pCallback);
    bool IsHook(LPVOID *pNextHook, LPVOID unknownFunction);
    bool CheckHook(LPVOID *pNextHook, LPVOID pCallback, bool checkOriginalCode = false, bool insideHookCheck = false);
    bool IsTargetModified(LPVOID target, DWORD opcodeSize);
    bool GetLastHookErrorDetails(char* buffer, size_t bufferSize);

#endif
}

#ifdef _USE_IGO_LOGGER_ 
#include "IGOLogger.h"

#define HookCode(pCode, pCallback, pNextHook, result, noPreCheck)\
    {\
    result = OriginIGO::_HookCode(pCode, pCallback, pNextHook, noPreCheck);\
    if (! result) \
        OriginIGO::IGOLogWarn("Hooking failed: %s", # pCode); \
    }
   
#else

#define HookCode(pCode, pCallback, pNextHook, noPreCheck)\
    OriginIGO::_HookCode(pCode, pCallback, pNextHook, noPreCheck);
#endif

#endif

// pull the method function from the vtable
LPVOID GetInterfaceMethod(LPVOID intf, DWORD methodIndex);

#if !defined(ORIGIN_MAC)
#define DEFINE_HOOK(result, name, args) \
    typedef result (WINAPI *name ## Func) args; \
    name ## Func volatile name ## Next = NULL; \
    bool volatile is ## name ## ed = false; \
    result WINAPI name args

#define UNHOOK(name) \
    if (name ## Next != NULL && is ## name ## ed == true) { \
        is ## name ## ed = OriginIGO::UnhookAPI((LPVOID *)&(name ## Next), name) == false; \
                            }

// new "gated" hooking calls & signatures

#define PRE_DEFINE_HOOK_SAFE_VOID(name, args) \
    EA::Thread::RWSpinLock name ## Mutex; \
    typedef void (WINAPI *name ## Func) args; \
    name ## Func volatile name ## Next = NULL; \
    bool volatile is ## name ## ed = false; \
    extern void WINAPI name args;


#define PRE_DEFINED_HOOK_SAFE_VOID(name, args) \
    void WINAPI name args { \
    EA::Thread::AutoRWSpinLock name ## MutexInstanc(name ## Mutex, EA::Thread::AutoRWSpinLock::kLockTypeRead); \
    if (name ## Next != NULL && is ## name ## ed == true) { \
        if (!OriginIGO::CheckHook((LPVOID *)&(name ## Next), name, false, true)) \
            return; \
                }
				
#define EXTERN_HOOK_SAFE(result, name, args) \
    typedef result (WINAPI *name ## Func) args; \
    extern result WINAPI name args;

#define PRE_DEFINE_HOOK_SAFE(result, name, args) \
    EA::Thread::RWSpinLock name ## Mutex; \
    typedef result (WINAPI *name ## Func) args; \
    name ## Func volatile name ## Next = NULL; \
    bool volatile is ## name ## ed = false; \
    extern result WINAPI name args;

#define PRE_DEFINED_HOOK_SAFE(result, name, args) \
    result WINAPI name args { \
    EA::Thread::AutoRWSpinLock name ## MutexInstanc(name ## Mutex, EA::Thread::AutoRWSpinLock::kLockTypeRead); \
    if (name ## Next != NULL && is ## name ## ed == true) { \
            if (!OriginIGO::CheckHook((LPVOID *)&(name ## Next), name, false, true)) \
                return 0; \
                    }

#define PRE_DEFINED_HOOK_SAFE_NO_CHECK(result, name, args) \
    result WINAPI name args { \
    EA::Thread::AutoRWSpinLock name ## MutexInstanc(name ## Mutex, EA::Thread::AutoRWSpinLock::kLockTypeRead); \
    if (name ## Next != NULL && is ## name ## ed == true) { \
                    }

#define CheckHook_SAFE(name, result) \
    if (name ## Next != NULL && is ## name ## ed == true) { \
        result = OriginIGO::CheckHook((LPVOID *)&(name ## Next), name); \
                                    }

#define DEFINE_MESSAGE_CALLBACK_SAFE(result, name, args) \
    EA::Thread::Futex name ## Mutex; \
    typedef result (WINAPI *name ## Func) args; \
    name ## Func volatile name ## Next = NULL; \
    bool volatile is ## name ## ed = false; \
    result WINAPI name args { \
    EA::Thread::AutoFutex name ## MutexInstanc(name ## Mutex);

#ifdef _USE_IGO_LOGGER_ 

#define DEFINE_HOOK_SAFE(result, name, args) \
    EA::Thread::RWSpinLock name ## Mutex; \
    typedef result (WINAPI *name ## Func) args; \
    name ## Func volatile name ## Next = NULL; \
    bool volatile is ## name ## ed = false; \
    result WINAPI name args { \
    EA::Thread::AutoRWSpinLock name ## MutexInstanc(name ## Mutex, EA::Thread::AutoRWSpinLock::kLockTypeRead); \
    if (name ## Next != NULL && is ## name ## ed == true) { \
        if (!OriginIGO::CheckHook((LPVOID *)&(name ## Next), name, false, true)) { \
            return 0; } \
    } else \
        IGOLogWarn("Dead hook was called: %s", # name);

#define DEFINE_HOOK_SAFE_EX(result, name, args, mutex) \
    EA::Thread::RWSpinLock name ## Mutex; \
    typedef result (WINAPI *name ## Func) args; \
    name ## Func volatile name ## Next = NULL; \
    bool volatile is ## name ## ed = false; \
    result WINAPI name args { \
    EA::Thread::AutoFutex m(mutex); \
    EA::Thread::AutoRWSpinLock name ## MutexInstanc(name ## Mutex, EA::Thread::AutoRWSpinLock::kLockTypeRead); \
    if (name ## Next != NULL && is ## name ## ed == true) { \
    } else \
        IGOLogWarn("Dead hook was called: %s", # name);

#define DEFINE_HOOK_SAFE_VOID(name, args) \
    EA::Thread::RWSpinLock name ## Mutex; \
    typedef void (WINAPI *name ## Func) args; \
    name ## Func volatile name ## Next = NULL; \
    bool volatile is ## name ## ed = false; \
    void WINAPI name args { \
    EA::Thread::AutoRWSpinLock name ## MutexInstanc(name ## Mutex, EA::Thread::AutoRWSpinLock::kLockTypeRead); \
    if (name ## Next != NULL && is ## name ## ed == true) { \
        if (!OriginIGO::CheckHook((LPVOID *)&(name ## Next), name, false, true)) { \
            return; } \
                                } else \
            IGOLogWarn("Dead hook was called: %s", # name);


#else

#define DEFINE_HOOK_SAFE_NO_CODE_CHECK(result, name, args) \
    EA::Thread::RWSpinLock name ## Mutex; \
    typedef result (WINAPI *name ## Func) args; \
    name ## Func volatile name ## Next = NULL; \
    bool volatile is ## name ## ed = false; \
    result WINAPI name args { \
    EA::Thread::AutoRWSpinLock name ## MutexInstanc(name ## Mutex, EA::Thread::AutoRWSpinLock::kLockTypeRead);

#define DEFINE_HOOK_SAFE(result, name, args) \
    EA::Thread::RWSpinLock name ## Mutex; \
    typedef result (WINAPI *name ## Func) args; \
    name ## Func volatile name ## Next = NULL; \
    bool volatile is ## name ## ed = false; \
    result WINAPI name args { \
    EA::Thread::AutoRWSpinLock name ## MutexInstanc(name ## Mutex, EA::Thread::AutoRWSpinLock::kLockTypeRead); \
    if (name ## Next != NULL && is ## name ## ed == true) { \
        if (!OriginIGO::CheckHook((LPVOID *)&(name ## Next), name, false, true)) { \
            return 0; } \
                    }


#define DEFINE_HOOK_SAFE_VOID(name, args) \
    EA::Thread::RWSpinLock name ## Mutex; \
    typedef void (WINAPI *name ## Func) args; \
    name ## Func volatile name ## Next = NULL; \
    bool volatile is ## name ## ed = false; \
    void WINAPI name args { \
    EA::Thread::AutoRWSpinLock name ## MutexInstanc(name ## Mutex, EA::Thread::AutoRWSpinLock::kLockTypeRead); \
    if (name ## Next != NULL && is ## name ## ed == true) { \
        if (!OriginIGO::CheckHook((LPVOID *)&(name ## Next), name, false, true)) { \
            return; } \
                                }
#endif



#define UNHOOK_SAFE(name) \
    if (name ## Next != NULL && is ## name ## ed == true) { \
        EA::Thread::AutoRWSpinLock name ## MutexInstanc(name ## Mutex, EA::Thread::AutoRWSpinLock::kLockTypeWrite); \
        if (name ## Next != NULL && is ## name ## ed == true) \
            is ## name ## ed =  OriginIGO::UnhookAPI((LPVOID *)&(name ## Next), name) == false; \
    }

//GetMessage is blocking, so we try for 1 second to get a lock, if that fails, skip unhooking
#define UNHOOK_SAFE_GETMESSAGE(name) \
    if (name ## Next != NULL && is ## name ## ed == true) \
    { \
        int i=0; \
        bool locked = false; \
        do { \
            Sleep(20); \
            i++; \
            locked = name ## Mutex.WriteTryLock(); \
        } \
        while (i<50 && !locked); \
        if (locked) \
        { \
            is ## name ## ed = OriginIGO::UnhookAPI((LPVOID *)&(name ## Next), name) == false; \
            name ## Mutex.WriteUnlock(); \
        } \
        else \
        { \
            OriginIGO::IGOLogWarn("unhooking GetMessage failed: %s", # name); \
        } \
    }


#define HOOKAPI_SAFE(api, call, name) \
    if ( is ## name ## ed == false ) { \
        EA::Thread::AutoRWSpinLock name ## MutexInstanc(name ## Mutex, EA::Thread::AutoRWSpinLock::kLockTypeWrite); \
        if ( is ## name ## ed == false ) \
            is ## name ## ed = OriginIGO::HookAPI(api, call, name, (LPVOID *)& name ## Next); \
    }

#define HOOKCODE_SAFE(ptr, name) \
    if ( is ## name ## ed == false ) { \
        EA::Thread::AutoRWSpinLock name ## MutexInstanc(name ## Mutex, EA::Thread::AutoRWSpinLock::kLockTypeWrite); \
        if ( is ## name ## ed == false ) { \
            int result = false; \
            HookCode(ptr, name, (LPVOID *)& name ## Next, result, false); \
            is ## name ## ed = (result == TRUE); \
        }\
    }


#define HOOKCODE_SAFE_NO_CHECK(ptr, name, result) \
    if ( is ## name ## ed == false ) { \
        EA::Thread::AutoRWSpinLock name ## MutexInstanc(name ## Mutex, EA::Thread::AutoRWSpinLock::kLockTypeWrite); \
        if ( is ## name ## ed == false ) { \
            HookCode(ptr, name, (LPVOID *)& name ## Next, result, true); \
            is ## name ## ed = (result == TRUE); \
        }\
    }



#define EXTERN_NEXT_FUNC(result, name, args) \
    typedef result (WINAPI *name ## Func) args; \
    extern name ## Func volatile name ## Next; \
    extern bool volatile is ## name ## ed;

#pragma optimize( "", on )

#else

#define DEFINE_HOOK(result, name, args) \
    typedef result (WINAPI *name ## Func) args; \
    name ## Func volatile name ## Next = NULL; \
    result WINAPI name args

#endif // ORIGIN_MAC

#endif
