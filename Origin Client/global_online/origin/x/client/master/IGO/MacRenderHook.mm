//
//  MacGoodies.m
//  InjectedCode
//
//  Created by Frederic Meraud on 6/5/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "MacRenderHook.h"

#if defined(ORIGIN_MAC)

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#import <objc/runtime.h>
#import <sys/syslog.h>
#import <AGL/agl.h>

#include </usr/include/dlfcn.h>

#include "IGO.h"
#include "MacJRSwizzle.h"
#include "IGOIPC/IGOIPC.h"
#include "IGOIPC/IGOIPCConnection.h"
#include "IGOApplication.h"


static bool _aglSupportChecked = false;
static bool _pureCarbonAPI = false;

#define OGLFN(retval, name, params) \
typedef retval (APIENTRY *name ## Fn) params; \
static name ## Fn _ ## name = NULL;

#define GL(name) _ ## name
#define GET_GL_FUNCTION(name) _ ## name = (name ## Fn)dlsym(RTLD_DEFAULT, #name)

OGLFN(AGLContext, aglGetCurrentContext, (void));
OGLFN(WindowRef, aglGetWindowRef, (AGLContext ctx));
OGLFN(AGLDrawable, aglGetDrawable, (AGLContext ctx));

static NSWindow* _mainWindow = NULL;
static WindowRef _mainPureCarbonWindow = NULL;
static bool _renderHooksEnabled = false;
static OSSpinLock _renderLock = OS_SPINLOCK_INIT;
static SetViewPropertiesCallback _setViewPropertiesCallback = NULL;

@interface MenuNotificationHandler : NSObject
{
    MenuActivationCallback _callback;
}

+(MenuNotificationHandler*) sharedHandler;

-(void)Start:(MenuActivationCallback)callback;
-(void)BeginTracking:(NSNotification*)notification;
-(void)EndTracking:(NSNotification*)notification;

@end

@implementation MenuNotificationHandler

static MenuNotificationHandler* _sharedHandler;

+(void)initialize
{
    static BOOL initialized = NO;
    if (!initialized)
    {
        initialized = YES;
        _sharedHandler = [[MenuNotificationHandler alloc] init];
    }
}

+(MenuNotificationHandler*)sharedHandler
{
    return _sharedHandler;
}

-(void)Start:(MenuActivationCallback)callback
{
    _callback = callback;
    
    // Setup notifications to turn OIG off/on when the user is interacting with the main menu bar
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(BeginTracking:) name:NSMenuDidBeginTrackingNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(EndTracking:) name:NSMenuDidEndTrackingNotification object:nil];
}

-(void)BeginTracking:(NSNotification*)notification
{
    OSSpinLockLock(&_renderLock);
    if (_renderHooksEnabled && _callback)
    {
        // If the user is accessing the main menu, switch off IGO and disable its toggling until the user is done.
        if (notification.object == [NSApp mainMenu])
            _callback(true);
    }
    OSSpinLockUnlock(&_renderLock);
}

-(void)EndTracking:(NSNotification *)notification
{
    OSSpinLockLock(&_renderLock);
    
    if (_renderHooksEnabled && _callback)
    {
        // Re-enable the toggling of IGO
        if (notification.object == [NSApp mainMenu])
            _callback(false);
    }
    
    OSSpinLockUnlock(&_renderLock);
}

@end

#ifdef SHOW_NOTIFICATIONS
@interface DebugNotificationHandler : NSObject
{
}

+(DebugNotificationHandler*) sharedHandler;

-(void)Setup;

-(void)DistributedNotification:(NSNotification*)notification;

-(void)NSApplicationDidBecomeActiveNotification:(NSNotification*)notification;
-(void)NSApplicationDidHideNotification:(NSNotification*)notification;
-(void)NSApplicationDidFinishLaunchingNotification:(NSNotification*)notification;
-(void)NSApplicationDidResignActiveNotification:(NSNotification*)notification;
-(void)NSApplicationDidUnhideNotification:(NSNotification*)notification;
-(void)NSApplicationDidUpdateNotification:(NSNotification*)notification;
-(void)NSApplicationWillBecomeActiveNotification:(NSNotification*)notification;
-(void)NSApplicationWillHideNotification:(NSNotification*)notification;
-(void)NSApplicationWillFinishLaunchingNotification:(NSNotification*)notification;
-(void)NSApplicationWillResignActiveNotification:(NSNotification*)notification;
-(void)NSApplicationWillUnhideNotification:(NSNotification*)notification;
-(void)NSApplicationWillUpdateNotification:(NSNotification*)notification;
-(void)NSApplicationWillTerminateNotification:(NSNotification*)notification;
-(void)NSApplicationDidChangeScreenParametersNotification:(NSNotification*)notification;

-(void)NSWindowDidBecomeKeyNotification:(NSNotification*)notification;
-(void)NSWindowDidBecomeMainNotification:(NSNotification*)notification;
-(void)NSWindowDidChangeScreenNotification:(NSNotification*)notification;
-(void)NSWindowDidDeminiaturizeNotification:(NSNotification*)notification;
-(void)NSWindowDidExposeNotification:(NSNotification*)notification;
-(void)NSWindowDidMiniaturizeNotification:(NSNotification*)notification;
-(void)NSWindowDidMoveNotification:(NSNotification*)notification;
-(void)NSWindowDidResignKeyNotification:(NSNotification*)notification;
-(void)NSWindowDidResignMainNotification:(NSNotification*)notification;
-(void)NSWindowDidResizeNotification:(NSNotification*)notification;
-(void)NSWindowDidUpdateNotification:(NSNotification*)notification;
-(void)NSWindowWillCloseNotification:(NSNotification*)notification;
-(void)NSWindowWillMiniaturizeNotification:(NSNotification*)notification;
-(void)NSWindowWillMoveNotification:(NSNotification*)notification;
-(void)NSWindowWillBeginSheetNotification:(NSNotification*)notification;
-(void)NSWindowDidEndSheetNotification:(NSNotification*)notification;
-(void)NSWindowDidChangeBackingPropertiesNotification:(NSNotification*)notification;
-(void)NSWindowDidChangeScreenProfileNotification:(NSNotification*)notification;
-(void)NSWindowWillStartLiveResizeNotification:(NSNotification*)notification;
-(void)NSWindowDidEndLiveResizeNotification:(NSNotification*)notification;
-(void)NSWindowWillEnterFullScreenNotification:(NSNotification*)notification;
-(void)NSWindowDidEnterFullScreenNotification:(NSNotification*)notification;
-(void)NSWindowWillExitFullScreenNotification:(NSNotification*)notification;
-(void)NSWindowDidExitFullScreenNotification:(NSNotification*)notification;
-(void)NSWindowWillEnterVersionBrowserNotification:(NSNotification*)notification;
-(void)NSWindowDidEnterVersionBrowserNotification:(NSNotification*)notification;
-(void)NSWindowWillExitVersionBrowserNotification:(NSNotification*)notification;
-(void)NSWindowDidExitVersionBrowserNotification:(NSNotification*)notification;

-(void)NSWorkspaceWillLaunchApplicationNotification:(NSNotification*)notification;
-(void)NSWorkspaceDidLaunchApplicationNotification:(NSNotification*)notification;
-(void)NSWorkspaceDidTerminateApplicationNotification:(NSNotification*)notification;
-(void)NSWorkspaceDidHideApplicationNotification:(NSNotification*)notification;
-(void)NSWorkspaceDidUnhideApplicationNotification:(NSNotification*)notification;
-(void)NSWorkspaceDidActivateApplicationNotification:(NSNotification*)notification;
-(void)NSWorkspaceDidDeactivateApplicationNotification:(NSNotification*)notification;
-(void)NSWorkspaceDidMountNotification:(NSNotification*)notification;
-(void)NSWorkspaceDidUnmountNotification:(NSNotification*)notification;
-(void)NSWorkspaceWillUnmountNotification:(NSNotification*)notification;
-(void)NSWorkspaceDidRenameVolumeNotification:(NSNotification*)notification;
-(void)NSWorkspaceWillPowerOffNotification:(NSNotification*)notification;
-(void)NSWorkspaceWillSleepNotification:(NSNotification*)notification;
-(void)NSWorkspaceDidWakeNotification:(NSNotification*)notification;
-(void)NSWorkspaceScreensDidSleepNotification:(NSNotification*)notification;
-(void)NSWorkspaceScreensDidWakeNotification:(NSNotification*)notification;
-(void)NSWorkspaceSessionDidBecomeActiveNotification:(NSNotification*)notification;
-(void)NSWorkspaceSessionDidResignActiveNotification:(NSNotification*)notification;
-(void)NSWorkspaceDidChangeFileLabelsNotification:(NSNotification*)notification;
-(void)NSWorkspaceActiveSpaceDidChangeNotification:(NSNotification*)notification;

-(void)WorkspaceNotificationInfo:(NSNotification*)notification ExtractPath:(char*)path WithPathMaxLength:(NSUInteger)pathMaxLength AndExtractBundleID:(char*)bundleID WithBundleIDMaxLength:(NSUInteger)bundleIDMaxLength;

@end

@implementation DebugNotificationHandler

static DebugNotificationHandler* _debugSharedHandler;

+(void)initialize
{
    static BOOL initialized = NO;
    if (!initialized)
    {
        initialized = YES;
        _debugSharedHandler = [[DebugNotificationHandler alloc] init];
    }
}

+(DebugNotificationHandler*)sharedHandler
{
    return _debugSharedHandler;
}

-(void)Setup
{
    // Setup a few notifications to keep track of what's going on
    [[NSDistributedNotificationCenter defaultCenter] addObserver:self selector:@selector(DistributedNotification:) name:nil object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSApplicationDidBecomeActiveNotification:) name:NSApplicationDidBecomeActiveNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSApplicationDidHideNotification:) name:NSApplicationDidHideNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSApplicationDidFinishLaunchingNotification:) name:NSApplicationDidFinishLaunchingNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSApplicationDidResignActiveNotification:) name:NSApplicationDidResignActiveNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSApplicationDidUnhideNotification:) name:NSApplicationDidUnhideNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSApplicationDidUpdateNotification:) name:NSApplicationDidUpdateNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSApplicationWillBecomeActiveNotification:) name:NSApplicationWillBecomeActiveNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSApplicationWillHideNotification:) name:NSApplicationWillHideNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSApplicationWillFinishLaunchingNotification:) name:NSApplicationWillFinishLaunchingNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSApplicationWillResignActiveNotification:) name:NSApplicationWillResignActiveNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSApplicationWillUnhideNotification:) name:NSApplicationWillUnhideNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSApplicationWillUpdateNotification:) name:NSApplicationWillUpdateNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSApplicationWillTerminateNotification:) name:NSApplicationWillTerminateNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSApplicationDidChangeScreenParametersNotification:) name:NSApplicationDidChangeScreenParametersNotification object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowDidBecomeKeyNotification:) name:NSWindowDidBecomeKeyNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowDidBecomeMainNotification:) name:NSWindowDidBecomeMainNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowDidChangeScreenNotification:) name:NSWindowDidChangeScreenNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowDidDeminiaturizeNotification:) name:NSWindowDidDeminiaturizeNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowDidExposeNotification:) name:NSWindowDidExposeNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowDidMiniaturizeNotification:) name:NSWindowDidMiniaturizeNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowDidMoveNotification:) name:NSWindowDidMoveNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowDidResignKeyNotification:) name:NSWindowDidResignKeyNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowDidResignMainNotification:) name:NSWindowDidResignMainNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowDidResizeNotification:) name:NSWindowDidResizeNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowDidUpdateNotification:) name:NSWindowDidUpdateNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowWillCloseNotification:) name:NSWindowWillCloseNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowWillMiniaturizeNotification:) name:NSWindowWillMiniaturizeNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowWillMoveNotification:) name:NSWindowWillMoveNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowWillBeginSheetNotification:) name:NSWindowWillBeginSheetNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowDidEndSheetNotification:) name:NSWindowDidEndSheetNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowDidChangeBackingPropertiesNotification:) name:NSWindowDidChangeBackingPropertiesNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowDidChangeScreenProfileNotification:) name:NSWindowDidChangeScreenProfileNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowWillStartLiveResizeNotification:) name:NSWindowWillStartLiveResizeNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowDidEndLiveResizeNotification:) name:NSWindowDidEndLiveResizeNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowWillEnterFullScreenNotification:) name:NSWindowWillEnterFullScreenNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowDidEnterFullScreenNotification:) name:NSWindowDidEnterFullScreenNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowWillExitFullScreenNotification:) name:NSWindowWillExitFullScreenNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowDidExitFullScreenNotification:) name:NSWindowDidExitFullScreenNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowWillEnterVersionBrowserNotification:) name:NSWindowWillEnterVersionBrowserNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowDidEnterVersionBrowserNotification:) name:NSWindowDidEnterVersionBrowserNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowWillExitVersionBrowserNotification:) name:NSWindowWillExitVersionBrowserNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(NSWindowDidExitVersionBrowserNotification:) name:NSWindowDidExitVersionBrowserNotification object:nil];
    
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(NSWorkspaceWillLaunchApplicationNotification:) name:NSWorkspaceWillLaunchApplicationNotification object:nil];
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(NSWorkspaceDidLaunchApplicationNotification:) name:NSWorkspaceDidLaunchApplicationNotification object:nil];
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(NSWorkspaceDidTerminateApplicationNotification:) name:NSWorkspaceDidTerminateApplicationNotification object:nil];
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(NSWorkspaceDidHideApplicationNotification:) name:NSWorkspaceDidHideApplicationNotification object:nil];
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(NSWorkspaceDidUnhideApplicationNotification:) name:NSWorkspaceDidUnhideApplicationNotification object:nil];
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(NSWorkspaceDidActivateApplicationNotification:) name:NSWorkspaceDidActivateApplicationNotification object:nil];
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(NSWorkspaceDidDeactivateApplicationNotification:) name:NSWorkspaceDidDeactivateApplicationNotification object:nil];
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(NSWorkspaceDidMountNotification:) name:NSWorkspaceDidMountNotification object:nil];
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(NSWorkspaceDidUnmountNotification:) name:NSWorkspaceDidUnmountNotification object:nil];
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(NSWorkspaceWillUnmountNotification:) name:NSWorkspaceWillUnmountNotification object:nil];
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(NSWorkspaceDidRenameVolumeNotification:) name:NSWorkspaceDidRenameVolumeNotification object:nil];
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(NSWorkspaceWillPowerOffNotification:) name:NSWorkspaceWillPowerOffNotification object:nil];
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(NSWorkspaceWillSleepNotification:) name:NSWorkspaceWillSleepNotification object:nil];
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(NSWorkspaceDidWakeNotification:) name:NSWorkspaceDidWakeNotification object:nil];
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(NSWorkspaceScreensDidSleepNotification:) name:NSWorkspaceScreensDidSleepNotification object:nil];
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(NSWorkspaceScreensDidWakeNotification:) name:NSWorkspaceScreensDidWakeNotification object:nil];
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(NSWorkspaceSessionDidBecomeActiveNotification:) name:NSWorkspaceSessionDidBecomeActiveNotification object:nil];
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(NSWorkspaceSessionDidResignActiveNotification:) name:NSWorkspaceSessionDidResignActiveNotification object:nil];
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(NSWorkspaceDidChangeFileLabelsNotification:) name:NSWorkspaceDidChangeFileLabelsNotification object:nil];
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(NSWorkspaceActiveSpaceDidChangeNotification:) name:NSWorkspaceActiveSpaceDidChangeNotification object:nil];
}

-(void)DistributedNotification:(NSNotification*)notification
{
    char name[MAX_PATH] = {0};
    if ([notification name])
    {
        if ([[notification name] getCString:name maxLength:sizeof(name) encoding:NSUTF8StringEncoding] == NO)
            name[0] = '\0';
    }
    
    IGO_TRACEF("DistributedNotification: name=%s, object=%p, userInfo=%p", name, [notification object], [notification userInfo]);
}

-(void)NSApplicationDidBecomeActiveNotification:(NSNotification*)notification
{
    IGO_TRACE("NSApplicationDidBecomeActiveNotification");
}

-(void)NSApplicationDidHideNotification:(NSNotification*)notification
{
    IGO_TRACE("NSApplicationDidHideNotification");
}

-(void)NSApplicationDidFinishLaunchingNotification:(NSNotification*)notification
{
    IGO_TRACE("NSApplicationDidFinishLaunchingNotification");
}

-(void)NSApplicationDidResignActiveNotification:(NSNotification*)notification
{
    IGO_TRACE("NSApplicationDidResignActiveNotification");
}

-(void)NSApplicationDidUnhideNotification:(NSNotification*)notification
{
    IGO_TRACE("NSApplicationDidUnhideNotification");
}

-(void)NSApplicationDidUpdateNotification:(NSNotification*)notification
{
    //IGO_TRACE("NSApplicationDidUpdateNotification");
}

-(void)NSApplicationWillBecomeActiveNotification:(NSNotification*)notification
{
    IGO_TRACE("NSApplicationWillBecomeActiveNotification");
}

-(void)NSApplicationWillHideNotification:(NSNotification*)notification
{
    IGO_TRACE("NSApplicationWillHideNotification");
}

-(void)NSApplicationWillFinishLaunchingNotification:(NSNotification*)notification
{
    IGO_TRACE("NSApplicationWillFinishLaunchingNotification");
}

-(void)NSApplicationWillResignActiveNotification:(NSNotification*)notification
{
    IGO_TRACE("NSApplicationWillResignActiveNotification");
}

-(void)NSApplicationWillUnhideNotification:(NSNotification*)notification
{
    IGO_TRACE("NSApplicationWillUnhideNotification");
}

-(void)NSApplicationWillUpdateNotification:(NSNotification*)notification
{
    //IGO_TRACE("NSApplicationWillUpdateNotification");
}

-(void)NSApplicationWillTerminateNotification:(NSNotification*)notification
{
    IGO_TRACE("NSApplicationWillTerminateNotification");
}

-(void)NSApplicationDidChangeScreenParametersNotification:(NSNotification*)notification
{
    IGO_TRACE("NSApplicationDidChangeScreenParametersNotification");
}

-(void)NSWindowDidBecomeKeyNotification:(NSNotification*)notification
{
    IGO_TRACEF("NSWindowDidBecomeKeyNotification wnd=%p", notification.object);
}

-(void)NSWindowDidBecomeMainNotification:(NSNotification*)notification
{
    IGO_TRACEF("NSWindowDidBecomeMainNotification wnd=%p", notification.object);
}

-(void)NSWindowDidChangeScreenNotification:(NSNotification*)notification
{
    IGO_TRACEF("NSWindowDidChangeScreenNotification wnd=%p", notification.object);
}

-(void)NSWindowDidDeminiaturizeNotification:(NSNotification*)notification
{
    IGO_TRACEF("NSWindowDidDeminiaturizeNotification wnd=%p", notification.object);
}

-(void)NSWindowDidExposeNotification:(NSNotification*)notification
{
    IGO_TRACEF("NSWindowDidExposeNotification wnd=%p", notification.object);
}

-(void)NSWindowDidMiniaturizeNotification:(NSNotification*)notification
{
    IGO_TRACEF("NSWindowDidMiniaturizeNotification wnd=%p", notification.object);
}

-(void)NSWindowDidMoveNotification:(NSNotification*)notification
{
    IGO_TRACEF("NSWindowDidMoveNotification wnd=%p", notification.object);
}

-(void)NSWindowDidResignKeyNotification:(NSNotification*)notification
{
    IGO_TRACEF("NSWindowDidResignKeyNotification wnd=%p", notification.object);
}

-(void)NSWindowDidResignMainNotification:(NSNotification*)notification
{
    IGO_TRACEF("NSWindowDidResignMainNotification wnd=%p", notification.object);
}

-(void)NSWindowDidResizeNotification:(NSNotification*)notification
{
    IGO_TRACEF("NSWindowDidResizeNotification wnd=%p", notification.object);
}

-(void)NSWindowDidUpdateNotification:(NSNotification*)notification
{
    //IGO_TRACEF("NSWindowDidUpdateNotification wnd=%p", notification.object);
}

-(void)NSWindowWillCloseNotification:(NSNotification*)notification
{
    IGO_TRACEF("NSWindowWillCloseNotification wnd=%p", notification.object);
}

-(void)NSWindowWillMiniaturizeNotification:(NSNotification*)notification
{
    IGO_TRACEF("NSWindowWillMiniaturizeNotification wnd=%p", notification.object);
}

-(void)NSWindowWillMoveNotification:(NSNotification*)notification
{
    IGO_TRACEF("NSWindowWillMoveNotification wnd=%p", notification.object);
}

-(void)NSWindowWillBeginSheetNotification:(NSNotification*)notification
{
    IGO_TRACEF("NSWindowWillBeginSheetNotification wnd=%p", notification.object);
}

-(void)NSWindowDidEndSheetNotification:(NSNotification*)notification
{
    IGO_TRACEF("NSWindowDidEndSheetNotification wnd=%p", notification.object);
}

-(void)NSWindowDidChangeBackingPropertiesNotification:(NSNotification*)notification
{
    IGO_TRACEF("NSWindowDidChangeBackingPropertiesNotification wnd=%p", notification.object);
}

-(void)NSWindowDidChangeScreenProfileNotification:(NSNotification*)notification
{
    IGO_TRACEF("NSWindowDidChangeScreenProfileNotification wnd=%p", notification.object);
}

-(void)NSWindowWillStartLiveResizeNotification:(NSNotification*)notification
{
    IGO_TRACEF("NSWindowWillStartLiveResizeNotification wnd=%p", notification.object);
}

-(void)NSWindowDidEndLiveResizeNotification:(NSNotification*)notification
{
    IGO_TRACEF("NSWindowDidEndLiveResizeNotification wnd=%p", notification.object);
}

-(void)NSWindowWillEnterFullScreenNotification:(NSNotification*)notification
{
    IGO_TRACEF("NSWindowWillEnterFullScreenNotification wnd=%p", notification.object);
}

-(void)NSWindowDidEnterFullScreenNotification:(NSNotification*)notification
{
    IGO_TRACEF("NSWindowDidEnterFullScreenNotification wnd=%p", notification.object);
}

-(void)NSWindowWillExitFullScreenNotification:(NSNotification*)notification
{
    IGO_TRACEF("NSWindowWillExitFullScreenNotification wnd=%p", notification.object);
}

-(void)NSWindowDidExitFullScreenNotification:(NSNotification*)notification
{
    IGO_TRACEF("NSWindowDidExitFullScreenNotification wnd=%p", notification.object);
}

-(void)NSWindowWillEnterVersionBrowserNotification:(NSNotification*)notification
{
    IGO_TRACEF("NSWindowWillEnterVersionBrowserNotification wnd=%p", notification.object);
}

-(void)NSWindowDidEnterVersionBrowserNotification:(NSNotification*)notification
{
    IGO_TRACEF("NSWindowDidEnterVersionBrowserNotification wnd=%p", notification.object);
}

-(void)NSWindowWillExitVersionBrowserNotification:(NSNotification*)notification
{
    IGO_TRACEF("NSWindowWillExitVersionBrowserNotification wnd=%p", notification.object);
}

-(void)NSWindowDidExitVersionBrowserNotification:(NSNotification*)notification
{
    IGO_TRACEF("NSWindowDidExitVersionBrowserNotification wnd=%p", notification.object);
}

// helper for NSWorkspace notification
-(void)WorkspaceNotificationInfo:(NSNotification*)notification ExtractPath:(char*)path WithPathMaxLength:(NSUInteger)pathMaxLength AndExtractBundleID:(char*)bundleID WithBundleIDMaxLength:(NSUInteger)bundleIDMaxLength
{
    if (!path || !bundleID || !pathMaxLength || !bundleIDMaxLength)
        return;
    
    path[0] = '\0';
    bundleID[0] = '\0';
    
    NSRunningApplication* app = [[notification userInfo] objectForKey:NSWorkspaceApplicationKey];
    NSString* exeName = [[app executableURL] path];
    if (exeName)
    {
        if ([exeName getCString:path maxLength:pathMaxLength encoding:NSUTF8StringEncoding] == NO)
            path[0] = '\0';
    }
    
    NSString* bundleIdentifier = [app bundleIdentifier];
    if (bundleIdentifier)
    {
        if ([exeName getCString:bundleID maxLength:bundleIDMaxLength encoding:NSUTF8StringEncoding] == NO)
            bundleID[0] = '\0';
    }
}

-(void)NSWorkspaceWillLaunchApplicationNotification:(NSNotification*)notification
{
    char fileName[MAX_PATH];
    char bundleID[MAX_PATH];
    [self WorkspaceNotificationInfo:notification ExtractPath:fileName WithPathMaxLength:sizeof(fileName) AndExtractBundleID:bundleID WithBundleIDMaxLength:sizeof(bundleID)];
    
    IGO_TRACEF("NSWorkspaceWillLaunchApplicationNotification app='%s' (%s')", bundleID, fileName);
}

-(void)NSWorkspaceDidLaunchApplicationNotification:(NSNotification*)notification
{
    char fileName[MAX_PATH];
    char bundleID[MAX_PATH];
    [self WorkspaceNotificationInfo:notification ExtractPath:fileName WithPathMaxLength:sizeof(fileName) AndExtractBundleID:bundleID WithBundleIDMaxLength:sizeof(bundleID)];
    
    IGO_TRACEF("NSWorkspaceDidLaunchApplicationNotification app='%s' (%s')", bundleID, fileName);
}

-(void)NSWorkspaceDidTerminateApplicationNotification:(NSNotification*)notification
{
    char fileName[MAX_PATH];
    char bundleID[MAX_PATH];
    [self WorkspaceNotificationInfo:notification ExtractPath:fileName WithPathMaxLength:sizeof(fileName) AndExtractBundleID:bundleID WithBundleIDMaxLength:sizeof(bundleID)];
    
    IGO_TRACEF("NSWorkspaceDidTerminateApplicationNotification app='%s' (%s')", bundleID, fileName);
}

-(void)NSWorkspaceDidHideApplicationNotification:(NSNotification*)notification
{
    char fileName[MAX_PATH];
    char bundleID[MAX_PATH];
    [self WorkspaceNotificationInfo:notification ExtractPath:fileName WithPathMaxLength:sizeof(fileName) AndExtractBundleID:bundleID WithBundleIDMaxLength:sizeof(bundleID)];
    
    IGO_TRACEF("NSWorkspaceDidHideApplicationNotification app='%s' (%s')", bundleID, fileName);
}

-(void)NSWorkspaceDidUnhideApplicationNotification:(NSNotification*)notification
{
    char fileName[MAX_PATH];
    char bundleID[MAX_PATH];
    [self WorkspaceNotificationInfo:notification ExtractPath:fileName WithPathMaxLength:sizeof(fileName) AndExtractBundleID:bundleID WithBundleIDMaxLength:sizeof(bundleID)];
    
    IGO_TRACEF("NSWorkspaceDidUnhideApplicationNotification app='%s' (%s')", bundleID, fileName);
}

-(void)NSWorkspaceDidActivateApplicationNotification:(NSNotification*)notification
{
    char fileName[MAX_PATH];
    char bundleID[MAX_PATH];
    [self WorkspaceNotificationInfo:notification ExtractPath:fileName WithPathMaxLength:sizeof(fileName) AndExtractBundleID:bundleID WithBundleIDMaxLength:sizeof(bundleID)];
    
    IGO_TRACEF("NSWorkspaceDidActivateApplicationNotification app='%s' (%s')", bundleID, fileName);
}

-(void)NSWorkspaceDidDeactivateApplicationNotification:(NSNotification*)notification
{
    char fileName[MAX_PATH];
    char bundleID[MAX_PATH];
    [self WorkspaceNotificationInfo:notification ExtractPath:fileName WithPathMaxLength:sizeof(fileName) AndExtractBundleID:bundleID WithBundleIDMaxLength:sizeof(bundleID)];
    
    IGO_TRACEF("NSWorkspaceDidDeactivateApplicationNotification app='%s' (%s')", bundleID, fileName);
}

-(void)NSWorkspaceDidMountNotification:(NSNotification*)notification
{
    IGO_TRACE("NSWorkspaceDidMountNotification");
}

-(void)NSWorkspaceDidUnmountNotification:(NSNotification*)notification
{
    IGO_TRACE("NSWorkspaceDidUnmountNotification");
}

-(void)NSWorkspaceWillUnmountNotification:(NSNotification*)notification
{
    IGO_TRACE("NSWorkspaceWillUnmountNotification");
}

-(void)NSWorkspaceDidRenameVolumeNotification:(NSNotification*)notification
{
    IGO_TRACE("NSWorkspaceDidRenameVolumeNotification");
}

-(void)NSWorkspaceWillPowerOffNotification:(NSNotification*)notification
{
    IGO_TRACE("NSWorkspaceWillPowerOffNotification");
}

-(void)NSWorkspaceWillSleepNotification:(NSNotification*)notification
{
    IGO_TRACE("NSWorkspaceWillSleepNotification");
}

-(void)NSWorkspaceDidWakeNotification:(NSNotification*)notification
{
    IGO_TRACE("NSWorkspaceDidWakeNotification");
}

-(void)NSWorkspaceScreensDidSleepNotification:(NSNotification*)notification
{
    IGO_TRACE("NSWorkspaceScreensDidSleepNotification");
}

-(void)NSWorkspaceScreensDidWakeNotification:(NSNotification*)notification
{
    IGO_TRACE("NSWorkspaceScreensDidWakeNotification");
}

-(void)NSWorkspaceSessionDidBecomeActiveNotification:(NSNotification*)notification
{
    IGO_TRACE("NSWorkspaceSessionDidBecomeActiveNotification");
}

-(void)NSWorkspaceSessionDidResignActiveNotification:(NSNotification*)notification
{
    IGO_TRACE("NSWorkspaceSessionDidResignActiveNotification");
}

-(void)NSWorkspaceDidChangeFileLabelsNotification:(NSNotification*)notification
{
    IGO_TRACE("NSWorkspaceDidChangeFileLabelsNotification");
}

-(void)NSWorkspaceActiveSpaceDidChangeNotification:(NSNotification*)notification
{
    IGO_TRACE("NSWorkspaceActiveSpaceDidChangeNotification");
}

@end

#endif // SHOW_NOTIFICATIONS

// Recursive search for a view that could be setup for HiDPI
float SearchViewsForValidBackingScale(NSView* view)
{
    float backingScale = 0.0f;
    
    // Do a breath-first search
    NSArray* subViews = [view subviews];
    for (NSView* subView in subViews)
    {
        if ([subView respondsToSelector:@selector(wantsBestResolutionOpenGLSurface)] == YES)
        {
            bool useBackingStore = [(NSOpenGLView*)subView wantsBestResolutionOpenGLSurface] == YES;
            if (useBackingStore)
            {
                backingScale = [subView convertSizeToBacking:NSMakeSize(1.0f, 1.0f)].width;
                break;
            }
        }
    }
    
    if (backingScale > 0.0f)
        return backingScale;
    
    for (NSView* subView in subViews)
    {
        backingScale = SearchViewsForValidBackingScale(subView);
        if (backingScale > 0.0f)
            return backingScale;
    }
    
    return backingScale;
}

void CaptureViewProperties(NSWindow* wnd, NSView* view, SetViewPropertiesCallback callback)
{
    int width = 0;
    int height = 0;
    
    int posX = 0;
    int posY = 0;
    
    int offsetX = 0;
    int offsetY = 0;
    
    NSRect bounds = [view bounds];
    NSRect boundsInWindow = [view convertRect:bounds toView:nil];
    NSRect sizeBounds = boundsInWindow;
    
    NSScreen* screen = [wnd screen];
    
    width = (int)sizeBounds.size.width;
    height = (int)sizeBounds.size.height;
      
	// convertRectToScreen is supported started 10.7
	if ([wnd respondsToSelector:@selector(convertRectToScreen:)] == YES)
	{	 
		NSRect rect = [wnd convertRectToScreen:boundsInWindow];
    	posX = rect.origin.x;
    	posY = rect.origin.y;
   	}

	else
	{
		NSPoint origin = [wnd convertBaseToScreen:boundsInWindow.origin];
		posX = origin.x;
		posY = origin.y;
	}	 
    
    NSRect screenFrame = [screen frame];
    
    offsetX = posX;
    offsetY = screenFrame.size.height - (posY + height);

    NSUInteger styles = [wnd styleMask];
    bool isFullscreen = (styles & NSFullScreenWindowMask) == NSFullScreenWindowMask;
    
    // Do we have a case where the window height is larger than the native height? (ie SimCity Mac had that behavior when first going to fullscreen)
    if (isFullscreen && height > screenFrame.size.height)
    {
        height = screenFrame.size.height;
        offsetY = -posY;
    }

    // Check whether we are on a retina display/OpenGL is setup correctly to use "extended" backing store
    bool searchDone = false;
    float pixelToPointScaleFactor = 1.0f;
    if ([view respondsToSelector:@selector(wantsBestResolutionOpenGLSurface)] == YES)
    {
        bool useBackingStore = [(NSOpenGLView*)view wantsBestResolutionOpenGLSurface] == YES;
        if (useBackingStore)
        {
            searchDone = true;
            pixelToPointScaleFactor = [view convertSizeToBacking:NSMakeSize(1.0f, 1.0f)].width;
        }
    }
    
    if (!searchDone)
    {
        // Search children
        float backingScale = SearchViewsForValidBackingScale(view);
        if (backingScale > 0.0f)
            pixelToPointScaleFactor = backingScale;
    }
    
    // There seems to be a bug with the wrapping of old carbon games (for example Bejeweled 3) when it comes to detecting whether this is the
    // main window for the application - so we try both interfaces.
    // Actually the isMainWindow seems to return false sometimes, although when asking for the main window we get our window!
    bool hasFocus = [wnd isMainWindow] == YES;
    hasFocus |= [NSApp mainWindow] == wnd;
    hasFocus |= [[NSApp windows] count] == 1;
        
#if defined(__x86_64__) // Not available for 64-bit
#define GetUserFocusWindow() 0
#define ActiveNonFloatingWindow() 0
#endif

    WindowRef refWindow = (WindowRef)[wnd windowRef];
    if (refWindow)
    {
        bool carbonHasFocus = refWindow == ActiveNonFloatingWindow();
        hasFocus |= carbonHasFocus;
    }

    bool isKey = [wnd isKeyWindow];
    isKey |= wnd == [NSApp keyWindow];
    
#if defined(__x86_64__) // Not available for 64-bit
#undef GetUserFocusWindow
#undef ActiveNonFloatingWindow
#endif
    
    callback(posX, posY, width, height, offsetX, offsetY, hasFocus, isKey, isFullscreen, pixelToPointScaleFactor);
}

@implementation NSOpenGLContext (OriginExtension)

DEFINE_SWIZZLE_METHOD_HOOK(NSOpenGLContext, void, flushBuffer, )
{
    OSSpinLockLock(&_renderLock);
    
    if (_renderHooksEnabled)
    {
        _mainWindow = NULL;
        NSView* view = [self view];
        if (view && ![view isHidden])
        {
            NSWindow* wnd = [view window];
            if (wnd)
            {
                _mainWindow = wnd;
                CaptureViewProperties(wnd, view, _setViewPropertiesCallback);
            }
        }
    }
    
    OSSpinLockUnlock(&_renderLock);
    
    CALL_SWIZZLED_METHOD(self, flushBuffer, );
}
@end

bool SetupRenderHooks(SetViewPropertiesCallback viewCallback, MenuActivationCallback menuCallback)
{
    bool success = true;
    

    @autoreleasepool
    {
        // Make sure we can find NSWindows to work with
        NSArray* windows = [NSApp windows];
        if (!windows || [windows count] == 0)
        {
            IGO_TRACE("Unable to access any Cocoa windows");
            //_pureCarbonAPI = true;
            
#ifdef DONT_USE_CHECK_ANYMORE_BUT_ENABLE_OIG_AT_RUNTIME_DEPENDING_ON_KEY_WINDOW_SIZE
            WindowRef wnd = ActiveNonFloatingWindow();
            if (!wnd)
            {
                wnd = FrontNonFloatingWindow();
            }

            if (!wnd)
            {
                IGO_TRACE("Unable to find main/front WindowRef to check size - skip injection");
                success = false;
            }
            
            else
            {
                HIRect bounds;
                
#if defined(__x86_64__) // Not available for 64-bit
#define HIWindowGetBounds(a, b, c, d) 1
#endif
                OSStatus error = HIWindowGetBounds(wnd, kWindowContentRgn, kHICoordSpaceScreenPixel, &bounds);
                if (!error)
                {
                    if (bounds.size.width < OriginIGO::MIN_IGO_DISPLAY_WIDTH && bounds.size.height < OriginIGO::MIN_IGO_DISPLAY_HEIGHT)
                    {
                        IGO_TRACEF("WindowRef size (%f/%f) less than requirements - skipping injection", bounds.size.width, bounds.size.height);
                        success = false;
                    }
                    
                    else
                    {
                        IGO_TRACEF("WindowRef size (%f/%f) valid - allow injection", bounds.size.width, bounds.size.height);
                    }
                }
                
                else
                {
                    IGO_TRACE("Unable to retrieve WindowRef size - skip injection");
                    success = false;
                }
                
#if defined(__x86_64__) // Not available for 64-bit
#undef HIWindowGetBounds
#endif
            }
#endif
        }
        
        else
        {
            // Find main window and make sure it's "big enough" - this is not to inject launchers - got no better criteria to use right now!!!
            NSWindow* wnd = [NSApp mainWindow];
            if (wnd == nil)
                wnd = [NSApp keyWindow];
            
            if (wnd == nil)
            {
                for (NSUInteger idx = 0; idx < [windows count]; ++idx)
                {
                    wnd = [windows objectAtIndex:idx];
                    if ([wnd isVisible])
                        break;
                }
            }
            
#ifdef DONT_USE_CHECK_ANYMORE_BUT_ENABLE_OIG_AT_RUNTIME_DEPENDING_ON_KEY_WINDOW_SIZE
            if (wnd == nil)
            {
                IGO_TRACE("Unable to find main/key/visible NSWindow to check size - skip injection");
                success = false;
            }
            
            else
            {
                NSSize size = [[wnd contentView] frame].size;
                if (size.width < _minWindowWidth && size.height < _minWindowHeight)
                {
                    IGO_TRACEF("NSWindow size (%f/%f) less than requirements - skipping injection", size.width, size.height);
                    success = false;
                }
                
                else
                {
                    IGO_TRACEF("NSWindow size (%f/%f) valid - allow injection", size.width, size.height);
                }
            }
#endif
            
#ifdef SHOW_NOTIFICATIONS
            [[DebugNotificationHandler sharedHandler] Setup];
#endif
        }
        
        if (success)
        {
            _setViewPropertiesCallback = viewCallback;
              
            if (!_pureCarbonAPI)
            {
                Class nsGL = NSClassFromString(@"NSOpenGLContext");
                if (nsGL)
                {   
                    NSError* myError = nil;
                    
                    bool notUsed = true;
                    SWIZZLE_METHOD(notUsed, NSOpenGLContext, nsGL, flushBuffer, , myError)
                }
            }
            
            [[MenuNotificationHandler sharedHandler] Start:menuCallback];
        }
    }
    
    return success;
}

// Enable/disable all the hooks
void EnableRenderHooks(bool enabled)
{
    OSSpinLockLock(&_renderLock);
    
    _renderHooksEnabled = enabled;
    
    OSSpinLockUnlock(&_renderLock);
}

// Clean up our hooks + swizzled methods
void CleanupRenderHooks()
{
    @autoreleasepool
    {
        Class nsGL = NSClassFromString(@"NSOpenGLContext");
        if (nsGL)
        {
            NSError* myError = nil;
            
            UNSWIZZLE_METHOD(NSOpenGLContext, nsGL, flushBuffer, , myError)
        }
    }
}

void ExtractCGLContextViewProperties(CGLContextObj ctxt, SetViewPropertiesCallback callback)
{
    _mainWindow = NULL;
    if (!_aglSupportChecked)
    {
        GET_GL_FUNCTION(aglGetCurrentContext);
        GET_GL_FUNCTION(aglGetWindowRef);
        GET_GL_FUNCTION(aglGetDrawable);
        
        _aglSupportChecked = true;
        if (GL(aglGetCurrentContext))
        {
            IGO_TRACE("AGL Context available and most likely used");
        }
    }
    
    if (GL(aglGetCurrentContext))
    {
        AGLContext aglCtxt = GL(aglGetCurrentContext());
        if (aglCtxt && ExtractAGLContextViewProperties(aglCtxt, callback))
            return;
    }
    
    @autoreleasepool 
    {
        NSArray* windows = [NSApp windows];
        for (NSWindow* wnd in windows)
        {
            if ([wnd isVisible])
            {
                NSView* mainView = [wnd contentView];
                if (mainView)
                {
                    NSOpenGLContext* ctxt = [NSOpenGLContext currentContext];
                    if (ctxt)
                    {
                        NSView* ctxtView = [ctxt view];
                        if (mainView == ctxtView)
                        {
                            _mainWindow = wnd;
                            CaptureViewProperties(wnd, mainView, callback);
                            return;
                        }
                        
                        NSArray* subViews = [mainView subviews];
                        
                        for (NSView* subView in subViews)
                        {
                            if (subView == ctxtView)
                            {
                                _mainWindow = wnd;                                
                                CaptureViewProperties(wnd, mainView, callback);
                                return;
                            }
                        }
                    }
                }
            }
        }
        
    }
}

bool ExtractAGLContextViewProperties(AGLContext ctxt, SetViewPropertiesCallback callback)
{
    _mainWindow = NULL;
    if (!_aglSupportChecked)
    {
        GET_GL_FUNCTION(aglGetCurrentContext);
        GET_GL_FUNCTION(aglGetWindowRef);
        GET_GL_FUNCTION(aglGetDrawable);
        
        _aglSupportChecked = true;
        if (GL(aglGetCurrentContext))
        {
            IGO_TRACE("AGL Context available and most likely used");
        }
    }
    

    if (GL(aglGetCurrentContext))
    {
        AGLContext aglCtxt = GL(aglGetCurrentContext());
        if (aglCtxt)
        {
            WindowRef ctxtWndRef = GL(aglGetWindowRef(aglCtxt));
            if (!ctxtWndRef && GL(aglGetDrawable))
            {
                AGLDrawable drawable = GL(aglGetDrawable(aglCtxt));
                if (drawable)
                {
#if defined(__x86_64__) // Not available for 64-bit
#define GetWindowFromPort(x) 0
#endif
                    ctxtWndRef = GetWindowFromPort(drawable);
#if defined(__x86_64__) // Not available for 64-bit
#undef GetWindowFromPort
#endif
                }
            }
            
            if (ctxtWndRef)
            {
                bool foundCocoaWnd = false;
                if (!_pureCarbonAPI)
                {
                    @autoreleasepool 
                    {
                        NSArray* windows = [NSApp windows];
                        for (NSWindow* wnd in windows)
                        {
                            if ([wnd windowRef] == ctxtWndRef)
                            {
                                foundCocoaWnd = true;
                                
                                _mainWindow = wnd;
                                CaptureViewProperties(wnd, [wnd contentView], callback);
                                break;
                            }
                        }
                    }
                
                    if (foundCocoaWnd)
                        return true;                    
                }

                _mainPureCarbonWindow = ctxtWndRef;
                
                HIRect bounds;
                
#if defined(__x86_64__) // Not available for 64-bit
#define HIWindowGetBounds(a, b, c, d) 1
#define HIWindowGetGreatestAreaDisplay(a, b, c, d, e) 1
#define GetUserFocusWindow() 0
#define ActiveNonFloatingWindow() 0
#endif
                OSStatus error = HIWindowGetBounds(ctxtWndRef, kWindowContentRgn, kHICoordSpaceScreenPixel, &bounds);
                if (!error)
                {
                    int width = (int)bounds.size.width;
                    int height = (int)bounds.size.height;
                    
                    int offsetX = (int)bounds.origin.x;
                    int offsetY = (int)bounds.origin.y;
                    
                    CGDirectDisplayID displayID = CGMainDisplayID();
                    int posX = offsetX;
                    int posY = CGDisplayPixelsHigh(displayID) - (offsetY + height);
                    
                    bool isKeyWindow = ctxtWndRef == GetUserFocusWindow();
                    bool hasFocus = ctxtWndRef == ActiveNonFloatingWindow();
                    
                    bool isFullscreen = false;  // Not sure how to determine this guy - we added the option for SimCity, which uses an NSWindow
                                                // I have no test case right now and I don't want to break other games - so I'll assume that Carbon fullscreen games
                                                // capture the display
                    
                    // No support there for OpenGL/retina
                    float pixelToPointScaleFactor = 1.0f;
                    callback(posX, posY, width, height, offsetX, offsetY, hasFocus, isKeyWindow, isFullscreen, pixelToPointScaleFactor);
                    
                    return true;
                }
#if defined(__x86_64__) // Not available for 64-bit
#undef HIWindowGetBounds
#undef HIWindowGetGreatestAreaDisplay
#undef GetUserFocusWindow
#endif
            }
        }                    
    }
    
    return false;
}

void* GetMainWindow()
{
    return _mainWindow;
}

bool MainWindowHasFocus()
{
    bool hasFocus = false;

#if defined(__x86_64__) // Not available for 64-bit
#define GetUserFocusWindow() 0
#endif
    
    // Do we have a safe Cocoa representation?
    if (_mainWindow)
    {
        hasFocus = [_mainWindow isKeyWindow] == YES;
        hasFocus |= [NSApp keyWindow] == _mainWindow;
    }
    
    else
    if (_mainPureCarbonWindow)
    {
        // Do we have a pure Carbon representation?
        hasFocus = _mainPureCarbonWindow == GetUserFocusWindow();
    }

#if defined(__x86_64__) // Not available for 64-bit
#undef GetUserFocusWindow
#endif
    
    return hasFocus;
}

// Make sure Cocoa structures are setup so that we can access NSCursors in pure Carbon apps
void InitializeCocoa()
{
    if (NSApp == nil)
    {
        OriginIGO::IGOLogInfo("Initializing Cocoa in Carbon app");
        
        NSApplicationLoad();
        
        // Create dummy window to setup machinery that allow the use of NSCursor (for Carbon only apps)
        [[[NSWindow alloc] init] release];
    }
}

#undef OGLFN
#undef GL

#endif // ORIGIN_MAC


