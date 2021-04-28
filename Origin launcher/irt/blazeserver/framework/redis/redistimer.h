/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REDIS_TIMER_H
#define BLAZE_REDIS_TIMER_H

/*** Include files *******************************************************************************/

#include "EASTL/utility.h"
#include "framework/redis/rediskeyfieldmap.h"
#include "framework/redis/redisuniqueidmanager.h"
#include "framework/tdf/frameworkredistypes_server.h"
#include "framework/callback.h"
#include "framework/controller/controller.h"
#include "framework/redis/redismanager.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


namespace Blaze
{

/**
 *  /class RedisTime
 *  This class implements a generic type safe interface for scheduling timers backed by Redis.
 *  The timer parameter can be any type compatible with RedisDecoder.
 *  
 *  /examples See testRedisTimer() @ redisunittests.cpp
*/
template<typename P>
class RedisTimer
{
public:
    typedef P TimeoutParam;
    typedef Functor1<TimeoutParam> TimerCb;
    
    RedisTimer(const RedisId& id, bool sharded = true);
    ~RedisTimer();
    
    bool registerHandler(const TimerCb& cb);
    bool deregisterHandler();
    bool syncTimers();

    bool isRegistered() const { return mTimerFiberGroupId != Fiber::INVALID_FIBER_GROUP_ID; }

    RedisTimerId scheduleTimer(const EA::TDF::TimeValue& timeout, const TimeoutParam& param);
    bool cancelTimer(RedisTimerId redisTimerId);

private:
    void handleTimer(TimeoutParam param, RedisTimerId redisTimerId);

private:
    typedef eastl::pair<eastl::string, InstanceId> TimerKey;
    typedef eastl::pair<int64_t, TimeoutParam> TimerEntry;
    typedef eastl::hash_map<RedisTimerId, TimerId> TimerIdMap;
    struct GetInstanceIdFromKey
    {
        InstanceId operator()(const TimerKey& key) { return key.second; }
    };
    typedef RedisKeyFieldMap<TimerKey, RedisTimerId, TimerEntry, GetInstanceIdFromKey> TimerKeyFieldMap;
    TimerCb mTimerCallback;
    TimerKeyFieldMap mTimerKeyFieldMap;
    eastl::string mTimerContext;
    TimerIdMap mTimerIdMap;
    Fiber::FiberGroupId mTimerFiberGroupId;
};


template<typename P>
RedisTimer<P>::RedisTimer(const RedisId& id, bool sharded) :
    mTimerKeyFieldMap(id, sharded),
    mTimerFiberGroupId(Fiber::INVALID_FIBER_GROUP_ID)
{
    mTimerContext.sprintf("%s.%s", id.getNamespace(), id.getName());
}

template<typename P>
RedisTimer<P>::~RedisTimer()
{
    // We require that timers be deregistered at component shutdown because we don't want the component handling any
    // timers after it has completed its shutdown logic possibly tearing down required state.
    EA_ASSERT_MSG(!mTimerKeyFieldMap.isRegistered(), "RedisTimer *must* be deregistered in Component::onShutdown");
}

template<typename P>
RedisTimerId RedisTimer<P>::scheduleTimer(const EA::TDF::TimeValue& timeout, const TimeoutParam& value)
{
    if (!mTimerKeyFieldMap.isRegistered())
        return INVALID_REDIS_TIMER_ID;

    RedisTimerId redisTimerId = gRedisManager->getUniqueIdManager().getNextUniqueId(mTimerKeyFieldMap.getCollectionId());

    if (redisTimerId == INVALID_REDIS_TIMER_ID)
        return INVALID_REDIS_TIMER_ID; // REDIS_TODO: log error, unusual scenario

    InstanceId instanceId = gController->getInstanceId();
    // make redis timer id into a sharded value (needed only to enable canceling a timer remotely from an instance that did not schedule it)
    redisTimerId = BuildInstanceKey64(instanceId, redisTimerId);

    int64_t result = 0;
    TimerEntry entry(timeout.getMicroSeconds(), value);
    TimerKey timerKey(mTimerContext, instanceId);
    RedisError rc = mTimerKeyFieldMap.insert(timerKey, redisTimerId, entry, &result);
    if (rc == REDIS_ERR_OK && result == 1)
    {
        TimerIdMap::insert_return_type ret = mTimerIdMap.insert(redisTimerId);

        // NOTE: scheduleFiberTimerCall() is *non-blocking* and does not allow the timer to fire before the next selector sleep, this means
        // it is valid to update the local data structure after we've scheduled the timer.
        Fiber::CreateParams fparams;
        fparams.groupId = mTimerFiberGroupId;
        ret.first->second = gSelector->scheduleFiberTimerCall(timeout, this, &RedisTimer<P>::handleTimer, value, redisTimerId, mTimerContext.c_str(), fparams);
        if (ret.first->second == INVALID_TIMER_ID)
        {
            // Failure, cleanup timers

            mTimerIdMap.erase(ret.first);

            rc = mTimerKeyFieldMap.erase(timerKey, redisTimerId);
            if (rc != REDIS_ERR_OK)
            {
                BLAZE_ASSERT_LOG(Log::REDIS, "RedisTimer.scheduleTimer: Failed to erase the redisTimerId(" << redisTimerId << "), with err(" << RedisErrorHelper::getName(rc) << ")");
            }
            redisTimerId = INVALID_REDIS_TIMER_ID;
        }
    }

    return redisTimerId;
}

template<typename P>
bool RedisTimer<P>::cancelTimer(RedisTimerId redisTimerId)
{
    if (!mTimerKeyFieldMap.isRegistered())
        return false;

    InstanceId instanceId = GetInstanceIdFromInstanceKey64(redisTimerId);
    // FUTURE: RedisTimer is currently non elastic(timers created on one instance cannot be taken over by others, only canceled by them).
    // This feature was needed for game reporting ZDT 1.0, but in the future redis timers can go away entirely, because game reporting
    // should be modified to support elastic scale up/scale down(ZDT 2.0) by being converted to the StorageManager.
    bool isLocalTimer = (instanceId == gController->getInstanceId());
    if (isLocalTimer)
    {
        TimerIdMap::const_iterator it = mTimerIdMap.find(redisTimerId);
        if (it == mTimerIdMap.end())
            return true; // timer already canceled, return true to enable idempotent behavior for callers of cancelTimer() 

        // NOTE: It's important to remove the timer from the local data structure immediately, since
        // the system timer could fire while we are waiting on redis, and we want to bypass the timer handler in that case.
        TimerId timerId = it->second;
        mTimerIdMap.erase(it);

        if (!gSelector->cancelTimer(timerId))
        {
            BLAZE_ASSERT_LOG(Log::REDIS, "RedisTimer.cancelTimer: gSelector->cancelTimer() failed to cancel timerId(" << timerId << ")");
            return false;
        }
    }

    // if isLocalTimer == false this deletes a timer scheduled by a foreign instance.
    // IMPORTANT: Users of timers that rely on rapid local reschedule and remote cancelation could potentially cause unbounded
    // memory growth on the schedule side. This is because the remote cancelation does not notify the schedule side
    // of the cancelation, for performance reasons. Instead the schedule side will expire the timer as usual, but then
    // NOOP the callback since it fill determine that the timer has already been canceled.
    TimerKey timerKey(mTimerContext, instanceId);
    RedisError rc = mTimerKeyFieldMap.erase(timerKey, redisTimerId);
    if (rc != REDIS_ERR_OK)
    {
        BLAZE_ASSERT_LOG(Log::REDIS, "RedisTimer.cancelTimer: Failed to erase the redisTimerId(" << redisTimerId << ")");
        return false;
    }

    return true;
}

template<typename P>
bool RedisTimer<P>::registerHandler(const TimerCb& cb)
{
    EA_ASSERT(!mTimerCallback.isValid() && cb.isValid());
    if (!mTimerKeyFieldMap.registerCollection())
        return false;
    if (!gRedisManager->getUniqueIdManager().registerUniqueId(mTimerKeyFieldMap.getCollectionId()))
        return false;
    // setup the handler first so that events that expire can be signalled immediately
    mTimerCallback = cb;
    mTimerFiberGroupId = Fiber::allocateFiberGroupId();
    return true;
}

template<typename P>
bool RedisTimer<P>::syncTimers()
{
    EA_ASSERT(mTimerCallback.isValid());

    InstanceId instanceId = gController->getInstanceId();
    TimerKey timerKey(mTimerContext, instanceId);
    RedisError rc;
    RedisResponsePtr scanResp;
    const char8_t* scanCursor = "0"; // cursor returned by SCAN is always a string-formatted int
    const uint32_t pageSizeHint = 1000;
    do
    {
        rc = mTimerKeyFieldMap.scan(timerKey, scanResp, pageSizeHint);
        if (rc != REDIS_ERR_OK)
        {
            BLAZE_ASSERT_LOG(Log::REDIS, "RedisTimer.syncTimers: Failed to scan redis for timers");
            return false;
        }

        scanCursor = scanResp->getArrayElement(REDIS_SCAN_RESULT_CURSOR).getString();
        const RedisResponse& scanArray = scanResp->getArrayElement(REDIS_SCAN_RESULT_ARRAY);
        const EA::TDF::TimeValue now = EA::TDF::TimeValue::getTimeOfDay();
        for (uint32_t vidx = 1, len = scanArray.getArraySize(); vidx < len; vidx += 2)
        {
            bool ok;
            RedisTimerId redisTimerId;
            ok = mTimerKeyFieldMap.extractFieldArrayElement(scanArray, vidx-1, redisTimerId);
            if (!ok)
            {
                BLAZE_ASSERT_LOG(Log::REDIS, "RedisTimer.syncTimers: Could not extract a field array element at index " << vidx-1);
                return false;
            }
            TimerEntry timerEntry;
            ok = mTimerKeyFieldMap.extractValueArrayElement(scanArray, vidx, timerEntry);
            if (!ok)
            {
                BLAZE_ASSERT_LOG(Log::REDIS, "RedisTimer.syncTimers: Could not extract a value array element at index " << vidx);
                return false;
            }

            if (timerEntry.first > now)
            {
                // reschedule the timer
                TimerIdMap::insert_return_type ret = mTimerIdMap.insert(redisTimerId);

                // NOTE: scheduleFiberTimerCall() is *non-blocking* and does not allow the timer to fire before the next selector sleep, this means
                // it is valid to update the local data structure after we've scheduled the timer.
                Fiber::CreateParams fparams;
                fparams.groupId = mTimerFiberGroupId;
                ret.first->second = gSelector->scheduleFiberTimerCall(timerEntry.first, this, &RedisTimer<P>::handleTimer, timerEntry.second, redisTimerId, mTimerContext.c_str(), fparams);
                if (ret.first->second == INVALID_TIMER_ID)
                {
                    // REDIS_TODO: LOG ERROR
                    mTimerIdMap.erase(ret.first);
                }
            }
            else
            {
                // first we erase the timer from Redis, ensuring it's atomicity
                int64_t result = 0;
                rc = mTimerKeyFieldMap.erase(timerKey, redisTimerId, &result);
                if (rc == REDIS_ERR_OK && result == 1)
                {
                    mTimerCallback(timerEntry.second);
                }
                else
                    ; // REDIS_TODO: LOG ERROR
            }


        }
    }
    while (strcmp(scanCursor, "0") != 0);

    return rc == REDIS_ERR_OK;
}

template<typename P>
bool RedisTimer<P>::deregisterHandler()
{
    // NOTE: This will only succeed once, is idempotent
    if (mTimerKeyFieldMap.deregisterCollection())
    {
        gRedisManager->getUniqueIdManager().deregisterUniqueId(mTimerKeyFieldMap.getCollectionId());

        // cancel all pending timers while avoiding removal from redis, 
        // since the handler has been detached so there is nothing to signal
        while (!mTimerIdMap.empty())
        {
            TimerIdMap::iterator it = mTimerIdMap.begin();
            TimerId tid = it->second;
            mTimerIdMap.erase(it);
            gSelector->cancelTimer(tid);
        }

        // Wait for any *currently executing* RedisTimer::handleTimer() fibers to finish.
        Fiber::join(mTimerFiberGroupId);
        mTimerFiberGroupId = Fiber::INVALID_FIBER_GROUP_ID;

        return true;
    }
    return false;
}

template<typename P>
void RedisTimer<P>::handleTimer(TimeoutParam param, RedisTimerId redisTimerId)
{
    TimerIdMap::const_iterator it = mTimerIdMap.find(redisTimerId);
    if (it != mTimerIdMap.end())
    {
        mTimerIdMap.erase(it);
        int64_t result = 0;
        TimerKey timerKey(mTimerContext, GetInstanceIdFromInstanceKey64(redisTimerId));
        RedisError rc = mTimerKeyFieldMap.erase(timerKey, redisTimerId, &result);  // erase first, checking result to ensure timeout is atomic
        if (rc == REDIS_ERR_OK)
        {
            if (result == 1)
                mTimerCallback(param);
            // else if (result == 0)
            //  NO-OP --> timer has already been canceled by a remote instance(e.g: GameReporting use case)
        }
        else
        {
            BLAZE_ASSERT_LOG(Log::REDIS, "RedisTimer.handleTimer: Could not erase redisTimerId(" << redisTimerId << "), with err(" << RedisErrorHelper::getName(rc) << ")");
        }
    }
    else
    {
        BLAZE_ASSERT_LOG(Log::REDIS,"RedisTimer.handleTimer: did not find timer redisTimerId (" << redisTimerId << ")");
    }
}



} // Blaze

#endif // BLAZE_REDIS_TIMER_H

