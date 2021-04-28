#pragma once

#include "CommandBuffer.h"

class RenderPass;

typedef enum _DrawCallType
{
    DCT_DRAWABLE,
    DCT_QUERY,
    DCT_PREPARE,
}DRAW_CALL_TYPE;

class DrawCommand
{
public:
    DrawCommand(int priority) : m_priority(priority){}
    virtual GR_RESULT execute( CommandBuffer*, RenderPass* ) = 0;
    virtual unsigned int countCommandsRec(DRAW_CALL_TYPE) = 0;

    int priority()const{return m_priority;}
private:
    int m_priority;
};

extern bool sortDrawCommands(DrawCommand* const ls, DrawCommand* const rs);