///////////////////////////////////////////////////////////////////////////////
// OriginWindowManager.cpp
//
// Copyright (c) 2011-2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "originwindowmanager.h"
#include "platform.h"

#include <QApplication>
#include <QDateTime>
#include <QDesktopWidget>
#include <QEvent>
#include <QMouseEvent>
#include <QWidget>

#if defined(Q_OS_WIN)
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#endif

// Set this to 1 to enable the measurement of how long it takes to redraw a view during a resize operation
#define ENABLE_REDRAW_TIMING    0

#if ENABLE_REDRAW_TIMING
#include <QDebug>
#endif

namespace
{
#if defined(Q_OS_WIN)
    ///
    /// method to see if the taskbar is hidden bar on the edge
    /// making it static in case anyone else needs it...
    /// 
    /// Input: 
    /// the edge you want to check (ABE_BOTTOM, ABE_TOP, ABE_LEFT, ABE_RIGHT)
    /// the window handle to find the screen that item is on
    ///
    /// Returns: bool if there is a hidden taskbar on that edge
    ///
    bool isTaskBarHidden(const UINT edge, const HWND windowId)
    {
        APPBARDATA barData = {0};
        barData.uEdge = edge;
        barData.cbSize = sizeof(APPBARDATA);
        HWND barWin = reinterpret_cast<HWND>(SHAppBarMessage(ABM_GETAUTOHIDEBAR, &barData));

        HMONITOR monitor = MonitorFromWindow(windowId, MONITOR_DEFAULTTONULL);
        return ((IsWindow(barWin) && 
            (MonitorFromWindow(barWin, MONITOR_DEFAULTTONULL) == monitor)));
    }
#endif
#ifdef Q_OS_MAC
    // For Mac, we rely more on the transparent window margins to show the resize cursors
    // -> when dealing with dialogs, which show a shadow box, we want to limit how far outside we want to allow the cursor
    // change... and the inside is limited because of the way the events are distributed to the views.
    const int RESIZE_SLOP = 6;
#else
    const int RESIZE_SLOP = 10;
#endif
}


namespace Origin {
    namespace UIToolkit {
OriginWindowManager::OriginWindowManager(QWidget* managed, const Functionalities& function, 
    const QMargins& borderwidths, QWidget* titleBar)
    : mBorderWidths(QMargins(0,0,0,0))
    , mManaged(NULL)
    , mTitleBar(NULL)
    , mState(0)
    , mFunctionality(function)
    , mPreMinMaxGeometry(QRect(-1,-1,-1,-1))
    , mInMove(false)
    , mHasMoved(false)
{
    setTitleBar(titleBar);
    setManagedWidget(managed);
    setBorderWidths(borderwidths);
    mCurrentFocusButton = NULL;
    mDefaultButton = NULL;
    
    // On Mac - we have our own Cocoa-level notification (using the NSApplicationDelegate). This is because Qt uses
    // the low-level Quartz Display notifications - which happen BEFORE the windows server re-organizes the windows
    // which doesn't work for us since we want to move the windows ourselves
#ifdef Q_OS_WIN
    connect(QApplication::desktop(), SIGNAL(screenCountChanged(int)), this, SLOT(ensureOnScreen()));
    connect(QApplication::desktop(), SIGNAL(workAreaResized(int)), this, SLOT(ensureOnScreen()));
    connect(QApplication::desktop(), SIGNAL(resized(int)), this, SLOT(ensureOnScreen()));
#endif
}


OriginWindowManager::~OriginWindowManager()
{
    // NULL out stuff we have pointers to. The window manager doesn't own them - that's why
    // they aren't being deleted here.
    mManaged = NULL;
    mTitleBar = NULL;
    mResizableObjects.clear();
    // Disconnect anything signals/slots connected to this.
    disconnect();
}


void OriginWindowManager::setBorderWidth(const unsigned int& width)
{
    setBorderWidths(QMargins(width, width, width, width));
}


void OriginWindowManager::setTitleBar(QWidget* titleBar) 
{
    if(titleBar)
    {
        mTitleBar = titleBar;
        mTitleBar->installEventFilter(this);
    }
}

void OriginWindowManager::setManagedWidget(QWidget* widget)
{
    // Clear the list here as a safety since this function is public
    mResizableObjects.clear();

    mManaged = widget;
    if(mManaged)
    {
        mManaged->installEventFilter(this);
        addResizableItem(mManaged);
    }
}


bool OriginWindowManager::eventFilter(QObject *obj, QEvent* event)
{
    bool eventhandled = false;
    //EBILOGEVENT << "OriginWindowManager::eventFilter: " << ((unsigned int)event->type()) << " obj: " << (obj->objectName());
    
    switch(event->type())
    {
#if defined(Q_OS_WIN)
    // We hardly track on window state on Mac. However, when we enable this on
    // Mac Qt will give us a back a bad geometry rect when coming from closed.
    // EBIBUGS-18873
    case QEvent::WindowStateChange:
#endif
    case QEvent::Hide:
        saveCurrentGeometry();
        break;

    case QEvent::MouseButtonPress:
        mousePressEvent((QMouseEvent*)event);
        eventhandled = true;
        break;

    case QEvent::MouseButtonRelease:
        mouseReleaseEvent((QMouseEvent*)event);
        break;

    case QEvent::MouseMove:
        Q_ASSERT(mManaged);
        if(mFunctionality & Movable)
        {
#ifdef Q_OS_MAC
            // Only allow resizing/cursor change when the window is active
            if (!mManaged || mManaged->isActiveWindow())
            {
#endif
            if(mTitleBar && obj == mTitleBar)
            {
                // add the borders - so now we are not local to the titlebar
                mouseMoveEvent((QMouseEvent*)event, QPoint(mBorderWidths.left(), mBorderWidths.top()));
            }
            // Check against the mResizableObjects list and not just mManaged.
            else if(mManaged && isManaged(dynamic_cast<QWidget*>(obj)) && (mFunctionality & Resizable))
            {
                mouseMoveToResizeEvent((QMouseEvent*)event);
            }
#ifdef Q_OS_MAC
        }
#endif
        }
        eventhandled = true;
        break;

    case QEvent::Enter:
        Q_ASSERT(mManaged);
        if(obj != mManaged)
        {
            // HACK - If user enters into the widget's content the mouse cursor will 
            // change to a normal cursor
            // This fixes the issue of the cursor inexplicably changing into a resize 
            // cursor while not on the edge.
            if(mState & Resizing)
            {
                if ((mState & MouseDown) == 0)
                {
                    if (mManaged)
                    {
#ifdef Q_OS_MAC
                        // Let AppKit do whatever
                        ToolkitPlatform::EnableSystemCursorManagement(mManaged, true);
#endif
                        mManaged->setCursor(Qt::ArrowCursor);
                    }
                    
                    mState &= ~Resizing;
                }
            }
            
#ifdef Q_OS_MAC
            // If we are hovering over the titlebar, prepare for a possible move
            if (obj == mTitleBar)
                ToolkitPlatform::PrepareForPotentialWindowMove(mManaged);
#endif
        }
        eventhandled = true;
        break;

    case QEvent::Move:
        {
            Q_ASSERT(mManaged);
            if (!mInMove && mManaged && ((mState & Resizing)==0))
            {
#ifndef Q_OS_MAC
                mInMove = true;
            
                QRect availableGeo = totalWorkspaceGeometry();
                
                // Passed the top
                if(mManaged->geometry().y() < availableGeo.y())
                    mManaged->move(mManaged->geometry().x(), availableGeo.y());

                // For the other edges just make sure we keep part of the app onscreen

                // Top side of the window passed the bottom edge of the screen
                if(mManaged->geometry().y() > availableGeo.bottom())
                    mManaged->move(mManaged->x(), availableGeo.bottom());

                // Right side of the window passed the left edge of the screen
                if(mManaged->geometry().right() < availableGeo.x())
                    mManaged->move(availableGeo.x()-mManaged->geometry().width(), mManaged->y());

                // Left side of the window passed the right edge of the screen
                if(mManaged->geometry().x() > availableGeo.right())
                    mManaged->move(availableGeo.right(), mManaged->y());
                
                mInMove = false;
#endif
            }
        }
        break;

    //TB: whenever we leave the widget or the widgets activation state changes, reset the mouse state and the resizing state
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
    case QEvent::ActivationChange:
    case QEvent::Leave:
        {
#ifdef Q_OS_MAC
            // On Mac we are losing the drag event, add this check so we don't remove Resizing from mState if the mouse is Down
            if ((mState & MouseDown) == 0)
            {
#endif
            if(mState & Resizing)
            {
                Q_ASSERT(mManaged);
                if (mManaged)
                {
#ifdef Q_OS_MAC
                    // Let AppKit do whatever
                    ToolkitPlatform::EnableSystemCursorManagement(mManaged, true);
#endif
                    mManaged->setCursor(Qt::ArrowCursor);
                }
                mState &= ~Resizing;
            }
            mState &= ~MouseDown;
#ifdef Q_OS_MAC
            }
            
            // If we are leaving the titlebar or the window, make sure to reset move properties
            if (obj == mTitleBar || obj == mManaged)
                ToolkitPlatform::ResetAfterPotentialWindowMove(mManaged);
#endif
        }
        break;
        
    default:
        break;
    }

    return eventhandled;// ? eventhandled : QObject::eventFilter(obj, event);
}


void OriginWindowManager::mousePressEvent(QMouseEvent* event)
{
    if(mManaged && event->button() == Qt::LeftButton)
    {
        mState |= MouseDown;
        mClickPos = event->globalPos();        
    }
}


void OriginWindowManager::mouseMoveEvent(QMouseEvent* event, const QPoint& offset)
{
    if(mState & MouseDown)
    {
        Q_ASSERT(mManaged);
        
        if (mManaged)
        {
            QPoint eventGlobalPos(event->globalPos());
            QPoint mouseDelta(eventGlobalPos - mClickPos);
            
            QPoint newPos = mManaged->pos() + mouseDelta;
            if (mTitleBar)
            {
#ifdef Q_OS_MAC
                QRect availableGeo = totalWorkspaceGeometry();
                QSize size = mManaged->size();

                if(newPos.y() < availableGeo.y())
                    newPos.setY(availableGeo.y());
                
                // For the other edges just make sure we keep part of the app onscreen
                
                // Top side of the window passed the bottom edge of the screen
                if(newPos.y() > availableGeo.bottom())
                    newPos.setY(availableGeo.bottom());
                
                // Right side of the window passed the left edge of the screen
                if((newPos.x() + size.width()) < availableGeo.x())
                    newPos.setX(availableGeo.x() - size.width());
                
                // Left side of the window passed the right edge of the screen
                if(newPos.x() > availableGeo.right())
                    newPos.setX(availableGeo.right());
#else
                ToolkitPlatform::ConstrainWidgetToDesktopArea(*mManaged, mouseDelta);
#endif
            }

            if (mManaged->pos() != newPos)
            {
                mHasMoved = true;
                mManaged->move(newPos);
                mClickPos = event->globalPos();
            }
        }
        mState &= ~Zoomed;
    }
}


void OriginWindowManager::mouseMoveToResizeEvent(QMouseEvent* event)
{
    Q_ASSERT(mManaged);
    
    static bool left = false, right = false, bottom = false, top = false;
    static QRect origGeo;
    
    if (!(mState & MouseDown))
    {
        origGeo = mManaged->geometry();
#ifdef Q_OS_MAC
        mScreenAvailGeometryCache = QRect();
#endif
        borderHitTest(event->pos(), top, right, bottom, left);
        return;
    }

#ifdef Q_OS_MAC
    // Make sure the system itself (AppKit) (or some other view tracking area) doesn't override our cursor while we are resizing
    ToolkitPlatform::EnableSystemCursorManagement(mManaged, false);
#endif
        
    if ( mManaged && (mState & Resizing))
    {
        const QPoint mousePos = event->globalPos();
        int dx = mousePos.x() - mClickPos.x();
        int dy = mousePos.y() - mClickPos.y();

        int newX = origGeo.x();
        int newY = origGeo.y();
        int newWidth = origGeo.width();
        int newHeight = origGeo.height();
        
        if (left)
        {
            newX += dx;
            newWidth -= dx;
            if (newWidth < mManaged->minimumWidth())
            {
                newX += (newWidth - mManaged->minimumWidth());
                newWidth = mManaged->minimumWidth();
            }
        }
        else if (right)
        {
            newWidth += dx;
        }

        if (top)
        {
            newY += dy;
            newHeight -= dy;
            if (newHeight < mManaged->minimumHeight())
            {
                newY += (newHeight - mManaged->minimumHeight());
                newHeight = mManaged->minimumHeight();
            }
        }
        else if (bottom)
        {
            newHeight += dy;
#ifdef Q_OS_MAC
            // For Mac, we need to stop resizing not to go over the dock - usually the OS would take care of that, but not when we use frameless windows!
            // Also we need to allow for a few extra pixels to compensate for the transparent border
            static const int EXTRA_BOTTOM_AVAILABLE_PIXELS = 4;
            
            if (mScreenAvailGeometryCache.isNull())
                mScreenAvailGeometryCache = ToolkitPlatform::ScreenAvailableArea(mManaged);
            
            int dockOverlap = (newY + newHeight) - (mScreenAvailGeometryCache.bottom() + EXTRA_BOTTOM_AVAILABLE_PIXELS);
            if (dockOverlap > 0)
                newHeight = newHeight - dockOverlap;
#endif
        }
        
#if ENABLE_REDRAW_TIMING
        qint64 startTime = QDateTime::currentMSecsSinceEpoch();
        qDebug() << "**** Start: " << startTime << " ms";
#endif

        const QRect& currGeo = mManaged->geometry();
        if (currGeo.x() == newX && currGeo.y() == newY && (currGeo.width() != newWidth || currGeo.height() != newHeight))
        {
            mManaged->resize(newWidth, newHeight);
            mState &= ~Zoomed;
        }
        else if (currGeo.x() != newX || currGeo.y() != newY || currGeo.width() != newWidth || currGeo.height() != newHeight)
        {
            mManaged->setGeometry(newX, newY, newWidth, newHeight);
            mState &= ~Zoomed;
        }
        
#if ENABLE_REDRAW_TIMING
        qint64 endTime = QDateTime::currentMSecsSinceEpoch();
        qDebug() << "**** End: " << endTime << " ms";
        qDebug() << "**** Delta: " << endTime - startTime << " ms";
#endif
    }
}


void OriginWindowManager::mouseReleaseEvent(QMouseEvent* event)
{
    if(event->button() == Qt::LeftButton)
    {
        mState &= ~MouseDown;
        if(mState & Resizing)
        {
            mState &= ~Resizing;
            Q_ASSERT(mManaged);
            if (mManaged)
            {
#ifdef Q_OS_MAC
                // Let AppKit do whatever
                ToolkitPlatform::EnableSystemCursorManagement(mManaged, true);
#endif
                mManaged->setCursor(Qt::ArrowCursor);
            }
        }
        // If the titlebar has been moved offscreen somehow, move the window back onscreen
        if (isTitlebarOnscreen() == false)
        {
            ensureOnScreen();
        }
        
        if (mHasMoved)
        {
            mHasMoved = false;
            
#ifdef Q_OS_MAC
            
            // Make sure we don't cross/overlap the menu bar
            QPoint newPos = ToolkitPlatform::MoveWindowAwayFromMenuBar(mManaged, mManaged->pos());
            if (newPos != mManaged->pos())
                mManaged->move(newPos);
#endif
        }
    }
}

bool OriginWindowManager::hitBorder(const QPoint& mousePos, bool& top, bool& right, bool& bottom, bool& left)
{
    // This gets the contents of the object - excluding the border
    Q_ASSERT(mManaged);
    if (mManaged)
    {
        QRect r;
        r.setLeft(mManaged->rect().left() + mBorderWidths.left());
        r.setTop(mManaged->rect().top() + mBorderWidths.top());
        r.setRight(mManaged->rect().right() - mBorderWidths.right());
        r.setBottom(mManaged->rect().bottom() - mBorderWidths.bottom());

        // Find out what edge the mouse is over
        top = qAbs((mousePos.y() - r.top())) <= RESIZE_SLOP;
        right = qAbs((mousePos.x() - r.right())) <= RESIZE_SLOP;
        bottom = qAbs(mousePos.y() - r.bottom()) <= RESIZE_SLOP;
        left = qAbs(mousePos.x() - r.left()) <= RESIZE_SLOP;

        return (top || right || bottom || left);
    }

    // if you couldn't test, return false
    return false;
}

void OriginWindowManager::borderHitTest(const QPoint& mousePos, bool& top, bool& right, bool& bottom, bool& left)
{
    Q_ASSERT(mManaged);
    if (mManaged)
    {
        hitBorder(mousePos, top, right, bottom, left);

        bool hor = left || right;
        // Change to expected cursor
        if (hor && bottom) 
        {
            if (left)
                mManaged->setCursor(Qt::SizeBDiagCursor);
            else
                mManaged->setCursor(Qt::SizeFDiagCursor);
            mState |= Resizing;
        } 
        else if (hor && top) 
        {
            if (right)
                mManaged->setCursor(Qt::SizeBDiagCursor);
            else
                mManaged->setCursor(Qt::SizeFDiagCursor);
            mState |= Resizing;
        } 
        else if (hor) 
        {
            mManaged->setCursor(Qt::SizeHorCursor);
            mState |= Resizing;
        } 
        else if (bottom || top) 
        {
            mManaged->setCursor(Qt::SizeVerCursor);
            mState |= Resizing;
        } 
        else 
        {
            mManaged->setCursor(Qt::ArrowCursor);
            mState &= ~Resizing;
        }

        mBottomRightPos = mManaged->geometry().bottomRight();
    }
}

void OriginWindowManager::setupButtonFocus()
{
    Q_ASSERT(mManaged);
    if (mManaged == NULL)
        return;

    QList<Origin::UIToolkit::OriginPushButton *> pushButtonList = mManaged->findChildren<Origin::UIToolkit::OriginPushButton *>();

    for (int i = 0; i < pushButtonList.size(); ++i)
    {
        OriginPushButton* btn = pushButtonList.at(i);
        if (btn->isDefault()) 
        {
            mDefaultButton= btn;
        }
        connect(btn, SIGNAL(focusIn(OriginPushButton*)), this, SLOT(buttonFocusIn(OriginPushButton*)));
        connect(btn, SIGNAL(focusOut(OriginPushButton*)), this, SLOT(buttonFocusOut(OriginPushButton*)));
    }
}

void OriginWindowManager::resetDefaultButton(OriginPushButton* defaultBtn)
{
    Q_ASSERT(mManaged);
    if (mManaged == NULL || defaultBtn == NULL)
        return;

    mDefaultButton = defaultBtn;
}

void OriginWindowManager::buttonFocusIn(Origin::UIToolkit::OriginPushButton* btn)
{
    if (btn == NULL)
        return;

    if (mCurrentFocusButton == btn) 
    {
        // mostly this means that the window gained and lost focus
        return;
    }

    // possible that there is no current focus button; then the default has been setup correctly so 
    // set the old type to be tertiary
    Origin::UIToolkit::OriginPushButton::ButtonType currentFocusType = Origin::UIToolkit::OriginPushButton::ButtonTertiary;
    if (mCurrentFocusButton)
        currentFocusType = mCurrentFocusButton->getButtonType();
    Origin::UIToolkit::OriginPushButton::ButtonType btnType = btn->getButtonType();

    switch (btnType)
    {
    case Origin::UIToolkit::OriginPushButton::ButtonTertiary:   
        {
            // if the old button was also tertiary, then we have nothing to do
            if (currentFocusType == Origin::UIToolkit::OriginPushButton::ButtonTertiary)
            {
                break;
            }
            else 
            {
                // otherwise, we need to go back to the original default if that wasn't the one previously selected 
                if (mDefaultButton)
                {
                    mDefaultButton->setButtonType(Origin::UIToolkit::OriginPushButton::ButtonPrimary);
                    mDefaultButton->setDefault(true);
                }
            }
            break;
        }
    case Origin::UIToolkit::OriginPushButton::ButtonSecondary:
    case Origin::UIToolkit::OriginPushButton::ButtonPrimary:
        {
            // if we didn't have a current focus, then the default button was set to default
            // turn it off if we are moving to a new button
            if ((currentFocusType == Origin::UIToolkit::OriginPushButton::ButtonTertiary || mCurrentFocusButton == NULL) && btn != mDefaultButton)
            {
                if (mDefaultButton)
                {
                    mDefaultButton->setButtonType(Origin::UIToolkit::OriginPushButton::ButtonSecondary);
                    mDefaultButton->setDefault(false);
                }
            }
            btn->setButtonType(Origin::UIToolkit::OriginPushButton::ButtonPrimary);
            btn->setDefault(true);
            break;
        }
    case Origin::UIToolkit::OriginPushButton::ButtonAccept:
    case Origin::UIToolkit::OriginPushButton::ButtonDecline:
    default:
        break;
    };
    mCurrentFocusButton = btn;
}

void OriginWindowManager::buttonFocusOut(Origin::UIToolkit::OriginPushButton* btn)
{
    Q_ASSERT(mManaged);    
    if (mManaged == NULL || btn == NULL)
        return;

    // find out if the new focus is a button
    Origin::UIToolkit::OriginPushButton* newFocus = dynamic_cast<Origin::UIToolkit::OriginPushButton *>(mManaged->focusWidget());
    if (newFocus == btn)
    {
        // this means the window has lost focus so just return and leave as is
        return;
    }

    // turn off the primary and default for the existing button
    if (btn->getButtonType() == Origin::UIToolkit::OriginPushButton::ButtonPrimary)
    {
        btn->setButtonType(Origin::UIToolkit::OriginPushButton::ButtonSecondary);
        btn->setDefault(false);
    }

    // it is not a button so set the current focus off and the default back on
    if (newFocus == NULL)
    {
        mCurrentFocusButton = NULL;        
        if (mDefaultButton)
        {
            mDefaultButton->setButtonType(Origin::UIToolkit::OriginPushButton::ButtonPrimary);
            mDefaultButton->setDefault(true);
        }
    }
    // nothing else because focus in will take care of it
}

void OriginWindowManager::onManagedMaximized()
{
#if defined(Q_OS_WIN)

    Q_ASSERT(mManaged);
    if (mManaged == NULL)
        return;

    if(mFunctionality & Resizable)
    {
        // it does the right thing for everything but the bottom
        // 
        const HWND winId = (HWND)mManaged->winId();
        const int adjustForBar = 1;

        saveCurrentGeometry();

        QRect maxRect = QApplication::desktop()->availableGeometry(mManaged);
        if (isTaskBarHidden(ABE_BOTTOM, winId))
        {
            maxRect.setBottom(maxRect.bottom() - adjustForBar);
        }
        else if (isTaskBarHidden(ABE_TOP, winId)) 
        {
            maxRect.setTop(maxRect.top() + adjustForBar);
        }        
        else if (isTaskBarHidden(ABE_LEFT, winId)) 
        {
            maxRect.setLeft(maxRect.left() + adjustForBar);
        }
        else if (isTaskBarHidden(ABE_RIGHT,winId)) 
        {
            maxRect.setRight(maxRect.right() - adjustForBar);
        }
        // Let the OS know that we are maximized
        mState |= Maximized;
        if (mManaged)
        {
            mManaged->setWindowState(mManaged->windowState() & ~Qt::WindowMinimized | Qt::WindowMaximized | Qt::WindowActive);
            mManaged->setGeometry(maxRect);
        }
        mFunctionality &= ~Movable;
    }
#endif
}

void OriginWindowManager::onManagedMinimized()
{
    saveCurrentGeometry();

    Q_ASSERT(mManaged);
    // Let the OS know that we are minimized
    if(mManaged)
    {
        mManaged->setWindowState(mManaged->windowState() & ~Qt::WindowMaximized | Qt::WindowMinimized | Qt::WindowActive);
        mManaged->showMinimized();
    }
}

void OriginWindowManager::onManagedRestored()
{
    // Let the OS know that we are no longer maximized/minimized
    mState &= ~Maximized;
    Q_ASSERT(mManaged);
    if(mManaged)
    {
        mManaged->setWindowState(Qt::WindowActive);
        if(mState & Zoomed)
            // TODO: Find a way (besides making a property) for doing a default 
            // case of changing the size if this signal isn't hooked up.
            // Also come up with what a default case would be.
            emit customizeZoomedSize();
        // Only set the window to the pre min/max geometry if it has changed from its initial value.
        else if(mPreMinMaxGeometry != QRect(-1,-1,-1,-1))
            mManaged->setGeometry(mPreMinMaxGeometry);
    }
    mFunctionality |= Movable;
}


void OriginWindowManager::onManagedZoomed()
{
    Q_ASSERT(mManaged);
    if (mManaged == NULL)
        return;

    if(mFunctionality & Resizable)
    {
        if((mState & Zoomed) == false)
        {
            // Save our geometry
            mPreZoomGeometry = mManaged->geometry();

            // Let us know that we are zoomed
            mState |= Zoomed;

            // TODO: Find a way (besides making a property) for doing a default 
            // case of changing the size if this signal isn't hooked up.
            // Also come up with what a default case would be.
            emit customizeZoomedSize();
        }
        else // If we are zoomed and the user presses the button again
        {
            // Extra case: If the user presses the zoom button, moves the window, and presses zoom, and zoom again
            // Plus the window should be taking up all of the window available geometry. Move it back there.
            const QRect availableGeo = QApplication::desktop()->availableGeometry(mManaged);
            if(mPreZoomGeometry.width() == availableGeo.width())
                mPreZoomGeometry.moveTo(availableGeo.x(), mPreZoomGeometry.y());
            if(mPreZoomGeometry.height() == availableGeo.height())
                mPreZoomGeometry.moveTo(mPreZoomGeometry.x(), availableGeo.y());

            // restore the geometry to pre-zoomed
            mManaged->setGeometry(mPreZoomGeometry);

            // Remove the zoomed state
            mState &= ~Zoomed;
        }
    }
}


void OriginWindowManager::centerWindow(const int& screenNumber)
{
    Q_ASSERT(mManaged);
    if (mManaged == NULL)
        return;
    QPoint screenCenterPos(0, 0);
    if(screenNumber == -1)
        screenCenterPos = QApplication::desktop()->availableGeometry(mManaged).center();
    else
        screenCenterPos = QApplication::desktop()->availableGeometry(screenNumber).center();
    // For some reason, mManaged->geometry().center() was acting up on the mac. Instead lets do it by size.
    mManaged->move(screenCenterPos - QPoint(mManaged->width()/2, mManaged->height()/2));
}


void OriginWindowManager::ensureOnScreen()
{
    ensureOnScreen(false);
}
        
void OriginWindowManager::ensureOnScreen(bool forceUpdate)
{
    Q_ASSERT(mManaged);
    if (mManaged == NULL || QApplication::desktop() == NULL)
        return;

#ifdef Q_OS_MAC
    const QRect availableGeo = totalRelevantWorkspaceGeometry();
#else
    const QRect availableGeo = QApplication::desktop()->availableGeometry(mManaged);
#endif
    
    // Resize the window to fit on the screen if needed
    QRect resizeSize = mManaged->rect();
    if (resizeSize.width() > availableGeo.width())
    {
        resizeSize.setWidth(availableGeo.width());
    }
    if (resizeSize.height() > availableGeo.height())
    {
        resizeSize.setHeight(availableGeo.height());
    }
    mManaged->resize(resizeSize.width(), resizeSize.height());
    
    QSize widgetSize = mManaged->size();
    QPoint widgetPos = mManaged->pos();
    
    // Passed the top
    if(widgetPos.y() < availableGeo.y())
        widgetPos.setY(availableGeo.y());
    
    // Passed the bottom
    if((widgetPos.y() + widgetSize.height()) > availableGeo.bottom())
        widgetPos.setY(availableGeo.bottom() - widgetSize.height());
    
    // Passed the left
    if(widgetPos.x() < availableGeo.x())
        widgetPos.setX(availableGeo.x());
    
    // Passed the right
    if((widgetPos.x() + widgetSize.width()) > availableGeo.right())
        widgetPos.setX(availableGeo.right() - widgetSize.width());
    
    if (widgetPos != mManaged->pos())
        mManaged->move(widgetPos);
    
    else
    if (forceUpdate)
    {
        // Force move - at least on Mac, after a game captures the display, Qt isn't aware of how the system moves the windows around
        // and will not move the Qt widgets if we pass down the same position.
        // There must be a way to do this better... just don't know it!
        widgetPos.setX(widgetPos.x() + 1);
        widgetPos.setY(widgetPos.y() + 1);
        mManaged->move(widgetPos);

        widgetPos.setX(widgetPos.x() - 1);
        widgetPos.setY(widgetPos.y() - 1);
        mManaged->move(widgetPos);
    }
}


void OriginWindowManager::saveCurrentGeometry()
{
    Q_ASSERT(mManaged);
    if (mManaged == NULL)
        return;

    // The geometry will be invalid if we are maximizing from a minimized state.  This is possible
    // if the user right clicks the taskbar item and selects "Maximize" while Origin is minimized.
    // The same is true of the opposite (going from maximized to minimized).
    if(!mManaged->isMinimized())
    {
#if defined(Q_OS_WIN)
        // On Mac - Qt will tell us that we are maximized even if we aren't. Therefore,
        // The geometry is never saved. EBIBUGS-18873
        if(!mManaged->isMaximized() && !isManagedMaximized())
            mPreMinMaxGeometry = mManaged->geometry();
#else
        // Zoom basically is same as maximized on PC
        if((mState & Zoomed) == false)
            mPreMinMaxGeometry = mManaged->geometry();
#endif
    }
}

QRect OriginWindowManager::totalWorkspaceGeometry()
{
    // Get the size of monitor 0
    QRect total = QApplication::desktop()->availableGeometry(0);
    
    // Add the sizes of other monitors (if attached)
    int screenCount = QApplication::desktop()->screenCount();
    if (screenCount > 1)
    {
        for (int i=1; i<screenCount; i++)
        {
            QRect monitorRect = QApplication::desktop()->availableGeometry(i);
            total = total.united(monitorRect);
        }
    }
    
    return total;
}

#ifdef Q_OS_MAC
QRect OriginWindowManager::totalRelevantWorkspaceGeometry()
{
    
    QRect total;
    
    int screenCount = QApplication::desktop()->screenCount();
    for (int i=0; i<screenCount; i++)
    {
        // Make sure we should take this screen into account
        if (!ToolkitPlatform::IsScreenRelevant(mManaged, i))
            continue;
        
        QRect monitorRect = QApplication::desktop()->availableGeometry(i);
        total = total.united(monitorRect);
    }
    
    return total;
}
#endif
        

bool OriginWindowManager::isTitlebarOnscreen()
{
    if (mTitleBar && mManaged)
    {
        QRect titlebarRect = mTitleBar->geometry();
        QPoint globalXY = mTitleBar->mapToGlobal(QPoint(titlebarRect.x(), titlebarRect.y()));
        titlebarRect.translate(globalXY);
        
        bool overlaps = false;
        
        int screenCount = QApplication::desktop()->screenCount();
        for (int i=0; i<screenCount; i++)
        {
#ifdef Q_OS_MAC
            // Make sure we should take this screen into account
            if (!ToolkitPlatform::IsScreenRelevant(mManaged, i))
                continue;
#endif
            
            QRect monitorRect = QApplication::desktop()->availableGeometry(i);
            if (monitorRect.intersects(titlebarRect))
            {
                overlaps = true;
                break;
            }
        }
        
        return overlaps;
    }
    
    return true;
}
        
void OriginWindowManager::addResizableItem(QWidget* widget)
{
    mResizableObjects.append(widget);
}

bool OriginWindowManager::isManaged(QWidget* widget)
{
    return mResizableObjects.contains(widget);
}
        
}
}
