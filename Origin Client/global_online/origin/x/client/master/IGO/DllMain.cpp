///////////////////////////////////////////////////////////////////////////////
// DllMain.cpp
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifdef ORIGIN_PC
#pragma warning(disable:4555) // warning C4555: expression has no effect; expected expression with side-effect
#endif

#include "IGO.h"
#include "Helpers.h"
#include "IGOSharedStructs.h"
#include "IGOLogger.h"
#include "IGOApplication.h"
#include "IGOTelemetry.h"
#include <time.h>

#if 0
#if defined(ORIGIN_PC) && defined(_DEBUG)
#pragma comment(lib, "Dbghelp")
#include <DbgHelp.h>
#endif
#endif

namespace OriginIGO
{
GameLaunchInfo::GameLaunchInfo()
    : mIsValid(false)
{
    memset(&mData, 0, sizeof(mData));
}

void GameLaunchInfo::setIsValid(bool flag)
{
    mIsValid = flag;
}

bool GameLaunchInfo::isValid() const
{
    return mIsValid;
}

void GameLaunchInfo::setProductId(const char16_t* productId, size_t length)
{
    gliMemcpy(mData.productId, sizeof(mData.productId), productId, length);
}

void GameLaunchInfo::setAchievementSetId(const char16_t* achievementSetId, size_t length)
{
    gliMemcpy(mData.achievementSetId, sizeof(mData.achievementSetId), achievementSetId, length);
}

void GameLaunchInfo::setExecutablePath(const char16_t* executablePath, size_t length)
{
    gliMemcpy(mData.executablePath, sizeof(mData.executablePath), executablePath, length);
}

void GameLaunchInfo::setTitle(const char16_t* title, size_t length)
{
    gliMemcpy(mData.title, sizeof(mData.title), title, length);
}

void GameLaunchInfo::setMasterTitle(const char16_t* title, size_t length)
{
    gliMemcpy(mData.masterTitle, sizeof(mData.masterTitle), title, length);
}

void GameLaunchInfo::setMasterTitleOverride(const char16_t* title, size_t length)
{
    gliMemcpy(mData.masterTitleOverride, sizeof(mData.masterTitleOverride), title, length);
}

void GameLaunchInfo::setDefaultURL(const char16_t* url, size_t length)
{
    gliMemcpy(mData.defaultUrl, sizeof(mData.defaultUrl), url, length);
}

void GameLaunchInfo::setIsTrial(bool isTrial)
{
    mData.forceKillAtOwnershipExpiry = isTrial;
    mData.trialTerminated = false;
}

void GameLaunchInfo::setTrialTerminated(bool terminated)
{
    mData.trialTerminated = terminated;
}

void GameLaunchInfo::setTimeRemaining(int64_t timeRemaining)
{
    mData.trialTimeRemaining = timeRemaining;
}

void GameLaunchInfo::setBroadcastStats(bool isBroadcasting, uint64_t streamId, const char16_t* channel, uint32_t minViewers, uint32_t maxViewers)
{
    mData.broadcastingDataChanged = true;
    
    mData.isBroadcasting = isBroadcasting;
    mData.broadcastingStreamId = streamId;
    mData.minBroadcastingViewers = minViewers;
    mData.maxBroadcastingViewers = maxViewers;
    gliMemcpy(mData.broadcastingChannel, sizeof(mData.broadcastingChannel), channel, gliStrlen(channel));
}

void GameLaunchInfo::resetBroadcastStats()
{
    mData.broadcastingDataChanged = true;

    mData.broadcastingStreamId = 0;
    mData.isBroadcasting = false;
    mData.maxBroadcastingViewers = 0;
    mData.minBroadcastingViewers = 0;
    
    memset(mData.broadcastingChannel, 0, sizeof(mData.broadcastingChannel));
}

const char16_t* GameLaunchInfo::productId() const
{
    return mData.productId;
}

const char16_t* GameLaunchInfo::achievementSetId() const
{
    return mData.achievementSetId;
}

const char16_t* GameLaunchInfo::executablePath() const
{
    return mData.executablePath;
}

const char16_t* GameLaunchInfo::title() const
{
    return mData.title;
}

const char16_t* GameLaunchInfo::masterTitle() const
{
    return mData.masterTitle;
}

const char16_t* GameLaunchInfo::masterTitleOverride() const
{
    return mData.masterTitleOverride;
}

const char16_t* GameLaunchInfo::defaultURL() const
{
    return mData.defaultUrl;
}

bool GameLaunchInfo::isTrial() const
{
    return mData.forceKillAtOwnershipExpiry;
}
    
bool GameLaunchInfo::trialTerminated() const
{
    return mData.trialTerminated;
}

int64_t GameLaunchInfo::trialTimeRemaining() const
{
    return mData.trialTimeRemaining;
}
    
bool GameLaunchInfo::broadcastStatsChanged() const
{
    return mData.broadcastingDataChanged;
}

void GameLaunchInfo::broadcastStats(bool& isBroadcasting, uint64_t& streamId, const char16_t** channel, uint32_t& minViewers, uint32_t& maxViewers)
{
    mData.broadcastingDataChanged = false;

    isBroadcasting = mData.isBroadcasting;
    *channel = mData.broadcastingChannel;
    streamId = mData.broadcastingStreamId;
    minViewers = mData.minBroadcastingViewers;
    maxViewers = mData.maxBroadcastingViewers;
}

void GameLaunchInfo::gliMemcpy(char16_t* dest, size_t maxLen, const char16_t* src, size_t length)
{
    if (src == NULL || length == 0)
        return;
    
    if (length >= maxLen)
        length = maxLen - sizeof(char16_t);
    
    memcpy(dest, src, length);
    
    length /= sizeof(char16_t);
    dest[length] = 0;
}

size_t GameLaunchInfo::gliStrlen(const char16_t* src) const
{
    size_t length = 0;
    while (length < 1024 && src[length])
        ++length;
    
    return length * sizeof(char16_t);
}
    
} // namespace OriginIGO

static void ExternalLibLogInfo(const char* file, int line, int alwaysLogMessage, const char* fmt, va_list arglist)
{
    OriginIGO::IGOLogger::instance()->infoWithArgs(file, line, alwaysLogMessage ? true : false, fmt, arglist);
} 

static void ExternalLibLogError(const char* file, int line, const char* fmt, va_list arglist)
{
    OriginIGO::IGOLogger::instance()->warnWithArgs(file, line, true, fmt, arglist);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(ORIGIN_PC)

#include <windows.h>
#include <winternl.h>

// renderers
#ifndef _WIN64
#include "DX8Hook.h"
#endif
#include "MantleHook.h"
#include "DX9Hook.h"
#include "DX10Hook.h"
#include "DX11Hook.h"
#include "DX12Hook.h"
#include "DXGIHook.h"
#include "OGLHook.h"
#include "HookAPI.h"
#include "Mhook/mhook.h"
#include "InputHook.h"
#include "IGOTrace.h"
#include "InjectHook.h"
#include "madCHook.h"
#include "Disasm/disasm.h"
#include <Tlhelp32.h>
#include "IGOSharedStructs.h"
#include "twitchsdk.h"  // always include it before TwitchManager.h !!!
#include "TwitchManager.h"
#include "IGOInfoDisplay.h"
#include "eathread/eathread_futex.h"

extern bool IsProcessDotNet(HANDLE hProcess); // From madCodeHook3 ProcessTools.h

#define DLL_EXPORT __declspec(dllexport)

namespace OriginIGO {

    // create shared data segment
    // see IGOSharedStructs.h for more information
    // do not use simply types here, they will be re-initialized once the IGO dll is loaded into a different architecture(86/64) 
    const uint32_t IGOSharedMemorySize =  sizeof(IGOConfigurationFlags) + 1;
    unsigned char* pIGOSharedMemory = NULL;                // pointer to shared memory
    static HANDLE hMapObject = INVALID_HANDLE_VALUE;    // handle to file mapping

    // shared data accessor
    GameLaunchInfo* gameInfo()
    {
        static GameLaunchInfo info;
        return &info;
    }
    IGOConfigurationFlags &getIGOConfigurationFlags() { return *((IGOConfigurationFlags*)pIGOSharedMemory);}

    static HANDLE gGlobalIGOApplicationSharedDataMutex = NULL;
    static wchar_t *gGlobalIGOApplicationSharedDataMutexName = L"Global\\IGOApplicationSharedDataMutex";

    HANDLE gIGODestroyThreadHandle = NULL;
    HANDLE gIGOCreateThreadHandle = NULL;
    HANDLE gIGODestroyWaitThreadHandle = NULL;
    HANDLE gIGOReInitWaitThreadHandle = NULL;
    HANDLE gIGORendererTimeoutDetectThreadHandle = NULL;
    bool gSDKSignaled = false;  // a game was launched by the SDK
    bool g3rdPartyResetForced = false;
	bool gIGOAlreadyLoaded = false;	// This is to help preventing the loading IGO multiple times on a machine with multiple Origin installs - this really only applies
									// for older games, as we have now fixed the problem in the SDK.
	
    extern DWORD gIGOHookMantleUpdateMessage;
    extern DWORD gIGOHookDX8UpdateMessage;
    extern DWORD gIGOHookDX9UpdateMessage;
    extern DWORD gIGOHookDX10UpdateMessage;
    extern DWORD gIGOHookDX11UpdateMessage;
    extern DWORD gIGOHookGLUpdateMessage;
    extern DWORD gIGO3rdPartyCheckMessage;

    bool g3rdPartyDllDetected = false;
    HINSTANCE gInstDLL = NULL;
    HWND gHwnd = NULL;
    BYTE gIGOKeyboardData[256] = {0};
    bool gInputHooked = false;
    volatile bool gQuitting = false;
    volatile bool gQuitted = false;
    volatile bool gExitingProcess = false;

    DWORD gPresentHookThreadId = 0;
    volatile DWORD gPresentHookCalled = 0;
    volatile bool gTestCooperativeLevelHook_or_ResetHook_Called = false;
    bool gLastTestCooperativeLevelError = false;

    bool InjectHook::mIsQuitting = false;
    HANDLE IGOInjectThreadHandle = NULL;
    static bool g3rdPartyDLLLoaded = false;
    static bool gStarted = false;
    static bool gHookingInitialized = false;
    static EA::Thread::Futex gIGOOffsetCalculationMutex;
    static EA::Thread::Futex gIGOCreateDestroyMutex;
    static EA::Thread::Futex gIGOCreateDestroyWaitMutex;
    static EA::Thread::Futex gIGOQuittingFlagMutex;
    static EA::Thread::Futex gIGOQuittedFlagMutex;
    

    // TODO: Remove this code and modify checkhook instead to get the 3rd party dll name!
    // 3rd party dlls that might interfer with IGO
    // this list is only for informational purpose, we do not use it for detection anymore
    const wchar_t *g_3drPartyDLLs[] = {
        // fraps
        L"fraps64.dll",
        L"fraps32.dll",
        L"fraps.dll",    // old fraps 1.9
        // unknown app using Microsofts detour package
        L"detoured.dll",
        L"detoured64.dll",
        // rivatuner & rivatuner based apps (evga precisison, msi afterburner)
        L"d3doverriderhooks",
        L"evgaprecisionhooks",
        L"msiafterburnerhooks",
        L"rivatunerhooks",
        L"rtsshooks",
        // DXtory
        L"dxtorycore",
        L"dxtorymm",
        L"dxtoryhk",
        // punto switcher
        L"pshook",
        // amBX
        L"loadlibinterceptor",
        L"dx11interceptor",
        L"dx10interceptor",
        L"dx9interceptor",
        L"dx8interceptor",
        L"openglinterceptor",
        L"ambx_interfacescom",
        // mumble
        L"mumble_ol",
        // xfire
        L"xfire_toucan",
        // easyhook based apps like raptr
        L"easyhook",
        L"ltc_game",
        L"ltc_fpsi",
        L"ltc_help",
        L"ltc_pkts",
        // bdcam & OBS
        L"bdcam",
        L"bdcap",
        L"graphicscapture",
        // steam
        L"gameoverlayrenderer",
        // steam
        L"razorhook",
    };
}

bool GetParentInfo(DWORD pid, PROCESSENTRY32* pPe);
HWND getProcMainWindow(DWORD PID);
bool RedirectIGOModuleLoading(PDWORD pdwLdrErr, WCHAR* srcFullModuleName, PVOID pResultInstance, DWORD* result);

using namespace OriginIGO;

// forward declaration
namespace OriginIGO{
    void IGOCleanup();
    void IGOReInit();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
#if defined(ORIGIN_PC) && defined(_DEBUG)
void dumpStackTrace()
{
    void* stack[100];

    HANDLE process = GetCurrentProcess();
    SymInitialize(process, NULL, TRUE);
    USHORT frames = CaptureStackBackTrace(0, sizeof(stack), stack, NULL);
    SYMBOL_INFO* symbol = (SYMBOL_INFO *)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    IGOLogInfo("StackTrace (%d):", frames);
    for (int idx = 0; idx < frames; ++idx)
    {
        SymFromAddr(process, (DWORD64)(stack[idx]), 0, symbol);
        OriginIGO::IGOLogInfo("%i: %s - 0x%0X", frames - idx - 1, symbol->Name, symbol->Address);
    }

    free(symbol);
}
#endif
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Helper loggers for the MacCHook library
void MadCHookLogInfo(const char* file, int line, const wchar_t* msg)
{
    char buffer[256];
    size_t bufferSize = sizeof(buffer);
    size_t len = wcstombs(buffer, msg, bufferSize);
    buffer[bufferSize-1] = 0;

    if (len > 0)
        IGOLogger::instance()->info(file, line, false, buffer);
}

void MadCHookLogWarn(const char* file, int line, const wchar_t* msg)
{
    char buffer[256];
    size_t bufferSize = sizeof(buffer);
    size_t len = wcstombs(buffer, msg, bufferSize);
    buffer[bufferSize-1] = 0;

    if (len > 0)
        IGOLogger::instance()->warn(file, line, false, buffer);
}

void MadCHookLogDebug(const char* file, int line, const wchar_t* msg)
{
    char buffer[256];
    size_t bufferSize = sizeof(buffer);
    size_t len = wcstombs(buffer, msg, bufferSize);
    buffer[bufferSize-1] = 0;

    if (len > 0)
        IGOLogger::instance()->debug(file, line, buffer);
}

static DWORD WINAPI OffsetCalculationAndEarlyHookingThread(LPVOID lpParam)
{
    if (!gQuitting && !gExitingProcess)
    {
        EA::Thread::AutoFutex m(gIGOOffsetCalculationMutex);
        if (!gQuitting && !gExitingProcess)
        {
            OriginIGO::IGOLogInfo("Starting OffsetCalculationAndEarlyHookingThread...");

            if (!MantleHook::IsHooked())
                MantleHook::TryHook(); // direct hook mantle, we have to catch the createdevice call!

            if (!DX9Hook::IsPrecalculationDone())
            {
                DX9Hook::TryHook(true); // Fifaworld fix - precalculate DX9 offsets
            }

#ifndef _WIN64
            if (!DX8Hook::IsPrecalculationDone())
            {
                DX8Hook::TryHook(true); // Commnand & Conquer - need to create device first to avoid crash when Alt+TAB
            }
#endif
            OriginIGO::IGOLogInfo("Quitting OffsetCalculationAndEarlyHookingThread...");
        }
    }
    else
    {
        OriginIGO::IGOLogInfo("Failed OffsetCalculationAndEarlyHookingThread...(gIGOOffsetCalculationMutex)");            
    }
    return 0;
}

DEFINE_HOOK_SAFE(DWORD, LdrLoadDll_Hook, (PWSTR *szcwPath,    // Optional
                                    PDWORD pdwLdrErr,    // Optional
                                    PUNICODE_STRING pUniModuleName,
                                    PVOID pResultInstance))

    if(!LdrLoadDll_HookNext || !isLdrLoadDll_Hooked)
    {
        return 0;    
    }
    
    if (gExitingProcess || gQuitting)
    {
        return LdrLoadDll_HookNext(szcwPath, pdwLdrErr, pUniModuleName, pResultInstance);
    }

    wchar_t *loadModuleName = NULL;
    DWORD dummyStatus = 0;
    DWORD oldStatus = 0;
    BOOL hr=FALSE;
    DWORD result=0;

    {       
        hr=VirtualProtect((LPVOID)pUniModuleName->Buffer, pUniModuleName->Length*2, PAGE_EXECUTE_READWRITE, &oldStatus);

        loadModuleName = new wchar_t[pUniModuleName->Length + 1];
        loadModuleName[pUniModuleName->Length] = 0;

        if(hr==TRUE && pUniModuleName->Length>0 && pUniModuleName->Buffer!=NULL)
        {
            memcpy(loadModuleName, pUniModuleName->Buffer, pUniModuleName->Length * 2);

            VirtualProtect((LPVOID)pUniModuleName->Buffer, pUniModuleName->Length*2, oldStatus, &dummyStatus);

            _wcslwr_s(loadModuleName, pUniModuleName->Length);
        }
        else
        {
            if(hr==TRUE)
            {
                VirtualProtect((LPVOID)pUniModuleName->Buffer, pUniModuleName->Length*2, oldStatus, &dummyStatus);
            }
        }
    }


    {
        // Is it already in memory?
        HMODULE loadedModule = NULL;
        if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, loadModuleName, &loadedModule) == 0)
            loadedModule = NULL;

        // Could it be that we are trying to load another instance of IGO? In that case, redirect to our currently loaded DLL
        if (!RedirectIGOModuleLoading(pdwLdrErr, loadModuleName, pResultInstance, &result))
            result = LdrLoadDll_HookNext(szcwPath, pdwLdrErr, pUniModuleName, pResultInstance);

        // Pre-allocate area for hook trampolines around this new module - we do this in a separate thread because it's always "dangerous"
        // to do too much during the loading/unloading of a DLL (especially when it comes to mutexes/locks)
        if (!loadedModule)
        {
            if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, loadModuleName, &loadedModule))
                Mhook_PreAllocateTrampolines(loadModuleName);
        }

        // detect certain API's that we have to hook immediately - be very very carefull, what you do inside TryHook -> deadlock danger!!!
        // so even TryHook would deadlock Spore...

        /* catching an edge case by stacking threads:

        1)	we automatically started the check thread for Mantle and DX9 in our ATTACH_PROCESS section
        2)	later on, we get a LdrLoadDll for Mantle or DX9
        3)	however we already have a handle for the check thread, so you don’t create it again
        4)	BUT in the thread itself, we could be have passed the point that checks whether the Dll is loaded in the TryHook
        5)	-> no offsets get precalculated
        6)  to prevent this, we check stack multiple threads
        */


#ifdef _WIN64
        if (wcsstr(loadModuleName, L"mantle64.dll"))
#else
        if (wcsstr(loadModuleName, L"mantle32.dll"))
#endif
        {
            EA::Thread::AutoFutex m(gIGOOffsetCalculationMutex);
            if (!MantleHook::IsHooked() && !gQuitting && !gExitingProcess)
                MantleHook::TryHook(); // direct hook mantle, we have to catch the createdevice call!            }
        }
#ifndef _WIN64
        else
        if (wcsstr(loadModuleName, L"d3d8.dll"))
        {
            EA::Thread::AutoFutex m(gIGOOffsetCalculationMutex);
            if (!DX8Hook::IsPrecalculationDone() && !gQuitting && !gExitingProcess)
                DX8Hook::TryHook(true); // Commnand & Conquer - need to create device first to avoid crash when Alt+TAB
        }
#endif
        else
        if (wcsstr(loadModuleName, L"d3d9.dll"))
        {
            EA::Thread::AutoFutex m(gIGOOffsetCalculationMutex);
            if (!DX9Hook::IsPrecalculationDone() && !gQuitting && !gExitingProcess)
                DX9Hook::TryHook(true); // Fifaworld fix - precalculate DX9 offsets            }
        }
		else
        {
            // Always run that guy in case the DLLs we are interested in are "sub-loads" of the DLL we just loaded
            // DO NOT check the gIGOOffsetCalculationMutex mutex here to avoid deadlocking while the process is closing down
            if (!gQuitting && !gExitingProcess)
                DXGIHook::Setup();
        }
    }
    delete[] loadModuleName;
    
    return result;
}

void PreAllocateHookingTrampolines()
{
    OriginIGO::IGOLogInfo("Preallocating trampolines for DLLs");

    HANDLE hModuleSnap;
    // Take a snapshot of all modules in the specified process.
    hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
    if (hModuleSnap == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();
        OriginIGO::IGOLogWarn("CreateToolhelp32Snapshot(%lu)", error);
        return;
    }

    MODULEENTRY32W me32 = { 0 };
    me32.dwSize = sizeof(MODULEENTRY32W);

    // Retrieve information about the first module, and exit if unsuccessful
    if (!Module32FirstW(hModuleSnap, &me32))
    {
        CloseHandle(hModuleSnap);
        return;
    }

    // Now walk the module list of the process, and display information about each module
    do
    {
        wchar_t tmp[_MAX_PATH];
        wcscpy_s(tmp, _MAX_PATH, me32.szModule);
        _wcslwr_s(tmp, _countof(tmp));

        Mhook_PreAllocateTrampolines(tmp);

    } while (Module32NextW(hModuleSnap, &me32));

    CloseHandle(hModuleSnap);
}

bool IsKernel32ModuleLoaded(DWORD pid, int* stopEarlyErrorCode)
{
    bool found = false;

    HANDLE hModuleSnap;
    MODULEENTRY32W me32 = { 0 };

    // Take a snapshot of all modules in the specified process.
    hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    if (hModuleSnap == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();
        char errorMsg[128] = { 0 };
        sprintf(errorMsg, "CreateToolhelp32Snapshot(%lu)", error);
        OriginIGO::IGOLogWarn(errorMsg);

		if (error == ERROR_ACCESS_DENIED) // Elevated process? no reason to keep on retrying and mess up any activation timing (think Alice:Madness Returns)
		{
			if (stopEarlyErrorCode)
				*stopEarlyErrorCode = error;
		}

        return found;
    }

	if (stopEarlyErrorCode)
		*stopEarlyErrorCode = ERROR_SUCCESS;

    // Set the size of the structure before using it.
    me32.dwSize = sizeof(MODULEENTRY32W);

    // Retrieve information about the first module,
    // and exit if unsuccessful
    if (!Module32FirstW(hModuleSnap, &me32))
    {
        CloseHandle(hModuleSnap);           // clean the snapshot object
        return found;
    }

    // Now walk the module list of the process,
    // and display information about each module
    do
    {
        wchar_t tmp[_MAX_PATH];
        wcscpy_s(tmp, _MAX_PATH, me32.szModule);
        _wcslwr_s(tmp, _countof(tmp));

        if (wcsstr(tmp, L"kernel32.dll") != NULL)
        {
            found = true;
        }

    } while (!found && Module32NextW(hModuleSnap, &me32));

    CloseHandle(hModuleSnap);
    OriginIGO::IGOLogWarn("CreateToolhelp32Snapshot(0)");
    return found;
}

bool ExtractModuleFileName(HMODULE hInstDLL, WCHAR* fileNameBuffer, DWORD bufferEltCount)
{
    WCHAR strExePath[_MAX_PATH] = {0};
    DWORD fileNameSize = ::GetModuleFileName(hInstDLL, strExePath, _MAX_PATH);
    if (fileNameSize > 0 && fileNameSize < _MAX_PATH)
    {
        WCHAR strLowerCurrentProcessPathName[_MAX_PATH] = {0};

        // convert the name to a long file name and make it lower case
        fileNameSize = ::GetLongPathName(strExePath, strLowerCurrentProcessPathName, _MAX_PATH);
        if (fileNameSize > 0 && fileNameSize < _MAX_PATH)
        {
            if (_wcslwr_s(strLowerCurrentProcessPathName, _MAX_PATH) == 0)
            {
                // remove the path from the process name
                bool success = false;
                WCHAR* strLastSlash = wcsrchr(strLowerCurrentProcessPathName, L'\\');
                if (strLastSlash)
                    success = wcscpy_s(fileNameBuffer, bufferEltCount, &strLastSlash[1]) == 0;
                else
                    success = wcscpy_s(fileNameBuffer, bufferEltCount, strLowerCurrentProcessPathName) == 0;

                return success;
            }
        }
    }

    return false;
}

bool IsModuleAlreadyLoaded(DWORD pid, HMODULE hInstDLL, WCHAR* moduleFileName, BYTE** modBaseAddr, WCHAR* exePath)
{
    MODULEENTRY32W me32 = {0};

    // Take a snapshot of all modules in the specified process.
    HANDLE hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    if( hModuleSnap == INVALID_HANDLE_VALUE )
    {
        DWORD error = GetLastError();
        char errorMsg[128]={0};
        sprintf(errorMsg, "CreateToolhelp32Snapshot(%lu)", error);
        OriginIGO::IGOLogWarn(errorMsg);

        return false;
    }

    // Set the size of the structure before using it.
    me32.dwSize = sizeof( MODULEENTRY32W );

    // Retrieve information about the first module,
    // and exit if unsuccessful
    if( !Module32FirstW( hModuleSnap, &me32 ) )
    {
        CloseHandle( hModuleSnap );           // clean the snapshot object
        return false;
    }

    // Now walk the module list of the process, and check whether there is another instance of the IGO module already loaded
    // and display information about each module
    bool found = false;
    do
    {
        // Skip ourself
        if (hInstDLL && me32.hModule == hInstDLL)
            continue;

        wchar_t tmp[_MAX_PATH];
        wcscpy_s(tmp, _MAX_PATH, me32.szModule);
        _wcslwr_s(tmp, _countof(tmp));

        if(wcsstr(tmp, moduleFileName))
        {
            if (modBaseAddr)
                *modBaseAddr = me32.modBaseAddr;

            if (exePath)
            {
                wcscpy_s(exePath, _MAX_PATH, me32.szExePath);
                _wcslwr_s(exePath, _MAX_PATH);
            }

            found = true;
            TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_FAIL, TelemetryContext_GENERIC, TelemetryRenderer_UNKNOWN, "Trying to load another instance of the IGO module - stop processing");
            break;
        }
    
    } while(Module32NextW( hModuleSnap, &me32 ) );

    CloseHandle( hModuleSnap );
    return found;
}

bool RedirectIGOModuleLoading(PDWORD pdwLdrErr, WCHAR* srcFullModuleName, PVOID pResultInstance, DWORD* result)
{
    WCHAR moduleFileName[_MAX_PATH] = {0};
    if (ExtractModuleFileName(gInstDLL, moduleFileName, _MAX_PATH))
    {
        WCHAR complementModuleFileName[_MAX_PATH] = {0};

        // Handle both release and debug...
        size_t eltCnt = wcsnlen(moduleFileName, _MAX_PATH - 1); // give us some room to add a 'd' if necessary
        if (eltCnt > 5 && eltCnt < (_MAX_PATH - 1))
        {
            // Format: igo32/64(d).dll...
            if (moduleFileName[eltCnt - 5] == L'd')
            {
                wcsncpy(complementModuleFileName, moduleFileName, eltCnt - 5);
                wcscat(complementModuleFileName, &moduleFileName[eltCnt - 4]);
            }

            else
            {
                wcsncpy(complementModuleFileName, moduleFileName, eltCnt - 4);
                complementModuleFileName[eltCnt - 4] = L'd';
                wcscat(complementModuleFileName, &moduleFileName[eltCnt - 4]);
            }
        }

        if (wcsstr(srcFullModuleName, moduleFileName) || wcsstr(srcFullModuleName, complementModuleFileName))
        {
            // Looks like we are trying to load the IGO module again! -> redirect to the loaded one
            UNICODE_STRING uniModuleName;

            DWORD fileNameSize = ::GetModuleFileName(gInstDLL, moduleFileName, _MAX_PATH);
            if (fileNameSize > 0 && fileNameSize < _MAX_PATH)
            {
                TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_FAIL, TelemetryContext_GENERIC, TelemetryRenderer_UNKNOWN, "Trying to load another instance of the IGO module - redirecting");

                uniModuleName.Buffer = moduleFileName;
                uniModuleName.Length = (USHORT) wcslen(moduleFileName) * sizeof(WCHAR);
                uniModuleName.MaximumLength = sizeof(moduleFileName);

                *result = LdrLoadDll_HookNext(NULL, pdwLdrErr, &uniModuleName, pResultInstance);
                return true;
            }
        }
    }

    return false;
}

extern "C" DLL_EXPORT bool GetTwitchBroadcastChannelURL(const char* authToken, wchar_t **outChannelName)
{
    static wchar_t channelName[256];
    ZeroMemory(channelName, sizeof(wchar_t)*256);

    if (TTV_SUCCEEDED(TwitchManager::TTV_Init()))
    {
        TTV_AuthToken twitchAuthToken = {0};
        strncpy_s(twitchAuthToken.data, kAuthTokenBufferSize, authToken, _TRUNCATE);
        if (TTV_SUCCEEDED(TwitchManager::TTV_GetChannelOnly(&twitchAuthToken, channelName)))
        {
            while ( TwitchManager::QueryTTVStatus(TwitchManager::TTV_GETCHANNEL_ONLY_STATUS) == TTV_EC_REQUEST_PENDING )
            {
                Sleep(150);
                TwitchManager::TTV_PollTasks();
            }
        }
        *outChannelName = channelName;
        TwitchManager::TTV_Stop();
        TwitchManager::TTV_Shutdown();
    }
    return true;
}

// TODO: Remove this code to no longer rely on a known DLL list, modify checkhook instead to get the 3rd party dll name!

extern "C" DLL_EXPORT bool Is3rdPartyDllLoaded(DWORD pid)
{
	HANDLE hModuleSnap;
	MODULEENTRY32W me32 = {0};

    // Take a snapshot of all modules in the specified process.
    hModuleSnap = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, pid );
    if( hModuleSnap == INVALID_HANDLE_VALUE )
    {
        return false;
    }

    // Set the size of the structure before using it.
    me32.dwSize = sizeof( MODULEENTRY32W );

    // Retrieve information about the first module,
    // and exit if unsuccessful
    if( !Module32FirstW( hModuleSnap, &me32 ) )
    {
        CloseHandle( hModuleSnap );           // clean the snapshot object
        return false;
    }

    // Limit the telemetry we send about 3rd party libs
    static eastl::vector<bool> thirdPartyNotificationStatus(sizeof(g_3drPartyDLLs)/sizeof(g_3drPartyDLLs[0]), false);

    // Now walk the module list of the process,
    // and display information about each module
    g3rdPartyDllDetected = false;
    do
    {
        wchar_t tmp[_MAX_PATH];
        wcscpy_s(tmp, _MAX_PATH, me32.szModule);
        _wcslwr_s(tmp, _countof(tmp));

        char dllName[_MAX_PATH] = { 0 };
        
        for (int i = 0; i < sizeof(g_3drPartyDLLs) / sizeof(g_3drPartyDLLs[0]); ++i)
        {
            if (wcsstr(tmp, g_3drPartyDLLs[i]) != NULL)
            {
                OriginIGO::IGOLogWarn("Loaded 3rd party dll detected!");
                g3rdPartyDllDetected = true;

                if (!thirdPartyNotificationStatus[i])
                {
                    thirdPartyNotificationStatus[i] = true;
                    // put the dll name to the global variable
                    size_t s;
                    wcstombs_s(&s, dllName, (size_t)_MAX_PATH, tmp, (size_t)_MAX_PATH);
                    TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_THIRD_PARTY, TelemetryDispatcher::instance()->GetRenderer(), "Loading 3rd party DLL:%s", dllName);
					break;
                }
            }
        }
    
    } while( !g3rdPartyDllDetected && Module32NextW( hModuleSnap, &me32 ) );

    CloseHandle( hModuleSnap );
    return g3rdPartyDllDetected;
}


// Mutex variant for cross process locking
class IGOAutoLock_Mutex
{
public:
    explicit IGOAutoLock_Mutex( HANDLE hMutex, long lTimeOut_ms )
    {
        if (hMutex && ::WaitForSingleObject(hMutex, lTimeOut_ms) == WAIT_OBJECT_0)
            mhMutex = hMutex;
        else
            mhMutex = NULL;  // Timed out or invalid handle etc.
    }
    ~IGOAutoLock_Mutex()
    {
        if (mhMutex)
        {
            ::ReleaseMutex(mhMutex);
            mhMutex = NULL;
        }
    }
    bool isLocked() 
    {
        return (mhMutex != NULL); 
    }

private:
    HANDLE mhMutex;
};
    
extern "C" DLL_EXPORT void EnableIGOStressTest(bool enable)
{
    IGOAutoLock_Mutex oAutoLock(gGlobalIGOApplicationSharedDataMutex, 10000);
    getIGOConfigurationFlags().gEnabledStressTest = enable;
}

extern "C" DLL_EXPORT void EnableIGOLog(bool enable)
{
    IGOAutoLock_Mutex oAutoLock(gGlobalIGOApplicationSharedDataMutex, 10000);
    getIGOConfigurationFlags().gEnabledLogging = enable;
}

extern "C" DLL_EXPORT void EnableIGO(bool enable)
{
    IGOAutoLock_Mutex oAutoLock(gGlobalIGOApplicationSharedDataMutex, 10000);
    getIGOConfigurationFlags().gEnabled = enable;
}

extern "C" DLL_EXPORT void EnableIGODebugMenu(bool enable)
{
    IGOAutoLock_Mutex oAutoLock(gGlobalIGOApplicationSharedDataMutex, 10000);
    getIGOConfigurationFlags().gEnabledDebugMenu = enable;
}

// remove our hooks, so that we no longer inject into createprocess
extern "C" DLL_EXPORT void Unload()
{
    InjectHook::Cleanup(true);
}


// this flag is to tell the current IGO if we should inject created processes or not
extern "C" DLL_EXPORT bool InjectIGODll(HANDLE process, HANDLE thread)
{
    extern bool InjectCurrentDLL(HANDLE hProcess, HANDLE thread = NULL);
    return InjectCurrentDLL(process, thread);
}


void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    return malloc(size);
}

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    return _aligned_malloc(size, alignment);
}


static DWORD WINAPI IGOInjectThread(LPVOID lpParam)
{
    OriginIGO::IGOLogInfo("Starting IGOInjectThread...");

    bool InjectHooked = false;

    for (int i = 0; i < 500; i++) // 2.5 seconds
    {
        // inject
        if (!InjectHooked)
            InjectHooked = InjectHook::TryHook();

        if (
            gQuitting ||
            InjectHooked ||
            InjectHook::IsQuitting())
            break;

        Sleep(5);
    }
    OriginIGO::IGOLogInfo("Quitting IGOInjectThread...");

    return 0;
}

struct structMainWindowSearch
{
    DWORD   PID;  // PID to find window for
    HWND    hWnd;    // Found handle of requested PID
};

BOOL CALLBACK getProcMainWindowCallback(HWND hWnd, LPARAM lParam)
{
    if (!hWnd)
        return TRUE;
    if (!::IsWindowVisible(hWnd))   // visible top window
        return TRUE;

    structMainWindowSearch* pSrch = (structMainWindowSearch*)lParam;

    // Invalid search request with missing PID?
    if (0 == pSrch->PID)
        return TRUE;

    DWORD dwWindowPID = 0;
    ::GetWindowThreadProcessId(hWnd, &dwWindowPID);
    if (dwWindowPID == pSrch->PID)
    {

        // we have two handles or two pointers?
        // that means: a Window that is not Ansi or Unicode / a Window that is Ansi + Unicode -> both are invalid within our current process!!!
        if (pSrch->PID == GetCurrentProcessId())
        {
#ifndef _WIN64
            if (((GetWindowLongW(hWnd, GWLP_WNDPROC) & 0xFFFF0000) == 0xFFFF0000) && ((GetWindowLongA(hWnd, GWLP_WNDPROC) & 0xFFFF0000) == 0xFFFF0000))
                return TRUE;    // discard those windows....

            if (!((GetWindowLongW(hWnd, GWLP_WNDPROC) & 0xFFFF0000) == 0xFFFF0000) && !((GetWindowLongA(hWnd, GWLP_WNDPROC) & 0xFFFF0000) == 0xFFFF0000))
                return TRUE;    // discard those windows....
#else
            // we have two handles or two pointers?
            // that means: a Window that is not Ansi or Unicode / a Window that is Ansi + Unicode -> both are invalid!!!
            if (((GetWindowLongPtrW(hWnd, GWLP_WNDPROC) & 0xFFFF0000) == 0xFFFF0000) && ((GetWindowLongPtrA(hWnd, GWLP_WNDPROC) & 0xFFFF0000) == 0xFFFF0000))
                return TRUE;    // discard those windows....

            if (!((GetWindowLongPtrW(hWnd, GWLP_WNDPROC) & 0xFFFF0000) == 0xFFFF0000) && !((GetWindowLongPtrA(hWnd, GWLP_WNDPROC) & 0xFFFF0000) == 0xFFFF0000))
                return TRUE;    // discard those windows....
#endif
        }
        pSrch->hWnd = hWnd;
        return FALSE;  // Match found so stop searching
    }

    return TRUE; // No match so keep searching
}


HWND getProcMainWindow(DWORD PID)
{
    if (0 == PID) return NULL;

    structMainWindowSearch oSrch;
    oSrch.PID = PID;
    oSrch.hWnd = NULL;
    ::EnumWindows(getProcMainWindowCallback, (LPARAM)&oSrch);
    return IsSystemWindow(oSrch.hWnd) ? NULL : oSrch.hWnd;  // Success or failure
}

static DWORD WINAPI MonitorTrialProcessThread(LPVOID lpParam)
{
    int64_t timeRemaining = -1;
    DWORD processId = GetCurrentProcessId();
    DWORD monitorThreadTime = GetTickCount();

    while(true)
    {
        // Have we received the appropriate trial info yet?
        if (gameInfo()->isValid())
        {
            {
                //not a free trial, or IGO is not responsible for terminating the free trial, hence no need to monitor
                if(!gameInfo()->isTrial())
                {
                    return 0;
                }
                timeRemaining = gameInfo()->trialTimeRemaining();
            }
            //lets wait for a valid time remaining
            if(timeRemaining != -1)
            {
                monitorThreadTime = GetTickCount();
                break;
            }
            else
            {
                //disable the failsafe for now while I investigate some cases where the game terminates
    #if 0
                //as a failsafe if for some reason we haven't received a valid time from Origin within a minute
                //kill the app
                if(GetTickCount() - monitorThreadTime > 60000)
                {
                    goto terminate;
                }
    #endif
                Sleep(2000);
            }
        }

        else
        {
            Sleep(2000);
        }
    }

    //sit here until our trial time has expired
    while(GetTickCount() - monitorThreadTime < timeRemaining)
    {
        Sleep(1000);
    }

    //kill the current app
    gameInfo()->setTrialTerminated(true);
    
    HWND hProcMainWnd = NULL;
    int procMainIterCnt = 5;

    // Wait until we have a valid window to work with, especially if we try to stop the game immediately/try to avoid a hard kill (immediate hard kill even worse) that may cause a hang!
    if (timeRemaining < 5000)
        Sleep(5000);

    while (procMainIterCnt > 0)
    {
        hProcMainWnd = getProcMainWindow(processId);
        if (hProcMainWnd)
            break;

        --procMainIterCnt;
        Sleep(1000);
    }

    if (hProcMainWnd)
    {
        if (IsWindowUnicode(hProcMainWnd))
            SendMessageTimeoutW(hProcMainWnd, WM_CLOSE, 0, 0, SMTO_BLOCK, 5000, NULL);
        else
            SendMessageTimeoutA(hProcMainWnd, WM_CLOSE, 0, 0, SMTO_BLOCK, 5000, NULL);
    }

    //lets see if the process is still active, if so we wait and if it hasn't closed before a minute
    //lets drop the hammer
    HANDLE hProcess = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE, FALSE, processId);
    // Different operating systems behave differently with this call.  XP doesn't support PROCESS_QUERY_LIMITED_INFORMATION,
    // but in some situations Windows 7 fails
    if (!hProcess)
        hProcess = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE, FALSE, processId);

    if (hProcess)
    {
        DWORD exitCode = STILL_ACTIVE;
        bool isSuccess = 0 != ::GetExitCodeProcess(hProcess, &exitCode);

        if (isSuccess && exitCode == STILL_ACTIVE)
        {
            DWORD result = WaitForSingleObject(hProcess, 15000);

            //if we reached here for any reason other than then app terminated
            if (result != WAIT_OBJECT_0)
            {
                HANDLE hGameProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
                if (hGameProcess)
                {
                    TerminateProcess(hGameProcess, 0xf291);
                    CloseHandle(hGameProcess);
                }
            }
        }
    }

    // Hopefully we'll never get here! - but got to stop this crazy game!
    int* ptr = NULL;
    *ptr = 0;

    return 0;
}


bool WaitForSDKSignal(DWORD pid, uint32_t msToWait)
{
    wchar_t signalName[128] = OIG_SDK_SIGNAL_NAME;
    wchar_t processId[16] = {};

    if (!_itow_s(pid, processId, _countof(processId), 10))
        wcscat_s(signalName, _countof(signalName), processId);

    HANDLE SDKSignalHandle = OpenEventW(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, signalName);
    bool found = false;

    if (SDKSignalHandle)
    {
        DWORD waitResult = ::WaitForSingleObject(SDKSignalHandle, msToWait);
        if (waitResult == WAIT_OBJECT_0)
        {
            OriginIGO::IGOLogWarn("Detected SDK Game launch (PID %i)!", pid);
            found = true;

            if (!ResetEvent(SDKSignalHandle))   // reset the signal for the next SDK game
                OriginIGO::IGOLogWarn("Reset signal failed.");

        }
        CloseHandle(SDKSignalHandle);
    }

    return found && SDKSignalHandle;
}


static DWORD WINAPI SDKSignalThread(LPVOID lpParam)
{
    OriginIGO::IGOLogInfo("Starting SDKSignalThread...");
    
    while (!gQuitting && !gSDKSignaled && !DX9Hook::mDX9Initialized
#ifndef _WIN64        
        && !DX8Hook::mDX8Initialized)
#else
        )
#endif
    {
        gSDKSignaled = WaitForSDKSignal(GetCurrentProcessId(), 250);    // signal with PID has priority over fallback signal with PID 0
        if (!gQuitting && !gSDKSignaled && !DX9Hook::mDX9Initialized
#ifndef _WIN64        
            && !DX8Hook::mDX8Initialized)
#else
            )
#endif
        {
            gSDKSignaled = WaitForSDKSignal(0 /*pid for non RTP games*/, 250);
        }
    }

    OriginIGO::IGOLogInfo("Quitting SDKSignalThread...");
    return 0;
}

static DWORD WINAPI IGODestroyThread(LPVOID lpParam)
{
    OriginIGO::IGOLogInfo("Starting IGODestroyThread...");
    EA::Thread::AutoFutex m(gIGOCreateDestroyMutex);
    OriginIGO::IGOLogDebug("  IGODestroyThread past mutex");
        
    // destruct all hooks in a seperate thread!!!
    if (TwitchManager::IsTTVDLLReady(false))
    {
        OriginIGO::IGOLogDebug("  IGODestroyThread past IsTTVDLLReady");
        TwitchManager::TTV_Stop();
        TwitchManager::TTV_Shutdown();
    }
    OriginIGO::IGOLogDebug("  IGODestroyThread past if (IsTTVDLLReady){}");
        
    // cleanup IGO
#ifndef _WIN64
    DX8Hook::Cleanup();
#endif
    DX9Hook::Cleanup();
    DX10Hook::Cleanup();
    DX11Hook::Cleanup();
    DX12Hook::Cleanup();
    OGLHook::Cleanup();
    
    // cleanup input hooks
    InputHook::Cleanup();
    
    gIGOQuittedFlagMutex.Lock();
    gQuitted = true;
    gIGOQuittedFlagMutex.Unlock();

    OriginIGO::IGOLogInfo("Quitting IGODestroyThread...");
    return 0;
}

static DWORD WINAPI IGOCreateThread(LPVOID lpParam)
{
    if (gQuitting)
        return 0;

    bool hookingStressTest = getIGOConfigurationFlags().gEnabledStressTest;

    gIGOQuittedFlagMutex.Lock();
    gQuitted = false;
    gIGOQuittedFlagMutex.Unlock();

    OriginIGO::IGOLogInfo("Starting IGOCreateThread...");
    EA::Thread::AutoFutex m(gIGOCreateDestroyMutex);

    // Reset from closing a hooked window (for example FIFA15 settings sub-window of launcher)
    InputHook::EnableWindowHooking();

    // Telemetry stats
    static bool telemetryFailCheckDone = false;
    static time_t telemetryStartTime = time(NULL);
    static const int TELEMETRY_MAX_DELAY_IN_SECONDS = 30;

    // Let's wait until we have a valid window to work with - we don't want to try/hook certain Windows API (ie SetWindowLong, etc...) while the
    // game creates a window - otherwise we may "disturb" the creation process (for example, we would get an ANSI titlebar to display as UNICODE in DOI)
    bool hasWindow = false;
    HWND mainWnd = NULL;

    OriginIGO::IGOLogInfo("Waiting on creation of main window");
    while (!hasWindow && !gQuitting)
    {
        mainWnd = getProcMainWindow(GetCurrentProcessId());
        hasWindow = mainWnd != NULL;

        if (!hasWindow && !gQuitting)
            Sleep(1000);
    }

    if (mainWnd)
    {
        OriginIGO::IGOLogInfo("Found main window %p", mainWnd);

        // Are we dealing with a .Net launcher? (ie we need to improve our handling of the SetWindowLong... stuff before
        // being able to hook properly into the process). I don't like this because we're using WAG heuristics, but it
        // temporarily fixes FIFA15 launchers

        // 1) Look for .Net core library
        bool netProcess = IsProcessDotNet(GetCurrentProcess());

        // 2) Check whether the windows' class name contains "WindowsForms"
        bool formWindow = false;
        HWND rootWnd = GetAncestor(mainWnd, GA_ROOTOWNER);
        if (!rootWnd)
            rootWnd = mainWnd;

        if (IsWindowUnicode(rootWnd))
        {
            WCHAR buffer[256] = {0};
            if (GetClassNameW(rootWnd, buffer, _countof(buffer)) > 0)
            {
                buffer[_countof(buffer) - 1] = 0;
                _wcslwr_s(buffer, _countof(buffer));
                if (wcsstr(buffer, L"windowsforms") > 0)
                    formWindow = true;
            }
        }

        else
        {
            CHAR buffer[256] = {0};
            if (GetClassNameA(rootWnd, buffer, _countof(buffer)) > 0)
            {
                buffer[_countof(buffer) - 1] = 0;
                _strlwr_s(buffer, _countof(buffer));
                if (strstr(buffer, "windowsforms") > 0)
                    formWindow = true;
            }
        }

        // We're done!
        if (netProcess && formWindow)
        {
            OriginIGO::IGOLogWarn("Detected .Net and WinForms");
            //return 0; - kept the code around in case we have to disable injection for .NET process again/while debugging of crash is pending
        }
    }


	InputHook::TryHook();

    while (!gQuitting)
    {
        if (!telemetryFailCheckDone)
        {
            // Skip Mantle here, since we immediately hook/load the dlls
            telemetryFailCheckDone = MantleHook::IsHooked()
#ifndef _WIN64
                || DX8Hook::IsHooked()
#endif
                || OGLHook::IsHooked()
                || DX9Hook::IsHooked()
                || DXGIHook::IsDX10Hooked()
                || DXGIHook::IsDX11Hooked()
                || DXGIHook::IsDX12Hooked();

            if (!telemetryFailCheckDone)
            {
                double elapsedInSeconds = difftime(time(NULL), telemetryStartTime);
                if (elapsedInSeconds >= TELEMETRY_MAX_DELAY_IN_SECONDS)
                {
                    TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_FAIL, TelemetryContext_GENERIC, TelemetryRenderer_UNKNOWN, "Unable to hook after %d seconds", TELEMETRY_MAX_DELAY_IN_SECONDS);
                    telemetryFailCheckDone = true;
                }
            }
        }

        if (gQuitting)
            break;
        

        //do DX8 hooking at first, to catch the CreateDevice calls !!!
        HWND hProcMainWnd = getProcMainWindow(GetCurrentProcessId());
        InputHook::HookWindow(hProcMainWnd);


        if (hookingStressTest)
        {
            MantleHook::Cleanup();
            Sleep(500);
            if (!MantleHook::IsHooked() && !gQuitting)
            {
                if (IsWindowUnicode(hProcMainWnd))
                    PostMessageW(hProcMainWnd, OriginIGO::gIGOHookMantleUpdateMessage, 0, 0);
                else
                    PostMessageA(hProcMainWnd, OriginIGO::gIGOHookMantleUpdateMessage, 0, 0);
            }
            Sleep(500);
#ifndef _WIN64
            DX8Hook::Cleanup();
            Sleep(500);
            if (!DX8Hook::IsHooked() && !gQuitting)
            {
                if (IsWindowUnicode(hProcMainWnd))
                    PostMessageW(hProcMainWnd, OriginIGO::gIGOHookDX8UpdateMessage, 0, 0);
                else
                    PostMessageA(hProcMainWnd, OriginIGO::gIGOHookDX8UpdateMessage, 0, 0);
            }
            Sleep(500);
#endif
        }

        int wait_count = hookingStressTest == true ? 2 : 16;    // give the game 8 seconds, 4 seconds seems to small, esp. if 3rd party apps like fraps attach to the game as well!) to initiate its renderers before we try to attach IGO
        
        for (int c = 0; (c < wait_count) && !gQuitting; c++)
            Sleep(500);

        if (gQuitting)
            break;

        // check if 3rd party dlls are loaded
        if (Is3rdPartyDllLoaded(GetCurrentProcessId()))
        {
            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_GENERIC, TelemetryRenderer_UNKNOWN, "Detected 3rd party hook"))
            OriginIGO::IGOLogInfo("Detected 3rd party hook");
            g3rdPartyDLLLoaded = true;

            // disable logging, if IGO is broken
            OriginIGO::IGOLogger::instance()->enableLogging(getIGOConfigurationFlags().gEnabledLogging);            
        }

        // startup IGO		
        if (!hookingStressTest)
        {
#ifndef _WIN64
            if (!DX8Hook::IsHooked() && DX8Hook::IsReadyForRehook() && !gQuitting)
            {
                CALL_ONCE_ONLY(IGOLogInfo("Posting DX8 hook message"));
                if (IsWindowUnicode(hProcMainWnd))
                    PostMessageW(hProcMainWnd, OriginIGO::gIGOHookDX8UpdateMessage, 0, 0);
                else
                    PostMessageA(hProcMainWnd, OriginIGO::gIGOHookDX8UpdateMessage, 0, 0);
            }
#endif

            // put opengl first, because of javaw.exe ...
            if (!OGLHook::IsHooked() && OGLHook::IsReadyForRehook() && !gQuitting)
            {
                CALL_ONCE_ONLY(IGOLogInfo("Posting OpenGL hook message"));
                if (IsWindowUnicode(hProcMainWnd))
                    PostMessageW(hProcMainWnd, OriginIGO::gIGOHookGLUpdateMessage, 0, 0);
                else
                    PostMessageA(hProcMainWnd, OriginIGO::gIGOHookGLUpdateMessage, 0, 0);
            }

            if (!DX9Hook::IsHooked() && DX9Hook::IsReadyForRehook() && !gQuitting)
            {
                CALL_ONCE_ONLY(IGOLogInfo("Posting DX9 hook message"));
                if (IsWindowUnicode(hProcMainWnd))
                    PostMessageW(hProcMainWnd, OriginIGO::gIGOHookDX9UpdateMessage, 0, 0);
                else
                    PostMessageA(hProcMainWnd, OriginIGO::gIGOHookDX9UpdateMessage, 0, 0);
            }
        }
        else
        {
            DX9Hook::Cleanup();
            if (IsWindowUnicode(hProcMainWnd))
                PostMessageW(hProcMainWnd, OriginIGO::gIGOHookDX9UpdateMessage, 0, 0);
            else
                PostMessageA(hProcMainWnd, OriginIGO::gIGOHookDX9UpdateMessage, 0, 0);
            if (!gQuitting)
                Sleep(1000);
            DX10Hook::Cleanup();
            DX11Hook::Cleanup();
                        			
			DX12Hook::Cleanup();
            if (!gQuitting)
                Sleep(1000);

            OGLHook::Cleanup();
            if (IsWindowUnicode(hProcMainWnd))
                PostMessageW(hProcMainWnd, OriginIGO::gIGOHookGLUpdateMessage, 0, 0);
            else
                PostMessageA(hProcMainWnd, OriginIGO::gIGOHookGLUpdateMessage, 0, 0);
            if (!gQuitting)
                Sleep(1000);
            InputHook::Cleanup();
            if (!gQuitting)
                Sleep(1000);
            InputHook::TryHook();
            if (!gQuitting)
                Sleep(1000);
        }
    }
    OriginIGO::IGOLogInfo("Quitting IGOCreateThread...");

    return 0;
}


// this thread checks, if the rendering thread is still called, if so, we do not have to to anything, otherwise, kill & re-init IGO
static DWORD WINAPI IGORendererTimeoutDetectThread(LPVOID lpParam)
{
    if ((gPresentHookCalled || g3rdPartyResetForced || gTestCooperativeLevelHook_or_ResetHook_Called) && !gQuitting && !gExitingProcess)
    {
        gTestCooperativeLevelHook_or_ResetHook_Called = false;  // only needed to get here in case of broken hooking!

        // wait REHOOKCHECK_DELAY + 5 seconds, to see if our renderer is still working
        gPresentHookCalled = false;
        for (int c = 0; c < (REHOOKCHECK_DELAY + 5000) && gPresentHookCalled == false; c+=200)
        {
            if (gQuitting || gExitingProcess)
            {
                CloseHandle(gIGORendererTimeoutDetectThreadHandle);
                gIGORendererTimeoutDetectThreadHandle = NULL;
                return 0; 
            }
            Sleep(200);
        }

        // still alive?
        if (gPresentHookCalled || gTestCooperativeLevelHook_or_ResetHook_Called)
        {
            OriginIGO::IGOLogWarn("IGORenderer timeout detected, but hooking is still working. (%i)", gLastTestCooperativeLevelError);
            CloseHandle(gIGORendererTimeoutDetectThreadHandle);
            gIGORendererTimeoutDetectThreadHandle = NULL;
            g3rdPartyResetForced = false;
            return 0;
        }
    

        if (!OriginIGO::gQuitting && !OriginIGO::gExitingProcess && !TwitchManager::IsBroadCastingRunning())
        {
            OriginIGO::IGOLogWarn("IGORenderer timeout detected. (%i)", gLastTestCooperativeLevelError);
            gPresentHookThreadId = 0;
            gPresentHookCalled = false;
            gTestCooperativeLevelHook_or_ResetHook_Called = false;
            IGOCleanup();
            for (int c = 0; c < 20; c++)
            {
                // ignore gQuitting here, since IGOCleanup set that to true!!!
                if (gExitingProcess)
                {
                    CloseHandle(gIGORendererTimeoutDetectThreadHandle);
                    gIGORendererTimeoutDetectThreadHandle = NULL;
                    g3rdPartyResetForced = false;
                    return 0; 
                }
                Sleep(100);
            }
            IGOReInit();
            g3rdPartyResetForced = false;
        }
    }

    CloseHandle(gIGORendererTimeoutDetectThreadHandle);
    gIGORendererTimeoutDetectThreadHandle = NULL;

    return 0;
}

static DWORD WINAPI IGOReInitWaitThread(LPVOID lpParam)
{
    OriginIGO::IGOLogInfo("Starting IGOReInitWaitThread...");

    OriginIGO::AutoTryFutex m(gIGOOffsetCalculationMutex, 2000);
    if (m.Locked())// to prevent the execution of OffsetCalculationAndEarlyHookingThread while we are here!
    {
        OriginIGO::AutoTryFutex m2(gIGOCreateDestroyMutex, 2000);
        if (m2.Locked())
        {
            if (gIGOCreateThreadHandle)
            {
                DWORD exitCode = 0;

                DWORD waitResult = ::WaitForSingleObject(gIGOCreateThreadHandle, 1000);
                if (waitResult == WAIT_OBJECT_0)
                {
                    if (GetExitCodeThread(gIGOCreateThreadHandle, &exitCode))
                    {
                        if (exitCode != STILL_ACTIVE)
                        {    // if the thread is no longer active, close the handle
                            CloseHandle(gIGOCreateThreadHandle);
                            gIGOCreateThreadHandle = NULL;
                        }
                    }
                }
                else
                {
                    OriginIGO::IGOLogWarn("gIGOCreateThreadHandle failed to terminate normally...");
                    TerminateThread(gIGOCreateThreadHandle, 0);
                    CloseHandle(gIGOCreateThreadHandle);
                    gIGOCreateThreadHandle = NULL;
                }
            }

            gIGOQuittingFlagMutex.Lock();
            if (!gQuitting || gExitingProcess)
            {
                gIGOQuittingFlagMutex.Unlock();
                OriginIGO::IGOLogDebug("Quitting IGOReInitWaitThread... (exiting)");

                return 0;
            }

            gQuitting = false;

            gIGOQuittingFlagMutex.Unlock();

            // start creation thread
            if (gIGOCreateThreadHandle == NULL)
                gIGOCreateThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)IGOCreateThread, gInstDLL, 0, NULL);   
        }
        else
        {
            OriginIGO::IGOLogInfo("Failed IGOReInitWaitThread...(gIGOCreateDestroyMutex)");
        }
    }
    else
    {
        OriginIGO::IGOLogInfo("Failed IGOReInitWaitThread...(gIGOOffsetCalculationMutex)");
    }
    
    OriginIGO::IGOLogInfo("Quitting IGOReInitWaitThread...");
    return 0;
}

static DWORD WINAPI IGODestroyWaitThread(LPVOID lpParam)
{
    OriginIGO::IGOLogInfo("Starting IGODestroyWaitThread...");
    EA::Thread::AutoFutex m(gIGOCreateDestroyMutex);

    if (gIGODestroyThreadHandle)
    {
        DWORD exitCode = 0;

        DWORD waitResult = ::WaitForSingleObject(gIGODestroyThreadHandle, 1000);
        if (waitResult == WAIT_OBJECT_0)
        {
            if (GetExitCodeThread(gIGODestroyThreadHandle, &exitCode))
            {
                if (exitCode != STILL_ACTIVE)
                {    // if the thread is no longer active, close the handle
                    CloseHandle(gIGODestroyThreadHandle);
                    gIGODestroyThreadHandle = NULL;
                }
            }
        }
        else
        {
            OriginIGO::IGOLogWarn("gIGODestroyThreadHandle failed to terminate normally...");
            TerminateThread(gIGODestroyThreadHandle, 0);
            CloseHandle(gIGODestroyThreadHandle);
            gIGODestroyThreadHandle = NULL;
        }
    }

    // start destroy thread
    if (gIGODestroyThreadHandle == NULL)
        gIGODestroyThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)IGODestroyThread, gInstDLL, 0, NULL);


    OriginIGO::IGOLogInfo("Quitting IGODestroyWaitThread...");
    
    return 0;
}

namespace OriginIGO {

    // called in InputHook when WM_CLOSE/WM_DESTROY is received
    void IGOCleanup()
    {
        OriginIGO::IGOLogInfo("Starting IGOCleanup...");
        EA::Thread::AutoFutex m(gIGOCreateDestroyWaitMutex);
        OriginIGO::IGOLogDebug(" IGOCleanup... past mutex");

        if (gIGODestroyWaitThreadHandle)
        {
            DWORD exitCode = 0;
            OriginIGO::IGOLogDebug(" IGOCleanup... WaitForSingleObject");

            DWORD waitResult = ::WaitForSingleObject(gIGODestroyWaitThreadHandle, 1000);
            if (waitResult == WAIT_OBJECT_0)
            {
                if (GetExitCodeThread(gIGODestroyWaitThreadHandle, &exitCode))
                {
                    if (exitCode != STILL_ACTIVE)
                    {    // if the thread is no longer active, close the handle
                        CloseHandle(gIGODestroyWaitThreadHandle);
                        gIGODestroyWaitThreadHandle = NULL;
                    }
                }
            }
            else
            {
                OriginIGO::IGOLogWarn("gIGODestroyWaitThreadHandle failed to terminate normally...");
                TerminateThread(gIGODestroyWaitThreadHandle, 0);
                CloseHandle(gIGODestroyWaitThreadHandle);
                gIGODestroyWaitThreadHandle = NULL;
            }
        }

        // Already quitting?
        gIGOQuittingFlagMutex.Lock();
        if (gQuitting)
        {
            gIGOQuittingFlagMutex.Unlock();
            OriginIGO::IGOLogDebug(" IGOCleanup... quitting");
            return;
        }

        gQuitting = true;

        gIGOQuittingFlagMutex.Unlock();

        if (gIGODestroyWaitThreadHandle == NULL)
            gIGODestroyWaitThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)IGODestroyWaitThread, gInstDLL, 0, NULL);

        OriginIGO::IGOLogInfo("Leaving IGOCleanup... ");
    }

    // called after we called IGOCleanup and have a new main game window that needs IGO hooks
    void IGOReInit()
    {
        OriginIGO::IGOLogInfo("Starting IGOReInit...");
        EA::Thread::AutoFutex m(gIGOCreateDestroyWaitMutex);
        OriginIGO::IGOLogDebug(" IGOReInit... past mutex");

        if (gIGOReInitWaitThreadHandle)
        {
            DWORD exitCode = 0;

            OriginIGO::IGOLogDebug(" IGOReInit... WaitForSingleObject");
            DWORD waitResult = ::WaitForSingleObject(gIGOReInitWaitThreadHandle, 1000);
            if (waitResult == WAIT_OBJECT_0)
            {
                if (GetExitCodeThread(gIGOReInitWaitThreadHandle, &exitCode))
                {
                    if (exitCode != STILL_ACTIVE)
                    {    // if the thread is no longer active, close the handle
                        CloseHandle(gIGOReInitWaitThreadHandle);
                        gIGOReInitWaitThreadHandle = NULL;
                    }
                }
            }
            else
            {
                OriginIGO::IGOLogWarn("gIGOReInitWaitThreadHandle failed to terminate normally...");
                TerminateThread(gIGOReInitWaitThreadHandle, 0);
                CloseHandle(gIGOReInitWaitThreadHandle);
                gIGOReInitWaitThreadHandle = NULL;
            }
        }

        gIGOQuittingFlagMutex.Lock();
        if (!gQuitting)
        {
            gIGOQuittingFlagMutex.Unlock();
            OriginIGO::IGOLogDebug(" IGOReInit... quitting");
            return;
        }
        gIGOQuittingFlagMutex.Unlock();

        if (gIGOReInitWaitThreadHandle == NULL)
            gIGOReInitWaitThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)IGOReInitWaitThread, gInstDLL, 0, NULL);

        OriginIGO::IGOLogInfo("Leaving IGOReInit...");
    }

    // called in InjectHook when ExitProcess is called
    // this is to allow the create/destroy/cleanup async threads a chance to complete before the DLL gets detached which causes
    // asserts to be fired due to Futexes getting released while still locked.
    void IGOFinalCleanup()
    {
        // Always turn on logging during cleanup
        OriginIGO::IGOLogger::instance()->enableLogging(true);

        OriginIGO::IGOLogInfo("IGOFinalCleanup called");

        gIGOQuittingFlagMutex.Lock();
        gQuitting = true;
        gExitingProcess = true;
        gIGOQuittingFlagMutex.Unlock();

        
        EA::Thread::AutoFutex m(gIGOCreateDestroyWaitMutex);

        EA::Thread::AutoFutex m2(gIGOOffsetCalculationMutex);   // gIGOOffsetCalculationMutex -> to prevent the execution of OffsetCalculationAndEarlyHookingThread (which would now fail anyway due to gQuitting = true)
                                                                // use AutoFutex instead of TryLock here, because IGOFinalCleanup has the priority over all "TryLock" code parts!

        // stop twitch...
        if (TwitchManager::IsTTVDLLReady(false))
        {
            TwitchManager::TTV_Stop();
            TwitchManager::TTV_Shutdown();
        }
        
        // this is ugly but it is remotely possible to have two layers of CreateWait/DestroyWait threads pending
        // the second pass through shouldn't cost anything, but will catch an extra pending create.
        
        for (int i = 0; i < 2; i++) // this will take max 2 seconds
        {
            eastl::vector<HANDLE> handles;

            if (gIGORendererTimeoutDetectThreadHandle)
            {
                OriginIGO::IGOLogDebug("IGOFinalCleanup ... adding gIGORendererTimeoutDetectThreadHandle");
                handles.push_back(gIGORendererTimeoutDetectThreadHandle);
            }
            
            if (gIGOCreateThreadHandle)
            {
                OriginIGO::IGOLogDebug("IGOFinalCleanup ... adding gIGOCreateThreadHandle");
                handles.push_back(gIGOCreateThreadHandle);
            }

            if (gIGODestroyWaitThreadHandle)
            {
                OriginIGO::IGOLogDebug("IGOFinalCleanup ... adding gIGODestroyWaitThreadHandle");
                handles.push_back(gIGODestroyWaitThreadHandle);
            }
            
            if (gIGODestroyThreadHandle)
            {
                OriginIGO::IGOLogDebug("IGOFinalCleanup ... adding gIGODestroyThreadHandle");
                handles.push_back(gIGODestroyThreadHandle);
            }
            
            if (gIGOReInitWaitThreadHandle)
            {
                OriginIGO::IGOLogDebug("IGOFinalCleanup ... adding gIGOReInitWaitThreadHandle");
                handles.push_back(gIGOReInitWaitThreadHandle);
            }

            if (!handles.empty())
            {
                // handles.data() is allocated without padding & alignment for entities <= 8 bytes
                // allocator: void* allocate_memory(Allocator& a, size_t n, size_t alignment, size_t alignmentOffset)
                DWORD waitResult = ::WaitForMultipleObjects(static_cast<DWORD>(handles.size()), handles.data(), TRUE, 1000);
                OriginIGO::IGOLogInfo("IGOFinalCleanup WaitForMultipleObjects returned %d (%d)", waitResult, handles.size());
                if (waitResult == WAIT_OBJECT_0)
                {
                    if (eastl::find(handles.begin(), handles.end(), gIGORendererTimeoutDetectThreadHandle) != handles.end())
                    {
                        CloseHandle(gIGORendererTimeoutDetectThreadHandle);
                        gIGORendererTimeoutDetectThreadHandle = NULL;
                    }

                    if (eastl::find(handles.begin(), handles.end(), gIGOCreateThreadHandle) != handles.end())
                    {
                        CloseHandle(gIGOCreateThreadHandle);
                        gIGOCreateThreadHandle = NULL;
                    }

                    if (eastl::find(handles.begin(), handles.end(), gIGODestroyWaitThreadHandle) != handles.end())
                    {
                        CloseHandle(gIGODestroyWaitThreadHandle);
                        gIGODestroyWaitThreadHandle = NULL;
                    }
                    
                    if (eastl::find(handles.begin(), handles.end(), gIGODestroyThreadHandle) != handles.end())
                    {
                        CloseHandle(gIGODestroyThreadHandle);
                        gIGODestroyThreadHandle = NULL;
                    }
                    
                    if (eastl::find(handles.begin(), handles.end(), gIGOReInitWaitThreadHandle) != handles.end())
                    {
                        CloseHandle(gIGOReInitWaitThreadHandle);
                        gIGOReInitWaitThreadHandle = NULL;
                    }
                }
                else
                {

                    OriginIGO::IGOLogWarn("IGOFinalCleanup: threads not done. Increase timeout, if this was not caused by a debugger!");

                    // DO NOT TERMINATE ANY THREAD EXPLICITLY - THIS COULD CAUSE DEADLOCKS, ESPECIALLY IN DEBUG MODE. MOST LIKELY
                    // IN THE LOGGER OR THE HOOKING CODE.
#if 0
                    if (eastl::find(handles.begin(), handles.end(), gIGORendererTimeoutDetectThreadHandle) != handles.end())
                    {
                        TerminateThread(gIGORendererTimeoutDetectThreadHandle, 0);
                        CloseHandle(gIGORendererTimeoutDetectThreadHandle);
                        gIGORendererTimeoutDetectThreadHandle = NULL;
                        OriginIGO::IGOLogWarn("gIGORendererTimeoutDetectThread failed to terminate normally...");
                    }
                    if (eastl::find(handles.begin(), handles.end(), gIGOCreateThreadHandle) != handles.end())
                    {
                        TerminateThread(gIGOCreateThreadHandle, 0);
                        CloseHandle(gIGOCreateThreadHandle);
                        gIGOCreateThreadHandle = NULL;
                        OriginIGO::IGOLogWarn("IGOCreateThread failed to terminate normally...");
                    }
                    if (eastl::find(handles.begin(), handles.end(), gIGODestroyWaitThreadHandle) != handles.end())
                    {
                        TerminateThread(gIGODestroyWaitThreadHandle, 0);
                        CloseHandle(gIGODestroyWaitThreadHandle);
                        gIGODestroyWaitThreadHandle = NULL;
                        OriginIGO::IGOLogWarn("IGODestroyWaitThread failed to terminate normally...");
                    }
                    if (eastl::find(handles.begin(), handles.end(), gIGODestroyThreadHandle) != handles.end())
                    {
                        TerminateThread(gIGODestroyThreadHandle, 0);
                        CloseHandle(gIGODestroyThreadHandle);
                        gIGODestroyThreadHandle = NULL;
                        OriginIGO::IGOLogWarn("IGODestroyThread failed to terminate normally...");
                    }
                    if (eastl::find(handles.begin(), handles.end(), gIGOReInitWaitThreadHandle) != handles.end())
                    {
                        TerminateThread(gIGOReInitWaitThreadHandle, 0);
                        CloseHandle(gIGOReInitWaitThreadHandle);
                        gIGOReInitWaitThreadHandle = NULL;
                        OriginIGO::IGOLogWarn("IGOReInitWaitThread failed to terminate normally...");
                    }
#endif
                }
            }
        }

        // In case InputHook::CleanupLater() was called on a DX8/DX9 release.
        // this will take max 0.5 seconds
        InputHook::FinishCleanup();

        // DO NOT TRY TO UNHOOK CORE LOAD/EXEC METHODS IN CASE OF A RACE CONDITION - FOR EXAMPLE A LAUNCHER EXECUTING A SHELL
        // COMMAND AND IMMEDIATELY EXITING
        //InjectHook::Cleanup(true);

#ifdef DEBUG
		// To debug DX12 leaks, make sure we get a report of objects in use
		DX12Hook::Cleanup();
#endif

        // End Telemetry session
        TelemetryDispatcher::instance()->EndSession();

        // End logger async mode
        IGOLogger::instance()->stopAsyncMode();
    }

    // the IGO message window is only there, because we have to make certain call from the main windows message thread
    // after those calls, the window is destroyed
    static HWND gIGOMessagehWnd = NULL;
}

bool GetParentInfo(DWORD pid, PROCESSENTRY32* pPe)
{
    bool result = false;
    
    if ( pPe )
    {
        ZeroMemory(pPe, sizeof(PROCESSENTRY32));
        pPe->dwSize = sizeof(PROCESSENTRY32);
    }

    if ( pid == 0 )
        pid = GetCurrentProcessId();

	HANDLE	hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	

    if ( hSnapshot != INVALID_HANDLE_VALUE )
    {
        PROCESSENTRY32    pe;
        pe.dwSize = sizeof(pe);

		if ( Process32First( hSnapshot, &pe ) )
		{
			DWORD parentPID = 0;
			do
			{
				if ( parentPID != 0 )
				{
					if ( pe.th32ProcessID == parentPID )
					{
						//Got our parent information!
						if ( pPe )
						{
							memcpy( pPe, &pe, sizeof(pe) );
							result = true;
							break;
						}
					}
				}
				else if ( pe.th32ProcessID == pid )
				{
					//Got it! Save parent PID and restart
					parentPID = pe.th32ParentProcessID;
					
					if ( !Process32First( hSnapshot, &pe ) )
						break;

                    continue;
                }

                ZeroMemory(&pe, sizeof(pe));
                pe.dwSize = sizeof(pe);

            } while ( Process32Next( hSnapshot, &pe ) );
        }

        CloseHandle( hSnapshot );
    }
    else
    {
        IGO_TRACEF("CreateToolhelp32Snapshot failed. Last error: %u", GetLastError());
    }

    return result;
}

bool GetOriginWindowInfo(PROCESSENTRY32* pPe)
{
    bool result = false;
    
    if ( pPe )
    {
        ZeroMemory(pPe, sizeof(PROCESSENTRY32));
        pPe->dwSize = sizeof(PROCESSENTRY32);
    }
    else
        return result;

    HANDLE    hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );

    if ( hSnapshot != INVALID_HANDLE_VALUE )
    {
        PROCESSENTRY32    pe;
        pe.dwSize = sizeof(pe);

        if ( Process32First( hSnapshot, &pe ) )
        {
            do
            {
                // szExeFile is NULL terminated
                if(_tcsstr(pe.szExeFile, _T("Origin.exe")) != NULL)
                {
                    // found the Origin client main window
                    if ( pPe )
                    {
                        memcpy( pPe, &pe, sizeof(pe) );
                        result = true;
                        break;
                    }
                }

                ZeroMemory(&pe, sizeof(pe));
                pe.dwSize = sizeof(pe);

            } while ( Process32Next( hSnapshot, &pe ) );
        }

        CloseHandle( hSnapshot );
    }
    else
    {
        IGO_TRACEF("CreateToolhelp32Snapshot failed. Last error: %u", GetLastError());
    }

    return result;
}
bool IsExludedProcessForHooking(WCHAR *strLowerCurrentProcessName)
{
    // lower case list of processes that we want to exclude from hooking (a game might start one of them!)
    const wchar_t excludedCommonProcesses[][64] = { 
        // common processes, that installer or a game might call
        L"rundll32.exe",
        L"explorer.exe",
        L"regedit.exe",
        L"reg.exe",
        L"csc.exe", // C&C alpha preview starts the C# compiler, which causes a 20 seconds delay for starting the actual game when IGO is injected
        L"regsvr32.exe",
        L"sonarhost.exe", // part of the BF3 browser plugin, but we want to skip it!
        L"gameoverlayui.exe"    // steams overlay process
    };
            
    // lower case list browser processes that we want to exclude from hooking (a game might start one of them!)
    const wchar_t excludedBrowserProcesses[][64] = { 
        // top ten browsers - http://tech.mobiletod.com/top-10-web-browsers-worth-trying/ , that games might call
        L"chrome.exe",        // google chrome
        L"iexplore.exe",    // microsoft IE
        L"firefox.exe",        // firefox
        L"opera.exe",        // opera
        L"safari.exe",        // apple safari
        L"mozilla.exe",        // old mozilla
        L"netscape.exe",    // old netscape
        L"avant.exe",        // avant
        L"deepnet.exe",        // deepnet
        L"maxthon.exe",        // maxthon
        L"phaseout.exe",    // phase out
        L"sbframe.exe",        // slim
        L"sbrowser.exe"        // slim
    };

    // lower case list browser processes that are used to launch plugins
    const wchar_t knownBrowserPluginProcesses[][64] = { 
        L"plugin-container.exe",        // firefox
        L"sbrender.exe",                // slim 
        L"ybrowser.exe",                // avant
        L"webkit2webprocess.exe"        // safari
    };
            
    for (int i=0; i<sizeof(excludedCommonProcesses)/sizeof(excludedCommonProcesses[0]); i++)
    {
        if (wcscmp(strLowerCurrentProcessName, excludedCommonProcesses[i]) == 0)
        {
            //OriginIGO::IGOLogInfo("(0)Not attaching to %S", strLowerCurrentProcessPathName);
            return TRUE;
        }
    }

    for (int i=0; i<sizeof(knownBrowserPluginProcesses)/sizeof(knownBrowserPluginProcesses[0]); i++)
    {
        if (wcscmp(strLowerCurrentProcessName, knownBrowserPluginProcesses[i]) == 0)
        {
            //OriginIGO::IGOLogInfo("(0)Not attaching to %S", strLowerCurrentProcessPathName);
            return TRUE;
        }
    }

    for (int i=0; i<sizeof(excludedBrowserProcesses)/sizeof(excludedBrowserProcesses[0]); i++)
    {
        if (wcscmp(strLowerCurrentProcessName, excludedBrowserProcesses[i]) == 0)
        {
            //OriginIGO::IGOLogInfo("(0)Not attaching to %S", strLowerCurrentProcessPathName);
            return TRUE;
        }
    }

    return FALSE;
}

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    // Don't do anything if we have multiple IGO loaded in the game process and this is not... the ONE!
	if (gIGOAlreadyLoaded)
		return TRUE;

    switch (fdwReason)
    {
    case DLL_THREAD_ATTACH:
        {
        }
        break;

    case DLL_THREAD_DETACH:
        {
			// EBIBUGS-19668: For Guild Wars 2, when the user clicks 'Play' in the launcher window,
			// the thread that executes the 'PresentHook' function gets killed. This prevents the possibility
			// of re-hooking the IGO. So if this is detected, clean up the IGO and try to restart it via
			// IGOCreateThread().
			
			// prevent an infinite loop by storing the current thread id, because IGORendererTimeoutDetectThread will start and end a thread and
			// this fires a DLL_THREAD_DETACH message, so we start again and again and again...
			static DWORD IGORendererTimeoutDetectThreadId = 0;

			DWORD threadId = GetCurrentThreadId();
			if (threadId != IGORendererTimeoutDetectThreadId)
			{
				if((gPresentHookThreadId && (threadId == gPresentHookThreadId)) || gTestCooperativeLevelHook_or_ResetHook_Called || g3rdPartyResetForced)
				{
					// start a thread that will check, if the rendering thread is still called, if so, we do not have to to anything, otherwise, kill & re-init IGO
					if (gIGORendererTimeoutDetectThreadHandle == NULL && !gQuitting && !gExitingProcess)
						gIGORendererTimeoutDetectThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)IGORendererTimeoutDetectThread, gInstDLL, 0, &IGORendererTimeoutDetectThreadId);
				}
			}
        }
        break;
    case DLL_PROCESS_ATTACH:
        {
            gPresentHookThreadId = 0;
            bool isIGOSharedMemoryNew = false;

            // Always turn on logging during injection process
            OriginIGO::IGOLogger::instance()->enableLogging(true);

            // Setup 
            DisasmSetupLogging(ExternalLibLogInfo, ExternalLibLogError);

            // Initial setup for telemetry
            TelemetryDispatcher::instance()->BeginSession();
            
            
			// Check whether we already have a loaded IGO module running...
            WCHAR moduleFileName[_MAX_PATH] = {0};
            if (ExtractModuleFileName(hInstDLL, moduleFileName, _MAX_PATH))
            {
			    gIGOAlreadyLoaded = IsModuleAlreadyLoaded(0, hInstDLL, moduleFileName, NULL, NULL);
			    if (gIGOAlreadyLoaded)
			    {
				    // Don't do anything if we have multiple IGO loaded in the game process and this is not... the ONE!
				    // End Telemetry session
				    TelemetryDispatcher::instance()->EndSession();

				    return TRUE;	// still need SDK to think the loading succeeded - it's ok, as long as the SDK doesn't use the handle returned from the LoadLibrary call.
			    }
            }

            hMapObject = CreateLocalFileMapping("IGOSharedMemFileMap", IGOSharedMemorySize);
            if (hMapObject == NULL || hMapObject == INVALID_HANDLE_VALUE) 
                return FALSE; 
 
            // the first process to attach initializes memory
            isIGOSharedMemoryNew = (GetLastError() != ERROR_ALREADY_EXISTS); 

            // get a pointer to the file-mapped shared memory
            pIGOSharedMemory = (unsigned char*)MapViewOfFile(hMapObject,    // object to map view of
                                                            FILE_MAP_WRITE,        // read/write access
                                                            0,                // high offset:  map from
                                                            0,                // low offset:   beginning
                                                            0);                // default: map entire file
            if (pIGOSharedMemory == NULL) 
                return FALSE; 

            IGO_TRACE("Creating IGO Mutex");
            // create our shared data mutex
            gGlobalIGOApplicationSharedDataMutex = ::CreateMutexW(NULL, FALSE, gGlobalIGOApplicationSharedDataMutexName);
            if (!gGlobalIGOApplicationSharedDataMutex)
            {
                gGlobalIGOApplicationSharedDataMutex = OpenMutexW(SYNCHRONIZE, FALSE, gGlobalIGOApplicationSharedDataMutexName);
            }

            if (!gGlobalIGOApplicationSharedDataMutex)
            {
                IGO_TRACE("Creating IGO Mutex failed.");
                IGO_ASSERT(0);
            }

            // initialize memory if this is the first process
            if (isIGOSharedMemoryNew)
            {
                IGOAutoLock_Mutex oAutoLock(gGlobalIGOApplicationSharedDataMutex, 10000);

                memset(pIGOSharedMemory, 0, IGOSharedMemorySize);

                // In the past we actually had an override in EACore.ini - this was for IGO to work for Origin SDK games, even if the game is not in the catalog - required for DEVs!!!
                // Now we just directly set it since we also need it for automation when running multiple instances / IGO32.dll is automatically loaded when
                // loading the OriginClient.dll from the bootstrap, and we always override it properly afterwards when starting a game, so it shouldn't influence
                // anything
                getIGOConfigurationFlags().gEnabled = true;

                // IGO developer override info from EACore.ini - Not used anymore, but kept the code around in case we need new overrides
                // {
                //      WCHAR currentIGOFolder[_MAX_PATH] = {0};
                //      // get the IGO runtime folder
                //   ::GetModuleFileName(hInstDLL, currentIGOFolder, _MAX_PATH);
                
                //      // remove dll name from folder
                    //wchar_t* strLastSlash = wcsrchr(currentIGOFolder, L'\\');
                    //if (strLastSlash)
                //          strLastSlash[1]=0;

                //      // add ini name
                //      wchar_t iniLocation[256] = {0};
                //      wsprintfW(iniLocation, L"%s%s", currentIGOFolder, L"EACore.ini");

                //      // parse override value
                //      wchar_t forceIGOEnabledFlag[16] = {0};
                //      GetPrivateProfileString(L"IGO", L"EnableIGOForAll", L"0", forceIGOEnabledFlag, 16, iniLocation);
                //      bool forceIGOEnabled = _wtoi(forceIGOEnabledFlag) != 0;

                //      if (forceIGOEnabled)
                //          getIGOConfigurationFlags().gEnabled = true; // this will force IGO to work for Origin SDK games, even if the game is not in the catalog - required for DEVs!!!
                //  }
            }

            // do not allow IGO initialization to happen more than once.
            // DLL_PROCESS_ATTACH can be called multiple times
            if (gStarted)
            {
                OriginIGO::IGOLogInfo("DLL_PROCESS_ATTACH called multiple times!");
                return TRUE;
            }

            gStarted = true;
            gInstDLL = hInstDLL;

            if(!isIGOSharedMemoryNew && !getIGOConfigurationFlags().gEnabled)    // disable IGO if it is not intended to run (skipe the first process aka. Origin.exe)!
            {
                IGO_TRACE("IGO disabled for this process, exiting now!");
                return FALSE;    // return false, so this DLL will be unloaded
            }
            
            IGO_TRACE("Process attach" );

#ifdef _DEBUG
            getIGOConfigurationFlags().gEnabledDebugMenu = true;
            //getIGOConfigurationFlags().gEnabledLogging = true;
#endif

            // set up info display flag
            OriginIGO::IGOInfoDisplay::allowDisplay(getIGOConfigurationFlags().gEnabledDebugMenu);
            OriginIGO::IGOLogInfo("Info Display: %s", getIGOConfigurationFlags().gEnabledDebugMenu ? "enabled" : "disabled");

#ifdef _WIN64
            OriginIGO::IGOLogInfo("64-bit DLL Process attach - %i", IGOSharedMemorySize);
#else
            OriginIGO::IGOLogInfo("32-bit DLL Process attach - %i", IGOSharedMemorySize);
#endif

            PROCESSENTRY32 pe = { 0 };
            pe.dwSize = sizeof(PROCESSENTRY32);
            GetParentInfo(GetCurrentProcessId(), &pe);
            WCHAR strLowerParentProcessName[_MAX_PATH] = {0};
            wcscpy_s(strLowerParentProcessName, _MAX_PATH, pe.szExeFile);
            _wcslwr_s(strLowerParentProcessName, _MAX_PATH);

            // EADM updates are installed via EAProxyInstaller.exe, so do not attach igo32.dll
            if(wcscmp(strLowerParentProcessName, L"eaproxyinstaller.exe") == 0)
                return FALSE; // return false, so this DLL will be unloaded

            // process exceptions list
            WCHAR strExePath[_MAX_PATH] = {0};
            WCHAR strLowerCurrentProcessPathName[_MAX_PATH] = {0};
            WCHAR strLowerCurrentProcessName[_MAX_PATH] = {0};
            ::GetModuleFileName(GetModuleHandle(NULL), strExePath, _MAX_PATH);
            // convert the name to a long file name and make it lower case
            ::GetLongPathName(strExePath, strLowerCurrentProcessPathName, _MAX_PATH);
            _wcslwr_s(strLowerCurrentProcessPathName, _MAX_PATH);

            // remove the path from the process name
            {
                wchar_t* strLastSlash = wcsrchr(strLowerCurrentProcessPathName, L'\\');
                if (strLastSlash)
                    wcscpy_s(strLowerCurrentProcessName, _MAX_PATH, &strLastSlash[1]);
                else
                    wcscpy_s(strLowerCurrentProcessName, _MAX_PATH, strLowerCurrentProcessPathName);
            }

            IGO_TRACEF("parent process name: %S (size %i)", strLowerParentProcessName, wcsnlen_s(strLowerParentProcessName, _MAX_PATH));
            OriginIGO::IGOLogInfo("parent process name: %S (size %i)", strLowerParentProcessName, wcsnlen_s(strLowerParentProcessName, _MAX_PATH));
            
            // if we attached to an exluded process, try to unload "nicely"...
            // NOTE: this may fail!!!
            if (IsExludedProcessForHooking(strLowerCurrentProcessName))
                return FALSE;
           
            // initialize madCHook library
            if (!gHookingInitialized)
            {
                MadCHookLoggers loggers = {NULL, NULL, NULL}; // DISABLED UNTIL WE CHECK THE VALIDITY OF THE LOGGING STATEMENTS - {MadCHookLogWarn, MadCHookLogDebug, NULL}; // VERBOSE IS REALLY TOO VERBOSE! MadCHookLogInfo};
                InitializeMadCHook(&loggers);
                gHookingInitialized = true;
            }

            // OK, we're safe to start logger in async mode
            IGOLogger::instance()->startAsyncMode();

            // here we put all processes that call any of the DLL_EXPORT functionality !!!
            if (wcscmp(strLowerCurrentProcessName, L"originclientservice.exe") == 0 || wcscmp(strLowerCurrentProcessName, L"origin.exe") == 0 || wcscmp(strLowerCurrentProcessName, L"origindebug.exe") == 0
#ifdef _WIN64
                || wcscmp(strLowerCurrentProcessName, L"igoproxy64d.exe")==0 || wcscmp(strLowerCurrentProcessName, L"igoproxy64.exe")==0    // proxy process for starting 64 injection from a 32 bit process
#endif
                )
            {
                return TRUE;    // return true, so this DLL will not be unloaded!
            }

            // And now the telemetry
            TelemetryDispatcher::instance()->RecordSession();

            // Preallocate hook trampolines for all the DLLs already loaded
            PreAllocateHookingTrampolines();

            // install the inject hooks no matter what.
            // children of this process will also have IGO dll loaded
            if (!InjectHook::TryHook())
            {
                IGOInjectThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)IGOInjectThread, gInstDLL, 0, NULL);
                IGO_ASSERT(IGOInjectThreadHandle);
			}


            // here we handle game launchers & legacy runtimes, that should be injected, but should not be able to show the IGO
            // some newer launchers use DirectX, so they would show our IGO - example Tiger Woods 12
            if (wcscmp(strLowerCurrentProcessName, L"twolauncher.exe")==0 || wcscmp(strLowerCurrentProcessName, L"steam.exe") == 0 ||
                wcscmp(strLowerCurrentProcessName, L"pmb.exe")==0 || wcscmp(strLowerCurrentProcessName, L"rads_user_kernel.exe")==0 || wcscmp(strLowerCurrentProcessName, L"lol.launcher.exe")==0 || wcscmp(strLowerCurrentProcessName, L"lolclient.exe")==0 ||
                // legacyPM runtime ...
                wcscmp(strLowerCurrentProcessName, L"eacoreserver.exe")==0 || wcscmp(strLowerCurrentProcessName, L"originlegacycli.exe")==0 || wcscmp(strLowerCurrentProcessName, L"patchprogress.exe")==0)
            {
                return TRUE;    // return true, so this DLL will not be unloaded!
            }

            
            // enable IGO ui/input
            // only the actual game process should ever come to this point!!!
            HOOKAPI_SAFE("ntdll.dll", "LdrLoadDll", LdrLoadDll_Hook);    // catch dlls, that are about to be loaded
            
            DXGIHook::Setup(); // Handles DX10, DX11, soon DX12...

            // try our early hooks as quickly as possible - if they fail, we will retry them in LdrLoadDll_Hook
            HANDLE tmpHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)OffsetCalculationAndEarlyHookingThread, gInstDLL, 0, NULL);
            if (tmpHandle != INVALID_HANDLE_VALUE)
            {
                if (!SetThreadPriority(tmpHandle, THREAD_PRIORITY_HIGHEST))
                    IGOLogWarn("Failed to set thread priority for OffsetCalculationAndEarlyHookingThread");

                CloseHandle(tmpHandle);
            }

            if (gIGOCreateThreadHandle==NULL)
                gIGOCreateThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)IGOCreateThread, gInstDLL, 0, NULL);

            HANDLE hMonThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MonitorTrialProcessThread, gInstDLL, 0, NULL);
            if (hMonThread != INVALID_HANDLE_VALUE)
                CloseHandle(hMonThread);

            HANDLE hSignalThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SDKSignalThread, gInstDLL, 0, NULL);
            if (hSignalThread != INVALID_HANDLE_VALUE)
                CloseHandle(hSignalThread);           
        }
        break;

    case DLL_PROCESS_DETACH:
        {
            // Always turn on logging during cleanup
            OriginIGO::IGOLogger::instance()->enableLogging(true);

			IGO_TRACE("Process detach");
			OriginIGO::IGOLogInfo("DLL Process detach");

            // Only cleanup when we explicitely call the Unload() method. When quitting the process, we may still have our own threads running
            // -> let the system clean itself
            if (!gExitingProcess)
            {
			    // close madCHook library
			    if (gHookingInitialized)
			    {
				    FinalizeMadCHook();
				    gHookingInitialized = false;
			    }

			    // Destroy shared data access mutex
			    if (gGlobalIGOApplicationSharedDataMutex)
			    {
				    // releasemutex/closehandle is not needed, the mutex will be destroyed, once all igo32.dll's are unloaded
				    gGlobalIGOApplicationSharedDataMutex = NULL;
			    }
			    gStarted = false;
			    // unmap shared memory from the process's address space
			    if (pIGOSharedMemory)
				    UnmapViewOfFile(pIGOSharedMemory);
			    pIGOSharedMemory = NULL;
			    // close the process's handle to the file-mapping object
			    if (hMapObject && hMapObject != INVALID_HANDLE_VALUE)
				    CloseHandle(hMapObject);
			    hMapObject = INVALID_HANDLE_VALUE;
            }
        }
        break;
    }

    return TRUE;
}

#elif defined(ORIGIN_MAC) // MAC OSX =============================================================================

#include "IGO.h"
#include "IGOTrace.h"
#include "MacWindows.h"

#include <syslog.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "MacDllMain.h"
#include "IGOSharedStructs.h"
#include <wchar.h>


namespace OriginIGO {

    // create shared data segment
    // see IGOSharedStructs.h for more information
    // do not use simply types here, they will be re-initialized once the IGO dll is loaded into a different architecture(86/64) 
    static uint32_t IGOSharedMemorySize =  sizeof(IGOConfigurationFlags) + 1;
    unsigned char* pIGOSharedMemory = NULL;                // pointer to shared memory
    static HANDLE hMapObject = INVALID_HANDLE_VALUE;    // handle to file mapping

    // shared data accessor
    GameLaunchInfo* gameInfo()
    {
        static GameLaunchInfo info;
        return &info;
    }
    IGOConfigurationFlags &getIGOConfigurationFlags() { return *((IGOConfigurationFlags*)pIGOSharedMemory);}

    static sem_t* gGlobalIGOApplicationSharedDataMutex = NULL;
    static const char* gGlobalIGOApplicationSharedDataMutexName = "IGOApplicationSharedDataMutex";
    static const char* gGlobalIGOApplicationSharedDataName = "IGOSharedMemFileMap";

    static bool gStarted = false;

    bool gInputHooked = false;
    bool gQuitting    = false;
    BYTE gIGOKeyboardData[256] = {0};

}


using namespace OriginIGO;

// Mutex variant for cross process locking
class IGOAutoLock_Mutex
{
public:
    explicit IGOAutoLock_Mutex(sem_t* hMutex, long lTimeOut_ms)
    {
        // FIXME: BIG TIME - COULD WE SAFELY ALLOCATE OUR SEMAPHORE OBJECT IN SHARED MEMORY?
        sem_wait(hMutex);
        mhMutex = hMutex;
    }
    ~IGOAutoLock_Mutex()
    {
        if (mhMutex)
        {
            sem_post(mhMutex);
            mhMutex = NULL;
        }
    }
    bool isLocked() 
    {
        return (mhMutex != NULL); 
    }
    
private:
    sem_t* mhMutex;
};

void EnableIGO(bool enable)
{
    IGOAutoLock_Mutex oAutoLock(gGlobalIGOApplicationSharedDataMutex, 10000);
    getIGOConfigurationFlags().gEnabled = enable;
}

void EnableIGOLog(bool enable)
{
    IGOAutoLock_Mutex oAutoLock(gGlobalIGOApplicationSharedDataMutex, 10000);
    getIGOConfigurationFlags().gEnabledLogging = enable;
}

void EnableIGODebugMenu(bool enable)
{
    IGOAutoLock_Mutex oAutoLock(gGlobalIGOApplicationSharedDataMutex, 10000);
    getIGOConfigurationFlags().gEnabledDebugMenu = enable;
}

bool IsIGOLogEnabled()
{
    IGOAutoLock_Mutex oAutoLock(gGlobalIGOApplicationSharedDataMutex, 10000);
    return getIGOConfigurationFlags().gEnabledLogging;
}

bool IsIGODebugMenuEnabled()
{
    IGOAutoLock_Mutex oAutoLock(gGlobalIGOApplicationSharedDataMutex, 10000);
    return getIGOConfigurationFlags().gEnabledDebugMenu;
}

bool AllocateSharedMemory(bool injected)
{
    if (gStarted)
        return true;
    
    int pageSize = getpagesize();
    if (pageSize > 0)
        IGOSharedMemorySize = ((IGOSharedMemorySize + pageSize - 1) / pageSize) * pageSize;
    
    if (!injected)
    {
        sem_unlink(gGlobalIGOApplicationSharedDataMutexName);  
        shm_unlink(gGlobalIGOApplicationSharedDataName);
    }

    bool isIGOSharedMemoryNew = false;

    int oflag = O_CREAT | O_EXCL | O_RDWR;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    hMapObject = shm_open(gGlobalIGOApplicationSharedDataName, oflag, mode);
    if (hMapObject == INVALID_HANDLE_VALUE)
    {
        if (errno == EEXIST)
        {
            oflag &= ~O_EXCL;
            hMapObject = shm_open(gGlobalIGOApplicationSharedDataName, oflag);
            if (hMapObject == -1)
            {
                IGO_TRACEF("Unable to create/open shared memory (%d)", errno);
                return false;
            }
        }
        
        else
        {
            IGO_TRACEF("Unable to create/open shared memory (%d)", errno);
            return false;            
        }
    }
    
    else
    {
        if (ftruncate(hMapObject, IGOSharedMemorySize) != 0)
        {
            IGO_TRACEF("Unable to resize shared memory appropriately (%d)", errno);
            
            close(hMapObject);
            hMapObject = INVALID_HANDLE_VALUE;
            return false;
        }
        
        isIGOSharedMemoryNew = true;
    }
    
    
    pIGOSharedMemory = (unsigned char*)mmap(NULL, IGOSharedMemorySize, PROT_READ | PROT_WRITE, MAP_SHARED, hMapObject, 0);
    if (pIGOSharedMemory == MAP_FAILED)
    {
        IGO_TRACEF("Unable to map shared memory to process (%d)", errno);
        
        close(hMapObject);
        if (injected)
            shm_unlink(gGlobalIGOApplicationSharedDataName);
        
        hMapObject = INVALID_HANDLE_VALUE;
        
        return false;
    }
    
    
    // create our shared data mutex
    mode = S_IRWXU | S_IRWXG | S_IRWXO;
    gGlobalIGOApplicationSharedDataMutex = sem_open(gGlobalIGOApplicationSharedDataMutexName, O_CREAT, mode, 1);
    if (gGlobalIGOApplicationSharedDataMutex == SEM_FAILED)
    {
        IGO_TRACEF("Unable to allocate/open shared memory semaphore (%d)", errno);
        
        munmap(pIGOSharedMemory, IGOSharedMemorySize);
        close(hMapObject);
        if (injected)
            shm_unlink(gGlobalIGOApplicationSharedDataName);

        hMapObject = INVALID_HANDLE_VALUE;
        pIGOSharedMemory = NULL;
        gGlobalIGOApplicationSharedDataMutex = NULL;
        
        return false;
    }
    
    // initialize memory if this is the first process
    if (isIGOSharedMemoryNew)
    {
        IGOAutoLock_Mutex oAutoLock(gGlobalIGOApplicationSharedDataMutex, 10000);
        memset(pIGOSharedMemory, 0, IGOSharedMemorySize);
        
        // default IGO configuration
#ifdef _DEBUG
        getIGOConfigurationFlags().gEnabledLogging = true;
#endif
    }
    
    gStarted = true;
    
    return true;
}

void FreeSharedMemory(bool injected)
{
    if (gGlobalIGOApplicationSharedDataMutex)
    {
        sem_close(gGlobalIGOApplicationSharedDataMutex);
        
        if (msync(pIGOSharedMemory, IGOSharedMemorySize, MS_SYNC) != 0)
            IGO_TRACEF("Unable to sync shared memory (%d)", errno);
        
        if (munmap(pIGOSharedMemory, IGOSharedMemorySize) != 0)
            IGO_TRACEF("Unable to undo shared memory mapping (%d)", errno);
        
        if (close(hMapObject) != 0)
            IGO_TRACEF("Unable to close shared memory (%d)", errno);
        
        if (!injected)
        {
            if (sem_unlink(gGlobalIGOApplicationSharedDataMutexName) != 0)
                IGO_TRACEF("Unable to unlink the shared memory semaphore (%d)", errno);
            
            if (shm_unlink(gGlobalIGOApplicationSharedDataName) != 0)
                IGO_TRACEF("Unable to unlink the shared memory (%d)", errno);
        }
    }
    
    gStarted = false;
    
    pIGOSharedMemory = NULL;
    hMapObject = INVALID_HANDLE_VALUE;
    gGlobalIGOApplicationSharedDataMutex = NULL;
}

#ifdef INJECTED_CODE

// This part should only happen in the code we inject into the game - however
// The rest of the file contains the interface to the shared memory between the client and the injected code
// -> we want to make that available to the Origin client/IGOLIB

#include "OGLHook.h"
#include "MacInputHook.h"
#include "IGOInfoDisplay.h"
#include <mach-o/dyld.h>
#include "Macmach_override.h"

static IGOLogger* logger = NULL;

__attribute__((constructor))
void OIGLoad()
{
    // Don't inject Origin executable or modules
    char* exePath = NULL;
    uint32_t exePathSize = 0;
    bool partOfOrigin = false;
    
    if (_NSGetExecutablePath(exePath, &exePathSize) < 0)
    {
        exePath = new char[exePathSize];
        if (exePath)
        {
            if (_NSGetExecutablePath(exePath, &exePathSize) == 0)
            {
                char* fileName = strrchr(exePath, '/');
                if (fileName)
                {
                    const char* OriginModulePrefix = "/Origin";
                    partOfOrigin = strncmp(fileName, OriginModulePrefix, strlen(OriginModulePrefix)) == 0;
                    
                    // But allow in SDK test app!
                    const char* OriginSDKKeyword = "SDK";
                    partOfOrigin &= strstr(fileName, OriginSDKKeyword) == NULL;
                }
            }
            
            delete []exePath;
        }
    }

    if (partOfOrigin)
        return;
    
    pthread_setname_np("InjectedCode::DllMain");
    
    IGO_TRACE("InjectedCode - LOADING...");
    SInt32 major = 0;
    SInt32 minor = 0;
    SInt32 bugFix = 0;
    Gestalt(gestaltSystemVersionMajor, &major);
    Gestalt(gestaltSystemVersionMinor, &minor);
    Gestalt(gestaltSystemVersionBugFix, &bugFix);
    IGO_TRACEF("InjectedCode - OS %ld.%ld.%ld", major, minor, bugFix);
    
    // Always enable logging for injection/cleanup
    logger = new OriginIGO::IGOLogger("IGO");
    OriginIGO::IGOLogger::setInstance(logger);
    OriginIGO::IGOLogger::instance()->enableLogging(true);
    
    mach_setup_logging(ExternalLibLogInfo, ExternalLibLogError);
    
    // Start logger in async mode
    OriginIGO::IGOLogger::instance()->startAsyncMode();
    
    // Initial setup for telemetry
    TelemetryDispatcher::instance()->BeginSession();
    
    // TODO: If injected in main module too, determine true/false from module name
    if (AllocateSharedMemory(true))
    {
        // set up logging flag
        bool enableLogging = IsIGOLogEnabled();
        OriginIGO::IGOLogInfo("IGO Logging: %s (%i)", enableLogging ? "enabled" : "disabled", IGOSharedMemorySize);
        
#ifdef _DEBUG
        bool enableDebugMenu = true;
#else
        bool enableDebugMenu = IsIGODebugMenuEnabled();
#endif
        OriginIGO::IGOInfoDisplay::allowDisplay(enableDebugMenu);
        OriginIGO::IGOLogInfo("Info Display: %s", enableDebugMenu ? "enabled" : "disabled");
        
        OriginIGO::IGOLogInfo("OSX %ld.%ld.%ld", major, minor, bugFix);
        
        TelemetryDispatcher::instance()->RecordSession();
        
        OGLHook::TryHook();
    }
    
    else
    {
        TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_FAIL, TelemetryContext_GENERIC, TelemetryRenderer_OPENGL, "Failed to setup shared memory");
    }
    
    IGO_TRACE("InjectedCode - LOADING DONE");
}

__attribute__((destructor))
void OIGUnload()
{
    // Flush/end telemetry
    TelemetryDispatcher::instance()->EndSession();
    
    // Re-enable logging during cleanup
    OriginIGO::IGOLogger::instance()->enableLogging(true);

    IGO_TRACE("InjectedCode - UNLOADING...");
    OriginIGO::IGOLogInfo("DLL Process detach");
    
    // Stop collecting events
    CleanupInputTap();
    
    OGLHook::Cleanup();
    
    FreeSharedMemory(true);

    IGO_TRACE("InjectedCode - UNLOADING DONE");
    
    // End logger async mode
    OriginIGO::IGOLogger::instance()->stopAsyncMode();
    
    OriginIGO::IGOLogger::instance()->setInstance(NULL);
}

#endif

#endif // MAC OSX
