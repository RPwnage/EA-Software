/*************************************************************************************************/
/*!
    \file   demotetomember_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class DemoteToMemberCommand

    demote someone to be a member. must be a gm or administrator to do this.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/event/eventmanager.h"
#include "framework/usersessions/usersession.h"

// clubs includes
#include "clubs/rpc/clubsslave/demotetomember_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

class DemoteToMemberCommand : public DemoteToMemberCommandStub, private ClubsCommandBase
{
public:
    DemoteToMemberCommand(Message* message, DemoteToMemberRequest* request, ClubsSlaveImpl* componentImpl)
        : DemoteToMemberCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~DemoteToMemberCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    DemoteToMemberCommandStub::Errors execute() override
    {
        TRACE_LOG("[DemoteToMemberCommand] start");

        BlazeRpcError error = Blaze::ERR_SYSTEM;
        const BlazeId requestorId = gCurrentUserSession->getBlazeId();
        const BlazeId demoteBlazeId = mRequest.getBlazeId();
        const ClubId demoteClubId = mRequest.getClubId();
        Club clubToDemote;

        ClubsDbConnector dbc(mComponent);

        if (!dbc.startTransaction())
            return ERR_SYSTEM;

        uint64_t version = 0;
        error = dbc.getDb().lockClub(demoteClubId, version);
        if (error != Blaze::ERR_OK)
        {
            dbc.completeTransaction(false);
            return commandErrorFromBlazeError(error);
        }

        // Only Clubs Admin or this club's GM can demote another GM to member.
        bool rtorIsAdmin = false;
        bool rtorIsThisClubGM = false;

        MembershipStatus rtorMshipStatus;
        ClubMembership membership;

        error = dbc.getDb().getMembershipInClub(
            demoteClubId,
            requestorId,
            membership);

        if (error == Blaze::ERR_OK)
        {
            rtorMshipStatus = membership.getClubMember().getMembershipStatus();
            rtorIsThisClubGM = (rtorMshipStatus >= CLUBS_GM);
        }

        if (!rtorIsThisClubGM)
        {
            rtorIsAdmin = UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_ADMINISTRATOR, true);

            if (!rtorIsAdmin)
            {
                TRACE_LOG("[DemoteToMemberCommand] user [" << requestorId << "] is not admin for club ["
                    << demoteClubId << "] and cannot demote user [" << demoteBlazeId << "] to member");
                dbc.completeTransaction(false);
                if (error == Blaze::ERR_OK)
                {
                    return CLUBS_ERR_NO_PRIVILEGE;
                }
                return commandErrorFromBlazeError(error);
            }
        }

        // Get the member being demoted
        ClubMembership domotemembership;

        error = dbc.getDb().getMembershipInClub(
            demoteClubId,
            demoteBlazeId,
            domotemembership);

        MembershipStatus demoteMshipStatus = domotemembership.getClubMember().getMembershipStatus();

        if (error != Blaze::ERR_OK)
        {
            dbc.completeTransaction(false);
            return commandErrorFromBlazeError(error);
        }

        if (demoteMshipStatus == CLUBS_MEMBER)
        {
            dbc.completeTransaction(false);
            return CLUBS_ERR_DEMOTE_MEMBER;
        }

        if (demoteMshipStatus == CLUBS_OWNER)
        {
            dbc.completeTransaction(false);
            return CLUBS_ERR_DEMOTE_OWNER;
        }

        error = dbc.getDb().getClub(demoteClubId, &clubToDemote, version);
        if (error != Blaze::ERR_OK)
        {
            dbc.completeTransaction(false);
            return commandErrorFromBlazeError(error);
        }
        else
        {
            // Can't demote last GM
            if (clubToDemote.getClubInfo().getGmCount() == 1)
            {
                dbc.completeTransaction(false);
                return CLUBS_ERR_DEMOTE_LAST_GM;
            }
        }

        error = dbc.getDb().demoteClubMember(demoteBlazeId, demoteClubId);

        if (error != Blaze::ERR_OK)
        {
            if (error != Blaze::CLUBS_ERR_USER_NOT_MEMBER)
            {
                // log unexpected database error
                ERR_LOG("[DemoteToMemberCommand] failed to demote club membership for user [blazeid]: " << demoteBlazeId 
                    << " error was " << ErrorHelp::getErrorName(error));
            }
            else
            {
                if (!mComponent->checkClubId(dbc.getDbConn(), demoteClubId))
                    error = Blaze::CLUBS_ERR_INVALID_CLUB_ID;
            }

            dbc.completeTransaction(false); 
            return commandErrorFromBlazeError(error);
        }

        // update membership information
        mComponent->invalidateCachedMemberInfo(demoteBlazeId, demoteClubId);

        if (!dbc.completeTransaction(true))
        {
            ERR_LOG("[DemoteToMemberCommand] could not commit transaction");
            return ERR_SYSTEM;
        }

        mComponent->mPerfAdminActions.increment();

        // send notification about demotion
        error = mComponent->notifyGmDemoted(
            demoteClubId,
            requestorId,
            demoteBlazeId);

        if (error != Blaze::ERR_OK)
        {
            WARN_LOG("[DemoteToMemberCommand] could not send notifications after demoting GM " << demoteBlazeId 
                << " from club " << demoteClubId << " because custom processing code notifyGmDemoted returned error: " 
                << SbFormats::HexLower(error));
        }

        GMDemotedtoMember playerDemotedEvent;
        playerDemotedEvent.setDemotedPersonaId(demoteBlazeId);
        playerDemotedEvent.setDemotingPersonaId(requestorId);
        playerDemotedEvent.setClubId(demoteClubId);
        gEventManager->submitEvent(static_cast<uint32_t>(ClubsSlave::EVENT_GMDEMOTEDTOMEMBEREVENT), playerDemotedEvent, true /*logBinaryEvent*/);

        TRACE_LOG("[DemoteToMemberCommand].execute: User [" << requestorId << "] demoted user [" << demoteBlazeId << "] to member in the club [" 
            << demoteClubId << "], had permission.");

        return ERR_OK;
    }

};
// static factory method impl
DEFINE_DEMOTETOMEMBER_CREATE()

} // Clubs
} // Blaze
