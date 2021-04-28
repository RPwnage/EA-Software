/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/system/allocator/coremanager.h"
#include "framework/util/rawbufferpool.h"


namespace Blaze
{

const uint32_t RawBufferPool::DEFAULT_POOL_SIZE = 0; // minimum pool size by default, since pool allocates in chunks we rely on unused elements in the chunk to reduce system calls during frequent allocation

RawBufferPool::RawBufferPool(uint32_t poolMax)
    : mPoolBlockCount(0), mPoolBlockMax(poolMax > 0 ? poolMax : DEFAULT_POOL_SIZE), mUsedChunkCount(0), mAllocatedChunkCount(0), mMaxAllocatedChunkCount(0)
    , mMetricsCollection(Metrics::gFrameworkCollection->getCollection(Metrics::Tag::memory_category, (const char8_t*)"framework").getCollection(Metrics::Tag::memory_subcategory, (const char8_t*)"rawbufpool"))
    , mUsedMemory(mMetricsCollection, "memory.usedMemory", [this]() { return (uint64_t)getUsedMemory(); })
    , mAllocatedMemory(mMetricsCollection, "memory.allocatedCoreMemory", [this]() { return (uint64_t)getAllocatedMemory(); })
    , mMaxAllocatedMemory(mMetricsCollection, "memory.maxUsedMemory", [this]() { return (uint64_t)getMaxAllocatedMemory(); })
{
    EA_ASSERT(gRawBufferPool == nullptr);
#if RAW_BUFFER_POOL_DETAIL_METRICS
    memset(mAllocCount, 0, sizeof(mAllocCount));
    memset(mFreeCount, 0, sizeof(mFreeCount));
#endif
}

RawBufferPool::~RawBufferPool()
{
    for (size_t i = 0; i < EAArrayCount(mBlockListArray); ++i)
    {
        RawBufferBlockList& blockList = mBlockListArray[i];
        while (!blockList.empty())
        {
            RawBufferBlock* block = &blockList.front();
            EA_ASSERT(block->mFreeChunks.count() == RawBufferBlock::CHUNK_COUNT_MAX);
            blockList.pop_front();
            OSFunctions::freeCore(block, sizeof(RawBufferBlock));
        }
    }
    gRawBufferPool = nullptr;
}

size_t RawBufferPool::getUsedMemory() const
{
    return mUsedChunkCount * sizeof(RawBufferBlock);
}

size_t RawBufferPool::getAllocatedMemory() const
{
    return mAllocatedChunkCount * sizeof(RawBufferBlock);
}

size_t RawBufferPool::getMaxAllocatedMemory() const
{
    return mMaxAllocatedChunkCount * sizeof(RawBufferBlock);
}

void* RawBufferPool::allocate()
{
    RawBufferBlock* block;
    uint32_t chunkIndex;

    if (!mBlockListArray[LIST_STUFFED].empty())
    {
        // prefer using a nearly filled block
        block = &mBlockListArray[LIST_STUFFED].front();
        chunkIndex = static_cast<uint32_t>(block->mFreeChunks.find_first());
        block->mFreeChunks.set(chunkIndex, false); // mark unavailable
        RawBufferBlockList::remove(*block);
        block->mpNext = block->mpPrev = nullptr;
#if RAW_BUFFER_POOL_DETAIL_METRICS
        ++mAllocCount[ALLOC_STUFFED];
#endif
    }
    else if (!mBlockListArray[LIST_REGULAR].empty())
    {
        // use a less filled block
        block = &mBlockListArray[LIST_REGULAR].front();
        chunkIndex = static_cast<uint32_t>(block->mFreeChunks.find_first());
        block->mFreeChunks.set(chunkIndex, false); // mark unavailable
        size_t freeChunks = static_cast<size_t>(block->mFreeChunks.count());
        if (freeChunks == 1)
        {
            // remove block from the regular list
            RawBufferBlockList::remove(*block);
            // push to the front of the stuffed list as it might be used again soon, and objects allocated together tend to get freed together
            mBlockListArray[LIST_STUFFED].push_front(*block);
#if RAW_BUFFER_POOL_DETAIL_METRICS
            ++mAllocCount[ALLOC_REGULAR_STUFFED];
#endif
        }
#if RAW_BUFFER_POOL_DETAIL_METRICS
        else
            ++mAllocCount[ALLOC_REGULAR];
#endif
    }
    else
    {
        block = (RawBufferBlock*) OSFunctions::commitCore(nullptr, sizeof(RawBufferBlock));
        if (block == nullptr)
            return nullptr;
        block->mFreeChunks.reset(); // set all to '0' == used
        block->mFreeChunks.flip(); // set all to '1' == available
        for (size_t i = 0; i < EAArrayCount(block->mChunks); ++i)
            block->mChunks[i].mParentBlock = block;
        chunkIndex = 0;
        block->mFreeChunks.set(chunkIndex, false); // mark unavailable
        // push to the front of the regular list since all lists are empty right now anyway
        mBlockListArray[LIST_REGULAR].push_front(*block);
        ++mPoolBlockCount;
        mAllocatedChunkCount += RawBufferBlock::CHUNK_COUNT_MAX;
        if (mMaxAllocatedChunkCount < mAllocatedChunkCount)
            mMaxAllocatedChunkCount = mAllocatedChunkCount;
#if RAW_BUFFER_POOL_DETAIL_METRICS
        ++mAllocCount[ALLOC_POOL];
#endif
    }

    ++mUsedChunkCount;

    RawBufferChunk* chunk = block->mChunks + chunkIndex;
    return chunk;
}

void RawBufferPool::release(void* buf)
{
    RawBufferChunk* chunk = (RawBufferChunk*) buf;
    RawBufferBlock* parentBlock = chunk->mParentBlock;
    const size_t freeChunks = static_cast<size_t>(parentBlock->mFreeChunks.count());
    const uint32_t chunkIndex = static_cast<uint32_t>(chunk - parentBlock->mChunks);
    parentBlock->mFreeChunks.set(chunkIndex, true); // mark available

    --mUsedChunkCount;

    // common case, we are in the regular list with more chunks yet to free
    if (freeChunks > 1 && freeChunks + 1 < RawBufferBlock::CHUNK_COUNT_MAX)
    {
#if RAW_BUFFER_POOL_DETAIL_METRICS
        ++mFreeCount[FREE_COMMON];
#endif
        return;
    }

    if (freeChunks == 0)
    {
        EA_ASSERT(parentBlock->mpNext == nullptr && parentBlock->mpPrev == nullptr);
        // we had no free chunks, add us to the *back* of stuffed list to give us a chance to free more blocks
        mBlockListArray[LIST_STUFFED].push_back(*parentBlock);
#if RAW_BUFFER_POOL_DETAIL_METRICS
        ++mFreeCount[FREE_0];
#endif
    }
    else if (freeChunks == 1)
    {
        EA_ASSERT(parentBlock->mpNext != nullptr && parentBlock->mpPrev != nullptr);
        // remove block from the stuffed list
        RawBufferBlockList::remove(*parentBlock);
        // push to the *back* of the regular list to give us a chance to free more blocks
        mBlockListArray[LIST_REGULAR].push_back(*parentBlock);
#if RAW_BUFFER_POOL_DETAIL_METRICS
        ++mFreeCount[FREE_1];
#endif
    }
    else if (mPoolBlockCount > mPoolBlockMax)
    {
        // remove block from whichever list
        RawBufferBlockList::remove(*parentBlock);
        // all chunks freed, shrink the pool
        OSFunctions::freeCore(parentBlock, sizeof(RawBufferBlock));
        --mPoolBlockCount;
        mAllocatedChunkCount -= RawBufferBlock::CHUNK_COUNT_MAX;
#if RAW_BUFFER_POOL_DETAIL_METRICS
        ++mFreeCount[FREE_POOL];
#endif
    }
#if RAW_BUFFER_POOL_DETAIL_METRICS
    else
        ++mFreeCount[FREE_KEEP];
#endif
}

//
// NOTE: The RawBuffer new/delete operators are implemented here because they are not used on the BLAZE_SDK
//
void* RawBuffer::operator new(size_t size)
{
    EA_ASSERT(size < sizeof(RawBufferChunk));
    if (gRawBufferPool != nullptr)
    {
        void* mem = gRawBufferPool->allocate();
        if (mem != nullptr)
            return mem;
    }
    RawBufferChunk* chunk = (RawBufferChunk*) BLAZE_ALLOC_MGID(sizeof(RawBufferChunk), MEM_GROUP_FRAMEWORK_RAWBUF, "RAWBUFFER_DEFAULT");
    chunk->mParentBlock = nullptr;
    return chunk;
}

void RawBuffer::operator delete(void *p)
{
    if (p != nullptr)
    {
        RawBufferChunk* chunk = (RawBufferChunk*) p;
        RawBufferBlock* parentBlock = chunk->mParentBlock;
        if (parentBlock != nullptr)
        {
            if (gRawBufferPool != nullptr)
            {
                gRawBufferPool->release(p);
                return;
            }
            EA_FAIL_MSG("Chunk with parent must come from the pool!");
        }
        BLAZE_FREE(p);
    }
}

} // namespace Blaze

