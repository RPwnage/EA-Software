/*************************************************************************************************/
/*!
    \file   acceptpetition_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class AcceptPetitionCommand

    takes a response to the petition and update the table

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/event/eventmanager.h"
#include "framework/controller/controller.h"

// clubs includes
#include "clubs/rpc/clubsslave/acceptpetition_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

// messaging includes
#include "messaging/messagingslaveimpl.h"
#include "messaging/tdf/messagingtypes.h"

namespace Blaze
{
namespace Clubs
{

class AcceptPetitionCommand : public AcceptPetitionCommandStub, private ClubsCommandBase
{
public:
    AcceptPetitionCommand(Message* message, ProcessPetitionRequest* request, ClubsSlaveImpl* componentImpl)
        : AcceptPetitionCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~AcceptPetitionCommand() override
    {
    }

        /* Private methods *******************************************************************************/
private:

    AcceptPetitionCommandStub::Errors execute() override
    {
        TRACE_LOG("[AcceptPetitionCommand] start");

        BlazeId rtorBlazeId = gCurrentUserSession->getBlazeId();

        ClubsDbConnector dbc(mComponent); 
        if (dbc.acquireDbConnection(false) == nullptr)
        {
            return ERR_SYSTEM;
        }

        BlazeRpcError error;
        ClubMessage messageToAccept;
        Club joinClub;
        uint64_t version = 0;

        error = dbc.getDb().getClubMessage(mRequest.getPetitionId(), messageToAccept);

        // Ensure the requested operation is on a valid message
        if (error != Blaze::ERR_OK)
        {
            dbc.completeTransaction(false);
            return CLUBS_ERR_INVALID_ARGUMENT;
        }

        // !!! Start transaction here because if we start it before
        // getting club message we will not see potential changes to
        // club_members table commited by some other parallel transaction
        // due to isolation level

        if (!dbc.startTransaction())
            return ERR_SYSTEM;

        const ClubId joinClubId = messageToAccept.getClubId();
        const BlazeId joinBlazeId = messageToAccept.getSender().getBlazeId();

        error = dbc.getDb().lockUserMessages(joinBlazeId);
        if (error != Blaze::ERR_OK)
        {
            dbc.completeTransaction(false);
            return commandErrorFromBlazeError(error);
        }

        error = dbc.getDb().lockClub(joinClubId, version, &joinClub);
        if (error != Blaze::ERR_OK)
        {
            dbc.completeTransaction(false);
            return commandErrorFromBlazeError(error);
        }

        ClubMembership membership;

        error = dbc.getDb().getMembershipInClub(
            joinClubId,
            rtorBlazeId,
            membership);

        if (error != Blaze::ERR_OK)
        {
            dbc.completeTransaction(false);
            return commandErrorFromBlazeError(error);
        }

        MembershipStatus rtorMshipStatus = membership.getClubMember().getMembershipStatus();

        // To accept a petition, the requester must be the GM of the club
        if (rtorMshipStatus < CLUBS_GM)
        {
            dbc.completeTransaction(false);
            return CLUBS_ERR_NO_PRIVILEGE;
        }

        // Check that the petitioner is not already banned from this club
        BanStatus banStatus = CLUBS_NO_BAN;
        error = dbc.getDb().getBan(joinClubId, joinBlazeId, &banStatus);
        if (error != Blaze::ERR_OK)
        {
            dbc.completeTransaction(false);
            return commandErrorFromBlazeError(error);
        }
        else if (banStatus != CLUBS_NO_BAN)
        {
            dbc.completeTransaction(false);
            return CLUBS_ERR_USER_BANNED;
        }

        // Check that the petitioner is not already a member of this club
        error = dbc.getDb().getMembershipInClub(
            joinClubId,
            joinBlazeId,
            membership);

        if (error == Blaze::ERR_OK)
        {
            // petitioner is already a member so just delete this petition
            dbc.getDb().deleteClubMessage(mRequest.getPetitionId());
            if (!dbc.completeTransaction(true))
            {
                ERR_LOG("[AcceptPetitionCommand] could not commit transaction");
                return ERR_SYSTEM;
            }

            return CLUBS_ERR_ALREADY_MEMBER;
        }
        else if (error != Blaze::CLUBS_ERR_USER_NOT_MEMBER)
        {
            dbc.completeTransaction(false);
            return commandErrorFromBlazeError(error);
        }

        const ClubDomain *domain = nullptr;

        error = mComponent->getClubDomainForClub(joinClubId, dbc.getDb(), domain);

        if (error != Blaze::ERR_OK)
        {
            dbc.completeTransaction(false);
            return commandErrorFromBlazeError(error);
        }

        error = dbc.getDb().lockMemberDomain(joinBlazeId, domain->getClubDomainId());
        if (error != Blaze::ERR_OK)
        {
            dbc.completeTransaction(false);
            return commandErrorFromBlazeError(error);
        }

        ClubMemberStatusList statusList;
        error = mComponent->getMemberStatusInDomain(
            domain->getClubDomainId(), 
            joinBlazeId, 
            dbc.getDb(), 
            statusList,
            true);

        if (static_cast<uint16_t>(statusList.size()) >= domain->getMaxMembershipsPerUser())
        {
            // user is already member of maximum number of clubs in this domain
            dbc.completeTransaction(false);
            return CLUBS_ERR_MAX_CLUBS;
        }

        if(joinClub.getClubInfo().getMemberCount() == domain->getMaxMembersPerClub())
        {
            // This club is full
            dbc.completeTransaction(false); 
            return CLUBS_ERR_CLUB_FULL;
        }

        ClubMember joinUser;
        joinUser.getUser().setBlazeId(joinBlazeId);
        joinUser.setMembershipStatus(CLUBS_MEMBER);
        error = dbc.getDb().insertMember(joinClubId, joinUser, 
            domain->getMaxMembersPerClub());

        if (error != Blaze::ERR_OK)
        {
            if (error != Blaze::CLUBS_ERR_CLUB_FULL)
            {
                // Log unexpected error
                ERR_LOG("[AcceptPetitionCommand] error (" << ErrorHelp::getErrorName(error) << ") user " << rtorBlazeId 
                    << " failed to accept petition " << mRequest.getPetitionId());

            }
            dbc.completeTransaction(false); 
            return commandErrorFromBlazeError(error);
        }

        error = dbc.getDb().updateClubLastActive(joinClubId);

        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("[AcceptPetitionCommand] failed to update last active time for clubid " << joinClubId);
            dbc.completeTransaction(false); 
            return commandErrorFromBlazeError(error);
        }

        error = dbc.getDb().deleteClubMessage(mRequest.getPetitionId());

        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("[AcceptPetitionCommand] error (" << ErrorHelp::getErrorName(error) << ") user " << rtorBlazeId
                << " failed to delete petition " << mRequest.getPetitionId());
            dbc.completeTransaction(false); 
            return commandErrorFromBlazeError(error);
        }

        error = mComponent->onMemberAdded(dbc.getDb(), joinClub, joinUser);
        if (error != Blaze::ERR_OK)
        {
            dbc.completeTransaction(false); 
            return commandErrorFromBlazeError(error);
        }

        if (gUserSessionManager->isUserOnline(joinUser.getUser().getBlazeId()))
        {
            // Cache this users club member info
            error = mComponent->updateCachedDataOnNewUserAdded(
                joinClubId,
                joinClub.getName(),
                joinClub.getClubDomainId(),
                joinClub.getClubSettings(),
                joinUser.getUser().getBlazeId(),
                joinUser.getMembershipStatus(),
                joinUser.getMembershipSinceTime(),
                UPDATE_REASON_USER_JOINED_CLUB);

            if (error != Blaze::ERR_OK)
            {
                ERR_LOG("[AcceptPetitionCommand] updateCachedDataOnNewUserAdded() failed");
                dbc.completeTransaction(false); 
                return commandErrorFromBlazeError(error);
            }
            error = mComponent->updateMemberUserExtendedData(joinUser.getUser().getBlazeId(), joinClubId, 
                joinClub.getClubDomainId(), UPDATE_REASON_USER_JOINED_CLUB,
                mComponent->getOnlineStatus(joinUser.getUser().getBlazeId()));
            if (error != Blaze::ERR_OK)
            {
                dbc.completeTransaction(false); 
                return commandErrorFromBlazeError(error);
            }
        }

        if (!dbc.completeTransaction(true))
        {
            if (gUserSessionManager->isUserOnline(joinUser.getUser().getBlazeId()))
                mComponent->removeMemberUserExtendedData(joinUser.getUser().getBlazeId(), joinClubId, UPDATE_REASON_USER_LEFT_CLUB);

            ERR_LOG("[AcceptPetitionCommand] could not commit transaction");
            return ERR_SYSTEM;
        }

        error = mComponent->notifyPetitionAccepted(
            joinClubId,
            joinClub.getName(),
            mRequest.getPetitionId(),
            rtorBlazeId,
            joinUser.getUser().getBlazeId());

        if (error != Blaze::ERR_OK)
        {
            WARN_LOG("[AcceptPetitionCommand] failed to send notifications for petition from user " << joinUser.getUser().getBlazeId()
                << " to join club " << joinClubId << " because custom code notifyPetitionAccepted returned error code " << SbFormats::HexLower(error) );
        }

        UserJoinedClub userJoinedClubEvent;
        userJoinedClubEvent.setJoiningPersonaId(joinUser.getUser().getBlazeId());
        userJoinedClubEvent.setClubId(joinClubId);
        gEventManager->submitEvent(static_cast<uint32_t>(ClubsSlave::EVENT_USERJOINEDCLUBEVENT), userJoinedClubEvent, true /*logBinaryEvent*/);

        mComponent->mPerfJoinsLeaves.increment();

        return ERR_OK;
    }
};

// static factory method impl
DEFINE_ACCEPTPETITION_CREATE()

} // Clubs
} // Blaze
