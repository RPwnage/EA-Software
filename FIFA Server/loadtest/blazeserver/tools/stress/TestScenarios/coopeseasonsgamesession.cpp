//  *************************************************************************************************
//
//   File:    coopseasonsgamesession.cpp
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
#include "./coopseasonsgamesession.h"
#include "./utility.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/connection/selector.h"
#include "framework/config/config_file.h"
#include "framework/config/config_map.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/util/random.h"
#include "framework/tdf/usersessiontypes_server.h"
#include "framework/rpc/usersessionsslave.h"
#include "coopseason/tdf/coopseasontypes.h"


using namespace Blaze::GameManager;
using namespace Blaze::GameReporting;

namespace Blaze {
	namespace Stress {

		CoopSeasonsGameSession::CoopSeasonsGameSession(StressPlayerInfo* playerData, uint32_t GameSize, GameNetworkTopology Topology, GameProtocolVersionString GameProtocolVersionString)
			: GameSession(playerData, GameSize, Topology, GameProtocolVersionString),
			mFriendBlazeId(0), mGameGroupId(0), mCoopId(0), isGrupPlatformHost(false)
		{
			mGameReportingId = 0;
			mPlayerIds.push_back(0);
		}

		CoopSeasonsGameSession::~CoopSeasonsGameSession()
		{
			BLAZE_INFO(BlazeRpcLog::util, "[CoopSeasonsGameSession][Destructor][%" PRIu64 "]: destroy called", mPlayerData->getPlayerId());
		}

		void CoopSeasonsGameSession::asyncHandlerFunction(uint16_t type, EA::TDF::Tdf *notification)
		{
			if (mPlayerData != NULL && mPlayerData->isPlayerActive())
			{
				//  This Notification has not been handled so use the Default Handler
				onGameSessionDefaultAsyncHandler(type, notification);
			}
		}

		void CoopSeasonsGameSession::onGameSessionDefaultAsyncHandler(uint16_t type, EA::TDF::Tdf* notification)
		{
			//  This is abstract class and handle the async handlers in the derived class

			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]:Message Type =" << type);
			switch (type)
			{
			case GameManagerSlave::NOTIFY_NOTIFYGAMESETUP:
				//  Async notification sent to the player who joined a game.  Triggered by JoinGame Request or a Matchmaking session finding a game.
			{
				NotifyGameSetup* notify = (NotifyGameSetup*)notification;

				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession::asyncHandlerFunction]: Received  NOTIFY_NOTIFYGAMESETUP." << mPlayerData->getPlayerId());
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
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMESTATECHANGE, state=" <<
					notify->getNewGameState());
				/*char8_t buf[10240];
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "NOTIFY_NOTIFYGAMESTATECHANGE :" << "\n" << notify->print(buf, sizeof(buf)));*/
				if (myGameId == notify->getGameId())
				{
					if (getPlayerState() == IN_GAME && notify->getNewGameState() == POST_GAME)
					{
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession::NOTIFY_NOTIFYGAMESTATECHANGE][" << mPlayerData->getPlayerId() << "]: submitGameReport");
						submitGameReport();
						//leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT);
						leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, getGameId());
						leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGameGroupId);
					}

					//  Set Game State
					setPlayerState(notify->getNewGameState());
					CoopplayerGameDataRef(mPlayerData).setGameState(myGameId, notify->getNewGameState());
				}
			}
			break;
			case GameManagerSlave::NOTIFY_NOTIFYGAMERESET:
			{
				NotifyGameReset* notify = (NotifyGameReset*)notification;
				StressGameInfo* gameInfo = CoopplayerGameDataRef(mPlayerData).getGameData(notify->getGameData().getGameId());
				if (gameInfo != NULL && gameInfo->getPlatformHostPlayerId() == mPlayerData->getPlayerId())
				{
					//  TODO: host specific actions
				}
				BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMERESET.gameId=" <<
					notify->getGameData().getGameId());
			}
			break;
			case GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINING:
			{
				NotifyPlayerJoining* notify = (NotifyPlayerJoining*)notification;
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERJOINING.gameId=" << notify->getGameId());

				// Add player to game map
				mPlayerInGameMap.insert(notify->getJoiningPlayer().getPlayerId());
				BLAZE_TRACE_LOG(BlazeRpcLog::coopseason, "[CoopSeasonsGameSession::NOTIFYPLAYERJOINCOMPLETED:mPlayerInGameMap][ GameID : " << myGameId << " joinerId " << notify->getJoiningPlayer().getPlayerId());

				//For Game Group
				if (notify->getGameId() == mGameGroupId)
				{
					//meshEndpointsConnected
					BlazeObjectId targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, notify->getJoiningPlayer().getConnectionGroupId());
					meshEndpointsConnected(targetGroupId, 0, 0, mGameGroupId);
				}
				//For Game Session Type
				if (notify->getGameId() == myGameId)
				{
					//meshEndpointsConnected
					BlazeObjectId targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, notify->getJoiningPlayer().getConnectionGroupId());
					meshEndpointsConnected(targetGroupId, 0, 0, myGameId);
				}
			}
			break;
			case GameManagerSlave::NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS:
			{
				BLAZE_INFO(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession::handlerFunction][%" PRIu64 "]: Received  NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS.", mPlayerData->getPlayerId());

			}
			break;
			case GameManagerSlave::NOTIFY_NOTIFYPLATFORMHOSTINITIALIZED:
			{
				NotifyPlatformHostInitialized* notify = (NotifyPlatformHostInitialized*)notification;
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLATFORMHOSTINITIALIZED.gameId=" <<
					notify->getGameId());
			}
			break;
			case GameManagerSlave::NOTIFY_NOTIFYGAMELISTUPDATE:
			{
				//NotifyGameListUpdate* notify = (NotifyGameListUpdate*)notification;
				BLAZE_INFO(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession::asyncHandlerFunction][%" PRIu64 "]: Received  NOTIFY_NOTIFYGAMELISTUPDATE.", mPlayerData->getPlayerId());

			}
			break;
			case GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINCOMPLETED:
			{
				NotifyPlayerJoinCompleted* notify = (NotifyPlayerJoinCompleted*)notification;
				BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERJOINCOMPLETED.");
				//char8_t buf[20240];
				//BLAZE_TRACE_RPC_LOG(BlazeRpcLog::gamemanager, "NOTIFY_NOTIFYPLAYERJOINCOMPLETED RPC : \n" << notify->print(buf, sizeof(buf)));
				// Add player to game map
				mPlayerInGameMap.insert(notify->getPlayerId());
				//we get all the players in a coop game during joinCompleted call Back
				if (notify->getGameId() == myGameId)
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::coopseason, "[CoopSeasonsGameSession::NOTIFYPLAYERJOINCOMPLETED:mPlayerInGameMap][ GameID : " << myGameId << " joinerId " << notify->getPlayerId());

					if (IsPlayerPresentInList((PlayerModule*)(mPlayerData->getOwner()), notify->getPlayerId(), COOPPLAYER) == true)
					{
						uint16_t count = CoopplayerGameDataRef(mPlayerData).getPlayerCount(myGameId);
						CoopplayerGameDataRef(mPlayerData).setPlayerCount(myGameId, ++count);
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession::NOTIFYPLAYERJOINCOMPLETED][" << mPlayerData->getPlayerId() << "]:GameId=" << myGameId << " Player count = " << count);
					}
				}
				if (notify->getGameId() == mGameGroupId)
				{
					invokeUdatePrimaryExternalSessionForUser(mPlayerData, mGameGroupId, "", "", mGameGroupId, GAME_JOIN, GAME_TYPE_GROUP, ACTIVE_CONNECTED);
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
					BlazeObjectId targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, mTopologyHostConnectionGroupId);
					meshEndpointsDisconnected(targetGroupId, DISCONNECTED_PLAYER_REMOVED, 5, myGameId);
				}
				preferredJoinOptOut(myGameId);
				StressGameInfo* ptrGameInfo = CoopplayerGameDataRef(mPlayerData).getGameData(myGameId);
				if (!ptrGameInfo)
				{
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]::receiving notification(" << type << ") and gameId " << myGameId <<
						"not present in CoopSeasonsGamesMap");
					break;
				}

				if (myGameId == notify->getGameId() && CoopplayerGameDataRef(mPlayerData).isPlayerExistInGame(notify->getPlayerId(), notify->getGameId()) == true)
				{
					CoopplayerGameDataRef(mPlayerData).removePlayerFromGameData(notify->getPlayerId(), notify->getGameId());
					if (IsPlayerPresentInList((PlayerModule*)(mPlayerData->getOwner()), notify->getPlayerId(), COOPPLAYER) == true)
					{
						uint16_t count = ptrGameInfo->getPlayerCount();
						ptrGameInfo->setPlayerCount(--count);
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession::asyncHandlerFunction]::NOTIFYPLAYERREMOVED[" << mPlayerData->getPlayerId() << "]:GameId=" << myGameId << " Player count = " << count
							<< "for player=" << notify->getPlayerId());
					}
				}

				//  if all players, in single exe, removed from the game delete the data from map
				if (ptrGameInfo->getPlayerCount() <= 0)
				{
					BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERREMOVED - Game Id = " <<
						notify->getGameId() << ".");
					CoopplayerGameDataRef(mPlayerData).removeGameData(notify->getGameId());
				}
			}
			break;
			case GameManagerSlave::NOTIFY_NOTIFYGAMEREMOVED:
			{
				//  NotifyGameRemoved* notify = (NotifyGameRemoved*)notification;
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMEREMOVED.");
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
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[MultiPlayerGameSession::OnNotifyMatchmakingFailed][" << mPlayerData->getPlayerId() << "]: Matchmake SESSION_TIMED_OUT.");
				}
				break;
				case GameManager::SESSION_CANCELED:
				{
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::OnNotifyMatchmakingFailed][" << mPlayerData->getPlayerId() << "]: Matchmake SESSION_CANCELED.");
					//  Throw Execption;
				}
				break;
				default:
					break;
				}
				BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYMATCHMAKINGFAILED.");

				leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGameGroupId);
				//Remove friend info from the map
				removeFriendFromMap();
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

				mPlayerIds.push_back(mPlayerData->getPlayerId());
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

				/*char8_t buf[10240];
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "-> [CoopSeasonsGameSession]" << "::NOTIFY_USERADDED" << "\n" << notify->print(buf, sizeof(buf)));*/
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
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession::handlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYMESSAGE.");
				// Messaging::ServerMessage* notify = (Messaging::ServerMessage*)notification;
				 //   Handle player message notifications.
				 //   Get the message details

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

		void CoopSeasonsGameSession::handleNotifyGameSetUpGroup(NotifyGameSetup* notify, uint16_t type /* = GameManagerSlave::NOTIFY_NOTIFYGAMESETUP*/)
		{
			if (notify->getGameData().getPlatformHostInfo().getPlayerId() == mPlayerData->getPlayerId()) {
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession::handleNotifyGameSetUpGroup][" << mPlayerData->getPlayerId() << "," << notify->getGameData().getPlatformHostInfo().getPlayerId() << "}");
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

				invokeUdatePrimaryExternalSessionForUser(mPlayerData, mGameGroupId, "", "", mGameGroupId, GAME_CREATE, GAME_TYPE_GROUP, ACTIVE_CONNECTED);
			}

			updateExtendedDataAttribute(mFriendBlazeId);

			char8_t opponentIdString[MAX_GAMENAME_LEN] = { '\0' };
			char8_t playerIdString[MAX_GAMENAME_LEN] = { '\0' };
			blaze_snzprintf(playerIdString, sizeof(playerIdString), "%" PRId64, mPlayerData->getPlayerId());
			blaze_snzprintf(opponentIdString, sizeof(opponentIdString), "%" PRId64, mFriendBlazeId);
			lookupUsersIdentification(mPlayerData);
			setPlayerAttributes(playerIdString, "0", mGameGroupId);

			if (isGrupPlatformHost)
			{
				Collections::AttributeMap attributes;
				attributes["PRE_G"] = "0";
				setGameAttributes(mGameGroupId, attributes);
				attributes.clear();

				if (StressConfigInst.getPlatform() == Platform::PLATFORM_PS5 || StressConfigInst.getPlatform() == Platform::PLATFORM_XBSX)
				{
					attributes["PRE_K"] = "16384";
					attributes["PRE_T"] = "1";
				}
				else
				{
					attributes["PRE_K"] = "163843";
					attributes["PRE_T"] = "10";
				}
				setGameAttributes(mGameGroupId, attributes);
				attributes.clear();

				/*attributes[playerIdString] = "0";
				setGameAttributes(mGameGroupId, attributes);
				attributes.clear();

				attributes["LU_S"] = "105846:500:18:0|190456:925:200:3|193011:713:175:4|208920:288:175:6|190885:75:200:7|189446:925:588:12|172768:650:513:13|213991:350:513:15|207807:75:588:16|185422:500:750:21|225539:500:875:25";
				attributes["PRE_CS"] = "-1:-1|-1:-1|-1:-1";
				attributes["PRE_F"] = "8";
				attributes["PRE_K"] = "31834112";
				attributes["PRE_SC"] = "193011";
				attributes["PRE_SR"] = "221618|184749|205540|220414|169638|233201|172723|230882|230876|220196|234249|135883|196978|224294|243669|215818|243767|239676|243765|243044|243668|243048|-1";
				attributes["PRE_T"] = "1943";
				attributes["PRE_TYPE"] = "0";
				setGameAttributes(mGameGroupId, attributes);
				attributes.clear();*/

				if (StressConfigInst.getPlatform() == Platform::PLATFORM_PS5 || StressConfigInst.getPlatform() == Platform::PLATFORM_XBSX)
				{
					attributes["MM_C"] = "0";
					attributes["MM_CQM"] = "0";
					attributes["MM_CT"] = "0";
					attributes["MM_MT"] = "0";
					attributes["MM_OQP"] = "100";
					attributes["MM_TS"] = "0";
					attributes["PRE_G"] = "0";
					attributes["PRE_M"] = "0";
					attributes["PRE_N"] = "0";
				}
				else
				{
					attributes["MM_C"] = "0";
					attributes["MM_CQM"] = "0";
					attributes["MM_CT"] = "0";
					attributes["MM_MT"] = "0";
					attributes["MM_OQP"] = "100";
					attributes["MM_TS"] = "0";
					attributes["PRE_M"] = "0";
					attributes["PRE_N"] = "0";
				}
				setGameAttributes(mGameGroupId, attributes);
				attributes.clear();

				attributes[playerIdString] = "0";
				setGameAttributes(mGameGroupId, attributes);
				attributes.clear();
				attributes["LU_S"] = "198355:500:18:0|225880:925:200:3|181404:675:150:4|215823:325:150:6|184643:75:200:7|203067:650:338:9|237203:350:338:11|223637:925:588:12|190577:75:588:16|172114:500:663:18|213620:500:875:25";
				attributes["MM_C"] = "0";
				attributes["MM_CQM"] = "0";
				attributes["MM_CT"] = "0";
				attributes["MM_MT"] = "0";
				attributes["MM_OQP"] = "100";
				attributes["MM_TS"] = "0";
				attributes["PRE_CS"] = "-1:-1|-1:-1|-1:-1";
				attributes["PRE_F"] = "3";
				attributes["PRE_M"] = "0";
				attributes["PRE_N"] = "0";
				attributes["PRE_SC"] = "172114";
				attributes["PRE_SR"] = "237256|236507|217190|229588|214354|236587|246958|214132|242503|242533|246951|201875|232094|220886|255887|-1";
				attributes["PRE_TYPE"] = "0";
				setGameAttributes(mGameGroupId, attributes);
				attributes.clear();
				attributes["MM_C"] = "0";
				attributes["MM_CQM"] = "0";
				attributes["MM_CT"] = "0";
				attributes["MM_MT"] = "0";
				attributes["MM_OQP"] = "100";
				attributes["MM_TS"] = "0";
				attributes["PRE_M"] = "0";
				attributes["PRE_N"] = "0";
				setGameAttributes(mGameGroupId, attributes);
				lookupUsersByPersonaNames(mPlayerData, "");
				attributes.clear();
				attributes["PRE_N"] = "1";
				setGameAttributes(mGameGroupId, attributes);
				attributes.clear();
				attributes["LU_S"] = "0";
				attributes["PRE_CS"] = "0";
				attributes["PRE_K"] = "3932160";
				attributes["PRE_N"] = "0";
				attributes["PRE_SR"] = "0";
				attributes["PRE_T"] = "240";
				setGameAttributes(mGameGroupId, attributes);
				attributes.clear();
				attributes["LU_S"] = "200389:500:18:0|186345:913:225:3|207863:666:152:4|216460:328:156:6|251573:99:225:7|214997:900:625:12|193747:640:509:13|209989:360:475:15|208421:100:601:16|201153:601:850:24|242444:386:850:26";
				attributes["PRE_F"] = "11";
				attributes["PRE_SC"] = "193747";
				attributes["PRE_SR"] = "208418|156519|213565|199715|226161|204639|178086|179844|229668|203890|204259|232207|252324|248805|243652|252326|252327|243080|254860|241588|236940|243308|-1";
				setGameAttributes(mGameGroupId, attributes);
				userSettingsSave(mPlayerData, "", "");
				attributes.clear();
				attributes["PRE_I"] = "0";
				setGameAttributes(mGameGroupId, attributes);
				attributes.clear();
				attributes["PRE_I"] = "0";
				setGameAttributes(mGameGroupId, attributes);
				attributes.clear();
				setGameAttributes(mGameGroupId, attributes);
			}
			mPlayerInGameMap.insert(mPlayerData->getPlayerId());
			//mPlayerInGameMap.insert(notify->getGameData().getPlatformHostInfo().getPlayerId());
		}

		void CoopSeasonsGameSession::handleNotifyGameSetUp(NotifyGameSetup* notify, uint16_t type /* = GameManagerSlave::NOTIFY_NOTIFYGAMESETUP*/)
		{
			BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId());
			char8_t homeCaptain[MAX_GAMENAME_LEN] = { '\0' };
			char8_t homeCoopId[MAX_GAMENAME_LEN] = { '\0' };
			blaze_snzprintf(homeCoopId, sizeof(homeCoopId), "%" PRId64, mCoopId);
			if (notify->getGameData().getTopologyHostInfo().getPlayerId() == mPlayerData->getPlayerId())
			{
				isTopologyHost = true;
			}
			if (notify->getGameData().getPlatformHostInfo().getPlayerId() == mPlayerData->getPlayerId())
			{
				isPlatformHost = true;
				blaze_snzprintf(homeCaptain, sizeof(homeCaptain), "%" PRId64, mPlayerData->getPlayerId());
			}
			/*****  Add Game Data *****/
			//  Set My Game ID
			setGameId(notify->getGameData().getGameId());
			// Read connection group ID 
			mTopologyHostConnectionGroupId = notify->getGameData().getTopologyHostInfo().getConnectionGroupId();

			//  Set Game State
			setPlayerState(notify->getGameData().getGameState());

			if (CoopplayerGameDataRef(mPlayerData).isGameIdExist(myGameId))
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "]::receiving notification(" << type << ") and gameId " <<
					notify->getGameData().getGameId() << "already present in CoopSeasonsGamesMap");
				//  Player will be added during NOTIFYPLAYERJOINCOMPLETED notification, not here.
			}
			else
			{
				//  create new game data
				CoopplayerGameDataRef(mPlayerData).addGameData(notify);
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
					//mPlayerInGameMap.insert(playerId);
					if (playerId != mPlayerData->getPlayerId())
					{
						//meshEndpointsConnected RPC other players in roster
						BlazeObjectId connId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, (*cit)->getConnectionGroupId());
						meshEndpointsConnected(connId, 0, 0, myGameId);
					}
				}
			}
			setPlayerAttributes("REQ", "2");
			if (isPlatformHost)
			{
				Blaze::BlazeId homesCaptain = 0;
				Blaze::BlazeId homePlayer = 0;
				Blaze::BlazeId awayCaptain = 0;
				Blaze::BlazeId awayOther = 0;
				Collections::AttributeMap gameAttributes;
				gameAttributes.clear();
				gameAttributes["PRE_S"] = "0";
				setGameAttributes(myGameId, gameAttributes);
				gameAttributes.clear();
				if (mPlayerIds.size() >= 3)
				{
					homesCaptain = mPlayerIds[1];
					homePlayer = mPlayerIds[2];
				}
				if (mPlayerIds.size() >= 5)
				{
					awayCaptain = mPlayerIds.at(3);
					awayOther = mPlayerIds.at(4);
				}
				char8_t homeCaptainString[MAX_GAMENAME_LEN] = { '\0' };
				char8_t homePlayerString[MAX_GAMENAME_LEN] = { '\0' };
				char8_t awayCaptainString[MAX_GAMENAME_LEN] = { '\0' };
				char8_t awayOtherString[MAX_GAMENAME_LEN] = { '\0' };
				char8_t homeGameIdString[MAX_GAMENAME_LEN] = { '\0' };
				blaze_snzprintf(homeCaptainString, sizeof(homeCaptainString), "%" PRId64, homesCaptain);
				blaze_snzprintf(homePlayerString, sizeof(homePlayerString), "%" PRId64, homePlayer);
				blaze_snzprintf(awayCaptainString, sizeof(awayCaptainString), "%" PRId64, awayCaptain);
				blaze_snzprintf(awayOtherString, sizeof(awayOtherString), "%" PRId64, awayOther);
				blaze_snzprintf(homeGameIdString, sizeof(homeGameIdString), "%" PRId64, myGameId);
				// Team 2 logs
				gameAttributes[awayCaptainString] = "0";	//	Team1 Captain blazeid
				gameAttributes[awayOtherString] = "0";	//	Team1 other blazeid
				gameAttributes["A_CO"] = "1";			//	Coopid of team1
				gameAttributes["A_CP"] = awayCaptainString;	// team1 captain
				gameAttributes["A_K"] = "294912";
				gameAttributes["A_P"] = "8375740848";		// team1 gameid
				gameAttributes["A_T"] = "18";
				gameAttributes["H_CO"] = homeCoopId;		//	team2 coopid
				gameAttributes["H_CP"] = homeCaptainString;	// team2 captain
				gameAttributes["H_K"] = "3932160";
				gameAttributes["H_P"] = homeGameIdString;		//	team2 gameid
				gameAttributes["H_T"] = "240";
				gameAttributes["S"] = "H";
				setGameAttributes(myGameId, gameAttributes);
				//finalizeGameCreation(myGameId);		
				gameAttributes.clear();
				gameAttributes["Gamemode_Name"] = "coop_season_match";
				gameAttributes["OSDK_gameMode"] = "27";
				gameAttributes["PRE_S"] = "1";
				setGameAttributes(myGameId, gameAttributes);
			}

			//updateGameSession(myGameId);

			// Add player to game map
			mPlayerInGameMap.insert(mPlayerData->getPlayerId());
			//mPlayerInGameMap.insert(notify->getGameData().getPlatformHostInfo().getPlayerId());
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession::handleNotifyGameSetUp]" << mPlayerData->getPlayerId() << "HANDLE_NOTIFYGAMESETUP Current Player count for game ID " << getGameId() << " count : " << mPlayerInGameMap.size() << " As follows :");


			StressGameInfo* ptrGameInfo = CoopplayerGameDataRef(mPlayerData).getGameData(myGameId);
			if (!ptrGameInfo)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "] gameId " << myGameId << " not present in CoopSeasonsGamesMap");
				return;
			}

			if (isPlatformHost == true && IsPlayerPresentInList((PlayerModule*)(mPlayerData->getOwner()), mPlayerData->getPlayerId(), COOPPLAYER) == true)
			{
				uint16_t count = CoopplayerGameDataRef(mPlayerData).getPlayerCount(myGameId);
				CoopplayerGameDataRef(mPlayerData).setPlayerCount(myGameId, ++count);
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "]:GameId=" << myGameId << " Player count = " << count);
			}
		}


		BlazeRpcError CoopSeasonsGameSession::setGameAttributes(GameId  gameId, Collections::AttributeMap &gameAttributes) {
			BlazeRpcError err = ERR_OK;
			Blaze::GameManager::SetGameAttributesRequest request;
			request.setGameId(gameId);
			Collections::AttributeMap::iterator it = gameAttributes.begin();
			while (it != gameAttributes.end()) {
				request.getGameAttributes()[it->first] = it->second;
				++it;
			}
			//char8_t buf[1024];
		 //   BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager , "]::setGameAttributes" << "\n" << request.print(buf, sizeof(buf)));
			err = mPlayerData->getComponentProxy()->mGameManagerProxy->setGameAttributes(request);
			if (err != ERR_OK)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession:setGameAttributes failed. Error(" << ErrorHelp::getErrorName(err) << ") " << mPlayerData->getPlayerId());
			}
			return err;
		}


		BlazeRpcError CoopSeasonsGameSession::updateExtendedDataAttribute(int64_t value) {
			BlazeRpcError err = ERR_OK;
			Blaze::UpdateExtendedDataAttributeRequest request;
			request.setRemove(false);
			request.setKey(1);
			request.setComponent(0);
			request.setValue(value);

			err = mPlayerData->getComponentProxy()->mUserSessionsProxy->updateExtendedDataAttribute(request);
			if (err != ERR_OK)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession:updateExtendedDataAttribute failed. Error(" << ErrorHelp::getErrorName(err) << ") " << mPlayerData->getPlayerId());
			}
			return err;
		}


		BlazeRpcError	CoopSeasonsGameSession::updateGameSession(GameId  gameId)
		{
			BlazeRpcError err = ERR_OK;
			Blaze::GameManager::UpdateGameSessionRequest request;
			request.setGameId(gameId);

			err = mPlayerData->getComponentProxy()->mGameManagerProxy->updateGameSession(request);
			if (err != ERR_OK)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession:updateGameSession failed. Error(" << ErrorHelp::getErrorName(err) << ") " << mPlayerData->getPlayerId());
			}
			return err;
		}

		BlazeRpcError CoopSeasonsGameSession::submitGameReport()
		{
			BlazeRpcError err = ERR_OK;

			StressGameInfo* ptrGameInfo = CoopplayerGameDataRef(mPlayerData).getGameData(myGameId);
			if (!ptrGameInfo)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession::submitGameReport][" << mPlayerData->getPlayerId() << " gameId " << myGameId << " not present in CoopSeasonsGamesMap");
				return ERR_SYSTEM;
			}

			GameReporting::SubmitGameReportRequest request;
			request.setFinishedStatus(Blaze::GameReporting::GAMEREPORT_FINISHED_STATUS_FINISHED);
			GameReporting::GameReport &gamereport = request.getGameReport();

			gamereport.setGameReportingId(mGameReportingId);
			gamereport.setGameReportName("gameType25");
			Blaze::GameReporting::FifaCoopReportBase::FifaCoopGameReportBase fifaCoopGameReptBase;
			Blaze::GameReporting::FifaCoopSeasonsReport::FifaCoopSeasonsGameReport fifaCoopSeasonsGameRept;
			fifaCoopSeasonsGameRept.setMom("");
			// fifaCoopPlayerReptBase is FifaCoopPlayerReportBase wrt TTY;
			Blaze::GameReporting::FifaCoopReportBase::FifaCoopPlayerReportBase *fifaCoopPlayerReptBase;
			// fifaCoopSeasonsPlyrRept is FifaCoopSeasonsPlayerReport wrt TTY;
			Blaze::GameReporting::FifaCoopSeasonsReport::FifaCoopSeasonsPlayerReport *fifaCoopSeasonsPlyrRept;
			Blaze::GameReporting::FifaCoopReportBase::FifaCoopSquadReport *fifaCoopSquadRept;
			Blaze::GameReporting::FifaCoopSeasonsReport::FifaCoopSeasonsSquadDetailsReport *fifaCoopSeasonsSqdDetailsRept;
			Blaze::GameReporting::Fifa::Substitution *substitution = BLAZE_NEW Blaze::GameReporting::Fifa::Substitution;

			// GAME has 1 member OSDKReport
			// OSDKReport has plyr (OSDKPlayerReport), gamr(osdkGamereport), tamr(TeamReport), and cgrt (custom report)
			GameReporting::OSDKGameReportBase::OSDKReport *osdkReport = BLAZE_NEW GameReporting::OSDKGameReportBase::OSDKReport;
			gamereport.setReport(*osdkReport);
			GameReporting::Fifa::H2HGameReport h2hGameReport;

			gamereport.setReport(*osdkReport);
			if (osdkReport != NULL) {
				/*
						***********START	-	FILL GAMR STRUCTURE -	*************************
				*/
				// To fill osdkGamereport
				// osdkGamereport is wrt GAMR in TTY; GAMR is of type OSDKGameReport gameReport;
				Blaze::GameReporting::OSDKGameReportBase::OSDKGameReport& osdkGamereport = osdkReport->getGameReport();  // osdkGamereport is wrt GAMR in TTY
				osdkGamereport.setArenaChallengeId(0);
				osdkGamereport.setCategoryId(0);
				osdkGamereport.setGameReportId(0);
				osdkGamereport.setGameTime(5460);
				osdkGamereport.setSimulate(false);
				osdkGamereport.setLeagueId(0);
				osdkGamereport.setRank(false);
				osdkGamereport.setRoomId(0);
				// osdkGamereport.setSponsoredEventId(0);
				osdkGamereport.setFinishedStatus(1);
				osdkGamereport.setGameType("gameType25");
				// To set cgrt of osdkGamereport; CGRT stands for Game custom game report
				// CGRT is of type variable customGameReport;
				// So, here we need to set FifaCoopGameReportBase as a custom game report to osdkGamereport;
				// FifaCoopGameReportBase has different functions, but as per log we need to fill only ccgr
				// ccgr stands for Game custom club game report; CCGR is of type // variable customCoopGameReport;
				// So, here we need to set FifaCoopSeasonsGameReport as a custom report to fifaCoopGameReptBase
				// FifaCoopSeasonsGameReport has CMGR -->Common Game report; have to set the values of CMGR here;
				Blaze::GameReporting::Fifa::CommonGameReport& commonGameReport = fifaCoopSeasonsGameRept.getCommonGameReport();
				commonGameReport.setAwayKitId(1372);
				commonGameReport.setIsRivalMatch(false);
				commonGameReport.setWentToPk(0);
				commonGameReport.setStadiumId(409);
				commonGameReport.setHomeKitId(3214);
				commonGameReport.setBallId(120);
				const int MAX_TEAM_COUNT = 11;
				const int MAX_PLAYER_VALUE = 100000;
				//  fill awayStartingXI and homeStartingXI values here
				for (int iteration = 0; iteration < MAX_TEAM_COUNT; iteration++) {
					commonGameReport.getAwayStartingXI().push_back(Random::getRandomNumber(MAX_PLAYER_VALUE));
				}
				for (int iteration = 0; iteration < MAX_TEAM_COUNT; iteration++) {
					commonGameReport.getHomeStartingXI().push_back(Random::getRandomNumber(MAX_PLAYER_VALUE));
				}
				substitution->setPlayerSentOff(Blaze::Random::getRandomNumber(250000));
				substitution->setPlayerSubbedIn(Blaze::Random::getRandomNumber(250000));
				substitution->setSubTime(83);
				commonGameReport.getAwaySubList().push_back(substitution);
				fifaCoopGameReptBase.setCustomCoopGameReport(fifaCoopSeasonsGameRept);
				osdkGamereport.setCustomGameReport(fifaCoopGameReptBase);
				/*
						***********END	-	FILL GAMR STRUCTURE -	*************************
				*/
				/*
						***********START	-	FILL PLYR STRUCTURE -	*************************
				*/
				//PlayerIdListMap::iterator start = ptrGameInfo->getPlayerIdListMap().begin();
				 //PlayerIdListMap::iterator end  = ptrGameInfo->getPlayerIdListMap().end();
				 // OsdkPlayerReportsMap is PLYR wrt TTY; PLYR is of type OSDKPlayerReportsMap playerReports;
				Blaze::GameReporting::OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = osdkReport->getPlayerReports();

				OsdkPlayerReportsMap.reserve(mPlayerInGameMap.size());

				/*for (eastl::set<BlazeId>::iterator citPlayerIt = mPlayerInGameMap.begin(), citPlayerItEnd = mPlayerInGameMap.end();
						citPlayerIt != citPlayerItEnd; ++citPlayerIt)
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::gamereporting, "submitGameReport::mPlayerInGameMap::playerID is :: " << *citPlayerIt);
				}*/
				if (mPlayerInGameMap.size() != 4)
				{
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "CoopSeasons:submitGameReport mPlayerInGameMap size is not equal to 4");
					return ERR_SYSTEM;
				}

				BlazeId	opponentPlayer[2] = { 0, 0 };
				uint16_t i = 0;
				OsdkPlayerReportsMap.reserve(mPlayerInGameMap.size());
				for (eastl::set<BlazeId>::iterator citPlayerIt = mPlayerInGameMap.begin(), citPlayerItEnd = mPlayerInGameMap.end();
					citPlayerIt != citPlayerItEnd; ++citPlayerIt)
				{

					if ((*citPlayerIt != mPlayerData->getPlayerId()) && (*citPlayerIt != mFriendBlazeId))
					{
						opponentPlayer[i] = *citPlayerIt;
						BLAZE_TRACE_LOG(BlazeRpcLog::gamereporting, "submitGameReport::playerID " << mPlayerData->getPlayerId() << " opponentPlayer is :: " << opponentPlayer[i]);
						i++;
					}
					if (i == 2) break;
				}
				if (opponentPlayer[0] == 0 || opponentPlayer[1] == 0)
				{
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "CoopSeasons:submitGameReport opponentPlayer info missing");
					return ERR_SYSTEM;
				}
				uint64_t opponentCoopId = 0;
				err = getCoopIdData(opponentPlayer[0], opponentPlayer[1], opponentCoopId);
				if (err != ERR_OK || opponentCoopId == 0)
				{
					BLAZE_ERR_LOG(BlazeRpcLog::util, "CoopSeasons::submitGameReport: failed with error " << ErrorHelp::getErrorName(err) << " " << mPlayerData->getPlayerId());
					opponentCoopId = 1;
				}

				PlayerInGameMap getPlayerMap = mPlayerInGameMap;
				for (eastl::set<BlazeId>::iterator citPlayerIt = mPlayerInGameMap.begin(), citPlayerItEnd = mPlayerInGameMap.end();
					citPlayerIt != citPlayerItEnd; ++citPlayerIt)
				{
					GameManager::PlayerId playerId = *citPlayerIt;

					// playerReport is PLYR wrt TTY;
					Blaze::GameReporting::OSDKGameReportBase::OSDKPlayerReport* playerReport = OsdkPlayerReportsMap.allocate_element();
					// CPRT is a member of OSDKPlayerReport
					// CPRT stands for "Game custom player report"; and is of type "variable customPlayerReport";
					// So here we have to fill CPRT first and then set CPRT to playerReport;
					// FifaCoopPlayerReportBase is a member of CPRT
					// so here we have to fill FifaCoopPlayerReportBase first and then set FifaCoopPlayerReportBase to playerReport(PLYR)
					// FifaCoopPlayerReportBase setting follows here.....
					// fifaCoopPlayerReptBase is FifaCoopPlayerReportBase wrt TTY;
					fifaCoopPlayerReptBase = BLAZE_NEW Blaze::GameReporting::FifaCoopReportBase::FifaCoopPlayerReportBase;
					fifaCoopPlayerReptBase->setCoopGames(0);
					if ((playerId != mPlayerData->getPlayerId()) && (playerId != mFriendBlazeId))
					{
						fifaCoopPlayerReptBase->setCoopId(mCoopId);
					}
					else
					{
						fifaCoopPlayerReptBase->setCoopId(opponentCoopId);
					}
					fifaCoopPlayerReptBase->setMinutes(0);
					fifaCoopPlayerReptBase->setPos(((uint16_t)Blaze::Random::getRandomNumber(5)) + 1);
					// COPR is a member of FifaCoopPlayerReportBase
					// Setting of COPR goes here....
					// COPR stands for "Game custom coop player report" and is of type "variable customCoopPlayerReport";
					// FifaCoopSeasonsPlayerReport is a member of COPR
					// So fill FifaCoopSeasonsPlayerReport and set FifaCoopSeasonsPlayerReport to FifaCoopPlayerReportBase
					fifaCoopSeasonsPlyrRept = BLAZE_NEW Blaze::GameReporting::FifaCoopSeasonsReport::FifaCoopSeasonsPlayerReport;
					fifaCoopSeasonsPlyrRept->setManOfTheMatch(0);
					fifaCoopSeasonsPlyrRept->setCleanSheetsAny(0);
					fifaCoopSeasonsPlyrRept->setCleanSheetsDef(0);
					fifaCoopSeasonsPlyrRept->setCleanSheetsGoalKeeper(0);
					// CMPR is a member of FifaCoopSeasonsPlayerReport
					// so here, we have to set CMPR first
					// fifaCoopsCmnPlyrRept is CMPR wrt TTY;
					Blaze::GameReporting::Fifa::CommonPlayerReport& fifaCoopsCmnPlyrRept = fifaCoopSeasonsPlyrRept->getCommonPlayerReport();
					fifaCoopsCmnPlyrRept.setAssists(rand() % 10);
					fifaCoopsCmnPlyrRept.setGoalsConceded(rand() % 10);
					fifaCoopsCmnPlyrRept.setHasCleanSheets(rand() % 10);
					fifaCoopsCmnPlyrRept.setCorners(rand() % 15);
					fifaCoopsCmnPlyrRept.setPassAttempts(rand() % 10);
					fifaCoopsCmnPlyrRept.setPassesMade(rand() % 12);
					fifaCoopsCmnPlyrRept.setRating((float)(rand() % 10));
					fifaCoopsCmnPlyrRept.setSaves(rand() % 10);
					fifaCoopsCmnPlyrRept.setShots(rand() % 12);
					fifaCoopsCmnPlyrRept.setTackleAttempts(rand() % 12);
					fifaCoopsCmnPlyrRept.setTacklesMade(rand() % 15);
					fifaCoopsCmnPlyrRept.setFouls(rand() % 12);
					fifaCoopsCmnPlyrRept.setGoals(rand() % 15);
					fifaCoopsCmnPlyrRept.setInterceptions(rand() % 10);
					fifaCoopsCmnPlyrRept.setHasMOTM(rand() % 10);
					fifaCoopsCmnPlyrRept.setOffsides(rand() % 15);
					fifaCoopsCmnPlyrRept.setOwnGoals(rand() % 10);
					fifaCoopsCmnPlyrRept.setPkGoals(0);
					fifaCoopsCmnPlyrRept.setPossession(rand() % 15);
					fifaCoopsCmnPlyrRept.setRedCard(rand() % 12);
					fifaCoopsCmnPlyrRept.setShotsOnGoal(rand() % 15);
					fifaCoopsCmnPlyrRept.setUnadjustedScore(rand() % 10);
					fifaCoopsCmnPlyrRept.setYellowCard(rand() % 12);
					// to set fifaCoopSeasonsPlyrRept to FifaCoopPlayerReportBase
					fifaCoopPlayerReptBase->setCustomCoopPlayerReport(*fifaCoopSeasonsPlyrRept);
					playerReport->setCustomDnf(rand() % 12);
					playerReport->setClientScore(rand() % 10);
					playerReport->setAccountCountry(rand() % 12);
					playerReport->setFinishReason(rand() % 15);
					playerReport->setGameResult(rand() % 10);
					if ((playerId != mPlayerData->getPlayerId()) && (playerId != mFriendBlazeId))
					{
						playerReport->setHome(true);
						playerReport->setScore(4);
					}
					else
					{
						playerReport->setHome(false);
						playerReport->setScore(0);
					}
					playerReport->setLosses(0);
					playerReport->setName(mPlayerData->getPersonaName().c_str());
					// playerReport->setName(mUserPersona.c_str());
					playerReport->setOpponentCount(0);
					playerReport->setExternalId(0);
					playerReport->setNucleusId(0);
					playerReport->setPersona("");
					playerReport->setPointsAgainst(0);
					playerReport->setUserResult(0);
					//playerReport->setScore(4);
					// playerReport->setSponsoredEventAccountRegion(0);
					playerReport->setSkill(0);
					playerReport->setSkillPoints(0);
					playerReport->setTeam(241);
					playerReport->setTies(0);
					playerReport->setWinnerByDnf(0);
					playerReport->setWins(0);
					// To set OPPR; OPPR stands for OSDKPrivatePlayerReport
					// osdkPrivatePlayerReport is OPPR wrt TTY;
					if (playerId == mPlayerData->getPlayerId()) {
						Blaze::GameReporting::OSDKGameReportBase::OSDKPrivatePlayerReport &osdkPrivatePlayerReport = playerReport->getPrivatePlayerReport();
						Blaze::GameReporting::OSDKGameReportBase::IntegerAttributeMap &PlayerPrivateIntAttribMap = osdkPrivatePlayerReport.getPrivateIntAttributeMap();
						PlayerPrivateIntAttribMap["BANDAVGGM"] = 1000 * (uint16_t)Random::getRandomNumber(1000) + 197215731;
						PlayerPrivateIntAttribMap["BANDAVGNET"] = (uint16_t)Random::getRandomNumber(1000) + 336;
						PlayerPrivateIntAttribMap["BANDHIGM"] = -1 * 10000 * (uint16_t)Random::getRandomNumber(10) + 247483648;
						PlayerPrivateIntAttribMap["BANDHINET"] = 1000 * (uint16_t)Random::getRandomNumber(10) + (uint16_t)Random::getRandomNumber(100);
						PlayerPrivateIntAttribMap["BANDLOWGM"] = 1000000 * (uint16_t)Random::getRandomNumber(10) + 15709128;
						PlayerPrivateIntAttribMap["BANDLOWNET"] = (uint16_t)Random::getRandomNumber(100);
						PlayerPrivateIntAttribMap["BYTESRCVDNET"] = (uint16_t)Random::getRandomNumber(10) + 1105358;
						PlayerPrivateIntAttribMap["BYTESSENTNET"] = 100000 * (uint16_t)Random::getRandomNumber(10) + 317652;
						PlayerPrivateIntAttribMap["DROPPKTS"] = -1 * (uint16_t)Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["FPSAVG"] = 100000 * (uint16_t)Random::getRandomNumber(10) + 227131;
						PlayerPrivateIntAttribMap["FPSDEV"] = 100000 * (uint16_t)Random::getRandomNumber(10) + 224483;
						PlayerPrivateIntAttribMap["FPSHI"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["FPSLOW"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["GAMEENDREASON"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["GDESYNCEND"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["GDESYNCRSN"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["GENDPHASE"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["GPCKTPERIOD"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["GPCKTPERIODSTD"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["GRESULT"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["GRPTTYPE"] = 25;
						PlayerPrivateIntAttribMap["GRPTVER"] = 8;
						PlayerPrivateIntAttribMap["GUESTS0"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["GUESTS1"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["LATEAVGGM"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["LATEAVGNET"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["LATEHIGM"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["LATEHINET"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["LATELOWGM"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["LATELOWNET"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["LATESDEVGM"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["LATESDEVNET"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["PKTLOSS"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["REALTIMEGAME"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["REALTIMEIDLE"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["USERSEND0"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["USERSEND1"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["USERSSTRT0"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["USERSSTRT1"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["VOIPEND0"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["VOIPEND1"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["VOIPSTRT0"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["VOIPSTRT1"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["gid"] = getGameId();
					}  // if end
					playerReport->setCustomPlayerReport(*fifaCoopPlayerReptBase);
					OsdkPlayerReportsMap[playerId] = playerReport;
				}  // for end
				/*
						***********END	-	FILL PLYR STRUCTURE -	*************************
				*/
				/*
						***********START	-	FILL TAMR STRUCTURE -	*************************
				*/
				// To set TAMR to osdkReport
				// TAMR is of type "variable teamReports";   // Map to OSDKClubReport TDF
				fifaCoopSquadRept = BLAZE_NEW Blaze::GameReporting::FifaCoopReportBase::FifaCoopSquadReport;
				osdkReport->setTeamReports(*fifaCoopSquadRept);

				Blaze::GameReporting::FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap& squadReportsMap = fifaCoopSquadRept->getSquadReports();
				squadReportsMap.reserve(2);
				for (int iValue = 1; iValue <= 2; iValue++)
				{
					// typedef map<uint64_t, FifaCoopSquadDetailsReportBase> FifaSquadReportsMap;
					Blaze::GameReporting::FifaCoopReportBase::FifaCoopSquadDetailsReportBase* squadDetailsReptBase = squadReportsMap.allocate_element();
					if (iValue == 1)
					{
						squadDetailsReptBase->setCoopId(mCoopId);
					}
					else
					{
						squadDetailsReptBase->setCoopId(opponentCoopId);
					}
					squadDetailsReptBase->setSquadDisc((uint16_t)Blaze::Random::getRandomNumber(2));
					squadDetailsReptBase->setHome(true);
					squadDetailsReptBase->setGameResult((uint16_t)Blaze::Random::getRandomNumber(2));
					squadDetailsReptBase->setScore((uint16_t)Blaze::Random::getRandomNumber(2));
					squadDetailsReptBase->setSkill((uint16_t)Blaze::Random::getRandomNumber(2));
					squadDetailsReptBase->setSkillPoints((uint16_t)Blaze::Random::getRandomNumber(2));
					squadDetailsReptBase->setTeam(1);
					squadDetailsReptBase->setTies((uint16_t)Blaze::Random::getRandomNumber(2));
					squadDetailsReptBase->setWinnerByDnf((uint16_t)Blaze::Random::getRandomNumber(2));
					squadDetailsReptBase->setWins((uint16_t)Blaze::Random::getRandomNumber(2));
					// cccr is Variable game club club TDF report and is of type "variable customSquadDetailsReport";
					fifaCoopSeasonsSqdDetailsRept = BLAZE_NEW Blaze::GameReporting::FifaCoopSeasonsReport::FifaCoopSeasonsSquadDetailsReport;
					squadDetailsReptBase->setCustomSquadDetailsReport(*fifaCoopSeasonsSqdDetailsRept);
					fifaCoopSeasonsSqdDetailsRept->setCleanSheets((uint16_t)(uint16_t)Blaze::Random::getRandomNumber(2));
					fifaCoopSeasonsSqdDetailsRept->setGoalsAgainst((uint16_t)Blaze::Random::getRandomNumber(6));
					fifaCoopSeasonsSqdDetailsRept->setGoals((uint16_t)Blaze::Random::getRandomNumber(8));
					fifaCoopSeasonsSqdDetailsRept->setShotsAgainst((uint16_t)Blaze::Random::getRandomNumber(2));
					fifaCoopSeasonsSqdDetailsRept->setShotsAgainstOnGoal((uint16_t)Blaze::Random::getRandomNumber(2));
					fifaCoopSeasonsSqdDetailsRept->setTeamrating(80);
					Blaze::GameReporting::Fifa::CommonPlayerReport& coopCommomClubReport = fifaCoopSeasonsSqdDetailsRept->getCommonClubReport();
					coopCommomClubReport.getCommondetailreport().setAveragePossessionLength(0);
					coopCommomClubReport.getCommondetailreport().setAverageFatigueAfterNinety(0);
					coopCommomClubReport.getCommondetailreport().setInjuriesRed(0);
					coopCommomClubReport.getCommondetailreport().setPassesIntercepted(0);
					coopCommomClubReport.getCommondetailreport().setPenaltiesAwarded(0);
					coopCommomClubReport.getCommondetailreport().setPenaltiesMissed(0);
					coopCommomClubReport.getCommondetailreport().setPenaltiesScored(0);
					coopCommomClubReport.getCommondetailreport().setPenaltiesSaved(0);
					coopCommomClubReport.setAssists((uint16_t)Blaze::Random::getRandomNumber(2));
					coopCommomClubReport.setGoalsConceded((uint16_t)Blaze::Random::getRandomNumber(4));
					coopCommomClubReport.setCorners((uint16_t)Blaze::Random::getRandomNumber(5));
					coopCommomClubReport.setPassAttempts((uint16_t)Blaze::Random::getRandomNumber(15));
					coopCommomClubReport.setPassesMade((uint16_t)Blaze::Random::getRandomNumber(6));
					coopCommomClubReport.setRating((float)((uint16_t)Blaze::Random::getRandomNumber(2)));
					coopCommomClubReport.setSaves((uint16_t)Blaze::Random::getRandomNumber(4));
					coopCommomClubReport.setShots((uint16_t)Blaze::Random::getRandomNumber(6));
					coopCommomClubReport.setTackleAttempts((uint16_t)Blaze::Random::getRandomNumber(6));
					coopCommomClubReport.setTacklesMade((uint16_t)Blaze::Random::getRandomNumber(5));
					coopCommomClubReport.setFouls((uint16_t)Blaze::Random::getRandomNumber(3));
					coopCommomClubReport.setGoals((uint16_t)Blaze::Random::getRandomNumber(5));
					coopCommomClubReport.setInterceptions((uint16_t)Blaze::Random::getRandomNumber(2));
					coopCommomClubReport.setOffsides((uint16_t)Blaze::Random::getRandomNumber(5));
					coopCommomClubReport.setOwnGoals((uint16_t)Blaze::Random::getRandomNumber(2));
					coopCommomClubReport.setPkGoals(0);
					coopCommomClubReport.setPossession((uint16_t)Blaze::Random::getRandomNumber(5));
					coopCommomClubReport.setRedCard((uint16_t)Blaze::Random::getRandomNumber(3));
					coopCommomClubReport.setShotsOnGoal((uint16_t)Blaze::Random::getRandomNumber(5));
					coopCommomClubReport.setUnadjustedScore((uint16_t)Blaze::Random::getRandomNumber(2));
					coopCommomClubReport.setYellowCard((uint16_t)Blaze::Random::getRandomNumber(3));
					Blaze::GameReporting::Fifa::SeasonalPlayData& coopSeasonalPlayData = fifaCoopSeasonsSqdDetailsRept->getSeasonalPlayData();
					coopSeasonalPlayData.setCurrentDivision((uint16_t)Blaze::Random::getRandomNumber(2));
					coopSeasonalPlayData.setCup_id((uint16_t)Blaze::Random::getRandomNumber(2));
					coopSeasonalPlayData.setUpdateDivision(Blaze::GameReporting::Fifa::NO_UPDATE);
					coopSeasonalPlayData.setSeasonId(1 + ((uint16_t)Blaze::Random::getRandomNumber(2)));
					coopSeasonalPlayData.setWonCompetition((uint16_t)Blaze::Random::getRandomNumber(2));
					coopSeasonalPlayData.setWonLeagueTitle(false);
					if (iValue == 1)
					{
						squadReportsMap[mCoopId] = squadDetailsReptBase;
					}
					else
					{
						squadReportsMap[opponentCoopId] = squadDetailsReptBase;
					}
				}  // for end in TAMR
				/*
						***********END	-	FILL TAMR STRUCTURE -	*************************
				*/
			}  // if (osdkReport != NULL) end

			// char8_t buf[29240];
			// BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession][" << mPlayerData->getPlayerId() << "]::submitGameReport" << "\n" << request.print(buf, sizeof(buf)));

			err = mPlayerData->getComponentProxy()->mGameReportingProxy->submitGameReport(request);
			if (err != ERR_OK)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession][" << mPlayerData->getPlayerId() << "]::submitGameReport failed with  error(" << ErrorHelp::getErrorName(err));
			}
			else
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession][" << mPlayerData->getPlayerId() << "]::submitGameReport Success");
			}
			return err;
		}

		void CoopSeasonsGameSession::addFriendToList(BlazeId playerId)
		{
			CoopFriendsList* friendsListReference = ((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->getCoopFriendsList();
			CoopFriendsList::iterator it = friendsListReference->begin();
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

		BlazeId CoopSeasonsGameSession::getARandomFriend()
		{
			CoopFriendsList* friendsListReference = ((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->getCoopFriendsList();
			if (friendsListReference->size() == 0)
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession::getARandomFriend] The gameId map is empty " << mPlayerData->getPlayerId());
				return 0;
			}
			CoopFriendsList::iterator it = friendsListReference->begin();
			BlazeId randomPlayerId; int32_t num = 0;

			mMutex.Lock();
			num = Random::getRandomNumber(friendsListReference->size());
			eastl::advance(it, num);
			randomPlayerId = *it;
			friendsListReference->erase(it);
			mMutex.Unlock();
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopSeasonsGameSession::getARandomFriend]  Randomly fetched playerID : " << randomPlayerId << " " << mPlayerData->getPlayerId());
			return randomPlayerId;
		}

		void CoopSeasonsGameSession::addCoopFriendsToMap(BlazeId hostId, BlazeId friendID)
		{
			CoopFriendsMap& friendsMap = ((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->getCoopFriendsMap();
			friendsMap[hostId] = friendID;
			friendsMap[friendID] = hostId;
		}

		BlazeId CoopSeasonsGameSession::getFriendInMap(BlazeId playerID)
		{
			CoopFriendsMap& friendsMap = ((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->getCoopFriendsMap();
			if (friendsMap.find(playerID) == friendsMap.end())
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::coopseason, "CoopSeasonsGameSession::getMyFriendInMap FriendId not found " << mPlayerData->getPlayerId());
				return 0;
			}
			return friendsMap[playerID];
		}

		void CoopSeasonsGameSession::removeFriendFromMap()
		{
			CoopFriendsMap& friendsMap = ((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->getCoopFriendsMap();
			CoopFriendsMap::iterator friendsData = friendsMap.find(mPlayerData->getPlayerId());
			if (friendsData != friendsMap.end())
			{
				friendsMap.erase(friendsData);
			}
		}

		BlazeRpcError CoopSeasonsGameSession::getCoopIdData(BlazeId BlazeID1, BlazeId BlazeID2, uint64_t &coopId)
		{
			BlazeRpcError err = ERR_SYSTEM;  // ERR_SYSTEM represents General system error.
			Blaze::CoopSeason::GetCoopIdDataRequest request;
			Blaze::CoopSeason::GetCoopIdDataResponse response;
			request.setMemberOneBlazeId(BlazeID1);
			request.setMemberTwoBlazeId(BlazeID2);
			if (BlazeID1 == 0)
			{
				request.setCoopId(coopId);
				mCoopId = coopId;
			}
			err = mPlayerData->getComponentProxy()->mCoopSeasonProxy->getCoopIdData(request, response);

			if (err != ERR_OK) {
				BLAZE_ERR_LOG(BlazeRpcLog::util, "CoopSeasonsGameSession::getCoopIdData: failed with error " << ErrorHelp::getErrorName(err) << mPlayerData->getPlayerId());
			}
			else {
				coopId = response.getCoopPairData().getCoopId();
			}
			BLAZE_TRACE_LOG(BlazeRpcLog::util, "CoopSeasonsGameSession::getCoopIdData:mCoopId=" << coopId << " for players " << BlazeID1 << " " << BlazeID2);
			return err;
		}


	}  // namespace Stress
}  // namespace Blaze
