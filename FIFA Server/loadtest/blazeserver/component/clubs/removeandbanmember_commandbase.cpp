/*************************************************************************************************/
/*!
    \file   removeandbanmember_commandbase.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/event/eventmanager.h"
#include "framework/usersessions/usersession.h"
#include "stats/statsslaveimpl.h"
#include "framework/controller/controller.h"

// clubs includes
#include "removeandbanmember_commandbase.h"

namespace Blaze
{
namespace Clubs
{

    RemoveAndBanMemberCommandBase::RemoveAndBanMemberCommandBase(ClubsSlaveImpl* componentImpl, 
                                                                 const char8_t*  mCmdName)
    :   ClubsCommandBase(componentImpl),
        mCmdName(mCmdName)
    {
    }


    BlazeRpcError 
    RemoveAndBanMemberCommandBase::removeMember(ClubId              toremClubId, 
                                                BlazeId             toremBlazeId, 
                                                ClubsDbConnector*   dbc)
    {
        bool bRemoved = false;
        const BlazeId rtorBlazeId = gCurrentUserSession->getBlazeId();

        BlazeRpcError error = Blaze::ERR_SYSTEM;

        // Figure out authorization level.
        bool rtorIsAdmin = false;
        bool rtorIsThisClubGM = false;

        ClubMembership membership;

        error = dbc->getDb().getMembershipInClub(
            toremClubId,
            rtorBlazeId,
            membership);

        MembershipStatus rtorMshipStatus = membership.getClubMember().getMembershipStatus();

        switch (error)
        {
        case Blaze::ERR_OK:
            rtorIsThisClubGM = (rtorMshipStatus >= CLUBS_GM);
            break;

        case Blaze::CLUBS_ERR_USER_NOT_MEMBER:
            // We should yet fail here, because we need to differentiate between
            // self-removal (in which case CLUBS_ERR_USER_NOT_MEMBER should be returned as
            // is) and removal of a member by a non-GM non-club-member user (in which case
            // the correct error is CLUBS_ERR_NO_PRIVILEGE. This is handled below.
            rtorIsThisClubGM = false;
            break;

        default:

            return error;
        }
        
        if (!rtorIsThisClubGM)
        {
            rtorIsAdmin = UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_ADMINISTRATOR, true);

            if (!rtorIsAdmin && error != Blaze::ERR_OK && error != Blaze::CLUBS_ERR_USER_NOT_MEMBER)
            {
                return error;
            }
        }

        // Get the member to remove and check that they:
        // a) actually belong to the club;
        // b) are not the owner of the club.
        ClubMembership toremMemberShip;

        error = dbc->getDb().getMembershipInClub(
            toremClubId,
            toremBlazeId,
            toremMemberShip);

        if (error != Blaze::ERR_OK)
        {
            return error;
        }

        MembershipStatus toremMshipStatus = toremMemberShip.getClubMember().getMembershipStatus();
        
        bool bClubDelete = false;

        // If trying to remove someone other than yourself, then must be Clubs
        // Admin or this club's GM.
        if (rtorBlazeId != toremBlazeId)
        {
            // The requestor is removing another user: the requestor must be
            // either Clubs Admin or this club's GM.
            TRACE_LOG("[" << mCmdName << "] User " << rtorBlazeId << " is trying to remove user " << toremBlazeId 
                      << " from club " << toremClubId);

            if (!rtorIsAdmin && !rtorIsThisClubGM)
            {
                TRACE_LOG("[RemoveAndBanMemberCommandBase].removeMember: User [" << rtorBlazeId
                    << "] is not admin for club [" << toremClubId << "] and cannot remove user [" << toremBlazeId << "]");
                return CLUBS_ERR_NO_PRIVILEGE;
            }

            // No one except Clubs Admin has the authority to remove an owner of a club 
            // (an owner can remove themselves, but this is the 'else' branch below).
            if (toremMshipStatus == CLUBS_OWNER && !rtorIsAdmin)
            {
                // because we tried to do the cmd as Club GM first, here we need to check if the requestor is admin or not.
                rtorIsAdmin = UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_ADMINISTRATOR, true);
    
                if (!rtorIsAdmin)
                {
                    return CLUBS_ERR_CANNOT_KICK_OWNER;
                }
            }

            bRemoved = true;
        }
        else
        {
            // The requestor is removing themselves: anybody has this privilege
            // (except the last GM, which is handled further below).
            TRACE_LOG("[" << mCmdName << "] User " << rtorBlazeId << " trying to remove themselves from club " << toremClubId);
        }

        // At this point, the requestor is either this club's GM or the Clubs
        // Admin. Check if the member to remove is this club's last GM (possibly
        // he's the requestor himself). If so, cannot delete if there are still
        // other users remaining in the club - another user must be promoted to
        // GM status in order to do that.
        if (toremMshipStatus >= CLUBS_GM)
        {
            Club club;
            uint64_t version = 0;
            error = dbc->getDb().getClub(toremClubId, &club, version);
            if (error != Blaze::ERR_OK)
            {
                return error;
            }
            if (club.getClubInfo().getGmCount() == 1)
            {
                if (club.getClubInfo().getMemberCount() > 1)
                {
                    return CLUBS_ERR_LAST_GM_CANNOT_LEAVE;
                }
                else
                {
                    // Last member to leave - delete club
                    bClubDelete = true;
                }
            }
        }

        // Remove the member and possibly delete the club if no more users are left.
        error = dbc->getDb().removeMember(toremClubId, toremBlazeId, toremMshipStatus);

        if(error != Blaze::ERR_OK)
        {
            if (error != Blaze::CLUBS_ERR_USER_NOT_MEMBER)
            {
                ERR_LOG("[" << mCmdName << "] Database error (" << ErrorHelp::getErrorName(error) << ")");
            }
            else
            {
                WARN_LOG("[" << mCmdName << "] Trying to remove member (" << (int64_t)toremBlazeId << ") from club (" 
                         << toremClubId << ") but member is not found in database.");
            }
            return error;
        }

        if (!bClubDelete && gUserSessionManager->isUserOnline(toremBlazeId))
        {
            // remove member from cache
            mComponent->invalidateCachedMemberInfo(toremBlazeId, toremClubId);
        }

        // Update user extended data
        error = mComponent->removeMemberUserExtendedData(toremBlazeId, toremClubId, UPDATE_REASON_USER_LEFT_CLUB);
        if (error != Blaze::ERR_OK)
        {
            return error;
        }
        
        const ClubDomain *domain = nullptr;
        error = mComponent->getClubDomainForClub(toremClubId, dbc->getDb(), domain);
        if (error != Blaze::ERR_OK)
        {
            return error;
        }
        if (domain->getClubJumpingInterval().getSec() > 0)  // If the domain doesn't have time limit for club jumping, we don't have to log leave time.
        {
            error = dbc->getDb().logUserLastLeaveClubTime(toremBlazeId, domain->getClubDomainId());
            if (error != Blaze::ERR_OK)
            {
                ERR_LOG("[" << mCmdName << "] Failed to upsert user [blazeid] " << toremBlazeId << " leave time from club [clubid] " << toremClubId << " in domain [domainid] " << domain->getClubDomainId() << ". Error was: " << ErrorHelp::getErrorName(error));
                return error;
            }
        }

        if (bClubDelete)
        {
            error = dbc->getDb().removeClub(toremClubId);
            if (error != Blaze::ERR_OK)
            {
                ERR_LOG("[" << mCmdName << "] Failed to delete club [clubid]: " << toremClubId << " Error was: " << ErrorHelp::getErrorName(error));
                return error;
            }
            mComponent->invalidateCachedClubInfo(toremClubId, true);
        }
         
        error = mComponent->onMemberRemoved(&dbc->getDb(), toremClubId,
            bRemoved ? toremBlazeId : rtorBlazeId, !bRemoved, bClubDelete);

        if(error != Blaze::ERR_OK)
        {
            return error;
        }

        mComponent->mPerfAdminActions.increment();

        // Delete stats
        Stats::StatsSlaveImpl* stats = 
            static_cast<Stats::StatsSlaveImpl*>
            (gController->getComponent(Stats::StatsSlave::COMPONENT_ID));    
        if (stats != nullptr && mComponent->isUsingClubKeyscopedStats() && !mComponent->getConfig().getDisableClubKeyscopeStatsWipeOnLeave())
        {
            Stats::DeleteAllStatsByKeyScopeAndEntityRequest delMemberStatsRequest;
            Stats::ScopeName scopeName = "club";
            Stats::ScopeValue scopeValue = static_cast<Stats::ScopeValue>(toremClubId);
            delMemberStatsRequest.getKeyScopeNameValueMap().insert(Stats::ScopeNameValueMap::value_type(scopeName, scopeValue));
            delMemberStatsRequest.setEntityType(ENTITY_TYPE_USER);
            delMemberStatsRequest.setEntityId(toremBlazeId);
            stats->deleteAllStatsByKeyScopeAndEntity(delMemberStatsRequest);
        }

        // Unsubscribe from club ticker messages
        mComponent->setClubTickerSubscription(toremClubId, toremBlazeId, false);
        
        if((stats != nullptr) && 
           (bClubDelete))
        {
            Stats::DeleteAllStatsByEntityRequest delClubStatsRequest;
            delClubStatsRequest.setEntityType(ENTITY_TYPE_CLUB);
            delClubStatsRequest.setEntityId(toremClubId);
            stats->deleteAllStatsByEntity(delClubStatsRequest);
        }

        // Send notification about removal
        error = mComponent->notifyMemberRemoved(
            toremClubId,
            rtorBlazeId,
            toremBlazeId);
       
        if (error != Blaze::ERR_OK)
        {
            WARN_LOG("[" << mCmdName << "] Could not send notifications after removing member " << toremBlazeId 
                << " from club " << toremClubId << " because custom processing code notifyUserRemoved returned error: " << SbFormats::HexLower(error) << ".");
        }

        mComponent->mPerfJoinsLeaves.increment();

        MemberRemovedFromClub memberRemovedFromClubEvent;
        memberRemovedFromClubEvent.setRemovedPersonaId(toremBlazeId);
        memberRemovedFromClubEvent.setRemoverPersonaId(rtorBlazeId);
        memberRemovedFromClubEvent.setClubId(toremClubId);
        gEventManager->submitEvent(static_cast<uint32_t>(ClubsSlave::EVENT_MEMBERREMOVEDFROMCLUBEVENT), memberRemovedFromClubEvent, true /*logBinaryEvent*/);

        TRACE_LOG("[RemoveAndBanMemberCommandBase].removeMember: User [" << rtorBlazeId << "] removed user [" << toremBlazeId << "] in the club [" 
            << toremClubId << "], had permission.");

        return Blaze::ERR_OK;
    }


    BlazeRpcError 
    RemoveAndBanMemberCommandBase::banMember(ClubId              banClubId, 
                                             BlazeId             banBlazeId, 
                                             BanAction           banAction, 
                                             ClubsDbConnector*   dbc)
    {
        const BlazeId rtorBlazeId = gCurrentUserSession->getBlazeId();

        if (rtorBlazeId == banBlazeId)
        {
            return CLUBS_ERR_USER_CANNOT_BAN_SELF;
        }

        BlazeRpcError error = Blaze::ERR_SYSTEM;

        bool rtorIsAdmin = false;
        bool rtorIsThisClubGM = false;

        // a GM may have privilege to ban/unban members of his club.
        MembershipStatus rtorMshipStatus = CLUBS_MEMBER;
        MemberOnlineStatus rtorOnlSatus = CLUBS_MEMBER_OFFLINE;
        TimeValue rtorMemberSince = 0;

        error = mComponent->getMemberStatusInClub(
            banClubId,
            rtorBlazeId,
            dbc->getDb(),
            rtorMshipStatus,
            rtorOnlSatus,
            rtorMemberSince);
        
        if (error == Blaze::ERR_OK)
        {
            rtorIsThisClubGM = (rtorMshipStatus >= CLUBS_GM);
        }

        if (!rtorIsThisClubGM)
        {
            rtorIsAdmin = UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_ADMINISTRATOR, true);

            // The requestor is not a Admin or not this club's GM - can't ban/unban.
            if (!rtorIsAdmin)
            {
                if (error == CLUBS_ERR_USER_NOT_MEMBER)
                {
                    TRACE_LOG("[RemoveAndBanMemberCommandBase].banMember: User [" << rtorBlazeId << "] is not admin for club ["
                        << banClubId << "] and cannot " << (banAction == BAN_ACTION_BAN ? "ban" : "unban") << " user [" << banBlazeId << "]");
                    return CLUBS_ERR_NO_PRIVILEGE;
                }
                
                return error;
            }
        }
        else
        {
            // The requestor is this club's GM, but if unbanning an already banned user, 
            // there's an additional requirement that the original requestor has to have the
            // same or lower authorization level, i.e. he/she can't be a Clubs Admin.
            if (banAction == BAN_ACTION_UNBAN)
            {
                BanStatus origBan = CLUBS_NO_BAN;
                error = dbc->getDb().getBan(banClubId, banBlazeId, &origBan);

                if (error == Blaze::ERR_OK && origBan == CLUBS_BAN_BY_ADMIN)
                {
                    error = CLUBS_ERR_NO_PRIVILEGE;
                }

                if (error != Blaze::ERR_OK)
                {
                    rtorIsAdmin = UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_ADMINISTRATOR, true);

                    if (!rtorIsAdmin)
                    {
                        if (error == CLUBS_ERR_NO_PRIVILEGE)
                        {
                            TRACE_LOG("[RemoveAndBanMemberCommandBase].banMember: User [" << rtorBlazeId << "] is not admin for club ["
                                << banClubId << "] and cannot unban user [" << banBlazeId << "]");
                        }
                        return error;
                    }
                }
            }
        }


        // Perform actual banning or unbanning. If banning, the user will also be kicked out 
        // of the club. If unbanning, the user will NOT be automatically re-added to the club.
        if (banAction == BAN_ACTION_BAN)
        {
            error = dbc->getDb().addBan(banClubId, banBlazeId, 
                                        rtorIsThisClubGM ? CLUBS_BAN_BY_GM : CLUBS_BAN_BY_ADMIN);
        }
        else
        {
            error = dbc->getDb().removeBan(banClubId, banBlazeId);
        }

        return error;
    }

    
    BlazeRpcError 
    RemoveAndBanMemberCommandBase::executeGeneric(ClubId clubId)
    {
        TRACE_LOG("[" << mCmdName << "] Start");

        // ClubsDbConnector could be cached in a member and exposed to sub-classes via a getter. 
        ClubsDbConnector dbc(mComponent);

        if (!dbc.startTransaction())
        {
            return ERR_SYSTEM;
        }

        // Lock this club to prevent racing condition
        uint64_t version = 0;
        BlazeRpcError error = dbc.getDb().lockClub(clubId, version);
        
        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("[" << mCmdName << "] Database error (" << ErrorHelp::getErrorName(error) << ")");
            dbc.completeTransaction(false); 
            return error;
        }

        error = executeConcrete(&dbc);

        if (error != Blaze::ERR_OK)
        {
            dbc.completeTransaction(false); 
            return error;
        }

        if (!dbc.completeTransaction(true))
        {
            ERR_LOG("[" << mCmdName << "] Could not commit transaction");
            return ERR_SYSTEM;
        }

        return Blaze::ERR_OK;
    }

} // Clubs
} // Blaze
