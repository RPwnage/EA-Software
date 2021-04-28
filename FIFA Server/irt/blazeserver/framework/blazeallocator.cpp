/*************************************************************************************************/
/*!
    \file

    This file implements custom allocators used by the blaze server.

    \attention
        (c) Electronic Arts Inc. All Rights Reserved.
*/
/*************************************************************************************************/


/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/blazeallocator.h"

#ifdef EA_PLATFORM_LINUX
#include <unistd.h>
#include <sys/mman.h>
#endif

#ifndef BLAZE_NODE_POOLS_ENABLED
#define BLAZE_NODE_POOLS_ENABLED 1 // use blaze node pools, if disabled, uses malloc/free instead
#endif

#ifndef BLAZE_NODE_POOL_METRICS_ENABLED
#define BLAZE_NODE_POOL_METRICS_ENABLED 1 // track node usage, if disabled, removes tracking variables
#endif

namespace Blaze
{

/**
 *  Maintains a list of slabs and a freelist of nodes from those slabs
 */
struct NodePool
{
    /**
     *  Keeps track of freed memory elements
     */
    struct Node
    {
        Node* mNext; // overlaid pointer, only used for freed nodes
    };

    /**
     *  Header for array of nodes, can be chained into a list
     */
    struct Slab
    {
        Slab* mNext; // used to chain slabs allocated for a given pool
        size_t mSize; // tracks the size of the slab (intentionally 32bit on Win32, 64bit on Linux64)
    };

    NodePool(const char8_t* name)
        : mMetricsCollection(Metrics::gFrameworkCollection->getCollection(Metrics::Tag::node_pool, name))
        , mTotalNodes(mMetricsCollection, "memory.node_pool.nodes", [this]() { return mTotalSlabSize / mNodeSize; })
        , mTotalSizeBytes(mMetricsCollection, "memory.node_pool.sizeBytes", [this]() { return mTotalSlabSize; })
        , mUnusedNodes(mMetricsCollection, "memory.node_pool.unusedNodes", [this]() { return mTotalSlabUnusedNodes; })
        , mUsedSizeBytes(mMetricsCollection, "memory.node_pool.usedSizeBytes", [this]() { return ((mTotalSlabSize / mNodeSize) - mTotalSlabUnusedNodes)*mNodeSize; })
#if BLAZE_NODE_POOL_METRICS_ENABLED
        , mReusedNodes(mMetricsCollection, "memory.node_pool.reusedNodes", [this]() { return mNodeReuseCount; })
        , mNodeSlack(mMetricsCollection, "memory.node_pool.nodeSlack", [this]() { return (uint64_t) ((100 * mNodeWasteBytes) / (double)(((mTotalSlabSize / mNodeSize) - mSlabUnusedNodes + mNodeReuseCount)*mNodeSize)); })
        , mSlabSlack(mMetricsCollection, "memory.node_pool.slabSlack", [this]() { return (uint64_t) ((100 * mSlabWasteBytes) / (double)(mTotalSlabSize)); })
#endif
    {
    }

    void initialize(uint32_t nodeSize, uint32_t slabNodeCount);
    void destroy();
    Node* allocate(size_t n, size_t pageSize);
    void deallocate(Node* node);
    int32_t logMetrics(char8_t* buf, uint32_t len) const;
    static int32_t logMetricsHeader(char8_t* buf, uint32_t len);
    void logHealthChecks(const char8_t* prefix, EA::TDF::TdfPrimitiveMap<EA::TDF::TdfString, EA::TDF::TdfString>& infoMap) const;
    static size_t getPageSize();
    static size_t roundUp(size_t n, size_t pageSize);
    static void trackAlloc(size_t n);
    uint64_t getUsedSizeBytes() const;

private:
    Metrics::MetricsCollection& mMetricsCollection;

    Metrics::PolledGauge mTotalNodes;
    Metrics::PolledGauge mTotalSizeBytes;
    Metrics::PolledGauge mUnusedNodes;
    Metrics::PolledGauge mUsedSizeBytes;
#if BLAZE_NODE_POOL_METRICS_ENABLED
    Metrics::PolledGauge mReusedNodes;
    Metrics::PolledGauge mNodeSlack;
    Metrics::PolledGauge mSlabSlack;
#endif

public:
    Node* mNode; // list of free nodes associated with the slab list
    Slab* mSlab; // list of slabs to be freed when pool is destroyed
    uint32_t mSlabNodeCount; // number of nodes per slab
    uint32_t mSlabUnusedNodes; // number of free nodes remaining in the most recent slab
    uint32_t mNodeSize; // node size in bytes
    uint64_t mTotalSlabSize; // Total size of all slabs allocated
    uint32_t mTotalSlabUnusedNodes; // number of free nodes in all slabs
#if BLAZE_NODE_POOL_METRICS_ENABLED
    uint32_t mNodeReuseCount; // number of allocations satisfied by reusing nodes
    uint32_t mNodeWasteBytes; // internal fragmentation in allocated nodes
    uint32_t mSlabWasteBytes; // internal fragmentation in allocated slabs
#endif
};

/************************************************************************/
/* NodePool methods                                                      */
/************************************************************************/
void NodePool::initialize(uint32_t nodeSize, uint32_t slabNodeCount)
{
    mNode = nullptr;
    mSlab = nullptr;
    mSlabNodeCount = slabNodeCount;
    mSlabUnusedNodes = 0;
    mNodeSize = nodeSize < sizeof(Node) ? sizeof(Node) : nodeSize;
    mTotalSlabSize = 0;
    mTotalSlabUnusedNodes = 0;
#if BLAZE_NODE_POOL_METRICS_ENABLED
    mNodeReuseCount = 0;
    mNodeWasteBytes = 0;
    mSlabWasteBytes = 0;
#endif
}

void NodePool::destroy()
{
    // free the actual memory slabs
    Slab* slab = mSlab;
    while(slab)
    {
        Slab* temp = slab;
        slab = slab->mNext;
#ifdef EA_PLATFORM_LINUX
        munmap(temp, temp->mSize);
#else
        VirtualFree(temp, 0, MEM_RELEASE);
#endif
    }
    // for safety, cheap way to avoid double delete!
    mSlab = nullptr;
}

inline NodePool::Node* NodePool::allocate(size_t n, size_t pageSize)
{
    Node* node;
    if(mNode != nullptr)
    {
        // pool freelist has nodes, grab first one
        node = mNode;
        mNode = mNode->mNext;
        --mTotalSlabUnusedNodes;
#if BLAZE_NODE_POOL_METRICS_ENABLED
        ++mNodeReuseCount;
#endif
    }
    else
    {
        // handle both [no more room in slab] and [mSlab == nullptr] conditions
        if(EA_UNLIKELY(mSlabUnusedNodes == 0))
        {
            const size_t slabSize = roundUp(mSlabNodeCount*mNodeSize+sizeof(Slab), pageSize);
#ifdef EA_PLATFORM_LINUX
            Slab* newSlab = (Slab*) mmap(0, slabSize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
#else
            Slab* newSlab = (Slab*) VirtualAlloc (0, slabSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
#endif
            newSlab->mNext = mSlab;
            newSlab->mSize = slabSize;
            mTotalSlabSize += slabSize - sizeof(Slab);
            mSlab = newSlab;
            mSlabUnusedNodes = static_cast<uint32_t>((slabSize - sizeof(Slab))/mNodeSize);
            mTotalSlabUnusedNodes += mSlabUnusedNodes;
#if BLAZE_NODE_POOL_METRICS_ENABLED
            mSlabWasteBytes += static_cast<uint32_t>(slabSize - sizeof(Slab) - mSlabUnusedNodes*mNodeSize);
#endif
        }

        //lint -save -e613 Lint gets confused by the next line
        node = (Node*)((char8_t*)mSlab + sizeof(Slab) + mNodeSize * --mSlabUnusedNodes);
        //lint -restore

        --mTotalSlabUnusedNodes;
    }
#if BLAZE_NODE_POOL_METRICS_ENABLED
    mNodeWasteBytes += (mNodeSize - n);
#endif
    return node;
}

inline void NodePool::deallocate(NodePool::Node* node)
{
    // add node to pool's freelist
    node->mNext = mNode;
    mNode = node;
    ++mTotalSlabUnusedNodes;
#ifdef EA_PLATFORM_WINDOWS
    //on windows, overwrite freed memory with garbage
    memset(node + 1, 0xFE, mNodeSize - sizeof(Node));
#endif
}

int32_t NodePool::logMetrics(char8_t* buf, uint32_t len) const
{
    // to compute internal fragmentation ratio use:
    // 100*mNodeWasteBytes/(double)((mNodeNewCount+mNodeReuseCount)*mNodeSize)
    if(mSlab != nullptr)
    {
        uint32_t totalSlabNodes = 0;
        totalSlabNodes = static_cast<uint32_t>(mTotalSlabSize/mNodeSize);

        return blaze_snzprintf(
            buf, len,
            "\n%8d %10u %11d %11d %7.1f %% %7.1f %%",
            mNodeSize, totalSlabNodes, mSlabUnusedNodes,
#if BLAZE_NODE_POOL_METRICS_ENABLED
            mNodeReuseCount,
            (100*mNodeWasteBytes)/(double)((totalSlabNodes-mSlabUnusedNodes+mNodeReuseCount)*mNodeSize),
            (100*mSlabWasteBytes)/(double)(mTotalSlabSize)
#else
            0, 0, 0
#endif
        );
    }
    return 0;
}

int32_t NodePool::logMetricsHeader(char8_t* buf, uint32_t len)
{
    return blaze_snzprintf(
        buf, len,
        "\n---------------------------------------------------------------------"
        "\nNodeSize TotalNodes UnusedNodes ReusedNodes NodeSlack SlabSlack"
        "\n---------------------------------------------------------------------"
    );
}

void NodePool::logHealthChecks(const char8_t* prefix, EA::TDF::TdfPrimitiveMap<EA::TDF::TdfString, EA::TDF::TdfString>& infoMap) const
{
    uint32_t totalSlabNodes = 0;
    totalSlabNodes = static_cast<uint32_t>(mTotalSlabSize/mNodeSize);

    char8_t key[128];
    char8_t buf[64];

    blaze_snzprintf(key, sizeof(key), "%s_TotalNodes", prefix);
    blaze_snzprintf(buf, sizeof(buf), "%u", totalSlabNodes);
    infoMap[key] = buf;

    blaze_snzprintf(key, sizeof(key), "%s_TotalSizeBytes", prefix);
    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mTotalSlabSize);
    infoMap[key] = buf;

    blaze_snzprintf(key, sizeof(key), "%s_UnusedNodes", prefix);
    blaze_snzprintf(buf, sizeof(buf), "%d", mTotalSlabUnusedNodes);
    infoMap[key] = buf;

    uint64_t usedSize = 0;
    usedSize = (totalSlabNodes-mTotalSlabUnusedNodes)*mNodeSize;
    blaze_snzprintf(key, sizeof(key), "%s_UsedSizeBytes", prefix);
    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, usedSize);
    infoMap[key] = buf;
}

uint64_t NodePool::getUsedSizeBytes() const
{
    uint32_t totalSlabNodes = 0;
    totalSlabNodes = static_cast<uint32_t>(mTotalSlabSize/mNodeSize);
    return (totalSlabNodes-mTotalSlabUnusedNodes) * mNodeSize;
}

inline size_t NodePool::getPageSize()
{
#ifdef EA_PLATFORM_LINUX
    return static_cast<size_t>(sysconf(_SC_PAGESIZE));
#else
    SYSTEM_INFO sysInfo;         // Useful information about the system
    GetSystemInfo(&sysInfo);     // Initialize the structure.
    return sysInfo.dwPageSize;
#endif
}

inline size_t NodePool::roundUp(size_t n, size_t pageSize)
{
    return ((n-1)/pageSize+1)*pageSize;
}

// these are *best guess* estimates for the number of nodes per slab allocation.
// if a different number of nodes is optimal, it should be specified in the pool ctor.
const static size_t DEFAULT_POOL_ALLOCATOR_NODES_PER_SLAB = 4096;
const static size_t DEFAULT_RNG_POOL_ALLOCATOR_NODES_PER_SLAB = 256;

/************************************************************************/
/* NodePoolAllocator methods                                                */
/************************************************************************/

NodePoolAllocator::NodePoolAllocator(const char8_t* name, uint32_t nodeSize, MemoryGroupId memGroupId, uint32_t nodesPerSlab)
{
    mPageSize = NodePool::getPageSize();
    blaze_strnzcpy(mName, (name != nullptr && name[0] != '\0') ? name : "NodePoolAllocator", sizeof(mName));
    nodesPerSlab = nodesPerSlab > 0 ? nodesPerSlab : NodePool::roundUp(DEFAULT_POOL_ALLOCATOR_NODES_PER_SLAB*nodeSize+sizeof(NodePool::Slab), mPageSize)/nodeSize;
    mPool = BLAZE_NEW_MGID(memGroupId, "NodePool") NodePool(mName);
    mPool->initialize(nodeSize, nodesPerSlab);
    mMemGroupId = memGroupId;
}

NodePoolAllocator::~NodePoolAllocator()
{
    mPool->destroy();
    delete mPool;
}

void* NodePoolAllocator::allocate(size_t n) // TODO: inline
{
#if BLAZE_NODE_POOLS_ENABLED
    return mPool->allocate(n, mPageSize);
#else
    return BLAZE_ALLOC_MGID(n, mMemGroupId, mName);
#endif
}

void NodePoolAllocator::deallocate(void* node) // TODO: inline
{
#if BLAZE_NODE_POOLS_ENABLED
    mPool->deallocate((NodePool::Node*)node);
#else
    BLAZE_FREE(node);
#endif
}

void NodePoolAllocator::logMetrics() const
{
    const uint32_t size = 4096;
    char8_t* buf = BLAZE_NEW_ARRAY_MGID(char8_t, size, mMemGroupId, "NodePoolLog");
    int32_t written = NodePool::logMetricsHeader(buf, size);
    written += mPool->logMetrics(buf + written, size - written);
    BLAZE_TRACE_LOG(Log::SYSTEM,
        "[NodePoolAllocator].logMetrics:\n\nallocator = '" << mName << "' pageSize = " << (static_cast<uint32_t>(mPageSize)) << buf);
    delete[] buf;
}

void NodePoolAllocator::logHealthChecks(EA::TDF::TdfPrimitiveMap<EA::TDF::TdfString, EA::TDF::TdfString>& infoMap, const char8_t* suffix) const
{
    char8_t buf[sizeof(mName)*2];
    blaze_snzprintf(buf, sizeof(buf), "%s%s", mName, suffix);
    mPool->logHealthChecks(buf, infoMap);
}

uint64_t NodePoolAllocator::getUsedSizeBytes() const 
{ 
    return (mPool == nullptr) ? 0 : mPool->getUsedSizeBytes();

}


/************************************************************************/
/* NodePoolRangeAllocator methods                                                 */
/************************************************************************/

NodePoolRangeAllocator::NodePoolRangeAllocator(const char8_t* name, uint32_t poolCount, uint32_t poolGranularity, MemoryGroupId memGroupId, uint32_t nodesPerSlab)
    : mPoolCount(poolCount)
    , mPoolGranularity(poolGranularity)
    , mMemGroupId(memGroupId)
{
    blaze_strnzcpy(mName, (name != nullptr && name[0] != '\0') ? name : "NodePoolRangeAllocator", sizeof(mName));
    mPools = BLAZE_NEW_ARRAY_MGID(NodePool*, mPoolCount, mMemGroupId, "NodePoolRangeAllocatorPools");
    mPageSize = NodePool::getPageSize();
    // initialize pool node sizes
    for(uint32_t i = 0; i < mPoolCount; ++i)
    {
        eastl::string poolName(mName);
        poolName.append_sprintf(".%u", i);
        mPools[i] = BLAZE_NEW_MGID(memGroupId, "NodePoolRangeAllocatorPools") NodePool(poolName.c_str());
        uint32_t nodeSize = mPoolGranularity*(i+1);
        uint32_t nodes = nodesPerSlab > 0 ? nodesPerSlab : NodePool::roundUp(DEFAULT_RNG_POOL_ALLOCATOR_NODES_PER_SLAB*nodeSize+sizeof(NodePool::Slab), mPageSize)/nodeSize;
        mPools[i]->initialize(nodeSize, nodes);
    }
}

NodePoolRangeAllocator::~NodePoolRangeAllocator()
{
    // destroy() can be safely called multiple times
    destroy();
}

void* NodePoolRangeAllocator::allocate(size_t n) // TODO: inline
{
#if BLAZE_NODE_POOLS_ENABLED
    const size_t index = (n - 1) / mPoolGranularity;
    if(EA_LIKELY(index < mPoolCount))
    {
        return mPools[index]->allocate(n, mPageSize);
    }
#endif
    return BLAZE_ALLOC(n);
}

void NodePoolRangeAllocator::deallocate(void* p, size_t n) // TODO: inline
{
#if BLAZE_NODE_POOLS_ENABLED
    const size_t index = (n - 1) / mPoolGranularity;
    if(EA_LIKELY(index < mPoolCount))
    {
        mPools[index]->deallocate((NodePool::Node*) p);
        return;
    }
#endif
    BLAZE_FREE(p);
}

void NodePoolRangeAllocator::logMetrics() const
{
    const uint32_t size = 4096;
    char8_t* buf = BLAZE_NEW_ARRAY_MGID(char8_t, size, mMemGroupId, "NodePoolRangeAllocatorLog");
    int32_t written = NodePool::logMetricsHeader(buf, size);
    for(uint32_t i = 0; i < mPoolCount; ++i)
    {
        written += mPools[i]->logMetrics(buf + written, size - written);
    }
    BLAZE_TRACE_LOG(Log::SYSTEM,
        "[NodePoolRangeAllocator].logMetrics:\n\nallocator = '" << mName << "' pageSize = " << (static_cast<uint32_t>(mPageSize)) << buf);
    delete[] buf;
}

void NodePoolRangeAllocator::destroy()
{
    if(mPools != nullptr)
    {
        //free memory associated with the individual pools
        for(uint32_t i = 0; i < mPoolCount; ++i)
        {
            mPools[i]->destroy();
            delete mPools[i];
        }

        delete[] mPools;

        // set to nullptr to allow destroy() to be called multiple times
        mPools = nullptr;
    }
}

} // Blaze

