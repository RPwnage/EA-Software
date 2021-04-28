/*************************************************************************************************/
/*!
    \file   updatestatshelper.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_STATS_DELETESTATSHELPER_H
#define BLAZE_STATS_DELETESTATSHELPER_H

/*** Include files *******************************************************************************/
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"

#include "EASTL/functional.h"
#include "EASTL/sort.h"

#include "statsconfig.h"
#include "statsslaveimpl.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
namespace Stats
{
// Forward declarations

// Contains various pieces of state needed to resolve derived stat values
typedef struct DeleteResolveContainer
{
    StatValMap* statValMap;
} DeleteResolveContainer;

void resolveStatValueForDelete(const char8_t* nameSpace, const char8_t* name, Blaze::ExpressionValueType type,
                               const void* context, Blaze::Expression::ExpressionVariableVal& val);

class DeleteStatsHelper
{

public:
    DeleteStatsHelper(StatsSlaveImpl* statsComp);
    ~DeleteStatsHelper();

    BlazeRpcError execute(const StatDelete* statDelete, DbConnPtr& dbConn, QueryPtr& query);

    BlazeRpcError broadcastDeleteAndUpdate();

private:
    StatsSlaveImpl* mStatsComp;
    StatUpdateNotification mNotification;
    StatDeleteAndUpdateBroadcast* mBroadcast;
    StringStatValueList mDerivedStrings;

    StringBuilder mLbQueryString;

    UpdateExtendedDataRequest mUedRequest;

    void appendDeleteStatement(const StatDelete* statDelete, const PeriodTypeList& periodTypes, const StatCategory* cat, QueryPtr& query);

    const StatsConfigData* mStatsConfig;
    int32_t mPeriodIds[STAT_NUM_PERIODS];
};

} // Stats
} // Blaze
#endif // BLAZE_STATS_DELETESTATSHELPER_H
