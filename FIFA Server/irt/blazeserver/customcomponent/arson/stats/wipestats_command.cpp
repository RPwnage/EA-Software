/*************************************************************************************************/
/*!
    \file   wipestats_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"
// arson includes
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/wipestats_stub.h"
#include "stats/rpc/statsslave_stub.h"

namespace Blaze
{
namespace Arson
{
class WipeStatsCommand : public WipeStatsCommandStub
{
public:
    WipeStatsCommand(
        Message* message, Blaze::Stats::WipeStatsRequest* request, ArsonSlaveImpl* componentImpl)
        : WipeStatsCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~WipeStatsCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    WipeStatsCommandStub::Errors execute() override
    {
        
        Stats::StatsSlave * statsComponent = nullptr;
        statsComponent = static_cast<Stats::StatsSlave*>(gController->getComponent(Stats::StatsSlave::COMPONENT_ID));

        if (statsComponent == nullptr)
        {
            WARN_LOG("WipeStatsCommand.execute() - Stats component is not available");
            return ERR_SYSTEM;
        }

        BlazeRpcError err = statsComponent->wipeStats(mRequest);
        return arsonErrorFromStatsError(err);
    }

    static Errors arsonErrorFromStatsError(BlazeRpcError error);
};

DEFINE_WIPESTATS_CREATE()

WipeStatsCommandStub::Errors WipeStatsCommand::arsonErrorFromStatsError(BlazeRpcError error)
{
    Errors result = ERR_SYSTEM;
    switch (error)
    {
        case ERR_OK: result = ERR_OK; break;
        case ERR_SYSTEM: result = ERR_SYSTEM; break;
        case ERR_TIMEOUT: result = ERR_TIMEOUT; break;
        case ERR_AUTHORIZATION_REQUIRED: result = ERR_AUTHORIZATION_REQUIRED; break;
        case Blaze::STATS_ERR_UNKNOWN_CATEGORY: result = ARSON_ERR_STATS_UNKNOWN_CATEGORY; break;
        case Blaze::STATS_ERR_INVALID_OPERATION: result = ARSON_ERR_STATS_INVALID_OPERATION; break;
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[WipeStatsCommand].arsonErrorFromStatsError: unexpected error(0x" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
