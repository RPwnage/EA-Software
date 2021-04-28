#include "..\MantleAppWsi.h"
#include "GpuMemMgr.h"
#include "ConstantBufferMgr.h"
#include "Defines.h"

ConstantBufferMgr::ConstantBufferMgr():
    m_maxAllocatedPages(0),
    m_numAllocatedPages(0),
    m_allocatedPages(NULL),
    m_unusedBlockDesc(NULL),
	m_gpu(0)
{
}

ConstantBufferMgr::~ConstantBufferMgr()
{
}

void ConstantBufferMgr::init()
{

}

void ConstantBufferMgr::deinit( )
{
    eastl::vector<MemBlock*>::iterator itb = m_vecBlockDesc.begin();
    eastl::vector<MemBlock*>::iterator ite = m_vecBlockDesc.end();
    for( eastl::vector<MemBlock*>::iterator it = itb; it!=ite; ++it )
    {
        delete[] *it;
    }
    m_vecBlockDesc.clear();
}

GR_RESULT ConstantBufferMgr::allocate(GR_FLAGS heapFlagFilter, GR_GPU_SIZE memorySize, GR_GPU_MEMORY* memory, GR_GPU_SIZE* offset, GR_UINT gpu)
{
	m_gpu = gpu;
    GR_RESULT result = GR_ERROR_UNKNOWN;

    GR_UINT numHeaps = GpuMemMgr::Instance(m_gpu)->getHeapCount();

    GR_MEMORY_ALLOC_INFO allocInfo = {};
    allocInfo.memPriority = GR_MEMORY_PRIORITY_NORMAL;
    allocInfo.heapCount = 0;
    GR_UINT heaps = 0;

    // select heap
    for (GR_UINT i = 0; i < numHeaps; ++i)
    {
        const GR_MEMORY_HEAP_PROPERTIES& heapInfo = GpuMemMgr::Instance(m_gpu)->getHeapInfo(i);

        if (heapInfo.flags & heapFlagFilter)
        {
            // else allocate a page (at least) of memory on that heap (ideally select a common multiple)
            {
                //GR_GPU_SIZE size = max(memblock->size, (memblock->size/(memblock->size & ~(memblock->size-1))) * heapInfo.pageSize );
                GR_GPU_SIZE size = ((memorySize + heapInfo.pageSize - 1) & ~(heapInfo.pageSize - 1));
                allocInfo.size = eastl::max(allocInfo.size, size);
                allocInfo.heaps[allocInfo.heapCount] = i;
                heaps |= 1<<i;
                ++allocInfo.heapCount;
            }
        }
    }

    // check if we have any free memblocks for a CB of that size already allocated
    result = searchAllocatedPages( heaps, memorySize, memory, offset );

    // else allocate a new memory page
    if( result != GR_SUCCESS)
    {
        if( m_numAllocatedPages == m_maxAllocatedPages )
        {
            m_maxAllocatedPages += 32;
            m_allocatedPages = (MemPage*)realloc(m_allocatedPages, m_maxAllocatedPages*sizeof(MemPage));
        }
        
        MemPage* page = &m_allocatedPages[m_numAllocatedPages];
        ++m_numAllocatedPages;

        result = _grAllocMemory(OriginIGO::getMantleDevice(m_gpu), &allocInfo, memory);
        if (result == GR_SUCCESS)
        {
            memset( page, 0, sizeof(MemPage) );
            page->heaps = heaps;
            page->size = allocInfo.size;
            page->ptr = *memory;

            if( NULL == m_unusedBlockDesc )
            {
                //realloc m_blockDesc
                m_unusedBlockDesc = new MemBlock[128];
                m_vecBlockDesc.push_back(m_unusedBlockDesc); // keep track of allocated memory

                for( int i = 0; i<128; ++i)
                {
                    m_unusedBlockDesc[i].prev = &m_unusedBlockDesc[i-1];
                    m_unusedBlockDesc[i].next = &m_unusedBlockDesc[i+1];
                }
                m_unusedBlockDesc[0].prev = NULL;
                m_unusedBlockDesc[127].next = NULL;
            }

            // add memblock to list of allocated memblocks
            page->free = m_unusedBlockDesc;
            m_unusedBlockDesc = m_unusedBlockDesc->next;
            if(m_unusedBlockDesc)
            {
                m_unusedBlockDesc->prev = NULL;
            }

            page->free->offset = 0;
            page->free->size = allocInfo.size;
            page->free->next = NULL;
            page->free->prev = NULL;

            // reserve one memblock for the requested data
            return allocateFromBlock( page, page->free, memorySize, memory, offset);
        }
    }

    return result;
}

void ConstantBufferMgr::deallocate(GR_GPU_MEMORY memblock, GR_GPU_SIZE offset, GR_CMD_BUFFER cmd)
{
    for( unsigned int page = 0; page<m_numAllocatedPages; ++page)
    {
        if(m_allocatedPages[page].ptr == memblock)
        {
            MemBlock* usedBlock = m_allocatedPages[page].used;
            while( usedBlock )
            {
                if( usedBlock->offset == offset )
                {
                    // remove from used list
                    if(usedBlock->prev)
                    {
                        usedBlock->prev->next = usedBlock->next;
                    }
                    else
                    {
                        m_allocatedPages[page].used = usedBlock->next;
                    }
                    if(usedBlock->next)
                    {
                        usedBlock->next->prev = usedBlock->prev;
                    }
                    
                    // add to free list
                    if( m_allocatedPages[page].free == NULL )
                    {
                        m_allocatedPages[page].free = usedBlock;
                    }
                    else
                    {
                        MemBlock* prevFreeBlock = NULL;
                        MemBlock* freeBlock = m_allocatedPages[page].free;
                        while(freeBlock && (freeBlock->offset < usedBlock->offset) )
                        {
                            prevFreeBlock = freeBlock;
                            freeBlock = freeBlock->next;
                        }
                        if( prevFreeBlock == NULL )
                        {
                            usedBlock->next = m_allocatedPages[page].free;
                            usedBlock->prev = NULL;
                            if( NULL != usedBlock->next)
                            {
                                usedBlock->next->prev = usedBlock;
                            }
                            m_allocatedPages[page].free = usedBlock;
                            
                            //mergeBlocks( &m_allocatedPages[page].free );
                        }
                        else
                        {
                            usedBlock->next = prevFreeBlock->next;
                            usedBlock->prev = prevFreeBlock;

                            prevFreeBlock->next = usedBlock;
                            prevFreeBlock->next->prev = usedBlock;
                            
                            //mergeBlocks( &prevFreeBlock );
                        }
                    }

                    // if whole page is free: free page
                    if( NULL == m_allocatedPages[page].used )
                    {
						SAFE_FREE(m_allocatedPages[page].ptr, cmd);
                        --m_numAllocatedPages;
                        if( page != m_numAllocatedPages)
                        {
                            m_allocatedPages[page] = m_allocatedPages[m_numAllocatedPages];
                        }
                        
                    }
                    
                    return;
                }
                usedBlock = usedBlock->next;
            }
        }
    }
}

GR_RESULT ConstantBufferMgr::searchAllocatedPages( GR_UINT heaps, GR_GPU_SIZE memorySize, GR_GPU_MEMORY* memory, GR_GPU_SIZE* offset )
{
    for(unsigned int page = 0; page<m_numAllocatedPages; ++page )
    {
        if( heaps == m_allocatedPages[page].heaps )
        {
            MemBlock* freeBlock = m_allocatedPages[page].free;
            while( NULL!=freeBlock )
            {
                if( freeBlock->size>=memorySize)
                {
                    return allocateFromBlock( &m_allocatedPages[page], freeBlock, memorySize, memory, offset);
                }
                freeBlock = freeBlock->next;
            }
        }
    }
    return GR_ERROR_UNAVAILABLE;
}

GR_RESULT ConstantBufferMgr::allocateFromBlock( MemPage* page, MemBlock* block, GR_GPU_SIZE memorySize, GR_GPU_MEMORY* memory, GR_GPU_SIZE* offset)
{
    if( block->size==memorySize)
    {
        // remove from unused list
        if(block->next) 
            block->next->prev = block->prev;
        if(block->prev) 
            block->prev->next = block->next; 
        else 
            page->free = block->next;

        // add to used list
        block->next = page->used;
        page->used = block;

        *offset = block->offset;
    }
    else if( block->size>memorySize)
    {
        // create a new used block, split block into free and used
        if( NULL == m_unusedBlockDesc )
        {
            //realloc m_blockDesc
            m_unusedBlockDesc = new MemBlock[128];
            m_vecBlockDesc.push_back(m_unusedBlockDesc); // keep track of allocated memory

            for( int i = 0; i<128; ++i)
            {
                m_unusedBlockDesc[i].prev = &m_unusedBlockDesc[i-1];
                m_unusedBlockDesc[i].next = &m_unusedBlockDesc[i+1];
            }
            m_unusedBlockDesc[0].prev = NULL;
            m_unusedBlockDesc[127].next = NULL;
        }

        MemBlock* freeBlock = block;
        MemBlock* usedBlock = m_unusedBlockDesc;
        if( m_unusedBlockDesc->next )
        {
            m_unusedBlockDesc->next->prev = NULL;
        }
        m_unusedBlockDesc = m_unusedBlockDesc->next;

        // set size and offset for used and free blocks
        usedBlock->offset = block->offset;
        usedBlock->size = memorySize;
        freeBlock->offset+= memorySize;
        freeBlock->size -= memorySize;

        // add used block descriptor to page-list
        usedBlock->prev = NULL;
        usedBlock->next = page->used;
        page->used = usedBlock;

        *offset = usedBlock->offset;
    }

    *memory = page->ptr;
    return GR_SUCCESS;
}