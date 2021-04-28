#include "..\MantleAppWsi.h"
#include "GpuMemMgr.h"
#include "CommandBuffer.h"
#include "DescriptorSet.h"
#include "Defines.h"

DescriptorSet::DescriptorSet() :
    m_numSlots(0),
    m_attachedMemory( NULL ),
    m_rsrcDescSet( GR_NULL_HANDLE ),
    m_rsrcDescSetMem( GR_NULL_HANDLE ),
    m_rsrcDescSetOffset(0)
{
}

DescriptorSet::~DescriptorSet()
{

}

void DescriptorSet::dealloc(GR_CMD_BUFFER cmd)
{
    if (m_attachedMemory)
        free(m_attachedMemory);
    m_attachedMemory = NULL;
    SAFE_DESTROY(m_rsrcDescSet, cmd);
    SAFE_FREE(m_rsrcDescSetMem, cmd);
}


GR_RESULT DescriptorSet::init( const Pipeline* pipeline )
{
    m_pipeline = pipeline;
    m_numSlots = pipeline->resourceCount();

    m_attachedMemory = reinterpret_cast<GR_GPU_MEMORY*>(calloc(m_numSlots, sizeof(GR_GPU_MEMORY) ));

    // create DescriptorSets
    GR_DESCRIPTOR_SET_CREATE_INFO dsInfo = {};
    dsInfo.slots   = m_pipeline->resourceCount();
        
    GR_RESULT result = _grCreateDescriptorSet(OriginIGO::getMantleDevice(pipeline->gpu()), &dsInfo, &m_rsrcDescSet);
    if (result == GR_SUCCESS)
    {
        result = GpuMemMgr::Instance(pipeline->gpu())->allocAndBindGpuMem(m_rsrcDescSet,0,&m_rsrcDescSetMem, &m_rsrcDescSetOffset);
    }
    return result;
}

void DescriptorSet::BindMemory( SHADER_STAGE shader, DX_SLOTTYPE type, UINT StartSlot, UINT Count, GR_MEMORY_VIEW_ATTACH_INFO const *pMemAttachInfos)
{
    _grBeginDescriptorSetUpdate(m_rsrcDescSet);

    for( unsigned int i = 0; i<Count; ++i)
    {
        GR_INT32 index = m_pipeline->slotIndex( shader, type, StartSlot+i );
        if( index!=-1)
        {
            _grAttachMemoryViewDescriptors(m_rsrcDescSet, index, 1, &pMemAttachInfos[i] );
            m_attachedMemory[index] = pMemAttachInfos[i].mem;
        }
    }

    _grEndDescriptorSetUpdate(m_rsrcDescSet);
}

void DescriptorSet::BindSamplers(SHADER_STAGE shader, DX_SLOTTYPE type, UINT StartSlot, UINT Count, GR_SAMPLER const *pSamplers)
{
    _grBeginDescriptorSetUpdate(m_rsrcDescSet);

    for( unsigned int i = 0; i<Count; ++i)
    {
        GR_INT32 index = m_pipeline->slotIndex( shader, type, StartSlot+i );
        if( index!=-1)
        {
            _grAttachSamplerDescriptors(m_rsrcDescSet, index, 1, &pSamplers[i] );
        }
    }

    _grEndDescriptorSetUpdate(m_rsrcDescSet);
    
}

void DescriptorSet::BindImages(SHADER_STAGE shader, DX_SLOTTYPE type, UINT StartSlot, UINT Count, GR_GPU_MEMORY const *memory, GR_IMAGE_VIEW_ATTACH_INFO const *pImageViewAttachInfos)
{
    _grBeginDescriptorSetUpdate(m_rsrcDescSet);

    for( unsigned int i = 0; i<Count; ++i)
    {
        GR_INT32 index = m_pipeline->slotIndex( shader, type, StartSlot+i );
        if( index!=-1)
        {
            _grAttachImageViewDescriptors(m_rsrcDescSet, index, 1, &pImageViewAttachInfos[i] );
            m_attachedMemory[index] = memory[i];
        }
    }

    _grEndDescriptorSetUpdate(m_rsrcDescSet);
}

void DescriptorSet::bind( CommandBuffer* cmdBuffer )
{
    if( GR_NULL_HANDLE != m_rsrcDescSet)
    {
        _grCmdBindDescriptorSet( cmdBuffer->cmdBuffer(),
                                m_pipeline->bindPoint(),
                                0,
                                m_rsrcDescSet,
                                0);
    }
}
