//  *************************************************************************************************
//
//   File:    dedicatedserversgamesession.cpp
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
#include "./dedicatedserversgamesession.h"
#include "./utility.h"

using namespace Blaze::GameManager;
using namespace Blaze::GameReporting; 

namespace Blaze {
namespace Stress {

DedicatedServerGameSession::DedicatedServerGameSession(StressPlayerInfo* playerData, uint32_t GameSize, GameNetworkTopology Topology, GameProtocolVersionString GameProtocolVersionString)
    : GameSession(playerData, GameSize, Topology, GameProtocolVersionString)
{
}

DedicatedServerGameSession::~DedicatedServerGameSession()
{
    BLAZE_INFO(BlazeRpcLog::util, "[DedicatedServerGameSession][Destructor][%" PRIu64 "]: destroy called", mPlayerData->getPlayerId());
}

void DedicatedServerGameSession::asyncHandlerFunction(uint16_t type, EA::TDF::Tdf *notification)
{
    if ( mPlayerData != NULL && mPlayerData->isPlayerActive())
    {
            //  This Notification has not been handled so use the Default Handler
            onGameSessionDefaultAsyncHandler(type, notification);
    }
}

void DedicatedServerGameSession::onGameSessionDefaultAsyncHandler(uint16_t type, EA::TDF::Tdf* notification)
{
    //  This is abstract class and handle the async handlers in the derived class

    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]:Message Type =" << type);
    switch (type)
    {
        case GameManagerSlave::NOTIFY_NOTIFYGAMESETUP:
            //  Async notification sent to the player who joined a game.  Triggered by JoinGame Request or a Matchmaking session finding a game.
            {
                NotifyGameSetup* notify = (NotifyGameSetup*)notification;
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerGameSession::asyncHandlerFunction]: Received  NOTIFY_NOTIFYGAMESETUP." << mPlayerData->getPlayerId());
				handleNotifyGameSetUp(notify);
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMESTATECHANGE:
            {
                NotifyGameStateChange* notify = (NotifyGameStateChange*)notification;
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMESTATECHANGE, state=" <<
                                notify->getNewGameState());
                if (myGameId == notify->getGameId())
                {
					if( getPlayerState() == IN_GAME && notify->getNewGameState() == POST_GAME )
					{
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerGameSession::NOTIFY_NOTIFYGAMESTATECHANGE][" << mPlayerData->getPlayerId() << "]");
					}

                    //  Set Game State
                    setPlayerState(notify->getNewGameState());
                    DedicatedServerGameDataRef(mPlayerData).setGameState(myGameId, notify->getNewGameState());
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMERESET:
            {
                NotifyGameReset* notify = (NotifyGameReset*)notification;
               // StressGameInfo* gameInfo = DedicatedServerGameDataRef(mPlayerData).getGameData(notify->getGameData().getGameId());
				//if(notify->getGameData().getGameAttribs()["Gamemode_Name"] == "fut_online_champions" )
				//{
				//	mIsFutSupportDS = true;
				//}
				//if (gameInfo != NULL && gameInfo->getPlatformHostPlayerId() == mPlayerData->getPlayerId() && notify->getGameData().getGameId() == myGameId )
    //            {
    //                //  move game state to PRE_GAME.
				//	advanceGameState(PRE_GAME,myGameId);
    //            }
                BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMERESET.gameId=" <<
                               notify->getGameData().getGameId());
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINING:
            {
                NotifyPlayerJoining* notify = (NotifyPlayerJoining*)notification;
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERJOINING.gameId=" << notify->getGameId());
				if(notify->getGameId() == myGameId)
				{
					//meshEndpointsConnected
					BlazeObjectId targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, notify->getJoiningPlayer().getConnectionGroupId());
					meshEndpointsConnected(targetGroupId, 0, 0, myGameId);
					connectedUserGroupIds[notify->getJoiningPlayer().getPlayerId()] = targetGroupId;
					if( DedicatedServerGameDataRef(mPlayerData).isPlayerExistInGame(notify->getJoiningPlayer().getPlayerId(), notify->getGameId()) == false )
					{
						DedicatedServerGameDataRef(mPlayerData).addPlayerToGameInfo(notify);
						uint16_t count = DedicatedServerGameDataRef(mPlayerData).getPlayerCount(myGameId);
						DedicatedServerGameDataRef(mPlayerData).setPlayerCount(myGameId, ++count);
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]:GameId=" << myGameId << " Player count = " << count);
					}					
				}				
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS:
            {
                BLAZE_INFO(BlazeRpcLog::gamemanager, "[DedicatedServerGameSession::handlerFunction][%" PRIu64 "]: Received  NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS.", mPlayerData->getPlayerId());
                //NotifyGameSetup* notify = (NotifyGameSetup*)notification;
                
                //  In quickmatch scenario NOTIFYGAMESETUP is not recieved anymore.Instead NOTIFYJOININGPLAYERINITIATECONNECTIONS contains all the game related data
                //handleNotifyGameSetUp(notify, GameManagerSlave::NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS);
				
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLATFORMHOSTINITIALIZED:
            {
                NotifyPlatformHostInitialized* notify = (NotifyPlatformHostInitialized*)notification;
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLATFORMHOSTINITIALIZED.gameId=" <<
                                notify->getGameId());
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMELISTUPDATE:
            {
                //NotifyGameListUpdate* notify = (NotifyGameListUpdate*)notification;
                BLAZE_INFO(BlazeRpcLog::gamemanager, "[DedicatedServerGameSession::asyncHandlerFunction][%" PRIu64 "]: Received  NOTIFY_NOTIFYGAMELISTUPDATE.", mPlayerData->getPlayerId());
                
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINCOMPLETED:
            {
                NotifyPlayerJoinCompleted* notify = (NotifyPlayerJoinCompleted*)notification;
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERJOINCOMPLETED. for joiner= " << notify->getPlayerId());
/*				if(myGameId == notify->getGameId() && mPlayerData->getPlayerId() != notify->getPlayerId())
				{
				    uint16_t count = DedicatedServerGameDataRef(mPlayerData).getPlayerCount(myGameId);
				    DedicatedServerGameDataRef(mPlayerData).setPlayerCount(myGameId, ++count);
				    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "]:GameId=" << myGameId << " Player count = " << count);
				}	*/			
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLAYERREMOVED:
            {
                NotifyPlayerRemoved* notify = (NotifyPlayerRemoved*)notification;

				StressGameInfo* ptrGameInfo = DedicatedServerGameDataRef(mPlayerData).getGameData(myGameId);
                if (!ptrGameInfo)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]::receiving notification(" << type << ") and gameId " << myGameId <<
                                    "not present in dedicatedserverGamesMap");
                    break;
                }
				//meshEndpointsConnectionLost for each player who left the game
				BlazeObjectId targetGroupId = connectedUserGroupIds[notify->getPlayerId()];
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << " meshEndpointsConnectionLost for grouID" << targetGroupId.toString());
				meshEndpointsConnectionLost(targetGroupId, 0 , myGameId);
				connectedUserGroupIds.erase(notify->getPlayerId());

                if ( myGameId == notify->getGameId()  && DedicatedServerGameDataRef(mPlayerData).isPlayerExistInGame(notify->getPlayerId(), notify->getGameId()) == true )
                {
                    DedicatedServerGameDataRef(mPlayerData).removePlayerFromGameData(notify->getPlayerId(), notify->getGameId());
                    uint16_t count = ptrGameInfo->getPlayerCount();
                    ptrGameInfo->setPlayerCount(--count);
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerGameSession::asyncHandlerFunction]::NOTIFYPLAYERREMOVED[" << mPlayerData->getPlayerId() << "]:GameId=" << myGameId << " Player count = " << count
                                    << "for player=" << notify->getPlayerId());
					
				}
				// meshEndpointsConnectionLost
                //  if all players, in single exe, removed from the game delete the data from map
                if ( ptrGameInfo->getPlayerCount() <= 0)
                {
                    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERREMOVED - Game Id = " <<
                                   notify->getGameId() << ".");
                    //DedicatedServerGameDataRef(mPlayerData).removeGameData(notify->getGameId());
					//if(!mIsFutSupportDS)
					////{advanceGameState
					//	advanceGameState(POST_GAME);
					//	setPlayerState(POST_GAME);
					//}
				}				
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMEREMOVED:
            {
                //  NotifyGameRemoved* notify = (NotifyGameRemoved*)notification;
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMEREMOVED.");
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
                            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerGameSession::OnNotifyMatchmakingFailed][" << mPlayerData->getPlayerId() << "]: Matchmake SESSION_TIMED_OUT.");
                        }
                        break;
                    case GameManager::SESSION_CANCELED:
                        {
                            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerGameSession::OnNotifyMatchmakingFailed][" << mPlayerData->getPlayerId() << "]: Matchmake SESSION_CANCELED.");
                            //  Throw Execption;
                        }
						break;
                    default:
                        break;
                }
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
                //NotifyUserAdded* notify = (NotifyUserAdded*)notification;
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
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerGameSession::handlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYMESSAGE.");
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

void DedicatedServerGameSession::handleNotifyGameSetUp(NotifyGameSetup* notify, uint16_t type /* = GameManagerSlave::NOTIFY_NOTIFYGAMESETUP*/)
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
	 BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]" << notify->getGameData().getGameState());
    if (DedicatedServerGameDataRef(mPlayerData).isGameIdExist(myGameId))
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]::receiving notification(" << type << ") and gameId " <<
                        notify->getGameData().getGameId() << "already present in dedicatedserverGeamesMap");
        //  Player will be added during NOTIFYPLAYERJOINCOMPLETED notification, not here.
    }
    else
    {
        //  create new game data
        DedicatedServerGameDataRef(mPlayerData).addGameData(notify);
    }

	StressGameInfo* ptrGameInfo = DedicatedServerGameDataRef(mPlayerData).getGameData(myGameId);
    if (!ptrGameInfo)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "] gameId " << myGameId << " not present in dedicatedserverGamesMap");
		return;
    }
	uint16_t count = ptrGameInfo->getPlayerIdListMap().size();
	if(count > 0)
	{
		DedicatedServerGameDataRef(mPlayerData).setPlayerCount(myGameId, count);
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "]:GameId= " << myGameId << " Player count = " << count);
	}
}

BlazeRpcError DedicatedServerGameSession::returnDedicatedServerToPool()
{
    BlazeRpcError err = ERR_OK;
    ReturnDedicatedServerToPoolRequest req;
    req.setGameId(getGameId());
    err = mPlayerData->getComponentProxy()->mGameManagerProxy->returnDedicatedServerToPool(req);

    if (err != ERR_OK)
    {
        //  ErrorCounter::getInstance()->increaseErrorCount(err);
        BLAZE_ERR(BlazeRpcLog::gamemanager, "[DirtyBotsPlayerGameSession][%" PRId64"] Unable to ReturnDedicatedServerToPool game(%" PRId64") Err(%s)", mPlayerData->getPlayerId() ,
                  getGameId(),
                  Blaze::ErrorHelp::getErrorName(err));
    }

    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DirtyBotsPlayerGameSession::ReturnDedicatedServerToPool " << this << "]: Bot [" << mPlayerData->getPlayerId() <<
                    "] gameId  in progresss -> " << getGameId());
    return err;
}

}  // namespace Stress
}  // namespace Blaze
