/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/redis/redisdecoder.h"
#include "framework/redis/redisid.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

RedisId::RedisId()
{
}

RedisId::RedisId(const char8_t* redisNamespace, const char8_t* redisName) :
    mSchemaKeyPair(eastl::make_pair(redisNamespace, redisName))
{
    mFullName += redisNamespace;
    mFullName += REDIS_PAIR_DELIMITER;
    mFullName += redisName;
}

size_t RedisId::getHash() const
{
    return eastl::hash<eastl::string>()(mFullName);
}

} // Blaze
