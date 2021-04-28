/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_ATTRVALUE_UTIL
#define BLAZE_STRESS_ATTRVALUE_UTIL

#include "framework/tdf/attributes.h"

namespace Blaze
{

class ConfigMap;
class ConfigSequence;

namespace Stress
{


class AttrValueUpdateParams
{
public:
    AttrValueUpdateParams() :
        mPreGame(false),
        mPostGame(false), 
        mUpdateCount(0),
        mUpdateDelayMinMs(0),
        mUpdateDelayMaxMs(0)
    {}
    bool mPreGame;
    bool mPostGame;
    uint32_t mUpdateCount;
    uint32_t mUpdateDelayMinMs;
    uint32_t mUpdateDelayMaxMs;
};

typedef eastl::pair<Collections::AttributeValue, uint32_t> AttributeValueWeight;
typedef eastl::vector< AttributeValueWeight > AttrValueVector;    // value + weight.
typedef eastl::hash_map<const char8_t*, AttrValueVector, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > AttrValues;


class AttrValueUtil
{
public:
    static void initAttrUpdateValues(const ConfigSequence *attrs, const ConfigMap *valueSequences, AttrValues& valu);
    static void initAttrUpdateParams(const ConfigMap* map, AttrValueUpdateParams& params);
    static const char8_t* pickAttrValue(const AttrValues& values, const char8_t* attr);
    static void fillAttributeMap(const AttrValues& values, Collections::AttributeMap &attrMap);
};

}   // Stress
}   // Blaze


#endif
