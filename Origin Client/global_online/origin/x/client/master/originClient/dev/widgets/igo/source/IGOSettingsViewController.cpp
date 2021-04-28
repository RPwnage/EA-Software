//
//  IGOSettingsViewController.cpp
//  originClient
//
//  Created by Frederic Meraud on 3/5/14.
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//

#include "IGOSettingsViewController.h"

#include <QWebView>

#include "UIScope.h"
#include "ClientFlow.h"
#include "ClientViewController.h"
#include "originwebview.h"

#include "services/log/LogService.h"
#include "services/debug/DebugService.h"


namespace Origin
{
namespace Client
{
    
IGOSettingsViewController::IGOSettingsViewController(ISettingsViewController::Tab displayTab)
    : mDisplayTab(displayTab)
{
    using namespace Origin::UIToolkit;
    
    mWindow = new OriginWindow(OriginWindow::Icon | OriginWindow::Close, NULL, OriginWindow::WebContent,
                                                    QDialogButtonBox::NoButton, OriginWindow::ChromeStyle_Window, OriginWindow::OIG);
    mWindow->setTitleBarText(tr("ebisu_client_settings"));
    mWindow->webview()->setHasSpinner(true);
    mWindow->webview()->setIsContentSize(false);
    mWindow->setContentMargin(0);

    if (ClientFlow::instance() && ClientFlow::instance()->view())
        mWindow->installEventFilter(ClientFlow::instance()->view());

#ifdef ORIGIN_MAC
    // Need to match the mininum game window size
    static const int defaultXMargin = 20;
    static const int defaultYMargin = 20;
    
    QSize screen = Origin::Engine::IGOController::instance()->igowm()->screenSize();
    QSize minScreen = Origin::Engine::IGOController::instance()->igowm()->minimumScreenSize();
    
    QSize initSize(1024, 640);
    if (initSize.width() > (screen.width() - defaultXMargin* 2))
        initSize.setWidth(screen.width() - defaultXMargin * 2);
    
    if (initSize.height() > (screen.height() - defaultYMargin * 2))
        initSize.setHeight(screen.height() - defaultYMargin * 2);
    
    mWindow->resize(initSize);
    mWindow->setMinimumSize(minScreen.width() - defaultXMargin * 2, minScreen.height() - defaultYMargin * 2);
#else
    mWindow->resize(1024,640);
    mWindow->setMinimumSize(995,640);
#endif

    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SLOT(deleteLater()));

    if (Origin::Engine::IGOController::instance()->showWebInspectors())
        Origin::Engine::IGOController::instance()->showWebInspector(mWindow->webview()->webview()->page());
}

IGOSettingsViewController::~IGOSettingsViewController()
{
    mWindow->deleteLater();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

// IGO specific
bool IGOSettingsViewController::ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior)
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

void IGOSettingsViewController::ivcPostSetup()
{
    // Time to request the view from the ASP
    QUrlQuery query = Origin::Engine::IGOController::instance()->createJSRequestParams(this);
    query.addQueryItem(DisplayTabUrlParamKey, QString::number(mDisplayTab));

    ClientFlow::instance()->view()->openJSWindow("settings", query.toString(QUrl::FullyEncoded));
}

QWebPage* IGOSettingsViewController::createJSWindow(const QUrl& url)
{
    QUrlQuery query(url);
    if (query.hasQueryItem(DisplayTabUrlParamKey))
    {
        ORIGIN_LOG_DEBUG << "JUST FOR DEBUGGING: DISPLAYTAB=" << query.queryItemValue(DisplayTabUrlParamKey).toInt();
    }

    return mWindow->webview()->webview()->page();
}

void IGOSettingsViewController::onSizeChanged(uint32_t width, uint32_t height)
{
    Origin::Engine::IGOController::instance()->igowm()->setWindowPosition(mWindow, defaultPosition(width, height));
}

QPoint IGOSettingsViewController::defaultPosition(uint32_t width, uint32_t height)
{
    QPoint pos = Origin::Engine::IGOController::instance()->igowm()->centeredPosition(mWindow->size());
    if (pos.y() < 10)
        pos.setY(10);
    
    return pos;
}

} // Client
} // Origin