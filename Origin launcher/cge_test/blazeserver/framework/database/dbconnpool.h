/*************************************************************************************************/
/*!
    \file   dbconnpool.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_DBCONNPOOL_H
#define BLAZE_DBCONNPOOL_H

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
#include "framework/database/dbinstancepool.h"
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


namespace Metrics
{
    namespace Tag
    {
        extern Metrics::TagInfo<DbRole>* db_role;
        extern Metrics::TagInfo<const char*>* db_hostname;
    }
}

class DbConnPool
{
    NON_COPYABLE(DbConnPool);

public:
    DbConnPool(const char8_t* name, const DbConnConfig& config, uint32_t id);
    ~DbConnPool();

    uint32_t getId() const { return mId; }

    BlazeRpcError DEFINE_ASYNC_RET(getConn(DbConn::Mode mode, int32_t logCagegory, DbConn*& conn));
    
    void release(DbConnBase& connBase); 

    PreparedStatementId registerPreparedStatement(const char8_t* name, const char8_t* query);
    const PreparedStatementInfo* getStatementInfo(PreparedStatementId id) const;

    const DbConfig& getDbConfig() const { return mDbConfig; }

    const char8_t* getName() const { return mDbConfig.getName(); }

    void getStatusInfo(DbConnPoolStatus& status) const;

    const bool isSchemaInfoInitialized() const { return mSchemaMapInitialized; }
    void setSchemaInfoInitialized() { mSchemaMapInitialized = true; }
    void addComponentSchemaRecord(const char8_t* name, const uint32_t version);
    uint32_t getComponentSchemaVersion(const char8_t* name) const;
    void updateComponentSchemaVersion(const char8_t* name, uint32_t version);

    BlazeRpcError initialize();
    BlazeRpcError start();

    BlazeRpcError registerDbThread();
    BlazeRpcError unregisterDbThread();

    uint32_t getSlavePoolCount() const { return mSlavePoolCounter; }
    bool getUseAsyncDbConns() const;

    bool isWriteReplicationSynchronous() const;
    bool getSchemaUpdateBlocksAllTxn() const;

private:
    void lockMutex();
    void unlockMutex();
    friend class DbInstancePool;
    friend class DbScheduler;
    void reconfigure(const DbConnConfig &config);
    void prepareForReconfigure(const DbConnConfig &config);

    DbConn* activateClientConnection(DbConn::Mode mode, bool& squelching);
    void deactivateClientConection(DbConn &conn, bool replace = false) const;

    void trimOpenConnections();
    void trimInstancePool(DbInstancePool& pool);
    void scheduleTrimTimer();
    void scheduleTrimTimerInternal(const EA::TDF::TimeValue& trimPeriod);
    void scheduleQueryStatusTimer();
    void checkQueryStatus();
    EA::TDF::TimeValue getQuerySamplePeriodStart() const { return mQuerySamplePeriodStart; }

    enum DbTechType
    {
        DBTECH_UNKNOWN = 0, // unknown or not determined yet
        DBTECH_MYSQL_MASTER_SLAVE = 1,
        DBTECH_GALERA_CLUSTER = 2,
        DBTECH_AMAZON_AURORA = 3,
    };
    static const char8_t* dbTechToString(DbTechType dbTechType);

    uint32_t getMaxConnCountForMode(DbConn::Mode mode) const;

    typedef eastl::list<HostNameAddressPtr> HostNameAddressList;

    class ConnectionWaiter
    {
    public:
        ConnectionWaiter(DbConn** dbConnPtr, const Fiber::EventHandle& eventHandle)
            : mDbConnPtr(dbConnPtr), mEventHandle(eventHandle) {}

        DbConn** getDbConnPtr() const { return mDbConnPtr; }
        const Fiber::EventHandle& getEventHandle() const { return mEventHandle; }
        bool operator<(const ConnectionWaiter& rhs) const
        {
            return (mEventHandle.priority > rhs.mEventHandle.priority);
        }

    private:
        DbConn** mDbConnPtr;        
        Fiber::EventHandle mEventHandle;
    };

    typedef eastl::intrusive_list<DbInstancePool> SlaveList;

    DbConfig mDbConfig;
    uint32_t mId;
    DbTechType mDbTechType;
    DbInstancePool* mMaster;
    SlaveList mSlaves;
    HostNameAddressList mPrepareReconfigureFailedSlaveList;

    // For thread-safety.
    mutable EA::Thread::Mutex mMutex;

    typedef eastl::hash_map<PreparedStatementId, PreparedStatementInfo*> StatementInfoById;
    StatementInfoById mStatementInfo;
    PreparedStatementId mNextStatementId;

    typedef eastl::priority_queue<ConnectionWaiter, eastl::deque<ConnectionWaiter> > GetConnQueue;
    GetConnQueue mMasterConnectionWaiters;
    GetConnQueue mSlaveConnectionWaiters;

    bool mSchemaMapInitialized;
    typedef eastl::hash_map<const char8_t*, uint32_t, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > SchemaMap;
    SchemaMap mComponentSchemaMap;

    CreateDbAdminFunc mCreateDbAdminFunc;

    TimerId mDbTrimTimerId;
    TimerId mQueryStatusTimerId;
    EA::TDF::TimeValue mQuerySamplePeriodStart;

    int32_t mSlavePoolCounter;

    Metrics::MetricsCollection& mMetricsCollection;
    Metrics::PolledGauge mQueuedFibersMaster;
    Metrics::PolledGauge mQueuedFibersSlaves;
};

} // Blaze

#endif // BLAZE_DBCONNPOOL_H


