/*************************************************************************************************/
/*!
    \file   joinclub_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class JoinOrPetitionClubCommand

    Try to Join a club, or send petition to this club if join fails

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/event/eventmanager.h"


// clubs includes
#include "clubs/rpc/clubsslave/joinorpetitionclub_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

    class JoinOrPetitionClubCommand : public JoinOrPetitionClubCommandStub, private ClubsCommandBase
    {
    public:
        JoinOrPetitionClubCommand(Message* message, JoinOrPetitionClubRequest* request, ClubsSlaveImpl* componentImpl)
            : JoinOrPetitionClubCommandStub(message, request),
            ClubsCommandBase(componentImpl)
        {
        }

        ~JoinOrPetitionClubCommand() override
        {
        }

    
        /* Private methods *******************************************************************************/
    private:

        JoinOrPetitionClubCommandStub::Errors execute() override
        {
            TRACE_LOG("[JoinOrPetitionClubCommand] start");

            mResponse.setClubJoinOrPetitionStatus(CLUB_JOIN_AND_PETITION_FAILURE);
            BlazeRpcError error = Blaze::ERR_OK;
            const BlazeId rtorBlazeId = gCurrentUserSession->getBlazeId();
            const ClubId clubId = mRequest.getClubId();
            const ClubPassword password = mRequest.getPassword();
            const bool petitionIfJoinFails = mRequest.getPetitionIfJoinFails();
            uint32_t messageId = 0;
            Club club;

            ClubsDbConnector dbc(mComponent);
            if (!dbc.startTransaction())
                return ERR_SYSTEM;

            const ClubDomain *domain = nullptr;
            error = mComponent->getClubDomainForClub(clubId, dbc.getDb(), domain);
            if (error != Blaze::ERR_OK)
            {
                dbc.completeTransaction(false); 
                return commandErrorFromBlazeError(error);
            }

            int64_t configInterval = domain->getClubJumpingInterval().getSec();
            if (domain->getMaxMembershipsPerUser() == 1 && configInterval > 0)
            {
                // Check that user's jumping interval is not shorter than config interval.
                int64_t lastLeaveTime = 0;
                error = dbc.getDb().getUserLastLeaveClubTime(rtorBlazeId, domain->getClubDomainId(), lastLeaveTime);
                if (error != Blaze::ERR_OK)
                {
                    dbc.completeTransaction(false); 
                    return commandErrorFromBlazeError(error);
                }
                
                if (lastLeaveTime != 0)
                {
                    if (TimeValue::getTimeOfDay().getSec() - lastLeaveTime < configInterval)
                    {
                        dbc.completeTransaction(false); 
                        return CLUBS_ERR_JUMP_TOO_FREQUENTLY;
                    }
                }
            }

            // Check whether requestor has been invited by this club.
            ClubMessageList invitationList;
            error = dbc.getDb().getClubMessagesByUserReceived(rtorBlazeId, CLUBS_INVITATION,
                CLUBS_SORT_TIME_DESC, invitationList);
            if (error != Blaze::ERR_OK)
            {
                dbc.completeTransaction(false); 
                return commandErrorFromBlazeError(error);
            }
            const ClubMessage* invitationMsg = nullptr;
            for(ClubMessageList::const_iterator msgIt = invitationList.begin(); msgIt != invitationList.end(); ++msgIt)
            {
                if ((*msgIt)->getClubId() == clubId)
                {
                    invitationMsg = *msgIt;
                    break;
                }
            }
            if (invitationMsg != nullptr)
            { 
                // If requestor has been invited by this club, joins without password check.
                error = dbc.getDb().lockUserMessages(rtorBlazeId);
                if (error != Blaze::ERR_OK)
                {
                    dbc.completeTransaction(false);
                    return commandErrorFromBlazeError(error);
                }

                error = mComponent->joinClub(clubId, true/*skipJoinAcceptanceCheck*/, false/*petitionIfJoinFails*/, true/*skipPasswordCheck*/, password,
                    rtorBlazeId, false/*isGMFriend*/, invitationMsg->getMessageId(), dbc, messageId);
                if (error != Blaze::ERR_OK)
                {
                    ERR_LOG("[JoinOrPetitionClubCommand] error (" << ErrorHelp::getErrorName(error) << ") user " << rtorBlazeId
                        << " failed to join club " << clubId);
                    dbc.completeTransaction(false); 
                    return commandErrorFromBlazeError(error);
                }
                if (!dbc.completeTransaction(true))
                {
                    ERR_LOG("[JoinOrPetitionClubCommand] could not commit transaction");
                    mComponent->removeMemberUserExtendedData(rtorBlazeId, clubId, UPDATE_REASON_USER_LEFT_CLUB);
                    return ERR_SYSTEM;
                }

                mResponse.setClubJoinOrPetitionStatus(CLUB_JOIN_SUCCESS);
            }
            else
            {
                // Check whether requestor is friend of a GM of this club.
                bool isGMFriend = mComponent->isClubGMFriend(clubId, rtorBlazeId, dbc);

                uint64_t version = 0;
                error = dbc.getDb().getClub(clubId, &club, version);
                if (error != Blaze::ERR_OK)
                {
                    dbc.completeTransaction(false); 
                    return commandErrorFromBlazeError(error);
                }

                if (mComponent->isClubJoinAccepted(club.getClubSettings(), isGMFriend))
                {
                    // If this club accepts join, try to join with password check.
                    error = mComponent->joinClub(clubId, true/*skipJoinAcceptanceCheck*/, petitionIfJoinFails, false/*skipJoinAcceptanceCheck*/, password,
                        rtorBlazeId, isGMFriend, 0/*invitationId*/, dbc, messageId);
                    if (error != Blaze::ERR_OK)
                    {
                        ERR_LOG("[JoinOrPetitionClubCommand] error (" << ErrorHelp::getErrorName(error) << ") user " << rtorBlazeId
                            << " failed to join club " << clubId);
                        dbc.completeTransaction(false); 
                        return commandErrorFromBlazeError(error);
                    }
                    if (!dbc.completeTransaction(true))
                    {
                        ERR_LOG("[JoinOrPetitionClubCommand] could not commit transaction");
                        if (messageId == 0)
                            mComponent->removeMemberUserExtendedData(rtorBlazeId, clubId, UPDATE_REASON_USER_LEFT_CLUB);
                        return ERR_SYSTEM;
                    }
                    if (messageId == 0)
                        mResponse.setClubJoinOrPetitionStatus(CLUB_JOIN_SUCCESS);
                    else  // Perhaps, requestor sends petition if join fails
                        mResponse.setClubJoinOrPetitionStatus(CLUB_PETITION_SUCCESS);
                }
                else if (mComponent->isClubPetitionAcctepted(club.getClubSettings(), isGMFriend) && petitionIfJoinFails)
                {
                    // Else if this club accepts petition, send petition.
                    const ClubDomain *domainTemp = nullptr;
                    error = mComponent->getClubDomainForClub(mRequest.getClubId(), dbc.getDb(), domainTemp);
                    if (error != Blaze::ERR_OK)
                    {
                        dbc.completeTransaction(false);
                        return commandErrorFromBlazeError(error);
                    }
                    error = mComponent->petitionClub(clubId, domainTemp, rtorBlazeId, false/*skipMembershipCheck*/, dbc, messageId);
                    if (error != Blaze::ERR_OK)
                    {
                        dbc.completeTransaction(false);
                        return commandErrorFromBlazeError(error);
                    }
                    if (!dbc.completeTransaction(true))
                    {
                        ERR_LOG("[JoinOrPetitionClubCommand] could not commit transaction");
                        return ERR_SYSTEM;
                    }
                    mResponse.setClubJoinOrPetitionStatus(CLUB_PETITION_SUCCESS);
                }
                else
                {
                    dbc.completeTransaction(false);
                    return CLUBS_ERR_JOIN_AND_PETITION_NOT_ALLOWED;
                }
            }

            if (mResponse.getClubJoinOrPetitionStatus() == CLUB_JOIN_SUCCESS)
            {
                error = mComponent->notifyUserJoined(
                    clubId, 
                    rtorBlazeId);
                if (error != Blaze::ERR_OK)
                {
                    WARN_LOG("[JoinOrPetitionClubCommand] failed to send notifications after user " << rtorBlazeId << " joined the club " << clubId 
                        << " because custom code notifyUserJoined returned error code " << SbFormats::HexLower(error));
                }
                mComponent->mPerfJoinsLeaves.increment();

                UserJoinedClub userJoinedClubEvent;
                userJoinedClubEvent.setJoiningPersonaId(rtorBlazeId);
                userJoinedClubEvent.setClubId(clubId);
                gEventManager->submitEvent(static_cast<uint32_t>(ClubsSlave::EVENT_USERJOINEDCLUBEVENT), userJoinedClubEvent, true /*logBinaryEvent*/);
            }
            else if (mResponse.getClubJoinOrPetitionStatus() == CLUB_PETITION_SUCCESS)
            {
                error = mComponent->notifyPetitionSent(
                    clubId,
                    club.getName(),
                    messageId,
                    rtorBlazeId);
                if (error != Blaze::ERR_OK)
                {
                    WARN_LOG("[JoinOrPetitionClubCommand] failed to send notifications after user " << rtorBlazeId << " sent petition"
                        " to join club " << clubId << " because custom code notifyPetitionSent returned error code " << SbFormats::HexLower(error) );
                }
                mComponent->mPerfMessages.increment();

                PetitionSent petitionSentEvent;
                petitionSentEvent.setPetitionerPersonaId(rtorBlazeId);
                petitionSentEvent.setClubId(clubId);
                gEventManager->submitEvent(static_cast<uint32_t>(ClubsSlave::EVENT_PETITIONSENTEVENT), petitionSentEvent, true /*logBinaryEvent*/);
            }

            return ERR_OK;
        }
    };

    // static factory method impl

    DEFINE_JOINORPETITIONCLUB_CREATE()

} // Clubs
} // Blaze
