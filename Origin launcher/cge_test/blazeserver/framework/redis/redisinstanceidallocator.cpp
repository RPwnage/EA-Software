/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/redis/redisinstanceidallocator.h"
#include "framework/redis/redismanager.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


namespace Blaze
{

/*
   msGetOrRefreshInstanceIdScript: Exchanges an instance name for an InstanceId (implemented as a Lua script for atomicity).
  
  Arguments:
   [1] instance name
   [2] current instance id
   [3] TTL of instance id allocation
   [4] maximum valid instance id

   Note that redis maintains two maps for instance id allocation:
   (1) a map of instance ids by instance name, in which the entries expire after the TTL (Example:<coreSlave1, 42> : 2m, <coreSlave2, 52>: 2m).
   (2) a map of instance names by instance id, in which the entries do not expire (Example: <42, coreSlave1>, <52, coreSlave2>)

   The map of instance names by instance id provides a quick way to identify which instance ids may be in use when the instance id counter rolls over.
   It also allows the script to refresh instance id allocations for Blaze instances that obtained an initial instance id, but then lost connectivity 
   to the instance id allocator while the TTL expired. This ensures that the instance id associated with a given instance name can only change after
   the instance is restarted, or a new instance is being assigned an instance id.

   A field-map of all known instance names is also maintained. (Redis field-maps do not support expiration by TTL.)
   This provides a way to get all current (and previous) instance names without using SCAN or KEYS.
   Since the fields do not expire, the field-map may contain instance names that are no longer in use.
   As such, other data sources (such as the map of instance ids by instance name) should be used to validate the instance's existence before use.
   A dummy value of "1" is stored for the field to emphasize that the other map should be used to retrieve the instance id, if needed.
 */

const char8_t* RedisInstanceIdAllocator::msInstanceIdAllocatorHashTag = "InstanceIdAllocator";
const char8_t* RedisInstanceIdAllocator::msInstanceNameLedgerKey = "{InstanceIdAllocator}.InstanceNamesLedger";

RedisScript RedisInstanceIdAllocator::msGetOrRefreshInstanceIdScript(1,
    "local keyPrefix = '{'..KEYS[1]..'}.';"
    "local instanceIdsByName = keyPrefix..'InstanceIdsByName.'..ARGV[1];"

    "local currentid = redis.call('GET', instanceIdsByName);"
    "local isExpired = (not currentid);"
    "if (isExpired and (tonumber(ARGV[2]) ~= 0)) then " // If the value is not in expiring map but instance thinks it has a legit value.
        "local currentName = redis.call('GET', keyPrefix..'InstanceNamesById.'..ARGV[2]);" // If the unexpiring map knows that this instance was around and had the id it thinks it has, give this guy back his id.
        "if (currentName == ARGV[1]) then "
            "currentid = ARGV[2];"
        "end;"
    "end;"

    "if (currentid) then " // If the instance has succeeded in getting id by expiring or unexpiring map, add it's new expiry and return.
        "if (isExpired) then "
            "redis.call('SET', instanceIdsByName, currentid, 'EX', ARGV[3]);"
            "redis.call('HSET', keyPrefix..'InstanceNamesLedger', ARGV[1], '1');"
        "else "
            "redis.call('EXPIRE', instanceIdsByName, ARGV[3]);"
        "end;"
        "return tonumber(currentid);"
    "end;"

    // At this point, we took care of the running instance that might have suffered network partitioning or was executing happy path. 
    // Now, we have a situation where we need to allocate a new id using our counter key for a newly started instance.
    "local counter = keyPrefix..'InstanceIdCounter';" 
    "local newid = nil;"
    "local nextid = redis.call('INCR', counter);"
    "if (tonumber(nextid) > tonumber(ARGV[4])) then " //If overflowing, let's roll over the counter to 1.
        "redis.call('SET', counter, '1');"
        "nextid = 1;"
    "end;"

    "local startid = tonumber(nextid);" // We found a starting value based on the counter, repeat until we can make sure that no one else has it.
    "repeat "
    "local existingInstanceName = redis.call('GET', keyPrefix..'InstanceNamesById.'..nextid);"//check the id in the unexpiring map
    "if (not existingInstanceName) then " //no one has it so give it to the guy requesting it.
        "newid = nextid;"
    "else "
        "local idInUse = redis.call('GET', keyPrefix..'InstanceIdsByName.'..existingInstanceName);" // someone seems to have this id (may be we are rebooting), what is it?
        "if (tonumber(nextid) ~= tonumber(idInUse)) then " // Change our id.
            "newid = nextid;"
        "else "
            "nextid = redis.call('INCR', counter);"
            "if (tonumber(nextid) > tonumber(ARGV[4])) then "
                "redis.call('SET', counter, '1');"
                "nextid = 1;"
            "end;"
        "end;"
    "end;"
    "until(newid ~= nil or tonumber(nextid) == startid);" // if we have reached our startId, we have made a loop and exhausted ourselves. Bail out.

    "if (newid == nil) then "
        "return redis.error_reply('No available instance ids');"
    "end;"

    "redis.call('SET', keyPrefix..'InstanceNamesById.'..newid, ARGV[1]);"
    "redis.call('SET', instanceIdsByName, newid, 'EX', ARGV[3]);"
    "redis.call('HSET', keyPrefix..'InstanceNamesLedger', ARGV[1], '1');"
    "return tonumber(newid);");


RedisScript RedisInstanceIdAllocator::msCheckAndCleanupInstanceNameOnLedgerScript(1,
    "local keyPrefix = '{'..KEYS[1]..'}.';"
    "local instanceIdsByName = keyPrefix..'InstanceIdsByName.'..ARGV[1];"

    "local currentid = redis.call('GET', instanceIdsByName);"
    "if (not currentid) then "
        "redis.call('HDEL', keyPrefix..'InstanceNamesLedger', ARGV[1]);"
        "currentid = 0;"
    "end;"

    "return tonumber(currentid);");


InstanceId RedisInstanceIdAllocator::getOrRefreshInstanceId(const char8_t* instanceName, InstanceId instanceId, TimeValue ttl)
{
    InstanceId newInstanceId = INVALID_INSTANCE_ID;

    RedisCluster* cluster = gRedisManager->getRedisCluster(RedisManager::FRAMEWORK_NAMESPACE);
    if (cluster != nullptr)
    {
        RedisCommand cmd;
        cmd.addKey(msInstanceIdAllocatorHashTag);
        cmd.add(instanceName);
        cmd.add(instanceId);
        cmd.add(ttl.getSec());
        cmd.add(INSTANCE_ID_MAX);

        RedisResponsePtr resp;
        RedisError redisErr = cluster->execScript(msGetOrRefreshInstanceIdScript, cmd, resp);
        if ((redisErr == REDIS_ERR_OK) && resp->isInteger())
        {
            newInstanceId = (InstanceId)resp->getInteger();
        }
        else
        {
            BLAZE_ERR_LOG(Log::REDIS, "[RedisInstanceIdAllocator::getOrRefreshInstanceId]: failed to retrieve integer response, redisErr: " << redisErr);
        }
    }
    else
    {
        BLAZE_ERR_LOG(Log::REDIS, "[RedisInstanceIdAllocator::getOrRefreshInstanceId]: getRedisCluster returned nullptr for namespace: " << RedisManager::FRAMEWORK_NAMESPACE);
    }

    return newInstanceId;
}

InstanceId RedisInstanceIdAllocator::checkAndCleanupInstanceNameOnLedger(const eastl::string& instanceName)
{
    InstanceId currentInstanceId = INVALID_INSTANCE_ID;

    RedisCluster* cluster = gRedisManager->getRedisCluster(RedisManager::FRAMEWORK_NAMESPACE);
    if (cluster != nullptr)
    {
        RedisCommand cmd;
        cmd.addKey(msInstanceIdAllocatorHashTag);
        cmd.add(instanceName);

        RedisResponsePtr resp;
        RedisError redisErr = cluster->execScript(msCheckAndCleanupInstanceNameOnLedgerScript, cmd, resp);
        if ((redisErr == REDIS_ERR_OK) && resp->isInteger())
        {
            currentInstanceId = (InstanceId)resp->getInteger();
        }
        else
        {
            BLAZE_ERR_LOG(Log::REDIS, "[RedisInstanceIdAllocator::checkAndCleanupInstanceNameOnLedger]: failed to retrieve integer response, redisErr: " << redisErr);
        }
    }
    else
    {
        BLAZE_ERR_LOG(Log::REDIS, "[RedisInstanceIdAllocator::checkAndCleanupInstanceNameOnLedger]: getRedisCluster returned nullptr for namespace: " << RedisManager::FRAMEWORK_NAMESPACE);
    }

    return currentInstanceId;
}

void RedisInstanceIdAllocator::getInstanceNamesLedger(eastl::vector<eastl::string>& instanceNames)
{
    RedisCluster* cluster = gRedisManager->getRedisCluster(RedisManager::FRAMEWORK_NAMESPACE);
    if (cluster != nullptr)
    {
        RedisCommand cmd;
        cmd.add("HGETALL");
        cmd.addKey(msInstanceNameLedgerKey);

        RedisResponsePtr resp;
        RedisError redisErr = cluster->exec(cmd, resp);
        if (redisErr == REDIS_ERR_OK && resp->isArray())
        {
            uint32_t arraySize = resp->getArraySize();

            // Expect field/value pairs in the array response
            if (arraySize % 2 != 0)
                return;

            for (uint32_t i = 0; i < arraySize; ++i)
            {
                const RedisResponse& fieldElement = resp->getArrayElement(i);
                ++i; // valueElement is ignored

                if (fieldElement.isString())
                {
                    instanceNames.push_back(fieldElement.getString());
                }
            }
        }
        else
        {
            BLAZE_ERR_LOG(Log::REDIS, "[RedisInstanceIdAllocator::getOrRefreshInstanceId]: failed to retrieve array response, redisErr: " << redisErr);
        }
    }
    else
    {
        BLAZE_ERR_LOG(Log::REDIS, "[RedisInstanceIdAllocator::getInstanceNames]: getRedisCluster returned nullptr for namespace: " << RedisManager::FRAMEWORK_NAMESPACE);
    }
}

} // Blaze

