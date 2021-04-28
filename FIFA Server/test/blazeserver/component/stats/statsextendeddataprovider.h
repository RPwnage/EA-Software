/*************************************************************************************************/
/*!
    \file statsextendeddataprovider.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_STATS_EXTENDED_DATA_PROVIDER_H
#define BLAZE_STATS_EXTENDED_DATA_PROVIDER_H

/*** Include files *******************************************************************************/
#include "EASTL/vector.h"
#include "EASTL/vector_map.h"
#include "EASTL/hash_map.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/tdf/userextendeddatatypes.h"
#include "stats/statscommontypes.h"
#include "stats/tdf/stats_server.h"
#include "statscommontypes.h"
#include "userstats.h"
#include "userranks.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace Stats
{

class StatsSlaveImpl;

class StatsExtendedDataProvider : public ExtendedDataProvider
{
private:
    NON_COPYABLE(StatsExtendedDataProvider);

public:    
    StatsExtendedDataProvider(StatsSlaveImpl& component);

    bool parseExtendedData(const StatsConfig& config, StatsConfigData* statsConfig);

    const UserRanks& getUserRanks() const { return mUserRanks; }
    const UserStats& getUserStats() const { return mUserStats; }

private:

    /* \brief The custom data for a user session is being loaded. (global event) */
    BlazeRpcError onLoadExtendedData(const UserInfoData& data, UserSessionExtendedData &extendedData) override;
    /* \brief looks up the extended data associated with the given string key */
    virtual bool lookupExtendedData(const UserSessionMaster& session, const char8_t* key, UserExtendedDataValue& value) const;
    /* \brief pulls up ranking extended data and adds it to the provided extended data object */
    BlazeRpcError onRefreshExtendedData(const UserInfoData& data, UserSessionExtendedData &extendedData) override;

    // private data
    StatsSlaveImpl& mComponent;
    UserRanks mUserRanks;
    UserStats mUserStats;
    ExtendedDataKeyMap mExtendedDataKeyMap;
};   

}
}
#endif


