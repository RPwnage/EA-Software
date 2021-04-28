/*************************************************************************************************/
/*!
    \file   processpetition_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class RevokePetitionCommand

    takes a response to the petition and update the table

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
#include "clubs/rpc/clubsslave/revokepetition_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

    class RevokePetitionCommand : public RevokePetitionCommandStub, private ClubsCommandBase
    {
    public:
        RevokePetitionCommand(Message* message, ProcessPetitionRequest* request, ClubsSlaveImpl* componentImpl)
            : RevokePetitionCommandStub(message, request),
            ClubsCommandBase(componentImpl)
        {
        }

        ~RevokePetitionCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        RevokePetitionCommandStub::Errors execute() override
        {
            TRACE_LOG("[RevokePetitionCommand] start");

            BlazeId requestorId = gCurrentUserSession->getBlazeId();

            ClubsDbConnector dbc(mComponent); 
            if (dbc.acquireDbConnection(false) == nullptr)
                return ERR_SYSTEM;
            
            BlazeRpcError error;
            ClubMessage messageToRevoke;
            ClubMembership clubMembershipRequestor;

            error = dbc.getDb().getClubMessage(mRequest.getPetitionId(), messageToRevoke);

            // Ensure the requested operation is on a valid message
            if (error != Blaze::ERR_OK)
            {
                return CLUBS_ERR_INVALID_ARGUMENT;
            }


            // To revoke an petition, the requester must be the sender of the petition
            if (requestorId != messageToRevoke.getSender().getBlazeId())
            {
                // This petition was sent by another user and so this requester cannot revoke it
                return CLUBS_ERR_NO_PRIVILEGE;
            }

            if (!dbc.startTransaction())
                return ERR_SYSTEM;

            error = dbc.getDb().deleteClubMessage(mRequest.getPetitionId());

            if (error != Blaze::ERR_OK)
            {
                ERR_LOG("[RevokePetitionCommand] error (" << ErrorHelp::getErrorName(error) << ") user " << requestorId << " cannot revoke petition "
                    << mRequest.getPetitionId());
                dbc.completeTransaction(false); 
                return CLUBS_ERR_NO_PRIVILEGE;
            }

            if (!dbc.completeTransaction(true))
            {
                ERR_LOG("[RevokePetitionCommand] could not commit transaction");
                return ERR_SYSTEM;
            }

            error = mComponent->notifyPetitionRevoked(
                messageToRevoke.getClubId(),
                mRequest.getPetitionId(),
                requestorId);
                            
            if (error != Blaze::ERR_OK)
            {
                WARN_LOG("[RevokePetitionCommand] failed to send notifications after user " << requestorId << " revoked petition"
                    " to join club " << messageToRevoke.getClubId() << " because custom code notifyPetitionRevoked returned error code " << SbFormats::HexLower(error) );
            }
            
            mComponent->mPerfMessages.increment();
            
            return ERR_OK;

        }
    };

    // static factory method impl
    DEFINE_REVOKEPETITION_CREATE()

} // Clubs
} // Blaze
