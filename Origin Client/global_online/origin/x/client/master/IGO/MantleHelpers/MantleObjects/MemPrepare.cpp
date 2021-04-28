#include "MantleAppWsi.h"
#include "MemPrepare.h"
#include "..\..\IGOLogger.h"

MemPrepare::MemPrepare(GR_MEMORY_STATE_TRANSITION& transition, int priority) :
    DrawCommand(priority),
    m_transition(transition)
{
}

GR_RESULT MemPrepare::execute(CommandBuffer *cmdBuffer,RenderPass *)
{
    _grCmdPrepareMemoryRegions( cmdBuffer->cmdBuffer(), 1, &m_transition );

    GR_MEMORY_REF mem[] = 
    {
        { m_transition.mem,  0}
    };

    OriginIGO::IGOLogDebug("MemPrepare::execute %p", mem[0].mem);

    cmdBuffer->addMemReferences(sizeof(mem) / sizeof(mem[0]), mem);
    return GR_SUCCESS;
}

unsigned int MemPrepare::countCommandsRec(DRAW_CALL_TYPE dct)
{
    return dct == DCT_PREPARE ? 1 : 0;
}
