#pragma once
#include <mantle.h>
#include <assert.h>
#include "GpuMemPool.h"

class GpuMemMgr
{
public:
    GpuMemMgr();
    virtual ~GpuMemMgr();

    static GpuMemMgr* Instance(GR_UINT gpu)
    {
        static GpuMemMgr instance[GR_MAX_PHYSICAL_GPUS];
        return &instance[gpu];
    }

    const GR_UINT getHeapCount()
    {
        return m_gpuHeapCount;
    }

    const GR_MEMORY_HEAP_PROPERTIES&    getHeapInfo(GR_UINT heap)
    {
        return m_gpuHeapInfo[heap];
    }

    GR_RESULT init(GR_INT gpu);
    GR_RESULT uninit( );

    GR_RESULT   filterHeapsByType   (GR_UINT32* numHeaps, GR_UINT32* heaps, GR_HEAP_MEMORY_TYPE* select, GR_HEAP_MEMORY_TYPE* remove=0L);
    GR_RESULT   filterHeapsByFlags  (GR_UINT32* numHeaps, GR_UINT32* heaps, GR_MEMORY_HEAP_FLAGS* select, GR_MEMORY_HEAP_FLAGS* remove=0L);
    GR_RESULT   filterHeapsBySpeed  (GR_UINT32* numHeaps, GR_UINT32* heaps, float gpuRead = 0.f, float gpuWrite = 0.f, float cpuRead = 0.f, float cpuWrite = 0.f);
    GR_RESULT   filterHeapsBySize   (GR_UINT32* numHeaps, GR_UINT32* heaps, bool getLargest = true);

    GR_RESULT allocAndBindGpuMem( GR_UINT32 heap, GR_OBJECT object, GR_GPU_MEMORY* memory, GR_GPU_SIZE* offset );
    GR_RESULT allocAndBindGpuMem( GR_OBJECT object, GR_FLAGS  heapFlagFilter, GR_GPU_MEMORY* memory, GR_GPU_SIZE* offset );
    GR_RESULT allocGpuMem(  GR_FLAGS  heapFlagFilter, GR_GPU_SIZE memorySize, GR_GPU_MEMORY* memory, GR_GPU_SIZE* offset );
    GR_RESULT allocGpuMem(  GR_UINT32 numHeaps, GR_UINT32* heaps, GR_GPU_SIZE memorySize, GR_GPU_MEMORY* memory, GR_GPU_SIZE* offset );
	GR_RESULT freeGpuMem(GR_GPU_MEMORY memory, GR_CMD_BUFFER cmd);

    
private:
    GR_UINT                     m_gpu;

    // Mantle GPU memory heap info:
    GR_MEMORY_HEAP_PROPERTIES   m_gpuHeapInfo[GR_MAX_MEMORY_HEAPS];
    GR_UINT                     m_gpuHeapCount;

    // mempools sorted by size (each pool is for objects of size < (1<<(n+6)) and larger than next smaller pool
    //GpuMemPool*                 m_smallObjPools[10];

    // mempools for objects with a size>alignment (and min size 64k)
    // sorted by size (each pool is for objects of size < (1<<(n+16)) and larger than next smaller pool
    //GpuMemPool*                 m_largeObjPools[15];
};