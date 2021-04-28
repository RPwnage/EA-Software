// ShiftOfferListViewController.cpp
// Copyright 2014 Electronic Arts Inc. All rights reserved.

#include "ShiftOfferListViewController.h"
#include "services/debug/DebugService.h"
#include "OriginApplication.h"
#include "originwindow.h"
#include "originwebview.h"
#include "originpushbutton.h"
#include "originmsgbox.h"
#include "originwindowmanager.h"

#include "WebWidgetController.h"
#include "WebWidget/WidgetPage.h"
#include "DcmtDownloadManagerProxy.h"
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

ShiftOfferListViewController* ShiftOfferListViewController::sInstance = NULL;

void ShiftOfferListViewController::destroy()
{
    delete sInstance;
    sInstance = nullptr;
}


ShiftOfferListViewController* ShiftOfferListViewController::instance()
{
    if (!sInstance)
    {
        init();
    }

    return sInstance;
}


ShiftOfferListViewController::ShiftOfferListViewController(QWidget *parent)
    : QObject(parent)
    , mWindow(0)
    , mWebViewContainer(0)
    , mWebView(0)
{
}


ShiftOfferListViewController::~ShiftOfferListViewController()
{
    close();
}


void ShiftOfferListViewController::show()
{
    using namespace UIToolkit;
    using JsInterface::ShiftQueryProxy;
    using JsInterface::DcmtDownloadManagerProxy;

    if (!mWindow)
    {
        // Create our web view
        mWebView = new WebWidget::WidgetView();
        mWebViewContainer = new UIToolkit::OriginWebView();

        mWebViewContainer->setWebview(dynamic_cast<QWebView*>(mWebView));

        Client::NativeBehaviorManager::applyNativeBehavior(mWebView);

        mWebViewContainer->setWindowLoadingSize(QSize(800, 730));
        mWebViewContainer->setIsContentSize(true);
        mWebViewContainer->setHasSpinner(true);
        mWebViewContainer->setHScrollPolicy(Qt::ScrollBarAlwaysOff);
        mWebViewContainer->setVScrollPolicy(Qt::ScrollBarAlwaysOff);

        // Create a window containing the web view
        mWindow = new OriginWindow(static_cast<OriginWindow::TitlebarItems>(OriginWindow::Icon |
            OriginWindow::Minimize | OriginWindow::Close), mWebViewContainer);

        mWindow->setContentMargin(0);

        ORIGIN_VERIFY_CONNECT_EX(mWindow, SIGNAL(rejected()), this, SLOT(close()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT(mWebViewContainer->webview(), SIGNAL(titleChanged(const QString&)),
            this, SLOT(pageTitleChanged(const QString&)));

        mWindow->setTitleBarText("Shift Offer List");

        // Allow external network access through our network access manager
        mWebView->widgetPage()->setExternalNetworkAccessManager(new Client::OfflineCdnNetworkAccessManager(mWebView->page()));

        // Add a JavaScript object that we can use to talk to Shift
        WebWidget::NativeInterface shiftQueryProxy("shiftQuery", new ShiftQueryProxy(), true);
        mWebView->widgetPage()->addPageSpecificInterface(shiftQueryProxy);
        
        // Add a JavaScript object that we can use to manage downloads
        WebWidget::NativeInterface dcmtDownloadManagerProxy("dcmtDownloadManager", new DcmtDownloadManagerProxy());
        mWebView->widgetPage()->addPageSpecificInterface(dcmtDownloadManagerProxy);

        Origin::Engine::UserRef user = Origin::Engine::LoginController::instance()->currentUser();
        Client::WebWidgetController::instance()->loadUserSpecificWidget(mWebView->widgetPage(), "shiftofferlist", user);
    }

    mWindow->showWindow();
}


void ShiftOfferListViewController::pageTitleChanged(const QString& newTitle)
{
    if (mWindow)
    {
        mWindow->setTitleBarText(newTitle);
    }
}


bool ShiftOfferListViewController::close()
{
    if (mWindow)
    {
        ORIGIN_VERIFY_DISCONNECT(mWebViewContainer->webview(), SIGNAL(titleChanged(const QString&)),
            this, SLOT(pageTitleChanged(const QString&)));
        ORIGIN_VERIFY_DISCONNECT(mWindow, SIGNAL(rejected()), this, SLOT(close()));

        mWindow->close();
        delete mWindow;
        mWindow = nullptr;
    }

    return true;
}

} // namespace ShiftDirectDownload

} // namespace Plugins

} // namespace Origin
