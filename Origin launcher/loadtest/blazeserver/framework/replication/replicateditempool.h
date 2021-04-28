/*! ************************************************************************************************/
/*!
    \file replicateditempool.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_REPLICATED_ITEM_POOL_H
#define BLAZE_REPLICATED_ITEM_POOL_H

/*** Include files *******************************************************************************/

#include "EASTL/hash_map.h"
#include "PPMalloc/EAFixedAllocator.h"
#include "framework/replication/replication.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class ReplicatedItemPoolCollection
{
public:
    ReplicatedItemPoolCollection(MemoryGroupId memoryId);
    ~ReplicatedItemPoolCollection();

    EA::Allocator::FixedAllocatorBase& createPoolAllocator(size_t objectSize);

private:
    static void* poolAllocFunc(size_t size, void* _this);
    static void poolFreeFunc(void* mem, void*);
private:
    typedef eastl::hash_map<size_t, EA::Allocator::FixedAllocatorBase*> ItemPoolMap;
    MemoryGroupId mMemoryId;    // The memory group id we create memory with
    ItemPoolMap mItemPoolMap;   // The map of pool allocators by keyed by sizeof(ItemType)
};

} // Blaze

#endif // BLAZE_REPLICATED_ITEM_POOL_H

