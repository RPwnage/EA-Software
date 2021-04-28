#pragma once
#include <mantle.h>
#include "Mesh.h"
#include "Pipeline.h"

class RenderPass;
class CommandBuffer;
class RenderState;

class DescriptorSet
{
public:
    DescriptorSet();
    virtual ~DescriptorSet();

	void dealloc(GR_CMD_BUFFER cmd);
    GR_RESULT init( const Pipeline* pipeline );

    void BindMemory   (SHADER_STAGE shader, DX_SLOTTYPE type, UINT StartSlot, UINT Count, GR_MEMORY_VIEW_ATTACH_INFO const *pMemAttachInfos);
    void BindSamplers (SHADER_STAGE shader, DX_SLOTTYPE type, UINT StartSlot, UINT Count, GR_SAMPLER const *pSamplers);
    void BindImages   (SHADER_STAGE shader, DX_SLOTTYPE type, UINT StartSlot, UINT Count, GR_GPU_MEMORY const *memory, GR_IMAGE_VIEW_ATTACH_INFO const *pImageViewAttachInfos);

    void bind( CommandBuffer* cmdBuffer );

    unsigned int getNumSlots()
    {
        return m_numSlots;
    }

    GR_GPU_MEMORY getAttachedMemory( GR_UINT32 index)const
    {
        return m_attachedMemory[index];
    }

    GR_DESCRIPTOR_SET_SLOT_TYPE getSlotType(GR_UINT32 idx)
    {
        return m_pipeline->getSlotType(idx);
    }

    GR_GPU_MEMORY memory() const
    {
        return m_rsrcDescSetMem;
    }


private:
    const Pipeline*         m_pipeline;

    GR_UINT32               m_numSlots;
    GR_GPU_MEMORY*          m_attachedMemory;

    // defines how the mesh gets bound to the pipeline
    GR_DESCRIPTOR_SET       m_rsrcDescSet;
    GR_GPU_MEMORY           m_rsrcDescSetMem;
    GR_GPU_SIZE             m_rsrcDescSetOffset;
};