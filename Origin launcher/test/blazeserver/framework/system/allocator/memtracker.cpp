/*************************************************************************************************/
/*! \file


    This file has functions for handling the per thread allocation system.

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "EASTL/heap.h"
#include "framework/tdf/controllertypes_server.h" //for MemoryStatus definition
#include "framework/system/allocator/allocatormanager.h"
#include "framework/system/allocator/memtracker.h"


extern const char8_t* BLAZE_INVALID_ALLOC_NAME;

namespace Blaze
{
    extern EA_THREAD_LOCAL PerThreadMemTracker* gPerThreadMemTracker;
}

GlobalMemTracker::GlobalMemTracker(InternalAllocator& allocator) :
    mAllocInfoMap(Blaze::BlazeStlAllocator("GlobalMemTracker::mAllocByMgContextMap", &allocator)),
    mStackConfigMap(Blaze::BlazeStlAllocator("GlobalMemTracker::mStackConfigMap", &allocator)),
    mPerThreadTrackerList(Blaze::BlazeStlAllocator("GlobalMemTracker::mPerThreadTrackerList", &allocator))
{
}

GlobalMemTracker::~GlobalMemTracker()
{
    InternalAllocator* alloc = &AllocatorManager::getInstance().getInternalAllocator();
    for (AllocInfoMap::iterator i = mAllocInfoMap.begin(), e = mAllocInfoMap.end(); i != e; ++i)
    {
        char8_t* context = MEM_TRACKER_GET_CONTEXT(i->first);
        CORE_FREE(alloc, context);
    }
}

GAllocInfo& GlobalMemTracker::trackAlloc(MemTrackId id)
{
    GAllocInfo* info;

    mLock.Lock();
    AllocInfoMap::iterator it = mAllocInfoMap.find(id);
    if (it != mAllocInfoMap.end())
        info = &it->second;
    else
    {
        // NOTE: We must cache a copy of the string context pointer in the global memory tracker, because some libraries(openssl)
        // use the global memory tracker to perform memory allocations during process shutdown (even after some libraries that have 
        // registered allocation contexts have already been unloaded); therefore, we need to cache all the context strings to ensure 
        // that all allocation contexts always remain valid, even during process shutdown.
        const Blaze::MemoryGroupId mgId = MEM_TRACKER_GET_MGID(id);
        const char8_t* context = MEM_TRACKER_GET_CONTEXT(id);
        const size_t size = strlen(context) + 1;
        char8_t* newContext = (char8_t*) CORE_ALLOC(&AllocatorManager::getInstance().getInternalAllocator(), size, "GlobalMemTracker::mAllocInfoMap.key", 0);
        memcpy(newContext, context, size);
        const MemTrackId newId = MEM_TRACKER_MAKE_ID(mgId, newContext);
        info = &mAllocInfoMap[newId];
        info->mContext = newContext;
    }
    mLock.Unlock();

    return *info;
}

GAllocInfo::GStackAllocInfo& GlobalMemTracker::trackAlloc(GAllocInfo& alloc, const CallStack& cs)
{
    mLock.Lock();
    GAllocInfo::GStackAllocInfoMap::insert_return_type ret = alloc.mStackInfoMap.insert(cs);
    GAllocInfo::GStackAllocInfo& info = ret.first->second;
    if (ret.second)
    {
        info.mGAllocParent = &alloc;
        info.mCallStack = &ret.first->first;
    }
    mLock.Unlock();
    return info;
}

void GlobalMemTracker::setStackTracking(Blaze::MemoryGroupId memGroupId, const char8_t* context, bool enabled)
{
    // We intentionally avoid using the per thread tracker because it hashes the read-only statically allocated string pointer.
    // Meanwhile, to stack tracking enable/disable code always uses a dynamically allocated string specified in the TDF
    // body of an RPC. The global mem tracker on the other hand, hashes the actual contents of the statically allocated string
    // and is therefore the appropriate way to look up the AllocInfo.
    MemTrackId id = MEM_TRACKER_MAKE_ID(memGroupId, context);

    GAllocInfo& allocInfo = trackAlloc(id);

    // This value can be safely set outside a lock because delayed reads/writes to it 
    // can never affect the correctness of other operations.
    allocInfo.mStackTrackingEnabled = enabled;

    StackConfigSet& configSet = mStackConfigMap[memGroupId];
    if (enabled)
        configSet.insert(context);
    else
        configSet.erase(context);
}

void GlobalMemTracker::processConfigureMemoryMetrics(const Blaze::ConfigureMemoryMetricsRequest &request, Blaze::ConfigureMemoryMetricsResponse &response)
{
    const Blaze::ConfigureMemoryMetricsRequest::MemTrackerOptList& list = request.getTrackerOpts();
    for (Blaze::ConfigureMemoryMetricsRequest::MemTrackerOptList::const_iterator i = list.begin(), e = list.end(); i != e; ++i)
    {
        const Blaze::MemTrackerOpt& opt = **i;
        Blaze::MemoryGroupId mgId = Blaze::MEM_GROUP_FRAMEWORK_DEFAULT;
        if (AllocatorManager::parseMemoryGroupName(opt.getMemGroup(), mgId))
        {
            // this will also update the stack config map, which we use below to determine current configuration
            setStackTracking(mgId, opt.getContext(), opt.getEnableStackTracking());
        }
    }

    uint32_t configSize = 0;
    for (StackConfigMap::const_iterator i = mStackConfigMap.begin(), e = mStackConfigMap.end(); i != e; ++i)
    {
        configSize += (uint32_t) i->second.size();
    }

    response.getTrackerOpts().reserve(configSize);
    for (StackConfigMap::const_iterator i = mStackConfigMap.begin(), e = mStackConfigMap.end(); i != e; ++i)
    {
        for (StackConfigSet::const_iterator ji = i->second.begin(), je = i->second.end(); ji != je; ++ji)
        {
            char8_t buf[256];
            Blaze::MemTrackerOpt* respOpt = response.getTrackerOpts().pull_back();
            const char8_t* mgName = AllocatorManager::getMemoryGroupName(i->first, buf, sizeof(buf));
            respOpt->setMemGroup(mgName);
            respOpt->setContext(ji->c_str());
            respOpt->setEnableStackTracking(true);
        }
    }
}

struct MemTrackerReportEntry
{
    int64_t mSize;
    int64_t mCount;
    Blaze::MemoryGroupId mMemGroupId;
    const AllocInfoCommon* mAllocInfo;

    struct CompareSizeOrCount
    {
        bool mCompareSize;
        CompareSizeOrCount(bool compareSize) : mCompareSize(compareSize) {}
        bool operator()(const MemTrackerReportEntry& a, const MemTrackerReportEntry& b) const
        {
            // Intentionally reversed because heap sorts in descending order,
            // but we need to maintain ascending order to pop out the smallest item.
            // Conveniently, we can also use this reversed comparison to perform a final
            // descending sort.
            return mCompareSize ? a.mSize > b.mSize : a.mCount > b.mCount;
        }
    };
};

class MemTrackerReport
{
public:
    MemTrackerReport(const Blaze::GetMemoryMetricsRequest &request);
    void insert(Blaze::MemoryGroupId mgId, int64_t size, int64_t count, const AllocInfoCommon& info);
    bool filter(int64_t allocSize, int64_t allocCount) const;
    void buildResponse(Blaze::GetMemoryMetricsResponse &response);

private:
    typedef eastl::vector<MemTrackerReportEntry, Blaze::BlazeStlAllocator> MemTrackerReportList;
    static const uint32_t ENTRY_RESERVE_DEFAULT = 100;
    MemTrackerReportList mEntries;
    bool mStackOnly;
    bool mSortBySize;
    int64_t mMinSize;
    uint32_t mLimitCount;
};

MemTrackerReport::MemTrackerReport(const Blaze::GetMemoryMetricsRequest &request)
:   mEntries(Blaze::BlazeStlAllocator("MemTrackerReport::mEntries", &AllocatorManager::getInstance().getInternalAllocator())),
    mStackOnly(request.getHasStack()), mSortBySize(request.getSortBy() == Blaze::GetMemoryMetricsRequest::ALLOC_BYTES), mMinSize(request.getMinSize()), mLimitCount(request.getTopN())
{
    mEntries.reserve(mLimitCount > 0 ? mLimitCount : ENTRY_RESERVE_DEFAULT);
}

inline bool MemTrackerReport::filter(int64_t allocSize, int64_t allocCount) const
{
    // NOTE: alloc/free size/count totals are stored and updated independently and periodically flushed from thread local cache for performance reasons. This means that it is possible
    // for either allocSize=(totalAlloc-totalFree) or allocCount=(totalCount-totalFree) to become temporarily negative in the event when the 'free' counters are flushed before the 'alloc'
    // counters. This is a rare, harmless and self correcting condition and only occurs when the values are close enough to each other; therefore, we guard against negative deltas to
    // avoid reporting nonsense negative values.
    if (allocSize < 0 || allocCount < 0)
        return true;
    return (allocSize < mMinSize) || (mLimitCount > 0 && mEntries.size() >= mLimitCount && (mSortBySize ? allocSize <= mEntries.front().mSize : allocCount <= mEntries.front().mCount));
}

void MemTrackerReport::insert(Blaze::MemoryGroupId mgId, int64_t size, int64_t count, const AllocInfoCommon& info)
{
    MemTrackerReportEntry& entry = mEntries.push_back();
    entry.mSize = size;
    entry.mCount = count;
    entry.mMemGroupId = mgId;
    entry.mAllocInfo = &info;
    if (mLimitCount > 0)
    {
        eastl::push_heap(mEntries.begin(), mEntries.end(), MemTrackerReportEntry::CompareSizeOrCount(mSortBySize));
        if (mEntries.size() > mLimitCount)
        {
            // take out the smallest entry
            eastl::pop_heap(mEntries.begin(), mEntries.end(), MemTrackerReportEntry::CompareSizeOrCount(mSortBySize));
            mEntries.pop_back();
        }
    }
}

void MemTrackerReport::buildResponse(Blaze::GetMemoryMetricsResponse &response)
{
    // sort in descending order
    eastl::sort(mEntries.begin(), mEntries.end(), MemTrackerReportEntry::CompareSizeOrCount(mSortBySize));

    Blaze::GetMemoryMetricsResponse::MemTrackerStatusList& resultList = response.getMatchedTrackers();    
    resultList.reserve(mEntries.size());

    for (MemTrackerReportList::const_iterator i = mEntries.begin(), e = mEntries.end(); i != e; ++i)
    {
        char8_t buf[256];
        Blaze::MemoryGroupId mgId = i->mMemGroupId;
        const char8_t* mgName = AllocatorManager::getMemoryGroupName(mgId, buf, sizeof(buf));
        Blaze::MemTrackerStatus* status = resultList.pull_back();
        const CallStack* cs = i->mAllocInfo->getCallstack();
        if (mStackOnly && cs != nullptr)
            cs->getSymbols(status->getStackTrace());
        status->setMemGroup(mgName);
        status->setContext(i->mAllocInfo->getContext());
        status->setAllocBytes(i->mSize);
        status->setAllocCount(i->mCount);
    }
}

void GlobalMemTracker::processGetMemoryMetrics(const Blaze::GetMemoryMetricsRequest &request, Blaze::GetMemoryMetricsResponse &response)
{
    MemTrackerReport report(request);
    const bool onlyStack = request.getHasStack();
    const bool currentAlloc = request.getCurrentAlloc();
    const Blaze::GetMemoryMetricsRequest::MemTrackerInfoList& trackers = request.getTrackers();

    // first flush all the trackers from this thread, (most allocations are done on the server thread anyway)
    if (Blaze::gPerThreadMemTracker != nullptr)
    {
        Blaze::gPerThreadMemTracker->flushTrackers();
    }

    mLock.Lock();
    if (onlyStack)
    {
        AllocInfoMap::const_iterator end = mAllocInfoMap.end();
        if (trackers.empty())
        {
            for (AllocInfoMap::const_iterator it = mAllocInfoMap.begin(); it != end; ++it)
            {
                GAllocInfo::GStackAllocInfoMap::const_iterator si = it->second.mStackInfoMap.begin();
                GAllocInfo::GStackAllocInfoMap::const_iterator se = it->second.mStackInfoMap.end();
                Blaze::MemoryGroupId mgId = MEM_TRACKER_GET_MGID(it->first);
                for (; si != se; ++si)
                {
                    int64_t allocSize = 0;
                    int64_t allocCount = 0;
                    si->second.getAllocSizeAndCount(allocSize, allocCount, !currentAlloc);
                    if (!report.filter(allocSize, allocCount))
                        report.insert(mgId, allocSize, allocCount, si->second);
                }
            }
        }
        else
        {
            for (Blaze::GetMemoryMetricsRequest::MemTrackerInfoList::const_iterator i = trackers.begin(), e = trackers.end(); i != e; ++i)
            {
                const Blaze::MemTrackerInfo& mt = **i;
                Blaze::MemoryGroupId mgId = Blaze::MEM_GROUP_FRAMEWORK_DEFAULT;
                if (!AllocatorManager::parseMemoryGroupName(mt.getMemGroup(), mgId))
                    continue;

                MemTrackId id = MEM_TRACKER_MAKE_ID(mgId, mt.getContext());
                if (mt.getContext()[0] == '\0')
                {
                    AllocInfoMap::const_iterator it = mAllocInfoMap.lower_bound(id);
                    // no context specified iterate all contexts
                    for (; it != end && MEM_TRACKER_GET_MGID(it->first) == mgId; ++it)
                    {
                        GAllocInfo::GStackAllocInfoMap::const_iterator si = it->second.mStackInfoMap.begin();
                        GAllocInfo::GStackAllocInfoMap::const_iterator se = it->second.mStackInfoMap.end();
                        for (; si != se; ++si)
                        {
                            int64_t allocSize = 0;
                            int64_t allocCount = 0;
                            si->second.getAllocSizeAndCount(allocSize, allocCount, !currentAlloc);
                            if (!report.filter(allocSize, allocCount))
                                report.insert(mgId, allocSize, allocCount, si->second);
                        }
                    }
                }
                else
                {
                    AllocInfoMap::const_iterator it = mAllocInfoMap.find(id);
                    if (it != end)
                    {
                        Blaze::MemoryGroupId memgrpId = MEM_TRACKER_GET_MGID(it->first);
                        GAllocInfo::GStackAllocInfoMap::const_iterator si = it->second.mStackInfoMap.begin();
                        GAllocInfo::GStackAllocInfoMap::const_iterator se = it->second.mStackInfoMap.end();
                        for (; si != se; ++si)
                        {
                            int64_t allocSize = 0;
                            int64_t allocCount = 0;
                            si->second.getAllocSizeAndCount(allocSize, allocCount, !currentAlloc);
                            if (!report.filter(allocSize, allocCount))
                                report.insert(memgrpId, allocSize, allocCount, si->second);
                        }
                    }
                }
            }
        }
    }
    else
    {
        AllocInfoMap::const_iterator end = mAllocInfoMap.end();
        if (trackers.empty())
        {
            for (AllocInfoMap::const_iterator it = mAllocInfoMap.begin(); it != end; ++it)
            {
                int64_t allocSize = 0;
                int64_t allocCount = 0;
                it->second.getAllocSizeAndCount(allocSize, allocCount, !currentAlloc);
                if (!report.filter(allocSize, allocCount))
                    report.insert(MEM_TRACKER_GET_MGID(it->first), allocSize, allocCount, it->second);
            }
        }
        else
        {
            for (Blaze::GetMemoryMetricsRequest::MemTrackerInfoList::const_iterator i = trackers.begin(), e = trackers.end(); i != e; ++i)
            {
                const Blaze::MemTrackerInfo& mt = **i;
                Blaze::MemoryGroupId mgId = Blaze::MEM_GROUP_FRAMEWORK_DEFAULT;
                if (!AllocatorManager::parseMemoryGroupName(mt.getMemGroup(), mgId))
                    continue;
                MemTrackId id = MEM_TRACKER_MAKE_ID(mgId, mt.getContext());
                if (mt.getContext()[0] == '\0')
                {
                    AllocInfoMap::const_iterator it = mAllocInfoMap.lower_bound(id);
                    // no context specified iterate all contexts
                    for (; it != end && MEM_TRACKER_GET_MGID(it->first) == mgId; ++it)
                    {
                        int64_t allocSize = 0;
                        int64_t allocCount = 0;
                        it->second.getAllocSizeAndCount(allocSize, allocCount, !currentAlloc);
                        if (!report.filter(allocSize, allocCount))
                            report.insert(mgId, allocSize, allocCount, it->second);
                    }
                }
                else
                {
                    AllocInfoMap::const_iterator it = mAllocInfoMap.find(id);
                    if (it != end)
                    {
                        int64_t allocSize = 0;
                        int64_t allocCount = 0;
                        it->second.getAllocSizeAndCount(allocSize, allocCount, !currentAlloc);
                        if (!report.filter(allocSize, allocCount))
                            report.insert(MEM_TRACKER_GET_MGID(it->first), allocSize, allocCount, it->second);
                    }
                }
            }
        }
    }
    mLock.Unlock();

    // now it's safe to allocate memory for the response
    report.buildResponse(response);
}

void GlobalMemTracker::createPerThreadMemTracker()
{
    if (Blaze::gPerThreadMemTracker == nullptr)
    {
        // NOTE: This lock is very important because without it same per-thread tracker 
        // can get assigned to more than one thread local variables, 
        // resulting in unsafe access from different threads.
        mPerThreadTrackerListLock.Lock();
        Blaze::gPerThreadMemTracker = &mPerThreadTrackerList.push_back();
        mPerThreadTrackerListLock.Unlock();
    }
}

void GlobalMemTracker::shutdownPerThreadMemTracker()
{
    Blaze::gPerThreadMemTracker = nullptr;
}

void initPerThreadMemTracker()
{
    AllocatorManager::getInstance().getGlobalMemTracker().createPerThreadMemTracker();
}

void shutdownPerThreadMemTracker()
{
    AllocatorManager::getInstance().getGlobalMemTracker().shutdownPerThreadMemTracker();
}

PerThreadMemTracker::PerThreadMemTracker() : 
    mAllocInfoMap(Blaze::BlazeStlAllocator("PerThreadMemTracker::mAllocByMgContextMap", &AllocatorManager::getInstance().getInternalAllocator()))
{
    mCacheIndex = CACHE_ENTRY_MAX-1; // point to last entry in the cache, so that rollover will point to 0
}

PerThreadMemTracker::~PerThreadMemTracker()
{
}

void PerThreadMemTracker::trackAlloc(MemTag& tag, size_t size, Blaze::MemoryGroupId memGroupId, const char8_t* context)
{
    // NOTE: Allocation context may (rarely) be nullptr e.g. in case a 3rd party 
    // library directly calls malloc. It's a valid case that must be handled.
    const char8_t* nonNullContext = (context != nullptr) ? context : BLAZE_UNNAMED_ALLOC_NAME;
    const char8_t* validContext;
    if (MEM_TRACKER_VALIDATE_CONTEXT(nonNullContext))
    {
        validContext = nonNullContext;
    }
    else
    {
        EA_ASSERT_MSG(false, "Allocation context must be statically defined!");
        validContext = BLAZE_INVALID_ALLOC_NAME;
    }

    MemTrackId id = MEM_TRACKER_MAKE_ID(memGroupId, validContext);

    AllocInterface* ifc;
    if (Blaze::gPerThreadMemTracker != nullptr)
    {
        TAllocInfo& info = Blaze::gPerThreadMemTracker->trackAlloc(id);
        if (!info.mGAllocParent->mStackTrackingEnabled)
        {
            info.trackAlloc((int64_t)size, 1);
            ifc = &info;
        }
        else
        {
            // capture stack
            CallStack cs;
            cs.getStack(CallStack::MEM_TRACKER_FRAMES_TO_IGNORE);
            TAllocInfo::TStackAllocInfo& stackInfo = Blaze::gPerThreadMemTracker->trackAlloc(info, cs);
            stackInfo.trackAlloc((int64_t)size, 1);
            ifc = &stackInfo;
        }
    }
    else
    {
        GAllocInfo& info = AllocatorManager::getInstance().getGlobalMemTracker().trackAlloc(id);
        if (!info.mStackTrackingEnabled)
        {
            info.trackAlloc((int64_t)size, 1);
            ifc = &info;
        }
        else
        {
            // capture stack
            CallStack cs;
            cs.getStack(CallStack::MEM_TRACKER_FRAMES_TO_IGNORE);
            GAllocInfo::GStackAllocInfo& stackInfo = AllocatorManager::getInstance().getGlobalMemTracker().trackAlloc(info, cs);
            stackInfo.trackAlloc((int64_t)size, 1);
            ifc = &stackInfo;
        }
    }

    tag.mAllocInfo = ifc;
    tag.mAllocSize = size;
}

void PerThreadMemTracker::trackRealloc(MemTag& tag, size_t size, Blaze::MemoryGroupId memGroupId, const MemTag& oldTag)
{
    // add the delta by which our allocation changed
    int64_t sizeDelta = (int64_t)size - (int64_t)oldTag.mAllocSize;
    tag.mAllocInfo->trackAlloc(sizeDelta, 0);
    tag.mAllocInfo = oldTag.mAllocInfo;
    tag.mAllocSize = size;
}

void PerThreadMemTracker::trackFree(MemTag& tag)
{
    tag.mAllocInfo->trackFree((int64_t)tag.mAllocSize, 1);
}

void PerThreadMemTracker::flushTrackers()
{
    for (AllocInfoMap::iterator i = mAllocInfoMap.begin(), e = mAllocInfoMap.end(); i != e; ++i)
    {
        TAllocInfo& allocInfo = i->second;
        allocInfo.flush();
    }
}

TAllocInfo& PerThreadMemTracker::trackAlloc(MemTrackId id)
{
    uint32_t cacheIndex = mCacheIndex;
    uint32_t cacheEnd = mCacheIndex;
    do
    {
        // Walk backwards through the cache until matching tracker is found or we fall through
        if (mLastEntry[cacheIndex].mMemTrackId == id)
        {
            // NOTE: 80%+ of the time we return here without ever doing a hashmap 
            // lookup, because repated allocations tend to cluster.
            return *mLastEntry[cacheIndex].mAllocInfo;
        }
        cacheIndex = (cacheIndex - 1) & CACHE_ENTRY_MASK;
    }
    while (cacheIndex != cacheEnd);

    TAllocInfo* info;

    // We failed to find the item in the LRU, hit the data structures
    AllocInfoMap::iterator it = mAllocInfoMap.find(id);
    if (it != mAllocInfoMap.end())
    {
        info = &it->second;
    }
    else
    {
        info = &mAllocInfoMap[id];
        info->mGAllocParent = &AllocatorManager::getInstance().getGlobalMemTracker().trackAlloc(id);
    }

    cacheEnd = (cacheEnd + 1) & CACHE_ENTRY_MASK; // 
    mCacheIndex = cacheEnd; // upate cache index to point to new value
    mLastEntry[cacheEnd].mMemTrackId = id;
    mLastEntry[cacheEnd].mAllocInfo = info;

    return *info;
}

TAllocInfo::TStackAllocInfo& PerThreadMemTracker::trackAlloc(TAllocInfo& alloc, const CallStack& cs)
{
    TAllocInfo::TStackAllocInfo* info;

    TAllocInfo::TStackAllocInfoMap::iterator it = alloc.mStackInfoMap.find(cs);
    if (it != alloc.mStackInfoMap.end())
    {
        info = &it->second;
    }
    else
    {
        info = &alloc.mStackInfoMap[cs];
        info->mGStackAllocParent = &AllocatorManager::getInstance().getGlobalMemTracker().trackAlloc(*alloc.mGAllocParent, cs);
    }

    return *info;
}

