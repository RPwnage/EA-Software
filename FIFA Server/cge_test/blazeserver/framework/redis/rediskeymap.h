/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REDIS_KEYMAP_H
#define BLAZE_REDIS_KEYMAP_H

/*** Include files *******************************************************************************/

#include "EASTL/type_traits.h"
#include "framework/redis/rediscollection.h"
#include "framework/redis/redisresponse.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


namespace Blaze
{

/**
 *  /class RedisKeyMap
 *  
 *  This class implements a generic type safe interface for the redis 'keyed' values.
 *  The keys can be primitives or composite types(e.g: eastl::pair) compatible with 
 *  RedisDecoder (see redisdecoder.h).
 *  
 *  /note   This class differs from RedisKeyFieldMap in that every key/value pair of a 
 *          RedisKeyMap may be stored on a *different* Redis instance. By contrast
 *          all field/value pairs associated with a specific key of a RedisKeyFieldValueMap
 *          are always stored on a single Redis instance and may be operated on atomically.
 *  
 *  This class is appropriate for implementing distributed collections analogous to:
 *  
 *  hash_map<int32_t, hash_map<string, uint32_t> >
 *  hash_map<int32_t, hash_map<string, hash_map<uint64_t, string> > >
 *  
 *  /note The template arguments are used to lock down the types managed by the collection and
 *  to provide explicit and automatic conversion between the application layer and the redis
 *  storage layer.
 *  
 *  /examples See testRedisKeyMap() @ redisunittests.cpp
*/
template<typename K, typename V, typename Func = RedisGetSliverIdFromKey<K> >
class RedisKeyMap : public RedisCollection
{
public:
    typedef K Key;
    typedef V Value;

    RedisKeyMap(const RedisId& id, bool isSharded = false) : RedisCollection(id, isSharded) {}
    RedisError exists(const Key& key, int64_t* result) const;
    RedisError insert(const Key& key, const Value& value, int64_t* result = nullptr);
    RedisError insertWithTtl(const Key& key, const Value& value, EA::TDF::TimeValue ttl, int64_t* result = nullptr);
    RedisError upsert(const Key& key, const Value& value, int64_t* result = nullptr);
    RedisError upsertWithTtl(const Key& key, const Value& value, EA::TDF::TimeValue ttl, int64_t* result = nullptr);
    RedisError erase(const Key& key, int64_t* result = nullptr);
    RedisError incr(const Key& key, int64_t* result = nullptr);
    RedisError decr(const Key& key, int64_t* result = nullptr);
    RedisError incrBy(const Key& key, const Value offset, int64_t* result = nullptr);
    RedisError decrBy(const Key& key, const Value offset, int64_t* result = nullptr);
    RedisError decrRemoveWhenZero(const Key& key, bool* removed = nullptr);
    RedisError find(const Key& key, Value& value) const;
    RedisError expire(const Key& key, EA::TDF::TimeValue relativeTimeout, int64_t* result = nullptr);
    RedisError pexpire(const Key& key, EA::TDF::TimeValue relativeTimeout, int64_t* result = nullptr);
    RedisError getTimeToLive(const Key& key, EA::TDF::TimeValue& relativeTimeout);
    // DEPRECATED - Use getTimeToLive instead
    RedisError pttl(const Key& key, EA::TDF::TimeValue& relativeTimeout) { return getTimeToLive(key, relativeTimeout); }
};

template<typename K, typename V, typename Func>
RedisError RedisKeyMap<K, V, Func>::exists(const Key& key, int64_t* result) const
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "EXISTS");

    return execCommand(cmd, result);
}

template<typename K, typename V, typename Func>
RedisError RedisKeyMap<K,V,Func>::insert(const Key& key, const Value& value, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "SET");

    cmd.add(value);
    cmd.add("NX");

    return execCommand(cmd, result);
}

template<typename K, typename V, typename Func>
RedisError RedisKeyMap<K,V,Func>::insertWithTtl(const Key& key, const Value& value, EA::TDF::TimeValue ttl, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "SET");

    // Grab this value now and avoid truncation mistakes below
    int64_t ttlMilliseconds = ttl.getMillis();

    // The assumption here is that, if they are calling this Ttl version of upsert, the caller really wants to upsert, but
    // perhaps their math was off slightly.  So, we set to 1 millisecond, effectively causing the key to be set then immediately deleted.
    // Note: We can't set it to 0, because that is not considered a valid TTL by redis.
    if (ttlMilliseconds <= 0)
        ttlMilliseconds = 1;

    cmd.add(value);
    cmd.add("PX").add(ttlMilliseconds);
    cmd.add("NX");

    return execCommand(cmd, result);
}

template<typename K, typename V, typename Func>
RedisError RedisKeyMap<K,V,Func>::upsert(const Key& key, const Value& value, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "SET");

    cmd.add(value);

    return execCommand(cmd, result);
}

template<typename K, typename V, typename Func>
RedisError RedisKeyMap<K,V,Func>::upsertWithTtl(const Key& key, const Value& value, EA::TDF::TimeValue ttl, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "SET");

    // Grab this value now and avoid truncation mistakes below
    int64_t ttlMilliseconds = ttl.getMillis();

    // The assumption here is that, if they are calling this Ttl version of upsert, the caller really wants to upsert, but
    // perhaps their math was off slightly.  So, we set to 1 millisecond, effectively causing the key to be set then immediately deleted.
    // Note: We can't set it to 0, because that is not considered a valid TTL by redis.
    if (ttlMilliseconds <= 0)
        ttlMilliseconds = 1;

    cmd.add(value);
    cmd.add("PX").add(ttlMilliseconds);

    return execCommand(cmd, result);
}

template<typename K, typename V, typename Func>
RedisError RedisKeyMap<K,V,Func>::erase(const Key& key, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "DEL");

    return execCommand(cmd, result);
}

template<typename K, typename V, typename Func>
RedisError RedisKeyMap<K,V,Func>::incr(const Key& key, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "INCR");

    return execCommand(cmd, result);
}

template<typename K, typename V, typename Func>
RedisError RedisKeyMap<K,V,Func>::decr(const Key& key, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "DECR");

    return execCommand(cmd, result);
}

template<typename K, typename V, typename Func>
RedisError RedisKeyMap<K,V,Func>::incrBy(const Key& key, const Value value, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "INCRBY");
    
    cmd.add(value);

    return execCommand(cmd, result);
}

template<typename K, typename V, typename Func>
RedisError RedisKeyMap<K,V,Func>::decrBy(const Key& key, const Value value, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "DECRBY");

    cmd.add(value);

    return execCommand(cmd, result);
}

template<typename K, typename V, typename Func>
RedisError RedisKeyMap<K,V,Func>::decrRemoveWhenZero(const Key& key, bool* removed)
{
    RedisCommand cmd;
    RedisResponsePtr resp;
    RedisError rc;

    cmd.beginKey();
    cmd.add(eastl::make_pair(getCollectionKey(Func()(key)), key));
    cmd.end();

    rc = execScript(msDecrRemoveWhenZero, cmd, resp);
    if (rc == REDIS_ERR_OK)
    {
        if (removed != nullptr)
            *removed = resp->getInteger() == 0;
    }

    return rc;
}

template<typename K, typename V, typename Func>
RedisError RedisKeyMap<K,V,Func>::find(const Key& key, Value& value) const
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "GET");

    RedisResponsePtr resp;
    RedisError rc = exec(cmd, resp);
    if (rc == REDIS_ERR_OK)
    {
        if (resp->isString())
        {
            const char8_t* begin = resp->getString();
            const char8_t* end = begin + resp->getStringLen();
            RedisDecoder rdecoder;
            if (rdecoder.decodeValue(begin, end, value) == nullptr)
                rc = REDIS_ERR_SYSTEM;
        }
        else
            rc = REDIS_ERR_NOT_FOUND;
    }
    return rc;
}

template<typename K, typename V, typename Func>
RedisError RedisKeyMap<K,V, Func>::expire(const Key& key, EA::TDF::TimeValue relativeTimeout, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "EXPIRE");
    cmd.add(relativeTimeout.getSec());

    return execCommand(cmd, result);
}

template<typename K, typename V, typename Func>
RedisError RedisKeyMap<K,V, Func>::pexpire(const Key& key, EA::TDF::TimeValue relativeTimeout, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "PEXPIRE");
    cmd.add(relativeTimeout.getMillis());

    return execCommand(cmd, result);
}

template<typename K, typename V, typename Func>
RedisError RedisKeyMap<K,V, Func>::getTimeToLive(const Key& key, EA::TDF::TimeValue& relativeTimeout)
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

#endif // BLAZE_REDIS_KEYMAP_H

