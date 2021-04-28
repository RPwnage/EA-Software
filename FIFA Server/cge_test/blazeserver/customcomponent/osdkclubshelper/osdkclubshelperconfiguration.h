/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved. 2010
*/
/*************************************************************************************************/
#ifndef BLAZE_OSDKCLUBSHELPER_CONFIGURATION_H
#define BLAZE_OSDKCLUBSHELPER_CONFIGURATION_H

#include "EASTL/list.h"
#include "EASTL/hash_map.h"
#include "EASTL/string.h"
#include "EASTL/hash_set.h"  

namespace Blaze
{
namespace OSDKClubsHelper
{
 
typedef eastl::vector<uint32_t> ValidSettingsContainer;

// Use a hash_set, to avoid linear search for large sets, like  that of team ids.
typedef eastl::hash_set<uint32_t> ValidTeamsSet;

struct OSDKClubsHelperValidationSettings
{
    ValidTeamsSet mValidTeamsSet;
    ValidSettingsContainer mValidLanguages;
    ValidSettingsContainer mValidRegions;
};


} //namespace OSDKClubsHelper
} //namespace blaze


#endif // BLAZE_OSDKCLUBSHELPER_CONFIGURATION_H

