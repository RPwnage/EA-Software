/////////////////////////////////////////////////////////////////////////////
// CodeRedemptionDebugViewController.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "CodeRedemptionDebugViewController.h"
#include "CodeRedemptionDebugView.h"
#include "services/debug/DebugService.h"
#include "originwindow.h"
#include "originwindowmanager.h"

namespace Origin
{
namespace Client
{
CodeRedemptionDebugViewController* CodeRedemptionDebugViewController::sInstance = NULL;

CodeRedemptionDebugViewController* CodeRedemptionDebugViewController::instance()
{
    if(!sInstance)
        init();

    return sInstance;
}
CodeRedemptionDebugViewController::CodeRedemptionDebugViewController() : mWindow(NULL)
{
}

CodeRedemptionDebugViewController::~CodeRedemptionDebugViewController()
{
}

void CodeRedemptionDebugViewController::show()
{
    using namespace UIToolkit;
    if(!mWindow)
    {
        CodeRedemptionDebugView* debugView = new CodeRedemptionDebugView();
        mWindow = new OriginWindow((OriginWindow::Close | OriginWindow::Minimize),
            NULL, OriginWindow::Custom);
        ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SLOT(onCloseWindow()));
        mWindow->setContent(debugView);
    }
    else
        mWindow->activateWindow();

    mWindow->show();
    mWindow->adjustSize();
    if(mWindow->manager())
        mWindow->manager()->ensureOnScreen();
}

void CodeRedemptionDebugViewController::onCloseWindow()
{
    if(mWindow)
    {
        mWindow->close();
        mWindow=NULL;
    }
}

}
}