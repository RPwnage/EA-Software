//
//  IGOCodeRedemptionViewController.cpp
//  originClient
//
//  Created by Frederic Meraud on 3/26/15.
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//

#include "IGOCodeRedemptionViewController.h"

#include "originwebview.h"
#include "WebWidget/WidgetView.h"
#include "TelemetryAPIDLL.h"

// Legacy APP
#include "RedeemBrowser.h"

#include "services/debug/DebugService.h"


namespace Origin
{
namespace Client
{

IGOCodeRedemptionViewController::IGOCodeRedemptionViewController()
{
    using namespace Origin::UIToolkit;
    


    mRedeemBrowser = new RedeemBrowser(NULL, RedeemBrowser::Origin, RedeemBrowser::OriginCodeClient, NULL, "");
    mRedeemBrowser->setObjectName("RedeemCodeWindowOIG");

    mWindow = new OriginWindow(OriginWindow::Icon | OriginWindow::Close, mRedeemBrowser, OriginWindow::WebContent,
                                                  QDialogButtonBox::NoButton, OriginWindow::ChromeStyle_Dialog, OriginWindow::OIG);
   
    mWindow->setTitleBarText(tr("ebisu_client_redeem_game_code"));

    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SLOT(deleteLater()));
    ORIGIN_VERIFY_CONNECT(mRedeemBrowser, SIGNAL(closeWindow()), this, SLOT(deleteLater()));
   
    if (Origin::Engine::IGOController::instance()->showWebInspectors())
        Origin::Engine::IGOController::instance()->showWebInspector(mWindow->webview()->webview()->page());
}

IGOCodeRedemptionViewController::~IGOCodeRedemptionViewController()
{
    GetTelemetryInterface()->Metric_ACTIVATION_WINDOW_CLOSE("window");

    mWindow->deleteLater();
}

bool IGOCodeRedemptionViewController::ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior)
{
    QPoint pos = Origin::Engine::IGOController::instance()->igowm()->centeredPosition(ivcView()->size());
    properties->setPosition(pos);
    properties->setRestorable(true);
    
    behavior->pinnable = false;
    behavior->draggingSize = 40;
    behavior->disableResizing = true;
    
    return true;
}

} // Client
} // Origin