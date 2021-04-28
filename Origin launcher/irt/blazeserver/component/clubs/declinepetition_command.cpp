/*************************************************************************************************/
/*!
    \file   processpetition_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class DeclinePetitionCommand

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
#include "clubs/rpc/clubsslave/declinepetition_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

class DeclinePetitionCommand : public DeclinePetitionCommandStub, private ClubsCommandBase
{
public:
    DeclinePetitionCommand(Message* message, ProcessPetitionRequest* request, ClubsSlaveImpl* componentImpl)
        : DeclinePetitionCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~DeclinePetitionCommand() override
    {
        }

        /* Private methods *******************************************************************************/
private:

    DeclinePetitionCommandStub::Errors execute() override
    {
        TRACE_LOG("[DeclinePetitionCommand] start");

        BlazeId requestorId = gCurrentUserSession->getBlazeId();

        ClubsDbConnector dbc(mComponent); 
        if (dbc.acquireDbConnection(false) == nullptr)
            return ERR_SYSTEM;

        BlazeRpcError error;
        ClubMessage messageToDecline;

        MembershipStatus mshipStatus;
        MemberOnlineStatus onlSatus;
        TimeValue memberSince;
        ClubId requestorClubId;

        error = dbc.getDb().getClubMessage(mRequest.getPetitionId(), messageToDecline);

        // Ensure the requested operation is on a valid message
        if (error != Blaze::ERR_OK)
        {
            dbc.completeTransaction(false); 
            return CLUBS_ERR_INVALID_ARGUMENT;
        }

        requestorClubId = messageToDecline.getClubId();
        error = mComponent->getMemberStatusInClub(
            requestorClubId, 
            requestorId, 
            dbc.getDb(), 
            mshipStatus, 
            onlSatus,
            memberSince);

        if (error != Blaze::ERR_OK)
        {
            dbc.completeTransaction(false); 
            return commandErrorFromBlazeError(error);
        }

        // To decline an petition, the requester must be the GM of the club
        if (mshipStatus < CLUBS_GM)
        {
            // This petition is for another club, so this requester cannot decline it
            dbc.completeTransaction(false); 
            return CLUBS_ERR_NO_PRIVILEGE;
        }

        if (!dbc.startTransaction())
            return ERR_SYSTEM;

        error = dbc.getDb().deleteClubMessage(mRequest.getPetitionId());

        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("[DeclinePetitionCommand] error (" << ErrorHelp::getErrorName(error) << ") user " << requestorId << " failed to decline petition "
                << mRequest.getPetitionId());
            dbc.completeTransaction(false); 
            return commandErrorFromBlazeError(error);
        }

        error = dbc.getDb().updateClubLastActive(requestorClubId);

        if (error != (BlazeRpcError)ERR_OK)
        {
            ERR_LOG("[DeclinePetitionCommand] failed to update last active time for clubid " << requestorClubId);
            dbc.completeTransaction(false); 
            return commandErrorFromBlazeError(error);
        }

        Club club;
        uint64_t version = 0;
        error = dbc.getDb().getClub(requestorClubId, &club, version);

        if (!dbc.completeTransaction(true))
        {
            ERR_LOG("[DeclinePetitionCommand] could not commit transaction");
            return ERR_SYSTEM;
        }

        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("[DeclinePetitionCommand] Failed to send message to user " << messageToDecline.getSender().getBlazeId() 
                << " because of failure to obtain club [id] " << requestorClubId);
        }
        else
        { 
            BlazeRpcError result = mComponent->notifyPetitionDeclined(
                messageToDecline.getClubId(),
                club.getName(),
                mRequest.getPetitionId(),
                requestorId,
                messageToDecline.getSender().getBlazeId());

            if (result != Blaze::ERR_OK)
            {
                WARN_LOG("[DeclinePetitionCommand] failed to send notifications after petition from user " 
                    << messageToDecline.getSender().getBlazeId() << " to join club " << messageToDecline.getClubId() 
                    << " has been declined because custom code notifyPetitionDeclined returned error code " << SbFormats::HexLower(error));
            }

        }

        mComponent->mPerfAdminActions.increment();

        return ERR_OK;
    }

};
// static factory method impl
DEFINE_DECLINEPETITION_CREATE()

} // Clubs
} // Blaze
