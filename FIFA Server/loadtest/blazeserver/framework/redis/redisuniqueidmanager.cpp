/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/redis/redisuniqueidmanager.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


namespace Blaze
{


RedisUniqueIdManager::RedisUniqueIdManager()
: mUniqueIdKeyMap(RedisId("framework", "UniqueIdManager"), false) // NOTE: Never sharded!
{
}

RedisUniqueIdManager::~RedisUniqueIdManager()
{
}

bool RedisUniqueIdManager::registerUniqueId(const RedisId& id)
{
    if (!mUniqueIdKeyMap.isRegistered())
        return false; // REDIS_TODO: log error

    eastl::pair<UniqueIdByRedisIdMap::iterator, bool> ret = mUniqueIdByRedisIdMap.insert(id);
    if (ret.second)
        ret.first->second = INVALID_REDIS_UNIQUE_ID;

    return true;
}

bool RedisUniqueIdManager::deregisterUniqueId(const RedisId& id)
{
    if (!mUniqueIdKeyMap.isRegistered())
        return false; // REDIS_TODO: log error

    return mUniqueIdByRedisIdMap.erase(id) > 0;
}

RedisUniqueId RedisUniqueIdManager::getNextUniqueId(const RedisId& id)
{
    if (!mUniqueIdKeyMap.isRegistered())
        return INVALID_REDIS_UNIQUE_ID;
    
    RedisUniqueId uniqueId = INVALID_REDIS_UNIQUE_ID;
    
    UniqueIdByRedisIdMap::iterator it = mUniqueIdByRedisIdMap.find(id);
    if (it != mUniqueIdByRedisIdMap.end())
    {
        if ((it->second % ID_RESERVATION_RANGE) == 0)
        {
            int64_t result = 0;
            RedisError rc = mUniqueIdKeyMap.incrBy(id, ID_RESERVATION_RANGE, &result);
            if (rc == REDIS_ERR_OK && result >= static_cast<int64_t>(ID_RESERVATION_RANGE))
                it->second = static_cast<RedisUniqueId>(result) - ID_RESERVATION_RANGE;
            else
            {
                ; // REDIS_TODO: log error here
                return INVALID_REDIS_UNIQUE_ID;
            }
        }
        uniqueId = ++it->second;
    }

    return uniqueId;
}

bool RedisUniqueIdManager::registerManager()
{
    return mUniqueIdKeyMap.registerCollection();
}

bool RedisUniqueIdManager::deregisterManager()
{
    return mUniqueIdKeyMap.deregisterCollection();
}

} // Blaze

