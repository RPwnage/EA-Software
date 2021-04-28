/////////////////////////////////////////////////////////////////////////////
// OriginIGOProxy.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "OriginIGOProxy.h"
#include "OriginWebPage.h"
#include "engine/igo/IGOController.h"
#include "PopOutController.h"
#include <QVariant>

namespace Origin
{
namespace Client
{
namespace JsInterface
{
OriginIGOProxy::OriginIGOProxy()
    : mCreateWindowHandler(NULL)
{
}

OriginIGOProxy::~OriginIGOProxy()
{
}

void OriginIGOProxy::setCreateWindowHandler(QObject* handler)
{
    mCreateWindowHandler = handler;
}

void OriginIGOProxy::setCreateWindowRequest(const QString& url)
{
    if (mCreateWindowHandler)
        mCreateWindowHandler->setProperty(Origin::Client::OriginWebPage::CreateWindowUrlKey(), url);
}

void OriginIGOProxy::moveWindowToFront(const QString& url)
{
    Origin::Client::PopOutController::instance()->moveWindowToFront(url);
}

void OriginIGOProxy::openIGOConversation()
{
    // Do we need a check here for IGO?
    // When we open a conversation within the Main SPA while a game is running
    //Engine::IGOController::instance()->isConnected() == false
    //Engine::IGOController::instance()->isAvailable() == flase
    //if (Engine::IGOController::instance()->isGameInForeground() == true)
    Engine::IGOController::instance()->igoShowChatWindow(Origin::Engine::IIGOCommandController::CallOrigin_CLIENT);
}

void OriginIGOProxy::openIGOProfile(const QString& id)
{
    // Do we need a check here for IGO?
    // When we open a conversation within the Main SPA while a game is running
    //Engine::IGOController::instance()->isConnected() == false
    //Engine::IGOController::instance()->isAvailable() == flase
    //if (Engine::IGOController::instance()->isGameInForeground() == true)
    Engine::IGOController::instance()->igoShowSharedWindow(id, Origin::Engine::IIGOCommandController::CallOrigin_CLIENT);
}

}
}
}
