//
//  MacNativeSupport.cpp
//  OriginCrashReporter
//
//  Created by Frederic Meraud on 4/3/13.
//  Copyright (c) 2013 Electronic Arts. All rights reserved.
//

#include "MacNativeSupport.h"

#ifdef ORIGIN_MAC

#include <QWidget>

#import <Cocoa/Cocoa.h>

// Make the window transparent to help debugging layout
void setOpacity(QWidget* window, float alpha)
{
    NSWindow* nativeWindow = [(NSView*)window->winId() window];
    [nativeWindow setBackgroundColor:[NSColor clearColor]];
    [nativeWindow setOpaque:NO];
    [nativeWindow setAlphaValue:alpha];
}

// Hides all window titlebar decorations such as close, minimize, maximize, zoom buttons.
// Note the call to [NSWindow setStyleMask:] causes the view hierarchy to be rebuilt, confusing
// Qt, so this must be the first call made on a Qt window during its construction.
void hideTitlebarDecorations(QWidget* window)
{
    NSWindow* nativeWindow = [(NSView*)window->winId() window];
    [nativeWindow setStyleMask:NSTitledWindowMask];
    [[nativeWindow standardWindowButton:NSWindowFullScreenButton] setHidden:YES];
}

// Make sure the window has initial focus
void forceFocusOnWindow(QWidget* window)
{
    NSWindow* nativeWindow = [(NSView*)window->winId() window];
    [NSApp activateIgnoringOtherApps:YES];
    [nativeWindow makeKeyAndOrderFront:nativeWindow];
}

#endif