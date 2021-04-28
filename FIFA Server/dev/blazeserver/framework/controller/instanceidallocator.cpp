/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/instanceidallocator.h"
#include "framework/redis/redismanager.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


namespace Blaze
{

const char8_t* InstanceIdAllocator::msInstanceIdAllocatorHashTag = "InstanceIdAllocator";
const char8_t* InstanceIdAllocator::msInstanceNameLedgerKey = "{InstanceIdAllocator}.InstanceNamesLedger";
const char8_t* InstanceIdAllocator::msInstanceNameLedgerVersionInfoKey = "{InstanceIdAllocator}.InstanceNamesLedger.VersionInfo";

RedisScript InstanceIdAllocator::msGetOrRefreshInstanceIdScript(1, "../framework/controller/db/redis/allocateinstanceid.lua", true);
RedisScript InstanceIdAllocator::msCheckAndCleanupInstanceNameOnLedgerScript(1, "../framework/controller/db/redis/auditledger.lua", true);

InstanceId InstanceIdAllocator::getOrRefreshInstanceId(const char8_t* instanceName, InstanceId instanceId, TimeValue ttl)
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
            newInstanceId = static_cast<InstanceId>(resp->getInteger());
        }
        else
        {
            BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceIdAllocator::getOrRefreshInstanceId]: failed to retrieve integer response, redisErr: " << redisErr);
        }
    }
    else
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceIdAllocator::getOrRefreshInstanceId]: getRedisCluster returned nullptr for namespace: " << RedisManager::FRAMEWORK_NAMESPACE);
    }

    return newInstanceId;
}

InstanceId InstanceIdAllocator::checkAndCleanupInstanceNameOnLedger(const eastl::string& instanceName)
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
            currentInstanceId = static_cast<InstanceId>(resp->getInteger());
        }
        else
        {
            BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceIdAllocator::checkAndCleanupInstanceNameOnLedger]: failed to retrieve integer response, redisErr: " << redisErr);
        }
    }
    else
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceIdAllocator::checkAndCleanupInstanceNameOnLedger]: getRedisCluster returned nullptr for namespace: " << RedisManager::FRAMEWORK_NAMESPACE);
    }

    return currentInstanceId;
}

void InstanceIdAllocator::getInstanceNamesLedgerVersionInfo(eastl::string& versionInfo)
{
    RedisCluster* cluster = gRedisManager->getRedisCluster(RedisManager::FRAMEWORK_NAMESPACE);
    if (cluster != nullptr)
    {
        RedisCommand cmd;
        cmd.add("GET");
        cmd.addKey(msInstanceNameLedgerVersionInfoKey);

        RedisResponsePtr resp;
        RedisError redisErr = cluster->exec(cmd, resp);
        if (redisErr == REDIS_ERR_OK && resp->isString())
        {
            versionInfo = resp->getString();
        }
        else
        {
            BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceIdAllocator::getInstanceNamesLedgerVersionInfo]: failed to retrieve string response, redisErr: " << redisErr);
        }
    }
    else
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceIdAllocator::getInstanceNamesLedgerVersionInfo]: getRedisCluster returned nullptr for namespace: " << RedisManager::FRAMEWORK_NAMESPACE);
    }
}

void InstanceIdAllocator::getInstanceNamesLedger(eastl::vector<eastl::string>& instanceNames)
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
            BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceIdAllocator::getInstanceNamesLedger]: failed to retrieve array response, redisErr: " << redisErr);
        }
    }
    else
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceIdAllocator::getInstanceNamesLedger]: getRedisCluster returned nullptr for namespace: " << RedisManager::FRAMEWORK_NAMESPACE);
    }
}

} // Blaze

