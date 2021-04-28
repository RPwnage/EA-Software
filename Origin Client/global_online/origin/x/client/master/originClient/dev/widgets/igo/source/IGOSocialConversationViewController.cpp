//
//  IGOSocialConversationViewController.cpp
//  originClient
//
//   Created by Richard Spitler on 9/21/15.
//  Copyright (c) 2015 Electronic Arts. All rights reserved.
//

#include "IGOSocialConversationViewController.h"

#include <QWebView>

#include "UIScope.h"
#include "ClientFlow.h"
#include "originwebview.h"

#include "services/log/LogService.h"
#include "services/debug/DebugService.h"


namespace Origin
{
namespace Client
{
    
IGOSocialConversationViewController::IGOSocialConversationViewController()
{
    using namespace Origin::UIToolkit;
    
    mWindow = new OriginWindow(OriginWindow::Icon | OriginWindow::Close, NULL, OriginWindow::WebContent,
                                                    QDialogButtonBox::NoButton, OriginWindow::ChromeStyle_Window, OriginWindow::OIG);
    mWindow->webview()->setHasSpinner(true);
    mWindow->webview()->setIsContentSize(false);
    mWindow->setContentMargin(0);

#ifdef ORIGIN_MAC
    // Need to match the mininum game window size
    static const int defaultXMargin = 20;
    static const int defaultYMargin = 20;
    
    QSize screen = Origin::Engine::IGOController::instance()->igowm()->screenSize();
    QSize minScreen = Origin::Engine::IGOController::instance()->igowm()->minimumScreenSize();
    
    QSize initSize(323, 766);
    if (initSize.width() > (screen.width() - defaultXMargin* 2))
        initSize.setWidth(screen.width() - defaultXMargin * 2);
    
    if (initSize.height() > (screen.height() - defaultYMargin * 2))
        initSize.setHeight(screen.height() - defaultYMargin * 2);
    
    mWindow->resize(initSize);
    mWindow->setMinimumSize(minScreen.width() - defaultXMargin * 2, minScreen.height() - defaultYMargin * 2);
#else
    mWindow->resize(600, 600);
    mWindow->setMinimumWidth(200);
    mWindow->setMinimumHeight(200);
#endif

    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SLOT(deleteLater()));

    if (Origin::Engine::IGOController::instance()->showWebInspectors())
        Origin::Engine::IGOController::instance()->showWebInspector(mWindow->webview()->webview()->page());
}

IGOSocialConversationViewController::~IGOSocialConversationViewController()
{
    mWindow->deleteLater();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

// IGO specific
bool IGOSocialConversationViewController::ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior)
{
    QSize screen = Origin::Engine::IGOController::instance()->igowm()->screenSize();
    properties->setPosition(defaultPosition(screen.width(), screen.height()));
    properties->setRestorable(true);
    
    behavior->draggingSize = 40;
    behavior->nsBorderSize = 6;
    behavior->ewBorderSize = 4;
    behavior->pinnable = true;
    behavior->supportsContextMenu = true;
    behavior->screenListener = this;
    
    return true;
}

static const QString DisplayTabUrlParamKey = QStringLiteral("displaytab");

void IGOSocialConversationViewController::ivcPostSetup()
{
    // Time to request the view from the ASP
    QUrlQuery query = Origin::Engine::IGOController::instance()->createJSRequestParams(this);

    ClientFlow::instance()->view()->openJSWindow("socialwindow", query.toString(QUrl::FullyEncoded));
}

QWebPage* IGOSocialConversationViewController::createJSWindow(const QUrl& url)
{
    QUrlQuery query(url);
    if (query.hasQueryItem(DisplayTabUrlParamKey))
    {
        ORIGIN_LOG_DEBUG << "JUST FOR DEBUGGING: DISPLAYTAB=" << query.queryItemValue(DisplayTabUrlParamKey).toInt();
    }

    return mWindow->webview()->webview()->page();
}

void IGOSocialConversationViewController::onSizeChanged(uint32_t width, uint32_t height)
{
    Origin::Engine::IGOController::instance()->igowm()->setWindowPosition(mWindow, defaultPosition(width, height));
}

QPoint IGOSocialConversationViewController::defaultPosition(uint32_t width, uint32_t height)
{
    QPoint pos = Origin::Engine::IGOController::instance()->igowm()->centeredPosition(mWindow->size());
    if (pos.y() < 10)
        pos.setY(10);
    
    return pos;
}

} // Client
} // Origin