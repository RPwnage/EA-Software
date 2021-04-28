#include "../../HookAPI.h"
#include "GpuMemMgr.h"
#include "..\MantleAppWsi.h"
#include "EASTL\vector.h"

GpuMemMgr::GpuMemMgr() :
    m_gpu(0),
    m_gpuHeapCount(0)
{
}

GpuMemMgr::~GpuMemMgr()
{
}

GR_RESULT GpuMemMgr::init( GR_INT gpu )
{
    m_gpu = gpu;

    GR_RESULT result = _grGetMemoryHeapCount(OriginIGO::getMantleDevice(m_gpu), &m_gpuHeapCount);

    for (GR_UINT idx = 0; ((result == GR_SUCCESS) && (idx < m_gpuHeapCount)); ++idx)
    {
        GR_SIZE dataSize = sizeof(m_gpuHeapInfo[0]);

        result = _grGetMemoryHeapInfo(OriginIGO::getMantleDevice(m_gpu), idx, GR_INFO_TYPE_MEMORY_HEAP_PROPERTIES,
                                        &dataSize, &m_gpuHeapInfo[idx]);
    }

    //m_64Pool.init();
    //m_128Pool.init();
    //m_256Pool.init();
    //m_512Pool.init();
    //m_1kPool.init();
    //m_2kPool.init();
    //m_4kPool.init();
    //m_8kPool.init();
    return result;
}

GR_RESULT GpuMemMgr::uninit( )
{
    //m_64Pool.uninit();
    //m_128Pool.uninit();
    //m_256Pool.uninit();
    //m_512Pool.uninit();
    //m_1kPool.uninit();
    //m_2kPool.uninit();
    //m_4kPool.uninit();
    //m_8kPool.uninit();

    return GR_SUCCESS;
}


namespace OriginIGO{
	EXTERN_NEXT_FUNC(GR_RESULT, grBindObjectMemoryHook, (GR_OBJECT object, GR_GPU_MEMORY mem, GR_GPU_SIZE offset));
};

GR_RESULT GpuMemMgr::allocAndBindGpuMem( GR_UINT32 heap, GR_OBJECT object, GR_GPU_MEMORY* memory, GR_GPU_SIZE* offset )
{
    *memory = GR_NULL_HANDLE;
    *offset = 0;

    // Determine how much video memory we need for the object
    GR_MEMORY_REQUIREMENTS memRequirements = {};
    GR_SIZE reqSize = sizeof(memRequirements);

    GR_RESULT result = _grGetObjectInfo(object,
                                       GR_INFO_TYPE_MEMORY_REQUIREMENTS,
                                       &reqSize,
                                       &memRequirements);
    if ((result == GR_SUCCESS) && (memRequirements.size != 0))
    {
        // Fill in the alloc info
        GR_MEMORY_ALLOC_INFO allocInfo = {};
        allocInfo.memPriority = GR_MEMORY_PRIORITY_NORMAL;
        allocInfo.heapCount = 1;
        allocInfo.heaps[0] = heap;
        allocInfo.size = ((memRequirements.size + m_gpuHeapInfo[heap].pageSize - 1) & ~(m_gpuHeapInfo[heap].pageSize - 1));

        // Allocate the memory object
        result = _grAllocMemory(OriginIGO::getMantleDevice(m_gpu), &allocInfo, memory);
    }

    if ((result == GR_SUCCESS) && (*memory != GR_NULL_HANDLE))
    {
		IGO_ASSERT(OriginIGO::grBindObjectMemoryHookNext != NULL);
        result = OriginIGO::grBindObjectMemoryHookNext(object, *memory, *offset);
    }

    return result;
}

GR_RESULT GpuMemMgr::allocAndBindGpuMem( GR_OBJECT object, GR_FLAGS  heapFlagFilter, GR_GPU_MEMORY* memory, GR_GPU_SIZE* offset )
{
    *memory = GR_NULL_HANDLE;
    *offset = 0;

    if (heapFlagFilter == 0)
    {
        heapFlagFilter = GR_MEMORY_HEAP_CPU_VISIBLE        |
                         GR_MEMORY_HEAP_CPU_GPU_COHERENT   |
                         GR_MEMORY_HEAP_CPU_UNCACHED       |
                         GR_MEMORY_HEAP_CPU_WRITE_COMBINED |
                         GR_MEMORY_HEAP_HOLDS_PINNED;
    }

    // Determine how much video memory we need for the object
    GR_MEMORY_REQUIREMENTS memRequirements = {};
    GR_SIZE reqSize = sizeof(memRequirements);

    GR_RESULT result = _grGetObjectInfo(object,
                                       GR_INFO_TYPE_MEMORY_REQUIREMENTS,
                                       &reqSize,
                                       &memRequirements);
    if ((result == GR_SUCCESS) && (memRequirements.size != 0))
    {
        // Fill in the alloc info
        GR_MEMORY_ALLOC_INFO allocInfo = {};
        allocInfo.memPriority = GR_MEMORY_PRIORITY_NORMAL;
        allocInfo.heapCount = 0;

        for (GR_SIZE i = 0; i < memRequirements.heapCount; ++i)
        {
            const GR_UINT heapIdx = memRequirements.heaps[i];

            if (m_gpuHeapInfo[heapIdx].flags & heapFlagFilter)
            {
                memRequirements.size = ((memRequirements.size + m_gpuHeapInfo[heapIdx].pageSize - 1) & ~(m_gpuHeapInfo[heapIdx].pageSize - 1));
                allocInfo.size = memRequirements.size;
                allocInfo.heaps[allocInfo.heapCount] = heapIdx;
                ++allocInfo.heapCount;
            }
        }

        // Allocate the memory object
        result = _grAllocMemory(OriginIGO::getMantleDevice(m_gpu), &allocInfo, memory);
        if (result == GR_SUCCESS)
        {
            *offset = 0;
        }
    }

    if ((result == GR_SUCCESS) && (*memory != GR_NULL_HANDLE))
    {
		IGO_ASSERT(OriginIGO::grBindObjectMemoryHookNext != NULL);
		result = OriginIGO::grBindObjectMemoryHookNext(object, *memory, *offset);
    }

    return result;
}


GR_RESULT GpuMemMgr::allocGpuMem(GR_FLAGS heapFlagFilter, GR_GPU_SIZE memorySize, GR_GPU_MEMORY* memory, GR_GPU_SIZE* offset )
{
    *memory = GR_NULL_HANDLE;
    *offset = 0;

    if (heapFlagFilter == 0)
    {
        heapFlagFilter = GR_MEMORY_HEAP_CPU_VISIBLE        |
                         GR_MEMORY_HEAP_CPU_GPU_COHERENT   |
                         GR_MEMORY_HEAP_CPU_UNCACHED       |
                         GR_MEMORY_HEAP_CPU_WRITE_COMBINED |
                         GR_MEMORY_HEAP_HOLDS_PINNED;
    }

    // Fill in the alloc info
    GR_MEMORY_ALLOC_INFO allocInfo = {};
    allocInfo.memPriority = GR_MEMORY_PRIORITY_NORMAL;
    allocInfo.heapCount = 0;

    for (GR_UINT i = 0; i < m_gpuHeapCount; ++i)
    {
        if (m_gpuHeapInfo[i].flags & heapFlagFilter)
        {
            allocInfo.size = ((memorySize + m_gpuHeapInfo[i].pageSize - 1) & ~(m_gpuHeapInfo[i].pageSize - 1));
            allocInfo.heaps[allocInfo.heapCount] = i;
            ++allocInfo.heapCount;
        }
    }

    // Allocate the memory object
    GR_RESULT result = _grAllocMemory(OriginIGO::getMantleDevice(m_gpu), &allocInfo, memory);
    if (result == GR_SUCCESS)
    {
        *offset = 0;
    }

    return result;
}

GR_RESULT GpuMemMgr::allocGpuMem(GR_UINT32 numHeaps, GR_UINT32* heaps, GR_GPU_SIZE memorySize, GR_GPU_MEMORY* memory, GR_GPU_SIZE* offset)
{
    // Fill in the alloc info
    GR_MEMORY_ALLOC_INFO allocInfo = {};
    allocInfo.memPriority = GR_MEMORY_PRIORITY_NORMAL;
    allocInfo.heapCount = 0;

    for (GR_UINT i = 0; i < numHeaps; ++i)
    {
        allocInfo.size = ((memorySize + m_gpuHeapInfo[heaps[i]].pageSize - 1) & ~(m_gpuHeapInfo[heaps[i]].pageSize - 1));
        allocInfo.heaps[allocInfo.heapCount] = heaps[i];
        ++allocInfo.heapCount;
    }

    // Allocate the memory object
    GR_RESULT result = _grAllocMemory(OriginIGO::getMantleDevice(m_gpu), &allocInfo, memory);
    if (result == GR_SUCCESS)
    {
        *offset = 0;
    }

    return result;
}

GR_RESULT GpuMemMgr::freeGpuMem( GR_GPU_MEMORY memory, GR_CMD_BUFFER cmd )
{
    GR_RESULT result = _grFreeMemoryInternal(memory, cmd);
    memory = NULL;
    return result;
}

GR_RESULT GpuMemMgr::filterHeapsByType(GR_UINT32* numHeaps, GR_UINT32* heaps, GR_HEAP_MEMORY_TYPE* select, GR_HEAP_MEMORY_TYPE* remove)
{
    GR_UINT32 count = 0;
    GR_UINT32 matching[8] = {0};

    GR_UINT32 heap = 0;
    for( heap=0; heap<*numHeaps; ++heap )
    {
        bool skip = false;
        if( select && (*select != m_gpuHeapInfo[heaps[heap]].heapMemoryType) )
        {
            skip = true;
        }
        if( remove && (*remove == m_gpuHeapInfo[heaps[heap]].heapMemoryType) )
        {
            skip = true;
        }
        if( !skip)
        {
            matching[count] = heaps[heap];
            ++count;
        }
    }
    if( count>0 )
    {
        memcpy(heaps, matching, sizeof(GR_UINT32)*count);
        *numHeaps = count;
        return GR_SUCCESS;
    }

    return GR_ERROR_UNAVAILABLE;
}

GR_RESULT GpuMemMgr::filterHeapsByFlags(GR_UINT32* numHeaps, GR_UINT32* heaps, GR_MEMORY_HEAP_FLAGS* select, GR_MEMORY_HEAP_FLAGS* remove)
{
    GR_UINT32 count = 0;
    GR_UINT32 matching[8] = {0};

    GR_UINT32 heap = 0;
    for( heap=0; heap<*numHeaps; ++heap )
    {
        bool skip = false;
		if (select && ((*select & m_gpuHeapInfo[heaps[heap]].flags) != (GR_FLAGS)*select))
        {
            skip = true;
        }
        if( remove && ((*remove & m_gpuHeapInfo[heaps[heap]].flags) != 0) )
        {
            skip = true;
        }
        if( !skip)
        {
            matching[count] = heaps[heap];
            ++count;
        }
    }
    if( count>0 )
    {
        memcpy(heaps, matching, sizeof(GR_UINT32)*count);
        *numHeaps = count;
        return GR_SUCCESS;
    }

    return GR_ERROR_UNAVAILABLE;
}

GR_RESULT GpuMemMgr::filterHeapsBySpeed(GR_UINT32* numHeaps, GR_UINT32* heaps, float gpuRead, float gpuWrite, float cpuRead, float cpuWrite)
{
    float speed = 0;
    
    GR_UINT32 heap = 0;
    for( heap=0; heap<*numHeaps; ++heap )
    {
        float perfRating =  cpuRead * m_gpuHeapInfo[heaps[heap]].cpuReadPerfRating + 
                            cpuWrite * m_gpuHeapInfo[heaps[heap]].cpuWritePerfRating + 
                            gpuRead * m_gpuHeapInfo[heaps[heap]].gpuReadPerfRating + 
                            gpuWrite * m_gpuHeapInfo[heaps[heap]].gpuWritePerfRating;

		// knock out criterias
		if (cpuRead > 0.0f && m_gpuHeapInfo[heaps[heap]].cpuReadPerfRating <= 0.0f)
			perfRating = 0;
		else
		if (gpuWrite > 0.0f && m_gpuHeapInfo[heaps[heap]].cpuWritePerfRating <= 0.0f)
			perfRating = 0;
		else
		if (gpuRead > 0.0f && m_gpuHeapInfo[heaps[heap]].gpuReadPerfRating <= 0.0f)
			perfRating = 0;
		else
		if (gpuWrite > 0.0f && m_gpuHeapInfo[heaps[heap]].gpuWritePerfRating <= 0.0f)
			perfRating = 0;

        speed = eastl::max(perfRating, speed);
    }

    GR_UINT32 count = 0;
    GR_UINT32 matching[8] = {0};
    for( heap=0; heap<*numHeaps; ++heap )
    {
        float perfRating =  cpuRead * m_gpuHeapInfo[heaps[heap]].cpuReadPerfRating + 
                            cpuWrite * m_gpuHeapInfo[heaps[heap]].cpuWritePerfRating + 
                            gpuRead * m_gpuHeapInfo[heaps[heap]].gpuReadPerfRating + 
                            gpuWrite * m_gpuHeapInfo[heaps[heap]].gpuWritePerfRating;
        if( perfRating == speed)
        {
            matching[count] = heaps[heap];
            ++count;
        }
    }

    if( count>0 )
    {
        memcpy(heaps, matching, sizeof(GR_UINT32)*count);
        *numHeaps = count;
        return GR_SUCCESS;
    }

    return GR_ERROR_UNAVAILABLE;
}

GR_RESULT GpuMemMgr::filterHeapsBySize(GR_UINT32* numHeaps, GR_UINT32* heaps, bool getLargest)
{
    GR_GPU_SIZE size = m_gpuHeapInfo[heaps[0]].heapSize;
    
    GR_UINT32 heap = 0;
    for( heap=1; heap<*numHeaps; ++heap )
    {
        if( getLargest )
			size = eastl::max(size, m_gpuHeapInfo[heaps[heap]].heapSize);
        else
			size = eastl::min(size, m_gpuHeapInfo[heaps[heap]].heapSize);
    }

    GR_UINT32 count = 0;
    GR_UINT32 matching[8] = {0};
    for( heap=0; heap<*numHeaps; ++heap )
    {
        if( m_gpuHeapInfo[heaps[heap]].heapSize == size)
        {
            matching[count] = heaps[heap];
            ++count;
        }
    }

    if( count>0 )
    {
        memcpy(heaps, matching, sizeof(GR_UINT32)*count);
        *numHeaps = count;
        return GR_SUCCESS;
    }

    return GR_ERROR_UNAVAILABLE;
}