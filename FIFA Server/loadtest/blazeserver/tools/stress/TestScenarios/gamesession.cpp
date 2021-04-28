//  *************************************************************************************************
//
//   File:    gamesession.cpp
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#include "./gamesession.h"

#include "framework/protocol/shared/heat2decoder.h"
#include "gamemanager/tdf/gamebrowser.h"
#include "gamemanager/tdf/legacyrules.h"
#include "gamemanager/tdf/rules.h"

using namespace Blaze::GameManager;
using namespace Blaze::GameReporting;  //  Temp Fix

namespace Blaze {
namespace Stress {

GameSession::GameSession(StressPlayerInfo* PlayerData, uint32_t GameSize, GameNetworkTopology Topology, GameProtocolVersionString GameProtocolVersionString)
    : isTopologyHost(false),
      isPlatformHost(false),
	  isPlatformGameGroupHost(false),
      mGameNpSessionId(""),
      mGameSize(GameSize),
      mTopology(Topology),
      mGameProtocolVersionString(GameProtocolVersionString),
      mState(NEW_STATE),
      myGameId(0),
      mMatchmakingSessionId(0),
      doPing(true),
      mPlayerData(PlayerData),
      mInGameStartTime(0),
	  mCupId(0),
	  mClubID(0),
	  mTournamentId(0)
{
    if (mPlayerData->isConnected())
    {
        //   Set up notification CBs
        mPlayerData->getConnection()->addAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMEREMOVED, mPlayerData->getMyPlayerInstance());
        mPlayerData->getConnection()->addAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLAYERREMOVED, mPlayerData->getMyPlayerInstance());
        mPlayerData->getConnection()->addAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINCOMPLETED, mPlayerData->getMyPlayerInstance());
        mPlayerData->getConnection()->addAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMESTATECHANGE, mPlayerData->getMyPlayerInstance());
        mPlayerData->getConnection()->addAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMESETUP, mPlayerData->getMyPlayerInstance());
        mPlayerData->getConnection()->addAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYMATCHMAKINGFAILED, mPlayerData->getMyPlayerInstance());
		mPlayerData->getConnection()->addAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINING, mPlayerData->getMyPlayerInstance());
        mPlayerData->getConnection()->addAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS, mPlayerData->getMyPlayerInstance());
		mPlayerData->getConnection()->addAsyncHandler(GameReporting::GameReportingSlave::COMPONENT_ID, GameReportingSlave::NOTIFY_RESULTNOTIFICATION,mPlayerData->getMyPlayerInstance());
		mPlayerData->getConnection()->addAsyncHandler(Blaze::Perf::PerfSlave::COMPONENT_ID, Blaze::Perf::PerfSlave::NOTIFY_NOTIFYPINGRESPONSE, mPlayerData->getMyPlayerInstance());
		mPlayerData->getConnection()->addAsyncHandler(Blaze::Perf::PerfSlave::COMPONENT_ID, Blaze::Perf::PerfSlave::NOTIFY_NOTIFYPERFTUNE, mPlayerData->getMyPlayerInstance());
		//mPlayerData->getConnection()->addAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYREMOTEJOINFAILED, mPlayerData->getMyPlayerInstance());
        //mPlayerData->getConnection()->addAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMELISTUPDATE, mPlayerData->getMyPlayerInstance());
        //mPlayerData->getConnection()->addAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMERESET, mPlayerData->getMyPlayerInstance());      
        //mPlayerData->getConnection()->addAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLATFORMHOSTINITIALIZED, mPlayerData->getMyPlayerInstance());
        //mPlayerData->getConnection()->addAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMEREPORTINGIDCHANGE, mPlayerData->getMyPlayerInstance());
        //mPlayerData->getConnection()->addAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYHOSTMIGRATIONFINISHED, mPlayerData->getMyPlayerInstance());
        //mPlayerData->getConnection()->addAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYHOSTMIGRATIONSTART, mPlayerData->getMyPlayerInstance());

        //mPlayerData->getConnection()->addAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLAYERCLAIMINGRESERVATION, mPlayerData->getMyPlayerInstance());
        //mPlayerData->getConnection()->addAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMEATTRIBCHANGE, mPlayerData->getMyPlayerInstance());
        //mPlayerData->getConnection()->addAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLAYERJOININGQUEUE, mPlayerData->getMyPlayerInstance());
        //mPlayerData->getConnection()->addAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLAYERPROMOTEDFROMQUEUE, mPlayerData->getMyPlayerInstance());
        //mPlayerData->getConnection()->addAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMESESSIONUPDATED, mPlayerData->getMyPlayerInstance());
        //mPlayerData->getConnection()->addAsyncHandler(Messaging::MessagingSlave::COMPONENT_ID, Messaging::MessagingSlave::NOTIFY_NOTIFYMESSAGE, mPlayerData->getMyPlayerInstance());
        mPlayerData->setPlayerActiveState(true);
    }
    else
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "notification handlers are not registered as there is no connection to server for player: " << mPlayerData->getPlayerId());
    }
    mGameActivity.setGameId(0);
}

GameSession::~GameSession()
{
    if (mPlayerData->isConnected())
    {
        deregisterHandlers();
    }
    else
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "notification handlers are not de-registered as there is no connection to server for player: " << mPlayerData->getPlayerId());
    }
    //  mPlayerData = NULL;
}

void GameSession::asyncHandlerFunction(uint16_t type, EA::TDF::Tdf *notification)
{
    //  Handle the async handlers in the derived class and shouldn't come here
    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameSession::asyncHandlerFunction][" << mPlayerData->getPlayerId() << " ]:Message type-" << type);
}

void GameSession::deregisterHandlers()
{
    if (mPlayerData != NULL && mPlayerData->getConnection() != NULL && mPlayerData->isPlayerActive())
    {
        BLAZE_INFO(BlazeRpcLog::gamemanager, "[GameSession::deregisterHandlers][%" PRIu64 "]:De registering handlers", mPlayerData->getPlayerId());
        mPlayerData->setPlayerActiveState(false);

		mPlayerData->getConnection()->removeAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMEREMOVED, mPlayerData->getMyPlayerInstance());
        mPlayerData->getConnection()->removeAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLAYERREMOVED, mPlayerData->getMyPlayerInstance());
        mPlayerData->getConnection()->removeAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINCOMPLETED, mPlayerData->getMyPlayerInstance());
        mPlayerData->getConnection()->removeAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMESTATECHANGE, mPlayerData->getMyPlayerInstance());
        mPlayerData->getConnection()->removeAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMESETUP, mPlayerData->getMyPlayerInstance());
        mPlayerData->getConnection()->removeAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYMATCHMAKINGFAILED, mPlayerData->getMyPlayerInstance());
		mPlayerData->getConnection()->removeAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINING, mPlayerData->getMyPlayerInstance());
		mPlayerData->getConnection()->removeAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS, mPlayerData->getMyPlayerInstance());
		mPlayerData->getConnection()->removeAsyncHandler(GameReporting::GameReportingSlave::COMPONENT_ID, GameReportingSlave::NOTIFY_RESULTNOTIFICATION,mPlayerData->getMyPlayerInstance());
		
        //mPlayerData->getConnection()->removeAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYREMOTEJOINFAILED, mPlayerData->getMyPlayerInstance());
        //mPlayerData->getConnection()->removeAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMELISTUPDATE, mPlayerData->getMyPlayerInstance());
        //mPlayerData->getConnection()->removeAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMERESET, mPlayerData->getMyPlayerInstance());        
        //mPlayerData->getConnection()->removeAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLATFORMHOSTINITIALIZED, mPlayerData->getMyPlayerInstance());
        //mPlayerData->getConnection()->removeAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMEREPORTINGIDCHANGE, mPlayerData->getMyPlayerInstance());
        //mPlayerData->getConnection()->removeAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYHOSTMIGRATIONFINISHED, mPlayerData->getMyPlayerInstance());
        //mPlayerData->getConnection()->removeAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYHOSTMIGRATIONSTART, mPlayerData->getMyPlayerInstance());        
        //mPlayerData->getConnection()->removeAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLAYERCLAIMINGRESERVATION, mPlayerData->getMyPlayerInstance());
        //mPlayerData->getConnection()->removeAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMEATTRIBCHANGE, mPlayerData->getMyPlayerInstance());
        //mPlayerData->getConnection()->removeAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLAYERJOININGQUEUE, mPlayerData->getMyPlayerInstance());
        //mPlayerData->getConnection()->removeAsyncHandler(GameManager::GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLAYERPROMOTEDFROMQUEUE, mPlayerData->getMyPlayerInstance());
        //mPlayerData->getConnection()->removeAsyncHandler(Messaging::MessagingSlave::COMPONENT_ID, Messaging::MessagingSlave::NOTIFY_NOTIFYMESSAGE, mPlayerData->getMyPlayerInstance());
    }
}

void GameSession::setGameName(eastl::string name)
{
    blaze_snzprintf(mGameName, sizeof(mGameName), "stress_game_%s", name.c_str());
	BLAZE_TRACE_LOG(BlazeRpcLog::gamereporting, "[GameSession::setGameName]: GameName = " << mGameName);
}


void GameSession::onAsync(uint16_t component, uint16_t type, RawBuffer* payload)
{
    Heat2Decoder decoder;
    if (mPlayerData == NULL || !mPlayerData->isPlayerActive())
    {
        BLAZE_ERR(BlazeRpcLog::gamemanager, "[GameSession::onAsync]: playerData is null");
        return;
    }
    if ( (GameManagerSlave::COMPONENT_ID != component) && (GameReporting::GameReportingSlave::COMPONENT_ID != component) && (UserSessionsSlave::COMPONENT_ID != component) &&
        (Blaze::Messaging::MessagingSlave::COMPONENT_ID != component) && (Blaze::Perf::PerfSlave::COMPONENT_ID != component) )
    {
        //   payload not handled by recipient.
        BLAZE_INFO(BlazeRpcLog::gamereporting, "[GameSession::onAsync][%" PRIu64 "]: Payload not handled by us.", mPlayerData->getPlayerId());
        //  Don't delete payload pointer here.
        //  delete payload;
        return;
    }
    EA::TDF::Tdf* notification = NULL;
    switch (type)
    {
        case UserSessionsSlave::NOTIFY_USERAUTHENTICATED:
            {
                BLAZE_TRACE_LOG(BlazeRpcLog::gamereporting, "[Player::onAsync]: received NOTIFY_USERAUTHENTICATED.");
                Blaze::UserSessionLoginInfo nnotification;
                if (decoder.decode(*payload, nnotification) == ERR_OK)
                {
                    //  Read player data
                    mPlayerData->setPlayerId(nnotification.getBlazeId());
                    mPlayerData->setConnGroupId(nnotification.getConnectionGroupObjectId());
                    mPlayerData->setPersonaName(nnotification.getDisplayName());
                    mPlayerData->setExternalId(nnotification.getExtId());
					//setGameName(nnotification.getBlazeUserId());
					setGameName(nnotification.getDisplayName());
					mPlayerData->setAccountId(nnotification.getAccountId());
                }
                else
                {
                    BLAZE_ERR_LOG(Log::SYSTEM, "Heat2Decoder failed to decode notification type UserAuthenticated in component UserSessions");
                }
                break;
            }
        case Blaze::Messaging::MessagingSlave::NOTIFY_NOTIFYMESSAGE:
            {
                Blaze::Messaging::ServerMessage* notify = BLAZE_NEW Blaze::Messaging::ServerMessage;
                if (decoder.decode(*payload, *notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::messaging, "NOTIFYMESSAGE[" << mPlayerData->getPlayerId() << "]: Failed to decode notification(" << type << ")");
                    delete notify;
                }
                else
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::messaging, "[NOTIFYMESSAGE::onAsync][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYMESSAGE ");
                    notification = notify;
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMESETUP:
            {
                NotifyGameSetup* notify = BLAZE_NEW NotifyGameSetup;
                if (decoder.decode(*payload, *notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession[" << mPlayerData->getPlayerId() << "]: Failed to decode notification(" << type << ")");
                    delete notify;
                }
                else
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::onAsync]: Received  NOTIFY_NOTIFYGAMESETUP." << mPlayerData->getPlayerId() << ", gameId " << notify->getGameData().getGameId());
                    notification = notify;
                }
            }
            break;
		case GameReportingSlave::NOTIFY_RESULTNOTIFICATION:
			{
				ResultNotification* notify = BLAZE_NEW ResultNotification;
				if (decoder.decode(*payload, *notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamereporting, "GameSession[" << mPlayerData->getPlayerId() << "]: Failed to decode notification(" << type << ")");
                    delete notify;
                }
                else
                {
					BLAZE_TRACE_LOG(BlazeRpcLog::gamereporting, "[GameSession::onAsync]: Received  NOTIFY_RESULTNOTIFICATION." << mPlayerData->getPlayerId() );
                    notification = notify;
                }
			}
			break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMESTATECHANGE:
            {
                NotifyGameStateChange* notify = BLAZE_NEW NotifyGameStateChange;
                if (decoder.decode(*payload, *notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession[" << mPlayerData->getPlayerId() << "]: Failed to decode notification(" << type << ")");
                    delete notify;
                }
                else
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::onAsync][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMESTATECHANGE, state=" << notify->getNewGameState());
                    notification = notify;                   
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMERESET:
            {
                NotifyGameReset* notify = BLAZE_NEW NotifyGameReset;
                if (decoder.decode(*payload, *notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession[" << mPlayerData->getPlayerId() << "]: Failed to decode notification(" << type << ")");
                    delete notify;
                }
                else
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::onAsync][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMERESET.gameId=" << notify->getGameData().getGameId());                    
                    notification = notify;                    
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINING:
            {
                NotifyPlayerJoining* notify = BLAZE_NEW NotifyPlayerJoining;
                if (decoder.decode(*payload, *notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession[" << mPlayerData->getPlayerId() << "]: Failed to decode notification(" << type << ")");
                    delete notify;
                }
                else
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::onAsync][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERJOINING.gameId=" << notify->getGameId());                    
                    notification = notify;                 
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLATFORMHOSTINITIALIZED:
            {
                NotifyPlatformHostInitialized* notify = BLAZE_NEW NotifyPlatformHostInitialized;
                if (decoder.decode(*payload, *notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession[" << mPlayerData->getPlayerId() << "]: Failed to decode notification(" << type << ")");
                    delete notify;
                }
                else
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::onAsync][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLATFORMHOSTINITIALIZED.gameId=" << notify->getGameId());      
                    notification = notify;
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMELISTUPDATE:
            {
                GameManager::NotifyGameListUpdate* notify = BLAZE_NEW GameManager::NotifyGameListUpdate;
                if (decoder.decode(*payload, *notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession[" << mPlayerData->getPlayerId() << "]: Failed to decode notification(" << type << ")");
                    delete notify;
                }
                else
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::onAsync][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMELISTUPDATE.");
                    notification = notify;
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINCOMPLETED:
            {
                NotifyPlayerJoinCompleted* notify = BLAZE_NEW NotifyPlayerJoinCompleted;
                if (decoder.decode(*payload, *notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession[" << mPlayerData->getPlayerId() << "]: Failed to decode notification(" << type << ")");
                    delete notify;
                }
                else
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::onAsync][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERJOINCOMPLETED.");                    
                    notification = notify;                    
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLAYERREMOVED:
            {
                NotifyPlayerRemoved* notify = BLAZE_NEW NotifyPlayerRemoved;
                if (decoder.decode(*payload, *notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession[" << mPlayerData->getPlayerId() << "]: Failed to decode notification(" << type << ")");
                    delete notify;
                }
                else
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::onAsync][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERREMOVED.");
                    notification = notify;                    
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMEREMOVED:
            {
                NotifyGameRemoved* notify = BLAZE_NEW NotifyGameRemoved;
                if (decoder.decode(*payload, *notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession[" << mPlayerData->getPlayerId() << "]: Failed to decode notification(" << type << ")");
                    delete notify;
                }
                else
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::onAsync][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMEREMOVED.");                    
                    notification = notify;                    
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYREMOTEJOINFAILED:
            {
                NotifyRemoteJoinFailed* notify = BLAZE_NEW NotifyRemoteJoinFailed;
                if (decoder.decode(*payload, *notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession[" << mPlayerData->getPlayerId() << "]: Failed to decode notification(" << type << ")");
                    delete notify;
                }
                else
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameSession::onAsync][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYREMOTEJOINFAILED.");
                    notification = notify;
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYMATCHMAKINGFAILED:
            {
                NotifyMatchmakingFailed* notify = BLAZE_NEW NotifyMatchmakingFailed;
                if (decoder.decode(*payload, *notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession[" << mPlayerData->getPlayerId() << "]: Failed to decode notification(" << type << ")");
                    delete notify;
                }
                else
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameSession::onAsync][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYMATCHMAKINGFAILED.");
                    notification = notify;
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMEREPORTINGIDCHANGE:
            {
                NotifyGameReportingIdChange* notify = BLAZE_NEW NotifyGameReportingIdChange;
                if (decoder.decode(*payload, *notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession[" << mPlayerData->getPlayerId() << "]: Failed to decode notification(" << type << ")");
                    delete notify;
                }
                else
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::onAsync][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMEREPORTINGIDCHANGE.");                    
                    notification = notify;                    
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYHOSTMIGRATIONFINISHED:
            {
                NotifyHostMigrationFinished* notify = BLAZE_NEW NotifyHostMigrationFinished;
                if (decoder.decode(*payload, *notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession[" << mPlayerData->getPlayerId() << "]: Failed to decode notification(" << type << ")");
                    delete notify;
                }
                else
                {
                    BLAZE_INFO(BlazeRpcLog::gamemanager, "[GameSession::onAsync][%" PRIu64 "]: Received  NOTIFY_NOTIFYHOSTMIGRATIONFINISHED.", mPlayerData->getPlayerId());                    
                    notification = notify;                    
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYHOSTMIGRATIONSTART:
            {
                NotifyHostMigrationStart* notify = BLAZE_NEW NotifyHostMigrationStart;
                if (decoder.decode(*payload, *notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession[" << mPlayerData->getPlayerId() << "]: Failed to decode notification(" << type << ")");
                    delete notify;
                }
                else
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::onAsync][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYHOSTMIGRATIONFINISHED.");                    
                    notification = notify;                    
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS:
            {
                NotifyGameSetup* notify = BLAZE_NEW NotifyGameSetup;
                if (decoder.decode(*payload, *notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession[" << mPlayerData->getPlayerId() << "]: Failed to decode notification(" << type << ")");
                    delete notify;
                }
                else
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::onAsync][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS.");
                        notification = notify;
                    }
                }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLAYERCLAIMINGRESERVATION:
            {
                NotifyPlayerJoining* notify = BLAZE_NEW NotifyPlayerJoining;
                if (decoder.decode(*payload, *notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession[" << mPlayerData->getPlayerId() << "]: Failed to decode notification(" << type << ")");
                    delete notify;
                }
                else
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::onAsync][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERCLAIMINGRESERVATION.gameId=" << notify->getGameId());
                    notification = notify;
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMEATTRIBCHANGE:
            {
                NotifyGameAttribChange* notify = BLAZE_NEW NotifyGameAttribChange;
                if (decoder.decode(*payload, *notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession[" << mPlayerData->getPlayerId() << "]: Failed to decode notification(" << type << ")");
                    delete notify;
                }
                else
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::onAsync][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMEATTRIBCHANGE.gameId=" << notify->getGameId());                    
                    notification = notify;                    
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLAYERJOININGQUEUE:
            {
                NotifyPlayerJoining* notify = BLAZE_NEW NotifyPlayerJoining;
                if (decoder.decode(*payload, *notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession[" << mPlayerData->getPlayerId() << "]: Failed to decode notification(" << type << ")");
                    delete notify;
                }
                else
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::onAsync][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERJOININGQUEUE.gameId=" << notify->getGameId());                    
                    notification = notify;                    
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLAYERPROMOTEDFROMQUEUE:
            {
                NotifyPlayerJoining* notify = BLAZE_NEW NotifyPlayerJoining;
                if (decoder.decode(*payload, *notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession[" << mPlayerData->getPlayerId() << "]: Failed to decode notification(" << type << ")");
                    delete notify;
                }
                else
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::onAsync][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPLAYERPROMOTEDFROMQUEUE.gameId=" << notify->getGameId());                    
                    notification = notify;                    
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMESESSIONUPDATED:
            {
                GameSessionUpdatedNotification* notify = BLAZE_NEW GameSessionUpdatedNotification;
                if (decoder.decode(*payload, *notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession[" << mPlayerData->getPlayerId() << "]: Failed to decode notification(" << type << ")");
                    delete notify;
                }
                else
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::onAsync][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYGAMESESSIONUPDATED.gameId=" << notify->getGameId());
                    notification = notify;                    
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMESETTINGSCHANGE:
            {
                NotifyGameSettingsChange* notify = BLAZE_NEW NotifyGameSettingsChange;
                if (decoder.decode(*payload, *notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession[" << mPlayerData->getPlayerId() << "]: Failed to decode notification(" << type << ")");
            	    delete notify;
			    }
                else
                {    
				    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::onAsync]["<<mPlayerData->getPlayerId()<<"]: Received  NOTIFY_NOTIFYGAMESETTINGSCHANGE.gameId="<<notify->getGameId());
					notification = notify;				    
			    }
            }
            break;
        case UserSessionsSlave::NOTIFY_USERADDED:
            {
                NotifyUserAdded* notify = BLAZE_NEW NotifyUserAdded;
                if (decoder.decode(*payload, *notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "GameSession[" << mPlayerData->getPlayerId() << "]: Failed to decode notification(" << type << ")");
                    delete notify;
                }
                else
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GameSession::onAsync][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_USERADDED.playerId=" << notify->getUserInfo().getBlazeId());
                    //  notification = notify;
                }
            }
            break;
        case UserSessionsSlave::NOTIFY_USERUPDATED:
            {
                UserStatus* notify = BLAZE_NEW UserStatus;
                if (decoder.decode(*payload, *notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "GameSession[" << mPlayerData->getPlayerId() << "]: Failed to decode notification(" << type << ")");
                    delete notify;
                }
                else
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GameSession::onAsync][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_USERUPDATED.playerId=" << notify->getBlazeId());
                    //  notification = notify;
                }
            }
            break;
		case Blaze::Perf::PerfSlave::NOTIFY_NOTIFYPERFTUNE:
		{
			Blaze::Perf::NotifyPerfTuneResponse* notify = BLAZE_NEW Perf::NotifyPerfTuneResponse;
			if (decoder.decode(*payload, *notify) != ERR_OK)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession[" << mPlayerData->getPlayerId() << "]: Failed to decode notification(" << type << ")");
				delete notify;
			}
			else
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::NOTIFY_NOTIFYPERFTUNE][" << mPlayerData->getPlayerId() << "]: Is enable ? " << notify->getEnable());
				notification = notify;
			}
		}
		break;
		case Blaze::Perf::PerfSlave::NOTIFY_NOTIFYPINGRESPONSE:
		{
			Blaze::Perf::NotifyPingResponse* notify = BLAZE_NEW Perf::NotifyPingResponse;
			if (decoder.decode(*payload, *notify) != ERR_OK)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession[" << mPlayerData->getPlayerId() << "]: Failed to decode notification(" << type << ")");
				delete notify;
			}
			else
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::NOTIFY_NOTIFYPINGRESPONSE][" << mPlayerData->getPlayerId() << "]: Received  NOTIFY_NOTIFYPERFTUNE.");
				notification = notify;
			}
		}
		break;
        default:
            {
                //   payload not handled by recipient.
                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameSession::onAsync][" << mPlayerData->getPlayerId() << "]::GameData is NULL, receiving notification type" << type);
            }
            break;
    }
    if (notification && mPlayerData != NULL && mPlayerData->isPlayerActive() && mPlayerData->isConnected())
    {
        Selector::FiberCallParams params(Fiber::STACK_SMALL);
        gSelector->scheduleFiberCall(this, &GameSession::asyncHandlerFunction, type, notification, "GameSession::asyncHandlerFunction", params);
    }
}

void GameSession::finalizeGameCreation(GameId gameId/*= 0*/)
{
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::finalizeGameCreation]:"<<mPlayerData->getPlayerId());
	UpdateGameSessionRequest updateGameSessionRequest;
	if(0 == gameId)
	{
		gameId = myGameId;
	}
	updateGameSessionRequest.setGameId(gameId);
	updateGameSessionRequest.setNpSessionId(""); 
	//BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::finalizeGameCreation][" << mPlayerData->getPlayerId() << "] : game id = " << gameId);
	//finalizeGameCreation	
	BlazeRpcError err = mPlayerData->getComponentProxy()->mGameManagerProxy->finalizeGameCreation(updateGameSessionRequest);	
	if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession:finalizeGameCreation failed. Error(" <<ErrorHelp::getErrorName(err));                     
    }
}

BlazeRpcError GameSession::setGameAttributes(GameId  gameId, Collections::AttributeMap &gameAttributes) {
    BlazeRpcError err = ERR_OK;
    Blaze::GameManager::SetGameAttributesRequest request;
    request.setGameId(gameId);
    Collections::AttributeMap::iterator it = gameAttributes.begin();
    while (it != gameAttributes.end()) {
        request.getGameAttributes()[it->first] = it->second;
        ++it;
    }   
	err = mPlayerData->getComponentProxy()->mGameManagerProxy->setGameAttributes(request);
	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession:setGameAttributes failed. Error(" <<ErrorHelp::getErrorName(err) << ") " << mPlayerData->getPlayerId() << "  gameId "  << gameId);                     
	}
	else
	{
		BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "GameSession:setGameAttributes succeeded" << mPlayerData->getPlayerId() << "  gameId " << gameId);
	}
    return err;
}

BlazeRpcError GameSession::setPlayerAttributes(GameId  gameId, Collections::AttributeMap &attributes)
{
	BlazeRpcError err = ERR_OK;
	Blaze::GameManager::SetPlayerAttributesRequest request;
	if (0 == gameId)
	{
		gameId = myGameId;
	}
	request.setGameId(gameId);
	request.setPlayerId(mPlayerData->getPlayerId());
	Collections::AttributeMap::iterator it = attributes.begin();
	while (it != attributes.end()) {
		request.getPlayerAttributes()[it->first] = it->second;
		++it;
	}
	
	err = mPlayerData->getComponentProxy()->mGameManagerProxy->setPlayerAttributes(request);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "Utility->setPlayerAttributes failed with err=" << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError GameSession::setPlayerAttributes(const char8_t* key, const char8_t*  value, GameId gameId/*= 0*/)
{
    BlazeRpcError err = ERR_OK;
    Blaze::GameManager::SetPlayerAttributesRequest request;
	if(0 == gameId)
	{
		gameId = myGameId;
	}
	request.setGameId(gameId);
    request.setPlayerId(mPlayerData->getPlayerId());
    Blaze::Collections::AttributeMap& map = request.getPlayerAttributes();
    map[key] = value;
	err = mPlayerData->getComponentProxy()->mGameManagerProxy->setPlayerAttributes(request); 
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "Utility->setPlayerAttributes failed with err=" << ErrorHelp::getErrorName(err) );
    } 
	return err;
}

void GameSession::setGameSettings(uint32_t bits)
{
    BlazeRpcError err = ERR_OK;
    Blaze::GameManager::SetGameSettingsRequest request;
	request.setGameId(myGameId);
    request.getGameSettings().setBits(bits); //3104
	request.getGameSettingsMask().setBits(15);
	err = mPlayerData->getComponentProxy()->mGameManagerProxy->setGameSettings(request); 
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "Utility->getMyTournamentDetails failed with err=" << ErrorHelp::getErrorName(err));
    } 
}

void GameSession::updatePrimaryExternalSessionForUserStub(GameId gameId)
{
    GameActivityList activityList;
    GameActivity *pGameActivity = NULL;
    if (mPlayerData->getMyPartyGameId() != 0 && mPlayerData->getPartyGameActivity().getGameId() == gameId)
    {
        pGameActivity = BLAZE_NEW GameActivity;
        if (pGameActivity != NULL)
        {
            mPlayerData->getPartyGameActivity().copyInto(*pGameActivity);
            activityList.push_back(pGameActivity);
        }
    }
    else
    {
        if (mPlayerData->getMyPartyGameId() != 0 && mPlayerData->getMyPartyGameId() == mPlayerData->getPartyGameActivity().getGameId())
        {
            pGameActivity = BLAZE_NEW GameActivity;
            if (pGameActivity != NULL)
            {
                mPlayerData->getPartyGameActivity().copyInto(*pGameActivity);
                activityList.push_back(pGameActivity);
            }
        }
        if (mGameActivity.getGameId() != 0)
        {
            pGameActivity = BLAZE_NEW GameActivity;
            if (pGameActivity != NULL)
            {
                mGameActivity.copyInto(*pGameActivity);
                activityList.push_back(pGameActivity);
            }
        }
    }
    updatePrimaryExternalSessionForUser(&activityList);
}

BlazeRpcError GameSession::updatePrimaryExternalSessionForUser(GameActivityList* gameActivityList)
{
    BlazeRpcError err = ERR_SYSTEM;
    UpdatePrimaryExternalSessionForUserRequest req;
    UpdatePrimaryExternalSessionForUserResponse resp;
	UpdatePrimaryExternalSessionForUserErrorInfo error;
    if (gameActivityList != NULL && gameActivityList->size() > 0)
    {
        for (size_t i = 0; i < gameActivityList->size(); i++)
        {
            req.getCurrentGames().push_back((*gameActivityList)[i]);
        }
    }
    req.getUserIdentification().setBlazeId(mPlayerData->getPlayerId());
    req.getUserIdentification().setExternalId(mPlayerData->getExternalId());
    err = mPlayerData->getComponentProxy()->mGameManagerProxy->updatePrimaryExternalSessionForUser(req, resp, error);
    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession::updatePrimaryExternalSessionForUser: updatePrimaryExternalSessionForUser failed with " << (ErrorHelp::getErrorName(err)) << ")");
    }
    return err;
}

void GameSession::meshEndpointsConnected(BlazeObjectId& targetGroupId, uint32_t latency, uint32_t connFlags, GameId gameId /* = 0 */)
{
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::meshEndpointsConnected][" << mPlayerData->getPlayerId() << "] : game id = " << gameId);
    MeshEndpointsConnectedRequest req;
    //  if its a gamesession type game then myGameId would be set to game's id
    //  but if its a gamegroup type game then we use the passed gameId as we dont maintain a gameSession object for that.
    (gameId) ? req.setGameId(gameId) : req.setGameId(myGameId);
    req.getPlayerNetConnectionFlags().setBits(connFlags);
    req.setTargetGroupId(targetGroupId);
	if(latency != 0)
	{
		req.getQosInfo().setPacketLoss(0.00000000);
		req.getQosInfo().setLatencyMs(latency);
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "GameSession::meshEndpointsConnected [" << mPlayerData->getPlayerId() << " ]latency=" << latency);
	}

    BlazeRpcError err = mPlayerData->getComponentProxy()->mGameManagerProxy->meshEndpointsConnected(req);
    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession::meshEndpointsConnected[" << mPlayerData->getPlayerId() << "]: error " << ErrorHelp::getErrorName(err) << " meshEndpointsConnected failed for game id" << gameId);
    }
	else
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "Finished GameSession::meshEndpointsConnected [" << mPlayerData->getPlayerId() << " ]target=" <<
                        req.getTargetGroupId().toString() << " for game id " << gameId);
    }
}

void GameSession::meshEndpointsDisconnected(BlazeObjectId& targetGroupId, PlayerNetConnectionStatus disconnectStat ,uint32_t connFlags, GameId gameId /* = 0 */)
{
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::meshEndpointsDisconnected][" << mPlayerData->getPlayerId() << "] : game id = " << gameId);
   	MeshEndpointsDisconnectedRequest req;
	gameId ? req.setGameId(gameId) : req.setGameId(myGameId);
	req.getPlayerNetConnectionFlags().setBits(connFlags);
	req.setTargetGroupId(targetGroupId);
	req.setPlayerNetConnectionStatus(DISCONNECTED);
	
	BlazeRpcError err = ERR_OK;
    err = mPlayerData->getComponentProxy()->mGameManagerProxy->meshEndpointsDisconnected(req);
    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession::meshEndpointsDisconnected[" << mPlayerData->getPlayerId() << "]: error " << ErrorHelp::getErrorName(err) << " meshEndpointsDisconnected failed for game id" << myGameId);
    }
	else
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "Finished GameSession::meshEndpointsDisconnected[" << mPlayerData->getPlayerId() << " target=" <<
                        req.getTargetGroupId().toString() << " for game id" << gameId);
    }
 }

void GameSession::meshEndpointsConnectionLost(BlazeObjectId& targetGroupId, uint32_t connFlags, GameId gameId /* = 0 */)
{
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::meshEndpointsConnectionLost][" << mPlayerData->getPlayerId() << "] : game id = " << myGameId);
  	MeshEndpointsConnectionLostRequest req;
	(gameId) ? req.setGameId(gameId) : req.setGameId(myGameId);
	req.getPlayerNetConnectionFlags().setBits(connFlags);
	req.setTargetGroupId(targetGroupId);

	BlazeRpcError err = mPlayerData->getComponentProxy()->mGameManagerProxy->meshEndpointsConnectionLost(req);
    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession::meshEndpointsConnectionLost[" << mPlayerData->getPlayerId() << "]: error " << ErrorHelp::getErrorName(err) << " meshEndpointsConnectionLost failed for game id" << myGameId);
    }
	else
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "Finished GameSession::meshEndpointsConnectionLost[" << mPlayerData->getPlayerId() << " target=" <<
                        req.getTargetGroupId().toString() << " for game id" << myGameId);
    }
}

BlazeRpcError GameSession::cancelMatchMakingScenario(MatchmakingScenarioId scenarioId)
{
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession: cancelMatchmaking getting invoked by Player]" << mPlayerData->getPlayerId());
    CancelMatchmakingScenarioRequest req;
	req.setMatchmakingScenarioId(scenarioId);
    BlazeRpcError err = mPlayerData->getComponentProxy()->mGameManagerProxy->cancelMatchmakingScenario(req);
    if (ERR_OK != err)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession::cancelMatchmaking failed " << mPlayerData->getPlayerId() << " with Err =" << ErrorHelp::getErrorName(err));
    }
    else
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "GameSession::cancelMatchmaking successful" << mPlayerData->getPlayerId());
    }
    return err;
}

BlazeRpcError GameSession::advanceGameState(GameState state, GameId gameId)
{
    GameState curState = getPlayerState();
    BlazeRpcError err;
    AdvanceGameStateRequest req;
    (gameId) ? req.setGameId(gameId) : req.setGameId(myGameId);
	BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "GameSession::advanceGameState: " << mPlayerData->getPlayerId() << " (" << GameStateToString(curState) << " ==> " << GameStateToString(
		state) << ") for game (" << gameId << ")");

	if (!mPlayerData->getOwner()->isStressLogin() && curState == GameState::IN_GAME)
	{
		// ARUBA Calls
		if (StressConfigInst.getArubaEnabled())
		{
			mPlayerData->getArubaProxyHandler()->simulateArubaLoad();
		}

		// Reco Calls
		if (StressConfigInst.getRecoEnabled())
		{
			mPlayerData->getArubaProxyHandler()->simulateRecoLoad();
		}
	}
    req.setNewGameState(state);
    err = mPlayerData->getComponentProxy()->mGameManagerProxy->advanceGameState(req);
    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession::advanceGameState:" << mPlayerData->getPlayerId() << " (" << GameStateToString(curState) << " ==> " << GameStateToString(
                          state) << ") for game (" << gameId << ") failed. Error (" << (ErrorHelp::getErrorName(err)) << ")");
    }
    return err;
}

BlazeRpcError GameSession::leaveGameByGroup(PlayerId playerId, PlayerRemovedReason reason, GameId gameId)
{
    BlazeRpcError err = ERR_OK;
    RemovePlayerRequest req;
	(gameId) ? req.setGameId(gameId) : req.setGameId(myGameId);
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::leaveGameByGroup][" << mPlayerData->getPlayerId() << "] gameid= " << gameId);
    req.setPlayerId(playerId);
	//req.setPlayerId(mPlayerData->getPlayerId());	
    req.setPlayerRemovedReason(reason);
    req.setGroupId(mPlayerData->getConnGroupId());
	req.setPlayerRemovedTitleContext(0);
	req.setTitleContextString("");
    err = mPlayerData->getComponentProxy()->mGameManagerProxy->leaveGameByGroup(req);
    if (ERR_OK != err)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession::leaveGameByGroup: failed for gameId(" << gameId << ") failed. Error(" << (ErrorHelp::getErrorName(err)) << ") " << mPlayerData->getPlayerId());
    }
	else
	{
		BLAZE_INFO(BlazeRpcLog::gamemanager, "[FutPlayer::leaveGame][%" PRIu64 "]: player left game: GameId = %" PRIu64 "", mPlayerData->getPlayerId(), gameId);
	}
    return err;
}

BlazeRpcError GameSession::returnDedicatedServerToPool()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::returnDedicatedServerToPool][" << mPlayerData->getPlayerId() << "]");
    BlazeRpcError err = ERR_OK;
	ReturnDedicatedServerToPoolRequest req;
	req.setGameId(myGameId);
 
    err = mPlayerData->getComponentProxy()->mGameManagerProxy->returnDedicatedServerToPool(req);
    if (ERR_OK != err)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession::returnDedicatedServerToPool: failed for gameId(" << myGameId << ") failed. Error(" << (ErrorHelp::getErrorName(err)) << ") " << mPlayerData->getPlayerId());
    }
    return err;
}

BlazeRpcError GameSession::preferredJoinOptOut(GameId gameId)
{
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::preferredJoinOptOut][" << mPlayerData->getPlayerId() << "]");
    BlazeRpcError err = ERR_OK;
    if (myGameId)
    {
        PreferredJoinOptOutRequest req;
        req.setGameId(myGameId);
        err = mPlayerData->getComponentProxy()->mGameManagerProxy->preferredJoinOptOut(req);
        if (ERR_OK != err)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession::preferredJoinOptOut: failed for gameId(" << myGameId << ") failed. Error(" << (ErrorHelp::getErrorName(err)) << ")");
        }
    }
    return err;
}

BlazeRpcError GameSession::updateGameSession(GameId gameId)
{
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::updateGameSession][" << mPlayerData->getPlayerId() << "]");
    UpdateGameSessionRequest req;
	(gameId) ? req.setGameId(gameId) : req.setGameId(myGameId);
    req.setNpSessionId("");
    BlazeRpcError err = mPlayerData->getComponentProxy()->mGameManagerProxy->updateGameSession(req);
    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession::updateGameSession: failed for gameId(" << gameId << ")  Error(" << (ErrorHelp::getErrorName(err)) << ")");
    }
    return err;
}

BlazeRpcError GameSession::removePlayer(PlayerRemovedReason reason)
{
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::removePlayer][" << mPlayerData->getPlayerId() << "]");
    RemovePlayerRequest req;
    req.setGameId(myGameId);
    req.setPlayerId(mPlayerData->getPlayerId());
    req.setPlayerRemovedReason(reason);
    BlazeRpcError err = mPlayerData->getComponentProxy()->mGameManagerProxy->removePlayer(req);
    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession::removePlayer: failed for gameId " << mPlayerData->getPlayerId() << " (" << myGameId << ")  Error(" << (ErrorHelp::getErrorName(err)) << ")");
    }
    return err;
}

BlazeRpcError GameSession::replayGame()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameSession::replayGame][" << mPlayerData->getPlayerId() << "]");
    ReplayGameRequest req;
    req.setGameId(myGameId);
    BlazeRpcError err = mPlayerData->getComponentProxy()->mGameManagerProxy->replayGame(req);
    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GameSession::replayGame: failed for gameId(" << myGameId << ")  Error(" << (ErrorHelp::getErrorName(err)) << ")");
    }
    return err;
}

bool GameSession::IsInGameTimedOut()
{
    if (!mInGameStartTime)
    {
        return true;  //  If Game InTime not set return true to avoid player blocking in the same state.
    }
    TimeValue curTime = TimeValue::getTimeOfDay();
    int64_t  elapsedDurationMs = curTime.getMillis() - mInGameStartTime;
    int64_t  InGameDurationMs = getInGameDurationFromConfig() + Random::getRandomNumber(getInGameDurationDeviationFromConfig());

    return (elapsedDurationMs >= InGameDurationMs);
}

}  // namespace Stress
}  // namespace Blaze
