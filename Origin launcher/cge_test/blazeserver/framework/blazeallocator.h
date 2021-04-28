/*************************************************************************************************/
/*!
    \file

    This file defines custom allocators used by the blaze server.

    \attention
        (c) Electronic Arts Inc. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_ALLOCATOR_H
#define BLAZE_ALLOCATOR_H

/*** Include Files *******************************************************************************/

#include "framework/metrics/metrics.h"

namespace Blaze
{

struct NodePool;

/**
 *  Freelist based node allocator.
 */
class NodePoolAllocator
{
    NON_COPYABLE(NodePoolAllocator);

public:
    NodePoolAllocator(const char8_t* name, uint32_t nodeSize, MemoryGroupId memGroupId, uint32_t nodesPerSlab = 0);
    ~NodePoolAllocator();

    void* allocate(size_t n);
    void deallocate(void* node);
    void logMetrics() const;
    // We use the base type of Blaze::ComponentStatus::InfoMap to keep this class more generic
    void logHealthChecks(EA::TDF::TdfPrimitiveMap<EA::TDF::TdfString, EA::TDF::TdfString>& infoMap, const char8_t* suffix = "") const;

    uint64_t getUsedSizeBytes() const;

private:
    NodePool* mPool;
    size_t mPageSize;
    char8_t mName[64];
    MemoryGroupId mMemGroupId;
};

/**
 *  NodePoolRangeAllocator
 */
class NodePoolRangeAllocator
{
    NON_COPYABLE(NodePoolRangeAllocator);
public:
    NodePoolRangeAllocator(const char8_t* name, uint32_t poolCount, uint32_t poolGranularity, MemoryGroupId memGroupId, uint32_t nodesPerSlab = 0);
    virtual ~NodePoolRangeAllocator();
    void* allocate(size_t n);
    void deallocate(void* p, size_t n);
    void logMetrics() const;
    
protected:
    void destroy();

protected:
    NodePool** mPools;
    uint32_t mPoolCount;
    uint32_t mPoolGranularity;
    size_t mPageSize;
    char8_t mName[64];
    MemoryGroupId mMemGroupId;
};

} // Blaze

#endif // BLAZE_ALLOCATOR_H



