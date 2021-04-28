//  *************************************************************************************************
//
//   File:    livecompetitionsgamesession.cpp
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
#include "./livecompetitionsgamesession.h"
#include "./utility.h"

using namespace Blaze::GameManager;
using namespace Blaze::GameReporting;  //  Temp Fix

namespace Blaze {
namespace Stress {

LiveCompetitionsGameSession::LiveCompetitionsGameSession(StressPlayerInfo* playerData, uint32_t GameSize, GameNetworkTopology Topology, GameProtocolVersionString GameProtocolVersionString)
    : GameSession(playerData, GameSize, Topology, GameProtocolVersionString)
{
}

LiveCompetitionsGameSession::~LiveCompetitionsGameSession()
{
    BLAZE_INFO(BlazeRpcLog::util, "[LiveCompetitionsGameSession][Destructor][%" PRIu64 "]: destroy called", mPlayerData->getPlayerId());
}

void LiveCompetitionsGameSession::asyncHandlerFunction(uint16_t type, EA::TDF::Tdf *notification)
{
    if ( mPlayerData != NULL && mPlayerData->isPlayerActive())
    {
            //  This Notification has not been handled so use the Default Handler
            onGameSessionDefaultAsyncHandler(type, notification);
    }
}

void LiveCompetitionsGameSession::onGameSessionDefaultAsyncHandler(uint16_t type, EA::TDF::Tdf* notification)
{
    //  This is abstract class and handle the async handlers in the derived class

    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitionsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]:Message Type =" << type);
    switch (type)
    {
        case GameManagerSlave::NOTIFY_NOTIFYGAMESETUP:
            //  Async notification sent to the player who joined a game.  Triggered by JoinGame Request or a Matchmaking session finding a game.
            {
                NotifyGameSetup* notify = (NotifyGameSetup*)notification;
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitionsGameSession::asyncHandlerFunction]: Received  NOTIFY_NOTIFYGAMESETUP." << mPlayerData->getPlayerId());
				handleNotifyGameSetUp(notify);
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMESTATECHANGE:
            {
                NotifyGameStateChange* notify = (NotifyGameStateChange*)notification;
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitionsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMESTATECHANGE, state=" <<
                                notify->getNewGameState());
                if (myGameId == notify->getGameId())
                {
					if( getPlayerState() == IN_GAME && notify->getNewGameState() == POST_GAME )
					{
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitionsGameSession::NOTIFY_NOTIFYGAMESTATECHANGE][" << mPlayerData->getPlayerId() << "]: submitGameReport");
						submitGameReport();
						leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, myGameId);
					}

                    //  Set Game State
                    setPlayerState(notify->getNewGameState());
                    LiveCompetitionsGameDataRef(mPlayerData).setGameState(myGameId, notify->getNewGameState());
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMERESET:
            {
                NotifyGameReset* notify = (NotifyGameReset*)notification;
                StressGameInfo* gameInfo = LiveCompetitionsGameDataRef(mPlayerData).getGameData(notify->getGameData().getGameId());
                if (gameInfo != NULL && gameInfo->getPlatformHostPlayerId() == mPlayerData->getPlayerId())
                {
                    //  TODO: host specific actions
                }
                BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitionsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMERESET.gameId=" <<
                               notify->getGameData().getGameId());
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINING:
            {
                NotifyPlayerJoining* notify = (NotifyPlayerJoining*)notification;
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitionsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERJOINING.gameId=" << notify->getGameId());
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

            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS:
            {
                BLAZE_INFO(BlazeRpcLog::gamemanager, "[LiveCompetitionsGameSession::handlerFunction][%" PRIu64 "]: Received  NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS.", mPlayerData->getPlayerId());
                //NotifyGameSetup* notify = (NotifyGameSetup*)notification;
                
                //  In quickmatch scenario NOTIFYGAMESETUP is not recieved anymore.Instead NOTIFYJOININGPLAYERINITIATECONNECTIONS contains all the game related data
                //handleNotifyGameSetUp(notify, GameManagerSlave::NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS);
				
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLATFORMHOSTINITIALIZED:
            {
                NotifyPlatformHostInitialized* notify = (NotifyPlatformHostInitialized*)notification;
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitionsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLATFORMHOSTINITIALIZED.gameId=" <<
                                notify->getGameId());
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMELISTUPDATE:
            {
                //NotifyGameListUpdate* notify = (NotifyGameListUpdate*)notification;
                BLAZE_INFO(BlazeRpcLog::gamemanager, "[LiveCompetitionsGameSession::asyncHandlerFunction][%" PRIu64 "]: Received  NOTIFY_NOTIFYGAMELISTUPDATE.", mPlayerData->getPlayerId());
                
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINCOMPLETED:
            {
                NotifyPlayerJoinCompleted* notify = (NotifyPlayerJoinCompleted*)notification;
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitionsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERJOINCOMPLETED.");
                if ( myGameId == notify->getGameId() && LiveCompetitionsGameDataRef(mPlayerData).isInvitedPlayerExist(notify->getPlayerId(), myGameId) == true )
                {
					if (isPlatformHost)
					{
						invokeUdatePrimaryExternalSessionForUser(mPlayerData, myGameId, "", "", myGameId, GAME_CREATE, GAME_TYPE_GROUP, ACTIVE_CONNECTED);
					}
					invokeUdatePrimaryExternalSessionForUser(mPlayerData, myGameId, "", "", myGameId, GAME_JOIN, GAME_TYPE_GROUP, ACTIVE_CONNECTED);
                    //  Remove Invited Player
                    LiveCompetitionsGameDataRef(mPlayerData).removeInvitedPlayer(notify->getPlayerId(), myGameId);
                }                
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLAYERREMOVED:
            {
                NotifyPlayerRemoved* notify = (NotifyPlayerRemoved*)notification;
				//safe check when player removed due to location based MM fails
				if (mPlayerData->getPlayerId() == notify->getPlayerId() && myGameId == notify->getGameId())
				{
					setPlayerState(DESTRUCTING);
				}

				StressGameInfo* ptrGameInfo = LiveCompetitionsGameDataRef(mPlayerData).getGameData(myGameId);
                if (!ptrGameInfo)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitionsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]::receiving notification(" << type << ") and gameId " << myGameId <<
                                    "not present in LIVECOMPETITIONSGamesMap");
                    break;
                }

                if (mPlayerData->getPlayerId() == notify->getPlayerId() && myGameId == notify->getGameId() )
                {
                    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitionsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERREMOVED for game [" << notify->getGameId() << "] reason: "<<notify->getPlayerRemovedReason());
					if (isPlatformHost)
					{
						//meshEndpointsConnectionLost - to other player
						//meshEndpointsDisconnected - DISCONNECTED_PLAYER_REMOVED
						//meshEndpointsDisconnected - 30722/2/0, DISCONNECTED
						//meshEndpointsConnected RPC 
						BlazeObjectId targetGroupId = getOpponentPlayerInfo().getConnGroupId();
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitionsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << " meshEndpointsDisconnected for grouID" << targetGroupId.toString());
						meshEndpointsConnectionLost(targetGroupId, 0, myGameId);
						meshEndpointsDisconnected(targetGroupId, DISCONNECTED_PLAYER_REMOVED);	
						targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, 0);				
						meshEndpointsDisconnected(targetGroupId, DISCONNECTED);	
					}
					else
					{
						//meshEndpointsDisconnected - DISCONNECTED
						BlazeObjectId targetGroupId = getOpponentPlayerInfo().getConnGroupId();
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitionsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << " meshEndpointsDisconnected for grouID" << targetGroupId.toString());
						meshEndpointsDisconnected(targetGroupId, DISCONNECTED);	

					}                 
                }

                if ( myGameId == notify->getGameId()  && LiveCompetitionsGameDataRef(mPlayerData).isPlayerExistInGame(notify->getPlayerId(), notify->getGameId()) == true )
                {
                    LiveCompetitionsGameDataRef(mPlayerData).removePlayerFromGameData(notify->getPlayerId(), notify->getGameId());
                    if (IsPlayerPresentInList((PlayerModule*)(mPlayerData->getOwner()), notify->getPlayerId(), LIVECOMPETITIONS) == true)
                    {
                        uint16_t count = ptrGameInfo->getPlayerCount();
                        ptrGameInfo->setPlayerCount(--count);
                        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitionsGameSession::asyncHandlerFunction]::NOTIFYPLAYERREMOVED[" << mPlayerData->getPlayerId() << "]:GameId=" << myGameId << " Player count = " << count
                                        << "for player=" << notify->getPlayerId());
                    }
                }
                //  if all players, in single exe, removed from the game delete the data from map
                if ( ptrGameInfo->getPlayerCount() == 0)
                {
                    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitionsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERREMOVED - Game Id = " <<
                                   notify->getGameId() << ".");
                    LiveCompetitionsGameDataRef(mPlayerData).removeGameData(notify->getGameId());
                }				
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMEREMOVED:
            {
                //  NotifyGameRemoved* notify = (NotifyGameRemoved*)notification;
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitionsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMEREMOVED.");
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
                            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitionsGameSession::OnNotifyMatchmakingFailed][" << mPlayerData->getPlayerId() << "]: Matchmake SESSION_TIMED_OUT.");
                        }
                        break;
                    case GameManager::SESSION_CANCELED:
                        {
                            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitionsGameSession::OnNotifyMatchmakingFailed][" << mPlayerData->getPlayerId() << "]: Matchmake SESSION_CANCELED.");
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
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitionsGameSession::handlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYMESSAGE.");
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

void LiveCompetitionsGameSession::handleNotifyGameSetUp(NotifyGameSetup* notify, uint16_t type /* = GameManagerSlave::NOTIFY_NOTIFYGAMESETUP*/)
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
    if (LiveCompetitionsGameDataRef(mPlayerData).isGameIdExist(myGameId))
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitionsGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]::receiving notification(" << type << ") and gameId " <<
                        notify->getGameData().getGameId() << "already present in LIVECOMPETITIONSGamesMap");
        //  Player will be added during NOTIFYPLAYERJOINCOMPLETED notification, not here.
    }
    else
    {
        //  create new game data
        LiveCompetitionsGameDataRef(mPlayerData).addGameData(notify);
    }

	StressGameInfo* ptrGameInfo = LiveCompetitionsGameDataRef(mPlayerData).getGameData(myGameId);
    if (!ptrGameInfo)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitionsGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "] gameId " << myGameId <<" not present in LIVECOMPETITIONSGamesMap");
		return ;
    }
	
	//Add opponent player info
	PlayerIdListMap::iterator start = ptrGameInfo->getPlayerIdListMap().begin();
	PlayerIdListMap::iterator end  = ptrGameInfo->getPlayerIdListMap().end();
	for( ; start != end; ++start)
	{
		if(mPlayerData->getPlayerId() != start->first)
		{
			getOpponentPlayerInfo().setPlayerId(start->first);
			getOpponentPlayerInfo().setConnGroupId(BlazeObjectId(ENTITY_TYPE_CONN_GROUP, start->second->connGroupId));
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitionsGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << " OpponentPlayerInfo PlayerId=" << getOpponentPlayerInfo().getPlayerId() << " ConnGroupId=" << getOpponentPlayerInfo().getConnGroupId().toString());
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
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitionsGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "calling finalizeGameCreation");
        finalizeGameCreation();
		//requestPlatformHost(mPlayerData, getGameId(), 0);
    }

	//  Increase player count if found in same stress exe.
    if (IsPlayerPresentInList((PlayerModule*)(mPlayerData->getOwner()), mPlayerData->getPlayerId(), LIVECOMPETITIONS) == true)
    {
        uint16_t count = LiveCompetitionsGameDataRef(mPlayerData).getPlayerCount(myGameId);
        LiveCompetitionsGameDataRef(mPlayerData).setPlayerCount(myGameId, ++count);
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitionsGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "]:GameId=" << myGameId << " Player count = " << count);
    }
}

BlazeRpcError LiveCompetitionsGameSession::submitGameReport()
{
	BlazeRpcError error = ERR_OK;

	StressGameInfo* ptrGameInfo = LiveCompetitionsGameDataRef(mPlayerData).getGameData(myGameId);
    if (!ptrGameInfo)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitionsGameSession::submitGameReport][" << mPlayerData->getPlayerId() << " gameId " << myGameId << " not present in LIVECOMPETITIONSGamesMap");
		return ERR_SYSTEM;
    }

    GameReporting::SubmitGameReportRequest request;
    const int minScore = 3;

    GameReporting::GameReport &gamereport =   request.getGameReport();
	gamereport.setGameReportingId(ptrGameInfo->getGameReportingId());
    gamereport.setGameReportName("gameType0");
    // Blaze::GameReporting::OSDKGameReportBase::OSDKReport *osdkReport = static_cast<Blaze::GameReporting::OSDKGameReportBase::OSDKReport *> (gamereport.getReport());
    // Blaze::GameReporting::OSDKGameReportBase::OSDKReport *osdkReport = (Blaze::GameReporting::OSDKGameReportBase::OSDKReport *)(gamereport.getReport());
    GameReporting::Fifa::H2HGameReport h2hGameReport;
    GameReporting::OSDKGameReportBase::OSDKReport *osdkReport = BLAZE_NEW GameReporting::OSDKGameReportBase::OSDKReport;
	Blaze::GameReporting::OSDKGameReportBase::OSDKGameReport& osdkGamereport = osdkReport->getGameReport();  // osdkGamereport is wrt GAMR in 
	osdkGamereport.setArenaChallengeId(0);
    gamereport.setReport(*osdkReport);
    if (osdkReport != NULL) {
     //   GameReporting::OSDKGameReportBase::OSDKGameReport& osdkGamereport = osdkReport->getGameReport();
        osdkGamereport.setGameTime(5468);  // as per the new TTY, game time is not logged;Confirm with Sandeep;
        osdkGamereport.setGameType("gameType0");
        osdkGamereport.setFinishedStatus(1);
        GameReporting::Fifa::CommonGameReport& commonGameReport = h2hGameReport.getCommonGameReport();
		commonGameReport.setAwayKitId(1376);
		commonGameReport.setWentToPk(0);
		commonGameReport.setStadiumId(137);
		commonGameReport.setIsRivalMatch(false);  // As per the new TTY;
		for (int i = 0; i<11; i++)
		{	
			commonGameReport.getAwayStartingXI().push_back( ( ((uint32_t)rand()) % 300000 ) + 100000);
			commonGameReport.getHomeStartingXI().push_back( ( ((uint32_t)rand()) % 300000 ) + 100000);
		}
		commonGameReport.setBallId(37);
		commonGameReport.setHomeKitId(1379);
        h2hGameReport.setMatchInSeconds(5400);
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
            commonPlayerReport.setCorners(rand() % 10);

			GameReporting::Fifa::CommonDetailReport& commonDetailPlayerReport = commonPlayerReport.getCommondetailreport();
			commonDetailPlayerReport.setAveragePossessionLength(rand() % 10);
			commonDetailPlayerReport.setAverageFatigueAfterNinety(rand() % 15);
			commonDetailPlayerReport.setInjuriesRed(rand() % 15);
			commonDetailPlayerReport.setPassesIntercepted(playerId % 70);
			commonDetailPlayerReport.setPenaltiesAwarded(rand() % 10);
			commonDetailPlayerReport.setPenaltiesMissed(rand() % 10);
			commonDetailPlayerReport.setPenaltiesScored(rand() % 10);
			commonDetailPlayerReport.setPenaltiesSaved(rand() % 10);
            commonPlayerReport.setPassAttempts(rand() % 15);
            commonPlayerReport.setPassesMade(rand() % 12);
            commonPlayerReport.setRating((float)(rand() % 10));
            commonPlayerReport.setSaves(rand() % 10);
            commonPlayerReport.setShots(rand() % 12);
            commonPlayerReport.setTackleAttempts(rand() % 12);
            commonPlayerReport.setTacklesMade(rand() % 10);
            commonPlayerReport.setFouls(rand() % 15);
            commonPlayerReport.setGoals(rand() % 10);
            commonPlayerReport.setInterceptions(rand() % 10);
			commonPlayerReport.setHasMOTM(rand() % 10);
            commonPlayerReport.setOffsides(rand() % 10);
            commonPlayerReport.setOwnGoals(rand() % 10);
            commonPlayerReport.setPkGoals(0);
            commonPlayerReport.setPossession(rand() % 10);
            commonPlayerReport.setRedCard(rand() % 15);
            commonPlayerReport.setShotsOnGoal(rand() % 10);
            commonPlayerReport.setUnadjustedScore(rand() % 10);  // as per the new TTY
            commonPlayerReport.setYellowCard(rand() % 15);
            
			//  h2hCollationPlayerData is new addition w.r.t. latest TTY logs
			h2hCollationPlayerData.setGoalsConceded(rand() % 10);
			h2hCollationPlayerData.setFouls(rand() % 15);
			h2hCollationPlayerData.setGoals(rand() % 10);
			h2hCollationPlayerData.setOffsides(rand() % 10);
			h2hCollationPlayerData.setOwnGoals(rand() % 10);
			h2hCollationPlayerData.setPkGoals(0);
			h2hCollationPlayerData.setRedCard(rand() % 15);
			h2hCollationPlayerData.setYellowCard(rand() % 15);

            // h2hCustomPlayerData.setCurrentDivision(rand() % 15);
			h2hCustomPlayerData.setControls((rand() % 10) + 1);
            h2hCustomPlayerData.setGoalAgainst(rand() % 10);
            h2hCustomPlayerData.setNbGuests(rand() % 10);
			h2hCustomPlayerData.setKit(507904);
			h2hCustomPlayerData.setOpponentId(getOpponentPlayerInfo().getPlayerId());
            h2hCustomPlayerData.setShotsAgainst(rand() % 10);
            h2hCustomPlayerData.setShotsAgainstOnGoal(rand() % 10);
            h2hCustomPlayerData.setTeam(rand() % 10);
            h2hCustomPlayerData.setTeamrating(rand() % 100);
            h2hCustomPlayerData.setTeamName("Bayern"); 
            h2hCustomPlayerData.setWentToPk(rand() % 10);

            // Adding h2hSeasonalPlayData as per the new TTY
            h2hSeasonalPlayData.setCurrentDivision(rand() % 10);
            h2hSeasonalPlayData.setCup_id(0);
            h2hSeasonalPlayData.setUpdateDivision(GameReporting::Fifa::NO_UPDATE);
			h2hSeasonalPlayData.setGameNumber(rand() % 10);
            h2hSeasonalPlayData.setSeasonId(0);
            h2hSeasonalPlayData.setWonCompetition(rand() % 10);
            h2hSeasonalPlayData.setWonLeagueTitle(false);

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
            playerReport->setClientScore(rand() % 15);
            playerReport->setAccountCountry(rand() % 10);
            playerReport->setFinishReason(rand() % 10);
            playerReport->setGameResult(rand() % 10);
            playerReport->setHome(true);
            playerReport->setLosses(rand() % 15);
			playerReport->setName(mPlayerData->getPersonaName().c_str());
            playerReport->setOpponentCount(rand() % 10);
            playerReport->setExternalId(mPlayerData->getExternalId());
            playerReport->setNucleusId(mPlayerData->getPlayerId());
            playerReport->setPersona(mPlayerData->getPersonaName().c_str());
            playerReport->setPointsAgainst(rand() % 10);
            playerReport->setUserResult(rand() % 10);
            playerReport->setScore(rand() % 15);
            playerReport->setSkill(rand() % 10);
            playerReport->setSkillPoints(0);
            playerReport->setTeam(rand() % 100);
            playerReport->setTies(rand() % 10);
            playerReport->setWinnerByDnf(rand() % 10);
            playerReport->setWins(0);
            if (playerId == mPlayerData->getPlayerId()) {  // Need to check if this check is passes while running locally TTY16
                GameReporting::OSDKGameReportBase::OSDKPrivatePlayerReport &osdkPrivatePlayerReport  = playerReport->getPrivatePlayerReport();
                GameReporting::OSDKGameReportBase::IntegerAttributeMap &PlayerPrivateIntAttribMap = osdkPrivatePlayerReport.getPrivateIntAttributeMap();
                // Blaze::Collections::AttributeMap &privateAttribMap = osdkPrivatePlayerReport.getPrivateAttributeMap();
				PlayerPrivateIntAttribMap["BANDAVGGM"]		= 1000 * (uint16_t)Random::getRandomNumber(1000) + 6362040;
				PlayerPrivateIntAttribMap["BANDAVGNET"]		= (uint16_t)Random::getRandomNumber(1000);
				PlayerPrivateIntAttribMap["BANDHIGM"]		= -1 * 10000 * (uint16_t)Random::getRandomNumber(10) + 147483648;
				PlayerPrivateIntAttribMap["BANDHINET"]		= 1000 * (uint16_t)Random::getRandomNumber(10) + (uint16_t)Random::getRandomNumber(100);
				PlayerPrivateIntAttribMap["BANDLOWGM"]		= 1000000 * (uint16_t)Random::getRandomNumber(10) + 347935;
				PlayerPrivateIntAttribMap["BANDLOWNET"]		= (uint16_t)Random::getRandomNumber(100);
				PlayerPrivateIntAttribMap["BYTESRCVDNET"]	= (uint16_t)Random::getRandomNumber(10) + 454125;
				PlayerPrivateIntAttribMap["BYTESSENTNET"]	= 100000 * (uint16_t)Random::getRandomNumber(10) + 247803;
				PlayerPrivateIntAttribMap["DROPPKTS"]		= -1 * (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["FPSAVG"]			= 100000 * (uint16_t)Random::getRandomNumber(10) + 86221;
				PlayerPrivateIntAttribMap["FPSDEV"]			= 100000 * (uint16_t)Random::getRandomNumber(10) + 82331;
				PlayerPrivateIntAttribMap["FPSHI"]			= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["FPSLOW"]			= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["GAMEENDREASON"]	= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["GDESYNCEND"]		= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["GDESYNCRSN"]		= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["GENDPHASE"]		= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["GPCKTPERIOD"]	= (uint16_t)Random::getRandomNumber(10);  // as per the new TTY
				PlayerPrivateIntAttribMap["GPCKTPERIODSTD"] = (uint16_t)Random::getRandomNumber(10);  // as per the new TTY
				PlayerPrivateIntAttribMap["GRESULT"]		= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["GRPTTYPE"]		= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["GRPTVER"]		= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["GUESTS0"]		= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["GUESTS1"]		= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["LATEAVGGM"]		= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["LATEAVGNET"]		= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["LATEHIGM"]		= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["LATEHINET"]		= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["LATELOWGM"]		= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["LATELOWNET"]		= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["LATESDEVGM"]		= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["LATESDEVNET"]	= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["PKTLOSS"]		= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["REALTIMEGAME"]	= (uint16_t)Random::getRandomNumber(10);  // as per the new TTY
				PlayerPrivateIntAttribMap["REALTIMEIDLE"]	= (uint16_t)Random::getRandomNumber(10);  // as per the new TTY
				PlayerPrivateIntAttribMap["USERSEND0"]		= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["USERSEND1"]		= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["USERSSTRT0"]		= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["USERSSTRT1"]		= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["VOIPEND0"]		= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["VOIPEND1"]		= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["VOIPSTRT0"]		= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["VOIPSTRT1"]		= (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["gid"]			= getGameId();
                /* privateAttribMap["BYTESSENTGM"] = eastl::string().sprintf("%d", rand() % 10).c_str();//as per the new TTY BYTESSENTGM and BYTESRCVDGM not present in new TTY
                privateAttribMap["BYTESRCVDGM"] = eastl::string().sprintf("%d", rand() % 10).c_str();*/
            }
            playerReport->setCustomPlayerReport(*h2hPlayerReport);
            OsdkPlayerReportsMap[playerId] = playerReport;
        }
    }
  /*  if (BLAZE_IS_TRACE_RPC_ENABLED(BlazeRpcLog::gamemanager)) {
        char8_t buf[102400];
       BLAZE_TRACE_RPC_LOG(BlazeRpcLog::gamemanager, "-> [LiveCompetitionsGameSession][" << mPlayerData->getPlayerId() << "]::submitGameReport" << "\n" << request.print(buf, sizeof(buf)));
    }*/
    error = mPlayerData->getComponentProxy()->mGameReportingProxy->submitGameReport(request);
	if (error == ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "LiveCompetitionsGameSession::submitgamereport success.");
	}
	else
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "LiveCompetitionsGameSession::submitgamereport failed. Error(" << ErrorHelp::getErrorName(error) << ")");
	}

    // delete osdkReport;
    // delete gamereport;
    // gamereport = NULL;
    return error;

}

}  // namespace Stress
}  // namespace Blaze
