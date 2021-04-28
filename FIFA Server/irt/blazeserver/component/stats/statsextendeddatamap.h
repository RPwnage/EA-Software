/*************************************************************************************************/
/*!
    \file statsextendeddatamap.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_STATS_EXTENDED_DATA_MAP_H
#define BLAZE_STATS_EXTENDED_DATA_MAP_H

/*** Include files *******************************************************************************/
#include "EASTL/vector_map.h"
#include "stats/tdf/stats.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace Stats
{

enum ExtendedDataSource
{
    USER_STAT,
    USER_RANK
};

typedef eastl::vector_map<UserStatId, ExtendedDataSource> ExtendedDataKeyMap;

}
}
#endif


