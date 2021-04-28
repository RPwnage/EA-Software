/*************************************************************************************************/
/*!
    \file   processinvitation_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class DeclineInvitationCommand

    takes a response to the invitation and update the table

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"

// clubs includes
#include "clubs/rpc/clubsslave/declineinvitation_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

class DeclineInvitationCommand : public DeclineInvitationCommandStub, private ClubsCommandBase
{
public:
    DeclineInvitationCommand(Message* message, ProcessInvitationRequest* request, ClubsSlaveImpl* componentImpl)
        : DeclineInvitationCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~DeclineInvitationCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:

    DeclineInvitationCommandStub::Errors execute() override
    {
        TRACE_LOG("[DeclineInvitationCommand] start");

        BlazeId requestorId = gCurrentUserSession->getBlazeId();

        ClubsDbConnector dbc(mComponent); 
        if (dbc.acquireDbConnection(false) == nullptr)
            return ERR_SYSTEM;

        BlazeRpcError error;
        ClubMessage messageToDecline;

        error = dbc.getDb().getClubMessage(mRequest.getInvitationId(), messageToDecline);

        // Ensure the requested operation is on a valid message
        if (error != Blaze::ERR_OK)
        {
            dbc.completeTransaction(false); 
            return CLUBS_ERR_INVALID_ARGUMENT;
        }

        // To decline an invitation, the requester must be the same as the invitee
        if (requestorId != messageToDecline.getReceiver().getBlazeId())
        {
            // This invitation is for another user, so this requester cannot decline it
            dbc.completeTransaction(false); 
            return CLUBS_ERR_NO_PRIVILEGE;
        }

        if (!dbc.startTransaction())
            return ERR_SYSTEM;

        error = dbc.getDb().deleteClubMessage(mRequest.getInvitationId());

        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("[DeclineInvitationCommand] error (" << ErrorHelp::getErrorName(error) << ") user " << requestorId
                << " failed to decline invitation " << mRequest.getInvitationId());
            dbc.completeTransaction(false); 
            return commandErrorFromBlazeError(error);
        }

        if (!dbc.completeTransaction(true))
        {
            ERR_LOG("[DeclineInvitationCommand] could not commit transaction");
            return ERR_SYSTEM;
        }

        error = mComponent->notifyInvitationDeclined(
            messageToDecline.getClubId(),
            gCurrentUserSession->getBlazeId());

        if (error != Blaze::ERR_OK)
        {
            WARN_LOG("[DeclineInvitationCommand] Faile to send notifications after User " << requestorId << " declined invitation " 
                << mRequest.getInvitationId() << " bacause custom code notifyInvitationDeclined returned error code " << SbFormats::HexLower(error) );
        }

        // AT THIS POINT BLOCKING OPERATION MAY HAVE FAILED
        // DO NOT USE gCurrentUserSession

        mComponent->mPerfAdminActions.increment();

        return ERR_OK;
    }

};
// static factory method impl
DEFINE_DECLINEINVITATION_CREATE()

} // Clubs
} // Blaze
