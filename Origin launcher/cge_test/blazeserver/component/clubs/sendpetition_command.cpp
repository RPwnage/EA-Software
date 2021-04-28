/*************************************************************************************************/
/*!
    \file   sendpetition_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class SendPetitionCommand

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
#include "clubs/rpc/clubsslave/sendpetition_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

    class SendPetitionCommand : public SendPetitionCommandStub, private ClubsCommandBase
    {
    public:
        SendPetitionCommand(Message* message, SendPetitionRequest* request, ClubsSlaveImpl* componentImpl)
            : SendPetitionCommandStub(message, request),
            ClubsCommandBase(componentImpl)
        {
        }

        ~SendPetitionCommand() override
        {
        }

    /* Private methods *******************************************************************************/
    private:

        SendPetitionCommandStub::Errors execute() override
        {
            TRACE_LOG("[SendPetitionCommand] start");

            BlazeId requestorId = gCurrentUserSession->getBlazeId();

            ClubsDbConnector dbc(mComponent);

            if (!dbc.startTransaction())
                return ERR_SYSTEM;
            
            BlazeRpcError error;
            Club clubToPetition;

            uint64_t version = 0;
            error = dbc.getDb().getClub(mRequest.getClubId(), &clubToPetition, version);
            if (error != Blaze::ERR_OK)
            {
                if (error != Blaze::CLUBS_ERR_INVALID_CLUB_ID)
                {
                    ERR_LOG("[SendPetitionCommand] Database error " << ErrorHelp::getErrorName(error));
                }
                dbc.completeTransaction(false); 
                return commandErrorFromBlazeError(error);
            }

            // Check whether requestor is friend of a GM of this club.
            bool isGMFriend = mComponent->isClubGMFriend(mRequest.getClubId(), requestorId, dbc);

            // Check that this club is accepting petitions
            if (!mComponent->isClubPetitionAcctepted(clubToPetition.getClubSettings(), isGMFriend))
            {
                dbc.completeTransaction(false);
                return CLUBS_ERR_PETITION_DISABLED;
            }
            
            // petition can be sent only once
            error = mComponent->checkPetitionAlreadySent(dbc.getDb(), requestorId, mRequest.getClubId());
            if (error != (BlazeRpcError)ERR_OK)
            {
                dbc.completeTransaction(false);
                return commandErrorFromBlazeError(error);
            }
            
            // user cannot send petition if he was already invited to join
            error = mComponent->checkInvitationAlreadySent(dbc.getDb(), requestorId, mRequest.getClubId());
            
            if (error != (BlazeRpcError)ERR_OK)
            {
                // database error
                dbc.completeTransaction(false);
                return commandErrorFromBlazeError(error);
            }
            
            MembershipStatus rtorMshipStatus;
            MemberOnlineStatus rtorOnlSatus;
            TimeValue memberSince;

            // Check that the requester is not petitioning to a club that he already belongs to
            error = mComponent->getMemberStatusInClub(
                mRequest.getClubId(), 
                requestorId, dbc.getDb(), 
                rtorMshipStatus, 
                rtorOnlSatus,
                memberSince);

            if (error == (BlazeRpcError)ERR_OK)
            {
                dbc.completeTransaction(false);
                return CLUBS_ERR_ALREADY_MEMBER;
            }
            if (error != static_cast<BlazeRpcError>(CLUBS_ERR_USER_NOT_MEMBER))
            {
                dbc.completeTransaction(false);
                return commandErrorFromBlazeError(error);
            }

            // public club
            if (mComponent->isClubJoinAccepted(clubToPetition.getClubSettings(), false))
            {
                uint32_t messageId;
                error = mComponent->joinClub(mRequest.getClubId(), true/*skipJoinAcceptanceCheck*/, false/*petitionIfJoinFails*/, false/*skipPasswordCheck*/, mRequest.getPassword(),
                    requestorId, false/*isGMFriend*/, 0/*invitationId*/, dbc, messageId);
                if (error == Blaze::ERR_OK)
                {
                    if (!dbc.completeTransaction(true))
                    {
                        ERR_LOG("[SendPetitionCommand] could not commit transaction");
                        mComponent->removeMemberUserExtendedData(requestorId, mRequest.getClubId(), UPDATE_REASON_USER_LEFT_CLUB);
                        return ERR_SYSTEM;
                    }

                    BlazeRpcError result = mComponent->notifyUserJoined(
                        mRequest.getClubId(), 
                        requestorId);

                    if (result != Blaze::ERR_OK)
                    {
                        WARN_LOG("[SendPetitionCommand] failed to send notifications after user " << requestorId 
                            << " joined the club " << mRequest.getClubId()
                            << " because custom code notifyUserJoined returned error code 0x" << SbFormats::HexLower(result) );
                    }

                    mComponent->mPerfJoinsLeaves.increment();

                    return ERR_OK;
                }
                else
                {
                    mComponent->removeMemberUserExtendedData(requestorId, mRequest.getClubId(), UPDATE_REASON_USER_LEFT_CLUB);
                    dbc.completeTransaction(false);
                    if (error == Blaze::CLUBS_ERR_PASSWORD_REQUIRED)
                        error = Blaze::CLUBS_ERR_WRONG_PASSWORD;
                    // if password is wrong, the requester may try next time, do not send petition message.
                    if (error == Blaze::CLUBS_ERR_WRONG_PASSWORD )
                        return commandErrorFromBlazeError(error);
                }
            }
            
            if (!dbc.isTransactionStart())
            {
                if (!dbc.startTransaction())
                    return ERR_SYSTEM;
            }

            // update database
            ClubMessage message;
            message.setClubId(mRequest.getClubId());
            message.setMessageType(CLUBS_PETITION);
            message.getSender().setBlazeId(requestorId);

            const ClubDomain *domain = nullptr;

            error = mComponent->getClubDomainForClub(mRequest.getClubId(), dbc.getDb(), domain);

            if (error != Blaze::ERR_OK)
            {
                dbc.completeTransaction(false);
                return commandErrorFromBlazeError(error);
            }

            // insert the petition to club_messages table and also updates the last active time for the club
            error = dbc.getDb().insertClubMessage(message, domain->getClubDomainId(), domain->getMaxInvitationsPerUserOrClub());

            if (error != (BlazeRpcError)ERR_OK)
            {
                if (error == Blaze::CLUBS_ERR_MAX_MESSAGES_SENT)
                {
                    dbc.completeTransaction(false); 
                    return CLUBS_ERR_MAX_PETITIONS_SENT;
                }
                else if (error == Blaze::CLUBS_ERR_MAX_MESSAGES_RECEIVED)
                {
                    dbc.completeTransaction(false); 
                    return CLUBS_ERR_MAX_PETITIONS_RECEIVED;
                }
                else
                {
                    ERR_LOG("[SendPetitionCommand] failed to send petition from user [blazeid]: " 
                        << requestorId << " error was " << ErrorHelp::getErrorName(error));
                    dbc.completeTransaction(false);
                    return commandErrorFromBlazeError(error);
                }            
            }

            if (!dbc.completeTransaction(true))
            {
                ERR_LOG("[SendPetitionCommand] could not commit transaction");
                return ERR_SYSTEM;
            }

            BlazeRpcError result = mComponent->notifyPetitionSent(
                mRequest.getClubId(),
                clubToPetition.getName(),
                message.getMessageId(),
                requestorId);

            if (result != Blaze::ERR_OK)
            {
                WARN_LOG("[SendPetitionCommand] failed to send notifications after user " << requestorId << " sent petition"
                    " to join club " << mRequest.getClubId() << " because custom code notifyPetitionSent returned error code " << SbFormats::HexLower(result) );
            }

            PetitionSent petitionSentEvent;
            petitionSentEvent.setPetitionerPersonaId(requestorId);
            petitionSentEvent.setClubId(mRequest.getClubId());
            gEventManager->submitEvent(static_cast<uint32_t>(ClubsSlave::EVENT_PETITIONSENTEVENT), petitionSentEvent, true /*logBinaryEvent*/);

            mComponent->mPerfMessages.increment();
            
            return ERR_OK;
        }
    };

    // static factory method impl

    DEFINE_SENDPETITION_CREATE()

} // Clubs
} // Blaze
