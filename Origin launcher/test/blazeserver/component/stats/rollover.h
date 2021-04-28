/*************************************************************************************************/
/*!
    \file   rollover.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_STATS_ROLLOVER_H
#define BLAZE_STATS_ROLLOVER_H

#include "stats/tdf/stats_server.h"
#include "stats/statsslaveimpl.h"

#include "EASTL/hash_map.h"

namespace Blaze
{

namespace Stats
{

class StatsMasterImpl;

class Rollover
{

public:
    Rollover()
    : mComponent(nullptr),
    mDTimerId(0),
    mWTimerId(0),
    mMTimerId(0)
    {
    }
   
   ~Rollover();

    void setComponent(StatsMasterImpl* component) {mComponent = component;}
    
    bool parseRolloverData(const StatsConfig& config);
    int64_t getDailyTimer(int64_t t);
    int64_t getWeeklyTimer(int64_t t);
    int64_t getMonthlyTimer(int64_t t);
    void setTimers();
    PeriodIds* getPeriodIds() { return &mPeriodIds;}

    void cachePartitionName(const char8_t* tableName, int32_t periodId, const char8_t* partitionName);
    int32_t addExistingPeriodIdsToQuery(QueryPtr query, const char8_t* tableName, const PeriodIdList& existingPeriods, int32_t periodType);
    int32_t addNewPeriodIdsToQuery(QueryPtr query, const char8_t* tableName, const PeriodIdList& existingPeriods, int32_t periodType, int32_t currentPeriodId);

    static void gmTime(struct tm *tM, int64_t t);
    static int64_t mkgmTime(struct tm *tM);

private:
    typedef eastl::hash_map<int32_t, eastl::string> PartitionMap;
    typedef eastl::hash_map<eastl::string, PartitionMap> TablePartitionMap;

    void  dailyExpired();
    void  weeklyExpired();
    void  monthlyExpired();

    int32_t getBufferPeriodId(int32_t periodType, int32_t currentPeriodId) const;
    void getPeriodDescriptor(char8_t *buffer, size_t bufLen, int32_t periodType, int32_t periodId) const;

    void trimPeriods(int32_t period, int32_t currentPeriod, int32_t periodsToKeep); 
    void trimPeriodForTableDb(const DbIdList& dbIdList, const char8_t *tableName, int32_t periodTypeToTrim, int32_t currentPeriod, int32_t periodsToKeep);

    StatsMasterImpl* mComponent;
    PeriodIds mPeriodIds;

    static const size_t PARTITION_NAME_LENGTH = 32;

    TablePartitionMap mTablePartitionMap;

    DebugTimer mDebugTimer;

    TimerId mDTimerId;
    TimerId mWTimerId;
    TimerId mMTimerId;
    
};

} // namespace Stats
} // namespace Blaze
#endif
