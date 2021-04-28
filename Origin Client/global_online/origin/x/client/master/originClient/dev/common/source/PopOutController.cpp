/////////////////////////////////////////////////////////////////////////////
// PopOutController.cpp
//
// Copyright (c) 2015, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#include "PopOutController.h"
#include "services/debug/DebugService.h"
#include <QWebView>
#include <QWebInspector>
#include <QDesktopWidget>


#include "OriginWebPage.h"
#include "originwebview.h"
#include "NativeBehaviorManager.h"
#include "OriginIGOProxy.h"
#include "OriginWebController.h"
#include "originwindowmanager.h"

namespace Origin
{
namespace Client
{
static const QString SOCIAL_CHATWINDOW = "social-chatwindow.html";
static const QString SOCIAL_HUB = "social-hub.html";

PopOutController* PopOutControllerInstance = NULL;
PopOutController::PopOutController()
: QObject(NULL)
{
    mSizeMap[SOCIAL_CHATWINDOW] = QSize(600, 600);
    mSizeMap[SOCIAL_HUB] = QSize(323, 766);
    mMinSizeMap[SOCIAL_CHATWINDOW] = QSize(600, 300);
    mMinSizeMap[SOCIAL_HUB] = QSize(323, 450);
    
    mWindows.clear();
}


PopOutController::~PopOutController()
{
}


void PopOutController::create()
{
    ORIGIN_ASSERT(PopOutControllerInstance == NULL);
    PopOutControllerInstance = new PopOutController();
}


void PopOutController::destroy()
{
    delete PopOutControllerInstance;
}


PopOutController* PopOutController::instance()
{
    if (!PopOutControllerInstance)
        create();
    return PopOutControllerInstance;
}

QWebPage* PopOutController::createJSWindow(const QUrl& url)
{
    using namespace UIToolkit;
    QWebPage* childWebPage = NULL;
    OriginWindow::TitlebarItems titlebarItems = OriginWindow::Icon | OriginWindow::AllItems;
    OriginWindow::WindowContext windowContext = OriginWindow::Normal;
    OriginWindow* myWindow = new OriginWindow(titlebarItems, NULL, OriginWindow::WebContent,
        QDialogButtonBox::NoButton, OriginWindow::ChromeStyle_Dialog, windowContext);

    myWindow->webview()->setWindowLoadingSize(mSizeMap[url.fileName()]);
    myWindow->webview()->setHasSpinner(true);
    myWindow->webview()->setIsSelectable(true);
    myWindow->webview()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    myWindow->webview()->setChangeLoadSizeAfterInitLoad(true);
    myWindow->webview()->setUsesOriginScrollbars(false);
    myWindow->webview()->setIsContentSize(false);
    myWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    myWindow->setMinimumSize(mMinSizeMap[url.fileName()]);
    myWindow->resize(mSizeMap[url.fileName()]);
    myWindow->setContentMargin(0);

    myWindow->showWindow();

    childWebPage = new OriginWebPage(myWindow);
    myWindow->webview()->webview()->setPage(childWebPage);

    if (myWindow->manager())
    {
        ORIGIN_VERIFY_CONNECT(myWindow->manager(), SIGNAL(customizeZoomedSize()), this, SLOT(onZoomed()));
    }

    ORIGIN_VERIFY_CONNECT(childWebPage, SIGNAL(destroyed(QObject*)), myWindow, SLOT(close()));
    ORIGIN_VERIFY_CONNECT(myWindow, SIGNAL(rejected()), this, SLOT(onClosed()));


    NativeBehaviorManager::applyNativeBehavior(myWindow->webview()->webview());
    myWindow->webview()->webview()->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    myWindow->webview()->webview()->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
    myWindow->webview()->webview()->settings()->setAttribute(QWebSettings::DnsPrefetchEnabled, true);
    myWindow->webview()->webview()->settings()->setAttribute(QWebSettings::JavascriptCanAccessClipboard, true);
    myWindow->webview()->webview()->settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);
    myWindow->webview()->webview()->settings()->setAttribute(QWebSettings::TiledBackingStoreEnabled, false);

    // save window url so we can tell the JS about it later
    myWindow->setProperty(popOutWindowUrlKey(), url.fileName());
    mWindows[url.fileName()] = myWindow;
    return childWebPage;
}

void PopOutController::onClosed()
{
    UIToolkit::OriginWindow* window = dynamic_cast<Origin::UIToolkit::OriginWindow*>(sender());
    if (window)
    {
        QString url = window->property(popOutWindowUrlKey()).toString();
        emit(closed(url));
        window->deleteLater();
    }
}

void PopOutController::onPopInWindow(QString url)
{
    UIToolkit::OriginWindow* window = mWindows[url];
    if (window)
        window->close();
    mWindows.remove(url);
}

void PopOutController::moveWindowToFront(const QString& url) {
    UIToolkit::OriginWindow* window = mWindows[url];
    if (window) {
        window->showWindow();
    }
}

void PopOutController::onZoomed()
{
    UIToolkit::OriginWindowManager* manager = dynamic_cast<Origin::UIToolkit::OriginWindowManager*>(sender());
    if (manager)
    {
        QWidget* window = manager->managedWidget();
        if (window)
        {
            QString url = window->property(popOutWindowUrlKey()).toString();
            const QRect availableGeo = QDesktopWidget().availableGeometry(window);

            if (url == SOCIAL_HUB)
            {
                window->setGeometry(window->x(), availableGeo.y(), mSizeMap[url].width(), availableGeo.height());
            }
            else if (url == SOCIAL_CHATWINDOW)
            {
                window->setGeometry(availableGeo);
            }
            manager->ensureOnScreen();
        }
    }
}

UIToolkit::OriginWindow* PopOutController::chatWindow()
{
    return mWindows[SOCIAL_CHATWINDOW];
}

}
}
