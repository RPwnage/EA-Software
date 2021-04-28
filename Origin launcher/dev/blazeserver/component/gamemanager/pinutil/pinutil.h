/*! ************************************************************************************************/
/*!
    \file pinutil.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_PIN_UTIL_H
#define BLAZE_GAMEMANAGER_PIN_UTIL_H

#include "framework/tdf/riverposter.h"
#include "gamemanager/gamesessionmaster.h"
#include "gamemanager/playerrostermaster.h"

#include "EATDF/tdfbase.h"

namespace Blaze
{
namespace GameManager
{
    namespace Matchmaker 
    {
        class MatchmakingSession;
    }

    typedef eastl::vector_set<BlazeId> PlayerIdSet;
    typedef eastl::map<UserSessionId, PlayerIdSet> RelatedPlayerSetsMap;
    class PackerScenarioInternalRequest;

    class PINEventHelper
    {
    public:

        static void matchJoined(const PlayerInfoMaster &joiningPlayer, const GameSetupReason &setupReason, 
            const GameSessionMaster &gameSession, const RelatedPlayerSetsMap &relatedPlayerLists, const char8_t* trackingTag, PINSubmission &pinSubmission);

        static void matchLeft(const PlayerInfoMaster &leavingPlayer, const PlayerRemovalContext &removalContext, const GameSessionMaster &gameSession, PINSubmission &pinSubmission);

        static void gameEnd(PlayerId playerId, GameId gameId, const GameInfo& gameInfo, const EA::TDF::Tdf& report, PINSubmission &pinSubmission, bool includeCustom = true);

        static void matchInfoForAllMembers(const GameSessionMaster &gameSession, PINSubmission &pinSubmission, const char8_t* statusString);

        static void populatePlayerSpecificJoinInfo(const UserSessionInfo &joiningPlayerInfo, RiverPoster::MPMatchJoinEvent &matchJoinEvent);
        static void populatePlayerSpecificMatchInfo( RiverPoster::MPMatchInfoEvent &newMatchInfoEvent, const PlayerInfoMaster &currentMember );

        static void buildMatchInfoForGameSession(const GameSessionMaster &gameSession, RiverPoster::MPMatchInfoEvent &matchInfoEvent, const char8_t* statusString);

        static void buildMatchJoinFailedEvent(const GameManager::JoinGameByGroupMasterRequest &joinRequest, BlazeRpcError joinError, const GameSessionMasterPtr gameSession,
            const RelatedPlayerSetsMap &relatedPlayerLists, PINSubmission &pinSubmission);

        static void buildMatchJoinFailedEvent(const GameManager::JoinGameMasterRequest &joinRequest, BlazeRpcError joinError, const GameSessionMasterPtr gameSession,
            const RelatedPlayerSetsMap &relatedPlayerLists, PINSubmission &pinSubmission);

        static void buildMatchJoinForFailedMatchmakingSession(const Matchmaker::MatchmakingSession& mmSession, GameManager::MatchmakingResult result,
            const GameManager::UserSessionInfo& info, PINSubmission &pinSubmission);

        static void buildMatchJoinForFailedPackerScenario(const GameManager::PackerScenarioInternalRequest& internalRequest, const GameManager::UserJoinInfo& initiatorJoinInfo,
            const GameManager::UserJoinInfoList friendsList, const TimeValue& duration, GameManager::MatchmakingResult result, const GameManager::UserSessionInfo& info, PINSubmission& pinSubmission);

        static void buildMatchJoinForGameSession(const GameSessionMaster &gameSession, RiverPoster::MPMatchJoinEvent &matchJoinEvent);

        static const char8_t* getBlazeGameTypeAsString(GameType gameType);

        static RiverPoster::PINEventList * getUserSessionPINEventList( PINSubmission &pinSubmission, UserSessionId sessionId);

        static void addScenarioAttributesToMatchJoinEvent(RiverPoster::PINEventList& pinEventList, const ScenarioAttributes& scenarioAttributes);
    };
} // namespace GameManager
} // namespace Blaze

#endif //BLAZE_GAMEMANAGER_PIN_UTIL_H

