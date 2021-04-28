///////////////////////////////////////////////////////////////////////////////
// IGOWEBBROWSERJSHELPER.cpp
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGOWebBrowserJsHelper.h"
#include "engine/igo/IGOController.h"

namespace Origin
{
namespace Client
{

IGOWebBrowserJsHelper::IGOWebBrowserJsHelper( QObject* parent ):
    QObject(parent)
{}

void IGOWebBrowserJsHelper::closeIGO()
{
    Origin::Engine::IGOController::instance()->EbisuShowIGO(false);
}

void IGOWebBrowserJsHelper::close()
{
    emit closeIGOBrowserTab();
}

void IGOWebBrowserJsHelper::close(QString token)
{
    emit closeIGOBrowserByTwitch(token);
}

void IGOWebBrowserJsHelper::openLinkInNewOIGBrowser(QString url)
{
    Origin::Engine::IGOController::instance()->createWebBrowser(url);
}

IGOWebBrowserJsHelper::~IGOWebBrowserJsHelper(void)
{
}

} // Client
} // Origin

