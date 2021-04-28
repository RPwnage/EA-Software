/*************************************************************************************************/
/*!
    \file   leaderboardindex.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class LeaderboardIndexes

    Handles in-meomory leaderboards. The leaderboard indexes have a hierarchical structure partly resembling 
    the structure of db storage to provide more consistent access to the data as illustrated 
    below:
    
    CategoryMap
        |
    PeriodTypeMap
        |
    +class KeyScopeLeaderboards
     KeyScopeLeaderboardMap
        |
    +class HistoricalLeaderboards
     PeriodIdMap
        |
    LeaderboardMap
        |
    LeaderboardIndex
        |
    LeaderboardData

    At the top, CategoryMap and PeriodTypeMap together define the tables from which data is drawn.
    At the next levels, the KeyScopeMap and PeriodIdMap together define a unique combination of
    keys within a table (with the exception of the entity id of course).  The LeaderboardMap
    is a collection of actual leaderboards, keyed by leaderboard id (an optimization would later
    be to key by stat name, as it is theoretically possible to configure multiple leaderboards
    with different ids based on the same stat).  The actual leaderboards contain the ranking
    data and in the future binned ranking data as well.
    
    On server startup, this structure is created on both master and slaves.  The master populates
    its copy directly from the database, and maintains it throughout its lifetime.  The slave
    servers request this data from the masters before they go active.

    For stat update, delete and trimming of historical periods this structure is traversed
    from the top, as multiple leaderboards may be affected.  Fetch requests from a particular
    leaderboard can begin with the pointer to the KeyScopeLeaderboards class which is stored
    in the corresponding StatLeaderboard object.
    
    The LeaderboardData class manages the ranked data for a particular leaderboard.  Other stats
    may be of interest to clients, but those are obtained from the Stats component--whether it comes
    from some cache or must be fetched from DB.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/connection/selector.h"

#include "statsslaveimpl.h"
#include "statsmasterimpl.h"
#include "leaderboardindex.h"
#include "dbstatsprovider.h"

namespace Blaze
{
namespace Stats
{
    static const char8_t* SCOPE_STRING_PREFIX = "_ks";

    // Number of milliseconds to wait before reporting that the lb indexes is still populating
    static const uint32_t POPULATE_TIMEOUT = 5000;

    static const int32_t POPULATE_CHUNK_SIZE = 1000;
    static const int32_t DB_UPDATE_BATCH_SIZE = 100;


    template <typename T>
    LeaderboardIndex<T>::LeaderboardIndex(const StatLeaderboard* leaderboard) :
        LeaderboardIndexBase(leaderboard),
        mLeaderboardData(leaderboard->getSize(), leaderboard->getExtra()) 
    {
        RankingData data;
        const RankingStatList& stats = leaderboard->getRankingStats();
        int32_t statsSize = stats.size();
        data.rankingStatsUsed = (int8_t)statsSize;
        for( int32_t pos = 0; pos < statsSize; ++pos )
        {
            const RankingStatData* curStat = stats.at(pos);
            data.setAscendingIsInt(curStat->getAscending(), curStat->getIsInt(), pos);
        }

        mLeaderboardData.setRankingData( data ); 
    }


/*************************************************************************************************/
/*!
    \brief buildIndexesStructures

    Processes the entire collection of configured leaderboards and organizes them into a
    hierarchical structure.  This organization structure is the same on both the master and slave,
    so this method is called by both on server startup.  The mechanism by which the structures then
    gets populated with data differs on master and slave, so it is up to the component impl to
    then call the appropriate methods to cause the structures to become populated with data (on the
    master this data will come from the DB, on the slave this data will come from the master).
*/
/*************************************************************************************************/
void LeaderboardIndexes::buildIndexesStructures()
{
    INFO_LOG("[LeaderboardIndexes].buildIndexesStructures()");
    StatLeaderboardsMap::const_iterator leaderboardIter = mComponent->getConfigData()->getStatLeaderboardsMap()->begin();
    StatLeaderboardsMap::const_iterator leaderboardEnd = mComponent->getConfigData()->getStatLeaderboardsMap()->end();

    for (; leaderboardIter != leaderboardEnd; ++leaderboardIter)
    {
        StatLeaderboard* leaderboard = leaderboardIter->second;

        if (!leaderboard->isInMemory())
            continue;

        //Check if we've the category yet if not add it
        PeriodTypeMap* periodTypeMap;
        uint32_t categoryId = leaderboard->getStatCategory()->getId();
        CategoryMap::const_iterator catIter = mCategoryMap.find(categoryId);
        if (catIter == mCategoryMap.end())
        {
            periodTypeMap = BLAZE_NEW_STATS_LEADERBOARD PeriodTypeMap;
            mCategoryMap.insert(eastl::make_pair(categoryId, periodTypeMap));
        }
        else
        {
            periodTypeMap = catIter->second;
        }

        // Check if we've the period type yet if not create it
        int32_t periodType = leaderboard->getPeriodType();
        KeyScopeLeaderboards* keyScopeLeaderboards;
        PeriodTypeMap::const_iterator pTypeIter = periodTypeMap->find(periodType);
        if (pTypeIter == periodTypeMap->end())
        {
            keyScopeLeaderboards = BLAZE_NEW_STATS_LEADERBOARD KeyScopeLeaderboards;
            periodTypeMap->insert(eastl::make_pair(periodType, keyScopeLeaderboards));
        }
        else
        {
            keyScopeLeaderboards = pTypeIter->second;
        }

        leaderboard->setKeyScopeLeaderboards(keyScopeLeaderboards);

        int32_t statsSize = leaderboard->getRankingStatsSize();
        for( int32_t pos = 0; pos < statsSize; ++pos )
            keyScopeLeaderboards->addLeaderboardStat(leaderboard->getRankingStatName(pos));
    
        ScopeVector scopeVector;

        if (leaderboard->getHasScopes())
        {
            // We need to iterate through all combinations of user-specified keyscopes ('?').
            // To do that we will use a set of nested loops controlled by 'ScopeVector'.
            /// @todo cleanup: better explanation of what's going on here ...

            // setup 'ScopeVector' -- note that ScopeNameValueListMap keeps scope names in alphabetical order
            const ScopeNameValueListMap* scopeNameValueListMap = leaderboard->getScopeNameValueListMap();
            const KeyScopesMap& keyScopeMap = mComponent->getConfigData()->getKeyScopeMap();

            ScopeNameValueListMap::const_iterator scopeMapItr = scopeNameValueListMap->begin();
            ScopeNameValueListMap::const_iterator scopeMapEnd = scopeNameValueListMap->end();
            scopeVector.reserve(scopeNameValueListMap->size());
            for (; scopeMapItr != scopeMapEnd; ++scopeMapItr)
            {
                ScopeCell& scopeCell = scopeVector.push_back();
                /// @todo cleanup: use ScopeName instead of char* for ScopeCell's "name" ???
                scopeCell.name = scopeMapItr->first.c_str();
                scopeCell.currentValue = 0;
                if (mComponent->getConfigData()->getKeyScopeSingleValue(scopeMapItr->second->getKeyScopeValues()) == KEY_SCOPE_VALUE_USER)
                {
                    // setup this ScopeCell to iterator thru all possible values

                    KeyScopesMap::const_iterator itemIter = keyScopeMap.find(scopeMapItr->first.c_str());
                    // validation that this keyscope item exists has already been done
                    const KeyScopeItem* scopeItem = itemIter->second;

                    if (scopeItem->getKeyScopeValues().empty())
                    {
                        // treat this key scope as *not* user-specified

                        /// @todo cleanup: config should check for this, so deprecate this block if we never reach here
                        WARN_LOG("[LeaderboardIndexes].buildIndexesStructures() too many values for key scope <" 
                                 << scopeCell.name << "> and will be ignored");

                        if (scopeItem->getEnableAggregation())
                        {
                            scopeCell.currentValue = scopeItem->getAggregateKeyValue();
                        }
                    }
                    else
                    {
                        scopeCell.valuesMap.assign(scopeItem->getKeyScopeValues().begin(), scopeItem->getKeyScopeValues().end());

                        // include any aggregate key value (it shouldn't be in the map already)
                        if (scopeItem->getEnableAggregation())
                        {
                            scopeCell.valuesMap[scopeItem->getAggregateKeyValue()] = scopeItem->getAggregateKeyValue();
                            // not going to worry about compacting this map
                        }

                        scopeCell.mapItr = scopeCell.valuesMap.begin();

                        // with earlier empty-check, itr can't be at end here
                        scopeCell.currentValue = scopeCell.mapItr->first;
                    }
                }
                else
                {
                    // note that we're assuming scopeMapItr->second->getKeyScopeValues() can't be empty, and
                    // any aggregate key indicator has already been replaced with actual aggregate key value
                    scopeCell.valuesMap.assign(scopeMapItr->second->getKeyScopeValues().begin(), scopeMapItr->second->getKeyScopeValues().end());

                    scopeCell.currentValue = KEY_SCOPE_VALUE_MULTI;
                }
                
            }

            // loops running through all combinations of user-specified keyscopes ('?') starts here
            bool doneFlag = false;
            ScopeVector::iterator scopeBegin = scopeVector.begin();
            ScopeVector::iterator scopeIter = scopeBegin;
            ScopeVector::iterator scopeEnd = scopeVector.end();
            while (1)
            {
                // load all ranking tables to the maximum period id defined in the configuration 
                // file for this keyscope combination
                buildDetailedIndexesStructures(&scopeVector, leaderboard, keyScopeLeaderboards);
                
                scopeIter = scopeBegin;
                while (1)
                {
                    ScopeCell& scopeCell = *scopeIter;

                    // multi-value keyscope indicator is opposite of user-specified
                    if (scopeCell.currentValue != KEY_SCOPE_VALUE_MULTI)
                    {
                        // try next value
                        ++(scopeCell.currentValue);

                        if (scopeCell.currentValue <= scopeCell.mapItr->second)
                        {
                            // can use next value
                            break;
                        }

                        // try next mapping
                        ++(scopeCell.mapItr);
                        if (scopeCell.mapItr != scopeCell.valuesMap.end())
                        {
                            // can use start of next mapping
                            scopeCell.currentValue = scopeCell.mapItr->first;
                            break;
                        }

                        // wrap around to first value in this key scope
                        scopeCell.mapItr = scopeCell.valuesMap.begin();
                        scopeCell.currentValue = scopeCell.mapItr->first;

                        // and start running through the next key scope
                    } // if user specified

                    ++scopeIter;
                    if (scopeIter == scopeEnd)
                    {
                        doneFlag = true;
                        break;
                    }
                } // while(1)
                if (doneFlag)
                    break;
            } // while(1)

        } // has scopes
        else // no scopes just get one without
        {
            buildDetailedIndexesStructures(&scopeVector, leaderboard, keyScopeLeaderboards);
        }
    }

    initStatTablesForLeaderboards();

    mGaugeBudgetOfAllRankingTables.set(getMemoryBudget());

    INFO_LOG("[LeaderboardIndexes].buildIndexesStructures() done - " << mTotalLeaderboards << " leaderboards created representing "
        << mTotalLeaderboardEntries << " total entries and a memory budget of " << mGaugeBudgetOfAllRankingTables.get() << " bytes");
}

/*************************************************************************************************/
/*!
    \brief buildDetailedIndexesStructures

    Called by the main buildIndexesStructures method which creates the higher-level structures,
    this method then fills in the detailed structures. 

    \param[in]  scopeVector - vector defining scopes for periods to be loaded
    \param[in]  leaderboard - pointer to the StatLeaderboard object
    \param[in]  keyScopeLeaderboards - pointer to KeyScopeLeaderboards

    \return - true on success
*/
/*************************************************************************************************/
void LeaderboardIndexes::buildDetailedIndexesStructures(const ScopeVector* scopeVector,
    const StatLeaderboard* leaderboard, KeyScopeLeaderboards* keyScopeLeaderboards)
{
    using namespace eastl;

    KeyScopeLeaderboardMap* keyScopeLeaderboardMap = keyScopeLeaderboards->getKeyScopeLeaderboardMap();

    bool isMultiValue = false;
    char8_t* keyScopeString = makeKeyScopeString(scopeVector, isMultiValue);
    if (isMultiValue)
    {
        // One or more of these keyscopes have multi-values.
        // As with building the leaderboard structures, use ScopeVector/ScopeCell mechanism
        // to iterate through all combinations of multi-values as single-value.
        ScopeVector scopeVectorForSingle;
        scopeVectorForSingle.reserve(scopeVector->size());

        ScopeVector::const_iterator scopeItr = scopeVector->begin();
        ScopeVector::const_iterator scopeEnd = scopeVector->end();
        for (; scopeItr != scopeEnd; ++scopeItr)
        {
            ScopeCell& scopeCellForSingle = scopeVectorForSingle.push_back();
            scopeCellForSingle.name = (*scopeItr).name;

            if ((*scopeItr).currentValue == KEY_SCOPE_VALUE_MULTI)
            {
                // setup this ScopeCell to iterator thru all possible values
                scopeCellForSingle.valuesMap.assign((*scopeItr).valuesMap.begin(), (*scopeItr).valuesMap.end());
                scopeCellForSingle.mapItr = scopeCellForSingle.valuesMap.begin();
                scopeCellForSingle.currentValue = scopeCellForSingle.mapItr->first;
            }
            else
            {
                scopeCellForSingle.currentValue = (*scopeItr).currentValue;
            }

        }

        // loops running through all combinations of multi-value keyscopes starts here
        bool doneFlag = false;
        ScopeVector::iterator scopeForSingleBegin = scopeVectorForSingle.begin();
        ScopeVector::iterator scopeForSingleItr = scopeForSingleBegin;
        ScopeVector::iterator scopeForSingleEnd = scopeVectorForSingle.end();
        while (1)
        {
            // create a keyscope string of single values  for this keyscope combination
            char8_t* keyScopeForSingleString = makeKeyScopeString(&scopeVectorForSingle);
            // ... and build the single-to-multi-value keyscope map
            list<const char8_t*>* keyScopeStringList;
            KeyScopeSingleMultiValueMap* keyScopeSingleMultiValueMap = keyScopeLeaderboards->getKeyScopeSingleMultiValueMap();
            KeyScopeSingleMultiValueMap::const_iterator smItr = keyScopeSingleMultiValueMap->find(keyScopeForSingleString);
            if (smItr == keyScopeSingleMultiValueMap->end())
            {
                // the "single" entry doesn't exist; so create the list container for the "multi" entries
                keyScopeStringList = BLAZE_NEW_STATS_LEADERBOARD list<const char8_t*>;
                // ... and add to the single-to-multi-value keyscope map
                keyScopeSingleMultiValueMap->insert(eastl::make_pair(blaze_strdup(keyScopeForSingleString), keyScopeStringList));
            }
            else
            {
                // the "single" entry already exists; so get the list of "multi" entries
                keyScopeStringList = smItr->second;
            }

            // ... and add to the list of "multi" entries
            keyScopeStringList->push_back(blaze_strdup(keyScopeString));

            delete[] keyScopeForSingleString;

            // continue looping thru combinations
            scopeForSingleItr = scopeForSingleBegin;
            while (1)
            {
                ScopeCell& scopeCellForSingle = *scopeForSingleItr;

                // if we have a start-to-end values map, then it's multi-value
                if (!scopeCellForSingle.valuesMap.empty())
                {
                    // try next value
                    ++(scopeCellForSingle.currentValue);

                    if (scopeCellForSingle.currentValue <= scopeCellForSingle.mapItr->second)
                    {
                        // can use next value
                        break;
                    }

                    // try next mapping
                    ++(scopeCellForSingle.mapItr);
                    if (scopeCellForSingle.mapItr != scopeCellForSingle.valuesMap.end())
                    {
                        // can use start of next mapping
                        scopeCellForSingle.currentValue = scopeCellForSingle.mapItr->first;
                        break;
                    }

                    // wrap around to first value in this key scope
                    scopeCellForSingle.mapItr = scopeCellForSingle.valuesMap.begin();
                    scopeCellForSingle.currentValue = scopeCellForSingle.mapItr->first;

                    // and start running through the next key scope
                } // if user specified

                ++scopeForSingleItr;
                if (scopeForSingleItr == scopeForSingleEnd)
                {
                    doneFlag = true;
                    break;
                }
            } // while(1)
            if (doneFlag)
                break;
        } // while(1)

    }// if multi-value keyscopes

    HistoricalLeaderboards* historicalLeaderboards;
    KeyScopeLeaderboardMap::const_iterator ksIter = keyScopeLeaderboardMap->find(keyScopeString);
    if (ksIter == keyScopeLeaderboardMap->end())
    {
        historicalLeaderboards = BLAZE_NEW_STATS_LEADERBOARD HistoricalLeaderboards();
        keyScopeLeaderboardMap->insert(eastl::make_pair(blaze_strdup(keyScopeString), historicalLeaderboards));
    }
    else
    {
        historicalLeaderboards = ksIter->second;
    }
    delete[] keyScopeString;

    historicalLeaderboards->getLeaderboardList()->push_back(leaderboard);
    PeriodIdMap* periodIdMap = historicalLeaderboards->getPeriodIdMap();
    int32_t periodType = leaderboard->getPeriodType();
    int32_t currentPeriodId = mComponent->getPeriodId(periodType);
    int32_t startPeriodId = currentPeriodId - mComponent->getRetention(periodType);
    for (int32_t periodId = startPeriodId; periodId <= currentPeriodId; ++periodId)
    {
        LeaderboardMap* leaderboardMap;
        PeriodIdMap::const_iterator periodIdIter = periodIdMap->find(periodId);
        if (periodIdIter == periodIdMap->end())
        {
            leaderboardMap = BLAZE_NEW_STATS_LEADERBOARD LeaderboardMap();
            periodIdMap->insert(eastl::make_pair(periodId, leaderboardMap));
        }
        else
        {
            leaderboardMap = periodIdIter->second;
        }

        LeaderboardIndexBase* leaderboardIndex;
        LeaderboardMap::const_iterator lbIter = leaderboardMap->find(leaderboard->getBoardId());
        if (lbIter == leaderboardMap->end())
        {
            if (leaderboard->hasSecondaryRankings())
                leaderboardIndex = BLAZE_NEW_STATS_LEADERBOARD LeaderboardIndex<MultiRankData>(leaderboard);
            else if (leaderboard->getStat()->getDbType().getType() == STAT_TYPE_FLOAT)
                leaderboardIndex = BLAZE_NEW_STATS_LEADERBOARD LeaderboardIndex<float_t>(leaderboard);
            else
            {
                //If the stat is a 64-bit int use LeaderboardIndex<int64_t>
                if (leaderboard->getStat()->getTypeSize() == 8)
                {
                    leaderboardIndex = BLAZE_NEW_STATS_LEADERBOARD LeaderboardIndex<int64_t>(leaderboard);
                }
                else
                {
                    leaderboardIndex = BLAZE_NEW_STATS_LEADERBOARD LeaderboardIndex<int32_t>(leaderboard);
                }
            }
            leaderboardMap->insert(eastl::make_pair(leaderboard->getBoardId(), leaderboardIndex));

            ++mTotalLeaderboards;
            mTotalLeaderboardEntries += (uint64_t) leaderboard->getSize();
        }
    }
}

/*************************************************************************************************/
/*!
    \brief schedulePopulateTimeout

    Schedule the next timeout to report that the leaderboard indexes is still populating.
*/
/*************************************************************************************************/
void LeaderboardIndexes::schedulePopulateTimeout()
{
    TimeValue timeoutTime(TimeValue::getTimeOfDay().getMicroSeconds() + (POPULATE_TIMEOUT * 1000));
    mPopulateTimerId = gSelector->scheduleTimerCall(timeoutTime, this, 
        &LeaderboardIndexes::executePopulateTimeout,
        "LeaderboardIndexes::executePopulateTimeout");
}

/*************************************************************************************************/
/*!
    \brief executePopulateTimeout

    Report that the leaderboard indexes is still populating, and setup the next time to report.
*/
/*************************************************************************************************/
void LeaderboardIndexes::executePopulateTimeout()
{
    INFO_LOG("[LeaderboardIndexes].executePopulateTimeout: populated " << mCountLeaderboards
        << " of " << mTotalLeaderboards << " leaderboards; currently populating '" << mPopulatingLeaderboardName << "'");
    schedulePopulateTimeout();
}

/*************************************************************************************************/
/*!
    \brief cancelPopulateTimeout

    Stop reporting that the leaderboard indexes is still populating.
*/
/*************************************************************************************************/
void LeaderboardIndexes::cancelPopulateTimeout()
{
    gSelector->cancelTimer(mPopulateTimerId);
    mPopulateTimerId = INVALID_TIMER_ID;
}

/*************************************************************************************************/
/*!
    \brief requestIndexesFromMaster

    A slave only call used during the slave impl onResolve to request a full dump of the current
    state of the indexes from the master.

    \param[in] - statsMaster - handle to the master where data will be fetched from

    \return - true on success, false otherwise
*/
/*************************************************************************************************/
bool LeaderboardIndexes::requestIndexesFromMaster(StatsMaster* statsMaster)
{
    INFO_LOG("[LeaderboardIndexes].requestIndexesFromMaster()");
    uint64_t numRequests = 0;
    uint64_t numEntries = 0;
    schedulePopulateTimeout();

    CategoryMap::const_iterator catIter = mCategoryMap.begin();
    CategoryMap::const_iterator catEnd = mCategoryMap.end();
    for (; catIter != catEnd; ++catIter)
    {
        PeriodTypeMap* periodTypeMap = catIter->second;
        PeriodTypeMap::const_iterator periodTypeIter = periodTypeMap->begin();
        PeriodTypeMap::const_iterator periodTypeEnd = periodTypeMap->end();
        for (; periodTypeIter != periodTypeEnd; ++periodTypeIter)
        {
            int32_t periodType = periodTypeIter->first;
            KeyScopeLeaderboards* keyScopeLeaderboards = periodTypeIter->second;
            KeyScopeLeaderboardMap::const_iterator keyScopeIter = keyScopeLeaderboards->getKeyScopeLeaderboardMap()->begin();
            KeyScopeLeaderboardMap::const_iterator keyScopeEnd = keyScopeLeaderboards->getKeyScopeLeaderboardMap()->end();
            for (; keyScopeIter != keyScopeEnd; ++keyScopeIter)
            {
                HistoricalLeaderboards* historicalLeaderboards = keyScopeIter->second;
                PeriodIdMap::const_iterator periodIdIter = historicalLeaderboards->getPeriodIdMap()->begin();
                PeriodIdMap::const_iterator periodIdEnd = historicalLeaderboards->getPeriodIdMap()->end();
                for (; periodIdIter != periodIdEnd; ++periodIdIter)
                {
                    int32_t periodId = periodIdIter->first;
                    LeaderboardMap* leaderboardMap = periodIdIter->second;
                    LeaderboardMap::const_iterator lbIter = leaderboardMap->begin();
                    LeaderboardMap::const_iterator lbEnd = leaderboardMap->end();
                    for (; lbIter != lbEnd; ++lbIter)
                    {
                        PopulateLeaderboardIndexRequest request;
                        request.setCategoryId(catIter->first);
                        request.setPeriodType(periodType);
                        request.setKeyScopes(keyScopeIter->first);
                        request.setPeriodId(periodId);
                        request.setLeaderboardId(lbIter->first);
                        request.setFirst(1);

                        LeaderboardIndexBase* leaderboardIndex = lbIter->second;
                        mPopulatingLeaderboardName = leaderboardIndex->getLeaderboard()->getBoardName();

                        PopulateLeaderboardIndexData data;
                        ++numRequests;
                        if (statsMaster->populateLeaderboardIndex(request, data) != ERR_OK)
                        {
                            ERR_LOG("[LeaderboardIndexes].requestIndexesFromMaster(): Got error requesting first batch");
                            return false;
                        }

                        request.setFirst(0);

                        uint32_t count = 0;
                        if (data.getData().getData() != nullptr)
                        {
                            count = *(uint32_t *) data.getData().getData();
                        }
                        while (count > 0)
                        {
                            numEntries += (uint64_t) count;
                            if (!leaderboardIndex->populate(data.getData()))
                            {
                                ERR_LOG("[LeaderboardIndexes].requestIndexesFromMaster(): Received unexpected leaderboard data from master");
                                return false;
                            }
                            ++numRequests;
                            if (statsMaster->populateLeaderboardIndex(request, data) != ERR_OK)
                            {
                                ERR_LOG("[LeaderboardIndexes].requestIndexesFromMaster(): Got error requesting subsequent batch");
                                return false;
                            }
                            // get the count after each request
                            count = 0;
                            if (data.getData().getData() != nullptr)
                            {
                                count = *(uint32_t *) data.getData().getData();
                            }
                        }

                        ++mCountLeaderboards;
                    }
                }
            }
        }
    }

    cancelPopulateTimeout();
    INFO_LOG("[LeaderboardIndexes].requestIndexesFromMaster() done - received "
        << numEntries << " entries on " << numRequests << " requests for " << mCountLeaderboards << " leaderboards");
    return true;
}

/*************************************************************************************************/
/*!
    \brief loadData

    A master only call used on server startup to load all data from the DB needed to initially populate
    the lb indexes.  Where leaderboards are defined on multiple stats with the same table
    and key combinations, all ranked stat columns are fetched together to reduce the number
    of round trips to the database.

    \param[in] tableInfo - information about the stats tables corresponding to the configured leaderboards

    \return - ERR_OK or error
*/
/*************************************************************************************************/
BlazeRpcError LeaderboardIndexes::loadData(const TableInfoMap& tableInfo)
{
    BlazeRpcError err = ERR_OK;

    TableInfoMap::const_iterator tableItr = tableInfo.begin();
    TableInfoMap::const_iterator tableEnd = tableInfo.end();
    for(; tableItr != tableEnd; ++tableItr)
    {
        const char8_t* tableName = tableItr->first;
        const TableInfo& table = tableItr->second;

        if (table.statColumns.empty())
            continue;

        if (table.loadFromDbTables)
        {
            err = loadData(mComponent->getDbId(), tableName, table);
            if (err != ERR_OK)
                return err;
        }
        else
        {
            EA::TDF::ObjectType entityType =
                mComponent->getConfigData()->getCategoryMap()->find(table.categoryName)->second->getCategoryEntityType();
            const DbIdList& list = mComponent->getConfigData()->getShardConfiguration().getDbIds(entityType);
            for (DbIdList::const_iterator dbIdItr = list.begin(); dbIdItr != list.end(); ++dbIdItr)
            {
                err = loadData(*dbIdItr, tableName, table);
                if (err != ERR_OK)
                    return err;
            }
        }
    }

    return Blaze::ERR_OK;
}

BlazeRpcError LeaderboardIndexes::loadData(uint32_t dbId, const char8_t* tableName, const TableInfo& table)
{
    DbConnPtr conn = gDbScheduler->getConnPtr(dbId);
    if (conn == nullptr)
    {
        ERR_LOG("[LeaderboardIndexes].loadData(): Failed to connect to database, cannot continue");
        return Blaze::ERR_SYSTEM;
    }
    QueryPtr query = DB_NEW_QUERY_PTR(conn);
    query->append("SELECT `entity_id`");

    if (table.periodType != STAT_PERIOD_ALL_TIME)
        query->append(",`period_id`");

    // Add keyscope columns
    if (table.keyScopeColumns != nullptr)
    {
        ScopeNameSet::const_iterator scopeColIter = table.keyScopeColumns->begin();
        ScopeNameSet::const_iterator scopeColEnd = table.keyScopeColumns->end();
        for(; scopeColIter != scopeColEnd; ++scopeColIter)
        {
            query->append(",`$s`", (*scopeColIter).c_str());
        }
    }

    // Add leaderboard stat key columns
    ColumnSet::const_iterator statColIter = table.statColumns.begin();
    ColumnSet::const_iterator statColEnd = table.statColumns.end();
    for(; statColIter != statColEnd; ++statColIter)
    {
        query->append(",`$s`", (*statColIter).c_str());
    }

    char8_t lbTableName[LB_MAX_TABLE_NAME_LENGTH];
    getLeaderboardTableName(tableName, lbTableName, LB_MAX_TABLE_NAME_LENGTH);

    query->append(" FROM `$s`", (table.loadFromDbTables ? lbTableName : tableName));

    StreamingDbResultPtr result;
    TimeValue startTime = TimeValue::getTimeOfDay();
    DbError err = conn->executeStreamingQuery(query, result);

    if (err != DB_ERR_OK)
    {
        ERR_LOG("[LeaderboardIndexes].loadData() - failed to load data db err: " << ErrorHelp::getErrorName(err));
        return Blaze::ERR_SYSTEM;
    }

    uint32_t totalRows = 0;
    for (DbRow* row = result->next(); row != nullptr; row = result->next())
    {
        ++totalRows;

        char8_t* scopeKey = makeKeyScopeString(table.keyScopeColumns, row);

        TableInfo::LeaderboardList::const_iterator kslbIter = table.leaderboards.begin();
        TableInfo::LeaderboardList::const_iterator kslbEnd = table.leaderboards.end();
        for(; (kslbIter != kslbEnd); ++kslbIter)
        {
            KeyScopeLeaderboards* lb = *kslbIter;
            KeyScopeLeaderboardMap::iterator itr;

            // look for any multi-value keyscopes that this single-value keyscopes affects
            KeyScopeSingleMultiValueMap* keyScopeSingleMultiValueMap = lb->getKeyScopeSingleMultiValueMap();
            KeyScopeSingleMultiValueMap::const_iterator smItr = keyScopeSingleMultiValueMap->find(scopeKey);
            if (smItr != keyScopeSingleMultiValueMap->end())
            {
                eastl::list<const char8_t*>* keyScopeStringList = smItr->second;
                eastl::list<const char8_t*>::const_iterator ksStrItr = keyScopeStringList->begin();
                eastl::list<const char8_t*>::const_iterator ksStrEnd = keyScopeStringList->end();
                for (; ksStrItr != ksStrEnd; ++ksStrItr)
                {
                    itr = lb->getKeyScopeLeaderboardMap()->find(*ksStrItr);
                    if (itr == lb->getKeyScopeLeaderboardMap()->end())
                    {
                        continue; // we dont have any boards with this keyscopes
                    }

                    loadDataRow(tableName, table, lbTableName, scopeKey, itr->second, conn, result, row);
                }
            }

            // now for the single-value keyscopes update
            itr = lb->getKeyScopeLeaderboardMap()->find(scopeKey);
            if (itr == lb->getKeyScopeLeaderboardMap()->end())
            {
                continue;
            }

            loadDataRow(tableName, table, lbTableName, scopeKey, itr->second, conn, result, row);
        }
        delete[] scopeKey;
        delete row;
    }

    TimeValue endTime = TimeValue::getTimeOfDay();
    endTime -= startTime;
    double dt = endTime.getMillis()/(double)1000;

    INFO_LOG("[LeaderboardIndexes].loadData(): Leaderboard indexes population took a total time of " << dt << " seconds.  "
        "Fetched a total of " << totalRows << " rows from the database ");

    return ERR_OK;
}

void LeaderboardIndexes::loadDataRow(const char8_t* tableName, const TableInfo& table, const char8_t* lbTableName, const char8_t* keyscope,
                                     HistoricalLeaderboards* histLb, DbConnPtr conn, StreamingDbResultPtr result, DbRow* row)
{
    EntityId entityId = row->getInt64((uint32_t)0);
    int32_t periodId = 0;
    if (table.periodType != STAT_PERIOD_ALL_TIME)
        periodId = row->getInt((uint32_t)1);

    PeriodIdMap::iterator periodIter = histLb->getPeriodIdMap()->find(periodId);
    if (periodIter == histLb->getPeriodIdMap()->end())
    {
        return;
    }

    LeaderboardMap* leaderboardMap = periodIter->second;
    LeaderboardMap::const_iterator lbIter = leaderboardMap->begin();
    LeaderboardMap::const_iterator lbEnd = leaderboardMap->end();
    for (; (lbIter != lbEnd); ++lbIter)
    {
        LeaderboardIndexBase* leaderboardIndex = lbIter->second;
        const StatLeaderboard* slb = leaderboardIndex->getLeaderboard();
        const Stat* stat = slb->getStat();

        UpdateLeaderboardResult updateLeaderboardResult;
        const RankingStatList& stats = slb->getRankingStats();
        int32_t statsSize = stats.size();
        if (statsSize > 1)
        {
            // Create and fill in the MultiRankData
            MultiRankData multiVal;
            for( int32_t pos = 0; pos < statsSize; ++pos )
            {
                const RankingStatData* curStat = stats.at(pos);
                const char* statName = curStat->getStatName();
                if (curStat->getIsInt())
                    multiVal.data[pos].intValue = row->getInt64(statName);
                else
                    multiVal.data[pos].floatValue = row->getFloat(statName);
            }


            if (( stats[0]->getIsInt() && slb->isIntCutoff(multiVal.data[0].intValue)) ||
                (!stats[0]->getIsInt() && slb->isFloatCutoff(multiVal.data[0].floatValue)))
                continue;

            (static_cast<LeaderboardIndex<MultiRankData>*>(leaderboardIndex))->update(
                entityId, multiVal, updateLeaderboardResult);
        }
        else if (stat->getDbType().getType() == STAT_TYPE_FLOAT)
        {
            float_t floatVal = row->getFloat(stat->getName());
            // If minimum value for the leaderboard is defined, then
            // skip row if ranking stas is less than the value defined in leaderboard
            if (slb->isFloatCutoff(floatVal))
                continue;

            (static_cast<LeaderboardIndex<float_t>*>(leaderboardIndex))->update(
                entityId, floatVal, updateLeaderboardResult);
        }
        else
        {
            //If the stat is a 64-bit int use LeaderboardIndex<int64_t>
            if (stat->getTypeSize() == 8)
            {
                int64_t intVal = row->getInt64(stat->getName());

                if (slb->isIntCutoff(intVal))
                    continue;

                (static_cast<LeaderboardIndex<int64_t>*>(leaderboardIndex))->update(
                    entityId, intVal, updateLeaderboardResult);
            }
            else
            {
                int32_t intVal = row->getInt(stat->getName());
                // If minimum value for the leaderboard is defined, then
                // skip row if ranking stas is less than the value defined in leaderboard
                if (slb->isIntCutoff(intVal))
                    continue;

                (static_cast<LeaderboardIndex<int32_t>*>(leaderboardIndex))->update(
                    entityId, intVal, updateLeaderboardResult);
            }
        }

        // If we need to build the LB tables, we don't know until we've dumped the entire contents of the main stats tables
        // which rows we need to write into the LB tables, so cache off a query to be used later if a user was inserted into
        // a leaderboard, at the same time we may need to purge a cached query for a user who dropped off their last leaderboard
        if (!table.loadFromDbTables)
        {
            if (updateLeaderboardResult.mStatus == UpdateLeaderboardResult::NEWLY_RANKED)
            {
                TRACE_LOG("[LeaderboardIndexes].loadDataRow() - entity " << entityId << " is ranked in "
                          << slb->getBoardName() << ", updating query cache for table: " << lbTableName);

                // Note that we choose to use the main stat table name (as opposed to lbTableName) as the key, because
                // the lbTableName is local to this method, so it is simpler to use the main stat table name
                EntityIdKeyscopeMap& ksMap = mLeaderboardQueryMap[tableName];
                KeyscopeQueryMap& queryMap = ksMap[entityId];
                KeyscopeQueryMap::iterator ksqItr = queryMap.find(keyscope);
                if (ksqItr != queryMap.end())
                {
                    ++(ksqItr->second.count);
                }
                else
                {
                    // Note that the columns fetched from the main stats table should precisely match those we need to insert
                    // into the lb table.  Further note that we use a bit of a trick here - we know that lb tables
                    // can only contain numeric columns: entity_id, optional period_id, keyscopes, and rankable stat values,
                    // thus we don't have to worry about any string escaping issues, we simply take the string representation
                    // of each numeric column from the DB result from the main stats table and use that to populate the lb table
                    QueryPtr query = DB_NEW_QUERY_PTR(conn);
                    query->append("INSERT INTO `$s` (", lbTableName);
                    for (uint32_t column = 0; column < result->getColumnCount(); ++column)
                    {
                        query->append("`$s`,", result->getColumnName(column));
                    }
                    query->trim(1);
                    query->append(") VALUES (");
                    for (uint32_t column = 0; column < result->getColumnCount(); ++ column)
                    {
                        query->append("'$s',", row->getString(column));
                    }
                    query->trim(1);
                    query->append(");");
                    QueryAndCount& queryAndCount = queryMap[blaze_strdup(keyscope)];
                    queryAndCount.count = 1;
                    queryAndCount.query = blaze_strdup(query->get());
                }
            }

            if (updateLeaderboardResult.mIsStatRemoved)
            {
                TRACE_LOG("[LeaderboardIndexes].loadDataRow() - entity " << entityId << " is no longer ranked in "
                          << slb->getBoardName() << ", updating query cache for table: " << lbTableName);

                EntityIdKeyscopeMap& ksMap = mLeaderboardQueryMap[tableName];
                KeyscopeQueryMap& queryMap = ksMap[updateLeaderboardResult.mRemovedEntityId];
                KeyscopeQueryMap::iterator ksqItr = queryMap.find(keyscope);
                if (ksqItr == queryMap.end())
                {
                    WARN_LOG("[LeaderboardIndexes].loadDataRow() - did not find a query entry for " <<
                        updateLeaderboardResult.mRemovedEntityId << " while loading data for leaderboard table: " << lbTableName);
                }
                else
                {
                    --(ksqItr->second.count);
                    if (ksqItr->second.count == 0)
                    {
                        BLAZE_FREE((char8_t*)ksqItr->first);
                        queryMap.erase(ksqItr);
                    }
                }
            }
        }
    }
}

/*************************************************************************************************/
/*!
    \brief getLeaderboardIndex

    Performs the navigation through the hierarchical structure to locate the actual
    leaderboard in order to service a particular client request.

    \param[in]  leaderboard - the StatLeaderboard object
    \param[in]  scopeNameValueListMap - map of scope keys to scope values for this request
    \param[in]  periodId - period id for the request
    \param[out] leaderboardIndex - leaderboard to return to the caller

    \return - corresponding error code
*/
/*************************************************************************************************/
BlazeRpcError LeaderboardIndexes::getLeaderboardIndex(const StatLeaderboard& leaderboard,
    const ScopeNameValueListMap* scopeNameValueListMap, int32_t periodId,
    LeaderboardIndexBase** leaderboardIndex) const
{
    char8_t* keyScopeString = makeKeyScopeString(scopeNameValueListMap);

    const KeyScopeLeaderboards* keyScopeLeaderboards = leaderboard.getKeyScopeLeaderboards();
    if (keyScopeLeaderboards == nullptr) 
    {
        WARN_LOG("[LeaderboardIndexes].getLeaderboardIndex() - [1] Leaderboard: "
            "" << leaderboard.getBoardName() << " with scopes: " << keyScopeString << ": invalid scope/value");
    
        delete[] keyScopeString;
        return Blaze::STATS_ERR_BAD_SCOPE_INFO;
    }

    const KeyScopeLeaderboardMap* keyScopeLeaderboardMap = keyScopeLeaderboards->getKeyScopeLeaderboardMap();
    KeyScopeLeaderboardMap::const_iterator ksIter = keyScopeLeaderboardMap->find(keyScopeString);
    if (ksIter == keyScopeLeaderboardMap->end())
    {
        WARN_LOG("[LeaderboardIndexes].getLeaderboardIndex() - [2] Leaderboard: "
            "" << leaderboard.getBoardName() << " with scopes: " << keyScopeString << " : inalid scope/value");
    
        delete[] keyScopeString;
        return Blaze::STATS_ERR_BAD_SCOPE_INFO;
    }
    
    delete[] keyScopeString;
    HistoricalLeaderboards* historicalLeaderboards = ksIter->second;

    const PeriodIdMap* periodIdMap = historicalLeaderboards->getPeriodIdMap();
    PeriodIdMap::const_iterator idIter = periodIdMap->find(periodId);
    if (idIter == periodIdMap->end())
    {
        TRACE_LOG("[LeaderboardIndexes].getLeaderboardIndex() - [3] Leaderboard: "
            "" << leaderboard.getBoardName() << " for period ID: " << periodId << " : no data available");
        return Blaze::ERR_OK;
    }

    const LeaderboardMap* lbMap = idIter->second;
    LeaderboardMap::const_iterator lbIter = lbMap->find(leaderboard.getBoardId());
    if (lbIter == lbMap->end())
    {
        TRACE_LOG("[LeaderboardIndexes].getLeaderboardIndex() - [4] Leaderboard: "
            "" << leaderboard.getBoardName() << " period ID: " << periodId << " : no data available");
        return Blaze::ERR_OK;
    }
    
    if (leaderboardIndex != nullptr)
        *leaderboardIndex = lbIter->second;

    return Blaze::ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief getLeaderboardRows

    Gets entities & ranks for getLeaderboard command from appropriate ranking table.
    Also fills entity ID list used to query identity service.
    Other stats in the leaderboard group are NOT handled here.

    \param[in]  leaderboard - the StatLeaderboard object
    \param[in]  scopeNameValueListMap - list of keyscope name/values pairs as defined by configuration
                with user-required ones filled in from request
    \param[in]  periodId - period ID of requested leaderboard
    \param[in]  startRank - start rank of requested leaderboard
    \param[in]  count -number of rows requested
    \param[in]  userSetId - id of user set to filter leaderboard on (if not EA::TDF::OBJECT_ID_INVALID)
    \param[out] keys - storage for entity IDs to request Identity info for
    \param[out] response - command response object to be filled with response data
    \param[out] sorted - whether the response is sorted

    \return - corresponding error code
*/
/*************************************************************************************************/
BlazeRpcError LeaderboardIndexes::getLeaderboardRows(const StatLeaderboard& leaderboard,
    const ScopeNameValueListMap* scopeNameValueListMap, int32_t periodId, int32_t startRank, int32_t count,
    const EA::TDF::ObjectId& userSetId, EntityIdList& keys, LeaderboardStatValues& response, bool& sorted) const
{
    TRACE_LOG("[LeaderboardIndexes].getLeaderboardRows: lb name: <"
        << leaderboard.getBoardName() << "> lb id: " << leaderboard.getBoardId());

    LeaderboardIndexBase* leaderboardIndex = nullptr;
    BlazeRpcError err = getLeaderboardIndex(leaderboard, scopeNameValueListMap, periodId, &leaderboardIndex);
    if (err != ERR_OK)
        return err;

    // Leaderboard structures not found 
    if (leaderboardIndex == nullptr)
        return ERR_OK;

    if (userSetId == EA::TDF::OBJECT_ID_INVALID)
        err = leaderboardIndex->getRows(startRank, count, response.getRows(), keys);
    else
        err = leaderboardIndex->getRowsForUserSet(startRank, count, userSetId, response.getRows(), keys, sorted);

    return err;
}

/*************************************************************************************************/
/*!
    \brief getCenteredLeaderboardRows

    Gets entities & ranks for getCenteredLeaderboard command from appropriate ranking table.
    Also fills entity ID list used to query identity service.
    Other stats in the leaderboard group are NOT handled here.

    \param[in]  leaderboard - the StatLeaderboard object
    \param[in]  scopeNameValueListMap - list of keyscope name/values pairs as defined by configuration
                with user-required ones filled in from request
    \param[in]  periodId - period ID of requested leaderboard
    \param[in]  centerEntityId - entity ID of user to get stats centered at
    \param[in]  count -number of rows requested
    \param[in]  userSetId - id of user set to filter leaderboard on (if not EA::TDF::OBJECT_ID_INVALID)
    \param[in]  showAtBottomIfNotFound - if entity not found, show it at the bottom of the leaderboard
    \param[out] keys - storage for entity IDs to request Identity info for
    \param[out] response - command response object to be filled with respnse data
    \param[out] sorted - whether the response is sorted

    \return - corresponding error code
*/
/*************************************************************************************************/
BlazeRpcError LeaderboardIndexes::getCenteredLeaderboardRows(const StatLeaderboard& leaderboard,
    const ScopeNameValueListMap* scopeNameValueListMap, int32_t periodId, EntityId centerEntityId, int32_t count,
    const EA::TDF::ObjectId& userSetId, bool showAtBottomIfNotFound,
    EntityIdList& keys, LeaderboardStatValues& response, bool& sorted) const
{
    TRACE_LOG("[LeaderboardIndexes].getCenteredLeaderboardRows: lb name: <"
        << leaderboard.getBoardName() << "> lb id: " << leaderboard.getBoardId());

    LeaderboardIndexBase* leaderboardIndex = nullptr;
    BlazeRpcError err = getLeaderboardIndex(leaderboard, scopeNameValueListMap, periodId, &leaderboardIndex);
    if (err != ERR_OK)
        return err;

    // Leaderboard structures not found 
    if (leaderboardIndex == nullptr)
        return ERR_OK;

    if (userSetId == EA::TDF::OBJECT_ID_INVALID)
        err = leaderboardIndex->getCenteredRows(centerEntityId, count, showAtBottomIfNotFound, response.getRows(), keys);
    else
        err = leaderboardIndex->getCenteredRowsForUserSet(centerEntityId, count, userSetId, response.getRows(), keys, sorted);

    return err;
}

/*************************************************************************************************/
/*!
    \brief getFilteredLeaderboardRows

    Gets entities & ranks for getFilteredLeaderboard command from appropriate ranking table.
    Also fills entity ID list used to query identity service.
    Other stats in the leaderboard group are NOT handled here.

    \param[in]  leaderboard - the StatLeaderboard object
    \param[in]  scopeNameValueListMap - list of keyscope name/values pairs as defined by configuration
                with user-required ones filled in from request
    \param[in]  periodId - period ID of requested leaderboard
    \param[in]  idList - list of entity IDs to retrieve ranks for
    \param[in]  limit - maximum count of lb results to return
    \param[out] keys - storage for entity IDs to request Identity info for
    \param[out] response - command response object to be filled with respnse data
    \param[out] sorted - whether the response is sorted

    \return - corresponding error code
*/
/*************************************************************************************************/
BlazeRpcError LeaderboardIndexes::getFilteredLeaderboardRows(const StatLeaderboard& leaderboard,
    const ScopeNameValueListMap* scopeNameValueListMap, int32_t periodId, const EntityIdList& idList, uint32_t limit,
    EntityIdList& keys, LeaderboardStatValues& response, bool& sorted) const
{
    TRACE_LOG("[LeaderboardIndexes].getFilteredLeaderboardRows: lb name: <"
        << leaderboard.getBoardName() << "> lb id: " << leaderboard.getBoardId());

    LeaderboardIndexBase* leaderboardIndex = nullptr;
    BlazeRpcError err = getLeaderboardIndex(leaderboard, scopeNameValueListMap, periodId, &leaderboardIndex);
    if (err != ERR_OK)
        return err;

    // always assume unsorted in this case
    sorted = false;

    // Leaderboard structures not found 
    if (leaderboardIndex == nullptr)
        return ERR_OK;

    err = leaderboardIndex->getFilteredRows(idList, limit, response.getRows(), keys);

    return  ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief getLeaderboardRanking

    Gets the ranking position for a given entity id.

    \param[in]  leaderboard - pointer to the StatLeaderboard object
    \param[in]  scopeNameValueListMap - list of keyscope name/values pairs as defined by configuration
                with user-required ones filled in from request
    \param[in]  periodId - period ID of requested leaderboard
    \param[in]  centerEntityId - entity ID of user to get stats centered at
    \param[out] entityRank - ranking of this entity in the given leaderboard, scope and period id.
                not updated if getLeaderboardRanking returns an error

    \return - corresponding error code
*/
/*************************************************************************************************/
BlazeRpcError LeaderboardIndexes::getLeaderboardRanking(
    const StatLeaderboard* leaderboard, const ScopeNameValueListMap* scopeNameValueListMap,
    int32_t periodId, EntityId centerEntityId, int32_t &entityRank) const
{
    TRACE_LOG("[LeaderboardIndexes].getLeaderboardRanking lb name: <" << leaderboard->getBoardName() << "> lb id: " 
              << leaderboard->getBoardId()); 

    entityRank = 0;
    LeaderboardIndexBase* leaderboardIndex = nullptr;
    BlazeRpcError err = getLeaderboardIndex(*leaderboard, scopeNameValueListMap, periodId, &leaderboardIndex);
    if (err != ERR_OK)
        return err;

    // Leaderboard structures not found 
    if (leaderboardIndex == nullptr)
        return ERR_OK;

    if (leaderboardIndex->getCount() == 0)
        return ERR_OK;

    if (leaderboard->isTiedRank())
    {
        entityRank = leaderboardIndex->getTiedRankByEntity(centerEntityId);
    }
    else
    {
        entityRank = leaderboardIndex->getRankByEntity(centerEntityId);
    }

    return  ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief getEntityCount

    Provides number of entities in the leaderboard.

    \param[in] - leaderboard - pointer to the StatLeaderboard object
    \param[in] - scopeNameValueListMap - list of keyscope name/values pairs as defined by configuration
                 with user-required ones filled in from request
    \param[in] - periodId - period ID of requested leaderboard
    \param[in] - response - Response
    \return - corresponding error code
*/
/*************************************************************************************************/
BlazeRpcError LeaderboardIndexes::getEntityCount(const StatLeaderboard* leaderboard,
    const ScopeNameValueListMap* scopeNameValueListMap, int32_t periodId, EntityCount* response) const
{
    TRACE_LOG("[LeaderboardIndexes].getEntityCount lb name: <" << leaderboard->getBoardName() << "> lb id: " 
              << leaderboard->getBoardId()); 

    LeaderboardIndexBase* leaderboardIndex = nullptr;
    BlazeRpcError err = getLeaderboardIndex(*leaderboard, scopeNameValueListMap, periodId, &leaderboardIndex);
    if (err != ERR_OK)
        return err;

    // Leaderboard structures not found 
    if (leaderboardIndex == nullptr)
        return ERR_OK;

    response->setCount(static_cast<int32_t>(leaderboardIndex->getCount()));

    return  ERR_OK;
}        

/*************************************************************************************************/
/*!
    \brief extract

    A master only called issued by the master impl in order to build up a response to a slave's
    request for some leaderboard data.

    \param[in]  request - request for leaderboard indexes data from the slave
    \param[out] entries - response data from the master
*/
/*************************************************************************************************/
void LeaderboardIndexes::extract(const PopulateLeaderboardIndexRequest& request, LeaderboardEntries* entries) const
{
    CategoryMap::const_iterator catIter = mCategoryMap.find(request.getCategoryId());
    if (catIter != mCategoryMap.end())
    {
        PeriodTypeMap::const_iterator periodTypeIter = catIter->second->find(request.getPeriodType());
        if (periodTypeIter != catIter->second->end())
        {
            KeyScopeLeaderboardMap::const_iterator keyScopeIter = periodTypeIter->second->getKeyScopeLeaderboardMap()->find(request.getKeyScopes());
            if (keyScopeIter != periodTypeIter->second->getKeyScopeLeaderboardMap()->end())
            {
                PeriodIdMap::const_iterator periodIdIter = keyScopeIter->second->getPeriodIdMap()->find(request.getPeriodId());
                if (periodIdIter != keyScopeIter->second->getPeriodIdMap()->end())
                {
                    LeaderboardMap::const_iterator lbIter = periodIdIter->second->find(request.getLeaderboardId());
                    if (lbIter != periodIdIter->second->end())
                    {
                        LeaderboardIndexBase* leaderboardIndex = lbIter->second;

                        uint32_t count = POPULATE_CHUNK_SIZE;
                        for (uint32_t i = 0; ; i += count)
                        {
                            EA::TDF::TdfBlob* list = BLAZE_NEW_STATS_LEADERBOARD EA::TDF::TdfBlob();
                            uint32_t actualCount = leaderboardIndex->extract(list, i, count);
                            entries->push_back(list);
                            if (actualCount < count)
                                break;
                        }
                    }
                }
            }
        }
    }
}

/*************************************************************************************************/
/*!
    \brief writeIndexesToDb

    A master only call to write out the in memory leaderboard indexes to the corresponding 
    database tables for all tables that require it.

    \param[in]  conn - The database connection to use.
    \param[out] tableInfoMap - contains information about which leaderboards need to be written out 
        to the database.

    \return - true if the operation succeeded, false otherwise
*/
/*************************************************************************************************/
bool LeaderboardIndexes::writeIndexesToDb(DbConnPtr conn, const TableInfoMap& tableInfoMap)
{
    TRACE_LOG("[LeaderboardIndexes].writeIndexesToDb(): Writing the leaderboard indexes to the database");

    if (conn == nullptr)
    {
        ERR_LOG("[LeaderboardIndexes].writeIndexesToDb(): failed to obtain connection.");
        return false;
    }

    TableInfoMap::const_iterator tableItr = tableInfoMap.begin();
    TableInfoMap::const_iterator tableEnd = tableInfoMap.end();

    for (; tableItr != tableEnd; ++tableItr)
    {
        const TableInfo& tableInfo = tableItr->second;
        if (tableInfo.loadFromDbTables)
            continue;

        //the table was newly created, so we need to populate it with the values that we read
        // in from the stats tables
        const char8_t* tableName = tableItr->first;
        char8_t lbTableName[LB_MAX_TABLE_NAME_LENGTH];
        getLeaderboardTableName(tableName, lbTableName, LB_MAX_TABLE_NAME_LENGTH);
        INFO_LOG("[LeaderboardIndexes].writeIndexesToDb(): Writing to db for leaderboard table (" << lbTableName << ")");

        // Note that we use the stat table name as opposed to the lb table name to key this map, to go along with the
        // loadData code that populates it
        LeaderboardQueryMap::const_iterator lbsItr = mLeaderboardQueryMap.find(tableName);
        if (lbsItr == mLeaderboardQueryMap.end())
        {
            //this likely means that there is no data in the table.
            TRACE_LOG("[LeaderboardIndexes].writeIndexesToDb(): no data to write for table (" << lbTableName << ")."); 
        }
        else
        {
            const EntityIdKeyscopeMap& eikMap = lbsItr->second;
            EntityIdKeyscopeMap::const_iterator eiItr = eikMap.begin();
            EntityIdKeyscopeMap::const_iterator eiEnd = eikMap.end();
            for ( ; eiItr != eiEnd; ++eiItr)
            {
                const KeyscopeQueryMap& ksqMap = eiItr->second;
                QueryPtr query = DB_NEW_QUERY_PTR(conn);

                uint32_t count = ksqMap.size();
                KeyscopeQueryMap::const_iterator ksqItr = ksqMap.begin();
                KeyscopeQueryMap::const_iterator ksqEnd = ksqMap.end();
                for ( ; ksqItr != ksqEnd; ++ksqItr)
                {
                    query->append(ksqItr->second.query);
                    if (--count % DB_UPDATE_BATCH_SIZE == 0)
                    {
                        DbError dbErr = conn->executeMultiQuery(query);

                        if (dbErr != DB_ERR_OK)
                        {
                            ERR_LOG("[LeaderboardIndexes].writeIndexesToDb(): error writing leaderboard indexes to the database for table (" 
                                    << lbTableName << ")."); 
                            return false;
                        }
                        DB_QUERY_RESET(query);
                    }
                    BLAZE_FREE((char8_t*)ksqItr->first);
                }
            }
        }
    }

    //we don't need the cached queries anymore, so free up that memory
    // if leaks are detected due to any keyscope strings remaining in here, then we're caching off more queries than needed
    mLeaderboardQueryMap.clear();

    return true;
}

LeaderboardIndexes::LeaderboardIndexes(StatsComponentInterface* component, Metrics::MetricsCollection& metricsCollection, bool isMaster)
    : mComponent(component),
    mIsMaster(isMaster),
    mTotalLeaderboards(0),
    mTotalLeaderboardEntries(0),
    mCountLeaderboards(0),
    mPopulatingLeaderboardName(nullptr),
    mPopulateTimerId(INVALID_TIMER_ID),
    mGaugeBudgetOfAllRankingTables(metricsCollection, "gaugeBudgetOfAllRankingTables"),
    mGaugeSizeOfAllRankingTables(metricsCollection, "gaugeSizeOfAllRankingTables",
        [this]() -> uint64_t { return this->getMemoryAllocated(); }),
    mGaugeLeaderboardCount(metricsCollection, "gaugeLeaderboardCount",
        [this]() -> uint64_t { return this->getLeaderboardCount(); }),
    mGaugeLeaderboardRowCount(metricsCollection, "gaugeLeaderboardRowCount",
        [this]() -> uint64_t { return this->getLeaderboardRowCount(); })
{

    memset(mTrimPeriodsTimerIds, INVALID_TIMER_ID, sizeof(mTrimPeriodsTimerIds));
}

/*************************************************************************************************/
/*!
    \brief ~LeaderboardIndexes

    Clean up any resources owned by this object.
*/
/*************************************************************************************************/
LeaderboardIndexes::~LeaderboardIndexes()
{
    for (uint32_t periodType = 0; periodType < STAT_NUM_PERIODS; ++periodType)
    {
        if (mTrimPeriodsTimerIds[periodType] != INVALID_TIMER_ID)
        {
            gSelector->cancelTimer(mTrimPeriodsTimerIds[periodType]);
            mTrimPeriodsTimerIds[periodType] = INVALID_TIMER_ID;
        }
    }

    LeaderboardIndexes::CategoryMap::const_iterator catIter = mCategoryMap.begin();
    LeaderboardIndexes::CategoryMap::const_iterator catEnd = mCategoryMap.end();
    for (; catIter != catEnd; ++catIter)
    {
        LeaderboardIndexes::PeriodTypeMap::const_iterator periodIter = catIter->second->begin();
        LeaderboardIndexes::PeriodTypeMap::const_iterator periodEnd = catIter->second->end();
        for (; periodIter != periodEnd; ++periodIter)
        {
            delete periodIter->second;
        }
        
        delete catIter->second;
    }
}

/*************************************************************************************************/
/*!
    \brief ~KeyScopeLeaderboards

    Clean up any resources owned by this object.
*/
/*************************************************************************************************/
KeyScopeLeaderboards::~KeyScopeLeaderboards()
{
    using namespace eastl;
    
    LeaderboardIndexes::KeyScopeLeaderboardMap::const_iterator ksIter = mKeyScopeLeaderboardMap.begin();
    LeaderboardIndexes::KeyScopeLeaderboardMap::const_iterator ksEnd = mKeyScopeLeaderboardMap.end();
    for (;ksIter != ksEnd; ++ksIter)
    {
        delete ksIter->second;
        BLAZE_FREE((char8_t*)ksIter->first);
    }

    LeaderboardIndexes::KeyScopeSingleMultiValueMap::const_iterator smItr = mKeyScopeSingleMultiValueMap.begin();
    LeaderboardIndexes::KeyScopeSingleMultiValueMap::const_iterator smEnd = mKeyScopeSingleMultiValueMap.end();
    for (; smItr != smEnd; ++smItr)
    {
        list<const char8_t*>* keyScopeStringList = smItr->second;
        list<const char8_t*>::const_iterator ksStrItr = keyScopeStringList->begin();
        list<const char8_t*>::const_iterator ksStrEnd = keyScopeStringList->end();
        for (; ksStrItr != ksStrEnd; ++ksStrItr)
        {
            BLAZE_FREE((char8_t*)(*ksStrItr));
        }
        smItr->second->clear();
        delete smItr->second;
        BLAZE_FREE((char8_t*)smItr->first);
    }
}
/*************************************************************************************************/
/*!
    \brief ~HistoricalLeaderboards

    Clean up any resources owned by this object.
*/
/*************************************************************************************************/
HistoricalLeaderboards::~HistoricalLeaderboards()
{
    LeaderboardIndexes::PeriodIdMap::const_iterator perIdIter = mPeriodIdMap.begin();
    LeaderboardIndexes::PeriodIdMap::const_iterator perIdEnd = mPeriodIdMap.end();
    for (; perIdIter != perIdEnd; ++perIdIter)
    {
        LeaderboardIndexes::LeaderboardMap::const_iterator lbItr = perIdIter->second->begin();
        LeaderboardIndexes::LeaderboardMap::const_iterator lbEnd = perIdIter->second->end();
        for (; lbItr != lbEnd; ++lbItr)
        {
            delete lbItr->second;
        }
        delete perIdIter->second;
    }
}

/*************************************************************************************************/
/*!
    \brief isLeaderboard

    Return whether a leaderboard exists for the parameters.  Note that this method is used
    to determine whether to broadcast out stats, given the advent of stats sharding, it is
    important to send all stats that are needed to populate a leaderboard indexes table row,
    thus if any keyscope combo for a given table has a leaderboard for a given stat,
    we must send this stat out, even if no leaderboard is configured specifically on the
    current keyscope map in question.

    \param[in]  categoryId - internal ID of category for the leaderboard
    \param[in]  statName - ranking stat name for the leaderboard
    \param[in]  periodType - period type for the leaderboard
    \param[in]  scopeNameValueMap - keyscope name value map for the leaderboard

    \return - whether a leaderboard exists for the parameters
*/
/*************************************************************************************************/
bool LeaderboardIndexes::isLeaderboard(uint32_t categoryId, const char8_t* statName, int32_t periodType, const ScopeNameValueMap* scopeNameValueMap) const
{
    CategoryMap::const_iterator catIter = mCategoryMap.find(categoryId);
    if (catIter == mCategoryMap.end())
        return false; // category not used by any leaderboard

    PeriodTypeMap* periodTypeMap = catIter->second;
    PeriodTypeMap::const_iterator periodTypeIter = periodTypeMap->find(periodType);
    if (periodTypeIter == periodTypeMap->end())
        return false; // period type not found for category

    KeyScopeLeaderboards* keyScopeLeaderboards = periodTypeIter->second;
    return keyScopeLeaderboards->isLeaderboardStat(statName);
}

/*************************************************************************************************/
/*!
    \brief updateStats

    This method is the main entry point on the receiving side of both stats slaves as well as
    the stats master when they receive cache update notifications.  While there is some commonality
    between this method and the other updateStats method later in this file, there are enough
    differences it is cleaner to keep them separate.  All received rows (other than those with
    no stat updates) have been determined on the sending side to impact leaderboards.

    \param[in]  data - a list of cache rows to apply to the leaderboard cache
*/
/*************************************************************************************************/
void LeaderboardIndexes::updateStats(const CacheRowUpdateList& data)
{
    for (CacheRowUpdateList::const_iterator it = data.begin(); it != data.end(); ++it)
    {
        const CacheRowUpdate* cacheRow = *it;
        if (cacheRow->getStatUpdates().empty())
            continue;

        const uint8_t* bytes = cacheRow->getBinaryData().getData();
        CategoryMap::const_iterator catIter = mCategoryMap.find(*((const uint32_t*)(bytes)));
        PeriodTypeMap* periodTypeMap = catIter->second;
        PeriodTypeMap::const_iterator periodTypeIter = periodTypeMap->find(*((const int32_t*)(bytes+12)));
        const char8_t* keyScopeString = cacheRow->getKeyScopeString();

        KeyScopeLeaderboards* keyScopeLeaderboards = periodTypeIter->second;
        KeyScopeLeaderboardMap* keyScopeLeaderboardMap = keyScopeLeaderboards->getKeyScopeLeaderboardMap();
        KeyScopeLeaderboardMap::const_iterator scopeIter;

        // look for any multi-value keyscopes that this single-value keyscopes affects
        KeyScopeSingleMultiValueMap* keyScopeSingleMultiValueMap = keyScopeLeaderboards->getKeyScopeSingleMultiValueMap();
        KeyScopeSingleMultiValueMap::const_iterator smItr = keyScopeSingleMultiValueMap->find(keyScopeString);
        if (smItr != keyScopeSingleMultiValueMap->end())
        {
            eastl::list<const char8_t*>* keyScopeStringList = smItr->second;
            eastl::list<const char8_t*>::const_iterator ksStrItr = keyScopeStringList->begin();
            eastl::list<const char8_t*>::const_iterator ksStrEnd = keyScopeStringList->end();
            for (; ksStrItr != ksStrEnd; ++ksStrItr)
            {
                // this part is just a copy of the logic below for single-value keyscopes update
                scopeIter = keyScopeLeaderboardMap->find(*ksStrItr);
                if (scopeIter == keyScopeLeaderboardMap->end())
                    continue; // we dont have any boards with this keyscopes
                HistoricalLeaderboards* historicalLeaderboards = scopeIter->second;
                LeaderboardList* leaderboardList = historicalLeaderboards->getLeaderboardList();
                LeaderboardList::const_iterator lbIter = leaderboardList->begin();
                LeaderboardList::const_iterator lbEnd = leaderboardList->end();
                for (; lbIter != lbEnd; ++lbIter)
                {
                    if (!(*lbIter)->isInMemory())
                        continue;

                    updateIndexes(*lbIter, *((const EntityId*)(bytes+4)), *((const int32_t*)(bytes+16)), cacheRow->getStatUpdates(), historicalLeaderboards->getPeriodIdMap());
                }
            }
        }

        // now for the single-value keyscopes update
        scopeIter = keyScopeLeaderboardMap->find(keyScopeString);
        if (scopeIter == keyScopeLeaderboardMap->end())
            continue; // we dont have any boards with this keyscopes

        HistoricalLeaderboards* historicalLeaderboards = scopeIter->second;
        LeaderboardList* leaderboardList = historicalLeaderboards->getLeaderboardList();
        LeaderboardList::const_iterator lbIter = leaderboardList->begin();
        LeaderboardList::const_iterator lbEnd = leaderboardList->end();
        for (; lbIter != lbEnd; ++lbIter)
        {
            if (!(*lbIter)->isInMemory())
                continue;

            updateIndexes(*lbIter, *((const EntityId*)(bytes+4)), *((const int32_t*)(bytes+16)), cacheRow->getStatUpdates(), historicalLeaderboards->getPeriodIdMap());
        }
    }
}

/*************************************************************************************************/
/*!
    \brief updateIndexes

    This method is called by updateStats on the receiving side of both stats slaves as well as
    the stats master when they receive cache update notifications.  While there is some commonality
    between this method and the other updateIndexes method later in this file, there are enough
    differences it is cleaner to keep them separate.  On period rollover the first update will
    need to construct a new data structure to hold the new period's ranking data, but the main
    purpose of this method is to extract the string data from the broadcast update, convert it
    to numeric, and then apply it to the actual leaderboard.

    \param[in]  leaderboard - the leaderboard definition
    \param[in]  entityId - entity id who's stats are in statValMap
    \param[in]  periodId - period id of the stats
    \param[in]  statValMap - the stats which changed in a given row, may or may not contain a value for the leaderboard in question
    \param[in/out] periodIdMap - map that either already contains the actual leaderboard data structure, or in case of rollover will be updated to contain a newly created one
*/
/*************************************************************************************************/
void LeaderboardIndexes::updateIndexes(const StatLeaderboard* leaderboard, EntityId entityId, int32_t periodId, const StatNameValueMap& statValMap, PeriodIdMap* periodIdMap)
{
    // First chunk of method handles the relatively rare case of constructing a new data structure if
    // this is the first update of a particular leaderboard post periodic rollover
    LeaderboardMap* leaderboardMap;
    PeriodIdMap::const_iterator periodIter = periodIdMap->find(periodId);
    if (EA_UNLIKELY(periodIter == periodIdMap->end()))
    {
        leaderboardMap = BLAZE_NEW_STATS_LEADERBOARD LeaderboardMap();
        periodIdMap->insert(eastl::make_pair(periodId, leaderboardMap));
    }
    else
    {
        leaderboardMap = periodIter->second;
    }

    LeaderboardIndexBase* leaderboardIndex;
    LeaderboardMap::const_iterator lbIter = leaderboardMap->find(leaderboard->getBoardId());
    if (EA_UNLIKELY(lbIter == leaderboardMap->end()))
    {
        // Newly added leaderboards in this method should be due to period rollover, and thus on both
        // master and slave they are ready to be added to immediately and do not require populating
        if (leaderboard->hasSecondaryRankings())
            leaderboardIndex = BLAZE_NEW_STATS_LEADERBOARD LeaderboardIndex<MultiRankData>(leaderboard);
        else if (leaderboard->getStat()->getDbType().getType() == STAT_TYPE_FLOAT)
            leaderboardIndex = BLAZE_NEW_STATS_LEADERBOARD LeaderboardIndex<float_t>(leaderboard);
        else
        {
            if (leaderboard->getStat()->getTypeSize() == 8)
            {
                leaderboardIndex = BLAZE_NEW_STATS_LEADERBOARD LeaderboardIndex<int64_t>(leaderboard);
            }
            else
            {
                leaderboardIndex = BLAZE_NEW_STATS_LEADERBOARD LeaderboardIndex<int32_t>(leaderboard);
            }
        }
        leaderboardMap->insert(eastl::make_pair(leaderboard->getBoardId(), leaderboardIndex));
    }
    else
    {
        leaderboardIndex = lbIter->second;
    }

    // Second portion of this method applies the change to the leaderboard data structure
    UpdateLeaderboardResult result;
    if (leaderboard->hasSecondaryRankings())
    {
        // Note, if a leaderboard involving secondary rankings is updated on the sending side,
        // they will send us all the stats.  If even a single stat is missing, then while we may
        // need to update some other leaderboard based on the stats that are still present, we know
        // that we do not need to update the leaderboard in question
        bool doUpdate = true;
        MultiRankData value;
        for (int32_t pos = 0; pos < leaderboard->getRankingStatsSize(); ++pos)
        {
            StatNameValueMap::const_iterator it = statValMap.find(leaderboard->getRankingStatName(pos));
            if (it != statValMap.end())
            {
                if (leaderboard->getRankingStat(pos)->getDbType().getType() == STAT_TYPE_FLOAT)
                    blaze_str2flt(it->second, value.data[pos].floatValue);
                else
                    blaze_str2int(it->second, &(value.data[pos].intValue));
            }
            else
            {
                doUpdate = false;
                break;
            }
        }

        if (doUpdate)
            (static_cast<LeaderboardIndex<MultiRankData>*>(leaderboardIndex))->update(entityId, value, result);
    }
    else
    {
        StatNameValueMap::const_iterator it = statValMap.find(leaderboard->getRankingStatName(0));
        if (it != statValMap.end())
        {
            if (leaderboard->getStat()->getDbType().getType() == STAT_TYPE_FLOAT)
            {
                float_t value;
                blaze_str2flt(it->second.c_str(), value);
                (static_cast<LeaderboardIndex<float_t>*>(leaderboardIndex))->update(entityId, value, result);

            }
            else if (leaderboard->getStat()->getTypeSize() == 8)
            {
                int64_t value;
                blaze_str2int(it->second.c_str(), &value);
                (static_cast<LeaderboardIndex<int64_t>*>(leaderboardIndex))->update(entityId, value, result);
            }
            else
            {
                int32_t value;
                blaze_str2int(it->second.c_str(), &value);
                (static_cast<LeaderboardIndex<int32_t>*>(leaderboardIndex))->update(entityId, value, result);
            }
        }
    }
}


/*************************************************************************************************/
/*!
    \brief updateStats

    This is the main entry point into this class on side of the core slave who is performing an
    actual stats update.  This call is responsible not only for updating the local leaderboard
    data structures, but also populating the notification object that will eventually be
    broadcast to all other stats instances.  Additionally, it is responsible for populating a
    DB query which will be used to apply the same updates to the leaderboard database tables.
    See the other updateStats method on this class for the corresponding receive side.
    This method operates on a single stat row, and is called potentially many times during the
    course of handling a large stat update transaction.
    
    \param[in]  categoryId - category id of the update
    \param[in]  entityId - entity id of the update
    \param[in]  periodType - period type of the update
    \param[in]  periodId - period id of the update
    \param[in]  statValMap - map of all stat values (whether modified during the update call or not)
    \param[in]  scopeMap - key scope values for this row
    \param[out]  lbQueryString - query string builder to be filled in with any required updates
    \param[out]  statMap - map to be filled in for broadcasting to other stats instances
*/
/*************************************************************************************************/
void LeaderboardIndexes::updateStats(uint32_t categoryId, EntityId entityId, int32_t periodType, int32_t periodId,
                                     const StatValMap* statValMap, const ScopeNameValueMap* scopeMap, const char8_t* keyScopeString, StringBuilder& lbQueryString, StatNameValueMap& statMap)
{
    CategoryMap::const_iterator catIter = mCategoryMap.find(categoryId);
    if (catIter == mCategoryMap.end())
    {
        TRACE_LOG("[LeaderboardIndexes].updateStats() - category id <"
            << categoryId << "> is not used by any leaderboard.");
        return;
    }

    PeriodTypeMap* periodTypeMap = catIter->second;
    PeriodTypeMap::const_iterator periodTypeIter = periodTypeMap->find(periodType);
    if (periodTypeIter == periodTypeMap->end())
    {
        TRACE_LOG("[LeaderboardIndexes].updateStats() - no leaderboards for period type <"
            << periodType << "> for category id <" << categoryId << ">.");
        return;
    }

    TRACE_LOG("[LeaderboardIndexes].updateStats() - period type <" << periodType
        << "> category id <" << categoryId << "> keyscope: " << keyScopeString);

    KeyScopeLeaderboards* keyScopeLeaderboards = periodTypeIter->second;
    KeyScopeLeaderboardMap* keyScopeLeaderboardMap = keyScopeLeaderboards->getKeyScopeLeaderboardMap();
    KeyScopeLeaderboardMap::const_iterator scopeIter;

    // As we process the row, keep track of whether we added or modified the entity's ranking in at least one leaderboard (upsert)
    // or removed the entity from any leaderboard (remove)
    bool upsert = false;
    bool remove = false;

    // look for any multi-value keyscopes that this single-value keyscopes affects
    KeyScopeSingleMultiValueMap* keyScopeSingleMultiValueMap = keyScopeLeaderboards->getKeyScopeSingleMultiValueMap();
    KeyScopeSingleMultiValueMap::const_iterator smItr = keyScopeSingleMultiValueMap->find(keyScopeString);
    if (smItr != keyScopeSingleMultiValueMap->end())
    {
        eastl::list<const char8_t*>* keyScopeStringList = smItr->second;
        eastl::list<const char8_t*>::const_iterator ksStrItr = keyScopeStringList->begin();
        eastl::list<const char8_t*>::const_iterator ksStrEnd = keyScopeStringList->end();
        for (; ksStrItr != ksStrEnd; ++ksStrItr)
        {
            // this part is just a copy of the logic below for single-value keyscopes update
            scopeIter = keyScopeLeaderboardMap->find(*ksStrItr);
            if (scopeIter == keyScopeLeaderboardMap->end())
                continue; // we dont have any boards with this keyscopes
            HistoricalLeaderboards* historicalLeaderboards = scopeIter->second;
            LeaderboardList* leaderboardList = historicalLeaderboards->getLeaderboardList();
            LeaderboardList::const_iterator lbIter = leaderboardList->begin();
            LeaderboardList::const_iterator lbEnd = leaderboardList->end();
            for (; lbIter != lbEnd; ++lbIter)
            {
                if (!(*lbIter)->isInMemory())
                    continue;

                UpdateLeaderboardResult result;
                updateIndexes(*lbIter, entityId, periodId, statValMap, historicalLeaderboards->getPeriodIdMap(), statMap, result);
                if (result.mStatus == UpdateLeaderboardResult::NEWLY_RANKED || result.mStatus == UpdateLeaderboardResult::STILL_RANKED)
                    upsert = true;
                else if (result.mStatus == UpdateLeaderboardResult::NO_LONGER_RANKED)
                    remove = true;

                // If we bumped another entity out, and that entity is not ranked in any other relevant leaderboard, we can purge them from the DB
                if (result.mRemovedEntityId != 0 && !isRanked(result.mRemovedEntityId, categoryId, periodType, periodId, scopeMap))
                    makeDbTablesDelete(lbQueryString, result.mRemovedEntityId, categoryId, periodType, periodId, scopeMap);
            }
        }
    }

    // now for the single-value keyscopes update
    scopeIter = keyScopeLeaderboardMap->find(keyScopeString);
    if (scopeIter != keyScopeLeaderboardMap->end())
    {
        HistoricalLeaderboards* historicalLeaderboards = scopeIter->second;
        LeaderboardList* leaderboardList = historicalLeaderboards->getLeaderboardList();
        LeaderboardList::const_iterator lbIter = leaderboardList->begin();
        LeaderboardList::const_iterator lbEnd = leaderboardList->end();
        for (; lbIter != lbEnd; ++lbIter)
        {
            if (!(*lbIter)->isInMemory())
                continue;

            UpdateLeaderboardResult result;
            updateIndexes(*lbIter, entityId, periodId, statValMap, historicalLeaderboards->getPeriodIdMap(), statMap, result);
            if (result.mStatus == UpdateLeaderboardResult::NEWLY_RANKED || result.mStatus == UpdateLeaderboardResult::STILL_RANKED)
                upsert = true;
            else if (result.mStatus == UpdateLeaderboardResult::NO_LONGER_RANKED)
                remove = true;

            // If we bumped another entity out, and that entity is not ranked in any other relevant leaderboard, we can purge them from the DB
            if (result.mRemovedEntityId != 0 && !isRanked(result.mRemovedEntityId, categoryId, periodType, periodId, scopeMap))
                makeDbTablesDelete(lbQueryString, result.mRemovedEntityId, categoryId, periodType, periodId, scopeMap);
        }
    }

    // Note upsert takes precedence over remove - if we became newly ranked or are still ranked in any leaderboard, we only upsert data,
    // while we only remove if we fell out of some leaderboard and are no longer ranked anywhere else either
    if (upsert)
        makeDbTablesUpsert(lbQueryString, entityId, categoryId, periodType, periodId, scopeMap, keyScopeLeaderboards->getStatNames(), statValMap);
    else if (remove && !isRanked(entityId, categoryId, periodType, periodId, scopeMap))
        makeDbTablesDelete(lbQueryString, entityId, categoryId, periodType, periodId, scopeMap);
}

/*************************************************************************************************/
/*!
    \brief updateIndexes

    This method is called by updateStats on the core slave executing a stat update transaction.
    On period rollover the first update will need to construct a new data structure to hold
    the new period's ranking data, but the main purpose of this method is to determine if
    for a particular leaderboard any stat has changed which would impact it, then attempt
    to apply the change, and report the results back to the caller so it can then determine
    whether it should broadcast the changes out / apply them to the database.

    \param[in]  leaderboard - the leaderboard definition
    \param[in]  entityId - entity id who's stats are in statValMap
    \param[in]  periodId - period id of the stats
    \param[in]  statValMap - the full stats map for the row, which may contain both changed and unchanged values
    \param[in/out] periodIdMap - map that either already contains the actual leaderboard data structure, or in case of rollover will be updated to contain a newly created one
    \param[out] statMap - map of stat data that needs to be broadcasted, we will add to it if we modify a leaderboard in the course of this method
    \param[out] result - structure to report back to the caller, including whether we added / modified / removed the entity id, plus who we bumped out
*/
/*************************************************************************************************/
void LeaderboardIndexes::updateIndexes(const StatLeaderboard* leaderboard, EntityId entityId, int32_t periodId, const StatValMap* statValMap,
                                       PeriodIdMap* periodIdMap, StatNameValueMap& statMap, UpdateLeaderboardResult& result)
{
    // First chunk of method handles the relatively rare case of constructing a new data structure if
    // this is the first update of a particular leaderboard post periodic rollover
    LeaderboardMap* leaderboardMap;
    PeriodIdMap::const_iterator periodIter = periodIdMap->find(periodId);
    if (EA_UNLIKELY(periodIter == periodIdMap->end()))
    {
        leaderboardMap = BLAZE_NEW_STATS_LEADERBOARD LeaderboardMap();
        periodIdMap->insert(eastl::make_pair(periodId, leaderboardMap));
    }
    else
    {
        leaderboardMap = periodIter->second;
    }

    LeaderboardIndexBase* leaderboardIndex;
    LeaderboardMap::const_iterator lbIter = leaderboardMap->find(leaderboard->getBoardId());
    if (EA_UNLIKELY(lbIter == leaderboardMap->end()))
    {
        // Newly added leaderboards in this method should be due to period rollover, and thus on both
        // master and slave they are ready to be added to immediately and do not require populating
        if (leaderboard->hasSecondaryRankings())
            leaderboardIndex = BLAZE_NEW_STATS_LEADERBOARD LeaderboardIndex<MultiRankData>(leaderboard);
        else if (leaderboard->getStat()->getDbType().getType() == STAT_TYPE_FLOAT)
            leaderboardIndex = BLAZE_NEW_STATS_LEADERBOARD LeaderboardIndex<float_t>(leaderboard);
        else
        {
            if (leaderboard->getStat()->getTypeSize() == 8)
            {
                leaderboardIndex = BLAZE_NEW_STATS_LEADERBOARD LeaderboardIndex<int64_t>(leaderboard);
            }
            else
            {
                leaderboardIndex = BLAZE_NEW_STATS_LEADERBOARD LeaderboardIndex<int32_t>(leaderboard);
            }
        }
        leaderboardMap->insert(eastl::make_pair(leaderboard->getBoardId(), leaderboardIndex));
    }
    else
    {
        leaderboardIndex = lbIter->second;
    }

    // Second portion of this method applies the change to the leaderboard data structure
    if (leaderboard->hasSecondaryRankings())
    {
        // If even a single stat out of the multi-valued rank data changes, we will both update the
        // leaderboard and need to broadcast out the values of all stats involved in this key
        bool anyChanged = false;
        MultiRankData value;
        for (int32_t pos = 0; pos < leaderboard->getRankingStatsSize(); ++pos)
        {
            StatValMap::const_iterator it = statValMap->find(leaderboard->getRankingStatName(pos));
            if (it != statValMap->end())
            {
                if (it->second->changed)
                    anyChanged = true;

                if (leaderboard->getRankingStat(pos)->getDbType().getType() == STAT_TYPE_FLOAT)
                    value.data[pos].floatValue = it->second->floatVal;
                else
                    value.data[pos].intValue = it->second->intVal;
            }
        }

        if (anyChanged)
            (static_cast<LeaderboardIndex<MultiRankData>*>(leaderboardIndex))->update(entityId, value, result);

        if (result.mStatus != UpdateLeaderboardResult::NOT_RANKED)
        {
            for (int32_t pos = 0; pos < leaderboard->getRankingStatsSize(); ++pos)
            {
                char8_t valueBuf[STATS_STATVALUE_LENGTH];
                if (leaderboard->getRankingStat(pos)->getDbType().getType() == STAT_TYPE_FLOAT)
                    blaze_snzprintf(valueBuf, sizeof(valueBuf), "%f", value.data[pos].floatValue);
                else if (leaderboard->getRankingStat(pos)->getTypeSize() == 8)
                    blaze_snzprintf(valueBuf, sizeof(valueBuf), "%" PRId64, value.data[pos].intValue);
                else
                    blaze_snzprintf(valueBuf, sizeof(valueBuf), "%" PRId32, static_cast<int32_t>(value.data[pos].intValue));

                statMap.insert(eastl::make_pair(leaderboard->getRankingStatName(pos), valueBuf));
            }
        }
    }
    else
    {
        StatValMap::const_iterator it = statValMap->find(leaderboard->getRankingStatName(0));
        if (it != statValMap->end() && it->second->changed)
        {
            if (leaderboard->getStat()->getDbType().getType() == STAT_TYPE_FLOAT)
                (static_cast<LeaderboardIndex<float_t>*>(leaderboardIndex))->update(entityId, it->second->floatVal, result);
            else if (leaderboard->getStat()->getTypeSize() == 8)
                (static_cast<LeaderboardIndex<int64_t>*>(leaderboardIndex))->update(entityId, it->second->intVal, result);
            else
                (static_cast<LeaderboardIndex<int32_t>*>(leaderboardIndex))->update(entityId, static_cast<int32_t>(it->second->intVal), result);

            if (result.mStatus != UpdateLeaderboardResult::NOT_RANKED)
            {
                char8_t valueBuf[STATS_STATVALUE_LENGTH];
                if (leaderboard->getStat()->getDbType().getType() == STAT_TYPE_FLOAT)
                    blaze_snzprintf(valueBuf, sizeof(valueBuf), "%f", it->second->floatVal);
                else if (leaderboard->getStat()->getTypeSize() == 8)
                    blaze_snzprintf(valueBuf, sizeof(valueBuf), "%" PRId64, it->second->intVal);
                else
                    blaze_snzprintf(valueBuf, sizeof(valueBuf), "%" PRId32, static_cast<int32_t>(it->second->intVal));

                statMap.insert(eastl::make_pair(it->first, valueBuf));
            }
        }
    }
}

/*************************************************************************************************/
/*!
    \brief makeDbTablesUpsert

    Appends to a caller provided query object to add or update a row in the database leaderboard table.

    \param[out]  lbQueryString - the query string builder to append to
    \param[in]  entityId - the entity id who's stats are changing
    \param[in]  categoryId - category id of the update
    \param[in]  periodType - period type of the update
    \param[in]  periodId - period id of the update
    \param[in]  scopeMap - key scopes of the update
    \param[in]  statNames - a set of all the stat columns that should be present on the DB table
    \param[in]  statValMap - map containing all stats for the entity for a row (must contain at least statNames-worth of stats if not more)
*/
/*************************************************************************************************/
void LeaderboardIndexes::makeDbTablesUpsert(StringBuilder& lbQueryString, EntityId entityId, uint32_t categoryId, int32_t periodType, int32_t periodId,
                                            const ScopeNameValueMap* scopeMap, const StatNameSet& statNames, const StatValMap* statValMap)
{
    const CategoryIdMap* categoryMap = mComponent->getConfigData()->getCategoryIdMap();
    CategoryIdMap::const_iterator catIter = categoryMap->find(categoryId);

    const char8_t* statTableName = catIter->second->getTableName(periodType);
    char8_t table[LB_MAX_TABLE_NAME_LENGTH];
    getLeaderboardTableName(statTableName, table, sizeof(table));

    lbQueryString.append("INSERT INTO `%s` (`entity_id`", table);

    if (periodType != STAT_PERIOD_ALL_TIME)
    {
        lbQueryString.append(",`period_id`");
    }

    //keyscope columns
    ScopeNameValueMap::const_iterator scopeItr = scopeMap->begin();
    ScopeNameValueMap::const_iterator scopeEnd = scopeMap->end();
    for (; scopeItr != scopeEnd; ++scopeItr)
    {
        lbQueryString.append(",`%s`", scopeItr->first.c_str());
    }

    for (StatNameSet::const_iterator it = statNames.begin(); it != statNames.end(); ++it)
    {
        lbQueryString.append(",`%s`", *it);
    }

    lbQueryString.append(") VALUES (%" PRIi64, entityId);
    if (periodType != STAT_PERIOD_ALL_TIME)
    {
        lbQueryString.append(",%" PRIu32, periodId);
    }

    //keyscope columns
    scopeItr = scopeMap->begin();
    for (; scopeItr != scopeEnd; ++scopeItr)
    {
        lbQueryString.append(",%" PRIi64, scopeItr->second);
    }

    for (StatNameSet::const_iterator it = statNames.begin(); it != statNames.end(); ++it)
    {
        StatValMap::const_iterator val = statValMap->find(*it);
        if (val->second->type == STAT_TYPE_FLOAT)
            lbQueryString.append(",%f", val->second->floatVal);
        else
            lbQueryString.append(",%" PRIi64, val->second->intVal);
    }

    lbQueryString.append(") ON DUPLICATE KEY UPDATE");
    for (StatNameSet::const_iterator it = statNames.begin(); it != statNames.end(); ++it)
    {
        lbQueryString.append(" `%s` = VALUES(`%s`),", *it, *it);
    }

    lbQueryString.trim(1);
    lbQueryString.append(";");
}

/*************************************************************************************************/
/*!
    \brief makeDbTablesDelete

    Appends to a caller provided query object to remove a row from the database leaderboard table.
    This could occur either because an entity's stats have decreased enough to drop them out,
    or because other entity's have surpassed them and pushed them out.

    \param[out]  lbQueryString - the query string builder to append to
    \param[in]  entityId - the entity id being removed
    \param[in]  categoryId - category id of the removal
    \param[in]  periodType - period type of the removal
    \param[in]  periodId - period id of the removal
    \param[in]  scopeMap - key scopes of the removal
*/
/*************************************************************************************************/
void LeaderboardIndexes::makeDbTablesDelete(StringBuilder& lbQueryString, EntityId entityId, uint32_t categoryId, int32_t periodType, int32_t periodId, const ScopeNameValueMap* scopeMap)
{
    const CategoryIdMap* categoryMap = mComponent->getConfigData()->getCategoryIdMap();
    CategoryIdMap::const_iterator catIter = categoryMap->find(categoryId);

    const char8_t* statTableName = catIter->second->getTableName(periodType);
    char8_t table[LB_MAX_TABLE_NAME_LENGTH];
    getLeaderboardTableName(statTableName, table, sizeof(table));

    lbQueryString.append("DELETE FROM `%s` WHERE `entity_id` = %" PRIi64, table, entityId);

    if (periodType != STAT_PERIOD_ALL_TIME)
    {
        lbQueryString.append(" AND `period_id` = %" PRIu32, periodId);
    }

    //keyscope columns
    ScopeNameValueMap::const_iterator scopeItr = scopeMap->begin();
    ScopeNameValueMap::const_iterator scopeEnd = scopeMap->end();
    for (; scopeItr != scopeEnd; ++scopeItr)
    {
        lbQueryString.append(" AND `%s` = %" PRIi64, scopeItr->first.c_str(), scopeItr->second);
    }
    lbQueryString.append(";");
}


/*************************************************************************************************/
/*!
    \brief isRanked

    Lookup whether an entity is in any leaderboards of the given category, period and keyscope.

    \param[in]  entityId - the entity to lookup.
    \param[in]  categoryId - category id to check
    \param[in]  periodType - period type to check
    \param[in]  periodId - period id to check
    \param[in]  scopeMap - scope map to check

    \return - whether the entity is in any applicable leaderboards
*/
/*************************************************************************************************/
bool LeaderboardIndexes::isRanked(EntityId entityId, uint32_t categoryId, int32_t periodType, int32_t periodId, const ScopeNameValueMap* scopeMap)
{
    using namespace eastl;
    
    CategoryMap::const_iterator catItr = mCategoryMap.find(categoryId);
    if (catItr == mCategoryMap.end())
    {
        return false;
    }

    PeriodTypeMap* periodTypeMap = catItr->second;
    PeriodTypeMap::const_iterator periodTypeItr = periodTypeMap->find(periodType);
    if (periodTypeItr == periodTypeMap->end())
    {
        return false;
    }

    char8_t* scopeKey = makeKeyScopeString(scopeMap);

    KeyScopeLeaderboards* keyScopeLeaderboards = periodTypeItr->second;
    KeyScopeLeaderboardMap::const_iterator keyScopeItr = keyScopeLeaderboards->getKeyScopeLeaderboardMap()->find(scopeKey);
    // get single-to-multi-value keyscope map now ...
    KeyScopeSingleMultiValueMap* keyScopeSingleMultiValueMap = keyScopeLeaderboards->getKeyScopeSingleMultiValueMap();
    KeyScopeSingleMultiValueMap::const_iterator smItr = keyScopeSingleMultiValueMap->find(scopeKey);
    // so that we don't need this string anymore ...
    delete[] scopeKey;

    // check single-value keyscopes first
    if (keyScopeItr != keyScopeLeaderboards->getKeyScopeLeaderboardMap()->end())
    {
        HistoricalLeaderboards* histLb = keyScopeItr->second;
        PeriodIdMap::iterator periodItr = histLb->getPeriodIdMap()->find(periodId);
        if (periodItr != histLb->getPeriodIdMap()->end())
        {
            LeaderboardMap* leaderboardMap = periodItr->second;
            LeaderboardMap::const_iterator lbItr = leaderboardMap->begin();
            LeaderboardMap::const_iterator lbEnd = leaderboardMap->end();
            for (; lbItr != lbEnd; ++lbItr)
            {
                if (lbItr->second->containsEntity(entityId))
                    return true;
            }
        }
    }

    // then check any multi-value keyscopes that this single-value keyscopes affects
    if (smItr != keyScopeSingleMultiValueMap->end())
    {
        list<const char8_t*>* keyScopeStringList = smItr->second;
        list<const char8_t*>::const_iterator ksStrItr = keyScopeStringList->begin();
        list<const char8_t*>::const_iterator ksStrEnd = keyScopeStringList->end();
        for (; ksStrItr != ksStrEnd; ++ksStrItr)
        {
            // this part is just a copy of the logic above for single-value keyscopes check
            keyScopeItr = keyScopeLeaderboards->getKeyScopeLeaderboardMap()->find(*ksStrItr);
            if (keyScopeItr == keyScopeLeaderboards->getKeyScopeLeaderboardMap()->end())
            {
                continue; // we dont have any boards with this keyscopes
            }

            HistoricalLeaderboards* histLb = keyScopeItr->second;
            PeriodIdMap::iterator periodItr = histLb->getPeriodIdMap()->find(periodId);
            if (periodItr != histLb->getPeriodIdMap()->end())
            {
                LeaderboardMap* leaderboardMap = periodItr->second;
                LeaderboardMap::const_iterator lbItr = leaderboardMap->begin();
                LeaderboardMap::const_iterator lbEnd = leaderboardMap->end();
                for (; lbItr != lbEnd; ++lbItr)
                {
                    if (lbItr->second->containsEntity(entityId))
                        return true;
                }
            }
        }
    }

    return false;
}

/*************************************************************************************************/
/*!
    \brief deleteStats

    Deletes stats from ranking tables when stats are deleted from the server.
    Note that unlike updateStats where there are enough code differences to warrant separate
    versions on the core slave where the stat update transaction is executing versus the remote
    side where the broadcast is applied on all other instances - in the delete stats case we can
    use essentially the same code, except for skipping the appending to the DB query.  Thus,
    the caller should pass in an actual query object if they want it populated, or an empty
    query pointer to bypass this.

    \param[in]  data - data to be deleted
    \param[in]  currentPeriodIdMap - map of current period ids for each period type
    \param[out] lbQueryString - query string builder to populate with any updates required to leaderboard database tables
*/
/*************************************************************************************************/
void LeaderboardIndexes::deleteStats(const CacheRowDeleteList& data, const Stats::PeriodIdMap& currentPeriodIdMap, StringBuilder* lbQueryString)
{
    using namespace eastl;
    
    TRACE_LOG("[LeaderboardIndexes].deleteStats: starting. " << data.size() << " request rows to delete");

    const Stats::CategoryIdMap* statsCategoryIdMap = mComponent->getConfigData()->getCategoryIdMap();

    CacheRowDeleteList::const_iterator deleteIter = data.begin();
    CacheRowDeleteList::const_iterator deleteEnd = data.end();
    for (; deleteIter != deleteEnd; ++deleteIter)
    {
        const CacheRowDelete* deleteRow = *deleteIter;
        uint32_t categoryId = deleteRow->getCategoryId();

        // First check that the category is used in any in-memory leaderboards
        CategoryMap::const_iterator catIter = mCategoryMap.find(categoryId);
        if (catIter == mCategoryMap.end())
        {
            TRACE_LOG("[LeaderboardIndexes].deleteStats: attempt on unused category id: " << categoryId);
            continue;
        }

        Stats::CategoryIdMap::const_iterator cit = statsCategoryIdMap->find(categoryId);
        if (cit == statsCategoryIdMap->end())
        {
            ERR_LOG("[LeaderboardIndexes].deleteStats: attempt on unknown category id: " << categoryId);
            continue;
        }
        const StatCategory* category = cit->second;

        PeriodTypeMap* periodTypeMap = catIter->second;

        // Iterate through period types
        PeriodTypeList::const_iterator periodIter = deleteRow->getPeriodTypes().begin();
        PeriodTypeList::const_iterator periodEnd = deleteRow->getPeriodTypes().end();
        for (; periodIter != periodEnd; ++periodIter)
        {
            bool updateDb = false;
            int32_t periodType = *periodIter;
            PeriodTypeMap::const_iterator perTypeIter = periodTypeMap->find(periodType);
            if (perTypeIter == periodTypeMap->end())
                continue;

            int32_t currentPeriodId;
            Stats::PeriodIdMap::const_iterator cpi = currentPeriodIdMap.find(periodType);
            if (cpi != currentPeriodIdMap.end())
            {
                currentPeriodId = cpi->second;
            }
            else
            {
                ERR_LOG("[LeaderboardIndexes].deleteStats: missing current period id; using default for\n"
                    << deleteRow);
                currentPeriodId = mComponent->getPeriodId(periodType);
            }

            // for each row entry, we expect keyscopes to be fully-specified (and validation assumed)
            if (!category->isValidScopeNameSet(&deleteRow->getKeyScopeNameValueMap()))
            {
                ERR_LOG("[LeaderboardIndexes].deleteStats: invalid keyscope for\n"
                    << deleteRow);
                continue;
            }

            char8_t* keyScopeString = makeKeyScopeString(&deleteRow->getKeyScopeNameValueMap());

            TRACE_LOG("[LeaderboardIndexes].deleteStats: period type <" << periodType
                << "> category id <" << categoryId << "> keyscope: " << keyScopeString);

            KeyScopeLeaderboards* keyScopeLeaderboards = perTypeIter->second;
            KeyScopeLeaderboardMap* keyScopeLeaderboardMap = keyScopeLeaderboards->getKeyScopeLeaderboardMap();
            KeyScopeLeaderboardMap::const_iterator ksIter;

            // look for any multi-value keyscopes that this single-value keyscopes affects
            KeyScopeSingleMultiValueMap* keyScopeSingleMultiValueMap = keyScopeLeaderboards->getKeyScopeSingleMultiValueMap();
            KeyScopeSingleMultiValueMap::const_iterator smItr = keyScopeSingleMultiValueMap->find(keyScopeString);
            if (smItr != keyScopeSingleMultiValueMap->end())
            {
                list<const char8_t*>* keyScopeStringList = smItr->second;
                list<const char8_t*>::const_iterator ksStrItr = keyScopeStringList->begin();
                list<const char8_t*>::const_iterator ksStrEnd = keyScopeStringList->end();
                for (; ksStrItr != ksStrEnd; ++ksStrItr)
                {
                    ksIter = keyScopeLeaderboardMap->find(*ksStrItr);
                    if (ksIter == keyScopeLeaderboardMap->end())
                        continue; // we dont have any boards with this keyscopes

                    // this part is just a copy of the logic below for single-value keyscopes update
                    HistoricalLeaderboards* historicalLeaderboards = ksIter->second;
                    PeriodIdMap* periodIdMap = historicalLeaderboards->getPeriodIdMap();
                    PeriodIdMap::const_iterator idEnd = periodIdMap->end();

                    if (perTypeIter->first == STAT_PERIOD_ALL_TIME || deleteRow->getPerOffsets().empty())
                    {
                        PeriodIdMap::const_iterator idIter = periodIdMap->begin();
                        for (; idIter != idEnd; ++idIter)
                        {
                            LeaderboardMap* leaderboardMap = idIter->second;
                            LeaderboardMap::const_iterator lbIter = leaderboardMap->begin();
                            LeaderboardMap::const_iterator lbEnd = leaderboardMap->end();
                            for (; lbIter != lbEnd; ++lbIter)
                            {
                                LeaderboardIndexBase* leaderboardIndex = lbIter->second;
                                UpdateLeaderboardResult result;
                                leaderboardIndex->remove(deleteRow->getEntityId(), result);

                                updateDb = true;
                            }
                        }
                    }
                    else
                    {
                        OffsetList::const_iterator perOffsetIter = deleteRow->getPerOffsets().begin();
                        OffsetList::const_iterator perOffsetEnd = deleteRow->getPerOffsets().end();
                        for (;perOffsetIter != perOffsetEnd; ++perOffsetIter)
                        {
                            int32_t deletedPeriodId = currentPeriodId - static_cast<int32_t>(*perOffsetIter);
                            PeriodIdMap::const_iterator idIter = periodIdMap->find(deletedPeriodId);
                            if (idIter == idEnd)
                                continue;

                            LeaderboardMap* leaderboardMap = idIter->second;
                            LeaderboardMap::const_iterator lbIter = leaderboardMap->begin();
                            LeaderboardMap::const_iterator lbEnd = leaderboardMap->end();
                            for (; lbIter != lbEnd; ++lbIter)
                            {
                                LeaderboardIndexBase* leaderboardIndex = lbIter->second;
                                UpdateLeaderboardResult result;
                                leaderboardIndex->remove(deleteRow->getEntityId(), result);

                                updateDb = true;
                            }
                        }
                    }
                }// for multi-keyscope-value leaderboards
            }// if multi-keyscope-value leaderboards

            // now for the single-value keyscopes update
            ksIter = keyScopeLeaderboardMap->find(keyScopeString);
            delete[] keyScopeString;
            if (ksIter != keyScopeLeaderboardMap->end())
            {
                HistoricalLeaderboards* historicalLeaderboards = ksIter->second;
                PeriodIdMap* periodIdMap = historicalLeaderboards->getPeriodIdMap();
                PeriodIdMap::const_iterator idEnd = periodIdMap->end();

                if (perTypeIter->first == STAT_PERIOD_ALL_TIME || deleteRow->getPerOffsets().empty())
                {
                    PeriodIdMap::const_iterator idIter = periodIdMap->begin();
                    for (; idIter != idEnd; ++idIter)
                    {
                        LeaderboardMap* leaderboardMap = idIter->second;
                        LeaderboardMap::const_iterator lbIter = leaderboardMap->begin();
                        LeaderboardMap::const_iterator lbEnd = leaderboardMap->end();
                        for (; lbIter != lbEnd; ++lbIter)
                        {
                            LeaderboardIndexBase* leaderboardIndex = lbIter->second;
                            UpdateLeaderboardResult result;
                            leaderboardIndex->remove(deleteRow->getEntityId(), result);

                            updateDb = true;
                        }
                    }
                }
                else
                {
                    OffsetList::const_iterator perOffsetIter = deleteRow->getPerOffsets().begin();
                    OffsetList::const_iterator perOffsetEnd = deleteRow->getPerOffsets().end();
                    for (;perOffsetIter != perOffsetEnd; ++perOffsetIter)
                    {
                        int32_t deletedPeriodId = currentPeriodId - static_cast<int32_t>(*perOffsetIter);
                        PeriodIdMap::const_iterator idIter = periodIdMap->find(deletedPeriodId);
                        if (idIter == idEnd)
                            continue;

                        LeaderboardMap* leaderboardMap = idIter->second;
                        LeaderboardMap::const_iterator lbIter = leaderboardMap->begin();
                        LeaderboardMap::const_iterator lbEnd = leaderboardMap->end();
                        for (; lbIter != lbEnd; ++lbIter)
                        {
                            LeaderboardIndexBase* leaderboardIndex = lbIter->second;
                            UpdateLeaderboardResult result;
                            leaderboardIndex->remove(deleteRow->getEntityId(), result);

                            updateDb = true;
                        }
                    }
                }
            }// if single-keyscope-value leaderboard

            // update the db
            // we want to make sure that at least one leaderboard is impacted by this stat delete to avoid unnecessary
            // database queries, so we set the category in the inner most loop where we update the in-memory indexes
            if (lbQueryString != nullptr && updateDb)
            {
                const char8_t* statTableName = category->getTableName(periodType);
                char8_t lbTableName[LB_MAX_TABLE_NAME_LENGTH];
                getLeaderboardTableName(statTableName, lbTableName, LB_MAX_TABLE_NAME_LENGTH);

                lbQueryString->append("DELETE FROM `%s` WHERE `entity_id` = %" PRIi64, lbTableName, deleteRow->getEntityId());
                if ((periodType != STAT_PERIOD_ALL_TIME) && !deleteRow->getPerOffsets().empty())
                {
                    lbQueryString->append(" AND `period_id` IN (");
                    bool needsComma = false;
                    for (OffsetList::const_iterator perItr = deleteRow->getPerOffsets().begin(), peEnd = deleteRow->getPerOffsets().end(); perItr != peEnd; ++perItr)
                    {
                        if (needsComma)
                            lbQueryString->append(",");
                        lbQueryString->append("%" PRIu32, (currentPeriodId - static_cast<int32_t>(*perItr)));
                        needsComma = true;
                    }
                    lbQueryString->append(")");
                }

                //keyscope columns
                ScopeNameValueMap::const_iterator scopeItr = deleteRow->getKeyScopeNameValueMap().begin();
                ScopeNameValueMap::const_iterator scopeEnd = deleteRow->getKeyScopeNameValueMap().end();
                for (; scopeItr != scopeEnd; ++scopeItr)
                {
                    lbQueryString->append(" AND `%s` = %" PRIi64, scopeItr->first.c_str(), scopeItr->second);
                }
                lbQueryString->append(";");
            }
        }// for period types
    }
    TRACE_LOG("[LeaderboardIndexes].deleteStats: done");
}

/*************************************************************************************************/
/*!
    \brief deleteStatsByEntity

    Deletes stats from ranking tables when stats are deleted from the server.
    All stats for an entity (per category) are deleted regardless of keyscope.
    Note that unlike updateStats where there are enough code differences to warrant separate
    versions on the core slave where the stat update transaction is executing versus the remote
    side where the broadcast is applied on all other instances - in the delete stats case we can
    use essentially the same code, except for skipping the appending to the DB query.  Thus,
    the caller should pass in an actual query object if they want it populated, or an empty
    query pointer to bypass this.

    \param[in]  data - data to be deleted
    \param[in]  currentPeriodIdMap - map of current period ids for each period type
    \param[out] lbQueryString - query string builder to populate with any updates required to leaderboard database tables
*/
/*************************************************************************************************/
void LeaderboardIndexes::deleteStatsByEntity(const CacheRowDeleteList& data, const Stats::PeriodIdMap& currentPeriodIdMap, StringBuilder* lbQueryString)
{
    TRACE_LOG("[LeaderboardIndexes].deleteStatsByEntity: starting. " << data.size() << " request rows to delete");

    const Stats::CategoryIdMap* statsCategoryIdMap = mComponent->getConfigData()->getCategoryIdMap();

    CacheRowDeleteList::const_iterator deleteIter = data.begin();
    CacheRowDeleteList::const_iterator deleteEnd = data.end();
    for (; deleteIter != deleteEnd; ++deleteIter)
    {
        const CacheRowDelete* deleteRow = *deleteIter;
        uint32_t categoryId = deleteRow->getCategoryId();

        // First check that the category is used in any in-memory leaderboards
        CategoryMap::const_iterator catIter = mCategoryMap.find(categoryId);
        if (catIter == mCategoryMap.end())
        {
            TRACE_LOG("[LeaderboardIndexes].deleteStatsByEntity: attempt on unused category id: " << categoryId);
            continue;
        }

        Stats::CategoryIdMap::const_iterator cit = statsCategoryIdMap->find(categoryId);
        if (cit == statsCategoryIdMap->end())
        {
            ERR_LOG("[LeaderboardIndexes].deleteStatsByEntity: attempt on unknown category id: " << categoryId);
            continue;
        }
        const StatCategory* category = cit->second;

        PeriodTypeMap* periodTypeMap = catIter->second;

        // Iterate through period types
        PeriodTypeList::const_iterator periodIter = deleteRow->getPeriodTypes().begin();
        PeriodTypeList::const_iterator periodEnd = deleteRow->getPeriodTypes().end();
        for (; periodIter != periodEnd; ++periodIter)
        {
            bool updateDb = false;
            int32_t periodType = *periodIter;
            PeriodTypeMap::const_iterator perTypeIter = periodTypeMap->find(periodType);
            if (perTypeIter == periodTypeMap->end())
                continue;

            int32_t currentPeriodId;
            Stats::PeriodIdMap::const_iterator cpi = currentPeriodIdMap.find(periodType);
            if (cpi != currentPeriodIdMap.end())
            {
                currentPeriodId = cpi->second;
            }
            else
            {
                ERR_LOG("[LeaderboardIndexes].deleteStatsByEntity: missing current period id; using default for\n"
                    << deleteRow);
                currentPeriodId = mComponent->getPeriodId(periodType);
            }

            // Delete stats for all keyscopes to be consistent with db operation
            KeyScopeLeaderboards* keyScopeLeaderboards = perTypeIter->second;
            KeyScopeLeaderboardMap::const_iterator ksIter = keyScopeLeaderboards->getKeyScopeLeaderboardMap()->begin();
            KeyScopeLeaderboardMap::const_iterator ksEnd = keyScopeLeaderboards->getKeyScopeLeaderboardMap()->end();
            for (; ksIter != ksEnd; ++ksIter)
            {
                HistoricalLeaderboards* historicalLeaderboards = ksIter->second;
                PeriodIdMap* periodIdMap = historicalLeaderboards->getPeriodIdMap();
                PeriodIdMap::const_iterator idEnd = periodIdMap->end();

                if (perTypeIter->first == STAT_PERIOD_ALL_TIME || deleteRow->getPerOffsets().empty())
                {
                    PeriodIdMap::const_iterator idIter = periodIdMap->begin();
                    for (; idIter != idEnd; ++idIter)
                    {
                        LeaderboardMap* leaderboardMap = idIter->second;
                        LeaderboardMap::const_iterator lbIter = leaderboardMap->begin();
                        LeaderboardMap::const_iterator lbEnd = leaderboardMap->end();
                        for (; lbIter != lbEnd; ++lbIter)
                        {
                            LeaderboardIndexBase* leaderboardIndex = lbIter->second;
                            UpdateLeaderboardResult result;
                            leaderboardIndex->remove(deleteRow->getEntityId(), result);

                            updateDb = true;
                        }
                    }
                }
                else
                {
                    OffsetList::const_iterator perOffsetIter = deleteRow->getPerOffsets().begin();
                    OffsetList::const_iterator perOffsetEnd = deleteRow->getPerOffsets().end();
                    for (;perOffsetIter != perOffsetEnd; ++perOffsetIter)
                    {
                        int32_t deletedPeriodId = currentPeriodId - static_cast<int32_t>(*perOffsetIter);

                        PeriodIdMap::const_iterator idIter = periodIdMap->find(deletedPeriodId);
                        if (idIter == idEnd)
                        {
                            continue;
                        }

                        LeaderboardMap* leaderboardMap = idIter->second;
                        LeaderboardMap::const_iterator lbIter = leaderboardMap->begin();
                        LeaderboardMap::const_iterator lbEnd = leaderboardMap->end();
                        for (; lbIter != lbEnd; ++lbIter)
                        {
                            LeaderboardIndexBase* leaderboardIndex = lbIter->second;
                            UpdateLeaderboardResult result;
                            leaderboardIndex->remove(deleteRow->getEntityId(), result);

                            updateDb = true;
                        }
                    }
                }
            }

            // update the db
            // we want to make sure that at least one leaderboard is impacted by this stat delete to avoid unnecessary
            // database queries, so we set the category in the inner most loop where we update the in memory indexes
            if (lbQueryString != nullptr && updateDb)
            {
                const char8_t* statTableName = category->getTableName(periodType);
                char8_t lbTableName[LB_MAX_TABLE_NAME_LENGTH];
                getLeaderboardTableName(statTableName, lbTableName, LB_MAX_TABLE_NAME_LENGTH);

                lbQueryString->append("DELETE FROM `%s` WHERE `entity_id` = %" PRIi64 ";", lbTableName, deleteRow->getEntityId());
            }
        }
    }// for period types
    TRACE_LOG("[LeaderboardIndexes].deleteStatsByEntity: done");
}

/*************************************************************************************************/
/*!
    \brief deleteCategoryStats

    Deletes all stats from ranking tables when all stats from categories are deleted from the server.
    Note that unlike updateStats where there are enough code differences to warrant separate
    versions on the core slave where the stat update transaction is executing versus the remote
    side where the broadcast is applied on all other instances - in the delete stats case we can
    use essentially the same code, except for skipping the appending to the DB query.  Thus,
    the caller should pass in an actual query object if they want it populated, or an empty
    query pointer to bypass this.

    \param[in]  data - data to be deleted
    \param[in]  currentPeriodIdMap - map of current period ids for each period type
    \param[out] lbQueryString - query string builder to populate with any updates required to leaderboard database tables
    \param[in]  dbTruncateBlocksAllTxn - indicates if TRUNCATE TABLE blocks all transactions (i.e. Galera Cluster DB); only used if query is not nullptr
*/
/*************************************************************************************************/
void LeaderboardIndexes::deleteCategoryStats(const CacheRowDeleteList& data, const Stats::PeriodIdMap& currentPeriodIdMap, StringBuilder* lbQueryString, bool dbTruncateBlocksAllTxn)
{
    CacheRowDeleteList::const_iterator deleteIter = data.begin();
    CacheRowDeleteList::const_iterator deleteEnd = data.end();
    for (; deleteIter != deleteEnd; ++deleteIter)
    {
        const CacheRowDelete* deleteRow = *deleteIter;
        uint32_t categoryId = deleteRow->getCategoryId();

        // First check that the category is used in any in-memory leaderboards
        CategoryMap::const_iterator catIter = mCategoryMap.find(categoryId);
        if (catIter == mCategoryMap.end())
        {
            TRACE_LOG("[LeaderboardIndexes].deleteCategoryStats() - attempt to delete from category id <"
                << categoryId << "> that is not used by any leaderboard.");
            continue;
        }

        PeriodTypeMap* periodTypeMap = catIter->second;

        // Iterate through period types
        PeriodTypeList::const_iterator periodIter = deleteRow->getPeriodTypes().begin();
        PeriodTypeList::const_iterator periodEnd = deleteRow->getPeriodTypes().end();
        for (; periodIter != periodEnd; ++periodIter)
        {
            const StatCategory* category = nullptr;
            int32_t periodType = *periodIter;
            PeriodTypeMap::const_iterator perTypeIter = periodTypeMap->find(periodType);
            if (perTypeIter == periodTypeMap->end())
                continue;

            // Delete stats for all keyscopes to be consistent with db operation
            KeyScopeLeaderboards* keyScopeLeaderboards = perTypeIter->second;
            KeyScopeLeaderboardMap::const_iterator ksIter = keyScopeLeaderboards->getKeyScopeLeaderboardMap()->begin();
            KeyScopeLeaderboardMap::const_iterator ksEnd = keyScopeLeaderboards->getKeyScopeLeaderboardMap()->end();
            for (; ksIter != ksEnd; ++ksIter)
            {
                HistoricalLeaderboards* historicalLeaderboards = ksIter->second;
                PeriodIdMap* periodIdMap = historicalLeaderboards->getPeriodIdMap();
                PeriodIdMap::const_iterator idIter = periodIdMap->begin();
                PeriodIdMap::const_iterator idEnd = periodIdMap->end();
                for (; idIter != idEnd; ++idIter)
                {
                    LeaderboardMap* leaderboardMap = idIter->second;
                    LeaderboardMap::const_iterator lbIter = leaderboardMap->begin();
                    LeaderboardMap::const_iterator lbEnd = leaderboardMap->end();
                    for (; lbIter != lbEnd; ++lbIter)
                    {
                        LeaderboardIndexBase* leaderboardIndex = lbIter->second;
                        leaderboardIndex->reset();

                        category = leaderboardIndex->getLeaderboard()->getStatCategory();
                    }
                }
            }

            // update the db
            // we want to make sure that at least one leaderboard is impacted by this stat delete to avoid unnecessary
            //  database queries, so we set the category in the inner most loop where we update the in memory indexes
            if (lbQueryString != nullptr && category != nullptr)
            {
                /// @todo possible issue where a slave truncates a stats_* table, a rollover occurs,
                // and then the master truncates the corresponding lb_stats_* table.
                // (not sure if we can efficiently compensate using currentPeriodIdMap)
                char8_t lbTableName[LB_MAX_TABLE_NAME_LENGTH];
                getLeaderboardTableName(category->getTableName(periodType), lbTableName, sizeof(lbTableName));

                if (dbTruncateBlocksAllTxn)
                {
                    // as per GOS-31554, DELETE FROM performs better than TRUNCATE TABLE with Galera
                    // due to Total Order Isolation method used for Online Schema Upgrades
                    lbQueryString->append("DELETE FROM `%s`;", lbTableName);
                }
                else
                {
                    lbQueryString->append("TRUNCATE TABLE `%s`;", lbTableName);
                }
            }
        }
    }
}

void LeaderboardIndexes::populateCacheRow(CacheRowUpdate& update, uint32_t catId, EntityId entityId, int32_t periodType, int32_t periodId, const char8_t* keyScopeString)
{
    EA::TDF::TdfBlob& data = update.getBinaryData();
    data.resize(28);
    data.setCount(28);
    uint8_t* bytes = data.getData();
    *((uint32_t*)(bytes)) = catId;
    *((EntityId*)(bytes+4)) = entityId;
    *((int32_t*)(bytes+12)) = periodType;
    *((int32_t*)(bytes+16)) = periodId;
    update.setKeyScopeString(keyScopeString);
     *((uint64_t*)(bytes+20)) = StatsCache::computeCacheRowUpdateHashKey(update);
}

void LeaderboardIndexes::buildKeyScopeString(char8_t* buffer, size_t bufLen, const ScopeNameValueMap& scopeMap)
{
    size_t len = 0;
    len += blaze_snzprintf(buffer, bufLen-len, SCOPE_STRING_PREFIX);

    // TBD Can we move this above and kill even the prefix?
    if (scopeMap.empty())
    {
        return;
    }

    ScopeNameValueMap::const_iterator itr = scopeMap.begin();
    ScopeNameValueMap::const_iterator end = scopeMap.end();
    for (; itr != end; ++itr)
    {
        len += blaze_snzprintf(buffer + len, bufLen-len, "_%" PRId64, itr->second);
    }
}

/*************************************************************************************************/
/*!
    \brief makeKeyScopeString

    Generates string from values of a scope index map. 
    ScopeNameValueMap is a vector map and keys are odered alphabetically descending.

    \param[in]  scopeNameValueListMap - the map containing the values

    \return - pointer to the keyscope string, caller is responsible for deleting memory
*/
/*************************************************************************************************/
char8_t* LeaderboardIndexes::makeKeyScopeString(const ScopeNameValueListMap* scopeNameValueListMap) const
{
    size_t bufLen = 256;
    char8_t* keyScopeString = BLAZE_NEW_ARRAY_STATS_LEADERBOARD(char8_t, bufLen);
    size_t len = 0;
    len += blaze_snzprintf(keyScopeString, bufLen-len, SCOPE_STRING_PREFIX);

    if (scopeNameValueListMap != nullptr && !scopeNameValueListMap->empty())
    {
        ScopeNameValueListMap::const_iterator itr = scopeNameValueListMap->begin();
        ScopeNameValueListMap::const_iterator end = scopeNameValueListMap->end();
        for (; itr != end; ++itr)
        {
            const ScopeValues* scopeValues = itr->second;
            bool first = true;

            ScopeStartEndValuesMap::const_iterator valuesItr = scopeValues->getKeyScopeValues().begin();
            ScopeStartEndValuesMap::const_iterator valuesEnd = scopeValues->getKeyScopeValues().end();
            for (; valuesItr != valuesEnd; ++valuesItr)
            {
                ScopeValue scopeValue;
                for (scopeValue = valuesItr->first; scopeValue <= valuesItr->second; ++scopeValue)
                {
                    growString(keyScopeString, bufLen, len, 32);
                    if (first)
                    {
                        len += blaze_snzprintf(keyScopeString + len, bufLen-len, "_%" PRId64, scopeValue);
                        first = false;
                    }
                    else
                    {
                        len += blaze_snzprintf(keyScopeString + len, bufLen-len, ",%" PRId64, scopeValue);
                    }
                }
            }
        }
    }
    return keyScopeString;
}

/*************************************************************************************************/
/*!
    \brief makeKeyScopeString

    Generates string from values of a scope index map. 
    ScopeNameValueMap is a vector map and keys are odered alphabetically descending.

    \param[in]  scopeNameValueMap - the map containing the values

    \return - pointer to the keyscope string, caller is responsible for deleting memory
*/
/*************************************************************************************************/
char8_t* LeaderboardIndexes::makeKeyScopeString(const ScopeNameValueMap* scopeNameValueMap) const
{
    size_t bufLen = 256;
    char8_t* keyScopeString = BLAZE_NEW_ARRAY_STATS_LEADERBOARD(char8_t, bufLen);
    size_t len = 0;
    len += blaze_snzprintf(keyScopeString, bufLen-len, SCOPE_STRING_PREFIX);

    if (scopeNameValueMap != nullptr && !scopeNameValueMap->empty())
    {
        ScopeNameValueMap::const_iterator itr = scopeNameValueMap->begin();
        ScopeNameValueMap::const_iterator end = scopeNameValueMap->end();
        for (; itr != end; ++itr)
        {
            growString(keyScopeString, bufLen, len, 32); /// @todo possibly overkill here
            len += blaze_snzprintf(keyScopeString + len, bufLen-len, "_%" PRId64, itr->second);
        }
    }
    return keyScopeString;
}

/*************************************************************************************************/
/*!
    \brief makeKeyScopeString

    Generates string from a vector of scope name value pairs.
    This vector only has single-value keyscopes.

    \param[in]  scopeVector - the vector containing the values

    \return - pointer to the keyscope string, caller is responsible for deleting memory
*/
/*************************************************************************************************/
char8_t* LeaderboardIndexes::makeKeyScopeString(const ScopeVector* scopeVector) const
{
    size_t bufLen = 256;
    char8_t* keyScopeString = BLAZE_NEW_ARRAY_STATS_LEADERBOARD(char8_t, bufLen);
    size_t len = 0;
    len += blaze_snzprintf(keyScopeString, bufLen-len, SCOPE_STRING_PREFIX);

    if (scopeVector != nullptr && !scopeVector->empty())
    {
        ScopeVector::const_iterator scopeIter = scopeVector->begin();
        ScopeVector::const_iterator scopeEnd = scopeVector->end();
        for (; scopeIter != scopeEnd; ++scopeIter)
        {
            growString(keyScopeString, bufLen, len, 32); /// @todo possibly overkill here
            len += blaze_snzprintf(keyScopeString + len, bufLen-len, "_%" PRId64, (*scopeIter).currentValue);
        }
    }
    return keyScopeString;
}

/*************************************************************************************************/
/*!
    \brief makeKeyScopeString

    Generates string from a vector of scope name value pairs.

    \param[in]  scopeVector - the vector containing the values
    \param[out]  isMultiValue - whether the scope name value pairs actually have multi-values

    \return - pointer to the keyscope string, caller is responsible for deleting memory
*/
/*************************************************************************************************/
char8_t* LeaderboardIndexes::makeKeyScopeString(const ScopeVector* scopeVector, bool& isMultiValue) const
{
    isMultiValue = false;
    size_t bufLen = 256;
    char8_t* keyScopeString = BLAZE_NEW_ARRAY_STATS_LEADERBOARD(char8_t, bufLen);
    size_t len = 0;
    len += blaze_snzprintf(keyScopeString, bufLen-len, SCOPE_STRING_PREFIX);

    if (scopeVector != nullptr && !scopeVector->empty())
    {
        ScopeVector::const_iterator scopeIter = scopeVector->begin();
        ScopeVector::const_iterator scopeEnd = scopeVector->end();
        for (; scopeIter != scopeEnd; ++scopeIter)
        {
            if ((*scopeIter).currentValue == KEY_SCOPE_VALUE_MULTI)
            {
                bool first = true;
                ScopeStartEndValuesMap::const_iterator valuesItr = (*scopeIter).valuesMap.begin();
                ScopeStartEndValuesMap::const_iterator valuesEnd = (*scopeIter).valuesMap.end();
                for (; valuesItr != valuesEnd; ++valuesItr)
                {
                    ScopeValue scopeValue;
                    for (scopeValue = valuesItr->first; scopeValue <= valuesItr->second; ++scopeValue)
                    {
                        growString(keyScopeString, bufLen, len, 32);
                        if (first)
                        {
                            len += blaze_snzprintf(keyScopeString + len, bufLen-len, "_%" PRId64, scopeValue);
                            first = false;
                        }
                        else
                        {
                            len += blaze_snzprintf(keyScopeString + len, bufLen-len, ",%" PRId64, scopeValue);
                            isMultiValue = true;
                        }
                    }
                }
            }
            else
            {
                len += blaze_snzprintf(keyScopeString + len, bufLen-len, "_%" PRId64, (*scopeIter).currentValue);
            }
        }
    }
    return keyScopeString;
}

/*************************************************************************************************/
/*!
    \brief makeKeyScopeString

    Generates string from a database row, using an ordered set of names to pull the appropriate
    values out of the row.

    \param[in]  scopeNameSet - ordered set of keyscope names
    \param[in]  row - database row that should contain values for each keyscope name

    \return - pointer to the keyscope string, caller is responsible for deleting memory
*/
/*************************************************************************************************/
char8_t* LeaderboardIndexes::makeKeyScopeString(const ScopeNameSet* scopeNameSet, const DbRow* row) const
{
    size_t bufLen = 256;
    char8_t* keyScopeString = BLAZE_NEW_ARRAY_STATS_LEADERBOARD(char8_t, bufLen);
    size_t len = 0;
    len += blaze_snzprintf(keyScopeString, bufLen-len, SCOPE_STRING_PREFIX);

    if (scopeNameSet != nullptr && !scopeNameSet->empty())
    {
        ScopeNameSet::const_iterator scopeIter = scopeNameSet->begin();
        ScopeNameSet::const_iterator scopeEnd = scopeNameSet->end();
        for (; scopeIter != scopeEnd; ++scopeIter)
        {
            growString(keyScopeString, bufLen, len, 32); /// @todo possibly overkill here
            len += blaze_snzprintf(keyScopeString + len, bufLen-len, "_%" PRId64, row->getInt64(scopeIter->c_str()));
        }
    }
    return keyScopeString;
}

bool LeaderboardIndexes::growString(char8_t*& buf, size_t& bufSize, size_t bufCount, size_t bytesNeeded) const
{
    if ((bufSize - bufCount) > bytesNeeded)
        return true;

    if (bytesNeeded < 256)
        bytesNeeded = 256;
    size_t newSize = bufSize + bytesNeeded;
    char8_t* newBuf = BLAZE_NEW_ARRAY_STATS_LEADERBOARD(char8_t, newSize + 1);
    if (newBuf == nullptr)
        return false;
    if (buf != nullptr)
    {
        memcpy(newBuf, buf, bufCount);
        delete[] buf;
    }
    bufSize = newSize;
    buf = newBuf;
    buf[bufCount] = '\0';
    return true;
}

/*************************************************************************************************/
/*!
    \brief trimPeriods

    Deletes leaderboard structures and data corresponding to the expired historical periods as defined
    in the configuration file section 'rollover'.
    
    \param[in]  periodType - period type to trim (STAT_PERIOD_MONTHLY, for example)
    \param[in]  periodsToKeep - number of periods to keep
*/
/*************************************************************************************************/
void LeaderboardIndexes::trimPeriods(int32_t periodType, int32_t periodsToKeep)
{
    mTrimPeriodsTimerIds[periodType] = INVALID_TIMER_ID;

    if (periodType == STAT_PERIOD_ALL_TIME) return;

    TimeValue maxFinishTime = mComponent->getConfigData()->getTrimPeriodsMaxProcessingTime() + TimeValue::getTimeOfDay();
    int32_t lastToKeep = mComponent->getPeriodId(periodType) - periodsToKeep;

    // Go through all structures to find obsolete periods
    LeaderboardIndexes::CategoryMap::const_iterator catIter = mCategoryMap.begin();
    LeaderboardIndexes::CategoryMap::const_iterator catEnd = mCategoryMap.end();
    for (; catIter != catEnd; ++catIter)
    {
        LeaderboardIndexes::PeriodTypeMap::const_iterator periodIterTemp = catIter->second->find(periodType);
        if (periodIterTemp == catIter->second->end())
            continue;
        
        KeyScopeLeaderboardMap* keyScopeLeaderboardMap = periodIterTemp->second->getKeyScopeLeaderboardMap();
        KeyScopeLeaderboardMap::const_iterator ksIter = keyScopeLeaderboardMap->begin();
        KeyScopeLeaderboardMap::const_iterator ksEnd = keyScopeLeaderboardMap->end();
        for (; ksIter != ksEnd; ++ksIter)
        {
            PeriodIdMap* periodIdMap = ksIter->second->getPeriodIdMap();
            PeriodIdMap::iterator periodIter = periodIdMap->begin();
            PeriodIdMap::const_iterator periodEnd = periodIdMap->end();
            while (periodIter != periodEnd)
            {
                // delete all structures for obsolete period 
                // via nested destructors
                if (periodIter->first < lastToKeep)
                {
                    LeaderboardMap::const_iterator lbIter = periodIter->second->begin();
                    while (lbIter != periodIter->second->end())
                    {
                        delete lbIter->second;
                        lbIter = periodIter->second->erase(lbIter);

                        if (TimeValue::getTimeOfDay() >= maxFinishTime)
                        {
                            mTrimPeriodsTimerIds[periodType] = gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay() + mComponent->getConfigData()->getTrimPeriodsTimeout(),
                                this, &LeaderboardIndexes::trimPeriods, periodType, periodsToKeep, "LeaderboardIndexes::trimPeriods");
                            return;
                        }
                    }
                    delete periodIter->second;
                    periodIter = periodIdMap->erase(periodIter);
                }
                else
                {
                    ++periodIter;
                }
            }
        }
    }
}

void LeaderboardIndexes::scheduleTrimPeriods(int32_t periodType, int32_t periodsToKeep)
{
    if (periodType < STAT_NUM_PERIODS && periodType != STAT_PERIOD_ALL_TIME)
    {
        if (mTrimPeriodsTimerIds[periodType] != INVALID_TIMER_ID)
        {
            WARN_LOG("[LeaderboardIndexes].scheduleTrimPeriods:  Period type: " << periodType << "  periodsToKeep: " << periodsToKeep
                << ". Warning: trimming for this period is already in progress.");
            gSelector->cancelTimer(mTrimPeriodsTimerIds[periodType]);
        }
        mTrimPeriodsTimerIds[periodType] = gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay(), this, &LeaderboardIndexes::trimPeriods, periodType, periodsToKeep, "LeaderboardIndexes::trimPeriods");
    }
}

/*************************************************************************************************/
/*!
    \brief getMemoryBudget

    Returns the approximate amount of memory used by the leaderboard indices
*/
/*************************************************************************************************/
uint64_t LeaderboardIndexes::getMemoryBudget() const
{
    return getInfo(&LeaderboardIndexBase::getMemoryBudget);
}

/*************************************************************************************************/
/*!
    \brief getMemoryAllocated

    Returns the approximate amount of memory used by the leaderboard indices
*/
/*************************************************************************************************/
uint64_t LeaderboardIndexes::getMemoryAllocated() const
{
    return getInfo(&LeaderboardIndexBase::getMemoryAllocated);
}

/*************************************************************************************************/
/*!
    \brief getInfo

    Helper method to fetch info about the leaderboards as desired by the passed-in method,
    e.g. getMemoryBudget or getMemoryAllocated
*/
/*************************************************************************************************/
uint64_t LeaderboardIndexes::getInfo(uint64_t (LeaderboardIndexBase::*mf)() const) const
{
    uint64_t memSize = sizeof(CategoryMap);

    LeaderboardIndexes::CategoryMap::const_iterator catIter = mCategoryMap.begin();
    LeaderboardIndexes::CategoryMap::const_iterator catEnd = mCategoryMap.end();
    for (; catIter != catEnd; ++catIter)
    {
        memSize += sizeof(PeriodTypeMap);
        memSize += sizeof(CategoryMap::node_type);
        LeaderboardIndexes::PeriodTypeMap::const_iterator periodIter = catIter->second->begin();
        LeaderboardIndexes::PeriodTypeMap::const_iterator periodEnd = catIter->second->end();
        for (; periodIter != periodEnd; ++periodIter)
        {
            memSize += sizeof(KeyScopeLeaderboards);
            memSize += sizeof(PeriodTypeMap::node_type);
            const KeyScopeLeaderboardMap* ksMap = periodIter->second->getKeyScopeLeaderboardMap();
            LeaderboardIndexes::KeyScopeLeaderboardMap::const_iterator ksIter = ksMap->begin();
            LeaderboardIndexes::KeyScopeLeaderboardMap::const_iterator ksEnd = ksMap->end();
            for (; ksIter != ksEnd; ++ksIter)
            {
                memSize += sizeof(HistoricalLeaderboards);
                memSize += sizeof(KeyScopeLeaderboardMap::node_type);
                const PeriodIdMap* idMap = ksIter->second->getPeriodIdMap();
                LeaderboardIndexes::PeriodIdMap::const_iterator perIdIter = idMap->begin();
                LeaderboardIndexes::PeriodIdMap::const_iterator perIdEnd = idMap->end();
                for (; perIdIter != perIdEnd; ++perIdIter)
                {
                    memSize += sizeof(LeaderboardMap);
                    memSize += sizeof(PeriodIdMap::node_type);
                    const LeaderboardMap* leaderboardMap = perIdIter->second;
                    LeaderboardIndexes::LeaderboardMap::const_iterator lbIter = leaderboardMap->begin();
                    LeaderboardIndexes::LeaderboardMap::const_iterator lbEnd = leaderboardMap->end();
                    for (; lbIter != lbEnd; ++lbIter)
                    {
                        const LeaderboardIndexBase* leaderboardIndex = lbIter->second;
                        if (leaderboardIndex->getLeaderboard()->hasSecondaryRankings())
                        {
                            memSize += sizeof(LeaderboardIndex<MultiRankData>);
                        }
                        if (leaderboardIndex->getLeaderboard()->getStat()->getDbType().getType() == STAT_TYPE_FLOAT)
                        {
                            memSize += sizeof(LeaderboardIndex<float_t>);
                        }
                        else
                        {
                            //If the stat is a 64-bit int 
                            if (leaderboardIndex->getLeaderboard()->getStat()->getTypeSize() == 8)
                            {
                                memSize += sizeof(LeaderboardIndex<int64_t>);
                            }
                            else
                            {
                                memSize += sizeof(LeaderboardIndex<int32_t>);
                            }
                        }
                        memSize += sizeof(LeaderboardMap::node_type);
                        memSize += (uint64_t)(leaderboardIndex->*mf)();
                    }
                }
            }
        }
    }
    return memSize;
}

uint64_t LeaderboardIndexes::getLeaderboardCount() const
{
    uint64_t leaderboardCount = 0;

    for(auto& caIter : mCategoryMap)
    {
        for(auto& periodIter : *caIter.second)
        {
            for(auto& ksIter : *periodIter.second->getKeyScopeLeaderboardMap())
            {
                for(auto& perIdIter : *ksIter.second->getPeriodIdMap())
                {
                    leaderboardCount += perIdIter.second->size();
                }
            }
        }
    }
    return leaderboardCount;
}

uint64_t LeaderboardIndexes::getLeaderboardRowCount() const
{
    uint64_t leaderboardRowCount = 0;

    for(auto& caIter : mCategoryMap)
    {
        for(auto& periodIter : *caIter.second)
        {
            for(auto& ksIter : *periodIter.second->getKeyScopeLeaderboardMap())
            {
                for(auto& perIdIter : *ksIter.second->getPeriodIdMap())
                {
                    for(auto& lbIter : *perIdIter.second)
                    {
                        const LeaderboardIndexBase* leaderboardIndex = lbIter.second;

                        // a bit deceptive because the count returned here won't exceed the configured size (i.e. won't include any "extra" rows)
                        leaderboardRowCount += leaderboardIndex->getCount();
                    }
                }
            }
        }
    }
    return leaderboardRowCount;
}

/*************************************************************************************************/
/*!
    \brief initStatTablesForLeaderboards

    Retrieves the information pertaining to the stats tables that are referenced in the configured
    leaderboards
*/
/*************************************************************************************************/
void LeaderboardIndexes::initStatTablesForLeaderboards()
{
    const Stats::CategoryIdMap* statsCategoryIdMap = mComponent->getConfigData()->getCategoryIdMap();

    CategoryMap::const_iterator catIter = mCategoryMap.begin();
    CategoryMap::const_iterator catEnd = mCategoryMap.end();
    for (; catIter != catEnd; ++catIter)
    {
        PeriodTypeMap* periodTypeMap = catIter->second;
        PeriodTypeMap::const_iterator periodTypeIter = periodTypeMap->begin();
        PeriodTypeMap::const_iterator periodTypeEnd = periodTypeMap->end();
        for (; periodTypeIter != periodTypeEnd; ++periodTypeIter)
        {
            const char8_t* tableName = nullptr;
            KeyScopeLeaderboards* keyScopeLeaderboards = periodTypeIter->second;
            KeyScopeLeaderboardMap::const_iterator keyScopeIter = keyScopeLeaderboards->getKeyScopeLeaderboardMap()->begin();
            KeyScopeLeaderboardMap::const_iterator keyScopeEnd = keyScopeLeaderboards->getKeyScopeLeaderboardMap()->end();
            for (; keyScopeIter != keyScopeEnd; ++keyScopeIter)
            {
                HistoricalLeaderboards* historicalLeaderboards = keyScopeIter->second;
                PeriodIdMap::const_iterator periodIdIter = historicalLeaderboards->getPeriodIdMap()->begin();
                PeriodIdMap::const_iterator periodIdEnd = historicalLeaderboards->getPeriodIdMap()->end();
                for (; periodIdIter != periodIdEnd; ++periodIdIter)
                {
                    LeaderboardMap* leaderboardMap = periodIdIter->second;
                    const StatLeaderboard* leaderboard = leaderboardMap->begin()->second->getLeaderboard();

                    tableName = leaderboard->getDBTableName();
                    TableInfo& table = mStatTables[tableName];
                    table.keyScopeColumns = leaderboard->getStatCategory()->getScopeNameSet();
                    table.periodType = leaderboard->getPeriodType();

                    Stats::CategoryIdMap::const_iterator cit = statsCategoryIdMap->find(catIter->first);
                    if (cit == statsCategoryIdMap->end())
                    {
                        // shouldn't happen
                        ERR_LOG("[LeaderboardIndexes].initStatTablesForLeaderboards() - unknown category id <"
                            << catIter->first << ">");
                        continue;
                    }
                    table.categoryName = cit->second->getName();

                    // Add all the leaderboard key stat column names
                    LeaderboardMap::const_iterator lbIter = leaderboardMap->begin();
                    LeaderboardMap::const_iterator lbEnd = leaderboardMap->end();
                    for (; lbIter != lbEnd; ++lbIter)
                    {
                        const RankingStatList& stats = lbIter->second->getLeaderboard()->getRankingStats();
                        int32_t statsSize = stats.size();
                        for( int32_t pos = 0; pos < statsSize; ++pos )
                        {
                            table.statColumns.insert(stats[pos]->getStatName());
                        }
                    }
                }
            }
            if (tableName != nullptr)
            {
                TableInfo& table = mStatTables[tableName];
                table.leaderboards.push_back(keyScopeLeaderboards);
            }
        }
    }
}

/*************************************************************************************************/
/*!
    \brief getLeaderboardTableName

    gets the table name for a leaderboard based on the corresponding stat table name

    \param[in] - statTableName - the name of the corresponding stat table.
    \param[out] - buffer - the buffer to store the table name.
    \param[in] - length - the length of the buffer
*/
/*************************************************************************************************/
void LeaderboardIndexes::getLeaderboardTableName(const char8_t* statTableName, char8_t* buffer, size_t length)
{
    if (buffer != nullptr)
    {
        blaze_snzprintf(buffer, length, "lb_%s", statTableName);
    }
}

void LeaderboardIndexes::getStatusInfo(ComponentStatus& status) const
{
    char8_t buf[64];
    
    ComponentStatus::InfoMap& map = status.getInfo();

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mGaugeBudgetOfAllRankingTables.get());
    map["GaugeBudgetOfAllRankingTables"] = buf;
    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mGaugeSizeOfAllRankingTables.get());
    map["GaugeSizeOfAllRankingTables"] = buf;
    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mGaugeLeaderboardCount.get());
    map["TotalLeaderboardCount"] = buf;
    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mGaugeLeaderboardRowCount.get());
    map["GaugeLeaderboardRowCount"] = buf;
}


} // Stats
} // Blaze
