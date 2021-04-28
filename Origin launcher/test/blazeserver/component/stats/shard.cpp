/*************************************************************************************************/
/*!
    \file   shard.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ShardData

    Encapsulates data specific to one entity type.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/database/dbscheduler.h"
#include "statsslaveimpl.h"
#include "statscommontypes.h"
#include "shard.h"

namespace Blaze
{
namespace Stats
{

ShardData::ShardData()
    : mTableName(nullptr), mNextShard(0), mLruCache(100000, BlazeStlAllocator("ShardData::mLruCache", StatsSlave::COMPONENT_MEMORY_GROUP))
{
}

void ShardData::init(uint32_t dbId, const EA::TDF::ObjectType& objectType)
{
    // Main db id to be used to lookup and assign new entities to shards
    mDbId = dbId;

    // Define the DB table name to be used in the main table to store shard assignments,
    // for each entity type there will be one table called stats_shard_<entityType>
    static const char8_t* SHARD_DATA_TABLE_PREFIX = "stats_shard_";
    size_t length = strlen(SHARD_DATA_TABLE_PREFIX) + objectType.toString('_').length() + 1;
    mTableName = BLAZE_NEW_ARRAY(char8_t, length);
    blaze_snzprintf(mTableName, length, "%s%s", SHARD_DATA_TABLE_PREFIX, objectType.toString('_').c_str());
}

ShardData::~ShardData()
{
    delete[] mTableName;
}
    
BlazeRpcError ShardData::lookupDbId(EntityId entityId, uint32_t& dbId) const
{
    // While the multiple lookup will do a cache check as well, we want to make
    // the fetch from cache case as efficient as possible, thus quickly check here
    if (mLruCache.get(entityId, dbId))
    {
        return ERR_OK;
    }

    // Only if the cache failed do we build up the list/map data structures and
    // defer to the general case lookupEntities code to go to the DB
    EntityIdList entityIdList;
    entityIdList.push_back(entityId);
    ShardEntityMap shardEntityMap;
    BlazeRpcError err = lookupEntities(entityIdList, shardEntityMap);
    if (err != ERR_OK)
        return err;

    // We passed in only a single entity id, and there were no errors, thus there
    // should be one and exactly one entry in the map
    dbId = shardEntityMap.begin()->first;
    return ERR_OK;
}

BlazeRpcError ShardData::lookupEntities(const EntityIdList& entityIds, ShardEntityMap& shardEntityMap) const
{
    EntityIdSet uncachedIds;
    // First try the cache
    for (EntityIdList::const_iterator itr = entityIds.begin(); itr != entityIds.end(); ++itr)
    {
        uint32_t dbId = 0;
        if (mLruCache.get(*itr, dbId))
            shardEntityMap[dbId].push_back(*itr);
        else
            uncachedIds.insert(*itr);
    }

    if (uncachedIds.empty())
        return ERR_OK;

    // Ok, at least some entries not in the cache, go check the DB
    // To prevent deadlock, make sure the read-only DbConnPtr goes out of scope
    // before we request a read-write DbConnPtr later
    {
        DbConnPtr conn = gDbScheduler->getReadConnPtr(mDbId);
        if (conn == nullptr)
        {
            ERR_LOG("[ShardData].lookupEntities: failed to obtain read-only connection");
            return ERR_SYSTEM;
        }

        QueryPtr query = DB_NEW_QUERY_PTR(conn);
        query->append("SELECT * FROM `$s` WHERE entity_id IN (", mTableName);
        for (EntityIdSet::const_iterator itr = uncachedIds.begin(); itr != uncachedIds.end(); ++itr)
        {
            query->append("$D,", *itr);
        }
        query->trim(1);
        query->append(");");

        DbResultPtr dbRes;
        DbError dbErr = conn->executeQuery(query, dbRes);
        if (dbErr != DB_ERR_OK)
        {
            ERR_LOG("[ShardData].lookupEntities: first lookup for uncached entities from table "
                << mTableName << " failed with error " << getDbErrorString(dbErr));
            return ERR_SYSTEM;
        }

        if (!dbRes->empty())
        {
            for (DbResult::const_iterator itr = dbRes->begin(); itr != dbRes->end(); ++itr)
            {
                const DbRow* dbRow = *itr;
                EntityId entityId = dbRow->getInt64("entity_id");
                uint32_t shard = dbRow->getUInt("shard");
                if (shard >= mDbIds.size())
                {
                    // This is a severe problem, if the DB has a value that is out of range,
                    // that effectively means that this entity's stats are broken, and this will
                    // likely be a recurring error until addressed
                    ERR_LOG("[ShardData].lookupDbId: Illegal shard value " << shard
                        << " for entityId " << entityId << " in table " << mTableName);
                    return Blaze::ERR_SYSTEM;
                }
                uint32_t dbId = mDbIds.at(shard);
                shardEntityMap[dbId].push_back(entityId);
                mLruCache.add(entityId, dbId);
                uncachedIds.erase(entityId);
            }
        }

        if (uncachedIds.empty())
            return ERR_OK;
    }

    // At least some entities were not found in the DB, try to insert them now
    DbConnPtr conn = gDbScheduler->getConnPtr(mDbId);
    if (conn == nullptr)
    {
        ERR_LOG("[ShardData].lookupEntities: failed to obtain read-write connection");
        return ERR_SYSTEM;
    }
    QueryPtr query = DB_NEW_QUERY_PTR(conn);

    for (EntityIdSet::const_iterator itr = uncachedIds.begin(); itr != uncachedIds.end(); ++itr)
    {
        query->append("INSERT IGNORE INTO `$s` VALUES ($D, $u);", mTableName, *itr, mNextShard);
        ++mNextShard;
        mNextShard %= mDbIds.size();
    }
    query->append("SELECT * from `$s` WHERE entity_id IN (", mTableName);
    for (EntityIdSet::const_iterator itr = uncachedIds.begin(); itr != uncachedIds.end(); ++itr)
    {
        query->append("$D,", *itr);
    }
    query->trim(1);
    query->append(");");

    DbResultPtrs dbResults;
    DbError dbErr = conn->executeMultiQuery(query, dbResults);
    if (dbErr != DB_ERR_OK)
    {
        ERR_LOG("[ShardData].lookupDbId: second lookup for entityId from table "
            << mTableName << " failed with error " << getDbErrorString(dbErr));
        return Blaze::ERR_SYSTEM;
    }
    DbResultPtr dbRes;
    for (DbResultPtrs::const_iterator dbResultsItr = dbResults.begin(); dbResultsItr != dbResults.end(); ++dbResultsItr)
    {
        dbRes = *dbResultsItr;
        for (DbResult::const_iterator dbItr = dbRes->begin(); dbItr != dbRes->end(); ++dbItr)
        {
            const DbRow* dbRow = *dbItr;
            EntityId entityId = dbRow->getInt64("entity_id");
            uint32_t shard = dbRow->getUInt("shard");
            if (shard >= mDbIds.size())
            {
                // This is a severe problem, if the DB has a value that is out of range,
                // that effectively means that this entity's stats are broken, and this will
                // likely be a recurring error until addressed
                ERR_LOG("[ShardData].lookupDbId: Illegal shard value " << shard
                    << " for entityId " << entityId << " in table " << mTableName);
                return Blaze::ERR_SYSTEM;
            }
            uint32_t dbId = mDbIds.at(shard);
            shardEntityMap[dbId].push_back(entityId);
            mLruCache.add(entityId, dbId);
            uncachedIds.erase(entityId);
        }
    }

    if (uncachedIds.empty())
        return ERR_OK;

    ERR_LOG("[ShardData].lookupEntities: could not find nor insert shard row for some entity(ies) in table " << mTableName);
    return Blaze::ERR_SYSTEM;
}

void ShardData::invalidateEntity(EntityId entityId) const
{
    mLruCache.remove(entityId);
}

ShardConfiguration::ShardConfiguration()
: mMainDbId(DbScheduler::INVALID_DB_ID)
{
}

void ShardConfiguration::init(uint32_t mainDbId)
{
    // Cache the main stats db id, and create a single entry list,
    // that will be returned whenever the caller looks up a non-sharded entity type
    mMainDbId = mainDbId;
    mMainDbIdList.push_back(mMainDbId);
}

const DbIdList& ShardConfiguration::getDbIds(EA::TDF::ObjectType entityType) const
{
    ShardMap::const_iterator itr = mShardMap.find(entityType);
    if (itr == mShardMap.end())
    {
        return mMainDbIdList;
    }
    return itr->second.getDbIds();
}

BlazeRpcError ShardConfiguration::lookupDbId(EA::TDF::ObjectType entityType, EntityId entityId, uint32_t&  dbId) const
{
    ShardMap::const_iterator itr = mShardMap.find(entityType);
    if (itr == mShardMap.end())
    {
        dbId = mMainDbId;
        return ERR_OK;
    }
    return itr->second.lookupDbId(entityId, dbId);
}

BlazeRpcError ShardConfiguration::lookupDbIds(EA::TDF::ObjectType entityType, const EntityIdList& list, ShardEntityMap& shardEntityMap) const
{
    ShardMap::const_iterator itr = mShardMap.find(entityType);
    if (itr == mShardMap.end())
    {
        shardEntityMap[mMainDbId].assign(list.begin(), list.end());
        return ERR_OK;
    }
    return itr->second.lookupEntities(list, shardEntityMap);
}

void ShardConfiguration::invalidateEntity(EA::TDF::ObjectType entityType, EntityId entityId) const
{
    ShardMap::const_iterator itr = mShardMap.find(entityType);
    if (itr != mShardMap.end())
        itr->second.invalidateEntity(entityId);
}

BlazeRpcError StatsDbConnectionManager::acquireConnWithTxn(uint32_t dbId, DbConnPtr& conn, QueryPtr& query)
{
    StatsConnTrackerMap::const_iterator iter = mTrackerMap.find(dbId);
    if (iter == mTrackerMap.end())
    {
        conn = gDbScheduler->getConnPtr(dbId);
        if (conn == nullptr)
        {
            ERR_LOG("[StatsDbConnectionManager].acquireConnWithTxn: failed to obtain connection");        
            return ERR_SYSTEM;
        }

        conn->clearOwningFiber();
        BlazeRpcError startTxnError = conn->startTxn();
        if (startTxnError != DB_ERR_OK)
        {
            ERR_LOG("[StatsDbConnectionManager].acquiredConnWithTxn: failed to start transaction (" << SbFormats::HexLower(startTxnError) << ")");
            return ERR_SYSTEM;
        }

        query = DB_NEW_QUERY_PTR(conn);
        query->clearOwningFiber();
        StatsConnTracker& tracker = mTrackerMap[dbId];
        tracker.mConn = conn;
        tracker.mQuery = query;
    }
    else
    {
        StatsConnTracker& tracker = mTrackerMap[dbId];
        conn = tracker.mConn;
        query = tracker.mQuery;
    }
    return ERR_OK;
}

QueryPtr StatsDbConnectionManager::getQuery(uint32_t dbId)
{
    return mTrackerMap.find(dbId)->second.mQuery;
}

BlazeRpcError StatsDbConnectionManager::execute(bool trackResults)
{
    BlazeRpcError err = ERR_OK;

    StatsConnTrackerMap::iterator itr = mTrackerMap.begin();
    StatsConnTrackerMap::iterator end = mTrackerMap.end();
    for (; itr != end; ++itr)
    {
        StatsConnTracker& tracker = itr->second;
        if (!tracker.mQuery->isEmpty())
        {
            if (trackResults)
                err = tracker.mConn->executeMultiQuery(tracker.mQuery, tracker.mResultPtrs);
            else
                err = tracker.mConn->executeMultiQuery(tracker.mQuery);
            if (err != DB_ERR_OK)
            {
                if (err == ERR_DB_DUP_ENTRY || err == ERR_DB_LOCK_DEADLOCK)
                {
                    // Don't print an ERR log here - these are recoverable errors that may occur in valid cases 
                    // (e.g. multiple stats requests coming in one after another, trying to update the same rows).
                    INFO_LOG("[StatsDbConnectionManager].execute(): recoverable error processing db call: " << getDbErrorString(err));
                }
                else
                {
                    ERR_LOG("[StatsDbConnectionManager].execute(): error processing db call: " << getDbErrorString(err));
                }
                break;
            }
            DB_QUERY_RESET(tracker.mQuery);
            if (trackResults)
                tracker.mItr = tracker.mResultPtrs.begin();
        }
    }
    return err;
}

void StatsDbConnectionManager::resetQueries()
{
    StatsConnTrackerMap::iterator itr = mTrackerMap.begin();
    StatsConnTrackerMap::iterator end = mTrackerMap.end();
    for (; itr != end; ++itr)
        DB_QUERY_RESET(itr->second.mQuery);
}

BlazeRpcError StatsDbConnectionManager::commit()
{
    BlazeRpcError err = ERR_OK;

    // First try to execute all queries
    StatsConnTrackerMap::iterator itr = mTrackerMap.begin();
    StatsConnTrackerMap::iterator end = mTrackerMap.end();
    for (; itr != end; ++itr)
    {
        StatsConnTracker& tracker = itr->second;
        if (!tracker.mQuery->isEmpty())
        {
            err = tracker.mConn->executeMultiQuery(tracker.mQuery);
            if (err != DB_ERR_OK)
            {
                ERR_LOG("[StatsDbConnectionManager].commit(): error processing db call: " << getDbErrorString(err));
                break;
            }
        }
    }

    // If any failures in execution, try to roll back all open transactions
    if (err != ERR_OK)
    {
        for (itr = mTrackerMap.begin(); itr != end; ++itr)
        {
            StatsConnTracker& tracker = itr->second;
            if (tracker.mConn->isTxnInProgress())
            {
                tracker.mConn->rollback();
            }
        }
        return err;
    }

    // Done executing all queries successfully at this point, now we have an unfortunately non-atomic commit,
    // if there is more than one DB involved here we will try to commit to all of them, if something breaks
    // part way through there is not much we can do about that
    for (itr = mTrackerMap.begin(); itr != end; ++itr)
    {
        StatsConnTracker& tracker = itr->second;
        if (tracker.mConn->isTxnInProgress())
        {
            DbError dbErr = tracker.mConn->commit();
            if (dbErr != DB_ERR_OK)
            {
                // Log and return an error, but do not break, we will continue on committing as many transactions
                // as we can, the rationale being if some succeed we want to update as many users' data as possible,
                // errors on commits will hopefully be exceeding rare
                ERR_LOG("[StatsDbConnectionManager].commit(): error committing DB transaction: " << getDbErrorString(dbErr));
                err = ERR_SYSTEM;
            }
        }
    }

    // Failures here are bad, we attempted to commit as many transactions as we could above and now rollback any failures,
    // but this does mean the DB may be in a somewhat inconsistent state, that is the price to pay for sharding
    if (err != ERR_OK)
    {
        for (itr = mTrackerMap.begin(); itr != end; ++itr)
        {
            StatsConnTracker& tracker = itr->second;
            if (tracker.mConn->isTxnInProgress())
            {
                tracker.mConn->rollback();
            }
        }
    }

    return err;
}

BlazeRpcError StatsDbConnectionManager::rollback()
{
    StatsConnTrackerMap::iterator itr = mTrackerMap.begin();
    StatsConnTrackerMap::iterator end = mTrackerMap.end();
    for (; itr != end; ++itr)
    {
        StatsConnTracker& tracker = itr->second;
        if (tracker.mConn->isTxnInProgress())
        {
            tracker.mConn->rollback();
        }
    }
    return ERR_OK;
}

DbResultPtrs::const_iterator StatsDbConnectionManager::getNextResultItr(uint32_t dbId)
{
    StatsConnTracker& tracker = mTrackerMap.find(dbId)->second;
    if (tracker.mItr == tracker.mResultPtrs.cend())
        return tracker.mItr;

    return tracker.mItr++;
}

DbResultPtrs::const_iterator StatsDbConnectionManager::getEndResultItr(uint32_t dbId)
{
    return mTrackerMap.find(dbId)->second.mResultPtrs.cend();
}

} // Stats
} // Blaze
