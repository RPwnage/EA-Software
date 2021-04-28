//  *************************************************************************************************
//
//   File:    dedicatedserver.cpp
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#include "./dedicatedservers.h"
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

DedicatedServer::DedicatedServer(StressPlayerInfo* playerData)
    : Player(playerData, CLIENT_TYPE_DEDICATED_SERVER),
      DedicatedServerGameSession(playerData, 22 /*gamesize*/, CLIENT_SERVER_DEDICATED /*Topology*/,
                             StressConfigInst.getGameProtocolVersionString() /*GameProtocolVersionString*/),
      mIsMarkedForLeaving(false)
{
}

DedicatedServer::~DedicatedServer()
{
    BLAZE_INFO(BlazeRpcLog::util, "[DedicatedServer][Destructor][%" PRIu64 "]: DedicatedServer destroy called", mPlayerInfo->getPlayerId());
    // stopReportTelemtery();
    //  deregister async handlers to avoid client crashes when disconnect happens
    deregisterHandlers();
    //  wait for some time to allow already scheduled jobs to complete
    sleep(30000);
}

void DedicatedServer::deregisterHandlers()
{
    DedicatedServerGameSession::deregisterHandlers();
}

BlazeRpcError DedicatedServer::simulateLoad()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServer::simulateLoad]:" << mPlayerData->getPlayerId());
    BlazeRpcError err = ERR_OK;
    
	err = createGame();       //  createDedicatedServerGame

    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[DedicatedServer][" << mPlayerData->getPlayerId() << "] Error While creating game ");
        return err;
    }



    int64_t playerStateTimer = 0;
    int64_t  playerStateDurationMs = 0;
    GameState   playerStateTracker = getPlayerState();
    uint16_t stateLimit = 0;
    bool ingame_enter = false;
    while (1)
    {
        if ( !mPlayerData->isConnected())
        {
            BLAZE_ERR_LOG(BlazeRpcLog::util, "[DedicatedServer::simulateLoad::User Disconnected = " << mPlayerData->getPlayerId());
            return ERR_DISCONNECTED;
        }
        if (playerStateTracker != getPlayerState())  //  state changed
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServer::simulateLoad:playerStateTracker]" << mPlayerData->getPlayerId() << " Time Spent in Sate: " << GameStateToString(
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
                if (playerStateDurationMs > DS_CONFIG.maxPlayerStateDurationMs)
                {
                    BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[DedicatedServer::simulateLoad:playerStateTracker][" << mPlayerData->getPlayerId() << "]: Player exceded maximum Game state Duration:" << playerStateDurationMs <<
                                   " ms" << " Current Game State:" << GameStateToString(getPlayerState()));
                    if (IN_GAME == getPlayerState() || PRE_GAME == getPlayerState())
                    {
                        mIsMarkedForLeaving = true;   //  Player exceed maximum duration in InGAME State so mark this Player to Leave from InGame
                    }
                }
            }
        }
        const char8_t * stateMsg = GameStateToString(getPlayerState());
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServer::simulateLoad]: Player: " << mPlayerInfo->getPlayerId() << ", GameId: " << myGameId << " Current state : " << stateMsg << " InStateDuration:" <<
                       playerStateDurationMs);
        switch (getPlayerState())
        {
            //   Sleep for all the below states
            case NEW_STATE:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=NEW_STATE");
                    stateLimit++;
                    if (stateLimit >= DS_CONFIG.GameStateIterationsMax)
                    {
                        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=NEW_STATE for long time and return");
                        return err;
                    }
                    //  Wait here if resetDedicatedserver() called/player not found for invite.
                    sleep(DS_CONFIG.GameStateSleepInterval);
                    break;
                }
            case PRE_GAME:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=PRE_GAME");
					sleep(10000);
					advanceGameState(IN_GAME);    
					sleep(DS_CONFIG.GameStateSleepInterval);
                    break;
                }
            case IN_GAME:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=IN_GAME");

                    if (!ingame_enter)
                    {
                        ingame_enter = true;
                        //simulateInGame();
                        //Start InGame Timer                           
                        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServer::simulateLoad]" << mPlayerData->getPlayerId() << " setInGameStartTime. gameID=" << myGameId);
                        setInGameStartTime();                        
                    }

                    if (RollDice(DS_CONFIG.inGameReportTelemetryProbability))
                    {
                        executeReportTelemetry();
                    }

					if( IsInGameTimedOut() || mIsMarkedForLeaving == true )
                    {                             
                        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServer::simulateLoad]" << mPlayerData->getPlayerId() << " IsInGameTimedOut/mIsMarkedForLeaving=true. gameID=" << myGameId);
                        advanceGameState(POST_GAME);
                        ingame_enter = false;
                    }
                    
                    sleep(DS_CONFIG.GameStateSleepInterval);
                    break;
                }
            case INITIALIZING:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=INITIALIZING");                                                                  
                    ingame_enter = false;
                    mIsMarkedForLeaving = false;		
					BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServer::simulateLoad][" << mPlayerData->getPlayerId() << "calling advanceGameState");
					advanceGameState(PRE_GAME);
                    sleep(DS_CONFIG.GameStateSleepInterval);
                    break;
                }
			case POST_GAME:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=POST_GAME");
                    // return to dedicated server pool            
                    returnDedicatedServerToPool();
                    ingame_enter = false;
                    mIsMarkedForLeaving = false;
                    sleep(DS_CONFIG.GameStateSleepInterval);
                    break;
                }
            case RESETABLE:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=RESETABLE");
                    //  mGameData will be freed when NOTIFY_NOTIFYGAMEREMOVED notification comes.
                    ingame_enter = false;
                    mIsMarkedForLeaving = false;
					connectedUserGroupIds.clear();
					sleep(DS_CONFIG.GameStateSleepInterval);
                    break;
                }
            case DESTRUCTING:
                {
                    //  Player Left Game
                    //  Presently using this state when user is removed from the game and return from this function.
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=DESTRUCTING");
                    //  Return here
                    return err;
                }
            case MIGRATING:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=MIGRATING");
                    //  mGameData will be freed when NOTIFY_NOTIFYGAMEREMOVED notification comes.
                    sleep(10000);
                    break;
                }
            default:
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServer::simulateLoad]" << mPlayerData->getPlayerId() << " with state=" << getPlayerState());
                    //  shouldn't come here
                    return err;
                }
        }
    }
}

BlazeRpcError DedicatedServer::createGame()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::gamereporting, "[DedicatedServer::createGame:: Player [" << mPlayerData->getPlayerId() << "]");

    BlazeRpcError err = ERR_OK;
    GameManager::CreateGameRequest request;
    CreateGameResponse response;
    if (mPlayerData->getConnection() == NULL || !(mPlayerData->getConnection()->connected()))
    {
        return ERR_SYSTEM;
    }
    CommonGameRequestData& commonGameRequestData = request.getCommonGameData();
    NetworkAddress* hostAddress = &(commonGameRequestData.getPlayerNetworkAddress());
    hostAddress->switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);

    IpPairAddress* ipPairAddress = hostAddress->getIpPairAddress();
    IpAddress* ipAddress = &(ipPairAddress->getExternalAddress());
    const InetAddress& inetAddress = mPlayerData->getConnection()->getAddress();

    ipAddress->setIp(inetAddress.getIp());
    ipAddress->setPort(inetAddress.getPort());
    ipAddress->copyInto(ipPairAddress->getInternalAddress());

    //commonGameRequestData.setGameType(GAME_TYPE_GAMESESSION);
    commonGameRequestData.setGameProtocolVersionString(getGameProtocolVersionString());
    
    request.setGamePingSiteAlias("bio-dub");
    BLAZE_INFO_LOG(BlazeRpcLog::gamereporting, "[DedicatedServer::createGame:: Player [" << mPlayerData->getPlayerId() << "] pingSiteName: " << mPlayerData->getPlayerPingSite());
    GameCreationData& gameData = request.getGameCreationData();
    gameData.setGameModRegister(0);
    char name[MAX_GAMENAME_LEN];
    blaze_snzprintf(name, sizeof(name), "DedicatedServerGame_%d", int(mPlayerData->getPlayerId()));
    gameData.setGameName(name);
    gameData.getGameSettings().setBits(63);
    gameData.setNetworkTopology(CLIENT_SERVER_DEDICATED);
    gameData.setMaxPlayerCapacity(22);
    gameData.setMinPlayerCapacity(1);
    //gameData.setPresenceMode(PRESENCE_MODE_STANDARD);
    gameData.setQueueCapacity(0);
    //gameData.setExternalSessionTemplateName("");
    //gameData.setVoipNetwork(VOIP_DEDICATED_SERVER);
    SlotCapacitiesVector&  slotCapacitiesVector = request.getSlotCapacities();
    slotCapacitiesVector.clear();
    slotCapacitiesVector.push_back(22);
    slotCapacitiesVector.push_back(0);
    slotCapacitiesVector.push_back(0);
    slotCapacitiesVector.push_back(0);
    request.setPersistedGameId("");

	if (BLAZE_IS_TRACE_RPC_ENABLED(BlazeRpcLog::gamemanager)) {
		char8_t buf[10240];
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "CreateGame RPC : \n" << "\n" << request.print(buf, sizeof(buf)));
	}
    err = mPlayerData->getComponentProxy()->mGameManagerProxy->createGame(request, response);

    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[DirtyBotsPlayerGameSession::createDedicatedServerGame " << this << "]: BOT [" << mPlayerData->getPlayerId() <<
                      "]  Failed with error :" << Blaze::ErrorHelp::getErrorName(err));
    }

    return err;
}

bool DedicatedServer::executeReportTelemetry()
{
    BlazeRpcError err = ERR_OK;
    if (!mPlayerData->isConnected())
    {
        BLAZE_ERR_LOG(BlazeRpcLog::util, "[DedicatedServer::ReportTelemetry::User Disconnected = " << mPlayerData->getPlayerId());
        return false;
    }
    if (getGameId() == 0)
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServer::ReportTelemetry][GameId for player = " << mPlayerInfo->getPlayerId() << "] not present");
        return false;
    }
    //StressGameInfo* ptrGameInfo = FutPlayerGameDataRef(mPlayerInfo).getGameData(getGameId());
    //if (ptrGameInfo == NULL)
    //{
    //    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServer::ReportTelemetry][GameData for game = " << getGameId() << "] not present");
    //    return false;
    //}
    TelemetryReportRequest reportTelemetryReq;
    reportTelemetryReq.setGameId(getGameId());
	reportTelemetryReq.setNetworkTopology(CLIENT_SERVER_DEDICATED);
    reportTelemetryReq.setLocalConnGroupId(mPlayerInfo->getConnGroupId().id);
    TelemetryReport* telemetryReport = reportTelemetryReq.getTelemetryReports().pull_back();
    telemetryReport->setLatencyMs(Random::getRandomNumber(1));
    int totalPackets = Random::getRandomNumber(20000);
    telemetryReport->setLocalPacketsReceived(totalPackets);
    telemetryReport->setRemotePacketsSent(totalPackets);
    telemetryReport->setPacketLossPercent(0);
    //telemetryReport->setRemoteConnGroupId(ptrGameInfo->getTopologyHostConnectionGroupId());
    err = mPlayerInfo->getComponentProxy()->mGameManagerProxy->reportTelemetry(reportTelemetryReq);
    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "DedicatedServer: reportTelemetry failed with err=" << ErrorHelp::getErrorName(err));
    }
    return true;
}

}  // namespace Stress
}  // namespace Blaze

