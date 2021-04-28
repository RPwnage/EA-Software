/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REDIS_UNIQUEIDMANAGER_H
#define BLAZE_REDIS_UNIQUEIDMANAGER_H

/*** Include files *******************************************************************************/

#include "EASTL/vector_map.h"
#include "framework/tdf/frameworkredistypes_server.h"
#include "framework/redis/rediskeymap.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


namespace Blaze
{

class RedisCluster;

/**
 *  /class Redis
 *  
 *  This class implements a unique id backed by Redis.
 *  
 *  /examples See testRedisUniqueId() @ redisunittests.cpp
*/
class RedisUniqueIdManager
{
public:
    static const uint32_t ID_RESERVATION_RANGE = 1000;

    RedisUniqueIdManager();
    ~RedisUniqueIdManager();
    
    bool registerUniqueId(const RedisId& id);
    bool deregisterUniqueId(const RedisId& id);
    RedisUniqueId getNextUniqueId(const RedisId& id);

private:
    bool registerManager();
    bool deregisterManager();
    friend class RedisCluster;
    // intentionally compare names first because they are more likely to differ sooner across different entries
    typedef eastl::vector_map<RedisId, RedisUniqueId, RedisIdLessThan> UniqueIdByRedisIdMap;
    typedef RedisKeyMap<RedisId, RedisUniqueId> UniqueIdKeyMap;
    UniqueIdKeyMap mUniqueIdKeyMap;
    UniqueIdByRedisIdMap mUniqueIdByRedisIdMap; // stores active redis ids that have reserved counts
};

} // Blaze

#endif // BLAZE_REDIS_UNIQUEIDMANAGER_H

