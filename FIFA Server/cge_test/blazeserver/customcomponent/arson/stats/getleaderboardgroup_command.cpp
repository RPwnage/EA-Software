/*************************************************************************************************/
/*!
    \file  getleaderboardgroup_command.cpp

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
#include "arson/rpc/arsonslave/getleaderboardgroup_stub.h"
#include "stats/rpc/statsslave.h"

namespace Blaze
{
namespace Arson
{
    class GetLeaderboardGroupCommand : public GetLeaderboardGroupCommandStub
{
public:
    GetLeaderboardGroupCommand(
        Message* message, Blaze::Arson::LeaderboardGroupRequest* request, ArsonSlaveImpl* componentImpl)
        : GetLeaderboardGroupCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~GetLeaderboardGroupCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    GetLeaderboardGroupCommandStub::Errors execute() override
    {
      //  BlazeRpcError error;
        Blaze::Stats::StatsSlave *component = (Blaze::Stats::StatsSlave *) gController->getComponent(Blaze::Stats::StatsSlave::COMPONENT_ID);           

        if (component == nullptr)
        {
            return ARSON_ERR_STATS_COMPONENT_NOT_FOUND;
        }

        if(mRequest.getNullCurrentUserSession())
        {
            // Set the gCurrentUserSession to nullptr
            UserSession::setCurrentUserSessionId(INVALID_USER_SESSION_ID);
        }

        UserSession::SuperUserPermissionAutoPtr superPtr(true);
        BlazeRpcError err = component->getLeaderboardGroup(mRequest.getLeaderboardGroupRequest(), mResponse.getLeaderboardGroupResponse());

        return arsonErrorFromStatsError(err);
    }

    static Errors arsonErrorFromStatsError(BlazeRpcError error);
};

    DEFINE_GETLEADERBOARDGROUP_CREATE()

GetLeaderboardGroupCommandStub::Errors GetLeaderboardGroupCommand::arsonErrorFromStatsError(BlazeRpcError error)
{
    Errors result = ERR_SYSTEM;
    switch (error)
    {
        case ERR_OK: result = ERR_OK; break;
        case ERR_SYSTEM: result = ERR_SYSTEM; break;
        case ERR_TIMEOUT: result = ERR_TIMEOUT; break;
        case STATS_ERR_INVALID_LEADERBOARD_ID: result = ARSON_STATS_ERR_INVALID_LEADERBOARD_ID; break;
        case ARSON_ERR_STATS_COMPONENT_NOT_FOUND: result = ARSON_ERR_STATS_COMPONENT_NOT_FOUND; break;
        case STATS_ERR_CONFIG_NOTAVAILABLE: result = ARSON_STATS_ERR_CONFIG_NOTAVAILABLE; break;
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[GetLeaderboardGroupCommand].arsonErrorFromStatsError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
