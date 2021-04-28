#include "OriginWebController.h"

#include <QWidget>
#include <QtWebKitWidgets/QWebView>
#include <QtWebKitWidgets/QWebPage>
#include <QtWebKitWidgets/QWebFrame>

#include "OriginWebPage.h"
#include "NativeBehaviorManager.h"

namespace Origin
{
namespace Client
{

OriginWebController::OriginWebController(QWebView *webView, AbstractErrorPage *errorPage, bool handleOfflineAsError) : 
    QObject(webView)
{
    OriginWebPage *originPage = new OriginWebPage(webView);
    webView->setPage(originPage);
    init(webView, originPage, errorPage, handleOfflineAsError);
}

OriginWebController::OriginWebController(QWidget *viewWidget, QWebPage *page, AbstractErrorPage *errorPage, bool handleOfflineAsError) :
    QObject(page)
{
    init(viewWidget, page, errorPage, handleOfflineAsError);
}
    
PageErrorDetector* OriginWebController::errorDetector() const
{
    return mPageErrorHandler->errorDetector();
}
    
void OriginWebController::init(QWidget *viewWidget, QWebPage *page, AbstractErrorPage *errorPage, bool handleOfflineAsError)
{
    mPage = page;

    mJsBridge = new JavascriptCommunicationManager(page);

    // Set up our page error handler
    mPageErrorHandler = new PageErrorHandler(page, errorPage);

    // This is what we used to check for
    errorDetector()->enableErrorCheck(PageErrorDetector::FinishedWithFailure);
    errorDetector()->enableErrorCheck(PageErrorDetector::MainFrameRequestFailed);
    errorDetector()->enableErrorCheck(PageErrorDetector::LoadTimedOut);
    errorHandler()->setHandleOfflineAsError(handleOfflineAsError);

    // Act and look more native
    NativeBehaviorManager::applyNativeBehavior(viewWidget, page);
}

void OriginWebController::loadTrustedUrl(const QUrl &url)
{
    loadTrustedRequest(QNetworkRequest(url));
}
    
void OriginWebController::loadTrustedRequest(const QNetworkRequest &req, QNetworkAccessManager::Operation op, const QByteArray &body)
{
    mJsBridge->setTrustedHostname(req.url().host());
    // If we load a page while disconnected from the internet the main 
    // frame url is not saved because the web page didn't load.
    errorHandler()->setRequestToRestore(req, op, body);
    mPage->mainFrame()->load(req, op, body);
}


}
}
