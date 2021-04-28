//////////////////////////////////////////////////////////////////////////////
// SearchView.cpp
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "SearchView.h"
#include "services/debug/DebugService.h"
#include "services/network/NetworkAccessManager.h"
#include "engine/login/LoginController.h"
#include "engine/igo/IGOController.h"
#include "originwindow.h"
#include "originwebview.h"
#include "SocialJsHelper.h"
#include "OriginWebController.h"
#include "OriginWebPage.h"
#include <QWebFrame>
#include <QWebHistory>
#include <QUrl>
#include <QWebPage>
#include <QWebView>
#include <QWebSettings>

namespace Origin
{
namespace Client
{

SearchView::SearchView(const UIScope& scope, QWidget* parent) 
: QWebView(parent)
, mWebController(NULL)
, mScope(scope)
{
    auto searchPage = new SearchWebPage(scope, this);
    searchPage->setNetworkAccessManager(Services::NetworkAccessManager::threadDefaultInstance());

    setPage(searchPage);

    settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);
    // Use this obscure constructor so OriginWebController doesn't override our page
    mWebController = new OriginWebController(this, page());
    mWebController->errorDetector()->disableErrorCheck(PageErrorDetector::FinishedWithFailure);
    mWebController->errorHandler()->setHandleOfflineAsError(true);
}


SearchView::~SearchView()
{
}


void SearchView::loadTrustedUrl(const QString& urlStr)
{
    mWebController->loadTrustedUrl(QUrl(urlStr));
}


void SearchView::showError(const QString& title, const QString& error)
{
    QFile resourceFile(QString(":html/errorPage_dialog.html"));
    if (!resourceFile.open(QIODevice::ReadOnly))
    {
        ORIGIN_ASSERT(0);
        return;
    }

    const QString p = QString::fromUtf8(resourceFile.readAll())
        // Replace our stylesheet
        .arg(UIToolkit::OriginWebView::getErrorStyleSheet(false))
        .arg(title)
        .arg(error);

    mWebController->page()->mainFrame()->setHtml(p);
}



SearchWebPage::SearchWebPage(const UIScope& scope, QObject* parent) 
: OriginWebPage(parent)
, mScope(scope)
{
}


QWebPage* SearchWebPage::createWindow(QWebPage::WebWindowType)
{
    using namespace UIToolkit;
    OriginWindow::TitlebarItems titlebarItems = OriginWindow::Icon | OriginWindow::Close;
    OriginWindow::WindowContext windowContext = OriginWindow::OIG;
    if(mScope != IGOScope)
    {
        windowContext = OriginWindow::Normal;
        titlebarItems |= OriginWindow::Minimize;
    }
    OriginWindow* myWindow = new OriginWindow(titlebarItems, NULL, OriginWindow::WebContent,
        QDialogButtonBox::NoButton, OriginWindow::ChromeStyle_Dialog, windowContext);
    myWindow->webview()->setWindowLoadingSize(QSize(409,400));
    myWindow->webview()->setPreferredContentsSize(QSize(409,400));
    myWindow->webview()->setHasSpinner(true);
    myWindow->webview()->setIsSelectable(false);
    myWindow->webview()->setVScrollPolicy(Qt::ScrollBarAlwaysOff);
    myWindow->webview()->setHScrollPolicy(Qt::ScrollBarAlwaysOff);
    myWindow->webview()->setChangeLoadSizeAfterInitLoad(true);
    myWindow->webview()->setUsesOriginScrollbars(false);
    ORIGIN_VERIFY_CONNECT(myWindow, SIGNAL(rejected()), myWindow, SLOT(close()));
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(destroyed(QObject*)), myWindow, SLOT(close()));
    ORIGIN_VERIFY_CONNECT(myWindow->webview()->webview(), SIGNAL(urlChanged(const QUrl&)), this, SLOT(onUrlChanged(const QUrl&)));

    if(mScope == IGOScope)
    {
        Engine::IGOController::instance()->igowm()->addPopupWindow(myWindow, Engine::IIGOWindowManager::WindowProperties());
    }
    else
    {
        myWindow->showWindow();
    }
    
    auto searchPage = new SearchWebPage(mScope, myWindow);
    searchPage->setNetworkAccessManager(Services::NetworkAccessManager::threadDefaultInstance());
    myWindow->webview()->webview()->setPage(searchPage);

    OriginWebController* webController = new OriginWebController(myWindow->webview()->webview(), searchPage);
    webController->errorDetector()->disableErrorCheck(PageErrorDetector::FinishedWithFailure);
    webController->errorHandler()->setHandleOfflineAsError(true);
    webController->errorHandler()->setErrorPage(DefaultErrorPage::centerAlignedPage());

    ORIGIN_VERIFY_CONNECT(
        searchPage, SIGNAL(windowCloseRequested()),
        myWindow, SLOT(close()));

    return searchPage;
}


void SearchWebPage::onUrlChanged(const QUrl& url)
{
    QWebView* webview = dynamic_cast<QWebView*>(sender());
    if(webview && webview->parentWidget())
    {
        UIToolkit::OriginWebView* webContainer = dynamic_cast<UIToolkit::OriginWebView*>(webview->parentWidget());
        if(webContainer)
        {
            if(url.toString() != "" && url.host().contains("live") == false)
                webContainer->setUsesOriginScrollbars(true);

            webContainer->setHScrollPolicy(Qt::ScrollBarAlwaysOff);
            webContainer->setVScrollPolicy(Qt::ScrollBarAsNeeded);
        }
    }
}

} // Client
} // Origin
