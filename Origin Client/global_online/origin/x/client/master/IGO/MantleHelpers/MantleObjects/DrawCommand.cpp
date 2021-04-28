#include "DrawCommand.h"


bool sortDrawCommands(DrawCommand* const ls, DrawCommand* const rs)
{
    return ls->priority()<rs->priority();
}