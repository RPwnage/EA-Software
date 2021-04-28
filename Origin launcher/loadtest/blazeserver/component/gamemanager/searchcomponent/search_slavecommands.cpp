/*! ************************************************************************************************/
/*!
    \file search_slavecommands.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/connectionmanager.h"
#include "gamemanager/searchcomponent/searchslaveimpl.h"
#include "searchcomponent/sortedgamelist.h"
#include "searchcomponent/unsortedgamelist.h"
#include "gamemanager/rpc/searchslave/startfindgamematchmaking_stub.h"
#include "gamemanager/rpc/searchslave/creategamelist_stub.h"
#include "gamemanager/gamemanagerhelperutils.h"

namespace Blaze
{
namespace Search
{

    class StartFindGameMatchmakingCommand : public StartFindGameMatchmakingCommandStub
    {
    public:

        StartFindGameMatchmakingCommand(Message* message, StartFindGameMatchmakingRequest *request, SearchSlaveImpl* componentImpl)
            : StartFindGameMatchmakingCommandStub(message, request), mComponent(*componentImpl)
        {
        }

        ~StartFindGameMatchmakingCommand() override {}

    private:

        StartFindGameMatchmakingCommandStub::Errors execute() override
        {
            auto originatingInstanceId = GetInstanceIdFromInstanceKey64(getSlaveSessionId());
            return commandErrorFromBlazeError(mComponent.processStartFindGameMatchmaking(mRequest, mResponse, mErrorResponse, originatingInstanceId));
        }

    private:
        SearchSlaveImpl &mComponent;
    };

    //static creation factory method of command's stub class
    DEFINE_STARTFINDGAMEMATCHMAKING_CREATE()

    BlazeRpcError SearchSlaveImpl::processStartFindGameMatchmaking(const Blaze::Search::StartFindGameMatchmakingRequest &request,
        Blaze::Search::StartFindGameMatchmakingResponse& response, Blaze::GameManager::MatchmakingCriteriaError &criteriaError, InstanceId originatingInstanceId)
    {
        if (gFiberManager != nullptr && gFiberManager->getCpuUsageForProcessPercent() > getConfig().getOverloadCpuPercent())
        {
            return SEARCH_ERR_OVERLOADED;
        }

        const GameManager::UserSessionInfo& ownerUserSessionInfo = request.getStartMatchmakingInternalRequest().getOwnerUserSessionInfo(); 
        LogContextOverride logAudit(ownerUserSessionInfo.getSessionId());

        BlazeRpcError error = Blaze::ERR_OK;

        mMatchmakingSlave.createMatchmakingSession(
            request.getMatchmakingSessionId(),
            request.getStartMatchmakingInternalRequest(),
            error,
            criteriaError,
            response.getDiagnostics(), originatingInstanceId);

        if (error == Blaze::ERR_OK)
            mMetrics.mTotalSearchSessionsStarted.increment();

        return error;
    }

    
    TerminateFindGameMatchmakingError::Error SearchSlaveImpl::processTerminateFindGameMatchmaking(const Blaze::Search::TerminateFindGameMatchmakingRequest &request, const Message* message)
    {
        mMatchmakingSlave.cancelMatchmakingSession(request.getMatchmakingSessionId(), request.getMatchmakingResult());
        return TerminateFindGameMatchmakingError::ERR_OK;
    }


    CreateGameListError::Error SearchSlaveImpl::processCreateGameList(const Blaze::Search::CreateGameListRequest &request, Blaze::Search::CreateGameListResponse &response, Blaze::GameManager::MatchmakingCriteriaError &error, const ::Blaze::Message* message)
    {
        uint16_t maxPlayerCapacity = request.getMaxPlayerCapacity();
        const char8_t* preferredPingSite = request.getPreferredPingSite();
        GameNetworkTopology netTopology = request.getNetTopology();
        if (gFiberManager != nullptr && gFiberManager->getCpuUsageForProcessPercent() > getConfig().getOverloadCpuPercent())
        {
            return CreateGameListError::SEARCH_ERR_OVERLOADED;
        }

        if (request.getIsUserless())
        {
            if (request.getOwnerUserInfo().getSessionId() != UserSession::INVALID_SESSION_ID)
            {
                WARN_LOG("[SearchSlaveImpl].processCreateGameList: User " << UserSession::getCurrentUserBlazeId()
                    << " attempted to create internal list(" << request.getGetGameListRequest().getListConfigName() <<" ) owned by UserSession(" << request.getOwnerUserInfo().getSessionId() << "). Internal lists should not have a user owner.");
            }
        }
        else if (!gUserSessionManager->getSessionExists(request.getOwnerUserInfo().getSessionId()))
        {
            // list must be owned by usersession so if user session is gone, error out
            return CreateGameListError::ERR_SYSTEM;
        }

        // We no longer track a limit.  Titles are free to create GB lists until they crash.

        if (request.getGetGameListRequest().getListCapacity() == 0)
        {
            return CreateGameListError::SEARCH_ERR_INVALID_CAPACITY;
        }

        const GameManager::GameBrowserListConfig *listConfig = getListConfig(request.getGetGameListRequest().getListConfigName());
        if (listConfig == nullptr)
        {
            return CreateGameListError::SEARCH_ERR_INVALID_LIST_CONFIG_NAME;
        }

        if (!request.getGetGameListRequest().getListCriteria().getDedicatedServerAttributeRuleCriteriaMap().empty())
        {
            // Only DS search or create GB with permission allows to continue
            if (request.getNetTopology() != CLIENT_SERVER_DEDICATED && !UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_DEDICATED_SERVER_ATTRIBUTE_SEARCH))
            {
                ERR_LOG("[SearchSlaveImpl].processCreateGameList: User " << UserSession::getCurrentUserBlazeId()
                    << " attempted to search for dedicated server by attributes, no permission!");

                return CreateGameListError::ERR_AUTHORIZATION_REQUIRED;
            }
        }

        GameManager::ListType listType = request.getIsSnapshot() ? 
            GameManager::GAME_BROWSER_LIST_SNAPSHOT : GameManager::GAME_BROWSER_LIST_SUBSCRIPTION;

        GameList *newList = nullptr;
        
        // TODO_MC: Figure out what's going on here.  Can we just get rid of this block of code.
        SlaveSessionId instanceSessionId = SlaveSession::INVALID_SESSION_ID;
        InstanceId instanceId = GetInstanceIdFromInstanceKey64(request.getGameBrowserListId());
        if (gController->getInstanceId() != instanceId)
        {
            SlaveSessionPtr sess = gController->getConnectionManager().getSlaveSessionByInstanceId(instanceId);
            if (sess != nullptr)
            {
                instanceSessionId = sess->getId();
            }
        }

        bool filterOnly = !request.getFilterMap().empty(); // if filter criteria specified only use this

        if (listConfig->getSortType() == GameManager::SORTED_LIST)
        {
            newList = (GameList*) (BLAZE_NEW SortedGameList(*this, request.getGameBrowserListId(), instanceSessionId, listType, request.getOwnerUserInfo().getSessionId(), filterOnly));
        }
        else
        {
            if (listType == GameManager::GAME_BROWSER_LIST_SUBSCRIPTION)
            {
                // we don't allow unsorted subscription lists
                return CreateGameListError::SEARCH_ERR_INVALID_CRITERIA;
            }
            newList = (GameList*) (BLAZE_NEW UnsortedGameList(*this, request.getGameBrowserListId(), instanceSessionId, listType, request.getOwnerUserInfo().getSessionId(), filterOnly));
        }

        BlazeRpcError errCode = newList->initialize(request.getOwnerUserInfo(), request.getGetGameListRequest(), request.getEvaluateGameVersionString(), error, maxPlayerCapacity, preferredPingSite, netTopology, request.getFilterMap());
        if (errCode != ERR_OK)
        {
            delete newList;
            return CreateGameListError::commandErrorFromBlazeError(errCode);
        }

        TimeValue initReteProdStartTime = TimeValue::getTimeOfDay();

        // prep the player list WMEs before we init RETE productions
        mMatchmakingSlave.getXblAccountIdGamesMapping().processWatcherInitialMatches();
        mMatchmakingSlave.getPlayerGamesMapping().processWatcherInitialMatches();

        // initialize RETE productions
        newList->initializeReteProductions(mReteNetwork);

        TimeValue initReteProdEndTime = TimeValue::getTimeOfDay();
        TimeValue initReteProdElapseTime = initReteProdEndTime - initReteProdStartTime;

        // update metrics
        mIdleMetrics.mNewListsAtIdleStart++;
        mIdleMetrics.mGamesMatchedAtCreateList += newList->getUpdatedGamesSize();
        mIdleMetrics.mGamesMatchedAtCreateList_SumTimes += initReteProdElapseTime;
        mMetrics.mTotalGamesMatchedAtCreateList += newList->getUpdatedGamesSize();
        mMetrics.mTotalGamesMatchedAtCreateList_SumTimes += initReteProdElapseTime;
        if (newList->getUpdatedGamesSize() >= request.getGetGameListRequest().getListCapacity())
        {
            // set max/min overall
            mMetrics.mFilledAtCreateList_SumTimes += initReteProdElapseTime;
            if (mMetrics.mFilledAtCreateList_MaxTime < initReteProdElapseTime)
                mMetrics.mFilledAtCreateList_MaxTime = initReteProdElapseTime;
            if (mMetrics.mFilledAtCreateList_MinTime > initReteProdElapseTime || mMetrics.mFilledAtCreateList_MinTime == 0)
                mMetrics.mFilledAtCreateList_MinTime = initReteProdElapseTime;

            // set max/min since last idle
            mIdleMetrics.mFilledAtCreateList_SumTimes += initReteProdElapseTime;
            if (mIdleMetrics.mFilledAtCreateList_MaxTime < initReteProdElapseTime)
                mIdleMetrics.mFilledAtCreateList_MaxTime = initReteProdElapseTime;
            if (mIdleMetrics.mFilledAtCreateList_MinTime > initReteProdElapseTime || mIdleMetrics.mFilledAtCreateList_MinTime == 0)
                mIdleMetrics.mFilledAtCreateList_MinTime = initReteProdElapseTime;
        }
        
        addList(*newList);

        // send back what we've found in the first scoop
        response.setMaxPossibleFitScore(newList->calcMaxPossibleFitScore());
        response.setNumberOfGamesToBeDownloaded(newList->getUpdatedGamesSize());

        SPAM_LOG("[SearchSlaveImpl].processCreateGameList: Sending response for list id: " << request.getGameBrowserListId()
            << "; maxPossibleFitScore=" << response.getMaxPossibleFitScore() << "  numberOfGamesToBeDownloaded=" 
            << response.getNumberOfGamesToBeDownloaded());

        // schedule idle to send down the actual list
        scheduleNextIdle(TimeValue::getTimeOfDay(), getDefaultIdlePeriod());

        return CreateGameListError::ERR_OK;
    }

    TerminateGameListError::Error SearchSlaveImpl::processTerminateGameList(const Blaze::Search::TerminateGameListRequest &request, const Blaze::Message *)
    {
        destroyGameList(request.getListId());

        return TerminateGameListError::ERR_OK;
    }

    MatchmakingDedicatedServerOverrideError::Error SearchSlaveImpl::processMatchmakingDedicatedServerOverride(
        const Blaze::GameManager::MatchmakingDedicatedServerOverrideRequest &request, const Blaze::Message *)
    {
        mMatchmakingSlave.dedicatedServerOverride(request.getPlayerId(), request.getGameId());

        return MatchmakingDedicatedServerOverrideError::ERR_OK;
    }

    MatchmakingFillServersOverrideError::Error SearchSlaveImpl::processMatchmakingFillServersOverride(
        const Blaze::GameManager::MatchmakingFillServersOverrideList &request, const Message* message)
    {
        mMatchmakingSlave.fillServersOverride(request.getGameIdList());
        return MatchmakingFillServersOverrideError::ERR_OK;
    }

} // namespace Search
} // namespace Blaze
