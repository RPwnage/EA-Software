//  *************************************************************************************************
//
//   File:    futplayer.cpp
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#include "futplayer.h"
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
#include "shieldextension.h"


namespace Blaze {
namespace Stress {

FutPlayer::FutPlayer(StressPlayerInfo* playerData)
    : GamePlayUser(playerData),
      FutPlayerGameSession(playerData, 2 /*gamesize*/, PEER_TO_PEER_FULL_MESH /*Topology*/,
                             StressConfigInst.getGameProtocolVersionString() /*GameProtocolVersionString*/),
      mIsMarkedForLeaving(false)
{
	if(mScenarioType == FUTCHAMPIONMODE || mScenarioType == FUTDRAFTMODE || mScenarioType == FUTRIVALS)
	{
		mTopology = CLIENT_SERVER_DEDICATED;
	}
	mDSGroupId.id = 0;
	mFutDivision = 0;
	shieldExtension = BLAZE_NEW ShieldExtension();

}

FutPlayer::~FutPlayer()
{
    BLAZE_INFO(BlazeRpcLog::util, "[FutPlayer][Destructor][%" PRIu64 "]: futplayer destroy called", mPlayerInfo->getPlayerId());
	removeFriendFromMap();
	stopReportTelemtery();
    //  deregister async handlers to avoid client crashes when disconnect happens
    deregisterHandlers();
	if (shieldExtension)
		delete shieldExtension;
    //  wait for some time to allow already scheduled jobs to complete
    sleep(30000);
}

void FutPlayer::deregisterHandlers()
{
    FutPlayerGameSession::deregisterHandlers();
}

BlazeRpcError FutPlayer::simulateLoad()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad]:" << mPlayerData->getPlayerId());
    BlazeRpcError err = ERR_OK;
	mScenarioType = StressConfigInst.getRandomScenarioByPlayerType(FUTPLAYER);
	
    err = simulateMenuFlow();
	
	getInvitations(mPlayerInfo, 0, CLUBS_SENT_TO_ME);
	setStatus(mPlayerInfo, 1792);
	if (mScenarioType == FUTCHAMPIONMODE)
	{
		lookupUsersByPersonaNames(mPlayerInfo, "");
	}
	//if (mScenarioType == FUTCHAMPIONMODE || mScenarioType == FUT_PLAY_ONLINE || mScenarioType == FUTFRIENDLIES || mScenarioType == FUTRIVALS )
	{
		setConnectionState(mPlayerInfo);
		setClientState(mPlayerInfo, Blaze::ClientState::Mode::MODE_MULTI_PLAYER, Blaze::ClientState::STATUS_NORMAL);
	}

	if (mScenarioType == FUT_ONLINE_SQUAD_BATTLE)
	{
		setClientState(mPlayerInfo, Blaze::ClientState::MODE_SINGLE_PLAYER, Blaze::ClientState::STATUS_NORMAL);
	}

	/*if (mScenarioType == FUTRIVALS || mScenarioType == FUT_PLAY_ONLINE)
	{
		setConnectionState(mPlayerInfo);
		setClientState(mPlayerInfo, Blaze::ClientState::Mode::MODE_MULTI_PLAYER, Blaze::ClientState::Status::STATUS_NORMAL);
	}*/

	if (mScenarioType == FUTDRAFTMODE)
	{
		lookupUsersByPersonaNames(mPlayerInfo, "");
		localizeStrings(mPlayerInfo);
	}
	
	/*if (mScenarioType == FUTDRAFTMODE)
	{
		if (isPlatformHost)
		{
			Blaze::PersonaNameList personaNameList;		//	Changed as per log
			personaNameList.push_back("q7764733053-GBen");
			personaNameList.push_back("CGFUT19_04");
			personaNameList.push_back("dr_hoo");
			personaNameList.push_back("CGFUT19_02");
			lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
			personaNameList.pop_back();
			lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
			personaNameList.push_back("CGFUT19_02");
			lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
			personaNameList.pop_back();
			lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
			personaNameList.push_back("CGFUT19_02");
			lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
			lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
			lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
			lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
			lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
			personaNameList.pop_back();
			lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
			personaNameList.push_back("CGFUT19_02");
			lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		}
		else
		{
			Blaze::PersonaNameList personaNameList;
			personaNameList.push_back("dr_hoo");
			for (int i = 0; i < 29; i++)	//	Called 29 times
			{
				lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
			}
		}
	}*/
	
	
	if (mScenarioType != FUTDRAFTMODE && mScenarioType != FUTCHAMPIONMODE && mScenarioType != FUTFRIENDLIES && mScenarioType != FUT_COOP_RIVALS && mScenarioType != FUT_PLAY_ONLINE)
	{
		Blaze::PersonaNameList personaNameList;
		personaNameList.push_back("eusilentehren");
		personaNameList.push_back("q3b5727d127-GBen");
		personaNameList.push_back("q3b5727d127-GBen");
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
	}
	/*if (!isPlatformHost && mScenarioType == FUT_PLAY_ONLINE)
	{
		Blaze::PersonaNameList personaNameList;
		personaNameList.push_back("fut19mrtin");
		personaNameList.push_back("q7ca7112d23-USen");
		personaNameList.push_back("qc125233cf2-USen");
		personaNameList.push_back("qace59768af-GBen");
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
	}*/
	// lookupUsersByPersonaNames() with offline players will be added as part of Stats and Leaderboards calls
	//Choose one of the scenario type
	if (StressConfigInst.isOnlineScenario(mScenarioType))
	{
		mPlayerInfo->setShieldExtension(shieldExtension);
		mPlayerInfo->getShieldExtension()->perfTune(mPlayerInfo, true);
	}
	switch (mScenarioType)
    {
       
		case FUTONLINESINGLEMATCH:
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type " << mScenarioType);
			}
			break;
		case FUTRIVALS:
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type " << mScenarioType);
		}
		break;
		case FUT_COOP_RIVALS:
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type " << mScenarioType);
		}
		break;
        case FUTDRAFTMODE:
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type " << mScenarioType);
			}
			break;
        case FUTCHAMPIONMODE:
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type " << mScenarioType);
			}
			break;
        case FUTFRIENDLIES:
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type " << mScenarioType);
			}
			break;
       
			
		case FUTTOURNAMENT:
			{
				//Scenario deprecated
				mScenarioType = FUTDRAFTMODE;
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type " << mScenarioType);
			}
			break;
		case FUT_PLAY_ONLINE:
		{
			//Scenario deprecated
			mScenarioType = FUT_PLAY_ONLINE;
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type " << mScenarioType);
		}
		break;
		case FUT_ONLINE_SQUAD_BATTLE:
		{
			//Scenario deprecated
			mScenarioType = FUT_ONLINE_SQUAD_BATTLE;
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type " << mScenarioType);
		}
		break;

		default:
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " In default state");
				mScenarioType = FUTONLINESINGLEMATCH;
			}
            break;
    }

	sleepRandomTime(5000, 30000);

	if (mScenarioType == FUT_COOP_RIVALS || mScenarioType == FUT_PLAY_ONLINE || mScenarioType == FUT_ONLINE_SQUAD_BATTLE)
	{
		checkIfFriendExist();

		mFriendBlazeId = getFriendInMap(mPlayerInfo->getPlayerId());
		if (mFriendBlazeId == 0)
		{
			BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "CoopSeasons:: get Friend failed. " << mPlayerInfo->getPlayerId());
			sleep(60000);
			return err;
		}
	}
	//Send MM request 
	if( mScenarioType != FUTFRIENDLIES)
	{
		if (mScenarioType == FUT_COOP_RIVALS || mScenarioType == FUT_PLAY_ONLINE || mScenarioType == FUT_ONLINE_SQUAD_BATTLE)
		{
			err = createOrJoinGame();
			sleep(30000); // sleep for 30sec

			if (ERR_OK != err)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::simulateLoad]: createOrJoinGame failed - Voip Group " << mPlayerData->getPlayerId());
				unsubscribeFromCensusData(mPlayerData);
				//leave home group
				//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::Calling leaveGameByGroup for gamegroupID] createOrJoinGame failed" << mPlayerData->getPlayerId() << " with GameId - " << mGameGroupId);
				//leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGameGroupId);
				removeFriendFromMap();
				sleep(60000);
				return err;
			}
			for (eastl::set<BlazeId>::iterator citPlayerIt = mPlayerInGameMap.begin(), citPlayerItEnd = mPlayerInGameMap.end();
				citPlayerIt != citPlayerItEnd; ++citPlayerIt)
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopsPlayer::simulateLoad]mPlayerInGameMap for PlayerId = " << mPlayerInfo->getPlayerId() << " is" << *citPlayerIt << " mGameGroupId=" << mGameGroupId);
			}
			if (isGrupPlatformHost)
			{
				uint16_t count = 0;
				while (count <= 10)
				{
					//for (eastl::set<BlazeId>::iterator citPlayerIt = mPlayerInGameMap.begin(), citPlayerItEnd = mPlayerInGameMap.end();
					//	citPlayerIt != citPlayerItEnd; ++citPlayerIt)
					//{
					//	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopsPlayer::simulateLoad]mPlayerInGameMap.PlayerId " << *citPlayerIt << " mGameGroupId=" << mGameGroupId);
					//}
					if (mPlayerInGameGroupMap.size() >= 2)
					{
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[CoopsPlayer::simulateLoad]  mPlayerInGameMap.size reached 2 for mGameGroupId=" << mGameGroupId);
						break;
					}
					count++;
					sleep(30000); // sleep for 30sec
				} // while

				if (mPlayerInGameGroupMap.size() < 2 )
				{
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "CoopsPlayer:: mPlayerInGameMap.size not reached full " << mPlayerInfo->getPlayerId());
					removeFriendFromMap();
					BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::Calling leaveGameByGroup for gamegroupID] mPlayerInGameMap.size not reached full " << mPlayerData->getPlayerId() << " with GameId - " << mGameGroupId);
					leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGameGroupId);
					sleep(60000);
					return err;
				}
			}
			if (mScenarioType != FUT_PLAY_ONLINE && isGrupPlatformHost)
			{
				Collections::AttributeMap attributes;
				attributes["SLCTD_GMID"] = "76";
				attributes["SLCTD_TMSTMP"] = "1588717639";
				setGameAttributes(mGameGroupId, attributes);
				attributes.clear();

				attributes["USER_PRSNT"] = "0";
				char8_t time[MAX_GAMENAME_LEN] = { '\0' };
				blaze_snzprintf(time, sizeof(time), "%" PRId64, Blaze::TimeValue::getTimeOfDay().getSec());
				attributes["USER_TMSTMP"] = time;
				//setPlayerAttributes(mGameGroupId, attributes);

				setClientState(mPlayerInfo, ClientState::MODE_MULTI_PLAYER, ClientState::Status::STATUS_NORMAL);

				attributes["COM_GMID"] = "76";
				attributes["COM_HOST_SQUAD_UPDATE_TIME"] = "0";
				attributes["COM_KITA"] = "5013505";
				attributes["COM_KITC"] = "16";
				attributes["COM_KITG"] = "0";
				attributes["COM_KITH"] = "29556736";
				attributes["COM_OPP_ABBR"] = "";
				attributes["COM_OPP_BADGE"] = "0";
				attributes["COM_OPP_CHEM"] = "0";
				attributes["COM_OPP_ESTD"] = "0";
				attributes["COM_OPP_NAME"] = "";
				attributes["COM_OPP_OVR"] = "0";
				attributes["COM_SIDE"] = "0";
				attributes["COM_STAD"] = "443";
				setGameAttributes(mGameGroupId, attributes);
				attributes.clear();
				attributes["ARB_0"] = "1000140657926_0|1000161197584_0|";
				setGameAttributes(mGameGroupId, attributes);
				attributes.clear();
			}
			if (mScenarioType != FUT_PLAY_ONLINE)
			{
				Blaze::PersonaNameList personaNameList;
				personaNameList.push_back("jecheng");
				personaNameList.push_back("hyubz");
				personaNameList.push_back("q-9e4a76a8-US-en");
				personaNameList.push_back("q-a3202fcc-CA-en");
				lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
				personaNameList.push_back("qa9815183e8-CAen");
				lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
				lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
				lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
				Blaze::Messaging::MessageAttrMap messageAttrMap;
				GameId gameId = getGameId();
				messageAttrMap[1633839993] = "0";
				sendMessage(mPlayerInfo, gameId, messageAttrMap, 1734571116, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
				sendMessage(mPlayerInfo, gameId, messageAttrMap, 1734571116, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
				messageAttrMap[1633841779] = "1";
				sendMessage(mPlayerInfo, gameId, messageAttrMap, 1734571116, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
			}
			if (isGrupPlatformHost && mScenarioType != FUT_ONLINE_SQUAD_BATTLE)
			{
				err = startMatchmakingScenario(mScenarioType);
				if (err != ERR_OK)
				{
					BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::Calling leaveGameByGroup for gamegroupID] startMatchMaking failed" << mPlayerData->getPlayerId() << mPlayerData->getPlayerId() << " with GameId - " << mGameGroupId);
					removeFriendFromMap();
					leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGameGroupId);
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "Failed to StartMatchmaking call");
					sleep(60000);
					return err;
				}
				else
				{
					if (mMatchmakingSessionId == 0)
					{
						return err;
					}
				}
			}
			if (isGrupPlatformHost && mScenarioType == FUT_ONLINE_SQUAD_BATTLE)
			{
				setClientState(mPlayerInfo, Blaze::ClientState::MODE_MULTI_PLAYER, Blaze::ClientState::STATUS_NORMAL);
				err = createGameTemplate();
				if (err != ERR_OK)
				{
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "Failed to creategametemplate call");
					leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGameGroupId);
					return err;
				}
			}
		}
		else
		{
			err = startMatchmakingScenario(mScenarioType);
			if (err != ERR_OK)
			{
				sleep(60000);
				return err;
			}
		}
	}
	else
	{
		err = startPlayFriendGame();
	}

	if(err != ERR_OK) { return err; }
	
	// PlayerTimeStats RPC call
	if (StressConfigInst.getPTSEnabled())
	{
		// Update Eadp Stats(Read and Update Limits)
		if ((uint32_t)Random::getRandomNumber(100) < StressConfigInst.getEadpStatsProbability())
		{
			updateEadpStats(mPlayerInfo);
		}
	}
	//CancelMatchmaking
	//if ( (mScenarioType != FUTFRIENDLIES) && (Random::getRandomNumber(100) < (int)FUT_CONFIG.cancelMatchMakingProbability) )
	//{		
	//	err = cancelMatchMakingScenario(mMatchmakingSessionId);
	//	if(err == ERR_OK)
	//	{
	//		return err;
	//	}
	//}
	
	
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FUT PLayer::simulateLoad]mPlayerInGameMap.size for PlayerId " << mPlayerData->getPlayerId() << " is " << mPlayerInGameMap.size());
	for (eastl::set<BlazeId>::iterator citPlayerIt = mPlayerInGameMap.begin(), citPlayerItEnd = mPlayerInGameMap.end();
			citPlayerIt != citPlayerItEnd; ++citPlayerIt)
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FUT PLayer::simulateLoad]mPlayerInGameMap for PlayerId "<<mPlayerData->getPlayerId()<<" is " << *citPlayerIt << " mGameGroupId=" << mGameGroupId << "mGameID : " << myGameId);
		}

    int64_t playerStateTimer = 0;
    int64_t  playerStateDurationMs = 0;
    GameState   playerStateTracker = getPlayerState();
    uint16_t stateLimit = 0;
    bool ingame_enter = false;
	if (StressConfigInst.isOnlineScenario(mScenarioType))
	{
		sleepRandomTime(70000, 90000);
		sleepRandomTime(70000, 91000);
	}
    while (1)
    {
        if ( !mPlayerData->isConnected())
        {
            BLAZE_ERR_LOG(BlazeRpcLog::util, "[FutPlayer::simulateLoad::User Disconnected = " << mPlayerData->getPlayerId());
            return ERR_DISCONNECTED;
        }
        if (playerStateTracker != getPlayerState())  //  state changed
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad:playerStateTracker]" << mPlayerData->getPlayerId() << " Time Spent in Sate: " << GameStateToString(
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
                if (playerStateDurationMs > FUT_CONFIG.maxPlayerStateDurationMs)
                {
                    BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad:playerStateTracker][" << mPlayerData->getPlayerId() << "]: Player exceded maximum Game state Duration:" << playerStateDurationMs <<
                                   " ms" << " Current Game State:" << GameStateToString(getPlayerState()));
                    if (IN_GAME == getPlayerState())
                    {
                        mIsMarkedForLeaving = true;   //  Player exceed maximum duration in InGAME State so mark this Player to Leave from InGame
                    }
                }
            }
        }
        const char8_t * stateMsg = GameStateToString(getPlayerState());
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad]: Player: " << mPlayerInfo->getPlayerId() << ", GameId: " << myGameId << " Current state : " << stateMsg << " InStateDuration:" <<
                       playerStateDurationMs);
        switch (getPlayerState())
        {
            //   Sleep for all the below states
            case NEW_STATE:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=NEW_STATE");
                    stateLimit++;
                    if (stateLimit >= FUT_CONFIG.GameStateIterationsMax)
                    {
                        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=NEW_STATE for long time and return");
						unsubscribeFromCensusData(mPlayerData);
						if (mScenarioType == FUT_PLAY_ONLINE || mScenarioType == FUT_COOP_RIVALS || mScenarioType == FUT_ONLINE_SQUAD_BATTLE)
						{
							removeFriendFromMap();
							BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::Calling leaveGameByGroup for gamegroupID] from NEW_STATE " << mPlayerData->getPlayerId() << mPlayerData->getPlayerId() << " with GameId - " << mGameGroupId);
							leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGameGroupId);
						}
						sleep(30000); 
						return err;
                    }
                    //  Wait here if resetDedicatedserver() called/player not found for invite.
                    sleep(FUT_CONFIG.GameStateSleepInterval);
                    break;
                }
            case PRE_GAME:
                {
										//Stopping shield 
					//sleep(FUT_CONFIG.GameStateSleepInterval*10);
					if (StressConfigInst.isOnlineScenario(mScenarioType))
					{
						sleep(FUT_CONFIG.GameStateSleepInterval *2);
						mPlayerInfo->getShieldExtension()->perfTune(mPlayerInfo, false);
					}
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=PRE_GAME");
					
					if( isPlatformHost)// && (mScenarioType == FUTFRIENDLIES || mScenarioType == FUTCHAMPIONMODE || /*mScenarioType == FUTDRAFTMODE ||*/ mScenarioType == FUTONLINESINGLEMATCH || mScenarioType == FUTRIVALS || mScenarioType == FUT_ONLINE_SQUAD_BATTLE))
					{
						setGameSettingsMask(mPlayerInfo,myGameId,0,2063);
					}
					/*if (isPlatformHost && mScenarioType == FUTCHAMPIONMODE)
					{
						setPlayerAttributes("REQ", "1");
						setPlayerAttributes("REQ", "2");
					}*/
					if (!isPlatformHost)
					{
						/*if (mScenarioType != FUTFRIENDLIES)
						{
							setClientState(mPlayerInfo, ClientState::MODE_MULTI_PLAYER, ClientState::STATUS_SUSPENDED);
						}*/
						setPlayerAttributes("REQ", "1");
						
						if (mScenarioType == FUTFRIENDLIES)
						{
							lookupUsersByPersonaNames(mPlayerInfo, "");
						}
						setPlayerAttributes("REQ", "2");
						
					}

					if (isPlatformHost && mScenarioType == FUTFRIENDLIES)
					{
						Blaze::PersonaNameList personaNameList;
						personaNameList.push_back("fut19mrtin");
						personaNameList.push_back("q7ca7112d23-USen");
						personaNameList.push_back("qc125233cf2-USen");
						personaNameList.push_back("qace59768af-GBen");
						lookupUsersByPersonaNames(mPlayerInfo, personaNameList);

					}
					if ((mScenarioType == FUT_COOP_RIVALS || mScenarioType == FUT_ONLINE_SQUAD_BATTLE || mScenarioType == FUT_PLAY_ONLINE) && isGrupPlatformHost)
					{
						advanceGameState(IN_GAME);
					}
					if ((mScenarioType != FUT_COOP_RIVALS) && (mScenarioType != FUT_ONLINE_SQUAD_BATTLE) && (mScenarioType != FUT_PLAY_ONLINE) && isPlatformHost)
					{
							advanceGameState(IN_GAME);
					}
                    sleep(FUT_CONFIG.GameStateSleepInterval);
                    break;
                }
            case IN_GAME:
                {
					
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=IN_GAME");
					
                    if (!ingame_enter)
                    {
                        ingame_enter = true;         
						//Start InGame Timer	
						simulateInGame();
						if ((mScenarioType == FUT_COOP_RIVALS || mScenarioType == FUT_ONLINE_SQUAD_BATTLE || mScenarioType == FUT_PLAY_ONLINE) && isGrupPlatformHost)
						{
							setInGameStartTime();
						}
						if ((mScenarioType != FUT_COOP_RIVALS) && (mScenarioType != FUT_ONLINE_SQUAD_BATTLE) && (mScenarioType != FUT_PLAY_ONLINE) && isPlatformHost)
						{
							BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " setInGameStartTime. gameID=" << myGameId);
							setInGameStartTime();
						}
                    }
					
                    if (RollDice(FUT_CONFIG.inGameReportTelemetryProbability))
                    {
                        executeReportTelemetry();
                    }
					if ((mScenarioType == FUT_COOP_RIVALS || mScenarioType == FUT_ONLINE_SQUAD_BATTLE || mScenarioType == FUT_PLAY_ONLINE) && isGrupPlatformHost)
					{
						if (IsInGameTimedOut() || (mIsMarkedForLeaving == true))
						{
							BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " IsInGameTimedOut/mIsMarkedForLeaving=true. gameID=" << myGameId);
							advanceGameState(POST_GAME);
						}
					}
					if ((mScenarioType != FUT_COOP_RIVALS) && (mScenarioType != FUT_ONLINE_SQUAD_BATTLE) && (mScenarioType != FUT_PLAY_ONLINE))
					{
						if ((isPlatformHost && IsInGameTimedOut()) || (mIsMarkedForLeaving == true))
						{
							BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " IsInGameTimedOut/mIsMarkedForLeaving=true. gameID=" << myGameId);
							advanceGameState(POST_GAME);
						}
					}

                    sleep(FUT_CONFIG.GameStateSleepInterval);
                    break;
                }
            case INITIALIZING:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=INITIALIZING");					
					if (isPlatformHost && mScenarioType == FUT_PLAY_ONLINE)
					{
						lookupUsersByPersonaNames(mPlayerInfo, "");
						lookupUsersByPersonaNames(mPlayerInfo, "");
					}
					if (isPlatformHost && (mScenarioType != FUTCHAMPIONMODE) && (mScenarioType != FUTDRAFTMODE) && (mScenarioType != FUT_COOP_RIVALS) && (mScenarioType!= FUT_ONLINE_SQUAD_BATTLE))
					{
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad][" << mPlayerData->getPlayerId() << "calling advanceGameState");
						advanceGameState(PRE_GAME);
					}
					
                    sleep(FUT_CONFIG.GameStateSleepInterval);
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
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=POST_GAME");
				
                    //Submitgamereport is initiated from NOTIFY_NOTIFYGAMESTATECHANGE async notification
					//submitGameReport(mScenarioType);
					//invokeUdatePrimaryExternalSessionForUser(mPlayerInfo, myGameId, "", "", myGameId, UPDATE_PRESENCE_REASON_GAME_LEAVE, GAME_TYPE_GAMESESSION, ACTIVE_CONNECTED);
					
					if (mScenarioType == FUT_COOP_RIVALS || mScenarioType == FUT_PLAY_ONLINE || mScenarioType == FUT_ONLINE_SQUAD_BATTLE) 
					{
						updatePrimaryExternalSessionForUserRequest(mPlayerInfo, myGameId, myGameId, UPDATE_PRESENCE_REASON_GAME_LEAVE, GAME_TYPE_GAMESESSION, ACTIVE_CONNECTED, PRESENCE_MODE_STANDARD, myGameId, GAME_TYPE_GROUP);
						//Leave game
						//BLAZE_INFO(BlazeRpcLog::gamemanager, "[FutPlayer::leaveGame] :  leaving group : " );
						/*BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::Calling leaveGameByGroup for gameID] in POST_GAME " << mPlayerData->getPlayerId() << " with GameId - " << myGameId);
						leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, myGameId);

						BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::Calling leaveGameByGroup for gamegroupID] in POST_GAME " << mPlayerData->getPlayerId() << " with GameId - " << mGameGroupId);
						leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGameGroupId);*/

						//invokeUdatePrimaryExternalSessionForUser(mPlayerInfo,mGameGroupId,"","", mGameGroupId, UPDATE_PRESENCE_REASON_GAME_LEAVE, GAME_TYPE_GROUP, ACTIVE_CONNECTED );
						updatePrimaryExternalSessionForUserRequest(mPlayerInfo, mGameGroupId, mGameGroupId, UPDATE_PRESENCE_REASON_GAME_LEAVE, GAME_TYPE_GROUP, ACTIVE_CONNECTED, PRESENCE_MODE_PRIVATE, mGameGroupId);
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << " meshEndpointsDisconnected for DS" << mDSGroupId.toString());
					}
					else
					{
						//leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, myGameId);

						updatePrimaryExternalSessionForUserRequest(mPlayerInfo, myGameId, myGameId, UPDATE_PRESENCE_REASON_GAME_LEAVE, GAME_TYPE_GAMESESSION, ACTIVE_CONNECTED, PRESENCE_MODE_STANDARD, 0, false);
					}
					//BLAZE_INFO(BlazeRpcLog::gamemanager, "[FutPlayer::leaveGame][%" PRIu64 "]: player left game: GameId = %" PRIu64 "", mPlayerData->getPlayerId(), myGameId);
					//meshEndpointsDisconnected RPC is initiated from NOTIFY_NOTIFYPLAYERREMOVED
					//Remove friend info from the map
					removeFriendFromMap();
					
					postGameCalls();
					delayRandomTime(5000, 10000);
					setPlayerState(DESTRUCTING);
                    
                    sleep(FUT_CONFIG.GameStateSleepInterval);
                    break;
                }
            case DESTRUCTING:
                {
                    //  Player Left Game
                    //  Presently using this state when user is removed from the game and return from this function.
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=DESTRUCTING");
                    setClientState(mPlayerInfo, ClientState::MODE_MENU);
					if (StressConfigInst.isOnlineScenario(mScenarioType))
					{
						mPlayerInfo->getShieldExtension()->perfTune(mPlayerInfo,false);
					}
                    //  Return here
                    return err;
                }
            case MIGRATING:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=MIGRATING");
                    //  mGameData will be freed when NOTIFY_NOTIFYGAMEREMOVED notification comes.
                    sleep(10000);
                    break;
                }
            case GAME_GROUP_INITIALIZED:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=GAME_GROUP_INITIALIZED");
                    sleep(5000);
                    //if (playerStateDurationMs > FUT_CONFIG.MaxGameGroupInitializedStateDuration)
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
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=" << getPlayerState());
                    //  shouldn't come here
                    return err;
                }
        }
    }
}

void FutPlayer::checkIfFriendExist()
{
	BlazeId friendIDValue = 0;
	int count = 1;

	if ((((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->getFutFriendsList())->size() == 0 /*|| Random::getRandomNumber(100) < 50*/)
	{
		//add self to friends list and wait for the friend using map [50%]
		addFriendToList(mPlayerInfo->getPlayerId());
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "CoopSeasons::checkIfFriendExist addFriendToList " << mPlayerInfo->getPlayerId());

		//wait for 5min for a friend
		while (count <= 10 && friendIDValue == 0)
		{
			friendIDValue = getFriendInMap(mPlayerInfo->getPlayerId());
			count++;
			sleep(30000); // sleep for 30sec

		}
	}
	else
	{
		//find a random friend using map [50%]
		friendIDValue = getARandomFriend();
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "CoopSeasons::checkIfFriendExist getARandomFriend " << mPlayerInfo->getPlayerId());

		//wait for 5min for a friend
		while (count <= 10 && friendIDValue == 0)
		{
			friendIDValue = getARandomFriend();
			count++;
			sleep(30000); // sleep for 30sec

		}
	}

	if (friendIDValue != 0)
	{
		//Add friends set to map
		addFutFriendsToMap(mPlayerInfo->getPlayerId(), friendIDValue);
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "CoopSeasons::checkIfFriendExist successful Set " << mPlayerInfo->getPlayerId() << " - " << friendIDValue);
	}
}

BlazeRpcError FutPlayer::createGameTemplate()
{
	BlazeRpcError err = ERR_SYSTEM;
	CreateGameTemplateRequest request;
	JoinGameResponse response;
	MatchmakingCriteriaError error;
	request.getCommonGameData().setGameProtocolVersionString(StressConfigInst.getGameProtocolVersionString());
	request.getCommonGameData().setGameType(GAME_TYPE_GAMESESSION);
	NetworkAddress& hostAddress = request.getCommonGameData().getPlayerNetworkAddress();
	hostAddress.switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
	if (!mPlayerData->isConnected())
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "[FutPlayer::startMatchMaking::User Disconnected = " << mPlayerData->getPlayerId());
		return ERR_SYSTEM;
	}
	hostAddress.getIpPairAddress()->getExternalAddress().setIp(mPlayerData->getConnection()->getAddress().getIp());
	hostAddress.getIpPairAddress()->getExternalAddress().setPort(mPlayerData->getConnection()->getAddress().getPort());
	hostAddress.getIpPairAddress()->getExternalAddress().copyInto(hostAddress.getIpPairAddress()->getInternalAddress());
	hostAddress.getIpPairAddress()->getExternalAddress().setMachineId(0);
	hostAddress.getIpPairAddress()->getMachineId();

	PlayerJoinData& playerJoinData = request.getPlayerJoinData();
	playerJoinData.setGroupId(UserGroupId(4, 2, mGameGroupId));
	playerJoinData.setDefaultRole("");
	playerJoinData.setGameEntryType(GameEntryType::GAME_ENTRY_TYPE_DIRECT);
	playerJoinData.setDefaultSlotType(SLOT_PUBLIC_PARTICIPANT);
	if (mScenarioType == FUT_ONLINE_SQUAD_BATTLE)
	{
		playerJoinData.setDefaultTeamId(ANY_TEAM_ID);	//	as per logs
	}
	else
	{
		playerJoinData.setDefaultTeamId(ANY_TEAM_ID + 1);
	}
	playerJoinData.setDefaultTeamIndex(ANY_TEAM_ID + 1);
	PerPlayerJoinDataList& playerDataList = playerJoinData.getPlayerDataList();
	PerPlayerJoinDataPtr playerJoinDataPtr = playerDataList.pull_back();
	if (playerJoinDataPtr != NULL)
	{
		playerJoinDataPtr->setEncryptedBlazeId("");
		playerJoinDataPtr->setIsOptionalPlayer(false);
		playerJoinDataPtr->getPlayerAttributes().clear();
		playerJoinDataPtr->setSlotType(INVALID_SLOT_TYPE);
		playerJoinDataPtr->setTeamId(ANY_TEAM_ID + 1);
		playerJoinDataPtr->setTeamIndex(ANY_TEAM_ID + 1);
		playerJoinDataPtr->setRole("");
		playerJoinDataPtr->getUser().setExternalId(mPlayerData->getExternalId());
		playerJoinDataPtr->getUser().setBlazeId(mPlayerData->getPlayerId());
		playerJoinDataPtr->getUser().setName(mPlayerData->getPersonaName().c_str());
		playerJoinDataPtr->getUser().setPersonaNamespace("");
		playerJoinDataPtr->getUser().setAccountId(0);
		playerJoinDataPtr->getUser().setAccountLocale(0);
		playerJoinDataPtr->getUser().setAccountCountry(0);
		playerJoinDataPtr->getUser().setOriginPersonaId(0);
		playerJoinDataPtr->getUser().setPidId(0);
	}
	if (request.getTemplateAttributes()["GAME_MODE"] == NULL)
	{
		request.getTemplateAttributes()["GAME_MODE"] = request.getTemplateAttributes().allocate_element(); \
	}
	request.getTemplateAttributes()["GAME_MODE"]->set(71);
	request.setTemplateName("futCoopGameMode");
	//SET_SCENARIO_ATTRIBUTE(request, "GAME_MODE", "71");

	//char8_t buf[20240];
	//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "createGameTemplate RPC : \n" << request.print(buf, sizeof(buf)));
	//STRESS_INFO_LOG(request);
	err = mPlayerInfo->getComponentProxy()->mGameManagerProxy->createGameTemplate(request, response);
	if (err == ERR_OK)
	{
		myGameId = response.getGameId();
		BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "FutPlayer::createGameTemplate successful.for player " << mPlayerData->getPlayerId());// << " mMatchmakingSessionId= " << mMatchmakingSessionId);
	}
	else
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "FutPlayer::createGameTemplate failed. Error(" << ErrorHelp::getErrorName(err) << ")");
	}
	return err;
}

BlazeRpcError FutPlayer::createOrJoinGame()
{
	Blaze::GameManager::CreateGameRequest request;
	Blaze::GameManager::JoinGameResponse response;
	BlazeRpcError err = ERR_OK;

	if (!mPlayerData->isConnected())
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "[CoopSeasons::CreateOrJoinGame::User Disconnected = " << mPlayerData->getPlayerId());
		return ERR_DISCONNECTED;
	}

	CommonGameRequestData& commonData = request.getCommonGameData();
	NetworkAddress& hostAddress = commonData.getPlayerNetworkAddress();
	//NetworkAddress *hostAddress = &request.getCommonGameData().getPlayerNetworkAddress();
	hostAddress.switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
	hostAddress.getIpPairAddress()->getExternalAddress().setIp(mPlayerInfo->getConnection()->getAddress().getIp());
	hostAddress.getIpPairAddress()->getExternalAddress().setPort(mPlayerInfo->getConnection()->getAddress().getPort());
	hostAddress.getIpPairAddress()->getExternalAddress().copyInto(hostAddress.getIpPairAddress()->getInternalAddress());

	TournamentSessionData & tournamentSessionData = request.getTournamentSessionData();
	tournamentSessionData.setArbitrationTimeout(31536000000000);
	tournamentSessionData.setForfeitTimeout(31449600000000);
	tournamentSessionData.setTournamentDefinition("");
	tournamentSessionData.setScheduledStartTime("1970-01-01T00:00:00Z");

	commonData.setGameType(GAME_TYPE_GROUP);
	commonData.setGameProtocolVersionString(getGameProtocolVersionString());
	Collections::AttributeMap& gameAttribs = request.getGameCreationData().getGameAttribs();
	request.setGamePingSiteAlias("");
	request.setGameEventAddress("");
	request.setGameEndEventUri("");
	gameAttribs["Gamemode_Name"] = "FUTCOOP_MATCH";
	gameAttribs["OSDK_gameMode"] = "FUTCOOP_MATCH";

	GameCreationData & gameCreationData = request.getGameCreationData();
	gameCreationData.setDisconnectReservationTimeout(0);
	gameCreationData.setGameModRegister(0);
	gameCreationData.setGameName("FUT Co-op");// gameName.c_str());
	gameCreationData.setPermissionSystemName("");

	//request.getCommonGameData().
	request.getPlayerJoinData().setGameEntryType(GAME_ENTRY_TYPE_DIRECT);  // As per the new TTY;
	request.getPlayerJoinData().setGroupId(mPlayerData->getConnGroupId());

	eastl::string gameName = "FUTCOOP_";
	int64_t playerId = mPlayerInfo->getPlayerId();
	if (playerId % 2 == 0)
	{
		gameName += eastl::to_string(playerId);
	}
	else
	{
		gameName += eastl::to_string(playerId - 1);
	}
	request.setPersistedGameId(gameName.c_str());
	// GSET = 34823 (0x00008807)
	request.getGameCreationData().getGameSettings().setBits(0x00008817);
	request.getGameCreationData().setNetworkTopology(Blaze::NETWORK_DISABLED);
	request.getGameCreationData().setMaxPlayerCapacity(8);
	request.getGameCreationData().setMinPlayerCapacity(1);
	request.getGameCreationData().setPresenceMode(Blaze::PRESENCE_MODE_PRIVATE);
	request.getGameCreationData().setQueueCapacity(0);
	request.getGameCreationData().setVoipNetwork(Blaze::VOIP_PEER_TO_PEER);
	request.getGameCreationData().getTeamIds().push_back(65534);
	request.getGameCreationData().setExternalSessionTemplateName("");
	request.getGameCreationData().setSkipInitializing(false);
	request.getGameCreationData().setGameReportName("");

	request.setGameEventContentType("application/json");
	request.setGameStartEventUri("");
	request.setGameStatusUrl("");  // As per the new TTY;
	request.setServerNotResetable(false);

	request.getSlotCapacitiesMap().clear();
	request.getSlotCapacities().at(SLOT_PUBLIC_PARTICIPANT) = 2;
	request.getSlotCapacities().at(SLOT_PRIVATE_PARTICIPANT) = 0;
	request.getSlotCapacities().at(SLOT_PUBLIC_SPECTATOR) = 0;
	request.getSlotCapacities().at(SLOT_PRIVATE_SPECTATOR) = 0;

	Blaze::GameManager::PerPlayerJoinData * data = request.getPlayerJoinData().getPlayerDataList().pull_back();
	data->getUser().setBlazeId(mPlayerData->getPlayerId());
	if (PLATFORM_PS4 == StressConfigInst.getPlatform())
	{
		data->getUser().setPersonaNamespace("ps3");
	}
	else if (PLATFORM_XONE == StressConfigInst.getPlatform())
	{
		data->getUser().setPersonaNamespace("xbox");
	}
	else
	{
		data->getUser().setName(mPlayerData->getPersonaName().c_str());
	}
	request.getPlayerJoinData().setDefaultSlotType(Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT);
	request.getPlayerJoinData().setDefaultTeamIndex(0);
	request.getPlayerJoinData().setDefaultTeamId(65534);

	request.getCommonGameData().setGameProtocolVersionString(getGameProtocolVersionString());
	TournamentIdentification & tournamentIdentification = request.getTournamentIdentification();
	tournamentIdentification.setTournamentId("");
	tournamentIdentification.setTournamentOrganizer("");

	//char8_t buf[20046];
	//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "-> [FUTSeasonsGameSession][" << mPlayerInfo->getPlayerId() << "]::CreateOrJoinGame" << "\n" << request.print(buf, sizeof(buf)));

	err = mPlayerInfo->getComponentProxy()->mGameManagerProxy->createOrJoinGame(request, response);
	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "CoopSeasons::CreateOrJoinGame  failed. Error(" << ErrorHelp::getErrorName(err) << ")" << mPlayerInfo->getPlayerId());
		return err;
	}
	mGameGroupId = response.getGameId();
	//myGameId = response.getGameId();
	return err;
}

void FutPlayer::addPlayerToList()
{
    //  add player to universal list first
    Player::addPlayerToList();
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::addPlayerToList]: Player: " << mPlayerInfo->getPlayerId() << ", Game: " << myGameId);
    {
        ((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->futPlayerMap.insert(eastl::pair<PlayerId, eastl::string>(mPlayerInfo->getPlayerId(), mPlayerInfo->getPersonaName()));
    }
}

void FutPlayer::removePlayerFromList()
{
    //  remove player from universal list first
    Player::removePlayerFromList();
    PlayerId  id = mPlayerInfo->getPlayerId();
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::removePlayerFromList]: Player: " << id << ", Game: " << myGameId);
    PlayerMap& playerMap = ((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->futPlayerMap;
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

void FutPlayer::simulateInGame()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][FutPlayer]: In Game simulation:" << myGameId);
    
	setClientState(mPlayerInfo, ClientState::MODE_MULTI_PLAYER, ClientState::Status::STATUS_SUSPENDED);
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

void FutPlayer::postGameCalls()
{
    setClientState(mPlayerInfo, Blaze::ClientState::MODE_SINGLE_PLAYER, Blaze::ClientState::STATUS_NORMAL);  
	if (mScenarioType != FUTFRIENDLIES && mScenarioType != FUT_PLAY_ONLINE && mScenarioType != FUTCHAMPIONMODE && mScenarioType != FUT_COOP_RIVALS && mScenarioType != FUTDRAFTMODE)
	{
		lookupUsersByPersonaNames(mPlayerInfo, mPlayerInfo->getPersonaName().c_str());
		getKeyScopesMap(mPlayerInfo);
	}
	if (mScenarioType != FUTCHAMPIONMODE && mScenarioType != FUT_COOP_RIVALS && mScenarioType != FUTDRAFTMODE)
	{
		lookupUsersByPersonaNames(mPlayerInfo, mPlayerInfo->getPersonaName().c_str());
		getKeyScopesMap(mPlayerInfo);
	}
}


bool FutPlayer::executeReportTelemetry()
{
    BlazeRpcError err = ERR_OK;
    if (!mPlayerData->isConnected())
    {
        BLAZE_ERR_LOG(BlazeRpcLog::util, "[FutPlayer::ReportTelemetry::User Disconnected = " << mPlayerData->getPlayerId());
        return false;
    }
    if (getGameId() == 0)
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::ReportTelemetry][GameId for player = " << mPlayerInfo->getPlayerId() << "] not present");
        return false;
    }
    StressGameInfo* ptrGameInfo = FutPlayerGameDataRef(mPlayerInfo).getGameData(getGameId());
    if (ptrGameInfo == NULL)
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[FutPlayer::ReportTelemetry][GameData for game = " << getGameId() << "] not present");
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
	if(mScenarioType == FUTCHAMPIONMODE || mScenarioType == FUTDRAFTMODE || mScenarioType == FUTRIVALS)
	{
		telemetryReport->setRemoteConnGroupId(mDSGroupId.id);
	}
	else
	{
		telemetryReport->setRemoteConnGroupId(getOpponentPlayerInfo().getConnGroupId().id);
	}
	
    err = mPlayerInfo->getComponentProxy()->mGameManagerProxy->reportTelemetry(reportTelemetryReq);
    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "FutPlayer: reportTelemetry failed with err=" << ErrorHelp::getErrorName(err));
    }
    return true;
}

BlazeRpcError FutPlayer::startPlayFriendGame()
{
	BlazeRpcError result = ERR_OK;

	setClientState(mPlayerData,ClientState::MODE_MULTI_PLAYER);
	sleep(15000);
	GameId randGameId = getRandomCreatedGame();
	CreateGameResponse response;
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Friendlies::startPlayFriendGame]: " << mPlayerData->getPlayerId()<<" game ID got was : "<<randGameId);
	if(randGameId == 0)
	{
		result = futPlayFriendCreateGame(response);
		addFutPlayFriendCreatedGame(response.getGameId()); 
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Friendlies::RealFlow]: Called createGame fro real and added to map fro : " << mPlayerData->getPlayerId() <<"game id is :" <<response.getGameId());
	}
	else
	{		
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Friendlies::startPlayFriendGame] Calling JoinGame for : " << randGameId);
		result = futPlayFriendJoinGame(randGameId);
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Friendlies::RealFlow]: Joiner currently : " << mPlayerData->getPlayerId());
	}

	return result;

}

BlazeRpcError FutPlayer::startMatchmakingScenario(ScenarioType scenarioName)
{
	BlazeRpcError err = ERR_SYSTEM;
    StartMatchmakingScenarioRequest request;
	StartMatchmakingScenarioResponse response;
    MatchmakingCriteriaError error;
	request.getCommonGameData().setGameProtocolVersionString(StressConfigInst.getGameProtocolVersionString());
	request.getCommonGameData().setGameType(GAME_TYPE_GAMESESSION);
	NetworkAddress& hostAddress = request.getCommonGameData().getPlayerNetworkAddress();
	hostAddress.switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
	if(!mPlayerData->isConnected())
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "[OnlineSeasons::startMatchMaking::User Disconnected = "<<mPlayerData->getPlayerId());
		return ERR_SYSTEM;
	}
	hostAddress.getIpPairAddress()->getExternalAddress().setIp(mPlayerData->getConnection()->getAddress().getIp());
	hostAddress.getIpPairAddress()->getExternalAddress().setPort(mPlayerData->getConnection()->getAddress().getPort());
	hostAddress.getIpPairAddress()->getExternalAddress().copyInto(hostAddress.getIpPairAddress()->getInternalAddress());
	hostAddress.getIpPairAddress()->getExternalAddress().setMachineId(0);
	hostAddress.getIpPairAddress()->getMachineId();

	PlayerJoinData& playerJoinData = request.getPlayerJoinData();
	if (mScenarioType == FUT_COOP_RIVALS || mScenarioType == FUT_PLAY_ONLINE || mScenarioType == FUT_ONLINE_SQUAD_BATTLE)
	{
		playerJoinData.setGroupId(UserGroupId(4, 2, mGameGroupId));
	}
	else
	{
		playerJoinData.setGroupId(mPlayerInfo->getConnGroupId());
	}

    playerJoinData.setDefaultRole("");
    playerJoinData.setGameEntryType(GameEntryType::GAME_ENTRY_TYPE_DIRECT);
	playerJoinData.setDefaultSlotType(SLOT_PUBLIC_PARTICIPANT);
	if (mScenarioType == FUT_COOP_RIVALS || mScenarioType == FUT_PLAY_ONLINE || mScenarioType == FUTDRAFTMODE || mScenarioType == FUTCHAMPIONMODE)
	{
		playerJoinData.setDefaultTeamId(ANY_TEAM_ID);	//	as per logs
	}
	else
	{
		playerJoinData.setDefaultTeamId(ANY_TEAM_ID+1);
	}
	playerJoinData.setDefaultTeamIndex(ANY_TEAM_ID + 1);
    //playerJoinData.setTeamId(ANY_TEAM_ID);
    //playerJoinData.setTeamIndex(ANY_TEAM_ID + 1);
    PerPlayerJoinDataList& playerDataList = playerJoinData.getPlayerDataList();
    PerPlayerJoinDataPtr playerJoinDataPtr = playerDataList.pull_back();
    if (playerJoinDataPtr != NULL)
	{
		playerJoinDataPtr->setEncryptedBlazeId("");
		playerJoinDataPtr->setIsOptionalPlayer(false);
		playerJoinDataPtr->getPlayerAttributes().clear();
		playerJoinDataPtr->setSlotType(INVALID_SLOT_TYPE);
		if (mScenarioType == FUT_COOP_RIVALS || mScenarioType == FUT_PLAY_ONLINE || mScenarioType == FUTDRAFTMODE || mScenarioType == FUTCHAMPIONMODE)
		{
			playerJoinDataPtr->setTeamId(ANY_TEAM_ID + 1);
		}
		else
		{
			playerJoinDataPtr->setTeamId(ANY_TEAM_ID);
		}
	playerJoinDataPtr->setTeamIndex(ANY_TEAM_ID + 1);
		playerJoinDataPtr->setRole("");
		playerJoinDataPtr->getUser().setExternalId(mPlayerData->getExternalId());
		playerJoinDataPtr->getUser().setBlazeId(mPlayerData->getPlayerId());
		playerJoinDataPtr->getUser().setName(mPlayerData->getPersonaName().c_str());
		playerJoinDataPtr->getUser().setPersonaNamespace("");
		playerJoinDataPtr->getUser().setAccountId(0);
		playerJoinDataPtr->getUser().setAccountLocale(0);
		playerJoinDataPtr->getUser().setAccountCountry(0);
		playerJoinDataPtr->getUser().setOriginPersonaId(0);
		playerJoinDataPtr->getUser().setPidId(0);
	}
	SET_SCENARIO_ATTRIBUTE(request, "CUSTOM_CONTROLLER", "0");
	SET_SCENARIO_ATTRIBUTE(request, "GAMEGROUP_ONLY", "0");
	
	 if (scenarioName == FUTTOURNAMENT)
	 {
		SET_SCENARIO_ATTRIBUTE(request, "CUSTOM_CONTROLLER_THR", "OSDK_matchAny");
		SET_SCENARIO_ATTRIBUTE(request, "FUT_TOURNY_CRIT1", "0");
		SET_SCENARIO_ATTRIBUTE(request, "FUT_TOURNY_CRIT2", "0");
		// Todo implement fut tournamnet ID logic
		SET_SCENARIO_ATTRIBUTE(request, "FUT_TOURNY_ID", "3474");
		SET_SCENARIO_ATTRIBUTE(request, "FUT_TOURNY_RND", "4");
		SET_SCENARIO_ATTRIBUTE(request, "FUT_TOURNY_RND_THR", "matchSimilar");
		SET_SCENARIO_ATTRIBUTE(request, "FUT_NEW_USER", "1");
		SET_SCENARIO_ATTRIBUTE(request, "CUSTOM_CONTROLLER_THR", "OSDK_matchAny");
		SET_SCENARIO_ATTRIBUTE(request, "HALF_LENGTH", "4");
		SET_SCENARIO_ATTRIBUTE(request, "HALF_LENGTH_THR", "");
		SET_SCENARIO_ATTRIBUTE(request, "GAME_MODE", "82");
		SET_SCENARIO_ATTRIBUTE(request, "MATCHUP_HASH", "3246297");
		SET_SCENARIO_ATTRIBUTE(request, "NET_TOP", (uint64_t)1);
		request.setScenarioName("FUT_Online_Tournament");
	}
	else if ( scenarioName == FUTDRAFTMODE)
	{
		int value = Random::getRandomNumber(100);
		if (value < 53)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_TOURNY_RND", "1");
			SET_SCENARIO_ATTRIBUTE(request, "FUT_TOURNY_RND_THR", "matchExact");
		}
		else if (value < 79)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_TOURNY_RND", "2");
			SET_SCENARIO_ATTRIBUTE(request, "FUT_TOURNY_RND_THR", "matchExact");
		}
		else if (value < 92)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_TOURNY_RND", "3");
			SET_SCENARIO_ATTRIBUTE(request, "FUT_TOURNY_RND_THR", "matchExact");
		}
		else if (value < 100)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_TOURNY_RND", "4");
			SET_SCENARIO_ATTRIBUTE(request, "FUT_TOURNY_RND_THR", "matchExact");
		}
		SET_SCENARIO_ATTRIBUTE(request, "CUSTOM_CONTROLLER", "0");
		SET_SCENARIO_ATTRIBUTE(request, "FUT_NEW_USER", "0");
		SET_SCENARIO_ATTRIBUTE(request, "GAMEGROUP_ONLY", "0");
		SET_SCENARIO_ATTRIBUTE(request, "GAME_MODE", "80");
		SET_SCENARIO_ATTRIBUTE(request, "HALF_LENGTH", "4");
		SET_SCENARIO_ATTRIBUTE(request, "HALF_LENGTH_THR", "");
		SET_SCENARIO_ATTRIBUTE(request, "NET_TOP", (uint64_t)130);
		SET_SCENARIO_ATTRIBUTE(request, "MATCHUP_HASH", "4327531");
		SET_SCENARIO_ATTRIBUTE(request, "MATCH_CLUB_TYPE", "0");
		SET_SCENARIO_ATTRIBUTE(request, "MATCH_GUESTS", "0");
		SET_SCENARIO_ATTRIBUTE(request, "WOMEN_RULE", "0");

		request.setScenarioName("FUT_Draft");
	}
	else if (scenarioName == FUT_COOP_RIVALS)
	{

		SET_SCENARIO_ATTRIBUTE(request, "FUT_RIVALS_OVERRIDE_VALUE", (uint64_t)0);
		SET_SCENARIO_ATTRIBUTE(request, "FUT_RIVALS_SEARCH_VALUE", (uint64_t)0);
		SET_SCENARIO_ATTRIBUTE(request, "CUSTOM_CONTROLLER", "0");
		SET_SCENARIO_ATTRIBUTE(request, "FUT_COOP_MATCHMAKING_TYPE", "2");
		SET_SCENARIO_ATTRIBUTE(request, "FUT_NEW_USER", "1");
		SET_SCENARIO_ATTRIBUTE(request, "FUT_RIVALS_THR", "OSDK_matchAny");
		SET_SCENARIO_ATTRIBUTE(request, "GAMEGROUP_ONLY", "1");
		SET_SCENARIO_ATTRIBUTE(request, "GAME_MODE", "76");
		SET_SCENARIO_ATTRIBUTE(request, "HALF_LENGTH_THR", "");
		SET_SCENARIO_ATTRIBUTE(request, "NET_TOP", (uint64_t)1);
		SET_SCENARIO_ATTRIBUTE(request, "MATCHUP_HASH", "4231581");
		SET_SCENARIO_ATTRIBUTE(request, "HALF_LENGTH", "4");
		SET_SCENARIO_ATTRIBUTE(request, "MATCH_CLUB_TYPE", "0");
		SET_SCENARIO_ATTRIBUTE(request, "MATCH_GUESTS", "0");
		SET_SCENARIO_ATTRIBUTE(request, "NET_TOP", 1);
		/*SET_SCENARIO_ATTRIBUTE(request, "PLAYER_COUNT_DESIRED", 4);
		SET_SCENARIO_ATTRIBUTE(request, "PLAYER_COUNT_MAX", 4);
		SET_SCENARIO_ATTRIBUTE(request, "PLAYER_COUNT_MIN", 3);
		SET_SCENARIO_ATTRIBUTE(request, "TOTAL_PLAYER_SLOTS_DESIRED", 4);
		SET_SCENARIO_ATTRIBUTE(request, "TOTAL_PLAYER_SLOTS_MAX", 4);
		SET_SCENARIO_ATTRIBUTE(request, "TOTAL_PLAYER_SLOTS_MIN", 3);*/
		SET_SCENARIO_ATTRIBUTE(request, "WOMEN_RULE", "0");
		request.setScenarioName("FUT_Rivals");
	}
	else if (scenarioName == FUTRIVALS)
	{

		 SET_SCENARIO_ATTRIBUTE(request, "FUT_RIVALS_OVERRIDE_VALUE", (uint64_t)840);
		 SET_SCENARIO_ATTRIBUTE(request, "FUT_RIVALS_SEARCH_VALUE", (uint64_t)840);
		 SET_SCENARIO_ATTRIBUTE(request, "CUSTOM_CONTROLLER", "0");
		 SET_SCENARIO_ATTRIBUTE(request, "FUT_COOP_MATCHMAKING_TYPE", "1");
		 SET_SCENARIO_ATTRIBUTE(request, "FUT_NEW_USER", "1");
		 SET_SCENARIO_ATTRIBUTE(request, "FUT_RIVALS_THR", "OSDK_matchRelax");
		 SET_SCENARIO_ATTRIBUTE(request, "GAMEGROUP_ONLY", "0");
		 SET_SCENARIO_ATTRIBUTE(request, "GAME_MODE", "76");
		 SET_SCENARIO_ATTRIBUTE(request, "HALF_LENGTH", "4");
		 SET_SCENARIO_ATTRIBUTE(request, "HALF_LENGTH_THR", "");
		 SET_SCENARIO_ATTRIBUTE(request, "NET_TOP", (uint64_t)1);
		 SET_SCENARIO_ATTRIBUTE(request, "MATCHUP_HASH", "4327531");
		 SET_SCENARIO_ATTRIBUTE(request, "MATCH_CLUB_TYPE", "0");
		 SET_SCENARIO_ATTRIBUTE(request, "MATCH_GUESTS", "0");
		 SET_SCENARIO_ATTRIBUTE(request, "NET_TOP", 1);
		 SET_SCENARIO_ATTRIBUTE(request, "WOMEN_RULE", "0");
		 request.setScenarioName("FUT_Rivals");
	}

	else if (scenarioName == FUTONLINESINGLEMATCH)
	{
		SET_SCENARIO_ATTRIBUTE(request, "FUT_NEW_USER", "1");
		SET_SCENARIO_ATTRIBUTE(request, "FUT_SKILL_OVERRIDE_VALUE", "1206");
		SET_SCENARIO_ATTRIBUTE(request, "FUT_SKILL_SEARCH_VALUE", "1206");
		SET_SCENARIO_ATTRIBUTE(request, "FUT_SKILL_THR", "OSDK_matchRelax");
		SET_SCENARIO_ATTRIBUTE(request, "FUT_TEAM_OVR", "67");
		SET_SCENARIO_ATTRIBUTE(request, "GAME_MODE", "81");
		SET_SCENARIO_ATTRIBUTE(request, "HALF_LENGTH_THR", "");
		SET_SCENARIO_ATTRIBUTE(request, "NET_TOP", (uint64_t)130);
		SET_SCENARIO_ATTRIBUTE(request, "MATCHUP_HASH", "3832700");
		request.setScenarioName("FUT_Online_Single_Match");
	}
	
	else if ( scenarioName == FUTCHAMPIONMODE )
	{
		 int value = 0;// Random::getRandomNumber(1000);

		if (value < 10)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_OVERRIDE_VALUE", (uint64_t)0);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_SEARCH_VALUE", (uint64_t)0);
		}
		else if (value < 12)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_OVERRIDE_VALUE", (uint64_t)-9);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_SEARCH_VALUE", (uint64_t)-9);
		}
		else if (value < 14)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_OVERRIDE_VALUE", (uint64_t)-8);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_SEARCH_VALUE", (uint64_t)-8);
		}
		else if (value < 17)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_OVERRIDE_VALUE", (uint64_t)-7);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_SEARCH_VALUE", (uint64_t)-7);
		}
		else if (value < 22)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_OVERRIDE_VALUE", (uint64_t)-6);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_SEARCH_VALUE", (uint64_t)-6);
		}
		else if (value < 31)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_OVERRIDE_VALUE", (uint64_t)-5);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_SEARCH_VALUE", (uint64_t)-5);
		}
		else if (value < 36)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_OVERRIDE_VALUE", (uint64_t)-4);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_SEARCH_VALUE", (uint64_t)-4);
		}
		else if (value < 64)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_OVERRIDE_VALUE", (uint64_t)-3);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_SEARCH_VALUE", (uint64_t)-3);
		}
		else if (value < 110)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_OVERRIDE_VALUE", (uint64_t)-2);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_SEARCH_VALUE", (uint64_t)-2);
		}
		else if (value < 187)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_OVERRIDE_VALUE", (uint64_t)-1);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_SEARCH_VALUE", (uint64_t)-1);
		}
		else if (value < 318)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_OVERRIDE_VALUE", (uint64_t)0);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_SEARCH_VALUE", (uint64_t)0);
		}
		else if (value < 439)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_OVERRIDE_VALUE", (uint64_t)1);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_SEARCH_VALUE", (uint64_t)1);
		}
		else if (value < 550)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_OVERRIDE_VALUE", (uint64_t)2);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_SEARCH_VALUE", (uint64_t)2);
		}
		else if (value < 647)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_OVERRIDE_VALUE", (uint64_t)3);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_SEARCH_VALUE", (uint64_t)3);
		}
		else if (value < 730)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_OVERRIDE_VALUE", (uint64_t)4);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_SEARCH_VALUE", (uint64_t)4);
		}
		else if (value < 796)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_OVERRIDE_VALUE", (uint64_t)5);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_SEARCH_VALUE", (uint64_t)5);
		}
		else if (value < 850)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_OVERRIDE_VALUE", (uint64_t)6);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_SEARCH_VALUE", (uint64_t)6);
		}
		else if (value < 892)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_OVERRIDE_VALUE", (uint64_t)7);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_SEARCH_VALUE", (uint64_t)7);
		}
		else if (value < 924)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_OVERRIDE_VALUE", (uint64_t)8);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_SEARCH_VALUE", (uint64_t)8);
		}
		else if (value < 948)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_OVERRIDE_VALUE", (uint64_t)9);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_SEARCH_VALUE", (uint64_t)9);
		}
		else if (value < 965)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_OVERRIDE_VALUE", (uint64_t)10);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_SEARCH_VALUE", (uint64_t)10);
		}
		else if (value < 977)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_OVERRIDE_VALUE", (uint64_t)11);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_SEARCH_VALUE", (uint64_t)11);
		}
		else if (value < 986)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_OVERRIDE_VALUE", (uint64_t)12);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_SEARCH_VALUE", (uint64_t)12);
		}
		else if (value < 992)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_OVERRIDE_VALUE", (uint64_t)13);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_SEARCH_VALUE", (uint64_t)13);
		}
		else if (value < 996)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_OVERRIDE_VALUE", (uint64_t)14);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_SEARCH_VALUE", (uint64_t)14);
		}
		else if (value < 1000)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_OVERRIDE_VALUE", (uint64_t)15);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_CHAMPIONS_FORM_SEARCH_VALUE", (uint64_t)15);
		}

		SET_SCENARIO_ATTRIBUTE(request, "CUSTOM_CONTROLLER", "0");
		SET_SCENARIO_ATTRIBUTE(request, "CUSTOM_CONTROLLER_THR", "OSDK_matchAny");
		SET_SCENARIO_ATTRIBUTE(request, "FUT_NEW_USER", "0");
		SET_SCENARIO_ATTRIBUTE(request, "GAMEGROUP_ONLY", "0");
		SET_SCENARIO_ATTRIBUTE(request, "GAME_MODE", "79");
		SET_SCENARIO_ATTRIBUTE(request, "HALF_LENGTH", "4");

		SET_SCENARIO_ATTRIBUTE(request, "HALF_LENGTH_THR", "OSDK_matchExact");
		SET_SCENARIO_ATTRIBUTE(request, "MATCHUP_HASH", "4327531");
		SET_SCENARIO_ATTRIBUTE(request, "MATCH_CLUB_TYPE", "0");
		SET_SCENARIO_ATTRIBUTE(request, "MATCH_GUESTS", "0");

		SET_SCENARIO_ATTRIBUTE(request, "NET_TOP", (uint64_t)1);
		SET_SCENARIO_ATTRIBUTE(request, "WOMEN_RULE", "0");
		request.setScenarioName("FUT_Champions");
	}
	else if (scenarioName == FUT_PLAY_ONLINE)
	{
		SET_SCENARIO_ATTRIBUTE(request, "CUSTOM_CONTROLLER", "0");
		SET_SCENARIO_ATTRIBUTE(request, "FUT_COOP_MATCHMAKING_TYPE", "2");
		SET_SCENARIO_ATTRIBUTE(request, "FUT_HOUSE_RULE_PREF1", "0");
		SET_SCENARIO_ATTRIBUTE(request, "FUT_HOUSE_RULE_PREF2", "0");
		SET_SCENARIO_ATTRIBUTE(request, "FUT_HOUSE_RULE_PREF3", "0");
		SET_SCENARIO_ATTRIBUTE(request, "FUT_HOUSE_RULE_SEARCH", "1");
		if (isPlatformHost)
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_HOUSE_RULE_WIN_OVERRIDE", (uint64_t)20);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_HOUSE_RULE_WIN_SEARCH", (uint64_t)20);
		}
		else
		{
			SET_SCENARIO_ATTRIBUTE(request, "FUT_HOUSE_RULE_WIN_OVERRIDE", (uint64_t)50);
			SET_SCENARIO_ATTRIBUTE(request, "FUT_HOUSE_RULE_WIN_SEARCH", (uint64_t)50);
		}
		SET_SCENARIO_ATTRIBUTE(request, "FUT_HOUSE_RULE_WIN_THR", "OSDK_matchRelax")
		/*SET_SCENARIO_ATTRIBUTE(request, "CUSTOM_CONTROLLER_THR", "0");
		SET_SCENARIO_ATTRIBUTE(request, "FUT_NEW_USER", "1");
		SET_SCENARIO_ATTRIBUTE(request, "FUT_SKILL_OVERRIDE_VALUE", (uint64_t)1200);
		SET_SCENARIO_ATTRIBUTE(request, "FUT_SKILL_SEARCH_VALUE", (uint64_t)1200);
		SET_SCENARIO_ATTRIBUTE(request, "FUT_SKILL_THR", "OSDK_matchRelax");
		SET_SCENARIO_ATTRIBUTE(request, "FUT_TEAM_OVR", "67");*/
		SET_SCENARIO_ATTRIBUTE(request, "GAMEGROUP_ONLY", "1");
		SET_SCENARIO_ATTRIBUTE(request, "GAME_MODE", "73");
		SET_SCENARIO_ATTRIBUTE(request, "HALF_LENGTH", "4");
		SET_SCENARIO_ATTRIBUTE(request, "HALF_LENGTH_THR", "");
		SET_SCENARIO_ATTRIBUTE(request, "MATCHUP_HASH", "4329378");
		SET_SCENARIO_ATTRIBUTE(request, "MATCH_CLUB_TYPE", "0");
		SET_SCENARIO_ATTRIBUTE(request, "MATCH_GUESTS", "0");
		
		SET_SCENARIO_ATTRIBUTE(request, "NET_TOP", (uint64_t)1);
		/*SET_SCENARIO_ATTRIBUTE(request, "PLAYER_COUNT_DESIRED", (uint64_t)4);
		SET_SCENARIO_ATTRIBUTE(request, "PLAYER_COUNT_MAX", (uint64_t)4);
		SET_SCENARIO_ATTRIBUTE(request, "PLAYER_COUNT_MIN", (uint64_t)3);
		SET_SCENARIO_ATTRIBUTE(request, "TOTAL_PLAYER_SLOTS_DESIRED", (uint64_t)4);
		SET_SCENARIO_ATTRIBUTE(request, "TOTAL_PLAYER_SLOTS_MAX", (uint64_t)4);
		SET_SCENARIO_ATTRIBUTE(request, "TOTAL_PLAYER_SLOTS_MIN", (uint64_t)3);*/

		SET_SCENARIO_ATTRIBUTE(request, "WOMEN_RULE", "0");
		request.setScenarioName("FUT_HouseRules");
	}

	//char8_t buf[20240];
	//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "StartMatchmaking RPC : \n" << request.print(buf, sizeof(buf)));

	err = mPlayerInfo->getComponentProxy()->mGameManagerProxy->startMatchmakingScenario(request,response,error);
	if (err == ERR_OK) 
	{
        mMatchmakingSessionId = response.getScenarioId();
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "FutPlayer::startMatchmakingScenario successful.for player " << mPlayerData->getPlayerId() << " mMatchmakingSessionId= " << mMatchmakingSessionId);
    } 
	else
	{
       BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "FutPlayer::startMatchmakingScenario failed. Error(" << ErrorHelp::getErrorName(err) << ")");
	}
    return err;
	
}

}  // namespace Stress
}  // namespace Blaze
