/*************************************************************************************************/
/*!
    \file   leaderboardhelper.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class LeaderboardHelper

    Retrieves ranked stats.  The parameters for finding which stats are abstracted.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/usersessions/usersessionmanager.h"
#include "rollover.h"

#include "stats/statsconfig.h"
#include "stats/statsslaveimpl.h"
#include "stats/tdf/stats.h"

#include "stats/leaderboardhelper.h"
#include "stats/leaderboarddata.h"
#include "stats/statscacheprovider.h"

namespace Blaze
{
namespace Stats
{

struct DbRowCompareFull
{
    DbRowCompareFull(const StatLeaderboard *leaderboard): stats(leaderboard->getRankingStats()), statsSize(leaderboard->getRankingStats().size()) {  }
    DbRowCompareFull(const DbRowCompareFull& other): stats(other.stats), statsSize(other.statsSize) {  }


    const RankingStatList& stats;
    const int32_t statsSize;
    bool operator()(const DbRow* a, const DbRow* b) const
    {
        //check all the stats in order:
        for( int32_t pos = 0; pos < statsSize; ++pos )
        {
            const RankingStatData* curStat = stats.at(pos);
            const char* statName = curStat->getStatName();
            bool ascending = curStat->getAscending();
            if (curStat->getIsInt())
            {
                int64_t valA = a->getInt64(statName);
                int64_t valB = b->getInt64(statName);
                if (valA < valB)
                    return ascending;
                if (valA > valB)
                    return !ascending;
            }
            else
            {
                float_t valA = a->getFloat(statName);
                float_t valB = b->getFloat(statName);
                if (valA < valB)
                    return ascending;
                if (valA > valB)
                    return !ascending;
            }
        }

        // Last check, everything else matches, use Entity id:
        return (a->getUInt64("entity_id") > b->getUInt64("entity_id"));
    }
private:
    DbRowCompareFull& operator=(const DbRowCompareFull&);
};

struct DbRowCompareAscInt
{
    DbRowCompareAscInt(const char8_t* statName) : mStatName(statName) {}

    const char8_t* mStatName;

    bool operator()(const DbRow* a, const DbRow* b) const
    {
        if (a->getInt64(mStatName) < b->getInt64(mStatName))
            return true;
        if ((a->getInt64(mStatName) == b->getInt64(mStatName)) && (a->getUInt64("entity_id") > b->getUInt64("entity_id")))
            return true;
        return false;
    }
};

struct DbRowCompareDescInt
{
    DbRowCompareDescInt(const char8_t* statName) : mStatName(statName) {}

    const char8_t* mStatName;

    bool operator()(const DbRow* a, const DbRow* b) const
    {
        if (a->getInt64(mStatName) > b->getInt64(mStatName))
            return true;
        if ((a->getInt64(mStatName) == b->getInt64(mStatName)) && (a->getUInt64("entity_id") > b->getUInt64("entity_id")))
            return true;
        return false;
    }
};

struct DbRowCompareAscFloat
{
    DbRowCompareAscFloat(const char8_t* statName) : mStatName(statName) {}

    const char8_t* mStatName;

    bool operator()(const DbRow* a, const DbRow* b) const
    {
        if (a->getFloat(mStatName) < b->getFloat(mStatName))
            return true;
        if ((a->getFloat(mStatName) == b->getFloat(mStatName)) && (a->getUInt64("entity_id") > b->getUInt64("entity_id")))
            return true;
        return false;
    }
};

struct DbRowCompareDescFloat
{
    DbRowCompareDescFloat(const char8_t* statName) : mStatName(statName) {}

    const char8_t* mStatName;

    bool operator()(const DbRow* a, const DbRow* b) const
    {
        if (a->getFloat(mStatName) > b->getFloat(mStatName))
            return true;
        if ((a->getFloat(mStatName) == b->getFloat(mStatName)) && (a->getUInt64("entity_id") > b->getUInt64("entity_id")))
            return true;
        return false;
    }
};

/*************************************************************************************************/
/*!
    \brief LeaderboardRowCompareAscInt

    Compare function to sort leaderboard rows in an ascending order

    \param[in]  a - LeaderboardStatValuesRow*
    \param[in]  b - LeaderboardStatValuesRow*

    \return - bool
*/
/*************************************************************************************************/
struct LeaderboardRowCompareAscInt : public eastl::binary_function<LeaderboardStatValuesRow*, LeaderboardStatValuesRow*, bool>
{
    bool operator()(const LeaderboardStatValuesRow* a, const LeaderboardStatValuesRow* b) const
    {
        if (a->getRankedRawStat().getIntValue() < b->getRankedRawStat().getIntValue())
            return true;
        if (a->getRankedRawStat().getIntValue() > b->getRankedRawStat().getIntValue())
            return false;

        //in leaderboarddata.h DEFINE_GET_RANK_BY_ENTRY, the higher entity gets the higher 
        // rank when tied regardless of ASC or DESC leaderboard
        return a->getUser().getBlazeId() > b->getUser().getBlazeId();
    }
};

/*************************************************************************************************/
/*!
    \brief LeaderboardRowCompareAscFloat

    Compare function to sort leaderboard rows in an ascending order

    \param[in]  a - LeaderboardStatValuesRow*
    \param[in]  b - LeaderboardStatValuesRow*

    \return - bool
*/
/*************************************************************************************************/
struct LeaderboardRowCompareAscFloat : public eastl::binary_function<LeaderboardStatValuesRow*, LeaderboardStatValuesRow*, bool>
{
    bool operator()(const LeaderboardStatValuesRow* a, const LeaderboardStatValuesRow* b) const
    {
        if (a->getRankedRawStat().getFloatValue() < b->getRankedRawStat().getFloatValue())
            return true;
        if (a->getRankedRawStat().getFloatValue() > b->getRankedRawStat().getFloatValue())
            return false;

        //in leaderboarddata.h DEFINE_GET_RANK_BY_ENTRY, the higher entity gets the higher
        // rank when tied regardless of ASC or DESC leaderboard
        return a->getUser().getBlazeId() > b->getUser().getBlazeId();
    }
};

/*************************************************************************************************/
/*!
    \brief LeaderboardRowCompareDescInt

    Compare function to sort leaderboard rows in a decending order

    \param[in]  a - LeaderboardStatValuesRow*
    \param[in]  b - LeaderboardStatValuesRow*

    \return - bool
*/
/*************************************************************************************************/
struct LeaderboardRowCompareDescInt : public eastl::binary_function<LeaderboardStatValuesRow*, LeaderboardStatValuesRow*, bool>
{
    bool operator()(const LeaderboardStatValuesRow* a, const LeaderboardStatValuesRow* b) const
    {
        if (a->getRankedRawStat().getIntValue() > b->getRankedRawStat().getIntValue())
            return true;
        if (a->getRankedRawStat().getIntValue() < b->getRankedRawStat().getIntValue())
            return false;

        //in leaderboarddata.h DEFINE_GET_RANK_BY_ENTRY, the higher entity gets the higher
        // rank when tied regardless of ASC or DESC leaderboard
        return a->getUser().getBlazeId() > b->getUser().getBlazeId();
    }
};

/*************************************************************************************************/
/*!
    \brief LeaderboardRowCompareDescFloat

    Compare function to sort leaderboard rows in a decending order

    \param[in]  a - LeaderboardStatValuesRow*
    \param[in]  b - LeaderboardStatValuesRow*

    \return - bool
*/
/*************************************************************************************************/
struct LeaderboardRowCompareDescFloat : public eastl::binary_function<LeaderboardStatValuesRow*, LeaderboardStatValuesRow*, bool>
{
    bool operator()(const LeaderboardStatValuesRow* a, const LeaderboardStatValuesRow* b) const
    {
        if (a->getRankedRawStat().getFloatValue() > b->getRankedRawStat().getFloatValue())
            return true;
        if (a->getRankedRawStat().getFloatValue() < b->getRankedRawStat().getFloatValue())
            return false;

        //in leaderboarddata.h DEFINE_GET_RANK_BY_ENTRY, the higher entity gets the higher
        // rank when tied regardless of ASC or DESC leaderboard
        return a->getUser().getBlazeId() > b->getUser().getBlazeId();
    }
};


/*************************************************************************************************/
/*!
    \brief LeaderboardRowCompareFull

    Compare function to sort leaderboard rows in an ascending order

    \param[in]  a - LeaderboardStatValuesRow*
    \param[in]  b - LeaderboardStatValuesRow*

    \return - bool
*/
/*************************************************************************************************/
struct LeaderboardRowCompareFull : public eastl::binary_function<LeaderboardStatValuesRow*, LeaderboardStatValuesRow*, bool>
{
    LeaderboardRowCompareFull& operator=(const LeaderboardRowCompareFull&);
public:
    LeaderboardRowCompareFull(const StatLeaderboard *leaderboard): stats(leaderboard->getRankingStats()), statsSize(leaderboard->getRankingStats().size()) {  }
    LeaderboardRowCompareFull(const LeaderboardRowCompareFull& other) : stats(other.stats), statsSize(other.statsSize) {}
    const RankingStatList& stats;
    const int32_t statsSize;

    bool operator()(const LeaderboardStatValuesRow* a, const LeaderboardStatValuesRow* b) const
    {
        //check all the stats in order:
        for( int32_t pos = 0; pos < statsSize; ++pos )
        {
            const RankingStatData* curStat = stats.at(pos);
            bool ascending = curStat->getAscending();
            if (curStat->getIsInt())
            {
                int64_t valA = a->getOtherRawStats()[pos]->getIntValue();
                int64_t valB = b->getOtherRawStats()[pos]->getIntValue();
                if (valA < valB)
                    return ascending;
                if (valA > valB)
                    return !ascending;
            }
            else
            {
                float_t valA = a->getOtherRawStats()[pos]->getFloatValue();
                float_t valB = b->getOtherRawStats()[pos]->getFloatValue();
                if (valA < valB)
                    return ascending;
                if (valA > valB)
                    return !ascending;
            }
        }

        //in leaderboarddata.h DEFINE_GET_RANK_BY_ENTRY, the higher entity gets the higher 
        // rank when tied regardless of ASC or DESC leaderboard
        return a->getUser().getBlazeId() > b->getUser().getBlazeId();
    }
};

LeaderboardHelper::LeaderboardHelper(StatsSlaveImpl *component, LeaderboardStatValues& values, bool isStatRawValue)
:   mComponent(component),
    mLeaderboard(nullptr),
    mPeriodId(0),
    mValues(values),
    mSorted(true),
    mIsStatRawValue(isStatRawValue)
{
}

LeaderboardHelper::~LeaderboardHelper()
{
}

BlazeRpcError LeaderboardHelper::fetchLeaderboard(int32_t leaderboardId, const char8_t *leaderboardName,
    int32_t periodOffset, int32_t t, int32_t periodId, size_t rowCount, ScopeNameValueMap& scopeNameValueMap, bool includeStatlessEntities /* = true */, bool enforceCutoff /* = false */)
{
    BlazeRpcError error = _fetchLeaderboard(leaderboardId, leaderboardName,
        periodOffset, t, periodId, rowCount, scopeNameValueMap, includeStatlessEntities, enforceCutoff);

    // free any resource ptrs before the fiber forces them free
    mResultMap.clear();

    return error;
}

BlazeRpcError LeaderboardHelper::_fetchLeaderboard(int32_t leaderboardId, const char8_t *leaderboardName,
    int32_t periodOffset, int32_t t, int32_t periodId, size_t rowCount, ScopeNameValueMap& scopeNameValueMap, bool includeStatlessEntities, bool enforceCutoff)
{
    mLeaderboard = mComponent->getConfigData()->getStatLeaderboard(leaderboardId, leaderboardName);
    if (mLeaderboard == nullptr)
    {
        TRACE_LOG("[LeaderboardHelper].fetchLeaderboard() - invalid leaderboard ID: "
            << leaderboardId << "  or Name: " << leaderboardName << ".");
        return STATS_ERR_INVALID_LEADERBOARD_ID;
    }

    mComponent->getPeriodIds(mPeriodIds);
    StatPeriodType periodType = static_cast<StatPeriodType>(mLeaderboard->getPeriodType());
    mPeriodId = 0;
    if (periodType != STAT_PERIOD_ALL_TIME)
    {
        mPeriodId = periodId;
        if (mPeriodId == 0)
        {
            if (t == 0)
            {
                if (periodOffset<0 || mPeriodIds[periodType]<periodOffset)
                {
                    ERR_LOG("[LeaderboardHelper].fetchLeaderboard(): bad period offset value");
                    return STATS_ERR_BAD_PERIOD_OFFSET;
                }

                mPeriodId = mPeriodIds[periodType] - periodOffset;
            }
            else
            {
                mPeriodId = mComponent->getPeriodIdForTime(t, periodType);
            }
        }
    }

    mKeys.reserve(rowCount);

    // Combine leaderboard-defined and user-specified keyscopes in order defined by lb config.
    // Note that we don't care if there are more user-specified keyscopes than needed.
    if (mLeaderboard->getHasScopes())
    {
        const ScopeNameValueListMap* lbScopeNameValueListMap = mLeaderboard->getScopeNameValueListMap();
        ScopeNameValueListMap::const_iterator lbScopeMapItr = lbScopeNameValueListMap->begin();
        ScopeNameValueListMap::const_iterator lbScopeMapEnd = lbScopeNameValueListMap->end();
        for (; lbScopeMapItr != lbScopeMapEnd; ++lbScopeMapItr)
        {
            // any aggregate key indicator has already been replaced with actual aggregate key value

            if (mComponent->getConfigData()->getKeyScopeSingleValue(lbScopeMapItr->second->getKeyScopeValues()) != KEY_SCOPE_VALUE_USER)
            {
                mCombinedScopeNameValueListMap.insert(eastl::make_pair(lbScopeMapItr->first, lbScopeMapItr->second->clone()));
                continue;
            }

            ScopeNameValueMap::const_iterator userScopeMapItr = scopeNameValueMap.find(lbScopeMapItr->first);
            if (userScopeMapItr == scopeNameValueMap.end())
            {
                ERR_LOG("[LeaderboardHelper].fetchLeaderboard() - leaderboard: <" << mLeaderboard->getBoardName() 
                        << ">: scope value for <" << lbScopeMapItr->first.c_str() << "> is missing in request");
                return STATS_ERR_BAD_SCOPE_INFO;
            }

            if (!mComponent->getConfigData()->isValidScopeValue(userScopeMapItr->first, userScopeMapItr->second))
            {
                ERR_LOG("[LeaderboardHelper].fetchLeaderboard() - leaderboard: <" << mLeaderboard->getBoardName() 
                        << ">: scope value for <" << lbScopeMapItr->first.c_str() << "> is not defined");
                return STATS_ERR_BAD_SCOPE_INFO;
            }

            ScopeValues* scopeValues = BLAZE_NEW ScopeValues();
            (scopeValues->getKeyScopeValues())[userScopeMapItr->second] = userScopeMapItr->second;
            mCombinedScopeNameValueListMap.insert(eastl::make_pair(lbScopeMapItr->first, scopeValues));
        }
    }

    BlazeRpcError rc = ERR_OK;

    // start building the TDF response with LeaderboardStatValuesRows for entity ids and ranks
    if (!mLeaderboard->isInMemory())
    {
        rc = lookupDbRows(enforceCutoff);
    }
    else
    {
        // Retrieving in-memory leaderboard
        rc = getIndexEntries();
    }

    if (rc != ERR_OK)
        return rc;

    if (mKeys.empty())
        return ERR_OK;

    // fill in the group stats, starting with the primary category
    rc = buildPrimaryResults(mKeys, includeStatlessEntities, enforceCutoff);
    if (rc != ERR_OK)
        return rc;

    // check keys again because 'includeStatlessEntities' flag could have emptied the response
    if (mKeys.empty())
        return ERR_OK;

    if (!mSorted)
    {
        BlazeStlAllocator sortAllocator("LBSort"); //scratch for the sort
        // need to sort any unranked users, so sort the rows by the ranked stat value
        if (mLeaderboard->hasSecondaryRankings())
        {
            // Full comparison against all stats, including (primary) ranking stat:
            eastl::stable_sort(mValues.getRows().begin(), mValues.getRows().end(), LeaderboardRowCompareFull(mLeaderboard));
        }
        else if (mLeaderboard->isAscending())
        {
            if (mLeaderboard->getStat()->getDbType().getType() == STAT_TYPE_INT)
                eastl::stable_sort(mValues.getRows().begin(), mValues.getRows().end(), sortAllocator, LeaderboardRowCompareAscInt());
            if (mLeaderboard->getStat()->getDbType().getType() == STAT_TYPE_FLOAT)
                eastl::stable_sort(mValues.getRows().begin(), mValues.getRows().end(), sortAllocator, LeaderboardRowCompareAscFloat());
        }
        else
        {
            if (mLeaderboard->getStat()->getDbType().getType() == STAT_TYPE_INT)
                eastl::stable_sort(mValues.getRows().begin(), mValues.getRows().end(), sortAllocator, LeaderboardRowCompareDescInt());
            if (mLeaderboard->getStat()->getDbType().getType() == STAT_TYPE_FLOAT)
                eastl::stable_sort(mValues.getRows().begin(), mValues.getRows().end(), sortAllocator, LeaderboardRowCompareDescFloat());
        }
    }

    const bool hasSecondarySelects = mLeaderboard->getLeaderboardGroup()->getSelectList().size() > 1;
    if (hasSecondarySelects)
    {
        rc = buildSecondaryResults(mKeys);
        if (rc != ERR_OK)
            return rc;
    }

    // use intermediate data stored in provider results to build the rpc response object
    buildResponse(mValues);

    fillOutIdentities();

    return ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief providerStatToString

    Converts stat value from stat provider into string according to format specified in Stat object

    \param[out] buf - buffer for stat value
    \param[in]  bufLen - buffer length
    \param[in]  stat - pointer to Stat object
    \param[in]  row - stat provider row containing stat value

    \return - pointer to the string representation of the stat value
*/
/*************************************************************************************************/
const char8_t* LeaderboardHelper::providerStatToString(char8_t* buf, int32_t bufLen, const Stat* stat, const StatsProviderRowPtr row) const
{
    int32_t type = stat->getDbType().getType();
    const char8_t* format = stat->getStatFormat();
    const char8_t* name = stat->getName();

    if (type == STAT_TYPE_INT)
    {
        if (stat->getTypeSize() == 8)
        {
            blaze_snzprintf(buf, bufLen, format, row->getValueInt64(name));
        }
        else
        {
            blaze_snzprintf(buf, bufLen, format, row->getValueInt(name));
        }
    }
    else if (type == STAT_TYPE_FLOAT)
    {
        blaze_snzprintf(buf, bufLen, format, (double_t) row->getValueFloat(name));
    }
    else // assuming string
    {
        blaze_snzprintf(buf, bufLen, format, row->getValueString(name));
    }

    return buf;
}

/*************************************************************************************************/
/*!
    \brief providerStatToRaw

    Converts stat value from stat provider into string according to format specified in Stat object

    \param[out] statRawVal - output for raw stat value
    \param[in]  stat - pointer to Stat object
    \param[in]  row - stat provider row containing stat value

    \return - reference to the raw stat value`
*/
/*************************************************************************************************/
StatRawValue& LeaderboardHelper::providerStatToRaw(StatRawValue& statRawVal, const Stat* stat, const StatsProviderRowPtr row) const
{
    int32_t type = stat->getDbType().getType();
    const char8_t* name = stat->getName();

    if (type == STAT_TYPE_INT)
    {
        if (stat->getTypeSize() == 8)
        {
            statRawVal.setIntValue(row->getValueInt64(name));
        }
        else
        {
            statRawVal.setIntValue(row->getValueInt(name));
        }
    }
    else if (type == STAT_TYPE_FLOAT)
    {
        statRawVal.setFloatValue((float) row->getValueFloat(name));
    }
    else // assuming string
    {
        statRawVal.setStringValue(row->getValueString(name));
    }

    return statRawVal;
}

/*************************************************************************************************/
/*!
\brief appendScopesToQuery
       Adds scope defined conditions to the query or none if scopes are not defined for the leaderboard
\param[in]  dbQuery - query to append scope conditions to
\return - none
*/
/*************************************************************************************************/
void LeaderboardHelper::appendScopesToQuery(QueryPtr& dbQuery) const
{
    if (mLeaderboard->getHasScopes())
    {
        ScopeNameValueListMap::const_iterator itr = mCombinedScopeNameValueListMap.begin();
        ScopeNameValueListMap::const_iterator end = mCombinedScopeNameValueListMap.end();
        for (; itr != end; ++itr)
        {
            const ScopeValues* scopeValues = itr->second;

            dbQuery->append(" AND `$s` IN (", itr->first.c_str());

            ScopeStartEndValuesMap::const_iterator valuesItr = scopeValues->getKeyScopeValues().begin();
            ScopeStartEndValuesMap::const_iterator valuesEnd = scopeValues->getKeyScopeValues().end();
            for (; valuesItr != valuesEnd; ++valuesItr)
            {
                ScopeValue scopeValue;
                for (scopeValue = valuesItr->first; scopeValue <= valuesItr->second; ++scopeValue)
                {
                    dbQuery->append("$D,", scopeValue);
                }
            }

            dbQuery->trim(1); // remove last comma
            dbQuery->append(")");
        }
    }
}

/*************************************************************************************************/
/*!
\brief appendMinimumConditionToQuery
       If minimum value for ranking stat is defined in configuration file condition for 
       to query is added to filter on minimum value
\param[in]  dbQuery - query to append minimum conditions to
\return - none
*/
/*************************************************************************************************/
void LeaderboardHelper::appendCutoffConditionToQuery(QueryPtr& dbQuery) const
{
    if (mLeaderboard->isCutoffStatValueDefined())
    {
        dbQuery->append(" AND `$s`", mLeaderboard->getStatName());
        if (mLeaderboard->isAscending())
            dbQuery->append(" <=");
        else
            dbQuery->append(" >=");
        
        if (mLeaderboard->getStat()->getDbType().getType() == STAT_TYPE_INT)
            dbQuery->append(" $D",  mLeaderboard->getCutoffStatValueInt());
        else
            dbQuery->append(" $f",  mLeaderboard->getCutoffStatValueFloat());
    }
}

/*************************************************************************************************/
/*!
\brief appendColumnsToQuery
       Add the stats/columns for the primary query -- including entity_id.
       Don't care for the period id and keyscopes in the result.
\param[in]  dbQuery - query to append columns to
\return - none
*/
/*************************************************************************************************/
void LeaderboardHelper::appendColumnsToQuery(QueryPtr& dbQuery) 
{
    const LeaderboardGroup::Select& sel = mLeaderboard->getLeaderboardGroup()->getSelectList().front();

    dbQuery->append("`entity_id`");

    for (LeaderboardGroup::GroupStatList::const_iterator i = sel.mDescs.begin(), e = sel.mDescs.end(); i != e; ++i)
    {
        dbQuery->append(",`$s`", (*i)->getStat()->getName());
    }
}

BlazeRpcError LeaderboardHelper::doDbLookupQuery(uint32_t dbId, const EntityIdList& entityIds, bool enforceCutoff, DbResultPtrVector& dbResultPtrVector)
{
    DbConnPtr conn = gDbScheduler->getReadConnPtr(dbId);
    if (conn == nullptr)
    {
        ERR_LOG("[LeaderboardHelper].doDbLookupQuery: failed to obtain connection.");
        return ERR_SYSTEM;
    }

    QueryPtr query = DB_NEW_QUERY_PTR(conn);

    if (query == nullptr)
    {
        ERR_LOG("[LeaderboardHelper].doDbLookupQuery: failed to create new query.");
        return ERR_SYSTEM;
    }

    // only not-in-memory leaderboards come here
    if (!mLeaderboard->hasSecondaryRankings())
    {
        query->append("SELECT `entity_id`,`$s` FROM `$s` WHERE TRUE",
            mLeaderboard->getStatName(),
            mLeaderboard->getDBTableName());
    }
    else
    {
        query->append("SELECT `entity_id`");
        // Get all ranking stats
        int32_t statsSize = mLeaderboard->getRankingStatsSize();
        for( int32_t pos = 0; pos < statsSize; ++pos )
        {
            query->append(",`$s`", mLeaderboard->getRankingStatName(pos) );
        }
        query->append(" FROM `$s` WHERE TRUE",
            mLeaderboard->getDBTableName());
    }
    

    if (mLeaderboard->getPeriodType() != STAT_PERIOD_ALL_TIME)
    {
        query->append(" AND `period_id` = $d", mPeriodId);
    }

    // append scope string
    appendScopesToQuery(query);

    if (enforceCutoff)
    {
        appendCutoffConditionToQuery(query);
    }

    if (!entityIds.empty())
    {
        query->append(" AND `entity_id` IN (");

        for (EntityIdList::const_iterator itr = entityIds.begin(); itr != entityIds.end(); ++itr)
        {
            query->append("$D,", *itr);
        }

        query->trim(1);
        query->append(")");
    }

    DbResultPtr dbResultPtr;
    BlazeRpcError dbErr = conn->executeQuery(query, dbResultPtr);
    if (dbErr != ERR_OK)
        return dbErr;
    dbResultPtrVector.push_back(dbResultPtr);

    return Blaze::ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief lookupDbRows

    Load board from DB

    \param[in] enforceCutoff - Whether to include entities that do not meet the cutoff value. Enforcing will exclude entities.

    \return - error code
*/
/*************************************************************************************************/
BlazeRpcError LeaderboardHelper::lookupDbRows(bool enforceCutoff)
{
    BlazeRpcError err = ERR_OK;
    EA::TDF::ObjectType entityType = mLeaderboard->getStatCategory()->getCategoryEntityType();

    EntityIdList entities;
    prepareEntitiesForQuery(entities);

    // If a regular or centered leaderboard had a userset which turned out to be empty, or a
    // filtered leaderboard had no entities between the client-specified list combined with the userset
    // then we fast-complete this transaction to avoid wasting time going to the DB
    if (isEmptyQuery(entities))
        return ERR_OK;

    DbResultPtrVector dbResult;

    const ShardConfiguration& shardConfig = mComponent->getConfigData()->getShardConfiguration();
    if (!entities.empty())
    {
        ShardEntityMap shardEntityMap;
        err = shardConfig.lookupDbIds(entityType, entities, shardEntityMap);
        if (err != ERR_OK)
            return err;
        for (ShardEntityMap::const_iterator itr = shardEntityMap.begin(); itr != shardEntityMap.end(); ++itr)
        {
            if (itr->second.empty())
                continue;
            if ((err = doDbLookupQuery(itr->first, itr->second, enforceCutoff, dbResult)) != ERR_OK)
                return err;
        }
    }
    else
    {
        const DbIdList& list = shardConfig.getDbIds(entityType);
        for (DbIdList::const_iterator dbIdItr = list.begin(); dbIdItr != list.end(); ++dbIdItr)
        {
            if ((err = doDbLookupQuery(*dbIdItr, entities, enforceCutoff, dbResult)) != ERR_OK)
                return err;
        }
    }

    // Due to sharding, we have to combine the results from (potentially) multiple databases,
    // and thus do the sorting ourselves here
    DbRowVector dbRowVector;
    for (DbResultPtrVector::const_iterator itr = dbResult.begin(); itr != dbResult.end(); ++itr)
    {
        DbResultPtr result = *itr;
        if (result != nullptr && !result->empty())
        {
            DbResult::const_iterator resItr = result->begin();
            DbResult::const_iterator resEnd = result->end();
            dbRowVector.reserve(result->size());
            for (; resItr != resEnd; ++resItr)
            {
                const DbRow *row = *resItr;
                dbRowVector.push_back(row);
            }
        }
    }

    // Check if there are >1 ranking stats (1 should always be provided)
    if (mLeaderboard->hasSecondaryRankings())
    {
        // Full comparison against all stats, including (primary) ranking stat:
        eastl::sort(dbRowVector.begin(), dbRowVector.end(), DbRowCompareFull(mLeaderboard));
    }
    else if (mLeaderboard->getStat()->getDbType().getType() == STAT_TYPE_INT)
    {
        if (mLeaderboard->isAscending())
        {
            eastl::sort(dbRowVector.begin(), dbRowVector.end(), DbRowCompareAscInt(mLeaderboard->getStatName()));
        }
        else
        {
            eastl::sort(dbRowVector.begin(), dbRowVector.end(), DbRowCompareDescInt(mLeaderboard->getStatName()));
        }
    }
    else
    {
        if (mLeaderboard->isAscending())
        {
            eastl::sort(dbRowVector.begin(), dbRowVector.end(), DbRowCompareAscFloat(mLeaderboard->getStatName()));
        }
        else
        {
            eastl::sort(dbRowVector.begin(), dbRowVector.end(), DbRowCompareDescFloat(mLeaderboard->getStatName()));
        }
    }

    // as dbRowVector has been sorted by rank, we're relying that mKeys will be built in rank-order ...
    previewDbLookupResults(mLeaderboard->getSize(), dbRowVector, mKeys);

    // Getting in-memory LBs (via getIndexEntries) builds the TDF response with LeaderboardStatValuesRows
    // for entity ids and ranks (but the group stats have not been filled in yet).
    // We *expect* previewDbLookupResults() to have created response rows for each element (entity) in mKeys

    return ERR_OK;
}

void LeaderboardHelper::fillOutIdentities()
{
    EA::TDF::ObjectType type = mLeaderboard->getStatCategory()->getCategoryEntityType();
    
    if (!BlazeRpcComponentDb::hasIdentity(type))
        return;

    LeaderboardStatValues::RowList& lbList = mValues.getRows();
    LeaderboardStatValues::RowList::iterator valuesIter = lbList.begin();

    if (type == ENTITY_TYPE_USER)
    {
        BlazeIdToUserInfoMap userInfoMap;
        BlazeRpcError lookupError = gUserSessionManager->lookupUserInfoByBlazeIds(mKeys, userInfoMap);

        if (lookupError != Blaze::ERR_OK)
        {
            ERR_LOG("[LeaderboardHelper].filloutIdentities: "
                "error looking up names from leaderboard (" << mLeaderboard->getBoardName() << ")");
        }

        while (valuesIter != lbList.end())
        {
            if (lookupError == Blaze::ERR_OK)
            {
                BlazeIdToUserInfoMap::const_iterator identItr = userInfoMap.find((*valuesIter)->getUser().getBlazeId());
                if (identItr != userInfoMap.end())
                {
                    UserInfoPtr identityPtr = identItr->second;

                    (*valuesIter)->setAttribute(identityPtr.get()->getUserInfoAttribute());

                    identityPtr->filloutUserCoreIdentification((*valuesIter)->getUser());
                    ++valuesIter;
                }
                else
                {
                    // Only log an error for ranked users, for unranked users it is more than likely that a client
                    // passed in a nucleus id which may never have logged into Blaze
                    if ((*valuesIter)->getRank() != 0)
                    {
                        ERR_LOG("[LeaderboardHelper].filloutIdentities: dropping row from leaderboard ("
                            << mLeaderboard->getBoardName() << "), no name found for " << (*valuesIter)->getUser().getBlazeId());
                    }
                    else
                    {
                        TRACE_LOG("[LeaderboardHelper].filloutIdentities: dropping row from leaderboard ("
                            << mLeaderboard->getBoardName() << "), no name found for " << (*valuesIter)->getUser().getBlazeId());
                    }
                    
                    lbList.erase(valuesIter);
                }
            }
            else
            {
                lbList.erase(valuesIter);
            }
        }
    }
    else
    {
        IdentityInfoByEntityIdMap identityMap;
        BlazeRpcError identityError = gIdentityManager->getIdentities(type, mKeys, identityMap);
        IdentityInfoByEntityIdMap::const_iterator identEnd = identityMap.end();

        if (identityError != Blaze::ERR_OK)
        {
            ERR_LOG("[LeaderboardHelper].filloutIdentities: "
                "error looking up names from leaderboard (" << mLeaderboard->getBoardName() << ")");
        }

        while (valuesIter != lbList.end())
        {
            if (identityError == Blaze::ERR_OK)
            {
                IdentityInfoByEntityIdMap::const_iterator identItr = identityMap.find((*valuesIter)->getUser().getBlazeId());
                if (identItr != identEnd)
                {
                    const IdentityInfo* info = identItr->second;
                    (*valuesIter)->getUser().setName(info->getIdentityName());
                    ++valuesIter;
                }
                else
                {
                    // Unlike users (above), bad entities are more than likely indicative of a real bug
                    ERR_LOG("[LeaderboardHelper].filloutIdentities: dropping row from leaderboard ("
                        << mLeaderboard->getBoardName() << "), no name found for " << (*valuesIter)->getUser().getBlazeId());
                    
                    lbList.erase(valuesIter);
                }
            }
            else
            {
                lbList.erase(valuesIter);
            }
        }
    }
}

/*! ***************************************************************************/
/*! \brief  buildPrimaryResults

    Obtain stats provider rows of given entities for primary category.

    \param  entityList - entities to get
    \param  includeStatlessEntities - if true, then entities that do not have any stats are included in the response
            (redundant in the not-in-memory leaderboard case because we've already checked in lookupDbRows())
    \param  enforceCutoff - if true, then entities that do not 'make the cutoff' are excluded from the response
            and overrides 'includeStatlessEntities' (specifically, 'includeStatlessEntities' is treated as 'false')
            (redundant in the not-in-memory leaderboard case because we've already checked in lookupDbRows())

    \return BlazeRpcError result - ERR_OK on success; error otherwise
*******************************************************************************/
BlazeRpcError LeaderboardHelper::buildPrimaryResults(EntityIdList& entityList, bool includeStatlessEntities, bool enforceCutoff)
{
    // setup stat provider parameters for primary category fetch
    const LeaderboardGroup::Select& sel = mLeaderboard->getLeaderboardGroup()->getSelectList().front();
    if (sel.mDescs.empty())
    {
        ERR_LOG("[LeaderboardHelper].buildPrimaryResults: group [" << mLeaderboard->getLeaderboardGroup()->getGroup()
            << "] category [" << sel.mCategory->getName() << "] has no stats.");
        return ERR_SYSTEM;
    }

    Result& result = mResultMap[&sel];
    if (mComponent->getStatsCache().isLruEnabled())
    {
        result.statsProvider = StatsProviderPtr(BLAZE_NEW StatsCacheProvider(*mComponent, mComponent->getStatsCache()));
    }
    else
    {
        result.statsProvider = StatsProviderPtr(BLAZE_NEW DbStatsProvider(*mComponent));
    }

    BlazeRpcError err = result.statsProvider->executeFetchRequest(
        *sel.mCategory,
        entityList,
        mLeaderboard->getPeriodType(),
        mPeriodId,
        1,
        getGroupScopeMap());
    if (err != Blaze::ERR_OK)
    {
        return ERR_SYSTEM;
    }

    // setup the entity map for this primary result
    size_t totalRows = 0;
    result.statsProvider->reset();
    while (result.statsProvider->hasNextResultSet())
    {
        StatsProviderResultSet* resultSet = result.statsProvider->getNextResultSet();
        if (resultSet == nullptr || resultSet->size() == 0)
        {
            continue;
        }
        resultSet->reset();
        while (resultSet->hasNextRow())
        {
            StatsProviderRowPtr row = resultSet->getNextRow();

            if (enforceCutoff)
            {
                const Stat* stat = mLeaderboard->getStat();
                const RankingStatList& stats = mLeaderboard->getRankingStats();
                int32_t statsSize = stats.size();
                if (statsSize > 1)
                {
                    // Create and fill in the MultiRankData
                    MultiRankData multiVal;
                    for (int32_t pos = 0; pos < statsSize; ++pos)
                    {
                        const RankingStatData* curStat = stats.at(pos);
                        const char* statName = curStat->getStatName();
                        if (curStat->getIsInt())
                            multiVal.data[pos].intValue = row->getValueInt64(statName);
                        else
                            multiVal.data[pos].floatValue = row->getValueFloat(statName);
                    }

                    if ((stats[0]->getIsInt() && mLeaderboard->isIntCutoff(multiVal.data[0].intValue))
                        || (!stats[0]->getIsInt() && mLeaderboard->isFloatCutoff(multiVal.data[0].floatValue)))
                        continue; // didn't make the cutoff
                }
                else if (stat->getDbType().getType() == STAT_TYPE_FLOAT)
                {
                    float_t floatVal = row->getValueFloat(stat->getName());

                    if (mLeaderboard->isFloatCutoff(floatVal))
                        continue; // didn't make the cutoff
                }
                else
                {
                    if (stat->getTypeSize() == 8)
                    {
                        int64_t intVal = row->getValueInt64(stat->getName());

                        if (mLeaderboard->isIntCutoff(intVal))
                            continue; // didn't make the cutoff
                    }
                    else
                    {
                        int32_t intVal = row->getValueInt(stat->getName());

                        if (mLeaderboard->isIntCutoff(intVal))
                            continue; // didn't make the cutoff
                    }
                }
            }// if (enforceCutoff)

            EntityId entityId = row->getEntityId();
            result.entityMap[entityId] = row;

            ++totalRows;
        }
    }

    // how many rows did we get back?
    if (totalRows == 0)
    {
        // this category returned no rows
        if (enforceCutoff || !includeStatlessEntities)
        {
            // fast empty the response
            TRACE_LOG("[LeaderboardHelper].buildPrimaryResults: no primary rows for group ["
                << mLeaderboard->getLeaderboardGroup()->getGroup() << "] category [" << sel.mCategory->getName() << "], empty response.");
            entityList.clear();
            mValues.getRows().clear();
        }
        else
        {
            // its stat values will be default in the final response
            TRACE_LOG("[LeaderboardHelper].buildPrimaryResults: no primary rows for group ["
                << mLeaderboard->getLeaderboardGroup()->getGroup() << "] category [" << sel.mCategory->getName() << "], using defaults.");
        }
    }
    else
    {
        TRACE_LOG("[LeaderboardHelper].buildPrimaryResults: found " << totalRows << " rows in this result");

        if (enforceCutoff || !includeStatlessEntities)
        {
            // check the entity list against the entity map that was just created/built
            EntityIdList::iterator ki = entityList.begin();
            while (ki != entityList.end())
            {
                EntityId entityId = *ki;

                EntityStatRowMap::const_iterator ei = result.entityMap.find(entityId);
                if (ei == result.entityMap.end())
                {
                    ki = entityList.erase(ki);
                    continue;
                }

                ++ki;
            }
        }
    }

    // fill in the ranking stat for the response
    // the raw ranked stat is always set so that we can sort the results by the actual stat values
    // we can't sort by the rank because unranked users have rank 0

    int32_t statsSize = mLeaderboard->getRankingStatsSize();
    for( int32_t pos = 0; pos < statsSize; ++pos )
    {
        // find the Stat:
        const Stat* rankingStat = mLeaderboard->getRankingStat(pos);
        for (LeaderboardStatValues::RowList::iterator ki = mValues.getRows().begin(); ki != mValues.getRows().end(); ++ki)
        {
            LeaderboardStatValuesRow* statRow = *ki;
            EntityId entityId = statRow->getUser().getBlazeId();
            statRow->setIsRawStats(mIsStatRawValue);
            statRow->getOtherStats().reserve(statsSize);

            char8_t statAsStrBuf[STATS_STATVALUE_LENGTH];
            const char8_t* nextStatAsStr = nullptr;// set if using non-raw stat
            StatRawValue& otherRawStatValue = *(statRow->getOtherRawStats().pull_back());// Copy into the other raw stats

            EntityStatRowMap::iterator ri = result.entityMap.find(entityId);
            if (ri != result.entityMap.end())
            {
                StatsProviderRowPtr row = ri->second;

                if (!mIsStatRawValue && (pos == 0))
                {
                    nextStatAsStr = providerStatToString(statAsStrBuf, sizeof(statAsStrBuf), rankingStat, row);
                }
                providerStatToRaw(otherRawStatValue, rankingStat, row);
            }
            else
            {
                // unranked

                if (!mIsStatRawValue && (pos == 0))
                {
                    nextStatAsStr = rankingStat->getDefaultValAsString();
                }
                rankingStat->extractDefaultRawVal(otherRawStatValue);
            }

            // First element is the primary ranking stat
            if (pos == 0) 
            {
                if (!mIsStatRawValue)
                {
                    statRow->setRankedStat(nextStatAsStr);
                }
                otherRawStatValue.copyInto(statRow->getRankedRawStat());
            }
        }
    }

    return ERR_OK;
}

/*! ***************************************************************************/
/*! \brief  buildSecondaryResults

    Obtain stats provider rows of given entities for all secondary categories.

    \param  entityList - entities to get

    \return BlazeRpcError result - ERR_OK on success; error otherwise
*******************************************************************************/
BlazeRpcError LeaderboardHelper::buildSecondaryResults(const EntityIdList& entityList)
{
    // build and process all secondary queries
    // NOTE: begin() + 1 is always valid because selList is *never* empty(we always add at least 1 item)
    const LeaderboardGroup::SelectList& selectList = mLeaderboard->getLeaderboardGroup()->getSelectList();
    for (LeaderboardGroup::SelectList::const_iterator si = selectList.begin() + 1, se = selectList.end(); si != se; ++si)
    {
        // setup stat provider parameters for secondary fetch
        const LeaderboardGroup::Select& sel = *si;

        if (sel.mDescs.empty())
        {
            // NOTE: this is just for safety, should never be hit, because it's checked by the caller
            ERR_LOG("[LeaderboardHelper].buildSecondaryResults: group [" << mLeaderboard->getLeaderboardGroup()->getGroup()
                << "] category [" << sel.mCategory->getName() << "] has no stats.");
            return ERR_SYSTEM;
        }

        // stat periodType takes priority over group periodType
        int32_t periodType = sel.getPeriodType();
        if (periodType == -1)
            periodType = mLeaderboard->getPeriodType();

        int32_t periodId = 0;
        if (periodType != STAT_PERIOD_ALL_TIME)
        {
            if (periodType == mLeaderboard->getPeriodType())
            {
                periodId = mPeriodId;
            }
            else
            {
                // use current period
                periodId = mPeriodIds[periodType];
            }
        }

        // build keyscopes for secondary fetch
        ScopeNameValueListMap secondaryScopeMap;
        const ScopeNameValueListMap& groupScopeMap = getGroupScopeMap();
        if (sel.mCategory->hasScope())
        {
            const ScopeNameValueListMap* scopeMap = sel.mScopes;
            ScopeNameSet::const_iterator snItr = sel.mCategory->getScopeNameSet()->begin();
            ScopeNameSet::const_iterator snEnd = sel.mCategory->getScopeNameSet()->end();
            for (; snItr != snEnd; ++snItr)
            {
                bool useGroupScope = true;
                ScopeNameValueListMap::const_iterator selectScopeItr = nullptr;
                if (scopeMap != nullptr)
                {
                    selectScopeItr = scopeMap->find(*snItr);
                    useGroupScope = (selectScopeItr == scopeMap->end());
                }
                if (useGroupScope)
                {
                    selectScopeItr = groupScopeMap.find(*snItr);
                    if (selectScopeItr == groupScopeMap.end())
                    {
                        // NOTE: this is just for safety, should never be hit, because it's validated during config parsing
                        ERR_LOG("[LeaderboardHelper].buildSecondaryResults: group [" << mLeaderboard->getLeaderboardGroup()->getGroup()
                            << "] category [" << sel.mCategory->getName() << "] is missing keyscope [" << (*snItr).c_str() << "]");
                        return ERR_SYSTEM;
                    }
                }
                secondaryScopeMap.insert(eastl::make_pair(selectScopeItr->first, selectScopeItr->second->clone()));
            }
        }

        Result& result = mResultMap[&sel];
        if (mComponent->getStatsCache().isLruEnabled())
        {
            result.statsProvider = StatsProviderPtr(BLAZE_NEW StatsCacheProvider(*mComponent, mComponent->getStatsCache()));
        }
        else
        {
            result.statsProvider = StatsProviderPtr(BLAZE_NEW DbStatsProvider(*mComponent));
        }

        BlazeRpcError err = result.statsProvider->executeFetchRequest(
            *sel.mCategory,
            entityList,
            periodType,
            periodId,
            1,
            secondaryScopeMap);
        if (err != Blaze::ERR_OK)
        {
            return ERR_SYSTEM;
        }

        // setup the entity map for this secondary result
        size_t totalRows = 0;
        result.statsProvider->reset();
        while (result.statsProvider->hasNextResultSet())
        {
            StatsProviderResultSet* resultSet = result.statsProvider->getNextResultSet();
            if (resultSet == nullptr || resultSet->size() == 0)
            {
                continue;
            }
            resultSet->reset();
            while (resultSet->hasNextRow())
            {
                StatsProviderRowPtr row = resultSet->getNextRow();
                EntityId entityId = row->getEntityId();
                result.entityMap[entityId] = row;

                ++totalRows;
            }
        }

        // how many rows did we get back?
        if (totalRows == 0)
        {
            // this category returned no rows, its stat values will be default in the final response
            TRACE_LOG("[LeaderboardHelper].buildSecondaryResults: no secondary rows for group ["
                << mLeaderboard->getLeaderboardGroup()->getGroup() << "] category [" << sel.mCategory->getName() << "], using defaults.");
        }
        else
        {
            TRACE_LOG("[LeaderboardHelper].buildSecondaryResults: found " << totalRows << " rows in this result");
        }
    }
    return ERR_OK;
}

/*! ***************************************************************************/
/*! \brief  buildResponse

    Put together the TDF response from the stat provider results of the primary and
    secondary fetches.

    @todo build response as (primary and secondary) results are obtained and don't hold on to
    the results (this would be more similar to GetStatsByGroup behavior)

    \param  response - TDF response for leaderboard RPC

    \return none
*******************************************************************************/
void LeaderboardHelper::buildResponse(LeaderboardStatValues& response)
{
    StatRawValue emptyRawValue;

    LeaderboardStatValues::RowList::iterator ki = response.getRows().begin();
    LeaderboardStatValues::RowList::iterator ke = response.getRows().end();
    for (; ki != ke; ++ki)
    {
        LeaderboardStatValuesRow* statRow = *ki;
        EntityId entityId = statRow->getUser().getBlazeId();

        // these were only used for sorting; don't need them anymore...
        if (!mIsStatRawValue)
        {
            emptyRawValue.copyInto(statRow->getRankedRawStat());
        }
        // (even if we requested the raw stats, we can't want to use these because they're in an incorrect order)
        statRow->getOtherRawStats().release();

        LbStatList::const_iterator gsi = mLeaderboard->getLeaderboardGroup()->getStats().begin();
        LbStatList::const_iterator gse = mLeaderboard->getLeaderboardGroup()->getStats().end();
        statRow->getOtherStats().reserve(mLeaderboard->getLeaderboardGroup()->getStats().size());
        for (; gsi != gse; ++gsi)
        {
            const GroupStat* groupStat = *gsi;
            const Stat* stat = groupStat->getStat();
            const LeaderboardGroup::Select* select = mLeaderboard->getLeaderboardGroup()->getSelectMap().find(groupStat)->second;
            const Result& result = mResultMap[select];

            EntityStatRowMap::const_iterator ri = result.entityMap.find(entityId);
            if (ri != result.entityMap.end())
            {
                StatsProviderRowPtr row = ri->second;
                if (mIsStatRawValue)
                {
                    StatRawValue& otherRawStatValue = *(statRow->getOtherRawStats().pull_back());// Copy into the other raw stats
                    providerStatToRaw(otherRawStatValue, stat, row);
                }
                else
                {
                    char8_t statAsStrBuf[STATS_STATVALUE_LENGTH];
                    const char8_t* nextStatAsStr = providerStatToString(statAsStrBuf, sizeof(statAsStrBuf), stat, row);
                    statRow->getOtherStats().push_back(nextStatAsStr);
                }
            }
            else
            {
                if (mIsStatRawValue)
                {
                    StatRawValue& otherRawStatValue = *(statRow->getOtherRawStats().pull_back());// Copy into the other raw stats
                    stat->extractDefaultRawVal(otherRawStatValue);
                }
                else
                {
                    const char8_t* nextStatAsStr = stat->getDefaultValAsString();
                    statRow->getOtherStats().push_back(nextStatAsStr);
                }
            }
        }
    }
}

} // Stats
} // Blaze
