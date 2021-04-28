#pragma once
#include <mantle.h>
#include "DrawCommand.h"
#include "Mesh.h"
#include "Pipeline.h"

class RenderPass;
class CommandBuffer;
class RenderState;
class DescriptorSet;
class ConstantBuffer;
class Texture;
class Sampler;

class Drawable : public DrawCommand
{
public:
    Drawable(  );
    virtual ~Drawable();
    virtual GR_RESULT   init( CommandBuffer* cmdBuffer, Mesh* mesh, Pipeline* pipeline );
	void dealloc(GR_CMD_BUFFER cmdBuffer) {};

    Mesh*               GetMesh(){return m_mesh;}
    Pipeline*           GetPipeline(){return m_pipeline;}

    GR_RESULT           execute( CommandBuffer*, RenderPass* );
    unsigned int        countCommandsRec(DRAW_CALL_TYPE);

    void                        setConstantBuffer(GR_MEMORY_VIEW_ATTACH_INFO* bufferView){m_bufferView = *bufferView; updateMemRefs();}
    GR_MEMORY_VIEW_ATTACH_INFO  constantBufferView(){return m_bufferView;}
    GR_GPU_MEMORY               constantBufferMem(){return m_bufferView.mem;}

    GR_RESULT   setDescriptorSet(DescriptorSet* descriptorSet)
    {
        m_DescriptorSet = descriptorSet;
        updateMemRefs();
        return GR_SUCCESS;
    }

    GR_RESULT BindBuffer( const char* name, ConstantBuffer* buffer);
    GR_RESULT BindTexture( const char* name, Texture* texture);
    GR_RESULT BindSampler( const char* name, Sampler* sampler);

private:
    GR_RESULT updateMemRefs();

protected:
    GR_MEMORY_VIEW_ATTACH_INFO  m_bufferView;

private:
    Mesh*                       m_mesh;
    Texture*                    m_Texture;
    Sampler*                    m_Sampler;
    
    Pipeline*                   m_pipeline;
    DescriptorSet*              m_DescriptorSet;

    GR_UINT32                   m_memRefCount;
    GR_MEMORY_REF*              m_memRefs;
};
