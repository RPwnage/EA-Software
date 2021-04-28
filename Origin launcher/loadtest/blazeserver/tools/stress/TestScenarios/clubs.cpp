//  *************************************************************************************************
//
//   File:    clubs.cpp
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#include "clubs.h"
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
#include "gamemanager/tdf/gamemanager.h"

namespace Blaze {
namespace Stress {

//constants
//#define					SEASON_ID   1     //osdk season
//#define					SPORTS_SEASON_ID   2    //sports season
const eastl::string			CLUBNAME = "CLB_%" PRId64;

ClubsPlayer::ClubsPlayer(StressPlayerInfo* playerData)
	: GamePlayUser(playerData),
	ClubsPlayerGameSession(playerData, 22 /*gamesize*/, CLIENT_SERVER_DEDICATED /*Topology*/,
 			StressConfigInst.getGameProtocolVersionString() /*GameProtocolVersionString*/),
	mIsMarkedForLeaving(false),
	mTargetTeamSize(2)
{
	postNewsCount=0;

}

ClubsPlayer::~ClubsPlayer()
{
	BLAZE_INFO(BlazeRpcLog::util, "[ClubsPlayer][Destructor][%" PRIu64 "]: clubs destroy called", mPlayerInfo->getPlayerId());
	stopReportTelemtery();
	//  deregister async handlers to avoid client crashes when disconnect happens
	deregisterHandlers();
	//  wait for some time to allow already scheduled jobs to complete
	//sleep(30000);
}

void ClubsPlayer::deregisterHandlers()
{
	ClubsPlayerGameSession::deregisterHandlers();
}


BlazeRpcError ClubsPlayer::simulateLoad()
{
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]:" << mPlayerData->getPlayerId());
	BlazeRpcError err = ERR_OK;

	//Choose one of the scenario type
	mScenarioType = StressConfigInst.getRandomScenarioByPlayerType(CLUBSPLAYER);
	switch (mScenarioType)
	{
	case CLUBSCUP:
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type " << CLUBSCUP);
	}
	break;
	case CLUBMATCH:
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type " << CLUBMATCH);
	}
	break;

	case CLUBFRIENDLIES:
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type " << CLUBFRIENDLIES);
	}
	break;
	case CLUBSPRACTICE:
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type " << CLUBSPRACTICE);
	}
	break;
	default:
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type=" << "Default");
		//  shouldn't come here
		return err;
	}
	}
	err = simulateMenuFlow();

	if (StressConfigInst.isRPCTuningEnabled())
	{
		if (!mCensusdataSubscriptionStatus)
		{
			BlazeRpcError censusDataErr;
			censusDataErr = subscribeToCensusDataUpdates(mPlayerInfo);
			if (censusDataErr == ERR_OK)
			{
				SetSubscriptionSatatus();
			}
		}
		else {
			BlazeRpcError censusDataErr;
			censusDataErr = unsubscribeFromCensusData(mPlayerInfo);
			if (censusDataErr == ERR_OK)
			{
				UnsetSubscriptionSatatus();
			}
		}

		lookupUsersByPersonaNames(mPlayerInfo, "");
		lookupUsersByPersonaNames(mPlayerInfo, "");
		lookupUsersByPersonaNames(mPlayerInfo, "");
	}

	 if ((int)Random::getRandomNumber(100) < 100) 
	 {
		 clubsLeaderboards();
	 }

	getClubs(mPlayerInfo, getMyClubId());
	if (StressConfigInst.getProfanityAvatarProbability() > (uint32_t)Random::getRandomNumber(100))
	{
		getAvatarData(mPlayerInfo);
	}
	getInvitations(mPlayerInfo, getMyClubId(), CLUBS_SENT_TO_ME);
	//createOrJoinClubs
	//if (mScenarioType != CLUBSPRACTICE)//&&mScenarioType != CLUBMATCH)
	//{
		if (getMyClubId() == 0)
		{
			err = createOrJoinClubs();
		}
		if (getMyClubId() == 0)
		{
			BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "ClubsPlayer::simulateLoad:Creation/Joining Clubs process failed for " << mPlayerInfo->getPlayerId());
			delayRandomTime(60000, 120000);
			return err;
		}
		else
		{
			BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "ClubsPlayer::simulateLoad:ClubId creation/joining is successful for playerID= " << mPlayerInfo->getPlayerId() << " clubID= " << getMyClubId());
		}
	//}
	lobbyRpcCalls();
	getGameListSubscription();

	if (StressConfigInst.isRPCTuningEnabled())
	{
		if (!mCensusdataSubscriptionStatus)
		{
			BlazeRpcError censusDataErr;
			censusDataErr = subscribeToCensusDataUpdates(mPlayerInfo);
			if (censusDataErr == ERR_OK)
			{
				SetSubscriptionSatatus();
			}
		}
		else {
			BlazeRpcError censusDataErr;
			censusDataErr = unsubscribeFromCensusData(mPlayerInfo);
			if (censusDataErr == ERR_OK)
			{
				UnsetSubscriptionSatatus();
			}
		}
	}

	if (isPlatformHost && mScenarioType == CLUBSPRACTICE)
	{
		err = tournamentRpcCalls();
		if (err != ERR_OK)
		{
			BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "ClubsPlayer::simulateLoad: tournamentRpcCalls failed for " << mPlayerInfo->getPlayerId());
			//unsubscribeFromCensusData(mPlayerData);
			sleep(6000);
			//return err;
		}
	}

	if (StressConfigInst.isRPCTuningEnabled())
	{
		lookupUsersByPersonaNames(mPlayerInfo, "");
		lookupUsersByPersonaNames(mPlayerInfo, "");
		lookupUsersByPersonaNames(mPlayerInfo, "");
	}

	//Wait for club game to finish
	if (isClubGameInProgress())
	{
		uint16_t loopCount = 0;
		while (1)
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]: " << mPlayerData->getPlayerId() << "  waiting for isClubGameInProgress to be false " << getMyClubId());
			if (!isClubGameInProgress())
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]: " << mPlayerData->getPlayerId() << "  break here as isClubGameInProgress is false " << getMyClubId());
				break;
			}
			if (loopCount >= 120) //wait for max 30min
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]: " << mPlayerData->getPlayerId() << "  max limit reached, not good. " << getMyClubId());
				setClubGameStatus(false);
				break;
			}
			//sleep 
			sleep(60000);
			++loopCount;
		}
	}

	// Home Game Group
	mHomeGameName[MAX_GAMENAME_LEN - 1] = '\0';
	blaze_snzprintf(mHomeGameName, sizeof(mHomeGameName), "club_%" PRId64 "_HOME", getMyClubId());
	BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "ClubsPlayer::simulateLoad:HomeGameName " << mHomeGameName << " for " << mPlayerInfo->getPlayerId());
	
	// VOIP Game Group
	mVoipGameName[MAX_GAMENAME_LEN - 1] = '\0';
	blaze_snzprintf(mVoipGameName, sizeof(mVoipGameName), "club_%" PRId64 "_VOIP", getMyClubId());
	BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "ClubsPlayer::simulateLoad:VoipGameName " << mVoipGameName << " for " << mPlayerInfo->getPlayerId());
	
	// Game Group
	mGameGroupName[MAX_GAMENAME_LEN - 1] = '\0';
	blaze_snzprintf(mGameGroupName, sizeof(mGameGroupName), "club_%" PRId64 "_GAME", getMyClubId());
	BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "ClubsPlayer::simulateLoad:GameGroupName " << mGameGroupName << " for " << mPlayerInfo->getPlayerId());

	err = createOrJoinGame(GameGroupType::HOMEGROUP);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]: createOrJoinGame failed - Home Group " << mPlayerData->getPlayerId());
		unsubscribeFromCensusData(mPlayerData);
		sleep(60000);
		return err;
	}

	err = createOrJoinGame(GameGroupType::VOIPGROUP);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]: createOrJoinGame failed - Voip Group " << mPlayerData->getPlayerId());
		unsubscribeFromCensusData(mPlayerData);
		//leave home group
		leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mHomeGroupGId);
		sleep(60000);
		return err;
	}

	err = createOrJoinGame(GameGroupType::GAMEGROUP);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]: createOrJoinGame failed - Game Group " << mPlayerData->getPlayerId());
		unsubscribeFromCensusData(mPlayerData);
		//leave voip group
		leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mVoipGroupGId);
		//leave home group
		leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mHomeGroupGId);
		sleep(60000);
		return err;
	}
	//wait here to get notifygamesetup
	sleep(5000);

	// PlayerTimeStats RPC call
	if (StressConfigInst.getPTSEnabled())
	{
		// Update Eadp Stats(Read and Update Limits)
		if ((uint32_t)Random::getRandomNumber(100) < StressConfigInst.getEadpStatsProbability())
		{
			updateEadpStats(mPlayerInfo);
		}
	}
	//Other than mIsGameGroupOwner will wait in NEW_STATE

	if (mIsGameGroupOwner)
	{
		//if Target size not reached leave here
		if (!isTargetTeamSizeReached())
		{
			err = ERR_SYSTEM;
			//leave game group
			leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGamegroupGId);
			//leave voip group
			leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mVoipGroupGId);
			//leave home group
			leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mHomeGroupGId);
			BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]: isTargetTeamSizeReached failed, failed to reach Min Team Size" << mPlayerData->getPlayerId());
			sleep(60000);
			return err;
		}
		//target size reached
		if (mScenarioType != CLUBFRIENDLIES && mScenarioType != CLUBSPRACTICE)
		{
			//Send MM request 
			err = startMatchmakingScenario(mScenarioType);
			//if( err == ERR_OK)
			//{
			//	//CancelMatchmaking
			//	if (Random::getRandomNumber(100) < (int)CLUBS_CONFIG.cancelMatchMakingProbability)
			//	{		
			//		err = cancelMatchMakingScenario(mMatchmakingSessionId);
			//		if(err == ERR_OK)
			//		{			
			//			//return here if cancelMatchMaking successful
			//			err = ERR_SYSTEM;
			//		}
			//	}
			//}

			if (err != ERR_OK)
			{
				//leave game group
				leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGamegroupGId);
				//leave voip group
				leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mVoipGroupGId);
				//leave home group
				leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mHomeGroupGId);
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]: startMatchmaking failed " << mPlayerData->getPlayerId());
				sleep(60000);
				return err;
			}
			else
			{
				setClubGameStatus(true);
			}
		}
		//CLUBFRIENDLIES
		else
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]CLUBFRIENDLIES" << mPlayerData->getPlayerId());
			GameId gameID = addClubFriendliesCreatedGame();
			if (gameID == 0)
			{
				err = resetDedicatedServer();
				if (err == ERR_OK)
				{
					setClubGameStatus(true);
				}
				else
				{
					//leave game group
					leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGamegroupGId);
					//leave voip group
					leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mVoipGroupGId);
					//leave home group
					leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mHomeGroupGId);
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]: startMatchmaking failed" << mPlayerData->getPlayerId());
					sleep(60000);
					return err;
				}
			}
			else
			{
				err = joinGame(gameID);
				//	Check if below should be called here
				FilteredLeaderboardStatsRequest::EntityIdList filterBoardEntityList;
				filterBoardEntityList.push_back(355);
				getFilteredLeaderboard(mPlayerInfo, filterBoardEntityList, "ClubRankLeague1LB", 4294967295, 1);
				Blaze::Clubs::GetMembersResponse clubResponse;
				getMembers(mPlayerInfo, getMyClubId(), 50, clubResponse);
				actionGetAwards(mPlayerInfo, getMyClubId());
				if (err == ERR_OK)
				{
					setClubGameStatus(true);
				}
				else
				{
					//leave game group
					leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGamegroupGId);
					//leave voip group
					leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mVoipGroupGId);
					//leave home group
					leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mHomeGroupGId);
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]: startMatchmaking failed" << mPlayerData->getPlayerId());
					sleep(60000);
					return err;
				}
			}

			// Check if gameID avialable in the gloabl map
			// if available join game --> success --> setClubGameStatus --> true
			// if not available creategame(resetDedicatedServer) --> success --> setClubGameStatus --> true

			//add Global map code in modules.cpp file similar to Online Friendlies.
			// whoever creates game will add the gameID to map
			// whoever joins game will remove the gameID from map

		}
	}

	int gameAttributecount = 0;

	int64_t playerStateTimer = 0;
	int64_t  playerStateDurationMs = 0;
	GameState   playerStateTracker = getPlayerState();
	uint16_t stateLimit = 0;
	bool ingame_enter = false;
	while (1)
	{
		if (!mPlayerData->isConnected())
		{
			BLAZE_ERR_LOG(BlazeRpcLog::util, "[ClubsPlayer::simulateLoad::User Disconnected = " << mPlayerData->getPlayerId());
			return ERR_DISCONNECTED;
		}
		if (playerStateTracker != getPlayerState())  //  state changed
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad:playerStateTracker]" << mPlayerData->getPlayerId() << " Time Spent in Sate: " << GameStateToString(
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
				if (playerStateDurationMs > CLUBS_CONFIG.maxPlayerStateDurationMs)
				{
					BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad:playerStateTracker][" << mPlayerData->getPlayerId() << "]: Player exceded maximum Game state Duration:" << playerStateDurationMs <<
						" ms" << " Current Game State:" << GameStateToString(getPlayerState()));
					if (IN_GAME == getPlayerState())
					{
						mIsMarkedForLeaving = true;   //  Player exceed maximum duration in InGAME State so mark this Player to Leave from InGame
					}
				}
			}
		}
		const char8_t * stateMsg = GameStateToString(getPlayerState());
		BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]: Player: " << mPlayerInfo->getPlayerId() << ", GameId: " << myGameId << " Current state : " << stateMsg << " InStateDuration:" <<
			playerStateDurationMs);
		switch (getPlayerState())
		{
			//   Sleep for all the below states
		case NEW_STATE:
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=NEW_STATE");
			stateLimit++;
			if (stateLimit >= CLUBS_CONFIG.GameStateIterationsMax)
			{
				//Not for CLUBSDROPIN - TODO
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=NEW_STATE for long time and return");
				//leave game group
				leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGamegroupGId);
				//leave voip group
				leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mVoipGroupGId);
				//leave home group
				leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mHomeGroupGId);
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]: NEW_STATE timedout " << mPlayerData->getPlayerId());
				if (mIsGameGroupOwner)
				{
					setClubGameStatus(false);
				}
				unsubscribeFromCensusData(mPlayerData);
				sleep(15000);
				return err;
			}
			//  Wait here if resetDedicatedserver() called/player not found for invite.
			sleep(CLUBS_CONFIG.GameStateSleepInterval);
			break;
		}
		case PRE_GAME:
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=PRE_GAME");

			//leave voip group
			//TODO - add again
			//leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mVoipGroupGId);
			setClientState(mPlayerInfo, ClientState::MODE_MULTI_PLAYER, ClientState::STATUS_NORMAL);
			if (!isPlatformHost)
			{
				Blaze::BlazeIdList friendList;
				uint32_t viewId = mPlayerInfo->getViewId();
				friendList.push_back(mPlayerInfo->getPlayerId());
				friendList.push_back();    // Need to check while executing UT
				setPlayerAttributes("REQ", "1", myGameId);
				setPlayerAttributes("REQ", "2", myGameId);
				fetchLoadOuts(mPlayerInfo);
				//getStatGroup(mPlayerInfo, "VProAttributeXP");
				mPlayerInfo->setViewId(++viewId);	//	9
				if (mScenarioType == CLUBSPRACTICE)
				{
					getStatsByGroupAsync(mPlayerInfo, friendList, "VProAttributeXP");
					setPlayerAttributes("OV0", "undefined~undefined~0~-1~-1~-1~0~144", myGameId);
					setPlayerAttributes("OV1", "undefined~undefined~0~-1~-1~-1~0~144", myGameId);
				}
				else
				{
					if (mScenarioType == CLUBFRIENDLIES)
					{
						getStatsByGroupAsync(mPlayerInfo, friendList, "VProAttributeXP");
					}
					else
					{
						getStatsByGroupAsync(mPlayerInfo, friendList, "VProPosition", getMyClubId());
					}
				}
				if (mScenarioType == CLUBFRIENDLIES || mScenarioType == CLUBSCUP)
				{
					setPlayerAttributes("OV0", "2359296~2359296~0~-1~-1~-1~0~144", myGameId);
					setPlayerAttributes("OV1", "29622272~29622272~0~-1~-1~-1~0~1808", myGameId);
				}
				if (mScenarioType == CLUBMATCH)
				{
					setPlayerAttributes("OV0", "29622272~29622272~0~-1~-1~-1~0~1808", myGameId);
					setPlayerAttributes("OV1", "2359296~2359296~0~-1~-1~-1~0~144", myGameId);
					setPlayerAttributes("OV1", "2359296~2359296~0~-1~-1~-1~0~144", myGameId);
				}
			}
			else
			{
				Blaze::BlazeIdList friendList;
				uint32_t viewId = mPlayerInfo->getViewId();
				friendList.push_back(mPlayerInfo->getPlayerId());
				friendList.push_back();
				if (mScenarioType == CLUBSPRACTICE)
				{
					setGameSettingsMask(mPlayerInfo, myGameId, 0, 15);
					fetchLoadOuts(mPlayerInfo);
					mPlayerInfo->setViewId(++viewId);
					getStatsByGroupAsync(mPlayerInfo, friendList, "VProAttributeXP");
					setPlayerAttributes("OV0", "2359296~2359296~0~-1~-1~-1~0~144", myGameId);
					setPlayerAttributes("OV1", "2359297~2359297~0~-1~-1~-1~0~144", myGameId);
				}
				else
				{
					Collections::AttributeMap attributes;
					if (mScenarioType == CLUBFRIENDLIES || mScenarioType == CLUBSCUP)
					{
						setPlayerAttributes("REQ", "1", myGameId);
						setPlayerAttributes("REQ", "2", myGameId);
					}
					attributes.clear();
					attributes["OT"] = "144";
					setGameAttributes(myGameId, attributes);
					attributes.clear();
					attributes["OCA"] = "FUL";
					setGameAttributes(myGameId, attributes);
					attributes.clear();
					attributes["OTA"] = "144";
					setGameAttributes(myGameId, attributes);
					if (mScenarioType == CLUBMATCH)
					{
						setGameSettingsMask(mPlayerInfo, myGameId, 0, 15);
					}
					fetchLoadOuts(mPlayerInfo);
					mPlayerInfo->setViewId(++viewId);
					getStatsByGroupAsync(mPlayerInfo, friendList, "VProAttributeXP");
					if (mScenarioType == CLUBMATCH)
					{
						setPlayerAttributes("OV0", "29622272~29622272~0~-1~-1~-1~0~1808", myGameId);
						setPlayerAttributes("OV1", "2359296~2359296~0~-1~-1~-1~0~144", myGameId);
					}
					if (mScenarioType == CLUBFRIENDLIES || mScenarioType == CLUBSCUP)
					{
						setPlayerAttributes("OV1", "29622272~29622272~0~-1~-1~-1~0~1808", myGameId);
						setPlayerAttributes("OV0", "2359296~2359296~0~-1~-1~-1~0~144", myGameId);
					}
				}
			}
			sleep(CLUBS_CONFIG.GameStateSleepInterval);
			break;
		}
		case IN_GAME:
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=IN_GAME");
			if (!ingame_enter)
			{
				ingame_enter = true;
				simulateInGame();
				//Start InGame Timer	
				if (isPlatformHost)
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " setInGameStartTime. gameID=" << myGameId);
					setInGameStartTime();
				}
			}
			setPlayerAttributes("RDY", "0", myGameId);
			if (mScenarioType == CLUBSPRACTICE)
			{
				if (!isPlatformHost)
				{
					Collections::AttributeMap attributes;
					attributes.clear();
					attributes["CAP"] = "2 Dev 214843230";
					setGameAttributes(myGameId, attributes);
					attributes.clear();
					attributes["CAP"] = "2 Dev 36778560";
					setGameAttributes(myGameId, attributes);
				}
				else
				{
					Collections::AttributeMap attributes;
					attributes.clear();
					attributes["CAP"] = "FOU=0,0,0,&HAF=0&MST=1&MTM=0&SCR=0-0,0,0,";
					setGameAttributes(myGameId, attributes);
					attributes.clear();
					attributes["CAP"] = "2 Dev 36778560";
					setGameAttributes(myGameId, attributes);
				}
			}
			if (RollDice(CLUBS_CONFIG.inGameReportTelemetryProbability))
			{
				//10% probability setGameAttributes - if platformhost
				executeReportTelemetry();
			}
			if (isPlatformHost)
			{
				Collections::AttributeMap attributes;
				int randValue = Random::getRandomNumber(100);
				if (randValue < 20)
				{
					gameAttributecount = gameAttributecount + 300;
					char8_t attributeValue[10];
					blaze_snzprintf(attributeValue, sizeof(attributeValue), "%d", gameAttributecount);
					attributes.clear();
					attributes["MTM"] = attributeValue;
					setGameAttributes(myGameId, attributes);
				}
			}

			if ((isPlatformHost && IsInGameTimedOut()) || (mIsMarkedForLeaving == true))
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " IsInGameTimedOut/mIsMarkedForLeaving=true. gameID=" << myGameId);
				advanceGameState(POST_GAME);
				sleep(1000);
			}

			sleep(CLUBS_CONFIG.GameStateSleepInterval);
			break;
		}
		case INITIALIZING:
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=INITIALIZING");
			//if (isPlatformHost)
			//{
			//	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad][" << mPlayerData->getPlayerId() << "calling advanceGameState");
			//	//Move this to DS -TODO
			//	advanceGameState(PRE_GAME);
			//}
			sleep(CLUBS_CONFIG.GameStateSleepInterval);
			break;
		}

		case POST_GAME:
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=POST_GAME");
			//submitGameReport
			submitGameReport(mScenarioType);

			/*leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, getGameId());
			BLAZE_INFO(BlazeRpcLog::gamemanager, "[ClubsPlayer::leaveGame][%" PRIu64 "]: player left game: GameId = %" PRIu64 "", mPlayerData->getPlayerId(), myGameId);
			//meshEndpointsDisconnected RPC is initiated from NOTIFY_NOTIFYPLAYERREMOVED
			leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mVoipGroupGId);
			//leave game group
			leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGamegroupGId);
			//leave home group
			leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mHomeGroupGId);*/
			//set game status to false
			if (mIsGameGroupOwner)
			{
				setClubGameStatus(false);
			}
			
			postGameCalls();

			setPlayerState(DESTRUCTING);

			sleep(CLUBS_CONFIG.GameStateSleepInterval);
			break;
		}
		case DESTRUCTING:
		{
			//  Player Left Game
			//  Presently using this state when user is removed from the game and return from this function.
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=DESTRUCTING");
			sleep(20000);
			setClientState(mPlayerInfo, ClientState::MODE_MENU);
			return err;
		}
		case MIGRATING:
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=MIGRATING");
			//  mGameData will be freed when NOTIFY_NOTIFYGAMEREMOVED notification comes.
			sleep(10000);
			break;
		}
		case GAME_GROUP_INITIALIZED:
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=GAME_GROUP_INITIALIZED");
			sleep(5000);
			break;
		}
		case INACTIVE_VIRTUAL:
		default:
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=" << getPlayerState());
			//  shouldn't come here
			return err;
		}
		}
	}
}

BlazeRpcError ClubsPlayer::clubsLeaderboards()
{
	BlazeRpcError result = ERR_OK;
	getLeaderboardTreeAsync(mPlayerInfo, "Club");
	result = getLeaderboardGroup(mPlayerInfo, -1, "ClubRankLeague1LB");
	//getLeaderboardTreeAsync(mPlayerInfo, "ClubsSeasonalPlay");
	//result = getLeaderboardGroup(mPlayerInfo, -1, "1v1_sp_clubs_overall");
	//result = getCenteredLeaderboard(mPlayerInfo, 0, 1, mPlayerInfo->getPlayerId(), "1v1_sp_clubs_overall");

	// FIltered Leaderboard
	FilteredLeaderboardStatsRequest::EntityIdList filterBoardEntityList;
	filterBoardEntityList.push_back(mPlayerInfo->getPlayerId());
	filterBoardEntityList.push_back(((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->getRandomPlayerId());
	filterBoardEntityList.push_back(((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->getRandomPlayerId());
	filterBoardEntityList.push_back(((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->getRandomPlayerId());
	getFilteredLeaderboard(mPlayerInfo, filterBoardEntityList, "ClubRankLeague1LB", 4294967295, 0);
	getFilteredLeaderboard(mPlayerInfo, filterBoardEntityList, "ClubRankLeague1LB", 4294967295, 1);
	GetMembersResponse response;
	getMembers(mPlayerInfo, getMyClubId(), 50, response);
	actionGetAwards(mPlayerData, getMyClubId());
	findClubsAsync();
	getFilteredLeaderboard(mPlayerInfo, filterBoardEntityList, "ClubRankLeague1LB", 4294967295, 1);
	getFilteredLeaderboard(mPlayerInfo, filterBoardEntityList, "ClubRankLeague1LB", 4294967295, 1);
	getMembers(mPlayerInfo, getMyClubId(), 50, response);
	actionGetAwards(mPlayerInfo, getMyClubId());
	result = getLeaderboard(mPlayerInfo, "1v1_sp_clubs_overall");

	sleep(5000);
	return result;
}

void ClubsPlayer::addPlayerToList()
{
	//  add player to universal list first
	Player::addPlayerToList();
	BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::addPlayerToList]: Player: " << mPlayerInfo->getPlayerId() << ", Game: " << myGameId);
	{
		((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->clubsPlayerMap.insert(eastl::pair<PlayerId, eastl::string>(mPlayerInfo->getPlayerId(), mPlayerInfo->getPersonaName()));
	}
}

void ClubsPlayer::removePlayerFromList()
{
	//  remove player from universal list first
	Player::removePlayerFromList();
	PlayerId  id = mPlayerInfo->getPlayerId();
	BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::removePlayerFromList]: Player: " << id << ", Game: " << myGameId);
	PlayerMap& playerMap = ((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->clubsPlayerMap;
	PlayerMap::iterator start = playerMap.begin();
	PlayerMap::iterator end = playerMap.end();
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

void ClubsPlayer::simulateInGame()
{
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][ClubsPlayer]: In Game simulation:" << myGameId);

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

	//EADP Profinity Service - Message
	if (StressConfigInst.getProfanityMessageProbability() > (uint32_t)Random::getRandomNumber(100))
	{
		FilterUserTextResponse filterReq; //Fill this request TODO
		FilteredUserText* filteredText1 = BLAZE_NEW FilteredUserText();
		filteredText1->setResult(Blaze::Util::FILTER_RESULT_UNPROCESSED);
		filteredText1->setFilteredText(StressConfigInst.getRandomMessage().c_str());
		filterReq.getFilteredTextList().push_back(filteredText1);
		filterForProfanity(mPlayerInfo, &filterReq);

		//char8_t buf[5240];
		//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::filterForProfanity] : \n" << filterReq.print(buf, sizeof(buf)));

	}

	if (isPlatformHost)
	{
		Collections::AttributeMap attributes;
		attributes.clear();
		attributes["FOU"] = "0,0,0,";
		setGameAttributes(myGameId, attributes);
		attributes.clear();
		attributes["HAF"] = "0";
		setGameAttributes(myGameId, attributes);
		attributes.clear();
		attributes["MST"] = "1";
		setGameAttributes(myGameId, attributes);
		attributes.clear();
		attributes["MTM"] = "0";
		setGameAttributes(myGameId, attributes);
		attributes.clear();
		attributes["SCR"] = "0-0,0,0,";
		setGameAttributes(myGameId, attributes);
		attributes.clear();
		attributes["SCR"] = "1-0,0,1,2 Dev 36778560";
		setGameAttributes(myGameId, attributes);
	}
	setPlayerAttributes("RDY", "0", myGameId);
	delayRandomTime(2000, 5000);
}

void ClubsPlayer::lobbyRpcCalls()
{
	Blaze::BlazeIdList friendList;
	uint32_t viewId = mPlayerInfo->getViewId();
	friendList.push_back(mPlayerInfo->getPlayerId());
	if (mScenarioType == CLUBSPRACTICE)
	{
		if (!isPlatformHost)
		{
			friendList.push_back(mPlayerInfo->getPlayerId());
		}
	}

	Blaze::PersonaNameList personaNameList;

	if (StressConfigInst.isRPCTuningEnabled())
	{
		personaNameList.push_back("q7764733053-GBen");
		personaNameList.push_back("CGFUT19_04");
		personaNameList.push_back("dr_hoo");
		personaNameList.push_back("CGFUT19_02");
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
	}

	FetchConfigResponse configResponse;
	Blaze::Clubs::GetMembersResponse clubResponse;
	getMembers(mPlayerInfo, getMyClubId(), 50, clubResponse);

	if (mScenarioType != CLUBFRIENDLIES)
	{
		if (isPlatformHost)
		{
			getPetitions(mPlayerInfo, getMyClubId(), Blaze::Clubs::CLUBS_PETITION_SENT_TO_CLUB);
		}
	}

	getStatGroup(mPlayerInfo, "VProPosition");
	mPlayerInfo->setViewId(++viewId);	//	viewId=1
	getStatsByGroupAsync(mPlayerInfo, friendList, "VProPosition", getMyClubId());	// Check entityIds
	
	setConnectionState(mPlayerInfo);
	setConnectionState(mPlayerInfo);

	/*BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " getProfanityAvatarProbability = " << StressConfigInst.getProfanityAvatarProbability());
	BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " getProfanityClubnameProbability = " << StressConfigInst.getProfanityClubnameProbability());
	BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " getProfanityMessageProbability = " << StressConfigInst.getProfanityMessageProbability());
	BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " Random getRandomAvatarFirstName = " << StressConfigInst.getRandomAvatarFirstName());
	BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " Random getRandomAvatarLastName = " << StressConfigInst.getRandomAvatarLastName());
	BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " Random ClubName = " << StressConfigInst.getRandomClub());
	BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " Random Message = " << StressConfigInst.getRandomMessage());
	*/

	eastl::string firstName = StressConfigInst.getRandomAvatarFirstName();
	eastl::string lastName = StressConfigInst.getRandomAvatarLastName();

	eastl::string randomString = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

	//Add max of 10 random characters to the firstName & lastName
	uint32_t randomNum = (uint32_t)Random::getRandomNumber(10);
	for (uint32_t i = 0; i < randomNum; i++)
	{
		firstName += randomString[(uint32_t)Random::getRandomNumber(sizeof(randomString) - 1)];
		lastName += randomString[(uint32_t)Random::getRandomNumber(sizeof(randomString) - 1)];
	}

	//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " Random FirstName = " << firstName);
	//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " Random LastName = " << lastName);

	mAvatarName = firstName + " " + lastName;

	//EADP Profinity Service - Change Avatar Name
	if (StressConfigInst.getProfanityAvatarProbability() > (uint32_t)Random::getRandomNumber(100))
	{
		getAvatarData(mPlayerInfo);
		FilterUserTextResponse filterReq; //Fill this request TODO
		FilteredUserText* filteredText1 = BLAZE_NEW FilteredUserText();
		filteredText1->setResult(Blaze::Util::FILTER_RESULT_UNPROCESSED);
		filteredText1->setFilteredText(firstName.c_str());
		filterReq.getFilteredTextList().push_back(filteredText1);
		FilteredUserText* filteredText2 = BLAZE_NEW FilteredUserText();
		filteredText2->setResult(Blaze::Util::FILTER_RESULT_UNPROCESSED);
		filteredText2->setFilteredText("");
		filterReq.getFilteredTextList().push_back(filteredText2);
		FilteredUserText* filteredText3 = BLAZE_NEW FilteredUserText();
		filteredText3->setResult(Blaze::Util::FILTER_RESULT_UNPROCESSED);
		filteredText3->setFilteredText(lastName.c_str());
		filterReq.getFilteredTextList().push_back(filteredText3);
		FilteredUserText* filteredText4 = BLAZE_NEW FilteredUserText();
		filteredText4->setResult(Blaze::Util::FILTER_RESULT_UNPROCESSED);
		filteredText4->setFilteredText(lastName.c_str());
		filterReq.getFilteredTextList().push_back(filteredText4);
		FilteredUserText* filteredText5 = BLAZE_NEW FilteredUserText();
		filteredText5->setResult(Blaze::Util::FILTER_RESULT_UNPROCESSED);
		filteredText5->setFilteredText("");
		filterReq.getFilteredTextList().push_back(filteredText5);
		FilteredUserText* filteredText6 = BLAZE_NEW FilteredUserText();
		filteredText6->setResult(Blaze::Util::FILTER_RESULT_UNPROCESSED);
		filteredText6->setFilteredText("");
		filterReq.getFilteredTextList().push_back(filteredText6);
		filterForProfanity(mPlayerInfo, &filterReq);

		//char8_t buf[5240];
		//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::filterForProfanity] : \n" << filterReq.print(buf, sizeof(buf)));

		//if ((StressConfigInst.getProfanityAvatarProbability() - 15) > (uint32_t)Random::getRandomNumber(100)) {
		updateLoadOutsPeripherals(mPlayerInfo);
		sleep(1000);
		updateAvatarName(mPlayerInfo, firstName.c_str(), lastName.c_str());
		//}
	}

	//EADP Profinity Service - Change Club Name
	if (StressConfigInst.getProfanityClubnameProbability() > (uint32_t)Random::getRandomNumber(100))
	{
		FilterUserTextResponse filterReq; //Fill this request TODO
		FilteredUserText* filteredText1 = BLAZE_NEW FilteredUserText();
		filteredText1->setResult(Blaze::Util::FILTER_RESULT_UNPROCESSED);
		filteredText1->setFilteredText(StressConfigInst.getRandomClub().c_str());
		filterReq.getFilteredTextList().push_back(filteredText1);
		filterForProfanity(mPlayerInfo, &filterReq);

		//char8_t buf[5240];
		//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::filterForProfanity] : \n" << filterReq.print(buf, sizeof(buf)));
	}

	//updateLoadOutsPeripherals(mPlayerInfo);
	if (mScenarioType == CLUBSPRACTICE)
	{
		if (!isPlatformHost)
		{
			subscribeToCensusDataUpdates(mPlayerInfo);
		}
		//updateLoadOutsPeripherals(mPlayerInfo);
		//updateAvatarName(mPlayerInfo, firstName.c_str(), lastName.c_str());
		setConnectionState(mPlayerInfo);
		setConnectionState(mPlayerInfo);
	}
	// needed for clubs_tickr table population
	// actionPostNews(mPlayerInfo, getMyClubId(), postNewsCount++);

	getInvitations(mPlayerInfo, getMyClubId(), Clubs::CLUBS_SENT_TO_ME);
	getMessages(mPlayerInfo, 5);
	getStatGroup(mPlayerInfo, "VProStatAccom");
	getStatGroup(mPlayerInfo, "VProInfo");
	getStatGroup(mPlayerInfo, "VProPosStats");
	getStatGroup(mPlayerInfo, "VProCupStats");
	
	getClubs(mPlayerInfo, getMyClubId());
	fetchClientConfig(mPlayerInfo, "FIFA_AI_PLAYERS_SETTINGS_9_10", configResponse);
	fetchClientConfig(mPlayerInfo, "FIFA_AI_PLAYERS_SETTINGS_7_8", configResponse);
	fetchClientConfig(mPlayerInfo, "FIFA_AI_PLAYERS_SETTINGS_5_6", configResponse);

	if (isPlatformHost)
	{
		getInvitations(mPlayerInfo, getMyClubId(), Clubs::CLUBS_SENT_BY_CLUB);
	}
	else
	{
		getPetitions(mPlayerInfo, getMyClubId(), Blaze::Clubs::PetitionsType::CLUBS_PETITION_SENT_BY_ME);
	}

	mPlayerInfo->setViewId(++viewId);	//2
	getStatsByGroupAsync(mPlayerInfo, "VProStatAccom");
	mPlayerInfo->setViewId(++viewId);	//	3
	getStatsByGroupAsync(mPlayerInfo, "VProInfo");
	mPlayerInfo->setViewId(++viewId);	//	4
	getStatsByGroupAsync(mPlayerInfo, "VProPosStats");
	mPlayerInfo->setViewId(++viewId);	//	5
	getStatsByGroupAsync(mPlayerInfo, "VProCupStats");
	getMembers(mPlayerInfo, getMyClubId(), 50, clubResponse);
	fetchClientConfig(mPlayerInfo, "FIFA_AI_PLAYERS_SETTINGS_3_4", configResponse);
	fetchClientConfig(mPlayerInfo, "FIFA_AI_PLAYERS_SETTINGS_1_2", configResponse);
	
	if (isPlatformHost)
	{
		getPetitions(mPlayerInfo, getMyClubId(), Blaze::Clubs::CLUBS_PETITION_SENT_BY_ME);
	}
	else
	{
		getMembers(mPlayerInfo, getMyClubId(), 50, clubResponse);
	}
	
	if (!isPlatformHost && mScenarioType == CLUBMATCH)
	{
		lookupUsersIdentification(mPlayerInfo);		//	Added as per logs
	}
	
	fetchClientConfig(mPlayerInfo, "FIFA_AI_PLAYERS_PRACTICE_SETTINGS_5", configResponse);
	fetchLoadOuts(mPlayerInfo);
	fetchClientConfig(mPlayerInfo, "FIFA_AI_PLAYERS_PRACTICE_SETTINGS_4", configResponse);
	fetchClientConfig(mPlayerInfo, "FIFA_AI_PLAYERS_PRACTICE_SETTINGS_3", configResponse);
	
	if (isPlatformHost)
	{
		getPetitions(mPlayerInfo, getMyClubId(), Blaze::Clubs::CLUBS_PETITION_SENT_TO_CLUB);
		getPetitions(mPlayerInfo, getMyClubId(), Blaze::Clubs::CLUBS_PETITION_SENT_TO_CLUB);
	}
	
	fetchClientConfig(mPlayerInfo, "FIFA_AI_PLAYERS_PRACTICE_SETTINGS_2", configResponse);
	fetchClientConfig(mPlayerInfo, "FIFA_AI_PLAYERS_PRACTICE_SETTINGS_1", configResponse);
	mPlayerInfo->setViewId(++viewId);	//	6
	getStatsByGroupAsync(mPlayerInfo, "VProPosition");
	fetchClientConfig(mPlayerInfo, "FIFA_TACTICS_AI_SETTINGS_0", configResponse);
	fetchClientConfig(mPlayerInfo, "FIFA_TACTICS_AI_SETTINGS_1", configResponse);
	fetchClientConfig(mPlayerInfo, "FIFA_TACTICS_AI_SETTINGS_2", configResponse);
	fetchClientConfig(mPlayerInfo, "FIFA_TACTICS_AI_SETTINGS_3", configResponse);
	fetchClientConfig(mPlayerInfo, "FIFA_TACTICS_AI_SETTINGS_4", configResponse);
	fetchClientConfig(mPlayerInfo, "FIFA_TACTICS_AI_SETTINGS_5", configResponse);
	
	if (mScenarioType == CLUBFRIENDLIES || mScenarioType == CLUBSCUP)
	{
		// reduce client side CPU usage
		sleep(5000);
		getStatGroup(mPlayerInfo, "ClubSeasonalPlay");
		mPlayerInfo->setViewId(++viewId); //7
		friendList.clear();
		friendList.push_back(getMyClubId());
		getStatsByGroupAsync(mPlayerInfo, friendList, "ClubSeasonalPlay");
	}
		
	//Abuse Reporting Changes
	if (StressConfigInst.getProfanityAvatarProbability() > (uint32_t)Random::getRandomNumber(100))
	{
		getAvatarData(mPlayerInfo);
	}
	
	fetchClientConfig(mPlayerInfo, "FIFA_CLUBS_SEASONALPLAY", configResponse);

	FifaCups::CupInfo getCupsResponse;
	getCupInfo(mPlayerInfo, FifaCups::FIFACUPS_MEMBERTYPE_CLUB, getCupsResponse, getMyClubId());
	OSDKTournaments::GetMyTournamentIdResponse getTournamentIdResponse;
	BlazeRpcError err = ERR_OK;
	err = tournamentCalls();
	//Abuse Reporting Changes
	if (StressConfigInst.getProfanityAvatarProbability() > (uint32_t)Random::getRandomNumber(100))
	{
		getAvatarData(mPlayerInfo);
	}
	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "[ClubsPlayer::tournamentRpcCalls are failing due to " << err);
	}

	fetchPlayerGrowthConfig(mPlayerInfo);
	fetchSkillTreeConfig(mPlayerInfo);
	fetchPerksConfig(mPlayerInfo);
	fetchLoadOuts(mPlayerInfo);
	
	if (mScenarioType != CLUBSPRACTICE)
	{
		getInvitations(mPlayerInfo, getMyClubId(), Clubs::CLUBS_SENT_TO_ME);
		if (isPlatformHost)
		{
			getInvitations(mPlayerInfo, getMyClubId(), Clubs::CLUBS_SENT_BY_CLUB);
		}
		getPetitions(mPlayerInfo, getMyClubId(), Blaze::Clubs::CLUBS_PETITION_SENT_BY_ME);
		if (isPlatformHost)
		{
			getPetitions(mPlayerInfo, getMyClubId(), Blaze::Clubs::CLUBS_PETITION_SENT_TO_CLUB);
		}

		getInvitations(mPlayerInfo, getMyClubId(), Clubs::CLUBS_SENT_TO_ME);
		if (isPlatformHost)
		{
			getInvitations(mPlayerInfo, getMyClubId(), Clubs::CLUBS_SENT_BY_CLUB);
		}
		getPetitions(mPlayerInfo, getMyClubId(), Blaze::Clubs::CLUBS_PETITION_SENT_BY_ME);
		if (isPlatformHost)
		{
			getPetitions(mPlayerInfo, getMyClubId(), Blaze::Clubs::CLUBS_PETITION_SENT_TO_CLUB);
		}
	}
	else 
	{
		getInvitations(mPlayerInfo, getMyClubId(), Clubs::CLUBS_SENT_TO_ME);
		if (isPlatformHost)
		{
			getInvitations(mPlayerInfo, getMyClubId(), Clubs::CLUBS_SENT_BY_CLUB);
		}
		getPetitions(mPlayerInfo, getMyClubId(), Blaze::Clubs::CLUBS_PETITION_SENT_BY_ME);
		if (isPlatformHost)
		{
			getPetitions(mPlayerInfo, getMyClubId(), Blaze::Clubs::CLUBS_PETITION_SENT_TO_CLUB);
		}
		getInvitations(mPlayerInfo, getMyClubId(), Clubs::CLUBS_SENT_TO_ME);
		if (isPlatformHost)
		{
			getInvitations(mPlayerInfo, getMyClubId(), Clubs::CLUBS_SENT_BY_CLUB);
		}
		getPetitions(mPlayerInfo, getMyClubId(), Blaze::Clubs::CLUBS_PETITION_SENT_BY_ME);
		if (isPlatformHost)
		{
			getPetitions(mPlayerInfo, getMyClubId(), Blaze::Clubs::CLUBS_PETITION_SENT_TO_CLUB);
		}
		getInvitations(mPlayerInfo, getMyClubId(), Clubs::CLUBS_SENT_TO_ME);
		if (isPlatformHost)
		{
			getInvitations(mPlayerInfo, getMyClubId(), Clubs::CLUBS_SENT_BY_CLUB);
		}
		getPetitions(mPlayerInfo, getMyClubId(), Blaze::Clubs::CLUBS_PETITION_SENT_BY_ME);
		if (isPlatformHost)
		{
			getPetitions(mPlayerInfo, getMyClubId(), Blaze::Clubs::CLUBS_PETITION_SENT_TO_CLUB);
		}
		//removeMember(mPlayerInfo, getMyClubId());
		getInvitations(mPlayerInfo, getMyClubId(), Clubs::CLUBS_SENT_TO_ME);
		getPetitions(mPlayerInfo, getMyClubId(), Blaze::Clubs::CLUBS_PETITION_SENT_BY_ME);
		if (isPlatformHost)
		{
			getInvitations(mPlayerInfo, getMyClubId(), Clubs::CLUBS_SENT_BY_CLUB);
			getPetitions(mPlayerInfo, getMyClubId(), Blaze::Clubs::CLUBS_PETITION_SENT_TO_CLUB);
		}
	}
	fetchAIPlayerCustomization(mPlayerInfo, getMyClubId());
	
	//Abuse Reporting Changes
	if (StressConfigInst.getProfanityAvatarProbability() > (uint32_t)Random::getRandomNumber(100))
	{
		getAvatarData(mPlayerInfo);
	}

	// reduce client side CPU usage
	sleep(5000);

	//Abuse Report Changes - Get and accept the petetions if any
	if (StressConfigInst.getAcceptPetitionProbability() > (uint32_t)Random::getRandomNumber(100))
	{
		Clubs::GetPetitionsRequest request;
		Clubs::GetPetitionsResponse response;
		request.setPetitionsType(Blaze::Clubs::CLUBS_PETITION_SENT_TO_CLUB);
		request.setClubId(getMyClubId());
		request.setSortType(Clubs::CLUBS_SORT_TIME_DESC);
		err = mPlayerInfo->getComponentProxy()->mClubsProxy->getPetitions(request, response);
		if (err == ERR_OK) {
			//char8_t rebuf[5240];
			//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::GetPetitionsResponse] : \n" << response.print(rebuf, sizeof(rebuf)));
			if (response.getClubPetitionsList().size() > 0)
			{
				Blaze::Clubs::ClubMessage* clubMessage = response.getClubPetitionsList().front();
				//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::lobbyRPC]" << mPlayerData->getPlayerId() << " Petetion ID = " << clubMessage->getMessageId());
				acceptPetition(mPlayerInfo, clubMessage->getMessageId());
			}
		}
	}

	if (mScenarioType == CLUBFRIENDLIES)
	{
		personaNameList.clear();
		personaNameList.push_back("UFCnew");
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
	}
	
	if (mScenarioType != CLUBSCUP)
	{
		validateString(mPlayerInfo);
		updateAIPlayerCustomization(mPlayerInfo, getMyClubId());
	}

	getClubs(mPlayerInfo, getMyClubId());
	getMembers(mPlayerInfo, getMyClubId(), 50, clubResponse);
	if (mScenarioType != CLUBSCUP || mScenarioType != CLUBFRIENDLIES)
	{
		findClubsAsync();
	}

	if (isPlatformHost)
	{
		getPetitions(mPlayerInfo, getMyClubId(), Blaze::Clubs::CLUBS_PETITION_SENT_TO_CLUB);
	}
	friendList.clear();
	if (isPlatformHost)
	{
		friendList.push_back(mPlayerInfo->getPlayerId());
	}
	friendList.push_back(mPlayerInfo->getPlayerId());
	mPlayerInfo->setViewId(++viewId);	//	8
	getStatsByGroupAsync(mPlayerInfo, friendList, "VProPosition", getMyClubId());	// Called in if block for Practice

	fetchCustomTactics(mPlayerInfo, getMyClubId());
	fetchCustomTactics(mPlayerInfo, getMyClubId());
	sleep(2000);

	if (StressConfigInst.isRPCTuningEnabled())
	{
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		resetUserGeoIPData(mPlayerInfo);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		userSettingsLoadAll(mPlayerInfo);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
	}
}

BlazeRpcError ClubsPlayer::sendOrReceiveInvitation()
{
	BlazeRpcError err = ERR_OK;

	// check for invitaitons sent to me
	GetInvitationsRequest request;
	GetInvitationsResponse response;

	request.setInvitationsType(CLUBS_SENT_TO_ME);
	request.setSortType(CLUBS_SORT_TIME_DESC);
	err = mPlayerInfo->getComponentProxy()->mClubsProxy->getInvitations(request, response);

	if (err != ERR_OK)
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[ClubsPlayer::sendOrReceiveInvitation : " << mPlayerInfo->getPlayerId() << " Could not get list of invitations");
		return err;
	}

	if (getMyClubId() != 0)
	{
		// get memebers
		GetMembersRequest reqGetMembs;
		GetMembersResponse respGetMembs;

		reqGetMembs.setClubId(getMyClubId());
		err = mPlayerInfo->getComponentProxy()->mClubsProxy->getMembers(reqGetMembs, respGetMembs);

		if (err != ERR_OK)
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[ClubsPlayer::sendOrReceiveInvitation : " << mPlayerInfo->getPlayerId() << " Could not get list of club members");
			return err;
		}

		if (respGetMembs.getClubMemberList().size() > 0 && response.getClubInvList().size() > 0)
		{
			// Need to add player remove logic. Currently its not present

			// rigt now just either accept or declien invites
			ClubMessageList &msgList = response.getClubInvList();
			ClubMessageList::const_iterator it = msgList.begin();

			for (; it != msgList.end(); it++)
			{
				ProcessInvitationRequest inviteRequest;
				inviteRequest.setInvitationId((*it)->getMessageId());
				if ((CLUBS_CONFIG.acceptInvitationProbablity > (uint32_t)Random::getRandomNumber(100)) && (it == msgList.begin()))
				{
					err = mPlayerInfo->getComponentProxy()->mClubsProxy->acceptInvitation(inviteRequest);
					BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[ClubsPlayer::sendOrReceiveInvitation] : Accepted Invite for  " << (*it)->getMessageId() << " from player : " << mPlayerInfo->getPlayerId());
				}
				else
				{
					err = mPlayerInfo->getComponentProxy()->mClubsProxy->declineInvitation(inviteRequest);
					BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[ClubsPlayer::sendOrReceiveInvitation] : Declined Invite for  " << (*it)->getMessageId() << " from player : " << mPlayerInfo->getPlayerId());
				}

				if (err != ERR_OK)
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[ClubsPlayer::sendOrReceiveInvitation : " << mPlayerInfo->getPlayerId() << " Accept/Decline Invites failed");
					return err;
				}
			}
		}

		// Sending Invites
		GetClubMembershipForUsersRequest membershipReq;
		GetClubMembershipForUsersResponse membershipResp;

		membershipReq.getBlazeIdList().push_back(mPlayerInfo->getPlayerId());
		err = mPlayerInfo->getComponentProxy()->mClubsProxy->getClubMembershipForUsers(membershipReq, membershipResp);

		if (err == ERR_OK)
		{
			ClubMembershipMap::const_iterator itr = membershipResp.getMembershipMap().find(mPlayerInfo->getPlayerId());
			if (itr == membershipResp.getMembershipMap().end())
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[ClubsPlayer::sendOrReceiveInvitation : GetClubMemberShipforUser failed " << mPlayerInfo->getPlayerId() << " Player not found in his own club");
				return ERR_SYSTEM;
			}

			BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[ClubsPlayer::sendOrReceiveInvitation] : Successfully Got inviteList and clubMembershipmap " << mPlayerInfo->getPlayerId());

			ClubMembershipList &membershipList = itr->second->getClubMembershipList();
			ClubMembership *membership = *(membershipList.begin()); // assuming at least one (and the first one)
			MembershipStatus status = membership->getClubMember().getMembershipStatus();
			if (status == CLUBS_OWNER || status == CLUBS_GM)
			{
				// if I am GM in no-leave club, send invitation
				SendInvitationRequest sendInvitationRequest;
				BlazeId randomUserId = StressConfigInst.getBaseAccountId() + mPlayerInfo->getLogin()->getOwner()->getStartIndex() + Random::getRandomNumber((int64_t)mPlayerInfo->getLogin()->getOwner()->getPsu());

				//BlazeId randomUserId = (randomIndent > mPlayerInfo->getLogin()->getOwner()->getStartIndex()) ? (randomIndent - mPlayerInfo->getLogin()->getOwner()->getStartIndex()) : randomIndent ;
				BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[ClubsPlayer::sendOrReceiveInvitation] : This is our randomeUserzID : " << randomUserId << " this is our start index we subtract from it " << mPlayerInfo->getLogin()->getOwner()->getStartIndex());
				sendInvitationRequest.setBlazeId(randomUserId);
				sendInvitationRequest.setClubId(getMyClubId());
				//char8_t buf[20240];
				//BLAZE_TRACE_RPC_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::sendOrReceiveInvitation] : \n" << request.print(buf, sizeof(buf)));
				err = mPlayerInfo->getComponentProxy()->mClubsProxy->sendInvitation(sendInvitationRequest);

				BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[ClubsPlayer::sendOrReceiveInvitation] : Just called sendInvitation RPC " << mPlayerInfo->getPlayerId());

				if (err == Blaze::CLUBS_ERR_INVITATION_ALREADY_SENT || err == Blaze::CLUBS_ERR_PETITION_ALREADY_SENT)
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[ClubsPlayer::sendOrReceiveInvitation] : Invitation already sent from " << mPlayerInfo->getPlayerId() << " to : " << randomUserId);
					err = Blaze::ERR_OK;
				}
				else if (err == Blaze::CLUBS_ERR_MAX_INVITES_SENT)
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[ClubsPlayer::sendOrReceiveInvitation : Invite count maxed for  " << mPlayerInfo->getPlayerId() << " to : " << randomUserId);
					// revoke one invitation
					GetInvitationsRequest inviteRequest;
					GetInvitationsResponse inviteResponse;

					inviteRequest.setClubId(getMyClubId());
					inviteRequest.setInvitationsType(CLUBS_SENT_BY_CLUB);
					err = mPlayerInfo->getComponentProxy()->mClubsProxy->getInvitations(inviteRequest, inviteResponse);
					//BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[ClubsPlayer::sendOrReceiveInvitation : GenInvitations  " << mPlayerInfo->getPlayerId()<<" to : "

					if (err == ERR_OK)
					{
						ClubMessageList &msgListTemp = inviteResponse.getClubInvList();
						if (msgListTemp.size() > 0)
						{
							ProcessInvitationRequest processInviteReq;
							processInviteReq.setInvitationId((*msgListTemp.begin())->getMessageId());
							err = mPlayerInfo->getComponentProxy()->mClubsProxy->revokeInvitation(processInviteReq);
						}
						else
						{
							// The invite list is empty ???
							err = ERR_OK;
						}
					}
				}
				else if (err == CLUBS_ERR_MAX_INVITES_RECEIVED)
				{
					GetInvitationsRequest inviteRequest;
					GetInvitationsResponse inviteResponse;

					inviteRequest.setClubId(getMyClubId());
					inviteRequest.setInvitationsType(CLUBS_SENT_TO_ME);
					inviteRequest.setSortType(CLUBS_SORT_TIME_DESC);
					err = mPlayerInfo->getComponentProxy()->mClubsProxy->getInvitations(inviteRequest, inviteResponse);

					if (err == ERR_OK)
					{
						ClubMessageList &msgListTemp = inviteResponse.getClubInvList();
						if (msgListTemp.size() > 0)
						{
							ProcessInvitationRequest processInviteReq;
							processInviteReq.setInvitationId((*msgListTemp.begin())->getMessageId());
							err = mPlayerInfo->getComponentProxy()->mClubsProxy->revokeInvitation(processInviteReq);
						}
						else
						{
							// The invite list is empty ???
							err = ERR_OK;
						}
					}

				}
			}

		}
	}
	return err;

}

void ClubsPlayer::postGameCalls()
{
	setClientState(mPlayerInfo, Blaze::ClientState::MODE_SINGLE_PLAYER, Blaze::ClientState::STATUS_NORMAL);
	Blaze::BlazeIdList friendList;
	friendList.push_back(mPlayerInfo->getPlayerId());
	uint32_t viewId = mPlayerInfo->getViewId();
	//setPlayerAttributes("_A", "", myGameId);
	//setPlayerAttributes("_V", "", myGameId);
	if (isPlatformHost)
	{
		Collections::AttributeMap attributes;
		attributes.clear();
		attributes["HAF"] = "5";
		setGameAttributes(myGameId, attributes);
	}
	if (mScenarioType == CLUBSCUP || mScenarioType == CLUBMATCH)
	{
		if (isPlatformHost)
		{
			Collections::AttributeMap attributes;
			attributes.clear();
			attributes["MST"] = "2";
			setGameAttributes(myGameId, attributes);
		}
		setPlayerAttributes("_A", "", myGameId);
		setPlayerAttributes("_V", "", myGameId);
		getKeyScopesMap(mPlayerInfo);
		getStatGroup(mPlayerInfo, "ClubRankingPointsSummary");
		mPlayerInfo->setViewId(++viewId);
		friendList.clear();
		friendList.push_back(getMyClubId());
		getStatsByGroupAsync(mPlayerInfo, friendList, "ClubRankingPointsSummary");
		friendList.clear();
		friendList.push_back(mPlayerInfo->getPlayerId());
	}

	sleep(500);
	if (mScenarioType == CLUBMATCH)
	{
		getStatGroup(mPlayerInfo, "ClubSeasonalPlay");
		mPlayerInfo->setViewId(++viewId);
		friendList.clear();
		friendList.push_back(getMyClubId());
		getStatsByGroupAsync(mPlayerInfo, friendList, "ClubSeasonalPlay");
		friendList.clear();
		friendList.push_back(mPlayerInfo->getPlayerId());
		FetchConfigResponse configResponse;
		fetchClientConfig(mPlayerInfo, "FIFA_H2H_SEASONALPLAY", configResponse);
	}
	fetchLoadOuts(mPlayerInfo);
	fetchLoadOuts(mPlayerInfo);
	getStatGroup(mPlayerInfo, "VProAttributeXP");
	lookupUsersIdentification(mPlayerInfo);
	getKeyScopesMap(mPlayerInfo);
	//lookupUsersIdentification(mPlayerInfo);
	mPlayerInfo->setViewId(++viewId);
	getStatsByGroupAsync(mPlayerInfo, friendList, "VProAttributeXP", getMyClubId());
	setClientState(mPlayerInfo, ClientState::MODE_MENU, ClientState::STATUS_NORMAL);

	if (StressConfigInst.isRPCTuningEnabled())
	{
		if (!mCensusdataSubscriptionStatus)
		{
			BlazeRpcError censusDataErr;
			censusDataErr = subscribeToCensusDataUpdates(mPlayerInfo);
			if (censusDataErr == ERR_OK)
			{
				SetSubscriptionSatatus();
			}
		}
		else {
			BlazeRpcError censusDataErr;
			censusDataErr = unsubscribeFromCensusData(mPlayerInfo);
			if (censusDataErr == ERR_OK)
			{
				UnsetSubscriptionSatatus();
			}
		}
	}

}

BlazeRpcError ClubsPlayer::tournamentRpcCalls()
{
	BlazeRpcError err = ERR_OK;
	OSDKTournaments::GetMyTournamentIdResponse getTournamentIdResponse;
	OSDKTournaments::GetTournamentsResponse getTournamentsResponse;
	OSDKTournaments::GetMyTournamentDetailsResponse getMyTournamentDetailsResponse;
	OSDKTournaments::TournamentId tournamentId = 0;

	FifaCups::CupInfo getCupsResponse;
	err = getCupInfo(mPlayerInfo, FifaCups::FIFACUPS_MEMBERTYPE_CLUB, getCupsResponse, getMyClubId());
	if (err == ERR_OK)
	{
		setCupId(getCupsResponse.getCupId());
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "ClubsPlayer::tournamentRpcCalls:player= " << mPlayerData->getPlayerId() << " CupId= " << getCupId());
	}
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::initializeCupAndTournments]:" << mPlayerData->getPlayerId() << " getMyTournamentId call 2 ");
	err = getMyTournamentId(mPlayerInfo, Blaze::OSDKTournaments::MODE_FIFA_CUPS_CLUB, getTournamentIdResponse, getMyClubId());
	if (err == ERR_OK)
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::initializeCupAndTournments]:" << mPlayerData->getPlayerId() << " getMyTournamentId call 2 passed ");
		tournamentId = getTournamentsResponse.getTournamentList().at(0)->getTournamentId();
		getTournaments(mPlayerInfo, OSDKTournaments::TournamentModes::MODE_FIFA_CUPS_USER, getTournamentsResponse);
	}
	else if (err == OSDKTOURN_ERR_MEMBER_NOT_IN_TOURNAMENT)
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::initializeCupAndTournments]:" << mPlayerData->getPlayerId() << " getMyTournamentId call 2 faled but calling getTournaments");
		err = getTournaments(mPlayerInfo, Blaze::OSDKTournaments::MODE_FIFA_CUPS_CLUB, getTournamentsResponse);
		if (err == ERR_OK && getTournamentsResponse.getTournamentList().size() > 0)
		{
			// Means that the player is not yet into any tournaments, so get the tournament id first and join into that;
			tournamentId = getTournamentsResponse.getTournamentList().at(0)->getTournamentId();
			// Join Tournament	
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::initializeCupAndTournments]:" << mPlayerData->getPlayerId() << " getMyTournamentId call 2 successfuly got tournamnet id");
			err = joinTournament(mPlayerInfo, tournamentId);
		}
		setTournamentId(tournamentId);
	}
	if (err == ERR_OK)
	{
		setTournamentId(tournamentId);
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "ClubsPlayer::tournamentRpcCalls:player= " << mPlayerData->getPlayerId() << " TournamentId is set. " << getTournamentId());
		if (!getTournamentIdResponse.getIsActive())
		{
			//if the player is not in the tournament means that he might have lost/won the last tournament, so resetting here..
			osdkResetTournament(mPlayerInfo, getTournamentId(), Blaze::OSDKTournaments::MODE_FIFA_CUPS_CLUB);
		}
	}
	else
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "[ClubsPlayer::tournamentRpcCalls::TournamentId is Not Set for " << mPlayerData->getPlayerId());
	}
	return err;

	//   //get tournaments 	
		//bool mIscallJoinTournament = false;
		//err = getTournaments(mPlayerInfo,Blaze::OSDKTournaments::MODE_FIFA_CUPS_CLUB,getTournamentsResponse);			
		//if (tournamentId == 0 && getTournamentsResponse.getTournamentList().size() > 0) 
		//{  
		//	// Means that the player is not yet into any tournaments, so get the tournament id first and join into that;
		//	tournamentId = getTournamentsResponse.getTournamentList().at(0)->getTournamentId();
		//	mIscallJoinTournament = true;
		//}
		//// Join Tournament
		//if(getTournamentId() > 0 && mIscallJoinTournament)
		//{
		//	// setting blaze iD here if. it fails need to send Zero instead
		//	err = joinTournament(mPlayerInfo, getTournamentId());	
		//	mIscallJoinTournament = false;		
		//}
		//if(getTournamentId() > 0)
	//   {
		//	err = getMyTournamentDetails(mPlayerInfo,Blaze::OSDKTournaments::MODE_FIFA_CUPS_CLUB,tournamentId,getMyTournamentDetailsResponse);        
	//   }	
	//	return err;
}

bool ClubsPlayer::executeReportTelemetry()
{
	BlazeRpcError err = ERR_OK;
	if (!mPlayerData->isConnected())
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "[ClubsPlayer::ReportTelemetry::User Disconnected = " << mPlayerData->getPlayerId());
		return false;
	}
	if (getGameId() == 0)
	{
		BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::ReportTelemetry][GameId for player = " << mPlayerInfo->getPlayerId() << "] not present");
		return false;
	}
	StressGameInfo* ptrGameInfo = ClubsPlayerGameDataRef(mPlayerInfo).getGameData(getGameId());
	if (ptrGameInfo == NULL)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::ReportTelemetry][GameData for game = " << getGameId() << "] not present");
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
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "ClubsPlayer: reportTelemetry failed with err=" << ErrorHelp::getErrorName(err) << " playerID= " << mPlayerInfo->getPlayerId());
	}
	return true;
}

BlazeRpcError ClubsPlayer::startMatchmakingScenario(ScenarioType scenarioName)
{
	BlazeRpcError err = ERR_OK;
	StartMatchmakingScenarioRequest request;
	StartMatchmakingScenarioResponse response;
	MatchmakingCriteriaError criteriaError;

	char8_t clubIdString[MAX_GAMENAME_LEN] = { '\0' };
	blaze_snzprintf(clubIdString, sizeof(clubIdString), "%" PRId64, getMyClubId());

	request.getCommonGameData().setGameProtocolVersionString(StressConfigInst.getGameProtocolVersionString());
	request.getCommonGameData().setGameType(GAME_TYPE_GAMESESSION);
	NetworkAddress& hostAddress = request.getCommonGameData().getPlayerNetworkAddress();
/*	if (PLATFORM_XONE==StressConfigInst.getPlatform())
	{
		hostAddress.switchActiveMember(NetworkAddress::MEMBER_XBOXCLIENTADDRESS);
		if (mPlayerData->getExternalId())
			hostAddress.getXboxClientAddress()->setXuid(mPlayerData->getExternalId());
	}
	else
	{*/
		hostAddress.switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
		if (!mPlayerData->isConnected())
		{
			BLAZE_ERR_LOG(BlazeRpcLog::util, "[ClubsPlayer::clubsStartMatchmaking::User Disconnected = " << mPlayerData->getPlayerId());
			return ERR_SYSTEM;
		}
		hostAddress.getIpPairAddress()->getExternalAddress().setIp(mPlayerData->getConnection()->getAddress().getIp());
		hostAddress.getIpPairAddress()->getExternalAddress().setPort(mPlayerData->getConnection()->getAddress().getPort());
		hostAddress.getIpPairAddress()->getExternalAddress().copyInto(hostAddress.getIpPairAddress()->getInternalAddress());
//	}
	PlayerJoinData& playerJoinData = request.getPlayerJoinData();
	//Set Game Group ID 
	playerJoinData.setGroupId(BlazeObjectId(4, 2, mGamegroupGId));
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
		playerJoinDataPtr->setRole("");
		playerJoinDataPtr->setSlotType(GameManager::INVALID_SLOT_TYPE);
		if (mScenarioType == CLUBMATCH)
		{
			playerJoinDataPtr->getPlayerAttributes()["OSDK_clubIdInverse"] = clubIdString;
		}
		playerJoinDataPtr->getUser().getPlatformInfo().getExternalIds().setXblAccountId(mPlayerData->getExternalId());
		playerJoinDataPtr->getUser().getPlatformInfo().getEaIds().setNucleusAccountId(mPlayerData->getPlayerId());
		playerJoinDataPtr->getUser().setExternalId(mPlayerData->getExternalId());
		playerJoinDataPtr->getUser().setBlazeId(mPlayerData->getPlayerId());
		playerJoinDataPtr->getUser().setName(mPlayerData->getPersonaName().c_str());
	}
	if (mScenarioType == CLUBMATCH)
	{
		SET_SCENARIO_ATTRIBUTE(request, "CLUB_ID", clubIdString);
		SET_SCENARIO_ATTRIBUTE(request, "CLUB_ID_INVERSE_THRESHOLD", "OSDK_matchExact");
		SET_SCENARIO_ATTRIBUTE(request, "CLUB_LEAGUE", "16");
		SET_SCENARIO_ATTRIBUTE(request, "CLUB_LEAGUE_ID", "1");
		SET_SCENARIO_ATTRIBUTE(request, "CLUB_MM_ANY", "1");
		SET_SCENARIO_ATTRIBUTE(request, "CLUB_MM_GK", "1");
		SET_SCENARIO_ATTRIBUTE(request, "CLUB_NUM_PLAYERS", "2");
		SET_SCENARIO_ATTRIBUTE(request, "CLUB_NUM_PLAYERS_THR", "OSDK_matchAny");
		SET_SCENARIO_ATTRIBUTE(request, "CLUB_PLAYGROUP_SIZE", "2");
		SET_SCENARIO_ATTRIBUTE(request, "CUSTOM_CONTROLLER", "0");
		SET_SCENARIO_ATTRIBUTE(request, "DIVISION", "1");
		SET_SCENARIO_ATTRIBUTE(request, "DIVISION_THR", "OSDK_matchRelax");
		SET_SCENARIO_ATTRIBUTE(request, "GAMEGROUP_ONLY", "0");
		SET_SCENARIO_ATTRIBUTE(request, "GAME_MODE", "9");
		SET_SCENARIO_ATTRIBUTE(request, "GK_CONTROL", "abstain");
		SET_SCENARIO_ATTRIBUTE(request, "MATCHUP_HASH", "4252757");
		SET_SCENARIO_ATTRIBUTE(request, "MATCH_CLUB_TYPE", "0");
		SET_SCENARIO_ATTRIBUTE(request, "MATCH_GUESTS", "0");
		SET_SCENARIO_ATTRIBUTE(request, "NET_TOP", (uint64_t)1);
		SET_SCENARIO_ATTRIBUTE(request, "WOMEN_RULE", "0");
		request.setScenarioName("ProClubs_League_Match");
	}
	else if (mScenarioType == CLUBSCUP)
	{
		SET_SCENARIO_ATTRIBUTE(request, "CLUB_ID", clubIdString);
		SET_SCENARIO_ATTRIBUTE(request, "CLUB_LEAGUE", "16");
		SET_SCENARIO_ATTRIBUTE(request, "CLUB_MM_ANY", "1");
		SET_SCENARIO_ATTRIBUTE(request, "CLUB_MM_GK", "1");
		SET_SCENARIO_ATTRIBUTE(request, "CLUB_NUM_PLAYERS", "2");
		SET_SCENARIO_ATTRIBUTE(request, "CLUB_NUM_PLAYERS_THR", "OSDK_matchAny");
		SET_SCENARIO_ATTRIBUTE(request, "CUP", "12");
		SET_SCENARIO_ATTRIBUTE(request, "CUSTOM_CONTROLLER", "0");
		SET_SCENARIO_ATTRIBUTE(request, "GAMEGROUP_ONLY", "0");
		SET_SCENARIO_ATTRIBUTE(request, "GAME_MODE", "13");
		SET_SCENARIO_ATTRIBUTE(request, "GK_CONTROL", "abstain");
		SET_SCENARIO_ATTRIBUTE(request, "MATCHUP_HASH", "4356147");
		SET_SCENARIO_ATTRIBUTE(request, "MATCH_CLUB_TYPE", "0");
		SET_SCENARIO_ATTRIBUTE(request, "MATCH_GUESTS", "0");
		SET_SCENARIO_ATTRIBUTE(request, "NET_TOP", (uint64_t)1);
		SET_SCENARIO_ATTRIBUTE(request, "PPT", "0");
		SET_SCENARIO_ATTRIBUTE(request, "TOUR_TIER", "2");
		SET_SCENARIO_ATTRIBUTE(request, "TOUR_TIER_THR", "");
		SET_SCENARIO_ATTRIBUTE(request, "WOMEN_RULE", "0");
		request.setScenarioName("ProClubs_Cup_Match");
	}
	else if (mScenarioType == CLUBSPRACTICE)
	{
		SET_SCENARIO_ATTRIBUTE(request, "CLUB_ID", clubIdString);
		SET_SCENARIO_ATTRIBUTE(request, "CLUB_LEAGUE", "16");
		SET_SCENARIO_ATTRIBUTE(request, "CLUB_MM_ANY", "1");
		SET_SCENARIO_ATTRIBUTE(request, "CLUB_MM_GK", "1");
		SET_SCENARIO_ATTRIBUTE(request, "CLUB_NUM_PLAYERS", "2");
		SET_SCENARIO_ATTRIBUTE(request, "CLUB_NUM_PLAYERS_THR", "OSDK_matchAny");
		SET_SCENARIO_ATTRIBUTE(request, "CUP", "4");
		SET_SCENARIO_ATTRIBUTE(request, "CUSTOM_CONTROLLER", "0");
		SET_SCENARIO_ATTRIBUTE(request, "GAME_MODE", "13");
		SET_SCENARIO_ATTRIBUTE(request, "GK_CONTROL", "abstain");
		SET_SCENARIO_ATTRIBUTE(request, "MATCHUP_HASH", "3918126");
		SET_SCENARIO_ATTRIBUTE(request, "MATCH_CLUB_TYPE", "0");
		SET_SCENARIO_ATTRIBUTE(request, "MATCH_GUESTS", "0");
		SET_SCENARIO_ATTRIBUTE(request, "NET_TOP", (uint64_t)1);
		SET_SCENARIO_ATTRIBUTE(request, "PPT", "0");
		SET_SCENARIO_ATTRIBUTE(request, "TOUR_TIER", "1");
		SET_SCENARIO_ATTRIBUTE(request, "TOUR_TIER_THR", "");
		SET_SCENARIO_ATTRIBUTE(request, "WOMEN_RULE", "0");
		request.setScenarioName("ProClubs_Cup_Match");
	}

	/*char8_t buf[20240];
	BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "StartMatchmaking RPC : \n" << request.print(buf, sizeof(buf)));*/
	err = mPlayerInfo->getComponentProxy()->mGameManagerProxy->startMatchmakingScenario(request, response, criteriaError);
	if (err == ERR_OK)
	{
		mMatchmakingSessionId = response.getScenarioId();
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "ClubsPlayer::startMatchmakingScenario successful. mMatchmakingSessionId= " << mMatchmakingSessionId << " playerID= " << mPlayerInfo->getPlayerId());
	}
	else
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "ClubsPlayer::startMatchmakingScenario failed. Error(" << ErrorHelp::getErrorName(err) << " playerID= " << mPlayerInfo->getPlayerId());
	}
	return err;
}

BlazeRpcError ClubsPlayer::createOrJoinGame(GameGroupType groupName)
{
	BlazeRpcError result = ERR_OK;
	Blaze::GameManager::CreateGameRequest request;
	Blaze::GameManager::JoinGameResponse response;

	CommonGameRequestData& commonData = request.getCommonGameData();
	commonData.setDelineationGroup("");
	commonData.setGameType(GAME_TYPE_GROUP);
	commonData.setGameProtocolVersionString(getGameProtocolVersionString());

	if (!mPlayerData->isConnected())
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "[ClubsPlayer::createOrJoinGame::User Disconnected = " << mPlayerData->getPlayerId());
		return ERR_DISCONNECTED;
	}
	NetworkAddress& hostAddress = commonData.getPlayerNetworkAddress();
	//NetworkAddress *hostAddress = &request.getCommonGameData().getPlayerNetworkAddress();
	hostAddress.switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
	hostAddress.getIpPairAddress()->getExternalAddress().setIp(mPlayerInfo->getConnection()->getAddress().getIp());
	hostAddress.getIpPairAddress()->getExternalAddress().setPort(mPlayerInfo->getConnection()->getAddress().getPort());
	hostAddress.getIpPairAddress()->getExternalAddress().copyInto(hostAddress.getIpPairAddress()->getInternalAddress());

	request.getTournamentSessionData().setArbitrationTimeout(31536000000000);
	request.getTournamentSessionData().setForfeitTimeout(31449600000000);
	request.getTournamentSessionData().setTournamentDefinition("");
	request.getTournamentSessionData().setScheduledStartTime("1970-01-01T00:00:00Z"); // Need to use Time library
	char8_t* gameName;
	switch (groupName)
	{
	case HOMEGROUP:
	{
		gameName = mHomeGameName;
		request.getGameCreationData().setVoipNetwork(VoipTopology::VOIP_DISABLED);
		request.getGameCreationData().setMaxPlayerCapacity(50);
		request.getGameCreationData().setMinPlayerCapacity(1);
		request.getSlotCapacities().at(SLOT_PUBLIC_PARTICIPANT) = 50;
		break;
	}
	case VOIPGROUP:
	{
		gameName = mVoipGameName;
		request.getGameCreationData().setVoipNetwork(VoipTopology::VOIP_PEER_TO_PEER);
		request.getGameCreationData().setMaxPlayerCapacity(11);
		request.getGameCreationData().setMinPlayerCapacity(1);
		request.getSlotCapacities().at(SLOT_PUBLIC_PARTICIPANT) = 11;
		break;
	}
	case GAMEGROUP:
	{
		gameName = mGameGroupName;
		request.getGameCreationData().setVoipNetwork(VoipTopology::VOIP_DISABLED);
		request.getGameCreationData().setMaxPlayerCapacity(11);
		request.getGameCreationData().setMinPlayerCapacity(1);
		request.getSlotCapacities().at(SLOT_PUBLIC_PARTICIPANT) = 11;
		break;
	}
	default:
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "[ClubsPlayer::createOrJoinGame: In default state = " << mPlayerData->getPlayerId());
		return ERR_SYSTEM;
	}
	}
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "ClubsPlayer::createOrJoinGame:player= " << mPlayerData->getPlayerId() << " gameName " << gameName);

	request.setGamePingSiteAlias("");
	request.setGameEventAddress("");
	request.setGameEndEventUri("");

	Collections::AttributeMap& gameAttribs = request.getGameCreationData().getGameAttribs();
	gameAttribs["OSDK_gameMode"] = "";
	gameAttribs["Gamemode_Name"] = "";
	request.getGameCreationData().setDisconnectReservationTimeout(0);
	request.getGameCreationData().setGameModRegister(0);
	request.getGameCreationData().setPermissionSystemName("");
	//request.getGameCreationData().getExternalDataSourceApiNameList().clear();

	request.getGameCreationData().setGameName(gameName);
	request.getGameCreationData().getGameSettings().setBits(2071); //0x00000817
	request.getGameCreationData().setGameReportName("");
	request.getGameCreationData().setSkipInitializing(false);
	request.getGameCreationData().setNetworkTopology(Blaze::NETWORK_DISABLED);
	request.getGameCreationData().setPresenceMode(Blaze::PRESENCE_MODE_NONE);
	request.getGameCreationData().setPlayerReservationTimeout(0);
	request.getGameCreationData().setQueueCapacity(0);
	request.getGameCreationData().getExternalSessionStatus().setUnlocalizedValue("");
	request.getGameCreationData().setExternalSessionTemplateName("");
	request.getGameCreationData().getTeamIds().push_back(65534);

	request.setGameStartEventUri("");
	request.setGameEndEventUri("");
	request.setGameEventContentType("application/json");
	request.setGameReportName("");
	request.setGameStatusUrl("");
	request.getMeshAttribs().clear();
	request.getSlotCapacitiesMap().clear();
	request.setServerNotResetable(false);

	request.getSlotCapacities().at(SLOT_PRIVATE_PARTICIPANT) = 0;
	request.getSlotCapacities().at(SLOT_PUBLIC_SPECTATOR) = 0;
	request.getSlotCapacities().at(SLOT_PRIVATE_SPECTATOR) = 0;

	request.setPersistedGameId(gameName);

	request.getPlayerJoinData().setGroupId(mPlayerData->getConnGroupId());
	request.getPlayerJoinData().setGameEntryType(GAME_ENTRY_TYPE_DIRECT);
	request.getPlayerJoinData().setDefaultRole("");
	Blaze::GameManager::PerPlayerJoinData * data = request.getPlayerJoinData().getPlayerDataList().pull_back();
	data->setEncryptedBlazeId("");
	data->setIsOptionalPlayer(false);
	data->setSlotType(INVALID_SLOT_TYPE);
	data->setTeamId(65535);
	data->setTeamIndex(65535);
	data->setRole("");
	data->getPlayerAttributes().clear();
	/*	if (StressConfigInst.getPlatform() == Platform::PLATFORM_XONE)
		{
			data->getUser().setPersonaNamespace("xbox");
		}
		else
		{
			data->getUser().setPersonaNamespace("ps3");
		}*/
	data->getUser().setAccountId(0);	// mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId()
	data->getUser().setBlazeId(mPlayerData->getPlayerId());
	data->getUser().setName(mPlayerData->getPersonaName().c_str());
	data->getUser().setExternalId(mPlayerData->getExternalId());
	data->getUser().setOriginPersonaId(0);
	data->getUser().setPidId(0);

	if (StressConfigInst.getPlatform() == PLATFORM_XONE)
	{
		data->getUser().setPersonaNamespace("xbox");
		data->getUser().getPlatformInfo().setClientPlatform(Blaze::ClientPlatformType::xone);
	}
	else if (StressConfigInst.getPlatform() == PLATFORM_PS4)
	{
		data->getUser().setPersonaNamespace("ps3");
		data->getUser().getPlatformInfo().setClientPlatform(Blaze::ClientPlatformType::ps4);
	}
	request.getPlayerJoinData().setDefaultSlotType(Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT);
	request.getPlayerJoinData().setDefaultTeamId(65534);
	request.getPlayerJoinData().setDefaultTeamIndex(0);

	request.getTournamentIdentification().setTournamentId("");
	request.getTournamentIdentification().setTournamentOrganizer("");

	/*char8_t buf[20240];
	BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "CreateOrJoinGame RPC : \n" << request.print(buf, sizeof(buf)));*/

	result = mPlayerData->getComponentProxy()->mGameManagerProxy->createOrJoinGame(request, response);
	if (result == ERR_OK)
	{
		switch (groupName)
		{
		case HOMEGROUP:
		{
			mHomeGroupGId = response.getGameId();
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "ClubsPlayer::createOrJoinGame:player= " << mPlayerData->getPlayerId() << " received HomeGroupGId= " << mHomeGroupGId);
			break;
		}
		case VOIPGROUP:
		{
			mVoipGroupGId = response.getGameId();
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "ClubsPlayer::createOrJoinGame:player= " << mPlayerData->getPlayerId() << " received VoipGroupGId= " << mVoipGroupGId);
			break;
		}
		case GAMEGROUP:
		{
			mGamegroupGId = response.getGameId();
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "ClubsPlayer::createOrJoinGame:player= " << mPlayerData->getPlayerId() << " received GamegroupGId= " << mGamegroupGId);
			break;
		}
		default:
		{
			BLAZE_ERR_LOG(BlazeRpcLog::util, "[ClubsPlayer::createOrJoinGame: In default state = " << mPlayerData->getPlayerId());
			break;
		}
		}
	}
	else
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "ClubsPlayer::createOrJoinGame: failed with error " << ErrorHelp::getErrorName(result));
	}
	return result;
}

BlazeRpcError ClubsPlayer::createOrJoinClubs()
{
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::createOrJoinClubs] " << mPlayerData->getPlayerId());
	BlazeRpcError result = ERR_OK;

	//getClubMembershipForUsers
	Blaze::Clubs::GetClubMembershipForUsersResponse clubMembershipForUsersResponse;
	result = getClubMembershipForUsers(mPlayerInfo, clubMembershipForUsersResponse);

	Blaze::Clubs::ClubMemberships* memberShips = clubMembershipForUsersResponse.getMembershipMap()[mPlayerInfo->getPlayerId()];
	if (memberShips == NULL)
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "ClubsPlayer::createOrJoinClubs:getClubsComponentSettings() Response returned empty object " << mPlayerInfo->getPlayerId());
	}
	else
	{
		Blaze::Clubs::ClubMembershipList::iterator it = memberShips->getClubMembershipList().begin();
		setMyClubId((*it)->getClubId());
		//Player has already subsribed to a club - return here
		if (getMyClubId() != 0)
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "ClubsPlayer::simulateLoad:player= " << mPlayerData->getPlayerId() << " received clubID from getClubMembershipForUsers. " << getMyClubId());
			return result;
		}
	}

	//Player doesn't have any club, so create or join existing club
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
				return result;
			}
			//else
			//ERR_OK
			//{
			//	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::createOrJoinClubs]:joinClub successful " << getMyClubId() << " " << mPlayerData->getPlayerId());
			//	//increment member count
			//	//if count=11 remove from map
			//	if(iter->second >= 11)
			//	{
			//		clubsListReference->erase(iter);	
			//		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::createOrJoinClubs]:Removed clubID from map " << getMyClubId() << " " << mPlayerData->getPlayerId());
			//	}				
			//}
			//unlock
			//mMutex.Unlock();
		}
		////remove from the map
		//else
		//{
		//	mMutex.Lock(EA::Thread::RWMutex::kLockTypeRead);
		//	clubsListReference->erase(iter);	
		//	mMutex.Unlock();
		//	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::createOrJoinClubs]:Removed clubID from map " << getMyClubId() );
		//}
		iter++;
	}
	//unlock
	mMutex.Unlock();

	//No clubs exist in the map, create own club
	if (getMyClubId() == 0)
	{
		CreateClubResponse response;
		result = createClub(response);
		if (result != ERR_OK)
		{
			BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::createOrJoinClubs:createClub] " << mPlayerData->getPlayerId() << ErrorHelp::getErrorName(result));
			return result;
		}
		else
		{
			//setOwnClubID
			setMyClubId(response.getClubId());
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::createOrJoinClubs]:createClub successful-adding clubID to map " << getMyClubId());
			mMutex.Lock();
			//Add clubID to map
			(*clubsListReference)[getMyClubId()] = 1;
			mMutex.Unlock();
		}
	}
	return result;
}

BlazeRpcError ClubsPlayer::createClub(CreateClubResponse& response)
{
	BlazeRpcError err = ERR_SYSTEM;
	CreateClubRequest request;
	//Club Name
	eastl::string clubName;
	uint64_t clubsNameIndex;
	clubsNameIndex = (mPlayerInfo->getPlayerId()) % 1000000000;
	clubName.sprintf(CLUBNAME.c_str(), clubsNameIndex);
	//use CLUBS_CONFIG.ClubsNameIndex if required for clubs name 
	BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[ClubsPlayer::createClub]: [" << mPlayerInfo->getPlayerId() << " Create Clubs Name: " << clubName);
	request.setName(clubName.c_str());

	ClubSettings &clubSettings = request.getClubSettings();
	clubSettings.setNonUniqueName(clubName.c_str());
	clubSettings.setDescription(clubName.c_str());
	clubSettings.setPetitionAcceptance(CLUB_ACCEPT_ALL);

	clubSettings.getAcceptanceFlags().setBits(2);

	clubSettings.setJoinAcceptance(CLUB_ACCEPT_ALL);

	eastl::string lang;
	lang.sprintf("%c%c", Blaze::Random::getRandomNumber(20) + 'A', Blaze::Random::getRandomNumber(20) + 'A');
	clubSettings.setLanguage(lang.c_str());

	clubSettings.setRegion(Blaze::Random::getRandomNumber(20));
	clubSettings.setSeasonLevel(CLUBS_CONFIG.SeasonLevel); //1

	clubSettings.getCustClubSettings().setCustOpt5(1000);
	clubSettings.setTeamId(8); //8
	clubSettings.setLogoId(0); //0
	clubSettings.setSkipPasswordCheckAcceptance(CLUB_ACCEPT_ALL);

	request.setClubDomainId(CLUBS_CONFIG.ClubDomainId); //10

	//char8_t buf[20240];	BLAZE_TRACE_RPC_LOG(BlazeRpcLog::gamemanager, "CreateClub RPC : \n" << request.print(buf, sizeof(buf)));

	err = mPlayerInfo->getComponentProxy()->mClubsProxy->createClub(request, response);
	if (err == ERR_OK)
	{
		BLAZE_INFO_LOG(BlazeRpcLog::clubs, "ClubsPlayer::createClub: Player ID = " << mPlayerInfo->getPlayerId() << " ClubId = " << response.getClubId());
	}
	else
	{
		BLAZE_ERR_LOG(BlazeRpcLog::clubs, "ClubsPlayer::createClub: Player ID = " << mPlayerInfo->getPlayerId() << " error = " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError ClubsPlayer::createPracticeClub(CreateClubResponse& response)
{
	BlazeRpcError err = ERR_SYSTEM;
	CreateClubRequest request;
	request.getTagList().push_back("PosGK");
	request.getTagList().push_back("PosDEF");
	request.getTagList().push_back("PosMID");
	request.getTagList().push_back("PosFOR");
	//Club Name
	eastl::string clubName;
	uint64_t clubsNameIndex;
	clubsNameIndex = (mPlayerInfo->getPlayerId()) % 1000000000;
	clubName.sprintf(CLUBNAME.c_str(), clubsNameIndex);
	//use CLUBS_CONFIG.ClubsNameIndex if required for clubs name 
	BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[ClubsPlayer::createClub]: [" << mPlayerInfo->getPlayerId() << " Create Clubs Name: " << clubName);
	request.setName(clubName.c_str());

	ClubSettings &clubSettings = request.getClubSettings();
	clubSettings.setArtPackageType(CLUBS_ART_UNKNOWN);
	clubSettings.setBannerId(0);
	clubSettings.getClubArtSettingsFlags().setBits(1);
	clubSettings.getAcceptanceFlags().setBits(1);
	clubSettings.getCustClubSettings().setCustOpt1(0);
	clubSettings.getCustClubSettings().setCustOpt2(0);
	clubSettings.getCustClubSettings().setCustOpt3(0);
	clubSettings.getCustClubSettings().setCustOpt4(395);
	clubSettings.getCustClubSettings().setCustOpt5(0);
	clubSettings.setMetaData2("");
	clubSettings.setDescription("");
	clubSettings.getMetaDataUnion().setMetadataString("<StadName value=\"Municipal de Ipurua\" /><KitId value=\"2359296\" /><CustomKitId value=\"7509\" /><KitColor1 value=\"14219901\" /><KitColor2 value=\"11767040\" /><KitColor3 value=\"707566\" /><CustomAwayKitId value=\"7509\" /><KitAColor1 value=\"13179675\" /><KitAColor2 value=\"14219901\" /><KitAColor3 value=\"13179675\" /><DCustomKit value=\"0\" /><CrestColor value=\" - 1\" /><CrestAssetId value=\"99040107\" /><IsCustomTeam value=\"0\" />");
	clubSettings.setMetaData("");
	clubSettings.setRegion(4344147);
	clubSettings.setHasPassword(false);
	clubSettings.setIsNameProfane(false);
	clubSettings.setJoinAcceptance(CLUB_ACCEPT_ALL);
	eastl::string lang;
	lang.sprintf("%c%c", Blaze::Random::getRandomNumber(20) + 'A', Blaze::Random::getRandomNumber(20) + 'A');
	clubSettings.setLanguage(lang.c_str());
	clubSettings.setLogoId(0); //0
	clubSettings.setLastUpdatedBy(0);
	clubSettings.setLastSeasonLevelUpdate(0);
	clubSettings.getMetaDataUnion().setMetadataString("");
	clubSettings.setClubName("");
	clubSettings.setNonUniqueName("FUL");
	clubSettings.setPreviousSeasonLevel(0);
	clubSettings.setPassword("");
	clubSettings.setPetitionAcceptance(CLUB_ACCEPT_ALL);
	clubSettings.setSeasonLevel(CLUBS_CONFIG.SeasonLevel); //1
	clubSettings.setSkipPasswordCheckAcceptance(CLUB_ACCEPT_ALL);
	clubSettings.setTeamId(8); //8
	request.setClubDomainId(CLUBS_CONFIG.ClubDomainId); //10

	//char8_t buf[20240];
	//BLAZE_TRACE_RPC_LOG(BlazeRpcLog::gamemanager, "CreatePracticeClub RPC : \n" << request.print(buf, sizeof(buf)));

	err = mPlayerInfo->getComponentProxy()->mClubsProxy->createClub(request, response);
	if (err == ERR_OK)
	{
		BLAZE_INFO_LOG(BlazeRpcLog::clubs, "ClubsPlayer::createClub: Player ID = " << mPlayerInfo->getPlayerId() << " ClubId = " << response.getClubId());
	}
	else
	{
		BLAZE_ERR_LOG(BlazeRpcLog::clubs, "ClubsPlayer::createClub: Player ID = " << mPlayerInfo->getPlayerId() << " error = " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError ClubsPlayer::joinClub(ClubId clubID)
{
	BlazeRpcError err = ERR_OK;
	JoinOrPetitionClubRequest request;
	JoinOrPetitionClubResponse response;

	BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[" << mPlayerInfo->getPlayerId() << "][ClubsPlayer::joinClub]:for clubID= " << clubID);

	request.setClubId(clubID);
	request.setPetitionIfJoinFails(true);
	if (mScenarioType == CLUBSPRACTICE)
	{
		request.setPassword("");
	}
	err = mPlayerInfo->getComponentProxy()->mClubsProxy->joinOrPetitionClub(request, response);
	ClubJoinOrPetitionStatus joinStatus = response.getClubJoinOrPetitionStatus();
	if( err == ERR_OK && joinStatus == CLUB_JOIN_SUCCESS)
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[" << mPlayerInfo->getPlayerId() << "][ClubsPlayer::joinClub]:success for clubID= " << clubID);
		//sendClubMesasge(false,PlayerInfo);
	}
	//CLUBS_ERR_CLUB_FULL
	else
	{
		BLAZE_ERR_LOG(BlazeRpcLog::clubs, "[ClubsPlayer::JoinClub]: Player [" << mPlayerInfo->getPlayerId() << ". ERROR " << ErrorHelp::getErrorName(err) << " :clubID " << clubID );
	}
	return err;
}

bool ClubsPlayer::isTargetTeamSizeReached()
{
	//Fix Team Size
	mTargetTeamSize = CLUBS_CONFIG.getRandomTeamSize();
	BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "ClubsPlayer::isTargetTeamSizeReached:Target Team Size=  " << mTargetTeamSize << " for " << mPlayerInfo->getPlayerId());

	//Wait for other players to join the game group till team size or timeout reached
	uint32_t waitCounter = CLUBS_CONFIG.PlayerWaitingCounter;
	while (waitCounter > 0 )
	{
		if ( mPlayerInGameGroupMap.size() == mTargetTeamSize)
		{
			//reached target team size 
			BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "ClubsPlayer::isTargetTeamSizeReached:Team Size reached=  " << mTargetTeamSize << " for " << mPlayerInfo->getPlayerId());
			break;
		}
		BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "ClubsPlayer::isTargetTeamSizeReached:current map size  " << mPlayerInGameGroupMap.size() << " expected is  " << mTargetTeamSize);
		--waitCounter;
		sleep(15000); // 15 seconds
	}
	BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "ClubsPlayer::isTargetTeamSizeReached:target size  " << mTargetTeamSize << " gamegroup size " << mPlayerInGameGroupMap.size() << " for " << mPlayerInfo->getPlayerId());

	if(mPlayerInGameGroupMap.size() == mTargetTeamSize)
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "ClubsPlayer::isTargetTeamSizeReached:target size reached " << mTargetTeamSize << " for " << mPlayerInfo->getPlayerId());
		return true;
	}
	//if min team size reached
	else if(mPlayerInGameGroupMap.size() != mTargetTeamSize && mPlayerInGameGroupMap.size() >= CLUBS_CONFIG.MinTeamSize)
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "ClubsPlayer::isTargetTeamSizeReached:Changing target size to " << mTargetTeamSize << " for " << mPlayerInfo->getPlayerId());
		mTargetTeamSize = mPlayerInGameGroupMap.size();
		return true;
	}
	//MinTeamSize not reached, return false
	BLAZE_ERR_LOG(BlazeRpcLog::clubs, "ClubsPlayer::isTargetTeamSizeReached:MinTeamSize not reached for " << mPlayerInfo->getPlayerId());
	return false;
}

BlazeRpcError ClubsPlayer::tournamentCalls()
{
	OSDKTournaments::GetMyTournamentIdResponse getTournamentIdResponse;
	BlazeRpcError err = ERR_OK;
	OSDKTournaments::GetTournamentsResponse getTournamentsResponse;
	OSDKTournaments::TournamentId tournamentId = 0;
	err = getMyTournamentId(mPlayerInfo, Blaze::OSDKTournaments::MODE_FIFA_CUPS_CLUB, getTournamentIdResponse, getMyClubId());
	if (err == ERR_OK)
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::lobbyRpcCalls]:" << mPlayerData->getPlayerId() << " getMyTournamentId call 2 passed ");
		tournamentId = getTournamentsResponse.getTournamentList().at(0)->getTournamentId();
		getTournaments(mPlayerInfo, OSDKTournaments::TournamentModes::MODE_FIFA_CUPS_CLUB, getTournamentsResponse);
	}
	else if (err == OSDKTOURN_ERR_MEMBER_NOT_IN_TOURNAMENT)
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::initializeCupAndTournments]:" << mPlayerData->getPlayerId() << " getMyTournamentId call 2 faled but calling getTournaments");
		err = getTournaments(mPlayerInfo, Blaze::OSDKTournaments::MODE_FIFA_CUPS_CLUB, getTournamentsResponse);
		if (err == ERR_OK && getTournamentsResponse.getTournamentList().size() > 0)
		{
			// Means that the player is not yet into any tournaments, so get the tournament id first and join into that;
			tournamentId = getTournamentsResponse.getTournamentList().at(0)->getTournamentId();
			// Join Tournament	
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::initializeCupAndTournments]:" << mPlayerData->getPlayerId() << " getMyTournamentId call 2 successfuly got tournamnet id");
			err = joinTournament(mPlayerInfo, tournamentId);
		}
		setTournamentId(tournamentId);
	}
	if (err == ERR_OK)
	{
		setTournamentId(tournamentId);
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "ClubsPlayer::tournamentRpcCalls:player= " << mPlayerData->getPlayerId() << " TournamentId is set. " << getTournamentId());
		if (!getTournamentIdResponse.getIsActive())
		{
			//if the player is not in the tournament means that he might have lost/won the last tournament, so resetting here..
			osdkResetTournament(mPlayerInfo, getTournamentId(), Blaze::OSDKTournaments::MODE_FIFA_CUPS_CLUB);
		}
	}
	else
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "[ClubsPlayer::tournamentRpcCalls::TournamentId is Not Set for " << mPlayerData->getPlayerId());
	}
	if (err != OSDKTOURN_ERR_MEMBER_NOT_IN_TOURNAMENT)
	{
		OSDKTournaments::GetTournamentsResponse getTournamentResponse;
		getTournaments(mPlayerInfo, Blaze::OSDKTournaments::MODE_FIFA_CUPS_CLUB, getTournamentResponse);	//	mode = 5
	}
	lookupRandomUsersByPersonaNames(mPlayerInfo);
	OSDKTournaments::GetMyTournamentDetailsResponse getMyTournamentDetailsResponse;
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::initializeCupAndTournments]:" << mPlayerData->getPlayerId() << " getMyTournamentDetails call 1 ");
	getMyTournamentDetails(mPlayerInfo, Blaze::OSDKTournaments::MODE_FIFA_CUPS_CLUB, 13 /*getTournamentId()*/, getMyTournamentDetailsResponse, getMyClubId());
	return err;
}

}  // namespace Stress
}  // namespace Blaze
