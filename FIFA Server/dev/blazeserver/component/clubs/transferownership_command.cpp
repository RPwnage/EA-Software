/*************************************************************************************************/
/*!
    \file   createclub_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class CreateClubCommand

    Create a club in the Database.

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
#include "framework/util/profanityfilter.h"

// clubs includes
#include "clubs/rpc/clubsslave/transferownership_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"
#include "clubsdb.h"

namespace Blaze
{
namespace Clubs
{

    class TransferOwnershipCommand : public TransferOwnershipCommandStub, private ClubsCommandBase
    {
    public:
        TransferOwnershipCommand(Message* message, TransferOwnershipRequest* request, ClubsSlaveImpl* componentImpl)
            : TransferOwnershipCommandStub(message, request),
            ClubsCommandBase(componentImpl),
            mComponentImp(componentImpl)
        {
        }

        ~TransferOwnershipCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        TransferOwnershipCommandStub::Errors execute() override
        {

           TRACE_LOG("[PromoteToGMCommand: ] start");

            BlazeRpcError error = Blaze::ERR_SYSTEM;
            const BlazeId requestorId = gCurrentUserSession->getBlazeId();
            const BlazeId transferredBlazeId = mRequest.getBlazeId();
            const ClubId transferredClubId = mRequest.getClubId();
            const MembershipStatus exOwnersNewStatus = mRequest.getExOwnersNewStatus();

            if (exOwnersNewStatus == CLUBS_OWNER)
                return CLUBS_ERR_TRANSFER_OWNERSHIP_TO_OWNER;

            ClubsDbConnector dbc(mComponent);

            if (!dbc.startTransaction())
                return ERR_SYSTEM;

            uint64_t version = 0;
            error = dbc.getDb().lockClub(transferredClubId, version);
            if (error != Blaze::ERR_OK)
            {
                dbc.completeTransaction(false);
                return commandErrorFromBlazeError(error);
            }

            // Only Clubs Admin or this club's Owner can transfer ownership.
            bool rtorIsAdmin = false;
            bool rtorIsThisClubOwner = false;

            ClubMembership membership;

            error = dbc.getDb().getMembershipInClub(
                transferredClubId,
                requestorId,
                membership);

            if (error == Blaze::ERR_OK)
            {
                MembershipStatus rtorMshipStatus = membership.getClubMember().getMembershipStatus();
                rtorIsThisClubOwner = (rtorMshipStatus == CLUBS_OWNER);
            }

            if (!rtorIsThisClubOwner)
            {
                rtorIsAdmin = UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_ADMINISTRATOR, true);

                if (!rtorIsAdmin)
                {
                    TRACE_LOG("[TransferOwnershipCommand] user [" << requestorId
                        << "] is not admin for club [" << transferredClubId << "] and is not allowed this action");
                    dbc.completeTransaction(false);
                    if (error == Blaze::ERR_OK)
                    {
                        return CLUBS_ERR_NO_PRIVILEGE;
                    }
                    return commandErrorFromBlazeError(error);
                }
            }

            // Get the membership of the member that is being made owner.
            ClubMembership transferredmembership;

            error = dbc.getDb().getMembershipInClub(
                transferredClubId,
                transferredBlazeId,
                transferredmembership);

            if (error != Blaze::ERR_OK)
            {
                dbc.completeTransaction(false);
                return commandErrorFromBlazeError(error);
            }

            MembershipStatus transferredMshipStatus = transferredmembership.getClubMember().getMembershipStatus();

            if (transferredMshipStatus == CLUBS_OWNER)
            {
                dbc.completeTransaction(false);
                return CLUBS_ERR_TRANSFER_OWNERSHIP_TO_OWNER;
            }
            
            const ClubDomain *clubDomain = nullptr;

            error = mComponent->getClubDomainForClub(transferredClubId, dbc.getDb(), clubDomain);

            if (error != (BlazeRpcError)ERR_OK)
            {
                dbc.completeTransaction(false);
                return commandErrorFromBlazeError(error);
            }

            BlazeId owner = 0;

            if (rtorIsThisClubOwner)
            {
                owner = requestorId;
            }
            else
            {
                error = dbc.getDb().getClubOwner(owner, transferredClubId);
                if (error != (BlazeRpcError)ERR_OK)
                {
                    dbc.completeTransaction(false);
                    return commandErrorFromBlazeError(error);
                }
            }

            error = dbc.getDb().transferClubOwnership(owner, 
                transferredBlazeId, 
                transferredClubId,
                transferredMshipStatus,
                exOwnersNewStatus,
                clubDomain->getMaxGMsPerClub());

            if (error != Blaze::ERR_OK)
            {
                if (error != Blaze::CLUBS_ERR_TOO_MANY_GMS)
                {
                    // log unexpected database error
                    ERR_LOG("[TransferOwnershipCommand] failed to get club membership for user [blazeid]: " << transferredBlazeId << " error was " <<  error);
                }
                dbc.completeTransaction(false); 
                return commandErrorFromBlazeError(error);
            }

            // update membership information
            error = mComponent->updateCachedMamberInfoOnTransferOwnership(
                owner, 
                transferredBlazeId,
                exOwnersNewStatus,
                transferredClubId);
                
            if(error != Blaze::ERR_OK)
            {
                WARN_LOG("[TransferOwnershipCommand] updateCachedMamberInfoOnTransferOwnership failed");
                dbc.completeTransaction(false); 
                return commandErrorFromBlazeError(error);
            }

            if (!dbc.completeTransaction(true))
            {
                ERR_LOG("[TransferOwnershipCommand] could not commit transaction");
                return ERR_SYSTEM;
            }

            mComponent->mPerfAdminActions.increment();

            // send notification about promotion
            error = mComponent->notifyOwnershipTransferred(
                transferredClubId,
                requestorId,
                owner,
                transferredBlazeId);
           
            if (error != Blaze::ERR_OK)
            {
                WARN_LOG("[TransferOwnershipCommand] could not send notifications after transfer ownership to member " << transferredBlazeId << 
                    " from club " << transferredClubId << " because custom processing code notifyMemberPromoted returned error: " << ErrorHelp::getErrorName(error) << ".");
            }

            ClubOwnershipTransferred clubOwnershipTransferredEvent;
            clubOwnershipTransferredEvent.setOldOwnerPersonaId(owner);
            clubOwnershipTransferredEvent.setNewOwnerPersonaId(transferredBlazeId);
            clubOwnershipTransferredEvent.setTransferrerPersonaId(requestorId);
            clubOwnershipTransferredEvent.setClubId(transferredClubId);
            gEventManager->submitEvent(static_cast<uint32_t>(ClubsSlave::EVENT_CLUBOWNERSHIPTRANSFERREDEVENT), clubOwnershipTransferredEvent, true /*logBinaryEvent*/);

            TRACE_LOG("[TransferOwnershipCommand].execute: User [" << requestorId << "] transferred ownership in the club [" 
                << transferredClubId << "], had permission.");

            return commandErrorFromBlazeError(error);            
        }
        
        ClubsSlaveImpl *mComponentImp;
    };

    // static factory method impl
    DEFINE_TRANSFEROWNERSHIP_CREATE()

    
} // Clubs
} // Blaze
