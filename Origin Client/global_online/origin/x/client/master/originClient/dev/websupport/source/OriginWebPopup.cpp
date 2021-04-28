///////////////////////////////////////////////////////////////////////////////
// OriginWebPopup.cpp
//
// Copyright (c) 2014 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "OriginWebPopup.h"
#include "services/network/NetworkAccessManager.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"

#include "originwindow.h"
#include "originwebview.h"

#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrintDialog>
#include <QWebView>
#include <QWebPage>
#include <QWebFrame>

namespace Origin
{
namespace Client
{
    
OriginWebPopup::OriginWebPopup(QObject *parent)
: QObject(parent)
{
    using namespace UIToolkit;
    mWindow = new OriginWindow(OriginWindow::AllItems, NULL, OriginWindow::WebContent, QDialogButtonBox::NoButton, OriginWindow::ChromeStyle_Window);
    mWindow->resize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
    mWindow->webview()->setHasSpinner(true);
    mWindow->webview()->setIsContentSize(false);
    mWindow->setWindowFlags(mWindow->windowFlags() | Qt::Popup);

    // Close the window when the user requests it
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), mWindow, SLOT(close()));
    // Or when we're deleted
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(destroyed(QObject*)), mWindow, SLOT(close()));

    QWebPage *page = mWindow->webview()->webview()->page();
    page->setNetworkAccessManager(Services::NetworkAccessManager::threadDefaultInstance());

    ORIGIN_VERIFY_CONNECT(page, SIGNAL(printRequested(QWebFrame*)), this, SLOT(onPrintRequested()));

    if (Services::readSetting(Services::SETTING_WebDebugEnabled))
    {
        page->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
    }
}

void OriginWebPopup::show()
{
    if(mWindow)
    {
        mWindow->show();
    }
}

void OriginWebPopup::hide()
{
    if(mWindow)
    {
        mWindow->hide();
    }
}

void OriginWebPopup::onPrintRequested()
{
    QPrinter printer;
    printer.setOutputFormat(QPrinter::NativeFormat);
    QPrintDialog printDialog(&printer, mWindow);
    if (printDialog.exec() == QDialog::Accepted)
    {
        mWindow->webview()->webview()->print(printDialog.printer());
    }
}

} // namespace Client

} // namespace Origin
