//  *************************************************************************************************
//
//   File:    friendlies.cpp
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#include "friendlies.h"
#include "playermodule.h"
#include "./gamesession.h"
#include "friendliesgamesession.h"
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

Friendlies::Friendlies(StressPlayerInfo* playerData)
    : GamePlayUser(playerData),
	  FriendliesGameSession(playerData, 2 /*gamesize*/, PEER_TO_PEER_FULL_MESH /*Topology*/,
                             StressConfigInst.getGameProtocolVersionString() /*GameProtocolVersionString*/),
      mIsMarkedForLeaving(false)
{
}

Friendlies::~Friendlies()
{
    BLAZE_INFO(BlazeRpcLog::util, "[Friendlies][Destructor][%" PRIu64 "]: Friendlies destroy called", mPlayerInfo->getPlayerId());
    stopReportTelemtery();
    //  deregister async handlers to avoid client crashes when disconnect happens
    deregisterHandlers();
    //  wait for some time to allow already scheduled jobs to complete
    sleep(30000);
}

void Friendlies::deregisterHandlers()
{
    FriendliesGameSession::deregisterHandlers();
}

void Friendlies::preLogoutAction()
{
}

BlazeRpcError Friendlies::startGame()
{
	BlazeRpcError result = ERR_OK;
	GameId randGameId = getRandomCreatedGame();
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Friendlies::startGame]: " << mPlayerData->getPlayerId()<<" game ID got was : "<<randGameId);
	if(randGameId == 0)
	{
		/*uint32_t viewId = mPlayerInfo->getViewId();
		getStatGroup(mPlayerInfo, "MyFriendlies");
		mPlayerInfo->setViewId(++viewId);
		getStatsByGroupAsync(mPlayerInfo, "MyFriendlies");
		userSettingsSave(mPlayerInfo, "");*/
		result = createGame();
	}
	else
	{		
		StressGameInfo* ptrGameInfo = FriendliesGameDataRef(mPlayerInfo).getGameData(randGameId);
		if (ptrGameInfo == NULL)
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Friendlies::startGame][GameData for game = " << getGameId() << "] not present");
		}

		else
		{
			if(ptrGameInfo->getPlayerIdListMap().size() > 1)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[Friendlies::startGame] Game Size reached Max. Unable to  Join : " << getGameId() );
				return ERR_SYSTEM;
			}
			//blaze_snzprintf(creatorPersonName, sizeof(mParBuf), mOwner->getPersonaFormat(), mAccId);
   // return mParBuf;
			/*
			UserInfo* gameCreatorUserInfo = ptrGameInfo->getPlayerIdListMap().begin()->second;
			char8_t creatorPersonName[256];
			blaze_snzprintf(creatorPersonName, sizeof(creatorPersonName), "%s", gameCreatorUserInfo->personaName);
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Friendlies::startGame] Calling lookupuserbypersonaname at startGame " << creatorPersonName);
			lookupUsersByPersonaNames(mPlayerInfo,creatorPersonName);
			lookupUsersByPersonaNames(mPlayerInfo, creatorPersonName);
			*/
		}
		result = joinGame(randGameId);
	}

	return result;

}
BlazeRpcError Friendlies::simulateLoad()
{
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Friendlies::simulateLoad]:" << mPlayerData->getPlayerId());
    BlazeRpcError err = ERR_OK;
	mScenarioType = StressConfigInst.getRandomScenarioByPlayerType(FRIENDLIES);
    err = simulateMenuFlow();

	lobbyCalls();
	setConnectionState(mPlayerInfo);

	//Send MM request 
	err = startGame();
	if(err != ERR_OK) { return err; }

    int64_t playerStateTimer = 0;
    int64_t  playerStateDurationMs = 0;
    GameState   playerStateTracker = getPlayerState();
    uint16_t stateLimit = 0;
	//uint32_t viewId = mPlayerInfo->getViewId();
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
            BLAZE_ERR_LOG(BlazeRpcLog::util, "[Friendlies::simulateLoad::User Disconnected = " << mPlayerData->getPlayerId());
            return ERR_DISCONNECTED;
        }
        if (playerStateTracker != getPlayerState())  //  state changed
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Friendlies::simulateLoad:playerStateTracker]" << mPlayerData->getPlayerId() << " Time Spent in Sate: " << GameStateToString(
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
                if (playerStateDurationMs > FR_CONFIG.maxPlayerStateDurationMs)
                {
                    BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[Friendlies::simulateLoad:playerStateTracker][" << mPlayerData->getPlayerId() << "]: Player exceded maximum Game state Duration:" << playerStateDurationMs <<
                                   " ms" << " Current Game State:" << GameStateToString(getPlayerState()));
                    if (IN_GAME == getPlayerState())
                    {
                        mIsMarkedForLeaving = true;   //  Player exceed maximum duration in InGAME State so mark this Player to Leave from InGame
                    }
                }
            }
        }
        const char8_t * stateMsg = GameStateToString(getPlayerState());
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[Friendlies::simulateLoad]: Player: " << mPlayerInfo->getPlayerId() << ", GameId: " << myGameId << " Current state : " << stateMsg << " InStateDuration:" <<
                       playerStateDurationMs);
        switch (getPlayerState())
        {
            //   Sleep for all the below states
            case NEW_STATE:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Friendlies::simulateLoad]" << mPlayerData->getPlayerId() << " with state=NEW_STATE");
                    stateLimit++;
                    if (stateLimit >= FR_CONFIG.GameStateIterationsMax)
                    {
                        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Friendlies::simulateLoad]" << mPlayerData->getPlayerId() << " with state=NEW_STATE for long time and return");
                        return err;
                    }
                    //  Wait here if resetDedicatedserver() called/player not found for invite.
                    sleep(FR_CONFIG.GameStateSleepInterval);
                    break;
                }
            case PRE_GAME:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Friendlies::simulateLoad]" << mPlayerData->getPlayerId() << " with state=PRE_GAME");
					if (!isPlatformHost)
					{
						setConnectionState(mPlayerInfo);
						if (mScenarioType != FRRB) 
						{
							Blaze::BlazeIdList friendList;
							friendList.push_back(mPlayerInfo->getPlayerId());
							getStatGroup(mPlayerInfo, "MyFriends");
							uint32_t viewId = mPlayerInfo->getViewId();
							mPlayerInfo->setViewId(++viewId);
							getStatsByGroupAsync(mPlayerInfo, "MyFriends", friendList, "friend", getOpponentPlayerInfo().getPlayerId(), 0);	//	Adding playerId to entityId
							Blaze::BlazeIdList opponentList;
							opponentList.push_back(getOpponentPlayerInfo().getPlayerId());
							mPlayerInfo->setViewId(++viewId);
							getStatsByGroupAsync(mPlayerInfo, "MyFriends", opponentList, "friend", mPlayerInfo->getPlayerId(), 0);	//	Adding friendId to entityId
						}
						setPlayerAttributes("REQ", "1");
						setClientState(mPlayerInfo, Blaze::ClientState::MODE_MULTI_PLAYER, Blaze::ClientState::STATUS_NORMAL);
						setPlayerAttributes("REQ", "2");
						FetchConfigResponse response;
						fetchClientConfig(mPlayerInfo, "OSDK_CUSTOM_DATA", response);
					}
					//setClientState(mPlayerInfo, Blaze::ClientState::MODE_MULTI_PLAYER, Blaze::ClientState::STATUS_NORMAL);
					//if (!isTopologyHost)
					//{
					//	setPlayerAttributes("REQ", "2");
					//}
					//FetchConfigResponse response;
					//fetchClientConfig(mPlayerInfo, "OSDK_CUSTOM_DATA", response);
					//
					//if (isTopologyHost)
					//{
					//	meshEndpointsConnected(getOpponentPlayerInfo().getConnGroupId());
					//}
					//StressGameInfo* ptrGameInfo = FriendliesGameDataRef(mPlayerInfo).getGameData(getRandomCreatedGame());
					//UserInfo* gameCreatorUserInfo = ptrGameInfo->getPlayerIdListMap().begin()->second;
					//char8_t creatorPersonName[256];
					//blaze_snzprintf(creatorPersonName, sizeof(creatorPersonName), "%s", gameCreatorUserInfo->personaName);
					//BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Friendlies::startGame] Calling lookupuserbypersonaname at PREGAME " << creatorPersonName);
					//lookupUsersByPersonaNames(mPlayerInfo, creatorPersonName);
					
					//if (isTopologyHost)
					//{	
					//	lookupUsersIdentification(mPlayerInfo);
					//	setGameSettings(0);// setGameSettings(1056);	As per new logs
					//	lookupUsersByPersonaNames(mPlayerInfo, "robotosmoko");
					//	advanceGameState(IN_GAME);
					//}
					//else
					//{
					//	Blaze::BlazeIdList friendList;
					//	friendList.push_back(mPlayerInfo->getPlayerId());
					//	getStatGroup(mPlayerInfo, "MyFriends");
					//	uint32_t viewId = mPlayerInfo->getViewId();
					//	mPlayerInfo->setViewId(++viewId);
					//	getStatsByGroupAsync(mPlayerInfo, "MyFriends", friendList, "friend", getOpponentPlayerInfo().getPlayerId());	//	Adding playerId to entityId
					//	Blaze::BlazeIdList opponentList;
					//	opponentList.push_back(getOpponentPlayerInfo().getPlayerId());
					//	mPlayerInfo->setViewId(++viewId);
					//	// getStatsByGroupAsync(&mOpponentPlayerInfo, "MyFriends", opponentList, "friend", mPlayerInfo->getPlayerId());	//	Adding friendId to entityId
					//	setPlayerAttributes("REQ", "1");
					//	setClientState(mPlayerInfo, Blaze::ClientState::MODE_MULTI_PLAYER, Blaze::ClientState::STATUS_SUSPENDED);
					//	setPlayerAttributes("REQ", "2");
					//}
					if (isPlatformHost)
					{
						setGameSettingsMask(mPlayerInfo, myGameId, 0, 2063);
					}
					//setClientState(mPlayerInfo, Blaze::ClientState::MODE_MULTI_PLAYER, Blaze::ClientState::STATUS_SUSPENDED); // STATUS_NORMAL	As per new logs
					//unsubscribeFromCensusData(mPlayerInfo);
					if (isTopologyHost)
					{
						sleep(5000);
						advanceGameState(IN_GAME);
					}
                    sleep(FR_CONFIG.GameStateSleepInterval);
                    break;
                }
            case IN_GAME:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Friendlies::simulateLoad]" << mPlayerData->getPlayerId() << " with state=IN_GAME");
					setClientState(mPlayerInfo, Blaze::ClientState::MODE_MULTI_PLAYER, Blaze::ClientState::STATUS_SUSPENDED); // STATUS_NORMAL	As per new logs
					if (mCensusdataSubscriptionStatus)
					{
						err = unsubscribeFromCensusData(mPlayerInfo);
						if (err == ERR_OK)
						{
							UnsetSubscriptionSatatus();
						}
					}
					if (!ingame_enter)
                    {
                        ingame_enter = true;
                        simulateInGame();
						//Start InGame Timer	
						if(isTopologyHost)
						{
							BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Friendlies::simulateLoad]" << mPlayerData->getPlayerId() << " setInGameStartTime. gameID=" << myGameId);
							setInGameStartTime();
						}
                    }

                    executeReportTelemetry();
					//setClientState(mPlayerInfo, Blaze::ClientState::MODE_SINGLE_PLAYER, Blaze::ClientState::STATUS_NORMAL);
					if( (isTopologyHost && IsInGameTimedOut()) || (mIsMarkedForLeaving == true) )
					{		
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Friendlies::simulateLoad]" << mPlayerData->getPlayerId() << " IsInGameTimedOut/mIsMarkedForLeaving=true. gameID=" << myGameId);
						advanceGameState(POST_GAME);
					}
                    
                    sleep(FR_CONFIG.GameStateSleepInterval);
                    break;
                }
            case INITIALIZING:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Friendlies::simulateLoad]" << mPlayerData->getPlayerId() << " with state=INITIALIZING");
					if (isPlatformHost)
					{
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Friendlies::simulateLoad][" << mPlayerData->getPlayerId() << "calling advanceGameState");
						advanceGameState(PRE_GAME);
					}
                    sleep(FR_CONFIG.GameStateSleepInterval);
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
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Friendlies::simulateLoad]" << mPlayerData->getPlayerId() << " with state=POST_GAME");
					setClientState(mPlayerInfo, Blaze::ClientState::MODE_SINGLE_PLAYER, Blaze::ClientState::STATUS_NORMAL);
                    //Submitgamereport is initiated from NOTIFY_NOTIFYGAMESTATECHANGE async notification
					if (mScenarioType != FRRB)
					{
						getStatGroup(mPlayerInfo, "MyFriends");
					}
					//Leave game
					//leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT);
					//leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT);
					BLAZE_INFO(BlazeRpcLog::gamemanager, "[Friendlies::leaveGame][%" PRIu64 "]: player left game: GameId = %" PRIu64 "", mPlayerData->getPlayerId(), myGameId);
					//meshEndpointsDisconnected RPC is initiated from NOTIFY_NOTIFYPLAYERREMOVED
					//invokeUdatePrimaryExternalSessionForUser(mPlayerInfo, myGameId, "", "", myGameId, GAME_LEAVE, GAME_TYPE_GAMESESSION, ACTIVE_CONNECTED);
					updatePrimaryExternalSessionForUserRequest(mPlayerInfo, myGameId, myGameId, UPDATE_PRESENCE_REASON_GAME_LEAVE, GAME_TYPE_GAMESESSION, ACTIVE_CONNECTED, PRESENCE_MODE_STANDARD, 0, false);
					postGameCalls();
					delayRandomTime(5000, 10000);
					setPlayerState(DESTRUCTING);
                    
                    sleep(FR_CONFIG.GameStateSleepInterval);
                    break;
                }
            case RESETABLE:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Friendlies::simulateLoad]" << mPlayerData->getPlayerId() << " with state=RESETABLE");
                    //  mGameData will be freed when NOTIFY_NOTIFYGAMEREMOVED notification comes.
                    sleep(FR_CONFIG.GameStateSleepInterval);
                    break;
                }
            case DESTRUCTING:
                {
                    //  Player Left Game
                    //  Presently using this state when user is removed from the game and return from this function.
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Friendlies::simulateLoad]" << mPlayerData->getPlayerId() << " with state=DESTRUCTING");
                    setClientState(mPlayerInfo, ClientState::MODE_MENU);
                    //  Return here
                    return err;
                }
            case MIGRATING:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Friendlies::simulateLoad]" << mPlayerData->getPlayerId() << " with state=MIGRATING");
                    //  mGameData will be freed when NOTIFY_NOTIFYGAMEREMOVED notification comes.
                    sleep(10000);
                    break;
                }
            //case GAME_GROUP_INITIALIZED:
            //    {
            //        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Friendlies::simulateLoad]" << mPlayerData->getPlayerId() << " with state=GAME_GROUP_INITIALIZED");
            //        sleep(5000);
            //        if (playerStateDurationMs > FR_CONFIG.MaxGameGroupInitializedStateDuration)
            //        {
            //            //  Leader might have disconnected or failed in Matchmaking or in leaving the Group.
            //            setPlayerState(DESTRUCTING);
            //           
            //        }
            //        break;
            //    }
            case INACTIVE_VIRTUAL:
            default:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Friendlies::simulateLoad]" << mPlayerData->getPlayerId() << " with state=" << getPlayerState());
                    //  shouldn't come here
                    return err;
                }
        }
    }	
}

void Friendlies::lobbyCalls()
{
	Blaze::BlazeIdList friendList;
	uint32_t viewId = mPlayerInfo->getViewId();
	friendList.push_back(mPlayerInfo->getPlayerId());
	if (isTopologyHost)
	{
		friendList.push_back(mOpponentPlayerInfo.getPlayerId());
	}
	
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
		mutex.Lock();
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
					mutex.Unlock();
					setMyClubId(0);
					//return result;
				}
			}
			iter++;
		}
		//unlock
		mutex.Unlock();
	}

	Blaze::Clubs::GetMembersResponse clubResponse;
	//getClubs(mPlayerInfo, getMyClubId());		Commented as per logs
	getMembers(mPlayerInfo, getMyClubId(), 50, clubResponse);	
	if (getMyClubId() > 0)
	{
		getPetitions(mPlayerInfo, getMyClubId(), CLUBS_PETITION_SENT_TO_CLUB);
	}
	getStatGroup(mPlayerInfo, "VProPosition");
	mPlayerInfo->setViewId(++viewId);
	getStatsByGroupAsync(mPlayerInfo, friendList, "VProPosition",getMyClubId());
	setConnectionState(mPlayerInfo);
//	getInvitations(mPlayerInfo, getMyClubId(), Clubs::CLUBS_SENT_TO_ME);
	//getMessages(mPlayerInfo, 5);
	
	getStatGroup(mPlayerInfo, "MyFriendlies");
	mPlayerInfo->setViewId(++viewId);
	getStatsByGroupAsync(mPlayerInfo, friendList, "MyFriendlies");

	Blaze::PersonaNameList personaNameList;
	personaNameList.push_back("q36448b89fd-GBen");
	personaNameList.push_back("qdf3d34c996-USen");
	personaNameList.push_back("addafifa");
	personaNameList.push_back("walstab2021");
	personaNameList.push_back("Lorry_ONL");
	personaNameList.push_back("Cata_G");
	personaNameList.push_back("FIFA20Final_28");
	personaNameList.push_back("fifa21online1");
	personaNameList.push_back("FIFA20final_153");
	personaNameList.push_back("fifa212");
	lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
	lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
	lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
	/*
	if (isTopologyHost)
	{
		Blaze::PersonaNameList personaNameList;
		//lookupUsers
		personaNameList.push_back("robotosmoko");
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		personaNameList.push_back("fifa_onl021");
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		//personaNameList.pop_back();
		lookupUsersByPersonaNames(mPlayerInfo, "robotosmoko");
		localizeStrings(mPlayerInfo);
		//personaNameList.push_back("fifa_onl021");
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
	}
	else
	{
		lookupUsersByBlazeId(mPlayerInfo, mPlayerInfo->getPlayerId());
		lookupUsersByPersonaNames(mPlayerInfo, "");
		lookupUsersByPersonaNames(mPlayerInfo, "fifa_onl012");
		lookupUsersByPersonaNames(mPlayerInfo, "fifa_onl012");
		lookupUsersByPersonaNames(mPlayerInfo, "fifa_onl012");
		lookupUsersByPersonaNames(mPlayerInfo, "");
		lookupUsersByPersonaNames(mPlayerInfo, "fifa_onl012");
		lookupUsersByPersonaNames(mPlayerInfo, "fifa_onl012");
		lookupUsersByPersonaNames(mPlayerInfo, "");
		lookupUsersByPersonaNames(mPlayerInfo, "fifa_onl012");
		lookupUsersByPersonaNames(mPlayerInfo, "");
		lookupUsersByPersonaNames(mPlayerInfo, "fifa_onl012");
		lookupUsersByPersonaNames(mPlayerInfo, "");
		setConnectionState(mPlayerInfo);
	}
	*/
}


BlazeRpcError Friendlies::joinClub(ClubId clubID)
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


bool Friendlies::executeReportTelemetry()
{
	BlazeRpcError err = ERR_OK;
    if (!mPlayerData->isConnected())
    {
        BLAZE_ERR_LOG(BlazeRpcLog::util, "[Friendlies::ReportTelemetry::User Disconnected = " << mPlayerData->getPlayerId());
        return false;
    }
    if (getGameId() == 0)
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[Friendlies::ReportTelemetry][GameId for player = " << mPlayerInfo->getPlayerId() << "] not present");
        return false;
    }
    StressGameInfo* ptrGameInfo = FriendliesGameDataRef(mPlayerInfo).getGameData(getGameId());
    if (ptrGameInfo == NULL)
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[Friendlies::ReportTelemetry][GameData for game = " << getGameId() << "] not present");
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
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "Friendlies: reportTelemetry failed with err=" << ErrorHelp::getErrorName(err));
    }
    return true;
}

void Friendlies::simulateInGame()
{
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][Friendlies]: In Game simulation:" << myGameId);
	
    delayRandomTime(2000, 5000);
}


void   Friendlies::postGameCalls()
{
	//setClientState(mPlayerInfo, ClientState::MODE_MENU);
    //delayRandomTime(1000, 3000);
	uint32_t viewId = mPlayerInfo->getViewId();
	//getStatGroup(mPlayerInfo, "MyFriends");
	//meshEndpointsDisconnected(getOpponentPlayerInfo().getConnGroupId(), DISCONNECTED, 0);
	getKeyScopesMap(mPlayerInfo);
	Blaze::BlazeIdList friendList;
	friendList.push_back(mPlayerInfo->getPlayerId());
	Blaze::BlazeIdList opponentList;
	opponentList.push_back(getOpponentPlayerInfo().getPlayerId());
	mPlayerInfo->setViewId(++viewId);
	getStatsByGroupAsync(mPlayerInfo, "MyFriends", friendList, "friend", getOpponentPlayerInfo().getPlayerId());
	delayRandomTime(500, 1500);
	mPlayerInfo->setViewId(++viewId);
	getStatsByGroupAsync(mPlayerInfo, "MyFriends", opponentList, "friend", mPlayerInfo->getPlayerId());
	mPlayerInfo->setViewId(++viewId);
	getStatsByGroupAsync(mPlayerInfo, "MyFriends", friendList, "friend", getOpponentPlayerInfo().getPlayerId());
}


void  Friendlies::addPlayerToList()
{
	//  add player to universal list first
    Player::addPlayerToList();
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[Friendlies::addPlayerToList]: Player: " << mPlayerInfo->getPlayerId() << ", Game: " << myGameId);
    {
        ((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->friendliesMap.insert(eastl::pair<PlayerId, eastl::string>(mPlayerInfo->getPlayerId(), mPlayerInfo->getPersonaName()));
    }
}

void  Friendlies::removePlayerFromList()
{
	//  remove player from universal list first
    Player::removePlayerFromList();
    PlayerId  id = mPlayerInfo->getPlayerId();
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[Friendlies::removePlayerFromList]: Player: " << id << ", Game: " << myGameId);
    PlayerMap& playerMap = ((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->friendliesMap;
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


}
}