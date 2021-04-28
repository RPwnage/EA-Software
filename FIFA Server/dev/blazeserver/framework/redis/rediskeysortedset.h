/*************************************************************************************************/
/*!
\file

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REDIS_KEYSORTEDSET_H
#define BLAZE_REDIS_KEYSORTEDSET_H

/*** Include files *******************************************************************************/

#include "EASTL/vector.h"
#include "framework/redis/rediscollection.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
    /**
    *  /class RedisKeySortedSet
    *
    *  This class wraps the Sorted Set data type from Redis.
    *  Like normal sets, there can only be one of each member of a set, but Sorted Sets
    *  also have scores (implemented in Redis as 64 bit floating point).
    *
    *  For example:
    *
    *  "exampleSortedSet42":
    *      "the first" : 1
    *      "pi" : 3.14
    *      "greatestOfAllTime" : +inf
    *
    *  /note The template arguments are used to lock down the types managed by the collection and
    *  to provide explicit and automatic conversion between the application layer and the redis
    *  storage layer.  In the example above, K would be "exampleSortedSet42" and V would be
    *  the members (string in this case).  The scores are always type double, since this is
    *  the data type Redis uses internally to represent scores.
    *
    *  /examples See testRedisKeySortedSet() @ redisunittests.cpp
    */
    template<typename K, typename V, typename Func = RedisGetSliverIdFromKey<K> >
    class RedisKeySortedSet : public RedisCollection
    {
    public:
        typedef K Key;
        typedef V Value;
        typedef eastl::vector<Value> ValueList;

        RedisKeySortedSet(const RedisId& id, bool isSharded = false) : RedisCollection(id, isSharded) {}
        RedisError exists(const Key& key, int64_t* result) const;
        RedisError size(const Key& key, int64_t* result) const;
        RedisError remove(const Key& key, const Value& member, int64_t* result = nullptr);
        RedisError remove(const Key& key, const ValueList& members, int64_t* result = nullptr);
        RedisError rank(const Key& key, const Value& member, bool reverseSort, int64_t* result);
        RedisError add(const Key& key, const Value& member, double score, int64_t* result = nullptr);
        RedisError incrementScoreBy(const Key& key, const Value& member, double increment, int64_t* result = nullptr);
        RedisError getScore(const Key& key, const Value& member, double& score);
        RedisError getRange(const Key& key, int64_t startIndex, int64_t endIndex, bool reverseSort, ValueList& members, eastl::vector<double>* scores = nullptr) const;
        RedisError removeRange(const Key& key, int64_t startIndex, int64_t endIndex, int64_t* result = nullptr) const;
        RedisError getRangeByScore(const Key& key, double minScore, double maxScore, bool reverseSort, int32_t offset, int32_t count,
            ValueList& members, eastl::vector<double>* scores = nullptr, bool minExclusive = false, bool maxExclusive = false) const;
        RedisError erase(const Key& key, int64_t* result);
        RedisError expire(const Key& key, EA::TDF::TimeValue timeout, int64_t* result);
        RedisError pexpire(const Key& key, EA::TDF::TimeValue timeout, int64_t* result);
        RedisError getTimeToLive(const Key& key, EA::TDF::TimeValue& relativeTimeout);

    private:
        RedisError parseMembers(RedisResponsePtr resp, ValueList& members, eastl::vector<double>* scores) const;
    };

    template<typename K, typename V, typename Func>
    RedisError RedisKeySortedSet<K, V, Func>::exists(const Key& key, int64_t* result) const
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "EXISTS");

        return execCommand(cmd, result);
    }

    template<typename K, typename V, typename Func>
    RedisError RedisKeySortedSet<K, V, Func>::size(const Key& key, int64_t* result) const
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "ZCARD");

        return execCommand(cmd, result);
    }

    template<typename K, typename V, typename Func>
    RedisError RedisKeySortedSet<K, V, Func>::add(const Key& key, const Value& member, double score, int64_t* result)
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "ZADD");

        cmd.add(score);
        cmd.add(member);

        // result contains the number of elements that were added only (doesn't included existing elements with modified score)
        return execCommand(cmd, result);
    }

    template<typename K, typename V, typename Func>
    RedisError RedisKeySortedSet<K, V, Func>::remove(const Key& key, const Value& member, int64_t* result)
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "ZREM");

        cmd.add(member);

        return execCommand(cmd, result);
    }

    template<typename K, typename V, typename Func>
    RedisError RedisKeySortedSet<K, V, Func>::remove(const Key& key, const ValueList& members, int64_t* result)
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "ZREM");

        for (const auto& member : members)
        {
            cmd.add(member);
        }

        return execCommand(cmd, result);
    }

    template<typename K, typename V, typename Func>
    RedisError RedisKeySortedSet<K, V, Func>::rank(const Key& key, const Value& member, bool reverseSort, int64_t* result)
    {
        RedisCommand cmd;
        if (reverseSort)
        {
            buildCommand(cmd, Func()(key), key, "ZREVRANK");
        }
        else
        {
            buildCommand(cmd, Func()(key), key, "ZRANK");
        }

        cmd.add(member);

        // -1 result if not found
        return execCommand(cmd, result);
    }

    template<typename K, typename V, typename Func>
    RedisError RedisKeySortedSet<K, V, Func>::incrementScoreBy(const Key& key, const Value& member, double increment, int64_t* result)
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "ZINCRBY");

        cmd.add(increment);
        cmd.add(member);

        return execCommand(cmd, result);
    }

    template<typename K, typename V, typename Func>
    RedisError RedisKeySortedSet<K, V, Func>::getScore(const Key& key, const Value& member, double& score)
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "ZSCORE");

        cmd.add(member);

        RedisResponsePtr resp;
        RedisError rc = exec(cmd, resp);
        if (rc == REDIS_ERR_OK)
        {
            if (resp->isString())
            {
                const char8_t* begin = resp->getString();
                const char8_t* end = begin + resp->getStringLen();
                RedisDecoder rdecoder;

                if (rdecoder.decodeValue(begin, end, score) == nullptr)
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
    RedisError RedisKeySortedSet<K, V, Func>::getRange(const Key& key, int64_t startIndex, int64_t endIndex, bool reverseSort, ValueList& members, eastl::vector<double>* scores) const
    {
        members.clear();

        RedisCommand cmd;
        if (reverseSort)
        {
            buildCommand(cmd, Func()(key), key, "ZREVRANGE");        // scores returned in descending order
        }
        else
        {
            buildCommand(cmd, Func()(key), key, "ZRANGE");           // scores returned in ascending order
        }

        cmd.add(startIndex);
        cmd.add(endIndex);
        if (scores != nullptr)
        {
            scores->clear();
            cmd.add("WITHSCORES");
        }

        RedisResponsePtr resp;
        RedisError rc = exec(cmd, resp);
        if (rc == REDIS_ERR_OK)
        {
            rc = parseMembers(resp, members, scores);
        }

        return rc;
    }

    template<typename K, typename V, typename Func>
    RedisError RedisKeySortedSet<K, V, Func>::getRangeByScore(const Key& key, double minScore, double maxScore, bool reverseSort,
        int32_t offset, int32_t count, ValueList& members, eastl::vector<double>* scores, bool minExclusive, bool maxExclusive) const
    {
        members.clear();

        RedisCommand cmd;
        if (reverseSort)
        {
            buildCommand(cmd, Func()(key), key, "ZREVRANGEBYSCORE");        // scores returned in descending order
            cmd.begin().add(maxExclusive ? "(" : "").add(maxScore).end();   // first arg is max...
            cmd.begin().add(minExclusive ? "(" : "").add(minScore).end();
        }
        else
        {
            buildCommand(cmd, Func()(key), key, "ZRANGEBYSCORE");           // scores returned in ascending order
            cmd.begin().add(minExclusive ? "(" : "").add(minScore).end();   // first arg is min...
            cmd.begin().add(maxExclusive ? "(" : "").add(maxScore).end();
        }

        if (scores != nullptr)
        {
            scores->clear();
            cmd.add("WITHSCORES");
        }

        if (offset != 0 || count != 0)
        {
            cmd.add("LIMIT");
            cmd.add(offset);   // number of members to skip in response
            cmd.add(count);    // number of items to return total (after offset)
        }

        RedisResponsePtr resp;
        RedisError rc = exec(cmd, resp);
        if (rc == REDIS_ERR_OK)
        {
            rc = parseMembers(resp, members, scores);
        }

        return rc;
    }

    template<typename K, typename V, typename Func>
    RedisError RedisKeySortedSet<K, V, Func>::parseMembers(RedisResponsePtr resp, ValueList& members, eastl::vector<double>* scores) const
    {
        RedisError rc = REDIS_ERR_OK;

        const bool includeScores = (scores != nullptr);

        if (resp->isArray())
        {
            if (includeScores)
            {
                // half of the results are members, the other half are their scores
                members.reserve(resp->getArraySize() / 2);
                scores->reserve(resp->getArraySize() / 2);
            }
            else
            {
                members.reserve(resp->getArraySize());
            }

            bool readingScore = false;
            for (uint32_t i = 0, sz = resp->getArraySize(); i < sz; ++i)
            {
                const RedisResponse& element = resp->getArrayElement(i);
                if (element.isString())
                {
                    const char8_t* begin = element.getString();
                    const char8_t* end = begin + element.getStringLen();
                    RedisDecoder rdecoder;

                    if (readingScore)
                    {
                        double score = 0.0;

                        if (rdecoder.decodeValue(begin, end, score) == nullptr)
                        {
                            rc = REDIS_ERR_SYSTEM;
                            break;
                        }
                        else
                        {
                            scores->push_back(score);
                        }

                        readingScore = false;
                    }
                    else
                    {
                        Value& member = members.push_back();
                        if (rdecoder.decodeValue(begin, end, member) == nullptr)
                        {
                            rc = REDIS_ERR_SYSTEM;
                            break;
                        }

                        if (includeScores)
                        {
                            // next entry will be the score
                            readingScore = true;
                        }
                    }
                }
            }
        }

        return rc;
    }
    
    template<typename K, typename V, typename Func>
    RedisError RedisKeySortedSet<K, V, Func>::removeRange(const Key& key, int64_t startIndex, int64_t endIndex, int64_t* result) const
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "ZREMRANGEBYRANK");

        cmd.add(startIndex);
        cmd.add(endIndex);

        return execCommand(cmd, result);
    }

    template<typename K, typename V, typename Func>
    RedisError RedisKeySortedSet<K, V, Func>::erase(const Key& key, int64_t* result)
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "DEL");

        return execCommand(cmd, result);
    }

    template<typename K, typename V, typename Func>
    RedisError RedisKeySortedSet<K, V, Func>::expire(const Key& key, EA::TDF::TimeValue relativeTimeout, int64_t* result)
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "EXPIRE");
        cmd.add(relativeTimeout.getSec());

        return execCommand(cmd, result);
    }

    template<typename K, typename V, typename Func>
    RedisError RedisKeySortedSet<K, V, Func>::pexpire(const Key& key, EA::TDF::TimeValue relativeTimeout, int64_t* result)
    {
        RedisCommand cmd;
        buildCommand(cmd, Func()(key), key, "PEXPIRE");
        cmd.add(relativeTimeout.getMillis());

        return execCommand(cmd, result);
    }

    template<typename K, typename V, typename Func>
    RedisError RedisKeySortedSet<K, V, Func>::getTimeToLive(const Key& key, EA::TDF::TimeValue& relativeTimeout)
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

#endif // BLAZE_REDIS_KEYSORTEDSET_H

