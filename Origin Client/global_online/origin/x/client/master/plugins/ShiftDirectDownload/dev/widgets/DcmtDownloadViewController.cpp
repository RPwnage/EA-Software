// DcmtDownloadViewController.cpp
// Copyright 2014 Electronic Arts Inc. All rights reserved.

#include "DcmtDownloadViewController.h"
#include "DcmtDownloadManagerProxy.h"
#include "services/debug/DebugService.h"
#include "originwindow.h"
#include "originwebview.h"

#include "WebWidgetController.h"
#include "WebWidget/WidgetPage.h"

#include "services/settings/SettingsManager.h"
#include "flows/ClientFlow.h"
#include "widgets/notifications/source/OriginToastManager.h"

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
    
QList<DcmtDownloadViewController*> DcmtDownloadViewController::sInstances;

DcmtDownloadViewController::DcmtDownloadViewController(DcmtFileDownloader *download, QObject *parent)
    : QObject(parent)
    , mDownloadWindow(nullptr)
    , mDownload(download)
{
    ORIGIN_VERIFY_CONNECT(mDownload, SIGNAL(error(const QString&)), this, SLOT(onDownloadError()));
    ORIGIN_VERIFY_CONNECT(mDownload, SIGNAL(finished(const QString&)), this, SLOT(onDownloadComplete()));

    sInstances.append(this);
}


DcmtDownloadViewController::~DcmtDownloadViewController()
{
    close();
}


void DcmtDownloadViewController::destroy()
{
    for(QList<DcmtDownloadViewController*>::iterator it = sInstances.begin(); it != sInstances.end(); ++it)
    {
        DcmtDownloadViewController* inst = (*it);
        if(inst)
        {
            delete inst;
        }
    }

    sInstances.clear();
}


void DcmtDownloadViewController::show()
{
    using namespace UIToolkit;
    using JsInterface::DcmtDownloadManagerProxy;

    if (!mDownloadWindow)
    {
        // Create our web view
        WebWidget::WidgetView *webView = new WebWidget::WidgetView();
        UIToolkit::OriginWebView *webViewContainer = new UIToolkit::OriginWebView();

        webViewContainer->setWebview(dynamic_cast<QWebView*>(webView));

        Client::NativeBehaviorManager::applyNativeBehavior(webView);

        webViewContainer->setWindowLoadingSize(QSize(512, 212));
        webViewContainer->setIsContentSize(true);
        webViewContainer->setHasSpinner(true);
        webViewContainer->setShowSpinnerAfterInitLoad(false);
        webViewContainer->setHScrollPolicy(Qt::ScrollBarAlwaysOff);
        webViewContainer->setVScrollPolicy(Qt::ScrollBarAlwaysOff);

        // Create a window containing the web view
        mDownloadWindow = new OriginWindow(static_cast<OriginWindow::TitlebarItems>(OriginWindow::Icon |
            OriginWindow::Minimize | OriginWindow::Close), webViewContainer);

        mDownloadWindow->setContentMargin(0);

        ORIGIN_VERIFY_CONNECT_EX(mDownloadWindow, SIGNAL(rejected()), this, SLOT(close()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT(webViewContainer->webview(), SIGNAL(titleChanged(const QString&)),
            mDownloadWindow, SLOT(setTitleBarText(const QString&)));

        mDownloadWindow->setTitleBarText("Downloading Build");

        // Allow external network access through our network access manager
        webView->widgetPage()->setExternalNetworkAccessManager(new Client::OfflineCdnNetworkAccessManager(webView->page()));
        
        // Add a JavaScript object that we can use to manage downloads
        WebWidget::NativeInterface dcmtDownloadManagerProxy("dcmtDownload", mDownload);
        webView->widgetPage()->addPageSpecificInterface(dcmtDownloadManagerProxy);

        Client::WebWidgetController::instance()->loadSharedWidget(webView->widgetPage(), "shiftdownload");
    }

    mDownloadWindow->showWindow();
}


void DcmtDownloadViewController::close()
{
    if (mDownloadWindow)
    {
        ORIGIN_VERIFY_DISCONNECT(mDownloadWindow, SIGNAL(rejected()), this, SLOT(close()));
        ORIGIN_VERIFY_DISCONNECT(mDownload, SIGNAL(error(const QString&)), this, SLOT(onDownloadError()));
        ORIGIN_VERIFY_DISCONNECT(mDownload, SIGNAL(finished(const QString&)), this, SLOT(onDownloadComplete()));

        mDownloadWindow->close();
        delete mDownloadWindow;
        mDownloadWindow = nullptr;
    }
}


void DcmtDownloadViewController::onDownloadError()
{
    using namespace Client;  
    ClientFlow* clientFlow = Client::ClientFlow::instance();
    if (!clientFlow)
    {
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        return;
    }

    otm->showToast(Services::SETTING_NOTIFY_DOWNLOADERROR.name(),
        QString("<b>%0</b>").arg(mDownload->fileName()),
        "Shift download failed");
}


void DcmtDownloadViewController::onDownloadComplete()
{
    using namespace Client;
    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        return;
    }

    otm->showToast(Services::SETTING_NOTIFY_FINISHEDDOWNLOAD.name(),
        QString("<b>%0</b>").arg(mDownload->fileName()),
        "Shift download complete");
}

} // namespace ShiftDirectDownload

} // namespace Plugins

} // namespace Origin
