/*************************************************************************************************/
/*!
    \file


    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/config/config_file.h"
#include "framework/config/config_sequence.h"
#include "framework/util/random.h"

#include "gamebrowsermodule.h"
#include "loginmanager.h"
#include "blazerpcerrors.h"

#include "framework/protocol/shared/heat2decoder.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/tdf/attributes.h"


namespace Blaze
{
namespace Stress
{

    static const char8_t* GB_ACTION_STRINGS[] =  {
        "noop",
        "getGameData"
    };

    static const char8_t* GB_METRIC_STRINGS[] =  {
        "notifyUpdates",
        "notifyRemovals",
        "notifications"
    };


/*************************************************************************************************/
/*
    GameBrowserModule methods
*/
/*************************************************************************************************/


GameBrowserModule::GameBrowserModule()
    : mCreationConnDist(0.0f),
    mSearchFrequency(0.0f),
    mRemainingCreatorCount(0),
    mGameManager(BLAZE_NEW GameManagerUtil()),
    mDumpStats(DUMP_STATS_NONE),
    mMatchmakerRulesUtil(BLAZE_NEW MatchmakerRulesUtil())
{
    for (int i = 0; i < GameBrowserModule::NUM_METRICS; ++i)
    {
        mMetricCount[i] = 0;
    }
}


GameBrowserModule::~GameBrowserModule()
{
    delete mGameManager;
    delete mMatchmakerRulesUtil;
}


StressModule* GameBrowserModule::create()
{
    return BLAZE_NEW GameBrowserModule();
}

void GameBrowserModule::initCreateGameRequestOverride()
{
    // Create game Override
    // TODO: need a better way to start with default values from Game Manager

    // Based on the seed, we calculate the number of players in the game
    uint32_t playerLimitRoll = Random::getRandomNumber(mGameManager->getGamePlayerSeed());
    uint32_t gamePlayerLimit = (playerLimitRoll >= mGameManager->getGamePlayerLowerLimit()) ? playerLimitRoll : mGameManager->getGamePlayerLowerLimit();

    mCreateOverride.getGameCreationData().setMaxPlayerCapacity(static_cast<uint16_t>(gamePlayerLimit));
    mCreateOverride.getGameCreationData().setNetworkTopology(mGameManager->getGameTopology());
    mCreateOverride.getGameCreationData().setVoipNetwork(VOIP_DISABLED);

    mCreateOverride.getGameCreationData().getGameSettings().setOpenToBrowsing();
    mCreateOverride.getGameCreationData().getGameSettings().setOpenToInvites();
    mCreateOverride.getGameCreationData().getGameSettings().setOpenToJoinByPlayer();
    mCreateOverride.getGameCreationData().getGameSettings().setOpenToMatchmaking();
    mCreateOverride.getGameCreationData().getGameSettings().setRanked();

    mCreateOverride.getCommonGameData().setGameProtocolVersionString(mGameManager->getGameProtocolVersionString());

    // not for matchmaking
    // Game attributes
    Collections::AttributeMap &gameAttribs = mCreateOverride.getGameCreationData().getGameAttribs();
    for(int i = 0; i < getNumGameAttribs(); i++)
    {
        char name[Collections::MAX_ATTRIBUTENAME_LEN];
        char value[Collections::MAX_ATTRIBUTEVALUE_LEN];

        blaze_snzprintf(name, sizeof(name), "gameattr_%.9d", i);
        blaze_snzprintf(value, sizeof(value), "gameattrvalue_%.9d", i);

        gameAttribs[name] = value; 
    }

    // for matchmaking
    // Create Game Attributes
    GameAttributes::iterator itr = mGameAttributes.begin();
    GameAttributes::iterator end = mGameAttributes.end();

    for (; itr != end; ++itr)
    {
        StressGameAttribute& ga = *itr;
        const uint16_t autoSuffixCount = (uint16_t)ga.getAutoSuffixCount();
        if (autoSuffixCount == 0)
        {
            // only need to do once as no autosuffixes added
            char name[Collections::MAX_ATTRIBUTENAME_LEN];
            gameAttribs[ga.getAttribName(name, sizeof(name), 0)] = ga.chooseCreateValue();
        }
        else
        {
            // add for all autosuffixed rules
            for (uint32_t i = 0; i < autoSuffixCount; ++i)
        {
            char name[Collections::MAX_ATTRIBUTENAME_LEN];
            gameAttribs[ga.getAttribName(name, sizeof(name), i)] = ga.chooseCreateValue();
        }
    }
    }

    //  TODO - deal with public vs private.
    SlotCapacitiesVector &slotVector = mCreateOverride.getSlotCapacities();
    slotVector[SLOT_PUBLIC_PARTICIPANT] = mCreateOverride.getGameCreationData().getMaxPlayerCapacity();

    // if game name rule enabled, use randomized name instead of setInstanceCreateGame default
    if (mMatchmakerRulesUtil->isGameNameRuleEnabled())
    {
        eastl::string gameName;
        mMatchmakerRulesUtil->makeGameNameRuleFromConfig(mMatchmakerRulesUtil->getPredefinedRuleConfig(), true, gameName);
        mCreateOverride.getGameCreationData().setGameName(gameName.c_str());
    }

    mGameManager->setCreateGameRequestOverride(&mCreateOverride);
}
// update specified gm instance's next UpdateGameNameRequest's using game name rule randomization as needed
void GameBrowserModule::initUpdateGameNameRequestOverride(GameManagerUtilInstance &gameManagerInstance)
{
    // if game name rule enabled, use randomized name instead of setInstanceCreateGame default
    if (getMatchmakerRulesUtil()->isGameNameRuleEnabled())
    {
        eastl::string gameName;
        getMatchmakerRulesUtil()->makeGameNameRuleFromConfig(getMatchmakerRulesUtil()->getPredefinedRuleConfig(), true, gameName);
        gameManagerInstance.setUpdateGameNameRequest(gameName.c_str()); 
    }
}

void GameBrowserModule::initJoinGameRequestOverride()
{
    // Join Game Override

    mJoinOverride.getPlayerJoinData().setDefaultSlotType(SLOT_PUBLIC_PARTICIPANT);
    Collections::AttributeMap &playerAttribs = mJoinOverride.getPlayerJoinData().getPlayerDataList().pull_back()->getPlayerAttributes();
    for(int i = 0; i < getNumGameAttribs(); i++)
    {
        char name[Collections::MAX_ATTRIBUTENAME_LEN];
        char value[Collections::MAX_ATTRIBUTEVALUE_LEN];

        blaze_snzprintf(name, sizeof(name), "playattr_%.9d", i);
        blaze_snzprintf(value, sizeof(value), "playattrvalue_%.9d", i);

        playerAttribs[name] = value; 
    }
    // for matchmaking
    playerAttribs["playerAttrib1"] = "Large";
}

bool GameBrowserModule::initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil)
{
    if(!StressModule::initialize(config, bootUtil))
        return false;

    if(!parseConfig(config))
        return false;

    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameBrowserModule].initialize: " << GB_ACTION_STRINGS[mAction] << " action selected.");

    // Initialize the game manager utility
    mGameManager->initialize(config, bootUtil);

    // Initialize the matchmaking rules
    mMatchmakerRulesUtil->parseRules(config);

    initJoinGameRequestOverride();

    return true;
}

const char8_t* GameBrowserModule::StressGameAttribute::getAttribName(char8_t* name, uint32_t len, uint32_t ndx) const
{
    if (mAutoSuffixCount == 0)
    {
        blaze_snzprintf(name, len, "%s", mPrefix.c_str());
    }
    else
    {
        // autosuffix: if count > 0, makes consequetive numeric suffixes myPrefix1, myPrefix2,...
        blaze_snzprintf(name, len, "%s%.4d", mPrefix.c_str(), ndx + 1);
    }

    return name;
}

const char8_t* GameBrowserModule::StressGameAttribute::chooseValue(SelectionMode mode) const
{
    uint32_t choice = Random::getRandomNumber(100);
    PossibleValues::const_iterator pvItr = mPossibleValues.begin();
    PossibleValues::const_iterator pvEnd = mPossibleValues.end();

    for (; pvItr != pvEnd; ++pvItr)
    {
        const PossibleValue& pv = *pvItr;
        uint32_t chance = mode == CREATE ? pv.getCreateChance() : pv.getBrowseChance();
        if (chance > choice)
        {
            return pv.getValue();
        }
        else
        {
            choice -= chance;
        }
    }

    return "";
}

void GameBrowserModule::addMetric(Metric metric)
{
    this->mMetricCount[metric]++;
}


StressInstance* GameBrowserModule::createInstance(StressConnection* connection, Login* login)
{
    GameBrowserInstance *instance = BLAZE_NEW GameBrowserInstance(this, connection, login);
    InstanceMap::insert_return_type inserter = mInstances.insert(instance->getIdent());
    if (inserter.first == mInstances.end())
    {
        delete instance;
        instance = nullptr;
    }
    else
    {       
        inserter.first->second = instance;
    }
    return instance;
}


void GameBrowserModule::setConnectionDistribution(uint32_t conns)
{
    mRemainingCreatorCount = (uint32_t)(conns);
    StressModule::setConnectionDistribution(conns);
}


bool GameBrowserModule::saveStats()
{

    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameBrowserModule].saveStats: instances(" << mInstances.size() << "), games(" << mGameManager->getGameCount() << ")");

    for (int i = 0; i < GameBrowserModule::NUM_METRICS; ++i)
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameBrowserModule].saveStats: " << GB_METRIC_STRINGS[i] << "=" << mMetricCount[i]);
    }

    if (mDumpStats == DUMP_STATS_AVG)
    {
        printAvgGBStats();
    }
    else if (mDumpStats == DUMP_STATS_FULL)
    {
        printAllGBStats();
    }

    mGameManager->dumpStats(this);

    return false;
}

void GameBrowserModule::printGBStat(const GameBrowserInstance &inst)
{
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameBrowserModule].printGBStat: GBList (" << inst.getListId() << ") => (" << inst.getGameSet().size() 
                   << ") games firstUpdate(" << (inst.hasReceivedFirstUdpate() ? "true" : "false") << ") g(" << inst.getNumGamesInFirstUpdate() 
                   << ")/t(" << inst.getFirstUpdateTimeMs() << " ms)");
}

void GameBrowserModule::printAvgGBStats()
{
    if (mInstances.empty())
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameBrowserModule].printAvgGBStats: GBM - No Instances.");
        return;
    }

    struct GBStat
    {
        GBStat() : stressId(0), listId(0), numGames(0) {}
        GBStat(uint32_t sId, GameManager::GameBrowserListId lId, uint32_t ng) : stressId(sId), listId(lId), numGames(ng) {}

        uint32_t stressId;
        GameManager::GameBrowserListId listId;
        uint32_t numGames;
    };

    GBStat high, low;
    uint64_t totalGames = 0;

    InstanceMap::const_iterator iter = mInstances.begin();
    InstanceMap::const_iterator end = mInstances.end();

    uint32_t totalLists = 0;
    for (; iter != end; iter++)
    {
        GameBrowserInstance *inst = iter->second;

        if(inst->getListId() == 0)
            continue;
        ++totalLists;

        uint32_t curNumGames = (uint32_t)inst->getGameSet().size();
        totalGames += curNumGames;

        if ((iter == mInstances.begin()) || (curNumGames < low.numGames))
        {
            low = GBStat(inst->getIdent(), inst->getListId(), curNumGames);
        }

        if ((iter == mInstances.begin()) || (curNumGames > high.numGames))
        {
            high = GBStat(inst->getIdent(), inst->getListId(), curNumGames);
        }
    }

    uint32_t avgGamesPerList = totalLists > 0 ? (uint32_t)(totalGames / totalLists) : 0;
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameBrowserModule].printAvgGBStats: GBList(high(id(" << high.stressId << ", " << high.listId << "), g(" << high.numGames << ")), low(id(" 
                   << low.stressId << ", " << low.listId << "), g(" << low.numGames << ")), tgames(" << totalGames << "), tlists(" << totalLists 
                   << "), avggpl(" << avgGamesPerList << ")");
}

void GameBrowserModule::printAllGBStats()
{
    InstanceMap::const_iterator iter = mInstances.begin();
    InstanceMap::const_iterator end = mInstances.end();

    for (; iter != end; iter++)
    {
        GameBrowserInstance *inst = iter->second;

        if(inst->getListId() == 0)
            continue;

        printGBStat(*inst);
    }
}


bool GameBrowserModule::parseConfig(const ConfigMap& config)
{
    const char8_t* action = config.getString("action", "");

    if (blaze_stricmp(action, GB_ACTION_STRINGS[ACTION_NOOP]) == 0)
    {
        mAction = ACTION_NOOP;
    }     
    else if (blaze_stricmp(action, GB_ACTION_STRINGS[ACTION_GET_GAME_DATA]) == 0)
    {
        mAction = ACTION_GET_GAME_DATA;
    }     
    else
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameBrowserModule].parseConfig: unrecognized action: '" << action << "'");
        return false;
    }

    // creation count determined by setConnectionDistribution override.
    mCreationConnDist = (float)config.getDouble("gameCreators", 0.0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameBrowserModule].parseConfig: gameCreators = " << mCreationConnDist);

    // number of searching instances.
    mSearchFrequency = (float)config.getDouble("gameSearchers", 0.0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameBrowserModule].parseConfig: gameSearchers = " << mSearchFrequency);
    
    // game browser list configuration
    blaze_strnzcpy(mListConfigName, config.getString("listConfigName", "default"), 100);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameBrowserModule].parseConfig: listConfigName = " << mListConfigName);

    // Duration of a created list subscription
    mListLife = config.getUInt32("listLife", (uint32_t)-1);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameBrowserModule].parseConfig: listLife = " << mListLife);

    // Chance of a list getting destroyed once listLife is hit
    mListDestroyChance = (float)config.getDouble("listDestroyChance", 0.5);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameBrowserModule].parseConfig: listDestroyChance = " << mListDestroyChance);

    mListUpdateMaxInterval = config.getUInt32("listUpdateMaxInterval", 75);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameBrowserModule].parseConfig: listUpdateMaxInterval = " << mListUpdateMaxInterval);

    mSnapshotListSync = config.getBool("snapshotListSync", false);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameBrowserModule].parseConfig: snapshotListSync = " << mSnapshotListSync);

    mSubscriptionLists = config.getBool("subscriptionLists", false);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameBrowserModule].parseConfig: subscriptionLists = " << mSubscriptionLists);

    // Chance instance will lookup full game data on existing list
    mListGetDataChance = (float)config.getDouble("listGetDataChance", 0.5);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameBrowserModule].parseConfig: listGetDataChance = " << mListGetDataChance);

    // cycles to wait to start game browser actions
    mGameBrowserDelay = config.getUInt32("gameBrowserDelay", 8000);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameBrowserModule].parseConfig: gameBrowserDelay = " << mGameBrowserDelay);

    mNumGameAttribs = config.getInt32("numGameAttribs", 100);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameBrowserModule].parseConfig: numGameAttribs = " << mNumGameAttribs);

    mGameListSize = config.getUInt32("gameListSize", 50);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameBrowserModule].parseConfig: gameListSize = " << mGameListSize);

    const char8_t* GB_GAME_ATTRIBUTES = "gbGameAttributes";
    const ConfigSequence* gameAttributes = config.getSequence(GB_GAME_ATTRIBUTES);

    if (gameAttributes == nullptr)
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameBrowserModule].parseConfig: Note failed to find sequence '" << GB_GAME_ATTRIBUTES << "'.");
    }

    while (gameAttributes != nullptr && gameAttributes->hasNext())
    {
        const ConfigMap* gameAttribute = gameAttributes->nextMap();
        StressGameAttribute stressGameAttrib;

        stressGameAttrib.setPrefix(gameAttribute->getString("prefix", ""));
        stressGameAttrib.setAutoSuffixCount(gameAttribute->getUInt32("autoSuffixCount", 0));
        stressGameAttrib.setThresholdName(gameAttribute->getString("thresh", ""));

        const char8_t* GB_GAME_ATTRIBUTES_POSSIBLE_VALUES = "possibleValues";
        const ConfigSequence* possibleValues = gameAttribute->getSequence(GB_GAME_ATTRIBUTES_POSSIBLE_VALUES);

        if (possibleValues == nullptr)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameBrowserModule].parseConfig: Failed to find sequence '" << GB_GAME_ATTRIBUTES_POSSIBLE_VALUES << "'.");
            return false;
        }

        uint32_t createPercent = 0, browsePercent = 0;
        while (possibleValues->hasNext())
        {
            const ConfigMap* possibleValue = possibleValues->nextMap();
            PossibleValue stressPossibleValue;
            
            stressPossibleValue.setValue(possibleValue->getString("value", ""));
            stressPossibleValue.setCreateChance(possibleValue->getUInt32("create", 0));
            stressPossibleValue.setBrowseChance(possibleValue->getUInt32("browse", 0));

            createPercent += stressPossibleValue.getCreateChance();
            browsePercent += stressPossibleValue.getBrowseChance();

            stressGameAttrib.getPossibleValues().push_back(stressPossibleValue);
        }

        if (createPercent != 100)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameBrowserModule].parseConfig: Create Percent does not add up to 100; createPercent(" << createPercent << ").");
            return false;
        }

        if (browsePercent != 100 && browsePercent != 0)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameBrowserModule].parseConfig: Browse Percent does not add up to 100; browsePercent(" << browsePercent << ").");
            return false;
        }
        
        mGameAttributes.push_back(stressGameAttrib);
    }

    const char8_t* dumpStatsStr = config.getString("dumpStats", "DUMP_STATS_NONE");
    if (!parseDumpStats(mDumpStats, dumpStatsStr))
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameBrowserModule].parseConfig: Invalid value '" << dumpStatsStr << "' for dumpStats.");
        return false;
    }

    return true;
}

bool GameBrowserModule::parseDumpStats(DumpStats& dumpStats, const char8_t* dumpStatsStr)
{
    if (blaze_stricmp(dumpStatsStr, "DUMP_STATS_FULL") == 0)
    {
        dumpStats = DUMP_STATS_FULL;
        return true;
    }
    else if (blaze_stricmp(dumpStatsStr, "DUMP_STATS_AVG") == 0)
    {
        dumpStats = DUMP_STATS_AVG;
        return true;
    }
    else if (blaze_stricmp(dumpStatsStr, "DUMP_STATS_NONE") == 0)
    {
        dumpStats = DUMP_STATS_NONE;
        return true;
    }
    else
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameBrowserModule].parseDumpStats Invalid value '" << dumpStatsStr << "' for dumpStats.");
        return false;
    }
}

void GameBrowserModule::releaseInstance(StressInstance *inst)
{
    mInstances.erase(inst->getIdent());
}

/*************************************************************************************************/
/*
GameBrowserInstance methods
*/
/*************************************************************************************************/


GameBrowserInstance::GameBrowserInstance(GameBrowserModule *owner, StressConnection* connection, Login* login) :
    StressInstance(owner, connection, login, BlazeRpcLog::gamemanager),
    mOwner(owner),
    mProxy(BLAZE_NEW GameManagerSlave(*getConnection())),
    mListId(0),
    mNumGamesInFirstUpdate(0),
    mFirstUpdate(true)
{
    mGameManagerInstance = BLAZE_NEW GameManagerUtilInstance(owner->getGameManagerUtil());
    mGameManagerInstance->setStressInstance(this);

    int32_t roll = Random::getRandomNumber(100);
    int32_t createAction = (int32_t)(mOwner->getGameCreationFreq() * 100);

    // create a game
    if (roll < createAction)
    {
        mGameManagerInstance->setRole(GameManagerUtil::ROLE_HOST);
    }
    else
    {
        mGameManagerInstance->setRole(GameManagerUtil::ROLE_JOINER);
    }

    // Add async handlers
    connection->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMELISTUPDATE, this);

    mStartTime = TimeValue::getTimeOfDay();
    mCurrentDelay = mOwner->getGameBrowserDelay();
}


GameBrowserInstance::~GameBrowserInstance()
{
    //  clear out game map
    getConnection()->removeAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMELISTUPDATE, this);
    if (mGameManagerInstance != nullptr)
    {
        delete mGameManagerInstance;
    }

    delete mProxy;

    mOwner->releaseInstance(this);
}

void GameBrowserInstance::start()
{
    StressInstance::start();
}


BlazeRpcError GameBrowserInstance::execute()
{
    if (!isLoggedIn())
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameBrowserInstance].execute: Unable to process Execute() for instance.  User is not logged in.");
        // Let stress instance log us back in first.
        return ERR_OK;
    }

    BlazeRpcError gameErr = Blaze::ERR_OK;

    // Create a list
    if (mListId == 0)
    {
         gameErr = createGameBrowserList();
         return gameErr;
    }

    // Check an existing list for its duration
    if (mListId != 0 && Blaze::TimeValue::getTimeOfDay().getMillis() > (mListCreationTime.getMillis() + mOwner->getListLifespan()))
    {
        // Our random percentage 
        int32_t roll = Random::getRandomNumber(100);

        // Attempt to disperse destroys
        if (roll < (mOwner->getListDestroyChance() * 100))
        {
            if (mOwner->getSubscriptonLists())
            {
                return destroyGameBrowserList();
            }
            else
            {
                mListId = 0;
            }
        }

    }

    if (mListId != 0 && Blaze::TimeValue::getTimeOfDay().getSec() - mLastUpdateTime.getSec() > mOwner->getListUpdateMaxInterval())
    {
        if (mOwner->getSubscriptonLists())
        {
            BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameBrowserInstance].execute: Expected update for list (" << mListId << ") has not been received.");
            destroyGameBrowserList();
        }
        // reset list id for re-creating gb list
        mListId = 0;
    }

    return gameErr;
}


const char8_t *GameBrowserInstance::getName() const
{
    return GB_ACTION_STRINGS[mOwner->getAction()];;
}


void GameBrowserInstance::onAsync(uint16_t component, uint16_t type, RawBuffer* payload)
{
    if (component != GameManagerSlave::COMPONENT_ID)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameBrowserInstance].onAsync: Unwanted component id: " << component);
        return;
    }

    Heat2Decoder decoder;
    GameManager::NotifyGameListUpdate notifyListUpdate;

    switch (type)
    {

    case GameManagerSlave::NOTIFY_NOTIFYGAMELISTUPDATE:
        // We have received the list information
        if ((decoder.decode(*payload, notifyListUpdate) == ERR_OK) && 
            (notifyListUpdate.getListId() == mListId))
        {
            mOwner->addMetric(GameBrowserModule::NOTIFICATIONS);
            GameManager::GameIdList &removalList = notifyListUpdate.getRemovedGameList();
            GameManager::GameBrowserMatchDataList &updateList = notifyListUpdate.getUpdatedGames();

            BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager,"[GameBrowserInstance].onAsync: Notify game list(" << mListId 
                            << ") update.  Removals(" << removalList.size() << ") Updates(" << updateList.size() << ")");

            //removals
            GameIdList::const_iterator removeIter = removalList.begin();
            GameIdList::const_iterator removeEnd = removalList.end();
            for ( ; removeIter != removeEnd; ++removeIter)
            {
                mOwner->addMetric(GameBrowserModule::NOTIFY_REMOVALS);
                // remove game from game map
                mGameSet.erase(*removeIter);
            }

            if (mFirstUpdate)
            {
                mFirstUpdateTime = TimeValue::getTimeOfDay() - mListCreationTime;
            }

            mLastUpdateTime = TimeValue::getTimeOfDay();
            // add/updates
            GameManager::GameBrowserMatchDataList::const_iterator iter = updateList.begin();
            GameManager::GameBrowserMatchDataList::const_iterator end = updateList.end();
            for ( ; iter != end; ++iter)
            {
                const GameBrowserGameData& game = (*iter)->getGameData();
                mOwner->addMetric(GameBrowserModule::NOTIFY_UPDATES);
                mGameSet.insert(game.getGameId());
                if (mFirstUpdate) ++mNumGamesInFirstUpdate;
            }

            mFirstUpdate = false;

        }
        break;

    }

}

/*
    RPC function wrappers
*/

BlazeRpcError GameBrowserInstance::createGameBrowserList()
{
    BlazeRpcError rpcResult = ERR_OK;
    GetGameListRequest request;
    GetGameListResponse response;
    GetGameListSyncResponse syncRsp;

    MatchmakingCriteriaError error;
    const MatchmakerRulesUtil *matchmakerRulesUtil = mOwner->getMatchmakerRulesUtil();

    // List config name
    request.setListConfigName(mOwner->getListConfigName());

    // List capacity
    request.setListCapacity(mOwner->getGameListSize());

    request.setIgnoreGameEntryCriteria(false);

    // game protocol version rule
    if (matchmakerRulesUtil->getUseRandomizedGameProtocolVersionString())
    {
        //use the configured name + the persona name
        char8_t gameProtocolVersionString[Blaze::GameManager::MAX_GAMEPROTOCOLVERSIONSTRING_LEN];
        blaze_strnzcpy(gameProtocolVersionString, matchmakerRulesUtil->getGameProtocolVersionString(), Blaze::GameManager::MAX_GAMEPROTOCOLVERSIONSTRING_LEN);
        blaze_strnzcat(gameProtocolVersionString, getLogin()->getUserLoginInfo()->getPersonaDetails().getDisplayName(), Blaze::GameManager::MAX_GAMEPROTOCOLVERSIONSTRING_LEN);  
        request.getCommonGameData().setGameProtocolVersionString(gameProtocolVersionString);
    }
    else
    {
        request.getCommonGameData().setGameProtocolVersionString(matchmakerRulesUtil->getGameProtocolVersionString());
    }

    // List criteria
    MatchmakingCriteriaData &criteria = request.getListCriteria();

    // Set matchmaking criteria rules 
    // Note may override any rule settings from auto-suffix mechanism
    StartMatchmakingScenarioRequest dummyReq;
    matchmakerRulesUtil->buildMatchmakingCriteria(criteria, dummyReq, 0, true); // create GB list for full games

    // Searching for Games with these Generic Game Attributes
    // autosuffix: if count > 0, makes consequetive numeric suffixes myPrefix1, myPrefix2,...
    GameBrowserModule::GameAttributes::const_iterator itr = mOwner->getGameAttributes().begin();
    GameBrowserModule::GameAttributes::const_iterator end = mOwner->getGameAttributes().end();

    for (; itr != end; ++itr)
    {
        const GameBrowserModule::StressGameAttribute& ga = *itr;

        for (uint32_t i = 0; i < ga.getAutoSuffixCount(); ++i)
        {
            char name[Collections::MAX_ATTRIBUTENAME_LEN];
            ga.getAttribName(name, sizeof(name), i);
            GameManager::GameAttributeRuleCriteria* gameRuleCriteria = criteria.getGameAttributeRuleCriteriaMap()[name];
            if (gameRuleCriteria == nullptr)
            {
                gameRuleCriteria = criteria.getGameAttributeRuleCriteriaMap()[name] = criteria.getGameAttributeRuleCriteriaMap().allocate_element();
            }
            else
            {
                BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameBrowserInstance].createGameBrowserList: Multiple attributes assigned to rule " 
                    << name << ".  This will lead to a problem if the rule is ARBITRARY. (Skipping 2nd set)");
                continue;
            }

            gameRuleCriteria->setMinFitThresholdName(ga.getThresholdName());
            gameRuleCriteria->getDesiredValues().push_back(ga.chooseBrowseValue());
        }
    }

//     GameManager::GenericRulePrefs* playerRulePrefs = BLAZE_NEW GameManager::GenericRulePrefs();
//     criteria.getGenericRulePrefsList().push_back(playerRulePrefs);
// 
//     // modify the player generic rule, ruleName=playerAttribRule, MinFitTld=quickMatch, value=large
//     playerRulePrefs->setRuleName("playerAttribRule");
//     playerRulePrefs->setMinFitThresholdName("quickMatch");
//     playerRulePrefs->getDesiredValues().push_back("Large");
    

    // Create the list
    if (mOwner->getSnapshotListSync())
    {
        rpcResult =mProxy->getGameListSnapshotSync(request, syncRsp, error);
    }
    else
    {
        if (mOwner->getSubscriptonLists())
        {
            rpcResult = mProxy->getGameListSubscription(request, response, error);
        }
        else
        {
            rpcResult = mProxy->getGameListSnapshot(request, response, error);
        }
    }

    if (rpcResult != ERR_OK)
    {
        //  failed to create, release this creator.
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameBrowserInstance].createGameBrowserList: unable to create game browser list. Error(" 
                       << (Blaze::ErrorHelp::getErrorName(rpcResult)) << ") " << error.getErrMessage() << ".");
        return rpcResult;
    }

    if (response.getListId() % 100 == 0)
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameBrowserInstance].createGameBrowserList: GBRequest " << request);
    }

    mListCreationTime = TimeValue::getTimeOfDay();
    mLastUpdateTime = mListCreationTime;
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameBrowserInstance].createGameBrowserList: created game browser list(" << response.getListId() << ") expected games (" << response.getNumberOfGamesToBeDownloaded() << ") max fit score (" << response.getMaxPossibleFitScore() << ").");

    if (!mOwner->getSnapshotListSync())
        mListId = response.getListId();

    return rpcResult;
}

void GameBrowserInstance::onLogout(BlazeRpcError result)
{
    // GameBrowser server will simply destroy our list.
    mListId = 0;

    StressInstance::onLogout(result);
}

BlazeRpcError GameBrowserInstance::destroyGameBrowserList()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameBrowserInstance].destroyGameBrowserList: Destroying game browser list(" << mListId << ")");

    BlazeRpcError rpcResult = ERR_OK;
    DestroyGameListRequest request;

    // list id
    request.setListId(mListId);

    rpcResult = mProxy->destroyGameList(request);

    if (rpcResult != ERR_OK)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameBrowserInstance].destroyGameBrowserList: Unable to destroy game browser list(" << mListId 
                       << "). Error(" << (Blaze::ErrorHelp::getErrorName(rpcResult)) << ") " << (Blaze::ErrorHelp::getErrorDescription(rpcResult)));

        // force logout to dump lists owned by this user
        logout();
    }
    
    // clear local tracking
    mListId = 0;

    return rpcResult;
}


BlazeRpcError GameBrowserInstance::getGameData()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameBrowserInstance].getGameData: Getting game data for list(" << mListId << ")");

    BlazeRpcError rpcResult = ERR_OK;
    GetGameDataFromIdRequest request;
    GameBrowserDataList response;
 
    // loop through our known games.
    GameBrowserGameSet::const_iterator iter = mGameSet.begin();
    GameBrowserGameSet::const_iterator end = mGameSet.end();
    for ( ; iter != end; ++iter)
    {
        request.getGameIds().push_back(*iter);
    }

    rpcResult = mProxy->getGameDataFromId(request, response);

    if (rpcResult != ERR_OK)
    {
        //  failed to create, release this creator.
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameBrowserInstance].getGameData: Unable to get game data for list(" << mListId 
                       << ").  Error(" << (Blaze::ErrorHelp::getErrorName(rpcResult)) << ") " << (Blaze::ErrorHelp::getErrorDescription(rpcResult)));
    }
    return rpcResult;
}


BlazeRpcError GameBrowserInstance::getFullGameData()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameBrowserInstance].getFullGameData: Getting full game data for list(" << mListId << ")");

    BlazeRpcError rpcResult = ERR_OK;
    GetFullGameDataRequest request;
    GetFullGameDataResponse response;

    // loop through our known games.
    GameBrowserGameSet::const_iterator iter = mGameSet.begin();
    GameBrowserGameSet::const_iterator end = mGameSet.end();
    for ( ; iter != end; ++iter)
    {
        request.getGameIdList().push_back(*iter);
    }

    rpcResult = mProxy->getFullGameData(request, response);

    if (rpcResult != ERR_OK)
    {
        //  failed to create, release this creator.
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameBrowserInstance].getFullGameData: Unable to get full game data for list(" << mListId 
                       << ").  Error(" << (Blaze::ErrorHelp::getErrorName(rpcResult)) << ") " << (Blaze::ErrorHelp::getErrorDescription(rpcResult)));
    }

    return rpcResult;
}


void GameBrowserInstance::onDisconnected()
{
    BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameBrowserInstance].getFullGameData: Disconnected!");
}



} // Stress end
} // Blaze end
