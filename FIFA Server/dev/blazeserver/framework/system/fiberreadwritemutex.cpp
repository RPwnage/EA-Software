/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/system/fiberreadwritemutex.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

FiberReadWriteMutex::FiberReadWriteMutex() :
    mReadLockContextCounter(INVALID_READ_LOCK_CONTEXT),
    mIsUnlockingForWrite(false),
    mAllowPriorityLockForRead(false)
{
}

FiberReadWriteMutex::~FiberReadWriteMutex()
{
}

void FiberReadWriteMutex::allowPriorityLockForRead()
{
    EA_ASSERT(mWriterMutex.isOwnedByCurrentFiberStack());
    mAllowPriorityLockForRead = true;
}

void FiberReadWriteMutex::disallowPriorityLockForRead()
{
    EA_ASSERT(mWriterMutex.isOwnedByCurrentFiberStack());
    mAllowPriorityLockForRead = false;
    if (getReaderCount() > 0)
    {
        BlazeRpcError err = Fiber::getAndWait(mWaitingWriterEvent, "FiberReadWriteMutex::disallowPriorityLockForRead");
        if (err != ERR_OK)
        {
            BLAZE_ASSERT_LOG(Log::SYSTEM, "FiberReadWriteMutex.disallowPriorityLockForRead: getAndWait(mWaitingWriterEvent) failed with err(" << ErrorHelp::getErrorName(err) << "). Fiber context: " << Fiber::getCurrentContext());
        }
    }
}

bool FiberReadWriteMutex::priorityLockForRead(ReadLockContext& readLockContext)
{
    EA_ASSERT(readLockContext == INVALID_READ_LOCK_CONTEXT);
    readLockContext = ++mReadLockContextCounter;

    // The only time we cannot give priority access is when the mWriterMutex is actually the real current owner.
    // The writer has exclusive access only when it owns the write-lock *and* the reader count is 0.  In this case,
    // priority access cannot be given.
    if ((getReaderCount() == 0) && mWriterMutex.isOwnedByAnyFiberStack() && !mIsUnlockingForWrite && !mAllowPriorityLockForRead)
    {
        return false;
    }

    ReadLockContextData& context = mReadLockContextDataMap[readLockContext];
    context.rlcId = readLockContext;
    context.owner = this;
    context.fiberContext = Fiber::getCurrentContext();
    context.fiberId = Fiber::getCurrentFiberId();
    context.fiberGroupId = Fiber::getCurrentFiberGroupId();
    context.fiberStackId = Fiber::getFiberIdOfBlockingStack();
    context.lockTime = TimeValue::getTimeOfDay();
    context.priorityLock = true;
    context.callstack.getStack();

    mCallstackLastReadLock.getStack();

    ++mReadLockCountByFiberStackIdMap[context.fiberStackId];

    if (readLockContext.attachToFiberAsResource)
    {
        context.assign();
    }

    return true;
}

BlazeRpcError FiberReadWriteMutex::lockForRead(ReadLockContext& readLockContext, const char8_t* context)
{
    EA_ASSERT(!isWriteLockOwnedByCurrentFiberStack());

    EA_ASSERT(readLockContext == INVALID_READ_LOCK_CONTEXT);
    readLockContext = ++mReadLockContextCounter;

    BlazeRpcError err = ERR_OK;
    if (mWriterMutex.isInUse() && !isReadLockOwnedByCurrentFiberStack())
    {
        err = Fiber::getAndWait(mWaitingReaderEventList.push_back(), context);
        if (err != ERR_OK)
        {
            BLAZE_WARN_LOG(Log::SYSTEM, "FiberReadWriteMutex.lockForRead: Failed to get a read-lock due to err(" << ErrorHelp::getErrorName(err) << "). Fiber context: " << Fiber::getCurrentContext());
        }
    }

    if (err == ERR_OK)
    {
        ReadLockContextData& lockContext = mReadLockContextDataMap[readLockContext];
        lockContext.rlcId = readLockContext;
        lockContext.owner = this;
        lockContext.fiberContext = Fiber::getCurrentContext();
        lockContext.fiberId = Fiber::getCurrentFiberId();
        lockContext.fiberGroupId = Fiber::getCurrentFiberGroupId();
        lockContext.fiberStackId = Fiber::getFiberIdOfBlockingStack();
        lockContext.lockTime = TimeValue::getTimeOfDay();
        lockContext.priorityLock = false;
        lockContext.callstack.getStack();

        mCallstackLastReadLock.getStack();

        ++mReadLockCountByFiberStackIdMap[lockContext.fiberStackId];

        if (readLockContext.attachToFiberAsResource)
        {
            lockContext.assign();
        }
    }

    return err;
}

void FiberReadWriteMutex::unlockForRead(ReadLockContext& readLockContext)
{
    EA_ASSERT(readLockContext != INVALID_READ_LOCK_CONTEXT);

    ReadLockContextDataMap::iterator it = mReadLockContextDataMap.find(readLockContext);
    if (it == mReadLockContextDataMap.end())
    {
        EA_FAIL();
        return;
    }

    readLockContext = INVALID_READ_LOCK_CONTEXT;

    ReadLockContextData& readLockContextData = it->second;
    // FrameworkResource::clearOwningFiber() detaches 'this' from the owning fiber.  This prevents
    // the releaseInternal() method from being called when the owning fiber is reset.
    readLockContextData.clearOwningFiber();
    releaseReadLockContext(readLockContextData);
}

void FiberReadWriteMutex::ReadLockContextData::releaseInternal()
{
    // FrameworkResource::releaseInternal() will *always* be called for any framework resources that are still attached
    // to a fiber when it gets reset().  But in this case, we are only using the FrameworkResource facilities to react
    // to a fiber that has thrown an exception while it owns a Read-Lock.  Otherwise, it is entirely expected that the
    // user manually call unlockForRead(), which will properly clean the FrameworkResource and the owning fiber.
    if (Fiber::isCurrentlyInException() && (owner != nullptr))
    {
        // IMPORTANT: 'this' is deleted after this call.  Do not reference 'this' after this point.
        owner->releaseReadLockContext(*this);
    }
}

void FiberReadWriteMutex::releaseReadLockContext(const ReadLockContextData& readLockContextData)
{
    Int32ByFiberIdMap::iterator fiberMapIt = mReadLockCountByFiberStackIdMap.find(readLockContextData.fiberStackId);
    if ((fiberMapIt != mReadLockCountByFiberStackIdMap.end()) && (fiberMapIt->second > 0))
    {
        if (--fiberMapIt->second == 0)
            mReadLockCountByFiberStackIdMap.erase(fiberMapIt);
    }
    else
    {
        BLAZE_ASSERT_LOG(Log::SYSTEM, "FiberReadWriteMutex.unlockForRead: The ReadLockContext(" << readLockContextData.rlcId << ").fiberStackId(" << readLockContextData.fiberStackId << ") is not a know owner of a read-lock.");
    }

    ReadLockContextDataMap::iterator readLockMapIt = mReadLockContextDataMap.find(readLockContextData.rlcId);
    if (readLockMapIt != mReadLockContextDataMap.end())
    {
        // Turns out, eastl::hashtable::erase() has an annoying implementation.  There is no specialization for the erase()
        // method for the bUniqueKeys template arg.  Even though this is a hash_map with unique keys, if you try to use the erase(const key&)
        // overload, it actually iterates the entire map calling compare() and passing a reference to to the key arg.  But, if the
        // key arg was actually a member stored in the 'value' of the map, it will get deleted, and then immediately referenced.  Kinda lame.
        // So, to avoid both of those problems, we manually find the iterator and then erase() the iterator itself.
        mReadLockContextDataMap.erase(readLockMapIt);
    }

    mCallstackLastReadUnlock.getStack();
    if (getReaderCount() == 0)
    {
        Fiber::signal(mWaitingWriterEvent, ERR_OK);
    }
}

bool FiberReadWriteMutex::isReadLockOwnedByCurrentFiberStack() const
{
    return (mReadLockCountByFiberStackIdMap.find(Fiber::getFiberIdOfBlockingStack()) != mReadLockCountByFiberStackIdMap.end());
}

TimeValue FiberReadWriteMutex::getOldestReadLockTime() const
{
    TimeValue oldestReadLockTime;
    for (FiberReadWriteMutex::ReadLockContextDataMap::const_iterator rwIt = mReadLockContextDataMap.begin(), rwEnd = mReadLockContextDataMap.end(); rwIt != rwEnd; ++rwIt)
    {
        const FiberReadWriteMutex::ReadLockContextData& readContextData = rwIt->second;
        if ((readContextData.lockTime < oldestReadLockTime) || (oldestReadLockTime == 0))
            oldestReadLockTime = readContextData.lockTime;
    }

    return oldestReadLockTime;
}

BlazeRpcError FiberReadWriteMutex::lockForWrite()
{
    return lockForWriteWithTimeout(EA::TDF::TimeValue(0));
}
BlazeRpcError FiberReadWriteMutex::lockForWriteWithTimeout(EA::TDF::TimeValue waitTimeout, const char8_t* context)
{
    BlazeRpcError err = mWriterMutex.lockWithTimeout(waitTimeout, context);
    mWriteLockObtainedTime = TimeValue::getTimeOfDay();
    mCallstackLastWriteLock.getStack();
    if (err == ERR_OK)
    {
        if (getReaderCount() > 0)
        {
            err = Fiber::getAndWait(mWaitingWriterEvent, "FiberReadWriteMutex::lockForWrite");
            if (err != ERR_OK)
            {
                mWriteLockObtainedTime = 0;
                mWriterMutex.unlock();
            }
        }
    }

    return err;
}

void FiberReadWriteMutex::unlockForWrite()
{
    EA_ASSERT(!mAllowPriorityLockForRead);
    mIsUnlockingForWrite = true;
    while (!mWaitingReaderEventList.empty())
    {
        Fiber::signal(mWaitingReaderEventList.front(), ERR_OK);
        mWaitingReaderEventList.pop_front();
    }
    mIsUnlockingForWrite = false;

    mCallstackLastWriteUnlock.getStack();
    mWriteLockObtainedTime = 0;
    mWriterMutex.unlock();
}

void FiberReadWriteMutex::fillDebugInfo(StringBuilder& sb) const
{
    if (mWriterMutex.isOwnedByAnyFiberStack())
    {
        eastl::string writeLockCallstack;
        mCallstackLastWriteLock.getSymbols(writeLockCallstack);

        sb << "Write lock obtained at:\n" << writeLockCallstack << "\n";
    }
    else
    {
        eastl::string writeUnlockCallstack;
        mCallstackLastWriteUnlock.getSymbols(writeUnlockCallstack);

        sb << "Write lock was released at:\n" << writeUnlockCallstack << "\n";
    }

    sb << "Read lock context details (count: " << mReadLockContextDataMap.size() << "):\n";

    for (ReadLockContextDataMap::const_iterator it = mReadLockContextDataMap.begin(), end = mReadLockContextDataMap.end(); it != end; ++it)
    {
        const ReadLockContextData& data = it->second;

        char8_t lockTimeStr[64] = "";
        data.lockTime.toString(lockTimeStr, sizeof(lockTimeStr));

        TimeValue lockDuration;
        lockDuration = TimeValue::getTimeOfDay() - data.lockTime;
        char8_t lockDurationStr[64] = "";
        lockDuration.toIntervalString(lockDurationStr, sizeof(lockDurationStr));

        eastl::string readLockCallstack;
        data.callstack.getSymbols(readLockCallstack);

        StringBuilder fiberGroupStatus;
        Fiber::dumpFiberGroupStatus(data.fiberGroupId, fiberGroupStatus);

        sb << "ContextId: " << it->first << "\n";
        sb << "   fiber context: " << data.fiberContext << "\n";
        sb << "   fiber id     : " << data.fiberId << "\n";
        sb << "   fiber grp id : " << data.fiberGroupId << "\n";
        sb << "   fiber stk id : " << data.fiberStackId << "\n";
        sb << "   priority lock: " << data.priorityLock << "\n";
        sb << "   acquired time: " << lockTimeStr << " (" << data.lockTime.getMicroSeconds() << ")\n";
        sb << "   lock duration: " << lockDurationStr << " (" << lockDuration.getMicroSeconds() << ")\n";
        sb << "   callstack:\n";
        sb << SbFormats::PrefixAppender("      ", readLockCallstack.c_str()) << "\n";
        sb << "   fiber group status:\n";
        sb << SbFormats::PrefixAppender("      ", fiberGroupStatus.get()) << "\n";
    }
}

} // Blaze
