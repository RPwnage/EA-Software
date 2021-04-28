/*************************************************************************************************/
/*!
    \file   rollover.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
/*************************************************************************************************/
/*!
    \class Rollover
    Controlls historical periods rollover. 
    Daily period ID is a day number since counting for time() function started. 
    Weekly period ID is a week number since counting for time() function started. 
    Monthly period ID is a month number since beginning of last century. 
    Period IDs are incremented at the times defined in the config file. Only numbers of periods 
    defined in the config file are kept in the DB. Obsolete periods are deleted.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/selector.h"
#include "framework/database/dbconn.h"
#include "framework/database/dbconnpool.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/query.h"

#include "statscomponentinterface.h"
#include "leaderboardindex.h"
#include "time.h"
#include "statsmasterimpl.h"
#include "statsconfig.h"
#include "statscommontypes.h"
#include "rollover.h"

namespace Blaze
{
namespace Stats
{

    Rollover::~Rollover()
    {
        if (mDTimerId != 0) gSelector->cancelTimer(mDTimerId);
        if (mWTimerId != 0) gSelector->cancelTimer(mWTimerId);
        if (mMTimerId != 0) gSelector->cancelTimer(mMTimerId);
    }

/*************************************************************************************************/
/*!
    \brief parseRolloverData
    Parses config file data to define rollover parameters.
    \param[in] - config StatsConfig containing data
    \return - true on success
*/
/*************************************************************************************************/
bool Rollover::parseRolloverData(const StatsConfig& config)
{
    TRACE_LOG("[Rollover].parseRolloverData: parsing rollover data");
    const RolloverData& rolloverData = config.getRollover();

    mPeriodIds.setDailyHour(rolloverData.getDailyHour());
    mPeriodIds.setDailyRetention(rolloverData.getDailyRetention());
    mPeriodIds.setDailyBuffer(rolloverData.getDailyBuffer());
    mPeriodIds.setWeeklyHour(rolloverData.getWeeklyHour());
    mPeriodIds.setWeeklyDay(rolloverData.getWeeklyDay());
    mPeriodIds.setWeeklyRetention(rolloverData.getWeeklyRetention());
    mPeriodIds.setWeeklyBuffer(rolloverData.getWeeklyBuffer());
    mPeriodIds.setMonthlyHour(rolloverData.getMonthlyHour());
    mPeriodIds.setMonthlyDay(rolloverData.getMonthlyDay());
    mPeriodIds.setMonthlyRetention(rolloverData.getMonthlyRetention());
    mPeriodIds.setMonthlyBuffer(rolloverData.getMonthlyBuffer());

    mDebugTimer.useTimer = rolloverData.getDebugUseTimers();
    mDebugTimer.dailyTimer = rolloverData.getDebugDailyTimer();
    mDebugTimer.weeklyTimer = rolloverData.getDebugWeeklyTimer();
    mDebugTimer.monthlyTimer = rolloverData.getDebugMonthlyTimer();

    if (mDebugTimer.useTimer)
    {
        mPeriodIds.setCurrentDailyPeriodId(1);
        mPeriodIds.setCurrentWeeklyPeriodId(1);
        mPeriodIds.setCurrentMonthlyPeriodId(1);
    }

    return true;
}

/*************************************************************************************************/
/*!
    \brief getDailyTimer
    Returns number of microseconds to the  next rollover. 
    Sets current daily period ID.
    \param[in] - t - current epoch time. 
    \return - microseconds to the rollover
*/
/*************************************************************************************************/
int64_t Rollover::getDailyTimer(int64_t t)
{
    if (mDebugTimer.useTimer)
    {
        // calculate the next rollover period
        int64_t difference = t % mDebugTimer.dailyTimer;
        int64_t tRollOver = t - difference + mDebugTimer.dailyTimer;

        mPeriodIds.setCurrentDailyPeriodId(static_cast<int32_t>(t/mDebugTimer.dailyTimer));
        return (tRollOver - t) * 1000000;
    }

    struct tm tM;

    gmTime(&tM, t);
    tM.tm_hour = mPeriodIds.getDailyHour();
    tM.tm_min = 0;
    tM.tm_sec = 0;

    int64_t tRollOver = mkgmTime(&tM);
    if (tRollOver <= t) 
    {
        tRollOver += 24*3600; // today's rollover has passed
    }

    gmTime(&tM, tRollOver);

    int32_t periodId = static_cast<int32_t>(tRollOver/(3600 * 24));
    mPeriodIds.setCurrentDailyPeriodId(periodId);

    eastl::string timeBuf;
    timeBuf.sprintf("%2d:%02d:%02d", tM.tm_hour, tM.tm_min, tM.tm_sec);
    INFO_LOG("[Rollover].getDailyTimer: periodID: " << periodId << "  next rollover: " << (tM.tm_year + 1900) 
             << "/" << (tM.tm_mon + 1) << "/" << tM.tm_mday << " (" << tM.tm_wday << ") " << timeBuf.c_str() << " GMT");

    return (tRollOver - t) * 1000000;
}

/*************************************************************************************************/
/*!
    \brief getWeeklyTimer
    Returns number of microseconds until next weekly rollover. 
    Sets current weekly period ID.
    \param[in] - t - epoch time_t
    \return - microseconds to the next weekly rollover
*/
/*************************************************************************************************/
int64_t Rollover::getWeeklyTimer(int64_t t)
{
    if (mDebugTimer.useTimer)
    {
        // calculate the next rollover period
        int64_t difference = t % mDebugTimer.weeklyTimer;
        int64_t tRollOver = t - difference + mDebugTimer.weeklyTimer;

        mPeriodIds.setCurrentWeeklyPeriodId(static_cast<int32_t>(t/mDebugTimer.weeklyTimer));
        return (tRollOver - t) * 1000000;
    }

    struct tm tM;

    gmTime(&tM, t);
    tM.tm_mday += mPeriodIds.getWeeklyDay() - tM.tm_wday ;
    tM.tm_hour = mPeriodIds.getWeeklyHour();
    tM.tm_min = 0;
    tM.tm_sec = 0;

    int64_t tRollOver = mkgmTime(&tM);
    if (tRollOver <= t) 
    {
        tRollOver += 24 * 3600 * 7; // this week's rollover has passed
    }

    gmTime(&tM, tRollOver);

    int32_t periodId = static_cast<int32_t>(tRollOver/(3600 * 24 * 7));
    mPeriodIds.setCurrentWeeklyPeriodId(periodId);

    eastl::string timeBuf;
    timeBuf.sprintf("%2d:%02d:%02d", tM.tm_hour, tM.tm_min, tM.tm_sec);
    INFO_LOG("[Rollover].getWeeklyTimer: periodID: " << periodId << "  next rollover: " << (tM.tm_year + 1900) << "/" 
             << (tM.tm_mon + 1) << "/" << tM.tm_mday << " (" << tM.tm_wday << ") " << timeBuf.c_str() << " GMT");
    
    return (tRollOver - t) * 1000000;
}

/*************************************************************************************************/
/*!
    \brief getMonthlyTimer
    Returns number of microseconds until next monthly rollover. 
    Sets current monthly period ID.
    \param[in] - t - epoch time. 
    \return - microseconds to the next monthly rollover
*/
/*************************************************************************************************/
int64_t Rollover::getMonthlyTimer(int64_t t)
{
    if (mDebugTimer.useTimer)
    {
        // calculate the next rollover period
        int64_t difference = t % mDebugTimer.monthlyTimer;
        int64_t tRollOver = t - difference + mDebugTimer.monthlyTimer;

        mPeriodIds.setCurrentMonthlyPeriodId(static_cast<int32_t>(t/mDebugTimer.monthlyTimer));
        return (tRollOver - t) * 1000000;
    }

    struct tm tM;

    gmTime(&tM, t);
    tM.tm_hour = mPeriodIds.getMonthlyHour();
    tM.tm_mday = mPeriodIds.getMonthlyDay();
    tM.tm_min = 0;
    tM.tm_sec = 0;

    int64_t tRollOver = mkgmTime(&tM);
    if (tRollOver <= t) 
    {
        ++tM.tm_mon; // this month's rollover has passed
        tRollOver = mkgmTime(&tM); 
    }

    gmTime(&tM, tRollOver);

    int32_t periodId = (tM.tm_year - 70 ) * 12 + tM.tm_mon; //All the previous years since epoch start (1970) + this one
    mPeriodIds.setCurrentMonthlyPeriodId(periodId);

    eastl::string timeBuf;
    timeBuf.sprintf("%2d:%02d:%02d", tM.tm_hour, tM.tm_min, tM.tm_sec);
    INFO_LOG("[Rollover].getMonthlyTimer: periodID: " << periodId << "  next rollover: " << (tM.tm_year + 1900) << "/"
             << (tM.tm_mon + 1) << "/" << tM.tm_mday << " (" << tM.tm_wday << ") " << timeBuf.c_str() << " GMT");
    
    return (tRollOver - t) * 1000000;
}

/*************************************************************************************************/
/*!
    \brief setTimers
    Initializes timers at server start. Sets current period IDs.
    \param[in] - none 
    \return - none
*/
/*************************************************************************************************/
void Rollover::setTimers()
{
    TimeValue tv = TimeValue::getTimeOfDay();
    int64_t t = tv.getSec();
    int64_t timer ;
    
    timer = getDailyTimer(t);
    mDTimerId = gSelector->scheduleFiberTimerCall(
            TimeValue::getTimeOfDay() + timer, this, &Rollover::dailyExpired,
            "Rollover::dailyExpired");
     
    timer = getWeeklyTimer(t);
    mWTimerId = gSelector->scheduleFiberTimerCall(
        TimeValue::getTimeOfDay() + timer, this, &Rollover::weeklyExpired,
        "Rollover::weeklyExpired");

    timer = getMonthlyTimer(t);
    mMTimerId = gSelector->scheduleFiberTimerCall(
        TimeValue::getTimeOfDay() + timer, this, &Rollover::monthlyExpired,
        "Rollover::monthlyExpired");

    return;
}

/*************************************************************************************************/
/*!
    \brief dailyExpired
    Daily rollover timer callback. If daily period ID has changed it initiates trimming 
    of daily historical periods older than number periods defined in the config file.
    \param[in]  - TimerId
    \return - none
*/
/*************************************************************************************************/
void Rollover::dailyExpired()
{
    TimeValue tv = TimeValue::getTimeOfDay();
    int64_t t = tv.getSec();

    int64_t timer = getDailyTimer(t);
    
    mDTimerId = gSelector->scheduleFiberTimerCall(
            TimeValue::getTimeOfDay() + timer, this, &Rollover::dailyExpired,
            "Rollover::dailyExpired");
    
    mComponent->sendSetPeriodIdsNotificationToSlaves(&mPeriodIds);
    trimPeriods(STAT_PERIOD_DAILY, mPeriodIds.getCurrentDailyPeriodId(), mPeriodIds.getDailyRetention());
    
    return;
}

/*************************************************************************************************/
/*!
    \brief weeklyExpired
    Weekly rollover timer callback. If weekly period ID has changed it initiates trimming 
    of weekly historical periods older than number periods defined in the config file.
    \param[in]  - TimerId
    \return - none
*/
/*************************************************************************************************/
void  Rollover::weeklyExpired()
{
    TimeValue tv = TimeValue::getTimeOfDay();
    int64_t t = tv.getSec();

    int64_t timer = getWeeklyTimer(t);
    
    mWTimerId = gSelector->scheduleFiberTimerCall(
            TimeValue::getTimeOfDay() + timer, this, &Rollover::weeklyExpired,
            "Rollover::weeklyExpired");

    mComponent->sendSetPeriodIdsNotificationToSlaves(&mPeriodIds);

    trimPeriods(STAT_PERIOD_WEEKLY, mPeriodIds.getCurrentWeeklyPeriodId(), mPeriodIds.getWeeklyRetention());

}

/*************************************************************************************************/
/*!
    \brief weeklyExpired
    Monthly rollover timer callback. If monthly period ID has changed it initiates trimming 
    of monthly historical periods older than number periods defined in the config file.
    \param[in]  - TimerId
    \return - none
*/
/*************************************************************************************************/
void  Rollover::monthlyExpired()
{
    TimeValue tv = TimeValue::getTimeOfDay();
    int64_t t = tv.getSec();

    int64_t timer = getMonthlyTimer(t);
    
    mMTimerId = gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay() + timer, this, &Rollover::monthlyExpired,
        "Rollover::monthlyExpired");

    mComponent->sendSetPeriodIdsNotificationToSlaves(&mPeriodIds);
    
    trimPeriods(STAT_PERIOD_MONTHLY, mPeriodIds.getCurrentMonthlyPeriodId(), mPeriodIds.getMonthlyRetention());    
}

/*************************************************************************************************/
/*!
    \brief getPeriodDescriptor

    Create a partition name based on period (type and ID).

    \param[out]  buffer - output buffer
    \param[in]  bufLen - size of output buffer
    \param[in]  periodType - period type (STAT_PERIOD_MONTHLY, for example)
    \param[in]  periodId - period ID (from epoch start, based on period type)

    \return - none
*/
/*************************************************************************************************/
void Rollover::getPeriodDescriptor(char8_t *buffer, size_t bufLen, int32_t periodType, int32_t periodId) const
{
    int64_t t;
    struct tm tM;

    if (mDebugTimer.useTimer)
    {
        switch (periodType)
        {
        case STAT_PERIOD_MONTHLY:
            t = static_cast<int64_t>(periodId) * mDebugTimer.monthlyTimer;
            gmTime(&tM, t);
            blaze_snzprintf(buffer, bufLen, "month_of_%04d_%02d_%02d_%d", tM.tm_year + 1900, tM.tm_mon + 1, tM.tm_mday, periodId);
            break;
        case STAT_PERIOD_WEEKLY:
            t = static_cast<int64_t>(periodId) * mDebugTimer.weeklyTimer;
            gmTime(&tM, t);
            blaze_snzprintf(buffer, bufLen, "week_of_%04d_%02d_%02d_%d", tM.tm_year + 1900, tM.tm_mon + 1, tM.tm_mday, periodId);
            break;
        case STAT_PERIOD_DAILY:
            t = static_cast<int64_t>(periodId) * mDebugTimer.dailyTimer;
            gmTime(&tM, t);
            blaze_snzprintf(buffer, bufLen, "day_of_%04d_%02d_%02d_%d", tM.tm_year + 1900, tM.tm_mon + 1, tM.tm_mday, periodId);
            break;
        }
        return;
    }

    switch (periodType)
    {
    case STAT_PERIOD_MONTHLY:
        blaze_snzprintf(buffer, bufLen, "month_of_%04d_%02d", periodId / 12 + 1970, periodId % 12 + 1);
        break;
    case STAT_PERIOD_WEEKLY:
        t = static_cast<int64_t>(periodId) * 24 * 3600 * 7;
        gmTime(&tM, t);
        blaze_snzprintf(buffer, bufLen, "week_of_%04d_%02d_%02d", tM.tm_year + 1900, tM.tm_mon + 1, tM.tm_mday);
        break;
    case STAT_PERIOD_DAILY:
        t = static_cast<int64_t>(periodId) * 24 * 3600;
        gmTime(&tM, t);
        blaze_snzprintf(buffer, bufLen, "day_of_%04d_%02d_%02d", tM.tm_year + 1900, tM.tm_mon + 1, tM.tm_mday);
        break;
    }
}

void Rollover::cachePartitionName(const char8_t* tableName, int32_t periodId, const char8_t* partitionName)
{
    PartitionMap& partitionMap = mTablePartitionMap[tableName];
    partitionMap[periodId] = partitionName;
}

int32_t Rollover::getBufferPeriodId(int32_t periodType, int32_t currentPeriodId) const
{
    int32_t bufferPeriodId = currentPeriodId;
    switch (periodType)
    {
        case STAT_PERIOD_DAILY:
            bufferPeriodId += mPeriodIds.getDailyBuffer();
            break;
        case STAT_PERIOD_WEEKLY:
            bufferPeriodId += mPeriodIds.getWeeklyBuffer();
            break;
        case STAT_PERIOD_MONTHLY:
            bufferPeriodId += mPeriodIds.getMonthlyBuffer();
            break;
    }
    return bufferPeriodId;
}

/*************************************************************************************************/
/*!
    \brief addExistingPeriodIdsToQuery

    Add partition definitions to query statement based on list of period IDs.

    \param[out]  query - output query (statement should have been started already)
    \param[in]  tableName - name of the table to add partitions to
    \param[in]  existingPeriods - list of period IDs to add to query
    \param[in]  periodType - period type (STAT_PERIOD_MONTHLY, for example)

    \return - number of partition definitions added to query
*/
/*************************************************************************************************/
int32_t Rollover::addExistingPeriodIdsToQuery(QueryPtr query, const char8_t* tableName, const PeriodIdList& existingPeriods, int32_t periodType)
{
    int32_t numAdds = 0;

    PartitionMap& partitionMap = mTablePartitionMap[tableName];

    PeriodIdList::const_iterator idItr = existingPeriods.begin();
    PeriodIdList::const_iterator idEnd = existingPeriods.end();
    for (; idItr != idEnd; ++idItr)
    {
        int32_t periodId = *idItr;
        char8_t periodDescriptor[PARTITION_NAME_LENGTH]; // partition name
        getPeriodDescriptor(periodDescriptor, sizeof(periodDescriptor), periodType, periodId);

        // This method should only be called on server startup, thus we can safely cache this partition
        // information even without knowing whether the query to add the partitions succeeds, because
        // the server will simply fail to startup should the query fail, rendering this cache moot
        partitionMap[periodId] = periodDescriptor;

        // note that it's up to the caller to trim the last comma if not needed
        query->append("\n    PARTITION `$s` VALUES IN ($d),", periodDescriptor, periodId);
        ++numAdds;
    }

    return numAdds;
}

/*************************************************************************************************/
/*!
    \brief addNewPeriodIdsToQuery

    Add partition definitions to query statement based on current period ID and excluding list of period IDs.

    \param[out]  query - output query (statement should have been started already)
    \param[in]  tableName - name of table to add partitions to
    \param[in]  existingPeriods - list of period IDs to check for exclusion
    \param[in]  periodType - period type (STAT_PERIOD_MONTHLY, for example)
    \param[in]  currentPeriodId - current period ID

    \return - number of partition definitions added to query
*/
/*************************************************************************************************/
int32_t Rollover::addNewPeriodIdsToQuery(QueryPtr query, const char8_t* tableName, const PeriodIdList& existingPeriods, int32_t periodType, int32_t currentPeriodId)
{
    int32_t numAdds = 0;

    int32_t bufferPeriodId = getBufferPeriodId(periodType, currentPeriodId);

    PartitionMap& partitionMap = mTablePartitionMap[tableName];

    for (int32_t periodId = currentPeriodId; periodId <= bufferPeriodId; ++periodId)
    {
        // look in the existing period id list
        PeriodIdList::const_iterator idItr = existingPeriods.begin();
        PeriodIdList::const_iterator idEnd = existingPeriods.end();
        for (; idItr != idEnd; ++idItr)
        {
            if (*idItr == periodId)
            {
                break;
            }
        }
        if (idItr != idEnd)
        {
            // already exists
            continue;
        }

        char8_t periodDescriptor[PARTITION_NAME_LENGTH]; // partition name
        getPeriodDescriptor(periodDescriptor, sizeof(periodDescriptor), periodType, periodId);

        // This method should only be called on server startup, thus we can safely cache this partition
        // information even without knowing whether the query to add the partitions succeeds, because
        // the server will simply fail to startup should the query fail, rendering this cache moot
        partitionMap[periodId] = periodDescriptor;

        // note that it's up to the caller to trim the last comma if not needed
        query->append("\n    PARTITION `$s` VALUES IN ($d),", periodDescriptor, periodId);
        ++numAdds;
    }

    return numAdds;
}

/*************************************************************************************************/
/*!
    \brief trimPeriods

    Instructs slave to delete outdated historical periods for the given period type.

    \param[in]  periodType - period type
    \param[in]  retention - number of periods to keep

    \return - none
*/
/*************************************************************************************************/
void Rollover::trimPeriods(int32_t periodTypeToTrim, int32_t currentPeriodId, int32_t periodsToKeep)
{
    StatPeriod statPeriod;
    statPeriod.setPeriod(periodTypeToTrim);
    statPeriod.setRetention(periodsToKeep);

    mComponent->trimPeriods(statPeriod);
    mComponent->sendTrimPeriodNotificationToSlaves(&statPeriod);

    const CategoryMap* categoryMap = mComponent->getConfigData()->getCategoryMap();
    CategoryMap::const_iterator itCat = categoryMap->begin();
    CategoryMap::const_iterator itCatEnd = categoryMap->end();

    for (; itCat != itCatEnd; ++itCat)
    {
        const StatCategory* statCategory = itCat->second;
        if (statCategory->isValidPeriod(periodTypeToTrim))
        {
            const DbIdList& list = 
                mComponent->getConfigData()->getShardConfiguration().getDbIds(statCategory->getCategoryEntityType());
            trimPeriodForTableDb(list, statCategory->getTableName(periodTypeToTrim),
                    periodTypeToTrim, currentPeriodId, periodsToKeep);
        }
    }

    LeaderboardIndexes* lbIndexes = mComponent->mLeaderboardIndexes;
    if (lbIndexes != nullptr)
    {
        TableInfoMap& statTablesForLb = lbIndexes->getStatTablesForLeaderboards();

        // iterate the list of stats tables that we should have leaderboards for
        TableInfoMap::iterator tableItr = statTablesForLb.begin();
        TableInfoMap::iterator tableEnd = statTablesForLb.end();

        for (; tableItr != tableEnd; ++tableItr)
        {
            TableInfo& tableInfo = tableItr->second;

            if (tableInfo.periodType != periodTypeToTrim)
            {
                // not applicable
                continue;
            }

            CategoryMap::const_iterator categoryItr = categoryMap->find(tableInfo.categoryName);
            if (categoryItr == categoryMap->end())
            {
                ERR_LOG("[Rollover].trimPeriodsDb: Leaderboard is defined with unknown stat category (" << tableItr->first << ")");
                // try next table...
                continue;
            }
            
            char8_t lbTableName[LB_MAX_TABLE_NAME_LENGTH];
            LeaderboardIndexes::getLeaderboardTableName(tableItr->first, lbTableName, LB_MAX_TABLE_NAME_LENGTH);

            DbIdList list;
            list.push_back(mComponent->getDbId());
            trimPeriodForTableDb(list, lbTableName, periodTypeToTrim, currentPeriodId, periodsToKeep);
        }
    }
}

/*************************************************************************************************/
/*!
    \brief trimPeriodForTableDb

    Helper for trimPeriodDb.  Creates partitions for new periods and drops partitions for old periods.

    \param[in] dbIdList - ID(s) used to get DB connection(s)
                          (if sharding is used, there will be multiple IDs; one for each shard)
    \param[in] tableName - DB table
    \param[in] periodTypeToTrim - period type
    \param[in] currentPeriodId - period ID
    \param[in] periodsToKeep - number of periods to keep (retain)

    \return - none
*/
/*************************************************************************************************/
void Rollover::trimPeriodForTableDb(const DbIdList& dbIdList, const char8_t *tableName, int32_t periodTypeToTrim, int32_t currentPeriodId, int32_t periodsToKeep)
{
    DbError dbError;
    int32_t numDrops = 0;
    int32_t numAdds = 0;
    PartitionMap& partitionMap = mTablePartitionMap[tableName];
    int32_t periodId;

    /// @hack to mitigate GOSOPS-192817 where blaze loses connection to DB (Galera-specific?) on ALTER TABLE and prevents subsequent ALTER TABLE updates (via squelching of getConn); mitigation is pausing to allow blaze to recover its connection
    char8_t timeBuf[64];
    TimeValue pause = mComponent->getConfig().getRollover().getPauseBetweenUpdates();
    const EA::TDF::TimeValue MAX_PAUSE("5m", EA::TDF::TimeValue::TIME_FORMAT_INTERVAL);
    if (pause > MAX_PAUSE)
    {
        // limit the pause in case it's "accidentally" configured to a "large" value
        pause = MAX_PAUSE;
    }
    INFO_LOG("[Rollover].trimPeriodForTableDb: pause before each '" << tableName << "' table update is "
        << pause.toIntervalString(timeBuf, sizeof(timeBuf)) << (pause < mComponent->getConfig().getRollover().getPauseBetweenUpdates() ? " (capped)" : ""));

    // try adding new periods (partitions) to this table...
    int32_t bufferPeriodId = getBufferPeriodId(periodTypeToTrim, currentPeriodId);
    for (periodId = currentPeriodId; periodId <= bufferPeriodId; ++periodId)
    {
        if (partitionMap.find(periodId) == partitionMap.end())
        {
            char8_t partitionName[PARTITION_NAME_LENGTH];
            getPeriodDescriptor(partitionName, sizeof(partitionName), periodTypeToTrim, periodId);

            int32_t numFailures = 0;
            for (DbIdList::const_iterator dbIdItr = dbIdList.cbegin(); dbIdItr != dbIdList.cend(); ++dbIdItr)
            {
                Fiber::sleep(pause, "Rollover::trimPeriodForTableDb sleep before adding each new partition");
                DbConnPtr conn = gDbScheduler->getConnPtr(*dbIdItr);
                if (conn == nullptr)
                {
                    ERR_LOG("[Rollover].trimPeriodForTableDb: could not acquire db conn to add partition ("
                        << partitionName << ") for table (" << tableName << ") in database with id (" << *dbIdItr << ")");
                    ++numFailures;
                    continue;
                }
                QueryPtr query = DB_NEW_QUERY_PTR(conn);
                query->append("ALTER TABLE `$s` ADD PARTITION (PARTITION `$s` VALUES IN ($d));", tableName, partitionName, periodId);

                const char8_t* dbName = conn->getDbConnPool().getName();
                INFO_LOG("[Rollover].trimPeriodForTableDb: about to add partition ("
                    << partitionName << ") for table (" << tableName << ") in database (" << dbName << ")");

                dbError = conn->executeQuery(query);
                if (dbError == DB_ERR_SAME_NAME_PARTITION)
                {
                    WARN_LOG("[Rollover].trimPeriodForTableDb: partition ("
                        << partitionName << ") for table (" << tableName << ") already exists in database (" << dbName << ")");
                }
                else if (dbError != DB_ERR_OK)
                {
                    ERR_LOG("[Rollover].trimPeriodForTableDb: failed to add partition ("
                        << partitionName << ") for table (" << tableName << ") in database (" << dbName << "): " << getDbErrorString(dbError));
                    ++numFailures;
                    continue;
                }
                else
                {
                    ++numAdds;
                }
            }

            if (numFailures == 0)
                partitionMap[periodId] = partitionName;
        }
    }

    if (numAdds == 0)
    {
        WARN_LOG("[Rollover].trimPeriodForTableDb: no partitions added for table (" << tableName << ")");
    }

    // try dropping old periods (partitions) from this table...
    PartitionMap::iterator pItr = partitionMap.begin();
    while (pItr != partitionMap.end())
    {
        periodId = pItr->first;

        // keeping an extra period in case someone is actually fetching with oldest
        // configured/retained period while a rollover occurs
        if (periodId >= currentPeriodId - periodsToKeep - 1)
        {
            ++pItr;
            continue;
        }

        int32_t numFailures = 0;
        for (DbIdList::const_iterator dbIdItr = dbIdList.cbegin(); dbIdItr != dbIdList.cend(); ++dbIdItr)
        {
            Fiber::sleep(pause, "Rollover::trimPeriodForTableDb sleep before dropping each old partition");
            DbConnPtr conn = gDbScheduler->getConnPtr(*dbIdItr);

            if (conn == nullptr)
            {
                ERR_LOG("[Rollover].trimPeriodForTableDb: could not acquire db conn to drop partition ("
                    << pItr->second.c_str() << ") for table (" << tableName << ") in database with id (" << *dbIdItr << ")");
                ++numFailures;
                continue;
            }
            QueryPtr query = DB_NEW_QUERY_PTR(conn);
            query->append("ALTER TABLE `$s` DROP PARTITION `$s`;", tableName, pItr->second.c_str());

            const char8_t* dbName = conn->getDbConnPool().getName();
            INFO_LOG("[Rollover].trimPeriodForTableDb: about to drop partition ("
                << pItr->second.c_str() << ") for table (" << tableName << ") in database (" << dbName << ")");

            // Try to drop the partition, if we fail we will not remove the entry from our list, and hope that we
            // can drop it next time around
            dbError = conn->executeQuery(query);
            if (dbError == DB_ERR_DROP_PARTITION_NON_EXISTENT)
            {
                WARN_LOG("[Rollover].trimPeriodForTableDb: partition ("
                    << pItr->second.c_str() << ") for table (" << tableName << ") did not exist in database (" << dbName << ")");
            }
            else if (dbError != DB_ERR_OK)
            {
                ERR_LOG("[Rollover].trimPeriodForTableDb: failed to drop partition ("
                    << pItr->second.c_str() << ") for table (" << tableName << ") in database (" << dbName << "): " << getDbErrorString(dbError));
                ++numFailures;
                continue;
            }
            else
            {
                ++numDrops;
            }
        }

        if(numFailures == 0)
            pItr = partitionMap.erase(pItr);
        else
            ++pItr;
    }

    if (numDrops == 0)
    {
        WARN_LOG("[Rollover].trimPeriodForTableDb: no partitions dropped for table (" << tableName << ")");
    }
}

/*************************************************************************************************/
/*!
    \brief gmTime  static 
    Converts epoch time into GMT components
    \param[in]  - t - 64 bit epoch time
    \param[out]  - tM - pointer to tm structure
    \return - none
*/
/*************************************************************************************************/
void Rollover::gmTime(struct tm *tM, int64_t t)
{
    #ifdef EA_PLATFORM_WINDOWS
    _gmtime64_s(tM, &t);
    #else
    gmtime_r(&t, tM);
    #endif
} 

/*************************************************************************************************/
/*!
    \brief mkgmTime  static 
    Converts GMT components into epoch time
    \param[in]  - tM - pointer to tm structure
    \return - 64 bit epoch time
*/
/*************************************************************************************************/
int64_t Rollover::mkgmTime(struct tm *tM)
{
    tM->tm_wday = 0;
    tM->tm_yday = 0;
    tM->tm_isdst = -1;

    int64_t t = mktime(tM);
    
    #ifdef EA_PLATFORM_WINDOWS
    t -= _timezone;
    #else
    t -= timezone;
    #endif


    if (tM->tm_isdst > 0) t += 3600; // daylight saving time is in effect

    return t;
}

} // namespace Stats
} // namespace Blaze
