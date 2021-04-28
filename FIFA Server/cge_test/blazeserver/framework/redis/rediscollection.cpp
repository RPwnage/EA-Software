/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "EASTL/algorithm.h"

#include "framework/redis/rediscollectionmanager.h"
#include "framework/redis/rediscollection.h"
#include "framework/redis/redismanager.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

// Note: These are Lua scripts, so KEYS[1] & ARGV[1] refers to the first key and first argument (Lua is 1-indexed).
RedisScript RedisCollection::msDecrRemoveWhenZero(1, "local result = redis.call('DECR', KEYS[1]); if (result == 0) then redis.call('DEL', KEYS[1]); end; return result;");
RedisScript RedisCollection::msDecrFieldRemoveWhenZero(1, "local result = redis.call('HINCRBY', KEYS[1], ARGV[1], -1); if (result == 0) then redis.call('HDEL', KEYS[1], ARGV[1]); end; return result;");
RedisScript RedisCollection::msGetAllByKeys(0, "local response = {}; for k,v in pairs(ARGV) do local result = redis.call('HGETALL', v); if (next(result) ~= nil) then table.insert(response,v); table.insert(response, result); end; end; return response;");

bool RedisCollection::registerCollection()
{
    if (!mIsRegistered)
    {
        RedisCluster* cluster = gRedisManager->getRedisCluster(mCollectionId.getNamespace());
        if (cluster != nullptr)
            mIsRegistered = cluster->getCollectionManager().registerCollection(*this);
    }
    return mIsRegistered;
}

bool RedisCollection::deregisterCollection()
{
    if (mIsRegistered)
    {
        mIsRegistered = false;
        RedisCluster* cluster = gRedisManager->getRedisCluster(mCollectionId.getNamespace());
        if (cluster != nullptr)
            return cluster->getCollectionManager().deregisterCollection(*this);
    }
    return false;
}

RedisCollection::CollectionKey RedisCollection::getCollectionKey(SliverId sliverId /*= INVALID_SLIVER_ID*/) const
{
    EA_ASSERT_MSG(!mIsSharded || sliverId != INVALID_SLIVER_ID, "Sharded collection requires a valid sliver id!");
    return CollectionKey(mCollectionId, RedisSliver(sliverId, mIsSharded));
}

RedisError RedisCollection::execCommand(RedisCommand& cmd, int64_t* result)
{
    RedisResponsePtr resp;

    RedisError rc = exec(cmd, resp);
    if (rc == REDIS_ERR_OK)
    {
        if (result != nullptr)
        {
            if (resp->isInteger())
                *result = resp->getInteger();
            else if (resp->isStatus())
                *result = strcmp(resp->getString(), "OK") == 0;
            else if (resp->isEmpty())
                *result = -1;
        }
    }
    return rc;
}

RedisError RedisCollection::exec(const RedisCommand &command, RedisResponsePtr &response)
{
    RedisCluster* cluster = gRedisManager->getRedisCluster(command.getNamespace());
    if (cluster != nullptr)
        return cluster->exec(command, response);

    return REDIS_ERR_SYSTEM;
}

RedisError RedisCollection::execScript(const RedisScript& script, const RedisCommand &command, RedisResponsePtr &response)
{
    RedisCluster* cluster = gRedisManager->getRedisCluster(command.getNamespace());
    if (cluster != nullptr)
        return cluster->execScript(script, command, response);

    return REDIS_ERR_SYSTEM;
}

RedisError RedisCollection::execScriptByMasterName(const RedisScript& script, const RedisCommandsByMasterNameMap &commandMap, eastl::vector<RedisResponsePtr> &results)
{
    RedisError rc = REDIS_ERR_OK;
    for (RedisCommandsByMasterNameMap::const_iterator itr = commandMap.cbegin(); itr != commandMap.cend(); ++itr)
    {
        for (RedisCommandList::const_iterator cmdItr = itr->second.cbegin(); cmdItr != itr->second.cend(); ++cmdItr)
        {
            RedisCommand* cmd = *cmdItr;
            RedisCluster* cluster = gRedisManager->getRedisCluster(cmd->getNamespace());
            if (cluster == nullptr)
                return REDIS_ERR_SYSTEM;

            RedisResponsePtr resp;
            rc = cluster->execScriptByMasterName(itr->first.c_str(), script, *cmd, resp);

            if (rc != REDIS_ERR_OK)
                return rc;
            results.push_back(resp);
        }
    }

    return rc;
}


bool RedisCollection::addParamToRedisCommandsByMasterNameMap(const char8_t* param, size_t paramlen, RedisCommandsByMasterNameMap &commandMap, uint32_t maxParamsPerCommand) const
{
    RedisCluster* cluster = gRedisManager->getRedisCluster(mCollectionId.getNamespace());
    if (cluster == nullptr)
        return false;

    const char8_t* masterName = cluster->getRedisMasterNameForKey(param, paramlen);
    if (masterName == nullptr)
        return false;

    RedisCommandList& cmdList = commandMap[masterName];
    RedisCommand* cmd = nullptr;
    if (cmdList.empty())
    {
        cmd = BLAZE_NEW RedisCommand();
        cmdList.push_back(cmd);
    }
    else
    {
        cmd = cmdList.back();
        if (cmd->getParamCount() >= maxParamsPerCommand)
        {
            cmd = BLAZE_NEW RedisCommand();
            cmdList.push_back(cmd);
        }
    }

    cmd->setNamespace(mCollectionId.getNamespace());

    cmd->begin();
    cmd->add(param);
    cmd->end();

    return true;
}

} // Blaze
