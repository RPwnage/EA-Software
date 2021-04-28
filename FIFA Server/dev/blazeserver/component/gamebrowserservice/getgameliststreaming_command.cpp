/*************************************************************************************************/
/*!
\file   getgameliststreaming_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class GetGameListStreamingCommand


\note
 The following is a command handler meant to create a server streaming grpcOnly GameBrowser Subscription.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include

#include "framework/blaze.h"
#include "framework/connection/selector.h"
#include "framework/grpc/inboundgrpcmanager.h"
#include "component/gamebrowserservice/getgameliststreaming_command.h"
#include "component/gamebrowserservice/gamebrowserserviceimpl.h"
#include "component/gamebrowserservice/gamebrowserlistownergbservice.h"

#include "component/gamemanager/gamemanagerslaveimpl.h"
#include "component/gamemanager/gamebrowser/gamebrowser.h"
#include "component/gamemanager/gamebrowser/gamebrowserlist.h"
#include "component/gamemanager/tdf/gamebrowser.h"

namespace Blaze
{
namespace GameBrowserService
{
namespace v1
{

GetGameListStreamingCommand::~GetGameListStreamingCommand()
{
    if (mCleanupTimer != INVALID_TIMER_ID)
        gSelector->cancelTimer(mCleanupTimer);
}

void GetGameListStreamingCommand::onProcessRequest(const GameBrowserListRequest& /*request*/) // work off of mRequest 
{
    TRACE_LOG("[GetGameListStreamingCommand].onProcessRequest: rpc (" << getName() << ") request id: " << mRequest.getRequestUUID());

    if (static_cast<GameBrowserServiceSlaveImpl*>(mComponent)->getConfig().getStreamLife() != 0)
        mCleanupTimer = gSelector->scheduleTimerCall(TimeValue::getTimeOfDay() + static_cast<GameBrowserServiceSlaveImpl*>(mComponent)->getConfig().getStreamLife(),
            this, &GetGameListStreamingCommand::done, "GetGameListStreamingCommand::done");

    GameManager::GameManagerSlaveImpl* gMgrSlave =
        static_cast<GameManager::GameManagerSlaveImpl*>(gController->getComponent(GameManager::GameManagerSlave::COMPONENT_ID, true /*reqLocal*/, true /*reqActive*/));

    if (gMgrSlave != nullptr)
    {
        Blaze::GameManager::GameBrowserListId listId = 0;
        if (gMgrSlave->getNextUniqueGameBrowserSessionId(listId) != ERR_OK)
        {
            ERR_LOG("[GetGameListStreamingCommand].onProcessRequest: failed to get unique id for request: " << mRequest.getRequestUUID());
            finishWithError(grpc::Status(grpc::RESOURCE_EXHAUSTED, FixedString128(FixedString128::CtorSprintf(), "failed to get unique id for request:%s", mRequest.getRequestUUID()).c_str()));
            return;
        }
                                 
        mListOwner = BLAZE_NEW_NAMED("GameBrowserListOwnerGBService") GameBrowserListOwnerGBService(listId, this);

        mGbrRequestInProgress = true;
        Blaze::GameManager::GetGameListResponse gbrResponse;
        Blaze::GameManager::MatchmakingCriteriaError gbrErrResponse;

        BlazeRpcError err = gMgrSlave->getGameBrowser().processCreateGameList(mRequest.getGameListRequest(),
            mRequest.getIsSnapshotList() ? GameManager::GAME_BROWSER_LIST_SNAPSHOT : GameManager::GAME_BROWSER_LIST_SUBSCRIPTION,
            gbrResponse, gbrErrResponse, 0, mListOwner.get()); 
        if (err == ERR_OK)
        {
            mListId = gbrResponse.getListId();
                
            GameBrowserListResponse response;
            response.getListUpdate().setListId(mListId);
            response.getListUpdate().setIsFinalUpdate(mListDestroyedByGbr); // Handle an edge case of the existing game browser behavior. The list may be destroyed immediately but still considered a success. 

            TRACE_LOG("[GetGameListStreamingCommand].onProcessRequest: sent initial response for request: " << mRequest.getRequestUUID() << ", listId:" << mListId);
            sendResponse(response);

            if (mListDestroyedByGbr)
                done();
        }
        else
        {
            ERR_LOG("[GetGameListStreamingCommand].onProcessRequest: sent error(" << ErrorHelp::getErrorName(err) << ") response for request: " << mRequest.getRequestUUID() << ". The error message: " << gbrErrResponse.getErrMessage());
            finishWithError(grpc::Status(grpc::INTERNAL, FixedString128(FixedString128::CtorSprintf(), "%s for request:%s", gbrErrResponse.getErrMessage(), mRequest.getRequestUUID()).c_str()));
        }

        mGbrRequestInProgress = false;
    }
    else
    {
        ERR_LOG("[GetGameListStreamingCommand].onProcessRequest: failed to find a locally active GameManagerSlave for request: " << mRequest.getRequestUUID());
        finishWithError(grpc::Status(grpc::FAILED_PRECONDITION, FixedString128(FixedString128::CtorSprintf(), "no locally active GameManagerSlave for request:%s", mRequest.getRequestUUID()).c_str()));
    }
        
}

void GetGameListStreamingCommand::onCleanup(bool) 
{
    if (mCleanedUp)
        return;

    TRACE_LOG("[GetGameListStreamingCommand].onCleanup: request: " << mRequest.getRequestUUID() << ", listId:" << mListId);

    mCleanedUp = true;
    
    if (mCleanupTimer != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mCleanupTimer);
        mCleanupTimer = INVALID_TIMER_ID;
    }
    
    if (mListOwner != nullptr)
        static_cast<GameBrowserListOwnerGBService*>(mListOwner.get())->clearOwner(); //We are no longer interested in any updates from the listOwner as command side is done.
 
    if (mListId != 0)
    {
        Blaze::GameManager::DestroyGameListRequest request;
        request.setListId(mListId);
        static_cast<GameBrowserServiceSlaveImpl*>(mComponent)->terminateList(request);

        mListId = 0;
    }
}

void GetGameListStreamingCommand::onListDestroy()
{
    TRACE_LOG("[GetGameListStreamingCommand].onListDestroy: rpc (" << getName() << ") request id: " << mRequest.getRequestUUID());
    
    mListDestroyedByGbr = true;

    // Clean this up as we no longer need an explicit terminateList call from this rpc layer. The underlying API has already terminated it.
    mListId = 0;

    // The list is getting prematurely deleted while the request is in progress. Ignore it and rely on OnProcessRequest to handle this.
    if (mGbrRequestInProgress)
        return;
    
    // This technically should not happen as we clean up the pointer on the GameBrowserListOwnerGBService when mCleanUp is set but just in case...
    if (mCleanedUp)
        return;

    // We are here because underlying layer has cleaned up a subscription list. Could be SearchSlave maintenance/reconfigure/whatever.
    if (!mSignaledDone)
    {
        ERR_LOG("[GetGameListStreamingCommand].onListDestroy: List has been destroyed on the server for request: " << mRequest.getRequestUUID());
        finishWithError(grpc::Status(grpc::INTERNAL, FixedString128(FixedString128::CtorSprintf(), "List has been destroyed on the server for request:%s", mRequest.getRequestUUID()).c_str()));
    }
}

void GetGameListStreamingCommand::onListUpdate(const Blaze::GameManager::NotifyGameListUpdate& listUpdate)
{
    TRACE_LOG("[GetGameListStreamingCommand].onListUpdate: rpc (" << getName() << ") request id: " << mRequest.getRequestUUID());
    
    if (!mGbrRequestInProgress)
    {
        GameBrowserListResponse response;
        listUpdate.copyInto(response.getListUpdate());

        sendResponse(response);
        
        if (listUpdate.getIsFinalUpdate())
            done();
    }
    else
    {
        // The list is getting prematurely deleted while the request is in progress. Ignore it and rely on OnProcessRequest to handle this.
        // Add a log here just to be safe (in case some future change breaks that invariant)
        if (!listUpdate.getIsFinalUpdate())
        {
            ASSERT_LOG("[GetGameListStreamingCommand].onListUpdate: Skip list notification because request is in progress. listUpdate: "
                << listUpdate);
        }
    }
}

void GetGameListStreamingCommand::done()
{
    TRACE_LOG("[GetGameListStreamingCommand].done: signaled done for request: " << mRequest.getRequestUUID() << ", listId:" << mListId);
    
    mSignaledDone = true;
    signalDone();
}

DEFINE_GETGAMELISTSTREAMING_CREATE();

} // namespace v1
} // namespace GameBrowserService
} // namespace Blaze


