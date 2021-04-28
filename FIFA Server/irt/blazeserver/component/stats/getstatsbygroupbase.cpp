/*************************************************************************************************/
/*!
    \file   getstatsbygroupbase.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetStatsByGroupBase

    Retrieves statistics for pre-defined stat columns.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "EASTL/sort.h"
#include "EASTL/algorithm.h"

#include "statsconfig.h"
#include "statsslaveimpl.h"
#include "stats/rpc/statsslave/getstatsbygroup_stub.h"
#include "getstatsbygroupbase.h"
#include "scope.h"
#include "statscacheprovider.h"

namespace Blaze
{
namespace Stats
{

GetStatsByGroupBase::GetStatsByGroupBase(StatsSlaveImpl* component)
:   mComponent(component),
    mGroup(nullptr),
    mScopeBlockHead(nullptr)
{
}

GetStatsByGroupBase::~GetStatsByGroupBase()
{
    // NOTE: Although StatRow objects point to a sub-array of scopes, GetStatsByGroupBase::dtor()
    // is *always* responsible for freeing the memory associated with all the scope blocks.
    while (mScopeBlockHead != nullptr)
    {
        ScopeBlock* tmp = mScopeBlockHead;
        mScopeBlockHead = mScopeBlockHead->next;
        BLAZE_FREE(tmp);
    }
}

BlazeRpcError GetStatsByGroupBase::executeBase(const GetStatsByGroupRequest& request, GetStatsResponse& response)
{
    StatGroupMap::const_iterator iter = mComponent->getConfigData()->getStatGroupMap()->find(request.getGroupName());
    if (iter == mComponent->getConfigData()->getStatGroupMap()->end())
    {
        TRACE_LOG("[GetStatsByGroupBase].executeBase: unknown stat group: " << request.getGroupName());
        return STATS_ERR_UNKNOWN_STAT_GROUP;
    }

    mGroup = iter->second;
    if (mGroup->getStats().empty())
    {
        // this should never happen, added for safety
        ERR_LOG("[GetStatsByGroupBase].executeBase: empty stat group: " << request.getGroupName());
        return ERR_SYSTEM;
    }

    // store off the current periods *before* executing queries in case of a period rollover during any blocking call
    mComponent->getPeriodIds(mPeriodIds);
    const StatPeriodType periodType = static_cast<StatPeriodType>(request.getPeriodType());
    if (periodType != STAT_PERIOD_ALL_TIME && request.getPeriodCtr() < 1)
    {
        TRACE_LOG("[GetStatsByGroupBase].executeBase: Invalid period counter <" << request.getPeriodCtr()
            << "> for non-all-time period type; must be at least <1>");
        return STATS_ERR_BAD_PERIOD_COUNTER;
    }

    // NOTE: We only need to validate the scopes for the group because all secondary categories
    // must have their scopes fully specified at configuration time, or inherited from the group.
    BlazeRpcError rc = validateScope(request);
    if (rc != ERR_OK)
        return rc;

    if (!mGroup->getStatCategory()->isValidPeriod(periodType))
    {
        TRACE_LOG("[GetStatsByGroupBase].executeBase(): Stat group [" << mGroup->getName()
                  << "] primary category [" << mGroup->getStatCategory()->getName() << "] does not support periodType <" << periodType << ">");
        return STATS_ERR_BAD_PERIOD_TYPE;
    }

    // if we are doing an inner join, we will only use entity ids
    // that were found in the primary select, hence this set.
    EntityIdSet entityIdSet;
    
    // (outer join: if a row exists in any category, the missing fields from other categories are filled in with defaults)
    // (inner join: rows are only taken from secondary categories when the primary category contains the same row)
    bool doOuterJoin = !request.getEntityIds().empty();
    rc = buildPrimaryResults(request, entityIdSet, doOuterJoin);
    if (rc != ERR_OK)
        return rc;
    
    if (doOuterJoin)
    {
        // grab all the incoming entity ids because we are doing a join on them
        entityIdSet.reserve(request.getEntityIds().size());
        entityIdSet.assign(request.getEntityIds().begin(), request.getEntityIds().end());
    }
    else if (entityIdSet.empty())
    {
        TRACE_LOG("[GetStatsByGroupBase].executeBase: no primary stats fetched for group [" << mGroup->getName() << "].");
        return ERR_OK;
    }
    
    const bool hasSecondarySelects = mGroup->getSelectList().size() > 1;
    if (hasSecondarySelects)
    {
        rc = buildSecondaryResults(request, entityIdSet, doOuterJoin);
        if (rc != ERR_OK)
            return rc;
    }
    
    // use intermediate data stored in mRowList to build the rpc response object
    buildResponse(response);
    
    return ERR_OK;
}

/*! ***********************************************************************************************/
/*! \brief validate the scope settings from client for get stat by group

    \param[in] scopeNameValueMap - list of scope name/value pairs in the request

    \return ERR_OK if scope settings in request are good, otherwise STATS_ERR_BAD_SCOPE_INFO
***************************************************************************************************/
BlazeRpcError GetStatsByGroupBase::validateScope(const GetStatsByGroupRequest& request) const
{
    uint32_t questionMarks = 0;
    const ScopeNameValueMap& reqScopeMap = request.getKeyScopeNameValueMap();
    const ScopeNameValueMap& groupScopeMap = getGroupScopeMap();
    // using group stat name/value pair as query condition, need to grab
    // actual value from request if the value defined for group is '?'
    ScopeNameValueMap::const_iterator it = groupScopeMap.begin();
    ScopeNameValueMap::const_iterator itend = groupScopeMap.end();
    for (; it != itend; ++it)
    {
        if (it->second == KEY_SCOPE_VALUE_USER)
        {
            ScopeNameValueMap::const_iterator reqIter = reqScopeMap.find(it->first);
            if (reqIter == reqScopeMap.end())
            {
                WARN_LOG("[GetStatsByGroupBase].validateScope: group [" << mGroup->getName()
                    << "] scope [" << it->first.c_str() << "] value not defined in the request.");
                return STATS_ERR_BAD_SCOPE_INFO;
            }

            ++questionMarks;

            if (reqIter->second == KEY_SCOPE_VALUE_ALL)
            {
                continue;
            }

            if (!mComponent->getConfigData()->isValidScopeValue(reqIter->first, reqIter->second))
            {
                WARN_LOG("[GetStatsByGroupBase].validateScope: group [" << mGroup->getName()
                    << "] scope [" << reqIter->first.c_str() << "] has invalid value <" << reqIter->second << ">.");
                return STATS_ERR_BAD_SCOPE_INFO;
            }
        }
    }

    const uint32_t numReqScopes = static_cast<uint32_t>(reqScopeMap.size());
    if (questionMarks != numReqScopes)
    {
        WARN_LOG("[GetStatsByGroupBase].validateScope: group [" << mGroup->getName()
            << "] requires <" << questionMarks << "> scopes, request specified <" << numReqScopes << "> scopes.");
        return STATS_ERR_BAD_SCOPE_INFO;
    }

    return ERR_OK;
}

/*! ***********************************************************************************************/
/*! \brief get the stats in the primary category

    \param[in] request - the request TDF
    \param[out] entityIdSet - entities retrieved
    \param[out] doOuterJoin - whether or not to include secondary stats with no corresponding primary stats

    \return ERR_OK if successful, otherwise some error code
***************************************************************************************************/
BlazeRpcError GetStatsByGroupBase::buildPrimaryResults(const GetStatsByGroupRequest& request,
    EntityIdSet& entityIdSet, bool& doOuterJoin)
{
    // setup stat provider parameters for primary category fetch
    const StatGroup::Select& sel = mGroup->getSelectList().front();
    if (sel.mDescs.empty())
    {
        ERR_LOG("[GetStatsByGroupBase].buildPrimaryResults: group [" << mGroup->getName()
            << "] category [" << sel.mCategory->getName() << "] has no stats.");
        return ERR_SYSTEM;
    }

    // if periodType is specified in stats config,
    // it takes priority over requested period
    int32_t periodType = sel.getPeriodType();
    if (periodType == -1)
        periodType = request.getPeriodType();

    int32_t periodId = 0;
    int32_t periodCount = 1;
    if (periodType != STAT_PERIOD_ALL_TIME)
    {
        if (periodType == request.getPeriodType())
        {
            if(request.getPeriodOffset() < 0 || mPeriodIds[periodType] < request.getPeriodOffset())
            {
                ERR_LOG("[GetStatsByGroupBase].buildPrimaryResults(): bad period offset value");
                return STATS_ERR_BAD_PERIOD_OFFSET;
            }
            periodId = getOffsetPeriodId(periodType, request);
            // more than 1 period may have been requested
            periodCount = request.getPeriodCtr();
        }
        else
        {
            // use current period
            periodId = mPeriodIds[periodType];
        }
    }

    // build keyscopes for primary fetch
    ScopeNameValueMap primaryScopeMap;
    const ScopeNameValueMap &groupScopeMap = getGroupScopeMap();
    ScopeNameValueMap::const_iterator groupScopeItr = groupScopeMap.begin();
    ScopeNameValueMap::const_iterator groupScopeEnd = groupScopeMap.end();
    for (; groupScopeItr != groupScopeEnd; ++groupScopeItr)
    {
        if (groupScopeItr->second == KEY_SCOPE_VALUE_ALL)
        {
            // When the user's keyscope contains '*', it is not possible to join all the 
            // results using an 'outer' join, because if a row exists in a secondary category, 
            // but not in the primary a row would need to be created for each keyscope value 
            // designated by '*'. To avoid this we always use a left join for groups with '*' keyscopes.
            doOuterJoin = false;
            continue;
        }

        ScopeValue scopeValue;
        if (groupScopeItr->second == KEY_SCOPE_VALUE_USER)
        {
            ScopeNameValueMap::const_iterator requestScopeItr = request.getKeyScopeNameValueMap().find(groupScopeItr->first);
            if (requestScopeItr == request.getKeyScopeNameValueMap().end())
            {
                ERR_LOG("[GetStatsByGroupBase].buildPrimaryResults: missing request scope ["
                    << groupScopeItr->first.c_str() << "] in group [" << mGroup->getName() << "]");
                return STATS_ERR_BAD_SCOPE_INFO;
            }

            if (requestScopeItr->second == KEY_SCOPE_VALUE_ALL)
            {
                // When the user's keyscope contains '*', it is not possible to join all the 
                // results using an 'outer' join, because if a row exists in a secondary category, 
                // but not in the primary a row would need to be created for each keyscope value 
                // designated by '*'. To avoid this we always use a left join for groups with '*' keyscopes.
                doOuterJoin = false;
                continue;
            }

            if (!mComponent->getConfigData()->isValidScopeValue(requestScopeItr->first, requestScopeItr->second))
            {
                ERR_LOG("[GetStatsByGroupBase].buildPrimaryResults: invalid value for scope ["
                    << requestScopeItr->first.c_str() << "] in group [" << mGroup->getName() << "]");
                return STATS_ERR_BAD_SCOPE_INFO;
            }

            scopeValue = requestScopeItr->second;
        }
        else if (groupScopeItr->second == KEY_SCOPE_VALUE_AGGREGATE)
        {
            // validation of aggregateKey-enabled should have been done on server startup
            scopeValue = mComponent->getConfigData()->getAggregateKeyValue(groupScopeItr->first.c_str());
        }
        else
        {
            scopeValue = groupScopeItr->second;
        }

        primaryScopeMap.insert(eastl::make_pair(groupScopeItr->first, scopeValue));
    }

    if (request.getEntityIds().empty())
    {
        // when no entity ids are specified we always do a left join
        doOuterJoin = false;
    }

    StatsProviderPtr statsProvider;

    if (mComponent->getStatsCache().isLruEnabled())
    {
        statsProvider = StatsProviderPtr(BLAZE_NEW StatsCacheProvider(*mComponent, mComponent->getStatsCache()));
    }
    else
    {
        statsProvider = StatsProviderPtr(BLAZE_NEW DbStatsProvider(*mComponent));
    }

    BlazeRpcError err = statsProvider->executeFetchRequest(
        *mGroup->getStatCategory(),
        request.getEntityIds().asVector(),
        periodType,
        periodId,
        periodCount,
        primaryScopeMap);
    if (err != Blaze::ERR_OK)
    {
        return ERR_SYSTEM;
    }

    // how many rows did we get back?
    size_t totalRows = 0;
    statsProvider->reset();
    while (statsProvider->hasNextResultSet())
    {
        StatsProviderResultSet* resultSet = statsProvider->getNextResultSet();
        if (resultSet != nullptr)
        {
            totalRows += resultSet->size();
        }
    }

    if (doOuterJoin)
    {
        // entity id set cannot be bigger than the set of ids in the request
        entityIdSet.reserve(request.getEntityIds().size());
    }
    else
    {
        // entity id set cannot be bigger than the number of returned rows
        entityIdSet.reserve(totalRows);
    }

    // reserve enough space for primary results
    mRowList.reserve(totalRows);

    // build a read-only list of default stat values
    buildDefaultStatList();

    const EA::TDF::ObjectType entityType = mGroup->getStatCategory()->getCategoryEntityType();
    ScopeValue* scope = allocateScopeBlock(totalRows*groupScopeMap.size());

    statsProvider->reset();
    while (statsProvider->hasNextResultSet())
    {
        StatsProviderResultSet* resultSet = statsProvider->getNextResultSet();
        if (resultSet == nullptr || resultSet->size() == 0)
        {
            continue;
        }
        resultSet->reset();
        while (resultSet->hasNextRow())
        {
            StatsProviderRowPtr row = resultSet->getNextRow();

            StatRow& statRow = mRowList.push_back();
            statRow.entityId = row->getEntityId();
            statRow.stats = BLAZE_NEW EntityStats();
            statRow.scopes = scope;
            for (ScopeNameValueMap::const_iterator i = groupScopeMap.begin(), e = groupScopeMap.end(); i != e; ++i)
            {
                // write the scope values
                *scope++ = row->getValueInt64(i->first.c_str());
            }
            statRow.stats->setEntityId(statRow.entityId);
            statRow.stats->setEntityType(entityType);
            // this "mPeriodIds" is our copy of the current period ids when the command started
            if (periodType != STAT_PERIOD_ALL_TIME)
            {
                statRow.stats->setPeriodOffset(mPeriodIds[periodType] - row->getPeriodId());
            }
            // set the # of stats to the total defined by this stat group
            statRow.stats->getStatValues().resize(mGroup->getStats().size());

            // set all stat values to point to default values for each particular stat
            for (size_t i = 0, size = mDefaultStatList.size(); i < size; ++i)
            {
                // NOTE: does not create a new string, just points to the values in the mDefaultStatList
                statRow.stats->getStatValues()[i].assignBuffer(mDefaultStatList[i]);
            }

            // iterate only the group stat descs from the primary category
            for (StatGroup::DescList::const_iterator i = sel.mDescs.begin(), e = sel.mDescs.end(); i != e; ++i)
            {
                const StatDesc& desc = **i;
                const Stat& stat = *desc.getStat();
                formatStat(mAsStr, sizeof(mAsStr), stat, row);
                // write the primary stat value directly to the index of the stat desc
                statRow.stats->getStatValues()[desc.getIndex()] = mAsStr;
            }

            if (!doOuterJoin)
            {
                // if we are doing a left join, take the entity id from the resultset
                entityIdSet.insert(statRow.entityId);
            }
        } // while there are rows in resultSet
    } // while there are resultSets

    // sort existing rows in case there aren't secondary categories
    eastl::sort(mRowList.begin(), mRowList.end(), EidCompare());

    return ERR_OK;
}

/*! ***********************************************************************************************/
/*! \brief get any and all stats in secondary categories

    \param[in] request - the request TDF
    \param[in] entityIdSet - entities retrieved from the primary fetch (never empty because it's checked by the caller)
    \param[in] doOuterJoin - whether or not to include secondary stats with no corresponding primary stats

    \return ERR_OK if successful, otherwise some error code
***************************************************************************************************/
BlazeRpcError GetStatsByGroupBase::buildSecondaryResults(const GetStatsByGroupRequest& request, 
    const EntityIdSet& entityIdSet, bool doOuterJoin)
{
    const ScopeNameValueMap &groupScopeMap = getGroupScopeMap();
    const EA::TDF::ObjectType entityType = mGroup->getStatCategory()->getCategoryEntityType();

    // scope map that contains the intersecting scopes
    ScopeNameValueMap commonScopeMap;
    commonScopeMap.reserve(groupScopeMap.size());

    // scope indices contains the group scope indices of intersecting scopes
    ScopeIndexList scopeIndices;
    scopeIndices.reserve(groupScopeMap.size());

    // stores keyscope values used when searching for a keyscope row
    ScopeList scopeValues(groupScopeMap.size());

    // build and process all secondary category fetches
    // NOTE: begin() + 1 is always valid because selList is *never* empty(we always add at least 1 item)
    for (StatGroup::SelectList::const_iterator si = mGroup->getSelectList().begin() + 1, se = mGroup->getSelectList().end(); si != se; ++si)
    {
        const StatGroup::Select& sel = *si;
        const ScopeNameValueMap* scopeMap = sel.mScopes;
        if (scopeMap != nullptr)
        {
            commonScopeMap.clear();
            scopeIndices.clear();
            // only scopes that overlap with group's scopes are added to the common map, because 
            // their values will be needed to merge the secondary rows with their primary brethren.
            ScopeNameValueMap::const_iterator end = scopeMap->end();
            int32_t scopeIndex = 0;
            ScopeNameValueMap::const_iterator groupScopeItr = groupScopeMap.begin();
            ScopeNameValueMap::const_iterator groupScopeEnd = groupScopeMap.end();
            for (; groupScopeItr != groupScopeEnd; ++groupScopeItr, ++scopeIndex)
            {
                if (scopeMap->find(groupScopeItr->first) != end)
                {
                    commonScopeMap[groupScopeItr->first] = groupScopeItr->second;
                    scopeIndices.push_back(scopeIndex);
                }
            }
        }

        // setup stat provider parameters for secondary fetch
        if (sel.mDescs.empty())
        {
            // NOTE: this is just for safety, should never be hit, because it's checked by the caller 
            ERR_LOG("[GetStatsByGroupBase].buildSecondaryResults: group [" << mGroup->getName()
                << "] category [" << sel.mCategory->getName() << "] has no stats.");
            return ERR_SYSTEM;
        }

        // if periodType is specified in stats config,
        // it takes priority over requested period
        int32_t periodType = sel.getPeriodType();
        if (periodType == -1)
            periodType = request.getPeriodType();

        int32_t periodId = 0;
        int32_t periodCount = 1;
        if (periodType != STAT_PERIOD_ALL_TIME)
        {
            if (periodType == request.getPeriodType())
            {
                if(request.getPeriodOffset() < 0 || mPeriodIds[periodType] < request.getPeriodOffset())
                {
                    ERR_LOG("[GetStatsByGroupBase].buildSecondaryResults(): bad period offset value");
                    return STATS_ERR_BAD_PERIOD_OFFSET;
                }
                periodId = getOffsetPeriodId(periodType, request);
                // more than 1 period may have been requested
                periodCount = request.getPeriodCtr();
            }
            else
            {
                // use current period
                periodId = mPeriodIds[periodType];
            }
        }

        // build keyscopes for secondary fetch
        ScopeNameValueMap secondaryScopeMap;
        if (scopeMap != nullptr)
        {
            ScopeNameValueMap::const_iterator selectScopeItr = scopeMap->begin();
            ScopeNameValueMap::const_iterator selectScopeEnd = scopeMap->end();
            for (; selectScopeItr != selectScopeEnd; ++selectScopeItr)
            {
                if (selectScopeItr->second == KEY_SCOPE_VALUE_ALL)
                {
                    continue;
                }

                ScopeValue scopeValue;
                if (selectScopeItr->second == KEY_SCOPE_VALUE_USER)
                {
                    ScopeNameValueMap::const_iterator requestScopeItr = request.getKeyScopeNameValueMap().find(selectScopeItr->first);
                    if (requestScopeItr == request.getKeyScopeNameValueMap().end())
                    {
                        // NOTE: This is just for safety but should *never* be hit,
                        // because primary category build step always checks it.
                        ERR_LOG("[GetStatsByGroupBase].buildSecondaryResults: request missing secondary scope ["
                            << selectScopeItr->first.c_str() << "] in group [" << mGroup->getName() << "]");
                        return STATS_ERR_BAD_SCOPE_INFO;
                    }

                    if (requestScopeItr->second == KEY_SCOPE_VALUE_ALL)
                    {
                        continue;
                    }

                    scopeValue = requestScopeItr->second;
                }
                else if (selectScopeItr->second == KEY_SCOPE_VALUE_AGGREGATE)
                {
                    // validation of aggregateKey-enabled should have been done on server startup
                    scopeValue = mComponent->getConfigData()->getAggregateKeyValue(selectScopeItr->first.c_str());
                }
                else
                {
                    scopeValue = selectScopeItr->second;
                }

                secondaryScopeMap.insert(eastl::make_pair(selectScopeItr->first, scopeValue));
            }
        }

        StatsProviderPtr statsProvider;

        if (mComponent->getStatsCache().isLruEnabled())
        {
            statsProvider = StatsProviderPtr(BLAZE_NEW StatsCacheProvider(*mComponent, mComponent->getStatsCache()));
        }
        else
        {
            statsProvider = StatsProviderPtr(BLAZE_NEW DbStatsProvider(*mComponent));
        }

        EntityIdList entityIdList;
        entityIdList.reserve(entityIdSet.size());
        for (EntityIdSet::const_iterator eidItr = entityIdSet.begin(); eidItr != entityIdSet.end(); ++eidItr)
            entityIdList.push_back(*eidItr);

        BlazeRpcError err = statsProvider->executeFetchRequest(
            *sel.mCategory,
            entityIdList,
            periodType,
            periodId,
            periodCount,
            secondaryScopeMap);
        if (err != Blaze::ERR_OK)
        {
            return ERR_SYSTEM;
        }

        // how many rows did we get back?
        size_t totalRows = 0;
        statsProvider->reset();
        while (statsProvider->hasNextResultSet())
        {
            StatsProviderResultSet* resultSet = statsProvider->getNextResultSet();
            if (resultSet != nullptr)
            {
                totalRows += resultSet->size();
            }
        }

        if (totalRows == 0)
        {
            // this category returned no rows, its stat values will be default in the final response
            TRACE_LOG("[GetStatsByGroupBase].buildSecondaryResults: no secondary results for group ["
                << mGroup->getName() << "] category [" << sel.mCategory->getName() << "], using defaults.");
            continue;
        }

        if (commonScopeMap.empty())
        {
            // sort existing rows since we will be performing lookups on them to do the merge
            // NOTE: use simpler comparator when no scopes or scopes do not intersect
            eastl::sort(mRowList.begin(), mRowList.end(), EidCompare());
        }
        else
        {
            // sort existing rows since we will be performing lookups on them to do the merge
            // NOTE: use scope specific comparator when scopes intersect
            eastl::sort(mRowList.begin(), mRowList.end(), EidScopeCompare(&scopeIndices));
        }

        // now that the rows are sorted, perform lookup and merging
        statsProvider->reset();
        while (statsProvider->hasNextResultSet())
        {
            StatsProviderResultSet* resultSet = statsProvider->getNextResultSet();
            if (resultSet == nullptr || resultSet->size() == 0)
            {
                continue;
            }
            resultSet->reset();
            while (resultSet->hasNextRow())
            {
                StatsProviderRowPtr row = resultSet->getNextRow();

                StatRow statRow;
                statRow.entityId = row->getEntityId();
                eastl::pair<RowList::iterator, RowList::iterator> pair;
                if (commonScopeMap.empty())
                {
                    statRow.scopes = nullptr; // not necessary, but nice to avoid unused garbage pointers
                    pair = eastl::equal_range(mRowList.begin(), mRowList.end(), statRow, EidCompare());
                }
                else
                {
                    // NOTE: scopeIndices.size() == commonScopeMap.size() is always true!
                    ScopeIndexList::const_iterator index = scopeIndices.begin();
                    for (ScopeNameValueMap::const_iterator 
                        ci = commonScopeMap.begin(),
                        ce = commonScopeMap.end(); ci != ce; ++ci, ++index)
                    {
                        // sets only values @ indices that will be used by EidScopeCompare
                        scopeValues[*index] = row->getValueInt64(ci->first.c_str());
                    }
                    // point the statRow.scopes to the scope data being used when searching
                    statRow.scopes = scopeValues.data();
                    pair = eastl::equal_range(mRowList.begin(), mRowList.end(), statRow, EidScopeCompare(&scopeIndices));
                }

                if (pair.first != pair.second)
                {
                    // found some matching rows, need to update stat values from stat provider
                    do {
                        StatRow& statRowTemp = *pair.first;
                        // process each returned stat column in the secondary result set
                        for (StatGroup::DescList::const_iterator i = sel.mDescs.begin(), e = sel.mDescs.end(); i != e; ++i)
                        {
                            const StatDesc& desc = **i;
                            const Stat& stat = *desc.getStat();
                            formatStat(mAsStr, sizeof(mAsStr), stat, row);
                            // write the secondary stat value directly to the index of the stat desc
                            statRowTemp.stats->getStatValues()[desc.getIndex()] = mAsStr;
                        }
                    } while (++pair.first != pair.second);
                }
                else if (doOuterJoin)
                {
                    // no matching rows found, and we are doing an outer join, 
                    // so we need to insert a new row with default values
                    if (!groupScopeMap.empty())
                    {
                        // reserve enough space for keyscope results for a single row
                        ScopeValue* scope = allocateScopeBlock(groupScopeMap.size());
                        statRow.scopes = scope;
                        for (ScopeNameValueMap::const_iterator i = groupScopeMap.begin(), e = groupScopeMap.end(); i != e; ++i)
                        {
                            ScopeValue scopeValue;
                            ScopeNameValueMap::const_iterator r;
                            if (commonScopeMap.find(i->first) != commonScopeMap.end())
                            {
                                // if common scope, grab it from the stat provider
                                scopeValue = row->getValueInt64(i->first.c_str());
                            }
                            else if ((r = request.getKeyScopeNameValueMap().find(i->first)) != request.getKeyScopeNameValueMap().end())
                            {
                                // this means the scope has been passed in the request
                                scopeValue = r->second;
                            }
                            else
                            {
                                // this means the scope is defined in the group's config
                                scopeValue = i->second;
                            }
                            // fill in the scope value
                            *scope++ = scopeValue;
                        }
                    }

                    statRow.stats = BLAZE_NEW EntityStats();
                    statRow.stats->setEntityId(statRow.entityId);
                    statRow.stats->setEntityType(entityType);
                    // this "mPeriodIds" is our copy of the current period ids when the command started
                    if (periodType != STAT_PERIOD_ALL_TIME)
                    {
                        statRow.stats->setPeriodOffset(mPeriodIds[periodType] - row->getPeriodId());
                    }
                    // set the # of stats to the total defined by this stat group
                    statRow.stats->getStatValues().resize(mGroup->getStats().size());

                    // set all stat values to point to default values for each particular stat
                    for (size_t i = 0, size = mDefaultStatList.size(); i < size; ++i)
                    {
                        // NOTE: does not create a new string, just points to the values in the mDefaultStatList
                        statRow.stats->getStatValues()[i].assignBuffer(mDefaultStatList[i]);
                    }

                    // iterate only the group stat descs from the current category
                    for (StatGroup::DescList::const_iterator i = sel.mDescs.begin(), e = sel.mDescs.end(); i != e; ++i)
                    {
                        const StatDesc& desc = **i;
                        const Stat& stat = *desc.getStat();
                        formatStat(mAsStr, sizeof(mAsStr), stat, row);
                        // write the primary stat value directly to the index of the stat desc
                        statRow.stats->getStatValues()[desc.getIndex()] = mAsStr;
                    }

                    // push the build up stat row at the end of the list
                    // NOTE: this is fine as we resort the rows for each secondary category
                    mRowList.push_back(statRow);
                }
            } // while there are rows in resultSet
        } // while there are resultSets
    }
    return ERR_OK;
}

void GetStatsByGroupBase::buildDefaultStatList()
{
    // build a row of empty stats
    mDefaultStatList.reserve(mGroup->getStats().size());
    for (StatDescList::const_iterator i = mGroup->getStats().begin(), e = mGroup->getStats().end(); i != e; ++i)
    {
        const char8_t* nextDefaultValAsStr = (*i)->getStat()->getDefaultValAsString();
        mDefaultStatList.push_back(nextDefaultValAsStr);
    }
}

void GetStatsByGroupBase::buildResponse(GetStatsResponse& response)
{
    const ScopeNameValueMap &groupScopeMap = getGroupScopeMap();
    if (groupScopeMap.empty())
    {
        // if no group keyscopes the convention is to use a single dummy keyscope value
        StatValues& statValues = *response.getKeyScopeStatsValueMap().allocate_element();
        response.getKeyScopeStatsValueMap()[SCOPE_NAME_NONE] = &statValues;
        statValues.getEntityStatsList().reserve(mRowList.size());
        // now use mRowList that we built up to construct the response
        for (RowList::iterator ri = mRowList.begin(), re = mRowList.end(); ri != re; ++ri)
        {
            // this is where the ownership of EntityStats is transferred from StatRow to response
            statValues.getEntityStatsList().push_back(ri->stats);
        }
    }
    else
    {
        ScopeNameValueMap nameValueMap;
        nameValueMap.reserve(groupScopeMap.size());
        // initialize the scope name value map used to generate string scope keys in the response
        for (ScopeNameValueMap::const_iterator i = groupScopeMap.begin(), e = groupScopeMap.end(); i != e; ++i)
        {
            nameValueMap[i->first.c_str()] = 0;
        }
        // now use mRowList that we built up to construct the response
        for (RowList::iterator ri = mRowList.begin(), re = mRowList.end(); ri != re; ++ri)
        {
            int32_t index = 0;
            for (ScopeNameValueMap::iterator i = nameValueMap.begin(), e = nameValueMap.end(); i != e; ++i, ++index)
            {
                // set the scope value from the cached information in the row, note that the order
                // of scopes in the StatRow object is *always* identical to that in nameValueMap.
                i->second = ri->scopes[index];
            }
            mTempKey[0] = '\0';
            // generate map key for this scope value combination for this row
            genStatValueMapKeyForUnitMap(nameValueMap, mTempKey, sizeof(mTempKey));
            // check for the mapkey in the response, use if found, create new one if not found
            eastl::pair<KeyScopeStatsValueMap::iterator, bool> inserter = 
                response.getKeyScopeStatsValueMap().insert(eastl::make_pair(mTempKey, (StatValues*)nullptr));
            if (inserter.second)
            {
                // if inserted successfully, allocate a new element
                inserter.first->second = response.getKeyScopeStatsValueMap().allocate_element();
            }
            StatValues& statValues = *inserter.first->second;
            // this is where the ownership of EntityStats is transferred from StatRow to response
            statValues.getEntityStatsList().push_back(ri->stats);
        }
    }
    // NOTE: Since response takes ownership of EntityStats objects,
    // we must clear the row list to prevent the GetStatsByGroupBase::dtor
    // from trying to clean them up.
    mRowList.clear();
}

ScopeValue* GetStatsByGroupBase::allocateScopeBlock(size_t scopeCount)
{
    if (scopeCount < 1)
        return nullptr;
        
    // scope values follow immediately after the ScopeBlock header
    ScopeBlock* block = (ScopeBlock*) BLAZE_ALLOC_NAMED(sizeof(ScopeBlock)+sizeof(ScopeValue)*scopeCount, "ScopeBlock");

    // link header into the list used to free the blocks later
    block->next = mScopeBlockHead;
    mScopeBlockHead = block;

    // scope values begin right after the ScopeBlock structure   
    return reinterpret_cast<ScopeValue*>(block + 1);
}

void GetStatsByGroupBase::formatStat(char8_t* buf, size_t len, const Stat& stat, const StatsProviderRowPtr& row)
{
    if (stat.getDbType().getType() == STAT_TYPE_INT)
    {
        if (stat.getTypeSize() == 8)
            blaze_snzprintf(buf, len, stat.getStatFormat(), row->getValueInt64(stat.getName()));
        else
            blaze_snzprintf(buf, len, stat.getStatFormat(), row->getValueInt(stat.getName()));
    }
    else if (stat.getDbType().getType() == STAT_TYPE_FLOAT)
        blaze_snzprintf(buf, len, stat.getStatFormat(), (double_t) row->getValueFloat(stat.getName()));
    else if (stat.getDbType().getType() == STAT_TYPE_STRING)
        blaze_snzprintf(buf, len, stat.getStatFormat(), row->getValueString(stat.getName()));
    else
        buf[0] = '\0';
}

const ScopeNameValueMap& GetStatsByGroupBase::getGroupScopeMap() const
{
    const ScopeNameValueMap* scopeMap = mGroup->getScopeNameValueMap();
    return (scopeMap == nullptr) ? mEmptyScopeMap : *scopeMap;
}

int32_t GetStatsByGroupBase::getOffsetPeriodId(int32_t periodType, const GetStatsByGroupRequest& request) const
{
    int32_t reqPeriodId = 0;
    reqPeriodId = request.getPeriodId();

    if (reqPeriodId == 0)
    {
        int32_t t = request.getTime();
        if (t == 0)
        {
            reqPeriodId = mPeriodIds[periodType] - request.getPeriodOffset();
        }
        else
        {
            reqPeriodId = mComponent->getPeriodIdForTime(t, static_cast<StatPeriodType>(periodType));
        }
    }

    return reqPeriodId;
}

bool GetStatsByGroupBase::EidCompare::operator()(const StatRow& a, const StatRow& b)
{
    return a.entityId < b.entityId;
}

bool GetStatsByGroupBase::EidScopeCompare::operator()(const StatRow& a, const StatRow& b)
{
    if (a.entityId < b.entityId)
        return true;
    if (a.entityId > b.entityId)
        return false;
    // scan and compare defined scopes using specified scope indices
    for (ScopeIndexList::const_iterator i = mScopeIndices->begin(), e = mScopeIndices->end(); i != e; ++i)
    {
        ScopeValue ascope = a.scopes[*i];
        ScopeValue bscope = b.scopes[*i];
        if (ascope < bscope)
            return true;
        if (ascope > bscope)
            return false;
    }
    return false;
}

} // Stats
} // Blaze
