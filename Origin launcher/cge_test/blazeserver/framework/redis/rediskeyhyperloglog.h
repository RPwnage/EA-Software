/*************************************************************************************************/
/*!
\file

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REDIS_KEYHYPERLOGLOG_H
#define BLAZE_REDIS_KEYHYPERLOGLOG_H

/*** Include files *******************************************************************************/

#include "framework/redis/rediscollection.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
    /**
    *  /class RedisKeyHyperLogLog
    * 
    *  This class wraps the HyperLogLog data type from Redis.
    *  This collection is used to estimate the number of entries in a set
    *  without having to store all of the items being counted.  The most memory
    *  that a HyperLogLog set will consume in Redis is only about 12k, regardless
    *  of how many entries are added.  Because it's an estimate, you can expect
    *  a standard error rate of 0.81% on the number of elements in the set, but
    *  there is practically no limit to the number of elements that can be added/counted.
    *  Another unique property of HyperLogLog is that sets can be merged
    *  into a new set, containing the estimated total unique elements
    *  of the source sets.

    *  Cluster Caveat:
    *  PFMERGE assumes that the keys being merged (and the destination key) all
    *  exist on the same node.  You can't merge keys from different nodes.  If
    *  you need to merge keys, you must ensure they all hash to the same keyslot
    *  by matching the text between braces {} inside their keys.  Since 'add' and 'count'
    *  operate on a single key, they can reside on any keyslot/node.  You only need
    *  to force a matching keyslot using {} if you intend to merge those keys.
    *
    *  /examples See testRedisKeyHyperLogLog() @ redisunittests.cpp
    */
    template<typename K, typename V, typename Func = RedisGetSliverIdFromKey<K> >
    class RedisKeyHyperLogLog : public RedisCollection
    {
    public:
        typedef K Key;
        typedef V Value;

        RedisKeyHyperLogLog(const RedisId& id, bool isSharded = false) : RedisCollection(id, isSharded) {}
        RedisError exists(const Key& key, int64_t* result) const;
        RedisError add(const Key& key, const Value& element, int64_t* result = nullptr) const;
        RedisError count(const Key& key, int64_t* result) const;
        RedisError merge(const Key& keyDest, const Key& keySource1, const Key& keySource2, int64_t* result) const;
        RedisError erase(const Key& key, int64_t* result) const;
        RedisError expire(const Key& key, EA::TDF::TimeValue timeout, int64_t* result = nullptr) const;
        RedisError pexpire(const Key& key, EA::TDF::TimeValue timeout, int64_t* result = nullptr) const;
        RedisError getTimeToLive(const Key& key, EA::TDF::TimeValue& relativeTimeout) const;
    };

    template<typename K, typename V, typename Func>
    RedisError RedisKeyHyperLogLog<K, V, Func>::exists(const Key& key, int64_t* result) const
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "EXISTS");

        return execCommand(cmd, result);
    }

    template<typename K, typename V, typename Func>
    RedisError RedisKeyHyperLogLog<K, V, Func>::add(const Key& key, const Value& element, int64_t* result) const
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "PFADD");

        cmd.add(element);

        // result will be 1 if added, or 0 if the entry already existed
        return execCommand(cmd, result);
    }
    
    template<typename K, typename V, typename Func>
    RedisError RedisKeyHyperLogLog<K, V, Func>::count(const Key& key, int64_t* result) const
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "PFCOUNT");

        // result contains a very close estimate on the number of elements that have been added so far
        return execCommand(cmd, result);
    }
    
    template<typename K, typename V, typename Func>
    RedisError RedisKeyHyperLogLog<K, V, Func>::merge(const Key& keyDest, const Key& keySource1, const Key& keySource2, int64_t* result) const
    {
        // NOTE: all keys must hash to the same keyslot if using a cluster.  If you aren't using {} in
        // the keys, this merge will not do what you expect.  See 'Cluster Caveat' above.
        RedisCommand cmd;
        buildCommand(cmd, Func()(keyDest), keyDest, "PFMERGE");

        cmd.add(eastl::make_pair(getCollectionKey(Func()(keySource1)), keySource1));
        cmd.add(eastl::make_pair(getCollectionKey(Func()(keySource2)), keySource2));

        return execCommand(cmd, result);
    }

    template<typename K, typename V, typename Func>
    RedisError RedisKeyHyperLogLog<K, V, Func>::erase(const Key& key, int64_t* result) const
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "DEL");

        return execCommand(cmd, result);
    }

    template<typename K, typename V, typename Func>
    RedisError RedisKeyHyperLogLog<K, V, Func>::expire(const Key& key, EA::TDF::TimeValue relativeTimeout, int64_t* result) const
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "EXPIRE");
        cmd.add(relativeTimeout.getSec());

        return execCommand(cmd, result);
    }

    template<typename K, typename V, typename Func>
    RedisError RedisKeyHyperLogLog<K, V, Func>::pexpire(const Key& key, EA::TDF::TimeValue relativeTimeout, int64_t* result) const
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "PEXPIRE");
        cmd.add(relativeTimeout.getMillis());

        return execCommand(cmd, result);
    }

    template<typename K, typename V, typename Func>
    RedisError RedisKeyHyperLogLog<K, V, Func>::getTimeToLive(const Key& key, EA::TDF::TimeValue& relativeTimeout) const
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

#endif // BLAZE_REDIS_KEYHYPERLOGLOG_H

