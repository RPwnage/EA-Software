#pragma once
#include <mantle.h>
#include <assert.h>

class GpuMemMgr;
class GpuMemPool;
class CommandBuffer;

class GpuMemObject
{
public:
    typedef enum 
    {
        // Immutable, (static,) dynamic, volatile, staging?
        MEM_GPU_READ  = 1,
        MEM_GPU_WRITE = 2,
        MEM_CPU_READ  = 4,
        MEM_CPU_WRITE = 8,
    }MEM_FLAGS;

    GpuMemObject(GR_GPU_SIZE size, MEM_FLAGS flags);
    virtual ~GpuMemObject();

    void*   Map();
    void    Unmap();
    void    Prepare( CommandBuffer* cmdBuffer, GR_MEMORY_STATE state );

private:
    void*                       m_SysMemPtr;
    MEM_FLAGS                   m_flags;

    GpuMemPool*                 pool;
    unsigned int                m_memPage;

    GR_GPU_SIZE                 m_offset;
    GR_GPU_SIZE                 m_size;
};
