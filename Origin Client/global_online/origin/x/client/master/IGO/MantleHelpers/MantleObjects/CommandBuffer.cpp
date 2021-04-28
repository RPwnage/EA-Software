#include <memory>
#include "CommandBuffer.h"
#include "Pipeline.h"
#include "DescriptorSet.h"
#include "../MantleAppWsi.h"
#include "..\..\HookAPI.h"
#include <assert.h>
#include "..\..\IGOLogger.h"


namespace OriginIGO{
	EXTERN_NEXT_FUNC(GR_RESULT, grQueueSubmitHook, (
		GR_QUEUE             queue,
		GR_UINT              cmdBufferCount,
		const GR_CMD_BUFFER* pCmdBuffers,
		GR_UINT              memRefCount,
		const GR_MEMORY_REF* pMemRefs,
		GR_FENCE             fence));

	EXTERN_NEXT_FUNC(GR_RESULT, grCreateCommandBufferHook, (
		GR_DEVICE                        device,
		const GR_CMD_BUFFER_CREATE_INFO* pCreateInfo,
		GR_CMD_BUFFER*                   pCmdBuffer));

	EXTERN_NEXT_FUNC(GR_RESULT, grBeginCommandBufferHook, (
		GR_CMD_BUFFER cmdBuffer,
		GR_FLAGS      flags));       // GR_CMD_BUFFER_BUILD_FLAGS

	EXTERN_NEXT_FUNC(GR_RESULT, grEndCommandBufferHook, (
		GR_CMD_BUFFER cmdBuffer));

	EXTERN_NEXT_FUNC(GR_RESULT, grResetCommandBufferHook, (
		GR_CMD_BUFFER cmdBuffer));
};



CommandBuffer::CommandBuffer(GR_CMD_BUFFER cmd) :
m_gpu(0),
m_freeCmdBuffer(false),
m_cmdBuffer(cmd),
m_fence(GR_NULL_HANDLE),
m_bDirty(true)
{
	m_memRefs.m_data = NULL;
	m_memRefs.m_reservedSize = 0;
	m_memRefs.m_size = 0;
}


CommandBuffer::CommandBuffer() :
    m_gpu(0),    
	m_freeCmdBuffer(true),
	m_cmdBuffer(GR_NULL_HANDLE),
    m_fence(GR_NULL_HANDLE),
    m_bDirty(true)
{
    m_memRefs.m_data = NULL;
    m_memRefs.m_reservedSize = 0;
    m_memRefs.m_size = 0;
}

CommandBuffer::~CommandBuffer()
{
    free( m_memRefs.m_data );
    m_memRefs.m_reservedSize = 0;
    m_memRefs.m_size = 0;

    // delete Mantle objects
    // wait for fence
    waitIdle();

    // release objects
	if (m_freeCmdBuffer && GR_NULL_HANDLE != m_cmdBuffer)
    {
		_grDestroyObjectInternal(m_fence, m_cmdBuffer);
		m_fence = GR_NULL_HANDLE;

		_grDestroyObjectInternal(m_cmdBuffer, m_cmdBuffer);
        m_cmdBuffer = GR_NULL_HANDLE;
    }
}

GR_RESULT CommandBuffer::init( GR_QUEUE_TYPE type, GR_UINT gpu )
{
    GR_RESULT result = GR_SUCCESS;
    
    m_gpu = gpu, 
    m_type = type;

    GR_CMD_BUFFER_CREATE_INFO cbInfo = {};
    cbInfo.queueType = type;
    cbInfo.flags     = 0;

    GR_FENCE_CREATE_INFO fInfo = {};
    fInfo.flags     = 0;

    m_cmdBufferState = CS_UNUSED;

	if (m_freeCmdBuffer)
	{

		IGO_ASSERT(OriginIGO::grCreateCommandBufferHookNext != NULL);
		result = OriginIGO::grCreateCommandBufferHookNext(OriginIGO::getMantleDevice(m_gpu), &cbInfo, &m_cmdBuffer);

		//result = _grCreateCommandBuffer(OriginIGO::getMantleDevice(m_gpu), &cbInfo, &m_cmdBuffer);
		if (result != GR_SUCCESS)
			return result;

		result = _grCreateFence(OriginIGO::getMantleDevice(m_gpu), &fInfo, &m_fence);
	}

    return result;
}

GR_RESULT CommandBuffer::begin(GR_FLAGS flags)
{
	if (!m_freeCmdBuffer)
		return GR_SUCCESS;

    //wait for command buffer to become available, reset mem-refs
    waitIdle();
    m_cmdBufferState = CS_UNUSED;
    m_bDirty = true;
    m_memRefs.m_size = 0;
    m_memRefSet.clear();

    // begin commandbuffer
    GR_RESULT result;
    //result = _grBeginCommandBuffer(m_cmdBuffer, flags);
	IGO_ASSERT(OriginIGO::grBeginCommandBufferHookNext != NULL);
	result = OriginIGO::grBeginCommandBufferHookNext(m_cmdBuffer, flags);

	assert(result == GR_SUCCESS);
    m_cmdBufferState = CS_OPEN;
	return result;
}

GR_RESULT CommandBuffer::end()
{
	if (!m_freeCmdBuffer)
		return GR_SUCCESS;
	
	GR_RESULT result;
    //result = _grEndCommandBuffer(m_cmdBuffer);
	IGO_ASSERT(OriginIGO::grEndCommandBufferHookNext != NULL);
	result = OriginIGO::grEndCommandBufferHookNext(m_cmdBuffer);
	
	m_cmdBufferState = CS_READY;
    return GR_SUCCESS;
}

GR_RESULT CommandBuffer::getMemRefs(GR_MEMORY_REF**  data,  GR_UINT32 *size, bool force)
{
	GR_RESULT result = GR_SUCCESS;

	if (!m_freeCmdBuffer)
	{

		if (m_bDirty || force)
		{
			if (m_memRefs.m_reservedSize < m_memRefSet.size())
			{
				m_memRefs.m_reservedSize = ((m_memRefSet.size() - 1) & ~0xF) + 16;
				m_memRefs.m_data = reinterpret_cast<GR_MEMORY_REF*>(realloc(m_memRefs.m_data, m_memRefs.m_reservedSize * sizeof(GR_MEMORY_REF)));
			}
			GR_UINT32 i = 0;
			for (MemRefSet::const_iterator it = m_memRefSet.begin(); it != m_memRefSet.end(); ++it, ++i)
			{
				m_memRefs.m_data[i] = (*it).ref;
			}
			m_memRefs.m_size = i;

			*data = m_memRefs.m_data;
			*size = m_memRefs.m_size;

			m_bDirty = false;
		}

	}
	return result;
}

GR_RESULT CommandBuffer::clearMemRefs(bool freeMemory)
{
	m_bDirty = true;

	if (freeMemory)
	{
		free(m_memRefs.m_data);
		m_memRefs.m_reservedSize = 0;
	}

	m_memRefs.m_size = 0;
	m_memRefSet.clear();

	return GR_SUCCESS;
}

GR_RESULT CommandBuffer::submit(GR_QUEUE queue)
{
	if (!m_freeCmdBuffer)
		return GR_SUCCESS;
	
	GR_RESULT result = GR_SUCCESS;

    if( m_bDirty )
    {
        if( m_memRefs.m_reservedSize < m_memRefSet.size() )
        {
            m_memRefs.m_reservedSize = ((m_memRefSet.size() - 1) & ~0xF) + 16;
            m_memRefs.m_data = reinterpret_cast<GR_MEMORY_REF*>( realloc(m_memRefs.m_data, m_memRefs.m_reservedSize * sizeof(GR_MEMORY_REF)) );
        }
        GR_UINT32 i = 0;
		for (MemRefSet::const_iterator it = m_memRefSet.begin(); it != m_memRefSet.end(); ++it, ++i)
        {
            m_memRefs.m_data[i] = (*it).ref;
        }
        m_memRefs.m_size = i;
        m_bDirty = false;
    }
    //result = _grQueueSubmit(queue, 1, &m_cmdBuffer, m_memRefs.m_size, m_memRefs.m_data, m_fence);
	IGO_ASSERT(OriginIGO::grQueueSubmitHookNext != NULL);
	result = OriginIGO::grQueueSubmitHookNext(queue, 1, &m_cmdBuffer, m_memRefs.m_size, m_memRefs.m_data, m_fence);
	assert(result == GR_SUCCESS);
	
	if (result != GR_SUCCESS)
        return result;

    m_cmdBufferState = CS_USED;

    return result;
}

GR_RESULT CommandBuffer::reset()
{
	if (!m_freeCmdBuffer)
		return GR_SUCCESS;
	
	// wait until queue is done executing the commandbuffer
    // reaching the Sleep should be avoided by choosing the init value large enough
    // waitIdle();

    //GR_RESULT result = _grResetCommandBuffer( m_cmdBuffer );
	IGO_ASSERT(OriginIGO::grResetCommandBufferHookNext != NULL);
	GR_RESULT result = OriginIGO::grResetCommandBufferHookNext(m_cmdBuffer);
    IGO_UNUSED(result)
	assert(result == GR_SUCCESS);

	m_cmdBufferState = CS_UNUSED;

    m_bDirty = true;
    m_memRefs.m_size = 0;
    m_memRefSet.clear();
    
    return GR_SUCCESS;
}

GR_RESULT CommandBuffer::addMemReferences( GR_UINT32 numReferences, GR_MEMORY_REF* references )
{
    m_bDirty = true;
    for( unsigned int i = 0; i<numReferences; ++i )
    {
        if( GR_NULL_HANDLE!=references[i].mem)
        {
            MemRefSet::iterator it = m_memRefSet.find(references[i]);
            if (m_memRefSet.end() == it)
            {
                m_memRefSet.insert(references[i]);
            }
            else
            {
                // for read-only allocations we should only add it to the list of references if it isn't already added
                // as we do not want to overwrite read-write references
                //if (references[i].flags != GR_MEMORY_REF_READ_ONLY)
                const_cast<MEMREF*>(&*it)->ref.flags = references[i].flags;
            }
        }
    }

    return GR_SUCCESS;
}

GR_RESULT CommandBuffer::bindPipeline(Pipeline* pipeline, GR_PIPELINE_BIND_POINT bindPoint )
{
    GR_PIPELINE grPipeline = pipeline->pipeline();
    _grCmdBindPipeline(m_cmdBuffer, bindPoint, grPipeline);
    return GR_SUCCESS;
}

GR_RESULT CommandBuffer::bindDescriptorSet(DescriptorSet* descriptorSet)
{
    descriptorSet->bind( this );
    return GR_SUCCESS;
}

GR_RESULT CommandBuffer::bindIndexData( GR_GPU_MEMORY mem, GR_GPU_SIZE offset, GR_ENUM indexType)
{
    _grCmdBindIndexData(m_cmdBuffer, mem, offset, indexType);
    return GR_SUCCESS;
}

GR_CMD_BUFFER CommandBuffer::cmdBuffer()
{
    return m_cmdBuffer;
}

CommandBuffer::STATE CommandBuffer::state()
{
    return m_cmdBufferState;
}

void CommandBuffer::waitIdle()
{
	if (!m_freeCmdBuffer)
		return;
	
	if (m_cmdBufferState == CS_USED)
    {
        _grWaitForFences( OriginIGO::getMantleDevice(m_gpu), 1, &m_fence, true, 10.f);
    }
}