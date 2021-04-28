#pragma once

#include <windows.h>
#include <io.h>
#include <direct.h>
#include <stdio.h>
#include <tchar.h>
#include <crtdbg.h>
#include <stdlib.h>
#include <string>

#include "ilconverter.h"

#include "CommandBuffer.h"

typedef enum _SHADER_STAGE
{
    VERTEX_SHADER,
    HULL_SHADER,
    DOMAIN_SHADER,
    GEOMETRY_SHADER,
    PIXEL_SHADER,
    COMPUTE_SHADER,
    SHADERSTAGE_COUNT
}SHADER_STAGE;

typedef enum _DX_SLOTTYPE
{
    DXST_VERTEXBUFFER,
    DXST_CONSTANTS,
    DXST_TEXTURE,
    DXST_UAV,
    DXST_ATOMIC_COUNTER,
    DXST_SAMPLER,
    DXST_COUNT
}DX_SLOTTYPE;

typedef struct _DX_SLOT_INFO
{
    DX_SLOTTYPE     slotObjectType;
    unsigned int    slotIndex;
}DX_SLOT_INFO;

class Shader
{
public:
    Shader();
    virtual ~Shader();
	void dealloc(GR_CMD_BUFFER cmd);
	void    initFromHLSL(const WCHAR* wsFilename, const WCHAR* wsCommandLine, ILC_VS_INPUT_LAYOUT& vsInputLayout, const DX_SLOT_INFO* dynamicMapping, GR_UINT gpu);
	GR_RESULT initFromBinaryHLSL(const char* blob, unsigned int blobSize, ILC_VS_INPUT_LAYOUT& vsInputLayout, const DX_SLOT_INFO* dynamicMapping, GR_UINT gpu);

    GR_SHADER   shader()
    {
        return m_pipelineShaderInfo.shader;
    }
        
    GR_PIPELINE_SHADER getPipelineShaderInfo()
    {
        return m_pipelineShaderInfo;
    }

    unsigned int numSlots()
    {
        return baseSlot(DXST_COUNT);
    }

    unsigned int baseSlot(GR_UINT32 dx_type)
    {
        return  m_DxOffsets[dx_type];
    }
    
    const ILC_DX_RESOURCE_MAPPING& getResMap() const
    {
        return m_ResMap;
    }
private:
    void                        ProcessFile( const WCHAR* pFileName, ILC_VS_INPUT_LAYOUT& vsInputLayout, ILC_DX_RESOURCE_MAPPING& vsResMap);
    void                        countUsedDescriptorSlots();
    void                        initDescriptorSlotInfo(const DX_SLOT_INFO* dynamic);
    void                        initDescriptors(unsigned int numDescriptors, GR_DESCRIPTOR_SLOT_INFO* descriptors, const DX_SLOT_INFO*);
    
    static VOID* ILC_STDCALL    MemAlloc( SIZE_T dataSize, ILC_ALLOC_TYPE  allocType);
    static void                 MemFree(void* pMem);

private:
    GR_UINT                 m_gpu;
    GR_UINT32               m_DxOffsets[DXST_COUNT+1];

    GR_PIPELINE_SHADER      m_pipelineShaderInfo;

    ILC_DX_RESOURCE_MAPPING    m_ResMap;
};