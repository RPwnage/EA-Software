//  *************************************************************************************************
//
//   File:    clubsgamesession.cpp
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
#include "clubsgamesession.h"
#include "utility.h"
#include "gamereporting/fifa/tdf/fifaclubreport.h"
#include "gamereporting/osdk/tdf/gameosdkclubreport.h"
#include "loginmanager.h"

using namespace Blaze::GameManager;
using namespace Blaze::GameReporting; 

namespace Blaze {
namespace Stress {

ClubsPlayerGameSession::ClubsPlayerGameSession(StressPlayerInfo* playerData, uint32_t GameSize, GameNetworkTopology Topology, GameProtocolVersionString GameProtocolVersionString)
	: GameSession(playerData, GameSize, Topology, GameProtocolVersionString)
{
	mIsGameGroupOwner = false;
	mIsHomeGroupOwner = false;
	mOpponentClubId = 0;
	mHomeGroupGId = 0;
	mVoipGroupGId = 0;
	mGamegroupGId = 0;
	mGameListId = 0;
	mGameReportingId = 0;
}

ClubsPlayerGameSession::~ClubsPlayerGameSession()
{
}

void ClubsPlayerGameSession::asyncHandlerFunction(uint16_t type, EA::TDF::Tdf *notification)
{
	if ( mPlayerData != NULL && mPlayerData->isPlayerActive())
	{
			//  This Notification has not been handled so use the Default Handler
			onGameSessionDefaultAsyncHandler(type, notification);
	}
}

void ClubsPlayerGameSession::onGameSessionDefaultAsyncHandler(uint16_t type, EA::TDF::Tdf* notification)
{
	//  This is abstract class and handle the async handlers in the derived class

	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]:Message Type =" << type);
	switch (type)
	{
		 case GameManagerSlave::NOTIFY_NOTIFYGAMESETUP:
            //  Async notification sent to the player who joined a game.  Triggered by JoinGame Request or a Matchmaking session finding a game.
            {
                NotifyGameSetup* notify = (NotifyGameSetup*)notification;
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::asyncHandlerFunction]: Received  NOTIFY_NOTIFYGAMESETUP." << mPlayerData->getPlayerId());
				/*char8_t buf[404800];
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "NotifygameSetup : \n" << notify->print(buf, sizeof(buf)));*/

				if(notify->getGameData().getGameType() == GAME_TYPE_GAMESESSION)
				{
					handleNotifyGameSetUp(notify);
					mGameReportingId = notify->getGameData().getGameReportingId();
				}
				else if(notify->getGameData().getGameType() == GAME_TYPE_GROUP)
				{
					GameId gameId = notify->getGameData().getGameId();
					GameGroupType gamType;
					if(gameId == mHomeGroupGId)
					{
						gamType = HOMEGROUP;
					}
					else if(gameId == mVoipGroupGId)
					{
						gamType = VOIPGROUP;
					} 
					else if(gameId == mGamegroupGId)
					{
						gamType = GAMEGROUP;
					} 
					else
					{
						BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::NOTIFY_NOTIFYGAMESETUP][" << mPlayerData->getPlayerId() << "]::gameID not match ");
						break;
					}
					handleNotifyGameGroupSetUp(notify, gamType);
				}
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMESTATECHANGE:
            {
                NotifyGameStateChange* notify = (NotifyGameStateChange*)notification;
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMESTATECHANGE, state=" <<
                                notify->getNewGameState());
                if (myGameId == notify->getGameId())
                {
					if( getPlayerState() == IN_GAME && notify->getNewGameState() == POST_GAME )
					{
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::NOTIFY_NOTIFYGAMESTATECHANGE][" << mPlayerData->getPlayerId() << "]: submitGameReport");
						//submitGameReport(mScenarioType

						//Abuse Reporting: File Online Report 
						PlayerAccountMap& mPlayerAccountMap = ((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->playerAccountMap;
						PlayerInGameMap::const_iterator itrEnd = mPlayerInGameMap.end();
						for (PlayerInGameMap::const_iterator itrStart = mPlayerInGameMap.begin(); itrStart != itrEnd; ++itrStart)
						{
							//BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "ClubsPlayerGameSession:submitGameReport::mPlayerInGameMap myGameId = " << myGameId << " , PlayerId= " << mPlayerData->getPlayerId() << " adding report for player " << (*itrStart));
							PlayerId playerID = *itrStart;
							if (playerID != mPlayerData->getPlayerId())
							{
								//Report AI Playernames
								if (StressConfigInst.getFileAIPlayerNamesProbability() > (uint32_t)Random::getRandomNumber(100))
								{
									//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "ClubsPlayerGameSession:fileOnlineReport::mPlayerInGameMap myGameId = " << myGameId << " , PlayerId= " << mPlayerData->getPlayerId() << " with account id : " << mPlayerAccountMap[playerID]);

									eastl::set<eastl::string> aIPlayerNameList;

									while (aIPlayerNameList.size() < 11)
									{
										aIPlayerNameList.insert(StressConfigInst.getRandomAIPlayerName().c_str());
									}

									eastl::string aiPlayerNames;

									eastl::set<eastl::string>::iterator aIplayerNamesItr;
									for (aIplayerNamesItr = aIPlayerNameList.begin(); aIplayerNamesItr != aIPlayerNameList.end(); aIplayerNamesItr++)
									{
										aiPlayerNames += "|";
										aiPlayerNames += *aIplayerNamesItr;
										aiPlayerNames += "|";
										//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "ClubsPlayerGameSession:fileOnlineReport::AI Player Name : " << *aIplayerNamesItr);
									}

									//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "ClubsPlayerGameSession:fileOnlineReport:: AI Player Names :  " << aiPlayerNames);

									eastl::string reportSubject = "Profane Pro Clubs Ai Player Names";
									fileOnlineReport(mPlayerData, getMyClubId(), playerID, mPlayerAccountMap[playerID], Blaze::ContentReporter::ReportContentType::NAME, Blaze::ContentReporter::OnlineReportType::AI_PLAYER_NAMES, reportSubject.c_str(), aiPlayerNames.c_str());
								}

								//Report Avatar Names
								if (StressConfigInst.getFileAvatarNameProbability() > (uint32_t)Random::getRandomNumber(100))
								{
									eastl::string reportSubject = "Profane Pro Clubs Avatar Name";
									fileOnlineReport(mPlayerData, getMyClubId(), playerID, mPlayerAccountMap[playerID], Blaze::ContentReporter::ReportContentType::NAME, Blaze::ContentReporter::OnlineReportType::AVATAR_NAME, reportSubject.c_str(), mAvatarName.c_str());
								}

								//Reprt Clubname
								if (StressConfigInst.getFileClubNameProbability() > (uint32_t)Random::getRandomNumber(100))
								{
									eastl::string clubName = StressConfigInst.getRandomClub();

									eastl::string randomString = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

									//Add max of 10 random characters to the firstName & lastName
									uint32_t randomNum = (uint32_t)Random::getRandomNumber(10);
									for (uint32_t i = 0; i < randomNum; i++)
									{
										clubName += randomString[(uint32_t)Random::getRandomNumber(sizeof(randomString) - 1)];
									}

									eastl::string reportSubject = "Profane Pro Clubs Club Name";
									fileOnlineReport(mPlayerData, getMyClubId(), playerID, mPlayerAccountMap[playerID], Blaze::ContentReporter::ReportContentType::NAME, Blaze::ContentReporter::OnlineReportType::CLUB_NAME, reportSubject.c_str(), clubName.c_str());
								}

								break;
							}
						}

						leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, myGameId);

						leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mVoipGroupGId);
						//leave game group
						leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGamegroupGId);
						//leave home group
						leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mHomeGroupGId);

					}
					if (notify->getNewGameState() == CONNECTION_VERIFICATION && isPlatformHost)
					{
						eastl::string sessionName = "test_fifa-2022-";
						if (StressConfigInst.getPlatform() == PLATFORM_XONE)
						{
							sessionName += "xone";
						}
						else
						{
							sessionName += "ps4";
						}
						sessionName += "_4_1_00000000000000360342";
						invokeUdatePrimaryExternalSessionForUser(mPlayerData, myGameId, sessionName, "OTP", myGameId, GAME_JOIN, GAME_TYPE_GROUP, ACTIVE_CONNECTED);
					}
                    //  Set Game State
                    setPlayerState(notify->getNewGameState());
                    ClubsPlayerGameDataRef(mPlayerData).setGameState(myGameId, notify->getNewGameState());
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMERESET:
            {
                NotifyGameReset* notify = (NotifyGameReset*)notification;
                StressGameInfo* gameInfo = ClubsPlayerGameDataRef(mPlayerData).getGameData(notify->getGameData().getGameId());
                if (gameInfo != NULL && gameInfo->getPlatformHostPlayerId() == mPlayerData->getPlayerId())
                {
                    //  TODO: host specific actions
                }
                BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMERESET.gameId=" <<
                               notify->getGameData().getGameId());
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINING:
            {
                NotifyPlayerJoining* notify = (NotifyPlayerJoining*)notification;
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERJOINING.gameId=" << notify->getGameId());

				//add player if joined game group
				if(notify->getGameId() == mGamegroupGId)
				{
					// Add player to game map
					mPlayerInGameGroupMap.insert(notify->getJoiningPlayer().getPlayerId());
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::NOTIFYPLAYERJOINING:GAME_TYPE_GROUP][ GameGroupID : "<< mGamegroupGId << " joinerId " << notify->getJoiningPlayer().getPlayerId());
				}

				//meshEndpointsConnected for GAME_TYPE_GAMESESSION
				if(notify->getGameId() == myGameId)
				{
					// Add player to game map
					mPlayerInGameMap.insert(notify->getJoiningPlayer().getPlayerId());
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::NOTIFYPLAYERJOINING:GAME_TYPE_GAMESESSION][ GameID : "<< myGameId << " joinerId " << notify->getJoiningPlayer().getPlayerId());

					//meshEndpointsConnected
					BlazeObjectId targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, notify->getJoiningPlayer().getConnectionGroupId());
					meshEndpointsConnected(targetGroupId, 0, 0, myGameId);
				}

				//meshEndpointsConnected for GAME_TYPE_GROUP
				if(notify->getGameId() != myGameId)
				{
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::NOTIFYPLAYERJOINING:GAME_TYPE_GROUP][ GameID : "<< notify->getGameId() << " joinerId " << notify->getJoiningPlayer().getPlayerId());

					//meshEndpointsConnected
					BlazeObjectId targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, notify->getJoiningPlayer().getConnectionGroupId());
					meshEndpointsConnected(targetGroupId, 0, 0, notify->getGameId());
				}
				getGameReportViewInfo(mPlayerData, "ClubsCupRecentGames");
				eastl::string sessionName = "test_fifa-2022-";
				if (StressConfigInst.getPlatform() == PLATFORM_XONE)
				{
					sessionName += "xone";
				}
				else
				{
					sessionName += "ps4";
				}
				sessionName += "_4_1_00000000000000424974";
				invokeUdatePrimaryExternalSessionForUser(mPlayerData, myGameId, sessionName, "OTP", myGameId, GAME_JOIN, GAME_TYPE_GROUP, ACTIVE_CONNECTED);
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS:
            {
                BLAZE_INFO(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::handlerFunction][%" PRIu64 "]: Received  NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS.", mPlayerData->getPlayerId());
                //NotifyGameSetup* notify = (NotifyGameSetup*)notification;
                
                //  In quickmatch scenario NOTIFYGAMESETUP is not recieved anymore.Instead NOTIFYJOININGPLAYERINITIATECONNECTIONS contains all the game related data
                //handleNotifyGameSetUp(notify, GameManagerSlave::NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS);
				
            }
            break;

        case GameManagerSlave::NOTIFY_NOTIFYPLATFORMHOSTINITIALIZED:
            {
                NotifyPlatformHostInitialized* notify = (NotifyPlatformHostInitialized*)notification;
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLATFORMHOSTINITIALIZED.gameId=" <<
                                notify->getGameId());
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMELISTUPDATE:
            {
                //NotifyGameListUpdate* notify = (NotifyGameListUpdate*)notification;
                BLAZE_INFO(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::asyncHandlerFunction][%" PRIu64 "]: Received  NOTIFY_NOTIFYGAMELISTUPDATE.", mPlayerData->getPlayerId());
                
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINCOMPLETED:
            {
                NotifyPlayerJoinCompleted* notify = (NotifyPlayerJoinCompleted*)notification;
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERJOINCOMPLETED.");
				if (notify->getGameId() == myGameId)
				{	
					// Add player to game map
					//mPlayerInGameMap.insert(notify->getPlayerId());
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::NOTIFYPLAYERJOINCOMPLETED:mPlayerInGameMap][ GameID : "<< myGameId << " playerID= " << mPlayerData->getPlayerId() << " joinerId " << notify->getPlayerId());

					if (IsPlayerPresentInList((PlayerModule*)(mPlayerData->getOwner()), notify->getPlayerId(), CLUBSPLAYER) == true)
					{
						uint16_t count = ClubsPlayerGameDataRef(mPlayerData).getPlayerCount(myGameId);
						ClubsPlayerGameDataRef(mPlayerData).setPlayerCount(myGameId, ++count);
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::NOTIFYPLAYERJOINCOMPLETED][" << mPlayerData->getPlayerId() << "]:GameId=" << myGameId << " Player count = " << count);
					}
					eastl::string sessionName = "test_fifa-2022-";
					if (StressConfigInst.getPlatform() == PLATFORM_XONE)
					{
						sessionName += "xone";
					}
					else
					{
						sessionName += "ps4";
					}
					sessionName += "_4_1_00000000000000424974";
					invokeUdatePrimaryExternalSessionForUser(mPlayerData, myGameId, "", "OTP", myGameId, GAME_CREATE, GAME_TYPE_GROUP, ACTIVE_CONNECTED);
				}
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLAYERREMOVED:
            {
                NotifyPlayerRemoved* notify = (NotifyPlayerRemoved*)notification;
				//if Game_Type_Group
				if (mPlayerData->getPlayerId() == notify->getPlayerId() && myGameId != notify->getGameId() )
				{
					BlazeObjectId targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, 0);
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << " meshEndpointsDisconnected for game grouID" << targetGroupId.toString());
					/*char8_t buff[2048];
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager , "Game_Type_Group  NOTIFYPLAYERREMOVED" << "\n" << notify->print(buff, sizeof(buff)));*/

					meshEndpointsDisconnected(targetGroupId, DISCONNECTED, 4, notify->getGameId());
					break;
				}
				//If Game_Type_Gamesession
				StressGameInfo* ptrGameInfo = ClubsPlayerGameDataRef(mPlayerData).getGameData(myGameId);
                if (!ptrGameInfo)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]::receiving notification(" << type << ") and gameId " << myGameId <<
                                    "not present in clubsGamesMap");
                    break;
                }
				/*char8_t buff[2048];
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager , "Game_Type_Gamesession  NOTIFYPLAYERREMOVED" << "\n" << notify->print(buff, sizeof(buff)));*/
				
                if (mPlayerData->getPlayerId() == notify->getPlayerId() && myGameId == notify->getGameId() )
                {
					cancelMatchMakingScenario(mMatchmakingSessionId);
                    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERREMOVED for game [" << notify->getGameId() << "] reason: "<<notify->getPlayerRemovedReason());
					//meshEndpointsConnectionLost [to dedicated server], meshEndpointsDisconnected[to dedicated server],

					if (isPlatformHost)
					{
						//meshEndpointsConnected RPC 
						BlazeObjectId targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, ptrGameInfo->getTopologyHostConnectionGroupId());
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << " meshEndpointsDisconnected for grouID" << targetGroupId.toString());
						meshEndpointsConnectionLost(targetGroupId, 1, myGameId);
						meshEndpointsDisconnected(targetGroupId, DISCONNECTED_PLAYER_REMOVED);
						targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, 0);
						meshEndpointsDisconnected(targetGroupId, DISCONNECTED);
						eastl::string sessionName = "test_fifa-2022-";
						if (StressConfigInst.getPlatform() == PLATFORM_XONE)
						{
							sessionName += "xone";
						}
						else
						{
							sessionName += "ps4";
						}
						sessionName += "_4_1_00000000000000424974";
						invokeUdatePrimaryExternalSessionForUser(mPlayerData, myGameId, "", "OTP", myGameId, GAME_LEAVE, GAME_TYPE_GROUP, ACTIVE_CONNECTED);
					}
					else
					{
						//meshEndpointsDisconnected - DISCONNECTED
						BlazeObjectId targetGroupId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, ptrGameInfo->getTopologyHostConnectionGroupId());
						BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << " meshEndpointsDisconnected for grouID" << targetGroupId.toString());
						meshEndpointsConnectionLost(targetGroupId, 1, myGameId);
						meshEndpointsDisconnected(targetGroupId, DISCONNECTED);
					}
			}
                

            if ( myGameId == notify->getGameId()  && ClubsPlayerGameDataRef(mPlayerData).isPlayerExistInGame(notify->getPlayerId(), notify->getGameId()) == true )
            {
				//mPlayerInGameMap.erase(notify->getPlayerId());
                ClubsPlayerGameDataRef(mPlayerData).removePlayerFromGameData(notify->getPlayerId(), notify->getGameId());
                if (IsPlayerPresentInList((PlayerModule*)(mPlayerData->getOwner()), notify->getPlayerId(), CLUBSPLAYER) == true)
                {
                    uint16_t count = ptrGameInfo->getPlayerCount();
                    ptrGameInfo->setPlayerCount(--count);
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::asyncHandlerFunction]::NOTIFYPLAYERREMOVED[" << mPlayerData->getPlayerId() << "]:GameId=" << myGameId << " Player count = " << count
                                    << "for player=" << notify->getPlayerId());
                }
            }
            //  if all players, in single exe, removed from the game delete the data from map
            if ( ptrGameInfo->getPlayerCount() <= 0)
            {
                BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERREMOVED - Game Id = " <<
                                notify->getGameId() << ".");
                ClubsPlayerGameDataRef(mPlayerData).removeGameData(notify->getGameId());
            }		
			if (mPlayerData->getPlayerId() == notify->getPlayerId() && myGameId == notify->getGameId())
			{
				setPlayerState(POST_GAME);
			}
        }
        break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMEREMOVED:
            {
                //  NotifyGameRemoved* notify = (NotifyGameRemoved*)notification;
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMEREMOVED.");
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
                            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::OnNotifyMatchmakingFailed][" << mPlayerData->getPlayerId() << "]: Matchmake SESSION_TIMED_OUT.");
                        }
                        break;
                    case GameManager::SESSION_CANCELED:
                        {
                            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::OnNotifyMatchmakingFailed][" << mPlayerData->getPlayerId() << "]: Matchmake SESSION_CANCELED.");
                            //  Throw Execption;
                        }
						break;
                    default:
                        break;
                }
				//leave game group
				leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mGamegroupGId);
				//leave voip group
				leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mVoipGroupGId);
				//leave home group
				leaveGameByGroup(mPlayerData->getPlayerId(), GROUP_LEFT, mHomeGroupGId);				
				if(mIsGameGroupOwner)
				{
					setClubGameStatus(false);
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
				if (notify->getMigrationType() == HostMigrationType::PLATFORM_HOST_MIGRATION)
				{
					//   Update the host migration status as soon as migration starts - all players + game server does this.
					updateGameHostMigrationStatus(mPlayerData, notify->getGameId(), HostMigrationType::PLATFORM_HOST_MIGRATION);
				}
				if (notify->getMigrationType() == HostMigrationType::TOPOLOGY_PLATFORM_HOST_MIGRATION)
				{
					//   Update the host migration status as soon as migration starts - all players + game server does this.
					updateGameHostMigrationStatus(mPlayerData, notify->getGameId(), HostMigrationType::TOPOLOGY_HOST_MIGRATION);
					updateGameHostMigrationStatus(mPlayerData, notify->getGameId(), HostMigrationType::PLATFORM_HOST_MIGRATION);
				}
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
							BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::handlerFunction][" << mPlayerData->getPlayerId() << "]: Received  ClubId from NOTIFY_USERADDED : " << getMyClubId());
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
		case GameManagerSlave::NOTIFY_NOTIFYPLAYERATTRIBCHANGE:
		{
			NotifyGameAttribChange* notify = (NotifyGameAttribChange*)notification;
			if (notify->getGameId() == myGameId)  //  check for GameID to ensure we're updating the right Game
			{
			}
			/*	Check the flow
			Blaze::Messaging::MessageAttrMap messageAttrMap;
			GameId gameId = notify->getGameId();
			messageAttrMap[65282] = "OT=18";
			sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP);
			messageAttrMap.clear();
			messageAttrMap[65282] = "OCA=asd";
			sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP);
			messageAttrMap.clear();
			messageAttrMap[65282] = "OTA=99070109";
			sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP);
			messageAttrMap.clear();
			messageAttrMap[65282] = "OTA=99070109";
			sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP);
			messageAttrMap.clear();
			*/
			break;
		}
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
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::handlerFunction][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYMESSAGE.");
				NotifyGameAttribChange* notify = (NotifyGameAttribChange*)notification;
				if (notify->getGameId() == myGameId)  //  check for GameID to ensure we're updating the right Game
				{
				}

				//setGameSettings(0);
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

void ClubsPlayerGameSession::handleNotifyGameSetUp(NotifyGameSetup* notify, uint16_t type /* = GameManagerSlave::NOTIFY_NOTIFYGAMESETUP*/)
{
	if (notify->getGameData().getTopologyHostInfo().getPlayerId() == mPlayerData->getPlayerId())
	{
		isTopologyHost = true;
	}
	if (notify->getGameData().getPlatformHostInfo().getPlayerId() == mPlayerData->getPlayerId()) {
		isPlatformHost = true;
	}

	setGameId(notify->getGameData().getGameId());
	//char8_t buf[20240];
	//BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Friendlies::RealFlow]: NotifyGameSetup hopefully from createGame : \n" << notify->print(buf, sizeof(buf)));
	//  Set Player State
	setPlayerState(notify->getGameData().getGameState());

	if (ClubsPlayerGameDataRef(mPlayerData).isGameIdExist(myGameId))
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << "]::receiving notification(" << type << ") and gameId " <<
			notify->getGameData().getGameId() << "already present in clubsGamesMap");
		//  Player will be added during NOTIFYPLAYERJOINCOMPLETED notification, not here.
	}
	else
	{
		//  create new game data
		ClubsPlayerGameDataRef(mPlayerData).addGameData(notify);
	}

	StressGameInfo* ptrGameInfo = ClubsPlayerGameDataRef(mPlayerData).getGameData(myGameId);
	if (!ptrGameInfo)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "] gameId " << myGameId << " not present in clubsGamesMap");
		return;
	}

	//meshEndpointsConnected RPC to self
	BlazeObjectId targetGroupId = mPlayerData->getConnGroupId();
	//BLAZE_TRACE_RPC_LOG(BlazeRpcLog::gamemanager, "GameSession::meshEndpointsConnected RPC with target group id : \n" );
	meshEndpointsConnected(targetGroupId, 0, 0, notify->getGameData().getGameId());

	//meshEndpointsConnected RPC to Dedicated server
	BlazeObjectId topologyHostConnId = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, notify->getGameData().getTopologyHostInfo().getConnectionGroupId());
	//BLAZE_TRACE_RPC_LOG(BlazeRpcLog::gamemanager, "GameSession::meshEndpointsConnected RPC with topology host conn id : \n"); 
	meshEndpointsConnected(topologyHostConnId, 0, 0, myGameId);

	if (isPlatformHost)
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "calling finalizeGameCreation");
		finalizeGameCreation();
		eastl::string sessionName = "test_fifa-2022-";
		if (StressConfigInst.getPlatform() == PLATFORM_XONE)
		{
			sessionName += "xone";
		}
		else
		{
			sessionName += "ps4";
		}
		sessionName += "_4_1_00000000000000424974";
		invokeUdatePrimaryExternalSessionForUser(mPlayerData, myGameId, sessionName, "OTP", myGameId, GAME_JOIN, GAME_TYPE_GROUP, ACTIVE_CONNECTED);
	}

	if (mGameListId != 0)
	{
		destroyGameList(mPlayerData, mGameListId);
	}
	updateMemberOnlineStatus(mPlayerData, getMyClubId(), Blaze::Clubs::CLUBS_MEMBER_IN_OPEN_SESSION);
	//Add players to the map
	ReplicatedGamePlayerList &roster = notify->getGameRoster();
	if (roster.size() > 0)
	{
		ReplicatedGamePlayerList::const_iterator citEnd = roster.end();
		for (ReplicatedGamePlayerList::const_iterator cit = roster.begin(); cit != citEnd; ++cit)
		{
			PlayerId playerId = (*cit)->getPlayerId();
			mPlayerInGameMap.insert(playerId);
			if (mPlayerData->getPlayerId() != playerId)
			{
				//meshEndpointsConnected RPC to others
				BlazeObjectId targetGroupIdOther = BlazeObjectId(ENTITY_TYPE_CONN_GROUP, (*cit)->getConnectionGroupId());
				meshEndpointsConnected(targetGroupIdOther, 0, 0, myGameId);
			}
		}
	}

	//  Increase player count if found in same stress exe.
	if (IsPlayerPresentInList((PlayerModule*)(mPlayerData->getOwner()), mPlayerData->getPlayerId(), CLUBSPLAYER) == true)
	{
		uint16_t count = ClubsPlayerGameDataRef(mPlayerData).getPlayerCount(myGameId);
		ClubsPlayerGameDataRef(mPlayerData).setPlayerCount(myGameId, ++count);
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::handleNotifyGameSetUp][" << mPlayerData->getPlayerId() << "]:GameId=" << myGameId << " Player count = " << count);
	}
}

void ClubsPlayerGameSession::handleNotifyGameGroupSetUp(NotifyGameSetup* notify, GameGroupType gameGroupType)
{
	if (notify->getGameData().getPlatformHostInfo().getPlayerId() == mPlayerData->getPlayerId())
	{
		isPlatformGameGroupHost = true;
	}
	if (gameGroupType == GAMEGROUP && isPlatformGameGroupHost == true)
	{
		mIsGameGroupOwner = true;
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::handleNotifyGameGroupSetUp:mIsGameGroupOwner(true)] " << mPlayerData->getPlayerId());
	}
	if (gameGroupType == HOMEGROUP && isPlatformGameGroupHost == true)
	{
		mIsHomeGroupOwner = true;
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::handleNotifyGameGroupSetUp:mIsHomeGroupOwner(true)] " << mPlayerData->getPlayerId());
	}

	GameId gameId = notify->getGameData().getGameId();

	//meshEndpointsConnected RPC to self
	BlazeObjectId targetGroupId = mPlayerData->getConnGroupId();
	meshEndpointsConnected(targetGroupId, 0, 0, gameId);

	if (isPlatformGameGroupHost)
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::handleNotifyGameGroupSetUp][" << mPlayerData->getPlayerId() << " calling finalizeGameCreation");
		finalizeGameCreation(gameId);
		//advanceGameState
		advanceGameState(GAME_GROUP_INITIALIZED, gameId);
	}
	/*else
	{
		updateGameSession(gameId);
	}*/

	Collections::AttributeMap attributes;
	Blaze::Messaging::MessageAttrMap messageAttrMap;

	switch (gameGroupType)
	{
		case HOMEGROUP:
		{
			//updateMemberOnlineStatus
			updateMemberOnlineStatus(mPlayerData, getMyClubId(), Blaze::Clubs::CLUBS_MEMBER_IN_GAMEROOM);
			//lookupUsersByPersonaNames(mPlayerData, mPlayerData->getPersonaName().c_str());
			/*char8_t attributeString[MAX_GAMENAME_LEN];
			attributeString[MAX_GAMENAME_LEN - 1] = '\0';
			blaze_snzprintf(attributeString, sizeof(attributeString), "%s", "STG=0~0~0~5~5");*/

			if (isPlatformGameGroupHost)
			{
				attributes.clear();
				if (mScenarioType == CLUBSPRACTICE)
				{
					attributes["GPA"] = "80,74,78,73,58,78,76,85,1";
					setGameAttributes(gameId, attributes);
					attributes.clear();
				}
				else
				{
					attributes["OSDK_ClubIsDDP"] = "false";
					setGameAttributes(gameId, attributes);
					attributes.clear();
					setPlayerAttributes("_A", "1", gameId);
				}
				messageAttrMap.clear();
				messageAttrMap[65282] = "DFC=-1&PAI=-1&TAI=-1";
				
				sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
				setPlayerAttributes("CTM", "F|80|693|ST|25|", gameId);
				
				messageAttrMap.clear();
				messageAttrMap[65282] = "TYP=2";
				sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);

				messageAttrMap.clear();
				messageAttrMap[65282] = "FRM=20";
				sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
				if (mScenarioType == CLUBFRIENDLIES || mScenarioType == CLUBMATCH)
				{
					messageAttrMap.clear();
					messageAttrMap[65282] = "UNF=11354112~11354114~0~-1~-1~-1~-1~0";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
					messageAttrMap.clear();
					messageAttrMap[65282] = "STG=0~0~0~693~693";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
				}
				if (mScenarioType == CLUBSPRACTICE)
				{
					messageAttrMap.clear();
					messageAttrMap[65282] = "UNF=11354112~11354114~0~-1~-1~-1~-1~0";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
					messageAttrMap.clear();
					messageAttrMap[65282] = "STG=0~0~0~693~693";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
				}
				if (mScenarioType == CLUBSCUP)
				{
					messageAttrMap.clear();
					messageAttrMap[65282] = "UNF=11354112~11354114~0~-1~-1~-1~-1~0";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
					messageAttrMap.clear();
					messageAttrMap[65282] = "STG=0~0~0~693~693";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
				}
				if (mScenarioType == CLUBFRIENDLIES)
				{
					setPlayerAttributes("GPA", "80,74,78,73,58,78,76,85,1", gameId);
					setPlayerAttributes("CTM", "F|80|693|ST|25|", gameId);
				}
				if (mScenarioType == CLUBMATCH )
				{
					setPlayerAttributes("GPA", "81,83,75,78,74,80,80,86,1", gameId);
					setPlayerAttributes("CTM", "F|82|99010116|ST|25|", gameId);
					updateCustomTactics(mPlayerData, getMyClubId(), 7); // Need to check the response
				}
				if (mScenarioType == CLUBSPRACTICE)
				{
					setPlayerAttributes("GPA", "82,77,75,79,74,80,80,84,1", gameId);
					setPlayerAttributes("CTM", "F|82|2|ST|25|", gameId);
				}
				if (mScenarioType == CLUBSCUP)
				{
					setPlayerAttributes("GPA", "82,73,78,75,68,77,78,84,1", gameId);
					setPlayerAttributes("CTM", "F|80|1796|ST|25|", gameId);
				}
			}
			else
			{
				//setPlayerAttributes("_A", "1", gameId);
				if (mScenarioType == CLUBSCUP)
				{
					messageAttrMap.clear();
					messageAttrMap[65282] = "TAC=IND:-1,FRM:9,CRC:4228860084448169468";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP);
					setPlayerAttributes("CTM", "F|80|99010116|ST|25|", gameId);
					messageAttrMap.clear();
					messageAttrMap[65282] = "CAP=-1";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP);
					messageAttrMap.clear();
					messageAttrMap[65282] = "CAP=-1";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP);
					messageAttrMap.clear();
					messageAttrMap[65282] = "CAP=-1";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP);
					messageAttrMap.clear();
					messageAttrMap[65282] = "CAP=-1";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP);
					messageAttrMap.clear();
					messageAttrMap[65282] = "CAP=Alaska01";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP);
				}
				if (mScenarioType == CLUBFRIENDLIES)
				{
					messageAttrMap.clear();
					messageAttrMap[65282] = "DFC=-1&PAI=-1&TAI=-1";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
					setPlayerAttributes("CTM", "F|80|99010116|ST|25|", gameId);
					setPlayerAttributes("GPA", "82,74,72,76,70,77,79,84,1", gameId);
					setPlayerAttributes("CTM", "F|80|99010116|ST|25|", gameId);
				}
				if (mScenarioType == CLUBMATCH)
				{
					messageAttrMap.clear();
					messageAttrMap[65282] = "DFC=-1&PAI=-1&TAI=-1";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
					setPlayerAttributes("CTM", "F|80|99010116|ST|25|", gameId);
					setPlayerAttributes("GPA", "82,73,71,75,68,76,78,84,1", gameId);
					setPlayerAttributes("CTM", "F|80|99010116|ST|25|", gameId);
					messageAttrMap.clear();
				}
				if (mScenarioType == CLUBSPRACTICE)
				{
					messageAttrMap.clear();
					messageAttrMap[65282] = "DFC=-1&PAI=-1&TAI=-1";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
					setPlayerAttributes("CTM", "F|82|2|ST|25|", gameId);
					messageAttrMap.clear();
					messageAttrMap[65282] = "CAP=-1";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
					setPlayerAttributes("GPA", "81,82,75,77,73,79,80,86,1", gameId);
					setPlayerAttributes("CTM", "F|82|2|ST|25|", gameId);
				}
			}
			vector<eastl::string> stringIdList;
			stringIdList.push_back("SDB_QMSG0");
			stringIdList.push_back("SDB_QMSG1");
			stringIdList.push_back("SDB_QMSG2");
			stringIdList.push_back("SDB_QMSG3");
			stringIdList.push_back("SDB_QMSG4");
			stringIdList.push_back("SDB_QMSG5");
			stringIdList.push_back("SDB_QMSG6");
			stringIdList.push_back("SDB_QMSG7");
			stringIdList.push_back("SDB_QMSG8");
			localizeStrings(mPlayerData, 1701726018, stringIdList);
			if (isPlatformGameGroupHost && mScenarioType == CLUBSCUP)
			{
				messageAttrMap.clear();
				messageAttrMap[65282] = "TAC=IND:-1,FRM:9,CRC:4228860084448169468";
				sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);

			}
			else if (isPlatformGameGroupHost && mScenarioType == CLUBMATCH)
			{
				fetchCustomTactics(mPlayerData, getMyClubId()); // Need to check the response
				updateCustomTactics(mPlayerData, getMyClubId(), 7);
			}
			//if (isPlatformHost && mScenarioType == CLUBSPRACTICE)
			//{
			//	updateCustomTactics(mPlayerData, getMyClubId(), 7);
			//}
			else if (mScenarioType == CLUBSPRACTICE)
			{
				fetchCustomTactics(mPlayerData, getMyClubId());
			}
			break;
		}
		case VOIPGROUP:
		{
			if (isPlatformGameGroupHost)
			{
				//		Need to check if the flow is correct
				if (mScenarioType == CLUBFRIENDLIES)
				{
					setPlayerAttributes("CTM", "F|80|1808|ST|25|", gameId);
				}
				if (mScenarioType == CLUBMATCH)
				{
					setPlayerAttributes("CTM", "F|81|144|ST|25|", gameId);
				}
				if (mScenarioType == CLUBSPRACTICE)
				{
					setPlayerAttributes("CTM", "F|82|2|ST|25|", gameId);
				}
				messageAttrMap.clear();
				if (mScenarioType == CLUBFRIENDLIES || mScenarioType == CLUBSPRACTICE)
				{
					messageAttrMap[65282] = "TYP=1";
				}
				else if (mScenarioType == CLUBMATCH)
				{
					messageAttrMap[65282] = "TYP=3";
				}
				else if (mScenarioType == CLUBSCUP)
				{
					setPlayerAttributes("CTM", "F|82|99010116|ST|25|", gameId);
					messageAttrMap[65282] = "TYP=1";
				}
				sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
				messageAttrMap.clear();
				messageAttrMap[65282] = "FRM=20";
				if (mScenarioType == CLUBFRIENDLIES || mScenarioType == CLUBMATCH)
				{
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
					messageAttrMap.clear();
					messageAttrMap[65282] = "UNF=29622272~29622274~0~-1~-1~-1~0";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
					messageAttrMap.clear();
					messageAttrMap[65282] = "STG=0~0~0~1808~1808";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
				}
				if (mScenarioType == CLUBSCUP)
				{
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
					messageAttrMap.clear();
					messageAttrMap[65282] = "TAC=IND:-1,FRM:9,CRC:4228860084448169468";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
					messageAttrMap.clear();
					messageAttrMap[65282] = "UNF=7509~7509~1~16234451~10278653~16734520~0";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
					messageAttrMap.clear();
					messageAttrMap[65282] = "STG=0~0~0~1~99010116";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
				}
				if (mScenarioType == CLUBSPRACTICE)
				{
					messageAttrMap.clear();
					messageAttrMap[65282] = "UNF=32768~32770~0~-1~-1~-1~0";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
					messageAttrMap.clear();
					messageAttrMap[65282] = "STG=0~0~0~2~2";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
				}
				attributes.clear();
				if (mScenarioType == CLUBFRIENDLIES || mScenarioType == CLUBSCUP || mScenarioType == CLUBSPRACTICE)
				{
					attributes["DFC"] = "-1";
				}
				else if (mScenarioType == CLUBMATCH)
				{
					attributes["DFC"] = "0";
				}
				setGameAttributes(gameId, attributes);
				attributes.clear();
				if (mScenarioType == CLUBFRIENDLIES || mScenarioType == CLUBSCUP || mScenarioType == CLUBSPRACTICE)
				{
					attributes["PAI"] = "-1";
				}
				else if (mScenarioType == CLUBMATCH)
				{
					attributes["PAI"] = "9";
				}
				setGameAttributes(gameId, attributes);
				attributes.clear();
				if (mScenarioType == CLUBFRIENDLIES || mScenarioType == CLUBSCUP || mScenarioType == CLUBSPRACTICE)
				{
					attributes["TAI"] = "-1";
				}
				else if (mScenarioType == CLUBMATCH)
				{
					attributes["TAI"] = "1";
				}
				setGameAttributes(gameId, attributes);
				if (mScenarioType == CLUBSCUP || mScenarioType == CLUBSPRACTICE)
				{
					attributes.clear();
					attributes["CAP"] = "-1";
					setGameAttributes(gameId, attributes);
				}
				attributes.clear();
				if (mScenarioType == CLUBFRIENDLIES || mScenarioType == CLUBSPRACTICE)
				{
					attributes["TYP"] = "1";
				}
				else if (mScenarioType == CLUBMATCH)
				{
					attributes["TYP"] = "3";
				}
				else if (mScenarioType == CLUBSCUP)
				{
					attributes["TYP"] = "1";
				}
				setGameAttributes(gameId, attributes);
				attributes.clear();
				attributes["FRM"] = "20";
				setGameAttributes(gameId, attributes);
				attributes.clear();
				if (mScenarioType == CLUBFRIENDLIES || mScenarioType == CLUBSCUP)
				{
					attributes["UNF"] = "7509~7509~1~16234451~10278653~16734520~0";
				}
				else if (mScenarioType == CLUBMATCH)
				{
					attributes["UNF"] = "2359296~2359298~0~-1~-1~-1~0";
				}
				else if (mScenarioType == CLUBSPRACTICE)
				{
					attributes["UNF"] = "32768~32770~0~-1~-1~-1~0";
				}
				setGameAttributes(gameId, attributes);
				attributes.clear();
				if (mScenarioType == CLUBFRIENDLIES || mScenarioType == CLUBSCUP)
				{
					attributes["STG"] = "0~0~0~1~99010116";
				}
				else if (mScenarioType == CLUBMATCH)
				{
					attributes["STG"] = "0~0~0~144~144";
				}
				else if (mScenarioType == CLUBSPRACTICE)
				{
					attributes["STG"] = "0~0~0~2~2";
				}
				setGameAttributes(gameId, attributes);

				attributes.clear();
				attributes["CAP"] = "-1";
				setGameAttributes(gameId, attributes);
				attributes.clear();
				if (mScenarioType == CLUBSPRACTICE)
				{
					updateCustomTactics(mPlayerData, getMyClubId(), 8);
				}
				if (mScenarioType == CLUBFRIENDLIES || mScenarioType == CLUBSPRACTICE || mScenarioType == CLUBSCUP)
				{
					attributes["TYP"] = "1";
					setGameAttributes(gameId, attributes);
				}
				else if (mScenarioType == CLUBMATCH)
				{
					attributes["TYP"] = "3";
					setGameAttributes(gameId, attributes);
				}
				else if (mScenarioType == CLUBSCUP)
				{
					attributes["TAC"] = "IND:-1,FRM:9,CRC:4228860084448169468";
					setGameAttributes(gameId, attributes);
				}
				
				attributes.clear();
				attributes["FRM"] = "20";
				setGameAttributes(gameId, attributes);
				attributes.clear();
				if (mScenarioType == CLUBFRIENDLIES || mScenarioType == CLUBSCUP)
				{
					attributes["TAC"] = "IND:-1,FRM:9,CRC:4228860084448169468";
				}
				else if (mScenarioType == CLUBMATCH || mScenarioType == CLUBSPRACTICE)
				{
					attributes["UNF"] = "2359296~2359298~0~-1~-1~-1~0";
				}
				else if (mScenarioType == CLUBSPRACTICE)
				{
					attributes["UNF"] = "32768~32770~0~-1~-1~-1~0";
				}
				setGameAttributes(gameId, attributes);
				attributes.clear();
				if (mScenarioType == CLUBFRIENDLIES || mScenarioType == CLUBSCUP)
				{
					attributes["STG"] = "0~0~0~1~99010116";
				}
				else if (mScenarioType == CLUBMATCH || mScenarioType == CLUBSPRACTICE)
				{
					attributes["STG"] = "0~0~0~144~144";
				}
				else if (mScenarioType == CLUBSPRACTICE)
				{
					attributes["STG"] = "0~0~0~2~2";
				}
				setGameAttributes(gameId, attributes);

				//if (mScenarioType == CLUBSPRACTICE)
				//{
				//	// Need to check for slotid
				//	updateCustomTactics(mPlayerData, getMyClubId(), 9);
				//	updateCustomTactics(mPlayerData, getMyClubId(), 10);
				//	updateCustomTactics(mPlayerData, getMyClubId(), 11);
				//	updateCustomTactics(mPlayerData, getMyClubId(), 6);
				//}
				
				if (mScenarioType == CLUBSPRACTICE)
				{
					messageAttrMap.clear();
					messageAttrMap[65282] = "TAC=IND:-1,FRM:9,CRC:18446744073709551615";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
					messageAttrMap.clear();
					messageAttrMap[65282] = "Clauuuudiu|POS=0";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
					messageAttrMap.clear();
					messageAttrMap[65282] = "Clauuuudiu|RDY=1";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
				}
				
				//	Need to check if the below flow is called from VOIP_GROUP
				messageAttrMap.clear();
				messageAttrMap[65282] = "ungezielt_Zent|POS=0";
				sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
				messageAttrMap.clear();
				messageAttrMap[65282] = "ungezielt_Zent|RDY=1";
				sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
				
				if (mScenarioType == CLUBSPRACTICE)
				{
					setPlayerAttributes("POS", "1", gameId);
				}
				else
				{
					setPlayerAttributes("POS", "0", gameId);
				}
				meshEndpointsConnected(targetGroupId, 0, 0, myGameId);
				setPlayerAttributes("RDY", "1", gameId);
				attributes.clear();
				attributes["CAP"] = "ungezielt_Zent";
				setGameAttributes(gameId, attributes);
				if (mScenarioType == CLUBFRIENDLIES || mScenarioType == CLUBSCUP)
				{
					attributes["CAP"] = "2 Dev 691712877";
				}
				else if (mScenarioType == CLUBMATCH || mScenarioType == CLUBSPRACTICE)
				{
					attributes["CAP"] = "2 Dev 36778560";
				}
				setGameAttributes(gameId, attributes);
				//if (mScenarioType == CLUBFRIENDLIES || mScenarioType == CLUBMATCH || mScenarioType == CLUBSPRACTICE)
			//	{
					setGameAttributes(gameId, attributes);
					setGameAttributes(gameId, attributes);
				//}
				if (mScenarioType == CLUBFRIENDLIES)
				{
					setMetaData2(mPlayerData, getMyClubId(), "", "CLUBSSETTINGS,0,0,0,2,-1,,-1,-1,-1,-1,1655732045");
					messageAttrMap.clear();
					messageAttrMap[65282] = "TYP=2";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
					Blaze::PersonaNameList personaName;
					personaName.push_back("fifafinal321");
					lookupUsersByPersonaNames(mPlayerData, personaName);
				}
				
				if (mScenarioType == CLUBSPRACTICE)
				{
					messageAttrMap.clear();
					messageAttrMap[65282] = "TYP=3";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
					Blaze::PersonaNameList personaName;
					lookupUsersByPersonaNames(mPlayerData, personaName);
				}
				attributes.clear();
				if (mScenarioType == CLUBFRIENDLIES)
				{
					attributes["TYP"] = "2";
				}
				else if (mScenarioType == CLUBMATCH)
				{
					attributes["TYP"] = "0";
				}
				if (mScenarioType == CLUBFRIENDLIES || mScenarioType == CLUBMATCH)
				{
					setGameAttributes(gameId, attributes);
				}
				if (mScenarioType == CLUBMATCH || mScenarioType == CLUBSPRACTICE)
				{
					messageAttrMap.clear();
					messageAttrMap[65282] = "2 Dev 214843230|POS=1";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
				}
			//	if (mScenarioType == CLUBFRIENDLIES || mScenarioType == CLUBSCUP || mScenarioType == CLUBSPRACTICE||mScenarioType==CLUBMATCH)
				//{
					setPlayerAttributes("_V", "1", gameId);
					if (mScenarioType == CLUBSPRACTICE)
					{
						setPlayerAttributes("POS", "0", gameId);
					}
					else
					{
						setPlayerAttributes("POS", "1", gameId);
					}
			//	}
				if (mScenarioType == CLUBSPRACTICE)
				{
					attributes.clear();
					attributes["DFC"] = "0";
					setGameAttributes(gameId, attributes);
					attributes.clear();
					attributes["PAI"] = "9";
					setGameAttributes(gameId, attributes);
					attributes.clear();
					attributes["TAI"] = "1";
					setGameAttributes(gameId, attributes);
					attributes.clear();
					attributes["STA"] = "1";
					setGameAttributes(gameId, attributes);
					updateCustomTactics(mPlayerData, getMyClubId(), 10);
					updateCustomTactics(mPlayerData, getMyClubId(), 11);
					updateCustomTactics(mPlayerData, getMyClubId(), 6);

				}
				if (mScenarioType == CLUBSCUP)
				{
					setMetaData2(mPlayerData, getMyClubId(), "", "CLUBSSETTINGS,0,0,0,1,20,,1,-1,-1,-1,0");
					messageAttrMap.clear();
					messageAttrMap[65282] = "STA=1";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
					attributes.clear();
					attributes["STA"] = "1";
					setGameAttributes(gameId, attributes);
				}
				if (mScenarioType == CLUBMATCH)
				{
					setMetaData2(mPlayerData, getMyClubId(), "", "CLUBSSETTINGS,0,0,0,0,-1,,-1,-1,-1,-1");
					messageAttrMap.clear();
					messageAttrMap[65282] = "TYP=0";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
					attributes.clear();
					attributes["TYP"] = "0";
					setGameAttributes(gameId, attributes);
					setMetaData2(mPlayerData, getMyClubId(), "", "CLUBSSETTINGS,0,0,0,0,21,29622272~29622274~0~-1~-1~-1~0,1808,-1,-1,-1");
					messageAttrMap.clear();
					messageAttrMap[65282] = "STA=1";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP);
					attributes.clear();
					attributes["STA"] = "1";
					setGameAttributes(gameId, attributes);
					updateCustomTactics(mPlayerData, getMyClubId(), 8);
				}
			}
			else
			{
				if (mScenarioType == CLUBSCUP)
				{
					messageAttrMap.clear();
					messageAttrMap[65282] = "CAP=ungezielt_Zent";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
					messageAttrMap.clear();
					messageAttrMap[65282] = "qfae6a7e1ce-DEde|POS=1";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
					messageAttrMap.clear();
					messageAttrMap[65282] = "qfae6a7e1ce-DEde|RDY=1";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
				}
				else
				{
					messageAttrMap.clear();
					messageAttrMap[65282] = "CAP=-1";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP);
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP);
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP);
					meshEndpointsConnected(targetGroupId, 0, 0,myGameId);
					messageAttrMap.clear();
					if (mScenarioType == CLUBFRIENDLIES)
					{
						messageAttrMap[65282] = "CAP=2 Dev 36778560";
					}
					else if (mScenarioType == CLUBMATCH)
					{
						messageAttrMap[65282] = "2 Dev 36778560|POS=0";
					}
					else if (mScenarioType == CLUBSPRACTICE)
					{
						messageAttrMap[65282] = "2 Dev 36778560|POS=1";
					}
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP);
					if (mScenarioType == CLUBSPRACTICE)
					{
						setMetaData2(mPlayerData, getMyClubId(), "", "CLUBSSETTINGS,0,0,0,3,-1,,-1,-1,-1,-1");
					}
					if (mScenarioType == CLUBFRIENDLIES)
					{
						messageAttrMap.clear();
						messageAttrMap[65282] = "2 Dev 214843230|POS=1";
					}
					else if (mScenarioType == CLUBMATCH)
					{
						messageAttrMap.clear();
						messageAttrMap[65282] = "CAP=2 Dev 36778560";
					}
					else if (mScenarioType == CLUBSPRACTICE)
					{
						messageAttrMap.clear();
						messageAttrMap[65282] = "TYP=3";
					}
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP);
					if (mScenarioType == CLUBMATCH)
					{
						setMetaData2(mPlayerData, getMyClubId(), "", "CLUBSSETTINGS,0,0,0,0,-1,,-1,-1,-1,-1");
						messageAttrMap.clear();
						messageAttrMap[65282] = "TYP=0";
						sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
						setMetaData2(mPlayerData, getMyClubId(), "", "CLUBSSETTINGS,0,0,0,0,21,2359296~2359298~0~-1~-1~-1~0,144,0,9,1");
						messageAttrMap.clear();
						messageAttrMap[65282] = "STA=1";
						sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
					}
					if (mScenarioType == CLUBSPRACTICE)
					{
						lookupUsers(mPlayerData, "");
						setMetaData2(mPlayerData, getMyClubId(), "", "CLUBSSETTINGS,0,0,0,3,21,2359296~2359298~0~-1~-1~-1~0,144,0,9,1");
						messageAttrMap.clear();
						messageAttrMap[65282] = "DFC=0&PAI=9&TAI=1";
						sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
						messageAttrMap.clear();
						messageAttrMap[65282] = "STA=1";
						sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP /*BlazeObjectType(0x0004, 2)*/);
					}
					fetchCustomTactics(mPlayerData, getMyClubId());
				}

			}
			break;
		}
		case GAMEGROUP:
		{
			if (isPlatformGameGroupHost)
			{
				updateNetworkInfo(mPlayerData, Util::NatType::NAT_TYPE_OPEN, 0x00000004);
				if (mScenarioType == CLUBSCUP)
				{
					messageAttrMap.clear();
					messageAttrMap[65282] = "STA=0";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP);
					attributes.clear();
					attributes["STA"] = "0";
					setGameAttributes(gameId, attributes);
				}
				else if (mScenarioType == CLUBFRIENDLIES && mScenarioType == CLUBMATCH)
				{
					attributes.clear();
					attributes["_J"] = "0";
					setGameAttributes(gameId, attributes);
					setGameAttributes(gameId, attributes);
					messageAttrMap.clear();
					messageAttrMap[65282] = "STA=0";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP);
					updateNetworkInfo(mPlayerData, Util::NatType::NAT_TYPE_OPEN, 0x00000004);
					updateCustomTactics(mPlayerData, getMyClubId(), 9);
					updateCustomTactics(mPlayerData, getMyClubId(), 10);
					updateCustomTactics(mPlayerData, getMyClubId(), 6);


				}
				else
				{
					attributes.clear();
					attributes["STA"] = "0";
					setGameAttributes(gameId, attributes);
				}
			}
			else
			{
				if (mScenarioType == CLUBSPRACTICE)
				{
					messageAttrMap.clear();
					messageAttrMap[65282] = "STA=0";
					sendMessage(mPlayerData, gameId, messageAttrMap, 1885566060, ENTITY_TYPE_GAME_GROUP);
				}
			}
			//else
			//{
			//	//if joiner
			//	finalizeGameCreation(gameId);
			//}
			break;
		}
		default:
		{
			BLAZE_ERR_LOG(BlazeRpcLog::util, "[ClubsPlayer::handleNotifyGameGroupSetUp: In default state = " << mPlayerData->getPlayerId());
			return;
		}
	}

	//Add players to the game group map
	if (gameGroupType == GAMEGROUP)
	{
		mPlayerInGameGroupMap.insert(mPlayerData->getPlayerId());
		ReplicatedGamePlayerList &roster = notify->getGameRoster();
		if (roster.size() > 0)
		{
			ReplicatedGamePlayerList::const_iterator citEnd = roster.end();
			for (ReplicatedGamePlayerList::const_iterator cit = roster.begin(); cit != citEnd; ++cit)
			{
				PlayerId playerId = (*cit)->getPlayerId();
				mPlayerInGameGroupMap.insert(playerId);
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::handleNotifyGameGroupSetUp:mPlayerInGameGroupMap] " << mPlayerData->getPlayerId() << "  member= " << playerId);
			}
		}
	}
}

BlazeRpcError	ClubsPlayerGameSession::findClubsAsync()
{
	FindClubsRequest request;
	FindClubsAsyncResponse response;
	request.getAcceptanceFlags().setBits(1);
	request.getAcceptanceMask().setBits(1);
	request.setAnyDomain(true);
	request.setAnyTeamId(true);
	request.getClubFilterList().clear();

	int tag = Random::getRandomNumber(3) + 1;
	switch(tag)
	{
	case 1:
		request.getTagList().push_back("PosDEF");
		break;
	case 2:
		request.getTagList().push_back("PosFOR");
		break;
	case 3:
		request.getTagList().push_back("PosGK");
		break;
	default:
		request.getTagList().push_back("PosMID");
		break;
	}

	request.setIncludeClubTags(true);
	request.setTagSearchOperation(CLUB_TAG_SEARCH_IGNORE);
	request.setClubsOrder(Clubs::ClubsOrder::CLUBS_NO_ORDER);;	// CLUBS_ORDER_BY_CREATIONTIME	As per log
	request.setName("clubg");
	request.setRegion(0);
	request.setClubDomainId(0);
	request.setJoinAcceptance(Clubs::SearchRequestorAcceptance::CLUB_ACCEPTS_UNSPECIFIED);
	request.setLanguage("");
	request.setLastGameTimeOffset(0);
	request.setMaxMemberCount(0);
	request.setMinMemberCount(0);
	//eastl::pair<Clubs::MemberOnlineStatus,uint16_t> pairOne = eastl::make_pair<Clubs::MemberOnlineStatus,uint16_t>(Clubs::MemberOnlineStatus::CLUBS_MEMBER_OFFLINE,0);
	/*
	request.getMinMemberOnlineStatusCounts()[Clubs::MemberOnlineStatus::CLUBS_MEMBER_OFFLINE] = 0;
	request.getMinMemberOnlineStatusCounts()[Clubs::MemberOnlineStatus::CLUBS_MEMBER_ONLINE_INTERACTIVE] = 0;
	request.getMinMemberOnlineStatusCounts()[Clubs::MemberOnlineStatus::CLUBS_MEMBER_ONLINE_NON_INTERACTIVE] = 0;
	request.getMinMemberOnlineStatusCounts()[Clubs::MemberOnlineStatus::CLUBS_MEMBER_IN_GAMEROOM] = 2;
	request.getMinMemberOnlineStatusCounts()[Clubs::MemberOnlineStatus::CLUBS_MEMBER_IN_OPEN_SESSION] = 0;
	request.getMinMemberOnlineStatusCounts()[Clubs::MemberOnlineStatus::CLUBS_MEMBER_IN_GAME] = 0;
	*/
	request.setMaxResultCount(80);
	request.setNonUniqueName("");
	request.setOrderMode(Clubs::OrderMode::DESC_ORDER);
	request.setOffset(0);
	request.setPetitionAcceptance(Clubs::SearchRequestorAcceptance::CLUB_ACCEPTS_UNSPECIFIED);
	request.setPasswordOption(Clubs::PasswordOption::CLUB_PASSWORD_IGNORE);
	request.setSeasonLevel(0);
	request.setSkipCalcDbRows(false);
	request.setSkipMetadata(0);
	request.setTeamId(0);
	request.getMemberFilterList().clear();
	BlazeRpcError err = mPlayerData->getComponentProxy()->mClubsProxy->findClubsAsync(request, response);
	return err;
}




BlazeRpcError ClubsPlayerGameSession::submitGameReport(ScenarioType scenarioName)
{
	BlazeRpcError error = ERR_OK;

	StressGameInfo* ptrGameInfo = ClubsPlayerGameDataRef(mPlayerData).getGameData(myGameId);
	if (!ptrGameInfo)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::submitGameReport][" << mPlayerData->getPlayerId() << " gameId " << myGameId << " not present in ClubsPlayerGameData");
		return ERR_SYSTEM;
	}

	//mOpponentClubId
	PlayerInGameMap homePlayerMap;
	PlayerInGameMap opponentPlayerMap;
	mOpponentClubId = 0;
	homePlayerMap.clear();
	opponentPlayerMap.clear();

	//Print all the members in the PlayerInGameMap
	PlayerInGameMap::const_iterator itrEnd = mPlayerInGameMap.end();
	for (PlayerInGameMap::const_iterator itrStart = mPlayerInGameMap.begin(); itrStart != itrEnd; ++itrStart)
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "ClubsPlayerGameSession:submitGameReport::mPlayerInGameMap myGameId = " << myGameId << " , PlayerId= " << mPlayerData->getPlayerId() << " adding report for player " << (*itrStart));
		PlayerId playerID = *itrStart;

		ClubId tempClubId = 0;
		Clubs::GetClubMembershipForUsersRequest request;
		Clubs::GetClubMembershipForUsersResponse response;
		request.getBlazeIdList().push_back(playerID);
		error = mPlayerData->getComponentProxy()->mClubsProxy->getClubMembershipForUsers(request, response);
		if (error == ERR_OK)
		{
			Blaze::Clubs::ClubMemberships* memberShips = response.getMembershipMap()[playerID];
			if (memberShips != nullptr)
			{
				Clubs::ClubMembershipList::iterator it = memberShips->getClubMembershipList().begin();
				tempClubId = (*it)->getClubId();
				//set clubID
				//Home Club ID - getMyClubId()
				//Opponent Clubd ID
				if (mOpponentClubId == 0 && getMyClubId() != tempClubId)
				{
					mOpponentClubId = tempClubId;
				}
				//Add players to map
				if (tempClubId == getMyClubId())
				{
					//homePlayers
					homePlayerMap.insert(playerID);
				}
				else
				{
					//opponentPlayers
					opponentPlayerMap.insert(playerID);
				}
			}
			}
		else
		{
			BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "ClubsPlayerGameSession:submitGameReport. getClubMembershipForUsers failed. " << mPlayerData->getPlayerId());
			return error;
		}
	} //for

	/*if (scenarioName != CLUBSPRACTICE &&  (mPlayerInGameMap.size() != 4 || homePlayerMap.size() == 0 || opponentPlayerMap.size() == 0 || mOpponentClubId == 0 || getMyClubId() == 0))
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "ClubsPlayerGameSession:submitGameReport: something wrong happened return here. " << mPlayerData->getPlayerId());
		return ERR_SYSTEM;
	}
	else if (scenarioName == CLUBSPRACTICE && (mPlayerInGameMap.size() != 2 || homePlayerMap.size() == 0 || opponentPlayerMap.size() == 0 || mOpponentClubId == 0 || getMyClubId() == 0))
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "ClubsPlayerGameSession:submitGameReport: something wrong happened return here. " << mPlayerData->getPlayerId());
		return ERR_SYSTEM;
	}*/

	//Print playerIds and clubID
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "ClubsPlayerGameSession:submitGameReport. Home ClubID= " << getMyClubId());
	PlayerInGameMap::const_iterator iterator_end = homePlayerMap.end();
	PlayerInGameMap::const_iterator iterator_start = homePlayerMap.begin();
	for (; iterator_start != iterator_end; ++iterator_start)
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "ClubsPlayerGameSession:submitGameReport. Home PlayerId= " << (*iterator_start));
	}
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "ClubsPlayerGameSession:submitGameReport. Opponent ClubID= " << mOpponentClubId);
	iterator_end = opponentPlayerMap.end();
	iterator_start = opponentPlayerMap.begin();
	for (; iterator_start != iterator_end; ++iterator_start)
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "ClubsPlayerGameSession:submitGameReport. Opponent PlayerId= " << (*iterator_start));
	}

	//startGameReporting
	GameReporting::SubmitGameReportRequest request;
	request.setFinishedStatus(GAMEREPORT_FINISHED_STATUS_FINISHED);
	uint16_t minScore = 2;
	const int MAX_TEAM_COUNT = 11;
	const int MAX_PLAYER_VALUE = 100000;

	GameReporting::GameReport &gamereport = request.getGameReport();
	gamereport.setGameReportingId(mGameReportingId);
	if (mScenarioType == CLUBSCUP)
	{
		gamereport.setGameReportName("gameType13");
	}
	else
	{
		gamereport.setGameReportName("gameType9");
	}

	Blaze::GameReporting::OSDKClubGameReportBase::OSDKClubGameReport* clubGameReport = BLAZE_NEW Blaze::GameReporting::OSDKClubGameReportBase::OSDKClubGameReport;
	Blaze::GameReporting::OSDKClubGameReportBase::OSDKClubPlayerReport *clubPlayerReport;
	Blaze::GameReporting::FifaClubReportBase::FifaClubsGameReport* fifaClubsGameReport = BLAZE_NEW Blaze::GameReporting::FifaClubReportBase::FifaClubsGameReport;
	Blaze::GameReporting::OSDKGameReportBase::OSDKReport *osdkReport = BLAZE_NEW Blaze::GameReporting::OSDKGameReportBase::OSDKReport;
	Blaze::GameReporting::FifaClubReportBase::FifaClubsPlayerReport* FifaClubsPlayerReport;
	Blaze::GameReporting::OSDKClubGameReportBase::OSDKClubReport* clubReport;
	Blaze::GameReporting::FifaClubReportBase::FifaClubsClubReport* fifaClubClubReport;

	gamereport.setReport(*osdkReport);
	if (osdkReport != NULL)
	{
		Blaze::GameReporting::OSDKGameReportBase::OSDKGameReport& osdkGamereport = osdkReport->getGameReport();//osdkGamereport is wrt GAMR in TTY
		if (mScenarioType == CLUBSCUP)
			osdkGamereport.setGameType("gameType13");
		else
			osdkGamereport.setGameType("gameType9");
		osdkGamereport.setFinishedStatus(1);
		osdkGamereport.setGameTime(5460);
		osdkGamereport.setArenaChallengeId(0);//To confirm setArenaChallengeId takes of type ArenaChallengeId, in log ARID = 0 (0x0000000000000000)
		osdkGamereport.setRoomId(0);
		osdkGamereport.setSimulate(false);
		osdkGamereport.setLeagueId(0);
		osdkGamereport.setRank(false);
		//osdkGamereport.setRoomId(0);
		//osdkGamereport.setSponsoredEventId(0);
		fifaClubsGameReport->setMom("forward-cliff");
		Blaze::GameReporting::Fifa::CommonGameReport& commonGameReport = fifaClubsGameReport->getCommonGameReport();//commonGameReport is CMGR wrt TTY;
		commonGameReport.setAwayKitId(4599);

		//As per the new TTY; Added commom game report set functions
		//To confirm whether need to set commonGameReport anywhere? I think no, since we are setting clubGameReport; osdkGamereport.setCustomGameReport(clubGameReport);
		commonGameReport.setIsRivalMatch(false);
		commonGameReport.setWentToPk(0);
		commonGameReport.setBallId(120);
		commonGameReport.setHomeKitId(1167);
		//commonGameReport.setWentToPk(0);

		//Fill awayStartingXI and homeStartingXI values here

		for (int iteration = 0; iteration < MAX_TEAM_COUNT; iteration++) {
			commonGameReport.getAwayStartingXI().push_back(Random::getRandomNumber(MAX_PLAYER_VALUE));
		}

		for (int iteration = 0; iteration < MAX_TEAM_COUNT; iteration++) {
			commonGameReport.getHomeStartingXI().push_back(Random::getRandomNumber(MAX_PLAYER_VALUE));
		}
		commonGameReport.setStadiumId(395);

	//	fifaClubsGameReport->setMom(mPlayerData->getPersonaName().c_str());

		//As per the new TTY; Added club game report set functions

		clubGameReport->setChallenge(0);
		clubGameReport->setChallengeClubId(0);
		clubGameReport->setClubGameKey("");
		clubGameReport->setMember(0);
		clubGameReport->setGameType(0);
		clubGameReport->setCustomClubGameReport(*fifaClubsGameReport);
		osdkGamereport.setCustomGameReport(*clubGameReport);

		Blaze::GameReporting::OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = osdkReport->getPlayerReports();
		//OsdkPlayerReportsMap.reserve(playerMap.size());

		OsdkPlayerReportsMap.reserve(mPlayerInGameMap.size());
		int count = 0;
		for (PlayerInGameMap::iterator citPlayerIt = mPlayerInGameMap.begin(), citPlayerItEnd = mPlayerInGameMap.end();
			citPlayerIt != citPlayerItEnd; ++citPlayerIt)
		{
			GameManager::PlayerId playerId = *citPlayerIt;
			Blaze::GameReporting::OSDKGameReportBase::OSDKPlayerReport* playerReport = OsdkPlayerReportsMap.allocate_element();
			clubPlayerReport = BLAZE_NEW Blaze::GameReporting::OSDKClubGameReportBase::OSDKClubPlayerReport;

			ClubId playerClubID = 0;
			bool isHomePlayer = false;
			//check if local player
			PlayerInGameMap::const_iterator itrEnds = homePlayerMap.end();
			for (PlayerInGameMap::const_iterator itrStart = homePlayerMap.begin(); itrStart != itrEnds; ++itrStart)
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "ClubsPlayerGameSession:submitGameReport. PlayerId= " << (*itrStart));
				if (playerId == (*itrStart))
				{
					isHomePlayer = true;
				}
			}
			//playerId present in homePlayerMap
			if (isHomePlayer)
			{
				playerClubID = getMyClubId();
			}
			else
			{
				playerClubID = mOpponentClubId;
			}
			clubPlayerReport->setClubId(playerClubID);
			clubPlayerReport->setPos((rand() % 15) + 1);//To confirm Why +1; in log it is 4
			//As per the new TTY; Added clubPlayerReport set functions
			clubPlayerReport->setChallenge(0);
			clubPlayerReport->setChallengePoints(0);
			clubPlayerReport->setClubGames(0);
			clubPlayerReport->setH2hPoints(0);
			clubPlayerReport->setMinutes(0);

			FifaClubsPlayerReport = BLAZE_NEW Blaze::GameReporting::FifaClubReportBase::FifaClubsPlayerReport;

			//As per the new TTY; To confirm whether is it at the right place?
			FifaClubsPlayerReport->setManOfTheMatch(0);
			FifaClubsPlayerReport->setCleanSheetsAny(0);
			FifaClubsPlayerReport->setCleanSheetsDef(0);
			FifaClubsPlayerReport->setCleanSheetsGoalKeeper(0);

			Blaze::GameReporting::Fifa::CommonPlayerReport& fifaCommonPlayerReport = FifaClubsPlayerReport->getCommonPlayerReport();

			fifaCommonPlayerReport.setAssists(rand() % 10);
			fifaCommonPlayerReport.setGoalsConceded(rand() % 10);
			fifaCommonPlayerReport.setHasCleanSheets(rand() % 10);
			//mHasCleanSheets need to be set to 1 as per the new TTY16
			fifaCommonPlayerReport.setCorners(rand() % 15);
			fifaCommonPlayerReport.setPassAttempts(rand() % 10);
			fifaCommonPlayerReport.setPassesMade(rand() % 12);
			fifaCommonPlayerReport.setRating((float)(rand() % 10));
			fifaCommonPlayerReport.setSaves(rand() % 12);
			fifaCommonPlayerReport.setShots(rand() % 12);
			fifaCommonPlayerReport.setTackleAttempts(rand() % 12);
			fifaCommonPlayerReport.setTacklesMade(rand() % 15);
			fifaCommonPlayerReport.setFouls(rand() % 10);
			fifaCommonPlayerReport.setGoals(minScore + (rand() % 15));
			fifaCommonPlayerReport.setInterceptions(rand() % 10);//As per the new TTY; new one
			//mHasMOTM = 1 (0x01) need to be set to 1 as per the new TTY16
			fifaCommonPlayerReport.setHasMOTM(rand() % 15);
			fifaCommonPlayerReport.setOffsides(rand() % 15);
			fifaCommonPlayerReport.setOwnGoals(rand() % 10);
			fifaCommonPlayerReport.setPkGoals(0);
			fifaCommonPlayerReport.setPossession(rand() % 15);//As per the new TTY it is 38,62...
			fifaCommonPlayerReport.setRedCard(rand() % 10);
			fifaCommonPlayerReport.setShotsOnGoal(rand() % 15);
			fifaCommonPlayerReport.setUnadjustedScore(0);//As per the new TTY; new one
			fifaCommonPlayerReport.setYellowCard(rand() % 10);

			//Changing the order as per new TTY;
			playerReport->setCustomDnf(rand() % 10);
			playerReport->setClientScore(rand() % 15);
			playerReport->setAccountCountry(rand() % 12);
			playerReport->setFinishReason(rand() % 10);
			playerReport->setGameResult(rand() % 10);
			//playerReport->setHome(true);
			if (getMyClubId() - mOpponentClubId > 0)
			{
				playerReport->setHome(false);
			}
			else
			{
				playerReport->setHome(true);
			}
			playerReport->setLosses(0);
			playerReport->setName(mPlayerData->getPersonaName().c_str());
			playerReport->setOpponentCount(0);
			playerReport->setExternalId(0);
			playerReport->setNucleusId(mPlayerData->getPlayerId());
			playerReport->setPersona(mPlayerData->getPersonaName().c_str());
			playerReport->setPointsAgainst(0);
			//playerReport->setUserResult(1);
			playerReport->setUserResult(13);//To confirm
			playerReport->setScore(count + (rand() % 10));
			//playerReport->setSponsoredEventAccountRegion(0);
			playerReport->setSkill(0);
			playerReport->setSkillPoints(0);
			//playerReport->setTeam(0);
			playerReport->setTeam(rand() % 100);//To confirm
			playerReport->setTies(0);
			playerReport->setWinnerByDnf(0);
			playerReport->setWins(0);
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
				PlayerPrivateIntAttribMap["GAMEENDREASON"] = (rand() % 10);
				PlayerPrivateIntAttribMap["GDESYNCEND"] = (rand() % 10);
				PlayerPrivateIntAttribMap["GDESYNCRSN"] = (rand() % 10);
				PlayerPrivateIntAttribMap["GENDPHASE"] = (rand() % 10);
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
				PlayerPrivateIntAttribMap["attribute0_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute1_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute2_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute3_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute4_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute5_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute6_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute7_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute8_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute9_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute10_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute11_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute12_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute13_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute14_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute15_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute16_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute17_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute18_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute19_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute20_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute21_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute22_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute23_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute24_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute25_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute26_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute27_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute28_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute29_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute30_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute31_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute32_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["attribute33_type"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["game_end_w_enough_player"] = (uint32_t)(playerId % 7);
				PlayerPrivateIntAttribMap["gid"] = getGameId();
				PlayerPrivateIntAttribMap["match_event_count"] = 273;
				PlayerPrivateIntAttribMap["matchevent_100"] = 4;
				PlayerPrivateIntAttribMap["matchevent_111"] = 79;
				PlayerPrivateIntAttribMap["matchevent_219"] = 6;
				PlayerPrivateIntAttribMap["matchevent_99"] = 2;
				PlayerPrivateIntAttribMap["player_attribute_count"] = 34;
				PlayerPrivateIntAttribMap["vpro_isvalid"] = 1;
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
				PlayerPrivateAttribMap["VProName"] = "H. Brown";
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

			}//if end

			clubPlayerReport->setCustomClubPlayerReport(*FifaClubsPlayerReport);
			playerReport->setCustomPlayerReport(*clubPlayerReport);

			OsdkPlayerReportsMap[playerId] = playerReport;
		}//for end

		clubReport = BLAZE_NEW Blaze::GameReporting::OSDKClubGameReportBase::OSDKClubReport;

		Blaze::GameReporting::OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& teamReportsMap = clubReport->getClubReports();
		teamReportsMap.reserve(2);
		for (int i = 1; i <= 2; i++)
		{
			Blaze::Clubs::ClubId clubid = 0;
			//if(i%2==0)
			if (i % 2 != 0)
			{
				//Own team
				clubid = getMyClubId();
			}
			else
			{
				//Joiners Team OR Oppoent Team
				clubid = mOpponentClubId;
			}
			Blaze::GameReporting::OSDKClubGameReportBase::OSDKClubClubReport* clubClubReport = teamReportsMap.allocate_element();
			if ((getMyClubId() - mOpponentClubId) > 0)
			{
				clubClubReport->setHome(false);

			}
			else
			{
				clubClubReport->setHome(true);
			}
			clubClubReport->setClubId(clubid);
			//clubClubReport->setTeam(clubid%100);
			clubClubReport->setTeam(rand() % 100);//To fix got bad TeamId Error from server side when club id is zero, need to check on load..


			//As per the new TTY; added OSDKClubClubReport set functions newly;
			clubClubReport->setChallengePoints(clubid % 2);
			clubClubReport->setClubRegion(clubid % 2);
			clubClubReport->setClubDisc(clubid % 2);
			clubClubReport->setH2hPoints(clubid % 2);
			clubClubReport->setLosses(clubid % 2);
			clubClubReport->setGameResult(clubid % 2);
			clubClubReport->setScore(clubid % 3);
			clubClubReport->setClubSeason(clubid % 2);
			clubClubReport->setSkill(clubid % 2);
			clubClubReport->setSkillPoints(clubid % 2);
			clubClubReport->setTournamentId(clubid % 2);
			clubClubReport->setTies(clubid % 2);
			clubClubReport->setWinnerByDnf(clubid % 2);
			clubClubReport->setWins(clubid % 2);

			fifaClubClubReport = BLAZE_NEW Blaze::GameReporting::FifaClubReportBase::FifaClubsClubReport;


			fifaClubClubReport->setCleanSheets(clubid % 2);
			fifaClubClubReport->setGoalsAgainst(clubid % 2);
			fifaClubClubReport->setGoals(minScore + (clubid % 5));
			fifaClubClubReport->setTeamrating(70);//As per the new TTY; added newly
			Blaze::GameReporting::Fifa::CommonPlayerReport& clubCommonPlayerReport = fifaClubClubReport->getCommonClubReport();

			clubCommonPlayerReport.setAssists(clubid % 2);
			clubCommonPlayerReport.setGoalsConceded(clubid % 2);
			clubCommonPlayerReport.setCorners(clubid % 5);
			clubCommonPlayerReport.setPassAttempts(clubid % 15);
			clubCommonPlayerReport.setPassesMade(clubid % 6);
			clubCommonPlayerReport.setRating((float)(clubid % 2));
			clubCommonPlayerReport.setSaves(clubid % 4);
			clubCommonPlayerReport.setShots(clubid % 6);
			clubCommonPlayerReport.setTackleAttempts(clubid % 6);
			clubCommonPlayerReport.setTacklesMade(clubid % 5);
			clubCommonPlayerReport.setFouls(clubid % 3);
			clubCommonPlayerReport.setGoals((minScore + (clubid % 5)));
			clubCommonPlayerReport.setInterceptions(clubid % 2);//As per the new TTY; Added newly
			clubCommonPlayerReport.setOffsides(clubid % 5);
			clubCommonPlayerReport.setOwnGoals(clubid % 2);
			clubCommonPlayerReport.setPkGoals(0);
			clubCommonPlayerReport.setPossession(clubid % 5);//in log it is 62
			clubCommonPlayerReport.setRedCard(clubid % 3);
			clubCommonPlayerReport.setShotsOnGoal(clubid % 5);
			clubCommonPlayerReport.setUnadjustedScore(clubid % 2);//As per the new TTY; Added newly                 
			clubCommonPlayerReport.setYellowCard(clubid % 3);

			//As per the new TTY; SeasonalPlayData added newly;
			Blaze::GameReporting::Fifa::SeasonalPlayData& clubSeasonalPlayData = fifaClubClubReport->getSeasonalPlayData();
			clubSeasonalPlayData.setCurrentDivision(clubid % 2);
			clubSeasonalPlayData.setCup_id(clubid % 2);
			clubSeasonalPlayData.setUpdateDivision(Blaze::GameReporting::Fifa::NO_UPDATE);
			clubSeasonalPlayData.setGameNumber(clubid % 2);
			clubSeasonalPlayData.setSeasonId(1 + clubid % 2);
			clubSeasonalPlayData.setWonCompetition(clubid % 2);
			clubSeasonalPlayData.setWonLeagueTitle(false);
			clubClubReport->setCustomClubClubReport(*fifaClubClubReport);
			teamReportsMap[clubid] = clubClubReport;
		}
		osdkReport->setTeamReports(*clubReport);
	}//if(osdkReport != NULL) end

	/*char8_t buf[404800];
	BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "SubmitGameReport RPC : \n" << request.print(buf, sizeof(buf)));*/

	error = mPlayerData->getComponentProxy()->mGameReportingProxy->submitGameReport(request);
	if (error == ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "Clubs::submitgamereport success.");
	}
	else
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "Clubs::submitgamereport failed. Error(" << ErrorHelp::getErrorName(error) << ")");
	}

	return error;
}

BlazeRpcError ClubsPlayerGameSession::getGameListSubscription()
{
	BlazeRpcError err = ERR_OK;

	Blaze::GameManager::GetGameListRequest request;
	Blaze::GameManager::GetGameListResponse response;
	Blaze::GameManager::MatchmakingCriteriaError error;

	if (!mPlayerData->isConnected())
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "[ClubsPlayerGameSession::getGameListSubscription::User Disconnected = " << mPlayerData->getPlayerId());
		return ERR_DISCONNECTED;
	}
	CommonGameRequestData& commonData = request.getCommonGameData();
	NetworkAddress& hostAddress = commonData.getPlayerNetworkAddress();
	//NetworkAddress *hostAddress = &request.getCommonGameData().getPlayerNetworkAddress();
	hostAddress.switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
	hostAddress.getIpPairAddress()->getExternalAddress().setIp(mPlayerData->getConnection()->getAddress().getIp());
	hostAddress.getIpPairAddress()->getExternalAddress().setPort(mPlayerData->getConnection()->getAddress().getPort());
	hostAddress.getIpPairAddress()->getExternalAddress().copyInto(hostAddress.getIpPairAddress()->getInternalAddress());

	commonData.setGameType(GAME_TYPE_GAMESESSION);
	commonData.setGameProtocolVersionString(getGameProtocolVersionString());


	request.setListCapacity(5);
	request.setListConfigName("default");
	request.setIgnoreGameEntryCriteria(false);

	Blaze::GameManager::MatchmakingCriteriaData& criteriaData = request.getListCriteria();
	criteriaData.getExpandedPingSiteRuleCriteria().setLatencyCalcMethod(INVALID_LATENCY_CALC_METHOD);		//	Added as per logs
	criteriaData.getExpandedPingSiteRuleCriteria().setPingSiteSelectionMethod(INVALID_PING_SITE_SELECTION_METHOD);
	criteriaData.getExpandedPingSiteRuleCriteria().setSessionMatchCalcMethod(INVALID_SESSION_CALC_METHOD);
	criteriaData.getExpandedPingSiteRuleCriteria().setRangeOffsetListName("OSDK_matchAny");

	criteriaData.getFreePlayerSlotsRuleCriteria().setMaxFreePlayerSlots(65535);
	criteriaData.getFreePlayerSlotsRuleCriteria().setMinFreePlayerSlots(0);

	GameManager::GameAttributeRuleCriteriaMap& gameRulePrefs = criteriaData.getGameAttributeRuleCriteriaMap();
	GameAttributeRuleCriteria* gameAttributeRuleCriteria = gameRulePrefs.allocate_element();

	gameAttributeRuleCriteria = gameRulePrefs.allocate_element();
	gameAttributeRuleCriteria->setMinFitThresholdName("OSDK_matchAny");
	gameAttributeRuleCriteria->getDesiredValues().push_back("16");
	gameRulePrefs.insert(eastl::make_pair("fifaClubLeague", gameAttributeRuleCriteria));

	gameAttributeRuleCriteria = gameRulePrefs.allocate_element();
	gameAttributeRuleCriteria->setMinFitThresholdName("OSDK_matchAny");
	gameAttributeRuleCriteria->getDesiredValues().push_back("0");
	gameRulePrefs.insert(eastl::make_pair("fifaCustomController", gameAttributeRuleCriteria));

	gameAttributeRuleCriteria = gameRulePrefs.allocate_element();
	gameAttributeRuleCriteria->setMinFitThresholdName("OSDK_matchAny");
	gameAttributeRuleCriteria->getDesiredValues().push_back("1");
	gameRulePrefs.insert(eastl::make_pair("fifaGameSpeed", gameAttributeRuleCriteria));

	gameAttributeRuleCriteria = gameRulePrefs.allocate_element();
	gameAttributeRuleCriteria->setMinFitThresholdName("OSDK_matchAny");
	gameAttributeRuleCriteria->getDesiredValues().push_back("1");
	gameRulePrefs.insert(eastl::make_pair("fifaGKControl", gameAttributeRuleCriteria));

	gameAttributeRuleCriteria = gameRulePrefs.allocate_element();
	gameAttributeRuleCriteria->setMinFitThresholdName("OSDK_matchAny");
	gameAttributeRuleCriteria->getDesiredValues().push_back("4");
	gameRulePrefs.insert(eastl::make_pair("fifaHalfLength", gameAttributeRuleCriteria));

	gameAttributeRuleCriteria = gameRulePrefs.allocate_element();
	gameAttributeRuleCriteria->setMinFitThresholdName("OSDK_matchExact");
	gameAttributeRuleCriteria->getDesiredValues().push_back("3996824");
	gameRulePrefs.insert(eastl::make_pair("fifaMatchupHash", gameAttributeRuleCriteria));

	gameAttributeRuleCriteria = gameRulePrefs.allocate_element();
	gameAttributeRuleCriteria->setMinFitThresholdName("OSDK_matchAny");
	gameAttributeRuleCriteria->getDesiredValues().push_back("0");
	gameRulePrefs.insert(eastl::make_pair("fifaPPT", gameAttributeRuleCriteria));

	gameAttributeRuleCriteria = gameRulePrefs.allocate_element();
	gameAttributeRuleCriteria->setMinFitThresholdName("OSDK_matchAny");
	gameAttributeRuleCriteria->getDesiredValues().push_back("0");
	gameRulePrefs.insert(eastl::make_pair("FUTFriendlyType", gameAttributeRuleCriteria));

	/*
	gameAttributeRuleCriteria = gameRulePrefs.allocate_element();
	gameAttributeRuleCriteria->setMinFitThresholdName("OSDK_matchAny");
	gameAttributeRuleCriteria->getDesiredValues().push_back("0");
	gameRulePrefs.insert(eastl::make_pair("OSDK_arenaChallengeId", gameAttributeRuleCriteria));
	*/

	gameAttributeRuleCriteria = gameRulePrefs.allocate_element();
	gameAttributeRuleCriteria->setMinFitThresholdName("OSDK_matchExact");
	gameAttributeRuleCriteria->getDesiredValues().push_back("335");
	gameRulePrefs.insert(eastl::make_pair("OSDK_clubId", gameAttributeRuleCriteria));

	gameAttributeRuleCriteria = gameRulePrefs.allocate_element();
	gameAttributeRuleCriteria->setMinFitThresholdName("OSDK_matchBrowse");
	gameAttributeRuleCriteria->getDesiredValues().push_back("1");
	gameRulePrefs.insert(eastl::make_pair("OSDK_clubLeagueId", gameAttributeRuleCriteria));

	gameAttributeRuleCriteria = gameRulePrefs.allocate_element();
	gameAttributeRuleCriteria->setMinFitThresholdName("OSDK_matchExact");
	gameAttributeRuleCriteria->getDesiredValues().push_back("9");
	gameRulePrefs.insert(eastl::make_pair("OSDK_gameMode", gameAttributeRuleCriteria));

	criteriaData.getGeoLocationRuleCriteria().setMinFitThresholdName("");

	criteriaData.getGameNameRuleCriteria().setSearchString("");

	criteriaData.getModRuleCriteria().setIsEnabled(false);
	criteriaData.getModRuleCriteria().setDesiredModRegister(0);

	criteriaData.getHostBalancingRulePrefs().setMinFitThresholdName("");

	criteriaData.getPlayerCountRuleCriteria().setIsSingleGroupMatch(0);
	criteriaData.getPlayerCountRuleCriteria().setMaxPlayerCount(21);
	criteriaData.getPlayerCountRuleCriteria().setDesiredPlayerCount(1);
	criteriaData.getPlayerCountRuleCriteria().setMinPlayerCount(1);
	criteriaData.getPlayerCountRuleCriteria().setRangeOffsetListName("OSDK_matchAny");

	criteriaData.getPlayerSlotUtilizationRuleCriteria().setDesiredPercentFull(100);
	criteriaData.getPlayerSlotUtilizationRuleCriteria().setMaxPercentFull(100);
	criteriaData.getPlayerSlotUtilizationRuleCriteria().setMinPercentFull(0);
	criteriaData.getPlayerSlotUtilizationRuleCriteria().setRangeOffsetListName("");

	criteriaData.getPreferredGamesRuleCriteria().setRequirePreferredGame(false);
	criteriaData.getPreferredPlayersRuleCriteria().setRequirePreferredPlayer(false);

	//criteriaData.getPingSiteRulePrefs().setMinFitThresholdName("OSDK_matchAny");

	criteriaData.getRankedGameRulePrefs().setMinFitThresholdName("OSDK_matchExact");
	criteriaData.getRankedGameRulePrefs().setDesiredRankedGameValue(RankedGameRulePrefs::UNRANKED);

	criteriaData.getReputationRulePrefs().setReputationRequirement(REJECT_POOR_REPUTATION);

	criteriaData.getRosterSizeRulePrefs().setMaxPlayerCount(65535);
	criteriaData.getRosterSizeRulePrefs().setMinPlayerCount(0);

	criteriaData.getTeamBalanceRulePrefs().setMaxTeamSizeDifferenceAllowed(0);
	criteriaData.getTeamBalanceRulePrefs().setRangeOffsetListName("");

	criteriaData.getTeamCountRulePrefs().setTeamCount(0);

	criteriaData.getTeamCompositionRulePrefs().setRuleName("");
	criteriaData.getTeamCompositionRulePrefs().setMinFitThresholdName("");

	criteriaData.getTeamMinSizeRulePrefs().setTeamMinSize(0);
	criteriaData.getTeamMinSizeRulePrefs().setRangeOffsetListName("");

	criteriaData.getTotalPlayerSlotsRuleCriteria().setDesiredTotalPlayerSlots(2);
	criteriaData.getTotalPlayerSlotsRuleCriteria().setMaxTotalPlayerSlots(22);
	criteriaData.getTotalPlayerSlotsRuleCriteria().setMinTotalPlayerSlots(2);
	criteriaData.getTotalPlayerSlotsRuleCriteria().setRangeOffsetListName("OSDK_matchAny");

	criteriaData.getTeamUEDPositionParityRulePrefs().setRuleName("");
	criteriaData.getTeamUEDPositionParityRulePrefs().setRangeOffsetListName("");
	criteriaData.getTeamUEDBalanceRulePrefs().setRuleName("");
	criteriaData.getTeamUEDBalanceRulePrefs().setRangeOffsetListName("");
	criteriaData.getHostViabilityRulePrefs().setMinFitThresholdName("");

	criteriaData.getVirtualGameRulePrefs().setMinFitThresholdName("");
	criteriaData.getVirtualGameRulePrefs().setDesiredVirtualGameValue(VirtualGameRulePrefs::STANDARD);

	//playerJoinData
	PlayerJoinData& playerJoinData = request.getPlayerJoinData();
	playerJoinData.setDefaultRole("");
	playerJoinData.setGameEntryType(GAME_ENTRY_TYPE_DIRECT);
	/*    playerJoinData.setSlotType(SLOT_PUBLIC_PARTICIPANT);
		playerJoinData.setTeamId(ANY_TEAM_ID);
		playerJoinData.setTeamIndex(ANY_TEAM_ID + 1);   */

	err = mPlayerData->getComponentProxy()->mGameManagerProxy->getGameListSubscription(request, response, error);
	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "ClubsPlayerGameSession:getGameListSubscription failed. Error(" << ErrorHelp::getErrorName(err) << ") " << mPlayerData->getPlayerId());
	}
	else
	{
		mGameListId = response.getListId();
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "ClubsPlayerGameSession:getGameListSubscription successful. GameListId= " << mGameListId << " playerid " << mPlayerData->getPlayerId());
	}
	return err;
}


bool ClubsPlayerGameSession::isClubGameInProgress()
{
	ClubsGameStatusMap& clubStatus = ((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->getClubsStatusMap();
	ClubsGameStatusMap::iterator clubData = clubStatus.find(getMyClubId());
	//clubid not present in the map, add it
	if (clubData == clubStatus.end())
	{
		clubStatus[getMyClubId()] = false;
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::isClubGameInProgress][PlayerId =" << mPlayerData->getPlayerId() << "] adding clubID to ClubsGameStatusMap " << getMyClubId() << "  status " << clubStatus[getMyClubId()]);
		return false;
	}
	else
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::isClubGameInProgress][PlayerId =" << mPlayerData->getPlayerId() << "] ClubsGameStatusMap " << clubData->first << "  status " << clubData->second);
		return clubData->second;
	}
}

GameId ClubsPlayerGameSession::addClubFriendliesCreatedGame()
{
	CreatedGamesList* gameListReference = ((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->getClubsFriendliesGameList();
	if (gameListReference->size() == 0)
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::addClubFriendliesCreatedGame] The gameId map is empty " << mPlayerData->getPlayerId());
		return 0;
	}
	CreatedGamesList::iterator it = gameListReference->begin();
	GameId randomGameId = 0; int32_t num = 0;

	mMutex.Lock();
	num = Random::getRandomNumber(gameListReference->size());
	eastl::advance(it, num);
	randomGameId = *it;
	gameListReference->erase(it);
	mMutex.Unlock();
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::addClubFriendliesCreatedGame]  Randomly fetched GameId : " << randomGameId);
	return randomGameId;
}
BlazeRpcError ClubsPlayerGameSession::resetDedicatedServer()
{
	BlazeRpcError err = ERR_OK;
	Blaze::GameManager::CreateGameRequest request;
	Blaze::GameManager::JoinGameResponse response;
	GameCreationData& gameCreationData = request.getGameCreationData();

	char8_t clubIdString[MAX_GAMENAME_LEN] = { '\0' };
	char8_t opponentclubIdString[MAX_GAMENAME_LEN] = { '\0' };
	blaze_snzprintf(clubIdString, sizeof(clubIdString), "%" PRId64, getMyClubId());
	blaze_snzprintf(opponentclubIdString, sizeof(opponentclubIdString), "%" PRId64, mOpponentClubId);
	if (!mPlayerData->isConnected())
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "[ClubsPlayerGameSession::User Disconnected = " << mPlayerData->getPlayerId());
		return ERR_DISCONNECTED;
	}

	NetworkAddress *hostAddress = &request.getCommonGameData().getPlayerNetworkAddress();
	hostAddress->switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
	hostAddress->getIpPairAddress()->getExternalAddress().setIp(mPlayerData->getConnection()->getAddress().getIp());
	hostAddress->getIpPairAddress()->getExternalAddress().setPort(mPlayerData->getConnection()->getAddress().getPort());
	hostAddress->getIpPairAddress()->getExternalAddress().copyInto(hostAddress->getIpPairAddress()->getInternalAddress());
	Collections::AttributeMap& gameAttribs = gameCreationData.getGameAttribs();

	request.getTournamentSessionData().setArbitrationTimeout(31536000000000);
	request.getTournamentSessionData().setForfeitTimeout(31449600000000);
	request.getTournamentSessionData().setTournamentDefinition("");
	request.getTournamentSessionData().setScheduledStartTime("1970-01-01T00:00:00Z"); // Need to use Time library
	gameAttribs["fifaClubLeague"] = "16";
	gameAttribs["fifaCustomController"] = "0";
	gameAttribs["fifaGameSpeed"] = "1";
	gameAttribs["fifaGKControl"] = "1";
	gameAttribs["fifaHalfLength"] = "4";
	gameAttribs["fifaMatchupHash"] = "4171130";  // 
	gameAttribs["fifaPPT"] = "0";
	gameAttribs["FUTFriendlyType"] = "0";
	gameAttribs["Gamemode_Name"] = "club_season_match";
	gameAttribs["OSDK_categoryId"] = "gameType9";
	gameAttribs["OSDK_ChlngrClubId"] = clubIdString;
	if(mScenarioType!=CLUBSPRACTICE)
	{
		gameAttribs["OSDK_clubId"] = opponentclubIdString;
		gameAttribs["OSDK_clubLeagueId"] = "1";
	}
	gameAttribs["OSDK_gameMode"] = "9";
	gameAttribs["OSDK_rosterURL"] = "";
	gameAttribs["OSDK_rosterVersion"] = "1";
	gameAttribs["OSDK_sponsoredEventId"] = "0";
	gameAttribs["ssfMatchType"] = "-1";
	gameAttribs["ssfStadium"] = "0";
	gameAttribs["ssfWalls"] = "-1";

	Blaze::EntryCriteriaMap& map = gameCreationData.getEntryCriteriaMap();
	map["OSDK_maxDNF"] = "stats_dnf <= 100";
	request.setGamePingSiteAlias("");  // As per the new TTY; need to confirm?
	request.setGameEventAddress("");


	gameCreationData.setDisconnectReservationTimeout(0);
	//gameCreationData.getExternalDataSourceApiNameList().clear();
	gameCreationData.setGameModRegister(0);
	gameCreationData.setGameName("");
	gameCreationData.setPermissionSystemName("");
	gameCreationData.getGameSettings().setBits(0x0004048F);
	gameCreationData.setGameReportName("");
	gameCreationData.setSkipInitializing(false);
	gameCreationData.setNetworkTopology(Blaze::CLIENT_SERVER_DEDICATED);
	gameCreationData.setMaxPlayerCapacity(0);
	gameCreationData.setMinPlayerCapacity(1);
	gameCreationData.setPresenceMode(Blaze::PRESENCE_MODE_PRIVATE);
	gameCreationData.setPlayerReservationTimeout(0);
	gameCreationData.setQueueCapacity(0);
	gameCreationData.getRoleInformation().getRoleCriteriaMap().clear();
	gameCreationData.getRoleInformation().getMultiRoleCriteria().clear();
	gameCreationData.getExternalSessionStatus().getLangMap().clear();
	gameCreationData.getExternalSessionStatus().setUnlocalizedValue("");
	gameCreationData.setExternalSessionTemplateName("");
	gameCreationData.getTeamIds().push_back(65534);
	gameCreationData.setVoipNetwork(Blaze::VOIP_DISABLED);

	request.setGameEventContentType("application/json");
	request.setGameEndEventUri("");
	request.setGameStartEventUri("");
	request.setGameReportName("");
	request.setGameStatusUrl("");
	request.getMeshAttribs().clear();
	request.setServerNotResetable(false);
	request.getSlotCapacities().at(SLOT_PUBLIC_PARTICIPANT) = 22;
	request.getSlotCapacities().at(SLOT_PRIVATE_PARTICIPANT) = 0;
	request.getSlotCapacities().at(SLOT_PUBLIC_SPECTATOR) = 0;
	request.getSlotCapacities().at(SLOT_PRIVATE_SPECTATOR) = 0;
	request.getSlotCapacitiesMap().clear();
	request.setPersistedGameId("");

	PlayerJoinData &playerJoinData = request.getPlayerJoinData();
	playerJoinData.setGameEntryType(GAME_ENTRY_TYPE_DIRECT);  // As per the new TTY;
	playerJoinData.setGroupId(mPlayerData->getConnGroupId());

	playerJoinData.setDefaultSlotType(Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT);
	playerJoinData.setDefaultTeamIndex(0);  // As per the new TTY;
	playerJoinData.setDefaultTeamId(65534);

	request.getTournamentIdentification().setTournamentId("");
	request.getTournamentIdentification().setTournamentOrganizer("");


	request.getCommonGameData().setGameProtocolVersionString(StressConfigInst.getGameProtocolVersionString());
	// Set random ping sites from the list of allowed ones
	request.setGamePingSiteAlias(mPlayerData->getPlayerPingSite());
	//if (BLAZE_IS_TRACE_RPC_ENABLED(BlazeRpcLog::gamemanager)) {
		char8_t buf[10240];
		BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "ResetDedicatedServer Request: \n" << request.print(buf, sizeof(buf)));
	//}
	err = mPlayerData->getComponentProxy()->mGameManagerProxy->resetDedicatedServer(request, response);

	if (err == ERR_OK)
	{
		if (response.getGameId() != 0)
		{
			CreatedGamesList* gameListReference = ((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->getClubsFriendliesGameList();
			gameListReference->push_back(response.getGameId());
		}
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "ResetDedicatedServer the game id is: " << response.getGameId() << "  The Club Id is: \n" << getMyClubId());
	}
	else
	{
		//char8_t buf[10240];
		//BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "ResetDedicatedServer request buffer : "<<request.print(buf, sizeof(buf)));
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "ClubsPlayerGameSession:ResetDedicatedServer rpc failed. Error(" << ErrorHelp::getErrorName(err) << ") " << mPlayerData->getPlayerId());

	}

	return err;
}

BlazeRpcError ClubsPlayerGameSession::joinGame(GameId gameId)
{
	BlazeRpcError err = ERR_OK;
	Blaze::GameManager::JoinGameRequest request;
	Blaze::GameManager::JoinGameResponse response;
	Blaze::EntryCriteriaError error;

	if (!mPlayerData->isConnected())
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "[ClubsPlayerGameSession::User Disconnected = " << mPlayerData->getPlayerId());
		return ERR_DISCONNECTED;
	}

	NetworkAddress *hostAddress = &request.getCommonGameData().getPlayerNetworkAddress();
	hostAddress->switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
	hostAddress->getIpPairAddress()->getExternalAddress().setIp(mPlayerData->getConnection()->getAddress().getIp());
	hostAddress->getIpPairAddress()->getExternalAddress().setPort(mPlayerData->getConnection()->getAddress().getPort());
	hostAddress->getIpPairAddress()->getExternalAddress().copyInto(hostAddress->getIpPairAddress()->getInternalAddress());
	request.getCommonGameData().setGameProtocolVersionString(StressConfigInst.getGameProtocolVersionString());
	request.setGameId(gameId);
	request.setJoinMethod(Blaze::GameManager::JOIN_BY_BROWSING);
	request.setRequestedSlotId(255);

	request.getPlayerJoinData().setDefaultSlotType(Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT);
	request.getPlayerJoinData().setGroupId(mPlayerData->getConnGroupId());
	Blaze::GameManager::PerPlayerJoinData * joinerData = request.getPlayerJoinData().getPlayerDataList().pull_back();
	joinerData->setEncryptedBlazeId("");
	joinerData->setIsOptionalPlayer(false);
	joinerData->setSlotType(INVALID_SLOT_TYPE);
	joinerData->setTeamId(65534);
	joinerData->setTeamIndex(65535);
	joinerData->getPlayerAttributes().clear();
	joinerData->getUser().setBlazeId(mPlayerData->getPlayerId());
	if (StressConfigInst.getPlatform() == Platform::PLATFORM_XONE)
	{
		joinerData->getUser().setPersonaNamespace("xone");
	}
	else
	{
		joinerData->getUser().setPersonaNamespace("ps3");
	}
	joinerData->getUser().setExternalId(mPlayerData->getExternalId());
	joinerData->getUser().setName(mPlayerData->getLogin()->getUserLoginInfo()->getPersonaDetails().getDisplayName());
	joinerData->getUser().setAccountId(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());

	request.getPlayerJoinData().setDefaultTeamId(65534);
	request.getPlayerJoinData().setDefaultTeamIndex(65535);
	//if (BLAZE_IS_TRACE_RPC_ENABLED(BlazeRpcLog::gamemanager)) {
	//	char8_t buf[10240];
	//	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "JoinGame RPC : \n" << "\n" << request.print(buf, sizeof(buf)));
	//}
	err = mPlayerData->getComponentProxy()->mGameManagerProxy->joinGame(request, response, error);
	if (err!=ERR_OK) {
		//char8_t buf[10240];
		//BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "JoinGame error : \n" << "\n" << error.print(buf, sizeof(buf)));
	}
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::clubs, "ClubsPlayer::JoinGame failed for Player ID = " << mPlayerData->getPlayerId() << " error = " << ErrorHelp::getErrorName(err));
	}
	else
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "Join game success for player : " << mPlayerData->getPlayerId() << " with gameId : " << gameId);
	}

	return err;
}


void ClubsPlayerGameSession::setClubGameStatus(bool gameProgress)
{
	ClubsGameStatusMap& clubStatus = ((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->getClubsStatusMap();
	ClubsGameStatusMap::iterator clubData = clubStatus.find(getMyClubId());

	//if gameProgress == true set a timer - TODO

	//clubid not present in the map, add it
	if(clubData == clubStatus.end())
	{
		clubStatus[getMyClubId()] = gameProgress;
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::setClubGameStatus][PlayerId =" << mPlayerData->getPlayerId() << "] adding clubID to ClubsGameStatusMap " << getMyClubId() << "  status " << clubStatus[getMyClubId()]);
	}
	else
	{
		clubData->second = gameProgress;
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayerGameSession::setClubGameStatus][PlayerId =" << mPlayerData->getPlayerId() << "] ClubsGameStatusMap " << clubData->first << "  status " << clubData->second);
	}
}

}  // namespace Stress
}  // namespace Blaze
