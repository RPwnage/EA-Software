//  *************************************************************************************************
//
//   File:    friendliesgamesession.cpp
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
//#include "gamereporting/fifa/tdf/futh2hfriendlycompetitivereport.h"
#include "gamereporting/osdk/tdf/gameosdkreport.h"
#include "framework/blaze.h"
#include "./friendliesgamesession.h"
#include "./utility.h"

using namespace Blaze::GameManager;
using namespace Blaze::GameReporting;  //  Temp Fix

namespace Blaze {
namespace Stress {

FriendliesGameSession::FriendliesGameSession(StressPlayerInfo* playerData, uint32_t GameSize, GameNetworkTopology Topology, GameProtocolVersionString GameProtocolVersionString)
    : GameSession(playerData, GameSize, Topology, GameProtocolVersionString)
{
	
}

FriendliesGameSession::~FriendliesGameSession()
{
    BLAZE_INFO(BlazeRpcLog::util, "[FriendliesGameSession][Destructor][%" PRIu64 "]: destroy called", mPlayerData->getPlayerId());
}



void FriendliesGameSession::asyncHandlerFunction(uint16_t type, EA::TDF::Tdf *notification)
{
    if ( mPlayerData != NULL && mPlayerData->isPlayerActive())
    {
            //  This Notification has not been handled so use the Default Handler
            onGameSessionDefaultAsyncHandler(type, notification);
    }
}

void FriendliesGameSession::onGameSessionDefaultAsyncHandler(uint16_t type, EA::TDF::Tdf* notification)
{
    //  This is abstract class and handle the async handlers in the derived class

    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]:Message Type =" << type);
    switch (type)
    {
        case GameManagerSlave::NOTIFY_NOTIFYGAMESETUP:
            //  Async notification sent to the player who joined a game.  Triggered by JoinGame Request or a Matchmaking session finding a game.
            {
                NotifyGameSetup* notify = (NotifyGameSetup*)notification;
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FriendliesSeasonsGameSession::asyncHandlerFunction]: Received  NOTIFY_NOTIFYGAMESETUP." << mPlayerData->getPlayerId());
				handleNotifyGameSetUp(notify);
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMESTATECHANGE:
            {
                NotifyGameStateChange* notify = (NotifyGameStateChange*)notification;
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMESTATECHANGE, state=" <<
                                notify->getNewGameState());
                if (myGameId == notify->getGameId())
                {
					if( getPlayerState() == IN_GAME && notify->getNewGameState() == POST_GAME )
					{
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::NOTIFY_NOTIFYGAMESTATECHANGE][" << mPlayerData->getPlayerId() << "]: submitGameReport");
						submitGameReport();	
						//if (isPlatformHost) {
						//	replayGame();
						//}
						leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, myGameId);

					}
					if (isPlatformHost)
					{
						if (getPlayerState() == PRE_GAME || getPlayerState() == IN_GAME)
						{
							Blaze::Messaging::MessageAttrMap messageAttrMap;
							messageAttrMap[65282] = "GAID=33614280270";
							sendMessage(mPlayerData, myGameId, messageAttrMap, 1734438245, ENTITY_TYPE_GAME);
							messageAttrMap.clear();
						}
					}
					else
					{
						if (getPlayerState() == IN_GAME)
						{
							Blaze::Messaging::MessageAttrMap messageAttrMap;
							messageAttrMap[65282] = "GAID=33614280270";
							sendMessage(mPlayerData, myGameId, messageAttrMap, 1734438245, ENTITY_TYPE_GAME);
							messageAttrMap.clear();
						}
					}
                    //  Set Game State
                    setPlayerState(notify->getNewGameState());
                    FriendliesGameDataRef(mPlayerData).setGameState(myGameId, notify->getNewGameState());
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMERESET:
            {
                NotifyGameReset* notify = (NotifyGameReset*)notification;
                StressGameInfo* gameInfo = FriendliesGameDataRef(mPlayerData).getGameData(notify->getGameData().getGameId());
                if (gameInfo != NULL && gameInfo->getPlatformHostPlayerId() == mPlayerData->getPlayerId())
                {
                    //  TODO: host specific actions
                }
                BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMERESET.gameId=" <<
                               notify->getGameData().getGameId());
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINING:
            {
                NotifyPlayerJoining* notify = (NotifyPlayerJoining*)notification;
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERJOINING.gameId=" << notify->getGameId());
				/*if (!isPlatformHost)
				{
					meshEndpointsConnected(getOpponentPlayerInfo().getConnGroupId());
				}*/
				//Add opponent player info if not received in notifygamesetup
				if (getOpponentPlayerInfo().getPlayerId() == 0 && mPlayerData->getPlayerId() != notify->getJoiningPlayer().getPlayerId() && myGameId == notify->getGameId())
				{					
					getOpponentPlayerInfo().setPlayerId(notify->getJoiningPlayer().getPlayerId());
					getOpponentPlayerInfo().setConnGroupId(BlazeObjectId(ENTITY_TYPE_CONN_GROUP, notify->getJoiningPlayer().getConnectionGroupId()));
					BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << " OpponentPlayerInfo PlayerId=" << getOpponentPlayerInfo().getPlayerId() << " ConnGroupId=" << getOpponentPlayerInfo().getConnGroupId().toString());

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
						meshEndpointsConnected(targetGroupId, (uint32_t)calculateLatency(distnace), 4, myGameId);	
					}
					else
					{
						meshEndpointsConnected(targetGroupId, 0, 4, myGameId);
					}	
				}
				else
				{
					BlazeObjectId targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, notify->getJoiningPlayer().getConnectionGroupId());
					meshEndpointsConnected(targetGroupId, 0, 4, myGameId);
				}
                ////  Add Player Data and increase player count if found in same stress exe.
                //if ( myGameId == notify->getGameId() && FriendliesGameDataRef(mPlayerData).isPlayerExistInGame(notify->getJoiningPlayer().getPlayerId(), myGameId) == false )
                //{
                //    FriendliesGameDataRef(mPlayerData).addPlayerToGameInfo(notify);
                //    if (IsPlayerPresentInList((PlayerModule*)(mPlayerData->getOwner()), notify->getJoiningPlayer().getPlayerId(), FRIENDLIES) == true)
                //    {
                //        uint16_t count = FriendliesGameDataRef(mPlayerData).getPlayerCount(myGameId);
                //        FriendliesGameDataRef(mPlayerData).setPlayerCount(myGameId, ++count);
                //        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameDataRef::asyncHandlerFunction::NOTIFYPLAYERJOINCOMPLETED][" << mPlayerData->getPlayerId() << "]:GameId=" << myGameId << " Player count = " <<
                //                        count << "joiner=" << notify->getJoiningPlayer().getPlayerId());
                //    }
                //}
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS:
            {
                BLAZE_INFO(BlazeRpcLog::gamemanager, "[FriendliesGameSession::handlerFunction][%" PRIu64 "]: Received  NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS.", mPlayerData->getPlayerId());
                //NotifyGameSetup* notify = (NotifyGameSetup*)notification;                           
				
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLATFORMHOSTINITIALIZED:
            {
                NotifyPlatformHostInitialized* notify = (NotifyPlatformHostInitialized*)notification;
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLATFORMHOSTINITIALIZED.gameId=" <<
                                notify->getGameId());
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMELISTUPDATE:
            {
                //NotifyGameListUpdate* notify = (NotifyGameListUpdate*)notification;
                BLAZE_INFO(BlazeRpcLog::gamemanager, "[FriendliesGameSession::asyncHandlerFunction][%" PRIu64 "]: Received  NOTIFY_NOTIFYGAMELISTUPDATE.", mPlayerData->getPlayerId());
                
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINCOMPLETED:
            {
                NotifyPlayerJoinCompleted* notify = (NotifyPlayerJoinCompleted*)notification;
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERJOINCOMPLETED.");
                if ( myGameId == notify->getGameId() && FriendliesGameDataRef(mPlayerData).isInvitedPlayerExist(notify->getPlayerId(), myGameId) == true )
                {
                    //  Remove Invited Player
                    FriendliesGameDataRef(mPlayerData).removeInvitedPlayer(notify->getPlayerId(), myGameId);
                }
				if (isPlatformHost)
				{
					//invokeUdatePrimaryExternalSessionForUser(mPlayerData, myGameId, "", "", myGameId, GAME_CREATE, GAME_TYPE_GROUP, ACTIVE_CONNECTED);
					updatePrimaryExternalSessionForUserRequest(mPlayerData, myGameId, 0, UPDATE_PRESENCE_REASON_GAME_CREATE, GAME_TYPE_GAMESESSION, ACTIVE_CONNECTED, PRESENCE_MODE_STANDARD, 0, false);
				}
				//invokeUdatePrimaryExternalSessionForUser(mPlayerData, myGameId, "", "", myGameId, GAME_JOIN, GAME_TYPE_GROUP, ACTIVE_CONNECTED);
				updatePrimaryExternalSessionForUserRequest(mPlayerData, myGameId, 0, UPDATE_PRESENCE_REASON_GAME_JOIN, GAME_TYPE_GAMESESSION, ACTIVE_CONNECTED, PRESENCE_MODE_STANDARD, 0, false);

                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::PlayerJoinCompleted. Player][" << mPlayerData->getPlayerId()) ;
                
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

				StressGameInfo* ptrGameInfo = FriendliesGameDataRef(mPlayerData).getGameData(myGameId);
                if (!ptrGameInfo)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]::receiving notification(" << type << ") and gameId " << myGameId <<
                                    "not present in FriendliesGamesMap");
                    break;
                }

                if (mPlayerData->getPlayerId() == notify->getPlayerId() && myGameId == notify->getGameId() )
                {
                    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERREMOVED for game [" << notify->getGameId() << "] reason: "<<notify->getPlayerRemovedReason());
					if (isPlatformHost)
					{
						BlazeObjectId targetGroupId = getOpponentPlayerInfo().getConnGroupId();
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << " meshEndpointsDisconnected for grouID" << targetGroupId.toString());
						//meshEndpointsConnectionLost(targetGroupId, 0, myGameId);
						meshEndpointsDisconnected(targetGroupId, DISCONNECTED_PLAYER_REMOVED, 4, myGameId);
						targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, 0);				
						meshEndpointsDisconnected(targetGroupId, DISCONNECTED, 0, myGameId);	
					}
					else
					{
						//meshEndpointsDisconnected - DISCONNECTED
						//BlazeObjectId targetGroupId = getOpponentPlayerInfo().getConnGroupId();
						BlazeObjectId topologyHostConnId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, mTopologyHostConnectionGroupId);
						meshEndpointsConnectionLost(topologyHostConnId, 4, myGameId);
						//BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << " meshEndpointsDisconnected for grouID" << targetGroupId.toString());
						meshEndpointsDisconnected(topologyHostConnId, DISCONNECTED, 0 , myGameId);
					}                 
                }

                if ( myGameId == notify->getGameId()  && FriendliesGameDataRef(mPlayerData).isPlayerExistInGame(notify->getPlayerId(), notify->getGameId()) == true )
                {
                    FriendliesGameDataRef(mPlayerData).removePlayerFromGameData(notify->getPlayerId(), notify->getGameId());
                    if (IsPlayerPresentInList((PlayerModule*)(mPlayerData->getOwner()), notify->getPlayerId(), FRIENDLIES) == true)
                    {
                        uint16_t count = ptrGameInfo->getPlayerCount();
                        ptrGameInfo->setPlayerCount(--count);
                        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::asyncHandlerFunction]::NOTIFYPLAYERREMOVED[" << mPlayerData->getPlayerId() << "]:GameId=" << myGameId << " Player count = " << count
                                        << "for player=" << notify->getPlayerId());
                    }
                }
                //  if all players, in single exe, removed from the game delete the data from map
                if ( ptrGameInfo->getPlayerCount() <= 0)
                {
                    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERREMOVED - Game Id = " <<
                                   notify->getGameId() << ".");
                    FriendliesGameDataRef(mPlayerData).removeGameData(notify->getGameId());
                }				
				invokeUdatePrimaryExternalSessionForUser(mPlayerData, myGameId, "", "", myGameId, GAME_LEAVE, GAME_TYPE_GROUP, ACTIVE_CONNECTED);
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMEREMOVED:
            {
                //  NotifyGameRemoved* notify = (NotifyGameRemoved*)notification;
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMEREMOVED.");
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
                            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::OnNotifyMatchmakingFailed][" << mPlayerData->getPlayerId() << "]: Matchmake SESSION_TIMED_OUT.");
                        }
                        break;
                    case GameManager::SESSION_CANCELED:
                        {
                            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::OnNotifyMatchmakingFailed][" << mPlayerData->getPlayerId() << "]: Matchmake SESSION_CANCELED.");
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
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::handlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYMESSAGE.");
				
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

BlazeRpcError FriendliesGameSession::submitGameReport()
{
	BlazeRpcError error = ERR_OK;

	StressGameInfo* ptrGameInfo = FriendliesGameDataRef(mPlayerData).getGameData(myGameId);
    if (!ptrGameInfo)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::submitGameReport][" << mPlayerData->getPlayerId() << " gameId " << myGameId << " not present in FriendliesGamesMap");
		return ERR_SYSTEM;
    }

    GameReporting::SubmitGameReportRequest request;
    const int minScore = 3;

	request.setFinishedStatus(GAMEREPORT_FINISHED_STATUS_FINISHED);
    GameReporting::GameReport &gamereport =   request.getGameReport();
	

	gamereport.setGameReportingId(ptrGameInfo->getGameReportingId());

	if (mScenarioType == FRRB)
	{
		gamereport.setGameReportName("gameType75");
	}
	else
	{
		gamereport.setGameReportName("gameType20");
	}

    // Blaze::GameReporting::OSDKGameReportBase::OSDKReport *osdkReport = static_cast<Blaze::GameReporting::OSDKGameReportBase::OSDKReport *> (gamereport.getReport());
    // Blaze::GameReporting::OSDKGameReportBase::OSDKReport *osdkReport = (Blaze::GameReporting::OSDKGameReportBase::OSDKReport *)(gamereport.getReport());
    GameReporting::Fifa::H2HGameReport h2hGameReport;
	//Blaze::GameReporting::Fifa::FutH2HFriendlyCompetitiveGameReport futH2hGameReport;
    GameReporting::OSDKGameReportBase::OSDKReport *osdkReport = BLAZE_NEW GameReporting::OSDKGameReportBase::OSDKReport;
    gamereport.setReport(*osdkReport);
    if (osdkReport != NULL) {
        GameReporting::OSDKGameReportBase::OSDKGameReport& osdkGamereport = osdkReport->getGameReport();
        osdkGamereport.setGameTime(5468);  // TODO - change to random number
		osdkGamereport.setGameType("gameType20");
		osdkGamereport.setArenaChallengeId(0);
		osdkGamereport.setCategoryId(0);
		osdkGamereport.setGameReportId(0);
		osdkGamereport.setSimulate(false);
		osdkGamereport.setLeagueId(0);
		osdkGamereport.setRank(false);
		osdkGamereport.setRoomId(0);

        osdkGamereport.setFinishedStatus(1);
		if (mScenarioType == FRIENDLY)
		{
			GameReporting::Fifa::CommonGameReport& commonGameReport = h2hGameReport.getCommonGameReport();
			commonGameReport.setWentToPk(0);
			commonGameReport.setIsRivalMatch(false);  // As per the new TTY;
			commonGameReport.setAwayKitId(1367);	//	As per latest logs
			commonGameReport.setBallId(120);
			commonGameReport.setHomeKitId(1362);
			commonGameReport.setStadiumId(246);
			for (int i = 0; i < 11; i++)
			{
				commonGameReport.getAwayStartingXI().push_back((((uint32_t)rand()) % 300000) + 100000);
				commonGameReport.getHomeStartingXI().push_back((((uint32_t)rand()) % 300000) + 100000);
			}
			h2hGameReport.setMatchInSeconds(5400);
			h2hGameReport.setSponsoredEventId(0);
			osdkGamereport.setCustomGameReport(h2hGameReport);
		}
		if (mScenarioType == FRRB)
		{
			//GameReporting::Fifa::CommonGameReport& commonGameReport = futH2hGameReport.getCommonGameReport();
			//futH2hGameReport.getFutH2HFriendlyCompetitiveMatchSettings().setMatchLeg(1);
			//futH2hGameReport.getFutH2HFriendlyCompetitiveMatchSettings().setAggregateGoalsAway(0);
			//futH2hGameReport.getFutH2HFriendlyCompetitiveMatchSettings().setAggregateGoalsHome(0);
			//futH2hGameReport.getFutH2HFriendlyCompetitiveMatchSettings().setServerName("Direct");
			//futH2hGameReport.getFutH2HFriendlyCompetitiveMatchSettings().setGameStartTime(0);
			//futH2hGameReport.getFutH2HFriendlyCompetitiveMatchSettings().setStartTeamPossession("Ravenna AC");
			//commonGameReport.setWentToPk(0);
			//commonGameReport.setIsRivalMatch(false);  // As per the new TTY;
			//commonGameReport.setAwayKitId(1367);	//	As per latest logs
			//commonGameReport.setBallId(120);
			//commonGameReport.setHomeKitId(1362);
			//commonGameReport.setStadiumId(246);
			//for (int i = 0; i < 11; i++)
			//{
			//	commonGameReport.getAwayStartingXI().push_back((((uint32_t)rand()) % 300000) + 100000);
			//	commonGameReport.getHomeStartingXI().push_back((((uint32_t)rand()) % 300000) + 100000);
			//}
			//futH2hGameReport.setMatchInSeconds(5600);
			//osdkGamereport.setCustomGameReport(futH2hGameReport);
		}
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
			if (mScenarioType == FRIENDLY)
			{
				GameReporting::Fifa::H2HPlayerReport *h2hPlayerReport = BLAZE_NEW GameReporting::Fifa::H2HPlayerReport;
				GameReporting::Fifa::CommonPlayerReport& commonPlayerReport = h2hPlayerReport->getCommonPlayerReport();
				GameReporting::Fifa::H2HCollationPlayerData& h2hCollationPlayerData = h2hPlayerReport->getH2HCollationPlayerData();
				GameReporting::Fifa::H2HCustomPlayerData& h2hCustomPlayerData = h2hPlayerReport->getH2HCustomPlayerData();
				GameReporting::Fifa::SeasonalPlayData& h2hSeasonalPlayData = h2hPlayerReport->getSeasonalPlayData();  // as per the new TTY
				// Calling the set functions as per the new TTY
				// Followed the order of calling functions as per the new TTY
				commonPlayerReport.setAssists(playerId % 2);
				commonPlayerReport.setGoalsConceded(playerId % 4);
				commonPlayerReport.setHasCleanSheets(playerId % 2);
				// mHasCleanSheets need to be set to 1 as per the new TTY16
				//	Setting CommonDetailReport as per new logs
				commonPlayerReport.setCorners(playerId % 5);
				commonPlayerReport.getCommondetailreport().setAveragePossessionLength((uint16_t)Random::getRandomNumber(10) + 10);
				commonPlayerReport.getCommondetailreport().setAverageFatigueAfterNinety(100 * ((uint16_t)Random::getRandomNumber(4) + 4) + (uint16_t)Random::getRandomNumber(100));
				commonPlayerReport.getCommondetailreport().setInjuriesRed((uint16_t)Random::getRandomNumber(5));
				commonPlayerReport.getCommondetailreport().setPassesIntercepted((uint16_t)Random::getRandomNumber(10));
				commonPlayerReport.getCommondetailreport().setPenaltiesAwarded((uint16_t)Random::getRandomNumber(5));
				commonPlayerReport.getCommondetailreport().setPenaltiesMissed((uint16_t)Random::getRandomNumber(5));
				commonPlayerReport.getCommondetailreport().setPenaltiesScored((uint16_t)Random::getRandomNumber(5));
				commonPlayerReport.getCommondetailreport().setPenaltiesSaved((uint16_t)Random::getRandomNumber(5));

				commonPlayerReport.setPassAttempts(playerId % 15);
				commonPlayerReport.setPassesMade(playerId % 6);
				commonPlayerReport.setRating((float)(playerId % 2));
				commonPlayerReport.setSaves(playerId % 4);
				commonPlayerReport.setShots(playerId % 6);
				commonPlayerReport.setTackleAttempts(playerId % 6);
				commonPlayerReport.setTacklesMade(playerId % 5);
				commonPlayerReport.setFouls(playerId % 3);
				commonPlayerReport.setGoals(playerId % 5);
				commonPlayerReport.setInterceptions(playerId % 5);  // as per the new TTY
				// mHasMOTM = 1 (0x01) need to be set to 1 as per the new TTY16
				commonPlayerReport.setHasMOTM(playerId % 5);
				commonPlayerReport.setOffsides(playerId % 5);
				commonPlayerReport.setOwnGoals(playerId % 2);
				commonPlayerReport.setPkGoals(0);
				commonPlayerReport.setPossession(playerId % 5);
				commonPlayerReport.setRedCard(playerId % 3);
				commonPlayerReport.setShotsOnGoal(playerId % 5);
				commonPlayerReport.setUnadjustedScore(playerId % 5);  // as per the new TTY
				commonPlayerReport.setYellowCard(playerId % 3);
				h2hCustomPlayerData.setControls((playerId % 2) + 1);
				//  h2hCollationPlayerData is new addition w.r.t. latest TTY logs
				h2hCollationPlayerData.setGoalsConceded(playerId % 4);
				h2hCollationPlayerData.setFouls(playerId % 3);
				h2hCollationPlayerData.setGoals(playerId % 5);
				h2hCollationPlayerData.setOffsides(playerId % 5);
				h2hCollationPlayerData.setOwnGoals(playerId % 5);
				h2hCollationPlayerData.setPkGoals(0);
				h2hCollationPlayerData.setRedCard(playerId % 3);
				h2hCollationPlayerData.setYellowCard(playerId % 3);
				// h2hCustomPlayerData.setCurrentDivision(playerId % 3);
				h2hCustomPlayerData.setGoalAgainst(playerId % 4);
				h2hCustomPlayerData.setNbGuests(playerId % 2);
				// set Kit=147456 as per the new TTY16
				h2hCustomPlayerData.setKit(0);
				h2hCustomPlayerData.setOpponentId(getOpponentPlayerInfo().getPlayerId());	//	Need to check
				h2hCustomPlayerData.setShotsAgainst(playerId % 5);
				h2hCustomPlayerData.setShotsAgainstOnGoal(playerId % 4);
				h2hCustomPlayerData.setTeam(playerId % 2);
				h2hCustomPlayerData.setTeamrating(playerId % 50);
				h2hCustomPlayerData.setTeamName("Everton");  // as per the new TTY Team name can be manual or not? Need to confirm with Sandeep.
				h2hCustomPlayerData.setWentToPk(playerId % 2);
				// Adding h2hSeasonalPlayData as per the new TTY
				h2hSeasonalPlayData.setCurrentDivision(playerId % 5);
				h2hSeasonalPlayData.setCup_id(playerId % 5);  // Need to remove this as per the TTY16
				h2hSeasonalPlayData.setUpdateDivision(GameReporting::Fifa::NO_UPDATE);
				h2hSeasonalPlayData.setSeasonId(1 + (playerId % 5));  // Need to remove this as per the TTY16
				h2hSeasonalPlayData.setWonCompetition(playerId % 5);
				h2hSeasonalPlayData.setWonLeagueTitle(false);
				h2hSeasonalPlayData.setGameNumber(playerId % 5);
				playerReport->setCustomPlayerReport(*h2hPlayerReport);
			}
			if (mScenarioType == FRRB)
			{
				//GameReporting::Fifa::FutH2HFriendlyCompetitivePlayerReport *futh2hPlayerReport = BLAZE_NEW GameReporting::Fifa::FutH2HFriendlyCompetitivePlayerReport;
				//GameReporting::Fifa::CommonPlayerReport& commonPlayerReport = futh2hPlayerReport->getCommonPlayerReport();
				//GameReporting::Fifa::FutH2HFriendlyCompetitivePlayerData& futH2HFriendlyCompetitivePlayerData = futh2hPlayerReport->getFutH2HFriendlyCompetitivePlayerData();
				//GameReporting::Fifa::H2HCustomPlayerData& h2hCustomPlayerData = futh2hPlayerReport->getH2HCustomPlayerData();
				//// Calling the set functions as per the new TTY
				//// Followed the order of calling functions as per the new TTY
				//commonPlayerReport.setAssists(playerId % 2);
				//commonPlayerReport.setGoalsConceded(playerId % 4);
				//commonPlayerReport.setHasCleanSheets(playerId % 2);
				//// mHasCleanSheets need to be set to 1 as per the new TTY16
				////	Setting CommonDetailReport as per new logs
				//commonPlayerReport.setCorners(playerId % 5);
				//commonPlayerReport.getCommondetailreport().setAveragePossessionLength((uint16_t)Random::getRandomNumber(10) + 10);
				//commonPlayerReport.getCommondetailreport().setAverageFatigueAfterNinety(100 * ((uint16_t)Random::getRandomNumber(4) + 4) + (uint16_t)Random::getRandomNumber(100));
				//commonPlayerReport.getCommondetailreport().setInjuriesRed((uint16_t)Random::getRandomNumber(5));
				//commonPlayerReport.getCommondetailreport().setPassesIntercepted((uint16_t)Random::getRandomNumber(10));
				//commonPlayerReport.getCommondetailreport().setPenaltiesAwarded((uint16_t)Random::getRandomNumber(5));
				//commonPlayerReport.getCommondetailreport().setPenaltiesMissed((uint16_t)Random::getRandomNumber(5));
				//commonPlayerReport.getCommondetailreport().setPenaltiesScored((uint16_t)Random::getRandomNumber(5));
				//commonPlayerReport.getCommondetailreport().setPenaltiesSaved((uint16_t)Random::getRandomNumber(5));

				//commonPlayerReport.setPassAttempts(playerId % 15);
				//commonPlayerReport.setPassesMade(playerId % 6);
				//commonPlayerReport.setRating((float)(playerId % 2));
				//commonPlayerReport.setSaves(playerId % 4);
				//commonPlayerReport.setShots(playerId % 6);
				//commonPlayerReport.setTackleAttempts(playerId % 6);
				//commonPlayerReport.setTacklesMade(playerId % 5);
				//commonPlayerReport.setFouls(playerId % 3);
				//commonPlayerReport.setGoals(playerId % 5);
				//commonPlayerReport.setInterceptions(playerId % 5);  // as per the new TTY
				//// mHasMOTM = 1 (0x01) need to be set to 1 as per the new TTY16
				//commonPlayerReport.setHasMOTM(playerId % 5);
				//commonPlayerReport.setOffsides(playerId % 5);
				//commonPlayerReport.setOwnGoals(playerId % 2);
				//commonPlayerReport.setPkGoals(0);
				//commonPlayerReport.setPossession(playerId % 5);
				//commonPlayerReport.setRedCard(playerId % 3);
				//commonPlayerReport.setShotsOnGoal(playerId % 5);
				//commonPlayerReport.setUnadjustedScore(playerId % 5);  // as per the new TTY
				//commonPlayerReport.setYellowCard(playerId % 3);
				////  futH2HFriendlyCompetitivePlayerData is new addition w.r.t. latest TTY logs
				//futH2HFriendlyCompetitivePlayerData.setAggregateGoalsFor(playerId % 4);
				//futH2HFriendlyCompetitivePlayerData.setExtraTimeGoalsFor(playerId % 3);
				//futH2HFriendlyCompetitivePlayerData.setGoalsFor(playerId % 5);
				//for (int i = 0; i < 5; i++)
				//{
				//	futH2HFriendlyCompetitivePlayerData.getStartingSubs().push_back((((uint32_t)rand()) % 300000) + 100000);
				//}
				//for (int i = 0; i < 11; i++)
				//{
				//	futH2HFriendlyCompetitivePlayerData.getStartingXI().push_back((((uint32_t)rand()) % 300000) + 100000);
				//}
				//// h2hCustomPlayerData.setCurrentDivision(playerId % 3);
				//h2hCustomPlayerData.setControls((playerId % 2) + 1);
				//h2hCustomPlayerData.setGoalAgainst(playerId % 4);
				//h2hCustomPlayerData.setNbGuests(playerId % 2);
				//// set Kit=147456 as per the new TTY16
				//h2hCustomPlayerData.setKit(0);
				//h2hCustomPlayerData.setOpponentId(getOpponentPlayerInfo().getPlayerId());	//	Need to check
				//h2hCustomPlayerData.setShotsAgainst(playerId % 5);
				//h2hCustomPlayerData.setShotsAgainstOnGoal(playerId % 4);
				//h2hCustomPlayerData.setTeam(playerId % 2);
				//h2hCustomPlayerData.setTeamrating(playerId % 50);
				//h2hCustomPlayerData.setTeamName("Everton");  // as per the new TTY Team name can be manual or not? Need to confirm with Sandeep.
				//h2hCustomPlayerData.setWentToPk(playerId % 2);
				//playerReport->setCustomPlayerReport(*futh2hPlayerReport);
			}
			if (ptrGameInfo->getTopologyHostPlayerId() == playerId) {
                playerReport->setHome(true);
                int score = (minScore + (playerId % 3));
                playerReport->setScore(score);  // 1 in LOG
                playerReport->setClientScore(score);
                BLAZE_INFO(BlazeRpcLog::gamereporting, "H2HCupMatch Home Player Score: %d", score);
            } else {
                int awayScore = (playerId % 2);
                playerReport->setScore(awayScore);  // 1 in LOG
                playerReport->setClientScore(awayScore);
				playerReport->setHome(false);
                BLAZE_INFO(BlazeRpcLog::gamereporting, "H2HCupMatch Away Player Score: %d", awayScore);
            }
            // playerReport->setCustomDnf(0);  // Need to remove this as per the TTY16
            //playerReport->setClientScore(0);
            playerReport->setAccountCountry(0);
            playerReport->setFinishReason(0);
            playerReport->setGameResult(0);
            //playerReport->setHome(true);  // as per the TTY16
            playerReport->setLosses(0);
			playerReport->setName(mPlayerData->getPersonaName().c_str());
            playerReport->setOpponentCount(0);
            playerReport->setExternalId(mPlayerData->getExternalId());
            playerReport->setNucleusId(mPlayerData->getPlayerId());
            playerReport->setPersona(mPlayerData->getPersonaName().c_str());
            playerReport->setPointsAgainst(0);
            playerReport->setUserResult(1);
            playerReport->setScore(0);
            // playerReport->setSponsoredEventAccountRegion(0);
            playerReport->setSkill(0);
            playerReport->setSkillPoints(0);
            playerReport->setTeam(0);
            playerReport->setTies(0);
            playerReport->setWinnerByDnf(0);
            playerReport->setWins(0);
			if (playerId == mPlayerData->getPlayerId()) {  // Need to check if this check is passes while running locally TTY16
                GameReporting::OSDKGameReportBase::OSDKPrivatePlayerReport &osdkPrivatePlayerReport  = playerReport->getPrivatePlayerReport();
                GameReporting::OSDKGameReportBase::IntegerAttributeMap &PlayerPrivateIntAttribMap = osdkPrivatePlayerReport.getPrivateIntAttributeMap();
                // Blaze::Collections::AttributeMap &privateAttribMap = osdkPrivatePlayerReport.getPrivateAttributeMap();
                PlayerPrivateIntAttribMap["BANDAVGGM"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["BANDAVGNET"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["BANDHIGM"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["BANDHINET"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["BANDLOWGM"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["BANDLOWNET"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["BYTESRCVDNET"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["BYTESSENTNET"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["DROPPKTS"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["FPSAVG"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["FPSDEV"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["FPSHI"] = (uint16_t)Random::getRandomNumber(10);
                PlayerPrivateIntAttribMap["FPSLOW"] = (uint16_t)Random::getRandomNumber(10);
				PlayerPrivateIntAttribMap["GAMEENDREASON"] = 0;
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
				if (mScenarioType == FRRB) {
					PlayerPrivateIntAttribMap["futMatchFlags"] = (uint16_t)Blaze::Random::getRandomNumber(100);
					PlayerPrivateIntAttribMap["futMatchResult"] = (rand() % 15);
					PlayerPrivateIntAttribMap["futMatchTime"] = (osdkGamereport.getGameTime());
				}
                /* privateAttribMap["BYTESSENTGM"] = eastl::string().sprintf("%d", rand() % 10).c_str();//as per the new TTY BYTESSENTGM and BYTESRCVDGM not present in new TTY
                privateAttribMap["BYTESRCVDGM"] = eastl::string().sprintf("%d", rand() % 10).c_str();*/
            }
            //playerReport->setCustomPlayerReport(*h2hPlayerReport);
            OsdkPlayerReportsMap[playerId] = playerReport;
        }
    }

	if (isPlatformHost)
	{
		BLAZE_TRACE_RPC_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::submitgamereport] Platform Host --> " << mPlayerData->getPlayerId());
	}
    //if (BLAZE_IS_TRACE_RPC_ENABLED(BlazeRpcLog::gamemanager)) {
        //char8_t buf[20240];
        //BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "-> [FriendliesGameSession][" << mPlayerData->getPlayerId() << "]::SubmitGameReport" << "\n" << request.print(buf, sizeof(buf)));
   //}
    error = mPlayerData->getComponentProxy()->mGameReportingProxy->submitGameReport(request);
	if (error == ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "FriendliesGameSession::submitgamereport success for " << mPlayerData->getPlayerId());
	}
	else
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "FriendliesGameSession::submitgamereport failed. Error(" << ErrorHelp::getErrorName(error) << ") for "<<mPlayerData->getPlayerId());
	}

    return error;

}

void FriendliesGameSession::handleNotifyGameSetUp(NotifyGameSetup* notify, uint16_t type )
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
    
	mTopologyHostConnectionGroupId = notify->getGameData().getTopologyHostInfo().getConnectionGroupId();

	// Add game ID to our created game slist
	if (isPlatformHost)
    {
		addFriendliesCreatedGame(notify->getGameData().getGameId()); 
	}
    //  Set Game State
    setPlayerState(notify->getGameData().getGameState());
    if (FriendliesGameDataRef(mPlayerData).isGameIdExist(myGameId))
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]::receiving notification(" << type << ") and gameId " <<
                        notify->getGameData().getGameId() << "already present in friendliesGamesMap");        
    }
    else
    {
        //  create new game data
        FriendliesGameDataRef(mPlayerData).addGameData(notify);
    }

	StressGameInfo* ptrGameInfo = FriendliesGameDataRef(mPlayerData).getGameData(myGameId);
    if (!ptrGameInfo)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "] gameId " << myGameId <<" not present in friendliesGamesMap");
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
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << " OpponentPlayerInfo PlayerId=" << getOpponentPlayerInfo().getPlayerId() << " ConnGroupId=" << getOpponentPlayerInfo().getConnGroupId().toString());
		}
	}
	//meshEndpointsConnected RPC to self
	BlazeObjectId targetGroupId = mPlayerData->getConnGroupId();
	meshEndpointsConnected(targetGroupId, 0, 0, myGameId);
	if (!isPlatformHost)
	{
		BlazeObjectId topologyHostConnId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, mTopologyHostConnectionGroupId);
		meshEndpointsConnected(topologyHostConnId, 0, 4, myGameId);
	}
	/*if(getOpponentPlayerInfo().getPlayerId() != 0)
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
	}*/
	
	if (isPlatformHost)
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "calling finalizeGameCreation");
        finalizeGameCreation();
    }

    if (IsPlayerPresentInList((PlayerModule*)(mPlayerData->getOwner()), mPlayerData->getPlayerId(), FRIENDLIES) == true)
    {
        uint16_t count = FriendliesGameDataRef(mPlayerData).getPlayerCount(myGameId);
        FriendliesGameDataRef(mPlayerData).setPlayerCount(myGameId, ++count);
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "]:GameId=" << myGameId << " Player count = " << count);
    }

}

BlazeRpcError FriendliesGameSession::createGame()
{
	BlazeRpcError err= ERR_OK;
	Blaze::GameManager::CreateGameRequest request;
	Blaze::GameManager::CreateGameResponse response;
	
	if ( !mPlayerData->isConnected())
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "[Friendlies::User Disconnected = " << mPlayerData->getPlayerId());
		return ERR_DISCONNECTED;
	}
	
	NetworkAddress *hostAddress = &request.getCommonGameData().getPlayerNetworkAddress();
	hostAddress->switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
	hostAddress->getIpPairAddress()->getExternalAddress().setIp(mPlayerData->getConnection()->getAddress().getIp());
	hostAddress->getIpPairAddress()->getExternalAddress().setMachineId(0);
	hostAddress->getIpPairAddress()->getExternalAddress().setPort(mPlayerData->getConnection()->getAddress().getPort());
	hostAddress->getIpPairAddress()->getExternalAddress().copyInto(hostAddress->getIpPairAddress()->getInternalAddress());
	hostAddress->getIpPairAddress()->getMachineId();
	Collections::AttributeMap& gameAttribs = request.getGameCreationData().getGameAttribs();
 
    gameAttribs["fifaClubLeague"] = "16";
    gameAttribs["fifaClubNumPlayers"] = "0";
    gameAttribs["fifaCustomController"] = "0";
    gameAttribs["fifaGameSpeed"] = "1";  
    gameAttribs["fifaGKControl"] = "1";
    gameAttribs["fifaHalfLength"] = "0"; 
    gameAttribs["fifaMatchupHash"] = "4356147";  // 
    gameAttribs["fifaPPT"] = "0";
    gameAttribs["fifaRoster"] = "0";
	gameAttribs["FUTFriendlyType"] = "0";	//	Added as per new logs
    gameAttribs["Gamemode_Name"] = "h2h_friendlies_match"; 
    gameAttribs["OSDK_categoryId"] = "gameType20"; 
    gameAttribs["OSDK_clubId"] = "0";
    gameAttribs["OSDK_coop"] = "1";  // As per the new TTY; added newly;
    gameAttribs["OSDK_gameMode"] = "20";
    gameAttribs["OSDK_rosterURL"] = "";
    gameAttribs["OSDK_rosterVersion"] = "1";
    gameAttribs["OSDK_sponsoredEventId"] = "0";
	gameAttribs["ssfMatchType"] = "-1";
	gameAttribs["ssfOppType"] = "0";
	gameAttribs["ssfStadium"] = "0";
	gameAttribs["ssfWalls"] = "-1";

    Blaze::EntryCriteriaMap& map = request.getGameCreationData().getEntryCriteriaMap();
    map["OSDK_maxDNF"] = "stats_dnf <= 100";
    map["OSDK_skillLevel"] = "stats_skillLevel >= 1 && stats_skillLevel <= 99";
    request.setGamePingSiteAlias("");  // As per the new TTY; need to confirm?

    request.getPlayerJoinData().setGameEntryType(GAME_ENTRY_TYPE_DIRECT);  // As per the new TTY;
    request.getGameCreationData().setGameName("");  // As per the new TTY;
	request.getGameCreationData().getTeamIds().push_back(65534);
	request.getGameCreationData().setDisconnectReservationTimeout(0);
	request.getGameCreationData().setGameModRegister(0);
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
	if (StressConfigInst.getPlatform() == PLATFORM_XONE) {
        request.getGameCreationData().getGameSettings().setBits(0x00040424);
    } else {
        request.getGameCreationData().getGameSettings().setBits(0x00000424);
    }

	request.setGameEventContentType("application/json");
	request.setGameStartEventUri("");
    request.setGameReportName("gameType20");
    request.setGameStatusUrl("");  // As per the new TTY;

    request.setServerNotResetable(false);
    request.getGameCreationData().setNetworkTopology(Blaze::PEER_TO_PEER_FULL_MESH);

	request.getSlotCapacities().at(SLOT_PUBLIC_PARTICIPANT) = 0;
    request.getSlotCapacities().at(SLOT_PRIVATE_PARTICIPANT) = 2;
	request.getSlotCapacities().at(SLOT_PUBLIC_SPECTATOR) = 0;
	request.getSlotCapacities().at(SLOT_PRIVATE_SPECTATOR) = 0;
	request.setPersistedGameId("");  // As per the new TTY

    request.getGameCreationData().setMaxPlayerCapacity(2);

    request.getGameCreationData().setPresenceMode(Blaze::PRESENCE_MODE_PRIVATE);
    request.getGameCreationData().setQueueCapacity(0);  // As per the new TTY;
    
    request.getPlayerJoinData().setGroupId(mPlayerData->getConnGroupId());
    Blaze::GameManager::PerPlayerJoinData * joinData = request.getPlayerJoinData().getPlayerDataList().pull_back();
	joinData->getUser().setBlazeId(mPlayerData->getPlayerId());
	if(StressConfigInst.getPlatform() == Platform::PLATFORM_XONE)
	{
		joinData->getUser().setPersonaNamespace("xbox");
	}
	else
	{
		joinData->getUser().setPersonaNamespace("ps3");
	}
	joinData->getUser().setAccountLocale(1701729619);
	joinData->getUser().setAccountCountry(21843);
	joinData->getUser().setOriginPersonaId(0);
	joinData->getUser().setPidId(0);
    joinData->getUser().setName(mPlayerData->getLogin()->getUserLoginInfo()->getPersonaDetails().getDisplayName());
    joinData->getUser().setAccountId(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
	joinData->getUser().setExternalId(mPlayerData->getLogin()->getUserLoginInfo()->getPersonaDetails().getExtId());
	joinData->setEncryptedBlazeId("");
	joinData->setIsOptionalPlayer(false);
	joinData->setRole("");
	joinData->setSlotType(INVALID_SLOT_TYPE);
	joinData->setTeamId(65535);
	joinData->setTeamIndex(65535);

	request.getPlayerJoinData().setDefaultRole("");
	request.getPlayerJoinData().setDefaultSlotType(Blaze::GameManager::SLOT_PRIVATE_PARTICIPANT);
    request.getPlayerJoinData().setDefaultTeamIndex(0);  // As per the new TTY;
    request.getPlayerJoinData().setDefaultTeamId(65534);

	request.getTournamentIdentification().setTournamentId("");
	request.getTournamentIdentification().setTournamentOrganizer("");

	request.getTournamentSessionData().setArbitrationTimeout(31536000000000);
	request.getTournamentSessionData().setForfeitTimeout(31449600000000);
	request.getTournamentSessionData().setTournamentDefinition("");
	request.getTournamentSessionData().setScheduledStartTime("1970-01-01T00:00:00Z");

    request.getGameCreationData().setVoipNetwork(Blaze::VOIP_PEER_TO_PEER);
	request.getCommonGameData().setGameType(GAME_TYPE_GAMESESSION);
    request.getCommonGameData().setGameProtocolVersionString(StressConfigInst.getGameProtocolVersionString());
	// Set random ping sites from the list of allowed ones
	request.setGamePingSiteAlias(mPlayerData->getPlayerPingSite());
	request.setGameEventAddress("");
	request.setGameEndEventUri("");
	
	//char8_t buf[10240];
	//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "CreateGame Request: \n"<< request.print(buf, sizeof(buf)));
	
	err = mPlayerData->getComponentProxy()->mGameManagerProxy->createGame(request, response);

	if (err == ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "Friendlies::CreateGame success.");
	}
	else
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "Friendlies::CreateGame failed. Error(" << ErrorHelp::getErrorName(err) << ")");
	}
	return err;
}



BlazeRpcError FriendliesGameSession::joinGame(GameId gameId)
{
	BlazeRpcError err = ERR_OK;
	Blaze::GameManager::JoinGameRequest request;
    Blaze::GameManager::JoinGameResponse response;
    Blaze::EntryCriteriaError error;

	if ( !mPlayerData->isConnected())
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "[Friendlies::User Disconnected = " << mPlayerData->getPlayerId());
		return ERR_DISCONNECTED;
	}

	NetworkAddress *hostAddress = &request.getCommonGameData().getPlayerNetworkAddress();
	request.getCommonGameData().setGameType(GAME_TYPE_GAMESESSION);
	hostAddress->switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
	hostAddress->getIpPairAddress()->getExternalAddress().setIp(mPlayerData->getConnection()->getAddress().getIp());
	hostAddress->getIpPairAddress()->getExternalAddress().setMachineId(0);
	hostAddress->getIpPairAddress()->getExternalAddress().setPort(mPlayerData->getConnection()->getAddress().getPort());
	hostAddress->getIpPairAddress()->getExternalAddress().copyInto(hostAddress->getIpPairAddress()->getInternalAddress());
	hostAddress->getIpPairAddress()->getMachineId();	//	Need to check
	request.getCommonGameData().setGameProtocolVersionString(StressConfigInst.getGameProtocolVersionString());
    request.setJoinMethod(Blaze::GameManager::JOIN_BY_INVITES);
    request.setRequestedSlotId(255);
    request.getPlayerJoinData().setDefaultSlotType(Blaze::GameManager::SLOT_PRIVATE_PARTICIPANT);
	request.getPlayerJoinData().setGroupId(mPlayerData->getConnGroupId());
    Blaze::GameManager::PerPlayerJoinData * joinerData = request.getPlayerJoinData().getPlayerDataList().pull_back();
	joinerData->getUser().setBlazeId(mPlayerData->getPlayerId());
	if(StressConfigInst.getPlatform() == Platform::PLATFORM_XONE)
	{
		joinerData->getUser().setPersonaNamespace("xone");
	}
	else
	{
		joinerData->getUser().setPersonaNamespace("ps3");
	}
	joinerData->getUser().setPersonaNamespace("");
	joinerData->getUser().setExternalId(mPlayerData->getExternalId());
    joinerData->getUser().setName(mPlayerData->getLogin()->getUserLoginInfo()->getPersonaDetails().getDisplayName());
    joinerData->getUser().setAccountId(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
	joinerData->getUser().setAccountLocale(1701729619);
	joinerData->getUser().setAccountCountry(21843);
	joinerData->getUser().setOriginPersonaId(0);
	joinerData->getUser().setPidId(0);

	joinerData->setEncryptedBlazeId("");
	joinerData->setIsOptionalPlayer(false);
	joinerData->setRole("");
	joinerData->setSlotType(INVALID_SLOT_TYPE);
	joinerData->setTeamId(ANY_TEAM_ID);
	joinerData->setTeamIndex(ANY_TEAM_ID + 1);
	
    request.setGameId(gameId);
    request.getPlayerJoinData().setDefaultTeamId(ANY_TEAM_ID + 1);
    request.getPlayerJoinData().setDefaultTeamIndex(ANY_TEAM_ID + 1);
	request.getPlayerJoinData().setDefaultRole("");
	request.getPlayerJoinData().setGameEntryType(GAME_ENTRY_TYPE_DIRECT);
	
	//char8_t buf[10240];
	//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "JoinGame RPC : \n" << "\n" << request.print(buf, sizeof(buf)));
	
	err = mPlayerData->getComponentProxy()->mGameManagerProxy->joinGame(request, response, error);
	if (err == ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "Friendlies::JoinGame success.");
	}
	else
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "Friendlies::JoinGame failed. Error(" << ErrorHelp::getErrorName(err) << ")");
	}
	return err;
}

void FriendliesGameSession::addFriendliesCreatedGame(GameId gameId)
{
	CreatedGamesList* gameListReference = ((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->getFriendliesGameList();
	CreatedGamesList::iterator it = gameListReference->begin();
	while( it != gameListReference->end())
	{
		if(gameId == *it)
		{
			return;
		}
		it++;
	}
	gameListReference->push_back(gameId);
	//it = gameListReference->begin();
	//while (it != gameListReference->end())
	//{
	//	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, " Game ID : " << *it << " " << mPlayerData->getPlayerId() );
	//	it++;
	//}
}

GameId FriendliesGameSession::getRandomCreatedGame()
{
		CreatedGamesList* gameListReference = ((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->getFriendliesGameList();
		if( gameListReference->size() == 0)
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::getRandomCreatedGame] The gameId map is empty " << mPlayerData->getPlayerId() );
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
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FriendliesGameSession::getRandomCreatedGame]  Randomly fetched GameId : "<< randomGameId);
		return randomGameId;
}

}
}