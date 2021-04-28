/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_MYSQLASYNCADMIN_H
#define BLAZE_MYSQLASYNCADMIN_H

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

class MySqlAsyncAdmin : public DbAdmin
{
    NON_COPYABLE(MySqlAsyncAdmin);

public:
    MySqlAsyncAdmin(const DbConfig& config, DbInstancePool& owner);
    ~MySqlAsyncAdmin() override;

    BlazeRpcError checkDbStatus() override;
    DbConnInterface* createDbConn(const DbConfig& config, DbConnBase& owner) override;

    EA::Thread::ThreadId start() override { return EA::Thread::kThreadIdInvalid; }
    void stop() override;
    BlazeRpcError registerDbThread() override { return ERR_SYSTEM; }
    BlazeRpcError unregisterDbThread() override { return ERR_SYSTEM; }

    void killConnection(uint64_t threadid);

private:
    FiberJobQueue mKillConnectionJobQueue;
    void killConnectionImpl(uint64_t threadid);

    DbConn* mConn;
    DbConn* mPingConn;
};

} // Blaze

#endif // BLAZE_MYSQLASYNCADMIN_H

