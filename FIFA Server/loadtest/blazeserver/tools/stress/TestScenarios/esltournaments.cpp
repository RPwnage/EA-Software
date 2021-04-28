//  *************************************************************************************************
//
//   File:    friendlies.cpp
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#include "esltournaments.h"
#include "playermodule.h"
#include "./gamesession.h"
#include "esltournamentsgamesession.h"
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

ESLTournaments::ESLTournaments(StressPlayerInfo* playerData)
    : GamePlayUser(playerData),
	ESLTournamentsGameSession(playerData, 2 /*gamesize*/, PEER_TO_PEER_FULL_MESH /*Topology*/,
                             StressConfigInst.getGameProtocolVersionString() /*GameProtocolVersionString*/),
      mIsMarkedForLeaving(false)
{
	mHttpESLProxyConnMgr = NULL;
	initESLProxyHTTPConnection();
}

ESLTournaments::~ESLTournaments()
{
    BLAZE_INFO(BlazeRpcLog::util, "[Friendlies][Destructor][%" PRIu64 "]: Friendlies destroy called", mPlayerInfo->getPlayerId());
    stopReportTelemtery();
    //  deregister async handlers to avoid client crashes when disconnect happens
    deregisterHandlers();
    //  wait for some time to allow already scheduled jobs to complete
    sleep(30000);
}

void ESLTournaments::deregisterHandlers()
{
	ESLTournamentsGameSession::deregisterHandlers();
}

void ESLTournaments::preLogoutAction()
{
}

BlazeRpcError ESLTournaments::startGame()
{
	BlazeRpcError result = ERR_OK;
	
	if (myGameId == 0)	//	No game is created, so, create the game
	{
		result = createGame();
	}
	else
	{
		result = joinGame(myGameId);
	}

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
BlazeRpcError ESLTournaments::simulateLoad()
{
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Friendlies::simulateLoad]:" << mPlayerData->getPlayerId());
    BlazeRpcError err = ERR_OK;
	/*
	static bool isServer = false;
	EA::TDF::TdfPrimitiveVector<PlayerId>::iterator playerIdIterator;
	
	int flag = 1;
	for (playerIdIterator = playerIds.begin(); playerIdIterator < playerIds.end(); playerIdIterator++)
	{
		if (flag == 1)
		{
			BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, " Player Iterator : " << *playerIdIterator);
		}
		else
		{
			BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, " Player Iterator : " << *playerIdIterator);
		}
		flag++;
	}*/
	static int numOfPlayers = 0;
	//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[]: Is Server ? : " << isServer);
	CoopPlayerList& cooplist = ((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->eslPlayerList;
	ESLAccountList& mEslAccountList = ((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->eslAccountList;

	if (numOfPlayers % 3 == 2)
	{
		//Server calls
		BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[]: Client= PlayerId: " << cooplist.at(numOfPlayers-2));
		BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[]: Client= AccountId: " << mEslAccountList.at(numOfPlayers-1));
		//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[]: Client= : " << cooplist.at(numOfPlayers+2));
		//createGame();
		numOfPlayers = numOfPlayers + 1;
		createTournamentGame(numOfPlayers-1, cooplist);
		sleep(10000);
		//cancelTournamentGame();
	}
	else
	{
		BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[]: Player ID= : " << cooplist.at(numOfPlayers));
		numOfPlayers = numOfPlayers + 1;

		err = simulateMenuFlow();

		lobbyCalls();
		setConnectionState(mPlayerInfo);
	}
	//numOfPlayers = numOfPlayers + 1;
	//if (count == 2)
		//count = 0;
	//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[]: Is Server ? : " << isServer);
	BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[]: Count : " << numOfPlayers);
	//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[]: Count : " << mEslAuthCode);

	/*
	err = simulateMenuFlow();

	lobbyCalls();
	setConnectionState(mPlayerInfo);

	//Send MM request 
	err = startGame();
	if(err != ERR_OK) { return err; }
	*/
    int64_t playerStateTimer = 0;
    int64_t  playerStateDurationMs = 0;
    GameState   playerStateTracker = getPlayerState();
    uint16_t stateLimit = 0;
	//uint32_t viewId = mPlayerInfo->getViewId();
    bool ingame_enter = false;
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
						Blaze::BlazeIdList friendList;
						friendList.push_back(mPlayerInfo->getPlayerId());
						//getStatGroup(mPlayerInfo, "MyFriends");
						//uint32_t viewId = mPlayerInfo->getViewId();
						//mPlayerInfo->setViewId(++viewId);
						//getStatsByGroupAsync(mPlayerInfo, "MyFriends", friendList, "friend", getOpponentPlayerInfo().getPlayerId(),0);	//	Adding playerId to entityId
						//Blaze::BlazeIdList opponentList;
						//opponentList.push_back(getOpponentPlayerInfo().getPlayerId());
						//mPlayerInfo->setViewId(++viewId);
						//getStatsByGroupAsync(&mOpponentPlayerInfo, "MyFriends", opponentList, "friend", mPlayerInfo->getPlayerId(),0);	//	Adding friendId to entityId
						setPlayerAttributes("REQ", "1");
						setClientState(mPlayerInfo, Blaze::ClientState::MODE_MULTI_PLAYER, Blaze::ClientState::STATUS_NORMAL);
						setPlayerAttributes("REQ", "2");
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
					FetchConfigResponse response;
					fetchClientConfig(mPlayerInfo, "OSDK_CUSTOM_DATA", response);
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
					setClientState(mPlayerInfo, Blaze::ClientState::MODE_MULTI_PLAYER, Blaze::ClientState::STATUS_SUSPENDED); // STATUS_NORMAL	As per new logs
					unsubscribeFromCensusData(mPlayerInfo);
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
					//getStatGroup(mPlayerInfo, "MyFriends");
					//Leave game
					leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT);
					//leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT);
					BLAZE_INFO(BlazeRpcLog::gamemanager, "[Friendlies::leaveGame][%" PRIu64 "]: player left game: GameId = %" PRIu64 "", mPlayerData->getPlayerId(), myGameId);
					//meshEndpointsDisconnected RPC is initiated from NOTIFY_NOTIFYPLAYERREMOVED
					invokeUdatePrimaryExternalSessionForUser(mPlayerInfo, myGameId, "", "", myGameId, GAME_LEAVE, GAME_TYPE_GAMESESSION, ACTIVE_CONNECTED);
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

void ESLTournaments::lobbyCalls()
{
	Blaze::BlazeIdList friendList;
	uint32_t viewId = mPlayerInfo->getViewId();
	friendList.push_back(mPlayerInfo->getPlayerId());
	if (isTopologyHost)
	{
		friendList.push_back(mOpponentPlayerInfo.getPlayerId());
	}
	/*Blaze::PersonaNameList personaNameList;
	if (isTopologyHost)
	{
		personaNameList.push_back("robotosmoko");
	}
	else
	{
		personaNameList.push_back("");
	}
	lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
	lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
	lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
	localizeStrings(mPlayerInfo);
	*/
	//lookupUsersByPersonaNames();
	// Set the club id or join the club before getting the club
	// commenting below if condition because esl doesn't creategame
	
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
	if (isPlatformHost)
	{
		getPetitions(mPlayerInfo, getMyClubId(), CLUBS_PETITION_SENT_TO_CLUB);
	}
	getStatGroup(mPlayerInfo, "VProPosition");
	mPlayerInfo->setViewId(++viewId);
	getStatsByGroupAsync(mPlayerInfo, friendList, "VProPosition");
	setConnectionState(mPlayerInfo);
//	getInvitations(mPlayerInfo, getMyClubId(), Clubs::CLUBS_SENT_TO_ME);
	//getMessages(mPlayerInfo, 5);
	if (isPlatformHost)
	{
		getStatGroup(mPlayerInfo, "MyFriendlies");
		mPlayerInfo->setViewId(++viewId);
		getStatsByGroupAsync(mPlayerInfo, friendList, "MyFriendlies");
	}
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


BlazeRpcError ESLTournaments::joinClub(ClubId clubID)
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


bool ESLTournaments::executeReportTelemetry()
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

void ESLTournaments::simulateInGame()
{
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][Friendlies]: In Game simulation:" << myGameId);
	
    delayRandomTime(2000, 5000);
}


void ESLTournaments::postGameCalls()
{
	//setClientState(mPlayerInfo, ClientState::MODE_MENU);
    //delayRandomTime(1000, 3000);
	//uint32_t viewId = mPlayerInfo->getViewId();
	getStatGroup(mPlayerInfo, "MyFriends");
	//meshEndpointsDisconnected(getOpponentPlayerInfo().getConnGroupId(), DISCONNECTED, 0);
	getKeyScopesMap(mPlayerInfo);
	//Blaze::BlazeIdList friendList;
	//friendList.push_back(mPlayerInfo->getPlayerId());
	//Blaze::BlazeIdList opponentList;
	//opponentList.push_back(getOpponentPlayerInfo().getPlayerId());
	//PlayerInfo->setViewId(++viewId);
	//getStatsByGroupAsync(mPlayerInfo, "MyFriends", friendList, "friend", getOpponentPlayerInfo().getPlayerId());
	delayRandomTime(500, 1500);
	//mPlayerInfo->setViewId(++viewId);
	//getStatsByGroupAsync(&mOpponentPlayerInfo, "MyFriends", opponentList, "friend", mPlayerInfo->getPlayerId());
	//getStatsByGroupAsync(mPlayerInfo, "MyFriends");
}


void ESLTournaments::addPlayerToList()
{
	//  add player to universal list first
    Player::addPlayerToList();
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[Friendlies::addPlayerToList]: Player: " << mPlayerInfo->getPlayerId() << ", Game: " << myGameId);
    {
        //((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->friendliesMap.insert(eastl::pair<PlayerId, eastl::string>(mPlayerInfo->getPlayerId(), mPlayerInfo->getPersonaName()));
		PlayerId  id = mPlayerInfo->getPlayerId();
		PlayerIdList& playerIds = ((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->eslPlayerList;
		playerIds.push_back(id);
    }
}

void ESLTournaments::removePlayerFromList()
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

bool ESLTournaments::getESLAccessToken()
{
	bool result = true;
	//get Auth Code
	//eastl::string authCode = "";// mEslAuthCode;
	LoginManager * loginManager = BLAZE_NEW LoginManager();
	eastl::string authCode = loginManager->getAuthCode();
	eastl::string accessToken = "";
	NucleusProxy * nucleusProxy = BLAZE_NEW NucleusProxy(mPlayerData);
	result = nucleusProxy->getPlayerAccessToken("", accessToken);
	if (!result)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[" << mPlayerData->getPlayerId() << "][ESLProxyHandler::getMyAccessToken]: Access token failed");
	}
	else
	{
		mESLAccessToken = accessToken;
	}
	delete nucleusProxy;
	delete loginManager;
	return result;
}

BlazeRpcError ESLTournaments::trustedLogin()
{
	bool result = false;

	ESLHttpResult httpResult;
	eastl::string jsonData;
	const int headerLen = 128;


	char8_t httpHeader_AcceptType[headerLen];
	char8_t httpHeader_ContentType[headerLen];

	blaze_strnzcpy(httpHeader_AcceptType, "Accept: application/json", headerLen);
	blaze_strnzcpy(httpHeader_ContentType, "Content-Type: application/json", headerLen);

	const char8_t *httpHeaders[2];
	httpHeaders[0] = httpHeader_AcceptType;
	httpHeaders[1] = httpHeader_ContentType;

	eastl::string bearer = "{\"accesstoken\":\"NEXUS_S2S ";
	bearer += mESLAccessToken;
	jsonData.append(bearer);
	jsonData.append(",\"id\" : \"sampleId\", \"type\" : \"sampleType\"}");
	const char * loginUri = "";
	loginUri = "10.10.29.78:10005/authentication/trustedLogin";

	OutboundHttpConnection::ContentPartList contentList;
	OutboundHttpConnection::ContentPart content;

	content.mContent = reinterpret_cast<const char8_t*>(jsonData.c_str());
	content.mContentLength = jsonData.size();
	contentList.push_back(&content);

	result = sendESLHttpRequest(
		HttpProtocolUtil::HTTP_POST,
		loginUri,
		httpHeaders,
		sizeof(httpHeaders) / sizeof(char8_t*),
		httpResult,
		contentList
	);

	if (false == result)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[UPSProxy::getMessage:result]:" << mPlayerData->getPlayerId() << " UPS returned error for getMessage.");
		return false;
	}

	if (httpResult.hasError())
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[UPSProxy::getMessage:hasError]:" << mPlayerData->getPlayerId() << " UPS returned error for getMessage");
		return false;
	}

	return true;
}

BlazeRpcError ESLTournaments::loginToBlaze()
{
	bool result = true;

	//get Access Token
	result = getESLAccessToken();
	if (result)
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[UPSProxy::simulateUPSLoad][" << mPlayerData->getPlayerId() << "] getMyAccessToken= " << mESLAccessToken.c_str());
	}
	else
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[UPSProxy::simulateUPSLoad][" << mPlayerData->getPlayerId() << "] getMyAccessToken failed");
		return false;
	}

	//getMessage()
	trustedLogin();

	return true;
}

bool ESLTournaments::initESLProxyHTTPConnection()
{

	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[UPSProxy::initUPSProxyHTTPConnection]: Initializing the HttpConnectionManager");
	mHttpESLProxyConnMgr = BLAZE_NEW OutboundHttpConnectionManager("fifa-2021-ps4-lt");
	if (mHttpESLProxyConnMgr == NULL)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[UPSProxy::initUPSProxyHTTPConnection]: Failed to Initialize the HttpConnectionManager");
		return false;
	}
	const char8_t* serviceHostname = NULL;
	bool serviceSecure = false;



	HttpProtocolUtil::getHostnameFromConfig("http://159.153.62.145", serviceHostname, serviceSecure);
	//BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[UPSProxy::initUPSProxyHTTPConnection]: UPSServerUri= " << StressConfigInst.getUPSServerUri());

	InetAddress* inetAddress;
	uint16_t portNumber = 13402;
	//bool8_t	isSecure = true;
	inetAddress = BLAZE_NEW InetAddress(serviceHostname, portNumber);
	if (inetAddress != NULL)
	{
		mHttpESLProxyConnMgr->initialize(*inetAddress, 1, serviceSecure, false);
	}
	else
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[UPSProxy::initUPSProxyHTTPConnection]: Failed to Create InetAddress with given hostname");
		return false;
	}
	return true;
}

bool ESLTournaments::sendESLHttpRequest(HttpProtocolUtil::HttpMethod method, const char8_t* URI, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult& result, OutboundHttpConnection::ContentPartList& contentList)
{
	BlazeRpcError err = ERR_SYSTEM;
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[UPSProxy::sendUPSHttpRequest][" << mPlayerData->getPlayerId() << "] method= " << method << " URI= " << URI << " httpHeaders= " << httpHeaders);

	err = mHttpESLProxyConnMgr->sendRequest(method, URI, NULL, 0, httpHeaders, headerCount, &result, &contentList);

	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[UPSProxy::sendUPSHttpRequest][" << mPlayerData->getPlayerId() << "] failed to send http request");
		return false;
	}
	return true;
}

bool ESLTournaments::sendESLHttpLoginRequest(HttpProtocolUtil::HttpMethod method, const char8_t* URI, const HttpParam params[], uint32_t paramsize, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult* result)
{
	BlazeRpcError err = ERR_SYSTEM;
	//BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::sendGroupsHttpRequest][" << mPlayerData->getPlayerId() << "] method= " << method << " URI= " << URI << " httpHeaders= " << httpHeaders);
	err = mHttpESLProxyConnMgr->sendRequest(method, URI, params, paramsize, httpHeaders, headerCount, result);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[ESL::sendESLHttpLoginRequest][" << mPlayerData->getPlayerId() << "] failed to send http request");
		return false;
	}
	return true;
}

bool ESLTournaments::expressLogin()
{
	BlazeRpcError err = ERR_SYSTEM;
	bool retValue = false;
	ESLHttpResult httpResult;
	HttpParam params[2];
	// Fill the request here
	uint32_t paramCount = 0;
	const char * expressLoginUri = "/authentication/expressLogin";
		//"https://wal2.tools.gos.bio-dub.ea.com/wal2/fifa-2020-ps4-lt/authentication/expressLogin";

	params[paramCount].name = "mail";
	params[paramCount].value = "ps4fifa20-1000001@ea.com";
	params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "pass";
	params[paramCount].value = "loadtest1234";
	params[paramCount].encodeValue = true;
	paramCount++;

	err = sendESLHttpLoginRequest(HttpProtocolUtil::HTTP_GET,
		expressLoginUri,
		params,
		paramCount,
		NULL,
		0,
		&httpResult);

	if (ERR_OK != err)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[LoginManager::requestAuthCode]: failed send the request");
		//return retValue;
	}
	if (httpResult.hasError())
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[LoginManager::requestAuthCode]: Nucleus returned error");
		return retValue;
	}
	char*sessionKey = httpResult.getSessionKey();

	if (sessionKey != NULL)
	{
		blaze_strnzcpy(mSessionKey, sessionKey, sizeof(mSessionKey));
		BLAZE_INFO_LOG(BlazeRpcLog::usersessions, "Session Key: " << sessionKey);
		retValue = true;
		//delete[] sessionKey;   //delete temporary authCode created on Heap
	}
	else
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "Session Key generated Failed.");
		retValue = false;
	}
	return retValue;
}

BlazeRpcError ESLTournaments::createTournamentGame(int numOfPlayers, CoopPlayerList cooplist)
{
	expressLogin();
	//loginToBlaze();
	bool result = false;
	BLAZE_INFO_LOG(BlazeRpcLog::usersessions, "Session Key: " << mSessionKey);
	string createTournamentGameURL = "/gamemanager/createTournamentGame/";
	createTournamentGameURL += mSessionKey;

	ESLHttpResult httpResult;
	eastl::string jsonData;
	const int headerLen = 128;
	const int accessKey = 1231212;	//	Use the actual accessKey
	int32_t tournamentId = Random::getRandomNumber(100000);

	//{"dimensionSelector":{"gameNames":["FIFA 18"],"platforms":["ps4"],"modeTypes":["ut","story","career","exhibition","createtournament"],"releaseTypes":["prod"]}}
	jsonData.append("{\"templatename\":");
	if (StressConfigInst.getPlatform() == PLATFORM_XONE)
	{
		jsonData.append("\"xoneOrganizedTournament\"");
	}
	else if (StressConfigInst.getPlatform() == PLATFORM_PS4)
	{
		jsonData.append("\"ps4OrganizedTournament\"");
	}
	BLAZE_INFO_LOG(BlazeRpcLog::usersessions, "External ID: " << mPlayerInfo->getExternalId());
	// PS4 - projectId - 312054
	//jsonData.append("\",\"tid\":\"");
	jsonData.append(",\"templateattributes\":{\"TOURNAMENT_ID\": {\"tdfid\":6, \"tdfclass\":\"string\", \"value\": \"");
	jsonData.append(to_string(tournamentId));
	jsonData.append("\" },");
	jsonData.append("\"TOURNAMENT_ORGANIZER\": {\"tdfid\":6, \"tdfclass\" : \"string\", \"value\" : \"ESL\"}, ");
	jsonData.append("\"GAME_EVENT_ADDRESS\": {\"tdfid\":6, \"tdfclass\":\"string\", \"value\":\"https://api.tet.io\"},");
	jsonData.append("\"GAME_START_EVENT_URI\": {\"tdfid\":6, \"tdfclass\":\"string\", \"value\":\"/gi/v1/ea/matches/37309251/start?token=ee2586f8-6dfc-4d05-8229-72ec6c575f32\"},");
	jsonData.append("\"GAME_END_EVENT_URI\": {\"tdfid\":6, \"tdfclass\":\"string\", \"value\":\"/gi/v1/ea/matches/37309251/results?token=ee2586f8-6dfc-4d05-8229-72ec6c575f32\"},");
	//jsonData.append("},");
	
	jsonData.append("\"GAME_LENGTH\":{\"tdfid\":6, \"tdfclass\" : \"string\", \"value\" : \"6\"},");
	jsonData.append("\"CONTROL\" : {\"tdfid\":6, \"tdfclass\" : \"string\", \"value\" : \"ANY\"},");
	jsonData.append("\"DRAW_MODE\" : {\"tdfid\":6, \"tdfclass\" : \"string\", \"value\" : \"EXTRATIME\"},");
	jsonData.append("\"GAME_SPEED\" : {\"tdfid\":6, \"tdfclass\" : \"string\", \"value\" : \"NORMAL\"},");
	jsonData.append("\"SQUAD_TYPE\" : {\"tdfid\":6, \"tdfclass\" : \"string\", \"value\" : \"ONLINE\"},");
	jsonData.append("\"TEAM_LEVEL\" : {\"tdfid\":6, \"tdfclass\" : \"string\", \"value\" : \"ANY\"},");
	jsonData.append("\"STADIUM\" : {\"tdfid\":6, \"tdfclass\" : \"string\", \"value\" : \"ANY\"},");
	jsonData.append("\"TIME\" : {\"tdfid\":6, \"tdfclass\" : \"string\", \"value\" : \"ANY\"},");
	jsonData.append("\"WEATHER\" : {\"tdfid\":6, \"tdfclass\" : \"string\", \"value\" : \"ANY\"}},");

	jsonData.append("\"playerjoindata\": {\"gameentrytype\": \"GAME_ENTRY_TYPE_MAKE_RESERVATION\", ");
	jsonData.append("\"playerdatalist\": [{\"encryptedBlazeId\":\"\",");
	jsonData.append("\"user\":{\"blazeid\":");
	jsonData.append(to_string(cooplist.at(numOfPlayers - 2)));
	jsonData.append("},\"teamindex\" : 0},");
	jsonData.append("{\"encryptedBlazeId\":\"\",\"user\":{\"blazeid\":");
	jsonData.append(to_string(cooplist.at(numOfPlayers - 1)));
	jsonData.append("}, \"teamindex\" : 1");
	/*
	jsonData.append("\"encryptedBlazeId\":\"");
	jsonData.append(to_string(cooplist.at(numOfPlayers + 1))); 
	jsonData.append("\"}, ");
	jsonData.append("{\"encryptedBlazeId\":\"");
	jsonData.append(to_string(cooplist.at(numOfPlayers + 2))); 
	*/jsonData.append("}]}}");
	

	//jsonData.append("\"]}");
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[ESLProxy::createTournament] " << " [" << mPlayerData->getPlayerId() << "] JSONRPC request[" << jsonData.c_str() << "]");

	OutboundHttpConnection::ContentPartList contentList;
	OutboundHttpConnection::ContentPart content;
	content.mName = "content";
	content.mContentType = JSON_CONTENTTYPE;
	content.mContent = reinterpret_cast<const char8_t*>(jsonData.c_str());
	content.mContentLength = jsonData.size();
	contentList.push_back(&content);

	char8_t httpHeader_Application_Key[headerLen];
	char8_t httpHeader_ContentType[headerLen];
	char8_t httpHeader_AcceptType[headerLen];

	blaze_snzprintf(httpHeader_Application_Key, headerLen, "X-Application-Key: <yourWal2AccessKey> %" PRIi64 "", accessKey);
	blaze_strnzcpy(httpHeader_ContentType, "Content-Type: application/json", headerLen);
	blaze_strnzcpy(httpHeader_AcceptType, "Accept: application/json", headerLen);

	const char8_t* httpHeaders[1];
	//httpHeaders[0] = httpHeader_Application_Key;
	httpHeaders[0] = httpHeader_AcceptType;

	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[UPSProxy::getMessage] httpHeaders[0]" << " [" << mPlayerData->getPlayerId() << "] " << httpHeaders[0]);
	//BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[UPSProxy::getMessage] getUPSTrackingUri()" << " [" << mPlayerData->getPlayerId() << "] getMessage URI[" << StressConfigInst.getUPSTrackingUri() << "]");

	result = sendESLHttpRequest(HttpProtocolUtil::HTTP_POST, createTournamentGameURL.c_str(), httpHeaders, sizeof(httpHeaders) / sizeof(char8_t*), httpResult, contentList);

	if (false == result)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[UPSProxy::getMessage:result]:" << mPlayerData->getPlayerId() << " UPS returned error for getMessage.");
		return false;
	}

	if (httpResult.hasError())
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[UPSProxy::getMessage:hasError]:" << mPlayerData->getPlayerId() << " UPS returned error for getMessage");
		return false;
	}

	return true;

}

BlazeRpcError ESLTournaments::getTournamentGame()
{
	bool result = false;

	ESLHttpResult httpResult;
	eastl::string jsonData;
	const int headerLen = 128;
	const int accessKey = 1231212;	//	Use the actual accessKey

	char8_t httpHeader_Application_Key[headerLen];
	char8_t httpHeader_ContentType[headerLen];
	char8_t httpHeader_AcceptType[headerLen];

	blaze_snzprintf(httpHeader_Application_Key, headerLen, "X-Application-Key: <yourWal2AccessKey> %" PRIi64 "", accessKey);
	blaze_strnzcpy(httpHeader_ContentType, "Content-Type: application/json", headerLen);
	blaze_strnzcpy(httpHeader_AcceptType, "Accept: application/json", headerLen);

	const char8_t* httpHeaders[2];
	httpHeaders[0] = httpHeader_Application_Key;
	httpHeaders[1] = httpHeader_AcceptType;

	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[UPSProxy::getMessage] httpHeaders[0]" << " [" << mPlayerData->getPlayerId() << "] " << httpHeaders[0]);
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[UPSProxy::getMessage] getUPSTrackingUri()" << " [" << mPlayerData->getPlayerId() << "] getMessage URI[" << StressConfigInst.getUPSTrackingUri() << "]");


	eastl::string bearer = "{\"accesstoken\":\"NEXUS_S2S ";
	bearer += mESLAccessToken;
	jsonData.append(bearer);
	jsonData.append(",\"id\" : \"sampleId\", \"type\" : \"sampleType\"}");
	const char * getGameUri = "";
	getGameUri = "https://test.wal2.tools.gos.bio-dub.ea.com/wal2/<yourBlazeServiceName>/gamemanager/getTournamentGameStatus/<yourSessionKey>";

	OutboundHttpConnection::ContentPartList contentList;
	OutboundHttpConnection::ContentPart content;

	content.mContent = reinterpret_cast<const char8_t*>(jsonData.c_str());
	content.mContentLength = jsonData.size();
	contentList.push_back(&content);

	result = sendESLHttpRequest(
		HttpProtocolUtil::HTTP_POST,
		getGameUri,
		httpHeaders,
		sizeof(httpHeaders) / sizeof(char8_t*),
		httpResult,
		contentList
	);

	if (false == result)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[UPSProxy::getMessage:result]:" << mPlayerData->getPlayerId() << " UPS returned error for getMessage.");
		return false;
	}

	if (httpResult.hasError())
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[UPSProxy::getMessage:hasError]:" << mPlayerData->getPlayerId() << " UPS returned error for getMessage");
		return false;
	}

	return true;
}

BlazeRpcError ESLTournaments::cancelTournamentGame()
{
	if (expressLogin())
	{
		bool result = false;

		ESLHttpResult httpResult;
		eastl::string jsonData;
		const int headerLen = 128;
		const int accessKey = 1231212;	//	Use the actual accessKey

		char8_t httpHeader_Application_Key[headerLen];
		char8_t httpHeader_ContentType[headerLen];
		char8_t httpHeader_AcceptType[headerLen];

		blaze_snzprintf(httpHeader_Application_Key, headerLen, "X-Application-Key: <yourWal2AccessKey> %" PRIi64 "", accessKey);
		blaze_strnzcpy(httpHeader_ContentType, "Content-Type: application/json", headerLen);
		blaze_strnzcpy(httpHeader_AcceptType, "Accept: application/json", headerLen);

		const char8_t* httpHeaders[2];
		httpHeaders[0] = httpHeader_Application_Key;
		httpHeaders[1] = httpHeader_AcceptType;

		eastl::string gameID = "{\"gameId\":";
		//gameID += to_string(headerLen);	//	Add game id here
		//jsonData.append(gameID);
		jsonData.append("63329169570841}");

		string removeGameUri = "/gamemanager/cancelTournamentGame/";
		removeGameUri += mSessionKey;
		OutboundHttpConnection::ContentPartList contentList;
		OutboundHttpConnection::ContentPart content;

		content.mContent = reinterpret_cast<const char8_t*>(jsonData.c_str());
		content.mContentLength = jsonData.size();
		contentList.push_back(&content);

		result = sendESLHttpRequest(
			HttpProtocolUtil::HTTP_POST,
			removeGameUri.c_str(),
			httpHeaders,
			sizeof(httpHeaders) / sizeof(char8_t*),
			httpResult,
			contentList
		);

		if (false == result)
		{
			BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[ESLProxy::removeGam:result]:" << mPlayerData->getPlayerId() << " UPS returned error for getMessage.");
			return false;
		}

		if (httpResult.hasError())
		{
			BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[ESLProxy::removeGam:hasError]:" << mPlayerData->getPlayerId() << " UPS returned error for getMessage");
			return false;
		}

		return true;
	}
	return false;
}

}
}