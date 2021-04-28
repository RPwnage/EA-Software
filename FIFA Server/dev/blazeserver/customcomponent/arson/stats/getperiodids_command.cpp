/*************************************************************************************************/
/*!
    \file   GetPeriodIds_command.cpp


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
#include "arson/rpc/arsonslave/getperiodids_stub.h"
#include "component/stats/rpc/statsslave_stub.h"

namespace Blaze
{
namespace Arson
{
class GetPeriodIdsCommand : public GetPeriodIdsCommandStub
{
public:
    GetPeriodIdsCommand(
        Message* message, ArsonSlaveImpl* componentImpl)
        : GetPeriodIdsCommandStub(message),
        mComponent(componentImpl)
    {
    }

    ~GetPeriodIdsCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    GetPeriodIdsCommandStub::Errors execute() override
    {
        
        Stats::StatsSlaveStub * statsComponent = nullptr;
        statsComponent = static_cast<Stats::StatsSlaveStub*>(gController->getComponent(Stats::StatsSlaveStub::COMPONENT_ID));
        
        if (statsComponent == nullptr)
        {
            WARN("GetPeriodIdsCommand.execute() - Stats component is not available");
            return ERR_SYSTEM;
        }
        Blaze::Stats::PeriodIds response;
        BlazeRpcError err = statsComponent->getPeriodIds(response);
        
        translatePeriodIdsBlazeToArsonTDF(response);
        return arsonErrorFromStatsError(err);
    }

    static Errors arsonErrorFromStatsError(BlazeRpcError error);
    void translatePeriodIdsBlazeToArsonTDF(Blaze::Stats::PeriodIds & blazeTDF);
};

DEFINE_GETPERIODIDS_CREATE();

void GetPeriodIdsCommand::translatePeriodIdsBlazeToArsonTDF(Blaze::Stats::PeriodIds & blazeTDF)
{
    mResponse.setCurrentDailyPeriodId(blazeTDF.getCurrentDailyPeriodId());
    mResponse.setCurrentMonthlyPeriodId(blazeTDF.getCurrentMonthlyPeriodId());
    mResponse.setCurrentWeeklyPeriodId(blazeTDF.getCurrentWeeklyPeriodId());
    mResponse.setDailyBuffer(blazeTDF.getDailyBuffer());
    mResponse.setDailyHour(blazeTDF.getDailyHour());
    mResponse.setDailyRetention(blazeTDF.getDailyRetention());
    mResponse.setMonthlyBuffer(blazeTDF.getMonthlyBuffer());
    mResponse.setMonthlyDay(blazeTDF.getMonthlyDay());
    mResponse.setMonthlyHour(blazeTDF.getMonthlyHour());
    mResponse.setMonthlyRetention(blazeTDF.getMonthlyRetention());
    mResponse.setWeeklyBuffer(blazeTDF.getWeeklyBuffer());
    mResponse.setWeeklyDay(blazeTDF.getWeeklyDay());
    mResponse.setWeeklyHour(blazeTDF.getWeeklyHour());
    mResponse.setWeeklyRetention(blazeTDF.getWeeklyRetention());
}

GetPeriodIdsCommandStub::Errors GetPeriodIdsCommand::arsonErrorFromStatsError(BlazeRpcError error)
{
    Errors result = ERR_SYSTEM;
    switch (error)
    {
        case ERR_OK: result = ERR_OK; break;
        case ERR_SYSTEM: result = ERR_SYSTEM; break;
        case ERR_TIMEOUT: result = ERR_TIMEOUT; break;
        case ERR_AUTHORIZATION_REQUIRED: result = ERR_AUTHORIZATION_REQUIRED; break;
        default:
        {
            //got an error not defined in rpc definition, log it
            char8_t errorHexBuf[32];
            blaze_snzprintf(errorHexBuf, sizeof(errorHexBuf), "%x", error);

            TRACE_LOG("[GetPeriodIdsCommand].arsonErrorFromStatsError: unexpected error(0x" << errorHexBuf << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
