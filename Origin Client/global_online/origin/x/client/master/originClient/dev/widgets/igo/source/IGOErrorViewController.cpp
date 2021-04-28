//
//  IGOErrorViewController.cpp
//  originClient
//
//  Created by Frederic Meraud on 3/6/14.
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//

#include "IGOErrorViewController.h"

#include "OriginPushButton.h"

#include "services/debug/DebugService.h"


namespace Origin
{
namespace Client
{

IGOErrorViewController::IGOErrorViewController()
    : mWindow(NULL)
{
}

IGOErrorViewController::~IGOErrorViewController()
{
    if (mWindow)
        mWindow->deleteLater();
}

void IGOErrorViewController::setView(UIToolkit::OriginWindow* view)
{
    if (!view)
        return;
    
    // Remove any previous view
    if (mWindow)
        mWindow->deleteLater();
    
    mWindow = view;
    
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(destroyed()), this, SLOT(windowDestroyed()));
}
    
void IGOErrorViewController::setView(const QString& title, const QString& text, const QString& okBtnText, QObject* okBtnObj, const QString& okBtnSlot)
{
    // Remove any previous view
    if (mWindow)
        mWindow->deleteLater();
    
    mWindow = new UIToolkit::OriginWindow(UIToolkit::OriginWindow::Icon | UIToolkit::OriginWindow::Close, NULL, UIToolkit::OriginWindow::MsgBox
                                                                  , QDialogButtonBox::Close, UIToolkit::OriginWindow::ChromeStyle_Dialog, UIToolkit::OriginWindow::OIG);
    
    if (okBtnText.count())
    {
        mWindow->setButtonText(QDialogButtonBox::Close, tr("ebisu_client_cancel"));
        mWindow->addButton(QDialogButtonBox::Yes);
        mWindow->setButtonText(QDialogButtonBox::Yes, okBtnText);
        
        if (okBtnObj && !okBtnSlot.isEmpty())
            ORIGIN_VERIFY_CONNECT(mWindow->button(QDialogButtonBox::Yes), SIGNAL(clicked()), okBtnObj, okBtnSlot.toStdString().c_str());
    }
    
    mWindow->msgBox()->setup(UIToolkit::OriginMsgBox::Error, tr(title.toStdString().c_str()), tr(text.toStdString().c_str()));
    mWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    
    ORIGIN_VERIFY_CONNECT(mWindow->button(QDialogButtonBox::Close), SIGNAL(clicked()), mWindow, SLOT(close()));
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SLOT(deleteLater()));
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(destroyed()), this, SLOT(windowDestroyed()));
}

void IGOErrorViewController::windowDestroyed()
{
    if (sender() == mWindow)
    {
        mWindow = NULL;
        this->deleteLater();
    }
}

bool IGOErrorViewController::ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior)
{
    behavior->modal = true;
    
    return true;
}

void IGOErrorViewController::ivcPostSetup()
{
    Origin::Engine::IGOController::instance()->igowm()->fitWindowToContent(ivcView());
}
    
} // Client
} // Origin