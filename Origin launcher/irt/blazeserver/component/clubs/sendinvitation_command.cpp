/*************************************************************************************************/
/*!
    \file   sendinvitation_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class SendInvitationCommand

    Invite someone else to join a club

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
#include "clubs/rpc/clubsslave/sendinvitation_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

    class SendInvitationCommand : public SendInvitationCommandStub, private ClubsCommandBase
    {
    public:
        SendInvitationCommand(Message* message, SendInvitationRequest* request, ClubsSlaveImpl* componentImpl)
            : SendInvitationCommandStub(message, request),
            ClubsCommandBase(componentImpl)
        {
        }

        ~SendInvitationCommand() override
        {
        }

    /* Private methods *******************************************************************************/
    private:

        SendInvitationCommandStub::Errors execute() override
        {
            TRACE_LOG("[SendInvitationCommand] start");
            BlazeRpcError error;

            UserInfoPtr userInfo = nullptr;
            error = gUserSessionManager->lookupUserInfoByBlazeId(mRequest.getBlazeId(), userInfo);
            if (error == USER_ERR_USER_NOT_FOUND)
                return CLUBS_ERR_INVALID_USER_ID;
            else if (error != Blaze::ERR_OK)
                return commandErrorFromBlazeError(error);

            BlazeId requestorId = gCurrentUserSession->getBlazeId();

            ClubsDbConnector dbc(mComponent);

            if (!dbc.startTransaction())
                return ERR_SYSTEM;

            MembershipStatus rtorMshipStatus;
            MemberOnlineStatus rtorOnlSatus;

            MembershipStatus subjMshipStatus = CLUBS_MEMBER;
            MemberOnlineStatus subjOnlSatus;
            TimeValue memberSince;

            error = mComponent->getMemberStatusInClub(
                mRequest.getClubId(), 
                requestorId, dbc.getDb(), 
                rtorMshipStatus, 
                rtorOnlSatus,
                memberSince);

            if (error != (BlazeRpcError)ERR_OK)
            {
                dbc.completeTransaction(false); 
                return commandErrorFromBlazeError(error);
            }

            // Check that the requester is not inviting himself to the club, which is obviously not valid
            if (mRequest.getBlazeId() == requestorId)
            {
                dbc.completeTransaction(false); 
                return CLUBS_ERR_INVALID_ARGUMENT;
            }

            // Check that requestor is GM of his/her club otherwise, no privileges to send an invitation
            if (rtorMshipStatus < CLUBS_GM)
            {
                dbc.completeTransaction(false); 
                return CLUBS_ERR_NO_PRIVILEGE;
            }

            // Try to get the invitee so we can ensure that he is not already a member of the requestor's club
            error = mComponent->getMemberStatusInClub(
                mRequest.getClubId(), 
                mRequest.getBlazeId(), 
                dbc.getDb(), 
                subjMshipStatus, 
                subjOnlSatus,
                memberSince);

            if (error == Blaze::ERR_OK)
            {
                dbc.completeTransaction(false); 
                return CLUBS_ERR_ALREADY_MEMBER;
            }

            if (error != static_cast<BlazeRpcError>(CLUBS_ERR_USER_NOT_MEMBER))
            {
                dbc.completeTransaction(false); 
                return commandErrorFromBlazeError(error);
            }

            // invitation can be sent only once to the same user
            error = mComponent->checkInvitationAlreadySent(dbc.getDb(), mRequest.getBlazeId(), mRequest.getClubId());
            
            if (error != (BlazeRpcError)ERR_OK)
            {
                if (error != (BlazeRpcError)CLUBS_ERR_INVITATION_ALREADY_SENT)
                {
                    // database error
                    ERR_LOG("[SendInvitationCommand] Database error " << ErrorHelp::getErrorName(error));
                }
                dbc.completeTransaction(false);
                return commandErrorFromBlazeError(error);
            }

            
            // GM cannot send an invitation if petition has been already filed
            error = mComponent->checkPetitionAlreadySent(dbc.getDb(), mRequest.getBlazeId(), mRequest.getClubId());
            
            if (error != (BlazeRpcError)ERR_OK)
            {
                if (error != (BlazeRpcError)CLUBS_ERR_PETITION_ALREADY_SENT)
                {
                    // database error
                    ERR_LOG("[SendInvitationCommand] Database error " << ErrorHelp::getErrorName(error));
                }
                dbc.completeTransaction(false);
                return commandErrorFromBlazeError(error);
            }
            
            // update database
            ClubMessage message;
            message.setClubId(mRequest.getClubId());
            message.setMessageType(CLUBS_INVITATION);
            message.getSender().setBlazeId(requestorId);
            message.getReceiver().setBlazeId(mRequest.getBlazeId());

            const ClubDomain *domain = nullptr;

            error = mComponent->getClubDomainForClub(mRequest.getClubId(), dbc.getDb(), domain);

            if (error != Blaze::ERR_OK)
            {
                dbc.completeTransaction(false);
                return commandErrorFromBlazeError(error);
            }

            // insert the invitation to club_messages table and also updates the last active time for the club
            error = dbc.getDb().insertClubMessage(message, domain->getClubDomainId(), domain->getMaxInvitationsPerUserOrClub());

            if (error != (BlazeRpcError)ERR_OK)
            {
                dbc.completeTransaction(false); 
                if (error == Blaze::CLUBS_ERR_MAX_MESSAGES_SENT)
                {
                    return CLUBS_ERR_MAX_INVITES_SENT;
                }
                else if (error == Blaze::CLUBS_ERR_MAX_MESSAGES_RECEIVED)
                {
                     return CLUBS_ERR_MAX_INVITES_RECEIVED;
                }
                else
                {
                     ERR_LOG("[SendInvitationCommand] failed to send invitation from user [blazeid]: " 
                             << requestorId << " to user [blazeid]: " << mRequest.getBlazeId() << ", error was " << ErrorHelp::getErrorName(error));
                    return commandErrorFromBlazeError(error);
                }
            }

            error = dbc.getDb().updateClubLastActive(mRequest.getClubId());

            if (error != (BlazeRpcError)ERR_OK)
            {
                ERR_LOG("[SendInvitationCommand] failed to update last active time for clubid " 
                        << mRequest.getClubId() << ", error was " << ErrorHelp::getErrorName(error));
                dbc.completeTransaction(false); 
                return commandErrorFromBlazeError(error);
            }

            if (!dbc.completeTransaction(true))
            {
                ERR_LOG("[SendInvitationCommand] could not commit transaction");
                return ERR_SYSTEM;
            }

            BlazeRpcError res = mComponent->notifyInvitationSent(
                mRequest.getClubId(),
                requestorId, 
                mRequest.getBlazeId());
            
            if (res != Blaze::ERR_OK)
            {
                WARN_LOG("[SendInvitationCommand] could not send notification after sending invitation to user " 
                    << mRequest.getBlazeId() << " because custom processing code notifyInvitationSent returned error: " << SbFormats::HexLower(res) << ".");
            }

            ClubInvitationSent clubInvitationSentEvent;
            clubInvitationSentEvent.setInviterPersonaId(requestorId);
            clubInvitationSentEvent.setInviteePersonaId(mRequest.getBlazeId());
            clubInvitationSentEvent.setClubId(mRequest.getClubId());
            gEventManager->submitEvent(static_cast<uint32_t>(ClubsSlave::EVENT_CLUBINVITATIONSENTEVENT), clubInvitationSentEvent, true /*logBinaryEvent*/);

            mComponent->mPerfMessages.increment();

            return ERR_OK;
        }
    };

    // static factory method impl

    DEFINE_SENDINVITATION_CREATE()

} // Clubs
} // Blaze
