/*************************************************************************************************/
/*!
    \file   dbconnpool.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_DBSCHEDULER_H
#define BLAZE_DBSCHEDULER_H

/*** Include files *******************************************************************************/
#include "eathread/eathread_rwmutex.h"
#include "eathread/eathread_storage.h"
#include "framework/system/threadpooljobhandler.h"
#include "framework/system/job.h"
#include "framework/system/fiber.h"
#include "framework/database/preparedstatement.h"
#include "framework/database/dbconn.h"
#include "EATDF/time.h"
#include "EASTL/map.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#ifndef DB_LOGGER_CATEGORY
    #ifdef LOGGER_CATEGORY
        #define DB_LOGGER_CATEGORY LOGGER_CATEGORY
    #else
        #define DB_LOGGER_CATEGORY Log::DB
    #endif
#endif

#define VERIFY_WORKER_FIBER_DB() \
    if (!Fiber::isCurrentlyBlockable()) \
    { \
        THROW_WORKER_FIBER_ASSERT(); \
        return DB_ERR_SYSTEM; \
    }


namespace Blaze
{

class DbConfig;
class DbConnPool;
class Logger;
class Fiber;
class DbConnConfig;
class FrameworkConfig;
class DatabaseStatus;

class DbScheduler : public ThreadPoolJobHandler
{
public:
    static const uint32_t INVALID_DB_ID = 0xFFFFFFFF;    

    DbConnPtr getConnPtr(uint32_t dbId, int32_t logCategory = DB_LOGGER_CATEGORY);
    DbConnPtr getReadConnPtr(uint32_t dbId, int32_t logCategory = DB_LOGGER_CATEGORY);
    DbConnPtr getLagFreeReadConnPtr(uint32_t dbId, int32_t logCategory = DB_LOGGER_CATEGORY);

#ifdef BLAZE_ENABLE_DEPRECATED_DATABASE_INTERFACE
    [[deprecated("use getConnPtr instead")]]
    DbConn* getConn(uint32_t dbId, int32_t logCategory = DB_LOGGER_CATEGORY)
    {
        return getConn(dbId, DbConnBase::READ_WRITE, logCategory, false);
    }

    [[deprecated("use getReadConnPtr instead")]]
    DbConn* getReadConn(uint32_t dbId, int32_t logCategory = DB_LOGGER_CATEGORY)
    {
        return getConn(dbId, DbConnBase::READ_ONLY, logCategory, false);
    }

    [[deprecated("use getLagFreeReadConnPtr instead")]]
    DbConn* getLagFreeReadConn(uint32_t dbId, int32_t logCategory = DB_LOGGER_CATEGORY)
    {
        return getConn(dbId, DbConnBase::READ_ONLY, logCategory, true);
    }
#endif

    uint32_t getDbId(const char8_t *dbName);
    const DbConfig *getConfig(uint32_t dbId);
    PreparedStatementId registerPreparedStatement(
            uint32_t dbId, const char8_t* name, const char8_t* query);

    const DbMigConfig& getDbMigConfig() const { return mDbMigConfig; }

    bool getAllowRequests() const { return mAllowRequests; }
    void stopAllowingRequests() { mAllowRequests = false; }

    void getStatusInfo(DatabaseStatus& status) const;

    uint32_t getComponentVersion(uint32_t db, const char8_t* component);
    void updateComponentVersion(uint32_t db, const char8_t* component, uint32_t newVersion);

    bool getVerbatimQueryStringChecksEnabled() const { return mVerbatimQueryStringChecksEnabled; }

    int64_t getDefaultRuntimeQueryTimeout() const { return mDbSchedulerConfig.getDbRuntimeQueryTimeout().getMicroSeconds(); }

    //The write isn't threadsafe, but this solution as a whole doesn't account
    //for multiple threads utilizing seperate timeouts on the db scheduler.
    void setDefaultQueryTimeout(int64_t timeout) { mDbSchedulerConfig.setDbStartupQueryTimeout(EA::TDF::TimeValue(timeout)); }
    int64_t getDefaultQueryTimeout() const { return mDbSchedulerConfig.getDbStartupQueryTimeout().getMicroSeconds(); }

    // Return the query execution time sampling period in milliseconds.
    EA::TDF::TimeValue getQuerySamplePeriod() const { return mDbSchedulerConfig.getDbQuerySamplePeriod(); }

    float getDbQueryVarianceThreshold() const { return mDbSchedulerConfig.getDbQueryVarianceThreshold(); }
    EA::TDF::TimeValue getDbQueryTimeBaselineDuration() const { return mDbSchedulerConfig.getDbQueryTimeBaselineDuration(); }
    EA::TDF::TimeValue getDbQueryTimeBaselineMinimum() const { return mDbSchedulerConfig.getDbQueryTimeBaselineMinimum(); }
    uint32_t getDbQueryVarianceHistorySize() const { return mDbSchedulerConfig.getDbQueryVarianceHistorySize(); }
    float getDbSquelchingMinConnectionCountPercent() const { return mDbSchedulerConfig.getDbSquelchingMinConnectionCountPercent(); }
    float getDbSquelchingMaxQueryVariance() const { return mDbSchedulerConfig.getDbSquelchingMaxQueryVariance(); }

    EA::TDF::TimeValue getDbConnectionTrimPeriod() const { return mDbSchedulerConfig.getDbConnectionTrimPeriod(); }
    float getDbConnectionTrimThreshold() const { return mDbSchedulerConfig.getDbConnectionTrimThreshold(); }

    void tickQuery(const char8_t* queryName, const EA::TDF::TimeValue& time);
    void tickQuerySuccess(const char8_t* queryName);
    void tickSlowQuery(const char8_t* queryName);
    void tickQueryFailure(const char8_t* queryName, BlazeRpcError err);

    const eastl::string& getBuildPath() const { return mBuildPath; }

    void buildDbQueryAccountingMap(DbQueryAccountingInfos& map);

    bool parseDbNameByPlatformTypeMap(const char8_t* componentName, const DbNameByPlatformTypeMap& dbNameByPlatformTypeMap, DbIdByPlatformTypeMap& dbIdByPlatformTypeMap);

    //DEPRECATED - DO NOT USE
    //These functions previously allowed for functional access to DB threads.  Because of safety reasons, they now simply continue to run on the requested fiber.
    //There is no advantage to using these functions, their use is deprecated and should no longer be used in new code.

#ifdef BLAZE_ENABLE_DEPRECATED_DATABASE_INTERFACE
    //Call schedulers - one each for parameter name/whether DBConn is needed
    template <class Caller> 
    [[deprecated]]
    BlazeRpcError DEFINE_ASYNC_RET(scheduleCall(uint32_t db, Caller *caller, BlazeRpcError (Caller::*mf)())); 

    template <class Caller> 
    [[deprecated]]
    BlazeRpcError DEFINE_ASYNC_RET(scheduleCall(uint32_t db, Caller *caller, BlazeRpcError (Caller::*mf)(DbConn*), int32_t logCategory = DB_LOGGER_CATEGORY)); 

    template <class Caller, typename P1> 
    [[deprecated]]
    BlazeRpcError DEFINE_ASYNC_RET(scheduleCall(uint32_t db, Caller *caller, BlazeRpcError (Caller::*mf)(P1), P1 arg1)); 

    template <class Caller, typename P1> 
    [[deprecated]]
    BlazeRpcError DEFINE_ASYNC_RET(scheduleCall(uint32_t db, Caller *caller, BlazeRpcError (Caller::*mf)(DbConn*, P1), P1 arg1, int32_t logCategory = DB_LOGGER_CATEGORY)); 

    template <class Caller, typename P1, typename P2> 
    [[deprecated]]
    BlazeRpcError DEFINE_ASYNC_RET(scheduleCall(uint32_t db, Caller *caller, BlazeRpcError (Caller::*mf)(P1, P2), P1 arg1, P2 arg2)); 

    template <class Caller, typename P1, typename P2> 
    [[deprecated]]
    BlazeRpcError DEFINE_ASYNC_RET(scheduleCall(uint32_t db, Caller *caller, BlazeRpcError (Caller::*mf)(DbConn*, P1, P2), P1 arg1, P2 arg2, int32_t logCategory = DB_LOGGER_CATEGORY)); 

    template <class Caller, typename P1, typename P2, typename P3> 
    [[deprecated]]
    BlazeRpcError DEFINE_ASYNC_RET(scheduleCall(uint32_t db, Caller *caller, BlazeRpcError (Caller::*mf)(P1, P2, P3), P1 arg1, P2 arg2, P3 arg3)); 

    template <class Caller, typename P1, typename P2, typename P3> 
    [[deprecated]]
    BlazeRpcError DEFINE_ASYNC_RET(scheduleCall(uint32_t db, Caller *caller, BlazeRpcError (Caller::*mf)(DbConn*, P1, P2, P3), P1 arg1, P2 arg2, P3 arg3, int32_t logCategory = DB_LOGGER_CATEGORY)); 

    template <class Caller, typename P1, typename P2, typename P3, typename P4> 
    [[deprecated]]
    BlazeRpcError DEFINE_ASYNC_RET(scheduleCall(uint32_t db, Caller *caller, BlazeRpcError (Caller::*mf)(P1, P2, P3, P4), P1 arg1, P2 arg2, P3 arg3, P4 arg4)); 

    template <class Caller, typename P1, typename P2, typename P3, typename P4> 
    [[deprecated]]
    BlazeRpcError DEFINE_ASYNC_RET(scheduleCall(uint32_t db, Caller *caller, BlazeRpcError (Caller::*mf)(DbConn*, P1, P2, P3, P4), P1 arg1, P2 arg2, P3 arg3, P4 arg4, int32_t logCategory = DB_LOGGER_CATEGORY)); 

    template <class Caller, typename P1, typename P2, typename P3, typename P4, typename P5> 
    [[deprecated]]
    BlazeRpcError DEFINE_ASYNC_RET(scheduleCall(uint32_t db, Caller *caller, BlazeRpcError (Caller::*mf)(P1, P2, P3, P4, P5), P1 arg1, P2 arg2, P3 arg3, P4 arg4, P5 arg5)); 

    template <class Caller, typename P1, typename P2, typename P3, typename P4, typename P5> 
    [[deprecated]]
    BlazeRpcError DEFINE_ASYNC_RET(scheduleCall(uint32_t db, Caller *caller, BlazeRpcError (Caller::*mf)(DbConn*, P1, P2, P3, P4, P5), P1 arg1, P2 arg2, P3 arg3, P4 arg4, P5 arg5, int32_t logCategory = DB_LOGGER_CATEGORY)); 
#endif

private:
    friend class ServerRunnerThread;
    friend class Controller;
    friend class DbConnPool;   
    friend class DbConn;
    friend class ThreadedDbConn;
    friend class DbInstancePool;

    DbScheduler();
    ~DbScheduler() override;

    void start() override;
    void stop() override;

    BlazeRpcError addDbConfig(const char8_t* name, const DbConnConfig& config);

    DbConn* getConn(uint32_t dbId, DbConn::Mode mode, int32_t logCategory, bool forceReadWithoutRepLag);
    BlazeRpcError initFromConfig(const DatabaseConfiguration& config);
    BlazeRpcError initSchemaVersions();
    BlazeRpcError checkDBVersion();
    BlazeRpcError DEFINE_ASYNC_RET(selectComponentSchemaVersions(uint32_t poolId));

    void reconfigure(const DatabaseConfiguration& config);
    void prepareForReconfigure(const DatabaseConfiguration& config);
    void validateConfig(DatabaseConfiguration& config, const DatabaseConfiguration* refConfig, ConfigureValidationErrors& validationErrors);

    /* \brief An internal job used to handled scheduled calls */
    class DbScheduleJob : public Job
    {
    public:
        DbScheduleJob() : mDbConnPool(0), mConnectionNeeded(false), 
                          mDbConnPtr(nullptr), mDbConnMode(DbConn::READ_WRITE), 
                          mLogContext(nullptr), 
                          mTimeout(0), mLogCategory(Log::DB), mReturnValuePtr(nullptr) {}

        /* \brief Sets the connection pool this job runs out of. */
        void setDbConnPool(DbConnPool *pool ) { mDbConnPool = pool; }
        DbConnPool *getDbConnPool() const { return mDbConnPool; }

        /* \brief Sets whether or not this job needs a connection allocated before its run */
        void setConnectionNeeded(bool needed) { mConnectionNeeded = needed; }
        bool isConnectionNeeded() const { return mConnectionNeeded; }

        void setDbConnPtr(DbConn **ptr) { mDbConnPtr = ptr; }
        void setDbConn(DbConn *conn) { *mDbConnPtr = conn; }
        bool hasDbConn() const { return ((mDbConnPtr == nullptr) ? false : (*mDbConnPtr != nullptr)); }

        void setDbConnMode(DbConn::Mode mode) { mDbConnMode = mode; }
        DbConn::Mode getDbConnMode() const { return mDbConnMode; }

        void setEventHandle(const Fiber::EventHandle& id) { mEventHandle = id; }
        const Fiber::EventHandle& getEventHandle() const { return mEventHandle; }

        void setLogContext(const LogContextWrapper& context) { mLogContext = &context; }
        const LogContextWrapper* getLogContext() const { return mLogContext; }
       
        void setTimeout(int64_t timeout) { mTimeout = timeout; }
        int64_t getTimeout() const { return mTimeout; }

        void setLogCategory(int32_t logCategory) { mLogCategory = logCategory; }
        int32_t getLogCategory() const { return mLogCategory; }

        void setReturnValuePtr(uint32_t *ptr) { mReturnValuePtr = nullptr; }
        uint32_t *getReturnValuePtr() const { return mReturnValuePtr; }

    private:
        DbConnPool *mDbConnPool;
        bool mConnectionNeeded;
        DbConn **mDbConnPtr;
        DbConn::Mode mDbConnMode;
        
        Fiber::EventHandle mEventHandle;
        const LogContextWrapper* mLogContext;
        int64_t mTimeout;

        int32_t mLogCategory;
         
        uint32_t *mReturnValuePtr;

        TimerId mExpiryTimerId;
    };

    DbConnPool *getConnPool(uint32_t db);
    BlazeRpcError queueJob(uint32_t id, DbScheduleJob &job, bool block, bool* jobQueued = nullptr);

    void threadStart() override;
    void threadEnd() override;
    void threadReconfigure(int32_t workerId) override;
    void executeJob(Job& job) override;
    uint32_t getDbIdInternal(const char8_t *dbName);

    bool mInitialized;
    bool mStarted;
    bool mSchemaVersionInitialized;

    typedef eastl::map<uint32_t, DbConnPool *> ConnPoolMap;
    ConnPoolMap mConnPoolMap;
    uint32_t mNextIdentifier;

    DatabaseConfiguration::DbAliasMap mConnPoolAliasMap;
    
    mutable EA::Thread::RWMutex mMutex;

    bool mVerbatimQueryStringChecksEnabled;

    DbMigConfig mDbMigConfig;
    DbSchedulerConfig mDbSchedulerConfig;
    DbSchedulerConfig mTempDbSchedulerConfig;

    bool mAllowRequests;

    struct DbMetrics
    {
        DbMetrics();
        void buildDbQueryAccountingMap(DbQueryAccountingInfos& map);

        Metrics::TaggedTimer<Metrics::DbQueryName, Metrics::FiberContext> mTimer;
        Metrics::TaggedCounter<Metrics::DbQueryName, Metrics::FiberContext> mSuccessCount;
        Metrics::TaggedCounter<Metrics::DbQueryName, Metrics::FiberContext> mSlowQueryCount;
        Metrics::TaggedCounter<Metrics::DbQueryName, Metrics::FiberContext, BlazeRpcError> mFailCount;

        Metrics::PolledGauge mTotalWorkerThreads;
        Metrics::PolledGauge mActiveWorkerThreads;
        Metrics::PolledGauge mPeakWorkerThreads;
    };
    DbMetrics mDbMetrics;
    eastl::string mBuildPath;
};

extern EA_THREAD_LOCAL DbScheduler *gDbScheduler;
extern EA_THREAD_LOCAL bool gIsDbThread;
extern EA_THREAD_LOCAL uint64_t gDbThreadTimeout;

//DEPRECATED - These calls are deprecated and now simply just make the requested call immediately.
#ifdef BLAZE_ENABLE_DEPRECATED_DATABASE_INTERFACE
template <class Caller>
BlazeRpcError DbScheduler::scheduleCall(uint32_t db, Caller *caller, BlazeRpcError (Caller::*mf)())
{
    VERIFY_WORKER_FIBER();
    return (caller->*mf)();
}

template <class Caller>
BlazeRpcError DbScheduler::scheduleCall(uint32_t db, Caller *caller, BlazeRpcError (Caller::*mf)(DbConn*), int32_t logCategory)
{
    VERIFY_WORKER_FIBER();
    return (caller->*mf)(gDbScheduler->getConn(db, DbConn::READ_WRITE, logCategory, false));
}

template <class Caller, typename P1>
BlazeRpcError DbScheduler::scheduleCall(uint32_t db, Caller *caller, BlazeRpcError (Caller::*mf)(P1), P1 arg1)
{
    VERIFY_WORKER_FIBER();
    return (caller->*mf)(arg1);
}

template <class Caller, typename P1>
BlazeRpcError DbScheduler::scheduleCall(uint32_t db, Caller *caller, BlazeRpcError (Caller::*mf)(DbConn*, P1), P1 arg1, int32_t logCategory)
{
    VERIFY_WORKER_FIBER();
    return (caller->*mf)(gDbScheduler->getConn(db, DbConn::READ_WRITE, logCategory, false), arg1);
}

template <class Caller, typename P1, typename P2>
BlazeRpcError DbScheduler::scheduleCall(uint32_t db, Caller *caller, BlazeRpcError (Caller::*mf)(P1, P2), P1 arg1, P2 arg2)
{
    VERIFY_WORKER_FIBER();
    return (caller->*mf)(arg1, arg2);
}

template <class Caller, typename P1, typename P2>
BlazeRpcError DbScheduler::scheduleCall(uint32_t db, Caller *caller, BlazeRpcError (Caller::*mf)(DbConn*, P1, P2), P1 arg1, P2 arg2, int32_t logCategory)
{
    VERIFY_WORKER_FIBER();
    return (caller->*mf)(gDbScheduler->getConn(db, DbConn::READ_WRITE, logCategory, false), arg1, arg2);
}


template <class Caller, typename P1, typename P2, typename P3>
BlazeRpcError DbScheduler::scheduleCall(uint32_t db, Caller *caller, 
                               BlazeRpcError (Caller::*mf)(P1, P2, P3), P1 arg1, P2 arg2, P3 arg3)
{
    VERIFY_WORKER_FIBER();
    return (caller->*mf)(arg1, arg2, arg3);
}

template <class Caller, typename P1, typename P2, typename P3>
BlazeRpcError DbScheduler::scheduleCall(uint32_t db, Caller *caller, 
                               BlazeRpcError (Caller::*mf)(DbConn*, P1, P2, P3), P1 arg1, P2 arg2, P3 arg3, int32_t logCategory)
{
    VERIFY_WORKER_FIBER();
    return (caller->*mf)(gDbScheduler->getConn(db, DbConn::READ_WRITE, logCategory, false), arg1, arg2, arg3);
}

template <class Caller, typename P1, typename P2, typename P3, typename P4>
BlazeRpcError DbScheduler::scheduleCall(uint32_t db, Caller *caller, 
                               BlazeRpcError (Caller::*mf)(P1, P2, P3, P4), P1 arg1, P2 arg2, P3 arg3, P4 arg4)
{
    VERIFY_WORKER_FIBER();
    return (caller->*mf)(arg1, arg2, arg3, arg4);
}

template <class Caller, typename P1, typename P2, typename P3, typename P4>
BlazeRpcError DbScheduler::scheduleCall(uint32_t db, Caller *caller, 
                               BlazeRpcError (Caller::*mf)(DbConn*, P1, P2, P3, P4), P1 arg1, P2 arg2, P3 arg3, P4 arg4, int32_t logCategory)
{
    VERIFY_WORKER_FIBER();
    return (caller->*mf)(gDbScheduler->getConn(db, DbConn::READ_WRITE, logCategory, false), arg1, arg2, arg3, arg4);
}

template <class Caller, typename P1, typename P2, typename P3, typename P4, typename P5>
BlazeRpcError DbScheduler::scheduleCall(uint32_t db, Caller *caller, 
                               BlazeRpcError (Caller::*mf)(P1, P2, P3, P4, P5), P1 arg1, P2 arg2, P3 arg3, P4 arg4, P5 arg5)
{
    VERIFY_WORKER_FIBER();
    return (caller->*mf)(arg1, arg2, arg3, arg4, arg5);
}

template <class Caller, typename P1, typename P2, typename P3, typename P4, typename P5>
BlazeRpcError DbScheduler::scheduleCall(uint32_t db, Caller *caller, 
                               BlazeRpcError (Caller::*mf)(DbConn*, P1, P2, P3, P4, P5), P1 arg1, P2 arg2, P3 arg3, P4 arg4, P5 arg5, int32_t logCategory)
{
    VERIFY_WORKER_FIBER();
    return (caller->*mf)(gDbScheduler->getConn(db, DbConn::READ_WRITE, logCategory, false), arg1, arg2, arg3, arg4, arg5);
}

#endif //BLAZE_ENABLE_DEPRECATED_DATABASE_INTERFACE

} // Blaze

#endif // BLAZE_DBSCHEDULER_H


