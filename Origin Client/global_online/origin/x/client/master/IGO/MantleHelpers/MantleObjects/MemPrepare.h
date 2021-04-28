#pragma once

#include "DrawCommand.h"

class MemPrepare : public DrawCommand
{
public:
    MemPrepare(GR_MEMORY_STATE_TRANSITION& transition, int priority = 0);

    GR_RESULT           execute( CommandBuffer *,RenderPass * );
    unsigned int        countCommandsRec(DRAW_CALL_TYPE);

private:
    GR_MEMORY_STATE_TRANSITION  m_transition;
};