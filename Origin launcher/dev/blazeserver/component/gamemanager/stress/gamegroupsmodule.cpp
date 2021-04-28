/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GamegroupsModule

    Stress tests game groups.
    ---------------------------------------
    TODO.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "gamegroupsmodule.h"
#include "loginmanager.h"
#include "framework/connection/selector.h"
#include "framework/config/config_file.h"
#include "framework/config/config_map.h"
#include "framework/util/random.h"
#include "blazerpcerrors.h"
#include "framework/util/shared/blazestring.h"
#include "framework/util/shared/rawbuffer.h"

namespace Blaze
{
namespace Stress
{

/*
 * List of available RPCs that can be called by the module.
 */
static const char8_t* GAMEGROUPS_ACTION_STRINGSS[] =  {
    "noop",
    "getGameData"
};

// static
StressModule* GamegroupsModule::create()
{
    return BLAZE_NEW GamegroupsModule();
}


GamegroupsModule::GamegroupsModule()
{
    mAction = ACTION_INVALID;
  
    for (int i = 0; i < NUM_METRICS; ++i)
    {
        mMetricTime[i] = 0;
        mMetricCount[i] = 0;
    }

    mGameUtil = BLAZE_NEW Stress::GameManagerUtil();
    mGamegroupsUtil = BLAZE_NEW Stress::GamegroupsUtil();

    mConnsRemaining = 0;
    mRemainingLeaderCount = 0;
    mDoKickFreq = 0.0f;
    mDoKickHostFreq = 0.0f;
    mDoKickAdminFreq = 0.0f;
    mSetAttrFreq = 0.0f;
}

GamegroupsModule::~GamegroupsModule()
{
    delete mGamegroupsUtil;
    delete mGameUtil;
}

bool GamegroupsModule::parseConfig(const ConfigMap& config)
{
    //  sets frequency of RPC call for game group owners.
    mDoKickFreq = (float)config.getDouble("doKick", 0.0);
    mDoKickHostFreq = (float)config.getDouble("doKickHost", 0.0);
    mDoKickAdminFreq = (float)config.getDouble("doKickAdmin", 0.0);
    mSetAttrFreq = (float)config.getDouble("doSetAttr", 0.0f);

    return true;
}


bool GamegroupsModule::initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil)
{
    if (!StressModule::initialize(config, bootUtil)) 
        return false;

    if (!mGameUtil->initialize(config, bootUtil))
        return false;

    if (!mGamegroupsUtil->initialize(config, bootUtil))
        return false;

    if (!parseConfig(config)) 
        return false;

    mInitTimestamp = TimeValue::getTimeOfDay();

    mConnsRemaining = mTotalConns;
    mRemainingLeaderCount = mGamegroupsUtil->getGamegroupsFreq() > 0.f ? (uint32_t)((float)mTotalConns * mGamegroupsUtil->getGamegroupsFreq()) : 0;

    const char8_t* action = config.getString("action", "");
    if (blaze_stricmp(action, GAMEGROUPS_ACTION_STRINGSS[ACTION_NOOP]) == 0)
    {
        mAction = ACTION_NOOP;
    }
    else if (blaze_stricmp(action, GAMEGROUPS_ACTION_STRINGSS[ACTION_GET_GAME_DATA]) == 0)
    {
        mAction = ACTION_GET_GAME_DATA;
    }
    else
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GamegroupsModule].initialize: Unrecognized action: '" << action << "'");
        return false;
    }

    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[GamegroupsModule].initialize: " << action << " action selected.");
   
    return true;
}


// Called when the stress tester determines how many connections the module will use out of the total pool
void GamegroupsModule::setConnectionDistribution(uint32_t conns)
{
    mTotalConns = conns;
    StressModule::setConnectionDistribution(conns);
}


void GamegroupsModule::addMetric(Metric metric, const TimeValue& ms)
{
    mMetricTime[metric] += ms.getMicroSeconds();
    mMetricCount[metric]++;
}


bool GamegroupsModule::saveStats()
{
    mGamegroupsUtil->dumpStats(this);
    return false;
}

StressInstance* GamegroupsModule::createInstance(StressConnection* connection, Login* login)
{
    GamegroupsInstance *instance = BLAZE_NEW GamegroupsInstance(this, connection, login);
    InstanceMap::insert_return_type inserter = mInstances.insert(instance->getIdent());
    if (inserter.first == mInstances.end())
    {
        delete instance;
        instance = nullptr;
    }
    else
    {
        inserter.first->second = instance;
        
        if ( mRemainingLeaderCount <= mConnsRemaining ||
            (mRemainingLeaderCount > 0 && mConnsRemaining > 0 && (Random::getRandomFloat() <= (float)mRemainingLeaderCount / (float)mConnsRemaining) ))
        {
            instance->setRole(GamegroupsUtilInstance::ROLE_LEADER);
            mRemainingLeaderCount--;
        }
        else
        {
            instance->setRole(GamegroupsUtilInstance::ROLE_JOINER);
        }
        --mConnsRemaining;
    }
    return instance;
}


void GamegroupsModule::releaseInstance(StressInstance *instance)
{
    mInstances.erase(instance->getIdent());
}


GamegroupsInstance::GamegroupsInstance(GamegroupsModule *owner, StressConnection* connection, Login* login) :
    StressInstance(owner, connection, login, BlazeRpcLog::gamemanager),
    mOwner(owner),
    mProxy(BLAZE_NEW GameManagerSlave(*getConnection())),
    mTrialIndex(0),
    mGameInst(owner->getGameManagerUtil()),
    mGamegroupInst(owner->getGamegroupsUtil())
{
    mGameInst.setStressInstance(this);
    mGamegroupInst.setStressInstance(this);
}


GamegroupsInstance::~GamegroupsInstance()
{
    mGameInst.setStressInstance(nullptr);
    mGamegroupInst.setStressInstance(nullptr);

    if (mProxy != nullptr)
    {
        delete mProxy;
    }

    mOwner->releaseInstance(this);
}


BlazeId GamegroupsInstance::getBlazeId() const 
{
    return getLogin()->getUserLoginInfo()->getBlazeId();
}


const char8_t *GamegroupsInstance::getName() const
{
    return GAMEGROUPS_ACTION_STRINGSS[mOwner->getAction()];
}


////////////////////////////////////////////////////////////////////////////////////////

void GamegroupsInstance::start()
{
    StressInstance::start();
}

void GamegroupsInstance::onLogin(BlazeRpcError result)
{
    if (!getOwner()->getUseServerQosSettings())
        updateNetworkInfo();
}

BlazeRpcError GamegroupsInstance::execute()
{
    BlazeRpcError rpcResult = ERR_OK;
    mTrialIndex++;

    rpcResult = mGameInst.execute();
    if (rpcResult != ERR_OK)
        return rpcResult;

    rpcResult = mGamegroupInst.execute();
    if (rpcResult != ERR_OK)
        return rpcResult;

    GamegroupsModule::Action action = mOwner->getAction();

    //  EXECUTE STRESS ACTIONS
    GameId localGamegroupId = mGamegroupInst.getGamegroupId();
    if (localGamegroupId == INVALID_GAME_ID)
        return rpcResult;

    const GamegroupData *myGG = mGamegroupInst.getGamegroupData();

    if (mGamegroupInst.getRole() == GamegroupsUtilInstance::ROLE_LEADER && myGG != nullptr && myGG->getMemberCount() > 0)
    {
        //  OWNER BASED STRESS ACTIONS
        int32_t rollAction = rand() % 100;
        int32_t kickActionThreshold = (int32_t)(mOwner->queryDoKickFreq() * 100);
        int32_t kickHostActionThreshold = (int32_t)(kickActionThreshold + mOwner->queryDoKickHostFreq() * 100);
        int32_t kickAdminActionThreshold = (int32_t)(kickHostActionThreshold + mOwner->queryDoKickAdminFreq() * 100);
        int32_t setAttrThreshold = (int32_t)(kickAdminActionThreshold + mOwner->queryDoSetAttrFreq() * 100);

        // Don't kick the last player in the game group (i.e. this isn't here to stress game group destruction.)
        if (rollAction < kickAdminActionThreshold && myGG->getMemberCount() > 1)
        {
            BlazeId playerToKick = INVALID_BLAZE_ID;
            if (rollAction < kickActionThreshold)
            {
                size_t memberIndex = (size_t)(rand()) % (myGG->getNonHostNonAdminMemberCount());
                playerToKick = myGG->getNonHostNonAdminMemberByIndex(memberIndex);
            }
            else if (rollAction < kickHostActionThreshold)
            {
                playerToKick = myGG->getHost();
            }
            else
            {
                playerToKick = myGG->getAdmin();
            }

            if (playerToKick != INVALID_BLAZE_ID && mPendingKicks.insert(playerToKick).second)
            {
                RemovePlayerRequest request;
                request.setGameId(localGamegroupId);
                request.setPlayerId(playerToKick);
                request.setPlayerRemovedReason(PLAYER_KICKED);
                rpcResult = mProxy->removePlayer(request);
                mPendingKicks.erase(playerToKick);
            }

            return rpcResult;
        }
        else if (rollAction >= kickAdminActionThreshold && rollAction < setAttrThreshold)
        {
            SetGameAttributesRequest req;
            req.setGameId(localGamegroupId);
            AttrValueUtil::fillAttributeMap(mGamegroupInst.getUtil()->getGamegroupAttrValues(), req.getGameAttributes());
            rpcResult = mProxy->setGameAttributes(req);

            return rpcResult;
        }
    }
    else if (mGamegroupInst.getRole() != GamegroupsUtilInstance::ROLE_LEADER && myGG != nullptr)
    {
        //  NON-OWNER BASED STRESS ACTIONS
        switch (action)
        {
        case GamegroupsModule::ACTION_GET_GAME_DATA:
            {
                //  find a game group in the instance list.
                GamegroupData *ggData = mOwner->getGamegroupsUtil()->pickGamegroup(false);
                if (ggData != nullptr)
                {
                    GameId gid = ggData->getGamegroupId();
                    //  execute simple lookup
                    GetGameDataFromIdRequest request;
                    request.getGameIds().push_back(gid);
                    GameBrowserDataList list;
                    rpcResult = mProxy->getGameDataFromId(request, list);
                    if (rpcResult == GAMEMANAGER_ERR_INVALID_GAME_ID)
                    {
                        //  soft errors - expected if the game group admin destroys his group while the request is going through.
                        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GamegroupsInstance].execute: Error " << rpcResult << "(" 
                            << Blaze::ErrorHelp::getErrorName(rpcResult) 
                            << ") looking up a game group (id=" << gid << ") ?");
                        rpcResult = ERR_OK;
                    }
                    else if (rpcResult != ERR_OK)
                    {
                        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[GamegroupsInstance].execute: Error " << rpcResult << "(" << Blaze::ErrorHelp::getErrorName(rpcResult) 
                            << "looking up game group (id=" << gid << ") ?");
                    }
                }
            }
            return rpcResult;

        default:
            break;
        }
    }

    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[GamegroupsInstance].execute: " 
        << (getConnection()->connected() ? 1 : 0) << " : end");

    return rpcResult;
}

} // Stress
} // Blaze

