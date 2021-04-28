/*************************************************************************************************/
/*!
    \file   gamebrowserserviceimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GameBrowserServiceImpl

    Implements the grpcOnly GameBrowserService.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "gamebrowserservice/gamebrowserserviceimpl.h"
#include "component/gamemanager/gamemanagerslaveimpl.h"

namespace Blaze
{
namespace GameBrowserService
{
namespace v1
{

/*** Public Methods ******************************************************************************/
// static
GameBrowserServiceSlave* GameBrowserServiceSlave::createImpl()
{
    return BLAZE_NEW_NAMED("GameBrowserServiceSlaveImpl") GameBrowserServiceSlaveImpl();
}

GameBrowserServiceSlaveImpl::GameBrowserServiceSlaveImpl()
    : GameBrowserServiceSlaveStub()
    , mTerminateListQueue("GameBrowserServiceSlaveImpl::mTerminateListQueue")
    , mShutdown(false)
{
    mTerminateListQueue.setMaximumWorkerFibers(FiberJobQueue::SMALL_WORKER_LIMIT);
}

GameBrowserServiceSlaveImpl::~GameBrowserServiceSlaveImpl()
{
}


bool GameBrowserServiceSlaveImpl::onConfigure()
{
    return true;
}

bool GameBrowserServiceSlaveImpl::onReconfigure()
{
    return onConfigure();
}

void GameBrowserServiceSlaveImpl::obfuscatePlatformInfo(Blaze::ClientPlatformType platform, EA::TDF::Tdf& tdf) const
{
    
}


void GameBrowserServiceSlaveImpl::terminateList(const Blaze::GameManager::DestroyGameListRequest& request)
{
    if (mShutdown)
        return;

    mTerminateListQueue.queueFiberJob(this, &GameBrowserServiceSlaveImpl::terminateListInternal, request, "GameBrowserServiceSlaveImpl::terminateList");
}

void GameBrowserServiceSlaveImpl::terminateListInternal(const Blaze::GameManager::DestroyGameListRequest request)
{
    GameManager::GameManagerSlaveImpl* gMgrSlave =
        static_cast<GameManager::GameManagerSlaveImpl*>(gController->getComponent(GameManager::GameManagerSlave::COMPONENT_ID, true /*reqLocal*/, true /*reqActive*/));
    if (gMgrSlave != nullptr)
    {
        BlazeRpcError err = gMgrSlave->getGameBrowser().processDestroyGameList(request);
        if (err != ERR_OK)
        {
            ERR_LOG("[GameBrowserServiceSlaveImpl].terminateListInternal: failed with error( " << ErrorHelp::getErrorName(err) << " )");
        }
    }
}

bool GameBrowserServiceSlaveImpl::onResolve()
{
    gController->registerDrainStatusCheckHandler(COMPONENT_INFO.name, *this);
    return true;
}

bool GameBrowserServiceSlaveImpl::isDrainComplete() 
{
    if (mTerminateListQueue.hasPendingWork())
    {
        INFO_LOG("[GameBrowserServiceSlaveImpl].isDrainComplete: terminate list queue still has pending work!");
        return false;
    }

    return true;
}

void GameBrowserServiceSlaveImpl::onShutdown()
{
    mShutdown = true;

    if (mTerminateListQueue.hasPendingWork())
    {
        WARN_LOG("[GameBrowserServiceSlaveImpl].onShutdown: cancel new work scheduled after drain completed!");
        mTerminateListQueue.cancelAllWork();
    }
    gController->deregisterDrainStatusCheckHandler(COMPONENT_INFO.name);
}

} // namespace v1
} // namespace GameBrowserService
} // namespace Blaze
