//  *************************************************************************************************
//
//   File:    dropingamesession.cpp
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
#include "dropingamesession.h"
#include "utility.h"
#include "gamereporting/fifa/tdf/fifaotpreport.h"

using namespace Blaze::GameManager;
using namespace Blaze::GameReporting; 

namespace Blaze {
namespace Stress {

	DropinPlayerGameSession::DropinPlayerGameSession(StressPlayerInfo* playerData, uint32_t GameSize, GameNetworkTopology Topology, GameProtocolVersionString GameProtocolVersionString)
    : GameSession(playerData, GameSize, Topology, GameProtocolVersionString)
{
		mGameReportingId = 0;
}

	DropinPlayerGameSession::~DropinPlayerGameSession()
{
}

void DropinPlayerGameSession::asyncHandlerFunction(uint16_t type, EA::TDF::Tdf *notification)
{
    if ( mPlayerData != NULL && mPlayerData->isPlayerActive())
    {
            //  This Notification has not been handled so use the Default Handler
            onGameSessionDefaultAsyncHandler(type, notification);
    }
}

void DropinPlayerGameSession::onGameSessionDefaultAsyncHandler(uint16_t type, EA::TDF::Tdf* notification)
{
    //  This is abstract class and handle the async handlers in the derived class

    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]:Message Type =" << type);
    switch (type)
    {
        case GameManagerSlave::NOTIFY_NOTIFYGAMESETUP:
            //  Async notification sent to the player who joined a game.  Triggered by JoinGame Request or a Matchmaking session finding a game.
            {
                NotifyGameSetup* notify = (NotifyGameSetup*)notification;
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::asyncHandlerFunction]: Received  NOTIFY_NOTIFYGAMESETUP." << mPlayerData->getPlayerId());
				if(notify->getGameData().getGameType() == GAME_TYPE_GAMESESSION)
				{
					handleNotifyGameSetUp(notify);
					mGameReportingId = notify->getGameData().getGameReportingId();
				}
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMESTATECHANGE:
            {
                NotifyGameStateChange* notify = (NotifyGameStateChange*)notification;
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMESTATECHANGE, state=" <<
                                notify->getNewGameState());
                if (myGameId == notify->getGameId())
                {
					if( getPlayerState() == IN_GAME && notify->getNewGameState() == POST_GAME )
					{
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::NOTIFY_NOTIFYGAMESTATECHANGE][" << mPlayerData->getPlayerId() << "]: submitGameReport");
						submitGameReport();
						leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, myGameId);
					}

                    //  Set Game State
                    setPlayerState(notify->getNewGameState());
					DropinPlayerGameDataRef(mPlayerData).setGameState(myGameId, notify->getNewGameState());
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMERESET:
            {
                NotifyGameReset* notify = (NotifyGameReset*)notification;
                StressGameInfo* gameInfo = DropinPlayerGameDataRef(mPlayerData).getGameData(notify->getGameData().getGameId());
                if (gameInfo != NULL && gameInfo->getPlatformHostPlayerId() == mPlayerData->getPlayerId())
                {
                    //  TODO: host specific actions
                }
                BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMERESET.gameId=" <<
                               notify->getGameData().getGameId());
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINING:
            {
                NotifyPlayerJoining* notify = (NotifyPlayerJoining*)notification;
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERJOINING.gameId=" << notify->getGameId());

				//meshEndpointsConnected for GAME_TYPE_GAMESESSION
				if(notify->getGameId() == myGameId)
				{
					// Add player to game map
					mPlayerInGameMap.insert(notify->getJoiningPlayer().getPlayerId());
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::NOTIFYPLAYERJOINING:GAME_TYPE_GAMESESSION][ GameID : "<< myGameId << " joinerId " << notify->getJoiningPlayer().getPlayerId());

					//meshEndpointsConnected
					BlazeObjectId targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, notify->getJoiningPlayer().getConnectionGroupId());
					meshEndpointsConnected(targetGroupId, 0, 0, myGameId);
				}
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS:
            {
                BLAZE_INFO(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::handlerFunction][%" PRIu64 "]: Received  NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS.", mPlayerData->getPlayerId());
                //NotifyGameSetup* notify = (NotifyGameSetup*)notification;
                
                //  In quickmatch scenario NOTIFYGAMESETUP is not recieved anymore.Instead NOTIFYJOININGPLAYERINITIATECONNECTIONS contains all the game related data
                //handleNotifyGameSetUp(notify, GameManagerSlave::NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS);
				
            }
            break;

        case GameManagerSlave::NOTIFY_NOTIFYPLATFORMHOSTINITIALIZED:
            {
                NotifyPlatformHostInitialized* notify = (NotifyPlatformHostInitialized*)notification;
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLATFORMHOSTINITIALIZED.gameId=" <<
                                notify->getGameId());
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMELISTUPDATE:
            {
                //NotifyGameListUpdate* notify = (NotifyGameListUpdate*)notification;
                BLAZE_INFO(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::asyncHandlerFunction][%" PRIu64 "]: Received  NOTIFY_NOTIFYGAMELISTUPDATE.", mPlayerData->getPlayerId());
                
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINCOMPLETED:
            {
                NotifyPlayerJoinCompleted* notify = (NotifyPlayerJoinCompleted*)notification;
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERJOINCOMPLETED.");
				if (notify->getGameId() == myGameId)
				{	
					// Add player to game map
					mPlayerInGameMap.insert(notify->getPlayerId());
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::NOTIFYPLAYERJOINCOMPLETED:mPlayerInGameMap][ GameID : "<< myGameId << " playerID= " << mPlayerData->getPlayerId() << " joinerId " << notify->getPlayerId());

					if (IsPlayerPresentInList((PlayerModule*)(mPlayerData->getOwner()), notify->getPlayerId(), DROPINPLAYER) == true)
					{
						uint16_t count = DropinPlayerGameDataRef(mPlayerData).getPlayerCount(myGameId);
						DropinPlayerGameDataRef(mPlayerData).setPlayerCount(myGameId, ++count);
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::NOTIFYPLAYERJOINCOMPLETED][" << mPlayerData->getPlayerId() << "]:GameId=" << myGameId << " Player count = " << count);
					}
					
						//invokeUdatePrimaryExternalSessionForUser(mPlayerData, myGameId, "", "", myGameId, GAME_JOIN, GAME_TYPE_GROUP, ACTIVE_CONNECTED);
					updatePrimaryExternalSessionForUserRequest(mPlayerData, myGameId, myGameId, UPDATE_PRESENCE_REASON_GAME_JOIN, GAME_TYPE_GAMESESSION, ACTIVE_CONNECTED, PRESENCE_MODE_STANDARD, false);
					
				}
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLAYERREMOVED:
            {
                NotifyPlayerRemoved* notify = (NotifyPlayerRemoved*)notification;
				//If Game_Type_Gamesession
				StressGameInfo* ptrGameInfo = DropinPlayerGameDataRef(mPlayerData).getGameData(myGameId);
                if (!ptrGameInfo)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]::receiving notification(" << type << ") and gameId " << myGameId <<
                                    "not present in DropInGamesMap");
                    break;
                }
				/*char8_t buff[2048];
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager , "Game_Type_Gamesession  NOTIFYPLAYERREMOVED" << "\n" << notify->print(buff, sizeof(buff)));*/
				
                if (mPlayerData->getPlayerId() == notify->getPlayerId() && myGameId == notify->getGameId() )
                {
                    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERREMOVED for game [" << notify->getGameId() << "] reason: "<<notify->getPlayerRemovedReason());
					
					//meshEndpointsConnectionLost [to dedicated server], meshEndpointsDisconnected[to dedicated server],

					BlazeObjectId targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, ptrGameInfo->getTopologyHostConnectionGroupId());
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << " meshEndpointsDisconnected for grouID" << targetGroupId.toString());
					meshEndpointsConnectionLost(targetGroupId, 1, myGameId);
					meshEndpointsDisconnected(targetGroupId, DISCONNECTED);										
                }

                if ( myGameId == notify->getGameId()  && DropinPlayerGameDataRef(mPlayerData).isPlayerExistInGame(notify->getPlayerId(), notify->getGameId()) == true )
                {
					DropinPlayerGameDataRef(mPlayerData).removePlayerFromGameData(notify->getPlayerId(), notify->getGameId());
                    if (IsPlayerPresentInList((PlayerModule*)(mPlayerData->getOwner()), notify->getPlayerId(), DROPINPLAYER) == true)
                    {
                        uint16_t count = ptrGameInfo->getPlayerCount();
                        ptrGameInfo->setPlayerCount(--count);
                        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::asyncHandlerFunction]::NOTIFYPLAYERREMOVED[" << mPlayerData->getPlayerId() << "]:GameId=" << myGameId << " Player count = " << count
                                        << "for player=" << notify->getPlayerId());
                    }
                }
                //  if all players, in single exe, removed from the game delete the data from map
                if ( ptrGameInfo->getPlayerCount() <= 0)
                {
                    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERREMOVED - Game Id = " <<
                                   notify->getGameId() << ".");
					DropinPlayerGameDataRef(mPlayerData).removeGameData(notify->getGameId());
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
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMEREMOVED.");
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
                            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::OnNotifyMatchmakingFailed][" << mPlayerData->getPlayerId() << "]: Matchmake SESSION_TIMED_OUT.");
                        }
                        break;
                    case GameManager::SESSION_CANCELED:
                        {
                            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::OnNotifyMatchmakingFailed][" << mPlayerData->getPlayerId() << "]: Matchmake SESSION_CANCELED.");
                            //  Throw Execption;
                        }
						break;
                    default:
                        break;
                }
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
                if (notify == NULL) //&& (PLATFORM_HOST_MIGRATION != notify->getMigrationType()))
                {
                    break;
                }
				if(notify->getMigrationType() == HostMigrationType::TOPOLOGY_HOST_MIGRATION)
				{
					//   Update the host migration status as soon as migration starts - all players + game server does this.
					updateGameHostMigrationStatus(mPlayerData, notify->getGameId(), HostMigrationType::TOPOLOGY_HOST_MIGRATION);
				}
            }
            break;
        case UserSessionsSlave::NOTIFY_USERADDED:
            {
              //  NotifyUserAdded* notify = (NotifyUserAdded*)notification;
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
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::handlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYMESSAGE.");
				
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

void DropinPlayerGameSession::handleNotifyGameSetUp(NotifyGameSetup* notify, uint16_t type /* = GameManagerSlave::NOTIFY_NOTIFYGAMESETUP*/)
{
    if (notify->getGameData().getTopologyHostInfo().getPlayerId() == mPlayerData->getPlayerId())  
    {
            isTopologyHost = true;
    }
    if (notify->getGameData().getPlatformHostInfo().getPlayerId() == mPlayerData->getPlayerId())  {
        isPlatformHost = true;
    }
 
    setGameId(notify->getGameData().getGameId());
	/*char8_t buf[20240];
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Friendlies::RealFlow]: NotifyGameSetup hopefully from createGame : \n" << notify->print(buf, sizeof(buf)));*/
    //  Set Player State
    setPlayerState(notify->getGameData().getGameState());
	
    if (DropinPlayerGameDataRef(mPlayerData).isGameIdExist(myGameId))
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]::receiving notification(" << type << ") and gameId " <<
                        notify->getGameData().getGameId() << "already present in DropInGamesMap");
        //  Player will be added during NOTIFYPLAYERJOINCOMPLETED notification, not here.
    }
    else
    {
        //  create new game data
		DropinPlayerGameDataRef(mPlayerData).addGameData(notify);
    }

	StressGameInfo* ptrGameInfo = DropinPlayerGameDataRef(mPlayerData).getGameData(myGameId);
    if (!ptrGameInfo)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "] gameId " << myGameId <<" not present in DropInGamesMap");
		return ;
    }


	if (!isPlatformHost )
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "calling requestPlatformhost");
	//	requestPlatformHost(mPlayerData, myGameId, 0);
	}


	//meshEndpointsConnected RPC to self
	BlazeObjectId targetGroupId = mPlayerData->getConnGroupId();
	meshEndpointsConnected(targetGroupId, 0 , 0, myGameId);
	
	//meshEndpointsConnected RPC to Dedicated server
	BlazeObjectId topologyHostConnId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, notify->getGameData().getTopologyHostInfo().getConnectionGroupId());
	meshEndpointsConnected(topologyHostConnId, 0, 0, myGameId);
/*
	if (isPlatformHost)
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "calling finalizeGameCreation");
        finalizeGameCreation();
    }
	if (isPlatformHost )
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "calling updateGameSession");
		updateGameSession();
	}
	*/

//	updateGameSession(myGameId);

	
	//Add players to the map
	ReplicatedGamePlayerList &roster = notify->getGameRoster();
    if (roster.size() > 0)
    {     
        ReplicatedGamePlayerList::const_iterator citEnd = roster.end();
        for (ReplicatedGamePlayerList::const_iterator cit = roster.begin(); cit != citEnd; ++cit)
        {
            PlayerId playerId = (*cit)->getPlayerId();
			mPlayerInGameMap.insert(playerId);
        }        
    }

	//  Increase player count if found in same stress exe.
    if (IsPlayerPresentInList((PlayerModule*)(mPlayerData->getOwner()), mPlayerData->getPlayerId(), DROPINPLAYER) == true)
    {
        uint16_t count = DropinPlayerGameDataRef(mPlayerData).getPlayerCount(myGameId);
		DropinPlayerGameDataRef(mPlayerData).setPlayerCount(myGameId, ++count);
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "]:GameId=" << myGameId << " Player count = " << count);
    }
}

BlazeRpcError DropinPlayerGameSession::submitGameReport()
{
	BlazeRpcError error = ERR_OK;

	//StressGameInfo* ptrGameInfo = DropinPlayerGameDataRef(mPlayerData).getGameData(myGameId);
	/*if (!ptrGameInfo)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::submitGameReport][" << mPlayerData->getPlayerId() << " gameId " << myGameId << " not present in DropinPlayerGameDataRef");
		return ERR_SYSTEM;
	}*/

	//startGameReporting
	GameReporting::SubmitGameReportRequest request;
	request.setFinishedStatus(GAMEREPORT_FINISHED_STATUS_FINISHED);
	uint16_t minScore = 2;
	const int MAX_TEAM_COUNT = 11;
	const int MAX_PLAYER_VALUE = 100000;

	GameReporting::GameReport &gamereport = request.getGameReport();

	gamereport.setGameReportingId(mGameReportingId);

	gamereport.setGameReportName("gameType5");

	Blaze::GameReporting::FifaOTPReportBase::GameReport * mGameReport = BLAZE_NEW Blaze::GameReporting::FifaOTPReportBase::GameReport;
	Blaze::GameReporting::FifaOTPReportBase::PlayerReport* mplayerReport = BLAZE_NEW Blaze::GameReporting::FifaOTPReportBase::PlayerReport;

	mGameReport->setMom(mPlayerData->getPersonaName().c_str());
	mGameReport->setMember(0);

	Blaze::GameReporting::OSDKGameReportBase::OSDKReport *osdkReport = BLAZE_NEW Blaze::GameReporting::OSDKGameReportBase::OSDKReport;

	gamereport.setReport(*osdkReport);
	if (osdkReport != NULL)
	{
		Blaze::GameReporting::OSDKGameReportBase::OSDKGameReport& osdkGamereport = osdkReport->getGameReport();//osdkGamereport is wrt GAMR in TTY
		osdkGamereport.setGameType("gameType5");
		osdkGamereport.setFinishedStatus(1);
		osdkGamereport.setGameTime(5496);
		osdkGamereport.setArenaChallengeId(0);//To confirm setArenaChallengeId takes of type ArenaChallengeId, in log ARID = 0 (0x0000000000000000)
		osdkGamereport.setRoomId(0);
		osdkGamereport.setSimulate(false);
		osdkGamereport.setLeagueId(0);
		osdkGamereport.setRank(false);
		osdkGamereport.setRoomId(0);
		//osdkGamereport.setSponsoredEventId(0);

		Blaze::GameReporting::Fifa::CommonGameReport& commonGameReport = mGameReport->getCommonGameReport();//commonGameReport is CMGR wrt TTY;
		commonGameReport.setAwayKitId(1355);
		commonGameReport.setIsRivalMatch(false);
		commonGameReport.setWentToPk(0);
		commonGameReport.setBallId(-1);
		commonGameReport.setHomeKitId(0);

		//Fill awayStartingXI and homeStartingXI values here

		for (int iteration = 0; iteration < MAX_TEAM_COUNT; iteration++) {
			commonGameReport.getAwayStartingXI().push_back(Random::getRandomNumber(MAX_PLAYER_VALUE));
		}

		for (int iteration = 0; iteration < MAX_TEAM_COUNT; iteration++) {
			commonGameReport.getHomeStartingXI().push_back(Random::getRandomNumber(MAX_PLAYER_VALUE));
		}
		commonGameReport.setStadiumId(34);

		mGameReport->setMom(mPlayerData->getPersonaName().c_str());

		osdkGamereport.setCustomGameReport(*mGameReport);

		Blaze::GameReporting::OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = osdkReport->getPlayerReports();
		int playerMapSize = mPlayerInGameMap.size();
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::submitGameReport][" << mPlayerData->getPlayerId() << "]:player map size =" << playerMapSize);
		if (playerMapSize < 2)
		{
			BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[DropinPlayerGameSession::submitGameReport][" << mPlayerData->getPlayerId() << "]:player map size is wrong, return");
			return ERR_SYSTEM;
		}

		OsdkPlayerReportsMap.reserve(mPlayerInGameMap.size());
		int count = 0;
		for (PlayerInGameMap::iterator citPlayerIt = mPlayerInGameMap.begin(), citPlayerItEnd = mPlayerInGameMap.end();
			citPlayerIt != citPlayerItEnd; ++citPlayerIt)
		{
			GameManager::PlayerId playerId = *citPlayerIt;
			Blaze::GameReporting::OSDKGameReportBase::OSDKPlayerReport* playerReport = OsdkPlayerReportsMap.allocate_element();
			//As per the new TTY; To confirm whether is it at the right place?
			mplayerReport->setManOfTheMatch(0);
			mplayerReport->setCleanSheetsAny(0);
			mplayerReport->setCleanSheetsDef(0);
			mplayerReport->setCleanSheetsGoalKeeper(0);
			mplayerReport->setOtpGames(0);
			mplayerReport->setPos(4);

			Blaze::GameReporting::Fifa::CommonPlayerReport& fifaCommonPlayerReport = mplayerReport->getCommonPlayerReport();

			fifaCommonPlayerReport.setAssists(rand() % 10);
			fifaCommonPlayerReport.setGoalsConceded(rand() % 10);
			//mHasCleanSheets need to be set to 1 as per the new TTY16
			fifaCommonPlayerReport.setCorners(rand() % 10);
			fifaCommonPlayerReport.setPassAttempts(playerId % 15);
			fifaCommonPlayerReport.setPassesMade(rand() % 15);
			fifaCommonPlayerReport.setRating((float)(rand() % 10));
			fifaCommonPlayerReport.setSaves(rand() % 12);
			fifaCommonPlayerReport.setShots(rand() % 15);
			fifaCommonPlayerReport.setTackleAttempts(rand() % 15);
			fifaCommonPlayerReport.setTacklesMade(rand() % 10);
			fifaCommonPlayerReport.setFouls(rand() % 12);
			fifaCommonPlayerReport.setGoals(minScore + (rand() % 10));
			fifaCommonPlayerReport.setInterceptions(rand() % 10);//As per the new TTY; new one
																  //mHasMOTM = 1 (0x01) need to be set to 1 as per the new TTY16
			fifaCommonPlayerReport.setOffsides(rand() % 10);
			fifaCommonPlayerReport.setOwnGoals(rand() % 10);
			fifaCommonPlayerReport.setPkGoals(0);
			fifaCommonPlayerReport.setPossession(rand() % 10);//As per the new TTY it is 38,62...
			fifaCommonPlayerReport.setRedCard(rand() % 12);
			fifaCommonPlayerReport.setShotsOnGoal(rand() % 10);
			fifaCommonPlayerReport.setUnadjustedScore(0);//As per the new TTY; new one
			fifaCommonPlayerReport.setYellowCard(rand() % 12);

			//Changing the order as per new TTY;
			playerReport->setCustomDnf(0);
			playerReport->setClientScore(0);
			playerReport->setAccountCountry(0);
			playerReport->setFinishReason(0);
			playerReport->setGameResult(0);
			playerReport->setHome(true);
			playerReport->setLosses(0);
			playerReport->setName(mPlayerData->getPersonaName().c_str());
			playerReport->setOpponentCount(0);
			playerReport->setExternalId(0);
			playerReport->setNucleusId(mPlayerData->getPlayerId());
			playerReport->setPersona(mPlayerData->getPersonaName().c_str());
			playerReport->setPointsAgainst(rand() % 10);
			//playerReport->setUserResult(1);
			playerReport->setUserResult(rand() % 10);//To confirm
			playerReport->setScore(count + (rand() % 10));
			//playerReport->setSponsoredEventAccountRegion(0);
			playerReport->setSkill(rand() % 12);
			playerReport->setSkillPoints(rand() % 12);
			//playerReport->setTeam(0);
			playerReport->setTeam(rand() % 100);//To confirm
			playerReport->setTies(rand() % 12);
			playerReport->setWinnerByDnf(0);
			playerReport->setWins(rand() % 12);
			count++;
			if (playerId == mPlayerData->getPlayerId())
			{
				Blaze::GameReporting::OSDKGameReportBase::OSDKPrivatePlayerReport &osdkPrivatePlayerReport = playerReport->getPrivatePlayerReport();

				//As per the new TTY;
				Blaze::GameReporting::OSDKGameReportBase::IntegerAttributeMap &PlayerPrivateIntAttribMap = osdkPrivatePlayerReport.getPrivateIntAttributeMap();
				Blaze::Collections::AttributeMap &PlayerPrivateAttribMap = osdkPrivatePlayerReport.getPrivateAttributeMap();

				PlayerPrivateIntAttribMap["BANDAVGGM"] = (rand() % 1000);
				PlayerPrivateIntAttribMap["BANDAVGNET"] = (rand() % 1000);
				PlayerPrivateIntAttribMap["BANDHIGM"] = (rand() % 1000000);
				PlayerPrivateIntAttribMap["BANDHINET"] = (rand() % 1000000);
				PlayerPrivateIntAttribMap["BANDLOWGM"] = (rand() % 10);
				PlayerPrivateIntAttribMap["BANDLOWNET"] = (rand() % 10);
				PlayerPrivateIntAttribMap["BYTESRCVDNET"] = (rand() % 10);
				PlayerPrivateIntAttribMap["BYTESSENTNET"] = (rand() % 10);
				PlayerPrivateIntAttribMap["DROPPKTS"] = (rand() % 10);
				PlayerPrivateIntAttribMap["FPSAVG"] = (rand() % 10);
				PlayerPrivateIntAttribMap["FPSDEV"] = (rand() % 10);
				PlayerPrivateIntAttribMap["FPSHI"] = (rand() % 10);
				PlayerPrivateIntAttribMap["FPSLOW"] = (rand() % 10);
				PlayerPrivateIntAttribMap["GDESYNCEND"] = (rand() % 10);
				PlayerPrivateIntAttribMap["GDESYNCRSN"] = (rand() % 10);
				PlayerPrivateIntAttribMap["GENDPHASE"] = (rand() % 10);
				PlayerPrivateIntAttribMap["GAMEENDREASON"] = (rand() % 10);//As per the new TTY; added newly;
				PlayerPrivateIntAttribMap["GPCKTPERIOD"] = (rand() % 10);//As per the new TTY; added newly;
				PlayerPrivateIntAttribMap["GPCKTPERIODSTD"] = (rand() % 10);//As per the new TTY; added newly;
				PlayerPrivateIntAttribMap["GRESULT"] = (rand() % 10);
				PlayerPrivateIntAttribMap["GRPTTYPE"] = 9;
				PlayerPrivateIntAttribMap["GRPTVER"] = 7;
				PlayerPrivateIntAttribMap["GUESTS0"] = (rand() % 10);
				PlayerPrivateIntAttribMap["GUESTS1"] = (rand() % 10);
				PlayerPrivateIntAttribMap["LATEAVGGM"] = (rand() % 10);
				PlayerPrivateIntAttribMap["LATEAVGNET"] = (rand() % 10);
				PlayerPrivateIntAttribMap["LATEHIGM"] = (rand() % 10);
				PlayerPrivateIntAttribMap["LATEHINET"] = (rand() % 10);
				PlayerPrivateIntAttribMap["LATELOWGM"] = (rand() % 10);
				PlayerPrivateIntAttribMap["LATELOWNET"] = (rand() % 10);
				PlayerPrivateIntAttribMap["LATESDEVGM"] = (rand() % 10);
				PlayerPrivateIntAttribMap["LATESDEVNET"] = (rand() % 10);
				PlayerPrivateIntAttribMap["PKTLOSS"] = (rand() % 10);
				PlayerPrivateIntAttribMap["REALTIMEGAME"] = (rand() % 10);//As per the new TTY; added newly;
				PlayerPrivateIntAttribMap["REALTIMEIDLE"] = (rand() % 10);//As per the new TTY; added newly;
				PlayerPrivateIntAttribMap["USERSEND0"] = (rand() % 10);
				PlayerPrivateIntAttribMap["USERSEND1"] = (rand() % 10);
				PlayerPrivateIntAttribMap["USERSSTRT0"] = (rand() % 10);
				PlayerPrivateIntAttribMap["USERSSTRT1"] = (rand() % 10);
				PlayerPrivateIntAttribMap["VOIPEND0"] = (rand() % 10);
				PlayerPrivateIntAttribMap["VOIPEND1"] = (rand() % 10);
				PlayerPrivateIntAttribMap["VOIPSTRT0"] = (rand() % 10);
				PlayerPrivateIntAttribMap["VOIPSTRT1"] = (rand() % 10);
				PlayerPrivateIntAttribMap["VPROHACKREASON"] = (rand() % 10);//As per the new TTY; added newly;
				PlayerPrivateIntAttribMap["vpro_isvalid"] = 1;//As per the new TTY; added newly;
				PlayerPrivateIntAttribMap["attribute0_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute1_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute2_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute3_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute4_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute5_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute6_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute7_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute8_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute9_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute10_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute11_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute12_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute13_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute14_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute15_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute16_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute17_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute18_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute19_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute20_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute21_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute22_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute23_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute24_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute25_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute26_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute27_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute28_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute29_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute30_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute31_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute32_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["attribute33_type"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["game_end_w_enough_player"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["gid"] = getGameId();
				PlayerPrivateIntAttribMap["match_event_count"] = 273;
				PlayerPrivateIntAttribMap["matchevent_100"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["matchevent_101"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["matchevent_102"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["matchevent_103"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["matchevent_105"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["matchevent_106"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["matchevent_107"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["matchevent_111"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["matchevent_143"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["matchevent_145"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["matchevent_157"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["matchevent_175"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["matchevent_178"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["matchevent_183"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["matchevent_184"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["matchevent_212"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["matchevent_215"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["matchevent_216"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["matchevent_219"] = (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["matchevent_34"]	= (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["matchevent_35"]	= (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["matchevent_36"]	= (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["matchevent_37"]	= (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["matchevent_39"]	= (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["matchevent_4"]	= (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["matchevent_97"]	= (uint32_t)(rand() % 14);
				PlayerPrivateIntAttribMap["matchevent_99"]	= (uint32_t)(rand() % 14);

				PlayerPrivateIntAttribMap["player_attribute_count"] = 34;
				PlayerPrivateIntAttribMap["vpro_rating_floor"] = (uint32_t)(rand() % 10);

				//As per the new TTY; BYTESSENTGM and BYTESRCVDGM not existed in the log;
				/*PlayerPrivateIntAttribMap["BYTESSENTGM"] = eastl::string().sprintf("%d", rand() % 10).c_str();
				PlayerPrivateIntAttribMap["BYTESRCVDGM"] = eastl::string().sprintf("%d", rand() % 10).c_str();*/

				//As per the new TTY; added PlayerPrivateAttribMap newly;
				int randStump = Random::getRandomNumber(100);
				if (randStump < 30)
				{
					PlayerPrivateAttribMap["VPROHACKATTR"] = "082|084|079|075|078|083|070|083|065|076|051|081|081|083|083|086|081|081|077|082|085|045|086|090|051|050|083|079|081|010|010|010|010|010|";
				}
				else if (randStump < 60)
				{
					PlayerPrivateAttribMap["VPROHACKATTR"] = "073|076|070|076|080|084|089|083|082|065|087|045|055|086|040|061|040|060|090|076|080|082|060|040|087|082|042|060|050|010|010|010|010|010|";
				}
				else
				{
					PlayerPrivateAttribMap["VPROHACKATTR"] = "NH";
				}
				PlayerPrivateAttribMap["VProHeight"] = eastl::string().sprintf("%d", rand() % 500).c_str();
				PlayerPrivateAttribMap["VProName"] = "anxiety";
				PlayerPrivateAttribMap["VProNationality"] = eastl::string().sprintf("%d", rand() % 100).c_str();
				PlayerPrivateAttribMap["VProOverall"] = eastl::string().sprintf("%d", rand() % 100).c_str();
				PlayerPrivateAttribMap["VProPos"] = eastl::string().sprintf("%d", rand() % 100).c_str();
				PlayerPrivateAttribMap["VProRating"] = eastl::string().sprintf("%d", rand() % 10).c_str();
				PlayerPrivateAttribMap["vprostat_accomplishment"] = "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@";//eastl::string().sprintf("%d", rand() % 10).c_str();
				PlayerPrivateAttribMap["vprostat_ingame"] = "??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????";//eastl::string().sprintf("%d", rand() % 10).c_str();
				PlayerPrivateAttribMap["vprostat_savedstatrole0c0"] = "??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????";//eastl::string().sprintf("%d", rand() % 10).c_str();
				PlayerPrivateAttribMap["vprostat_savedstatrole0c1"] = "??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????";//eastl::string().sprintf("%d", rand() % 10).c_str();
				PlayerPrivateAttribMap["vprostat_savedstatrole0c2"] = "??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????";//eastl::string().sprintf("%d", rand() % 10).c_str();
				PlayerPrivateAttribMap["vprostat_savedstatrole0c3"] = "??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????";//eastl::string().sprintf("%d", rand() % 10).c_str();
				PlayerPrivateAttribMap["vprostat_savedstatrole1c0"] = "??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????";//eastl::string().sprintf("%d", rand() % 10).c_str();
				PlayerPrivateAttribMap["vprostat_savedstatrole1c1"] = "??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????";//eastl::string().sprintf("%d", rand() % 10).c_str();
				PlayerPrivateAttribMap["vprostat_savedstatrole1c2"] = "??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????";//eastl::string().sprintf("%d", rand() % 10).c_str();
				PlayerPrivateAttribMap["vprostat_savedstatrole1c3"] = "??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????";//eastl::string().sprintf("%d", rand() % 10).c_str();
				PlayerPrivateAttribMap["vprostat_savedstatrole2c0"] = "??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????";//eastl::string().sprintf("%d", rand() % 10).c_str();
				PlayerPrivateAttribMap["vprostat_savedstatrole2c1"] = "??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????";//eastl::string().sprintf("%d", rand() % 10).c_str();
				PlayerPrivateAttribMap["vprostat_savedstatrole2c2"] = "??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????";//eastl::string().sprintf("%d", rand() % 10).c_str();
				PlayerPrivateAttribMap["vprostat_savedstatrole2c3"] = "??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????";//eastl::string().sprintf("%d", rand() % 10).c_str();
				PlayerPrivateAttribMap["vprostat_savedstatrole3c0"] = "??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????";//eastl::string().sprintf("%d", rand() % 10).c_str();
				PlayerPrivateAttribMap["vprostat_savedstatrole3c1"] = "??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????";//eastl::string().sprintf("%d", rand() % 10).c_str();
				PlayerPrivateAttribMap["vprostat_savedstatrole3c2"] = "??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????";//eastl::string().sprintf("%d", rand() % 10).c_str();
				PlayerPrivateAttribMap["vprostat_savedstatrole3c3"] = "??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????";//eastl::string().sprintf("%d", rand() % 10).c_str();
				PlayerPrivateAttribMap["VProStyle"] = eastl::string().sprintf("%d", rand() % 10).c_str();
			}
			playerReport->setCustomPlayerReport(*mplayerReport);
			OsdkPlayerReportsMap[playerId] = playerReport;
		}

	}
	
	//char8_t buf[404800];
	//BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "SubmitGameReport RPC : \n" << request.print(buf, sizeof(buf)));
	error = mPlayerData->getComponentProxy()->mGameReportingProxy->submitGameReport(request);
	if (error == ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "DropinPlayerGameSession::submitgamereport success.");
	}
	else
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "DropinPlayerGameSession::submitgamereport failed. Error(" << ErrorHelp::getErrorName(error) << ")");
	}

    return error;
}

}  // namespace Stress
}  // namespace Blaze
