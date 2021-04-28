/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"

//#include "blazerpclog.h"
#include "blazerpcerrors.h"
//#include "framework/config/config_file.h"
//#include "framework/config/config_boot_util.h"
#include "framework/config/config_sequence.h"
#include "framework/config/config_map.h"
#include "framework/util/shared/blazestring.h"
//#include "framework/protocol/shared/heat2decoder.h"
//#include "framework/util/shared/rawbuffer.h"
#include "framework/util/random.h"
#include "attrvalueutil.h"

namespace Blaze
{
namespace Stress
{

void AttrValueUtil::initAttrUpdateValues(const ConfigSequence *attrs, const ConfigMap *valueSequences, AttrValues& valu)
{
    if (attrs != NULL && attrs->getSize() > 0)
    {
        //  setup game attributes
        for (size_t i = 0; i < attrs->getSize(); ++i)
        {   
            //  write out game attrvalue vector
            AttrValues::insert_return_type inserter = valu.insert(attrs->at(i));

            if (inserter.second)
            {
                const ConfigSequence *values = valueSequences->getSequence(inserter.first->first);
                if (values->getSize() > 0)
                {
                    uint32_t defaultWeight = 100 / (uint32_t)values->getSize();
                    for (size_t valueIdx = 0; valueIdx < values->getSize(); ++valueIdx)
                    {
                        char8_t *text = blaze_strdup(values->at(valueIdx));
                        char8_t *weight_token = blaze_strstr(text, ":");

                        AttributeValueWeight value;
                        if (weight_token != NULL)
                        {
                            *weight_token = '\0';
                            ++weight_token;
                            value.second = (uint32_t)(atoi(weight_token));
                        }
                        else
                        {
                            value.second = defaultWeight;
                        }
                        value.first = text;
                        inserter.first->second.push_back(value);
                        BLAZE_FREE(text);
                    }
                }
            }
        }   
    }
}

void AttrValueUtil::initAttrUpdateParams(const ConfigMap* map, AttrValueUpdateParams& params)
{
    if (map == NULL)
        return;
    params.mPreGame = map->getBool("preGame", false);
    params.mPostGame = map->getBool("postGame", false);
    params.mUpdateCount = map->getUInt32("updateCount", 0);
    params.mUpdateDelayMinMs = map->getUInt32("updateDelayMinMs", 0);
    params.mUpdateDelayMaxMs = map->getUInt32("updateDelayMaxMs", 1000);
}

const char8_t* AttrValueUtil::pickAttrValue(const AttrValues& values, const char8_t* attr)
{
    AttrValues::const_iterator itAttrValues = values.find(attr);
    if (itAttrValues == values.end() || itAttrValues->second.empty())
    {
        return NULL;
    }

    //  determine which value to use based on the random weight selected compared to the weights on each value.
    uint32_t weight = (uint32_t)(rand() % 100);
    AttrValueVector::const_iterator itValVecEnd = itAttrValues->second.end();
    AttrValueVector::const_iterator itValVec = itAttrValues->second.begin();
    size_t valueindex = 0;
    uint32_t weightBase = 0;
    for (; itValVec != itValVecEnd; ++itValVec, ++valueindex)
    {
        AttributeValueWeight valueAndWeight = *itValVec;
        if (weight >= weightBase && weight < (weightBase+valueAndWeight.second))
        {
            break;
        }
        // calculate next low-threshold for next attribute compare using the high-threashold for this attribute
        weightBase += valueAndWeight.second;
    }
    //  Take care of cases where the weights don't add up to 100 in the config.   This usually means the last value is the selected one.
    if (valueindex >= itAttrValues->second.size())
    {
        valueindex = itAttrValues->second.size()-1;
    }

    return itAttrValues->second.at(valueindex).first;
}

void AttrValueUtil::fillAttributeMap(const AttrValues& values, Collections::AttributeMap &attrMap)
{
    if (!values.empty())
    {
        for (AttrValues::const_iterator it = values.begin(); it != values.end(); ++it)
        {
            Collections::AttributeMap::value_type val;
            val.first.set(it->first);
            val.second.set(pickAttrValue(values, it->first));
            attrMap.insert(val); 
        }
    }
}

}    // Stress
}    // Blaze
