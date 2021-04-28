#include "Common.h"

namespace lsx
{


uint32_t GetIndexFromEnum(EnumToIndexMap * map, uint32_t size, int32_t enumValue, uint32_t _default)
{
    for(uint32_t i=0; i<size; i++)
    {
        if(map[i].enumValue == enumValue)
        {
            return map[i].index;
        }
    }
    return _default;
}

int32_t GetEnumFromIndex(EnumToIndexMap * map, uint32_t size, uint32_t index, int32_t _default)
{
    for(uint32_t i=0; i<size; i++)
    {
        if(map[i].index == index)
        {
            return map[i].enumValue;
        }
    }
    return _default;
}


}
