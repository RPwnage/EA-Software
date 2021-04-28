/*************************************************************************************************/
/*!
    \file   acceptinvitation_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class AcceptInvitationCommand

    takes a response to the invitation and update the table

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/event/eventmanager.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"

// clubs includes
#include "clubs/rpc/clubsslave/acceptinvitation_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"
#include "clubsdbconnector.h"

namespace Blaze
{
namespace Clubs
{

class AcceptInvitationCommand : public AcceptInvitationCommandStub, private ClubsCommandBase
{
public:
    AcceptInvitationCommand(Message* message, ProcessInvitationRequest* request, ClubsSlaveImpl* componentImpl)
        : AcceptInvitationCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~AcceptInvitationCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:

    AcceptInvitationCommandStub::Errors execute() override
    {
        TRACE_LOG("[AcceptInvitationCommand] start");

        BlazeId rtorBlazeId = gCurrentUserSession->getBlazeId();

        ClubsDbConnector dbc(mComponent); 

        if (!dbc.startTransaction())
            return ERR_SYSTEM;

        BlazeRpcError error = Blaze::ERR_SYSTEM;

        error = dbc.getDb().lockUserMessages(rtorBlazeId);
        if (error != Blaze::ERR_OK)
        {
            dbc.completeTransaction(false);
            return commandErrorFromBlazeError(error);
        }

        ClubMessage messageToAccept;

        error = dbc.getDb().getClubMessage(mRequest.getInvitationId(), messageToAccept);

        // Ensure the requested operation is on a valid message
        if (error != Blaze::ERR_OK)
        {
            dbc.completeTransaction(false);
            return CLUBS_ERR_INVALID_ARGUMENT;
        }

        // To accept an invitation, the requester must be the same as the invitee
        if (rtorBlazeId != messageToAccept.getReceiver().getBlazeId())
        {
            // This invitation is for another user, so this requester cannot decline it
            dbc.completeTransaction(false);
            return CLUBS_ERR_NO_PRIVILEGE;
        }

        ClubId joinClubId = messageToAccept.getClubId();

        ClubPassword password;
        uint32_t messageId;
        error = mComponent->joinClub(joinClubId, true/*skipJoinAcceptanceCheck*/, false/*petitionIfJoinFails*/, true/*skipPasswordCheck*/, password,
            rtorBlazeId, false/*isGMFriend*/, mRequest.getInvitationId(), dbc, messageId);
        if (error != Blaze::ERR_OK)
        {
            dbc.completeTransaction(false);
            return commandErrorFromBlazeError(error);
        }

        if (!dbc.completeTransaction(true))
        {
            mComponent->removeMemberUserExtendedData(rtorBlazeId, joinClubId, UPDATE_REASON_USER_LEFT_CLUB);
            ERR_LOG("[AcceptInvitationCommand] could not commit transaction");
            return ERR_SYSTEM;
        }

        error = mComponent->notifyInvitationAccepted(
            joinClubId, 
            rtorBlazeId);

        if (error != Blaze::ERR_OK)
        {
            WARN_LOG("[AcceptInvitationCommand] failed to send notifications for invitation for user " << rtorBlazeId
                << " to join club " << joinClubId << " because custom code notifyAcceptInvitation returned error code " << SbFormats::HexLower(error) );
        }

        UserJoinedClub userJoinedClubEvent;
        userJoinedClubEvent.setJoiningPersonaId(rtorBlazeId);
        userJoinedClubEvent.setClubId(joinClubId);
        gEventManager->submitEvent(static_cast<uint32_t>(ClubsSlave::EVENT_USERJOINEDCLUBEVENT), userJoinedClubEvent, true /*logBinaryEvent*/);

        mComponent->mPerfJoinsLeaves.increment();

        return ERR_OK;
    }
};

// static factory method impl
DEFINE_ACCEPTINVITATION_CREATE()

} // Clubs
} // Blaze
