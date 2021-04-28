/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REDIS_KEYFIELDMAP_H
#define BLAZE_REDIS_KEYFIELDMAP_H

/*** Include files *******************************************************************************/

#include "EASTL/type_traits.h"
#include "framework/redis/rediscollection.h"
#include "framework/redis/redisdecoder.h"
#include "framework/redis/redisresponse.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


namespace Blaze
{

/**
 *  /class RedisKeyFieldMap
 *  
 *  This class implements a generic type safe interface for the redis 'keyed' hash maps.
 *  The keys can be primitives or composite types(e.g: eastl::pair) compatible with 
 *  RedisDecoder (see redisdecoder.h).
 *  
 *  /note   All the field/value pairs inserted under a given key are stored on a *single* Redis instance
 *          thus allowing atomic operations on the fields and values but limiting scaling options.
 *  
 *  This class is appropriate for implementing distributed collections analogous to:
 *  
 *  hash_map<int32_t, hash_map<string, uint32_t> >
 *  hash_map<eastl::string, hash_map<int32_t, hash_map<uint64_t, string> > >
 *  
 *  /note The template arguments are used to lock down the types managed by the collection and
 *  to provide explicit and automatic conversion between the application layer and the redis
 *  storage layer.
 *  
 *  /examples See testRedisKeyFieldMap() @ redisunittests.cpp
*/
template<typename K, typename F, typename V, typename Func = RedisGetSliverIdFromKey<K> >
class RedisKeyFieldMap : public RedisCollection
{
public:
    typedef K Key;
    typedef F Field;
    typedef V Value;
    typedef eastl::hash_set<Key> KeySet;
    typedef eastl::vector<Field> FieldList;
    typedef eastl::hash_map<Field, Value> FieldValueMap;

    RedisKeyFieldMap(const RedisId& id, bool isSharded = false) : RedisCollection(id, isSharded) {}
    RedisError exists(const Key& key, int64_t* result) const;
    RedisError insert(const Key& key, const Field& field, const Value& value, int64_t* result = nullptr);
    RedisError upsert(const Key& key, const Field& field, const Value& value, int64_t* result = nullptr);
    RedisError upsert(const Key& key, const FieldList& fields, const Value& value, int64_t* result = nullptr);
    RedisError upsert(const Key& key, const FieldValueMap& fieldValueMap, int64_t* result = nullptr);
    RedisError erase(const Key& key, const FieldList& fields, int64_t* result = nullptr);
    RedisError upsertn(const Key& key, const Field** fields, const Value** values, size_t n, int64_t* result = nullptr);
    RedisError erase(const Key& key, const Field& field, int64_t* result = nullptr);
    RedisError erasen(const Key& key, const Field** fields, size_t n, int64_t* result = nullptr);
    RedisError erase(const Key& key, int64_t* result = nullptr);
    RedisError find(const Key& key, const Field& field, Value& value) const;
    RedisError scan(const Key& key, RedisResponsePtr& scanResult, uint32_t pageSizeHint = REDIS_SCAN_STRIDE_MIN);
    RedisError incrBy(const Key& key, const Field& field, int64_t delta, int64_t* result = nullptr);
    RedisError decrBy(const Key& key, const Field& field, int64_t delta, int64_t* result = nullptr);
    RedisError decrRemoveWhenZero(const Key& key, const Field& field, bool* removed);
    RedisError getFields(const Key& key, FieldList& fields) const;
    RedisError getFieldValueMap(const Key& key, FieldValueMap& fieldValueMap) const;
    RedisError expire(const Key& key, EA::TDF::TimeValue relativeTimeout, int64_t* result = nullptr);
    RedisError pexpire(const Key& key, EA::TDF::TimeValue relativeTimeout, int64_t* result = nullptr);
    RedisError getTimeToLive(const Key& key, EA::TDF::TimeValue& relativeTimeout);

    [[deprecated("This method cannot be executed reliably on Redis in cluster mode")]]
    RedisError getAllByKeys(const KeySet& keys, eastl::vector<RedisResponsePtr>& results, uint32_t maxKeysPerCall) const;

    static bool extractFieldArrayElement(const RedisResponse& resultArray, uint32_t index, Field& field);
    static bool extractValueArrayElement(const RedisResponse& resultArray, uint32_t index, Value& field);
};

template<typename K, typename F, typename V, typename Func>
RedisError RedisKeyFieldMap<K,F,V,Func>::exists(const Key& key, int64_t* result) const
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "EXISTS");

    return execCommand(cmd, result);
}

template<typename K, typename F, typename V, typename Func>
RedisError RedisKeyFieldMap<K,F,V,Func>::insert(const Key& key, const Field& field, const Value& value, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "HSETNX");

    cmd.add(field);
    cmd.add(value);

    return execCommand(cmd, result);
}

template<typename K, typename F, typename V, typename Func>
RedisError RedisKeyFieldMap<K,F,V,Func>::upsert(const Key& key, const Field& field, const Value& value, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "HSET");

    cmd.add(field);
    cmd.add(value);

    return execCommand(cmd, result);
}

template<typename K, typename F, typename V, typename Func>
RedisError RedisKeyFieldMap<K,F,V,Func>::upsert(const Key& key, const FieldList& fields, const Value& value, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "HMSET");

    for (typename Blaze::RedisKeyFieldMap<K, F, V>::FieldList::const_iterator itr = fields.cbegin(); itr != fields.cend(); ++itr)
    {
        cmd.add(*itr);
        cmd.add(value);
    }

    return execCommand(cmd, result);
}

template<typename K, typename F, typename V, typename Func>
RedisError RedisKeyFieldMap<K,F,V,Func>::upsert(const Key& key, const FieldValueMap& fieldValueMap, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "HMSET");

    for (typename Blaze::RedisKeyFieldMap<K, F, V>::FieldValueMap::const_iterator itr = fieldValueMap.cbegin(); itr != fieldValueMap.cend(); ++itr)
    {
        cmd.add(itr->first);
        cmd.add(itr->second);
    }

    return execCommand(cmd, result);
}

template<typename K, typename F, typename V, typename Func>
RedisError RedisKeyFieldMap<K,F,V,Func>::erase(const Key& key, const FieldList& fields, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "HDEL");

    for (typename Blaze::RedisKeyFieldMap<K, F, V>::FieldList::const_iterator itr = fields.cbegin(); itr != fields.cend(); ++itr)
        cmd.add(*itr);

    return execCommand(cmd, result);
}

template<typename K, typename F, typename V, typename Func>
RedisError RedisKeyFieldMap<K,F,V,Func>::upsertn(const Key& key, const Field** fields, const Value** values, size_t n, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "HMSET");

    for (size_t i = 0; i < n; ++i)
    {
        cmd.add(*fields[i]);
        cmd.add(*values[i]);
    }

    return execCommand(cmd, result);
}

template<typename K, typename F, typename V, typename Func>
RedisError RedisKeyFieldMap<K,F,V,Func>::erase(const Key& key, const Field& field, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "HDEL");

    cmd.add(field);

    return execCommand(cmd, result);
}

template<typename K, typename F, typename V, typename Func>
RedisError RedisKeyFieldMap<K,F,V,Func>::erasen(const Key& key, const Field** fields, size_t n, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "HDEL");

    for (size_t i = 0; i < n; ++i)
    {
        cmd.add(*fields[i]);
    }

    return execCommand(cmd, result);
}

template<typename K, typename F, typename V, typename Func>
RedisError RedisKeyFieldMap<K,F,V,Func>::erase(const Key& key, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "DEL");

    return execCommand(cmd, result);
}

template<typename K, typename F, typename V, typename Func>
RedisError RedisKeyFieldMap<K,F,V,Func>::find(const Key& key, const Field& field, Value& value) const
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "HGET");

    cmd.add(field);

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

/** \brief This method returns a RedisResponse containing a *single* 'page' of key/value elements along with an embedded scan 'cursor'.
    Repeated calls to scan with the same scanResult enable iterating the entire field map one page at a time.
    To determine if the next scan could return any more elements, check scanResult->getArrayElement(REDIS_SCAN_RESULT_CURSOR).getString() != "0"
    */
template<typename K, typename F, typename V, typename Func>
RedisError RedisKeyFieldMap<K,F,V,Func>::scan(const Key& key, RedisResponsePtr& scanResult, uint32_t pageSizeHint)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "HSCAN");

    const char8_t* cursor = "0"; // cursor returned by SCAN is always a string-formatted int
    if (scanResult.get() != nullptr && scanResult->isArray())
        cursor = scanResult->getArrayElement(REDIS_SCAN_RESULT_CURSOR).getString();
    cmd.add(cursor);
    if (pageSizeHint > 0)
    {
        cmd.add("COUNT");
        cmd.add(pageSizeHint);
    }

    RedisError rc = exec(cmd, scanResult);
    if (rc == REDIS_ERR_OK && 
        scanResult->isArray() && 
        scanResult->getArraySize() == REDIS_SCAN_RESULT_MAX && 
        scanResult->getArrayElement(REDIS_SCAN_RESULT_ARRAY).getArraySize() % 2 == 0) // result returns pairs of key/value
    {
        rc = REDIS_ERR_OK;
    }
    else
        rc = REDIS_ERR_SYSTEM;

    return rc;
}

template<typename K, typename F, typename V, typename Func>
RedisError RedisKeyFieldMap<K,F,V,Func>::incrBy(const Key& key, const Field& field, int64_t delta, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "HINCRBY");

    cmd.add(field);
    cmd.add(delta);

    return execCommand(cmd, result);
}

template<typename K, typename F, typename V, typename Func>
RedisError RedisKeyFieldMap<K,F,V,Func>::decrBy(const Key& key, const Field& field, int64_t delta, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "HINCRBY");

    cmd.add(field);
    cmd.add(-delta);

    return execCommand(cmd, result);
}

template<typename K, typename F, typename V, typename Func>
RedisError RedisKeyFieldMap<K,F,V,Func>::decrRemoveWhenZero(const Key& key, const Field& field, bool* removed)
{
    RedisCommand cmd;
    cmd.beginKey();
    cmd.add(eastl::make_pair(getCollectionKey(Func()(key)), key));
    cmd.end();
    cmd.add(field);

    cmd.setNamespace(getCollectionId().getNamespace());

    RedisResponsePtr resp;
    RedisError rc = execScript(msDecrFieldRemoveWhenZero, cmd, resp);
    if (rc == REDIS_ERR_OK)
    {
        if (removed != nullptr)
            *removed = (resp->getInteger() == 0);
    }

    return rc;
}

template<typename K, typename F, typename V, typename Func>
RedisError RedisKeyFieldMap<K,F,V,Func>::getFields(const Key& key, FieldList& fields) const
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "HGETALL");

    RedisResponsePtr resp;
    RedisError rc = exec(cmd, resp);
    if (rc == REDIS_ERR_OK && resp->isArray())
    {
        uint32_t arraySize = resp->getArraySize();

        // Expect field/value pairs in the array response
        if (arraySize%2 != 0)
            return REDIS_ERR_SYSTEM;

        fields.reserve(arraySize/2);
        for (uint32_t i = 0; i < arraySize; i+=2)
        {
            const RedisResponse& element = resp->getArrayElement(i);
            if (element.isString())
            {
                Field& field = fields.push_back();
                const char8_t* begin = element.getString();
                const char8_t* end = begin + element.getStringLen();
                RedisDecoder rdecoder;
                if (rdecoder.decodeValue(begin, end, field) == nullptr)
                {
                    rc = REDIS_ERR_SYSTEM;
                    break;
                }
            }
        }
    }
    return rc;
}

template<typename K, typename F, typename V, typename Func>
RedisError RedisKeyFieldMap<K,F,V,Func>::getFieldValueMap(const Key& key, FieldValueMap& fieldValueMap) const
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "HGETALL");

    RedisResponsePtr resp;
    RedisError rc = exec(cmd, resp);
    if (rc == REDIS_ERR_OK && resp->isArray())
    {
        uint32_t arraySize = resp->getArraySize();

        // Expect field/value pairs in the array response
        if (arraySize%2 != 0)
            return REDIS_ERR_SYSTEM;

        for (uint32_t i = 0; i < arraySize; ++i)
        {
            const RedisResponse& fieldElement = resp->getArrayElement(i);
            const RedisResponse& valueElement = resp->getArrayElement(++i);
            if (fieldElement.isString() && valueElement.isString())
            {
                Field field;
                RedisDecoder rdecoder;
                if (rdecoder.decodeValue(fieldElement.getString(), fieldElement.getString() + fieldElement.getStringLen(), field) == nullptr)
                {
                    rc = REDIS_ERR_SYSTEM;
                    break;
                }

                Value& value = fieldValueMap.insert(field).first->second;
                if (rdecoder.decodeValue(valueElement.getString(), valueElement.getString() + valueElement.getStringLen(), value) == nullptr)
                {
                    rc = REDIS_ERR_SYSTEM;
                    break;
                }
            }
        }
    }
    return rc;
}

template<typename K, typename F, typename V, typename Func>
RedisError RedisKeyFieldMap<K,F,V,Func>::expire(const Key& key, EA::TDF::TimeValue relativeTimeout, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "EXPIRE");
    cmd.add(relativeTimeout.getSec());

    return execCommand(cmd, result);
}

template<typename K, typename F, typename V, typename Func>
RedisError RedisKeyFieldMap<K,F,V,Func>::pexpire(const Key& key, EA::TDF::TimeValue relativeTimeout, int64_t* result)
{
    RedisCommand cmd;
    buildCommand(cmd, Func()(key), key, "PEXPIRE");
    cmd.add(relativeTimeout.getMillis());

    return execCommand(cmd, result);
}

template<typename K, typename F, typename V, typename Func>
RedisError RedisKeyFieldMap<K,F,V,Func>::getTimeToLive(const Key& key, EA::TDF::TimeValue& relativeTimeout)
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

/** \brief DEPRECATED - This method cannot be executed reliably on Redis in cluster mode

           This method subdivides the given KeySet according to the Redis node that would store each key. It then executes a
           lua script on the appropriate node for each subset, and appends each response to the supplied results vector. Each 
           subset will include at most maxKeysPerCall keys; for larger subsets, there will be multiple executions on the same node.
    */
template<typename K, typename F, typename V, typename Func>
RedisError RedisKeyFieldMap<K,F,V,Func>::getAllByKeys(const KeySet& keys, eastl::vector<RedisResponsePtr>& results, uint32_t maxKeysPerCall) const
{
    RedisCommandsByMasterNameMap commandsByMasterName;
    RedisCommand temp;
    RedisError err = REDIS_ERR_OK;
    for (typename Blaze::RedisKeyFieldMap<K, F, V, Func>::KeySet::const_iterator i = keys.begin(), e = keys.end(); i != e; ++i)
    {
        const Key& k = *i;
        SliverId sliverId = Func()(k);
        temp.beginKey();
        temp.add(eastl::make_pair(getCollectionKey(sliverId), k));
        temp.end();
        const char8_t* key = nullptr;
        size_t keylen = 0;

        if (!temp.getParam(0, key, keylen) || !addParamToRedisCommandsByMasterNameMap(key, keylen, commandsByMasterName, maxKeysPerCall))
        {
            err = REDIS_ERR_SYSTEM;
            break;
        }

        temp.reset();
    }

    if (err == REDIS_ERR_OK)
        err = execScriptByMasterName(msGetAllByKeys, commandsByMasterName, results);

    for (RedisCommandsByMasterNameMap::iterator itr = commandsByMasterName.begin(); itr != commandsByMasterName.end(); ++itr)
    {
        for (RedisCommandList::iterator cmdItr = itr->second.begin(); cmdItr != itr->second.end(); ++cmdItr)
            delete *cmdItr;
    }
    return err;
}

template<typename K, typename F, typename V, typename Func>
bool RedisKeyFieldMap<K,F,V,Func>::extractFieldArrayElement(const RedisResponse& resultArray, uint32_t index, Field& field)
{
    // NOTE: this wrapper function is just for type safety
    return resultArray.extractArrayElement(index, field);
}

template<typename K, typename F, typename V, typename Func>
bool RedisKeyFieldMap<K,F,V,Func>::extractValueArrayElement(const RedisResponse& resultArray, uint32_t index, Value& value)
{
    // NOTE: this wrapper function is just for type safety
    return resultArray.extractArrayElement(index, value);
}

} // Blaze

#endif // BLAZE_REDIS_KEYFIELDMAP_H

