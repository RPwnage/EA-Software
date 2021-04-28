/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REDIS_MANAGER_H
#define BLAZE_REDIS_MANAGER_H

/*** Include files *******************************************************************************/

#include "blazerpcerrors.h"
#include "framework/tdf/frameworkredistypes_server.h"
#include "framework/system/fiber.h"
#include "framework/system/frameworkresource.h"
#include "framework/redis/rediscluster.h"
#include "framework/redis/rediskeyset.h"
#include "EASTL/intrusive_list.h"
#include "EASTL/list.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{


typedef eastl::vector<const char8_t*> ComponentList;
class RedisCluster;
class RedisDatabaseStatus;

class RedisManager
{
    NON_COPYABLE(RedisManager);
public:
    static const char8_t* FRAMEWORK_NAMESPACE;
    static const char8_t* UNITTESTS_NAMESPACE;

public:
    RedisManager();
    virtual ~RedisManager() {}

    /*! ************************************************************************************************/
    /*!
        \brief Configures this RedisManager.  No connections are made until activate() is called.

        \param[in] config - A RedisClusterConfig which provides a list of Redis servers, and their associated passwords.
        \return true if the configuration was valid.
    ****************************************************************************************************/
    RedisError initFromConfig(const RedisConfig& config);


    /*! ************************************************************************************************/
    /*!
        \brief Disconnects from all connected Redis clusters.
    ****************************************************************************************************/
    void deactivate();

    void getStatusInfo(RedisDatabaseStatus& status) const;

    RedisCluster* getRedisCluster(const char8_t* namespaceName = "");

    const RedisErrorHelper &getLastError() const { return mLastError; }
    StringBuilder &setLastError(RedisError lastError) { return mLastError.setLastError(lastError); }

    /*! ************************************************************************************************/
    /*!
        \brief Sends the command to a Redis server, and blocks until a response is received.
               The Redis server in the cluster that the command is sent to is determined
               by the hash code obtained from the command parameter.

        \param[in] command - The command to send to the Redis server.  The has code used to select the
                             Redis server is obtained via the getHashCope() method of the command.
        \param[in] response - If no error occurs, the response contains a populated RedisResponse
                              object for examining the contents of the response from the Redis server.

        \return ERR_OK if a response was received, otherwise an error.  The response itself may contain
                an error from the Redis server.  This will still return ERR_OK.
    ****************************************************************************************************/
    RedisError exec(const RedisCommand &command, RedisResponsePtr &response);
    RedisError execAll(const RedisCommand &command, RedisResponsePtrList &response);
    
    RedisUniqueIdManager& getUniqueIdManager();
    const RedisConfig& getRedisConfig() const { return mRedisConfig; }

private:
    friend class RedisLock;

    Fiber::FiberGroupId getActiveFiberGroupId() const { return mActiveFiberGroupId; }

    void addRedisLockKey(const eastl::string& lockKey);
    void removeRedisLockKey(const eastl::string& lockKey);

private:
    typedef eastl::vector<RedisCluster*> RedisClusterList;
    typedef eastl::hash_map<eastl::string, RedisCluster*, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo, EASTLAllocatorType, true> RedisClusterByNameMap;

    RedisClusterList mRedisClusterList; // map component name to redis clusters' name
    RedisClusterByNameMap mRedisClusterByNameMap; // map for all the clusters, key by cluster name

    typedef RedisKeySet<InstanceId, eastl::string> RedisLockKeysByInstanceId;
    RedisLockKeysByInstanceId mOwnedLocks;

    RedisConfig mRedisConfig;
    RedisErrorHelper mLastError;

    Fiber::FiberGroupId mActiveFiberGroupId;
};

extern EA_THREAD_LOCAL RedisManager* gRedisManager;


} // Blaze

#endif // BLAZE_REDIS_MANAGER_H
