/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REDIS_KEYSET_H
#define BLAZE_REDIS_KEYSET_H

/*** Include files *******************************************************************************/

#include "EASTL/type_traits.h"
#include "EASTL/algorithm.h"
#include "framework/redis/rediscollection.h"
#include "framework/redis/redisdecoder.h"
#include "framework/redis/redisresponse.h"
/*** Defines/Macros/Constants/Typedefs ***********************************************************/


namespace Blaze
{

/**
 *  /class RedisKeySet
 *  
 *  This class implements a generic type safe interface for the redis 'keyed' hash sets.
 *  The keys can be primitives or composite types(e.g: eastl::pair) compatible with 
 *  RedisDecoder (see redisdecoder.h).
 *  
 *  This class is appropriate for implementing distributed collections analogous to:
 *  
 *  hash_map<int32_t, hash_set<string> >
 *  hash_map<int32_t, hash_map<string, hash_set<uint64_t> > >
 *  
 *  /note The template arguments are used to lock down the types managed by the collection and
 *  to provide explicit and automatic conversion between the application layer and the redis
 *  storage layer.
 *  
 *  /examples See testRedisKeySet() @ redisunittests.cpp
*/
template<typename K, typename V, typename Func = RedisGetSliverIdFromKey<K> >
class RedisKeySet : public RedisCollection
{
public:
    typedef K Key;
    typedef V Value;
    typedef eastl::vector<Key> KeyList;
    typedef eastl::vector<Value> ValueList;

    RedisKeySet(const RedisId& id, bool isSharded = false) : RedisCollection(id, isSharded) {}
    RedisError add(const Key& key, const Value& value, int64_t* result = nullptr);
    RedisError remove(const Key& key, const Value& value, int64_t* result = nullptr);
    RedisError pop(const Key& key, Value& value);
    RedisError exists(const Key& key, int64_t* result) const;
    RedisError exists(const Key& key, const Value& value, int64_t* result) const;
    RedisError getMembers(const Key& key, ValueList& values) const;
    RedisError getSize(const Key& key, int64_t& size) const;
    RedisError expire(const Key& key, EA::TDF::TimeValue relativeTimeout, int64_t* result = nullptr);
    RedisError pexpire(const Key& key, EA::TDF::TimeValue relativeTimeout, int64_t* result = nullptr);
    RedisError getTimeToLive(const Key& key, EA::TDF::TimeValue& relativeTimeout);
};

template<typename K, typename V, typename Func>
RedisError RedisKeySet<K,V,Func>::add(const Key& key, const Value& value, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "SADD");

    cmd.add(value);

    return execCommand(cmd, result);
}

template<typename K, typename V, typename Func>
RedisError RedisKeySet<K,V,Func>::remove(const Key& key, const Value& value, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "SREM");

    cmd.add(value);

    return execCommand(cmd, result);
}

template<typename K, typename V, typename Func>
RedisError RedisKeySet<K,V,Func>::pop(const Key& key, Value& value)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "SPOP");

    RedisResponsePtr resp;
    RedisError rc = exec(cmd, resp);
    if (rc == REDIS_ERR_OK && resp->isString())
    {
        const char8_t* begin = resp->getString();
        const char8_t* end = begin + resp->getStringLen();
        RedisDecoder rdecoder;
        if (rdecoder.decodeValue(begin, end, value) == nullptr)
            rc = REDIS_ERR_SYSTEM;
    }
    return rc;
}

template<typename K, typename V, typename Func>
RedisError RedisKeySet<K, V, Func>::exists(const Key& key, int64_t* result) const
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "EXISTS");

    return execCommand(cmd, result);
}

template<typename K, typename V, typename Func>
RedisError RedisKeySet<K,V,Func>::exists(const Key& key, const Value& value, int64_t* result) const
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "SISMEMBER");

    cmd.add(value);

    return execCommand(cmd, result);
}

template<typename K, typename V, typename Func>
RedisError RedisKeySet<K,V,Func>::getMembers(const Key& key, eastl::vector<Value>& values) const
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "SMEMBERS");

    RedisResponsePtr resp;
    RedisError rc = exec(cmd, resp);
    if (rc == REDIS_ERR_OK && resp->isArray())
    {
        values.reserve(resp->getArraySize());
        for (uint32_t i = 0, sz = resp->getArraySize(); i < sz; ++i)
        {
            const RedisResponse& element = resp->getArrayElement(i);
            if (element.isString())
            {
                Value& val = values.push_back();
                const char8_t* begin = element.getString();
                const char8_t* end = begin + element.getStringLen();
                RedisDecoder rdecoder;
                if (rdecoder.decodeValue(begin, end, val) == nullptr)
                {
                    rc = REDIS_ERR_SYSTEM;
                    break;
                }
            }
        }
    }
    return rc;
}

template<typename K, typename V, typename Func>
RedisError RedisKeySet<K,V,Func>::getSize(const Key& key, int64_t& size) const
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "SCARD");

    return execCommand(cmd, &size);
}

template<typename K, typename V, typename Func>
RedisError RedisKeySet<K,V, Func>::expire(const Key& key, EA::TDF::TimeValue relativeTimeout, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "EXPIRE");
    cmd.add(relativeTimeout.getSec());

    return execCommand(cmd, result);
}

template<typename K, typename V, typename Func>
RedisError RedisKeySet<K,V, Func>::pexpire(const Key& key, EA::TDF::TimeValue relativeTimeout, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "PEXPIRE");
    cmd.add(relativeTimeout.getMillis());

    return execCommand(cmd, result);
}

template<typename K, typename V, typename Func>
RedisError RedisKeySet<K,V, Func>::getTimeToLive(const Key& key, EA::TDF::TimeValue& relativeTimeout)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "PTTL");

    RedisResponsePtr resp;
    RedisError rc = exec(cmd, resp);
    if (rc == REDIS_ERR_OK)
    {
        if (resp->isInteger())
        {
            if (resp->getInteger() == -1)
            {
                rc = REDIS_ERR_NO_EXPIRY;
            }
            else if (resp->getInteger() == -2)
            {
                rc = REDIS_ERR_NOT_FOUND;
            }
            else
            {
                relativeTimeout.setMillis(resp->getInteger());
            }
        }
        else
        {
            rc = REDIS_ERR_SYSTEM;
        }
    }
    return rc;
}


} // Blaze

#endif // BLAZE_REDIS_KEYSET_H

