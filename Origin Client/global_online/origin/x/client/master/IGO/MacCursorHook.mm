//
//  MacCursorHook.m
//  InjectedCode
//
//  Created by Frederic Meraud on 7/16/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import "MacCursorHook.h"

#if defined(ORIGIN_MAC)

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>

#include "MacJRSwizzle.h"
#include "HookAPI.h"
#include "OGLHook.h"
#include "MacInputHook.h"
#include "MacRenderHook.h"

//#define USE_CONTEXT_LOCKING_ON_STATE_CHANGE   // Skip on the context locking for now as it locks up SimCity. Will need to find which game previously required this to see if it's
                                                // still applicable.

// THINGS TO REMEMBER:
// There are some "interesting" rules as to when [NSCursor set] actually applies. depending on which thread we are running on, whether the game is using tracking rectangles, and
// also which OS we are using: for example on 10.7, DA2 and SimCity won't update their cursors properly if the calls aren't used on the main thread. But on Mavericks (and I suspect
// Mountain Lion since that was my main dev environment before), you want to immediately run the set commands, otherwise you may see delays as to when the cursor actually changes.
// Note that we don't want to wait for the calls performed on the main thread in case of potential deadlocks.

#define MOVE_CURSOR_FILENAME            @"/System/Library/Frameworks/WebKit.framework/Frameworks/WebCore.framework/Resources/moveCursor.png"
#define NE_RESIZE_CURSOR_FILENAME       @"/System/Library/Frameworks/WebKit.framework/Frameworks/WebCore.framework/Resources/northEastResizeCursor.png"
#define NW_RESIZE_CURSOR_FILENANE       @"/System/Library/Frameworks/WebKit.framework/Frameworks/WebCore.framework/Resources/northWestResizeCursor.png"
#define SE_RESIZE_CURSOR_FILENAME       @"/System/Library/Frameworks/WebKit.framework/Frameworks/WebCore.framework/Resources/southEastResizeCursor.png"
#define SW_RESIZE_CURSOR_FILENAME       @"/System/Library/Frameworks/WebKit.framework/Frameworks/WebCore.framework/Resources/southWestResizeCursor.png"
#define NESW_RESIZE_CURSOR_FILENAME     @"/System/Library/Frameworks/WebKit.framework/Frameworks/WebCore.framework/Resources/northEastSouthWestResizeCursor.png"
#define NWSE_RESIZE_CURSOR_FILENAME     @"/System/Library/Frameworks/WebKit.framework/Frameworks/WebCore.framework/Resources/northWestSouthEastResizeCursor.png"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// We use our own "version" of the [NSObject performSelectorOnMainThread] to minimize delays to process the requests
enum MainThreadCommand
{
    MainThreadCommand_SET_CURSOR,
    MainThreadCommand_ENABLE_OIG,
    MainThreadCommand_DISABLE_OIG
};

@interface MTCommandQueueItem : NSObject
{
    NSObject* _data;
    enum MainThreadCommand _command;
}

-(id)initWithCommand:(enum MainThreadCommand)cmd andObject:(NSObject*)obj;

@property(assign) NSObject* data;
@property(assign) enum MainThreadCommand command;

@end

@implementation MTCommandQueueItem

@synthesize data = _data;
@synthesize command = _command;

-(id)initWithCommand:(enum MainThreadCommand)cmd andObject:(NSObject*)obj
{
    if ((self = [super init]))
    {
        self->_data = obj;
        self->_command = cmd;
    }
    
    return self;
}
@end

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

@interface NSCursor (HiddenMethods)

-(NSCursor*) forceSet;
-(NSCursor*) _reallySet;

+(NSCursor*) _moveCursor;
+(NSCursor*) _windowResizeNorthEastSouthWestCursor;
+(NSCursor*) _windowResizeNorthWestSouthEastCursor;

@end

enum CursorCmd
{
    CursorCmd_NOTHING,
    
    // Cocoa
    CursorCmd_SET,
    CursorCmd_HIDE,
    CursorCmd_UNHIDE,
    
    // Carbon
    CursorCmd_SET_CURSOR,
    CursorCmd_SET_THEME_CURSOR,
    CursorCmd_SET_ANIMATED_THEME_CURSOR,
    
    // QuickDraw
    CursorCmd_INIT_CURSOR,
    CursorCmd_HIDE_CURSOR,
    CursorCmd_SHOW_CURSOR,
};

@interface IGOCursorState : NSObject
{
    Origin::Mac::Semaphore _lock;
    Origin::Mac::Semaphore _mainThreadQueueLock;
    
    bool _hasFocus;
    bool _igoEnabled;
    bool _setupComplete;
    bool _igoPendingCGLLock;
    bool _visibilityChecked;
    bool _acquiredMouse;
    bool _acquiredGlobalMouse;
    bool _initialCursorPositionSet;
    bool _changeCursorOnMainFrame;
    
    int _showCnt;
    int _showOffset;
    int _qdCursorLevel;
    UInt32 _lastThemeAnimatedStep;
    NSCursor* _lastNSCursor;
    ThemeCursor _lastThemeCursor;
    CGPoint _lastCursorPosition;
    CGPoint _lastIGOCursorPosition;
    
    CGRect _wndRect;
    OSSpinLock _wndRectLock;
    
    bool _mouseAndCursorAssociationSet;
    bool _lastAssociateMouseAndCursor;
    enum CursorCmd _lastCursorCmd;
    
    bool _leftButtonDown;
    bool _rightButtonDown;
    
    NSWindow* _mainWindow;
    bool _mainWindowUsesCursorRects;
    
    NSMutableArray* _mainThreadCommandQueue;
}

+(IGOCursorState*) instance;
+(bool) extended;

-(NSCursor*) MoveCursor;
-(NSCursor*) NESWCursor;
-(NSCursor*) NWSECursor;

-(void)SetupComplete:(bool)isComplete;
-(bool)IsSetupComplete;
-(void)SetFocus:(bool)hasFocus;
-(bool)ChangeCursorOnMainThread;
-(void)EnableIGOOnMainThread:(NSNumber*)skipLocationRestore;
-(void)DisableIGOOnMainThread:(NSNumber*)skipLocationRestore;
-(bool)IsIGOEnabled;
-(void)SetIGOEnabled:(bool)enabled WithImmediateUpdate:(bool)immediateUpdate AndSkipLocationRestore:(bool)skipLocationRestore;
-(void)SetWindowInfo:(CGRect)rect;
-(CGRect)GetWindowInfo;
-(void)Overwrite:(NSCursor*)cursor;
-(void)NotifyCursorDetachedFromMouse;

-(void)PostMainThreadCursorEvent:(enum MainThreadCommand)command WithObject:(NSObject*)data;
-(void)NotifyMainThreadCursorEvent;

-(void)GetRealMouseLocation:(float*)x AndY:(float*)y;

-(void)Hide;
-(void)Unhide;
-(void)Set:(NSCursor*)cursor;
-(void)SetHiddenUntilMouseMoves:(BOOL)flag;
-(NSCursor*)CurrentCursor;

-(void)AddCursorRect:(NSRect)aRect Cursor:(NSCursor *)aCursor ToView:(NSView*)view;

-(NSPoint)MouseLocation;

-(OSStatus)SetThemeCursor:(ThemeCursor)inCursor;
-(OSStatus)SetThemeCursor:(ThemeCursor)inCursor WithAnimationStep:(UInt32)animationStep;

-(CGError)DisplayHideCursor:(CGDirectDisplayID)display;
-(CGError)DisplayShowCursor:(CGDirectDisplayID)display;
-(boolean_t)CursorIsVisible;
-(CGError)DisplayMoveCursor:(CGDirectDisplayID)display ToPoint:(CGPoint)point;
-(CGError)AssociateMouseAndMouseCursorPosition:(boolean_t)connected;
-(CGError)WarpMouseCursorPosition:(CGPoint)newCursorPosition;
-(CGError)GetLastMouseDeltaX:(int32_t*)deltaX AndDeltaY:(int32_t*)deltaY;

-(void)InitCursor;
-(void)HideCursor;
-(void)ShowCursor;
-(void)GetMouse:(Point*)mouseLoc;
-(void)GetGlobalMouse:(Point*)globalMouse;

-(CGPoint)CGEventGetLocation:(CGEventRef)event;
-(bool)CGEventSourceButtonState:(CGEventSourceStateID)sourceState ForButton:(CGMouseButton)button;

@end

@implementation NSCursor (OriginExtension)

DEFINE_SWIZZLE_CLASS_METHOD_HOOK(NSCursor, void, hide, )
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACE("NSCursor hide...");
#endif
    
    [[IGOCursorState instance] Hide];
}

DEFINE_SWIZZLE_CLASS_METHOD_HOOK(NSCursor, void, unhide, )
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACE("NSCursor unhide...");
#endif
    
    [[IGOCursorState instance] Unhide];
}

DEFINE_SWIZZLE_METHOD_HOOK(NSCursor, void, pop, )
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACE("NSCursor -pop...");
#endif
    
    CALL_SWIZZLED_METHOD(self, pop, );
}

DEFINE_SWIZZLE_CLASS_METHOD_HOOK(NSCursor, void, pop, )
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACE("NSCursor +pop...");
#endif

    CALL_SWIZZLED_CLASS_METHOD(self, pop, );
}

DEFINE_SWIZZLE_METHOD_HOOK(NSCursor, void, push, )
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACE("NSCursor push...");
#endif

    CALL_SWIZZLED_METHOD(self, push, );
}

DEFINE_SWIZZLE_METHOD_HOOK(NSCursor, void, set, )
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACEF("NSCursor set %p", self);    
#endif
    
    [[IGOCursorState instance] Set:self];
}

DEFINE_SWIZZLE_CLASS_METHOD_HOOK(NSCursor, void, setHiddenUntilMouseMoves, :(BOOL)flag)
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACEF("NSCursor setHiddenUntilMouseMoves %d", flag);
#endif
    
    [[IGOCursorState instance] SetHiddenUntilMouseMoves:flag];
}

DEFINE_SWIZZLE_CLASS_METHOD_HOOK(NSCursor, NSCursor*, currentCursor, )
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACE("NSCursor currentCursor...");
#endif
    
    return [[IGOCursorState instance] CurrentCursor];
}

@end

@implementation NSWindow (OriginExtension)

DEFINE_SWIZZLE_METHOD_HOOK(NSWindow, BOOL, areCursorRectsEnabled, )
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACE("NSWindow areCursorRectsEnabled...");
#endif
    
    return CALL_SWIZZLED_METHOD(self, areCursorRectsEnabled, );
}

DEFINE_SWIZZLE_METHOD_HOOK(NSWindow, void, enableCursorRects, )
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACE("NSWindow enableCursorRects...");
#endif
    
    CALL_SWIZZLED_METHOD(self, enableCursorRects, );
}

DEFINE_SWIZZLE_METHOD_HOOK(NSWindow, void, disableCursorRects, )
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACE("NSWindow disableCursorRects...");
#endif

    CALL_SWIZZLED_METHOD(self, disableCursorRects, );
}
@end

@implementation NSView (OriginExtension)

DEFINE_SWIZZLE_METHOD_HOOK(NSView, void, addCursorRect, :(NSRect)aRect cursor:(NSCursor *)aCursor)
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACEF("NSView (%p) addCursorRect [(%f, %f), (%f, %f)], cursor=%p...", self, aRect.origin.x, aRect.origin.y, aRect.size.width, aRect.size.height, aCursor);
#endif
    
    [[IGOCursorState instance] AddCursorRect:aRect Cursor:aCursor ToView:self];
}

/*
DEFINE_SWIZZLE_METHOD_HOOK(NSView, void, removeCursorRect, :(NSRect)aRect cursor:(NSCursor *)aCursor)
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACEF("NSView (%p) removeCursorRect [(%f, %f), (%f, %f)], cursor=%p...", self, aRect.origin.x, aRect.origin.y, aRect.size.width, aRect.size.height, aCursor);
#endif

    CALL_SWIZZLED_METHOD(self, removeCursorRect, :aRect cursor:aCursor);
}

DEFINE_SWIZZLE_METHOD_HOOK(NSView, void, discardCursorRects, )
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACEF("NSView (%p) discardCursorRects...", self);
#endif
    
    CALL_SWIZZLED_METHOD(self, discardCursorRects, );
}

DEFINE_SWIZZLE_METHOD_HOOK(NSView, void, resetCursorRects, )
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACEF("NSView (%p) resetCursorRects...", self);
#endif
    
    CALL_SWIZZLED_METHOD(self, resetCursorRects, );
}

DEFINE_SWIZZLE_METHOD_HOOK(NSView, NSTrackingRectTag, addTrackingRect, :(NSRect)aRect owner:(id)userObject userData:(void *)userData assumeInside:(BOOL)flag)
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACEF("NSView (%p) addTrackingRect [(%f, %f), (%f, %f)], owner=%p, data=%p, assumeInside=%d...", self, aRect.origin.x, aRect.origin.y, aRect.size.width, aRect.size.height, userObject, userData, flag);
#endif
    
    NSTrackingRectTag tag = 0;
    NSTrackingRectTag tag = CALL_SWIZZLED_METHOD(self, addTrackingRect, :aRect owner:userObject userData:userData assumeInside:flag);
    
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACEF("Created rect tag=%d", tag);
#endif
 
    return tag;
}

DEFINE_SWIZZLE_METHOD_HOOK(NSView, void, removeTrackingRect, :(NSTrackingRectTag)aTag)
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACEF("NSView (%p) removeTrackingRect %d...", self, aTag);
#endif
    
    CALL_SWIZZLED_METHOD(self, removeTrackingRect, :aTag);
}

DEFINE_SWIZZLE_METHOD_HOOK(NSView, void, addTrackingArea, :(NSTrackingArea*)trackingArea)
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACEF("NSView (%p) addTrackingArea %p...", self, trackingArea);
#endif
    
    CALL_SWIZZLED_METHOD(self, addTrackingArea, :trackingArea);
}

DEFINE_SWIZZLE_METHOD_HOOK(NSView, void, removeTrackingArea, :(NSTrackingArea*)trackingArea)
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACEF("NSView (%p) removeTrackingArea %p...", self, trackingArea);
#endif
    
    CALL_SWIZZLED_METHOD(self, removeTrackingArea, :trackingArea);
}

DEFINE_SWIZZLE_METHOD_HOOK(NSView, NSArray*, trackingAreas, )
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACEF("NSView (%p) trackingAreas...", self);
#endif
    
    NSArray* areas = CALL_SWIZZLED_METHOD(self, trackingAreas, );
    
#ifdef SHOW_CURSOR_CALLS
    NSUInteger count = [areas count];
    
    IGO_TRACEF("Got %d areas", count);
    for (NSUInteger idx = 0; idx < count; ++idx)
    {
        IGO_TRACEF("%02d. %p", idx + 1, [areas objectAtIndex:idx]);
    }
#endif
    
    return areas;
}

DEFINE_SWIZZLE_METHOD_HOOK(NSView, void, updateTrackingAreas, )
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACEF("NSView (%p) updateTrackingAreas...", self);
#endif
    
    CALL_SWIZZLED_METHOD(self, updateTrackingAreas, );
}
*/
@end

@implementation NSEvent (OriginExtension)

DEFINE_SWIZZLE_CLASS_METHOD_HOOK(NSEvent, NSPoint, mouseLocation, )
{
#ifdef SHOW_CURSOR_CALLS
    //IGO_TRACE("NSEvent mouseLocation...");
#endif
    
    return [[IGOCursorState instance] MouseLocation];
}
@end

DEFINE_HOOK(OSStatus, SetThemeCursorHook, (ThemeCursor inCursor))
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACEF("Calling SetThemeCursor (%ld)...", inCursor);
#endif
    
    return [[IGOCursorState instance] SetThemeCursor:inCursor];
}

DEFINE_HOOK(OSStatus, SetAnimatedThemeCursorHook, (ThemeCursor inCursor, UInt32 inAnimationStep))
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACEF("Calling SetAnimatedThemeCursor (%ld, %ld)...", inCursor, inAnimationStep);
#endif
    
    return [[IGOCursorState instance] SetThemeCursor:inCursor WithAnimationStep:inAnimationStep];
}

DEFINE_HOOK(void, SetCursorHook, (const void* /*Cursor* */ crsr))
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACEF("Calling SetCursor %p", crsr);
#endif
    
    SetCursorHookNext(crsr);
}

DEFINE_HOOK(void, SetCCursorHook, (void* /*CCrsrHandle - DONT KNOW*/ cCrsr))
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACEF("Calling SetCCursor %p", cCrsr);
#endif
    
    SetCCursorHookNext(cCrsr);
}

DEFINE_HOOK(void, HideCursorHook, ())
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACE("Calling HideCursor...");
#endif
    
    [[IGOCursorState instance] HideCursor];
}

DEFINE_HOOK(void, InitCursorHook, ())
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACE("Calling InitCursor...");
#endif
    
    [[IGOCursorState instance] InitCursor];
}

DEFINE_HOOK(void, ObscureCursorHook, ())
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACE("Calling ObscureCursor...");
#endif
    
    ObscureCursorHookNext();
}

DEFINE_HOOK(void, ShieldCursorHook, (const void* /*Rect* */ shieldRect, Point offsetPt))
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACE("Calling ShieldCursor...");
#endif
    
    ShieldCursorHookNext(shieldRect, offsetPt);
}

DEFINE_HOOK(void, ShowCursorHook, ())
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACE("Calling ShowCursor...");
#endif
    
    [[IGOCursorState instance] ShowCursor];
}

DEFINE_HOOK(void, SetQDGlobalsArrowHook, (const void* /*Cursor* */ arrow))
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACEF("Calling SetQDGlobalsArrow %p", arrow);
#endif
    
    SetQDGlobalsArrowHookNext(arrow);
}

DEFINE_HOOK(OSStatus, QDSetNamedPixMapCursorHook, (const char name[128]))
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACE("Calling QDSetNamedPixMapCursor...");
#endif
    
    return QDSetNamedPixMapCursorHookNext(name);
}

DEFINE_HOOK(OSStatus, QDRegisterNamedPixMapCursorHook, (PixMapHandle crsrData, PixMapHandle crsrMask, Point hotSpot, const char name[128]))
{
#ifdef SHOW_CURSOR_CALLS    
    IGO_TRACE("Calling QDRegisterNamedPixMapCursor...");
#endif
    
    return QDRegisterNamedPixMapCursorHookNext(crsrData, crsrMask, hotSpot, name);
}

DEFINE_HOOK(CGError, CGDisplayHideCursorHook, (CGDirectDisplayID display))
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACEF("Calling CGDisplayHideCursor %d", display);
#endif
    
    return [[IGOCursorState instance] DisplayHideCursor:display];
}

DEFINE_HOOK(CGError, CGDisplayShowCursorHook, (CGDirectDisplayID display))
{
#ifdef SHOW_CURSOR_CALLS // - FIX ME!!!! DOESN'T WORK IF THIS IS COMMENTED OUT!!!!!
    IGO_TRACEF("Calling CGDisplayShowCursor %d", display);
#endif

    return [[IGOCursorState instance] DisplayShowCursor:display];
}

DEFINE_HOOK(CGError, CGDisplayMoveCursorToPointHook, (CGDirectDisplayID display, CGPoint point))
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACEF("Calling CGDisplayMoveCursorToPoint %d (%f, %f)", display, point.x, point.y);
#endif
    
    return [[IGOCursorState instance] DisplayMoveCursor:display ToPoint:point];
}

DEFINE_HOOK(boolean_t, CGCursorIsVisibleHook, (void))
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACE("Calling CGCursorIsVisible...");
#endif
    
    return [[IGOCursorState instance] CursorIsVisible];
}

//DEFINE_HOOK(CGError, CGCursorIsDrawnInFramebufferHook, (void))
//{
//#ifdef SHOW_CURSOR_CALLS
//    IGO_TRACE("Calling CGCursorIsDrawnInFramebuffer...");
//#endif
//    return CGCursorIsDrawnInFramebufferHookNext();
//}

DEFINE_HOOK(CGError, CGAssociateMouseAndMouseCursorPositionHook, (boolean_t connected))
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACEF("Calling CGAssociateMouseAndMouseCursorPosition %d", connected);
#endif
    
    return [[IGOCursorState instance] AssociateMouseAndMouseCursorPosition:connected];
}

DEFINE_HOOK(CGError, CGWarpMouseCursorPositionHook, (CGPoint newCursorPosition))
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACEF("Calling CGWarpMouseCursorPosition (%f, %f)", newCursorPosition.x, newCursorPosition.y);
#endif
    
    return [[IGOCursorState instance] WarpMouseCursorPosition:newCursorPosition];
}

DEFINE_HOOK(CGError, CGGetLastMouseDeltaHook, (int32_t *deltaX, int32_t *deltaY))
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACE("Calling CGGetLastMouseDelta...");
#endif
    
    return [[IGOCursorState instance] GetLastMouseDeltaX:deltaX AndDeltaY:deltaY];
}

DEFINE_HOOK(void, GetMouseHook, (Point* mouseLoc))
{
#ifdef SHOW_CURSOR_CALLS
    //IGO_TRACE("Calling GetMouse...");
#endif
    
    [[IGOCursorState instance] GetMouse:mouseLoc];
}

DEFINE_HOOK(HIPoint*, HIGetMousePositionHook, (HICoordinateSpace inSpace, void* inObject, HIPoint* outPoint))
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACE("Calling HIGetMousePosition...");
#endif
    
    return HIGetMousePositionHookNext(inSpace, inObject, outPoint);
}

DEFINE_HOOK(void, GetGlobalMouseHook, (Point* globalMouse))
{
#ifdef SHOW_CURSOR_CALLS
    //IGO_TRACE("Calling GetGlobalMouse...");
#endif
    
    [[IGOCursorState instance] GetGlobalMouse:globalMouse];
}

DEFINE_HOOK(CGEventRef, CGEventCreateHook, (CGEventSourceRef source))
{
#ifdef SHOW_CURSOR_CALLS
    //IGO_TRACE("Calling CGEventCreate...");
#endif
    
    return CGEventCreateHookNext(source);
}

DEFINE_HOOK(CGPoint, CGEventGetLocationHook, (CGEventRef event))
{
#ifdef SHOW_CURSOR_CALLS
    //IGO_TRACE("Calling CGEventGetLocation...");
#endif
    
    return [[IGOCursorState instance] CGEventGetLocation:event];
}

DEFINE_HOOK(bool, CGEventSourceButtonStateHook, (CGEventSourceStateID sourceState, CGMouseButton button))
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACE("Calling CGEventSourceButtonState...");
#endif
    
    return [[IGOCursorState instance] CGEventSourceButtonState:sourceState ForButton:button];
}

@implementation IGOCursorState

static IGOCursorState* _instance;
static bool _hiddenMethodsAvailable;

// Make copy of supported cursors so that we can distinguish our cursors from the game's cursors when dealing with tracking rects/areas.
static NSCursor* _arrowCursorCopy;
static NSCursor* _crosshairCursorCopy;
static NSCursor* _pointingHandCursorCopy;
static NSCursor* _IBeamCursorCopy;
static NSCursor* _resizeUpDownCursorCopy;
static NSCursor* _resizeLeftRightCursorCopy;

static NSCursor* _moveCursorCopy;
static NSCursor* _windowResizeNorthEastSouthWestCursorCopy;
static NSCursor* _windowResizeNorthWestSouthEastCursorCopy;


+(void)initialize
{
    static BOOL initialized = NO;
    if (!initialized)
    {
        initialized = YES;
        _instance = [[IGOCursorState alloc] init];
        
        _instance->_lastCursorCmd = CursorCmd_NOTHING;
        _instance->_wndRectLock = OS_SPINLOCK_INIT;
        
        // Assume the cursor is attached to the mouse. We have to wait until we receive mouse move events from the hardware to detect whether this is the case.
        _instance->_lastAssociateMouseAndCursor = true;
    
        _instance->_mainThreadCommandQueue = [[NSMutableArray alloc] initWithCapacity:50];
        
        @autoreleasepool 
        {
            _hiddenMethodsAvailable  = [[NSCursor class] respondsToSelector:@selector(_moveCursor)] == YES;
            _hiddenMethodsAvailable &= [[NSCursor class] respondsToSelector:@selector(_windowResizeNorthEastSouthWestCursor)] == YES;
            _hiddenMethodsAvailable &= [[NSCursor class] respondsToSelector:@selector(_windowResizeNorthWestSouthEastCursor)] == YES;
            
            NSCursor* cursor = [[NSCursor alloc] init];
            [cursor autorelease];
            
            _hiddenMethodsAvailable &= [cursor respondsToSelector:@selector(forceSet)] == YES;
            _hiddenMethodsAvailable &= [cursor respondsToSelector:@selector(_reallySet)] == YES;
            
            _arrowCursorCopy = [[NSCursor alloc] initWithImage:[[NSCursor arrowCursor] image] hotSpot:[[NSCursor arrowCursor] hotSpot]];
            _crosshairCursorCopy = [[NSCursor alloc] initWithImage:[[NSCursor crosshairCursor] image] hotSpot:[[NSCursor crosshairCursor] hotSpot]];
            _pointingHandCursorCopy = [[NSCursor alloc] initWithImage:[[NSCursor pointingHandCursor] image] hotSpot:[[NSCursor pointingHandCursor] hotSpot]];
            _IBeamCursorCopy = [[NSCursor alloc] initWithImage:[[NSCursor IBeamCursor] image] hotSpot:[[NSCursor IBeamCursor] hotSpot]];
            _resizeUpDownCursorCopy = [[NSCursor alloc] initWithImage:[[NSCursor resizeUpDownCursor] image] hotSpot:[[NSCursor resizeUpDownCursor] hotSpot]];
            _resizeLeftRightCursorCopy = [[NSCursor alloc] initWithImage:[[NSCursor resizeLeftRightCursor] image] hotSpot:[[NSCursor resizeLeftRightCursor] hotSpot]];
            
            if (_hiddenMethodsAvailable)
            {
                _moveCursorCopy = [[NSCursor alloc] initWithImage:[[NSCursor _moveCursor] image] hotSpot:[[NSCursor _moveCursor] hotSpot]];
                _windowResizeNorthEastSouthWestCursorCopy = [[NSCursor alloc] initWithImage:[[NSCursor _windowResizeNorthEastSouthWestCursor] image] hotSpot:[[NSCursor _windowResizeNorthEastSouthWestCursor] hotSpot]];
                _windowResizeNorthWestSouthEastCursorCopy = [[NSCursor alloc] initWithImage:[[NSCursor _windowResizeNorthWestSouthEastCursor] image] hotSpot:[[NSCursor _windowResizeNorthWestSouthEastCursor] hotSpot]];
            }
        }
    }
}

+(IGOCursorState*)instance
{
    return _instance;
}

+(bool) extended
{
    return _hiddenMethodsAvailable;
}

-(NSCursor*) MoveCursor
{
    static NSCursor* moveCursor = NULL;
    if (!moveCursor)
    {
        @autoreleasepool
        {
            NSImage* image = [[NSImage alloc] initWithContentsOfFile:MOVE_CURSOR_FILENAME];
            if (image)
            {
                [image autorelease];
                moveCursor = [[NSCursor alloc] initWithImage:image hotSpot:NSMakePoint(1, 1)];
            }
            
            else
            {
                IGO_TRACE("Unable to locate image for move cursor");
                moveCursor = _arrowCursorCopy;
            }
        }
    }
    
    return moveCursor;
}

-(NSCursor*) NESWCursor
{
    static NSCursor* neswCursor = NULL;
    if (!neswCursor)
    {
        @autoreleasepool
        {
            NSImage* image = [[NSImage alloc] initWithContentsOfFile:NESW_RESIZE_CURSOR_FILENAME];
            if (image)
            {
                [image autorelease];
                neswCursor = [[NSCursor alloc] initWithImage:image hotSpot:NSMakePoint(1, 1)];
            }
            
            else
            {
                IGO_TRACE("Unable to locate image for NESW resize cursor");
                neswCursor = _arrowCursorCopy;
            }
        }
    }
    
    return neswCursor;
}
                      
-(NSCursor*) NWSECursor
{
    static NSCursor* nwseCursor = NULL;
    if (!nwseCursor)
    {
        @autoreleasepool
        {
            NSImage* image = [[NSImage alloc] initWithContentsOfFile:NWSE_RESIZE_CURSOR_FILENAME];
            if (image)
            {
                [image autorelease];
                nwseCursor = [[NSCursor alloc] initWithImage:image hotSpot:NSMakePoint(1, 1)];
            }
            
            else
            {
                IGO_TRACE("Unable to locate image for NWSE resize cursor");
                nwseCursor = _arrowCursorCopy;
            }
        }
    }
    
    return nwseCursor;
}

-(void)SetupComplete:(bool)isComplete
{
    Origin::Mac::AutoLock lock(&_lock);
    _setupComplete = isComplete;
}

-(bool)IsSetupComplete
{
    Origin::Mac::AutoLock lock(&_lock);
    return _setupComplete;
}

-(void)SetFocus:(bool)hasFocus
{
    Origin::Mac::AutoLock lock(&_lock);
    
    if (_setupComplete)
        _hasFocus = hasFocus;
}

-(bool)ChangeCursorOnMainThread
{
    static bool initialized = false;
    if (!initialized)
    {
        initialized = true;
        
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACE("Testing whether to run cursor changes on main thread...");
#endif
        
        NSCursor* currentCursor = CALL_SWIZZLED_CLASS_METHOD(NSCursor, currentCursor,  );
        CALL_SWIZZLED_METHOD(_arrowCursorCopy, set, );
        NSCursor* newCursor = CALL_SWIZZLED_CLASS_METHOD(NSCursor, currentCursor,  );
        
        if (newCursor != _arrowCursorCopy)
        {
#ifdef SHOW_CURSOR_CALLS
            IGO_TRACE("NSCursor set failed -> run on main thread");
#endif
            _changeCursorOnMainFrame = true;
        }
        
        else
        {
#ifdef SHOW_CURSOR_CALLS
            IGO_TRACE("NSCursor set successful while running on this thread");
#endif
            CALL_SWIZZLED_METHOD(currentCursor, set, );
        }
    }
    
    return _changeCursorOnMainFrame;
}

-(void)EnableIGOOnMainThread:(NSNumber*)skipLocationRestore
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACEF("Enabling IGO - lock CGL=%d", _igoPendingCGLLock);
#endif
    
    Origin::Mac::AutoLock lock(&_lock);

    bool skipLocationRestoreValue = [skipLocationRestore boolValue];
    [skipLocationRestore release];
    
    if (!_setupComplete)
    {
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACE("Disabling IGO done - setup not complete");
#endif
        return;
    }

    if (_igoEnabled == true)
    {
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACE("Disabling IGO done - IGO not enabled");
#endif
        return;
    }

    // Make sure we have a valid render context to work with and lock it!
#ifdef USE_CONTEXT_LOCKING_ON_STATE_CHANGE
    if (_igoPendingCGLLock && !OGLHook::LockGLContext())
    {
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACE("Disabling IGO done - unable to lock GL context");
#endif
        return;
    }
#endif
    
    _mainWindow = (NSWindow*)GetMainWindow();
    if (_mainWindow)
    {
        _mainWindowUsesCursorRects = true; // [_mainWindow areCursorRectsEnabled];
        if (_mainWindowUsesCursorRects)
        {
#ifdef SHOW_CURSOR_CALLS
            IGO_TRACE("Disabling window cursor rects");
#endif
            [_mainWindow discardCursorRects];// NOTE: calling disableCursorRects breaks the events/click behavior
        }
    }
    
    if (!_lastAssociateMouseAndCursor && CGAssociateMouseAndMouseCursorPositionHookNext)
    {
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACE("Reassociating mouse and mouse cursor");
#endif
        CGAssociateMouseAndMouseCursorPositionHookNext(true);
    }

    // Check if the user was pressing the left/right mouse buttons down!
    if (CGEventSourceButtonStateHookNext && CGEventSourceButtonStateHookNext && CGEventSourceButtonStateHookNext)
    {
        _leftButtonDown = CGEventSourceButtonStateHookNext(kCGEventSourceStateCombinedSessionState, kCGMouseButtonLeft);
        _rightButtonDown = CGEventSourceButtonStateHookNext(kCGEventSourceStateCombinedSessionState, kCGMouseButtonRight);
    }
    
    if (_lastCursorCmd == CursorCmd_NOTHING)
    {
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACE("No registered cursor cmd for restore - using currentSystemCursor instead");
#endif
        
        NSCursor* cursor = [NSCursor currentSystemCursor];
        if (cursor)
        {
#ifdef SHOW_CURSOR_CALLS
            IGO_TRACEF("Using system cursor %p as last cursor", cursor);
#endif
            [cursor retain];
            [_lastNSCursor release];
            
            _lastNSCursor = cursor;            
            _lastCursorCmd = CursorCmd_SET;
        }        
    }

    [self Overwrite:_arrowCursorCopy];
    
    _acquiredMouse = false;
    _acquiredGlobalMouse = false;
    
    CGEventRef event = CGEventCreateHookNext(NULL);
    if (CGEventGetLocationHookNext)
    {
        _lastCursorPosition = CGEventGetLocationHookNext(event);
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACEF("Storing last game know cursor position: %f, %f", _lastCursorPosition.x, _lastCursorPosition.y);
#endif
    }
    CFRelease(event);
    

    _showCnt = 0;
    _showOffset = 0;
    _visibilityChecked = false;
    if (CGCursorIsVisibleHookNext && CGDisplayShowCursorHookNext && CGDisplayHideCursorHookNext)
    {
        const int MaxShowCount = 10;
        while (!CGCursorIsVisibleHookNext() && _showOffset < MaxShowCount)
        {
            ++_showOffset;
            CGError error = CGDisplayShowCursorHookNext(kCGDirectMainDisplay);
            if (error)
            {
                IGO_TRACEF("Got error while trying to show cursor %d", error);
            }
            
            else
            {
                // Trying to force system to register we did a show cursor, otherwise CGCursorIsVisibleHookNext may return false when the cursor is actually visible!
                if (CGEventGetLocationHookNext && CGWarpMouseCursorPositionHookNext && !skipLocationRestoreValue)
                    CGWarpMouseCursorPositionHookNext(_lastCursorPosition);
            }
        }
      
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACEF("Had to show the cursor %d times", _showOffset);
#endif
        
        if (_showOffset == MaxShowCount)
        {
            IGO_TRACEF("Tried to show the cursor more than %d times!!!", MaxShowCount);
            
            while (_showOffset > 0)
            {
                --_showOffset;
                CGError error = CGDisplayHideCursorHookNext(kCGDirectMainDisplay);
                if (error)
                {
                    IGO_TRACEF("Got error while trying to hide cursor %d", error);
                }
            }         
        }
    }
    
    // Is this the first time we're using IGO? -> center the cursor position if this is the case, otherwise simple restore the previous known position
    CGPoint cursorPosition;
    if (!_initialCursorPositionSet)
    {
        _initialCursorPositionSet = true;
        cursorPosition = CGPointMake(_wndRect.origin.x + _wndRect.size.width * 0.5f, _wndRect.origin.y + _wndRect.size.height * 0.5f);
    }
    
    else
    {
        if (CGEventGetLocationHookNext)
            cursorPosition = _lastIGOCursorPosition;
    }
    
    if (CGEventGetLocationHookNext && CGWarpMouseCursorPositionHookNext && !skipLocationRestoreValue)
    {
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACEF("Set IGO cursor pos:%f, %f (wnd origin: %f, %f - size:%f, %f", cursorPosition.x, cursorPosition.y, _wndRect.origin.x, _wndRect.origin.y, _wndRect.size.width, _wndRect.size.height);
#endif
        CGWarpMouseCursorPositionHookNext(cursorPosition);
    }

    
    _igoEnabled = true;
    
    // Release context now
#ifdef USE_CONTEXT_LOCKING_ON_STATE_CHANGE
    if (_igoPendingCGLLock)
        OGLHook::UnlockGLContext();
#endif
    
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACE("Disabling IGO done - all is good!");
#endif
}

-(void)DisableIGOOnMainThread:(NSNumber*)skipLocationRestore
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACEF("Disabling IGO - lock CGL=%d", _igoPendingCGLLock);
#endif
    
    Origin::Mac::AutoLock lock(&_lock);
    
    bool skipLocationRestoreValue = [skipLocationRestore boolValue];
    [skipLocationRestore release];

    if (!_setupComplete)
    {
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACE("Disabling IGO done - setup not complete");
#endif
        return;
    }

    if (_igoEnabled == false)
    {
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACE("Disabling IGO done - IGO not enabled");
#endif
        return;
    }

    // Make sure we have a valid render context to work with and lock it!
#ifdef USE_CONTEXT_LOCKING_ON_STATE_CHANGE
    if (_igoPendingCGLLock && !OGLHook::LockGLContext())
    {
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACE("Disabling IGO done - unable to lock GL context");
#endif
        return;
    }
#endif

    _igoEnabled = false;
    
    // Store last know cursor position in IGO
    CGEventRef event = CGEventCreateHookNext(NULL);
    if (CGEventGetLocationHookNext) //  && !skipLocationRestoreValue)
    {
        _lastIGOCursorPosition = CGEventGetLocationHookNext(event);
#ifdef SHOW_CUSOR_CALLS
    IGO_TRACEF("Storing last IGO known cursor position: %f, %f", _lastIGOCursorPosition.x, _lastIGOCursorPosition.y);
#endif
    }
    CFRelease(event);

    // Restoring the rects before setting the previously recorded cursor is that otherwise it may overwrite... our overwrite.
    // This is visible when looking at Spore for example. Don't have much of a better solution yet.
    if (_mainWindow && _mainWindowUsesCursorRects)
    {
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACE("Enabling window cursor rects");
#endif
        
        [_mainWindow resetCursorRects];
    }
    
    switch (_lastCursorCmd)
    {
        case CursorCmd_SET:
            CALL_SWIZZLED_METHOD(_lastNSCursor, set, );
            break;
            
        case CursorCmd_SET_THEME_CURSOR:
            SetThemeCursorHookNext(_lastThemeCursor);
            break;
            
        case CursorCmd_SET_ANIMATED_THEME_CURSOR:
            SetAnimatedThemeCursorHookNext(_lastThemeCursor, _lastThemeAnimatedStep);
            break;
            
        case CursorCmd_INIT_CURSOR:
            InitCursorHookNext();
            break;
            
        default:
#ifdef SHOW_CURSOR_CALLS
            IGO_TRACE("Nothing set to restore cursor");
#endif
            break;
    }
    
    if (CGDisplayShowCursorHookNext)
    {
        int showCnt = _visibilityChecked ? _showCnt : _showOffset;
        
        if (showCnt > 0)
        {
#ifdef SHOW_CURSOR_CALLS
            IGO_TRACEF("Have to hide the cursor %d times (%d/%d)", showCnt, _showOffset, _showCnt);
#endif
        
            while (showCnt > 0)
            {
                --showCnt;
                CGDisplayHideCursorHookNext(kCGDirectMainDisplay);
            }
        }
        
        else
        {
            if (showCnt < 0)
                IGO_TRACEF("Have a negative hide count (%d = %d/%d)!!!", showCnt, _showOffset, _showCnt);
        }
    }
    
    if (!_lastAssociateMouseAndCursor && CGAssociateMouseAndMouseCursorPositionHookNext)
    {
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACEF("Set mouse/mouse cursor association: %d", _lastAssociateMouseAndCursor);
#endif
        CGAssociateMouseAndMouseCursorPositionHookNext(false);
    }
    
    if (CGWarpMouseCursorPositionHookNext)
    {
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACEF("Reset game mouse cursor position (%f, %f)", _lastCursorPosition.x, _lastCursorPosition.y);
#endif
    if (CGEventGetLocationHookNext && CGWarpMouseCursorPositionHookNext && !skipLocationRestoreValue)
        CGWarpMouseCursorPositionHookNext(_lastCursorPosition);
    }
    
    // Re-create press down events if necessary
    if (CGEventSourceButtonStateHookNext && CGEventSourceButtonStateHookNext && CGEventSourceButtonStateHookNext)
    {
        if (_leftButtonDown)
        {
            bool leftButtonDown = CGEventSourceButtonStateHookNext(kCGEventSourceStateCombinedSessionState, kCGMouseButtonLeft);
            if (_leftButtonDown != leftButtonDown)
                CreateMouseEvents(kCGEventLeftMouseUp, _lastCursorPosition, kCGMouseButtonLeft);
        }
        
        if (_rightButtonDown)
        {
            bool rightButtonDown = CGEventSourceButtonStateHookNext(kCGEventSourceStateCombinedSessionState, kCGMouseButtonRight);
            if (_rightButtonDown != rightButtonDown)
                CreateMouseEvents(kCGEventRightMouseUp, _lastCursorPosition, kCGMouseButtonRight);
        }
    }
    
    // Release context now
#ifdef USE_CONTEXT_LOCKING_ON_STATE_CHANGE
    if (_igoPendingCGLLock)
        OGLHook::UnlockGLContext();
#endif
    
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACE("Disabling IGO done - all is good!");
#endif
}

-(bool)IsIGOEnabled
{
    Origin::Mac::AutoLock lock(&_lock);
    return _setupComplete && _igoEnabled;
}

-(void)SetIGOEnabled:(bool)enabled WithImmediateUpdate:(bool)immediateUpdate AndSkipLocationRestore:(bool)skipLocationRestore
{
    Origin::Mac::AutoLock lock(&_lock);
    
    if (!_setupComplete)
        return;
    
    if (_igoEnabled == enabled)
        return;

    // This is necessary for at least SimCity/DA2 to work on 10.7
    immediateUpdate &= ![self ChangeCursorOnMainThread];
    
    if (immediateUpdate || pthread_main_np())
    {
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACEF("Setting IGO enabled state immediately (enabled=%d, immUpdate=%d, skipLocationRestore=%d, pthread=%d)", enabled, immediateUpdate, skipLocationRestore, GetCurrentThreadId());
#endif
        _igoPendingCGLLock = false;
        if (enabled)
            [self EnableIGOOnMainThread:[[NSNumber alloc] initWithBool:skipLocationRestore ? YES : NO]];
        
        else
            [self DisableIGOOnMainThread:[[NSNumber alloc] initWithBool:skipLocationRestore ? YES : NO]];
    }
    
    else
    {
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACEF("Posting setting IGO enabled state (enabled=%d, immUpdate=%d, skipLocationRestore=%d, pthread=%d)", enabled, immediateUpdate, skipLocationRestore, GetCurrentThreadId());
#endif
        _igoPendingCGLLock = true;
        
        NSNumber* data = [[NSNumber alloc] initWithBool:skipLocationRestore ? YES : NO];

        if (enabled)
        {
            // [self performSelectorOnMainThread:@selector(EnableIGOOnMainThread:) withObject:[[NSNumber alloc] initWithBool:skipLocationRestore ? YES : NO] waitUntilDone:NO];
            [[IGOCursorState instance] PostMainThreadCursorEvent:MainThreadCommand_ENABLE_OIG WithObject:data];
        }
        
        else
        {
            // [self performSelectorOnMainThread:@selector(DisableIGOOnMainThread:) withObject:[[NSNumber alloc] initWithBool:skipLocationRestore ? YES : NO] waitUntilDone:NO];
            [[IGOCursorState instance] PostMainThreadCursorEvent:MainThreadCommand_DISABLE_OIG WithObject:data];
        }
    }
}

-(void)SetWindowInfo:(CGRect)rect
{
    OSSpinLockLock(&_wndRectLock);
    _wndRect = rect;
    OSSpinLockUnlock(&_wndRectLock);
}

-(CGRect)GetWindowInfo
{
    CGRect rect;
    
    OSSpinLockLock(&_wndRectLock);
    rect = _wndRect;
    OSSpinLockUnlock(&_wndRectLock);
    
    return rect;
}

-(void)Overwrite:(NSCursor*)cursor
{
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACEF("Overwrite cursor with %p", cursor)
#endif
    CALL_SWIZZLED_METHOD(cursor, set, );
}

-(void)NotifyCursorDetachedFromMouse
{
    Origin::Mac::AutoLock lock(&_lock);
    
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACE("Got notification cursor detached from mouse");
#endif
    
    if (!_mouseAndCursorAssociationSet)
    {
        _lastAssociateMouseAndCursor = false;
        _mouseAndCursorAssociationSet = true;
        
        // If IGO is on at that point, re-attach the cursor to the mouse
        if (_setupComplete && _igoEnabled)
        {
            if (CGAssociateMouseAndMouseCursorPositionHookNext)
            {
#ifdef SHOW_CURSOR_CALLS
                IGO_TRACE("Reassociating mouse and mouse cursor");
#endif
                CGAssociateMouseAndMouseCursorPositionHookNext(true);
            }
        }
    }
}

// Queue a new command and post a NULL input event to run it (hopefully) on the main thread
-(void)PostMainThreadCursorEvent:(enum MainThreadCommand)command WithObject:(NSObject*)data
{
    Origin::Mac::AutoLock lock(&_mainThreadQueueLock);

    MTCommandQueueItem* item = [[MTCommandQueueItem alloc] initWithCommand:command andObject:data];
    [_mainThreadCommandQueue addObject:item];
    
    CreateEmptyEvent(MainThreadImmediateHandler_CURSOR);
}

// Grab an event from our queue and process it
-(void)NotifyMainThreadCursorEvent
{
    MTCommandQueueItem* item = NULL;
    {
        Origin::Mac::AutoLock lock(&_mainThreadQueueLock);
        if ([_mainThreadCommandQueue count] > 0)
        {
            item = [_mainThreadCommandQueue objectAtIndex:0];
            [_mainThreadCommandQueue removeObjectAtIndex:0];
        }
    }
    
    if (item)
    {
        switch (item.command)
        {
            case MainThreadCommand_SET_CURSOR:
#ifdef SHOW_CURSOR_CALLS
                IGO_TRACEF("Running SET_CURSOR command cursor=%p", item.data);
#endif
                [[IGOCursorState instance] Overwrite:(NSCursor*)item.data];
                break;
                
            case MainThreadCommand_ENABLE_OIG:
#ifdef SHOW_CURSOR_CALLS
                IGO_TRACE("Running ENABLE_OIG command");
#endif
                [self EnableIGOOnMainThread:(NSNumber*)item.data];
                break;
                
            case MainThreadCommand_DISABLE_OIG:
#ifdef SHOW_CURSOR_CALLS
                IGO_TRACE("Running DISABLE_OIG command");
#endif
                [self DisableIGOOnMainThread:(NSNumber*)item.data];
                break;
        }
        
        [item release];
    }
    
    else
    {
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACE("RECEIVED MAIN THREAD EVENT NOTIFY BUT WE HAVE AN EMPTY QUEUE!");
#endif
    }
}

-(void)GetRealMouseLocation:(float*)x AndY:(float*)y
{
    Origin::Mac::AutoLock lock(&_lock);
    if (_setupComplete)
    {
        CGPoint pos = {0.0f, 0.0f};
        CGEventRef event = NULL;
        
        if (CGEventCreateHookNext && CGEventGetLocationHookNext)
        {
            event = CGEventCreateHookNext(NULL);
            pos = CGEventGetLocationHookNext(event);
        }
        
        else
        {
            event = CGEventCreate(NULL);
            pos = CGEventGetLocation(event);
        }
        
        CFRelease(event);
        if (x)
            *x = pos.x;
        
        if (y)
            *y = pos.y;
    }
}

-(void)Hide
{
    Origin::Mac::AutoLock lock(&_lock);
    
    if (_setupComplete && _igoEnabled)
    {
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACE("Skipping during IGO [NSCursor Hide]");
#endif
        return;
    }

    CALL_SWIZZLED_CLASS_METHOD(NSCursor, hide, );
}

-(void)Unhide
{
    Origin::Mac::AutoLock lock(&_lock);
    
    if (_setupComplete && _igoEnabled)
    {
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACE("Skipping during IGO [NSCursor Unhide]");
#endif
        return;
    }

    CALL_SWIZZLED_CLASS_METHOD(NSCursor, unhide, );
}

-(void)Set:(NSCursor*)cursor
{
    Origin::Mac::AutoLock lock(&_lock);
    
    if (_setupComplete)
    {
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACE("In cursor set...");
#endif

        if (_hasFocus)
        {
#ifdef SHOW_CURSOR_CALLS
            IGO_TRACE("We have focus...");
#endif

            if (_lastNSCursor != cursor)
            {
#ifdef SHOW_CURSOR_CALLS
                IGO_TRACEF("Current and previous cursors are different prev=%p, curr=%p", _lastNSCursor, cursor);
#endif
                [cursor retain];
                [_lastNSCursor release];
                
                _lastNSCursor = cursor;
            }
            
            _lastCursorCmd = CursorCmd_SET;
            
            if (_igoEnabled)
            {
#ifdef SHOW_CURSOR_CALLS
                IGO_TRACE("IGO is not enabled - don't set cursor yet");
#endif

                return;
            }            
        }
        
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACEF("K -> do set new cursor %p", cursor);
#endif
    }
    
    CALL_SWIZZLED_METHOD(cursor, set, );
}

-(void)SetHiddenUntilMouseMoves:(BOOL)flag
{
    Origin::Mac::AutoLock lock(&_lock);
    
    if (_setupComplete && _hasFocus && _igoEnabled)
    {
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACE("Skipping during IGO SetHiddenUntilMouseMoves");
#endif
        return;
    }

    CALL_SWIZZLED_CLASS_METHOD(NSCursor, setHiddenUntilMouseMoves, :flag);
}

-(NSCursor*)CurrentCursor
{
    Origin::Mac::AutoLock lock(&_lock);
    
    if (_setupComplete && _hasFocus && _igoEnabled)
    {
        if (_lastCursorCmd == CursorCmd_SET)
        {
#ifdef SHOW_CURSOR_CALLS
            IGO_TRACEF("Returning last set NSCursor %p", _lastNSCursor);
#endif
            return _lastNSCursor;
        }
    }
    
    return CALL_SWIZZLED_CLASS_METHOD(NSCursor, currentCursor,  );
}

-(void)AddCursorRect:(NSRect)aRect Cursor:(NSCursor *)aCursor ToView:(NSView*)view
{
    Origin::Mac::AutoLock lock(&_lock);
    
    if (_setupComplete && _hasFocus && _igoEnabled)
    {
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACE("Skipping during IGO AddCursorRect");
#endif
        return;
    }
    
    CALL_SWIZZLED_METHOD(view, addCursorRect, :aRect cursor:aCursor);
}

-(NSPoint)MouseLocation
{
    Origin::Mac::AutoLock lock(&_lock);
    
    static NSPoint mouseLocation;
    
    if (_setupComplete && _igoEnabled)
    {
        if (!_acquiredGlobalMouse)
        {
            mouseLocation = CALL_SWIZZLED_CLASS_METHOD(NSEvent, mouseLocation, );
            _acquiredGlobalMouse = true;
        }
      
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACEF("Using IGO MouseLocation %f, %f", mouseLocation.x, mouseLocation.y);
#endif
        return mouseLocation;
    }
    
    return CALL_SWIZZLED_CLASS_METHOD(NSEvent, mouseLocation, );
}

-(OSStatus)SetThemeCursor:(ThemeCursor)inCursor
{
    Origin::Mac::AutoLock lock(&_lock);
    
    if (_setupComplete && _hasFocus)
    {
        _lastThemeCursor = inCursor;
        _lastCursorCmd = CursorCmd_SET_THEME_CURSOR;

        if (_igoEnabled)
        {
#ifdef SHOW_CURSOR_CALLS
            IGO_TRACE("Storing during IGO SetThemeCursor");
#endif
            return 0;
        }
    }
    
    return SetThemeCursorHookNext(inCursor);
}

-(OSStatus)SetThemeCursor:(ThemeCursor)inCursor WithAnimationStep:(UInt32)animationStep
{
    Origin::Mac::AutoLock lock(&_lock);
    
    if (_setupComplete && _hasFocus)
    {
        _lastThemeCursor = inCursor;
        _lastThemeAnimatedStep = animationStep;
        _lastCursorCmd = CursorCmd_SET_ANIMATED_THEME_CURSOR;

        if (_igoEnabled)
        {
#ifdef SHOW_CURSOR_CALLS
            IGO_TRACE("Storing during IGO SetThemeCursor");
#endif
            return 0;
        }
    }
    
    return SetAnimatedThemeCursorHookNext(inCursor, animationStep);    
}

-(CGError)DisplayHideCursor:(CGDirectDisplayID)display
{
    Origin::Mac::AutoLock lock(&_lock);
    
    if (_setupComplete && _igoEnabled)
    {
        ++_showCnt;
        
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACEF("Storing during IGO DisplayHideCursor (%d)", _showCnt);
#endif
        
        return kCGErrorSuccess;
    }
    
    return CGDisplayHideCursorHookNext(display);
}

-(CGError)DisplayShowCursor:(CGDirectDisplayID)display
{
    Origin::Mac::AutoLock lock(&_lock);
    
    if (_setupComplete && _igoEnabled)
    {
        --_showCnt;
        if (_showCnt < 0)
            _showCnt = 0;

#ifdef SHOW_CURSOR_CALLS
        IGO_TRACEF("Storing during IGO DisplayShowCursor (%d)", _showCnt);
#endif
        
        return kCGErrorSuccess;
    }
    
    return CGDisplayShowCursorHookNext(display);
}

-(boolean_t)CursorIsVisible
{
    Origin::Mac::AutoLock lock(&_lock);
    
    if (_setupComplete && _igoEnabled)
    {
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACEF("Using IGO CursorIsVisible=%d", _showCnt == 0);
#endif
        
        _visibilityChecked = true;
        return _showCnt == 0;
    }

    return CGCursorIsVisibleHookNext();
}

-(CGError)DisplayMoveCursor:(CGDirectDisplayID)display ToPoint:(CGPoint)point
{
    Origin::Mac::AutoLock lock(&_lock);
    
    if (_setupComplete && _hasFocus && _igoEnabled)
    {
        CGRect bounds = CGDisplayBounds(display);
        _lastCursorPosition.x = bounds.origin.x + point.x;
        _lastCursorPosition.y = bounds.origin.y + point.y;
      
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACEF("Storing during IGO DisplayMoveCursor display=%d, %f, %f", display, point.x, point.y);
#endif
        return kCGErrorSuccess;
    }
    
    return CGDisplayMoveCursorToPointHookNext(display, point);
}

-(CGError)AssociateMouseAndMouseCursorPosition:(boolean_t)connected
{
    Origin::Mac::AutoLock lock(&_lock);
    
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACEF("Storing during IGO AssociateMouseAndMouseCursorPosition connected=%d", connected);
#endif
    _lastAssociateMouseAndCursor = connected;
    _mouseAndCursorAssociationSet = true;

    if (_setupComplete && _hasFocus)
    {
        if (_igoEnabled)
            return kCGErrorSuccess;
    }
    
#ifdef SHOW_CURSOR_CALLS
    IGO_TRACEF("PASSING THROUGH AssociateMouseAndMouseCursorPosition connected=%d", connected);
#endif

    return CGAssociateMouseAndMouseCursorPositionHookNext(connected);
}

-(CGError)WarpMouseCursorPosition:(CGPoint)newCursorPosition
{
    Origin::Mac::AutoLock lock(&_lock);
    
    if (_setupComplete && _hasFocus)
    {
#ifdef SHOW_CURSOR_CALLS
        if (CGEventGetLocationHookNext)
        {
            CGEventRef event = CGEventCreateHookNext(NULL);
            CGPoint currentPos = CGEventGetLocationHookNext(event);
        CFRelease(event);
        
            IGO_TRACEF("Storing during IGO WarpMouseCursorPosition %f, %f (%f, %f)", newCursorPosition.x, newCursorPosition.y, currentPos.x, currentPos.y);
        }
#endif
        _lastCursorPosition = newCursorPosition;
        
        if (_igoEnabled)
            return kCGErrorSuccess;
    }
    
    return CGWarpMouseCursorPositionHookNext(newCursorPosition);
}

-(CGError)GetLastMouseDeltaX:(int32_t*)deltaX AndDeltaY:(int32_t*)deltaY
{
    Origin::Mac::AutoLock lock(&_lock);
    
    if (_setupComplete && _hasFocus && _igoEnabled)
    {
        if (deltaX)
            *deltaX = 0;
        
        if (deltaY)
            *deltaY = 0;
     
#ifdef SHOW_CURSOR_CALLS
        IGO_TRACE("Using IGO GetLastMouseDelta 0, 0");
#endif
        
        return kCGErrorSuccess;
    }
    
    return CGGetLastMouseDeltaHookNext(deltaX, deltaY);
}

-(void)InitCursor
{
    Origin::Mac::AutoLock lock(&_lock);
    
    if (_setupComplete && _hasFocus)
    {
        _lastCursorCmd = CursorCmd_INIT_CURSOR;
        
        if (_igoEnabled)
        {
#ifdef SHOW_CURSOR_CALLS
            IGO_TRACE("Skipping during IGO InitCursor");
#endif
            
            return;
        }
    }
    
    InitCursorHookNext();
}

-(void)HideCursor
{
    Origin::Mac::AutoLock lock(&_lock);
    
    if (_setupComplete && _hasFocus)
    {
        --_qdCursorLevel;
        _lastCursorCmd = CursorCmd_HIDE_CURSOR;
        if (_igoEnabled)
        {
#ifdef SHOW_CURSOR_CALLS
            IGO_TRACEF("Skipping during IGO HideCursor (%d)", _qdCursorLevel);
#endif
            return;
        }
    }
    
    HideCursorHookNext();
}

-(void)ShowCursor
{
    Origin::Mac::AutoLock lock(&_lock);
    
    if (_setupComplete && _hasFocus)
    {
        ++_qdCursorLevel;
        if (_qdCursorLevel > 0)
            _qdCursorLevel = 0;
        
        _lastCursorCmd = CursorCmd_SHOW_CURSOR;
        if (_igoEnabled)
        {
#ifdef SHOW_CURSOR_CALLS
            IGO_TRACEF("Skipping during IGO ShowCursor (%d)", _qdCursorLevel);
#endif
            return;
        }
    }
    
    ShowCursorHookNext();
}

-(void)GetMouse:(Point*)mouseLoc
{
    static Point mouseLocation;
    
#ifdef SHOW_CURSOR_CALLS
    static bool inIGO = false;
    static Point prevMouseLocation;
#endif

    Origin::Mac::AutoLock lock(&_lock);
    
    if (_setupComplete && _igoEnabled)
    {
        if (!_acquiredMouse)
        {
            if (mouseLoc)
            {
                GetMouseHookNext(mouseLoc);
                mouseLocation = *mouseLoc;
                _acquiredMouse = true;
            }
        }
        
        else
        {
            if (mouseLoc)
                *mouseLoc = mouseLocation;
        }
        
#ifdef SHOW_CURSOR_CALLS
        if (inIGO == false || prevMouseLocation.h != mouseLocation.h || prevMouseLocation.v != mouseLocation.v)
        {
            inIGO = true;
            prevMouseLocation = mouseLocation;
            IGO_TRACEF("Using IGO GetMouse %d, %d", mouseLocation.h, mouseLocation.v);
        }
#endif
        return;
    }
    
#ifdef SHOW_CURSOR_CALLS
    if (inIGO == true)
    {
        inIGO = false;
        IGO_TRACE("Using base GetMouse");
    }
#endif
    
    GetMouseHookNext(mouseLoc);
}

-(void)GetGlobalMouse:(Point *)globalMouse
{
    static Point mouseGLocation;
 
#ifdef SHOW_CURSOR_CALLS
    static bool inIGO = false;
    static Point prevMouseGLocation;
#endif
    
    Origin::Mac::AutoLock lock(&_lock);
    
    if (_setupComplete && _igoEnabled)
    {
        if (!_acquiredGlobalMouse)
        {
            if (globalMouse)
            {
                GetGlobalMouseHookNext(globalMouse);
                mouseGLocation = *globalMouse;
                _acquiredGlobalMouse = true;
            }
        }
        
        else
        {
            if (globalMouse)
                *globalMouse = mouseGLocation;
        }
        
#ifdef SHOW_CURSOR_CALLS
        if (inIGO == false || prevMouseGLocation.h != mouseGLocation.h || prevMouseGLocation.v != mouseGLocation.v)
        {
            inIGO = true;
            prevMouseGLocation = mouseGLocation;
            IGO_TRACEF("Using IGO GetGlobalMouse %d, %d", mouseGLocation.h, mouseGLocation.v);
        }
#endif
        return;
    }
    
#ifdef SHOW_CURSOR_CALLS
    if (inIGO == true)
    {
        inIGO = false;
        IGO_TRACE("Using base GetGlobalMouse");
    }
#endif
    GetGlobalMouseHookNext(globalMouse);
}

-(CGPoint)CGEventGetLocation:(CGEventRef)event
{
    Origin::Mac::AutoLock lock(&_lock);
    
#ifdef SHOW_CURSOR_CALLS
    static bool inIGO = false;
    static CGPoint prevLastCursorPosition;
#endif
    
    if (_setupComplete && _hasFocus && _igoEnabled)
    {
#ifdef SHOW_CURSOR_CALLS
        if (inIGO == false || prevLastCursorPosition.x != _lastCursorPosition.x || prevLastCursorPosition.y != _lastCursorPosition.y)
        {
            inIGO = true;
            prevLastCursorPosition = _lastCursorPosition;
            IGO_TRACEF("Using IGO CGEventGetLocation %f, %f", _lastCursorPosition.x, _lastCursorPosition.y);
        }
#endif
        return _lastCursorPosition;
    }

#ifdef SHOW_CURSOR_CALLS
    if (inIGO == true)
    {
        inIGO = false;
        IGO_TRACE("Using base CGEventGetLocation");
    }
#endif
    return CGEventGetLocationHookNext(event);
}

-(bool)CGEventSourceButtonState:(CGEventSourceStateID)sourceState ForButton:(CGMouseButton)button
{
    Origin::Mac::AutoLock lock(&_lock);
    
    if (_setupComplete && _hasFocus && _igoEnabled)
    {
        if (button == kCGMouseButtonLeft)
            return _leftButtonDown;
        
        if (button == kCGMouseButtonRight)
            return _rightButtonDown;
        
        return false;
    }
    
    return CGEventSourceButtonStateHookNext(sourceState, button);
}

@end

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool SetupCursorHooks()
{
    @autoreleasepool
    {
        // Create dummy window to setup machinery that allow the use of NSCursor (for Carbon only apps)
        if (NSApp == nil)
        {
            OriginIGO::IGOLogInfo("Initializing Cocoa in Carbon app");
            
            NSApplicationLoad();
            [[[NSWindow alloc] init] release];
        }
        
        bool success = true;
        NSError* myError = nil;
        
        Class nsCursor = NSClassFromString(@"NSCursor");
        if (nsCursor)
        {
            OriginIGO::IGOLogInfo("Do NSCursor...");
            
            SWIZZLE_METHOD(success, NSCursor, nsCursor, set, ,  myError)
            SWIZZLE_METHOD(success, NSCursor, nsCursor, push, , myError)
            SWIZZLE_METHOD(success, NSCursor, nsCursor, pop, , myError)
            SWIZZLE_CLASS_METHOD(success, NSCursor, nsCursor, hide, , myError)
            SWIZZLE_CLASS_METHOD(success, NSCursor, nsCursor, unhide, , myError)
            SWIZZLE_CLASS_METHOD(success, NSCursor, nsCursor, pop, , myError)
            SWIZZLE_CLASS_METHOD(success, NSCursor, nsCursor, setHiddenUntilMouseMoves, :, myError)
            SWIZZLE_CLASS_METHOD(success, NSCursor, nsCursor, currentCursor, , myError)
        }
        
        Class nsWindow = NSClassFromString(@"NSWindow");
        if (nsWindow)
        {
            OriginIGO::IGOLogInfo("Do NSWindow cursor rectangles...");
            
            SWIZZLE_METHOD(success, NSWindow, nsWindow, areCursorRectsEnabled, , myError)
            SWIZZLE_METHOD(success, NSWindow, nsWindow, enableCursorRects, , myError)
            SWIZZLE_METHOD(success, NSWindow, nsWindow, disableCursorRects, , myError)
        }
        
        Class nsView = NSClassFromString(@"NSView");
        if (nsView)
        {
            OriginIGO::IGOLogInfo("Do NSView cursor rectangles...");
            
            SWIZZLE_METHOD(success, NSView, nsView, addCursorRect, :cursor:, myError)
            /*
            SWIZZLE_METHOD(success, NSView, nsView, removeCursorRect, :cursor:, myError)
            SWIZZLE_METHOD(success, NSView, nsView, discardCursorRects, , myError)
            SWIZZLE_METHOD(success, NSView, nsView, resetCursorRects, , myError);
            
            SWIZZLE_METHOD(success, NSView, nsView, addTrackingRect, :owner:userData:assumeInside:, myError)
            SWIZZLE_METHOD(success, NSView, nsView, removeTrackingRect, :, myError)
            SWIZZLE_METHOD(success, NSView, nsView, addTrackingArea, :, myError)
            SWIZZLE_METHOD(success, NSView, nsView, removeTrackingArea, :, myError)
            SWIZZLE_METHOD(success, NSView, nsView, trackingAreas, , myError)
            SWIZZLE_METHOD(success, NSView, nsView, updateTrackingAreas, , myError)
             */
        }
        
        Class nsEvent = NSClassFromString(@"NSEvent");
        if (nsEvent)
        {
            OriginIGO::IGOLogInfo("Do NSEvent ...");
            
            SWIZZLE_CLASS_METHOD(success, NSEvent, nsEvent, mouseLocation, , myError)
        }
        
        OriginIGO::HookAPI(NULL, "SetThemeCursor", (LPVOID)SetThemeCursorHook, (LPVOID*)&SetThemeCursorHookNext);
        OriginIGO::HookAPI(NULL, "SetAnimatedThemeCursor", (LPVOID)SetAnimatedThemeCursorHook, (LPVOID*)&SetAnimatedThemeCursorHookNext);
        OriginIGO::HookAPI(NULL, "SetCursor", (LPVOID)SetCursorHook, (LPVOID*)&SetCursorHookNext);
        OriginIGO::HookAPI(NULL, "SetCCursor", (LPVOID)SetCCursorHook, (LPVOID*)&SetCCursorHookNext);
        OriginIGO::HookAPI(NULL, "HideCursor", (LPVOID)HideCursorHook, (LPVOID*)&HideCursorHookNext);
        OriginIGO::HookAPI(NULL, "InitCursor", (LPVOID)InitCursorHook, (LPVOID*)&InitCursorHookNext);
        OriginIGO::HookAPI(NULL, "ObscureCursor", (LPVOID)ObscureCursorHook, (LPVOID*)&ObscureCursorHookNext);
        OriginIGO::HookAPI(NULL, "ShieldCursor", (LPVOID)ShieldCursorHook, (LPVOID*)&ShieldCursorHookNext);
        OriginIGO::HookAPI(NULL, "ShowCursor", (LPVOID)ShowCursorHook, (LPVOID*)&ShowCursorHookNext);
        OriginIGO::HookAPI(NULL, "SetQDGlobalsArrow", (LPVOID)SetQDGlobalsArrowHook, (LPVOID*)&SetQDGlobalsArrowHookNext);
        OriginIGO::HookAPI(NULL, "QDSetNamedPixMapCursor", (LPVOID)QDSetNamedPixMapCursorHook, (LPVOID*)&QDSetNamedPixMapCursorHookNext);
        OriginIGO::HookAPI(NULL, "QDRegisterNamedPixMapCursor", (LPVOID)QDRegisterNamedPixMapCursorHook, (LPVOID*)&QDRegisterNamedPixMapCursorHookNext);
        OriginIGO::HookAPI(NULL, "GetMouse", (LPVOID)GetMouseHook, (LPVOID*)&GetMouseHookNext);
        OriginIGO::HookAPI(NULL, "HIGetMousePosition", (LPVOID)HIGetMousePositionHook, (LPVOID*)&HIGetMousePositionHookNext);
        OriginIGO::HookAPI(NULL, "GetGlobalMouse", (LPVOID)GetGlobalMouseHook, (LPVOID*)&GetGlobalMouseHookNext);
        
        OriginIGO::HookAPI(NULL, "CGDisplayHideCursor", (LPVOID)CGDisplayHideCursorHook, (LPVOID *)&CGDisplayHideCursorHookNext);
        OriginIGO::HookAPI(NULL, "CGDisplayShowCursor", (LPVOID)CGDisplayShowCursorHook, (LPVOID *)&CGDisplayShowCursorHookNext);
        OriginIGO::HookAPI(NULL, "CGDisplayMoveCursorToPoint", (LPVOID)CGDisplayMoveCursorToPointHook, (LPVOID *)&CGDisplayMoveCursorToPointHookNext);
        OriginIGO::HookAPI(NULL, "CGCursorIsVisible", (LPVOID)CGCursorIsVisibleHook, (LPVOID *)&CGCursorIsVisibleHookNext);
        //OriginIGO::HookAPI(NULL, "CGCursorIsDrawnInFramebuffer", (LPVOID)CGCursorIsDrawnInFramebufferHook, (LPVOID *)&CGCursorIsDrawnInFramebufferHookNext);
        OriginIGO::HookAPI(NULL, "CGAssociateMouseAndMouseCursorPosition", (LPVOID)CGAssociateMouseAndMouseCursorPositionHook, (LPVOID *)&CGAssociateMouseAndMouseCursorPositionHookNext);
        OriginIGO::HookAPI(NULL, "CGWarpMouseCursorPosition", (LPVOID)CGWarpMouseCursorPositionHook, (LPVOID *)&CGWarpMouseCursorPositionHookNext);
        OriginIGO::HookAPI(NULL, "CGGetLastMouseDelta", (LPVOID)CGGetLastMouseDeltaHook, (LPVOID *)&CGGetLastMouseDeltaHookNext);
        OriginIGO::HookAPI(NULL, "CGEventCreate", (LPVOID)CGEventCreateHook, (LPVOID *)&CGEventCreateHookNext);
        OriginIGO::HookAPI(NULL, "CGEventGetLocation", (LPVOID)CGEventGetLocationHook, (LPVOID *)&CGEventGetLocationHookNext);
        OriginIGO::HookAPI(NULL, "CGEventSourceButtonState", (LPVOID)CGEventSourceButtonStateHook, (LPVOID *)&CGEventSourceButtonStateHookNext);
        
        // Force initialize of our handler
        [IGOCursorState instance];
    }
    
    return true;
}

// Enable/disable all the hooks
void EnableCursorHooks(bool enabled)
{
    [[IGOCursorState instance] SetupComplete:enabled];
}

// Clean up our hooks + swizzled methods
void CleanupCursorHooks()
{
    NSError* myError = nil;
    
    Class nsCursor = NSClassFromString(@"NSCursor");
    if (nsCursor)
    {
        UNSWIZZLE_METHOD(NSCursor, nsCursor, set, ,  myError)
        UNSWIZZLE_METHOD(NSCursor, nsCursor, push, , myError)
        UNSWIZZLE_METHOD(NSCursor, nsCursor, pop, , myError)
        UNSWIZZLE_CLASS_METHOD(NSCursor, nsCursor, hide, , myError)
        UNSWIZZLE_CLASS_METHOD(NSCursor, nsCursor, unhide, , myError)
        UNSWIZZLE_CLASS_METHOD(NSCursor, nsCursor, pop, , myError)
        UNSWIZZLE_CLASS_METHOD(NSCursor, nsCursor, setHiddenUntilMouseMoves, :, myError)
        UNSWIZZLE_CLASS_METHOD(NSCursor, nsCursor, currentCursor, , myError)
    }
    
    Class nsWindow = NSClassFromString(@"NSWindow");
    if (nsWindow)
    {
        UNSWIZZLE_METHOD(NSWindow, nsWindow, areCursorRectsEnabled, , myError)
        UNSWIZZLE_METHOD(NSWindow, nsWindow, enableCursorRects, , myError)
        UNSWIZZLE_METHOD(NSWindow, nsWindow, disableCursorRects, , myError)
    }
    
    Class nsView = NSClassFromString(@"NSView");
    if (nsView)
    {
        UNSWIZZLE_METHOD(NSView, nsView, addCursorRect, :cursor:, myError)
        
        /*
        UNSWIZZLE_METHOD(NSView, nsView, removeCursorRect, :cursor:, myError)
        UNSWIZZLE_METHOD(NSView, nsView, discardCursorRects, , myError)
        UNSWIZZLE_METHOD(NSView, nsView, resetCursorRects, , myError);
        
        UNSWIZZLE_METHOD(NSView, nsView, addTrackingRect, :owner:userData:assumeInside:, myError)
        UNSWIZZLE_METHOD(NSView, nsView, removeTrackingRect, :, myError)
        UNSWIZZLE_METHOD(NSView, nsView, addTrackingArea, :, myError)
        UNSWIZZLE_METHOD(NSView, nsView, removeTrackingArea, :, myError)
        UNSWIZZLE_METHOD(NSView, nsView, trackingAreas, , myError)
        UNSWIZZLE_METHOD(NSView, nsView, updateTrackingAreas, , myError)
         */
    }
    
    Class nsEvent = NSClassFromString(@"NSEvent");
    if (nsEvent)
    {
        UNSWIZZLE_CLASS_METHOD(NSEvent, nsEvent, mouseLocation, , myError)
    }

    OriginIGO::UnhookAPI(NULL, "SetThemeCursor", (LPVOID*)&SetThemeCursorHookNext);
    OriginIGO::UnhookAPI(NULL, "SetAnimatedThemeCursor", (LPVOID*)&SetAnimatedThemeCursorHookNext);
    OriginIGO::UnhookAPI(NULL, "SetCursor", (LPVOID*)&SetCursorHookNext);
    OriginIGO::UnhookAPI(NULL, "SetCCursor", (LPVOID*)&SetCCursorHookNext);
    OriginIGO::UnhookAPI(NULL, "HideCursor", (LPVOID*)&HideCursorHookNext);
    OriginIGO::UnhookAPI(NULL, "InitCursor", (LPVOID*)&InitCursorHookNext);
    OriginIGO::UnhookAPI(NULL, "ObscureCursor", (LPVOID*)&ObscureCursorHookNext);
    OriginIGO::UnhookAPI(NULL, "ShieldCursor", (LPVOID*)&ShieldCursorHookNext);
    OriginIGO::UnhookAPI(NULL, "ShowCursor", (LPVOID*)&ShowCursorHookNext);
    OriginIGO::UnhookAPI(NULL, "SetQDGlobalsArrow", (LPVOID*)&SetQDGlobalsArrowHookNext);
    OriginIGO::UnhookAPI(NULL, "QDSetNamedPixMapCursor", (LPVOID*)&QDSetNamedPixMapCursorHookNext);
    OriginIGO::UnhookAPI(NULL, "QDRegisterNamedPixMapCursor", (LPVOID*)&QDRegisterNamedPixMapCursorHookNext);
    OriginIGO::UnhookAPI(NULL, "GetMouse", (LPVOID*)&GetMouseHookNext);
    OriginIGO::UnhookAPI(NULL, "HIGetMousePosition", (LPVOID*)&HIGetMousePositionHookNext);
    OriginIGO::UnhookAPI(NULL, "GetGlobalMouse", (LPVOID*)&GetGlobalMouseHookNext);
    
    OriginIGO::UnhookAPI(NULL, "CGDisplayHideCursor", (LPVOID *)&CGDisplayHideCursorHookNext);
    OriginIGO::UnhookAPI(NULL, "CGDisplayShowCursor", (LPVOID *)&CGDisplayShowCursorHookNext);
    OriginIGO::UnhookAPI(NULL, "CGDisplayMoveCursorToPoint", (LPVOID *)&CGDisplayMoveCursorToPointHookNext);
    OriginIGO::UnhookAPI(NULL, "CGCursorIsVisible", (LPVOID *)&CGCursorIsVisibleHookNext);
    //OriginIGO::UnhookAPI(NULL, "CGCursorIsDrawnInFramebuffer", (LPVOID *)&CGCursorIsDrawnInFramebufferHookNext);
    OriginIGO::UnhookAPI(NULL, "CGAssociateMouseAndMouseCursorPosition", (LPVOID *)&CGAssociateMouseAndMouseCursorPositionHookNext);
    OriginIGO::UnhookAPI(NULL, "CGWarpMouseCursorPosition", (LPVOID *)&CGWarpMouseCursorPositionHookNext);
    OriginIGO::UnhookAPI(NULL, "CGGetLastMouseDelta", (LPVOID *)&CGGetLastMouseDeltaHookNext);
    OriginIGO::UnhookAPI(NULL, "CGEventCreate", (LPVOID *)&CGEventCreateHookNext);
    OriginIGO::UnhookAPI(NULL, "CGEventGetLocation", (LPVOID *)&CGEventGetLocationHookNext);
    OriginIGO::UnhookAPI(NULL, "CGEventSourceButtonState", (LPVOID *)&CGEventSourceButtonStateHookNext);
}

void SetIGOCursor(IGOIPC::CursorType type)
{
#ifdef SHOW_CURSOR_CALLS
    
#define SET_CURSOR(x, y) case IGOIPC::x: { IGO_TRACE("SetIGOCursor " #y); cursor = y; } break
#define SET_LOCAL_CURSOR(x, y) case IGOIPC::x: { IGO_TRACE("SetIGOCursor " #y); cursor = (NSCursor*)[[IGOCursorState instance] y]; } break
    
#else
    
#define SET_CURSOR(x, y) case IGOIPC::x: cursor = y; break
#define SET_LOCAL_CURSOR(x, y) case IGOIPC::x: cursor = (NSCursor*)[[IGOCursorState instance] y]; break
    
#endif
    
	NSCursor* cursor = nil;
    
    if ([[IGOCursorState instance] IsSetupComplete])
    {
        if ([IGOCursorState extended])
        {
            switch (type)
            {
                    SET_CURSOR(CURSOR_ARROW, _arrowCursorCopy);
                    SET_CURSOR(CURSOR_CROSS, _crosshairCursorCopy);
                    SET_CURSOR(CURSOR_HAND, _pointingHandCursorCopy);
                    SET_CURSOR(CURSOR_IBEAM, _IBeamCursorCopy);
                    SET_CURSOR(CURSOR_SIZENS, _resizeUpDownCursorCopy);
                    SET_CURSOR(CURSOR_SIZEWE, _resizeLeftRightCursorCopy);
                    
                    SET_CURSOR(CURSOR_SIZEALL, _moveCursorCopy);
                    SET_CURSOR(CURSOR_SIZENESW, _windowResizeNorthEastSouthWestCursorCopy);
                    SET_CURSOR(CURSOR_SIZENWSE, _windowResizeNorthWestSouthEastCursorCopy);
                    
                    SET_CURSOR(CURSOR_UNKNOWN, _arrowCursorCopy);
            }
        }
        
        else
        {
            switch (type)
            {
                    SET_CURSOR(CURSOR_ARROW, _arrowCursorCopy);
                    SET_CURSOR(CURSOR_CROSS, _crosshairCursorCopy);
                    SET_CURSOR(CURSOR_HAND, _pointingHandCursorCopy);
                    SET_CURSOR(CURSOR_IBEAM, _IBeamCursorCopy);
                    SET_CURSOR(CURSOR_SIZENS, _resizeUpDownCursorCopy);
                    SET_CURSOR(CURSOR_SIZEWE, _resizeLeftRightCursorCopy);
                    
                    SET_LOCAL_CURSOR(CURSOR_SIZEALL, MoveCursor);
                    SET_LOCAL_CURSOR(CURSOR_SIZENESW, NESWCursor);
                    SET_LOCAL_CURSOR(CURSOR_SIZENWSE, NWSECursor);
                    
                    SET_CURSOR(CURSOR_UNKNOWN, _arrowCursorCopy);
            }
        }
        
        if (cursor)
        {
#ifdef SHOW_CURSOR_CALLS
            IGO_TRACEF("Updating IGO cursor (%d, %p)", type, cursor);
#endif
            
            if ([[IGOCursorState instance] ChangeCursorOnMainThread])
			{
				//[[IGOCursorState instance] performSelectorOnMainThread:@selector(Overwrite:) withObject:cursor waitUntilDone:NO];
	            [[IGOCursorState instance] PostMainThreadCursorEvent:MainThreadCommand_SET_CURSOR WithObject:cursor];
			}
            
            else
                [[IGOCursorState instance] Overwrite:cursor];
        }
    }
    
#undef SET_CURSOR
#undef SET_LOCAL_CURSOR
}

bool IsIGOEnabled()
{
    return [[IGOCursorState instance] IsIGOEnabled];
}

void SetCursorIGOState(bool enabled, bool immediateUpdate, bool skipLocationRestore)
{
    [[IGOCursorState instance] SetIGOEnabled:enabled WithImmediateUpdate:immediateUpdate AndSkipLocationRestore:skipLocationRestore];
}

void SetCursorFocusState(bool hasFocus)
{
    [[IGOCursorState instance] SetFocus:hasFocus];
}

void SetCursorWindowInfo(int posX, int posY, int width, int height)
{
    CGRect rect = CGRectMake(posX, posY, width, height);
    [[IGOCursorState instance] SetWindowInfo:rect];
}

void NotifyCursorDetachedFromMouse()
{
    [[IGOCursorState instance] NotifyCursorDetachedFromMouse];
}

void NotifyMainThreadCursorEvent()
{
    [[IGOCursorState instance] NotifyMainThreadCursorEvent];
}

void GetRealMouseLocation(float* x, float *y)
{
    [[IGOCursorState instance] GetRealMouseLocation:x AndY:y];
}

#endif // ORIGIN_MAC
