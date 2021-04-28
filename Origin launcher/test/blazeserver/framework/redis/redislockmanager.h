/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REDIS_LOCK_MANAGER_H
#define BLAZE_REDIS_LOCK_MANAGER_H

/*** Include files *******************************************************************************/

#include "framework/system/fibermutex.h"
#include "framework/redis/redisscriptregistry.h"
#include "framework/redis/rediserrors.h"
#include "framework/redis/redisresponse.h"
#include "framework/redis/rediscommand.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

typedef int64_t RedisLockId;

class RedisLock : public FrameworkResource
{
    NON_COPYABLE(RedisLock);

public:
    static const RedisLockId INVALID_LOCK_ID = 0;

public:
    RedisLock();
    RedisLock(const char8_t *key);
    ~RedisLock() override;

    template <typename T>
    void setKey(const T& key);

    const eastl::string &getKey() const { return mKey; }
    const eastl::string &getLockKey() const { return mLockKey; }
    RedisLockId getLockId() const { return mLockId; }

    RedisError lock();
    // (Renamed to avoid issue with converting from old ms times to TimeValue.  TimeValue constructor uses microseconds.)
    RedisError lockWithTimeout(EA::TDF::TimeValue waitTimeout, const char8_t* context = nullptr);
    RedisError unlock();

    bool isInUse() const { return mFiberMutex.isInUse(); }
    bool isOwnedByAnyFiberStack() const { return mFiberMutex.isOwnedByAnyFiberStack(); }
    bool isOwnedByCurrentFiberStack() const { return mFiberMutex.isOwnedByCurrentFiberStack(); }

    int32_t getLockDepth() const { return mFiberMutex.getLockDepth(); }

    const char8_t* getTypeName() const override { return "RedisLock"; }

protected:
    void assignInternal() override;
    void releaseInternal() override;

private:
    friend class RedisCluster;

    static void doDeleteLock(const eastl::string& _lockKey, RedisLockId lockId);

private:
    eastl::string mKey;
    eastl::string mLockKey;
    RedisLockId mLockId;

    FiberMutex mFiberMutex;

    static EA_THREAD_LOCAL RedisLockId mLockIdCounter;
    static RedisLockId getNextLockId();

    static RedisScript msDeleteLock;
    static RedisError callDeleteLock(RedisResponsePtr &resp, const eastl::string& lockKey, RedisLockId lockId);
};

template <typename T>
void RedisLock::setKey(const T& key)
{
    EA_ASSERT_MSG(!isInUse(), "Attempt to call RedisLock::setKey() while this instance is already locked.");
    mKey.clear();

    RedisCommand cmd;
    cmd.add(key);
    cmd.encode(mKey);

    mLockKey.clear();
    mLockKey.append("{");
    cmd.encode(mLockKey);
    mLockKey.append("}.__lock");
}

class RedisAutoLock :
    public RedisLock
{
    NON_COPYABLE(RedisAutoLock);
public:
    RedisAutoLock(const char8_t* key) : RedisLock(key) {}

    ~RedisAutoLock() override
    {
        if (isOwnedByCurrentFiberStack())
        {
            unlock();
        }
    }

    bool hasLock() const { return isOwnedByCurrentFiberStack(); }
};

struct WorkCoordinator
{
public:
    WorkCoordinator() : mIsWorking(false) {}

    bool beginWorkOrWait();
    void doneWork(BlazeRpcError error);

    bool isWorking() const { return mIsWorking; }

private:
    bool mIsWorking;

    typedef eastl::deque<Fiber::EventHandle> EventHandleList;
    EventHandleList mWaitList;
};

} // Blaze

#endif // BLAZE_REDIS_LOCK_MANAGER_H
