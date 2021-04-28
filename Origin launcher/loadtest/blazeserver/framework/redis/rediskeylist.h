/*************************************************************************************************/
/*!
\file

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REDIS_KEYLIST_H
#define BLAZE_REDIS_KEYLIST_H

/*** Include files *******************************************************************************/

#include "framework/redis/rediscollection.h"

#include "EASTL/type_traits.h"
#include "EASTL/vector.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


namespace Blaze
{

    /**
    *  /class RedisKeyList
    *
    *  This class implements a generic type safe interface for the redis 'keyed' lists.
    *  The keys can be primitives or composite types(e.g: eastl::pair) compatible with
    *  RedisDecoder (see redisdecoder.h).
    *
    *  This class is appropriate for implementing distributed collections analogous to:
    *
    *  hash_map<int32_t, list<string> >
    *  hash_map<int32_t, hash_map<string, list<uint64_t> > >
    *
    *  /note The template arguments are used to lock down the types managed by the collection and
    *  to provide explicit and automatic conversion between the application layer and the redis
    *  storage layer.
    *
    *  /examples See testRedisKeyList() @ redisunittests.cpp
    */
    template<typename K, typename V, typename Func = RedisGetSliverIdFromKey<K> >
    class RedisKeyList : public RedisCollection
    {
    public:
        typedef K Key;
        typedef V Value;
        typedef eastl::vector<Value> ValueList;

        RedisKeyList(const RedisId& id, bool isSharded = false) : RedisCollection(id, isSharded) {}
        RedisError exists(const Key& key, int64_t* result) const;
        RedisError pushFront(const Key& key, const Value& value, int64_t* result = nullptr);
        RedisError pushFront(const Key& key, const ValueList& values, int64_t* result = nullptr);
        RedisError pushBack(const Key& key, const Value& value, int64_t* result = nullptr);
        RedisError set(const Key& key, int64_t index, const Value& value, int64_t* result = nullptr);
        RedisError get(const Key& key, int64_t index, Value& value) const;
        RedisError getRange(const Key& key, int64_t startIndex, int64_t endIndex, ValueList& values) const;
        RedisError popFront(const Key& key, Value& value) const;
        RedisError popBack(const Key& key, Value& value) const;
        RedisError trim(const Key& key, int64_t startIndex, int64_t endIndex, int64_t* result = nullptr) const;
        RedisError expire(const Key& key, EA::TDF::TimeValue timeout, int64_t* result = nullptr);
        RedisError pexpire(const Key& key, EA::TDF::TimeValue timeout, int64_t* result = nullptr);
        RedisError getTimeToLive(const Key& key, EA::TDF::TimeValue& relativeTimeout);
    };

    template<typename K, typename V, typename Func>
    RedisError RedisKeyList<K, V, Func>::exists(const Key& key, int64_t* result) const
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "EXISTS");

        return execCommand(cmd, result);
    }

    template<typename K, typename V, typename Func>
    RedisError RedisKeyList<K, V, Func>::pushFront(const Key& key, const Value& value, int64_t* result)
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "LPUSH");

        cmd.add(value);

        return execCommand(cmd, result);
    }

    template<typename K, typename V, typename Func>
    RedisError RedisKeyList<K, V, Func>::pushFront(const Key& key, const ValueList& values, int64_t* result)
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "LPUSH");

        // if we want the first item in our list to be the first item in our collection, we have to push it on last
        for (typename ValueList::const_reverse_iterator it = values.rbegin(), end = values.rend(); it != end; ++it)
        {
            cmd.add(*it);
        }

        return execCommand(cmd, result);
    }

    template<typename K, typename V, typename Func>
    RedisError RedisKeyList<K, V, Func>::set(const Key& key, int64_t index, const Value& value, int64_t* result)
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "LSET");

        cmd.add(index);
        cmd.add(value);

        return execCommand(cmd, result);
    }

    template<typename K, typename V, typename Func>
    RedisError RedisKeyList<K, V, Func>::get(const Key& key, int64_t index, Value& value) const
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "LINDEX");

        cmd.add(index);

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
                {
                    rc = REDIS_ERR_SYSTEM;
                }
            }
            else
            {
                rc = REDIS_ERR_NOT_FOUND;
            }
        }
        return rc;
    }

    template<typename K, typename V, typename Func>
    RedisError RedisKeyList<K, V, Func>::getRange(const Key& key, int64_t startIndex, int64_t endIndex, ValueList& values) const
    {
        values.clear();

        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "LRANGE");

        cmd.add(startIndex);
        cmd.add(endIndex);

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
    RedisError RedisKeyList<K, V, Func>::pushBack(const Key& key, const Value& value, int64_t* result)
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "RPUSH");

        cmd.add(value);

        return execCommand(cmd, result);
    }


    template<typename K, typename V, typename Func>
    RedisError RedisKeyList<K, V, Func>::popFront(const Key& key, Value& value) const
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "LPOP");

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
                {
                    rc = REDIS_ERR_SYSTEM;
                }
            }
            else
            {
                rc = REDIS_ERR_NOT_FOUND;
            }
        }
        return rc;
    }

    template<typename K, typename V, typename Func>
    RedisError RedisKeyList<K, V, Func>::popBack(const Key& key, Value& value) const
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "RPOP");

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
                {
                    rc = REDIS_ERR_SYSTEM;
                }
            }
            else
            {
                rc = REDIS_ERR_NOT_FOUND;
            }
        }
        return rc;
    }


    template<typename K, typename V, typename Func>
    RedisError RedisKeyList<K, V, Func>::trim(const Key& key, int64_t startIndex, int64_t endIndex, int64_t* result) const
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "LTRIM");

        cmd.add(startIndex);
        cmd.add(endIndex);

        return execCommand(cmd, result);
    }

    template<typename K, typename V, typename Func>
    RedisError RedisKeyList<K, V, Func>::expire(const Key& key, EA::TDF::TimeValue relativeTimeout, int64_t* result)
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "EXPIRE");
        cmd.add(relativeTimeout.getSec());

        return execCommand(cmd, result);
    }

    template<typename K, typename V, typename Func>
    RedisError RedisKeyList<K, V, Func>::pexpire(const Key& key, EA::TDF::TimeValue relativeTimeout, int64_t* result)
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "PEXPIRE");
        cmd.add(relativeTimeout.getMillis());

        return execCommand(cmd, result);
    }

    template<typename K, typename V, typename Func>
    RedisError RedisKeyList<K, V, Func>::getTimeToLive(const Key& key, EA::TDF::TimeValue& relativeTimeout)
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

#endif // BLAZE_REDIS_KEYLIST_H
