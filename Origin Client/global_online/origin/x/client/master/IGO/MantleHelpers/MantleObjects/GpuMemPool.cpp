#include "GpuMemPool.h"
#include "GpuMemMgr.h"
#include "..\MantleAppWsi.h"

GpuMemPool::~GpuMemPool()
{
}

bool GpuMemPool::init(GR_UINT gpu, GR_UINT32 heap, GR_GPU_SIZE _pageSize, GR_GPU_SIZE _count)
{
    // make sure _pageSize is power of 2
    assert( 0 == (_pageSize&(_pageSize-1)) );

    m_maxElementsUsed = 0;
    m_heap = heap;
    m_poolPageSize = _pageSize;
	m_gpu = gpu;
        
	GR_MEMORY_HEAP_PROPERTIES heapInfo = GpuMemMgr::Instance(m_gpu)->getHeapInfo(heap);
    // if allocation is smaller than one heap-page: increase count
    GR_GPU_SIZE allocSize = _count*_pageSize;
    GR_GPU_SIZE remainder = heapInfo.pageSize - (allocSize % heapInfo.pageSize);
    m_countPerPoolPage += _count + remainder / _pageSize;

    // if pagesize is larger than heapInfo.pagesize: reduce pagesize to avoid wasting memory
    if( m_poolPageSize > heapInfo.pageSize )
    {
        m_countPerPoolPage *= m_poolPageSize/heapInfo.pageSize;
        m_poolPageSize = heapInfo.pageSize;
    }

    MEMPAGE* page = new MEMPAGE;
    GpuMemMgr::Instance(m_gpu)->allocGpuMem( 1, &m_heap, m_poolPageSize*m_countPerPoolPage, &page->m_memory, &page->m_offset);

    page->m_unallocated = new GR_UINT64[unsigned int((m_countPerPoolPage+63)>>6)];
    for( int i = 0; i<((m_countPerPoolPage+63)>>6)-1; ++i )
    {
        page->m_unallocated[i] = (GR_UINT64) ~0;
    }
    page->m_unallocated[((m_countPerPoolPage+63)>>6)-1] = (static_cast<GR_UINT64>(1)<<(m_countPerPoolPage&63))-1;

    m_memPages.push_back(page);
    m_isInit = true;

/*
    GR_UINT32 numHeaps=GpuMemMgr::Instance()->getHeapCount();
    GR_UINT32 heaps[8]={0,1,2,3,4,5,6,7};

    GR_HEAP_MEMORY_TYPE type_local = GR_HEAP_MEMORY_LOCAL;
    GR_MEMORY_HEAP_FLAGS flags = GR_MEMORY_HEAP_CPU_VISIBLE;

    // remove all non local heaps if we have any local ones
    GpuMemMgr::Instance()->filterHeapsByType(&numHeaps, heaps, &type_local);
    // remove all cpu visible heaps if we have invisible ones
    GpuMemMgr::Instance()->filterHeapsByFlags(&numHeaps, heaps, 0L, &flags);
    // filter fastest heaps for GPU read
    GpuMemMgr::Instance()->filterHeapsBySpeed(&numHeaps, heaps, 1,0,0,0);
    // filter largest heaps
    GpuMemMgr::Instance()->filterHeapsBySize(&numHeaps, heaps);
    // if we still have more than one heap left: take the first one.
*/
    return true;
}

bool GpuMemPool::uninit(GR_CMD_BUFFER cmd)
{
    eastl::vector<MEMPAGE*>::iterator itb = m_memPages.begin();
    eastl::vector<MEMPAGE*>::iterator ite = m_memPages.end();
    eastl::vector<MEMPAGE*>::iterator it;
    for( it = itb; it!=ite; ++it )
    {
        // verify all elements are deallocated
        for( int i = 0; i<((m_countPerPoolPage+63)>>6)-1; ++i )
        {
            assert((*it)->m_unallocated[i] == ~0);
        }
        assert((*it)->m_unallocated[((m_countPerPoolPage+63)>>6)-1] == (1<<(m_countPerPoolPage&63))-1);

        delete[] (*it)->m_unallocated;

        // free memory
        GpuMemMgr::Instance(m_gpu)->freeGpuMem((*it)->m_memory, cmd);
        delete (*it);
    }
        
    m_isInit = false;

    // output usage statistics to debug output so allocation size can be optimized
    for( it = itb; it!=ite; ++it )
    {
    }

    m_memPages.clear();
    return true;
}

// return the first unallocated element
GR_RESULT GpuMemPool::allocate(GR_GPU_SIZE size, GR_GPU_MEMORY* memory, GR_GPU_SIZE* offset)
{
    GR_GPU_SIZE numPages = ((size-1)/m_poolPageSize)+1;
    if( numPages==1 )
    {
        eastl::vector<MEMPAGE*>::iterator itb = m_memPages.begin();
        eastl::vector<MEMPAGE*>::iterator ite = m_memPages.end();
        eastl::vector<MEMPAGE*>::iterator it;
        for( it = itb; it!=ite; ++it )
        {
            GR_GPU_SIZE element = 0;
            for( unsigned int i = 0; i<(m_countPerPoolPage+63)>>6; ++i )
            {
                if( (*it)->m_unallocated[i] )
                {
                    GR_UINT64 bitMask = (*it)->m_unallocated[i]&((*it)->m_unallocated[i]-1);
                    GR_UINT64 bit = (*it)->m_unallocated[i]^bitMask;

                    while(bit>>=1)
                    {
                        ++element;
                    }
                    *memory = (*it)->m_memory;
                    *offset = (*it)->m_offset+element*m_poolPageSize;
                    return GR_SUCCESS;
                }
                element+=64;
            }
        }
    }
    else
    {
        GR_UINT64 mask = ((static_cast<GR_UINT64>(1)<<numPages)-1);

        eastl::vector<MEMPAGE*>::iterator itb = m_memPages.begin();
        eastl::vector<MEMPAGE*>::iterator ite = m_memPages.end();
        eastl::vector<MEMPAGE*>::iterator it;
        for( it = itb; it!=ite; ++it )
        {
            for( unsigned int i = 0; i<m_countPerPoolPage; ++i )
            {
                unsigned int shift = i&0x3F;
                unsigned int baseIndex = i>>6;
                GR_UINT64 free_mask = ((*it)->m_unallocated[baseIndex]>>shift) + ((*it)->m_unallocated[baseIndex+1]<<(64-shift));
                if( (free_mask & mask) == mask) 
                {
                    *memory = (*it)->m_memory;
                    *offset = (*it)->m_offset+i*m_poolPageSize;
                    return GR_SUCCESS;
                }
            }
        }
    }

    return GR_ERROR_OUT_OF_MEMORY;
}

// return an element to the pool
GR_RESULT GpuMemPool::free(unsigned int memPage, GR_GPU_SIZE offset, GR_GPU_SIZE size)
{
    MEMPAGE* page = m_memPages[memPage];
    GR_GPU_SIZE numPages = ((size-1)/m_poolPageSize)+1;

    GR_GPU_SIZE idx = (offset - page->m_offset)/m_poolPageSize;
    GR_GPU_SIZE block = idx>>6;
    GR_GPU_SIZE mask = (static_cast<GR_GPU_SIZE>(1)<<numPages)-1;
    GR_GPU_SIZE mask0 = (mask<<block);
    GR_GPU_SIZE mask1 = (mask>>(64-block));

    // check the pages we want to free are actually used
    assert((page->m_unallocated[block]&mask0) == 0);
    assert((page->m_unallocated[block+1]&mask1) == 0);

    page->m_unallocated[block] |= mask0;
    page->m_unallocated[block+1] |= mask1;

    return GR_SUCCESS;
}