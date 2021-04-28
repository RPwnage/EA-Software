//  CustomizeBoxartFlow.cpp
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#include "flows/CustomizeBoxartFlow.h"

#include "engine/content/ContentController.h"
#include "engine/content/CustomBoxartController.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

#include "EbisuSystemTray.h"

#include "TelemetryAPIDLL.h"

namespace Origin
{
namespace Client
{

    CustomizeBoxartFlow::CustomizeBoxartFlow() :
        AbstractFlow(),
        mViewController(NULL),
        mGame(NULL),
        mCustomBoxartController(NULL)
    {

    }

    CustomizeBoxartFlow::~CustomizeBoxartFlow()
    {

    }

    void CustomizeBoxartFlow::initialize(Engine::Content::EntitlementRef game /*= Engine::Content::EntitlementRef(NULL)*/)
    {
        mGame = game;

        mViewController = new CustomizeBoxartViewController(this);
        mViewController->initialize();

        ORIGIN_VERIFY_CONNECT(mViewController, SIGNAL(boxartChanged(const Services::Entitlements::Parser::BoxartData&)), this,
            SLOT(onBoxartChanged(const Services::Entitlements::Parser::BoxartData&)));
        ORIGIN_VERIFY_CONNECT(mViewController, SIGNAL(removeBoxart()), this, SLOT(onRemoveBoxart()));

        mCustomBoxartController = Engine::Content::ContentController::currentUserContentController()->boxartController();
        if (mCustomBoxartController)
        {
            ORIGIN_VERIFY_CONNECT_EX(mCustomBoxartController, SIGNAL(saveComplete()), this, SLOT(onCacheUpdated()), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mCustomBoxartController, SIGNAL(removeComplete()), this, SLOT(onCacheUpdated()), Qt::QueuedConnection);
        }

    }

    void CustomizeBoxartFlow::start()
    {
        if (!mViewController)
        {
            ORIGIN_LOG_ERROR << "Flow has not been initialized.";
            CustomizeBoxartFlowResult result;
            result.result = FLOW_FAILED;
            emit finished(result);
            return;
        }

        mViewController->onShowCustomizeBoxartDialog(mGame);
    }

    void CustomizeBoxartFlow::focus()
    {
        if(mViewController)
        {
            mViewController->focus();
        }
    }

    void  CustomizeBoxartFlow::onBoxartChanged(const  Services::Entitlements::Parser::BoxartData& boxartData)
    {
        if (mCustomBoxartController)
        {
            Services::Entitlements::Parser::BoxartData newBoxartData(boxartData);
            newBoxartData.setProductId(mGame->contentConfiguration()->productId());
            mCustomBoxartController->saveCustomBoxart(newBoxartData);
        }
        else
        {
            stop();
        }
    }

    void CustomizeBoxartFlow::onRemoveBoxart()
    {
        mCustomBoxartController->removeCustomBoxart(mGame->contentConfiguration()->productId());
    }

    void CustomizeBoxartFlow::onCacheUpdated()
    {
        stop();
    }

    void CustomizeBoxartFlow::stop()
    {
        CustomizeBoxartFlowResult result;
        result.result = FLOW_SUCCEEDED;
        emit finished(result);
    }

}
}
