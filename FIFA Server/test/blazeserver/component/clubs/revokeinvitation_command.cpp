/*************************************************************************************************/
/*!
    \file   processinvitation_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class RevokeInvitationCommand

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
#include "clubs/rpc/clubsslave/revokeinvitation_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

    class RevokeInvitationCommand : public RevokeInvitationCommandStub, private ClubsCommandBase
    {
    public:
        RevokeInvitationCommand(Message* message, ProcessInvitationRequest* request, ClubsSlaveImpl* componentImpl)
            : RevokeInvitationCommandStub(message, request),
            ClubsCommandBase(componentImpl)
        {
        }

        ~RevokeInvitationCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        RevokeInvitationCommandStub::Errors execute() override
        {
            TRACE_LOG("[RevokeInvitationCommand] start");

            BlazeId requestorId = gCurrentUserSession->getBlazeId();

            ClubsDbConnector dbc(mComponent); 
            if (dbc.acquireDbConnection(false) == nullptr)
                return ERR_SYSTEM;
            
            BlazeRpcError error;
            ClubMessage messageToRevoke;

            error = dbc.getDb().getClubMessage(mRequest.getInvitationId(), messageToRevoke);

            // Ensure the requested operation is on a valid message
            if (error != Blaze::ERR_OK)
            {
                return CLUBS_ERR_INVALID_ARGUMENT;
            }

            MembershipStatus mshipStatus;
            MemberOnlineStatus onlSatus;
            TimeValue memberSince;
            ClubId clubId = messageToRevoke.getClubId();
            error = mComponent->getMemberStatusInClub(
                clubId, 
                requestorId, 
                dbc.getDb(), 
                mshipStatus, 
                onlSatus,
                memberSince);

            if (error != Blaze::ERR_OK)
            {
                return commandErrorFromBlazeError(error);
            }

            // To revoke an invitation, the requester must be a GM of the club that sent the invite
            if (mshipStatus < CLUBS_GM)
            {
                return CLUBS_ERR_NO_PRIVILEGE;
            }

            if (!dbc.startTransaction())
                return ERR_SYSTEM;

            error = dbc.getDb().deleteClubMessage(mRequest.getInvitationId());

            if (error != Blaze::ERR_OK)
            {
                ERR_LOG("[RevokeInvitationCommand] error (" << ErrorHelp::getErrorName(error) << ") user " << requestorId << " failed to revoke invitation "
                        << mRequest.getInvitationId());
                dbc.completeTransaction(false); 
                return commandErrorFromBlazeError(error);
            }

            error = dbc.getDb().updateClubLastActive(clubId);

            if (error != (BlazeRpcError)ERR_OK)
            {
                ERR_LOG("[RevokeInvitationCommand] failed to update last active time for clubid " 
                        << clubId << ", error was " << ErrorHelp::getErrorName(error));
                dbc.completeTransaction(false); 
                return commandErrorFromBlazeError(error);
            }

            if (!dbc.completeTransaction(true))

            {
                ERR_LOG("[RevokeInvitationCommand] could not commit transaction");
                return ERR_SYSTEM;
            }
            
            BlazeRpcError res = mComponent->notifyInvitationRevoked(
                clubId, 
                requestorId, 
                messageToRevoke.getReceiver().getBlazeId());
            
            if (res != Blaze::ERR_OK)
            {
                WARN_LOG("[RevokeInvitationCommand] could not send notifications after invitation to user " 
                    << messageToRevoke.getReceiver().getBlazeId() << " was revoked because custom processing code notifyInvitationRevoked returned error: " 
                    << SbFormats::HexLower(res) << ".");
            }

            // AT THIS POINT BLOCKING OPERATION MAY HAVE FAILED
            // DO NOT USE gCurrentUserSession

            mComponent->mPerfMessages.increment();

            return ERR_OK;

        }
    };

    // static factory method impl
    DEFINE_REVOKEINVITATION_CREATE()

} // Clubs
} // Blaze
