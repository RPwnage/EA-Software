/*************************************************************************************************/
/*!
    \file scope.h

    header file with scope-related utilities, included by both server and client.

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_STATS_SCOPE_H
#define BLAZE_STATS_SCOPE_H

#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/component/stats/tdf/stats.h"
#else
#include "stats/tdf/stats.h"
#endif

namespace Blaze
{
namespace Stats
{

bool genStatValueMapKeyForUnit(const char8_t* scopeName, ScopeValue scopeValue, char8_t* mapKey, size_t mapKeyLength);
bool genStatValueMapKeyForUnitMap(const ScopeNameValueMap& scopeNameValueMap, char8_t* mapKey, size_t mapKeyLength);
void genScopeValueListFromKey(const char8_t* mapKey, ScopeNameValueMap& scopeNameValueMap);

}// Stats
}// Blaze

#endif // BLAZE_STATS_SCOPE_H
