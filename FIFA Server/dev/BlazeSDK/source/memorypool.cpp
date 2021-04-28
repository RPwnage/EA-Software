/*! ***************************************************************************/
/*!
\file
Source file for allocation routines.


\attention
(c) Electronic Arts. All Rights Reserved.

*******************************************************************************/
#include "framework/blaze.h"
#include <string.h> //for memset
#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/memorypool.h"

namespace Blaze
{


struct MemNode
{
    MemNode* mNext;
};


/*! \brief      Initializes the container. Until a reserved memory area is allocated by calling
                ::reserve(), the specified allocator will be used for all allocations.
                
                NOTE: All memory obtained outside the reserved memory area will be freed to the
                system immediately upon the call to ::free(). This way the pool will not pin any
                memory that fell outside of the budget of the reserved memory area.
    
    \param[in]  memGroupId  memory group id to use for allocations
    \return     
*/
MemNodeList::MemNodeList(MemoryGroupId memGroupId)
: mFree(nullptr), mHead(nullptr), mTail(nullptr), mAllocator(Allocator::getAllocator(memGroupId))
{
}


/*! \brief      Frees the reserved memory area. Callers of ::alloc() are responsible for calling
                ::free() on each object obtained from this container prior to destructor execution.
    
    \return     
*/
MemNodeList::~MemNodeList()
{
    CORE_FREE(mAllocator, mHead);
}

/*! \brief      Allocates the reserved memory area used by subsequent calls to ::alloc()
                It is safe to call this method multiple times because repeated calls are ignored.
    
    \param[in]  nodeCount   number of nodes to store in the reserved memory area
    \param[in]  nodeSize    size of node stored in the reserved memory area must be >= sizeof(MemNode)
    \param[in]  debugName   debug string passed to the allocator when allocating the reserved memory area
    \return     void
*/
void MemNodeList::reserve(size_t nodeCount, size_t nodeSize, const char8_t* debugName)
{
    BlazeAssert(mHead == nullptr);
    size_t memSize = nodeCount * nodeSize;
    if(mHead == nullptr && memSize > 0)
    {
        mHead = (MemNode*) CORE_ALLOC(mAllocator, memSize, debugName, EA::Allocator::MEM_PERM);
        if(mHead != nullptr)
        {
            // tail points to last valid node
            mTail = (MemNode*) ((uintptr_t) mHead + memSize - nodeSize);
            mFree = mHead;
            // walk through the memory and create a simple linked list within it.
            while(mFree < mTail)
            {
                mFree->mNext = (MemNode*) ((uintptr_t) mFree + nodeSize);
                mFree = mFree->mNext;
            }
            mFree->mNext = nullptr;
            mFree = mHead;
        }
    }
}

/*! \brief      Allocates a new memory block from the freelist. 
                If the freelist is empty, uses the allocator.
    
    \param[in]  nodeSize    size of node to allocate (must be same as nodeSize parameter of ::reserve())
    \param[in]  debugName   debug string passed to allocator when a node is allocated outside the reserved memory area
    \return     void*
*/
void* MemNodeList::alloc(size_t nodeSize, const char8_t* debugName)
{
    void* node;

    if(mFree != nullptr)
    {
        // if there is a node in the free list, take it.
        node = mFree;
        mFree = mFree->mNext;
    }
    else
    {
        // no space left in the pool
        node = CORE_ALLOC(mAllocator, nodeSize, debugName, EA::Allocator::MEM_PERM);
    }

    return node;
}


/*! \brief      If the node address falls withing the reserved memory area add it
                to the freelist, otherwise, free immediately using allocator.
    
    \param[in]  node    pointer to memory allocated with this container
    \return     void
*/
void MemNodeList::free(void* node)
{
    if(mHead != nullptr && mTail != nullptr && node >= mHead && node <= mTail)
    {
        static_cast<MemNode*>(node)->mNext = mFree;
        mFree = static_cast<MemNode*>(node);
    }
    else
    {
        CORE_FREE(mAllocator, node);
    }
}

} // Blaze
