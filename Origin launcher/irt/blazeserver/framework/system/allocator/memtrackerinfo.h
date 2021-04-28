/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_MEM_TRACKER_INFO_H
#define BLAZE_MEM_TRACKER_INFO_H

/*** Include files *******************************************************************************/
#include "EASTL/map.h"
#include "eathread/eathread_atomic.h"
#include "framework/system/allocator/callstack.h"
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

typedef EA::Thread::AtomicInt<int64_t> AtomicInt64;


// ----------------------------------------------------
// Memory allocation tracker
// ----------------------------------------------------

struct AllocInterface
{
    static const int32_t TRACKER_UPDATE_SIZE = 100000;
    static const int32_t TRACKER_UPDATE_COUNT = 100;

    virtual ~AllocInterface() {}
    virtual void trackAlloc(int64_t size, int64_t count) = 0;
    virtual void trackFree(int64_t size, int64_t count) = 0;
};

struct AllocInfoCommon : public AllocInterface
{
    AtomicInt64 mAllocCount;
    AtomicInt64 mAllocSize;
    AtomicInt64 mFreeCount;
    AtomicInt64 mFreeSize;

    AllocInfoCommon();
    void getAllocSizeAndCount(int64_t& size, int64_t& count, bool total) const;
    virtual const char8_t* getContext() const = 0;
    virtual const CallStack* getCallstack() const = 0;
};

struct GAllocInfo : public AllocInfoCommon
{
    struct GStackAllocInfo : public AllocInfoCommon
    {
        GAllocInfo* mGAllocParent;
        const CallStack* mCallStack;

        GStackAllocInfo();
        void trackAlloc(int64_t size, int64_t count) override;
        void trackFree(int64_t size, int64_t count) override;
        const char8_t* getContext() const override;
        const CallStack* getCallstack() const override;
    };
    // NOTE: Intentionally uses map instead of hash_map because hashing the callstack proved troublesome since
    // stacks are large blocks of data that often differ only in a few top addresses, causing hashing to collide
    // and create very long hash_node chains that adversely affect lookup performance.
    // Using a map on the other hand is safe from collisions, and has the advantange of using memcmp
    // which first compares the most likely addresses to be different. The latter property allows the
    // comparison function to touch minimal memory when comparing elements of the map, raising efficiency.
    typedef eastl::map<CallStack, GStackAllocInfo, CallStackCompare, Blaze::BlazeStlAllocator> GStackAllocInfoMap;

    bool mStackTrackingEnabled;
    const char8_t* mContext;
    GStackAllocInfoMap mStackInfoMap;

    GAllocInfo();
    void trackAlloc(int64_t size, int64_t count) override;
    void trackFree(int64_t size, int64_t count) override;
    const char8_t* getContext() const override { return mContext; }
    const CallStack* getCallstack() const override { return nullptr; }
};

class PerThreadMemTracker;

struct TAllocInfoCommon : public AllocInterface
{
    int64_t mAllocCount;
    int64_t mAllocSize;
    int64_t mFreeCount;
    int64_t mFreeSize;
    PerThreadMemTracker* mTracker;
    TAllocInfoCommon();
};

struct TAllocInfo : public TAllocInfoCommon
{
    struct TStackAllocInfo : public TAllocInfoCommon
    {
        GAllocInfo::GStackAllocInfo* mGStackAllocParent;

        TStackAllocInfo();
        void trackAlloc(int64_t size, int64_t count) override;
        void trackFree(int64_t size, int64_t count) override;
        void flush();
    };
    // NOTE: Intentionally uses map instead of hash_map because hashing the callstack proved troublesome since
    // stacks are large blocks of data that often differ only in a few top addresses, causing hashing to collide
    // and create very long hash_node chains that adversely affect lookup performance.
    // Using a map on the other hand is safe from collisions, and has the advantange of using memcmp
    // which first compares the most likely addresses to be different. The latter property allows the
    // comparison function to touch minimal memory when comparing elements of the map, raising efficiency.
    typedef eastl::map<CallStack, TStackAllocInfo, CallStackCompare, Blaze::BlazeStlAllocator> TStackAllocInfoMap;

    GAllocInfo* mGAllocParent;
    TStackAllocInfoMap mStackInfoMap;

    TAllocInfo();
    void trackAlloc(int64_t size, int64_t count) override;
    void trackFree(int64_t size, int64_t count) override;
    void flush();
};


#endif // BLAZE_MEM_TRACKER_INFO_H

