//  *************************************************************************************************
//
//   File:    ssfseasonsgamesession.cpp
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#include "framework/protocol/shared/heat2decoder.h"
#include "gamemanager/tdf/gamebrowser.h"
#include "gamemanager/tdf/legacyrules.h"
#include "gamemanager/tdf/rules.h"
#include "gamereporting/fifa/tdf/fifah2hreport.h"
#include "gamereporting/osdk/tdf/gameosdkreport.h"
#include "./ssfseasonsgamesession.h"
#include "./utility.h"

using namespace Blaze::GameManager;
using namespace Blaze::GameReporting;

namespace Blaze {
	namespace Stress {

		SsfSeasonsGameSession::SsfSeasonsGameSession(StressPlayerInfo* playerData, uint32_t GameSize, GameNetworkTopology Topology, GameProtocolVersionString GameProtocolVersionString)
			: GameSession(playerData, GameSize, Topology, GameProtocolVersionString),
			mFriendBlazeId(0), isGrupPlatformHost(false), mGameGroupId(0)
		{
			mGameReportingId = 0;
			mPlayerIds.push_back(0);
		}

		SsfSeasonsGameSession::~SsfSeasonsGameSession()
		{
			BLAZE_INFO(BlazeRpcLog::util, "[SsfSeasonsGameSession][Destructor][%" PRIu64 "]: destroy called", mPlayerData->getPlayerId());
		}

		void SsfSeasonsGameSession::asyncHandlerFunction(uint16_t type, EA::TDF::Tdf *notification)
		{
			if (mPlayerData != NULL && mPlayerData->isPlayerActive())
			{
				//  This Notification has not been handled so use the Default Handler
				onGameSessionDefaultAsyncHandler(type, notification);
			}
		}

		void SsfSeasonsGameSession::onGameSessionDefaultAsyncHandler(uint16_t type, EA::TDF::Tdf* notification)
		{
			//  This is abstract class and handle the async handlers in the derived class

			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]:Message Type =" << type);
			switch (type)
			{
			case GameManagerSlave::NOTIFY_NOTIFYGAMESETUP:
				//  Async notification sent to the player who joined a game.  Triggered by JoinGame Request or a Matchmaking session finding a game.
			{
				NotifyGameSetup* notify = (NotifyGameSetup*)notification;
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::asyncHandlerFunction]: Received  NOTIFY_NOTIFYGAMESETUP." << mPlayerData->getPlayerId());
				if (notify->getGameData().getGameType() == GAME_TYPE_GROUP)
				{
					handleNotifyGameSetUpGroup(notify);
					//homeGameId = notify->getGameData().getGameId();
				}
				else if (notify->getGameData().getGameType() == GAME_TYPE_GAMESESSION)
				{
					handleNotifyGameSetUp(notify);
					mGameReportingId = notify->getGameData().getGameReportingId();
				}
			}
			break;
			case GameManagerSlave::NOTIFY_NOTIFYGAMESTATECHANGE:
			{
				NotifyGameStateChange* notify = (NotifyGameStateChange*)notification;
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMESTATECHANGE, state=" <<
					notify->getNewGameState());
				if (myGameId == notify->getGameId())
				{
					if (getPlayerState() == IN_GAME && notify->getNewGameState() == POST_GAME)
					{
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::NOTIFY_NOTIFYGAMESTATECHANGE][" << mPlayerData->getPlayerId() << "]: submitGameReport");
						submitGameReport(mScenarioType);
						if (StressConfigInst.isOnlineScenario(mScenarioType))
						{
							mPlayerData->getShieldExtension()->perfTune(mPlayerData, true);
						}
						leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, myGameId);
						BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[SsfSeasons::Calling leaveGameByGroup for gamegroupID] in POST_GAME " << mPlayerData->getPlayerId() << " with GameGroupId - " << mGameGroupId);
						leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGameGroupId);
						//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[SSFPlayer::Calling leaveGameByGroup for gameID] in NOTIFY_NOTIFYGAMESTATECHANGE " << mPlayerData->getPlayerId() << " with GameId - " << myGameId);
						//leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, myGameId);
					}
					//  Set Game State
					setPlayerState(notify->getNewGameState());
					SsfSeasonsGameDataRef(mPlayerData).setGameState(myGameId, notify->getNewGameState());
				}
			}
			break;
			case GameManagerSlave::NOTIFY_NOTIFYGAMERESET:
			{
				NotifyGameReset* notify = (NotifyGameReset*)notification;
				StressGameInfo* gameInfo = SsfSeasonsGameDataRef(mPlayerData).getGameData(notify->getGameData().getGameId());
				if (gameInfo != NULL && gameInfo->getPlatformHostPlayerId() == mPlayerData->getPlayerId())
				{
					//  TODO: host specific actions
				}
				BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMERESET.gameId=" <<
					notify->getGameData().getGameId());
			}
			break;
			case GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINING:
			{
				NotifyPlayerJoining* notify = (NotifyPlayerJoining*)notification;
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERJOINING.gameId=" << notify->getGameId());

				//For Game Group
				if (notify->getGameId() == mGameGroupId)
				{
					//meshEndpointsConnected
					BlazeObjectId targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, notify->getJoiningPlayer().getConnectionGroupId());
					meshEndpointsConnected(targetGroupId, 0, 0, mGameGroupId);
					// Add player to game map
					mPlayerInGameGroupMap.insert(notify->getJoiningPlayer().getPlayerId());
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::NOTIFYPLAYERJOINING:GAME_TYPE_GROUP][ GameGroupID : " << mGameGroupId << " joinerId " << notify->getJoiningPlayer().getPlayerId());
				}
				//For Game Session Type
				if (notify->getGameId() == myGameId)
				{
					//meshEndpointsConnected
					BlazeObjectId targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, notify->getJoiningPlayer().getConnectionGroupId());
					meshEndpointsConnected(targetGroupId, 0, 0, myGameId);
					// Add player to game map
					mPlayerInGameMap.insert(notify->getJoiningPlayer().getPlayerId());
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::NOTIFYPLAYERJOINCOMPLETED:mPlayerInGameMap][ GameID : " << myGameId << " joinerId " << notify->getJoiningPlayer().getPlayerId());

					if ((mScenarioType != VOLTA_DROPIN) && (mScenarioType != VOLTA_PWF))
					{
						//Add opponent player info if not received in notifygamesetup
						if (getOpponentPlayerInfo().getPlayerId() == 0 && mPlayerData->getPlayerId() != notify->getJoiningPlayer().getPlayerId())
						{
							getOpponentPlayerInfo().setPlayerId(notify->getJoiningPlayer().getPlayerId());
							getOpponentPlayerInfo().setConnGroupId(BlazeObjectId(ENTITY_TYPE_CONN_GROUP, notify->getJoiningPlayer().getConnectionGroupId()));
							BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << " OpponentPlayerInfo PlayerId=" << getOpponentPlayerInfo().getPlayerId() << " ConnGroupId=" << getOpponentPlayerInfo().getConnGroupId().toString());
						}
					}
				}
			}
			break;
			case GameManagerSlave::NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS:
			{
				BLAZE_INFO(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::handlerFunction][%" PRIu64 "]: Received  NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS.", mPlayerData->getPlayerId());
			}
			break;
			case GameManagerSlave::NOTIFY_NOTIFYPLATFORMHOSTINITIALIZED:
			{
				NotifyPlatformHostInitialized* notify = (NotifyPlatformHostInitialized*)notification;
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLATFORMHOSTINITIALIZED.gameId=" <<
					notify->getGameId());
			}
			break;
			case GameManagerSlave::NOTIFY_NOTIFYGAMELISTUPDATE:
			{
				//NotifyGameListUpdate* notify = (NotifyGameListUpdate*)notification;
				BLAZE_INFO(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::asyncHandlerFunction][%" PRIu64 "]: Received  NOTIFY_NOTIFYGAMELISTUPDATE.", mPlayerData->getPlayerId());

			}
			break;
			case GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINCOMPLETED:
			{
				NotifyPlayerJoinCompleted* notify = (NotifyPlayerJoinCompleted*)notification;
				BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERJOINCOMPLETED.");
				//char8_t buf[20240];
				//BLAZE_TRACE_RPC_LOG(BlazeRpcLog::gamemanager, "NOTIFY_NOTIFYPLAYERJOINCOMPLETED RPC : \n" << notify->print(buf, sizeof(buf)));
				//we get all the players in a coop game during joinCompleted call Back
				if (notify->getGameId() == myGameId)
				{
					// Add player to game map
					mPlayerInGameMap.insert(notify->getPlayerId());

					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::NOTIFYPLAYERJOINCOMPLETED:mPlayerInGameMap][ GameID : " << myGameId << " joinerId " << notify->getPlayerId());

					if (IsPlayerPresentInList((PlayerModule*)(mPlayerData->getOwner()), notify->getPlayerId(), SSFSEASONS) == true)
					{
						uint16_t count = SsfSeasonsGameDataRef(mPlayerData).getPlayerCount(myGameId);
						SsfSeasonsGameDataRef(mPlayerData).setPlayerCount(myGameId, ++count);
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::NOTIFYPLAYERJOINCOMPLETED][" << mPlayerData->getPlayerId() << "]:GameId=" << myGameId << " Player count = " << count);
					}

					if (mScenarioType == VOLTA_DROPIN || mScenarioType == VOLTA_PWF)
					{
						updatePrimaryExternalSessionForUserRequest(mPlayerData, myGameId, myGameId, UPDATE_PRESENCE_REASON_GAME_JOIN, GAME_TYPE_GAMESESSION, ACTIVE_CONNECTED, PRESENCE_MODE_STANDARD, mGameGroupId);
					}
					else
					{
						/*if (isPlatformHost)
						{
							invokeUdatePrimaryExternalSessionForUser(mPlayerData, myGameId, "", "", myGameId, GAME_JOIN, GAME_TYPE_GAMESESSION, ACTIVE_CONNECTED);
						}*/
						invokeUdatePrimaryExternalSessionForUser(mPlayerData, myGameId, "", "", myGameId, GAME_JOIN, GAME_TYPE_GAMESESSION, ACTIVE_CONNECTED);
					}
				}
				if (notify->getGameId() == mGameGroupId)
				{
					//invokeUdatePrimaryExternalSessionForUser(mPlayerData, mGameGroupId, "", "", mGameGroupId, GAME_JOIN, GAME_TYPE_GROUP, ACTIVE_CONNECTED);
					updatePrimaryExternalSessionForUserRequest(mPlayerData, mGameGroupId, 0, UPDATE_PRESENCE_REASON_GAME_JOIN, GAME_TYPE_GROUP, ACTIVE_CONNECTED, PRESENCE_MODE_PRIVATE);
				}
			}
			break;
			case GameManagerSlave::NOTIFY_NOTIFYPLAYERREMOVED:
			{
				NotifyPlayerRemoved* notify = (NotifyPlayerRemoved*)notification;
				//safe check when player removed due to location based MM fails								

				//Game Type Group
				if (notify->getGameId() == mGameGroupId)
				{
					if (isGrupPlatformHost)
					{
						//meshEndpointsDisconnected
						BlazeObjectId targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, mTopologyHostConnectionGroupId);
						meshEndpointsDisconnected(targetGroupId, DISCONNECTED_PLAYER_REMOVED, 0, mGameGroupId);
						targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, 0);
						meshEndpointsDisconnected(targetGroupId, DISCONNECTED, 0, mGameGroupId);
					}
					else
					{
						//meshEndpointsDisconnected to opponent group ID TODO
						BlazeObjectId targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, 0);
						meshEndpointsDisconnected(targetGroupId, DISCONNECTED, 0, mGameGroupId);
					}
				}
				//If Game_Type_Gamesession
				if (mPlayerData->getPlayerId() == notify->getPlayerId() && myGameId == notify->getGameId())
				{
					setPlayerState(POST_GAME);
					//meshEndpointsDisconnected
					if (mScenarioType == VOLTA_DROPIN || mScenarioType == VOLTA_PWF)
					{
						BlazeObjectId targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, mTopologyHostConnectionGroupId);
						meshEndpointsDisconnected(targetGroupId, DISCONNECTED_PLAYER_REMOVED, 5, myGameId);
					}
					else
					{
						BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERREMOVED for game [" << notify->getGameId() << "] reason: " << notify->getPlayerRemovedReason());
						if (isPlatformHost)
						{
							BlazeObjectId targetGroupId = getOpponentPlayerInfo().getConnGroupId();
							BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << " meshEndpointsDisconnected for grouID" << targetGroupId.toString());
							meshEndpointsDisconnected(targetGroupId, DISCONNECTED_PLAYER_REMOVED);
							targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, 0);
							meshEndpointsDisconnected(targetGroupId, DISCONNECTED);
						}
						else
						{
							//meshEndpointsDisconnected - DISCONNECTED
							BlazeObjectId targetGroupId = getOpponentPlayerInfo().getConnGroupId();
							BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << " meshEndpointsDisconnected for grouID" << targetGroupId.toString());
							meshEndpointsConnectionLost(targetGroupId, 1, myGameId);
							meshEndpointsDisconnected(targetGroupId, DISCONNECTED);

						}
					}
				}

				StressGameInfo* ptrGameInfo = SsfSeasonsGameDataRef(mPlayerData).getGameData(myGameId);
				if (!ptrGameInfo)
				{
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]::receiving notification(" << type << ") and gameId " << myGameId <<
						"not present in SSFSeasonsGamesMap");
					break;
				}
				if (myGameId == notify->getGameId() && SsfSeasonsGameDataRef(mPlayerData).isPlayerExistInGame(notify->getPlayerId(), notify->getGameId()) == true)
				{
					SsfSeasonsGameDataRef(mPlayerData).removePlayerFromGameData(notify->getPlayerId(), notify->getGameId());
					if (IsPlayerPresentInList((PlayerModule*)(mPlayerData->getOwner()), notify->getPlayerId(), SSFSEASONS) == true)
					{
						uint16_t count = ptrGameInfo->getPlayerCount();
						ptrGameInfo->setPlayerCount(--count);
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::asyncHandlerFunction]::NOTIFYPLAYERREMOVED[" << mPlayerData->getPlayerId() << "]:GameId=" << myGameId << " Player count = " << count
							<< "for player=" << notify->getPlayerId());
					}
				}
				//  if all players, in single exe, removed from the game delete the data from map
				if (ptrGameInfo->getPlayerCount() <= 0)
				{
					BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERREMOVED - Game Id = " <<
						notify->getGameId() << ".");
					SsfSeasonsGameDataRef(mPlayerData).removeGameData(notify->getGameId());
				}
			}
			break;
			case GameManagerSlave::NOTIFY_NOTIFYGAMEREMOVED:
			{
				//  NotifyGameRemoved* notify = (NotifyGameRemoved*)notification;
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMEREMOVED.");
			}
			break;
			case GameManagerSlave::NOTIFY_NOTIFYREMOTEMATCHMAKINGENDED:
			{
				//updateGameSession(mGameGroupId);
			}
			break;
			case GameManagerSlave::NOTIFY_NOTIFYMATCHMAKINGFAILED:
			{

				NotifyMatchmakingFailed* notify = (NotifyMatchmakingFailed*)notification;
				MatchmakingResult result = notify->getMatchmakingResult();
				switch (result)
				{
				case GameManager::SESSION_TIMED_OUT:
				{
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::OnNotifyMatchmakingFailed][" << mPlayerData->getPlayerId() << "]: Matchmake SESSION_TIMED_OUT.");
				}
				break;
				case GameManager::SESSION_CANCELED:
				{
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::OnNotifyMatchmakingFailed][" << mPlayerData->getPlayerId() << "]: Matchmake SESSION_CANCELED.");
					//  Throw Execption;
				}
				break;
				default:
					break;
				}
				BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYMATCHMAKINGFAILED.");

				if (mScenarioType == VOLTA_DROPIN || mScenarioType == VOLTA_PWF)
				{
					leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGameGroupId);
					//Remove friend info from the map
					removeFriendFromMap();
				}
				unsubscribeFromCensusData(mPlayerData);
				//Move player state to destructing
				setPlayerState(DESTRUCTING);
			}
			break;
			case GameManagerSlave::NOTIFY_NOTIFYGAMEREPORTINGIDCHANGE:
			{
			}
			break;
			case GameManagerSlave::NOTIFY_NOTIFYHOSTMIGRATIONFINISHED:
			{
			}
			break;
			case GameManagerSlave::NOTIFY_NOTIFYHOSTMIGRATIONSTART:
			{
				NotifyHostMigrationStart* notify = (NotifyHostMigrationStart*)notification;
				if ((NULL == notify) && (PLATFORM_HOST_MIGRATION != notify->getMigrationType()))
				{
					break;
				}
				//   Update the host migration status as soon as migration starts - all players + game server does this.
				updateGameHostMigrationStatus(mPlayerData, notify->getGameId(), GameManager::PLATFORM_HOST_MIGRATION);
			}
			break;
			case UserSessionsSlave::NOTIFY_USERADDED:
			{
				NotifyUserAdded* notify = (NotifyUserAdded*)notification;

				if (mScenarioType == VOLTA_DROPIN || mScenarioType == VOLTA_PWF)
				{
					mPlayerIds.push_back(mPlayerData->getPlayerId());
				}
				if (notify->getUserInfo().getBlazeId() == mPlayerData->getPlayerId())
				{
					Blaze::ObjectIdList::iterator iter = notify->getExtendedData().getBlazeObjectIdList().begin();
					while (iter != notify->getExtendedData().getBlazeObjectIdList().end())
					{
						if (iter->type.component == (uint16_t)11)
						{
							setMyClubId(iter->id);
							break;
						}
						iter++;
					}
				}
			}
			break;
			case UserSessionsSlave::NOTIFY_USERUPDATED:
			{
				//UserStatus* notify = (UserStatus*)notification;
			}
			break;
			case GameManagerSlave::NOTIFY_NOTIFYGAMEATTRIBCHANGE:
			{
				NotifyGameAttribChange* notify = (NotifyGameAttribChange*)notification;
				if (notify->getGameId() == myGameId)  //  check for GameID to ensure we're updating the right Game
				{
				}
			}
			break;
			case Blaze::Messaging::MessagingSlave::NOTIFY_NOTIFYMESSAGE:
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::handlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYMESSAGE.");
				// Messaging::ServerMessage* notify = (Messaging::ServerMessage*)notification;
					//   Handle player message notifications.
					//   Get the message details

			}
			break;
			case Blaze::Perf::PerfSlave::NOTIFY_NOTIFYPERFTUNE:
			{

				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::handlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPERFTUNE.");
				Blaze::Perf::NotifyPerfTuneResponse* notify = (Blaze::Perf::NotifyPerfTuneResponse*)notification;
				//BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::handlerFunction][" << mPlayerData->getPlayerId() << "]: Is enable ? " << notify->getEnable());
				if (notify->getEnable())
				{
					//first ping, so it is extension heartbeat which runs every 30secs 
					mPlayerData->getShieldExtension()->ping(mPlayerData, true,0);
					mPlayerData->getShieldExtension()->isFirstPingResponse = true;//to indicate that the next ping response will have canary module
				}
				else
				{
					mPlayerData->getShieldExtension()->stopHeartBeat();
				}
			}
			break;
			case Blaze::Perf::PerfSlave::NOTIFY_NOTIFYPINGRESPONSE:
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::handlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPINGRESPONSE.");
				Blaze::Perf::NotifyPingResponse* notify = (Blaze::Perf::NotifyPingResponse*)notification;
				//BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::handlerFunction][" << mPlayerData->getPlayerId() << "]: Ping Data : " << notify);
				handleNotifyPingResponse(notify);
			}
			break;
			default:
			{
				//   payload not handled by recipient.
			}
			break;
			}
			//onGameSessionAsyncHandlerExtended(type, notification);
			//  Delete the memory
			delete notification;
		}

		void SsfSeasonsGameSession::handleNotifyGameSetUpGroup(NotifyGameSetup* notify, uint16_t type /* = GameManagerSlave::NOTIFY_NOTIFYGAMESETUP*/)
		{
			if (notify->getGameData().getPlatformHostInfo().getPlayerId() == mPlayerData->getPlayerId()) {
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::handleNotifyGameSetUpGroup][" << mPlayerData->getPlayerId() << "," << notify->getGameData().getPlatformHostInfo().getPlayerId() << "}");
				isGrupPlatformHost = true;

			}
			//set gamegroup ID
			mGameGroupId = notify->getGameData().getGameId();

			//meshEndpointsConnected RPC to self
			BlazeObjectId targetGroupId = mPlayerData->getConnGroupId();
			meshEndpointsConnected(targetGroupId, 0, 0, mGameGroupId);
			//If host call meshEndpointsConnected RPC for joining player in NOTIFYPLAYERJOINING notification
			//If joiner call meshEndpointsConnected RPC to platformHost

			if (isGrupPlatformHost)
			{
				finalizeGameCreation(mGameGroupId);
				//advanceGameState
				//advanceGameState(GAME_GROUP_INITIALIZED, mGameGroupId);

				//invokeUdatePrimaryExternalSessionForUser(mPlayerData, mGameGroupId, "", "", mGameGroupId, GAME_CREATE, GAME_TYPE_GROUP, ACTIVE_CONNECTED);
				updatePrimaryExternalSessionForUserRequest(mPlayerData, mGameGroupId, mGameGroupId, UPDATE_PRESENCE_REASON_GAME_CREATE, GAME_TYPE_GROUP, ACTIVE_CONNECTED, PRESENCE_MODE_PRIVATE);

			}

			Collections::AttributeMap attributes;
			char8_t playerIdString[MAX_GAMENAME_LEN] = { '\0' };
			blaze_snzprintf(playerIdString, sizeof(playerIdString), "%" PRId64, mPlayerData->getPlayerId());

			attributes.clear();
			attributes["AINFO"]		= "-1,1,14,892,4,1000,0,5,6,3,100,8,167,75";
			attributes["AMORPH"]	= "6,0,0,0,0,0,8,0,0,0,0,0,5,0,0,0,0,0,7,0,0,0,0,0,9,0,0,0,0,0,17,0,0,0,0,0,5,0,0,0,0,0";
			attributes["ANAME"]		= "Frosty";
			attributes["AOUTFIT"]	= "1161,1425,-1,1206,0,1405,0,0,0,0,0,0,0,0,-1";
			attributes["ASTATS"]	= "81,1,2,-1,-1,-1,3,-1,21,0";
			attributes["MMRRTG"]	= "1383";
			attributes["PLUPD"]		= "2";
			attributes["ULOC"]		= "0";
			attributes["UNAME"]		= playerIdString;

			setPlayerAttributes(mGameGroupId, attributes);

			if (isGrupPlatformHost)
			{
				char8_t opponentIdString[MAX_GAMENAME_LEN] = { '\0' };
			
				blaze_snzprintf(opponentIdString, sizeof(opponentIdString), "%" PRId64, mFriendBlazeId);

				Blaze::Messaging::MessageAttrMap messageAttrMap;
				messageAttrMap.clear();
				messageAttrMap[12] = "0";
				sendMessage(mPlayerData, mGameGroupId, messageAttrMap, 1734571116, ENTITY_TYPE_GAME_GROUP);

				sendMessage(mPlayerData, mGameGroupId, messageAttrMap, 1734571116, ENTITY_TYPE_GAME_GROUP);

				Collections::AttributeMap gameAttributes;
				gameAttributes["AINFO0"] = "-1,0,14,847,3,1014,0,4,0,2,0,2,180,85";
				gameAttributes["AINFO1"] = "-1,0,59,847,1,1017,0,1,0,6,0,1,160,75";
				gameAttributes["AINFO2"] = "-1,1,56,888,4,1000,0,3,0,1,150,6,188,115";
				gameAttributes["AINFO3"] = "-1,1,27,878,1,1000,0,3,0,6,150,2,175,75";
				gameAttributes["AINFO4"] = "-1,1,14,892,4,1000,0,5,6,3,100,8,167,75";
				gameAttributes["AMORPH0"] = "7.000,0,0,0,0,0,3.000,0,0,0,0,0,10.000,0,0,0,0,0,7.000,0,0,0,0,0,20.000,0,0,0,0,0,23.000,0,0,0,0,0,0,0,0,0,0,0";
				gameAttributes["AMORPH1"] = "0,0,0,0,0,0,0,0,0,0,0,0,15.000,0,0,0,0,0,9.000,0,0,0,0,0,18.000,0,0,0,0,0,26.000,0,0,0,0,0,2.000,0,0,0,0,0";
				gameAttributes["AMORPH2"] = "7.000,0,0,0,0,0,5.000,0,0,0,0,0,3.000,0,0,0,0,0,6.000,0,0,0,0,0,6.000,0,0,0,0,0,3.000,0,0,0,0,0,3.000,0,0,0,0,0";
				gameAttributes["AMORPH3"] = "1.000,0,0,0,0,0,6.000,0,0,0,0,0,4.000,0,0,0,0,0,6.000,0,0,0,0,0,6.000,0,0,0,0,0,5.000,0,0,0,0,0,3.000,0,0,0,0,0";
				gameAttributes["AMORPH4"] = "6.000,0,0,0,0,0,11.000,0,0,0,0,0,2.000,0,0,0,0,0,1.000,0,0,0,0,0,20.000,0,0,0,0,0,0,0,0,0,0,0,5.000,0,0,0,0,0";
				gameAttributes["ANAME0"] = "Patty";
				gameAttributes["ANAME1"] = "Vic";
				gameAttributes["ANAME2"] = "Nellie";
				gameAttributes["ANAME3"] = "Annie";
				gameAttributes["ANAME4"] = "Zzzxd";
				gameAttributes["AOUTFIT0"] = "1161,491,-1,566,0,474,0,0,0,0,0,0,0,0,-1";
				gameAttributes["AOUTFIT1"] = "1161,491,-1,566,0,474,0,0,0,0,0,0,0,0,-1";
				gameAttributes["AOUTFIT2"] = "1161,491,-1,566,0,474,0,0,0,0,0,0,0,0,-1";
				gameAttributes["AOUTFIT3"] = "1161,491,-1,566,0,474,0,0,0,0,0,0,0,0,-1";
				gameAttributes["AOUTFIT4"] = "1161,554,-1,567,644,518,0,0,0,0,0,0,0,0,-1";
				gameAttributes["ASTATS0"] = "67,4,3,-1,-1,-1";
				gameAttributes["ASTATS1"] = "66,2,3,-1,-1,-1";
				gameAttributes["ASTATS2"] = "66,2,2,-1,-1,-1";
				gameAttributes["ASTATS3"] = "67,2,2,-1,-1,-1";
				gameAttributes["ASTATS4"] = "73,1,2,-1,-1,-1";
				gameAttributes["FORM"] = "2";
				gameAttributes["LEADER"] = "1000023251242";
				gameAttributes["LUSLOT0"] = "-1";
				gameAttributes["LUSLOT1"] = "1000362127571";
				gameAttributes["LUSLOT2"] = "-1";
				gameAttributes["LUSLOT3"] = "-1";
				gameAttributes["LUSLOT4"] = "1000023251242";
				gameAttributes["MMSETTINGS"] = "0,0,-1,0";
				gameAttributes["STATE"] = "1";
				gameAttributes["TYPE"] = "2";
				gameAttributes["UPDATE"] = "4";
				setGameAttributes(mGameGroupId, gameAttributes);

				gameAttributes.clear();
				eastl::string mGameAttribute = eastl::string().sprintf("%" PRIu64 "_0|""%" PRIu64 """_0|", mPlayerData->getPlayerId(), mFriendBlazeId).c_str();
				gameAttributes["ARB_0"] = mGameAttribute.c_str();
				setGameAttributes(mGameGroupId, gameAttributes);

				sendMessage(mPlayerData, mGameGroupId, messageAttrMap, 1734571116, ENTITY_TYPE_GAME_GROUP);
				messageAttrMap[1633841779] = "1";
				sendMessage(mPlayerData, mGameGroupId, messageAttrMap, 1734571116, ENTITY_TYPE_GAME_GROUP);

				gameAttributes.clear();
				mGameAttribute = eastl::string().sprintf("%" PRIu64 "_1|""%" PRIu64 """_0|", mPlayerData->getPlayerId(), mFriendBlazeId).c_str();
				gameAttributes["ARB_0"] = mGameAttribute.c_str();
				setGameAttributes(mGameGroupId, gameAttributes);

				messageAttrMap.clear();
				messageAttrMap[1633839993] = "0";
				messageAttrMap[1633841260] = "1";
				sendMessage(mPlayerData, mGameGroupId, messageAttrMap, 1734571116, ENTITY_TYPE_GAME_GROUP);

				gameAttributes.clear();
				mGameAttribute = eastl::string().sprintf("%" PRIu64 "_1|""%" PRIu64 """_1|", mPlayerData->getPlayerId(), mFriendBlazeId).c_str();
				gameAttributes["ARB_0"] = mGameAttribute.c_str();
				setGameAttributes(mGameGroupId, gameAttributes);

				sendMessage(mPlayerData, mGameGroupId, messageAttrMap, 1734571116, ENTITY_TYPE_GAME_GROUP);

				messageAttrMap.clear();
				messageAttrMap[1633839993] = "0";
				sendMessage(mPlayerData, mGameGroupId, messageAttrMap, 1734571116, ENTITY_TYPE_GAME_GROUP);

				gameAttributes.clear();
				gameAttributes["LEADER"] = playerIdString;
				gameAttributes["STATE"] = "2";
				gameAttributes["UPDATE"] = "0";
				setGameAttributes(mGameGroupId, gameAttributes);

			}
			//mPlayerInGameMap.insert(mPlayerData->getPlayerId());
			//mPlayerInGameMap.insert(notify->getGameData().getPlatformHostInfo().getPlayerId());
			mPlayerInGameGroupMap.insert(mPlayerData->getPlayerId());
			ReplicatedGamePlayerList &roster = notify->getGameRoster();
			if (roster.size() > 0)
			{
				ReplicatedGamePlayerList::const_iterator citEnd = roster.end();
				for (ReplicatedGamePlayerList::const_iterator cit = roster.begin(); cit != citEnd; ++cit)
				{
					PlayerId playerId = (*cit)->getPlayerId();
					mPlayerInGameGroupMap.insert(playerId);
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::handleNotifyGameGroupSetUp:mPlayerInGameGroupMap] " << mPlayerData->getPlayerId() << "  member= " << playerId);
				}
			}
		}

		void SsfSeasonsGameSession::handleNotifyGameSetUp(NotifyGameSetup* notify, uint16_t type /* = GameManagerSlave::NOTIFY_NOTIFYGAMESETUP*/)
		{
			BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId());
			if (notify->getGameData().getTopologyHostInfo().getPlayerId() == mPlayerData->getPlayerId())
			{
				isTopologyHost = true;
			}
			//if (notify->getGameData().getPlatformHostInfo().getPlayerId() == mPlayerData->getPlayerId())
			//{
			//	isPlatformHost = true;
			//	//blaze_snzprintf(homeCaptain, sizeof(homeCaptain), "%" PRId64, mPlayerData->getPlayerId());
			//}
			PlayerIdList playerList = notify->getGameData().getAdminPlayerList();
			PlayerIdList::iterator it = playerList.begin();
			while (it != playerList.end())
			{
				if (mPlayerData->getPlayerId() == *it)
				{
					isPlatformHost = true;
					break;
				}
				it++;
			}

			/*****  Add Game Data *****/
			//  Set My Game ID
			setGameId(notify->getGameData().getGameId());
			// Read connection group ID 
			mTopologyHostConnectionGroupId = notify->getGameData().getTopologyHostInfo().getConnectionGroupId();

			//  Set Game State
			setPlayerState(notify->getGameData().getGameState());

			if (SsfSeasonsGameDataRef(mPlayerData).isGameIdExist(myGameId))
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "]::receiving notification(" << type << ") and gameId " <<
					notify->getGameData().getGameId() << "already present in SSFSeasonsGamesMap");
				//  Player will be added during NOTIFYPLAYERJOINCOMPLETED notification, not here.
			}
			else
			{
				//  create new game data
				SsfSeasonsGameDataRef(mPlayerData).addGameData(notify);
			}

			//meshEndpointsConnected RPC to self
			BlazeObjectId targetGroupId = mPlayerData->getConnGroupId();
			meshEndpointsConnected(targetGroupId, 0, 0, myGameId);
			setClientState(mPlayerData, ClientState::Mode::MODE_MULTI_PLAYER, ClientState::Status::STATUS_NORMAL);
			//meshEndpointsConnected RPC to Dedicated server
			BlazeObjectId topologyHostConnId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, mTopologyHostConnectionGroupId);
			meshEndpointsConnected(topologyHostConnId, 0, 0, myGameId);

			//Add players to the map
			ReplicatedGamePlayerList &roster = notify->getGameRoster();
			if (roster.size() > 0)
			{
				ReplicatedGamePlayerList::const_iterator citEnd = roster.end();
				for (ReplicatedGamePlayerList::const_iterator cit = roster.begin(); cit != citEnd; ++cit)
				{
					PlayerId playerId = (*cit)->getPlayerId();
					mPlayerInGameMap.insert(playerId);
					if (playerId != mPlayerData->getPlayerId())
					{
						mPlayerInGameMap.insert(playerId);
						if (mScenarioType == VOLTA_DROPIN || mScenarioType == VOLTA_PWF)
						{
							//meshEndpointsConnected RPC other players in roster
							BlazeObjectId connId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, (*cit)->getConnectionGroupId());
							meshEndpointsConnected(connId, 0, 0, myGameId);
						}
						else
						{
							getOpponentPlayerInfo().setPlayerId(playerId);
							//meshEndpointsConnected RPC to opponent
							targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, (*cit)->getConnectionGroupId());
							meshEndpointsConnected(targetGroupId, 0, 0, myGameId);
						}
					}
				}
			}

			/*if (mScenarioType == VOLTA_DROPIN || mScenarioType == VOLTA_PWF || mScenarioType == VOLTA_SOLO)
			{

			}*/
			// Add player to game map
			//mPlayerInGameMap.insert(mPlayerData->getPlayerId());
			//mPlayerInGameMap.insert(notify->getGameData().getPlatformHostInfo().getPlayerId());
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::handleNotifyGameSetUp]" << mPlayerData->getPlayerId() << "HANDLE_NOTIFYGAMESETUP Current Player count for game ID " << getGameId() << " count : " << mPlayerInGameMap.size() << " As follows :");

			if (isPlatformHost && (mScenarioType == VOLTA_5v5 || mScenarioType == VOLTA_FUTSAL))
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "calling finalizeGameCreation");
				finalizeGameCreation();
			}

			StressGameInfo* ptrGameInfo = SsfSeasonsGameDataRef(mPlayerData).getGameData(myGameId);
			if (!ptrGameInfo)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "] gameId " << myGameId << " not present in ssf SeasonsGamesMap");
				return;
			}

			if (isPlatformHost == true && IsPlayerPresentInList((PlayerModule*)(mPlayerData->getOwner()), mPlayerData->getPlayerId(), SSFSEASONS) == true)
			{
				uint16_t count = SsfSeasonsGameDataRef(mPlayerData).getPlayerCount(myGameId);
				SsfSeasonsGameDataRef(mPlayerData).setPlayerCount(myGameId, ++count);
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "]:GameId=" << myGameId << " Player count = " << count);
			}
		}
		void SsfSeasonsGameSession::handleNotifyPingResponse(Blaze::Perf::NotifyPingResponse* notify, uint16_t type /*= Blaze::Perf::PerfSlave::NOTIFY_NOTIFYPINGRESPONSE*/)
		{
			int datasize = notify->getData().getSize();
			if (datasize != 0)
			{
				std::string data = { '\x0a','\x02' ,'\x12','\x00' };
				std::vector<uint8_t> myVector(data.begin(), data.end());
				uint8_t *p = &myVector[0];
				bool unload = true;
				for (int i = 0; i < 4; i++)
				{
					if (notify->getData().getData()[i] != p[i])
						unload = false;
				}
				if (unload)
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::perf, "unload module received by playerid:" << mPlayerData->getPlayerId());
					mPlayerData->getShieldExtension()->ping(mPlayerData, true, 0);
				}
				else
				{
					//ping for canary or PC device id or loadtestmodule
					if (datasize == 57039)
					{//canary
						BLAZE_TRACE_LOG(BlazeRpcLog::perf, "canary module ping received by playerid:" << mPlayerData->getPlayerId());
						mPlayerData->getShieldExtension()->ping(mPlayerData, false, 1);
					}
					else if (datasize == 104727)
					{//pc device id
						BLAZE_TRACE_LOG(BlazeRpcLog::perf, "pc device id received by playerid:" << mPlayerData->getPlayerId());
						mPlayerData->getShieldExtension()->ping(mPlayerData, false, 2);
					}
					else if (datasize == 119271)
					{//loadtest module
						BLAZE_TRACE_LOG(BlazeRpcLog::perf, "loadtest request received by playerid:" << mPlayerData->getPlayerId());
						mPlayerData->getShieldExtension()->ping(mPlayerData, false, 3);
					}
					else
					{
						BLAZE_INFO_LOG(BlazeRpcLog::perf, "empty received by playerid" << mPlayerData->getPlayerId() << notify->getData());
					}
				}
			}
		}

		EA::TDF::tdf_ptr <Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> FillSkillEvent(uint16_t dribblepast, uint16_t count, int skilltype)
		{
			EA::TDF::tdf_ptr < Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent1 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent();

			skillEvent1->setDribblePast(dribblepast);
			skillEvent1->setCount(count);
			skillEvent1->setSkillType(skilltype);
			return skillEvent1;
		}

		BlazeRpcError SsfSeasonsGameSession::submitGameReport(ScenarioType scenarioName)
		{
			Blaze::BlazeRpcError err = ERR_OK;
			Blaze::GameReporting::SSFSeasonsReportBase::SSFSeasonsGameReport* ssfSeasonsGameReport = NULL;
			Blaze::GameReporting::OSDKGameReportBase::OSDKReport *osdkReport = NULL;
			Blaze::GameReporting::SSFSeasonsReportBase::SSFTeamSummaryReport *ssfTeamSummaryReport = NULL;

			GameReporting::SubmitGameReportRequest request;

			// Set status of FNSH
			if (scenarioName == VOLTA_DROPIN || scenarioName == VOLTA_PWF)
			{
				request.setFinishedStatus(Blaze::GameReporting::GAMEREPORT_FINISHED_STATUS_FINISHED);
			}
			else
			{
				request.setFinishedStatus(Blaze::GameReporting::GAMEREPORT_FINISHED_STATUS_DEFAULT);
			}
			// Get gamereport object which needs to be filled with game data
			GameReporting::GameReport &gamereport = request.getGameReport();

			// Set GRID
			gamereport.setGameReportingId(mGameReportingId); //VoltaGameDataRef(mPlayerData).getGameData(myGameId)->getGameReportingId());
			//soloGameReport = BLAZE_NEW Blaze::GameReporting::Fifa::SoloGameReport;
			ssfSeasonsGameReport = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SSFSeasonsGameReport;
			if (ssfSeasonsGameReport == NULL) {
				BLAZE_ERR(BlazeRpcLog::gamemanager, "[SSFGameReportInstance] startgamereporting function call failed, failed to allocate memory for variable soloGameReport");
				return ERR_SYSTEM;
			}
			osdkReport = BLAZE_NEW Blaze::GameReporting::OSDKGameReportBase::OSDKReport;
			//substitution = BLAZE_NEW Blaze::GameReporting::Fifa::Substitution;
			if (osdkReport == NULL) {
				BLAZE_ERR(BlazeRpcLog::gamemanager, "[SSFGameReportInstance] startgamereporting function call failed, failed to allocate memory for variable osdkReport");
				return ERR_SYSTEM;
			}
			/*if (substitution == NULL) {
				BLAZE_ERR(BlazeRpcLog::gamemanager, "[SSFGameReportInstance] startgamereporting function call failed, failed to allocate memory for variable substitution");
				return ERR_SYSTEM;
			}*/
			BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "MOMO : inside gamereport : memory alloc successful");

			// Game report contains osdk report, adding osdkreport to game report
			gamereport.setReport(*osdkReport);
			gamereport.setGameReportName("gameType33");

			// Fill the osdkgamereport fields
			Blaze::GameReporting::OSDKGameReportBase::OSDKGameReport& osdkGamereport = osdkReport->getGameReport();
			//Blaze::GameReporting::OSDKGameReportBase::OSDKGameReport& osdkGamereport = osdkReport->getCustomReport();

			// GTIM
			osdkGamereport.setGameTime(52225);
			osdkGamereport.setGameType("gameType33");

			// Set GTYP && TYPE
			osdkGamereport.setFinishedStatus(0);
			osdkGamereport.setArenaChallengeId(0);
			osdkGamereport.setCategoryId(0);
			osdkGamereport.setGameReportId(0);
			osdkGamereport.setSimulate(false);
			osdkGamereport.setLeagueId(0);
			osdkGamereport.setRank(false);
			osdkGamereport.setRoomId(0);
			//osdkGamereport.setSponsoredEventId(0);
			osdkGamereport.setFinishedStatus(1);
			// For now updating values as per client logs.TODO: need to add values according to server side settings
			ssfSeasonsGameReport->setMom("");
			//if (scenarioName == VOLTA_DROPIN || mSubScenarioPlayerType == VOLTA_PWF)
			//{
			Blaze::GameReporting::SSFSeasonsReportBase::GoalEventVector& goalSummary = ssfSeasonsGameReport->getGoalSummary();

			Blaze::GameReporting::SSFSeasonsReportBase::GoalEvent goal;
			goal.setScoringTeam(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid1 = goal.getPrimaryAssist();
			avid1.setPersonaId(0);
			avid1.setPlayerId(0);
			avid1.setSlotId(0);
			goal.setPrimaryAssistType(0);
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid2 = goal.getSecondaryAssist();
			avid2.setPersonaId(0);
			avid2.setPlayerId(0);
			avid2.setSlotId(0);
			goal.setSecondaryAssistType(0);
			goal.getGoalFlags().push_back(20);
			goal.setGoalEventTime(610);
			goal.setGoalType(0);
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid3 = goal.getScorer();
			avid3.setPersonaId(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
			avid3.setPlayerId(((mPlayerData->getPlayerId()) % 10));
			avid3.setSlotId(0);
			goalSummary.push_back(&goal);
			//}
			// fill the common game report value
			//Blaze::GameReporting::Fifa::CommonGameReport& commonGameReport = soloGameReport->getCommonGameReport();
			Blaze::GameReporting::SSF::CommonGameReport& commonGameReport = ssfSeasonsGameReport->getCommonGameReport();
			// WNPK
			commonGameReport.setWentToPk(0);
			commonGameReport.setIsRivalMatch(false);
			// For now updating values as per client logs.TODO: need to add values according to server side settings
			commonGameReport.setBallId(120);
			commonGameReport.setAwayCrestId(0);
			commonGameReport.setHomeCrestId(0);
			commonGameReport.setStadiumId(1000);

			osdkGamereport.setCustomGameReport(*ssfSeasonsGameReport);

			int playerMapSize = mPlayerInGameMap.size();
			//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[SSFGameReportInstance::submitGameReport][" << mPlayerData->getPlayerId() << "]:player map size =" << playerMapSize);

			if ((scenarioName == VOLTA_DROPIN || scenarioName == VOLTA_PWF) && playerMapSize != 4)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[SSFGameReportInstance::submitGameReport][" << mPlayerData->getPlayerId() << "]:player map size is wrong, return");
				return ERR_SYSTEM;
			}

			if ((scenarioName == VOLTA_5v5 || scenarioName == VOLTA_FUTSAL) && playerMapSize != 2)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[SSFGameReportInstance::submitGameReport][" << mPlayerData->getPlayerId() << "]:player map size is wrong, return");
				return ERR_SYSTEM;
			}

			// populate player data in playermap
			Blaze::GameReporting::OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = osdkReport->getPlayerReports();
			OsdkPlayerReportsMap.reserve(mPlayerInGameMap.size());

			for (eastl::set<BlazeId>::iterator citPlayerIt = mPlayerInGameMap.begin(), citPlayerItEnd = mPlayerInGameMap.end();
				citPlayerIt != citPlayerItEnd; ++citPlayerIt)
			{
				GameManager::PlayerId playerId = *citPlayerIt;
				Blaze::GameReporting::OSDKGameReportBase::OSDKPlayerReport* playerReport = OsdkPlayerReportsMap.allocate_element();

				Blaze::GameReporting::SSFSeasonsReportBase::SSFSeasonsPlayerReport* ssfPlayerReport = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SSFSeasonsPlayerReport;
				if (ssfPlayerReport == NULL) {
					BLAZE_ERR(BlazeRpcLog::gamemanager, "[SSFGameReportInstance] startgamereporting function call failed, failed to allocate memory for variable ssfPlayerReport");
					return ERR_SYSTEM;
				}
				Blaze::GameReporting::SSF::CommonStatsReport& commonPlayerReport = ssfPlayerReport->getCommonPlayerReport();

				// set the commonplayer report attribute values
				commonPlayerReport.setAveragePossessionLength(0);
				commonPlayerReport.setAssists(rand() % 10);
				commonPlayerReport.setBlocks(0); //Added as per logs
				commonPlayerReport.setGoalsConceded(rand() % 10);
				commonPlayerReport.setHasCleanSheets(0);
				commonPlayerReport.setCorners(rand() % 15);
				commonPlayerReport.setPassAttempts(rand() % 15);
				commonPlayerReport.setPassesMade(rand() % 12);
				commonPlayerReport.setRating((float)(rand() % 10));
				commonPlayerReport.setSaves(rand() % 10);
				commonPlayerReport.setShots(rand() % 12);
				commonPlayerReport.setTackleAttempts(rand() % 12);
				commonPlayerReport.setTacklesMade(rand() % 15);
				commonPlayerReport.setFouls(rand() % 12);
				commonPlayerReport.setAverageFatigueAfterNinety(0);
				commonPlayerReport.setGoals(rand() % 15);
				commonPlayerReport.setInjuriesRed(0);
				commonPlayerReport.setInterceptions(rand() % 10);  // Added newly
				commonPlayerReport.setHasMOTM(0);
				commonPlayerReport.setOffsides(rand() % 15);
				commonPlayerReport.setOwnGoals(rand() % 10);
				commonPlayerReport.setPkGoals(rand() % 10);
				commonPlayerReport.setPossession(rand() % 15);
				commonPlayerReport.setPassesIntercepted(0);
				commonPlayerReport.setPenaltiesAwarded(0);
				commonPlayerReport.setPenaltiesMissed(0);
				commonPlayerReport.setPenaltiesScored(0);
				commonPlayerReport.setPenaltiesSaved(0);
				commonPlayerReport.setRedCard(rand() % 12);
				commonPlayerReport.setShotsOnGoal(rand() % 15);
				commonPlayerReport.setUnadjustedScore(rand() % 10);  // Added newly
				commonPlayerReport.setYellowCard(rand() % 12);

				ssfPlayerReport->setManOfTheMatch(rand() % 10);
				ssfPlayerReport->setPos(rand() % 12);
				ssfPlayerReport->setCleanSheetsAny(rand() % 10);
				ssfPlayerReport->setCleanSheetsDef(rand() % 10);
				ssfPlayerReport->setCleanSheetsGoalKeeper(rand() % 10);
				ssfPlayerReport->setTeamEntityId(playerId);
				ssfPlayerReport->setSsfEndResult(Blaze::GameReporting::SSFSeasonsReportBase::SsfMatchEndResult::SSF_END_INVALID);// missing
				ssfPlayerReport->setUserEndSubReason(Blaze::GameReporting::SSFSeasonsReportBase::SsfUserEndSubReason::SSF_USER_RESULT_NONE); // missing

				playerReport->setCustomDnf(rand() % 10);
				playerReport->setScore(rand() % 15);
				playerReport->setAccountCountry(0);
				playerReport->setFinishReason(rand() % 10);
				playerReport->setGameResult(rand() % 10);
				playerReport->setHome(true);
				playerReport->setLosses(rand() % 10);
				playerReport->setName("2 Dev 303311227");
				playerReport->setOpponentCount(rand() % 10);
				playerReport->setExternalId(rand() % 10);
				playerReport->setNucleusId(playerId);
				playerReport->setPersona("2 Dev 303311227");
				playerReport->setPointsAgainst(rand() % 10);
				playerReport->setUserResult(rand() % 10);
				playerReport->setClientScore(rand() % 10);
				// playerReport->setSponsoredEventAccountRegion(0); // missing
				playerReport->setSkill(rand() % 10);
				playerReport->setSkillPoints(rand() % 10);
				playerReport->setTeam(114152);
				playerReport->setTies(rand() % 10);
				playerReport->setWinnerByDnf(rand() % 10);
				playerReport->setWins(0);
				playerReport->setCustomPlayerReport(*ssfPlayerReport);

				if (playerId == mPlayerData->getPlayerId())
				{
					Blaze::GameReporting::OSDKGameReportBase::OSDKPrivatePlayerReport &osdkPrivatePlayerReport = playerReport->getPrivatePlayerReport();
					Blaze::GameReporting::OSDKGameReportBase::IntegerAttributeMap &attrMap = osdkPrivatePlayerReport.getPrivateIntAttributeMap();
					attrMap.reserve(45);
					attrMap["BANDAVGGM"] = 1000 * (uint16_t)Random::getRandomNumber(1000) + 7362040;
					attrMap["BANDAVGNET"] = (uint16_t)Random::getRandomNumber(1000);
					attrMap["BANDHIGM"] = -1 * Random::getRandomNumber(2000000000);
					attrMap["BANDHINET"] = 1000 * (uint16_t)Random::getRandomNumber(10) + (uint16_t)Random::getRandomNumber(100);
					attrMap["BANDLOWGM"] = 1000000 * (uint16_t)Random::getRandomNumber(10) + 347935;
					attrMap["BANDLOWNET"] = (uint16_t)Random::getRandomNumber(100);
					attrMap["BYTESRCVDNET"] = (uint16_t)Random::getRandomNumber(10) + 454125;
					attrMap["BYTESSENTNET"] = 100000 * (uint16_t)Random::getRandomNumber(10) + 247803;
					attrMap["DROPPKTS"] = -1 * (uint16_t)Random::getRandomNumber(10);
					attrMap["FPSAVG"] = 100000 * (uint16_t)Random::getRandomNumber(10) + 86221;;
					attrMap["FPSDEV"] = 100000 * (uint16_t)Random::getRandomNumber(10) + 82331;;
					attrMap["FPSHI"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["FPSLOW"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["GAMEENDREASON"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["GDESYNCEND"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["GDESYNCRSN"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["GENDPHASE"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["GPCKTPERIOD"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["GPCKTPERIODSTD"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["GRESULT"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["GRPTTYPE"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["GRPTVER"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["GUESTS0"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["GUESTS1"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["LATEAVGGM"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["LATEAVGNET"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["LATEHIGM"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["LATEHINET"] = (uint16_t)Random::getRandomNumber(100);
					attrMap["LATELOWGM"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["LATELOWNET"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["LATESDEVGM"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["LATESDEVNET"] = (uint16_t)Random::getRandomNumber(100);
					attrMap["PKTLOSS"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["REALTIMEGAME"] = (uint16_t)Random::getRandomNumber(1000);
					attrMap["REALTIMEIDLE"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["USERSEND0"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["USERSEND1"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["USERSSTRT0"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["USERSSTRT1"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["VOIPEND0"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["VOIPEND1"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["VOIPSTRT0"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["VOIPSTRT1"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["gid"] = getGameId();
					attrMap["matchStatus"] = (uint16_t)Random::getRandomNumber(10);
					attrMap["user_end_subreason"] = (uint16_t)Random::getRandomNumber(10);
				}

				OsdkPlayerReportsMap[playerId] = playerReport;

			}
			GameManager::PlayerId opponentPlayerId = getOpponentPlayerInfo().getPlayerId();
			//playerReport = OsdkPlayerReportsMap.allocate_element();

			GameManager::PlayerId playerId = mPlayerData->getPlayerId();
			ssfTeamSummaryReport = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SSFTeamSummaryReport;
			if (ssfTeamSummaryReport == NULL) {
				BLAZE_ERR(BlazeRpcLog::gamemanager, "[SSFGameReportInstance] startgamereporting function call failed, failed to allocate memory for variable ssfTeamSummaryReport");
				return ERR_SYSTEM;
			}
			Blaze::GameReporting::SSFSeasonsReportBase::SSFTeamReportMap& ssfTeamReportMap = ssfTeamSummaryReport->getSSFTeamReportMap();
			ssfTeamReportMap.reserve(2);
			Blaze::GameReporting::SSFSeasonsReportBase::SSFTeamReport *teamReport1 = ssfTeamReportMap.allocate_element();
			teamReport1->setSquadDisc(rand() % 10);
			teamReport1->setHome(false);
			teamReport1->setLosses(rand() % 10);
			teamReport1->setGameResult(rand() % 10);
			teamReport1->setScore(rand() % 10);
			teamReport1->setTies(rand() % 10);
			teamReport1->setSkillMoveCount(rand() % 10); //missing
			teamReport1->setWinnerByDnf(rand() % 10);
			teamReport1->setWins(playerId % 10);
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarVector& avVector = teamReport1->getAvatarVector();
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry *avEntry1 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry;
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avatarId1 = avEntry1->getAvatarId();
			avatarId1.setPersonaId(opponentPlayerId);
			avatarId1.setPlayerId(0);
			avatarId1.setSlotId(1);
			Blaze::GameReporting::SSF::CommonStatsReport &avatarStatsReport1 = avEntry1->getAvatarStatReport();
			avatarStatsReport1.setAveragePossessionLength(rand() % 10);
			avatarStatsReport1.setAssists(opponentPlayerId % 10);
			//avatarStatsReport1.setBlocks(0); // missing
			avatarStatsReport1.setGoalsConceded(opponentPlayerId % 10);
			avatarStatsReport1.setHasCleanSheets(rand() % 10);
			avatarStatsReport1.setCorners(opponentPlayerId % 15);
			avatarStatsReport1.setPassAttempts(opponentPlayerId % 15);
			avatarStatsReport1.setPassesMade(opponentPlayerId % 12);
			avatarStatsReport1.setRating((float)(opponentPlayerId % 10));
			avatarStatsReport1.setSaves(opponentPlayerId % 10);
			avatarStatsReport1.setShots(opponentPlayerId % 12);
			avatarStatsReport1.setTackleAttempts(opponentPlayerId % 12);
			avatarStatsReport1.setTacklesMade(opponentPlayerId % 15);
			avatarStatsReport1.setFouls(opponentPlayerId % 12);
			avatarStatsReport1.setAverageFatigueAfterNinety(0);
			avatarStatsReport1.setGoals(opponentPlayerId % 15);
			avatarStatsReport1.setInjuriesRed(0);
			avatarStatsReport1.setInterceptions(opponentPlayerId % 10);
			avatarStatsReport1.setHasMOTM(0);
			avatarStatsReport1.setOffsides(opponentPlayerId % 15);
			avatarStatsReport1.setOwnGoals(opponentPlayerId % 10);
			avatarStatsReport1.setPkGoals(opponentPlayerId % 10);
			avatarStatsReport1.setPossession(opponentPlayerId % 15);
			avatarStatsReport1.setPassesIntercepted(0);
			avatarStatsReport1.setPenaltiesAwarded(0);
			avatarStatsReport1.setPenaltiesMissed(0);
			avatarStatsReport1.setPenaltiesScored(0);
			avatarStatsReport1.setPenaltiesSaved(0);
			avatarStatsReport1.setRedCard(opponentPlayerId % 12);
			avatarStatsReport1.setShotsOnGoal(opponentPlayerId % 15);
			avatarStatsReport1.setUnadjustedScore(opponentPlayerId % 10);
			avatarStatsReport1.setYellowCard(opponentPlayerId % 12);
			avVector.push_back(avEntry1);

			//avatarvector entry 2
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry *avEntry2 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry;
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avatarId2 = avEntry2->getAvatarId();
			avatarId2.setPersonaId(opponentPlayerId);
			avatarId2.setPlayerId(1);
			avatarId2.setSlotId(2);
			Blaze::GameReporting::SSF::CommonStatsReport &avatarStatsReport2 = avEntry2->getAvatarStatReport();
			avatarStatsReport2.setAveragePossessionLength(0);
			avatarStatsReport2.setAssists(opponentPlayerId % 10);
			//avatarStatsReport2.setBlocks(0); // missing
			avatarStatsReport2.setGoalsConceded(opponentPlayerId % 10);
			avatarStatsReport2.setHasCleanSheets(0);
			avatarStatsReport2.setCorners(opponentPlayerId % 15);
			avatarStatsReport2.setPassAttempts(opponentPlayerId % 15);
			avatarStatsReport2.setPassesMade(opponentPlayerId % 12);
			avatarStatsReport2.setRating((float)(opponentPlayerId % 10));
			avatarStatsReport2.setSaves(opponentPlayerId % 10);
			avatarStatsReport2.setShots(opponentPlayerId % 12);
			avatarStatsReport2.setTackleAttempts(opponentPlayerId % 12);
			avatarStatsReport2.setTacklesMade(opponentPlayerId % 15);
			avatarStatsReport2.setFouls(opponentPlayerId % 12);
			avatarStatsReport2.setAverageFatigueAfterNinety(0);
			avatarStatsReport2.setGoals(opponentPlayerId % 15);
			avatarStatsReport2.setInjuriesRed(0);
			avatarStatsReport2.setInterceptions(opponentPlayerId % 10);
			avatarStatsReport2.setHasMOTM(0);
			avatarStatsReport2.setOffsides(opponentPlayerId % 15);
			avatarStatsReport2.setOwnGoals(opponentPlayerId % 10);
			avatarStatsReport2.setPkGoals(opponentPlayerId % 10);
			avatarStatsReport2.setPossession(opponentPlayerId % 15);
			avatarStatsReport2.setPassesIntercepted(0);
			avatarStatsReport2.setPenaltiesAwarded(0);
			avatarStatsReport2.setPenaltiesMissed(0);
			avatarStatsReport2.setPenaltiesScored(0);
			avatarStatsReport2.setPenaltiesSaved(0);
			avatarStatsReport2.setRedCard(opponentPlayerId % 12);
			avatarStatsReport2.setShotsOnGoal(opponentPlayerId % 15);
			avatarStatsReport2.setUnadjustedScore(opponentPlayerId % 10);
			avatarStatsReport2.setYellowCard(opponentPlayerId % 12);

			if (scenarioName != VOLTA_DROPIN && scenarioName != VOLTA_PWF)
			{
				Blaze::GameReporting::SSFSeasonsReportBase::SkillTimeBucketMap &avatarSkillTime2 = avEntry2->getSkillTimeBucketMap();
				avatarSkillTime2.initMap(1);
				Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap *skillEventmap2 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap();
				Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType2 = 39;
				EA::TDF::tdf_ptr < Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent2 = FillSkillEvent(0, 2, 39);
				skillEventmap2->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType2, skillEvent2)));
				auto it = avatarSkillTime2.begin();
				it->first = 6;
				it->second->setTimePeriod(6);
				skillEventmap2->copyInto(it->second->getSkillMap());
			}
			avVector.push_back(avEntry2);

			//avatarvector entry 3

			Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry *avEntry3 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry;
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avatarId3 = avEntry3->getAvatarId();
			avatarId3.setPersonaId(opponentPlayerId);
			avatarId3.setPlayerId(2);
			avatarId3.setSlotId(3);
			Blaze::GameReporting::SSF::CommonStatsReport &avatarStatsReport3 = avEntry3->getAvatarStatReport();
			avatarStatsReport3.setAveragePossessionLength(0);
			avatarStatsReport3.setAssists(opponentPlayerId % 10);
			//avatarStatsReport3.setBlocks(0); // missing
			avatarStatsReport3.setGoalsConceded(opponentPlayerId % 10);
			avatarStatsReport3.setHasCleanSheets(0);
			avatarStatsReport3.setCorners(opponentPlayerId % 15);
			avatarStatsReport3.setPassAttempts(opponentPlayerId % 15);
			avatarStatsReport3.setPassesMade(opponentPlayerId % 12);
			avatarStatsReport3.setRating((float)(opponentPlayerId % 10));
			avatarStatsReport3.setSaves(opponentPlayerId % 10);
			avatarStatsReport3.setShots(opponentPlayerId % 12);
			avatarStatsReport3.setTackleAttempts(opponentPlayerId % 12);
			avatarStatsReport3.setTacklesMade(opponentPlayerId % 15);
			avatarStatsReport3.setFouls(opponentPlayerId % 12);
			avatarStatsReport3.setAverageFatigueAfterNinety(0);
			avatarStatsReport3.setGoals(opponentPlayerId % 15);
			avatarStatsReport3.setInjuriesRed(0);
			avatarStatsReport3.setInterceptions(opponentPlayerId % 10);
			avatarStatsReport3.setHasMOTM(0);
			avatarStatsReport3.setOffsides(opponentPlayerId % 15);
			avatarStatsReport3.setOwnGoals(opponentPlayerId % 10);
			avatarStatsReport3.setPkGoals(opponentPlayerId % 10);
			avatarStatsReport3.setPossession(opponentPlayerId % 15);
			avatarStatsReport3.setPassesIntercepted(0);
			avatarStatsReport3.setPenaltiesAwarded(0);
			avatarStatsReport3.setPenaltiesMissed(0);
			avatarStatsReport3.setPenaltiesScored(0);
			avatarStatsReport3.setPenaltiesSaved(0);
			avatarStatsReport3.setRedCard(opponentPlayerId % 12);
			avatarStatsReport3.setShotsOnGoal(opponentPlayerId % 15);
			avatarStatsReport3.setUnadjustedScore(opponentPlayerId % 10);
			avatarStatsReport3.setYellowCard(opponentPlayerId % 12);
			if (scenarioName == VOLTA_DROPIN)
			{
				Blaze::GameReporting::SSFSeasonsReportBase::SkillTimeBucketMap &avatarSkillTime3 = avEntry3->getSkillTimeBucketMap();
				avatarSkillTime3.initMap(2);
				Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap *skillEventmap3 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap();
				Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap *skillEventmap31 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap();
				
				Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType3 = 7;
				Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType31 = 2;
				Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType32 = 7;
				
				EA::TDF::tdf_ptr < Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent3 = FillSkillEvent(1, 0, 0);
				EA::TDF::tdf_ptr < Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent31 = FillSkillEvent(0, 1, 2);
				EA::TDF::tdf_ptr < Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent32 = FillSkillEvent(0, 2, 7);
				
				skillEventmap3->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType3, skillEvent3)));
				
				skillEventmap31->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType31, skillEvent31)));
				skillEventmap31->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType32, skillEvent32)));
				
				auto it = avatarSkillTime3.begin();
				it->first = 0;
				it->second->setTimePeriod(6);
				skillEventmap3->copyInto(it->second->getSkillMap());
				++it;
				it->first = rand() % 9;
				it->second->setTimePeriod(6);
				skillEventmap31->copyInto(it->second->getSkillMap());
			}
			avVector.push_back(avEntry3);

			//avatarvector entry 4
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry *avEntry4 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry;
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avatarId4 = avEntry4->getAvatarId();
			avatarId4.setPersonaId(opponentPlayerId);
			avatarId4.setPlayerId(3);
			avatarId4.setSlotId(0);
			Blaze::GameReporting::SSF::CommonStatsReport &avatarStatsReport4 = avEntry4->getAvatarStatReport();
			avatarStatsReport4.setAveragePossessionLength(0);
			avatarStatsReport4.setAssists(opponentPlayerId % 10);
			//avatarStatsReport4.setBlocks(0); // missing
			avatarStatsReport4.setGoalsConceded(opponentPlayerId % 10);
			avatarStatsReport4.setHasCleanSheets(0);
			avatarStatsReport4.setCorners(opponentPlayerId % 15);
			avatarStatsReport4.setPassAttempts(opponentPlayerId % 15);
			avatarStatsReport4.setPassesMade(opponentPlayerId % 12);
			avatarStatsReport4.setRating((float)(opponentPlayerId % 10));
			avatarStatsReport4.setSaves(opponentPlayerId % 10);
			avatarStatsReport4.setShots(opponentPlayerId % 12);
			avatarStatsReport4.setTackleAttempts(opponentPlayerId % 12);
			avatarStatsReport4.setTacklesMade(opponentPlayerId % 15);
			avatarStatsReport4.setFouls(opponentPlayerId % 12);
			avatarStatsReport4.setAverageFatigueAfterNinety(0);
			avatarStatsReport4.setGoals(opponentPlayerId % 15);
			avatarStatsReport4.setInjuriesRed(0);
			avatarStatsReport4.setInterceptions(opponentPlayerId % 10);
			avatarStatsReport4.setHasMOTM(0);
			avatarStatsReport4.setOffsides(opponentPlayerId % 15);
			avatarStatsReport4.setOwnGoals(opponentPlayerId % 10);
			avatarStatsReport4.setPkGoals(opponentPlayerId % 10);
			avatarStatsReport4.setPossession(opponentPlayerId % 15);
			avatarStatsReport4.setPassesIntercepted(0);
			avatarStatsReport4.setPenaltiesAwarded(0);
			avatarStatsReport4.setPenaltiesMissed(0);
			avatarStatsReport4.setPenaltiesScored(0);
			avatarStatsReport4.setPenaltiesSaved(0);
			avatarStatsReport4.setRedCard(opponentPlayerId % 12);
			avatarStatsReport4.setShotsOnGoal(opponentPlayerId % 15);
			avatarStatsReport4.setUnadjustedScore(opponentPlayerId % 10);
			avatarStatsReport4.setYellowCard(opponentPlayerId % 12);
			
			if (scenarioName != VOLTA_DROPIN && scenarioName != VOLTA_PWF)
			{
				Blaze::GameReporting::SSFSeasonsReportBase::SkillTimeBucketMap &avatarSkillTime4 = avEntry4->getSkillTimeBucketMap();
				avatarSkillTime4.initMap(1);
				Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap *skillEventmap4 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap();
				Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType4 = 40;
				EA::TDF::tdf_ptr < Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent4 = FillSkillEvent(0, 1, 40);
				skillEventmap4->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType4, skillEvent4)));
				auto it = avatarSkillTime4.begin();
				it->first = 0;
				it->second->setTimePeriod(0);
				skillEventmap4->copyInto(it->second->getSkillMap());
			}
			avVector.push_back(avEntry4);

			//avatarvector entry 5
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry *avEntry5 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry;
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avatarId5 = avEntry5->getAvatarId();
			avatarId5.setPersonaId(opponentPlayerId);
			avatarId5.setPlayerId(4);
			avatarId5.setSlotId(4);
			Blaze::GameReporting::SSF::CommonStatsReport &avatarStatsReport5 = avEntry5->getAvatarStatReport();
			avatarStatsReport5.setAveragePossessionLength(0);
			avatarStatsReport5.setAssists(opponentPlayerId % 10);
			//avatarStatsReport5.setBlocks(0); // missing
			avatarStatsReport5.setGoalsConceded(opponentPlayerId % 10);
			avatarStatsReport5.setHasCleanSheets(0);
			avatarStatsReport5.setCorners(opponentPlayerId % 15);
			avatarStatsReport5.setPassAttempts(opponentPlayerId % 15);
			avatarStatsReport5.setPassesMade(opponentPlayerId % 12);
			avatarStatsReport5.setRating((float)(opponentPlayerId % 10));
			avatarStatsReport5.setSaves(opponentPlayerId % 10);
			avatarStatsReport5.setShots(opponentPlayerId % 12);
			avatarStatsReport5.setTackleAttempts(opponentPlayerId % 12);
			avatarStatsReport5.setTacklesMade(opponentPlayerId % 15);
			avatarStatsReport5.setFouls(opponentPlayerId % 12);
			avatarStatsReport5.setAverageFatigueAfterNinety(0);
			avatarStatsReport5.setGoals(opponentPlayerId % 15);
			avatarStatsReport5.setInjuriesRed(0);
			avatarStatsReport5.setInterceptions(opponentPlayerId % 10);
			avatarStatsReport5.setHasMOTM(0);
			avatarStatsReport5.setOffsides(opponentPlayerId % 15);
			avatarStatsReport5.setOwnGoals(opponentPlayerId % 10);
			avatarStatsReport5.setPkGoals(opponentPlayerId % 10);
			avatarStatsReport5.setPossession(opponentPlayerId % 15);
			avatarStatsReport5.setPassesIntercepted(0);
			avatarStatsReport5.setPenaltiesAwarded(0);
			avatarStatsReport5.setPenaltiesMissed(0);
			avatarStatsReport5.setPenaltiesScored(0);
			avatarStatsReport5.setPenaltiesSaved(0);
			avatarStatsReport5.setRedCard(opponentPlayerId % 12);
			avatarStatsReport5.setShotsOnGoal(opponentPlayerId % 15);
			avatarStatsReport5.setUnadjustedScore(opponentPlayerId % 10);
			avatarStatsReport5.setYellowCard(opponentPlayerId % 12);
			if (scenarioName == VOLTA_DROPIN && scenarioName != VOLTA_PWF)
			{
				Blaze::GameReporting::SSFSeasonsReportBase::SkillTimeBucketMap &avatarSkillTime5 = avEntry5->getSkillTimeBucketMap();
				avatarSkillTime5.initMap(1);
				Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap *skillEventmap5 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap();
				Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType4 = 40;
				EA::TDF::tdf_ptr < Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent4 = FillSkillEvent(0, 1, 40);
				skillEventmap5->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType4, skillEvent4)));
				auto it = avatarSkillTime5.begin();
				it->first = 0;
				it->second->setTimePeriod(0);
				skillEventmap5->copyInto(it->second->getSkillMap());
			}
			avVector.push_back(avEntry5);

			Blaze::GameReporting::SSF::CommonStatsReport& commonTeamReport = teamReport1->getCommonTeamReport();
			commonTeamReport.setAveragePossessionLength(0);
			commonTeamReport.setAssists(opponentPlayerId % 10);
			commonTeamReport.setBlocks(0); // Added as per logs
			commonTeamReport.setGoalsConceded(opponentPlayerId % 10);
			commonTeamReport.setHasCleanSheets(0);
			commonTeamReport.setCorners(opponentPlayerId % 15);
			commonTeamReport.setPassAttempts(opponentPlayerId % 15);
			commonTeamReport.setPassesMade(opponentPlayerId % 12);
			commonTeamReport.setRating((float)(opponentPlayerId % 10));
			commonTeamReport.setSaves(opponentPlayerId % 10);
			commonTeamReport.setShots(opponentPlayerId % 12);
			commonTeamReport.setTackleAttempts(opponentPlayerId % 12);
			commonTeamReport.setTacklesMade(opponentPlayerId % 15);
			commonTeamReport.setFouls(opponentPlayerId % 12);
			commonTeamReport.setAverageFatigueAfterNinety(0);
			commonTeamReport.setGoals(opponentPlayerId % 15);
			commonTeamReport.setInjuriesRed(0);
			commonTeamReport.setInterceptions(opponentPlayerId % 10);
			commonTeamReport.setHasMOTM(0);
			commonTeamReport.setOffsides(opponentPlayerId % 15);
			commonTeamReport.setOwnGoals(opponentPlayerId % 10);
			commonTeamReport.setPkGoals(opponentPlayerId % 10);
			commonTeamReport.setPossession(opponentPlayerId % 15);
			commonTeamReport.setPassesIntercepted(0);
			commonTeamReport.setPenaltiesAwarded(0);
			commonTeamReport.setPenaltiesMissed(0);
			commonTeamReport.setPenaltiesScored(0);
			commonTeamReport.setPenaltiesSaved(0);
			commonTeamReport.setRedCard(opponentPlayerId % 12);
			commonTeamReport.setShotsOnGoal(opponentPlayerId % 15);
			commonTeamReport.setUnadjustedScore(opponentPlayerId % 10);
			commonTeamReport.setYellowCard(opponentPlayerId % 12);
			ssfTeamReportMap[opponentPlayerId] = teamReport1;

			Blaze::GameReporting::SSFSeasonsReportBase::SSFTeamReport *teamReport2 = ssfTeamReportMap.allocate_element();
			teamReport2->setSquadDisc(0);
			teamReport2->setHome(true);
			teamReport2->setLosses(0);
			teamReport2->setGameResult(0);
			teamReport2->setScore(1);
			teamReport2->setTies(0);
			teamReport2->setSkillMoveCount(0); //missing
			teamReport2->setWinnerByDnf(0);
			teamReport2->setWins(0);
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarVector& avVector2 = teamReport2->getAvatarVector();
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry *avEntry11 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry;
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avatarId11 = avEntry11->getAvatarId();
			avatarId11.setPersonaId(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
			avatarId11.setPlayerId(0);
			avatarId11.setSlotId(1);
			Blaze::GameReporting::SSF::CommonStatsReport &avatarStatsReport11 = avEntry11->getAvatarStatReport();
			avatarStatsReport11.setAveragePossessionLength(0);
			avatarStatsReport11.setAssists(rand() % 10);
			//avatarStatsReport11.setBlocks(0); // missing
			avatarStatsReport11.setGoalsConceded(rand() % 10);
			avatarStatsReport11.setHasCleanSheets(0);
			avatarStatsReport11.setCorners(rand() % 15);
			avatarStatsReport11.setPassAttempts(rand() % 15);
			avatarStatsReport11.setPassesMade(rand() % 12);
			avatarStatsReport11.setRating((float)(rand() % 10));
			avatarStatsReport11.setSaves(rand() % 10);
			avatarStatsReport11.setShots(rand() % 12);
			avatarStatsReport11.setTackleAttempts(rand() % 12);
			avatarStatsReport11.setTacklesMade(rand() % 15);
			avatarStatsReport11.setFouls(rand() % 12);
			avatarStatsReport11.setAverageFatigueAfterNinety(0);
			avatarStatsReport11.setGoals(rand() % 15);
			avatarStatsReport11.setInjuriesRed(0);
			avatarStatsReport11.setInterceptions(rand() % 10);
			avatarStatsReport11.setHasMOTM(0);
			avatarStatsReport11.setOffsides(rand() % 15);
			avatarStatsReport11.setOwnGoals(rand() % 10);
			avatarStatsReport11.setPkGoals(rand() % 10);
			avatarStatsReport11.setPossession(rand() % 15);
			avatarStatsReport11.setPassesIntercepted(0);
			avatarStatsReport11.setPenaltiesAwarded(0);
			avatarStatsReport11.setPenaltiesMissed(0);
			avatarStatsReport11.setPenaltiesScored(0);
			avatarStatsReport11.setPenaltiesSaved(0);
			avatarStatsReport11.setRedCard(rand() % 12);
			avatarStatsReport11.setShotsOnGoal(rand() % 15);
			avatarStatsReport11.setUnadjustedScore(rand() % 10);
			avatarStatsReport11.setYellowCard(rand() % 12);
			
			if (scenarioName != VOLTA_DROPIN && scenarioName != VOLTA_PWF)
			{
				Blaze::GameReporting::SSFSeasonsReportBase::SkillTimeBucketMap &avatarSkillTime11 = avEntry11->getSkillTimeBucketMap();
				avatarSkillTime11.initMap(1);
				Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap *skillEventmap11 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap();
				Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType11 = 40;
				EA::TDF::tdf_ptr < Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent11 = FillSkillEvent(0, 1, 40);
				skillEventmap11->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType11, skillEvent11)));
				auto it = avatarSkillTime11.begin();
				it->first = 1;
				it->second->setTimePeriod(1);
				skillEventmap11->copyInto(it->second->getSkillMap());
			}
			avVector2.push_back(avEntry11);

			//avatarvector entry 2
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry *avEntry12 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry;
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avatarId12 = avEntry12->getAvatarId();
			avatarId12.setPersonaId(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
			avatarId12.setPlayerId(1);
			avatarId12.setSlotId(2);
			Blaze::GameReporting::SSF::CommonStatsReport &avatarStatsReport12 = avEntry12->getAvatarStatReport();
			avatarStatsReport12.setAveragePossessionLength(0);
			avatarStatsReport12.setAssists(rand() % 10);
			//avatarStatsReport12.setBlocks(0); // missing
			avatarStatsReport12.setGoalsConceded(rand() % 10);
			avatarStatsReport12.setHasCleanSheets(0);
			avatarStatsReport12.setCorners(rand() % 15);
			avatarStatsReport12.setPassAttempts(rand() % 15);
			avatarStatsReport12.setPassesMade(rand() % 12);
			avatarStatsReport12.setRating((float)(rand() % 10));
			avatarStatsReport12.setSaves(rand() % 10);
			avatarStatsReport12.setShots(rand() % 12);
			avatarStatsReport12.setTackleAttempts(rand() % 12);
			avatarStatsReport12.setTacklesMade(rand() % 15);
			avatarStatsReport12.setFouls(rand() % 12);
			avatarStatsReport12.setAverageFatigueAfterNinety(0);
			avatarStatsReport12.setGoals(rand() % 15);
			avatarStatsReport12.setInjuriesRed(0);
			avatarStatsReport12.setInterceptions(rand() % 10);
			avatarStatsReport12.setHasMOTM(0);
			avatarStatsReport12.setOffsides(rand() % 15);
			avatarStatsReport12.setOwnGoals(rand() % 10);
			avatarStatsReport12.setPkGoals(rand() % 10);
			avatarStatsReport12.setPossession(rand() % 15);
			avatarStatsReport12.setPassesIntercepted(0);
			avatarStatsReport12.setPenaltiesAwarded(0);
			avatarStatsReport12.setPenaltiesMissed(0);
			avatarStatsReport12.setPenaltiesScored(0);
			avatarStatsReport12.setPenaltiesSaved(0);
			avatarStatsReport12.setRedCard(rand() % 12);
			avatarStatsReport12.setShotsOnGoal(rand() % 15);
			avatarStatsReport12.setUnadjustedScore(rand() % 10);
			avatarStatsReport12.setYellowCard(rand() % 12);
			avVector2.push_back(avEntry12);

			//avatarvector entry 3
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry *avEntry13 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry;
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avatarId13 = avEntry13->getAvatarId();
			avatarId13.setPersonaId(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
			avatarId13.setPlayerId(2);
			avatarId13.setSlotId(3);
			Blaze::GameReporting::SSF::CommonStatsReport &avatarStatsReport13 = avEntry13->getAvatarStatReport();
			avatarStatsReport13.setAveragePossessionLength(0);
			avatarStatsReport13.setAssists(rand() % 10);
			//avatarStatsReport13.setBlocks(0); // missing
			avatarStatsReport13.setGoalsConceded(rand() % 10);
			avatarStatsReport13.setHasCleanSheets(0);
			avatarStatsReport13.setCorners(rand() % 15);
			avatarStatsReport13.setPassAttempts(rand() % 15);
			avatarStatsReport13.setPassesMade(rand() % 12);
			avatarStatsReport13.setRating((float)(rand() % 10));
			avatarStatsReport13.setSaves(rand() % 10);
			avatarStatsReport13.setShots(rand() % 12);
			avatarStatsReport13.setTackleAttempts(rand() % 12);
			avatarStatsReport13.setTacklesMade(rand() % 15);
			avatarStatsReport13.setFouls(rand() % 12);
			avatarStatsReport13.setAverageFatigueAfterNinety(0);
			avatarStatsReport13.setGoals(rand() % 15);
			avatarStatsReport13.setInjuriesRed(0);
			avatarStatsReport13.setInterceptions(rand() % 10);
			avatarStatsReport13.setHasMOTM(0);
			avatarStatsReport13.setOffsides(rand() % 15);
			avatarStatsReport13.setOwnGoals(rand() % 10);
			avatarStatsReport13.setPkGoals(rand() % 10);
			avatarStatsReport13.setPossession(rand() % 15);
			avatarStatsReport13.setPassesIntercepted(0);
			avatarStatsReport13.setPenaltiesAwarded(0);
			avatarStatsReport13.setPenaltiesMissed(0);
			avatarStatsReport13.setPenaltiesScored(0);
			avatarStatsReport13.setPenaltiesSaved(0);
			avatarStatsReport13.setRedCard(rand() % 12);
			avatarStatsReport13.setShotsOnGoal(rand() % 15);
			avatarStatsReport13.setUnadjustedScore(rand() % 10);
			avatarStatsReport13.setYellowCard(rand() % 12);
			avVector2.push_back(avEntry13);

			//avatarvector entry 4
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry *avEntry14 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry;
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avatarId14 = avEntry14->getAvatarId();
			avatarId14.setPersonaId(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
			avatarId14.setPlayerId(3);
			avatarId14.setSlotId(0);
			Blaze::GameReporting::SSF::CommonStatsReport &avatarStatsReport14 = avEntry14->getAvatarStatReport();
			avatarStatsReport14.setAveragePossessionLength(0);
			avatarStatsReport14.setAssists(rand() % 10);
			//avatarStatsReport14.setBlocks(0); // missing
			avatarStatsReport14.setGoalsConceded(rand() % 10);
			avatarStatsReport14.setHasCleanSheets(0);
			avatarStatsReport14.setCorners(rand() % 15);
			avatarStatsReport14.setPassAttempts(rand() % 15);
			avatarStatsReport14.setPassesMade(rand() % 12);
			avatarStatsReport14.setRating((float)(rand() % 10));
			avatarStatsReport14.setSaves(rand() % 10);
			avatarStatsReport14.setShots(rand() % 12);
			avatarStatsReport14.setTackleAttempts(rand() % 12);
			avatarStatsReport14.setTacklesMade(rand() % 15);
			avatarStatsReport14.setFouls(rand() % 12);
			avatarStatsReport14.setAverageFatigueAfterNinety(0);
			avatarStatsReport14.setGoals(rand() % 15);
			avatarStatsReport14.setInjuriesRed(0);
			avatarStatsReport14.setInterceptions(rand() % 10);
			avatarStatsReport14.setHasMOTM(0);
			avatarStatsReport14.setOffsides(rand() % 15);
			avatarStatsReport14.setOwnGoals(rand() % 10);
			avatarStatsReport14.setPkGoals(rand() % 10);
			avatarStatsReport14.setPossession(rand() % 15);
			avatarStatsReport14.setPassesIntercepted(0);
			avatarStatsReport14.setPenaltiesAwarded(0);
			avatarStatsReport14.setPenaltiesMissed(0);
			avatarStatsReport14.setPenaltiesScored(0);
			avatarStatsReport14.setPenaltiesSaved(0);
			avatarStatsReport14.setRedCard(rand() % 12);
			avatarStatsReport14.setShotsOnGoal(rand() % 15);
			avatarStatsReport14.setUnadjustedScore(rand() % 10);
			avatarStatsReport14.setYellowCard(rand() % 12);
			
			if (scenarioName != VOLTA_DROPIN && scenarioName != VOLTA_PWF)
			{
				Blaze::GameReporting::SSFSeasonsReportBase::SkillTimeBucketMap &avatarSkillTime14 = avEntry14->getSkillTimeBucketMap();
				avatarSkillTime14.initMap(1);
				Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap *skillEventmap14 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap();
				Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType14 = 22;
				EA::TDF::tdf_ptr < Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent14 = FillSkillEvent(0, 1, 22);
				skillEventmap14->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType14, skillEvent14)));
				auto it = avatarSkillTime14.begin();
				it->first = 0;
				it->second->setTimePeriod(0);
				skillEventmap14->copyInto(it->second->getSkillMap());
			}
			avVector2.push_back(avEntry14);

			//avatarvector entry 5
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry *avEntry15 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry;
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avatarId15 = avEntry15->getAvatarId();
			avatarId15.setPersonaId(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
			avatarId15.setPlayerId(3);
			avatarId15.setSlotId(0);
			Blaze::GameReporting::SSF::CommonStatsReport &avatarStatsReport15 = avEntry15->getAvatarStatReport();
			avatarStatsReport15.setAveragePossessionLength(0);
			avatarStatsReport15.setAssists(rand() % 10);
			//avatarStatsReport15.setBlocks(0); // missing
			avatarStatsReport15.setGoalsConceded(rand() % 10);
			avatarStatsReport15.setHasCleanSheets(0);
			avatarStatsReport15.setCorners(rand() % 15);
			avatarStatsReport15.setPassAttempts(rand() % 15);
			avatarStatsReport15.setPassesMade(rand() % 12);
			avatarStatsReport15.setRating((float)(rand() % 10));
			avatarStatsReport15.setSaves(rand() % 10);
			avatarStatsReport15.setShots(rand() % 12);
			avatarStatsReport15.setTackleAttempts(rand() % 12);
			avatarStatsReport15.setTacklesMade(rand() % 15);
			avatarStatsReport15.setFouls(rand() % 12);
			avatarStatsReport15.setAverageFatigueAfterNinety(0);
			avatarStatsReport15.setGoals(rand() % 15);
			avatarStatsReport15.setInjuriesRed(0);
			avatarStatsReport15.setInterceptions(rand() % 10);
			avatarStatsReport15.setHasMOTM(0);
			avatarStatsReport15.setOffsides(rand() % 15);
			avatarStatsReport15.setOwnGoals(rand() % 10);
			avatarStatsReport15.setPkGoals(rand() % 10);
			avatarStatsReport15.setPossession(rand() % 15);
			avatarStatsReport15.setPassesIntercepted(0);
			avatarStatsReport15.setPenaltiesAwarded(0);
			avatarStatsReport15.setPenaltiesMissed(0);
			avatarStatsReport15.setPenaltiesScored(0);
			avatarStatsReport15.setPenaltiesSaved(0);
			avatarStatsReport15.setRedCard(rand() % 12);
			avatarStatsReport15.setShotsOnGoal(rand() % 15);
			avatarStatsReport15.setUnadjustedScore(rand() % 10);
			avatarStatsReport15.setYellowCard(rand() % 12);
			
			if (scenarioName != VOLTA_DROPIN && scenarioName != VOLTA_PWF)
			{
				Blaze::GameReporting::SSFSeasonsReportBase::SkillTimeBucketMap &avatarSkillTime15 = avEntry15->getSkillTimeBucketMap();
				avatarSkillTime15.initMap(1);
				Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap *skillEventmap15 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap();
				Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType15 = 40;
				EA::TDF::tdf_ptr < Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent15 = FillSkillEvent(0, 1, 40);
				skillEventmap15->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType15, skillEvent15)));
				auto it = avatarSkillTime15.begin();
				it->first = 1;
				it->second->setTimePeriod(1);
				skillEventmap15->copyInto(it->second->getSkillMap());
			}
			avVector2.push_back(avEntry15);

			Blaze::GameReporting::SSF::CommonStatsReport& commonTeamReport2 = teamReport2->getCommonTeamReport();
			commonTeamReport2.setAveragePossessionLength(0);
			commonTeamReport2.setAssists(rand() % 10);
			//commonTeamReport2.setBlocks(0); // missing
			commonTeamReport2.setGoalsConceded(rand() % 10);
			commonTeamReport2.setHasCleanSheets(0);
			commonTeamReport2.setCorners(rand() % 15);
			commonTeamReport2.setPassAttempts(rand() % 15);
			commonTeamReport2.setPassesMade(rand() % 12);
			commonTeamReport2.setRating((float)(rand() % 10));
			commonTeamReport2.setSaves(rand() % 10);
			commonTeamReport2.setShots(rand() % 12);
			commonTeamReport2.setTackleAttempts(rand() % 12);
			commonTeamReport2.setTacklesMade(rand() % 15);
			commonTeamReport2.setFouls(rand() % 12);
			commonTeamReport2.setAverageFatigueAfterNinety(0);
			commonTeamReport2.setGoals(rand() % 15);
			commonTeamReport2.setInjuriesRed(0);
			commonTeamReport2.setInterceptions(rand() % 10);
			commonTeamReport2.setHasMOTM(0);
			commonTeamReport2.setOffsides(rand() % 15);
			commonTeamReport2.setOwnGoals(rand() % 10);
			commonTeamReport2.setPkGoals(rand() % 10);
			commonTeamReport2.setPossession(rand() % 15);
			commonTeamReport2.setPassesIntercepted(0);
			commonTeamReport2.setPenaltiesAwarded(0);
			commonTeamReport2.setPenaltiesMissed(0);
			commonTeamReport2.setPenaltiesScored(0);
			commonTeamReport2.setPenaltiesSaved(0);
			commonTeamReport2.setRedCard(rand() % 12);
			commonTeamReport2.setShotsOnGoal(rand() % 15);
			commonTeamReport2.setUnadjustedScore(rand() % 10);
			commonTeamReport2.setYellowCard(rand() % 12);
			ssfTeamReportMap[mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId()] = teamReport2;

			osdkReport->setTeamReports(*ssfTeamSummaryReport);

			//if (BLAZE_IS_TRACE_RPC_ENABLED(BlazeRpcLog::gamemanager)) {
				//char8_t buf[40640];
				//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "-> [SsfSeasonsGameSession][" << mPlayerData->getPlayerId() << "]::submitGameReport" << "\n" << request.print(buf, sizeof(buf)));
			//}
			err = mPlayerData->getComponentProxy()->mGameReportingProxy->submitGameReport(request);
			if (err == ERR_OK)
			{
				BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "SSF Player::SubmitGamerport successful. mMatchmakingSessionId= " << mMatchmakingSessionId);
			}
			else
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[SSF Player::submitGameReport failed][" << mPlayerData->getPlayerId() << " gameId " << myGameId << err);
			}
			// delete osdkReport;
			// delete gamereport;
			// gamereport = NULL;
			return err;
		}

		void SsfSeasonsGameSession::addFriendToList(BlazeId playerId)
		{
			VoltaFriendsList* friendsListReference = ((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->getVoltaFriendsList();
			VoltaFriendsList::iterator it = friendsListReference->begin();
			while (it != friendsListReference->end())
			{
				if (playerId == *it)
				{
					return;
				}
				it++;
			}
			friendsListReference->push_back(playerId);

		}

		BlazeId SsfSeasonsGameSession::getARandomFriend()
		{
			VoltaFriendsList* friendsListReference = ((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->getVoltaFriendsList();
			if (friendsListReference->size() == 0)
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::getARandomFriend] The gameId map is empty " << mPlayerData->getPlayerId());
				return 0;
			}
			VoltaFriendsList::iterator it = friendsListReference->begin();
			BlazeId randomPlayerId; int32_t num = 0;

			mMutex.Lock();
			num = Random::getRandomNumber(friendsListReference->size());
			eastl::advance(it, num);
			randomPlayerId = *it;
			friendsListReference->erase(it);
			mMutex.Unlock();
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SsfSeasonsGameSession::getARandomFriend]  Randomly fetched playerID : " << randomPlayerId << " " << mPlayerData->getPlayerId());
			return randomPlayerId;
		}

		void SsfSeasonsGameSession::addVoltaFriendsToMap(BlazeId hostId, BlazeId friendID)
		{
			VoltaFriendsMap& friendsMap = ((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->getVoltaFriendsMap();
			friendsMap[hostId] = friendID;
			friendsMap[friendID] = hostId;
		}

		BlazeId SsfSeasonsGameSession::getFriendInMap(BlazeId playerID)
		{
			VoltaFriendsMap& friendsMap = ((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->getVoltaFriendsMap();
			if (friendsMap.find(playerID) == friendsMap.end())
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "SsfSeasonsGameSession::getMyFriendInMap FriendId not found " << mPlayerData->getPlayerId());
				return 0;
			}
			return friendsMap[playerID];
		}

		void SsfSeasonsGameSession::removeFriendFromMap()
		{
			VoltaFriendsMap& friendsMap = ((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->getVoltaFriendsMap();
			VoltaFriendsMap::iterator friendsData = friendsMap.find(mPlayerData->getPlayerId());
			if (friendsData != friendsMap.end())
			{
				friendsMap.erase(friendsData);
			}
		}


	}  // namespace Stress
}  // namespace Blaze
