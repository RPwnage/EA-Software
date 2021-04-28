#include "CommandBufferRing.h"
#include <assert.h>
#include <windows.h>

CommandBufferRing::CommandBufferRing(GR_UINT gpu) :
    m_gpu(gpu),
    m_numCmdBuffers(0),
    m_CommandBuffers(NULL)
{
}

CommandBufferRing::~CommandBufferRing()
{
    delete[] m_CommandBuffers;
}

GR_RESULT CommandBufferRing::init(GR_UINT arraySize)
{
    m_numCmdBuffers = arraySize;
    m_CommandBuffers = new CommandBuffer[arraySize];

    for( unsigned int i = 0; i<arraySize; ++i )
    {
        m_CommandBuffers[i].init(GR_QUEUE_UNIVERSAL, m_gpu);
    }

    return GR_SUCCESS;
}

bool CommandBufferRing::isOpen()
{
	assert(m_threadToIndex.find(GetCurrentThreadId()) != m_threadToIndex.end());
	GR_UINT currentIndex = m_threadToIndex[GetCurrentThreadId()];
	return CommandBuffer::CS_OPEN == m_CommandBuffers[currentIndex].state();
}

GR_RESULT CommandBufferRing::begin(GR_FLAGS flags)
{
	GR_UINT currentIndex = m_threadToIndex[GetCurrentThreadId()];
	if (CommandBuffer::CS_USED == m_CommandBuffers[currentIndex].state())
    {
        ++currentIndex;
        if( currentIndex == m_numCmdBuffers)
            currentIndex = 0;
	
		m_threadToIndex[GetCurrentThreadId()] = currentIndex;
	}

    return m_CommandBuffers[currentIndex].begin(flags);
}

GR_RESULT CommandBufferRing::end()
{
	assert(m_threadToIndex.find(GetCurrentThreadId()) != m_threadToIndex.end());
	GR_UINT currentIndex = m_threadToIndex[GetCurrentThreadId()];
	return m_CommandBuffers[currentIndex].end();
}

GR_RESULT CommandBufferRing::submit(GR_QUEUE queue)
{
	assert(m_threadToIndex.find(GetCurrentThreadId()) != m_threadToIndex.end());
	GR_UINT currentIndex = m_threadToIndex[GetCurrentThreadId()];
	return  m_CommandBuffers[currentIndex].submit(queue);
}

GR_RESULT CommandBufferRing::reset()
{
	assert(m_threadToIndex.find(GetCurrentThreadId()) != m_threadToIndex.end());
	GR_UINT currentIndex = m_threadToIndex[GetCurrentThreadId()];
	if (CommandBuffer::CS_USED == m_CommandBuffers[currentIndex].state())
    {
        ++currentIndex;
        if( currentIndex == m_numCmdBuffers)
            currentIndex = 0;
	
		m_threadToIndex[GetCurrentThreadId()] = currentIndex;
	}

    return m_CommandBuffers[currentIndex].reset();
}

GR_RESULT CommandBufferRing::addMemReferences( GR_UINT32 numReferences, GR_MEMORY_REF* references )
{
	assert(m_threadToIndex.find(GetCurrentThreadId()) != m_threadToIndex.end());
	GR_UINT currentIndex = m_threadToIndex[GetCurrentThreadId()];
	return m_CommandBuffers[currentIndex].addMemReferences(numReferences, references);
}

GR_RESULT CommandBufferRing::bindPipeline( Pipeline* pipeline)
{
	assert(m_threadToIndex.find(GetCurrentThreadId()) != m_threadToIndex.end());
	GR_UINT currentIndex = m_threadToIndex[GetCurrentThreadId()];
	return m_CommandBuffers[currentIndex].bindPipeline(pipeline);
}

GR_RESULT CommandBufferRing::bindDescriptorSet( DescriptorSet* ds)
{
	assert(m_threadToIndex.find(GetCurrentThreadId()) != m_threadToIndex.end());
	GR_UINT currentIndex = m_threadToIndex[GetCurrentThreadId()];
	return m_CommandBuffers[currentIndex].bindDescriptorSet(ds);
}

GR_RESULT CommandBufferRing::bindIndexData( GR_GPU_MEMORY mem, GR_GPU_SIZE offset, GR_ENUM indexType)
{
	assert(m_threadToIndex.find(GetCurrentThreadId()) != m_threadToIndex.end());
	GR_UINT currentIndex = m_threadToIndex[GetCurrentThreadId()];
	return m_CommandBuffers[currentIndex].bindIndexData(mem, offset, indexType);
}

GR_CMD_BUFFER CommandBufferRing::cmdBuffer()
{
	assert(m_threadToIndex.find(GetCurrentThreadId()) != m_threadToIndex.end());
	GR_UINT currentIndex = m_threadToIndex[GetCurrentThreadId()];
	return m_CommandBuffers[currentIndex].cmdBuffer();
}

CommandBuffer::STATE CommandBufferRing::state()
{
	assert(m_threadToIndex.find(GetCurrentThreadId()) != m_threadToIndex.end());
	GR_UINT currentIndex = m_threadToIndex[GetCurrentThreadId()];
	return m_CommandBuffers[currentIndex].state();
}
