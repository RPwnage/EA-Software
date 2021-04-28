//////////////////////////////////////////////////////////////////////////////
// StoreWebPage.cpp
//
// Copyright (c) 2014 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "StoreWebPage.h"
#include "OriginWebPopup.h"
#include "services/debug/DebugService.h"
#include "services/config/OriginConfigService.h"
#include "originwindow.h"
#include "originwebview.h"
#include <QWebView>
#include <QWebPage>
#include <QUrl>
#include <QEvent>
#include <QWebFrame>

namespace Origin
{
namespace Client
{

StoreWebPage::StoreWebPage(QObject* parent)
: OriginWebPage(parent)
, mUseWhiteList(false)
{
    mUrlBlackList << "youtube";
	// LoginViewController::init already enables persistent storage, so don't do it here again, it will crash QtWebkit!!!
	/*
	settings()->enablePersistentStorage();
	settings()->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, true);
	settings()->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, true);
	settings()->setAttribute(QWebSettings::LocalStorageEnabled, true);
	*/
}

QWebPage* StoreWebPage::createWindow(QWebPage::WebWindowType)
{
    // Only allow pop-ups if the current page is on our client access whitelist.
    // Note that this is distinct from StoreWebPage::mUrlBlackList, which contains URLs that are not allowed to be loaded in the popup.
    if (mUseWhiteList && !urlInWhiteList(mainFrame()->url().toString()))
    {
        return NULL;
    }

    using namespace UIToolkit;
    OriginWebPopup* popup = new OriginWebPopup(this);

    // We aren't sure if we want to show a window. Despite capturing the show event a window icon still shows on the taskbar. We need to stop
    // it from showing until we know for sure we want to actually show the window.
    // - We have to do this within createWindow because QWebPage::acceptNavigationRequest doesn't works to well with iframes. For youtube I couldn't
    // stop the it from creating a new window. EBIBUGS-26356. We couldn't turn off QWebSettings::JavascriptCanOpenWindows because the store needs to
    // open up some windows.
    popup->installEventFilter(this);
    popup->hide();

    ORIGIN_VERIFY_CONNECT(popup->window()->webview()->webview(), SIGNAL(urlChanged(const QUrl&)), this, SLOT(onUrlChanged(const QUrl&)));

    return popup->window()->webview()->webview()->page();
}


bool StoreWebPage::eventFilter(QObject* obj, QEvent* e)
{
    QEvent::Type ty = e->type();
    switch(ty)
    {
    case QEvent::Show:
        {
            UIToolkit::OriginWindow* window = dynamic_cast<UIToolkit::OriginWindow*>(obj);
            if(window && window->webview() && window->webview()->webview())
            {
                const QString urlStr = window->webview()->webview()->url().toString();
                if(urlInBlackList(urlStr) || urlStr == "")
                {
                    window->hide();
                    e->accept();
                    return true;
                }
            }
        }
        break;
    default:
        break;
    }
    return QWebPage::eventFilter(obj, e);
}

void StoreWebPage::onUrlChanged(const QUrl& url)
{
    QWebView* webview = dynamic_cast<QWebView*>(sender());
    if(webview && webview->topLevelWidget())
    {
        const QString urlStr = url.toString();
        QWidget* window = webview->topLevelWidget();
        if(urlInBlackList(urlStr))
        {
            window->close();
        }
        else if(urlStr != "")
        {
            window->setWindowFlags((window->windowFlags() & ~Qt::Popup) | Qt::Window);
            window->show();
            window->raise();
        }
    }
}

bool StoreWebPage::urlInWhiteList(const QString& url)
{
    const std::vector<QString> &whitelistedUrls = Origin::Services::OriginConfigService::instance()->clientAccessWhiteListUrls().url;
    for (std::vector<QString>::size_type i = 0; i < whitelistedUrls.size(); i++)
    {
        QUrl whiteListUrl = whitelistedUrls[i];
        QUrl urlToCheck = url;

        if (whiteListUrl.host().toLower() == urlToCheck.host().toLower() &&
            whiteListUrl.scheme().toLower() == urlToCheck.scheme().toLower())
        {
            return true;
        }
    }
    return false;
}

bool StoreWebPage::urlInBlackList(const QString& url)
{
    foreach(QString str, mUrlBlackList)
    {
        if(url.contains(str, Qt::CaseInsensitive))
            return true;
    }
    return false;
}

}
}