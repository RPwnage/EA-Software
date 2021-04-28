/*************************************************************************************************/
/*!
    \file   Updatestats_command.cpp


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
#include "arson/rpc/arsonslave/updatestats_stub.h"
#include "component/stats/rpc/statsslave_stub.h"

namespace Blaze
{
namespace Arson
{
class UpdateStatsCommand : public UpdateStatsCommandStub
{
public:
    UpdateStatsCommand(
        Message* message, Blaze::Stats::UpdateStatsRequest* request, ArsonSlaveImpl* componentImpl)
        : UpdateStatsCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~UpdateStatsCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    UpdateStatsCommandStub::Errors execute() override
    {
        
        Stats::StatsSlave * statsComponent = nullptr;
        statsComponent = static_cast<Stats::StatsSlave*>(gController->getComponent(Stats::StatsSlave::COMPONENT_ID));
        
        if (statsComponent == nullptr)
        {
            WARN_LOG("UpdateStatsCommand.execute() - Stats component is not available");
            return ERR_SYSTEM;
        }

        BlazeRpcError err = statsComponent->updateStats(mRequest);
        
        return arsonErrorFromStatsError(err);
    }

    static Errors arsonErrorFromStatsError(BlazeRpcError error);
};

DEFINE_UPDATESTATS_CREATE();

UpdateStatsCommandStub::Errors UpdateStatsCommand::arsonErrorFromStatsError(BlazeRpcError error)
{
    Errors result = ERR_SYSTEM;
    switch (error)
    {
        case ERR_OK: result = ERR_OK; break;
        case ERR_SYSTEM: result = ERR_SYSTEM; break;
        case ERR_TIMEOUT: result = ERR_TIMEOUT; break;
        case ERR_AUTHORIZATION_REQUIRED: result = ERR_AUTHORIZATION_REQUIRED; break;
        case Blaze::STATS_ERR_UNKNOWN_CATEGORY: result = ARSON_ERR_STATS_UNKNOWN_CATEGORY; break;
        case Blaze::STATS_ERR_BAD_PERIOD_TYPE: result = ARSON_ERR_STATS_BAD_PERIOD_TYPE; break;
        case Blaze::STATS_ERR_BAD_SCOPE_INFO: result = ARSON_ERR_STATS_BAD_SCOPE_INFO; break;
        case Blaze::STATS_ERR_STAT_NOT_FOUND: result = ARSON_ERR_STAT_NOT_FOUND; break;
        case Blaze::STATS_ERR_INVALID_UPDATE_TYPE: result = ARSON_ERR_STATS_INVALID_UPDATE_TYPE; break;
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[UpdateStatsCommand].arsonErrorFromStatsError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
