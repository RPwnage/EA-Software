// LoginViewController.cpp
// Copyright 2014 Electronic Arts Inc. All rights reserved.

#include "ShiftLoginViewController.h"
#include "services/debug/DebugService.h"
#include "OriginApplication.h"
#include "originwindow.h"
#include "originwebview.h"
#include "originpushbutton.h"
#include "originmsgbox.h"
#include "originwindowmanager.h"

#include "WebWidgetController.h"
#include "WebWidget/WidgetPage.h"
#include "engine/login/LoginController.h"

#include "Services/Settings/SettingsManager.h"
#include "NativeBehaviorManager.h"
#include "OfflineCdnNetworkAccessManager.h"

#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKitWidgets/QWebView>

namespace Origin
{

namespace Plugins
{

namespace ShiftDirectDownload
{

ShiftLoginViewController* ShiftLoginViewController::sInstance = NULL;

void ShiftLoginViewController::destroy()
{
    delete sInstance;
    sInstance = nullptr;
}


ShiftLoginViewController* ShiftLoginViewController::instance()
{
    if (!sInstance)
    {
        init();
    }

    return sInstance;
}


ShiftLoginViewController::ShiftLoginViewController(QWidget *parent)
    : QObject(parent)
    , mLoginWindow(0)
    , mWebViewContainer(0)
    , mWebView(0)
{
}


ShiftLoginViewController::~ShiftLoginViewController()
{
    close();
}


void ShiftLoginViewController::show()
{
    using namespace UIToolkit;
    using JsInterface::ShiftQueryProxy;

    if (!mLoginWindow)
    {
        // Create our web view
        mWebView = new WebWidget::WidgetView();
        mWebViewContainer = new UIToolkit::OriginWebView();

        mWebViewContainer->setWebview(dynamic_cast<QWebView*>(mWebView));

        Client::NativeBehaviorManager::applyNativeBehavior(mWebView);

        mWebViewContainer->setWindowLoadingSize(QSize(360, 200));
        mWebViewContainer->setIsContentSize(true);
        mWebViewContainer->setHasSpinner(true);
        mWebViewContainer->setShowSpinnerAfterInitLoad(false);
        mWebViewContainer->setHScrollPolicy(Qt::ScrollBarAlwaysOff);
        mWebViewContainer->setVScrollPolicy(Qt::ScrollBarAlwaysOff);

        // Create a window containing the web view
        mLoginWindow = new OriginWindow(static_cast<OriginWindow::TitlebarItems>(OriginWindow::Icon |
            OriginWindow::Minimize | OriginWindow::Close), mWebViewContainer);

        mLoginWindow->setContentMargin(0);

        ORIGIN_VERIFY_CONNECT_EX(mLoginWindow, SIGNAL(rejected()), this, SLOT(close()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT(mWebViewContainer->webview(), SIGNAL(titleChanged(const QString&)),
            this, SLOT(pageTitleChanged(const QString&)));

        mLoginWindow->setTitleBarText("Shift Login");

        // Allow external network access through our network access manager
        mWebView->widgetPage()->setExternalNetworkAccessManager(new Client::OfflineCdnNetworkAccessManager(mWebView->page()));

        // Add a JavaScript object that we can use to talk to Shift
        WebWidget::NativeInterface shiftQueryProxy("shiftQuery", new ShiftQueryProxy(), true);
        mWebView->widgetPage()->addPageSpecificInterface(shiftQueryProxy);

        Client::WebWidgetController::instance()->loadSharedWidget(mWebView->widgetPage(), "shiftlogin");
    }

    mLoginWindow->showWindow();
}


void ShiftLoginViewController::pageTitleChanged(const QString& newTitle)
{
    if (mLoginWindow)
    {
        mLoginWindow->setTitleBarText(newTitle);
    }
}


bool ShiftLoginViewController::close()
{
    if (mLoginWindow)
    {
        ORIGIN_VERIFY_DISCONNECT(mWebViewContainer->webview(), SIGNAL(titleChanged(const QString&)),
            this, SLOT(pageTitleChanged(const QString&)));
        ORIGIN_VERIFY_DISCONNECT(mLoginWindow, SIGNAL(rejected()), this, SLOT(close()));

        mLoginWindow->close();
        delete mLoginWindow;
        mLoginWindow = nullptr;
    }

    return true;
}

} // namespace ShiftDirectDownload

} // namespace Plugins

} // namespace Origin
