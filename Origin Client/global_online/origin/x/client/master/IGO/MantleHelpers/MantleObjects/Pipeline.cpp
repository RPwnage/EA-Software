#include <math.h>
#include "..\MantleAppWsi.h"
#include "Pipeline.h"
#include "GpuMemMgr.h"
#include "CommandBuffer.h"
#include "DescriptorSet.h"
#include "Shader.h"
#include "Defines.h"

Pipeline::Pipeline() :
    m_gpu(0),
    m_pipelineBindPoint(GR_PIPELINE_BIND_POINT_GRAPHICS),
    m_Pipeline( GR_NULL_HANDLE ),
    m_PipelineMem(GR_NULL_HANDLE),
    m_PipelineOffset(0)
{
    ZeroMemory(&m_resourceMapping, sizeof(m_resourceMapping));
    for( unsigned int i = 0; i<SHADERSTAGE_COUNT; ++i)
    {
        ZeroMemory(&m_Offsets[i], sizeof(m_Offsets[i]));
    }
}

Pipeline::~Pipeline()
{

}

void Pipeline::dealloc(GR_CMD_BUFFER cmdBuffer)
{
    SAFE_DESTROY(m_Pipeline, cmdBuffer);
    SAFE_FREE(m_PipelineMem, cmdBuffer);
}

GR_RESULT Pipeline::init( GR_GRAPHICS_PIPELINE_CREATE_INFO& pipeInfo, Shader* vs, Shader* hs, Shader* ds, Shader* gs, Shader* ps, GR_UINT gpu )
{
    m_gpu = gpu;

    // update pipelineInfo using the information from vs and ps
    GR_GRAPHICS_PIPELINE_CREATE_INFO pi = pipeInfo;
    pi.vs = vs->getPipelineShaderInfo();
    m_resourceMapping[VERTEX_SHADER] = vs->getResMap();
    for( unsigned int i = 0; i<DXST_COUNT+1; ++i )
    {
        m_Offsets[VERTEX_SHADER][i] = vs->baseSlot(i);
    }

    GR_UINT32 baseSlot = vs->numSlots();
    if(hs)
    {
        m_resourceMapping[HULL_SHADER] = hs->getResMap();
        pi.hs = hs->getPipelineShaderInfo();
        if(0!=pi.hs.descriptorSetMapping[0].descriptorCount)
        {
            pi.hs.descriptorSetMapping[0].descriptorCount += baseSlot;

            GR_DESCRIPTOR_SLOT_INFO* descriptorSlots = new GR_DESCRIPTOR_SLOT_INFO[pi.hs.descriptorSetMapping[0].descriptorCount];
            pi.hs.descriptorSetMapping[0].pDescriptorInfo = descriptorSlots;
            for( GR_UINT32 i = 0; i<baseSlot; ++i )
            {
                descriptorSlots[i].slotObjectType = GR_SLOT_UNUSED;
                descriptorSlots[i].shaderEntityIndex = 0;
            }
            memcpy(&descriptorSlots[baseSlot], hs->getPipelineShaderInfo().descriptorSetMapping[0].pDescriptorInfo, sizeof(GR_DESCRIPTOR_SLOT_INFO)*hs->getPipelineShaderInfo().descriptorSetMapping[0].descriptorCount);
        }

        for( unsigned int i = 0; i<DXST_COUNT+1; ++i )
        {
            m_Offsets[HULL_SHADER][i] = baseSlot + hs->baseSlot(i);
        }

        baseSlot += hs->numSlots();
    }
    
    if(ds)
    {
        m_resourceMapping[DOMAIN_SHADER] = ds->getResMap();
        pi.ds = ds->getPipelineShaderInfo();
        if(0!=pi.ds.descriptorSetMapping[0].descriptorCount)
        {
            pi.ds.descriptorSetMapping[0].descriptorCount += baseSlot;
            GR_DESCRIPTOR_SLOT_INFO* descriptorSlots = new GR_DESCRIPTOR_SLOT_INFO[pi.ds.descriptorSetMapping[0].descriptorCount];
            pi.ds.descriptorSetMapping[0].pDescriptorInfo = descriptorSlots;
            for( GR_UINT32 i = 0; i<baseSlot; ++i )
            {
                descriptorSlots[i].slotObjectType = GR_SLOT_UNUSED;
                descriptorSlots[i].shaderEntityIndex = 0;
            }
            memcpy(&descriptorSlots[baseSlot], ds->getPipelineShaderInfo().descriptorSetMapping[0].pDescriptorInfo, sizeof(GR_DESCRIPTOR_SLOT_INFO)*ds->getPipelineShaderInfo().descriptorSetMapping[0].descriptorCount);
        }

        for( unsigned int i = 0; i<DXST_COUNT+1; ++i )
        {
            m_Offsets[DOMAIN_SHADER][i] = baseSlot + ds->baseSlot(i);
        }

        baseSlot += ds->numSlots();
    }

    if(gs)
    {
        m_resourceMapping[GEOMETRY_SHADER] = gs->getResMap();
        pi.gs = gs->getPipelineShaderInfo();
        if(0!=pi.gs.descriptorSetMapping[0].descriptorCount)
        {
            pi.gs.descriptorSetMapping[0].descriptorCount += baseSlot;
            GR_DESCRIPTOR_SLOT_INFO* descriptorSlots = new GR_DESCRIPTOR_SLOT_INFO[pi.gs.descriptorSetMapping[0].descriptorCount];
            pi.gs.descriptorSetMapping[0].pDescriptorInfo = descriptorSlots;
            for( GR_UINT32 i = 0; i<baseSlot; ++i )
            {
                descriptorSlots[i].slotObjectType = GR_SLOT_UNUSED;
                descriptorSlots[i].shaderEntityIndex = 0;
            }
            memcpy(&descriptorSlots[baseSlot], gs->getPipelineShaderInfo().descriptorSetMapping[0].pDescriptorInfo, sizeof(GR_DESCRIPTOR_SLOT_INFO)*gs->getPipelineShaderInfo().descriptorSetMapping[0].descriptorCount);
        }

        for( unsigned int i = 0; i<DXST_COUNT+1; ++i )
        {
            m_Offsets[GEOMETRY_SHADER][i] = baseSlot + gs->baseSlot(i);
        }

        baseSlot += gs->numSlots();
    }
    
    if(ps)
    {
        m_resourceMapping[PIXEL_SHADER] = ps->getResMap();
        pi.ps = ps->getPipelineShaderInfo();
        if(0!=pi.ps.descriptorSetMapping[0].descriptorCount)
        {
            pi.ps.descriptorSetMapping[0].descriptorCount += baseSlot;
            GR_DESCRIPTOR_SLOT_INFO* descriptorSlots = new GR_DESCRIPTOR_SLOT_INFO[pi.ps.descriptorSetMapping[0].descriptorCount];
            pi.ps.descriptorSetMapping[0].pDescriptorInfo = descriptorSlots;
            for( GR_UINT32 i = 0; i<baseSlot; ++i )
            {
                descriptorSlots[i].slotObjectType = GR_SLOT_UNUSED;
                descriptorSlots[i].shaderEntityIndex = 0;
            }
            memcpy(&descriptorSlots[baseSlot], ps->getPipelineShaderInfo().descriptorSetMapping[0].pDescriptorInfo, sizeof(GR_DESCRIPTOR_SLOT_INFO)*ps->getPipelineShaderInfo().descriptorSetMapping[0].descriptorCount);
        }

        for( unsigned int i = 0; i<DXST_COUNT+1; ++i )
        {
            m_Offsets[PIXEL_SHADER][i] = baseSlot + ps->baseSlot(i);
        }

        baseSlot += ps->numSlots();
    }

    GR_RESULT result = _grCreateGraphicsPipeline(OriginIGO::getMantleDevice(m_gpu), &pi, &m_Pipeline);
    if (result == GR_SUCCESS)
    {
        result = GpuMemMgr::Instance(m_gpu)->allocAndBindGpuMem(m_Pipeline, 0, &m_PipelineMem, &m_PipelineOffset);
    }
    else
    {
        OutputDebugString(TEXT(__FUNCTION__)L": grCreateGraphicsPipeline failed!\n");
    }

    m_pipelineBindPoint = GR_PIPELINE_BIND_POINT_GRAPHICS;
    return result;
}

GR_RESULT Pipeline::init( GR_COMPUTE_PIPELINE_CREATE_INFO& pipeInfo, Shader* cs )
{
    m_resourceMapping[COMPUTE_SHADER] = cs->getResMap();
    for( unsigned int i = 0; i<DXST_COUNT+1; ++i )
    {
        m_Offsets[COMPUTE_SHADER][i] = cs->baseSlot(i);
    }

    // update pipelineInfo using the information from vs and ps
    GR_COMPUTE_PIPELINE_CREATE_INFO pi = pipeInfo;
    pi.cs = cs->getPipelineShaderInfo();

    GR_RESULT result = _grCreateComputePipeline(OriginIGO::getMantleDevice(m_gpu), &pi, &m_Pipeline);
    if (result == GR_SUCCESS)
    {
        result = GpuMemMgr::Instance(m_gpu)->allocAndBindGpuMem(m_Pipeline, 0, &m_PipelineMem, &m_PipelineOffset);
    }
    else
    {
        OutputDebugString(TEXT(__FUNCTION__)L": grCreateComputePipeline failed!\n");
    }

    m_pipelineBindPoint = GR_PIPELINE_BIND_POINT_COMPUTE;
    return result;
}

DescriptorSet*    Pipeline::createDescriptorSet()
{
    DescriptorSet* ds = new DescriptorSet();
    ds->init( this );
    return ds;
}
