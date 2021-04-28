//  *************************************************************************************************
//
//   File:    onlineseasons.cpp
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#include "./onlineseasons.h"
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


namespace Blaze {
namespace Stress {

OnlineSeasons::OnlineSeasons(StressPlayerInfo* playerData)
    : GamePlayUser(playerData),
      OnlineSeasonsGameSession(playerData, 2 /*gamesize*/, PEER_TO_PEER_FULL_MESH /*Topology*/,
                             StressConfigInst.getGameProtocolVersionString() /*GameProtocolVersionString*/),
      mIsMarkedForLeaving(false)
{
	mEventId = 0;
}

OnlineSeasons::~OnlineSeasons()
{
    BLAZE_INFO(BlazeRpcLog::util, "[OnlineSeasons][Destructor][%" PRIu64 "]: onlineseasons destroy called", mPlayerInfo->getPlayerId());
    stopReportTelemtery();
    //  deregister async handlers to avoid client crashes when disconnect happens
    deregisterHandlers();
    //  wait for some time to allow already scheduled jobs to complete
    //  sleep(30000);
}

void OnlineSeasons::deregisterHandlers()
{
    OnlineSeasonsGameSession::deregisterHandlers();
}

BlazeRpcError OnlineSeasons::lobbyRPCcalls()
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
			if (iter->second < CLUBS_CONFIG.ClubSizeMax)
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
					iter->second = CLUBS_CONFIG.ClubSizeMax;
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

	if(getMyClubId() > 0)
	{
		GetMembersResponse getMembersResponse;
		getMembers(mPlayerInfo, getMyClubId(), 50, getMembersResponse);
	}

	/*if (!isPlatformHost)
	{
		getPetitions(mPlayerInfo, getMyClubId(), Blaze::Clubs::CLUBS_PETITION_SENT_TO_CLUB);
	}*/
	/*if(getMyClubId() > 0)
	{
		getStatsByGroupAsync(mPlayerInfo,"VProPosition", entityList, "club", getMyClubId(),0 );
		mPlayerInfo->setViewId(++viewId);
	}*/
	
	if (mScenarioType == SEASONSCUP)
	{
		checkUserRegistration(mPlayerInfo, mEventId, checkUserRegistrationResponse);
	//	// updateLoadOutsPeripherals(mPlayerInfo);
	//	blaze_snzprintf(configSection, sizeof(configSection), "FIFA_LIVE_COMP_SEASONALPLAY_%d", mEventId);
	//	fetchClientConfig(mPlayerInfo, configSection, response);
	//	lookupUsersByPersonaNames(mPlayerInfo, mPlayerInfo->getPersonaName().c_str());
		getStatGroup(mPlayerInfo, "LiveCompSeasonalPlayMonthly");
		getStatsByGroupAsync(mPlayerInfo, mPlayerInfo->getPlayerId(), viewId, "LiveCompSeasonalPlayMonthly");
		mPlayerInfo->setViewId(++viewId);
		getEntityRank(mPlayerInfo);
	}

	entityList.clear();
	entityList.push_back(mPlayerInfo->getPlayerId());
	
	// getEntityRank(mPlayerInfo);
	getStatGroup(mPlayerInfo, "VProPosition");
	getStatsByGroupAsync(mPlayerInfo, friendsList, "VProPosition", getMyClubId());
	mPlayerInfo->setViewId(++viewId);

	if (mScenarioType == SEASONSCUP)
	{
		getGameReportViewInfo(mPlayerInfo, "CupSeasonRecentGames");
		getGameReportView(mPlayerInfo, "CupSeasonRecentGames");
	}
	/*else
	{
		getGameReportViewInfo(mPlayerInfo, "NormalRecentGames");
		getGameReportView(mPlayerInfo, "NormalRecentGames");
	}*/
	//if (mScenarioType != SEASONS)
	{
		lookupUsers(mPlayerInfo, "");
	}
	setConnectionState(mPlayerInfo);
	/*Blaze::PersonaNameList personaNameList;
	personaNameList.push_back("2 Dev 52817513");
	personaNameList.push_back("2 Dev 884104052");
	personaNameList.push_back("2 Dev 314878371");
	personaNameList.push_back("2 Dev 179078601");
	personaNameList.push_back("2 Dev 338638415");
	lookupUsersByPersonaNames(mPlayerInfo, personaNameList);*/
	lookupUsersByPersonaNames(mPlayerInfo, "");
	getStatGroup(mPlayerInfo, "H2HPreviousSeasonalPlay");
	getStatsByGroupAsync(mPlayerInfo, "H2HPreviousSeasonalPlay");
	mPlayerInfo->setViewId(++viewId);
	lookupUsersByPersonaNames(mPlayerInfo, "");
	getStatGroup(mPlayerInfo, "H2HSeasonalPlay");
	getStatsByGroupAsync(mPlayerInfo, "H2HSeasonalPlay");
	mPlayerInfo->setViewId(++viewId);
	fetchClientConfig(mPlayerInfo, "FIFA_H2H_SEASONALPLAY", response);

	return err;
}

BlazeRpcError OnlineSeasons::joinClub(ClubId clubID)
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

BlazeRpcError OnlineSeasons::initializeCupAndTournments()
{
	BlazeRpcError err = ERR_OK;
	bool8_t isTournamentActive, joinTournamentFlag;
	//Read cupID
	FifaCups::CupInfo cupsResp;

	err = getCupInfo(mPlayerInfo, FifaCups::MemberType::FIFACUPS_MEMBERTYPE_USER, cupsResp);
	if (ERR_OK == err)
	{
		setCupId(cupsResp.getCupId());
	}
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::initializeCupAndTournments]:" << mPlayerData->getPlayerId() << " cupID= " << getCupId());

	//Read TournamentID
	OSDKTournaments::GetMyTournamentIdResponse tournamentIdResponse;
	OSDKTournaments::GetTournamentsResponse getTournmentsResponse;
	OSDKTournaments::TournamentId tournamentId = 0;
	err = getMyTournamentId(mPlayerInfo, OSDKTournaments::TournamentModes::MODE_FIFA_CUPS_USER, tournamentIdResponse);
	isTournamentActive = tournamentIdResponse.getIsActive();
	if (ERR_OK == err)
	{
		setTournamentId(tournamentIdResponse.getTournamentId());
		isTournamentActive = tournamentIdResponse.getIsActive();
		getTournaments(mPlayerInfo, OSDKTournaments::TournamentModes::MODE_FIFA_CUPS_USER, getTournmentsResponse);
	}
	else if (err == OSDKTOURN_ERR_MEMBER_NOT_IN_TOURNAMENT)
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::initializeCupAndTournments]:" << mPlayerData->getPlayerId() << " getMyTournamentId call 2 faled but calling getTournaments");
		err = getTournaments(mPlayerInfo, OSDKTournaments::TournamentModes::MODE_FIFA_CUPS_USER, getTournmentsResponse);
		if (err == ERR_OK && getTournmentsResponse.getTournamentList().size() > 0)
		{
			// Means that the player is not yet into any tournaments, so get the tournament id first and join into that;
			tournamentId = getTournmentsResponse.getTournamentList().at(0)->getTournamentId();
		}
		setTournamentId(tournamentId);
	}
	if (err == ERR_OK)
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "OnlineSeasons::tournamentRpcCalls:player= " << mPlayerData->getPlayerId() << " TournamentId is set. " << getTournamentId());
	}
	else
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "[OnlineSeasons::tournamentRpcCalls::TournamentId is Not Set for " << mPlayerData->getPlayerId());
	}

	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::initializeCupAndTournments]:" << mPlayerData->getPlayerId() << " tournamentId= " << getTournamentId() << " is Active= " << isTournamentActive);
	//Check if player joined the Tournament
	OSDKTournaments::GetTournamentsResponse tournmentsResponse;
	//getTournaments(mPlayerInfo, OSDKTournaments::TournamentModes::MODE_FIFA_CUPS_USER, tournmentsResponse);
	joinTournamentFlag = false;
	if (getTournamentId() == 0 && tournmentsResponse.getTournamentList().size() > 0)
	{
		// Player is not yet into any tournaments, so get the tournament id first and join into that;
		setTournamentId(tournmentsResponse.getTournamentList().at(0)->getTournamentId());
		joinTournamentFlag = true;
	}

	//If tournment ID not received joining the tournament
	if (getTournamentId() > 0 && joinTournamentFlag) {
		joinTournament(mPlayerInfo, getTournamentId());
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::initializeCupAndTournments::joinTournament]:" << mPlayerData->getPlayerId() << " tournamentId= " << getTournamentId());
	}

	OSDKTournaments::GetMyTournamentDetailsResponse tournamentDetailsResponse;
	if (getTournamentId() > 0)
	{
		getMyTournamentDetails(mPlayerInfo, OSDKTournaments::TournamentModes::MODE_FIFA_CUPS_USER, getTournamentId(), tournamentDetailsResponse);
	}
	getGameReportViewInfo(mPlayerInfo, "NormalRecentGames");
	getGameReportView(mPlayerInfo, "NormalRecentGames");
	// Lookup random users
	if( mScenarioType != SEASONS)
	{
		lookupRandomUsersByPersonaNames(mPlayerInfo);
	}
	if( mScenarioType == SEASONSCUP)
	{
		if( isTournamentActive == false)
		{
			// if we are not in a tournament that means we lost/won the last tournament, reset cup status
			if (getCupId() != 0) {
				resetCupStatus(mPlayerInfo, FifaCups::FIFACUPS_MEMBERTYPE_USER);	//	Need to check this 
			}
			err = getCupInfo(mPlayerInfo, FifaCups::MemberType::FIFACUPS_MEMBERTYPE_USER, cupsResp);
			if(ERR_OK == err)
			{
				setCupId(cupsResp.getCupId());
			}
			getTournaments(mPlayerInfo, OSDKTournaments::TournamentModes::MODE_FIFA_CUPS_USER, tournmentsResponse);
			if (getTournamentId() == 0 && tournmentsResponse.getTournamentList().size() > 0) 
			{  
				// Player is not yet into any tournaments, so get the tournament id first and join into that;
				setTournamentId(tournmentsResponse.getTournamentList().at(0)->getTournamentId());
				if(getTournamentId() != 0)
				{
					joinTournament(mPlayerInfo, getTournamentId());
					sleep(500);
					getMyTournamentDetails(mPlayerInfo, OSDKTournaments::TournamentModes::MODE_FIFA_CUPS_USER, getTournamentId(), tournamentDetailsResponse);
				}
			}

		}
		if(getCupId() == 0)
		{
			//set default active cup ID to 1
			setCupId(1);		
		}
		err = setActiveCupId(mPlayerInfo, getCupId(), FifaCups::MemberType::FIFACUPS_MEMBERTYPE_USER);
		err = getCupInfo(mPlayerInfo, FifaCups::MemberType::FIFACUPS_MEMBERTYPE_USER, cupsResp);
		if(ERR_OK == err)
		{
			setCupId(cupsResp.getCupId());
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::initializeCupAndTournments] GetCupInfo" << mPlayerData->getPlayerId() << " cupID= " << getCupId());
		}	
		getTournaments(mPlayerInfo, OSDKTournaments::TournamentModes::MODE_FIFA_CUPS_USER, tournmentsResponse);
		if (getTournamentId() == 0 && tournmentsResponse.getTournamentList().size() > 0) 
		{  
			// Player is not yet into any tournaments, so get the tournament id first and join into that;
			setTournamentId(tournmentsResponse.getTournamentList().at(0)->getTournamentId());
			if(getTournamentId() != 0)
			{
				err = joinTournament(mPlayerInfo, getTournamentId());
				sleep(500);
				err = getMyTournamentDetails(mPlayerInfo, OSDKTournaments::TournamentModes::MODE_FIFA_CUPS_USER, getTournamentId(), tournamentDetailsResponse);
				lookupUsersByPersonaNames(mPlayerInfo, "");
				getStatsByGroupAsync(mPlayerInfo, "H2HSeasonalPlay");
				//mPlayerInfo->setViewId(++viewId);
				getGameReportViewInfo(mPlayerInfo, "CupSeasonRecentGames");
				getGameReportView(mPlayerInfo, "CupSeasonRecentGames");
				userSettingsSave(mPlayerInfo, "AchievementCache", "AchievementCache");
			}
		}
	}
	
	userSettingsSave(mPlayerInfo, "AchievementCache", "AchievementCache");
	return err;
}

BlazeRpcError OnlineSeasons::seasonLeaderboardCalls()
{
	BlazeRpcError result = ERR_OK;	
	result = getLeaderboardTreeAsync(mPlayerInfo, "H2HSeasonalPlay");
	result = getLeaderboardGroup(mPlayerInfo, -1, "1v1_seasonalplay_overall_fd");
	result = getCenteredLeaderboard(mPlayerInfo, 0, 1, mPlayerInfo->getPlayerId(), "1v1_seasonalplay_overall_fd");
	result = getLeaderboard(mPlayerInfo, 0, 100, "1v1_seasonalplay_overall_fd");
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

BlazeRpcError OnlineSeasons::simulateLoad()
{
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::simulateLoad]:" << mPlayerData->getPlayerId());
	BlazeRpcError err = ERR_OK;
	
	err = simulateMenuFlow();

	 if ((int)Random::getRandomNumber(100) < 100)
	 { 
		 seasonLeaderboardCalls();
	 }

	seteventId(getOSEventId());
	//Seasons Lobby
	lobbyRPCcalls();
	// Cup and Tournament initialization
	initializeCupAndTournments();

	//Choose one of the scenario type
	mScenarioType = StressConfigInst.getRandomScenarioByPlayerType(ONLINESEASONS);
	switch (mScenarioType)
	{
	case SEASONS:
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type " << mScenarioType);
	}
	break;
	case SEASONSCUP:
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type " << mScenarioType);
	}
	break;
	default:
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " In default state");
		mScenarioType = SEASONS;
	}
	break;
	}

	// PlayerTimeStats RPC call
	if (StressConfigInst.getPTSEnabled())
	{
		// Update Eadp Stats(Read and Update Limits)
		if ((uint32_t)Random::getRandomNumber(100) < StressConfigInst.getEadpStatsProbability())
		{
			updateEadpStats(mPlayerInfo);
		}
	}

	//Send MM request 
	err = startMatchmakingScenario(mScenarioType);

	if (err != ERR_OK)
	{
		unsubscribeFromCensusData(mPlayerInfo);
		sleep(30000);
		return err;
	}

	//CancelMatchmaking
	if (Random::getRandomNumber(100) < (int)OS_CONFIG.cancelMatchMakingProbability)
	{
		err = cancelMatchMakingScenario(mMatchmakingSessionId);
		if (err == ERR_OK)
		{
			unsubscribeFromCensusData(mPlayerInfo);
			return err;
		}
	}

	int64_t playerStateTimer = 0;
	int64_t  playerStateDurationMs = 0;
	GameState   playerStateTracker = getPlayerState();
	uint16_t stateLimit = 0;
	bool ingame_enter = false;
	while (1)
	{
		if (!mPlayerData->isConnected())
		{
			BLAZE_ERR_LOG(BlazeRpcLog::util, "[OnlineSeasons::simulateLoad::User Disconnected = " << mPlayerData->getPlayerId());
			return ERR_DISCONNECTED;
		}
		if (playerStateTracker != getPlayerState())  //  state changed
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::simulateLoad:playerStateTracker]" << mPlayerData->getPlayerId() << " Time Spent in Sate: " << GameStateToString(
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
				if (playerStateDurationMs > OS_CONFIG.maxPlayerStateDurationMs)
				{
					BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::simulateLoad:playerStateTracker][" << mPlayerData->getPlayerId() << "]: Player exceded maximum Game state Duration:" << playerStateDurationMs <<
						" ms" << " Current Game State:" << GameStateToString(getPlayerState()));
					if (IN_GAME == getPlayerState())
					{
						mIsMarkedForLeaving = true;   //  Player exceed maximum duration in InGAME State so mark this Player to Leave from InGame
					}
				}
			}
		}
		const char8_t * stateMsg = GameStateToString(getPlayerState());
		BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::simulateLoad]: Player: " << mPlayerInfo->getPlayerId() << ", GameId: " << myGameId << " Current state : " << stateMsg << " InStateDuration:" <<
			playerStateDurationMs);
		switch (getPlayerState())
		{
			//   Sleep for all the below states
		case NEW_STATE:
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=NEW_STATE");
			stateLimit++;
			if (stateLimit >= OS_CONFIG.GameStateIterationsMax)
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=NEW_STATE for long time and return");
				unsubscribeFromCensusData(mPlayerInfo);
				return err;
			}
			//  Wait here if resetDedicatedserver() called/player not found for invite.
			sleep(OS_CONFIG.GameStateSleepInterval);
			break;
		}
		case PRE_GAME:
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=PRE_GAME");
			setClientState(mPlayerInfo, ClientState::MODE_MULTI_PLAYER, ClientState::Status::STATUS_NORMAL);
			setPlayerAttributes("REQ", "2");

			if (isPlatformHost)
			{
				setGameSettingsMask(mPlayerInfo, myGameId, 0, 2063);
				advanceGameState(IN_GAME);
			}
			sleep(OS_CONFIG.GameStateSleepInterval);
			break;
		}
		case IN_GAME:
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=IN_GAME");
			setClientState(mPlayerInfo, ClientState::MODE_SINGLE_PLAYER);

			if (!ingame_enter)
			{
				ingame_enter = true;
				simulateInGame();
				//Start InGame Timer	
				if (isPlatformHost)
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " setInGameStartTime. gameID=" << myGameId);
					setInGameStartTime();
				}
			}

			if (RollDice(OS_CONFIG.inGameReportTelemetryProbability))
			{
				executeReportTelemetry();
			}

			if ((isPlatformHost && IsInGameTimedOut()) || (mIsMarkedForLeaving == true))
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " IsInGameTimedOut/mIsMarkedForLeaving=true. gameID=" << myGameId);
				advanceGameState(POST_GAME);
			}

			sleep(OS_CONFIG.GameStateSleepInterval);
			break;
		}
		case INITIALIZING:
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=INITIALIZING");
			setClientState(mPlayerInfo, ClientState::MODE_MULTI_PLAYER);
			if (isPlatformHost)
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::simulateLoad][" << mPlayerData->getPlayerId() << "calling advanceGameState");
				advanceGameState(PRE_GAME);
			}
			sleep(OS_CONFIG.GameStateSleepInterval);
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
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=POST_GAME");

			//Submitgamereport is initiated from NOTIFY_NOTIFYGAMESTATECHANGE async notification

			//Leave game
			//leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, myGameId);
			BLAZE_INFO(BlazeRpcLog::gamemanager, "[OnlineSeasons::leaveGame][%" PRIu64 "]: player left game: GameId = %" PRIu64 "", mPlayerData->getPlayerId(), myGameId);
			updatePrimaryExternalSessionForUserRequest(mPlayerInfo, myGameId, myGameId, UPDATE_PRESENCE_REASON_GAME_LEAVE, GAME_TYPE_GAMESESSION, ACTIVE_CONNECTED, PRESENCE_MODE_STANDARD, 0, false);
			//meshEndpointsDisconnected RPC is initiated from NOTIFY_NOTIFYPLAYERREMOVED
			if (!isPlatformHost)
			{
				postGameCalls();
			}
			delayRandomTime(5000, 10000);
			setPlayerState(DESTRUCTING);

			sleep(OS_CONFIG.GameStateSleepInterval);
			break;
		}
		case RESETABLE:
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=RESETABLE");
			//  mGameData will be freed when NOTIFY_NOTIFYGAMEREMOVED notification comes.
			sleep(OS_CONFIG.GameStateSleepInterval);
			break;
		}
		case DESTRUCTING:
		{
			//  Player Left Game
			//  Presently using this state when user is removed from the game and return from this function.
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=DESTRUCTING");

			setClientState(mPlayerInfo, ClientState::MODE_MENU);
			//  Return here
			return err;
		}
		case MIGRATING:
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=MIGRATING");
			//  mGameData will be freed when NOTIFY_NOTIFYGAMEREMOVED notification comes.
			sleep(10000);
			break;
		}
		case GAME_GROUP_INITIALIZED:
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=GAME_GROUP_INITIALIZED");
			sleep(5000);
			//if (playerStateDurationMs > OS_CONFIG.MaxGameGroupInitializedStateDuration)
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
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::simulateLoad]" << mPlayerData->getPlayerId() << " with state=" << getPlayerState());
			//  shouldn't come here
			return err;
		}
		}
	}
}

void OnlineSeasons::addPlayerToList()
{
    //  add player to universal list first
    Player::addPlayerToList();
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::addPlayerToList]: Player: " << mPlayerInfo->getPlayerId() << ", Game: " << myGameId);
    {
        ((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->onlineSeasonsMap.insert(eastl::pair<PlayerId, eastl::string>(mPlayerInfo->getPlayerId(), mPlayerInfo->getPersonaName()));
    }
}

void OnlineSeasons::removePlayerFromList()
{
    //  remove player from universal list first
    Player::removePlayerFromList();
    PlayerId  id = mPlayerInfo->getPlayerId();
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::removePlayerFromList]: Player: " << id << ", Game: " << myGameId);
    PlayerMap& playerMap = ((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->onlineSeasonsMap;
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

void OnlineSeasons::simulateInGame()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][OnlineSeasons]: In Game simulation:" << myGameId);
    
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

void OnlineSeasons::postGameCalls()
{
    setClientState(mPlayerInfo, Blaze::ClientState::MODE_SINGLE_PLAYER, Blaze::ClientState::STATUS_NORMAL);  
	lookupUsersByPersonaNames(mPlayerInfo, ""); // mPlayerInfo->getPersonaName().c_str()	As per logs
	getStatGroup(mPlayerInfo, "H2HSeasonalPlay");		
    getKeyScopesMap(mPlayerInfo);	
	getStatsByGroupAsync(mPlayerInfo, "H2HSeasonalPlay");
	//	Added fetchClientConfig RPC as per logs
	FetchConfigResponse response;
	fetchClientConfig(mPlayerInfo, "FIFA_H2H_SEASONALPLAY", response);
	//subscribeToCensusDataUpdates(mPlayerInfo);	Commented as per logs
}


bool OnlineSeasons::executeReportTelemetry()
{
    BlazeRpcError err = ERR_OK;
    if (!mPlayerData->isConnected())
    {
        BLAZE_ERR_LOG(BlazeRpcLog::util, "[OnlineSeasons::ReportTelemetry::User Disconnected = " << mPlayerData->getPlayerId());
        return false;
    }
    if (getGameId() == 0)
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::ReportTelemetry][GameId for player = " << mPlayerInfo->getPlayerId() << "] not present");
        return false;
    }
    StressGameInfo* ptrGameInf = OnlineSeasonsGameDataRef(mPlayerInfo).getGameData(getGameId());
    if (ptrGameInf == NULL)
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::ReportTelemetry][GameData for game = " << getGameId() << "] not present");
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
    //telemetryReport->setRemoteConnGroupId(ptrGameInfo->getTopologyHostConnectionGroupId());
	telemetryReport->setRemoteConnGroupId(getOpponentPlayerInfo().getConnGroupId().id);
    err = mPlayerInfo->getComponentProxy()->mGameManagerProxy->reportTelemetry(reportTelemetryReq);
    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "OnlineSeasons: reportTelemetry failed with err=" << ErrorHelp::getErrorName(err));
    }
    return true;
}

BlazeRpcError OnlineSeasons::startMatchmakingScenario(ScenarioType scenarioName)
{
	BlazeRpcError err = ERR_SYSTEM;
    StartMatchmakingScenarioRequest request;
	StartMatchmakingScenarioResponse response;
    MatchmakingCriteriaError error;
	request.getCommonGameData().setGameProtocolVersionString(StressConfigInst.getGameProtocolVersionString());
	request.getCommonGameData().setGameType(GAME_TYPE_GAMESESSION);
	NetworkAddress& hostAddress = request.getCommonGameData().getPlayerNetworkAddress();
	//if (PLATFORM_XONE==StressConfigInst.getPlatform())
	//{
		//hostAddress.switchActiveMember(NetworkAddress::MEMBER_XBOXCLIENTADDRESS);
		//if (mPlayerData->getExternalId())
			//hostAddress.getXboxClientAddress()->setXuid(mPlayerData->getExternalId());
	//}
	//else
	//{
		hostAddress.switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
		if (!mPlayerData->isConnected())
		{
			BLAZE_ERR_LOG(BlazeRpcLog::util, "[OnlineSeasons::startMatchMaking::User Disconnected = " << mPlayerData->getPlayerId());
			return ERR_SYSTEM;
		}
		hostAddress.getIpPairAddress()->getExternalAddress().setIp(mPlayerData->getConnection()->getAddress().getIp());
		hostAddress.getIpPairAddress()->getExternalAddress().setPort(mPlayerData->getConnection()->getAddress().getPort());
		hostAddress.getIpPairAddress()->getExternalAddress().copyInto(hostAddress.getIpPairAddress()->getInternalAddress());
//	}
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
		playerJoinDataPtr->setIsOptionalPlayer(false);
		playerJoinDataPtr->setRole("");
		playerJoinDataPtr->setSlotType(INVALID_SLOT_TYPE);
		playerJoinDataPtr->setTeamId(ANY_TEAM_ID+1);
		playerJoinDataPtr->setTeamIndex(ANY_TEAM_ID + 1);
		
		//User Identification
		playerJoinDataPtr->getUser().setAccountId(0);
		playerJoinDataPtr->getUser().setAccountLocale(0);
		playerJoinDataPtr->getUser().setPersonaNamespace("");
		playerJoinDataPtr->getUser().getPlatformInfo().getExternalIds().setSwitchId("");
		playerJoinDataPtr->getUser().getPlatformInfo().getExternalIds().setXblAccountId(mPlayerData->getExternalId());
		playerJoinDataPtr->getUser().setPidId(0);
		playerJoinDataPtr->getUser().setOriginPersonaId(0);
		playerJoinDataPtr->getUser().setExternalId(mPlayerData->getExternalId());
		playerJoinDataPtr->getUser().setBlazeId(mPlayerData->getPlayerId());
		playerJoinDataPtr->getUser().setName(mPlayerData->getPersonaName().c_str());
	}
	if (mScenarioType == SEASONS)
	{
		SET_SCENARIO_ATTRIBUTE(request, "AVOID_TEAM", "18");
		SET_SCENARIO_ATTRIBUTE(request, "AVOID_TEAM_THR", "OSDK_matchExact");
		SET_SCENARIO_ATTRIBUTE(request, "CUSTOM_CONTROLLER", "0");
		SET_SCENARIO_ATTRIBUTE(request, "DIVISION", "1");
		SET_SCENARIO_ATTRIBUTE(request, "DIVISION_THR", "OSDK_matchRelax");
		SET_SCENARIO_ATTRIBUTE(request, "GAMEGROUP_ONLY", "0");
		SET_SCENARIO_ATTRIBUTE(request, "GAME_MODE", "0");
		SET_SCENARIO_ATTRIBUTE(request, "MATCHUP_HASH", "4340580");
		SET_SCENARIO_ATTRIBUTE(request, "MATCH_CLUB_TYPE", "0");
		SET_SCENARIO_ATTRIBUTE(request, "MATCH_GUESTS", "0");
		SET_SCENARIO_ATTRIBUTE(request, "NET_TOP", (uint64_t)130);
		SET_SCENARIO_ATTRIBUTE(request, "NEW_USER_RULE", "1");
		SET_SCENARIO_ATTRIBUTE(request, "NEW_USER_RULE_THR", "OSDK_matchRelax");
		SET_SCENARIO_ATTRIBUTE(request, "TEAM_OVR", "83");
		SET_SCENARIO_ATTRIBUTE(request, "TEAM_OVR_THR", "OSDK_matchRelax");
		SET_SCENARIO_ATTRIBUTE(request, "WOMEN_RULE", "0");
		request.setScenarioName("Seasons");
	}
	else if (mScenarioType == SEASONSCUP)
	{
		SET_SCENARIO_ATTRIBUTE(request, "AVOID_TEAM", "18");
		SET_SCENARIO_ATTRIBUTE(request, "AVOID_TEAM_THR", "OSDK_matchExact");
		SET_SCENARIO_ATTRIBUTE(request, "CUP", "1");
		SET_SCENARIO_ATTRIBUTE(request, "CUSTOM_CONTROLLER", "0");
		SET_SCENARIO_ATTRIBUTE(request, "GAMEGROUP_ONLY", "0");
		SET_SCENARIO_ATTRIBUTE(request, "GAME_MODE", "1");
		SET_SCENARIO_ATTRIBUTE(request, "MATCHUP_HASH", "4340580");
		SET_SCENARIO_ATTRIBUTE(request, "MATCH_CLUB_TYPE", "0");
		SET_SCENARIO_ATTRIBUTE(request, "MATCH_GUESTS", "0");
		SET_SCENARIO_ATTRIBUTE(request, "NET_TOP", (uint64_t)130);
		SET_SCENARIO_ATTRIBUTE(request, "NEW_USER_RULE", "1"); // for PS4
		SET_SCENARIO_ATTRIBUTE(request, "NEW_USER_RULE_THR", "OSDK_matchRelax");
		SET_SCENARIO_ATTRIBUTE(request, "TEAM_OVR", "83");
		SET_SCENARIO_ATTRIBUTE(request, "TEAM_OVR_THR", "OSDK_matchRelax");
		SET_SCENARIO_ATTRIBUTE(request, "TOUR_TIER", "1");
		SET_SCENARIO_ATTRIBUTE(request, "TOUR_TIER_THR", "");
		SET_SCENARIO_ATTRIBUTE(request, "WOMEN_RULE", "0");
		request.setScenarioName("Seasons_Cup");
	}

	/*if (BLAZE_IS_TRACE_RPC_ENABLED(BlazeRpcLog::gamemanager)) {
		char8_t buf[20240];
		BLAZE_TRACE_RPC_LOG(BlazeRpcLog::gamemanager, "StartMatchmaking RPC : \n" << request.print(buf, sizeof(buf)));
	}*/
	err = mPlayerInfo->getComponentProxy()->mGameManagerProxy->startMatchmakingScenario(request,response,error);
	if (err == ERR_OK) 
	{
        mMatchmakingSessionId = response.getScenarioId();
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "OnlineSeasons::startMatchmakingScenario successful. mMatchmakingSessionId= " << mMatchmakingSessionId);
    } 
	else
	{
       BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "OnlineSeasons::startMatchmakingScenario failed. Error(" << ErrorHelp::getErrorName(err) << ")");
	}
    return err;
	
}
int32_t OnlineSeasons::getOSEventId() {
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
	BLAZE_TRACE_LOG(BlazeRpcLog::sponsoredevents, "[" << mPlayerInfo->getPlayerId() << "][OnlineSeasons]: EVENT ID:" << eventId);
	return eventId;
}
}  // namespace Stress
}  // namespace Blaze
