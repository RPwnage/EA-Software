///////////////////////////////////////////////////////////////////////////////
// IGOQContainer.h
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef _IGOQtContainer_H_
#define _IGOQtContainer_H_

#include <QMutex>
#include <QGraphicsScene>
#include <QGraphicsItem>

#include "engine/igo/IGOQWidget.h"
#include "engine/igo/IGOController.h"

class QWidget;
class QGraphicsProxyWidget;
class QGraphicsView;
class QMouseEvent;

namespace Origin
{
namespace Engine
{

class IGOQtContainer : public QGraphicsScene
{
Q_OBJECT

public:

    IGOQtContainer(IGOWindowManager* igowm);
    virtual ~IGOQtContainer();

    // we use these next two methods to immediately detect that the object is invalid (since the deleteLater/QDeferredDelete event will only be processed in the next event loop)
    void postDeletion();
    bool pendingDeletion() const;
    
    void sinkQWidget(IGOQWidget *w, int32_t wid, const IIGOWindowManager::WindowBehavior& settings, const IIGOWindowManager::AnimPropertySet& openAnimProps, const IIGOWindowManager::AnimPropertySet& closeAnimProps);
    bool isModal() { return mBehavior.modal; }
    QGraphicsProxyWidget* getProxy() const { return mProxy; }
    
    // Set connection-specific information
    uint32_t connectionId() const { return mConnectionId; }
    void setConnectionId(uint32_t connectionId) { mConnectionId = connectionId; }
    
    // Indicates whether the widget should be closed when the IGO is closed.
    bool closeOnIgoClose(){return mBehavior.closeOnIGOClose;}

    // Indices whether we should close IGO when we close the window
    bool closeIGOOnClose() const { return mBehavior.closeIGOOnClose; }
    void setCloseIGOOnClose(bool flag) { mBehavior.closeIGOOnClose = flag; }

#ifdef ORIGIN_MAC
    void sendMousePos(QPoint pt, const QPoint &ptOffset, const QPointF &ptScaling, uint64_t timestamp);
#else
    void sendMousePos(const QPoint &pt, const QPoint &ptOffset, const QPointF &ptScaling);
#endif
    void sendMouseButtons(const Qt::MouseButtons &buttons, bool isDoubleClick = false);
    void sendMouseWheel(int wheelVerticalPos, int wheelHorizontalPos);
    void sendDoubleClick();

#if !defined(ORIGIN_MAC)
    void sendKeyEvent(uint type, uint keyCodeWParam, uint keyCodeLParam);
#else
    void sendKeyEvent(QByteArray keyData);
#endif

    // Used to manage windows that are singleton (ie only one instance can exist at once)
    bool singleton() const { return mBehavior.singleton; }
    
    // Specify whether a window is persistent across games (for example the desktop elements: top/bottom bars)
    bool isPersistent() const { return mBehavior.persistent;}
    
    // Specify whether to delete the actual embedded window when we close the IGO window
    bool deleteEmbeddedWindowOnClose() const { return !mBehavior.doNotDeleteOnClose; }
    
    // Used to maintain a reference to the controller of the actual view (real parent)
    IViewController* viewController() const { return mBehavior.controller; }
    
    // Set/Get the animation "definition" (props) for when we open/close windows
    void setDefaultAnims(const IIGOWindowManager::AnimPropertySet& openAnimProps, const IIGOWindowManager::AnimPropertySet& closeAnimProps);
    const IIGOWindowManager::AnimPropertySet& openAnimProps() const { return mOpenAnimProps; }
    const IIGOWindowManager::AnimPropertySet& closeAnimProps() const { return mCloseAnimProps; }

    // Set/Get creation origin
    void setCallOrigin(IIGOCommandController::CallOrigin callOrigin) { mCallOrigin = callOrigin; }
    IIGOCommandController::CallOrigin callOrigin() const { return mCallOrigin; }

    // API to manage whether we have sent the initial window state for a specific connection
    bool initialStateBroadcast(uint32_t connectionId) const { return eastl::find(mInitialStateBroadcast.begin(), mInitialStateBroadcast.end(), connectionId) != mInitialStateBroadcast.end(); }
    void setInitialStateBroadcast(uint32_t connectionId) { mInitialStateBroadcast.push_back(connectionId); }
    void resetInitialStateBroadcast(uint32_t connectionId) { mInitialStateBroadcast.erase(eastl::remove(mInitialStateBroadcast.begin(), mInitialStateBroadcast.end(), connectionId), mInitialStateBroadcast.end()); }
    void clearInitialStateBroadcast() { mInitialStateBroadcast.clear(); }

    // Is this window using open/close animations?
    bool useAnimations() const { return mUseAnimations; }

    bool contains(const QPoint &pt, int32_t dx1 = 0, int32_t dy1 = 0, int32_t dx2 = 0, int32_t dy2 = 0);
    bool isDirty() { return mDirty; }
    void setDirty(bool dirty) { mDirty = dirty; if (!mDirty) mFlags = 0; }
    uint id() { return mId; }
    uint flags() { return mFlags; }

#ifdef ORIGIN_MAC
    QPoint getLastClickLocalCoords() const { return mMouseClickPos; }
#endif
    
    const uchar *pixelData() const;
    uint pixelDataSize();

    QRect &rect(bool boundingBoxOnly);
    void setX(const int x);
    void setY(const int y);
    void setDockingPosition(uint32_t screenWidth, uint32_t screenHeight, const QPoint& pos, IIGOWindowManager::WindowDockingOrigin docking, bool locked);
    void layoutDockingPosition(uint32_t screenWidth, uint32_t screenHeight);
    QPoint getPosition() { return mPosition; }
    void resize(uint w, uint h, bool redraw = true);
    void resizeView(int w, int h);
    void addContextMenu(QWidget *menu, int x, int y);
    void addContextSubmenu(QMenu* submenu);

    void show(bool visible);
    bool isVisible() { if (mWidget) { return mWidget->isVisibleInIGO(); } else { return false; } }
    bool isAlwaysVisible() { return mBehavior.alwaysVisible; }
    void setAlwaysVisible(bool alwaysVisible) { mBehavior.alwaysVisible = alwaysVisible; }
    bool isPinned();

    bool visibleBeforeGameSwitch() const { return mVisibleBeforeGameSwitch; }
    void setVisibleBeforeGameSwitch(bool flag) { mVisibleBeforeGameSwitch = flag; }
    
    // Use this property to detect a change in the visibility of the widget from one update to the next (to handle pinned widgets while OIG is off/prevent
    // unnecessary updates)
    bool updateVisibility() const { return mUpdateVisibility; }
    void setUpdateVisibility(bool vis) { mUpdateVisibility = vis; }

    int32_t wid() const;
    void setWId(int32_t wid);
    const QString& uniqueID() const { return mUniqueID; }
    void setUniqueID(const QString& uniqueID) { mUniqueID = uniqueID; }
    bool isPinnable() const { return mBehavior.pinnable; }
    bool isPinningInProgress() { return mPinningInProgress; }
    bool isPinningLoadStoreInProgress() { return mLoadStoreInProgress; }
    bool isRenderingBlockedWhilePinning() { return !mBehavior.dontblockRenderingWhilePinningInProgress; }
    void setPendingPinnedState(bool isPending);
    bool isPendingPinnedState() { return mIsPinningPendingState; }
    void setPinnedSate(bool pinned);
    void setHoverOnTransparent(const bool& hoverOnTransparent) { mHoverOnTransparent = hoverOnTransparent; }
    uint8_t pinnedAlpha();
    
    // Methods related to the storage/restore of the window properties to disk
    bool restorable() const { return mRestorable; }
    void setRestorable(bool flag) { mRestorable = flag; }
    void storeWidget(const QString& productId = "");
    bool restoreWidget();


    void setIGOVisible(bool visible);
    uint8_t alpha() { return mAlpha; }
    void setFocus(bool s);
    void removeFocus();
    QGraphicsView* view() const { return mView; }

    bool isMouseGrabbing();
    bool isMouseGrabbingOrClicked();
    bool dontStealFocus() const { return mBehavior.dontStealFocus; }
    bool alwaysOnTop() const { return mBehavior.alwaysOnTop; }

    void setLayout(uint layout);
    uint layout() { return mLayout; }

    // set the location of our dragging area
    void setDraggingArea(QRect &area);        // custom dragging area
    void setDraggingTop(uint height);        // dragging area at the top of the widget
    void registerTitlebarButton(QObject* btn);  // register widget as button we need to skip when determining whether we can move the window around
    
    // set the size of the resizing area. Allows horizontal and vertical resizing to be controlled independently.
    // param: nsSize - vertical resize border height
    // param: ewSize - horizontal resize border width
    void setResizingBorder(int nsSize, int ewSize);
    QSize resizingBorder() const {return QSize(mEwSizingBorder, mNsSizingBorder);}

    // set an outer margin used to detect resizing/moving when the art itself offsets the content
    void setOuterMargins(int left, int top, int right, int bottom);
    
    uint mouseCursor() { return mCursor; }
    void setCursor(uint cursor) { mCursor = cursor; }
    void lockWindow();
    void unlockWindow();

    IGOQWidget *widget() { return mWidget; }
    void invalidate();
    
    bool drawWindow();

#if defined(ORIGIN_PC)
    void setWindowStateFlags();
#endif

public slots:
    void removeContextMenu();
    void removeContextSubmenu();
    void setPosition(const QPoint &);
    void setSize(const QSize &);
    void setAlpha(qreal);
    void updatePinnedAlpha();
    void pinningFinished();
    void pinningStarted();
    void pinningToggled();
    void updateWindow(uint);
    void pinButtonClicked();
    void closeWidget(bool deleteWidget);

    // deal with request focus on widget
    void needFocusOnWidget();
    void destroyWidget();
    void onWidgetDestroyed();

    bool deleteOnLosingFocus() const { return mBehavior.deleteOnLosingFocus; }
    
    // web browser tabs
    void setTopItem(QGraphicsItem* item);

    void enableFrameEvents();
    void disableFrameEvents();

signals:
    void focusLost();
    void moving();
    void resizing();

private:

    enum IGOSizing 
    {
        SIZING_NONE = 0x0000,

        SIZING_LEFT = 0x0001,
        SIZING_RIGHT = 0x0002,
        SIZING_TOP = 0x0004,
        SIZING_BOTTOM = 0x0008,
    };

    bool isTransparentPixel(const QPoint &pt);
    void setSizing(const bool& isResizing);
    void setMoving(const bool& isMoving);

    void sendMouseEvent(QGraphicsSceneMouseEvent& event);
    void sendMouseEvent(QGraphicsSceneWheelEvent& event);

public slots:
    void repaintRequestedSlot(const QList<QRectF> &);
private slots:
    void dirtyWidget();
    void pinButtonFocusChanged(bool focus);
    void pinButtonDeactivated();
    void onIgoVisibleChanged(bool visible, bool);
    void onScreenSizeChanged(uint32_t width, uint32_t height, uint32_t connectionId);

signals:
    void layoutChanged();
    void cursorChanged(uint cursor);
    void windowDirty(IGOQtContainer *);
    void pinningStateChanged(bool isPinningInProgress);
    void pinningToggled(IGOQtContainer *);
    void focusChanged(QWidget* window, bool hasFocus);
    void contextMenuDeactivate();

private:
    virtual bool eventFilter(QObject *obj, QEvent *event);
#ifdef ORIGIN_MAC
    bool filterMouseMoveEvent(uint64_t timestamp, bool isMoving, bool isResizing);
#endif
    
    IGOWindowManager* mWindowManager;
    
    QGraphicsProxyWidget *mProxy;
    QGraphicsView *mView;
    QMutex mWindowMutex;
    IGOQWidget *mWidget;
    QWidget *mContextMenu;
    QWidget* mSubmenu;

    IPinButtonViewController* mPinButton;
    QPointer<QObject> mPinButtonGuard;  // We need a QPointer here, since the parent of the pin button is the window, which may have
                                        // been destroyed before the IGOQtContainer
    IIGOWindowManager::WindowBehavior mBehavior;
    IIGOCommandController::CallOrigin mCallOrigin;

    QString mUniqueID;
    IIGOWindowManager::AnimPropertySet mOpenAnimProps;
    IIGOWindowManager::AnimPropertySet mCloseAnimProps;

    eastl::vector<uint32_t> mInitialStateBroadcast;    // set of connections for which we have sent the initial window state

    QImage *mImageData;
    uint mTS;
    Qt::MouseButtons mPrevMouseButtonState;
    Qt::MouseButtons mCurrentMouseButtonState;
    QPoint mMouseClickPos;        enum FrameTransparency
    {
        Transparency,
        NoTransparency
    };
    
    QPoint mCurrentMousePos;
    QPoint mCurrentMousePosOffset;
    QPointF mCurrentMousePosScaling;
    QPoint mPosition;
    QPoint mDownMousePosition;
    QPoint mOriginalWindowPosition;
    QSize mOriginalWindowSize;
    QPoint mDockingPosition;
    IIGOWindowManager::WindowDockingOrigin mDockingOrigin;
#ifdef ORIGIN_MAC
    uint64_t mLastProcessDelta;
    uint64_t mLastProcessTimestamp;
    uint64_t mLastMouseMoveTimestamp;
#endif
    uint8_t mAlpha;
    uint mId;
    uint mLayout;
    uint mFlags;
    uint mCursor;
    QRect mDraggingArea;
    uint mSizingEdge;
    // Size of vertical resize border. 0 = no resizing.
    int mNsSizingBorder;
    // Size of horizonal resize border. 0 = no resizing.
    int mEwSizingBorder;
    int32_t mOuterTopMargin;
    int32_t mOuterLeftMargin;
    int32_t mOuterRightMargin;
    int32_t mOuterBottomMargin;
    uint32_t mConnectionId;
    
    bool mDisableFrameEvents;
    bool mUpdateVisibility;
    bool mLocked;
    bool mDirty;
    bool mForcedRedraw;
    bool mMouseInWidget;
    bool mHasFocus;
    bool mUseLayout;
    bool mCustomDraggingArea;
    bool mSizing;
    bool mMoving;
    bool mResizingHasAlreadyStarted;
    bool mLoadStoreInProgress;
    bool mPinningInProgress;
    bool mIsPinningPendingState;
    bool mIsRenderingBlockedWhilePinning;
    bool mRestorable;
    bool mDeferredDelete;
    bool mVisibleBeforeGameSwitch;
    bool mNoWindowDraggingWhileMouseOverButton;
    bool mUseAnimations;
    bool mHoverOnTransparent;
    bool mPositionLocked;
    
    QGraphicsItem* mTopItem; // the item that is drawn on top in the graphics scene
};

} // Engine
} // Origin

#endif // _IGOQtContainer_H_
