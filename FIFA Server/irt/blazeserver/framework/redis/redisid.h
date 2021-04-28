/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REDIS_ID_H
#define BLAZE_REDIS_ID_H


/*** Include files *******************************************************************************/
#include "EASTL/utility.h"


namespace Blaze
{

typedef eastl::pair<eastl::string, eastl::string> RedisSchemaKeyPair;

class RedisCommand;
class RedisDecoder;

struct RedisId
{
public:
    RedisId();
    RedisId(const char8_t* redisNamespace, const char8_t* redisName);
    
    const char8_t* getNamespace() const { return mSchemaKeyPair.first.c_str(); }
    const char8_t* getName() const { return mSchemaKeyPair.second.c_str(); }
    const char8_t* getFullName() const { return mFullName.c_str(); }
    const RedisSchemaKeyPair& getSchemaKeyPair() const { return mSchemaKeyPair; }
    size_t getHash() const;

private:
    friend RedisDecoder;
    friend RedisCommand;
    RedisSchemaKeyPair mSchemaKeyPair;
    eastl::string mFullName;
};

struct RedisIdLessThan
{
    bool operator()(const RedisId& a, const RedisId& b) const
    {
        return blaze_strcmp(a.getFullName(), b.getFullName()) < 0;
    }
};

inline bool operator==(const RedisId& a, const RedisId& b) 
{
    return blaze_strcmp(a.getFullName(), b.getFullName()) == 0;
}

} // Blaze

namespace eastl
{
    template <>
    struct hash<Blaze::RedisId>
    {
        size_t operator()(const Blaze::RedisId& p) const 
        {
            return p.getHash();
        }
    };

    template <>
    struct equal_to<Blaze::RedisId>
    {
        bool operator()(const Blaze::RedisId& a, const Blaze::RedisId& b) const
        {
            return blaze_strcmp(a.getFullName(), b.getFullName()) == 0;
        }
    };
}

#endif // BLAZE_REDIS_VALUE_H

