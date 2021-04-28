/*************************************************************************************************/
/*!
    \file   deletestats.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class DeleteStats

    Delete stats.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"

#include "framework/userset/userset.h"

#include "stats/statsconfig.h"
#include "stats/statsslaveimpl.h"
#include "stats/deletestatshelper.h"
#include "stats/userstats.h"
#include "stats/userranks.h"
#include "stats/leaderboardindex.h"
#include "stats/statscache.h"
#include "stats/statsextendeddataprovider.h"

namespace Blaze
{
namespace Stats
{

void StatsSlaveImpl::cachePeriodIds(PeriodIdMap& currentPeriodIds)
{
    // get the current period ids in case we get a rollover while executing the query or during broadcast
    for (int32_t periodType = 0; periodType < STAT_NUM_PERIODS; ++periodType)
    {
        currentPeriodIds[periodType] = getPeriodId(periodType);
    }
}

/*************************************************************************************************/
/*!
    \brief validateStatDelete

    Private helper to validate delete requests.

    \param[in]  categoryName - name of the category to delete from
    \param[in]  checkDependency - whether to check the category for multi-category derived stats dependency
    \param[in]  scopeNameValueMap - keyscope name/value map to delete from
    \param[in]  periodTypeList - period types to delete from

    \return - ERR_OK or error
*/
/*************************************************************************************************/
BlazeRpcError StatsSlaveImpl::validateStatDelete(const char8_t* categoryName, bool checkDependency,
    const ScopeNameValueMap& scopeNameValueMap, const PeriodTypeList& periodTypeList) const
{
    CategoryMap::const_iterator catIter = getConfigData()->getCategoryMap()->find(categoryName);
    if (catIter == getConfigData()->getCategoryMap()->end())
    {
        ERR_LOG("[StatsSlaveImpl].validateStatDelete: unknown category " << categoryName);
        return STATS_ERR_UNKNOWN_CATEGORY;
    }
    const StatCategory* cat = catIter->second;

    //
    // TODO: Move wipe stats to stat provider
    //
    if (cat->getCategoryType() == CATEGORY_TYPE_GLOBAL)
        return STATS_ERR_INVALID_OPERATION;

    if (checkDependency)
    {
        // We do not allow deletion from categories that are involved in multi-category derived stats,
        // this would cause strange inconsistencies because it does not make sense to remove some but
        // not all rows from the DB in such a case.  At some point we may wish to provide an option to
        // cascade the delete to all related categories.
        if (cat->getCategoryDependency()->catSet.size() > 1)
        {
            ERR_LOG("[StatsSlaveImpl].validateStatDelete: Not allowed because category "
                << categoryName << " is involved in a multi-category derived stats relationship.");
            return STATS_ERR_INVALID_OPERATION;
        }
    }

    ScopeNameValueMap::const_iterator keyScopeItr = scopeNameValueMap.begin();
    ScopeNameValueMap::const_iterator keyScopeEnd = scopeNameValueMap.end();
    for (; keyScopeItr != keyScopeEnd; ++keyScopeItr)
    {
        ScopeName scopeName = keyScopeItr->first;
        if (!cat->isValidScopeName(scopeName.c_str()))
        {
            ERR_LOG("[StatsSlaveImpl].validateStatDelete: Invalid keyscope "
                << scopeName.c_str() << " for category " << categoryName);
            return STATS_ERR_BAD_SCOPE_INFO;
        }
        if (!getConfigData()->isValidScopeValue(scopeName.c_str(), keyScopeItr->second))
        {
            ERR_LOG("[StatsSlaveImpl].validateStatDelete: Invalid value "
                << keyScopeItr->second << " for keyscope " << scopeName.c_str());
            return STATS_ERR_BAD_SCOPE_INFO;
        }
        if (getConfigData()->getAggregateKeyValue(scopeName.c_str()) == keyScopeItr->second)
        {
            ERR_LOG("[StatsSlaveImpl].validateStatDelete: Not allowed to use aggregrate key for keyscope "
                << scopeName.c_str());
            return STATS_ERR_BAD_SCOPE_INFO;
        }

        // assumed that there won't be dupe keyscope names in list
        /// @todo support multiple values for a keyscope
        // build compact mapping of keyscope names to the set of values for that keyscope
        // using ScopeNameValueSetMapWrapper
    }

    PeriodTypeList::const_iterator periodTypeItr = periodTypeList.begin();
    PeriodTypeList::const_iterator periodTypeEnd = periodTypeList.end();
    for (; periodTypeItr != periodTypeEnd; ++periodTypeItr)
    {
        int32_t periodType = *periodTypeItr;

        if (!cat->isValidPeriod(periodType))
        {
            ERR_LOG("[StatsSlaveImpl].validateStatDelete: Invalid period "
                << periodType << " for category " << categoryName);
            return STATS_ERR_BAD_PERIOD_TYPE;
        }
    }

    return ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief deleteStats

    Delete particular stats by category, keyscope and entity id as specified in the request's
    delete list.  Specific deletes may also result in updates (to keyscope aggregate stats),
    so the broadcast is both delete and update.

    \param[in]  request - tdf holding the delete list

    \return - ERR_OK or error
*/
/*************************************************************************************************/
BlazeRpcError StatsSlaveImpl::deleteStats(const DeleteStatsRequest &request)
{
    TRACE_LOG("[StatsSlaveImpl].deleteStats starting.");
    BlazeRpcError err;

    // Go through each row of requested updates and validate them
    DeleteStatsRequest::StatDeleteList::const_iterator deleteIter = request.getStatDeletes().begin();
    DeleteStatsRequest::StatDeleteList::const_iterator deleteEnd = request.getStatDeletes().end();
    for (; deleteIter != deleteEnd; ++deleteIter)
    {
        const StatDelete* statDelete = *deleteIter;

        err = validateStatDelete(statDelete->getCategory(), true,
            statDelete->getKeyScopeNameValueMap(), statDelete->getPeriodTypes());
        if (err != ERR_OK)
        {
            ERR_LOG("[StatsSlaveImpl].deleteStats: validation failed for category "
                << statDelete->getCategory());
            return err;
        }
    }

    StatsDbConnectionManager connMgr;
    DeleteStatsHelper helper(this);

    deleteIter = request.getStatDeletes().begin(); // reset delete list iteration
    for (; deleteIter != deleteEnd; ++deleteIter)
    {
        const StatDelete* statDelete = *deleteIter;
        uint32_t dbId;
        const StatCategory* cat = getConfigData()->getCategoryMap()->find(statDelete->getCategory())->second;
        err = getConfigData()->getShardConfiguration().lookupDbId(cat->getCategoryEntityType(), statDelete->getEntityId(), dbId);
        if (err != ERR_OK)
            return err;

        DbConnPtr conn;
        QueryPtr query;
        if ((err = connMgr.acquireConnWithTxn(dbId, conn, query)) != ERR_OK)
            return err;

        err = helper.execute(statDelete, conn, query);
        if (err != ERR_OK)
            return err;
    }

    helper.broadcastDeleteAndUpdate();

    err = connMgr.commit();
    if (err != ERR_OK)
        return err;

    return ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief deleteStatsByUserSet

    Delete particular stats by category, keyscope and userset as specified in the request's
    delete list.  Iterative version of deleteStats() where a userset specifies a set of entity ids.

    \param[in]  request - tdf holding the delete list

    \return - ERR_OK or error
*/
/*************************************************************************************************/
BlazeRpcError StatsSlaveImpl::deleteStatsByUserSet(const DeleteStatsByUserSetRequest &request)
{
    TRACE_LOG("[StatsSlaveImpl].deleteStatsByUserSet starting.");
    BlazeRpcError err;

    // Go through each row of requested updates and validate them
    DeleteStatsByUserSetRequest::StatDeleteByUserSetList::const_iterator deleteIter = request.getStatDeletes().begin();
    DeleteStatsByUserSetRequest::StatDeleteByUserSetList::const_iterator deleteEnd = request.getStatDeletes().end();
    for (; deleteIter != deleteEnd; ++deleteIter)
    {
        const StatDeleteByUserSet* statDeleteByUserSet = *deleteIter;

        if (statDeleteByUserSet->getUserSetId() == EA::TDF::OBJECT_ID_INVALID)
        {
            ERR_LOG("[StatsSlaveImpl].deleteStatsByUserSet: missing user set");
            return STATS_ERR_INVALID_OBJECT_ID;
        }

        err = validateStatDelete(statDeleteByUserSet->getCategory(), true,
            statDeleteByUserSet->getKeyScopeNameValueMap(), statDeleteByUserSet->getPeriodTypes());
        if (err != ERR_OK)
        {
            ERR_LOG("[StatsSlaveImpl].deleteStatsByUserSet: validation failed for category "
                << statDeleteByUserSet->getCategory());
            return err;
        }
    }

    StatsDbConnectionManager connMgr;
    DeleteStatsHelper helper(this);

    deleteIter = request.getStatDeletes().begin(); // reset delete list iteration
    for (; deleteIter != deleteEnd; ++deleteIter)
    {
        const StatDeleteByUserSet* statDeleteByUserSet = *deleteIter;

        // expand userset to individual entity id stat deletes
        BlazeIdList ids;
        err = gUserSetManager->getUserBlazeIds(statDeleteByUserSet->getUserSetId(), ids);
        if (err != Blaze::ERR_OK)
        {
            WARN_LOG("[StatsSlaveImpl].deleteStatsByUserSet: userset is invalid <"
                << statDeleteByUserSet->getUserSetId().toString().c_str() << ">");
            continue;
        }

        StatDelete statDelete;
        statDelete.setCategory(statDeleteByUserSet->getCategory());
        statDeleteByUserSet->getPeriodTypes().copyInto(statDelete.getPeriodTypes());
        statDeleteByUserSet->getPerOffsets().copyInto(statDelete.getPerOffsets());
        statDeleteByUserSet->getKeyScopeNameValueMap().copyInto(statDelete.getKeyScopeNameValueMap());

        for (BlazeIdList::const_iterator blazeIdItr = ids.begin(); blazeIdItr != ids.end(); ++blazeIdItr)
        {
            statDelete.setEntityId(*blazeIdItr);
            uint32_t dbId;
            const StatCategory* cat = getConfigData()->getCategoryMap()->find(statDelete.getCategory())->second;
            err = getConfigData()->getShardConfiguration().lookupDbId(cat->getCategoryEntityType(), statDelete.getEntityId(), dbId);
            if (err != ERR_OK)
                return err;

            DbConnPtr conn;
            QueryPtr query;
            if ((err = connMgr.acquireConnWithTxn(dbId, conn, query)) != ERR_OK)
                return err;

            err = helper.execute(&statDelete, conn, query);
            if (err != ERR_OK)
                return err;
        }
    }

    helper.broadcastDeleteAndUpdate();

    err = connMgr.commit();
    if (err != ERR_OK)
        return err;

    return ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief deleteStatsByCategory

    Delete all stats by category and period type

    \param[in]  request - list of category-periodtype pairs

    \return - ERR_OK or error
*/
/*************************************************************************************************/
BlazeRpcError StatsSlaveImpl::deleteStatsByCategory(const DeleteStatsByCategoryRequest &request)
{
    TRACE_LOG("[StatsSlaveImpl].deleteStatsByCategory - deleting stats for the given categories.");
    StatUpdateNotification notif;
    notif.getBroadcast().switchActiveMember(StatBroadcast::MEMBER_STATDELETEBYCATEGORY);
    StatDeleteByCategoryBroadcast* broadcast = notif.getBroadcast().getStatDeleteByCategory();
    BlazeRpcError err;

    // Go through each row of requested updates and validate them
    DeleteStatsByCategoryRequest::StatDeleteByCategoryList::const_iterator deleteIter, deleteEnd;
    deleteIter = request.getStatDeletes().begin();
    deleteEnd = request.getStatDeletes().end();

    for (; deleteIter != deleteEnd; ++deleteIter)
    {
        const StatDeleteByCategory* statDelete = *deleteIter;

        // First check that the category exists
        const char8_t* categoryName = statDelete->getCategory();
        CategoryMap::const_iterator catIter = getConfigData()->getCategoryMap()->find(categoryName);
        if (catIter == getConfigData()->getCategoryMap()->end())
        {
            ERR_LOG("[StatsSlaveImpl].deleteStatsByCategory: Could not delete stats for unknown category " << categoryName);
            return STATS_ERR_UNKNOWN_CATEGORY;
        }

        //
        // TODO: Move wipe stats to stat provider
        //
        if (catIter->second->getCategoryType() == CATEGORY_TYPE_GLOBAL)
            return STATS_ERR_INVALID_OPERATION;

        // We do not allow deletion from categories that are involved in multi-category derived stats,
        // this would cause strange inconsistencies because it does not make sense to remove some but
        // not all rows from the DB in such a case.  At some point we may wish to provide an option to
        // cascade the delete to all related categories.
        if (catIter->second->getCategoryDependency()->catSet.size() > 1)
        {
            ERR_LOG("[StatsSlaveImpl].deleteStatsByCategory: Not allowed to delete stats from category "
                << categoryName << " which is involved in a multi-category derived stats relationship.");
            return STATS_ERR_INVALID_OPERATION;
        }

        int32_t periodType = statDelete->getPeriodType();
        if (!catIter->second->isValidPeriod(periodType))
        {
            ERR_LOG("[StatsSlaveImpl].deleteStatsByCategory: invalid period type <" << periodType << ">");
            return STATS_ERR_BAD_PERIOD_TYPE;
        }
    }

    cachePeriodIds(broadcast->getCurrentPeriodIds());

    StatsDbConnectionManager connMgr;
    deleteIter = request.getStatDeletes().begin(); // reset delete list iteration
    broadcast->getCacheDeletes().reserve(request.getStatDeletes().size());
    for (; deleteIter != deleteEnd; ++deleteIter)
    {
        const StatDeleteByCategory* statDelete = *deleteIter;

        const char8_t* categoryName = statDelete->getCategory();
        CategoryMap::const_iterator catIter = getConfigData()->getCategoryMap()->find(categoryName);
        int32_t periodType = statDelete->getPeriodType();
        const char8_t* statsTableName = catIter->second->getTableName(periodType);

        DbConnPtr conn;
        QueryPtr query;
        const DbIdList& list = getConfigData()->getShardConfiguration().getDbIds(catIter->second->getCategoryEntityType());
        for (DbIdList::const_iterator dbIdItr = list.begin(); dbIdItr != list.end(); ++dbIdItr)
        {
            if ((err = connMgr.acquireConnWithTxn(*dbIdItr, conn, query)) != ERR_OK)
                return err;

            if (conn->getSchemaUpdateBlocksAllTxn())
            {
                // as per GOS-31554, DELETE FROM performs better than TRUNCATE TABLE with Galera
                // due to Total Order Isolation method used for Online Schema Upgrades
                query->append("DELETE FROM `$s`;\n", statsTableName);
            }
            else
            {
                query->append("TRUNCATE TABLE `$s`;\n", statsTableName);
            }
        }

        // need to broadcast for any stats-level caching (as well as leaderboard tables)
        CacheRowDelete *cacheRowDelete = broadcast->getCacheDeletes().pull_back();
        cacheRowDelete->setCategoryId(catIter->second->getId());
        cacheRowDelete->getPeriodTypes().push_back(periodType);
    }

    if (!broadcast->getCacheDeletes().empty())
    {
        DbConnPtr lbConn = gDbScheduler->getConnPtr(getDbId());
        if (lbConn == nullptr)
        {
            ERR_LOG("[StatsSlaveImpl].deleteStatsByCategory: failed to obtain connection");
            return ERR_SYSTEM;
        }

        StringBuilder lbQueryString;
        mLeaderboardIndexes->deleteCategoryStats(broadcast->getCacheDeletes(), broadcast->getCurrentPeriodIds(), &lbQueryString, lbConn->getSchemaUpdateBlocksAllTxn());
        if (!lbQueryString.isEmpty())
        {
            QueryPtr query = DB_NEW_QUERY_PTR(lbConn);
            query->append("$s", lbQueryString.get());
            DbError lbError = lbConn->executeMultiQuery(query);
            if (lbError != DB_ERR_OK)
            {
                WARN_LOG("[StatsSlaveImpl].deleteStatsByCategory: failed to update lb tables");
            }
        }

        mStatsCache->reset();
        sendUpdateCacheNotificationToRemoteSlaves(&notif);
    }

    err = connMgr.commit();
    if (err != ERR_OK)
        return err;

    return ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief deleteStatsByKeyScope

    Delete stats by categories and keyscopes.

    Keyscope aggregation (single key) and cached leaderboards are NOT supported.  Support for keyscope aggregation
    is computationally too expensive in a generic case, and this call will fail if attempted.
    Originally, this method was a replacement for deleting stats by "context", and "context" did not
    support cached leaderboards (behavior undefined).

    Keyscopes with aggregate stats CAN still be deleted using KEY_SCOPE_VALUE_ALL for the aggregate stat keyscope. 
    This also allows individual non-aggregate keyscopes to be used with aggregate ones (as long as all aggregates are removed). 

    @todo DEPRECATE in favor of deleteStatsByUserSet(); however, the new method is limited to
    user entity types, and there are use-cases beyond this limitation
    (as well as users/members).

    \param[in]  request - tdf holding the categories and keyscopes

    \return - ERR_OK or error
*/
/*************************************************************************************************/
BlazeRpcError StatsSlaveImpl::deleteStatsByKeyScope(const DeleteStatsByKeyScopeRequest &request)
{
    TRACE_LOG("[StatsSlaveImpl].deleteStatsByKeyScope - deleting stats for the given keyscope.");
    BlazeRpcError err;

    // Go through each row of requested updates and validate them
    DeleteStatsByKeyScopeRequest::StatDeleteByKeyScopeList::const_iterator deleteIter = request.getStatDeletes().begin();
    DeleteStatsByKeyScopeRequest::StatDeleteByKeyScopeList::const_iterator deleteEnd = request.getStatDeletes().end();
    for (; deleteIter != deleteEnd; ++deleteIter)
    {
        const StatDeleteByKeyScope* statDelete = *deleteIter;

        // First check that the category exists
        const char8_t* categoryName = statDelete->getCategory();
        CategoryMap::const_iterator catIter = getConfigData()->getCategoryMap()->find(categoryName);
        if (catIter == getConfigData()->getCategoryMap()->end())
        {
            ERR_LOG("[StatsSlaveImpl].deleteStatsByKeyScope: Could not delete stats for unknown category " << categoryName);
            return STATS_ERR_UNKNOWN_CATEGORY;
        }

        const StatCategory* cat = catIter->second;

        // validate any keyscopes
        if (statDelete->getKeyScopeNameValueMap().empty())
        {
            ERR_LOG("[StatsSlaveImpl].deleteStatsByKeyScope - no keyscopes specified");
            return STATS_ERR_BAD_SCOPE_INFO;
        }
        if (!cat->isValidScopeNameSet(&statDelete->getKeyScopeNameValueMap()))
        {
            ERR_LOG("[StatsSlaveImpl].deleteStatsByKeyScope: Invalid keyscopes for category " << categoryName << ".");
            return STATS_ERR_BAD_SCOPE_INFO;
        }
        ScopeNameValueMap::const_iterator keyScopeItr = statDelete->getKeyScopeNameValueMap().begin();
        ScopeNameValueMap::const_iterator keyScopeEnd = statDelete->getKeyScopeNameValueMap().end();
        for (; keyScopeItr != keyScopeEnd; ++keyScopeItr)
        {
            if (keyScopeItr->second != KEY_SCOPE_VALUE_ALL)
            {
                ScopeName scopeName = keyScopeItr->first;
                if (!getConfigData()->isValidScopeValue(scopeName.c_str(), keyScopeItr->second))
                {
                    ERR_LOG("[StatsSlaveImpl].deleteStatsByKeyScope: Invalid value " << keyScopeItr->second 
                            << " for keyscope " << scopeName.c_str() << ".");
                    return STATS_ERR_BAD_SCOPE_INFO;
                }
                if (getConfigData()->getAggregateKeyValue(scopeName.c_str()) != KEY_SCOPE_VALUE_AGGREGATE)
                {
                    ERR_LOG("[StatsSlaveImpl].deleteStatsByKeyScope - unsupported because it may be too expensive with aggregation enabled for keyscope " << scopeName.c_str() << ".");
                    return STATS_ERR_INVALID_OPERATION;
                }
    
                // assumed that there won't be dupe keyscope names in list
                /// @todo support multiple values for a keyscope
                // build compact mapping of keyscope names to the set of values for that keyscope
                // using ScopeNameValueSetMapWrapper
            }
        }

        int32_t periodType = statDelete->getPeriodType();
        if (!cat->isValidPeriod(periodType))
        {
            ERR_LOG("[StatsSlaveImpl].deleteStatsByKeyScope: invalid period type <" << periodType << ">");
            return STATS_ERR_BAD_PERIOD_TYPE;
        }
    }

    StatsDbConnectionManager connMgr;
    // Iterate through all the rows we want to delete
    deleteIter = request.getStatDeletes().begin();
    for (; deleteIter != deleteEnd; ++deleteIter)
    {
        const StatDeleteByKeyScope* statDelete = *deleteIter;

        const char8_t* categoryName = statDelete->getCategory();
        const StatCategory* cat = getConfigData()->getCategoryMap()->find(categoryName)->second;

        /// @todo select the entities that will be deleted and build a list of
        // individual stat deletes for LRU cache update (could be a huge list)

        DbConnPtr conn;
        QueryPtr query;
        const DbIdList& list = getConfigData()->getShardConfiguration().getDbIds(cat->getCategoryEntityType());
        for (DbIdList::const_iterator dbIdItr = list.begin(); dbIdItr != list.end(); ++dbIdItr)
        {
            if ((err = connMgr.acquireConnWithTxn(*dbIdItr, conn, query)) != ERR_OK)
                return err;
            query->append("DELETE FROM `$s` WHERE TRUE", cat->getTableName(statDelete->getPeriodType()));
            ScopeNameValueMap::const_iterator keyScopeItr = statDelete->getKeyScopeNameValueMap().begin();
            ScopeNameValueMap::const_iterator keyScopeEnd = statDelete->getKeyScopeNameValueMap().end();
            for (; keyScopeItr != keyScopeEnd; ++keyScopeItr)
            {
                if ( keyScopeItr->second != KEY_SCOPE_VALUE_ALL )
                {
                    ScopeName scopeName = keyScopeItr->first;
                    query->append(" AND `$s` = $D", scopeName.c_str(), keyScopeItr->second);
    
                    /// @todo support multiple values for a keyscope
                    // use append(" AND `$s` IN (", ...
                }
            }
            query->append("; ");
        }
    }

    if (mStatsCache->isLruEnabled())
    {
        mStatsCache->reset();

        StatUpdateNotification notif;
        notif.getBroadcast().switchActiveMember(StatBroadcast::MEMBER_STATCLEARCACHE);
        sendUpdateCacheNotificationToRemoteSlaves(&notif);
    }

    err = connMgr.commit();
    if (err != ERR_OK)
        return err;

    return ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief deleteAllStatsByEntity

    Delete all stats in all categories given the entity type and entity id

    \param[in]  request - tdf holding the entity type and entity id

    \return - ERR_OK or error
*/
/*************************************************************************************************/
BlazeRpcError StatsSlaveImpl::deleteAllStatsByEntity(const DeleteAllStatsByEntityRequest &request)
{
    TRACE_LOG("[StatsSlaveImpl].deleteAllStatsByEntity - deleting stats for the given entity and entity type.");
    StatUpdateNotification notif;
    notif.getBroadcast().switchActiveMember(StatBroadcast::MEMBER_STATDELETEBYENTITY);
    StatDeleteByEntityBroadcast* broadcast = notif.getBroadcast().getStatDeleteByEntity();
    BlazeRpcError err;

    cachePeriodIds(broadcast->getCurrentPeriodIds());

    StatsDbConnectionManager connMgr;
    uint32_t dbId;
    err = getConfigData()->getShardConfiguration().lookupDbId(
        request.getEntityType(), request.getEntityId(), dbId);
    if (err != ERR_OK)
        return err;

    DbConnPtr conn;
    QueryPtr query;
    if ((err = connMgr.acquireConnWithTxn(dbId, conn, query)) != ERR_OK)
        return err;

    // Go through all categories and find ones matching the entity type
    CategoryMap::const_iterator categoryItr = getConfigData()->getCategoryMap()->begin();
    CategoryMap::const_iterator categoryEnd = getConfigData()->getCategoryMap()->end();
    for (; categoryItr != categoryEnd; ++categoryItr)
    {
        // First check that the entity type matches
        const StatCategory* cat = categoryItr->second;

        if (cat->getCategoryEntityType() == request.getEntityType())
        {
            //
            // TODO: Move wipe stats to stat provider
            //
            if (cat->getCategoryType() == CATEGORY_TYPE_GLOBAL)
                continue;

            // need to broadcast for any stats-level caching (as well as leaderboard tables)
            CacheRowDelete *cacheRowDelete = broadcast->getCacheDeletes().pull_back();
            cacheRowDelete->setCategoryId(cat->getId());
            cacheRowDelete->setEntityId(request.getEntityId());
            cacheRowDelete->getPeriodTypes().reserve(STAT_NUM_PERIODS);

            for(int32_t periodType = 0; periodType < STAT_NUM_PERIODS; ++periodType)
            {
                if(cat->isValidPeriod(periodType))
                {
                    query->append(
                        "DELETE FROM `$s` WHERE `entity_id` = $D;",
                        cat->getTableName(periodType), request.getEntityId());

                    cacheRowDelete->getPeriodTypes().push_back(periodType);
                }
            }
        }
    }

    if (query->isEmpty())
    {
        WARN_LOG("[StatsSlaveImpl].deleteAllStatsByEntity: did not match any categories for type "
            << request.getEntityType().toString().c_str());
    }
    
    StringBuilder lbQueryString;
    mLeaderboardIndexes->deleteStatsByEntity(broadcast->getCacheDeletes(), broadcast->getCurrentPeriodIds(), &lbQueryString);

    UpdateExtendedDataRequest req;
    req.setComponentId(StatsSlave::COMPONENT_ID);
    req.setIdsAreSessions(false);
    mStatsExtendedDataProvider->getUserStats().deleteUserStats(req, request.getEntityId());
    mStatsExtendedDataProvider->getUserRanks().updateUserRanks(req, broadcast->getCacheDeletes());

    if (!lbQueryString.isEmpty())
    {
        DbConnPtr lbConn = gDbScheduler->getConnPtr(getDbId());
        if (lbConn == nullptr)
        {
            ERR_LOG("[StatsTransactionContext].commit: failed to obtain connection to leaderboard database, will rollback.");
            return ERR_SYSTEM;
        }
        QueryPtr lbQuery = DB_NEW_QUERY_PTR(lbConn);
        lbQuery->append("$s", lbQueryString.get());
        DbError lbError = lbConn->executeMultiQuery(lbQuery);
        if (lbError != DB_ERR_OK)
        {
            WARN_LOG("[StatsSlaveImpl].deleteAllStatsByEntity: failed to update leaderboard cache tables with error <" << getDbErrorString(lbError) << ">.");
        }
    }

    // Send update to master and remote slaves
    if (!broadcast->getCacheDeletes().empty())
    {
        mStatsCache->reset();
        sendUpdateCacheNotificationToRemoteSlaves(&notif);
    }

    err = connMgr.commit();
    if (err != ERR_OK)
        return err;

    BlazeRpcError uedErr = Blaze::ERR_OK;
    uedErr = gUserSessionManager->updateExtendedData(req);
    if (uedErr != Blaze::ERR_OK)
    {
        WARN_LOG("[StatsSlaveImpl].deleteAllStatsByEntity: " << (ErrorHelp::getErrorName(uedErr)) << " error updating user extended data");
    }

    return ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief deleteAllStatsByKeyScope

    Delete all stats in all categories given the keyscope

    Keyscope aggregation and cached leaderboards are NOT supported.  Support for keyscope aggregation
    is computationally too expensive in a generic case, and this call will fail if attempted.
    Originally, this method was a replacement for deleting stats by "context", and "context" did not
    support cached leaderboards (behavior undefined).

    @todo DEPRECATE in favor of deleteAllStatsByKeyScopeAndUserSet(); however, the new method is limited
    to user entity types, and there are use-cases beyond this limitation, e.g. clubs stats tables may
    use types other than "club" and "user".

    \param[in]  request - tdf holding the keyscope

    \return - ERR_OK or error
*/
/*************************************************************************************************/
BlazeRpcError StatsSlaveImpl::deleteAllStatsByKeyScope(const DeleteAllStatsByKeyScopeRequest &request)
{
    TRACE_LOG("[StatsSlaveImpl].deleteAllStatsByKeyScope - deleting stats for the given keyscope");
    BlazeRpcError err;

    if (request.getKeyScopeNameValueMap().empty())
    {
        ERR_LOG("[StatsSlaveImpl].deleteAllStatsByKeyScope - no keyscopes specified");
        return STATS_ERR_BAD_SCOPE_INFO;
    }

    // while validating the keyscopes, build the keyscope part of the query and cache it off (this keyscope part will be the same for all the queries used by this method)
    StringBuilder ksQueryString;

    // validate the given keyscopes
    ScopeNameValueMap::const_iterator keyScopeItr = request.getKeyScopeNameValueMap().begin();
    ScopeNameValueMap::const_iterator keyScopeEnd = request.getKeyScopeNameValueMap().end();
    for (; keyScopeItr != keyScopeEnd; ++keyScopeItr)
    {
        ScopeName scopeName = keyScopeItr->first;
        if (!getConfigData()->isValidScopeValue(scopeName.c_str(), keyScopeItr->second))
        {
            ERR_LOG("[StatsSlaveImpl].deleteAllStatsByKeyScope - Invalid value " << keyScopeItr->second 
                    << " for keyscope " << scopeName.c_str() << ".");
            return STATS_ERR_BAD_SCOPE_INFO;
        }
        if (getConfigData()->getAggregateKeyValue(scopeName.c_str()) != KEY_SCOPE_VALUE_AGGREGATE)
        {
            ERR_LOG("[StatsSlaveImpl].deleteAllStatsByKeyScope - unsupported because it may be too expensive with aggregation enabled for keyscope " << scopeName.c_str() << ".");
            return STATS_ERR_INVALID_OPERATION;
        }

        // assumed that there won't be dupe keyscope names in list
        /// @todo support multiple values for a keyscope, e.g. use append(" AND `%s` IN (", ...
        // consider building a compact mapping of keyscope names to the set of values for that keyscope using ScopeNameValueSetMapWrapper

        ksQueryString.append(" AND `%s` = %" PRIi64, scopeName.c_str(), keyScopeItr->second);
    }

    StatsDbConnectionManager connMgr;
    // Go through all categories and find ones matching the keyscope
    CategoryMap::const_iterator categoryItr = getConfigData()->getCategoryMap()->begin();
    CategoryMap::const_iterator categoryEnd = getConfigData()->getCategoryMap()->end();
    for (; categoryItr != categoryEnd; ++categoryItr)
    {
        const StatCategory* cat = categoryItr->second;

        keyScopeItr = request.getKeyScopeNameValueMap().begin();
        for (; keyScopeItr != keyScopeEnd; ++keyScopeItr)
        {
            if (!cat->isValidScopeName(keyScopeItr->first.c_str()))
            {
                break;
            }
        }
        // if iterator reaches the end, then this category matches the given keyscopes
        if (keyScopeItr == keyScopeEnd)
        {
            DbConnPtr conn;
            QueryPtr query;
            DbIdList list = getConfigData()->getShardConfiguration().getDbIds(cat->getCategoryEntityType());
            for (DbIdList::const_iterator dbIdItr = list.begin(); dbIdItr != list.end(); ++dbIdItr)
            {
                if ((err = connMgr.acquireConnWithTxn(*dbIdItr, conn, query)) != ERR_OK)
                    return err;

                for (int32_t periodType = 0; periodType < STAT_NUM_PERIODS; ++periodType)
                {
                    if (!cat->isValidPeriod(periodType))
                        continue;

                    /// @todo select the entities that will be deleted and build a list of
                    // individual stat deletes for LRU cache update (could be a huge list)

                    // optimization as per GOS-31534, explicitly delete from each period (partition)
                    if (periodType == STAT_PERIOD_ALL_TIME)
                    {
                        query->append(
                            "DELETE FROM `$s` WHERE TRUE $s; ",
                            cat->getTableName(periodType),
                            ksQueryString.get()
                            );
                    }
                    else
                    {
                        int32_t currentPeriodId = getPeriodId(periodType);
                        int32_t periodId = currentPeriodId - getRetention(periodType);
                        for (; periodId <= currentPeriodId; ++periodId)
                        {
                            query->append(
                                "DELETE FROM `$s` WHERE `period_id` = $d $s; ",
                                cat->getTableName(periodType),
                                periodId,
                                ksQueryString.get()
                                );
                        }
                    }
                }

                // execute each of these 'little' multi-queries separately (should be okay since we're in a transaction),
                // rather than build potentially 'too large' ones for execution only at the end (see commit())
                if (!query->isEmpty())
                {
                    err = conn->executeMultiQuery(query);
                    if (err != ERR_OK)
                        return err;
                    DB_QUERY_RESET(query);
                }
            }
        }
    }

    if (mStatsCache->isLruEnabled())
    {
        mStatsCache->reset();

        StatUpdateNotification notif;
        notif.getBroadcast().switchActiveMember(StatBroadcast::MEMBER_STATCLEARCACHE);
        sendUpdateCacheNotificationToRemoteSlaves(&notif);
    }

    err = connMgr.commit();
    if (err != ERR_OK)
        return err;

    return ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief deleteAllStatsByKeyScopeAndEntity

    Delete all stats in all categories given the keyscope and entity id.  Specific deletes may also
    result in updates (to keyscope aggregate stats), so the broadcast is both delete and update.

    \param[in]  request - tdf holding the keyscope and entity id

    \return - ERR_OK or error
*/
/*************************************************************************************************/
BlazeRpcError StatsSlaveImpl::deleteAllStatsByKeyScopeAndEntity(
    const DeleteAllStatsByKeyScopeAndEntityRequest &request)
{
    TRACE_LOG("[StatsSlaveImpl].deleteAllStatsByKeyScopeAndEntity - deleting stats for the given keyscope and entity id.");
    BlazeRpcError err;

    if (request.getKeyScopeNameValueMap().empty())
    {
        ERR_LOG("[StatsSlaveImpl].deleteAllStatsByKeyScopeAndEntity - missing keyscope parameter");
        // and deleteAllStatsByEntity might as well have been used
        return STATS_ERR_BAD_SCOPE_INFO;
    }

    // validate the given keyscopes
    ScopeNameValueMap::const_iterator keyScopeItr = request.getKeyScopeNameValueMap().begin();
    ScopeNameValueMap::const_iterator keyScopeEnd = request.getKeyScopeNameValueMap().end();
    for (; keyScopeItr != keyScopeEnd; ++keyScopeItr)
    {
        ScopeName scopeName = keyScopeItr->first;
        if (!getConfigData()->isValidScopeValue(scopeName.c_str(), keyScopeItr->second))
        {
            ERR_LOG("[StatsSlaveImpl].deleteAllStatsByKeyScopeAndEntity: Invalid value " << keyScopeItr->second 
                    << " for keyscope " << scopeName.c_str() << ".");
            return STATS_ERR_BAD_SCOPE_INFO;
        }
        if (getConfigData()->getAggregateKeyValue(scopeName.c_str()) == keyScopeItr->second)
        {
            ERR_LOG("[StatsSlaveImpl].deleteAllStatsByKeyScopeAndEntity: Not allowed to delete aggregrate key " 
                    << keyScopeItr->second << " for keyscope " << scopeName.c_str() << ".");
            return STATS_ERR_BAD_SCOPE_INFO;
        }

        // assumed that there won't be dupe keyscope names in list
        /// @todo support multiple values for a keyscope
        // build compact mapping of keyscope names to the set of values for that keyscope
        // using ScopeNameValueSetMapWrapper
    }

    StatsDbConnectionManager connMgr;
    uint32_t dbId;
    err = getConfigData()->getShardConfiguration().lookupDbId(
        request.getEntityType(), request.getEntityId(), dbId);
    if (err != ERR_OK)
        return err;

    DbConnPtr conn;
    QueryPtr query;
    if ((err = connMgr.acquireConnWithTxn(dbId, conn, query)) != ERR_OK)
        return err;

    StatDelete statDelete;
    statDelete.setEntityId(request.getEntityId());
    request.getKeyScopeNameValueMap().copyInto(statDelete.getKeyScopeNameValueMap());
    DeleteStatsHelper helper(this);

    // Go through all categories and find ones matching the keyscope and entity
    CategoryMap::const_iterator categoryItr = getConfigData()->getCategoryMap()->begin();
    CategoryMap::const_iterator categoryEnd = getConfigData()->getCategoryMap()->end();
    for (; categoryItr != categoryEnd; ++categoryItr)
    {
        const StatCategory* cat = categoryItr->second;

        if (cat->getCategoryEntityType() != request.getEntityType())
            continue;

        //
        // TODO: Move wipe stats to stat provider
        //
        if (cat->getCategoryType() == CATEGORY_TYPE_GLOBAL)
            continue;

        keyScopeItr = request.getKeyScopeNameValueMap().begin();
        for (; keyScopeItr != keyScopeEnd; ++keyScopeItr)
        {
            if (!cat->isValidScopeName(keyScopeItr->first.c_str()))
            {
                break;
            }
        }
        // if iterator reaches the end, then this category matches the given keyscopes
        if (keyScopeItr == keyScopeEnd)
        {
            statDelete.setCategory(cat->getName());

            BlazeRpcError error = helper.execute(&statDelete, conn, query);
            if (error != ERR_OK)
                return error;
        }
    }

    helper.broadcastDeleteAndUpdate();

    err = connMgr.commit();
    if (err != ERR_OK)
        return err;

    return ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief deleteAllStatsByKeyScopeAndUserSet

    Delete all stats in all categories given the keyscope and userset.  Iterative version of 
    deleteAllStatsByKeyScopeAndEntity() where a userset specifies a set of entity ids.

    \param[in]  request - tdf holding the keyscope and userset

    \return - ERR_OK or error
*/
/*************************************************************************************************/
BlazeRpcError StatsSlaveImpl::deleteAllStatsByKeyScopeAndUserSet(
    const DeleteAllStatsByKeyScopeAndUserSetRequest &request)
{
    TRACE_LOG("[StatsSlaveImpl].deleteAllStatsByKeyScopeAndUserSet - deleting stats for the given keyscope and userset.");
    BlazeRpcError err;

    if (request.getKeyScopeNameValueMap().empty())
    {
        ERR_LOG("[StatsSlaveImpl].deleteAllStatsByKeyScopeAndUserSet - missing keyscope parameter");
        // and deleteAllStatsByUserSet might as well have been used
        return STATS_ERR_BAD_SCOPE_INFO;
    }

    // validate the given keyscopes
    ScopeNameValueMap::const_iterator keyScopeItr = request.getKeyScopeNameValueMap().begin();
    ScopeNameValueMap::const_iterator keyScopeEnd = request.getKeyScopeNameValueMap().end();
    for (; keyScopeItr != keyScopeEnd; ++keyScopeItr)
    {
        ScopeName scopeName = keyScopeItr->first;
        if (!getConfigData()->isValidScopeValue(scopeName.c_str(), keyScopeItr->second))
        {
            ERR_LOG("[StatsSlaveImpl].deleteAllStatsByKeyScopeAndUserSet: Invalid value " << keyScopeItr->second 
                    << " for keyscope " << scopeName.c_str() << ".");
            return STATS_ERR_BAD_SCOPE_INFO;
        }
        if (getConfigData()->getAggregateKeyValue(scopeName.c_str()) == keyScopeItr->second)
        {
            ERR_LOG("[StatsSlaveImpl].deleteAllStatsByKeyScopeAndUserSet: Not allowed to delete aggregrate key " 
                    << keyScopeItr->second << " for keyscope " << scopeName.c_str() << ".");
            return STATS_ERR_BAD_SCOPE_INFO;
        }

        // assumed that there won't be dupe keyscope names in list
        /// @todo support multiple values for a keyscope
        // build compact mapping of keyscope names to the set of values for that keyscope
        // using ScopeNameValueSetMapWrapper
    }

    if (request.getUserSetId() == EA::TDF::OBJECT_ID_INVALID)
    {
        ERR_LOG("[StatsSlaveImpl].deleteAllStatsByKeyScopeAndUserSet: missing user set");
        return STATS_ERR_INVALID_OBJECT_ID;
    }
    BlazeIdList ids;
    err = gUserSetManager->getUserBlazeIds(request.getUserSetId(), ids);
    if (err != Blaze::ERR_OK)
    {
        ERR_LOG("[StatsSlaveImpl].deleteAllStatsByKeyScopeAndUserSet: userset is invalid <"
            << request.getUserSetId().toString().c_str() << ">");
        return STATS_ERR_INVALID_OBJECT_ID;
    }
    if (ids.empty())
    {
        WARN_LOG("[StatsSlaveImpl].deleteAllStatsByKeyScopeAndUserSet: userset is empty <"
            << request.getUserSetId().toString().c_str() << ">");
        return ERR_OK;
    }

    StatsDbConnectionManager connMgr;
    StatDelete statDelete;
    request.getKeyScopeNameValueMap().copyInto(statDelete.getKeyScopeNameValueMap());
    DeleteStatsHelper helper(this);

    // Go through all categories and find ones matching the keyscope and entity
    CategoryMap::const_iterator categoryItr = getConfigData()->getCategoryMap()->begin();
    CategoryMap::const_iterator categoryEnd = getConfigData()->getCategoryMap()->end();
    for (; categoryItr != categoryEnd; ++categoryItr)
    {
        const StatCategory* cat = categoryItr->second;

        if (cat->getCategoryEntityType() != ENTITY_TYPE_USER)
            continue;

        //
        // TODO: Move wipe stats to stat provider
        //
        if (cat->getCategoryType() == CATEGORY_TYPE_GLOBAL)
            continue;

        keyScopeItr = request.getKeyScopeNameValueMap().begin();
        for (; keyScopeItr != keyScopeEnd; ++keyScopeItr)
        {
            if (!cat->isValidScopeName(keyScopeItr->first.c_str()))
            {
                break;
            }
        }
        // if iterator reaches the end, then this category matches the given keyscopes
        if (keyScopeItr == keyScopeEnd)
        {
            statDelete.setCategory(cat->getName());

            // expand userset to individual entity id stat deletes
            for (BlazeIdList::const_iterator blazeIdItr = ids.begin(); blazeIdItr != ids.end(); ++blazeIdItr)
            {
                statDelete.setEntityId(*blazeIdItr);

                uint32_t dbId;
                err = getConfigData()->getShardConfiguration().lookupDbId(
                    cat->getCategoryEntityType(), statDelete.getEntityId(), dbId);
                if (err != ERR_OK)
                    return err;

                DbConnPtr conn;
                QueryPtr query;
                if ((err = connMgr.acquireConnWithTxn(dbId, conn, query)) != ERR_OK)
                    return err;

                err = helper.execute(&statDelete, conn, query);
                if (err != ERR_OK)
                    return err;
            }
        }
    }

    helper.broadcastDeleteAndUpdate();

    err = connMgr.commit();
    if (err != ERR_OK)
        return err;

    return ERR_OK;
}


/*************************************************************************************************/
/*!
    \brief deleteAllStatsByCategoryAndEntity

    Delete all stats by category and entityId

    \param[in]  request - tdf holding the entity type, entity id and category 

    \return - ERR_OK or error
*/
/*************************************************************************************************/
BlazeRpcError StatsSlaveImpl::deleteAllStatsByCategoryAndEntity(const DeleteAllStatsByCategoryAndEntityRequest  &request)
{
    TRACE_LOG("[StatsSlaveImpl].deleteAllStatsByCategoryAndEntity - deleting stats for the given category and entity.");
    StatUpdateNotification notif;
    notif.getBroadcast().switchActiveMember(StatBroadcast::MEMBER_STATDELETEBYENTITY);
    StatDeleteByEntityBroadcast* broadcast = notif.getBroadcast().getStatDeleteByEntity();
    BlazeRpcError err;

    const char8_t* categoryName = request.getCategory();
    CategoryMap::const_iterator catIter = getConfigData()->getCategoryMap()->find(categoryName);
    if (catIter == getConfigData()->getCategoryMap()->end())
    {
        ERR_LOG("[StatsSlaveImpl].deleteAllStatsByCategoryAndEntity: unknown category " << categoryName);
        return STATS_ERR_UNKNOWN_CATEGORY;
    }
    const StatCategory* cat = catIter->second;

    //
    // TODO: Move wipe stats to stat provider
    //
    if (cat->getCategoryType() == CATEGORY_TYPE_GLOBAL)
        return STATS_ERR_INVALID_OPERATION;

    // We do not allow deletion from categories that are involved in multi-category derived stats,
    // this would cause strange inconsistencies because it does not make sense to remove some but
    // not all rows from the DB in such a case.  At some point we may wish to provide an option to
    // cascade the delete to all related categories.
    if (cat->getCategoryDependency()->catSet.size() > 1)
    {
        ERR_LOG("[StatsSlaveImpl].deleteAllStatsByCategoryAndEntity: Not allowed to delete stats from category "
            << categoryName << " which is involved in a multi-category derived stats relationship.");
        return STATS_ERR_INVALID_OPERATION;
    }

    cachePeriodIds(broadcast->getCurrentPeriodIds());

    StatsDbConnectionManager connMgr;
    uint32_t dbId;
    err = getConfigData()->getShardConfiguration().lookupDbId(
        cat->getCategoryEntityType(), request.getEntityId(), dbId);
    if (err != ERR_OK)
        return err;

    DbConnPtr conn;
    QueryPtr query;
    if ((err = connMgr.acquireConnWithTxn(dbId, conn, query)) != ERR_OK)
        return err;

    // need to broadcast for any stats-level caching (as well as leaderboard tables)
    CacheRowDelete *cacheRowDelete = broadcast->getCacheDeletes().pull_back();
    cacheRowDelete->setCategoryId(cat->getId());
    cacheRowDelete->setEntityId(request.getEntityId());

    PeriodTypeList periodTypes;
    if (!request.getPeriodTypes().empty())
    {
        periodTypes.reserve(request.getPeriodTypes().size());
        for (DeleteAllStatsByCategoryAndEntityRequest::StatPeriodTypeList::const_iterator it = request.getPeriodTypes().begin(); it != request.getPeriodTypes().end(); ++it)
            periodTypes.push_back(*it);
    }
    else
    {
        int32_t catPeriodTypes = cat->getPeriodTypes();
        periodTypes.reserve(STAT_NUM_PERIODS);
        for (int32_t period = 0; period < STAT_NUM_PERIODS; ++period)
            if ((catPeriodTypes & (1 << period)) != 0)
                periodTypes.push_back(period);
    }

    PeriodTypeList::const_iterator perIter = periodTypes.begin();
    PeriodTypeList::const_iterator perEnd = periodTypes.end();
    periodTypes.copyInto(cacheRowDelete->getPeriodTypes());
    for(; perIter != perEnd; ++perIter)
    {
        query->append(
            "DELETE FROM `$s` WHERE `entity_id` = $D;",
            cat->getTableName(*perIter), request.getEntityId());
    }

    UpdateExtendedDataRequest req;
    req.setComponentId(StatsSlave::COMPONENT_ID);
    req.setIdsAreSessions(false);

    StringBuilder lbQueryString;
    mLeaderboardIndexes->deleteStatsByEntity(broadcast->getCacheDeletes(), broadcast->getCurrentPeriodIds(), &lbQueryString);

    for (PeriodTypeList::const_iterator pit = periodTypes.begin(); pit != periodTypes.end(); ++pit)
        mStatsExtendedDataProvider->getUserStats().deleteUserStats(req, request.getEntityId(), cat->getId(), *pit);
    mStatsExtendedDataProvider->getUserRanks().updateUserRanks(req, broadcast->getCacheDeletes());

    if (!lbQueryString.isEmpty())
    {
        DbConnPtr lbConn = gDbScheduler->getConnPtr(getDbId());
        if (lbConn == nullptr)
        {
            ERR_LOG("[StatsSlaveImpl].deleteAllStatsByEntityAndCategory: failed to obtain connection to leaderboard database, will rollback.");
            return ERR_SYSTEM;
        }
        QueryPtr lbQuery = DB_NEW_QUERY_PTR(lbConn);
        lbQuery->append("$s", lbQueryString.get());
        DbError lbError = lbConn->executeMultiQuery(lbQuery);
        if (lbError != DB_ERR_OK)
        {
            WARN_LOG("[StatsSlaveImpl].deleteAllStatsByEntityAndCategory: failed to update leaderboard cache tables with error <" << getDbErrorString(lbError) << ">.");
        }
    }

    // Send update to master and remote slaves
    mStatsCache->reset();
    sendUpdateCacheNotificationToRemoteSlaves(&notif);

    err = connMgr.commit();
    if (err != ERR_OK)
        return err;

    BlazeRpcError uedErr = Blaze::ERR_OK;
    uedErr = gUserSessionManager->updateExtendedData(req);
    if (uedErr != Blaze::ERR_OK)
    {
        WARN_LOG("[StatsSlaveImpl].deleteAllStatsByCategoryAndEntity: " << (ErrorHelp::getErrorName(uedErr)) << " error updating user extended data");
    }
        
    return ERR_OK;
}

} // Stats
} // Blaze
