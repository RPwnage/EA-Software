#include <math.h>
#include "../MantleObjects.h"
#include "EASTL/algorithm.h"
#include "EASTL/sort.h"

Query::Query( ):
    DrawCommand(0),
    m_Pass(NULL)
{
}

Query::~Query()
{
}

GR_RESULT Query::init(CommandBuffer* cmdBuffer, QUERY_TYPE type)
{    
    return GR_SUCCESS;
}

GR_RESULT Query::add( DrawCommand* drawCommand )
{
    m_drawCommands.push_back( drawCommand );
    return GR_SUCCESS;
}

GR_RESULT Query::execute( CommandBuffer* cmdBuffer, RenderPass* pass )
{
    m_Pass = pass;
    GR_QUERY_POOL queryPool = pass->getQueryPool();
    m_QueryID = pass->getQuery();

#ifdef RESET_EACH_QUERY
    _grCmdResetQueryPool( cmdBuffer->cmdBuffer(), queryPool, m_QueryID, 1);
#endif

	// no sorting needed right now!
    //eastl::sort( m_drawCommands.begin(), m_drawCommands.end(), sortDrawCommands );
 
    _grCmdBeginQuery(cmdBuffer->cmdBuffer(), queryPool, m_QueryID, 0);
    
    eastl::vector<DrawCommand*>::iterator itb = m_drawCommands.begin();
    eastl::vector<DrawCommand*>::iterator ite = m_drawCommands.end();
    for(eastl::vector<DrawCommand*>::iterator it = itb; it<ite; ++it)
    {
        (*it)->execute( cmdBuffer, pass );
    }

    _grCmdEndQuery(cmdBuffer->cmdBuffer(), queryPool, m_QueryID);

    return GR_SUCCESS;
}

unsigned int Query::countCommandsRec(DRAW_CALL_TYPE dct)
{
    unsigned int ret = (dct == DCT_QUERY) ? 1 : 0;
    eastl::vector<DrawCommand*>::iterator itb = m_drawCommands.begin();
    eastl::vector<DrawCommand*>::iterator ite = m_drawCommands.end();
    for(eastl::vector<DrawCommand*>::iterator it = itb; it<ite; ++it)
    {
        ret += (*it)->countCommandsRec( dct );
    }

    return ret;
}


GR_UINT64 Query::getResult()
{
    if( NULL != m_Pass )
        return m_Pass->getQueryResult(m_QueryID);
    else
        return (GR_UINT)-1;
}