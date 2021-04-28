/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class MySqlAsyncAdmin

    This class provides an admin interface to the MySql Async driver.  It exists to provide administration
    type operations such as killing long running queries.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/system/job.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconnpool.h"
#include "framework/database/dbadmin.h"
#include "framework/database/mysql/mysqladmin.h"
#include "framework/database/mysql/mysqlasyncadmin.h"
#include "framework/database/mysql/mysqlconn.h"
#include "framework/database/mysql/mysqlasyncconn.h"

namespace Blaze
{

MySqlAsyncAdmin::MySqlAsyncAdmin(const DbConfig& config, DbInstancePool& owner) :
    mKillConnectionJobQueue("MySqlAsyncAdmin::mKillConnectionJobQueue")
{
    MySqlAdmin::create(false);

    mConn = BLAZE_NEW DbConn(config, *this, owner, DbConnBase::DB_ADMIN_CONN_ID, DbConnBase::READ_WRITE);
    mPingConn = BLAZE_NEW DbConn(config, *this, owner, DbConnBase::DB_ADMIN_CONN_ID, DbConnBase::READ_ONLY);
}

MySqlAsyncAdmin::~MySqlAsyncAdmin()
{
    delete mConn;
    delete mPingConn;

    MySqlAdmin::destroy();
}

void MySqlAsyncAdmin::stop()
{
    mKillConnectionJobQueue.cancelAllWork();
    mConn->disconnect(ERR_OK);
    mPingConn->disconnect(ERR_OK);
}

DbConnInterface* MySqlAsyncAdmin::createDbConn(const DbConfig& config, DbConnBase& owner)
{
    return BLAZE_NEW MySqlAsyncConn(config, owner);
}

BlazeRpcError MySqlAsyncAdmin::checkDbStatus()
{
    return static_cast<MySqlAsyncConn&>(mPingConn->getDbConnInterface()).ping();
}

void MySqlAsyncAdmin::killConnection(uint64_t threadid)
{
    mKillConnectionJobQueue.queueFiberJob(this, &MySqlAsyncAdmin::killConnectionImpl, threadid, "MySqlAsyncAdmin::killConnection");
}

void MySqlAsyncAdmin::killConnectionImpl(uint64_t threadid)
{
    MySqlAsyncConn& adminConn = static_cast<MySqlAsyncConn&>(mConn->getDbConnInterface());
    
    BlazeError rc = adminConn.kill(threadid);
    if (rc == ERR_OK || rc == DB_ERR_NO_SUCH_THREAD)
    {
        BLAZE_TRACE_LOG(Log::DB, "[MySqlAsyncAdmin].killConnectionImpl: " << (rc == ERR_OK ? "Killed" : "No such") << " thread_id(" << threadid << ") on host = " << adminConn.mHostAddress);
    }
    else
    {
        BLAZE_ERR_LOG(Log::DB, "[MySqlAsyncAdmin].killConnectionImpl: Failed to kill thread_id(" << threadid << ") on host = " << adminConn.mHostAddress << ", rc=" << rc);
    }
}

} // Blaze

