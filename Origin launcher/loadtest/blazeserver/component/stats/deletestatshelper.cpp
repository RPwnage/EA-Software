/*************************************************************************************************/
/*!
    \file   deletestatshelper.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class DeleteStatsHelper

    Analogous to the updateStatsHelper, this class does the detailed work required in some
    circumstances when deleting stats.  Some delete stats requests may specify a subset of all
    keyscoped data to delete, and in that case it is not merely a matter of deleting rows, we
    must also fetch some aggregate rows and update them.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/database/query.h"
#include "framework/util/expression.h"

#include "EASTL/functional.h"
#include "EASTL/sort.h"

#include "stats/userstats.h"
#include "stats/userranks.h"
#include "stats/deletestatshelper.h"
#include "stats/statscache.h"
#include "stats/leaderboardindex.h"
#include "stats/statsextendeddataprovider.h"

namespace Blaze
{
namespace Stats
{

// The DataMap is used to organize all the data fetched from the DB, both aggregate rows
// that may need to be updated, as well as stat data which is about to deleted but first
// needs to be subtracted from the aggregate rows
typedef struct DataKey
{
    DataKey() {}
    DataKey(int32_t periodType, int32_t periodId, ScopeNameValueMap* scopeNameValueMap);
    int32_t periodType;
    int32_t periodId;
    EA::TDF::tdf_ptr<ScopeNameValueMap> scopeNameValueMap;
    bool operator < (const DataKey& other) const;
} DataKey;

typedef eastl::vector_map<DataKey, StatValMap> DataMap;

DataKey::DataKey(int32_t _periodType, int32_t _periodId, ScopeNameValueMap* _scopeNameValueMap)
    : periodType(_periodType), periodId(_periodId), scopeNameValueMap(_scopeNameValueMap)
{
}

bool DataKey::operator < (const DataKey& other) const
{
    if (periodType != other.periodType) 
        return (periodType < other.periodType);

    if (periodId != other.periodId)
        return (periodId < other.periodId);

    const size_t sizeA = (scopeNameValueMap != nullptr       ? scopeNameValueMap->size() : 0);
    const size_t sizeB = (other.scopeNameValueMap != nullptr ? other.scopeNameValueMap->size() : 0);
    if (sizeA != sizeB)
        return (sizeA < sizeB); /* our map size < his map size*/

    if (scopeNameValueMap != nullptr && other.scopeNameValueMap != nullptr)
    {
        // this means maps are the same size
        ScopeNameValueMap::const_iterator itA = scopeNameValueMap->begin();
        ScopeNameValueMap::const_iterator itAEnd = scopeNameValueMap->end();
        ScopeNameValueMap::const_iterator itB = other.scopeNameValueMap->begin();
        for(; itA != itAEnd; ++itA, ++itB)
        {
            if (itA->first != itB->first)
                return (itA->first < itB->first); /* our value < his value first */
            if (itA->second != itB->second)
                return (itA->second < itB->second); /* our value < his value first */
        }
    }

    // Otherwise we are equal, and thus not less than
    return false;
}

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief DeleteStatsHelper

    Constructor for the DeleteStatsHelper wrapper that holds the data structures and acts as a 
    state machine required for performing database actions including fetch, apply derived changes 
    and commit.

    \param[in] statsComp - handle to the stats component
*/
/*************************************************************************************************/
DeleteStatsHelper::DeleteStatsHelper(StatsSlaveImpl* statsComp)
    : mStatsComp(statsComp)
{
    mNotification.getBroadcast().switchActiveMember(StatBroadcast::MEMBER_STATDELETEANDUPDATE);
    mBroadcast = mNotification.getBroadcast().getStatDeleteAndUpdate();

    if ( statsComp != nullptr)
    {
        statsComp->cachePeriodIds(mBroadcast->getCurrentPeriodIds());
    }
}

/*************************************************************************************************/
/*!
    \brief ~DeleteStatsHelper

    Destructor for the DeleteStatsHelper.
*/
/*************************************************************************************************/
DeleteStatsHelper::~DeleteStatsHelper()
{
    mDerivedStrings.clear();
}

BlazeRpcError DeleteStatsHelper::broadcastDeleteAndUpdate()
{
    BlazeRpcError err = ERR_OK;

    if ( mStatsComp == nullptr )
        return ERR_SYSTEM;

    if ( !mBroadcast->getCacheDeletes().empty() || !mBroadcast->getCacheUpdates().empty() )
    {
        mStatsComp->mLeaderboardIndexes->deleteStats(mBroadcast->getCacheDeletes(), mBroadcast->getCurrentPeriodIds(), &mLbQueryString);

        if (!mLbQueryString.isEmpty())
        {
            DbConnPtr lbConn = gDbScheduler->getConnPtr(mStatsComp->getDbId());
            if (lbConn == nullptr)
            {
                ERR_LOG("[DeleteStatsHeler].broadcastDeleteAndUpdate: failed to obtain connection");
                return ERR_SYSTEM;
            }

            QueryPtr lbQuery = DB_NEW_QUERY_PTR(lbConn);
            lbQuery->append("$s", mLbQueryString.get());

            DbError lbError = lbConn->executeMultiQuery(lbQuery);
            if (lbError != DB_ERR_OK)
            {
                WARN_LOG("[DeleteStatsHelper].broadcastDeleteAndUpdate: failed to update LB cache tables");
            }
        }

        mUedRequest.setComponentId(StatsSlave::COMPONENT_ID);
        mUedRequest.setIdsAreSessions(false);

        // Tell master and remote slaves to update their cache
        if (mStatsComp->getStatsCache().isLruEnabled())
        {
            mStatsComp->getStatsCache().deleteRows(mBroadcast->getCacheUpdates());
            mStatsComp->getStatsCache().deleteRows(mBroadcast->getCacheDeletes(), mBroadcast->getCurrentPeriodIds());
        }
        mStatsComp->sendUpdateCacheNotificationToRemoteSlaves(&mNotification);

        mStatsComp->mStatsExtendedDataProvider->getUserRanks().updateUserRanks(mUedRequest, mBroadcast->getCacheUpdates());
        mStatsComp->mStatsExtendedDataProvider->getUserRanks().updateUserRanks(mUedRequest, mBroadcast->getCacheDeletes());

        BlazeRpcError uedErr = Blaze::ERR_OK;
        uedErr = gUserSessionManager->updateExtendedData(mUedRequest);
        if (uedErr != Blaze::ERR_OK)
        {
            WARN_LOG("[DeleteStatsHelper].broadcastDeleteAndUpdate: " << (ErrorHelp::getErrorName(uedErr)) << " error updating user extended data");
        }
    }

    return err;
}

/*************************************************************************************************/
/*!
    \brief execute

    The main method that does the work of deleting stats and potentially updating aggregate rows.

    \param[in] statDelete the deletion request
    \param[in] dbConn the db connection to use
    \param[out] query the query object to fill in
 
    \return - corresponding database error code
*/
/*************************************************************************************************/
DbError DeleteStatsHelper::execute(const StatDelete* statDelete, DbConnPtr& dbConn, QueryPtr& query)
{
    if (mStatsComp == nullptr)
        return ERR_SYSTEM;

    // Grab the current period ids and remember them as there is a minute chance rollover could
    // occur during a trip to the database
    mStatsConfig = mStatsComp->getConfigData();
    mStatsComp->getPeriodIds(mPeriodIds);

    const StatCategory* cat = mStatsConfig->getCategoryMap()->find(statDelete->getCategory())->second;

    // If specified, use the provided period types from the request (it is assumed these have
    // already been validated against the category before this method was invoked), otherwise
    // use all valid period types for the category
    PeriodTypeList periodTypes;
    if (!statDelete->getPeriodTypes().empty())
    {
        statDelete->getPeriodTypes().copyInto(periodTypes);
    }
    else
    {
        int32_t catPeriodTypes = cat->getPeriodTypes();
        periodTypes.reserve(STAT_NUM_PERIODS);
        for (int32_t period = 0; period < STAT_NUM_PERIODS; ++period)
            if ((catPeriodTypes & (1 << period)) != 0)
                periodTypes.push_back(period);
    }

    // Check if there are no keyscopes amongst those specified in the request that have aggregates
    // and if that is the case, we can do a much simpler delete only call with no need to even
    // fetch any aggregate rows for update
    bool updateRequired = false;
    ScopeNameValueMap::const_iterator itScope = statDelete->getKeyScopeNameValueMap().begin();
    ScopeNameValueMap::const_iterator itendScope = statDelete->getKeyScopeNameValueMap().end();
    for (; itScope != itendScope; ++itScope)
    {
        ScopeValue scopeValue = mStatsConfig->getAggregateKeyValue(itScope->first.c_str());
        if (scopeValue >= 0)
        {
            updateRequired = true;
            break;
        }
    }

    for (PeriodTypeList::const_iterator pit = periodTypes.begin(); pit != periodTypes.end(); ++pit)
    {
        mStatsComp->mStatsExtendedDataProvider->getUserStats().deleteUserStats(
            mUedRequest, statDelete->getEntityId(), cat->getId(), *pit, statDelete->getKeyScopeNameValueMap());
    }

    if (!updateRequired)
    {
        appendDeleteStatement(statDelete, periodTypes, cat, query);

        // need to broadcast for any stats-level caching
        CacheRowDelete* cacheRowDelete = mBroadcast->getCacheDeletes().pull_back();
        cacheRowDelete->setCategoryId(cat->getId());
        cacheRowDelete->setEntityId(statDelete->getEntityId());
        periodTypes.copyInto(cacheRowDelete->getPeriodTypes());
        statDelete->getPerOffsets().copyInto(cacheRowDelete->getPerOffsets());
        statDelete->getKeyScopeNameValueMap().copyInto(cacheRowDelete->getKeyScopeNameValueMap());

        return ERR_OK;
    }

    // Ok, now we have to do more work and actually fetch some rows and do some math
    QueryPtr getStatsQuery = DB_NEW_QUERY_PTR(dbConn);
    for (PeriodTypeList::const_iterator iter = periodTypes.begin(); iter != periodTypes.end(); ++iter)
    {
        int32_t periodType = *iter;

        getStatsQuery->append("SELECT * FROM `$s` WHERE `entity_id` = $D", cat->getTableName(periodType), statDelete->getEntityId());

        if (periodType != STAT_PERIOD_ALL_TIME)
        {
            int32_t periodId = mPeriodIds[periodType];

            PeriodIdMap::const_iterator cpi = mBroadcast->getCurrentPeriodIds().find(periodType);
            if (cpi != mBroadcast->getCurrentPeriodIds().end())
            {
                periodId = cpi->second;
            }

            // If no period offsets are supplied, that means ALL, otherwise request specific ones
            if (!statDelete->getPerOffsets().empty())
            {
                getStatsQuery->append(" AND `period_id` IN (");
                for (OffsetList::const_iterator poIter = statDelete->getPerOffsets().begin(); poIter != statDelete->getPerOffsets().end(); ++poIter)
                    getStatsQuery->append("$d,", periodId - *poIter);
                getStatsQuery->trim(1);
                getStatsQuery->append(")");
            }
        }

        ScopeNameValueMap::const_iterator itScopeTemp = statDelete->getKeyScopeNameValueMap().begin();
        ScopeNameValueMap::const_iterator itScopeTempEnd = statDelete->getKeyScopeNameValueMap().end();
        for (; itScopeTemp != itScopeTempEnd; ++itScopeTemp)
        {
            ScopeValue scopeValue = mStatsConfig->getAggregateKeyValue(itScopeTemp->first.c_str());
            if (scopeValue >= 0)
                getStatsQuery->append(" AND `$s` IN ($D, $D)", itScopeTemp->first.c_str(), itScopeTemp->second, scopeValue);
            else
                getStatsQuery->append(" AND `$s` = $D", itScopeTemp->first.c_str(), itScopeTemp->second);
        }
        getStatsQuery->append(" FOR UPDATE;\n");
    }

    TRACE_LOG("[DeleteStatsHelper].execute(): prepared select query");

    // Now begins the second main part of deleting stats - executing the fetch query and validating the results
    DbResultPtrs getStatsResults;
    DbError getStatsError = dbConn->executeMultiQuery(getStatsQuery, getStatsResults);
    if (getStatsError != DB_ERR_OK)
    {
        ERR_LOG("[DeleteStatsHelper].execute(): select query failed: " << getDbErrorString(getStatsError) << ".");
        return getStatsError;
    }

    TRACE_LOG("[DeleteStatsHelper].execute(): Received " << getStatsResults.size() << " results back");

    if (periodTypes.size() != getStatsResults.size())
    {
        ERR_LOG("[DeleteStatsHelper].execute(): did not receive the expected number of results");
        return DB_ERR_SYSTEM;
    }

    // Third major step, parse out the db results into some data structures
    DataMap dataMap;
    PeriodTypeList::const_iterator periodTypeIter = periodTypes.begin();
    for (DbResultPtrs::const_iterator dbi = getStatsResults.begin(); dbi != getStatsResults.end(); ++dbi)
    {
        int32_t periodType = *periodTypeIter++;
        const DbResultPtr dbResult = *dbi;
        for (DbResult::const_iterator resIter = dbResult->begin(); resIter != dbResult->end(); ++resIter)
        {
            const DbRow* dbRow = *resIter;

            ScopeNameValueMap* scopeNameValueMap = BLAZE_NEW ScopeNameValueMap();
            // we have checked above that a keyscope aggregate update is required; so this
            // category does have keyscopes, and cat->getScopeNameSet() can't (shouldn't) be nullptr
            for (ScopeNameSet::const_iterator scopeIter = cat->getScopeNameSet()->begin();
                scopeIter != cat->getScopeNameSet()->end(); ++scopeIter)
            {
               (*scopeNameValueMap)[*scopeIter] = dbRow->getInt64(*scopeIter);
            }
            
            DataKey key(periodType, dbRow->getInt("period_id"), scopeNameValueMap);
            StatValMap& statValMap = dataMap[key];
            for (StatMap::const_iterator statIter = cat->getStatsMap()->begin(); statIter != cat->getStatsMap()->end(); ++statIter)
            {
                const Stat* stat = statIter->second;
                StatValMap::iterator statValIter = statValMap.find(stat->getName());

                if (statValIter == statValMap.end())
                {
                    StatVal* val = BLAZE_NEW StatVal();
                    val->type = stat->getDbType().getType();
                    val->changed = true;

                    if (stat->getDbType().getType() == STAT_TYPE_INT)
                        val->intVal = dbRow->getInt64(stat->getName());
                    else if (stat->getDbType().getType() == STAT_TYPE_FLOAT)
                        val->floatVal = dbRow->getFloat(stat->getName());
                    else if (stat->getDbType().getType() == STAT_TYPE_STRING)
                        val->stringVal = dbRow->getString(stat->getName());
                    statValMap[stat->getName()] = val;
                }
            }
        }
    }

    // Fourth step, iterate through all the results, for any aggregate rows that need to be updated (and not simply deleted),
    // we need to subtract out the underlying rows being deleted
    for (DataMap::iterator iter = dataMap.begin(); iter != dataMap.end(); ++iter)
    {
        const DataKey& key = iter->first;
        const ScopeNameValueMap* scopes = key.scopeNameValueMap;

        // Look for rows that contain at least one aggregate keyscope, but not just any keyscope, it must be a keyscope
        // that is specified in the request.  If the only keyscopes that are aggregate for this row are not specified
        // in the request, then this row will be deleted, so no need to update it
        updateRequired = false;
        for (ScopeNameValueMap::const_iterator scopeIter = scopes->begin(); scopeIter != scopes->end(); ++scopeIter)
        {
            if (statDelete->getKeyScopeNameValueMap().find(scopeIter->first) == statDelete->getKeyScopeNameValueMap().end())
                continue;

            ScopeValue aggrValue = mStatsConfig->getAggregateKeyValue(scopeIter->first.c_str());
            if (aggrValue == scopeIter->second)
            {
                updateRequired = true;
                break;
            }
        }

        if (!updateRequired)
            continue;

        // Build up a keyscope value map for the underlying base row that we will need to locate and subtract out
        // from the current row we are processing, we generate this keyscope map by taking the values from the incoming
        // request where specified, and the rest come from the current row we are processing
        ScopeNameValueMap baseMap;
        for (ScopeNameValueMap::const_iterator scopeIter = scopes->begin(); scopeIter != scopes->end(); ++scopeIter)
        {
            ScopeNameValueMap::const_iterator reqIter = statDelete->getKeyScopeNameValueMap().find(scopeIter->first);
            if (reqIter != statDelete->getKeyScopeNameValueMap().end())
                baseMap[scopeIter->first] = reqIter->second;
            else
                baseMap[scopeIter->first] = scopeIter->second;
        }

        // The stats for the current row we are processing that need to be updated
        StatValMap& statValMap = iter->second;

        // Create a key to locate the underlying base row and find it
        DataKey baseKey(key.periodType, key.periodId, &baseMap);
        DataMap::iterator baseIter = dataMap.find(baseKey);
        if (baseIter != dataMap.end())
        {
            // Do the math to subtract out the value of each stat from the row being deleted,
            // note that aggregation really doesn't make any sense for string stats, so we take no action there
            StatValMap& baseMapTemp = baseIter->second;
            for (StatMap::const_iterator statIter = cat->getStatsMap()->begin(); statIter != cat->getStatsMap()->end(); ++statIter)
            {
                const Stat* stat = statIter->second;
                if (stat->isDerived())
                    continue;

                if (stat->getDbType().getType() == STAT_TYPE_INT)
                {
                    statValMap[stat->getName()]->intVal -= baseMapTemp[stat->getName()]->intVal;
                }
                else if (stat->getDbType().getType() == STAT_TYPE_FLOAT)
                {
                    statValMap[stat->getName()]->floatVal -= baseMapTemp[stat->getName()]->floatVal;
                }
                else
                {
                    // Do nothing for strings
                }
            }

            // Now do the derived stats, note that because we are deleting some rows, we effectively need to update every single non-derived stat,
            // and because we are also limited to non-multi-category derived stats, it makes things nice and simple - no need to check for whether
            // dependencies have changed, or pull in multiple rows, we simply evaluate every derived formula in order
            const CategoryDependency* catDep = cat->getCategoryDependency();
            for (CategoryStatList::const_iterator derivedIter = catDep->catStatList.begin(); derivedIter != catDep->catStatList.end(); ++derivedIter)
            {
                const CategoryStat& catStat = *derivedIter;
                const Stat* stat = cat->getStatsMap()->find(catStat.statName)->second;

                DeleteResolveContainer container;
                container.statValMap = &statValMap;

                const Blaze::Expression* derivedExpression = stat->getDerivedExpression();
                StatVal* statVal = statValMap.find(stat->getName())->second;

                Blaze::Expression::ResolveVariableCb cb(&resolveStatValueForDelete);

                if (stat->getDbType().getType() == STAT_TYPE_INT)
                {
                    int64_t val = derivedExpression->evalInt(cb, static_cast<void*>(&container));
                    statVal->intVal = val;
                }
                else if (stat->getDbType().getType() == STAT_TYPE_FLOAT)
                {
                    float_t val = derivedExpression->evalFloat(cb, static_cast<void*>(&container));
                    statVal->floatVal = val;
                }
                else if (stat->getDbType().getType() == STAT_TYPE_STRING)
                {
                    StringStatValue& stringStatValue = mDerivedStrings.push_back();

                    derivedExpression->evalString(cb, static_cast<void*>(&container),
                        stringStatValue.buf, sizeof(stringStatValue.buf));
                    statVal->stringVal = stringStatValue.buf;
                }
            }
        }

        // Build up the update stats query statement now that we have finished computing what needs to change
        query->append("UPDATE `$s` SET `last_modified`=NULL", cat->getTableName(key.periodType));
        for (StatValMap::const_iterator valIter = statValMap.begin(); valIter != statValMap.end(); ++valIter)
        {
            StatMap::const_iterator statIter = cat->getStatsMap()->find(valIter->first);
            int32_t type = statIter->second->getDbType().getType();
            query->append(", `$s` = ", valIter->first);

            // Note even though we do not modify any string stats as a result of aggregation,
            // we still may update some as a result of derived stats, thus it is important to
            // remember to write them out to the DB
            switch (type)
            {
                case STAT_TYPE_INT:
                    query->append("$D", valIter->second->intVal);
                    break;
                case STAT_TYPE_FLOAT:
                    query->append("$f", valIter->second->floatVal);
                    break;
                case STAT_TYPE_STRING:
                    query->append("'$s'", valIter->second->stringVal);
                    break;
                default:
                    WARN_LOG("[DeleteStatsHelper].execute(): Invalid stat type(" << type << ") specified.");
                    return DB_ERR_SYSTEM;
            }
        }
        query->append(" WHERE `entity_id` = $D", statDelete->getEntityId());
        if (key.periodType != STAT_PERIOD_ALL_TIME)
            query->append(" AND period_id = $d", key.periodId);
        for (ScopeNameValueMap::const_iterator scopeIter = key.scopeNameValueMap->begin(); scopeIter != key.scopeNameValueMap->end(); ++scopeIter)
            query->append(" AND `$s` = $D", scopeIter->first.c_str(), scopeIter->second);
        query->append(";\n");

        CacheRowUpdate* cacheRowUpdate = BLAZE_NEW CacheRowUpdate();
        char8_t keyScopeString[128];
        LeaderboardIndexes::buildKeyScopeString(keyScopeString, sizeof(keyScopeString), *(key.scopeNameValueMap));
        mStatsComp->mLeaderboardIndexes->updateStats(cat->getId(), statDelete->getEntityId(), key.periodType, key.periodId, &statValMap, key.scopeNameValueMap, keyScopeString, mLbQueryString, cacheRowUpdate->getStatUpdates());
        if (!cacheRowUpdate->getStatUpdates().empty() || mStatsComp->getStatsCache().isLruEnabled())
        {
            // need to send at least the category and entity info to invalidate any LRU cache
            LeaderboardIndexes::populateCacheRow(*cacheRowUpdate, cat->getId(), statDelete->getEntityId(), key.periodType, key.periodId, keyScopeString);
            mBroadcast->getCacheUpdates().push_back(cacheRowUpdate);
        }
        else
            delete cacheRowUpdate;

        if (key.periodType == STAT_PERIOD_ALL_TIME || mPeriodIds[key.periodType] == key.periodId)
        {
            mStatsComp->mStatsExtendedDataProvider->getUserStats().updateUserStats(mUedRequest, statDelete->getEntityId(), 
                cat->getId(), key.periodType, statDelete->getKeyScopeNameValueMap(), statValMap);
        }
    }

    // Free memory from the scope maps we had to allocate for each data map entry
    for (DataMap::const_iterator dataIter = dataMap.begin(); dataIter != dataMap.end(); ++dataIter)
    {
        const StatValMap& statValMap = dataIter->second;
        for (StatValMap::const_iterator statValIter = statValMap.begin(); statValIter != statValMap.end(); ++statValIter)
        {
            delete statValIter->second;
        }
    }

    // To this point we have only built up statements for rows we are updating, now tack on the additional SQL
    // for the rows we are deleting
    appendDeleteStatement(statDelete, periodTypes, cat, query);

    // need to broadcast for any stats-level caching
    CacheRowDelete* cacheRowDelete = mBroadcast->getCacheDeletes().pull_back();
    cacheRowDelete->setCategoryId(cat->getId());
    cacheRowDelete->setEntityId(statDelete->getEntityId());
    periodTypes.copyInto(cacheRowDelete->getPeriodTypes());
    statDelete->getPerOffsets().copyInto(cacheRowDelete->getPerOffsets());
    statDelete->getKeyScopeNameValueMap().copyInto(cacheRowDelete->getKeyScopeNameValueMap());

    return DB_ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief appendDeleteStatement

    A helper method to generate the delete query statement(s) for a given stat delete request.

    \param[in]  statDelete - the original request object
    \param[in]  periodTypes - the period types that need to be deleted from
    \param[in]  cat - the category to be deleted from
    \param[out] query - the query object to append to
*/
/*************************************************************************************************/
void DeleteStatsHelper::appendDeleteStatement(const StatDelete* statDelete,
    const PeriodTypeList& periodTypes, const StatCategory* cat, QueryPtr& query)
{
    for (PeriodTypeList::const_iterator iter = periodTypes.begin(); iter != periodTypes.end(); ++iter)
    {
        int32_t periodType = *iter;

        query->append("DELETE FROM `$s` WHERE `entity_id` = $D", cat->getTableName(periodType), statDelete->getEntityId());

        if (periodType != STAT_PERIOD_ALL_TIME)
        {
            int32_t periodId = mPeriodIds[periodType];

            PeriodIdMap::const_iterator cpi = mBroadcast->getCurrentPeriodIds().find(periodType);
            if (cpi != mBroadcast->getCurrentPeriodIds().end())
            {
                periodId = cpi->second;
            }

            // If no period offsets are supplied, that means ALL, otherwise request specific ones
            if (!statDelete->getPerOffsets().empty())
            {
                query->append(" AND `period_id` IN (");
                for (OffsetList::const_iterator poIter = statDelete->getPerOffsets().begin(); poIter != statDelete->getPerOffsets().end(); ++poIter)
                    query->append("$d,", periodId - *poIter);
                query->trim(1);
                query->append(")");
            }
        }

        ScopeNameValueMap::const_iterator itScope = statDelete->getKeyScopeNameValueMap().begin();
        ScopeNameValueMap::const_iterator itendScope = statDelete->getKeyScopeNameValueMap().end();
        for (; itScope != itendScope; ++itScope)
        {
            query->append(" AND `$s` = $D", itScope->first.c_str(), itScope->second);
        }
        query->append(";\n");
    }
}

/*************************************************************************************************/
/*!
    \brief resolveStatValueForDelete

    Implements the ResolveVariableCb needed by the Blaze expression framework to look up the values
    of named variables.  This method is used for computation of derived values specifically in the
    case where we need to update aggregate keyscope rows.  It is a much simpler case than the more
    complex update stats case, because we do not have to worry about multi-category derived stats.

    \param[in]  nameSpace - the name of the category to lookup
    \param[in]  name - the name of the stat to lookup
    \param[in]  type - the data type the variable is expected to have
    \param[in]  context - opaque reference to what we know is a map of stat names to values
    \param[out] val - value to be filled in by this method
*/
/*************************************************************************************************/
void resolveStatValueForDelete(const char8_t* nameSpace, const char8_t* name, Blaze::ExpressionValueType type,
    const void* context, Blaze::Expression::ExpressionVariableVal& val)
{
    const DeleteResolveContainer* container = static_cast<const DeleteResolveContainer*>(context);
    StatVal* statVal = container->statValMap->find(name)->second;

    switch (type)
    {
        case Blaze::EXPRESSION_TYPE_INT:
            val.intVal = statVal->intVal;
            break;
        case Blaze::EXPRESSION_TYPE_FLOAT:
            val.floatVal = statVal->floatVal;
            break;
        case Blaze::EXPRESSION_TYPE_STRING:
            val.stringVal = statVal->stringVal;
            break;
        default:
            break;
    }
}

} // Stats
} // Blaze
