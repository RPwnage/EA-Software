#pragma once
#include <mantle.h>
#include "EASTL/hash_set.h"

class Pipeline;
class DescriptorSet;

class CommandBuffer
{
public:
    enum STATE
    {
        CS_UNUSED,
        CS_OPEN,
        CS_READY,
        CS_USED,
    };


	CommandBuffer();
	CommandBuffer(GR_CMD_BUFFER cmd);
	virtual ~CommandBuffer();
	void dealloc(GR_CMD_BUFFER cmd) {};

    GR_RESULT init(GR_QUEUE_TYPE queue, GR_UINT gpu);
	GR_RESULT getMemRefs(GR_MEMORY_REF**  m_data, GR_UINT32 *m_size, bool force);
	GR_RESULT clearMemRefs(bool free);


    GR_RESULT begin(GR_FLAGS flags);
    GR_RESULT end();
    GR_RESULT submit(GR_QUEUE queue);
    GR_RESULT reset();

    GR_RESULT addMemReferences( GR_UINT32 numReferences, GR_MEMORY_REF* references );

    // mantle binding operations: filter redundant grCmds
    GR_RESULT bindPipeline( Pipeline*, GR_PIPELINE_BIND_POINT bindPoint = GR_PIPELINE_BIND_POINT_GRAPHICS );
    GR_RESULT bindDescriptorSet( DescriptorSet* );
    GR_RESULT bindIndexData( GR_GPU_MEMORY mem, GR_GPU_SIZE offset, GR_ENUM indexType);

    GR_CMD_BUFFER   cmdBuffer();
    STATE           state();
    void            waitIdle();

private:
    typedef struct _MEMREF
    {
        GR_MEMORY_REF   ref;
        _MEMREF() { memset(&ref, 0, sizeof(ref)); } // need a default constructor to bypass warning when instantiating iterator for eastl::hash_set
        _MEMREF(GR_MEMORY_REF& r):ref(r){};
        operator size_t()const { return (size_t)(ref.mem); }
        bool operator< ( _MEMREF const& i)const  { return (ref.mem<i.ref.mem); }
        bool operator== ( _MEMREF const& i)const { return (ref.mem==i.ref.mem); }
        
        class Hash
        {
        public:
            size_t operator()(const _MEMREF &v) const
            {
                return *(size_t*)(&v.ref.mem);
            }
        };
    }MEMREF;
    

    // commandbuffer
    GR_UINT             m_gpu;
    GR_QUEUE_TYPE       m_type;
    GR_CMD_BUFFER       m_cmdBuffer;
    GR_FENCE            m_fence;
    STATE               m_cmdBufferState;

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
    bool                        m_bDirty;
	bool						m_freeCmdBuffer;
};