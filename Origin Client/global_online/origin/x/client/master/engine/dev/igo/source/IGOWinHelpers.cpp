//
//  IGOWinHelpers.cpp
//  engine
//
//  Created by Frederic Meraud on 2/24/14.
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//

#include "IGOWinHelpers.h"

#if defined(ORIGIN_PC)

#include <QString>

#include "IGO/HookAPI.h"
#include "IGOController.h"
#include "IGOWindowManagerIPC.h"
#include "services/log/LogService.h"
#include "services/platform/PlatformService.h"


namespace Origin
{
namespace Engine
{
    // Global flash plugin configuration filename
    static const QString GLOBAL_FLASH_PLUGIN_CONFIG_FILENAME("mms.cfg");

    // Local/tmp flash plugin configuration filename - don't use the same as the original config filename since that's our only key to detect the 
    // loading process -> otherwise we would get into a lock situation or infinite loop (depending on the mutex config)
    static const QString LOCAL_FLASH_PLUGIN_CONFIG_FILENAME("tmpmms.cfg");

    // Additional entries we want in the Flash configuration file (right now disable fullscreen option)
    static const QString FLASH_OVERRIDE_SETTINGS("\nFullScreenDisable=1\nFullScreenInteractiveDisable=1\n");

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Win32 windows hooks to prevent QT win32 windows from switching into the foreground and deactivating the game process
DEFINE_HOOK_SAFE(HRESULT, DoDragDropHook, (LPDATAOBJECT pDataObj, LPDROPSOURCE pDropSourc, DWORD dwOKEffects, LPDWORD pdwEffect))

    IGO_TRACE("DoDragDropHook");
    if(Engine::IGOController::instance()->isGameInForeground())
        return DRAGDROP_S_CANCEL;
    else
        return DoDragDropHookNext(pDataObj, pDropSourc, dwOKEffects, pdwEffect);
}

#if 0
DEFINE_HOOK_SAFE(HWND, GetForegroundWindowHook, (void))

    IGO_TRACE("GetForegroundWindowHook");
    if(Engine::IGOController::instance()->isGameInForeground())
        return 0;
    else
        return GetForegroundWindowHookNext();
}
#endif

DEFINE_HOOK_SAFE(BOOL, SetForegroundWindowHook, (HWND hWnd))

    IGO_TRACE("SetForegroundWindowHook");
    if(Engine::IGOController::instance()->isGameInForeground())
        return TRUE;
    else
    {
        if( SetForegroundWindowHookNext )
            return SetForegroundWindowHookNext(hWnd);
        
        return FALSE;
    }
}

DEFINE_HOOK_SAFE(BOOL, BringWindowToTopHook, (HWND hWnd))

    IGO_TRACE("BringWindowToTopHook");
    if(Engine::IGOController::instance()->isGameInForeground())
        return TRUE;
    else
    {
        if( BringWindowToTopHookNext )
            return BringWindowToTopHookNext(hWnd);
        
        return FALSE;
    }
}

DEFINE_HOOK_SAFE_VOID(SwitchToThisWindowHook, (HWND hWnd, BOOL fUnknown))

    IGO_TRACE("SwitchToThisWindowHook");
    if(!Engine::IGOController::instance()->isGameInForeground())
    {
        if( SwitchToThisWindowHookNext )
            SwitchToThisWindowHookNext(hWnd, fUnknown);
    }
}


DEFINE_HOOK_SAFE(BOOL, ShowWindowAsyncHook, (HWND hWnd, int nCmdShow))

    IGO_TRACEF("ShowWindowAsyncHook %x", nCmdShow);
    int newCmd=nCmdShow;
    if(Engine::IGOController::instance()->isGameInForeground())
    {
        switch(nCmdShow)
        {
            case SW_HIDE             :
                break;
            case SW_SHOWNORMAL       :
                newCmd=SW_SHOWNOACTIVATE;
                break;
            case SW_SHOWMINIMIZED    :
                break;
            case SW_SHOWMAXIMIZED    :
                break;
            case SW_SHOWNOACTIVATE   :
                break;
            case SW_SHOW             :
                newCmd=SW_SHOWNA;
                break;
            case SW_MINIMIZE         :
                break;
            case SW_SHOWMINNOACTIVE  :
                break;
            case SW_SHOWNA           :
                break;
            case SW_RESTORE          :
                break;
            case SW_SHOWDEFAULT      :
                newCmd=SW_SHOWNOACTIVATE;
                break;
            case SW_FORCEMINIMIZE    :
                break;
        }
    }
    
    if( ShowWindowAsyncHookNext )
        return ShowWindowAsyncHookNext(hWnd, newCmd);
    
    return FALSE;
}

DEFINE_HOOK_SAFE(BOOL, ShowWindowHook, (HWND hWnd, int nCmdShow))

    IGO_TRACEF("ShowWindowHook %x", nCmdShow);
    int newCmd=nCmdShow;
    if(Engine::IGOController::instance()->isGameInForeground())
    {
        switch(nCmdShow)
        {
            case SW_HIDE             :
                break;
            case SW_SHOWNORMAL       :
                newCmd=SW_SHOWNOACTIVATE;
                break;
            case SW_SHOWMINIMIZED    :
                break;
            case SW_SHOWMAXIMIZED    :
                break;
            case SW_SHOWNOACTIVATE   :
                break;
            case SW_SHOW             :
                newCmd=SW_SHOWNA;
                break;
            case SW_MINIMIZE         :
                break;
            case SW_SHOWMINNOACTIVE  :
                break;
            case SW_SHOWNA           :
                break;
            case SW_RESTORE          :
                break;
            case SW_SHOWDEFAULT      :
                newCmd=SW_SHOWNOACTIVATE;
                break;
            case SW_FORCEMINIMIZE    :
                break;
        }
    }
    
    if( ShowWindowHookNext )
        return ShowWindowHookNext(hWnd, newCmd);
    
    return FALSE;
}

DEFINE_HOOK_SAFE(HWND, SetFocusHook, (HWND hWnd))

    if(Engine::IGOController::instance()->isGameInForeground())
        return 0;
    
    if( SetFocusHookNext )
        return SetFocusHookNext(hWnd);
    
    return 0;
}

#if 0
DEFINE_HOOK_SAFE(BOOL, PeekMessageAHook, (LPMSG lpMsg,
                                     HWND hWnd,
                                     UINT wMsgFilterMin,
                                     UINT wMsgFilterMax,
                                     UINT wRemoveMsg))

    BOOL s=PeekMessageAHookNext(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
    
    if(s && Engine::IGOController::instance()->isGameInForeground() && lpMsg && lpMsg->message==WM_CAPTURECHANGED)
        lpMsg->message = 0;
    return s;
}

DEFINE_HOOK_SAFE(BOOL, PeekMessageWHook, (LPMSG lpMsg,
                                     HWND hWnd,
                                     UINT wMsgFilterMin,
                                     UINT wMsgFilterMax,
                                     UINT wRemoveMsg))

    BOOL s=PeekMessageWHookNext(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
    
    if(s && Engine::IGOController::instance()->isGameInForeground() && lpMsg && lpMsg->message==WM_CAPTURECHANGED)
        lpMsg->message = 0;
    
    return s;
}
#endif

DEFINE_HOOK_SAFE(BOOL, SetWindowPosHook, (HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags))

    
    IGO_TRACEF("SetWindowPosHook %x", uFlags);
    UINT myFlags=uFlags;
    if(Engine::IGOController::instance()->isGameInForeground())
    {
        myFlags=myFlags|SWP_NOACTIVATE|SWP_NOSENDCHANGING|SWP_NOZORDER|SWP_NOOWNERZORDER;
    }
    if( SetWindowPosHookNext )
        return SetWindowPosHookNext(hWnd, hWndInsertAfter, X, Y, cx, cy, myFlags);
    
    return FALSE;
}

DEFINE_HOOK_SAFE(HWND, SetActiveWindowHook, (HWND hWnd))

    IGO_TRACE("SetActiveWindowHook");
    if(Engine::IGOController::instance()->isGameInForeground())
        return 0;
    
    if( SetActiveWindowHookNext )
        return SetActiveWindowHookNext(hWnd);
    
    return 0;
}

#if 0
DEFINE_HOOK_SAFE(HWND, GetActiveWindowHook, (void))

    IGO_TRACE("GetActiveWindowHook");
    if(Engine::IGOController::instance()->isGameInForeground())
        return 0;
    
    return GetActiveWindowHookNext();
}
#endif

// Fwd decl
QString CreateTemporaryFlashConfigurationFile(const QString& originalFileName, LPCWSTR lpFileName);
// we have to use DEFINE_HOOK_SAFE_NO_CODE_CHECK instead of DEFINE_HOOK_SAFE to avoud a recursive CreateFileW calls, caused by "CheckHook" code

DEFINE_HOOK_SAFE_NO_CODE_CHECK(HANDLE, CreateFileWHook, (LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes,
            DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile))

    // Are we trying to load the Flash configuration file? I'm only using the filename right now because I don't know how consistent the location is through
    // the different OS versions and Flash plugin versions... hopefully this is good enough!
    if (lpFileName)
    {
        QFileInfo fileInfo(QString::fromWCharArray(lpFileName));
        if (fileInfo.fileName().compare(GLOBAL_FLASH_PLUGIN_CONFIG_FILENAME, Qt::CaseInsensitive) == 0)
        {
            // Btw, we only want to disable fullscreen while in IGO - btw here we'd better be safe and check whether the controller was instantiated
            // (just don't want to create its instance here)
            if (IGOController::instantiated() && IGOController::instance()->isActive())
            {
                // Create our own version of the configuration file that is going to disable fullscreen functionality
                QString newFileName = CreateTemporaryFlashConfigurationFile(fileInfo.absoluteFilePath(), lpFileName);
                if (!newFileName.isEmpty())
                {
                    LPTSTR fileName = new TCHAR[newFileName.length() +1];
                    int fileNameLen = newFileName.toWCharArray(fileName);
                    fileName[fileNameLen] = 0;

                    HANDLE fileHandle = CreateFileWHookNext(fileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
                    delete[] fileName;

                    return fileHandle;
                }
            }
        }
    }

    return CreateFileWHookNext(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

// Create a duplicate of the Flash global configuration file
QString CreateTemporaryFlashConfigurationFile(const QString& originalFileName, LPCWSTR lpFileName)
{
    static QMutex mutex;
    static QString tempFlashFileName;
    static int isInitializedSuccessfully = 0;

    QMutexLocker locker(&mutex);
    if (isInitializedSuccessfully == 0)
    {
        // Assume we did try but failed...
        isInitializedSuccessfully = -1; 

        QString originalContent;

        // Read content of original config file - unfortunately we can't use Qt here since we don't want to re-enter our own hook!!
        // If we fail to properly read the config file we'll just only use our own settings
        HANDLE fileHandle = CreateFileWHookNext(lpFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (fileHandle != INVALID_HANDLE_VALUE)
        {
            DWORD fileSize = GetFileSize(fileHandle, NULL);
            if (fileSize != INVALID_FILE_SIZE)
            {
                char* buffer = new char[fileSize + 1];
                memset(buffer, 0, sizeof(char) * (fileSize + 1));
                DWORD bytesRead = 0;

                BOOL contentRead = ReadFile(fileHandle, buffer, fileSize, &bytesRead, NULL);
                if (contentRead != FALSE && bytesRead == fileSize)
                {
                    originalContent = QString::fromUtf8(QByteArray(buffer));
                    ORIGIN_LOG_DEBUG << "Flash configuration file content:\n" << originalContent;
                }

                else
                    ORIGIN_LOG_ERROR << "Failed to read the content of flash configuration file '" << originalFileName << "' errCode = " << GetLastError();

                delete[] buffer;
            }

            else
                ORIGIN_LOG_ERROR << "Unable to retrieve size of flash configuration file '" << originalFileName << "' errCode = " << GetLastError();

            CloseHandle(fileHandle);
        }

        else
            ORIGIN_LOG_ERROR << "Unable to open flash configuration file '" << originalFileName << "' errCode = " << GetLastError();

        
        // Append our settings at the end of the file - they will overwrite any previous settting
        originalContent.append(FLASH_OVERRIDE_SETTINGS);

        // Save our temporary new file
        QString newFileName = Origin::Services::PlatformService::commonAppDataPath() + LOCAL_FLASH_PLUGIN_CONFIG_FILENAME;
        QFile newFile(newFileName);
        if (newFile.open(QFile::WriteOnly | QFile::Text | QFile::Truncate))
        {
            QTextStream newText(&newFile);
            newText << originalContent;
            newText.flush();

            if (newText.status() == QTextStream::Ok)
            {
                isInitializedSuccessfully = 1;
                tempFlashFileName = newFileName;
            }

            else
                ORIGIN_LOG_ERROR << "Error writing tmp flash configuration file '" << newFileName << "' errCode = " << newText.status(); 
        }

        else
            ORIGIN_LOG_ERROR << "Unable to open tmp flash configuration file '" << newFileName << "'";
    }
    
    return tempFlashFileName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HMODULE LoadIGODll()
{
    const WCHAR *igoDLL =
#ifdef _DEBUG
    L"igo32d.dll";
#else
    L"igo32.dll";
#endif
    
    HMODULE hIGO = NULL;
    if (!(hIGO = GetModuleHandle(igoDLL)))
    {
        hIGO = LoadLibrary(igoDLL);
    }
    
    return hIGO;
}

void UnloadIGODll()
{
#ifdef ORIGIN_PC
#ifdef DEBUG
    const QString igoName = "igo32d.dll";
#else
    const QString igoName = "igo32.dll";
#endif
    
    HMODULE hIGO = GetModuleHandle(igoName.utf16());
    if (!hIGO)
        hIGO = LoadLibrary(igoName.utf16());
    
    if (hIGO)
    {
        typedef void (*UnloadIGOFunc)(void);
        UnloadIGOFunc UnloadIGO = (UnloadIGOFunc)GetProcAddress(hIGO, "Unload");
        if (UnloadIGO)
        {
            UnloadIGO();
        }
    }
#endif
}

void HookWin32()
{
    ORIGIN_LOG_EVENT << "Hooking IGO.";
    
    HOOKAPI_SAFE("ole32.dll", "DoDragDrop", DoDragDropHook);
    
    //HOOKAPI_SAFE("user32.dll", "PeekMessageA", PeekMessageAHook);
    //HOOKAPI_SAFE("user32.dll", "PeekMessageW", PeekMessageWHook);
    
    HOOKAPI_SAFE("user32.dll", "SetFocus", SetFocusHook);
    HOOKAPI_SAFE("user32.dll", "SetForegroundWindow", SetForegroundWindowHook);
    //HOOKAPI_SAFE("user32.dll", "GetForegroundWindow", GetForegroundWindowHook);
    
    HOOKAPI_SAFE("user32.dll", "BringWindowToTop", BringWindowToTopHook);
    HOOKAPI_SAFE("user32.dll", "SwitchToThisWindow", SwitchToThisWindowHook);
    HOOKAPI_SAFE("user32.dll", "ShowWindowAsync", ShowWindowAsyncHook);
    HOOKAPI_SAFE("user32.dll", "ShowWindow", ShowWindowHook);
    HOOKAPI_SAFE("user32.dll", "SetWindowPos", SetWindowPosHook);
    
    HOOKAPI_SAFE("user32.dll", "SetActiveWindow", SetActiveWindowHook);
    //HOOKAPI_SAFE("user32.dll", "GetActiveWindow", GetActiveWindowHook);
    
    HOOKAPI_SAFE("kernel32.dll", "CreateFileW", CreateFileWHook);

    ORIGIN_LOG_EVENT << "Successfully hooked IGO.";
}

void UnhookWin32()
{
    ORIGIN_LOG_EVENT << "Unhooking IGO.";
    UNHOOK_SAFE(DoDragDropHook);
    //UNHOOK_SAFE(PeekMessageAHook);
    //UNHOOK_SAFE(PeekMessageWHook);
    UNHOOK_SAFE(SetFocusHook);
    UNHOOK_SAFE(SetForegroundWindowHook);
    //UNHOOK_SAFE(GetForegroundWindowHook);
    UNHOOK_SAFE(BringWindowToTopHook);
    UNHOOK_SAFE(SwitchToThisWindowHook);
    UNHOOK_SAFE(ShowWindowAsyncHook);
    UNHOOK_SAFE(ShowWindowHook);
    UNHOOK_SAFE(SetWindowPosHook);
    UNHOOK_SAFE(SetActiveWindowHook);
    //UNHOOK_SAFE(GetActiveWindowHook);
    
    ORIGIN_LOG_EVENT << "Successfully unhooked IGO.";
}

}
}

#endif // ORIGIN_PC