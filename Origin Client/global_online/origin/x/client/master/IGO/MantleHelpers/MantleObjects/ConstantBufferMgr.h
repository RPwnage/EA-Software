#pragma once
#include <mantle.h>
#include "GpuMemMgr.h"

#include "EASTL/vector.h"
class CommandBuffer;

typedef struct _MemBlock
{
    GR_GPU_SIZE     offset;
    GR_GPU_SIZE     size;
    _MemBlock*      prev;
    _MemBlock*      next;
}MemBlock;

typedef struct _MemPage
{
    GR_GPU_SIZE     size;
    GR_GPU_MEMORY   ptr;
    GR_UINT         heaps;

    MemBlock*       free;
    MemBlock*       used;
}MemPage;

class ConstantBufferMgr
{
public:
    ConstantBufferMgr();
    virtual ~ConstantBufferMgr();
    
    static ConstantBufferMgr* Instance(GR_UINT gpu)
    {
        static ConstantBufferMgr instance[GR_MAX_PHYSICAL_GPUS];
        return &instance[gpu];
    }

    void    init( );
    void    deinit( );

	GR_RESULT   allocate(GR_FLAGS  heapFlagFilter, GR_GPU_SIZE memorySize, GR_GPU_MEMORY* memory, GR_GPU_SIZE* offset, GR_UINT gpu);
	void        deallocate(GR_GPU_MEMORY memblock, GR_GPU_SIZE offset, GR_CMD_BUFFER cmd);
private:
    GR_RESULT   searchAllocatedPages( GR_UINT heaps, GR_GPU_SIZE memorySize, GR_GPU_MEMORY* memory, GR_GPU_SIZE* offset );
    GR_RESULT   allocateFromBlock( MemPage* page, MemBlock* block, GR_GPU_SIZE memorySize, GR_GPU_MEMORY* memory, GR_GPU_SIZE* offset);

private:
    GR_UINT                 m_gpu;

    unsigned int            m_maxAllocatedPages;
    unsigned int            m_numAllocatedPages;
    MemPage*                m_allocatedPages;
    MemBlock*               m_unusedBlockDesc;
    eastl::vector<MemBlock*>  m_vecBlockDesc;
};