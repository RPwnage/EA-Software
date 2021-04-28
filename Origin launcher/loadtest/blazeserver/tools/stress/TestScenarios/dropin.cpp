//  *************************************************************************************************
//
//   File:    dropin.cpp
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#include "dropin.h"
#include "playermodule.h"
#include "gamesession.h"
#include "utility.h"
#include "reference.h"
#include "framework/util/shared/base64.h"
#include "gamemanager/tdf/matchmaker.h"
#include "framework/tdf/entrycriteria.h"
#include "osdksettings/tdf/osdksettingstypes.h"
//#include "mail/tdf/mail.h"

namespace Blaze {
namespace Stress {

//constants
//#define					SEASON_ID   1     //osdk season
//#define					SPORTS_SEASON_ID   2    //sports season


DropinPlayer::DropinPlayer(StressPlayerInfo* playerData)
    : GamePlayUser(playerData),
	DropinPlayerGameSession(playerData, 2 /*gamesize*/, CLIENT_SERVER_DEDICATED /*Topology*/,
                             StressConfigInst.getGameProtocolVersionString() /*GameProtocolVersionString*/),
      mIsMarkedForLeaving(false)
{
}

	DropinPlayer::~DropinPlayer()
{
    BLAZE_INFO(BlazeRpcLog::util, "[DropinPlayer][Destructor][%" PRIu64 "]: dropin destroy called", mPlayerInfo->getPlayerId());
    stopReportTelemtery();
    //  deregister async handlers to avoid client crashes when disconnect happens
    deregisterHandlers();
    //  wait for some time to allow already scheduled jobs to complete
    //sleep(30000);
}

void DropinPlayer::deregisterHandlers()
{
	DropinPlayerGameSession::deregisterHandlers();
}


BlazeRpcError DropinPlayer::simulateLoad()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[dropinPlayer::simulateLoad]:" << mPlayerData->getPlayerId());
	BlazeRpcError err = ERR_OK;
	uint32_t viewId = mPlayerInfo->getViewId();
	err = simulateMenuFlow();
	lobbyRpcCalls();
	//DROPIN
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[dropinPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " DROPIN startMatchmakingScenario");
	err = startMatchmakingScenario(mScenarioType);
	
	//setClientState(mPlayerInfo, Blaze::ClientState::Mode::MODE_MULTI_PLAYER);
	//unsubscribeFromCensusData(mPlayerInfo);

	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[dropinPlayer::simulateLoad]: DROPIN-startMatchmaking failed " << mPlayerData->getPlayerId());
		sleep(30000);
		return err;
	}
	
	//int gameAttributecount = 0;

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
            BLAZE_ERR_LOG(BlazeRpcLog::util, "[dropinPlayer::simulateLoad::User Disconnected = " << mPlayerData->getPlayerId());
            return ERR_DISCONNECTED;
        }
        if (playerStateTracker != getPlayerState())  //  state changed
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[dropinPlayer::simulateLoad:playerStateTracker]" << mPlayerData->getPlayerId() << " Time Spent in Sate: " << GameStateToString(
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
                if (playerStateDurationMs > DROPIN_CONFIG.maxPlayerStateDurationMs)
                {
                    BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[dropinPlayer::simulateLoad:playerStateTracker][" << mPlayerData->getPlayerId() << "]: Player exceded maximum Game state Duration:" << playerStateDurationMs <<
                                   " ms" << " Current Game State:" << GameStateToString(getPlayerState()));
                    if (IN_GAME == getPlayerState())
                    {
                        mIsMarkedForLeaving = true;   //  Player exceed maximum duration in InGAME State so mark this Player to Leave from InGame
                    }
                }
            }
        }
        const char8_t * stateMsg = GameStateToString(getPlayerState());
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[dropinPlayer::simulateLoad]: Player: " << mPlayerInfo->getPlayerId() << ", GameId: " << myGameId << " Current state : " << stateMsg << " InStateDuration:" <<
                       playerStateDurationMs);
        switch (getPlayerState())
        {
            //   Sleep for all the below states
            case NEW_STATE:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[dropinPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=NEW_STATE");
                    stateLimit++;
                    if (stateLimit >= DROPIN_CONFIG.GameStateIterationsMax)
                    {
                        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[dropinPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=NEW_STATE for long time and return");
                        return err;
                    }
                    //  Wait here if resetDedicatedserver() called/player not found for invite.
                    sleep(DROPIN_CONFIG.GameStateSleepInterval);
                    break;
                }
            case PRE_GAME:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[dropinPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=PRE_GAME");
					
					//leave voip group
					//TODO - add again
					//leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mVoipGroupGId);
					setClientState(mPlayerInfo, ClientState::MODE_MULTI_PLAYER);
					//validateString(mPlayerInfo);
					//Filter for profanity in pre game now
					//filterForProfanity
					FilterUserTextResponse filterReq; //Fill this request TODO
					FilteredUserText* filteredText1 = BLAZE_NEW FilteredUserText();
					filteredText1->setResult(Blaze::Util::FILTER_RESULT_PASSED);
					filteredText1->setFilteredText("Hugh");
					filterReq.getFilteredTextList().push_back(filteredText1);
					FilteredUserText* filteredText2 = BLAZE_NEW FilteredUserText();
					filteredText2->setResult(Blaze::Util::FILTER_RESULT_PASSED);
					filteredText2->setFilteredText("");
					filterReq.getFilteredTextList().push_back(filteredText2);
					FilteredUserText* filteredText3 = BLAZE_NEW FilteredUserText();
					filteredText3->setResult(Blaze::Util::FILTER_RESULT_PASSED);
					filteredText3->setFilteredText("Brown");
					filterReq.getFilteredTextList().push_back(filteredText3);
					FilteredUserText* filteredText4 = BLAZE_NEW FilteredUserText();
					filteredText4->setResult(Blaze::Util::FILTER_RESULT_PASSED);
					filteredText4->setFilteredText("H.Brown");
					filterReq.getFilteredTextList().push_back(filteredText4);
					FilteredUserText* filteredText5 = BLAZE_NEW FilteredUserText();
					filteredText5->setResult(Blaze::Util::FILTER_RESULT_PASSED);
					filteredText5->setFilteredText("");
					filterReq.getFilteredTextList().push_back(filteredText5);
					FilteredUserText* filteredText6 = BLAZE_NEW FilteredUserText();
					filteredText6->setResult(Blaze::Util::FILTER_RESULT_UNPROCESSED);
					filteredText6->setFilteredText("");
					filterReq.getFilteredTextList().push_back(filteredText6);
					//filterForProfanity
					filterForProfanity(mPlayerInfo, &filterReq);

					//fetchLoadOuts(mPlayerInfo); // add second player as well
					if (!isPlatformHost) {
						setPlayerAttributes("REQ", "1", myGameId);
						setPlayerAttributes("REQ", "2", myGameId);
					}

					getStatGroup(mPlayerInfo, "VProAveMatchRating");
					getStatsByGroupAsync(mPlayerInfo, "VProAveMatchRating");
					mPlayerInfo->setViewId(++viewId);

					fetchLoadOuts(mPlayerInfo);

					Blaze::BlazeIdList friendList;
					friendList.push_back(mPlayerInfo->getPlayerId());
					//friendList.push_back(getOpponentPlayerInfo().getPlayerId());
					getStatsByGroupAsync(mPlayerInfo, friendList, "VProAttributeXP");

					if (isPlatformHost)
					{	
							setGameSettingsMask(mPlayerInfo, myGameId, 0, 2063);
							sleep(5000);
							advanceGameState(IN_GAME);
					}
                    sleep(DROPIN_CONFIG.GameStateSleepInterval);
                    break;
                }
            case IN_GAME:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[dropinPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=IN_GAME");

					if (!ingame_enter)
                    {
                        ingame_enter = true;
                        //simulateInGame();
						//Start InGame Timer	
						if(isPlatformHost)
						{
							BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[dropinPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " setInGameStartTime. gameID=" << myGameId);
							setInGameStartTime();
						}
                    }
					setClientState(mPlayerInfo, Blaze::ClientState::MODE_MULTI_PLAYER, Blaze::ClientState::STATUS_SUSPENDED);
					if (mCensusdataSubscriptionStatus)
					{
						err = unsubscribeFromCensusData(mPlayerInfo);
						if (err == ERR_OK)
						{
							UnsetSubscriptionSatatus();
						}
					}
                    if (RollDice(DROPIN_CONFIG.inGameReportTelemetryProbability))
                    {
						//10% probability setGameAttributes - if platformhost
                        executeReportTelemetry();
                    }
					/*if(isPlatformHost)
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
					}*/

			if ((isPlatformHost && IsInGameTimedOut()) || (mIsMarkedForLeaving == true))
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[dropinPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " IsInGameTimedOut/mIsMarkedForLeaving=true. gameID=" << myGameId);
				advanceGameState(POST_GAME);
				sleep(1000);
			}

			sleep(DROPIN_CONFIG.GameStateSleepInterval);
			break;
				}
            case INITIALIZING:
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[dropinPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=INITIALIZING");
				//if (isPlatformHost)
				//{
				//	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[dropinPlayer::simulateLoad][" << mPlayerData->getPlayerId() << "calling advanceGameState");
				//	//Move this to DS -TODO
				//	advanceGameState(PRE_GAME);
				//}
				sleep(DROPIN_CONFIG.GameStateSleepInterval);
				break;
			}

			case POST_GAME:
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[dropinPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=POST_GAME");

				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[dropinPlayer::simulateLoad][" << mPlayerData->getPlayerId() << "]: submitGameReport");
				//submitGameReport();
				//invokeUdatePrimaryExternalSessionForUser(mPlayerInfo, myGameId, "", "", myGameId, GAME_LEAVE, GAME_TYPE_GAMESESSION, ACTIVE_CONNECTED);
				updatePrimaryExternalSessionForUserRequest(mPlayerInfo, myGameId, myGameId, UPDATE_PRESENCE_REASON_GAME_LEAVE, GAME_TYPE_GAMESESSION, ACTIVE_CONNECTED, PRESENCE_MODE_STANDARD, myGameId);
				//leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT);
				//BLAZE_INFO(BlazeRpcLog::gamemanager, "[dropinPlayer::leaveGame][%" PRIu64 "]: player left game: GameId = %" PRIu64 "", mPlayerData->getPlayerId(), myGameId);
				//meshEndpointsDisconnected RPC is initiated from NOTIFY_NOTIFYPLAYERREMOVED

				setClientState(mPlayerInfo, Blaze::ClientState::MODE_SINGLE_PLAYER, Blaze::ClientState::STATUS_NORMAL);
				//postGameCalls();

				setPlayerState(DESTRUCTING);

				sleep(DROPIN_CONFIG.GameStateSleepInterval);
				break;
			}
			case DESTRUCTING:
			{
				//  Player Left Game
				//  Presently using this state when user is removed from the game and return from this function.
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[dropinPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=DESTRUCTING");
				sleep(20000);
				setClientState(mPlayerInfo, ClientState::MODE_MENU);
				//  Return here
				return err;
			}
			case MIGRATING:
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[dropinPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=MIGRATING");
				//  mGameData will be freed when NOTIFY_NOTIFYGAMEREMOVED notification comes.
				sleep(10000);
				break;
			}
			case GAME_GROUP_INITIALIZED:
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[dropinPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=GAME_GROUP_INITIALIZED");
				sleep(5000);
				break;
			}
			case INACTIVE_VIRTUAL:
			default:
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[dropinPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=" << getPlayerState());
				//  shouldn't come here
				return err;
			}
		}
	}
}

void DropinPlayer::addPlayerToList()
{
    //  add player to universal list first
    Player::addPlayerToList();
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[dropinPlayer::addPlayerToList]: Player: " << mPlayerInfo->getPlayerId() << ", Game: " << myGameId);
    {
        ((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->dropinPlayerMap.insert(eastl::pair<PlayerId, eastl::string>(mPlayerInfo->getPlayerId(), mPlayerInfo->getPersonaName()));
    }
}

void DropinPlayer::removePlayerFromList()
{
    //  remove player from universal list first
    Player::removePlayerFromList();
    PlayerId  id = mPlayerInfo->getPlayerId();
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[dropinPlayer::removePlayerFromList]: Player: " << id << ", Game: " << myGameId);
    PlayerMap& playerMap = ((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->dropinPlayerMap;
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

void DropinPlayer::simulateInGame()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][DROPINPlayer]: In Game simulation:" << myGameId);
    
	setClientState(mPlayerInfo, ClientState::MODE_MULTI_PLAYER, ClientState::Status::STATUS_SUSPENDED);
	if(	mCensusdataSubscriptionStatus)
	{
		BlazeRpcError err;
		err = unsubscribeFromCensusData(mPlayerInfo);
		if (err == ERR_OK)
		{
			UnsetSubscriptionSatatus();
		}
	}
	
	if(isPlatformHost)
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
	}
	setPlayerAttributes("RDY", "0", myGameId);

	delayRandomTime(2000, 5000);
}

void DropinPlayer::lobbyRpcCalls()
{
	//BlazeRpcError err = ERR_OK;
	Blaze::BlazeIdList friendList;
	friendList.push_back(mPlayerInfo->getPlayerId());
	Blaze::PersonaNameList personaNameList;
	personaNameList.push_back(""); 
	FetchConfigResponse configResponse;
	Blaze::Clubs::GetMembersResponse clubResponse;
	uint32_t viewId = mPlayerInfo->getViewId();

	/*if (isTopologyHost)
	{
		friendList.push_back(mOpponentPlayerInfo.getPlayerId());
	}*/

	// Set the club id or join the club before getting the club
	if (getMyClubId() == 0)
	{
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
		//GlobalClubsMap* clubsListReference = ((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->getCreatedClubsMap();
		//GlobalClubsMap::iterator iter = clubsListReference->begin();

		//BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::createOrJoinClubs]:CLUBS_CONFIG.ClubSizeMax= " << CLUBS_CONFIG.ClubSizeMax << " " << mPlayerInfo->getPlayerId() );

		//Check if club has max players CLUBS_CONFIG.ClubSizeMax [11]
		//if space available join the club and increment count
		//if max players reached remove from the map

		//lock
		//mMutex.Lock();
		//while (iter != clubsListReference->end() && getMyClubId() == 0)
		//{
		//	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::createOrJoinClubs]:external loop ClubID= " << iter->first << "  member count= " << iter->second << " " << mPlayerInfo->getPlayerId());
		//	if (iter->second < CLUBS_CONFIG.ClubSizeMax)
		//	{
		//		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::createOrJoinClubs]: Inside loop ClubID= " << iter->first << "  member count= " << iter->second << " " << mPlayerInfo->getPlayerId());
		//		setMyClubId(iter->first);
		//		++(iter->second);
		//		//join club
		//		result = joinClub(getMyClubId());
		//		//CLUBS_ERR_CLUB_FULL
		//		if (result == CLUBS_ERR_CLUB_FULL)
		//		{
		//			//erase the clubID from map
		//			//clubsListReference->erase(iter);	
		//			iter->second = CLUBS_CONFIG.ClubSizeMax;
		//			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::createOrJoinClubs]:CLUBS_ERR_CLUB_FULL:Removed clubID from map " << getMyClubId());
		//			setMyClubId(0);
		//		}
		//		else if (result != ERR_OK)
		//		{
		//			BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::createOrJoinClubs:joinClub failed ] " << mPlayerData->getPlayerId() << " " << ErrorHelp::getErrorName(result));
		//			--(iter->second);
		//			//unlock
		//			mMutex.Unlock();
		//			setMyClubId(0);
		//			//return result;
		//		}
		//	}
		//	iter++;
		//}
		////unlock
		//mMutex.Unlock();
	}

	if (getMyClubId() > 0)
	{
		GetMembersResponse getMembersResponse;
		getMembers(mPlayerInfo, getMyClubId(), 50, getMembersResponse);
	}

	lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
	lookupUsersByPersonaNames(mPlayerInfo, personaNameList);

	//getStatGroup(mPlayerInfo, "H2HPreviousSeasonalPlay");
	//getStatsByGroupAsync(mPlayerInfo, "H2HPreviousSeasonalPlay");
	//lookupUsersByPersonaNames(mPlayerInfo, "");
	//getStatGroup(mPlayerInfo, "H2HSeasonalPlay");
	//getStatsByGroupAsync(mPlayerInfo, "H2HSeasonalPlay");
	//fetchClientConfig(mPlayerInfo, "FIFA_H2H_SEASONALPLAY", configResponse);
	//FifaCups::CupInfo getCupsResponse;
	//getCupInfo(mPlayerInfo, FifaCups::FIFACUPS_MEMBERTYPE_USER, getCupsResponse, getMyClubId());
	//OSDKTournaments::GetMyTournamentIdResponse getTournamentIdResponse;
	//BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[dropinPlayer::initializeCupAndTournments]:" << mPlayerData->getPlayerId() << " getMyTournamentId call 1 ");
	//getMyTournamentId(mPlayerInfo, Blaze::OSDKTournaments::MODE_FIFA_CUPS_USER, getTournamentIdResponse, getMyClubId());
	//OSDKTournaments::GetTournamentsResponse getTournamentsResponse;
	//getTournaments(mPlayerInfo, Blaze::OSDKTournaments::MODE_FIFA_CUPS_USER, getTournamentsResponse);
	//OSDKTournaments::GetMyTournamentDetailsResponse getMyTournamentDetailsResponse;
	//BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[dropinPlayer::initializeCupAndTournments]:" << mPlayerData->getPlayerId() << " getMyTournamentDetails call 1 ");
	//joinTournament(mPlayerInfo, getTournamentId());
	//getMyTournamentDetails(mPlayerInfo, Blaze::OSDKTournaments::MODE_FIFA_CUPS_CLUB, 13 /*getTournamentId()*/, getMyTournamentDetailsResponse, getMyClubId());
	//updateLoadOutsPeripherals(mPlayerInfo);
	//setConnectionState(mPlayerInfo);
	//getInvitations(mPlayerInfo, getMyClubId(), Clubs::CLUBS_SENT_TO_ME);
	//getMessages(mPlayerInfo, 5);
	/*if (!isPlatformHost)
	{
		getPetitions(mPlayerInfo, getMyClubId(), CLUBS_PETITION_SENT_TO_CLUB);
	}
	getStatGroup(mPlayerInfo, "VProPosition");
	friendList.push_back(mOpponentPlayerInfo.getPlayerId());
	getStatsByGroupAsync(mPlayerInfo, friendList, "VProPosition");
	mPlayerInfo->setViewId(++viewId);*/
	setConnectionState(mPlayerInfo);
	setConnectionState(mPlayerInfo);
	/*getInvitations(mPlayerInfo, getMyClubId(), Clubs::CLUBS_SENT_TO_ME);
	getMessages(mPlayerInfo, 5);
	friendList.pop_back();*/
	fetchObjectiveConfig(mPlayerInfo);

	getStatGroup(mPlayerInfo, "VProStatAccom");
	getStatGroup(mPlayerInfo, "VProInfo");
	getStatGroup(mPlayerInfo, "VProPosStats");
	getStatGroup(mPlayerInfo, "VProCupStats");
	getClubs(mPlayerInfo, getMyClubId());
	fetchClientConfig(mPlayerInfo, "FIFA_AI_PLAYERS_SETTINGS_9_10", configResponse);
	/*if (isPlatformHost)
	{
		getInvitations(mPlayerInfo, getMyClubId(), Clubs::CLUBS_SENT_BY_CLUB);
	}
	if (!isPlatformHost)
	{
		getPetitions(mPlayerInfo, getMyClubId(), CLUBS_PETITION_SENT_BY_ME);
	}*/
	if (getMyClubId() > 0)
	{
		getPetitions(mPlayerInfo, getMyClubId(), CLUBS_PETITION_SENT_BY_ME);
	}
	fetchObjectiveProgress(mPlayerInfo);

	getStatsByGroupAsync(mPlayerInfo, "VProStatAccom");
	mPlayerInfo->setViewId(++viewId);
	getStatsByGroupAsync(mPlayerInfo, "VProInfo");
	mPlayerInfo->setViewId(++viewId);
	getStatsByGroupAsync(mPlayerInfo, "VProPosStats");
	mPlayerInfo->setViewId(++viewId);
	getStatsByGroupAsync(mPlayerInfo, "VProCupStats");
	mPlayerInfo->setViewId(++viewId);
	/*if (getMyClubId() > 0)
	{
		GetMembersResponse getMembersResponse;
		getMembers(mPlayerInfo, getMyClubId(), 50, getMembersResponse);
	}*/

	fetchClientConfig(mPlayerInfo, "FIFA_AI_PLAYERS_SETTINGS_7_8", configResponse);
	/*if (isPlatformHost)
	{
		getPetitions(mPlayerInfo, getMyClubId(), CLUBS_PETITION_SENT_BY_ME);
	}*/
	fetchClientConfig(mPlayerInfo, "FIFA_VPRO_GROWTH_UNLOCKS_CONFIG", configResponse);
	/*if (isPlatformHost)
	{
		getPetitions(mPlayerInfo, getMyClubId(), CLUBS_PETITION_SENT_TO_CLUB);
		getPetitions(mPlayerInfo, getMyClubId(), CLUBS_PETITION_SENT_TO_CLUB);
	}*/
	fetchClientConfig(mPlayerInfo, "FIFA_AI_PLAYERS_SETTINGS_5_6", configResponse);
	fetchClientConfig(mPlayerInfo, "FIFA_AI_PLAYERS_SETTINGS_3_4", configResponse);
	fetchLoadOuts(mPlayerInfo);
	
	/*friendList.push_back(mOpponentPlayerInfo.getPlayerId());
	getStatsByGroupAsync(mPlayerInfo, friendList, "VProPosition");
	mPlayerInfo->setViewId(++viewId);
	friendList.pop_back();*/

	fetchClientConfig(mPlayerInfo, "FIFA_AI_PLAYERS_SETTINGS_1_2", configResponse);
	//getStatGroup(mPlayerInfo, "VProAttributeXP");
	fetchClientConfig(mPlayerInfo, "FIFA_AI_PLAYERS_PRACTICE_SETTINGS_5", configResponse);
	/*getStatsByGroupAsync(mPlayerInfo, "VProAttributeXP");
	mPlayerInfo->setViewId(++viewId);*/
	fetchClientConfig(mPlayerInfo, "FIFA_AI_PLAYERS_PRACTICE_SETTINGS_4", configResponse);
	fetchClientConfig(mPlayerInfo, "FIFA_AI_PLAYERS_PRACTICE_SETTINGS_3", configResponse);
	fetchClientConfig(mPlayerInfo, "FIFA_AI_PLAYERS_PRACTICE_SETTINGS_2", configResponse);
	fetchClientConfig(mPlayerInfo, "FIFA_AI_PLAYERS_PRACTICE_SETTINGS_1", configResponse);
	//fetchClientConfig(mPlayerInfo, "FIFA_AI_PLAYERS_PRACTICE_SETTINGS_0", configResponse);
	fetchLoadOuts(mPlayerInfo);

	fetchClientConfig(mPlayerInfo, "FIFA_TACTICS_AI_SETTINGS_0", configResponse);
	fetchClientConfig(mPlayerInfo, "FIFA_TACTICS_AI_SETTINGS_1", configResponse);
	fetchClientConfig(mPlayerInfo, "FIFA_TACTICS_AI_SETTINGS_2", configResponse);
	fetchClientConfig(mPlayerInfo, "FIFA_TACTICS_AI_SETTINGS_3", configResponse);
	fetchClientConfig(mPlayerInfo, "FIFA_TACTICS_AI_SETTINGS_4", configResponse);
	fetchClientConfig(mPlayerInfo, "FIFA_TACTICS_AI_SETTINGS_5", configResponse);
	
	getStatGroup(mPlayerInfo, "ClubSeasonalPlay");
	getStatsByGroupAsync(mPlayerInfo, "ClubSeasonalPlay");
	mPlayerInfo->setViewId(++viewId);

	fetchClientConfig(mPlayerInfo, "FIFA_CLUBS_SEASONALPLAY", configResponse);

	tournamentRpcCalls();

	getInvitations(mPlayerInfo, getMyClubId(), Clubs::CLUBS_SENT_TO_ME);
	getInvitations(mPlayerInfo, getMyClubId(), Clubs::CLUBS_SENT_BY_CLUB);
	if (getMyClubId() > 0)
	{
		getPetitions(mPlayerInfo, getMyClubId(), Blaze::Clubs::CLUBS_PETITION_SENT_BY_ME);
		getPetitions(mPlayerInfo, getMyClubId(), Blaze::Clubs::CLUBS_PETITION_SENT_TO_CLUB);
	}

}

BlazeRpcError DropinPlayer::joinClub(ClubId clubID)
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

BlazeRpcError DropinPlayer::sendOrReceiveInvitation()
{
	BlazeRpcError err = ERR_OK;

	// check for invitaitons sent to me
	GetInvitationsRequest request;
	GetInvitationsResponse response;

	request.setInvitationsType(CLUBS_SENT_TO_ME);
	err = mPlayerInfo->getComponentProxy()->mClubsProxy->getInvitations(request, response);


	if (err != ERR_OK)
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[dropinPlayer::sendOrReceiveInvitation : " << mPlayerInfo->getPlayerId() << " Could not get list of invitations");
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
			BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[dropinPlayer::sendOrReceiveInvitation : " << mPlayerInfo->getPlayerId() << " Could not get list of club members");
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
				if ((DROPIN_CONFIG.acceptInvitationProbablity > (uint32_t)Random::getRandomNumber(100)) && (it == msgList.begin()))
				{
					err = mPlayerInfo->getComponentProxy()->mClubsProxy->acceptInvitation(inviteRequest);
					BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[dropinPlayer::sendOrReceiveInvitation] : Accepted Invite for  " << (*it)->getMessageId() << " from player : " << mPlayerInfo->getPlayerId());
				}
				else
				{
					err = mPlayerInfo->getComponentProxy()->mClubsProxy->declineInvitation(inviteRequest);
					BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[dropinPlayer::sendOrReceiveInvitation] : Declined Invite for  " << (*it)->getMessageId() << " from player : " << mPlayerInfo->getPlayerId());
				}

				if (err != ERR_OK)
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[dropinPlayer::sendOrReceiveInvitation : " << mPlayerInfo->getPlayerId() << " Accept/Decline Invites failed");
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
				BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[dropinPlayer::sendOrReceiveInvitation : GetClubMemberShipforUser failed " << mPlayerInfo->getPlayerId() << " Player not found in his own club");
				return ERR_SYSTEM;
			}

			BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[dropinPlayer::sendOrReceiveInvitation] : Successfully Got inviteList and clubMembershipmap " << mPlayerInfo->getPlayerId());

			ClubMembershipList &membershipList = itr->second->getClubMembershipList();
			ClubMembership *membership = *(membershipList.begin()); // assuming at least one (and the first one)
			MembershipStatus status = membership->getClubMember().getMembershipStatus();
			if (status == CLUBS_OWNER || status == CLUBS_GM)
			{
				// if I am GM in no-leave club, send invitation
				SendInvitationRequest sendInvitationrequest;
				BlazeId randomUserId = StressConfigInst.getBaseAccountId() + mPlayerInfo->getLogin()->getOwner()->getStartIndex() + Random::getRandomNumber((int64_t)mPlayerInfo->getLogin()->getOwner()->getPsu());

				//BlazeId randomUserId = (randomIndent > mPlayerInfo->getLogin()->getOwner()->getStartIndex()) ? (randomIndent - mPlayerInfo->getLogin()->getOwner()->getStartIndex()) : randomIndent ;
				BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[dropinPlayer::sendOrReceiveInvitation] : This is our randomeUserzID : " << randomUserId << " this is our start index we subtract from it " << mPlayerInfo->getLogin()->getOwner()->getStartIndex());
				sendInvitationrequest.setBlazeId(randomUserId);
				sendInvitationrequest.setClubId(getMyClubId());
				/*char8_t buf[20240];
				BLAZE_TRACE_RPC_LOG(BlazeRpcLog::gamemanager, "[dropinPlayer::sendOrReceiveInvitation] : \n" << request.print(buf, sizeof(buf)));*/
				err = mPlayerInfo->getComponentProxy()->mClubsProxy->sendInvitation(sendInvitationrequest);

				BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[dropinPlayer::sendOrReceiveInvitation] : Just called sendInvitation RPC " << mPlayerInfo->getPlayerId());

				if (err == Blaze::CLUBS_ERR_INVITATION_ALREADY_SENT || err == Blaze::CLUBS_ERR_PETITION_ALREADY_SENT)
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[dropinPlayer::sendOrReceiveInvitation] : Invitation already sent from " << mPlayerInfo->getPlayerId() << " to : " << randomUserId);
					err = Blaze::ERR_OK;
				}
				else if (err == Blaze::CLUBS_ERR_MAX_INVITES_SENT)
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[dropinPlayer::sendOrReceiveInvitation : Invite count maxed for  " << mPlayerInfo->getPlayerId() << " to : " << randomUserId);
					// revoke one invitation
					GetInvitationsRequest inviteRequest;
					GetInvitationsResponse inviteResponse;

					inviteRequest.setClubId(getMyClubId());
					inviteRequest.setInvitationsType(CLUBS_SENT_BY_CLUB);
					err = mPlayerInfo->getComponentProxy()->mClubsProxy->getInvitations(inviteRequest, inviteResponse);
					//BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[dropinPlayer::sendOrReceiveInvitation : GenInvitations  " << mPlayerInfo->getPlayerId()<<" to : "

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

void DropinPlayer::postGameCalls()
{
	setClientState(mPlayerInfo, Blaze::ClientState::MODE_SINGLE_PLAYER, Blaze::ClientState::STATUS_NORMAL);
	Blaze::BlazeIdList friendList;
	friendList.push_back(mPlayerInfo->getPlayerId());
	setPlayerAttributes("_A", "", myGameId);
	setPlayerAttributes("_V", "", myGameId);
	if (isPlatformHost)
	{
		Collections::AttributeMap attributes;
		attributes.clear();
		attributes["MST"] = "2";
		setGameAttributes(myGameId, attributes);
	}
	sleep(500);
	//lookupUsersIdentification(mPlayerInfo);
	getStatsByGroupAsync(mPlayerInfo, friendList, "VProPosition", getMyClubId());
	getStatGroup(mPlayerInfo, "ClubSeasonalPlay");
	getKeyScopesMap(mPlayerInfo);
	getStatGroup(mPlayerInfo, "ClubRankingPointsSummary");
	//friendList.push_back(mPlayerInfo->getPlayerId()); - add all players in the club
	getStatsByGroupAsync(mPlayerInfo, friendList, "ClubRankingPointsSummary");
	getStatGroup(mPlayerInfo, "ClubSeasonalPlay");
	friendList.clear();
	friendList.push_back(getMyClubId());
	// lookuprandomusers
	lookupRandomUsersByPersonaNames(mPlayerInfo);
	getStatsByGroupAsync(mPlayerInfo, friendList, "ClubSeasonalPlay");
	FetchConfigResponse configResponse;
	fetchClientConfig(mPlayerInfo, "FIFA_H2H_SEASONALPLAY", configResponse);
	setClientState(mPlayerInfo, ClientState::MODE_MENU);
	fetchClientConfig(mPlayerInfo, "OSDK_ABUSE_REPORTING", configResponse);
	fetchMessages(mPlayerInfo, 4, 1634497140);
	fetchMessages(mPlayerInfo, 4, 1668052322);
	if (!mCensusdataSubscriptionStatus)
	{
		BlazeRpcError err;
		err = subscribeToCensusDataUpdates(mPlayerInfo);
		if (err == ERR_OK)
		{
			SetSubscriptionSatatus();
		}
	}
	getStatGroup(mPlayerInfo, "VProStatAccom");
	getStatsByGroup(mPlayerInfo, "VProStatAccom");
	getStatsByGroupAsync(mPlayerInfo, "VProStatAccom");

	getStatGroup(mPlayerInfo, "VProAttributeXP");
	getStatsByGroup(mPlayerInfo, "VProAttributeXP");
	getStatsByGroupAsync(mPlayerInfo, "VProAttributeXP");
	fetchMessages(mPlayerInfo, 4, 1734438245);
	associationGetLists(mPlayerInfo);
	getClubsComponentSettings(mPlayerInfo);
	fetchClientConfig(mPlayerInfo, "OSDK_TICKER", configResponse);
	fetchClientConfig(mPlayerInfo, "OSDK_ARENA", configResponse);
	fetchClientConfig(mPlayerInfo, "OSDK_ROSTER", configResponse);
	localizeStrings(mPlayerInfo);
	getClubs(mPlayerInfo, getMyClubId());
	getStatGroupList(mPlayerInfo);
	Blaze::Clubs::GetClubMembershipForUsersResponse clubMembershipForUsersResponse;
	getClubMembershipForUsers(mPlayerInfo, clubMembershipForUsersResponse);
	getInvitations(mPlayerInfo, getMyClubId(), Clubs::CLUBS_SENT_TO_ME);
	getPeriodIds(mPlayerInfo);
	lookupRandomUsersByPersonaNames(mPlayerInfo);
	getEventsURL(mPlayerInfo);
	getMemberHash(mPlayerInfo, "OSDKPlatformFriendList");
}

BlazeRpcError DropinPlayer::tournamentRpcCalls()
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
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "dropinPlayer::tournamentRpcCalls:player= " << mPlayerData->getPlayerId() << " CupId= " << getCupId());
	}
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::initializeCupAndTournments]:" << mPlayerData->getPlayerId() << " getMyTournamentId call 2 ");
	err = getMyTournamentId(mPlayerInfo, Blaze::OSDKTournaments::MODE_FIFA_CUPS_CLUB, getTournamentIdResponse, getMyClubId());
	if (err == ERR_OK)
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[OnlineSeasons::initializeCupAndTournments]:" << mPlayerData->getPlayerId() << " getMyTournamentId call 2 passed ");
		//tournamentId = getTournamentsResponse.getTournamentList().at(0)->getTournamentId();
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
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "dropinPlayer::tournamentRpcCalls:player= " << mPlayerData->getPlayerId() << " TournamentId is set. " << getTournamentId());
		if (!getTournamentIdResponse.getIsActive())
		{
			//if the player is not in the tournament means that he might have lost/won the last tournament, so resetting here..
			osdkResetTournament(mPlayerInfo, getTournamentId(), Blaze::OSDKTournaments::MODE_FIFA_CUPS_CLUB);
		}
	}
	else
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "[dropinPlayer::tournamentRpcCalls::TournamentId is Not Set for " << mPlayerData->getPlayerId());
	}

	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::initializeCupAndTournments]:" << mPlayerInfo->getPlayerId() << " getMyTournamentDetails call 1 ");
	getMyTournamentDetails(mPlayerInfo, Blaze::OSDKTournaments::MODE_FIFA_CUPS_CLUB, 13 /*GetTournamentId()*/, getMyTournamentDetailsResponse, getMyClubId());

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

bool DropinPlayer::executeReportTelemetry()
{
    BlazeRpcError err = ERR_OK;
    if (!mPlayerData->isConnected())
    {
        BLAZE_ERR_LOG(BlazeRpcLog::util, "[dropinPlayer::ReportTelemetry::User Disconnected = " << mPlayerData->getPlayerId());
        return false;
    }
    if (getGameId() == 0)
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[dropinPlayer::ReportTelemetry][GameId for player = " << mPlayerInfo->getPlayerId() << "] not present");
        return false;
    }
    StressGameInfo* ptrGameInfo = DropinPlayerGameDataRef(mPlayerInfo).getGameData(getGameId());
    if (ptrGameInfo == NULL)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[dropinPlayer::ReportTelemetry][GameData for game = " << getGameId() << "] not present");
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
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "Dropin: reportTelemetry failed with err=" << ErrorHelp::getErrorName(err));
    }
    return true;
}

BlazeRpcError DropinPlayer::startMatchmakingScenario(ScenarioType scenarioName)
{
	BlazeRpcError err = ERR_OK;
    StartMatchmakingScenarioRequest request;
    StartMatchmakingScenarioResponse response;
	MatchmakingCriteriaError criteriaError;

	request.getCommonGameData().setGameProtocolVersionString(StressConfigInst.getGameProtocolVersionString());
	request.getCommonGameData().setGameType(GAME_TYPE_GAMESESSION);
	NetworkAddress& hostAddress = request.getCommonGameData().getPlayerNetworkAddress();
	hostAddress.switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
	if (!mPlayerData->isConnected())
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "[dropinPlayer::startMatchmakingScenario::User Disconnected = " << mPlayerData->getPlayerId());
		return ERR_SYSTEM;
	}
	hostAddress.getIpPairAddress()->getExternalAddress().setIp(mPlayerData->getConnection()->getAddress().getIp());
	hostAddress.getIpPairAddress()->getExternalAddress().setPort(mPlayerData->getConnection()->getAddress().getPort());
	hostAddress.getIpPairAddress()->getExternalAddress().copyInto(hostAddress.getIpPairAddress()->getInternalAddress());
	
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
		playerJoinDataPtr->setRole("");
		playerJoinDataPtr->setSlotType(GameManager::INVALID_SLOT_TYPE);
		playerJoinDataPtr->getUser().setExternalId(mPlayerInfo->getExternalId());
		playerJoinDataPtr->getUser().getPlatformInfo().getExternalIds().setSwitchId("");
		playerJoinDataPtr->getUser().getPlatformInfo().getExternalIds().setXblAccountId(mPlayerInfo->getExternalId());
		playerJoinDataPtr->getUser().setBlazeId(mPlayerInfo->getPlayerId());
		playerJoinDataPtr->getUser().setName(mPlayerInfo->getPersonaName().c_str());
		playerJoinDataPtr->getUser().setPersonaNamespace("");
		playerJoinDataPtr->getUser().setOriginPersonaId(0);
		playerJoinDataPtr->getUser().setPidId(0);
		playerJoinDataPtr->getUser().setAccountId(0);
		playerJoinDataPtr->getUser().setAccountLocale(0);
		playerJoinDataPtr->getUser().setAccountCountry(0);
		playerJoinDataPtr->setTeamId(ANY_TEAM_ID + 1);
		playerJoinDataPtr->setTeamIndex(ANY_TEAM_ID + 1);
	}

	SET_SCENARIO_ATTRIBUTE(request, "CLUB_ID", "0");
	SET_SCENARIO_ATTRIBUTE(request, "CUSTOM_CONTROLLER", "0");
	SET_SCENARIO_ATTRIBUTE(request, "GAME_MODE", "5");
	SET_SCENARIO_ATTRIBUTE(request, "GK_CONTROL", "abstain");
	SET_SCENARIO_ATTRIBUTE(request, "MATCHUP_HASH", "4173721");
	SET_SCENARIO_ATTRIBUTE(request, "MATCH_CLUB_TYPE", "0");
	SET_SCENARIO_ATTRIBUTE(request, "MATCH_GUESTS", "0");
	SET_SCENARIO_ATTRIBUTE(request, "NET_TOP", (uint64_t)1);
	SET_SCENARIO_ATTRIBUTE(request, "PPT", "0");
	SET_SCENARIO_ATTRIBUTE(request, "WOMEN_RULE", "0");
	request.setScenarioName("Drop_In");

	//char8_t buf[20240];
	//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "StartMatchmaking RPC : \n" << request.print(buf, sizeof(buf)));
	err = mPlayerInfo->getComponentProxy()->mGameManagerProxy->startMatchmakingScenario(request,response,criteriaError);
	if (err == ERR_OK) 
	{
        mMatchmakingSessionId = response.getScenarioId();
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "dropinPlayer::startMatchmakingScenario successful. mMatchmakingSessionId= " << mMatchmakingSessionId);
    } 
	else
	{
       BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "dropinPlayer::startMatchmakingScenario failed. Error(" << ErrorHelp::getErrorName(err) << ")");
	}
	return err;
}

}  // namespace Stress
}  // namespace Blaze
