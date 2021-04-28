#pragma once
#include <mantle.h>
#include <assert.h>
#include "EASTL/vector.h"

class GpuMemMgr;

// GpuMemPool manages a mempool in a dedicated heap
// this will save GPU memory by pooling smaller resources together and reduce number of memory references per commandbuffer
class GpuMemPool
{
public:
    GpuMemPool() {}
    virtual ~GpuMemPool();

	bool init(GR_UINT gpu, GR_UINT32 heap, GR_GPU_SIZE _pageSize, GR_GPU_SIZE _count = 64);
	bool uninit(GR_CMD_BUFFER cmd);

    // return the first unallocated element
    GR_RESULT allocate(GR_GPU_SIZE size, GR_GPU_MEMORY* memory, GR_GPU_SIZE* offset);
    // return an element to the pool
    GR_RESULT free(unsigned int memPage, GR_GPU_SIZE m_offset, GR_GPU_SIZE size);

private:
    bool            m_isInit;
    GR_UINT32       m_maxElements;
    GR_UINT32       m_curElementsUsed;
    GR_UINT32       m_maxElementsUsed;

	GR_UINT32       m_heap;
	GR_UINT			m_gpu;

    GR_GPU_SIZE     m_poolPageSize;
    GR_GPU_SIZE     m_countPerPoolPage;

    typedef struct _MEMPAGE
    {
        bool            m_dirty;
        GR_UINT32       m_heap;
        GR_MEMORY_STATE m_state;
        
        GR_GPU_MEMORY   m_memory;
        GR_GPU_SIZE     m_offset;
        GR_UINT64*      m_unallocated;
    }MEMPAGE;

    eastl::vector<MEMPAGE*>   m_memPages;
};
