///////////////////////////////////////////////////////////////////////////////
// InjectHook.cpp
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include "InjectHook.h"

#if defined(ORIGIN_PC)

#include <windows.h>
#include <stdio.h>
#include <Psapi.h>

#include "IGOTrace.h"
#include "HookAPI.h"
#include "IGOLogger.h"
#include "madCHook.h"
#include "IGOSharedStructs.h"
#include "IGOTelemetry.h"

extern "C" {
    LPVOID  WINAPI GetImageProcAddress(HMODULE hModule, LPCSTR name, BOOL doubleCheck = TRUE);
    HMODULE WINAPI GetRemoteModuleHandle32(HANDLE ProcessHandle, LPCWSTR DllName, LPWSTR DllFullPath);
    HMODULE WINAPI GetRemoteModuleHandle64(HANDLE ProcessHandle, LPCWSTR DllName, LPWSTR DllFullPath);
    BOOL WINAPI GetImageProcAddressesRaw(LPCWSTR ModuleFileName, PVOID ImageBaseAddress, LPCSTR *ApiNames, PVOID *ProcAddresses, int ApiCount);
    void WINAPI EnableAllPrivileges(void);
}


#pragma optimize( "", off )

namespace OriginIGO {
    
    static bool gShell32Loaded = false;
    static bool gAdvapi32Loaded = false;
    static bool gKernel32Loaded = false;


    extern bool gCoreEnabledIGO;
    extern IGOConfigurationFlags &getIGOConfigurationFlags();
    extern bool gBypassInject;
    extern HANDLE IGOInjectThreadHandle;
    extern HINSTANCE gInstDLL;

    extern void IGOFinalCleanup();
}

using namespace OriginIGO;

// This class is used to "wrap" the current environment and add any OIG specific setting if necessary
class OIGEnvironmentWrapper
{
public:
    OIGEnvironmentWrapper(LPVOID lpEnvironment, DWORD dwCreationFlags)
        : mNewEnv(NULL), mOriginalEnv(lpEnvironment)
    {
        if (lpEnvironment)
        {
            // Are we dealing with UNICODE or ANSI characters?
            if (dwCreationFlags & CREATE_UNICODE_ENVIRONMENT)
                CreateUnicodeEnvironment();

            else
                CreateAnsiEnvironment();
        }
    }

    ~OIGEnvironmentWrapper()
    {
        if (mNewEnv)
            delete []mNewEnv;
    }

    LPVOID operator() () const { return mNewEnv ? mNewEnv : mOriginalEnv; };

private:
    void CreateUnicodeEnvironment()
    {
        // Variable strings are separated by NULL character, and the block is terminated by yet another NULL character
        // Check if we already have the OIG settings in there + find where we need to insert our settings
        // (from http://msdn.microsoft.com/en-us/library/windows/desktop/ms682009(v=vs.85).aspx, looks like we need to sort
        // the entries)
        int insertIndex = 0;
        bool insertCheckDone = false;   // we don't want to stop when we know what to do because we also use the loop to compute the
                                        // actual env size

#ifdef _DEBUG
        OutputDebugString(L"ORIGINAL ENV:\n");
#endif
        wchar_t* settings = static_cast<wchar_t*>(mOriginalEnv);
        wchar_t* setting = settings;
        size_t originalEnvLen = 0;
        size_t keywordLen = lstrlen(OIG_TELEMETRY_PRODUCTID_ENV_NAME);
        while (*setting)
        {
#ifdef _DEBUG
            OutputDebugStringW(setting);
            OutputDebugStringW(L"\n");
#endif
            size_t len = lstrlen(setting) + 1; // preparing for next entry

            if (!insertCheckDone)
            {
                int cmpResult = _wcsnicmp(OIG_TELEMETRY_PRODUCTID_ENV_NAME, setting, keywordLen);
                if (cmpResult == 0)
                {
                    insertIndex = -1;
                    insertCheckDone = true; // Looks like we don't need to do anything
                }

                else
                if (cmpResult > 0)
                    insertIndex += static_cast<int>(len);
            }

            setting += len;
            originalEnvLen += len;
        }

        ++originalEnvLen; // final block NULL character

        if (insertIndex >= 0)
        {
            // Ok... we need to insert our stuff!
            wchar_t productId[128];
            wchar_t timestamp[64];
            int productIdLen = _snwprintf(productId, _countof(productId), L"%s=%s", OIG_TELEMETRY_PRODUCTID_ENV_NAME, TelemetryDispatcher::instance()->GetProductId());
            int timestampLen = _snwprintf(timestamp, _countof(timestamp), L"%s=%ld", OIG_TELEMETRY_TIMESTAMP_ENV_NAME, TelemetryDispatcher::instance()->GetTimestamp());

            if (productIdLen > 0 && productIdLen < _countof(productId) && timestampLen > 0 && timestampLen < _countof(timestamp))
            {
                size_t newEnvLen = originalEnvLen + productIdLen + 1 + timestampLen + 1;

                mNewEnv = new char[newEnvLen * sizeof(wchar_t)]; // cheating here!
                wchar_t* newEnv = reinterpret_cast<wchar_t*>(mNewEnv);

                size_t offset = 0;
                wmemcpy(&newEnv[offset], &settings[0], insertIndex);

                offset += insertIndex;
                wmemcpy(&newEnv[offset], productId, productIdLen + 1);

                offset += productIdLen + 1;
                wmemcpy(&newEnv[offset], timestamp, timestampLen + 1);

                offset += timestampLen + 1;
                wmemcpy(&newEnv[offset], &settings[insertIndex], originalEnvLen - insertIndex);

#ifdef _DEBUG
                OutputDebugString(L"NEW ENV:\n");

                setting = newEnv;
                while (*setting)
                {
                    OutputDebugStringW(setting);
                    OutputDebugStringW(L"\n");
                    setting += lstrlen(setting) + 1;
                }
#endif
            }
        }
    }

    void CreateAnsiEnvironment()
    {
        // Variable strings are separated by NULL character, and the block is terminated by yet another NULL character
        // Check if we already have the OIG settings in there + find where we need to insert our settings
        // (from http://msdn.microsoft.com/en-us/library/windows/desktop/ms682009(v=vs.85).aspx, looks like we need to sort
        // the entries)
        int insertIndex = 0;
        bool insertCheckDone = false;   // we don't want to stop when we know what to do because we also use the loop to compute the
                                        // actual env size

#ifdef _DEBUG
        OutputDebugString(L"ORIGINAL ENV:\n");
#endif
        const char* settings = static_cast<const char*>(mOriginalEnv);
        const char* setting = settings;
        size_t originalEnvLen = 0;
        size_t keywordLen = strlen(OIG_TELEMETRY_PRODUCTID_ENV_NAME_A);
        while (*setting)
        {
#ifdef _DEBUG
            OutputDebugStringA(setting);
            OutputDebugStringA("\n");
#endif
            size_t len = strlen(setting) + 1; // preparing for next entry

            if (!insertCheckDone)
            {
                int cmpResult = _strnicmp(OIG_TELEMETRY_PRODUCTID_ENV_NAME_A, setting, keywordLen);
                if (cmpResult == 0)
                {
                    insertIndex = -1;
                    insertCheckDone = true; // Looks like we don't need to do anything
                }

                else
                if (cmpResult > 0)
                    insertIndex += static_cast<int>(len);
            }

            setting += len;
            originalEnvLen += len;
        }

        ++originalEnvLen; // final block NULL character

        if (insertIndex >= 0)
        {
            // Ok... we need to insert our stuff!
            char productIdUtf8[64];
            if (WideCharToMultiByte(CP_UTF8, 0, TelemetryDispatcher::instance()->GetProductId(), -1, productIdUtf8, sizeof(productIdUtf8), NULL, NULL) > 0)
            {
                char productId[128];
                char timestamp[64];

                int productIdLen = _snprintf(productId, _countof(productId), "%s=%s", OIG_TELEMETRY_PRODUCTID_ENV_NAME_A, productIdUtf8);
                int timestampLen = _snprintf(timestamp, _countof(timestamp), "%s=%lld", OIG_TELEMETRY_TIMESTAMP_ENV_NAME_A, TelemetryDispatcher::instance()->GetTimestamp());

                if (productIdLen > 0 && productIdLen < _countof(productId) && timestampLen > 0 && timestampLen < _countof(timestamp))
                {
                    size_t newEnvLen = originalEnvLen + productIdLen + 1 + timestampLen + 1;
                    mNewEnv = new char[newEnvLen];

                    size_t offset = 0;
                    memcpy(&mNewEnv[offset], &settings[0], insertIndex);

                    offset += insertIndex;
                    memcpy(&mNewEnv[offset], productId, productIdLen + 1);

                    offset += productIdLen + 1;
                    memcpy(&mNewEnv[offset], timestamp, timestampLen + 1);

                    offset += timestampLen + 1;
                    memcpy(&mNewEnv[offset], &settings[insertIndex], originalEnvLen - insertIndex);

#ifdef _DEBUG
                    OutputDebugString(L"NEW ENV:\n");

                    setting = mNewEnv;
                    while (*setting)
                    {
                        OutputDebugStringA(setting);
                        OutputDebugStringA("\n");
                        setting += strlen(setting) + 1;
                    }
#endif
                }
            }
        }
    }

    char* mNewEnv;
    LPVOID mOriginalEnv;
};

DEFINE_HOOK_SAFE(HANDLE, CreateFileWHook, (LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile))

    return CreateFileWHookNext(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

DEFINE_HOOK_SAFE(BOOL, CreateProcessWithLogonWHook, (LPCWSTR lpUsername,
                                                    LPCWSTR lpDomain,
                                                    LPCWSTR lpPassword,
                                                    DWORD dwLogonFlags,
                                                    LPCWSTR lpApplicationName,
                                                    LPWSTR lpCommandLine,
                                                    DWORD dwCreationFlags,
                                                    LPVOID lpEnvironment,
                                                    LPCWSTR lpCurrentDirectory,
                                                    LPSTARTUPINFOW lpStartupInfo,
                                                    LPPROCESS_INFORMATION lpProcessInformation))

    PROCESS_INFORMATION pinfo = {0};
    LPPROCESS_INFORMATION lpPI = lpProcessInformation;
    if (!lpPI)
        lpPI = &pinfo;

    IGOLogInfo("InjectHook::CreateProcessWithLogonWHook %S %S", lpApplicationName, lpCommandLine);

    OIGEnvironmentWrapper envWrapper(lpEnvironment, dwCreationFlags);
    BOOL retval = CreateProcessWithLogonWHookNext(lpUsername,
                                                    lpDomain,
                                                    lpPassword,
                                                    dwLogonFlags,
                                                    lpApplicationName,
                                                    lpCommandLine,
                                                    dwCreationFlags | CREATE_SUSPENDED,
                                                    envWrapper(),
                                                    lpCurrentDirectory,
                                                    lpStartupInfo,
                                                    lpPI);

    if (retval && lpPI->hProcess)
    {
        InjectCurrentDLL(lpPI->hProcess, lpPI->hThread);

        // Resume main process thread IF NOT created suspended in original call
        if (!(dwCreationFlags & CREATE_SUSPENDED))
            ResumeThread(lpPI->hThread);

        if (!lpProcessInformation)
        {
            CloseHandle(pinfo.hProcess);
            CloseHandle(pinfo.hThread);
            pinfo.hProcess = NULL;
            pinfo.hThread = NULL;
        }
    }

    return retval;
}

DEFINE_HOOK_SAFE(BOOL, CreateProcessWithTokenWHook, (HANDLE hToken,
                                                    DWORD dwLogonFlags,
                                                    LPCWSTR lpApplicationName,
                                                    LPWSTR lpCommandLine,
                                                    DWORD dwCreationFlags,
                                                    LPVOID lpEnvironment,
                                                    LPCWSTR lpCurrentDirectory,
                                                    LPSTARTUPINFOW lpStartupInfo,
                                                    LPPROCESS_INFORMATION lpProcessInformation))

    PROCESS_INFORMATION pinfo = {0};
    LPPROCESS_INFORMATION lpPI = lpProcessInformation;
    if (!lpPI)
        lpPI = &pinfo;

    IGOLogInfo("InjectHook::CreateProcessWithTokenWHook %S %S, flags=0x08%x", lpApplicationName, lpCommandLine, dwCreationFlags);

    OIGEnvironmentWrapper envWrapper(lpEnvironment, dwCreationFlags);
    BOOL retval = CreateProcessWithTokenWHookNext(hToken,
                                                    dwLogonFlags,
                                                    lpApplicationName,
                                                    lpCommandLine,
                                                    dwCreationFlags | CREATE_SUSPENDED,
                                                    envWrapper(),
                                                    lpCurrentDirectory,
                                                    lpStartupInfo,
                                                    lpPI);

    if (retval && lpPI->hProcess)
    {
        InjectCurrentDLL(lpPI->hProcess, lpPI->hThread);

        // Resume main process thread IF NOT created suspended in original call
        if (!(dwCreationFlags & CREATE_SUSPENDED))
            ResumeThread(lpPI->hThread);

        if (!lpProcessInformation)
        {
            CloseHandle(pinfo.hProcess);
            CloseHandle(pinfo.hThread);
            pinfo.hProcess = NULL;
            pinfo.hThread = NULL;
        }
    }

    return retval;
}

DEFINE_HOOK_SAFE(BOOL, CreateProcessAHook, (LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation))

    PROCESS_INFORMATION pinfo = {0};
    LPPROCESS_INFORMATION lpPI = lpProcessInformation;
    if (!lpPI)
        lpPI = &pinfo;

    IGOLogInfo("InjectHook::CreateProcessAHook %s %s, flags=0x08%x", lpApplicationName, lpCommandLine, dwCreationFlags);

    OIGEnvironmentWrapper envWrapper(lpEnvironment, dwCreationFlags);
    BOOL retval = CreateProcessAHookNext(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags | CREATE_SUSPENDED, envWrapper(), lpCurrentDirectory, lpStartupInfo, lpPI);

    if (retval && lpPI->hProcess)
    {
        InjectCurrentDLL(lpPI->hProcess, lpPI->hThread);

        // Resume main process thread IF NOT created suspended in original call
        if (!(dwCreationFlags & CREATE_SUSPENDED))
            ResumeThread(lpPI->hThread);

        if (!lpProcessInformation)
        {
            CloseHandle(pinfo.hProcess);
            CloseHandle(pinfo.hThread);
            pinfo.hProcess = NULL;
            pinfo.hThread = NULL;
        }
    }

    return retval;
}

DEFINE_HOOK_SAFE(BOOL, CreateProcessWHook, (LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation))

    PROCESS_INFORMATION pinfo = {0};
    LPPROCESS_INFORMATION lpPI = lpProcessInformation;
    if (!lpPI)
        lpPI = &pinfo;

    IGOLogInfo("InjectHook::CreateProcessWHook %S %S, flags=0x08%x", lpApplicationName, lpCommandLine, dwCreationFlags);

    OIGEnvironmentWrapper envWrapper(lpEnvironment, dwCreationFlags);
    BOOL retval = CreateProcessWHookNext(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags | CREATE_SUSPENDED, envWrapper(), lpCurrentDirectory, lpStartupInfo, lpPI);

    if (retval && lpPI->hProcess)
    {
        InjectCurrentDLL(lpPI->hProcess, lpPI->hThread);

        // Resume main process thread IF NOT created suspended in original call
        if (!(dwCreationFlags & CREATE_SUSPENDED))
            ResumeThread(lpPI->hThread);

        if (!lpProcessInformation)
        {
            CloseHandle(pinfo.hProcess);
            CloseHandle(pinfo.hThread);
            pinfo.hProcess = NULL;
            pinfo.hThread = NULL;
        }
    }

    return retval;
}

DEFINE_HOOK_SAFE(BOOL, CreateProcessAsUserAHook, (HANDLE hToken, LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation))

    PROCESS_INFORMATION pinfo = {0};
    LPPROCESS_INFORMATION lpPI = lpProcessInformation;
    if (!lpPI)
        lpPI = &pinfo;

    IGOLogInfo("InjectHook::CreateProcessAsUserAHook %s %s, flags=0x08%x", lpApplicationName, lpCommandLine, dwCreationFlags);

    OIGEnvironmentWrapper envWrapper(lpEnvironment, dwCreationFlags);
    BOOL result = CreateProcessAsUserAHookNext(hToken, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags | CREATE_SUSPENDED, envWrapper(), lpCurrentDirectory, lpStartupInfo, lpPI);
    
    if (result && lpPI->hProcess)
    {
        InjectCurrentDLL(lpPI->hProcess, lpPI->hThread);

        // Resume main process thread IF NOT created suspended in original call
        if (!(dwCreationFlags & CREATE_SUSPENDED))
            ResumeThread(lpPI->hThread);

        if (!lpProcessInformation)
        {
            CloseHandle(pinfo.hProcess);
            CloseHandle(pinfo.hThread);
            pinfo.hProcess = NULL;
            pinfo.hThread = NULL;
        }
    }
    return result;
}

DEFINE_HOOK_SAFE(BOOL, CreateProcessAsUserWHook, (HANDLE hToken, LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCTSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation))

    PROCESS_INFORMATION pinfo = {0};
    LPPROCESS_INFORMATION lpPI = lpProcessInformation;
    if (!lpPI)
        lpPI = &pinfo;

    IGOLogInfo("InjectHook::CreateProcessAsUserWHook %S %S, flags=0x08%x", lpApplicationName, lpCommandLine, dwCreationFlags);

    OIGEnvironmentWrapper envWrapper(lpEnvironment, dwCreationFlags);
    BOOL result = CreateProcessAsUserWHookNext(hToken, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags | CREATE_SUSPENDED, envWrapper(), lpCurrentDirectory, lpStartupInfo, lpPI);
    
    if (result && lpPI->hProcess)
    {
        InjectCurrentDLL(lpPI->hProcess, lpPI->hThread);

        // Resume main process thread IF NOT created suspended in original call
        if (!(dwCreationFlags & CREATE_SUSPENDED))
            ResumeThread(lpPI->hThread);

        if (!lpProcessInformation)
        {
            CloseHandle(pinfo.hProcess);
            CloseHandle(pinfo.hThread);
            pinfo.hProcess = NULL;
            pinfo.hThread = NULL;
        }
    }
    return result;
}

DEFINE_HOOK_SAFE(BOOL, ShellExecuteExAHook, (SHELLEXECUTEINFOA *pExecInfo))

    pExecInfo->fMask |= SEE_MASK_NOCLOSEPROCESS;
	LPCSTR verb = pExecInfo->lpVerb ? pExecInfo->lpVerb : "";
    IGOLogInfo("InjectHook::ShellExecuteExWHook %s - fMask=0x08%x - verb=%s", pExecInfo->lpFile, pExecInfo->fMask, verb);
    BOOL success = ShellExecuteExAHookNext(pExecInfo);
	IGOLogInfo("Success = %d (%d)", success, GetLastError());
	if (pExecInfo->hProcess)
    {
        InjectCurrentDLL(pExecInfo->hProcess);
    }
    return success;
}

DEFINE_HOOK_SAFE(BOOL, ShellExecuteExWHook, (SHELLEXECUTEINFOW *pExecInfo))

    pExecInfo->fMask |= SEE_MASK_NOCLOSEPROCESS;
	LPCWSTR verb = pExecInfo->lpVerb ? pExecInfo->lpVerb : L"";
    IGOLogInfo("InjectHook::ShellExecuteExWHook %S - fMask=0x08%x - verb=%S", pExecInfo->lpFile, pExecInfo->fMask, verb);
    BOOL success = ShellExecuteExWHookNext(pExecInfo);
	IGOLogInfo("Success = %d (%d)", success, GetLastError());
    if (pExecInfo->hProcess)
    {
        InjectCurrentDLL(pExecInfo->hProcess);
    }
    return success;
}

DEFINE_HOOK_SAFE(HINSTANCE, ShellExecuteAHook, (HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd))

    SHELLEXECUTEINFOA se = {0};
    se.cbSize = sizeof(se);
    se.fMask = SEE_MASK_NOCLOSEPROCESS;
    se.hwnd = hwnd ? hwnd : NULL;
    se.lpVerb = lpOperation ? lpOperation : "";;
    se.lpFile = lpFile ? lpFile : "";
    se.lpParameters = lpParameters ? lpParameters : "";
    se.lpDirectory = lpDirectory ? lpDirectory : "";
    se.nShow = nShowCmd;
    IGOLogInfo("InjectHook::ShellExecuteAHook %s - verb=%s", se.lpFile, se.lpVerb);

    if (ShellExecuteExAHookNext && isShellExecuteExAHooked)
    {
        BOOL success = ShellExecuteExAHookNext(&se);
        if (se.hProcess)
        {
            InjectCurrentDLL(se.hProcess);
        }

        // If the function success, it returns a value greater than 32. If the function fails, it returns an error value that
        // indicates the cause of the failure. Because the original error code list was limited to 32, any new error should return
        // SE_ERR_ACCESSDENIED (you can look at the Old New Thing for more info in this)
        const int MAGIC_SUCCESS_RETURN_VALUE = 33;
        int retVal = MAGIC_SUCCESS_RETURN_VALUE;
        if (!success)
        {
            retVal = GetLastError();
            if (retVal >= MAGIC_SUCCESS_RETURN_VALUE)
                retVal = SE_ERR_ACCESSDENIED;
        }

        return (HINSTANCE)retVal;
    }
    else
    {
        return ShellExecuteAHookNext(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
    }
}

DEFINE_HOOK_SAFE(HINSTANCE, ShellExecuteWHook, (HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd))

    SHELLEXECUTEINFOW se = {0};
    se.cbSize = sizeof(se);
    se.fMask = SEE_MASK_NOCLOSEPROCESS;
    se.hwnd = hwnd ? hwnd : NULL;
    se.lpVerb = lpOperation ? lpOperation : L"";;
    se.lpFile = lpFile ? lpFile : L"";
    se.lpParameters = lpParameters ? lpParameters : L"";
    se.lpDirectory = lpDirectory ? lpDirectory : L"";
    se.nShow = nShowCmd;
    IGOLogInfo("InjectHook::ShellExecuteWHook %S - verb=%S", se.lpFile, se.lpVerb);

    if (ShellExecuteExWHookNext && isShellExecuteExWHooked)
    {
        BOOL success = ShellExecuteExWHookNext(&se);
        if (se.hProcess)
        {
            InjectCurrentDLL(se.hProcess);
        }

        // If the function success, it returns a value greater than 32. If the function fails, it returns an error value that
        // indicates the cause of the failure. Because the original error code list was limited to 32, any new error should return
        // SE_ERR_ACCESSDENIED (you can look at the Old New Thing for more info in this)
        const int MAGIC_SUCCESS_RETURN_VALUE = 33;
        int retVal = MAGIC_SUCCESS_RETURN_VALUE;
        if (!success)
        {
            retVal = GetLastError();
            if (retVal >= MAGIC_SUCCESS_RETURN_VALUE)
                retVal = SE_ERR_ACCESSDENIED;
        }

        return (HINSTANCE)retVal;
    }
    else
    {
        return ShellExecuteWHookNext(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
    }
}

// Hook ExitProcess to have a chance to cleanup running DLL threads before we get detached
DEFINE_HOOK_SAFE_VOID(ExitProcessHook, (UINT uExitCode))

    IGOFinalCleanup();
    ExitProcessHookNext(uExitCode);
}

// declaration - dllmain.cpp
bool IsKernel32ModuleLoaded(DWORD pid, int* stopEarlyErrorCode);
bool IsModuleAlreadyLoaded(DWORD pid, HMODULE hInstDLL, WCHAR* moduleFileName, BYTE** modBaseAddr, TCHAR* exePath);

// declaration - processtools.cpp
bool IsProcessDotNet(HANDLE hProcess);


namespace OriginIGO {

    // Use a separate thread to allow the remote process to complete the loading of Kernel32.dll (and other DLLs too)
    bool ForceKernel32DllLoadingCompletion(HANDLE hProcess, bool is64bit)
    {
        unsigned char EmptyThreadOpcodes32[] = 
        {
            0xC2, 0x04, 0x00    // ret 4  (__stdcall => responsible for cleaning up stack and we have 1 param...)
        };

        unsigned char EmptyThreadOpcodes64[] = 
        {
            0xC3                // ret  
        };

        SIZE_T opcodeCount = is64bit ? _countof(EmptyThreadOpcodes64) : _countof(EmptyThreadOpcodes32);
        LPVOID remoteThreadProc = (LPVOID)VirtualAllocEx(hProcess, NULL, opcodeCount, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        if (!remoteThreadProc)
        {
            DWORD error = GetLastError();
            IGOLogWarn("Failed to allocate memory for the kernel32 completion remote thread proc (%d)", error);
            return false;
        }

        SIZE_T bytesWritten = 0;
        if (!WriteProcessMemory(hProcess, (LPVOID)remoteThreadProc, is64bit ? EmptyThreadOpcodes64 : EmptyThreadOpcodes32, opcodeCount, &bytesWritten) || bytesWritten != opcodeCount)
        {
            DWORD error = GetLastError();
            IGOLogWarn("Unable to copy over kernel32 completion remote thread proc (%d)", error);
            return false;
        }

        if (!FlushInstructionCache(hProcess, remoteThreadProc, bytesWritten))
        {
            DWORD error = GetLastError();
            IGOLogWarn("Unable to flush kernel32 completion remote thread proc (%d)", error);
            return false;
        }

        HANDLE remoteThreadHandle = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)remoteThreadProc, (LPVOID)NULL, NULL, NULL);
        if (!remoteThreadHandle)
        {
            DWORD error = GetLastError();
            IGOLogWarn("Failed to start kernel32 completion remote thread (%d)", error);
            return false;
        }

        DWORD result = ::WaitForSingleObject(remoteThreadHandle, 2000);
        if (result != WAIT_OBJECT_0)
            IGOLogWarn("Timing out on kernel32 completion remote thread");

        CloseHandle(remoteThreadHandle);
        return result == WAIT_OBJECT_0;
    }

    static bool IsProcessReadyForInjection(HANDLE hProcess, HANDLE hThread, bool is64bit)
    {       
		int counter = 0;
		bool foundDll = false;
        bool forceKernel32Injection = false;
		int stopEarlyErrorCode = ERROR_SUCCESS;
        bool isVista = false;
          
        OSVERSIONINFO verInfo;
        ZeroMemory(&verInfo, sizeof(OSVERSIONINFO));
        verInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&verInfo);
        isVista = (verInfo.dwMajorVersion == 6) && (verInfo.dwMinorVersion == 0);

        while(counter<5000 && !foundDll)
        {
            foundDll = IsKernel32ModuleLoaded(GetProcessId(hProcess), &stopEarlyErrorCode);
            if (stopEarlyErrorCode != ERROR_SUCCESS)
                break;

            if (!foundDll)  // kernel32.dll not found, so try to resume the process (if it is suspended, othwerwise we have no handle) to give it some loading time
            {               
                // Windows Vista only (Fifa 15)
                if (isVista)
                {
                    bool netProcess = IsProcessDotNet(hProcess);
                    OriginIGO::IGOLogWarn("IsProcessDotNet: %i", netProcess);

                    if (netProcess)   // we detected mscoree.dll -> .Net executbale, so we do not use our forcing completion code!
                        forceKernel32Injection = true;
                }

                if (!forceKernel32Injection)
                {
                    IGOLogInfo("Forcing the completion of the kernel32 dll loading...");
                    forceKernel32Injection = true;
                    if (ForceKernel32DllLoadingCompletion(hProcess, is64bit))
                    {
                        foundDll = IsKernel32ModuleLoaded(GetProcessId(hProcess), &stopEarlyErrorCode);
                        if (foundDll)
                            IGOLogInfo("Forced completion successful");

                        if (foundDll || stopEarlyErrorCode != ERROR_SUCCESS)
                            break;
                    }
                }
                
                if (hThread != NULL)
                    ResumeThread(hThread);
            
                counter++;  // do this for max. 5 seconds, then the basic process should be up and running, even on slow HDD's with heavy "traffic"!

                
                Sleep(1); // use a short timeout in case we are dealing with a small launcher (think BJ3) that would immediately trigger a CreateProcess that which could miss
                          // Sleep(10) was too long for Mantle, so I switched to Sleep(1) which fixed the problem for DAI debug builds
                if (hThread != NULL)
                    SuspendThread(hThread);
            }
        }
        
        if (!foundDll)
        {
			if (stopEarlyErrorCode)
			{
				IGOLogWarn("Stopped waiting for Kernel32 (%d)", stopEarlyErrorCode);
				TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_GENERIC, TelemetryRenderer_UNKNOWN, "Stopped waiting for kernel32.dll (%d)", stopEarlyErrorCode);
			}

			else
			{
				IGOLogWarn("Kernel32 library not loaded.");
				TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_GENERIC, TelemetryRenderer_UNKNOWN, "No kernel32.dll found in target process");
			}

            return false;
        }
        IGOLogInfo("Completion successful");
        return true;
    }

    //

    static bool InjectProcess(HANDLE hProcess, HANDLE hThread, const WCHAR *dllName, bool is64bit)
    {
        if (!hProcess)
            return false;

        bool ret = false;

#ifdef _WIN64    
        if (is64bit)    // run this code on from igo64.dll for x64 processes only
        {
            if (!IsProcessReadyForInjection(hProcess, hThread, is64bit))
                return false;
        }
#else
        if (!is64bit)   // run this code on from igo32.dll for x86 processes only
        {
            if (!IsProcessReadyForInjection(hProcess, hThread, is64bit))
                return false;
        }
#endif

        // do not use InjectLibrary from madshi here - it is broken on 32-bit OS we use the VS2013 runtime

        {
#ifndef _WIN64    // on a 64-bit system, we cannot inject a 64-bit DLL from a 32-bit process, so we use a proxy process
            if (is64bit)
            {
                WCHAR *commandLine = new WCHAR[32767];
                ZeroMemory(commandLine, sizeof(WCHAR) * 32767);

                PROCESS_INFORMATION procInfo = { 0 };
                STARTUPINFOW startupInfo = { 0 };
                startupInfo.cb = sizeof(startupInfo);

                wcscpy(commandLine, dllName);
                WCHAR *found = wcsstr(commandLine, L"\\igo64");    // command line is X:\...\...\igo64.dll ...
				if (!found)
					found = wcsstr(commandLine, L"/igo64");

                if (found)
                {
					found += 4;			// skip \igo part
                    *found = '\0';		// null terminate our string here!
                    DWORD processId = ProcessHandleToId(hProcess);
                    DWORD threadId = hThread != NULL ? ThreadHandleToId(hThread) : 0;

                    if (processId)
                    {
#ifdef _DEBUG
                        WCHAR proxyProcess[64] = L"proxy64d.exe ";    // igoproxy64d.exe XXX (PID) (TID)
#else
                        WCHAR proxyProcess[64] = L"proxy64.exe ";    // igoproxy64.exe XXX (PID) (TID)
#endif

                        errno_t error = wcscat_s(commandLine, 32767, proxyProcess);
                        if (!error)
                        {
                            WCHAR processIdString[64] = { 0 };
                            error = _ui64tow_s(processId, &processIdString[0], _countof(processIdString), 10);
                            if (!error)
                            {
                                WCHAR threadIdString[64] = { 0 };
                                error = _ui64tow_s(threadId, &threadIdString[0], _countof(threadIdString), 10);
                                if (!error)
                                {
                                    // build the final command line
                                    error |= wcscat_s(commandLine, 32767, processIdString);
                                    error |= wcscat_s(commandLine, 32767, L" ");
                                    error |= wcscat_s(commandLine, 32767, threadIdString);

                                    if (!error)
                                    {
                                        // execute the 64-bit igoproxy64.exe
#ifdef _DEBUG
                                        DWORD dwCreationFlags = 0; // CREATE_SUSPENDED;
#else
                                        DWORD dwCreationFlags = 0;
#endif
                                        BOOL result = (CreateProcessWHookNext != NULL) ? ::CreateProcessWHookNext(NULL, commandLine, NULL, NULL, FALSE, dwCreationFlags, NULL, NULL, &startupInfo, &procInfo) : ::CreateProcessW(NULL, commandLine, NULL, NULL, FALSE, dwCreationFlags, NULL, NULL, &startupInfo, &procInfo);
                                        if (result == FALSE)
                                        {
                                            if (procInfo.hProcess)
                                                ::CloseHandle(procInfo.hProcess);

                                            if (procInfo.hThread)
                                                ::CloseHandle(procInfo.hThread);
                                        }
                                        else
                                        {
                                            WaitForSingleObject(procInfo.hProcess, INFINITE);
                                            DWORD exitCode = STILL_ACTIVE;
                                            bool isSuccess = 0 != ::GetExitCodeProcess(procInfo.hProcess, &exitCode);
                                            if (isSuccess && exitCode == 1 /*returned from igoproxy64*/)
                                            {
                                                ret = true;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                delete[] commandLine;
                commandLine = NULL;

                return ret;
            }
#endif

            // do not inject IGO, if we are launching a 32-bit process from a 64-bit process
#ifdef _WIN64
            if (is64bit)
#endif
            {

                // on x64 systems use our simple mechanism
                HANDLE remoteThreadHandle = NULL;
                WCHAR fileName[MAX_PATH + 1] = { 0 };
#ifdef _WIN64
                PVOID imageBaseAddress = GetRemoteModuleHandle64(hProcess, L"kernel32.dll", fileName);
#else
                PVOID imageBaseAddress = GetRemoteModuleHandle32(hProcess, L"kernel32.dll", fileName);
#endif
                if (imageBaseAddress != NULL)
                {
                    LPVOID LoadLibAddy[1] = { NULL };
                    LPSTR apiNames[1] = { "LoadLibraryW" };

                    if (GetImageProcAddressesRaw(fileName, imageBaseAddress, (LPCSTR*)apiNames, LoadLibAddy, 1))
                    {
                        if (LoadLibAddy[0] != NULL)
                        {
                            LPVOID localAddr = GetImageProcAddress(GetModuleHandleW(L"kernel32.dll"), apiNames[0], true);
                            if (localAddr != LoadLibAddy[0])
                                IGOLogWarn("Inject ASLR (x64 %i) detected.", is64bit); // ASLR seems not to be supported on 32-bit processes, but keep this info, just to be sure...

                            LPVOID RemoteString = (LPVOID)VirtualAllocEx(hProcess,
                                NULL,
                                (wcslen(dllName) + 1 + sizeof(HANDLE) + sizeof(DWORD))*sizeof(WCHAR),
                                MEM_RESERVE | MEM_COMMIT,
                                PAGE_READWRITE
                                );

                            WriteProcessMemory(hProcess, (LPVOID)RemoteString, dllName, (wcslen(dllName) + 1 + sizeof(HANDLE) + sizeof(DWORD))*sizeof(WCHAR), NULL);
                            ::SetLastError(0);
                            remoteThreadHandle = CreateRemoteThread(hProcess,
                                NULL,
                                NULL,
                                (LPTHREAD_START_ROUTINE)LoadLibAddy[0],
                                (LPVOID)RemoteString,
                                NULL,
                                NULL
                                );
                            ret = (remoteThreadHandle != NULL);

                            if (!ret)
                            {
                                DWORD dwError = ::GetLastError();
                                if (remoteThreadHandle == NULL && dwError)
                                {
                                    char* pError = NULL;
                                    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&pError, 0, NULL);
                                    IGOLogWarn("Inject IGO dll failed (first attempt): (%x) %s.", dwError, pError);
                                    LocalFree(pError);
                                }

                                // try CreateRemoteThread alternative
                                DWORD tId = 0;
                                HANDLE remoteThreadHandle = madCreateRemoteThread(hProcess,
                                    NULL,
                                    NULL,
                                    (LPTHREAD_START_ROUTINE)LoadLibAddy[0],
                                    (LPVOID)RemoteString,
                                    0,
                                    &tId
                                    );

                                ret = (remoteThreadHandle != NULL);
                                if (!ret)
                                {
                                    DWORD dwError = ::GetLastError();
                                    if (remoteThreadHandle == NULL && dwError)
                                    {
                                        char* pError = NULL;
                                        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&pError, 0, NULL);
                                        IGOLogWarn("Inject (second attempt) IGO dll failed: (%x) %s.", dwError, pError);
                                        LocalFree(pError);
                                    }
                                }
                                else
                                {
                                    // Wait for the thread to be done/setup our hooking threads (especially because we're trying to hook API creation methods, ie Direct3DCreate8/9
                                    // before they are called
                                    DWORD result = ::WaitForSingleObject(remoteThreadHandle, 2000);
                                    if (result != WAIT_OBJECT_0)
                                        IGOLogWarn("Continuing before injection thread done");

                                    CloseHandle(remoteThreadHandle);
                                }
                            }
                            else
                                CloseHandle(remoteThreadHandle);

                            if (IGOLogEnabled())
                            {
                                CHAR szExePath[MAX_PATH + 1] = { 0 };
                                HMODULE loadedPSAPI = NULL;
                                typedef DWORD(WINAPI *GetModuleFileNameExFunc)(HANDLE hProcess, LPSTR lpImageFileName, DWORD nSize);

                                // try PSAPI_VERSION 2
                                GetModuleFileNameExFunc pGetProcessImageFileNameA = (GetModuleFileNameExFunc)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "K32GetProcessImageFileNameA");
                                if (!pGetProcessImageFileNameA)
                                {
                                    // fallback to PSAPI_VERSION 1
                                    HMODULE retrievedPSAPI = GetModuleHandle(L"psapi.dll");
                                    if (!retrievedPSAPI)
                                    {
                                        loadedPSAPI = LoadLibrary(L"psapi.dll");
                                        if (loadedPSAPI)
                                            pGetProcessImageFileNameA = (GetModuleFileNameExFunc)GetProcAddress(loadedPSAPI, "GetProcessImageFileNameA");
                                    }
                                    else
                                        pGetProcessImageFileNameA = (GetModuleFileNameExFunc)GetProcAddress(retrievedPSAPI, "GetProcessImageFileNameA");
                                }

                                if (pGetProcessImageFileNameA)
                                    pGetProcessImageFileNameA(hProcess, szExePath, MAX_PATH);

                                if (loadedPSAPI)
                                    FreeLibrary(loadedPSAPI);

                                IGOLogInfo("Inject IGO dll to pid %d file %s", GetProcessId(hProcess), szExePath);
                            }
                        }
                        else
                        {
                            IGOLogWarn("Inject could not find function address.");
                        }
                    }
                    else
                    {
                        IGOLogWarn("Inject could not find function address.");
                    }
                }
                else
                {
                    IGOLogWarn("Inject could not find base address.");
                }
            }
        }
        return ret;
    } 
    

    bool InjectCurrentDLL(HANDLE hProcess, HANDLE hThread)
    {
		const size_t strDLLPathBufferSize = 32767;
		WCHAR *strDLLPath = new WCHAR[strDLLPathBufferSize];
		ZeroMemory(strDLLPath, sizeof(WCHAR) * strDLLPathBufferSize);

		::GetModuleFileName(gInstDLL, strDLLPath, strDLLPathBufferSize);
		_wcslwr_s(strDLLPath, strDLLPathBufferSize);

        bool is64bitProcess = Is64bitProcess(hProcess) == TRUE;
        if (is64bitProcess)
        {
            // strDllPath is igo32d.dll or igo32.dll, so we change it to igo64.dll or igo64d.dll
            WCHAR *found = wcsstr(strDLLPath, L"\\igo32");
			if (!found)
				found = wcsstr(strDLLPath, L"/igo32");

            if(found)
            {
				found += 4; // skip \igo part

                *found++='6';
                *found++='4';
            }
        }
        bool result = InjectProcess(hProcess, hThread, strDLLPath, is64bitProcess);
        if (!result)
        {
            Sleep(250); // give the process some time... otherwise injection may fail!!!
            result = InjectProcess(hProcess, hThread, strDLLPath, is64bitProcess);
            if (!result)
            {
                IGOLogWarn("InjectHook::InjectCurrentDLL failed");
            }
        }
        else
            Sleep(250); // give the process some time... otherwise injection may fail!!!
        
    
        delete[] strDLLPath;
        strDLLPath = NULL;

        return result;
    }
}


bool InjectHook::TryHook()
{
#ifndef _WIN64    
    if (!mIsQuitting && GetModuleHandle(L"kernel32.dll") && !gKernel32Loaded)
    {
        gKernel32Loaded = true;
        HOOKAPI_SAFE("kernel32.dll", "CreateProcessA", CreateProcessAHook);
        HOOKAPI_SAFE("kernel32.dll", "CreateProcessW", CreateProcessWHook);
        HOOKAPI_SAFE("kernel32.dll", "ExitProcess", ExitProcessHook);

        IGOLogInfo("InjectHook::TryHook %i %i %i", gAdvapi32Loaded, gShell32Loaded, gKernel32Loaded);
    }
#else
    // try kernelbase.dll on x64 OS's first (API moved starting with Win8)
    if (!mIsQuitting && !gKernel32Loaded)
    {
        HMODULE kernelBaseLoaded = GetModuleHandle(L"kernelBase.dll");
        HMODULE kernel32Loaded = GetModuleHandle(L"kernel32.dll");
        if (kernelBaseLoaded || kernel32Loaded)
        {
            gKernel32Loaded = true;
            if (kernelBaseLoaded)
            {
                HOOKAPI_SAFE("kernelBase.dll", "CreateProcessA", CreateProcessAHook);
                HOOKAPI_SAFE("kernelBase.dll", "CreateProcessW", CreateProcessWHook);
                HOOKAPI_SAFE("kernelBase.dll", "ExitProcess", ExitProcessHook);
            }

            if (kernel32Loaded)
            {
                HOOKAPI_SAFE("kernel32.dll", "CreateProcessA", CreateProcessAHook);
                HOOKAPI_SAFE("kernel32.dll", "CreateProcessW", CreateProcessWHook);
                HOOKAPI_SAFE("kernel32.dll", "ExitProcess", ExitProcessHook);
            }

            IGOLogInfo("InjectHook::TryHook %i %i %i", gAdvapi32Loaded, gShell32Loaded, gKernel32Loaded);
        }
    }
#endif

    if (!mIsQuitting && GetModuleHandle(L"shell32.dll") && !gShell32Loaded)
    {
        gShell32Loaded = true;
        HOOKAPI_SAFE("shell32.dll", "ShellExecuteA", ShellExecuteAHook);
        HOOKAPI_SAFE("shell32.dll", "ShellExecuteW", ShellExecuteWHook);
        HOOKAPI_SAFE("shell32.dll", "ShellExecuteExA", ShellExecuteExAHook);
        HOOKAPI_SAFE("shell32.dll", "ShellExecuteExW", ShellExecuteExWHook);
        IGOLogInfo("InjectHook::TryHook %i %i %i", gAdvapi32Loaded, gShell32Loaded, gKernel32Loaded);
    }

    if (!mIsQuitting && GetModuleHandle(L"advapi32.dll") && !gAdvapi32Loaded)
    {
        gAdvapi32Loaded = true;
        HOOKAPI_SAFE("advapi32.dll", "CreateProcessAsUserA", CreateProcessAsUserAHook);
        HOOKAPI_SAFE("advapi32.dll", "CreateProcessAsUserW", CreateProcessAsUserWHook);
        HOOKAPI_SAFE("advapi32.dll", "CreateProcessWithTokenW", CreateProcessWithTokenWHook);
        HOOKAPI_SAFE("advapi32.dll", "CreateProcessWithLogonW", CreateProcessWithLogonWHook);
        IGOLogInfo("InjectHook::TryHook %i %i %i", gAdvapi32Loaded, gShell32Loaded, gKernel32Loaded);
    }

    return (gAdvapi32Loaded && gShell32Loaded && gKernel32Loaded);
}

bool InjectHook::IsQuitting()
{
    return mIsQuitting;
}

void InjectHook::Cleanup(bool forcedUnload)
{
    mIsQuitting = forcedUnload;    // only set this flag, if igo32.dll was forced to unload!!!
                                // this should only happen through EbisuApp::restart(), to perform client updates
    
    // terminate our hooking thread, if we have one running
    if (IGOInjectThreadHandle)
    {
        WaitForSingleObject(IGOInjectThreadHandle, 100);
        TerminateThread(IGOInjectThreadHandle, 0);
        CloseHandle(IGOInjectThreadHandle);
        IGOInjectThreadHandle = 0;
    }
    
    if (gKernel32Loaded)
    {
        UNHOOK_SAFE(CreateProcessAHook);
        UNHOOK_SAFE(CreateProcessWHook);
        // UNHOOK_SAFE(ExitProcessHook); // do not unhook this call, we call InjectHook::Cleanup() from inside ExitProcessHook !!!
    }

    if (gShell32Loaded)
    {
        UNHOOK_SAFE(ShellExecuteAHook);
        UNHOOK_SAFE(ShellExecuteWHook);
        UNHOOK_SAFE(ShellExecuteExAHook);
        UNHOOK_SAFE(ShellExecuteExWHook);
    }

    if (gAdvapi32Loaded)
    {
        UNHOOK_SAFE(CreateProcessAsUserAHook);
        UNHOOK_SAFE(CreateProcessAsUserWHook);
        UNHOOK_SAFE(CreateProcessWithTokenWHook);
        UNHOOK_SAFE(CreateProcessWithLogonWHook);
    }

    gKernel32Loaded = false;
    gShell32Loaded = false;
    gAdvapi32Loaded = false;
}
#endif // ORIGIN_PC
