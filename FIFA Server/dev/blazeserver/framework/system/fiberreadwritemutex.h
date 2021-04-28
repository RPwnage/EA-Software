/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_FIBERREADWRITEMUTEX_H
#define BLAZE_FIBERREADWRITEMUTEX_H

/*** Include files *******************************************************************************/
#include "framework/system/fibermutex.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

typedef int32_t ReadLockContextId;

// This struct is a simple little grouping of an int32_t and an "option" for how a particular instance of ReadLockContext
// should be treated.  It serves a very specific purpose related to configuring a FiberReadWriteMutex's read-locks, and
// how they attach to a Fiber.  This all comes down to a read-locks ability to be automatically released if a Fiber
// throws an exception.
struct ReadLockContext
{
    // Set _attachToFiberAsResource to true causes the associated read-lock to be released automatically if a Fiber
    // excepts.  However, that also means that you can't pass a read-lock to another Fiber.  If you need to hand off
    // the read-lock to another Fiber (via scheduledFiberTimerCall, for example), then you need to pass false.
    inline ReadLockContext(bool _attachToFiberAsResource) : id(0), attachToFiberAsResource(_attachToFiberAsResource) {}
    inline void operator=(const int32_t& rlcId) { id = rlcId; }
    inline operator const int32_t() const { return id; }

    ReadLockContextId id;
    const bool attachToFiberAsResource;
};

const ReadLockContextId INVALID_READ_LOCK_CONTEXT = 0;

struct FiberReadWriteMutex
{
    NON_COPYABLE(FiberReadWriteMutex);

public:
    FiberReadWriteMutex();
    ~FiberReadWriteMutex();

    BlazeRpcError lockForRead(ReadLockContext& readLockContext, const char8_t* context);
    void unlockForRead(ReadLockContext& readLockContext);
    size_t getReaderCount() const { return mReadLockContextDataMap.size(); }
    bool isReadLockOwnedByCurrentFiberStack() const;
    EA::TDF::TimeValue getOldestReadLockTime() const;

    BlazeRpcError lockForWrite();
    // (Renamed to avoid issue with converting from old ms times to TimeValue.  TimeValue constructor uses microseconds.)
    BlazeRpcError lockForWriteWithTimeout(EA::TDF::TimeValue waitTimeout, const char8_t* context = nullptr);
    void unlockForWrite();

    bool isLockedForWrite() const { return mWriterMutex.isInUse(); }
    bool isWriteLockOwnedByCurrentFiberStack() const { return mWriterMutex.isOwnedByCurrentFiberStack(); }
    EA::TDF::TimeValue getWriteLockObtainedTime() const { return mWriteLockObtainedTime; }

    // This a special function, and should be used with care.  It simply increases the "readers" count without blocking
    // even if a write-lock has been requested.  Care should be used, because this can lead to starvation, whereby a fiber
    // waiting for the write-lock could never obtain the write-lock if other fibers continue calling this method.
    // Note, if the write-lock has already been obtained, then readers count will *not* be incremented.
    // 
    // Returns true if the readers count was incremented.
    // Returns false if the the readers count was not incremented, due to the write-lock already having been obtained.
    bool priorityLockForRead(ReadLockContext& readLockContext);
    void allowPriorityLockForRead();
    void disallowPriorityLockForRead();

    void fillDebugInfo(StringBuilder& sb) const;

private:
    friend class ReadLockContextData;

    class ReadLockContextData : public FrameworkResource // FrameworkResource supports auto-unlocking in the event of an exception
    {
    public:
        ReadLockContextData() : rlcId(INVALID_READ_LOCK_CONTEXT), owner(nullptr), fiberContext("<unknown>"), fiberId(Fiber::INVALID_FIBER_ID), fiberStackId(Fiber::INVALID_FIBER_ID), lockTime(0), priorityLock(false) {}
        ReadLockContextId rlcId;
        FiberReadWriteMutex* owner;
        const char8_t* fiberContext;
        Fiber::FiberId fiberId;
        Fiber::FiberId fiberStackId;
        Fiber::FiberGroupId fiberGroupId;
        EA::TDF::TimeValue lockTime;
        bool priorityLock;
        CallStack callstack;

        const char8_t* getTypeName() const override { return "ReadLockContextData"; }

        using FrameworkResource::assign;

    protected:
        void assignInternal() override { /* no-op */ }
        void releaseInternal() override;
    };

    void releaseReadLockContext(const ReadLockContextData& readLockContext);

    typedef eastl::hash_map<Fiber::FiberId, int32_t> Int32ByFiberIdMap;
    Int32ByFiberIdMap mReadLockCountByFiberStackIdMap;

    typedef eastl::hash_map<ReadLockContextId, ReadLockContextData/*, ReadLockContext::Hash*/> ReadLockContextDataMap;
    ReadLockContextDataMap mReadLockContextDataMap;
    ReadLockContextId mReadLockContextCounter;

    typedef eastl::list<Fiber::EventHandle> EventHandleList;
    EventHandleList mWaitingReaderEventList;

    Fiber::EventHandle mWaitingWriterEvent;
    FiberMutex mWriterMutex;
    bool mIsUnlockingForWrite;
    bool mAllowPriorityLockForRead;

    CallStack mCallstackLastReadLock;
    CallStack mCallstackLastReadUnlock;
    CallStack mCallstackLastWriteLock;
    CallStack mCallstackLastWriteUnlock;
    EA::TDF::TimeValue mWriteLockObtainedTime;
};

}

#endif // BLAZE_FIBERREADWRITEMUTEX_H
