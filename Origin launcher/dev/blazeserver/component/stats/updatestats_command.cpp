/*************************************************************************************************/
/*!
    \file   updatestats_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdateStatsCommand

    Update statistics.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "stats/statsslaveimpl.h"
#include "stats/rpc/statsslave/updatestats_stub.h"
#include "stats/updatestatshelper.h"

namespace Blaze
{
namespace Stats
{

class UpdateStatsCommand : public UpdateStatsCommandStub
{
public:
    UpdateStatsCommand(Message *message, UpdateStatsRequest *request, StatsSlaveImpl* componentImpl) 
    : UpdateStatsCommandStub(message, request),
      mComponent(componentImpl)
    {
    }

    BlazeRpcError updateStatsExecute()
    {
        BlazeRpcError result = ERR_OK;
        if (mRequest.getStatUpdates().empty())
        {
            return result;
        }

        UpdateStatsHelper updateStatsHelper;
        result = updateStatsHelper.initializeStatUpdate(mRequest);
        if (result != ERR_OK)
            return result;
 
        result = updateStatsHelper.commitStats();
        return result;
    }

    UpdateStatsCommandStub::Errors execute() override
    {
        BlazeId blazeId = INVALID_BLAZE_ID; // default super user
        if (gCurrentUserSession != nullptr)
        {
            blazeId = gCurrentUserSession->getBlazeId();
        }
        if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_UPDATE_STATS))
        {
            WARN_LOG("[UpdateStatsCommand].execute: User [" << blazeId << "] attempted to update stats, no permission!");
            return commandErrorFromBlazeError(Blaze::ERR_AUTHORIZATION_REQUIRED);
        }


        mComponent->incrementUpdateStatsCount();

        BlazeRpcError error = updateStatsExecute();
        if (error == Blaze::ERR_OK)
        {
            INFO_LOG("[UpdateStatsCommand].execute: User [" << blazeId << "] updated stats, had permission.");
        }
        else
        {
            mComponent->incrementUpdateStatsFailureCount();
        }
        return (commandErrorFromBlazeError(error));
    }

private:
    StatsSlaveImpl* mComponent;
};

DEFINE_UPDATESTATS_CREATE()

} // Stats
} // Blaze
