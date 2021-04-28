//
//  InputHook.m
//  InjectedCode
//
//  Created by Frederic Meraud on 6/25/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "MacInputHook.h"

#if defined(ORIGIN_MAC)

#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CGEventTypes.h>
#import <IOKit/hid/IOHIDLibObsolete.h>

#include "MacWindows.h"
#include "IGO.h"
#include "IGOIPC/IGOIPC.h"
#include "IGOIPC/IGOIPCConnection.h"

#include "HookAPI.h"
#include "MacJRSwizzle.h"
#include "MacCursorHook.h"
#include "MacRenderHook.h"
#include "IGOApplication.h"
//#include "MacSystemPreferences.h"
#include </usr/include/libkern/OSAtomic.h>

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1090
typedef uint32_t CGTableCount;
#endif

class InputTapContext;
static InputTapContext*     _inputTapContext = NULL;

static CFMachPortRef        _inputTap = NULL;
static CFRunLoopSourceRef   _inputRunLoopSource = NULL;
static CFRunLoopRef         _inputRunLoop = NULL;

//#define STARTED_IMPL_WITH_NO_TAP

// Right now, we support two sets of hot keys:
// 1) to turn OIG on/off
// 2) to turn pinned widgets on/off
enum HotKeySet
{
    HotKeySet_OIG = 0,
    HotKeySet_PINNED_WIDGETS,
    HotKeySet_APPLE_SWITCH_APP,
    HotKeySet_COUNT
};

// Set of modifier keys to store/restore when switching OIG on/off
static const BYTE HotKeysModifiersKeySet[] = { kVK_Option, kVK_RightOption, kVK_Control, kVK_RightControl, kVK_Shift, kVK_RightShift, kVK_Command, kVK_Function, kVK_CapsLock };

// Fwd decls
extern void (*volatile CGSGetKeysHookNext)(KeyMap theKeys);

/////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_HOOK(CFMachPortRef, CGEventTapCreateHook, (CGEventTapLocation tap, CGEventTapPlacement place, CGEventTapOptions options, CGEventMask eventsOfInterest, CGEventTapCallBack callback, void* refcon))
{
    IGO_TRACE("Calling CGEventTapCreate...");
    return CGEventTapCreateHookNext(tap, place, options, eventsOfInterest, callback, refcon);
}

DEFINE_HOOK(CFMachPortRef, CGEventTapCreateForPSNHook, (void* processSerialNumber, CGEventTapPlacement place, CGEventTapOptions options, CGEventMask eventsOfInterest, CGEventTapCallBack callback, void* refcon))
{
    IGO_TRACE("Calling CGEventTapCreateForPSN...");
    return CGEventTapCreateForPSNHookNext(processSerialNumber, place, options, eventsOfInterest, callback, refcon);
}

DEFINE_HOOK(void, CGEventTapEnableHook, (CFMachPortRef myTap, bool enable))
{
    IGO_TRACEF("Calling CGEventTapEnable (%p / %d)...", myTap, enable ? 1 : 0);
    CGEventTapEnableHookNext(myTap, enable);
}

DEFINE_HOOK(bool, CGEventTapIsEnabledHook, (CFMachPortRef myTap))
{
    IGO_TRACE("Calling CGEventTapIsEnabled...");
    return CGEventTapIsEnabledHookNext(myTap);
}

DEFINE_HOOK(void, CGEventTapPostEventHook, (CGEventTapProxy proxy, CGEventRef event))
{
    IGO_TRACE("Calling CGEventTapPostEvent...");
    CGEventTapPostEventHookNext(proxy, event);
}

DEFINE_HOOK(void, CGEventPostHook, (CGEventTapLocation tap, CGEventRef event))
{
    IGO_TRACE("Calling CGEventPost...");
    CGEventPostHookNext(tap, event);
}

DEFINE_HOOK(void, CGEventPostToPSNHook, (void* processSerialNumber, CGEventRef event))
{
    IGO_TRACE("Calling CGEventPostToPSN...");
    CGEventPostToPSNHookNext(processSerialNumber, event);
}

DEFINE_HOOK(CGError, CGGetEventTapListHook, (CGTableCount maxNumberOfTaps, CGEventTapInformation tapList[], CGTableCount* eventTapCount))
{
    IGO_TRACE("Calling CGGetEventTapList...");
    return CGGetEventTapListHookNext(maxNumberOfTaps, tapList, eventTapCount);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

class InputTapContext
{
    static const BYTE KEY_UP   = 0x00;
    static const BYTE KEY_DOWN = 0x80;
    inline bool IS_KEY_DOWN(BYTE key) const { return key & KEY_DOWN; }
    
    static const uint64_t HotKeysModifiersMask = kCGEventFlagMaskAlphaShift | kCGEventFlagMaskShift | kCGEventFlagMaskControl | kCGEventFlagMaskAlternate | kCGEventFlagMaskCommand;
    
    static const int LeftMouseDown = 0x01;
    static const int RightMouseDown = 0x02;
    
    static const CGKeyCode InvalidKeyCode = kVK_VolumeUp; // shouldn't be using that guy as a hotkey!
    
    // Prototype of actions associated with our set of hotkeys - returns true whether this is to behave as a known OIG hotkey.
    typedef bool (InputTapContext::*HotKeyFcn)(OriginIGO::IGOApplication* app, CGEventRef event);

    public:
        InputTapContext(InputCallback callback)
            : _tap(NULL), _callback(callback), /*_systemPreferences(new SystemPreferences()), */ _viewExtentsLock(OS_SPINLOCK_INIT)
            , _lastMoveLocation(INVALID_MOVE_LOCATION), _lastMoveTimestamp(0), _frameIdx(0), _mouseClickDownDetected(false), _mouseClickStartedOutside(0), _hasFocus(false)
            , _isFullscreen(false), _isKeyWindow(false), _igoEnabled(false), _isIGOAllowed(false), _hasNewHotKeys(false), _globalEnableIGO(true), _enableFiltering(true)
            , _restoreWindowMovableProperty(false), _disableTimeoutBypass(false)
        {
            memset(_keyData, 0, sizeof(_keyData));
            _viewExtents = CGRectMake(0, 0, 0, 0);
            
            CGEventRef event = CGEventCreate(NULL);
            _hotKeysModifiers = CGEventGetFlags(event) & HotKeysModifiersMask;
            CFRelease(event);

            _hotKeyActions[HotKeySet_OIG] = &InputTapContext::HotKey_OIGSwitchAction;
            _hotKeyActions[HotKeySet_PINNED_WIDGETS] = &InputTapContext::HotKey_PinnedWidgetsAction;
            _hotKeyActions[HotKeySet_APPLE_SWITCH_APP] = &InputTapContext::HotKey_AppleSwitchAppAction;
            
            SetInputHotKeys(VK_CONTROL, kVK_ANSI_1, VK_CONTROL, kVK_ANSI_2);
        }
        
        ~InputTapContext()
        {
            //delete _systemPreferences;
        }
    
        void SetInputTap(CFMachPortRef tap) { _tap = tap; }

        void SetIGOState(bool enabled) 
        {
            if (!_globalEnableIGO)
                return;
            
            _mouseClickDownDetected = false;
            _mouseClickStartedOutside = 0;
            
            if (_igoEnabled == enabled)
                return;
            
            if (!enabled)
            {
                memset(_keyData, 0, sizeof(_keyData));
                
                // If the reason for switching off OIG is losing focus (user clicking on a different app),
                // make the window unmovable to avoid snapping the window to the saved OIG cursor position
                // when the user returns to the game
                OriginIGO::IGOApplication* app = OriginIGO::IGOApplication::instance();
                if (app && !app->isActive() && app->wasActive())
                {
                    NSWindow* wnd = (NSWindow*)GetMainWindow();
                    if ([wnd isMovable])
                    {
#ifdef SHOW_TAP_KEYS
                        IGO_TRACE("Making game window unmovable to avoid window snapping to previous OIG cursor position");
#endif
                        _restoreWindowMovableProperty = true;
                        [wnd setMovable:NO];
                    }
                }

            }
            
            bool prevEnabled = _igoEnabled;
            _igoEnabled = enabled;
            
#ifdef USE_CLIENT_SIDE_FILTERING
            _lastMoveLocation = INVALID_MOVE_LOCATION;
#endif
            
            // Normally the flag should have been directly set to false from the tap
            // -> the event is coming from the client (for example the user has clicked the top bar X button)
            // -> make sure to resend the current modifiers state
            if (prevEnabled)
                RestoreGameModifierKeysState(HotKeySet_OIG, NULL, InvalidKeyCode);
        }
    
        void SetFocusState(bool hasFocus)
        { 
            if (_hasFocus == hasFocus)
                return;
            
            if (!hasFocus)
                memset(_keyData, 0, sizeof(_keyData));
            
            _hasFocus = hasFocus; 
        }
    
        void SetFullscreenState(bool isFullscreen)
        {
            if (_isFullscreen == isFullscreen)
                return;
            
            _isFullscreen = isFullscreen;
        }

        void SetInputWindowInfo(int posX, int posY, int width, int height, bool isKeyWindow)
        {
            _isKeyWindow = isKeyWindow;
            
            CGRect viewExtents = CGRectMake(posX, posY, width, height);
            OSSpinLockLock(&_viewExtentsLock);
            _viewExtents = viewExtents;
            
            _isIGOAllowed = (viewExtents.size.width >= OriginIGO::MIN_IGO_DISPLAY_WIDTH) && (viewExtents.size.height >= OriginIGO::MIN_IGO_DISPLAY_HEIGHT);
#ifdef SHOW_TAP_KEYS
            static bool wasIGOAllowed = true;
            if (wasIGOAllowed != _isIGOAllowed)
            {
                if (_isIGOAllowed)
                {
                    IGO_TRACEF("OIG Re-enabled because of view extents (%d, %d)", viewExtents.size.width, viewExtents.size.height);
                }
                
                else
                {
                    IGO_TRACEF("OIG Disable because of view extents (%d, %d)", viewExtents.size.width, viewExtents.size.height);
                }
                
                wasIGOAllowed = _isIGOAllowed;
            }
#endif
            
            OSSpinLockUnlock(&_viewExtentsLock);
            
            //if (isKeyWindow)
            //    _systemPreferences->Refresh();
        }

        CGRect GetViewExtents()
        {
            OSSpinLockLock(&_viewExtentsLock);
            CGRect viewExtents = _viewExtents;
            OSSpinLockUnlock(&_viewExtentsLock);
    
            return viewExtents;
        }

        bool IsKeyWindow() const { return _isKeyWindow; }
        void SetFrameIdx(int frameIdx) { _frameIdx = frameIdx; }
        int  GetFrameIdx() const { return _frameIdx; }
    
        void GetKeyboardState(BYTE keyData[KEYBOARD_DATA_SIZE]) { memcpy(keyData, _keyData, sizeof(BYTE) * KEYBOARD_DATA_SIZE); }
    
        //bool AreNextInputSourceHotKeysOn(BYTE keyData[KEYBOARD_DATA_SIZE]) { return AreHotKeysOn(SystemPreferences::HKID_NEXT_INPUT_SOURCE, keyData); }
        //bool ArePreviousInputSourceHotKeysOn(BYTE keyData[KEYBOARD_DATA_SIZE]) { return AreHotKeysOn(SystemPreferences::HKID_PREVIOUS_INPUT_SOURCE, keyData); }
        
        CGEventRef HandleEvent(CGEventTapProxy proxy, CGEventType type, CGEventRef event)
        {
            // Always reset tap if necessary and collect current keyboard state for hot keys
            bool hasFocus = _hasFocus;
            bool hotKeyEvent = false;
            bool mouseEvent = false;
            bool isKeyWindow = IsKeyWindow() && hasFocus;
            CGKeyCode keyCode = 0;

#ifdef SHOW_TAP_KEYS
            IGO_TRACEF("Handling event type=%d (subtype=%d, flags=0x%08)", type, CGEventGetIntegerValueField(event, kCGMouseEventSubtype), CGEventGetFlags(event));
#endif
            
            // Check if we have a new set of hot keys to handle...
            bool hasNewHotKeys = _hasNewHotKeys;
            if (hasNewHotKeys)
                ApplyHotKeys();
            
            // Should we restore the window movable property?
            if (_restoreWindowMovableProperty)
            {
                // Make sure to wait until we know we have focus back
                OriginIGO::IGOApplication* app = OriginIGO::IGOApplication::instance();
                if (app && app->isActive())
                {
#ifdef SHOW_TAP_KEYS
                    IGO_TRACE("Restoring game window movable property");
#endif
                    _restoreWindowMovableProperty = false;
                    NSWindow* wnd = (NSWindow*)GetMainWindow();
                    [wnd setMovable:YES];
                }
            }
            
            switch (type)
            {
                case kCGEventTapDisabledByUserInput:
                    {
#ifdef SHOW_TAP_KEYS
                        IGO_TRACE("Re-enabling tap after user disable");
#endif
                        CGEventTapEnable(_tap, true);
                        return NULL; //event;
                    }
                break;
                    
                case kCGEventTapDisabledByTimeout:
                    {
#ifdef SHOW_TAP_KEYS
                    IGO_TRACEF("Re-enabling tap after timeout - bypassing mode=%d", !_disableTimeoutBypass);
#endif
                    if (_disableTimeoutBypass)
                        return event;
                    
                        CGEventTapEnable(_tap, true);
                        return NULL; //event;
                    }
                break;
                    
#ifndef STARTED_IMPL_WITH_NO_TAP
                case kCGEventNull:
                    {
                        // Is this an internal NULL event we use to "immediately" run handlers on the main thread?
                        int64_t data = CGEventGetIntegerValueField(event, kCGEventSourceUserData);
                        if (data == MainThreadImmediateHandler_CURSOR)
                        {
#if defined(SHOW_TAP_KEYS) || defined(SHOW_CURSOR_CALLS)
                            IGO_TRACE("Got null event to trigger cursor handling...");
#endif
                            NotifyMainThreadCursorEvent();
                            return NULL;
                        }
                    }
                    break;
#endif
                    
                case kCGEventKeyUp:
                    {
                        keyCode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
                        if (isKeyWindow && _enableFiltering)
                        {
#ifdef SHOW_TAP_KEYS
                            IGO_TRACEF("Got key up event for %d", keyCode);
#endif

                            if (keyCode < MAX_VKEY_ENTRY)
                            {
                                _keyData[keyCode] = KEY_UP;
                                
                                // Are we waiting on the hot key to go up to re-enable hotkey on/off check?
                                for (uint8_t setIdx = 0; setIdx < HotKeySet_COUNT; ++setIdx)
                                {
                                    if (!_hotKeyIsReady[setIdx])
                                    {
#ifdef SHOW_TAP_KEYS
                                        IGO_TRACEF("We are still waiting on key up before enabling hotkey set=%d toggle again", setIdx);
#endif
                                    
                                        if (keyCode == _hotKeyReadyCheck[setIdx])
                                        {
#ifdef SHOW_TAP_KEYS
                                            IGO_TRACEF("Got pending key release to enable hotkey set=%d toggle again (%d)", setIdx, keyCode);
#endif
                                            _hotKeyIsReady[setIdx] = true;
                                            return NULL; // No need to go further + don't allow CheckHotKeys() call + hiding entire IGO hot key press from IGO + game
                                        }
                                        
                                        break; // only allow one hotkey set at once
                                    }
                                }
                            }
#if DEBUG
                            else
                            {
                                IGO_TRACEF("Got key code larger than MAX_VKEY_ENTRY (%d/%d)\n", keyCode, MAX_VKEY_ENTRY);
                            }
#endif                
                        }
                        
                        else
                        {
#ifdef SHOW_TAP_KEYS
                            IGO_TRACEF("Got key up event for %d BUT not key window (isKeyWindow=%d, hasFocus=%d)", keyCode, IsKeyWindow(), hasFocus);
#endif
                        }
                    }
                    break;
                    
                    
                case kCGEventKeyDown:
                    {
                        keyCode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
                        if (isKeyWindow && _enableFiltering)
                        {
#ifdef SHOW_TAP_KEYS
                            IGO_TRACEF("Got key down event for %d", keyCode);
#endif
                            if (keyCode < MAX_VKEY_ENTRY)
                            {
                                hotKeyEvent = true;
                                _keyData[keyCode] = KEY_DOWN;
                            }
#if DEBUG
                            else
                                IGO_TRACEF("Got key code larger than MAX_VKEY_ENTRY (%d/%d)\n", keyCode, MAX_VKEY_ENTRY);
#endif                
                        }
                        else
                        {
#ifdef SHOW_TAP_KEYS
                            IGO_TRACEF("Got key up event for %d BUT not key window (isKeyWindow=%d, hasFocus=%d)", keyCode, IsKeyWindow(), hasFocus);
#endif
                        }
                    }
                    break;
            
                case kCGEventFlagsChanged:
                    {
                        CGEventFlags flags = CGEventGetFlags(event);
                        if (isKeyWindow && _enableFiltering)
                        {
#ifdef SHOW_TAP_KEYS
                            IGO_TRACEF("Got flags changed event %llx", flags);
#endif
                            _hotKeysModifiers = flags & HotKeysModifiersMask;

                            _keyData[VK_CAPITAL] = (flags & kCGEventFlagMaskAlphaShift) ? KEY_DOWN : KEY_UP;
                            _keyData[VK_MENU]    = (flags & kCGEventFlagMaskAlternate)  ? KEY_DOWN : KEY_UP;
                            _keyData[VK_CONTROL] = (flags & kCGEventFlagMaskControl)    ? KEY_DOWN : KEY_UP;
                            _keyData[VK_SHIFT]   = (flags & kCGEventFlagMaskShift)      ? KEY_DOWN : KEY_UP;
                            _keyData[VK_LWIN]    = (flags & kCGEventFlagMaskCommand)    ? KEY_DOWN : KEY_UP;

                            // Use key code later if necessary to distinguish between left/right keys
                            // CGKeyCode keyCode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
                        }
                        
                        else
                        {
#ifdef SHOW_TAP_KEYS
                            IGO_TRACEF("Got flags changed event %llx BUT not key window (isKeyWindow=%d, hasFocus=%d)", flags, IsKeyWindow(), hasFocus);
#endif
                        }
                    }
                    break;
                
                case kCGEventMouseMoved:
                    {
                        // Try to detect whether the cursor is attached to the mouse (just necessary at startup/afterwards we actually keep track of the calls to CGAssociateMouseAndMouseCursorPosition ourselves).
                        // If it's the case, we should be getting the same absolute location from the events but a delta.
                        // -> we need 2 phases:
                        // phase 1: grab the absolute mouse location
                        // phase 2: check whether the absolute mouse location has changed + we are getting deltas (while moving, we still could be getting 0, 0 deltas
                        // Note that we can't generate the events ourselves: we need events from HID
                        static int initialMouseLocationPhase = 0;
                        if (initialMouseLocationPhase < 2)
                        {
                            static CGPoint initialMouseLocation;

                            // Access hooked method
                            extern CGPoint (*CGEventGetLocationHookNext)(CGEventRef event);
                            if (CGEventGetLocationHookNext)
                            {
                                int64_t userEvent = CGEventGetIntegerValueField(event, kCGEventSourceUnixProcessID);
                                if (!userEvent)
                                {
                                    if (initialMouseLocationPhase == 0)
                                    {
                                        initialMouseLocation = CGEventGetLocationHookNext(event);
                                        initialMouseLocationPhase = 1;
                                    }
                                    
                                    else
                                    {
                                        int64_t deltaX = CGEventGetIntegerValueField(event, kCGMouseEventDeltaX);
                                        int64_t deltaY = CGEventGetIntegerValueField(event, kCGMouseEventDeltaY);

                                        if (deltaX != 0 || deltaY != 0)
                                        {
                                            // In any case, we are done
                                            initialMouseLocationPhase = 2;
                                            
                                            CGPoint mouseLocation = CGEventGetLocationHookNext(event);
                                            if (mouseLocation.x == initialMouseLocation.x && mouseLocation.y == initialMouseLocation.y)
                                                NotifyCursorDetachedFromMouse();
                                        }
                                    }
                                }
                            }
                        }
                        
                        mouseEvent = true;
                    }
                    break;
                    
                default:
                    {
                        mouseEvent = true;
                    }
                    break;
            }
            
            if (!isKeyWindow)
            {
#if defined(SHOW_TAP_KEYS) && defined(SHOW_TAP_UNUSED_KEYS)
                IGO_TRACEF("Not key window -> letting event go (type=%d, keyCode=%d)", type, keyCode);
#endif
                return event;
            }

            // Have we disabled the filtering (while interacting with the main menu) but we want to keep the OIG state intact (we will only
            // switch off OIG if activating a new window from the menu. To remember: when the user starts interacting with the main menu, the rendering loop
            // is interrupted, ie you cannot disable OIG right then since it will happen only for the next rendering loop, which will run once you're done with
            // the main menu
            if (!_enableFiltering)
            {
#if defined(SHOW_TAP_KEYS) && defined(SHOW_TAP_UNUSED_KEYS)
                IGO_TRACEF("Filtering disabled (main menu) -> letting event go (type=%d, keyCode=%d)", type, keyCode);
#endif
                return event;
            }
            
            // Need to be key window and have a minimum size before we allow turn IGO on (this is to help with launchers and initial config windows that are
            // really part of the application)
            if (!IsIGOAllowed())
            {
#ifdef SHOW_TAP_KEYS
                IGO_TRACE("IGO is globally disabled or min requirements not met");
#endif
                return event;
            }
            
            bool filterEvent = _igoEnabled;
            if (!filterEvent)
            {
                // Are we about to apply hot keys? (turn on OIG/switch pinned widgets on/off)
                if (!hotKeyEvent || !CheckHotKeys(keyCode, event))
                {
#if defined(SHOW_TAP_KEYS) && defined(SHOW_TAP_UNUSED_KEYS)
                    IGO_TRACEF("Letting event go (type=%d, keyCode=%d) (not key| not hot key switch)", type, keyCode);
#endif
                    return event;
                }
                
                else
                {
#ifdef SHOW_TAP_KEYS
                    IGO_TRACEF("Keeping event (type=%d, keyCode=%d)(turning IGO on)", type, keyCode);
#endif
                    return NULL;
                }
            }

            // Are we about to turn IGO off?
            if (hotKeyEvent && CheckHotKeys(keyCode, event))
            {
                // don't forward keypress to client if possible not to confuse the keyboard state/get repeated key strokes
#ifdef SHOW_TAP_KEYS
                IGO_TRACE("Keeping event (turning IGO off)");
#endif
                return NULL;
            }
            
            // Entirely skip mouse events that started outside the view
            if (mouseEvent && _mouseClickStartedOutside)
            {
                if (type == kCGEventLeftMouseDown)
                    _mouseClickStartedOutside |= LeftMouseDown;
                
                if (type == kCGEventLeftMouseUp)
                    _mouseClickStartedOutside &= ~LeftMouseDown;
                
                if (type == kCGEventRightMouseDown)
                    _mouseClickStartedOutside |= RightMouseDown;
                
                if (type == kCGEventRightMouseUp)
                    _mouseClickStartedOutside &= ~RightMouseDown;
                
#ifdef SHOW_TAP_KEYS
                IGO_TRACEF("Letting the event (%d) go since the mouse was clicked outside the window", type);
#endif
                return event;
            }
            
            // Don't filter events if not directly in the view (ie allow the selection of the window bar) - EXCEPT FOR RELEASE THE
            // MOUSE BUTTONS (otherwise we may lose the release of an OIG window move -> the window will stay linked to the cursor)
            // Also keep dragging events in case the user is moving a window but the cursor moves outside the game.
            CGPoint location = CGEventGetUnflippedLocation(event);
            
            bool isFullscreen = _isFullscreen;
            if (!isFullscreen) // When in fullscreen, we always grab all the inputs (this is to prevent SimCity to scroll when placing the cursor in the corners)
            {
                if (mouseEvent && (!_mouseClickDownDetected || (type != kCGEventLeftMouseUp && type != kCGEventRightMouseUp && type != kCGEventLeftMouseDragged && type != kCGEventRightMouseDragged)))
                {
                    CGRect viewExtents = GetViewExtents();
                    if (!CGRectContainsPoint(viewExtents, location))
                    {
                        if (type == kCGEventLeftMouseDown)
                            _mouseClickStartedOutside |= LeftMouseDown;
                        
                        if (type == kCGEventRightMouseDown)
                            _mouseClickStartedOutside |= RightMouseDown;
                        
#ifdef SHOW_TAP_KEYS
                        IGO_TRACEF("Letting the event (%d) go since outside window (outside click=%d)", type, _mouseClickStartedOutside);
#endif
                        return event;
                    }
                }
            }
            
            
            switch (type)
            {
                case kCGEventLeftMouseDown:
                {
                    _mouseClickDownDetected = true;
                    
                    int64_t clickType = CGEventGetIntegerValueField(event, kCGMouseEventClickState);
                    IGOIPC::InputEventType msgType = (clickType == ClickState_DOUBLE) ? IGOIPC::EVENT_MOUSE_LEFT_DOUBLE_CLICK : IGOIPC::EVENT_MOUSE_LEFT_DOWN;
                    _callback(IGOIPC::InputEvent(msgType, location.x, location.y, 0, 0), NULL);
                        
#ifdef SHOW_TAP_KEYS
                    if (clickType == ClickState_DOUBLE)
                    {
                        IGO_TRACEF("Got double left mouse down (type = %lld)", clickType);
                    }
                    
                    else
                    {
                        IGO_TRACEF("Sending single left mouse down (type = %lld)", clickType);
                    }
#endif
                }
                    break;
                    
                case kCGEventLeftMouseUp:
                {
                    _mouseClickDownDetected = false;
                    
                    int64_t clickType = CGEventGetIntegerValueField(event, kCGMouseEventClickState);
                    _callback(IGOIPC::InputEvent(IGOIPC::EVENT_MOUSE_LEFT_UP, location.x, location.y, 0, 0), NULL);
                    
#ifdef SHOW_TAP_KEYS
                    if (clickType == ClickState_DOUBLE)
                    {
                        IGO_TRACEF("Got double left mouse up (type = %lld)", clickType);
                    }
                    
                    else
                    {
                        IGO_TRACEF("Sending single left mouse up (type = %lld)", clickType);
                    }
#else
                    (void) clickType;
#endif
                }
                    break;
                    
                case kCGEventRightMouseDown:
                {
                    _mouseClickDownDetected = true;
                    
                    int64_t clickType = CGEventGetIntegerValueField(event, kCGMouseEventClickState);
                    _callback(IGOIPC::InputEvent(IGOIPC::EVENT_MOUSE_RIGHT_DOWN, location.x, location.y, 0, 0), NULL);
                        
#ifdef SHOW_TAP_KEYS
                    IGO_TRACEF("Sending single right mouse down (type = %lld)", clickType);
#else
                    (void) clickType;
#endif
                }
                    break;
                    
                case kCGEventRightMouseUp:
                {
                    _mouseClickDownDetected = false;
                    
                    int64_t clickType = CGEventGetIntegerValueField(event, kCGMouseEventClickState);
                    // Always send the up event in case of a "quick" left + right clicks that forces the type to > 1
                    _callback(IGOIPC::InputEvent(IGOIPC::EVENT_MOUSE_RIGHT_UP, location.x, location.y, 0, 0), NULL);
                        
#ifdef SHOW_TAP_KEYS
                    IGO_TRACEF("Sending single right mouse up (type = %lld)", clickType);
#else
                    (void) clickType;
#endif
                }
                    break;
                    
                case kCGEventMouseMoved:
                case kCGEventLeftMouseDragged:
                case kCGEventRightMouseDragged:
                {
#ifdef FILTER_MOUSE_MOVE_EVENTS
                    uint64_t timestamp = UnsignedWideToUInt64(AbsoluteToNanoseconds(UpTime()));
                    
#ifndef USE_CLIENT_SIDE_FILTERING
                    uint64_t prevTimestamp = _lastMoveTimestamp;
                    if (UseMouseEvent(prevTimestamp, timestamp))
                    {
                        _lastMoveLocation = INVALID_MOVE_LOCATION;
                        _lastMoveTimestamp = timestamp;
                        _callback(IGOIPC::InputEvent(IGOIPC::EVENT_MOUSE_MOVE, location.x, location.y, 0, 0), NULL);
                    }
                    
                    else
                    {
                        uint64_t moveLoc = COMBINE_MOVE_LOCATION(location.x, location.y);
                        _lastMoveLocation = moveLoc;
                    }
#else
                    // Keep track of the last mouse location because of the filtering on the client side -> we always send at least one mouse
                    // move event per frame to make sure to send the last location known/force the final update when resizing/moving
                    uint64_t moveLoc = COMBINE_MOVE_LOCATION(location.x, location.y);
#ifdef SHOW_TAP_KEYS
                    IGO_TRACEF("Queueing new location %f, %f", location.x, location.y);
#endif
                    _lastMoveLocation = moveLoc;
                    _lastMoveTimestamp = timestamp;
                    
                    uint32_t lowTS = (uint32_t)timestamp;
                    uint32_t highTS = (uint32_t)(timestamp >> 32);
                    _callback(IGOIPC::InputEvent(IGOIPC::EVENT_MOUSE_MOVE, location.x, location.y, lowTS, highTS), NULL);
#endif
#else
                    uint64_t xOffset = CGEventGetIntegerValueField(event, kCGMouseEventDeltaX);
                    uint64_t yOffset = CGEventGetIntegerValueField(event, kCGMouseEventDeltaY);
                    if (xOffset != 0 || yOffset != 0)
                    {
                        _callback(IGOIPC::InputEvent(IGOIPC::EVENT_MOUSE_MOVE, location.x, location.y, 0, 0), NULL);
                    }
#endif
                }
                    break;
                    
                case kCGEventScrollWheel:
                {
                    uint32_t deltaV = (uint32_t)CGEventGetIntegerValueField(event, kCGScrollWheelEventPointDeltaAxis1);
                    uint32_t deltaH = (uint32_t)CGEventGetIntegerValueField(event, kCGScrollWheelEventPointDeltaAxis2);                    

                    _callback(IGOIPC::InputEvent(IGOIPC::EVENT_MOUSE_WHEEL, location.x, location.y, deltaV, deltaH), NULL);
#ifdef SHOW_TAP_KEYS
                    IGO_TRACEF("Got scroll wheel event (%f, %f, %d, %d)", location.x, location.y, deltaV, deltaH);
#endif
                }
                    break;
                    
                case kCGEventFlagsChanged:
                case kCGEventKeyDown:
                case kCGEventKeyUp:
                {
                    SendKeyboardEventToOrigin(event);
                }
                    break;
                    
                case kCGEventTabletPointer:
                case kCGEventTabletProximity:
                case kCGEventOtherMouseDown:
                case kCGEventOtherMouseUp:
                case kCGEventOtherMouseDragged:
                    // Known but unused events
#ifdef SHOW_TAP_KEYS
                    IGO_TRACEF("Got other unused event (type = %d)", type);
#endif
                    break;

                default: // In case of system-specific/internal events, pass them along?
#ifdef SHOW_TAP_KEYS
                    IGO_TRACEF("Letting unknown event go (type = %d)", type);
#endif
                    return event;
            }
            
            return NULL;

        }

#ifdef FILTER_MOUSE_MOVE_EVENTS
        // Filer mouse events not to overload client (especially for image renders when resizing!)
        inline bool UseMouseEvent(uint64_t prevTimestamp, uint64_t timestamp)
        {
            static const uint64_t MINIMUM_MOUSE_MOVE_EVENT_INTERVAL_IN_NS = 200000000;
            
            uint32_t diff = (uint32_t)(timestamp - prevTimestamp);
            return diff > MINIMUM_MOUSE_MOVE_EVENT_INTERVAL_IN_NS;
        }
    
        uint64_t GetLastMoveLocation()
        {
#ifndef USE_CLIENT_SIDE_FILTERING
            uint64_t location = INVALID_MOVE_LOCATION;
            uint64_t prevTimestamp = _lastMoveTimestamp;
            uint64_t myTime = UnsignedWideToUInt64(AbsoluteToNanoseconds(UpTime()));

            if (UseMouseEvent(prevTimestamp, myTime))
            {
                location = _lastMoveLocation;
                _lastMoveLocation = INVALID_MOVE_LOCATION;
            }
#else
            uint64_t location = _lastMoveLocation;
#endif
            return location;
        }
#endif
    
        bool IsIGOAllowed() const
        {
            bool allowed = _isIGOAllowed && _globalEnableIGO;
            return allowed;
        }
    
        bool IsIGOEnabled() const
        {
            bool enabled = _igoEnabled;
            return enabled;
        }
    
        void SetGlobalEnableIGO(bool enabled)
        {
            // Make sure we disable IGO if it was enable first
            if (!enabled)
                SetIGOState(false);
            
            _globalEnableIGO = enabled;
        }
    
        void SetEnableFiltering(bool enabled)
        {
            _enableFiltering = enabled;
        }
    
        // Notify that we have a new set of hot keys to handle - however wait until the next event to
        // reset the whole process
        void SetInputHotKeys(uint8_t key0, uint8_t key1, uint8_t pinKey0, uint8_t pinKey1)
        {
            // No need for spinlock - can't set the hotkeys multiple times in a row!
            _hasNewHotKeys = true;
            _newHotKeys[HotKeySet_OIG][0] = key0;
            _newHotKeys[HotKeySet_OIG][1] = key1;
            
            _newHotKeys[HotKeySet_PINNED_WIDGETS][0] = pinKey0;
            _newHotKeys[HotKeySet_PINNED_WIDGETS][1] = pinKey1;
            
            // Hard-coded/Apple-specific
            
            // Command+Tab to switch app (only need to handle when in fullscreen)
            _newHotKeys[HotKeySet_APPLE_SWITCH_APP][0] = VK_LWIN;
            _newHotKeys[HotKeySet_APPLE_SWITCH_APP][1] = VK_TAB;
        }
    
        void DisableTimeoutBypass(bool disable)
        {
#ifdef SHOW_TAP_KEYS
            IGO_TRACEF("Disabling timeout bypass=%d", disable);
#endif
            _disableTimeoutBypass = disable;
            if (!disable)
            {
                // Make sure our tap is enabled
                CGEventTapEnable(_tap, true);
            }
        }
    
    private:
        void SendKeyboardEventToOrigin(CGEventRef event)
        {
            CFDataRef data = CGEventCreateData(NULL, event);
            if (data)
            {
                CFIndex length = CFDataGetLength(data);
                if (length < (CFIndex)IGOIPCConnection::MAX_MSG_SIZE)
                {
                    CFDataGetBytes(data, CFRangeMake(0, length), _tmpBytes);
                    _callback(IGOIPC::InputEvent(IGOIPC::EVENT_KEYBOARD, length, 0, 0, 0), _tmpBytes);
                }
#if DEBUG
                else
                    Origin::Mac::LogError("Keyboard event larger than message max size (%ld/%ld)\n", length, IGOIPCConnection::MAX_MSG_SIZE);
#endif
            
                CFRelease(data);
            }
        }
    
        // Store current state of modifier keys
        void SaveModifierKeysState(uint8_t setIdx, CGEventRef event)
        {
            CGEventSourceRef source = NULL;
            CGEventSourceStateID sourceStateID = kCGEventSourceStateHIDSystemState;
        
            if (event)
            {
                source = CGEventCreateSourceFromEvent(event);
                sourceStateID = CGEventSourceGetSourceStateID(source);
                CFRelease(source);
            }
        
            _hotKeyModifiersState[setIdx] = 0;
            size_t count = sizeof(HotKeysModifiersKeySet) / sizeof(BYTE);
            for (size_t idx = 0; idx < count; ++idx)
            {
                // Is the key down?
                if (CGEventSourceKeyState(sourceStateID, HotKeysModifiersKeySet[idx]))
                    _hotKeyModifiersState[setIdx] |= 1 << idx;
            }
        
#ifdef SHOW_TAP_KEYS
            IGO_TRACEF("Storing modifier keys=0x%08x", _hotKeyModifiersState);
#endif
        }
    
        // Resend current state of modifier keys if updated while OIG on, followed by a hotkey if valid
        void RestoreGameModifierKeysState(uint8_t setIdx, CGEventRef event, CGKeyCode optionalHotKey)
        {
            CGEventSourceRef source = NULL;
            CGEventSourceStateID sourceStateID = kCGEventSourceStateHIDSystemState;
        
            if (event)
            {
                source = CGEventCreateSourceFromEvent(event);
                sourceStateID = CGEventSourceGetSourceStateID(source);
            }
        
            size_t count = sizeof(HotKeysModifiersKeySet) / sizeof(BYTE);
            for (size_t idx = 0; idx < count; ++idx)
            {
                bool isDown = CGEventSourceKeyState(sourceStateID, HotKeysModifiersKeySet[idx]);
                bool wasDown = (_hotKeyModifiersState[setIdx] & (1 << idx)) != 0;
            
                if (isDown != wasDown)
                {
#ifdef SHOW_TAP_KEYS
                    IGO_TRACEF("Restoring keyCode=%d, isDown=%d", HotKeysModifiersKeySet[idx], isDown ? 1 : 0);
#endif
                    CGEventRef modEvent = CGEventCreateKeyboardEvent(source, HotKeysModifiersKeySet[idx], isDown);
                    if (modEvent)
                    {
                        CGEventSetType(modEvent, isDown ? kCGEventKeyDown : kCGEventKeyUp); // this is because somebody stated there was a bug at some point with the Create function not correctly setting the type!?
                        CGEventPostHookNext(kCGAnnotatedSessionEventTap, modEvent);
                        CFRelease(modEvent);
                    }
                }
            }
            
            if (optionalHotKey)
            {
                CGEventRef modEvent = CGEventCreateKeyboardEvent(source, optionalHotKey, true);
                if (modEvent)
                {
                    CGEventSetType(modEvent, kCGEventKeyDown); // this is because somebody stated there was a bug at some point with the Create function not correctly setting the type!?
                    CGEventPostHookNext(kCGAnnotatedSessionEventTap, modEvent);
                    CFRelease(modEvent);
                }
            }
            
            if (source)
                CFRelease(source);
        }
    
        // Resend current state of modifier keys if updated while OIG off
        void RestoreOIGModifierKeysState(CGEventRef event)
        {
            CGEventFlags flags = CGEventGetFlags(event);
            CGEventFlags hotKeysModifiers = flags & HotKeysModifiersMask;
            if (hotKeysModifiers)
            {
#ifdef SHOW_TAP_KEYS
                IGO_TRACEF("Reseting OIG modifiers %lx", flags);
#endif

                CGEventSourceRef source = CGEventCreateSourceFromEvent(event);
                CGEventRef modEvent = CGEventCreate(source);
                CGEventSetType(modEvent, kCGEventFlagsChanged);
                
                hotKeysModifiers = flags & ~HotKeysModifiersMask;
                CGEventSetFlags(modEvent, hotKeysModifiers);
                
                SendKeyboardEventToOrigin(modEvent);
                
                CFRelease(modEvent);
                CFRelease(source);
            }
        }

        // Setup system to use the new hot keys
        void ApplyHotKeys()
        {
            _hasNewHotKeys = false;
            
            for (uint8_t setIdx = 0; setIdx < HotKeySet_COUNT; ++setIdx)
            {
                _hotKeysModifierMask[setIdx] = 0;
                
                // Setup keyboard map to use
                for (int idx = 0; idx < 4; ++idx)
                    _hotKeyKdbMask[setIdx][idx].bigEndianValue = 0;
                
                FlagKeyboardKeyCode(_hotKeyKdbMask[setIdx], VK_CAPITAL);
                FlagKeyboardKeyCode(_hotKeyKdbMask[setIdx], kVK_Function);

                // Two options: we have 1 modifier + key or a single key
                uint8_t key0 = _newHotKeys[setIdx][0];
                uint8_t key1 = _newHotKeys[setIdx][1];
                if (key0 != key1)
                {
                    switch (key0)
                    {
                        case VK_MENU:
                            _hotKeysModifierMask[setIdx] |= kCGEventFlagMaskAlternate;
                            FlagKeyboardKeyCode(_hotKeyKdbMask[setIdx], kVK_Option);
                            FlagKeyboardKeyCode(_hotKeyKdbMask[setIdx], kVK_RightOption);
                            break;
                            
                        case VK_CONTROL:
                            _hotKeysModifierMask[setIdx] |= kCGEventFlagMaskControl;
                            FlagKeyboardKeyCode(_hotKeyKdbMask[setIdx], kVK_Control);
                            FlagKeyboardKeyCode(_hotKeyKdbMask[setIdx], kVK_RightControl);
                            break;
                            
                        case VK_SHIFT:
                            _hotKeysModifierMask[setIdx] |= kCGEventFlagMaskShift;
                            FlagKeyboardKeyCode(_hotKeyKdbMask[setIdx], kVK_Shift);
                            FlagKeyboardKeyCode(_hotKeyKdbMask[setIdx], kVK_RightShift);
                            break;
                            
                        case VK_LWIN:
                            _hotKeysModifierMask[setIdx] |= kCGEventFlagMaskCommand;
                            FlagKeyboardKeyCode(_hotKeyKdbMask[setIdx], kVK_Command);
                            break;
                            
                        default:
                            Origin::Mac::LogError("Unknown hot key modifier %d", key0);
                            break;
                    }
                }
                
                _hotKey[setIdx] = key1;
                _hotKeyIsReady[setIdx] = true;
            }
        }
        
        // Helper to "activate" a particular keycode in a keyboard map
        void FlagKeyboardKeyCode(KeyMap& map, CGKeyCode keyCode)
        {
            uint8_t index = keyCode / 32;
            uint8_t shift = keyCode % 32;
            uint32_t value = 1 << shift;
            if (index < 4)
                map[index].bigEndianValue |= value;
        }
        
        // Check whether any other key than the hot keys (+ special keys) are currently pressed
        bool AreHotKeysOnlyKeysPressed(CGKeyCode keyCode, BigEndianUInt32* keyMap)
        {
            // Get current keyboard state
            KeyMap kbdMap;
            if (CGSGetKeysHookNext)
                CGSGetKeysHookNext(kbdMap);
            
            else
                GetKeys(kbdMap);
            
                
            // Update our mask with the current keycode we are considering
            KeyMap hotKeyMap;
            for (int idx = 0; idx < 4; ++idx)
                hotKeyMap[idx].bigEndianValue = keyMap[idx].bigEndianValue;
            
            FlagKeyboardKeyCode(hotKeyMap, keyCode);
            
#ifdef SHOW_TAP_KEYS
            IGO_TRACE("Check if other keys pressed:\nCurrent Map:");
            for (int idx = 0; idx < 4; ++idx)
            {
                IGO_TRACEF("%d. 0x%08x", idx + 1, kbdMap[idx].bigEndianValue);
            }
            
            IGO_TRACE("Hot keys Map:");
            for (int idx = 0; idx < 4; ++idx)
            {
                IGO_TRACEF("%d. 0x%08x", idx + 1, hotKeyMap[idx].bigEndianValue);
            }
#endif
            // Check if anything else is pressed...
            for (int idx = 0; idx < 4; ++idx)
            {
                if ((kbdMap[idx].bigEndianValue & ~hotKeyMap[idx].bigEndianValue) != 0)
                {
#ifdef SHOW_TAP_KEYS
                    IGO_TRACE("Other keys pressed");
#endif
                    return false;
                }
            }
            
            return true;
        }

        // Check whether we are trying to use a hotkey set (turn IGO on/off, pinned widgets, etc...)
        bool CheckHotKeys(CGKeyCode keyCode, CGEventRef event)
        {
            if (!_globalEnableIGO)
                return false;
            
            // Are we ready to check the hot keys?
            for (uint8_t setIdx = 0; setIdx < HotKeySet_COUNT; ++setIdx)
            {
                if (!_hotKeyIsReady[setIdx])
                    return false;
            }

            // Are we ready to check the hot keys?
            for (uint8_t setIdx = 0; setIdx < HotKeySet_COUNT; ++setIdx)
            {
                OriginIGO::IGOApplication* app = OriginIGO::IGOApplication::instance();
                
                // Don't care about caps lock
                CGEventFlags modifiers = _hotKeysModifiers & ~kCGEventFlagMaskAlphaShift;
                
                // Special case: pressing ESC (with no modifiers) to close OIG
                if (setIdx == 0 && modifiers == 0)
                {
                    if (keyCode == VK_ESCAPE && app->isActive())
                    {
                        // Make sure we don't have any other key press
                        if (AreHotKeysOnlyKeysPressed(VK_ESCAPE, _hotKeyKdbMask[setIdx]))
                        {
#ifdef SHOW_TAP_KEYS
                            IGO_TRACE("Detected closing OIG because of ESC key");
#endif
                            // Make sure to post down/up esc events to the IGO manager to close any open browser window (don't ask me why!)
                            SendKeyboardEventToOrigin(event);
                            CGEventSetType(event, kCGEventKeyUp);
                            SendKeyboardEventToOrigin(event);
                            
                            app->toggleIGO();
                            
                            // Send the current modifier state to the game -> make sure the tap knows OIG is off
                            // We also use that to detect when the turning off comes from the client (after the user clicks on the top bar X for example)
                            _igoEnabled = false;
                            RestoreGameModifierKeysState(HotKeySet_OIG, event, InvalidKeyCode);
                            
                            _hotKeyIsReady[setIdx] = false;
                            _hotKeyReadyCheck[setIdx] = VK_ESCAPE;
                            return true;
                        }
                    }
                }
                
                // Are we good with the modifiers? (Don't care about caplock key)
                if ((modifiers ^ _hotKeysModifierMask[setIdx]) == 0)
                {
                    if (keyCode == _hotKey[setIdx])
                    {
                        // Make sure we don't have any other key press
                        if (AreHotKeysOnlyKeysPressed(keyCode, _hotKeyKdbMask[setIdx]))
                        {
#ifdef SHOW_TAP_KEYS
                            IGO_TRACEF("Detected toggle hotkey set=%d", setIdx);
#endif
                            // Setup to wait for the combo to be released to re-enable hotkey lookup. Need to do this
                            // BEFORE calling the action method, in case the action disables that check (for example
                            // the Command+Tab forces an application switch -> we won't see when the key is released!)
                            _hotKeyIsReady[setIdx] = false;
                            _hotKeyReadyCheck[setIdx] = keyCode;

                            if (!(this->*_hotKeyActions[setIdx])(app, event))
                            {
                                _hotKeyIsReady[setIdx] = true;
                                return false;
                            }
                            
                            return true;
                        }
                    }
                }
            }
            
            return false;
        }

        // Called whenever the hot key(s) combo to toggle OIG is activated
        bool HotKey_OIGSwitchAction(OriginIGO::IGOApplication* app, CGEventRef event)
        {
#ifdef SHOW_TAP_KEYS
            IGO_TRACE("Running OIG Switch action");
#endif
            if (app->isActive())
            {
                // Restore hot key modifiers before leaving OIG, in case we were using a textbox (ie Chat window)
                RestoreOIGModifierKeysState(event);
                
                // Send the current modifier state to the game -> make sure the tap knows OIG is off
                // We also use that to detect when the turning off comes from the client (after the user clicks on the top bar X for example)
                _igoEnabled = false;
                RestoreGameModifierKeysState(HotKeySet_OIG, event, InvalidKeyCode);
            }
            
            else
            {
                // Memorize what the modifers were when OIG got turned on (this case is mostly in case the user changes the hot keys
                // from OIG)
                SaveModifierKeysState(HotKeySet_OIG, event);
            }
            
            app->toggleIGO();
            
            return true;
        }
        
        // Called whenever the hot key(s) combo to toggle pinned widgets is activated
        bool HotKey_PinnedWidgetsAction(OriginIGO::IGOApplication* app, CGEventRef event)
        {
#ifdef SHOW_TAP_KEYS
            IGO_TRACE("Running Pinned widgets action");
#endif
            app->togglePinnedWidgets();
            return true;
        }
    
        // Called whenever the user presses Command+Tab to switch app
        bool HotKey_AppleSwitchAppAction(OriginIGO::IGOApplication* app, CGEventRef event)
        {
#ifdef SHOW_TAP_KEYS
            IGO_TRACE("Running Apple switch app (cmd+tab) action");
#endif
            // NOTE: if we're here, we assume we have captured the screen, otherwise we would not be able to detect the combo (OS would bypass us)
            // No need to do anything when OIG is off
            if (app->isActive())
            {
#ifdef SHOW_TAP_KEYS
                IGO_TRACE("AWA Action: OIG is on");
#endif
                // Restore hot key modifiers before leaving OIG
                RestoreOIGModifierKeysState(event);
                
                // Send the current modifier state to the game -> make sure the tap knows OIG is off
                _igoEnabled = false;
                
                // Restore modifiers and reset hot key
                RestoreGameModifierKeysState(HotKeySet_APPLE_SWITCH_APP, event, _hotKey[HotKeySet_APPLE_SWITCH_APP]);
                
                app->toggleIGO();
                
                // Because this is going to force an application switch, we won't get the key release event -> don't wait
                // on it/disable hotkeys because of it!
                _hotKeyIsReady[HotKeySet_APPLE_SWITCH_APP] = true;
                
                return true;
            }
            
            return false;
        }
    
    /*
        bool AreHotKeysOn(SystemPreferences::HKID id, BYTE keyData[KEYBOARD_DATA_SIZE])
        {
            CGEventFlags modifiers = _hotKeysModifiers;
     
            CGKeyCode hkKeyCode;
            CGEventFlags hkModifiers;
            _systemPreferences->GetHotKeys(id, &hkKeyCode, &hkModifiers);
     
            if (hkModifiers == modifiers)
            {
                if (hkKeyCode >= 0 && hkKeyCode < KEYBOARD_DATA_SIZE)
                {
                    if (IS_KEY_DOWN(keyData[hkKeyCode]))
                    {
                        for (int idx = 0; idx < KEYBOARD_DATA_SIZE; ++idx)
                        {
                            if (IS_KEY_DOWN(keyData[idx]) && idx != hkKeyCode && idx != VK_CAPITAL && idx != VK_MENU && idx != VK_CONTROL && idx != VK_SHIFT && idx != VK_LWIN)
                                return false;
                        }
                        
                        return true;
                    }
                }
                
                else
                {
                    IGO_TRACEF("Hotkey key code (%d) outside keyboard state range (0 - %d)!!!", hkKeyCode, KEYBOARD_DATA_SIZE);
                }
            }
            
            return false;
        }
     */
    
        enum ClickState
        {
            ClickState_SINGLE = 1,
            ClickState_DOUBLE = 2,
            ClickState_TRIPE  = 3
        };


        BYTE _keyData[KEYBOARD_DATA_SIZE];
        UInt8 _tmpBytes[IGOIPCConnection::MAX_MSG_SIZE];

        CFMachPortRef _tap;
        InputCallback _callback;

        //SystemPreferences* _systemPreferences;
    
        CGRect _viewExtents;
        OSSpinLock _viewExtentsLock;

        volatile uint64_t _lastMoveLocation;
        volatile uint64_t _lastMoveTimestamp;
    
        KeyMap _hotKeyKdbMask[HotKeySet_COUNT];
    
        CGKeyCode _hotKey[HotKeySet_COUNT];
        CGKeyCode _hotKeyReadyCheck[HotKeySet_COUNT];
        CGEventFlags _hotKeysModifierMask[HotKeySet_COUNT];
        CGEventFlags _hotKeysModifiers;
        HotKeyFcn _hotKeyActions[HotKeySet_COUNT];
        uint32_t _hotKeyModifiersState[HotKeySet_COUNT];
    
        int _lastEscFrameIdx;
        uint8_t _newHotKeys[HotKeySet_COUNT][2];
    
        volatile int _frameIdx;
    
        bool _hotKeyIsReady[HotKeySet_COUNT];
        bool _mouseClickDownDetected;
        int  _mouseClickStartedOutside;
    
        volatile bool _hasFocus;
        volatile bool _isFullscreen;
        volatile bool _isKeyWindow;
        volatile bool _igoEnabled;
        volatile bool _isIGOAllowed;
        volatile bool _hasNewHotKeys;
        volatile bool _globalEnableIGO;
        volatile bool _enableFiltering;
        volatile bool _restoreWindowMovableProperty;
        volatile bool _disableTimeoutBypass;
};

static CGEventRef InputTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void* userData)
{
    InputTapContext* ctxt = reinterpret_cast<InputTapContext*>(userData);
    if (ctxt)
        return ctxt->HandleEvent(proxy, type, event);
    
    return event;
}
                                                                         
////////////////////////////////////////////////////////////////////////////////////////////////////////

static EventHandlerRef OIGCarbonEH = NULL;

// Fwd decls
void RegisterOIGCarbonEventHandler();
void UnregisterOIGCarbonEventHandler();

DEFINE_HOOK(OSStatus, RemoveEventHandlerHook, (EventHandlerRef inHandlerRef))
{
#ifdef SHOW_TAP_KEYS
    IGO_TRACEF("Calling RemoveEventHandler %p...", inHandlerRef);
#endif
    
    // Don't know why somebody would do this but...
    if (inHandlerRef == OIGCarbonEH)
        return noErr;
    
    return RemoveEventHandlerHookNext(inHandlerRef);
}

DEFINE_HOOK(OSStatus, InstallEventHandlerHook, (EventTargetRef inTarget, EventHandlerUPP inHandler, ItemCount inNumTypes, const EventTypeSpec* inList, void* inUserData, EventHandlerRef* outRef))
{
    OSStatus status = noErr;

#ifdef SHOW_TAP_KEYS
    IGO_TRACEF("Calling InstallEventHandler %s, %p (%d)...", inTarget, inHandler, inNumTypes);

    for (ItemCount idx = 0; idx < inNumTypes; ++idx)
    {
        IGO_TRACEF("%d. %d / %d", idx + 1, inList[idx].eventClass, inList[idx].eventKind);
    }
#endif
    
    bool updateOIGHandler = inTarget == GetEventDispatcherTarget();
    if (updateOIGHandler)
        UnregisterOIGCarbonEventHandler();
    
    // Register the game handler
    status = InstallEventHandlerHookNext(inTarget, inHandler, inNumTypes, inList, inUserData, outRef);
    
    EventHandlerRef handler = NULL;
    if (outRef)
        handler = *outRef;
    
#ifdef SHOW_TAP_KEYS
    IGO_TRACEF("Result = %d, handler=%p", status, handler);
#endif
    
    if (updateOIGHandler)
        RegisterOIGCarbonEventHandler();
    
    return status;
}

#if !defined(__x86_64__)

DEFINE_HOOK(OSStatus, InstallStandardEventHandlerHook, (EventTargetRef inTarget))
{
    OSStatus status = noErr;

#ifdef SHOW_TAP_KEYS
    IGO_TRACEF("Calling InstallStandardEventHandler %s...", inTarget);
#endif
    
    bool updateOIGHandler = inTarget == GetEventDispatcherTarget();
    if (updateOIGHandler)
        UnregisterOIGCarbonEventHandler();
    
    // Call base method
    status = InstallStandardEventHandlerHookNext(inTarget);
    
#ifdef SHOW_TAP_KEYS
    IGO_TRACEF("Result = %d", status);
#endif
    
    if (updateOIGHandler)
        RegisterOIGCarbonEventHandler();
    
    return status;

}

DEFINE_HOOK(OSStatus, RemoveStandardEventHandlerHook, (EventTargetRef inTarget))
{
#ifdef SHOW_TAP_KEYS
    IGO_TRACEF("Calling RemoveStandardEventHandler %s...", inTarget);
#endif
    return RemoveStandardEventHandlerHookNext(inTarget);
}

#endif

// Actual keyboard input handler for Carbon Apps - simply forward to our tap check code.
pascal OSStatus OIGCarbonKeyboardHandler(EventHandlerCallRef nextHandler, EventRef theEvent, void* userData)
{
#ifdef SHOW_TAP_KEYS
    UInt32 code = 0;
    GetEventParameter(theEvent, kEventParamKeyCode, typeUInt32, NULL, sizeof(code), NULL, &code);
    IGO_TRACEF("Carbon Keyboard Event %d, %d", GetEventKind(theEvent), code);
#endif
    
    CGEventRef cgEvent = CopyEventCGEvent(theEvent);
    CGEventRef cgResult = NULL;
    
    if (cgEvent)
    {
        CGEventType eventType = CGEventGetType(cgEvent);
        cgResult = _inputTapContext->HandleEvent(NULL, eventType, cgEvent);
        
        CFRelease(cgEvent);
    }
    
    return cgResult ? eventNotHandledErr : noErr;
}

// Methods to register/unregfister our OIG Carbon event handler
void RegisterOIGCarbonEventHandler()
{
#ifdef SHOW_TAP_KEYS
    IGO_TRACE("Register OIG Carbon keyboard event handler");
#endif
    
    EventHandlerUPP handlerUPP;
    handlerUPP = NewEventHandlerUPP(OIGCarbonKeyboardHandler);
    
#ifdef STARTED_IMPL_WITH_NO_TAP
    EventTypeSpec eventTypes[10];
#else
    EventTypeSpec eventTypes[4];
#endif
    
    eventTypes[0].eventClass = kEventClassKeyboard;
    eventTypes[0].eventKind = kEventRawKeyDown;
    
    eventTypes[1].eventClass = kEventClassKeyboard;
    eventTypes[1].eventKind = kEventRawKeyUp;
    
    eventTypes[2].eventClass = kEventClassKeyboard;
    eventTypes[2].eventKind = kEventRawKeyModifiersChanged;
    
    eventTypes[3].eventClass = kEventClassKeyboard;
    eventTypes[3].eventKind = kEventRawKeyRepeat;
    
#ifdef STARTED_IMPL_WITH_NO_TAP
    eventTypes[4].eventClass = kEventClassMouse;
    eventTypes[4].eventKind = kEventMouseDown;
    
    eventTypes[5].eventClass = kEventClassMouse;
    eventTypes[5].eventKind = kEventMouseUp;
    
    eventTypes[6].eventClass = kEventClassMouse;
    eventTypes[6].eventKind = kEventMouseMoved;
    
    eventTypes[7].eventClass = kEventClassMouse;
    eventTypes[7].eventKind = kEventMouseDragged;
    
    eventTypes[8].eventClass = kEventClassMouse;
    eventTypes[8].eventKind = kEventMouseWheelMoved;
    
    eventTypes[9].eventClass = kEventClassMouse;
    eventTypes[9].eventKind = kEventMouseScroll;
#endif
    
    int count = GetEventTypeCount(eventTypes);
    
    void* userData = NULL;
    OSStatus err = InstallEventHandlerHookNext(GetEventDispatcherTarget(), handlerUPP, count, eventTypes, userData, &OIGCarbonEH);
    if (err != noErr)
        Origin::Mac::LogError("Unable to install OIG Carbon EH (%d)", err);
    
#ifdef SHOW_TAP_KEYS
    else
    {
        IGO_TRACEF("OIG Carbon handler installed (%p)", OIGCarbonEH);
    }
#endif
}

void UnregisterOIGCarbonEventHandler()
{
#ifdef SHOW_TAP_KEYS
    IGO_TRACE("Unregister OIG Carbon keyboard event handler");
#endif
    
    // Remove our own handler first
    if (OIGCarbonEH)
    {
        OSStatus status = RemoveEventHandlerHookNext(OIGCarbonEH);
        if (status != noErr)
            Origin::Mac::LogError("Failed to properly remove our carbon EH for update - %d", status);
        
        OIGCarbonEH = NULL;
    }
}

@implementation NSApplication (OriginExtension)

static NSTimeInterval HotkeySkippedTimestamp = -1.0;

DEFINE_SWIZZLE_METHOD_HOOK(NSApplication, void, sendEvent, :(NSEvent*)event)
{
    // Did we just stop this event from going through from the 'nextEventMatchingMask' because it's the hotkey toggle?
    if ([event timestamp] == HotkeySkippedTimestamp)
    {
#ifdef SHOW_TAP_KEYS
        IGO_TRACEF("NSApp sendEvent skipping event (%lf)", HotkeySkippedTimestamp);
#endif
        return;
    }
    
    // Always for through our tap check code for keyboard inputs
    NSEventType type = [event type];
    bool keyEvent = type == NSKeyDown || type == NSKeyUp || type == NSFlagsChanged;
#ifdef STARTED_IMPL_WITH_NO_TAP
    bool mouseEvent = type == NSLeftMouseDown || type == NSLeftMouseUp || type == NSRightMouseDown || type == NSRightMouseUp
                    || type == NSMouseMoved || type == NSLeftMouseDragged || type == NSRightMouseDragged
                    || type == NSOtherMouseDown || type == NSOtherMouseUp || type == NSOtherMouseDragged;
#else
    bool mouseEvent = false;
#endif
    
    if (keyEvent || mouseEvent)
    {
#ifdef SHOW_TAP_KEYS
        if (keyEvent)
        IGO_TRACEF("NSApp sendEvent = %d / %d", type, [event keyCode]);
#endif
        CGEventRef cgEvent = [event CGEvent];
        CGEventRef cgResult = NULL;
        
        if (cgEvent)
        {
            CGEventType eventType = CGEventGetType(cgEvent);
            cgResult = _inputTapContext->HandleEvent(NULL, eventType, cgEvent);
        }
        
        if (!cgResult)
        {
            IGO_TRACE("Don't forward event");
            return;
        }
    }
    
    CALL_SWIZZLED_METHOD(self, sendEvent, :event);
}

DEFINE_SWIZZLE_METHOD_HOOK(NSApplication, NSEvent*, nextEventMatchingMask, :(NSUInteger)mask untilDate:(NSDate *)expiration inMode:(NSString *)mode dequeue:(BOOL)flag)
{
    NSEvent* event = CALL_SWIZZLED_METHOD(self, nextEventMatchingMask, :mask untilDate:expiration inMode:mode dequeue:flag);
    
    // Always for through our tap check code for keyboard inputs
    if (event)
    {
        NSEventType type = [event type];
        bool keyEvent = type == NSKeyDown || type == NSKeyUp || type == NSFlagsChanged;
#ifdef STARTED_IMPL_WITH_NO_TAP
        bool mouseEvent = type == NSLeftMouseDown || type == NSLeftMouseUp || type == NSRightMouseDown || type == NSRightMouseUp
                        || type == NSMouseMoved || type == NSLeftMouseDragged || type == NSRightMouseDragged
                        || type == NSOtherMouseDown || type == NSOtherMouseUp || type == NSOtherMouseDragged;
        
        bool moveEvent = type == NSMouseMoved || type == NSLeftMouseDragged || type == NSRightMouseDragged || type == NSOtherMouseDragged;
        float realMousePosX;
        float realMousePosY;
#else
        bool mouseEvent = false;
#endif
        
        if (keyEvent || mouseEvent)
        {
#ifdef SHOW_TAP_KEYS
            if (keyEvent)
            {
            IGO_TRACEF("NSApp nextEvent (%d) = %d / %d", flag, type, [event keyCode]);
            }
            
#ifdef STARTED_IMPL_WITH_NO_TAP
            else
            {
                if (moveEvent)
                {
                    NSPoint wndLocation = [event locationInWindow];
                    NSWindow* window = [event window];
                    NSPoint screenLocation = NSZeroPoint;
                    if (window)
                    {
                        NSRect rect = [window convertRectToScreen:NSMakeRect(wndLocation.x, wndLocation.y, 1, 1)];
                        screenLocation = rect.origin;
                    }
                    
                    GetRealMouseLocation(&realMousePosX, &realMousePosY);

                    IGO_TRACEF("NSApp nextEvent (%d/%d) - wnd=%p (%d/%p) - flags=0x%08x, inWindow=%f, %f", type, [event subtype], window, [event windowNumber], [event context], [event modifierFlags], wndLocation.x, wndLocation.y);
                    IGO_TRACEF("inScreen=%f, %f - screenQuery=%f, %f", screenLocation.x, screenLocation.y, realMousePosX, realMousePosY);
                }
            }
#endif
#endif
            CGEventRef cgEvent = [event CGEvent];
            CGEventRef cgResult = NULL;
            
            if (cgEvent)
            {
                CGEventType eventType = CGEventGetType(cgEvent);
#ifdef STARTED_IMPL_WITH_NO_TAP
                if (moveEvent)
                    CGEventSetLocation(cgEvent, CGPointMake(realMousePosX, realMousePosY));
#endif
                cgResult = _inputTapContext->HandleEvent(NULL, eventType, cgEvent);
            }
            
            if (!cgResult)
            {
                // Remember skipping this event for the other NSApplication swizzled methods
                HotkeySkippedTimestamp = [event timestamp];
                
#ifdef SHOW_TAP_KEYS
                IGO_TRACEF("Don't forward event (%lf)", [event timestamp]);
#endif
                return nil;
            }
        }
        
#ifdef STARTED_IMPL_WITH_NO_TAP
        if (type == NSApplicationDefined)
        {
            short subType = [event subtype];
            if (subType == MainThreadImmediateHandler_CURSOR)
            {
                NotifyMainThreadCursorEvent();
                
                // Remember skipping this event for the other NSApplication swizzled methods
                HotkeySkippedTimestamp = [event timestamp];
                return nil;
            }
    }
#endif
    }
    
    return event;
}

DEFINE_SWIZZLE_METHOD_HOOK(NSApplication, NSEvent*, currentEvent, )
{
    NSEvent* event = CALL_SWIZZLED_METHOD(self, currentEvent, );

    if (event)
    {
        // Did we just stop this event from going through from the 'nextEventMatchingMask' because it's the hotkey toggle?
        if ([event timestamp] == HotkeySkippedTimestamp)
        {
#ifdef SHOW_TAP_KEYS
            IGO_TRACEF("NSApp currentEvent skipping event (%lf)", HotkeySkippedTimestamp);
#endif
            return nil;
        }
    }
    
    return event;
}

@end

/////////////////////////////////////////////////////////////////////////////////////////////////

@interface SessionNotificationHandler : NSObject
{
    NSWindow*        _keyWindow;
    CFMachPortRef    _tap;
    InputTapContext* _context;
}

+(SessionNotificationHandler*) sharedHandler;

-(void)StartWithTap:(CFMachPortRef)tap AndContext:(InputTapContext*)context;
-(void)Stop;

-(void)SessionActivated:(NSNotification*)notification;
-(void)SessionDeactivated:(NSNotification*)notification;

@end

@implementation SessionNotificationHandler

static SessionNotificationHandler* _sessionSharedHandler;

+(void)initialize
{
    static BOOL initialized = NO;
    if (!initialized)
    {
        initialized = YES;
        _sessionSharedHandler = [[SessionNotificationHandler alloc] init];
    }
}

+(SessionNotificationHandler*)sharedHandler
{
    return _sessionSharedHandler;
}

-(void)StartWithTap:(CFMachPortRef)tap AndContext:(InputTapContext*)context
{
    if (tap)
    {
        _tap = tap;
        _context = context;
        
        [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(SessionActivated:) name:NSWorkspaceSessionDidBecomeActiveNotification object:nil];
        [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(SessionDeactivated:) name:NSWorkspaceSessionDidResignActiveNotification object:nil];
    }
}

-(void)Stop
{
    if (_tap)
    {
        [[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:self name:NSWorkspaceSessionDidBecomeActiveNotification object:nil];
        [[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:self name:NSWorkspaceSessionDidResignActiveNotification object:nil];
    }
}


-(void)SessionActivated:(NSNotification*)notification
{
    IGO_TRACEF("NSWorkspaceSessionDidBecomeActiveNotification - restoring key wnd=%p", _keyWindow);

    if (_keyWindow)
    {
        // Somehow on Yosemite, and in this particular case The Sims4, the application is "confused" when returning from the "Fast User Switch" session if it previously was the active
        // application / although it's back to being the active application, it/the game doesn't know about it!!!
        // Looks like this is coming from the tap - even having a tap that doesn't nothing seem to break the game. I've started implementing a version of IGO that doesn't use the tap
        // (see the STARTED_IMPL_WITH_NO_TAP macro at the top of the file), but that change is too risky right now. So instead I fake the app reactivation:
        
        // 1) first we make sure the system/the application itself knows it's not active
        [NSApp deactivate];
        
        // 2) then we create a fake mouse left click event on the title bar
        //  a) because this user case is only available when in window mode
        //  b) believe it or not, but simply calling -[NSApplication activateIgnoringOtherApps:], -[NSRunningApplication activateWithOptions:], ...

        // NOTE: I tried other methods, like using core AppleEvents, but to no avail
        
        // Compute the title bar bbox
        NSRect wndBBox = [_keyWindow frame];
        NSRect viewContentBBox = [[_keyWindow contentView] frame];
        NSRect wndContentBBox = [_keyWindow convertRectToScreen:viewContentBBox];
        NSRect titlebarBBox = NSMakeRect(wndBBox.origin.x, wndContentBBox.origin.y + wndContentBBox.size.height, wndBBox.size.width, wndBBox.size.height - wndContentBBox.size.height);
        
        // Intersect with screen where most of the window is on
        NSRect screenBBox = [[_keyWindow screen] frame];
        NSRect intersection = NSIntersectionRect(screenBBox, titlebarBBox);
        if (NSIsEmptyRect(intersection) == NO)
        {
            CGPoint mouseLeftClickPos = CGPointMake(intersection.origin.x + intersection.size.width * 0.5, (screenBBox.origin.y + screenBBox.size.height) - (intersection.origin.y + intersection.size.height * 0.5));
        
            IGO_TRACEF("Fake left mouse click at %f, %f", mouseLeftClickPos.x, mouseLeftClickPos.y);

            // Safe previous location
            float posX;
            float posY;
            GetRealMouseLocation(&posX, &posY);
            
            CreateMouseEvents(kCGEventLeftMouseDown, mouseLeftClickPos, kCGMouseButtonLeft);
            CreateMouseEvents(kCGEventLeftMouseUp, mouseLeftClickPos, kCGMouseButtonLeft);
            CreateMouseEvents(kCGEventMouseMoved, CGPointMake(posX, posY), kCGMouseButtonLeft);
        }
    }
    
    //_context->DisableTimeoutBypass(false);
    CGEventTapEnable(_tap, true);
}

-(void)SessionDeactivated:(NSNotification*)notification
{
    CGEventTapEnable(_tap, false);
//    _context->DisableTimeoutBypass(true);

    _keyWindow = nil;
    if ([NSApp isActive] == YES)
    {
        _keyWindow = [NSApp keyWindow];
        if (!_keyWindow)
            _keyWindow = (NSWindow*)GetMainWindow();
        
        IGO_TRACEF("NSWorkspaceSessionDidResignActiveNotification - store key wnd=%p", _keyWindow);
    }
    
    else
        IGO_TRACE("NSWorkspaceSessionDidResignActiveNotification - game isn't active");
}
@end

////////////////////////////////////////////////////////////////////////////////////////////////////////

bool SetupInputTap(InputCallback callback)
{
    bool success = false;
    
    pid_t pid = 0;
    ProcessSerialNumber psn;
    
    OSErr errCode = GetCurrentProcess(&psn);
    GetProcessPID(&psn, &pid);
    if (errCode == 0)
    {
        _inputTapContext = new InputTapContext(callback);

#ifndef STARTED_IMPL_WITH_NO_TAP
        // Because of Maverick, we don't really anymore on assistive devices for tapping keyboard events -> we directly hook relevant methods
        CGEventMask mask = kCGEventMaskForAllEvents;
        mask |= ~(CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp));
        
        CFMachPortRef inputTap = CGEventTapCreateForPSNHookNext(&psn, kCGHeadInsertEventTap, kCGEventTapOptionDefault, mask, InputTapCallback, _inputTapContext);
        if (inputTap)
        {
            _inputTapContext->SetInputTap(inputTap);
            
            _inputRunLoop = CFRunLoopGetMain();
            IGO_TRACEF("Create input tap=%p (threadId=%d) - mainLoop=%p, currentLoop=%p", inputTap, GetCurrentThreadId(), CFRunLoopGetMain(), CFRunLoopGetCurrent());

            _inputRunLoopSource = CFMachPortCreateRunLoopSource(NULL, inputTap, 0);
            CFRunLoopAddSource(_inputRunLoop, _inputRunLoopSource, kCFRunLoopCommonModes);
                
            _inputTap = inputTap;
#else
        {
#endif
            success = true;
            
#if DEBUG
            DumpInputs();
#endif
      
            // Only intercept keyboard events once we have a valid tap/context is properly initialized AND make sure to swizzle game implementation of NSApp
            Class nsApplication = [NSApp class];
            if (nsApplication)
            {
                NSError* myError = nil;
                
                bool notUsed = true;
                SWIZZLE_METHOD(notUsed, NSApplication, nsApplication, sendEvent, :, myError)
                SWIZZLE_METHOD(notUsed, NSApplication, nsApplication, nextEventMatchingMask, :untilDate:inMode:dequeue:, myError)
                SWIZZLE_METHOD(notUsed, NSApplication, nsApplication, currentEvent, , myError)
            }

#ifndef STARTED_IMPL_WITH_NO_TAP
            // Hook Carbon event handler to always keep our keyboard handler on top of the stack
            OriginIGO::HookAPI(NULL, "InstallEventHandler", (LPVOID)InstallEventHandlerHook, (LPVOID*)&InstallEventHandlerHookNext);
            OriginIGO::HookAPI(NULL, "RemoveEventHandler", (LPVOID)RemoveEventHandlerHook, (LPVOID*)&RemoveEventHandlerHookNext);
#if !defined(__x86_64__)
            OriginIGO::HookAPI(NULL, "InstallStandardEventHandler", (LPVOID)InstallStandardEventHandlerHook, (LPVOID*)&InstallStandardEventHandlerHookNext);
            OriginIGO::HookAPI(NULL, "RemoveStandardEventHandler", (LPVOID)RemoveStandardEventHandlerHook, (LPVOID*)&RemoveStandardEventHandlerHookNext);
#endif
            
            if (InstallEventHandlerHookNext)
                RegisterOIGCarbonEventHandler();
            
            // Install observers to detect when the user leaves session or goes back to it (fast user switch interface) - wanted to momentarily disable the tap
            // and avoid breaking the games/OIG afterwards (we lose the key window) - but didn't work -> gave up -> on returning from the login session, I reset the past key window
            // BIG hack, but it works/avoids changing too much code surrounding the tap
            // Limit the damage to Yosemite and later
            NSProcessInfo* processInfo = [NSProcessInfo processInfo];
            SEL selector = NSSelectorFromString(@"isOperatingSystemAtLeastVersion:");
            if ([processInfo respondsToSelector:selector] == YES)
            {
#if __MAC_OS_X_VERSION_MAX_ALLOWED <= 1090
                // From 10100 SDK (Yosemite):
                typedef struct {
                    NSInteger majorVersion;
                    NSInteger minorVersion;
                    NSInteger patchVersion;
                } NSOperatingSystemVersion;

#endif
                NSOperatingSystemVersion version = {10, 0, 0};

                NSInvocation* nsActivityCall = [NSInvocation invocationWithMethodSignature:[processInfo methodSignatureForSelector:selector]];
                [nsActivityCall setSelector:selector];
                [nsActivityCall setTarget:processInfo];
                [nsActivityCall setArgument:&version atIndex:2];
                [nsActivityCall invoke];
                
                BOOL usingAtMinYosemite = NO;
                [nsActivityCall getReturnValue:&usingAtMinYosemite];

                if (usingAtMinYosemite)
                {
                    IGO_TRACE("On Yosemite or later -> setting up session notification handler");
                    [[SessionNotificationHandler sharedHandler] StartWithTap:_inputTap AndContext:_inputTapContext];
                }
        }
#endif
        }
        
#ifndef STARTED_IMPL_WITH_NO_TAP
        else
        {
            Origin::Mac::LogError("Failed to setup input tap on process %d\n", pid);
            
            delete _inputTapContext;
            _inputTapContext = NULL;
        }
#endif
    }
    
    else
    {
#if DEBUG
        Origin::Mac::LogError("Failed to get process id to setup input tap (%d)\n", errCode);
#endif
    }
    
    return success;
}

void CleanupInputTap()
{
#if DEBUG
    Origin::Mac::LogError("CLEANING UP TAP!\n");
#endif
#ifndef STARTED_IMPL_WITH_NO_TAP
    if (_inputTap)
    {
        // Remove session observers
        [[SessionNotificationHandler sharedHandler] Stop];
        
        // Clean up our handlers that use the input tap first!
        if (InstallEventHandlerHookNext)
            UnregisterOIGCarbonEventHandler();

        OriginIGO::UnhookAPI(NULL, "InstallEventHandler", (LPVOID*)&InstallEventHandlerHookNext);
        OriginIGO::UnhookAPI(NULL, "RemoveEventHandler", (LPVOID*)&RemoveEventHandlerHookNext);
#if !defined(__x86_64__)
        OriginIGO::UnhookAPI(NULL, "InstallStandardEventHandler", (LPVOID*)&InstallStandardEventHandlerHookNext);
        OriginIGO::UnhookAPI(NULL, "RemoveStandardEventHandler", (LPVOID*)&RemoveStandardEventHandlerHookNext);
#endif

#else
    {
#endif
        Class nsApplication = [NSApp class];
        if (nsApplication)
        {
            NSError* myError = nil;
            UNSWIZZLE_METHOD(NSApplication, nsApplication, sendEvent, :, myError)
            UNSWIZZLE_METHOD(NSApplication, nsApplication, nextEventMatchingMask, :untilDate:inMode:dequeue:, myError)
            UNSWIZZLE_METHOD(NSApplication, nsApplication, currentEvent, , myError)
        }

#ifndef STARTED_IMPL_WITH_NO_TAP
        if (_inputRunLoopSource)
        {
        CFRunLoopRemoveSource(_inputRunLoop, _inputRunLoopSource, kCFRunLoopCommonModes);
            CFRelease(_inputRunLoopSource);
        }
    
        CFRelease(_inputTap);
#endif
        delete _inputTapContext;
    }
    
    _inputTap = NULL;
    _inputRunLoop = NULL;
    _inputTapContext = NULL;
    _inputRunLoopSource = NULL;
}

#ifdef FILTER_MOUSE_MOVE_EVENTS
uint64_t GetLastMoveLocation()
{
    if (_inputTapContext)
        return _inputTapContext->GetLastMoveLocation();
    
    return -1;
}
#endif

bool IsIGOAllowed()
{
    if (_inputTapContext)
        return _inputTapContext->IsIGOAllowed();
    
    return false;
}

void SetGlobalEnableIGO(bool enabled)
{
    if (_inputTapContext)
        _inputTapContext->SetGlobalEnableIGO(enabled);
}

void SetInputEnableFiltering(bool enabled)
{
    if (_inputTapContext)
        _inputTapContext->SetEnableFiltering(enabled);
}

void SetInputHotKeys(uint8_t key0, uint8_t key1, uint8_t pinKey0, uint8_t pinKey1)
{
    if (_inputTapContext)
        _inputTapContext->SetInputHotKeys(key0, key1, pinKey0, pinKey1);
}

void SetInputIGOState(bool enabled)
{
    if (_inputTapContext)
        _inputTapContext->SetIGOState(enabled);
}

void SetInputFocusState(bool hasFocus)
{
    if (_inputTapContext)
        _inputTapContext->SetFocusState(hasFocus);
}

void SetInputFullscreenState(bool isFullscreen)
{
    if (_inputTapContext)
        _inputTapContext->SetFullscreenState(isFullscreen);
}

void SetInputWindowInfo(int posX, int posY, int width, int height, bool isKeyWindow)
{
    if (_inputTapContext)
        _inputTapContext->SetInputWindowInfo(posX, posY, width, height, isKeyWindow);
}

void SetInputFrameIdx(int frameIdx)
{
    if (_inputTapContext)
        _inputTapContext->SetFrameIdx(frameIdx);
}

void GetKeyboardState(BYTE keyData[KEYBOARD_DATA_SIZE])
{
    if (_inputTapContext)
        _inputTapContext->GetKeyboardState(keyData);
}

// Create fake mouse button release event
void CreateMouseEvents(CGEventType type, CGPoint point, CGMouseButton button)
{
    CGEventRef event = CGEventCreateMouseEvent(NULL, type, point, button);
    CGEventSetType(event, type); // this is because somebody stated there was a bug at some point with the Create function not correctly setting the type!?
    CGEventPostHookNext(kCGHIDEventTap, event);
    CFRelease(event);
}

// Create empty event to kick-off the main thread
void CreateEmptyEvent(enum MainThreadImmHandler handler)
{
#ifdef STARTED_IMPL_WITH_NO_TAP
    NSEvent* nsEvent = [NSEvent otherEventWithType:NSApplicationDefined location:NSZeroPoint modifierFlags:NSApplicationDefinedMask timestamp:[[NSProcessInfo processInfo] systemUptime] windowNumber:[[NSApp keyWindow] windowNumber] context:[NSGraphicsContext currentContext] subtype:MainThreadImmediateHandler_CURSOR data1:0 data2:0];
    [NSApp postEvent:nsEvent atStart:NO];
#else
    CGEventRef event = CGEventCreate(NULL);
    CGEventSetIntegerValueField(event, kCGEventSourceUserData, handler);
    CGEventPostHookNext(kCGHIDEventTap, event);
    CFRelease(event);
#endif
}

/*
bool AreNextInputSourceHotKeysOn(BYTE keyData[KEYBOARD_DATA_SIZE])
{
    if (_inputTapContext)
        return _inputTapContext->AreNextInputSourceHotKeysOn(keyData);
    
    else
        return false;
}

bool ArePreviousInputSourceHotKeysOn(BYTE keyData[KEYBOARD_DATA_SIZE])
{
    if (_inputTapContext)
        return _inputTapContext->ArePreviousInputSourceHotKeysOn(keyData);    
    
    else
        return false;
}
*/

bool IsFrontProcess()
{
    ProcessSerialNumber frontProcess;
    if (!GetFrontProcess(&frontProcess))
    {
        ProcessSerialNumber currentProcess;
        if (!GetCurrentProcess(&currentProcess))
        {
            return (frontProcess.highLongOfPSN == currentProcess.highLongOfPSN)
                && (frontProcess.lowLongOfPSN == currentProcess.lowLongOfPSN);
        }
    }
    
    return false;
}

// We assume the older HID APIs are available but do not actually remove the event entries (which should be accessible through the Quartz Event Services
// (ie we don't need to worry about removing the event entries), it simply "peeks" at the entries -> make sure we don't allow peeking while OIG is on.
// Test game used: Tiger Woods 12
typedef void* IODataQueueEntryPtr;
typedef void* IODataQueueMemoryPtr;
DEFINE_HOOK(IODataQueueEntryPtr, IODataQueuePeekHook, (IODataQueueMemoryPtr dataQueue))
{
#ifdef SHOW_TAP_KEYS
    static bool showTrace = false;
    if (!showTrace)
    {
        showTrace = true;
        IGO_TRACE("Calling IODataQueuePeek...");
    }
#endif
    
    bool enabled = false;
    if (_inputTapContext)
        enabled = _inputTapContext->IsIGOEnabled();
    
    if (!enabled)
        return IODataQueuePeekHookNext(dataQueue);
    
    else
        return NULL;
}

// Test game used: LEGO Batman
DEFINE_HOOK(void, CGSGetKeysHook, (KeyMap theKeys))
{
#ifdef SHOW_TAP_KEYS
    static bool showTrace = false;
    if (!showTrace)
    {
        showTrace = true;
        IGO_TRACE("Calling GetKeys...");
    }
#endif
    
    bool enabled = false;
    if (_inputTapContext)
        enabled = _inputTapContext->IsIGOEnabled();
    
    if (!enabled)
        return CGSGetKeysHookNext(theKeys);
    
    theKeys[0].bigEndianValue = 0;
    theKeys[1].bigEndianValue = 0;
    theKeys[2].bigEndianValue = 0;
    theKeys[3].bigEndianValue = 0;
}

// Setup a few hooks to watch how games try using tapping - No need to do anything until
// we find a test case to counter though!
bool SetupInputHooks()
{
    bool success = true;
    
    success &= OriginIGO::HookAPI(NULL, "CGEventTapCreate", (LPVOID)CGEventTapCreateHook, (LPVOID*)&CGEventTapCreateHookNext);
    success &= OriginIGO::HookAPI(NULL, "CGEventTapCreateForPSN", (LPVOID)CGEventTapCreateForPSNHook, (LPVOID*)&CGEventTapCreateForPSNHookNext);
    success &= OriginIGO::HookAPI(NULL, "CGEventTapEnable", (LPVOID)CGEventTapEnableHook, (LPVOID*)&CGEventTapEnableHookNext);
    success &= OriginIGO::HookAPI(NULL, "CGEventTapIsEnabled", (LPVOID)CGEventTapIsEnabledHook, (LPVOID*)&CGEventTapIsEnabledHookNext);
    success &= OriginIGO::HookAPI(NULL, "CGEventTapPostEvent", (LPVOID)CGEventTapPostEventHook, (LPVOID*)&CGEventTapPostEventHookNext);
    success &= OriginIGO::HookAPI(NULL, "CGEventPost", (LPVOID)CGEventPostHook, (LPVOID*)&CGEventPostHookNext);
    success &= OriginIGO::HookAPI(NULL, "CGEventPostToPSN", (LPVOID)CGEventPostToPSNHook, (LPVOID*)&CGEventPostToPSNHookNext);
    success &= OriginIGO::HookAPI(NULL, "CGGetEventTapList", (LPVOID)CGGetEventTapListHook, (LPVOID*)&CGGetEventTapListHookNext);

    // This hook is used for Tiger Woods 12
    OriginIGO::HookAPI(NULL, "IODataQueuePeek", (LPVOID)IODataQueuePeekHook, (LPVOID*)&IODataQueuePeekHookNext);
        
    // And this one for LEGO Batman (couldn't hook GetKeys directly without changing the override asm, but GetKeys directly calls CGSGetKeys!)
    OriginIGO::HookAPI(NULL, "CGSGetKeys", (LPVOID)CGSGetKeysHook, (LPVOID*)&CGSGetKeysHookNext);
    
    return success;
}

// Enable/disable all the hooks (tap is handled separately)
void EnableInputHooks(bool enabled)
{
}

// Clean up our hooks
void CleanupInputHooks()
{
    OriginIGO::UnhookAPI(NULL, "CGEventTapCreate", (LPVOID*)&CGEventTapCreateHookNext);
    OriginIGO::UnhookAPI(NULL, "CGEventTapCreateForPSN", (LPVOID*)&CGEventTapCreateForPSNHookNext);
    OriginIGO::UnhookAPI(NULL, "CGEventTapEnable", (LPVOID*)&CGEventTapEnableHookNext);
    OriginIGO::UnhookAPI(NULL, "CGEventTapIsEnabled", (LPVOID*)&CGEventTapIsEnabledHookNext);
    OriginIGO::UnhookAPI(NULL, "CGEventTapPostEvent", (LPVOID*)&CGEventTapPostEventHookNext);
    OriginIGO::UnhookAPI(NULL, "CGEventPost", (LPVOID*)&CGEventPostHookNext);
    OriginIGO::UnhookAPI(NULL, "CGEventPostToPSN", (LPVOID*)&CGEventPostToPSNHookNext);
    OriginIGO::UnhookAPI(NULL, "CGGetEventTapList", (LPVOID*)&CGGetEventTapListHookNext);

    OriginIGO::UnhookAPI(NULL, "IODataQueuePeek", (LPVOID*)&IODataQueuePeekHookNext);
    OriginIGO::UnhookAPI(NULL, "CGSGetKeys", (LPVOID*)&CGSGetKeysHookNext);
}

void SetupWindowInputHooks(WindowRef wnd)
{
    
}

void CleanupWindowInputHooks(WindowRef wnd)
{
    
}

// Dump info related to inputs
void DumpInputs()
{
    Origin::Mac::LogError("Our input tap: %p/%p (%d)", _inputTap, _inputRunLoopSource, getpid());

    CGTableCount tapCnt = 0;
    CGError err = CGGetEventTapList(0, NULL, &tapCnt);
    if (err == kCGErrorSuccess && tapCnt > 0)
    {
        CGEventTapInformation* tapInfo = new CGEventTapInformation[tapCnt];
        
        CGTableCount retCount = 0;
        err = CGGetEventTapList(tapCnt, tapInfo, &retCount);
        if (err == kCGErrorSuccess)
        {
            Origin::Mac::LogMessage("Current Taps (%d):", retCount);

            for (CGTableCount idx = 0; idx < retCount; ++idx)
            {
                Origin::Mac::LogMessage("  - %d. id=0x%08x, loc=%d, passive=%d, events=0x%016llx, creator pid=%d, target pid=%d, enabled=%d"
                                        , idx + 1, tapInfo[idx].eventTapID, tapInfo[idx].tapPoint, tapInfo[idx].options, tapInfo[idx].eventsOfInterest, tapInfo[idx].tappingProcess, tapInfo[idx].processBeingTapped, tapInfo[idx].enabled ? 1 : 0);
            }
        }
        
        else
            Origin::Mac::LogMessage("Unable to access tap info (%d)", err);
        
        delete []tapInfo;
    }
    
    else
        Origin::Mac::LogMessage("Unable to access tap list (%d)", err);
    
    CFArrayRef modes = CFRunLoopCopyAllModes(CFRunLoopGetMain());
    if (modes)
    {
        CFIndex cnt = CFArrayGetCount(modes);
        
        Origin::Mac::LogMessage("Main runloop modes (%p, %d):", CFRunLoopGetMain(), cnt);
        for (CFIndex idx = 0; idx < cnt; ++idx)
        {
            const CFStringRef name = (CFStringRef)CFArrayGetValueAtIndex(modes, idx);
            const char* namePtr = CFStringGetCStringPtr(name, kCFStringEncodingMacRoman);
            if (namePtr)
                Origin::Mac::LogMessage("  - %d. %s", idx, namePtr);
        }
        
        CFRelease(modes);
    }
    
    else
        Origin::Mac::LogMessage("No runloop modes for main!!!");
    
    if (CFRunLoopGetCurrent() != CFRunLoopGetMain())
    {
        modes = CFRunLoopCopyAllModes(CFRunLoopGetCurrent());
        if (modes)
        {
            CFIndex cnt = CFArrayGetCount(modes);
            
            Origin::Mac::LogMessage("Current runloop modes (%p, %d):", CFRunLoopGetCurrent(), cnt);
            for (CFIndex idx = 0; idx < cnt; ++idx)
            {
                const CFStringRef name = (CFStringRef)CFArrayGetValueAtIndex(modes, idx);
                const char* namePtr = CFStringGetCStringPtr(name, kCFStringEncodingMacRoman);
                if (namePtr)
                    Origin::Mac::LogMessage("  - %d. %s", idx, namePtr);
            }
            
            CFRelease(modes);
        }    
    }
}

#endif // ORIGIN_MAC
