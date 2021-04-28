#pragma once
#include <mantle.h>
#include "CommandBuffer.h"
#include "EASTL/hash_map.h"
#include <windows.h>

class Pipeline;
class DescriptorSet;

class CommandBufferRing
{
public:
    CommandBufferRing(GR_UINT gpu);
    virtual ~CommandBufferRing();
	void dealloc(GR_CMD_BUFFER cmd) {};

    GR_RESULT init(GR_UINT arraySize);

	bool isOpen();
	GR_RESULT begin(GR_FLAGS flags);
    GR_RESULT end();
    GR_RESULT submit(GR_QUEUE queue);
    GR_RESULT reset();

    GR_RESULT addMemReferences( GR_UINT32 numReferences, GR_MEMORY_REF* references );

    // mantle binding operations: filter redundant grCmds
    GR_RESULT bindPipeline( Pipeline* );
    GR_RESULT bindDescriptorSet( DescriptorSet* );
    GR_RESULT bindSamplerDescriptorSet( DescriptorSet* );
    GR_RESULT bindIndexData( GR_GPU_MEMORY mem, GR_GPU_SIZE offset, GR_ENUM indexType);

    GR_CMD_BUFFER           cmdBuffer();
    CommandBuffer::STATE    state();

    CommandBuffer* currentBuffer()
    {
		GR_UINT currentIndex = m_threadToIndex[GetCurrentThreadId()];
		return &m_CommandBuffers[currentIndex];
    }
private:
    GR_UINT             m_gpu;
    GR_UINT             m_numCmdBuffers;
	eastl::hash_map<GR_UINT, DWORD> m_threadToIndex;
    CommandBuffer*      m_CommandBuffers;
};