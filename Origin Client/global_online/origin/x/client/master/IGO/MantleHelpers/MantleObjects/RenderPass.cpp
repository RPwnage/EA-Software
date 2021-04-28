#include "../MantleObjects.h"
#include <math.h>
#include "EASTL/algorithm.h"
#include "EASTL/sort.h"
#include "../../HookAPI.h"

RenderPass::RenderPass() :
    m_renderState( NULL ),
    m_clearFlags(CLEAR_NONE),
    m_numTargets(0),
	m_cmdBuffer(NULL),
    m_bQueryResultsDirty(false),
    m_maxQueries(0),
    m_numQueries( 0 ),
    m_QueryData(NULL),
    m_queryPool( NULL ),
    m_queryPoolMem(NULL),
	m_freeCmdBuffer(false)
{
}

RenderPass::~RenderPass()
{
	if (NULL != m_cmdBuffer && m_freeCmdBuffer)
	{
		delete m_cmdBuffer;
		m_cmdBuffer = NULL;
	}
}


void RenderPass::dealloc(GR_CMD_BUFFER cmdBuffer)
{
	SAFE_DELETE_ARRAY(m_QueryData);
    SAFE_DESTROY(m_queryPool, cmdBuffer);
    SAFE_FREE(m_queryPoolMem, cmdBuffer);
}

GR_RESULT RenderPass::init(CommandBuffer* cmdBuffer, RenderState* renderState, unsigned int rtCount, RenderTarget** renderTarget, float* clearColor, GR_UINT gpu)
{
	GR_RESULT result = GR_SUCCESS;

	m_gpu = gpu;

	if (NULL != clearColor)
	{
		m_clearFlags |= CLEAR_COLOR;
		memcpy(m_clearColor, clearColor, sizeof(m_clearColor));
	}

	m_numTargets = rtCount;
	for (unsigned int rt = 0; rt < 8; ++rt)
	{
		m_renderTarget[rt] = rt < rtCount ? renderTarget[rt] : NULL;
	}

	m_freeCmdBuffer = false;
	/*
	if (cmdBuffer == NULL)
	{
		m_cmdBuffer = new CommandBuffer();
		m_cmdBuffer->init(GR_QUEUE_UNIVERSAL, m_gpu);
		m_freeCmdBuffer = true;
	}
	else*/
	{
		m_cmdBuffer = cmdBuffer;
	}

    m_renderState = renderState;

    return result;
}

GR_RESULT RenderPass::add( DrawCommand* drawCommand )
{
	for (eastl::vector<DrawCommand*>::iterator it = m_drawCommands.begin(); it != m_drawCommands.end(); ++it)
	{
		if ((*it) == drawCommand)
			return GR_SUCCESS;
	}
	
	m_drawCommands.push_back(drawCommand);
    return GR_SUCCESS;
}

GR_RESULT RenderPass::remove(DrawCommand* drawCommand)
{
	for (eastl::vector<DrawCommand*>::iterator it = m_drawCommands.begin(); it != m_drawCommands.end(); ++it)
	{
		if ((*it) == drawCommand)
		{
			it = m_drawCommands.erase(it);
			break;
		}
	}

	return GR_SUCCESS;
}



namespace OriginIGO{
	EXTERN_NEXT_FUNC(GR_VOID, grCmdBindTargetsHook, (
		GR_CMD_BUFFER                     cmdBuffer,
		GR_UINT                           colorTargetCount,
		const GR_COLOR_TARGET_BIND_INFO*  pColorTargets,
		const GR_DEPTH_STENCIL_BIND_INFO* pDepthTarget));
};


GR_RESULT RenderPass::bakeCmdBuffer(CommandBuffer *cmd, GR_IMAGE_STATE currentState, GR_IMAGE_STATE stateAfterRendering, GR_MEMORY_REF** data, GR_UINT32 *size, bool repeatable /* = false */)
{

	GR_RESULT result = GR_SUCCESS;
	if (!cmd)
		return result;

	GR_MEMORY_REF mem[8];
	GR_UINT32 memRefCnt = 0;
	for (unsigned int i = 0; i<m_numTargets; ++i)
	{
		if (m_renderTarget[i] && m_renderTarget[i]->memory())
		{
			mem[memRefCnt].mem = m_renderTarget[i]->memory();
            mem[memRefCnt].flags = 0;
            
            OriginIGO::IGOLogDebug("RenderPass::bakeCmdBuffer(m_renderTarget)  %p", mem[memRefCnt].mem);
            
            ++memRefCnt;
		}
	}

    result = cmd->addMemReferences(memRefCnt, mem);
		
	// prepare everything for render
	GR_COLOR_TARGET_BIND_INFO colorBind[8] = {};
	for (unsigned int i = 0; i<m_numTargets; ++i)
	{
        if (m_renderTarget[i])
        {
            if ((m_clearFlags & CLEAR_COLOR))
                m_renderTarget[i]->clear(cmd, m_clearColor);
            
            if (currentState != GR_IMAGE_STATE_UNINITIALIZED_TARGET)
                m_renderTarget[i]->syncState(currentState);

			colorBind[i].view = m_renderTarget[i]->targetView();
			colorBind[i].colorTargetState = GR_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL;
			
			m_renderTarget[i]->prepare(cmd, GR_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL);
		}
	}

	// MSAA state needs to be bound before targets
	m_renderState->bind(cmd);

	IGO_ASSERT(OriginIGO::grCmdBindTargetsHookNext != NULL);

	OriginIGO::grCmdBindTargetsHookNext(cmd->cmdBuffer(), m_numTargets, (m_numTargets == 0) ? NULL : colorBind, NULL);
	//_grCmdBindTargets( m_cmdBuffer->cmdBuffer(), m_numTargets, (m_numTargets==0)?NULL:colorBind, NULL );

	// no sorting needed right now!
	//eastl::sort( m_drawCommands.begin(), m_drawCommands.end(), sortDrawCommands );

	for (eastl::vector<DrawCommand*>::const_iterator it = m_drawCommands.begin(); it != m_drawCommands.end(); ++it)
	{
		result = (*it)->execute(cmd, this);
	}


	// set the expected state
	if (stateAfterRendering != GR_IMAGE_STATE_UNINITIALIZED_TARGET)
		m_renderTarget[0]->prepare(cmd, stateAfterRendering);

	result = cmd->getMemRefs(data, size, true);

	if (repeatable == false)
		m_drawCommands.clear();	// remove drawing commands

	return result;
}

GR_RESULT RenderPass::bakeCmdBuffer(bool finalize, GR_IMAGE_STATE state)
{
    GR_RESULT result = GR_SUCCESS;
	if (!m_cmdBuffer)
		return result;

    //CommandBuffer::STATE cmdBufState = m_cmdBuffer->state();
	
    if((m_cmdBuffer->state() == CommandBuffer::CS_READY) ||
       (m_cmdBuffer->state() == CommandBuffer::CS_USED))
    {
        m_cmdBuffer->reset();
    }
	
    //allocateQueryBuffer();
    
	result = m_cmdBuffer->begin(GR_CMD_BUFFER_OPTIMIZE_PIPELINE_SWITCH);

    if( result == GR_SUCCESS )
    {
		/*
        if( m_queryPool )
        {
            _grCmdResetQueryPool( m_cmdBuffer->cmdBuffer(), m_queryPool, 0, m_maxQueries);
            GR_MEMORY_REF queryMem = {0};
            queryMem.mem = m_queryPoolMem;
            m_cmdBuffer->addMemReferences( 1, &queryMem );
        }
		*/

        GR_MEMORY_REF mem[2];
        GR_UINT32 memRefCnt = 0;
        for( unsigned int i = 0; i<m_numTargets; ++i)
        {
            if(  m_renderTarget[i] && m_renderTarget[i]->memory() )
            {
                mem[memRefCnt].mem = m_renderTarget[i]->memory();
                mem[memRefCnt].flags = 0;
                OriginIGO::IGOLogDebug("RenderPass::bakeCmdBuffer(2)(m_renderTarget)  %p", mem[memRefCnt].mem);
                ++memRefCnt;
            }
        }
        m_cmdBuffer->addMemReferences( memRefCnt, mem );

        // clear rendertarget
		/*
        for( unsigned int i = 0; i<m_numTargets; ++i)
        {
            if( (m_clearFlags&CLEAR_COLOR) && (NULL!=m_renderTarget[i]) )
            {
                m_renderTarget[i]->clear( m_cmdBuffer, m_clearColor );
            }
        }
		*/

        // prepare everything for render
        GR_COLOR_TARGET_BIND_INFO colorBind[8] = {};
        for( unsigned int i = 0; i<m_numTargets; ++i)
        {
            if( m_renderTarget[i] )
            {
				colorBind[i].view = m_renderTarget[i]->targetView();
                colorBind[i].colorTargetState = GR_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL;
                m_renderTarget[i]->prepare( m_cmdBuffer, GR_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL );
            }
        }

        // MSAA state needs to be bound before targets
        m_renderState->bind( m_cmdBuffer );

		IGO_ASSERT(OriginIGO::grCmdBindTargetsHookNext != NULL);

		OriginIGO::grCmdBindTargetsHookNext(m_cmdBuffer->cmdBuffer(), m_numTargets, (m_numTargets == 0) ? NULL : colorBind, NULL);
        //_grCmdBindTargets( m_cmdBuffer->cmdBuffer(), m_numTargets, (m_numTargets==0)?NULL:colorBind, NULL );

		// no sorting needed right now!
		//eastl::sort( m_drawCommands.begin(), m_drawCommands.end(), sortDrawCommands );
        
		for (eastl::vector<DrawCommand*>::const_iterator it = m_drawCommands.begin(); it != m_drawCommands.end(); ++it)
        {
            (*it)->execute( m_cmdBuffer, this );
        }

        // perpare rendertarget for output to screen
        if( finalize )
        {
			if (state != GR_IMAGE_STATE_UNINITIALIZED_TARGET)
				m_renderTarget[0]->prepare(m_cmdBuffer, state);
			else
				m_renderTarget[0]->prepare(m_cmdBuffer, OriginIGO::getMantleDeviceFullscreen(m_gpu) ? (GR_IMAGE_STATE)GR_WSI_WIN_PRESENT_SOURCE_FLIP : (GR_IMAGE_STATE)GR_WSI_WIN_PRESENT_SOURCE_BLT);
			//m_renderTarget[0]->prepareAfterPresent(m_cmdBuffer);
		}

        result = m_cmdBuffer->end();

    }

    return result;
}

GR_RESULT RenderPass::submitToQueue( GR_QUEUE queue )
{
    //m_bQueryResultsDirty = true;

    GR_RESULT result = GR_NOT_READY;
	if (!m_cmdBuffer)
		return result;

    // only need to do this if pass contents changed significantly
    if((m_cmdBuffer->state() == CommandBuffer::CS_READY) ||
       (m_cmdBuffer->state() == CommandBuffer::CS_USED))
    {
        // submit commandbuffer
        result = m_cmdBuffer->submit( queue );
    }
    return result;
}
void RenderPass::waitIdle( )
{
    m_cmdBuffer->waitIdle();
}

void RenderPass::setConstants(ConstantBuffer* cb)
{
    m_cbConstants = cb;
}

void RenderPass::allocateQueryBuffer()
{
	/*
    unsigned int numQueries = 0;

    eastl::vector<DrawCommand*>::iterator itb = m_drawCommands.begin();
    eastl::vector<DrawCommand*>::iterator ite = m_drawCommands.end();
    for(eastl::vector<DrawCommand*>::iterator it = itb; it != ite; ++it)
    {
        numQueries += (*it)->countCommandsRec(DCT_QUERY);
    }

    if( numQueries > m_maxQueries )
    {
        SAFE_DELETE_ARRAY(m_QueryData);
        SAFE_DESTROY(m_queryPool );
        SAFE_FREE(m_queryPoolMem );

        m_maxQueries = numQueries;
        m_numQueries = 0;
        m_QueryData = new GR_UINT64[m_maxQueries];
        GR_QUERY_POOL_CREATE_INFO qpCInfo={0};
        qpCInfo.queryType = GR_QUERY_OCCLUSION;
        qpCInfo.slots = m_maxQueries+10;
        GR_RESULT result = _grCreateQueryPool( OriginIGO::getMantleDevice(m_gpu), &qpCInfo, &m_queryPool);
        if (result == GR_SUCCESS)
        {
            result = GpuMemMgr::Instance(m_gpu)->allocAndBindGpuMem(m_queryPool, 0, &m_queryPoolMem, &m_queryPoolOffset);
        }
    }
	*/
}
void RenderPass::pollQueryResults()
{
	/*
    if(m_bQueryResultsDirty)
    {
        GR_SIZE size = sizeof(GR_UINT64) * m_numQueries;

        GR_RESULT result = GR_NOT_READY;
        while( GR_NOT_READY == result)
        {
            result = _grGetQueryPoolResults(
                m_queryPool,
                0,
                m_numQueries,
                &size,
                m_QueryData);
        }
        m_bQueryResultsDirty = false;
    }
	*/
}

GR_UINT64 RenderPass::getQueryResult(unsigned int id)
{
    //pollQueryResults();
    return m_QueryData[id];
}
