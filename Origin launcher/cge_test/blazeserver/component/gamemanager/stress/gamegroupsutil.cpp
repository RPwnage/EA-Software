/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "gamegroupsutil.h"
#include "loginmanager.h"

#include "blazerpcerrors.h"
#include "framework/config/config_file.h"
#include "framework/connection/selector.h"
#include "framework/util/shared/blazestring.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/util/random.h"
#include "gamemanager/tdf/gamemanagerconfig_server.h" // for GameManagerServerConfig in updateGamePresenceForLocalUser
#include "framework/controller/controller.h" // for gController in updateGamePresenceForLocalUser
#include "framework/config/config_boot_util.h" // for ConfigBootUtil in scheduleUpdatePrimaryGameForLocalUser

namespace Blaze
{
namespace Stress
{
static const char8_t* METRIC_STRINGS[] =  {
    "ggCreated",
    "ggDestroyed",
    "ggRegistered",
    "ggUnregistered",
    "ggJoinFull",
    "ggJoins",
    "ggLeaves"
};

GamegroupsUtil::GamegroupsUtil() :
    mGamegroups(),
    mNextAvailableGamegroupIt(mGamegroups.end()),
    mPendingGamegroupCount(0),
    mGamegroupsFreq(0),
    mGamegroupLifespan(0),
    mGamegroupCapacity(0),
    mGamegroupMemberLifespan(0),
    mCreateOrJoinMethod(false),
    mAvgNumAttrs(0),
    mAvgNumMemberAttrs(0)
{
    mGamegroupAttrPrefix[0] = '\0';
    mGamegroupMemberAttrPrefix[0] = '\0';

    mLogBuffer[0] = '\0';

    for (int i = 0; i < NUM_METRICS; ++i)
    {
        mMetricCount[i] = 0;
    }
}


GamegroupsUtil::~GamegroupsUtil()
{
    while (!mGamegroups.empty())
    {
        GameIdToDataMap::iterator it = mGamegroups.begin();
        delete it->second;
        mGamegroups.erase(it);
    }
}


//  parses Gamegroups-utility-specific configuration for setup.
bool GamegroupsUtil::initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil)
{
    mGamegroupsFreq = (float)config.getDouble("ggGamegroupsFreq", 0);
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtil].initialize: ggGamegroupsFreq = " << mGamegroupsFreq);

    mGamegroupLifespan = config.getUInt32("ggLifespan", (uint32_t)(20));
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtil].initialize: ggLifespan = " << mGamegroupLifespan);

    mGamegroupMemberLifespan = config.getUInt32("ggMemberLifespan", (uint32_t)(5));
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtil].initialize: ggLifespan = " << mGamegroupLifespan);

    mGamegroupCapacity = config.getUInt32("ggCapacity", (uint32_t)(4));
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtil].initialize: ggCapacity = " << mGamegroupCapacity);

    mCreateOrJoinMethod = config.getBool("ggCreateOrJoin", false);
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtil].initialize: ggCreateOrJoin = " << (mCreateOrJoinMethod ? "true" : "false"));

    const char8_t *str = config.getString("ggAttrPrefix", "ggattr");
    blaze_strnzcpy(mGamegroupAttrPrefix, str, sizeof(mGamegroupAttrPrefix));
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtil].initialize: ggAttrPrefix = " << str);

    str = config.getString("ggMemberAttrPrefix", "memberattr");
    blaze_strnzcpy(mGamegroupMemberAttrPrefix, str, sizeof(mGamegroupMemberAttrPrefix));
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtil].initialize: ggMemberAttrPrefix = " << str);

    mAvgNumAttrs = config.getUInt32("ggAvgNumAttrs", 0);
    mAvgNumMemberAttrs = config.getUInt32("ggAvgNumMemberAttrs", 0);

    const ConfigSequence *attrs;
    const ConfigMap *valueSequences;
    const ConfigMap *params;
    attrs = config.getSequence("ggGamegroupAttrKeys");
    valueSequences = config.getMap("ggGamegroupAttrValues");
    params = config.getMap("ggGamegroupAttrUpdate");
    AttrValueUtil::initAttrUpdateValues(attrs, valueSequences, mGamegroupAttrValues);
    AttrValueUtil::initAttrUpdateParams(params, mGamegroupAttrUpdateParams);
    attrs = config.getSequence("ggMemberAttrKeys");
    valueSequences = config.getMap("ggMemberAttrValues");
    params = config.getMap("ggMemberAttrUpdate");
    AttrValueUtil::initAttrUpdateValues(attrs, valueSequences, mMemberAttrValues);
    AttrValueUtil::initAttrUpdateParams(params, mMemberAttrUpdateParams);

    if (bootUtil != nullptr)
    {
        PredefineMap::const_iterator itr = bootUtil->getConfigFile()->getDefineMap().find("MOCK_EXTERNAL_SESSIONS");
        if (itr != bootUtil->getConfigFile()->getDefineMap().end())
        {
            mMockExternalSessions = itr->second.c_str();
            BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtil].initialize: mockExternalSessionPlatform defined " << mMockExternalSessions.c_str());
        }
    }
    return true;
}


void GamegroupsUtil::dumpStats(StressModule *module)
{
    size_t memberCount = 0;
    GameIdToDataMap::const_iterator citEnd = mGamegroups.end();
    for (GameIdToDataMap::const_iterator cit = mGamegroups.begin(); cit != citEnd; ++cit)
    {
        memberCount += (*cit).second->getMemberCount();
    }

    char8_t statbuffer[128];
    blaze_snzprintf(statbuffer, sizeof(statbuffer), "time=%.2f, game groups=%" PRIsize ", members=%" PRIsize, module->getTotalTestTime(), mGamegroups.size(), memberCount);

    char8_t *metricOutput = mLogBuffer;
    metricOutput[0] = '\0';
    char8_t *curStr = metricOutput;

    for (int i = 0; i < NUM_METRICS; ++i)
    {
        blaze_snzprintf(curStr, sizeof(mLogBuffer) - strlen(metricOutput), "%s=%" PRIu64 " ",METRIC_STRINGS[i], this->mMetricCount[i]);
        curStr += strlen(curStr);
    }

    BLAZE_INFO_LOG(Log::CONTROLLER, "[GamegroupsUtil].dumpStats: " << statbuffer << " @ " << metricOutput);
}


//    diagnostics for utility not meant to measure load.
void GamegroupsUtil::addMetric(Metric metric)
{
    mMetricCount[metric]++;
}

bool GamegroupsUtil::removeMember(GameId gid, BlazeId bid, size_t& membersLeft)
{
    GameIdToDataMap::iterator it = mGamegroups.find(gid);
    if (it == mGamegroups.end())
    {
        membersLeft = 0;
        return false;
    }

    bool removed = it->second->removeMember(bid);
    membersLeft = it->second->getMemberCount();

    // If we've removed the last remaining player, we should unregister the game group now
    // because the blazeserver will destroy it without sending a GameRemoved notification.
    if (membersLeft == 0)
        unregisterGamegroup(gid);

    return removed;
}


//  Looks up a game group.
GamegroupData* GamegroupsUtil::getGamegroupData(GameId gid)
{
    GameIdToDataMap::iterator it = mGamegroups.find(gid);
    if (it == mGamegroups.end())
        return nullptr;
    return it->second;
}

GamegroupData* GamegroupsUtil::registerGamegroup(const ReplicatedGameData &data, ReplicatedGamePlayerList &replPlayerData)
{
    GameId gid = data.getGameId();
    GameIdToDataMap::insert_return_type inserter = mGamegroups.insert(gid);
    if (inserter.second)
    {
        GamegroupData *ggData = BLAZE_NEW GamegroupData(data, replPlayerData);
        inserter.first->second = ggData;
        addMetric(METRIC_GAMEGROUPS_REGISTERED);
    }

    GameId nextAvailJoinId = INVALID_GAME_ID;
    if (mNextAvailableGamegroupIt != mGamegroups.end())
    {
        nextAvailJoinId = mNextAvailableGamegroupIt->first;
    }

    //  initialize the next game group iterator for automatic joining.
    if (mNextAvailableGamegroupIt == mGamegroups.end())
    {
        mNextAvailableGamegroupIt = inserter.first;
    }
    else if (nextAvailJoinId != INVALID_GAME_ID)
    {
        mNextAvailableGamegroupIt = mGamegroups.find(nextAvailJoinId);
    }

    return inserter.first->second;
}

void GamegroupsUtil::unregisterGamegroup(GameId gid)
{
    GameIdToDataMap::iterator it = mGamegroups.find(gid);
    if (it != mGamegroups.end())
    {
        GameId nextAvailJoinId = INVALID_GAME_ID;
        if (mNextAvailableGamegroupIt != mGamegroups.end())
        {
            nextAvailJoinId = mNextAvailableGamegroupIt->first;
        }

        GamegroupData *data = it->second;
        if (nextAvailJoinId == it->first)
        {
            mNextAvailableGamegroupIt = mGamegroups.erase(it);
        }
        else
        {
            mGamegroups.erase(it);
            mNextAvailableGamegroupIt = mGamegroups.find(nextAvailJoinId);
        }

        addMetric(METRIC_GAMEGROUPS_UNREGISTERED);

        delete data;
    }
    else
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtil].unregisterGamegroup: attempted to remove a game group with Id (" << gid << ") that wasn't added.");
    }
}


//  Returns an available game group to join
GamegroupData* GamegroupsUtil::pickGamegroup(bool open)
{
    GamegroupData *pickedGamegroup = nullptr;
    if (open)
    {
        if (mGamegroups.size() == 0)
            return nullptr;

        if (mNextAvailableGamegroupIt == mGamegroups.end())
        {
            mNextAvailableGamegroupIt = mGamegroups.begin();
        }

        GameIdToDataMap::iterator initialIt = mNextAvailableGamegroupIt;

        //  find a game in the game map with room for a player to join.
        pickedGamegroup = mNextAvailableGamegroupIt->second;
        while (!pickedGamegroup->queryJoinGamegroup())
        {
            ++mNextAvailableGamegroupIt;
            if (mNextAvailableGamegroupIt == mGamegroups.end())
                mNextAvailableGamegroupIt = mGamegroups.begin();
            if (mNextAvailableGamegroupIt == initialIt)
                return nullptr;
            pickedGamegroup = mNextAvailableGamegroupIt->second;
        }
        ++mNextAvailableGamegroupIt;
    }
    else if (mGamegroups.size() != 0)
    {
        uint32_t roll = (uint32_t)(rand()) % static_cast<uint32_t>(mGamegroups.size());
        GameIdToDataMap::iterator it = mGamegroups.begin();
        for (uint32_t i = 0; i < roll && it != mGamegroups.end(); ++i, ++it);

        if (it != mGamegroups.end())
        {
            pickedGamegroup = it->second;
        }
    }

    return pickedGamegroup;
}


///////////////////////////////////////////////////////////////////////////////

GamegroupsUtilInstance::GamegroupsUtilInstance(GamegroupsUtil *util) :
    mUtil(util),
    mStressInst(nullptr),
    mProxy(nullptr),
    mGamegroupJoined(INVALID_GAME_ID),
    mRole(ROLE_NONE),
    mCyclesLeft(0),
    mPrimaryGameId(INVALID_GAME_ID)
{
}


GamegroupsUtilInstance::~GamegroupsUtilInstance()
{
    setStressInstance(nullptr);
}

BlazeRpcError GamegroupsUtilInstance::createOrJoinGamegroup()
{
    BlazeRpcError rpcResult = ERR_OK;
    CreateGameRequest request;
    request.getCommonGameData().setGameType(GAME_TYPE_GROUP);
    request.getGameCreationData().setNetworkTopology(NETWORK_DISABLED);

    GameSettings& gamegroupSettings = request.getGameCreationData().getGameSettings();
    gamegroupSettings.setHostMigratable();
    gamegroupSettings.setOpenToBrowsing();
    gamegroupSettings.setOpenToInvites();
    gamegroupSettings.setOpenToJoinByPlayer();
    gamegroupSettings.setOpenToMatchmaking();

    JoinGameResponse response;
    char8_t name[MAX_GAMENAME_LEN];
    blaze_snzprintf(name, sizeof(name), "ggutil_%d", this->getIdent());

    if (mUtil->getCreateOrJoinPref())
    {
        gamegroupSettings.setEnablePersistedGameId();
        request.setPersistedGameId(name);
    }

    IpAddress ipAddr;
    if (mStressInst == nullptr)
    {
        request.getCommonGameData().getPlayerNetworkAddress().setIpAddress(nullptr);
    }
    else
    {
        ipAddr.setIp(mStressInst->getConnection()->getAddress().getIp());
        ipAddr.setPort(mStressInst->getConnection()->getAddress().getPort());
        request.getCommonGameData().getPlayerNetworkAddress().setIpAddress(&ipAddr);
    }

    uint32_t ggCapacity = mUtil->getGamegroupMemberCapacity();
    request.getGameCreationData().setMaxPlayerCapacity(static_cast<uint16_t>(ggCapacity));
    request.getSlotCapacities()[SLOT_PUBLIC_PARTICIPANT] = static_cast<uint16_t>(ggCapacity);
    request.getGameCreationData().setGameName(name);

    //  generate attribute map
    if (mUtil->getAvgNumAttrs() > 0)
    {
        uint32_t numAttrs = rand() % (mUtil->getAvgNumAttrs()*2);
        if (numAttrs > 0)
        {
            Collections::AttributeMap &attrs = request.getGameCreationData().getGameAttribs();
            attrs.reserve(numAttrs);
            char attrname[Collections::MAX_ATTRIBUTENAME_LEN];
            char attrvalue[MAX_GAMENAME_LEN+16];
            for (uint32_t idx = 0; idx < numAttrs; ++idx)
            {
                blaze_snzprintf(attrname, sizeof(attrname), "%s_%u", mUtil->getAttrPrefix(), idx);
                blaze_snzprintf(attrvalue, sizeof(attrvalue), "%s_%u", name, idx);
                attrs[attrname] = attrvalue;
            }
        }
    }

    mUtil->incrementPendingGamegroupCount();
    rpcResult = mProxy->createOrJoinGame(request, response);
    if (rpcResult != ERR_OK)
    {
        //  failed to create, release this creator.
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].createOrJoinGamegroup: Error creating game group " << name);
        mUtil->decrementPendingGamegroupCount();
        return rpcResult;
    }

#if 1
    // TURD ALERT! Sleep 1s here to accomodate replication lag from master to search slaves
    // Until we avoid the lag by ensuring that getGameDataFromId requests go to the master,
    // we remain prone to the replication lag causing a failure in the lookup below even though
    // the gamegroup is successfully created above. This failure results in the group 'leaking'
    // because we early out below if we don't find the game, and a subsequent retry will create
    // another game group, and so forth.
    rpcResult = gSelector->sleep(1LL*1000*1000);
#endif

    mGamegroupJoined = response.getGameId();
    GetFullGameDataRequest getGameDataReq;
    GetFullGameDataResponse gameBrowserDataList;
    getGameDataReq.getGameIdList().push_back(mGamegroupJoined);
    rpcResult = mProxy->getFullGameData(getGameDataReq, gameBrowserDataList);

    ReplicatedGameData *ggData = nullptr;
    if ( gameBrowserDataList.getGames().empty() || ((ggData = &gameBrowserDataList.getGames().begin()->get()->getGame()) == nullptr) )
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].createOrJoinGamegroup: Error fetching data for game group " << name << ", id " << mGamegroupJoined);
        mGamegroupJoined = INVALID_GAME_ID;
        mUtil->decrementPendingGamegroupCount();
        return (rpcResult == ERR_OK ? ERR_SYSTEM : rpcResult);
    }

    bool createdNewGamegroup = ggData->getGameState() == INITIALIZING;
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].createOrJoinGamegroup: Player " << getBlazeId() << (createdNewGamegroup ? " created new" : " joined existing") << " game group: " << mGamegroupJoined);
    if (createdNewGamegroup)
    {
        mUtil->registerGamegroup(*ggData, gameBrowserDataList.getGames().front()->getGameRoster());

        // This game group was newly created by our createOrJoinGame request.
        // We need to advance the game state to allow other players to join.
        AdvanceGameStateRequest agsReq;
        agsReq.setGameId(mGamegroupJoined);
        agsReq.setNewGameState(GAME_GROUP_INITIALIZED);
        mProxy->advanceGameState(agsReq);
        mUtil->addMetric(GamegroupsUtil::METRIC_GAMEGROUPS_CREATED);
    }
    mUtil->decrementPendingGamegroupCount();

    scheduleUpdateGamePresenceForLocalUser(mGamegroupJoined, UPDATE_PRESENCE_REASON_GAME_CREATE);

    return rpcResult;
}


//  updates the game groups utility simulation - if an error occurs, handle the return based on how 
BlazeRpcError GamegroupsUtilInstance::execute()
{
    if (mCyclesLeft > 0)
        mCyclesLeft--;

    BlazeRpcError rpcResult = ERR_OK;

    switch (mRole)
    {
    case ROLE_LEADER:
        {
            if (mGamegroupJoined == INVALID_GAME_ID)
            {
                BlazeRpcError result = createOrJoinGamegroup();
                if (result != ERR_OK)
                    return result;
                mCyclesLeft = mUtil->getGamegroupLifespan() + (uint32_t)((rand() % mUtil->getGamegroupLifespan()) * 0.1);
            }
            else
            {
                if (mCyclesLeft == 0)
                {
                    GameId gid = mGamegroupJoined;
                    mGamegroupJoined = INVALID_GAME_ID;

                    if (mUtil->getGamegroupData(gid) == nullptr)
                    {
                        // No need to destroy this game group - it has already been unregistered, 
                        // probably because its last member has left
                        return rpcResult;
                    }

                    // update primary external session
                    scheduleUpdateGamePresenceForLocalUser(gid, UPDATE_PRESENCE_REASON_GAME_LEAVE, Blaze::GameManager::GAME_DESTROYED);

                    // unregister game group first to avoid having joiners try to join it while we are about to destroy it
                    mUtil->unregisterGamegroup(gid);
                    mUtil->addMetric(GamegroupsUtil::METRIC_GAMEGROUPS_DESTROYED);

                    DestroyGameRequest request;
                    DestroyGameResponse response;
                    request.setGameId(gid);
                    rpcResult = mProxy->destroyGame(request, response);

                    if (rpcResult != ERR_OK)
                    {
                        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].execute: Player " << getBlazeId() << " failed to destroy game group " << gid << ", error (" << ErrorHelp::getErrorName(rpcResult) << ")");
                    }
                    else
                    {
                        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].execute: Player " << getBlazeId() << " destroyed game group: " << gid);
                    }
                }
            }
        }
        break;
    case ROLE_JOINER:
        {
            if (mGamegroupJoined == INVALID_GAME_ID)
            {
                GamegroupData *pickedGroup = mUtil->pickGamegroup();
                if (pickedGroup!=nullptr)
                {
                    GameId gid = pickedGroup->getGamegroupId();
                    if (gid != INVALID_GAME_ID)
                    {
                        pickedGroup->incrementPendingEntryCount();
                        JoinGameResponse response;
                        EntryCriteriaError joinError;
                        if (mUtil->getCreateOrJoinPref())
                        {
                            rpcResult = createOrJoinGamegroup();
                        }
                        else
                        {
                            JoinGameRequest request;
                            request.setGameId(gid);
                            rpcResult = mProxy->joinGame(request, response, joinError);
                        }
                        if (rpcResult != ERR_OK)
                        {
                            BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].execute: Player " << getBlazeId() << " failed to join game group " << gid
                                << ", error (" << ErrorHelp::getErrorName(rpcResult) << ")");

                            //  soft errors - expected when other instances are joining game groups at the same time, 
                            //  or if a game group admin destroys the group while the request is going through.

                            if (rpcResult == GAMEMANAGER_ERR_PARTICIPANT_SLOTS_FULL)
                            {
                                mUtil->addMetric(GamegroupsUtil::METRIC_JOIN_GAMEGROUP_FULL);
                                rpcResult = ERR_OK;
                            }
                            else if (rpcResult == GAMEMANAGER_ERR_INVALID_GAME_ID)
                            {
                                rpcResult = ERR_OK;
                            }
                        }

                        if (rpcResult == ERR_OK)
                        {
                            mGamegroupJoined = response.getGameId();
                            mCyclesLeft = mUtil->getGamegroupMemberLifespan() + (uint32_t)((rand() % mUtil->getGamegroupMemberLifespan())*0.1);
                            mUtil->addMetric(GamegroupsUtil::METRIC_JOIN_ATTEMPTS);

                            BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].execute: Player " << getBlazeId() << " joined game group: " << mGamegroupJoined);
                        }

                        GamegroupData *pickedGameGroup = mUtil->getGamegroupData(gid);
                        if (pickedGameGroup != nullptr)
                        {
                            pickedGameGroup->decrementPendingEntryCount();
                        }
                        else
                        {
                            BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].execute: Failed to decrement pending entry count for game group: " << gid);
                        }
                    }
                }
                else
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].execute: Player " << getBlazeId() << " is waiting to pick an open game group...");
                }
            }
            else
            {
                if (mCyclesLeft == 0)
                {
                    GameId gid = mGamegroupJoined;
                    mGamegroupJoined = INVALID_GAME_ID;

                    RemovePlayerRequest req;
                    req.setGameId(gid);
                    req.setPlayerId(getBlazeId());
                    req.setPlayerRemovedReason(PLAYER_LEFT);
                    rpcResult = mProxy->removePlayer(req);

                    if (rpcResult == GAMEMANAGER_ERR_INVALID_GAME_ID)
                    {
                        rpcResult = ERR_OK;
                    }
                    else if (rpcResult == ERR_OK)
                    {
                        mUtil->addMetric(GamegroupsUtil::METRIC_LEAVE_ATTEMPTS);
                    }

                    if (rpcResult == ERR_OK)
                    {
                        // update primary external session, before erasing the local player
                        scheduleUpdateGamePresenceForLocalUser(gid, UPDATE_PRESENCE_REASON_GAME_LEAVE, Blaze::GameManager::PLAYER_LEFT);

                        size_t membersLeft;
                        if (mUtil->removeMember(gid, getBlazeId(), membersLeft))
                        {
                            BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].execute: Player " << getBlazeId()
                                << " has voluntarily left game group " << gid << ". " << membersLeft << " player(s) remain in the group. " );
                        }
                    }
                }
            }
        }
        break;
    default:
        break;
    }
    return rpcResult;
}

void GamegroupsUtilInstance::setJoinable(bool joinable)
{
    GamegroupData *gg = mUtil->getGamegroupData(mGamegroupJoined);
    if (gg != nullptr)
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].setJoinable: Set game group " << mGamegroupJoined  << " joinable = " << joinable);
        gg->setJoinable(joinable);
    }
}

//  if instance logs out, invoke this method to reset state
void GamegroupsUtilInstance::onLogout()
{
    mCyclesLeft = 0;

    // We may have lost connection to the blazeserver, so remove this player now
    // in case we don't get a PlayerRemoved notification
    // update primary external session, before erasing the local player
    if (mGamegroupJoined != INVALID_GAME_ID)
        scheduleUpdateGamePresenceForLocalUser(mGamegroupJoined, UPDATE_PRESENCE_REASON_GAME_LEAVE, BLAZESERVER_CONN_LOST);
    size_t membersLeft;
    if (mUtil->removeMember(mGamegroupJoined, getBlazeId(), membersLeft))
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].onLogout: Player " << getBlazeId()
            << " has logged out, leaving game group " << mGamegroupJoined << ". " << membersLeft << " player(s) remain in the group. " );
    }

    mGamegroupJoined = INVALID_GAME_ID;
}

void GamegroupsUtilInstance::setStressInstance(StressInstance *inst)
{
    if (inst == nullptr && mStressInst != nullptr)
    {
        StressConnection *conn = mStressInst->getConnection();
        conn->removeAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMEREMOVED, this);
        conn->removeAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLAYERREMOVED, this);
        conn->removeAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYADMINLISTCHANGE, this);
        conn->removeAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINCOMPLETED, this);
        conn->removeAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYHOSTMIGRATIONSTART, this);
        conn->removeAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYHOSTMIGRATIONFINISHED, this);

        delete mProxy;
    }
    if (inst != nullptr)
    {
        StressConnection *conn = inst->getConnection();

        mProxy = BLAZE_NEW GameManagerSlave(*conn);
        conn->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYGAMEREMOVED, this);
        conn->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLAYERREMOVED, this);
        conn->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYADMINLISTCHANGE, this);
        conn->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINCOMPLETED, this);
        conn->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYHOSTMIGRATIONSTART, this);
        conn->addAsyncHandler(GameManagerSlave::COMPONENT_ID, GameManagerSlave::NOTIFY_NOTIFYHOSTMIGRATIONFINISHED, this);
    }

    mStressInst = inst;
}

void GamegroupsUtilInstance::onAsync(uint16_t component, uint16_t type, RawBuffer* payload)
{
    if (component != GameManagerSlave::COMPONENT_ID)
        return;

    Heat2Decoder decoder;

    NotifyGameRemoved gameRemovedNotify;
    NotifyPlayerRemoved playerRemovedNotify;
    NotifyAdminListChange adminListChangeNotify;
    NotifyPlayerJoinCompleted joinNotify;
    NotifyHostMigrationStart hostMigStartNotify;
    NotifyHostMigrationFinished hostMigFinNotify;

    switch (type)
    {
    case GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINCOMPLETED:
        //  verify this notification is meant for this instance (not necessary since the notifications should be filtered by connection.)
        if (decoder.decode(*payload, joinNotify) != ERR_OK)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].onAsync: Error decoding notification " << (uint32_t)type << ".");
        }
        else
        {
            GameId gid = joinNotify.getGameId();
            GamegroupData *data = mUtil->getGamegroupData(gid);
            //Note: we'll skip below handling, if game type wasn't game group. Pre: GameIds are unique across game types, so we only handle game groups
            if (data == nullptr)
            {
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].onAsync: Ignoring PlayerJoinCompleted notification for unregistered GameId " << gid);
                break;
            }

            BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].onAsync: Player " << getBlazeId() << " received PlayerJoinCompleted notification for player "
                << joinNotify.getPlayerId() << " and game group " << gid);

            if (joinNotify.getPlayerId() == getBlazeId())
                mGamegroupJoined = gid;

            if (data->addMember(joinNotify.getPlayerId()))
                BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].onAsync: Player " << joinNotify.getPlayerId() << " has joined game group " << gid);

            // update primary external session
            scheduleUpdateGamePresenceForLocalUser(gid, UPDATE_PRESENCE_REASON_GAME_JOIN);
        }
        break;

    case GameManagerSlave::NOTIFY_NOTIFYGAMEREMOVED:
        //  verify this notification is meant for this instance (not necessary since the notifications should be filtered by connection.)
        if (decoder.decode(*payload, gameRemovedNotify) != ERR_OK)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].onAsync: Error decoding notification " << (uint32_t)type << ".");
        }
        else
        {
            GameId gid = gameRemovedNotify.getGameId();
            GamegroupData *data = mUtil->getGamegroupData(gid);
            if (data == nullptr)
            {
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].onAsync: Ignoring GameRemoved notification for unregistered GameId " << gid);
                break;
            }

            BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].onAsync: Player " << getBlazeId() << " received GameRemoved notification for game group " << gid);

            mGamegroupJoined = INVALID_GAME_ID;
            // update primary external session, before erasing local player
            scheduleUpdateGamePresenceForLocalUser(gid, UPDATE_PRESENCE_REASON_GAME_LEAVE, GAME_DESTROYED);

            mUtil->addMetric(GamegroupsUtil::METRIC_GAMEGROUPS_DESTROYED);
            mUtil->unregisterGamegroup(gid);
        }
        break;

    case GameManagerSlave::NOTIFY_NOTIFYPLAYERREMOVED:
        //  verify this notification is meant for this instance (not necessary since the notifications should be filtered by connection.)
        if (decoder.decode(*payload, playerRemovedNotify) != ERR_OK)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].onAsync: Error decoding notification " << (uint32_t)type << ".");
        }
        else
        {
            GameId gid = playerRemovedNotify.getGameId();
            if (mUtil->getGamegroupData(gid) == nullptr)
            {
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].onAsync: Ignoring PlayerRemoved notification for unregistered GameId " << gid);
                break;
            }

            BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].onAsync: Player " << getBlazeId() << " received PlayerRemoved notification for game group " 
                << gid << " and player " << playerRemovedNotify.getPlayerId() << ". Removal reason: " << PlayerRemovedReasonToString(playerRemovedNotify.getPlayerRemovedReason()));

            if (gid == mGamegroupJoined && playerRemovedNotify.getPlayerId() == getBlazeId())
            {
                // update primary external session, before erasing local player
                scheduleUpdateGamePresenceForLocalUser(gid, UPDATE_PRESENCE_REASON_GAME_LEAVE, PLAYER_LEFT);
                mGamegroupJoined = INVALID_GAME_ID;
            }

            size_t membersLeft;
            if (mUtil->removeMember(gid, playerRemovedNotify.getPlayerId(), membersLeft))
            {
                BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].onAsync: Player " << playerRemovedNotify.getPlayerId()
                    << " has left game group: " << gid << " with removal reason " << PlayerRemovedReasonToString(playerRemovedNotify.getPlayerRemovedReason()) << ". " << membersLeft << " player(s) remain. " );
            }
        }
        break;

    case GameManagerSlave::NOTIFY_NOTIFYADMINLISTCHANGE:
        if (decoder.decode(*payload, adminListChangeNotify) != ERR_OK)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].onAsync: Error decoding notification " << (uint32_t)type << ".");
        }
        else
        {
            GameId gid = adminListChangeNotify.getGameId();
            GamegroupData *data = mUtil->getGamegroupData(gid);
            if (data == nullptr)
            {
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].onAsync: Ignoring AdminListChange notification for unregistered GameId " << gid);
                break;
            }

            BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].onAsync: Player " << getBlazeId() << " received AdminListChange (" << adminListChangeNotify.getOperation()
                << ") notification for game group " << gid << " and player " << adminListChangeNotify.getAdminPlayerId() << ", updated by player " << adminListChangeNotify.getUpdaterPlayerId()); 

            UpdateAdminListOperation op = adminListChangeNotify.getOperation();
            if (op == GM_ADMIN_ADDED || op == GM_ADMIN_MIGRATED)
            {
                if (data->addAdmin(adminListChangeNotify.getAdminPlayerId()))
                    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].onAsync: Player " << adminListChangeNotify.getAdminPlayerId() << " is a new admin of game group " << gid);
                if (getBlazeId() == adminListChangeNotify.getAdminPlayerId() && mRole == ROLE_JOINER)
                    mRole = ROLE_LEADER;
            }

            if (op == GM_ADMIN_MIGRATED)
            {
                if (data->removeAdmin(adminListChangeNotify.getUpdaterPlayerId()))
                    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].onAsync: Player " << adminListChangeNotify.getUpdaterPlayerId() << " is no longer an admin of game group " << gid);
                if (getBlazeId() == adminListChangeNotify.getUpdaterPlayerId() && mRole == ROLE_LEADER)
                    mRole = ROLE_JOINER;
            }

            if (op == GM_ADMIN_REMOVED)
            {
                if (data->removeAdmin(adminListChangeNotify.getAdminPlayerId()))
                    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].onAsync: Player " << adminListChangeNotify.getAdminPlayerId() << " is no longer an admin of game group " << gid);
                if (getBlazeId() == adminListChangeNotify.getAdminPlayerId() && mRole == ROLE_LEADER)
                    mRole = ROLE_JOINER;
            }
        }
        break;

    case GameManagerSlave::NOTIFY_NOTIFYHOSTMIGRATIONSTART:
        if (decoder.decode(*payload, hostMigStartNotify) != ERR_OK)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].onAsync: Error decoding notification " << (uint32_t)type << ".");
        }
        else
        {
            GameId gid = hostMigStartNotify.getGameId();
            GamegroupData *data = mUtil->getGamegroupData(gid);
            if (data == nullptr)
            {
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].onAsync: Ignoring HostMigrationStart notification for unregistered GameId " << gid);
                break;
            }

            BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].onAsync: Player " << getBlazeId() << " received HostMigrationStart notification for game group "
                << gid << " and new host " << hostMigStartNotify.getNewHostId());

            data->setPendingHost(hostMigStartNotify.getNewHostId());
        }
        break;

    case GameManagerSlave::NOTIFY_NOTIFYHOSTMIGRATIONFINISHED:
        if (decoder.decode(*payload, hostMigFinNotify) != ERR_OK)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].onAsync: Error decoding notification " << (uint32_t)type << ".");
        }
        else
        {
            GameId gid = hostMigFinNotify.getGameId();
            GamegroupData *data = mUtil->getGamegroupData(gid);
            if (data == nullptr)
            {
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].onAsync: Ignoring HostMigrationFinished notification for unregistered GameId " << gid);
                break;
            }

            BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].onAsync: Player " << getBlazeId() << " received HostMigrationFinished notification for game group " << gid);

            if (data->updateHost())
                BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].onAsync: Player " << data->getHost() << " is now the host of game group " << gid);
        }
        break;

    default:
        break;
    }
}


BlazeId GamegroupsUtilInstance::getBlazeId() const 
{
    return (mStressInst != nullptr) ? mStressInst->getLogin()->getUserLoginInfo()->getBlazeId() : INVALID_BLAZE_ID;
}

void GamegroupsUtilInstance::scheduleUpdateGamePresenceForLocalUser(GameId changedGameId, UpdateExternalSessionPresenceForUserReason change, PlayerRemovedReason removeReason /*= SYS_PLAYER_REMOVE_REASON_INVALID*/)
{
    if (mUtil->getMockExternalSessions()[0] != '\0')
    {
        gSelector->scheduleFiberCall(this,  &GamegroupsUtilInstance::updateGamePresenceForLocalUser, changedGameId, change, removeReason, "GamegroupsUtilInstance::updateGamePresenceForLocalUser");
    }
}
// Based on BlazeSDK's GameManagerAPI method of the same name
void GamegroupsUtilInstance::updateGamePresenceForLocalUser(GameId changedGameId, UpdateExternalSessionPresenceForUserReason change, PlayerRemovedReason removeReason /*= SYS_PLAYER_REMOVE_REASON_INVALID*/)
{
    StressLogContextOverride stressLogContextOverride(getIdent(), getBlazeId());

    const GamegroupData* ggData = getGamegroupData();
    UpdateExternalSessionPresenceForUserRequest request;
    request.getUserIdentification().setBlazeId(getBlazeId());
    request.setChangedGameId(changedGameId);//deprecated, for back compatibility w/older servers.
    if (ggData)
    {
        populateGameActivity(request.getChangedGame(), *ggData);
        //(Stress tool only ever join at most one game group per user, so this should be it)
        populateGameActivity(*request.getCurrentGames().pull_back(), *ggData);
    }
    request.setChange(change);
    request.setOldPrimaryGameId(mPrimaryGameId);
    request.setRemoveReason(removeReason);
    // Note: Blaze Server will automatically set user's external id/blob (to be its BlazeId) when mock platform testing

    // skip left game if its still in our list.
    if ((ggData != nullptr) && ((change != UPDATE_PRESENCE_REASON_GAME_LEAVE) || (ggData->getGamegroupId() != changedGameId)))
    {
        populateGameActivity(*request.getCurrentGames().pull_back(), *ggData);
    }

    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].updateGamePresenceForLocalUser: Determining which of " << (ggData != nullptr? 1 : 0) <<
        " games (if any) to set as primary game for user(" << getBlazeId() << ") on " << UpdateExternalSessionPresenceForUserReasonToString(change) << " of game(" << changedGameId << ", gameType=GAMEGROUP.)");

    // Pre: user is logged into one client, so the client is the central authority on the games the user is in at any time.
    // The server has the logic for deciding which game (if any) should be designated as the primary game. Send the games and data to server.
    UpdateExternalSessionPresenceForUserResponse response;
    UpdateExternalSessionPresenceForUserErrorInfo errInfo;
    BlazeRpcError rpcResult = mProxy->updatePrimaryExternalSessionForUser(request, response, errInfo);
    if (rpcResult != ERR_OK)
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GamegroupsUtilInstance].updateGamePresenceForLocalUser: Update of primary game for user(" << getBlazeId() << ") result: " << ErrorHelp::getErrorName(rpcResult) << ".\n");
        mPrimaryGameId = errInfo.getCurrentPrimary().getGameId();
        return;
    }
    mPrimaryGameId = response.getCurrentPrimary().getGameId();
}

void GamegroupsUtilInstance::populateGameActivity(GameActivity& activity, const GamegroupData& game)
{
    activity.setGameId(game.getGamegroupId());
    activity.setGameType(GAME_TYPE_GROUP);
    activity.setPresenceMode(PRESENCE_MODE_STANDARD);//for simplicity
    game.getExternalSessionIdentification().copyInto(activity.getSessionIdentification());
    activity.getPlayer().setPlayerState(ACTIVE_CONNECTED);//for simplicity
    activity.getPlayer().setJoinedGameTimestamp(123);//for simplicity
    activity.getPlayer().setTeamIndex(0);//for simplicity
    activity.getPlayer().setSlotType(SLOT_PUBLIC_PARTICIPANT);//for simplicity

    // also set deprecated external session parameters. For backward compatibility with older servers.
    activity.setPlayerState(activity.getPlayer().getPlayerState());
    activity.setJoinedGameTimestamp(activity.getPlayer().getJoinedGameTimestamp());
    activity.setExternalSessionName(game.getExternalSessionIdentification().getXone().getSessionName());
    activity.setExternalSessionTemplateName(game.getExternalSessionIdentification().getXone().getTemplateName());
}

GamegroupData::GamegroupData(const ReplicatedGameData& data, const ReplicatedGamePlayerList &replPlayerData)
{
    mGamegroupId = data.getGameId();
    mHostId = data.getTopologyHostInfo().getPlayerId();
    mPendingHostId = INVALID_BLAZE_ID;
    blaze_strnzcpy(mPersistedGameId, data.getPersistedGameId(), sizeof(mPersistedGameId));
    mMaxPlayerCapacity = data.getSlotCapacities()[SLOT_PUBLIC_PARTICIPANT];
    mAdminCount = 0;
    mPendingEntryCount = 0;
    mJoinable = true;
    data.getExternalSessionIdentification().copyInto(mExternalSessionIdentification);

    for (PlayerIdList::const_iterator itr = data.getAdminPlayerList().begin(); itr != data.getAdminPlayerList().end(); ++itr)
    {
        if(addAdmin(*itr))
            BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GamegroupData].GamegroupData: Player " << *itr << " is an admin of game group " << mGamegroupId);
    }

    for (ReplicatedGamePlayerList::const_iterator itr = replPlayerData.begin(); itr != replPlayerData.end(); ++itr)
    {
        if (addMember((*itr)->getPlayerId()))
            BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GamegroupData].GamegroupData: Player " << (*itr)->getPlayerId() << " is a member of game group " << mGamegroupId);
    }
}


GamegroupData::~GamegroupData()
{
}

//  call to check whether the instance can join a game group
bool GamegroupData::queryJoinGamegroup() const
{
    if (mJoinable && (mMemberMap.size() + mPendingEntryCount) < mMaxPlayerCapacity)
        return true;

    return false;
}


bool GamegroupData::hasMember(BlazeId id) const
{
    MemberMap::const_iterator it = mMemberMap.find(id);
    return (it != mMemberMap.cend());
}

BlazeId GamegroupData::getMemberByIndex(size_t index) const
{
    return mMemberMap.at(index).first;
}

BlazeId GamegroupData::getNonHostNonAdminMemberByIndex(size_t index) const
{
    if ( getNonHostNonAdminMemberCount() == 0 )
        return INVALID_BLAZE_ID;

    size_t numMembers = mMemberMap.size();
    MemberMap::const_pointer itr = &(mMemberMap.at(index));
    for ( size_t newIndex=index+1; (newIndex % numMembers) != index; ++newIndex)
    {
        if (!itr->second && itr->first != mHostId)
            return itr->first;
        itr = &(mMemberMap.at(newIndex % numMembers));
    }

    return INVALID_BLAZE_ID;
}

size_t GamegroupData::getNonHostNonAdminMemberCount() const
{
    size_t nonAdminHost = isAdmin(mHostId) ? 0 : 1;
    return mMemberMap.size() - mAdminCount - nonAdminHost;
}

// Returns a non-host admin, unless the game group's
// only admin is also its host
BlazeId GamegroupData::getAdmin() const
{
    for (MemberMap::const_iterator itr = mMemberMap.cbegin(); itr != mMemberMap.cend(); ++itr)
    {
        if (itr->second && (mAdminCount == 1 || itr->first != mHostId))
            return itr->first;
    }
    return INVALID_BLAZE_ID;
}

bool GamegroupData::addAdmin(BlazeId blazeId)
{
    MemberMap::insert_return_type inserter = mMemberMap.insert(blazeId);
    if ( inserter.second || !(inserter.first->second) )
    {
        inserter.first->second = true;
        ++mAdminCount;
        return true;
    }
    return false;
}

bool GamegroupData::removeAdmin(BlazeId blazeId)
{
    MemberMap::iterator it = mMemberMap.find(blazeId);
    if (it != mMemberMap.end() && it->second)
    {
        it->second = false;
        --mAdminCount;
        return true;
    }
    return false;
}

bool GamegroupData::addMember(BlazeId blazeId)
{
    MemberMap::insert_return_type inserter = mMemberMap.insert(blazeId);
    if (inserter.second)
        inserter.first->second = false; // if this new member is actually an admin,
                                       // we'll update this when we get the AdminListChange notification
    return inserter.second;
}

bool GamegroupData::removeMember(BlazeId id)
{
    MemberMap::iterator it = mMemberMap.find(id);
    if (it == mMemberMap.end())
        return false;

    if (it->second)
        --mAdminCount;

    mMemberMap.erase(it);
    return true;
}

bool GamegroupData::isAdmin(BlazeId id) const
{
    MemberMap::const_iterator it = mMemberMap.find(id);
    if (it == mMemberMap.cend())
        return false;
    return it->second;
}

bool GamegroupData::updateHost()
{
    if (mPendingHostId != INVALID_BLAZE_ID)
    {
        mHostId = mPendingHostId;
        mPendingHostId = INVALID_BLAZE_ID;
        return true;
    }
    return false;
}

}   // Stress
}   // Blaze

