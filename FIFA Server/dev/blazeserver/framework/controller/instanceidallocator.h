/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_INSTANCEIDALLOCATOR_H
#define BLAZE_INSTANCEIDALLOCATOR_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


namespace Blaze
{

struct RedisScript;

/**
 *  /class InstanceIdAllocator
 *
 *  This class exchanges Blaze instance names for InstanceIds.
 *
*/
class InstanceIdAllocator
{
public:

    static InstanceId getOrRefreshInstanceId(const char8_t* instanceName, InstanceId instanceId, TimeValue ttl);
    static InstanceId checkAndCleanupInstanceNameOnLedger(const eastl::string& instanceName);

    static void getInstanceNamesLedgerVersionInfo(eastl::string& versionInfo);
    static void getInstanceNamesLedger(eastl::vector<eastl::string>& instanceNames);

private:
    static const char8_t* msInstanceIdAllocatorHashTag;
    static const char8_t* msInstanceNameLedgerKey;
    static const char8_t* msInstanceNameLedgerVersionInfoKey;

    static RedisScript msGetOrRefreshInstanceIdScript;
    static RedisScript msCheckAndCleanupInstanceNameOnLedgerScript;
};

} // Blaze

#endif // BLAZE_INSTANCEIDALLOCATOR_H

