#include <math.h>
#include "../MantleObjects.h"
#include "..\..\IGOLogger.h"

Drawable::Drawable( ):
    DrawCommand(0),
    m_DescriptorSet( NULL ),
    m_memRefCount( 0 ),
    m_memRefs( NULL ),
    m_Texture( NULL )
{
    ZeroMemory(&m_bufferView, sizeof(m_bufferView));
}

Drawable::~Drawable()
{
    if( NULL != m_memRefs )
    {
        delete[] m_memRefs;
        m_memRefs = NULL;
    }
    m_memRefCount = 0;
}

GR_RESULT Drawable::init( CommandBuffer* cmdBuffer, Mesh* mesh, Pipeline* pipeline)
{
    m_mesh = mesh;
    m_pipeline = pipeline;

    updateMemRefs();
   
//////////////////////////////////////////////////////////////
    
    return GR_SUCCESS;
}

GR_RESULT Drawable::updateMemRefs()
{
    m_memRefCount = 1; //pipeline

    // if we need to bind a vertexbuffer it is included in the reflectioncount
    if( m_mesh->ibMemory() ) ++m_memRefCount;
    if( m_DescriptorSet && m_DescriptorSet->memory() ) ++m_memRefCount;
    if(constantBufferMem()) ++m_memRefCount;

    m_memRefCount += m_pipeline->resourceCount();
    
    if( NULL != m_memRefs )
    {
        delete[] m_memRefs;
    }
    m_memRefs = new GR_MEMORY_REF[m_memRefCount];

    GR_UINT32 index = 0;
    {
        m_memRefs[index].mem = m_pipeline->memory();
        m_memRefs[index].flags = GR_MEMORY_REF_READ_ONLY;

        OriginIGO::IGOLogDebug("m_pipeline->memory(): %p", m_pipeline->memory());

        ++index;
    }

    if( m_DescriptorSet && m_DescriptorSet->memory() )
    {
        m_memRefs[index].mem = m_DescriptorSet->memory();
        m_memRefs[index].flags = GR_MEMORY_REF_READ_ONLY;
        OriginIGO::IGOLogDebug("m_DescriptorSet->memory(): %p", m_DescriptorSet->memory());
        ++index;
    }
        
    if( m_mesh->ibMemory() )
    {
        m_memRefs[index].mem = m_mesh->ibMemory();
        m_memRefs[index].flags = GR_MEMORY_REF_READ_ONLY;
        OriginIGO::IGOLogDebug("m_mesh->ibMemory(): %p", m_mesh->ibMemory());
        ++index;
    }
    
    if( constantBufferMem() )
    {
        m_memRefs[index].mem = constantBufferMem();
        m_memRefs[index].flags = 0;
        OriginIGO::IGOLogDebug("constantBufferMem(): %p", constantBufferMem());
        ++index;
    }

    if( m_DescriptorSet )
    {
        for( unsigned int i=0; i<m_DescriptorSet->getNumSlots(); ++i )
        {
            if(m_DescriptorSet->getSlotType(i) == GR_SLOT_SHADER_RESOURCE)
            {
                // could be NULL if no resource is bound or slot is used for grCmdBindDynamicMemoryView
                if(NULL == m_DescriptorSet->getAttachedMemory(i))
                {
                    m_memRefs[index].mem = m_DescriptorSet->getAttachedMemory(i);
                    m_memRefs[index].flags = GR_MEMORY_REF_READ_ONLY;
                    ++index;
                }
            }            
            else if(m_DescriptorSet->getSlotType(i) == GR_SLOT_SHADER_UAV)
            {
                // could be NULL if no resource is bound or slot is used for grCmdBindDynamicMemoryView
                if(NULL == m_DescriptorSet->getAttachedMemory(i))
                {
                    m_memRefs[index].mem = m_DescriptorSet->getAttachedMemory(i);
                    m_memRefs[index].flags = 0;
                    ++index;
                }
            }
            
        }
    }

    // adjust refcount to actually used memory references
    m_memRefCount = index;
    return GR_SUCCESS;
}

GR_RESULT Drawable::execute( CommandBuffer* cmdBuffer, RenderPass* )
{
    cmdBuffer->addMemReferences( m_memRefCount, m_memRefs );
    cmdBuffer->bindPipeline( m_pipeline );

    if(m_DescriptorSet)
        cmdBuffer->bindDescriptorSet( m_DescriptorSet );
    
    if( m_bufferView.mem )
    {
        GR_MEMORY_VIEW_ATTACH_INFO attachInfo = constantBufferView();
        _grCmdBindDynamicMemoryView(cmdBuffer->cmdBuffer(), GR_PIPELINE_BIND_POINT_GRAPHICS,  &attachInfo);
    }
    
    if( m_mesh->indexCount() > 0 )
    {
        cmdBuffer->bindIndexData( m_mesh->ibMemory(), 0, m_mesh->indexType() );
        _grCmdDrawIndexed( cmdBuffer->cmdBuffer(), 0, m_mesh->indexCount(), 0, 0, 1 );
    }
    else
    {
        _grCmdDraw( cmdBuffer->cmdBuffer(), 0, m_mesh->vertexCount(), 0, 1 );
    }

    return GR_SUCCESS;
}

unsigned int Drawable::countCommandsRec(DRAW_CALL_TYPE dct)
{
    return dct == DCT_DRAWABLE ? 1 : 0;
}