/*! ************************************************************************************************/
/*!
    \file findgameshardingbroker.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/component/componentmanager.h"
#include "gamemanager/searchcomponent/findgameshardingbroker.h"
#include "gamemanager/tdf/matchmaker_component_config_server.h"
#include "gamemanager/matchmakercomponent/matchmakerslaveimpl.h"
#include "gamemanager/tdf/matchmakingmetrics_server.h"
#include "gamemanager/matchmaker/matchmaker.h"

namespace Blaze
{
namespace Search
{
    FindGameShardingBroker::FindGameShardingBroker() :
        mHostingSearchSlaveListsByMatchmakingSessionId(BlazeStlAllocator("mHostingSearchSlaveListsByMatchmakingSessionId"))
    {
    }

    FindGameShardingBroker::~FindGameShardingBroker()
    {
        //need to clean up the fg to search slave mapping
        SlaveInstanceIdListMatchmakingSessionId::iterator hostingSearchInstancesIter = mHostingSearchSlaveListsByMatchmakingSessionId.begin();
        while(hostingSearchInstancesIter !=mHostingSearchSlaveListsByMatchmakingSessionId.end())
        {
            delete hostingSearchInstancesIter->second;
            hostingSearchInstancesIter->second = nullptr;
            ++hostingSearchInstancesIter;
        }
    }

    /*! ************************************************************************************************/
    /*! \brief param[in] saveDiagnostics If true, update the MM session's diagnostics counts based on current match counts
    ***************************************************************************************************/
    BlazeRpcError FindGameShardingBroker::startMatchmakingFindGameSession(GameManager::MatchmakingSessionId sessionId,
        const GameManager::StartMatchmakingInternalRequest &startMatchmakingRequest,
        GameManager::MatchmakingCriteriaError &criteriaError, TimeValue multiRpcTimeout, EA::TDF::tdf_ptr<GameManager::MatchmakingSubsessionDiagnostics> diagnostics)
    {
        SlaveInstanceIdList resolvedSearchSlaveList;
        getFullCoverageSearchSet(resolvedSearchSlaveList);

        if (resolvedSearchSlaveList.empty())
        {
            ERR_LOG("[FindGameShardingBroker].startMatchmakingFindGameSession()"
                    " No search slave instances available.");
            return Blaze::ERR_SYSTEM;
        }

        Blaze::Search::SearchSlave* searchComponent = static_cast<Blaze::Search::SearchSlave*>(
            gController->getComponentManager().getComponent(Blaze::Search::SearchSlave::COMPONENT_ID));
        if (searchComponent == nullptr)
        {
            ERR_LOG("[FindGameShardingBroker].startMatchmakingFindGameSession()"
                " Unable to get instance - this matchmaking session will be bogus, session id " 
                << sessionId);

            return Blaze::ERR_SYSTEM;
        }

        TRACE_LOG("[FindGameShardingBroker].startMatchmakingFindGameSession()"
            " starting find game session for, session id " << sessionId << ".");

        RpcCallOptions opts;
        opts.timeoutOverride = multiRpcTimeout;

        StartFindGameMatchmakingRequest request;
        request.setMatchmakingSessionId(sessionId);
        request.setStartMatchmakingInternalRequest(const_cast<Blaze::GameManager::StartMatchmakingInternalRequest&>(startMatchmakingRequest));

        // insert this matchmaking session into the map so we can clean it up later
        SlaveInstanceIdList *slaveInstanceIdList = BLAZE_NEW SlaveInstanceIdList();
        slaveInstanceIdList->assign(resolvedSearchSlaveList.begin(), resolvedSearchSlaveList.end());
        mHostingSearchSlaveListsByMatchmakingSessionId.insert(eastl::make_pair(sessionId, slaveInstanceIdList));

        SearchSlaveMultiResponse responses(resolvedSearchSlaveList);
        BlazeRpcError error = searchComponent->sendMultiRequest(Blaze::Search::SearchSlave::CMD_INFO_STARTFINDGAMEMATCHMAKING, &request, responses, opts);
        bool success = false;

        if (error != ERR_OK)
        {
            for (Component::MultiRequestResponseList::const_iterator itr = responses.begin(), end = responses.end(); itr != end; ++itr)
            {
                if ( (*itr)->error == ERR_OK )
                {
                    success = true;
                }
                else
                {
                    //log all instance's errors for debugging network etc
                    WARN_LOG("[FindGameShardingBroker].startMatchmakingFindGameSession() session " << sessionId <<
                        " got an error " << ErrorHelp::getErrorName(error) << " from Search instance " << (*itr)->instanceId << ".");
                }
            }

            // If we got OK back from at least one search slave, we treat it as a success, only log and return an error if every single search slave failed
            if (!success)
            {
                ERR_LOG("[FindGameShardingBroker].startMatchmakingFindGameSession() session " << sessionId << 
                    " failed, all search slaves returned errors.");

                // failed to create session - cancel it now
                // in case we timed out on one or more search slaves, make sure to clean up remote session, otherwise just clean local session
                cancelMatchmakingFindGameSession(sessionId, GameManager::SESSION_CANCELED);

                return error;
            }
        }

        // return session's diagnostic counts to MM for collation. use cached ptr
        if (diagnostics != nullptr)
        {
            for (auto itr = responses.begin(), end = responses.end(); itr != end; ++itr)
            {
                if (((*itr)->error != ERR_OK) || ((*itr)->response == nullptr))
                {
                    continue;//logged above
                }

                ASSERT_COND_LOG((StartFindGameMatchmakingResponse::TDF_ID == (*itr)->response->getTdfId()), "[FindGameShardingBroker].startMatchmakingFindGameSession: search response type(" << ((*itr)->response->getClassName() != nullptr ? (*itr)->response->getClassName() : "<nullptr>") << ":tdfId " << (*itr)->response->getTdfId() << ") was not expected response type(tdfId " << StartFindGameMatchmakingResponse::TDF_ID << "). Cannot tally its diagnostics.");
                if (StartFindGameMatchmakingResponse::TDF_ID != (*itr)->response->getTdfId())
                {
                    continue;
                }
                const StartFindGameMatchmakingResponse& rsp = static_cast<const StartFindGameMatchmakingResponse&>(*(*itr)->response);

                GameManager::Matchmaker::DiagnosticsTallyUtils::addDiagnosticsCountsToOther(*diagnostics,
                    rsp.getDiagnostics(), false, true, true);
            }
        }

        return ERR_OK;
    }

    eastl::string& FindGameShardingBroker::makeMultiResponseErrorLogStr(const SearchSlaveMultiResponse& responses, eastl::string &errorString) const
    {
        for (Component::MultiRequestResponseList::const_iterator itr = responses.begin(), end = responses.end(); itr != end; ++itr)
        {
            errorString.append_sprintf("\n\tSearch server instanceId %u got %s", (*itr)->instanceId, ErrorHelp::getErrorName((*itr)->error));
        }
        return errorString;
    }


    void FindGameShardingBroker::cancelMatchmakingFindGameSession(GameManager::MatchmakingSessionId sessionId, GameManager::MatchmakingResult result)
    {
        SlaveInstanceIdListMatchmakingSessionId::iterator hostingSlaveIter = mHostingSearchSlaveListsByMatchmakingSessionId.find(sessionId);
        if ((hostingSlaveIter == mHostingSearchSlaveListsByMatchmakingSessionId.end()) || (hostingSlaveIter->second == nullptr))
        {
            return; 
        }
        
        SlaveInstanceIdList *hostingSearchSlaveList = hostingSlaveIter->second;
        // since we're making blocking calls, remove the list from the map, and the entry for this user
        // this will prevent it from being deleted from underneath us or an erroneously caused double cancel attempt
        // to send double notifications
        hostingSlaveIter->second = nullptr;
        mHostingSearchSlaveListsByMatchmakingSessionId.erase(hostingSlaveIter);

        Blaze::Search::SearchSlave* searchComponent = static_cast<Blaze::Search::SearchSlave*>(
            gController->getComponentManager().getComponent(Blaze::Search::SearchSlave::COMPONENT_ID));
        if (searchComponent == nullptr)
        {
            ERR_LOG("[FindGameShardingBroker].cancelMatchmakingFindGameSession()"
                " Unable to get instance - cannot cancel find game matchmaking session, session id " << sessionId << ".");
        }

        RpcCallOptions opts;
        opts.ignoreReply = true;

        TerminateFindGameMatchmakingRequest request;
        request.setMatchmakingSessionId(sessionId);
        request.setMatchmakingResult(result);
        for (SlaveInstanceIdList::const_iterator it = hostingSearchSlaveList->begin(),
            itEnd = hostingSearchSlaveList->end();
            it != itEnd; ++it)
        {
            opts.routeTo.setInstanceId(*it);

            BlazeRpcError error = searchComponent->terminateFindGameMatchmaking(request, opts);
            if (error != Blaze::ERR_OK)
            {
                TRACE_LOG("[FindGameShardingBroker].cancelMatchmakingFindGameSessionFiber()"
                    " Could not cancel find game session, session id " << request.getMatchmakingSessionId() << ".");
            }
        }

        // delete the list
        delete hostingSearchSlaveList;
        return;
    }

} // namespace Search
} // namespace Blaze
