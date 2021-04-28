/*************************************************************************************************/
/*!
    \file   promotetogm_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class PromoteToGMCommand

    promote someone to be a gm. must be a gm to do this.

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
#include "clubs/rpc/clubsslave/promotetogm_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

    class PromoteToGMCommand : public PromoteToGMCommandStub, private ClubsCommandBase
    {
    public:
        PromoteToGMCommand(Message* message, PromoteToGMRequest* request, ClubsSlaveImpl* componentImpl)
            : PromoteToGMCommandStub(message, request),
            ClubsCommandBase(componentImpl)
        {
        }

        ~PromoteToGMCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        PromoteToGMCommandStub::Errors execute() override
        {
            TRACE_LOG("[PromoteToGMCommand] start");

            BlazeRpcError error = Blaze::ERR_SYSTEM;
            const BlazeId requestorId = gCurrentUserSession->getBlazeId();
            const BlazeId promoteBlazeId = mRequest.getBlazeId();
            const ClubId promoteClubId = mRequest.getClubId();
            Club clubToPromote;

            ClubsDbConnector dbc(mComponent);

            if (!dbc.startTransaction())
                return ERR_SYSTEM;

            uint64_t version = 0;
            error = dbc.getDb().lockClub(promoteClubId, version);
            if (error != Blaze::ERR_OK)
            {
                dbc.completeTransaction(false);
                return commandErrorFromBlazeError(error);
            }

            // Only Clubs Admin or this club's GM can promote another member to GM.
            bool rtorIsAdmin = false;    
            bool rtorIsThisClubGM = false;

            ClubMembership membership;

            error = dbc.getDb().getMembershipInClub(
                promoteClubId,
                requestorId,
                membership);

            if (error == Blaze::ERR_OK)
            {
                MembershipStatus rtorMshipStatus = membership.getClubMember().getMembershipStatus();
                rtorIsThisClubGM = (rtorMshipStatus >= CLUBS_GM);
            }

            if (!rtorIsThisClubGM)
            {
                rtorIsAdmin = UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_ADMINISTRATOR, true);

                if (!rtorIsAdmin)
                {
                    TRACE_LOG("[PromoteToGMCommand] user [" << requestorId << "] is not admin for club ["
                        << promoteClubId << "] and cannot promote user [" << promoteBlazeId << "] to gm");
                    dbc.completeTransaction(false);
                    if (error == Blaze::ERR_OK)
                    {
                        return CLUBS_ERR_NO_PRIVILEGE;
                    }
                    return commandErrorFromBlazeError(error);
                }
            }

            // Get the member being promoted
            ClubMembership promotmembership;

            error = dbc.getDb().getMembershipInClub(
                promoteClubId,
                promoteBlazeId,
                promotmembership);

            MembershipStatus promoteMshipStatus = promotmembership.getClubMember().getMembershipStatus();

            if (error != Blaze::ERR_OK)
            {
                dbc.completeTransaction(false);
                return commandErrorFromBlazeError(error);
            }

            if (promoteMshipStatus >= CLUBS_GM)
            {
                dbc.completeTransaction(false);
                return CLUBS_ERR_ALREADY_GM;
            }
            
            const ClubDomain *clubDomain = nullptr;
            
            error = mComponent->getClubDomainForClub(promoteClubId, dbc.getDb(), clubDomain);

            if (error != (BlazeRpcError)ERR_OK)
            {
                dbc.completeTransaction(false);
                return commandErrorFromBlazeError(error);
            }
            
            error = dbc.getDb().promoteClubGM(promoteBlazeId, promoteClubId,
                clubDomain->getMaxGMsPerClub());
                
            if (error != Blaze::ERR_OK)
            {
                if (error != Blaze::CLUBS_ERR_TOO_MANY_GMS)
                {
                    // log unexpected database error
                    ERR_LOG("[PromoteToGMCommand] failed to get club membership for user [blazeid]: " << promoteBlazeId 
                            << " error was " << ErrorHelp::getErrorName(error));
                }
                dbc.completeTransaction(false); 
                return commandErrorFromBlazeError(error);
            }

            // update membership information
            mComponent->invalidateCachedMemberInfo(promoteBlazeId, promoteClubId);

            if (!dbc.completeTransaction(true))
            {
                ERR_LOG("[PromoteToGMCommand] could not commit transaction");
                return ERR_SYSTEM;
            }

            mComponent->mPerfAdminActions.increment();

            // send notification about promotion
            error = mComponent->notifyMemberPromoted(
                promoteClubId,
                requestorId,
                promoteBlazeId);
           
            if (error != Blaze::ERR_OK)
            {
                WARN_LOG("[PromoteToGMCommand] could not send notifications after promoting member " << promoteBlazeId
                    << " from club " << promoteClubId << " because custom processing code notifyMemberPromoted returned error: " << SbFormats::HexLower(error) << ".");
            }

            PlayerPromotedToGM playerPromotedEvent;
            playerPromotedEvent.setPromotedPersonaId(promoteBlazeId);
            playerPromotedEvent.setPromotingPersonaId(requestorId);
            playerPromotedEvent.setClubId(promoteClubId);
            gEventManager->submitEvent(static_cast<uint32_t>(ClubsSlave::EVENT_PLAYERPROMOTEDTOGMEVENT), playerPromotedEvent, true /*logBinaryEvent*/);

            TRACE_LOG("[PromoteToGMCommand].execute: User [" << requestorId << "] promoted user [" << promoteBlazeId << "] to gm in the club [" 
                << promoteClubId << "], had permission.");

            return ERR_OK;
        }
    };

    // static factory method impl
    DEFINE_PROMOTETOGM_CREATE()

} // Clubs
} // Blaze
