/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class RedisManager

    Manages a list of redis clusters, and provides support to connect to and issue commands to a cluster of Redis servers.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/redis/redismanager.h"

namespace Blaze
{

const char8_t* RedisManager::FRAMEWORK_NAMESPACE = "framework";
const char8_t* RedisManager::UNITTESTS_NAMESPACE = "unittests";

RedisManager::RedisManager() :
    mRedisClusterByNameMap(BlazeStlAllocator("mRedisClusterByNameMap")),
    mOwnedLocks(RedisId(FRAMEWORK_NAMESPACE, "mOwnedLocks")),
    mActiveFiberGroupId(Fiber::INVALID_FIBER_GROUP_ID)
{
}

RedisError RedisManager::initFromConfig(const RedisConfig& config)
{
    mActiveFiberGroupId = Fiber::allocateFiberGroupId();

    config.copyInto(mRedisConfig);

    // create redis cluster from config
    const char8_t* defaultClusterName = mRedisConfig.getDefaultAlias();
    const RedisClustersMap& redisClusterConfigMap = mRedisConfig.getClusters();
    if (redisClusterConfigMap.empty())
    {
        RedisClusterByNameMap::insert_return_type insertResult = mRedisClusterByNameMap.insert(defaultClusterName);
        if (insertResult.second)
        {
            RedisCluster* cluster = BLAZE_NEW RedisCluster(defaultClusterName);
            insertResult.first->second = cluster;
            mRedisClusterList.push_back(cluster);
        }
    }
    else
    {
        for (RedisClustersMap::const_iterator it = redisClusterConfigMap.begin(); it != redisClusterConfigMap.end(); ++it)
        {
            const char8_t* clusterName = it->first.c_str();
            RedisClusterByNameMap::insert_return_type insertResult = mRedisClusterByNameMap.insert(clusterName);
            if (insertResult.second)
            {
                RedisCluster* cluster = BLAZE_NEW RedisCluster(clusterName);
                insertResult.first->second = cluster;
                mRedisClusterList.push_back(cluster);
            }
            else
            {
                BLAZE_ERR_LOG(Log::REDIS, "RedisManager.initFromConfig: Unable to add cluster to map. Multiple clusters exist with name (" << clusterName << ").");
                return  REDIS_ERR_INVALID_CONFIG;
            }
        }
    }

    // Populate alias map from config
    RedisClusterByAliasMap& aliasesMap = mRedisConfig.getAliases();
    for (RedisClusterByAliasMap::iterator itr = aliasesMap.begin(); itr != aliasesMap.end(); ++itr)
    {
        // Insert the alias to the map, and the value is pointing to the corresponding cluster
        const char8_t* aliasName = itr->first.c_str();
        const char8_t* clusterName = itr->second.c_str();
        RedisClusterByNameMap::iterator nameItr = mRedisClusterByNameMap.find(clusterName);
        if (nameItr != mRedisClusterByNameMap.end())
        {
            RedisClusterByNameMap::insert_return_type insertResult = mRedisClusterByNameMap.insert(aliasName);
            insertResult.first->second = nameItr->second;
        }
        else
        {
            BLAZE_ERR_LOG(Log::REDIS, "RedisManager.initFromConfig: cluster (" << clusterName << ") in aliases for namespace " << aliasName << "does not exist.");
            return REDIS_ERR_INVALID_CONFIG;
        }
    }
    
    typedef eastl::vector<const char8_t*> RedisNameList;
    RedisNameList redisNameList;
    size_t componentSize = ComponentData::getComponentCount();
    for (size_t i = 0; i < componentSize; ++i)
    {
        const ComponentData* data = ComponentData::getComponentDataAtIndex(i);
        if (data != nullptr)
        {
            const char8_t* namespaceName = data->name;
            redisNameList.push_back(namespaceName);
        }
    }
    redisNameList.push_back(FRAMEWORK_NAMESPACE);
    redisNameList.push_back(UNITTESTS_NAMESPACE);

    RedisClusterByNameMap::const_iterator defaultItr = mRedisClusterByNameMap.find(defaultClusterName);
    if (defaultItr == mRedisClusterByNameMap.end())
    {
        BLAZE_ERR_LOG(Log::REDIS, "RedisManager.initFromConfig: Default redis cluster (" << defaultClusterName << ") does not exist.");
        return  REDIS_ERR_INVALID_CONFIG;
    }
    RedisCluster* defaultCluster = defaultItr->second;


    for (RedisNameList::const_iterator it = redisNameList.begin(); it != redisNameList.end(); ++it)
    {
        // Insert will NOOP if the redisName already exists. For all the remaining(successfully inserted) components, we point them to the default redis cluster
        mRedisClusterByNameMap.insert(eastl::make_pair(*it, defaultCluster));
    }

    // activate each redis cluster
    RedisError err = REDIS_ERR_SYSTEM;
    if (redisClusterConfigMap.empty())
    {
        BLAZE_ERR_LOG(Log::REDIS, "RedisManager.initFromConfig: No redis clusters specified in config.");
        return REDIS_ERR_INVALID_CONFIG;
    }
    else
    {
        RedisClustersMap::const_iterator defaultConfigItr = redisClusterConfigMap.find(defaultClusterName);
        if (defaultConfigItr == redisClusterConfigMap.end())
            return REDIS_ERR_INVALID_CONFIG;
        err = defaultCluster->initFromConfig(config, *defaultConfigItr->second); // always activate default cluster first
        if (err != REDIS_ERR_OK)
        {
            BLAZE_ERR_LOG(Log::REDIS, "RedisManager.initFromConfig: Fail to initialize config for cluster (" << defaultClusterName << ").");
            return err;
        }

        for (RedisClustersMap::const_iterator it = redisClusterConfigMap.begin(); it != redisClusterConfigMap.end(); ++it)
        {
            const char8_t* clusterName = it->first.c_str();
            RedisCluster* cluster = mRedisClusterByNameMap[clusterName];
            err = cluster->initFromConfig(config, *it->second);

            if (err != REDIS_ERR_OK)
            {
                BLAZE_ERR_LOG(Log::REDIS, "RedisManager.initFromConfig: Fail to initialize config for cluster (" << clusterName << ").");
                return err;
            }
        }
    }

    mOwnedLocks.registerCollection();

    RedisLockKeysByInstanceId::ValueList existingLocks;
    mOwnedLocks.getMembers(gController->getInstanceId(), existingLocks);
    for (RedisLockKeysByInstanceId::ValueList::iterator it = existingLocks.begin(), end = existingLocks.end(); it != end; ++it)
    {
        RedisCommand cmd;
        cmd.add("DEL").addKey(*it);

        RedisResponsePtr resp;
        RedisError error = getRedisCluster()->exec(cmd, resp);
        if (error == REDIS_ERR_OK)
        {
            if (resp->isInteger() && (resp->getInteger() == 1))
            {
                BLAZE_WARN_LOG(Log::REDIS, "RedisManager.initFromConfig: "
                    "Found a locked key (" << *it << ") during instance startup, which should not happen.  The key has been forced unlocked.");
            }
        }
        else
        {
            BLAZE_ERR_LOG(Log::REDIS, "RedisManager.initFromConfig: "
                "Failed to the delete LockKey (" << *it << ") from Redis.  The error was '" << gRedisManager->getLastError().getLastErrorDescription() << "'");
        }
    }

    return err;
}

RedisCluster* RedisManager::getRedisCluster(const char8_t* namespaceName)
{
    if (*namespaceName == '\0')
        namespaceName = FRAMEWORK_NAMESPACE;

    RedisClusterByNameMap::iterator clusterItr = mRedisClusterByNameMap.find(namespaceName);
    if (clusterItr != mRedisClusterByNameMap.end())
    {
        return clusterItr->second;
    }
    return nullptr;
}

RedisError RedisManager::exec(const RedisCommand &command, RedisResponsePtr &response)
{
    const char8_t* commandNamespace = command.getNamespace();
    RedisCluster* cluster = getRedisCluster(commandNamespace);
    if (cluster != nullptr)
    {
        return cluster->exec(command, response);
    }
    return REDIS_ERR_SYSTEM;
}
RedisError RedisManager::execAll(const RedisCommand &command, RedisResponsePtrList &response)
{
    const char8_t* commandNamespace = command.getNamespace();
    RedisCluster* cluster = getRedisCluster(commandNamespace);
    if (cluster != nullptr)
    {
        return cluster->execAll(command, response);
    }
    return REDIS_ERR_SYSTEM;
}


RedisUniqueIdManager& RedisManager::getUniqueIdManager() 
{ 
    return getRedisCluster(FRAMEWORK_NAMESPACE)->getUniqueIdManager(); 
}

void RedisManager::deactivate()
{
    if (mActiveFiberGroupId != Fiber::INVALID_FIBER_GROUP_ID)
        Fiber::join(mActiveFiberGroupId);

    mRedisClusterByNameMap.clear();
    while (!mRedisClusterList.empty())
    {
        RedisCluster* cluster = mRedisClusterList.back();
        mRedisClusterList.pop_back();
        cluster->deactivate();
        delete cluster;
    }
}

void RedisManager::getStatusInfo(RedisDatabaseStatus& status) const
{
    RedisDatabaseStatus::ClusterInfosMap& clusterMap = status.getClusterInfos();
    for (RedisClusterList::const_iterator it = mRedisClusterList.begin(), end = mRedisClusterList.end(); it != end; ++it)
    {
        RedisClusterStatus* clusterStatus = clusterMap.allocate_element();
        (*it)->getStatus(*clusterStatus);
        clusterMap[(*it)->getName()] = clusterStatus;
    }
}

void RedisManager::addRedisLockKey(const eastl::string& lockKey)
{
    mOwnedLocks.add(gController->getInstanceId(), lockKey);
}

void RedisManager::removeRedisLockKey(const eastl::string& lockKey)
{
    mOwnedLocks.remove(gController->getInstanceId(), lockKey);
}

}
