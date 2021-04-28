/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REDIS_INSTANCEIDALLOCATOR_H
#define BLAZE_REDIS_INSTANCEIDALLOCATOR_H

/*** Include files *******************************************************************************/

#include "EASTL/vector_map.h"
#include "framework/tdf/frameworkredistypes_server.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


namespace Blaze
{

class RedisCluster;

/**
 *  /class Redis
 *
 *  This class exchanges Blaze instance names for InstanceIds.
 *
*/
class RedisInstanceIdAllocator
{
public:

    static InstanceId getOrRefreshInstanceId(const char8_t* instanceName, InstanceId instanceId, TimeValue ttl);
    static InstanceId checkAndCleanupInstanceNameOnLedger(const eastl::string& instanceName);
    static void getInstanceNamesLedger(eastl::vector<eastl::string>& instanceNames);

private:
    static const char8_t* msInstanceIdAllocatorHashTag;
    static const char8_t* msInstanceNameLedgerKey;
    static RedisScript msGetOrRefreshInstanceIdScript;
    static RedisScript msCheckAndCleanupInstanceNameOnLedgerScript;
};

} // Blaze

#endif // BLAZE_REDIS_INSTANCEIDALLOCATOR_H

