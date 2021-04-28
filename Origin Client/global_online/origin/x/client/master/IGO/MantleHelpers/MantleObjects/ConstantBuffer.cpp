#include "..\MantleAppWsi.h"
#include "Drawable.h"
#include "GpuMemMgr.h"
#include "CommandBuffer.h"
#include "ConstantBuffer.h"
#include "ConstantBufferMgr.h"
#include "..\..\IGOLogger.h"

ConstantBuffer::ConstantBuffer() :
    m_gpu( 0 ),
    m_bufferSize( 0 ),
    m_bufferMem( GR_NULL_HANDLE ),
    m_bufferOffset( 0 )
{
    m_bufferView.state = GR_MEMORY_STATE_DATA_TRANSFER;
}

ConstantBuffer::~ConstantBuffer()
{
}

void ConstantBuffer::dealloc(GR_CMD_BUFFER cmd)
{
	ConstantBufferMgr::Instance(m_gpu)->deallocate(m_bufferMem, m_bufferOffset, cmd);
}

void ConstantBuffer::init( CommandBuffer* cmdBuffer, const GR_GPU_SIZE stride, const GR_GPU_SIZE byteSize, const void* data, GR_UINT gpu )
{
    m_gpu = gpu;
	GR_RESULT result = GR_SUCCESS;

	if (m_bufferMem == GR_NULL_HANDLE)
	{
		result = ConstantBufferMgr::Instance(m_gpu)->allocate( GR_MEMORY_HEAP_CPU_VISIBLE, byteSize, &m_bufferMem, &m_bufferOffset, m_gpu);

		if (result == GR_SUCCESS)
		{
			if (NULL != data)
			{
				GR_BYTE* pData = GR_NULL_HANDLE;
				if (_grMapMemory(m_bufferMem, 0, (GR_VOID**)&pData) == GR_SUCCESS)
				{
					memcpy(&pData[m_bufferOffset], data, size_t(byteSize));
					_grUnmapMemory(m_bufferMem);
				}
			}

			m_bufferSize = byteSize;
		}
    }

    if (result == GR_SUCCESS)
    {
		Prepare(cmdBuffer, GR_MEMORY_STATE_GRAPHICS_SHADER_READ_ONLY);
		
		m_bufferView.stride = stride;
        m_bufferView.range  = byteSize;
        m_bufferView.offset = m_bufferOffset;
        m_bufferView.mem    = m_bufferMem;
        m_bufferView.format.channelFormat = GR_CH_FMT_UNDEFINED;
        m_bufferView.format.numericFormat = GR_NUM_FMT_UNDEFINED;
    }
}

void ConstantBuffer::Prepare( CommandBuffer* cmdBuffer, GR_MEMORY_STATE state )
{
    GR_MEMORY_STATE_TRANSITION transition = 
    {
        m_bufferMem,
        m_bufferView.state,
        state,
        m_bufferOffset,
        m_bufferSize
    };

    _grCmdPrepareMemoryRegions( cmdBuffer->cmdBuffer(), 1, &transition );
    m_bufferView.state = state;

    GR_MEMORY_REF mem[] = 
    {
        { m_bufferMem, 0 }
    };

    OriginIGO::IGOLogDebug("ConstantBuffer::Prepare: %p", m_bufferMem);

    cmdBuffer->addMemReferences(sizeof(mem) / sizeof(mem[0]), mem);
}