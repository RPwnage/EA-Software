/*************************************************************************************************/
/*!
    \file   statsslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_STATS_STATSSLAVEIMPL_H
#define BLAZE_STATS_STATSSLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "framework/usersessions/usersessionsubscriber.h"
#include "framework/metrics/metrics.h"
//#include "framework/controller/controller.h"
#include "framework/controller/drainlistener.h"
#include "framework/controller/remoteinstancelistener.h"
#include "framework/tdf/userextendeddatatypes_server.h"

#include "stats/statscomponentinterface.h"
#include "stats/rpc/statsslave_stub.h"
#include "stats/rpc/statsmaster.h"
#include "stats/tdf/stats_server.h"
#include "stats/transformstats.h"
#include "stats/dbstatsprovider.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
// Forward declarations

namespace Stats
{

#define MEM_GROUP_STATS_CACHE       MEM_GROUP_GET_SUB_ALLOC_GROUP(StatsSlave::COMPONENT_MEMORY_GROUP, 1)
#define MEM_GROUP_STATS_LEADERBOARD MEM_GROUP_GET_SUB_ALLOC_GROUP(StatsSlave::COMPONENT_MEMORY_GROUP, 2)

#define BLAZE_NEW_STATS_CACHE BLAZE_NEW_MGID(MEM_GROUP_STATS_CACHE, "stats_cache")
#define BLAZE_NEW_ARRAY_STATS_CACHE(_objtype, _count)  BLAZE_NEW_ARRAY_MGID(_objtype, _count, MEM_GROUP_STATS_CACHE, "stats_cache")

#define BLAZE_NEW_STATS_LEADERBOARD BLAZE_NEW_MGID(MEM_GROUP_STATS_LEADERBOARD, "leaderboard")
#define BLAZE_NEW_ARRAY_STATS_LEADERBOARD(_objtype, _count)  BLAZE_NEW_ARRAY_MGID(_objtype, _count, MEM_GROUP_STATS_LEADERBOARD, "leaderboard")

// Forward declarations
class StatsConfigData;
class StatCategory;
class LeaderboardIndexes;
class GlobalStatsCache;
class StatsCache;
class StatsSlaveImpl;
class StatsExtendedDataProvider;

struct DebugTimer
{
    bool useTimer;
    int64_t dailyTimer;
    int64_t weeklyTimer;
    int64_t monthlyTimer;
};

// Contains various pieces of state needed to resolve derived stat values
typedef struct ResolveContainer
{
    const char8_t* categoryName;
    const CategoryUpdateRowKey* catRowKey;
    const UpdateRowMap* updateRowMap;
} ResolveContainer;

// Contains various pieces of state needed to resolve derived stat values for aggregate scope rows
typedef struct ResolveAggregateContainer
{
    const char8_t* categoryName;
    const CategoryUpdateAggregateRowKey* catRowKey;
    const UpdateAggregateRowMap* updateRowMap;
} ResolveAggregateContainer;

// The MultiCategoryDerivedMap is built-up at the beginning of processing an update stats request,
// it is used to group up the category, entity, and scope tuples involved in the request so that
// any additional rows needed for multi-category derived stats can be determined
typedef eastl::set<const ScopeNameValueMap*, scope_index_map_less> ScopeNameValueMapSet;
typedef eastl::hash_map<EntityId, ScopeNameValueMapSet> EntityToScopeMap;
typedef eastl::hash_map<const char8_t*, EntityToScopeMap, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > MultiCategoryDerivedMap;

enum UpdateState
{
    UNINITIALIZED     = 0x00000000,
    INITIALIZED       = 0x00000001,
    FETCHED           = 0x00000002,
    APPLIED_UPDATES   = 0x00000004,
    APPLIED_DERIVED   = 0x00000008,
    COMMITED          = 0x00000010,
    ABORTED           = 0x00000020
};

struct StatValMapWrapper : public StatValMap 
{
    ~StatValMapWrapper()
    {
        for (StatValMap::iterator i = begin(), e = end(); i != e; ++i)
        {
            delete i->second;
        }
    }
};

static const uint64_t INVALID_TRANSACTION_ID = 0xFFFFFFFFFFFFFFFF;
static const uint32_t TRANSACTION_TIMEOUT_DEFAULT = 30; //seconds

void resolveStatValue(const char8_t* nameSpace, const char8_t* name, Blaze::ExpressionValueType type,
                      const void* context, Blaze::Expression::ExpressionVariableVal& val);
void resolveAggregateStatValue(const char8_t* nameSpace, const char8_t* name, Blaze::ExpressionValueType type,
    const void* context, Blaze::Expression::ExpressionVariableVal& val);

class StatsTransactionContext : public TransactionContext
{
    NON_COPYABLE(StatsTransactionContext);

public:
    typedef EA::TDF::TdfStructVector<StatRowUpdate > StatRowUpdateList;
    StatRowUpdateList& getIncomingRequest() { return mIncomingRequest; }
    UpdateHelper& getUpdateHelper() { return mUpdateHelper; }
    uint32_t getUpdateState() const { return mUpdateState; }
    void setUpdateState(uint32_t state) { mUpdateState = state; }
    void setProcessGlobalStats(bool processGlocalStats) { mProcessGlobalStats = processGlocalStats; }
    const UpdateStatsRequest& getGlobalStats() const { return mGlobalStats; }
    void setTimeout(uint64_t timeout) { mTimeout = timeout; } 

    BlazeRpcError preprocessMultiCategoryDerivedStats();
    BlazeRpcError validateRequest(bool strict);
    void initializeStatsProvider();
    void reset();
    BlazeRpcError fetchStats();
    void getRowsToUpdateForOnePeriod(const ScopeNameValueMap& updateStatIndexRow, 
        ScopeNameValueSetMap& nameValueSetMap, ScopesVectorForRows& scopeVectorForRows) const;
    void getRowScopeSettings(const ScopeNameValueSetMap& nameValueSetMap, 
        ScopesVectorForRows& scopeVectorForRows) const;
    bool getNextCombineIndex(uint32_t tracking[], uint32_t radix[], uint32_t numOfDataset) const;
    BlazeRpcError calcStatsChange(bool coreStatsChange, bool derivedStatsChange);
    void updateCoreStatsInRow(UpdateRow& row) const;
    void updateDerivedStatsInRows();
    void evaluateStats(const StatUpdate* update, const Stat* stat, StatValMap& statValMap, StatPtrValMap& modifiedValMap) const;
    void updateProviderRow(UpdateRow& row, const StatsProviderRow* provRow, EntityId entityId, int32_t period,
        int32_t periodId,
        StatValMap* diffStatValMap = nullptr, const RowScopesVector* rowScopes = nullptr) ;
    void addBroadcastData(const StatCategory* cat,EntityId entityId, 
        int32_t period, int32_t periodId, const StatValMap* statValMap, const RowScopesVector* rowScopes);
    void updateAggregateDerivedStats();

    BlazeRpcError commitUpdates();
    BlazeRpcError DEFINE_ASYNC_RET(commitTransaction());
    void abortTransaction();
    void clear();
    void updateProviderRows(const FetchedRowUpdateList& cacheList);
    
private:
    friend class StatsSlaveImpl;   

    StatsTransactionContext(const char* description, StatsSlaveImpl &component);
    ~StatsTransactionContext() override;

    // following have to be implemented and are invoked only by Component class   
    BlazeRpcError commit() override;   
    BlazeRpcError rollback() override; 

    StatsSlaveImpl* mComp;
    StringBuilder mLbQueryString;

    StatRowUpdateList mIncomingRequest;
    UpdateHelper mUpdateHelper;
    uint32_t mUpdateState;
    StatsProvider* mStatsProvider;
    bool mProcessGlobalStats;
    UpdateStatsRequest mGlobalStats;
    StatUpdateNotification mNotification;
    UpdateExtendedDataRequest mExtDataReq;
    uint64_t mTimeout;
    StringStatValueList mDerivedStrings;
    // Keep track of all ScopesVectorForRows objects allocated in the block below so that we can clean them up at the end
    typedef eastl::vector<ScopesVectorForRows> ScopesVectorForRowsVector;
    ScopesVectorForRowsVector mScopesVectorForRowsVector;
    int32_t mPeriodIds[STAT_NUM_PERIODS]; 
};


class StatsSlaveImpl : public StatsSlaveStub, public StatsComponentInterface,
    private UserSessionSubscriber,
    private LocalUserSessionSubscriber,
    private StatsMasterListener,
    private RemoteServerInstanceListener,
    private DrainStatusCheckHandler
{
public:
    StatsSlaveImpl();
    ~StatsSlaveImpl() override;

    const StatsConfigData* getConfigData() const override { return mStatsConfig; }
    StatsConfigData* getConfigData() { return mStatsConfig; }
    uint32_t getDbId() const override { return mDbId; }

    bool isKeyScopeChangeable(const char8_t* keyscope) const;

    StatsCache& getStatsCache() const { return *mStatsCache; }
    GlobalStatsCache *getGlobalStatsCache() const { return mGlobalStatsCache; }
    int32_t getPeriodId(int32_t periodType) override;
    void getPeriodIds(int32_t periodIds[STAT_NUM_PERIODS]);
    int32_t getRetention(int32_t periodType) override;
    PeriodIds* getPeriodData() {return &mPeriodIds;}
    void printMemBudget() const;
    uint16_t getUserRankId(const char8_t* name) const;
    int32_t getUserRank(BlazeId blazeId, const char8_t* name) const;
    int32_t getPeriodIdForTime(int32_t t, StatPeriodType periodType) const;

    //Stat modification code
    BlazeRpcError deleteStats(const DeleteStatsRequest &request); //defined in deletestats.cpp
    BlazeRpcError deleteStatsByUserSet(const DeleteStatsByUserSetRequest &request); //defined in deletestats.cpp
    BlazeRpcError deleteStatsByCategory(const DeleteStatsByCategoryRequest &request); //defined in deletestats.cpp
    BlazeRpcError deleteStatsByKeyScope(const DeleteStatsByKeyScopeRequest &request); //defined in deletestats.cpp
    BlazeRpcError deleteAllStatsByEntity(const DeleteAllStatsByEntityRequest &request); //defined in deletestats.cpp
    BlazeRpcError deleteAllStatsByKeyScope(const DeleteAllStatsByKeyScopeRequest &request); //defined in deletestats.cpp
    BlazeRpcError deleteAllStatsByKeyScopeAndEntity(const DeleteAllStatsByKeyScopeAndEntityRequest &request); //defined in deletestats.cpp
    BlazeRpcError deleteAllStatsByKeyScopeAndUserSet(const DeleteAllStatsByKeyScopeAndUserSetRequest &request); //defined in deletestats.cpp
    BlazeRpcError deleteAllStatsByCategoryAndEntity(const DeleteAllStatsByCategoryAndEntityRequest &request); //defined in deletestats.cpp

    BlazeRpcError getCenteredLeaderboardEntries(const StatLeaderboard& leaderboard,
        const ScopeNameValueListMap* scopeNameValueListMap, int32_t periodId, EntityId centerEntityId, int32_t count,
        const EA::TDF::ObjectId& userSetId, bool showAtBottomIfNotFound,
        EntityIdList& keys, LeaderboardStatValues& response, bool& sorted) const;
    BlazeRpcError getLeaderboardEntries(const StatLeaderboard& leaderboard,
        const ScopeNameValueListMap* scopeNameValueListMap, int32_t periodId, int32_t startRank, int32_t count,
        const EA::TDF::ObjectId& userSetId, EntityIdList& keys, LeaderboardStatValues& response, bool& sorted) const;
    BlazeRpcError getFilteredLeaderboardEntries(const StatLeaderboard& leaderboard,
        const ScopeNameValueListMap* scopeNameValueListMap, int32_t periodId, const EntityIdList& idList, uint32_t limit,
        EntityIdList& keys, LeaderboardStatValues& response, bool& sorted) const;

    BlazeRpcError getEntityCount(const StatLeaderboard* leaderboard, const ScopeNameValueListMap* scopeNameValueListMap,
        int32_t periodId, EntityCount* response) const;

    BlazeRpcError getLeaderboardRanking(const StatLeaderboard* leaderboard, const ScopeNameValueListMap* scopeNameValueListMap,
        int32_t periodId, EntityId centerEntityId, int32_t &entityRank) const;

    void doChangeKeyscopeValue(KeyScopeChangeRequestPtr request);
    BlazeRpcError changeKeyscopeValue(const KeyScopeChangeRequest& request);

    void cachePeriodIds(PeriodIdMap& currentPeriodIds);

    void incrementUpdateStatsCount() { mUpdateStatsCount.increment(); }
    void incrementUpdateStatsFailureCount() { mUpdateStatsFailedCount.increment(); }

private:

    friend class StatsTransactionContext;
    friend class DeleteStatsHelper;

    bool onConfigure() override;
    bool onReconfigure() override;
    bool onResolve() override;

    bool onValidateConfig(StatsConfig& config, const StatsConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;    

    void onShutdown() override;

    void onRemoteInstanceChanged(const RemoteServerInstance& instance) override;
    bool isDrainComplete() override;
    bool initMasterServer();

    void onUpdateCacheNotification(const StatUpdateNotification& data, UserSession *associatedUserSession) override;
    void onTrimPeriodNotification(const StatPeriod& data, UserSession *associatedUserSession) override;
    void onSetPeriodIdsNotification(const PeriodIds& data, UserSession *associatedUserSession) override;
    void onUpdateGlobalStatsNotification(const UpdateStatsRequest& data, UserSession *associatedUserSession) override;
    void onExecuteGlobalCacheInstruction(const StatsGlobalCacheInstruction &data, UserSession *associatedUserSession) override;

    GetStatCategoryListError::Error processGetStatCategoryList(Blaze::Stats::StatCategoryList &response, const Message *message) override;
    GetPeriodIdsError::Error processGetPeriodIds(PeriodIds &response, const Message *message) override;
    GetStatDescsError::Error processGetStatDescs(const Blaze::Stats::GetStatDescsRequest &request,Blaze::Stats::StatDescs &response, const Message *message) override;
    GetStatGroupListError::Error processGetStatGroupList(Blaze::Stats::StatGroupList &response, const Message *message) override;
    GetStatGroupError::Error processGetStatGroup(const Blaze::Stats::GetStatGroupRequest &request,Blaze::Stats::StatGroupResponse &response, const Message *message) override;
    GetDateRangeError::Error processGetDateRange(const Blaze::Stats::GetDateRangeRequest &request,Blaze::Stats::DateRange &response, const Message *message) override;
    GetLeaderboardGroupError::Error processGetLeaderboardGroup(const Blaze::Stats::LeaderboardGroupRequest &request,Blaze::Stats::LeaderboardGroupResponse &response, const Message *message) override;
    GetLeaderboardFolderGroupError::Error processGetLeaderboardFolderGroup(const Blaze::Stats::LeaderboardFolderGroupRequest &request,Blaze::Stats::LeaderboardFolderGroup &response, const Message *message) override;
    GetKeyScopesMapError::Error processGetKeyScopesMap(Blaze::Stats::KeyScopes &response, const Message *message) override;
    GetLeaderboardTreeAsyncError::Error processGetLeaderboardTreeAsync(const Blaze::Stats::GetLeaderboardTreeRequest &request, const Message *message) override;
    GetLeaderboardTreeAsync2Error::Error processGetLeaderboardTreeAsync2(const Blaze::Stats::GetLeaderboardTreeRequest &request, const Message *message) override;

    void getStatusInfo(ComponentStatus& status) const override;

    void updateAccountCountry(const UserSessionMaster& userSession, const char8_t* operation);

    BlazeRpcError validateStatDelete(const char8_t* categoryName, bool checkDependency, const ScopeNameValueMap& scopeNameValueMap, const PeriodTypeList& periodTypeList) const;

    // event handlers for User Session Manager events
    void onLocalUserSessionLogin(const UserSessionMaster& userSession) override;
    void onLocalUserAccountInfoUpdated(const UserSessionMaster& userSession) override;

    // function that runs on fiber when notification arrives
    void executeGlobalCacheInstruction(const StatsGlobalCacheInstruction data);
    void executeGlobalStatsUpdate(const UpdateStatsRequest request);

    void handleStatUpdate(const StatUpdateNotification& update);
    void startStatUpdateTimer();
    void stopStatUpdateTimer();
    void fetchStatUpdates(SequenceId fetchUpToSeqId);
    void handleQueuedStatUpdates(bool skipGaps, SequenceId processAtLeastToSeqId);

    // transaction context implementation override
    BlazeRpcError createTransactionContext(uint64_t customData, TransactionContext*& result) override;

private:
    BlazeRpcError processGetLeaderboardTreeAsyncInternal(const Blaze::Stats::GetLeaderboardTreeRequest &request, const Message *message);
    DebugTimer mDebugTimer;

    StatsConfigData* mStatsConfig;
    uint32_t mDbId;
    PeriodIds mPeriodIds;
    // User stats available through the user session object
    StatsExtendedDataProvider* mStatsExtendedDataProvider;
    LeaderboardIndexes* mLeaderboardIndexes;
    StatsCache* mStatsCache;
    bool mPopulated;
    typedef eastl::vector<StatUpdateNotification> UpdateNotificationList;
    UpdateNotificationList mBufferedUpdates;

    Metrics::Counter mUpdateStatsCount;
    Metrics::Counter mUpdateStatsFailedCount;

    StatUpdateMap mStatUpdateMap;

    uint64_t mStatUpdateSequenceVersion;
    SequenceId mNextStatUpdateSequenceId;
    TimerId mStatUpdateTimerId;

    GlobalStatsCache *mGlobalStatsCache;
    bool mLastGlobalCacheInstructionWasWrite;

public: 
    static const uint16_t MAX_FETCH_RETRY = 10;
};

} // Stats
} // Blaze
#endif // BLAZE_STATS_STATSSLAVEIMPL_H
