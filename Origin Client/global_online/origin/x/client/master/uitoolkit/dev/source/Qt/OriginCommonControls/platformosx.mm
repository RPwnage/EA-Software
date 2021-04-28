// Copyright (c) 2012-2013 Electronic Arts, Inc. -- All Rights Reserved.

#include "platform.h"

#include <Foundation/Foundation.h>
#include <Cocoa/Cocoa.h>
#include <QApplication>
#include <QDesktopWidget>
#include <QScreen>

#if defined(ORIGIN_DEBUG)
// set this switch to enable debug tracing of the widget-dragging constraint mechanism
#define ENABLE_CONSTRAINWIDGET_TRACING  0
#else
#define ENABLE_CONSTRAINWIDGET_TRACING  0
#endif

#if ENABLE_CONSTRAINWIDGET_TRACING
#include <QtCore/QDebug.h>
#endif

namespace ToolkitPlatform
{
    bool supportsFullscreen(const QWidget* window)
    {
        // Make sure this version of Mac OS supports fullscreen (10.7 or greater)
        NSView *nsview = (NSView *)window->winId();
        if (nsview)
        {
            NSWindow *nswindow = [nsview window];
            if ([nswindow respondsToSelector:@selector(toggleFullScreen:)]
                && [nswindow respondsToSelector:@selector(setCollectionBehavior:)])
            {
                return true;
            }
        }
        return false;
    }
    
    void toggleFullscreen(const QWidget* window)
    {
        NSView *nsview = (NSView *)window->winId();
        if (nsview)
        {
            NSWindow *nswindow = [nsview window];
            if (nswindow)
            {
                [nswindow setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
                [nswindow toggleFullScreen:nil];
            }
        }
    }

    bool isFullscreen(const QWidget* window)
    {
        NSView *nsview = (NSView *)window->winId();
        if (nsview)
        {
            NSWindow *nswindow = [nsview window];
            if (nswindow)
            {
                NSUInteger masks = [nswindow styleMask];
                return (masks & NSFullScreenWindowMask);
            }
        }
        return false;
    }
    
    // Make a transparency part of a widget clickable.
    void MakeWidgetTransparencyClickable(int winId)
    {
        if (!winId)
            return;
        
        NSView* view = (NSView*)winId;
        NSWindow* wnd = [view window];
        
        [wnd setIgnoresMouseEvents:NO];
    }
    
    // Get the currently available screen area for a specific widget - this is necessary as Qt doesn't properly update the
    // available area for Mavericks (depending on the configuration) + if configuration changed (ie system preferences/automatically
    // hide and show the dock)
    QRect ScreenAvailableArea(const QWidget* window)
    {
        QRect rect;
        
        if (window)
        {
            NSView* nsView = (NSView*)window->winId();
            if (nsView)
            {
                NSWindow* nsWindow = [nsView window];
                NSScreen* nsScreen = [nsWindow screen];
                NSRect availArea = [nsScreen visibleFrame]; // the Y info starts from the bottom of the screen
                
                // Grab the actual screen dimension
                NSDictionary* devDesc = [nsScreen deviceDescription];
                CGDirectDisplayID cid = [[devDesc objectForKey:@"NSScreenNumber"] unsignedIntValue];
                size_t height = CGDisplayPixelsHigh(cid);
                
                int top = height - (int)(availArea.origin.y + availArea.size.height);
                rect = QRect((int)availArea.origin.x, top, (int)availArea.size.width, (int)availArea.size.height);
            }
        }
        
        return rect;
    }

    // Disable/Enable AppKit cursor management (ie we don't want AppKit to override our cursor while we are resizing a window nor do we want any view tracking area/cursor rects to prevail)
    // Don't really need to make it 'per-window' since this is really used for the window with current focus, but...
    void EnableSystemCursorManagement(const QWidget* window, bool enabled)
    {
        static id cursorUpdateTap = nil;
        static NSMutableSet* disabledCursorUpdates = nil;
        if (!disabledCursorUpdates)
        {
            disabledCursorUpdates = [[NSMutableSet alloc] initWithCapacity:10];
            cursorUpdateTap = [NSEvent addLocalMonitorForEventsMatchingMask:NSCursorUpdateMask handler:^(NSEvent* event)
                               {
                                   if ([disabledCursorUpdates count] > 0)
                                   {
                                       NSWindow* window = [event window];
                                       if (window && [disabledCursorUpdates containsObject:window] == YES)
                                           return (NSEvent*)nil;
                                   }
                                   
                                   return event;
                               }];
        }
        
        if (!window)
            return;
        
        NSView* nsView = (NSView*)window->winId();
        if (nsView)
        {
            NSWindow* nsWindow = [nsView window];
            
            bool prevEnabled = [disabledCursorUpdates containsObject:nsWindow] == YES ? false : true;
            
            if (enabled)
            {
                if (!prevEnabled)
                {
                    [disabledCursorUpdates removeObject:nsWindow];
                    [nsWindow enableCursorRects];
                }
            }
            
            else
            {
                if (prevEnabled)
                {
                    [disabledCursorUpdates addObject:nsWindow];
                    [nsWindow disableCursorRects];
                }
            }
        }
    }

    // Helper to check whether we are using the "separate spaces configuration"
    bool UsingSeparateSpaces()
    {
        // We only need to check for this once, since this feature requires the user to logout.
        static bool usingSeparateSpaces = false;
        static bool separateSpacesChecked = false;
        if (!separateSpacesChecked)
        {
            separateSpacesChecked = true;
            
            SEL selector = NSSelectorFromString(@"screensHaveSeparateSpaces");
            if ([[NSScreen class] respondsToSelector:selector] == YES)
            {
                typedef BOOL (*ScreensHaveSeparateSpacesFcn)(id obj, SEL selector);
                ScreensHaveSeparateSpacesFcn haveSeparateSpacesFcn = (ScreensHaveSeparateSpacesFcn)[NSScreen methodForSelector:selector];
                
                if (haveSeparateSpacesFcn)
                    usingSeparateSpaces = haveSeparateSpacesFcn([NSScreen class], selector) == YES;
            }
        }
        
        return usingSeparateSpaces;
    }
    
    // After moving a window around, make sure it doesn't overlap the menubar - if it's the case, the method returns a new location that the
    // window could move to to avoid the menu bar.
    QPoint MoveWindowAwayFromMenuBar(const QWidget* window, const QPoint& location)
    {
        QPoint retVal = location;
        
        NSView *nsView = (NSView *)window->winId();
        if (nsView)
        {
            NSWindow *nsWindow = [nsView window];
            if (nsWindow)
            {
                // Compute the bbox of the menu bar
                // 1) for regular/shared workspace mode: the menu bar is on the main screen - the primary screen is always the first object in the list of screens + contains the menu bar + origin at (0, 0)
                // 2) for separate spaces mode (starting with Mavericks) - we have to use the window current screen, otherwise moving the window to another screen wouldn't get properly refreshed and the OS confused
                // (especially for vertically stacked screens)
                NSRect primaryScreenFrame;
                if (UsingSeparateSpaces())
                    primaryScreenFrame = [[nsWindow screen] frame];
                
                else
                    primaryScreenFrame = [[[NSScreen screens] objectAtIndex:0] frame];
                
                const CGFloat menuBarHeight = [[[NSApplication sharedApplication] mainMenu] menuBarHeight];
                NSRect menuBarBBox = {{primaryScreenFrame.origin.x, primaryScreenFrame.origin.y + primaryScreenFrame.size.height - menuBarHeight}, {primaryScreenFrame.size.width, menuBarHeight}};
                
                // Does it overlap our window?
                NSRect windowBBox = [nsWindow frame];
                if (NSIntersectsRect(windowBBox, menuBarBBox) == YES)
                {
                    // Simply snap up or down - we can simplify the logic since our windows will always be thicker than the menu bar + the mouse pointer will always be on the window titlebar.
                    CGFloat distFromTop = windowBBox.origin.y + windowBBox.size.height - primaryScreenFrame.size.height;
                    CGFloat distFromBottom = menuBarBBox.origin.y - windowBBox.origin.y;
                    
                    CGFloat yDisplacement = 0.0f;
                    if (distFromTop > distFromBottom)
                        yDisplacement = primaryScreenFrame.size.height - windowBBox.origin.y;
                    
                    else
                        yDisplacement = menuBarBBox.origin.y - windowBBox.size.height - windowBBox.origin.y;
                    
                    // Cocoa coordinate system goes left/up and Qt goes left/down
                    retVal.setY(location.y() - yDisplacement);                    
                }
                
            }
        }
        

        return retVal;
    }
    
    // Check whether this screen should be taken into account when checking whether a window is visible/needs re-allocation
    bool IsScreenRelevant(const QWidget* window, int screenNumber)
    {
        bool retVal = true;
        
        if (!window || screenNumber < 0)
            return retVal;
        
        // All screens are relevant except if we are using separate spaces
        if (!UsingSeparateSpaces())
            return retVal;
        
        NSView *nsView = reinterpret_cast<NSView *>(window->winId());
        if (nsView)
        {
            NSWindow *nsWindow = [nsView window];
            if (nsWindow)
            {
                NSScreen* wndScreen = [nsWindow screen];
                
                const QList<QScreen *> screens = QGuiApplication::screens();
                if (screenNumber < screens.count())
                {
                    NSScreen* refScreen = reinterpret_cast<NSScreen*>(screens.at(screenNumber)->nativeHandle());
                    return wndScreen == refScreen;
                }
            }
        }

        return retVal;
    }
    
    // Prepare the window for a move op
    void PrepareForPotentialWindowMove(const QWidget* window)
    {
        // When using separate spaces (starting with Mavericks), we need to use some magic to allow
        // windows to go across vertically-stacked screens)
        if (!UsingSeparateSpaces())
            return;
        
        NSView *nsView = reinterpret_cast<NSView *>(window->winId());
        if (nsView)
        {
            NSWindow *nsWindow = [nsView window];
            if (nsWindow)
                [nsWindow setMovableByWindowBackground:YES];
        }
    }
    
    // Done moving the window -> reset window properties as necessary
    void ResetAfterPotentialWindowMove(const QWidget* window)
    {
        // When using separate spaces (starting with Mavericks), we need to use some magic to allow
        // windows to go across vertically-stacked screens)
        if (!UsingSeparateSpaces())
            return;
        
        NSView *nsView = reinterpret_cast<NSView *>(window->winId());
        if (nsView)
        {
            NSWindow *nsWindow = [nsView window];
            if (nsWindow)
                [nsWindow setMovableByWindowBackground:NO];
        }
    }
    
    void ConstrainWidgetToDesktopArea(QWidget& widget, QPoint& delta)
    {
        // This is broken in different ways (especially when the screens are stacked up vertically), so we're going for a different direction right now and
        // only move away from the menu bar once the user is done moving the window, ie we let the os do its job + smooth transitions between screens
#if 0
        // determine window and its frame
        NSView* nsview = reinterpret_cast<NSView*>(widget.winId());
        NSWindow* nswindow = [nsview window];
        if (!nswindow)
            return;
        
        // determine screens
        NSArray* screens = [NSScreen screens];
        const int numScreens = [screens count];
        if (numScreens < 1)
            return;
        
        // determine main screen (w/o menu bar), root of co-ordinate system
        NSScreen* mainScreen = reinterpret_cast<NSScreen*>([screens objectAtIndex:0]);
        NSRect mainScreenRect = [mainScreen visibleFrame];
        // convert from Qt to Quartz co-ordinate system
        NSPoint clampedDelta = { static_cast<CGFloat>(delta.x()), static_cast<CGFloat>(-delta.y()) };
        
        // determine menu bar rect
        const CGFloat menuBarHeight = [[[NSApplication sharedApplication] mainMenu] menuBarHeight];
        const CGFloat menuBarHeightOffset = 2 * 2160.0; // somewhat arbitrary height offset
        NSRect menuBarRect = { { 0, mainScreenRect.size.height }, { mainScreenRect.size.width, menuBarHeight + menuBarHeightOffset } };

        NSRect windowRect = [nswindow frame];
        NSRect movedWindowRect = windowRect;
        movedWindowRect.origin.x += clampedDelta.x;
        movedWindowRect.origin.y += clampedDelta.y;
        
        // constrain movement against the main menu bar
        if (NSIntersectsRect(movedWindowRect, menuBarRect))
        {
            bool wasToLeftOfMenubar = (windowRect.origin.x + windowRect.size.width) <= menuBarRect.origin.x;
            bool wasToRightOfMenubar = windowRect.origin.x >= (menuBarRect.origin.x + menuBarRect.size.width);
            bool wasAboveMenubar = windowRect.origin.y >= (menuBarRect.origin.y + menuBarRect.size.height);
            if (wasToLeftOfMenubar && wasAboveMenubar)
            {
#if ENABLE_CONSTRAINWIDGET_TRACING
                qDebug() << "from left and above menu bar";
#endif
                
                // clamp horizontal movement to the right
                clampedDelta.x -= movedWindowRect.origin.x;
                if (clampedDelta.x < 0)
                    clampedDelta.x = 0;
            }
            else if (wasToRightOfMenubar && wasAboveMenubar)
            {
#if ENABLE_CONSTRAINWIDGET_TRACING
                qDebug() << "from right and above menu bar";
#endif
                
                // clamp horizontal movement to the left
                clampedDelta.x += menuBarRect.size.width - movedWindowRect.origin.x;
                if (clampedDelta.x > 0)
                    clampedDelta.x = 0;
            }
            else if (wasToLeftOfMenubar)
            {
#if ENABLE_CONSTRAINWIDGET_TRACING
                qDebug() << "from left and below menu bar";
#endif
                
                // determine whether to clamp horizontal or vertical movement
                NSPoint overlap = {
                    movedWindowRect.origin.x + movedWindowRect.size.width,
                    movedWindowRect.origin.y - (menuBarRect.origin.y - menuBarRect.size.height)
                };

                if (overlap.x > overlap.y)
                {
                    // clamp y
                    clampedDelta.y -= overlap.y;
                }
                else if (overlap.y > overlap.x)
                {
                    // clamp x
                    clampedDelta.x -= overlap.x;
                }
                else
                {
                    // clamp x & y
                    clampedDelta.x -= overlap.x;
                    clampedDelta.y -= overlap.y;
                }
            }
            else if (wasToRightOfMenubar)
            {
                // determine whether to clamp horizontal or vertical movement
#if ENABLE_CONSTRAINWIDGET_TRACING
                qDebug() << "from right below menu bar";
#endif
                
                NSPoint overlap = {
                    menuBarRect.size.width - movedWindowRect.origin.x,
                    movedWindowRect.origin.y - (menuBarRect.origin.y - menuBarRect.size.height)
                };
                
                if (overlap.x > overlap.y)
                {
                    // clamp y
                    clampedDelta.y -= overlap.y;
                }
                else if (overlap.y > overlap.x)
                {
                    // clamp x
                    clampedDelta.x += overlap.x;
                }
                else
                {
                    // clamp x & y
                    clampedDelta.x += overlap.x;
                    clampedDelta.y -= overlap.y;
                }
            }
            else if (!wasAboveMenubar)
            {
#if ENABLE_CONSTRAINWIDGET_TRACING
                qDebug() << "from below menu bar";
#endif
                // ignore - leave to top-of-screen constraint below
            }
            else
            {
#if ENABLE_CONSTRAINWIDGET_TRACING
                qDebug() << "from above menu bar";
#endif
                // ignore - leave to top-of-screen constraint below
            }
            
            // update the moved window rect with the clamped delta
            movedWindowRect = windowRect;
            movedWindowRect.origin.x += clampedDelta.x;
            movedWindowRect.origin.y += clampedDelta.y;
        }
        
        // constrain movement against the top of each individual screen
        for (int i = 0; i < numScreens; ++i)
        {
            NSScreen* screen = reinterpret_cast<NSScreen*>([screens objectAtIndex:i]);
            NSRect screenRect = [screen visibleFrame];
            
            if (NSIntersectsRect(movedWindowRect, screenRect))
            {
                if (movedWindowRect.origin.y + movedWindowRect.size.height > screenRect.origin.y + screenRect.size.height)
                {
                    CGFloat delta = (movedWindowRect.origin.y + movedWindowRect.size.height) - (screenRect.origin.y + screenRect.size.height);
                    clampedDelta.y -= delta;
                    movedWindowRect.origin.y -= delta;
                    
#if ENABLE_CONSTRAINWIDGET_TRACING
                    qDebug() << "constrained window on screen " << i << " by " << delta;
#endif
                }
            }
        }
        
#if ENABLE_CONSTRAINWIDGET_TRACING
        if(clampedDelta.x != 0 || clampedDelta.y != 0)
        {
            qDebug()
                << "delta in, x=" << delta.x() << ", y=" << delta.y()
                << ", out, x=" << clampedDelta.x << ", y=" << -clampedDelta.y;
        }
#endif
        
        delta.setX(clampedDelta.x);
        delta.setY(-clampedDelta.y);
        
#if ENABLE_CONSTRAINWIDGET_TRACING
        qDebug()
            << "from = (" << windowRect.origin.x << ", " << windowRect.origin.y << "), "
            << "to = (" << windowRect.origin.x + clampedDelta.x << ", " << windowRect.origin.y + clampedDelta.y << ")";
#endif
        
#endif // 0
    }
};


