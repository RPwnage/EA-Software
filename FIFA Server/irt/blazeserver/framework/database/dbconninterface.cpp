/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/dbconnpool.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/query.h"
#include "framework/connection/selector.h"
#include "framework/system/fiber.h"


namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#if ENABLE_CLANG_SANITIZERS
    const uint32_t DbConnInterface::CONNECT_TIMEOUT = 100;
#else
    const uint32_t DbConnInterface::CONNECT_TIMEOUT = 10; // 10 seconds connection timeout should be long enough to determine if there is any network issue
#endif

/*** Public Methods ******************************************************************************/

DbConnInterface::DbConnInterface(const DbConfig& config, DbConnBase& owner)
    : mOwner(owner),
      mTxnInProgress(false),
      mPrefetchRowCount(UNSET_PREFETCH_ROW_COUNT),
      mPrefetchMemory(UNSET_PREFETCH_MAX_MEMORY),
      mQueryTimedOut(false),
      mStatements(BlazeStlAllocator("DbConnInterface::mStatements")),
      mUseCount(0)

{
}   

DbConnInterface::~DbConnInterface()
{
    StatementsById::iterator itr = mStatements.begin();
    StatementsById::iterator end = mStatements.end();
    for (; itr != end; ++itr)
        delete itr->second;
}

void DbConnInterface::assign()
{
    mUseCount = 0;
    mQuerySetupTimeOnThread = 0;
    mQueryExecutionTimeOnThread = 0;
    mQueryCleanupTimeOnThread = 0;
}

void DbConnInterface::release()
{
    clearPrefetchParameters();

    if (!isTxnInProgress())
    {
        mOwner.getDbConnPool().release(mOwner);
        return;
    }

    // Transaction wasn't completed - possibly due to command time.  Just close this connection
    // to reset the transaction (rather than worrying that subsequent calls to rollback might
    // fail) so that the connection will be in a good state for the next consumer.
    BLAZE_ERR_LOG(Log::DB, "[DbConnInterface].release: conn=" 
        << mOwner.getName() << " was not commited or rolled back before "
        "releasing; closing connection to reset its state.");
        
    // NOTE: disconnect has special handling to release the mOwner dbconn back to the pool
    disconnect(DB_ERR_TRANSACTION_NOT_COMPLETE);
    
    mTxnInProgress = false;
}

PreparedStatement* DbConnInterface::getPreparedStatement(PreparedStatementId id)
{
    StatementsById::iterator find = mStatements.find(id);
    if (find != mStatements.end())
    {
        find->second->reset();
        return find->second;
    }

    // Statement doesn't exist yet so create it
    const PreparedStatementInfo* info = mOwner.getDbConnPool().getStatementInfo(id);
    if (info == nullptr)
        return nullptr;
    PreparedStatement* statement = createPreparedStatement(*info);
    statement->setRegistered(true);
    mStatements[id] = statement;
    return statement;
}

const char8_t* DbConnInterface::getName() const
{
    return mOwner.getName();
}

/*** Protected Methods ***************************************************************************/

void DbConnInterface::closeAllStatements()
{
    StatementsById::iterator itr = mStatements.begin();
    StatementsById::iterator end = mStatements.end();
    for(; itr != end; ++itr)
    {
        itr->second->close(*this);
    }
}

TimeValue DbConnInterface::calculateQueryTimeout(TimeValue timeout)
{
    mQueryTimeout = timeout;
    if (mQueryTimeout == 0)
    {
        if (gIsDbThread)
        {
            mQueryTimeout = gDbThreadTimeout != 0 ? gDbThreadTimeout : TimeValue::getTimeOfDay() + TimeValue(gDbScheduler->getDefaultQueryTimeout());
        }
        else
        {
            mQueryTimeout = Fiber::getCurrentTimeout() != 0 ? 
                Fiber::getCurrentTimeout() :  TimeValue::getTimeOfDay() + TimeValue(gDbScheduler->getDefaultQueryTimeout());
        }
    }
    return mQueryTimeout;
}

/*** Private Methods *****************************************************************************/

void DbConnInterface::setPrefetchParameters(int32_t rowCount, int32_t maxMemory)
{
    mPrefetchRowCount = rowCount;
    mPrefetchMemory = maxMemory;
}

void DbConnInterface::getPrefetchParameters(int32_t& rowCount, int32_t& maxMemory)
{
    rowCount = mPrefetchRowCount;
    maxMemory = mPrefetchMemory;
}

void DbConnInterface::clearPrefetchParameters()
{
    mPrefetchRowCount = UNSET_PREFETCH_ROW_COUNT;
    mPrefetchMemory = UNSET_PREFETCH_MAX_MEMORY;
}

} // namespace Blaze

