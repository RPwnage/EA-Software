/*************************************************************************************************/
/*!
    \file   dbinstancepool.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class DbConnPool

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/dbconninterface.h"
#include "framework/database/dbinstancepool.h"
#include "framework/database/dbconnpool.h"
#include "framework/database/dbadmin.h"
#include "framework/database/dbscheduler.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/controller/controller.h"
#include "framework/connection/selector.h"
#include "EASTL/algorithm.h"
#include "framework/util/random.h"
#include "framework/database/mysql/mysqladmin.h"
#include "framework/database/mysql/mysqlasyncconn.h"

namespace Blaze
{


/*** Defines/Macros/Constants/Typedefs ***********************************************************/

// Amount of time between keep-alive pings to the database when under normal operation
#define CHECK_DB_STATUS_NORMAL_INTERVAL     (10 * 1000 * 1000) // 10 seconds in micro-seconds

// Amount of time between keep-alive pings to the database when squelching
#define CHECK_DB_STATUS_SQUELCHING_INTERVAL (1 * 1000 * 1000) // 1 second in micro-seconds

/*** Public Methods ******************************************************************************/

const char8_t* DbRoleToString(const Blaze::DbRole& value)
{
    switch (value)
    {
    case DB_MASTER:
        return "master";
    case DB_SLAVE:
        return "slave";
    }
    return "unknown";
}

namespace Metrics
{
    namespace Tag
    {
        Metrics::TagInfo<DbRole>* db_role = BLAZE_NEW Metrics::TagInfo<DbRole>("db_role", [](const Blaze::DbRole& role, Metrics::TagValue&) { return DbRoleToString(role); });
        Metrics::TagInfo<const char*>* db_hostname = BLAZE_NEW TagInfo<const char*>("db_hostname");
    }
}

DbInstancePool::DbInstancePool(DbConnPool& owner, const DbConfig& config, DbRole role, DbConn::Mode mode, uint32_t id)
    : mOwner(owner),
      mDbConfig(config), // clone copy
      mRole(role),
      mMode(mode),
      mMetricsCollection(owner.mMetricsCollection.getCollection(Metrics::Tag::db_role, mRole).getCollection(Metrics::Tag::db_hostname, mDbConfig.getHost())),
      mActivePoolCount(mMetricsCollection, "database.dbpool.activePoolCount"),
      mInactivePoolCount(mMetricsCollection, "database.dbpool.inactivePoolCount"),
      mTotalConnections(mMetricsCollection, "database.dbpool.connections", [this]() { return mActivePoolCount.get() + mInactivePoolCount.get(); }),
      mPeakPoolCount(mMetricsCollection, "database.dbpool.peakConnections"),
      mAllowPeakPoolCountManipulation(true),
      mAdmin(nullptr),
      mSquelching(false),
      mDraining(false),
      mDbCheckTimerId(INVALID_TIMER_ID),
      mDbStatusCheckInProgress(false),
      mTotalErrors(mMetricsCollection, "database.dbpool.errors"),
      mTotalQueries(mMetricsCollection, "database.dbpool.queries"),
      mTotalMultiQueries(mMetricsCollection, "database.dbpool.multiQueries"),
      mTotalPreparedStatementQueries(mMetricsCollection, "database.dbpool.preparedStatementQueries"),
      mTotalCommits(mMetricsCollection, "database.dbpool.commits"),
      mTotalRollbacks(mMetricsCollection, "database.dbpool.rollbacks"),
      mCurrentQueryTimeAverage(0),
      mCurrentQueryTimeDeviation(0),
      mPreviousQueryTimeAverage(0),
      mPreviousQueryTimeDeviation(0),
      mBaselineQueryTimeAverage(0),
      mBaselineQueryTimeDeviation(0),
      mBaselineEndTime(0),
      mBaselineSet(false),
      mQueryVarianceHistory(gDbScheduler->getDbQueryVarianceHistorySize()),
      mCurrentMaxConnCount(mMetricsCollection, "database.dbpool.availableConnections"),
      mTotalQueryTimeThresholdCrosses(mMetricsCollection, "database.dbpool.queryTimeThresholdCrosses"),
      mId(id),
      mDrainingMetric(mMetricsCollection, "database.dbpool.draining", [this]() { return (uint64_t)mDraining; }),
      mSquelchingMetric(mMetricsCollection, "database.dbpool.squelching", [this]() { return (uint64_t)mSquelching; }),
      mQueryTimeAverageMs(mMetricsCollection, "database.dbpool.queryTimeAverageMs", [this]() { return mCurrentQueryTimeAverage; }),
      mQueryTimeDeviationMs(mMetricsCollection, "database.dbpool.queryTimeDeviationMs", [this]() { return mCurrentQueryTimeDeviation; })


{
    mpPrev = mpNext = nullptr;
}

DbInstancePool::~DbInstancePool()
{
    //Kill our inactive db conns
    while (!mInactivePool.empty())
    {
        DbConn* orphan = &mInactivePool.front().getDbConn();
        mInactivePool.pop_front();
        if (orphan)
            delete orphan;
    }

    //Shouldn't be anything in here, but just in case.
    while (!mActivePool.empty())
    {
        DbConn* orphan = &mActivePool.front().getDbConn();
        mActivePool.pop_front();
        if (orphan)
        {
            if (orphan->isConnected())
            {
                char8_t displayBuf[256];
                BLAZE_WARN_LOG(Log::DB, "[DbInstancePool].dtor: " << getDisplayString(displayBuf, sizeof(displayBuf)) << "deleting active connection " << orphan->getName());
            }
            delete orphan;
        }
    }

    if (mDbCheckTimerId != INVALID_TIMER_ID)
        gSelector->cancelTimer(mDbCheckTimerId);

    if (mAdmin != nullptr)
    {
        if (!mOwner.getUseAsyncDbConns())
            mAdmin->unregisterDbThread();

        mAdmin->stop();
        delete mAdmin;
    }
}

// this doesn't actually destroy anything, but rather, sets up the condition
// to drain all active db connections, and then delete itself
void DbInstancePool::markForDrain()
{
    if (mDbCheckTimerId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mDbCheckTimerId);
        mDbCheckTimerId = INVALID_TIMER_ID;
    }

    if (mActivePool.size() == 0 && !mDbStatusCheckInProgress)
    {
        destroy();
    }
    else
    {
        mDraining = true;
    }
}

void DbInstancePool::destroy()
{
    if (!mOwner.getUseAsyncDbConns())
        gDbScheduler->removeWorkers(mDbConfig.getMaxConnCount());

    delete this;
}

BlazeRpcError DbInstancePool::initialize(CreateDbAdminFunc createDbAdminFunc)
{
    mAdmin = createDbAdminFunc(mDbConfig, *this);
    if (!mOwner.getUseAsyncDbConns())
    {
        BlazeRpcError rc = mAdmin->registerDbThread();
        if (rc == ERR_OK)
        {
            if (mAdmin->start() == EA::Thread::kThreadIdInvalid)
            {
                char8_t displayBuf[256];
                BLAZE_ERR_LOG(Log::DB, "[DbInstancePool].initialize: " << getDisplayString(displayBuf, sizeof(displayBuf)) << " failed to start MySqlAdmin thread");
                rc = ERR_SYSTEM;
                mAdmin->unregisterDbThread();
            }
        }

        if (rc != ERR_OK)
        {
            delete mAdmin;
            mAdmin = nullptr;
            return rc;
        }
    }
    
    // TODO: We shouldn't need to precreate all these connections on startup because they are not connected anyway. We must be able to create them on demand, so let's exercise this feature.
    // Pre-create all the connection objects
    mCurrentMaxConnCount.set(mDbConfig.getMaxConnCount());
    for(uint32_t idx = 0; idx < mCurrentMaxConnCount.get(); ++idx)
    {
        DbConn* conn = BLAZE_NEW DbConn(mDbConfig, *mAdmin, *this, idx, mMode);
        mInactivePool.push_back(*conn);
        mInactivePoolCount.increment();
    }

    if (!mOwner.getUseAsyncDbConns())
        gDbScheduler->addWorkers(mCurrentMaxConnCount.get());

    return DB_ERR_OK;
}

void DbInstancePool::updateMaxConnCount(uint32_t count)
{
    if (count > mDbConfig.getMaxConnCount())
    {
        size_t connsAdded = count - mDbConfig.getMaxConnCount();
        for (uint32_t idx = mDbConfig.getMaxConnCount(); idx < count; ++idx)
        {
            DbConn* conn = BLAZE_NEW DbConn(mDbConfig, *mAdmin, *this, idx, mMode);
            mInactivePool.push_back(*conn);
            mInactivePoolCount.increment();
        }

        if (!mOwner.getUseAsyncDbConns())
            gDbScheduler->addWorkers(connsAdded);
    }
    // else if count < geetMaxConnCount, we rely on trimOpenConnections() to shrink the conn pool

    // update maxConnCount before resetSquelchingState, because latter depends on former
    mDbConfig.setMaxConnCount(count);
    resetSquelchingState();
}

void DbInstancePool::updateTimeouts(const DbConnConfig& config)
{
    mDbConfig.setTimeouts(config);
}

BlazeRpcError DbInstancePool::reconfigureStart()
{
    if (mDbCheckTimerId == INVALID_TIMER_ID)
    {
        mDbCheckTimerId = gSelector->scheduleFiberTimerCall(
            TimeValue::getTimeOfDay()
            + (mSquelching ? CHECK_DB_STATUS_SQUELCHING_INTERVAL : CHECK_DB_STATUS_NORMAL_INTERVAL),
            this, &DbInstancePool::checkDbStatus,
            "DbInstancePool::checkDbStatus"
            );
    }

    return DB_ERR_OK;
}

void DbInstancePool::resetSquelchingState()
{
    // Clear out any DB query time squelching so that the stored state can be resynchronized
    // with the new config values.
    char8_t displayBuf[256];
    BLAZE_INFO_LOG(Log::DB, "[DbInstancePool].resetSquelchingState: "
            << getDisplayString(displayBuf, sizeof(displayBuf))
            << ": resetting squelching state due to reconfigure.");

    // Reset the history and max connection count
    while (!mQueryVarianceHistory.empty())
        mQueryVarianceHistory.pop_front();
    mCurrentMaxConnCount.set(mDbConfig.getMaxConnCount());

    float threshold = gDbScheduler->getDbQueryVarianceThreshold();
    if ((threshold > 0) && mBaselineSet)
    {
        // Attempt to re-populate any squelching history based on the newly configured parameters.
        // Create history entries based on the new threshold settings and the current variance from
        // the baseline.
        float curr = (float)(mCurrentQueryTimeAverage + mCurrentQueryTimeDeviation);
        float base = (float)(mBaselineQueryTimeAverage + mBaselineQueryTimeDeviation);

        if (curr > base)
        {
            TimeValue now = TimeValue::getTimeOfDay();
            float val = (float)mCurrentQueryTimeAverage;
            for(uint32_t idx = 0; idx < gDbScheduler->getDbQueryVarianceHistorySize(); ++idx)
            {
                val = val - (val * threshold);
                if (val < base)
                    break;
                pushQueryVarianceHistory(threshold, (int64_t)val, 0, now);
            }
        }
    }
}

BlazeRpcError DbInstancePool::start()
{
    mDbCheckTimerId = gSelector->scheduleFiberTimerCall(
            TimeValue::getTimeOfDay()
            + (mSquelching ? CHECK_DB_STATUS_SQUELCHING_INTERVAL : CHECK_DB_STATUS_NORMAL_INTERVAL),
            this, &DbInstancePool::checkDbStatus,
            "DbInstancePool::checkDbStatus"
        );

    return DB_ERR_OK;
}

DbConn* DbInstancePool::activateConnection(bool& squelching)
{
    // if we are squelching or draining, then don't activate any connection
    squelching = mSquelching || mDraining;
    if (squelching)
        return nullptr;

    if (isOverActiveConnectionLimit())
    {
        // mCurrentMaxConnCount could be less than the total number of connections because
        // squelching may have kicked in so don't assign out new connection if over the limit.
        BLAZE_TRACE_LOG(Log::DB, "[DbInstancePool].activateConnection: db(" << mDbConfig.getName() <<
            ") over active limit; owner(" << &mOwner <<
            "); activePoolCount(" << mActivePoolCount.get() <<
            "); currentMaxConnCount(" << mCurrentMaxConnCount.get() <<
            "); summaryMaxConnCount(" << getDbConfig().getSummaryMaxConnCount() <<
            "); slave count(" << getDbConfig().getSlaves().size() << ").");
        return nullptr;
    }

    DbConn *result = moveConnectionToActive();
    if (result != nullptr && mAllowPeakPoolCountManipulation)
    {
        if (mActivePoolCount.get() > mPeakPoolCount.get())
            mPeakPoolCount.set(mActivePoolCount.get());
    }

    BLAZE_TRACE_LOG(Log::DB, "[DbInstancePool].activateConnection: db(" << mDbConfig.getName() << ") activated(" << (result != nullptr ? result->getName() : "<no conn available>") <<
        "); owner(" << &mOwner <<
        "); activePoolCount(" << mActivePoolCount.get() <<
        "); currentMaxConnCount(" << mCurrentMaxConnCount.get() <<
        "); inactivePoolCount(" << mInactivePoolCount.get() <<
        "); summaryMaxConnCount(" << getDbConfig().getSummaryMaxConnCount() <<
        "); slave count(" << getDbConfig().getSlaves().size() << ").");

    return result;
}

DbConn* DbInstancePool::moveConnectionToActive()
{
    DbConn *result = nullptr;
    if (!mInactivePool.empty())
    {
        result = &mInactivePool.front().getDbConn();
        
        // always pop first before possibly blocking in connect() below
        mInactivePool.pop_front();
        mInactivePoolCount.decrement();
        result->activate();

        mActivePool.push_back(*result);
        mActivePoolCount.increment();
    }
    return result;
}

void DbInstancePool::deactivateConnection(DbConn& conn, bool replace)
{
    conn.deactivate();
    mActivePool.remove(conn);
    mActivePoolCount.decrement();

    if (EA_UNLIKELY(mDraining))
    {
        delete &conn;
        if (mActivePool.size() == 0 && !mDbStatusCheckInProgress)
        {
            destroy();
        }
        return;
    }

    if (replace)
    {
        if (conn.isConnected())
        {
            conn.disconnect(DB_ERR_OK);
        }
        conn.invalidate(); // this conn shouldn't be used anymore
        if (mDbConfig.getMaxConnCount() > mActivePoolCount.get() + mInactivePoolCount.get())
        {
            DbConn* replacementConn = BLAZE_NEW DbConn(mDbConfig, *mAdmin, *this, conn.getConnNum(), conn.getMode());
            mInactivePool.push_back(*replacementConn);
            mInactivePoolCount.increment();
        }
    }
    else
    {
        if (mDbConfig.getMaxConnCount() > mActivePoolCount.get() + mInactivePoolCount.get())
        {
            // Put connected DbConns at the front of the queue and disconnected ones at the end
            if (conn.isConnected())
                mInactivePool.push_front(conn);
            else
                mInactivePool.push_back(conn);
            mInactivePoolCount.increment();
        }
        else
        {
            if (conn.isConnected())
            {
                conn.disconnect(DB_ERR_OK);
            }

            if (!mOwner.getUseAsyncDbConns())
            {
                gDbScheduler->removeWorkers(1);
            }
            delete &conn;
        }
    }
}

void DbInstancePool::getStatus(DbInstancePoolStatus& status) const 
{
    status.setHost(mDbConfig.getHost());
    status.setPort((uint16_t)mDbConfig.getPort());
    status.setUser(mDbConfig.getUser());
    status.setTotalConnections((uint32_t)mTotalConnections.get());
    status.setTotalAvailableConnections((uint32_t)mCurrentMaxConnCount.get());
    status.setActiveConnections((uint32_t)mActivePoolCount.get());
    status.setPeakConnections((uint32_t)mPeakPoolCount.get());
    status.setSquelching(mSquelching);
    status.setDraining(mDraining);
    status.setTotalErrors(mTotalErrors.getTotal());
    status.setTotalQueries(mTotalQueries.getTotal());
    status.setTotalMultiQueries(mTotalMultiQueries.getTotal());
    status.setTotalPreparedStatementQueries(mTotalPreparedStatementQueries.getTotal());
    status.setTotalCommits(mTotalCommits.getTotal());
    status.setTotalRollbacks(mTotalRollbacks.getTotal());
    status.setQueryTimeAverageMs(mCurrentQueryTimeAverage);
    status.setQueryTimeDeviationMs(mCurrentQueryTimeDeviation);
    status.setQueryTimeBaselineAverageMs(mBaselineQueryTimeAverage);
    status.setQueryTimeBaselineDeviationMs(mBaselineQueryTimeDeviation);
    status.setTotalQueryTimeThresholdCrosses(mTotalQueryTimeThresholdCrosses.getTotal());
}

void DbInstancePool::checkDbStatus()
{
    mDbStatusCheckInProgress = true;
    if (mAdmin != nullptr)
    {
        BlazeRpcError result = mAdmin->checkDbStatus();
        if (result != DB_ERR_OK)
        {
            if (!mSquelching)
            {
                BLAZE_WARN_LOG(Log::DB, "[DbInstancePool].checkDbStatus: unable to ping database "
                    "" << mDbConfig.getIdentity() << "; squelching attempts to obtain connections.");
            }
            mSquelching = true;
        }
        else
        {
            if (mSquelching)
            {
                BLAZE_INFO_LOG(Log::DB, "[DbInstancePool].checkDbStatus: connection successfully "
                    "re-established to " << mDbConfig.getIdentity() << "; unsquelching attempts to obtain connections.");
            }
            mSquelching = false;
        }
    }

    mDbStatusCheckInProgress = false;

    if (!mDraining)
    {
        mDbCheckTimerId = gSelector->scheduleFiberTimerCall(
            TimeValue::getTimeOfDay()
            + (mSquelching ? CHECK_DB_STATUS_SQUELCHING_INTERVAL : CHECK_DB_STATUS_NORMAL_INTERVAL),
            this, &DbInstancePool::checkDbStatus,
            "DbInstancePool::checkDbStatus");
    }
    else if (mActivePool.size() == 0)
        destroy(); // Need to destroy here since we won't do it in markForDrain() or deactivateConnection()
}

void DbInstancePool::updateQueryAverageMetrics(TimeValue now, TimeValue queryTime)
{
    // Update the overall query time average and deviation for this pool.
    // This code is based on the RTT calculations used in DirtySDK's NetGameLink
    // which is in itself derived from http://ee.lbl.gov/papers/congavoid.pdf.

    int64_t nowMs = now.getMillis();
    int64_t queryTimeMs = queryTime.getMillis();
    int64_t samplePeriod = gDbScheduler->getQuerySamplePeriod().getMillis();
    int64_t samplePeriodStart = mOwner.getQuerySamplePeriodStart().getMillis();

    if (mCurrentQueryTimeAverage == 0)
    {
        // The period rolled over since the last call to this function
        mCurrentQueryTimeAverage = queryTimeMs;
        mCurrentQueryTimeDeviation = 0;
        return;
    }

    int64_t elapsed = (nowMs - samplePeriodStart);
    if (elapsed < 0)
        elapsed = 0;
    int64_t weight = samplePeriod - elapsed;
    int64_t deviate = queryTimeMs - mCurrentQueryTimeAverage;
    if (deviate < 0)
        deviate = -deviate;
    mCurrentQueryTimeDeviation = (weight * mCurrentQueryTimeDeviation + elapsed * deviate) / samplePeriod;
    mCurrentQueryTimeAverage = (weight * mCurrentQueryTimeAverage + elapsed * queryTimeMs) / samplePeriod;
}

uint32_t DbInstancePool::getSquelchedConnectionCount() const
{
    // Determine how many connections should be made available based on the current
    // variation from the baseline query time averages.

    float baseline = (float)(mBaselineQueryTimeAverage + mBaselineQueryTimeDeviation);
    float current = (float)(mCurrentQueryTimeAverage + mCurrentQueryTimeDeviation);
    float baselineVariation = current / baseline;
    if (baselineVariation < 1.0)
        baselineVariation = 0;
    float pctOfMaxDeviation = baselineVariation / gDbScheduler->getDbSquelchingMaxQueryVariance();
    if (pctOfMaxDeviation > 1.0)
        pctOfMaxDeviation = 1.0;

    float minConnCount = (float)(mDbConfig.getMaxConnCount()) * gDbScheduler->getDbSquelchingMinConnectionCountPercent();
    float newConnCount = minConnCount + ((mDbConfig.getMaxConnCount() - minConnCount) * (1 - pctOfMaxDeviation));
    return (uint32_t)(newConnCount + 0.5);
}

const char8_t* DbInstancePool::getDisplayString(char8_t* buffer, size_t len)
{
    blaze_snzprintf(buffer, len, "%s/%s (%s:%d)", mDbConfig.getName(), DbConn::modeToString(mMode),
            mDbConfig.getHost(), mDbConfig.getPort());
    return buffer;
}

void DbInstancePool::checkQueryStatus()
{
    char8_t displayBuf[256];

    if (gController->getState() != ComponentStub::ACTIVE)
    {
        // Don't calculate any metrics or do any squelching if the component hasn't done
        // active yet.
        return;
    }

    TimeValue now = TimeValue::getTimeOfDay();

    if (mBaselineEndTime == 0)
    {
        // Server is active now so start the baseline duration
        mBaselineEndTime = now + gDbScheduler->getDbQueryTimeBaselineDuration();
        return;
    }
    else if (now < mBaselineEndTime)
    {
        // Still baselining the query times so just update the baseline values if the current
        // period's query times are bigger.
        if ((mCurrentQueryTimeAverage + mCurrentQueryTimeDeviation) > (mBaselineQueryTimeAverage + mBaselineQueryTimeDeviation))
        {
            mBaselineQueryTimeAverage = mCurrentQueryTimeAverage;
            mBaselineQueryTimeDeviation = mCurrentQueryTimeDeviation;
        }

        mPreviousQueryTimeAverage = mCurrentQueryTimeAverage;
        mPreviousQueryTimeDeviation = mCurrentQueryTimeDeviation;
        mCurrentQueryTimeAverage = 0;
        mCurrentQueryTimeDeviation = 0;
        return;
    }

    if (!mBaselineSet)
    {
        mBaselineSet = true;

        int64_t baselineMin = gDbScheduler->getDbQueryTimeBaselineMinimum().getMillis();
        if (mBaselineQueryTimeAverage < baselineMin)
        {
            // The computed baseline is less than the allowable minimum so bump it up
            mBaselineQueryTimeAverage = baselineMin;
            mBaselineQueryTimeDeviation = 0;
        }

        BLAZE_INFO_LOG(Log::DB, "[DbInstancePool].checkQueryStatus: "
                << getDisplayString(displayBuf, sizeof(displayBuf))
                << ": baseline query time set (avg=" << mBaselineQueryTimeAverage
                << " dev=" << mBaselineQueryTimeDeviation << ")");
    }

    float threshold = gDbScheduler->getDbQueryVarianceThreshold();
    float curr = (float)(mCurrentQueryTimeAverage + mCurrentQueryTimeDeviation);
    float prev = (float)(mPreviousQueryTimeAverage + mPreviousQueryTimeDeviation);
    float base = (float)(mBaselineQueryTimeAverage + mBaselineQueryTimeDeviation);

    // Only check for squelching if we have a valid variance config and we have a previous
    // query time average to compare against
    if ((threshold > 0) && (prev > 0))
    {
        // Check if the squelching variance has been exceeded
        float variance = (curr - prev) / prev;

        BLAZE_TRACE_LOG(Log::DB, "[DbInstancePool].checkQueryStatus: "
                << getDisplayString(displayBuf, sizeof(displayBuf))
                << ": curr=" << curr << " prev=" << prev << " variance=" << variance);

        if (variance > threshold)
        {
            if (curr > base)
            {
                // Only push a history entry if the exceeded threshold was above the  baseline
                mTotalQueryTimeThresholdCrosses.increment();
                pushQueryVarianceHistory(variance, mPreviousQueryTimeAverage, mPreviousQueryTimeDeviation, now);
            }
        }
        else if (mQueryVarianceHistory.size() > 0)
        {
            // Check if the query times have dropped down below previous squelching thresholds
            while (!mQueryVarianceHistory.empty())
            {
                QueryTimeMetrics& metrics = mQueryVarianceHistory.front();
                if ((mCurrentQueryTimeAverage + mCurrentQueryTimeDeviation) < (metrics.mAverage + metrics.mDeviation))
                {
                    char8_t whenStr[64];
                    BLAZE_INFO_LOG(Log::DB, "[DbInstancePool].checkQueryStatus: "
                            << getDisplayString(displayBuf, sizeof(displayBuf))
                            << ": recovered from query time variance threshold exception: "
                            << (variance * 100) << "% curr(avg=" << mCurrentQueryTimeAverage
                            << " dev=" << mCurrentQueryTimeDeviation << ") prev(avg="
                            << mPreviousQueryTimeAverage << " dev=" << mPreviousQueryTimeDeviation
                            << ") hist(avg=" << metrics.mAverage << " dev=" << metrics.mDeviation
                            << " when=" << metrics.mWhen.toString(whenStr, sizeof(whenStr))
                            << ") newHistorySize=" << (mQueryVarianceHistory.size() - 1));

                    // Unsquelch the connection count if necessary
                    if (metrics.mSquelched)
                    {
                        uint32_t newMaxConnCount = getSquelchedConnectionCount();
                        if (newMaxConnCount > mDbConfig.getMaxConnCount())
                            newMaxConnCount = mDbConfig.getMaxConnCount();
                        if (newMaxConnCount != mCurrentMaxConnCount.get())
                        {
                            mCurrentMaxConnCount.set(newMaxConnCount);

                            BLAZE_INFO_LOG(Log::DB, "[DbInstancePool].checkQueryStatus: "
                                    << getDisplayString(displayBuf, sizeof(displayBuf))
                                    << ": connection count squelching reverted; "
                                    "increasing max connection count to " << mCurrentMaxConnCount.get()
                                    << "; configured max " << mDbConfig.getMaxConnCount());
                        }
                    }

                    mQueryVarianceHistory.pop_front();
                }
                else
                {
                    break;
                }
            }
        }
    }

    mPreviousQueryTimeAverage = mCurrentQueryTimeAverage;
    mPreviousQueryTimeDeviation = mCurrentQueryTimeDeviation;
    mCurrentQueryTimeAverage = 0;
    mCurrentQueryTimeDeviation = 0;
}

void DbInstancePool::pushQueryVarianceHistory(float variance, int64_t prevAverage, int64_t prevDeviation, TimeValue now)
{
    char8_t displayBuf[256];

    // Push a new history record
    mQueryVarianceHistory.push_front();
    QueryTimeMetrics& metrics = mQueryVarianceHistory.front();
    metrics.mAverage = prevAverage;
    metrics.mDeviation = prevDeviation;
    metrics.mVariance = variance;
    metrics.mSquelched = false;
    metrics.mWhen = now;

    BLAZE_INFO_LOG(Log::DB, "[DbInstancePool].pushQueryVarianceHistory: "
            << getDisplayString(displayBuf, sizeof(displayBuf))
            << ": exceeded query time variance threshold: variance: "
            << (variance * 100) << "% curr(avg=" << mCurrentQueryTimeAverage <<
            " dev=" << mCurrentQueryTimeDeviation << ") prev(avg="
            << prevAverage << " dev=" << prevDeviation
            << ") newHistorySize=" << mQueryVarianceHistory.size());

    // Reduce number of available connections if squelching is enabled
    if (isSquelchingEnabled())
    {
        uint32_t newMaxConnCount = getSquelchedConnectionCount();
        if (mCurrentMaxConnCount.get() != newMaxConnCount)
        {
            mCurrentMaxConnCount.set(newMaxConnCount);
            metrics.mSquelched = true;
            BLAZE_INFO_LOG(Log::DB, "[DbInstancePool].pushQueryVarianceHistory: "
                    << getDisplayString(displayBuf, sizeof(displayBuf))
                    << ": connection count squelching enabled; reducing "
                    "max connection count to " << mCurrentMaxConnCount.get()
                    << "; configured max " << mDbConfig.getMaxConnCount());
        }
    }
}

bool DbInstancePool::isSquelchingEnabled() const
{
    return (gDbScheduler->getDbSquelchingMinConnectionCountPercent() < 1.0f);
}

void DbInstancePool::getTrimList(eastl::vector<DbConn*>& trimList)
{
    uint32_t idealConnections = (uint32_t)((float)mActivePoolCount.get() * (1 + gDbScheduler->getDbConnectionTrimThreshold()));
    if (idealConnections > (mActivePoolCount.get() + mInactivePoolCount.get()))
        return;
    uint32_t closableConnections = (uint32_t)(mActivePoolCount.get() + mInactivePoolCount.get()) - idealConnections;
    trimList.reserve(closableConnections);

    for(; closableConnections != 0; closableConnections--)
    {
        DbConn* conn = moveConnectionToActive();
        if (conn == nullptr)
            break;
        trimList.push_back(conn);
    }
}

} // Blaze

