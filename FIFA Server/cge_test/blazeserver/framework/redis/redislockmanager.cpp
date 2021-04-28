/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/selector.h"
#include "framework/redis/redis.h"
#include "framework/redis/redislockmanager.h"
#include "framework/system/fibermanager.h"
#include "framework/controller/controller.h"
#include "framework/redis/redismanager.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

EA_THREAD_LOCAL RedisLockId RedisLock::mLockIdCounter = RedisLock::INVALID_LOCK_ID;

RedisLockId RedisLock::getNextLockId() { return ((gController ? (int64_t)gController->getInstanceId() : 0) << 32) | mLockIdCounter++; }

RedisLock::RedisLock()
    : mLockId(INVALID_LOCK_ID)
{
}

RedisLock::RedisLock(const char8_t *key)
    : mLockId(INVALID_LOCK_ID)
{
    setKey(key);
}

RedisLock::~RedisLock()
{
    EA_ASSERT(mLockId == INVALID_LOCK_ID);
}

RedisError RedisLock::lock()
{
    return lockWithTimeout(EA::TDF::TimeValue(0));
}

RedisError RedisLock::lockWithTimeout(EA::TDF::TimeValue waitTimeout, const char8_t* context)
{
    EA_ASSERT_MSG(!mLockKey.empty(), "Attempt to call RedisLock::lock() without setting the key to lock via setKey().");
    Fiber::markRedisLockStartTimeOnCurrentFiber();

    TimeValue absoluteTimeout;
    if (waitTimeout != EA::TDF::TimeValue(0))
    {
        absoluteTimeout = TimeValue::getTimeOfDay() + waitTimeout;
    }
    else
    {
        // If no timeout was provided by the caller, then we use the current Fiber's timeout, if set.
        // Only if the current fiber has no timeout (at all) set on it, will we allow a lock to be held indefinitely.
        if (Fiber::getCurrentTimeout() > 0)
            absoluteTimeout = Fiber::getCurrentTimeout();
        else
            absoluteTimeout = INT64_MAX;
    }

    RedisError err = REDIS_ERR_OK;
    BlazeRpcError blazeErr = mFiberMutex.lockWithTimeout(waitTimeout, context);
    if (blazeErr != ERR_OK)
    {
        Fiber::markRedisLockEndTimeOnCurrentFiber();
        err = RedisErrorHelper::convertError(blazeErr);
        return REDIS_ERROR(err, "RedisLock.lock: Call to FiberMutex::lock() returned (" << err << ")");
    }

    if (mFiberMutex.getLockDepth() > 1)
    {
        Fiber::markRedisLockEndTimeOnCurrentFiber();
        return err;
    }

    mLockId = RedisLock::getNextLockId();

    // Assigns 'this' as a FrameworkResource on the current Fiber.
    assign();

    RedisCluster* cluster = gRedisManager->getRedisCluster(RedisManager::FRAMEWORK_NAMESPACE);
    if (cluster != nullptr)
    {
        RedisCommand cmd;
        cmd.setNamespace(RedisManager::FRAMEWORK_NAMESPACE);

        TimeValue lockKeyTtl;
        while ((lockKeyTtl = absoluteTimeout - TimeValue::getTimeOfDay()) > 0)
        {
            cmd.reset();
            if (absoluteTimeout != INT64_MAX)
                cmd.add("SET").addKey(mLockKey).add(mLockId).add("PX").add(lockKeyTtl.getMillis() + 1000).add("NX"); // plus 1 second to allow for some latency
            else
                cmd.add("SET").addKey(mLockKey).add(mLockId).add("NX");

            RedisResponsePtr resp;
            err = cluster->exec(cmd, resp);
            if (err != REDIS_ERR_OK)
            {
                Fiber::markRedisLockEndTimeOnCurrentFiber();
                BLAZE_ASSERT_LOG(Log::REDIS, "RedisLock.lock: Failed to call Redis while trying to obtain a lock, with err(" << RedisErrorHelper::getName(err) << ")");
                break;
            }

            if (resp->isStatus() && (blaze_stricmp(resp->getString(), "OK") == 0))
            {
                Fiber::markRedisLockEndTimeOnCurrentFiber();
                gRedisManager->addRedisLockKey(getLockKey());
                return REDIS_ERR_OK;
            }

            err = RedisErrorHelper::convertError(Fiber::sleep(50 * 1000, "RedisLock::lock"));
            if (err != REDIS_ERR_OK)
            {
                Fiber::markRedisLockEndTimeOnCurrentFiber();
                BLAZE_ASSERT_LOG(Log::REDIS, "RedisLock.lock: Failed to call Redis while trying to obtain a lock, with err(" << RedisErrorHelper::getName(err) << ")");
                break;
            }
            Fiber::incRedisLockAttemptsOnCurrentFiber();
        }
    }
    else
    {
        err = REDIS_ERR_SYSTEM;
        BLAZE_ASSERT_LOG(Log::REDIS, "RedisLock.lock: Failed to call Redis while trying to obtain a lock.");
    }
    
    unlock();

    Fiber::markRedisLockEndTimeOnCurrentFiber();

    if (err == REDIS_ERR_OK)
    {
        err = REDIS_ERR_TIMEOUT;
    }

    return err;
}

RedisError RedisLock::unlock()
{
    EA_ASSERT(mFiberMutex.isOwnedByCurrentFiberStack());

    if (mFiberMutex.getLockDepth() == 1)
    {
        release();
    }

    mFiberMutex.unlock();

    return REDIS_ERR_OK;
}

void RedisLock::assignInternal()
{
    // nothing to do here!
}

void RedisLock::releaseInternal()
{
    // We need to run the code that actually makes calls to Redis on a separate blockable Fiber.
    // This is because the Fiber that owns the lock might have already timed out, and can no longer 
    // make a blocking call to Redis.
    Fiber::CreateParams params;
    params.stackSize = Fiber::STACK_SMALL;
    params.groupId = gRedisManager->getActiveFiberGroupId();
    gSelector->scheduleFiberCall<const eastl::string&, RedisLockId>(&RedisLock::doDeleteLock, mLockKey, mLockId, "RedisLock::doDeleteLock", params);

    mLockId = INVALID_LOCK_ID;
}

void RedisLock::doDeleteLock(const eastl::string& _lockKey, RedisLockId lockId)
{
    // Make a copy of the string pointed to by _lockKey, because it is owned by the calling Fiber
    // and could be deleted when removeRedisLockKey() blocks, but we still need for callDeleteLock().
    eastl::string lockKey(_lockKey);

    gRedisManager->removeRedisLockKey(lockKey);

    RedisResponsePtr resp;
    callDeleteLock(resp, lockKey, lockId);
}


RedisScript RedisLock::msDeleteLock(1,
    "local a = redis.call('GET', KEYS[1]);"
    "if (a == ARGV[1]) then"
    "  redis.call('DEL', KEYS[1]);"
    "  return redis.status_reply('OK');"
    "else"
    "  return redis.error_reply('Lost the Lock');"
    "end;");

RedisError RedisLock::callDeleteLock(RedisResponsePtr &resp, const eastl::string &lockKey, RedisLockId lockId)
{
    RedisCommand cmd;
    cmd.addKey(lockKey).add(lockId);

    RedisCluster* cluster = gRedisManager->getRedisCluster(RedisManager::FRAMEWORK_NAMESPACE);
    if (cluster != nullptr)
    {
        return cluster->execScript(msDeleteLock, cmd, resp);
    }
    BLAZE_ASSERT_LOG(Log::REDIS, "RedisLock.callDeleteLock: Failed to get redis cluster.");
    return REDIS_ERR_SYSTEM;
}


bool WorkCoordinator::beginWorkOrWait()
{
    if (!mIsWorking)
    {
        mIsWorking = true;
        return true;
    }

    Fiber::EventHandle& waitHandle = mWaitList.push_back();
    waitHandle = Fiber::getNextEventHandle();
    Fiber::wait(waitHandle, "WorkCoordinator::beginWorkOrWait");
    return false;
}

void WorkCoordinator::doneWork(BlazeRpcError error)
{
    for (EventHandleList::iterator it = mWaitList.begin(), end = mWaitList.end();  it != end; ++it)
    {
        Fiber::delaySignal(*it, error);
    }

    mWaitList.clear();

    mIsWorking = false;
}

/*** Private Methods *****************************************************************************/

} // Blaze
