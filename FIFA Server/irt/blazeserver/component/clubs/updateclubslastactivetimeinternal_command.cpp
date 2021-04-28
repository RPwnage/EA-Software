/*************************************************************************************************/
/*!
    \file   updateclublastactivetimeinternal_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdateClubLastActiveTimeInternal

    Updates the last active time to the current time for the requested clubs

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"

// clubs includes
#include "clubs/rpc/clubsslave/updateclublastactivetimeinternal_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

    class UpdateClubLastActiveTimeInternalCommand : public UpdateClubLastActiveTimeInternalCommandStub, private ClubsCommandBase
    {
    public:
        UpdateClubLastActiveTimeInternalCommand(Message* message, UpdateClubLastActiveTimeRequest* request, ClubsSlaveImpl* componentImpl)
            : UpdateClubLastActiveTimeInternalCommandStub(message, request),
            ClubsCommandBase(componentImpl)
        {
        }

        ~UpdateClubLastActiveTimeInternalCommand() override
        {
        }

    /* Private methods *******************************************************************************/
    private:
        UpdateClubLastActiveTimeInternalCommandStub::Errors execute() override
        {
            TRACE_LOG("[UpdateClubLastActiveTimeInternalCommand] start");

            BlazeRpcError error = Blaze::ERR_OK;
            ClubId clubId = mRequest.getClubId();

            ClubsDbConnector dbc(mComponent);
            if (dbc.acquireDbConnection(false) == nullptr)
                return ERR_SYSTEM;

            error = dbc.getDb().updateClubLastActive(clubId);
            if (error != Blaze::ERR_OK)
            {
                ERR_LOG("[UpdateClubLastActiveTimeInternalCommand] Could not update last active, database error " << ErrorHelp::getErrorName(error));
                dbc.completeTransaction(false);
                return commandErrorFromBlazeError(error);
            }

            return ERR_OK;
        }
    };

    // static factory method impl
    DEFINE_UPDATECLUBLASTACTIVETIMEINTERNAL_CREATE()
}
}
