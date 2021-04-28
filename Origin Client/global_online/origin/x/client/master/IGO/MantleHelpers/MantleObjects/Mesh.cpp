#include "Mesh.h"
#include "..\MantleAppWsi.h"
#include "GpuMemMgr.h"
#include "CommandBuffer.h"
#include "ConstantBuffer.h"
#include "..\..\IGOLogger.h"
#include <math.h>
#include "Defines.h"

Mesh::Mesh() :
    m_numVertices( 0 ),
    m_numIndices( 0 ),
    m_idxType( 0 ),
    m_vtxBufferMemory( GR_NULL_HANDLE ),
    m_idxBufferMemory( GR_NULL_HANDLE )
{
}

Mesh::~Mesh()
{

}

void Mesh::dealloc(GR_CMD_BUFFER cmdBuffer)
{
	SAFE_FREE(m_vtxBufferMemory, cmdBuffer);
    SAFE_FREE(m_idxBufferMemory, cmdBuffer);
}

GR_RESULT Mesh::init( CommandBuffer* cmdBuffer, GR_UINT32 numVertices, GR_GPU_SIZE vbStride, const void* vertices, GR_UINT32 numIndices, GR_GPU_SIZE ibStride, const void* indices, GR_UINT gpu )
{
    GR_RESULT result = GR_SUCCESS;

    if( numVertices*vbStride > 0 )
    {
        if (result == GR_SUCCESS)
        {
            result = GpuMemMgr::Instance(gpu)->allocGpuMem( GR_MEMORY_HEAP_CPU_VISIBLE, numVertices*vbStride, &m_vtxBufferMemory, &m_vtxBufferOffset);
        }

        if ((vertices != NULL) && (numVertices != 0) && (result == GR_SUCCESS))
        {
            GR_VOID* pData = GR_NULL_HANDLE;
            if (_grMapMemory(m_vtxBufferMemory, 0, &pData) == GR_SUCCESS)
            {
                memcpy(pData, vertices, (size_t)(numVertices*vbStride) );
                _grUnmapMemory( m_vtxBufferMemory );
            }
        }
        
        if (result == GR_SUCCESS)
        {
            m_vtxBufferView.stride = vbStride;
            m_vtxBufferView.range  = numVertices*vbStride;
            m_vtxBufferView.offset = 0;
            m_vtxBufferView.mem    = m_vtxBufferMemory;
            m_vtxBufferView.format.channelFormat = GR_CH_FMT_UNDEFINED;
            m_vtxBufferView.format.numericFormat = GR_NUM_FMT_UNDEFINED;
        }
        
        GR_MEMORY_STATE_TRANSITION transition[1] = 
        {
           {
                m_vtxBufferMemory,
                GR_MEMORY_STATE_DATA_TRANSFER,
                GR_MEMORY_STATE_GRAPHICS_SHADER_READ_ONLY,
                m_vtxBufferOffset,
                numVertices*vbStride
            }
        };
        _grCmdPrepareMemoryRegions( cmdBuffer->cmdBuffer(), 1, transition );
        m_vtxBufferView.state = GR_MEMORY_STATE_GRAPHICS_SHADER_READ_ONLY;

        GR_MEMORY_REF mem[] = 
        {
            { m_vtxBufferMemory, 0 }
        };
        OriginIGO::IGOLogDebug("Mesh::init(m_vtxBufferMemory) %p", mem[0].mem);
        cmdBuffer->addMemReferences(sizeof(mem) / sizeof(mem[0]), mem);
    }

    if( numIndices )
    {
        if (result == GR_SUCCESS)
        {
            result = GpuMemMgr::Instance(gpu)->allocGpuMem( GR_MEMORY_HEAP_CPU_VISIBLE, numIndices*ibStride, &m_idxBufferMemory, &m_idxBufferOffset);
        }

        if ((indices != NULL) && (numIndices != 0) && (result == GR_SUCCESS))
        {
            GR_VOID* pData = GR_NULL_HANDLE;
            if (_grMapMemory(m_idxBufferMemory, 0, &pData) == GR_SUCCESS)
            {
                memcpy(pData, indices, (size_t)(numIndices*ibStride) );
                _grUnmapMemory( m_idxBufferMemory );
            }
            
        }
        
        if (result == GR_SUCCESS)
        {
            m_idxBufferView.stride = ibStride;
            m_idxBufferView.range  = numIndices*ibStride;
            m_idxBufferView.offset = 0;
            m_idxBufferView.mem    = m_idxBufferMemory;
            m_idxBufferView.format.channelFormat = GR_CH_FMT_UNDEFINED;
            m_idxBufferView.format.numericFormat = GR_NUM_FMT_UNDEFINED;
        }

        GR_MEMORY_STATE_TRANSITION transition[1] = 
        {
           {
                m_idxBufferMemory,
                GR_MEMORY_STATE_DATA_TRANSFER,
                GR_MEMORY_STATE_GRAPHICS_SHADER_READ_ONLY,
                m_idxBufferOffset,
                numIndices*ibStride
            }
        };
        _grCmdPrepareMemoryRegions( cmdBuffer->cmdBuffer(), 1, transition );
        m_idxBufferView.state = GR_MEMORY_STATE_GRAPHICS_SHADER_READ_ONLY;

        GR_MEMORY_REF mem[] = 
        {
            { m_idxBufferMemory, GR_MEMORY_REF_READ_ONLY }
        };
        OriginIGO::IGOLogDebug("Mesh::init(m_idxBufferMemory) %p", mem[0].mem);
        cmdBuffer->addMemReferences(sizeof(mem) / sizeof(mem[0]), mem);
    }

    m_numVertices = numVertices;
    m_numIndices = numIndices;
    
    // init indexStride
    if( 0 != m_numIndices )
    {
        switch( ibStride )
        {
        case 2:
            m_idxType = GR_INDEX_16;
            break;
        case 4:
            m_idxType = GR_INDEX_32;
            break;
        default:
            return GR_ERROR_INVALID_FORMAT;
        }
    }

    return GR_SUCCESS;
}
