/*************************************************************************************************/
/*!
    \file   getstatsbygroupbase.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetStatsByGroupBase

    A base class for getStatsByGroup and getStatsByGroupAsync command classes.
    Handles stats provider access for both commands.
*/
/*************************************************************************************************/
#ifndef BLAZE_STATS_GET_STATS_BY_GROUP_BASE
#define BLAZE_STATS_GET_STATS_BY_GROUP_BASE
#include "EASTL/vector.h"
#include "EASTL/vector_set.h"

namespace Blaze
{
class StatsProviderRowPtr;

namespace Stats
{
class Stat;
class StatsSlaveImpl;
class StatCategory;
class StatDesc;

class GetStatsByGroupBase
{
public:
    GetStatsByGroupBase(StatsSlaveImpl* component);
    ~GetStatsByGroupBase();
    
    BlazeRpcError executeBase(const GetStatsByGroupRequest& request, GetStatsResponse& response);
    StatsSlaveImpl* mComponent;

private:

    struct ScopeBlock
    {
        ScopeBlock* next; // always owned
    };
    
    struct StatRow
    {
        EntityId entityId;
        ScopeValue* scopes; // never owned
        EntityStatsPtr stats; // owned only on failure
    };
    
    struct EidCompare
    {
        bool operator() (const StatRow& a, const StatRow& b);
    };
    
    typedef eastl::vector<int32_t> ScopeIndexList;
    
    struct EidScopeCompare
    {
        // scopeIndices points to a -1 terminated array
        EidScopeCompare(const ScopeIndexList* scopeIndices)
        : mScopeIndices(scopeIndices) {}
        bool operator() (const StatRow& a, const StatRow& b);
        const ScopeIndexList* mScopeIndices; // never owned
    };
    
    typedef eastl::vector<StatRow> RowList;
    typedef eastl::vector_set<EntityId> EntityIdSet;
    typedef eastl::vector<ScopeValue> ScopeList;
    typedef eastl::vector<const char8_t*> StatValueList;
    
    BlazeRpcError validateScope(const GetStatsByGroupRequest& request) const;
    BlazeRpcError buildPrimaryResults(const GetStatsByGroupRequest& request, EntityIdSet& entityIdSet, bool& doOuterJoin);
    BlazeRpcError buildSecondaryResults(const GetStatsByGroupRequest& request, const EntityIdSet& entityIdSet, bool doOuterJoin);
    void buildResponse(GetStatsResponse& response);
    void buildDefaultStatList();
    ScopeValue* allocateScopeBlock(size_t scopeCount);
    void formatStat(char8_t* buf, size_t len, const Stat& stat, const StatsProviderRowPtr& row);
    int32_t getOffsetPeriodId(int32_t periodType, const GetStatsByGroupRequest& request) const;
    const ScopeNameValueMap& getGroupScopeMap() const;

    const StatGroup* mGroup; // not owned
    ScopeNameValueMap mEmptyScopeMap; // never written, read only
    StatValueList mDefaultStatList; // written once, read only stat values used by multiple stat rows
    ScopeBlock* mScopeBlockHead; // freed in dtor
    RowList mRowList; // each StatRow structure corresponds to a single stat provider row

    char8_t mAsStr[STATS_STATVALUE_LENGTH];
    char8_t mTempKey[STATS_SCOPESTRING_LENGTH];
    int32_t mPeriodIds[STAT_NUM_PERIODS];
};
} // Stats
} // Blaze

#endif
