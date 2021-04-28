#ifndef __COMMON_H__
#define __COMMON_H__

#include <OriginSDK/OriginTypes.h>

namespace lsx
{
    struct EnumToIndexMap
    {
        int32_t enumValue;
        uint32_t index;
    };

    uint32_t GetIndexFromEnum(EnumToIndexMap * map, uint32_t size, int32_t enumValue, uint32_t _default);
    int32_t GetEnumFromIndex(EnumToIndexMap * map, uint32_t size, uint32_t index, int32_t _default);
}




#endif  //__COMMON_H__