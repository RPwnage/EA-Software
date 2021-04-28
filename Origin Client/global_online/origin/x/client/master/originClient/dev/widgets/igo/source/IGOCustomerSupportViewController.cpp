//
//  IGOCustomerSupportViewController.cpp
//  originClient
//
//  Created by Frederic Meraud on 3/5/14.
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//

#include "IGOCustomerSupportViewController.h"

#include "CustomerSupportDialog.h"
#include "CustomerSupportJsHelper.h"

#include "services/debug/DebugService.h"


namespace Origin
{
namespace Client
{

IGOCustomerSupportViewController::IGOCustomerSupportViewController(const QString& url)
{
    using namespace Origin::UIToolkit;
    
    CustomerSupportDialog* view = new CustomerSupportDialog(url, 0, CustomerSupportDialog::IGO);
    mWindow = new OriginWindow(OriginWindow::Icon | OriginWindow::Close,
                               view, OriginWindow::Custom, QDialogButtonBox::NoButton, OriginWindow::ChromeStyle_Dialog, OriginWindow::OIG);
    mWindow->setTitleBarText(tr("ebisu_mainmenuitem_help"));
    mWindow->setContentMargin(0);
    mWindow->setMinimumSize(480, 480);

    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SLOT(deleteLater()));
}

IGOCustomerSupportViewController::~IGOCustomerSupportViewController()
{
    mWindow->deleteLater();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

// IGO specific
bool IGOCustomerSupportViewController::ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior)
{
    properties->setRestorable(true);
    
    behavior->pinnable = true;
#ifdef ORIGIN_MAC
    behavior->draggingSize = 40;
    behavior->nsBorderSize = 10;
    behavior->ewBorderSize = 10;
    behavior->nsMarginSize = 6;
    behavior->ewMarginSize = 6;
#else
    behavior->draggingSize = 40;
    behavior->nsBorderSize = 10;
    behavior->ewBorderSize = 10;
#endif
    
    return true;
}

} // Client
} // Origin