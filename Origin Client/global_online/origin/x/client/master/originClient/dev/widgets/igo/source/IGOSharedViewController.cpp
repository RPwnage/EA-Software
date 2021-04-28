//
//  IGOSharedViewController.cpp
//  originClient
//
//   Created by Richard Spitler on 9/23/15.
//  Copyright (c) 2015 Electronic Arts. All rights reserved.
//

#include "IGOSharedViewController.h"

#include <QWebView>

#include "UIScope.h"
#include "ClientFlow.h"
#include "originwebview.h"
#include "OriginIGOProxy.h"
#include "OriginWebController.h"

#include "GamesManagerProxy.h"
#include "ContentOperationQueueControllerProxy.h"
#include "OnlineStatusProxy.h"
#include "OriginSocialProxy.h"
#include "ClientSettingsProxy.h"
#include "InstallDirectoryManager.h"
#include "DialogProxy.h"
#include "OriginUserProxy.h"
#include "OriginIGOProxy.h"
#include "VoiceProxy.h"
#include "DesktopServicesProxy.h"
#include "PopOutProxy.h"

#include "services/log/LogService.h"
#include "services/debug/DebugService.h"


namespace Origin
{
namespace Client
{
    
    IGOSharedViewController::IGOSharedViewController(const QString& id) 
        : mProfileId(id),
          mIntialized(false)
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
    
    QSize initSize(600, 800);
    if (initSize.width() > (screen.width() - defaultXMargin* 2))
        initSize.setWidth(screen.width() - defaultXMargin * 2);
    
    if (initSize.height() > (screen.height() - defaultYMargin * 2))
        initSize.setHeight(screen.height() - defaultYMargin * 2);
    
    mWindow->resize(initSize);
    mWindow->setMinimumSize(minScreen.width() - defaultXMargin * 2, minScreen.height() - defaultYMargin * 2);
#else
    mWindow->resize(600, 800);
    mWindow->setMinimumWidth(323);
#endif

    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SLOT(deleteLater()));

    // This is for web inspectors which makes debugging easier.
    mWebController = new OriginWebController(mWindow->webview()->webview(), NULL, false);


    if (Origin::Engine::IGOController::instance()->showWebInspectors())
        Origin::Engine::IGOController::instance()->showWebInspector(mWindow->webview()->webview()->page());
}

IGOSharedViewController::~IGOSharedViewController()
{
    mWindow->deleteLater();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

// IGO specific
bool IGOSharedViewController::ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior)
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

void IGOSharedViewController::ivcPostSetup()
{
}

QWebPage* IGOSharedViewController::createJSWindow(const QUrl& url)
{
    QUrlQuery query(url);
    if (query.hasQueryItem(DisplayTabUrlParamKey))
    {
        ORIGIN_LOG_DEBUG << "JUST FOR DEBUGGING: DISPLAYTAB=" << query.queryItemValue(DisplayTabUrlParamKey).toInt();
    }

    return mWindow->webview()->webview()->page();
}

void IGOSharedViewController::onSizeChanged(uint32_t width, uint32_t height)
{
    Origin::Engine::IGOController::instance()->igowm()->setWindowPosition(mWindow, defaultPosition(width, height));
}

QPoint IGOSharedViewController::defaultPosition(uint32_t width, uint32_t height)
{
    QPoint pos = Origin::Engine::IGOController::instance()->igowm()->centeredPosition(mWindow->size());
    if (pos.y() < 10)
        pos.setY(10);
    
    return pos;
}

void IGOSharedViewController::navigate(QString id)
{
    mProfileId = id;
    // Time to request the view from the SPA
    QUrlQuery query = Origin::Engine::IGOController::instance()->createJSRequestParams(this);
    // We are assuming this is a profile window this will probably change in the future
    query.addQueryItem("id", mProfileId);
    if (mIntialized)
        ClientFlow::instance()->view()->navigateJSWindow("profile", query.toString(QUrl::FullyEncoded));
    else{
        ClientFlow::instance()->view()->openJSWindow("profile", query.toString(QUrl::FullyEncoded));
        mIntialized = true;
    }
}

} // Client
} // Origin