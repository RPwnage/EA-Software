/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef RAWBUFFERPOOL_H
#define RAWBUFFERPOOL_H

/*** Include files *******************************************************************************/

#include "EASTL/bitset.h"
#include "EASTL/intrusive_list.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/metrics/metrics.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#define RAW_BUFFER_POOL_DETAIL_METRICS 0

namespace Blaze
{

struct RawBufferBlock;

struct RawBufferChunk
{
    char8_t mData[sizeof(RawBuffer)];
    RawBufferBlock* mParentBlock;
};

struct RawBufferBlock : public eastl::intrusive_list_node
{
    static const uint32_t CHUNK_COUNT_MAX = 8;
    typedef eastl::bitset<CHUNK_COUNT_MAX> FreeChunkBitset;
    FreeChunkBitset mFreeChunks;
    RawBufferChunk mChunks[CHUNK_COUNT_MAX];
};

class RawBufferPool
{
    NON_COPYABLE(RawBufferPool);
public:
    static const uint32_t DEFAULT_POOL_SIZE;

    RawBufferPool(uint32_t poolMax = 0);
    ~RawBufferPool();
    
    size_t getUsedMemory() const;
    size_t getAllocatedMemory() const;
    size_t getMaxAllocatedMemory() const;

private:
    friend class RawBuffer;

    enum BlockListType
    {
        LIST_STUFFED,
        LIST_REGULAR,
        LIST_MAX // Sentinel, do not use
    };

    typedef eastl::intrusive_list<RawBufferBlock> RawBufferBlockList;
    RawBufferBlockList mBlockListArray[LIST_MAX];
    uint32_t mPoolBlockCount;
    uint32_t mPoolBlockMax;
    uint32_t mUsedChunkCount;
    uint32_t mAllocatedChunkCount;
    uint32_t mMaxAllocatedChunkCount;

    Metrics::MetricsCollection& mMetricsCollection;
    Metrics::PolledGauge mUsedMemory;
    Metrics::PolledGauge mAllocatedMemory;
    Metrics::PolledGauge mMaxAllocatedMemory;

#if RAW_BUFFER_POOL_DETAIL_METRICS
    enum AllocType
    {
        ALLOC_STUFFED,
        ALLOC_REGULAR,
        ALLOC_REGULAR_STUFFED,
        ALLOC_POOL,
        ALLOC_MAX
    };

    enum FreeType
    {
        FREE_COMMON,
        FREE_0,
        FREE_1,
        FREE_POOL,
        FREE_KEEP,
        FREE_MAX
    };

    uint64_t mAllocCount[ALLOC_MAX];
    uint64_t mFreeCount[FREE_MAX];
#endif

    void release(void* buf);
    void* allocate();
};

extern EA_THREAD_LOCAL RawBufferPool* gRawBufferPool;

} // Blaze

#endif // RAWBUFFERPOOL_H

