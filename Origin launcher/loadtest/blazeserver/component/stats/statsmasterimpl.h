/*************************************************************************************************/
/*!
    \file   statsmasterimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_STATS_STATSMASTERIMPL_H
#define BLAZE_STATS_STATSMASTERIMPL_H

/*** Include files *******************************************************************************/
#include "stats/statscomponentinterface.h"
#include "stats/rpc/statsmaster_stub.h"
#include "rollover.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
namespace Stats
{
// Forward declarations
class StatsConfigData;
class StatCategory;
class LeaderboardIndexes;

typedef eastl::list<EA::TDF::TdfBlobPtr> LeaderboardEntries; // for extraction and population

class StatsMasterImpl : public StatsMasterStub, public StatsComponentInterface, private StatsSlaveListener
{
public:
    StatsMasterImpl();
    ~StatsMasterImpl() override;
    
    const StatsConfigData* getConfigData() const override { return mStatsConfig; }

    uint32_t getDbId() const override { return mDbId; }

    int32_t getPeriodId(int32_t periodType) override;
    int32_t getRetention(int32_t periodType) override;

    void trimPeriods(const StatPeriod& statPeriod);

private:
    friend class StatsBootstrap;
    friend class Rollover;

    bool onResolve() override;
    bool onConfigure() override;
    bool onReconfigure() override;
    void onShutdown() override;
    void configureCommon();
    bool onValidateConfig(StatsConfig& config, const StatsConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;    

    void onSlaveSessionAdded(SlaveSession &session) override;
    void onSlaveSessionRemoved(SlaveSession& session) override;

    void onUpdateCacheNotification(const StatUpdateNotification& data, UserSession *associatedUserSession) override;
    void onUpdateGlobalStatsNotification(const UpdateStatsRequest& data, UserSession *associatedUserSession) override;

    GetPeriodIdsMasterError::Error processGetPeriodIdsMaster(PeriodIds &response, const Message *message) override;
    PopulateLeaderboardIndexError::Error processPopulateLeaderboardIndex(
        const Blaze::Stats::PopulateLeaderboardIndexRequest &request, Blaze::Stats::PopulateLeaderboardIndexData &response, const Message *message) override;
    ReportGlobalCacheInstructionExecutionResultError::Error 
        processReportGlobalCacheInstructionExecutionResult(
            const Blaze::Stats::GlobalCacheInstructionExecutionResult &request, const Message *message) override;
    InitializeGlobalCacheError::Error processInitializeGlobalCache(
        const Blaze::Stats::InitializeGlobalCacheRequest &request, const Message *message) override;

    void startGlobalCacheSynchronization(SlaveSession &session);

    // periodically called to write global cache to database
    void onWriteGlobalCacheToDatabase();
    // instruct slaves to write cache to db or just clear row statuses
    void issueWriteInstructionToSlaves(bool forceWrite);

    bool mBootstrapSuccess;
    typedef eastl::vector<StatUpdateNotification> UpdateNotificationList;
    UpdateNotificationList mBufferedUpdates;
    LeaderboardIndexes* mLeaderboardIndexes;
    StatsConfigData* mStatsConfig;
    uint32_t mDbId;

    Rollover mRollover;

    typedef eastl::hash_map<SlaveSessionId, LeaderboardEntries*> SlaveSessionStatsMap;
    SlaveSessionStatsMap mSlaveSessionStatsMap;

    // global cache timer
    TimerId mGlobalCacheTimerId;
    // hardcode serialization interval to 1 min
    static const long GLOBAL_CACHE_SERIALIZATION_INTERVAL = 60000000L;

    typedef eastl::hash_set<InstanceId> InstanceIdSet;
    // list of slaves using global cache/stats
    InstanceIdSet mGlobalCacheSlaveIds;
    // list of new slaves using global cache/stats added to the master before database
    // persisted data is ready for picking up by the new slaves
    InstanceIdSet mNewGlobalCacheSlaveIds;
    // indicates if the cache is being serialized to database.
    // should the slave selected to serialize cache terminate during the
    // the operation then we need to force write next time timer expires
    bool mGlobalCacheIsBeingSerialized;
};

} // Stats
} // Blaze
#endif // BLAZE_STATS_STATSMASTERIMPL_H
