/*! ************************************************************************************************/
/*!
    \file pinutil.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/shared/rawbufferistream.h"
#include "gamemanager/pinutil/pinutil.h"
#include "gamemanager/matchmaker/matchmakingsession.h"

#include "EAJson/JsonWriter.h"
#include "EAIO/EAStream.h"

#include "EATDF/codec/tdfjsonencoder.h"

namespace Blaze
{
    namespace GameManager
    {
        template<class T>
        static T* createPinTdf(const char8_t* allocName)
        {
            auto* tdf = EA::TDF::TdfFactory::get().create(T::TDF_ID, *Allocator::getAllocator(), allocName);
            return static_cast<T*>(tdf);
        }

        void PINEventHelper::matchJoined(const PlayerInfoMaster &joiningPlayer, const GameSetupReason &setupReason, const GameSessionMaster &gameSession,
                                         const RelatedPlayerSetsMap &relatedPlayerLists, const char8_t* trackingTag, PINSubmission &pinSubmission)
        {
            // only need an event for the joining player
            auto* newPinEventList = getUserSessionPINEventList(pinSubmission, joiningPlayer.getPlayerSessionId());
            EA_ASSERT_MSG(newPinEventList != nullptr, "pin event list cannot be null!");

            // first set up the match join event
            RiverPoster::MPMatchJoinEventPtr matchJoinEvent = createPinTdf<RiverPoster::MPMatchJoinEvent>("RiverPoster::MPMatchJoinEvent");
            if (matchJoinEvent == nullptr)
            {
                // something went wrong
                return;
            }

            StringBuilder sb;
            // build out the join event
            buildMatchJoinForGameSession(gameSession, *matchJoinEvent);
            InetAddress clientIp;
            clientIp.setHostnameAndPort(joiningPlayer.getConnectionAddr());
            matchJoinEvent->getHeaderCore().setClientIp(clientIp.getHostname());
            matchJoinEvent->setClientType(ClientTypeToPinString(joiningPlayer.getClientType()));
            BlazeId initiatorId = INVALID_BLAZE_ID;

            // mm info
            if (joiningPlayer.getJoinedViaMatchmaking())
            {
                matchJoinEvent->setJoinMethod(RiverPoster::MATCH_JOIN_METHOD_MATCHMAKING);
                const ScenarioInfo scenarioInfo = joiningPlayer.getPlayerDataMaster()->getScenarioInfo();
                if (scenarioInfo.getScenarioName()[0] != '\0')
                {
                    matchJoinEvent->setScenarioName(scenarioInfo.getScenarioName());
                    if (scenarioInfo.getSubSessionName()[0] != '\0')
                    {
                        matchJoinEvent->setScenarioSubsessionName(scenarioInfo.getSubSessionName());
                    }
                    sb.reset() << scenarioInfo.getScenarioVersion();
                    matchJoinEvent->setScenarioVersion(sb.get());
                    matchJoinEvent->setScenarioVariant(scenarioInfo.getScenarioVariant());
                    // set tracking tag only if populated from reco service
                    if (trackingTag != nullptr && *trackingTag != '\0')
                    {
                        matchJoinEvent->setTrackingTag(trackingTag);
                    }

                    for (auto& scenIt : scenarioInfo.getScenarioAttributes())
                    {
                        EA::TDF::GenericTdfType* gen = scenIt.second.get();
                        const EA::TDF::TdfGenericValue& val = gen->get();
                        if (val.getType() != EA::TDF::TDF_ACTUAL_TYPE_STRING)
                        {
                            sb.reset();
                            EA::TDF::PrintEncoder printEncoder;
                            EA::TDF::TdfGenericReferenceConst ref(val);          // Get a reference to the value the Generic is pointing to (not the Generic itself)
                            printEncoder.print(sb, ref);
                            matchJoinEvent->getScenarioParams()[scenIt.first].set(sb.get(), sb.length());
                        }
                        else
                        {
                            // Just add the string without running it through PrintEncoder as the encoder
                            // will wrap the value in quotes
                            matchJoinEvent->getScenarioParams()[scenIt.first] = val.asString();
                        }
                    }
                }

                switch (setupReason.getActiveMember())
                {
                    case GameSetupReason::MEMBER_MATCHMAKINGSETUPCONTEXT:
                        matchJoinEvent->setMatchmakingDurationSeconds(setupReason.getMatchmakingSetupContext()->getTimeToMatch().getSec());
                        matchJoinEvent->setEstimatedTimeToMatchSeconds(setupReason.getMatchmakingSetupContext()->getEstimatedTimeToMatch().getSec());
                        matchJoinEvent->setMatchmakingTimeoutDuration(setupReason.getMatchmakingSetupContext()->getMatchmakingTimeoutDuration().getSec());
                        matchJoinEvent->setMatchmakingFitScore(setupReason.getMatchmakingSetupContext()->getFitScore());
                        matchJoinEvent->setMatchmakingMaxFitScore(setupReason.getMatchmakingSetupContext()->getMaxPossibleFitScore());
                        matchJoinEvent->setTotalUsersOnline(setupReason.getMatchmakingSetupContext()->getTotalUsersOnline());
                        matchJoinEvent->setTotalUsersInGame(setupReason.getMatchmakingSetupContext()->getTotalUsersInGame());
                        matchJoinEvent->setTotalUsersInMatchmaking(setupReason.getMatchmakingSetupContext()->getTotalUsersInMatchmaking());
                        matchJoinEvent->setTotalUsersMatched(setupReason.getMatchmakingSetupContext()->getTotalUsersMatched());
                        matchJoinEvent->setTotalUsersPotentiallyMatched(setupReason.getMatchmakingSetupContext()->getTotalUsersPotentiallyMatched());

                        if (setupReason.getMatchmakingSetupContext()->getMatchmakingResult() == MatchmakingResult::SUCCESS_JOINED_EXISTING_GAME)
                        {
                            matchJoinEvent->setJoinStatus(joiningPlayer.isReserved() ? RiverPoster::MATCH_JOIN_STATUS_MATCHMAKING_FOUND_GAME_RESERVED : RiverPoster::MATCH_JOIN_STATUS_MATCHMAKING_FOUND_GAME);
                        }
                        else
                        {
                            matchJoinEvent->setJoinStatus(joiningPlayer.isReserved() ? RiverPoster::MATCH_JOIN_STATUS_MATCHMAKING_CREATED_GAME_RESERVED : RiverPoster::MATCH_JOIN_STATUS_MATCHMAKING_CREATED_GAME);
                        }

                        initiatorId = setupReason.getMatchmakingSetupContext()->getInitiatorId();
                        break;
                    case GameSetupReason::MEMBER_INDIRECTMATCHMAKINGSETUPCONTEXT:
                        matchJoinEvent->setMatchmakingDurationSeconds(setupReason.getIndirectMatchmakingSetupContext()->getTimeToMatch().getSec());
                        matchJoinEvent->setEstimatedTimeToMatchSeconds(setupReason.getIndirectMatchmakingSetupContext()->getEstimatedTimeToMatch().getSec());
                        matchJoinEvent->setMatchmakingTimeoutDuration(setupReason.getIndirectMatchmakingSetupContext()->getMatchmakingTimeoutDuration().getSec());
                        matchJoinEvent->setMatchmakingFitScore(setupReason.getIndirectMatchmakingSetupContext()->getFitScore());
                        matchJoinEvent->setMatchmakingMaxFitScore(setupReason.getIndirectMatchmakingSetupContext()->getMaxPossibleFitScore());
                        matchJoinEvent->setTotalUsersOnline(setupReason.getIndirectMatchmakingSetupContext()->getTotalUsersOnline());
                        matchJoinEvent->setTotalUsersInGame(setupReason.getIndirectMatchmakingSetupContext()->getTotalUsersInGame());
                        matchJoinEvent->setTotalUsersInMatchmaking(setupReason.getIndirectMatchmakingSetupContext()->getTotalUsersInMatchmaking());
                        matchJoinEvent->setTotalUsersMatched(setupReason.getIndirectMatchmakingSetupContext()->getTotalUsersMatched());
                        matchJoinEvent->setTotalUsersPotentiallyMatched(setupReason.getIndirectMatchmakingSetupContext()->getTotalUsersPotentiallyMatched());

                        if (setupReason.getIndirectMatchmakingSetupContext()->getMatchmakingResult() == MatchmakingResult::SUCCESS_JOINED_EXISTING_GAME)
                        {
                            matchJoinEvent->setJoinStatus(joiningPlayer.isReserved() ? RiverPoster::MATCH_JOIN_STATUS_MATCHMAKING_FOUND_GAME_RESERVED : RiverPoster::MATCH_JOIN_STATUS_MATCHMAKING_FOUND_GAME);
                        }
                        else
                        {
                            matchJoinEvent->setJoinStatus(joiningPlayer.isReserved() ? RiverPoster::MATCH_JOIN_STATUS_MATCHMAKING_CREATED_GAME_RESERVED : RiverPoster::MATCH_JOIN_STATUS_MATCHMAKING_CREATED_GAME);
                        }

                        initiatorId = setupReason.getIndirectMatchmakingSetupContext()->getInitiatorId();
                        break;
                    case GameSetupReason::MEMBER_DATALESSSETUPCONTEXT:
                        if (setupReason.getDatalessSetupContext()->getSetupContext() == INDIRECT_JOIN_GAME_FROM_RESERVATION_CONTEXT)
                        {
                            // claiming a reservation originally made via matchmaking
                            matchJoinEvent->setJoinStatus(RiverPoster::MATCH_JOIN_STATUS_SUCCESS);
                        }
                        break;
                    default:
                        WARN_LOG("[PINEventHelper].matchJoined: unhandled setup reason(" << setupReason.getActiveMember() << "), for player(" << joiningPlayer.getPlayerId() << ") that joined via matchmaking, PlayerState(" << PlayerStateToString(joiningPlayer.getPlayerState()) << "), Game(" << gameSession.getGameId() << ")");
                        break; // nothing to do
                }
            }
            else 
            {
                matchJoinEvent->setJoinStatus(joiningPlayer.isReserved() ? RiverPoster::MATCH_JOIN_STATUS_SUCCESS_RESERVED : RiverPoster::MATCH_JOIN_STATUS_SUCCESS);
                switch (joiningPlayer.getConnectionJoinType())
                {
                    case CREATED:
                        matchJoinEvent->setJoinMethod(RiverPoster::MATCH_JOIN_METHOD_CREATED);
                        initiatorId = joiningPlayer.getPlayerId();
                        break;
                    case DIRECTJOIN:
                        matchJoinEvent->setJoinMethod(RiverPoster::MATCH_JOIN_METHOD_DIRECT);
                        initiatorId = joiningPlayer.getPlayerId();
                        break;
                    default:
                        // don't set if we don't know what we had
                        break;
                }
            }

            if (initiatorId != INVALID_BLAZE_ID)
            {
                sb.reset() << initiatorId;
                matchJoinEvent->getInitiatorId()[RiverPoster::PIN_PLAYER_ID_TYPE_PERSONA].set(sb.get(), sb.length());
            }

            // friend info (GG members, or target player for join by player/invite)
            RelatedPlayerSetsMap::const_iterator relatedPlayersIter = relatedPlayerLists.find(joiningPlayer.getUserInfo().getSessionId());
            if (relatedPlayersIter != relatedPlayerLists.end())
            {
                PlayerIdSet::const_iterator usersIter = relatedPlayersIter->second.begin();
                PlayerIdSet::const_iterator usersEnd = relatedPlayersIter->second.end();
                for(; usersIter != usersEnd; ++usersIter)
                {
                    BlazeId blazeId = (*usersIter);
                    const PlayerInfoMaster* friendPtr = gameSession.getPlayerRoster()->getPlayer(blazeId);
                    if (friendPtr != nullptr)
                    {
                        RiverPoster::FriendInfo* friendInfo = matchJoinEvent->getFriendsList().allocate_element();
                        eastl::string blazeIdString;
                        blazeIdString.sprintf("%" PRIu64, blazeId);
                        friendInfo->setPlayerId(blazeIdString.c_str());
                        friendInfo->setPlayerIdType(RiverPoster::PIN_PLAYER_ID_TYPE_PERSONA);
                        friendInfo->setPlatform(ClientPlatformTypeToString(friendPtr->getPlatformInfo().getClientPlatform()));
                        bool inGroup = false;
                        // we call users that are part of your mm session your 'group'
                        if (joiningPlayer.getTargetPlayerId() != INVALID_BLAZE_ID)
                        {
                            inGroup = false;
                            const PlayerInfoMaster *targetPlayer = gameSession.getPlayerRoster()->getPlayer(joiningPlayer.getTargetPlayerId());
                            if (targetPlayer != nullptr)
                            {
                                if (targetPlayer->getUserGroupId() == joiningPlayer.getUserGroupId())
                                {
                                    // target player is in same group
                                    inGroup = true;
                                }
                            }
                        }
                        else
                        {
                            inGroup = true;
                        }
                        friendInfo->setInGroup(inGroup);
                        matchJoinEvent->getFriendsList().push_back(friendInfo);
                    }
                }

            }

            matchJoinEvent->setPlayerRole(joiningPlayer.getRoleName());

            populatePlayerSpecificJoinInfo(joiningPlayer.getUserInfo(), *matchJoinEvent);

            // now insert the join event into the user's pin request
            newPinEventList->pull_back()->set(matchJoinEvent.get());

            // next, set up the match info event
            RiverPoster::MPMatchInfoEventPtr matchInfoEvent = createPinTdf<RiverPoster::MPMatchInfoEvent>("RiverPoster::MPMatchInfoEvent");
            if (matchInfoEvent == nullptr)
            {
                // something went wrong, clean up and bail.
                return;
            }
            buildMatchInfoForGameSession(gameSession, *matchInfoEvent, RiverPoster::MATCH_INFO_STATUS_JOIN);
            matchInfoEvent->setJoinMethod(matchJoinEvent->getJoinMethod());
            // update user-specific stuff
            // much of this is on the user session, particularly in the HeaderCore, and we populate it on the usersessionmanager
            populatePlayerSpecificMatchInfo(*matchInfoEvent, joiningPlayer);

            // now insert the event into the user's pin request
            newPinEventList->pull_back()->set(matchInfoEvent.get());
        }

        // match left
        void PINEventHelper::matchLeft(const PlayerInfoMaster &leavingPlayer, const PlayerRemovalContext &removalContext, const GameSessionMaster &gameSession, PINSubmission &pinSubmission) 
        {
            // only need an event for the departing player
            RiverPoster::PINEventList *newPinEventList = getUserSessionPINEventList(pinSubmission, leavingPlayer.getPlayerSessionId());
            EA_ASSERT_MSG(newPinEventList != nullptr, "pin event list cannot be null!");

            RiverPoster::MPMatchInfoEventPtr matchInfoEvent = createPinTdf<RiverPoster::MPMatchInfoEvent>("RiverPoster::MPMatchInfoEvent");
            if (matchInfoEvent == nullptr)
            {
                // something went wrong, clean up and bail.
                return;
            }

            buildMatchInfoForGameSession(gameSession, *matchInfoEvent, RiverPoster::MATCH_INFO_STATUS_LEAVE);
            // update user-specific stuff
            // much of this is on the user session, particularly in the HeaderCore, and we populate it on the usersessionmanager
            populatePlayerSpecificMatchInfo(*matchInfoEvent, leavingPlayer);
            matchInfoEvent->setStatusCode(PlayerRemovedReasonToString(removalContext.mPlayerRemovedReason));
            switch(removalContext.mPlayerRemovedReason)
            {
                case GAME_DESTROYED:
                case GAME_ENDED:
                case PLAYER_LEFT:
                case PLAYER_LEFT_MAKE_RESERVATION:
                case PLAYER_LEFT_CANCELLED_MATCHMAKING:
                case PLAYER_LEFT_SWITCHED_GAME_SESSION:
                case GROUP_LEFT:
                case GROUP_LEFT_MAKE_RESERVATION:
                case PLAYER_KICKED:
                case PLAYER_KICKED_WITH_BAN:
                case PLAYER_RESERVATION_TIMEOUT:
                case HOST_EJECTED:
                case RESERVATION_TRANSFER_TO_NEW_USER:
                case DISCONNECT_RESERVATION_TIMEOUT:
                    matchInfoEvent->setEndReason(RiverPoster::MATCH_INFO_END_REASON_NORMAL);
                    break;
                default:
                    matchInfoEvent->setEndReason(RiverPoster::MATCH_INFO_END_REASON_ERROR);
                    break;
            }

            if ((gameSession.getGameType() == GAME_TYPE_GAMESESSION) && !gameSession.isGamePreMatch())
            {
                char8_t leaveTimestamp[64];
                TimeValue::getTimeOfDay().toAccountString(leaveTimestamp, 64, true);
                matchInfoEvent->setMatchEndTimestamp(leaveTimestamp);
            }

            // now insert the event into the user's pin request
            newPinEventList->pull_back()->set(matchInfoEvent.get());
        }

        void PINEventHelper::matchInfoForAllMembers(const GameSessionMaster &gameSession, PINSubmission &pinSubmission, const char8_t* statusString) 
        {
            // for the most part, we have one event with minor changes for each member of the game
            // so build one event out, and duplicate then modify for the individual game members
            RiverPoster::MPMatchInfoEvent matchInfoEvent;
            buildMatchInfoForGameSession(gameSession, matchInfoEvent, statusString);

            // now iterate over the game roster, give everyone their own matchInfoEvent.
            PlayerRoster::PlayerInfoList playerRoster = gameSession.getPlayerRoster()->getPlayers(PlayerRoster::ROSTER_PLAYERS);
            PlayerRoster::PlayerInfoList::const_iterator rosterIter = playerRoster.begin();
            PlayerRoster::PlayerInfoList::const_iterator rosterEnd = playerRoster.end();
            for(; rosterIter != rosterEnd; ++rosterIter)
            {
                const PlayerInfoMaster *currentMember = static_cast<const PlayerInfoMaster*>(*rosterIter);
                RiverPoster::PINEventList *newPinEventList = getUserSessionPINEventList(pinSubmission, currentMember->getPlayerSessionId());
                EA_ASSERT_MSG(newPinEventList != nullptr, "pin event list cannot be null!");
                // set up the match info event
                // and insert the event into the user's pin request
                RiverPoster::MPMatchInfoEventPtr newMatchInfoEvent = matchInfoEvent.clone();
                newPinEventList->pull_back()->set(newMatchInfoEvent.get());

                // update user-specific stuff
                // much of the user-specific stuff is the user session, particularly in the HeaderCore, and we populate it on the usersessionmanager
                populatePlayerSpecificMatchInfo(*newMatchInfoEvent, *currentMember);
            }
        }

        void PINEventHelper::populatePlayerSpecificJoinInfo(const UserSessionInfo &joiningPlayerInfo, RiverPoster::MPMatchJoinEvent &matchJoinEvent)
        {
            // Insert the player's QoS ping site latencies into the event
            for (PingSiteLatencyByAliasMap::const_iterator it = joiningPlayerInfo.getLatencyMap().begin(); it != joiningPlayerInfo.getLatencyMap().end(); ++it)
            {
                matchJoinEvent.getQoSLatencies()[it->first.c_str()] = it->second;
            }

            // Insert the player's UED into the event
            for (auto& uedItr : joiningPlayerInfo.getDataMap())
            {
                const char8_t* uedName = gUserSessionManager->getUserExtendedDataName(uedItr.first);
                if (uedName != nullptr)
                {
                    char8_t uedValueString[32];
                    auto len = blaze_snzprintf(uedValueString, sizeof(uedValueString), "%" PRIi64, uedItr.second);
                    matchJoinEvent.getUserExtendedData()[uedName].set(uedValueString, len);
                }
            }

            UserSessionPtr session = gUserSessionManager->getSession(joiningPlayerInfo.getSessionId());
            if (session != nullptr)
            {
                matchJoinEvent.getHeaderCore().setSdkVersion(session->getBlazeSDKVersion());
            }
        }

        void PINEventHelper::populatePlayerSpecificMatchInfo( RiverPoster::MPMatchInfoEvent &newMatchInfoEvent, const PlayerInfoMaster &currentMember )
        {
            InetAddress clientIp;
            clientIp.setHostnameAndPort(currentMember.getConnectionAddr());
            newMatchInfoEvent.getHeaderCore().setClientIp(clientIp.getHostname());
            if (EA_LIKELY(currentMember.getUserInfo().getBestPingSiteAlias()[0] != '\0'))
            {
                newMatchInfoEvent.setPlayerPingSite(currentMember.getUserInfo().getBestPingSiteAlias());
            }
            else
            {
                newMatchInfoEvent.setPlayerPingSite(UNKNOWN_PINGSITE);
            }
            
            newMatchInfoEvent.setClientType(ClientTypeToPinString(currentMember.getClientType()));
            if (currentMember.getJoinedGameTimestamp() != JOINED_GAME_TIMESTAMP_NOT_JOINED)
            {
                char8_t joinTimestamp[64];
                currentMember.getJoinedGameTimestamp().toAccountString(joinTimestamp, 64, true);
                newMatchInfoEvent.setPlayerJoinTimestamp(joinTimestamp);
            }
            RiverPoster::PlayerStats& playerStats = newMatchInfoEvent.getPlayerStats();
            playerStats.insert(eastl::make_pair(RiverPoster::MATCH_INFO_STATS_KEY_TEAM_INDEX, currentMember.getTeamIndex()));

            char8_t playerIdString[32];
            blaze_snzprintf(playerIdString, sizeof(playerIdString), "%" PRIi64, currentMember.getPlayerId());
            newMatchInfoEvent.getHeaderCore().setPlayerId(playerIdString);
            newMatchInfoEvent.getHeaderCore().setPlayerIdType(RiverPoster::PIN_PLAYER_ID_TYPE_PERSONA);

            newMatchInfoEvent.setPlayerRole(currentMember.getRoleName());

            UserSessionPtr session = gUserSessionManager->getSession(currentMember.getUserInfo().getSessionId());
            if (session != nullptr)
            {
                newMatchInfoEvent.getHeaderCore().setSdkVersion(session->getBlazeSDKVersion());
            }

            const_cast<GameManager::PlayerInfoMaster*>(&currentMember)->getPlayerAttribs().copyInto(newMatchInfoEvent.getPlayerAttributes());
        }

        void PINEventHelper::buildMatchInfoForGameSession(const GameSessionMaster &gameSession, RiverPoster::MPMatchInfoEvent &matchInfoEvent, const char8_t* statusString)
        {
            matchInfoEvent.getHeaderCore().setEventName(RiverPoster::MP_MATCH_INFO_EVENT_NAME);
            matchInfoEvent.getHeaderCore().setGameMode(gameSession.getGameMode());
            matchInfoEvent.getHeaderCore().setModeType(gameSession.getPINGameModeType());
            matchInfoEvent.getHeaderCore().setGameType(gameSession.getPINGameType());
            if (*gameSession.getPINGameMap() != '\0')
                matchInfoEvent.getHeaderCore().setMap(gameSession.getPINGameMap());
            matchInfoEvent.setMatchStatus(statusString);

            // Add all platform information:
            matchInfoEvent.setCrossplayEnabled(gameSession.isCrossplayEnabled() );
            matchInfoEvent.setPINIsCrossplayGame(gameSession.getPINIsCrossplayGame() );
            
            eastl::string gameReportingId;
            gameReportingId.sprintf("%" PRIu64, gameSession.getGameReportingId());
            matchInfoEvent.setMatchId(gameReportingId.c_str());

            eastl::string gameId;
            gameId.sprintf("%" PRIu64, gameSession.getGameId());
            matchInfoEvent.setGameId(gameId.c_str());
            matchInfoEvent.setGamePingSite(gameSession.getBestPingSiteAlias());
            matchInfoEvent.setBlazeGameType(getBlazeGameTypeAsString(gameSession.getGameType()));
            matchInfoEvent.setNetworkTopology(GameNetworkTopologyToString(gameSession.getGameNetworkTopology()));
            if (gameSession.getGameType() == GAME_TYPE_GAMESESSION)
            {
                if (gameSession.isGamePreMatch())
                {
                    matchInfoEvent.setGamePhase(GameStateToString(PRE_GAME));
                }
                else
                {
                    if (gameSession.isGameInProgress())
                    {
                        matchInfoEvent.setGamePhase(GameStateToString(IN_GAME));
                    }
                    else if (gameSession.isGamePostMatch())
                    {
                        matchInfoEvent.setGamePhase(GameStateToString(POST_GAME));
                        // this match has ended, generate match end data

                        char8_t matchEndTimestamp[64];
                        gameSession.getMatchEndTime().toAccountString(matchEndTimestamp, 64, true);
                        matchInfoEvent.setMatchEndTimestamp(matchEndTimestamp);
                    }

                    char8_t matchStartTimestamp[64];
                    gameSession.getMatchStartTime().toAccountString(matchStartTimestamp, 64, true);
                    // need a timestamp for match start
                    matchInfoEvent.setMatchStartTimestamp(matchStartTimestamp);
                }
            }

            char8_t gameCreationTimestamp[64];
            gameSession.getGameCreationTime().toAccountString(gameCreationTimestamp, 64, true);
            // need accessor for creation timestamp
            matchInfoEvent.setGameCreationTimestamp(gameCreationTimestamp);
                
            matchInfoEvent.setPlayerCount(gameSession.getPlayerRoster()->getParticipantCount());
            matchInfoEvent.setPlayerCapacity(gameSession.getTotalParticipantCapacity());
            matchInfoEvent.setTeamCount(gameSession.getTeamCount());
            // need to build out team info
            for(uint16_t teamIndex = 0; teamIndex < gameSession.getTeamCount(); ++teamIndex)
            {
                // PIN also would like to know team scores / rankings, but Blaze has no insight into game title logic to determine that
                RiverPoster::TeamInfo* teamInfo = matchInfoEvent.getTeamStatsList().allocate_element();
                teamInfo->setTeamIndex(teamIndex);
                teamInfo->setPlayerCount(gameSession.getPlayerRoster()->getTeamSize(teamIndex));
                PlayerState state = RESERVED;
                bool isParticipant = true;
                teamInfo->setReservedPlayerCount(gameSession.getPlayerRoster()->getPlayerCount(&state, nullptr, &isParticipant, &teamIndex));

                RiverPoster::UserExtendedDataStringMap &uedMap = teamInfo->getTeamUeds();
                TeamIndexToTeamUedMap::const_iterator teamUedMapItr = gameSession.getTeamUeds().find(teamIndex);
                if (teamUedMapItr != gameSession.getTeamUeds().end())
                {
                    for (auto& teamUedItr : teamUedMapItr->second)
                    {
                        eastl::string uedValue;
                        uedValue.sprintf("%" PRId64, teamUedItr.second);
                        uedMap[teamUedItr.first.c_str()] = uedValue.c_str();
                    }
                }

                GameManager::RoleMap &roleSizeMap = teamInfo->getTeamRoles();
                GameManager::RoleSizeMap teamRoleSizes;
                gameSession.getPlayerRoster()->getRoleSizeMap(teamRoleSizes, teamIndex);
                for (auto& teamRoleSizeItr : teamRoleSizes)
                {
                    roleSizeMap[teamRoleSizeItr.first] = teamRoleSizeItr.second;
                }

                matchInfoEvent.getTeamStatsList().push_back(teamInfo);
            }

            gameSession.getGameAttribs().copyInto(matchInfoEvent.getGameAttributes());
        }

        void PINEventHelper::buildMatchJoinFailedEvent(const GameManager::JoinGameByGroupMasterRequest &joinRequest, BlazeRpcError joinError, const GameSessionMasterPtr gameSession,
                                                        const RelatedPlayerSetsMap &relatedPlayerLists, PINSubmission &pinSubmission)
        {
            // the game may not exist, but if it does, set up the match join info
            RiverPoster::MPMatchJoinEvent baseJoinEvent;
            if (gameSession != nullptr)
            {
                buildMatchJoinForGameSession(*gameSession, baseJoinEvent);
            }
            else
            {
                // we have to get the game id in, at least
                eastl::string gameId;
                gameId.sprintf("%" PRIu64, joinRequest.getJoinRequest().getGameId());
                baseJoinEvent.setGameId(gameId.c_str());
                baseJoinEvent.setBlazeGameType(getBlazeGameTypeAsString(joinRequest.getJoinRequest().getCommonGameData().getGameType()));
                baseJoinEvent.getHeaderCore().setEventName(RiverPoster::MP_MATCH_JOIN_EVENT_NAME);
            }

            // fill out the pertinent failure info
            // this will only be set up for direct joins
            baseJoinEvent.setJoinMethod(RiverPoster::MATCH_JOIN_METHOD_DIRECT);
            baseJoinEvent.setJoinStatus(RiverPoster::MATCH_JOIN_STATUS_ERROR);
            baseJoinEvent.setStatusCode(ErrorHelp::getErrorName(joinError));

            // now iterate over all the joining players and set up the events
            UserJoinInfoList::const_iterator usersIter = joinRequest.getUsersInfo().begin();
            UserJoinInfoList::const_iterator usersEnd = joinRequest.getUsersInfo().end();
            for(; usersIter != usersEnd; ++usersIter)
            {
                UserSessionId sessionId = (*usersIter)->getUser().getSessionId();
                RiverPoster::PINEventList *newPinEventList = getUserSessionPINEventList(pinSubmission, sessionId);
                EA_ASSERT_MSG(newPinEventList != nullptr, "pin event list cannot be null!");
                RiverPoster::MPMatchJoinEventPtr matchJoinEvent = baseJoinEvent.clone();

                // set up the match info event
                // and insert the event into the user's pin request
                newPinEventList->pull_back()->set(matchJoinEvent.get());

                // set up user-specific stuff in the base event
                InetAddress clientIp;
                clientIp.setHostnameAndPort((*usersIter)->getUser().getConnectionAddr());
                matchJoinEvent->getHeaderCore().setClientIp(clientIp.getHostname());
                matchJoinEvent->setClientType(ClientTypeToPinString((*usersIter)->getUser().getClientType()));

                // friend info (GG members, or target player for join by player/invite)
                if (gameSession != nullptr)
                {
                    RelatedPlayerSetsMap::const_iterator relatedPlayersSetIter = relatedPlayerLists.find((*usersIter)->getUser().getSessionId());
                    if (relatedPlayersSetIter != relatedPlayerLists.end())
                    {
                        if (!relatedPlayersSetIter->second.empty())
                        {
                            PlayerIdSet::const_iterator relatedPlayersIter = relatedPlayersSetIter->second.begin();
                            PlayerIdSet::const_iterator relatedPlayersEnd = relatedPlayersSetIter->second.end();
                            for (; relatedPlayersIter != relatedPlayersEnd; ++relatedPlayersIter)
                            {
                                BlazeId blazeId = (*relatedPlayersIter);
                                const PlayerInfoMaster* friendPtr = gameSession->getPlayerRoster()->getPlayer(blazeId);
                                if (friendPtr != nullptr)
                                {
                                    Blaze::RiverPoster::FriendInfo* friendInfo = matchJoinEvent->getFriendsList().allocate_element();
                                    eastl::string blazeIdString;
                                    blazeIdString.sprintf("%" PRIu64, blazeId);
                                    friendInfo->setPlayerId(blazeIdString.c_str());
                                    friendInfo->setPlayerIdType(RiverPoster::PIN_PLAYER_ID_TYPE_PERSONA);
                                    friendInfo->setPlatform(ClientPlatformTypeToString(friendPtr->getPlatformInfo().getClientPlatform()));
                                    if (joinRequest.getJoinRequest().getUser().getBlazeId() == INVALID_BLAZE_ID)
                                    {
                                        // we call users that are part of your mm session your 'group'
                                        friendInfo->setInGroup(true);
                                    }
                                    matchJoinEvent->getFriendsList().push_back(friendInfo);
                                }
                            }
                        }
                    }
                }

                populatePlayerSpecificJoinInfo((*usersIter)->getUser(), *matchJoinEvent);
            }
        }

        void PINEventHelper::buildMatchJoinFailedEvent(const GameManager::JoinGameMasterRequest &joinRequest, BlazeRpcError joinError, const GameSessionMasterPtr gameSession,
                                                        const RelatedPlayerSetsMap &relatedPlayerLists, PINSubmission &pinSubmission)
        {
            RiverPoster::PINEventList *newPinEventList = getUserSessionPINEventList(pinSubmission,  joinRequest.getUserJoinInfo().getUser().getSessionId());
            EA_ASSERT_MSG(newPinEventList != nullptr, "pin event list cannot be null!");

            // first set up the match join event
            RiverPoster::MPMatchJoinEventPtr matchJoinEvent = createPinTdf<RiverPoster::MPMatchJoinEvent>("RiverPoster::MPMatchJoinEvent");
            if (matchJoinEvent == nullptr)
            {
                // something went wrong
                return;
            }

            // the game may not exist, but if it does, set up the match join info
            if (gameSession != nullptr)
            {
                buildMatchJoinForGameSession(*gameSession, *matchJoinEvent);
            }
            else
            {
                // we have to get the game id in, at least
                eastl::string gameId;
                gameId.sprintf("%" PRIu64, joinRequest.getJoinRequest().getGameId());
                matchJoinEvent->setGameId(gameId.c_str());
                matchJoinEvent->setBlazeGameType(getBlazeGameTypeAsString(joinRequest.getJoinRequest().getCommonGameData().getGameType()));
                matchJoinEvent->getHeaderCore().setEventName(RiverPoster::MP_MATCH_JOIN_EVENT_NAME);
            }

            // fill out the pertinent failure info
            // this will only be set up for direct joins
            matchJoinEvent->setJoinMethod(RiverPoster::MATCH_JOIN_METHOD_DIRECT);
            matchJoinEvent->setJoinStatus(RiverPoster::MATCH_JOIN_STATUS_ERROR);
            matchJoinEvent->setStatusCode(ErrorHelp::getErrorName(joinError));

            // set up user-specific stuff in the base event
            InetAddress clientIp;
            clientIp.setHostnameAndPort(joinRequest.getUserJoinInfo().getUser().getConnectionAddr());
            matchJoinEvent->getHeaderCore().setClientIp(clientIp.getHostname());
            matchJoinEvent->setClientType(ClientTypeToPinString(joinRequest.getUserJoinInfo().getUser().getClientType()));

            // friend info (GG members, or target player for join by player/invite)
            if (gameSession != nullptr)
            {
                RelatedPlayerSetsMap::const_iterator relatedPlayersIter = relatedPlayerLists.find(joinRequest.getUserJoinInfo().getUser().getSessionId());
                if (relatedPlayersIter != relatedPlayerLists.end())
                {
                    PlayerIdSet::const_iterator usersIter = relatedPlayersIter->second.begin();
                    PlayerIdSet::const_iterator usersEnd = relatedPlayersIter->second.end();
                    for (; usersIter != usersEnd; ++usersIter)
                    {
                        BlazeId blazeId = (*usersIter);
                        const PlayerInfoMaster* friendPtr = gameSession->getPlayerRoster()->getPlayer(blazeId);
                        if (friendPtr != nullptr)
                        {
                            Blaze::RiverPoster::FriendInfo* friendInfo = matchJoinEvent->getFriendsList().allocate_element();
                            eastl::string blazeIdString;
                            blazeIdString.sprintf("%" PRIu64, blazeId);
                            friendInfo->setPlayerId(blazeIdString.c_str());
                            friendInfo->setPlayerIdType(RiverPoster::PIN_PLAYER_ID_TYPE_PERSONA);
                            friendInfo->setPlatform(ClientPlatformTypeToString(friendPtr->getPlatformInfo().getClientPlatform()));
                            bool inGroup = false;
                            if (joinRequest.getJoinRequest().getUser().getBlazeId() != INVALID_BLAZE_ID)
                            {
                                // trying to join a specific user
                                // we already added the user to the friend list
                                inGroup = false;
                            }
                            else
                            {
                                // we call users that are part of your mm session your 'group'
                                inGroup = true;
                            }
                            friendInfo->setInGroup(inGroup);
                            matchJoinEvent->getFriendsList().push_back(friendInfo);
                        }
                    }
                }
            }

            // Insert the player's QoS ping site latencies into the event
            populatePlayerSpecificJoinInfo(joinRequest.getUserJoinInfo().getUser(), *matchJoinEvent);

            // now insert the join event into the user's pin request
            newPinEventList->pull_back()->set(matchJoinEvent.get());
        }

        // NOTE: this method could be a pass-thru to buildMatchJoinForFailedUserSessionInfo(), but we'll need to build a friends list of the appropriate type as well as add some missing settings
        void PINEventHelper::buildMatchJoinForFailedMatchmakingSession(const Matchmaker::MatchmakingSession& mmSession, GameManager::MatchmakingResult result,
                                                                        const GameManager::UserSessionInfo& info, PINSubmission &pinSubmission)
        {
            // only need an event for the joining player
            RiverPoster::PINEventList *newPinEventList = getUserSessionPINEventList(pinSubmission, info.getSessionId());
            EA_ASSERT_MSG(newPinEventList != nullptr, "pin event list cannot be null!");

            // first set up the match join event
            RiverPoster::MPMatchJoinEventPtr matchJoinEvent = createPinTdf<RiverPoster::MPMatchJoinEvent>("RiverPoster::MPMatchJoinEvent");
            if (matchJoinEvent == nullptr)
            {
                // something went wrong
                return;
            }

            matchJoinEvent->getHeaderCore().setEventName(RiverPoster::MP_MATCH_JOIN_EVENT_NAME);
            matchJoinEvent->setBlazeGameType(getBlazeGameTypeAsString(mmSession.getCachedStartMatchmakingInternalRequestPtr()->getRequest().getCommonGameData().getGameType()));

            // set up user-specific stuff in the base event
            InetAddress clientIp;
            clientIp.setHostnameAndPort(info.getConnectionAddr());
            matchJoinEvent->getHeaderCore().setClientIp(clientIp.getHostname());
            matchJoinEvent->setClientType(ClientTypeToPinString(info.getClientType()));

            matchJoinEvent->setJoinMethod(RiverPoster::MATCH_JOIN_METHOD_MATCHMAKING);
            switch(result)
            {
            case SESSION_CANCELED:
                matchJoinEvent->setJoinStatus(RiverPoster::MATCH_JOIN_STATUS_CANCELED);
                break;
            case SESSION_TIMED_OUT:
                matchJoinEvent->setJoinStatus(RiverPoster::MATCH_JOIN_STATUS_TIMEOUT);
                break;
            default:
                matchJoinEvent->setJoinStatus(RiverPoster::MATCH_JOIN_STATUS_ERROR);
                matchJoinEvent->setStatusCode(MatchmakingResultToString(result));
                break;
            }

            StringBuilder sb;

            if (mmSession.getMMScenarioId() != INVALID_SCENARIO_ID)
            {
                matchJoinEvent->setScenarioName(mmSession.getScenarioInfo().getScenarioName());
                sb.reset() << mmSession.getScenarioInfo().getScenarioVersion();
                matchJoinEvent->setScenarioVersion(sb.get());
                matchJoinEvent->setScenarioVariant(mmSession.getScenarioInfo().getScenarioVariant());
                // set tracking tag only if populated from reco service
                if (!mmSession.getTrackingTag().empty())
                {
                    matchJoinEvent->setTrackingTag(mmSession.getTrackingTag().c_str());
                }

                for (auto& scenIt : mmSession.getScenarioInfo().getScenarioAttributes())
                {
                    EA::TDF::GenericTdfType* gen = scenIt.second.get();
                    const EA::TDF::TdfGenericValue& val = gen->get();
                    if (val.getType() != EA::TDF::TDF_ACTUAL_TYPE_STRING)
                    {
                        sb.reset();
                        EA::TDF::PrintEncoder printEncoder;
                        EA::TDF::TdfGenericReferenceConst ref(val);          // Get a reference to the value the Generic is pointing to (not the Generic itself)
                        printEncoder.print(sb, ref);

                        matchJoinEvent->getScenarioParams()[scenIt.first].set(sb.get(), sb.length());
                    }
                    else
                    {
                        // Just add the string without running it through PrintEncoder as the encoder
                        // will wrap the value in quotes
                        matchJoinEvent->getScenarioParams()[scenIt.first] = val.asString();
                    }
                }
            }
            else
            {
                ERR_LOG("[buildMatchJoinForFailedMatchmakingSession] No Scenario Id provided.  Check for possible side-effects from old MMing removal.")
            }


            // set up friends
            Matchmaker::MatchmakingSession::MemberInfoList::const_iterator groupMemberIter = mmSession.getMemberInfoList().begin();
            Matchmaker::MatchmakingSession::MemberInfoList::const_iterator groupMemberEnd = mmSession.getMemberInfoList().end();
            for ( ; groupMemberIter != groupMemberEnd; ++groupMemberIter )
            {
                const Matchmaker::MatchmakingSession::MemberInfo& memberInfo = static_cast<const Matchmaker::MatchmakingSession::MemberInfo &>(*groupMemberIter);
                BlazeId blazeId = memberInfo.mUserSessionInfo.getUserInfo().getId();
                if (blazeId != INVALID_BLAZE_ID && blazeId != info.getUserInfo().getId()) // not himself
                {
                    eastl::string blazeIdString;
                    blazeIdString.sprintf("%" PRIu64, blazeId);
                    Blaze::RiverPoster::FriendInfo* friendInfo = matchJoinEvent->getFriendsList().allocate_element();
                    friendInfo->setPlayerId(blazeIdString.c_str());
                    friendInfo->setPlayerIdType(RiverPoster::PIN_PLAYER_ID_TYPE_PERSONA);
                    friendInfo->setPlatform(ClientPlatformTypeToString(memberInfo.mUserSessionInfo.getUserInfo().getPlatformInfo().getClientPlatform()));
                    friendInfo->setInGroup(true); // we call users that are part of your mm session your 'group'
                    matchJoinEvent->getFriendsList().push_back(friendInfo);
                }
            }

            matchJoinEvent->setMatchmakingDurationSeconds(mmSession.getMatchmakingRuntime().getSec());
            matchJoinEvent->setEstimatedTimeToMatchSeconds(mmSession.getEstimatedTimeToMatch().getSec());
            matchJoinEvent->setMatchmakingTimeoutDuration(mmSession.getTotalSessionDurationSeconds());
            if ((result == SESSION_QOS_VALIDATION_FAILED) || (result == SESSION_ERROR_GAME_SETUP_FAILED))
            {
                // we likely have a valid fit score for one of these events
                matchJoinEvent->setMatchmakingFitScore(mmSession.getFitScoreForMatchmakedGame());
            }
            else
            {
                matchJoinEvent->setMatchmakingFitScore(0);
            }
            matchJoinEvent->setMatchmakingMaxFitScore(mmSession.getMaxFitScoreForMatchmakedGame());
            matchJoinEvent->setTotalUsersOnline(mmSession.getTotalGameplayUsersAtSessionStart());
            matchJoinEvent->setTotalUsersInGame(mmSession.getTotalUsersInGameAtSessionStart());
            matchJoinEvent->setTotalUsersInMatchmaking(mmSession.getTotalUsersInMatchmakingAtSessionStart());
            matchJoinEvent->setTotalUsersMatched(mmSession.getCurrentMatchingSessionListPlayerCount());
            matchJoinEvent->setTotalUsersPotentiallyMatched(mmSession.getTotalMatchingSessionListPlayerCount());

            BlazeId initiatorId = mmSession.getPrimaryUserBlazeId();
            if (initiatorId != INVALID_BLAZE_ID)
            {
                sb.reset() << initiatorId;
                matchJoinEvent->getInitiatorId()[RiverPoster::PIN_PLAYER_ID_TYPE_PERSONA].set(sb.get(), sb.length());
            }

            // Insert the player's QoS ping site latencies into the event
            populatePlayerSpecificJoinInfo(info, *matchJoinEvent);

            newPinEventList->pull_back()->set(matchJoinEvent.get());
        }

        void PINEventHelper::buildMatchJoinForFailedUserSessionInfo(const GameManager::StartMatchmakingInternalRequest& internalRequest, const GameManager::UserJoinInfo& initiatorJoinInfo,
            const GameManager::UserJoinInfoList friendsList, const TimeValue& duration, GameManager::MatchmakingResult result, const GameManager::UserSessionInfo& info, PINSubmission& pinSubmission)
        {
            // only need an event for the joining player
            RiverPoster::PINEventList *newPinEventList = getUserSessionPINEventList(pinSubmission, info.getSessionId());
            EA_ASSERT_MSG(newPinEventList != nullptr, "pin event list cannot be null!");

            // first set up the match join event
            RiverPoster::MPMatchJoinEventPtr matchJoinEvent = createPinTdf<RiverPoster::MPMatchJoinEvent>("RiverPoster::MPMatchJoinEvent");
            if (matchJoinEvent == nullptr)
            {
                // something went wrong
                return;
            }

            matchJoinEvent->getHeaderCore().setEventName(RiverPoster::MP_MATCH_JOIN_EVENT_NAME);
            matchJoinEvent->setBlazeGameType(getBlazeGameTypeAsString(internalRequest.getRequest().getCommonGameData().getGameType()));

            // set up user-specific stuff in the base event
            InetAddress clientIp;
            clientIp.setHostnameAndPort(info.getConnectionAddr());
            matchJoinEvent->getHeaderCore().setClientIp(clientIp.getHostname());
            matchJoinEvent->setClientType(ClientTypeToPinString(info.getClientType()));

            matchJoinEvent->setJoinMethod(RiverPoster::MATCH_JOIN_METHOD_MATCHMAKING);
            switch (result)
            {
            case SESSION_CANCELED:
                matchJoinEvent->setJoinStatus(RiverPoster::MATCH_JOIN_STATUS_CANCELED);
                break;
            case SESSION_TIMED_OUT:
                matchJoinEvent->setJoinStatus(RiverPoster::MATCH_JOIN_STATUS_TIMEOUT);
                break;
            default:
                matchJoinEvent->setJoinStatus(RiverPoster::MATCH_JOIN_STATUS_ERROR);
                matchJoinEvent->setStatusCode(MatchmakingResultToString(result));
                break;
            }

            StringBuilder sb;

            if (initiatorJoinInfo.getOriginatingScenarioId() != INVALID_SCENARIO_ID)
            {
                matchJoinEvent->setScenarioName(initiatorJoinInfo.getScenarioInfo().getScenarioName());
                sb.reset() << initiatorJoinInfo.getScenarioInfo().getScenarioVersion();
                matchJoinEvent->setScenarioVersion(sb.get());
                matchJoinEvent->setScenarioVariant(initiatorJoinInfo.getScenarioInfo().getScenarioVariant());
                // set tracking tag only if populated from reco service
                if (internalRequest.getTrackingTag()[0] != '\0')
                {
                    matchJoinEvent->setTrackingTag(internalRequest.getTrackingTag());
                }

                for (auto& scenAttr : initiatorJoinInfo.getScenarioInfo().getScenarioAttributes())
                {
                    EA::TDF::GenericTdfType* gen = scenAttr.second.get();
                    const EA::TDF::TdfGenericValue& val = gen->get();
                    if (val.getType() != EA::TDF::TDF_ACTUAL_TYPE_STRING)
                    {
                        sb.reset();
                        EA::TDF::PrintEncoder printEncoder;
                        EA::TDF::TdfGenericReferenceConst ref(val);          // Get a reference to the value the Generic is pointing to (not the Generic itself)
                        printEncoder.print(sb, ref);

                        matchJoinEvent->getScenarioParams()[scenAttr.first].set(sb.get(), sb.length());
                    }
                    else
                    {
                        // Just add the string without running it through PrintEncoder as the encoder
                        // will wrap the value in quotes
                        matchJoinEvent->getScenarioParams()[scenAttr.first] = val.asString();
                    }
                }
            }
            else
            {
                ERR_LOG("[buildMatchJoinForFailedUserSessionInfo] No Scenario Id provided.  Check for possible side-effects from old MMing removal.")
            }

            // set up friends
            for (auto& userJoinInfo : friendsList)
            {
                BlazeId blazeId = userJoinInfo->getUser().getUserInfo().getId();
                if (blazeId != info.getUserInfo().getId()) // not himself
                {
                    Blaze::RiverPoster::FriendInfo* friendInfo = matchJoinEvent->getFriendsList().allocate_element();
                    eastl::string blazeIdString;
                    blazeIdString.sprintf("%" PRIu64, blazeId);
                    friendInfo->setPlayerId(blazeIdString.c_str());
                    friendInfo->setPlayerIdType(RiverPoster::PIN_PLAYER_ID_TYPE_PERSONA);
                    friendInfo->setPlatform(ClientPlatformTypeToString(userJoinInfo->getUser().getUserInfo().getPlatformInfo().getClientPlatform()));
                    friendInfo->setInGroup(true); // we call users that are part of your mm session your 'group'
                    matchJoinEvent->getFriendsList().push_back(friendInfo);
                }
            }

            // PACKER_TODO: for mp_match_join, how to obtain other values (est. time to match, fit score, max fit score, "overall" timeout, etc.)
            matchJoinEvent->setMatchmakingDurationSeconds(duration.getSec());
            //matchJoinEvent->setEstimatedTimeToMatchSeconds(mmSession.getEstimatedTimeToMatch().getSec());
            matchJoinEvent->setMatchmakingTimeoutDuration(internalRequest.getScenarioTimeoutDuration().getSec());
            //if ((result == SESSION_QOS_VALIDATION_FAILED) || (result == SESSION_ERROR_GAME_SETUP_FAILED))
            //{
            //    // we likely have a valid fit score for one of these events
            //    matchJoinEvent->setMatchmakingFitScore(mmSession.getFitScoreForMatchmakedGame());
            //}
            //else
            //{
            //    matchJoinEvent->setMatchmakingFitScore(0);
            //}
            //matchJoinEvent->setMatchmakingMaxFitScore(mmSession.getMaxFitScoreForMatchmakedGame());
            matchJoinEvent->setTotalUsersOnline(internalRequest.getTotalUsersOnline());
            matchJoinEvent->setTotalUsersInGame(internalRequest.getTotalUsersInGame());
            matchJoinEvent->setTotalUsersInMatchmaking(internalRequest.getTotalUsersInMatchmaking());

            BlazeId initiatorId = initiatorJoinInfo.getUser().getUserInfo().getId();
            if (initiatorId != INVALID_BLAZE_ID)
            {
                sb.reset() << initiatorId;
                matchJoinEvent->getInitiatorId()[RiverPoster::PIN_PLAYER_ID_TYPE_PERSONA].set(sb.get(), sb.length());
            }

            // Insert the player's QoS ping site latencies into the event
            populatePlayerSpecificJoinInfo(info, *matchJoinEvent);

            newPinEventList->pull_back()->set(matchJoinEvent.get());
        }

        void PINEventHelper::buildMatchJoinForGameSession(const GameSessionMaster &gameSession, RiverPoster::MPMatchJoinEvent &matchJoinEvent)
        {
            matchJoinEvent.getHeaderCore().setEventName(RiverPoster::MP_MATCH_JOIN_EVENT_NAME);
            matchJoinEvent.getHeaderCore().setGameMode(gameSession.getGameMode());
            matchJoinEvent.getHeaderCore().setModeType(gameSession.getPINGameModeType());
            matchJoinEvent.getHeaderCore().setGameType(gameSession.getPINGameType());
            if (*gameSession.getPINGameMap() != '\0')
                matchJoinEvent.getHeaderCore().setMap(gameSession.getPINGameMap());

            eastl::string gameReportingId;
            gameReportingId.sprintf("%" PRIu64, gameSession.getGameReportingId());
            matchJoinEvent.setMatchId(gameReportingId.c_str());

            eastl::string gameId;
            gameId.sprintf("%" PRIu64, gameSession.getGameId());
            matchJoinEvent.setGameId(gameId.c_str());
            matchJoinEvent.setBlazeGameType(getBlazeGameTypeAsString(gameSession.getGameType()));
            matchJoinEvent.setNetworkTopology(GameNetworkTopologyToString(gameSession.getGameNetworkTopology()));
            if (gameSession.getGameType() == GAME_TYPE_GAMESESSION)
            {
                if (gameSession.isGamePreMatch())
                {
                    matchJoinEvent.setGamePhase(GameStateToString(PRE_GAME));
                }
                else
                {
                    if (gameSession.isGameInProgress())
                    {
                        matchJoinEvent.setGamePhase(GameStateToString(IN_GAME));
                    }
                    else if (gameSession.isGamePostMatch())
                    {
                        matchJoinEvent.setGamePhase(GameStateToString(POST_GAME));
                    }
                }
            }

            if (gameSession.hasPersistedGameId())
                matchJoinEvent.setServerId(gameSession.getPersistedGameId());
        }

        RiverPoster::PINEventList* PINEventHelper::getUserSessionPINEventList(PINSubmission &pinSubmission, UserSessionId sessionId)
        {
            auto& eventListMap = pinSubmission.getEventsMap();
            auto& pinEventList = eventListMap[sessionId];
            if (pinEventList == nullptr)
            {
                pinEventList = eventListMap.allocate_element();
            }
            return pinEventList;
        }


        void PINEventHelper::gameEnd(PlayerId playerId, GameId gameId, const GameInfo& gameInfo, const EA::TDF::Tdf& report, PINSubmission& pinSubmission, bool includeCustom /*=true*/)
        {
            EA::TDF::JsonEncoder encoder;
            EA::TDF::MemberVisitOptions visitOpt;
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
            visitOpt.onlyIfSet = true;
#endif

            encoder.setIncludeTDFMetaData(true);

            EA::TDF::JsonEncoder::FormatOptions formatOptions;

            formatOptions.options[EA::Json::JsonWriter::kFormatOptionIndentSpacing] = 0;
            formatOptions.options[EA::Json::JsonWriter::kFormatOptionLineEnd] = 0;
            encoder.setFormatOptions(formatOptions);

            // look up the user info the game info
            const GameInfo::PlayerInfoMap& playerInfoMap = gameInfo.getPlayerInfoMap();
            auto playerItr = playerInfoMap.find(playerId);
            if (playerItr == playerInfoMap.end())
            {
                // gr code should validate
                return;
            }
            GamePlayerInfo& playerInfo = *playerItr->second;

            // only need an event for the departing player
            RiverPoster::GameEndEventPtr gameEndEvent = createPinTdf<RiverPoster::GameEndEvent>("RiverPoster::GameEndEvent");
            if (gameEndEvent == nullptr)
            {
                // something went wrong
                return;
            }
            RiverPoster::PINEventList *newPinEventList = getUserSessionPINEventList(pinSubmission, playerInfo.getUserSessionId());
            EA_ASSERT_MSG(newPinEventList != nullptr, "pin event list cannot be null!");
            gameEndEvent->getHeaderCore().setEventName(RiverPoster::GAME_END_EVENT_NAME);

            eastl::string midString;
            midString.sprintf("%" PRIu64, gameInfo.getGameReportingId());
            gameEndEvent->getGameAttributes()["mid"] = midString.c_str();

            eastl::string goidString;
            goidString.sprintf("%" PRIu64, gameId);
            gameEndEvent->getGameAttributes()["goid"] = goidString.c_str();

            gameEndEvent->setGameDuration(gameInfo.getGameDurationMs() / 1000);

            if (includeCustom)
            {
                RawBuffer gameReportJson(2048);
                RawBufferIStream istream(gameReportJson);
                encoder.encode(istream, report, nullptr, visitOpt);
                gameEndEvent->getCustom().getGameReport().setAttributes(reinterpret_cast<char8_t*>(gameReportJson.data()));
            }
            
            // parameter is required by PIN, but we may not have the proper context here.
            switch (playerInfo.getRemovedReason())
            {
            case GAME_ENDED:
                gameEndEvent->setEndReason("game_end");
                break;
            case PLAYER_LEFT:
            case PLAYER_LEFT_MAKE_RESERVATION:
            case PLAYER_LEFT_CANCELLED_MATCHMAKING:
            case PLAYER_LEFT_SWITCHED_GAME_SESSION:
            case GROUP_LEFT:
            case GROUP_LEFT_MAKE_RESERVATION:
                gameEndEvent->setEndReason("leave");
                break;
            case PLAYER_KICKED:
            case PLAYER_KICKED_WITH_BAN:
                gameEndEvent->setEndReason("kicked");
                break;
            case PLAYER_RESERVATION_TIMEOUT:
            case HOST_EJECTED:
            case GAME_DESTROYED:
            case RESERVATION_TRANSFER_TO_NEW_USER:
            case DISCONNECT_RESERVATION_TIMEOUT:
            default:
                gameEndEvent->setEndReason("unknown");
                break;
            }

            // now insert the event into the user's pin request
            newPinEventList->pull_back()->set(gameEndEvent.get());
        }

        const char8_t* PINEventHelper::getBlazeGameTypeAsString(GameType gameType)
        {
            switch (gameType)
            {
            case GAME_TYPE_GROUP:
                return RiverPoster::PIN_BLAZE_GAME_TYPE_GROUP;
            case GAME_TYPE_GAMESESSION:
                return RiverPoster::PIN_BLAZE_GAME_TYPE_GAMESESSION;
            default:
                return "unknown";
            }
        }

        void PINEventHelper::addScenarioAttributesToMatchJoinEvent(RiverPoster::PINEventList& pinEventList, const ScenarioAttributes& scenarioAttributes)
        {
            for (auto& pinEvent : pinEventList)
            {
                EA::TDF::VariableTdfBase* t = pinEvent.get();
                if (t->get()->getTdfId() != RiverPoster::MPMatchJoinEvent::TDF_ID)
                    continue;

                // Convert the scenario attributes into a PIN-friendly format (map of string/string pairs)
                RiverPoster::MPMatchJoinEvent* joinEvent = static_cast<RiverPoster::MPMatchJoinEvent*>(t->get());

                for (auto& scenAttr : scenarioAttributes)
                {
                    EA::TDF::GenericTdfType* gen = scenAttr.second.get();
                    const EA::TDF::TdfGenericValue& val = gen->get();
                    if (val.getType() != EA::TDF::TDF_ACTUAL_TYPE_STRING)
                    {
                        StringBuilder sb;
                        EA::TDF::PrintEncoder printEncoder;
                        EA::TDF::TdfGenericReferenceConst ref(val); // Get a reference to the value the Generic is pointing to (not the Generic itself)
                        printEncoder.print(sb, ref);

                        joinEvent->getScenarioParams()[scenAttr.first].set(sb.get(), sb.length());
                    }
                    else
                    {
                        // Just add the string without running it through PrintEncoder as the encoder will wrap the value in quotes
                        joinEvent->getScenarioParams()[scenAttr.first] = val.asString();
                    }
                }

                break;
            }
        }
    } // namespace GameManager
} // namespace Blaze
