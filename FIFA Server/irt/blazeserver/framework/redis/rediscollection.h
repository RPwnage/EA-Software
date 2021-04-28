/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REDIS_COLLECTION_H
#define BLAZE_REDIS_COLLECTION_H

/*** Include files *******************************************************************************/
#include "EASTL/utility.h"
#include "framework/redis/redisid.h"
#include "framework/redis/rediscluster.h"
#include "framework/redis/redissliver.h"
#include "framework/redis/redisdecoder.h"
#include "framework/redis/rediscommand.h"
#include "framework/redis/redisresponse.h"
#include "framework/redis/rediserrors.h"
#include "framework/redis/redisscriptregistry.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


namespace Blaze
{

const uint32_t REDIS_SCAN_RESULT_CURSOR = 0;
const uint32_t REDIS_SCAN_RESULT_ARRAY = 1;
const uint32_t REDIS_SCAN_RESULT_MAX = 2;
const uint32_t REDIS_SCAN_STRIDE_MIN = 100; // Redis SCAN stride(COUNT) default is 10 which is far too small.
const uint32_t REDIS_SCAN_STRIDE_MAX = 100000; // Define Redis SCAN stride(COUNT) max to be large enough to allow fast striding through Redis Keyspace of 10M keys (max 100 strides of 0.1s each).

template<typename Key>
struct RedisGetSliverIdFromKey
{
    SliverId operator()(const Key&) { return INVALID_SLIVER_ID; }
};

template<>
struct RedisGetSliverIdFromKey<uint64_t>
{
    SliverId operator()(const uint64_t& key) { return GetSliverIdFromSliverKey(key); }
};

class RedisCollectionManager;

class RedisCollection
{
    NON_COPYABLE(RedisCollection);
public:
    typedef eastl::pair<RedisId, RedisSliver> CollectionKey;
    typedef eastl::vector<RedisCommand*> RedisCommandList;
    typedef eastl::hash_map<eastl::string, RedisCommandList> RedisCommandsByMasterNameMap;

    bool registerCollection();
    bool deregisterCollection();
    bool isRegistered() const { return mIsRegistered; }
    bool isFirstRegistration() const { return mIsFirstRegistration; }
    const RedisId& getCollectionId() const { return mCollectionId; }
    CollectionKey getCollectionKey(SliverId sliverId) const;

    template<typename Key>
    void buildCommand(RedisCommand& cmd, SliverId sliverId, const Key& key, const char8_t* command) const;
    static RedisError execCommand(RedisCommand& cmd, int64_t* result = nullptr);

protected:
    friend class RedisCollectionManager;
    RedisCollection(const RedisId& collectionId, bool isSharded)
        : mCollectionId(collectionId), mIsSharded(isSharded), mIsRegistered(false), mIsFirstRegistration(false) {}
    virtual ~RedisCollection() {}
    static RedisError exec(const RedisCommand &command, RedisResponsePtr &response);
    static RedisError execScript(const RedisScript& script, const RedisCommand &command, RedisResponsePtr &response);
    RedisId mCollectionId;
    bool mIsSharded;
    bool mIsRegistered;
    bool mIsFirstRegistration;
    static RedisScript msDecrRemoveWhenZero;
    static RedisScript msDecrFieldRemoveWhenZero;

    // DEPRECATED - These methods and script are only used by RediskeyFieldMap::getAllByKeys,
    // which is itself deprecated because it can't be executed reliably on redis in cluster mode
    static RedisError execScriptByMasterName(const RedisScript& script, const RedisCommandsByMasterNameMap &commandMap, eastl::vector<RedisResponsePtr> &results);
    bool addParamToRedisCommandsByMasterNameMap(const char8_t* param, size_t paramlen, RedisCommandsByMasterNameMap &commandMap, uint32_t maxParamsPerCommand) const;
    static RedisScript msGetAllByKeys;

};

template<typename Key>
void RedisCollection::buildCommand(RedisCommand& cmd, SliverId sliverId, const Key& key, const char8_t* command) const
{
    cmd.add(command);
    cmd.beginKey();
    cmd.add(eastl::make_pair(getCollectionKey(sliverId), key));
    cmd.end();

    cmd.setNamespace(getCollectionId().getNamespace());
}

} // Blaze

#endif // BLAZE_REDIS_COLLECTION_H

