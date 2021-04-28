/////////////////////////////////////////////////////////////////////////////
// WebBrowserTabs.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "WebBrowserTabs.h"
#include "../igo/source/IGOWebBrowser.h"
#include "origintabwidget.h"
#include "originwindow.h"
#include "TelemetryAPIDLL.h"
#include "engine/igo/IGOController.h"
#include "Services/debug/DebugService.h"

namespace Origin
{
namespace Client
{

WebBrowserTabs::WebBrowserTabs(QObject* parent)
: QObject(parent)
, mTabWidget(NULL)
, mParentWindow(NULL)
, mEventFilterObject(NULL)
, mReducedMode(false)
{
    mTabWidget = new UIToolkit::OriginTabWidget();
    mTabWidget->setMovable(true);
    mTabWidget->setTabsClosable(false);
    mTabWidget->setDocumentMode(true);
    mTabWidget->setUsesScrollButtons(true);
    mTabWidget->setDrawBase(false);
    mTabWidget->setExpanding(false);
    mTabWidget->setTabBarVisible(true);
    // Since OriginTabWidget inherits multiple classes, explicitly cast it to a QWidget for use in QPointer
    mTabWidgetGuard = static_cast<QWidget*>(mTabWidget);

    ORIGIN_VERIFY_CONNECT(mTabWidget, SIGNAL(addTabClicked()), this, SLOT(onAddTabClicked()));
    ORIGIN_VERIFY_CONNECT(mTabWidget, SIGNAL(currentChanged(int)), this, SLOT(onTabChanged(int)));
    ORIGIN_VERIFY_CONNECT(mTabWidget, SIGNAL(tabClosed(int)), this, SLOT(onTabClosed(int)));
    ORIGIN_VERIFY_CONNECT(mTabWidget, SIGNAL(lastTabClosed()), this, SIGNAL(lastTabClosed()));
}

WebBrowserTabs::~WebBrowserTabs()
{
    if(mTabWidgetGuard)
    {
        ORIGIN_VERIFY_DISCONNECT(mTabWidget, SIGNAL(addTabClicked()), this, SLOT(onAddTabClicked()));
        ORIGIN_VERIFY_DISCONNECT(mTabWidget, SIGNAL(currentChanged(int)), this, SLOT(onTabChanged(int)));
        ORIGIN_VERIFY_DISCONNECT(mTabWidget, SIGNAL(tabClosed(int)), this, SLOT(onTabClosed(int)));
        ORIGIN_VERIFY_DISCONNECT(mTabWidget, SIGNAL(lastTabClosed()), this, SIGNAL(lastTabClosed()));
        mTabWidget->deleteLater();
        mTabWidget = NULL;
    }
}

void WebBrowserTabs::setTabText(QWidget* widget, const QString &text)
{
    if(mTabWidget)
    {
        int index = mTabWidget->indexOf(widget);
        mTabWidget->setTabText(index, text);
    }
}

void WebBrowserTabs::setTabIcon(QWidget* widget, const QIcon &icon)
{
    if(mTabWidget)
    {
        int index = mTabWidget->indexOf(widget);
        mTabWidget->setTabIcon(index, icon);
    }
}

bool WebBrowserTabs::isAddTabAllowed() const
{
    return mTabWidget->allowAddTab();
}

void WebBrowserTabs::addTab(const QString& url, bool blank /*=false*/)
{
    IGOWebBrowser* browser = new IGOWebBrowser();
    ORIGIN_ASSERT(browser);
    if( browser )
    {
        GetTelemetryInterface()->Metric_IGO_BROWSER_ADDTAB();

        browser->setTabWidget(mTabWidget);
        browser->setReducedMode(mReducedMode);
        browser->setupContextMenu(mParentWindow);
        if( blank )
            browser->setBlankPage();
        else
            browser->setInitialURL(url);

        mTabWidget->addTab(browser, tr("ebisu_client_new_tab"));

        browser->browser()->setupContextMenu(mParentWindow, browser);
        browser->browser()->installEventFilter(mEventFilterObject);

        // Combo box will need actual size of graphics view to limit the size of combo boxes
        browser->browser()->setProperty("OriginWindowContainer", qVariantFromValue((void*)browser->browser()));

        ORIGIN_VERIFY_CONNECT(browser->browser(), SIGNAL(cursorChanged(int)), this, SLOT(setCursor(int)));
        // for webview's focus
        ORIGIN_VERIFY_CONNECT(browser, SIGNAL(needFocusOnBrowser()), this, SLOT(needFocusOnBrowser()));
#ifdef ORIGIN_MAC
        // Only enable move/resize when we're outside the browser content - this is to prevent conflicts when setting the cursor shape
        ORIGIN_VERIFY_CONNECT(browser->browser(), SIGNAL(enteringWebContent()), this, SLOT(enteringWebContent()));
        ORIGIN_VERIFY_CONNECT(browser->browser(), SIGNAL(leavingWebContent()), this, SLOT(leavingWebContent()));
#endif
        // tab and window title
        ORIGIN_VERIFY_CONNECT(browser, SIGNAL(titleChanged(IGOWebBrowser*, const QString&)), this, SLOT(onTitleChanged(IGOWebBrowser*, const QString&)));
        // favicon
        ORIGIN_VERIFY_CONNECT(browser, SIGNAL(iconChanged(IGOWebBrowser*, const QIcon&)), this, SLOT(onIconChanged(IGOWebBrowser*, const QIcon&)));
        ORIGIN_VERIFY_CONNECT(browser, SIGNAL(closeTabRequested()), this, SLOT(onCloseTabRequested()));
        
        if (Origin::Engine::IGOController::instance()->showWebInspectors())
            Origin::Engine::IGOController::instance()->showWebInspector(browser->browser()->page());
    }
}

void WebBrowserTabs::enteringWebContent()
{
    Origin::Engine::IGOController::instance()->igowm()->enableWindowFrame(mParentWindow, false);
}
    
void WebBrowserTabs::leavingWebContent()
{
    Origin::Engine::IGOController::instance()->igowm()->enableWindowFrame(mParentWindow, true);
}

void WebBrowserTabs::setCursor(int cursorShape)
{
    Origin::Engine::IGOController::instance()->igowm()->setCursor(cursorShape);
}

void WebBrowserTabs::needFocusOnBrowser()
{
    Origin::Engine::IGOController::instance()->igowm()->setFocusWindow(mParentWindow);
}
    
void WebBrowserTabs::nextTab()
{
    int index = mTabWidget->currentIndex();
    index++;
    if( index >= mTabWidget->count() )
        index = 0;

    mTabWidget->setCurrentIndex(index);
}

void WebBrowserTabs::previousTab()
{
    int index = mTabWidget->currentIndex();
    index--;
    if( index < 0 )
        index = mTabWidget->count()-1;

    mTabWidget->setCurrentIndex(index);
}

void WebBrowserTabs::activateTab(int index)
{
    mTabWidget->setCurrentIndex(index);
}

void WebBrowserTabs::closeTab()
{
    int index = mTabWidget->currentIndex();
    mTabWidget->closeTab(index);
}

void WebBrowserTabs::setReducedMode(bool reduced)
{
    mReducedMode = reduced;
    mTabWidget->setTabBarVisible(!mReducedMode);
}

int WebBrowserTabs::count() const
{
    return mTabWidget->count();
}

IGOWebBrowser* WebBrowserTabs::browser(int index) const
{
    return (static_cast<IGOWebBrowser*>(mTabWidget->widget(index)));
}

BrowserGraphicsWebView* WebBrowserTabs::browserWebView(int index) const
{
    return (static_cast<IGOWebBrowser*>(mTabWidget->widget(index)))->browser();
}

void WebBrowserTabs::onTitleChanged(IGOWebBrowser* browser, const QString& title)
{
    setTabText(browser, title);
}

void WebBrowserTabs::onIconChanged(IGOWebBrowser* browser, const QIcon& icon)
{
    setTabIcon(browser, icon);
}

void WebBrowserTabs::onTabChanged(int index)
{
    if( mTabWidget && index >= 0 )
    {
        // Bring the appropriate webview to the top of the GraphicsScene
        // so that it can be seen
        BrowserGraphicsWebView* webview = browserWebView(index);
        emit( tabChanged(webview) );

        // Check whether the webview has a valid content - if so, give it focus, otherwise give focus to the URL bar
        // Emit since we are in the processing of the click/tab event
        emit browser(index)->initializeFocus();
    }
}

void WebBrowserTabs::onTabClosed(int index)
{
    GetTelemetryInterface()->Metric_IGO_BROWSER_CLOSETAB();
}

void WebBrowserTabs::onAddTabClicked()
{
    addTab("", true /*blank*/);
}

void WebBrowserTabs::onCloseTabRequested()
{
    IGOWebBrowser* browser = dynamic_cast<IGOWebBrowser*>(sender());
    if(browser)
    {
        int index = mTabWidget->indexOf(browser);
        mTabWidget->closeTab(index);
    }
}

}
}