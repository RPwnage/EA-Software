/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_MEM_TRACKER_H
#define BLAZE_MEM_TRACKER_H

/*** Include files *******************************************************************************/
#include "eathread/eathread_atomic.h"
#include "eathread/eathread_mutex.h"
#include "framework/system/allocator/callstack.h"
#include "framework/system/allocator/memallocator.h"
#include "framework/system/allocator/memtrackerinfo.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

class CoreManager;
class AllocatorManager;

// ----------------------------------------------------
// Memory allocation tracker
// ----------------------------------------------------

/* *
 * This structure is _currently_ 16 byte aligned, but if we find this
 * overhead too much in the future, we could shrink it to total 8 bytes
 * by changing mAllocInfo to a 24 bit index, and storing alloc infos in
 * an array, and by storing the size in the remining bits.
 * One upside of using 16 bytes per tag is that it may enable better 
 * troubleshooting of memory stomps in C++ objects because the freelist 
 * pointers in malloc will use the space reserved for MemTag instead of 
 * clobbering the v-table.
 * */
struct MemTag
{
    AllocInterface* mAllocInfo;
    uint64_t mAllocSize;
};

typedef uintptr_t MemTrackId;

/**
    Since all our contexts are static strings in the .rodata segment, their addresses fit comfortably into 48 bits of address space
    leaving the remaining 16 bits for the memory group id. This allows us to perform very fast lookups on those keys.
*/
#define MEM_TRACKER_MAKE_ID(_memGroupId, _context) (MemTrackId)(((uintptr_t) _memGroupId << 48) | (uintptr_t)_context)
#define MEM_TRACKER_GET_MGID(_memTrackId) (Blaze::MemoryGroupId)(_memTrackId >> 48)
#define MEM_TRACKER_GET_CONTEXT(_memTrackId) (char8_t*)((_memTrackId << 16) >> 16)
#define MEM_TRACKER_VALIDATE_CONTEXT(_context) (((uintptr_t) _context >> 48) == 0)

struct MemTrackIdLess
{
    bool operator()(const MemTrackId a, const MemTrackId b) const
    {
        Blaze::MemoryGroupId _a = MEM_TRACKER_GET_MGID(a);
        Blaze::MemoryGroupId _b = MEM_TRACKER_GET_MGID(b);
        if (_a == _b)
            return blaze_stricmp(MEM_TRACKER_GET_CONTEXT(a), MEM_TRACKER_GET_CONTEXT(b)) < 0;
        return _a < _b;
    }
};

class InternalAllocator;
class PerThreadMemTracker;

class GlobalMemTracker
{
public:
    GlobalMemTracker(InternalAllocator& allocator);
    ~GlobalMemTracker();
    GAllocInfo& trackAlloc(MemTrackId id);
    GAllocInfo::GStackAllocInfo& trackAlloc(GAllocInfo& alloc, const CallStack& cs);
    void setStackTracking(Blaze::MemoryGroupId memGroupId, const char8_t* context, bool enabled);
    void processConfigureMemoryMetrics(const Blaze::ConfigureMemoryMetricsRequest &request, Blaze::ConfigureMemoryMetricsResponse &response);
    void processGetMemoryMetrics(const Blaze::GetMemoryMetricsRequest &request, Blaze::GetMemoryMetricsResponse &response);
    void createPerThreadMemTracker();
    void shutdownPerThreadMemTracker();
private:
    typedef eastl::map<MemTrackId, GAllocInfo, MemTrackIdLess, Blaze::BlazeStlAllocator> AllocInfoMap;
    typedef eastl::hash_set<eastl::string, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> StackConfigSet;
    typedef eastl::hash_map<Blaze::MemoryGroupId, StackConfigSet> StackConfigMap;
    typedef eastl::list<PerThreadMemTracker, Blaze::BlazeStlAllocator> PerThreadMemTrackerList;

    EA::Thread::Mutex mLock;
    EA::Thread::Mutex mPerThreadTrackerListLock; // need separate mutex for per-thread tracker construction to avoid thrashing with mLock used for managing global trackers
    AllocInfoMap mAllocInfoMap; // map grouping allocations by MemoryGroupId & context string
    StackConfigMap mStackConfigMap; // always used outside the lock, doesn't use the internal allocator!
    PerThreadMemTrackerList mPerThreadTrackerList; // owns thread specific trackers because threads will try to update per/thread info that they did not allocate after the owner thread has exited.
};

class PerThreadMemTracker
{
public:
    PerThreadMemTracker();
    ~PerThreadMemTracker();
    static void trackAlloc(MemTag& tag, size_t size, Blaze::MemoryGroupId memGroupId, const char8_t* context);
    static void trackRealloc(MemTag& tag, size_t size, Blaze::MemoryGroupId memGroupId, const MemTag& oldTag);
    static void trackFree(MemTag& tag);
    void flushTrackers();

private:
    typedef eastl::hash_map<MemTrackId, TAllocInfo, eastl::hash<MemTrackId>, eastl::equal_to<MemTrackId>, Blaze::BlazeStlAllocator> AllocInfoMap;

    TAllocInfo& trackAlloc(MemTrackId id);
    TAllocInfo::TStackAllocInfo& trackAlloc(TAllocInfo& alloc, const CallStack& cs);

    static const uint32_t CACHE_ENTRY_MAX = 4; // 4 entries result in 80% cache hit rate, without adding any overhead because they fit into a single 64 bit cache line
    static const uint32_t CACHE_ENTRY_MASK = CACHE_ENTRY_MAX-1;

    struct AllocCacheEntry
    {
        AllocCacheEntry() : mMemTrackId(0), mAllocInfo(nullptr) {}
        MemTrackId mMemTrackId;
        TAllocInfo* mAllocInfo;
    };

    AllocCacheEntry mLastEntry[CACHE_ENTRY_MAX];
    uint32_t mCacheIndex;


    AllocInfoMap mAllocInfoMap; // map of maps grouping allocations by MemoryGroupId & context string
};

template<typename T>
class TrackingAdapter : public T
{
public:
    TrackingAdapter(Blaze::MemoryGroupId id, CoreManager& coreManager, AllocatorManager& manager)
        : T(id, coreManager, manager) {}

    void *Alloc(size_t size, const char *name, unsigned int flags) override
    {
        size += sizeof(MemTag);
        void* mem = T::Alloc(size, name, flags);
        if (mem != nullptr)
        {
            MemTag* mt = (MemTag*) mem;
            PerThreadMemTracker::trackAlloc(*mt, size, T::mId, name);
            mem = (mt + 1);
        }
        return mem;
    }

    void *Alloc(size_t size, const char *name, unsigned int flags,
        unsigned int align, unsigned int alignOffset = 0) override
    {
        size += sizeof(MemTag);
        void* mem = T::Alloc(size, name, flags, align, alignOffset);
        if (mem != nullptr)
        {
            MemTag* mt = (MemTag*) mem;
            PerThreadMemTracker::trackAlloc(*mt, size, T::mId, name);
            mem = (mt + 1);
        }
        return mem;
    }

    void* Realloc(void *addr, size_t size, unsigned int flags) override
    {
        size += sizeof(MemTag);
        MemTag* tag = ((MemTag*) addr) - 1;
        MemTag oldTag = *tag;
        void* mem = T::Realloc(tag, size, flags);
        if (mem != nullptr)
        {
            MemTag* mt = (MemTag*) mem;
            PerThreadMemTracker::trackRealloc(*mt, size, T::mId, oldTag);
            mem = (mt + 1);
        }
        return mem;
    }

    void Free(void *block, size_t size=0) override
    {
        if (block != nullptr)
        {
            MemTag* tag = ((MemTag*) block) - 1;
            PerThreadMemTracker::trackFree(*tag);
            T::Free(tag, size + sizeof(MemTag));
        }
    }
};


#endif // BLAZE_MEM_TRACKER_H

