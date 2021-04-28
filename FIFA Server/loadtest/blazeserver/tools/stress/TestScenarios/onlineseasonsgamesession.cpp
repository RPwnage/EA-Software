//  *************************************************************************************************
//
//   File:    onlineseasonsgamesession.cpp
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
#include "./onlineseasonsgamesession.h"
#include "./utility.h"

using namespace Blaze::GameManager;
using namespace Blaze::GameReporting; 

namespace Blaze {
namespace Stress {

OnlineSeasonsGameSession::OnlineSeasonsGameSession(StressPlayerInfo* playerData, uint32_t GameSize, GameNetworkTopology Topology, GameProtocolVersionString GameProtocolVersionString)
    : GameSession(playerData, GameSize, Topology, GameProtocolVersionString)
{
}

OnlineSeasonsGameSession::~OnlineSeasonsGameSession()
{
    BLAZE_INFO(BlazeRpcLog::util, "[OnlineSeasonsGameSession][Destructor][%" PRIu64 "]: destroy called", mPlayerData->getPlayerId());
}

void OnlineSeasonsGameSession::asyncHandlerFunction(uint16_t type, EA::TDF::Tdf *notification)
{
    if ( mPlayerData != NULL && mPlayerData->isPlayerActive())
    {
            //  This Notification has not been handled so use the Default Handler
            onGameSessionDefaultAsyncHandler(type, notification);
    }
}

void OnlineSeasonsGameSession::onGameSessionDefaultAsyncHandler(uint16_t type, EA::TDF::Tdf* notification)
{
    //  This is abstract class and handle the async handlers in the derived class

    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]:Message Type =" << type);
    switch (type)
    {
        case GameManagerSlave::NOTIFY_NOTIFYGAMESETUP:
            //  Async notification sent to the player who joined a game.  Triggered by JoinGame Request or a Matchmaking session finding a game.
            {
                NotifyGameSetup* notify = (NotifyGameSetup*)notification;
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::asyncHandlerFunction]: Received  NOTIFY_NOTIFYGAMESETUP." << mPlayerData->getPlayerId());
				if (notify->getGameData().getGameType() == GAME_TYPE_GAMESESSION)
				{
					handleNotifyGameSetUp(notify);
				}
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMESTATECHANGE:
            {
                NotifyGameStateChange* notify = (NotifyGameStateChange*)notification;
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMESTATECHANGE, state=" <<
                                notify->getNewGameState());
                if (myGameId == notify->getGameId())
                {
					if( getPlayerState() == IN_GAME && notify->getNewGameState() == POST_GAME )
					{
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::NOTIFY_NOTIFYGAMESTATECHANGE][" << mPlayerData->getPlayerId() << "]: submitGameReport");
						submitGameReport(mScenarioType);
						leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, myGameId);
					}

                    //  Set Game State
                    setPlayerState(notify->getNewGameState());
                    OnlineSeasonsGameDataRef(mPlayerData).setGameState(myGameId, notify->getNewGameState());
                }
            }
            break;
		case GameReportingSlave::NOTIFY_RESULTNOTIFICATION:
			{
				ResultNotification* notify = (ResultNotification*)notification;
				//char8_t buf[20240];
				//BLAZE_TRACE_LOG(BlazeRpcLog::gamereporting, "NOTIFY_RESULTNOTIFICATION  : \n" << notify->print(buf,sizeof(buf)));
				if(notify->getFinalResult())
				{
					BLAZE_INFO_LOG(BlazeRpcLog::gamereporting, " THe game reporting for ID : "<<notify->getGameReportingId() <<" PASSED ");
				}
			}
			break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMERESET:
            {
                NotifyGameReset* notify = (NotifyGameReset*)notification;
                //StressGameInfo* gameInfo 
				StressGameInfo* ptrGameInfo= OnlineSeasonsGameDataRef(mPlayerData).getGameData(notify->getGameData().getGameId());
                if (ptrGameInfo != NULL && ptrGameInfo->getPlatformHostPlayerId() == mPlayerData->getPlayerId())
                {
                    //  TODO: host specific actions
                }
                BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMERESET.gameId=" <<
                               notify->getGameData().getGameId());
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINING:
            {
                NotifyPlayerJoining* notify = (NotifyPlayerJoining*)notification;
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERJOINING.gameId=" << notify->getGameId());
			
				//Add opponent player info if not received in notifygamesetup
				if (getOpponentPlayerInfo().getPlayerId() == 0 && mPlayerData->getPlayerId() != notify->getJoiningPlayer().getPlayerId() && myGameId == notify->getGameId())
				{					
					getOpponentPlayerInfo().setPlayerId(notify->getJoiningPlayer().getPlayerId());
					getOpponentPlayerInfo().setConnGroupId(BlazeObjectId(ENTITY_TYPE_CONN_GROUP, notify->getJoiningPlayer().getConnectionGroupId()));
					BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << " OpponentPlayerInfo PlayerId=" << getOpponentPlayerInfo().getPlayerId() << " ConnGroupId=" << getOpponentPlayerInfo().getConnGroupId().toString());

					//meshEndpointsConnected RPC to opponent player
					BlazeObjectId targetGroupId = getOpponentPlayerInfo().getConnGroupId();
					//If Location MM enabled include latency in the meshEndpointsConnected RPC request
					if(StressConfigInst.isGeoLocation_MM_Enabled())
					{
						LocationInfo opponentLocation;
						if( ((PlayerModule*)mPlayerData->getOwner())->getPlayerLocationDataFromMap(getOpponentPlayerInfo().getPlayerId(), opponentLocation) == false)
						{
							BlazeRpcError error = userSessionsLookupUserGeoIPData(mPlayerData, getOpponentPlayerInfo().getPlayerId(), opponentLocation);
							if(error != ERR_OK) { opponentLocation.latitude = opponentLocation.longitude = 0; }
						}
						double distnace = haversine(mPlayerData->getLocation().latitude, mPlayerData->getLocation().longitude, opponentLocation.latitude, opponentLocation.longitude);
						meshEndpointsConnected(targetGroupId, (uint32_t)calculateLatency(distnace));	
					}
					else
					{
						meshEndpointsConnected(targetGroupId);	
					}	
				}
				//The below case is handled in notifygamesetup() function
				//  Add Player Data and increase player count if found in same stress exe.
                /*if ( myGameId == notify->getGameId() && OnlineSeasonsGameDataRef(mPlayerData).isPlayerExistInGame(notify->getJoiningPlayer().getPlayerId(), myGameId) == false )
                {
                    OnlineSeasonsGameDataRef(mPlayerData).addPlayerToGameInfo(notify);
                    if (IsPlayerPresentInList((PlayerModule*)(mPlayerData->getOwner()), notify->getJoiningPlayer().getPlayerId(), ONLINESEASONS) == true)
                    {
                        uint16_t count = OnlineSeasonsGameDataRef(mPlayerData).getPlayerCount(myGameId);
                        OnlineSeasonsGameDataRef(mPlayerData).setPlayerCount(myGameId, --count);
                        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::asyncHandlerFunction::NOTIFYPLAYERJOINING][" << mPlayerData->getPlayerId() << "]:GameId=" << myGameId << " Player count = " <<
                                        count << "joiner=" << notify->getJoiningPlayer().getPlayerId());
                    }
                }	
				//  if all players, in single exe, removed from the game delete the data from map
				if (ptrGameInfo->getPlayerCount() <= 0)
				{
					BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameDataRef::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERREMOVED - Game Id = " <<
						notify->getGameId() << ".");
					OnlineSeasonsGameDataRef(mPlayerData).removeGameData(notify->getGameId());
				}
				if (mPlayerData->getPlayerId() == notify->getJoiningPlayer().getPlayerId() && myGameId == notify->getGameId())
				{
					setPlayerState(DESTRUCTING);
				}*/
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS:
            {
                BLAZE_INFO(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::handlerFunction][%" PRIu64 "]: Received  NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS.", mPlayerData->getPlayerId());
                //NotifyGameSetup* notify = (NotifyGameSetup*)notification;
                
                //  In quickmatch scenario NOTIFYGAMESETUP is not recieved anymore.Instead NOTIFYJOININGPLAYERINITIATECONNECTIONS contains all the game related data
                //handleNotifyGameSetUp(notify, GameManagerSlave::NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS);
				
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLATFORMHOSTINITIALIZED:
            {
                NotifyPlatformHostInitialized* notify = (NotifyPlatformHostInitialized*)notification;
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLATFORMHOSTINITIALIZED.gameId=" <<
                                notify->getGameId());
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMELISTUPDATE:
            {
                //NotifyGameListUpdate* notify = (NotifyGameListUpdate*)notification;
                BLAZE_INFO(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::asyncHandlerFunction][%" PRIu64 "]: Received  NOTIFY_NOTIFYGAMELISTUPDATE.", mPlayerData->getPlayerId());
                
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINCOMPLETED:
            {
                NotifyPlayerJoinCompleted* notify = (NotifyPlayerJoinCompleted*)notification;
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERJOINCOMPLETED." << notify->getPlayerId());
				if (notify->getGameId() == myGameId)
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameDataRef::NOTIFYPLAYERJOINCOMPLETED:mPlayerInGameMap][ GameID : " << myGameId << " playerID= " << mPlayerData->getPlayerId() << " joinerId " << notify->getPlayerId());

					if (IsPlayerPresentInList((PlayerModule*)(mPlayerData->getOwner()), notify->getPlayerId(), DROPINPLAYER) == true)
					{
						uint16_t count = OnlineSeasonsGameDataRef(mPlayerData).getPlayerCount(myGameId);
						OnlineSeasonsGameDataRef(mPlayerData).setPlayerCount(myGameId, ++count);
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameDataRef::NOTIFYPLAYERJOINCOMPLETED][" << mPlayerData->getPlayerId() << "]:GameId=" << myGameId << " Player count = " << count);
					}

					invokeUdatePrimaryExternalSessionForUser(mPlayerData, myGameId, "", "", myGameId, GAME_JOIN, GAME_TYPE_GROUP, ACTIVE_CONNECTED);
				}
				if (isPlatformHost)
				{
					invokeUdatePrimaryExternalSessionForUser(mPlayerData, myGameId, "", "", myGameId, GAME_CREATE, GAME_TYPE_GROUP, ACTIVE_CONNECTED);
				}
			}
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLAYERREMOVED:
            {
                NotifyPlayerRemoved* notify = (NotifyPlayerRemoved*)notification;
				//safe check when player removed due to location based MM fails
				
				if (notify->getGameId() == myGameId)
				{
					invokeUdatePrimaryExternalSessionForUser(mPlayerData, myGameId, "", "", myGameId, GAME_LEAVE, GAME_TYPE_GROUP, ACTIVE_CONNECTED);
				}
				StressGameInfo* ptrGameInfo = OnlineSeasonsGameDataRef(mPlayerData).getGameData(myGameId);
				if (!ptrGameInfo)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]::receiving notification(" << type << ") and gameId " << myGameId <<
                                    "not present in onlineSeasonsGamesMap");
                    break;
                }

                if (mPlayerData->getPlayerId() == notify->getPlayerId() && myGameId == notify->getGameId() )
                {
                    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERREMOVED for game [" << notify->getGameId() << "] reason: "<<notify->getPlayerRemovedReason());
					if (isPlatformHost)
					{
						//meshEndpointsConnectionLost - to other player
						//meshEndpointsDisconnected - DISCONNECTED_PLAYER_REMOVED
						//meshEndpointsDisconnected - 30722/2/0, DISCONNECTED
						//meshEndpointsConnected RPC 
						BlazeObjectId targetGroupId = getOpponentPlayerInfo().getConnGroupId();
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << " meshEndpointsDisconnected for grouID" << targetGroupId.toString());
						if (mScenarioType == SEASONS)
						{
							meshEndpointsConnectionLost(targetGroupId, 0, myGameId);
						}
						if (mScenarioType == SEASONS)
						{
							meshEndpointsDisconnected(targetGroupId, DISCONNECTED_PLAYER_REMOVED);
							preferredJoinOptOut(myGameId);
							preferredJoinOptOut(myGameId);
							preferredJoinOptOut(myGameId);
							preferredJoinOptOut(myGameId);
						}
						
						targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, 0);				
						meshEndpointsDisconnected(targetGroupId, DISCONNECTED);	
					}
					else
					{
						//meshEndpointsDisconnected - DISCONNECTED
						BlazeObjectId targetGroupId = getOpponentPlayerInfo().getConnGroupId();
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << " meshEndpointsDisconnected for grouID" << targetGroupId.toString());
						
						if (mScenarioType == SEASONSCUP)
						{
							meshEndpointsConnectionLost(targetGroupId, 0, myGameId);
						}
						
						meshEndpointsDisconnected(targetGroupId, DISCONNECTED);	

						if (mScenarioType == SEASONSCUP)
						{
							preferredJoinOptOut(myGameId);
							preferredJoinOptOut(myGameId);
							preferredJoinOptOut(myGameId);
							preferredJoinOptOut(myGameId);
						}
					}                 
                }

                if ( myGameId == notify->getGameId()  && OnlineSeasonsGameDataRef(mPlayerData).isPlayerExistInGame(notify->getPlayerId(), notify->getGameId()) == true )
                {
                    OnlineSeasonsGameDataRef(mPlayerData).removePlayerFromGameData(notify->getPlayerId(), notify->getGameId());
                    if (IsPlayerPresentInList((PlayerModule*)(mPlayerData->getOwner()), notify->getPlayerId(), ONLINESEASONS) == true)
                    {
                        uint16_t count = ptrGameInfo->getPlayerCount();
                        ptrGameInfo->setPlayerCount(--count);
                        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::asyncHandlerFunction]::NOTIFYPLAYERREMOVED[" << mPlayerData->getPlayerId() << "]:GameId=" << myGameId << " Player count = " << count
                                        << "for player=" << notify->getPlayerId());
                    }
                }
                //  if all players, in single exe, removed from the game delete the data from map
                if ( ptrGameInfo->getPlayerCount() <= 0)
                {
                    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERREMOVED - Game Id = " <<
                                   notify->getGameId() << ".");
                    OnlineSeasonsGameDataRef(mPlayerData).removeGameData(notify->getGameId());
                }		
				if (mPlayerData->getPlayerId() == notify->getPlayerId() && myGameId == notify->getGameId())
				{
					setPlayerState(DESTRUCTING);
				}
            }
			
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMEREMOVED:
            {
                //  NotifyGameRemoved* notify = (NotifyGameRemoved*)notification;
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMEREMOVED.");
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
                            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::OnNotifyMatchmakingFailed][" << mPlayerData->getPlayerId() << "]: Matchmake SESSION_TIMED_OUT.");
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
				unsubscribeFromCensusData(mPlayerData);
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
                NotifyHostMigrationStart* notify = (NotifyHostMigrationStart*) notification;
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
				if( notify->getUserInfo().getBlazeId() == mPlayerData->getPlayerId() )
				{
					
					Blaze::ObjectIdList::iterator iter = notify->getExtendedData().getBlazeObjectIdList().begin();
					while(iter != notify->getExtendedData().getBlazeObjectIdList().end())
					{
						if(iter->type.component == (uint16_t)11 )
						{
							setMyClubId(iter->id);
							BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::handlerFunction][" << mPlayerData->getPlayerId() << "]: Received  ClubId from NOTIFY_USERADDED : " << getMyClubId());
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
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::handlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYMESSAGE.");
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

void OnlineSeasonsGameSession::handleNotifyGameSetUp(NotifyGameSetup* notify, uint16_t type /* = GameManagerSlave::NOTIFY_NOTIFYGAMESETUP*/)
{
    if (notify->getGameData().getTopologyHostInfo().getPlayerId() == mPlayerData->getPlayerId())  
    {
            isTopologyHost = true;
    }
    if (notify->getGameData().getPlatformHostInfo().getPlayerId() == mPlayerData->getPlayerId())  {
        isPlatformHost = true;
    }
    /*****  Add Game Data *****/
    //  Set My Game ID
    setGameId(notify->getGameData().getGameId());
    
    //  Set Game State
    setPlayerState(notify->getGameData().getGameState());
    if (OnlineSeasonsGameDataRef(mPlayerData).isGameIdExist(myGameId))
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]::receiving notification(" << type << ") and gameId " <<
                        notify->getGameData().getGameId() << "already present in onlineSeasonsGamesMap");
        //  Player will be added during NOTIFYPLAYERJOINCOMPLETED notification, not here.
    }
    else
    {
		//	checking if another thread already created the gameData
		if (!OnlineSeasonsGameDataRef(mPlayerData).getGameData(myGameId))
		{
			//  create new game data
			OnlineSeasonsGameDataRef(mPlayerData).addGameData(notify);
		}
    }

	StressGameInfo *ptrGameInfo = OnlineSeasonsGameDataRef(mPlayerData).getGameData(myGameId);
    if (!ptrGameInfo)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "] gameId " << myGameId <<" not present in onlineSeasonsGamesMap");
		return ;
    }

	//Add opponent player info
	PlayerIdListMap::iterator start = ptrGameInfo->getPlayerIdListMap().begin();
	PlayerIdListMap::iterator end   = ptrGameInfo->getPlayerIdListMap().end();
	for( ; start != end; ++start)
	{
		if(mPlayerData->getPlayerId() != start->first)
		{
			getOpponentPlayerInfo().setPlayerId(start->first);
			getOpponentPlayerInfo().setConnGroupId(BlazeObjectId(ENTITY_TYPE_CONN_GROUP, start->second->connGroupId));
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << " OpponentPlayerInfo PlayerId=" << getOpponentPlayerInfo().getPlayerId() << " ConnGroupId=" << getOpponentPlayerInfo().getConnGroupId().toString());
		}
	}

	//meshEndpointsConnected RPC to self
	BlazeObjectId targetGroupId = mPlayerData->getConnGroupId();
	meshEndpointsConnected(targetGroupId);

	if(getOpponentPlayerInfo().getPlayerId() != 0)
	{
		//meshEndpointsConnected RPC to opponent player
		targetGroupId = getOpponentPlayerInfo().getConnGroupId();
		//If Location MM enabled include latency in the meshEndpointsConnected RPC request
		if(StressConfigInst.isGeoLocation_MM_Enabled())
		{
			LocationInfo opponentLocation;
			if( ((PlayerModule*)mPlayerData->getOwner())->getPlayerLocationDataFromMap(getOpponentPlayerInfo().getPlayerId(), opponentLocation) == false)
			{
				BlazeRpcError error = userSessionsLookupUserGeoIPData(mPlayerData, getOpponentPlayerInfo().getPlayerId(), opponentLocation);
				if(error != ERR_OK) { opponentLocation.latitude = opponentLocation.longitude = 0; }
			}
			double distnace = haversine(mPlayerData->getLocation().latitude, mPlayerData->getLocation().longitude, opponentLocation.latitude, opponentLocation.longitude);
			meshEndpointsConnected(targetGroupId, (uint32_t)calculateLatency(distnace));	
		}
		else
		{
			meshEndpointsConnected(targetGroupId);	
		}
	}
	if (isPlatformHost)
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "calling finalizeGameCreation");
        finalizeGameCreation();

		//requesting platform host
		//requestPlatformHost(mPlayerData, getGameId(), 0);
    }


	//  Increase player count if found in same stress exe.
    if (IsPlayerPresentInList((PlayerModule*)(mPlayerData->getOwner()), mPlayerData->getPlayerId(), ONLINESEASONS) == true)
    {
        uint16_t count = OnlineSeasonsGameDataRef(mPlayerData).getPlayerCount(myGameId);
        OnlineSeasonsGameDataRef(mPlayerData).setPlayerCount(myGameId, ++count);
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "]:GameId=" << myGameId << " Player count = " << count);
    }
}

BlazeRpcError OnlineSeasonsGameSession::submitGameReport(ScenarioType scenarioName)
{
	BlazeRpcError error = ERR_OK;
	//Blaze::GameReporting::Fifa::Substitution *substitution = NULL;

	StressGameInfo* ptrGameInfo = OnlineSeasonsGameDataRef(mPlayerData).getGameData(myGameId);
    if (!ptrGameInfo)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasonsGameSession::submitGameReport][" << mPlayerData->getPlayerId() << " gameId " << myGameId << " not present in onlineSeasonsGamesMap");
		return ERR_SYSTEM;
    }

    GameReporting::SubmitGameReportRequest request;
    const int minScore = 3;
	request.setFinishedStatus(Blaze::GameReporting::GAMEREPORT_FINISHED_STATUS_FINISHED);

    GameReporting::GameReport &gamereport = request.getGameReport();

	gamereport.setGameReportingId(ptrGameInfo->getGameReportingId());

	if( scenarioName == SEASONS )
	{
		gamereport.setGameReportName("gameType0");
	}
	//SEASONSCUP
	else
	{
		gamereport.setGameReportName("gameType1");	
	}
    // Blaze::GameReporting::OSDKGameReportBase::OSDKReport *osdkReport = static_cast<Blaze::GameReporting::OSDKGameReportBase::OSDKReport *> (gamereport.getReport());
    // Blaze::GameReporting::OSDKGameReportBase::OSDKReport *osdkReport = (Blaze::GameReporting::OSDKGameReportBase::OSDKReport *)(gamereport.getReport());
    GameReporting::Fifa::H2HGameReport h2hGameReport;
    GameReporting::OSDKGameReportBase::OSDKReport *osdkReport = BLAZE_NEW GameReporting::OSDKGameReportBase::OSDKReport;
    gamereport.setReport(*osdkReport);

	/*substitution = BLAZE_NEW Blaze::GameReporting::Fifa::Substitution;
	if (substitution == NULL) {
		BLAZE_ERR(BlazeRpcLog::gamemanager, "[OfflineGameReportInstance]startgamereporting function call failed, failed to allocate memory for variable substitution");
		return ERR_SYSTEM;
	}*/

    if (osdkReport != NULL) {
        GameReporting::OSDKGameReportBase::OSDKGameReport& osdkGamereport = osdkReport->getGameReport();
        osdkGamereport.setGameTime(5525);
		osdkGamereport.setArenaChallengeId(0);
		osdkGamereport.setCategoryId(0);
		osdkGamereport.setGameReportId(0);
		osdkGamereport.setSimulate(false);
		osdkGamereport.setLeagueId(0);
		osdkGamereport.setRank(false);
		osdkGamereport.setRoomId(0);

		if( scenarioName == SEASONS )
		{
			osdkGamereport.setGameType("gameType0");
		}
		//SEASONSCUP
		else
		{
			osdkGamereport.setGameType("gameType1");
		}
        osdkGamereport.setFinishedStatus(1);
        GameReporting::Fifa::CommonGameReport& commonGameReport = h2hGameReport.getCommonGameReport();
        commonGameReport.setWentToPk(0);
		commonGameReport.setIsRivalMatch(false);  // As per the new TTY;
		commonGameReport.setAwayKitId(1360);
		commonGameReport.setBallId(120);
		commonGameReport.setHomeKitId(1362);
		commonGameReport.setStadiumId(246);
		//substitution->setPlayerSentOff(Blaze::Random::getRandomNumber(250000));
		//substitution->setPlayerSubbedIn(Blaze::Random::getRandomNumber(250000));
		//substitution->setSubTime(67);
		//commonGameReport.getAwaySubList().push_back(substitution);
		for (int i = 0; i<11; i++)
		{	
			commonGameReport.getAwayStartingXI().push_back( ( ((uint32_t)rand()) % 300000 ) + 100000);
			commonGameReport.getHomeStartingXI().push_back( ( ((uint32_t)rand()) % 300000 ) + 100000);
		}
        h2hGameReport.setMatchInSeconds(5525);
		h2hGameReport.setSponsoredEventId(mEventId);
        osdkGamereport.setCustomGameReport(h2hGameReport);
		//Push players in-game into players list
		eastl::set<BlazeId> mPlayerInGameMap;
		//self
		mPlayerInGameMap.insert(mPlayerData->getPlayerId());
		//opponent
		mPlayerInGameMap.insert(getOpponentPlayerInfo().getPlayerId());

		// const Blaze::Stress::GameData::PlayerMap& playerMap = gamedata->getPlayerMap();
        GameReporting::OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = osdkReport->getPlayerReports();
        OsdkPlayerReportsMap.reserve(mPlayerInGameMap.size());
        for (eastl::set<BlazeId>::iterator citPlayerIt = mPlayerInGameMap.begin(), citPlayerItEnd = mPlayerInGameMap.end();
            citPlayerIt != citPlayerItEnd; ++citPlayerIt) {
            GameManager::PlayerId playerId = *citPlayerIt;
            GameReporting::OSDKGameReportBase::OSDKPlayerReport* playerReport = OsdkPlayerReportsMap.allocate_element();
            GameReporting::Fifa::H2HPlayerReport *h2hPlayerReport = BLAZE_NEW GameReporting::Fifa::H2HPlayerReport;
            GameReporting::Fifa::CommonPlayerReport& commonPlayerReport = h2hPlayerReport->getCommonPlayerReport();
			GameReporting::Fifa::H2HCollationPlayerData& h2hCollationPlayerData = h2hPlayerReport->getH2HCollationPlayerData();
            GameReporting::Fifa::H2HCustomPlayerData& h2hCustomPlayerData = h2hPlayerReport->getH2HCustomPlayerData();
            GameReporting::Fifa::SeasonalPlayData& h2hSeasonalPlayData = h2hPlayerReport->getSeasonalPlayData();  // as per the new TTY
            // Calling the set functions as per the new TTY
            // Followed the order of calling functions as per the new TTY
            commonPlayerReport.setAssists(rand() % 10);
            commonPlayerReport.setGoalsConceded(rand() % 10);
			commonPlayerReport.setHasCleanSheets(rand() % 10);
			commonPlayerReport.getCommondetailreport().setAveragePossessionLength((uint16_t)Random::getRandomNumber(10) +  10);
			commonPlayerReport.getCommondetailreport().setAverageFatigueAfterNinety(100*((uint16_t)Random::getRandomNumber(4) + 4) + (uint16_t)Random::getRandomNumber(100));
			commonPlayerReport.getCommondetailreport().setInjuriesRed((uint16_t)Random::getRandomNumber(5));
			commonPlayerReport.getCommondetailreport().setPassesIntercepted((uint16_t)Random::getRandomNumber(10));
			commonPlayerReport.getCommondetailreport().setPenaltiesAwarded((uint16_t)Random::getRandomNumber(5));
			commonPlayerReport.getCommondetailreport().setPenaltiesMissed((uint16_t)Random::getRandomNumber(5));
			commonPlayerReport.getCommondetailreport().setPenaltiesScored((uint16_t)Random::getRandomNumber(5));
			commonPlayerReport.getCommondetailreport().setPenaltiesSaved((uint16_t)Random::getRandomNumber(5));

            commonPlayerReport.setCorners(rand() % 15);
            commonPlayerReport.setPassAttempts(rand() % 10);
            commonPlayerReport.setPassesMade(rand() % 10);
            commonPlayerReport.setRating((float)(rand() % 10));
            commonPlayerReport.setSaves(rand() % 10);
            commonPlayerReport.setShots(rand() % 10);
            commonPlayerReport.setTackleAttempts(rand() % 10);
            commonPlayerReport.setTacklesMade(rand() % 15);
            commonPlayerReport.setFouls(rand() % 15);
            commonPlayerReport.setGoals(rand() % 15);
            commonPlayerReport.setInterceptions(rand() % 15);  // as per the new TTY
            // mHasMOTM = 1 (0x01) need to be set to 1 as per the new TTY16
			commonPlayerReport.setHasMOTM(rand() % 15);
            commonPlayerReport.setOffsides(rand() % 15);
            commonPlayerReport.setOwnGoals(rand() % 10);
            commonPlayerReport.setPkGoals(0);
            commonPlayerReport.setPossession(rand() % 15);
            commonPlayerReport.setRedCard(rand() % 15);
            commonPlayerReport.setShotsOnGoal(rand() % 15);
            commonPlayerReport.setUnadjustedScore(rand() % 15);  // as per the new TTY
            commonPlayerReport.setYellowCard(rand() % 15);
            h2hCustomPlayerData.setControls((rand() % 10) + 1);
			//  h2hCollationPlayerData is new addition w.r.t. latest TTY logs
			h2hCollationPlayerData.setGoalsConceded(rand() % 10);
			h2hCollationPlayerData.setFouls(rand() % 15);
			h2hCollationPlayerData.setGoals(rand() % 15);
			h2hCollationPlayerData.setOffsides(rand() % 15);
			h2hCollationPlayerData.setOwnGoals(rand() % 15);
			h2hCollationPlayerData.setPkGoals(0);
			h2hCollationPlayerData.setRedCard(rand() % 15);
			h2hCollationPlayerData.setYellowCard(rand() % 15);
            // h2hCustomPlayerData.setCurrentDivision(rand() % 15);
            h2hCustomPlayerData.setGoalAgainst(rand() % 10);
            h2hCustomPlayerData.setNbGuests(rand() % 10);
            // set Kit=147456 as per the new TTY16
            h2hCustomPlayerData.setShotsAgainst(rand() % 15);
            h2hCustomPlayerData.setShotsAgainstOnGoal(rand() % 10);
            h2hCustomPlayerData.setTeam(rand() % 10);
            h2hCustomPlayerData.setTeamrating(rand() % 150);
            h2hCustomPlayerData.setTeamName("Manchester City");  // as per the new TTY Team name can be manual or not? Need to confirm with Sandeep.
            h2hCustomPlayerData.setWentToPk(rand() % 10);
            // Adding h2hSeasonalPlayData as per the new TTY
            h2hSeasonalPlayData.setCurrentDivision(rand() % 15);
            h2hSeasonalPlayData.setCup_id(rand() % 15);  // Need to remove this as per the TTY16
            h2hSeasonalPlayData.setUpdateDivision(GameReporting::Fifa::NO_UPDATE);
            h2hSeasonalPlayData.setSeasonId(1+(rand() % 15));  // Need to remove this as per the TTY16
            h2hSeasonalPlayData.setWonCompetition(rand() % 15);
            h2hSeasonalPlayData.setWonLeagueTitle(false);
			h2hSeasonalPlayData.setGameNumber(0);

			if (ptrGameInfo->getTopologyHostPlayerId() == playerId) {
                playerReport->setHome(true);
                int score = (minScore + (rand() % 15));
                playerReport->setScore(score);  // 1 in LOG
                playerReport->setClientScore(score);
                BLAZE_INFO(BlazeRpcLog::gamereporting, "H2HCupMatch Home Player Score: %d", score);
            } else {
                int awayScore = (rand() % 10);
                playerReport->setScore(awayScore);  // 1 in LOG
                playerReport->setClientScore(awayScore);
                BLAZE_INFO(BlazeRpcLog::gamereporting, "H2HCupMatch Away Player Score: %d", awayScore);
            }
            // playerReport->setCustomDnf(0);  // Need to remove this as per the TTY16
            playerReport->setAccountCountry(0);
            playerReport->setFinishReason(0);
            playerReport->setGameResult(0);
            playerReport->setHome(true);  // as per the TTY20
            playerReport->setLosses(0);
			playerReport->setName(mPlayerData->getPersonaName().c_str());
            playerReport->setOpponentCount(0);
            playerReport->setExternalId(mPlayerData->getExternalId());
            playerReport->setNucleusId(mPlayerData->getPlayerId());
            playerReport->setPersona(mPlayerData->getPersonaName().c_str());
            playerReport->setPointsAgainst(0);
            playerReport->setUserResult(0);
            playerReport->setScore(5);
            // playerReport->setSponsoredEventAccountRegion(0);
            playerReport->setSkill(0);
            playerReport->setSkillPoints(0);
            playerReport->setTeam(1357);
            playerReport->setTies(0);
            playerReport->setWinnerByDnf(0);
            playerReport->setWins(0);
            if (playerId == mPlayerData->getPlayerId()) {  // Need to check if this check is passes while running locally TTY16
                GameReporting::OSDKGameReportBase::OSDKPrivatePlayerReport &osdkPrivatePlayerReport  = playerReport->getPrivatePlayerReport();
                GameReporting::OSDKGameReportBase::IntegerAttributeMap &PlayerPrivateIntAttribMap = osdkPrivatePlayerReport.getPrivateIntAttributeMap();
                // Blaze::Collections::AttributeMap &privateAttribMap = osdkPrivatePlayerReport.getPrivateAttributeMap();
                PlayerPrivateIntAttribMap["BANDAVGGM"] = 1000 * (uint16_t)Random::getRandomNumber(1000) + 6362040;
                PlayerPrivateIntAttribMap["BANDAVGNET"] = (uint16_t)Random::getRandomNumber(1000);
                PlayerPrivateIntAttribMap["BANDHIGM"] = -1 * 10000 * (uint16_t)Random::getRandomNumber(10) + 147483648;
                PlayerPrivateIntAttribMap["BANDHINET"] = 1000 * (uint16_t)Random::getRandomNumber(10) + (uint16_t)Random::getRandomNumber(100);
                PlayerPrivateIntAttribMap["BANDLOWGM"] = 1000000 * (uint16_t)Random::getRandomNumber(10) + 347935;
                PlayerPrivateIntAttribMap["BANDLOWNET"] = (uint16_t)Random::getRandomNumber(100);
                PlayerPrivateIntAttribMap["BYTESRCVDNET"] = (uint16_t)Random::getRandomNumber(10) + 84794;
                PlayerPrivateIntAttribMap["BYTESSENTNET"] = 100000 * (uint16_t)Random::getRandomNumber(10) + 65553;
                PlayerPrivateIntAttribMap["DROPPKTS"] = -1 * (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["FPSAVG"] = 100000 * (uint16_t)Random::getRandomNumber(10) + 86221;
                PlayerPrivateIntAttribMap["FPSDEV"] = 100000 * (uint16_t)Random::getRandomNumber(10) + 82331;
                PlayerPrivateIntAttribMap["FPSHI"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["FPSLOW"] = (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["GAMEENDREASON"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["GDESYNCEND"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["GDESYNCRSN"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["GENDPHASE"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["GPCKTPERIOD"] = (uint16_t)Random::getRandomNumber(10);  // as per the new TTY
                PlayerPrivateIntAttribMap["GPCKTPERIODSTD"] = (uint16_t)Random::getRandomNumber(10);  // as per the new TTY
                PlayerPrivateIntAttribMap["GRESULT"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["GRPTTYPE"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["GRPTVER"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["GUESTS0"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["GUESTS1"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["LATEAVGGM"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["LATEAVGNET"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["LATEHIGM"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["LATEHINET"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["LATELOWGM"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["LATELOWNET"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["LATESDEVGM"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["LATESDEVNET"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["PKTLOSS"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["REALTIMEGAME"] = (uint16_t)Random::getRandomNumber(10);  // as per the new TTY
                PlayerPrivateIntAttribMap["REALTIMEIDLE"] = (uint16_t)Random::getRandomNumber(10);  // as per the new TTY
                PlayerPrivateIntAttribMap["USERSEND0"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["USERSEND1"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["USERSSTRT0"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["USERSSTRT1"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["VOIPEND0"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["VOIPEND1"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["VOIPSTRT0"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["VOIPSTRT1"] = (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["gid"] = getGameId();
                /* privateAttribMap["BYTESSENTGM"] = eastl::string().sprintf("%d", rand() % 10).c_str();//as per the new TTY BYTESSENTGM and BYTESRCVDGM not present in new TTY
                privateAttribMap["BYTESRCVDGM"] = eastl::string().sprintf("%d", rand() % 10).c_str();*/
            }
            playerReport->setCustomPlayerReport(*h2hPlayerReport);
            OsdkPlayerReportsMap[playerId] = playerReport;
        }
    }
    /*if (BLAZE_IS_TRACE_RPC_ENABLED(BlazeRpcLog::gamemanager)) {
        char8_t buf[20240];
        BLAZE_TRACE_RPC_LOG(BlazeRpcLog::gamemanager, "-> [OnlineSeasonsGameSession][" << mPlayerData->getPlayerId() << "]::submitGameReport" << "\n" << request.print(buf, sizeof(buf)));
    }*/
    error = mPlayerData->getComponentProxy()->mGameReportingProxy->submitGameReport(request);
	if (error == ERR_OK)
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "OnlineSeasons::SubmitGamerport successful. mMatchmakingSessionId= " << mMatchmakingSessionId);
	}
	else
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::submitGameReport][" << mPlayerData->getPlayerId() << " gameId " << myGameId << error);
	}
    // delete osdkReport;
    // delete gamereport;
    // gamereport = NULL;
    return error;

}

}  // namespace Stress
}  // namespace Blaze
