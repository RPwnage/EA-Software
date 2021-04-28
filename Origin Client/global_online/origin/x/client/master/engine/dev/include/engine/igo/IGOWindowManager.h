///////////////////////////////////////////////////////////////////////////////
// IGOWindowManager.h
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef _IGOWINDOWMANAGER_H_
#define _IGOWINDOWMANAGER_H_

#include <limits>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QGraphicsItem>
#include <QList>
#include <QMutex>
#include <QScopedPointer>
#include <QTimer>
#include <QVector>
#include <QtWebKitWidgets/QWebView>
#include <QWidget>

#include "EASTL/vector.h"
#include "EASTL/list.h"
#include "EASTL/hash_map.h"
#include "EASTL/map.h"

#include "engine/igo/IIGOCommandController.h"
#include "engine/igo/IIGOWindowManager.h"
#include "engine/login/LoginController.h"
#include "engine/content/Entitlement.h"
#include "services/settings/SettingsManager.h"
#include "services/plugin/PluginAPI.h"


// Private implementation to the Engine module - if you're trying to do something from the Client module, please update the interface instead.
    
    
namespace Origin
{

namespace Engine
{
class IGOQWidget;
class IGOQtContainer;
class IGOWindowManagerIPC;
    
class ORIGIN_PLUGIN_API IGOWindowManager : public QObject, public IIGOWindowManager
{
    Q_OBJECT

public:
    ~IGOWindowManager();
    
    void setIPCServer(IGOWindowManagerIPC* ipcServer);
    
    // Interface impl
    virtual void reset();
    virtual void enableUpdates(bool enabled);

    virtual bool registerScreenListener(IScreenListener* listener);

    virtual QSize screenSize() const;
    virtual QSize minimumScreenSize() const;
    virtual QSize maximumWindowSize() const;
    virtual bool isScreenLargeEnough() const;
    virtual QPoint centeredPosition(const QSize& windowSize) const;
    
    virtual bool visible() const;
    virtual void clear();
    
    virtual bool addWindow(Origin::UIToolkit::OriginWindow* window, const WindowProperties& settings);
    virtual bool addWindow(Origin::UIToolkit::OriginWindow* window, const WindowProperties& settings, const IIGOWindowManager::WindowBehavior& behavior);
    virtual bool addWindow(QWidget* window, const WindowProperties& settings);
    virtual bool addWindow(QWidget* window, const WindowProperties& settings, const WindowBehavior& behavior);
    virtual bool addPopupWindow(QWidget* window, const WindowProperties& settings);
    virtual bool addPopupWindow(QWidget* window, const WindowProperties& settings, const WindowBehavior& behavior);
    virtual bool removeWindow(QWidget* window, bool autoDelete);
    
    virtual QWidget* window(int32_t wid);

    virtual bool setFocusWindow(QWidget* window);
    virtual bool setWindowFocusWidget(QWidget* window);
    virtual bool isActiveWindow(QWidget* window);
    virtual bool activateWindow(QWidget* window);
    virtual bool enableWindowFrame(QWidget* window, bool flag);
    virtual bool moveWindowToFront(QWidget* window);
    virtual bool moveWindowToBack(QWidget* window);
    virtual bool ensureOnScreen(QWidget* window);
    
    virtual bool setWindowProperties(QWidget* window, const WindowProperties& properties);
    virtual bool windowProperties(QWidget* window, WindowProperties* properties);
    virtual bool setWindowOpacity(QWidget* window, qreal opacity);
    virtual bool setWindowPosition(QWidget* window, const QPoint& pos, WindowDockingOrigin docking = WindowDockingOrigin_DEFAULT, bool locked = false);
	virtual bool showWindow(QWidget* window, bool overrideAlwaysVisible = false);
	virtual bool hideWindow(QWidget* window, bool overrideAlwaysVisible = false);
    virtual bool maximizeWindow(QWidget* window);
    virtual bool fitWindowToContent(QWidget* window);

    virtual bool setWindowEditableBehaviors(QWidget* window, const WindowEditableBehaviors& behaviors);
    virtual bool windowEditableBehaviors(QWidget* window, WindowEditableBehaviors* behaviors);

    virtual bool setWindowItemFlag(QWidget* window, QWidget* item, QGraphicsItem::GraphicsItemFlag flag, bool enabled);
    virtual bool moveWindowItemToFront(QWidget* window, QGraphicsItem* item);

    virtual bool createContextMenu(QWidget* window, QMenu* menu, IContextMenuListener* listener, bool toolWindow);
    virtual bool showContextMenu(QWidget* window, QMenu* menu, WindowProperties* settings);
    virtual bool hideContextMenu(QWidget* window);
    virtual bool showContextSubMenu(QWidget* window, QMenu* submenu);
    virtual bool hideContextSubMenu(QWidget* window);
    
    virtual int32_t windowID(const IViewController* const controller);
    virtual IViewController* windowController(QWidget* window);

    virtual void* nativeWindow(QWidget* window);

public slots:
    virtual void setCursor(int cursor);

    // End interface impl
    
public:
    // Turn game-side IGO on/off + use callback when IGO visible actually toggled
    bool setIGOVisible(bool visible, ICallback* callback, IIGOCommandController::CallOrigin callOrigin);


    bool isEmpty() {return mTopLevelWindows.empty(); }

    // window actions
    
    // Internally we have 2 separate lists:
    // - one for the currently visible windows
    // - one for all created windows - this includes windows we may have partially removed (ie removed from the set of
    // visible windows, but the window itself is still around - for example notification popups, or hidden windows)
    // When looking for a window, we look into the currently visible set. If not found, we may decide to lookup the
    // entire set of windows (unhideIfNecessary = true) - if we do this it, we automatically add it
    // back to the current set of visible windows.
    IGOQtContainer *getEmbeddedWindowContainer(const QWidget* widget, bool unhideIfNecessary = false);
    QPoint getMousePos() { return mCurrentMousePos; }

    bool moveToFront(IGOQtContainer* window);
    bool moveToBack(IGOQtContainer* window);

    void dirtyAllWindows();
    void quit();

    QString getActiveProductId() const;
    bool restorePinnedWidget(IGOQtContainer* container);
    void loadPinnedWidget(int32_t wid, const QString& activeProductId, const QString& uniqueID, QString& data);
    void storePinnedWidget(int32_t wid, const QString& activeProductId, const QString& uniqueID, const QString& data);
    void deletePinnedWidget(int32_t wid, const QString& activeProductId, const QString& uniqueID);
    bool setPinningInProgress(bool state) { return mPinningInProgress = state; }
    bool getPinningInProgress() { return mPinningInProgress; }

    bool isGameProcessStillRunning(uint32_t connectionId);

    /// \brief creates profile container
    IGOQtContainer* createContainer(Origin::UIToolkit::OriginWindow* const window, int32_t wid, const WindowBehavior& behavior, const IIGOWindowManager::AnimPropertySet& openAnimProps, const IIGOWindowManager::AnimPropertySet& closeAnimProps);
    
public slots:
    void onGameProcessFinished(uint32_t pid, int exitCode);
    void onGameSpecificWindowClosed(uint32_t connectionId);

    void invalidate();      // re-layout all windows

signals:    // public signals

    void screenSizeChanged(uint32_t screenWidth, uint32_t screenHeight, uint32_t connectionId);
    
    // Emitted by SDK Ebisu-wrappers
    void requestShowIGOPinOnToast(const QString&);
    void requestShowIGOPinOffToast(const QString&);

    // Private signals triggered by IPC-specific API
    void igoConnect(uint id, const QString& gameProcessFolder);
    void igoDisconnect(uint id, bool processIsGone = false);
    void igoStateChange(bool active, bool sdkCall);
    void igoStart(uint id);
    void igoStop();
    void igoVisible(bool visible, bool emitStateChange);
    void igoShowChatWindow();
    void igoUpdateActiveGameData(uint connID);
    void igoResetAnimations();
    void igoScreenResize(uint32_t connectionId, uint32_t width, uint32_t height, bool forceUpdate);

    // pins
    void igoTogglePins(bool state);
    void igoUnpinAll(const IGOQtContainer *keepPinnedWidget);

    //////////////////////////////////////////////////////////////////////////
    /// \brief Close all the top windows in the IGO that are marked to close on closing the IGO.
    void closeWindowsOnIgoClose();

	/// \brief signal to the game that the user tried to open
	void igoUserIgoOpenAttemptIgnored();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

private:
    // IPC-dedicated API
#if defined(ORIGIN_MAC)
    void injectKeyboardEvent(const char* data, int32_t size);
    void injectInputEvent(uint32_t type, int32_t param1, int32_t param2, int32_t param3, int32_t param4, float param5, float param6, uint64_t param7);
#else
    void injectInputEvent(uint32_t type, int32_t param1, int32_t param2, int32_t param3, int32_t param4, float param5, float param6);
#endif

    void injectIGOStart(uint id);
    void injectIGOStop();
    void injectIGOVisible(bool visible, bool emitStateChange);
    void injectIgoConnect(uint id, const QString& gameProcessFolder);
    void injectIgoDisconnect(uint id, bool processIsGone);

    
    void processTick(bool alwaysProcessMouseEvent);

    void updateMouseButtons();

    IGOQtContainer* findWindow(uint32_t windowId) const;
    IGOQtContainer* findWindowWithMouse(QPoint &pt) const;
    IGOQtContainer* findModalWindow() const;
    const QList<IGOQtContainer const*> pinnedWindows();
    void updatePinnedWindowCount();

    void showStartupWindows();
    void closeGameWindows(uint32_t connectionId);
    void closeAllWindows(bool preserveChatWindows = false);

    WindowBehavior validateWindowBehavior(const WindowProperties& properties, const WindowBehavior& behavior);
    IGOQtContainer *createQtContainer(QWidget *widget, int32_t wid, const WindowBehavior& settings, const IIGOWindowManager::AnimPropertySet& openAnimProps, const IIGOWindowManager::AnimPropertySet& closeAnimProps);
    void centerContainer(IGOQtContainer *container, int yOffset = 0);
    void maximizeContainer(IGOQtContainer *container);
    uint32_t windowIndex(IGOQtContainer *container);

    typedef eastl::vector<IGOQtContainer *> tWindowList;
    tWindowList mAllWindows;
    tWindowList mTopLevelWindows;
    QString mProductId;
    
    QPoint mCurrentMousePos;
    QPoint mCurrentMousePosOffset;
    QPointF mCurrentMousePosScaling;
    QPoint mLastMousePos;
#ifdef ORIGIN_MAC
    uint64_t mCurrentMouseTimestamp;
#endif
    signed int mMouseWheelDeltaH;
    signed int mMouseWheelDeltaV;
    Qt::MouseButtons mCurrentMouseButtonState;
    Qt::MouseButtons mLastMouseButtonState;
    bool mDoubleClick;
    
    IGOQtContainer* volatile mWindowHasMouse;   // this guy needs to be volatile for when triggering recursive before from sgnals,
                                                // which may remove the object we're working with (ie in processTick)
    IGOQtContainer* mWindowHasFocus;
    uint mCurrentKeyWParam;
    uint mCurrentKeyLParam;
    uint mCurrentKeyEventType;

#if defined(ORIGIN_MAC)
    QByteArray mCurrentKeyData;
#endif

    bool mDirty;
    bool mPinningInProgress;
    bool mIGOVisible;
    uint mCursor;

    uint32_t mScreenWidth;
    uint32_t mScreenHeight;
    uint32_t mPrevScreenWidth;
    uint32_t mPrevScreenHeight;
    uint32_t mMinIGOScreenWidth;
    uint32_t mMinIGOScreenHeight;

    int mWindowOffset;

    IIGOCommandController::CallOrigin mCallOrigin;

    // IPC
    IGOWindowManagerIPC *mIPC;
#if defined(ORIGIN_PC)
    // IPC server alive check
    QTimer mIPCServerCheckTimer;
#endif

    // Notification
    bool mShowPinCanToggleToast;
    bool mShowPinHiddenToast;

    mutable QMutex* mTopLevelWindowsMutex;
    mutable QMutex* mIPCMutex;

    // Settings & Game Info
    QString mUsername;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// INTERNAL SIGNALS
signals:
    // TODO: REMOVE
    void requestGoOnline();
    void layoutWindows(uint screenWidth, uint screenHeight);
#ifdef ORIGIN_MAC
    void inputEvent(uint type, int param1, int param2, int param3, int param4, float param5, float param6, uint64_t param7);
#else
    void inputEvent(uint type, int param1, int param2, int param3, int param4, float param5, float param6);
#endif

    void keyboardEvent(QByteArray data); // MAC OSX ONLY

    void requestIGOVisibleCallbacksProcessing();    // emitted to trigger the processing of callbacks associated with setIGOVisible (mainly called from SDK commands)
    
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
// INTERNAL SLOTS
private slots:
    
#if defined(ORIGIN_PC)
    // IPC server alive slot
    void checkIPCServer();
#endif

    // window event updates
#ifdef ORIGIN_MAC
    void updateInputEvent(uint type, int param1, int param2, int param3, int param4, float param5, float param6, uint64_t param7);
#else
    void updateInputEvent(uint type, int param1, int param2, int param3, int param4, float param5, float param6);
#endif
    void updateKeyboardEvent(QByteArray data); // MAC OSX ONLY

    // window rendering
    void layoutAllWindows(uint screenWidth, uint screenHeight);
    void layoutWindow(IGOQtContainer *window, bool isPinningInProgress);
    // cursor
    void setGameCursor(uint cursor);
    uint getGameCursor() { return mCursor; }
    // modal state
    void updateWindow(IGOQtContainer *);
    // pinning state
    void updatePinningState(bool inPinningInProgress);
    void pinningToggled(IGOQtContainer *widget);

    void igoSetVisible(bool visible, bool emitStateChange);
    void igoScreenResized(uint32_t connectionId, uint32_t screenWidth, uint32_t screenHeight, bool forceUpdate);
    void igoCloseWindowsOnIgoClose();
    void showBrowserForGame();
    void processIGOVisibleCallbacks();  // process pending callbacks from setIGOVisible (mainly used to handle SDK commands)
    

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // commands
public slots:
    bool igoCommand(int cmd, Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
    void igoStarted(uint id);
    void igoAddWindow(IGOQtContainer *window);
    void igoConnected(uint id, const QString& gameProcessFolder);
    void igoDisconnected(uint id, bool processIsGone);

    void ensureOnScreen();

    // multiple games support function
    void igoUpdatedActiveGameData(uint connectionId);

    // pinnable widgets toggle
    void igoToggledPins(bool state);
    void igoUnpinnedAll(const IGOQtContainer *keepPinnedWidget);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

public:
    void addContextMenu(QWidget *widget, int x, int y);
    void removeContextMenu();

    // Utility API

    void setWindowFocus(IGOQtContainer* window, bool update=true, bool forcedRefresh=false);
    void updateWindowFocus();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

private:
    IGOWindowManager();
    IGOWindowManager(const IGOWindowManager&);
    IGOWindowManager& operator=(const IGOWindowManager&);
    

    void addWindow(IGOQtContainer* window);
    void removeWindow(IGOQtContainer* window, bool deleteWindow);
    void removeWindowAndContent(IGOQtContainer* window);
    void ensureOnScreen(IGOQtContainer* container);
    IGOQtContainer* addPopupWindowImpl(QWidget* widget, const WindowProperties& settings, const WindowBehavior& behavior);
    IGOQtContainer* windowContainer(int32_t wid, bool unhideIfNecessary = false);

    void centerWindow(IGOQtContainer* window, const QPoint& offset);
    QPoint nextWindowPositionAvailable(); // returns the next position the user can use to initially position his window
    void updatePinnedWindows(bool resolutionChange);

    
    typedef eastl::vector<ICallback*> IGOVisibleCallbacks;
    IGOVisibleCallbacks mIGOVisibleCallbacks;
    
    struct IGOGameInfo // Info per game internal to the window manager
    {
        explicit IGOGameInfo()
        {
			mIsValid = false;

            screenLargeEnough = -1;
            maxPinnedWidgets = 0;
        }
        
		bool isValid() const { return mIsValid; }

        int32_t screenLargeEnough;
        uint16_t maxPinnedWidgets;
        
		bool mIsValid;
    };
    typedef eastl::hash_map<uint32_t, IGOGameInfo> IGOGameInstances;
    IGOGameInstances mGames;

	void updateGameInfo(uint32_t gameId, const IGOGameInfo& info);
	IGOGameInfo gameInfo(uint32_t gameId) const;

    QRect mReservedHeights; // reserved spaces for docked items (ie bottom/top bar)
    
    bool mEnableUpdates;
    
    friend class IGOController;
    friend class IGOWindowManagerIPC;
    friend class IGOQtContainer;
};

} // Engine
} // Origin

#endif // _IGOWINDOWMANAGER_H_
