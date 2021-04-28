/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GameManagerModule

    Stress tests GameManager component.
    ---------------------------------------
    TODO.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "gamemanagermodule.h"
#include "loginmanager.h"
#include "framework/connection/selector.h"
#include "framework/config/config_file.h"
#include "framework/config/config_boot_util.h"
#include "framework/config/config_sequence.h"
#include "framework/config/config_map.h"
#include "framework/util/random.h"
#include "blazerpcerrors.h"

#include "framework/util/shared/blazestring.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "framework/util/shared/rawbuffer.h"

#include "framework/config/configdecoder.h"

#include "gamemanager/tdf/gamemanagerconfig_server.h"
#ifdef TARGET_arson
#include "arson/tdf/arson.h"
#endif

namespace Blaze
{
namespace Stress
{

/*
 * List of available RPCs that can be called by the module.
 */
static const char8_t* GM_ACTION_STRINGS[] =  {
    "noop"
};

static const char8_t* GM_METRIC_STRINGS[] =  {
    "LegacyReportsSent",
    "LegacyReportsProc",
    "JoinByBrowser",
    "JoinByUser",
    "GaugeLists"
};

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** GameManagerModule Methods ******************************************************************************/

// static
StressModule* GameManagerModule::create()
{
    return BLAZE_NEW GameManagerModule();
}


GameManagerModule::GameManagerModule()
{
    mAction = ACTION_INVALID;
  
    mGameUtil = BLAZE_NEW Blaze::Stress::GameManagerUtil();
    mGamegroupsUtil = BLAZE_NEW Blaze::Stress::GamegroupsUtil();

    mTotalConns = 0;
    mRemainingCreatorCount = 0;
    mRemainingVirtualCreatorCount = 0;

    mHostMigrationRate = 0.0f;
    mKickPlayerRate = 0.0f;
    mSetAttributeRate = 0.0f;
    mSetPlayerAttributeRate = 0.0f;
    mBatchAttributeSetPreGame = false;
    mBatchAttributeSetPostGame = false;
    mSendLegacyGameReport = false;
    
    mJoinByUserRate = 0.0f;
    mJoinByBrowserRate = 0.0f;
    mBrowserGameResetRate = 0.0f;
    mRoleIdleLifespan = 0;
    mBrowserListLife = (uint32_t)-1;    
    mBrowserListSize = 0;
    mBrowserSubscriptionLists = false;
    mAdditionalUserSetSubscriptionLists = false;
    mGameSizeRuleMinFitThreshold[0] = '\0';

    mGamegroupJoinerFreq = 0;

    mLogBuffer[0] = '\0';

    for (int i = 0; i < NUM_METRICS; ++i)
    {
        mMetricCount[i] = 0;
    }
}


GameManagerModule::~GameManagerModule()
{
    CreateGameRequest *createGameRequest = mGameUtil->getCreateGameRequestOverride();
    if (createGameRequest != nullptr)
    {
        delete createGameRequest;
    }

    delete mGamegroupsUtil;
    delete mGameUtil;
}


//    diagnostics for utility not meant to measure load.
void GameManagerModule::addMetric(Metric metric)
{
    mMetricCount[metric]++;
}

void GameManagerModule::subMetric(Metric metric)
{
    mMetricCount[metric]--;
}


bool GameManagerModule::saveStats()
{
    mGameUtil->dumpStats(this);

    char8_t *metricOutput = mLogBuffer;
    metricOutput[0] = '\0';
    char8_t *curStr = metricOutput;

    for (int i = 0; i < NUM_METRICS; ++i)
    {
        blaze_snzprintf(curStr, sizeof(mLogBuffer) - strlen(metricOutput), "%s=%" PRIu64 " ",GM_METRIC_STRINGS[i], this->mMetricCount[i]);
        curStr += strlen(curStr);
    }

    BLAZE_INFO_LOG(Log::CONTROLLER, "[GameManagerModule].saveStats: " << metricOutput);


    return false;
}

//  GameManager Stress Test Configuration
//      Defines fields passed to GameManager RPCs invoked during a stress test.
//      Each Test uses a subset of the listed fields.
//
bool GameManagerModule::parseConfig(const ConfigMap& config, const ConfigBootUtil* bootUtil)
{    
    //  parse gamebrowser configuration for list configs.    
    GameBrowserServerConfig gameBrowserServerConfig;
    if (bootUtil!= nullptr)
    {
        const ConfigFile* gmConfigFile = bootUtil->createFrom("gamemanager", ConfigBootUtil::CONFIG_IS_COMPONENT);
        if (gmConfigFile != nullptr)
        {
            const ConfigMap* gmMap = gmConfigFile->getMap("gamemanager");
            if (gmMap != nullptr)
            {
                GameManagerServerConfig gameManagerServerConfig;
                ConfigDecoder configTDFDecoder(*gmMap);
                if (!configTDFDecoder.decode(&gameManagerServerConfig))
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerModule].parseConfig: Unable to decode config to TDF!");
                    return false;
                }
                else
                {
                    gameManagerServerConfig.getGameBrowser().copyInto(gameBrowserServerConfig);
                }
            }
            else
            {
                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerModule].parseConfig: Error gamemanager map not found in config file.");
                return false;
            }
        }
        else
        {
            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerModule].parseConfig: Error config file not found for gamemanager.");
            return false;
        }
    }
    else
    {
        const ConfigMap *gbConfigFile = ConfigFile::createFromFile("component/gamemanager/gamebrowser.cfg", true);
        if (gbConfigFile != nullptr)
        {
            const ConfigMap* gbMap = gbConfigFile->getMap("gamebrowser");
            if (gbMap != nullptr)
            {
                ConfigDecoder configTDFDecoder(*gbMap);
                if (!configTDFDecoder.decode(&gameBrowserServerConfig))
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerModule].parseConfig: Unable to decode config to TDF!");
                    return false;
                }               
            }  
            else
            {
                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerModule].parseConfig: Error gamebrowser map not found in config file.");
                return false;
            }
        }
        else
        {
            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerModule].parseConfig: Error config file not found for gamebrowser.");
            return false;
        }
    }

    setListConfigMap(gameBrowserServerConfig.getListConfig());


    //  general test settings
    mRoleIdleLifespan = config.getUInt32("gmmRoleIdleLifespan", 0);

     // Duration of a created list subscription
    mBrowserListLife = config.getUInt32("listLife", (uint32_t)-1);
    mBrowserListSize = config.getUInt32("listSize", 50);
    mBrowserSubscriptionLists = config.getBool("subscriptionLists", false);
    mAdditionalUserSetSubscriptionLists = config.getBool("additionalUserSetSubscriptionLists", false);

    const char *cfgstr = config.getString("gbrGameSizeRuleMinFit","matchAny");
    blaze_strnzcpy(mGameSizeRuleMinFitThreshold, cfgstr, sizeof(mGameSizeRuleMinFitThreshold));
 
    //  sets frequency of RPC calls for specified actions
    mHostMigrationRate = (float)config.getDouble("gmmHostMigrationRate", 0.0);
    mKickPlayerRate = (float)config.getDouble("gmmKickPlayerRate", 0.0);
    mSetAttributeRate = (float)config.getDouble("gmmSetAttributeRate", 0.0);
    mSetPlayerAttributeRate = (float)config.getDouble("gmmSetPlayerAttributeRate", 0.0);

    //  sets join method chance for instances that are non hosts.
    mJoinByUserRate = (float)config.getDouble("gmmJoinByUserRate", 0.0);
    mJoinByBrowserRate = (float)config.getDouble("gmmJoinByBrowserRate", 0.0);
    mBrowserGameResetRate = (float)config.getDouble("gmmBrowserListGameResetRate", 0.0);
    mBrowserListName = config.getString("gmmBrowserListName", "default");

    // game attributes
    const ConfigSequence *attrs = config.getSequence("gmmGameAttrKeys");
    const ConfigMap *valueSequences = config.getMap("gmmGameAttrValues");

    // game groups
    mGamegroupJoinerFreq = config.getUInt16("ggJoinerFreq", 0);

    if (attrs != nullptr && attrs->getSize() > 0)
    {
        //  setup game attributes
        for (size_t i = 0; i < attrs->getSize(); ++i)
        {   
            //  write out game attrvalue vector
            AttrValues::insert_return_type inserter = mGameAttrValues.insert(attrs->at(i));
            
            if (inserter.second)
            {
                const ConfigSequence *values = valueSequences->getSequence(inserter.first->first);
                if (values->getSize() > 0)
                {
                    uint32_t defaultWeight = 100 / (uint32_t)values->getSize();
                    for (size_t valueIdx = 0; valueIdx < values->getSize(); ++valueIdx)
                    {
                        char8_t *text = blaze_strdup(values->at(valueIdx));
                        char8_t *weight_token = blaze_strstr(text, ":");

                        AttributeValueWeight value;
                        if (weight_token != nullptr)
                        {
                            *weight_token = '\0';
                            ++weight_token;
                            value.second = (uint32_t)(atoi(weight_token));
                        }
                        else
                        {
                            value.second = defaultWeight;
                        }
                        value.first = text;
                        inserter.first->second.push_back(value);
                        BLAZE_FREE(text);
                    }
                }
            }
        }   
    }

    ///////////////////////////////////////////////////////////////////////////////

    attrs = config.getSequence("gmmPlayerAttrKeys");
    valueSequences = config.getMap("gmmPlayerAttrValues");

    if (attrs != nullptr && attrs->getSize() > 0)
    {
        //  setup game attributes
        for (size_t i = 0; i < attrs->getSize(); ++i)
        {   
            //  write out game attrvalue vector
            AttrValues::insert_return_type inserter = mPlayerAttrValues.insert(attrs->at(i));
            
            if (inserter.second)
            {
                const ConfigSequence *values = valueSequences->getSequence(inserter.first->first);
                if (values->getSize() > 0)
                {
                    uint32_t defaultWeight = 100 / (uint32_t)values->getSize();
                    for (size_t valueIdx = 0; valueIdx < values->getSize(); ++valueIdx)
                    {
                        char8_t *text = blaze_strdup(values->at(valueIdx));
                        char8_t *weight_token = blaze_strstr(text, ":");

                        AttributeValueWeight value;
                        if (weight_token != nullptr)
                        {
                            *weight_token = '\0';
                            ++weight_token;
                            value.second = (uint32_t)(atoi(weight_token));
                        }
                        else
                        {
                            value.second = defaultWeight;
                        }
                        value.first = text;
                        inserter.first->second.push_back(value);
                        BLAZE_FREE(text);
                    }
                }
            }       
        }   
    }

    mBatchAttributeSetPreGame = config.getBool("gmmBatchSetAttributesPreGame", false);
    mBatchAttributeSetPostGame = config.getBool("gmmBatchSetAttributesPostGame", false);

#ifdef TARGET_gamereporting
    //  3.x style game reports override the legacy reporting code - enforce that.
    mSendLegacyGameReport = false;
#endif

    if (mSendLegacyGameReport)
    {
        //  Parse GameReport Types.
        const ConfigSequence *reportTypeWeights = config.getSequence("gmmGameReportTypes");
       
        if (reportTypeWeights != nullptr && reportTypeWeights->getSize() > 0)
        {
            uint32_t defaultWeight = 100 / (uint32_t)reportTypeWeights->getSize();
            for (size_t valueIdx = 0; valueIdx < reportTypeWeights->getSize(); ++valueIdx)
            {
                //  memory for type key allocated here for the map (which takes a key of const char8_t*)
                char8_t *text = blaze_strdup(reportTypeWeights->at(valueIdx));
                char8_t *weight_token = blaze_strstr(text, ":");

                uint32_t weightValue = defaultWeight;
                if (weight_token != nullptr)
                {
                    *weight_token = '\0';
                    ++weight_token;
                    weightValue = (uint32_t)(atoi(weight_token));
                }

                mLegacyGameReportTypeWeights.insert(LegacyGameReportTypeWeights::value_type(text, weightValue));                                
            }
        }
    }

    return true;
}


const char8_t* GameManagerModule::pickGameAttrValue(const char8_t* attr) const
{
    AttrValues::const_iterator itAttrValues = mGameAttrValues.find(attr);
    if (itAttrValues == mGameAttrValues.end() || itAttrValues->second.size() == 0)
    {
        return nullptr;
    }

    //  determine which value to use based on the random weight selected compared to the weights on each value.
    uint32_t weight = (uint32_t)(rand() % 100);
    AttrValueVector::const_iterator itValVecEnd = itAttrValues->second.end();
    AttrValueVector::const_iterator itValVec = itAttrValues->second.begin();
    size_t valueindex = 0;
    uint32_t weightBase = 0;
    for (; itValVec != itValVecEnd; ++itValVec, ++valueindex)
    {
        AttributeValueWeight valueAndWeight = *itValVec;
        if (weight >= weightBase && weight < (weightBase+valueAndWeight.second))
        {
            break;
        }
        // calculate next low-threshold for next attribute compare using the high-threashold for this attribute
        weightBase += valueAndWeight.second;
    }
    //  Take care of cases where the weights don't add up to 100 in the config.   This usually means the last value is the selected one.
    if (valueindex >= itAttrValues->second.size())
    {
        valueindex = itAttrValues->second.size()-1;
    }

    return itAttrValues->second.at(valueindex).first;
}


const char8_t* GameManagerModule::pickPlayerAttrValue(const char8_t* attr) const
{
    AttrValues::const_iterator itAttrValues = mPlayerAttrValues.find(attr);
    if (itAttrValues == mPlayerAttrValues.end() || itAttrValues->second.size() == 0)
    {
        return nullptr;
    }

    //  determine which value to use based on the random weight selected compared to the weights on each value.
    uint32_t weight = (uint32_t)(rand() % 100);
    AttrValueVector::const_iterator itValVecEnd = itAttrValues->second.end();
    AttrValueVector::const_iterator itValVec = itAttrValues->second.begin();
    size_t valueindex = 0;
    uint32_t weightBase = 0;
    for (; itValVec != itValVecEnd; ++itValVec, ++valueindex)
    {
        AttributeValueWeight valueAndWeight = *itValVec;
        if (weight >= weightBase && weight < (weightBase+valueAndWeight.second))
        {
            break;
        }
        // calculate next low-threshold for next attribute compare using the high-threashold for this attribute
        weightBase += valueAndWeight.second;
    }
    //  Take care of cases where the weights don't add up to 100 in the config.   This usually means the last value is the selected one.
    if (valueindex >= itAttrValues->second.size())
    {
        valueindex = itAttrValues->second.size()-1;
    }
    
    return itAttrValues->second.at(valueindex).first;
}

bool GameManagerModule::initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil)
{
    if (!StressModule::initialize(config, bootUtil)) 
        return false;

    if (!mGameUtil->initialize(config, bootUtil))
        return false;

    if (!mGamegroupsUtil->initialize(config, bootUtil))
        return false;

    if (!parseConfig(config, bootUtil)) 
        return false;
    
    mInitTimestamp = TimeValue::getTimeOfDay();
    mRemainingCreatorCount = mGameUtil->getMaxGames();
    mRemainingVirtualCreatorCount = mGameUtil->getMaxVirtualCreators();
 
    //  parse actions.
    const char8_t* action = config.getString("action", "");
    if (blaze_stricmp(action, GM_ACTION_STRINGS[ACTION_NOOP]) == 0)
    {
        mAction = ACTION_NOOP;
    }       
    else
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerModule].initialize: Unrecognized action: '" << action << "'");
        return false;
    }    

    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerModule].initialize: " << action << " action selected.");
   
    return true;
}


// Called when the stress tester determines how many connections the module will use out of the total pool
void GameManagerModule::setConnectionDistribution(uint32_t conns)
{
    mTotalConns = conns;
    StressModule::setConnectionDistribution(conns);
}

StressInstance* GameManagerModule::createInstance(StressConnection* connection, Login* login)
{
    // modify behavior of the instance based on configuration.
    uint32_t numJoinByUsers = (uint32_t)(mTotalInstances * getJoinByUserRate());
    bool joinByUser = false;
    if ((uint32_t)connection->getIdent() < numJoinByUsers)
    {
        joinByUser = true;
    }

    GameManagerInstance *instance = BLAZE_NEW GameManagerInstance(this, connection, login, joinByUser);
    InstanceMap::insert_return_type inserter = mInstances.insert(instance->getIdent());
    if (inserter.first == mInstances.end())
    {
        delete instance;
        instance = nullptr;
    }
    else
    {        
        inserter.first->second = instance;
        
        if ((mGameUtil->getMaxVirtualCreators() == 0) && (mRemainingCreatorCount > 0))
        {
            instance->setRole(GameManagerUtil::ROLE_HOST);
            mRemainingCreatorCount--;
        }
        else if(mRemainingVirtualCreatorCount > 0)
        {
            instance->setRole(GameManagerUtil::ROLE_VIRTUAL_GAME_CREATOR);
            mRemainingVirtualCreatorCount--;
        }
        else
        {
            instance->setRole(GameManagerUtil::ROLE_NONE);
        }
    }
    return instance;
}


void GameManagerModule::releaseInstance(GameManagerInstance *instance)
{
    if (instance->getRole() == GameManagerUtil::ROLE_HOST)
        mRemainingCreatorCount++;
    else if( instance->getRole() == GameManagerUtil::ROLE_VIRTUAL_GAME_CREATOR)
        mRemainingVirtualCreatorCount++;

    mInstances.erase(instance->getIdent());
}


GameManagerInstance::GameManagerInstance(GameManagerModule *owner, StressConnection* connection, Login* login, bool joinByUser) :
    StressInstance(owner, connection, login, BlazeRpcLog::gamemanager),
    mOwner(owner),
    mProxy(BLAZE_NEW GameManagerSlave(*getConnection())),
#ifdef TARGET_arson
    mArsonProxy(BLAZE_NEW Arson::ArsonSlave(*getConnection())),
#endif
#ifdef TARGET_util
    mUtilSlave(BLAZE_NEW Util::UtilSlave(*getConnection())),
#endif
    mTrialIndex(0),
    mGameInst(owner->getGameManagerUtil()),
    mGamegroupInst(owner->getGamegroupsUtil()),
    mStateCounter(0),
    mBrowserListLifeCounter(0),
    mBrowserListId(0),
    mUserSetBrowserListId(0)
{
    mGameInst.setStressInstance(this);
    mGameInst.setJoinByUser(joinByUser);

    mGamegroupInst.setStressInstance(this);

    if (mOwner->getGamegroupJoinerFreq() > 0)
    {
        if (Blaze::Random::getRandomNumber(100) < mOwner->getGamegroupJoinerFreq())
            mGamegroupInst.setRole(GamegroupsUtilInstance::ROLE_JOINER);
        else
            mGamegroupInst.setRole(GamegroupsUtilInstance::ROLE_LEADER);
    }

    mGameInst.setNotificationCb(GameManagerSlave::NOTIFY_NOTIFYGAMESTATECHANGE, GameManagerUtilInstance::AsyncNotifyCb(this, &GameManagerInstance::onGameStateChange));

    mGameInst.setCreateGameRequestCallback(GameManagerUtilInstance::CreateGameRequestCb(this, &GameManagerInstance::createGameRequestOverrideCb));

    mGameInst.setPostGameMode(GameManagerUtilInstance::POST_GAME_MODE_CUSTOM);

    connection->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMELISTUPDATE, this);
}


GameManagerInstance::~GameManagerInstance()
{
    getConnection()->removeAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMELISTUPDATE, this);

    //  clear out game map
    mGameMap.clear();


    mGameInst.clearNotificationCb(GameManagerSlave::NOTIFY_NOTIFYPLAYERREMOVED);
    mGamegroupInst.setStressInstance(nullptr);
    mGameInst.setStressInstance(nullptr);
#ifdef TARGET_util
    delete mUtilSlave;
#endif
    delete mProxy;
#ifdef TARGET_arson
    delete mArsonProxy;
#endif

    mOwner->releaseInstance(this);
    
}


void GameManagerInstance::onDisconnected()
{
    mGameInst.onLogout();
    mGamegroupInst.onLogout();
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerInstance].onDisconnected: GameManagerInstance disconnected!");
}


BlazeId GameManagerInstance::getBlazeId() const 
{
    return getLogin()->getUserLoginInfo()->getBlazeId();
}


const char8_t *GameManagerInstance::getName() const
{
    return GM_ACTION_STRINGS[mOwner->getAction()];
}


void GameManagerInstance::fillGameAttributeMap(Collections::AttributeMap &attrMap)
{
    const GameManagerModule::AttrValues& attrs = mOwner->getGameAttrValues();
    if (attrs.size() > 0)
    {
        for (GameManagerModule::AttrValues::const_iterator it = attrs.begin(); it != attrs.end(); ++it)
        {
            Collections::AttributeMap::value_type val;
            val.first.set(it->first);
            val.second.set(mOwner->pickGameAttrValue(it->first));
            attrMap.insert(val); 
        }
    }
}


void GameManagerInstance::fillPlayerAttributeMap(Collections::AttributeMap &attrMap)
{
    const GameManagerModule::AttrValues& attrs = mOwner->getPlayerAttrValues();
    if (attrs.size() > 0)
    {
        for (GameManagerModule::AttrValues::const_iterator it = attrs.begin(); it != attrs.end(); ++it)
        {
            Collections::AttributeMap::value_type val;
            val.first.set(it->first);
            val.second.set(mOwner->pickPlayerAttrValue(it->first));
            attrMap.insert(val); 
        }
    }
}

                       
void GameManagerInstance::createGameRequestOverrideCb(CreateGameRequest *req)
{
    //  modify attributes.
    fillGameAttributeMap(req->getGameCreationData().getGameAttribs());
    
    if (req->getGameCreationData().getNetworkTopology() != CLIENT_SERVER_DEDICATED)
    {
        //  host is a player, set player attributes.
        //  modify attributes.
        PerPlayerJoinData* data = req->getPlayerJoinData().getPlayerDataList().pull_back();
        fillPlayerAttributeMap(data->getPlayerAttributes());
    }
}


////////////////////////////////////////////////////////////////////////////////////////

void GameManagerInstance::start()
{
    StressInstance::start();
}


BlazeRpcError GameManagerInstance::execute()
{
    BlazeRpcError rpcResult = ERR_OK;

    mTrialIndex++;
    bool initialGmExec = true;
    bool initialGgExec = true;
    
    // index at which we start scanning GB list to get the game we want to join
    int pickGameListScanStartIndex = 0;
            
    while (true)
    {
        //  sync local instance's game Id with the game manager util copy as the utility is responsible for keeping track of what game its in. 
        GameId localGameId = mGameInst.getGameId();
        const GameData *gameData = mGameInst.getGameData();
        
        //  roll the dice.
        float rollAction = (rand() % 1000)/1000.0f;

        ///////////////////////////////////////////////////////////////////////////
        //    HOST BASED ACTIONS
        if (mGameInst.getRole() == GameManagerUtil::ROLE_HOST)
        {
            if (gameData != nullptr)
            {
                float kickActionThreshold = mOwner->getKickPlayerRate();
                float hostMigrationThreshold = kickActionThreshold + mOwner->getHostMigrationRate();
                float setAttrThreshold =  mOwner->getSetAttributeRate();
                float setPlayerAttrThreshold = mOwner->getSetPlayerAttributeRate();
                PlayerId hostPlayerId = gameData->getHostId();
                GameId myGameId = gameData->getGameId();

                if (rollAction < kickActionThreshold)
                {
                    //    pick a player other than the leader from the roster to kick.
                    size_t numPlayers = gameData->getNumPlayers();
                    if (numPlayers > 1)
                    {
                        for (size_t playerIndex = 0; playerIndex < numPlayers; ++playerIndex)
                        {
                            PlayerData* player = gameData->getPlayerDataByIndex(playerIndex);
                            //    TODO: randomize this?                          
                            EA_ASSERT(player != nullptr);
                            if (hostPlayerId != player->mPlayerId)
                            {
                                RemovePlayerRequest req;
                                req.setGameId(myGameId);
                                req.setPlayerId(player->mPlayerId);
                                req.setPlayerRemovedReason(PLAYER_KICKED);
                                rpcResult = mProxy->removePlayer(req);
                                if (rpcResult != ERR_OK)
                                {
                                    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerInstance].execute: Failed to kick player from gameid=" << myGameId << " " << (ErrorHelp::getErrorName(rpcResult)) << ".");
                                }
                                break;
                            }
                        }
                    }
                }
                else if (rollAction < hostMigrationThreshold)
                {
                    // pick someone other than the host to migrate to.
                    if (gameData->getGameSettings()->getHostMigratable() && gameData->getNumJoinCompleted() > 0)
                    {
                        const PlayerData *playerData = gameData->getPlayerDataByIndex(size_t(rand()) % gameData->getNumPlayers());
                        if (playerData->mJoinCompleted)
                        {
                            //  set the current host's role to none to prevent util changes to this user's state as the migration occurs.
                            setRole(GameManagerUtil::ROLE_JOINER);

                            MigrateHostRequest req;
                            req.setGameId(myGameId);
                            req.setNewHostPlayer(playerData->mPlayerId);
                            rpcResult = mProxy->migrateGame(req);
                            if (rpcResult != ERR_OK)
                            {
                                BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerInstance].execute: Failed to migrate playerid=" 
                                               << playerData->mPlayerId << " from gameid=" << myGameId << " to new host");
                                setRole(GameManagerUtil::ROLE_HOST);     // migration failed, reset this user to a host role.
                            }
                        }
                    }
                }
                else if (rollAction < setAttrThreshold)
                {
                    // pick an attribute to modify.
                    const Collections::AttributeMap* attrMap = gameData->getAttributeMap();
                    if (attrMap->size() > 0)
                    {                    
                        SetGameAttributesRequest req;
                        req.setGameId(myGameId);
                        fillGameAttributeMap(req.getGameAttributes());
                        if(gameData->getGameSettings()->getVirtualized())
                        {
#ifdef TARGET_arson
                            Arson::SetGameAttributesRequest arsonReq;
                            req.copyInto(arsonReq.getSetGameAttributesRequest());
                            rpcResult = mArsonProxy->setGameAttributes(arsonReq);
#endif
                        }
                        else
                        {
                            rpcResult = mProxy->setGameAttributes(req);
                        }
                        
                        if (rpcResult != ERR_OK)
                        {
                            BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerInstance].execute: Failed to set attribute for gameid=" << myGameId << ".");
                        }
                    }
                }
                else if (gameData->getNumPlayers() > 0 && rollAction < setPlayerAttrThreshold)
                {
                    const PlayerData *playerData = gameData->getPlayerDataByIndex(size_t(rand()) % gameData->getNumPlayers());
                    if (playerData->mJoinCompleted)
                    {
                        const Collections::AttributeMap *attrs = playerData->getAttributeMap();
                        if (attrs->size() > 0)
                        {                    
                            SetPlayerAttributesRequest req;
                            req.setGameId(myGameId);
                            req.setPlayerId(playerData->mPlayerId);
                            fillPlayerAttributeMap(req.getPlayerAttributes());
                            rpcResult = mProxy->setPlayerAttributes(req);
                            if (rpcResult != ERR_OK)
                            {
                                BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerInstance].execute: Failed to set attribute for playerid=" << playerData->mPlayerId << " in gameid=" << myGameId << ".");
                            }
                        }
                    }
                }
            }
        }
        else if (mGameInst.getRole() == GameManagerUtil::ROLE_NONE)
        {
            ///////////////////////////////////////////////////////////////////////////
            //  These instances are not managed by the GameManagerUtilInstance class.
            GameManagerUtil *gmUtil = mOwner->getGameManagerUtil();

            //  update timers
            if (mBrowserListLifeCounter > 0)
            {
                mBrowserListLifeCounter--;
            }
            //  update idle timer - during idle 
            if (mStateCounter > 0)
            {
                mStateCounter--;
            }

            if (localGameId == 0 && mStateCounter == 0)
            {
                float joinByUserRate = mOwner->getJoinByUserRate();
                float joinByBrowserRate = joinByUserRate + mOwner->getJoinByBrowserRate();

                JoinGameResponse joinResp;
                BlazeRpcError err = ERR_OK;

                GameId pickedGameId = 0;
                GameId joinedGameId = 0;

                //  modify join request for this instance (okay to create on stack since operation is executed within this fiber.)
                JoinGameRequest joinReq;
                gmUtil->defaultJoinGameRequest(joinReq);

                //  modify attributes.
                const GameManagerModule::AttrValues& attrs = mOwner->getPlayerAttrValues();
                if (attrs.size() > 0)
                {
                    PerPlayerJoinData* playerData = joinReq.getPlayerJoinData().getPlayerDataList().pull_back();
                    // playerData->getUser().setBlazeId();

                    Collections::AttributeMap &attrMap = playerData->getPlayerAttributes();
                    for (GameManagerModule::AttrValues::const_iterator it = attrs.begin(); it != attrs.end(); ++it)
                    {
                        attrMap[it->first] = mOwner->pickPlayerAttrValue(it->first);
                    }
                }
          
                           
                //  join by user unless there's an active browser list for this instance.            
                if (mBrowserListId == 0 && rollAction < joinByUserRate)
                {                
                    //  pick a game then a user in that game to join.
                    pickedGameId = gmUtil->pickGame();
                    if (pickedGameId != 0)
                    {
                        //  look up game information as if a client might to display info to the incoming player.
                        joinedGameId = gmUtil->joinGame(&err, mProxy, &joinResp, this, true, &joinReq);
                        if (joinedGameId != 0)
                        {
                            mOwner->addMetric(GameManagerModule::METRIC_JOIN_BY_USER);
                        }
                    }
                }
                
                //  if there's a browser list , use that to join
                if (pickedGameId == 0 && (mBrowserListId != 0 || (rollAction >= joinByUserRate && rollAction < joinByBrowserRate)))
                {
                    if (mBrowserListId == 0)
                    {
                        //  create list for browsing on the next simulation cycle.
                        rpcResult = createGameBrowserList();
                    }
                    else if (mBrowserListId != 0)
                    {
                        //  pick a game within the browser list to join.
                        pickedGameId = 0;

                        size_t numResetable = 0;                    
                        GameBrowserGameMap::const_iterator citEnd = mGameMap.end();
                        
                        GameBrowserGameMap::iterator cit = mGameMap.begin();

                        if (!mGameMap.empty())
                            eastl::advance(cit, pickGameListScanStartIndex % mGameMap.size());

                        for (; cit != citEnd; ++cit)
                        {
                            const GameBrowserGameData *gbgame = cit->second;
                            if (gbgame->getGameState() == RESETABLE)
                            {
                                numResetable++;
                            }
                            if (gbgame->getGameState() == PRE_GAME || ((gbgame->getGameState() == IN_GAME || gbgame->getGameState() == POST_GAME) && mOwner->getGameManagerUtil()->getJoinInProgress()) )
                            {
                                //  TODO: why not let the GameBrowser list updates take care of filtering games with available players?
                                //  TODO: figure out how to do this with matchmaking criteria... currently it's a bit unclear how to do this right.
                                if (checkGameBrowserGameCapacity(SLOT_PUBLIC_PARTICIPANT, gbgame))
                                {
                                    pickedGameId = cit->first;
                                    break;
                                }
                            }
                            ++pickGameListScanStartIndex;
                        }
                        
                        if (mGameMap.size() > 0)
                        {
                            BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerInstance].execute: Games=" << mGameMap.size());                   
                        }
                                
                        bool tryReset = false;
                        if (pickedGameId != 0)
                        {  
                            joinedGameId = gmUtil->joinGame(&err, mProxy, pickedGameId, &joinResp, false, &joinReq);
                            
                            if (err == Blaze::ERR_OK)
                            {
                                BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerInstance].execute: Successfully joined game (id=" << pickedGameId << ").");
                                destroyGameBrowserList(); // don't need the list
                            }
                            if (err == GAMEMANAGER_ERR_PARTICIPANT_SLOTS_FULL || err == GAMEMANAGER_ERR_SPECTATOR_SLOTS_FULL)
                            {
                                //  IGNORE since yes it's possible a user chose a game that wasn't full only to be full because of other players joining at the same time.
                                tryReset = true;
                                ++pickGameListScanStartIndex; // try next game
                            }
                            else if (joinedGameId == 0 && err != ERR_OK)
                            {
                                if (err == GAMEMANAGER_ERR_INVALID_GAME_ID)
                                {
                                    mBrowserListLifeCounter = 0;         // clear current list since it's out of date?
                                }
                                BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerInstance].execute: Erroneous game join failure (id=" << pickedGameId << ") err=" << (ErrorHelp::getErrorName(err)) << "?");
                            }
                        }
                        else
                        {
                            tryReset = true;
                        }
                                                               
                        //  randomize attempt to reset if no games are available.
                        float resetRoll = (rand() % 1000) / 1000.0f;
                        if (tryReset && mOwner->getGameManagerUtil()->getGameTopology() == CLIENT_SERVER_DEDICATED && (resetRoll < mOwner->getBrowserListGameResetRate()))
                        {    
                            CreateGameRequest createReq;
                            mOwner->getGameManagerUtil()->defaultResetGameRequest(this, createReq);
                            fillGameAttributeMap(createReq.getGameCreationData().getGameAttribs());
                            if (createReq.getCommonGameData().getPlayerNetworkAddress().getActiveMember() == NetworkAddress::MEMBER_UNSET)
                            {
                                // need even a garbage network address here to avoid error on game create
                                NetworkAddress *hostAddress = &createReq.getCommonGameData().getPlayerNetworkAddress();
                                hostAddress->switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
                            }
                            err = mProxy->resetDedicatedServer(createReq, joinResp);
                            if (err == ERR_OK)
                            {
                                joinedGameId = joinResp.getGameId();
                                BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerInstance].execute: Resetting server game id = " << joinedGameId);
                                pickedGameId = 0;
                            }
                        }
                        if (joinedGameId != 0)
                        {
                            mOwner->addMetric(GameManagerModule::METRIC_BROWSER_SELECTIONS);
                            mBrowserListLifeCounter = 0;         // wipe out list since user is in game
                        }
                    }
                }
                
                if (joinedGameId != 0)
                {
                    //  start leave timer for the player who's joined a game.
                    mStateCounter = gmUtil->getGamePlayerLifespan() - (gmUtil->getGamePlayerLifespanDeviation()>0 ? (gmUtil->getGamePlayerLifespanDeviation()/2 + (((size_t)rand()) % gmUtil->getGamePlayerLifespanDeviation())) : 0);
                }
                else
                {
                    // try again later
                    mStateCounter = Blaze::Random::getRandomNumber(mOwner->getRoleIdleLifespan()) + 1;
                }
            }
            else if (mStateCounter == 0)
            {
                BlazeRpcError err = ERR_OK;
                localGameId = mGameInst.getGameId();
                if (localGameId != 0)
                {
                    //  player should still be in the game at this time. issue a leave.
                    gmUtil->removePlayer(&err, mProxy, localGameId, getBlazeId(), PLAYER_LEFT);
                    if (err != ERR_OK)
                    {
                        //  assumption is game was destroyed - but log the error in any case.
                        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerInstance].execute: RemovePlayer failed (err=" << (ErrorHelp::getErrorName(err)) << ")");
                    }
                }
            }

            if (mBrowserListLifeCounter == 0)
            {
                if (mBrowserListId != 0)
                {
                    rpcResult = Blaze::ERR_OK;
                    
                    if (mOwner->getBrowserSubscriptonLists())
                        rpcResult = destroyGameBrowserList();

                    if (rpcResult == Blaze::ERR_OK)
                        mBrowserListId = 0;
                }
            }     

        }

        if (initialGmExec ||
            mGameInst.getCyclesLeft() != 0 ||
            (mGameInst.getRole() == GameManagerUtil::ROLE_HOST && mGameInst.getPostGameCountdown() != 0) ||
            (mGameInst.getRole() == GameManagerUtil::ROLE_NONE && mStateCounter > 0))
        {
            // we don't need to pump for ROLE_NONE as this module controls it using mStateCounter
            // we need to keep pumping the ROLE_HOST until the post game countdown reaches 0
            initialGmExec = false;
            rpcResult = mGameInst.execute();
            if (rpcResult != ERR_OK)
                return rpcResult;
        }
        else
        {
            rpcResult = mGameInst.execute();
            if (rpcResult != ERR_OK)
                return rpcResult;
        }

        if (initialGgExec ||
            mGamegroupInst.getCyclesLeft() != 0)
        {
            initialGgExec = false;
            rpcResult = mGamegroupInst.execute();
            if (rpcResult != ERR_OK)
                return rpcResult;
        }
        else
        {
            rpcResult = mGamegroupInst.execute();
            if (rpcResult != ERR_OK)
                return rpcResult;
        }

        // Delay further actions
        BlazeRpcError err = gSelector->sleep(mOwner->getDelay() * 1000);
        if (err != ERR_OK)
        {
            BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameManagerInstance].execute: Failed to sleep.");
        }

    }

    return rpcResult;
}


bool GameManagerInstance::checkGameBrowserGameCapacity(SlotType slotType, const GameBrowserGameData *gbgame) const
{
    uint16_t playerCap = gbgame->getSlotCapacities().at(slotType);
    uint16_t playerCount = gbgame->getPlayerCounts().at(slotType);
    return (playerCount < playerCap);
}


void GameManagerInstance::setOpenMode(const GameData& gameData, bool open)
{
    GameSettings settings;
    settings.setBits(gameData.getGameSettings()->getBits());

    if (open)
    {
        settings.setOpenToMatchmaking();
        settings.setOpenToBrowsing();
        settings.setOpenToJoinByPlayer();
        settings.setOpenToInvites();
    }
    else
    {
        settings.clearOpenToMatchmaking();
        settings.clearOpenToBrowsing();
        settings.clearOpenToJoinByPlayer();
        settings.clearOpenToInvites();
    }
    BlazeRpcError err = ERR_OK;
    mOwner->getGameManagerUtil()->setGameSettings(&err, this->mProxy, gameData.getGameId(), &settings);
}


BlazeRpcError GameManagerInstance::createGameBrowserList()
{
    freeGameMap();

    BlazeRpcError rpcResult = ERR_OK;
    GetGameListRequest request;
    GetGameListResponse response;
    MatchmakingCriteriaError error;

    // Choose a list config
    const GameBrowserListConfigMap& configs =  mOwner->getListConfigMap();
    if (configs.size() == 0)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerInstance].createGameBrowserList: No configs loaded?");
        return ERR_SYSTEM;
    }

    request.setListConfigName(mOwner->getBrowserListName()); 
    // List capacity
    request.setListCapacity(mOwner->getBrowserListSize());
    request.getCommonGameData().setGameProtocolVersionString(mOwner->getGameManagerUtil()->getGameProtocolVersionString());

    request.setIgnoreGameEntryCriteria(false);

    //  TODO: integrate matchmaking config to use the correct attribute values based on rule attributes.
    if (!request.getIgnoreGameEntryCriteria())
    {
        // List criteria
        MatchmakingCriteriaData &criteria = request.getListCriteria();
        criteria.getPlayerCountRuleCriteria().setMaxPlayerCount(mOwner->getGameManagerUtil()->getMaxPlayerCapacity()); 
        criteria.getTotalPlayerSlotsRuleCriteria().setMaxTotalPlayerSlots(mOwner->getGameManagerUtil()->getMaxPlayerCapacity());
        uint16_t desired = mOwner->getGameManagerUtil()->getGamePlayerMinRequired(); // Desire the min players required for game to be in progress.
        criteria.getPlayerCountRuleCriteria().setDesiredPlayerCount(desired);
        criteria.getPlayerCountRuleCriteria().setMinPlayerCount(1);// game must have at least one player in it 
        criteria.getPlayerCountRuleCriteria().setRangeOffsetListName(mOwner->getGameSizeRuleMinFitThreshold());

        if (!mOwner->getGameAttrValues().empty())
        {
            for (AttrValues::const_iterator it = mOwner->getGameAttrValues().begin(); it != mOwner->getGameAttrValues().end(); ++it)
            {
                eastl::string ruleName(it->first);
                ruleName.append("Rule");
                GameManager::GameAttributeRuleCriteria* gameRuleCriteria = criteria.getGameAttributeRuleCriteriaMap()[ruleName.c_str()];
                if (gameRuleCriteria == nullptr)
                {
                    gameRuleCriteria = criteria.getGameAttributeRuleCriteriaMap()[ruleName.c_str()] = criteria.getGameAttributeRuleCriteriaMap().allocate_element();
                }
                else
                {
                    BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameManagerInstance].createGameBrowserList: Multiple attributes assigned to rule " 
                       << ruleName.c_str() << ".  This will lead to a problem if the rule is ARBITRARY. (skipping 2nd set)");
                    continue;
                }

                gameRuleCriteria->setMinFitThresholdName("requireExactMatch");
                gameRuleCriteria->getDesiredValues().push_back(it->second.at(rand() % it->second.size()).first);
            }
        }
    }
    

    // Create the list
    if (mOwner->getBrowserSubscriptonLists())
    {
        rpcResult = mProxy->getGameListSubscription(request, response, error);
    }
    else
    {
        rpcResult = mProxy->getGameListSnapshot(request, response, error);
    }

    if (rpcResult != ERR_OK)
    {
        //  failed to create, release this creator.
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameManagerInstance].createGameBrowserList: Unable to create game browser list. Error(" 
                       << (Blaze::ErrorHelp::getErrorName(rpcResult)) << ") " << error.getErrMessage() << ".");
        return rpcResult;
    }
 
    mBrowserListLifeCounter = mOwner->getBrowserListLifespan();

    if (mOwner->getBrowserSubscriptonLists())
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerInstance].createGameBrowserList: Created subscription game browser list(" << response.getListId() << ") ");
    }
    else
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerInstance].createGameBrowserList: Created snapshot game browser list(" << response.getListId() << ")"
            " which returned " << response.getNumberOfGamesToBeDownloaded() << " games." );
    }

    mBrowserListId = response.getListId();
    
    if (mOwner->getAdditionalUserSetSubscriptionLists() && mGamegroupInst.getGamegroupId() != INVALID_GAME_ID)
    {
        // create additional user set subscription list based on game group
        GetUserSetGameListSubscriptionRequest req;
        req.setUserSetId(EA::TDF::ObjectId(ENTITY_TYPE_GAME_GROUP, (Blaze::EntityId)mGamegroupInst.getGamegroupId()));
        rpcResult = mProxy->getUserSetGameListSubscription(req, response, error);
        if (rpcResult != ERR_OK)
        {
            //  failed to create, release this creator.
            BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameManagerInstance].createGameBrowserList: Unable to create user set game browser list. Error(" 
                           << (Blaze::ErrorHelp::getErrorName(rpcResult)) << ") " << error.getErrMessage() << ".");
        }
        else
        {
            mUserSetBrowserListId = response.getListId();
            BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerInstance].createGameBrowserList: Created user set subscription game browser list(" << response.getListId() << ") ");
        }
    }

    mOwner->addMetric(GameManagerModule::METRIC_BROWSER_LISTS);

    return rpcResult;
}


BlazeRpcError GameManagerInstance::destroyGameBrowserList()
{
    if (!mOwner->getBrowserSubscriptonLists())
        return Blaze::ERR_OK;

    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerInstance].destroyGameBrowserList: Destroying game browser list(" << mBrowserListId << ")");

    BlazeRpcError rpcResult = ERR_OK;
    DestroyGameListRequest request;

    // list id
    request.setListId(mBrowserListId);

    rpcResult = mProxy->destroyGameList(request);

    if (rpcResult != ERR_OK)
    {
        //  failed to create, release this creator.
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameManagerInstance].destroyGameBrowserList: Unable to destroy game browser list(" 
                       << mBrowserListId << "). Error(" << (Blaze::ErrorHelp::getErrorName(rpcResult)) << ") " 
                       << (Blaze::ErrorHelp::getErrorDescription(rpcResult)));
    }
    else
    {
        mOwner->subMetric(GameManagerModule::METRIC_BROWSER_LISTS);        
    }

    if (mUserSetBrowserListId != 0)
    {
        request.setListId(mUserSetBrowserListId);
        rpcResult = mProxy->destroyGameList(request);
        if (rpcResult != Blaze::ERR_OK)
        {
            BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameManagerInstance].destroyGameBrowserList: Unable to destroy user set game browser list(" 
                           << mUserSetBrowserListId << "). Error(" << (Blaze::ErrorHelp::getErrorName(rpcResult)) << ") " 
                           << (Blaze::ErrorHelp::getErrorDescription(rpcResult)));
        }
    }

   
    freeGameMap();    
   
    return rpcResult;
}


void GameManagerInstance::freeGameMap()
{
    // clear local tracking
    mBrowserListId = 0;
    mUserSetBrowserListId = 0;

    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerInstance].freeGameMap: Deleting " << mGameMap.size() << " games");
    mGameMap.clear();
}


BlazeRpcError GameManagerInstance::getGameDataFromId(GameId gameId)
{
    BlazeRpcError rpcResult = ERR_OK;
    GetGameDataFromIdRequest request;
    GameBrowserDataList response;
 
    // TODO: choose a list config through some other means
    request.setListConfigName("default");
    request.getGameIds().push_back(gameId);

    rpcResult = mProxy->getGameDataFromId(request, response);

    if (rpcResult != ERR_OK)
    {
        //  failed to create, release this creator.
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameManagerInstance].getGameDataFromId: Unable to get game data.  Error(" 
                       << (Blaze::ErrorHelp::getErrorName(rpcResult)) << ") " << (Blaze::ErrorHelp::getErrorDescription(rpcResult)));
    }
    return rpcResult;
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////

void GameManagerInstance::onLogout(BlazeRpcError result)
{
    mStateCounter = 0;
    mBrowserListLifeCounter = 0;
    mBrowserListId = 0;

    freeGameMap();

    mGamegroupInst.onLogout();
    mGameInst.onLogout();

    StressInstance::onLogout(result);
}

void GameManagerInstance::onLogin(BlazeRpcError result)
{
    // update network info - note that if UseServerQosSettings is enabled, updateNetworkInfo() will be called by the base StressInstance on login
    if (!getOwner()->getUseServerQosSettings())
        updateNetworkInfo();
}

void GameManagerInstance::onAsync(uint16_t component, uint16_t type, RawBuffer* payload)
{
    Heat2Decoder decoder;
    
    if (component == GameManagerSlave::COMPONENT_ID)
    {
        GameManager::NotifyGameListUpdate notifyListUpdate;

        switch (type)
        {
        case GameManagerSlave::NOTIFY_NOTIFYGAMELISTUPDATE:
            // We have received the list information
            if ((decoder.decode(*payload, notifyListUpdate) == ERR_OK) && 
                (notifyListUpdate.getListId() == mBrowserListId))
            {
            //    mOwner->addMetric(GameBrowserModule::NOTIFICATIONS);
                GameManager::GameIdList &removalList = notifyListUpdate.getRemovedGameList();
                GameManager::GameBrowserMatchDataList &updateList = notifyListUpdate.getUpdatedGames();

                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager,"[GameManagerInstance].onAsync: Notify game list(" << mBrowserListId 
                                << ") update.  Removals(" << removalList.size() << ") Updates(" << updateList.size() << ")");

                //removals
                GameIdList::const_iterator removeIter = removalList.begin();
                GameIdList::const_iterator removeEnd = removalList.end();
                for ( ; removeIter != removeEnd; ++removeIter)
                {
              //      mOwner->addMetric(GameBrowserModule::NOTIFY_REMOVALS);
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager,"[GameManagerInstance].onAsync: Notify game list(" << mBrowserListId 
                                    << ") update.  Removing game (" << *removeIter << ")");
                    GameBrowserGameMap::iterator gameMapIter = mGameMap.find(*removeIter);
                    if (gameMapIter != mGameMap.end())
                    {
                        // remove game from game map
                        mGameMap.erase(gameMapIter);
                    }
                }

                // add/updates
                GameManager::GameBrowserMatchDataList::const_iterator iter = updateList.begin();
                GameManager::GameBrowserMatchDataList::const_iterator end = updateList.end();
                for ( ; iter != end; ++iter)
                {
                    //    mOwner->addMetric(GameBrowserModule::NOTIFY_UPDATES);
                    const GameBrowserGameData& game = (*iter)->getGameData();
                    mGameMap[game.getGameId()] = game.clone();
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager,"[GameManagerInstance].onAsync: Notify game list(" << mBrowserListId 
                                    << ") update.  Adding game (" << game.getGameId() << ")");
                }
            }
            break;
        }
    }
}



void GameManagerInstance::onGameStateChange(EA::TDF::Tdf *tdf)
{
    GameManager::NotifyGameStateChange *stateChange = static_cast<NotifyGameStateChange *>(tdf);

    // Ignore state change notifications for game groups
    if (stateChange->getGameId() == mGamegroupInst.getGamegroupId())
        return;

    GameId localGameId = mGameInst.getGameId();
    if(localGameId != stateChange->getGameId())
    {
        if (localGameId != 0)
        {
            BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerInstance].onGameStateChange: Got unexpected state notification for game " << stateChange->getGameId()
                << "; local game is " << localGameId);
        }
        else
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerInstance].onGameStateChange: Game " << stateChange->getGameId() 
                            << " freed locally before receiving state notification");
        }
        return;
    }

    const GameData *gameData = mGameInst.getGameData();
    BlazeRpcError rpcResult = ERR_OK;
    switch(stateChange->getNewGameState())
    {
    case GameManager::PRE_GAME:
        if (gameData != nullptr && gameData->getHostId() == this->getBlazeId())
        {
            if (mOwner->getBatchAttributeSetPreGame())
            {
                //  batch attribute set
                SetGameAttributesRequest req;
                fillGameAttributeMap(req.getGameAttributes());
                req.setGameId(gameData->getGameId());
                if(gameData->getGameSettings()->getVirtualized())
                {
#ifdef TARGET_arson
                    Arson::SetGameAttributesRequest arsonReq;
                    req.copyInto(arsonReq.getSetGameAttributesRequest());
                    rpcResult = mArsonProxy->setGameAttributes(arsonReq);
#endif
                }
                else
                {
                    rpcResult = mProxy->setGameAttributes(req);
                }
                
                gameData = mGameInst.getGameData();
                if (gameData != nullptr && mOwner->getPlayerAttrValues().size() > 0)
                {
                    //  set all player attributes
                    for (size_t i = 0; i < gameData->getNumPlayers(); ++i)
                    {
                        PlayerData *player = gameData->getPlayerDataByIndex(i);
                        SetPlayerAttributesRequest request;
                        request.setGameId(gameData->getGameId());
                        request.setPlayerId(player->mPlayerId);
                        fillPlayerAttributeMap(request.getPlayerAttributes());
                        mProxy->setPlayerAttributes(request);
                    }
                }
            }
        }
        break;

    case GameManager::IN_GAME:
        if (gameData != nullptr && gameData->getHostId() == this->getBlazeId())
        {
            //  open/close to matchmaking
            setOpenMode(*gameData, mOwner->getGameManagerUtil()->getJoinInProgress());
        }
        break;

    case GameManager::POST_GAME:
        if (gameData != nullptr && gameData->getHostId() == this->getBlazeId())
        {
            if (mOwner->getBatchAttributeSetPostGame())
            {
                //  batch attribute set
                SetGameAttributesRequest req;
                fillGameAttributeMap(req.getGameAttributes());
                req.setGameId(gameData->getGameId());
                if(gameData->getGameSettings()->getVirtualized())
                {
#ifdef TARGET_arson
                    Arson::SetGameAttributesRequest arsonReq;
                    req.copyInto(arsonReq.getSetGameAttributesRequest());
                    rpcResult = mArsonProxy->setGameAttributes(arsonReq);
#endif
                }
                else
                {
                    rpcResult = mProxy->setGameAttributes(req);
                }

                gameData = mGameInst.getGameData();
                if (gameData != nullptr && mOwner->getPlayerAttrValues().size() > 0)
                {
                    //  set all player attributes
                    for (size_t i = 0; i < gameData->getNumPlayers(); ++i)
                    {
                        PlayerData *player = gameData->getPlayerDataByIndex(i);
                        SetPlayerAttributesRequest request;
                        request.setGameId(gameData->getGameId());
                        request.setPlayerId(player->mPlayerId);
                        fillPlayerAttributeMap(request.getPlayerAttributes());
                        mProxy->setPlayerAttributes(request);
                        gameData = mGameInst.getGameData();
                    }
                }
            }
        }

        //  indicates post game actions have finished - resumes simulation for the current role.
        mGameInst.finishPostGame();
        break;
    default:
        break;
    }

    if (rpcResult != Blaze::ERR_OK)
    {
       BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerInstance].onGameStateChange: Got unexpected setGameAttributes error " << ErrorHelp::getErrorName(rpcResult) << " in game " << localGameId << " onGameStateChanged to " << (GameStateToString(stateChange->getNewGameState())) << "." );
    }
}

} // Stress
} // Blaze

