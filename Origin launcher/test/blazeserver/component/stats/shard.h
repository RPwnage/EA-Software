/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_STATS_SHARD_H
#define BLAZE_STATS_SHARD_H

/*** Include files *******************************************************************************/
#include "EASTL/list.h"
#include "EASTL/hash_map.h"
#include "framework/database/dbconn.h"
#include "framework/util/lrucache.h"
#include "framework/identity/identity.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
namespace Stats
{

typedef eastl::vector<uint32_t> DbIdList;
typedef eastl::hash_map<uint32_t, EntityIdList> ShardEntityMap;
typedef eastl::hash_set<EntityId> EntityIdSet;

// For a given entity type, the ShardData class manages the assignment, caching, and lookup of shards for entities
class ShardData
{
public:
    ShardData();
    ~ShardData();

    void init(uint32_t dbId, const EA::TDF::ObjectType& objectType);
    const char8_t* getTableName() const { return mTableName; }
    DbIdList& getDbIds() { return mDbIds; }
    const DbIdList& getDbIds() const { return mDbIds; }
    BlazeRpcError lookupDbId(EntityId entityId, uint32_t& dbId) const;
    BlazeRpcError lookupEntities(const EntityIdList& entityIds, ShardEntityMap& shardEntityMap) const;
    void invalidateEntity(EntityId entityId) const;

private:
    // Most config data is const, however we need a mutable LRU cache to hold recently
    // looked up entities, as well as a "next shard" value, used to assign new users to
    // a round robin shard
    char8_t* mTableName;
    mutable uint32_t mNextShard;
    uint32_t mDbId; // Main stats database
    DbIdList mDbIds; // Sharded database ids
    mutable LruCache<EntityId, uint32_t> mLruCache;
};

typedef eastl::hash_map<EA::TDF::ObjectType, ShardData> ShardMap;

// The singleton owned by the stats config, used to lookup all db ids for an entity type, or
// to lookup the db id(s) for one or more specific entity(ies) in a given entity type
class ShardConfiguration
{
public:
    ShardConfiguration();
    void init(uint32_t mainDbId);
    ShardMap& getShardMap() { return mShardMap; } // Maybe make this private and a friend of StatsConfig???
    const ShardMap& getShardMap() const { return mShardMap; }
    const DbIdList& getDbIds(EA::TDF::ObjectType entityType) const;
    BlazeRpcError lookupDbId(EA::TDF::ObjectType entityType, EntityId entityId, uint32_t&  dbId) const;
    BlazeRpcError lookupDbIds(EA::TDF::ObjectType entityType, const EntityIdList& list, ShardEntityMap& shardEntityMap) const;
    void invalidateEntity(EA::TDF::ObjectType entityType, EntityId entityId) const;

private:
    uint32_t mMainDbId;
    DbIdList mMainDbIdList;
    ShardMap mShardMap;
};

// A utility class that contains some parameters around a DB connection, useful in conjunction
// with the StatsDbConnectionManager to handle sharded db requests to multiple database
class StatsConnTracker
{
public:
    StatsConnTracker() 
    {
        mItr = mResultPtrs.cbegin();
    }

    DbConnPtr mConn;
    QueryPtr mQuery;
    DbResultPtrs mResultPtrs;
    DbResultPtrs::const_iterator mItr;
};

typedef eastl::hash_map<uint32_t, StatsConnTracker> StatsConnTrackerMap;

// For those stats calls that need to (potentially) manage several connections and possibly transactions
// this class holds on to the multiple connections, and a query object on each of those connections
class StatsDbConnectionManager
{
public:
    StatsDbConnectionManager() {}
    BlazeRpcError acquireConnWithTxn(uint32_t dbId, DbConnPtr& conn, QueryPtr& query);
    QueryPtr getQuery(uint32_t dbId);
    BlazeRpcError execute(bool trackResults);
    void resetQueries();
    BlazeRpcError commit();
    BlazeRpcError rollback();

    DbResultPtrs::const_iterator getNextResultItr(uint32_t dbId);
    DbResultPtrs::const_iterator getEndResultItr(uint32_t dbId);

private:
    StatsConnTrackerMap mTrackerMap;
};

} // Stats
} // Blaze
#endif // BLAZE_STATS_SHARD_H
