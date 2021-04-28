//  *************************************************************************************************
//
//   File:    livecompetitions.cpp
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#include "livecompetitions.h"
#include "playermodule.h"
#include "gamesession.h"
#include "utility.h"
#include "reference.h"
#include "framework/util/shared/base64.h"
#include "gamemanager/tdf/matchmaker.h"
#include "framework/tdf/entrycriteria.h"
#include "osdksettings/tdf/osdksettingstypes.h"
#include "clubs/tdf/clubs.h"
//#include "mail/tdf/mail.h"


namespace Blaze {
namespace Stress {

LiveCompetitions::LiveCompetitions(StressPlayerInfo* playerData)
    : GamePlayUser(playerData),
      LiveCompetitionsGameSession(playerData, 2 /*gamesize*/, PEER_TO_PEER_FULL_MESH /*Topology*/,
                             StressConfigInst.getGameProtocolVersionString() /*GameProtocolVersionString*/),
      mIsMarkedForLeaving(false)
{
}

LiveCompetitions::~LiveCompetitions()
{
    BLAZE_INFO(BlazeRpcLog::util, "[LiveCompetitions][Destructor][%" PRIu64 "]: LIVECOMPETITIONS destroy called", mPlayerInfo->getPlayerId());
    stopReportTelemtery();
    //  deregister async handlers to avoid client crashes when disconnect happens
    deregisterHandlers();
    //  wait for some time to allow already scheduled jobs to complete
    sleep(30000);
}

void LiveCompetitions::deregisterHandlers()
{
    LiveCompetitionsGameSession::deregisterHandlers();
}

void LiveCompetitions::preLogoutAction()
{
    //leaveParty(mPlayerInfo);
}

BlazeRpcError LiveCompetitions::lobbyRPCs()
{
	Blaze::GameReporting::QueryVarValuesList queryValues;
	queryValues.clear();
	char8_t queryVarValueStr[16];
	blaze_snzprintf(queryVarValueStr, sizeof(queryVarValueStr), "%" PRId64 "", mPlayerInfo->getPlayerId());
	queryValues.push_back(queryVarValueStr);
	
	BlazeRpcError err = ERR_OK;
	int32_t maxCount = 50;
	Blaze::Clubs::GetMembersResponse response;
	FetchConfigResponse fetchConfigResponse;
	GetStatsByGroupRequest::EntityIdList entityList;
	entityList.clear();
	err = getMembers(mPlayerInfo, getMyClubId(), maxCount, response);
	if (getMyClubId() > 0)
	{
		err = getPetitions(mPlayerInfo, getMyClubId(), Blaze::Clubs::PetitionsType::CLUBS_PETITION_SENT_TO_CLUB);
	}
	err = getStatGroup(mPlayerInfo, "VProPosition"); 
	entityList.push_back(mPlayerInfo->getPlayerId());
	
	if (isPlatformHost)
	{
		entityList.push_back(mPlayerInfo->getPlayerId());
	}
	
	if ( getMyClubId() != 0)
	{
		err = getStatsByGroupAsync(mPlayerInfo, "VProPosition", entityList, "club",getMyClubId() );
	}
	err = lookupUsersIdentification(mPlayerInfo);
	err = setConnectionState(mPlayerInfo);
	err = playerEventRegistrationflow();
	Blaze::PersonaNameList personaNameList;
	personaNameList.clear();
	err = lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
	err = getStatGroup(mPlayerInfo, "LiveCompSeasonalPlayMonthly"); 
	err = getStatsByGroupAsync(mPlayerInfo, "LiveCompSeasonalPlayMonthly", entityList,"competitionId", getLCEventId(), 1);
	err = fetchClientConfig(mPlayerInfo, mCompetitionName, fetchConfigResponse);

	char8_t competitionIdString[32];
    blaze_snzprintf(competitionIdString, sizeof(competitionIdString), "%" PRId32, geteventId());
	queryValues.push_back(competitionIdString);
	err = getGameReportViewInfo(mPlayerInfo, "SponsoredEventRecentGames");
	err = getGameReportView(mPlayerInfo, "SponsoredEventRecentGames", queryValues);
	err = userSettingsSave(mPlayerInfo);
	
	return err;
}

BlazeRpcError LiveCompetitions::liveCompetitionsLeaderboards()
{
	BlazeRpcError result = ERR_OK;	
	getLeaderboardTreeAsync(mPlayerInfo, "BundesPS4LB");
	result = getLeaderboardGroup(mPlayerInfo, -1, "bundes_seasonal_currentPS4");
	result = getCenteredLeaderboard(mPlayerInfo, 0, 1, mPlayerInfo->getPlayerId(), "bundes_seasonal_currentPS4");
	sleep(1000);
	// FIltered Leaderboard
	FilteredLeaderboardStatsRequest::EntityIdList filterBoardEntityList;
	filterBoardEntityList.push_back(mPlayerInfo->getPlayerId());
	filterBoardEntityList.push_back(((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->getRandomPlayerId());
	filterBoardEntityList.push_back(((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->getRandomPlayerId());
	filterBoardEntityList.push_back(((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->getRandomPlayerId());
	getFilteredLeaderboard(mPlayerInfo, filterBoardEntityList, "bundes_seasonal_currentPS4", 4294967295);
	result = getLeaderboard(mPlayerInfo, "bundes_seasonal_currentPS4");
	
	sleep(5000);
	if ((int)Random::getRandomNumber(100) < 3)
	{
		Blaze::PersonaNameList personaNameList;
		//lookupUsers
		for (int i = 0; i < 10; i++)
		{
			personaNameList.push_back(StressConfigInst.getRandomPersonaNames());
		}
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
	}
	
	return result;
}


BlazeRpcError LiveCompetitions::simulateLoad()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitions::simulateLoad]:" << mPlayerData->getPlayerId());
    BlazeRpcError err = ERR_OK;
	
    err = simulateMenuFlow();

	if ((int)Random::getRandomNumber(100) < 100)
	{ 
		liveCompetitionsLeaderboards();
	}
	
	//eventID was read from config file with fixed distribution.
	seteventId(getLCEventId());
	blaze_snzprintf(mCompetitionName, sizeof(mCompetitionName), "FIFA_LIVE_COMP_SEASONALPLAY_%d", mEventId);
	lobbyRPCs();
	//Send MM request 
	err = startMatchmaking();
	
	if(err != ERR_OK) 
	{
		sleep(30000);
		return err; 
	}

	//CancelMatchmaking
	if ( Random::getRandomNumber(100) < (int)LC_CONFIG.cancelMatchMakingProbability )
	{		
		err = cancelMatchMakingScenario(mMatchmakingSessionId);
		if(err == ERR_OK)
		{
			return err;
		}
	}

    int64_t playerStateTimer = 0;
    int64_t  playerStateDurationMs = 0;
    GameState   playerStateTracker = getPlayerState();
    uint16_t stateLimit = 0;
    bool ingame_enter = false;

	// PlayerTimeStats RPC call
	if (StressConfigInst.getPTSEnabled())
	{
		// Update Eadp Stats(Read and Update Limits)
		if ((uint32_t)Random::getRandomNumber(100) < StressConfigInst.getEadpStatsProbability())
		{
			updateEadpStats(mPlayerInfo);
		}
	}
	
	while (1)
    {
        if ( !mPlayerData->isConnected())
        {
            BLAZE_ERR_LOG(BlazeRpcLog::util, "[LiveCompetitions::simulateLoad::User Disconnected = " << mPlayerData->getPlayerId());
            return ERR_DISCONNECTED;
        }
        if (playerStateTracker != getPlayerState())  //  state changed
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitions::simulateLoad:playerStateTracker]" << mPlayerData->getPlayerId() << " Time Spent in Sate: " << GameStateToString(
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
                if (playerStateDurationMs > LC_CONFIG.maxPlayerStateDurationMs)
                {
                    BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitions::simulateLoad:playerStateTracker][" << mPlayerData->getPlayerId() << "]: Player exceded maximum Game state Duration:" << playerStateDurationMs <<
                                   " ms" << " Current Game State:" << GameStateToString(getPlayerState()));
                    if (IN_GAME == getPlayerState())
                    {
                        mIsMarkedForLeaving = true;   //  Player exceed maximum duration in InGAME State so mark this Player to Leave from InGame
                    }
                }
            }
        }
        const char8_t * stateMsg = GameStateToString(getPlayerState());
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitions::simulateLoad]: Player: " << mPlayerInfo->getPlayerId() << ", GameId: " << myGameId << " Current state : " << stateMsg << " InStateDuration:" <<
                       playerStateDurationMs);
        switch (getPlayerState())
        {
            //   Sleep for all the below states
            case NEW_STATE:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitions::simulateLoad]" << mPlayerData->getPlayerId() << " with state=NEW_STATE");
                    stateLimit++;
                    if (stateLimit >= LC_CONFIG.GameStateIterationsMax)
                    {
                        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitions::simulateLoad]" << mPlayerData->getPlayerId() << " with state=NEW_STATE for long time and return");
						if (mCensusdataSubscriptionStatus)
						{
							err = unsubscribeFromCensusData(mPlayerInfo);
							if (err == ERR_OK)
							{
								UnsetSubscriptionSatatus();
							}
						}
                        return err;
                    }
                    //  Wait here if resetDedicatedserver() called/player not found for invite.
                    sleep(LC_CONFIG.GameStateSleepInterval);
                    break;
                }
            case PRE_GAME:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitions::simulateLoad]" << mPlayerData->getPlayerId() << " with state=PRE_GAME");
					setClientState(mPlayerInfo, ClientState::MODE_MULTI_PLAYER);
					setPlayerAttributes("REQ", "2");
					if (isPlatformHost)
					{
						setGameSettings(2063);
						advanceGameState(IN_GAME);
					}
                    sleep(LC_CONFIG.GameStateSleepInterval);
                    break;
                }
            case IN_GAME:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitions::simulateLoad]" << mPlayerData->getPlayerId() << " with state=IN_GAME");

                    if (!ingame_enter)
                    {
                        ingame_enter = true;
                        simulateInGame();
						//Start InGame Timer	
						if(isPlatformHost)
						{
							BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitions::simulateLoad]" << mPlayerData->getPlayerId() << " setInGameStartTime. gameID=" << myGameId);
							setInGameStartTime();
						}
                    }

                    if (RollDice(LC_CONFIG.inGameReportTelemetryProbability))
                    {
                        executeReportTelemetry();
                    }

					if( (isPlatformHost && IsInGameTimedOut()) || (mIsMarkedForLeaving == true) )
					{		
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitions::simulateLoad]" << mPlayerData->getPlayerId() << " IsInGameTimedOut/mIsMarkedForLeaving=true. gameID=" << myGameId);
						advanceGameState(POST_GAME);
					}
                    
                    sleep(LC_CONFIG.GameStateSleepInterval);
                    break;
                }
            case INITIALIZING:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitions::simulateLoad]" << mPlayerData->getPlayerId() << " with state=INITIALIZING");
					if (isPlatformHost)
					{
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitions::simulateLoad][" << mPlayerData->getPlayerId() << "calling advanceGameState");
						advanceGameState(PRE_GAME);
					}
                    sleep(LC_CONFIG.GameStateSleepInterval);
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
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitions::simulateLoad]" << mPlayerData->getPlayerId() << " with state=POST_GAME");
                 
                    //Submitgamereport is initiated from NOTIFY_NOTIFYGAMESTATECHANGE async notification
					
					invokeUdatePrimaryExternalSessionForUser(mPlayerInfo, myGameId, "", "", myGameId, GAME_LEAVE, GAME_TYPE_GAMESESSION, ACTIVE_CONNECTED);
					//Leave game
					//leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT);
					//BLAZE_INFO(BlazeRpcLog::gamemanager, "[LiveCompetitions::leaveGame][%" PRIu64 "]: player left game: GameId = %" PRIu64 "", mPlayerData->getPlayerId(), myGameId);
					//meshEndpointsDisconnected RPC is initiated from NOTIFY_NOTIFYPLAYERREMOVED
					postGameCalls();
					delayRandomTime(5000, 10000);
					setPlayerState(DESTRUCTING);
                    
                    sleep(LC_CONFIG.GameStateSleepInterval);
                    break;
                }
            case RESETABLE:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitions::simulateLoad]" << mPlayerData->getPlayerId() << " with state=RESETABLE");
                    //  mGameData will be freed when NOTIFY_NOTIFYGAMEREMOVED notification comes.
                    sleep(LC_CONFIG.GameStateSleepInterval);
                    break;
                }
            case DESTRUCTING:
                {
                    //  Player Left Game
                    //  Presently using this state when user is removed from the game and return from this function.
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitions::simulateLoad]" << mPlayerData->getPlayerId() << " with state=DESTRUCTING");
                    setClientState(mPlayerInfo, ClientState::MODE_MENU);
                    //  Return here
                    return err;
                }
            case MIGRATING:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitions::simulateLoad]" << mPlayerData->getPlayerId() << " with state=MIGRATING");
                    //  mGameData will be freed when NOTIFY_NOTIFYGAMEREMOVED notification comes.
                    sleep(10000);
                    break;
                }
            case GAME_GROUP_INITIALIZED:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitions::simulateLoad]" << mPlayerData->getPlayerId() << " with state=GAME_GROUP_INITIALIZED");
                    sleep(5000);
                    //if (playerStateDurationMs > LC_CONFIG.MaxGameGroupInitializedStateDuration)
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
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitions::simulateLoad]" << mPlayerData->getPlayerId() << " with state=" << getPlayerState());
                    //  shouldn't come here
                    return err;
                }
        }
    }
}

void LiveCompetitions::addPlayerToList()
{
    //  add player to universal list first
    Player::addPlayerToList();
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitions::addPlayerToList]: Player: " << mPlayerInfo->getPlayerId() << ", Game: " << myGameId);
    {
        ((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->liveCompetitionsMap.insert(eastl::pair<PlayerId, eastl::string>(mPlayerInfo->getPlayerId(), mPlayerInfo->getPersonaName()));
    }
}

void LiveCompetitions::removePlayerFromList()
{
    //  remove player from universal list first
    Player::removePlayerFromList();
    PlayerId  id = mPlayerInfo->getPlayerId();
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitions::removePlayerFromList]: Player: " << id << ", Game: " << myGameId);
    PlayerMap& playerMap = ((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->liveCompetitionsMap;
    PlayerMap::iterator start = playerMap.begin();
    PlayerMap::iterator end   = playerMap.end();
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

void LiveCompetitions::simulateInGame()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][LiveCompetitions]: In Game simulation:" << myGameId);
    
	setClientState(mPlayerInfo, ClientState::MODE_MULTI_PLAYER, ClientState::Status::STATUS_SUSPENDED);
	lookupUsersByPersonaNames(mPlayerInfo, "");

	if(mCensusdataSubscriptionStatus)
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

void LiveCompetitions::postGameCalls()
{
    setClientState(mPlayerInfo, Blaze::ClientState::MODE_SINGLE_PLAYER, Blaze::ClientState::STATUS_NORMAL);  
	lookupUsersByPersonaNames(mPlayerInfo, mPlayerInfo->getPersonaName().c_str());
	getStatGroup(mPlayerInfo, "LiveCompSeasonalPlayMonthly");		
    getKeyScopesMap(mPlayerInfo);	
}

bool LiveCompetitions::executeReportTelemetry()
{
    BlazeRpcError err = ERR_OK;
    if (!mPlayerData->isConnected())
    {
        BLAZE_ERR_LOG(BlazeRpcLog::util, "[LiveCompetitions::ReportTelemetry::User Disconnected = " << mPlayerData->getPlayerId());
        return false;
    }
    if (getGameId() == 0)
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitions::ReportTelemetry][GameId for player = " << mPlayerInfo->getPlayerId() << "] not present");
        return false;
    }
    StressGameInfo* ptrGameInfo = LiveCompetitionsGameDataRef(mPlayerInfo).getGameData(getGameId());
    if (ptrGameInfo == NULL)
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[LiveCompetitions::ReportTelemetry][GameData for game = " << getGameId() << "] not present");
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
    err = mPlayerInfo->getComponentProxy()->mGameManagerProxy->reportTelemetry(reportTelemetryReq);
    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "LiveCompetitions: reportTelemetry failed with err=" << ErrorHelp::getErrorName(err));
    }
    return true;
}

BlazeRpcError LiveCompetitions::startMatchmaking()
{
	BlazeRpcError err = ERR_OK;
    StartMatchmakingScenarioRequest request;
    StartMatchmakingScenarioResponse response;
	MatchmakingCriteriaError criteriaError;

	CommonGameRequestData& commonData = request.getCommonGameData();
	commonData.setGameType(GAME_TYPE_GAMESESSION);
	commonData.setGameProtocolVersionString(getGameProtocolVersionString());
	
	
	//Player Network Address
	NetworkAddress & networkAddress = request.getCommonGameData().getPlayerNetworkAddress();
	networkAddress.switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
	if(!mPlayerData->isConnected())
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "[LiveCompetitions::startMatchmakingScenario::User Disconnected = "<<mPlayerData->getPlayerId());
		return ERR_SYSTEM;
	}
	networkAddress.getIpPairAddress()->getExternalAddress().setIp(mPlayerData->getConnection()->getAddress().getIp());
	networkAddress.getIpPairAddress()->getExternalAddress().setPort(mPlayerData->getConnection()->getAddress().getPort());
	networkAddress.getIpPairAddress()->getExternalAddress().copyInto(networkAddress.getIpPairAddress()->getInternalAddress());
	
	//playerJoinData
	PlayerJoinData& playerJoinData = request.getPlayerJoinData();
	playerJoinData.setGroupId(mPlayerData->getConnGroupId());
    playerJoinData.setDefaultRole("");
    playerJoinData.setGameEntryType(GAME_ENTRY_TYPE_DIRECT);
	
	PerPlayerJoinData* perPlayerJoinData = request.getPlayerJoinData().getPlayerDataList().pull_back();
	perPlayerJoinData->setIsOptionalPlayer(false);
	perPlayerJoinData->setRole("");
	//User Identification
	UserIdentification & userId = perPlayerJoinData->getUser();
	userId.setAccountId(0);
	userId.setAccountLocale(0);
	userId.setExternalId(mPlayerData->getExternalId());
	userId.setBlazeId(mPlayerData->getPlayerId());
	userId.setName("");
	userId.setPersonaNamespace("");
	userId.setOriginPersonaId(0);
	userId.setPidId(0);
 
	playerJoinData.setDefaultSlotType(SLOT_PUBLIC_PARTICIPANT);
    playerJoinData.setDefaultTeamId(ANY_TEAM_ID);
    playerJoinData.setDefaultTeamIndex(ANY_TEAM_ID + 1);
	
	char8_t competitionIdString[32];
    blaze_snzprintf(competitionIdString, sizeof(competitionIdString), "%" PRId32, getLCEventId());
	//Scenario Attributes
	SET_SCENARIO_ATTRIBUTE(request, "COOP_DIVISION", "1");
	SET_SCENARIO_ATTRIBUTE(request, "COOP_DIVISION_THR", "OSDK_matchRelax");
	SET_SCENARIO_ATTRIBUTE(request, "CUSTOM_CONTROLLER", "0");
	SET_SCENARIO_ATTRIBUTE(request, "GAMEGROUP_ONLY", "0");
	SET_SCENARIO_ATTRIBUTE(request, "GAME_MODE", "0");
	SET_SCENARIO_ATTRIBUTE(request, "MATCHUP_HASH", "4356147");
	SET_SCENARIO_ATTRIBUTE(request, "MATCH_CLUB_TYPE", "0");
	SET_SCENARIO_ATTRIBUTE(request, "MATCH_GUESTS", "0");
	SET_SCENARIO_ATTRIBUTE(request, "NET_TOP", (int64_t)130);
	SET_SCENARIO_ATTRIBUTE(request, "SPONSORED_EVENT_ID", competitionIdString);
	SET_SCENARIO_ATTRIBUTE(request, "WOMEN_RULE", "0");

	request.setScenarioName("LiveComp");
	/*if (BLAZE_IS_TRACE_RPC_ENABLED(BlazeRpcLog::gamemanager)) {
	char8_t buff[20480];
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager , "LiveCompetitions::startMatchmakingScenario" << "\n" << request.print(buff, sizeof(buff)));
	}*/
	err = mPlayerInfo->getComponentProxy()->mGameManagerProxy->startMatchmakingScenario(request, response, criteriaError);
	if (err == ERR_OK) 
	{
        mMatchmakingSessionId = response.getScenarioId();
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "LiveCompetitions::startMatchmakingScenario successful. mMatchmakingSessionId= " << mMatchmakingSessionId);
    } 
	else
	{
       BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "LiveCompetitions::startMatchmakingScenario failed. Error(" << ErrorHelp::getErrorName(err) << ")");
	}
	return err;
}

BlazeRpcError LiveCompetitions::playerEventRegistrationflow() {
    BlazeRpcError err = ERR_OK;
	SponsoredEvents::CheckUserRegistrationResponse response;
	checkUserRegistration(mPlayerInfo, geteventId(), response);
	int8_t isRegistered = response.getIsRegistered();
    if (!isRegistered) {
		BLAZE_INFO_LOG(BlazeRpcLog::sponsoredevents, "[LiveCompetitions]: Event not registered. Registering Event");
		FetchConfigResponse fresponse;
		fetchClientConfig(mPlayerInfo, mCompetitionName, fresponse);
		err = registerUser(mPlayerInfo, mEventId);
        if (err == ERR_OK) {
			BLAZE_INFO_LOG(BlazeRpcLog::sponsoredevents, "[LiveCompetitions]: Event registeration successfull");
        }
    }
	checkUserRegistration(mPlayerInfo, mEventId, response);
    return err;
}

int32_t LiveCompetitions::getLCEventId() {
	int32_t eventId = 0;
	int32_t sumprobability = 0;
	int32_t randomprobability = Random::getRandomNumber(100);
	EventIdProbabilityMap eventIdMap = StressConfigInst.getEventIdProbabilityMap();
	EventIdProbabilityMap::iterator it = eventIdMap.begin();
	while(it!=eventIdMap.end())
	{
		sumprobability += it->second;
		if(randomprobability <= sumprobability)
		{
			eventId = it->first;
		}
		it++;
	}
	BLAZE_TRACE_LOG(BlazeRpcLog::sponsoredevents, "[" << mPlayerInfo->getPlayerId() << "][LiveCompetitions]: EVENT ID:" << eventId);
	return eventId;
};

}  // namespace Stress
}  // namespace Blaze
