//////////////////////////////////////////////////////////////////////////////
// StoreView.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "StoreView.h"
#include "StoreWebPage.h"

#include "StoreJsHelper.h"
#include "CommonJsHelper.h"

#include "OriginWebController.h"

#include "services/session/SessionService.h"
#include "services/connection/ConnectionStatesService.h"
#include "WebWidget/FeatureRequest.h"
#include "services/qt/QtUtil.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "OriginWebPage.h"
#include "ExposedWidgetDetector.h"
#include "TelemetryAPIDLL.h"
#ifdef ORIGIN_MAC
#include "OriginApplicationDelegateOSX.h"
#endif

#include "Qt/originwindow.h"
#include "Qt/originwebview.h"

#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKit/QWebHistory>
#include <QUrl>
#include <QNetworkCookie>
#include <QtWebKitWidgets/QWebPage>
#include <QtWebKitWidgets/QWebView>
#include <QtWebKit/QWebSettings>
#include <QColor>
#include <QEvent>
#include <QPaintEvent>
#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrinter>

namespace Origin
{
namespace Client
{

namespace
{

class StoreErrorPage : public DefaultErrorPage 
{
public:
    StoreErrorPage() : DefaultErrorPage(DefaultErrorPage::CenterAlign)
    {
    }

    QString offlineErrorHtml() const
    {
        if (Services::Connection::ConnectionStatesService::isRequiredUpdatePending())
        {
            return buildErrorPage(
                QObject::tr("ebisu_login_offline_mode_title"),
                QObject::tr("ebisu_client_update_offline_requires_restart_description"));
        }

        // Use the default
        return Client::DefaultErrorPage::offlineErrorHtml();
    }
};
}

StoreView::StoreView(QWidget *parent) :
    QWebView(parent), 
    mHasBeenExposed(false)
{
    // Set a page to allow popups
    // As of Oct 2012 this can happen during a FAQ in the checkout flow
    setPage(new StoreWebPage(this));
    settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);
    settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    
    // Use this obscure constuctor so OriginWebController doesn't override our page
    mWebController = new OriginWebController(this, page());

    mWebController->jsBridge()->addHelper("storeHelper", new StoreJsHelper(this));
    mWebController->jsBridge()->addHelper("commonHelper", new CommonJsHelper(this));
    WebWidget::FeatureRequest clientNavigation("http://widgets.dm.origin.com/features/interfaces/clientNavigation");
    mWebController->jsBridge()->addFeatureRequest(clientNavigation);
    WebWidget::FeatureRequest telemetryClient("http://widgets.dm.origin.com/features/interfaces/telemetryClient");
    mWebController->jsBridge()->addFeatureRequest(telemetryClient);
    mWebController->errorHandler()->setHandleOfflineAsError(true);
    mWebController->errorHandler()->setErrorPage(DefaultErrorPage::centerAlignedPage());

    ORIGIN_VERIFY_CONNECT(mWebController->jsBridge()->helper("storeHelper"), SIGNAL(printRequested()), this, SLOT(onPrintRequested()));
    ORIGIN_VERIFY_CONNECT(page(), SIGNAL(printRequested(QWebFrame*)), this, SLOT(onPrintRequested()));

    // Track when page loading starts so we can record how long it takes to load
    ORIGIN_VERIFY_CONNECT(page()->mainFrame(), SIGNAL(loadStarted()), this, SLOT(onStoreViewLoadStarted()));

    // Stop blanking once the page has loaded
    ORIGIN_VERIFY_CONNECT(page()->mainFrame(), SIGNAL(loadFinished(bool)), this, SLOT(onStoreViewLoadFinished(bool)));

    // Stop animations when we're exposed and unexposed
    mExposureDetector = new ExposedWidgetDetector(parentWidget()); 
   
    ORIGIN_VERIFY_CONNECT(mExposureDetector, SIGNAL(unexposed()),
            this, SLOT(stopAnimations()));

    ORIGIN_VERIFY_CONNECT(mExposureDetector, SIGNAL(exposed()),
            this, SLOT(exposed()));
}

void StoreView::load(const QUrl &url)
{
    QNetworkRequest req;
    req.setUrl(url);

    mWebController->loadTrustedRequest(req);
}

void StoreView::loadStoreUrl(const QUrl &wantedUrl)
{
    // If this hasn't been exposed and we aren't trying to do our initial load.
    if (!mHasBeenExposed)
    {
        // Load this URL the first time we're shown
        mLoadOnExpose = wantedUrl;
        return;
    }

    if (wantedUrl != url())
    {
        // Hide the current page so the user doesn't think whatever was here
        // previously is the final page
        load(wantedUrl);
    }
}

void StoreView::loadEntryUrlAndExpose(const QUrl &url)
{
    //Mark that this has been shown, so that on a repaint event we don't try to load a URL again
    //used in cases where a URL is already loading and then the store tab is show for the first time (triggered by code) 
    //example: when the user clicks on the button in the promo manager right after launching Origin
    mHasBeenExposed = true;
    load(url);
}

void StoreView::onStoreViewLoadStarted()
{
    // Keep track of when page loading starts so we can track how long it takes to load
    mStorePageLoadTimer.start();
}
    
void StoreView::onStoreViewLoadFinished(bool success)
{
    // Stop animations if we're hidden
    if (!mExposureDetector->isExposed())
    {
        stopAnimations();
    }

    // Report how long it took to load the store page, and whether it loaded successfully
    GetTelemetryInterface()->Metric_STORE_LOAD_STATUS(Services::EncodeUrlForTelemetry(page()->mainFrame()->requestedUrl()).data(), 
        mStorePageLoadTimer.elapsed(), success);
}

void StoreView::exposed()
{
    if (!mHasBeenExposed)
    {
        // We've been asked to paint. This means someone is actually looking
        // at us. Load the content that's supposed to be visible
        mHasBeenExposed = true;
        if (!mLoadOnExpose.isEmpty())
        {
            load(mLoadOnExpose);
        }
    }
    
    // Restart our animations
    page()->mainFrame()->evaluateJavaScript("jQuery.fx.off = false;");
}

void StoreView::stopAnimations()
{
    // Stop all animations
    page()->mainFrame()->evaluateJavaScript("jQuery.fx.off = true;");
}

void StoreView::onPrintRequested()
{
    QPrinter printer;
    printer.setOutputFormat(QPrinter::NativeFormat);
    QPrintDialog printDialog(&printer, this);
    if (printDialog.exec() == QDialog::Accepted)
    {
        print(printDialog.printer());
    }
}

void StoreView::showError(const QString& title, const QString &error)
{
    QFile resourceFile(QString(":html/errorPage.html"));
	if (!resourceFile.open(QIODevice::ReadOnly))
	{
		ORIGIN_ASSERT(0);
	        return;
	}

	// Replace our stylesheet
	QString stylesheet = UIToolkit::OriginWebView::getErrorStyleSheet(true);

	QString p = QString::fromUtf8(resourceFile.readAll())
	.arg(stylesheet)
	.arg(title)
	.arg(error);

    mWebController->page()->mainFrame()->setHtml(p);
}

}
}
