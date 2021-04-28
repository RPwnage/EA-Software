/*************************************************************************************************/
/*!
    \file userstats.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_STATS_USERSTATS_H
#define BLAZE_STATS_USERSTATS_H

/*** Include files *******************************************************************************/
#include "EASTL/vector_map.h"
#include "EASTL/hash_map.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/tdf/userextendeddatatypes.h"
#include "stats/statscommontypes.h"
#include "stats/tdf/stats_server.h"
#include "stats/statsextendeddatamap.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class UpdateExtendedDataRequest;
    
namespace Stats
{
class UserStat;
class UserCategory;
class StatsSlaveImpl;

    typedef eastl::hash_map<const char8_t*, UserStat*, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > UserStatMap;
    typedef eastl::vector_map<EA::TDF::tdf_ptr<const ScopeNameValueMap>, UserStatMap*, scope_index_map_less > ScopeStatsMap;
    typedef eastl::hash_map<int32_t, ScopeStatsMap*> UserPeriodMap;
    typedef eastl::hash_map<uint32_t, UserCategory*> UserCategoryMap;

class UserStat
{
public:
    UserStat(UserStatId id,  const char8_t* name, const Stat* stat)
    : mId(id),
      mName(name),
      mStat(stat)
    {}

    ~UserStat() 
    {
        if (mName != nullptr) BLAZE_FREE((char8_t*)mName);
    }

    UserStatId getId() const {return mId;}
    const Stat* getStat() const {return mStat;}
    const char8_t* getName() const {return mName;}
    const char8_t* getStatName() const {return mStat->getName();}
private:
    UserStatId mId;
    const char8_t* mName;
    const Stat* mStat;
};

class UserCategory
{
public:
    UserCategory(const StatCategory* category){mStatCategory = category;}
    ~UserCategory();
    
    const StatCategory* getStatCategory() const {return mStatCategory;}
    UserPeriodMap* getUserPeriodMap() {return &mUserPeriodMap;}
    const UserPeriodMap* getUserPeriodMap() const {return &mUserPeriodMap;}
    const char8_t* getDbTable(int32_t period) const { return mStatCategory->getTableName(period);}

private:
    
    const StatCategory* mStatCategory;
    UserPeriodMap mUserPeriodMap;
};

class UserStats
{
private:
    NON_COPYABLE(UserStats);

public:    
    UserStats(StatsSlaveImpl& component) 
    : mComponent(component)
    {}

    ~UserStats();

    void updateUserStats(UpdateExtendedDataRequest& request, EntityId entityId, uint32_t categoryId, int32_t periodType, const ScopeNameValueMap& scopeMap, const StatValMap& statValMap) const;
    void deleteUserStats(UpdateExtendedDataRequest& request, EntityId entityId) const;
    void deleteUserStats(UpdateExtendedDataRequest& request, EntityId entityId, uint32_t categoryId, int32_t periodType) const;
    void deleteUserStats(UpdateExtendedDataRequest& request, EntityId entityId, uint32_t categoryId, int32_t periodType, const ScopeNameValueMap& scopeMap) const;
    bool parseUserStats(const StatsConfig& config, ExtendedDataKeyMap &keyMap,
        StatsConfigData* statsConfig, UserExtendedDataIdMap& uedIdMap, ConfigureValidationErrors& validationErrors);

private:
    friend class StatsExtendedDataProvider;

    void deleteUserStats(UpdateExtendedDataRequest& request, EntityId entityId, const UserStatMap& statMap) const;

    bool addStat(UserStatId id, const char8_t* name, const StatCategory* cat, const Stat* stat, 
        int32_t periodType, ScopeNameValueMap* scopeNameValueMap = nullptr);

    /* \brief The custom data for a user session is being loaded. (global event) */
    BlazeRpcError onLoadExtendedData(const UserInfoData& data, UserSessionExtendedData &extendedData);
    /* \brief looks up the extended data associated with the given string key */
    bool lookupExtendedData(const UserSessionMaster& session, const char8_t* key, UserExtendedDataValue& value) const;
    /* \brief pulls up ranking extended data and adds it to the provided extended data object */
    BlazeRpcError onRefreshExtendedData(const UserInfoData& data, UserSessionExtendedData &extendedData) { return ERR_OK; }

    /* \brief Does the actual fiber work of calling the stat */
    void doUpdateUserStats(UpdateExtendedDataRequestPtr req);

    // private data
    UserCategoryMap mUserCategoryMap;
    UserStatMap mUserStatMap;
    StatsSlaveImpl& mComponent;
};

}
}
#endif


