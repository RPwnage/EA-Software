///////////////////////////////////////////////////////////////////////////////
// HookAPI.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "HookAPI.h"

#if defined(ORIGIN_PC)

#include "IGOSharedStructs.h"
#include "Mhook/mhook.h"
#include "IGOTelemetry.h"

#ifdef _USE_IGO_LOGGER_        // we only use the logger in the igo32.dll, not in the Origin.exe !!!
#include "IGOLogger.h"
#endif

extern "C" {
    LPVOID  WINAPI GetImageProcAddress(HMODULE hModule, LPCSTR name, BOOL doubleCheck = TRUE);
    HMODULE WINAPI GetRemoteModuleHandle32(HANDLE ProcessHandle, LPCWSTR DllName, LPWSTR DllFullPath);
    HMODULE WINAPI GetRemoteModuleHandle64(HANDLE ProcessHandle, LPCWSTR DllName);
    BOOL WINAPI GetImageProcAddressesRaw(LPCWSTR ModuleFileName, PVOID ImageBaseAddress, LPCSTR *ApiNames, PVOID *ProcAddresses, int ApiCount);

    LPVOID WINAPI FindRealCode(LPVOID pCode, bool loadCheckOnly = false);
    BOOL WINAPI FindModule(LPVOID pCode, HMODULE *hModule, LPSTR moduleName, DWORD moduleNameSize);
    BOOL WINAPI GetImageProcName(HMODULE hModule, LPVOID proc, LPSTR apiName, DWORD procNameSize);
}

namespace OriginIGO
{
    
bool HookAPI(LPCSTR moduleName, LPCSTR apiName, LPVOID pCallback, LPVOID *pNextHook)
{
    HMODULE hModule = GetModuleHandleA(moduleName);
    if (!hModule)
        return false;

    if(!pNextHook)
        return false;

    *pNextHook = (PVOID)GetImageProcAddress(hModule, apiName, true);

    if (!*pNextHook)
    {
#ifdef _USE_IGO_LOGGER_
        OriginIGO::IGOLogWarn("GetImageProcAddress failed: %s", apiName);
#endif
        return false;
    }

#ifdef _DEBUG
    LPVOID apiAddress = *pNextHook;
#endif

    bool s = Mhook_SetHook(hModule, moduleName, apiName, pNextHook, pCallback, false, false) ? true : false;
#ifdef _USE_IGO_LOGGER_
    if (!s)
        OriginIGO::IGOLogWarn("Hooking failed: %s", apiName);
    else
    {
#ifdef _DEBUG
        IGOLogDebug("*****Hooking : %s (%s) %p -> %p (%p)", apiName, moduleName, pCallback, *pNextHook, apiAddress);
#endif
    }
#endif
    // if we fail to hook, we need to make sure the pNextHook is NULL to avoid later confusion and make sure the default function is used
    if (!s)
    {
        *pNextHook = NULL;
    }
    return s;
}

// mhook

int _HookCode(LPVOID pCode, LPVOID pCallback, LPVOID *pNextHook, bool noPreCheck)
{
    if(!pNextHook)
        return FALSE;

    if(!pCode)
        return FALSE;

    *pNextHook = pCode;

    if (!*pNextHook)
        return FALSE;    

    pCode = FindRealCode(pCode, noPreCheck);

    if (!pCode)
    {
#ifdef _USE_IGO_LOGGER_
        OriginIGO::IGOLogWarn("HookCode: Detected DLL unloaded - resetting hooks.");
#endif
        CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_INFO, OriginIGO::TelemetryContext_GENERIC, OriginIGO::TelemetryRenderer_UNKNOWN, "Detected DLL unloaded - resetting hooks."));
        return FALSE;
    }

    if (pCode == (LPVOID)((LONG_PTR)-1))
    {
#ifdef _USE_IGO_LOGGER_
        OriginIGO::IGOLogWarn("HookCode: Detected code is PAGE_EXECUTE_READWRITE - possible DRM, aborting.");
#endif
        CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_INFO, OriginIGO::TelemetryContext_GENERIC, OriginIGO::TelemetryRenderer_UNKNOWN, "Detected code is PAGE_EXECUTE_READWRITE - possible DRM, aborting."));
        return -1;
    }

    HMODULE hModule = NULL;
    char moduleName[MAX_PATH];
    char apiName[MAX_PATH];
    *moduleName = '\0';
    *apiName = '\0';
    if (FindModule(pCode, &hModule, moduleName, MAX_PATH))
    {
        GetImageProcName(hModule, pCode, apiName, MAX_PATH);
    }

    bool s = Mhook_SetHook(hModule, moduleName, apiName, pNextHook, pCallback, false, noPreCheck) ? true : false;

#ifdef _USE_IGO_LOGGER_
    if (!s)
        OriginIGO::IGOLogWarn("Hooking failed: %p", pNextHook);
#endif

    return s;
}

bool CheckHook(LPVOID *pNextHook, LPVOID pCallback, bool checkOriginalCode, bool insideHookCheck)
{
    if (!pNextHook)
        return false;

    if (!*pNextHook)
        return false;

    bool s = Mhook_CheckHook(pNextHook, &pCallback, checkOriginalCode, insideHookCheck) ? true : false;

#ifdef _USE_IGO_LOGGER_
    if (!s)
    {
        OriginIGO::IGOLogWarn("Hook was altered: %p", pNextHook);
    }
#endif

    return s;
}


bool IsTargetModified(LPVOID target, DWORD opcodeSize)
{
    if (!target)
        return FALSE;

    return Mhook_CheckTarget(target, opcodeSize) == TRUE;
}


bool IsHook(LPVOID *pNextHook, LPVOID unknownFunction)
{
    if (!pNextHook)
        return false;

    if (!*pNextHook)
        return false;

    bool s = Mhook_IsHook(pNextHook, unknownFunction) ? true : false;

    return s;
}


bool UnhookAPI(LPVOID *pNextHook, LPVOID pCallback)
{
    if (!pNextHook)
        return false;

    if (!*pNextHook)
        return false;

#ifdef _USE_IGO_LOGGER_
#ifdef _DEBUG
    IGOLogDebug("*****Unhooking : %p -> %p", pCallback, *pNextHook);
#endif
#endif

    bool s = Mhook_Unhook(pNextHook, &pCallback) ? true : false;
    
#ifdef _USE_IGO_LOGGER_
    if (!s)
        OriginIGO::IGOLogWarn("Unhooking failed: %p", pNextHook);
#endif

    return s;
}

bool GetLastHookErrorDetails(char* buffer, size_t bufferSize)
{
    return Mhook_LastErrorDetails(buffer, bufferSize);
}

} // Origin


LPVOID GetInterfaceMethod(LPVOID intf, DWORD methodIndex)
{
#ifdef _WIN64
    return *(LPVOID*)(*(ULONG_PTR*)intf + methodIndex * 8);
#else
    return *(LPVOID*)(*(ULONG_PTR*)intf + methodIndex * 4);
#endif
}

#elif defined(ORIGIN_MAC) // MACOSX

#include "IGOLogger.h"
#include "IGOTelemetry.h"
#include "MacMach_override.h"
#include </usr/include/dlfcn.h>
#include </usr/include/mach-o/dyld.h>

namespace OriginIGO
{

bool HookAPI(LPCSTR moduleName, LPCSTR apiName, LPVOID pCallback, LPVOID *pNextHook)
{
    OriginIGO::IGOLogInfo("Do '%s'", apiName);
    
    if (!apiName || !pCallback)
        return false;
    
    // we don't want to rehook
    if (pNextHook && *pNextHook)
        return false;
    
#if 0 // def _DEBUG
    {
        char altName[MAX_PATH];
        snprintf(altName, sizeof(altName), "_%s", apiName);
        
        uint32_t imageCount = _dyld_image_count();
        for (uint32_t imageIdx = 0; imageIdx < imageCount; ++imageIdx)
        {
            const char* imageName = _dyld_get_image_name(imageIdx);
            void* handle = dlopen(imageName, RTLD_NOW);
            if (handle)
            {
                void* fcn = dlsym(handle, apiName);
                if (fcn)
                {
                    IGO_TRACEF("FOUND '%s' in '%s'", apiName, imageName);
                }

                fcn = dlsym(handle, altName);
                if (fcn)
                {
                    IGO_TRACEF("FOUND '%s' in '%s'", altName, imageName);
                }
                
                dlclose(handle);
            }
            
            else
                IGO_TRACEF("Unable to open module '%s'", imageName);
        }
        
    }
#endif
    
    void* handle = (moduleName == NULL) ? RTLD_DEFAULT : dlopen(moduleName, RTLD_NOW);
    if (!handle)
    {
        OriginIGO::IGOLogWarn("Unable to open module '%s' for '%s'", moduleName, apiName);
        return false;
    }
    
    void* fcn = dlsym(handle, apiName);
    
    if (handle != RTLD_DEFAULT)
        dlclose(handle);
    
    if (!fcn)
    {
        OriginIGO::IGOLogWarn("Unable to find '%s'", apiName);
        return false;
    }
    
#ifdef _DEBUG
    Dl_info info;
    if (dladdr(fcn, &info))
    {
        if (info.dli_fname)
        {
            OriginIGO::IGOLogInfo("Symbol '%s' picked from library '%s'", apiName, info.dli_fname);
        }
        
        else
        {
            OriginIGO::IGOLogInfo("Unable to grab image name for symbol '%s'", apiName);
        }
    }
    
    else
        IGO_TRACEF("Unable to find image containing the symbol '%s'", apiName);
#endif
    
    mach_error_t err = mach_override_ptr(fcn, pCallback, pNextHook);
    if (err)
        TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_INFO, OriginIGO::TelemetryContext_MHOOK, OriginIGO::TelemetryRenderer_OPENGL, "Failed to hook '%s' (0x%08x)", apiName, err);
    
    return err == 0;
}

bool UnhookAPI(LPCSTR moduleName, LPCSTR apiName, LPVOID* pNextHook)
{
    OriginIGO::IGOLogInfo("Undo '%s'", apiName);
    
    if (!apiName || !pNextHook || !(*pNextHook))
        return false;
    
    void* handle = (moduleName == NULL) ? RTLD_DEFAULT : dlopen(moduleName, RTLD_NOW);
    if (!handle)
    {
        OriginIGO::IGOLogWarn("Unable to open module '%s' for '%s'", moduleName, apiName);
        return false;
    }
    
    void* fcn = dlsym(handle, apiName);
    
    if (handle != RTLD_DEFAULT)
        dlclose(handle);
        
    if (!fcn)
    {
        OriginIGO::IGOLogWarn("Unable to find '%s'", apiName);
        return false;
    }

    mach_error_t err = mach_reverse_override_ptr(fcn, (LPVOID*)*pNextHook);
    if (err)
        TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_INFO, OriginIGO::TelemetryContext_MHOOK, OriginIGO::TelemetryRenderer_OPENGL, "Failed to reverse processed '%s' (%d)", apiName, err);

    return err == 0;
}

// Returns whether the Steam overlay is running
bool IsSteamRunning()
{
    // Not iterating through the list of image names since it's not thread-safe...
    // But dladdr is thread-safe, so let's see if the root method to flush the rendering has been "replaced"
    void* fcn = dlsym(RTLD_DEFAULT, "CGLFlushDrawable");
    if (fcn)
    {
        Dl_info info;
        if (dladdr(fcn, &info))
        {
            if (info.dli_fname)
                return strstr(info.dli_fname, "/gameoverlayrenderer.dylib") != NULL;
        }
    }
    
    return false;
}

} // OriginIGO

#endif // MAC OSX
