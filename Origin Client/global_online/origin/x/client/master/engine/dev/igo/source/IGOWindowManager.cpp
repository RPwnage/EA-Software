// *********************************************************************
//  IGOWindowManager.cpp
//  Copyright (c) 2012 Electronic Arts Inc. All rights reserved.
// **********************************************************************

#include <limits>
#include <QDateTime>
#include <QMenu>
#include <QVBoxLayout>
#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsProxyWidget>

#include "IGO/IGO.h"
#include "IGOWindowManager.h"
#include "IGOQtContainer.h"
#include "IGOIPC/IGOIPC.h"
#include "IGOWindowManagerIPC.h"
#include "IGOWinHelpers.h"
#include "IGOMacHelpers.h"
#include "IGOGameTracker.h"

#include "EASTL/functional.h"
#include "EASTL/shared_ptr.h"

#include "engine/igo/IGOController.h"
#include "engine/content/ContentController.h"
#include "engine/login/LoginController.h"
#include "engine/utilities/ContentUtils.h"

#include "TelemetryAPIDLL.h"
#include "Qt/originwindow.h"
#include "Qt/originwebview.h"

#include "services/session/SessionService.h"
#include "services/settings/InGameHotKey.h"
#include "services/settings/SettingsManager.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

#if defined(ORIGIN_MAC)
#include <dlfcn.h>
#endif


namespace Origin
{
namespace Engine
{

// This is defined in EbisuError.h in the SDK.
// We define it here to prevent sdk/client cross requirements
#define EBISU_SUCCESS 0
#define EBISU_ERROR_GENERIC -1

#if defined(ORIGIN_PC)

// detect concurrencies - assert if our mutex is already locked
#ifdef _DEBUG_MUTEXES
#define DEBUG_MUTEX_TEST(x){\
bool s=x->tryLock();\
if(s)\
x->unlock();\
ORIGIN_ASSERT(s==true);\
}
#else
#define DEBUG_MUTEX_TEST(x) void(0)
#endif

#elif defined (ORIGIN_MAC)
    
#define DEBUG_MUTEX_TEST(x) void(0)
    
#endif // !MAC OSX

    
static const int WINDOW_INITIAL_MIN_Y = 10;
static const int WINDOW_AUTO_POSITON_START = 50;
static const int WINDOW_AUTO_POSITION_MAX = 150;
    
#ifdef ORIGIN_MAC
static const int WINDOW_AUTO_POSITION_INC = 25;
#else
static const int WINDOW_AUTO_POSITION_INC = 50;
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline void SendWindowInitialState(IGOQtContainer* window, int32_t index, IGOWindowManagerIPC* ipc)
{
    uint32_t connectionId = ipc->getCurrentConnectionId();
    if (connectionId && !window->initialStateBroadcast(connectionId))
    {
        window->setInitialStateBroadcast(connectionId);

        // Do we need to send a create window message first?
        if (index >= 0)
        {
            eastl::shared_ptr<IGOIPCMessage> msg(IGOIPC::instance()->createMsgNewWindow(window->id(), index));
            if(msg.get())
                ipc->send(msg);
        }

        if (window->useAnimations())
        {
            eastl::shared_ptr<IGOIPCMessage> openMsg(IGOIPC::instance()->createMsgWindowSetAnimation(window->id(), IIGOWindowManager::AnimPropertySet::Version, IIGOWindowManager::WindowAnimContext_OPEN, reinterpret_cast<const char*>(&window->openAnimProps()), sizeof(IIGOWindowManager::AnimPropertySet)));
            if (openMsg.get())
                ipc->send(openMsg);

            eastl::shared_ptr<IGOIPCMessage> closeMsg(IGOIPC::instance()->createMsgWindowSetAnimation(window->id(), IIGOWindowManager::AnimPropertySet::Version, IIGOWindowManager::WindowAnimContext_CLOSE, reinterpret_cast<const char*>(&window->closeAnimProps()), sizeof(IIGOWindowManager::AnimPropertySet)));
            if (closeMsg.get())
                ipc->send(closeMsg);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IGOWindowManager::IGOWindowManager():
mCurrentMousePos(0, 0),
mCurrentMousePosOffset(0, 0),
mCurrentMousePosScaling(0, 0),
mLastMousePos(0, 0),
#ifdef ORIGIN_MAC
mCurrentMouseTimestamp(0),
#endif
mMouseWheelDeltaH(0),
mMouseWheelDeltaV(0),
mCurrentMouseButtonState(Qt::NoButton),
mLastMouseButtonState(Qt::NoButton),
mDoubleClick(false),
mWindowHasMouse(NULL),
mWindowHasFocus(NULL),
mCurrentKeyWParam(0),
mCurrentKeyLParam(0),
mCurrentKeyEventType(0),
mDirty(true),
mPinningInProgress(false),
mIGOVisible(false),
mCallOrigin(IIGOCommandController::CallOrigin_CLIENT),
mCursor((uint)IGOIPC::CURSOR_ARROW),
mScreenWidth(0),
mScreenHeight(0),
mPrevScreenWidth(0),
mPrevScreenHeight(0),
mMinIGOScreenWidth(OriginIGO::MIN_IGO_DISPLAY_WIDTH),
mMinIGOScreenHeight(OriginIGO::MIN_IGO_DISPLAY_HEIGHT),
mWindowOffset(WINDOW_AUTO_POSITON_START),
mIPC(NULL),
mShowPinCanToggleToast(false),
mShowPinHiddenToast(false),
mReservedHeights(0, 0, 0, 0),
mEnableUpdates(false)
{
    mTopLevelWindowsMutex = new QMutex(QMutex::Recursive);
    mIPCMutex = new QMutex(QMutex::Recursive);

    // Do we have an override for the min IGO resolution?
    QString minRes = Services::readSetting(Services::SETTING_ForceMinIGOResolution.name());
    if (!minRes.isEmpty())
    {
        bool validOverride = false;
        QStringList dims = minRes.split('x', QString::SplitBehavior::SkipEmptyParts, Qt::CaseInsensitive);
        if (dims.size() == 2)
        {
            int width = dims[0].toInt();
            int height = dims[1].toInt();
            if (width > 0 && height > 0)
            {
                validOverride = true;
                mMinIGOScreenWidth = width;
                mMinIGOScreenHeight = height;

                ORIGIN_LOG_EVENT << "Using min IGO resolution override: " << minRes;
            }
        }

        if (!validOverride)
            ORIGIN_LOG_ERROR << "Found invalid min IGO resolution override: " << minRes;
    }

    // the reason ipc sends signals instead of calling directly is because it's running 
    // in another thread.  processing should be done in the main thread.
#ifdef ORIGIN_MAC
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(inputEvent(uint, int, int, int, int, float, float, uint64_t)), this, SLOT(updateInputEvent(uint, int, int, int, int, float, float, uint64_t)));
#else
    ORIGIN_VERIFY_CONNECT_EX(this, SIGNAL(inputEvent(uint, int, int, int, int, float, float)), this, SLOT(updateInputEvent(uint, int, int, int, int, float, float)), Qt::QueuedConnection);
#endif
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(keyboardEvent(QByteArray)), this, SLOT(updateKeyboardEvent(QByteArray)));
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(layoutWindows(uint, uint)), this, SLOT(layoutAllWindows(uint, uint)));
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(igoStart(uint)), this, SLOT(igoStarted(uint)));
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(igoVisible(bool, bool)), this, SLOT(igoSetVisible(bool, bool)));
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(igoScreenResize(uint32_t, uint32_t, uint32_t, bool)), this, SLOT(igoScreenResized(uint32_t, uint32_t, uint32_t, bool)));
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(igoConnect(uint, const QString&)), this, SLOT(igoConnected(uint, const QString&)));
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(igoDisconnect(uint, bool)), this, SLOT(igoDisconnected(uint, bool)));
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(igoUpdateActiveGameData(uint)), this, SLOT(igoUpdatedActiveGameData(uint)));
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(closeWindowsOnIgoClose()), this, SLOT(igoCloseWindowsOnIgoClose()));
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(igoTogglePins(bool)), this, SLOT(igoToggledPins(bool)));
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(igoUnpinAll(const IGOQtContainer *)), this, SLOT(igoUnpinnedAll(const IGOQtContainer *)));

    ORIGIN_VERIFY_CONNECT(this, SIGNAL(requestIGOVisibleCallbacksProcessing()), this, SLOT(processIGOVisibleCallbacks()));

#if defined(ORIGIN_PC)
    ORIGIN_VERIFY_CONNECT(&mIPCServerCheckTimer, SIGNAL(timeout()), this, SLOT(checkIPCServer()));
    mIPCServerCheckTimer.start(5000);
#endif

    QCoreApplication::addLibraryPath("./plugins");
}

void IGOWindowManager::setIPCServer(IGOWindowManagerIPC* ipcServer)
{
    DEBUG_MUTEX_TEST(mIPCMutex); QMutexLocker ipcLocker(mIPCMutex);
    mIPC = ipcServer;
}

IGOWindowManager::~IGOWindowManager()
{
#if defined(ORIGIN_PC)
    mIPCServerCheckTimer.stop();
#endif

    if(!mAllWindows.empty())
    {
        int numWindows = static_cast<int>(mAllWindows.size());
        ORIGIN_LOG_EVENT << "Deleting " << numWindows << " IGO windows.";

        // clear all other windows - make sure to copy the list of windows since we're going to update the mAllWindows ref list
        tWindowList windows(mAllWindows.begin(), mAllWindows.end());
        for (tWindowList::const_iterator iter = windows.begin(); iter != windows.end(); ++iter)
        {
            IGOQtContainer* container = (*iter);
            if (container)
            {
                ORIGIN_LOG_EVENT << "Successfully deleted " << qPrintable(container->objectName())  << " IGO window.";
                
                if (container->viewController())
                {
                    QObject* obj = dynamic_cast<QObject*>(container->viewController());
                    if (obj)
                        obj->deleteLater();
                }
                
                container->destroyWidget();
            }
        }
        ORIGIN_LOG_EVENT << "Successfully deleted " << numWindows << " IGO windows.";
    }
    mAllWindows.clear();
    mTopLevelWindows.clear();

    quit();

    ORIGIN_LOG_EVENT << "Destroying IGO mutexes.";

    if (mTopLevelWindowsMutex)
        delete mTopLevelWindowsMutex;
    mTopLevelWindowsMutex = NULL;
    
    if (mIPCMutex)
        delete mIPCMutex;
    mIPCMutex = NULL;

    ORIGIN_LOG_EVENT << "Successfully destroyed IGO mutexes.";
}

void IGOWindowManager::layoutAllWindows(uint screenWidth, uint screenHeight)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    DEBUG_MUTEX_TEST(mIPCMutex); QMutexLocker ipcLocker(mIPCMutex);
    IGOIPC *ipc = IGOIPC::instance();

    
    // disconnect all windows, so that we do not get accidential uppdates
    for (tWindowList::const_iterator iter = mTopLevelWindows.begin(); iter != mTopLevelWindows.end(); ++iter)
    {
        ORIGIN_VERIFY_DISCONNECT((*iter), SIGNAL(windowDirty(IGOQtContainer *)), this, SLOT(updateWindow(IGOQtContainer *)));
    }

    // tell the IGOApplication which windows are still valid/provide current ordering
    eastl::vector<uint32_t> ids;
    for (tWindowList::const_iterator iter = mTopLevelWindows.begin(); iter != mTopLevelWindows.end(); ++iter)
        ids.push_back((*iter)->id());
    
    bool pinningStatus = getPinningInProgress();

    // set IGO's pinning state
    {
        eastl::shared_ptr<IGOIPCMessage> msg(ipc->createMsgIsPinning(pinningStatus));
        if (mIPC && msg.get())
            mIPC->send(msg);
    }

    eastl::shared_ptr<IGOIPCMessage> msg (ipc->createMsgResetWindows(ids));
    if(mIPC && msg.get())
        mIPC->send(msg);
    
    // Send the necessary animation data
    if (mIPC)
    {
        for (tWindowList::const_iterator iter = mTopLevelWindows.begin(); iter != mTopLevelWindows.end(); ++iter)
            SendWindowInitialState(*iter, -1, mIPC);
    }

    // re-connect all windows
    for (tWindowList::const_iterator iter = mTopLevelWindows.begin(); iter != mTopLevelWindows.end(); ++iter)
    {
        ORIGIN_VERIFY_CONNECT_EX((*iter), SIGNAL(windowDirty(IGOQtContainer *)), this, SLOT(updateWindow(IGOQtContainer *)), Qt::QueuedConnection);
    }
    
    // redraw all windows
    for (tWindowList::const_iterator iter = mTopLevelWindows.begin(); iter != mTopLevelWindows.end(); ++iter)
    {
        // re-layout    
        layoutWindow(*iter, pinningStatus);
        (*iter)->invalidate();

        // just making sure that we're going to send the image data (for example right now we would get a windowDirty() signal for the IGONavigation control - not sure why!!!)
        QList<QRectF> regions;
        (*iter)->repaintRequestedSlot(regions);
    }

    mPrevScreenWidth = mScreenWidth;
    mPrevScreenHeight = mScreenHeight;
}

void IGOWindowManager::layoutWindow(IGOQtContainer *window, bool isPinningInProgress)
{
    if (!isPinningInProgress)   // do not re-layout windows, while we are pinning one of them....
    {
        uint layout = window->layout();

        if( layout&WINDOW_FIT_SCREEN_WIDTH && layout&WINDOW_FIT_SCREEN_HEIGHT )
            window->resize(mScreenWidth, mScreenHeight, true);
        else
        {
            if( layout&WINDOW_FIT_SCREEN_WIDTH )
                window->resize(mScreenWidth, 0, true);

            if( layout&WINDOW_FIT_SCREEN_HEIGHT )
                window->resize(0, mScreenHeight, true);
        }

        if( layout&WINDOW_ALIGN_TOP )
            window->setY(0);

        if( layout&WINDOW_ALIGN_BOTTOM )
            window->setY(mScreenHeight - window->height());

        if( layout&WINDOW_ALIGN_RIGHT )
            window->setX(mScreenWidth - window->width());
            
        if( layout&WINDOW_ALIGN_LEFT )
            window->setX(0);

        if( layout&WINDOW_ALIGN_CENTER_HORZ )
            window->setX( 0.5*(mScreenWidth - window->width()) );

        if( layout&WINDOW_ALIGN_CENTER_VERT )
            window->setY( 0.5*(mScreenHeight - window->height()) );

        // New to the game, we support the position "docking" preferences (that get discarded as soon as we move a window)
        window->layoutDockingPosition(mScreenWidth, mScreenHeight);

        // make windows visible in viewport
        if (layout == 0)
        {
            // Only try to be smart when we actually know the screen resolution
            if (isScreenLargeEnough())
            {
                // If triggered from an actually screen resolution change, ensure the windows are on screen when the user reduces the screen resolution
                // (no change when the user increases the screen resolution)
                if (mScreenWidth < mPrevScreenWidth || mScreenHeight < mPrevScreenHeight)
                {
                    if (IGOController::instance()->extraLogging())
                    {
                        QRect rect = window->rect(false);
                        ORIGIN_LOG_ACTION << "IGOWindowManager - ensureOnScreen layoutWindow=" << window << ", rect=(" << rect.x() << "," << rect.y() << "," << rect.width() << "," << rect.height() <<"), prevScreen=(" << mPrevScreenWidth << "," << mPrevScreenHeight << "), screen=(" << mScreenWidth << "," << mScreenHeight << ")";
                    }

                    ensureOnScreen(window);
                }
                
                else
                {
                    QRect rect = window->rect(true);
                    if (!rect.isEmpty())
                    {
                        const int TITLE_BAR_HEIGHT = 25;

                        // EBIBUGS-20163 reset window location if the window titlebar is entirely outside the game window
                        if (rect.x() > (int)mScreenWidth || rect.y() > (int)mScreenHeight ||
                            rect.x() + rect.width() <= 0 || rect.y() + TITLE_BAR_HEIGHT <= 0)
                        {
                            QPoint pos = nextWindowPositionAvailable();
                            
                            if (IGOController::instance()->extraLogging())
                                ORIGIN_LOG_ACTION << "IGOWindowManager - reset location layoutWindow=" << window << ", rect=(" << rect.x() << "," << rect.y() << "," << rect.width() << "," << rect.height() << "), screen=(" << mScreenWidth << "," << mScreenHeight << ", newPos=(" << pos.x() << "," << pos.y() << ")";

                            window->setX(pos.x());
                            window->setY(pos.y());
                        }
                    }
                }
            }
        }
    }
}

#ifdef ORIGIN_PC

void IGOWindowManager::checkIPCServer()
{
    DEBUG_MUTEX_TEST(mIPCMutex); QMutexLocker ipcLocker(mIPCMutex);

    if (mIPC && !mIPC->isStarted())
    {
        if (!mIPC->restart(false))
        {
            ORIGIN_LOG_ERROR << "Restarting IPC failed.";
        }
    }
}

#endif


#ifdef ORIGIN_MAC
void IGOWindowManager::updateInputEvent(uint type, int param1, int param2, int param3, int param4, float param5, float param6, uint64_t param7)
#else
void IGOWindowManager::updateInputEvent(uint type, int param1, int param2, int param3, int param4, float param5, float param6)
#endif
{
    mDirty = true;
    
    bool processMouseEvent = false;
    
    switch(type)
    {
    case IGOIPC::EVENT_MOUSE_LEFT_DOWN:
        mCurrentMouseButtonState = mCurrentMouseButtonState | Qt::LeftButton;
        break;
    case IGOIPC::EVENT_MOUSE_LEFT_UP:
        mCurrentMouseButtonState = mCurrentMouseButtonState &~ Qt::LeftButton;
        break;
    case IGOIPC::EVENT_MOUSE_RIGHT_DOWN:
        mCurrentMouseButtonState = mCurrentMouseButtonState | Qt::RightButton;
        break;
    case IGOIPC::EVENT_MOUSE_RIGHT_UP:
        mCurrentMouseButtonState = mCurrentMouseButtonState &~ Qt::RightButton;
        break;
    case IGOIPC::EVENT_MOUSE_LEFT_DOUBLE_CLICK:
        // We still need to know the button is down
        mCurrentMouseButtonState = mCurrentMouseButtonState | Qt::LeftButton;
        mDoubleClick = true;
#if defined(ORIGIN_PC)
        // handle double click state
        processTick(processMouseEvent);
        // send an artificial button up event for PC only, otherwise Qt thinks we hold down the mouse button
        mCurrentMouseButtonState = mCurrentMouseButtonState &~ Qt::LeftButton;
#endif
        break;
    case IGOIPC::EVENT_MOUSE_RIGHT_DOUBLE_CLICK:
        // We still need to know the button is down
        mCurrentMouseButtonState = mCurrentMouseButtonState | Qt::RightButton;
        mDoubleClick = true;
#if defined(ORIGIN_PC)
        // handle double click state
        processTick(processMouseEvent);
        // send an artificial button up event for PC only, otherwise Qt thinks we hold down the mouse button
        mCurrentMouseButtonState = mCurrentMouseButtonState &~ Qt::RightButton;
#endif
    break;

    case IGOIPC::EVENT_MOUSE_MOVE:
        mCurrentMousePos.setX(param1); 
        mCurrentMousePos.setY(param2);

#if !defined(ORIGIN_MAC)

        mCurrentMousePosOffset.setX(LOWORD(param4)); 
        mCurrentMousePosOffset.setY(HIWORD(param4));

#else

        mCurrentMousePosOffset.setX(param3); 
        mCurrentMousePosOffset.setY(param4);

#endif

        mCurrentMousePosScaling.setX(param5); 
        mCurrentMousePosScaling.setY(param6);

#ifdef ORIGIN_MAC
        mCurrentMouseTimestamp = param7;
        processMouseEvent = true; // force the processing of mouse move events because of our filtering when resizing/moving
#endif
        break;

    case IGOIPC::EVENT_MOUSE_WHEEL:
        mCurrentMousePos.setX(param1); 
        mCurrentMousePos.setY(param2);
        mMouseWheelDeltaV  = param3;
#ifdef ORIGIN_MAC
        mMouseWheelDeltaH = param4;
#endif
        break;

#if !defined(ORIGIN_MAC)

    case IGOIPC::EVENT_KEYBOARD:
        mCurrentKeyEventType = param1;
        mCurrentKeyWParam = param2;
        mCurrentKeyLParam = param3;
        break;

#endif

    }

    // handle the state
    processTick(processMouseEvent);
}

void IGOWindowManager::updateKeyboardEvent(QByteArray data)
{
#if defined(ORIGIN_MAC)

    mDirty = true;
    
    mCurrentKeyWParam = 1;
    mCurrentKeyData = data;
    
    processTick(false);

#endif
}

// Internally we have 2 separate lists:
// - one for the currently visible windows
// - one for all created windows - this includes windows we may have partially removed (ie removed from the set of
// visible windows, but the window itself is still around - for example notification popups, or hidden windows)
// When looking for a window, we look into the currently visible set. If not found, we may decide to lookup the
// entire set of windows (unhideIfNecessary = true) - if we do this it, we automatically add it
// back to the current set of visible windows.
IGOQtContainer* IGOWindowManager::getEmbeddedWindowContainer(const QWidget* widget, bool unhideIfNecessary)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    tWindowList::const_iterator iter = mTopLevelWindows.begin();
    while (iter != mTopLevelWindows.end())
    {
        IGOQtContainer* container = (*iter);
        IGOQWidget* containerWidget = container->widget();
        
        if (!container->pendingDeletion() && containerWidget)
        {
            // Normal case: the window was directly an IGOQWidget (top/bottom bars) or is an embedded QWidget (UIToolkit::Window) ?
            if (containerWidget == widget || containerWidget->getRealParent() == widget)
                return container;
        
            // K, maybe we are passing a child of the embedded QWidget (ie call from context menu) ?
            if (containerWidget->isAncestorOf(widget))
                return container;
        }
        
        ++iter;
    }
    
    if (unhideIfNecessary)
    {
        iter = mAllWindows.begin();
        while (iter != mAllWindows.end())
        {
            IGOQtContainer* container = (*iter);
            IGOQWidget* containerWidget = container->widget();
            
            if (containerWidget && (containerWidget == widget || containerWidget->getRealParent() == widget))
            {
                addWindow(container);
                return container;
            }
            
            ++iter;
        }
    }
    
    return NULL;
}

bool IGOWindowManager::moveToFront(IGOQtContainer* window)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    DEBUG_MUTEX_TEST(mIPCMutex); QMutexLocker ipcLocker(mIPCMutex);
    
    if (!mTopLevelWindows.empty())
    {
        if (mTopLevelWindows.back() == window)
            return false;
    }
    
    size_t index = 0;
    tWindowList::iterator iter = eastl::find(mTopLevelWindows.begin(), mTopLevelWindows.end(), window);
    if (iter != mTopLevelWindows.end() && window->widget()->canBringToFront())
    {
        mTopLevelWindows.erase(iter);

        if (window->alwaysOnTop())
        {
            // Just put the window at the top
            index = mTopLevelWindows.size();
            mTopLevelWindows.push_back(window);
        }
        
        else
        {
            // We need to insert AFTER all the not-'alwaysOnTop' windows
            tWindowList::iterator topIter = mTopLevelWindows.begin();
            for (; topIter != mTopLevelWindows.end(); ++topIter)
            {
                IGOQtContainer* topWindow = *topIter;
                if (topWindow->alwaysOnTop())
                    break;
                
                ++index;
            }
            
            mTopLevelWindows.insert(topIter, window);
        }

        eastl::shared_ptr<IGOIPCMessage> msg(IGOIPC::instance()->createMsgMoveWindow(window->id(), index));
        if(msg.get())
            mIPC->send(msg);
    }
    
    return true;
}

bool IGOWindowManager::moveToBack(IGOQtContainer* window)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    DEBUG_MUTEX_TEST(mIPCMutex); QMutexLocker ipcLocker(mIPCMutex);
    
    if (!mTopLevelWindows.empty())
    {
        if (mTopLevelWindows.front() == window)
            return false;
    }

    size_t index = 0;
    tWindowList::iterator iter = eastl::find(mTopLevelWindows.begin(), mTopLevelWindows.end(), window);
    if (iter != mTopLevelWindows.end())
    {
        mTopLevelWindows.erase(iter);
        
        if (window->alwaysOnTop())
        {
            // Put the window behind all the 'alwaysOnTop' windows
            tWindowList::iterator topIter = mTopLevelWindows.begin();
            for (; topIter != mTopLevelWindows.end(); ++topIter)
            {
                IGOQtContainer* topWindow = *topIter;
                if (topWindow->alwaysOnTop())
                    break;
                
                ++index;
            }
            
            mTopLevelWindows.insert(topIter, window);
        }
        
        else
        {
            // Just put the window in the back
            mTopLevelWindows.insert(mTopLevelWindows.begin(), window);
        }
        
        eastl::shared_ptr<IGOIPCMessage> msg (IGOIPC::instance()->createMsgMoveWindow(window->id(), index));
        if(msg.get())
            mIPC->send(msg);
    }
    
    return true;
}

void IGOWindowManager::updatePinnedWindowCount()
{
    // update the max. pinned widgets count
    DEBUG_MUTEX_TEST(mIPCMutex); QMutexLocker ipcLocker(mIPCMutex);
    if (mIPC)
    {
        IGOGameInstances::iterator iter = mGames.find(mIPC->getCurrentConnectionId());
        if (iter != mGames.end())
        {
            uint16_t pinnedWidgets = (uint16_t)pinnedWindows().count();
            if (iter->second.maxPinnedWidgets < pinnedWidgets)
                iter->second.maxPinnedWidgets = pinnedWidgets;
        }
    }
}

void IGOWindowManager::igoSetVisible(bool visible, bool emitStateChange)
{
    mIGOVisible = visible; // Make sure to let the system know first

    updatePinnedWindowCount();

#ifdef ORIGIN_MAC
    // Grab the current window with focus in case it is NOT an IGO window
    // -> if the application itself is the front app, we can then restore the focus properly so that the user interaction isn't messed up!
    QWidget* focusWindow = NULL;
    if (IsFrontProcess())
    {
        QWidget* wnd = qApp->activeWindow();
        if (wnd && !IsOIGWindow((void*)wnd->winId()))
            focusWindow = wnd;
    }
    
    {
        DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
        for (tWindowList::const_iterator iter = mTopLevelWindows.begin(); iter != mTopLevelWindows.end(); ++iter)
        {
            IGOQtContainer* window = *iter;
            window->setIGOVisible(visible);
            
            if (!visible)
            {
                window->removeFocus();
            }
            
            else
            {
                // Check whether the window was dirty but never updated its bitmap because OIG was off
                if (window->isDirty())
                    updateWindow(window);
            }
        }
        
        // Initial focus is only broken on Mac so leaving this #ifdef'ed out for Windows
        if (visible)
        {
            // Restore focus to the previously focused window
            setWindowFocus(mWindowHasFocus, true, true);
        }
    }
#endif
    //emitStateChange could be false for the case when we don't want to transmit a statechange due to IGO being destroyed as a result of the game losing focus
    if (emitStateChange)
        emit igoStateChange(visible, mCallOrigin == IIGOCommandController::CallOrigin_SDK);

    mCallOrigin = IIGOCommandController::CallOrigin_CLIENT;

    if(visible == false && mShowPinCanToggleToast)
    {
        const QString title = tr("ebisu_client_window_pinned");
        emit IGOController::instance()->igoShowToast("POPUP_IGO_PIN_ON", title, "");
        mShowPinCanToggleToast = false;
    }

    // Do we have any pending commands to run now that IGO visibility has changed?
    processIGOVisibleCallbacks();

#ifdef ORIGIN_MAC
    // Do we need to restore the client focus?
    if (focusWindow && qApp->activeWindow() != focusWindow)
        qApp->setActiveWindow(focusWindow->window());
#endif
}

void IGOWindowManager::dirtyAllWindows()
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    for (tWindowList::const_iterator iter = mTopLevelWindows.begin(); iter != mTopLevelWindows.end(); ++iter)
    {
        (*iter)->invalidate();
    }
}

void IGOWindowManager::igoScreenResized(uint32_t connectionId, uint32_t screenWidth, uint32_t screenHeight, bool forceUpdate)
{
    // Skip invalid notifications
    if (screenWidth == 0 || screenHeight == 0)
        return;

    if (screenWidth != mScreenWidth || screenHeight != mScreenHeight)
    {
        mPrevScreenWidth = mScreenWidth;
        mPrevScreenHeight = mScreenHeight;
        
        mScreenWidth = screenWidth;
        mScreenHeight = screenHeight;
        
        {
            DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker wndLocker(mTopLevelWindowsMutex);
            
            IGOGameInstances::iterator iter = mGames.find(connectionId);
            if (iter != mGames.end())
            {
                // Log when we cannot display IGO/back again
                int screenLargeEnough = isScreenLargeEnough() ? 1 : 0;
                if (iter->second.screenLargeEnough != screenLargeEnough)
                {
                    iter->second.screenLargeEnough = screenLargeEnough;
                    if (!screenLargeEnough)
                    {
                        ORIGIN_LOG_EVENT << "IGO UNAVAILABLE FOR " << connectionId << ": RESOLUTION OF " << screenWidth << "x" << screenHeight << " - MIN (" << mMinIGOScreenWidth << "x" << mMinIGOScreenHeight << ")";
                    }
                    
                    else
                    {
                        ORIGIN_LOG_EVENT << "IGO AVAILABLE FOR " << connectionId << ": RESOLUTION OF " << screenWidth << "x" << screenHeight << " - MIN (" << mMinIGOScreenWidth << "x" << mMinIGOScreenHeight << ")";
                    }
                }
            }
        }
        
        // We want the windows that are actually listening to the screen size change to be notified first so that they can apply
        // their own logic before we check all the windows to ensure they are on screen + skip invalid screen sizes (like when first attaching to the game)
        if (screenWidth > 0 && screenHeight > 0)
            emit screenSizeChanged(screenWidth, screenHeight, connectionId);

        invalidate();
    }

    else
    if (forceUpdate)
    {
        // Useful when the size hasn't changed, but we re-focused on the game -> need to resend the info since losing focus triggers a reset
        invalidate();
    }
}

IGOQtContainer* IGOWindowManager::findWindow(uint32_t windowId) const
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    for (tWindowList::const_iterator iter = mTopLevelWindows.begin(); iter != mTopLevelWindows.end(); ++iter)
    {
        if ((*iter)->id() == windowId)
            return (*iter);
    }
    return NULL;
}

IGOQtContainer * IGOWindowManager::findWindowWithMouse(QPoint &pt) const
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    tWindowList::const_reverse_iterator iter;
    for (iter = mTopLevelWindows.rbegin(); iter != mTopLevelWindows.rend(); ++iter)
    {
        if ((*iter)->contains(pt))
            return (*iter);
    }
    return NULL;
}

IGOQtContainer * IGOWindowManager::findModalWindow() const
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    for (tWindowList::const_reverse_iterator iter = mTopLevelWindows.rbegin(); iter != mTopLevelWindows.rend(); ++iter)
    {
        if ((*iter)->isModal())
            return (*iter);
    }

    return NULL;
}

void IGOWindowManager::updateMouseButtons()
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    
    // 1) We have to be careful here because of recursivity! For example, releasing the mouse button on a combobox would trigger the
    // combobox hide(), which uses a set of QEventLoops to "flicker" the new selection -> those event loops may process mouse events,
    // which would get us all the way back here -> we need to make sure we have updated the current mouse button state BEFORE we actually
    // send the event!!!
    Qt::MouseButtons lastMouseButtonState = mLastMouseButtonState;
    
    // 2) Always update global state in case we are clicking outside any window (we don't want to give focus to a window by clicking outside and
    // then dragging the cursor inside the window)
    mLastMouseButtonState = mCurrentMouseButtonState;

    if(mCurrentMouseButtonState != lastMouseButtonState && mWindowHasMouse)
    {
        // Activate window first in case of right-click
        if((mCurrentMouseButtonState & (Qt::LeftButton | Qt::RightButton)) != 0)
        {
            if(!mWindowHasMouse->dontStealFocus())
                setWindowFocus(mWindowHasMouse);
        }
        
        if (mWindowHasMouse)
            mWindowHasMouse->sendMouseButtons(mCurrentMouseButtonState, mDoubleClick);
    }

    if (mDoubleClick)
    {
        if(mWindowHasMouse)
            mWindowHasMouse->sendDoubleClick();
        
        mDoubleClick = false;
    }
}

IIGOWindowManager::WindowBehavior IGOWindowManager::validateWindowBehavior(const WindowProperties& properties, const WindowBehavior& behavior)
{
    WindowBehavior winBehavior = behavior;

    // If the window was created by the SDK we need to enfore the closing of the window when closing IGO, and the closing of IGO when closing the window...
    if (properties.callOrigin() == CallOrigin_SDK)
    {
        winBehavior.closeIGOOnClose = true;
        winBehavior.closeOnIGOClose = true;
    }

    // Now do some minimum check/log any inconsistencies (don't log an error when we are the ones who forced some flags!)
    if (winBehavior.closeOnIGOClose && winBehavior.pinnable)
    {
        if (behavior.closeOnIGOClose)
            ORIGIN_LOG_ERROR << "Trying to open new window with closeOnIGOClose and pinnable set at the same time - clearing pinnable";

        winBehavior.pinnable = false;
    }

    if (winBehavior.closeOnIGOClose && winBehavior.persistent)
    {
        if (behavior.closeOnIGOClose)
            ORIGIN_LOG_ERROR << "Trying to open new window with closeOnIGOClose and persistent set at the same time - clearing closeOnIGOClose";

        winBehavior.closeOnIGOClose = false;
    }

    return winBehavior;
}

IGOQtContainer *IGOWindowManager::createQtContainer(QWidget *widget, int32_t wid, const WindowBehavior& settings, const IIGOWindowManager::AnimPropertySet& openAnimProps, const IIGOWindowManager::AnimPropertySet& closeAnimProps)
{
    // First thing: if this a "singleton" window / do we need to remove the previous instance?
    if (settings.singleton)
    {
        for (tWindowList::const_iterator iter = mTopLevelWindows.begin(); iter != mTopLevelWindows.end(); ++iter)
        {
            IGOQtContainer* container = (*iter);
            if (container->wid() == wid)
            {
                QWidget* embeddedWindow = NULL;
                if (container->widget())
                    embeddedWindow = container->widget()->getRealParent();

                bool deleteWindow = embeddedWindow && embeddedWindow != widget;
                if (deleteWindow)
                    removeWindowAndContent(container);
                    
                else
                    removeWindow(container, false);
                break;
            }
        }
    }
    
    IGOQtContainer *window = new IGOQtContainer(this);
    IGOQWidget *igoWidget = new IGOQWidget();

    igoWidget->storeRealParent(widget);

    if(settings.useWidgetAsFocusProxy)
        igoWidget->setFocusProxy(widget);

    // create layout
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(widget);
    igoWidget->setLayout(layout);

    // position widget
    widget->show();
    if (!settings.allowMoveable)
        widget->move(0, 0);
    
    // filter close
    if(!settings.useCustomClose) //in the case of the CS Chat we need to handle this close externally so we set useCustomClose to true
        widget->installEventFilter(igoWidget);

    // filter resize triggered by changed content 
    widget->installEventFilter(window);
    igoWidget->allowClose(settings.allowClose);
    igoWidget->allowMovable(settings.allowMoveable);

    if (settings.allowMoveable)
    {
        ORIGIN_VERIFY_CONNECT(widget, SIGNAL(changePos(QPoint)), igoWidget, SIGNAL(igoMove(QPoint)));
        ORIGIN_VERIFY_CONNECT(widget, SIGNAL(opacityChanged(qreal)), window, SLOT(setAlpha(qreal)));
    }

    // set transparent background palette
    QPalette palette;
    QBrush brush(QColor(255, 255, 255, 0));
    brush.setStyle(Qt::SolidPattern);
    palette.setBrush(QPalette::Active, QPalette::Base, brush);
    palette.setBrush(QPalette::Active, QPalette::Window, brush);
    palette.setBrush(QPalette::Inactive, QPalette::Base, brush);
    palette.setBrush(QPalette::Inactive, QPalette::Window, brush);
    palette.setBrush(QPalette::Disabled, QPalette::Base, brush);
    palette.setBrush(QPalette::Disabled, QPalette::Window, brush);

    // set transparent background
    widget->setPalette(palette);
    if(settings.transparency == Transparency)
        igoWidget->setPalette(palette);

    // set the igoWidget size hint
    igoWidget->setSizeHint(widget->size());

    // sink it before we resize the widget!
    window->sinkQWidget(igoWidget, wid, settings, openAnimProps, closeAnimProps);
    
    // set size
    QSize size = widget->size();
    igoWidget->resize(size);
    window->resize(size.width(), size.height());

    // Is this window game-specific? if so, make sure we hide it when switching to another game + close it automatically
    // when the connection closes
    if (settings.connectionSpecific)
    {
        DEBUG_MUTEX_TEST(mIPCMutex); QMutexLocker ipcLocker(mIPCMutex);
        if(mIPC)
            window->setConnectionId(mIPC->getCurrentConnectionId());
    }

    // Register list of titlebar active buttons that we need to avoid when determining whether the user can drag the window
    UIToolkit::OriginWindow* oWidget = dynamic_cast<Origin::UIToolkit::OriginWindow*>(widget);
    if (oWidget)
    {
        QVector<QObject*> activeButtons = oWidget->titlebarButtons();
        foreach (QObject* btn, activeButtons)
        window->registerTitlebarButton(btn);
    }

    return window;
}

void IGOWindowManager::invalidate()
{
    emit(layoutWindows(mScreenWidth, mScreenHeight));
}

void IGOWindowManager::updatePinningState(bool isPinningInProgress)
{
    // remove all windows and re-draw them when we change the pinning
    setPinningInProgress(isPinningInProgress);
    invalidate();
}

void IGOWindowManager::pinningToggled(IGOQtContainer *widget)
{
    if (widget != NULL)
    {
        GetTelemetryInterface()->Metric_PINNED_WIDGETS_STATE_CHANGED(getActiveProductId().toUtf8().data(), widget->wid(), widget->isPinned(), false);
        updatePinnedWindowCount();
    }
}

void IGOWindowManager::updateWindow(IGOQtContainer *window)
{
    DEBUG_MUTEX_TEST(mIPCMutex); QMutexLocker ipcLocker(mIPCMutex);
    IGO_ASSERT(mIPC);
    if (!mIPC || !mEnableUpdates)  // if no user is yet logged in, do not show any windows...
        return;

    IGOIPC *ipc = IGOIPC::instance();
    
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);

    // check if we have this window in our list, if not, then it might have been removed in the meantime (due to Qt::QueuedConnection), so don't update it!!!!
    tWindowList::const_iterator iter = eastl::find(mTopLevelWindows.begin(), mTopLevelWindows.end(), window);
    if (iter == mTopLevelWindows.end())
        return;

    window->lockWindow();
          
    bool alwaysVisible = window->isAlwaysVisible() || window->isPinned();
    bool visible = alwaysVisible || window->isVisible();

    // Keep track of visibility transitions to update the window properly
    bool wasVisible = window->updateVisibility();
    if (wasVisible != visible)
        window->setUpdateVisibility(visible);
    
    if (IGOWindowManagerIPC::isRunning() && window->isDirty()  // do not update IGO windows if we are in background - wait until we are back
        && (mIGOVisible || visible || wasVisible))  // do not update IGO windows if IGO is off except for windows that are always visible (notifications)
    {
        uint32_t id = window->id();
        ORIGIN_ASSERT(id!=0);

        // Just make sure we sent the initial create message
        uint32_t index = windowIndex(window);
        SendWindowInitialState(window, static_cast<int32_t>(index), mIPC);

        uint8_t alpha = 255;

        if (getPinningInProgress() && window->isRenderingBlockedWhilePinning())
        {
            // only show pinned windows while we are "pinning" a window
            alpha = (window->isPinningInProgress() || window->isPinned()) ? window->pinnedAlpha() : 0;
        }
        else
        {
            if(window->isPinningInProgress())
            {
                alpha = window->pinnedAlpha();
            }
            else
            {
                alpha = IGOController::instance()->isActive() ? window->alpha() : (window->isPinned() ? window->pinnedAlpha() : window->alpha());
            }
        }

        uint flags = window->flags();

        // Let the game-side know this window requires animations to be defined before it can be displayed properly
        uint extraFlags = 0;
        if (window->useAnimations())
            extraFlags |= IGOIPC::WINDOW_UPDATE_REQUIRE_ANIMS;

        // Play with the alpha if not visible at all
        if (!visible)
        {
            alpha = 0;
            flags |=  IGOIPC::WINDOW_UPDATE_ALPHA;
        }

        if (flags&IGOIPC::WINDOW_UPDATE_BITS)
        {
            if (window->drawWindow())
            {
                QRect rect = window->rect(true);
                uint size = window->pixelDataSize();
                IGO_ASSERT(rect.width()*rect.height()*4 == (int)size);

#if !defined(ORIGIN_MAC)
                IGO_TRACEF("%d: window(%x) size(%d)", GetTickCount(), window->id(), size);
#else
                //IGO_TRACEF("%ld: window(%x) size(%d)", GetTickCount(), window->id(), size);
#endif

                if (rect.width()*rect.height()*4 == (int)size)  // don't update if size is not correct.  potential fix for EBIBUGS-14050 - bug sentry crash ending in B9BC (kernelbase.dll for bad allocation)
                {
#ifdef ORIGIN_MAC
                    
//#define SAVE_CONTROL_SNAPSHOTS
#ifdef SAVE_CONTROL_SNAPSHOTS
                    char homePath[MAX_PATH];
                    Origin::Mac::GetHomeDirectory(homePath, sizeof(homePath));
                    
                    char fileName[MAX_PATH];
                    snprintf(fileName, sizeof(fileName), "%s/snapshot%d.tga", homePath, window->id());
                    Origin::Mac::SaveRawRGBAToTGA(fileName, rect.width(), rect.height(), (const unsigned char*)window->pixelData());   
#endif
#endif

                    eastl::shared_ptr<IGOIPCMessage> msg (ipc->createMsgWindowUpdate(id, alwaysVisible, alpha, (int16_t)rect.x(), (int16_t)rect.y(), (int16_t)rect.width(), (int16_t)rect.height(), (const char *)window->pixelData(), window->pixelDataSize(), IGOIPC::WINDOW_UPDATE_ALL | extraFlags));
                    mIPC->send(msg);

                    if (IGOController::instance()->extraLogging())
                        ORIGIN_LOG_ACTION << "IGOWindowManager - updateWindow (ALL)=" << window << "(id=" << id << "), alwaysVisible=" << alwaysVisible << ", alpha=" << alpha << ", x=" << rect.x() << ", y=" << rect.y() << ", width=" << rect.width() << ", height=" << rect.height();
                }
            }
        }
        else
        if (flags == IGOIPC::WINDOW_UPDATE_ALPHA) // only if we change the alpha value exclusivly
        {
            QRect rect = window->rect(true);
            eastl::shared_ptr<IGOIPCMessage> msg (ipc->createMsgWindowUpdate(id, alwaysVisible, alpha, (int16_t)rect.x(), (int16_t)rect.y(), (int16_t)rect.width(), (int16_t)rect.height(), NULL, 0, IGOIPC::WINDOW_UPDATE_ALPHA | extraFlags));
            mIPC->send(msg);

            if (IGOController::instance()->extraLogging())
                ORIGIN_LOG_ACTION << "IGOWindowManager - updateWindow (ALPHA)=" << window << "(id=" << id << "), alwaysVisible=" << alwaysVisible << ", alpha=" << alpha << ", x=" << rect.x() << ", y=" << rect.y() << ", width=" << rect.width() << ", height=" << rect.height();
        }
        else
        {
            IGO_ASSERT((flags&(IGOIPC::WINDOW_UPDATE_WIDTH|IGOIPC::WINDOW_UPDATE_HEIGHT)) == 0);
            flags &= ~(IGOIPC::WINDOW_UPDATE_WIDTH|IGOIPC::WINDOW_UPDATE_HEIGHT);
            QRect rect = window->rect(true);
            eastl::shared_ptr<IGOIPCMessage> msg (ipc->createMsgWindowUpdate(id, alwaysVisible, alpha, (int16_t)rect.x(), (int16_t)rect.y(), (int16_t)rect.width(), (int16_t)rect.height(), NULL, 0, flags | extraFlags));
            mIPC->send(msg);

            if (IGOController::instance()->extraLogging())
                ORIGIN_LOG_ACTION << "IGOWindowManager - updateWindow (ALL BUT Width/Height)=" << window << "(id=" << id << "), alwaysVisible=" << alwaysVisible << ", alpha=" << alpha << ", x=" << rect.x() << ", y=" << rect.y() << ", width=" << rect.width() << ", height=" << rect.height();
        }

        window->setDirty(false);
    }

    window->unlockWindow();
}


void IGOWindowManager::setCursor(int cursor)
{
    switch (cursor)
    {
    case Qt::ArrowCursor:
        setGameCursor((uint)IGOIPC::CURSOR_ARROW);
        break;

    case Qt::CrossCursor:
        setGameCursor((uint)IGOIPC::CURSOR_CROSS);
        break;

    case Qt::PointingHandCursor:
        setGameCursor((uint)IGOIPC::CURSOR_HAND);
        break;

    case Qt::IBeamCursor:
        setGameCursor((uint)IGOIPC::CURSOR_IBEAM);
        break;

    case Qt::SizeAllCursor:
        setGameCursor((uint)IGOIPC::CURSOR_SIZEALL);
        break;

    case Qt::SizeBDiagCursor:
        setGameCursor((uint)IGOIPC::CURSOR_SIZENESW);
        break;

    case Qt::SizeVerCursor:
        setGameCursor((uint)IGOIPC::CURSOR_SIZENS);
        break;

    case Qt::SizeFDiagCursor:
        setGameCursor((uint)IGOIPC::CURSOR_SIZENWSE);
        break;

    case Qt::SizeHorCursor:
        setGameCursor((uint)IGOIPC::CURSOR_SIZEWE);
        break;
            
    default:
        setGameCursor((uint)IGOIPC::CURSOR_ARROW);
        break;
    }
}

void IGOWindowManager::setGameCursor(uint cursor)
{
    if(mCursor!=cursor && cursor != (uint)IGOIPC::CURSOR_UNKNOWN)
    {
        mCursor = cursor;
        DEBUG_MUTEX_TEST(mIPCMutex); QMutexLocker ipcLocker(mIPCMutex);
        IGOIPC *ipc = IGOIPC::instance();
        
        if(ipc && mIPC)
        {
            eastl::shared_ptr<IGOIPCMessage> msg (ipc->createMsgSetCursor((IGOIPC::CursorType)mCursor));
            if(msg.get())
                mIPC->send(msg);
        }
    }
}

void IGOWindowManager::closeGameWindows(uint32_t connectionId)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    
    QVector<IGOQtContainer*> toDelete;
    
    tWindowList::iterator iter = mAllWindows.begin();
    while(iter != mAllWindows.end())
    {
        IGOQtContainer* container = *iter;

        if (container->connectionId() == connectionId)
            toDelete.push_back(container);
        
        else // Notify we're done with this connection
            container->resetInitialStateBroadcast(connectionId);

        ++iter;
    }
    
    foreach (IGOQtContainer* container, toDelete)
    removeWindowAndContent(container);
}

void IGOWindowManager::closeAllWindows(bool preserveChatWindows)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);

    QVector<IGOQtContainer*> toDelete;
    
    tWindowList::iterator iter = mAllWindows.begin();
    while(iter != mAllWindows.end())
    {
        IGOQtContainer* container = *iter;

        bool doNotCloseWidget = container->isPersistent();
        
        // TODO: Need to remove that crappy special case
        if (!container->closeOnIgoClose() && container->wid() == IViewController::WI_CHAT && preserveChatWindows)
        {
            doNotCloseWidget = true;
            
            // We want to keep the conversation alive, but we want the same behavior when it comes to pinneable windows -> unpin if necessary
            if (container->isPinned())
                container->setPinnedSate(false);
        }
        
        if (!doNotCloseWidget)
        {
            // Force the friends list to reload when shown to clear out any internal state it may still have from a previous IGO session (search, etc)
            // TODO: check we don't need this anymore
//            if (mFriends && (*iter) == mFriends && mFriendsController)
//            {
//                mFriendsController->reload();
//            }
            
            toDelete.push_back(container);
        }

        else // Notify we're done with this connection
            container->clearInitialStateBroadcast();

        
        ++iter;
    }
    
    foreach (IGOQtContainer* container, toDelete)
        removeWindowAndContent(container);
}

using namespace Origin::UIToolkit;
IGOQtContainer* IGOWindowManager::createContainer(Origin::UIToolkit::OriginWindow* const window, int32_t wid, const WindowBehavior& behavior, const IIGOWindowManager::AnimPropertySet& openAnimProps, const IIGOWindowManager::AnimPropertySet& closeAnimProps)
{
    IGOQtContainer* container = createQtContainer(window, wid, behavior, openAnimProps, closeAnimProps);

    if (!behavior.disableDragging)
        container->setDraggingTop(40);

    else
        container->setDraggingTop(0);

#ifdef ORIGIN_MAC
    if (!behavior.disableResizing)
    {
        container->setResizingBorder(10, 10);
        container->setOuterMargins(6, 6, 6, 6);
    }

    else
    {
        container->setResizingBorder(0, 0);
        container->setOuterMargins(0, 0, 0, 0);
    }
#else
    if (!behavior.disableResizing)
        container->setResizingBorder(20, 10);

    else
        container->setResizingBorder(0, 0);
#endif
    
    addWindow(container);
    return container;
}

bool IGOWindowManager::igoCommand(int cmd, IIGOCommandController::CallOrigin callOrigin)
{
    DEBUG_MUTEX_TEST(mIPCMutex); QMutexLocker ipcLocker(mIPCMutex);
    IGO_ASSERT(mIPC);
    if (!mIPC)
        return false;
    

    IGO_ASSERT(IGOController::instance());

#ifdef ORIGIN_PC
    IGO_ASSERT(IGOController::instance()->twitch());

    // twitch command handling
    if (IGOController::instance()->twitch()->igoCommand(cmd, callOrigin))
        return true;
#endif

    // ui command handling
    switch (cmd)
    {

    case IIGOCommandController::CMD_NEW_BROWSER:
        {
            IGOGameTracker::GameInfo info = IGOController::instance()->gameTracker()->gameInfo(mIPC->getCurrentConnectionId());
            QString url(info.mDefaultUrl.toEncoded());
            WindowProperties settings;
#ifndef ORIGIN_MAC
            settings.setMaximumSize();
#endif
            Origin::Engine::IWebBrowserViewController::BrowserFlags flags = ( Origin::Engine::IWebBrowserViewController::BrowserFlags)(IWebBrowserViewController::IGO_BROWSER_SHOW_NAV | IWebBrowserViewController::IGO_BROWSER_RESOLVE_URL);
            IGOController::instance()->createWebBrowser(url, flags, settings, IIGOWindowManager::WindowBehavior());
        }
        return true;
    
    case IIGOCommandController::CMD_SHOW_FRIENDS:
        {
            IGO_TRACE("Show Friends");
            IGOController::instance()->igoShowFriendsList(callOrigin);
        }
        return true;
    
    case IIGOCommandController::CMD_SHOW_ACHIEVEMENTS:
        {
            IGO_TRACE("Show Achievements");
            IGOGameTracker::GameInfo info = IGOController::instance()->gameTracker()->gameInfo(mIPC->getCurrentConnectionId());
            IGOController::instance()->EbisuShowAchievementsUI(0, 0, info.mBase.mAchievementSetID, false, callOrigin);
        }
        return true;

    case IIGOCommandController::CMD_SHOW_SETTINGS:
        {
            IGO_TRACE("Show Settings");
            IGOController::instance()->igoShowSettings();
        }
        return true;

    case IIGOCommandController::CMD_SHOW_CS:
        {
            IGOController::instance()->igoShowCustomerSupport();
        }
        return true;

    case IIGOCommandController::CMD_CLOSE_IGO:
        {
            eastl::shared_ptr<IGOIPCMessage> msg(IGOIPC::instance()->createMsgSetIGOMode(false));
            mIPC->send(msg);            
        }

        return true;
    }

    return false;
}

void IGOWindowManager::injectIGOStart(uint id)
{
    emit igoStart(id);
}

void IGOWindowManager::injectIGOStop()
{
    emit igoStop();
}

void IGOWindowManager::injectIGOVisible(bool visible, bool emitStateChange)
{
    emit igoVisible(visible, emitStateChange);
}

void IGOWindowManager::igoStarted(uint connectionId)
{
    DEBUG_MUTEX_TEST(mIPCMutex); QMutexLocker ipcLocker(mIPCMutex);
    IGOController::instance()->resetHotKeys();
}


void IGOWindowManager::igoAddWindow(IGOQtContainer *window)
{
    addWindow(window);
}

void IGOWindowManager::onGameProcessFinished(uint32_t pid, int exitCode)
{
    Q_UNUSED(exitCode);

    if(mGames.find(pid) != mGames.end())
    {
        igoDisconnected(pid, true);
    }

    else
    {
        // For some games (ie Bejeweled 3), the initial process id may not correspond to the actual connection id (ie no rendering loop).
        // We have some code on the IPC manager side to check whether the process is still alive when losing the connection, but then it's all about timing.
        // So just in case, request the IPC manager to check for any dead connection processes that we knew about
        DEBUG_MUTEX_TEST(mIPCMutex); QMutexLocker ipcLocker(mIPCMutex);
        if (mIPC)
            mIPC->checkConnectionProcesses();
    }
}

void IGOWindowManager::onGameSpecificWindowClosed(uint32_t connectionId)
{
    bool closeIGO = true;

    // If not connection-specific, do close IGO immediately
    if (connectionId != 0)
    {
        // This is called when a game-specific window (mini-app browser?) is flagged to close IGO
        // -> we need to check whether there are other such windows open. If not then we can actually
        // close IGO (this is to work with scenarios like the Sims4 store + separate paypal window)
        {
            DEBUG_MUTEX_TEST(mIPCMutex); QMutexLocker ipcLocker(mIPCMutex);
            if (mIPC)
            {
                if (mIPC->getCurrentConnectionId() != connectionId)
                    return;
            }
        }

        DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker wndLocker(mTopLevelWindowsMutex);
        foreach(IGOQtContainer * view, mTopLevelWindows)
        {
            if (view->connectionId() == connectionId && view->closeIGOOnClose())
            {
                closeIGO = false;
                break;
            }
        }
    }

    if (closeIGO)
        Origin::Engine::IGOController::instance()->EbisuShowIGO(false);
}

///////////////////////////////////////////////////////////////////////////////////////////////
//NOTE: igoDisconnected may be called multiple times, event if the game process still lives!!!
///////////////////////////////////////////////////////////////////////////////////////////////
void IGOWindowManager::igoDisconnected(uint id, bool processIsGone)
{
    if (processIsGone)
    {
        IGOGameInstances::iterator iter = mGames.find(id);
        if (iter != mGames.end())
        {
            IGOGameTracker::GameInfo info = IGOController::instance()->gameTracker()->gameInfo(id);
            if (info.isValid())
            {
                GetTelemetryInterface()->Metric_PINNED_WIDGETS_COUNT(info.mBase.mProductID.toUtf8().data(), iter->second.maxPinnedWidgets, false);

                // Before we remove our reference to the game product and all, make sure to save our current set of windows
                DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
                for (tWindowList::const_iterator iter = mTopLevelWindows.begin(); iter != mTopLevelWindows.end(); ++iter)
                    (*iter)->storeWidget(info.mBase.mProductID);
            }

            mGames.erase(iter);
        }
        
        uint32_t gameCount = IGOController::instance()->gameTracker()->removeGame(id);
        if (gameCount == 0)
        {
            // Chat sessions should persist even if SDK disconnection is detected.
            // Exiting a game should keep the users chat sessions open!
            closeAllWindows(true);

            ORIGIN_LOG_EVENT << "igoDisconnected and no games left to track and processIsGone - reset mScreenWidth and mScreenHeight";
            mScreenWidth = mScreenHeight = 0;   // reset the screen size otherwise the next game will start with this size even if minimized (EBIBUGS-28486)
        }
        
        else
        {
            // Close game-specific windows
            closeGameWindows(id);
        }
    }
}

void IGOWindowManager::igoConnected(uint id, const QString& gameProcessFolder)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker wndLocker(mTopLevelWindowsMutex);

	// Check whether the game really closed (ie the process was gone) before resetting our tracking info
	// (on PC, we could have lost the connection without really leaving the game)
	IGOGameInstances::iterator iter = mGames.find(id);
	if (iter == mGames.end())
	{
        IGOGameInfo info;
        info.mIsValid = true;

		mGames[id] = info;
        
        IGOController::instance()->gameTracker()->addGame(id, gameProcessFolder);
	}
}


void IGOWindowManager::injectIgoConnect(uint id, const QString& gameProcessFolder)
{
    emit igoConnect(id, gameProcessFolder);
}

void IGOWindowManager::injectIgoDisconnect(uint id, bool processIsGone)
{
    emit igoDisconnect(id, processIsGone);
}

void IGOWindowManager::processTick(bool alwaysProcessMouseEvent)
{
    // if the mouse was moved, process the event
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    IGOQtContainer * newModalWindow = findModalWindow();
    
    IGOQtContainer* initWindowHasMouse = mWindowHasMouse; // at first we're not going to modify the content of mWindowHasFocus
    if(mCurrentMousePos != mLastMousePos || ( newModalWindow != NULL && newModalWindow != initWindowHasMouse ) )
    {       
        IGOQtContainer * newTargetWindow = NULL;        
        if( newModalWindow )
        {
            newTargetWindow = newModalWindow;

            if (IGOController::instance()->extraLogging())
                ORIGIN_LOG_ACTION << "IGOWindowManager - processTick - New target window from findModalWindow = " << newTargetWindow;
        }
        else
        {

            if( !initWindowHasMouse || !initWindowHasMouse->isMouseGrabbingOrClicked() )
            {
                newTargetWindow = findWindowWithMouse(mCurrentMousePos);    // returns NULL if no window is found!!!
                
                if (IGOController::instance()->extraLogging())
                {
                    if (newTargetWindow != initWindowHasMouse)
                        ORIGIN_LOG_ACTION << "IGOWindowManager - processTick - New target window from findWindowWithMouse = " << newTargetWindow;
                }
            }
            else
            {
                newTargetWindow = initWindowHasMouse;
            }
        }
        
        // if the window containing the mouse has changed, notify the old window
        if( initWindowHasMouse != newTargetWindow )
        {
            if (IGOController::instance()->extraLogging())
            {
                ORIGIN_LOG_ACTION << "IGOWindowManager - processTick - changing window from " << initWindowHasMouse << " to " << newTargetWindow;
                ORIGIN_LOG_ACTION << "with hasMouse = " << mWindowHasMouse << ", hasFocus = " << mWindowHasFocus;
            }
            
            // We're about to send mouse events, which may in turn invalidate the pointers we are currently using (could get deleted from an removeWindow() call
            // or simply changed. So if we detect that the context has changed, we simply stop the processing of the event. Note that, in the case of the deletion,
            // this works because we directly use delete on the IGOQtContainer instead of a deleteLater(). If that changes, this code needs to be updated accordingly.
            QPointer<IGOQtContainer> safeGuard(newTargetWindow);
            
            if(initWindowHasMouse)
            {
#ifdef ORIGIN_MAC
                initWindowHasMouse->sendMousePos(QPoint(0xffff, 0xffff), mCurrentMousePosOffset, mCurrentMousePosScaling, 0);  // use a invalid mouse position to remove mouse over/hover effects
#else
                initWindowHasMouse->sendMousePos(QPoint(0xffff, 0xffff), mCurrentMousePosOffset, mCurrentMousePosScaling); // use a invalid mouse position to remove mouse over/hover effects
#endif
                // At this point mWindowHasMouse may have been modified by previous event processing
                initWindowHasMouse->sendMouseButtons(0);
            }
            
            if (!newTargetWindow || (safeGuard && !safeGuard->pendingDeletion()))
                mWindowHasMouse = safeGuard;
            
            else
            {
                if (IGOController::instance()->extraLogging())
                    ORIGIN_LOG_ACTION << "IGOWindowManager - processTick - prevented use of invalide window";
                
                mWindowHasMouse = NULL;
            }
        }

        // At this point mWindowHasMouse may have been modified by previous event processing
        if(mWindowHasMouse)
        {
            // notify new window with the new mouse position
#ifdef ORIGIN_MAC
            mWindowHasMouse->sendMousePos(mCurrentMousePos, mCurrentMousePosOffset, mCurrentMousePosScaling, mCurrentMouseTimestamp);
#else
            mWindowHasMouse->sendMousePos(mCurrentMousePos, mCurrentMousePosOffset, mCurrentMousePosScaling);
#endif

            updateMouseButtons();
            
        }
        mLastMousePos = mCurrentMousePos;
    }
    else
    {
        if (alwaysProcessMouseEvent)
        {
            if (mWindowHasMouse)
            {
#ifdef ORIGIN_MAC
                mWindowHasMouse->sendMousePos(mCurrentMousePos, mCurrentMousePosOffset, mCurrentMousePosScaling, mCurrentMouseTimestamp);
#else
                mWindowHasMouse->sendMousePos(mCurrentMousePos, mCurrentMousePosOffset, mCurrentMousePosScaling);
#endif
            }
        }
        
        updateMouseButtons();
    }

    // we enable scrolling with the mouse wheel, even if the Window has no click focus
    if (mMouseWheelDeltaV != 0 || mMouseWheelDeltaH != 0)
    {
        IGOQtContainer* scrollWindow = findWindowWithMouse(mCurrentMousePos);
        
        if (scrollWindow)
            scrollWindow->sendMouseWheel(mMouseWheelDeltaV, mMouseWheelDeltaH);
        mMouseWheelDeltaV = 0;
        mMouseWheelDeltaH = 0;
    }

    if(mCurrentKeyWParam!=0 && mWindowHasFocus)
    {
#ifdef ORIGIN_MAC
        updateWindowFocus();
        if (mWindowHasFocus)
            mWindowHasFocus->sendKeyEvent(mCurrentKeyData);
#else
        if (mWindowHasFocus)
        {
            mWindowHasFocus->setFocus(true);
            mWindowHasFocus->sendKeyEvent(mCurrentKeyEventType, mCurrentKeyWParam, mCurrentKeyLParam);
        }
#endif

        mCurrentKeyWParam=0;
        mCurrentKeyLParam=0;
        mCurrentKeyEventType = 0;
    }
}

#if defined(ORIGIN_MAC)

void IGOWindowManager::injectKeyboardEvent(const char* data, int32_t size)
{
    // TODO: if Qt isn't smart about this, may need to create a nice little cyclic queue/pool for that data
    emit keyboardEvent(QByteArray(data, size));
}

void IGOWindowManager::injectInputEvent(uint32_t type, int32_t param1, int32_t param2, int32_t param3, int32_t param4, float param5, float param6, uint64_t param7)
{
    emit inputEvent(type, param1, param2, param3, param4, param5, param6, param7);
}

#else

void IGOWindowManager::injectInputEvent(uint32_t type, int32_t param1, int32_t param2, int32_t param3, int32_t param4, float param5, float param6)
{
    emit inputEvent(type, param1, param2, param3, param4, param5, param6);
}

#endif

void IGOWindowManager::addContextMenu(QWidget *widget, int x, int y)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    if (mWindowHasMouse)
    {
        if (IGOController::instance()->extraLogging())
            ORIGIN_LOG_ACTION << "IGOWindowManager - addContextMenu = " << widget << " to " << mWindowHasMouse;

        // let the current window know about the context menu
        mWindowHasMouse->addContextMenu(widget, x, y);
    }
}

void IGOWindowManager::removeContextMenu()
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    if (mWindowHasMouse)
    {
        if (IGOController::instance()->extraLogging())
            ORIGIN_LOG_ACTION << "IGOWindowManager - removeContextMenu from " << mWindowHasMouse;

        mWindowHasMouse->removeContextMenu();
    }
}

void IGOWindowManager::quit()
{
    ORIGIN_LOG_EVENT << "Deleting IGO IPC.";
    DEBUG_MUTEX_TEST(mIPCMutex); QMutexLocker ipcLocker(mIPCMutex);
    if (mIPC)
    {
        delete mIPC;
        mIPC = NULL;
    }
    ORIGIN_LOG_EVENT << "Successfully deleted IGO IPC.";

    // do not delete any Qt widgets manually here, Qt takes care of them!
}

void IGOWindowManager::maximizeContainer(IGOQtContainer *container)
{
    QSize maxSize = maximumWindowSize();
    container->resize(maxSize.width(), maxSize.height());
    container->setPosition(centeredPosition(maxSize));
    }

uint32_t IGOWindowManager::windowIndex(IGOQtContainer *container)
{
    
    uint32_t index = 0;
    for (tWindowList::iterator topIter = mTopLevelWindows.begin(); topIter != mTopLevelWindows.end(); ++topIter)
    {
        if (*topIter == container)
            return index;

        ++index;
    }

    // We should never get here!
    IGO_ASSERT(0);
    return 0;
}

void IGOWindowManager::igoToggledPins(bool state)
{
    // got through all pinned windows and put them into a "pending" state - if we are not already in that state
    // redraw all windows
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);

    for (tWindowList::const_iterator iter = mTopLevelWindows.begin(); iter != mTopLevelWindows.end(); ++iter)
    {
        (*iter)->setPendingPinnedState(state);
        (*iter)->invalidate();
    }

    if(state && mIGOVisible == false && mShowPinHiddenToast)
    {
        const QString title = tr("ebisu_client_all_windows_have_been_unpinned").arg(tr("ebisu_client_igo_name"));
        emit IGOController::instance()->igoShowToast("POPUP_IGO_PIN_OFF", title, "");
        mShowPinHiddenToast = false;
    }

    GetTelemetryInterface()->Metric_PINNED_WIDGETS_HOTKEY_TOGGLED(getActiveProductId().toUtf8().data(), !state, false);
}

void IGOWindowManager::igoUnpinnedAll(const IGOQtContainer *keepPinnedWidget)
{
    // unpin all widgets except the keepPinnedWidget
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    bool inPinsToggleMode = false;
    for (tWindowList::const_iterator iter = mTopLevelWindows.begin(); iter != mTopLevelWindows.end(); ++iter)
    {
        if ((*iter) != keepPinnedWidget && (*iter)->isPendingPinnedState())
        {
            inPinsToggleMode = true;
            break;
        }
    }

    if (inPinsToggleMode)
    {
        for (tWindowList::const_iterator iter = mTopLevelWindows.begin(); iter != mTopLevelWindows.end(); ++iter)
        {
            if ((*iter) != keepPinnedWidget)
            {
                // reset the pinning state of the widget
                (*iter)->setPendingPinnedState(false);
                (*iter)->setPinnedSate(false);
                (*iter)->pinButtonClicked();    // do the actual pin setting deletion

                (*iter)->invalidate();
            }
        }
    }

}

// contains code that update the UI depending on the current game that has the focus!!!
void IGOWindowManager::igoUpdatedActiveGameData(uint connectionId)
{
    // First check which game had focus
    uint32_t prevConnectionId = IGOController::instance()->gameTracker()->currentGameId();
    
    // Now notify we have changed
    IGOController::instance()->gameTracker()->setCurrentGame(connectionId);
    
    // Hide previous game-specific windows/show current game-specific windows
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    
    tWindowList::iterator iter = mAllWindows.begin();
    while(iter != mAllWindows.end())
    {
        IGOQtContainer* container = *iter;

        if (container->connectionId() != IGOWindowManagerIPC::NO_CONNECTION)
        {
            if (container->connectionId() == connectionId)
            {
                // Show the windows - if it was visible to start with
                if (container->visibleBeforeGameSwitch())
                    container->show(true);
            }
            
            else // Only try to hide windows for the game that lost focus - otherwise we may never see our windows again!
            if (container->connectionId() == prevConnectionId)
            {
                // Hide window
                container->setVisibleBeforeGameSwitch(container->isVisible());
                container->show(false);
            }
        }
        
        ++iter;
    }
}
    
QString IGOWindowManager::getActiveProductId() const
{
    DEBUG_MUTEX_TEST(mIPCMutex); QMutexLocker ipcLocker(mIPCMutex);
    QString activeProductId;
    if (mIPC)
    {
        IGOGameTracker::GameInfo info = IGOController::instance()->gameTracker()->gameInfo(mIPC->getCurrentConnectionId());
        if (info.isValid())
        {
            ORIGIN_ASSERT(sizeof(wchar_t)==sizeof(wchar_t));
            return info.mBase.mProductID;
        }
}
    return QString();
}

const QList<IGOQtContainer const*> IGOWindowManager::pinnedWindows()
{
    QList<IGOQtContainer const*> pinnedWindows;
    for(tWindowList::const_iterator iter = mTopLevelWindows.begin(); iter != mTopLevelWindows.end(); ++iter)
    {
        IGOQtContainer* window = *iter;
        if(window && window->isPinned())
            pinnedWindows.append(window);
    }
    return pinnedWindows;
}

bool IGOWindowManager::restorePinnedWidget(IGOQtContainer* container)
{
    int pinnedCount = pinnedWindows().count();
    bool restored = container->restoreWidget();

    // Reset message if this is the first pinned window
    if(pinnedCount == 0 && container->isPinned())
    {
        mShowPinCanToggleToast = true;
        mShowPinHiddenToast = true;
    }

    return restored;
}

void IGOWindowManager::loadPinnedWidget(int32_t wid, const QString& activeProductId, const QString& uniqueID, QString& data)
{
    if (!activeProductId.isEmpty())
    {
        // Don't use the wid when saving the data for a window with a unique ID (ie mini-app), otherwise depending on the order the window is created, we may not find out data back
        QString settingsName;
        if (uniqueID.isEmpty())
            settingsName = QString("PinnedIGOWidget_") + activeProductId + QString("_") + QString::number(wid) + QString("_") + QString::number(mScreenWidth) + QString("_") + QString::number(mScreenHeight);

        else
            settingsName = QString("PinnedIGOWidget_") + activeProductId + QString("_0_") + QString::number(mScreenWidth) + QString("_") + QString::number(mScreenHeight) + QString("_") + uniqueID;

        Origin::Services::Setting SETTING_PINNED_DATA(settingsName, Origin::Services::Variant::String, "", Origin::Services::Setting::LocalPerUser, Origin::Services::Setting::ReadWrite);
        data = Origin::Services::readSetting(SETTING_PINNED_DATA, Origin::Services::Session::SessionService::currentSession()).toString();
    }
}

void IGOWindowManager::storePinnedWidget(int32_t wid, const QString& activeProductId, const QString& uniqueID, const QString& data)
{
    if (!activeProductId.isEmpty())
    {
        // Don't use the wid when saving the data for a window with a unique ID (ie mini-app), otherwise depending on the order the window is created, we may not find out data back
        QString settingsName;
        if (uniqueID.isEmpty())
            settingsName = QString("PinnedIGOWidget_") + activeProductId + QString("_") + QString::number(wid) + QString("_") + QString::number(mScreenWidth) + QString("_") + QString::number(mScreenHeight);

        else
            settingsName = QString("PinnedIGOWidget_") + activeProductId + QString("_0_") + QString::number(mScreenWidth) + QString("_") + QString::number(mScreenHeight) + QString("_") + uniqueID;

        Origin::Services::Setting SETTING_PINNED_DATA(settingsName, Origin::Services::Variant::String, "", Origin::Services::Setting::LocalPerUser, Origin::Services::Setting::ReadWrite);
        Origin::Services::writeSetting(SETTING_PINNED_DATA, data, Origin::Services::Session::SessionService::currentSession());

        // We want to show the "You can toggle pin windows" on the first pin the user does, or if they unpin all windows and pin a window again.
        if(pinnedWindows().count() == 1)
        {
            mShowPinCanToggleToast = true;
            mShowPinHiddenToast = true;
        }
    }
}

void IGOWindowManager::deletePinnedWidget(int32_t wid, const QString& activeProductId, const QString& uniqueID)
{
    if (!activeProductId.isEmpty())
    {
        QString settingsName = QString("PinnedIGOWidget_")+ activeProductId + QString("_") + QString::number(wid) + QString("_") + QString::number(mScreenWidth) + QString("_") + QString::number(mScreenHeight);
        if (!uniqueID.isEmpty())
            settingsName += QString("_") + uniqueID;

        Origin::Services::Setting SETTING_PINNED_DATA(settingsName, Origin::Services::Variant::String, "", Origin::Services::Setting::LocalPerUser, Origin::Services::Setting::ReadWrite);
        Origin::Services::deleteSetting(SETTING_PINNED_DATA, Origin::Services::Session::SessionService::currentSession());
    }

    if(pinnedWindows().count() == 0)
    {
        mShowPinCanToggleToast = false;
        mShowPinHiddenToast = false;
    }
}

bool IGOWindowManager::isGameProcessStillRunning(uint32_t connectionId)
{
    bool isAlive = true;

#if defined(ORIGIN_PC)
    if (connectionId!=0)
    {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, connectionId);
        // Different operating systems behave differently with this call.  XP doesn't support PROCESS_QUERY_LIMITED_INFORMATION,
        // but in some situations Windows 7 fails
        if (!hProcess)
            hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, connectionId);

        DWORD exitCode = STILL_ACTIVE;
        bool isSuccess = 0 != ::GetExitCodeProcess( hProcess, &exitCode );
        if (hProcess == NULL || hProcess == INVALID_HANDLE_VALUE || !isSuccess || exitCode != STILL_ACTIVE)
        {
            isAlive = false;
        }

        if (hProcess != NULL && hProcess != INVALID_HANDLE_VALUE)
            CloseHandle(hProcess);
    }
#endif

    return isAlive;
}


void IGOWindowManager::setWindowFocus(IGOQtContainer* window, bool update/*=true*/, bool forcedRefresh/*=false*/)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    
    if (IGOController::instance()->extraLogging())
        ORIGIN_LOG_ACTION << "IGOWindowManager - setWindowFocus = " << window << ", old = " << mWindowHasFocus << ", forced = " << forcedRefresh;
    
    IGOQtContainer* hasFocus = mWindowHasFocus;
    if (hasFocus != window || forcedRefresh)
    {
        // Should we delete the window when it loses focus? (for example the Web ssl info popup)
        if (hasFocus && hasFocus != window && hasFocus->deleteOnLosingFocus())
        {
            if (IGOController::instance()->extraLogging())
                ORIGIN_LOG_ACTION << "IGOWindowManager - setWindowFocus - deleteOnLosingFocus";

            removeWindowAndContent(hasFocus);
        }
        
        if (update)
        {
            mWindowHasFocus = window;
            updateWindowFocus();
        }
    }
}

void IGOWindowManager::updateWindowFocus()
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    // remove focus from all other top level windows
    IGOQtContainer* hasFocus = mWindowHasFocus;
    for (tWindowList::const_reverse_iterator iter = mTopLevelWindows.rbegin(); iter != mTopLevelWindows.rend(); ++iter)
    {
        if ((*iter) != hasFocus)
            (*iter)->setFocus(false);
    }

    if (hasFocus)
    {
        moveToFront(hasFocus);
#ifdef ORIGIN_MAC
        if (mIGOVisible)
            hasFocus->setFocus(true);
#else
        hasFocus->setFocus(true);
#endif
    }
}

void IGOWindowManager::igoCloseWindowsOnIgoClose()
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);

    QList<IGOQtContainer *> viewsToClose;
    foreach(IGOQtContainer * view, mTopLevelWindows)
    {
        if(view && view->closeOnIgoClose())
        {
            viewsToClose.append(view);
        }
    }

    foreach(IGOQtContainer *view, viewsToClose)
        removeWindowAndContent(view);
}

void IGOWindowManager::showBrowserForGame()
{
    const IGOGameTracker::GameInfo info = IGOController::instance()->gameTracker()->gameInfo(mIPC->getCurrentConnectionId());
    const QString url(info.mDefaultUrl.toEncoded());
    WindowProperties settings;
#ifndef ORIGIN_MAC
    settings.setMaximumSize();
#endif
    IGOController::instance()->createWebBrowser(url, IWebBrowserViewController::IGO_BROWSER_SHOW_NAV, settings, IIGOWindowManager::WindowBehavior());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// PUBLIC API:
bool IGOWindowManager::setIGOVisible(bool visible, ICallback* callback, IIGOCommandController::CallOrigin callOrigin)
{
    DEBUG_MUTEX_TEST(mIPCMutex); QMutexLocker ipcLocker(mIPCMutex);
    IGO_ASSERT(mIPC);
    if (mIPC)
    {
        if (callback)
            mIGOVisibleCallbacks.push_back(callback);

        if (mIGOVisible != visible)
        {
            mCallOrigin = callOrigin;   // If the reason for opening IGO is a call from SDK, do not show the initial Welcome window.
                                        // Of course this is not a perfect solution, but good enough for the common scenario/don't worry about edge-cases
                                        // as this is going to change entirely for Origin X.

            eastl::shared_ptr<IGOIPCMessage> msg (IGOIPC::instance()->createMsgSetIGOMode(visible));
            mIPC->send(msg);
        }
        
        else
        {
            emit requestIGOVisibleCallbacksProcessing();
        }
        
        return true;
    }
    
    delete callback;
    
    return false;
}

QPoint IGOWindowManager::centeredPosition(const QSize& windowSize) const
{
    int x = (mScreenWidth - windowSize.width()) / 2;
    int y = (mScreenHeight - windowSize.height()) / 2;
    
    return QPoint(x, y);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// PRIVATE SLOTS
void IGOWindowManager::processIGOVisibleCallbacks()
{
    // Let's make sure we're not going to have problems with re-entry!
    IGOVisibleCallbacks callbacks;
    {
        DEBUG_MUTEX_TEST(mIPCMutex); QMutexLocker ipcLocker(mIPCMutex);
        callbacks.swap(mIGOVisibleCallbacks);
    }

    for (IGOVisibleCallbacks::const_iterator iter = callbacks.begin(); iter != callbacks.end(); ++iter)
    {
        ICallback* callback = *iter;
        (*callback)(this);
        delete callback;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// PRIVATE
void IGOWindowManager::addWindow(IGOQtContainer *window)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    
    IGO_ASSERT(window);
    if (!window)
        return;
    
    tWindowList::const_iterator iter = eastl::find(mTopLevelWindows.begin(), mTopLevelWindows.end(), window);
    if (iter == mTopLevelWindows.end())
    {
        // The last top level window is always the topmost window (well, after all the 'alwaysOnTop' windows)
        size_t index = 0;
        if (window->alwaysOnTop())
        {
            index = mTopLevelWindows.size();
            mTopLevelWindows.push_back(window);
        }
        
        else
        {
            tWindowList::iterator topIter = mTopLevelWindows.begin();
            for (; topIter != mTopLevelWindows.end(); ++topIter)
            {
                IGOQtContainer* topWindow = *topIter;
                if (topWindow->alwaysOnTop())
                    break;
                
                ++index;
            }
            
            mTopLevelWindows.insert(topIter, window);
        }
        
        // Notify IGO about the new window (this is really so that we can set the initial window location in the stack)
        // HOWEVER we don't want to automatically start the IPC server when creating the initial/persistent windows
        // (ie the bottom/top bar) since we only want to start the server when the initial set of entitlement is loaded
        // (so that we can reconnect to games after closing the Origin client) - no worries since we will send a reset()
        // anyways at the beginning
        if (mIPC->isStarted())
            SendWindowInitialState(window, static_cast<int32_t>(index), mIPC);
        
        tWindowList::const_iterator allWindowsIter = eastl::find(mAllWindows.begin(), mAllWindows.end(), window);
        if (allWindowsIter == mAllWindows.end())
            mAllWindows.push_back(window);
        
        ORIGIN_VERIFY_CONNECT(window, SIGNAL(layoutChanged()), this, SLOT(invalidate()));
        ORIGIN_VERIFY_CONNECT(window, SIGNAL(cursorChanged(uint)), this, SLOT(setGameCursor(uint)));
        ORIGIN_VERIFY_CONNECT(window->widget(), SIGNAL(igoCommand(int, Origin::Engine::IIGOCommandController::CallOrigin)), this, SLOT(igoCommand(int, Origin::Engine::IIGOCommandController::CallOrigin)));
        ORIGIN_VERIFY_CONNECT(window, SIGNAL(pinningStateChanged(bool)), this, SLOT(updatePinningState(bool)));
        ORIGIN_VERIFY_CONNECT(window, SIGNAL(pinningToggled(IGOQtContainer *)), this, SLOT(pinningToggled(IGOQtContainer *)));
        ORIGIN_VERIFY_CONNECT_EX(window, SIGNAL(windowDirty(IGOQtContainer *)), this, SLOT(updateWindow(IGOQtContainer *)), Qt::QueuedConnection);
        
        if (!window->dontStealFocus())
            setWindowFocus(window, IGOController::instance()->isGameInForeground());
        
        window->invalidate();
        window->show(true); // little squish helper
    }
    else
    {
        IGO_ASSERT(0);
    }
}

void IGOWindowManager::removeWindow(IGOQtContainer *window, bool deleteWindow)
{
    if (!window)
        return;
    
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker2(mTopLevelWindowsMutex);
    
    tWindowList::iterator iter = eastl::find(mAllWindows.begin(), mAllWindows.end(), window);
    if (iter != mAllWindows.end())
    {
        // Reset the cursor so it won't be stuck at a resize cursor
        setCursor(Qt::ArrowCursor);
        
        tWindowList::iterator topWindowsIter = eastl::find(mTopLevelWindows.begin(), mTopLevelWindows.end(), window);
        bool isActive = topWindowsIter != mTopLevelWindows.end();
        
        if (IGOController::instance()->extraLogging())
            ORIGIN_LOG_ACTION << "IGOWindowManager - removeWindow - window active = " << isActive;
        
        // Store the window if enabled
        window->storeWidget();
        
        // disable the global pinnning state if we are about to remove that window while we pin it!!!
        if (window->isPinningInProgress())
        {
            updatePinningState(false);
        }
        
        if (isActive)
        {
            ORIGIN_VERIFY_DISCONNECT(window, 0, this, 0);
            
            window->show(false); // little squish helper
            // tell IGO to remove window
            DEBUG_MUTEX_TEST(mIPCMutex); QMutexLocker ipcLocker(mIPCMutex);
            IGOIPC *ipc = IGOIPC::instance();
            
            if(ipc && mIPC)
            {
                eastl::shared_ptr<IGOIPCMessage> msg (ipc->createMsgCloseWindow(window->id()));
                if(msg.get())
                    mIPC->send(msg);
            }
            
            mTopLevelWindows.erase(topWindowsIter);
        }
        
        if (IGOController::instance()->extraLogging())
            ORIGIN_LOG_ACTION << "IGOWindowManager - removeWindow - begin - hasMouse = " << mWindowHasMouse << ", hasFocus = " << mWindowHasFocus;
        
        if (mWindowHasMouse == window)
            mWindowHasMouse = NULL;
        
        if (mWindowHasFocus == window)
        {
            // Make sure we're not going to access this guy when switching focus
            window->setFocus(false);
            mWindowHasFocus = NULL;
        }
        
        QWidget* embeddedWindow = NULL;
        if (window->widget())
        {
            embeddedWindow = window->widget()->getRealParent();
            
            if (embeddedWindow && deleteWindow)
                embeddedWindow->deleteLater();
            
            window->widget()->storeRealParent(NULL);
        }
        
        // We need to use deleteLater, however the event will only need be processed during the next event loop -> also flag the
        // object itself so that we can detect it's invalid if the removal happened in an event while the event source is trying to
        // use the same object (for example in processTick)
        window->postDeletion();
        
        mAllWindows.erase(iter);
               
#ifdef ORIGIN_MAC
        if (isActive)
            setWindowFocus(mWindowHasFocus);
#endif
        
        if (IGOController::instance()->extraLogging())
            ORIGIN_LOG_ACTION << "IGOWindowManager - removeWindow - end - hasMouse = " << mWindowHasMouse << ", hasFocus = " << mWindowHasFocus;
    
        // We may have closed a pinned window - update our flags not to show toasts if there are no pinned widget
        if(pinnedWindows().count() == 0)
        {
            mShowPinCanToggleToast = false;
            mShowPinHiddenToast = false;
        }
    }
    
    else
    {
        if (IGOController::instance()->extraLogging())
            ORIGIN_LOG_ACTION << "IGOWindowManager - removeWindow - window not found = " << window;
    }
}

// We are trying to limit the number of scenarios to handle object destructions.
// Here are the 3 separate scenarios we support that would be checked in turn:
// 1) IGO wraps a UIToolkit::OriginWindow:
//      - The destroy is based on the user closing the window -> window->close() is called (or we will call it ourselves when automatically closing a window)
//      - The controller gets a notification (hopefully all those cases have a controller) and kicks off a controller->deleteLater()
//      - In the controller destructor, we call window->deleteLater()
//      - OIG will detect the deleteLater on the window object, and properly remove the associated IGOQWidget && IGOQtContainer
// 2) IGO wraps a QWidget with a controller:
//      - Call controller->deleteLater(), which should handle the proper deletion of its window
//      - Same as previously stated: OIG will detect the deleteLater on embedded window and remove the associated IGOQWidget && IGOQtContainer
// 3) The user has to directly call removeWindow(window, true) which will automatically delete the embedded widget
//      - Remember that when you call removeWindow(window, false), we are responsible for cleanup your own window (but the internal wrappers will be wiped out)
void IGOWindowManager::removeWindowAndContent(IGOQtContainer* window)
{
    window->setPinnedSate(false);
    
    bool handled = false;
    
    IGOQWidget *widget = window->widget();
    if (widget && widget->getRealParent())
    {
        Origin::UIToolkit::OriginWindow *tmpWindowObject = dynamic_cast<Origin::UIToolkit::OriginWindow *>(widget->getRealParent());
        if (tmpWindowObject)
        {
            // Fire the close signal, to properly close the web widgets
            tmpWindowObject->closeWindow();
            handled = true;
        }
    }
    
    if (!handled)
    {
        if (window->deleteEmbeddedWindowOnClose() && window->viewController())
        {
            QObject* obj = dynamic_cast<QObject*>(window->viewController());
            if (obj)
                obj->deleteLater();
        }
        
        else
            window->closeWidget(true);
    }
}

void IGOWindowManager::ensureOnScreen()
{
    ensureOnScreen(dynamic_cast<QWidget*>(this->sender()));
}

void IGOWindowManager::ensureOnScreen(IGOQtContainer* container)
{
    if(container)
    {
        // Check max size - use small margin
        static const QMargins DesktopMargins(10, 10, 10, 10);

        bool resize = false;
        QRect containerGeo = container->rect(false);
        if (containerGeo.width() > (mScreenWidth - DesktopMargins.left() - DesktopMargins.right()))
        {
            resize = true;
            containerGeo.setWidth(mScreenWidth - DesktopMargins.left() - DesktopMargins.right());
        }
        
        if (containerGeo.height() > (mScreenHeight - DesktopMargins.top() - DesktopMargins.bottom()))
        {
            resize = true;
            containerGeo.setHeight(mScreenHeight - DesktopMargins.top() - DesktopMargins.bottom());
        }
        
        if (resize)
        {
            container->resize(containerGeo.width(), containerGeo.height());
        
            if (IGOController::instance()->extraLogging())
                ORIGIN_LOG_ACTION << "IGOWindowManager - ensureOnScreen new size=(" << containerGeo.width() << "," << containerGeo.height() << ")";
        }
        
        
        QPoint curPosition = container->getPosition();
        QPoint newPosition = curPosition;
        
        // Passed the top
        if(containerGeo.y() < 0)
            newPosition = QPoint(containerGeo.x(), DesktopMargins.top());
        
        // Passed the bottom
        if(containerGeo.bottom() > (int)mScreenHeight)
            newPosition = QPoint(newPosition.x(), mScreenHeight - containerGeo.height() - DesktopMargins.bottom());
        
        // Passed the left
        if(containerGeo.x() < 0)
            newPosition = QPoint(DesktopMargins.left(), newPosition.y());
        
        // Passed the right
        if(containerGeo.right() > (int)mScreenWidth)
            newPosition = QPoint(mScreenWidth - containerGeo.width() - DesktopMargins.right(), newPosition.y());
        
        if (newPosition != curPosition)
        {
            container->setPosition(newPosition);

            if (IGOController::instance()->extraLogging())
                ORIGIN_LOG_ACTION << "IGOWindowManager - ensureOnScreen new pos=(" << newPosition.x() << "," << newPosition.y() << ")";
        }
    }
}

IGOQtContainer *IGOWindowManager::addPopupWindowImpl(QWidget *widget, const WindowProperties& settings, const WindowBehavior& behavior)
{
    IGOQtContainer *container = NULL;
    if (widget)
    {
        // The OriginWindow has a WebView, so only connect this signal, if we have a OriginWindow widget
        // we use QT's dynamic cast, because it does not rely on RTTI
        Origin::UIToolkit::OriginWindow *tmpObject = dynamic_cast<Origin::UIToolkit::OriginWindow *>(widget);
        if (tmpObject && tmpObject->webview())
            ORIGIN_VERIFY_CONNECT(tmpObject->webview(), SIGNAL(cursorChanged(int)), this, SLOT(setCursor(int)));
        
        // Copy over since we need to force a few fields
        WindowBehavior popupBehavior = behavior;
        popupBehavior.draggingSize = (behavior.disableDragging) ? 0 : 40;
        popupBehavior.allowClose = true;
        
        WindowBehavior winBehavior = validateWindowBehavior(WindowProperties(), popupBehavior);

        container = createQtContainer(widget, IViewController::WI_GENERIC, winBehavior, settings.openAnimProps(), settings.closeAnimProps());
        
        int posX = (mScreenWidth - container->widget()->width())/2;
        int posY = (mScreenHeight - container->widget()->height())/2;
        if (posY < WINDOW_INITIAL_MIN_Y)
            posY = WINDOW_INITIAL_MIN_Y;
        
        container->setX(posX);
        container->setY(posY);
        
        addWindow(container);
    }
    return container;
}

IGOQtContainer* IGOWindowManager::windowContainer(int32_t wid, bool unhideIfNecessary)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    tWindowList::const_iterator iter = mTopLevelWindows.begin();
    while (iter != mTopLevelWindows.end())
    {
        IGOQtContainer* container = (*iter);
        if (container->wid() == wid)
            return container;
        
        ++iter;
    }
    
    if (unhideIfNecessary)
    {
        iter = mAllWindows.begin();
        while (iter != mAllWindows.end())
        {
            IGOQtContainer* container = (*iter);
            if (container->wid() == wid)
            {
                addWindow(container);
                return container;
            }
            
            ++iter;
        }
    }
    
    return NULL;
}
    
void IGOWindowManager::centerWindow(IGOQtContainer* window, const QPoint& offset)
{
    QPoint pos = centeredPosition(QSize(window->width(), window->height()));
    pos += offset;
    
    window->setX(pos.x());
    window->setY(pos.y());
}
    
QPoint IGOWindowManager::nextWindowPositionAvailable()
{
    QPoint pos(mWindowOffset, mWindowOffset);
    
    mWindowOffset += WINDOW_AUTO_POSITION_INC;
    if (mWindowOffset > WINDOW_AUTO_POSITION_MAX)
        mWindowOffset = WINDOW_AUTO_POSITON_START;
    
    return pos;
}

void IGOWindowManager::updateGameInfo(uint32_t gameId, const IGOGameInfo& info)
{
    DEBUG_MUTEX_TEST(mIPCMutex); QMutexLocker ipcLocker(mIPCMutex);
    IGOGameInstances::iterator iter = mGames.find(gameId);
    if (iter != mGames.end())
        iter->second = info;
}

IGOWindowManager::IGOGameInfo IGOWindowManager::gameInfo(uint32_t gameId) const
{
    DEBUG_MUTEX_TEST(mIPCMutex); QMutexLocker ipcLocker(mIPCMutex);
    IGOGameInstances::const_iterator iter = mGames.find(gameId);
    if (iter != mGames.end())
        return iter->second;
    
    return IGOGameInfo();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
// Interface Impl

void IGOWindowManager::reset()
{
    enableUpdates(false);
    mGames.clear();
}
    
void IGOWindowManager::enableUpdates(bool enabled)
{
    mEnableUpdates = enabled;
}

bool IGOWindowManager::registerScreenListener(IScreenListener* listener)
{
    QObject* obj = dynamic_cast<QObject*>(listener);
    if (obj)
    {
        ORIGIN_VERIFY_CONNECT(this, SIGNAL(screenSizeChanged(uint32_t, uint32_t, uint32_t)), obj, SLOT(onSizeChanged(uint32_t, uint32_t)));
        return true;
    }
    
    return false;
}

QSize IGOWindowManager::screenSize() const
{
    return QSize(mScreenWidth, mScreenHeight);
}

QSize IGOWindowManager::minimumScreenSize() const
{
    return QSize(mMinIGOScreenWidth, mMinIGOScreenHeight);
}
    
QSize IGOWindowManager::maximumWindowSize() const
{
    return QSize(mScreenWidth*0.99f /*99% width*/, 100/*pixels for the clock & shadow area on the bottom area*/ + mScreenHeight - (mReservedHeights.top() + mReservedHeights.bottom()));
}

bool IGOWindowManager::isScreenLargeEnough() const
{
    return mScreenWidth >= mMinIGOScreenWidth && mScreenHeight >= mMinIGOScreenHeight;
}

bool IGOWindowManager::addWindow(Origin::UIToolkit::OriginWindow* window, const WindowProperties& settings)
{
    return addWindow(window, settings, WindowBehavior());
}

bool IGOWindowManager::addWindow(Origin::UIToolkit::OriginWindow* window, const WindowProperties& settings, const WindowBehavior& behavior)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    
    if (IGOController::instance()->extraLogging())
        ORIGIN_LOG_ACTION << "IGOWindowManager - ADD ORIGIN WINDOW = " << window;
    
    WindowBehavior winBehavior = validateWindowBehavior(settings, behavior);

    // Make sure the window doesn't have the titlebar buttons not currently supported
    Origin::UIToolkit::OriginWindow::TitlebarItems titlebarItems = window->titlebarItems();
    titlebarItems &= ~(Origin::UIToolkit::OriginWindow::Minimize | Origin::UIToolkit::OriginWindow::Maximize | Origin::UIToolkit::OriginWindow::FullScreen);
    window->setTitlebarItems(titlebarItems);

    // In setWindowProperties(), we check whether the user wants to explicitly change the pinId, which is not a common case
    // But here we always want to take the pinId into account
    IGOQtContainer* container = createContainer(window, settings.wid(), winBehavior, settings.openAnimProps(), settings.closeAnimProps());
    if (container)
    {
        bool propertiesRestored = false;
        container->setUniqueID(settings.uniqueID());
        container->setCallOrigin(settings.callOrigin());

        if (settings.restorable())
        {
            container->setRestorable(true);
            propertiesRestored = restorePinnedWidget(container);
        }
        
        if (!propertiesRestored)
        {
            WindowProperties adjustedSettings = settings;
            if (!settings.positionDefined())
                adjustedSettings.setPosition(nextWindowPositionAvailable());
            
            setWindowProperties(window, adjustedSettings);
        }
        
        return true;
    }
    
    return false;
}


bool IGOWindowManager::addWindow(QWidget* window, const WindowProperties& settings)
{
    return addWindow(window, settings, WindowBehavior());
}

bool IGOWindowManager::addWindow(QWidget* window, const WindowProperties& settings, const WindowBehavior& behavior)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    
    if (IGOController::instance()->extraLogging())
        ORIGIN_LOG_ACTION << "IGOWindowManager - ADD QWIDGET WINDOW = " << window;

    WindowBehavior winBehavior = validateWindowBehavior(settings, behavior);

    IGOQtContainer* container = NULL;
    
    // Determines whether we should use the next available "stacked" position if there is no saved preset (ie pinnable object) and
    // the user hasn't defined one in the 'settings' parameter
    bool useStackedPositioning = true;
    
    // Detect whether we need to embedded this window or this is the actual content
    IGOQWidget* qWidget = dynamic_cast<IGOQWidget*>(window);
    if (qWidget)
    {
        container = new IGOQtContainer(this);
        
        // In setWindowProperties(), we check whether the user wants to explicitly change the pinId, which is not a common case
        // But here we always want to take the pinId into account
        container->sinkQWidget(qWidget, settings.wid(), winBehavior, settings.openAnimProps(), settings.closeAnimProps());
        
        // Only override positioning if the content doesn't have an explicit layout
        useStackedPositioning = qWidget->layout() == 0;
        if (!useStackedPositioning)
        {
            // Reserved space for docked items (top/bottom bars for now)
            if (qWidget->layout() & WINDOW_ALIGN_TOP)
                mReservedHeights.setTop(qWidget->height());
            
            if (qWidget->layout() & WINDOW_ALIGN_BOTTOM)
                mReservedHeights.setBottom(qWidget->height());
        }
    }
    
    else
    {
        Origin::UIToolkit::OriginWindow* oWindow = dynamic_cast<Origin::UIToolkit::OriginWindow*>(window);
        if (oWindow)
        {
            // Make sure the window doesn't have the titlebar buttons not currently supported
            Origin::UIToolkit::OriginWindow::TitlebarItems titlebarItems = oWindow->titlebarItems();
            titlebarItems &= ~(Origin::UIToolkit::OriginWindow::Minimize | Origin::UIToolkit::OriginWindow::Maximize | Origin::UIToolkit::OriginWindow::FullScreen);
            oWindow->setTitlebarItems(titlebarItems);
        }

        // In setWindowProperties(), we check whether the user wants to explicitly change the pinId, which is not a common case
        // But here we always want to take the pinId into account
        container = createQtContainer(window, settings.wid(), winBehavior, settings.openAnimProps(), settings.closeAnimProps());
    }
    
    if (container)
    {
        addWindow(container);
        
        // Restore any pinnable widget properties
        bool propertiesRestored = false;
        container->setUniqueID(settings.uniqueID());
        container->setCallOrigin(settings.callOrigin());

        if (settings.restorable())
        {
            container->setRestorable(true);
            propertiesRestored = restorePinnedWidget(container);
        }

        if (!propertiesRestored)
        {
            // Ok, we couldn't restore anything so use any user-specified defaults
            WindowProperties adjustedSettings = settings;
            if (useStackedPositioning && !settings.positionDefined())
                adjustedSettings.setPosition(nextWindowPositionAvailable());
            
            setWindowProperties(window, adjustedSettings);
        }
        
        return true;
    }
    
    return false;
}

bool IGOWindowManager::addPopupWindow(QWidget* window, const WindowProperties& settings)
{
    return addPopupWindow(window, settings, WindowBehavior());
}

bool IGOWindowManager::addPopupWindow(QWidget* window, const WindowProperties& settings, const WindowBehavior& behavior)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    
    if (IGOController::instance()->extraLogging())
        ORIGIN_LOG_ACTION << "IGOWindowManager - ADD POPUP WINDOW = " << window;

    WindowBehavior winBehavior = validateWindowBehavior(settings, behavior);

    Origin::UIToolkit::OriginWindow* oWindow = dynamic_cast<Origin::UIToolkit::OriginWindow*>(window);
    if (oWindow)
    {
        // Make sure the window doesn't have the titlebar buttons not currently supported
        Origin::UIToolkit::OriginWindow::TitlebarItems titlebarItems = oWindow->titlebarItems();
        titlebarItems &= ~(Origin::UIToolkit::OriginWindow::Minimize | Origin::UIToolkit::OriginWindow::Maximize | Origin::UIToolkit::OriginWindow::FullScreen);
        oWindow->setTitlebarItems(titlebarItems);
    }

    IGOQtContainer* container = addPopupWindowImpl(window, settings, winBehavior);
    if (container)
    {
        setWindowProperties(window, settings);
        return true;
    }
    
    return false;
}

QWidget* IGOWindowManager::window(int32_t wid)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    tWindowList::const_iterator iter = mTopLevelWindows.begin();
    while (iter != mTopLevelWindows.end())
    {
        IGOQtContainer* container = (*iter);
        if (container->wid() == wid)
        {
            IGOQWidget* containerWidget = container->widget();
            if (containerWidget)
                return (containerWidget->getRealParent()) ? containerWidget->getRealParent() : containerWidget;
            
            return NULL;
        }
        
        ++iter;
    }

    return NULL;
}

bool IGOWindowManager::removeWindow(QWidget* window, bool autoDelete)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    
    if (IGOController::instance()->extraLogging())
        ORIGIN_LOG_ACTION << "IGOWindowManager - REMOVE WINDOW = " << window;

    IGOQtContainer* container = getEmbeddedWindowContainer(window);
    if (container)
    {
        if (autoDelete)
        {
            if (IGOController::instance()->extraLogging())
                ORIGIN_LOG_ACTION << "IGOWindowManager - USE removeWindowAndContent, IGOQtContainer = " << container;

            removeWindowAndContent(container);
        }
        
        else
        {
            if (IGOController::instance()->extraLogging())
                ORIGIN_LOG_ACTION << "IGOWindowManager - USE removeWindow(false), IGOQtContainer = " << container;

            removeWindow(container, false);
        }
        
        return true;
    }
    
    else
    {
        // The user still expects us to delete this window
        if (autoDelete)
        {
            if (IGOController::instance()->extraLogging())
                ORIGIN_LOG_ACTION << "IGOWindowManager - CALL DELETE LATER";

            window->deleteLater();
        }
    }
    
    return false;
    
}

bool IGOWindowManager::visible() const
{
    return mIGOVisible;
}

void IGOWindowManager::clear()
{
    closeAllWindows();
}

bool IGOWindowManager::setFocusWindow(QWidget* window)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    IGOQtContainer* container = getEmbeddedWindowContainer(window);
    if (container)
    {
        setWindowFocus(container);
        container->needFocusOnWidget();
        return true;
    }
    
    return false;
}

bool IGOWindowManager::setWindowFocusWidget(QWidget* window)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    IGOQtContainer* container = getEmbeddedWindowContainer(window);
    if (container)
    {
        container->needFocusOnWidget();
        return true;
    }
    
    return false;
}
    
bool IGOWindowManager::isActiveWindow(QWidget* window)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    IGOQtContainer* container = getEmbeddedWindowContainer(window);
    if (container)
        return mWindowHasFocus == container;
    
    return false;
}

bool IGOWindowManager::enableWindowFrame(QWidget* window, bool flag)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    IGOQtContainer* container = getEmbeddedWindowContainer(window);
    if (container)
    {
        if (flag)
            container->enableFrameEvents();
        else
            container->disableFrameEvents();
        
        return true;
    }
    
    return false;
}

bool IGOWindowManager::activateWindow(QWidget* widget)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    for (tWindowList::const_reverse_iterator iter = mTopLevelWindows.rbegin(); iter != mTopLevelWindows.rend(); ++iter)
    {
        IGOQtContainer* container = (*iter);
        IGOQWidget* containerWidget = container->widget();
        
        if (containerWidget->isAncestorOf(widget))
        {
            if(!container->dontStealFocus())
            {
#ifdef ORIGIN_MAC
                setWindowFocus(container, true, true);
#else
                setWindowFocus(container, true, false);
#endif
            }

            return true;
        }
    }
    
    return false;
}

bool IGOWindowManager::moveWindowToFront(QWidget* window)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    IGOQtContainer* container = getEmbeddedWindowContainer(window);
    if (container)
        return moveToFront(container);
    
    return false;
}

bool IGOWindowManager::moveWindowToBack(QWidget* window)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    IGOQtContainer* container = getEmbeddedWindowContainer(window);
    if (container)
        return moveToBack(container);
    
    return false;
}

bool IGOWindowManager::ensureOnScreen(QWidget* window)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    if (window && window->topLevelWidget())
    {
        IGOQtContainer* container = getEmbeddedWindowContainer(window);
        if (container)
            ensureOnScreen(container);
    }

    return false;
}
    
bool IGOWindowManager::setWindowProperties(QWidget* window, const WindowProperties& settings)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    IGOQtContainer* container = getEmbeddedWindowContainer(window);
    if (container)
    {
        if (settings.widDefined())
            container->setWId(settings.wid());
        
        if (settings.callOriginDefined())
            container->setCallOrigin(settings.callOrigin());

        bool maxSize = false;
        bool centeredWindow = settings.positionDefined() && settings.position() == WindowProperties::centeredPosition();
        if (settings.positionDefined())
        {
            if (!centeredWindow)
            container->setDockingPosition(mScreenWidth, mScreenHeight, settings.position(), settings.dockingOrigin(), settings.positionLocked());
        }

        if (settings.sizeDefined())
        {
            if (settings.size() == WindowProperties::maximumSize())
            {
                maxSize = true;
                maximizeContainer(container);
            }
            
            else
                container->setSize(settings.size());
        }
        
        if (centeredWindow && !maxSize)
        {
            QSize winSize(static_cast<int>(container->width()), static_cast<int>(container->height()));
			container->setDockingPosition(mScreenWidth, mScreenHeight, centeredPosition(winSize), settings.dockingOrigin(), settings.positionLocked());
        }

        if (settings.opacityDefined())
            container->setAlpha(settings.opacity());
        
        if (settings.restorableDefined())
            container->setRestorable(settings.restorable());
        
        if (settings.hoverOnTransparentDefined())
            container->setHoverOnTransparent(settings.hoverOnTransparent());

        if (!settings.openAnimProps().empty() && !settings.closeAnimProps().empty())
        {
            container->setDefaultAnims(settings.openAnimProps(), settings.closeAnimProps());
            SendWindowInitialState(container, -1, mIPC);
        }
        
        return true;
    }
    
    return false;
}

bool IGOWindowManager::windowProperties(QWidget* window, WindowProperties* properties)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    IGOQtContainer* container = getEmbeddedWindowContainer(window);
    if (container)
    {
        properties->setWId(container->wid());
        properties->setUniqueID(container->uniqueID());
        properties->setPosition(container->getPosition());
        properties->setSize(QSize(container->width(), container->height()));
        properties->setOpacity(container->alpha());
        properties->setRestorable(container->restorable());
        properties->setCallOrigin(container->callOrigin());
        
        return true;
    }
    
    return false;
}

bool IGOWindowManager::setWindowOpacity(QWidget* window, qreal opacity)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    IGOQtContainer* container = getEmbeddedWindowContainer(window);
    if (container)
    {
        container->setAlpha(opacity);
        return true;
    }
    
    return false;
}

bool IGOWindowManager::setWindowPosition(QWidget* window, const QPoint& pos, WindowDockingOrigin docking, bool locked)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    IGOQtContainer* container = getEmbeddedWindowContainer(window);
    if (container)
    {
        container->setDockingPosition(mScreenWidth, mScreenHeight, pos, docking, locked);
        return true;
    }
    
    return false;
}

bool IGOWindowManager::hideWindow(QWidget* window, bool overrideAlwaysVisible)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    IGOQtContainer* container = getEmbeddedWindowContainer(window);
    if (container)
    {
        if (overrideAlwaysVisible)
            container->setAlwaysVisible(false);

        container->show(false);
        return true;
    }
    
    return false;
}

bool IGOWindowManager::showWindow(QWidget* window, bool overrideAlwaysVisible)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    IGOQtContainer* container = getEmbeddedWindowContainer(window, true);
    if (container)
    {
        if (overrideAlwaysVisible)
            container->setAlwaysVisible(true);

        container->show(true);
        layoutWindow(container, false);
        return true;
    }
    
    return false;
}

bool IGOWindowManager::maximizeWindow(QWidget* window)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    IGOQtContainer* container = getEmbeddedWindowContainer(window);
    if (container)
    {
        maximizeContainer(container);
        return true;
    }
    
    return false;
}

bool IGOWindowManager::fitWindowToContent(QWidget* window)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    IGOQtContainer* container = getEmbeddedWindowContainer(window);
    if (container)
    {
        QWidget* content = NULL;
        if (container->widget())
            content = container->widget()->getRealParent();
        
        if (content)
            container->resize(content->width(), content->height());
        
        return true;
    }
    
    return false;
}

bool IGOWindowManager::setWindowEditableBehaviors(QWidget* window, const WindowEditableBehaviors& behaviors)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    IGOQtContainer* container = getEmbeddedWindowContainer(window);
    if (container)
    {
        if (behaviors.closeIGOOnCloseDefined())
            container->setCloseIGOOnClose(behaviors.closeIGOOnClose());

        return true;
    }

    return false;
}

bool IGOWindowManager::windowEditableBehaviors(QWidget* window, WindowEditableBehaviors* behaviors)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    IGOQtContainer* container = getEmbeddedWindowContainer(window);
    if (container)
    {
        behaviors->setCloseIGOOnClose(container->closeIGOOnClose());
        return true;
    }

    return false;
}

bool IGOWindowManager::setWindowItemFlag(QWidget* window, QWidget* item, QGraphicsItem::GraphicsItemFlag flag, bool enabled)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    IGOQtContainer* container = getEmbeddedWindowContainer(window);
    if (container)
    {
        QGraphicsProxyWidget* windowProxy = container->getProxy();
        if (windowProxy)
        {
            QGraphicsProxyWidget* itemProxy = windowProxy->createProxyForChildWidget(item);
            if (itemProxy)
            {
                itemProxy->setFlag(flag, enabled);
                return true;
            }
        }
    }
    
    return false;
}

bool IGOWindowManager::moveWindowItemToFront(QWidget* window, QGraphicsItem* item)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    IGOQtContainer* container = getEmbeddedWindowContainer(window);
    if (container)
    {
        container->setTopItem(item);
        return true;
    }
    
    return false;
}

bool IGOWindowManager::createContextMenu(QWidget* window, QMenu* menu, IContextMenuListener* listener, bool toolWindow)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    IGOQtContainer* container = getEmbeddedWindowContainer(window);
    if (container)
    {
        if (IGOController::instance()->extraLogging())
            ORIGIN_LOG_ACTION << "IGOWindowManager - createContextMenu = " << window << " for " << container;

        window->setContextMenuPolicy(Qt::CustomContextMenu);

        // set the transparency so that the background will not show (i.e., fix messed up corners)
        menu->setStyleSheet("background-color: transparent;");
        
        // Mac: Need to set the parent so that the proxy created will be parented.
        // This prevents the issue of the game losing focus when right-clicking to bring up the context menu (only happens on Mac).
        if (toolWindow)
        {
            // ie copy/paste popups used for web browser/twitch
            menu->setParent(window);
            
            // Make sure the context menu can be seen over the widget (think web browser here)
            QGraphicsProxyWidget* proxy = container->addWidget(menu);
            proxy->setZValue(10);
            
            menu->hide();
        }
        
        else
        {
            // Chat friends'actions
            menu->setParent(window, (Qt::WindowFlags)(Qt::Window | Qt::FramelessWindowHint));
            
            menu->setWindowModality(Qt::WindowModal);
        }
        
        QObject* listenerObj = dynamic_cast<QObject*>(listener);
        if (listenerObj)
        {
            ORIGIN_VERIFY_CONNECT(container, SIGNAL(focusChanged(QWidget*, bool)), listenerObj, SLOT(onFocusChanged(QWidget*, bool)));
            ORIGIN_VERIFY_CONNECT(container, SIGNAL(contextMenuDeactivate()), listenerObj, SLOT(onDeactivate()));
        }
        
        // Need to set the parent to NULL after we are done this ensures the context menu appears
        // in the expected location "MAGIC?" - BTW make sure this comes last! (REAL MAGIC!)
        menu->setParent(NULL);
        
        return true;
    }
    
    return false;
}

bool IGOWindowManager::showContextMenu(QWidget* window, QMenu* menu, WindowProperties* settings)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    IGOQtContainer* container = getEmbeddedWindowContainer(window);
    if (container)
    {
        if (IGOController::instance()->extraLogging())
            ORIGIN_LOG_ACTION << "IGOWindowManager - showContextMenu for " << container;

        QPoint cmPosition;
        if (settings && settings->positionDefined())
            cmPosition = settings->position();
        else
            cmPosition = getMousePos() - container->getPosition();
        
        addContextMenu(menu, cmPosition.x(), cmPosition.y());
        
        if (settings)
            settings->setPosition(cmPosition); // don't support docking for context menus
        
        return true;
    }
    
    return false;
}
    
bool IGOWindowManager::hideContextMenu(QWidget* window)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    IGOQtContainer* container = getEmbeddedWindowContainer(window);
    if (container)
    {
        if (IGOController::instance()->extraLogging())
            ORIGIN_LOG_ACTION << "IGOWindowManager - hideContextMenu for " << container;

        container->removeContextMenu();
        return true;
    }
    
    return false;
}

bool IGOWindowManager::showContextSubMenu(QWidget* window, QMenu* menu)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    IGOQtContainer* container = getEmbeddedWindowContainer(window);
    if (container)
    {
        if (IGOController::instance()->extraLogging())
            ORIGIN_LOG_ACTION << "IGOWindowManager - showContextSubMenu = " << menu << " for " << container;

        container->addContextSubmenu(menu);
        return true;
    }
    
    return false;
}

bool IGOWindowManager::hideContextSubMenu(QWidget* window)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    IGOQtContainer* container = getEmbeddedWindowContainer(window);
    if (container)
    {
        if (IGOController::instance()->extraLogging())
            ORIGIN_LOG_ACTION << "IGOWindowManager - hideContextSubMenu for " << container;

        container->removeContextSubmenu();
        return true;
    }
    
    return false;
}

int32_t IGOWindowManager::windowID(const IViewController* const controller)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    for (tWindowList::iterator iter = mTopLevelWindows.begin(); iter != mTopLevelWindows.end(); ++iter)
    {
        IGOQtContainer* container = (*iter);
        if (container->viewController() == controller)
            return container->wid();
    }

    return IViewController::WI_INVALID;
}

IViewController* IGOWindowManager::windowController(QWidget* window)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    IGOQtContainer* container = getEmbeddedWindowContainer(window);
    if (container)
        return container->viewController();
    
    return NULL;
}

void* IGOWindowManager::nativeWindow(QWidget* window)
{
    DEBUG_MUTEX_TEST(mTopLevelWindowsMutex); QMutexLocker locker(mTopLevelWindowsMutex);
    IGOQtContainer* container = getEmbeddedWindowContainer(window);
    if (container)
        return (void*)container->view()->winId();
    
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IIGOWindowManager::ICallback::~ICallback() {}
IIGOWindowManager::IContextMenuListener::~IContextMenuListener() {}
IIGOWindowManager::IWindowListener::~IWindowListener() {}
IIGOWindowManager::IScreenListener::~IScreenListener() {}
IIGOWindowManager::~IIGOWindowManager() {}
    
} // Engine
} // Origin

