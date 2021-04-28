//  *************************************************************************************************
//
//   File:    ssfseasons.cpp
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#include "./ssfseasons.h"
#include "./playermodule.h"
#include "./gamesession.h"
#include "./utility.h"
#include "./reference.h"
#include "framework/util/shared/base64.h"
#include "gamemanager/tdf/matchmaker.h"
#include "framework/tdf/entrycriteria.h"
#include "osdksettings/tdf/osdksettingstypes.h"
#include "clubs/tdf/clubs.h"
//#include "mail/tdf/mail.h"
#include "shieldextension.h"

namespace Blaze {
	namespace Stress {

		SsfSeasons::SsfSeasons(StressPlayerInfo* playerData)
			: GamePlayUser(playerData),
			SsfSeasonsGameSession(playerData, 2 /*gamesize*/, PEER_TO_PEER_FULL_MESH /*Topology*/,
				StressConfigInst.getGameProtocolVersionString() /*GameProtocolVersionString*/),
			mIsMarkedForLeaving(false)
		{
			mEventId = 0;
			shieldExtension = BLAZE_NEW ShieldExtension();
		}

		SsfSeasons::~SsfSeasons()
		{
			BLAZE_INFO(BlazeRpcLog::util, "[SsfSeasons][Destructor][%" PRIu64 "]: SSF player destroy called", mPlayerInfo->getPlayerId());
			removeFriendFromMap();
			stopReportTelemtery();
			//  deregister async handlers to avoid client crashes when disconnect happens
			deregisterHandlers();
			if (shieldExtension)
				delete shieldExtension;
			//  wait for some time to allow already scheduled jobs to complete
			sleep(30000);
		}

		void SsfSeasons::deregisterHandlers()
		{
			SsfSeasonsGameSession::deregisterHandlers();
		}

		BlazeRpcError SsfSeasons::simulateLoad()
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::simulateLoad]:" << mPlayerData->getPlayerId());
			BlazeRpcError err = ERR_OK;
			mScenarioType = StressConfigInst.getRandomScenarioByPlayerType(SSFSEASONS);

			err = simulateMenuFlow();
			err = lobbyRPCcalls();
			//Choose one of the scenario type
			switch (mScenarioType)
			{
			case VOLTA_5v5:
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type " << mScenarioType);
			}
			break;
			case VOLTA_FUTSAL:
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type " << mScenarioType);
			}
			break;
			case VOLTA_DROPIN:
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type " << mScenarioType);
			}
			break;
			case VOLTA_SOLO:
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type " << mScenarioType);
			}
			break;
			case VOLTA_PWF:
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type " << mScenarioType);
			}
			break;
			default:
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " In default state");
				mScenarioType = VOLTA_DROPIN;
			}
			break;
			}
			if (StressConfigInst.isOnlineScenario(mScenarioType))
			{
				mPlayerInfo->setShieldExtension(shieldExtension);
				mPlayerInfo->getShieldExtension()->perfTune(mPlayerInfo, true);
			}
			sleepRandomTime(5000, 30000);

			if (mScenarioType == VOLTA_DROPIN || mScenarioType == VOLTA_PWF || mScenarioType == VOLTA_SOLO)
			{
				checkIfFriendExist();
				mFriendBlazeId = getFriendInMap(mPlayerInfo->getPlayerId());
				if (mFriendBlazeId == 0)
				{
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "CoopSeasons:: get Friend failed. " << mPlayerInfo->getPlayerId());
					sleep(60000);
					return err;
				}
			}
			//Send MM request 
			if (mScenarioType == VOLTA_DROPIN || mScenarioType == VOLTA_PWF || mScenarioType == VOLTA_SOLO)
			{
				if (mScenarioType == VOLTA_DROPIN)
				{
					err = startGroupMatchmakingScenario(mScenarioType);

					if (err != ERR_OK)
					{
						removeFriendFromMap();
						//removeFriendFromList();
						sleep(30000);
						return err;
					}
					sleep(30000);
					setGameSettingsMask(mPlayerInfo, mGameGroupId, 0, 2063);
				}
				if (mScenarioType == VOLTA_PWF || mScenarioType == VOLTA_SOLO)
				{
					err = createOrJoinGame();
					sleep(30000); // sleep for 30sec

					if (ERR_OK != err)
					{
						BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]: createOrJoinGame failed - Voip Group " << mPlayerData->getPlayerId());
						unsubscribeFromCensusData(mPlayerData);
						//leave home group
						//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::Calling leaveGameByGroup for gamegroupID] createOrJoinGame failed" << mPlayerData->getPlayerId() << " with GameId - " << mGameGroupId);
						//leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGameGroupId);
						removeFriendFromMap();
						sleep(60000);
						return err;
					}
				}
				for (eastl::set<BlazeId>::iterator citPlayerIt = mPlayerInGameMap.begin(), citPlayerItEnd = mPlayerInGameMap.end();
					citPlayerIt != citPlayerItEnd; ++citPlayerIt)
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopsPlayer::simulateLoad]mPlayerInGameMap for PlayerId = " << mPlayerInfo->getPlayerId() << " is" << *citPlayerIt << " mGameGroupId=" << mGameGroupId);
				}
				if (isGrupPlatformHost)
				{
					uint16_t count = 0;
					while (count <= 10)
					{
						//for (eastl::set<BlazeId>::iterator citPlayerIt = mPlayerInGameMap.begin(), citPlayerItEnd = mPlayerInGameMap.end();
						//	citPlayerIt != citPlayerItEnd; ++citPlayerIt)
						//{
						//	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopsPlayer::simulateLoad]mPlayerInGameMap.PlayerId " << *citPlayerIt << " mGameGroupId=" << mGameGroupId);
						//}
						if (mPlayerInGameGroupMap.size() >= 2)
						{
							BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopsPlayer::simulateLoad]  mPlayerInGameMap.size reached 2 for mGameGroupId=" << mGameGroupId);
							break;
						}
						count++;
						sleep(30000); // sleep for 30sec
					} // while

					if (mPlayerInGameGroupMap.size() < 2)
					{
						BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "CoopsPlayer:: mPlayerInGameMap.size not reached full " << mPlayerInfo->getPlayerId());
						removeFriendFromMap();
						BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::Calling leaveGameByGroup for gamegroupID] mPlayerInGameMap.size not reached full " << mPlayerData->getPlayerId() << " with GameId - " << mGameGroupId);
						if (mScenarioType == VOLTA_DROPIN || mScenarioType == VOLTA_PWF || mScenarioType == VOLTA_SOLO)
						{
							leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGameGroupId);
						}
						sleep(60000);
						return err;
					}
				}
				if (isGrupPlatformHost)
				{
					err = startMatchmakingScenario(mScenarioType);
					if (err != ERR_OK)
					{
						BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::Calling leaveGameByGroup for gamegroupID] startMatchMaking failed" << mPlayerData->getPlayerId() << mPlayerData->getPlayerId() << " with GameId - " << mGameGroupId);
						removeFriendFromMap();
						if (mScenarioType == VOLTA_DROPIN || mScenarioType == VOLTA_PWF || mScenarioType == VOLTA_SOLO)
						{
							leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGameGroupId);
						}
						BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "Failed to StartMatchmaking call");
						sleep(60000);
						return err;
					}
					else
					{
						if (mMatchmakingSessionId == 0)
						{
							return err;
						}
					}
				}
			}
			else
			{
				err = startMatchmakingScenario(mScenarioType);
				if (err != ERR_OK)
				{
					sleep(60000);
					return err;
				}
			}
			
			if (err != ERR_OK) { return err; }

			// PlayerTimeStats RPC call
			if (StressConfigInst.getPTSEnabled())
			{
				// Update Eadp Stats(Read and Update Limits)
				if ((uint32_t)Random::getRandomNumber(100) < StressConfigInst.getEadpStatsProbability())
				{
					updateEadpStats(mPlayerInfo);
				}
			}

			//CancelMatchmaking
			//if ( (mScenarioType != FUTFRIENDLIES) && (Random::getRandomNumber(100) < (int)SSFSEASONS_CONFIG.cancelMatchMakingProbability) )
			//{		
			//	err = cancelMatchMakingScenario(mMatchmakingSessionId);
			//	if(err == ERR_OK)
			//	{
			//		return err;
			//	}
			//}


			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SSF PLayer::simulateLoad]mPlayerInGameMap.size for PlayerId " << mPlayerData->getPlayerId() << " is " << mPlayerInGameMap.size());
			for (eastl::set<BlazeId>::iterator citPlayerIt = mPlayerInGameMap.begin(), citPlayerItEnd = mPlayerInGameMap.end();
				citPlayerIt != citPlayerItEnd; ++citPlayerIt)
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SSF PLayer::simulateLoad]mPlayerInGameMap for PlayerId " << mPlayerData->getPlayerId() << " is " << *citPlayerIt << " mGameGroupId=" << mGameGroupId << "mGameID : " << myGameId);
			}

			int64_t playerStateTimer = 0;
			int64_t  playerStateDurationMs = 0;
			GameState   playerStateTracker = getPlayerState();
			uint16_t stateLimit = 0;
			bool ingame_enter = false;
			if (StressConfigInst.isOnlineScenario(mScenarioType))
			{
				sleepRandomTime(70000, 90000);
				sleepRandomTime(70000, 90000);
			}
			while (1)
			{
				if (!mPlayerData->isConnected())
				{
					BLAZE_ERR_LOG(BlazeRpcLog::util, "[SsfSeasons::simulateLoad::User Disconnected = " << mPlayerData->getPlayerId());
					return ERR_DISCONNECTED;
				}
				if (playerStateTracker != getPlayerState())  //  state changed
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::simulateLoad:playerStateTracker]" << mPlayerData->getPlayerId() << " Time Spent in Sate: " << GameStateToString(
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
						if (playerStateDurationMs > SSFSEASONS_CONFIG.maxPlayerStateDurationMs)
						{
							BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::simulateLoad:playerStateTracker][" << mPlayerData->getPlayerId() << "]: Player exceded maximum Game state Duration:" << playerStateDurationMs <<
								" ms" << " Current Game State:" << GameStateToString(getPlayerState()));
							if (IN_GAME == getPlayerState())
							{
								mIsMarkedForLeaving = true;   //  Player exceed maximum duration in InGAME State so mark this Player to Leave from InGame
							}
						}
					}
				}
				const char8_t * stateMsg = GameStateToString(getPlayerState());
				BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::simulateLoad]: Player: " << mPlayerInfo->getPlayerId() << ", GameId: " << myGameId << " Current state : " << stateMsg << " InStateDuration:" <<
					playerStateDurationMs);

				switch (getPlayerState())
				{
					//   Sleep for all the below states
				case NEW_STATE:
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=NEW_STATE");
					stateLimit++;
					if (stateLimit >= SSFSEASONS_CONFIG.GameStateIterationsMax)
					{
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=NEW_STATE for long time and return");
						unsubscribeFromCensusData(mPlayerData);
						if (mScenarioType == VOLTA_DROPIN || mScenarioType == VOLTA_PWF || mScenarioType == VOLTA_SOLO)
						{
							removeFriendFromMap();
							BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::Calling leaveGameByGroup for gamegroupID] from NEW_STATE " << mPlayerData->getPlayerId() << mPlayerData->getPlayerId() << " with GameId - " << mGameGroupId);
							if (mScenarioType == VOLTA_DROPIN || mScenarioType == VOLTA_PWF || mScenarioType == VOLTA_SOLO)
							{
								leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGameGroupId);
							}
						}
						sleep(30000);
						return err;
					}
					//  Wait here if resetDedicatedserver() called/player not found for invite.
					sleep(SSFSEASONS_CONFIG.GameStateSleepInterval);
					break;
				}
				case PRE_GAME:
				{
					if (StressConfigInst.isOnlineScenario(mScenarioType))
					{
						sleep(FUT_CONFIG.GameStateSleepInterval * 2);
						mPlayerInfo->getShieldExtension()->perfTune(mPlayerInfo, false);
					}
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=PRE_GAME");

					if (isPlatformHost)
					{
						setPlayerAttributes("REQ", "2");
						setGameSettingsMask(mPlayerInfo, getGameId(), 0, 2063);

						if (mScenarioType == VOLTA_PWF || mScenarioType == VOLTA_SOLO)
						{
							char8_t opponentIdString[MAX_GAMENAME_LEN] = { '\0' };
							char8_t playerIdString[MAX_GAMENAME_LEN] = { '\0' };
							blaze_snzprintf(playerIdString, sizeof(playerIdString), "%" PRId64, mPlayerData->getPlayerId());
							blaze_snzprintf(opponentIdString, sizeof(opponentIdString), "%" PRId64, mFriendBlazeId);

							Blaze::Messaging::MessageAttrMap messageAttrMap;
							messageAttrMap.clear();

							eastl::string mMessageAttribute = "H_captain:" + eastl::string().sprintf(opponentIdString) + ",H_FIx_4:" + eastl::string().sprintf(opponentIdString) + ",H_FIx_1:" + eastl::string().sprintf(playerIdString) + ",H_SIx_0:5,H_SIx_1:1,H_SIx_2:4,H_SIx_3:2,H_SIx_4:0";

							messageAttrMap[0] = mMessageAttribute.c_str();
							sendMessage(mPlayerInfo, mGameGroupId, messageAttrMap, 1734571116, ENTITY_TYPE_GAME_GROUP);

							Collections::AttributeMap gameAttributes;
							gameAttributes["H_captain"] = playerIdString;	//mplayerid
							gameAttributes["H_FIx_1"] = opponentIdString;	//gamegroup-playerid
							gameAttributes["H_FIx_4"] = playerIdString;
							gameAttributes["H_SIx_0"] = "5";
							gameAttributes["H_SIx_1"] = "1";
							gameAttributes["H_SIx_2"] = "4";
							gameAttributes["H_SIx_3"] = "2";
							gameAttributes["H_SIx_4"] = "0";
						}
					}
					/*if ((mScenarioType != VOLTA_DROPIN) && (mScenarioType != VOLTA_PWF) && (mScenarioType != VOLTA_SOLO) && isPlatformHost)
					{
						advanceGameState(IN_GAME);
					}*/
					sleep(SSFSEASONS_CONFIG.GameStateSleepInterval);
					break;
				}
				case IN_GAME:
				{

					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=IN_GAME");

					if (!ingame_enter)
					{
						ingame_enter = true;
						//Start InGame Timer	

						if (mScenarioType == VOLTA_DROPIN || mScenarioType == VOLTA_PWF || mScenarioType == VOLTA_SOLO)
						{
							char8_t opponentIdString[MAX_GAMENAME_LEN] = { '\0' };
							char8_t playerIdString[MAX_GAMENAME_LEN] = { '\0' };
							blaze_snzprintf(playerIdString, sizeof(playerIdString), "%" PRId64, mPlayerData->getPlayerId());
							blaze_snzprintf(opponentIdString, sizeof(opponentIdString), "%" PRId64, mFriendBlazeId);

							Blaze::Messaging::MessageAttrMap messageAttrMap;
							messageAttrMap.clear();
							messageAttrMap[0] = "A_captain:1000023251242, A_FIx_4 : 1000023251242, A_FIx_1 : 1000362127571, A_SIx_0 : 5, A_SIx_1 : 1, A_SIx_2 : 4, A_SIx_3 : 2, A_SIx_4 : 0";
							sendMessage(mPlayerInfo, myGameId, messageAttrMap, 1735222131, ENTITY_TYPE_GAME_GROUP);

							Collections::AttributeMap gameAttributes;
							gameAttributes.clear();
							gameAttributes["LEADER"] = playerIdString;
							gameAttributes["STATE"] = "2";
							gameAttributes["UPDATE"] = "0";
							setGameAttributes(myGameId, gameAttributes);

							messageAttrMap.clear();
							messageAttrMap[1633839993] = "1";
							sendMessage(mPlayerInfo, mGameGroupId, messageAttrMap, 1734571116, ENTITY_TYPE_GAME_GROUP);
							sendMessage(mPlayerInfo, mGameGroupId, messageAttrMap, 1734571116, ENTITY_TYPE_GAME_GROUP);

							gameAttributes.clear();
							eastl::string mGameAttribute = eastl::string().sprintf("%" PRIu64 "_0|""%" PRIu64 """_0|", mPlayerInfo->getPlayerId(), mFriendBlazeId).c_str();
							gameAttributes["ARB_0"] = mGameAttribute.c_str();
							setGameAttributes(myGameId, gameAttributes);

							sendMessage(mPlayerInfo, mGameGroupId, messageAttrMap, 1734571116, ENTITY_TYPE_GAME_GROUP);
							messageAttrMap[1633841779] = "1";
							sendMessage(mPlayerInfo, mGameGroupId, messageAttrMap, 1734571116, ENTITY_TYPE_GAME_GROUP);

							gameAttributes.clear();
							mGameAttribute = eastl::string().sprintf("%" PRIu64 "_1|""%" PRIu64 """_0|", mPlayerInfo->getPlayerId(), mFriendBlazeId).c_str();
							gameAttributes["ARB_0"] = mGameAttribute.c_str();
							setGameAttributes(myGameId, gameAttributes);

							messageAttrMap.clear();
							messageAttrMap[1633839993] = "0";
							messageAttrMap[1633841260] = "1";
							sendMessage(mPlayerInfo, mGameGroupId, messageAttrMap, 1734571116, ENTITY_TYPE_GAME_GROUP);

							gameAttributes.clear();
							mGameAttribute = eastl::string().sprintf("%" PRIu64 "_1|""%" PRIu64 """_1|", mPlayerInfo->getPlayerId(), mFriendBlazeId).c_str();
							gameAttributes["ARB_1"] = mGameAttribute.c_str();
							setGameAttributes(myGameId, gameAttributes);

							sendMessage(mPlayerInfo, mGameGroupId, messageAttrMap, 1734571116, ENTITY_TYPE_GAME_GROUP);

							messageAttrMap.clear();
							messageAttrMap[1633839993] = "0";
							sendMessage(mPlayerInfo, mGameGroupId, messageAttrMap, 1734571116, ENTITY_TYPE_GAME_GROUP);

							gameAttributes.clear();
							gameAttributes["LEADER"] = playerIdString;
							gameAttributes["STATE"] = "4";
							gameAttributes["UPDATE"] = "0";
							setGameAttributes(myGameId, gameAttributes);
						}

						simulateInGame();
						if ((mScenarioType == VOLTA_DROPIN || mScenarioType == VOLTA_PWF || mScenarioType == VOLTA_SOLO) && isGrupPlatformHost)
						{
							setInGameStartTime();
						}
						if ((mScenarioType != VOLTA_DROPIN) && (mScenarioType != VOLTA_PWF) && (mScenarioType != VOLTA_SOLO) && isPlatformHost)
						{
							BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " setInGameStartTime. gameID=" << myGameId);
							setInGameStartTime();
						}
					}

					if (RollDice(SSFSEASONS_CONFIG.inGameReportTelemetryProbability))
					{
						executeReportTelemetry();
					}
					if ((mScenarioType == VOLTA_DROPIN || mScenarioType == VOLTA_PWF || mScenarioType == VOLTA_SOLO) && isGrupPlatformHost)
					{
						if (IsInGameTimedOut() || (mIsMarkedForLeaving == true))
						{
							BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " IsInGameTimedOut/mIsMarkedForLeaving=true. gameID=" << myGameId);
							//advanceGameState(POST_GAME);
						}
					}
					if ((mScenarioType != VOLTA_DROPIN) && (mScenarioType != VOLTA_PWF) && (mScenarioType != VOLTA_SOLO))
					{
						if ((isPlatformHost && IsInGameTimedOut()) || (mIsMarkedForLeaving == true))
						{
							BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " IsInGameTimedOut/mIsMarkedForLeaving=true. gameID=" << myGameId);
							advanceGameState(POST_GAME);
						}
					}

					sleep(SSFSEASONS_CONFIG.GameStateSleepInterval);
					break;
				}
				case INITIALIZING:
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=INITIALIZING");
					if (isPlatformHost)
					{
						lookupUsersByPersonaNames(mPlayerInfo, "");
						lookupUsersByPersonaNames(mPlayerInfo, "");
					}
					//if (isPlatformHost && (mScenarioType != FUTCHAMPIONMODE) && (mScenarioType != FUTDRAFTMODE) && (mScenarioType != FUTRIVALS) && (mScenarioType != FUT_ONLINE_SQUAD_BATTLE))
					//{
					//	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::simulateLoad][" << mPlayerData->getPlayerId() << "calling advanceGameState");
						//advanceGameState(PRE_GAME);
					//}

					sleep(SSFSEASONS_CONFIG.GameStateSleepInterval);
					break;
				}
				case CONNECTION_VERIFICATION:
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[onlineseasons::simulateload]" << mPlayerData->getPlayerId() << " with state=CONNECTION_VERIFICATION");
					sleep(OS_CONFIG.GameStateSleepInterval);
					break;
				}
				case POST_GAME:
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=POST_GAME");

					//Submitgamereport is initiated from NOTIFY_NOTIFYGAMESTATECHANGE async notification
					//submitGameReport(mScenarioType);
					//invokeUdatePrimaryExternalSessionForUser(mPlayerInfo, myGameId, "", "", myGameId, UPDATE_PRESENCE_REASON_GAME_LEAVE, GAME_TYPE_GAMESESSION, ACTIVE_CONNECTED);
					updatePrimaryExternalSessionForUserRequest(mPlayerInfo, myGameId, myGameId, UPDATE_PRESENCE_REASON_GAME_LEAVE, GAME_TYPE_GAMESESSION, ACTIVE_CONNECTED, PRESENCE_MODE_STANDARD, myGameId, GAME_TYPE_GROUP);
					//Leave game 	//	Calling in NOTIFY_NOTIFYGAMESTATECHANGE
					//BLAZE_INFO(BlazeRpcLog::gamemanager, "[SsfSeasons::leaveGame] :  leaving group : " );
					/*BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::Calling leaveGameByGroup for gameID] in POST_GAME " << mPlayerData->getPlayerId() << " with GameId - " << myGameId);
					leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, myGameId);
					if (mScenarioType == VOLTA_DROPIN || mScenarioType == VOLTA_PWF || mScenarioType == VOLTA_SOLO)
					{
						BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::Calling leaveGameByGroup for gamegroupID] in POST_GAME " << mPlayerData->getPlayerId() << " with GameGroupId - " << mGameGroupId);
						leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGameGroupId);
					}*/
					//invokeUdatePrimaryExternalSessionForUser(mPlayerInfo,mGameGroupId,"","", mGameGroupId, UPDATE_PRESENCE_REASON_GAME_LEAVE, GAME_TYPE_GROUP, ACTIVE_CONNECTED );
					updatePrimaryExternalSessionForUserRequest(mPlayerInfo, mGameGroupId, mGameGroupId, UPDATE_PRESENCE_REASON_GAME_LEAVE, GAME_TYPE_GROUP, ACTIVE_CONNECTED, PRESENCE_MODE_PRIVATE, mGameGroupId);
					
					//BLAZE_INFO(BlazeRpcLog::gamemanager, "[SsfSeasons::leaveGame][%" PRIu64 "]: player left game: GameId = %" PRIu64 "", mPlayerData->getPlayerId(), myGameId);
					//meshEndpointsDisconnected RPC is initiated from NOTIFY_NOTIFYPLAYERREMOVED
					//Remove friend info from the map
					removeFriendFromMap();

					postGameCalls();
					delayRandomTime(5000, 10000);
					setPlayerState(DESTRUCTING);

					sleep(SSFSEASONS_CONFIG.GameStateSleepInterval);
					break;
				}
				case DESTRUCTING:
				{
					//  Player Left Game
					//  Presently using this state when user is removed from the game and return from this function.
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=DESTRUCTING");
					setClientState(mPlayerInfo, ClientState::MODE_MENU);
					if (StressConfigInst.isOnlineScenario(mScenarioType))
					{
						mPlayerInfo->getShieldExtension()->perfTune(mPlayerInfo, false);
					}
					//  Return here
					return err;
				}
				case MIGRATING:
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=MIGRATING");
					//  mGameData will be freed when NOTIFY_NOTIFYGAMEREMOVED notification comes.
					sleep(10000);
					break;
				}
				case GAME_GROUP_INITIALIZED:
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=GAME_GROUP_INITIALIZED");
					sleep(5000);
					//if (playerStateDurationMs > SSFSEASONS_CONFIG.MaxGameGroupInitializedStateDuration)
					//{
					//    //  Leader might have disconnected or failed in Matchmaking or in leaving the Group.
					//    setPlayerState(DESTRUCTING);
					//   
					//}
					break;
				}
				case INACTIVE_VIRTUAL:
				default:
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=" << getPlayerState());
					//  shouldn't come here
					return err;
				}
				}
			}
		}

		void SsfSeasons::checkIfFriendExist()
		{
			BlazeId friendIDValue = 0;
			int count = 1;

			if ((((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->getVoltaFriendsList())->size() == 0 /*|| Random::getRandomNumber(100) < 50*/)
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
				addVoltaFriendsToMap(mPlayerInfo->getPlayerId(), friendIDValue);
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "CoopSeasons::checkIfFriendExist successful Set " << mPlayerInfo->getPlayerId() << " - " << friendIDValue);
			}
		}

		BlazeRpcError SsfSeasons::createOrJoinGame()
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
			hostAddress.switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
			hostAddress.getIpPairAddress()->getExternalAddress().setIp(mPlayerInfo->getConnection()->getAddress().getIp());
			hostAddress.getIpPairAddress()->getExternalAddress().setPort(mPlayerInfo->getConnection()->getAddress().getPort());
			hostAddress.getIpPairAddress()->getExternalAddress().copyInto(hostAddress.getIpPairAddress()->getInternalAddress());

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
			gameAttribs["Gamemode_Name"] = "VOLTA_MATCH";
			gameAttribs["OSDK_gameMode"] = "VOLTA_MATCH";

			GameCreationData & gameCreationData = request.getGameCreationData();
			gameCreationData.setDisconnectReservationTimeout(0);
			gameCreationData.setGameModRegister(0);
			gameCreationData.setGameName("VoltaClub_0");// gameName.c_str());
			gameCreationData.setPermissionSystemName("");

			//request.getCommonGameData().
			request.getPlayerJoinData().setGameEntryType(GAME_ENTRY_TYPE_DIRECT);  // As per the new TTY;
			request.getPlayerJoinData().setGroupId(mPlayerData->getConnGroupId());

			eastl::string persistedGameId = "VOLTA_";
			if (mPlayerInfo->getPlayerId() % 2 == 0)
			{
				persistedGameId += eastl::to_string(mPlayerInfo->getPlayerId());
			}
			else
			{
				persistedGameId += eastl::to_string((mPlayerInfo->getPlayerId() - 1));
			}
			persistedGameId += "_0";
			request.setPersistedGameId(persistedGameId.c_str());
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

			//char8_t buf[20046];
			//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "-> [VoltaSeasonsGameSession][" << mPlayerInfo->getPlayerId() << "]::CreateOrJoinGame" << "\n" << request.print(buf, sizeof(buf)));

			err = mPlayerInfo->getComponentProxy()->mGameManagerProxy->createOrJoinGame(request, response);
			if (err != ERR_OK)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "CoopSeasons::CreateOrJoinGame  failed. Error(" << ErrorHelp::getErrorName(err) << ")" << mPlayerInfo->getPlayerId());
				return err;
			}
			mGameGroupId = response.getGameId();
			//myGameId = response.getGameId();
			return err;
		}

		BlazeRpcError SsfSeasons::lobbyRPCcalls()
		{
			BlazeRpcError err = ERR_OK;
			FetchConfigResponse response;
			SponsoredEvents::CheckUserRegistrationResponse checkUserRegistrationResponse;
			GetStatsByGroupRequest::EntityIdList entityList;
			Blaze::BlazeIdList friendsList;
			friendsList.push_back(mPlayerInfo->getPlayerId());
			//char configSection[64];
			uint32_t viewId = mPlayerInfo->getViewId();
			entityList.push_back(mPlayerInfo->getPlayerId());

			Blaze::Clubs::GetMembersResponse clubResponse;
			// Set the club id or join the club before getting the club
			if (getMyClubId() == 0)
			{
				//BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "getMyClubId_rpc is failing " << getMyClubId());

				BlazeRpcError result = ERR_OK;
				Blaze::Clubs::GetClubMembershipForUsersResponse clubMembershipForUsersResponse;
				result = getClubMembershipForUsers(mPlayerInfo, clubMembershipForUsersResponse);

				Blaze::Clubs::ClubMemberships* memberShips = clubMembershipForUsersResponse.getMembershipMap()[mPlayerInfo->getPlayerId()];
				if (memberShips == NULL)
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "ClubsPlayer::lobbyRpcCalls:getMembershipMap() Response returned empty object " << mPlayerInfo->getPlayerId());
				}
				else
				{
					Blaze::Clubs::ClubMembershipList::iterator it = memberShips->getClubMembershipList().begin();
					setMyClubId((*it)->getClubId());
					//Player has already subsribed to a club - return here
					if (getMyClubId() != 0)
					{
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "ClubsPlayer::simulateLoad:player= " << mPlayerData->getPlayerId() << " received clubID from getClubMembershipForUsers. " << getMyClubId());
					}
				}
				GlobalClubsMap* clubsListReference = ((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->getCreatedClubsMap();
				GlobalClubsMap::iterator iter = clubsListReference->begin();

				//BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::createOrJoinClubs]:CLUBS_CONFIG.ClubSizeMax= " << CLUBS_CONFIG.ClubSizeMax << " " << mPlayerInfo->getPlayerId() );

				//Check if club has max players CLUBS_CONFIG.ClubSizeMax [11]
				//if space available join the club and increment count
				//if max players reached remove from the map

				//lock
				mMutex.Lock();
				while (iter != clubsListReference->end() && getMyClubId() == 0)
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::createOrJoinClubs]:external loop ClubID= " << iter->first << "  member count= " << iter->second << " " << mPlayerInfo->getPlayerId());
					if (iter->second < SSFSEASONS_CONFIG.clubSizeMax)
					{
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::createOrJoinClubs]: Inside loop ClubID= " << iter->first << "  member count= " << iter->second << " " << mPlayerInfo->getPlayerId());
						setMyClubId(iter->first);
						++(iter->second);
						//join club
						result = joinClub(getMyClubId());
						//CLUBS_ERR_CLUB_FULL
						if (result == CLUBS_ERR_CLUB_FULL)
						{
							//erase the clubID from map
							//clubsListReference->erase(iter);	
							iter->second = SSFSEASONS_CONFIG.clubSizeMax;
							BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::createOrJoinClubs]:CLUBS_ERR_CLUB_FULL:Removed clubID from map " << getMyClubId());
							setMyClubId(0);
						}
						else if (result != ERR_OK)
						{
							BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::createOrJoinClubs:joinClub failed ] " << mPlayerData->getPlayerId() << " " << ErrorHelp::getErrorName(result));
							--(iter->second);
							//unlock
							mMutex.Unlock();
							setMyClubId(0);
							//return result;
						}
					}
					iter++;
				}
				//unlock
				mMutex.Unlock();
			}

			if (getMyClubId() > 0)
			{
				GetMembersResponse getMembersResponse;
				getMembers(mPlayerInfo, getMyClubId(), 50, getMembersResponse);
			}

			if (!isPlatformHost && (getMyClubId() > 0))
			{
				getPetitions(mPlayerInfo, getMyClubId(), Blaze::Clubs::CLUBS_PETITION_SENT_TO_CLUB);
			}
			/*if(getMyClubId() > 0)
			{
				getStatsByGroupAsync(mPlayerInfo,"VProPosition", entityList, "club", getMyClubId(),0 );
				mPlayerInfo->setViewId(++viewId);
			}*/

			entityList.clear();
			entityList.push_back(mPlayerInfo->getPlayerId());

			getStatGroup(mPlayerInfo, "VProPosition");
			getStatsByGroupAsync(mPlayerInfo, friendsList, "VProPosition", getMyClubId());
			mPlayerInfo->setViewId(++viewId);

			// If volta dropin 
			if (mScenarioType == VOLTA_DROPIN)
			{
				getGameReportView(mPlayerInfo, "VoltaRecentGameParticipants");
				lookupUsers(mPlayerInfo, "");
			}
			
			setConnectionState(mPlayerInfo);
			lookupUsersByPersonaNames(mPlayerInfo, "");
			return err;
		}
		void SsfSeasons::addPlayerToList()
		{
			//  add player to universal list first
			Player::addPlayerToList();
			BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::addPlayerToList]: Player: " << mPlayerInfo->getPlayerId() << ", Game: " << myGameId);
			{
				((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->ssfSeasonsMap.insert(eastl::pair<PlayerId, eastl::string>(mPlayerInfo->getPlayerId(), mPlayerInfo->getPersonaName()));
			}
		}

		void SsfSeasons::removePlayerFromList()
		{
			//  remove player from universal list first
			Player::removePlayerFromList();
			PlayerId  id = mPlayerInfo->getPlayerId();
			BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::removePlayerFromList]: Player: " << id << ", Game: " << myGameId);
			PlayerMap& playerMap = ((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->ssfSeasonsMap;
			PlayerMap::iterator start = playerMap.begin();
			PlayerMap::iterator end = playerMap.end();
			for (; start != end; start++)
			{
				if (id == (*start).first) {
					break;
				}
			}
			if (start != end)
			{
				playerMap.erase(start);
			}
		}

		void SsfSeasons::simulateInGame()
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][SsfSeasons]: In Game simulation:" << myGameId);

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

		void SsfSeasons::postGameCalls()
		{
			setClientState(mPlayerInfo, Blaze::ClientState::MODE_SINGLE_PLAYER, Blaze::ClientState::STATUS_NORMAL);
		}


		bool SsfSeasons::executeReportTelemetry()
		{
			BlazeRpcError err = ERR_OK;
			if (!mPlayerData->isConnected())
			{
				BLAZE_ERR_LOG(BlazeRpcLog::util, "[SsfSeasons::ReportTelemetry::User Disconnected = " << mPlayerData->getPlayerId());
				return false;
			}
			if (getGameId() == 0)
			{
				BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::ReportTelemetry][GameId for player = " << mPlayerInfo->getPlayerId() << "] not present");
				return false;
			}
			StressGameInfo* ptrGameInfo = SsfSeasonsGameDataRef(mPlayerInfo).getGameData(getGameId());
			if (ptrGameInfo == NULL)
			{
				BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::ReportTelemetry][GameData for game = " << getGameId() << "] not present");
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
			telemetryReport->setRemoteConnGroupId(getOpponentPlayerInfo().getConnGroupId().id);
			
			err = mPlayerInfo->getComponentProxy()->mGameManagerProxy->reportTelemetry(reportTelemetryReq);
			if (err != ERR_OK)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "SsfSeasons: reportTelemetry failed with err=" << ErrorHelp::getErrorName(err));
			}
			return true;
		}

		BlazeRpcError SsfSeasons::startGroupMatchmakingScenario(ScenarioType scenarioName)
		{
			BlazeRpcError err = ERR_SYSTEM;
			StartMatchmakingScenarioRequest request;
			StartMatchmakingScenarioResponse response;
			MatchmakingCriteriaError error;
			request.getCommonGameData().setGameProtocolVersionString(getGameProtocolVersionString());
			request.getCommonGameData().setGameType(GAME_TYPE_GAMESESSION);
			NetworkAddress& hostAddress = request.getCommonGameData().getPlayerNetworkAddress();
			hostAddress.switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
			if (!mPlayerData->isConnected())
			{
				BLAZE_ERR_LOG(BlazeRpcLog::util, "[Volta::startMatchMaking::User Disconnected = " << mPlayerData->getPlayerId());
				return ERR_SYSTEM;
			}
			if (StressConfigInst.getPlatform() == PLATFORM_PS4)
			{
				hostAddress.getIpPairAddress()->getExternalAddress().setIp(mPlayerData->getConnection()->getAddress().getIp());
				hostAddress.getIpPairAddress()->getExternalAddress().setPort(mPlayerData->getConnection()->getAddress().getPort());
				hostAddress.getIpPairAddress()->getExternalAddress().copyInto(hostAddress.getIpPairAddress()->getInternalAddress());
			}

			PlayerJoinData& playerJoinData = request.getPlayerJoinData();
			playerJoinData.setGroupId(mPlayerInfo->getConnGroupId());
			playerJoinData.setDefaultRole("");
			playerJoinData.setGameEntryType(GameEntryType::GAME_ENTRY_TYPE_DIRECT);
			playerJoinData.setDefaultSlotType(SLOT_PUBLIC_PARTICIPANT);
			playerJoinData.setDefaultTeamId(ANY_TEAM_ID);
			playerJoinData.setDefaultTeamIndex(ANY_TEAM_ID + 1);
			PerPlayerJoinDataList& playerDataList = playerJoinData.getPlayerDataList();
			PerPlayerJoinDataPtr playerJoinDataPtr = playerDataList.pull_back();
			if (playerJoinDataPtr != NULL)
			{
				playerJoinDataPtr->setIsOptionalPlayer(false);
				playerJoinDataPtr->setEncryptedBlazeId("");
				//playerJoinDataPtr->setIsOptionalPlayer(false);
				playerJoinDataPtr->setRole("");
				playerJoinDataPtr->setSlotType(INVALID_SLOT_TYPE);
				playerJoinDataPtr->setTeamId(ANY_TEAM_ID + 1);
				playerJoinDataPtr->setTeamIndex(ANY_TEAM_ID + 1);
				Blaze::Collections::AttributeMap &attrMap = playerJoinDataPtr->getPlayerAttributes();
				attrMap.reserve(2);
				//attrMap["SSFAvatarDisplayName"] = "Cat";// mPlayerData->GetPersonaName().c_str();
				//attrMap["SSFAvatarOverallRating"] = "71";
				//attrMap["SSFMatchType"] = "6";
				//attrMap["SSFWalls"] = "0";
				attrMap["SSFAvatarDisplayName"] = mPlayerInfo->getPersonaName().c_str();
				attrMap["SSFAvatarOverallRating"] = "73";
				attrMap["SSFMatchType"] = "0";
				attrMap["SSFWalls"] = "-1";
				//User Identification
				playerJoinDataPtr->getUser().setAccountId(0);
				playerJoinDataPtr->getUser().setAccountLocale(0);
				playerJoinDataPtr->getUser().setAccountCountry(0);
				playerJoinDataPtr->getUser().setPersonaNamespace("");
				playerJoinDataPtr->getUser().setPidId(0);
				playerJoinDataPtr->getUser().setOriginPersonaId(0);
				playerJoinDataPtr->getUser().setExternalId(0);
				playerJoinDataPtr->getUser().setBlazeId(mPlayerData->getPlayerId());
				playerJoinDataPtr->getUser().setName("");
				playerJoinDataPtr->getUser().getPlatformInfo().setClientPlatform(ClientPlatformType::INVALID);

				playerJoinDataPtr->getUser().getPlatformInfo().getEaIds().setNucleusAccountId(mPlayerData->getPlayerId());
				playerJoinDataPtr->getUser().getPlatformInfo().getEaIds().setOriginPersonaId(0);
				playerJoinDataPtr->getUser().getPlatformInfo().getEaIds().setOriginPersonaName("");
			}

			SET_SCENARIO_ATTRIBUTE(request, "CUSTOM_CONTROLLER", "0");
			SET_SCENARIO_ATTRIBUTE(request, "GAMEGROUP_ONLY", "1");
			SET_SCENARIO_ATTRIBUTE(request, "GAME_MODE", "33");
			SET_SCENARIO_ATTRIBUTE(request, "MATCHUP_HASH", "4255788");
			SET_SCENARIO_ATTRIBUTE(request, "MATCH_CLUB_TYPE", "0");
			SET_SCENARIO_ATTRIBUTE(request, "MATCH_GUESTS", "0");
			SET_SCENARIO_ATTRIBUTE(request, "NET_TOP", (uint64_t)1);
			SET_SCENARIO_ATTRIBUTE(request, "NEW_USER_RULE_THR", "OSDK_matchAny");
			SET_SCENARIO_ATTRIBUTE(request, "SSF_DIVISION_THR", "matchLimited");
			SET_SCENARIO_ATTRIBUTE(request, "SSF_MATCH_TYPE_COUNT", (uint64_t)1);
			SET_SCENARIO_ATTRIBUTE(request, "SSF_MMR_OVERRIDE_VALUE", (uint64_t)1400);
			SET_SCENARIO_ATTRIBUTE(request, "SSF_MMR_SEARCH_VALUE", (uint64_t)1400);
			SET_SCENARIO_ATTRIBUTE(request, "SSF_MMR_THR", "OSDK_matchRelax");
			SET_SCENARIO_ATTRIBUTE(request, "SSF_NEW_USER", "1");
			SET_SCENARIO_ATTRIBUTE(request, "SSF_OPPONENT_TYPE", "0");

			SET_SCENARIO_ATTRIBUTE(request, "SSF_TEAM_OVR", (uint64_t)65);
			SET_SCENARIO_ATTRIBUTE(request, "SSF_TEAM_OVR_THR", "matchLimited");

			if (scenarioName == VOLTA_DROPIN)
			{
				SET_SCENARIO_ATTRIBUTE(request, "NEW_USER_RULE", (uint64_t)0);
				SET_SCENARIO_ATTRIBUTE(request, "SSF_DIVISION", (uint64_t)8);
				SET_SCENARIO_ATTRIBUTE(request, "SSF_MATCH_TYPE_ATTR", (uint64_t)0);
				SET_SCENARIO_ATTRIBUTE(request, "SSF_WALLS_ATTR", "abstain");
				request.setScenarioName("SSF_Seasons_Skill");
			}
			//char8_t buf[20240];
			//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "StartMatchmakingGroup RPC : \n" << request.print(buf, sizeof(buf)));

			err = mPlayerInfo->getComponentProxy()->mGameManagerProxy->startMatchmakingScenario(request, response, error);
			if (err == ERR_OK)
			{
				mMatchmakingSessionId = response.getScenarioId();
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "Volta::startMatchmakingScenarioGroup successful. For Player ID " << mPlayerInfo->getPlayerId() << " mMatchmakingSessionId= " << mMatchmakingSessionId);
				mSceanrioVariant = response.getScenarioVariant();
			}
			else
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "Volta::startMatchmakingScenarioGroup failed. For Player ID " << mPlayerInfo->getPlayerId() << " Error(" << ErrorHelp::getErrorName(err) << ")");
			}
			return err;
		}

		BlazeRpcError SsfSeasons::startMatchmakingScenario(ScenarioType scenarioName)
		{
			BlazeRpcError err = ERR_SYSTEM;
			StartMatchmakingScenarioRequest request;
			StartMatchmakingScenarioResponse response;
			MatchmakingCriteriaError error;
			request.getCommonGameData().setGameProtocolVersionString(StressConfigInst.getGameProtocolVersionString());
			request.getCommonGameData().setGameType(GAME_TYPE_GAMESESSION);
			NetworkAddress& hostAddress = request.getCommonGameData().getPlayerNetworkAddress();
			hostAddress.switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
			if (!mPlayerData->isConnected())
			{
				BLAZE_ERR_LOG(BlazeRpcLog::util, "[OnlineSeasons::startMatchMaking::User Disconnected = " << mPlayerData->getPlayerId());
				return ERR_SYSTEM;
			}
			hostAddress.getIpPairAddress()->getExternalAddress().setIp(mPlayerData->getConnection()->getAddress().getIp());
			hostAddress.getIpPairAddress()->getExternalAddress().setPort(mPlayerData->getConnection()->getAddress().getPort());
			hostAddress.getIpPairAddress()->getExternalAddress().copyInto(hostAddress.getIpPairAddress()->getInternalAddress());
			hostAddress.getIpPairAddress()->getExternalAddress().setMachineId(0);
			hostAddress.getIpPairAddress()->getMachineId();

			PlayerJoinData& playerJoinData = request.getPlayerJoinData();
			if (scenarioName == VOLTA_DROPIN || scenarioName == VOLTA_PWF || scenarioName == VOLTA_SOLO)
			{
				playerJoinData.setGroupId(BlazeObjectId(4, 2, mGameGroupId));
			}
			else
			{
				playerJoinData.setGroupId(mPlayerInfo->getConnGroupId());
			}
			playerJoinData.setDefaultRole("");
			playerJoinData.setGameEntryType(GameEntryType::GAME_ENTRY_TYPE_DIRECT);
			playerJoinData.setDefaultSlotType(SLOT_PUBLIC_PARTICIPANT);
			playerJoinData.setDefaultTeamId(ANY_TEAM_ID);	//	as per logs
			playerJoinData.setDefaultTeamIndex(ANY_TEAM_ID + 1);
			//playerJoinData.setTeamId(ANY_TEAM_ID);
			//playerJoinData.setTeamIndex(ANY_TEAM_ID + 1);
			PerPlayerJoinDataList& playerDataList = playerJoinData.getPlayerDataList();
			PerPlayerJoinDataPtr playerJoinDataPtr = playerDataList.pull_back();
			if (playerJoinDataPtr != NULL)
			{
				playerJoinDataPtr->setEncryptedBlazeId("");
				playerJoinDataPtr->setIsOptionalPlayer(false);
				playerJoinDataPtr->getPlayerAttributes().clear();
				playerJoinDataPtr->setSlotType(INVALID_SLOT_TYPE);
				Blaze::Collections::AttributeMap &attrMap = playerJoinDataPtr->getPlayerAttributes();
				attrMap.reserve(4);
				attrMap["SSFAvatarDisplayName"] = mPlayerInfo->getPersonaName().c_str();
				if (scenarioName == VOLTA_DROPIN)
				{
					attrMap["SSFAvatarOverallRating"] = "73";
				}
				else
				{
					attrMap["SSFAvatarOverallRating"] = "72";
				}
				attrMap["SSFMatchType"] = "-1";
				attrMap["SSFWalls"] = "-1";
				playerJoinDataPtr->setTeamIndex(ANY_TEAM_ID + 1);
				playerJoinDataPtr->setRole("");
				playerJoinDataPtr->getUser().setExternalId(mPlayerData->getExternalId());
				playerJoinDataPtr->getUser().setBlazeId(mPlayerData->getPlayerId());
				playerJoinDataPtr->getUser().setName(mPlayerData->getPersonaName().c_str());
				playerJoinDataPtr->getUser().setPersonaNamespace("");
				playerJoinDataPtr->getUser().setAccountId(0);
				playerJoinDataPtr->getUser().setAccountLocale(0);
				playerJoinDataPtr->getUser().setAccountCountry(0);
				playerJoinDataPtr->getUser().setOriginPersonaId(0);
				playerJoinDataPtr->getUser().setPidId(0);
			}

			SET_SCENARIO_ATTRIBUTE(request, "CUSTOM_CONTROLLER", "0");
			SET_SCENARIO_ATTRIBUTE(request, "GAMEGROUP_ONLY", "0");
			SET_SCENARIO_ATTRIBUTE(request, "GAME_MODE", "33");
			SET_SCENARIO_ATTRIBUTE(request, "MATCHUP_HASH", "4255788");
			SET_SCENARIO_ATTRIBUTE(request, "MATCH_CLUB_TYPE", "0");
			SET_SCENARIO_ATTRIBUTE(request, "MATCH_GUESTS", "0");
			SET_SCENARIO_ATTRIBUTE(request, "NET_TOP", (uint64_t)1);
			SET_SCENARIO_ATTRIBUTE(request, "NEW_USER_RULE_THR", "OSDK_matchAny");
			SET_SCENARIO_ATTRIBUTE(request, "SSF_DIVISION_THR", "matchLimited");
			SET_SCENARIO_ATTRIBUTE(request, "SSF_MATCH_TYPE_COUNT", (uint64_t)1);
			SET_SCENARIO_ATTRIBUTE(request, "SSF_MMR_OVERRIDE_VALUE", (uint64_t)0);
			SET_SCENARIO_ATTRIBUTE(request, "SSF_MMR_SEARCH_VALUE", (uint64_t)0);
			SET_SCENARIO_ATTRIBUTE(request, "SSF_MMR_THR", "OSDK_matchAny");
			SET_SCENARIO_ATTRIBUTE(request, "SSF_NEW_USER", "0");

			SET_SCENARIO_ATTRIBUTE(request, "SSF_TEAM_OVR", (uint64_t)69);
			SET_SCENARIO_ATTRIBUTE(request, "SSF_TEAM_OVR_THR", "matchLimited");

			if (scenarioName == VOLTA_FUTSAL)
			{
				SET_SCENARIO_ATTRIBUTE(request, "NEW_USER_RULE", (uint64_t)1);
				SET_SCENARIO_ATTRIBUTE(request, "SSF_DIVISION", (uint64_t)7);
				SET_SCENARIO_ATTRIBUTE(request, "SSF_MATCH_TYPE_ATTR", (uint64_t)6);
				SET_SCENARIO_ATTRIBUTE(request, "SSF_WALLS_ATTR", /*"abstain"*/(uint64_t)0);
				request.setScenarioName("SSF_Seasons");
			}
			else if (scenarioName == VOLTA_5v5)
			{
				SET_SCENARIO_ATTRIBUTE(request, "NEW_USER_RULE", (uint64_t)0);
				SET_SCENARIO_ATTRIBUTE(request, "SSF_DIVISION", (uint64_t)9);
				SET_SCENARIO_ATTRIBUTE(request, "SSF_MATCH_TYPE_ATTR", (uint64_t)5);
				SET_SCENARIO_ATTRIBUTE(request, "SSF_WALLS_ATTR", /*"abstain"*/(uint64_t)0);
				request.setScenarioName("SSF_Seasons");
			}
			else if (scenarioName == VOLTA_DROPIN)
			{
				SET_SCENARIO_ATTRIBUTE(request, "NEW_USER_RULE", (uint64_t)0);
				SET_SCENARIO_ATTRIBUTE(request, "SSF_DIVISION", (uint64_t)8);
				SET_SCENARIO_ATTRIBUTE(request, "SSF_MATCH_TYPE_ATTR", (uint64_t)6);

				SET_SCENARIO_ATTRIBUTE(request, "SSF_MMR_OVERRIDE_VALUE", (uint64_t)1402);
				SET_SCENARIO_ATTRIBUTE(request, "SSF_MMR_SEARCH_VALUE", (uint64_t)1402);
				SET_SCENARIO_ATTRIBUTE(request, "SSF_MMR_THR", "OSDK_matchRelax");

				SET_SCENARIO_ATTRIBUTE(request, "SSF_OPPONENT_TYPE", "0");

				SET_SCENARIO_ATTRIBUTE(request, "SSF_NEW_USER", "1");
				SET_SCENARIO_ATTRIBUTE(request, "SSF_TEAM_OVR", (uint64_t)67);
				SET_SCENARIO_ATTRIBUTE(request, "SSF_WALLS_ATTR", (uint64_t)0);

				request.setScenarioName("SSF_Seasons_Skill");
			}
			else if (scenarioName == VOLTA_PWF)
			{
				SET_SCENARIO_ATTRIBUTE(request, "NEW_USER_RULE", (uint64_t)0);
				SET_SCENARIO_ATTRIBUTE(request, "SSF_DIVISION", (uint64_t)5);
				SET_SCENARIO_ATTRIBUTE(request, "SSF_MATCH_TYPE_ATTR", "abstain");

				SET_SCENARIO_ATTRIBUTE(request, "SSF_MMR_OVERRIDE_VALUE", (uint64_t)1405);
				SET_SCENARIO_ATTRIBUTE(request, "SSF_MMR_SEARCH_VALUE", (uint64_t)1405);
				SET_SCENARIO_ATTRIBUTE(request, "SSF_MMR_THR", "OSDK_matchRelax");

				SET_SCENARIO_ATTRIBUTE(request, "SSF_OPPONENT_TYPE", "0");

				SET_SCENARIO_ATTRIBUTE(request, "SSF_NEW_USER", "1");
				SET_SCENARIO_ATTRIBUTE(request, "SSF_TEAM_OVR", (uint64_t)67);
				SET_SCENARIO_ATTRIBUTE(request, "SSF_WALLS_ATTR", "abstain");

				request.setScenarioName("SSF_Seasons_Skill");
			}

			else if (scenarioName == VOLTA_SOLO)
			{
				SET_SCENARIO_ATTRIBUTE(request, "NEW_USER_RULE", (uint64_t)0);
				SET_SCENARIO_ATTRIBUTE(request, "SSF_DIVISION", (uint64_t)8);
				SET_SCENARIO_ATTRIBUTE(request, "SSF_MATCH_TYPE_ATTR", "abstain");

				SET_SCENARIO_ATTRIBUTE(request, "SSF_MMR_OVERRIDE_VALUE", (uint64_t)1254);
				SET_SCENARIO_ATTRIBUTE(request, "SSF_MMR_SEARCH_VALUE", (uint64_t)1254);
				SET_SCENARIO_ATTRIBUTE(request, "SSF_MMR_THR", "OSDK_matchRelax");

				SET_SCENARIO_ATTRIBUTE(request, "SSF_OPPONENT_TYPE", "0");

				SET_SCENARIO_ATTRIBUTE(request, "SSF_NEW_USER", "1");
				SET_SCENARIO_ATTRIBUTE(request, "SSF_TEAM_OVR", (uint64_t)68);
				SET_SCENARIO_ATTRIBUTE(request, "SSF_WALLS_ATTR", "abstain");

				request.setScenarioName("SSF_Seasons_Skill");
			}
			//char8_t buf[20240];
			//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "StartMatchmaking RPC : \n" << request.print(buf, sizeof(buf)));

			err = mPlayerInfo->getComponentProxy()->mGameManagerProxy->startMatchmakingScenario(request, response, error);
			if (err == ERR_OK)
			{
				mMatchmakingSessionId = response.getScenarioId();
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "SsfSeasons::startMatchmakingScenario successful.for player " << mPlayerData->getPlayerId() << " mMatchmakingSessionId= " << mMatchmakingSessionId);
			}
			else
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "SsfSeasons::startMatchmakingScenario failed. Error(" << ErrorHelp::getErrorName(err) << ")");
			}
			return err;

		}
		
		BlazeRpcError SsfSeasons::joinClub(ClubId clubID)
		{
			BlazeRpcError err = ERR_OK;
			JoinOrPetitionClubRequest request;
			JoinOrPetitionClubResponse response;

			BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[" << mPlayerInfo->getPlayerId() << "][ClubsPlayer::joinClub]:for clubID= " << clubID);

			request.setClubId(clubID);
			request.setPetitionIfJoinFails(true);

			err = mPlayerInfo->getComponentProxy()->mClubsProxy->joinOrPetitionClub(request, response);
			ClubJoinOrPetitionStatus joinStatus = response.getClubJoinOrPetitionStatus();
			if (err == ERR_OK && joinStatus == CLUB_JOIN_SUCCESS)
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[" << mPlayerInfo->getPlayerId() << "][ClubsPlayer::joinClub]:success for clubID= " << clubID);
				//sendClubMesasge(false,PlayerInfo);
			}
			//CLUBS_ERR_CLUB_FULL
			else
			{
				BLAZE_ERR_LOG(BlazeRpcLog::clubs, "[ClubsPlayer::JoinClub]: Player [" << mPlayerInfo->getPlayerId() << ". ERROR " << ErrorHelp::getErrorName(err) << " :clubID " << clubID);
			}
			return err;
		}

	}  // namespace Stress
}  // namespace Blaze
