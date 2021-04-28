/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/replication/replicateditempool.h"

namespace Blaze
{

ReplicatedItemPoolCollection::ReplicatedItemPoolCollection(MemoryGroupId memoryId)
: mMemoryId(memoryId),
    mItemPoolMap(BlazeStlAllocator("ReplicatedItemPoolCollection::mItemPoolMap"))
{
}

ReplicatedItemPoolCollection::~ReplicatedItemPoolCollection()
{
    for (ItemPoolMap::iterator i = mItemPoolMap.begin(), e = mItemPoolMap.end(); i != e; ++i)
        delete i->second;
}

EA::Allocator::FixedAllocatorBase& ReplicatedItemPoolCollection::createPoolAllocator(size_t objectSize)
{
    ItemPoolMap::insert_return_type result = mItemPoolMap.insert(eastl::make_pair(objectSize, (EA::Allocator::FixedAllocatorBase*)nullptr));
    if (result.second)
    {
        result.first->second = BLAZE_NEW EA::Allocator::FixedAllocatorBase(objectSize, 
            0, 128, nullptr, SIZE_MAX, &ReplicatedItemPoolCollection::poolAllocFunc, &ReplicatedItemPoolCollection::poolFreeFunc, this);
    }
    return *result.first->second;
}

void* ReplicatedItemPoolCollection::poolAllocFunc(size_t size, void* _this)
{
    return BLAZE_ALLOC_MGID(size, ((ReplicatedItemPoolCollection*)_this)->mMemoryId, "ReplicatedItemBlock");
}

void ReplicatedItemPoolCollection::poolFreeFunc(void* mem, void*)
{
    BLAZE_FREE(mem);
}

}


