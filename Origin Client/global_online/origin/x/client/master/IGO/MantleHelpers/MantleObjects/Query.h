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

class Query : public DrawCommand
{
public:
    typedef enum
    {
        QUERY_OCCLUSION
    }QUERY_TYPE;

    Query(  );
    virtual ~Query();
    virtual GR_RESULT   init( CommandBuffer* cmdBuffer, QUERY_TYPE type );

    GR_RESULT           add( DrawCommand* drawable );
    GR_RESULT           execute( CommandBuffer*, RenderPass* );
    unsigned int        countCommandsRec(DRAW_CALL_TYPE);

    GR_UINT64           getResult( );
    
private:

protected:

private:
    RenderPass*                 m_Pass;
    GR_UINT32                   m_QueryID;

    QUERY_TYPE                  m_Type;
    eastl::vector<DrawCommand*>   m_drawCommands;
    unsigned int                m_Result;
    unsigned int                m_Index;
};
