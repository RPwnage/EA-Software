/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_THREADEDADMIN_H
#define BLAZE_THREADEDADMIN_H

/*** Include files *******************************************************************************/

#include "framework/system/timerqueue.h"
#include "framework/database/dbadmin.h"
#include "framework/database/dbconn.h"
#include "framework/database/mysql/mysqlconn.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class DbConn;
class DbConfig;
class DbInstancePool;
class DbScheduler;

class ThreadedAdmin : public DbAdmin, private BlazeThread
{
    friend class ThreadedAdminCheckDbStatusJob;

    NON_COPYABLE(ThreadedAdmin);

public:
    ThreadedAdmin(const DbConfig& config, DbInstancePool& owner);

    BlazeRpcError checkDbStatus() override;

    EA::Thread::ThreadId start() override;
    void stop() override;

    TimerId killQuery(const EA::TDF::TimeValue& whenAbsolute, DbConnBase& conn);
    void cancelKill(TimerId id);

private:
    void run() override;

    virtual void killQueryImpl(DbConnBase& conn) = 0;
    virtual BlazeRpcError checkDbStatusImpl() = 0;

private:
    volatile bool mActive;
    DbScheduler* mDbScheduler;
    TimerQueue mQueue;
    EA::Thread::Mutex mMutex;
    EA::Thread::Condition mCondition;
};

} // Blaze

#endif // BLAZE_THREADEDADMIN_H

