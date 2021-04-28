/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/util/quantize.h"
#include <EASTL/algorithm.h>

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

uint32_t quantizeToNearestValue(uint32_t value, const uint32_t data[], size_t count)
{
    if (count == 0)
        return value;
    if (count == 1)
        return *data;
    auto* begin = data;
    auto* end = data + count;
    auto itr = eastl::lower_bound(begin, end, value);
    if (itr == begin)
        return *itr;
    if (itr == end)
        return *(end - 1);
    auto delta = *itr - value;
    auto delta2 = value - *(itr - 1);
    if (delta <= delta2)
        return *itr;
    return *(itr - 1);
}

} // Blaze
