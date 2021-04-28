///////////////////////////////////////////////////////////////////////////////
// IIGOWindowManager.h
// 
// Created by Frederic Meraud
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef _IIGOWINDOWMANAGER_H_
#define _IIGOWINDOWMANAGER_H_

#include <QMenu>
#include <QWidget>
#include <QGraphicsItem>
#include "EABase/eabase.h"
#include "IIGOCommandController.h"

namespace Origin
{
// Fwd decls
namespace UIToolkit
{
    class OriginWindow;
}

namespace Engine
{
    class IGOQWidget;
    class IViewController;
	class IWebBrowserViewController;
    class ITwitchBroadcastIndicator;

// Interface you should be using (accessible from IGOController) when directly playing with OIG windows
class IIGOWindowManager : public IIGOCommandController
{
public:
    // Defines the specific docking/alignment properties of a window in the desktop
    enum Layout
    {
        WINDOW_ALIGN_NONE = 0x0000,
        
        WINDOW_ALIGN_LEFT = 0x0001,
        WINDOW_ALIGN_RIGHT = 0x0002,
        WINDOW_ALIGN_TOP = 0x0004,
        WINDOW_ALIGN_BOTTOM = 0x0008,
        WINDOW_ALIGN_CENTER_HORZ = 0x0010,
        WINDOW_ALIGN_CENTER_VERT = 0x0020,
        
        WINDOW_FIT_SCREEN_WIDTH = 0x0040,
        WINDOW_FIT_SCREEN_HEIGHT = 0x0080,
    };
    
    enum WindowDockingOrigin
    {
        WindowDockingOrigin_DEFAULT   = 0x0000, // TOP LEFT

        WindowDockingOrigin_X_RIGHT   = 0x0001,
        WindowDockingOrigin_X_CENTER  = 0x0002,

        WindowDockingOrigin_Y_BOTTOM  = 0x0010,
        WindowDockingOrigin_Y_CENTER  = 0x0020,

        WindowDockingOrigin_X_MASK    = 0x000f,
        WindowDockingOrigin_Y_MASK    = 0x00f0,
    };

    enum FrameTransparency
    {
        Transparency,
        NoTransparency
    };
    
#include "IGOAnimationTypes.h" 

    // Base interface used for SDK command callbacks (ie waiting on IGO to actually be visible before
    // opening certain windows to help with activation/focus problems)
    class ICallback
    {
    public:
        virtual ~ICallback();
        virtual void operator() (IIGOWindowManager* igowm) = 0;
    };
    
    // Defines the interface of a QObject that listens to specific IGO context menu events
    class IContextMenuListener
    {
    public:
        virtual ~IContextMenuListener();
        
        virtual void onFocusChanged(QWidget* window, bool hasFocus) = 0;
        virtual void onDeactivate() = 0;
    };

    // Defines the interface of QObject that listens to specific IGO window events
    class IWindowListener
    {
    public:
        virtual ~IWindowListener();
        
        virtual void onFocusChanged(QWidget* window, bool hasFocus) = 0;
    };
    
    // Defines the interface of QObject that listens to specific IGO screen events
    class IScreenListener
    {
    public:
        virtual ~IScreenListener();
        
        virtual void onSizeChanged(uint32_t width, uint32_t height) = 0;
    };
    
    // Defines the set of available properties of a specific window
    class WindowProperties
    {
    public:
        explicit WindowProperties()
            : mDockingOrigin(WindowDockingOrigin_DEFAULT), mOpacity(-1.0f), mWId(0), mCallOrigin(IIGOCommandController::CallOrigin_CLIENT), mRestorable(false)
            , mWIdDefined(false), mPositionDefined(false), mRestorableDefined(false), mHoverOnTransparentDefined(false), mPositionLocked(false)
            , mCallOriginDefined(false)
        {
            memset(&mOpenAnimProps, 0, sizeof(mOpenAnimProps));
            memset(&mCloseAnimProps, 0, sizeof(mCloseAnimProps));
        }
        
        void setWId(int32_t wid) { mWId = wid; mWIdDefined = true; }
        bool widDefined() const { return mWIdDefined; }
        int32_t wid() const { return mWId; }
        
        void setCallOrigin(IIGOCommandController::CallOrigin callOrigin) { mCallOrigin = callOrigin; mCallOriginDefined = true; }
        bool callOriginDefined() const { return mCallOriginDefined; }
        IIGOCommandController::CallOrigin callOrigin() const { return mCallOrigin; }

        void setUniqueID(const QString& uniqueID) { mUniqueID = uniqueID; }
        QString uniqueID() const { return mUniqueID; }

        void setRestorable(bool flag) { mRestorable = flag; mRestorableDefined = true; }
        bool restorableDefined() const { return mRestorableDefined; }
        bool restorable() const { return mRestorable; }

        void setHoverOnTransparent(bool flag) { mHoverOnTransparent = flag; mHoverOnTransparentDefined = true; }
        bool hoverOnTransparentDefined() const { return mHoverOnTransparentDefined; }
        bool hoverOnTransparent() const { return mHoverOnTransparent; }
        
        static QPoint centeredPosition() { return QPoint(INT_MAX, INT_MAX); }
        void setPosition(const QPoint& pos, WindowDockingOrigin docking = WindowDockingOrigin_DEFAULT, bool locked = false) { mPosition = pos; mDockingOrigin = docking; mPositionDefined = true; mPositionLocked = locked; }
        void setCenteredPosition(WindowDockingOrigin docking = WindowDockingOrigin_DEFAULT, bool locked = false) { mPosition = centeredPosition(); mDockingOrigin = docking; mPositionDefined = true; mPositionLocked = locked; }

        bool positionDefined() const { return mPositionDefined; }
        QPoint position() const { return mPosition; }
        WindowDockingOrigin dockingOrigin() const { return mDockingOrigin; }
        bool positionLocked() const { return mPositionLocked; }
        
        static QSize maximumSize() { return QSize(INT_MAX, INT_MAX); }
        void setSize(const QSize& size) { mSize = size; }
        void setMaximumSize() { mSize = maximumSize(); }
        bool sizeDefined() const { return mSize.width() > 0 && mSize.height() > 0; }
        QSize size() const { return mSize; }
        
        void setOpacity(float opacity) { mOpacity = opacity; }
        bool opacityDefined() const { return mOpacity >= 0.0f; }
        float opacity() const { return mOpacity; }

        void setOpenAnimProps(const AnimPropertySet& props) { mOpenAnimProps = props; }
        const AnimPropertySet& openAnimProps() const { return mOpenAnimProps; }
        void setCloseAnimProps(const AnimPropertySet& props) { mCloseAnimProps = props; }
        const AnimPropertySet& closeAnimProps() const { return mCloseAnimProps; }

    private:
        QString mUniqueID;

        QSize mSize;
        QPoint mPosition;
        WindowDockingOrigin mDockingOrigin;

        float mOpacity;
    
        int32_t mWId;
        
        AnimPropertySet mOpenAnimProps;
        AnimPropertySet mCloseAnimProps;

        IIGOCommandController::CallOrigin mCallOrigin;

        bool mRestorable;
        bool mHoverOnTransparent;
        
        bool mWIdDefined;
        bool mPositionDefined;
        bool mRestorableDefined;
        bool mAnimsDefined;
        bool mHoverOnTransparentDefined;
        bool mPositionLocked;
        bool mCallOriginDefined;

        friend class IGOWindowManager;
    };
    
    // Defines the behavior properties used when embedding a new window
    struct WindowBehavior
    {
        explicit WindowBehavior()
        {
            controller = NULL;
            screenListener = NULL;
            windowListener = NULL;
            
            draggingSize = 0;
            nsBorderSize = 0;
            ewBorderSize = 0;
            nsMarginSize = 0;
            ewMarginSize = 0;
            
            transparency = Transparency;
            cacheMode = QGraphicsItem::NoCache;
            
            modal = false;
            allowClose = false;
            allowMoveable = false;
            alwaysVisible = false;
            doNotDeleteOnClose = false;
            closeOnIGOClose = false;
            closeIGOOnClose = false;
            useCustomClose = false;
            useWidgetAsFocusProxy = true;
            dontStealFocus = false;
            alwaysOnTop = false;
            dontblockRenderingWhilePinningInProgress = false;
            deleteOnLosingFocus = false;
            disableDragging = false;
            disableResizing = false;
            supportsContextMenu = false;
            persistent = false;
            pinnable = false;
            singleton = false;
            connectionSpecific = false;
        }
        

        IViewController* controller;
        IWindowListener* windowListener;
        IScreenListener* screenListener;
        
        int draggingSize;
        int nsBorderSize;
        int ewBorderSize;
        int nsMarginSize;
        int ewMarginSize;
        
        FrameTransparency transparency;
        QGraphicsItem::CacheMode cacheMode;
    
        bool modal;                                     // modal window
        bool allowClose;
        bool allowMoveable;
        bool alwaysVisible;
        bool doNotDeleteOnClose;                        // true to bypass the user-specified behavior when calling removeWindow so that we actually don't destroy the container
        bool closeOnIGOClose;                           // true to automatically close the window when IGO is switched off
        bool closeIGOOnClose;                           // true to automatically close IGO when the user closes the window
        bool useCustomClose;
        bool useWidgetAsFocusProxy;
        bool dontStealFocus;                            // don't focus on window when user clicks on it
        bool alwaysOnTop;                               // true if the window is always above all other windows
        bool dontblockRenderingWhilePinningInProgress;  // true if this window will be visible while changing the pinned window transparency
        bool deleteOnLosingFocus;                       // delete the window as soon as it loses focus
        bool disableDragging;                           // true to pin a window to a specific location
        bool disableResizing;                           // true if the user can't resize the window on the desktop
        bool supportsContextMenu;                       // true if we want to properly handle right-clicks to show a context menu
        bool persistent;                                // true if the widget should stick around after all game connections are closed/OIG is completely off
        bool pinnable;                                  // true if the widget can be "pinned" to the game screen/visible even when IGO is switched off
        bool singleton;                                 // true if only one instance of this window id can exist at any one time - the previous window with the same id will be removed automatically
        bool connectionSpecific;                        // true if the window should only show for a specific connection(game)
    };

    // Defines the set of behaviors that are editable AFTER the window creation (ie don't want to revamp the WindowBehavior struct at this point / only for closing SDK windows
    // at this point)
    class WindowEditableBehaviors
    {
    public:
        explicit WindowEditableBehaviors()
            : mCloseIGOOnClose(false), mCloseIGOOnCloseDefined(false)
        {
        }

        void setCloseIGOOnClose(bool flag) { mCloseIGOOnClose = flag; mCloseIGOOnCloseDefined = true; }
        bool closeIGOOnCloseDefined() const { return mCloseIGOOnCloseDefined; }
        bool closeIGOOnClose() const { return mCloseIGOOnClose; }

    private:
        bool mCloseIGOOnClose;
        bool mCloseIGOOnCloseDefined;
    };


    // Defines the interface
public:
    virtual ~IIGOWindowManager();
    
    // Behavior
    virtual void reset() = 0;
    virtual void enableUpdates(bool enabled) = 0;
    
    // Events
    virtual bool registerScreenListener(IScreenListener* listener) = 0;
    
    // Screen
    virtual QSize screenSize() const = 0;
    virtual QSize minimumScreenSize() const = 0;
    virtual QSize maximumWindowSize() const = 0;
    virtual bool isScreenLargeEnough() const = 0;
    virtual QPoint centeredPosition(const QSize& windowSize) const = 0;

    // Desktop
    virtual bool visible() const = 0;
    virtual void clear() = 0;
    
    // Cursor
    virtual void setCursor(int cursor) = 0;

    // Windows
    virtual bool addWindow(Origin::UIToolkit::OriginWindow* window, const WindowProperties& settings) = 0;
    virtual bool addWindow(Origin::UIToolkit::OriginWindow* window, const WindowProperties& settings, const WindowBehavior& behavior) = 0;
    virtual bool addWindow(QWidget* window, const WindowProperties& settings) = 0;
    virtual bool addWindow(QWidget* window, const WindowProperties& settings, const WindowBehavior& behavior) = 0;
    virtual bool addPopupWindow(QWidget* window, const WindowProperties& settings) = 0;
    virtual bool addPopupWindow(QWidget* window, const WindowProperties& settings, const WindowBehavior& behavior) = 0;
    virtual bool removeWindow(QWidget* window, bool autoDelete) = 0;
    
    virtual QWidget* window(int32_t wid) = 0;

    virtual bool setFocusWindow(QWidget* window) = 0;
    virtual bool setWindowFocusWidget(QWidget* window) = 0;
    virtual bool isActiveWindow(QWidget* window) = 0;
    virtual bool activateWindow(QWidget* window) = 0;
    virtual bool enableWindowFrame(QWidget* window, bool flag) = 0;
    virtual bool moveWindowToFront(QWidget* window) = 0;
    virtual bool moveWindowToBack(QWidget* window) = 0;
    virtual bool ensureOnScreen(QWidget* window) = 0;
    
    // Window properties
    virtual bool setWindowProperties(QWidget* window, const WindowProperties& properties) = 0;
    virtual bool windowProperties(QWidget* window, WindowProperties* properties) = 0;
    
    virtual bool setWindowOpacity(QWidget* window, qreal opacity) = 0;
    virtual bool setWindowPosition(QWidget* window, const QPoint& pos, WindowDockingOrigin docking = WindowDockingOrigin_DEFAULT, bool locked = false) = 0;

    virtual bool showWindow(QWidget* window, bool overrideAlwaysVisible = false) = 0;
    virtual bool hideWindow(QWidget* window, bool overrideAlwaysVisible = false) = 0;
    
    virtual bool maximizeWindow(QWidget* window) = 0;
    virtual bool fitWindowToContent(QWidget* window) = 0;
    
    // Window editable behaviors -ie not all of them!
    virtual bool setWindowEditableBehaviors(QWidget* window, const WindowEditableBehaviors& behaviors) = 0;
    virtual bool windowEditableBehaviors(QWidget* window, WindowEditableBehaviors* behaviors) = 0;

    // Window content (I wish we wouldn't need those!)
    virtual bool setWindowItemFlag(QWidget* window, QWidget* item, QGraphicsItem::GraphicsItemFlag flag, bool enabled) = 0;
    virtual bool moveWindowItemToFront(QWidget* window, QGraphicsItem* item) = 0;

    // Context menus
    // TODO: we may be able to remove the InitContextMenu -> do the work in the addContextMenu
    // or maybe we could use a template instead of this set of routines
    virtual bool createContextMenu(QWidget* window, QMenu* menu, IContextMenuListener* listener, bool toolWindow) = 0;
    virtual bool showContextMenu(QWidget* window, QMenu* menu, WindowProperties* settings) = 0;
    virtual bool hideContextMenu(QWidget* window) = 0;
    virtual bool showContextSubMenu(QWidget* window, QMenu* submenu) = 0;
    virtual bool hideContextSubMenu(QWidget* window) = 0;
    
    // Controllers
    virtual int32_t windowID(const IViewController* const controller) = 0;
    virtual IViewController* windowController(QWidget* window) = 0;
    
    // Native
    virtual void* nativeWindow(QWidget* window) = 0;
};

} // Engine
} // Origin

#endif // _IIGOWINDOWMANAGER_H_