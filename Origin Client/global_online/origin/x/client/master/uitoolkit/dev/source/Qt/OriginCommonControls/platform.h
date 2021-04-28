// Copyright (c) 2012-2013 Electronic Arts, Inc. -- All Rights Reserved.

#ifndef __TOOLKIT_PLATFORM_H__
#define __TOOLKIT_PLATFORM_H__

#include <QWidget>

namespace ToolkitPlatform
{
#if defined(Q_OS_MAC)

    bool supportsFullscreen(const QWidget* window);
    void toggleFullscreen(const QWidget* window);
    bool isFullscreen(const QWidget* window);
    void ConstrainWidgetToDesktopArea(QWidget& widget, QPoint& location);
    
    // Make a transparency part of a widget clickable.
    void MakeWidgetTransparencyClickable(int winId);
    
    // Get the currently available screen area for a specific widget - this is necessary as Qt doesn't properly update the
    // available area for Mavericks (depending on the configuration) + if configuration changed (ie system preferences/automatically
    // hide and show the dock)
    QRect ScreenAvailableArea(const QWidget* window);
    
    // Disable/Enable AppKit cursor management (ie we don't want AppKit to override our cursor while we are resizing a window)
    void EnableSystemCursorManagement(const QWidget* window, bool enabled);

    // After moving a window around, make sure it doesn't overlap the menubar - if it's the case, the method returns a new location that the
    // window could move to to avoid the menu bar.
    QPoint MoveWindowAwayFromMenuBar(const QWidget* window, const QPoint& location);
    
    // Check whether this screen should be taken into account when checking whether a window is visible/needs re-allocation
    bool IsScreenRelevant(const QWidget* window, int screenNumber);
    
    // Prepare the window for a potential move op
    void PrepareForPotentialWindowMove(const QWidget* window);
    
    // Done moving the window -> reset window properties as necessary
    void ResetAfterPotentialWindowMove(const QWidget* window);
    
#elif defined(Q_OS_WIN)

    inline void ConstrainWidgetToDesktopArea(QWidget&, QPoint&) {}

#endif

}

#endif // __TOOLKIT_PLATFORM_H__
