/*! ************************************************************************************************/
/*!
    \file findgameshardingbroker.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_FIND_GAME_SHARDING_BROKER_H
#define BLAZE_FIND_GAME_SHARDING_BROKER_H

#include "gamemanager/rpc/searchslave_stub.h"
#include "gamemanager/tdf/search_server.h"
#include "gamemanager/searchcomponent/searchshardingbroker.h"
#include "EASTL/map.h"

namespace Blaze
{
namespace GameManager
{
    class MatchmakingSubsessionDiagnostics;
}
namespace Matchmaker
{
    class MatchmakerSlaveImpl;
}

namespace Search
{

    typedef eastl::vector<Blaze::GameManager::MatchmakingSessionId> MatchmakingSessionIdList;

    // This class is only to be inherited from
    class FindGameShardingBroker : public SearchShardingBroker
    {
        NON_COPYABLE(FindGameShardingBroker);
    protected:
        FindGameShardingBroker();
        ~FindGameShardingBroker() override;

        // sharded functionality
        BlazeRpcError startMatchmakingFindGameSession(GameManager::MatchmakingSessionId sessionId, const GameManager::StartMatchmakingInternalRequest &request, GameManager::MatchmakingCriteriaError &criteriaError, TimeValue multiRpcTimeout, EA::TDF::tdf_ptr<GameManager::MatchmakingSubsessionDiagnostics> diagnostics = nullptr);
        void cancelMatchmakingFindGameSession(GameManager::MatchmakingSessionId sessionId, GameManager::MatchmakingResult result);

        // accessors

        // search slave notifications
        void onNotifyGameListUpdate(const Blaze::Search::NotifyGameListUpdate& data,UserSession *associatedUserSession) override {} // never called, these notifications are targeted by session id  

        typedef Component::MultiRequestResponseHelper<Blaze::Search::StartFindGameMatchmakingResponse, Blaze::GameManager::MatchmakingCriteriaError> SearchSlaveMultiResponse;
        eastl::string& makeMultiResponseErrorLogStr(const SearchSlaveMultiResponse& multiResponse, eastl::string &buf) const;

        typedef eastl::hash_map<GameManager::MatchmakingSessionId, SearchShardingBroker::SlaveInstanceIdList*> SlaveInstanceIdListMatchmakingSessionId;
        SlaveInstanceIdListMatchmakingSessionId mHostingSearchSlaveListsByMatchmakingSessionId;
    };

} // namespace GameManager
} // namespace Blaze

#endif // BLAZE_FIND_GAME_SHARDING_BROKER_H
