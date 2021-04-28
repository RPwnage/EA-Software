///////////////////////////////////////////////////////////////////////////////
// InputHook.cpp
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////
#include "IGO.h"

#if defined(ORIGIN_PC)

#include "InputHook.h"
#include "mantlehook.h"
#include "dx11hook.h"
#include "dx10hook.h"
#include "dx9hook.h"
#include "dx8hook.h"
#include "oglhook.h"

#include <Windows.h>
#include <CommCtrl.h>

#include "IGOApplication.h"
#include "IGOIPC/IGOIPC.h"
#include "IGOLogger.h"
#include "IGOTelemetry.h"
#include "Helpers.h"

#include "HookApi.h"
#include "EASTL/functional.h"
#include "EASTL/hash_map.h"

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#pragma comment (lib, "dxguid.lib")

#pragma optimize( "", off )

extern HWND getProcMainWindow(DWORD PID);

namespace OriginIGO {

    extern void IGOCleanup();
    extern void IGOReInit();

    HANDLE InputHook::mIGOInputhookDestroyThreadHandle = NULL;
    HANDLE InputHook::mIGOInputhookCreateThreadHandle = NULL;
    int volatile InputHook::mInputHookState = 0;
    bool volatile InputHook::mWindowHooked = false;
    HHOOK mPreWndProcObserverHandle = NULL;
    HHOOK mPostWndProcObserverHandle = NULL;
    static bool gNoMoreFiltering = false;
    POINT gSavedMousePos = { 0 };
    int gShowCursorCount = 0;
    HCURSOR gPrevCursor = 0;
    RECT gPrevClipRect = { 0 };
    EA::Thread::Futex InputHook::mInstanceHookMutex;
    EA::Thread::Futex InputHook::mInstanceCleanupMutex;
    static POINT gMousePos;
    extern HWND gHwnd;
    extern BYTE gIGOKeyboardData[256];
    extern HINSTANCE gInstDLL;
    extern bool gInputHooked;
    extern volatile bool gQuitting;
    extern DWORD gPresentHookThreadId;
    extern HANDLE gIGOCreateThreadHandle;
    extern HANDLE gIGODestroyThreadHandle;
    extern bool g3rdPartyResetForced;
    bool gNonUnicodeCallWasUsed = false;
    LPARAM gLastWM_INPUT_param = 0;
    WPARAM gLastWM_INPUT_device = 0;
    static bool gWin32Hooked = false;
    static bool gDInputHooked = false;
    static bool gRawMouse = false;
    static bool gRawKeyboard = false;
    static DWORD gRawMouseState = 0;
    static DWORD gRawKeyboardState = 0;
    // this code is currently not needed, because we make sure to call IGOApplication::toggleIGO() only from the main win32 message thread!!!
    //DWORD gIGOShowCursor = 0;
    DWORD gIGOUpdateMessageTime = 0;
    DWORD gIGOUpdateMessage = 0;
    DWORD gIGOHookDX8UpdateMessage = 0;
    DWORD gIGOHookDX9UpdateMessage = 0;
    DWORD gIGOHookGLUpdateMessage = 0;
    DWORD gIGOHookMantleUpdateMessage = 0;
    DWORD gIGO3rdPartyCheckMessage = 0;
    DWORD gIGOSwitchGameToBackground = 0;
    DWORD gIGOKeyUpMessage_WM_INPUT = 0;
    DWORD gIGOKeyUpMessage_WM_KEYUP = 0;
    DWORD gIGOHookWindowMessage = 0;

    static DWORD gLast3rdPartyCheck = 0;
    static DWORD gLastInputHookAttempt = 0;
    static DWORD gLastKeyboardSync = 0;
    static DWORD gLastLButtonUp = 0;
    static DWORD gLastRButtonUp = 0;

    static bool gCurrentLButtonDown = 0;
    static bool gCurrentMButtonDown = 0;
    static bool gCurrentRButtonDown = 0;

    static bool gCurrentNCLButtonDown = 0;
    static bool gCurrentNCMButtonDown = 0;
    static bool gCurrentNCRButtonDown = 0;

    // **************************extra keyboard and mouse functions*************************************

    bool filterMessages(LPMSG lpMsg, bool discardKeyboardData = false, bool autoCompleteKeyEvents = false);

    MemoryUnProtector::MemoryUnProtector(LPVOID address, DWORD size)
        :
        mCallSucceeded(false),
        mProtectionState(0),
        mAddress(address),
        mSize(size)
    {
        mCallSucceeded = VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &mProtectionState) == TRUE;
        if (!mCallSucceeded)
            IGO_TRACEF("ProtectMemory failed: %i\n", mProtectionState);
    }

    MemoryUnProtector::~MemoryUnProtector()
    {
        if (mCallSucceeded)
        {
            DWORD dummy = 0;
            mCallSucceeded = VirtualProtect(mAddress, mSize, mProtectionState, &dummy) == TRUE;
            if (!mCallSucceeded)
                IGO_TRACEF("UnprotectMemory failed: %i\n", mProtectionState);
            mCallSucceeded = false;
        }
    }

	// We may have games with windows dedicated to keyboard input (ie Osu!) - apply a few "funky heuristics" in hope we can properly determine whether we need to handle the 
    // keyboard info / limit the damage for other supported games
    void PrepareKeyboardInputFiltering(HWND renderWnd, HWND msgWnd, UINT messageType, bool* discardKeyboardData, bool* autoCompleteKeyEvents)
    {
        *discardKeyboardData = msgWnd != gHwnd;
        *autoCompleteKeyEvents = false;
        if (*discardKeyboardData && msgWnd && gHwnd)
        {
            // 1) Is the main render window active but the other one has keyboard focus?
            if (GetActiveWindow() == renderWnd && GetFocus() == msgWnd)
            {
                // 2) Here we are only worried about keyboard-related messages... aren't we?
                if (messageType == WM_DEADCHAR || messageType == WM_SYSDEADCHAR || messageType == WM_SYSCHAR
                    || messageType == WM_CHAR || messageType == WM_SYSKEYDOWN || messageType == WM_KEYDOWN
                    || messageType == WM_SYSKEYUP || messageType == WM_KEYUP
    #if(_WIN32_WINNT >= 0x0501)        
                    || messageType == WM_UNICHAR
    #endif
                )
                {
                    // 3) Is the focus window invisible? Unfortunately this could get quite expensive
                    // Extra things that don't apply to our current test case (Osu!) but that we may need to do (skipping them until there's a game out there that needs it!)
                    // - Use GetLayeredWindowAttributes to check window alpha
                    // - Use GetWindowRect to get window rect, and then see if on any of monitors (but then we would have to enumerate displays with EnumMonitors/cache that info + add code
                    // to detect setup changes)

                    // Easy enough to check whether the WS_VISIBLE style is set - but most likely useless - don't even know if a "hidden" window can get keyboard focus
                    BOOL hidden = !IsWindowVisible(msgWnd);

                    // In the case of Osu! the keyboard window has the CS_PARENTDC class style set, and one of its parent's height is 0 -> invisible!
                    // Interestingly enough, doing a GetWindowRect() on the window returns a non-empty bbox but located way outside the monitor extends. However as previously
                    // mentioned, doing that we quite expensive, especially because this whole test is called from the 'GetMessage'/'PeekMessage' event processing loop.
                    wchar_t className[128] = {0};
                    bool parentClamping = false;
                    if (GetClassName(msgWnd, className, _countof(className)) > 0)
                    {
                        static HMODULE igoModule = NULL;
                        if (!igoModule)
                            GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT , reinterpret_cast<LPCWSTR>(&OriginIGO::PrepareKeyboardInputFiltering), &igoModule);

                        WNDCLASS classInfo;
                        if (GetClassInfo(igoModule, className, &classInfo))
                            parentClamping = (classInfo.style & CS_PARENTDC) != 0;
                    }

                    // Check whether this is a child window to the main window we hooked up - if so, still forward input keys (a game that needs this is OSU!)
                    HWND parent = msgWnd;
                    while ((parent = GetAncestor(parent, GA_PARENT)) != NULL)
                    {
                        if (!hidden && parentClamping)
                        {
                            RECT rect;
                            if (GetWindowRect(parent, &rect))
                            {
                                if (((rect.right - rect.left) <= 0) || ((rect.bottom - rect.top) <= 0))
                                    hidden = true;
                            }
                        }

                        if (parent == gHwnd)
                        {
                            if (hidden)
                            {
                                *discardKeyboardData = false;
                                *autoCompleteKeyEvents = true;
                            }

                            break;
                        }
                    }
                }
            }
        }
    }

    /*
    static LRESULT CALLBACK RawInputMessageHookFunc(int nCode, WPARAM wParam, LPARAM lParam )
    {
    if (nCode<0)
    return CallNextHookEx(gMessageHook, nCode, wParam, lParam);
    else
    {
    MSG *lpMsg = (MSG *)lParam;


    MemoryUnProtector p(lpMsg, sizeof(MSG));

    if( p.isOkay() && filterMessages(lpMsg, lpMsg->hwnd!=gHwnd) )
    {
    lpMsg->message=0;
    }

    return CallNextHookEx(gMessageHook, nCode, wParam, lParam);
    }
    }
    */


    BOOL CALLBACK FindOriginWindowCallback(HWND hWnd, LPARAM lParam)
    {
        if (!hWnd || !lParam)
            return TRUE;
        if (!::IsWindowVisible(hWnd))    // must be a visible window!!!
            return TRUE;

        wchar_t title[32] = { 0 };
        wchar_t className[32] = { 0 };
        GetWindowText(hWnd, title, _countof(title));
        GetClassName(hWnd, className, _countof(className));
        if (wcscmp(title, L"Origin") == 0 && wcscmp(className, L"QWidget") == 0)
        {
            *((HWND*)lParam) = hWnd;
            return FALSE;
        }

        return TRUE;
    }


    HWND FindOriginWindow()
    {
        HWND window = 0;

        ::EnumWindows(FindOriginWindowCallback, (LPARAM)&window);
        return window;
    }

    static DWORD WINAPI ReHookWaitThread(LPVOID *lpParam)
    {
        OriginIGO::IGOLogInfo("Starting ReHookWaitThread...");
        Sleep(5000);    // wait 5 seconds before we end, so we give 3rd party apps a chance to rehook before the IGO does!!!
        g3rdPartyResetForced = true;
        return 0;
    }

    LRESULT CALLBACK PreWndProcObserverFunc(int code, WPARAM wParam, LPARAM lParam)
    {
        if (code)
            return CallNextHookEx(NULL, code, wParam, lParam);

        HWND watchedWnd = gHwnd;

        LPCWPSTRUCT msg = reinterpret_cast<LPCWPSTRUCT>(lParam);
        switch (code)
        {
            case HC_ACTION:
                if (watchedWnd && msg->hwnd == watchedWnd)
                {
                    // Cleanup if we needed.
                    if ((msg->message == WM_NCDESTROY || msg->message == WM_DESTROY || msg->message == WM_QUIT))
                    {
                        IGOLogWarn("IGOMessageProcHookNext (IGO instance active) WM_NCDESTROY / WM_DESTROY / WM_QUIT received.");

                        gNoMoreFiltering = true;
                        IGOCleanup();
                        IGOReInit();
                    }

                    else
                    if (IGOApplication::instance())
                    {
                        SAFE_CALL_LOCK_ENTER
                            if (!SAFE_CALL(IGOApplication::instance(), &IGOApplication::hasFocus))
                            {
                                if (!SAFE_CALL(IGOApplication::instance(), &IGOApplication::checkPipe))// if the repair attempt failed, deactivate IGO (usually this is done via handleDisconnect, but we want to be sure, so it here too)
                                {
                                    if (SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive))    // deactivate IGO
                                    {
                                        SAFE_CALL(IGOApplication::instance(), &IGOApplication::toggleIGO);
                                    }
                                }
                            }

                            if (SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive) || SAFE_CALL(IGOApplication::instance(), &IGOApplication::isHotKeyPressed))    // if IGO is not activated or the IGO featureing game is not active(in foreground), do not filter WM messages!!!
                            {
                                switch (msg->message)
                                {
                                    case WM_INPUTLANGCHANGE:
                                    {
                                        SAFE_CALL(IGOApplication::instance(), &IGOApplication::setIGOKeyboardLayout, (HKL)lParam);
                                    }
                                    break;

                                    case WM_ACTIVATE:
                                    {
                                        if (LOWORD(msg->wParam) == WA_INACTIVE)
                                        {
                                            if (SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive))    // deactivate IGO
                                            {
                                                SAFE_CALL(IGOApplication::instance(), &IGOApplication::toggleIGO);
                                                SAFE_CALL(IGOApplication::instance(), &IGOApplication::processPendingIGOToggle);
                                            }
                                        }
                                    }
                                    break;

                                    case WM_ACTIVATEAPP:
                                    {
                                        if (msg->wParam == FALSE)
                                        {
                                            if (SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive))    // deactivate IGO
                                            {
                                                SAFE_CALL(IGOApplication::instance(), &IGOApplication::toggleIGO);
                                                SAFE_CALL(IGOApplication::instance(), &IGOApplication::processPendingIGOToggle);
                                            }
                                        }
                                    }
                                    break;
                                } // switch message
                            }
                        SAFE_CALL_LOCK_LEAVE
                    }
                }
                break;

            default:break;
        }

        return CallNextHookEx(NULL, code, wParam, lParam);
    }

    LRESULT CALLBACK PostWndProcObserverFunc(int code, WPARAM wParam, LPARAM lParam)
    {
        if (code)
            return CallNextHookEx(NULL, code, wParam, lParam);

        HWND watchedWnd = gHwnd;

        LPCWPRETSTRUCT msg = reinterpret_cast<LPCWPRETSTRUCT>(lParam);
        switch (code)
        {
            case HC_ACTION:
                if (watchedWnd && msg->hwnd == watchedWnd)
                {
                    if (IGOApplication::instance())
                    {
                        SAFE_CALL_LOCK_ENTER
                            if (SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive) || SAFE_CALL(IGOApplication::instance(), &IGOApplication::isHotKeyPressed))    // if IGO is not activated or the IGO featureing game is not active(in foreground), do not filter WM messages!!!
                            {
                                switch (msg->message)
                                {
                                    case WM_SETCURSOR:
                                        // we put this in the post-processing in case the message was called internally/from a SendMessage -> we can't stop it, but we can immediately override the
                                        // cursor if changed by Windows
                                        SAFE_CALL(IGOApplication::instance(), &IGOApplication::updateCursor, IGOIPC::CURSOR_UNKNOWN);
                                        break;
                                }
                            }
                        SAFE_CALL_LOCK_LEAVE
                    }
                }
                break;

            default:break;
        }

        return CallNextHookEx(NULL, code, wParam, lParam);
    }

    bool IGOMessageDispatchFilter(HWND hwnd, UINT* uMsgPtr, WPARAM* wParamPtr, LPARAM* lParamPtr)
    {
        UINT uMsg = *uMsgPtr;
        WPARAM wParam = *wParamPtr;
        LPARAM lParam = *lParamPtr;

        // Don't filter if the window is getting destroyed or if this is not the main window we are supposed to handle
        if (gNoMoreFiltering || (gHwnd != NULL && gHwnd != hwnd))
            return false;

        // don't spam the system with this task, execute it every 15 seconds only!!!
        DWORD now = GetTickCount();
        if ((now - gLast3rdPartyCheck >= REHOOKCHECK_DELAY) && !gQuitting)
        {
            OriginIGO::IGOLogDebug("ReHookWaitThread");
            gLast3rdPartyCheck = now;

            // this will cause a DLL_THREAD_DETACH which initiates IGORendererTimeoutDetectThread
            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReHookWaitThread, gInstDLL, 0, NULL);
        }

#ifndef _WIN64
        if (uMsg == gIGOHookDX8UpdateMessage)
        {
            OriginIGO::IGOLogDebug("gIGOHookDX8UpdateMessage");
            DX8Hook::TryHook();
            return true;
        }
#endif
        if (uMsg == gIGOHookDX9UpdateMessage)
        {
            OriginIGO::IGOLogDebug("gIGOHookDX9UpdateMessage");
            DX9Hook::TryHook();
            return true;
        }

        if (uMsg == gIGOHookGLUpdateMessage)
        {
            OriginIGO::IGOLogDebug("gIGOHookGLUpdateMessage");
            OGLHook::TryHook();
            return true;
        }

        if (IGOApplication::instance())
        {
            // perform pending toggles
            SAFE_CALL_LOCK_ENTER
                SAFE_CALL(IGOApplication::instance(), &IGOApplication::processPendingIGOToggle);
            SAFE_CALL_LOCK_LEAVE

            if (uMsg == gIGOSwitchGameToBackground)
            {
                OriginIGO::IGOLogDebug("gIGOSwitchGameToBackground");
                SAFE_CALL_LOCK_ENTER
                    if (SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive))    // de-deactivate IGO
                    {
                        SAFE_CALL(IGOApplication::instance(), &IGOApplication::toggleIGO);
                        // perform pending toggles
                        SAFE_CALL(IGOApplication::instance(), &IGOApplication::processPendingIGOToggle);
                    }
                SAFE_CALL_LOCK_LEAVE

                // we switch to Origin to force the game behave like an alt+tab occured
                LockSetForegroundWindow(LSFW_UNLOCK);
                HWND hwnd = FindOriginWindow();
                if (IsWindowUnicode(hwnd))
                    SendMessageTimeoutW(hwnd, WM_SYSCOMMAND, SC_RESTORE, SC_RESTORE, SMTO_BLOCK, 3000U, NULL);
                else
                    SendMessageTimeoutA(hwnd, WM_SYSCOMMAND, SC_RESTORE, SC_RESTORE, SMTO_BLOCK, 3000U, NULL);
                SetForegroundWindow(hwnd);

                return true;    // do not forward gIGOSwitchGameToBackground message to the game
            }

            SAFE_CALL_LOCK_ENTER
                if (!SAFE_CALL(IGOApplication::instance(), &IGOApplication::isOriginAlive))    // deactivate IGO, if Origin is gone
                {
                    if (SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive))
                    {
                        SAFE_CALL(IGOApplication::instance(), &IGOApplication::toggleIGO);
                        SAFE_CALL(IGOApplication::instance(), &IGOApplication::processPendingIGOToggle);
                    }
                    IGOApplication::deleteInstanceLater();    // delete IGO from the rendering thread, not from this one!!!
                }
            SAFE_CALL_LOCK_LEAVE

            // check for hotkeys
            // only call doHotKeyCheck() here, in the main windows message thread !!!
            SAFE_CALL_LOCK_ENTER
                if (SAFE_CALL(IGOApplication::instance(), &IGOApplication::doHotKeyCheck))
                {
                    SAFE_CALL_LOCK_LEAVE
                        OriginIGO::IGOLogDebug("doHotKeyCheck");
                    return true;
                }
                else
                if (SAFE_CALL(IGOApplication::instance(), &IGOApplication::recentlyHidden))
                {
                    SAFE_CALL_LOCK_LEAVE
                    if (uMsg != gIGOUpdateMessage)
                    {
                        return false;
                    }

                    return true;
                }
                else
                {
                    SAFE_CALL_LOCK_LEAVE
                }

            // check if we should filter windows messages
            SAFE_CALL_LOCK_ENTER
                if (!SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive) && !SAFE_CALL(IGOApplication::instance(), &IGOApplication::isHotKeyPressed))    // if IGO is not activated or the IGO featureing game is not active(in foreground), do not filter WM messages!!!
                {
                    SAFE_CALL_LOCK_LEAVE
                        if (uMsg != gIGOUpdateMessage)
                        {
                            return false;
                        }

                    return true;    // do not forward gIGOUpdateMessage message to the game
                }
                else
                {
                    SAFE_CALL_LOCK_LEAVE
                }

            if (uMsg == gIGOUpdateMessage)        // this is a dummy message we send once IGOToggle is called, so that we
            {                                     // get here in the main WndProc of the game to update certain IGO states
                OriginIGO::IGOLogDebug("uMsg == gIGOUpdateMessage");
                return true;                      
            }

            if (uMsg == gIGOKeyUpMessage_WM_INPUT)
            {
                OriginIGO::IGOLogDebug("uMsg == gIGOKeyUpMessage_WM_INPUT");

                // build our artificial WM_INPUT message
                gLastWM_INPUT_param = lParam;
                gLastWM_INPUT_device = wParam;    //RIM_TYPEMOUSE / RIM_TYPEKEYBOARD

                *uMsgPtr = WM_INPUT;
                *wParamPtr = RIM_INPUT;
                *lParamPtr = NULL;

                return false;
            }

            if (uMsg == gIGOKeyUpMessage_WM_KEYUP)
            {
                OriginIGO::IGOLogDebug("uMsg == gIGOKeyUpMessage_WM_KEYUP");
                SAFE_CALL_LOCK_ENTER
                    bool hotKeyPressed = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isHotKeyPressed);
                SAFE_CALL_LOCK_LEAVE

                    // build our artificial WM_KEYUP message
                    if (!hotKeyPressed)
                    {
                        OriginIGO::IGOLogDebug("!hotKeyPressed");

                        *uMsgPtr = WM_KEYUP;
                        *lParamPtr = 0xc0000001;

                        return false;
                    }
                return true;
            }

            switch (uMsg)
            {
                case WM_INPUTLANGCHANGE:
                {
                    // Handled in SetWindowsHookEx hook
                    return true;
                }

                case WM_SETCURSOR:
                {
                    // Handled in SetWindowsHookEx hook
                    return true;
                }
            }
        } // if instance()

        return false; // not filtered
    }

    // In charge of registering our internal windows messages + hook the window (ie we want to perform this on the proper window UI thread)
    void InternalMessagesImmediateHandling(LPMSG lpMsg)
    {
        if (!lpMsg)
            return;

        static bool msgRegister = false;
        if (!msgRegister && lpMsg->hwnd && ::IsWindowVisible(lpMsg->hwnd))
        {
            msgRegister = true;
            if (IsWindowUnicode(lpMsg->hwnd))
            {
                IGOLogWarn("Registering window messages for Unicode (%p)", lpMsg->hwnd);

                gIGO3rdPartyCheckMessage = RegisterWindowMessageW(L"gIGO3rdPartyCheckMessage");
                gIGOHookWindowMessage = RegisterWindowMessageW(L"gIGOHookWindowMessage");
                gIGOHookGLUpdateMessage = RegisterWindowMessageW(L"gIGOHookGLUpdateMessage");
                gIGOHookMantleUpdateMessage = RegisterWindowMessageW(L"gIGOHookMantleUpdateMessage");
                gIGOHookDX8UpdateMessage = RegisterWindowMessageW(L"gIGOHookDX8UpdateMessage");
                gIGOHookDX9UpdateMessage = RegisterWindowMessageW(L"gIGOHookDX9UpdateMessage");
                gIGOUpdateMessage = RegisterWindowMessageW(L"gIGOUpdateMessage");
                gIGOSwitchGameToBackground = RegisterWindowMessageW(L"gIGOSwitchGameToBackground");
                gIGOKeyUpMessage_WM_INPUT = RegisterWindowMessageW(L"gIGOKeyUpMessage_WM_INPUT");
                gIGOKeyUpMessage_WM_KEYUP = RegisterWindowMessageW(L"gIGOKeyUpMessage_WM_KEYUP");
            }
            else
            {
                IGOLogWarn("Registering window messages for Ansi (%p)", lpMsg->hwnd);

                gIGO3rdPartyCheckMessage = RegisterWindowMessageA("gIGO3rdPartyCheckMessage");
                gIGOHookWindowMessage = RegisterWindowMessageA("gIGOHookWindowMessage");
                gIGOHookGLUpdateMessage = RegisterWindowMessageA("gIGOHookGLUpdateMessage");
                gIGOHookMantleUpdateMessage = RegisterWindowMessageA("gIGOHookMantleUpdateMessage");
                gIGOHookDX8UpdateMessage = RegisterWindowMessageA("gIGOHookDX8UpdateMessage");
                gIGOHookDX9UpdateMessage = RegisterWindowMessageA("gIGOHookDX9UpdateMessage");
                gIGOUpdateMessage = RegisterWindowMessageA("gIGOUpdateMessage");
                gIGOSwitchGameToBackground = RegisterWindowMessageA("gIGOSwitchGameToBackground");
                gIGOKeyUpMessage_WM_INPUT = RegisterWindowMessageA("gIGOKeyUpMessage_WM_INPUT");
                gIGOKeyUpMessage_WM_KEYUP = RegisterWindowMessageA("gIGOKeyUpMessage_WM_KEYUP");
            }

#ifdef _DEBUG
            IGOLogWarn("gIGO3rdPartyCheckMessage = 0x%08x", gIGO3rdPartyCheckMessage);
            IGOLogWarn("gIGOHookWindowMessage = 0x%08x", gIGOHookWindowMessage);
            IGOLogWarn("gIGOHookGLUpdateMessage = 0x%08x", gIGOHookGLUpdateMessage);
            IGOLogWarn("gIGOHookMantleUpdateMessage = 0x%08x", gIGOHookMantleUpdateMessage);
            IGOLogWarn("gIGOHookDX8UpdateMessage = 0x%08x", gIGOHookDX8UpdateMessage);
            IGOLogWarn("gIGOHookDX9UpdateMessage = 0x%08x", gIGOHookDX9UpdateMessage);
            IGOLogWarn("gIGOUpdateMessage = 0x%08x", gIGOUpdateMessage);
            IGOLogWarn("gIGOSwitchGameToBackground = 0x%08x", gIGOSwitchGameToBackground);
            IGOLogWarn("gIGOKeyUpMessage_WM_INPUT = 0x%08x", gIGOKeyUpMessage_WM_INPUT);
            IGOLogWarn("gIGOKeyUpMessage_WM_KEYUP = 0x%08x", gIGOKeyUpMessage_WM_KEYUP);
#endif

            //??? Why can't we wait until we get the gIGOHookWindowMessage ?
            // Just in case we can quickly hook the window
            HWND mainWnd = getProcMainWindow(GetCurrentProcessId());
            if (mainWnd && mainWnd == lpMsg->hwnd)
                InputHook::HookWindowImpl(mainWnd);
        }

        // As long as we are not shutting down/resetting OIG
        if (!gNoMoreFiltering)
        {
            // Is this our internal message to trigger the hooking of the window?
            if (lpMsg->message == OriginIGO::gIGOHookWindowMessage)
                InputHook::HookWindowImpl(lpMsg->hwnd);
        }
    }

// This is an attempt to determine whether a window is "internal" (for example the DDE window created when executing a ShellExecuteEx command) to the system
// and we shouldn't worry about it (especially if it could cause us to deadlock) - feel free to update this function with better criteria!
	bool IsSystemWindow(HWND hWnd)
	{
		if (!hWnd || !IsWindow(hWnd))
			return false;

		BOOL visible = IsWindowVisible(hWnd);
		
		WINDOWINFO info;
		info.cbSize = sizeof(info);
		BOOL success = GetWindowInfo(hWnd, &info);

		 return success && !visible && (info.dwStyle | WS_DISABLED) && info.rcClient.right == 0;
	}

    DEFINE_HOOK_SAFE(BOOL, PeekMessageWHook, (LPMSG lpMsg, HWND hWnd,UINT wMsgFilterMin,UINT wMsgFilterMax,UINT wRemoveMsg))
    
        if (PeekMessageWHookNext == NULL || isPeekMessageWHooked == false)
        {
            return PeekMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
        }


        if(InputHook::IsDestroyed())
            return PeekMessageWHookNext(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);

        BOOL m = PeekMessageWHookNext(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);

        if(m)
        {
            InternalMessagesImmediateHandling(lpMsg);

            if (wRemoveMsg == PM_REMOVE)
            {
                MemoryUnProtector p(lpMsg, sizeof(MSG));

                bool discardKeyboardData = false;
                bool autoCompleteKeyEvents = false;
                if (lpMsg)
                {
				    PrepareKeyboardInputFiltering(gHwnd, lpMsg->hwnd, lpMsg->message, &discardKeyboardData, &autoCompleteKeyEvents);
                    if(p.isOkay() && filterMessages(lpMsg, discardKeyboardData, autoCompleteKeyEvents) )    // filter the message from the game
				    {
					    lpMsg->message=0;
				    }
			    }
            }
        }
        return m;
    }

    DEFINE_HOOK_SAFE(BOOL, PeekMessageAHook, (LPMSG lpMsg, HWND hWnd,UINT wMsgFilterMin,UINT wMsgFilterMax,UINT wRemoveMsg))
    

        if (PeekMessageAHookNext == NULL || isPeekMessageAHooked == false)
        {
            return PeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
        }

        if(InputHook::IsDestroyed())
            return PeekMessageAHookNext(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);

        BOOL m = PeekMessageAHookNext(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);

        if(m)
        {
            InternalMessagesImmediateHandling(lpMsg);

            if (wRemoveMsg == PM_REMOVE)
            {
                MemoryUnProtector p(lpMsg, sizeof(MSG));

                bool discardKeyboardData = false;
                bool autoCompleteKeyEvents = false;
                if (lpMsg)
                {
				    PrepareKeyboardInputFiltering(gHwnd, lpMsg->hwnd, lpMsg->message, &discardKeyboardData, &autoCompleteKeyEvents);
                    if(p.isOkay() && filterMessages(lpMsg, discardKeyboardData, autoCompleteKeyEvents) )    // filter the message from the game
				    {
					    lpMsg->message=0;
				    }
			    }
            }
        }
        return m;
    }

    DEFINE_HOOK_SAFE(BOOL, GetMessageAHook, (LPMSG lpMsg, HWND hWnd,UINT wMsgFilterMin,UINT wMsgFilterMax))
    
        if (GetMessageAHookNext == NULL || isGetMessageAHooked == false)
        {
            return GetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
        }


        if(InputHook::IsDestroyed())
            return GetMessageAHookNext(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);

        BOOL m = GetMessageAHookNext(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);

        if(m!=-1)
        {
            InternalMessagesImmediateHandling(lpMsg);

            MemoryUnProtector p(lpMsg, sizeof(MSG));

            bool discardKeyboardData = false;
            bool autoCompleteKeyEvents = false;
            if (lpMsg)
            {
				PrepareKeyboardInputFiltering(gHwnd, lpMsg->hwnd, lpMsg->message, &discardKeyboardData, &autoCompleteKeyEvents);
                if(p.isOkay() && filterMessages(lpMsg, discardKeyboardData, autoCompleteKeyEvents) )    // filter the message from the game
				{
					lpMsg->message=0;
				}
            }
        }
        return m;
    }

    DEFINE_HOOK_SAFE(BOOL, GetMessageWHook, (LPMSG lpMsg, HWND hWnd,UINT wMsgFilterMin,UINT wMsgFilterMax))
    
        if (GetMessageWHookNext == NULL || isGetMessageWHooked == false)
        {
            return GetMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
        }


        if(InputHook::IsDestroyed())
            return GetMessageWHookNext(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);

        BOOL m = GetMessageWHookNext(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);

        if(m!=-1)
        {
            InternalMessagesImmediateHandling(lpMsg);

            MemoryUnProtector p(lpMsg, sizeof(MSG));

            bool discardKeyboardData = false;
            bool autoCompleteKeyEvents = false;
            if (lpMsg)
            {
				PrepareKeyboardInputFiltering(gHwnd, lpMsg->hwnd, lpMsg->message, &discardKeyboardData, &autoCompleteKeyEvents);
                if(p.isOkay() && filterMessages(lpMsg, discardKeyboardData, autoCompleteKeyEvents) )    // filter the message from the game
				{
					lpMsg->message=0;
				}
			}
        }
        return m;
    }



    DEFINE_HOOK_SAFE(BOOL, TranslateMessageHook, (const MSG *lpMsg))
    
        if (TranslateMessageHookNext == NULL || isTranslateMessageHooked == false)
        {
            return TranslateMessage(lpMsg);
        }


        if(InputHook::IsDestroyed())
            return TranslateMessageHookNext(lpMsg);

        MemoryUnProtector p((MSG*)lpMsg, sizeof(MSG));

        bool discardKeyboardData = false;
        bool autoCompleteKeyEvents = false;
        if (lpMsg)
        {
			PrepareKeyboardInputFiltering(gHwnd, lpMsg->hwnd, lpMsg->message, &discardKeyboardData, &autoCompleteKeyEvents);
            if(p.isOkay() && filterMessages((MSG*)lpMsg, discardKeyboardData, autoCompleteKeyEvents) )    // filter the message from the game
			{
				((MSG*)lpMsg)->message=0;
			}
        }
        return TranslateMessageHookNext(lpMsg);
    }


    DEFINE_HOOK_SAFE(LRESULT, DispatchMessageAHook, (const MSG *lpMsg))
    
        if (DispatchMessageAHookNext == NULL || isDispatchMessageAHooked == false)
        {
            return DispatchMessageA(lpMsg);
        }


        if(InputHook::IsDestroyed())
            return DispatchMessageAHookNext(lpMsg);

        MemoryUnProtector p((MSG*)lpMsg, sizeof(MSG));

        if( lpMsg && p.isOkay() && filterMessages((MSG*)lpMsg, true) )    // filter the message from the game
        {
            ((MSG*)lpMsg)->message=0;
        }

        if (IGOMessageDispatchFilter(lpMsg->hwnd, &((MSG*)lpMsg)->message, &((MSG*)lpMsg)->wParam, &((MSG*)lpMsg)->lParam))
            return 0;

        return DispatchMessageAHookNext(lpMsg);
    }

    DEFINE_HOOK_SAFE(LRESULT, DispatchMessageWHook, (const MSG *lpMsg))
    
        if (DispatchMessageWHookNext == NULL || isDispatchMessageWHooked == false)
        {
            return DispatchMessageW(lpMsg);
        }


        if(InputHook::IsDestroyed())
            return DispatchMessageWHookNext(lpMsg);

        MemoryUnProtector p((MSG*)lpMsg, sizeof(MSG));

        if( lpMsg && p.isOkay() && filterMessages((MSG*)lpMsg, true) )    // filter the message from the game
        {
            ((MSG*)lpMsg)->message=0;
        }

        if (IGOMessageDispatchFilter(lpMsg->hwnd, &((MSG*)lpMsg)->message, &((MSG*)lpMsg)->wParam, &((MSG*)lpMsg)->lParam))
            return 0;

        return DispatchMessageWHookNext(lpMsg);
    }

    DEFINE_HOOK_SAFE(LRESULT, SendMessageAHook, (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam))

        return SendMessageAHookNext(hWnd, Msg, wParam, lParam);
    }

    DEFINE_HOOK_SAFE(LRESULT, SendMessageWHook, (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam))

        return SendMessageWHookNext(hWnd, Msg, wParam, lParam);
    }

    DEFINE_HOOK_SAFE(BOOL, SendMessageCallbackAHook, (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, SENDASYNCPROC lpResultCallBack, ULONG_PTR dwData))

        return SendMessageCallbackAHookNext(hWnd, Msg, wParam, lParam, lpResultCallBack, dwData);
    }

    DEFINE_HOOK_SAFE(BOOL, SendMessageCallbackWHook, (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, SENDASYNCPROC lpResultCallBack, ULONG_PTR dwData))

        return SendMessageCallbackWHookNext(hWnd, Msg, wParam, lParam, lpResultCallBack, dwData);
    }

    DEFINE_HOOK_SAFE(LRESULT, SendMessageTimeoutAHook, (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, UINT fuFlags, UINT uTimeout, PDWORD_PTR lpdwResult))

        return SendMessageTimeoutAHookNext(hWnd, Msg, wParam, lParam, fuFlags, uTimeout, lpdwResult);
    }

    DEFINE_HOOK_SAFE(LRESULT, SendMessageTimeoutWHook, (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, UINT fuFlags, UINT uTimeout, PDWORD_PTR lpdwResult))

        return SendMessageTimeoutWHookNext(hWnd, Msg, wParam, lParam, fuFlags, uTimeout, lpdwResult);
    }

    DEFINE_HOOK_SAFE(BOOL, SendNotifyMessageAHook, (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam))

        return SendNotifyMessageAHookNext(hWnd, Msg, wParam, lParam);
    }

    DEFINE_HOOK_SAFE(BOOL, SendNotifyMessageWHook, (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam))

        return SendNotifyMessageWHookNext(hWnd, Msg, wParam, lParam);
    }

    static BYTE gGetRawInputDataState[256]={0};

    DEFINE_HOOK_SAFE(UINT, GetRawInputDataHook, (HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader))
    
        
        static LONG lastMouseX = 0;
        static LONG lastMouseY = 0;
        static USHORT lastMouseFlags = 0;
        static HANDLE hKeyboardDevice = NULL;
        static HANDLE hMouseDevice = NULL;

        IGO_TRACEF("GetRawInputData: uiCommand %i", uiCommand);
        bool s = false;
        if(SAFE_CALL_LOCK_TRYREADLOCK_COND)
        {
            s = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive);
            SAFE_CALL_LOCK_LEAVE
        }
    
        if (s)
        {
            if (uiCommand == RID_INPUT)
            {
                if (gLastWM_INPUT_param != 0) //our artificial WM_INPUT message
                {
                    if (!pData)
                    {
                        if (pcbSize)
                        {
                            *pcbSize = sizeof(RAWINPUT);    //size of our buffer
                            gLastWM_INPUT_param = 0;
                            return 0;
                        }
                        else
                        {
                            gLastWM_INPUT_param = 0;
                            return (UINT)-1;    // error
                        }
                    }

                    RAWINPUT* pRawInput = static_cast<LPRAWINPUT>(pData);
                    if (gLastWM_INPUT_device == RIM_TYPEKEYBOARD && RIM_TYPEKEYBOARD == pRawInput->header.dwType && pcbSize && (*pcbSize)>=sizeof(RAWINPUT))
                    {
                        memset(pRawInput, 0, sizeof(RAWINPUT));
                
                        pRawInput->data.keyboard.VKey = (USHORT)gLastWM_INPUT_param;
                        pRawInput->data.keyboard.Flags = RI_KEY_BREAK;
                        pRawInput->data.keyboard.Message = WM_KEYUP;
                        pRawInput->data.keyboard.MakeCode = MapVirtualKeyW(pRawInput->data.keyboard.VKey, MAPVK_VK_TO_VSC)&0xFFFF;
                    
                        pRawInput->header.dwSize = sizeof(RAWINPUT);
                        pRawInput->header.hDevice = hKeyboardDevice;
                        pRawInput->header.wParam = RIM_INPUT;
                        pRawInput->header.dwType = RIM_TYPEKEYBOARD;
                        gLastWM_INPUT_param = 0;
                        return sizeof(RAWINPUT);
                    }
                    else
                    if (gLastWM_INPUT_device == RIM_TYPEMOUSE && RIM_TYPEMOUSE == pRawInput->header.dwType && pcbSize && (*pcbSize)>=sizeof(RAWINPUT))
                    {

                        memset(pRawInput, 0, sizeof(RAWINPUT));
            
                        if(gLastWM_INPUT_param == RI_MOUSE_LEFT_BUTTON_UP)
                            pRawInput->data.mouse.usButtonFlags = RI_MOUSE_LEFT_BUTTON_UP;
                        else
                        if(gLastWM_INPUT_param == RI_MOUSE_MIDDLE_BUTTON_UP)
                            pRawInput->data.mouse.usButtonFlags = RI_MOUSE_MIDDLE_BUTTON_UP;
                        else
                        if(gLastWM_INPUT_param == RI_MOUSE_RIGHT_BUTTON_UP)
                            pRawInput->data.mouse.usButtonFlags = RI_MOUSE_RIGHT_BUTTON_UP;

                        pRawInput->data.mouse.lLastX = lastMouseX;
                        pRawInput->data.mouse.lLastY = lastMouseY;
                        pRawInput->data.mouse.usFlags = lastMouseFlags;
                    
                        pRawInput->header.dwSize = sizeof(RAWINPUT);
                        pRawInput->header.hDevice = hMouseDevice;
                        pRawInput->header.wParam = RIM_INPUT;
                        pRawInput->header.dwType = RIM_TYPEKEYBOARD;
                        gLastWM_INPUT_param = 0;
                        return sizeof(RAWINPUT);
                    }
                    else
                    {
                        gLastWM_INPUT_param = 0;
                        return GetRawInputDataHookNext(hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);
                    }
                }

                // clear old data!!!
                if (pData && pcbSize && (*pcbSize)>=sizeof(RAWINPUTHEADER))
                {
                    memset(pData, 0, sizeof(RAWINPUTHEADER));
                    RAWINPUT* pRawInput = static_cast<LPRAWINPUT>(pData);
                    pRawInput->header.dwSize = sizeof(RAWINPUTHEADER);
                    pRawInput->header.dwType = 255; // return a fake device to exclude all devices, because some games only use the 
                                                    // dwType to determin, if input data is available
                                                    // which is wrong, but we can't do anything about it
                }
                return (UINT)-1; // do not return any data!!!
            }
            else
            {
                if(gLastWM_INPUT_param!=0)
                {
                    if (!pData)
                    {
                        if (pcbSize)
                        {
                            *pcbSize = sizeof(RAWINPUTHEADER);    //size of our buffer
                            return 0;
                        }
                        else
                            return (UINT)-1;    // error
                    }

                    if (gLastWM_INPUT_device == RIM_TYPEKEYBOARD && pcbSize && (*pcbSize)>=sizeof(RAWINPUTHEADER))
                    {
                        RAWINPUT* pRawInput = static_cast<LPRAWINPUT>(pData);
                        pRawInput->header.dwSize = sizeof(RAWINPUTHEADER);
                        pRawInput->header.dwType = RIM_TYPEKEYBOARD;
                        pRawInput->header.hDevice = hKeyboardDevice;
                        pRawInput->header.wParam = RIM_INPUT;
                    }
                    else
                        if (gLastWM_INPUT_device == RIM_TYPEMOUSE && pcbSize && (*pcbSize)>=sizeof(RAWINPUTHEADER))
                        {
                            RAWINPUT* pRawInput = static_cast<LPRAWINPUT>(pData);
                            pRawInput->header.dwSize = sizeof(RAWINPUTHEADER);
                            pRawInput->header.dwType = RIM_TYPEMOUSE;
                            pRawInput->header.hDevice = hMouseDevice;
                            pRawInput->header.wParam = RIM_INPUT;
                        }

                    return sizeof(RAWINPUTHEADER);
                }

                // clear old data!!!
                if (pData && pcbSize && (*pcbSize)>=sizeof(RAWINPUTHEADER))
                {
                    memset(pData, 0, sizeof(RAWINPUTHEADER));
                    RAWINPUT* pRawInput = static_cast<LPRAWINPUT>(pData);
                    pRawInput->header.dwSize = sizeof(RAWINPUTHEADER);
                    pRawInput->header.dwType = 255; // return a fake device to exclude all devices, because some games only use the 
                                                    // dwType to determin, if input data is available
                                                    // which is wrong, but we can't do anything about it
                }

                return (UINT)-1;    // otherwise just throw away the data
            }
        }
        else
        {
            // store the "last" state
            if (uiCommand == RID_INPUT)
            {
                if(NULL != pData)
                {
                    // Get raw input data
                    UINT res = GetRawInputDataHookNext(hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);
                    if(res != (UINT)-1 && res > (UINT)0)
                    {
                        RAWINPUT* pRawInput = static_cast<LPRAWINPUT>(pData);
                        if (RIM_TYPEMOUSE == pRawInput->header.dwType)
                        {
                            // store the device handle for later usage
                            hMouseDevice = pRawInput->header.hDevice;

                            // store mouse position flags for later usage
                                lastMouseFlags = pRawInput->data.mouse.usFlags &~ MOUSE_ATTRIBUTES_CHANGED;

                            // store last cursor position
                            lastMouseX = pRawInput->data.mouse.lLastX;
                            lastMouseY = pRawInput->data.mouse.lLastY;

                            if(pRawInput->data.mouse.usButtonFlags&RI_MOUSE_LEFT_BUTTON_DOWN)
                                gGetRawInputDataState[VK_LBUTTON] = 0x80;
                            else
                                gGetRawInputDataState[VK_LBUTTON] = 0x0;

                            if(pRawInput->data.mouse.usButtonFlags&RI_MOUSE_MIDDLE_BUTTON_DOWN)
                                gGetRawInputDataState[VK_MBUTTON] = 0x80;
                            else
                                gGetRawInputDataState[VK_MBUTTON] = 0x0;

                            if(pRawInput->data.mouse.usButtonFlags&RI_MOUSE_RIGHT_BUTTON_DOWN)
                                gGetRawInputDataState[VK_RBUTTON] = 0x80;
                            else
                                gGetRawInputDataState[VK_RBUTTON] = 0x0;
                        }
                        else
                        {
                            if (RIM_TYPEKEYBOARD == pRawInput->header.dwType)
                            {
                                // store the device handle for later usage
                                hKeyboardDevice = pRawInput->header.hDevice;

                                    if (pRawInput->data.keyboard.VKey>=0 && pRawInput->data.keyboard.VKey<256/*range check*/ && pRawInput->data.keyboard.Flags & RI_KEY_BREAK) // key up
                                        gGetRawInputDataState[pRawInput->data.keyboard.VKey] = 0x0;

                                else
                                if (pRawInput->data.keyboard.VKey>=0 && pRawInput->data.keyboard.VKey<256/*range check*/) // key down
                                    gGetRawInputDataState[pRawInput->data.keyboard.VKey] = 0x80;
                            }
                        }
                    }
                    return res;
                }
            }
        }

        return GetRawInputDataHookNext(hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);
    }

// create our own version because 64-bit pc compiles doesn't recognize QWORD from WinUser.h
#ifdef _WIN64
#define RAWINPUT_ALIGN_ORIGIN(x)   (((x) + sizeof(int64_t) - 1) & ~(sizeof(int64_t) - 1))
#else   // _WIN64
#define RAWINPUT_ALIGN_ORIGIN(x)   (((x) + sizeof(int32_t) - 1) & ~(sizeof(int32_t) - 1))
#endif  // _WIN64

#define NEXTRAWINPUTBLOCK_ORIGIN(ptr) ((PRAWINPUT)RAWINPUT_ALIGN_ORIGIN((ULONG_PTR)((PBYTE)(ptr) + (ptr)->header.dwSize)))


    DEFINE_HOOK_SAFE(UINT, GetRawInputBufferHook, (PRAWINPUT pData, PUINT pcbSize, UINT cbSizeHeader))
    
        

        IGO_TRACE("GetRawInputBufferHook\n");
        //IGOLogInfo("GetRawInputBufferHook");
        // if we find a game that uses GetRawInputBuffer we might use this code... (found one: Neverwinter)
        if (SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive))
        {    
            UINT numInputs=0;
            numInputs = GetRawInputBufferHookNext(pData, pcbSize, sizeof(RAWINPUTHEADER));

            if(pData!=NULL && (*pcbSize)>0 && numInputs != 0 && numInputs != (UINT)-1)
            {
                PRAWINPUT pri = reinterpret_cast<LPRAWINPUT>(pData);

                for (UINT i = 0; i < numInputs; ++i) 
                {                     
                    if (RIM_TYPEMOUSE == pri->header.dwType)
                    {
                        if(pri->data.mouse.usButtonFlags&RI_MOUSE_LEFT_BUTTON_DOWN)
                        {
                            pri->data.mouse.usButtonFlags = pri->data.mouse.usButtonFlags &~ RI_MOUSE_LEFT_BUTTON_DOWN;
                            pri->data.mouse.usButtonFlags = pri->data.mouse.usButtonFlags | RI_MOUSE_LEFT_BUTTON_UP;
                        }

                        if(pri->data.mouse.usButtonFlags&RI_MOUSE_MIDDLE_BUTTON_DOWN)
                        {
                            pri->data.mouse.usButtonFlags = pri->data.mouse.usButtonFlags &~ RI_MOUSE_MIDDLE_BUTTON_DOWN;
                            pri->data.mouse.usButtonFlags = pri->data.mouse.usButtonFlags | RI_MOUSE_MIDDLE_BUTTON_UP;
                        }

                        if(pri->data.mouse.usButtonFlags&RI_MOUSE_RIGHT_BUTTON_DOWN)
                        {
                            pri->data.mouse.usButtonFlags = pri->data.mouse.usButtonFlags &~ RI_MOUSE_RIGHT_BUTTON_DOWN;
                            pri->data.mouse.usButtonFlags = pri->data.mouse.usButtonFlags | RI_MOUSE_RIGHT_BUTTON_UP;
                        }
                    }
                    else
                    {
                        if (RIM_TYPEKEYBOARD == pri->header.dwType)
                        {
                            //pri->data.keyboard.VKey;
                            if (pri->data.keyboard.Flags & RI_KEY_MAKE) // key down
                            {
                                pri->data.keyboard.Flags = pri->data.keyboard.Flags &~ RI_KEY_MAKE;
                                pri->data.keyboard.Flags = pri->data.keyboard.Flags | RI_KEY_BREAK;
                        
                                if (pri->data.keyboard.Message == WM_KEYDOWN)
                                    pri->data.keyboard.Message = WM_KEYUP;
                                if (pri->data.keyboard.Message == WM_SYSKEYDOWN)
                                    pri->data.keyboard.Message = WM_SYSKEYUP;
                            }
                        }
                    }
                    pri = NEXTRAWINPUTBLOCK_ORIGIN(pri);    // next element
                }
            }

            return numInputs;
        }
        
        return GetRawInputBufferHookNext(pData, pcbSize, cbSizeHeader);
    }

    DEFINE_HOOK_SAFE(UINT, SendInputHook, (UINT cInputs, LPINPUT pInputs, int cbSize))
    
        
        bool s = false;
        if(SAFE_CALL_LOCK_TRYREADLOCK_COND)
        {
            s = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive);
            SAFE_CALL_LOCK_LEAVE
        }
        if (s)
            return cInputs;

        return SendInputHookNext(cInputs, pInputs, cbSize);
    }

    DEFINE_HOOK_SAFE_VOID( mouse_eventHook, (DWORD dwFlags, DWORD dx, DWORD dy, DWORD dwData, ULONG_PTR dwExtraInfo))
    
        
        bool s = false;
        if(SAFE_CALL_LOCK_TRYREADLOCK_COND)
        {
            s = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive);
            SAFE_CALL_LOCK_LEAVE
        }
        if (s)
            return;

        mouse_eventHookNext(dwFlags,
                            dx,
                            dy,
                            dwData,
                            dwExtraInfo);
    }



    DEFINE_HOOK_SAFE_VOID( keybd_eventHook, (BYTE bVk, BYTE bScan, DWORD dwFlags, ULONG_PTR dwExtraInfo))
    
        
    
        bool s = false;
        if(SAFE_CALL_LOCK_TRYREADLOCK_COND)
        {
            s = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive);
            SAFE_CALL_LOCK_LEAVE
        }
        if (s)
            return;

        keybd_eventHookNext(bVk,
                            bScan,
                            dwFlags,
                            dwExtraInfo);
    }

    // DInput keyboard & mouse - they only differ from their pDev

    DEFINE_HOOK_SAFE(HRESULT, GetDeviceStateHook, (IDirectInputDeviceA *pDev, DWORD cbData, LPVOID lpvData))
    
        
    
        bool s = false;
        if(SAFE_CALL_LOCK_TRYREADLOCK_COND)
        {
            s = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive);
            SAFE_CALL_LOCK_LEAVE
        }
        if (s)
        {
            if (lpvData)
                ZeroMemory(lpvData, cbData);
            return S_OK;
        }

        HRESULT hr = GetDeviceStateHookNext(pDev, cbData, lpvData);

        return hr;
    }


    // for buffered DInput keyboard data, we have to save the last state, in order to return "key release" events when IGO is active!!!!
    // example game: BFBC2
    DEFINE_HOOK_SAFE(HRESULT, GetDeviceDataHook, (IDirectInputDeviceA *pDev, DWORD cbObjectData, LPDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD dwFlags))
    
        
        typedef eastl::hash_map<DWORD, bool> tmpDataPair;
    
        static eastl::hash_map<IDirectInputDeviceA*, tmpDataPair> DInputStates;    // states are per device

    

        DWORD dwInOut = pdwInOut!=NULL ? ( ((*pdwInOut)!=0xffffffff) ? (*pdwInOut) : 0 ): 0;    // save the size of the initial buffer
        bool bQuery = false;
        bool bFlush = false;
        bool bOverflow = false;

        // QUERY, FLUSH or OVERFLOW operation
        if (!rgdod && pdwInOut && (*pdwInOut)==0xffffffff && !dwFlags)
            bFlush = true;
        else
        if (!rgdod && pdwInOut && (*pdwInOut)==0xffffffff && dwFlags==DIGDD_PEEK)
            bQuery = true;
        else
        if (!rgdod && pdwInOut && (*pdwInOut)==0x0 && !dwFlags)
            bOverflow = true;

        HRESULT hr = GetDeviceDataHookNext(pDev, cbObjectData, rgdod, pdwInOut, dwFlags);

        bool s = false;
        if(SAFE_CALL_LOCK_TRYREADLOCK_COND)
        {
            s = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive);
            SAFE_CALL_LOCK_LEAVE
        }
        if (s)
        {
            if (hr==DI_OK)
            {
                if (bFlush)
                {
                    *pdwInOut = static_cast<DWORD>(DInputStates[pDev].size());
                    DInputStates[pDev].clear();
                    return hr;
                }
            
                if (bQuery)
                {
                    *pdwInOut = static_cast<DWORD>(DInputStates[pDev].size());
                    return hr;
                }

                if (bOverflow)
                {
                    return hr;
                }

                if (rgdod && dwInOut!=0 && pdwInOut)
                {
                    DWORD *dataPtr=(DWORD*)rgdod;

                    *pdwInOut=0;    // set element counter to zero

                    // buffer is too small -> request a larger one
                    if (dwInOut<DInputStates[pDev].size())
                        return DI_BUFFEROVERFLOW;

                    if(DInputStates[pDev].empty())
                    {
                        memset(rgdod, 0, cbObjectData*dwInOut);
                    }
                    else
                    {
                        eastl::hash_map<DWORD, bool>::const_iterator tmpDInputDataIter = DInputStates[pDev].begin();
                        for(DWORD i=0; i<dwInOut; i++)
                        {
                            if( tmpDInputDataIter!=DInputStates[pDev].end() )
                            {
                                (*(dataPtr+0))=(DWORD)tmpDInputDataIter->first;// dwOfs
                                (*(dataPtr+1))=(DWORD)0L; // dwData
                                ++tmpDInputDataIter;

                                *pdwInOut=(i+1);    // increase element counter
                                dataPtr+=cbObjectData/sizeof(DWORD); // next element
                            }
                            else
                                break;
                        }

                        // we have sent all data -> clear our buffer to prevent repeated events
                        if (dwInOut>=DInputStates[pDev].size())
                            DInputStates[pDev].clear();
                    }
                }
            }
        }
        else
        {
            if (hr==DI_OK && !bQuery && !bFlush && !bOverflow && rgdod && (*pdwInOut)!=0xffffffff && (*pdwInOut)!=0)
            {
                DWORD *dataPtr=(DWORD*)rgdod;

                // store last keyboard state
                for(DWORD i=0; i<(*pdwInOut); i++)
                {
                    if( (*(dataPtr+1)) & 0x80 )    // key down?
                        DInputStates[pDev].insert(eastl::make_pair( ( (DWORD) (*(dataPtr)) ), (bool)true) );    // store
                    else
                    {
                        // otherwise remove it
                        eastl::hash_map<DWORD, bool>::iterator tmpDInputDataIter = DInputStates[pDev].find( (DWORD) *(dataPtr) );
                        if(tmpDInputDataIter != DInputStates[pDev].end())
                            DInputStates[pDev].erase(tmpDInputDataIter);
                    }

                    dataPtr+=cbObjectData/sizeof(DWORD); // next element
                }
            }
        }
        return hr;
    }


    DEFINE_HOOK_SAFE(HRESULT, GetDeviceState8Hook, (IDirectInputDevice8A *pDev, DWORD cbData, LPVOID lpvData))
    
        

        bool s = false;
        if(SAFE_CALL_LOCK_TRYREADLOCK_COND)
        {
            s = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive);
            SAFE_CALL_LOCK_LEAVE
        }
        if (s)
        {
            if (lpvData)
                ZeroMemory(lpvData, cbData);
            return S_OK;
        }

        HRESULT hr = GetDeviceState8HookNext(pDev, cbData, lpvData);

        return hr;
    }

    // for buffered DInput keyboard data, we have to save the last state, in order to return "key release" events when IGO is active!!!!
    // example game: BFBC2
    DEFINE_HOOK_SAFE(HRESULT, GetDeviceData8Hook, (IDirectInputDevice8A *pDev, DWORD cbObjectData, LPDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD dwFlags))
    
        
        typedef eastl::hash_map<DWORD, bool> tmpDataPair;
        static eastl::hash_map<IDirectInputDevice8A*, tmpDataPair> DInputStates;    // states are per device

    

        DWORD dwInOut = pdwInOut!=NULL ? ( ((*pdwInOut)!=0xffffffff) ? (*pdwInOut) : 0 ): 0;    // save the size of the initial buffer
        bool bQuery = false;
        bool bFlush = false;
        bool bOverflow = false;

        // QUERY, FLUSH or OVERFLOW operation
        if (!rgdod && pdwInOut && (*pdwInOut)==0xffffffff && !dwFlags)
            bFlush = true;
        else
        if (!rgdod && pdwInOut && (*pdwInOut)==0xffffffff && dwFlags==DIGDD_PEEK)
            bQuery = true;
        else
        if (!rgdod && pdwInOut && (*pdwInOut)==0x0 && !dwFlags)
            bOverflow = true;

        HRESULT hr = GetDeviceData8HookNext(pDev, cbObjectData, rgdod, pdwInOut, dwFlags);

        bool s = false;
        if(SAFE_CALL_LOCK_TRYREADLOCK_COND)
        {
            s = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive);
            SAFE_CALL_LOCK_LEAVE
        }
        if (s)
        {
            if (hr==DI_OK)
            {
                if (bFlush)
                {
                    *pdwInOut = static_cast<DWORD>(DInputStates[pDev].size());
                    DInputStates[pDev].clear();
                    return hr;
                }
            
                if (bQuery)
                {
                    *pdwInOut = static_cast<DWORD>(DInputStates[pDev].size());
                    return hr;
                }

                if (bOverflow)
                {
                    return hr;
                }

                if (rgdod && dwInOut!=0 && pdwInOut)
                {
                    DWORD *dataPtr=(DWORD*)rgdod;

                    *pdwInOut=0;    // set element counter to zero

                    // buffer is too small -> request a larger one
                    if (dwInOut<DInputStates[pDev].size())
                        return DI_BUFFEROVERFLOW;

                    if(DInputStates[pDev].empty())
                    {
                        memset(rgdod, 0, cbObjectData*dwInOut);
                    }
                    else
                    {
                        eastl::hash_map<DWORD, bool>::const_iterator tmpDInputDataIter = DInputStates[pDev].begin();
                        for(DWORD i=0; i<dwInOut; i++)
                        {
                            if( tmpDInputDataIter!=DInputStates[pDev].end() )
                            {
                                (*(dataPtr+0))=(DWORD)tmpDInputDataIter->first;// dwOfs
                                (*(dataPtr+1))=(DWORD)0L; // dwData
                                //(*(dataPtr+2))=lastTimeStamp++; // dwTimeStamp
                                //(*(dataPtr+3))=lastSequence++; // dwSequence
                                ++tmpDInputDataIter;

                                *pdwInOut=(i+1);    // increase element counter
                                dataPtr+=cbObjectData/sizeof(DWORD); // next element
                            }
                            else
                                break;
                        }

                        // we have sent all data -> clear our buffer to prevent repeated events
                        if (dwInOut>=DInputStates[pDev].size())
                            DInputStates[pDev].clear();
                    }
                }
            }
        }
        else
        {
            if (hr==DI_OK && !bQuery && !bFlush && !bOverflow && rgdod && (*pdwInOut)!=0xffffffff && (*pdwInOut)!=0)
            {
                DWORD *dataPtr=(DWORD*)rgdod;

                // store last keyboard state
                for(DWORD i=0; i<(*pdwInOut); i++)
                {
                    if( (*(dataPtr+1)) & 0x80 )    // key down?
                        DInputStates[pDev].insert(eastl::make_pair( ( (DWORD) (*(dataPtr)) ), (bool)true) );    // store
                    else
                    {
                        // otherwise remove it
                        eastl::hash_map<DWORD, bool>::iterator tmpDInputDataIter = DInputStates[pDev].find( (DWORD) *(dataPtr) );
                        if(tmpDInputDataIter != DInputStates[pDev].end())
                            DInputStates[pDev].erase(tmpDInputDataIter);
                    }

                    dataPtr+=cbObjectData/sizeof(DWORD); // next element
                }
            }
        }
        return hr;
    }

    DEFINE_HOOK_SAFE(HRESULT, SetCooperativeLevel8Hook, (IDirectInputDevice8A *pDev, HWND hwnd, DWORD dwFlags))
    
        static DWORD lastFlags = 0;
        if (lastFlags != dwFlags)
        {
            lastFlags = dwFlags;
            IGOLogInfo("SetCooperativeLevel, by the game: dwFlags %i", dwFlags);
        }

        return SetCooperativeLevel8HookNext(pDev, hwnd, dwFlags);
    }

    DEFINE_HOOK_SAFE(HRESULT, SetCooperativeLevelHook, (IDirectInputDeviceA *pDev, HWND hwnd, DWORD dwFlags))
    
        static DWORD lastFlags = 0;
        if (lastFlags != dwFlags)
        {
            lastFlags = dwFlags;
            IGOLogInfo("SetCooperativeLevel, by the game: dwFlags %i", dwFlags);
        }

        return SetCooperativeLevelHookNext(pDev, hwnd, dwFlags);
    }

    DEFINE_HOOK_SAFE(BOOL, RegisterRawInputDevicesHook, (PCRAWINPUTDEVICE pRawInputDevices, UINT uiNumDevices, UINT cbSize))
    
        
        // Uncomment when needed - otherwise may spam logs (ME1) - IGOLogInfo("RegisterRawInputDevices, by the game: uiNumDevices %i usUsage %i usUsagePage %i dwFlags %i", uiNumDevices, pRawInputDevices!=NULL ? pRawInputDevices->usUsage: 0, pRawInputDevices!=NULL ? pRawInputDevices->usUsagePage: 0, pRawInputDevices!=NULL ? pRawInputDevices->dwFlags: 0);
        const int MAX_DEVICES = 64;
        static RAWINPUTDEVICE deviceList[MAX_DEVICES]={0};

        IGO_ASSERT(uiNumDevices<=MAX_DEVICES);

        if (pRawInputDevices)
        {
            if (uiNumDevices)
                memcpy(&deviceList[0], pRawInputDevices, (uiNumDevices>MAX_DEVICES ? MAX_DEVICES : uiNumDevices)*sizeof(RAWINPUTDEVICE));
            else
                memset(&deviceList[0], 0, MAX_DEVICES*sizeof(RAWINPUTDEVICE));

        

            // check if the game uses mouse and/or keyboard for raw input and removes the WM_xxx legacy messages required by IGO
            // RegisterRawInputDevices parameters will be altered, if IGO is active, otherwise we loose IGO input functionality
            for (UINT i = 0; i < (uiNumDevices>MAX_DEVICES ? MAX_DEVICES : uiNumDevices); i++) 
            {
                if( ((deviceList[i].dwFlags & RIDEV_NOLEGACY) || (deviceList[i].dwFlags & RIDEV_CAPTUREMOUSE)) && (deviceList[i].usUsagePage == 0x01) && (deviceList[i].usUsage == 0x02 /*HID_DEVICE_SYSTEM_MOUSE*/) )
                {
                    gRawMouseState = deviceList[i].dwFlags; // save original flags
                    gRawMouse = true;
                    if (SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive))
                    {
                        deviceList[i].dwFlags = deviceList[i].dwFlags &~ (RIDEV_NOLEGACY | RIDEV_CAPTUREMOUSE);
                    }
                }
            
                // device will be removed
                if( (deviceList[i].dwFlags & RIDEV_REMOVE) && (deviceList[i].usUsagePage == 0x01) && (deviceList[i].usUsage == 0x02 /*HID_DEVICE_SYSTEM_MOUSE*/) )
                {
                    gRawMouseState = deviceList[i].dwFlags; // save original flags
                    gRawMouse = false;
                }

                if( ((deviceList[i].dwFlags & RIDEV_NOLEGACY) /*|| (deviceList[i].dwFlags & RIDEV_NOHOTKEYS) do not allow winkey+tab, this breaks Crysis 3 */) && (deviceList[i].usUsagePage == 0x01) && (deviceList[i].usUsage == 0x06 /*HID_DEVICE_SYSTEM_KEYBOARD*/) )
                {
                    gRawKeyboardState = deviceList[i].dwFlags; // save original flags
                    gRawKeyboard = true;
                    if (SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive))
                    {
                        deviceList[i].dwFlags = deviceList[i].dwFlags &~ (RIDEV_NOLEGACY);// | RIDEV_NOHOTKEYS);
                    }
                }
                // device will be removed
                if( (deviceList[i].dwFlags & RIDEV_REMOVE) && (deviceList[i].usUsagePage == 0x01) && (deviceList[i].usUsage == 0x06 /*HID_DEVICE_SYSTEM_KEYBOARD*/) )
                {
                    gRawKeyboardState = deviceList[i].dwFlags; // save original flags
                    gRawKeyboard = false;
                }
            }
        
            return RegisterRawInputDevicesHookNext(&deviceList[0], (uiNumDevices>MAX_DEVICES ? MAX_DEVICES : uiNumDevices), cbSize);
        }
        else
            return RegisterRawInputDevicesHookNext(pRawInputDevices, uiNumDevices, cbSize);
    }

    DEFINE_HOOK_SAFE(BOOL, BlockInputHook, (BOOL fBlockIt))
    
        
        IGOLogInfo("BlockInput, by the game: fBlockIt %i", fBlockIt);

        return BlockInputHookNext(fBlockIt);
    }


    DEFINE_HOOK_SAFE(HCURSOR, SetCursorHook, (HCURSOR hCursor))
    
        
        if(SAFE_CALL_LOCK_TRYREADLOCK_COND)
        {
            bool s = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive);
            if (s)
            {
                SAFE_CALL(IGOApplication::instance(), &IGOApplication::updateCursor, IGOIPC::CURSOR_UNKNOWN);
                SAFE_CALL_LOCK_LEAVE
                gPrevCursor = hCursor;
                return GetCursor();
            }
            else
                SAFE_CALL_LOCK_LEAVE
        }

        return SetCursorHookNext(hCursor);
    }

    DEFINE_HOOK_SAFE(int, ShowCursorHook, (BOOL bShow))
    
        
        if(SAFE_CALL_LOCK_TRYREADLOCK_COND)
        {
            bool s = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive);
            SAFE_CALL_LOCK_LEAVE
            if (s)
            {
                if (bShow)
                    ++gShowCursorCount;

                else
                    --gShowCursorCount;

                //IGOLogInfo("ShowCursor %i %i", gShowCursorCount, bShow);
                return gShowCursorCount;
            }
        }

        return ShowCursorHookNext(bShow);
    }

    DEFINE_HOOK_SAFE(BOOL, SetCursorPosHook, (int X, int Y))
    
        if(SAFE_CALL_LOCK_TRYREADLOCK_COND)
        {
            bool s = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive);
            SAFE_CALL_LOCK_LEAVE
            if (s)
                return TRUE;
        }
        return SetCursorPosHookNext(X, Y);
    }

    DEFINE_HOOK_SAFE(int, GetCursorPosHook, (LPPOINT lpPoint))
    
        
        if(SAFE_CALL_LOCK_TRYREADLOCK_COND)
        {
            bool s = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive);
            SAFE_CALL_LOCK_LEAVE
            if (s)
            {
                lpPoint->x = gSavedMousePos.x;
                lpPoint->y = gSavedMousePos.y;
                return TRUE;
            }
        }

        return GetCursorPosHookNext(lpPoint);
    }

    DEFINE_HOOK_SAFE(BOOL, GetKeyboardStateHook, (PBYTE lpKeyState))
    
        
        if(SAFE_CALL_LOCK_TRYREADLOCK_COND)
        {
            bool s = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive);
            SAFE_CALL_LOCK_LEAVE
            if (s)
            {
                memset(lpKeyState, 0, sizeof(BYTE)*256);
                return TRUE;
            }
        }
        return GetKeyboardStateHookNext(lpKeyState);
    }
    
    DEFINE_HOOK_SAFE(SHORT, GetKeyStateHook, (int vKey))
    
        
        if(SAFE_CALL_LOCK_TRYREADLOCK_COND)
        {
            bool s = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive);
            SAFE_CALL_LOCK_LEAVE
            if (s)
                return 0;    // key up
        }
        return GetKeyStateHookNext(vKey);
    }

    DEFINE_HOOK_SAFE(SHORT, GetAsyncKeyStateHook, (int vKey))
    
	
        if(SAFE_CALL_LOCK_TRYREADLOCK_COND)
        {
            bool s = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive);
            SAFE_CALL_LOCK_LEAVE
            if (s)
                return 0;    // key up
        }

        return GetAsyncKeyStateHookNext(vKey);
    }

    #include <xinput.h>

    DEFINE_HOOK_SAFE(DWORD, XInputGetKeystrokeHook, (DWORD dwUserIndex, DWORD dwReserved, PXINPUT_KEYSTROKE pKeystroke))
    
        
    
        DWORD res = ERROR_SUCCESS;

        if(SAFE_CALL_LOCK_TRYREADLOCK_COND)
        {
            bool s = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive);
            SAFE_CALL_LOCK_LEAVE
            if (s)
                res = ERROR_EMPTY;
            else
                res = XInputGetKeystrokeHookNext(dwUserIndex, dwReserved, pKeystroke);
        }
        return res;
    }

    DEFINE_HOOK_SAFE(DWORD, XInputGetStateHook, (DWORD dwUserIndex, XINPUT_STATE* pState))
    
        
	    static XINPUT_STATE lastInputState = {0};

	    DWORD res = XInputGetStateHookNext(dwUserIndex, pState);
        if(SAFE_CALL_LOCK_TRYREADLOCK_COND)
        {
            bool s = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive);
            SAFE_CALL_LOCK_LEAVE
            if (s)
            {
                if (res == ERROR_SUCCESS && pState!=NULL)
                {
                    memcpy(pState, &lastInputState, sizeof(XINPUT_STATE));
                }
            }
            else
            {
                if (res == ERROR_SUCCESS)
                {
                    memcpy(&lastInputState, pState, sizeof(XINPUT_STATE));
                }
            }
        }
        return res;
    }

    static void DInputInit(HMODULE hDI)
    {
        typedef HRESULT (WINAPI *LPDirectInputCreateEx)(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter);
    
        LPDirectInputCreateEx s_DirectInputCreateEx = (LPDirectInputCreateEx)GetProcAddress(hDI, "DirectInputCreateEx");

        IDirectInputA *dinputA = NULL;
        HRESULT hr = s_DirectInputCreateEx(GetModuleHandle(NULL), 0x0700, IID_IDirectInputA, (LPVOID *)&dinputA, NULL);

        if(SUCCEEDED(hr))
        {
            // keyboard & mouse - only one device for hooking needed, they share the funtion ptr
            IDirectInputDeviceA *keyboard = NULL;
            hr = dinputA->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
            if(SUCCEEDED(hr))
            {

                // Note: A & W versions of the object hav ethe same function ptr for these functions
                // That's why we only have to hook one of them
                HOOKCODE_SAFE(GetInterfaceMethod(keyboard, 9), GetDeviceStateHook);
                HOOKCODE_SAFE(GetInterfaceMethod(keyboard, 10), GetDeviceDataHook);
                HOOKCODE_SAFE(GetInterfaceMethod(keyboard, 13), SetCooperativeLevelHook);

                if (keyboard)
                    keyboard->Release();
            }

                dinputA->Release();
        }
    }

    static void DInputInit8(HMODULE hDI)
    {
        typedef HRESULT (WINAPI *LPDirectInput8Create)(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter);

        LPDirectInput8Create s_DirectInput8Create = (LPDirectInput8Create)GetProcAddress(hDI, "DirectInput8Create");
    
        IDirectInput8A *dinputA = NULL;
        HRESULT hr = s_DirectInput8Create(GetModuleHandle(NULL), 0x800, IID_IDirectInput8A, (LPVOID *)&dinputA, NULL);

        if(SUCCEEDED(hr))
        {
            // keyboard & mouse - only one device for hooking needed, they share the funtion ptr
            IDirectInputDevice8A *keyboard = NULL;
            hr = dinputA->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
            if(SUCCEEDED(hr))
            {
                // Note: A & W versions of the object hav ethe same function ptr for these functions
                // That's why we only have to hook one of them
                HOOKCODE_SAFE(GetInterfaceMethod(keyboard, 9), GetDeviceState8Hook);
                HOOKCODE_SAFE(GetInterfaceMethod(keyboard, 10), GetDeviceData8Hook);
                HOOKCODE_SAFE(GetInterfaceMethod(keyboard, 13), SetCooperativeLevel8Hook);

                if (keyboard)
                    keyboard->Release();
            }

                dinputA->Release();
        }
    }

    #define SetCursorFunc(cursor) ((SetCursorHookNext && isSetCursorHooked) ? SetCursorHookNext(cursor) : SetCursor(cursor))
    #define ShowCursorFunc(visible) ((ShowCursorHookNext && isShowCursorHooked) ? ShowCursorHookNext(visible) : ShowCursor(visible))
    #define SetCursorPosFunc(X, Y) ((SetCursorPosHookNext && isSetCursorPosHooked) ? SetCursorPosHookNext(X, Y) : SetCursorPos(X, Y))
    #define GetCursorPosFunc(point) ((GetCursorPosHookNext && isGetCursorPosHooked) ? GetCursorPosHookNext(point) : GetCursorPos(point))
    #define GetAsyncKeyStateFunc(key) ((GetAsyncKeyStateHookNext && isGetAsyncKeyStateHooked) ? GetAsyncKeyStateHookNext(key) : GetAsyncKeyState(key))
    #define GetKeyStateFunc(key) ((GetKeyStateHookNext && isGetKeyStateHooked) ? GetKeyStateHookNext(key) : GetKeyState(key))
    #define GetKeyboardStateFunc(lpState) ((GetKeyboardStateHookNext && isGetKeyboardStateHooked) ? GetKeyboardStateHookNext(lpState) : GetKeyboardState(lpState))
    #define SendInputFunc(cInputs, pInputs, cbSize) ((SendInputHookNext && isSendInputHooked) ? SendInputHookNext(cInputs, pInputs, cbSize) : SendInput(cInputs, pInputs, cbSize))
    #define SetWindowsHookExAFunc(idHook, lpfn, hmod, dwThreadId) ((SetWindowsHookExAHookNext && isSetWindowsHookExAHooked) ? SetWindowsHookExAHookNext(idHook, lpfn, hmod, dwThreadId) : SetWindowsHookExA(idHook, lpfn, hmod, dwThreadId))
    #define SetWindowsHookExWFunc(idHook, lpfn, hmod, dwThreadId) ((SetWindowsHookExWHookNext && isSetWindowsHookExWHooked) ? SetWindowsHookExWHookNext(idHook, lpfn, hmod, dwThreadId) : SetWindowsHookExW(idHook, lpfn, hmod, dwThreadId))
    #define UnhookWindowsHookExFunc(hhk) ((UnhookWindowsHookExHookNext && isUnhookWindowsHookExHooked) ? UnhookWindowsHookExHookNext(hhk) : UnhookWindowsHookEx(hhk))

    void releaseInputs();


    DEFINE_HOOK_SAFE(HHOOK, SetWindowsHookExAHook, (int idHook, HOOKPROC lpfn, HINSTANCE hmod, DWORD dwThreadId))

        static int lastIdHook = 0;
        if (lastIdHook != idHook)
        {
            lastIdHook = idHook;
            IGOLogInfo("Windows hooks(SetWindowsHookExA), installed by the game: %i %i", idHook, dwThreadId);
        }
        return SetWindowsHookExAHookNext(idHook, lpfn, hmod, dwThreadId);
    }

    DEFINE_HOOK_SAFE(HHOOK, SetWindowsHookExWHook, (int idHook, HOOKPROC lpfn, HINSTANCE hmod, DWORD dwThreadId))

        static int lastIdHook = 0;
        if (lastIdHook != idHook)
        {
            lastIdHook = idHook;
            IGOLogInfo("Windows hooks(SetWindowsHookExW), installed by the game: %i %i", idHook, dwThreadId);
        }
        return SetWindowsHookExWHookNext(idHook, lpfn, hmod, dwThreadId);
    }

    DEFINE_HOOK_SAFE(BOOL, UnhookWindowsHookExHook, (HHOOK hhk))

        IGOLogInfo("Windows hooks(UnhookWindowsHookEx), removed by the game: %p", hhk);
        return UnhookWindowsHookExHookNext(hhk);
    }
   
    DEFINE_HOOK_SAFE(int, MessageBoxAHook, (HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType))

        if (SAFE_CALL_LOCK_TRYREADLOCK_COND)
        {
            bool s = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive);
            if (s)
                SAFE_CALL(IGOApplication::instance(), &IGOApplication::toggleIGO);    // force IGO into the background, so the user can interact with the MessageBox
            SAFE_CALL_LOCK_LEAVE
        }

        return MessageBoxAHookNext(hWnd, lpText, lpCaption, uType);
    }

    DEFINE_HOOK_SAFE(int, MessageBoxWHook, (HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType))

        if (SAFE_CALL_LOCK_TRYREADLOCK_COND)
        {
            bool s = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isActive);
            if (s)
                SAFE_CALL(IGOApplication::instance(), &IGOApplication::toggleIGO);    // force IGO into the background, so the user can interact with the MessageBox
            SAFE_CALL_LOCK_LEAVE
        }
    
        return MessageBoxWHookNext(hWnd, lpText, lpCaption, uType);
    }

    /*
    http://msdn.microsoft.com/en-us/windows/hardware/gg487473
    usUsage 0x02 HID_DEVICE_SYSTEM_MOUSE
    usUsage 0x06 HID_DEVICE_SYSTEM_KEYBOARD
    */

    // we have to reset the RIDEV_NOLEGACY flag, otherwise IGO will not be able to get any WM_xxx messages for mouse & keyboard events!!!
    void resetRawInputDeviceFlags()
    {
        RAWINPUTDEVICE *devices=NULL;
        UINT numDevices=0;
        UINT res = GetRegisteredRawInputDevices(NULL, &numDevices, sizeof(RAWINPUTDEVICE));
        
        //IGOLogInfo("resetRawInputDeviceFlags %i", numDevices);
        if (numDevices > 0)
        {
            devices = (RAWINPUTDEVICE *)malloc(numDevices*sizeof(RAWINPUTDEVICE));
            memset(devices, 0, numDevices*sizeof(RAWINPUTDEVICE));

            res = GetRegisteredRawInputDevices(devices, &numDevices, sizeof(RAWINPUTDEVICE));
            if (res != (UINT)-1)
            {
                for (UINT i = 0; i < numDevices; i++) 
                {
                    //IGOLogInfo("resetRawInputDeviceFlags: uiDevices %i usUsage %i usUsagePage %i dwFlags %i", i, devices[i].usUsage, devices[i].usUsagePage, devices[i].dwFlags);
                    if( ((devices[i].dwFlags & RIDEV_NOLEGACY) || (devices[i].dwFlags & RIDEV_CAPTUREMOUSE)) && (devices[i].usUsagePage == 0x01) && (devices[i].usUsage == 0x02 /*HID_DEVICE_SYSTEM_MOUSE*/) )
                    {
                        gRawMouse = true;
                        gRawMouseState = devices[i].dwFlags; // save original flags
                        // unregister
                        RAWINPUTDEVICE rid;
                        rid.usUsagePage = devices[i].usUsagePage;
                        rid.usUsage = devices[i].usUsage;
                        rid.dwFlags = RIDEV_REMOVE;
                        rid.hwndTarget = devices[i].hwndTarget;
                        RegisterRawInputDevicesHookNext(&rid, 1, sizeof(rid));

                        // re-register
                        rid.usUsagePage = devices[i].usUsagePage;
                        rid.usUsage = devices[i].usUsage;
                        rid.dwFlags = devices[i].dwFlags &~ (RIDEV_NOLEGACY | RIDEV_CAPTUREMOUSE);
                        rid.hwndTarget = devices[i].hwndTarget;
                        RegisterRawInputDevicesHookNext(&rid, 1, sizeof(rid));
                    }

                    if( ((devices[i].dwFlags & RIDEV_NOLEGACY)/* || (devices[i].dwFlags & RIDEV_NOHOTKEYS) do not allow winkey+tab, this breaks Crysis 3 */) && (devices[i].usUsagePage == 0x01) && (devices[i].usUsage == 0x06 /*HID_DEVICE_SYSTEM_KEYBOARD*/) )
                    {
                        gRawKeyboard = true;
                        gRawKeyboardState = devices[i].dwFlags; // save original flags
                        // unregister
                        RAWINPUTDEVICE rid;
                        rid.usUsagePage = devices[i].usUsagePage;
                        rid.usUsage = devices[i].usUsage;
                        rid.dwFlags = RIDEV_REMOVE;
                        rid.hwndTarget = devices[i].hwndTarget;
                        RegisterRawInputDevicesHookNext(&rid, 1, sizeof(rid));

                        // re-register
                        rid.usUsagePage = devices[i].usUsagePage;
                        rid.usUsage = devices[i].usUsage;
                        rid.dwFlags = devices[i].dwFlags &~ (RIDEV_NOLEGACY/* | RIDEV_NOHOTKEYS*/);
                        rid.hwndTarget = devices[i].hwndTarget;
                        RegisterRawInputDevicesHookNext(&rid, 1, sizeof(rid));
                    }
                }
            }
            free(devices);
        }
    }

    // restore Raw Input device states for the game
    void restoreRawInputDeviceFlags()
    {
        RAWINPUTDEVICE *devices=NULL;

        UINT numDevices=0;
        UINT res = GetRegisteredRawInputDevices(NULL, &numDevices, sizeof(RAWINPUTDEVICE));
        
        //IGOLogInfo("restoreRawInputDeviceFlags %i", numDevices);
        if (numDevices > 0)
        {
            devices = (RAWINPUTDEVICE *)malloc(numDevices*sizeof(RAWINPUTDEVICE));
            memset(devices, 0, numDevices*sizeof(RAWINPUTDEVICE));

            res = GetRegisteredRawInputDevices(devices, &numDevices, sizeof(RAWINPUTDEVICE));
            if (res != (UINT) -1)
            {
                for (UINT i = 0; i < numDevices; i++) 
                {
                    //IGOLogInfo("restoreRawInputDeviceFlags: uiDevices %i usUsage %i usUsagePage %i dwFlags %i", i, devices[i].usUsage, devices[i].usUsagePage, devices[i].dwFlags);
                    if( gRawMouse && (devices[i].usUsagePage == 0x01) && (devices[i].usUsage == 0x02 /*HID_DEVICE_SYSTEM_MOUSE*/) )
                    {
                        // unregister
                        RAWINPUTDEVICE rid;
                        rid.usUsagePage = devices[i].usUsagePage;
                        rid.usUsage = devices[i].usUsage;
                        rid.dwFlags = RIDEV_REMOVE;
                        rid.hwndTarget = devices[i].hwndTarget;
                        RegisterRawInputDevicesHookNext(&rid, 1, sizeof(rid));

                        // re-register
                        rid.usUsagePage = devices[i].usUsagePage;
                        rid.usUsage = devices[i].usUsage;
                        rid.dwFlags = gRawMouseState;
                        rid.hwndTarget = devices[i].hwndTarget;
                        RegisterRawInputDevicesHookNext(&rid, 1, sizeof(rid));
                    }

                    if( gRawKeyboard && (devices[i].usUsagePage == 0x01) && (devices[i].usUsage == 0x06 /*HID_DEVICE_SYSTEM_KEYBOARD*/) )
                    {
                        // unregister
                        RAWINPUTDEVICE rid;
                        rid.usUsagePage = devices[i].usUsagePage;
                        rid.usUsage = devices[i].usUsage;
                        rid.dwFlags = RIDEV_REMOVE;
                        rid.hwndTarget = devices[i].hwndTarget;
                        RegisterRawInputDevicesHookNext(&rid, 1, sizeof(rid));

                        // re-register
                        rid.usUsagePage = devices[i].usUsagePage;
                        rid.usUsage = devices[i].usUsage;
                        rid.dwFlags = gRawKeyboardState;
                        rid.hwndTarget = devices[i].hwndTarget;
                        RegisterRawInputDevicesHookNext(&rid, 1, sizeof(rid));
                    }
                }
            }
            free(devices);
        }
    }

    void releaseInputs()
    {
        // no need to allocate them for every call
        static INPUT keyDownSnapshot[256];
        static INPUT mouseButtonDownSnapshot[4];
        // reset counters
        int keyDownSnapshotCounter = 0;
        int mouseButtonDownSnapshotCounter = 0;

        // reset keyboard
        memset(keyDownSnapshot, 0, sizeof(keyDownSnapshot));
        for (size_t i=VK_BACK; i<=VK_OEM_CLEAR; i++)
        {
            if (gIGOKeyboardData[i] & 0x80 || gGetRawInputDataState[i] & 0x80)
            {
                keyDownSnapshot[keyDownSnapshotCounter].type = INPUT_KEYBOARD;
                keyDownSnapshot[keyDownSnapshotCounter].ki.wScan = (WORD)MapVirtualKey((UINT)i, MAPVK_VK_TO_VSC);
                keyDownSnapshot[keyDownSnapshotCounter].ki.wVk = (WORD)i;
                keyDownSnapshot[keyDownSnapshotCounter].ki.dwExtraInfo =  GetMessageExtraInfo();
                keyDownSnapshot[keyDownSnapshotCounter].ki.dwFlags = KEYEVENTF_KEYUP;
                keyDownSnapshotCounter++;
                // build our artificial key up messages
                if (IsWindowUnicode(gHwnd))
                {
                    PostMessageW(gHwnd, gIGOKeyUpMessage_WM_INPUT, RIM_TYPEKEYBOARD, i/*vKey*/);
                    PostMessageW(gHwnd, gIGOKeyUpMessage_WM_KEYUP, i/*vKey*/, 0);
                }
                else
                {
                    PostMessageA(gHwnd, gIGOKeyUpMessage_WM_INPUT, RIM_TYPEKEYBOARD, i/*vKey*/);
                    PostMessageA(gHwnd, gIGOKeyUpMessage_WM_KEYUP, i/*vKey*/, 0);
                }
            }
        }
        if(keyDownSnapshotCounter)
        {
            // ideally we would use this to generate WM_INPUT messages, but this also has an effect on GetAsyncKeyState / GetKeyState / GetKeyboardState
            // so we use our PostMessage way from above
        
            //SendInputFunc(keyDownSnapshotCounter, keyDownSnapshot, sizeof(INPUT));
        }

        // reset mouse buttons
        memset(mouseButtonDownSnapshot, 0, sizeof(mouseButtonDownSnapshot));

        mouseButtonDownSnapshot[mouseButtonDownSnapshotCounter].type = INPUT_MOUSE;
        mouseButtonDownSnapshot[mouseButtonDownSnapshotCounter].mi.dx = 0;
        mouseButtonDownSnapshot[mouseButtonDownSnapshotCounter].mi.dy = 0;
        mouseButtonDownSnapshot[mouseButtonDownSnapshotCounter].mi.mouseData = 0;
        mouseButtonDownSnapshot[mouseButtonDownSnapshotCounter].mi.time = 0;
        mouseButtonDownSnapshot[mouseButtonDownSnapshotCounter].mi.dwExtraInfo = 0;
        if(gCurrentLButtonDown || gIGOKeyboardData[VK_LBUTTON] & 0x80 || gGetRawInputDataState[VK_LBUTTON] & 0x80)
        {
            // build our artificial key up messages
            if (IsWindowUnicode(gHwnd))
                PostMessageW(gHwnd, gIGOKeyUpMessage_WM_INPUT, RIM_TYPEMOUSE, RI_MOUSE_LEFT_BUTTON_UP/*mouse button*/);
            else
                PostMessageA(gHwnd, gIGOKeyUpMessage_WM_INPUT, RIM_TYPEMOUSE, RI_MOUSE_LEFT_BUTTON_UP/*mouse button*/);

            mouseButtonDownSnapshot[mouseButtonDownSnapshotCounter].mi.dwFlags = (MOUSEEVENTF_LEFTUP);
            mouseButtonDownSnapshotCounter++;
        }


        mouseButtonDownSnapshot[mouseButtonDownSnapshotCounter].type = INPUT_MOUSE;
        mouseButtonDownSnapshot[mouseButtonDownSnapshotCounter].mi.dx = 0;
        mouseButtonDownSnapshot[mouseButtonDownSnapshotCounter].mi.dy = 0;
        mouseButtonDownSnapshot[mouseButtonDownSnapshotCounter].mi.mouseData = 0;
        mouseButtonDownSnapshot[mouseButtonDownSnapshotCounter].mi.time = 0;
        mouseButtonDownSnapshot[mouseButtonDownSnapshotCounter].mi.dwExtraInfo = 0;
        if(gCurrentMButtonDown || gIGOKeyboardData[VK_MBUTTON] & 0x80 || gGetRawInputDataState[VK_MBUTTON] & 0x80)
        {
            // build our artificial key up messages
            if (IsWindowUnicode(gHwnd))
                PostMessageW(gHwnd, gIGOKeyUpMessage_WM_INPUT, RIM_TYPEMOUSE, RI_MOUSE_MIDDLE_BUTTON_UP/*mouse button*/);
            else
                PostMessageA(gHwnd, gIGOKeyUpMessage_WM_INPUT, RIM_TYPEMOUSE, RI_MOUSE_MIDDLE_BUTTON_UP/*mouse button*/);

            mouseButtonDownSnapshot[mouseButtonDownSnapshotCounter].mi.dwFlags = (MOUSEEVENTF_MIDDLEUP);
            mouseButtonDownSnapshotCounter++;
        }

        mouseButtonDownSnapshot[mouseButtonDownSnapshotCounter].type = INPUT_MOUSE;
        mouseButtonDownSnapshot[mouseButtonDownSnapshotCounter].mi.dx = 0;
        mouseButtonDownSnapshot[mouseButtonDownSnapshotCounter].mi.dy = 0;
        mouseButtonDownSnapshot[mouseButtonDownSnapshotCounter].mi.mouseData = 0;
        mouseButtonDownSnapshot[mouseButtonDownSnapshotCounter].mi.time = 0;
        mouseButtonDownSnapshot[mouseButtonDownSnapshotCounter].mi.dwExtraInfo = 0;
        if(gCurrentRButtonDown || gIGOKeyboardData[VK_RBUTTON] & 0x80 || gGetRawInputDataState[VK_RBUTTON] & 0x80)
        {
            // build our artificial key up messages
            if (IsWindowUnicode(gHwnd))
                PostMessageW(gHwnd, gIGOKeyUpMessage_WM_INPUT, RIM_TYPEMOUSE, RI_MOUSE_RIGHT_BUTTON_UP/*mouse button*/);
            else
                PostMessageA(gHwnd, gIGOKeyUpMessage_WM_INPUT, RIM_TYPEMOUSE, RI_MOUSE_RIGHT_BUTTON_UP/*mouse button*/);

            mouseButtonDownSnapshot[mouseButtonDownSnapshotCounter].mi.dwFlags = (MOUSEEVENTF_RIGHTUP);
            mouseButtonDownSnapshotCounter++;
        }

        if(mouseButtonDownSnapshotCounter)
        {
            SendInputFunc(mouseButtonDownSnapshotCounter, mouseButtonDownSnapshot, sizeof(INPUT));
        }
    }

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    static BYTE gIGOKeyboardDataTypeLastSent[256] = {0};

    enum AutoCompleteKeyEventType
    {
        AutoCompleteKeyEventType_UNKNOWN = 0,
        AutoCompleteKeyEventType_SYSKEYDOWN,
        AutoCompleteKeyEventType_KEYDOWN,
        AutoCompleteKeyEventType_SYSKEYUP,
        AutoCompleteKeyEventType_KEYUP
    };

    void sendAutoCompleteModifierEvents(const MSG& msg, IGOIPC* ipc)
    {
        // Check the status of the modifier keys (Shift, Ctrl, Wnd) and send keydown/up events if necessary
        static BYTE AutoCompleteModifierVirtualKeyCodes[] = 
        {
            0x10, // VK_SHIFT
            0x11, // VK_CONTROL
            0x12, // VK_ALT
            0x5b, // LWIN
            0x5c, // RWIN
        };

        int count = sizeof(AutoCompleteModifierVirtualKeyCodes) / sizeof(BYTE);

        // First of, bail out if we are starting from a modifier message!
        for (int idx = 0; idx < count; ++idx)
        {
            if (AutoCompleteModifierVirtualKeyCodes[idx] == msg.wParam)
                return;
        }

        for (int idx = 0; idx < count; ++idx)
        {
            BYTE vKeyCode = AutoCompleteModifierVirtualKeyCodes[idx];
            if (gIGOKeyboardData[vKeyCode] & 0x80 || gGetRawInputDataState[vKeyCode] & 0x80)
            {
                if (gIGOKeyboardDataTypeLastSent[vKeyCode] != AutoCompleteKeyEventType_KEYDOWN)
                {
                    UINT scanCode = MapVirtualKey(vKeyCode, MAPVK_VK_TO_VSC);
                    if (scanCode)
                    {
                        // Bit template for keydown message:
                        // 0-15: repeat count
                        // 16-23: scan code
                        // 24: extended key (right-hand ALT/CTRL)
                        // 25-28: reserved
                        // 29: context code (0)
                        // 30: previous key state - 1 is down, 0 if up
                        // 31: transition state (0)
                        UINT lParam = scanCode << 16;
                        lParam |= 1;            // repeat count = 1
                        if (vKeyCode == VK_RCONTROL)
                            lParam |= 1 << 24;      // extended key

                        eastl::shared_ptr<IGOIPCMessage> msg (ipc->createMsgInputEvent(IGOIPC::EVENT_KEYBOARD, WM_KEYDOWN, (uint32_t)vKeyCode, (uint32_t)lParam));
                        SAFE_CALL(IGOApplication::instance(), &IGOApplication::send, msg);

                        gIGOKeyboardDataTypeLastSent[vKeyCode] = AutoCompleteKeyEventType_KEYDOWN;
                    }
                }
            }
        }
    }

    void sendAutoCompleteKeyboardEvents(const MSG& msg, IGOIPC* ipc)
    {
        // Some games (think Osu!) may handle the keyboard events in a separate window (different from the render/hooked window) and may only pass along key up events
        // -> make sure we send a single key down event if missing
        UINT uMsg = msg.message;
        WPARAM wParam = msg.wParam;

        if (wParam < sizeof(gIGOKeyboardDataTypeLastSent))
        {
            switch (uMsg)
            {
                case WM_SYSKEYDOWN:
                    gIGOKeyboardDataTypeLastSent[wParam] = AutoCompleteKeyEventType_SYSKEYDOWN;
                    break;

                case WM_KEYDOWN:
                    gIGOKeyboardDataTypeLastSent[wParam] = AutoCompleteKeyEventType_KEYDOWN;
                    break;

                case WM_SYSKEYUP:
                    if (gIGOKeyboardDataTypeLastSent[wParam] != AutoCompleteKeyEventType_SYSKEYDOWN)
                    {
                        // Since we have incomplete messages, we'd better check the modifier keys first! (Shift, Ctrl, Wnd)
                        sendAutoCompleteModifierEvents(msg, ipc);

                        MSG acMsg = msg;
                        acMsg.message = WM_SYSKEYDOWN;
                        acMsg.lParam &= 0x3fffffff; // reset upper bits for keydown state

                        eastl::shared_ptr<IGOIPCMessage> msg (ipc->createMsgInputEvent(IGOIPC::EVENT_KEYBOARD, acMsg.message, (uint32_t)acMsg.wParam, (uint32_t)acMsg.lParam));
                        SAFE_CALL(IGOApplication::instance(), &IGOApplication::send, msg);
                    }

                    gIGOKeyboardDataTypeLastSent[wParam] = AutoCompleteKeyEventType_SYSKEYUP;
                    break;

                case WM_KEYUP:
                    if (gIGOKeyboardDataTypeLastSent[wParam] != AutoCompleteKeyEventType_KEYDOWN)
                    {
                        // Since we have incomplete messages, we'd better check the modifier keys first! (Shift, Ctrl, Wnd)
                        sendAutoCompleteModifierEvents(msg, ipc);

                        MSG acMsg = msg;
                        acMsg.message = WM_KEYDOWN;
                        acMsg.lParam &= 0x3fffffff; // reset upper bits for keydown state

                        eastl::shared_ptr<IGOIPCMessage> msg (ipc->createMsgInputEvent(IGOIPC::EVENT_KEYBOARD, acMsg.message, (uint32_t)acMsg.wParam, (uint32_t)acMsg.lParam));
                        SAFE_CALL(IGOApplication::instance(), &IGOApplication::send, msg);
                    }

                    gIGOKeyboardDataTypeLastSent[wParam] = AutoCompleteKeyEventType_SYSKEYUP;
                    break;

                default:
                    break;
            }
        }
    }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    bool filterMessages(LPMSG lpMsg, bool discardKeyboardData, bool autoCompleteKeyEvents)
    {
        if (!lpMsg || gNoMoreFiltering)
            return false;
        
        HWND hwnd = lpMsg->hwnd;
        UINT uMsg = lpMsg->message;
        WPARAM wParam = lpMsg->wParam;
        LPARAM lParam = lpMsg->lParam;

        // if our main window is about to be destroyed, destroy IGO too 
        if ((uMsg == WM_NCDESTROY || uMsg == WM_DESTROY || uMsg == WM_QUIT) && gHwnd != NULL && hwnd == gHwnd)
        {
            IGOLogWarn("filterMessages WM_NCDESTROY / WM_DESTROY / WM_QUIT received (%p %x).", hwnd, uMsg);
            gNoMoreFiltering = true;
            IGOCleanup();
            IGOReInit();
            return false;
        }
    
        SAFE_CALL_LOCK_AUTO

        bool hasInstance = IGOApplication::instance()!=NULL;
        bool isActive = hasInstance==true ? IGOApplication::instance()->isActive() : false;
        if (hasInstance)
        {
            // always update our IGO internal keyboard state buffer
            if (!SAFE_CALL(IGOApplication::instance(), &IGOApplication::languageChangeInProgress))
            {
                DWORD now = GetTickCount();
                if ((now - gLastKeyboardSync >= KEYBOARD_POLL_RATE) ||
                    (uMsg>=WM_KEYFIRST && uMsg<=WM_KEYLAST))
                {
                    for(int i=0; i<256; i++)
                        gIGOKeyboardData[i] = GetAsyncKeyStateFunc(i) & 0x8000 ? 0x80 : 0x0;
                    gLastKeyboardSync = now;
                }
            }
            
            if (!isActive)    // if IGO is not activated or the IGO featureing game is not active(in foreground), do not filter WM messages!!!
                return false;

            IGOIPC *ipc = IGOIPC::instance();

            switch (uMsg)
            {
            case WM_MOUSEMOVE:
                {
                    int32_t x = LOWORD(lParam);
                    int32_t y = HIWORD(lParam);
                    if (gMousePos.x != x || gMousePos.y != y)
                    {

                        float sX=SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScalingX);
                        float sY=SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScalingY);

                        // get the gaming window top left coordinate
                        RECT rect={0};
                        GetClientRect(hwnd, &rect);
                        POINT windowOffset;
                        windowOffset.x = rect.left;
                        windowOffset.y = rect.top;
                        ClientToScreen(hwnd, &windowOffset);
                        

                        eastl::shared_ptr<IGOIPCMessage> msg (ipc->createMsgInputEvent(IGOIPC::EVENT_MOUSE_MOVE,
                            SAFE_CALL(IGOApplication::instance(), &IGOApplication::getOffsetX) +(int)(((float)x)*sX), SAFE_CALL(IGOApplication::instance(), &IGOApplication::getOffsetY)+(int)(((float)y)*sY),
                            0,
                            MAKELONG(windowOffset.x, windowOffset.y),
                            sX, sY));

                        SAFE_CALL(IGOApplication::instance(), &IGOApplication::send, msg);


                        gMousePos.x = x;
                        gMousePos.y = y;
                    }
                }
                break;

            // handle dbl click messages
            case WM_NCRBUTTONDBLCLK:
            case WM_RBUTTONDBLCLK:
                {
                    // send right double click

                    eastl::shared_ptr<IGOIPCMessage> msg (ipc->createMsgInputEvent(IGOIPC::EVENT_MOUSE_RIGHT_DOUBLE_CLICK, LOWORD(lParam), HIWORD(lParam)));
                    SAFE_CALL(IGOApplication::instance(), &IGOApplication::send, msg);

                    gLastRButtonUp = 0;
                    gCurrentRButtonDown = gCurrentNCRButtonDown = false;
                }
                break;

            case WM_NCLBUTTONDBLCLK:
            case WM_LBUTTONDBLCLK:
                {
                    // send left double click

                    eastl::shared_ptr<IGOIPCMessage> msg (ipc->createMsgInputEvent(IGOIPC::EVENT_MOUSE_LEFT_DOUBLE_CLICK, LOWORD(lParam), HIWORD(lParam)));
                    SAFE_CALL(IGOApplication::instance(), &IGOApplication::send, msg);

                    gLastLButtonUp = 0;
                    gCurrentLButtonDown = gCurrentNCLButtonDown = false;
                }
                break;

            case WM_NCLBUTTONDOWN:
                {
                    if (!gCurrentNCLButtonDown)
                    {
                        gCurrentNCLButtonDown = true;
                    }
                }
                break;

            case WM_LBUTTONDOWN:
                {
                    if (!gCurrentLButtonDown)
                    {

                        eastl::shared_ptr<IGOIPCMessage> msg (ipc->createMsgInputEvent(IGOIPC::EVENT_MOUSE_LEFT_DOWN, LOWORD(lParam), HIWORD(lParam)));
                        SAFE_CALL(IGOApplication::instance(), &IGOApplication::send, msg);

                        gCurrentLButtonDown = true;
                    }
                }
                break;

            case WM_NCLBUTTONUP:
            case WM_LBUTTONUP:
                {
                    if (gCurrentLButtonDown || gCurrentNCLButtonDown)
                    {
                        
                        gCurrentNCLButtonDown = false;


                        eastl::shared_ptr<IGOIPCMessage> msg (ipc->createMsgInputEvent(IGOIPC::EVENT_MOUSE_LEFT_UP, LOWORD(lParam), HIWORD(lParam)));
                        SAFE_CALL(IGOApplication::instance(), &IGOApplication::send, msg);


                        gCurrentLButtonDown = false;

                        // special dbl click handling if the game window does not support CS_DBLCLKS
                        DWORD now = GetTickCount();
                        if (now - gLastLButtonUp < DOUBLE_CLICK_DT)
                        {
                            // send left double click

                            eastl::shared_ptr<IGOIPCMessage> msg (ipc->createMsgInputEvent(IGOIPC::EVENT_MOUSE_LEFT_DOUBLE_CLICK, LOWORD(lParam), HIWORD(lParam)));
                            SAFE_CALL(IGOApplication::instance(), &IGOApplication::send, msg);

                            gLastLButtonUp = 0;
                        }
                        else
                        {
                            gLastLButtonUp = now;
                        }

                        return false; // pass the mouse button up event to the game - always
                    }
                }
                break;

            case WM_NCRBUTTONDOWN:
                {
                    if (!gCurrentNCRButtonDown)
                    {
                        gCurrentNCRButtonDown = true;
                    }
                }
                break;

            case WM_RBUTTONDOWN:
                {
                    if (!gCurrentRButtonDown)
                    {

                        eastl::shared_ptr<IGOIPCMessage> msg (ipc->createMsgInputEvent(IGOIPC::EVENT_MOUSE_RIGHT_DOWN, LOWORD(lParam), HIWORD(lParam)));
                        SAFE_CALL(IGOApplication::instance(), &IGOApplication::send, msg);

                        gCurrentRButtonDown = true;
                    }
                }
                break;

            case WM_RBUTTONUP:
                {
                    if (gCurrentRButtonDown || gCurrentNCRButtonDown)
                    {
                        gCurrentNCRButtonDown = false;


                        eastl::shared_ptr<IGOIPCMessage> msg (ipc->createMsgInputEvent(IGOIPC::EVENT_MOUSE_RIGHT_UP, LOWORD(lParam), HIWORD(lParam)));
                        SAFE_CALL(IGOApplication::instance(), &IGOApplication::send, msg);

                        gCurrentRButtonDown = false;

                        // special dbl click handling if the game window does not support CS_DBLCLKS
                        DWORD now = GetTickCount();
                        if (now - gLastRButtonUp < DOUBLE_CLICK_DT)
                        {
                            // send right double click

                            eastl::shared_ptr<IGOIPCMessage> msg (ipc->createMsgInputEvent(IGOIPC::EVENT_MOUSE_RIGHT_DOUBLE_CLICK, LOWORD(lParam), HIWORD(lParam)));
                            SAFE_CALL(IGOApplication::instance(), &IGOApplication::send, msg);
 
                            gLastRButtonUp = 0;
                        }
                        else
                        {
                            gLastRButtonUp = now;
                        }

                        return false; // pass the mouse button up event to the game - always
                    }
                }
                break;
            
            case WM_NCMBUTTONDOWN:
                {
                    if (!gCurrentNCMButtonDown)
                    {
                        gCurrentNCMButtonDown = true;
                    }
                }
                break;

            case WM_MBUTTONDOWN:
                {
                    if (!gCurrentMButtonDown)
                    {
                        gCurrentMButtonDown = true;
                    }
                }
                break;

            case WM_MBUTTONUP:
                {
                    if (gCurrentMButtonDown || gCurrentNCMButtonDown)
                    {
                        gCurrentNCMButtonDown = false;

                        gCurrentMButtonDown = false;

                        return false; // pass the mouse button up event to the game - always
                    }
                }

                break;

            case WM_MOUSEWHEEL:
                {
                    
                    POINT pt = { LOWORD(lParam), HIWORD(lParam) };
                    ScreenToClient(hwnd, &pt);

                    eastl::shared_ptr<IGOIPCMessage> msg (ipc->createMsgInputEvent(IGOIPC::EVENT_MOUSE_WHEEL, pt.x, pt.y, (SHORT)HIWORD(wParam)));
                    SAFE_CALL(IGOApplication::instance(), &IGOApplication::send, msg);

                }
                break;
            case WM_DEADCHAR:
            case WM_SYSDEADCHAR:
            case WM_SYSCHAR:
            case WM_CHAR:
            case WM_SYSKEYDOWN:
            case WM_KEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYUP:
    #if(_WIN32_WINNT >= 0x0501)        
            case WM_UNICHAR:
    #endif
                {

                    if (!SAFE_CALL(IGOApplication::instance(), &IGOApplication::languageChangeInProgress) && !discardKeyboardData && !SAFE_CALL(IGOApplication::instance(), &IGOApplication::isHotKeyPressed))    // hide IGO hotkey key down events from IGO windows
                    {
						// Is this a game that sends all partial key events (ie only keyup events?)
                        if (autoCompleteKeyEvents)
                            sendAutoCompleteKeyboardEvents(*lpMsg, ipc);

                        eastl::shared_ptr<IGOIPCMessage> msg (ipc->createMsgInputEvent(IGOIPC::EVENT_KEYBOARD, uMsg, (uint32_t)wParam, (uint32_t)lParam));
                        SAFE_CALL(IGOApplication::instance(), &IGOApplication::send, msg);
                    }

                }
                break;
            case WM_INPUT:
                    return false; // handle this message in the GetRawInput calls
                break;

            case WM_IME_KEYUP:
            case WM_SETCURSOR:
            case WM_IME_CHAR:
            case WM_IME_KEYDOWN:
                return true;
            case WM_INPUTLANGCHANGE:

                SAFE_CALL(IGOApplication::instance(), &IGOApplication::setIGOKeyboardLayout,  (HKL)lParam);

                return true;
            }
            // we block the mouse and keyboard
            if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)
                return true;

            if (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST)
                return true;
        }
        return false;
    }


    static DWORD WINAPI IGOInputHookCreateThread(LPVOID *lpParam)
    {
		IGO_TRACE("In InputHook thread ...");
        if (lpParam !=NULL )
            *((bool*)lpParam) = InputHook::TryHook();
        else
            InputHook::TryHook();
        
        CloseHandle(InputHook::mIGOInputhookCreateThreadHandle);
        InputHook::mIGOInputhookCreateThreadHandle = NULL;
        return 0;
    }


    void InputHook::TryHookLater(bool *isHooked)
    {
        // don't spam the system with this thread, execute it every 15 seconds only!!!
        DWORD now = GetTickCount();
        if (now - gLastInputHookAttempt >= REHOOKCHECK_DELAY)
        {
            gLastInputHookAttempt = now;

            if (InputHook::mIGOInputhookCreateThreadHandle==NULL)
			{
				IGO_TRACE("Starting InputHook thread");
				InputHook::mIGOInputhookCreateThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)IGOInputHookCreateThread, isHooked, 0, NULL);
			}
        }
    }

	void HookWindowsEventLoopAPI()
	{
		if (!gWin32Hooked)
		{
			IGO_TRACE("Hooking Windows event loop API");

            HOOKAPI_SAFE("user32.dll", "RegisterRawInputDevices", RegisterRawInputDevicesHook);
            HOOKAPI_SAFE("user32.dll", "BlockInput", BlockInputHook);

            HOOKAPI_SAFE("user32.dll", "SetWindowsHookExA", SetWindowsHookExAHook);
            HOOKAPI_SAFE("user32.dll", "SetWindowsHookExW", SetWindowsHookExWHook);
            HOOKAPI_SAFE("user32.dll", "UnhookWindowsHookEx", UnhookWindowsHookExHook);

            HOOKAPI_SAFE("user32.dll", "MessageBoxA", MessageBoxAHook);
            HOOKAPI_SAFE("user32.dll", "MessageBoxW", MessageBoxWHook);
            HOOKAPI_SAFE("user32.dll", "SetCursor", SetCursorHook);
            HOOKAPI_SAFE("user32.dll", "ShowCursor", ShowCursorHook);
            HOOKAPI_SAFE("user32.dll", "SetCursorPos", SetCursorPosHook);
            HOOKAPI_SAFE("user32.dll", "GetCursorPos", GetCursorPosHook);
            HOOKAPI_SAFE("user32.dll", "GetAsyncKeyState", GetAsyncKeyStateHook);
            HOOKAPI_SAFE("user32.dll", "GetKeyState", GetKeyStateHook);
            HOOKAPI_SAFE("user32.dll", "GetKeyboardState", GetKeyboardStateHook);
        
            HOOKAPI_SAFE("user32.dll", "GetRawInputData", GetRawInputDataHook);
            HOOKAPI_SAFE("user32.dll", "GetRawInputBuffer", GetRawInputBufferHook);

            HOOKAPI_SAFE("user32.dll", "keybd_event", keybd_eventHook);
            HOOKAPI_SAFE("user32.dll", "mouse_event", mouse_eventHook);
            HOOKAPI_SAFE("user32.dll", "SendInput", SendInputHook);
                   
            HOOKAPI_SAFE("user32.dll", "DispatchMessageA", DispatchMessageAHook);
            HOOKAPI_SAFE("user32.dll", "DispatchMessageW", DispatchMessageWHook);
            HOOKAPI_SAFE("user32.dll", "TranslateMessage", TranslateMessageHook);

            HOOKAPI_SAFE("user32.dll", "PeekMessageA", PeekMessageAHook);
            HOOKAPI_SAFE("user32.dll", "PeekMessageW", PeekMessageWHook);
                    
            HOOKAPI_SAFE("user32.dll", "GetMessageA", GetMessageAHook);
            HOOKAPI_SAFE("user32.dll", "GetMessageW", GetMessageWHook);

            HOOKAPI_SAFE("user32.dll", "SendMessageA", SendMessageAHook);
            HOOKAPI_SAFE("user32.dll", "SendMessageW", SendMessageWHook);
            HOOKAPI_SAFE("user32.dll", "SendMessageCallbackA", SendMessageCallbackAHook);
            HOOKAPI_SAFE("user32.dll", "SendMessageCallbackW", SendMessageCallbackWHook);
            HOOKAPI_SAFE("user32.dll", "SendMessageTimeoutA", SendMessageTimeoutAHook);
            HOOKAPI_SAFE("user32.dll", "SendMessageTimeoutW", SendMessageTimeoutWHook);
            HOOKAPI_SAFE("user32.dll", "SendNotifyMessageA", SendNotifyMessageAHook);
            HOOKAPI_SAFE("user32.dll", "SendNotifyMessageW", SendNotifyMessageWHook);

            if (GetModuleHandle(L"user32.dll") && GetModuleHandle(L"kernel32.dll"))
                gWin32Hooked = true;
        }
	}

    bool InputHook::TryHook()
    {
        if (InputHook::mInstanceHookMutex.TryLock())
        {
            if (mInputHookState==1)
            {
                InputHook::mInstanceHookMutex.Unlock();
                return false;
            }

            OriginIGO::IGOLogInfo("InputHook::TryHook");

            mInputHookState=1;

			HookWindowsEventLoopAPI();

            if (!isXInputGetKeystrokeHooked && !isXInputGetStateHooked)
            {
                if (GetModuleHandle(L"xinput1_1.dll"))
                {
                    HOOKAPI_SAFE("xinput1_1.dll", "XInputGetKeystroke", XInputGetKeystrokeHook);
                    HOOKAPI_SAFE("xinput1_1.dll", "XInputGetState", XInputGetStateHook);
                }
                else
                if (!isXInputGetKeystrokeHooked && !isXInputGetStateHooked && GetModuleHandle(L"xinput1_2.dll"))
                {
                    HOOKAPI_SAFE("xinput1_2.dll", "XInputGetKeystroke", XInputGetKeystrokeHook);
                    HOOKAPI_SAFE("xinput1_2.dll", "XInputGetState", XInputGetStateHook);
                }
                else
                if (!isXInputGetKeystrokeHooked && !isXInputGetStateHooked && GetModuleHandle(L"xinput1_3.dll"))
                {
                    HOOKAPI_SAFE("xinput1_3.dll", "XInputGetKeystroke", XInputGetKeystrokeHook);
                    HOOKAPI_SAFE("xinput1_3.dll", "XInputGetState", XInputGetStateHook);
                }
                else
                if (!isXInputGetKeystrokeHooked && !isXInputGetStateHooked && GetModuleHandle(L"XInput9_1_0.dll"))
                {
                    HOOKAPI_SAFE("XInput9_1_0.dll", "XInputGetKeystroke", XInputGetKeystrokeHook);
                    HOOKAPI_SAFE("XInput9_1_0.dll", "XInputGetState", XInputGetStateHook);
                }
            }

            if (!gDInputHooked)
            {
                HMODULE hDI = NULL;
                if ((hDI = GetModuleHandle(L"dinput.dll")) == NULL)
                {
                    if ((hDI = GetModuleHandle(L"dinput8.dll")) == NULL)
                    {
                        // nothing
                        IGOLogWarn("input hooks: no DirectInput found.");
                    }
                    else
                    {
                        gDInputHooked = true;
                        DInputInit8(hDI);
                    }
                }
                else
                {
                    gDInputHooked = true;
                    DInputInit(hDI);
                }
            }

            IGOLogInfo("input hooks: w %i x %i d %i\n", gWin32Hooked, isXInputGetKeystrokeHooked && isXInputGetStateHooked, gDInputHooked);

            mInputHookState = 0;
            InputHook::mInstanceHookMutex.Unlock();
        }

        if (gWin32Hooked)   // win32 is mandatory, XInput and DInput optional!
            return true;
        else
            return false;
    }


    static DWORD WINAPI IGOInputHookDestroyThread(LPVOID *lpParam)
    {
        InputHook::Cleanup();
        return 0;
    }


    void InputHook::CleanupLater()
    {
        EA::Thread::AutoFutex m(mInstanceCleanupMutex);

        if (mIGOInputhookDestroyThreadHandle)
        {
            DWORD exitCode = 0;

            if (GetExitCodeThread(mIGOInputhookDestroyThreadHandle, &exitCode))
            {
                if (exitCode != STILL_ACTIVE)
                {    // if the thread is no longer active, close the handle
                    CloseHandle(InputHook::mIGOInputhookDestroyThreadHandle);
                    InputHook::mIGOInputhookDestroyThreadHandle = NULL;
                }
            }
        }

        if (InputHook::mIGOInputhookDestroyThreadHandle == NULL)
            InputHook::mIGOInputhookDestroyThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)IGOInputHookDestroyThread, NULL, 0, NULL);
    }

    void InputHook::FinishCleanup()
    {
        OriginIGO::IGOLogInfo("InputHook::FinishCleanup called");
        EA::Thread::AutoFutex m(mInstanceCleanupMutex);

        // this check is thread-safe because InputHook::mIGOInputhookDestroyThreadHandle is not cleared within the thread anymore
        if (InputHook::mIGOInputhookDestroyThreadHandle && ::WaitForSingleObject(InputHook::mIGOInputhookDestroyThreadHandle, 500) == WAIT_OBJECT_0)
        {
            CloseHandle(InputHook::mIGOInputhookDestroyThreadHandle);
            InputHook::mIGOInputhookDestroyThreadHandle = NULL;
        }
        /*
        else
        {
            if (InputHook::mIGOInputhookDestroyThreadHandle)
            {
                TerminateThread(InputHook::mIGOInputhookDestroyThreadHandle, 0);
                CloseHandle(InputHook::mIGOInputhookDestroyThreadHandle);
                InputHook::mIGOInputhookDestroyThreadHandle = NULL;
                OriginIGO::IGOLogWarn("IGOInputHookDestroyThread failed to terminate normally...");
            }
        }
        */
    }

    void InputHook::Cleanup()
    {
        EA::Thread::AutoFutex m(InputHook::mInstanceHookMutex);
        
        if (mInputHookState==-1)
            return;
        
        OriginIGO::IGOLogInfo("InputHook::Cleanup");

        mInputHookState = -1;

        if (mPreWndProcObserverHandle)
        {
            if (!UnhookWindowsHookExFunc(mPreWndProcObserverHandle))
                IGOLogWarn("Failed to unhook PreWndProc observer (0x%08x)", GetLastError());

            mPreWndProcObserverHandle = NULL;
        }

        if (mPostWndProcObserverHandle)
        {
            if (!UnhookWindowsHookExFunc(mPostWndProcObserverHandle))
                IGOLogWarn("Failed to unhook PostWndProc observer (0x%08x)", GetLastError());

            mPostWndProcObserverHandle = NULL;
        }

#if 0
        UNHOOK_SAFE(XInputGetKeystrokeHook);
        UNHOOK_SAFE(XInputGetStateHook);

        UNHOOK_SAFE(GetDeviceStateHook);
        UNHOOK_SAFE(GetDeviceDataHook);
        UNHOOK_SAFE(SetCooperativeLevelHook);

        UNHOOK_SAFE(GetDeviceState8Hook);
        UNHOOK_SAFE(GetDeviceData8Hook);
        UNHOOK_SAFE(SetCooperativeLevel8Hook);

        UNHOOK_SAFE(RegisterRawInputDevicesHook);
        UNHOOK_SAFE(BlockInputHook);

        UNHOOK_SAFE(SetWindowsHookExAHook);
        UNHOOK_SAFE(SetWindowsHookExWHook);
        UNHOOK_SAFE(UnhookWindowsHookExHook);

        UNHOOK_SAFE(MessageBoxAHook);
        UNHOOK_SAFE(MessageBoxWHook);
        UNHOOK_SAFE(SetCursorHook);
        UNHOOK_SAFE(ShowCursorHook);
        UNHOOK_SAFE(SetCursorPosHook);
        UNHOOK_SAFE(GetCursorPosHook);
        UNHOOK_SAFE(GetAsyncKeyStateHook);
        UNHOOK_SAFE(GetKeyStateHook);
        UNHOOK_SAFE(GetKeyboardStateHook);
        
        UNHOOK_SAFE(GetRawInputDataHook);
        UNHOOK_SAFE(GetRawInputBufferHook);

        UNHOOK_SAFE(keybd_eventHook);
        UNHOOK_SAFE(mouse_eventHook);
        UNHOOK_SAFE(SendInputHook);

        UNHOOK_SAFE(DispatchMessageAHook);
        UNHOOK_SAFE(DispatchMessageWHook);
        UNHOOK_SAFE(TranslateMessageHook);

        UNHOOK_SAFE(PeekMessageAHook);
        UNHOOK_SAFE(PeekMessageWHook);

        UNHOOK_SAFE_GETMESSAGE(GetMessageAHook);
        UNHOOK_SAFE_GETMESSAGE(GetMessageWHook);

        UNHOOK_SAFE(SendMessageAHook);
        UNHOOK_SAFE(SendMessageWHook);
        UNHOOK_SAFE(SendMessageCallbackAHook);
        UNHOOK_SAFE(SendMessageCallbackWHook);
        UNHOOK_SAFE(SendMessageTimeoutAHook);
        UNHOOK_SAFE(SendMessageTimeoutWHook);
        UNHOOK_SAFE(SendNotifyMessageAHook);
        UNHOOK_SAFE(SendNotifyMessageWHook);

        gWin32Hooked = false;
        gDInputHooked = false;
        isXInputGetKeystrokeHooked = false;
    
        gInputHooked = false;
#endif
        gLastInputHookAttempt = 0;  // reset our input hooking timeout, if we intentionally unhooked the APIs!
        mInputHookState = 0;
        mWindowHooked = false;
        gHwnd = NULL;
    }


    bool InputHook::IsMessagHookActive()
    {
        return gHwnd != NULL;
    }

    void InputHook::EnableWindowHooking()
    {
         gNoMoreFiltering = false;
    }

    void InputHook::HookWindow(HWND hwnd)
    {
        if (!hwnd)
            return;

        if (InputHook::mInstanceHookMutex.TryLock())
        {
            if (mInputHookState == 0)
            {
                if (((!mWindowHooked || gHwnd != hwnd || gNoMoreFiltering == true) && (hwnd != NULL && IsWindow(hwnd) && !IsSystemWindow(hwnd))))
                {
                    if (IsWindowUnicode(hwnd))
                        PostMessageW(hwnd, OriginIGO::gIGOHookWindowMessage, 0, 0);
                    else
                        PostMessageA(hwnd, OriginIGO::gIGOHookWindowMessage, 0, 0);
                }
            }

            InputHook::mInstanceHookMutex.Unlock();
        }
    }

    void InputHook::HookWindowImpl(HWND hwnd)
    {
        if (!hwnd)
            return;

        if (InputHook::mInstanceHookMutex.TryLock())
        {
            if ((!InputHook::mWindowHooked || gHwnd != hwnd || gNoMoreFiltering == true) && (IsWindow(hwnd) && !IsSystemWindow(hwnd)))
            {
                if (mPreWndProcObserverHandle)
                {
                    if (!UnhookWindowsHookExFunc(mPreWndProcObserverHandle))
                        IGOLogWarn("Failed to unhook PreWndProc observer (0x%08x)", GetLastError());

                    mPreWndProcObserverHandle = NULL;
                }

                if (mPostWndProcObserverHandle)
                {
                    if (!UnhookWindowsHookExFunc(mPostWndProcObserverHandle))
                        IGOLogWarn("Failed to unhook PostWndProc observer (0x%08x)", GetLastError());

                    mPostWndProcObserverHandle = NULL;
                }

                gHwnd = hwnd;
                HookWindowsEventLoopAPI();

                IGOLogInfo("Observing new window=%p", gHwnd);

                if (IsWindowUnicode(hwnd))
                {
                    mPreWndProcObserverHandle = SetWindowsHookExWFunc(WH_CALLWNDPROC, PreWndProcObserverFunc, NULL, GetWindowThreadProcessId(hwnd, NULL));
                    if (!mPreWndProcObserverHandle)
                        IGOLogWarn("Failed to hook PreWndProc observer for wnd=%p (0x%08x)", hwnd, GetLastError());

                    mPostWndProcObserverHandle = SetWindowsHookExWFunc(WH_CALLWNDPROCRET, PostWndProcObserverFunc, NULL, GetWindowThreadProcessId(hwnd, NULL));
                    if (!mPostWndProcObserverHandle)
                        IGOLogWarn("Failed to hook PostWndProc observer for wnd=%p (0x%08x)", hwnd, GetLastError());
                }

                else
                {
                    mPreWndProcObserverHandle = SetWindowsHookExAFunc(WH_CALLWNDPROC, PreWndProcObserverFunc, NULL, GetWindowThreadProcessId(hwnd, NULL));
                    if (!mPreWndProcObserverHandle)
                        IGOLogWarn("Failed to hook PreWndProc observer for wnd=%p (0x%08x)", hwnd, GetLastError());

                    mPostWndProcObserverHandle = SetWindowsHookExAFunc(WH_CALLWNDPROCRET, PostWndProcObserverFunc, NULL, GetWindowThreadProcessId(hwnd, NULL));
                    if (!mPostWndProcObserverHandle)
                        IGOLogWarn("Failed to hook PostWndProc observer for wnd=%p (0x%08x)", hwnd, GetLastError());
                }

                gNoMoreFiltering = false;
                mWindowHooked = true;
            }

            InputHook::mInstanceHookMutex.Unlock();
        }
    }
}

#pragma optimize( "", off )

#endif // ORIGIN_PC
