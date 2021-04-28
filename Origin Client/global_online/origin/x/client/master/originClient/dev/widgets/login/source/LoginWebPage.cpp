//////////////////////////////////////////////////////////////////////////////
// LoginWebPage.cpp
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "LoginWebPage.h"
#include "originwindow.h"
#include "originwebview.h"
#include "OriginWebController.h"
#include <QWebPage>
#include "services/debug/DebugService.h"
#include "services/network/NetworkAccessManager.h"

namespace Origin
{
namespace Client
{
    LoginWebPage::LoginWebPage(QObject* parent) 
        : OriginWebPage(parent)
    {
    }


    QWebPage* LoginWebPage::createWindow(QWebPage::WebWindowType)
    {

        using namespace UIToolkit;
        OriginWindow::TitlebarItems titlebarItems = OriginWindow::Icon | OriginWindow::Close;
        OriginWindow::WindowContext windowContext = OriginWindow::Normal;
        OriginWindow* myWindow = new OriginWindow(titlebarItems, NULL, OriginWindow::WebContent,
            QDialogButtonBox::NoButton, OriginWindow::ChromeStyle_Dialog, windowContext);
        myWindow->webview()->setWindowLoadingSize(QSize(450, 270));
        myWindow->webview()->setPreferredContentsSize(QSize(450,270));
        myWindow->webview()->setHasSpinner(true);
        myWindow->webview()->setIsSelectable(false);
        myWindow->webview()->setVScrollPolicy(Qt::ScrollBarAlwaysOff);
        myWindow->webview()->setHScrollPolicy(Qt::ScrollBarAlwaysOff);
        myWindow->webview()->setChangeLoadSizeAfterInitLoad(true);
        myWindow->webview()->setUsesOriginScrollbars(false);
        ORIGIN_VERIFY_CONNECT(myWindow, SIGNAL(rejected()), myWindow, SLOT(close()));
        ORIGIN_VERIFY_CONNECT(this, SIGNAL(destroyed(QObject*)), myWindow, SLOT(close()));
        ORIGIN_VERIFY_CONNECT(myWindow->webview()->webview(), SIGNAL(loadStarted()), this, SLOT(onLoadStarted()));
        myWindow->setModal(true);
        myWindow->showWindow();

        auto childLoginWebPage = new LoginWebPage(myWindow);
        childLoginWebPage->setNetworkAccessManager(this->networkAccessManager());
        myWindow->webview()->webview()->setPage(childLoginWebPage);

        OriginWebController* webController = new OriginWebController(myWindow->webview()->webview(), childLoginWebPage);
        webController->errorDetector()->disableErrorCheck(PageErrorDetector::FinishedWithFailure);
        webController->errorHandler()->setHandleOfflineAsError(true);
        webController->errorHandler()->setErrorPage(DefaultErrorPage::centerAlignedPage());
   

        ORIGIN_VERIFY_CONNECT(
            childLoginWebPage, SIGNAL(windowCloseRequested()),
            myWindow, SLOT(close()));

        return childLoginWebPage;
    }

    void LoginWebPage::onLoadStarted()
    {
        QWebView* webview = dynamic_cast<QWebView*>(sender());
        if(webview && webview->parentWidget())
        {
            UIToolkit::OriginWebView* webContainer = dynamic_cast<UIToolkit::OriginWebView*>(webview->parentWidget());
            if(webContainer)
            {
                webContainer->setHScrollPolicy(Qt::ScrollBarAlwaysOff);
                webContainer->setVScrollPolicy(Qt::ScrollBarAlwaysOff);
            }
        }

    }

    bool LoginWebPage::supportsExtension(Extension extension) const
    {
        return extension == Extension::ErrorPageExtension;
    }

    bool LoginWebPage::extension(Extension extension, const ExtensionOption * option, ExtensionReturn * output)
    {
        if (extension == Extension::ErrorPageExtension)
        {
            ErrorPageExtensionOption *error = (ErrorPageExtensionOption*)option;
            //String is hardcoded as error case in QNetworkReplyHandler.cpp
            if (error->errorString.compare("Redirection limit reached") == 0)
            {
                emit onRedirectionLoopDetected();
            }
        }
        return true;

    }

}
}
