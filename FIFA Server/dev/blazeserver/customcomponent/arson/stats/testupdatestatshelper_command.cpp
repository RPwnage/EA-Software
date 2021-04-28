/*************************************************************************************************/
/*!
    \file   testupdatestatshelper_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/testupdatestatshelper_stub.h"
#include "stats/updatestatsrequestbuilder.h"
#include "stats/updatestatshelper.h"

namespace Blaze
{
namespace Arson
{
class TestUpdateStatsHelperCommand : public TestUpdateStatsHelperCommandStub
{
public:
    TestUpdateStatsHelperCommand(
        Message* message, Blaze::Arson::TestUpdateStatsHelperRequest* request, ArsonSlaveImpl* componentImpl)
        : TestUpdateStatsHelperCommandStub(message, request),
        mComponent(componentImpl)
    {
        allTestsPassed = true;
    }

    ~TestUpdateStatsHelperCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;
    bool allTestsPassed;

    void normal_case()
    {
        Stats::UpdateStatsRequestBuilder builder;
        builder.startStatRow("Core", mRequest.getPlayerId());
        builder.incrementStat("kills", 2);
        builder.incrementStat("deaths", 1);
        builder.completeStatRow();

        Stats::UpdateStatsHelper updateStatsHelper;
        BlazeRpcError error = ERR_OK;

        error = updateStatsHelper.initializeStatUpdate(builder);
        TRACE_LOG("[TestUpdateStatsHelperCommand].normal_case: initializeStatUpdate error code: " << ErrorHelp::getErrorName(error));
        if (error != ERR_OK)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].normal_case: initializeStatUpdate expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
            allTestsPassed = false;
            return;
        }

        error = updateStatsHelper.commitStats();
        TRACE_LOG("[TestUpdateStatsHelperCommand].normal_case: commitStats error code: " << ErrorHelp::getErrorName(error));
        if (error != ERR_OK)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].normal_case: commitStats expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
            allTestsPassed = false;
        }
    }

    void no_init_commit()
    {
        Stats::UpdateStatsHelper updateStatsHelper;
        BlazeRpcError error = ERR_OK;

        // 1. commitStats
        error = updateStatsHelper.commitStats();
        TRACE_LOG("[TestUpdateStatsHelperCommand].no_init_commit: commitStats error code: " << ErrorHelp::getErrorName(error));
        if (error != ERR_SYSTEM)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].no_init_commit: commitStats expecting error code: ERR_SYSTEM, but got: " << ErrorHelp::getErrorName(error));
            allTestsPassed = false;
        }
    }

    void timeout_commit()
    {
        Stats::UpdateStatsRequestBuilder builder;
        builder.startStatRow("Core", mRequest.getPlayerId());
        builder.incrementStat("kills", 2);
        builder.incrementStat("deaths", 1);
        builder.completeStatRow();

        Stats::UpdateStatsHelper updateStatsHelper;
        BlazeRpcError error = ERR_OK;

        // 1. initializeStatUpdate with timeout = 2 seconds
        error = updateStatsHelper.initializeStatUpdate(builder, true, false, 2);
        TRACE_LOG("[TestUpdateStatsHelperCommand].timeout_commit: initializeStatUpdate error code: " << ErrorHelp::getErrorName(error));
        if (error != ERR_OK)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].timeout_commit: initializeStatUpdate expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
            allTestsPassed = false;
            return;
        }

        // 2. Wait for 3 seconds
        error = gSelector->sleep(3*1000*1000);
        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].timeout_commit: sleep expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
            allTestsPassed = false;
            return;
        }

        // 3. commitStats
        error = updateStatsHelper.commitStats();
        TRACE_LOG("[TestUpdateStatsHelperCommand].timeout_commit: commitStats error code: " << ErrorHelp::getErrorName(error));
        if (error != ERR_SYSTEM)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].timeout_commit: commitStats expecting error code: ERR_SYSTEM, but got: " << ErrorHelp::getErrorName(error));
            allTestsPassed = false;
        }
    }

    void abort_commit()
    {
        Stats::UpdateStatsRequestBuilder builder;
        builder.startStatRow("Core", mRequest.getPlayerId());
        builder.incrementStat("kills", 2);
        builder.incrementStat("deaths", 1);
        builder.completeStatRow();

        Stats::UpdateStatsHelper updateStatsHelper;
        BlazeRpcError error = ERR_OK;

        // 1. initializeStatUpdate
        error = updateStatsHelper.initializeStatUpdate(builder);
        TRACE_LOG("[TestUpdateStatsHelperCommand].abort_commit: initializeStatUpdate error code: " << ErrorHelp::getErrorName(error));
        if (error != ERR_OK)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].abort_commit: initializeStatUpdate expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
            allTestsPassed = false;
            return;
        }

        // 2. abortTransaction
        updateStatsHelper.abortTransaction();

        // 3. commitStats
        error = updateStatsHelper.commitStats();
        TRACE_LOG("[TestUpdateStatsHelperCommand].abort_commit: commitStats error code: " << ErrorHelp::getErrorName(error));
        if (error != ERR_SYSTEM)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].abort_commit: commitStats expecting error code: ERR_SYSTEM, but got: " << ErrorHelp::getErrorName(error));
            allTestsPassed = false;
        }
    }

    void no_fetch_getValue()
    {
        Stats::UpdateStatsRequestBuilder builder;
        builder.startStatRow("Core", mRequest.getPlayerId());
        builder.completeStatRow();

        Stats::UpdateStatsHelper updateStatsHelper;
        BlazeRpcError error = ERR_OK;

        // 1. initializeStatUpdate
        error = updateStatsHelper.initializeStatUpdate(builder);
        TRACE_LOG("[TestUpdateStatsHelperCommand].no_fetch_getValue: initializeStatUpdate error code: " << ErrorHelp::getErrorName(error));
        if (error != ERR_OK)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].no_fetch_getValue: initializeStatUpdate expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
            allTestsPassed = false;
            return;
        }

        // 2. getUpdateRowKey
        Stats::UpdateRowKey* key = builder.getUpdateRowKey("Core", mRequest.getPlayerId());
        if (key == nullptr)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].no_fetch_getValue: getUpdateRowKey failed because key is nullptr");
            allTestsPassed = false;
            return;
        }

        // 3. getValueInt
        // Expected Behavior: ASSERTION FAILED!
        int32_t kills = (int32_t)updateStatsHelper.getValueInt(key, "kills", Stats::STAT_PERIOD_ALL_TIME, true);
        TRACE_LOG("[TestUpdateStatsHelperCommand].no_fetch_getValue: getValueInt: " << kills);   
    }

    void no_fetch_setValue()
    {
        Stats::UpdateStatsRequestBuilder builder;
        builder.startStatRow("Core", mRequest.getPlayerId());
        builder.completeStatRow();

        Stats::UpdateStatsHelper updateStatsHelper;
        BlazeRpcError error = ERR_OK;

        // 1. initializeStatUpdate
        error = updateStatsHelper.initializeStatUpdate(builder);
        TRACE_LOG("[TestUpdateStatsHelperCommand].no_fetch_setValue: initializeStatUpdate error code: " << ErrorHelp::getErrorName(error));
        if (error != ERR_OK)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].no_fetch_setValue: initializeStatUpdate expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
            allTestsPassed = false;
            return;
        }

        // 2. getUpdateRowKey
        Stats::UpdateRowKey* key = builder.getUpdateRowKey("Core", mRequest.getPlayerId());
        if (key == nullptr)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].no_fetch_setValue: getUpdateRowKey failed because key is nullptr");
            allTestsPassed = false;
            return;
        }

        // 3. setValueInt
        // Expected Behavior: ASSERTION FAILED!
        int32_t newKills = 10;
        updateStatsHelper.setValueInt(key, "kills", Stats::STAT_PERIOD_ALL_TIME, newKills);
    }

    void fetch_commit_getValue()
    {
        Stats::UpdateStatsRequestBuilder builder;
        builder.startStatRow("Core", mRequest.getPlayerId());
        builder.incrementStat("kills", 2);
        builder.incrementStat("deaths", 1);
        builder.completeStatRow();

        Stats::UpdateStatsHelper updateStatsHelper;
        BlazeRpcError error = ERR_OK;

        // 1. initializeStatUpdate
        error = updateStatsHelper.initializeStatUpdate(builder);
        TRACE_LOG("[TestUpdateStatsHelperCommand].fetch_commit_getValue: initializeStatUpdate error code: " << ErrorHelp::getErrorName(error));
        if (error != ERR_OK)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].fetch_commit_getValue: initializeStatUpdate expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
            allTestsPassed = false;
            return;
        }

        // 2. fetchStats
        error = updateStatsHelper.fetchStats();
        TRACE_LOG("[TestUpdateStatsHelperCommand].fetch_commit_getValue: fetchStats error code: " << ErrorHelp::getErrorName(error));
        if (error != ERR_OK)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].fetch_commit_getValue: fetchStats expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
            allTestsPassed = false;
            return;
        }

        // 3. commitStats
        error = updateStatsHelper.commitStats();
        TRACE_LOG("[TestUpdateStatsHelperCommand].fetch_commit_getValue: commitStats error code: " << ErrorHelp::getErrorName(error));
        if (error != ERR_OK)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].fetch_commit_getValue: commitStats expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
            allTestsPassed = false;
            return;
        }

        // 4. getUpdateRowKey
        Stats::UpdateRowKey* key = builder.getUpdateRowKey("Core", mRequest.getPlayerId());
        if (key == nullptr)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].fetch_commit_getValue: getUpdateRowKey failed because key is nullptr");
            allTestsPassed = false;
            return;
        }

        // 5. getValueInt
        // Expected Behavior: ASSERTION FAILED!
        int32_t kills = (int32_t)updateStatsHelper.getValueInt(key, "kills", Stats::STAT_PERIOD_ALL_TIME, true);
        TRACE_LOG("[TestUpdateStatsHelperCommand].fetch_commit_getValue: getValueInt: " << kills);                
    }

    void fetch_commit_setValue()
    {
        Stats::UpdateStatsRequestBuilder builder;
        builder.startStatRow("Core", mRequest.getPlayerId());
        builder.incrementStat("kills", 2);
        builder.incrementStat("deaths", 1);
        builder.completeStatRow();

        Stats::UpdateStatsHelper updateStatsHelper;
        BlazeRpcError error = ERR_OK;

        // 1. initializeStatUpdate
        error = updateStatsHelper.initializeStatUpdate(builder);
        TRACE_LOG("[TestUpdateStatsHelperCommand].fetch_commit_setValue: initializeStatUpdate error code: " << ErrorHelp::getErrorName(error));
        if (error != ERR_OK)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].fetch_commit_setValue: initializeStatUpdate expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
            allTestsPassed = false;
            return;
        }

        // 2. fetchStats
        error = updateStatsHelper.fetchStats();
        TRACE_LOG("[TestUpdateStatsHelperCommand].fetch_commit_setValue: fetchStats error code: " << ErrorHelp::getErrorName(error));
        if (error != ERR_OK)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].fetch_commit_setValue: fetchStats expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
            allTestsPassed = false;
            return;
        }

        // 3. commitStats
        error = updateStatsHelper.commitStats();
        TRACE_LOG("[TestUpdateStatsHelperCommand].fetch_commit_setValue: commitStats error code: " << ErrorHelp::getErrorName(error));
        if (error != ERR_OK)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].fetch_commit_setValue: commitStats expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
            allTestsPassed = false;
            return;
        }

        // 4. getUpdateRowKey
        Stats::UpdateRowKey* key = builder.getUpdateRowKey("Core", mRequest.getPlayerId());
        if (key == nullptr)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].fetch_commit_setValue: getUpdateRowKey failed because key is nullptr");
            allTestsPassed = false;
            return;
        }

        // 5. setValueInt
        // Expected Behavior: ASSERTION FAILED!
        int32_t newKills = 10;
        updateStatsHelper.setValueInt(key, "kills", Stats::STAT_PERIOD_ALL_TIME, newKills);
    }

    void fetch_commit_calcDerivedStats()
    {
        Stats::UpdateStatsRequestBuilder builder;
        builder.startStatRow("Core", mRequest.getPlayerId());
        builder.incrementStat("kills", 2);
        builder.incrementStat("deaths", 1);
        builder.completeStatRow();

        Stats::UpdateStatsHelper updateStatsHelper;
        BlazeRpcError error = ERR_OK;

        // 1. initializeStatUpdate
        error = updateStatsHelper.initializeStatUpdate(builder);
        TRACE_LOG("[TestUpdateStatsHelperCommand].fetch_commit_calcDerivedStats: initializeStatUpdate error code: " << ErrorHelp::getErrorName(error));
        if (error != ERR_OK)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].fetch_commit_calcDerivedStats: initializeStatUpdate expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
            allTestsPassed = false;
            return;
        }

        // 2. fetchStats
        error = updateStatsHelper.fetchStats();
        TRACE_LOG("[TestUpdateStatsHelperCommand].fetch_commit_calcDerivedStats: fetchStats error code: " << ErrorHelp::getErrorName(error));
        if (error != ERR_OK)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].fetch_commit_calcDerivedStats: fetchStats expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
            allTestsPassed = false;
            return;
        }

        // 3. commitStats
        error = updateStatsHelper.commitStats();
        TRACE_LOG("[TestUpdateStatsHelperCommand].fetch_commit_calcDerivedStats: commitStats error code: " << ErrorHelp::getErrorName(error));
        if (error != ERR_OK)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].fetch_commit_calcDerivedStats: commitStats expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
            allTestsPassed = false;
            return;
        }

        // 4. getUpdateRowKey
        Stats::UpdateRowKey* key = builder.getUpdateRowKey("Core", mRequest.getPlayerId());
        if (key == nullptr)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].fetch_commit_calcDerivedStats: getUpdateRowKey failed because key is nullptr");
            allTestsPassed = false;
            return;
        }

        // 5. calcDerivedStats
        error = updateStatsHelper.calcDerivedStats();
        TRACE_LOG("[TestUpdateStatsHelperCommand].fetch_commit_calcDerivedStats: calcDerivedStats error code: " << ErrorHelp::getErrorName(error));
        if (error != ERR_SYSTEM)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].fetch_commit_calcDerivedStats: calcDerivedStats expecting error code: ERR_SYSTEM, but got: " << ErrorHelp::getErrorName(error));
            allTestsPassed = false;
        }
    }

    void fetch_setValue_calcDerivedStats()
    {
        Stats::UpdateStatsRequestBuilder builder;
        builder.startStatRow("Core", mRequest.getPlayerId());
        builder.incrementStat("kills", 2);
        builder.incrementStat("deaths", 1);
        builder.completeStatRow();

        Stats::UpdateStatsHelper updateStatsHelper;
        BlazeRpcError error = ERR_OK;

        // 1. initializeStatUpdate
        error = updateStatsHelper.initializeStatUpdate(builder);
        TRACE_LOG("[TestUpdateStatsHelperCommand].fetch_setValue_calcDerivedStats: initializeStatUpdate error code: " << ErrorHelp::getErrorName(error));
        if (error != ERR_OK)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].fetch_setValue_calcDerivedStats: initializeStatUpdate expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
            allTestsPassed = false;
            return;
        }

        // 2. fetchStats
        error = updateStatsHelper.fetchStats();
        TRACE_LOG("[TestUpdateStatsHelperCommand].fetch_setValue_calcDerivedStats: fetchStats error code: " << ErrorHelp::getErrorName(error));
        if (error != ERR_OK)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].fetch_setValue_calcDerivedStats: fetchStats expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
            allTestsPassed = false;
            return;
        }

        // 3. getUpdateRowKey
        Stats::UpdateRowKey* key = builder.getUpdateRowKey("Core", mRequest.getPlayerId());
        if (key == nullptr)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].fetch_setValue_calcDerivedStats: getUpdateRowKey failed because key is nullptr");
            allTestsPassed = false;
            return;
        }

        // 3.5. getValueFloat
        int64_t dbKills = updateStatsHelper.getValueInt(key, "kills", Stats::STAT_PERIOD_ALL_TIME, true);
        int64_t dbDeaths = updateStatsHelper.getValueInt(key, "deaths", Stats::STAT_PERIOD_ALL_TIME, true);
        int64_t oldKills = updateStatsHelper.getValueInt(key, "kills", Stats::STAT_PERIOD_ALL_TIME, false);
        int64_t oldDeaths = updateStatsHelper.getValueInt(key, "deaths", Stats::STAT_PERIOD_ALL_TIME, false);
        TRACE_LOG("[TestUpdateStatsHelperCommand].fetch_setValue_calcDerivedStats: getValue kills: " << oldKills << " deaths: " << oldDeaths);
        if (dbKills != 0 || dbDeaths != 0 || oldKills != 2 || oldDeaths != 1)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].fetch_setValue_calcDerivedStats: getValue db kills expecting: 0, but got: " << dbKills << "db deaths expecting: 0, but got: " << dbDeaths <<
                " updated kills expecting: 12, but got: " << oldKills << " updated deaths expecting: 3, but got: " << oldDeaths );
            allTestsPassed = false;
        }

        // 4. setValueInt
        int32_t newKills = 10;
        int32_t newDeaths = 2;
        updateStatsHelper.setValueInt(key, "kills", Stats::STAT_PERIOD_ALL_TIME, newKills);
        updateStatsHelper.setValueInt(key, "deaths", Stats::STAT_PERIOD_ALL_TIME, newDeaths);

        // 5. calcDerivedStats
        error = updateStatsHelper.calcDerivedStats();
        TRACE_LOG("[TestUpdateStatsHelperCommand].fetch_setValue_calcDerivedStats: calcDerivedStats error code: " << ErrorHelp::getErrorName(error));
        if (error != ERR_OK)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].fetch_setValue_calcDerivedStats: calcDerivedStats expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
            allTestsPassed = false;
            return;
        }

        // 6. getValueFloat
        int64_t kills = updateStatsHelper.getValueInt(key, "kills", Stats::STAT_PERIOD_ALL_TIME, false);
        int64_t deaths = updateStatsHelper.getValueInt(key, "deaths", Stats::STAT_PERIOD_ALL_TIME, false);
        float_t killDeathRatio = updateStatsHelper.getValueFloat(key, "killDeathRatio", Stats::STAT_PERIOD_ALL_TIME, false);
        TRACE_LOG("[TestUpdateStatsHelperCommand].fetch_setValue_calcDerivedStats: getValue kills: " << kills << " deaths: " << deaths << " killDeathRatio: " << killDeathRatio);
        if (killDeathRatio != 5.0 || kills != 10 || deaths != 2)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].fetch_setValue_calcDerivedStats: getValue killDeathRatio expecting: 5.0, but got: " << killDeathRatio <<
                " kills expecting: 10, but got: " << kills << " deaths expecting: 2, but got: " << deaths );
            allTestsPassed = false;
        }
    }

    void getValue_fromProvider()
    {
        Stats::UpdateStatsRequestBuilder builder;
        builder.startStatRow("Core", mRequest.getPlayerId());
        builder.completeStatRow();

        Stats::UpdateStatsHelper updateStatsHelper;
        BlazeRpcError error = ERR_OK;

        // 1. initializeStatUpdate
        error = updateStatsHelper.initializeStatUpdate(builder);
        TRACE_LOG("[TestUpdateStatsHelperCommand].getValue_fromProvider: initializeStatUpdate error code: " << ErrorHelp::getErrorName(error));
        if (error != ERR_OK)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].getValue_fromProvider: initializeStatUpdate expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
            allTestsPassed = false;
            return;
        }

        // 2. fetchStats
        error = updateStatsHelper.fetchStats();
        TRACE_LOG("[TestUpdateStatsHelperCommand].getValue_fromProvider: fetchStats error code: " << ErrorHelp::getErrorName(error));
        if (error != ERR_OK)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].getValue_fromProvider: fetchStats expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
            allTestsPassed = false;
            return;
        }

        // 3. getUpdateRowKey
        Stats::UpdateRowKey* key = builder.getUpdateRowKey("Core", mRequest.getPlayerId());
        if (key == nullptr)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].getValue_fromProvider: getUpdateRowKey failed because key is nullptr");
            allTestsPassed = false;
            return;
        }

        // 4. getValueInt
        int32_t originalKills = (int32_t)updateStatsHelper.getValueInt(key, "kills", Stats::STAT_PERIOD_ALL_TIME, true);
        TRACE_LOG("[TestUpdateStatsHelperCommand].getValue_fromProvider: getValueInt originalKills: " << originalKills);

        // 5. setValueInt to a different value
        int32_t newKills = originalKills + 1;
        updateStatsHelper.setValueInt(key, "kills", Stats::STAT_PERIOD_ALL_TIME, newKills);

        // 6. getValueInt with fromProvider set to true
        int32_t fromProviderKills = (int32_t)updateStatsHelper.getValueInt(key, "kills", Stats::STAT_PERIOD_ALL_TIME, true);
        TRACE_LOG("[TestUpdateStatsHelperCommand].getValue_fromProvider: getValueInt fromProviderKills: " << fromProviderKills);
        if (fromProviderKills != originalKills)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].getValue_fromProvider: fromProviderKills: "<< fromProviderKills << " doesn't equal to originalKills: " << originalKills);
            allTestsPassed = false;
            return;
        }

        // 7. getValueInt with fromProvider set to false
        int32_t cachedKills = (int32_t)updateStatsHelper.getValueInt(key, "kills", Stats::STAT_PERIOD_ALL_TIME, false);
        TRACE_LOG("[TestUpdateStatsHelperCommand].getValue_fromProvider: getValueInt cachedKills: " << cachedKills);
        if (cachedKills != newKills)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].getValue_fromProvider: cachedKills: "<< cachedKills << " doesn't equal to newKills: " << newKills);
            allTestsPassed = false;
            return;
        }

        // 8. commitStats
        error = updateStatsHelper.commitStats();
        TRACE_LOG("[TestUpdateStatsHelperCommand].getValue_fromProvider: commitStats error code: " << ErrorHelp::getErrorName(error));
        if (error != ERR_OK)
        {
            ERR_LOG("[TestUpdateStatsHelperCommand].getValue_fromProvider: commitStats expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
            allTestsPassed = false;
        }
    }

    TestUpdateStatsHelperCommandStub::Errors execute() override
    {
        // Run the tests
        fetch_setValue_calcDerivedStats();
        normal_case();
        no_init_commit();
        //timeout_commit(); // failing in commitStats(): returns ERR_OK instead of ERR_SYSTEM. Related to GOS-12552 which will be fixed in Winter 13 0.4
        abort_commit();
        // no_fetch_getValue(); // Expected Behavior: ASSERTION FAILED! This test must be enabled and run manually
        // no_fetch_setValue(); // Expected Behavior: ASSERTION FAILED! This test must be enabled and run manually
        // fetch_commit_getValue(); // Expected Behavior: ASSERTION FAILED! This test must be enabled and run manually
        // fetch_commit_setValue(); // Expected Behavior: ASSERTION FAILED! This test must be enabled and run manually
        fetch_commit_calcDerivedStats();
        getValue_fromProvider();

        // Determine the return error code
        if (allTestsPassed)
        {
            return ERR_OK;
        }
        else
        {
            return ERR_SYSTEM;
        }
    }
};

DEFINE_TESTUPDATESTATSHELPER_CREATE()

} //Arson
} //Blaze
