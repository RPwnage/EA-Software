///////////////////////////////////////////////////////////////////////////////
// IGOQContainer.cpp
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGOQtContainer.h"

#include <QtCore/QEvent>
#include <QtCore/QDebug>
#include <QtCore/QStack>
#include <QtWidgets/QLabel>
#include <QImage>
#include <QSlider>

#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QGraphicsView>
#include <QtWidgets/QGraphicsSceneMouseEvent>

#include <QApplication>
#include <QAction>
#include <QMenu>

#include "IGOTrace.h"
#include "IGOIPC/IGOIPC.h"
#include "IGOWindowManager.h"

#include "services/log/LogService.h"
#include "services/debug/DebugService.h"

#include "OriginWindow.h"
#include "origintitlebarbase.h"


#if defined(ORIGIN_PC)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// #define USE_MOUSE_EVENTS // Used to compare PC vs Mac way of forwarding mouse events to Qt

#include <windows.h>
#include <QWindow>

#elif defined(ORIGIN_MAC)
#include "MacWindows.h"
#endif

#if defined(ORIGIN_MAC)
#include "IGOMacHelpers.h"
#endif

bool qt_sendSpontaneousEvent(QObject *receiver, QEvent *event)
{
	if(!receiver)
		return false;
	
	return QCoreApplication::sendSpontaneousEvent(receiver, event);

}

namespace Origin
{
namespace Engine
{
#if defined(_DEBUG)
const uint DEBUG_COLOR = 0x55000055;
#endif

static uint gGlobalIdGenerator = 1;

static const char* OIG_TITLEBAR_BUTTON_KEYWORD = "OIGTitlebarButton";
    
IGOQtContainer::IGOQtContainer(IGOWindowManager* igowm) :
	QGraphicsScene(),
    mWindowManager(igowm),
	mProxy(NULL),
	mView(NULL),
	mWidget(NULL),
	mContextMenu(NULL),
    mSubmenu(NULL),
    mPinButton(NULL),
    mCallOrigin(IIGOCommandController::CallOrigin_CLIENT),
	mImageData(NULL),
	mTS(0),
	mPrevMouseButtonState(Qt::NoButton),
	mCurrentMouseButtonState(Qt::NoButton),
	mMouseClickPos(0, 0),
	mCurrentMousePos(0, 0),
	mCurrentMousePosOffset(0, 0),
    mCurrentMousePosScaling(0, 0),
	mPosition(0, 0),
    mDockingPosition(0, 0),
    mDockingOrigin(IIGOWindowManager::WindowDockingOrigin_DEFAULT),
#ifdef ORIGIN_MAC
    mLastProcessDelta(0),
    mLastProcessTimestamp(0),
    mLastMouseMoveTimestamp(0),
#endif
	mAlpha(255),
	mLayout(IIGOWindowManager::WINDOW_ALIGN_NONE),
	mFlags(IGOIPC::WINDOW_UPDATE_NONE),
	mCursor(0),
	mDraggingArea(0,0,0,0), // zero means no dragging are, so we cannot drag the window
	mSizingEdge(IGOQtContainer::SIZING_NONE),
	mNsSizingBorder(0), // zero means no vertical resizing possible
	mEwSizingBorder(0), // zero means no horizontal resizing possible
    mOuterTopMargin(0),
    mOuterLeftMargin(0),
    mOuterRightMargin(0),
    mOuterBottomMargin(0),
    mConnectionId(0),
    mDisableFrameEvents(false),
    mUpdateVisibility(false),
	mLocked(false),
	mDirty(true),
	mMouseInWidget(false),
	mHasFocus(false),
	mUseLayout(false),
	mCustomDraggingArea(false),
	mSizing(false),
	mMoving(false),
    mResizingHasAlreadyStarted(false),
    mLoadStoreInProgress(false),
    mPinningInProgress(false),
	mIsPinningPendingState(false),
    mRestorable(false),
    mDeferredDelete(false),
    mVisibleBeforeGameSwitch(false),
    mNoWindowDraggingWhileMouseOverButton(false),
    mUseAnimations(false),
    mHoverOnTransparent(false),
    mPositionLocked(false),
    mTopItem(NULL)
{
	gGlobalIdGenerator++;
	
	if(gGlobalIdGenerator<=1) // 0 reserved ID for a special background window
							 // 1 reserved ID for debug information display
        gGlobalIdGenerator=2;

	mId = gGlobalIdGenerator;

	ORIGIN_VERIFY_CONNECT(this, SIGNAL(changed(const QList<QRectF> &)), this, SLOT(repaintRequestedSlot(const QList<QRectF> &)));

    if (IGOController::instance()->extraLogging())
        ORIGIN_LOG_ACTION << "IGOQtContainer - NEW = " << this;
}

IGOQtContainer::~IGOQtContainer()
{
    if (IGOController::instance()->extraLogging())
        ORIGIN_LOG_ACTION << "IGOQtContainer - DELETE = " << this;

    // reset Qt's focus context, othwerwise we may crash ( https://developer.origin.com/support/browse/EBIBUGS-24045 )
    if (this->hasFocus())
    {
        this->clearFocus();
    }

    if(mWidget && mWidget->hasFocus())
    {
        mWidget->clearFocus();
    }
   
    if (mView)
	{
        mView->removeEventFilter(this);
		mView->deleteLater();
		mView = NULL;
	}

    // mProxy will be destroyed when we kill the graphics scene object IGOQtContainer
    mProxy = NULL;
	
	if (mImageData)
	{
		delete mImageData;
		mImageData = NULL;
	}

	if (mWidget)
	{
		ORIGIN_VERIFY_DISCONNECT(mWidget, SIGNAL(igoCloseWidget(bool)), this, SLOT(closeWidget(bool)));
		ORIGIN_VERIFY_DISCONNECT(mWidget, SIGNAL(igoMove(QPoint)), this, SLOT(setPosition(QPoint)));
		ORIGIN_VERIFY_DISCONNECT(mWidget, SIGNAL(igoDestroy()), this, SLOT(destroyWidget()));
		ORIGIN_VERIFY_DISCONNECT(mWidget, SIGNAL(destroyed()), this, SLOT(onWidgetDestroyed()));       
        mWidget->storeRealParent(NULL);
        
        // Make sure to call deleteLater here since the deletion of the container may have been triggered from the IGOQWidget (when the embedded object
        // closed/try to delete itself)
        mWidget->deleteLater();
		mWidget = NULL;
	}

    // it's a child of the titlebar, no deletion needed
    mPinButton = NULL;

    // Should we close IGO because the user closed this window?
    if (mBehavior.closeIGOOnClose)
        QMetaObject::invokeMethod(mWindowManager, "onGameSpecificWindowClosed", Qt::QueuedConnection, Q_ARG(uint32_t, mConnectionId));
}

void IGOQtContainer::postDeletion()
{
    mDeferredDelete = true;
    deleteLater();
}

bool IGOQtContainer::pendingDeletion() const
{
    return mDeferredDelete;
}
    
void IGOQtContainer::setLayout(uint flags)
{
	mLayout = flags;

	emit (layoutChanged());
}

bool IGOQtContainer::isMouseGrabbing()
{
	return !(mLayout&IGOWindowManager::WINDOW_ALIGN_LEFT && mLayout&IGOWindowManager::WINDOW_ALIGN_RIGHT
            && mLayout&IGOWindowManager::WINDOW_ALIGN_TOP && mLayout&IGOWindowManager::WINDOW_ALIGN_BOTTOM)
    && (mMoving || mSizing);

}

// This version of the 'isMouseGrabbing' method should be used by the window manager to detect whether the mouse is currently in use by the window.
// This is to help with scrolling (otherwise while scrolling and then switching to a different window, we would send an "invalid" mouse event/outside the window
// and the scrollbar would potentially "teleport" to a new position) and a more general/safe way in case of yet to discover cases : don't lose the focus while dragging the cursor!
bool IGOQtContainer::isMouseGrabbingOrClicked()
{
	return !(mLayout&IGOWindowManager::WINDOW_ALIGN_LEFT && mLayout&IGOWindowManager::WINDOW_ALIGN_RIGHT
             && mLayout&IGOWindowManager::WINDOW_ALIGN_TOP && mLayout&IGOWindowManager::WINDOW_ALIGN_BOTTOM)
    && (mMoving || mSizing || ((mCurrentMouseButtonState & (Qt::LeftButton | Qt::RightButton)) != 0));
    
}

void IGOQtContainer::repaintRequestedSlot(const QList<QRectF> &regions)
{
	if (!mWidget || mLocked) {
		return;
	}

	mDirty = true;
	mFlags |= IGOIPC::WINDOW_UPDATE_BITS;
	emit windowDirty(this);
}

void IGOQtContainer::closeWidget(bool deleteWidget)
{
    if (mPinButtonGuard)
        mPinButton->hideSlider();

	mWindowManager->removeWindow(this, mBehavior.doNotDeleteOnClose==true ? false : deleteWidget);
}

void IGOQtContainer::dirtyWidget()
{
	mDirty = true;
	mFlags |= IGOIPC::WINDOW_UPDATE_ALL;
	emit windowDirty(this);
}

void IGOQtContainer::destroyWidget()
{
	// The IGOQWidget object requested that we destroy it. At that point we don't need to look at the mBehavior.doNotDeleteOnClose since we are
    // destroying the object we are wrapping - otherwise we may access the proxy and get a nice kaboom
	ORIGIN_VERIFY_DISCONNECT(mWidget, SIGNAL(destroyed()), this, SLOT(onWidgetDestroyed()));
	mWindowManager->removeWindow(this, true);
}

void IGOQtContainer::onWidgetDestroyed()
{
	// The IGOQWidget object was destroyed through some other means. At that point we don't need to look at the mBehavior.doNotDeleteOnClose since we are
    // destroying the object we are wrapping - otherwise we may access the proxy and get a nice kaboom

	mWidget = NULL;
	mWindowManager->removeWindow(this, true);
}

void IGOQtContainer::setTopItem( QGraphicsItem* item )
{
    if(mTopItem)
    {
        mTopItem->setZValue(0); // lower the current webview
        mTopItem->setVisible(false);
    }
    mTopItem = item;
    mTopItem->setZValue(1); // raise the current webview
    mTopItem->setVisible(true);
    addItem(item); // function does nothing if 'item' already exists in the scene
}

bool IGOQtContainer::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type())
	{
        case QEvent::Enter:
            // Is this an event for a title button getting/losing focus (pin button, close, etc...)?
            // This is so that we can disable window dragging
            if (obj->property(OIG_TITLEBAR_BUTTON_KEYWORD).isValid())
                mNoWindowDraggingWhileMouseOverButton = true;
            break;
                
        case QEvent::Leave:
            // Is this an event for a title button getting/losing focus (pin button, close, etc...)?
            // This is so that we can disable window dragging
            if (obj->property(OIG_TITLEBAR_BUTTON_KEYWORD).isValid())
                mNoWindowDraggingWhileMouseOverButton = false;
            break;
            
        case QEvent::WindowDeactivate:
        if (obj == mView)
        {
            emit focusLost();

            if (mPinButtonGuard)
                mPinButton->hideSlider();
        }

        break;

        case QEvent::Resize:
            resize(mWidget->width(), mWidget->height());

            if (mPinButtonGuard)
                mPinButton->hideSlider();

            break;

		case QEvent::Close:
            if (mPinButtonGuard)
                mPinButton->hideSlider();

            if (obj == mSubmenu)
				mSubmenu = NULL;
            else
			if (obj == mContextMenu)
            {
				mContextMenu = NULL;
				mSubmenu = NULL;
            }
			break;
            
        default:
            break;

	}

	return QGraphicsScene::eventFilter(obj, event);

}

bool IGOQtContainer::drawWindow()
{
	// should only be drawn in IGOWindowManager::redrawAllWindows
	if(mView)
	{
		// QImage::Format_ARGB32 is not used, because it breaks font anti-aliasing in IGO!!!
		if(mImageData)
		{
			if(mImageData->size() != mView->viewport()->size())
			{
				delete mImageData;
				mImageData = new QImage(mView->viewport()->size(), QImage::Format_ARGB32_Premultiplied);
			}
		}
		else
			mImageData = new QImage(mView->viewport()->size(), QImage::Format_ARGB32_Premultiplied);

		if (mImageData)
		{
			#ifdef _DEBUG
				mImageData->fill(DEBUG_COLOR);
			#else
				mImageData->fill(0);
			#endif

			QPainter imagePainter(mImageData);
            mView->render(&imagePainter);
			imagePainter.end();
		}
		else
			return false;

	//	save to a JPG file
	//	static int imageIndex=0;
	//	imageIndex++;
	//	QString fileName = "image%1.jpg";
	//	fileName=fileName.arg(imageIndex);
	//	mImageData->save(fileName, "JPEG");

	}

	return true;
}

#if defined(ORIGIN_PC)
void IGOQtContainer::setWindowStateFlags()
{
	if( mView )
	{
        // EBIBUGS-25746, EBIBUGS-24629
        // To prevent IGO windows showing up outside of the IGO during monitor switching.
        // In Qt5, the windowing system was refactored such that a QWidget contains a QWindow, the actual class representing
        // the native window. During a monitor switch, the underlying QWindow gets destroyed and created again.
        // Previously, IGO windows were made invisible on the desktop and taskbar by setting the native window states directly using
        // Windows calls. Now, we set the window states through the QWindow interface.

		// NOTE 1: QWidget::winId() creates a QWindow if one has not been created.
		//   The creation of a QWindow is necessary for setting the window states.
        // NOTE 2: As noted in the Qt5 docs (http://qt-project.org/doc/qt-5/qwidget.html#windowHandle),
        //   QWidget::windowHandle() is "under development and is subject to change." So this may need to be revisited in the future.
        mView->winId(); // to create the QWindow
        mView->windowHandle()->setFlags(Qt::Tool);
        mView->windowHandle()->setOpacity(0);
	}
}
#endif

void IGOQtContainer::setDefaultAnims(const IIGOWindowManager::AnimPropertySet& openAnimProps, const IIGOWindowManager::AnimPropertySet& closeAnimProps)
{
    mOpenAnimProps = openAnimProps;
    mCloseAnimProps = closeAnimProps;
    mUseAnimations = !openAnimProps.empty() || !closeAnimProps.empty();
    
    clearInitialStateBroadcast();

}
    
void IGOQtContainer::sinkQWidget(IGOQWidget *w, int32_t wid, const IIGOWindowManager::WindowBehavior& settings, const IIGOWindowManager::AnimPropertySet& openAnimProps, const IIGOWindowManager::AnimPropertySet& closeAnimProps)
{
    if (IGOController::instance()->extraLogging())
        ORIGIN_LOG_ACTION << "IGOQtContainer - SINK = " << this << ", ID = " << wid << ", IGOQWidget = " << w;

	if (w)
	{
        mWidget = w;
        
        mBehavior = settings;
        mOpenAnimProps = openAnimProps;
        mCloseAnimProps = closeAnimProps;
        mUseAnimations = !openAnimProps.empty() || !closeAnimProps.empty();

        if (settings.draggingSize)
            setDraggingTop(settings.draggingSize);
        if (settings.nsBorderSize || settings.ewBorderSize)
            setResizingBorder(settings.nsBorderSize, settings.ewBorderSize);
        if (settings.nsMarginSize || settings.ewMarginSize)
            setOuterMargins(settings.ewMarginSize, settings.nsMarginSize, settings.ewMarginSize, settings.nsMarginSize);
        
        mView = new QGraphicsView(this);
        mView->installEventFilter(this);
        mView->setContentsMargins(0,0,0,0);
		mView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
		mView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		mView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		mView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		setItemIndexMethod(QGraphicsScene::NoIndex);
        mView->setProperty("OIG", true);

#if defined(ORIGIN_MAC)
        mView->setStyleSheet("QGraphicsView {border-style: none; }");

        Qt::WindowFlags windowFlags = Qt::FramelessWindowHint;
        if (settings.dontStealFocus)
            windowFlags |= Qt::WindowDoesNotAcceptFocus;
    
        mView->setWindowFlags(windowFlags);
#endif
        if(settings.pinnable)
        {
            setWId(wid);
            
            mPinButton = IGOController::instance()->uiFactory()->createPinButton(w->getRealParent());
            if (mPinButton)
            {
                setPinnedSate(false);	// default to unpinned
                mPinButton->setSliderValue(76.0f * 2.55f); // 75 % opacity per default (76 so it will round up)
                
                // This isn't "pretty" because of the separation between the interface and QObjects, but I guess it's good enough!
                mPinButtonGuard = dynamic_cast<QObject*>(mPinButton);
                if (mPinButtonGuard)
                {
                    ORIGIN_VERIFY_CONNECT(mPinButtonGuard, SIGNAL(clicked()), this, SLOT(pinButtonClicked()));
                    ORIGIN_VERIFY_CONNECT(mPinButtonGuard, SIGNAL(pinningFinished()), this, SLOT(pinningFinished()));
                    ORIGIN_VERIFY_CONNECT(mPinButtonGuard, SIGNAL(pinningStarted()), this, SLOT(pinningStarted()));
                    ORIGIN_VERIFY_CONNECT(mPinButtonGuard, SIGNAL(pinningToggled()), this, SLOT(pinningToggled()));
                    ORIGIN_VERIFY_CONNECT(mPinButtonGuard, SIGNAL(sliderChanged(const int&)), this, SLOT(updatePinnedAlpha()));

                    // Get notifications when the user is above the button so that we can disable dragging at that point
                    registerTitlebarButton(mPinButtonGuard);
                    
                    ORIGIN_VERIFY_CONNECT(mWindowManager, SIGNAL(igoVisible(bool, bool)), this, SLOT(onIgoVisibleChanged(bool, bool)));
                }
            }
        }

		mProxy = addWidget(w);
		mProxy->setCacheMode(settings.cacheMode);
		
		// squish helper
		mView->setObjectName(mWidget->objectName());	// set the object name to identify our object in squish
		this->setObjectName(mWidget->objectName());

        ORIGIN_ASSERT(mProxy);
		
		if (!settings.doNotDeleteOnClose)
			mWidget->setAttribute(Qt::WA_DeleteOnClose, true);
		
		setLayout(w->layout());
		
		// This is not a bug.  We set the flags initially so when we call show(), the window is not popped up
#if defined(ORIGIN_PC)
		setWindowStateFlags();
#endif
        //change coordinates, so that every IGO window goes from 0,0 to width,height and not real screen coordinates
		w->move(0,0);

#ifdef ORIGIN_MAC
        // Move the view off-screen first because we have to show the window before being able to change its style to be transparent, so as
        // to avoid a quick 'flash' onscreen otherwise
        mView->move(-4096, -4096);
        mView->show();
        
        // Need to investigate why later (in a hurry/this would require me to rebuild Qt5/this wasn't happening with my local version of Qt5 -> new behavior because of a patch?)
        // Or maybe this is some "feedback" from the OriginWindow/uitoolkit event handling
        // Anyways the show() resizes the actual wrapped widget's size -> reset it appropriately
        // Same goes for the positiom - for example showing the PDLC/Simcity window moves it relative to their parent, which is a NO NO
        if (w->getRealParent())
        {
            w->getRealParent()->resize(w->sizeHint());
            w->getRealParent()->move(0, 0);
        }
        
        SetupIGOWindow((void*)mView->winId(), 0.0);
#endif
        
#if defined(ORIGIN_PC)
        // show resets a bunch of the window state to be shown.
		mView->show();

		// We reset the window flags to allow IGO to set focus on the windows
		setWindowStateFlags();
#endif

        mView->setOptimizationFlag(QGraphicsView::DontSavePainterState);
        mView->setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
        
        resize(mWidget->width(), mWidget->height());

		mWindowManager->layoutWindow(this, false);

		ORIGIN_VERIFY_CONNECT(mWidget, SIGNAL(igoCloseWidget(bool)), this, SLOT(closeWidget(bool)));
        ORIGIN_VERIFY_CONNECT(mWidget, SIGNAL(igoMove(QPoint)), this, SLOT(setPosition(QPoint)));
        ORIGIN_VERIFY_CONNECT(mWidget, SIGNAL(igoResize(const QSize&)), this, SLOT(setSize(const QSize&)));
		ORIGIN_VERIFY_CONNECT(mWidget, SIGNAL(igoDestroy()), this, SLOT(destroyWidget()));
		ORIGIN_VERIFY_CONNECT(mWidget, SIGNAL(destroyed()), this, SLOT(onWidgetDestroyed()));
        
        // Is the embedded object actually trying to listen to some of the container's events?
        if (settings.windowListener)
        {
            QObject* listenerObj = dynamic_cast<QObject*>(settings.windowListener);
            if (listenerObj)
            {
                ORIGIN_VERIFY_CONNECT(this, SIGNAL(focusChanged(QWidget*, bool)), listenerObj, SLOT(onFocusChanged(QWidget*, bool)));
            }
        }
        
        // Is the embedded object actually trying to listen to some of the 
        if (settings.screenListener)
        {
            QObject* listener = dynamic_cast<QObject*>(settings.screenListener);
            if (listener)
            {
                ORIGIN_VERIFY_CONNECT(mWindowManager, SIGNAL(screenSizeChanged(uint32_t, uint32_t, uint32_t)), this, SLOT(onScreenSizeChanged(uint32_t, uint32_t, uint32_t)));
            }
        }
    }
}

void IGOQtContainer::onIgoVisibleChanged(bool visible, bool)
{
    if(mPinButtonGuard)
    {
        // Complete the pinning process if necessary
        if (!visible && isPinningInProgress())
            mPinButton->hideSlider();

        mPinButton->setVisibility(visible);
    }
}

void IGOQtContainer::onScreenSizeChanged(uint32_t width, uint32_t height, uint32_t connectionId)
{
    ORIGIN_ASSERT(mWindowManager->thread() == this->thread());
    QObject* listener = dynamic_cast<QObject*>(mBehavior.screenListener);
    if (listener)
    {
        if (mConnectionId == 0 || mConnectionId == connectionId)
        {
            mBehavior.screenListener->onSizeChanged(width, height);
        }
    }
}
    
void IGOQtContainer::setIGOVisible(bool visible)
{
    if (mWidget)
        mWidget->setIGOVisible(visible);
    
#ifdef ORIGIN_MAC
    if (mContextMenu)
    {
        emit contextMenuDeactivate();
    }
#endif
}

void IGOQtContainer::setFocus(bool s)
{
    bool hasChanged = mHasFocus != s;
    bool hadFocus = mHasFocus;
    
	mHasFocus=s;

    if (mView)
    {
        if(mHasFocus)
        {
            mView->activateWindow();
            QApplication::setActiveWindow(mView);
        }
        
        else
        if (hadFocus)
        {
            QApplication::setActiveWindow(NULL);
        }
    }
    
    if (mWidget)
    {
        if (mHasFocus)
        {
            QEvent e(QEvent::WindowActivate);
            QCoreApplication::sendEvent(mWidget, &e);
        }
        else
        if (hadFocus)
        {
            QEvent e(QEvent::WindowDeactivate);
            QCoreApplication::sendEvent(mWidget, &e);
        }
    }
    
    if (hasChanged)
    {
        QWidget* window = NULL;
        if (mWidget)
            window = (mWidget->getRealParent()) ? mWidget->getRealParent() : mWidget;
        
        focusChanged(window, mHasFocus);
    }
}

void IGOQtContainer::removeFocus()
{
    if (mView)
    {
        mView->clearFocus();
        QApplication::setActiveWindow(NULL);
    }
}

// Disable optimisations, so that our little squish helper is not optimized away!
#ifdef _MSC_VER
    #pragma optimize("", off) 
#endif

void IGOQtContainer::show(bool visible)
{
	if (mWidget)
	{
		if(mWidget->isVisibleInIGO()!=visible)
		{
			mWidget->setVisibleInIGO(visible);

			mDirty = true;
			mFlags |= IGOIPC::WINDOW_UPDATE_ALL;

			emit windowDirty(this);
		}
	}
}
#ifdef _MSC_VER
    #pragma optimize("", on) // See comments above regarding this optimization change.
#endif

void IGOQtContainer::setResizingBorder(int nsSize, int ewSize)
{
	mNsSizingBorder=nsSize;
	mEwSizingBorder=ewSize;
}

// Set an outer margin used to detect resizing/moving when the art itself offsets the content
void IGOQtContainer::setOuterMargins(int left, int top, int right, int bottom)
{
    if (left > 0)
        mOuterLeftMargin = left;
    
    if (top > 0)
        mOuterTopMargin = top;
    
    if (right > 0)
        mOuterRightMargin = right;
    
    if (bottom > 0)
        mOuterBottomMargin = bottom;
}

void IGOQtContainer::setDraggingTop(uint height)
{
	//_ASSERT(mWidget);
	mCustomDraggingArea=false;
	mDraggingArea.setWidth(mWidget->width());
	mDraggingArea.setHeight(height);
}

void IGOQtContainer::setDraggingArea(QRect &area)
{
	mCustomDraggingArea=true;
	mDraggingArea=area;
}

void IGOQtContainer::registerTitlebarButton(QObject* btn)
{
    // Add it to our event filter to listen to the enter/leave events to disable dragging accordingly
    btn->installEventFilter(this);
    btn->setProperty(OIG_TITLEBAR_BUTTON_KEYWORD, true);
}

#ifdef ORIGIN_MAC

// We need to filter the mouse move events when we are moving/resizing a window to help with the
// latency - however we don't want to filter them when simply scrolling the content
bool IGOQtContainer::filterMouseMoveEvent(uint64_t timestamp, bool isMoving, bool isResizing)
{
    bool filterEvent = false;
    
    if ((isMoving || isResizing) && (timestamp != 0))
    {
        if (mLastMouseMoveTimestamp != 0)
        {
            uint64_t gameDelta = timestamp - mLastMouseMoveTimestamp;
           
            if (mLastProcessDelta == 0)
            {
                uint64_t processTimestamp = UnsignedWideToUInt64(AbsoluteToNanoseconds(UpTime()));
                mLastProcessDelta = processTimestamp - mLastProcessTimestamp;
                mLastProcessTimestamp = processTimestamp;
            }
            
            // Is the delta large enough to consider the event?
            if (gameDelta < mLastProcessDelta)
            {
                // Skip the event
                filterEvent = true;
            }
            
            else
            {
                // Use the event/update our references
                mLastMouseMoveTimestamp = timestamp;                
                mLastProcessDelta = 0;
                mLastProcessTimestamp = UnsignedWideToUInt64(AbsoluteToNanoseconds(UpTime()));
            }
        }
        
        else
        {
            // Initial setup
            mLastProcessDelta = 0;
            mLastProcessTimestamp = UnsignedWideToUInt64(AbsoluteToNanoseconds(UpTime()));
            mLastMouseMoveTimestamp = timestamp;
        }
    }
    
    else
    {
        // Reset it all
        mLastProcessDelta = 0;
        mLastProcessTimestamp = 0;
        mLastMouseMoveTimestamp = 0;
    }

    return filterEvent;
}

#endif

// Enable the window resizing/moving + cursor shape change(useful to avoid conflicts with
// the content itself, for example when dealing with a browser and its internal webview)
void IGOQtContainer::enableFrameEvents()
{
    mDisableFrameEvents = false;
}

// Disable the window resizing/moving + cursor shape change (useful to avoid conflicts with
// the content itself, for example when dealing with a browser and its internal webview)
void IGOQtContainer::disableFrameEvents()
{
    mDisableFrameEvents = true;
    
    setSizing(false);
    setMoving(false);
    mSizingEdge = SIZING_NONE;
}

#ifdef ORIGIN_MAC
void IGOQtContainer::sendMousePos(QPoint pt, const QPoint &ptOffset, const QPointF &ptScaling, uint64_t timestamp)
#else
void IGOQtContainer::sendMousePos(const QPoint &pt, const QPoint &ptOffset, const QPointF &ptScaling)
#endif
{
	Qt::KeyboardModifiers qt_modifiers = Qt::NoModifier;
    bool mouseInWidget = contains(pt, mOuterLeftMargin, mOuterTopMargin, -mOuterRightMargin, -mOuterBottomMargin);

    QPoint prevMousePos = mCurrentMousePos;

#if defined(ORIGIN_MAC) && defined(USE_CLIENT_SIDE_FILTERING)
    if (filterMouseMoveEvent(timestamp, mMoving, mSizing))
        return;
#endif
    
	//TODO: cleanup.....
    mCurrentMousePosOffset = ptOffset;
    mCurrentMousePosScaling = ptScaling;

	if(mMouseInWidget != mouseInWidget && !mSizing && !mMoving)	// we entered / left the widget
	{
		// mouse leave / enter
#ifdef SHOW_MOUSE_MOVE_LOGIC
		IGO_TRACE("mouse leave / enter");
#endif
		mCurrentMousePos = pt - mPosition;
		mMouseInWidget = mouseInWidget;
        mSizingEdge = SIZING_NONE;
	}

	// dragging mouse cursor change
	uint cursor = mCursor;


    if (!mDisableFrameEvents)
    {
    
	// do not apply resizing/moving logic if we are over a context menu
	if(mMouseInWidget && ((mContextMenu && mContextMenu->isVisible()) || (mSubmenu && mSubmenu->isVisible())))
	{
		cursor = IGOIPC::CURSOR_ARROW;
		mouseInWidget = false;
        setSizing(false);
        setMoving(false);
		mSizingEdge = SIZING_NONE;
	}
	else
	if((mSizingEdge == SIZING_NONE || !mSizing) && mMoving)
	{
#ifdef SHOW_MOUSE_MOVE_LOGIC
		IGO_TRACE("cursor: dragging cursor");
#endif
		cursor = IGOIPC::CURSOR_ARROW;
		//cursor = IGOIPC::CURSOR_HAND;
	}
	else
	{
		// resizing mouse cursor change
		if((mSizingEdge == SIZING_NONE || !mSizing) && mouseInWidget && !(mCurrentMouseButtonState & Qt::LeftButton) && ( mNsSizingBorder > 0 || mEwSizingBorder > 0))
		{
#ifdef SHOW_MOUSE_MOVE_LOGIC
			IGO_TRACE("cursor: sizing cursor");
#endif
			QRect r = rect(false);
			r.adjust(mEwSizingBorder, mNsSizingBorder, -mEwSizingBorder, -mNsSizingBorder);

			// detect which edges we are on
			bool top	= (pt.y() < r.top());
			bool bottom = (pt.y() >= r.bottom());
			bool left	= (pt.x() < r.left());
			bool right	= (pt.x() >= r.right());

			if (top && left && mNsSizingBorder > 0 && mEwSizingBorder > 0)
			{
				cursor = IGOIPC::CURSOR_SIZENWSE;
				mSizingEdge = SIZING_TOP | SIZING_LEFT;
#ifdef SHOW_MOUSE_MOVE_LOGIC
				IGO_TRACE("cursor: top left");
#endif

			}
			else if (top && right && mNsSizingBorder > 0 && mEwSizingBorder > 0)
			{
				cursor = IGOIPC::CURSOR_SIZENESW;
				mSizingEdge = SIZING_TOP | SIZING_RIGHT;
#ifdef SHOW_MOUSE_MOVE_LOGIC
				IGO_TRACE("cursor: top right");
#endif
			}
			else if (bottom && left && mNsSizingBorder > 0 && mEwSizingBorder > 0)
			{
				cursor = IGOIPC::CURSOR_SIZENESW;
				mSizingEdge = SIZING_BOTTOM | SIZING_LEFT;
#ifdef SHOW_MOUSE_MOVE_LOGIC
				IGO_TRACE("cursor: bottom left");
#endif
			}
			else if (bottom && right && mNsSizingBorder > 0 && mEwSizingBorder > 0)
			{
				cursor = IGOIPC::CURSOR_SIZENWSE;
				mSizingEdge = SIZING_BOTTOM | SIZING_RIGHT;
#ifdef SHOW_MOUSE_MOVE_LOGIC
				IGO_TRACE("cursor: bottom right");
#endif
			}
			else if (top && mNsSizingBorder > 0)
			{
				cursor = IGOIPC::CURSOR_SIZENS;
				mSizingEdge = SIZING_TOP;
#ifdef SHOW_MOUSE_MOVE_LOGIC
				IGO_TRACE("cursor: top");
#endif
			}
			else if (bottom && mNsSizingBorder > 0)
			{
				cursor = IGOIPC::CURSOR_SIZENS;
				mSizingEdge = SIZING_BOTTOM;
#ifdef SHOW_MOUSE_MOVE_LOGIC
				IGO_TRACE("cursor: bottom");
#endif
			}
			else if (left && mEwSizingBorder > 0)
			{
				cursor = IGOIPC::CURSOR_SIZEWE;
				mSizingEdge = SIZING_LEFT;
#ifdef SHOW_MOUSE_MOVE_LOGIC
				IGO_TRACE("cursor: left");
#endif
			}
			else if (right && mEwSizingBorder > 0)
			{
				cursor = IGOIPC::CURSOR_SIZEWE;
				mSizingEdge = SIZING_RIGHT;
#ifdef SHOW_MOUSE_MOVE_LOGIC
				IGO_TRACE("cursor: right");
#endif
			}
			else
			{
				cursor = IGOIPC::CURSOR_ARROW;
				mSizingEdge = SIZING_NONE;
#ifdef SHOW_MOUSE_MOVE_LOGIC
				IGO_TRACE("cursor: none");
#endif
			}		
		}
		else
		{

			if((mSizingEdge == SIZING_NONE || !mSizing) && !mMoving && !(mCurrentMouseButtonState & Qt::LeftButton))
			{	// default cursor
#ifdef SHOW_MOUSE_MOVE_LOGIC
				IGO_TRACE("cursor: !mSizingEdge && !mMoving");
#endif

				cursor = IGOIPC::CURSOR_ARROW;
			}
		}
	}

	if (cursor != mCursor)
    {
#ifdef SHOW_MOUSE_MOVE_LOGIC
        IGO_TRACEF("Old cursor: %d, new cursor: %d -> notify manager", mCursor,cursor);
#endif
        mCursor = cursor;
		emit (cursorChanged(cursor));
    }

    } // Resizing/moving not disabled

#ifdef ORIGIN_MAC
    // We removed the clamping on the OIG side to get all events/get along with filtering, so let's clamp the coordinates here if we're moving/resizing
    QSize screen = mWindowManager->screenSize();
    
    if (pt.x() < 0)
        pt.rx() = 0;
    
    else
    if (pt.x() > screen.width())
        pt.rx() = screen.width();
    
    if (pt.y() < 0)
        pt.ry() = 0;
    
    else
    if (pt.y() > screen.height())
        pt.ry() = screen.height();
#endif

    if (!mDisableFrameEvents)
    {
	// dragging action
	if((mSizingEdge == SIZING_NONE || !mSizing)
       && !mNoWindowDraggingWhileMouseOverButton
       && (mMoving || (mDraggingArea.contains(mMouseClickPos) && mouseInWidget))
       && mHasFocus && (mCurrentMouseButtonState & Qt::LeftButton) && (mCurrentMousePos != (pt - mPosition)))
	{
		// drag enter
#ifdef SHOW_MOUSE_MOVE_LOGIC
		IGO_TRACE("drag enter");
#endif 

        setMoving(true);
        setSizing(false);
		mDirty = true;
		QPoint offset = mCurrentMousePos;
		mCurrentMousePos = pt - mPosition;		
		mPosition -= (offset - mCurrentMousePos);
		mCurrentMousePos = pt - mPosition;
        setPosition(mPosition);
	}
	else
	{	
		if((mSizingEdge == SIZING_NONE || !mSizing) && mMoving && !(mCurrentMouseButtonState & Qt::LeftButton))
		{
			// drag leave
#ifdef SHOW_MOUSE_MOVE_LOGIC
			IGO_TRACE("drag leave");
#endif
			mCurrentMousePos = pt - mPosition;
            setMoving(false);
		}
		else
		{
			if(mSizingEdge != SIZING_NONE)// && (mCurrentMousePos != (pt - mPosition)))
			{
				if(mHasFocus && (mCurrentMouseButtonState & Qt::LeftButton))
				{
					// sizing...
                    setSizing(true);
#ifdef SHOW_MOUSE_MOVE_LOGIC
					IGO_TRACE("sizing");
#endif		
					switch(mSizingEdge)
					{
					case SIZING_LEFT:
						{
							mDirty = true;
							mCurrentMousePos = pt - mPosition;
							QPoint offset = (pt - mOriginalWindowPosition) - mDownMousePosition;

                            int newWidth = mOriginalWindowSize.width() - offset.x();
                            int newPosX = mOriginalWindowPosition.x() + offset.x();
                            if (newWidth < mWidget->minimumWidth())
                            {
                                newWidth = mWidget->minimumWidth();
                                newPosX = mOriginalWindowPosition.x() + mOriginalWindowSize.width() - newWidth;
                            }
                            
                            if (mWidget->width() >= mWidget->minimumWidth() && ( newWidth >= mWidget->minimumWidth()))
							{
                                mPosition.setX(newPosX);
								resize(newWidth, 0, false);
							}
						}
						break;
                            
					case SIZING_RIGHT:
						{
							mDirty = true;
							QPoint offset = (pt - mPosition) - mDownMousePosition;
                            
                            int newWidth = mOriginalWindowSize.width() + offset.x();
                            if (newWidth < mWidget->minimumWidth())
                                newWidth = mWidget->minimumWidth();

							if (mWidget->width() >= mWidget->minimumWidth() && ( newWidth >= mWidget->minimumWidth()))
							{
								resize(newWidth, 0, false);
							}
							mCurrentMousePos = pt - mPosition;
						}
						break;
                            
					case SIZING_BOTTOM:
						{
							mDirty = true;
							QPoint offset = (pt - mPosition) - mDownMousePosition;
                            
                            int newHeight = mOriginalWindowSize.height() + offset.y();
                            if (newHeight < mWidget->minimumHeight())
                                newHeight = mWidget->minimumHeight();

							if (mWidget->height() >= mWidget->minimumHeight() && ( newHeight >= mWidget->minimumHeight()))
							{
								resize(0, newHeight, false);
							}
							mCurrentMousePos = pt - mPosition;
						}
						break;
                            
					case SIZING_TOP:
						{
							mDirty = true;
							mCurrentMousePos = pt - mPosition;
							QPoint offset = (pt - mOriginalWindowPosition) - mDownMousePosition;
                            
                            int newHeight = mOriginalWindowSize.height() - offset.y();
                            int newPosY = mOriginalWindowPosition.y() + offset.y();
                            if (newHeight < mWidget->minimumHeight())
                            {
                                newHeight = mWidget->minimumHeight();
                                newPosY = mOriginalWindowPosition.y() + mOriginalWindowSize.height() - newHeight;
                            }

							if (mWidget->height() >= mWidget->minimumHeight() && ( newHeight >= mWidget->minimumHeight()))
							{
								mPosition.setY(newPosY);
								resize(0, newHeight, false);
							}

						}
						break;
                            
					case SIZING_LEFT+SIZING_BOTTOM:
						{
							mDirty = true;
							mCurrentMousePos = pt - mPosition;
							QPoint offset = (pt - mOriginalWindowPosition) - mDownMousePosition;
                            
                            int newWidth = mOriginalWindowSize.width() - offset.x();
                            int newPosX = mOriginalWindowPosition.x() + offset.x();
                            if (newWidth < mWidget->minimumWidth())
                            {
                                newWidth = mWidget->minimumWidth();
                                newPosX = mOriginalWindowPosition.x() + mOriginalWindowSize.width() - newWidth;
                            }
                            
                            mPosition.setX(newPosX);
                            
                            int newHeight = mOriginalWindowSize.height() + offset.y();
                            if (newHeight < mWidget->minimumHeight())
                                newHeight = mWidget->minimumHeight();

							resize(newWidth, newHeight , false);
							
						}
						break;
                            
					case SIZING_RIGHT+SIZING_BOTTOM:
						{
							mDirty = true;
							
							QPoint offset = (pt - mPosition) - mDownMousePosition;
                            
                            int newWidth = mOriginalWindowSize.width() + offset.x();
                            if (newWidth < mWidget->minimumWidth())
                                newWidth = mWidget->minimumWidth();
                            
                            int newHeight = mOriginalWindowSize.height() + offset.y();
                            if (newHeight < mWidget->minimumHeight())
                                newHeight = mWidget->minimumHeight();

							resize(newWidth, newHeight , false);
                            mCurrentMousePos = pt - mPosition;							
						}

						break;
					case SIZING_LEFT+SIZING_TOP:
						{
							mDirty = true;
							mCurrentMousePos = pt - mPosition;

							QPoint offset = (pt - mOriginalWindowPosition) - mDownMousePosition;
                            
                            int newWidth = mOriginalWindowSize.width() - offset.x();
                            int newPosX = mOriginalWindowPosition.x() + offset.x();
                            if (newWidth < mWidget->minimumWidth())
                            {
                                newWidth = mWidget->minimumWidth();
                                newPosX = mOriginalWindowPosition.x() + mOriginalWindowSize.width() - newWidth;
                            }
                            
                            mPosition.setX(newPosX);
                            
                            int newHeight = mOriginalWindowSize.height() - offset.y();
                            int newPosY = mOriginalWindowPosition.y() + offset.y();
                            if (newHeight < mWidget->minimumHeight())
                            {
                                newHeight = mWidget->minimumHeight();
                                newPosY = mOriginalWindowPosition.y() + mOriginalWindowSize.height() - newHeight;
                            }
                            
                            mPosition.setY(newPosY);
							resize(newWidth, newHeight , false);
						}

						break;
					case SIZING_RIGHT+SIZING_TOP:
						{
							mDirty = true;
							mCurrentMousePos = pt - mPosition;

							QPoint offset = (pt - mOriginalWindowPosition) - mDownMousePosition;
                            
                            int newWidth = mOriginalWindowSize.width() + offset.x();
                            if (newWidth < mWidget->minimumWidth())
                                newWidth = mWidget->minimumWidth();
                            
                            int newHeight = mOriginalWindowSize.height() - offset.y();
                            int newPosY = mOriginalWindowPosition.y() + offset.y();
                            if (newHeight < mWidget->minimumHeight())
                            {
                                newHeight = mWidget->minimumHeight();
                                newPosY = mOriginalWindowPosition.y() + mOriginalWindowSize.height() - newHeight;
                            }
                            
                            mPosition.setY(newPosY);
							resize(newWidth, newHeight , false);
						}

						break;
					}
				}
				else
				{
					if(mSizing && !(mCurrentMouseButtonState & Qt::LeftButton))
					{
                        setSizing(false);
						// sizing ended
#ifdef SHOW_MOUSE_MOVE_LOGIC
						IGO_TRACE("sizing ended");
#endif
						mSizingEdge = SIZING_NONE;
						mCurrentMousePos = pt - mPosition;
					}
					else
					{
						// mouse move
#ifdef SHOW_MOUSE_MOVE_LOGIC
						IGO_TRACE("mouse move");
#endif
						mCurrentMousePos = pt - mPosition;
					}
				}
			}
			else
			{
				// mouse move
#ifdef SHOW_MOUSE_MOVE_LOGIC
				IGO_TRACE("mouse move");
#endif
				mCurrentMousePos = pt - mPosition;
			}
		}
	}

	// keep the widget on screen - only moveable widgets !!!!
	if ( !(layout() & IGOWindowManager::WINDOW_ALIGN_LEFT) &&
		!(layout() & IGOWindowManager::WINDOW_ALIGN_RIGHT) &&
		!(layout() & IGOWindowManager::WINDOW_ALIGN_TOP) &&
		!(layout() & IGOWindowManager::WINDOW_ALIGN_BOTTOM))
	{
#ifdef ORIGIN_MAC
        static const int FUDDGE_AREA = 37;      // updated to better take into account margin due to rounded corners (especially for Y component)
        static const int TOP_FUDDGE_AREA = 33;  // unfortunately the art is not consistent across windows, so I used the browser window as my reference!
#else
		static const int FUDDGE_AREA = 25; // 25 pixel
        static const int TOP_FUDDGE_AREA = 25;
#endif

		QRect widgetRect = rect(false);   // minimize calculation time
		int widgetWidth = widgetRect.width();

        QSize screen = mWindowManager->screenSize();
		if ((mPosition.x() + FUDDGE_AREA) > screen.width()) // right edge
			setX( screen.width() - FUDDGE_AREA );

		if (((mPosition.x() + widgetWidth) - FUDDGE_AREA) < 0) // left edge
			setX( -widgetWidth + FUDDGE_AREA );

		if ((mPosition.y() + FUDDGE_AREA) > screen.height())       // bottom edge
			setY( screen.height() - FUDDGE_AREA );

        if (mDraggingArea.height()>0 /*if we have no dragging area, don't fuddge!!!*/ && (((mPosition.y() + mDraggingArea.height()) - TOP_FUDDGE_AREA) < 0))    // top edge   (usually the dragging area)
			setY( -mDraggingArea.height() + TOP_FUDDGE_AREA );

		mCurrentMousePos = pt - mPosition;
	}

    }
    
    else
    {
        // Resizing/moving currently disabled
        mCurrentMousePos = pt - mPosition;
    }
        
    // Only send mouse position if we have moved - this is because we're sending "fake" events/filter the events for latency reasons,
    // we don't want to send move events while the user is double-clicking/triple-clicking, otherwise it would undo his/her selection.
    if (mCurrentMousePos != prevMousePos)
    {
        QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMouseMove);
        QPoint ptView = mView->mapFromScene(mCurrentMousePos);
        QPoint ptGlobal = mView->viewport()->mapToGlobal(ptView);
        event.setScenePos(mCurrentMousePos);
        event.setPos(mCurrentMousePos);
        event.setScreenPos(ptGlobal);
        event.setButton(Qt::NoButton);
        event.setButtons(mCurrentMouseButtonState);
        event.setModifiers(qt_modifiers);
        
        sendMouseEvent(event);
    }
}

void IGOQtContainer::sendMouseButtons(const Qt::MouseButtons &buttons, bool isDoubleClick)
{

    if (!mView)
        return;

	Qt::KeyboardModifiers qt_modifiers = Qt::NoModifier;
    QEvent::Type type = QEvent::None;
    Qt::MouseButton qt_button = Qt::NoButton;
    
    bool sendEvent = false;


	if ( ((buttons & Qt::LeftButton) && !(mCurrentMouseButtonState & Qt::LeftButton)) || (!(buttons & Qt::LeftButton) && (mCurrentMouseButtonState & Qt::LeftButton)) )
	{
		type = (buttons & Qt::LeftButton) ? QEvent::GraphicsSceneMousePress : QEvent::GraphicsSceneMouseRelease;
		qt_button = Qt::LeftButton;
		mMouseClickPos = mCurrentMousePos;
        
        sendEvent = true;
	}
    
	if ( ((buttons & Qt::MiddleButton) && !(mCurrentMouseButtonState & Qt::MiddleButton)) || (!(buttons & Qt::MiddleButton) && (mCurrentMouseButtonState & Qt::MiddleButton)) )
	{
		type = (buttons & Qt::MiddleButton) ? QEvent::GraphicsSceneMousePress : QEvent::GraphicsSceneMouseRelease;
		qt_button = Qt::MiddleButton;
		mMouseClickPos = mCurrentMousePos;
        
        sendEvent = true;
	}
    
	if ( ((buttons & Qt::RightButton) && !(mCurrentMouseButtonState & Qt::RightButton)) || (!(buttons & Qt::RightButton) && (mCurrentMouseButtonState & Qt::RightButton)) )
	{
		type = (buttons & Qt::RightButton) ? QEvent::GraphicsSceneMousePress : QEvent::GraphicsSceneMouseRelease;
		qt_button = Qt::RightButton;
		mMouseClickPos = mCurrentMousePos;
        
        sendEvent = true;
	}

    if (sendEvent)
    {
        // Check if we are clicking DOWN outside a context menu BEFORE sending the click event (in case that click event would be the one to create the context menu, for example
        // the Chat window chevron)
        if (mContextMenu && type == QEvent::GraphicsSceneMousePress)
        {
            // For now, assume the menu is attached to the main widget that's embedded in the container
            // Use a little margin too for error + not to fail on the initial click that creates the context menu!

			// need to add calculations for the submenu too
			QSize subMenuSize = QSize(0,0);
            if( mSubmenu )
            {
                // TBD: how is this expected to work when there is more than one sub-menu in the context menu???
			    foreach(QAction* childAction, mContextMenu->actions())
			    {
                    QMenu* submenu = dynamic_cast<QMenu*>(childAction->menu());
                    if (submenu)
                    {
                        subMenuSize = submenu->sizeHint();
                        break;
                    }
			    }
            }

            QRect menuArea(mContextMenu->pos() - QPoint(2, 2), (mContextMenu->size() + subMenuSize) + QSize(2, 2));
            if (!menuArea.contains(mMouseClickPos))
            {
                sendEvent = false;
                emit contextMenuDeactivate();
            }
        }

        // Re-check whether to send the click event in case we used it to deactivate a context menu
        // Also skip it for a double-click (only coming from a button down event (only tracking left-clicks right now))
        // We'll send it later (but still we need to update the current button states!)
        if (sendEvent && !isDoubleClick)
        {
            QGraphicsSceneMouseEvent event(type);
            QPoint ptView = mView->mapFromScene(mMouseClickPos);
            QPoint ptGlobal = mView->viewport()->mapToGlobal(ptView);
            event.setScenePos(mMouseClickPos);
            event.setPos(mMouseClickPos);
            event.setScreenPos(ptGlobal);
            event.setButton(qt_button);
            event.setButtons(buttons);
            event.setModifiers(qt_modifiers);
            
            sendMouseEvent(event);
            
            
            
            // The normal Qt behavior for mouse event (see qt_cocoa_helpers_mac.mm) is to send a ContextMenu event when the user right-click on an item
            // -> just send one and see what happens (this is to fix the new buddylist/show the context menu when right-clicking on a contact)
            // BE AWARE: you need to have FOCUS for this event to make it all the way up -> otherwise you'll need to create this event at a different
            // spot so that you can right-click on an item without focus to show the context menu
            // NEW: we now only do this when we explicitely enable it not to see some of the default popups that would make OIG/game lose focus and go beserk
            if (mBehavior.supportsContextMenu && qt_button == Qt::RightButton && type == QEvent::GraphicsSceneMousePress)
            {
                QGraphicsSceneContextMenuEvent event(QEvent::GraphicsSceneContextMenu);
                event.setScenePos(mMouseClickPos);
                event.setPos(mMouseClickPos);
                event.setScreenPos(ptGlobal);
                event.setReason(QGraphicsSceneContextMenuEvent::Mouse);
                event.setModifiers(qt_modifiers);
                
                qt_sendSpontaneousEvent(mView->scene(), &event);
            }
        }
    }
    
    else
    if (buttons == 0)
    {
        // We clicked outside the window but not on a new one
        emit contextMenuDeactivate();
    }

	mPrevMouseButtonState = mCurrentMouseButtonState;
	mCurrentMouseButtonState = buttons;
    
    int prevLeftBtn = mPrevMouseButtonState & Qt::LeftButton;
    int currLeftBtn = mCurrentMouseButtonState & Qt::LeftButton;
    
    if (currLeftBtn ^ prevLeftBtn)
    {
        if (currLeftBtn && !isDoubleClick)
        {
            // Left button down
            // save the down click
            mDownMousePosition = mCurrentMousePos;
            mOriginalWindowPosition = mPosition;
            mOriginalWindowSize = mWidget->size();

            if (mSizingEdge != SIZING_NONE)
                setSizing(true);
        }
        
        else
        {
            // Left button up
            setSizing(false);
            setMoving(false);
        }
    }
}

void IGOQtContainer::sendMouseWheel(int wheelVerticalPos, int wheelHorizontalPos)
{
    if (!mView)
        return;

    if (wheelVerticalPos != 0)
    {
        Qt::KeyboardModifiers qt_modifiers = Qt::NoModifier;
	    QEvent::Type type = QEvent::GraphicsSceneWheel;
    
        QGraphicsSceneWheelEvent event(type);
        QPoint ptView = mView->mapFromScene(mCurrentMousePos);
        QPoint ptGlobal = mView->viewport()->mapToGlobal(ptView);
        event.setScenePos(mCurrentMousePos);
        event.setPos(mCurrentMousePos);
        event.setScreenPos(ptGlobal);
        event.setDelta(wheelVerticalPos);
        event.setOrientation(Qt::Vertical);
    #if defined(ORIGIN_MAC)
        event.setButtons(mCurrentMouseButtonState);
    #else
        event.setButtons(mCurrentMouseButtonState);
    #endif
        event.setModifiers(qt_modifiers);
    
        sendMouseEvent(event); 
    }
    
    if (wheelHorizontalPos != 0)
    {
        Qt::KeyboardModifiers qt_modifiers = Qt::NoModifier;
	    QEvent::Type type = QEvent::GraphicsSceneWheel;
    
        QGraphicsSceneWheelEvent event(type);
        QPoint ptView = mView->mapFromScene(mCurrentMousePos);
        QPoint ptGlobal = mView->viewport()->mapToGlobal(ptView);
        event.setScenePos(mCurrentMousePos);
        event.setPos(mCurrentMousePos);
        event.setScreenPos(ptGlobal);
        event.setDelta(wheelHorizontalPos);
        event.setOrientation(Qt::Horizontal);
    #if defined(ORIGIN_MAC)
        event.setButtons(mCurrentMouseButtonState);
    #else
        event.setButtons(mCurrentMouseButtonState);
    #endif
        event.setModifiers(qt_modifiers);
    
        sendMouseEvent(event); 
    }
}

void IGOQtContainer::sendDoubleClick()
{
	Qt::KeyboardModifiers qt_modifiers = Qt::NoModifier;
	QEvent::Type type = QEvent::GraphicsSceneMouseDoubleClick;
	Qt::MouseButton qt_button = Qt::LeftButton; // Not supporting double right-clicks
	mMouseClickPos = mCurrentMousePos;
    
    QGraphicsSceneMouseEvent event(type);
    QPoint ptView = mView->mapFromScene(mCurrentMousePos);
    QPoint ptGlobal = mView->viewport()->mapToGlobal(ptView);
    event.setScenePos(mCurrentMousePos);
    event.setPos(mCurrentMousePos);
    event.setScreenPos(ptGlobal);
    event.setButton(qt_button);
#if defined(ORIGIN_MAC)
    event.setButtons(mCurrentMouseButtonState & ~qt_button);
#else
    event.setButtons(mCurrentMouseButtonState ^qt_button);
#endif
    event.setModifiers(qt_modifiers);
    
    sendMouseEvent(event); 
}

bool IGOQtContainer::contains(const QPoint &pt, int32_t dx1, int32_t dy1, int32_t dx2, int32_t dy2)
{
    // simple bounding box test
    QRect r = rect(true);
    r.adjust(dx1, dy1, dx2, dy2);

    if(r.contains(pt, true))
    {
        // see if we hit transparent pixels(bounding boxes could be larger, because of drop down lists), then we would return false on the hit test!
        if(isTransparentPixel(pt - mPosition))
            return mHoverOnTransparent;
        else
            return true;
    }
    return false;
}

void IGOQtContainer::lockWindow()
{
	QMutexLocker locker(&mWindowMutex);
	mLocked=true;
}

void IGOQtContainer::unlockWindow()
{
	QMutexLocker locker(&mWindowMutex);
	mLocked=false;
}

const uchar *IGOQtContainer::pixelData() const
{
	return mImageData!=NULL ? mImageData->constBits() : NULL;
}

uint IGOQtContainer::pixelDataSize()
{
	return mImageData!=NULL ? mImageData->byteCount() : 0;
}

void IGOQtContainer::setPosition(const QPoint &pos)
{
    if (!mPositionLocked)
    {
        mPosition = pos;
        mDockingPosition = pos;
        mDockingOrigin = IIGOWindowManager::WindowDockingOrigin_DEFAULT;
        mDirty = true;
        mFlags |= IGOIPC::WINDOW_UPDATE_X | IGOIPC::WINDOW_UPDATE_Y;

        emit windowDirty(this);
    }
}

void IGOQtContainer::setSize(const QSize &size)
{
	resize(size.width(), size.height(), true);
}

void IGOQtContainer::setAlpha(qreal alpha)
{
	mAlpha = (uint8_t)(alpha*255);
	mDirty = true;
	mFlags |= IGOIPC::WINDOW_UPDATE_ALPHA;
	emit windowDirty(this);
}

void IGOQtContainer::pinningFinished()
{
    mIsPinningPendingState = false;
    mPinningInProgress = false;

    mDirty = true;
	mFlags |= IGOIPC::WINDOW_UPDATE_ALPHA;
	emit windowDirty(this);
    emit pinningStateChanged(mPinningInProgress);
}

void IGOQtContainer::pinningStarted()
{
    // Move this window to the front and then the pin slider in front of it.
    if(hasFocus() == false)
    {
        mWindowManager->setWindowFocus(this);
        if(mPinButtonGuard)
            mPinButton->raiseSlider();
    }

	emit mWindowManager->igoUnpinAll(this);	// unpin all other pins, if we have untoggled them via the hotkey
	
	mIsPinningPendingState = false;
    mPinningInProgress = true;
	mDirty = true;
	mFlags |= IGOIPC::WINDOW_UPDATE_ALPHA;
	emit windowDirty(this);
    emit pinningStateChanged(mPinningInProgress);
}

void IGOQtContainer::pinningToggled()
{
    emit pinningToggled(this);
}


void IGOQtContainer::updatePinnedAlpha()
{
    mPinningInProgress = true;
	mDirty = true;
	mFlags |= IGOIPC::WINDOW_UPDATE_ALPHA;
	emit windowDirty(this);
}

uint8_t IGOQtContainer::pinnedAlpha()
{
    uint8_t alpha = mPinButtonGuard ? mPinButton->sliderValue() : 13;
    return alpha;
}

void IGOQtContainer::updateWindow(uint flags)
{
	mDirty = true;
	mFlags |= flags;
	emit windowDirty(this);
}

void IGOQtContainer::setX(const int x)
{
    if (x != mPosition.x() && !mPositionLocked)
    {
	    mPosition.setX(x);
        mDockingPosition.setX(x);
        mDockingOrigin = IIGOWindowManager::WindowDockingOrigin_DEFAULT;
        mDirty = true;
	    mFlags |= IGOIPC::WINDOW_UPDATE_X;
	    emit windowDirty(this);
    }
}

void IGOQtContainer::setY(const int y)
{
    if (y != mPosition.y() && !mPositionLocked)
    {
	    mPosition.setY(y);
        mDockingPosition.setY(y);
        mDockingOrigin = IIGOWindowManager::WindowDockingOrigin_DEFAULT;
	    mDirty = true;
	    mFlags |= IGOIPC::WINDOW_UPDATE_Y;
	    emit windowDirty(this);
    }
}

void IGOQtContainer::setDockingPosition(uint32_t screenWidth, uint32_t screenHeight, const QPoint& pos, IIGOWindowManager::WindowDockingOrigin docking, bool locked)
{
    mDockingPosition = pos;
    mDockingOrigin = docking;
    mPositionLocked = locked;
    layoutDockingPosition(screenWidth, screenHeight);
}

void IGOQtContainer::layoutDockingPosition(uint32_t screenWidth, uint32_t screenHeight)
{
    // Compute correct X value
    bool xDirty = false;
    int x = 0;
    if (mDockingOrigin & IIGOWindowManager::WindowDockingOrigin_X_RIGHT)
    {
        x = screenWidth - mDockingPosition.x();
        if (x != mPosition.x())
            xDirty = true;
    }

    else
    if (mDockingOrigin & IIGOWindowManager::WindowDockingOrigin_X_CENTER)
    {
        x = (screenWidth - (int)width()) / 2 + mDockingPosition.x();
        if (x != mPosition.x())
            xDirty = true;
    }

    else
    {
        x = mDockingPosition.x();
        if (x != mPosition.x())
            xDirty = true;
    }

    // Compute correct Y value
    bool yDirty = false;
    int y = 0;
    if (mDockingOrigin & IIGOWindowManager::WindowDockingOrigin_Y_BOTTOM)
    {
        y = screenHeight - mDockingPosition.y();
        if (y != mPosition.y())
            yDirty = true;
    }

    else
    if (mDockingOrigin & IIGOWindowManager::WindowDockingOrigin_Y_CENTER)
    {
        y = (screenHeight - (int)height()) / 2 + mDockingPosition.y();
        if (y != mPosition.y())
            yDirty = true;
    }

    else
    {
        y = mDockingPosition.y();
        if (y != mPosition.y())
            yDirty = true;
    }

    // Did the window actually move?
    if (xDirty || yDirty)
    {
        mDirty = true;
        mPosition.setX(x);
        mPosition.setY(y);
        if (xDirty)
            mFlags |= IGOIPC::WINDOW_UPDATE_X;

        if (yDirty)
            mFlags |= IGOIPC::WINDOW_UPDATE_Y;

        emit windowDirty(this);
    }
}

void IGOQtContainer::pinButtonDeactivated()
{
    removeContextMenu();
}

void IGOQtContainer::pinButtonFocusChanged(bool focus)
{
    removeContextMenu();
}


void IGOQtContainer::setWId(int32_t wid)
{
    if(mWidget)
    {
        mWidget->setObjectName(QString::number(wid));
    }
}

int32_t IGOQtContainer::wid() const
{
    return mWidget->objectName().toLong();
}

void IGOQtContainer::pinButtonClicked()
{
}

void IGOQtContainer::storeWidget(const QString& productId)
{
    if (!restorable())
        return;
    
    //store widgets location+size, product id, game resolution setting        
    QByteArray pinningData;
    QDataStream pinningDataStream(&pinningData, QIODevice::ReadWrite);
    pinningDataStream.setVersion(QDataStream::Qt_5_1);
    pinningDataStream << getPosition();
    pinningDataStream << mWidget->size();
    pinningDataStream << pinnedAlpha();
    pinningDataStream << isPinned();

    QString pinningDataString = pinningData.toBase64();

    QString activeProductId = productId.isEmpty() ? mWindowManager->getActiveProductId() : productId;

    int32_t windowId = wid();
    // For browsers, we only save/restore the properties of the oldest browser -> should always use the WI_BROWSER id (this is inforced in the IGOController::WebBrowserManager)
    if (windowId >= IViewController::WI_BROWSER && windowId < IViewController::WI_GENERIC)
        windowId = IViewController::WI_BROWSER;

    mWindowManager->storePinnedWidget(windowId, activeProductId, uniqueID(), pinningDataString);
}

void IGOQtContainer::setPendingPinnedState(bool isPending)
{
    if (isPending)
    {
        if (isPinned())
        {
            mIsPinningPendingState = true;
            setPinnedSate(false);
        }
    }
    else
    {
		if (mIsPinningPendingState && mPinButtonGuard)
		{
			mIsPinningPendingState = false;
			setPinnedSate(true);
		}
    }
}

bool IGOQtContainer::isPinned()
{
    return mPinButtonGuard ? mPinButton->pinned() : false;
}

void IGOQtContainer::setPinnedSate(bool pinned)
{
    if(mPinButtonGuard)
        mPinButton->setPinned(pinned);
}


bool IGOQtContainer::restoreWidget()
{
    bool success = false;
    
    if (!restorable())
        return success;
    
    if (isPinningInProgress())  // don't restore anything if we just started a pinning for this widget
        return success;

    int32_t windowId = wid();
    // For browsers, we only save/restore the properties of the oldest browser -> should always use the WI_BROWSER id (this is inforced in the IGOController::WebBrowserManager)
    if (windowId >= IViewController::WI_BROWSER && windowId < IViewController::WI_GENERIC)
        windowId = IViewController::WI_BROWSER;

    // load pinned data
    QString data;    
    mWindowManager->loadPinnedWidget(windowId, mWindowManager->getActiveProductId(), uniqueID(), data);
    if (!data.isEmpty())
    {

        QByteArray pinningData = QByteArray::fromBase64(data.toLatin1());
        QDataStream pinningDataStream(&pinningData, QIODevice::ReadOnly);
        pinningDataStream.setVersion(QDataStream::Qt_5_1);
        QPoint pos;
        QSize size;
        uint8_t alpha = 0;
        pinningDataStream >> pos;
        pinningDataStream >> size;
        pinningDataStream >> alpha;
        
        // Additional properties we added later...
        bool pinned = false;
        if (!pinningDataStream.atEnd())
            pinningDataStream >> pinned;

        mLoadStoreInProgress = true;

        setSize(size);
        setPosition(pos);

        // For now, we force the windows to be on-screen/overriding somewhat the user's preferences
        mWindowManager->ensureOnScreen(this);
        if (mPinButtonGuard)
        {
            mPinButton->setSliderValue(alpha);
        }

        mLoadStoreInProgress = false;

        success = true;
    }
    return success;
}


void IGOQtContainer::resize(uint w, uint h, bool redraw)
{
    // Avoid recursive calls - this will stop from sending separate overlapping messagres (for example when resizing)/remove on-screen flickering/shaking when resizing/activating windows
    if (mResizingHasAlreadyStarted)
        return;
    
	if(!mView)
		return;
    
    mResizingHasAlreadyStarted = true;
    
	if(mWidget && mProxy)
	{
		// limit the widget size to the screen size, otherwise we could kill the GFX card with very large textures !!!
        QSize screen = mWindowManager->screenSize();
		if (w > screen.width() )
			w = screen.width();

		if (h > screen.height() )
			h = screen.height();

		if( (mWidget->width() != (int)w || w != 0) || (mWidget->height() != (int)h || h != 0))
		{
			w = w ? w : mWidget->width();
			h = h ? h : mWidget->height();
			mWidget->resize(w, h);
			w = mWidget->width();
			h = mWidget->height();

            uint dragWidth = w;
            if (mContextMenu)
            {
				// need to add calculations for the submenu too
				QSize subMenuSize = QSize(0,0);
                if (mSubmenu)
                {
                    subMenuSize = mSubmenu->sizeHint();
				}

                QSize menuSize = mContextMenu->sizeHint();
                // Depending on how the menus are created, looks like we may sometimes miss a few pixels (think Twitch) -> add a little fudge to the cocktail
                static const int CONTEXT_MENU_SIZE_FUDGE = 10;
                menuSize += QSize(CONTEXT_MENU_SIZE_FUDGE, CONTEXT_MENU_SIZE_FUDGE);

                QSize offset(mContextMenu->pos().x(), mContextMenu->pos().y());
                menuSize.rwidth() += subMenuSize.rwidth();
				menuSize += offset;
                
                if ((int)w < menuSize.width())
                    w = (uint)menuSize.width();
                
                if ((int)h < menuSize.height())
                    h = (uint)menuSize.height();
            }
			this->setSceneRect(0, 0, w, h);
#ifdef ORIGIN_MAC
            mView->resize(w, h); // don't know why we're adding +5 + it's not helping with the tuning of the move/resize borders!
#else
			mView->resize(w + 5, h + 5);
#endif
			
			if(!mCustomDraggingArea)
				mDraggingArea.setWidth(dragWidth);
						
			if (redraw)
			{
				mDirty = true;
				mFlags |= IGOIPC::WINDOW_UPDATE_BITS|IGOIPC::WINDOW_UPDATE_WIDTH|IGOIPC::WINDOW_UPDATE_HEIGHT;
				emit windowDirty(this);
			}
		}
    }
    
    mResizingHasAlreadyStarted = false;
}


void IGOQtContainer::resizeView(int w, int h)
{
	QSize size = mView->size();
	QSize newSize = QSize(w, h);
	if (size != newSize)
	{
		mView->resize(newSize);

		mDirty = true;
		mFlags |= IGOIPC::WINDOW_UPDATE_BITS|IGOIPC::WINDOW_UPDATE_WIDTH|IGOIPC::WINDOW_UPDATE_HEIGHT;
		emit windowDirty(this);
	}
}

void IGOQtContainer::addContextMenu(QWidget *menu, int x, int y)
{
	QSize menuSize = menu->sizeHint();
	QSize subMenuSize = QSize(0,0);
    if (mSubmenu)
        subMenuSize = mSubmenu->sizeHint();
//	foreach(QObject* child, menu->children())
//	{
//		if (child->inherits("QAction"))
//		{
//			QAction* childAction = dynamic_cast<QAction*>(child);
//			QWidget* submenu = dynamic_cast<QWidget*>(childAction->menu());
//			if (submenu)
//			{
//				subMenuSize = submenu->sizeHint();
//				break;
//			}
//		}
//	}
	QSize widgetSize = widget()->size();
	menuSize += QSize(x, y);
	subMenuSize += QSize(menuSize.width(), 0);
	menuSize += subMenuSize;
	if (widgetSize.width() < menuSize.width())
		widgetSize.rwidth() = menuSize.width();

	if (widgetSize.height() < menuSize.height())
		widgetSize.rheight() = menuSize.height();

	mContextMenu = menu;
    mContextMenu->move(x, y);
    
    ORIGIN_VERIFY_CONNECT(mContextMenu, SIGNAL(destroyed(QObject*)), this, SLOT(removeContextMenu()));
	menu->installEventFilter(this);
    
    resizeView(widgetSize.width(), widgetSize.height());
}

void IGOQtContainer::addContextSubmenu(QMenu* submenu)
{
    if (submenu)
    {
        mSubmenu = submenu;
        ORIGIN_VERIFY_CONNECT(mSubmenu, SIGNAL(destroyed(QObject*)), this, SLOT(removeContextSubmenu()));
	    mSubmenu->installEventFilter(this);
    }
}

void IGOQtContainer::removeContextMenu()
{
    if (mContextMenu)
        mContextMenu->removeEventFilter(this);
    mContextMenu = NULL;
    mSubmenu = NULL;
}

void IGOQtContainer::removeContextSubmenu()
{
    if (mSubmenu)
        mSubmenu->removeEventFilter(this);
    mSubmenu = NULL;
}

QRect &IGOQtContainer::rect(bool boundingBoxOnly)
{
	//TODO: here could be a problem, the rect might be larger than the actual widget
	static QRect rect;
		
	if(mImageData && boundingBoxOnly)
		return (rect=QRect(mPosition.x(), mPosition.y(), mImageData->width(), mImageData->height()));

	if(mWidget && !boundingBoxOnly)
		return (rect=QRect(mPosition.x(), mPosition.y(), mWidget->width(), mWidget->height()));


	return (rect=QRect(mPosition.x(), mPosition.y(), 0, 0));
}

bool IGOQtContainer::isTransparentPixel(const QPoint &pt)
{
	if(!mImageData || pt.x()<0 || pt.x()>=mImageData->width() || pt.y()<0 || pt.y()>=mImageData->height())
		return true;

	const uint *pixelPtr = (const uint*) mImageData->constBits();
	uint pixelData = *( pixelPtr + (pt.y()*mImageData->width()) + pt.x() );	// ARGB32


#ifdef _DEBUG
	if( (pixelData) != DEBUG_COLOR )
#else
	if( (pixelData) != 0 )
#endif
		return false;

	return true;
}

void IGOQtContainer::setSizing(const bool& isResizing)
{
    // If we are about to resize - emit a signal saying we are about to do it.
    // Only do it when we are starting. We don't need to spam signals
    if(isResizing == true && mSizing == false)
        emit resizing();
    mSizing = isResizing;
}

void IGOQtContainer::setMoving(const bool& isMoving)
{
    // If we are about to move - emit a signal saying we are about to do it.
    // Only do it when we are starting. We don't need to spam signals
    if(isMoving == true && mMoving == false)
        emit moving();
    mMoving = isMoving;
}

#if !defined(ORIGIN_MAC)

void IGOQtContainer::sendKeyEvent(uint keyMsgType, uint keyCodeWParam, uint keyCodeLParam)
{
	if(!mView)
		return;

	MSG tmpMsg={0};
	tmpMsg.hwnd=(HWND)mView->winId();
	tmpMsg.message=keyMsgType;
	tmpMsg.wParam=keyCodeWParam;
	tmpMsg.lParam=keyCodeLParam;

	QApplication::setActiveWindow(mView);

	TranslateMessage(&tmpMsg);
	DispatchMessage(&tmpMsg);
}

#endif

void IGOQtContainer::sendMouseEvent(QGraphicsSceneMouseEvent &event)
{
    if(!mView)
		return;
    
	// This is tricky. We always process mouse button release
	// because we want to process close button click on windows
	if (event.type() != QEvent::GraphicsSceneMouseRelease && isMouseGrabbing())
		return;
	
    // We still need to move the containers underneath the game window so that we can map global coordinates to local graphics views coordinates
    // (for example when dealing with context menus)
    QPoint scaledPosition;
    scaledPosition.setX(mPosition.rx() / (mCurrentMousePosScaling.rx()!=0.0f ? mCurrentMousePosScaling.rx() : 1.0f));
    scaledPosition.setY(mPosition.ry() / (mCurrentMousePosScaling.ry()!=0.0f ? mCurrentMousePosScaling.ry() : 1.0f));
    mView->move(scaledPosition + mCurrentMousePosOffset);

//#define HACK_TO_DISPLAY_CLICK_LOCATIONS
#ifdef HACK_TO_DISPLAY_CLICK_LOCATIONS
    if (event.type() == QEvent::GraphicsSceneMousePress)
    {
        IGO_TRACEF("Click at location %f, %f\n", event.pos().x(), event.pos().y());
        mView->scene()->addRect(event.pos().x(), event.pos().y(), 10, 10, QPen(Qt::SolidLine), QBrush(Qt::red));
        IGO_TRACEF("MOVE pos %d. %d - scalePos %d, %d - offset %d %d - total %d %d\n", mPosition.rx(), mPosition.ry(), scaledPosition.x(), scaledPosition.y()
                   , mCurrentMousePosOffset.x(), mCurrentMousePosOffset.y(), scaledPosition.x() + mCurrentMousePosOffset.x(), scaledPosition.y() + mCurrentMousePosOffset.y());
    }
#endif
    
    qt_sendSpontaneousEvent(mView->scene(), &event);
}

void IGOQtContainer::sendMouseEvent(QGraphicsSceneWheelEvent &event)
{
	if(!mView)
		return;
    
	if (event.type() != QEvent::GraphicsSceneWheel)
		return;
	
    // We still need to move the containers underneath the game window so that we can map global coordinates to local graphics views coordinates
    // (for example when dealing with context menus)
    QPoint scaledPosition;
    scaledPosition.setX(mPosition.rx() / (mCurrentMousePosScaling.rx()!=0.0f ? mCurrentMousePosScaling.rx() : 1.0f));
    scaledPosition.setY(mPosition.ry() / (mCurrentMousePosScaling.ry()!=0.0f ? mCurrentMousePosScaling.ry() : 1.0f));
    mView->move(scaledPosition + mCurrentMousePosOffset);

//#define HACK_TO_DISPLAY_CLICK_LOCATIONS
#ifdef HACK_TO_DISPLAY_CLICK_LOCATIONS
    if (event.type() == QEvent::GraphicsSceneMousePress)
    {
        IGO_TRACEF("Click at location %f, %f\n", event.pos().x(), event.pos().y());
        mView->scene()->addRect(event.pos().x(), event.pos().y(), 10, 10, QPen(Qt::SolidLine), QBrush(Qt::red));
        IGO_TRACEF("MOVE pos %d. %d - scalePos %d, %d - offset %d %d - total %d %d\n", mPosition.rx(), mPosition.ry(), scaledPosition.x(), scaledPosition.y()
                   , mCurrentMousePosOffset.x(), mCurrentMousePosOffset.y(), scaledPosition.x() + mCurrentMousePosOffset.x(), scaledPosition.y() + mCurrentMousePosOffset.y());
    }
#endif
    
    qt_sendSpontaneousEvent(mView->scene(), &event);
}


#ifdef ORIGIN_MAC

void IGOQtContainer::sendKeyEvent(QByteArray keyData)
{
	if(!mView)
		return;
    
	QApplication::setActiveWindow(mView);
    ForwardKeyData((void*)mView->winId(), keyData.data(), keyData.count());
}

#endif // MAC OSX

void IGOQtContainer::invalidate()
{
	mDirty = true;
	mFlags = IGOIPC::WINDOW_UPDATE_ALL;
	
	if (mWidget)
	{
		mWidget->repaint();
	}
	if (mView)
	{
		mView->invalidateScene();
	}
}

void IGOQtContainer::needFocusOnWidget()
{
	// if current container has focus and widget is not null
	// set focus to the widget
	if(mWidget
        && mHasFocus)
	{
		mWidget->setFocus();
	}
}
    
} // Engine
} // Origin
