/*************************************************************************************************/
/*!
    \file   dbinstancepool.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_DBINSTANCEPOOL_H
#define BLAZE_DBINSTANCEPOOL_H

/*** Include files *******************************************************************************/
#include "EASTL/intrusive_list.h"
#include "EASTL/list.h"
#include "EASTL/priority_queue.h"
#include "EASTL/hash_map.h"
#include "EASTL/deque.h"
#include "EASTL/bonus/ring_buffer.h"
#include "eathread/eathread_condition.h"
#include "eathread/eathread_mutex.h"
#include "framework/database/dbconfig.h"
#include "framework/database/preparedstatement.h"
#include "framework/database/dbconn.h"
#include "framework/system/fiber.h"
#include "framework/tdf/networkaddress.h"
#include "framework/metrics/metrics.h"


/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class DbConfig;
class DbAdmin;
class DbConnPoolStatus;
class DbConnPool;

typedef enum { DB_MASTER, DB_SLAVE } DbRole;

typedef DbAdmin* (*CreateDbAdminFunc)(const DbConfig& config, DbInstancePool& owner);

//  Note, this is the source container for all DbConn objects.   Any users of this class must use
//  DbConn objects, derived from DbConnBase.   
//  Currently it's GUARANTEED that any DbConn object is derived from a DbConnBase, so up-casting 
//  DbConnBase to DbConn is safe. 
class DbInstancePool : public eastl::intrusive_list_node
{
    NON_COPYABLE(DbInstancePool);


public:
    DbInstancePool(DbConnPool& owner, const DbConfig& config, DbRole role, DbConn::Mode mode, uint32_t id);
    ~DbInstancePool();

    DbConnPool& getDbConnPool() const { return mOwner; }
    const DbConfig& getDbConfig() const { return mDbConfig; }

    BlazeRpcError DEFINE_ASYNC_RET(initialize(CreateDbAdminFunc createDbAdminFunc));
    BlazeRpcError DEFINE_ASYNC_RET(start());
    BlazeRpcError DEFINE_ASYNC_RET(reconfigureStart());
    

    DbConn* activateConnection(bool& squelching);
    void deactivateConnection(DbConn& conn, bool replace = false);

    uint64_t getActivePoolCount() const { return mActivePoolCount.get(); }
    uint64_t getInactivePoolCount() const { return mInactivePoolCount.get(); }

    void getStatus(DbInstancePoolStatus& status) const;

    bool isSquelching() const { return mSquelching; }
    bool isDraining() const { return mDraining; }

    // markForDrain() only sets a flag.  Which is then used as an indication to
    // drain all active connections and then delete itself.
    void markForDrain();

    void incrementQueryCount(BlazeRpcError error)
    {
        mTotalQueries.increment();
        if (error != DB_ERR_OK)
            mTotalErrors.increment();
    }

    void incrementMultiQueryCount(BlazeRpcError error)
    {
        mTotalMultiQueries.increment();
        if (error != DB_ERR_OK)
            mTotalErrors.increment();
    }

    void incrementPreparedStatementCount(BlazeRpcError error)
    {
        mTotalPreparedStatementQueries.increment();
        if (error != DB_ERR_OK)
            mTotalErrors.increment();
    }

    void incrementCommitCount(BlazeRpcError error)
    {
        mTotalCommits.increment();
        if (error != DB_ERR_OK)
            mTotalErrors.increment();
    }

    void incrementRollbackCount(BlazeRpcError error)
    {
        mTotalRollbacks.increment();
        if (error != DB_ERR_OK)
            mTotalErrors.increment();
    }

    void updateQueryAverageMetrics(EA::TDF::TimeValue now, EA::TDF::TimeValue queryTime);

    DbAdmin& getDbAdmin() const { return *mAdmin; }

    bool isOverActiveConnectionLimit() const
    {
        return (mActivePoolCount.get() >= mCurrentMaxConnCount.get());
    }

    void updateMaxConnCount(uint32_t count);
    void updateTimeouts(const DbConnConfig& config);
    void allowPeakPoolCountManipulation(bool allow)
    {
        mAllowPeakPoolCountManipulation = allow;
    }

    uint32_t getId() const { return mId; }
    DbConn::Mode getMode() const { return mMode; }

private:
    friend class DbConnPool;

    void checkDbStatus();
    void checkQueryStatus();

    void destroy();

    DbConn* moveConnectionToActive();
    void getTrimList(eastl::vector<DbConn*>& trimList);
    uint32_t getSquelchedConnectionCount() const;
    const char8_t* getDisplayString(char8_t* buffer, size_t len);
    void resetSquelchingState();
    bool isSquelchingEnabled() const;
    void pushQueryVarianceHistory(float variance, int64_t prevAverage, int64_t prevDeviation, EA::TDF::TimeValue now);

private:
    typedef eastl::intrusive_list<DbConnNode> DbConnList;

    DbConnPool& mOwner;
    DbConfig mDbConfig;
    const DbRole mRole;
    const DbConn::Mode mMode;

    Metrics::MetricsCollection& mMetricsCollection;

    DbConnList mActivePool;
    Metrics::Gauge mActivePoolCount;
    DbConnList mInactivePool;
    Metrics::Gauge mInactivePoolCount;
    Metrics::PolledGauge mTotalConnections;

    Metrics::Gauge mPeakPoolCount;
    bool mAllowPeakPoolCountManipulation;

    DbAdmin* mAdmin;

    bool mSquelching;
    bool mDraining;
    TimerId mDbCheckTimerId;
    bool mDbStatusCheckInProgress;

    Metrics::Counter mTotalErrors;
    Metrics::Counter mTotalQueries;
    Metrics::Counter mTotalMultiQueries;
    Metrics::Counter mTotalPreparedStatementQueries;
    Metrics::Counter mTotalCommits;
    Metrics::Counter mTotalRollbacks;

    // Weighted average and deviation for queries executed on this pool
    int64_t mCurrentQueryTimeAverage;
    int64_t mCurrentQueryTimeDeviation;

    int64_t mPreviousQueryTimeAverage;
    int64_t mPreviousQueryTimeDeviation;

    int64_t mBaselineQueryTimeAverage;
    int64_t mBaselineQueryTimeDeviation;

    // The time when the query time baselining process completes
    EA::TDF::TimeValue mBaselineEndTime;
    bool mBaselineSet;

    struct QueryTimeMetrics
    {
        int64_t mAverage;
        int64_t mDeviation;
        float mVariance;
        bool mSquelched;
        EA::TDF::TimeValue mWhen;
    };
    typedef eastl::ring_buffer<QueryTimeMetrics, eastl::vector<QueryTimeMetrics> > QueryVarianceHistory;
    QueryVarianceHistory mQueryVarianceHistory;

    Metrics::Gauge mCurrentMaxConnCount;
    Metrics::Counter mTotalQueryTimeThresholdCrosses;

    const uint32_t mId;

    Metrics::PolledGauge mDrainingMetric;
    Metrics::PolledGauge mSquelchingMetric;
    Metrics::PolledGauge mQueryTimeAverageMs;
    Metrics::PolledGauge mQueryTimeDeviationMs;
};

} // Blaze

#endif // BLAZE_DBINSTANCEPOOL_H


