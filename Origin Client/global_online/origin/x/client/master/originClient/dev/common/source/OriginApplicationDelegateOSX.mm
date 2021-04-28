//
//  OriginApplicationDelegateOSX.mm
//  originClient
//
//  Created by Kiss, Bela on 12-08-23.
//  Copyright (c) 2012 Electronic Arts. All rights reserved.
//

#import "OriginApplicationDelegateOSX.h"

#include <QFileOpenEvent>
#include <QApplication>
#include <QWidget>

#include "TelemetryAPIDLL.h"
#include "services/network/GlobalConnectionStateMonitor.h"
#include "services/platform/PlatformService.h"
#include "services/log/LogService.h"
#include "originwindow.h"
#include "originwindowmanager.h"

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

#include "services/platform/JRSwizzleOSX.h"

@interface SleepMonitor : NSObject
{
    bool isInSleepMode;
}
@property bool isInSleepMode;

-(void) receiveSleepNote: (NSNotification*) note;
-(void) receiveWakeNote: (NSNotification*) note;
@end

@implementation SleepMonitor
@synthesize isInSleepMode;

-(id) init
{
    if((self = [super init]))
    {
        isInSleepMode = false;
    }
    
    return self;
}

-(void) receiveSleepNote:(NSNotification *)note
{
    if(!isInSleepMode)
    {
        isInSleepMode = true;
        Origin::Services::Network::GlobalConnectionStateMonitor::setComputerAwake(false);
        
        GetTelemetryInterface()->Metric_APP_POWER_MODE(EbisuTelemetryAPI::PowerSleep);
        ORIGIN_LOG_EVENT << "Received entering sleep mode event from OS.";
    }
}

-(void) receiveWakeNote:(NSNotification *)note
{
    if(isInSleepMode)
    {
        GetTelemetryInterface()->Metric_APP_POWER_MODE(EbisuTelemetryAPI::PowerWake);
        ORIGIN_LOG_EVENT << "Computer waking from sleep mode.";
        isInSleepMode = false;
        Origin::Services::Network::GlobalConnectionStateMonitor::setComputerAwake(true);
    }
}

@end

static SleepMonitor* kSleepMonitor = nil;

/// A simple interface containing a handler for GetURL Apple Events.
@interface OriginAEReceiver: NSObject
+ (void)openUrlEvent:(NSAppleEventDescriptor *)event withReplyEvent: (NSAppleEventDescriptor *)replyEvent;
@end

/// A forwarding app delegate for Origin.
@interface OriginAppDelegate: NSObject <NSApplicationDelegate>
{
    NSObject <NSApplicationDelegate> *m_wrappedDelegate;
    
    bool m_triggerEnsureOnScreenCheckPhase;
}

+ (OriginAppDelegate*)instance;

/// Sets the application delegate.
- (void) setAppDelegate:(NSObject <NSApplicationDelegate> *)appDelegate;

/// Sets the reflection delegate.
- (void) setWrappedDelegate:(NSObject <NSApplicationDelegate> *) wrappedDelegate;

// Try to detect when a game went into fullscreen
- (void) menuBarIsShown: (NSNotification *) notification;
- (void) menuBarIsHidden: (NSNotification *) notification;
- (void) ensureWindowsOnScreen;
- (void)WindowBecameKeyNotification: ( NSNotification* ) notification;
@end

@implementation NSApplication (OriginExtension)

DEFINE_SWIZZLE_METHOD_HOOK(NSApplication, void, setDelegate, :(id<NSApplicationDelegate>)anObject)
{
    [[OriginAppDelegate instance] setWrappedDelegate:anObject];

    if ([self delegate] != [OriginAppDelegate instance])
        CALL_SWIZZLED_METHOD(self, setDelegate, :[OriginAppDelegate instance]);
}

@end

namespace {

void registerUrlHandler(CFStringRef scheme)
{
    CFStringRef currentHandler = LSCopyDefaultHandlerForURLScheme (scheme);
    CFStringRef thisBundleId = (CFStringRef)[[NSBundle mainBundle] bundleIdentifier];
    if (currentHandler != thisBundleId)
    {
        OSStatus httpResult = LSSetDefaultHandlerForURLScheme(scheme, thisBundleId);
        NSCAssert1( httpResult == 0, @"LSSetDefaultHandlerForURLScheme, err =", httpResult);
    }
    
    CFRelease(currentHandler);
}
    
} // unnamed namespace

namespace Origin {
namespace Client {

/// Registers the protocol scheme handlers (i.e., "origin://" and "eadm://") with the operating system.
void registerOriginProtocolSchemes()
{
	::registerUrlHandler(CFSTR("origin2"));
	::registerUrlHandler(CFSTR("origin"));
	::registerUrlHandler(CFSTR("eadm"));
}

/// Registers for system sleep/wake events with the operating system.
void interceptSleepEvents()
{
    if(kSleepMonitor == nil)
    {
        kSleepMonitor = [[SleepMonitor alloc] init];

        [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:kSleepMonitor selector:@selector(receiveSleepNote:) name: NSWorkspaceWillSleepNotification object: NULL];
        [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:kSleepMonitor selector:@selector(receiveWakeNote:) name: NSWorkspaceDidWakeNotification object: NULL];
    }
}


/// Installs our own handler for GetURL Apple Events, bypassing the Qt-builtin.
/// The old Qt handler is not saved, so this action is not reversible.
void interceptGetUrlEvents()
{
    // Set the event handler.  This obliterates the old event handler (which Qt implements in
    // -[QCocoaApplicationDelegate getUrl:withReplyEvent:] )
    NSAppleEventManager *eventManager = [NSAppleEventManager sharedAppleEventManager];
    [eventManager setEventHandler:[OriginAEReceiver self] andSelector:@selector(openUrlEvent:withReplyEvent:) forEventClass:kInternetEventClass andEventID:kAEGetURL];
}
    
/// Installs our own handler for system shutdown events with the operating system.
/// Because the implementation currently uses a forwarding app delegate (on OSX)
/// this must be called affter any framework-installed app delegates (such as
/// a Qt-internal QCocoaApplicationDelegate) are installed.  Call this function after
/// all such initialization is performed, preferably from an event handler early on,
/// but after QApplication construction.
void interceptShutdownEvents()
{
    NSObject <NSApplicationDelegate>* appDelegate = [NSApp delegate];
    [[OriginAppDelegate instance] setAppDelegate:appDelegate];
}

/// Prepares and displays the badge icon indicating the number of unread messages the user has
void setDockTile(int count)
{
    NSApplication *NSApp = [NSApplication sharedApplication];
    NSDockTile *dockTile = NSApp.dockTile;
    NSString* newLabel;
    // For now cap the number of notifications at 99, this matches what we do for
    // the chat tabs.
    // We can display a larger number that this not sure what the cap should be
    if (count <= 99)
        newLabel = [NSString stringWithFormat:@"%d", count];
    else
        newLabel = @"...";
    // If we have any messages display the badge with propper number
    if (count == 0)
        dockTile.badgeLabel = NULL;
    else
        dockTile.badgeLabel = newLabel;
}

/// Returns the list of windows in the application ordered back-to-front.
QWidgetList GetOrderedTopLevelWidgets()
{
    // Get all windows server ... windows
    NSInteger osxWndCnt;
    NSCountWindows(&osxWndCnt);
    
    NSInteger* osxWndIDs = new NSInteger[osxWndCnt];
    NSWindowList(osxWndCnt, osxWndIDs);

    QWidget** orderedWidgets = new QWidget*[osxWndCnt];
    memset(orderedWidgets, 0, sizeof(QWidget*) * osxWndCnt);
    
    QWidgetList widgets = QApplication::topLevelWidgets();
    foreach (QWidget* widget, widgets)
    {
        if (Origin::Services::PlatformService::isOIGWindow(widget))
            continue;
        
        NSView* view = (NSView*)widget->winId();
        NSWindow* wnd = [view window];
        
        // Look up the window number in our ApplicationKit ordered list of native windows
        NSInteger windowNumber = [wnd windowNumber];
        if (windowNumber > 0)
        {
            for (int idx = 0; idx < osxWndCnt; ++idx)
            {
                if (osxWndIDs[idx] == windowNumber)
                {
                    orderedWidgets[idx] = widget;
                    break;
                }
            }
        }
    }

    // Reformat the result
    QWidgetList returnedWidgets;
    for (int idx = 0; idx < osxWndCnt; ++idx)
    {
        if (orderedWidgets[idx])
            returnedWidgets.insert(0, orderedWidgets[idx]);
    }
    
    delete []osxWndIDs;
    delete []orderedWidgets;
    
    return returnedWidgets;
}

// Trigger normal NSApp flow to terminate the application (equivalent to the default Command+Q)
void triggerAppTermination()
{
    [[NSApplication sharedApplication] terminate:nil];
}

} // namespace Origin
} // namespace Client


// ================== Qt utilities from Qt source code =================
extern bool qt_sendSpontaneousEvent(QObject *receiver, QEvent *event); // defined in qapplication_xxx.cpp
QString CfStringToQString(CFStringRef str) // Lifted from qcore_mac.cpp
{
    if (!str)
        return QString();
    
    CFIndex length = CFStringGetLength(str);
    if (length == 0)
        return QString();
    
    QString string(length, Qt::Uninitialized);
    CFStringGetCharacters(str, CFRangeMake(0, length), reinterpret_cast<UniChar *>(const_cast<QChar *>(string.unicode())));
    
    return string;
}
inline QString qt_mac_NSStringToQString(const NSString *nsstr)  // lifted from qt_cocoa_helpers_mac_p.h
    { return CfStringToQString(reinterpret_cast<const CFStringRef>(nsstr)); }
inline QApplication *qAppInstance()  // lifted from qt_cocoa_helpers_mac_p.h
    { return static_cast<QApplication *>(QCoreApplication::instance()); }
// =====================================================


namespace {

//
// The following two functions are duplicated in:
//       OriginBootstrap/dev-mac/appDelegate.m
//       originClient/common/OriginApplicationDelegateOSX.mm
//
// Please keep them identical.
//

/// \brief Returns true if the passed Bundle ID matches a known web browser.
bool isWebBrowser(NSString* bundleID)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsentinel"

    // A lower-case list of Mac web browsers.
    NSArray* macBrowserList = @[
           @"safari"
           , @"omniweb"
           , @"cruz"
           , @"chrome"
           , @"firefox"
           , @"camino"
           , @"stainless"
           , @"opera"
           , @"flock"
           , @"fake"
           ];
    
#pragma clang diagnostic pop

    // Get the lower-case name of the event sender.  (From the last component of the bundle identifier.)
    NSArray* bundleComponents = [bundleID componentsSeparatedByString: @"."];
    NSString* name = [(NSString*)[bundleComponents lastObject] lowercaseString];
    
    // Return true if the name is in the list of Mac web browsers,
    if ( [macBrowserList containsObject: name] ) return true;
    
    // Return false when the sender cannot be confirmed to be a web browser.
    return false;
}

/// \brief Returns the Bundle ID of the event sender, if possible.
NSString* getEventSenderBundleID(NSAppleEventDescriptor * event)
{
    // Find the original sender of the event.
    NSAppleEventDescriptor *eventSource = [event attributeDescriptorForKeyword:keyOriginalAddressAttr];
    if (! eventSource) return nil;
    NSData *eventSourcePsnData = [[eventSource coerceToDescriptorType:typeProcessSerialNumber] data];
    if (! eventSourcePsnData) return nil;
    ProcessSerialNumber eventPsn = *(ProcessSerialNumber *) [eventSourcePsnData bytes];
    
    // Lookup the BundleId for the event sender.
    CFDictionaryRef procInfo = ProcessInformationCopyDictionary(&eventPsn, kProcessDictionaryIncludeAllInformationMask);
    if (! procInfo) return nil;
    NSString* senderId = (NSString*) CFDictionaryGetValue(procInfo, kCFBundleIdentifierKey);
    if (! senderId) return nil;
    
    // Return the Bundle ID.
    return senderId;
}
} // unnamed namespace

extern NSString* getWebBrowserNameFromEventSender(NSAppleEventDescriptor * event);

/// \brief A simple interface containing a handler for GetURL Apple Events.
@implementation OriginAEReceiver

/// \brief Handle Get URL Events.
/// This function is identical to the Qt-builtin handler (QCocoaApplicationDelegate -getUrl:withReplyEvent:)
/// except that if the event source is a web browser, the URL sent to a special function. 
+ (void)openUrlEvent:(NSAppleEventDescriptor *)event withReplyEvent: (NSAppleEventDescriptor *)replyEvent
{
    Q_UNUSED(replyEvent);
    
    // Extract the URL from the event.  (Original Qt implementation.)
    NSString *urlString = [[event paramDescriptorForKeyword:keyDirectObject] stringValue];
    QUrl url(qt_mac_NSStringToQString(urlString));

    { // begin Origin-specific additional behavior
        // Extract the sender of the event source.
        NSString* senderId = getEventSenderBundleID(event);
        
        ORIGIN_LOG_EVENT << "Runtime GetURL request from: " << qt_mac_NSStringToQString(senderId) << " for URL " << url.toString();

        // If the event sender is a web browser,
        if ( isWebBrowser( senderId ) )
        {
            // Send the URL to a special function.
            QMetaObject::invokeMethod( qAppInstance(), "openUrlFromBrowser", Q_ARG(QUrl, url) );
            return; // do not fall through to normal URL handling behavior
        }
        
    } // end Origin-specific additional behavior
    
    // Send a spontaneous event to handle the URL.  (Original Qt implementation.)
    QFileOpenEvent qtEvent(url);
    qt_sendSpontaneousEvent(qAppInstance(), &qtEvent);
}

@end // implementation OriginAEReceiver

/// A forwarding app delegate for Origin.
@implementation OriginAppDelegate

static OriginAppDelegate* _instance;

+ (void)initialize
{
    static BOOL initialized = NO;
    if (!initialized)
    {
        _instance = [[OriginAppDelegate alloc] init];
        
        // Swizzle the setDelegate implementation so that we can properly keep track of Qt5 changing its delegate + stop it from setting the delegate to nil when closing the app
        // (which would stop us from collecting telemetry when shutting down)
        bool success = true;
        NSError* myError = nil;
        Class nsApplication = [NSApp class];
        if (nsApplication)
        {
            SWIZZLE_METHOD(success, NSApplication, nsApplication, setDelegate, :, myError)
        }
    }
}

+ (OriginAppDelegate*)instance
{
    return _instance;
}

/// Simple init method.
- (id)init
{
    self = [super init];
    
    // Let's use this object to register for notification that will help us detect when a game goes fullscreen - couldn't find any
    // other type of registration that would work when a game actually captures the displays
    NSDistributedNotificationCenter* dnc = [NSDistributedNotificationCenter defaultCenter];
    [dnc addObserver:self selector:@selector(menuBarIsShown:) name:@"com.apple.HIToolbox.frontMenuBarShown" object:nil];
    [dnc addObserver:self selector:@selector(menuBarIsHidden:) name:@"com.apple.HIToolbox.hideMenuBarShown" object:nil];
    //NSNotificationCenter * workspaceNotificationCenter = [[NSWorkspace sharedWorkspace] notificationCenter];

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector( WindowBecameKeyNotification: ) name: NSWindowDidBecomeKeyNotification object:nil];
    
    return self;
}

/// Dealloc method resets the app delegate to the wrapped delegate.
- (void) dealloc
{
    [NSApp setDelegate:m_wrappedDelegate];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [super dealloc];
}

/// Sets the application delegate.
- (void) setAppDelegate:(NSObject <NSApplicationDelegate> *)delegate
{
    // Let the swizzling do its magic
    [NSApp setDelegate:delegate];
}

/// Sets the delegate to be wrapped.
- (void) setWrappedDelegate:(NSObject <NSApplicationDelegate> *) wrappedDelegate
{
    [wrappedDelegate retain];
    [m_wrappedDelegate release];
    m_wrappedDelegate = wrappedDelegate;
}

/// Forwards method invocations to the wrapped delegate.
- (void) forwardInvocation:(NSInvocation*) invocation
{
    SEL invocationSelector = [invocation selector];
    if ( m_wrappedDelegate && [m_wrappedDelegate respondsToSelector:invocationSelector] )
        [invocation invokeWithTarget:m_wrappedDelegate];
    else
        [self doesNotRecognizeSelector:invocationSelector];
}

/// Returns the method signature for a selector by forwarding to the wrapped delegate.
- (NSMethodSignature *) methodSignatureForSelector:(SEL)aSelector
{
    NSMethodSignature *result = [super methodSignatureForSelector:aSelector];
    if (!result && m_wrappedDelegate) {
        result = [m_wrappedDelegate methodSignatureForSelector:aSelector];
    }
    return result;
}

/// Accepts selectors based on the wrapped delegate.
- (BOOL) respondsToSelector:(SEL) aSelector
{
    BOOL result = [super respondsToSelector:aSelector];
    if ( ! result && m_wrappedDelegate )
        result = [m_wrappedDelegate respondsToSelector:aSelector];
    return result;
}

/// The NSApplicationDelegate method used to notify applications of system shutdown.
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
    ORIGIN_LOG_EVENT << "Received NSApplication applicationShouldTerminate message.";
    GetTelemetryInterface()->Metric_APP_POWER_MODE(EbisuTelemetryAPI::PowerShutdown);
    
    // Pass on the event to the wrapped delegate.
    if (m_wrappedDelegate && [m_wrappedDelegate respondsToSelector:@selector(applicationShouldTerminate:)])
        return [m_wrappedDelegate applicationShouldTerminate: sender];
    
    else
        return NSTerminateNow;
}

// We need to use the notification for the screens at the Cocoa level so that we get it AFTER the windows server has re-organized the windows
// (ie Qt uses the low-level Quartz Display notifications -> at that point, we can't reliably move the windows)
- (void)applicationDidChangeScreenParameters:(NSNotification *)notification
{
    ORIGIN_LOG_DEBUG << "Received NSApplication applicationDidChangeScreenParameters.";
    
    // Make sure we notify Qt first so that it updates the set of screens and their properties
    if (m_wrappedDelegate && [m_wrappedDelegate respondsToSelector:@selector(applicationDidChangeScreenParameters:)])
        [m_wrappedDelegate applicationDidChangeScreenParameters:notification];
    
    [self ensureWindowsOnScreen];
}

// Try to detect when a game went into fullscreen
- (void) menuBarIsShown: (NSNotification *) notification
{
    // We need to match with a previous "hidden" notification as we get many "shown" notifications
    if (m_triggerEnsureOnScreenCheckPhase)
    {
        m_triggerEnsureOnScreenCheckPhase = false;
        ORIGIN_LOG_DEBUG << "Detected com.apple.HIToolbox.hideMenuBarShown/showMenuBarShown";
        
        // Need to give a little bit of time for some games (for example Dirt 2) to properly restore the screen setup
        [NSTimer scheduledTimerWithTimeInterval:0.5 target:self selector:@selector(ensureWindowsOnScreen) userInfo:nil repeats:NO];
    }
}

- (void) menuBarIsHidden: (NSNotification *) notification
{
    m_triggerEnsureOnScreenCheckPhase = true;
}

- (void) ensureWindowsOnScreen
{
    foreach (QWidget *widget, QApplication::topLevelWidgets())
    {
        Origin::UIToolkit::OriginWindow* window = dynamic_cast<Origin::UIToolkit::OriginWindow*>(widget);
        if (window && window->manager())
            window->manager()->ensureOnScreen(true);
    }
}

-(void)WindowBecameKeyNotification: ( NSNotification* ) notification
{
    GetTelemetryInterface()->Metric_ACTIVE_TIMER_START();
    // restart the running timer if it was stopped
    // This could happen if a user logs out without closing the client
    GetTelemetryInterface()->RUNNING_TIMER_START(0);
}

@end // implementation OriginAppDelegate

