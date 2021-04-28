#pragma once

#include "..\MantleAppWsi.h"
#include "RenderTarget.h"
#include "Drawable.h"
#include "RenderState.h"
#include "CommandbufferRing.h"
#include "DrawCommand.h"

class RenderPass
{
public:
    RenderPass();
    virtual ~RenderPass();
	void dealloc(GR_CMD_BUFFER cmdBuffer);
    GR_RESULT       init( CommandBuffer* cmdBuffer, RenderState*, unsigned int rtCount, RenderTarget**, float* clearColor, GR_UINT gpu );

//    GR_RESULT       addDrawable( CommandBuffer* cmdBuffer, Drawable* drawable );
	GR_RESULT       add(DrawCommand* drawable);
	GR_RESULT       remove(DrawCommand* drawable);

	GR_RESULT		bakeCmdBuffer(CommandBuffer *cmd, GR_IMAGE_STATE currentState, GR_IMAGE_STATE stateAfterRendering, GR_MEMORY_REF** data, GR_UINT32 *size, bool repeatable = false);
		
	GR_RESULT       bakeCmdBuffer(bool final, GR_IMAGE_STATE state = GR_IMAGE_STATE_UNINITIALIZED_TARGET);
    GR_RESULT       submitToQueue( GR_QUEUE );
    void            waitIdle( );

    void            setClearColor( float* clearColor)
    {
        if(NULL!=clearColor)
        {
            m_clearFlags |= CLEAR_COLOR;
            memcpy(m_clearColor, clearColor, sizeof(m_clearColor));
        }
        else
        {
            m_clearFlags &= ~CLEAR_COLOR;
        }
    }

    void updateRenderTarget(unsigned int n, RenderTarget* renderTarget)
    {
        // create new comandbuffer
        m_renderTarget[n] = renderTarget;
    }

    RenderTarget* renderTarget(unsigned int n)const
    {
        return m_renderTarget[n];
    }

    GR_MEMORY_VIEW_ATTACH_INFO  getConstantsView() const
    {
        return m_cbConstants->view();
    }
    GR_GPU_MEMORY   getConstantsMem() const
    {
        return m_cbConstants->memory();
    }
    void            setConstants(ConstantBuffer* cb);

    GR_QUERY_POOL   getQueryPool() const
    {
        return m_queryPool; 
    }
    unsigned int    getQuery()
    {
        return m_numQueries++; 
    }
    void            allocateQueryBuffer();
    void            pollQueryResults();
    GR_UINT64       getQueryResult(GR_UINT32 queryID);
    
    // destroy
private:
    enum CLEAR_FLAGS
    {
        CLEAR_NONE = 0,
        CLEAR_COLOR = 1,
    };

	GR_CMD_BUFFER	m_real_cmd_buffer;



	typedef struct _MEMREF
	{
		GR_MEMORY_REF   ref;
        _MEMREF() { memset(&ref, 0, sizeof(ref)); } // need a default constructor to bypass warning when instantiating iterator for eastl::hash_set
		_MEMREF(GR_MEMORY_REF& r) :ref(r){};
		operator size_t()const { return (size_t)(ref.mem); }
		bool operator< (_MEMREF const& i)const  { return (ref.mem<i.ref.mem); }
		bool operator== (_MEMREF const& i)const { return (ref.mem == i.ref.mem); }

		class Hash
		{
		public:
			size_t operator()(const _MEMREF &v) const
			{
				return *(size_t*)(&v.ref.mem);
			}
		};
	}MEMREF;

	// memoryreferences
	typedef struct MEMORY_REFERENCES_TYPE
	{
		GR_MEMORY_REF*  m_data;
		GR_UINT32       m_size;
		GR_UINT32       m_reservedSize;
	}MEMORY_REFERENCES;

	typedef eastl::hash_set<MEMREF, MEMREF::Hash> MemRefSet;

	MemRefSet                   m_memRefSet;
	MEMORY_REFERENCES           m_memRefs;


    GR_UINT             m_gpu;

    ConstantBuffer*     m_cbConstants;

    GR_UINT32           m_clearFlags;
    float             m_clearColor[4];

    RenderState*                m_renderState;
    eastl::vector<DrawCommand*>   m_drawCommands;
    
    unsigned int        m_numTargets;
    RenderTarget*       m_renderTarget[8];
    CommandBuffer*      m_cmdBuffer;

    bool                m_bQueryResultsDirty;
    unsigned int        m_numQueries;
    unsigned int        m_maxQueries;
    GR_UINT64*          m_QueryData;
    GR_QUERY_POOL       m_queryPool;
    GR_GPU_MEMORY       m_queryPoolMem;
    GR_GPU_SIZE         m_queryPoolOffset;
	bool				m_freeCmdBuffer;
};
