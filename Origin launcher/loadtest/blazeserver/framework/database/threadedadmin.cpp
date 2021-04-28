/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/system/job.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconnpool.h"
#include "framework/database/threadedadmin.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

ThreadedAdmin::ThreadedAdmin(const DbConfig& config, DbInstancePool& owner)
    : BlazeThread("ThreadedAdminThread", BlazeThread::DB_ADMIN, 256 * 1024),
      mActive(false),
      mDbScheduler(gDbScheduler),
      mQueue("DbAdminTimerQueue")
{
}

EA::Thread::ThreadId ThreadedAdmin::start()
{
    mMutex.Lock();
    mActive = true;
    mMutex.Unlock();
    
    return BlazeThread::start();
}

void ThreadedAdmin::stop()
{
    mMutex.Lock();
    if (mActive)
    {
        mActive = false;
        mMutex.Unlock();

        mCondition.Signal(true);
        waitForEnd();
    }
    else
    {
        mMutex.Unlock();
    }
}

TimerId ThreadedAdmin::killQuery(const TimeValue& whenAbsolute, DbConnBase& conn)
{
    MethodCall1Job<ThreadedAdmin, DbConnBase&>* job
        = BLAZE_NEW MethodCall1Job<ThreadedAdmin, DbConnBase&>(this, &ThreadedAdmin::killQueryImpl, conn);

    mMutex.Lock();
    Fiber::CreateParams params;
    params.namedContext = "ThreadedAdmin::killQuery";
    TimerId timerId = mQueue.schedule(whenAbsolute, job, nullptr, params);
    mMutex.Unlock();

    if (timerId == INVALID_TIMER_ID)
    {
        delete job;
    }

    mCondition.Signal(false);
    return timerId;
}

void ThreadedAdmin::cancelKill(TimerId id)
{
    mMutex.Lock();
    mQueue.cancel(id);
    mMutex.Unlock();
}

class ThreadedAdminCheckDbStatusJob : public Job
{
public:
    ThreadedAdminCheckDbStatusJob(ThreadedAdmin* admin)
        : mAdmin(admin), 
        mEventHandle(Fiber::getNextEventHandle())
    {
    }

    const Fiber::EventHandle& getEventHandle() const { return mEventHandle; }

    void execute() override
    {
        BlazeRpcError err = mAdmin->checkDbStatusImpl();
        Fiber::signal(mEventHandle, err);
    }

private:
    ThreadedAdmin* mAdmin;
    Fiber::EventHandle mEventHandle;
};

BlazeRpcError ThreadedAdmin::checkDbStatus()
{
    ThreadedAdminCheckDbStatusJob* job = BLAZE_NEW ThreadedAdminCheckDbStatusJob(this);
    Fiber::EventHandle ev = job->getEventHandle();

    mMutex.Lock();
    Fiber::CreateParams params;
    params.namedContext = "ThreadedAdmin::checkDbStatus";
    TimerId timerId = mQueue.schedule(0, job, nullptr, params);
    mMutex.Unlock();

    if (timerId == INVALID_TIMER_ID)
    {
        delete job;
    }

    mCondition.Signal(false);
    
    return Fiber::wait(ev, "ThreadedAdminCheck::checkDbStatus");
}

/*** Private Methods *****************************************************************************/

void ThreadedAdmin::run()
{
    registerDbThread();

    gDbScheduler = mDbScheduler;
    gIsDbThread = true;

    while (true)
    {
        TimeValue currentTime = TimeValue::getTimeOfDay();
        TimerNode* node;
        do
        {
            // Pop a node off the timer queue and process it outside of the mutex lock to prevent
            // contention on other threads for queueing.
            mMutex.Lock();
            node = mQueue.popExpiredNode(currentTime);
            mMutex.Unlock();
            if (node != nullptr)
            {
                //Process the node outside of the mutex to prevent blocking other threads from adding timers.
                mQueue.processNode(*node);

                //Clean up the node now that we're done with it.
                mMutex.Lock();
                mQueue.cleanupNode(*node);
                mMutex.Unlock();
            }
        } while (mActive && (node != nullptr));

        currentTime = TimeValue::getTimeOfDay();
        
        mMutex.Lock();
        if (!mActive)
        {
            mMutex.Unlock();
            break;
        }

        TimeValue nextExpiry = mQueue.getNextExpiry();
        int64_t time;
        if (nextExpiry < 0)
            mCondition.Wait(&mMutex, EA::Thread::kTimeoutNone);
        else if ((time = nextExpiry.getMillis() - currentTime.getMillis()) > 0)
            mCondition.Wait(&mMutex, EA::Thread::GetThreadTime() + EA::Thread::ThreadTime(time));
        mMutex.Unlock();
    }

    unregisterDbThread();
}

} // Blaze

