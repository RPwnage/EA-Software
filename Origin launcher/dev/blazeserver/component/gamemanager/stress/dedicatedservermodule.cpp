/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class DedicatedServerModule


*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "dedicatedservermodule.h"
#include "stressinstance.h"
#include "loginmanager.h"
#include "framework/connection/selector.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/config/config_map.h"

#include "framework/protocol/shared/heat2decoder.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/tdf/attributes.h"
#include "framework/util/random.h"

#include "gamemanager/tdf/gamemanager_server.h"

namespace Blaze
{
namespace Stress
{

/*
 * DedicatedServerModule.
 */

DedicatedServerModule::DedicatedServerModule()
{
    mRoleProbability_GameReseter = 0;
    mRoleProbability_DSCreator = 0;

    mCreateGameRequest = BLAZE_NEW CreateGameRequest();
    mStartMatchmakingRequest = BLAZE_NEW StartMatchmakingRequest();
    
    mGameNameIncrementor = 1;
    mNoOfTimesGameCreated = 0;
    mNoOfTimesResetDedicatedServerCalled = 0;
    mNoOfTimesStartMatchMakingCalled = 0;

    mGameEndMode = INVALID_UNSET;
    mGameLifespan = 0;
    mGameLifespanDeviation = 0;
    mMachineRegistrationLifespanMin = 0;
    mMachineRegistrationLifespanDeviation = 0;
}


DedicatedServerModule::~DedicatedServerModule()
{
    if (mLogFile != nullptr) fclose(mLogFile);
}


StressModule* DedicatedServerModule::create()
{
    return BLAZE_NEW DedicatedServerModule();
}


StressInstance* DedicatedServerModule::createInstance(StressConnection* connection, Login* login)
{
    return BLAZE_NEW DedicatedServerInstance(this, connection, login);
}


bool DedicatedServerModule::initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil)
{
    if (!StressModule::initialize(config, bootUtil)) 
    return false;

    if (!parseConfig(config)) 
    return false;

    return true;
}


bool DedicatedServerModule::parseConfig(const ConfigMap& config)
{
    if (!parseConfig_RoleProbabilities(config))
    {
        return false;
    }
    if (!parseConfig_MachineRegistration(config))
    {
        return false;
    }
    if (!parseConfig_CreateGameRequest(config))
    {
        return false;
    }
    if (!parseConfig_StartMatchmakingRequest(config))
    {
        return false;
    }

    // get endGameMode for platform hosts
    if (!parseConfig_GameEndMode(config))
    {
        return false; //err logged
    }

    // min game duration, and, time between reset attempts in seconds
    mGameLifespan = config.getUInt32("gmGameLifespan", (uint32_t)(-1));
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerModule].parseConfig: gmGameLifespan = " << mGameLifespan);
    mGameLifespanDeviation = config.getUInt32("gmGameLifespanDeviation", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerModule].parseConfig: gmGameLifespanDeviation = " << mGameLifespanDeviation);

    if (!openLogs())
    {
        return false;
    }
    return true;
}

bool DedicatedServerModule::parseConfig_RoleProbabilities(const ConfigMap& config)
{
    uint16_t totalProbability = 0;

    // Probability of non-dynamic (regular) DS Creator
    mRoleProbability_DSCreator = config.getUInt16("DedicatedServerCreatorProbabilty", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerModule].parseConfig_RoleProbabilities: DedicatedServerCreatorProbabilty = " << mRoleProbability_DSCreator);
    totalProbability += mRoleProbability_DSCreator;

    // Probability of Gm game reseter
    mRoleProbability_GameReseter = config.getUInt16("GameReseterProbability", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerModule].parseConfig_RoleProbabilities: GameReseterProbability = " << mRoleProbability_GameReseter);
    totalProbability += mRoleProbability_GameReseter;

    if (totalProbability != 100)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager,"[DedicatedServerModule].parseConfig_RoleProbabilities: Test issue in stress config, total role probability != 100, actual = " << totalProbability);
        return false;
    }
    return true;
}
///////// Dynamic Dedicated Server Creator role
bool DedicatedServerModule::parseConfig_MachineRegistration(const ConfigMap& config)
{
    // Setting of DSCreator Creator
    blaze_strnzcpy(mDDSCreatorRegisteredMachines.sGameProtocolVersionString, config.getString("GameProtocolVersionString", "stresstester"),32);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerModule].parseConfig_MachineRegistration: GameProtocolVersionString = " << mDDSCreatorRegisteredMachines.sGameProtocolVersionString);

    // min time between DS creator registrations in seconds
    mMachineRegistrationLifespanMin = config.getUInt32("DSCreatorMachineRegistrationLifespan", 300);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerModule].parseConfig_MachineRegistration: DSCreatorMachineRegistrationLifespan = " << mMachineRegistrationLifespanMin);
    mMachineRegistrationLifespanDeviation = config.getUInt32("DSCreatorMachineRegistrationLifespanDeviation", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerModule].parseConfig_MachineRegistration: DSCreatorMachineRegistrationLifespanDeviation = " << mMachineRegistrationLifespanDeviation);

    return true;
}
////////// GameManager reset/create game request used by DDSCreator at NotifyCreatedDynamicDedicatedServerGame, and by non-dynamic ds creators at execute
bool DedicatedServerModule::parseConfig_CreateGameRequest(const ConfigMap& config)
{
    // Setting of ViopNetwork 
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerModule].parseConfig_CreateGameRequest: ViopNetwork = VOIP_DISABLED");
    mCreateGameRequest->getGameCreationData().setVoipNetwork(VOIP_DISABLED);

    Blaze::ClientPlatformTypeList platList;
    const ConfigSequence* platformOverrideList = config.getSequence("clientPlatformListOverride");
    if (platformOverrideList != nullptr)
    {
        while (platformOverrideList->hasNext())
        {
            const char8_t* platformName = platformOverrideList->nextString("");
            Blaze::ClientPlatformType platformType;
            if (!ParseClientPlatformType(platformName, platformType))
            {
                BLAZE_ERR_LOG(Log::SYSTEM, "[DedicatedServerModule].parseConfig_CreateGameRequest: Invalid platform type listed: " << platformName << "\n");
                return false;
            }
            platList.insertAsSet(platformType);
        }
        mCreateGameRequest->getClientPlatformListOverride().clear();
        mCreateGameRequest->getClientPlatformListOverride().copyInto(platList);
    }
    
    char8_t strResetDedicatedGameName[32];
    blaze_strnzcpy(strResetDedicatedGameName, config.getString("ResetDedicatedGameName", "ResetDedicatedGame"),32);
    mCreateGameRequest->getGameCreationData().setGameName(strResetDedicatedGameName);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerModule].parseConfig_CreateGameRequest: ResetDedicatedGameName = " << mCreateGameRequest->getGameCreationData().getGameName());
    
    // Setting of GameNetworkTopology CLIENT_SERVER_DEDICATED
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerModule].parseConfig_CreateGameRequest: GameNetworkTopology = CLIENT_SERVER_DEDICATED");   
    mCreateGameRequest->getGameCreationData().setNetworkTopology(CLIENT_SERVER_DEDICATED);

    // Setting of MaxPlayerCapacity
    mCreateGameRequest->getGameCreationData().setMaxPlayerCapacity (config.getUInt16("MaxPlayerCapacity", 32));
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerModule].parseConfig_CreateGameRequest: MaxPlayerCapacity = " << mCreateGameRequest->getGameCreationData().getMaxPlayerCapacity());

    // Setting of PublicSlotCapacity
    mCreateGameRequest->getSlotCapacities()[GameManager::SLOT_PUBLIC_PARTICIPANT] = config.getUInt16("PublicSlotCapacity", 32);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerModule].parseConfig_CreateGameRequest: PublicSlotCapacity = " << (mCreateGameRequest->getSlotCapacities()[GameManager::SLOT_PUBLIC_PARTICIPANT]));

    // Setting of PrivateSlotCapacity
    mCreateGameRequest->getSlotCapacities()[GameManager::SLOT_PRIVATE_PARTICIPANT] = config.getUInt16("PrivateSlotCapacity", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerModule].parseConfig_CreateGameRequest: PrivateSlotCapacity = " << (mCreateGameRequest->getSlotCapacities()[GameManager::SLOT_PRIVATE_PARTICIPANT]));

    // Setting of GamePingSiteAlias
    // Note that this will be overridden on login if useServerQosSettings is enabled
    mCreateGameRequest->setGamePingSiteAlias(config.getString("GamePingSiteAlias", ""));
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerModule].parseConfig_CreateGameRequest: GamePingSiteAlias = " << mCreateGameRequest->getGamePingSiteAlias());

    // set game mods
    mCreateGameRequest->getGameCreationData().setGameModRegister(config.getUInt32("GameModRegister", 0));
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerModule].parseConfig_CreateGameRequest: GameModRegister = " << mCreateGameRequest->getGameCreationData().getGameModRegister());

    mCreateGameRequest->getCommonGameData().setGameProtocolVersionString(mDDSCreatorRegisteredMachines.sGameProtocolVersionString);

    blaze_strnzcpy(mTemplateName, config.getString("TemplateName", ""), 64);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerModule].parseConfig_TemplateName: TemplateName = " << getTemplateName());

    return true;
}

//////// Matchmaking
bool DedicatedServerModule::parseConfig_StartMatchmakingRequest(const ConfigMap& config)
{
    char8_t strMatchMakerGameName[32];
    blaze_strnzcpy(strMatchMakerGameName, config.getString("MatchmakerGameName", "MatchmakerGame"),32);
    mStartMatchmakingRequest->getGameCreationData().setGameName(strMatchMakerGameName);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerModule].parseConfig_StartMatchmakingRequest: MatchmakerGameName = " << mStartMatchmakingRequest->getGameCreationData().getGameName());

    // Setting of MatchmakingTimeout
    mStartMatchmakingRequest->getSessionData().getSessionDuration().setMillis(config.getUInt32("MatchmakingTimeout", 0));
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerModule].parseConfig_StartMatchmakingRequest: MatchmakingTimeout = " << mStartMatchmakingRequest->getSessionData().getSessionDuration().getMillis());

    // Setting of ViopNetwork  and NetwrokTopology
    mStartMatchmakingRequest->getGameCreationData().setVoipNetwork(VOIP_DISABLED);
    mStartMatchmakingRequest->getGameCreationData().setNetworkTopology(CLIENT_SERVER_DEDICATED);

    // Setting of GameSizeMinFitThresholdName
    mStartMatchmakingRequest->getCriteriaData().getPlayerCountRuleCriteria().setRangeOffsetListName(config.getString("GameSizeMinFitThresholdName", ""));
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerModule].parseConfig_StartMatchmakingRequest: GameSizeMinFitThresholdName = " << mStartMatchmakingRequest->getCriteriaData().getPlayerCountRuleCriteria().getRangeOffsetListName());

    // Setting of MinPlayerCount
    mStartMatchmakingRequest->getCriteriaData().getPlayerCountRuleCriteria().setMinPlayerCount( config.getUInt16("MinPlayerCount", 1) );
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerModule].parseConfig_StartMatchmakingRequest: MinPlayerCount = " << mStartMatchmakingRequest->getCriteriaData().getPlayerCountRuleCriteria().getMinPlayerCount());

    // Setting of MaxPlayerCount
    mStartMatchmakingRequest->getCriteriaData().getPlayerCountRuleCriteria().setMaxPlayerCount(config.getUInt16("MaxPlayerCount", 1) );
    mStartMatchmakingRequest->getCriteriaData().getTotalPlayerSlotsRuleCriteria().setMaxTotalPlayerSlots(config.getUInt16("MaxPlayerCount", 1) );
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerModule].parseConfig_StartMatchmakingRequest: MaxPlayerCount = " << mStartMatchmakingRequest->getCriteriaData().getPlayerCountRuleCriteria().getMaxPlayerCount());

    // Setting of DesiredGameSize
    mStartMatchmakingRequest->getCriteriaData().getPlayerCountRuleCriteria().setDesiredPlayerCount( config.getUInt16("DesiredGameSize", 1) );
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerModule].parseConfig_StartMatchmakingRequest: DesiredGameSize = " << mStartMatchmakingRequest->getCriteriaData().getPlayerCountRuleCriteria().getDesiredPlayerCount());

    return true;
}

// parse end game mode
bool DedicatedServerModule::parseConfig_GameEndMode(const ConfigMap& config)
{
    char8_t strGameEndMode[32];
    blaze_strnzcpy(strGameEndMode, config.getString("gameEndMode", "INVALID_UNSET"),32);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerModule].parseConfig_GameEndMode: gameEndMode = " <<  strGameEndMode);
    if (blaze_strncmp(strGameEndMode, "DESTROY", sizeof(strGameEndMode)) == 0)
    {
        mGameEndMode = DESTROY;
        return true;
    }
    else if (blaze_strncmp(strGameEndMode, "RETURN_TO_POOL", sizeof(strGameEndMode)) == 0)
    {
        mGameEndMode = RETURN_TO_POOL;
        return true;
    }
    else
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager,"[DedicatedServerModule].parseConfig_GameEndMode: Unhandled string " << strGameEndMode << " does not match possible enum values");
        return false;
    }
}

bool DedicatedServerModule::openLogs()
{

    uint32_t tyear, tmonth, tday, thour, tmin, tsec;
    TimeValue tnow = TimeValue::getTimeOfDay(); 
    tnow.getGmTimeComponents(tnow,&tyear,&tmonth,&tday,&thour,&tmin,&tsec);
    char8_t fName[256];
    blaze_snzprintf(fName, sizeof(fName), "%s/%s_stress_%02d%02d%02d%02d%02d%02d.log",(Logger::getLogDir()[0] != '\0')?Logger::getLogDir():".",getModuleName(), tyear, tmonth, tday, thour, tmin, tsec);

    
    mLogFile = fopen(fName, "wt");
    if (mLogFile == nullptr)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager,"[StatsModule].openLogs: Could not create log file("<<fName<<") in folder("<<Logger::getLogDir()<<")");
        return false;
    }

    return true;  
}

bool DedicatedServerModule::saveStats()
{
    if (mLogFile != nullptr)
    {
        fprintf(mLogFile, "+----------------------------+--------+---------+-----------+-----------+--------+--------+--------+---------+--------+----------+\n");
        fprintf(mLogFile,"[DedicatedServerModule] Number of times games are created = %d.\n", mNoOfTimesGameCreated );
        fprintf(mLogFile,"[DedicatedServerModule] Number of times Reset dedicated server methods are called = %d.\n", mNoOfTimesResetDedicatedServerCalled );
        fprintf(mLogFile,"[DedicatedServerModule] Number of times start matchmaking sessions are called = %d.\n", mNoOfTimesStartMatchMakingCalled );

        fflush(mLogFile);
    }
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerModule].saveStats: Number of times games are created = " <<  mNoOfTimesGameCreated); 
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerModule].saveStats: Number of times Reset dedicated server methods are called  = " <<  mNoOfTimesResetDedicatedServerCalled); 
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerModule].saveStats: Number of times start matchmaking sessions are called = " <<  mNoOfTimesStartMatchMakingCalled); 
    return false;
}


/*
 * DedicatedServerInstance.
 */ 

DedicatedServerInstance::DedicatedServerInstance(DedicatedServerModule* owner, StressConnection* connection, Login* login)
        : StressInstance(owner, connection, login, BlazeRpcLog::gamemanager),
          mOwner(owner),
          mProxy(BLAZE_NEW GameManagerSlave(*getConnection())),
          mName("dedicatedserver"),
          mRole(ROLE_INVALID),
          mBlazeId(0),
          mGameId(0),
          mCyclesLeft(0),
          mGotPingSiteLatencyUpdate(false),
          mIsWaitingForGameInfo(false),
          mBestPingSiteAlias(nullptr),
          mBestPingSiteChanged(false)
{
    connection->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMESETUP, this);
    connection->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMESTATECHANGE, this);
    connection->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMEREPORTINGIDCHANGE, this);
    connection->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINING, this);
    connection->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLAYERREMOVED, this);

    // init mRole
    initRole();
}

DedicatedServerInstance::~DedicatedServerInstance()
{
    delete mProxy;
}

const char8_t * DedicatedServerInstance::getName() const { return mName; }

void DedicatedServerInstance::start()
{
    StressInstance::start();
}

/*! ************************************************************************************************/
/*! \brief StressInstance::execute implementation
***************************************************************************************************/
BlazeRpcError DedicatedServerInstance::execute()
{
    BlazeRpcError result = ERR_OK;

    if (mRole == ROLE_INVALID)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerInstance].execute: Stress client aborted execute() due to invalid role.");
        return ERR_OK;
    }
    if (mCyclesLeft > 0)
    {
        mCyclesLeft--;
    }

    // If our role is game creator and the best ping alias changed,
    // we need to create a new game with the new ping site at the next opportunity
    if (mCyclesLeft != 0 && (mRole != ROLE_DSCREATOR || !mBestPingSiteChanged))
    {
        return ERR_OK;
    }

    // mCyclesLeft == 0 here. execute instance's role.


    // handle ROLE_DSCREATOR
    if (mRole == ROLE_DSCREATOR)
    {
        if (getGameId() == 0)
        {
            Blaze::GameManager::CreateGameRequest createRequest;
            mOwner->getCreateGameRequest()->copyInto(createRequest);
            if (mOwner->getUseServerQosSettings())
                createRequest.setGamePingSiteAlias(mBestPingSiteAlias);
            result = actionCreateGame_DSCreator(createRequest, mOwner->getTemplateName());
            mCyclesLeft = generateRandomValue(mOwner->getMinMachineRegistrationLifespanCycles(), mOwner->getMachineRegistrationLifespanDeviation());
            mBestPingSiteChanged = false;
        }
        else
        {
            if (!mBestPingSiteChanged || getGameState() == RESETABLE)
                result = actionDestroyGame();
        }
    }
    // handle ROLE_GAMERESETER
    else if (mRole == ROLE_GAMERESETER)
    {
        // Delay further actions //STRESS_TODO: implement when time for convenience
        /*TimeValue curTime = TimeValue::getTimeOfDay();
        if (curTime.getMillis() <= (mStartTime.getMillis() + mOwner->getGameManagerDelay())) 
        {
            return ERR_OK;
        }*/

        if (isWaitingForGameInfo())
        {
            BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerInstance].execute: Game host is waiting for game (" << getGameId() << ") info.");
            return ERR_OK;
        }
        else if (getGameId() == 0)
        {
            result = actionResetDedicatedServer();

            // side: delay till next, even if reset fails, to avoid result-skewing retry spamming, in case failed
            // due to momentary delay setting up or exhaustion of ded servers.
            mCyclesLeft = generateRandomValue(mOwner->getGameLifespanCycles(), mOwner->getGameLifespanDeviation());
        }
        else if (getGameId() != 0)
        {
            // exit game (to reset new one next cycle)
            if (getGameState() != Blaze::GameManager::RESETABLE)
            {
                switch (mOwner->getGameEndMode())
                {
                case DESTROY:
                    result = actionDestroyGame();
                    break;
                case RETURN_TO_POOL:
                    result = actionReturnDsToPool();
                    break;
                default:
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerInstance].execute: Test issue: unhandled end mode in execute() unable to end GameId=" << mGameId);
                };
            }
        }   
    }//ROLE_GAMERESETER 

    return result;
}

/*! ************************************************************************************************/
/*! \brief onAsync handles appropriate gm and related notifications below
***************************************************************************************************/
void DedicatedServerInstance::onAsync(uint16_t component, uint16_t iType, RawBuffer* payload)
{
    if (component == GameManagerSlave::COMPONENT_ID)
    {
        Heat2Decoder decoder;

        //  GameManager async updates
        GameManagerSlave::NotificationType type = (GameManagerSlave::NotificationType)iType;
        switch (type)
        {
        case GameManagerSlave::NOTIFY_NOTIFYGAMESETUP:
            {
                NotifyGameSetup notify;
                if (decoder.decode(*payload, notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerInstance].onAsync: Failed to decode notification(" << type << ")");
                }
                else
                {
                    //bsze_todo: temp to disable or convert to metric:
                    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerInstance].onAsync: Notify Game Setup for (gameId:" << notify.getGameData().getGameId() << ", state:" << Blaze::GameManager::GameStateToString(notify.getGameData().getGameState()) << ")");

                    const ResetDedicatedServerSetupContext *resetDsSetupContext = notify.getGameSetupReason().getResetDedicatedServerSetupContext();
                    if (resetDsSetupContext != nullptr)
                    {
                        if (resetDsSetupContext->getJoinErr() != Blaze::ERR_OK)
                        {
                            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerInstance].onAsync: OnNotifyGameSetup for Dynamic DS reserved gameId (" << getGameId() << ") failed with error: " << Blaze::ErrorHelp::getErrorName(static_cast<BlazeRpcError>(resetDsSetupContext->getJoinErr())));
                            clearIsWaitingForGameInfo();
                            setGameId(0);
                            break;
                        }
                    }

                    // save off latest game we're in to mGameInfo
                    if ((getGameId() != 0) && (getGameId() != notify.getGameData().getGameId()))
                    {            
                        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerInstance].onAsync: Receiving notification(" << type <<
                            ") for game (" << notify.getGameData().getGameId() << ") though the instance already is in/joining a game (" << getGameId() << ")");
                    }           
                    else
                    {
                        //for extra debugging, uncomment this to see the full notification game data:
                        //char buf[20480];
                        //notify.print(buf, sizeof(buf));
                        //BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[StressInstance][%d][user(%" PRIu64 ")]: DEBUG: notifcation ('%s') data: " <<  notify.getClassName(), buf);
                        notify.copyInto(mGameInfo);
                        clearIsWaitingForGameInfo();
                    }

                    // we are successfully setting up the local game, after reseting the dynamic DS here. update the network and finalize game
                    const bool isImTopologyHost = (notify.getGameData().getTopologyHostInfo().getPlayerId() == getBlazeId());
                    const bool isImPlatformHost = (notify.getGameData().getPlatformHostInfo().getPlayerId() == getBlazeId());
                    const bool isClientServer = (notify.getGameData().getNetworkTopology() == CLIENT_SERVER_DEDICATED) || (notify.getGameData().getNetworkTopology() == CLIENT_SERVER_PEER_HOSTED);
                    if (isImTopologyHost || isImPlatformHost)
                    {
                        //    schedule a finalizeGameCreation call without being the topology host
                        gSelector->scheduleFiberCall(this, &DedicatedServerInstance::finalizeGameCreation, notify.getGameData().getGameId(), isImPlatformHost, isImTopologyHost,
                            "DedicatedServerInstance::finalizeGameCreation");
                    }

                    // setup the network. update my connection to the topology host if I'm a player in game (and not the topology host).
                    if (isClientServer)
                    {
                        //    isImTopologyHost should always be false here but to make sure...
                        if (!isImTopologyHost)
                        {
                            ReplicatedGamePlayerList &roster = mGameInfo.getGameRoster();
                            if (roster.size() > 0)
                            {
                                ConnectionGroupId sourceGroupId = 0;
                                //  update player roster in game object
                                ReplicatedGamePlayerList::const_iterator citEnd = roster.end();
                                for (ReplicatedGamePlayerList::const_iterator cit = roster.begin(); cit != citEnd; ++cit)
                                {
                                    //  sync local player roster with the latest info.
                                    PlayerId targetPlayerId = (*cit)->getPlayerId();
                                    if (targetPlayerId == getBlazeId())
                                    {
                                        sourceGroupId = (*cit)->getConnectionGroupId();
                                    }
                                }
                                //    schedule a updateHostConnection call
                                gSelector->scheduleFiberCall(this, &DedicatedServerInstance::updateHostConnection, 
                                    notify.getGameData().getGameId(), sourceGroupId, 
                                    notify.getGameData().getTopologyHostInfo().getConnectionGroupId(), CONNECTED,
                                    "DedicatedServerInstance::updateHostConnection");
                            }
                        }
                    }                
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMESTATECHANGE: //bsze_todo: check if we really need game state, else remove this block
            {
                // update game state for reference
                NotifyGameStateChange notify;
                if (decoder.decode(*payload, notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerInstance].onAsync: Failed to decode notification(" << type << ")");
                }
                else if (getGameId() == notify.getGameId())
                {
                    setGameState(notify.getNewGameState());//bsze_todo: temp to disable or convert to metric:
                    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerInstance].onAsync: Notify Game State Change (gameId:" << getGameId() << ", state:" << Blaze::GameManager::GameStateToString(getGameState()) << ")");
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYGAMEREPORTINGIDCHANGE:
            {
                NotifyGameReportingIdChange notify;
                if (decoder.decode(*payload, notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerInstance].onAsync: Failed to decode notification(" << type << ")");
                }
                else if (getGameId() == notify.getGameId())
                {
                    GameReportingId oldId = mGameInfo.getGameData().getGameReportingId();
                    mGameInfo.getGameData().setGameReportingId(notify.getGameReportingId());
                    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerInstance].onAsync: Notify Game Reporting ID Change (gameId:" << getGameId()
                        << ", old report id:" << oldId << ", new report id:" << mGameInfo.getGameData().getGameReportingId() << ")");
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINING:
            {
                NotifyPlayerJoining notify;
                if (decoder.decode(*payload, notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerInstance].onAsync: Failed to decode notification(" << type << ")");
                }
                else 
                {
                    // update other player's connection status to host
                    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerInstance].onAsync: Notify Player Joining (gameId:" << notify.getGameId() << ")");
                    //    schedule a updateHostConnection call
                    gSelector->scheduleFiberCall(this, &DedicatedServerInstance::updateHostConnection, 
                        mGameInfo.getGameData().getGameId(), mGameInfo.getGameData().getTopologyHostInfo().getConnectionGroupId(), 
                        notify.getJoiningPlayer().getConnectionGroupId(), CONNECTED,
                        "DedicatedServerInstance::updateHostConnection");
                }
            }
            break;
        case GameManagerSlave::NOTIFY_NOTIFYPLAYERREMOVED:
            {
                NotifyPlayerRemoved notify;
                if (decoder.decode(*payload, notify) != ERR_OK)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerInstance].onAsync: Failed to decode notification(" << type << ")");
                }
                else
                {
                    ReplicatedGamePlayerList::iterator itCur = mGameInfo.getGameRoster().begin();
                    ReplicatedGamePlayerList::const_iterator itEnd = mGameInfo.getGameRoster().end();
                    for (; itCur != itEnd; ++itCur)
                    {
                        if ((*itCur)->getPlayerId() == notify.getPlayerId())
                        {
                            break;
                        } 
                    }
                    if (itCur != itEnd)
                    {
                        mGameInfo.getGameRoster().erase(itCur);
                    }

      
                    if (notify.getPlayerId() == getBlazeId())
                    {
                        //unregister the game with this instance
                        if (getGameId() == notify.getGameId())
                        {                            //bsze_todo: temp to disable or convert to metric:
                            BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerInstance].onAsync: Notify Player Removed (reason:" << Blaze::GameManager::PlayerRemovedReasonToString(notify.getPlayerRemovedReason()) << ", gameId:" << getGameId() << ", state:" << Blaze::GameManager::GameStateToString(getGameState()) << ")");
                            setGameId(0);
                        }
                        else
                        {
                            BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerInstance].onAsync: Player remove notification recieved for different game(" << notify.getGameId() << ") than the one single game player expected in (" << getGameId() << ")");
                        }
                    }
                } 
            }   
            break;
        default:
            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerInstance].onAsync: Unsupported notification(" << type << ") - addition to GameManagerUtilInstance required.");
            break;
        }; // switch-case
    }
}

/*! ************************************************************************************************/
/*! \brief updates ping site latency for the stress client
***************************************************************************************************/
void DedicatedServerInstance::onLogin(Blaze::BlazeRpcError result)
{
    if (mOwner->getUseServerQosSettings())
    {
        // Initialize seed for ping site alias selection
        Random::initializeSeed();
    }
    if (result == Blaze::ERR_OK)
    {
        mBlazeId = getLogin()->getUserLoginInfo()->getBlazeId();
        if (!getOwner()->getUseServerQosSettings())
            updateNetworkInfo();
    }
    StressInstance::onLogin(result);
}
//  if instance logs out, invoke this method to reset state
void DedicatedServerInstance::onLogout(BlazeRpcError result)
{
    setGameId(0);
    clearIsWaitingForGameInfo();
    mCyclesLeft = 0;

    StressInstance::onLogout(result);
}

/*! ************************************************************************************************/
/*! \brief determine and set role for the instance.
    \return the role. ROLE_INVALID indicates error.
***************************************************************************************************/
Role DedicatedServerInstance::initRole()
    {
    // user configured percentage for the Role.
    const uint16_t instanceRole = (uint16_t)Random::getRandomNumber(99); //stress_todo: are there test reasons to keep tool *randomizing* (inconsistent) counts rather than keeping count consistent every run by assigning stress instances to exact consistent proportion of total?
    uint16_t bound = 0;
    mRole = ROLE_INVALID;

    // do rolled ROLE_GAMERESETER
    bound += mOwner->getRoleProbability_GameReseter();
    if (instanceRole < bound)
            {
        mRole = ROLE_GAMERESETER;
        return mRole;
            }

    // do rolled ROLE_DSCREATOR
    bound += mOwner->getRoleProbability_DsCreator();
    if (instanceRole < bound)
    {
        mRole = ROLE_DSCREATOR;
        return mRole;
        }

    //STRESS_AUDIT: MM role not implemented. For testing MM, may be easier to just reuse mm module (rem code from here)
    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerInstance].initRole: Test issue in stress tool, unable init a valid role for player");     
    return mRole;
    }


/*! ************************************************************************************************/
/*! \brief ResetDedicatedServer action. validates ping sites set up for dynamic dedicated server testing.
    After this call, caller should set appropriate wait time till next call.
    If rpc call successful, calls setGameId() and setIsWaitingForGameInfo()
***************************************************************************************************/
BlazeRpcError DedicatedServerInstance::actionResetDedicatedServer()
{
    if (!validatePingSiteLatenciesUpdated())
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerInstance].actionResetDedicatedServer: Aborting attempt at create/reset dedicated server, as ping site latencies have not yet been initialized!");
        return Blaze::ERR_SYSTEM;
    }

    // clear is waiting for game info will get called after we receive
    // the replicated game data from the server, either
    // either on the response for a dedicated server or in the join
    // game notification for peer to peer games.
    // STRESS_TODO: need a timeout in case we never get the join game notification.
    setIsWaitingForGameInfo();

    //Create the Game using resetDedicatedServer() 
    CreateGameRequest createGameRequest; 
    BlazeRpcError blazeError = Blaze::ERR_OK;
    mOwner->getCreateGameRequest()->copyInto(createGameRequest);
    if (mOwner->getUseServerQosSettings())
        createGameRequest.setGamePingSiteAlias(mBestPingSiteAlias);


    char8_t gameName[Blaze::GameManager::MAX_GAMENAME_LEN];
    blaze_snzprintf(gameName, sizeof(gameName), "%s%u", createGameRequest.getGameCreationData().getGameName(), mOwner->getGameNameIncrementor());
    mOwner->incrementGameNameIncrementor();

    createGameRequest.getGameCreationData().setGameName(gameName);
    createGameRequest.getGameCreationData().getGameSettings().setOpenToBrowsing();
    createGameRequest.getGameCreationData().getGameSettings().setOpenToMatchmaking();
    createGameRequest.getGameCreationData().getGameSettings().setOpenToInvites();
    createGameRequest.getGameCreationData().getGameSettings().setOpenToJoinByPlayer();
    createGameRequest.getGameCreationData().getGameSettings().setAllowSameTeamId();
    createGameRequest.getGameCreationData().getGameSettings().setHostMigratable();
    createGameRequest.getGameCreationData().getGameSettings().setJoinInProgressSupported();
    createGameRequest.getGameCreationData().getGameSettings().clearRanked();
    createGameRequest.getCommonGameData().setGameProtocolVersionString(mOwner->getDSCreatorRegisterMachines().sGameProtocolVersionString);
    if (createGameRequest.getCommonGameData().getPlayerNetworkAddress().getActiveMember() == NetworkAddress::MEMBER_UNSET)
    {
        // need even a garbage network address here to avoid error on game create
        NetworkAddress *hostAddress = &createGameRequest.getCommonGameData().getPlayerNetworkAddress();
        hostAddress->switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
        hostAddress->getIpPairAddress()->getExternalAddress().setIp(getConnection()->getAddress().getIp());
        hostAddress->getIpPairAddress()->getExternalAddress().setPort(getConnection()->getAddress().getPort());
        hostAddress->getIpPairAddress()->getExternalAddress().copyInto(hostAddress->getIpPairAddress()->getInternalAddress());
    }


    JoinGameResponse joinGameResponse;
    blazeError = mProxy->resetDedicatedServer(createGameRequest, joinGameResponse);
    mOwner->IncrementNoOfTimesResetDedicatedServerCalled();

    if(blazeError == Blaze::ERR_OK)
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerInstance].actionResetDedicatedServer: Reset dedicated server call successfully made. Expecting to reset (possibly dynamic ds) reserved gameId(" << joinGameResponse.getGameId() << ")");
        setGameId(joinGameResponse.getGameId());
    }
    else
    {
        // clear await flag after setting it above
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerInstance].actionResetDedicatedServer: actionResetDedicatedServer: GM reseter failed to Reset dedicated server (attempted game name=" << createGameRequest.getGameCreationData().getGameName() << "), with error " << Blaze::ErrorHelp::getErrorName(blazeError));
        clearIsWaitingForGameInfo();
    }
    return blazeError;
}

/*! ************************************************************************************************/
/*! \brief destroys my current game and sets my GameId to zero. Use to remove dedicated servers
***************************************************************************************************/
BlazeRpcError DedicatedServerInstance::actionDestroyGame()
{
    DestroyGameRequest req;
    DestroyGameResponse resp;
    req.setGameId(getGameId());
    BlazeRpcError err = mProxy->destroyGame(req, resp);
    if (err != ERR_OK)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerInstance].actionDestroyGame: Destroying game(" << getGameId() << ") got error " << ErrorHelp::getErrorName(err));
        if (err == GAMEMANAGER_ERR_INVALID_GAME_ID)
        {
            err = ERR_OK;  // rare event something else beat us to destroy?
        }
    }
    else
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerInstance].actionDestroyGame: Removed player from GameId=" << getGameId());
    }
    setGameId(0);
    return err;
}

/*! ************************************************************************************************/
/*! \brief return (non-dynamic) dedicated server I'm in to pool and sets my GameId to zero. Use to return non-dynamic regular CSD game.
    \return error code. ERR_OK if already resetable.
***************************************************************************************************/
BlazeRpcError DedicatedServerInstance::actionReturnDsToPool()
{
    if (getGameState() == RESETABLE)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerInstance].actionReturnDsToPool: Rried returning GameId=" << getGameId() << " when already in RESETABLE state, aborting call");
        return ERR_OK;
    }

    ReturnDedicatedServerToPoolRequest req;
    req.setGameId(getGameId());
    BlazeRpcError err = mProxy->returnDedicatedServerToPool(req);    
    if (err == ERR_OK)
    {
        //  is this valid?   Shouldn't this come down in some kind of notification?
        setGameState(RESETABLE);
        setGameId(0);
    }
    return err;
}

/*! ************************************************************************************************/
/*! \brief Helper wrapper to createGame(). For non-DDSCreators, create new CSD game, via GM (non-dynamic).
    Calls setsGameId() on returned GameId
***************************************************************************************************/
BlazeRpcError DedicatedServerInstance::actionCreateGame_DSCreator(const Blaze::GameManager::CreateGameRequest &createGameRequest, const char8_t* templateName)
{
    Blaze::GameManager::CreateGameResponse createGameResponse;
    BlazeRpcError result;
    if((templateName != nullptr) && (strlen(templateName) > 0))
        result = internalCreateGameByTemplateName(createGameRequest, templateName, createGameResponse);
    else
        result = internalCreateGame(createGameRequest, createGameResponse);

    if(result == Blaze::ERR_OK)
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerInstance].actionCreateGame_DSCreator: (Non-Dynamic) DedicatedServer Game successfully created. GameId = " << createGameResponse.getGameId());
    }
    else
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager,  "[DedicatedServerInstance].actionCreateGame_DSCreator: Failed to create the (Non-Dynamic) ds game, with error " << Blaze::ErrorHelp::getErrorName(result));
    }
    return result;
}

/*! ************************************************************************************************/
/*! \brief Helper wrapper to createGame(). calls setsGameId() on returned GameId
***************************************************************************************************/
BlazeRpcError DedicatedServerInstance::internalCreateGame(const Blaze::GameManager::CreateGameRequest &createGameRequest,
                                                                 Blaze::GameManager::CreateGameResponse &createGameResponse)
{
    Blaze::GameManager::CreateGameRequest createRequest;
    createGameRequest.copyInto(createRequest);
    if (createRequest.getCommonGameData().getPlayerNetworkAddress().getActiveMember() == NetworkAddress::MEMBER_UNSET)
    {
        // need even a garbage network address here to avoid error on game create
        NetworkAddress *hostAddress = &createRequest.getCommonGameData().getPlayerNetworkAddress();
        hostAddress->switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
        hostAddress->getIpPairAddress()->getExternalAddress().setIp(getConnection()->getAddress().getIp());
        hostAddress->getIpPairAddress()->getExternalAddress().setPort(getConnection()->getAddress().getPort());
        hostAddress->getIpPairAddress()->getExternalAddress().copyInto(hostAddress->getIpPairAddress()->getInternalAddress());
    }
    BlazeRpcError result = mProxy->createGame(createRequest, createGameResponse);
    setGameId(createGameResponse.getGameId());
    return result;
}

BlazeRpcError DedicatedServerInstance::internalCreateGameByTemplateName(const Blaze::GameManager::CreateGameRequest &createGameRequest, const char8_t* templateName, 
    Blaze::GameManager::CreateGameResponse &createGameResponse)
{
    Blaze::GameManager::CreateGameRequest createRequest;
    createGameRequest.copyInto(createRequest);
    if (createRequest.getCommonGameData().getPlayerNetworkAddress().getActiveMember() == NetworkAddress::MEMBER_UNSET)
    {
        // need even a garbage network address here to avoid error on game create
        NetworkAddress *hostAddress = &createRequest.getCommonGameData().getPlayerNetworkAddress();
        hostAddress->switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
        hostAddress->getIpPairAddress()->getExternalAddress().setIp(getConnection()->getAddress().getIp());
        hostAddress->getIpPairAddress()->getExternalAddress().setPort(getConnection()->getAddress().getPort());
        hostAddress->getIpPairAddress()->getExternalAddress().copyInto(hostAddress->getIpPairAddress()->getInternalAddress());
    }

    JoinGameResponse response;
    Blaze::GameManager::CreateGameTemplateRequest request;
    createRequest.getCommonGameData().copyInto(request.getCommonGameData());
    createRequest.getPlayerJoinData().copyInto(request.getPlayerJoinData());
    request.setTemplateName(templateName);
    BlazeRpcError result = mProxy->createGameTemplate(request, response);
    setGameId(response.getGameId());
    return result;
}

/*! ************************************************************************************************/
/*! \brief finalizes the game on server. For topology host finalizeGameCreation, for platform host updateGameSession.
***************************************************************************************************/
void DedicatedServerInstance::finalizeGameCreation(GameId gameId, bool platformHost, bool topologyHost)
{
    StressLogContextOverride stressLogContextOverride(getIdent(), getBlazeId());

    UpdateGameSessionRequest updateGameSessionRequest;
    updateGameSessionRequest.setGameId( gameId );

    BlazeRpcError err = ERR_OK;

    if (topologyHost)
    {        
        err = mProxy->finalizeGameCreation(updateGameSessionRequest);
    }

    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager,  "[DedicatedServerInstance].finalizeGameCreation: Error " << err <<
            " calling finalizeGameCreation (platformHost=" << (platformHost ? 1 : 0) <<
            ") for game id " << gameId);
    }
}
//bsze_todo: STRESS_TODO: try consolidate with GameManagerUtil
/*! ************************************************************************************************/
/*! \brief sends blazeserver updateMeshConnection rpc to update other my connection to the topology host.
    Pre: this instance is a player in game (not the topology host).
***************************************************************************************************/
void DedicatedServerInstance::updateHostConnection(GameId gameId, ConnectionGroupId sourceGroupId, ConnectionGroupId topologyGroupId, PlayerNetConnectionStatus connStatus)
{
    StressLogContextOverride stressLogContextOverride(getIdent(), getBlazeId());

    UpdateMeshConnectionRequest req;
    req.setGameId( gameId );
    EA::TDF::ObjectId sourceObject;
    EA::TDF::ObjectId targetObject;
    sourceObject.type = ENTITY_TYPE_CONN_GROUP;
    targetObject.type = ENTITY_TYPE_CONN_GROUP;
    sourceObject.id = sourceGroupId;
    targetObject.id = topologyGroupId;

    req.setTargetGroupId(targetObject);
    req.setSourceGroupId(sourceObject);
    req.setPlayerNetConnectionStatus(connStatus);
    BlazeRpcError err = mProxy->updateMeshConnection(req);
    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager,  "[DedicatedServerInstance].updateHostConnection: Error " << err << " calling updateMeshConnection for game id " << gameId);
    }
}
//bsze_todo: STRESS_TODO: try consolidate with GameManagerUtil
/*! ************************************************************************************************/
/*! \brief sends blazeserver updateMeshConnection rpc to update other game player's connections to this instance.
    Pre: this instance is the topology host.
***************************************************************************************************/
void DedicatedServerInstance::updatePlayerConns(GameId gameId, UpdateMeshConnectionRequest req)
{
    req.setGameId(gameId);
    BlazeRpcError err = mProxy->updateMeshConnection(req);
    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[DedicatedServerInstance].updatePlayerConns: Error " << err << " updating player connection to group " << req.getTargetGroupId().id << " for game id " << mGameId);                
    }
}


BlazeId DedicatedServerInstance::getBlazeId() const
{
    return getLogin()->getUserLoginInfo()->getBlazeId();
}


/*! ************************************************************************************************/
/*! \brief validates the ping site latencies are available for the stress client.
    Should be called to check prior to trying dynamic DS reset rpcs
***************************************************************************************************/
bool DedicatedServerInstance::validatePingSiteLatenciesUpdated()
{
    if (mGotPingSiteLatencyUpdate)
    {
        return true;
    }

    Blaze::FetchExtendedDataRequest req;
    Blaze::UserSessionExtendedData rsp;
    req.setBlazeId(mBlazeId);
    getUserSessionsSlave()->fetchExtendedData(req, rsp);
    if (!rsp.getLatencyMap().empty())
    {
        mGotPingSiteLatencyUpdate = true;
    }
    return mGotPingSiteLatencyUpdate;
}

void DedicatedServerInstance::onQosSettingsUpdated()
{
    const char8_t* oldBestPingSite = mBestPingSiteAlias;
    mBestPingSiteAlias = getRandomPingSiteAlias();
    mGotPingSiteLatencyUpdate = false;
    if (oldBestPingSite == nullptr || mBestPingSiteAlias == nullptr)
        mBestPingSiteChanged = (oldBestPingSite != mBestPingSiteAlias);
    else
        mBestPingSiteChanged = (blaze_strcmp(oldBestPingSite, mBestPingSiteAlias) == 0);
}

} // Stress
} // Blaze

