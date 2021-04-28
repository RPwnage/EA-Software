/////////////////////////////////////////////////////////////////////////////
// WebBrowserViewController.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "WebBrowserViewController.h"

#include "services/settings/SettingsManager.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

#include "origintabwidget.h"
#include "originpushbutton.h"
#include "WebBrowserTabs.h"
#include "IGOWebBrowser.h"
#include "TelemetryAPIDLL.h"
#include <QGraphicsProxyWidget>
#include <QGraphicsView>

namespace Origin
{
namespace Client
{

namespace
{
    const QSize DEFAULT_SIZE = QSize(1024, 768);
}

WebBrowserViewController::WebBrowserViewController(const int& scope, Origin::Engine::IWebBrowserViewController::BrowserFlags creationFlags, QObject* parent)
: QObject(parent)
, mOigScope(scope)
, mCreationFlags(creationFlags)
#ifdef ORIGIN_MAC
, mKICWatcher(NULL)
#endif
, mInitialized(false)
{
    mDisableMultiTab = (creationFlags & Origin::Engine::IWebBrowserViewController::IGO_BROWSER_MINI_APP) != 0;

    using namespace UIToolkit;

    OriginWindow::TitlebarItems titlebarItems = OriginWindow::Icon | OriginWindow::Close;
    OriginWindow::WindowContext windowContext = OriginWindow::OIG;
    if(mOigScope == 0)
    {
        titlebarItems |= OriginWindow::Minimize;
        windowContext = OriginWindow::Normal;
    }

    mWindow = new OriginWindow(titlebarItems, NULL, OriginWindow::Custom, 
        QDialogButtonBox::NoButton, OriginWindow::ChromeStyle_Dialog, windowContext);
    mWindow->setTitleBarText(tr("ebisu_igo_web_browser"));
    mWindow->resize(DEFAULT_SIZE);
    
    // Since OriginWindow inherits multiple classes, explicitly cast it to a QDialog for use in QPointer
    mWindowGuard = static_cast<QDialog*>(mWindow);

    if(mOigScope != 0)
        mWindow->setIgnoreEscPress(true);

    WebBrowserTabs* content = new WebBrowserTabs(/*mWindow*/);
    mWindow->setContent(content->view());
    mWindow->installEventFilter(this);
    mWebBrowserTabs = content;
    mWebBrowserTabs->setParentWindow(mWindow);
    mWebBrowserTabs->setEventFilterObject(this);

    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SLOT(closeAllTabs()));
    ORIGIN_VERIFY_CONNECT(mWebBrowserTabs, SIGNAL(tabChanged(QGraphicsItem*)), this, SIGNAL(tabChanged(QGraphicsItem*)));
    ORIGIN_VERIFY_CONNECT(mWebBrowserTabs, SIGNAL(lastTabClosed()), this, SLOT(deleteLater()));
}

WebBrowserViewController::~WebBrowserViewController()
{
#ifdef ORIGIN_MAC
    Origin::Services::PlatformService::cleanupKICWatch(mKICWatcher);
#endif
    
    if(mWebBrowserTabs)
    {
        ORIGIN_VERIFY_DISCONNECT(mWebBrowserTabs, SIGNAL(tabChanged(QGraphicsItem*)), this, SIGNAL(tabChanged(QGraphicsItem*)));
        ORIGIN_VERIFY_DISCONNECT(mWebBrowserTabs, SIGNAL(lastTabClosed()), this, SLOT(deleteLater()));
        mWebBrowserTabs->deleteLater();
        mWebBrowserTabs = NULL;
    }

    if(mWindowGuard)
    {
        ORIGIN_VERIFY_DISCONNECT(mWindow, SIGNAL(rejected()), this, SLOT(closeAllTabs()));
        mWindow->deleteLater();
        mWindow = NULL;
    }

    setParent(NULL);
    GetTelemetryInterface()->Metric_IGO_BROWSER_END();
}

bool WebBrowserViewController::ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior)
{
    properties->setOpenAnimProps(Engine::IGOController::instance()->defaultOpenAnimation());
    properties->setCloseAnimProps(Engine::IGOController::instance()->defaultCloseAnimation());
    return true;
}

void WebBrowserViewController::init(const QString &url, bool reducedMode)
{
    Origin::Engine::IIGOWindowManager* igowm = Origin::Engine::IGOController::instance()->igowm();
    if(mOigScope != 0) // in OIG
    {
#ifdef ORIGIN_MAC
        mKICWatcher = Origin::Services::PlatformService::setupKICWatch(igowm->nativeWindow(mWindow), this);
#endif
    }
    mWebBrowserTabs->setReducedMode(reducedMode);

    // changing tabs
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(tabChanged(QGraphicsItem*)), this, SLOT(moveTabToFront(QGraphicsItem*)));

    // Make sure the individual tabs are clipped against the tab bar
    igowm->setWindowItemFlag(mWindow, mWebBrowserTabs->view()->tabBar(), QGraphicsItem::ItemClipsToShape, true);
}

void WebBrowserViewController::moveTabToFront(QGraphicsItem* tab)
{
    Origin::Engine::IGOController::instance()->igowm()->moveWindowItemToFront(mWindow, tab);
}
    
int WebBrowserViewController::tabCount() const
{
    if( mWebBrowserTabs )
    {
        return mWebBrowserTabs->count();
    }
    return 0;
}

BrowserGraphicsWebView* WebBrowserViewController::browserWebView(int index)
{
    return mWebBrowserTabs->browserWebView(index);
}

IGOWebBrowser* WebBrowserViewController::browser(int index)
{
    return mWebBrowserTabs->browser(index);
}

bool WebBrowserViewController::isTabBlank(int idx) const
{
    if (tabCount() > idx)
        return mWebBrowserTabs->browserWebView(idx)->isTabBlank();

    return false;
}

void WebBrowserViewController::setTabInitialUrl(int idx, const QString& url)
{
    if (tabCount() > idx)
        browser(idx)->setInitialURL(url);
}

bool WebBrowserViewController::isAddTabAllowed() const
{
    return !mDisableMultiTab && mWebBrowserTabs->isAddTabAllowed();
}

void WebBrowserViewController::addTab(const QString& url, bool blank)
{
    // If this is the first tab (ie we just created the view), we need to do some setup work
    if (!mInitialized)
    {
        mInitialized = true;
        
        bool reducedMode = Origin::Services::readSetting(Origin::Services::SETTING_IGOReducedBrowser);
        reducedMode |= mDisableMultiTab;
        init(url, reducedMode);
        
        mWebBrowserTabs->addTab(url, blank);
        
        // add to GraphicsScene. TODO: move this within 'mWebBrowserTabs->addTab(url)' call above
        //moveTabToFront(mWebBrowserTabs->browserWebView(0));
    }
    
    else
    {
        if (!mDisableMultiTab)
            mWebBrowserTabs->addTab(url, blank);

        else
            ORIGIN_LOG_ERROR << "Trying to add tab on single window browser for '" << url << '"';
    }
}

void WebBrowserViewController::showNavigation(bool bShow)
{
    ORIGIN_ASSERT(tabCount() >= 1);
    if( tabCount() >= 1 )
    {
        browser(0)->showNavigation(bShow);
    }
}

void WebBrowserViewController::setID(const QString& id)
{
    mID = id;
}

QString WebBrowserViewController::id()
{
    return mID;
}

bool WebBrowserViewController::isNavigationVisible()
{
    if( tabCount() >= 1 )
    {
        return browser(0)->isNavigationVisible();
    }

    return false;
}

QWebPage * WebBrowserViewController::page()
{
    ORIGIN_ASSERT(tabCount() >= 1);
    if( tabCount() >= 1 )
    {
        return browser(0)->page();
    }

    return NULL;
}

void WebBrowserViewController::closeAllTabs()
{
#if 1 // do not show confirmation dialog
    deleteLater();
#else
    // user confirmation needed only if more than one tab is open
    bool confirmed = mWebBrowserTabs->count() <= 1;
    if ( !confirmed )
    {
        using namespace Origin::UIToolkit;
        OriginWindow* messageBox = new OriginWindow(OriginWindow::Icon | OriginWindow::Close,
            NULL, OriginWindow::MsgBox, QDialogButtonBox::NoButton, 
            OriginWindow::ChromeStyle_Dialog, OriginWindow::OIG);
        messageBox->msgBox()->setup(OriginMsgBox::Notice, 
            tr("CLOSE TABS"), // ebisu_client_close_tabs
            tr("You are about to close %1 tabs, are you sure?").arg(mWebBrowserTabs->count()) // ebisu_client_close_tabs_are_you_sure
            );
        messageBox->addButton(QDialogButtonBox::Close);
        OriginPushButton *pCancelBtn =  messageBox->addButton(QDialogButtonBox::Cancel);
        messageBox->setDefaultButton(pCancelBtn);

        ORIGIN_VERIFY_CONNECT(messageBox, SIGNAL(rejected()), messageBox, SLOT(close()));
        ORIGIN_VERIFY_CONNECT(messageBox, SIGNAL(finished(int)), messageBox, SLOT(close()));
        ORIGIN_VERIFY_CONNECT(messageBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), messageBox, SLOT(accept()));
        ORIGIN_VERIFY_CONNECT(messageBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), messageBox, SLOT(reject()));

        IGOWindowManager::instance()->addPopupWindow(messageBox, true);
        IGOWindowManager::instance()->activateWindow(messageBox);
        confirmed = messageBox->exec() == QDialog::Accepted;

        // the browser loses focus to the dialog box, give it back
        IGOWindowManager::instance()->igowm()->setFocusWindow(mWebBrowserTabs->container());
    }
    if ( confirmed )
    {
        deleteLater();
    }
#endif
}

bool WebBrowserViewController::eventFilter(QObject * watched, QEvent * event)
{
    bool handled = false;
    if (isNavigationVisible())
    {
        switch (event->type())
        {
        case QEvent::KeyPress:
        {
            QKeyEvent * keyEvent = dynamic_cast<QKeyEvent*>(event);
            if (keyEvent)
                handled = handleKeyPressEvent(keyEvent);
            break;
        }

        default:
            break;
        };
    }

    if( handled )
    {
        event->accept();
        return true;
    }

    return QObject::eventFilter(watched, event);
}

bool WebBrowserViewController::handleKeyPressEvent(QKeyEvent* keyEvent)
{
    bool consume = false;
    if( keyEvent )
    {
        Qt::KeyboardModifiers mods = keyEvent->modifiers();
        int key = keyEvent->key();
        if ( mods == Qt::ControlModifier )
        {
            switch ( key )
            {
                case Qt::Key_T:
                    mWebBrowserTabs->addTab("", true);
                    consume = true;
                    break;
                case Qt::Key_Tab:
                    mWebBrowserTabs->nextTab();
                    consume = true;
                    break;  
                case Qt::Key_W:
                    mWebBrowserTabs->closeTab();
                    consume = true;
                    break;  
                case Qt::Key_1: case Qt::Key_2: case Qt::Key_3:
                case Qt::Key_4: case Qt::Key_5: case Qt::Key_6:
                case Qt::Key_7: case Qt::Key_8: case Qt::Key_9:
                    mWebBrowserTabs->activateTab(key - Qt::Key_1);
                    consume = true;
                    break;
                default:
                    break;
            }
        }
        else if ( mods == Qt::ControlModifier + Qt::ShiftModifier )
        {
            switch ( key )
            {
            case Qt::Key_Backtab: // the ShiftModifier turns the 'Tab' into a 'Backtab'
                mWebBrowserTabs->previousTab();
                consume = true;
                break;
            case Qt::Key_W:
                closeAllTabs();
                consume = true;
                break;  
            default:
                break;
            }
        }
    }

    return consume;
}

#ifdef ORIGIN_MAC
void WebBrowserViewController::KICWatchEvent(int key, Qt::KeyboardModifiers modifiers)
{
    if (isNavigationVisible())
    {
        if (key == Qt::Key_Tab)
        {
            if (modifiers == Qt::ControlModifier)
            {
                mWebBrowserTabs->nextTab();
            }
            
            else
            if (modifiers == (Qt::ControlModifier | Qt::ShiftModifier))
            {
                mWebBrowserTabs->previousTab();
            }
        }
    }
}
#endif

}

}
