/*************************************************************************************************/
/*!
    \file   updatestatshelper.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_STATS_UPDATESTATSHELPER_H
#define BLAZE_STATS_UPDATESTATSHELPER_H

/*** Include files *******************************************************************************/
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"

#include "EASTL/functional.h"
#include "EASTL/sort.h"

#include "statsconfig.h"
#include "stats/rpc/statsslave.h"
#include "transformstats.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

namespace Stats
{
// Forward declarations


// How Stats Work - Quick Summary for the confused:
// 
// We use the UpdateStatsRequest to get determine what stats to update. 
//   This either comes by parsing the stats config along with the game report, or by building it directly with UpdateStatsRequestBuilder. 
// The UpdateStatsHelper holds both the UpdateStatsRequest, which it uses to determine what stats to grab, along with the mCacheRowList (which holds the grabbed stats). 
// We call initializeStatUpdate() to get ready, and fetchStats() to actually grab the stats, and fill in the cache. 
//   Note: All of the stats in the categories used by the Update are grabbed, not just the stats that were selected for update by the UpdateStatsRequest. 
// Calling the setValue* functions causes the cached rows to have their values updated, and these updated values will be sent with commitStats. 
// 
// When commitStats is called, we send the updated cache, and then request that the update be performed.
// The cache is copied over by updateProviderRows, which fills in new values on both the mNewStatValMap, and the mModifiedList (on the UpdateRow).
// Eventually, the code will hit commitUpdates(), which leads to evaluateStats, which actually performs the update we requested. 
//  evaluateStats fills in the ModifiedStatValueMap, which gets used to generate the DB operation when addStatsUpdateRequest is called in updateProviderRow. 
//
// When committing stats (commitStats()), there are four operations:
//  Copy any locally modified (via setValue*) stats to the ModifiedStatValueMap
//  Apply core stat updates (if not already done by fetchStats() or calcDerivedStats()), from the UpdateStatsRequest, updating the ModifiedStatValueMap
//  Apply derived stat updates (if not already done by calcDerivedStats()), as set by the stats config, updating the ModifiedStatValueMap
//  Commit the ModifiedStatValueMap stats to the DB
// 
class UpdateStatsHelper
{

public:
    UpdateStatsHelper();
    ~UpdateStatsHelper();

    BlazeRpcError initializeStatUpdate(
        const UpdateStatsRequest &request, 
        bool strict = true, 
        bool processGlobalStats = false,
        uint64_t timeout = 0);
    BlazeRpcError fetchStats();                             // fetches the current DB stats, and applies the initial updates. Fills the cache lists.
    BlazeRpcError DEFINE_ASYNC_RET(calcDerivedStats());     // applies the derived stat calculations. Be careful when not to call this multiple times, if you have increment style updates. (A=A+B)
    BlazeRpcError DEFINE_ASYNC_RET(commitStats());
    void abortTransaction();

    // Get a given stat from either the original DB entry (fromProvider == true) or the current stats list (which has updates applied)
    int64_t getValueInt(const UpdateRowKey* key, const char8_t* statName, 
        StatPeriodType periodType, bool fromProvider);
    float_t getValueFloat(const UpdateRowKey* key, const char8_t* statName,
        StatPeriodType periodType, bool fromProvider);
    const char8_t* getValueString(const UpdateRowKey* key, const char8_t* statName,
        StatPeriodType periodType, bool fromProvider);

    // Set the value of a given stat. Stored in the local cache, and sent up with commitStats, or calcDerivedStats.
    void setValueInt(const UpdateRowKey* key, const char8_t* statName, 
        StatPeriodType periodType, int64_t newValue);
    void setValueFloat(const UpdateRowKey* key, const char8_t* statName,
        StatPeriodType periodType, float_t newValue);
    void setValueString(const UpdateRowKey* key, const char8_t* statName,
        StatPeriodType periodType, const char8_t* newValue);

    // This UpdateStatsRequest is only used by initializeStatUpdate, when a local copy is made on the stats server. 
    // It has no affect on subsequent calls to fetchStats() or other functions. 
    UpdateStatsRequest& getUpdateStatsRequest() { return mRequest; }

private:
    void reset();
    FetchedRowUpdate* findCacheRow(const UpdateRowKey* key, StatPeriodType periodType, FetchedRowUpdateList& cacheRowList);
private:
    UpdateStatsRequest mRequest;
    uint32_t mUpdateState;
    StatsSlave* mStatsComp;
    FetchedRowUpdateList mCacheRowList;             // Local Copy: Direct from fetchStats(), holds all updates (stat updates and derived stats) - Used to override existing DB values
    FetchedRowUpdateList mOriginalCacheRowList;     // Stats directly from DB (Before INC, DEC, or derived stats updates)

    bool mDirty;
    uint64_t mTransactionContextId;
    InstanceId mRemoteSlaveId;
};


} // Stats
} // Blaze
#endif // BLAZE_STATS_UPDATESTATSHELPER_H
