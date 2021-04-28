/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_MYSQLADMIN_H
#define BLAZE_MYSQLADMIN_H

/*** Include files *******************************************************************************/

#include "framework/system/timerqueue.h"
#include "framework/database/dbadmin.h"
#include "framework/database/dbconn.h"
#include "framework/database/threadedadmin.h"
#include "framework/database/mysql/mysqlconn.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class DbConfig;
class DbInstancePool;
class DbScheduler;

class MySqlAdmin : public ThreadedAdmin
{
    NON_COPYABLE(MySqlAdmin);

public:
    MySqlAdmin(const DbConfig& config, DbInstancePool& owner);
    ~MySqlAdmin() override;

    DbConnInterface* createDbConn(const DbConfig& config, DbConnBase& owner) override;

    BlazeRpcError registerDbThread() override;
    BlazeRpcError unregisterDbThread() override;

private:
    void killQueryImpl(DbConnBase& conn) override;
    BlazeRpcError checkDbStatusImpl() override;

private:
    friend class MySqlAsyncAdmin;

    DbConn* mConn;

    static bool create(bool usingThreads);
    static void destroy();

    static int32_t mDriverInitializedCount;
    static EA::Thread::Mutex mDriverMutex;
};

extern DbAdmin* MYSQL_createDbAdmin(const DbConfig& config, DbInstancePool& owner);

} // Blaze

#endif // BLAZE_MYSQLADMIN_H

