#pragma once
#include <mantle.h>
#include "ConstantBuffer.h"

class CommandBuffer;

class Mesh
{
public:
    Mesh();
    virtual ~Mesh();
	void dealloc(GR_CMD_BUFFER cmdBuffer);
    GR_RESULT init( CommandBuffer* cmdBuffer, GR_UINT32 numVertices, GR_GPU_SIZE vbStride, const void* vertices, GR_UINT32 numIndices, GR_GPU_SIZE ibStride, const void* indices, GR_UINT gpu );
    
    GR_MEMORY_VIEW_ATTACH_INFO  vbView(){return m_vtxBufferView;}
    GR_GPU_MEMORY   vbMemory(){return m_vtxBufferMemory;}
    GR_GPU_SIZE     vbOffset(){return m_vtxBufferOffset;}
    GR_UINT32       vertexCount(){return m_numVertices;}

    GR_MEMORY_VIEW_ATTACH_INFO  ibView(){return m_idxBufferView;}
    GR_GPU_MEMORY   ibMemory(){return m_idxBufferMemory;}
    GR_GPU_SIZE     ibOffset(){return m_idxBufferOffset;}
    GR_UINT32       indexCount(){return m_numIndices;}
    GR_ENUM         indexType(){return m_idxType;}

private:
    // render data
    GR_UINT32                   m_numVertices;
    GR_MEMORY_VIEW_ATTACH_INFO  m_vtxBufferView;
    GR_GPU_MEMORY               m_vtxBufferMemory;
    GR_GPU_SIZE                 m_vtxBufferOffset;

    GR_UINT32                   m_numIndices;
    GR_ENUM                     m_idxType;
    GR_MEMORY_VIEW_ATTACH_INFO  m_idxBufferView;
    GR_GPU_MEMORY               m_idxBufferMemory;
    GR_GPU_SIZE                 m_idxBufferOffset;
};