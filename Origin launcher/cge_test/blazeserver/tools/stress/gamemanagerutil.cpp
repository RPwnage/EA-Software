/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanagerutil.h"
#include "loginmanager.h"

#include "blazerpcerrors.h"
#include "framework/config/config_file.h"
#include "framework/config/config_boot_util.h"
#include "framework/config/config_sequence.h"
#include "framework/config/config_map.h"
#include "framework/connection/selector.h"
#include "framework/util/shared/blazestring.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/util/random.h"

#ifdef TARGET_censusdata
#include "censusdata/tdf/censusdata.h"
#endif

#include "gamemanager/tdf/gamebrowser.h"
#include "gamemanager/tdf/gamemanager_server.h"
#include "gamemanager/tdf/gamemanagerconfig_server.h" // for GameManagerServerConfig in updateGamePresenceForLocalUser
#include "framework/controller/controller.h" // for gController in updateGamePresenceForLocalUser

namespace Blaze
{
namespace Stress
{

static const char8_t* GAMEMANAGER_UTIL_METRIC_STRINGS[] =  {
    "gamesCreated",
    "gamesDestroyed",
    "playersLocalJoin",
    "playersJoining",
    "playersJoinCompleted",
    "playersJoinTimeouts",
    "playersLeft",
    "joinGameFull",
    "gamesConnLost",
    "gamesCycled",
    "gamesUnregd",
    "gamesReset",
    "gamesReturned",
    "playersRemoved",
    "gamesAborted",
    "gamesEarlyEnd",
    "censusUpdates"
};


const char8_t* GameManagerUtil::ROLE_STRINGS[NUM_ROLES] = {
    "ROLE_NONE",
    "ROLE_HOST",
    "ROLE_JOINER"
};

GameManagerUtil::GameManagerUtil() :
    mMaxGames(0),
    mMaxVirtualCreators(0),
    mGameLifespan(0),
    mGameLifespanDeviation(0),
    mGameStartTimer(0),
    mGamePlayerLifespan(0),
    mGamePlayerLifespanDeviation(0),
    mGamePlayerSeed(0),
    mGamePlayerLowerLimit(0),
    mGamePlayerMinRequired(0),
    mGameSettingsRandomUpdates(0.0f),
    mGameNameRandomUpdates(0.0f),
    mGameExternalStatusRandomUpdates(0.0f),
    mJoinInProgress(false),
    mShooterIntelligentDedicatedServer(false),
    mCensusSubscribe(true),
    mUseArson(false),
    mConfiguredPingSites(nullptr),
    mGameTopology(PEER_TO_PEER_FULL_MESH),
    mGames(),
    mNextJoinGameIt(mGames.end()),
    mCreateRequest(nullptr),
    mJoinRequest(nullptr),
    mPendingGameCount(0),
    mResetableServers(0)
{
    mGameProtocolVersionString[0] = '\0';
    mLogBuffer[0] = '\0';

    for (int i = 0; i < NUM_METRICS; ++i)
    {
        mMetricCount[i] = 0;
    }

    for (int i = 0; i < NUM_ROLES; ++i)
    {
        mRoleCount[i] = 0;
    }
}


GameManagerUtil::~GameManagerUtil()
{
    while (!mGames.empty())
    {
        GameIdToDataMap::iterator it = mGames.begin();
        delete it->second;
        mGames.erase(it);
    }
}


//    parse game manager utility specific configuration for setup
bool GameManagerUtil::initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil)
{
    mMaxGames = config.getUInt32("gmMaxGames", (uint32_t)(-1));
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].initialize: gmMaxGames = " << mMaxGames);

    mMaxVirtualCreators = config.getUInt32("gmMaxVirtualCreators", (uint32_t)(0));
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].initialize: gmMaxVirtualCreators = " << mMaxVirtualCreators);

    mGameLifespan = config.getUInt32("gmGameLifespan", (uint32_t)(-1));
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].initialize: gmGameLifespan = " << mGameLifespan);

    mGameLifespanDeviation = config.getUInt32("gmGameLifespanDeviation", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].initialize: gmGameLifespanDeviation = " << mGameLifespanDeviation);

    mGameStartTimer = config.getUInt32("gmGameStartTimer", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].initialize: gmGameStartTimer = " << mGameStartTimer);

    mGamePlayerLifespan = config.getUInt32("gmGamePlayerLifespan", (uint32_t)(-1));
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].initialize: gmGamePlayerLifespan = " << mGamePlayerLifespan);

    mGamePlayerLifespanDeviation = config.getUInt32("gmGamePlayerLifespanDeviation", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].initialize: gmGamePlayerLifespanDeviation = " << mGamePlayerLifespanDeviation);

    mGamePlayerLowerLimit = config.getUInt16("gmGamePlayerLowerLimit", 2);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].initialize: gmGamePlayerLowerLimit = " << mGamePlayerLowerLimit);

    mGamePlayerSeed = config.getUInt16("gmGamePlayerSeed", 8);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].initialize: gmGamePlayerSeed = " << mGamePlayerSeed);

    mGamePlayerMinRequired = config.getUInt16("gmGamePlayerMin", mGamePlayerLowerLimit);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].initialize: gmGamePlayerMin = " << mGamePlayerMinRequired);

    bool netParse = ParseGameNetworkTopology(config.getString("gmNetworkTopology", "PEER_TO_PEER_FULL_MESH"), mGameTopology);
    if (netParse == false)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].initialize: Unrecognizable game network topology " 
                       << config.getString("gmNetworkTopology", "PEER_TO_PEER_FULL_MESH"));
    }
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].initialize: gmNetworkTopology =" << GameNetworkTopologyToString(mGameTopology));

    // 0 is off
    mGameSettingsRandomUpdates = (float)config.getDouble("gmGameSettingsRandomUpdates", 0.0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].initialize: gmGameSettingsRandomUpdates = " << mGameSettingsRandomUpdates);
    // 0 is off
    mGameNameRandomUpdates = (float)config.getDouble("gmGameNameRandomUpdates", 0.0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].initialize: gmGameNameRandomUpdates = " << mGameNameRandomUpdates);
    // 0 is off
    mGameExternalStatusRandomUpdates = (float)config.getDouble("gmGameExternalStatusRandomUpdates", 0.0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].initialize: gmGameExternalStatusRandomUpdates = " << mGameExternalStatusRandomUpdates);

    bool resetPlayerAdmin = config.getBool("gmResetPlayerAdmin", false);
    if (resetPlayerAdmin)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].initialize: gmResetPlayerAdmin FEATURE NOT IMPLEMENTED!");
    }

    mJoinInProgress = config.getBool("gmJoinInProgress", false);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].initialize: gmJoinInProgress = " << (mJoinInProgress ? "true" : "false"));

    mShooterIntelligentDedicatedServer = config.getBool("gmShooterIntelligentDedicatedServer", false);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].initialize: gmShooterIntelligentDedicatedServer = " << (mShooterIntelligentDedicatedServer ? "true" : "false"));

    mCensusSubscribe = config.getBool("gmCensusSubscribe", true);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].initialize: gmCensusSubscribe = " << (mCensusSubscribe ? "true" : "false"));

    mUseArson = config.getBool("rspUseArson", true);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].initialize: rspUseArson = " << (mUseArson ? "true" : "false"));

    const char8_t* cfgstr = config.getString("gmGameProtocolVersionString", "stressTester");
    blaze_strnzcpy(mGameProtocolVersionString, cfgstr, sizeof(mGameProtocolVersionString));

    mGameModeAttributeName = config.getString("gmGameModeAttributeName", "OSDK_gameMode");

    mNextJoinGameIt = mGames.end();

#ifdef TARGET_gamereporting
    if (!mGameReportingUtil.initialize(config, bootUtil))
        return false;
#endif


    const ConfigSequence *attrs;
    const ConfigMap *valueSequences;
    const ConfigMap *params;
    attrs = config.getSequence("gmmGameAttrKeys");
    valueSequences = config.getMap("gmmGameAttrValues");
    params = config.getMap("gmmGameAttrUpdate");
    AttrValueUtil::initAttrUpdateValues(attrs, valueSequences, mGameAttrValues);
    AttrValueUtil::initAttrUpdateParams(params, mGameAttrUpdateParams);
    attrs = config.getSequence("gmmPlayerAttrKeys");
    valueSequences = config.getMap("gmmPlayerAttrValues");
    params = config.getMap("gmmPlayerAttrUpdate");
    AttrValueUtil::initAttrUpdateValues(attrs, valueSequences, mPlayerAttrValues);
    AttrValueUtil::initAttrUpdateParams(params, mPlayerAttrUpdateParams);

    mConfiguredPingSites = config.getSequence("gmmGamePingSites");
    if (bootUtil != nullptr)
    {
        PredefineMap::const_iterator itr = bootUtil->getConfigFile()->getDefineMap().find("MOCK_EXTERNAL_SESSIONS");
        if (itr != bootUtil->getConfigFile()->getDefineMap().end())
        {
            mMockExternalSessions = itr->second.c_str();
            BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].initialize: mockExternalSessionPlatform defined " << mMockExternalSessions.c_str());
        }
    }
    //    mark initial create game time.
    return true;
}

void GameManagerUtil::dumpStats(StressModule *module)
{
    size_t playerCount = 0;
    GameIdToDataMap::const_iterator citEnd = mGames.end();
    for (GameIdToDataMap::const_iterator cit = mGames.begin(); cit != citEnd; ++cit)
    {
        playerCount += (*cit).second->getNumPlayers();
    }

    char8_t statbuffer[128];
    blaze_snzprintf(statbuffer, sizeof(statbuffer), "time=%.2f, games=%" PRIsize ", players=%" PRIsize ", resetable=%" PRIsize, module->getTotalTestTime(), mGames.size(), playerCount, mResetableServers);

    char8_t *metricOutput = mLogBuffer;
    metricOutput[0] = '\0';
    char8_t *curStr = metricOutput;

    for (int i = 0; i < NUM_METRICS; ++i)
    {
        blaze_snzprintf(curStr, sizeof(mLogBuffer) - strlen(metricOutput), "%s=%" PRIu64 " ",GAMEMANAGER_UTIL_METRIC_STRINGS[i], this->mMetricCount[i]);
        curStr += strlen(curStr);
    }

    for (int i = 0; i < NUM_ROLES; ++i)
    {
        blaze_snzprintf(curStr, sizeof(mLogBuffer) - strlen(metricOutput), "%s=%u ", ROLE_STRINGS[i], getRoleCount((Role)i));
        curStr += strlen(curStr);
    }

    BLAZE_INFO_LOG(Log::CONTROLLER, "[GameManagerUtil].dumpStats: " << statbuffer << " @ " << metricOutput);

#ifdef TARGET_gamereporting
    mGameReportingUtil.dumpStats(module);    
#endif
}


//    diagnostics for utility not meant to measure load.
void GameManagerUtil::addMetric(Metric metric)
{
    mMetricCount[metric]++;
}

// sets create request network address data, for the specific instance.
// If no game name assigned yet, auto-assigns one based on stress instance id.
void GameManagerUtil::setInstanceCreateGame(StressInstance *inst, CreateGameRequest& req)
{
    if (req.getGameCreationData().getGameName()[0] == '\0')
    {
        char name[MAX_GAMENAME_LEN];
        int32_t ident = inst->getIdent();
        blaze_snzprintf(name, sizeof(name), "gameutilgame_%d", ident);
        req.getGameCreationData().setGameName(name);
    }
    BLAZE_SPAM_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].setInstanceCreateGame: Setting up CreateGameRequest GameName: " << req.getGameCreationData().getGameName());

    NetworkAddress *hostAddress = &req.getCommonGameData().getPlayerNetworkAddress();
    hostAddress->switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
    hostAddress->getIpPairAddress()->getExternalAddress().setIp(inst->getConnection()->getAddress().getIp());
    hostAddress->getIpPairAddress()->getExternalAddress().setPort(inst->getConnection()->getAddress().getPort());
    hostAddress->getIpPairAddress()->getExternalAddress().copyInto(hostAddress->getIpPairAddress()->getInternalAddress());
}

void GameManagerUtil::defaultResetGameRequest(StressInstance *inst, CreateGameRequest &req, uint32_t gameSize)
{
    defaultCreateGameRequest(inst, req, gameSize);
    setInstanceCreateGame(inst, req);
}


void GameManagerUtil::defaultCreateGameRequest(StressInstance *inst, CreateGameRequest& req, uint32_t gameSize)
{
    // Based on the seed, we calculate the number of players in the game
    uint32_t playerLimitRoll = 0;
    if (getGamePlayerSeed() != 0)
    {
        playerLimitRoll = Blaze::Random::getRandomNumber(getGamePlayerSeed());
    }
    uint32_t gamePlayerLimit = gameSize ? gameSize :
        ((playerLimitRoll >= getGamePlayerLowerLimit()) ? playerLimitRoll : getGamePlayerLowerLimit());

    req.getGameCreationData().setMaxPlayerCapacity(static_cast<uint16_t>(gamePlayerLimit));
    req.getGameCreationData().setNetworkTopology(mGameTopology);
    req.getGameCreationData().getGameSettings().setOpenToBrowsing();
    req.getGameCreationData().getGameSettings().setOpenToInvites();
    req.getGameCreationData().getGameSettings().setOpenToJoinByPlayer();
    req.getGameCreationData().getGameSettings().setOpenToMatchmaking();
    req.getGameCreationData().getGameSettings().setRanked();
    
    if (mJoinInProgress)
        req.getGameCreationData().getGameSettings().setJoinInProgressSupported();

    if (mShooterIntelligentDedicatedServer)
    {
        req.setServerNotResetable(true);
    }

    if (mGameTopology != CLIENT_SERVER_DEDICATED)
    {
        req.getGameCreationData().getGameSettings().setHostMigratable();
    }
    else
    {
        req.getGameCreationData().getGameSettings().clearHostMigratable();
    }
    //    TODO - deal with public vs private.
    SlotCapacitiesVector &slotVector = req.getSlotCapacities();
    slotVector[SLOT_PUBLIC_PARTICIPANT] = req.getGameCreationData().getMaxPlayerCapacity();

    req.getCommonGameData().setGameProtocolVersionString(getGameProtocolVersionString());

    if (!inst->getPingSites().empty() && mGameTopology == CLIENT_SERVER_DEDICATED)
    {
        req.setGamePingSiteAlias(inst->getRandomPingSiteAlias());
    }
    
    if (!mGameAttrValues.empty())
    {
        for (AttrValues::const_iterator it = mGameAttrValues.begin(); it != mGameAttrValues.end(); ++it)
        {
            req.getGameCreationData().getGameAttribs()[it->first] = it->second.at(Blaze::Random::getRandomNumber(it->second.size())).first;
        }
    }
}


void GameManagerUtil::startRegisterGame()
{
    // reserve a game slot so other instances won't attempt to call this method if we've reached the max game count.
    mPendingGameCount++;
}

void GameManagerUtil::finishRegisterGame()
{
    mPendingGameCount--;    // game inserted into game list or failure - either way this is no longer a pending game.
}


// Instance calls to create a game
GameData* GameManagerUtil::registerGame(ReplicatedGameData &replGameData, ReplicatedGamePlayerList &replPlayerData)
{
    GameData *gameData = nullptr;
    GameId gameId = replGameData.getGameId();
    GameIdToDataMap::insert_return_type inserter = mGames.insert(gameId);
    if (inserter.second)
    {
        GameId nextAvailJoinGameId = 0;
        if (mNextJoinGameIt != mGames.end())
        {
            nextAvailJoinGameId = mNextJoinGameIt->first;
        }

        gameData = BLAZE_NEW GameData(gameId);
        gameData->init(replGameData);

        //    add players in the roster from this join (there will only be the host unless a dedicated server.)
        ReplicatedGamePlayerList *playerList = &replPlayerData;
        for (ReplicatedGamePlayerList::const_iterator it = playerList->begin(); it != playerList->end(); ++it)
        {
            const ReplicatedGamePlayer *player = *it;
            gameData->addPlayer(*player);
        }
        inserter.first->second = gameData;

        //    initialize the next game iterator for automatic joining.
        if (mNextJoinGameIt == mGames.end())
        {
            mNextJoinGameIt = inserter.first;
        }
        else if (nextAvailJoinGameId != 0)
        {
            mNextJoinGameIt = mGames.find(nextAvailJoinGameId);
        }

        if (gameData->getGameState() == RESETABLE)
        {
            //  Could this conflict with the asynchronous handler?
            mResetableServers++;
        }
        addMetric(METRIC_GAMES_CREATED);

        //BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "GameManagerUtil: registering game (" << gameId << ")");
    }
    else
    {
        gameData = inserter.first->second;
        gameData->mRefCount++;
        if (gameData->getGameReportingId() != replGameData.getGameReportingId())
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].registerGame: Updating game report id (" << gameData->getGameReportingId() << " to " << replGameData.getGameReportingId() << ") for game (" << gameData->getGameId() << ")");
            gameData->setGameReportingId(replGameData.getGameReportingId());
        }
    }

#ifdef TARGET_gamereporting
    getGameReportingUtil().startReport(gameData->getGameReportingId(), gameData->getAttributeMap());
#endif

    return gameData;
}


//    unregisters the game from the util - if game manager destroys the game by some other means than an explicit RPC call.
bool GameManagerUtil::unregisterGame(GameId gameId)
{
    GameIdToDataMap::iterator it = mGames.find(gameId);
    if (it != mGames.end())
    {
        GameData *data = it->second;

    #ifdef TARGET_gamereporting
        getGameReportingUtil().endReport(data->getGameReportingId());
    #endif

        data->mRefCount--;
        if (data->mRefCount == 0)
        {
            GameId nextAvailJoinGameId = 0;
            if (mNextJoinGameIt != mGames.end())
            {
                nextAvailJoinGameId = mNextJoinGameIt->first;
            }
    
            
            if (it->first == nextAvailJoinGameId)
            {
                mNextJoinGameIt = mGames.erase(it);
            }
            else
            {
                mGames.erase(it);
                mNextJoinGameIt = mGames.find(nextAvailJoinGameId);
            }
    
            delete data;
    
            addMetric(METRIC_GAMES_UNREGISTERED);
            //BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "GameManagerUtil: unregistering game (" << gameId << ")");
            return true;
        }
    }
    return false;
}


// Instance calls when advancing game state
void GameManagerUtil::advanceGameState(BlazeRpcError *err, GameManagerSlave *proxy, GameId gameId, GameState newState)
{
    AdvanceGameStateRequest req;
    req.setGameId(gameId);
    req.setNewGameState(newState);

    *err = proxy->advanceGameState(req);
    if (*err != ERR_OK)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].advanceGameState: Advance game state for game(" << gameId << ") failed. Error(" 
                       << (ErrorHelp::getErrorName(*err)) << ")");
    }
}


void GameManagerUtil::setGameSettings(BlazeRpcError *err, GameManagerSlave *proxy, GameId gameId, GameSettings *newSettings)
{
    SetGameSettingsRequest req;
    req.setGameId(gameId);
    req.getGameSettings().setBits(newSettings->getBits());

    *err = proxy->setGameSettings(req);
    if (*err != ERR_OK)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].setGameSettings: Set game settings for game(" << gameId << ") failed. Error(" 
                       << (ErrorHelp::getErrorName(*err)) << ")");
    }
}
#ifdef TARGET_arson
void GameManagerUtil::setGameSettingsArson(BlazeRpcError *err, Arson::ArsonSlave *proxy, GameId gameId, GameSettings *newSettings)
{
    Arson::SetGameSettingsRequest req;
    newSettings->setVirtualized();
    req.getSetGameSettingsRequest().setGameId(gameId);
    req.getSetGameSettingsRequest().getGameSettings().setBits(newSettings->getBits());

    *err = proxy->setGameSettings(req);
    if (*err != ERR_OK)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].setGameSettingsArson: Set game settings via Arson for game(" << gameId << ") failed. Error(" 
            << (ErrorHelp::getErrorName(*err)) << ")");
    }
}
#endif
void GameManagerUtil::udpateGameName(BlazeRpcError *err, GameManagerSlave *proxy, GameId gameId, const char8_t *newName)
{
    UpdateGameNameRequest req;
    req.setGameId(gameId);
    req.setGameName(newName);

    *err = proxy->updateGameName(req);
    if (*err != ERR_OK)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].udpateGameName: Set game name(" << newName << ") for game(" << gameId << ") failed. Error(" 
            << (ErrorHelp::getErrorName(*err)) << ")");
    }
}


void GameManagerUtil::setInstanceJoinGame(GameId gameId, JoinGameRequest& req)
{
    req.setGameId(gameId);
}


void GameManagerUtil::defaultJoinGameRequest(JoinGameRequest& req)
{
    req.getPlayerJoinData().setDefaultSlotType(SLOT_PUBLIC_PARTICIPANT);
    req.getCommonGameData().setGameProtocolVersionString(getGameProtocolVersionString());
}


GameId GameManagerUtil::joinGame(BlazeRpcError *err, GameManagerSlave *proxy, JoinGameResponse *resp, StressInstance *instance, bool joinByUser, const JoinGameRequest *reqOverride)
{
    GameId gameId = pickGame();
    
    if(gameId != 0)
    {    
        gameId = joinGame(err, proxy, gameId, resp, joinByUser, reqOverride);     
    }
    else if (mResetableServers > 0)
    {
        CreateGameRequest req;
        defaultResetGameRequest(instance, req);
      
        //  find any game with attributes
        if (mNextJoinGameIt != mGames.end())
        {
            Collections::AttributeMap::const_iterator itBegin = mNextJoinGameIt->second->getAttributeMap()->begin();
            Collections::AttributeMap::const_iterator itEnd = mNextJoinGameIt->second->getAttributeMap()->end();
            req.getGameCreationData().getGameAttribs().insert(itBegin, itEnd);
        }
        else if (mGames.begin() != mGames.end())
        {
            Collections::AttributeMap::const_iterator itBegin = mGames.begin()->second->getAttributeMap()->begin();
            Collections::AttributeMap::const_iterator itEnd = mGames.begin()->second->getAttributeMap()->end();
            req.getGameCreationData().getGameAttribs().insert(itBegin, itEnd);
        }

        *err = proxy->resetDedicatedServer(req, *resp);

        if (*err == ERR_OK)
        {
            gameId = resp->getGameId();
        }
        else
        {
            BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].joinGame: Failed to reset game (error = " << *err << ")");
        }
    }

    return gameId;
}

GameId GameManagerUtil::joinGame(BlazeRpcError *err, GameManagerSlave *proxy, GameId gameId, JoinGameResponse *resp, bool joinByUser, const JoinGameRequest *reqOverride)
{
    JoinGameRequest req;
    EntryCriteriaError entrycrit;

    GameData *gameData = getGameData(gameId);
    
    if (mJoinRequest != nullptr)
    {
        mJoinRequest->copyInto(req);
    }
    else if (reqOverride != nullptr)
    {
        reqOverride->copyInto(req);
    }
    else
    {
        defaultJoinGameRequest(req);
    }
    if (!joinByUser)
    {
        //  this method is really minimal - investigate why we need a separate method for this.
        setInstanceJoinGame(gameId, req);
    }
    else
    {
        if (gameData != nullptr && gameData->getNumPlayers() == 0)
        {
            *err = ERR_OK;
            return 0;
        }
        size_t playerIndex = (size_t)(Blaze::Random::getRandomNumber(gameData->getNumPlayers()));
        req.setJoinMethod(JOIN_BY_PLAYER);
        req.getUser().setBlazeId(gameData->getPlayerDataByIndex(playerIndex)->mPlayerId);
    }

    if (gameData != nullptr)
    {   
        gameData->mPendingEntryCount++;
    }    

    *err = proxy->joinGame(req, *resp, entrycrit);
   
    GameData *postJoinGameData = getGameData(gameId);
    GameId joinedGameId = 0;
    if(*err == GAMEMANAGER_ERR_PARTICIPANT_SLOTS_FULL || *err == GAMEMANAGER_ERR_SPECTATOR_SLOTS_FULL)
    {
        addMetric(GameManagerUtil::METRIC_JOIN_GAME_FULL);
    }
    
    if (*err == GAMEMANAGER_ERR_INVALID_GAME_ID)
    {
        if (postJoinGameData != nullptr)
        {
            BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].joinGame: Tried to join invalid game ID " << gameId << " - local store of games may be out of sync.");
            //  redundancy to clean out internally stored game map if join failed to due a non-existant GameId
            //  unregisterGame(gameId);
                
        }
        else
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].joinGame: Invalid game ID " << gameId << " to join - game was likely killed during the join send.");
        }
    }
    else if (*err == ERR_OK)
    {
        joinedGameId = resp->getGameId();
    }

    //Get game data again in case game was destroyed while we slept - note we're using the gameId passed in instead of the
    //joined game ID.  it's possible the gameId passed in and the joined game ID could be different if we're joining by player, and the player
    //switched games during the idle.   But we still need to update the pending entry count using the original game ID.    
    if(postJoinGameData != nullptr && gameData != nullptr)
    {
        postJoinGameData->mPendingEntryCount--;
    }

    return joinedGameId;
}


void GameManagerUtil::removePlayer(BlazeRpcError *err, GameManagerSlave *proxy, GameId gameId, PlayerId playerId, PlayerRemovedReason reason)
{
    RemovePlayerRequest req;

    req.setGameId(gameId);
    req.setPlayerId(playerId);
    req.setPlayerRemovedReason(reason);
    
    *err = proxy->removePlayer(req);

    if (*err != ERR_OK)
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].removePlayer: Removing player(" << playerId << ") from game(" << gameId 
                        << ") failed. Error(" << (ErrorHelp::getErrorName(*err)) << ")");
        if (*err == GAMEMANAGER_ERR_PLAYER_NOT_FOUND)
        {
            *err = ERR_OK; 
        }
    }
    else
    {
        addMetric(GameManagerUtil::METRIC_PLAYER_REMOVED);
    }
}


void GameManagerUtil::startMatchmakingScenario(BlazeRpcError *err, GameManagerSlave *proxy, StartMatchmakingScenarioRequest *request, StartMatchmakingScenarioResponse *response, MatchmakingCriteriaError *error)
{
    *err = proxy->startMatchmakingScenario(*request, *response, *error);

    if (*err != ERR_OK)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].startMatchmakingScenario: Start matchmaking failed. Error(" << (ErrorHelp::getErrorName(*err)) 
                       << ") " << error->getErrMessage());
    }
}


void GameManagerUtil::cancelMatchmakingScenario(BlazeRpcError *err, GameManagerSlave *proxy, MatchmakingScenarioId scenarioId)
{
    CancelMatchmakingScenarioRequest req;

    req.setMatchmakingScenarioId(scenarioId);

    *err = proxy->cancelMatchmakingScenario(req);

    if (*err != ERR_OK)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtil].cancelMatchmakingScenario: Cancel matchmaking failed. Error(" << (ErrorHelp::getErrorName(*err)) << ")");
    }
}

//    returns modifyable game data
GameData* GameManagerUtil::getGameData(GameId gid)
{
    GameIdToDataMap::iterator it = mGames.find(gid);
    return (it != mGames.end()) ? it->second : nullptr;
}


GameId GameManagerUtil::pickGame()
{
    GameData *game = pickGameCommon();
    return (game!=nullptr) ? game->getGameId() : 0;
}



GameData* GameManagerUtil::pickGameCommon()
{
    if (mGames.size() == 0)
        return nullptr;

    if (mNextJoinGameIt == mGames.end()) 
    {
        mNextJoinGameIt = mGames.begin();
    }

    GameIdToDataMap::iterator initialIt = mNextJoinGameIt;


    //    find a game in the game map with room for a player to join.
    // First choice is where our next join pointer is at usually
    // Set to newly created games.
    GameData *pickedGame = mNextJoinGameIt->second;
    if (pickedGame == nullptr)
    {
        return nullptr;
    }

    while (pickedGame->queryJoinGame(mJoinInProgress) <= QUERY_JOIN_NO_ROOM)
    {
        ++mNextJoinGameIt;
        if (mNextJoinGameIt == mGames.end())
        {
            mNextJoinGameIt = mGames.begin();
        }

        // have we visited all games?
        if (mNextJoinGameIt == initialIt)
        {
            if (pickedGame->queryJoinGame(mJoinInProgress) <= QUERY_JOIN_NO_ROOM)
            {
                return nullptr;
            }
            else
            {
                break;
            }            
        }

        pickedGame = mNextJoinGameIt->second;
    }
    return pickedGame;
}


bool GameManagerUtil::queryCreateNewGame() const
{
    size_t estimatedGameCount = mPendingGameCount + mGames.size();
    if (estimatedGameCount >= mMaxGames)
        return false;

    return true;
}

///////////////////////////////////////////////////////////////////////////////
//    GameManagerUtilInstance
///////////////////////////////////////////////////////////////////////////////

GameManagerUtilInstance::GameManagerUtilInstance(GameManagerUtil *util) :
    mUtil(util),
#ifdef TARGET_gamereporting
    mGameReportingUtilInstance(nullptr),
#endif
    mStressInst(nullptr),
    mGameManagerProxy(nullptr),
#ifdef TARGET_arson
    mArsonSlave(nullptr),
#endif
#ifdef TARGET_censusdata
    mCensusDataProxy(nullptr),
#endif
    mGameId(Blaze::GameManager::INVALID_GAME_ID),
    mRole(GameManagerUtil::ROLE_NONE),
    mCyclesLeft(0),
    mPaused(false),
    mJoinByUser(false),
    mPostGameMode(POST_GAME_MODE_AUTO),
    mGameTimer((uint32_t)(-1)),
    mPostGameCountdown(0),
    mSetGameSettingsRequest(nullptr),
    mIsWaitingForGameInfo(false),
    mIsWaitingForGameStateNotification(false),
    mIsSubscribedCensusData(false),
    mPrimaryPresenceGameId(Blaze::GameManager::INVALID_GAME_ID)
{
    mTotalPacketsSent = mTotalPacketsRecv = 0;

    mUtil->incrementRole(mRole);

#ifdef TARGET_gamereporting
    if (mUtil->getGameReportingUtil().getSubmitReports())
    {
        mGameReportingUtilInstance = BLAZE_NEW GameReportingUtilInstance(&mUtil->getGameReportingUtil());
    }
#endif

    clearUpdateGameNameRequest();
}


GameManagerUtilInstance::~GameManagerUtilInstance()
{
#ifdef TARGET_gamereporting
    if (mGameReportingUtilInstance != nullptr)
    {
        delete mGameReportingUtilInstance;
        mGameReportingUtilInstance = nullptr;
    }
#endif
    setStressInstance(nullptr);
    mUtil->decrementRole(mRole);
}


void GameManagerUtilInstance::setStressInstance(StressInstance* inst) 
{
    if (inst == nullptr && mStressInst != nullptr)
    {
        StressConnection *conn = mStressInst->getConnection();
#ifdef TARGET_censusdata
        conn->removeAsyncHandler(CensusData::CensusDataSlave::COMPONENT_ID, CensusData::CensusDataSlave::NOTIFY_NOTIFYSERVERCENSUSDATA, this);
#endif
        conn->removeAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMESETUP, this);
        conn->removeAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINING, this);
        conn->removeAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS, this);
        conn->removeAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINCOMPLETED, this);
        conn->removeAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLAYERREMOVED, this);
        conn->removeAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMESTATECHANGE, this);
        conn->removeAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMEREPORTINGIDCHANGE, this);
        conn->removeAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMERESET, this);
        conn->removeAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYHOSTMIGRATIONSTART, this);
        conn->removeAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYHOSTMIGRATIONFINISHED, this);
        conn->removeAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLATFORMHOSTINITIALIZED, this);
        conn->removeAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMESETTINGSCHANGE, this);
        conn->removeAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLAYERCLAIMINGRESERVATION, this);

        if (mGameManagerProxy != nullptr)
        {
            delete mGameManagerProxy;
            mGameManagerProxy = nullptr;
        }
#ifdef TARGET_arson
        if(mArsonSlave != nullptr)
        {
            delete mArsonSlave;
            mArsonSlave = nullptr;
        }
#endif
#ifdef TARGET_censusdata
        if (mCensusDataProxy != nullptr)
        {
            delete mCensusDataProxy;
            mCensusDataProxy = nullptr;
        }
#endif

        mNotificationCbMap.clear();
        return;
    }
    if (inst != nullptr)
    {
        StressConnection *conn = inst->getConnection();

        mGameManagerProxy = BLAZE_NEW GameManagerSlave(*conn);
#ifdef TARGET_arson
        mArsonSlave = BLAZE_NEW Arson::ArsonSlave(*conn);
#endif
#ifdef TARGET_censusdata
        mCensusDataProxy = BLAZE_NEW CensusData::CensusDataSlave(*conn);
#endif

        //    this instance has joined a game.
        conn->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMESETUP, this);
        //    someone is joining the game the instance is already in.
        conn->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINING, this);
        conn->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLAYERCLAIMINGRESERVATION, this); 
        //    a joining player into the game has fully connected
        conn->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINCOMPLETED, this);
        //    a player left the game
        conn->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLAYERREMOVED, this);
        //  game state changed
        conn->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMESTATECHANGE, this);
        conn->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMEREPORTINGIDCHANGE, this);
        //    resetting a dedicated server game
        conn->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMERESET, this);
        //    host migration starts
        conn->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYHOSTMIGRATIONSTART, this);
        //    host migration ends
        conn->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYHOSTMIGRATIONFINISHED, this);
        // game settings update
        conn->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMESETTINGSCHANGE, this);
        // platform host settled
        conn->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLATFORMHOSTINITIALIZED, this);
        
        conn->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS, this);
        // census data updates
#ifdef TARGET_censusdata
        conn->addAsyncHandler(CensusData::CensusDataSlave::COMPONENT_ID, CensusData::CensusDataSlave::NOTIFY_NOTIFYSERVERCENSUSDATA, this);
#endif
        if (!inst->getOwner()->getUseServerQosSettings() && mUtil->getConfiguredPingSites() != nullptr)
        {
            inst->getPingSites().clear();
            for (size_t i = 0; i < mUtil->getConfiguredPingSites()->getSize(); ++i)
                inst->getPingSites().push_back(mUtil->getConfiguredPingSites()->at(i));
        }
    }

    mStressInst = inst;

#ifdef TARGET_gamereporting
    if (mGameReportingUtilInstance != nullptr)
    {
        mGameReportingUtilInstance->setStressInstance(mStressInst);
    }
#endif
}


void GameManagerUtilInstance::setRole(GameManagerUtil::Role role)
{
    mUtil->decrementRole(mRole);
    mRole = role;
    mUtil->incrementRole(mRole);
}

void GameManagerUtilInstance::endRole()
{
    mCyclesLeft = 0;
}

void GameManagerUtilInstance::setGameId(GameId gameId)
{
    EA_ASSERT(mRole == GameManagerUtil::ROLE_NONE);
    mGameId = gameId;
}


BlazeId GameManagerUtilInstance::getBlazeId() const 
{ 
    return mStressInst->getLogin()->getUserLoginInfo()->getBlazeId(); 
}


void GameManagerUtilInstance::finishPostGame()
{
    if (mPostGameMode == POST_GAME_MODE_CUSTOM)
    {
        resumeRole();
    }
}


//    updates the game instance belonging to its parent stress instance.
BlazeRpcError GameManagerUtilInstance::execute()
{
    //  always increment local game timer as it indicates how long the instance has been in a game.
    if (mGameId != 0)
    {
        if (mGameTimer != (uint32_t)(-1))
        {
            ++mGameTimer;
        }
        if (mPostGameCountdown > 0)
        {
            --mPostGameCountdown;
        }

        checkAndSendTelemetryReport();
    }

    if (mRole == GameManagerUtil::ROLE_NONE || mPaused)
    {
        return ERR_OK;
    }
    if (mCyclesLeft > 0)
    {
        mCyclesLeft--;
    }

    BlazeRpcError err = ERR_OK;
    PlayerId myBlazeId = (PlayerId)mStressInst->getLogin()->getUserLoginInfo()->getBlazeId();

    if (mGameId != 0)
    {
        GameData *gamedata = mUtil->getGameData(mGameId);

        /*
            Randomized automation
        */
        int32_t roll = Blaze::Random::getRandomNumber(100);
        if (roll < (mUtil->getGameSettingsRandomUpdates() * 100))
        {        
            if (gamedata != nullptr)
            {
                GameSettings *gameSettings = gamedata->getGameSettings();
                gameSettings->clearRanked();
                mSetGameSettingsRequest = gameSettings;
            }
        }
       
        /*
            Game changes
        */
        // Host only actions
        if (gamedata != nullptr && (gamedata->isHost(getStressInstance()->getLogin()->getUserLoginInfo()->getBlazeId()) ||
            gamedata->isPlatformHost(getStressInstance()->getLogin()->getUserLoginInfo()->getBlazeId())) )
        {
            if (mSetGameSettingsRequest != nullptr)
            {
                if(gamedata->getGameSettings()->getVirtualized())
                {
#ifdef TARGET_arson
                    mUtil->setGameSettingsArson(&err, mArsonSlave, mGameId, mSetGameSettingsRequest);
#else
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].execute: setGameSettingsArson cannot be used when arson is not included.");
#endif
                }
                else
                {
                    mUtil->setGameSettings(&err, mGameManagerProxy, mGameId, mSetGameSettingsRequest);
                }
                
                mSetGameSettingsRequest = nullptr;
                if (err != ERR_OK)
                {
                    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].execute: setGameSettings failed.");
                    return err;
                }
            }

            // Rename game
            err = updateGameName();
            if (err != ERR_OK)
                return err;

            // Update external session status
            err = updateGameExternalStatus();
            if (err != ERR_OK)
                return err;
        }
    }

    // Handle basic roles for the instance

    // Note: Currently possible for role to change between executes.  
    // I.E. ROLE_HOST may not actually be the current host of the game
    switch (mRole)
    {
    case GameManagerUtil::ROLE_HOST:
        {
            if (mGameId == 0)
            {
                //    attempt to create a new game
                if (mUtil->queryCreateNewGame())
                {
                    mUtil->startRegisterGame();
                    
                    // clear is waiting for game info will get called after we receive
                    // the replicated game data from the server, either
                    // either on the response for a dedicated server or in the join
                    // game notification for peer to peer games.
                    // TODO: need a timeout in case we never get the join game notification.
                    setIsWaitingForGameInfo();

                    CreateGameResponse resp;

                    const CreateGameRequest *customReq = mUtil->getCreateGameRequestOverride();
                    CreateGameRequest *req = BLAZE_NEW CreateGameRequest();

                    // Default or use custom defined create game request.
                    BlazeRpcError error = ERR_OK;
                    if (customReq == nullptr)
                    {
                        mUtil->setInstanceCreateGame(mStressInst, *req);
                        mUtil->defaultCreateGameRequest(mStressInst, *req);
                        mCreateGameRequestCb(req);
                        req->setPersistedGameId(mStressInst->getLogin()->getUserLoginInfo()->getPersonaDetails().getDisplayName());
                        error = mGameManagerProxy->createGame(*req, resp);
                        if (error == Blaze::ERR_OK)
                        {
                            BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].execute: Created game of " << req->getGameCreationData().getMaxPlayerCapacity() 
                                << " players with game id: " << resp.getGameId() << " and ping site: " << req->getGamePingSiteAlias());
                        }
                    }
                    else
                    {
                        customReq->copyInto(*req);
                        mUtil->setInstanceCreateGame(mStressInst, *req);
                        error = mGameManagerProxy->createGame(*req, resp);
                    }
                    delete req;

                    mUtil->finishRegisterGame();

                    if (error != ERR_OK)
                    {
                        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].execute: CreateGame failed.");
                        clearIsWaitingForGameInfo();
                        return error;
                    }
                    
                    mGameId = resp.getGameId();

                    mCyclesLeft = mUtil->getGameLifespanCycles() + mUtil->getGameLifespanDeviation() - (mUtil->getGameLifespanDeviation() > 0 ? (Blaze::Random::getRandomNumber(mUtil->getGameLifespanDeviation()*2)) : 0);
                }
            }
            else
            {
                if (isWaitingForGameInfo())
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].execute: Game host is waiting for game info.");
                }
                else
                {
                    GameData *game = mUtil->getGameData(mGameId);
                    if (game == nullptr)
                    {
                        mGameId = 0;
                    }
                    else if (mCyclesLeft > 0)  
                    {
                        //  in-game logic.
                        if (game->getGameState() == MIGRATING)
                        {
                            // let the magic happen
                            BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].execute: Game host is migrating.");
                        }
                        else if (game->getGameState() == INITIALIZING && mUtil->getShooterIntelligentDedicatedServer())
                        {
                                advanceGameState(PRE_GAME);
                        }
                        else if (game->getGameState() == PRE_GAME)
                        {
                            // never expire pre game state unless enough players joined
                            if (mUtil->getShooterIntelligentDedicatedServer())
                                mCyclesLeft++;

                            if (game->getNumPlayers() >= game->getPlayerCapacity() / 4)
                            {
                                advanceGameState(IN_GAME);
                            }
                            else if (mGameTimer >= mUtil->getGameStartTimer())
                            {
                                if (game->getNumPlayers() >= mUtil->getGamePlayerMinRequired())
                                {
                                    advanceGameState(IN_GAME);
                                }
                                else if (!mUtil->getShooterIntelligentDedicatedServer())
                                {
                                    //  prematurely abort game as minimum player threshold was not hit
                                    mCyclesLeft = 0;
                                    mUtil->addMetric(GameManagerUtil::METRIC_GAMES_ABORTED);
                                }
                            }
                        }
                        else if (game->getGameState() == IN_GAME && game->getNumPlayers() < mUtil->getGamePlayerMinRequired())
                        {
                            if (!mUtil->getShooterIntelligentDedicatedServer())
                            {
                                //  ending game early.
                                mUtil->addMetric(GameManagerUtil::METRIC_GAMES_EARLY_END);
                                mCyclesLeft = 0;
                            }
                        }
                    }            
                    else if (mCyclesLeft == 0 || game->getGameState() == POST_GAME)
                    {
                        //  put game into post game state to prevent people from joining.
                        game = mUtil->getGameData(mGameId);
                        if (game == nullptr)
                        {
                            mGameId = 0;
                        }
                        else
                        {                    
                            if (game->getGameState() == IN_GAME)
                            {                        
                                advanceGameState(POST_GAME);
                                mCyclesLeft++; // wait a cycle for the game state change
                                if (mPostGameMode == POST_GAME_MODE_CUSTOM)
                                {
                                    //  wait until called calls "finishGame"
                                    pauseRole();
                                }
                            }
                            else if ((game->getGameState() == POST_GAME && mPostGameCountdown == 0) || game->getGameState() != POST_GAME)
                            {
                                if (game->getTopology() == CLIENT_SERVER_DEDICATED)
                                {
                                    BlazeRpcError error = ERR_OK;
                                    if (game->getGameSettings()->getVirtualized())
                                    {
                                        //  throw everyone out of the game 
                                        for (size_t i = 0; i < game->getNumPlayers(); ++i)
                                        {
                                            mUtil->removePlayer(&error, mGameManagerProxy, game->getGameId(), game->getPlayerDataByIndex(i)->mPlayerId, PLAYER_KICKED);
                                        }
                                        //eject host if virtual
                                        EjectHostRequest req;
                                        req.setGameId(game->getGameId());
                                        error = mGameManagerProxy->ejectHost(req);
                                        if (error != ERR_OK)
                                        {
                                            BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].execute: ejectHost failed for id=" << mGameId << ".");
                                            mGameId = 0;
                                            return error;
                                        }
                                        else 
                                        {
                                            mUtil->addMetric(GameManagerUtil::METRIC_GAMES_CYCLED);
                                            mGameId = 0;
                                        }

                                    }
                                    else if (!mUtil->getShooterIntelligentDedicatedServer())
                                    {
                                        //  return game to pool if not resetable.

                                        if (game->getGameState() != RESETABLE)
                                        {
                                            //  throw everyone out of the game 
                                            for (size_t i = 0; i < game->getNumPlayers(); ++i)
                                            {
                                                mUtil->removePlayer(&error, mGameManagerProxy, game->getGameId(), game->getPlayerDataByIndex(i)->mPlayerId, PLAYER_KICKED);
                                            }
                                            ReturnDedicatedServerToPoolRequest req;
                                            req.setGameId(game->getGameId());
                                            error = mGameManagerProxy->returnDedicatedServerToPool(req);
                                            if (error == ERR_OK)
                                            {
                                                GameData *dsGame = mUtil->getGameData(mGameId);   // just in case the game died after the fiber call.
                                                //  is this valid?   Shouldn't this come down in some kind of notification?
                                                dsGame->setGameState(RESETABLE);
                                                mUtil->incrementResetableServers();
                                                mUtil->addMetric(GameManagerUtil::METRIC_GAMES_RETURNED);
                                                mGameTimer = (uint32_t)(-1);
                                            }
                                        }

                                        //  wait a few more cycles until attempting to return server to pool.
                                        mCyclesLeft = mUtil->getGameLifespanCycles() + mUtil->getGameLifespanDeviation() - (mUtil->getGameLifespanDeviation() > 0 ? (Blaze::Random::getRandomNumber(mUtil->getGameLifespanDeviation()*2)) : 0);
                                    }

                                    if (error != ERR_OK)
                                    {
                                        return error;
                                    }
                 
                                }
                                else
                                {
                                    DestroyGameRequest req;
                                    DestroyGameResponse resp;
                                    req.setGameId(mGameId);
                                    BlazeRpcError error = mGameManagerProxy->destroyGame(req, resp);
                                    if (error != ERR_OK)
                                    {
                                        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].execute: destroyGame failed for id=" << mGameId << ".");
                                        mGameId = 0;
                                        return error;
                                    }
                                    else 
                                    {
                                        mUtil->addMetric(GameManagerUtil::METRIC_GAMES_CYCLED);
                                        mGameId = 0;
                                    }
                                }
                            }
                        }
                    }
                }
            }                
        }
        break;
    case GameManagerUtil::ROLE_JOINER:
        {
            if(mGameId == 0)
            {
                JoinGameResponse resp;
                GameId gameId = mUtil->joinGame(&err, mGameManagerProxy, &resp, getStressInstance(), mJoinByUser);
                if(gameId != 0)
                {                    
                    if(err != ERR_OK && (err != GAMEMANAGER_ERR_SPECTATOR_SLOTS_FULL || err != GAMEMANAGER_ERR_PARTICIPANT_SLOTS_FULL))
                    {
                        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].execute: joinGame failed (id=" << gameId << ")");
                        return err;
                    }
                    mGameId = resp.getGameId();
                    mCyclesLeft = mUtil->getGamePlayerLifespan() + mUtil->getGamePlayerLifespanDeviation() - (mUtil->getGamePlayerLifespanDeviation() > 0 ? (Blaze::Random::getRandomNumber(mUtil->getGamePlayerLifespanDeviation()*2)) : 0);
                }
                // TODO: should we record join game failures?
            }
            else if (mCyclesLeft <= 0)
            {
                GameData *currentGame = mUtil->getGameData(mGameId);
                GameId myGameId = mGameId;

                if (currentGame != nullptr)
                {
                            //  time for this player to leave.
                     mUtil->removePlayer(&err, mGameManagerProxy, myGameId, myBlazeId, PLAYER_LEFT);
                }

                if (mGameId != 0)
                {
                    mGameId = 0;                    // also cleared in the PlayerRemoved notification.
                    
                    if (err != ERR_OK)
                    {
                        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].execute: removePlayer failed (gameid=" << myGameId << ")");
                        return err;
                    }
                }
                else
                {                    
                    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].execute: Game id cleared during removePlayer");
                }
            }
        }
        break;
    case GameManagerUtil::ROLE_VIRTUAL_GAME_CREATOR:
        {
#ifdef TARGET_arson
            if(mGameId == 0)
            {
                //    attempt to create a new game
                if (mUtil->queryCreateNewGame())
                {
                    mUtil->startRegisterGame();

                    // clear is waiting for game info will get called after we receive
                    // the replicated game data from the server, either
                    // either on the response for a dedicated server or in the join
                    // game notification for peer to peer games.
                    // TODO: need a timeout in case we never get the join game notification.
                    setIsWaitingForGameInfo();

                    CreateGameResponse resp;

                    const CreateGameRequest *customReq = mUtil->getCreateGameRequestOverride();
                    Arson::ArsonCreateGameRequest *req = BLAZE_NEW Arson::ArsonCreateGameRequest();

                    // Default or use custom defined create game request.
                    BlazeRpcError error = ERR_OK;
                    if (customReq == nullptr)
                    {
                        mUtil->setInstanceCreateGame(mStressInst, req->getCreateGameRequest());
                        mUtil->defaultCreateGameRequest(mStressInst, req->getCreateGameRequest());
                        req->getCreateGameRequest().getGameCreationData().getGameSettings().setVirtualized();


                        mCreateGameRequestCb(&(req->getCreateGameRequest()));

                        if(mUtil->getUseArson())
                        {
                            error = mArsonSlave->createGame(*req, resp);
                        }
                        else
                        {
                            error = mGameManagerProxy->createGame(req->getCreateGameRequest(), resp);
                        }
                    }
                    else
                    {
                        customReq->copyInto(req->getCreateGameRequest());
                        mUtil->setInstanceCreateGame(mStressInst, req->getCreateGameRequest());
                        req->getCreateGameRequest().getGameCreationData().getGameSettings().setVirtualized();

                        if(mUtil->getUseArson())
                        {
                            error = mArsonSlave->createGame(*req, resp);
                        }
                        else
                        {
                            error = mGameManagerProxy->createGame(req->getCreateGameRequest(), resp);
                        }
                    }
                    delete req;

                    mUtil->finishRegisterGame();
                    clearIsWaitingForGameInfo();

                    if (error != ERR_OK)
                    {
                        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].execute: createGame failed.");
                        return error;
                    }

                    mGameId = resp.getGameId();

                    GetFullGameDataRequest *gameDataRequest = BLAZE_NEW GetFullGameDataRequest();
                    GetFullGameDataResponse gameDataResponse;

                    gameDataRequest->getGameIdList().push_back(mGameId);
                    mGameManagerProxy->getFullGameData(*gameDataRequest, gameDataResponse);

                    if(!gameDataResponse.getGames().empty())
                    {
                        gameDataResponse.getGames().front()->getGame();
                        mUtil->registerGame(gameDataResponse.getGames().front()->getGame(), gameDataResponse.getGames().front()->getGameRoster());
                    }

                    delete gameDataRequest;

                    mCyclesLeft = mUtil->getGameLifespanCycles() + mUtil->getGameLifespanDeviation() - (mUtil->getGameLifespanDeviation() > 0 ? (Blaze::Random::getRandomNumber(mUtil->getGameLifespanDeviation()*2)) : 0);
                }

            }
            else if (mCyclesLeft == 0)
            {
                Arson::DestroyGameRequest req;
                DestroyGameResponse resp;
                req.getDestroyGameRequest().setGameId(mGameId);
                BlazeRpcError error = ERR_OK;
                if(mUtil->getUseArson())
                {
                    error = mArsonSlave->destroyGame(req, resp);
                }
                else
                {
                    error = mGameManagerProxy->destroyGame(req.getDestroyGameRequest(), resp);
                }
                
                // update primary external session
                scheduleUpdateGamePresenceForLocalUser(mGameId, UPDATE_PRESENCE_REASON_GAME_LEAVE, Blaze::GameManager::GAME_DESTROYED);

                mUtil->unregisterGame(mGameId);

                if (error != ERR_OK)
                {
                    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].execute: destroyGame failed for id=" << mGameId << ".");
                    mGameId = 0;
                    return error;
                }
                else 
                {
                    mUtil->addMetric(GameManagerUtil::METRIC_GAMES_CYCLED);
                    mGameId = 0;
                }
            }
#else
            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].execute: Virtualized games are not supported without Arson");
#endif
           }
        break;

    
    default:
        break;
    }

    return ERR_OK;
}

BlazeRpcError GameManagerUtilInstance::checkAndSendTelemetryReport()
{
    BlazeRpcError err = ERR_OK;

    GameData *gameData = mUtil->getGameData(mGameId);

    if (gameData == nullptr)
        return err;

    TimeValue now = TimeValue::getTimeOfDay();
    if ((gameData->mNextTelemetryTime > 0) && (gameData->mNextTelemetryTime < now))
    {
        if (gameData->mGameState == IN_GAME)
        {
            BlazeId me = getStressInstance()->getLogin()->getUserLoginInfo()->getBlazeId();
            PlayerData* myPlayerData = gameData->getPlayerData(me);

            if (myPlayerData == nullptr)
                return err; // player already left the game, don't send telemetry reports

            bool isHost = gameData->isHost(me);

            TelemetryReportRequest request;
            request.setGameId(gameData->getGameId());
            GameNetworkTopology topology = gameData->getTopology();
            request.setNetworkTopology(topology);
            request.setLocalConnGroupId(myPlayerData->mConnGroupId);

            if (!isHost && ((topology == CLIENT_SERVER_DEDICATED) || (topology == CLIENT_SERVER_PEER_HOSTED)))
            {
                TelemetryReport *report = request.getTelemetryReports().pull_back();
                report->setRemoteConnGroupId(gameData->mHostConnGroupId);

                randomizeLatencyData(report, *gameData);
            }
            else // either this is a mesh topology or the local user is the host of the hosted topology.
            {    // most topologies only need 2 endpoints for useful info to be collected
                uint16_t minEndpointCheckValue = 2;
                // for hosted topologies, the only information that matters is the connection to the host
                size_t numPlayers = gameData->getNumPlayers();
                if (numPlayers >= minEndpointCheckValue)
                {
                    for(size_t idx = 0; idx < numPlayers; idx++)
                    {
                        PlayerData* playerData = gameData->getPlayerDataByIndex(idx);
                        if (playerData->mPlayerId != me)
                        {
                            TelemetryReport *report = request.getTelemetryReports().pull_back();
                            report->setRemoteConnGroupId(playerData->mConnGroupId);

                            randomizeLatencyData(report, *gameData);
                        }
                    }
                }
            }

            if (request.getTelemetryReports().size() > 0)
                err = mGameManagerProxy->reportTelemetry(request);
        }

        gameData = mUtil->getGameData(mGameId);
        if (gameData != nullptr) // gameData might be invalid if game got destroyed while we were waiting for RPC to finish
            setNextQosTelemetryTime(*gameData);
    }
    return err;
}

//  if instance logs out, invoke this method to reset state
void GameManagerUtilInstance::onLogout()
{
    if (mGameId != 0)
    {
        // update primary external session
        scheduleUpdateGamePresenceForLocalUser(mGameId, UPDATE_PRESENCE_REASON_GAME_LEAVE, Blaze::GameManager::BLAZESERVER_CONN_LOST);

        mUtil->unregisterGame(mGameId);
        mGameId = 0;
    }
    mCyclesLeft = 0;
    mPaused = false;
    mJoinByUser = false;
    mPostGameMode = POST_GAME_MODE_AUTO;
    mGameTimer = (uint32_t)(-1);
    mPostGameCountdown = 0;
    mIsWaitingForGameStateNotification = false;
}


void GameManagerUtilInstance::setNotificationCb(GameManagerSlave::NotificationType type, const AsyncNotifyCb& cb)
{
    if (mStressInst == nullptr)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].setNotificationCb: Unable to set notification callback on uninitialized instance.");
        return;
    }
    mStressInst->getConnection()->addAsyncHandler(GameManagerSlave::COMPONENT_ID, (uint16_t)type, this);
    mNotificationCbMap[type] = cb;
}


void GameManagerUtilInstance::clearNotificationCb(GameManagerSlave::NotificationType type)
{
    mNotificationCbMap[type] = AsyncNotifyCb();
    if (mStressInst != nullptr)
    {
        mStressInst->getConnection()->removeAsyncHandler(GameManagerSlave::COMPONENT_ID, (uint16_t)type, this);
    }
}


void GameManagerUtilInstance::scheduleCustomNotification(GameManagerSlave::NotificationType type, EA::TDF::Tdf *notification)
{
    NotificationCbMap::const_iterator cit = mNotificationCbMap.find(type);
    if (cit != mNotificationCbMap.end())
    {
        if ((*cit).second.isValid())
        {
            EA::TDF::TdfPtr cbTdf = notification->clone();
            gSelector->scheduleFiberCall(this,  &GameManagerUtilInstance::runNotificationCb, type, cbTdf, "GameManagerUtilInstance::runNotificationCb");
        }
    }
}

void GameManagerUtilInstance::runNotificationCb(GameManagerSlave::NotificationType type, EA::TDF::TdfPtr tdf)
{
    StressLogContextOverride stressLogContextOverride(getIdent(), getBlazeId());

    NotificationCbMap::const_iterator cit = mNotificationCbMap.find(type);
    if (cit != mNotificationCbMap.end())
    {
        const AsyncNotifyCb &cb = (*cit).second;        
        cb(tdf);
    }
}



///////////////////////////////////////////////////////////////////////////////
//    asynchronous notification event handler.
void GameManagerUtilInstance::onAsync(uint16_t component, uint16_t iType, RawBuffer* payload)
{
    Heat2Decoder decoder;
#ifdef TARGET_censusdata
    //  Census Data Updates
    if (component == CensusData::CensusDataSlave::COMPONENT_ID)
    {
        CensusData::CensusDataSlave::NotificationType type = (CensusData::CensusDataSlave::NotificationType)iType;
        switch (type)
        {
        case CensusData::CensusDataSlave::NOTIFY_NOTIFYSERVERCENSUSDATA:
            {
                CensusData::NotifyServerCensusDataItem notify;
                if (decoder.decode(*payload, notify) == ERR_OK)
                {
                    mUtil->addMetric(GameManagerUtil::METRIC_CENSUS_UPDATES);
                }
                break;
            }
        }

        return;
    }
#endif

    if(component != GameManagerSlave::COMPONENT_ID)
    {
        return;
    }

    //  GameManager async updates
    GameManagerSlave::NotificationType type = (GameManagerSlave::NotificationType)iType;

    switch (type)
    {
    case GameManagerSlave::NOTIFY_NOTIFYGAMESETUP:
    case GameManagerSlave::NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS:
        {
            NotifyGameSetup notify;
            if (decoder.decode(*payload, notify) != ERR_OK)
            {
                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Failed to decode notification(" << type << ")");
            }
            else
            {
                // Ignore game group notifications
                if (notify.getGameData().getGameType() != GAME_TYPE_GAMESESSION)
                    return;

                if (mGameId != INVALID_GAME_ID && mGameId != notify.getGameData().getGameId())
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Receiving notification(" << type << ") for game (" 
                        << notify.getGameData().getGameId() << ") though the instance already is in a game (" << mGameId << ")");
                }

                ReplicatedGamePlayerList &roster = notify.getGameRoster();

                bool isHost = (notify.getGameData().getTopologyHostInfo().getPlayerId() == getBlazeId());
                bool isPlatformHost = (notify.getGameData().getPlatformHostInfo().getPlayerId() == getBlazeId());
                bool isClientServer = (notify.getGameData().getNetworkTopology() == CLIENT_SERVER_DEDICATED) || (notify.getGameData().getNetworkTopology() == CLIENT_SERVER_PEER_HOSTED);

                GameData *gameData = mUtil->registerGame(notify.getGameData(), notify.getGameRoster());
                gameData->setQosTelemetryInterval(notify.getQosTelemetryInterval());
                setNextQosTelemetryTime(*gameData);

                // TODO: clearIsWaitingForGameInfo should be called inside register game.
                clearIsWaitingForGameInfo();

                //    the util instance needs these values to be set for its notification handlers to work properly.
                mGameId = notify.getGameData().getGameId();
                mGameTimer = 0;     // reset current local game timer.

                if ( (isHost || isPlatformHost) && type != GameManagerSlave::NOTIFY_NOTIFYJOININGPLAYERINITIATECONNECTIONS)
                {
                    //    schedule a finalizeGameCreation call without being the topology host
                    Fiber::CreateParams params(Fiber::STACK_SMALL);
                    gSelector->scheduleFiberCall(this, &GameManagerUtilInstance::finalizeGameCreation, notify.getGameData().getGameId(), isPlatformHost, isHost,
                        "GameManagerUtilInstance::finalizeGameCreation", params);

                }
                else
                {
                    //    count as a join
                    mUtil->addMetric(GameManagerUtil::METRIC_PLAYER_LOCAL_JOIN);
                }

                if (isClientServer)
                {
                    // host does not send update mesh connection (just finalizes game creation above)
                    if (!isHost)
                    {
                        //    schedule a updateHostConnection call
                        ReplicatedGamePlayerList &rosters = notify.getGameRoster();
                        if (rosters.size() > 0)
                        {
                            ConnectionGroupId sourceGroupId = 0;
                            //  update player roster in game object
                            bool isReserved = false;
                            for (ReplicatedGamePlayerList::const_iterator cit = rosters.begin(), citEnd = rosters.end(); cit != citEnd; ++cit)
                            {
                                //  sync local player roster with the latest info.
                                PlayerId targetPlayerId = (*cit)->getPlayerId();
                                if (targetPlayerId == getBlazeId())
                                {
                                    sourceGroupId = (*cit)->getConnectionGroupId();
                                    if ((*cit)->getPlayerState() == RESERVED)
                                    {
                                        isReserved = true;
                                    }
                                }
                            }
                            //    schedule a updateHostConnection call
                            if (!isReserved)
                            {
                                gSelector->scheduleFiberCall(this, &GameManagerUtilInstance::updateHostConnection, 
                                    notify.getGameData().getGameId(), sourceGroupId, 
                                    notify.getGameData().getTopologyHostInfo().getConnectionGroupId(), CONNECTED,
                                    "GameManagerUtilInstance::updateHostConnection");
                            }

                        }
                    }
                }
                
                //  refresh local copy of roster.
                //  establish network connections with other players for non-dedicated server games. 
                if (roster.size() > 0)
                {
                    ConnectionGroupId sourceGroupId = 0;
                    //  update player roster in game object
                    bool isReserved = false;
                    for (ReplicatedGamePlayerList::const_iterator cit = roster.begin(), citEnd = roster.end(); cit != citEnd; ++cit)
                    {
                        //  sync local player roster with the latest info.
                        gameData->addPlayer(**cit);
                        PlayerId targetPlayerId = (*cit)->getPlayerId();
                        if (targetPlayerId == getBlazeId())
                        {
                            sourceGroupId = (*cit)->getConnectionGroupId();
                            if ((*cit)->getPlayerState() == RESERVED)
                            {
                                isReserved = true;
                            }
                        }
                    }
                    if (!isClientServer && !isReserved)
                    {
                        for (ReplicatedGamePlayerList::const_iterator cit = roster.begin(), citEnd = roster.end(); cit != citEnd; ++cit)
                        {
                            //  note, we don't skip local player as BlazeSDK actually sends an update for player to itself.
                            ConnectionGroupId targetConnGroupId = (*cit)->getConnectionGroupId();

                            gSelector->scheduleFiberCall(this, &GameManagerUtilInstance::updateHostConnection, 
                                notify.getGameData().getGameId(), sourceGroupId, 
                                targetConnGroupId, CONNECTED,
                                "GameManagerUtilInstance::updateHostConnection");
                        }
                    }
                }

                //  non-dedicated server hosts set pre-game here.  dedicated servers do so on the reset. intelligent dedicated servers do it here too
                if (isHost && (mUtil->getShooterIntelligentDedicatedServer() || gameData->getTopology() != CLIENT_SERVER_DEDICATED))
                {
                    //    advance to pre-game state for joining
                    Selector::FiberCallParams params(Fiber::STACK_SMALL);
                    gSelector->scheduleFiberCall(this, &GameManagerUtilInstance::advanceGameState, PRE_GAME, "GameManagerUtilInstance::advanceGameState", params);
                }

                //  subscribe to census data.
                if (mUtil->getCensusSubscribe())
                {
                    Fiber::CreateParams params(Fiber::STACK_SMALL);
                    gSelector->scheduleFiberCall(this, &GameManagerUtilInstance::censusDataSubscribe, "GameManagerUtilInstance::censusDataSubscribe", params);
                }

                scheduleCustomNotification( type, &notify);
            }
        }
        break;
    case GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINING:
    case GameManagerSlave::NOTIFY_NOTIFYPLAYERCLAIMINGRESERVATION:
        {
            NotifyPlayerJoining notify;
            if (decoder.decode(*payload, notify) != ERR_OK)
            {
                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Failed to decode notification(" << type << ")");
            }
            else 
            {
                GameData *data = mUtil->getGameData(notify.getGameId());
                if (data != nullptr)
                {
                    if (data->addPlayer(notify.getJoiningPlayer()))
                    {
                        mUtil->addMetric(GameManagerUtil::METRIC_PLAYER_JOINING);
                    }

                    PlayerData* playerData = data->getPlayerData(getBlazeId());
                    if (playerData != nullptr)
                    {
                        gSelector->scheduleFiberCall(this, &GameManagerUtilInstance::updateHostConnection, 
                            notify.getGameId(), playerData->mConnGroupId, 
                            notify.getJoiningPlayer().getConnectionGroupId(), CONNECTED,
                            "GameManagerUtilInstance::updateHostConnection");
                    }
                    else
                    {
                        if (!data->isHost(getBlazeId()))
                        {
                            // Assume that this instance is the game's topology host, even if we can't confirm it
                            // (Ported from a Madden customization that fixes an issue where DS bot machines don't send connected status for players entering games)
                            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Instance user " << getBlazeId() << " is neither a player nor the expected topology host (" << data->getHostId() << ") in game " << 
                                notify.getGameId() << ". This may mean there was a failed host migration or delayed host migration notification.");
                        }

                        gSelector->scheduleFiberCall(this, &GameManagerUtilInstance::updateHostConnection,
                            notify.getGameId(), data->getHostConnectionGroupId(),
                            notify.getJoiningPlayer().getConnectionGroupId(), CONNECTED,
                            "GameManagerUtilInstance::updateHostConnection");
                    }
                }
                else
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Ignoring PlayerJoining notification for unregistered GameId " << notify.getGameId());
                }

                scheduleCustomNotification(type, &notify);
            }
        }
        break;
    case GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINCOMPLETED:
        {
            NotifyPlayerJoinCompleted notify;
            if (decoder.decode(*payload, notify) != ERR_OK)
            {
                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Failed to decode notification(" << type << ")");
            }
            else
            {
                GameData *data = mUtil->getGameData(notify.getGameId());
                if (data != nullptr)
                {
                    PlayerData *player = data->getPlayerData(notify.getPlayerId());
                    if (player == nullptr)
                    {
                        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Player (" << notify.getPlayerId() 
                                       << ") join completed but not found in game " << notify.getGameId() << " local roster?");                       
                    }
                    else
                    {
                        player->mJoinCompleted = true;
                        //  TODO: this isn't reliable for games spread across multiple stress clients - kept in for compatibility with single stress client
                        if (data->getHostId() == getBlazeId())
                        {
                            data->mNumPlayersJoinCompleted++;
                            mUtil->addMetric(GameManagerUtil::METRIC_PLAYER_JOINCOMPLETED);
                        }
                        scheduleUpdateGamePresenceForLocalUser(notify.getGameId(), UPDATE_PRESENCE_REASON_GAME_JOIN);
                    }
                    scheduleCustomNotification( type, &notify);
                }
                else
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Ignoring PlayerJoinCompleted notification for unregistered GameId " << notify.getGameId());
                }
            }
        }
        break;
    case GameManagerSlave::NOTIFY_NOTIFYPLAYERREMOVED:
        {
            NotifyPlayerRemoved notify;
            if (decoder.decode(*payload, notify) != ERR_OK)
            {
                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Failed to decode notification(" << type << ")");
            }
            else 
            {
                GameData *data = mUtil->getGameData(notify.getGameId());

                if (data != nullptr) // && data->getHostId() == getBlazeId())
                {
                    if (notify.getPlayerId() == getBlazeId())
                    {
                        // update primary external session, before erasing local player
                        scheduleUpdateGamePresenceForLocalUser(notify.getGameId(), UPDATE_PRESENCE_REASON_GAME_LEAVE, Blaze::GameManager::PLAYER_LEFT);

                        if (notify.getGameId() == mGameId)
                        {
                            mGameId = 0;
                        }
                        //  subscribe to census data.
                        if (mUtil->getCensusSubscribe())
                        {
                            Fiber::CreateParams params(Fiber::STACK_SMALL);
                            gSelector->scheduleFiberCall(this, &GameManagerUtilInstance::censusDataUnsubscribe, "GameManagerUtilInstance::censusDataUnsubscribe", params);
                        }
                    }

                    if (notify.getPlayerRemovedReason() == PLAYER_LEFT)
                    {
                        if (data->removePlayer(notify.getPlayerId()))
                        {
                            mUtil->addMetric(GameManagerUtil::METRIC_PLAYER_LEFT);
                        }
                    }
                    else if (notify.getPlayerRemovedReason() == Blaze::GameManager::PLAYER_JOIN_TIMEOUT)
                    {
                        if (data->removePlayer(notify.getPlayerId()))
                        {
                            mUtil->addMetric(GameManagerUtil::METRIC_PLAYER_JOIN_TIMEOUTS);
                        }
                    }
                    else if (notify.getPlayerRemovedReason() == Blaze::GameManager::GAME_DESTROYED)
                    {
                        data->removePlayer(notify.getPlayerId());
                    }
                    else if (notify.getPlayerRemovedReason() == Blaze::GameManager::BLAZESERVER_CONN_LOST)
                    {
                        data->removePlayer(notify.getPlayerId());
                    }
                    else
                    {
                        data->removePlayer(notify.getPlayerId());
                    }
                    scheduleCustomNotification( type, &notify);
                }
                else
                {
                     BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Ignoring PlayerRemoved notification for unregistered GameId " << notify.getGameId());
                }

                if (notify.getPlayerId() == getBlazeId())
                {
                    //  mirrors the registerGame call in NotifyGameSetup - to keep games in sync locally.
                    if (mUtil->unregisterGame(notify.getGameId()))
                    {
                        mUtil->addMetric(GameManagerUtil::METRIC_GAMES_DESTROYED);
                    }
                }
            }
        }
        break;
    case GameManagerSlave::NOTIFY_NOTIFYGAMESTATECHANGE:
        {
            clearIsWaitingForGameStateNotification();
            NotifyGameStateChange notify;
            if (decoder.decode(*payload, notify) != ERR_OK)
            {
                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Failed to decode notification(" << type << ")");
            }
            else 
            {
                GameData *data = mUtil->getGameData(notify.getGameId());
                if (data != nullptr) // && data->getHostId() == getBlazeId())
                {
                    GameState state = notify.getNewGameState();
                    
                    data->mLastGameState = data->mGameState;
                    data->setGameState(state);

                    if (data->getGameState() == POST_GAME)
                    {
                        mPostGameCountdown = 5;            // hackish, delays finishing game until all reports (or most) are sent.
                    }

                    Fiber::CreateParams params(Fiber::STACK_SMALL);
                    gSelector->scheduleFiberCall(this, &GameManagerUtilInstance::newGameState, "GameManagerUtilInstance::newGameState", params);
                    scheduleCustomNotification( type, &notify);
                }
                else
                {
                     BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Ignoring GameStateChange notification for unregistered GameId " << notify.getGameId());
                }
            }
        }
        break;
    case GameManagerSlave::NOTIFY_NOTIFYGAMESETTINGSCHANGE:
        {
            NotifyGameSettingsChange notify;
            if (decoder.decode(*payload, notify) != ERR_OK)
            {
                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Failed to decode notification(" << type << ")");
            }
            else 
            {
                GameData *data = mUtil->getGameData(notify.getGameId());
                if (data != nullptr) // && data->getHostId() == getBlazeId())
                {
                    data->getGameSettings()->setBits(notify.getGameSettings().getBits());
                    scheduleCustomNotification( type, &notify);
                }
                else
                {
                     BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Ignoring GameSettingsChange notification for unregistered GameId " << notify.getGameId());
                }
            }
        }
        break;
    case GameManagerSlave::NOTIFY_NOTIFYGAMERESET:
        {
            NotifyGameReset notify;
            if (decoder.decode(*payload, notify) != ERR_OK)
            {
                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Failed to decode notification(" << type << ")");
            }
            else
            {
                // Ignore game group notifications
                if (notify.getGameData().getGameType() != GAME_TYPE_GAMESESSION)
                    return;

                GameData *gameData = mUtil->getGameData(notify.getGameData().getGameId());
                mUtil->decrementResetableServers();
                if (gameData != nullptr && gameData->getHostId() == getBlazeId())
                {
                    //  only host user should ever set the game state
                    mUtil->addMetric(GameManagerUtil::METRIC_GAMES_RESET);     
                    mGameTimer = 0;

                    gameData->init(notify.getGameData());
                    Fiber::CreateParams params(Fiber::STACK_SMALL);
                    gSelector->scheduleFiberCall(this, &GameManagerUtilInstance::advanceGameState, PRE_GAME, "GameManagerUtilInstance::advanceGameState", params);
                }

                scheduleCustomNotification( type, &notify);
            }
        }
        break;
    case GameManagerSlave::NOTIFY_NOTIFYHOSTMIGRATIONSTART:
        {
            NotifyHostMigrationStart notify;
            //    update the GameData's host information
            //    if this instance is the new host, set the instance role to OWNER.
            //    if this instance was the old host, set the instance role to JOINER
            if (decoder.decode(*payload, notify) != ERR_OK)
            {
                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Failed to decode notification(" << type << ")");
            }
            else
            {
                GameData *data = mUtil->getGameData(notify.getGameId());
                if (data != nullptr)
                {
                    NetworkAddress hostaddr;

                    if (notify.getNewHostId() == getBlazeId())
                    {
                        //  support only topology migration for now.
                        if (notify.getMigrationType() == TOPOLOGY_HOST_MIGRATION || notify.getMigrationType() == TOPOLOGY_PLATFORM_HOST_MIGRATION)
                        {
                            if (getRole() != GameManagerUtil::ROLE_NONE)
                            {
                                //  set role only if this instance is being managed by the role system.
                                setRole(GameManagerUtil::ROLE_HOST);
                            }
                         
                            hostaddr.switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);

                            hostaddr.getIpPairAddress()->getExternalAddress().setIp(mStressInst->getConnection()->getAddress().getIp());
                            hostaddr.getIpPairAddress()->getExternalAddress().setPort(mStressInst->getConnection()->getAddress().getPort());
                            hostaddr.getIpPairAddress()->getExternalAddress().copyInto(hostaddr.getIpPairAddress()->getInternalAddress());

                            data->setNewHost(notify.getNewHostId(), notify.getNewHostSlotId(), &hostaddr);

                            // for the topology host migrations, simulate connections with the new host. To help avoid delay issues with the
                            // stress tool that leads to excess host migration timeouts, we only use the host and update for everyone in 1 call here.
                            PlayerData* newHostPlayerData = data->getPlayerData(notify.getNewHostId());
                            if (newHostPlayerData != nullptr)
                            {
                                updateAllPlayerConnsToHost(notify.getGameId(), newHostPlayerData->mConnGroupId, newHostPlayerData->mPlayerId);
                            }
                            else
                                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Failed to attempt connection post migration for game(" << notify.getGameId() << ") for new host (" << notify.getNewHostId() << ")");

                        }

                        if (notify.getMigrationType() == TOPOLOGY_PLATFORM_HOST_MIGRATION || notify.getMigrationType() == PLATFORM_HOST_MIGRATION)
                        {
                            Fiber::CreateParams params(Fiber::STACK_SMALL);
                            gSelector->scheduleFiberCall(this, &GameManagerUtilInstance::updateGameSession, notify.getGameId(), true /*migration*/, notify.getMigrationType(), "GameManagerUtilInstance::updateGameSession", params);
                        }
                    }
                    else 
                    {
                        data->setNewHost(notify.getNewHostId(), notify.getNewHostSlotId(), nullptr);

                        if (notify.getMigrationType() == TOPOLOGY_HOST_MIGRATION || notify.getMigrationType() == TOPOLOGY_PLATFORM_HOST_MIGRATION)
                        {
                            if (getRole() != GameManagerUtil::ROLE_NONE)
                            {
                                //  set role only if this instance is being managed by the role system.
                                setRole(GameManagerUtil::ROLE_JOINER);
                            }
                        }
                    }
                    scheduleCustomNotification( type, &notify);
                }
                else
                {
                     BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Ignoring HostMigrationStart notification for unregistered GameId " << notify.getGameId());
                }
            }
        }
        break;
    case GameManagerSlave::NOTIFY_NOTIFYHOSTMIGRATIONFINISHED:
        {
            NotifyHostMigrationFinished notify;
            if (decoder.decode(*payload, notify) != ERR_OK)
            {
                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Failed to decode notification(" << type << ")");
            }
            else
            {
                GameData *data = mUtil->getGameData(notify.getGameId());
                if (data != nullptr)
                {
                    scheduleCustomNotification( type, &notify);
                }
                else
                {
                     BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Ignoring HostMigrationFinished notification for unregistered GameId " << notify.getGameId());
                }
            }
        }
        break;
    case GameManagerSlave::NOTIFY_NOTIFYMATCHMAKINGFAILED:
        {
            NotifyMatchmakingFailed notify;
            if (decoder.decode(*payload, notify) != ERR_OK)
            {
                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Failed to decode notification(" << type << ")");
            }
            else
            {
                scheduleCustomNotification( type, &notify);
            }
        }
        break;
    case GameManagerSlave::NOTIFY_NOTIFYMATCHMAKINGASYNCSTATUS:
        {
            NotifyMatchmakingAsyncStatus notify;
            if (decoder.decode(*payload, notify) != ERR_OK)
            {
                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Failed to decode notification(" << type << ")");
            }
            else
            {
                scheduleCustomNotification( type, &notify);
            }

        }
        break;
    case GameManagerSlave::NOTIFY_NOTIFYGAMEREMOVED:
        {
            NotifyGameRemoved notify;
            if (decoder.decode(*payload, notify) != ERR_OK)
            {
                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Failed to decode notification(" << type << ")");
            }
            else
            {
                GameData *data = mUtil->getGameData(notify.getGameId());
                if (data != nullptr)
                {
                    scheduleCustomNotification( type, &notify);
                }
                else
                {
                     BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Ignoring GameRemoved notification for unregistered GameId " << notify.getGameId());
                }
            }
        }
        break;
    case GameManagerSlave::NOTIFY_NOTIFYPLATFORMHOSTINITIALIZED:
        {
            NotifyPlatformHostInitialized notify;
            if (decoder.decode(*payload, notify) != ERR_OK)
            {
                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Failed to decode notification(" << type << ")");
            }
            else
            {
                GameData *data = mUtil->getGameData(notify.getGameId());
                if (data != nullptr)
                {
                    scheduleCustomNotification( type, &notify);
                }
                else
                {
                     BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Ignoring PlatformHostInitialized notification for unregistered GameId " << notify.getGameId());
                }
            }
        }
        break;
    case GameManagerSlave::NOTIFY_NOTIFYGAMEATTRIBCHANGE:
        {
            NotifyGameAttribChange notify;
            if (decoder.decode(*payload, notify) != ERR_OK)
            {
                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Failed to decode notification(" << type << ")");
            }
            else
            {
                GameData *data = mUtil->getGameData(notify.getGameId());
                if (data != nullptr) // && data->getHostId() == getBlazeId())
                {
                    Collections::AttributeMap* gameAttrMap = data->getAttributeMap();
                    Collections::AttributeMap::const_iterator citEnd = notify.getGameAttribs().end();
                    for (Collections::AttributeMap::const_iterator cit = notify.getGameAttribs().begin(); cit != citEnd; ++cit)
                    {
                        Collections::AttributeMap::iterator gameAttrIt = gameAttrMap->find(cit->first);
                        if (gameAttrIt != gameAttrMap->end())
                        {
                            //  modify this value
                            gameAttrIt->second = cit->second;
                        }
                        else
                        {
                            //  add this value.
                            gameAttrMap->insert(Collections::AttributeMap::value_type(cit->first, cit->second));
                        }
                    }
                    scheduleCustomNotification( type, &notify);
                }
                else
                {
                     BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Ignoring GameAttribChange notification for unregistered GameId " << notify.getGameId());
                }
            }
        }
        break;
    case GameManagerSlave::NOTIFY_NOTIFYDEDICATEDSERVERATTRIBCHANGE:
        {
            NotifyDedicatedServerAttribChange notify;
            if (decoder.decode(*payload, notify) != ERR_OK)
            {
                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Failed to decode notification(" << type << ")");
            }
            else
            {
                // We don't currently do anything with the DS attributes on the client.
            }
        }
        break;
    case GameManagerSlave::NOTIFY_NOTIFYPLAYERATTRIBCHANGE:
        {
            NotifyPlayerAttribChange notify;
            if (decoder.decode(*payload, notify) != ERR_OK)
            {
                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Failed to decode notification(" << type << ")");
            }
            else
            {
                GameData *data = mUtil->getGameData(notify.getGameId());
                if (data != nullptr) // && data->getHostId() == getBlazeId())
                {
                    PlayerData *player = data->getPlayerData(notify.getPlayerId());
                    if (player != nullptr)
                    {
                        Collections::AttributeMap* playerAttrMap = data->getAttributeMap();
                        Collections::AttributeMap::const_iterator citEnd = notify.getPlayerAttribs().end();
                        for (Collections::AttributeMap::const_iterator cit = notify.getPlayerAttribs().begin(); cit != citEnd; ++cit)
                        {
                            Collections::AttributeMap::iterator playerAttrIt = playerAttrMap->find(cit->first);
                            if (playerAttrIt != playerAttrMap->end())
                            {
                                //  modify this value
                                playerAttrIt->second = cit->second;
                            }
                            else
                            {
                                //  add this value.
                                playerAttrMap->insert(Collections::AttributeMap::value_type(cit->first, cit->second));
                            }
                        }
                    }
                    scheduleCustomNotification( type, &notify);
                }
                else
                {
                     BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Ignoring PlayerAttribChange notification for unregistered GameId " << notify.getGameId());
                }
            }
        }
        break;
    case GameManagerSlave::NOTIFY_NOTIFYPLAYERCUSTOMDATACHANGE:
        {
            NotifyPlayerCustomDataChange notify;
            if (decoder.decode(*payload, notify) != ERR_OK)
            {
                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Failed to decode notification(" << type << ")");
            }
            else
            {
                GameData *data = mUtil->getGameData(notify.getGameId());
                if (data != nullptr)
                {
                    scheduleCustomNotification( type, &notify);
                }
                else
                {
                     BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Ignoring PlayerCustomDataChange notification for unregistered GameId " << notify.getGameId());
                }
            }
        }
        break;
    case GameManagerSlave::NOTIFY_NOTIFYGAMECAPACITYCHANGE:
        {
            NotifyGameCapacityChange notify;
            if (decoder.decode(*payload, notify) != ERR_OK)
            {
                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Failed to decode notification(" << type << ")");
            }
            else
            {
                GameData *gameData = mUtil->getGameData(notify.getGameId());
                if (gameData != nullptr)
                {
                    gameData->mPlayerCapacity = notify.getSlotCapacities().at(SLOT_PUBLIC_PARTICIPANT) + notify.getSlotCapacities().at(SLOT_PRIVATE_PARTICIPANT);
                    scheduleCustomNotification( type, &notify);
                }
                else
                {
                     BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Ignoring GameCapacityChange notification for unregistered GameId " << notify.getGameId());
                }
            }
        }
        break;
    case GameManagerSlave::NOTIFY_NOTIFYGAMEREPORTINGIDCHANGE:
        {
            NotifyGameReportingIdChange notify;
            if (decoder.decode(*payload, notify) != ERR_OK)
            {
                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Failed to decode notification(" << type << ")");
            }
            else
            {
                GameData *data = mUtil->getGameData(notify.getGameId());
                if (data != nullptr)
                {
                    data->setGameReportingId(notify.getGameReportingId());
                    scheduleCustomNotification( type, &notify);
                }
                else
                {
                     BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Ignoring GameReportingIdChange notification for unregistered GameId " << notify.getGameId());
                }
            }
        }
        break;
    case GameManagerSlave::NOTIFY_NOTIFYGAMESESSIONUPDATED:
        {
            GameSessionUpdatedNotification notify;
            if (decoder.decode(*payload, notify) != ERR_OK)
            {
                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Failed to decode notification(" << type << ")");
            }
            else
            {
                GameData *data = mUtil->getGameData(notify.getGameId());
                if (data != nullptr)
                {
                    scheduleCustomNotification( type, &notify);
                }
                else
                {
                     BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Ignoring GameSessionUpdated notification for unregistered GameId " << notify.getGameId());
                }
            }
        }
        break;
    case GameManagerSlave::NOTIFY_NOTIFYGAMEPLAYERSTATECHANGE:
        {
            NotifyGamePlayerStateChange notify;
            if (decoder.decode(*payload, notify) != ERR_OK)
            {
                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Failed to decode notification(" << type << ")");
            }
            else
            {
                GameData *data = mUtil->getGameData(notify.getGameId());
                if (data != nullptr)
                {
                    scheduleCustomNotification( type, &notify);
                }
                else
                {
                     BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Ignoring GamePlayerStateChange notification for unregistered GameId " << notify.getGameId());
                }
            }
        }
        break;
    case GameManagerSlave::NOTIFY_NOTIFYADMINLISTCHANGE:
        {
            NotifyAdminListChange notify;
            if (decoder.decode(*payload, notify) != ERR_OK)
            {
                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Failed to decode notification(" << type << ")");
            }
            else
            {
                GameData *data = mUtil->getGameData(notify.getGameId());
                if (data != nullptr)
                {
                    scheduleCustomNotification( type, &notify);
                }
                else
                {
                     BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Ignoring AdminListChange notification for unregistered GameId " << notify.getGameId());
                }
            }
        }
        break;

    default:
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].onAsync: Unsupported notification(" << type << ") - addition to GameManagerUtilInstance required.");
        break;
    }
}

void GameManagerUtilInstance::setNextQosTelemetryTime(GameData& gameData)
{
    if (gameData.mQosTelemetryInterval > 0)
    {
        gameData.mNextTelemetryTime = TimeValue::getTimeOfDay() + gameData.mQosTelemetryInterval;
    }
    else
    {
        gameData.mNextTelemetryTime = 0;
    }
}

void GameManagerUtilInstance::advanceGameState(GameState state)
{
    advanceGameState(mGameId, state);
}

void GameManagerUtilInstance::advanceGameState(GameId gameId, GameState state)
{
    StressLogContextOverride stressLogContextOverride(getIdent(), getBlazeId());

    if (!isWaitingForGameStateNotification())
    {
        BlazeRpcError err;
        mUtil->advanceGameState(&err, mGameManagerProxy, gameId, state);
        if(err != ERR_OK)
        {
            const char8_t* currGameState = "";
            GameData *data = mUtil->getGameData(gameId);
            if (data != nullptr)
            {
                currGameState = GameStateToString(data->getGameState());
            }
            BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].advanceGameState: Error " << (ErrorHelp::getErrorName(err)) << "(" << err 
                           << ") while changing to game state from '" << currGameState << "' to '" << GameStateToString(state) << "' for game id " << gameId << ".");
        }
        else
        {
            setIsWaitingForGameStateNotification();
        }
    }
}


void GameManagerUtilInstance::updateGameSession(GameId gameId, bool migration, HostMigrationType migrationType)
{
    StressLogContextOverride stressLogContextOverride(getIdent(), getBlazeId());

    BlazeRpcError err;
    // UpdateGameSession RPC is only used for testing PS4.
    // if mock external sessions is enabled, we're using Blaze Server driven external sessions, and clients don't call updateGameSession
    if ((getPlatform() != Blaze::ps4) || (mUtil->getMockExternalSessions()[0] != '\0'))
    {
        err = ERR_OK;
    }
    else
    {
        UpdateGameSessionRequest req;
        req.setGameId(gameId);
        err = mGameManagerProxy->updateGameSession(req);
    }

    if(err != ERR_OK)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, 
            "[GameManagerUtilInstance].updateGameSession: Error " << err << " while updating game session game id " << gameId << ".");
    }
    else if (migration)
    {
        UpdateGameHostMigrationStatusRequest migReq;
        migReq.setGameId(gameId);
        migReq.setHostMigrationType(migrationType);
        err = mGameManagerProxy->updateGameHostMigrationStatus(migReq);
        if(err != ERR_OK)
        {
            BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, 
                "[GameManagerUtilInstance].updateGameSession: Error " << err << " while updating host migration status for game id " << gameId << ".");
        }
    }
    
}

// update mesh connections, auto connect all current users in the game to the host (hosted topologies)
void GameManagerUtilInstance::updateAllPlayerConnsToHost(GameId gameId, ConnectionGroupId hostConnId, BlazeId hostId)
{
    GameData *data = mUtil->getGameData(gameId);
    if (data == nullptr)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].updateAllPlayerConnsToHost: Game Data null. Failed to attempt connection post migration for game(" << gameId << ") for host(" << hostId << ") connid(" << hostConnId << ")");
        return;
    }
    PlayerData* hostPlayerData = data->getPlayerData(hostId);
    if (hostPlayerData == nullptr) 
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].updateAllPlayerConnsToHost: Host Player Data null. Failed to attempt connection post migration for game(" << gameId << ") for host(" << hostId << ") connid(" << hostConnId << ")");
        return;
    }

    typedef eastl::vector<ConnectionGroupId> ConnGroupIdList;
    ConnGroupIdList targetConnGroupIdList;
    const size_t numPlayers = data->getNumPlayers();
    for (size_t i = 0; i < numPlayers; ++i)
    {
        PlayerData* playerData = data->getPlayerDataByIndex(i);
        if ((playerData != nullptr) && (playerData->mPlayerId != hostId))
            targetConnGroupIdList.push_back(playerData->mConnGroupId);
    }

    for (ConnGroupIdList::const_iterator itr = targetConnGroupIdList.begin(); itr != targetConnGroupIdList.end(); ++itr)
    {
        // host -> member
        updatePlayerConns(gameId, hostConnId, *itr);
        // member -> host
        updatePlayerConns(gameId, *itr, hostConnId);
    }    
}

// update mesh connection, one way.
void GameManagerUtilInstance::updatePlayerConns(GameId gameId, ConnectionGroupId sourceConnId, ConnectionGroupId targetConnId)
{
    UpdateMeshConnectionRequest request;
    EA::TDF::ObjectId sourceObject;
    EA::TDF::ObjectId targetObject;
    sourceObject.type = ENTITY_TYPE_CONN_GROUP;
    targetObject.type = ENTITY_TYPE_CONN_GROUP;
    sourceObject.id = sourceConnId;
    targetObject.id = targetConnId;

    request.setSourceGroupId(sourceObject);
    request.setPlayerNetConnectionStatus(CONNECTED);
    request.setTargetGroupId(targetObject);          
    request.setGameId(gameId);
    BlazeRpcError err = mGameManagerProxy->updateMeshConnection(request);
    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].updatePlayerConns: Failed to attempt connection post migration for game(" << gameId << ") from source player connid(" << sourceConnId << ") to target connid(" << targetConnId << ")");
    }
}

// updates connections in the req
void GameManagerUtilInstance::updatePlayerConns(GameId gameId, UpdateMeshConnectionRequest req)
{
    req.setGameId(gameId);
    BlazeRpcError err = mGameManagerProxy->updateMeshConnection(req);
    if (err != ERR_OK)
    {
        /*BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, 
            "[GameManagerUtilInstance].updatePlayerConns: Error " << err << " updating player connection to player " << connStatusList[i]->getTargetPlayer() 
             << " for game id " << mGameId << ".");*/   
    }
}


void GameManagerUtilInstance::finalizeGameCreation(GameId gameId, bool platformHost, bool topologyHost)
{
    StressLogContextOverride stressLogContextOverride(getIdent(), getBlazeId());

    UpdateGameSessionRequest updateGameSessionRequest;
    updateGameSessionRequest.setGameId( gameId );

    BlazeRpcError err = ERR_OK;

    if (topologyHost)
    {        
        err = mGameManagerProxy->finalizeGameCreation(updateGameSessionRequest);
    }
    // UpdateGameSession RPC is only used for testing PS4.
    // if mock external sessions is enabled, we're using Blaze Server driven external sessions, and clients don't call updateGameSession
    else if ((getPlatform() == Blaze::ps4) && platformHost && (mUtil->getMockExternalSessions()[0] == '\0'))
    {
        err = mGameManagerProxy->updateGameSession(updateGameSessionRequest);
    }

    if (err != ERR_OK)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].finalizeGameCreation: Error " << err << " calling finalizeGameCreation (platformHost=" 
                       << (platformHost ? 1 : 0) << ") for game id " << gameId << ".");
    }
    else
    {
        // update primary external session
        scheduleUpdateGamePresenceForLocalUser(gameId, UPDATE_PRESENCE_REASON_GAME_CREATE);
    }
}


void GameManagerUtilInstance::updateHostConnection(GameId gameId, ConnectionGroupId sourceGroupId, ConnectionGroupId topologyGroupId, PlayerNetConnectionStatus connStatus)
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
    BlazeRpcError err = mGameManagerProxy->updateMeshConnection(req);
    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager,  "[GameManagerUtilInstance].updateHostConnection: Error " << err << " calling updateMeshConnection for game id " << gameId);
    }
}


// Census Data callbacks
void GameManagerUtilInstance::censusDataSubscribe()
{
    StressLogContextOverride stressLogContextOverride(getIdent(), getBlazeId());

#ifdef TARGET_censusdata
    if (mIsSubscribedCensusData || 
        mCensusDataProxy == nullptr)
        return;
    mIsSubscribedCensusData = true;
    BlazeRpcError err =  mCensusDataProxy->subscribeToCensusData();
    if (err != ERR_OK)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::censusdata, "[GameManagerUtilInstance].censusDataSubscribe: Error " << err << " subscribing to census data.");
        mIsSubscribedCensusData = false;
    }
#endif
}


void GameManagerUtilInstance::censusDataUnsubscribe()
{
    StressLogContextOverride stressLogContextOverride(getIdent(), getBlazeId());

#ifdef TARGET_censusdata
    if (!mIsSubscribedCensusData ||
        mCensusDataProxy == nullptr)
        return;
    mIsSubscribedCensusData = false;
    BlazeRpcError err =  mCensusDataProxy->unsubscribeFromCensusData();
    if (err != ERR_OK)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::censusdata, "[GameManagerUtilInstance].censusDataUnsubscribe: Error " << err << " unsubscribing to census data.");
    }
#endif
}


void GameManagerUtilInstance::newGameState()
{
    StressLogContextOverride stressLogContextOverride(getIdent(), getBlazeId());

    if (mGameId == 0)
        return;
    const GameData *game = getGameData();
    if (game == nullptr)
        return;

    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].newGameState: " 
        << " game Id=" << mGameId << " new state: " << GameStateToString(game->getGameState()));
#ifdef TARGET_gamereporting
    //  new game reporting stress code for online games
    if (mGameReportingUtilInstance != nullptr)
    {
        auto gameReportId = game->getGameReportingId();
        auto gameState = game->getGameState();
        switch (gameState)
        {
        case POST_GAME:
        {
            const bool needsUpdate = mUtil->getGameReportingUtil().getReportUpdateCount(gameReportId) < 1;
            if (needsUpdate)
            {
                GameReportingUtil::PlayerAttributeMap players;
                for (GameData::PlayerMap::const_iterator playerIt = game->getPlayerMap().begin(), playerItEnd = game->getPlayerMap().end(); playerIt != playerItEnd; ++playerIt)
                {
                    players[playerIt->first] = playerIt->second.getAttributeMap(); 
                }
                mUtil->getGameReportingUtil().updateReport(gameReportId, game->getAttributeMap(), &players);
            }
            BlazeRpcError rc = mUtil->getGameReportingUtil().submitReport(gameReportId, mGameReportingUtilInstance->getProxy());
            if (rc == ERR_OK)
            {
                BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].newGameState: " 
                    << " game reportId=" << gameReportId << " submitted report POST-GAME.");
            }
            else
            {
                BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].newGameState: " 
                    << " game reportId=" << gameReportId << " failed to submit game report: " << ErrorHelp::getErrorName(rc));
            }
        }
        break;
        default:
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].newGameState: " 
                << " game reportId=" << gameReportId << " unhandled: "<< GameManager::GameStateToString(gameState));
        }
        break;
        }
    }
#endif
}

void GameManagerUtilInstance::scheduleUpdateGamePresenceForLocalUser(const GameId changedGameId, UpdateExternalSessionPresenceForUserReason change, PlayerRemovedReason removeReason /*= SYS_PLAYER_REMOVE_REASON_INVALID*/)
{
    if (mUtil->getMockExternalSessions()[0] != '\0')
    {
        gSelector->scheduleFiberCall(this,  &GameManagerUtilInstance::updateGamePresenceForLocalUser, changedGameId, change, removeReason, "GameManagerUtilInstance::updateGamePresenceForLocalUser");
    }
}
// Based on BlazeSDK's GameManagerAPI method of the same name
void GameManagerUtilInstance::updateGamePresenceForLocalUser(const GameId changedGameId, UpdateExternalSessionPresenceForUserReason change, PlayerRemovedReason removeReason /*= SYS_PLAYER_REMOVE_REASON_INVALID*/)
{
    StressLogContextOverride stressLogContextOverride(getIdent(), getBlazeId());

    // Dedicated servers never become the primary game. Can just early out (as done on BlazeSDK)
    const GameData* gsData = getGameData();
    if ((gsData != nullptr) && (gsData->getGameId() == changedGameId) && (gsData->getTopology() == CLIENT_SERVER_DEDICATED))
        return;

    UpdateExternalSessionPresenceForUserRequest request;
    request.getUserIdentification().setBlazeId(getBlazeId());
    request.setChangedGameId(changedGameId);//deprecated, for back compatibility w/older servers.
    if (gsData)
    {
        populateGameActivity(request.getChangedGame(), *gsData, gsData->getPlayerData(getBlazeId()));
        //(Stress tool only ever join at most one game session per user, so this should be it)
        populateGameActivity(*request.getCurrentGames().pull_back(), *gsData, gsData->getPlayerData(getBlazeId()));
    }
    request.setChange(change);
    request.setOldPrimaryGameId(mPrimaryPresenceGameId);
    request.setRemoveReason(removeReason);
    // Note: Blaze Server will automatically set user's external id/blob (to be its BlazeId) when mock platform testing

    // skip left game if its still in our list.
    if ((gsData != nullptr) && ((change != UPDATE_PRESENCE_REASON_GAME_LEAVE) || (gsData->getGameId() != changedGameId)))
    {
        populateGameActivity(*request.getCurrentGames().pull_back(), *gsData, gsData->getPlayerData(getBlazeId()));
    }

    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].updateGamePresenceForLocalUser: Determining which of " << (gsData != nullptr? 1 : 0) <<
        " games (if any) to set as primary game for user(" << getBlazeId() << ") on " << UpdateExternalSessionPresenceForUserReasonToString(change) << " of game(" << changedGameId << ", gameType=GAMESESSION ).");

    // Pre: user is logged into one client, so the client is the central authority on the games the user is in at any time.
    // The server has the logic for deciding which game (if any) should be designated as the primary game. Send the games and data to server.
    UpdateExternalSessionPresenceForUserResponse response;
    UpdateExternalSessionPresenceForUserErrorInfo errInfo;
    BlazeRpcError rpcResult = getSlave()->updatePrimaryExternalSessionForUser(request, response, errInfo);
    if (rpcResult != ERR_OK)
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].updateGamePresenceForLocalUser: Update of primary game for user(" << getBlazeId() << ") result: " << ErrorHelp::getErrorName(rpcResult) << ".\n");
        mPrimaryPresenceGameId = errInfo.getCurrentPrimary().getGameId();
        return;
    }
    // Side: for stress testing, there is no pending title callback or onPlayerJoinComplete to dispatch after, simply return
    mPrimaryPresenceGameId = response.getCurrentPrimary().getGameId();
}

void GameManagerUtilInstance::populateGameActivity(GameActivity& activity, const GameData& game, const PlayerData* player)
{
    activity.setGameId(game.getGameId());
    activity.setGameType(GAME_TYPE_GAMESESSION);
    activity.setPresenceMode(PRESENCE_MODE_STANDARD);//for simplicity
    game.getExternalSessionIdentification().copyInto(activity.getSessionIdentification());
    activity.getPlayer().setPlayerState(ACTIVE_CONNECTED);//for simplicity
    activity.getPlayer().setJoinedGameTimestamp(123);//for simplicity
    activity.getPlayer().setTeamIndex(EA_LIKELY(player) ? player->mTeamIndex : 0);
    activity.getPlayer().setSlotType(EA_LIKELY(player) ? player->mSlotType : SLOT_PUBLIC_PARTICIPANT);

    // also set deprecated external session parameters. For backward compatibility with older servers.
    activity.setPlayerState(activity.getPlayer().getPlayerState());
    activity.setJoinedGameTimestamp(activity.getPlayer().getJoinedGameTimestamp());
    activity.setExternalSessionName(game.getExternalSessionIdentification().getXone().getSessionName());
    activity.setExternalSessionTemplateName(game.getExternalSessionIdentification().getXone().getTemplateName());
}

Blaze::BlazeRpcError GameManagerUtilInstance::updateGameName()
{
    BlazeRpcError err = ERR_OK;
    GameData *gamedata = (mGameId != Blaze::GameManager::INVALID_GAME_ID ? mUtil->getGameData(mGameId) : nullptr);
    if (gamedata == nullptr)
        return ERR_OK;

    int32_t roll = Blaze::Random::getRandomNumber(100);
    if (roll < (mUtil->getGameNameRandomUpdates() * 100))
    {
        // if name to update to not already set, use current game's as base.
        if (isUpdateGameNameRequestCleared())
        {
            setUpdateGameNameRequest(gamedata->getGameName()); // init mUpdateGameNameRequest
        }
        // ensure name changes
        if (mUpdateGameNameRequest[0] != 'X')
        {
            mUpdateGameNameRequest[0] = 'X';
        }
        else
        {
            mUpdateGameNameRequest[0] = 'Y';
        }
        BLAZE_SPAM_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].updateGameName: Updating game(" << gamedata->getGameId() << ") name: '" << getGameData()->getGameName() << "' ==to==> '" << mUpdateGameNameRequest << "'");
        gamedata->setGameName(mUpdateGameNameRequest);

        // call rpc. clear mUpdateGameNameRequest
        mUtil->udpateGameName(&err, mGameManagerProxy, mGameId, mUpdateGameNameRequest);
        clearUpdateGameNameRequest();
        if (err != ERR_OK)
        {
            BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].updateGameName: Update of game(" << gamedata->getGameId() << ") name failed. Error(" << ErrorHelp::getErrorName(err) << ").");
        }
    }
    return err;
}


Blaze::BlazeRpcError GameManagerUtilInstance::updateGameExternalStatus()
{
    BlazeRpcError err = ERR_OK;
    GameData *gamedata = (mGameId != Blaze::GameManager::INVALID_GAME_ID ? mUtil->getGameData(mGameId) : nullptr);
    if (gamedata == nullptr)
        return ERR_OK;

    // Update external session status
    int32_t roll = Blaze::Random::getRandomNumber(100);
    if (roll < (mUtil->getGameExternalStatusRandomUpdates() * 100))
    {
        // to have status change, just use the roll's string
        mUpdateGameExternalStatusRequest.sprintf("%i", roll);
        BLAZE_SPAM_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].updateGameExternalStatus: Updating game(" << gamedata->getGameId() << ") external session status: ==to==> '" << mUpdateGameExternalStatusRequest.c_str() << "'");

        // update status. test localizations
        UpdateExternalSessionStatusRequest statusReq;
        statusReq.setGameId(mGameId);
        statusReq.getExternalSessionStatus().setUnlocalizedValue(mUpdateGameExternalStatusRequest.c_str());
        statusReq.getExternalSessionStatus().getLangMap()["ja"] = mUpdateGameExternalStatusRequest.c_str();
        statusReq.getExternalSessionStatus().getLangMap()["fr"] = mUpdateGameExternalStatusRequest.c_str();
        err = mGameManagerProxy->updateExternalSessionStatus(statusReq);
        if (err != ERR_OK)
        {
            BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameManagerUtilInstance].updateGameExternalStatus: Set game external session status(" << mUpdateGameExternalStatusRequest.c_str() << ") for game(" << mGameId << ") failed. Error(" 
                << (ErrorHelp::getErrorName(err)) << ")");
        }
    }
    return err;
}


///////////////////////////////////////////////////////////////////////////////
//    GameData
///////////////////////////////////////////////////////////////////////////////

GameData::GameData(GameId gameId) :
    mGameId(gameId),
    mGameReportingId(0),
    mHostId(0),
    mHostSlotId(0),
    mPlatformHostId(0),
    mPlayerCapacity(0),
    mGameState(NEW_STATE),
    mLastGameState(NEW_STATE),
    mGameSettings(),
    mTopology(PEER_TO_PEER_FULL_MESH),
    mPlayerMap(),
    mPendingEntryCount(0),
    mNumPlayersJoinCompleted(0),
    mRefCount(0),
    mPacketsSent(0),
    mPacketsRecv(0)
{
    mName[0] = '\0';
}


GameData::~GameData()
{
}



void GameData::init(ReplicatedGameData &gameData)
{
//    EA_ASSERT(mPlayerMap.size() == 0);

    //    reserve enough space for all players in the player map.
    mPlayerMap.set_capacity(gameData.getMaxPlayerCapacity());

    mGameId = gameData.getGameId();
    mGameReportingId = gameData.getGameReportingId();
    mHostId = gameData.getTopologyHostInfo().getPlayerId();
    mHostSlotId = gameData.getTopologyHostInfo().getSlotId();
    mHostConnGroupId = gameData.getTopologyHostInfo().getConnectionGroupId();
    mPlatformHostId = gameData.getPlatformHostInfo().getPlayerId();
    size_t playerCapacity = 0;
    if (gameData.getSlotCapacities().size() < SLOT_PUBLIC_SPECTATOR)
    {
        playerCapacity = gameData.getMaxPlayerCapacity();
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GameData].init: Unexpected slot capacities for game(" << mGameId << "). ReplicatedGameData ("
            << gameData << ")");
    }
    else
    {
        playerCapacity = gameData.getSlotCapacities().at(SLOT_PUBLIC_PARTICIPANT) + gameData.getSlotCapacities().at(SLOT_PRIVATE_PARTICIPANT);
    }

    mPlayerCapacity = playerCapacity;
    mGameState = gameData.getGameState();
    mLastGameState = gameData.getGameState();
    mGameSettings.setBits(gameData.getGameSettings().getBits());
    mTopology = gameData.getNetworkTopology();
    
    mHostAddressList.clear();
    mHostAddressList.copyInto(gameData.getTopologyHostNetworkAddressList());

    mGameAttrs.insert(gameData.getGameAttribs().begin(), gameData.getGameAttribs().end());

    blaze_strnzcpy(mName, gameData.getGameName(), sizeof(mName));

    mRefCount = 1;

    gameData.getExternalSessionIdentification().copyInto(mExternalSessionIdentification);

}

void GameData::setQosTelemetryInterval(const TimeValue& qosTelemetryInterval)
{
    mQosTelemetryInterval = qosTelemetryInterval;
    mNextTelemetryTime = 0;
}

void GameData::setNewHost(PlayerId hostId, SlotId slotId, NetworkAddress *address)
{
    mHostId = hostId;
    mHostSlotId = slotId;
    if (address)
    {
        mHostAddressList.clear();
        address->copyInto(*mHostAddressList.pull_back());
    }

    // Try and update the host's connection group id
    PlayerData* player = getPlayerData(mHostId);
    if (player != nullptr)
        mHostConnGroupId = player->mConnGroupId;
}


PlayerData* GameData::getPlayerData(PlayerId id) const
{
    PlayerMap::const_iterator it = mPlayerMap.find(id);
    if (it == mPlayerMap.end())
        return nullptr;

    return const_cast<PlayerData*>(&it->second);
}


PlayerData* GameData::getPlayerDataByIndex(size_t index) const
{
    if (index >= mPlayerMap.size())
        return nullptr;

    PlayerMap::const_reference data = mPlayerMap.at(index);
    return const_cast<PlayerData*>(&data.second);
}


bool GameData::addPlayer(const ReplicatedGamePlayer &player)
{
    auto joinCompleted = player.getPlayerState() == ACTIVE_CONNECTED;
    auto inserter = mPlayerMap.insert(player.getPlayerId());
    auto& playerData = inserter.first->second;
    if (inserter.second)
    {
        playerData.mPlayerId = player.getPlayerId();
        playerData.mJoinCompleted = joinCompleted;
        playerData.mConnGroupId = player.getConnectionGroupId();
        playerData.getAttributeMap()->insert(player.getPlayerAttribs().begin(), player.getPlayerAttribs().end());
        playerData.mTimeAdded = TimeValue::getTimeOfDay();
        playerData.mTeamIndex = player.getTeamIndex();
        playerData.mSlotType = player.getSlotType();
        if(playerData.mJoinCompleted)
        {
            //  TODO: refactor since this won't work with games having players spread across different stress clients.
            mNumPlayersJoinCompleted++;
        }
        return true;
    }
    else
    {
        //  since this might be called during a NotifyGameSetup, redundant setting of the join completed flag
        //  attributes should've already been updated in NotifyGameSetup or NotifyPlayerJoining.
        playerData.mJoinCompleted = joinCompleted;
    }
    //else duplicates are fine since multiple instances may call this method at the same time.  
    return false;
}


bool GameData::removePlayer(PlayerId playerId)
{
    PlayerMap::iterator it = mPlayerMap.find(playerId);
    if (it != mPlayerMap.end())
    {
        if((*it).second.mJoinCompleted)
        {
            //  TODO: refactor since this won't work with games having players spread across different stress clients.
            mNumPlayersJoinCompleted--;
        }
        mPlayerMap.erase(it);
        return true;
    }
    //else duplicates are fine since multiple instances may call this method at the same time.  
    return false;
}


GameManagerUtil::QueryJoinFailure GameData::queryJoinGame(bool joinInProgress) const 
{ 
    //  in the process of resetting, don't touch.
    if(mGameState == RESETABLE)
    {
        return GameManagerUtil::QUERY_JOIN_NOT_READY;
    }
    //  don't allow games in invalid states to join 
    if (mGameState == IN_GAME && !joinInProgress)
    {
        return GameManagerUtil::QUERY_JOIN_NOT_READY;
    }
    if (mGameState != PRE_GAME)
    {
        return GameManagerUtil::QUERY_JOIN_NOT_READY;
    }
    //Should we allow join's in the POST_GAME or IN_GAME states?

    size_t estimatedPlayerCount = mPendingEntryCount + mPlayerMap.size();
    if(estimatedPlayerCount >= mPlayerCapacity)
        return GameManagerUtil::QUERY_JOIN_NO_ROOM;
    if (mPlayerMap.size() > 1 && (estimatedPlayerCount > mPlayerCapacity/2))
        return GameManagerUtil::QUERY_JOIN_SOME_ROOM;

    if (getGameState() == MIGRATING)
        return GameManagerUtil::QUERY_JOIN_NOT_READY;

    return GameManagerUtil::QUERY_JOIN_ROOM;
}

uint32_t GameData::getNextPacketsRecv()
{
    mPacketsRecv += Blaze::Random::getRandomNumber(90);
    return mPacketsRecv;
}

uint32_t GameData::getNextPacketsSent()
{
    mPacketsSent += 90 + Blaze::Random::getRandomNumber(10);
    return mPacketsSent;
}

void GameManagerUtilInstance::randomizeLatencyData(TelemetryReport* report, GameData& gameData)
{
    uint32_t totalSent = gameData.getNextPacketsSent();
    uint32_t totalRecv = gameData.getNextPacketsRecv();
    report->setLatencyMs(Blaze::Random::getRandomNumber(100));
    report->setPacketLossFloat((float)(totalRecv / totalSent));
    report->setLocalPacketsReceived(totalRecv);
    report->setRemotePacketsSent(totalSent);
}

}    // Stress
}    // Blaze
