//
//  IGOWebInspectorController.cpp
//  originClient
//
//  Created by Frederic Meraud on 6/4/14.
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//

#include "IGOWebInspectorController.h"

#include <QWebFrame>
#include <QWebInspector>

namespace Origin
{
namespace Client
{

IGOWebInspectorController::IGOWebInspectorController(QWebPage* page)
{
    page->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
    
    QWebInspector* wi = new QWebInspector();
    wi->setPage(page);
    wi->setVisible(true);
    
    mWindow = new UIToolkit::OriginWindow(UIToolkit::OriginWindow::Icon | UIToolkit::OriginWindow::Close, wi
                                         , UIToolkit::OriginWindow::Custom, QDialogButtonBox::NoButton
                                         , UIToolkit::OriginWindow::ChromeStyle_Dialog, UIToolkit::OriginWindow::OIG);
    
    QString wTitle = page->mainFrame()->url().toDisplayString();
    wTitle.insert(0, tr("ebisu_client_igo_web_inspector_window_title") + " - ");
    mWindow->setTitleBarText(wTitle);
    
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SLOT(deleteLater()));
}

IGOWebInspectorController::~IGOWebInspectorController()
{
    QWebInspector* wi = qobject_cast<QWebInspector*>(mWindow->takeContent());
    if (wi)
        delete wi;
    
    mWindow->deleteLater();
}
    
} // Client
} // Origin