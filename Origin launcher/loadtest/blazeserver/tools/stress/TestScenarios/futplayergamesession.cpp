//  *************************************************************************************************
//
//   File:    futplayergamesession.cpp
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
#include "futplayergamesession.h"
#include "utility.h"
#include "gamereporting/fifa/tdf/futreport_server.h"
#include "gamereporting/fifa/tdf/futotpreport.h"

using namespace Blaze::GameManager;
using namespace Blaze::GameReporting;

namespace Blaze {
namespace Stress {

FutPlayerGameSession::FutPlayerGameSession(StressPlayerInfo* playerData, uint32_t GameSize, GameNetworkTopology Topology, GameProtocolVersionString GameProtocolVersionString)
	: GameSession(playerData, GameSize, Topology, GameProtocolVersionString),
	mFriendBlazeId(0), mGameGroupId(0), isGrupPlatformHost(false)
{
	mGameReportingId = 0;
	mPlayerIds.push_back(0);
}

FutPlayerGameSession::~FutPlayerGameSession()
{
	BLAZE_INFO(BlazeRpcLog::util, "[FutPlayerGameSession][Destructor][%" PRIu64 "]: destroy called", mPlayerData->getPlayerId());
}

void FutPlayerGameSession::asyncHandlerFunction(uint16_t type, EA::TDF::Tdf *notification)
{
	if (mPlayerData != NULL && mPlayerData->isPlayerActive())
	{
		//  This Notification has not been handled so use the Default Handler
		onGameSessionDefaultAsyncHandler(type, notification);
	}
}

void FutPlayerGameSession::onGameSessionDefaultAsyncHandler(uint16_t type, EA::TDF::Tdf* notification)
{
	//  This is abstract class and handle the async handlers in the derived class

	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]:Message Type =" << type);
	switch (type)
	{
	case GameManagerSlave::NOTIFY_NOTIFYGAMESETUP:
		//  Async notification sent to the player who joined a game.  Triggered by JoinGame Request or a Matchmaking session finding a game.
	{
		NotifyGameSetup* notify = (NotifyGameSetup*)notification;
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::asyncHandlerFunction]: Received  NOTIFY_NOTIFYGAMESETUP." << mPlayerData->getPlayerId());
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
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMESTATECHANGE, state=" <<
			notify->getNewGameState());
		if (myGameId == notify->getGameId())
		{
			if (getPlayerState() == IN_GAME && notify->getNewGameState() == POST_GAME)
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::NOTIFY_NOTIFYGAMESTATECHANGE][" << mPlayerData->getPlayerId() << "]: submitGameReport");
				submitGameReport(mScenarioType);
				if (StressConfigInst.isOnlineScenario(mScenarioType))
				{
					mPlayerData->getShieldExtension()->perfTune(mPlayerData, true);
				}
				if (mScenarioType == FUT_COOP_RIVALS || mScenarioType == FUT_PLAY_ONLINE || mScenarioType == FUT_ONLINE_SQUAD_BATTLE)
				{
					leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, myGameId);
					leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGameGroupId);
				}
				else
				{
					leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, myGameId);
				}
				//leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT);
			}
			//  Set Game State
			setPlayerState(notify->getNewGameState());
			FutPlayerGameDataRef(mPlayerData).setGameState(myGameId, notify->getNewGameState());
		}
	}
	break;
	case GameManagerSlave::NOTIFY_NOTIFYGAMERESET:
	{
		NotifyGameReset* notify = (NotifyGameReset*)notification;
		StressGameInfo* gameInfo = FutPlayerGameDataRef(mPlayerData).getGameData(notify->getGameData().getGameId());
		if (gameInfo != NULL && gameInfo->getPlatformHostPlayerId() == mPlayerData->getPlayerId())
		{
			//  TODO: host specific actions
		}
		BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMERESET.gameId=" <<
			notify->getGameData().getGameId());
	}
	break;
	case GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINING:
	{
		NotifyPlayerJoining* notify = (NotifyPlayerJoining*)notification;
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERJOINING.gameId=" << notify->getGameId());

		//For Game Group
		if (notify->getGameId() == mGameGroupId)
		{
			//meshEndpointsConnected
			BlazeObjectId targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, notify->getJoiningPlayer().getConnectionGroupId());
			meshEndpointsConnected(targetGroupId, 0, 0, mGameGroupId);
			// Add player to game map
			mPlayerInGameGroupMap.insert(notify->getJoiningPlayer().getPlayerId());
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FUTPlayerGameSession::NOTIFYPLAYERJOINING:GAME_TYPE_GROUP][ GameGroupID : " << mGameGroupId << " joinerId " << notify->getJoiningPlayer().getPlayerId());
		}
		//For Game Session Type
		if (notify->getGameId() == myGameId)
		{
			//meshEndpointsConnected
			BlazeObjectId targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, notify->getJoiningPlayer().getConnectionGroupId());
			meshEndpointsConnected(targetGroupId, 0, 0, myGameId);
			// Add player to game map
			mPlayerInGameMap.insert(notify->getJoiningPlayer().getPlayerId());
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::NOTIFYPLAYERJOINCOMPLETED:mPlayerInGameMap][ GameID : " << myGameId << " joinerId " << notify->getJoiningPlayer().getPlayerId());

			if (mScenarioType != FUT_COOP_RIVALS && mScenarioType != FUT_PLAY_ONLINE && mScenarioType != FUT_ONLINE_SQUAD_BATTLE)
			{
				//Add opponent player info if not received in notifygamesetup
				if (getOpponentPlayerInfo().getPlayerId() == 0 && mPlayerData->getPlayerId() != notify->getJoiningPlayer().getPlayerId())
				{
					getOpponentPlayerInfo().setPlayerId(notify->getJoiningPlayer().getPlayerId());
					getOpponentPlayerInfo().setConnGroupId(BlazeObjectId(ENTITY_TYPE_CONN_GROUP, notify->getJoiningPlayer().getConnectionGroupId()));
					BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << " OpponentPlayerInfo PlayerId=" << getOpponentPlayerInfo().getPlayerId() << " ConnGroupId=" << getOpponentPlayerInfo().getConnGroupId().toString());
				}
			}
		}
	}
	break;
	case GameManagerSlave::NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS:
	{
		BLAZE_INFO(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::handlerFunction][%" PRIu64 "]: Received  NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS.", mPlayerData->getPlayerId());
	}
	break;
	case GameManagerSlave::NOTIFY_NOTIFYPLATFORMHOSTINITIALIZED:
	{
		NotifyPlatformHostInitialized* notify = (NotifyPlatformHostInitialized*)notification;
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLATFORMHOSTINITIALIZED.gameId=" <<
			notify->getGameId());
	}
	break;
	case GameManagerSlave::NOTIFY_NOTIFYGAMELISTUPDATE:
	{
		//NotifyGameListUpdate* notify = (NotifyGameListUpdate*)notification;
		BLAZE_INFO(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::asyncHandlerFunction][%" PRIu64 "]: Received  NOTIFY_NOTIFYGAMELISTUPDATE.", mPlayerData->getPlayerId());

	}
	break;
	case GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINCOMPLETED:
	{
		NotifyPlayerJoinCompleted* notify = (NotifyPlayerJoinCompleted*)notification;
		BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERJOINCOMPLETED.");
		//char8_t buf[20240];
		//BLAZE_TRACE_RPC_LOG(BlazeRpcLog::gamemanager, "NOTIFY_NOTIFYPLAYERJOINCOMPLETED RPC : \n" << notify->print(buf, sizeof(buf)));
		//we get all the players in a coop game during joinCompleted call Back
		if (notify->getGameId() == myGameId)
		{
			// Add player to game map
			mPlayerInGameMap.insert(notify->getPlayerId());
			
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::NOTIFYPLAYERJOINCOMPLETED:mPlayerInGameMap][ GameID : " << myGameId << " joinerId " << notify->getPlayerId());

			if (IsPlayerPresentInList((PlayerModule*)(mPlayerData->getOwner()), notify->getPlayerId(), FUTPLAYER) == true)
			{
				uint16_t count = FutPlayerGameDataRef(mPlayerData).getPlayerCount(myGameId);
				FutPlayerGameDataRef(mPlayerData).setPlayerCount(myGameId, ++count);
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::NOTIFYPLAYERJOINCOMPLETED][" << mPlayerData->getPlayerId() << "]:GameId=" << myGameId << " Player count = " << count);
			}

			if (mScenarioType == FUT_PLAY_ONLINE || mScenarioType == FUT_COOP_RIVALS || mScenarioType == FUT_ONLINE_SQUAD_BATTLE)
			{
				updatePrimaryExternalSessionForUserRequest(mPlayerData, myGameId, myGameId, UPDATE_PRESENCE_REASON_GAME_JOIN, GAME_TYPE_GAMESESSION, ACTIVE_CONNECTED, PRESENCE_MODE_STANDARD, mGameGroupId);
			}
			else if (mScenarioType != FUTFRIENDLIES && mScenarioType != FUTCHAMPIONMODE)
			{
				invokeUdatePrimaryExternalSessionForUser(mPlayerData, myGameId, "", "", myGameId, GAME_JOIN, GAME_TYPE_GROUP, ACTIVE_CONNECTED);
			}
			else
			{
				updatePrimaryExternalSessionForUserRequest(mPlayerData, myGameId, 0, UPDATE_PRESENCE_REASON_GAME_JOIN, GAME_TYPE_GAMESESSION, ACTIVE_CONNECTED, PRESENCE_MODE_STANDARD, 0, false);
				/*if (isPlatformHost)
				{
					invokeUdatePrimaryExternalSessionForUser(mPlayerData, myGameId, "", "", myGameId, GAME_JOIN, GAME_TYPE_GAMESESSION, ACTIVE_CONNECTED);
				}
				invokeUdatePrimaryExternalSessionForUser(mPlayerData, myGameId, "", "", myGameId, GAME_JOIN, GAME_TYPE_GAMESESSION, ACTIVE_CONNECTED);*/
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
				/*BlazeObjectId targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, mTopologyHostConnectionGroupId);
				meshEndpointsDisconnected(targetGroupId, DISCONNECTED_PLAYER_REMOVED, 0, mGameGroupId);
				targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, 0);
				meshEndpointsDisconnected(targetGroupId, DISCONNECTED, 0, mGameGroupId);*/
				BlazeObjectId targetGroupId = getOpponentPlayerInfo().getConnGroupId();
				meshEndpointsConnectionLost(targetGroupId, 4, mGameGroupId);
				meshEndpointsDisconnected(targetGroupId, DISCONNECTED_PLAYER_REMOVED, 4, mGameGroupId);
			}
			else
			{
				//meshEndpointsDisconnected to opponent group ID TODO
				BlazeObjectId targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, mGroupHostConnectionGroupId);
				meshEndpointsDisconnected(targetGroupId, DISCONNECTED, 0, mGameGroupId);
			}
		}
		//If Game_Type_Gamesession
		if (mPlayerData->getPlayerId() == notify->getPlayerId() && myGameId == notify->getGameId())
		{
			setPlayerState(POST_GAME);
			//meshEndpointsDisconnected
			if (mScenarioType == FUT_PLAY_ONLINE || mScenarioType == FUT_COOP_RIVALS || mScenarioType == FUT_ONLINE_SQUAD_BATTLE)
			{
				BlazeObjectId targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, mTopologyHostConnectionGroupId);
				meshEndpointsConnectionLost(targetGroupId, 4, myGameId);
				meshEndpointsDisconnected(targetGroupId, DISCONNECTED, 4, myGameId);
				
			}
			else
			{
				BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERREMOVED for game [" << notify->getGameId() << "] reason: " << notify->getPlayerRemovedReason());
				if (mScenarioType == FUTCHAMPIONMODE || mScenarioType == FUTDRAFTMODE)
				{
					BlazeObjectId targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, mTopologyHostConnectionGroupId);
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << " meshEndpointsDisconnected for grouID" << targetGroupId.toString());
					meshEndpointsConnectionLost(targetGroupId, 4, myGameId);
					meshEndpointsDisconnected(targetGroupId, DISCONNECTED, 4, myGameId);
				}
				else
				{
					if (isPlatformHost)
					{
						BlazeObjectId targetGroupId = getOpponentPlayerInfo().getConnGroupId();
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << " meshEndpointsDisconnected for grouID" << targetGroupId.toString());
						meshEndpointsDisconnected(targetGroupId, DISCONNECTED_PLAYER_REMOVED);
						targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, 0);
						meshEndpointsDisconnected(targetGroupId, DISCONNECTED);
					}
					else
					{
						//meshEndpointsDisconnected - DISCONNECTED
						BlazeObjectId targetGroupId = getOpponentPlayerInfo().getConnGroupId();
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << " meshEndpointsDisconnected for grouID" << targetGroupId.toString());
						meshEndpointsConnectionLost(targetGroupId, 4, myGameId);
						meshEndpointsDisconnected(targetGroupId, DISCONNECTED, 4, myGameId);

					}
				}
			}
		}

		StressGameInfo* ptrGameInfo = FutPlayerGameDataRef(mPlayerData).getGameData(myGameId);
		if (!ptrGameInfo)
		{
			BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]::receiving notification(" << type << ") and gameId " << myGameId <<
				"not present in FutSeasonsGamesMap");
			break;
		}
		if (myGameId == notify->getGameId() && FutPlayerGameDataRef(mPlayerData).isPlayerExistInGame(notify->getPlayerId(), notify->getGameId()) == true)
		{
			FutPlayerGameDataRef(mPlayerData).removePlayerFromGameData(notify->getPlayerId(), notify->getGameId());
			if (IsPlayerPresentInList((PlayerModule*)(mPlayerData->getOwner()), notify->getPlayerId(), FUTPLAYER) == true)
			{
				uint16_t count = ptrGameInfo->getPlayerCount();
				ptrGameInfo->setPlayerCount(--count);
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::asyncHandlerFunction]::NOTIFYPLAYERREMOVED[" << mPlayerData->getPlayerId() << "]:GameId=" << myGameId << " Player count = " << count
					<< "for player=" << notify->getPlayerId());
			}
		}
		//  if all players, in single exe, removed from the game delete the data from map
		if (ptrGameInfo->getPlayerCount() <= 0)
		{
			BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERREMOVED - Game Id = " <<
				notify->getGameId() << ".");
			FutPlayerGameDataRef(mPlayerData).removeGameData(notify->getGameId());
		}
	}
	break;
	case GameManagerSlave::NOTIFY_NOTIFYGAMEREMOVED:
	{
		//  NotifyGameRemoved* notify = (NotifyGameRemoved*)notification;
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMEREMOVED.");
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
			BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::OnNotifyMatchmakingFailed][" << mPlayerData->getPlayerId() << "]: Matchmake SESSION_TIMED_OUT.");
		}
		break;
		case GameManager::SESSION_CANCELED:
		{
			BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::OnNotifyMatchmakingFailed][" << mPlayerData->getPlayerId() << "]: Matchmake SESSION_CANCELED.");
			//  Throw Execption;
		}
		break;
		default:
			break;
		}
		BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYMATCHMAKINGFAILED.");

		if (mScenarioType == FUT_PLAY_ONLINE || mScenarioType == FUT_COOP_RIVALS || mScenarioType == FUT_ONLINE_SQUAD_BATTLE)
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

		if (mScenarioType == FUT_PLAY_ONLINE || mScenarioType == FUTRIVALS || mScenarioType == FUT_ONLINE_SQUAD_BATTLE)
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
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::handlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYMESSAGE.");
		// Messaging::ServerMessage* notify = (Messaging::ServerMessage*)notification;
			//   Handle player message notifications.
			//   Get the message details

	}
	break;
	case Blaze::Perf::PerfSlave::NOTIFY_NOTIFYPERFTUNE:
	{
			
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::handlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPERFTUNE.");
		Blaze::Perf::NotifyPerfTuneResponse* notify = (Blaze::Perf::NotifyPerfTuneResponse*)notification;
		//BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::handlerFunction][" << mPlayerData->getPlayerId() << "]: Is enable ? " << notify->getEnable());
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
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::handlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPINGRESPONSE.");
		Blaze::Perf::NotifyPingResponse* notify = (Blaze::Perf::NotifyPingResponse*)notification;
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::handlerFunction][" << mPlayerData->getPlayerId() << "]: Ping Data : " << notify);
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

void FutPlayerGameSession::handleNotifyPingResponse(Blaze::Perf::NotifyPingResponse* notify, uint16_t type /*= Blaze::Perf::PerfSlave::NOTIFY_NOTIFYPINGRESPONSE*/)
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
			mPlayerData->getShieldExtension()->ping(mPlayerData, true,0);
		}
		else
		{
			//ping for canary or PC device id or loadtestmodule
			if (datasize == 57039)
			{//canary
				BLAZE_TRACE_LOG(BlazeRpcLog::perf, "canary module ping received by playerid:" << mPlayerData->getPlayerId());
				mPlayerData->getShieldExtension()->ping(mPlayerData, false,1);
			}
			else if (datasize == 104727)
			{//pc device id
				//BLAZE_INFO_LOG(BlazeRpcLog::perf, "pc device id ping received by playerid:" << mPlayerData->getPlayerId());
				mPlayerData->getShieldExtension()->ping(mPlayerData, false,2);
			}
			else if (datasize == 119271)
			{//loadtest module
				BLAZE_TRACE_LOG(BlazeRpcLog::perf, "loadtest request received by playerid:" << mPlayerData->getPlayerId());
				mPlayerData->getShieldExtension()->ping(mPlayerData, false,3);
			}
			else
			{
				BLAZE_INFO_LOG(BlazeRpcLog::perf, "empty received by playerid" << mPlayerData->getPlayerId()<<notify->getData());
			}
		}
	}
}
	void FutPlayerGameSession::handleNotifyGameSetUpGroup(NotifyGameSetup* notify, uint16_t type /* = GameManagerSlave::NOTIFY_NOTIFYGAMESETUP*/)
	{
		if (notify->getGameData().getPlatformHostInfo().getPlayerId() == mPlayerData->getPlayerId()) {
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::handleNotifyGameSetUpGroup][" << mPlayerData->getPlayerId() << "," << notify->getGameData().getPlatformHostInfo().getPlayerId() << "}");
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
			else
			{
				mGroupHostConnectionGroupId = notify->getGameData().getTopologyHostInfo().getConnectionGroupId();
				//meshEndpointsConnected RPC to Dedicated server
				BlazeObjectId topologyHostConnId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, mGroupHostConnectionGroupId);
				meshEndpointsConnected(topologyHostConnId, 0, 4, mGameGroupId);
			}

			//updateExtendedDataAttribute(mFriendBlazeId);

			Collections::AttributeMap pattributes;
			pattributes["USER_LATENCY"] = "0";
			pattributes["USER_PRSNT"] = "1";
			char8_t time[MAX_GAMENAME_LEN] = { '\0' };
			blaze_snzprintf(time, sizeof(time), "%" PRId64, Blaze::TimeValue::getTimeOfDay().getSec());
			pattributes["USER_TMSTMP"] = time;
			setPlayerAttributes(mGameGroupId, pattributes);

			if (isGrupPlatformHost)
			{
				Collections::AttributeMap attributes;
				attributes["ADMIN_BADGE"] = "110752";
				attributes["ADMIN_BANN"] = "0";
				attributes["ADMIN_CBAB"] = "Med";
				attributes["ADMIN_CBNM"] = "Medway United";
				attributes["ADMIN_CHEM"] = "53";
				attributes["ADMIN_CRES"] = "110752";
				attributes["ADMIN_ESTD"] = "1588111882";
				attributes["ADMIN_GTAG"] = "q0ae08ba57b-CAen";
				attributes["ADMIN_OVER"] = "66";
				attributes["ADMIN_SQNM"] = "Medway United";
				setGameAttributes(mGameGroupId, attributes);
				attributes.clear();

				if (mScenarioType != FUT_PLAY_ONLINE)
				{
					attributes["PRE_K"] = "1820917760";
					attributes["PRE_T"] = "111140";
					setGameAttributes(mGameGroupId, attributes);
					attributes.clear();
				}
				pattributes.clear();
				pattributes["USER_PRSNT"] = "1";
				blaze_snzprintf(time, sizeof(time), "%" PRId64, Blaze::TimeValue::getTimeOfDay().getSec());
				pattributes["USER_TMSTMP"] = time;
				setPlayerAttributes(mGameGroupId, pattributes);

				if (mScenarioType == FUT_PLAY_ONLINE)
				{
					attributes["SLCTD_GMID"] = "73";
					blaze_snzprintf(time, sizeof(time), "%" PRId64, Blaze::TimeValue::getTimeOfDay().getSec());
					attributes["SLCTD_TMSTMP"] = time;
					setGameAttributes(mGameGroupId, attributes);
					attributes.clear();

					pattributes.clear();
					pattributes["USER_PRSNT"] = "0";
					blaze_snzprintf(time, sizeof(time), "%" PRId64, Blaze::TimeValue::getTimeOfDay().getSec());
					pattributes["USER_TMSTMP"] = time;
					setPlayerAttributes(mGameGroupId, pattributes);

					setClientState(mPlayerData, ClientState::MODE_MULTI_PLAYER, ClientState::Status::STATUS_NORMAL);

					attributes["COM_GMID"] = "73";
					attributes["COM_HOST_SQUAD_UPDATE_TIME"] = "0";
					attributes["COM_KITA"] = "5013505";
					attributes["COM_KITC"] = "16";
					attributes["COM_KITG"] = "0";
					attributes["COM_KITH"] = "29556736";
					attributes["COM_OPP_ABBR"] = "";
					attributes["COM_OPP_BADGE"] = "0";
					attributes["COM_OPP_CHEM"] = "0";
					attributes["COM_OPP_ESTD"] = "0";
					attributes["COM_OPP_NAME"] = "";
					attributes["COM_OPP_OVR"] = "0";
					attributes["COM_SIDE"] = "0";
					attributes["COM_STAD"] = "443";
					setGameAttributes(mGameGroupId, attributes);
					attributes.clear();

					attributes["HR_GAME_TYPE"] = "0";
					setGameAttributes(mGameGroupId, attributes);
					attributes.clear();

					Blaze::Messaging::MessageAttrMap messageAttrMap;
					messageAttrMap[1633839993] = "0";
					sendMessage(mPlayerData, mGameGroupId, messageAttrMap, 1734571116, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
					sendMessage(mPlayerData, mGameGroupId, messageAttrMap, 1734571116, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
					messageAttrMap[1633841779] = "1";
					sendMessage(mPlayerData, mGameGroupId, messageAttrMap, 1734571116, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);

					attributes["ARB_0"] = "1000140657926_0|1000161197584_0|";
					setGameAttributes(mGameGroupId, attributes);
					attributes.clear();

					attributes["ARB_0"] = "1000140657926_1|1000161197584_0|";
					setGameAttributes(mGameGroupId, attributes);
					attributes.clear();

					attributes["ARB_0"] = "1000140657926_1|1000161197584_1|";
					setGameAttributes(mGameGroupId, attributes);
					attributes.clear();
				}
			}
			else
			{
				pattributes["CPVNT_ID"] = "0 1588718834";
				setPlayerAttributes(mGameGroupId, pattributes);

				setClientState(mPlayerData, ClientState::MODE_MULTI_PLAYER, ClientState::Status::STATUS_NORMAL);
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

		void FutPlayerGameSession::handleNotifyGameSetUp(NotifyGameSetup* notify, uint16_t type /* = GameManagerSlave::NOTIFY_NOTIFYGAMESETUP*/)
		{
			BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId());
			if (notify->getGameData().getTopologyHostInfo().getPlayerId() == mPlayerData->getPlayerId())
			{
				isTopologyHost = true;
			}
			if (notify->getGameData().getPlatformHostInfo().getPlayerId() == mPlayerData->getPlayerId())
			{
				isPlatformHost = true;
				//blaze_snzprintf(homeCaptain, sizeof(homeCaptain), "%" PRId64, mPlayerData->getPlayerId());
			}
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

			if (FutPlayerGameDataRef(mPlayerData).isGameIdExist(myGameId))
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "]::receiving notification(" << type << ") and gameId " <<
					notify->getGameData().getGameId() << "already present in FutSeasonsGamesMap");
				//  Player will be added during NOTIFYPLAYERJOINCOMPLETED notification, not here.
			}
			else
			{
				//  create new game data
				FutPlayerGameDataRef(mPlayerData).addGameData(notify);
			}
		
			//meshEndpointsConnected RPC to self
			BlazeObjectId targetGroupId = mPlayerData->getConnGroupId();
			meshEndpointsConnected(targetGroupId, 0, 0, myGameId);
			setClientState(mPlayerData, ClientState::Mode::MODE_MULTI_PLAYER, ClientState::Status::STATUS_NORMAL);
			//meshEndpointsConnected RPC to Dedicated server
			BlazeObjectId topologyHostConnId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, mTopologyHostConnectionGroupId);
			meshEndpointsConnected(topologyHostConnId, 0, 4, myGameId);
		
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
						if (mScenarioType == FUT_PLAY_ONLINE || mScenarioType == FUT_COOP_RIVALS || mScenarioType == FUT_ONLINE_SQUAD_BATTLE)
						{
							//meshEndpointsConnected RPC other players in roster
							BlazeObjectId connId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, (*cit)->getConnectionGroupId());
							getOpponentPlayerInfo().setPlayerId(playerId);
							meshEndpointsConnected(connId, 0, 0, myGameId);
						}
						else if(mScenarioType != FUTCHAMPIONMODE)	//check for other modes
						{
							getOpponentPlayerInfo().setPlayerId(playerId);
							//meshEndpointsConnected RPC to opponent
							targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, (*cit)->getConnectionGroupId());
							meshEndpointsConnected(targetGroupId, 0, 0, myGameId);
						}
					}
				}
			}

			if (mScenarioType == FUT_PLAY_ONLINE || mScenarioType == FUT_COOP_RIVALS || mScenarioType == FUT_ONLINE_SQUAD_BATTLE)
			{
				Collections::AttributeMap attributes;
				attributes["COM_GMID"] = "73";
				attributes["COM_HOST_SQUAD_UPDATE_TIME"] = "0";
				attributes["COM_KITA"] = "5013505";
				attributes["COM_KITC"] = "16";
				attributes["COM_KITG"] = "0";
				attributes["COM_KITH"] = "29556736";
				attributes["COM_OPP_ABBR"] = "";
				attributes["COM_OPP_BADGE"] = "0";
				attributes["COM_OPP_CHEM"] = "0";
				attributes["COM_OPP_ESTD"] = "0";
				attributes["COM_OPP_NAME"] = "";
				attributes["COM_OPP_OVR"] = "0";
				attributes["COM_SIDE"] = "0";
				attributes["COM_STAD"] = "443";
				setGameAttributes(mGameGroupId, attributes);
				attributes.clear();

				attributes["HR_GAME_TYPE"] = "0";
				setGameAttributes(mGameGroupId, attributes);
				attributes.clear();
				if (isPlatformHost)
				{
					Blaze::Messaging::MessageAttrMap messageAttrMap;
					messageAttrMap[1633839993] = "0";
					sendMessage(mPlayerData, myGameId, messageAttrMap, 1734571116, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);

					attributes["COM_GMID"] = "73";
					attributes["COM_HOST_SQUAD_UPDATE_TIME"] = "0";
					attributes["COM_KITA"] = "5013505";
					attributes["COM_KITC"] = "16";
					attributes["COM_KITG"] = "0";
					attributes["COM_KITH"] = "29556736";
					attributes["COM_OPP_ABBR"] = "";
					attributes["COM_OPP_BADGE"] = "0";
					attributes["COM_OPP_CHEM"] = "0";
					attributes["COM_OPP_ESTD"] = "0";
					attributes["COM_OPP_NAME"] = "";
					attributes["COM_OPP_OVR"] = "0";
					attributes["COM_SIDE"] = "0";
					attributes["COM_STAD"] = "443";
					setGameAttributes(mGameGroupId, attributes);
					attributes.clear();

					attributes["ARB_1"] = "1000140657926_0|1000161197584_0|";
					setGameAttributes(mGameGroupId, attributes);
					attributes.clear();

					attributes["ARB_1"] = "1000140657926_1|1000161197584_0|";
					setGameAttributes(mGameGroupId, attributes);
					attributes.clear();

					attributes["ARB_1"] = "1000140657926_1|1000161197584_1|";
					setGameAttributes(mGameGroupId, attributes);
					attributes.clear();

					attributes["ARB_2"] = "1000140657926_0|1000161197584_0|";
					setGameAttributes(mGameGroupId, attributes);
					attributes.clear();

					attributes["ARB_2"] = "1000140657926_1|1000161197584_0|";
					setGameAttributes(mGameGroupId, attributes);
					attributes.clear();

					attributes["ARB_2"] = "1000140657926_1|1000161197584_1|";
					setGameAttributes(mGameGroupId, attributes);
					attributes.clear();
				}
				else
				{
					attributes["ARB_1"] = "1000140657926_0|1000161197584_0|";
					setGameAttributes(mGameGroupId, attributes);
					attributes.clear();

					attributes["ARB_1"] = "1000240536970_1|1000302555391_1|";
					setGameAttributes(mGameGroupId, attributes);
					attributes.clear();

					attributes["ARB_2"] = "1000240536970_0|1000302555391_0|";
					setGameAttributes(mGameGroupId, attributes);
					attributes.clear();

					attributes["ARB_2"] = "1000240536970_0|1000302555391_1|";
					setGameAttributes(mGameGroupId, attributes);
					attributes.clear();

					attributes["ARB_2"] = "1000140657926_1|1000161197584_1|";
					setGameAttributes(mGameGroupId, attributes);
					attributes.clear();
				}
			}
			//updateGameSession(myGameId);

			// Add player to game map
			//mPlayerInGameMap.insert(mPlayerData->getPlayerId());
			//mPlayerInGameMap.insert(notify->getGameData().getPlatformHostInfo().getPlayerId());
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::handleNotifyGameSetUp]" << mPlayerData->getPlayerId() << "HANDLE_NOTIFYGAMESETUP Current Player count for game ID " << getGameId() << " count : " << mPlayerInGameMap.size() << " As follows :");

			if (isPlatformHost && (mScenarioType == FUTONLINESINGLEMATCH || mScenarioType == FUTFRIENDLIES))
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "calling finalizeGameCreation");
				finalizeGameCreation();
			}

			StressGameInfo* ptrGameInfo = FutPlayerGameDataRef(mPlayerData).getGameData(myGameId);
			if (!ptrGameInfo)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "] gameId " << myGameId << " not present in futSeasonsGamesMap");
				return;
			}

			if (isPlatformHost == true && IsPlayerPresentInList((PlayerModule*)(mPlayerData->getOwner()), mPlayerData->getPlayerId(), FUTPLAYER) == true)
			{
				uint16_t count = FutPlayerGameDataRef(mPlayerData).getPlayerCount(myGameId);
				FutPlayerGameDataRef(mPlayerData).setPlayerCount(myGameId, ++count);
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "]:GameId=" << myGameId << " Player count = " << count);
			}
		}



		/*BlazeRpcError	FutPlayerGameSession::updateGameSession(GameId  gameId)
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
		}*/
		BlazeRpcError FutPlayerGameSession::futPlayFriendCreateGame(CreateGameResponse& response)
		{
			BlazeRpcError err = ERR_OK;
			Blaze::GameManager::CreateGameRequest request;

			if (!mPlayerData->isConnected())
			{
				BLAZE_ERR_LOG(BlazeRpcLog::util, "[Friendlies::User Disconnected = " << mPlayerData->getPlayerId());
				return ERR_DISCONNECTED;
			}

			NetworkAddress *hostAddress = &request.getCommonGameData().getPlayerNetworkAddress();
			hostAddress->switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
			hostAddress->getIpPairAddress()->getExternalAddress().setIp(mPlayerData->getConnection()->getAddress().getIp());
			hostAddress->getIpPairAddress()->getExternalAddress().setPort(mPlayerData->getConnection()->getAddress().getPort());
			hostAddress->getIpPairAddress()->getExternalAddress().setMachineId(0);
			hostAddress->getIpPairAddress()->getExternalAddress().copyInto(hostAddress->getIpPairAddress()->getInternalAddress());
			hostAddress->getIpPairAddress()->getMachineId();

			Collections::AttributeMap& gameAttribs = request.getGameCreationData().getGameAttribs();

			// Harrier Change
			if (StressConfigInst.getPlatform() == PLATFORM_XONE)
			{
				gameAttribs["fifaMatchupHash"] = "3939258";
				gameAttribs["futNewUser"] = "1";
			}
			else
			{
				gameAttribs["fifaMatchupHash"] = "4329378";
				gameAttribs["futNewUser"] = "0";
			}
			gameAttribs["FUTFriendlyType"] = "0";
			gameAttribs["futTeamOVR"] = "0";
			gameAttribs["Gamemode_Name"] = "72";//"fut_online_friendly_season";   
			gameAttribs["OSDK_categoryId"] = "gameType72";
			gameAttribs["OSDK_clubId"] = "0";
			gameAttribs["OSDK_coop"] = "1";
			gameAttribs["OSDK_gameMode"] = "72";
			gameAttribs["OSDK_rosterURL"] = "";
			gameAttribs["OSDK_rosterVersion"] = "1";
			gameAttribs["OSDK_sponsoredEventId"] = "0";
			gameAttribs["ssfMatchType"] = "-1";
			gameAttribs["ssfStadium"] = "0";
			gameAttribs["ssfWalls"] = "-1";


			Blaze::EntryCriteriaMap& map = request.getGameCreationData().getEntryCriteriaMap();
			map["OSDK_maxDNF"] = "stats_dnf <= 100";
			request.getPlayerJoinData().setGameEntryType(GAME_ENTRY_TYPE_DIRECT);
			request.getPlayerJoinData().setGroupId(mPlayerData->getConnGroupId());
			if (StressConfigInst.getPlatform() == PLATFORM_XONE)
			{
				request.getGameCreationData().getGameSettings().setBits(0x00040404);
			}
			else
			{
				request.getGameCreationData().getGameSettings().setBits(0x00000404);
			}

			request.setGameReportName("gameType88");

			PerPlayerJoinData* playerJoinData = BLAZE_NEW PerPlayerJoinData();
			playerJoinData->setIsOptionalPlayer(false);
			playerJoinData->getPlayerAttributes().clear();
			playerJoinData->setTeamId(65535);
			playerJoinData->setTeamIndex(65535);
			playerJoinData->setSlotType(INVALID_SLOT_TYPE);
			playerJoinData->setRole("");
			playerJoinData->setEncryptedBlazeId("");
			UserIdentification& userInfo = playerJoinData->getUser();
			userInfo.setAccountLocale(1701729619);
			userInfo.setAccountCountry(21843);
			userInfo.setOriginPersonaId(0);
			userInfo.setPidId(0);
			userInfo.setBlazeId(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
			userInfo.setAccountId(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
			userInfo.setExternalId(mPlayerData->getLogin()->getUserLoginInfo()->getPersonaDetails().getExtId());
			userInfo.setName(mPlayerData->getLogin()->getUserLoginInfo()->getPersonaDetails().getDisplayName());
			if (StressConfigInst.getPlatform() == Platform::PLATFORM_XONE)
			{
				userInfo.setPersonaNamespace("xbox");
			}
			else
			{
				userInfo.setPersonaNamespace("ps3");
			}

			request.getPlayerJoinData().setDefaultRole("");
			request.getPlayerJoinData().setDefaultSlotType(SLOT_PRIVATE_PARTICIPANT);
			request.getPlayerJoinData().setDefaultTeamId(65534);
			request.getPlayerJoinData().setDefaultTeamIndex(0);
			request.getPlayerJoinData().getPlayerDataList().push_back(playerJoinData);

			request.setGamePingSiteAlias("");
			request.setGameEventAddress("");
			request.setGameEndEventUri("");
			request.setServerNotResetable(false);

			request.getGameCreationData().setNetworkTopology(Blaze::PEER_TO_PEER_FULL_MESH);
			request.getGameCreationData().getTeamIds().push_back(65534);
			request.getGameCreationData().setDisconnectReservationTimeout(0);
			request.getGameCreationData().setGameModRegister(0);
			request.getGameCreationData().setGameName("");
			request.getGameCreationData().setPermissionSystemName("");
			request.getGameCreationData().setGameReportName("");
			request.getGameCreationData().setSkipInitializing(false);
			request.getGameCreationData().setMaxPlayerCapacity(2);
			request.getGameCreationData().setMinPlayerCapacity(1);
			request.getGameCreationData().setPresenceMode(PRESENCE_MODE_PRIVATE);
			request.getGameCreationData().setPlayerReservationTimeout(0);
			request.getGameCreationData().setQueueCapacity(0);
			request.getGameCreationData().getExternalSessionStatus().setUnlocalizedValue("");
			request.getGameCreationData().setExternalSessionTemplateName("");

			request.getSlotCapacities().at(SLOT_PUBLIC_PARTICIPANT) = 0;
			request.getSlotCapacities().at(SLOT_PRIVATE_PARTICIPANT) = 2;
			request.getSlotCapacities().at(SLOT_PUBLIC_SPECTATOR) = 0;
			request.getSlotCapacities().at(SLOT_PRIVATE_SPECTATOR) = 0;


			request.getGameCreationData().setMaxPlayerCapacity(2);
			request.getGameCreationData().setMinPlayerCapacity(1);
			request.getGameCreationData().setPresenceMode(Blaze::PRESENCE_MODE_PRIVATE);
			request.getGameCreationData().setQueueCapacity(0);
			//request.getGameCreationData().getExternalDataSourceApiNameList().clear();
			request.getGameCreationData().getExternalSessionStatus().getLangMap().clear();
			request.getGameCreationData().getExternalSessionStatus().setUnlocalizedValue("");
			request.getGameCreationData().setExternalSessionTemplateName("");
			request.getGameCreationData().setVoipNetwork(Blaze::VOIP_PEER_TO_PEER);
			request.setPersistedGameId("");

			request.setGameEventContentType("application/json");
			request.setGameStartEventUri("");
			request.setGameStatusUrl("");
			request.setGameReportName("gameType72");
			request.setGameEndEventUri("");



			request.getCommonGameData().setGameProtocolVersionString(StressConfigInst.getGameProtocolVersionString());
			request.getCommonGameData().setGameType(GAME_TYPE_GAMESESSION);
			request.getTournamentSessionData().setArbitrationTimeout(31536000000000);
			request.getTournamentSessionData().setForfeitTimeout(31449600000000);
			request.getTournamentSessionData().setTournamentDefinition("");
			request.getTournamentSessionData().setScheduledStartTime("1970-01-01T00:00:00Z");

			request.getTournamentIdentification().setTournamentId("");
			request.getTournamentIdentification().setTournamentOrganizer("");

			//if (BLAZE_IS_TRACE_RPC_ENABLED(BlazeRpcLog::gamemanager)) {
				//char8_t buf[10240];
				//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "CreateGame Request: \n"<< request.print(buf, sizeof(buf)));
			//}
			err = mPlayerData->getComponentProxy()->mGameManagerProxy->createGame(request, response);
			if (err != ERR_OK) {
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "FutPlayer::createGame failed. Error(" << ErrorHelp::getErrorName(err) << ")");
			}
			return err;
		}

		BlazeRpcError FutPlayerGameSession::futPlayFriendJoinGame(GameId gameId)
		{

			BlazeRpcError err;
			Blaze::GameManager::JoinGameRequest request;
			Blaze::GameManager::JoinGameResponse response;
			Blaze::EntryCriteriaError error;

			if (!mPlayerData->isConnected())
			{
				BLAZE_ERR_LOG(BlazeRpcLog::util, "[Friendlies::User Disconnected = " << mPlayerData->getPlayerId());
				return ERR_DISCONNECTED;
			}

			request.getCommonGameData().setGameType(GAME_TYPE_GAMESESSION);
			request.getPlayerJoinData().setGameEntryType(GAME_ENTRY_TYPE_DIRECT);

			NetworkAddress *hostAddress = &request.getCommonGameData().getPlayerNetworkAddress();
			hostAddress->switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
			hostAddress->getIpPairAddress()->getExternalAddress().setIp(mPlayerData->getConnection()->getAddress().getIp());
			hostAddress->getIpPairAddress()->getExternalAddress().setPort(mPlayerData->getConnection()->getAddress().getPort());
			hostAddress->getIpPairAddress()->getExternalAddress().setMachineId(0);
			hostAddress->getIpPairAddress()->getExternalAddress().copyInto(hostAddress->getIpPairAddress()->getInternalAddress());
			hostAddress->getIpPairAddress()->getMachineId();

			// GVER = "XBL2/FIFA12"
			request.getCommonGameData().setGameProtocolVersionString(StressConfigInst.getGameProtocolVersionString());
			// JMET = JOIN_BY_INVITES (4) (0x00000004)
			request.setJoinMethod(Blaze::GameManager::JOIN_BY_INVITES);
			// SLID = 255 (0xFF)
			request.setRequestedSlotId(255);
			// SLOT = SLOT_PRIVATE (1) (0x00000001)
			request.getPlayerJoinData().setDefaultSlotType(Blaze::GameManager::SLOT_PRIVATE_PARTICIPANT);
			request.getPlayerJoinData().setGroupId(mPlayerData->getConnGroupId());

			request.getPlayerJoinData().setDefaultSlotType(SLOT_PRIVATE_PARTICIPANT);
			request.getPlayerJoinData().setDefaultTeamId(ANY_TEAM_ID);
			request.getPlayerJoinData().setDefaultTeamIndex(ANY_TEAM_ID + 1);

			// adding player data 
			PerPlayerJoinData* playerJoinData = BLAZE_NEW PerPlayerJoinData();
			playerJoinData->getPlayerAttributes().clear();
			playerJoinData->setTeamId(65535);
			playerJoinData->setTeamIndex(65535);
			playerJoinData->setSlotType(INVALID_SLOT_TYPE);
			playerJoinData->setRole("");
			playerJoinData->setEncryptedBlazeId("");
			playerJoinData->setIsOptionalPlayer(false);
			playerJoinData->setSlotType(INVALID_SLOT_TYPE);
			playerJoinData->getPlayerAttributes().clear();
			playerJoinData->setRole("");
			UserIdentification& userInfo = playerJoinData->getUser();
			userInfo.setAccountLocale(1701729619);
			userInfo.setAccountCountry(21843);
			userInfo.setOriginPersonaId(0);
			userInfo.setPidId(0);
			userInfo.setBlazeId(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
			userInfo.setAccountId(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
			userInfo.setExternalId(mPlayerData->getLogin()->getUserLoginInfo()->getPersonaDetails().getExtId());
			userInfo.setName(mPlayerData->getLogin()->getUserLoginInfo()->getPersonaDetails().getDisplayName());
			if (StressConfigInst.getPlatform() == Platform::PLATFORM_XONE)
			{
				userInfo.setPersonaNamespace("xbox");
			}
			else
			{
				userInfo.setPersonaNamespace("ps3");
			}
			request.getPlayerJoinData().getPlayerDataList().push_back(playerJoinData);

			request.getUser().setAccountLocale(1701729619);
			request.getUser().setAccountCountry(21843);
			request.getUser().setOriginPersonaId(0);
			request.getUser().setPidId(0);
			request.getUser().setBlazeId(0);
			request.getUser().setAccountId(0);
			request.getUser().setExternalId(0);
			request.getUser().setName("");
			request.getUser().setPersonaNamespace("");

			request.setGameId(gameId);
			/*if (BLAZE_IS_TRACE_RPC_ENABLED(BlazeRpcLog::gamemanager)) {
				char8_t buf[10240];
				BLAZE_TRACE_RPC_LOG(BlazeRpcLog::gamemanager, "JoinGame RPC : \n" << request.print(buf, sizeof(buf)));
			}*/
			err = mPlayerData->getComponentProxy()->mGameManagerProxy->joinGame(request, response, error);

			return err;
		}

		void FutPlayerGameSession::addFutPlayFriendCreatedGame(GameId gameId)
		{
			CreatedGamesList* gameListReference = ((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->getFutPlayFriendGameList();
			CreatedGamesList::iterator it = gameListReference->begin();
			while (it != gameListReference->end())
			{
				if (gameId == *it)
				{
					return;
				}
				it++;
			}
			gameListReference->push_back(gameId);
			it = gameListReference->begin();
			while (it != gameListReference->end())
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, " Game ID : " << *it);
				it++;
			}
		}

		GameId FutPlayerGameSession::getRandomCreatedGame()
		{
			CreatedGamesList* gameListReference = ((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->getFutPlayFriendGameList();
			if (gameListReference->size() == 0)
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::getRandomCreatedGame] The gameId map is empty ");
				return 0;
			}
			CreatedGamesList::iterator it = gameListReference->begin();
			GameId randomGameId; int32_t num = 0;

			mMutex.Lock();
			num = Random::getRandomNumber(gameListReference->size());
			eastl::advance(it, num);
			randomGameId = *it;
			gameListReference->erase(it);
			mMutex.Unlock();
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::getRandomCreatedGame]  Randomly fetched GameId : " << randomGameId);
			return randomGameId;
		}

		BlazeRpcError FutPlayerGameSession::submitGameReport(ScenarioType scenarioName)
		{
			BlazeRpcError error = ERR_OK;

			GameReporting::SubmitGameReportRequest request;
			GameReporting::GameReport &gamereport = request.getGameReport();

			gamereport.setGameReportingId(mGameReportingId);

			//if (scenarioName == FUTCHAMPIONMODE)
			//{
			request.setFinishedStatus(GAMEREPORT_FINISHED_STATUS_FINISHED);
			//}

			if (scenarioName == FUTDRAFTMODE)
			{
				gamereport.setGameReportName("gameType80");
			}
			else if (scenarioName == FUTRIVALS)//online_rivals
			{
				gamereport.setGameReportName("gameType76");
			}
			else if (scenarioName == FUT_COOP_RIVALS)//online_rivals
			{
				gamereport.setGameReportName("gameType76");
			}
			else if (scenarioName == FUT_ONLINE_SQUAD_BATTLE)//online_squad_battle
			{
				gamereport.setGameReportName("gameType71");
			}
			else if (scenarioName == FUTONLINESINGLEMATCH)
			{
				gamereport.setGameReportName("gameType81");
			}

			else if (scenarioName == FUTTOURNAMENT)
			{
				gamereport.setGameReportName("gameType82");
			}
			else if (scenarioName == FUTFRIENDLIES)
			{
				gamereport.setGameReportName("gameType72");//online_friendly_house_rules - problem
			}
			else if (scenarioName == FUTCHAMPIONMODE)
			{
				gamereport.setGameReportName("gameType79");
			}
			else if (scenarioName == FUT_PLAY_ONLINE)//online_house_rules
			{
				gamereport.setGameReportName("gameType73");
			}

			// Blaze::GameReporting::OSDKGameReportBase::OSDKReport *osdkReport = static_cast<Blaze::GameReporting::OSDKGameReportBase::OSDKReport *> (gamereport.getReport());
			// Blaze::GameReporting::OSDKGameReportBase::OSDKReport *osdkReport = (Blaze::GameReporting::OSDKGameReportBase::OSDKReport *)(gamereport.getReport());
			Blaze::GameReporting::Fifa::H2HGameReport h2hGameReport;
			Blaze::GameReporting::FutOTPReportBase::FutOTPGameReport futOTPGameReport;
			Blaze::GameReporting::OSDKGameReportBase::OSDKReport *osdkReport = BLAZE_NEW Blaze::GameReporting::OSDKGameReportBase::OSDKReport;
			gamereport.setReport(*osdkReport);
			if (osdkReport != NULL) {
				Blaze::GameReporting::OSDKGameReportBase::OSDKGameReport& osdkGamereport = osdkReport->getGameReport();
				osdkGamereport.setCategoryId(0);
				osdkGamereport.setGameReportId(0);
				// osdkGamereport.setGameTime(5000);
				osdkGamereport.setGameTime(5575);
				osdkGamereport.setSimulate(false);
				osdkGamereport.setLeagueId(0);
				osdkGamereport.setRank(false);
				osdkGamereport.setRoomId(0);
				osdkGamereport.setFinishedStatus(1);

				if (scenarioName == FUTDRAFTMODE)
				{
					osdkGamereport.setGameType("gameType80");
				}
				else if (scenarioName == FUTRIVALS)
				{
					osdkGamereport.setGameType("gameType76");
				}
				else if (scenarioName == FUT_COOP_RIVALS)
				{
					osdkGamereport.setGameType("gameType76");
				}
				else if (scenarioName == FUT_ONLINE_SQUAD_BATTLE)
				{
					osdkGamereport.setGameType("gameType71");
				}
				else if (scenarioName == FUTFRIENDLIES)
				{
					osdkGamereport.setGameType("gameType72");
				}

				else if (scenarioName == FUTONLINESINGLEMATCH)
				{
					osdkGamereport.setGameType("gameType81");
				}
				else if (scenarioName == FUTTOURNAMENT)
				{
					osdkGamereport.setGameType("gameType82");
				}
				else if (scenarioName == FUTCHAMPIONMODE)
				{
					osdkGamereport.setGameType("gameType79");
				}
				else if (scenarioName == FUT_PLAY_ONLINE)
				{
					osdkGamereport.setGameType("gameType73");
				}
				//      Blaze::GameReporting::Fifa::CommonGameReport& commonGameReport = h2hGameReport.getCommonGameReport();
				//      commonGameReport.setWentToPk(0);
				//      commonGameReport.setIsRivalMatch(false);
				//      // For now updating values as per client logs.TODO: need to add values according to server side settings
				//      commonGameReport.setBallId(11);
				//      commonGameReport.setAwayKitId(3979);
				//      commonGameReport.setHomeKitId(4424);
				//      commonGameReport.setStadiumId(26);
				//      const int MAX_TEAM_COUNT = 11;
				//      const int MAX_PLAYER_VALUE = 100000;
				//      // fill awayStartingXI and homeStartingXI values here
				//      for (int iteration = 0 ; iteration < MAX_TEAM_COUNT ; iteration++) {
				//          commonGameReport.getAwayStartingXI().push_back(Random::getRandomNumber(MAX_PLAYER_VALUE));
				//      }
				//      for (int iteration = 0 ; iteration < MAX_TEAM_COUNT ; iteration++) {
				//          commonGameReport.getHomeStartingXI().push_back(Random::getRandomNumber(MAX_PLAYER_VALUE));
				//      }
				//      osdkGamereport.setCustomGameReport(h2hGameReport);
				//      h2hGameReport.setMatchInSeconds(7108);
					  //h2hGameReport.setSponsoredEventId(0);

				//      // const Blaze::Stress::GameData::PlayerMap& playerMap = gamedata->getPlayerMap();
				//      Blaze::GameReporting::OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = osdkReport->getPlayerReports();
				//            
				//      int counter = 0;
					  //int playerMapSize = mPlayerInGameMap.size();
					  //BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::submitGameReport][" << mPlayerData->getPlayerId() << "]:player map size =" << playerMapSize);
					  //if (playerMapSize != 2)
					  //{
					  //	BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::submitGameReport][" << mPlayerData->getPlayerId() << "]:player map size is wrong, return");
					  //	return ERR_SYSTEM;
					  //}
				//      OsdkPlayerReportsMap.reserve(mPlayerInGameMap.size());
					  //for (eastl::set<BlazeId>::iterator citPlayerIt = mPlayerInGameMap.begin(), citPlayerItEnd = mPlayerInGameMap.end();
					  //	citPlayerIt != citPlayerItEnd; ++citPlayerIt) {
					  //	GameManager::PlayerId playerId = *citPlayerIt;
					  //	Blaze::GameReporting::OSDKGameReportBase::OSDKPlayerReport* playerReport = OsdkPlayerReportsMap.allocate_element();
					  //	Blaze::GameReporting::Fifa::H2HPlayerReport *h2hPlayerReport = BLAZE_NEW Blaze::GameReporting::Fifa::H2HPlayerReport;
					  //	Blaze::GameReporting::Fifa::CommonPlayerReport& commonPlayerReport = h2hPlayerReport->getCommonPlayerReport();
					  //	Blaze::GameReporting::Fifa::H2HCollationPlayerData& h2hCollationPlayerData = h2hPlayerReport->getH2HCollationPlayerData();
					  //	Blaze::GameReporting::Fifa::H2HCustomPlayerData& h2hCustomPlayerData = h2hPlayerReport->getH2HCustomPlayerData();
					  //	Blaze::GameReporting::Fifa::SeasonalPlayData& h2hSeasonalPlayData = h2hPlayerReport->getSeasonalPlayData();  // as per the new TTY

					  //	commonPlayerReport.getCommondetailreport().setAveragePossessionLength(0);
					  //	commonPlayerReport.getCommondetailreport().setAverageFatigueAfterNinety(0);
					  //	commonPlayerReport.getCommondetailreport().setInjuriesRed(0);
					  //	commonPlayerReport.getCommondetailreport().setPassesIntercepted(0);
					  //	commonPlayerReport.getCommondetailreport().setPenaltiesAwarded(0);
					  //	commonPlayerReport.getCommondetailreport().setPenaltiesMissed(0);
					  //	commonPlayerReport.getCommondetailreport().setPenaltiesScored(0);
					  //	commonPlayerReport.getCommondetailreport().setPenaltiesSaved(0);

					  //	commonPlayerReport.setAssists(rand() % 12);
					  //	commonPlayerReport.setHasCleanSheets(rand() % 10);
					  //	commonPlayerReport.setGoalsConceded(rand() % 10);
					  //	commonPlayerReport.setCorners(rand() % 15);

					  //	commonPlayerReport.setPassAttempts(rand() % 15);
					  //	commonPlayerReport.setPassesMade(rand() % 12);
					  //	commonPlayerReport.setRating((float)(rand() % 10));
					  //	commonPlayerReport.setSaves(rand() % 12);
					  //	commonPlayerReport.setShots(rand() % 12);
					  //	commonPlayerReport.setTackleAttempts(rand() % 12);
					  //	commonPlayerReport.setTacklesMade(rand() % 15);
					  //	commonPlayerReport.setFouls(rand() % 12);
					  //	commonPlayerReport.setGoals(rand() % 10);
					  //	commonPlayerReport.setInterceptions(rand() % 15);
					  //	commonPlayerReport.setHasMOTM(rand() % 15);
					  //	commonPlayerReport.setOffsides(rand() % 15);
					  //	commonPlayerReport.setOwnGoals(rand() % 10);
					  //	commonPlayerReport.setPkGoals(0);
					  //	commonPlayerReport.setPossession(rand() % 15);
					  //	commonPlayerReport.setRedCard(rand() % 12);
					  //	commonPlayerReport.setShotsOnGoal(rand() % 15);
					  //	commonPlayerReport.setUnadjustedScore(0);
					  //	commonPlayerReport.setYellowCard(rand() % 12);

					  //	h2hCollationPlayerData.setGoalsConceded(rand() % 12);
					  //	h2hCollationPlayerData.setFouls(rand() % 12);
					  //	h2hCollationPlayerData.setGoals(rand() % 15);
					  //	h2hCollationPlayerData.setOffsides(rand() % 15);
					  //	h2hCollationPlayerData.setOwnGoals(rand() % 10);
					  //	h2hCollationPlayerData.setPkGoals(0);
					  //	h2hCollationPlayerData.setRedCard(rand() % 12);
					  //	h2hCollationPlayerData.setYellowCard(rand() % 12);

					  //	h2hCustomPlayerData.setControls((rand() % 10) + 1);
					  //	h2hCustomPlayerData.setGoalAgainst(rand() % 12);
					  //	h2hCustomPlayerData.setNbGuests(rand() % 10);
					  //	h2hCustomPlayerData.setKit(0);
					  //	h2hCustomPlayerData.setOpponentId(getOpponentPlayerInfo().getPlayerId());	//	Need to check
					  //	h2hCustomPlayerData.setShotsAgainst(rand() % 15);
					  //	h2hCustomPlayerData.setShotsAgainstOnGoal(rand() % 12);
					  //	h2hCustomPlayerData.setTeam(rand() % 10);
					  //	h2hCustomPlayerData.setTeamrating(rand() % 150);
					  //	h2hCustomPlayerData.setTeamName("Milano AS");
					  //	h2hCustomPlayerData.setWentToPk(rand() % 10);

					  //	//Set opponent id
					  //	PlayerInGameMap::const_iterator itrEnd = mPlayerInGameMap.end();
					  //	for (PlayerInGameMap::const_iterator itrStart = mPlayerInGameMap.begin(); itrStart != itrEnd; ++itrStart)
					  //	{				
					  //		PlayerId currentPlayerID = *itrStart;				
					  //		if(currentPlayerID != playerId)
					  //		{
					  //			h2hCustomPlayerData.setOpponentId(currentPlayerID);
					  //		}
					  //		//BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "FutPlayerGameSession:submitGameReport::mPlayerInGameMap myGameId = " << myGameId << " , PlaerId= " << mPlayerData->getPlayerId() << " adding report for player " << currentPlayerID);
					  //	} //for

				//          // Adding h2hSeasonalPlayData as per the new TTY
				//          h2hSeasonalPlayData.setCurrentDivision(0);
				//          h2hSeasonalPlayData.setCup_id(0);
				//          h2hSeasonalPlayData.setUpdateDivision(Blaze::GameReporting::Fifa::NO_UPDATE);
					  //	h2hSeasonalPlayData.setGameNumber(0);
				//          h2hSeasonalPlayData.setSeasonId(0);
				//          h2hSeasonalPlayData.setWonCompetition(0);
				//          h2hSeasonalPlayData.setWonLeagueTitle(false);
				if (scenarioName == FUT_PLAY_ONLINE || scenarioName == FUT_COOP_RIVALS|| scenarioName == FUT_ONLINE_SQUAD_BATTLE  || scenarioName == FUTRIVALS)//add gametype71 also
				{
					//Blaze::GameReporting::FutOTPReportBase::FutOTPGameReport futOTPGameReport;
					Blaze::GameReporting::Fifa::CommonGameReport& commonGameReport = futOTPGameReport.getCommonGameReport();
					commonGameReport.setWentToPk(0);
					commonGameReport.setIsRivalMatch(false);
					// For now updating values as per client logs.TODO: need to add values according to server side settings
					commonGameReport.setBallId(11);
					commonGameReport.setAwayKitId(3979);
					commonGameReport.setHomeKitId(4424);
					commonGameReport.setStadiumId(26);
					const int MAX_TEAM_COUNT = 11;
					const int MAX_PLAYER_VALUE = 100000;
					// fill awayStartingXI and homeStartingXI values here
					for (int iteration = 0; iteration < MAX_TEAM_COUNT; iteration++) {
						commonGameReport.getAwayStartingXI().push_back(Random::getRandomNumber(MAX_PLAYER_VALUE));
					}
					for (int iteration = 0; iteration < MAX_TEAM_COUNT; iteration++) {
						commonGameReport.getHomeStartingXI().push_back(Random::getRandomNumber(MAX_PLAYER_VALUE));
					}
					//	STRESS_INFO_LOG(request);
					futOTPGameReport.setMom("");
					osdkGamereport.setCustomGameReport(futOTPGameReport);
					//	STRESS_INFO_LOG(request);
				}
				else
				{
					//Blaze::GameReporting::Fifa::H2HGameReport h2hGameReport;
					Blaze::GameReporting::Fifa::CommonGameReport& commonGameReport = h2hGameReport.getCommonGameReport();
					commonGameReport.setWentToPk(0);
					commonGameReport.setIsRivalMatch(false);
					// For now updating values as per client logs.TODO: need to add values according to server side settings
					commonGameReport.setBallId(11);
					commonGameReport.setAwayKitId(3979);
					commonGameReport.setHomeKitId(4424);
					commonGameReport.setStadiumId(26);
					const int MAX_TEAM_COUNT = 11;
					const int MAX_PLAYER_VALUE = 100000;
					// fill awayStartingXI and homeStartingXI values here
					for (int iteration = 0; iteration < MAX_TEAM_COUNT; iteration++) {
						commonGameReport.getAwayStartingXI().push_back(Random::getRandomNumber(MAX_PLAYER_VALUE));
					}
					for (int iteration = 0; iteration < MAX_TEAM_COUNT; iteration++) {
						commonGameReport.getHomeStartingXI().push_back(Random::getRandomNumber(MAX_PLAYER_VALUE));
					}
					osdkGamereport.setCustomGameReport(h2hGameReport);
					h2hGameReport.setMatchInSeconds(7108);
					h2hGameReport.setSponsoredEventId(0);
				}
				//STRESS_INFO_LOG(request);
				// const Blaze::Stress::GameData::PlayerMap& playerMap = gamedata->getPlayerMap();
				Blaze::GameReporting::OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = osdkReport->getPlayerReports();

				int counter = 0;
				//	eastl::set<BlazeId> mInGamePlayerMap;
					//self
				//	mInGamePlayerMap.insert(mPlayerData->getPlayerId());
					//opponent
				//	mInGamePlayerMap.insert(getOpponentPlayerInfo().getPlayerId());

				int playerMapSize = mPlayerInGameMap.size();
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::submitGameReport][" << mPlayerData->getPlayerId() << "]:player map size =" << playerMapSize);
		
				if (playerMapSize != 4 && (mScenarioType == FUT_COOP_RIVALS || mScenarioType == FUT_PLAY_ONLINE || mScenarioType == FUT_ONLINE_SQUAD_BATTLE))
				{
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::submitGameReport][" << mPlayerData->getPlayerId() << "]:player map size is wrong, return");
					return ERR_SYSTEM;
				}
				if ((playerMapSize != 2) && (mScenarioType != FUT_COOP_RIVALS) && (mScenarioType != FUT_PLAY_ONLINE) && (mScenarioType != FUT_ONLINE_SQUAD_BATTLE))
				{
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::submitGameReport][" << mPlayerData->getPlayerId() << "]:player map size is wrong, return");
					return ERR_SYSTEM;
				}
				OsdkPlayerReportsMap.reserve(mPlayerInGameMap.size());
				for (eastl::set<BlazeId>::iterator citPlayerIt = mPlayerInGameMap.begin(), citPlayerItEnd = mPlayerInGameMap.end();
					citPlayerIt != citPlayerItEnd; ++citPlayerIt) {
					GameManager::PlayerId playerId = *citPlayerIt;
					Blaze::GameReporting::OSDKGameReportBase::OSDKPlayerReport* playerReport = OsdkPlayerReportsMap.allocate_element();
					Blaze::GameReporting::Fifa::SeasonalPlayData seasonalPlayData;
					if (scenarioName == FUT_PLAY_ONLINE || scenarioName == FUT_COOP_RIVALS|| scenarioName == FUT_ONLINE_SQUAD_BATTLE  || scenarioName == FUTRIVALS)//add gametype71 also
					{
						Blaze::GameReporting::FutOTPReportBase::FutOTPPlayerReport *futPlayerReport = BLAZE_NEW Blaze::GameReporting::FutOTPReportBase::FutOTPPlayerReport;
						seasonalPlayData = futPlayerReport->getSeasonalPlayData();  // as per the new TTY
						futPlayerReport->setManOfTheMatch(0);
						futPlayerReport->setPos(5);
						futPlayerReport->setCleanSheetsAny(0);
						futPlayerReport->setCleanSheetsDef(0);
						futPlayerReport->setCleanSheetsGoalKeeper(0);
						futPlayerReport->setControls(2);
						futPlayerReport->setGoalAgainst(rand() % 12);
						futPlayerReport->setIsCaptain(0);
						futPlayerReport->setKit(0);
						futPlayerReport->setTeamEntityId((rand() % 10) + 1);
						futPlayerReport->setOpponentId(getOpponentPlayerInfo().getPlayerId());
						futPlayerReport->setShotsAgainst(rand() % 15);
						futPlayerReport->setShotsAgainstOnGoal(rand() % 12);
						futPlayerReport->setTeam(rand() % 10);
						futPlayerReport->setTeamrating(rand() % 150);
						futPlayerReport->setTeamName("Buenos Aires FC");
						futPlayerReport->setTournamentTeamIndex(0);
						futPlayerReport->setWentToPk(rand() % 10);
						futPlayerReport->setMatchResult(Blaze::GameReporting::FutOTPReportBase::MatchResult::IN_PROGRESS);
						Blaze::GameReporting::Fifa::CommonPlayerReport& commonPlayerReport = futPlayerReport->getCommonPlayerReport();
						commonPlayerReport.getCommondetailreport().setAveragePossessionLength(0);
						commonPlayerReport.getCommondetailreport().setAverageFatigueAfterNinety(0);
						commonPlayerReport.getCommondetailreport().setInjuriesRed(0);
						commonPlayerReport.getCommondetailreport().setPassesIntercepted(0);
						commonPlayerReport.getCommondetailreport().setPenaltiesAwarded(0);
						commonPlayerReport.getCommondetailreport().setPenaltiesMissed(0);
						commonPlayerReport.getCommondetailreport().setPenaltiesScored(0);
						commonPlayerReport.getCommondetailreport().setPenaltiesSaved(0);

						commonPlayerReport.setAssists(rand() % 12);
						commonPlayerReport.setHasCleanSheets(rand() % 10);
						commonPlayerReport.setGoalsConceded(rand() % 10);
						commonPlayerReport.setCorners(rand() % 15);

						commonPlayerReport.setPassAttempts(rand() % 15);
						commonPlayerReport.setPassesMade(rand() % 12);
						commonPlayerReport.setRating((float)(rand() % 10));
						commonPlayerReport.setSaves(rand() % 12);
						commonPlayerReport.setShots(rand() % 12);
						commonPlayerReport.setTackleAttempts(rand() % 12);
						commonPlayerReport.setTacklesMade(rand() % 15);
						commonPlayerReport.setFouls(rand() % 12);
						commonPlayerReport.setGoals(rand() % 10);
						commonPlayerReport.setInterceptions(rand() % 15);
						commonPlayerReport.setHasMOTM(rand() % 15);
						commonPlayerReport.setOffsides(rand() % 15);
						commonPlayerReport.setOwnGoals(rand() % 10);
						commonPlayerReport.setPkGoals(0);
						commonPlayerReport.setPossession(rand() % 15);
						commonPlayerReport.setRedCard(rand() % 12);
						commonPlayerReport.setShotsOnGoal(rand() % 15);
						commonPlayerReport.setUnadjustedScore(0);
						commonPlayerReport.setYellowCard(rand() % 12);
						playerReport->setCustomPlayerReport(*futPlayerReport);
					}
					else
					{
						Blaze::GameReporting::Fifa::H2HPlayerReport *h2hPlayerReport = BLAZE_NEW Blaze::GameReporting::Fifa::H2HPlayerReport;
						Blaze::GameReporting::Fifa::CommonPlayerReport& commonPlayerReport = h2hPlayerReport->getCommonPlayerReport();
						commonPlayerReport.getCommondetailreport().setAveragePossessionLength(0);
						commonPlayerReport.getCommondetailreport().setAverageFatigueAfterNinety(0);
						commonPlayerReport.getCommondetailreport().setInjuriesRed(0);
						commonPlayerReport.getCommondetailreport().setPassesIntercepted(0);
						commonPlayerReport.getCommondetailreport().setPenaltiesAwarded(0);
						commonPlayerReport.getCommondetailreport().setPenaltiesMissed(0);
						commonPlayerReport.getCommondetailreport().setPenaltiesScored(0);
						commonPlayerReport.getCommondetailreport().setPenaltiesSaved(0);

						commonPlayerReport.setAssists(rand() % 12);
						commonPlayerReport.setHasCleanSheets(rand() % 10);
						commonPlayerReport.setGoalsConceded(rand() % 10);
						commonPlayerReport.setCorners(rand() % 15);

						commonPlayerReport.setPassAttempts(rand() % 15);
						commonPlayerReport.setPassesMade(rand() % 12);
						commonPlayerReport.setRating((float)(rand() % 10));
						commonPlayerReport.setSaves(rand() % 12);
						commonPlayerReport.setShots(rand() % 12);
						commonPlayerReport.setTackleAttempts(rand() % 12);
						commonPlayerReport.setTacklesMade(rand() % 15);
						commonPlayerReport.setFouls(rand() % 12);
						commonPlayerReport.setGoals(rand() % 10);
						commonPlayerReport.setInterceptions(rand() % 15);
						commonPlayerReport.setHasMOTM(rand() % 15);
						commonPlayerReport.setOffsides(rand() % 15);
						commonPlayerReport.setOwnGoals(rand() % 10);
						commonPlayerReport.setPkGoals(0);
						commonPlayerReport.setPossession(rand() % 15);
						commonPlayerReport.setRedCard(rand() % 12);
						commonPlayerReport.setShotsOnGoal(rand() % 15);
						commonPlayerReport.setUnadjustedScore(0);
						commonPlayerReport.setYellowCard(rand() % 12);
						Blaze::GameReporting::Fifa::H2HCollationPlayerData& h2hCollationPlayerData = h2hPlayerReport->getH2HCollationPlayerData();
						Blaze::GameReporting::Fifa::H2HCustomPlayerData& h2hCustomPlayerData = h2hPlayerReport->getH2HCustomPlayerData();
						seasonalPlayData = h2hPlayerReport->getSeasonalPlayData();  // as per the new TTY
						h2hCollationPlayerData.setGoalsConceded(rand() % 12);
						h2hCollationPlayerData.setFouls(rand() % 12);
						h2hCollationPlayerData.setGoals(rand() % 15);
						h2hCollationPlayerData.setOffsides(rand() % 15);
						h2hCollationPlayerData.setOwnGoals(rand() % 10);
						h2hCollationPlayerData.setPkGoals(0);
						h2hCollationPlayerData.setRedCard(rand() % 12);
						h2hCollationPlayerData.setYellowCard(rand() % 12);

						h2hCustomPlayerData.setControls((rand() % 10) + 1);
						h2hCustomPlayerData.setGoalAgainst(rand() % 12);
						h2hCustomPlayerData.setNbGuests(rand() % 10);
						h2hCustomPlayerData.setKit(0);
						h2hCustomPlayerData.setOpponentId(getOpponentPlayerInfo().getPlayerId());	//	Need to check
						h2hCustomPlayerData.setShotsAgainst(rand() % 15);
						h2hCustomPlayerData.setShotsAgainstOnGoal(rand() % 12);
						h2hCustomPlayerData.setTeam(rand() % 10);
						h2hCustomPlayerData.setTeamrating(rand() % 150);
						h2hCustomPlayerData.setTeamName("Milano AS");
						h2hCustomPlayerData.setWentToPk(rand() % 10);

						//Set opponent id
						PlayerInGameMap::const_iterator itrEnd = mPlayerInGameMap.end();
						for (PlayerInGameMap::const_iterator itrStart = mPlayerInGameMap.begin(); itrStart != itrEnd; ++itrStart)
						{
							PlayerId currentPlayerID = *itrStart;
							if (currentPlayerID != playerId)
							{
								h2hCustomPlayerData.setOpponentId(currentPlayerID);
							}
							//BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "FutPlayerGameSession:submitGameReport::mInGamePlayerMap myGameId = " << myGameId << " , PlaerId= " << mPlayerData->getPlayerId() << " adding report for player " << currentPlayerID);
						} //for
						// Set Platform host as the winner of the match
						if (isPlatformHost)
						{
							playerReport->setHome(true);
							playerReport->setScore(4);
							h2hCollationPlayerData.setGoalsConceded(4);
							h2hCollationPlayerData.setGoals(4);
						}
						else
						{
							playerReport->setHome(false);
							playerReport->setScore(1);
							h2hCollationPlayerData.setGoalsConceded(1);
							h2hCollationPlayerData.setGoals(1);
						}

						playerReport->setCustomPlayerReport(*h2hPlayerReport);
					}
					// Adding h2hSeasonalPlayData as per the new TTY
					seasonalPlayData.setCurrentDivision(0);
					seasonalPlayData.setCup_id(0);
					seasonalPlayData.setUpdateDivision(Blaze::GameReporting::Fifa::NO_UPDATE);
					seasonalPlayData.setGameNumber(0);
					seasonalPlayData.setSeasonId(0);
					seasonalPlayData.setWonCompetition(0);
					seasonalPlayData.setWonLeagueTitle(false);

					playerReport->setCustomDnf(0);
					//playerReport->setHome(false);
					playerReport->setClientScore(0);
					playerReport->setAccountCountry(0);
					playerReport->setFinishReason(0);
					playerReport->setGameResult(0);
					playerReport->setLosses(0);
					playerReport->setName(mPlayerData->getPersonaName().c_str());
					playerReport->setOpponentCount(0);
					playerReport->setExternalId(0);
					playerReport->setNucleusId(0);
					playerReport->setPersona(mPlayerData->getPersonaName().c_str());
					playerReport->setPointsAgainst(0);
					playerReport->setUserResult(0);
					playerReport->setScore(rand() % 10);
					playerReport->setSkill(0);
					playerReport->setSkillPoints(0);
					playerReport->setTeam(0);
					playerReport->setTies(0);
					playerReport->setWinnerByDnf(0);
					playerReport->setWins(0);

					//local player
					if (playerId == mPlayerData->getPlayerId()) {
						Blaze::GameReporting::OSDKGameReportBase::OSDKPrivatePlayerReport &osdkPrivatePlayerReport = playerReport->getPrivatePlayerReport();
						//Blaze::Collections::AttributeMap &privateAttribMap = osdkPrivatePlayerReport.getPrivateAttributeMap();
						Blaze::GameReporting::OSDKGameReportBase::IntegerAttributeMap &PlayerPrivateIntAttribMap = osdkPrivatePlayerReport.getPrivateIntAttributeMap();
						PlayerPrivateIntAttribMap["BANDAVGGM"] = (uint16_t)Random::getRandomNumber(1000);
						PlayerPrivateIntAttribMap["BANDAVGNET"] = (uint16_t)Random::getRandomNumber(3000);
						PlayerPrivateIntAttribMap["BANDHIGM"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["BANDHINET"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["BANDLOWGM"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["BANDLOWNET"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["BYTESRCVDNET"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["BYTESSENTNET"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["DROPPKTS"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["FPSAVG"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["FPSDEV"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["FPSHI"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["FPSLOW"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["GDESYNCEND"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["GDESYNCRSN"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["GENDPHASE"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["GPCKTPERIOD"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["GPCKTPERIODSTD"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						PlayerPrivateIntAttribMap["GRESULT"] = (uint16_t)Blaze::Random::getRandomNumber(10);
						if (scenarioName == FUTCHAMPIONMODE)
						{
							PlayerPrivateIntAttribMap["GRPTTYPE"] = 79;
						}
						else if (scenarioName == FUTRIVALS || scenarioName == FUT_COOP_RIVALS)
						{
							PlayerPrivateIntAttribMap["GRPTTYPE"] = 76;
						}
						else if (scenarioName == FUT_ONLINE_SQUAD_BATTLE)
						{
							PlayerPrivateIntAttribMap["GRPTTYPE"] = 71;
						}
						else if (scenarioName == FUTDRAFTMODE)
						{
							PlayerPrivateIntAttribMap["GRPTTYPE"] = 80;
						}
						else if (scenarioName == FUT_PLAY_ONLINE)
						{
							PlayerPrivateIntAttribMap["GRPTTYPE"] = 73;
						}
						else if (scenarioName == FUTFRIENDLIES)
						{
							PlayerPrivateIntAttribMap["GRPTTYPE"] = 72;
						}
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
						PlayerPrivateIntAttribMap["futMatchFlags"] = (uint16_t)Blaze::Random::getRandomNumber(100);
						PlayerPrivateIntAttribMap["futMatchResult"] = (rand() % 15);
						PlayerPrivateIntAttribMap["futMatchTime"] = (osdkGamereport.getGameTime());
						PlayerPrivateIntAttribMap["gid"] = getGameId();
						PlayerPrivateIntAttribMap["GAMEENDREASON"] = (uint16_t)Blaze::Random::getRandomNumber(10);

						if (mScenarioType == FUT_COOP_RIVALS  || scenarioName == FUTRIVALS) {
							PlayerPrivateIntAttribMap["player_cup_id"] = 0;
						}
						// Set Platform host as the winner of the match
						if (isPlatformHost)
						{
							PlayerPrivateIntAttribMap["futMatchResult"] = Blaze::GameReporting::FUT::MatchResult::WIN;
						}
						else
						{
							if (Random::getRandomNumber(100) < 50)
							{
								PlayerPrivateIntAttribMap["futMatchResult"] = Blaze::GameReporting::FUT::MatchResult::LOSS;
							}
							else
							{
								PlayerPrivateIntAttribMap["futMatchResult"] = Blaze::GameReporting::FUT::MatchResult::QUIT;
							}
						}
					}//local player
					//// Set Platform host as the winner of the match
					//if (isPlatformHost)
					//{
					//	playerReport->setHome(true);
					//	playerReport->setScore(4);
					//	h2hCollationPlayerData.setGoalsConceded(4);
					//	h2hCollationPlayerData.setGoals(4);
					//}
					//else
					//{
					//	playerReport->setHome(false);
					//	playerReport->setScore(1);
					//	h2hCollationPlayerData.setGoalsConceded(1);
					//	h2hCollationPlayerData.setGoals(1);
					//}
					counter++;
					//playerReport->setCustomPlayerReport(*h2hPlayerReport);
					OsdkPlayerReportsMap[playerId] = playerReport;
				}
				//STRESS_INFO_LOG(request);
				Blaze::GameReporting::FutOTPReportBase::FutOTPTeamSummaryReport *futOTPTeamSummaryReport = NULL;
				futOTPTeamSummaryReport = BLAZE_NEW Blaze::GameReporting::FutOTPReportBase::FutOTPTeamSummaryReport;
				Blaze::GameReporting::FutOTPReportBase::FutOTPTeamReportMap& futOtpTeamReportMap = futOTPTeamSummaryReport->getFutOTPTeamReportMap();
				futOtpTeamReportMap.reserve(2);
				Blaze::GameReporting::FutOTPReportBase::FutOTPTeamReport *teamReport1 = futOtpTeamReportMap.allocate_element();
				Blaze::GameReporting::FutOTPReportBase::FutOTPTeamReport *teamReport2 = futOtpTeamReportMap.allocate_element();

				Blaze::GameReporting::Fifa::CommonPlayerReport& commonTeamReport = teamReport1->getCommonTeamReport();
				Blaze::GameReporting::Fifa::CommonPlayerReport& commonTeamReport2 = teamReport2->getCommonTeamReport();

				if (scenarioName == FUT_PLAY_ONLINE || scenarioName == FUT_COOP_RIVALS || scenarioName == FUT_ONLINE_SQUAD_BATTLE  || scenarioName == FUTRIVALS)//add gametype71 also
				{
					teamReport1->setSquadDisc(0);
					teamReport1->setHome(false);
					teamReport1->setLosses(0);
					teamReport1->setGameResult(0);
					teamReport1->setScore(0);
					teamReport1->setTies(0);
					teamReport1->setWinnerByDnf(0);
					teamReport1->setWins(0);
					//commonTeamReport.setAveragePossessionLength(0);
					commonTeamReport.setAssists(getOpponentPlayerInfo().getPlayerId() % 2);
					//commonTeamReport.setBlocks(0); // Added as per logs
					commonTeamReport.setGoalsConceded(getOpponentPlayerInfo().getPlayerId() % 4);
					commonTeamReport.setHasCleanSheets(0);
					commonTeamReport.setCorners(getOpponentPlayerInfo().getPlayerId() % 5);
					commonTeamReport.setPassAttempts(getOpponentPlayerInfo().getPlayerId() % 15);
					commonTeamReport.setPassesMade(getOpponentPlayerInfo().getPlayerId() % 6);
					commonTeamReport.setRating((float)(getOpponentPlayerInfo().getPlayerId() % 2));
					commonTeamReport.setSaves(getOpponentPlayerInfo().getPlayerId() % 4);
					commonTeamReport.setShots(getOpponentPlayerInfo().getPlayerId() % 6);
					commonTeamReport.setTackleAttempts(getOpponentPlayerInfo().getPlayerId() % 6);
					commonTeamReport.setTacklesMade(getOpponentPlayerInfo().getPlayerId() % 5);
					commonTeamReport.setFouls(getOpponentPlayerInfo().getPlayerId() % 3);
					//commonTeamReport.setAverageFatigueAfterNinety(0);
					commonTeamReport.setGoals(getOpponentPlayerInfo().getPlayerId() % 5);
					//commonTeamReport.setInjuriesRed(0);
					commonTeamReport.setInterceptions(getOpponentPlayerInfo().getPlayerId() % 2);
					commonTeamReport.setHasMOTM(0);
					commonTeamReport.setOffsides(getOpponentPlayerInfo().getPlayerId() % 5);
					commonTeamReport.setOwnGoals(getOpponentPlayerInfo().getPlayerId() % 2);
					commonTeamReport.setPkGoals(getOpponentPlayerInfo().getPlayerId() % 4);
					commonTeamReport.setPossession(getOpponentPlayerInfo().getPlayerId() % 5);
					commonTeamReport.getCommondetailreport().setAveragePossessionLength(0);
					commonTeamReport.getCommondetailreport().setAverageFatigueAfterNinety(0);
					commonTeamReport.getCommondetailreport().setInjuriesRed(0);
					commonTeamReport.getCommondetailreport().setPassesIntercepted(0);
					commonTeamReport.getCommondetailreport().setPenaltiesAwarded(0);
					commonTeamReport.getCommondetailreport().setPenaltiesMissed(0);
					commonTeamReport.getCommondetailreport().setPenaltiesScored(0);
					commonTeamReport.getCommondetailreport().setPenaltiesSaved(0);
					commonTeamReport.setRedCard(getOpponentPlayerInfo().getPlayerId() % 3);
					commonTeamReport.setShotsOnGoal(getOpponentPlayerInfo().getPlayerId() % 5);
					commonTeamReport.setUnadjustedScore(getOpponentPlayerInfo().getPlayerId() % 2);
					commonTeamReport.setYellowCard(getOpponentPlayerInfo().getPlayerId() % 3);
					futOtpTeamReportMap[getOpponentPlayerInfo().getPlayerId()] = teamReport1;

					teamReport2->setSquadDisc(0);
					teamReport2->setHome(true);
					teamReport2->setLosses(0);
					teamReport2->setGameResult(0);
					teamReport2->setScore(1);
					teamReport2->setTies(0);
					teamReport2->setWinnerByDnf(0);
					teamReport2->setWins(0);

					commonTeamReport2.getCommondetailreport().setAveragePossessionLength(0);
					commonTeamReport2.setAssists(mPlayerData->getPlayerId() % 2);
					//commonTeamReport2.setBlocks(0); // missing
					commonTeamReport2.setGoalsConceded(mPlayerData->getPlayerId() % 4);
					commonTeamReport2.setHasCleanSheets(0);
					commonTeamReport2.setCorners(mPlayerData->getPlayerId() % 5);
					commonTeamReport2.setPassAttempts(mPlayerData->getPlayerId() % 15);
					commonTeamReport2.setPassesMade(mPlayerData->getPlayerId() % 6);
					commonTeamReport2.setRating((float)(mPlayerData->getPlayerId() % 2));
					commonTeamReport2.setSaves(mPlayerData->getPlayerId() % 4);
					commonTeamReport2.setShots(mPlayerData->getPlayerId() % 6);
					commonTeamReport2.setTackleAttempts(mPlayerData->getPlayerId() % 6);
					commonTeamReport2.setTacklesMade(mPlayerData->getPlayerId() % 5);
					commonTeamReport2.setFouls(mPlayerData->getPlayerId() % 3);
					commonTeamReport2.getCommondetailreport().setAverageFatigueAfterNinety(0);
					commonTeamReport2.setGoals(mPlayerData->getPlayerId() % 5);
					commonTeamReport2.getCommondetailreport().setInjuriesRed(0);
					commonTeamReport2.setInterceptions(mPlayerData->getPlayerId() % 2);
					commonTeamReport2.setHasMOTM(0);
					commonTeamReport2.setOffsides(mPlayerData->getPlayerId() % 5);
					commonTeamReport2.setOwnGoals(mPlayerData->getPlayerId() % 2);
					commonTeamReport2.setPkGoals(mPlayerData->getPlayerId() % 4);
					commonTeamReport2.setPossession(mPlayerData->getPlayerId() % 5);
					commonTeamReport2.getCommondetailreport().setPassesIntercepted(0);
					commonTeamReport2.getCommondetailreport().setPenaltiesAwarded(0);
					commonTeamReport2.getCommondetailreport().setPenaltiesMissed(0);
					commonTeamReport2.getCommondetailreport().setPenaltiesScored(0);
					commonTeamReport2.getCommondetailreport().setPenaltiesSaved(0);
					commonTeamReport2.setRedCard(mPlayerData->getPlayerId() % 3);
					commonTeamReport2.setShotsOnGoal(mPlayerData->getPlayerId() % 5);
					commonTeamReport2.setUnadjustedScore(mPlayerData->getPlayerId() % 2);
					commonTeamReport2.setYellowCard(mPlayerData->getPlayerId() % 3);
					futOtpTeamReportMap[mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId()] = teamReport2;
					//STRESS_INFO_LOG(request);

					osdkReport->setTeamReports(*futOTPTeamSummaryReport);
				}
				//STRESS_INFO_LOG(request);
			}
			//char8_t buf[40240];
			//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "-> [FutPlayerGameSession][" << mPlayerData->getPlayerId() << "]::submitGameReport" << "\n" << request.print(buf, sizeof(buf)));
			
			error = mPlayerData->getComponentProxy()->mGameReportingProxy->submitGameReport(request);
			if (error == ERR_OK)
			{
				BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "FUT Player::SubmitGamerport successful. mMatchmakingSessionId= " << mMatchmakingSessionId);
			}
			else
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[FUT Player::submitGameReport failed][" << mPlayerData->getPlayerId() << " gameId " << myGameId << error);
			}
			// delete osdkReport;
			// delete gamereport;
			// gamereport = NULL;
			return error;
		}

		void FutPlayerGameSession::addFriendToList(BlazeId playerId)
		{
			FutFriendsList* friendsListReference = ((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->getFutFriendsList();
			FutFriendsList::iterator it = friendsListReference->begin();
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

		BlazeId FutPlayerGameSession::getARandomFriend()
		{
			FutFriendsList* friendsListReference = ((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->getFutFriendsList();
			if (friendsListReference->size() == 0)
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::getARandomFriend] The gameId map is empty " << mPlayerData->getPlayerId());
				return 0;
			}
			FutFriendsList::iterator it = friendsListReference->begin();
			BlazeId randomPlayerId; int32_t num = 0;

			mMutex.Lock();
			num = Random::getRandomNumber(friendsListReference->size());
			eastl::advance(it, num);
			randomPlayerId = *it;
			friendsListReference->erase(it);
			mMutex.Unlock();
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::getARandomFriend]  Randomly fetched playerID : " << randomPlayerId << " " << mPlayerData->getPlayerId());
			return randomPlayerId;
		}

		void FutPlayerGameSession::addFutFriendsToMap(BlazeId hostId, BlazeId friendID)
		{
			FutFriendsMap& friendsMap = ((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->getFutFriendsMap();
			friendsMap[hostId] = friendID;
			friendsMap[friendID] = hostId;
		}

		BlazeId FutPlayerGameSession::getFriendInMap(BlazeId playerID)
		{
			FutFriendsMap& friendsMap = ((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->getFutFriendsMap();
			if (friendsMap.find(playerID) == friendsMap.end())
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "FutPlayerGameSession::getMyFriendInMap FriendId not found " << mPlayerData->getPlayerId());
				return 0;
			}
			return friendsMap[playerID];
		}

		void FutPlayerGameSession::removeFriendFromMap()
		{
			FutFriendsMap& friendsMap = ((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->getFutFriendsMap();
			FutFriendsMap::iterator friendsData = friendsMap.find(mPlayerData->getPlayerId());
			if (friendsData != friendsMap.end())
			{
				friendsMap.erase(friendsData);
			}
		}


	}  // namespace Stress
}  // namespace Blaze
