#pragma once
#include <mantle.h>
#include "shader.h"

class CommandBuffer;
class DescriptorSet;

class Pipeline
{
public:
    Pipeline();
    virtual ~Pipeline();
	void dealloc(GR_CMD_BUFFER cmdBuffer);
    GR_RESULT init( GR_GRAPHICS_PIPELINE_CREATE_INFO& pipeInfo, Shader* vs, Shader* hs, Shader* ds, Shader* gs, Shader* ps, GR_UINT gpu );
    GR_RESULT init( GR_COMPUTE_PIPELINE_CREATE_INFO& pipeInfo, Shader* cs );

    GR_UINT         gpu() const
    {
        return m_gpu;
    }

    GR_PIPELINE     pipeline() const
    {
        return m_Pipeline;
    }

    GR_GPU_MEMORY   memory() const
    {
        return m_PipelineMem;
    }

    GR_UINT32 resourceCount() const
    {
        if(m_pipelineBindPoint == GR_PIPELINE_BIND_POINT_GRAPHICS)
        {
            return m_Offsets[PIXEL_SHADER][DXST_COUNT];
        }
        else
        {
            return m_Offsets[COMPUTE_SHADER][DXST_COUNT];
        }
    }

    GR_DESCRIPTOR_SET_SLOT_TYPE getSlotType(GR_UINT32 idx) const
    {
        if(m_pipelineBindPoint == GR_PIPELINE_BIND_POINT_GRAPHICS)
        {
            for(int i = 0; i<SHADERSTAGE_COUNT; ++i )
            {
                if(idx<m_Offsets[i][DXST_COUNT])
                    return getSlotType( m_Offsets[i], idx );
            }
        }
        else
        {
            return getSlotType( m_Offsets[COMPUTE_SHADER], idx );
        }
        return GR_SLOT_UNUSED;
    }

    GR_ENUM bindPoint() const 
    {
        return m_pipelineBindPoint;
    }
    // each Drawable using a pipeline needs its own descriptorset
    // the pipeline knows what parameters it requires, so it can act as a factory
    DescriptorSet*    createDescriptorSet();

    GR_INT32 slotIndex(SHADER_STAGE stage, DX_SLOTTYPE type, UINT StartSlot) const
    {
        const ILC_DX_RESOURCE_MAPPING& rm = m_resourceMapping[stage];
        const GR_UINT32* offsets = m_Offsets[stage];
        GR_INT32 index = 0;
        switch( type )
        {
        case DXST_VERTEXBUFFER:
            if( rm.vb[StartSlot].used == false )
                return -1;
            index = offsets[DXST_VERTEXBUFFER]+rm.vb[StartSlot].ilSlot;
            return index;
        case DXST_CONSTANTS:
            if( rm.cb[StartSlot].used == false )
                return -1;
            index = offsets[DXST_VERTEXBUFFER]+rm.cb[StartSlot].ilSlot;
            return index;
        case DXST_TEXTURE:
            if( rm.tex[StartSlot].used == false )
                return -1;
            index = offsets[DXST_VERTEXBUFFER]+rm.tex[StartSlot].ilSlot;
            return index;
        case DXST_UAV:
            if( rm.uav[StartSlot].used == false )
                return -1;
            index = offsets[DXST_UAV]+rm.uav[StartSlot].ilSlot;
            return index;
        case DXST_ATOMIC_COUNTER:
            if( rm.atomicCounter[StartSlot].used == false )
                return -1;
            index = offsets[DXST_UAV]+rm.atomicCounter[StartSlot].ilSlot;
            return index;
        case DXST_SAMPLER:
            if( rm.sampler[StartSlot].used == false )
                return -1;
            index = offsets[DXST_SAMPLER]+rm.sampler[StartSlot].ilSlot;
            return index;
        default:
            return -1;
        }
    }
private:
    GR_DESCRIPTOR_SET_SLOT_TYPE getSlotType(const GR_UINT32 offsets[DXST_COUNT+1], GR_UINT32 idx) const
    {
        if( idx<offsets[DXST_UAV])
            return GR_SLOT_SHADER_RESOURCE;
        else if( idx<offsets[DXST_SAMPLER])
            return GR_SLOT_SHADER_UAV;
        else if( idx<offsets[DXST_COUNT])
            return GR_SLOT_SHADER_SAMPLER;
        else
            return GR_SLOT_UNUSED;
    }
    GR_UINT                 m_gpu;

    // resource mapping to allow DX style binding of resources
    ILC_DX_RESOURCE_MAPPING    m_resourceMapping[SHADERSTAGE_COUNT];

    // start offsets in descriptorsets for each DX slot type
    GR_UINT32               m_Offsets[SHADERSTAGE_COUNT][DXST_COUNT+1];
   
    GR_ENUM                 m_pipelineBindPoint;

    GR_PIPELINE             m_Pipeline;
    GR_GPU_MEMORY           m_PipelineMem;
    GR_GPU_SIZE             m_PipelineOffset;
};