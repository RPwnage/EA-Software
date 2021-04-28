/*************************************************************************************************/
/*!
    \file   leaderboardhelper.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_LEADERBOARD_HELPER_H
#define BLAZE_LEADERBOARD_HELPER_H

/*** Include Files *******************************************************************************/

namespace Blaze
{
class StatsProviderPtr;
class StatsProviderRowPtr;

namespace Stats
{
typedef eastl::hash_set<EntityId> EntitySet;

class LeaderboardHelper
{
public:
    LeaderboardHelper(StatsSlaveImpl *component, LeaderboardStatValues& values, bool isStatRawValue = false);
    virtual ~LeaderboardHelper();

    BlazeRpcError fetchLeaderboard(int32_t leaderboardId, const char8_t *leaderboardName, 
        int32_t periodOffset, int32_t t, int32_t periodId, size_t rowCount, ScopeNameValueMap& scopeNameValueMap, bool includeStatlessEntities = true, bool enforceCutoff = false);

protected:
    void appendScopesToQuery(QueryPtr& dbQuery) const;
    void appendCutoffConditionToQuery(QueryPtr& dbQuery) const;
    void appendColumnsToQuery(QueryPtr& dbQuery);

    typedef eastl::vector<const DbRow*> DbRowVector;

    virtual BlazeRpcError prepareEntitiesForQuery(EntityIdList& entityList) = 0;
    virtual bool isEmptyQuery(EntityIdList& entityList) = 0;
    virtual void previewDbLookupResults(uint32_t configuredSize, const DbRowVector& dbRowVector, EntityIdList& entityList) = 0;
    virtual BlazeRpcError getIndexEntries() = 0;

protected:
    StatsSlaveImpl* mComponent;
    const StatLeaderboard *mLeaderboard;
    int32_t mPeriodId;
    LeaderboardStatValues& mValues;
    EntityIdList mKeys;
    ScopeNameValueListMap mCombinedScopeNameValueListMap; // resulting keyscope after combining user-specified
                                                          // (request) keyscope with leaderboard config keyscope
    bool mSorted; // whether the response is sorted (by rank stat value) after primary fetch

private:
    typedef eastl::map<EntityId, StatsProviderRowPtr> EntityStatRowMap;
    typedef eastl::vector<DbResultPtr> DbResultPtrVector;

    struct Result
    {
        StatsProviderPtr statsProvider; // owned
        EntityStatRowMap entityMap;
    };

    typedef eastl::hash_map<const LeaderboardGroup::Select*, Result> ResultMap;

    BlazeRpcError _fetchLeaderboard(int32_t leaderboardId, const char8_t *leaderboardName,
        int32_t periodOffset, int32_t t, int32_t periodId, size_t rowCount, ScopeNameValueMap& scopeNameValueMap, bool includeStatlessEntities, bool enforceCutoff);

    BlazeRpcError doDbLookupQuery(uint32_t dbId, const EntityIdList& entityIds, bool enforceCutoff, DbResultPtrVector& dbResultPtrVector);
    BlazeRpcError lookupDbRows(bool enforceCutoff);
    void fillOutIdentities();
    const char8_t* providerStatToString(char8_t* buf, int32_t bufLen, const Stat* stat, const StatsProviderRowPtr row) const;
    StatRawValue& providerStatToRaw(StatRawValue& statRawVal, const Stat* stat, const StatsProviderRowPtr row)const;

    BlazeRpcError buildPrimaryResults(EntityIdList& entityList, bool includeStatlessEntities, bool enforceCutoff);
    BlazeRpcError buildSecondaryResults(const EntityIdList& entityList);
    void buildResponse(LeaderboardStatValues& response);
    const ScopeNameValueListMap& getGroupScopeMap() const { return mCombinedScopeNameValueListMap; }

    ResultMap mResultMap;
    bool mIsStatRawValue;
    int32_t mPeriodIds[STAT_NUM_PERIODS];
};

} // Stats
} // Blaze

#endif

