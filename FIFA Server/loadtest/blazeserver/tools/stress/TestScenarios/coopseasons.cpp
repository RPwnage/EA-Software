
//  *************************************************************************************************
//
//   File:    CoopSeasons.cpp
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#include "coopseasons.h"
#include "playermodule.h"

#include "utility.h"
#include "reference.h"
#include "framework/util/shared/base64.h"
#include "gamemanager/tdf/matchmaker.h"
#include "framework/tdf/entrycriteria.h"
#include "osdksettings/tdf/osdksettingstypes.h"
#include "coopseason/tdf/coopseasontypes.h"


namespace Blaze {
	namespace Stress {

		CoopSeasons::CoopSeasons(StressPlayerInfo* playerData)
			: GamePlayUser(playerData),
			CoopSeasonsGameSession(playerData, 4 /*gamesize*/, CLIENT_SERVER_DEDICATED /*Topology*/,
				StressConfigInst.getGameProtocolVersionString() /*GameProtocolVersionString*/),
			mIsMarkedForLeaving(false), mGameName(""), mCoopId("")
		{
		}

		CoopSeasons::~CoopSeasons()
		{
			BLAZE_INFO(BlazeRpcLog::util, "[CoopSeasons][Destructor][%" PRIu64 "]: CoopSeasons destroy called", mPlayerInfo->getPlayerId());
			//Remove friend info from the map
			removeFriendFromMap();
			//stop telemetry calls
			stopReportTelemtery();
			//  deregister async handlers to avoid client crashes when disconnect happens
			deregisterHandlers();
			//  wait for some time to allow already scheduled jobs to complete
			sleep(30000);
		}

		void CoopSeasons::deregisterHandlers()
		{
			CoopSeasonsGameSession::deregisterHandlers();
		}

		BlazeRpcError CoopSeasons::coopSeasonsLeaderboardCalls()
		{
			BlazeRpcError result = ERR_OK;
			FilteredLeaderboardStatsRequest::EntityIdList filterBoardEntityList;

			result = getLeaderboardTreeAsync(mPlayerInfo, "CoopSeasonalPlay");
			result = getLeaderboardGroup(mPlayerInfo, -1, "coop_seasonalplay_overall");
			result = getCenteredLeaderboard(mPlayerInfo, 0, 1, mPlayerInfo->getPlayerId(), "coop_seasonalplay_overall");
			filterBoardEntityList.push_back(mPlayerInfo->getPlayerId());
			result = getFilteredLeaderboard(mPlayerInfo, filterBoardEntityList, "coop_seasonalplay_overall", 4294967295);
			result = getLeaderboard(mPlayerInfo, 0, 100, "coop_seasonalplay_overall");
			sleep(5000);

			if ((int)Random::getRandomNumber(100) < 3)
			{
				Blaze::PersonaNameList personaNameList;
				//lookupUsers
				for (int i = 0; i < 5; i++)
				{
					personaNameList.push_back(StressConfigInst.getRandomPersonaNames());
				}
				lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
			}
			return result;
		}

		BlazeRpcError CoopSeasons::simulateLoad()
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::simulateLoad]:" << mPlayerInfo->getPlayerId());
			BlazeRpcError err = ERR_OK;

			err = simulateMenuFlow();

			if ((int)Random::getRandomNumber(100) < 100)
			{
				coopSeasonsLeaderboardCalls();
			}

			//check if a friend is available
			checkIfFriendExist();

			mFriendBlazeId = getFriendInMap(mPlayerInfo->getPlayerId());
			if (mFriendBlazeId == 0)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "CoopSeasons:: get Friend failed. " << mPlayerInfo->getPlayerId());
				sleep(60000);
				return err;
			}

			/*mCoopId = 0;
			if (isGrupPlatformHost)
			{
				err = setCoopIdData(mPlayerInfo->getPlayerId(), mFriendBlazeId, mCoopId);
				if (err != ERR_OK)
				{
					mCoopId = 0;
					err = getCoopIdData(mPlayerInfo->getPlayerId(), mFriendBlazeId, mCoopId);
					if (err != ERR_OK || mCoopId == 0)
					{
						BLAZE_ERR_LOG(BlazeRpcLog::util, "CoopSeasons::getCoopIdData: failed with error " << ErrorHelp::getErrorName(err) << " " << mPlayerInfo->getPlayerId());
						removeFriendFromMap();
						sleep(60000);
						return err;
					}
				}
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::simulateLoad]: " << mPlayerInfo->getPlayerId() << " mCoopId= " << mCoopId);
			}*/
			//set game group name
			//persistedGameId = "COOP_1000200563164_1000160774083"
			if (mPlayerInfo->getPlayerId() > mFriendBlazeId)
			{
				mGameName = eastl::string().sprintf("COOP_""%" PRIu64 "_""%" PRIu64 "", mPlayerInfo->getPlayerId(), mFriendBlazeId).c_str();
			}
			else
			{
				mGameName = eastl::string().sprintf("COOP_""%" PRIu64 "_""%" PRIu64 "", mFriendBlazeId, mPlayerInfo->getPlayerId()).c_str();
			}
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::simulateLoad]: " << mPlayerInfo->getPlayerId() << " mGameName= " << mGameName.c_str());

			lobbyRpcs();

			err = CreateOrJoinGame();
			if (err != ERR_OK)
			{
				//BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "CoopSeasons:: CreateOrJoinGame  failed. Error(" << ErrorHelp::getErrorName(err) << ")" << mPlayerInfo->getPlayerId());
				removeFriendFromMap();
				sleep(60000);
				return err;
			}
			sleep(10000);
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::simulateLoad]: finished createorjoingame" << mPlayerInfo->getPlayerId() << " ,mGameGroupId= " << mGameGroupId);

			// PlayerTimeStats RPC call
			if (StressConfigInst.getPTSEnabled())
			{
				// Update Eadp Stats(Read and Update Limits)
				if ((uint32_t)Random::getRandomNumber(100) < StressConfigInst.getEadpStatsProbability())
				{
					updateEadpStats(mPlayerInfo);
				}
			}

			//wait for 5min for friend to join Game Group
			if (isGrupPlatformHost)
			{
				BlazeId coopId = 0;
				setCoopIdData(mPlayerData->getPlayerId(), mFriendBlazeId, coopId);
				setCoopIdData(mPlayerData->getPlayerId(), mFriendBlazeId, coopId);
				uint16_t count = 0;
				while (count <= 10)
				{
					//for (eastl::set<BlazeId>::iterator citPlayerIt = mPlayerInGameMap.begin(), citPlayerItEnd = mPlayerInGameMap.end();
					//	citPlayerIt != citPlayerItEnd; ++citPlayerIt)
					//{
					//	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::simulateLoad]mPlayerInGameMap.PlayerId " << *citPlayerIt << " mGameGroupId=" << mGameGroupId);
					//}
					if (mPlayerInGameMap.size() >= 2)
					{
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::simulateLoad]  mPlayerInGameMap.size reached 2 for mGameGroupId=" << mGameGroupId);
						break;
					}
					count++;
					sleep(30000); // sleep for 30sec
				} // while

				if (mPlayerInGameMap.size() < 2)
				{
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "CoopSeasons:: mPlayerInGameMap.size not reached full " << mPlayerInfo->getPlayerId());
					removeFriendFromMap();
					leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGameGroupId);
					sleep(60000);
					return err;
				}

				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::simulateLoad]: coopStartMatchmakingScenario::isGrupPlatformHost " << mPlayerInfo->getPlayerId());
				//Send MM request	
				sleep(30000); // sleep for 30sec
				err = coopStartMatchmakingScenario();
				sleep(30000); // sleep for 30sec
				if (err != ERR_OK)
				{
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "CoopSeasons:: simulateLoad :: StartMatchmaking  failed. Error(" << ErrorHelp::getErrorName(err) << ")" << mPlayerInfo->getPlayerId());
					removeFriendFromMap();
					leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGameGroupId);
					sleep(60000);
					return err;
				}

				//CancelMatchmaking
				/*if (Random::getRandomNumber(100) < (int)CO_CONFIG.cancelMatchMakingProbability)
				{
					err = cancelMatchMakingScenario(mMatchmakingSessionId);
					if(err == ERR_OK)
					{
						removeFriendFromMap();
						leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGameGroupId);
						sleep(10000);
						return err;
					}
				}*/
			}
			else
			{
				sleep(60000);
			}
			int64_t playerStateTimer = 0;
			int64_t  playerStateDurationMs = 0;
			GameState   playerStateTracker = getPlayerState();
			uint16_t stateLimit = 0;
			bool ingame_enter = false;
			while (1)
			{
				if (!mPlayerData->isConnected())
				{
					BLAZE_ERR_LOG(BlazeRpcLog::util, "[CoopSeasons::simulateLoad::User Disconnected = " << mPlayerData->getPlayerId());
					return ERR_DISCONNECTED;
				}
				if (playerStateTracker != getPlayerState())  //  state changed
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::simulateLoad:playerStateTracker]" << mPlayerData->getPlayerId() << " Time Spent in Sate: " << GameStateToString(
						playerStateTracker) << " Duration: " << playerStateDurationMs << "  now moved to State:" << GameStateToString(getPlayerState()));
					playerStateTracker = getPlayerState();
					playerStateDurationMs = 0;
					playerStateTimer = 0;
				}
				else
				{
					if (!playerStateTimer) {
						playerStateTimer = TimeValue::getTimeOfDay().getMillis();
					}
					else
					{
						playerStateDurationMs = TimeValue::getTimeOfDay().getMillis() - playerStateTimer;
						if (playerStateDurationMs > CO_CONFIG.maxPlayerStateDurationMs)
						{
							BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::simulateLoad:playerStateTracker][" << mPlayerData->getPlayerId() << "]: Player exceded maximum Game state Duration:" << playerStateDurationMs <<
								" ms" << " Current Game State:" << GameStateToString(getPlayerState()));
							if (IN_GAME == getPlayerState())
							{
								mIsMarkedForLeaving = true;   //  Player exceed maximum duration in InGAME State so mark this Player to Leave from InGame
							}
						}
					}
				}
				const char8_t * stateMsg = GameStateToString(getPlayerState());
				BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::simulateLoad]: Player: " << mPlayerInfo->getPlayerId() << ", GameId: " << myGameId << " Current state : " << stateMsg << " InStateDuration:" <<
					playerStateDurationMs);
				switch (getPlayerState())
				{
					//   Sleep for all the below states
				case NEW_STATE:
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=NEW_STATE");
					stateLimit++;

					if (stateLimit >= CO_CONFIG.GameStateIterationsMax)
					{
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=NEW_STATE for long time and return");
						unsubscribeFromCensusData(mPlayerData);
						removeFriendFromMap();
						leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGameGroupId);
						sleep(30000);
						return err;
					}
					//  Wait here if resetDedicatedserver() called/player not found for invite.
					sleep(CO_CONFIG.GameStateSleepInterval);
					break;
				}
				case PRE_GAME:
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=PRE_GAME");
					//updateGameSession(getGameId());
					setClientState(mPlayerInfo, ClientState::MODE_MULTI_PLAYER);

					if (isPlatformHost)
					{
						Blaze::GameManager::SetGameSettingsRequest request;
						request.setGameId(myGameId);
						request.getGameSettings().setBits(0); //3104
						request.getGameSettingsMask().setBits(2063);
						err = mPlayerData->getComponentProxy()->mGameManagerProxy->setGameSettings(request);
						//setGameSettings(3232);
						if(isGrupPlatformHost)
							advanceGameState(IN_GAME);
					}
					sleep(CO_CONFIG.GameStateSleepInterval);
					break;
				}
				case IN_GAME:
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=IN_GAME");

					//BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::simulateLoad]" << mPlayerData->getPlayerId() << "IN_GAME Current Player count for game ID "<< getGameId()<<" "<< mPlayerInGameMap.size()<<" As follows :");
					//for (eastl::set<BlazeId>::iterator citPlayerIt = mPlayerInGameMap.begin(), citPlayerItEnd = mPlayerInGameMap.end();
					//	citPlayerIt != citPlayerItEnd; ++citPlayerIt) 
					//{
					//	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::simulateLoad]   PlayerId " << *citPlayerIt);
					//}
					if (isPlatformHost)
					{
						BlazeId coopId = 0;
						setCoopIdData(mPlayerData->getPlayerId(), mFriendBlazeId, coopId);
					}
					if (!ingame_enter)
					{
						ingame_enter = true;
						simulateInGame();
						//Start InGame Timer	
						if (isPlatformHost)
						{
							BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " setInGameStartTime. gameID=" << myGameId);
							setInGameStartTime();
						}
					}

					if (RollDice(CO_CONFIG.inGameReportTelemetryProbability))
					{
						executeReportTelemetry();
					}

					if (((isPlatformHost) && IsInGameTimedOut()) || (mIsMarkedForLeaving == true))
					{
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " IsInGameTimedOut/mIsMarkedForLeaving=true. gameID=" << myGameId);
						advanceGameState(POST_GAME);
					}

					sleep(CO_CONFIG.GameStateSleepInterval);
					break;
				}
				case INITIALIZING:
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=INITIALIZING");
					//if (isPlatformHost )
					//{
					//	
					//	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::simulateLoad][" << mPlayerData->getPlayerId() << "calling advanceGameState");
					//	//Move this to DS -TODO
					//	advanceGameState(PRE_GAME);
					//}
					sleep(CO_CONFIG.GameStateSleepInterval);
					break;
				}
				case POST_GAME:
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=POST_GAME");
					setPlayerAttributes("GAME_END", "1", getGameId());
					//leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, getGameId());

					setClientState(mPlayerInfo, Blaze::ClientState::MODE_SINGLE_PLAYER, Blaze::ClientState::STATUS_NORMAL);

					getStatGroup(mPlayerInfo, "CoopSeasonalPlay_StatGroup");
					Blaze::Stats::GetStatsByGroupRequest::EntityIdList coopIds;
					coopSeasonGetAllCoopIds(coopIds);
					getStatsByGroupAsync(mPlayerInfo, coopIds, "CoopSeasonalPlay_StatGroup");
					FetchConfigResponse response;
					fetchClientConfig(mPlayerInfo, "FIFA_COOP_SEASONALPLAY", response);
					//postGameCalls();

					//leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGameGroupId);
					//Remove friend info from the map
					removeFriendFromMap();

					setPlayerState(DESTRUCTING);

					sleep(CO_CONFIG.GameStateSleepInterval);
					break;
				}
				case DESTRUCTING:
				{
					//  Player Left Game
					//  Presently using this state when user is removed from the game and return from this function.
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=DESTRUCTING");
					setClientState(mPlayerInfo, ClientState::MODE_MENU);
					if (mCensusdataSubscriptionStatus)
					{
						err = unsubscribeFromCensusData(mPlayerInfo);
						if (err == ERR_OK)
						{
							UnsetSubscriptionSatatus();
						}
					}

					sleep(5000);
					//  Return here
					return err;
				}
				case MIGRATING:
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=MIGRATING");
					sleep(10000);
					break;
				}
				case GAME_GROUP_INITIALIZED:
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=GAME_GROUP_INITIALIZED");
					sleep(5000);
					break;
				}
				case INACTIVE_VIRTUAL:
				default:
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=" << getPlayerState());
					return err;
				}
				}
			}
		}

		void CoopSeasons::lobbyRpcs()
		{
			BlazeRpcError err;
			GetMembersResponse getMemberResponse;
			uint32_t viewId = mPlayerInfo->getViewId();

			// Read the clubID from RPC response getClubMembershipForUsers
			BlazeRpcError result = ERR_OK;
			Blaze::Clubs::GetClubMembershipForUsersResponse clubMembershipForUsersResponse;
			result = getClubMembershipForUsers(mPlayerInfo, clubMembershipForUsersResponse);

			Blaze::Clubs::ClubMemberships* memberShips = clubMembershipForUsersResponse.getMembershipMap()[mPlayerInfo->getPlayerId()];
			if (memberShips == NULL)
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "CoopsPlayer::createOrJoinClubs:getClubsComponentSettings() Response returned empty object " << mPlayerInfo->getPlayerId());
			}
			else
			{
				Blaze::Clubs::ClubMembershipList::iterator it = memberShips->getClubMembershipList().begin();
				setMyClubId((*it)->getClubId());
				//Player has already subsribed to a club - return here
				if (getMyClubId() != 0)
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "CoopsPlayer::simulateLoad:player= " << mPlayerData->getPlayerId() << " received clubID from getClubMembershipForUsers. " << getMyClubId());
				}
			}
			mPlayerInfo->setViewId(++viewId);
			Blaze::BlazeIdList friendList;
			friendList.push_back(mPlayerInfo->getPlayerId());
			friendList.push_back(mFriendBlazeId);

			if (PLATFORM_PS4 == StressConfigInst.getPlatform() || PLATFORM_XONE == StressConfigInst.getPlatform())
			{
				eastl::vector<eastl::string> stringIds;
				stringIds.push_back("TXT_CONTACT_INFO_TEAM_1799");
				stringIds.push_back("TXT_CONTACT_INFO_LEAGUE_13");
				stringIds.push_back("TXT_CONTACT_INFO_FIFA_GAME");
				stringIds.push_back("TeamName_1799");
				stringIds.push_back("LeagueName_13");
				stringIds.push_back("FEDERATIONINTERNATIONALEDEFOOTBALLASSOCIATION");
				stringIds.push_back("TXT_CONTACT_INFO_UEFA");
				stringIds.push_back("TXT_UEFA");
				localizeStrings(mPlayerInfo, 1701729619, stringIds);
				getInvitations(mPlayerInfo, getMyClubId(), Blaze::Clubs::InvitationsType::CLUBS_SENT_TO_ME);
				getStatGroup(mPlayerInfo, "VProPosition");
				getStatsByGroupAsync(mPlayerInfo, friendList, "VProPosition", getMyClubId());
				setConnectionState(mPlayerInfo);
				setConnectionState(mPlayerInfo);
				lookupUsersByPersonaNames(mPlayerInfo, "");
			}

			getStatGroup(mPlayerInfo, "H2HPreviousSeasonalPlay");
			getStatsByGroupAsync(mPlayerInfo, "H2HPreviousSeasonalPlay");
			mPlayerInfo->setViewId(++viewId);
			lookupUsersByPersonaNames(mPlayerInfo, "");
			sleep(5000);
			getStatGroup(mPlayerInfo, "H2HSeasonalPlay");
			getStatsByGroupAsync(mPlayerInfo, "H2HSeasonalPlay");
			mPlayerInfo->setViewId(++viewId);

			FetchConfigResponse response;
			fetchClientConfig(mPlayerInfo, "FIFA_H2H_SEASONALPLAY", response);

			FifaCups::CupInfo cupsResp;
			err = getCupInfo(mPlayerInfo, FifaCups::MemberType::FIFACUPS_MEMBERTYPE_USER, cupsResp);
			Blaze::Stats::GetStatsByGroupRequest::EntityIdList coopIds;
			coopSeasonGetAllCoopIds(coopIds);
			fetchClientConfig(mPlayerInfo, "FIFA_COOP_SEASONALPLAY", response);
			if (ERR_OK == err)
			{
				//check what is the cupID when player is not in the cup
				setCupId(cupsResp.getCupId());
			}
			getCupInfo(mPlayerInfo, FifaCups::MemberType::FIFACUPS_MEMBERTYPE_COOP, cupsResp);
			lookupUsersByPersonaNames(mPlayerInfo, "");
			lookupUsersByPersonaNames(mPlayerInfo, "");
			lookupUsersByPersonaNames(mPlayerInfo, "");
			lookupUsersByPersonaNames(mPlayerInfo, "");
			if (getMyClubId() > 0)
			{
				getMembers(mPlayerInfo, getMyClubId(), 50, getMemberResponse);
				if (!isPlatformHost)
				{
					getPetitions(mPlayerInfo, getMyClubId(), CLUBS_PETITION_SENT_TO_CLUB);
				}
			}
			getStatsByGroupAsync(mPlayerInfo, friendList, "VProPosition", getMyClubId());
			mPlayerInfo->setViewId(++viewId);
			setConnectionState(mPlayerInfo);
			lookupUsersByPersonaNames(mPlayerInfo, "");
			getStatsByGroupAsync(mPlayerInfo, friendList, "H2HPreviousSeasonalPlay", getMyClubId());
			mPlayerInfo->setViewId(++viewId);
			lookupUsersByPersonaNames(mPlayerInfo, "");
			getStatsByGroupAsync(mPlayerInfo, friendList, "VProPosition", getMyClubId());
			mPlayerInfo->setViewId(++viewId);
			fetchClientConfig(mPlayerInfo, "FIFA_H2H_SEASONALPLAY", response);
			if (coopIds.size() > 0)
			{
				getCupInfo(mPlayerInfo, FifaCups::MemberType::FIFACUPS_MEMBERTYPE_USER, cupsResp);
				fetchClientConfig(mPlayerInfo, "FIFA_COOP_SEASONALPLAY", response);
				getCupInfo(mPlayerInfo, FifaCups::MemberType::FIFACUPS_MEMBERTYPE_COOP, cupsResp);
				uint64_t x = atoi(mCoopId.c_str());
				getCoopIdData(0, 0, x);
				getCupInfo(mPlayerInfo, FifaCups::MemberType::FIFACUPS_MEMBERTYPE_COOP, cupsResp);
			}
			getGameReportViewInfo(mPlayerInfo, "CoopSeasonRecentGames");
			getGameReportView(mPlayerInfo, "CoopSeasonRecentGames", mCoopId.c_str());
			eastl::string playerIdString = eastl::string().sprintf("%" PRIu64 "", mPlayerInfo->getPlayerId()).c_str();
			getGameReportViewInfo(mPlayerInfo, "NormalRecentGames");
			getGameReportView(mPlayerInfo, "NormalRecentGames", playerIdString.c_str());
			userSettingsSave(mPlayerInfo, "");
			//getCupInfo(mPlayerInfo, FifaCups::MemberType::FIFACUPS_MEMBERTYPE_COOP, cupsResp);

			OSDKTournaments::GetMyTournamentIdResponse getTournamentIdResponse;
			OSDKTournaments::GetTournamentsResponse getTournamentsResponse;
			OSDKTournaments::GetMyTournamentDetailsResponse getMyTournamentDetailsResponse;
			OSDKTournaments::TournamentId tournamentId = 0;

			FifaCups::CupInfo getCupsResponse;
			err = getCupInfo(mPlayerInfo, FifaCups::FIFACUPS_MEMBERTYPE_USER, getCupsResponse);
			if (err == ERR_OK)
			{
				setCupId(getCupsResponse.getCupId());
			}
			err = getMyTournamentId(mPlayerInfo, OSDKTournaments::TournamentModes::MODE_FIFA_CUPS_USER, getTournamentIdResponse,0);
			if (err == ERR_OK)
			{
				getTournaments(mPlayerInfo, OSDKTournaments::TournamentModes::MODE_FIFA_CUPS_USER, getTournamentsResponse);
			}
			else if (err == OSDKTOURN_ERR_MEMBER_NOT_IN_TOURNAMENT)
			{
				err = getTournaments(mPlayerInfo, OSDKTournaments::TournamentModes::MODE_FIFA_CUPS_USER, getTournamentsResponse);
				if (err == ERR_OK && getTournamentsResponse.getTournamentList().size() > 0)
				{
					// Means that the player is not yet into any tournaments, so get the tournament id first and join into that;
					tournamentId = getTournamentsResponse.getTournamentList().at(0)->getTournamentId();
					// Join Tournament	
					err = joinTournament(mPlayerInfo, tournamentId);
				}
				setTournamentId(tournamentId);
			}
			if (err == ERR_OK)
			{
				setTournamentId(tournamentId);
				if (!getTournamentIdResponse.getIsActive())
				{
					//if the player is not in the tournament means that he might have lost/won the last tournament, so resetting here..
					osdkResetTournament(mPlayerInfo, getTournamentId(), OSDKTournaments::TournamentModes::MODE_FIFA_CUPS_USER);
				}
				OSDKTournaments::GetMyTournamentDetailsResponse tournamentDetailsResponse;
				getMyTournamentDetails(mPlayerInfo, OSDKTournaments::TournamentModes::MODE_FIFA_CUPS_USER, getTournamentId(), tournamentDetailsResponse,0);
			}
			else
			{
				BLAZE_ERR_LOG(BlazeRpcLog::util, "[CoopsPlayer::tournamentRpcCalls::TournamentId is Not Set for " << mPlayerData->getPlayerId());
			}
			
			if (coopIds.size() > 0) {
				getStatGroup(mPlayerInfo, "CoopSeasonalPlay_StatGroup");
				getStatsByGroupAsync(mPlayerInfo, coopIds, "CoopSeasonalPlay_StatGroup");
				mPlayerInfo->setViewId(++viewId);
				//mCoopId = eastl::string().sprintf("%" PRIu64 "", coopIds.at(0)).c_str();
			}
			//mGameName = "COOP_" + mCoopId;
			//Blaze::BlazeIdList friendsList;
			//friendsList.push_back(mPlayerInfo->getPlayerId());
			//getStatsByGroupAsync(mPlayerInfo, friendsList, "VProPosition");
			//mPlayerInfo->setViewId(++viewId);

			getStatsByGroupAsync(mPlayerInfo, coopIds, "CoopSeasonalPlay_StatGroup");
			mPlayerInfo->setViewId(++viewId);
			//eastl::string coopIDName = eastl::string().sprintf("%" PRIu64 "", mCoopId).c_str();
			sleep(5000);
		}

		void CoopSeasons::checkIfFriendExist()
		{
			BlazeId friendIDValue = 0;
			int count = 1;

			if ((((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->getCoopFriendsList())->size() == 0 /*|| Random::getRandomNumber(100) < 50*/)
			{
				//add self to friends list and wait for the friend using map [50%]
				addFriendToList(mPlayerInfo->getPlayerId());
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "CoopSeasons::checkIfFriendExist addFriendToList " << mPlayerInfo->getPlayerId());

				//wait for 5min for a friend
				while (count <= 10 && friendIDValue == 0)
				{
					friendIDValue = getFriendInMap(mPlayerInfo->getPlayerId());
					count++;
					sleep(30000); // sleep for 30sec

				}
			}
			else
			{
				//find a random friend using map [50%]
				friendIDValue = getARandomFriend();
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "CoopSeasons::checkIfFriendExist getARandomFriend " << mPlayerInfo->getPlayerId());

				//wait for 5min for a friend
				while (count <= 10 && friendIDValue == 0)
				{
					friendIDValue = getARandomFriend();
					count++;
					sleep(30000); // sleep for 30sec

				}
			}

			if (friendIDValue != 0)
			{
				//Add friends set to map
				addCoopFriendsToMap(mPlayerInfo->getPlayerId(), friendIDValue);
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "CoopSeasons::checkIfFriendExist successful Set " << mPlayerInfo->getPlayerId() << " - " << friendIDValue);
			}
		}


		void CoopSeasons::addPlayerToList()
		{
			//  add player to universal list first
			Player::addPlayerToList();
			BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::addPlayerToList]: Player: " << mPlayerInfo->getPlayerId() << ", Game: " << myGameId);
			{
				PlayerId  id = mPlayerInfo->getPlayerId();
				PlayerIdList& playerIds = ((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->coopPlayerList;
				playerIds.push_back(id);
			}
		}

		void CoopSeasons::removePlayerFromList()
		{
			//  remove player from universal list first
			Player::removePlayerFromList();
			PlayerId  id = mPlayerInfo->getPlayerId();
			BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::removePlayerFromList]: Player: " << id << ", Game: " << myGameId);
			PlayerIdList& playerIds = ((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->coopPlayerList;
			PlayerIdList::iterator start = playerIds.begin();
			PlayerIdList::iterator end = playerIds.end();
			for (; start != end; start++)
			{
				if (id == *start) {
					break;
				}
			}
			if (start != end)
			{
				playerIds.erase(start);
			}
		}

		void CoopSeasons::simulateInGame()
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][CoopSeasons]: In Game simulation:" << myGameId);

			//setPlayerAttributes ("COOP_RBS", "0");
			setClientState(mPlayerInfo, ClientState::MODE_MULTI_PLAYER, ClientState::Status::STATUS_SUSPENDED);
			if (mCensusdataSubscriptionStatus)
			{
				BlazeRpcError err;
				err = unsubscribeFromCensusData(mPlayerInfo);
				if (err == ERR_OK)
				{
					UnsetSubscriptionSatatus();
				}
			}

			delayRandomTime(2000, 5000);
		}

		void CoopSeasons::postGameCalls()
		{
			setClientState(mPlayerInfo, Blaze::ClientState::MODE_SINGLE_PLAYER, Blaze::ClientState::STATUS_NORMAL);
			FifaCups::CupInfo cupsResp;
			OSDKTournaments::GetMyTournamentIdResponse getTournamentIdResponse;
			OSDKTournaments::GetTournamentsResponse tournmentsResponse;
			OSDKTournaments::GetMyTournamentDetailsResponse tournamentDetailsResponse;
			//OSDKTournaments::TournamentId tournamentId = 0;
			BlazeRpcError err = getCupInfo(mPlayerInfo, FifaCups::MemberType::FIFACUPS_MEMBERTYPE_COOP, cupsResp);
			if (ERR_OK == err)
			{
				//check what is the cupID when player is not in the cup
				setCupId(cupsResp.getCupId());
			}
			OSDKTournaments::GetMyTournamentIdResponse tournamentIdResponse;
			err = getMyTournamentId(mPlayerInfo, OSDKTournaments::TournamentModes::MODE_FIFA_CUPS_USER, tournamentIdResponse);
			if (err == ERR_OK)
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::initializeCupAndTournments]:" << mPlayerData->getPlayerId() << " getMyTournamentId call 2 passed ");
				//tournamentId = tournmentsResponse.getTournamentList().at(0)->getTournamentId();
				getTournaments(mPlayerInfo, OSDKTournaments::TournamentModes::MODE_FIFA_CUPS_USER, tournmentsResponse);

			}
			else if (err == OSDKTOURN_ERR_MEMBER_NOT_IN_TOURNAMENT)
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::initializeCupAndTournments]:" << mPlayerData->getPlayerId() << " getMyTournamentId call 2 faled but calling getTournaments");
				err = getTournaments(mPlayerInfo, OSDKTournaments::TournamentModes::MODE_FIFA_CUPS_USER, tournmentsResponse);
				if (err == ERR_OK && tournmentsResponse.getTournamentList().size() > 0)
				{
					// Means that the player is not yet into any tournaments, so get the tournament id first and join into that;
					//tournamentId = tournmentsResponse.getTournamentList().at(0)->getTournamentId();
				}
			}
			if (err == ERR_OK)
			{
				//setTournamentId(tournamentId);
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "OnlineSeasons::tournamentRpcCalls:player= " << mPlayerData->getPlayerId() << " TournamentId is set. " << getTournamentId());
			}
			else
			{
				BLAZE_ERR_LOG(BlazeRpcLog::util, "[OnlineSeasons::tournamentRpcCalls::TournamentId is Not Set for " << mPlayerData->getPlayerId());
			}
			//OSDKTournaments::GetTournamentsResponse tournmentsResponse;
			//getTournaments(mPlayerInfo, OSDKTournaments::TournamentModes::MODE_FIFA_CUPS_USER, tournmentsResponse);
			//OSDKTournaments::GetMyTournamentDetailsResponse tournamentDetailsResponse;
			getMyTournamentDetails(mPlayerInfo, OSDKTournaments::TournamentModes::MODE_FIFA_CUPS_USER, getTournamentId(), tournamentDetailsResponse);
			sleep(5000);
			lookupUsersByPersonaNames(mPlayerInfo, mPlayerInfo->getPersonaName().c_str());
			updateExtendedDataAttribute(0);
			getStatGroup(mPlayerInfo, "CoopSeasonalPlay_StatGroup");
			getKeyScopesMap(mPlayerInfo);
		}


		bool CoopSeasons::executeReportTelemetry()
		{
			BlazeRpcError err = ERR_OK;
			if (!mPlayerData->isConnected())
			{
				BLAZE_ERR_LOG(BlazeRpcLog::util, "[CoopSeasons::ReportTelemetry::User Disconnected = " << mPlayerData->getPlayerId());
				return false;
			}
			if (getGameId() == 0)
			{
				BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::ReportTelemetry][GameId for player = " << mPlayerInfo->getPlayerId() << "] not present");
				return false;
			}
			StressGameInfo* ptrGameInfo = CoopplayerGameDataRef(mPlayerInfo).getGameData(getGameId());
			if (ptrGameInfo == NULL)
			{
				BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[CoopSeasons::ReportTelemetry][GameData for game = " << getGameId() << "] not present");
				return false;
			}
			TelemetryReportRequest reportTelemetryReq;
			reportTelemetryReq.setGameId(getGameId());
			reportTelemetryReq.setNetworkTopology(getGameNetworkTopology());
			reportTelemetryReq.setLocalConnGroupId(mPlayerInfo->getConnGroupId().id);
			TelemetryReport* telemetryReport = reportTelemetryReq.getTelemetryReports().pull_back();
			telemetryReport->setLatencyMs(Random::getRandomNumber(1));
			int totalPackets = Random::getRandomNumber(20000);
			telemetryReport->setLocalPacketsReceived(totalPackets);
			telemetryReport->setRemotePacketsSent(totalPackets);
			telemetryReport->setPacketLossPercent(0);
			telemetryReport->setRemoteConnGroupId(ptrGameInfo->getTopologyHostConnectionGroupId());
			//telemetryReport->setRemoteConnGroupId(getOpponentPlayerInfo().getConnGroupId().id);
			err = mPlayerInfo->getComponentProxy()->mGameManagerProxy->reportTelemetry(reportTelemetryReq);
			if (err != ERR_OK)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "CoopSeasons: reportTelemetry failed with err=" << ErrorHelp::getErrorName(err));
			}
			return true;
		}

		BlazeRpcError CoopSeasons::CreateOrJoinGame()
		{
			Blaze::GameManager::CreateGameRequest request;
			Blaze::GameManager::JoinGameResponse response;
			BlazeRpcError err = ERR_OK;

			if (!mPlayerData->isConnected())
			{
				BLAZE_ERR_LOG(BlazeRpcLog::util, "[CoopSeasons::CreateOrJoinGame::User Disconnected = " << mPlayerData->getPlayerId());
				return ERR_DISCONNECTED;
			}

			CommonGameRequestData& commonData = request.getCommonGameData();
			NetworkAddress& hostAddress = commonData.getPlayerNetworkAddress();
			//NetworkAddress *hostAddress = &request.getCommonGameData().getPlayerNetworkAddress();
			if (PLATFORM_XONE == StressConfigInst.getPlatform())
			{
				hostAddress.switchActiveMember(NetworkAddress::MEMBER_XBOXCLIENTADDRESS);
				if (mPlayerData->getExternalId())
					hostAddress.getXboxClientAddress()->setXuid(mPlayerData->getExternalId());
			}
			else
			{
				hostAddress.switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
				hostAddress.getIpPairAddress()->getExternalAddress().setIp(mPlayerInfo->getConnection()->getAddress().getIp());
				hostAddress.getIpPairAddress()->getExternalAddress().setPort(mPlayerInfo->getConnection()->getAddress().getPort());
				hostAddress.getIpPairAddress()->getExternalAddress().copyInto(hostAddress.getIpPairAddress()->getInternalAddress());
			}

			TournamentSessionData & tournamentSessionData = request.getTournamentSessionData();
			tournamentSessionData.setArbitrationTimeout(31536000000000);
			tournamentSessionData.setForfeitTimeout(31449600000000);
			tournamentSessionData.setTournamentDefinition("");
			tournamentSessionData.setScheduledStartTime("1970-01-01T00:00:00Z");

			commonData.setGameType(GAME_TYPE_GROUP);
			commonData.setGameProtocolVersionString(getGameProtocolVersionString());
			Collections::AttributeMap& gameAttribs = request.getGameCreationData().getGameAttribs();
			request.setGamePingSiteAlias("");
			request.setGameEventAddress("");
			request.setGameEndEventUri("");
			gameAttribs["Gamemode_Name"] = "COOP_MATCH";
			gameAttribs["OSDK_gameMode"] = "COOP_MATCH";

			GameCreationData & gameCreationData = request.getGameCreationData();
			gameCreationData.setDisconnectReservationTimeout(0);
			gameCreationData.setGameModRegister(0);
			eastl::string gameName = "Co-op Seasons";
			/*gameName += eastl::to_string(mPlayerInfo->getPlayerId());
			gameName += "_";
			gameName += eastl::to_string(mFriendBlazeId);*/
			gameCreationData.setGameName(gameName.c_str());// gameName.c_str());
			gameCreationData.setPermissionSystemName("");

			//request.getCommonGameData().
			request.getPlayerJoinData().setGameEntryType(GAME_ENTRY_TYPE_DIRECT);  // As per the new TTY;
			request.getPlayerJoinData().setGroupId(mPlayerData->getConnGroupId());

			request.setPersistedGameId(mGameName.c_str());
			// GSET = 34823 (0x00008807)
			request.getGameCreationData().getGameSettings().setBits(0x00008817);
			request.getGameCreationData().setNetworkTopology(Blaze::NETWORK_DISABLED);
			request.getGameCreationData().setMaxPlayerCapacity(8);
			request.getGameCreationData().setMinPlayerCapacity(1);
			request.getGameCreationData().setPresenceMode(Blaze::PRESENCE_MODE_PRIVATE);
			request.getGameCreationData().setQueueCapacity(0);
			request.getGameCreationData().setVoipNetwork(Blaze::VOIP_PEER_TO_PEER);
			request.getGameCreationData().getTeamIds().push_back(65534);
			request.getGameCreationData().setExternalSessionTemplateName("");
			request.getGameCreationData().getExternalSessionIdentSetup().getXone().setTemplateName("COOP");
			request.getGameCreationData().setSkipInitializing(false);
			request.getGameCreationData().setGameReportName("");

			request.setGameEventContentType("application/json");
			request.setGameStartEventUri("");
			request.setGameStatusUrl("");  // As per the new TTY;
			request.setServerNotResetable(false);

			request.getSlotCapacitiesMap().clear();
			request.getSlotCapacities().at(SLOT_PUBLIC_PARTICIPANT) = 2;
			request.getSlotCapacities().at(SLOT_PRIVATE_PARTICIPANT) = 0;
			request.getSlotCapacities().at(SLOT_PUBLIC_SPECTATOR) = 0;
			request.getSlotCapacities().at(SLOT_PRIVATE_SPECTATOR) = 0;

			Blaze::GameManager::PerPlayerJoinData * data = request.getPlayerJoinData().getPlayerDataList().pull_back();
			data->getUser().setBlazeId(mPlayerData->getPlayerId());
			if (PLATFORM_PS4 == StressConfigInst.getPlatform())
			{
				data->getUser().setPersonaNamespace("ps3");
			}
			else if (PLATFORM_XONE == StressConfigInst.getPlatform())
			{
				data->getUser().setPersonaNamespace("xbox");
			}
			else
			{
				data->getUser().setName(mPlayerData->getPersonaName().c_str());
			}
			request.getPlayerJoinData().setDefaultSlotType(Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT);
			request.getPlayerJoinData().setDefaultTeamIndex(0);
			request.getPlayerJoinData().setDefaultTeamId(65534);

			request.getCommonGameData().setGameProtocolVersionString(getGameProtocolVersionString());
			TournamentIdentification & tournamentIdentification = request.getTournamentIdentification();
			tournamentIdentification.setTournamentId("");
			tournamentIdentification.setTournamentOrganizer("");

			char8_t buf[20046];
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "-> [CoopSeasonsGameSession][" << mPlayerInfo->getPlayerId() << "]::CreateOrJoinGame" << "\n" << request.print(buf, sizeof(buf)));

			err = mPlayerInfo->getComponentProxy()->mGameManagerProxy->createOrJoinGame(request, response);
			if (err != ERR_OK)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "CoopSeasons::CreateOrJoinGame  failed. Error(" << ErrorHelp::getErrorName(err) << ")" << mPlayerInfo->getPlayerId());
				return err;
			}
			mGameGroupId = response.getGameId();
			myGameId = response.getGameId();
			return err;
		}

		BlazeRpcError CoopSeasons::coopSeasonGetAllCoopIds(Blaze::Stats::GetStatsByGroupRequest::EntityIdList& coopIds)
		{
			BlazeRpcError err = ERR_SYSTEM;
			Blaze::CoopSeason::GetAllCoopIdsResponse getAllCoopIdsResponse;
			err = getAllCoopIds(mPlayerData, getAllCoopIdsResponse);

			//char8_t buf[10240];
			//BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "-> [CoopSeasonsGameSession]" << ":: coopSeasonGetAllCoopIds . Also check for friendBlazeId : "<< mFriendBlazeId << "\n" << getAllCoopIdsResponse.print(buf, sizeof(buf)));
			CoopSeason::CoopPairDataList& pairDataList = getAllCoopIdsResponse.getCoopPairDataList();
			if (pairDataList.size() == 0 && getAllCoopIdsResponse.getResultCount() && isGrupPlatformHost)
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "-> [CoopSeasonsGameSession] :: Calling setCoopIdData ");
				BlazeId CoopId;
				setCoopIdData(mPlayerInfo->getPlayerId(), mFriendBlazeId, CoopId);
				coopIds.push_back(CoopId);
			}
			else
			{
				int32_t iter = 0;
				while (iter < getAllCoopIdsResponse.getResultCount())
				{
					coopIds.push_back(pairDataList[iter]->getCoopId());
					if (getAllCoopIdsResponse.getResultCount() == 1)
					{
						mCoopId = eastl::to_string(pairDataList[iter]->getCoopId());
						break;
					}
					if((pairDataList[iter]->getMemberTwoBlazeId() == mFriendBlazeId && pairDataList[iter]->getMemberOneBlazeId() == mPlayerInfo->getPlayerId())|| (pairDataList[iter]->getMemberOneBlazeId() == mFriendBlazeId && pairDataList[iter]->getMemberTwoBlazeId() == mPlayerInfo->getPlayerId()))
						mCoopId = eastl::to_string(pairDataList[iter]->getCoopId());
					iter++;
				}
			}
			return err;
		}

		BlazeRpcError CoopSeasons::setCoopIdData(BlazeId BlazeID1, BlazeId BlazeID2, BlazeId &coopId)
		{
			BlazeRpcError err = ERR_SYSTEM;  // ERR_SYSTEM represents General system error.
			Blaze::CoopSeason::SetCoopIdDataRequest request;
			Blaze::CoopSeason::SetCoopIdDataResponse response;
			request.setCoopId(coopId);
			request.setMetadata("0:0:0:0:0:100");
			request.setMemberOneBlazeId(BlazeID1);
			request.setMemberTwoBlazeId(BlazeID2);

			err = mPlayerInfo->getComponentProxy()->mCoopSeasonProxy->setCoopIdData(request, response);
			coopId = response.getCoopId();
			if (err != ERR_OK) {
				BLAZE_ERR_LOG(BlazeRpcLog::util, "CoopSeasons::setCoopIdData: failed with error " << ErrorHelp::getErrorName(err) << " " << mPlayerInfo->getPlayerId());
				coopId = 0;
			}
			else {
				coopId = response.getCoopId();
				BLAZE_TRACE_LOG(BlazeRpcLog::util, "CoopSeasons::setCoopIdData:coopId=" << coopId << " for players " << BlazeID1 << " " << BlazeID2);
			}
			return err;
		}

		BlazeRpcError CoopSeasons::coopStartMatchmakingScenario()
		{
			BlazeRpcError err = ERR_OK;
			StartMatchmakingScenarioRequest request;
			StartMatchmakingScenarioResponse response;
			MatchmakingCriteriaError criteriaError;

			request.getCommonGameData().setGameProtocolVersionString(getGameProtocolVersionString());
			request.getCommonGameData().setGameType(GAME_TYPE_GAMESESSION);
			NetworkAddress& hostAddress = request.getCommonGameData().getPlayerNetworkAddress();
			hostAddress.switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
			if (!mPlayerData->isConnected())
			{
				BLAZE_ERR_LOG(BlazeRpcLog::util, "[CoopSeasons::coopStartMatchmaking::User Disconnected = " << mPlayerData->getPlayerId());
				return ERR_SYSTEM;
			}
			if (PLATFORM_XONE == StressConfigInst.getPlatform())
			{
				hostAddress.switchActiveMember(NetworkAddress::MEMBER_XBOXCLIENTADDRESS);
				if (mPlayerData->getExternalId())
					hostAddress.getXboxClientAddress()->setXuid(mPlayerData->getExternalId());
			}
			else
			{
				hostAddress.switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
				hostAddress.getIpPairAddress()->getExternalAddress().setIp(mPlayerInfo->getConnection()->getAddress().getIp());
				hostAddress.getIpPairAddress()->getExternalAddress().setPort(mPlayerInfo->getConnection()->getAddress().getPort());
				hostAddress.getIpPairAddress()->getExternalAddress().copyInto(hostAddress.getIpPairAddress()->getInternalAddress());
			}
			PlayerJoinData& playerJoinData = request.getPlayerJoinData();
			//Set Game Group ID 
			playerJoinData.setGroupId(UserGroupId(4, 2, mGameGroupId));

			playerJoinData.setDefaultRole("");
			playerJoinData.setGameEntryType(GameEntryType::GAME_ENTRY_TYPE_DIRECT);
			playerJoinData.setDefaultSlotType(SLOT_PUBLIC_PARTICIPANT);
			playerJoinData.setDefaultTeamId(ANY_TEAM_ID);
			playerJoinData.setDefaultTeamIndex(ANY_TEAM_ID + 1);
			PerPlayerJoinDataList& playerDataList = playerJoinData.getPlayerDataList();
			PerPlayerJoinDataPtr playerJoinDataPtr = playerDataList.pull_back();
			if (playerJoinDataPtr != NULL)
			{
				playerJoinDataPtr->setEncryptedBlazeId("");
				playerJoinDataPtr->setIsOptionalPlayer(false);
				playerJoinDataPtr->setRole("");
				playerJoinDataPtr->setSlotType(INVALID_SLOT_TYPE);
				playerJoinDataPtr->setTeamId(65535);
				playerJoinDataPtr->setTeamIndex(65535);
				playerJoinDataPtr->getUser().setExternalId(mPlayerData->getExternalId());
				playerJoinDataPtr->getUser().setBlazeId(mPlayerData->getPlayerId());
				//playerJoinDataPtr->getUser().setName(mPlayerData->getPersonaName().c_str());
				playerJoinDataPtr->getUser().setName("");
				playerJoinDataPtr->getUser().setOriginPersonaId(0);
				playerJoinDataPtr->getUser().setPidId(0);
				//playerJoinDataPtr->getUser().getPlatformInfo().getEaIds().setNucleusAccountId(mPlayerData->getPlayerId());

			}
			
			if (StressConfigInst.getPlatform() == Platform::PLATFORM_PS5 || StressConfigInst.getPlatform() == Platform::PLATFORM_XBSX)
			{
				SET_SCENARIO_ATTRIBUTE(request, "ATTRIBDB_RULE", "1");
				SET_SCENARIO_ATTRIBUTE(request, "AVOID_TEAM", "1");
				SET_SCENARIO_ATTRIBUTE(request, "AVOID_TEAM_THR", "");
				SET_SCENARIO_ATTRIBUTE(request, "COOP_DIVISION", "1");
				SET_SCENARIO_ATTRIBUTE(request, "COOP_DIVISION_THR", "");
				SET_SCENARIO_ATTRIBUTE(request, "CUSTOM_CONTROLLER", "0");
				SET_SCENARIO_ATTRIBUTE(request, "GAMEGROUP_ONLY", "0");
				SET_SCENARIO_ATTRIBUTE(request, "GAME_MODE", "25");
			}
			else
			{
				SET_SCENARIO_ATTRIBUTE(request, "AVOID_TEAM", "10");
				SET_SCENARIO_ATTRIBUTE(request, "AVOID_TEAM_THR", "OSDK_matchExact");
				SET_SCENARIO_ATTRIBUTE(request, "COOP_DIVISION", "1");
				SET_SCENARIO_ATTRIBUTE(request, "COOP_DIVISION_THR", "OSDK_matchRelax");
				SET_SCENARIO_ATTRIBUTE(request, "CUSTOM_CONTROLLER", "0");
				SET_SCENARIO_ATTRIBUTE(request, "GAMEGROUP_ONLY", "0");
				SET_SCENARIO_ATTRIBUTE(request, "GAME_MODE", "25");
			}
			if (StressConfigInst.getPlatform() == PLATFORM_PS4)
			{
				SET_SCENARIO_ATTRIBUTE(request, "MATCHUP_HASH", "4340580");
			}
			else if(StressConfigInst.getPlatform() == PLATFORM_PS5)
			{
				SET_SCENARIO_ATTRIBUTE(request, "MATCHUP_HASH", "4506450");
			}
			SET_SCENARIO_ATTRIBUTE(request, "MATCH_CLUB_TYPE", "0");
			SET_SCENARIO_ATTRIBUTE(request, "MATCH_GUESTS", "1");
			SET_SCENARIO_ATTRIBUTE(request, "NET_TOP", (uint64_t)1);
			SET_SCENARIO_ATTRIBUTE(request, "NEW_USER_RULE", "1");
			SET_SCENARIO_ATTRIBUTE(request, "NEW_USER_RULE_THR", "");
			if (StressConfigInst.getPlatform() == PLATFORM_PS5)
			{
				SET_SCENARIO_ATTRIBUTE(request, "TEAM_OVR", "80");
				SET_SCENARIO_ATTRIBUTE(request, "TEAM_OVR_THR", "");
			}
			else
			{
				SET_SCENARIO_ATTRIBUTE(request, "TEAM_OVR", "83");
				SET_SCENARIO_ATTRIBUTE(request, "TEAM_OVR_THR", "OSDK_matchRelax");
			}
			
			SET_SCENARIO_ATTRIBUTE(request, "WOMEN_RULE", "0");

			request.setScenarioName("Coop_Seasons");

			//char8_t buf[20046];
			//BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "StartMatchmaking RPC : \n" << request.print(buf, sizeof(buf)));

			err = mPlayerData->getComponentProxy()->mGameManagerProxy->startMatchmakingScenario(request, response, criteriaError);
			if (err == ERR_OK)
			{
				mMatchmakingSessionId = response.getScenarioId();
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "CoopSeasons::startMatchmakingScenario successful " << mPlayerData->getPlayerId() << " , mMatchmakingSessionId= " << mMatchmakingSessionId);
			}
			else
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "CoopSeasons::startMatchmakingScenario failed. Error(" << ErrorHelp::getErrorName(err) << ")");
			}
			return err;
		}

	}  // namespace Stress
}  // namespace Blaze
