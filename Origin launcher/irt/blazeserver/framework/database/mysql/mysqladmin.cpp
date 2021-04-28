/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class MySqlAdmin

    This class provides an admin interface to the MySql driver.  It exists to provide administration
    type operations such as killing long running queries.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/system/job.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconnpool.h"
#include "framework/database/mysql/mysqladmin.h"
#include "framework/database/mysql/mysqlconn.h"
#include "framework/database/mysql/mysqlasyncconn.h"
#include "framework/database/mysql/mysqlasyncadmin.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

int32_t MySqlAdmin::mDriverInitializedCount = 0;
EA::Thread::Mutex MySqlAdmin::mDriverMutex;


/*** Public Methods ******************************************************************************/

MySqlAdmin::MySqlAdmin(const DbConfig& config, DbInstancePool& owner)
    : ThreadedAdmin(config, owner)
{
    MySqlAdmin::create(true);

    mConn = BLAZE_NEW DbConn(config, *this, owner, DbConnBase::DB_ADMIN_CONN_ID, DbConnBase::READ_WRITE);
}

MySqlAdmin::~MySqlAdmin()
{
    delete mConn;

    MySqlAdmin::destroy();
}

DbConnInterface* MySqlAdmin::createDbConn(const DbConfig& config, DbConnBase& owner)
{
    return BLAZE_NEW MySqlConn(config, owner);
}

BlazeRpcError MySqlAdmin::registerDbThread()
{
    if (mysql_thread_init() == 0) // 0 means all is well.
        return DB_ERR_OK;

    BLAZE_ERR_LOG(Log::DB, "[MySqlAdmin].registerDbThread: mysql_thread_init failed; admin conn: " << mConn->getName());
    return DB_ERR_SYSTEM;
}

BlazeRpcError MySqlAdmin::unregisterDbThread()
{
    mysql_thread_end();
    return DB_ERR_OK;
}


/*** Private Methods *****************************************************************************/

BlazeRpcError MySqlAdmin::checkDbStatusImpl()
{
    return static_cast<MySqlConn&>(mConn->getDbConnInterface()).ping();
}

void MySqlAdmin::killQueryImpl(DbConnBase& targetConn)
{
    MySqlConn& targetConnMySql = static_cast<MySqlConn&>(targetConn.getDbConnInterface());

    char8_t query[256];
    MYSQL* targetMySql = targetConnMySql.getMySql();
    blaze_snzprintf(query, sizeof(query), "KILL %" PRIu64 ";", static_cast<uint64_t>(mysql_thread_id(targetMySql)));
    targetConnMySql.timeoutQuery();
    mConn->assign();
    MySqlConn& adminConnMySql = static_cast<MySqlConn&>(mConn->getDbConnInterface());
    BlazeRpcError rc = ERR_OK;
    if (!adminConnMySql.isConnected())
    {
        rc = adminConnMySql.connectImpl();
    }
    if (rc == ERR_OK)
    {
        adminConnMySql.executeQueryImpl(query, nullptr);
    }
}

// static
bool MySqlAdmin::create(bool usingThreads)
{
    mDriverMutex.Lock();
    if (mDriverInitializedCount++ == 0)
    {
        mysql_library_init(-1, nullptr, nullptr);

        // NOTE: Currently mysql_library_end() is *not* guaranteed to be called
        // from the same thread that called mysql_library_init(). This means that
        // a TLS block allocated by mysql_library_init() could go unfreed, and would
        // therefore cause memory checking tool warnings and also cause the mysql_library_end()
        // call to 'stall' for 5 seconds in the vain hope of recovering the TLS block.
        // To avoid both of those issues, we free the TLS block associated with
        // this thread immediately. If we ever change our hamhanded library
        // initialization code to call mysql_library_init/mysql_library_end on the
        // same thread, calling mysql_thread_end() here will become unnecessary.
        if (usingThreads)
            mysql_thread_end();
    }
    mDriverMutex.Unlock();
    return true;
}

// static
void MySqlAdmin::destroy()
{
    mDriverMutex.Lock();
    if (mDriverInitializedCount > 0 && --mDriverInitializedCount == 0)
    {
        mysql_library_end();
    }
    mDriverMutex.Unlock();
}

DbAdmin* MYSQL_createDbAdmin(const DbConfig& config, DbInstancePool& owner)
{
    if (config.getDbConnConfig().getAsyncDbConns())
        return BLAZE_NEW MySqlAsyncAdmin(config, owner);

    return BLAZE_NEW MySqlAdmin(config, owner);
}

} // Blaze

