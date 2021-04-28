/*************************************************************************************************/
/*!
    \file scope.cpp

    implementation of common functions declared in scope.h, included by both server and client.

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/allocdefines.h"
#include "BlazeSDK/statsapi/scope.h"
#ifndef ASSERT
    #define ASSERT BlazeAssert
#endif
#else
#include "framework/blaze.h"
#include "scope.h"
#endif
#include "BlazeSDK/shared/framework/util/blazestring.h"

namespace Blaze
{
namespace Stats
{

/*! ************************************************************************************************/
/*! \brief generate a string combined by a pair of scope name and scope value

    Caller has to allocate memory for mapKey before calling this function.
    The function has two copies on both client and server side, we have to decode the key the same
    way we encode it.

    \param[in]scopeName - name of the scope
    \param[in]scopeValue - value of the scope
    \param[out]mapKey - map key generated
    \param[in]mapKeyLength - length of mapkey
    \return true if mapKey is generated properly
***************************************************************************************************/
bool genStatValueMapKeyForUnit(const char8_t* scopeName, ScopeValue scopeValue, char8_t* mapKey, size_t mapKeyLength) 
{
    if (!mapKey)
    {
        ASSERT(false && "mapKey passed in is invalid!");
        return false;
    }

    blaze_strnzcat(mapKey, scopeName, mapKeyLength);
    blaze_strnzcat(mapKey, SCOPE_NAME_VALUE_SEPARATOR, mapKeyLength);
    char8_t buff[32];
    blaze_snzprintf(buff, sizeof(buff), "%" PRId64, scopeValue);
    blaze_strnzcat(mapKey, buff, mapKeyLength);

    return true;
}

/*! ************************************************************************************************/
/*! \brief generate a string combined by a list of scope name and scope value pair

    Caller has to allocate memory for mapKey before calling this function.
    The function has two copies on both client and server side, we have to decode the key the same
    way we encode it.

    \param[in]scopeNameValueMap - a scope value map keyed by scope name
    \param[out]mapKey - map key generated
    \param[in]mapKeyLength - length of mapkey
    \return true if mapKey is generated properly
***************************************************************************************************/
bool genStatValueMapKeyForUnitMap(const ScopeNameValueMap& scopeNameValueMap, char8_t* mapKey, size_t mapKeyLength) 
{
    if (!mapKey)
    {
        ASSERT(false && "mapKey passed in is invalid!");
        return false;
    }

    ScopeNameValueMap::const_iterator itbegin = scopeNameValueMap.begin();
    ScopeNameValueMap::const_iterator it = itbegin;
    ScopeNameValueMap::const_iterator itend = scopeNameValueMap.end();
    while (it != itend)
    {
        if (it != itbegin)
        {
            blaze_strnzcat(mapKey, SCOPE_UNIT_SEPARATOR, mapKeyLength);
        }

        genStatValueMapKeyForUnit(it->first, it->second, mapKey, mapKeyLength);
        it++;
    }

    return true;
}

/*! ************************************************************************************************/
/*! \brief Split a combined map key to a list of scope name/value pair

    \param[in]mapKey - a combined scope name/scope value string
    \param[out]scopeNameValueMap - a list of scope name/scope value pair to pass out
***************************************************************************************************/
void genScopeValueListFromKey(const char8_t* mapKey, ScopeNameValueMap& scopeNameValueMap)
{
    const int unitBuffer = 100;
    const int unitElementBuffer = 32;
    char8_t leftKey[unitBuffer] = {0};
    char8_t scopeName[unitElementBuffer];
    char8_t scopeValue[unitElementBuffer];

    // make a working copy of the first 2 name-value pairs
    // (any 2nd pair doesn't have to be complete -- just filling a working buffer)
    blaze_strnzcat(leftKey, mapKey, sizeof(leftKey));

    // look for the next name-value pair
    char8_t* nextScopeUnit = blaze_stristr(leftKey, SCOPE_UNIT_SEPARATOR);
    while (nextScopeUnit != nullptr)
    {
        // does have a next name-value pair
        size_t keyLength = strlen(leftKey);
        size_t unitLength = keyLength - strlen(nextScopeUnit);

        // isolate/copy current name-value pair
        char8_t unit[unitBuffer] = {0};
        blaze_strsubzcat(unit, unitBuffer, leftKey, unitLength);

        // parse current name-value pair
        char8_t* nameFound = blaze_stristr(unit, SCOPE_NAME_VALUE_SEPARATOR);
        if (nameFound != nullptr)
        {
            scopeName[0] = '\0';
            scopeValue[0] = '\0';
            size_t valueLength = strlen(nameFound);
            blaze_strsubzcat(scopeName, unitElementBuffer, unit, unitLength - valueLength);
            blaze_strsubzcat(scopeValue, unitElementBuffer, nameFound + 1, valueLength);

            ScopeValue scpValue = 0;
            sscanf(scopeValue, "%" SCNd64, &scpValue);
            scopeNameValueMap.insert(eastl::make_pair(scopeName, scpValue));
        }

        // after current name-value pair, make a working copy of the next 2 pairs
        // (again, any 2nd pair doesn't have to be complete)
        leftKey[0] = '\0';
        blaze_strsubzcat(leftKey, sizeof(leftKey), nextScopeUnit + 1, STATS_SCOPESTRING_LENGTH);

        // look for the next name-value pair
        nextScopeUnit = blaze_stristr(leftKey, SCOPE_UNIT_SEPARATOR);
    }

    // parse last name-value pair -- which could be the only pair in this list/map
    char8_t* nameFound = blaze_stristr(leftKey, SCOPE_NAME_VALUE_SEPARATOR);
    if (nameFound != nullptr)
    {
        scopeName[0] = '\0';
        scopeValue[0] = '\0';
        size_t unitLength = strlen(leftKey);
        size_t valueLength = strlen(nameFound);
        blaze_strsubzcat(scopeName, unitBuffer, leftKey, unitLength - valueLength);
        blaze_strsubzcat(scopeValue, unitBuffer, nameFound + 1, valueLength);

        ScopeValue scpValue = 0;
        sscanf(scopeValue, "%" SCNd64, &scpValue);
        scopeNameValueMap.insert(eastl::make_pair(scopeName, scpValue));
    }
}

}// Stats
}// Blaze

