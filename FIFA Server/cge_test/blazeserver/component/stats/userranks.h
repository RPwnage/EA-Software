/*************************************************************************************************/
/*!
    \file userranks.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_STATS_USERRANKS_H
#define BLAZE_STATS_USERRANKS_H

/*** Include files *******************************************************************************/
#include "EASTL/vector.h"
#include "EASTL/vector_map.h"
#include "EASTL/hash_map.h"

#include "framework/tdf/userextendeddatatypes.h"
#include "stats/statscommontypes.h"
#include "stats/tdf/stats_server.h"
#include "stats/statsextendeddatamap.h"


/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class UserInfoData;
class UserSessionExtendedData;
class UserSessionMaster;

namespace Stats
{
class UserRank;
class StatsSlaveImpl;

typedef eastl::hash_map<const char8_t*, UserRank*, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > UserRankMap;

class UserRank
{
public:
    UserRank(UserStatId id, const char8_t* rankName, const StatLeaderboard* leaderboard, const EA::TDF::tdf_ptr<ScopeNameValueListMap>& scopeNameValueListMap)
    : mId(id),
    mRankName(blaze_strdup(rankName)),
    mLeaderboard(leaderboard),
    mScopeNameValueListMap(scopeNameValueListMap)
    {}

    ~UserRank()
    {
        BLAZE_FREE((char8_t*)mRankName);
    }

    UserStatId getId() const {return mId;}
    const StatLeaderboard* getLeaderboard() const {return mLeaderboard;}
    const char8_t* getName() const { return mRankName;}
    const ScopeNameValueListMap* getScopeNameValueListMap() const { return mScopeNameValueListMap; }

private:

    UserStatId mId;
    const char8_t* mRankName;
    const StatLeaderboard* mLeaderboard; 
    EA::TDF::tdf_ptr<ScopeNameValueListMap> mScopeNameValueListMap;
};

class UserRanks
{
private:
    NON_COPYABLE(UserRanks);

public:   
    UserRanks(StatsSlaveImpl& component) 
    : mComponent(component)
    {}

    ~UserRanks();

    BlazeRpcError updateUserRanks(const UserInfoData& data, UserSessionExtendedData &extendedData) const;
    void updateUserRanks(UpdateExtendedDataRequest& request, const CacheRowUpdateList& statList) const;
    void updateUserRanks(UpdateExtendedDataRequest& request, const CacheRowDeleteList& statList) const;
    void updateUserRanks(UpdateExtendedDataRequest& request, EntityId entityId, uint32_t categoryId, int32_t periodType) const;

    bool parseUserRanks(const StatsConfig& config, ExtendedDataKeyMap &keyMap, StatsConfigData* statsConfig,
        UserExtendedDataIdMap& uedIdMap, ConfigureValidationErrors& validationErrors);
    uint16_t getUserRankId(const char8_t* name) const;
    int32_t getUserRank(BlazeId blazeId, const char8_t* name) const;

private:

    friend class StatsExtendedDataProvider;


    bool addRank(UserStatId id, const char8_t* name, const StatLeaderboard* leaderboard, EA::TDF::tdf_ptr<ScopeNameValueListMap>& scopeNameValueListMap);
    bool scopeNameValueMapsEqual(const ScopeNameValueListMap* map1, const ScopeNameValueMap* map2) const;
    void doUpdateUserRanks(UpdateExtendedDataRequestPtr req);

    /* \brief The custom data for a user session is being loaded. (global event) */
    BlazeRpcError onLoadExtendedData(const UserInfoData& data, UserSessionExtendedData &extendedData);
    /* \brief looks up the extended data associated with the given string key */
    bool lookupExtendedData(const UserSessionMaster& session, const char8_t* key, UserExtendedDataValue& value) const;
    /* \brief pulls up ranking extended data and adds it to the provided extended data object */
    BlazeRpcError onRefreshExtendedData(const UserInfoData& data, UserSessionExtendedData &extendedData);

    // private data
    UserRankMap mUserRankMap;
    StatsSlaveImpl& mComponent;
};

}
}
#endif


