/*************************************************************************************************/
/*!
    \file   disbandclub_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class DisbandClubCommand

    disband a club

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "removeandbanmember_commandbase.h"
#include "clubs/rpc/clubsslave/disbandclub_stub.h"
#include "stats/statsslaveimpl.h"

namespace Blaze
{
namespace Clubs
{

// NOTE: Before disband a club, all the members should be removed.
class DisbandClubCommand : public DisbandClubCommandStub, private ClubsCommandBase
{
public:

    DisbandClubCommand(Message* message, DisbandClubRequest* request, ClubsSlaveImpl* componentImpl)
        : DisbandClubCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~DisbandClubCommand() override
    {
    }

private:

    BlazeRpcError purgeClubMembers(ClubId mClubId)
    {
        const char8_t* mCmdName = "PurgeClubMembers";
        const BlazeId rtorBlazeId = gCurrentUserSession->getBlazeId();

        // Get club members
        ClubsDbConnector dbc(mComponent); 
        if (dbc.acquireDbConnection(true) == nullptr)
            return Blaze::ERR_SYSTEM;
        ClubMemberList memberlist;
        BlazeRpcError error = dbc.getDb().getClubMembers(mClubId, UINT32_MAX, 0, memberlist);
        dbc.releaseConnection();
        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("[" << mCmdName << "] Get members failed!");
            return error;
        }

        // Purge club members
        for (ClubMemberList::iterator it = memberlist.begin(); it != memberlist.end(); it++)
        {
            const BlazeId blazeId = (*it)->getUser().getBlazeId();

            // Remove member from cache
            if (gUserSessionManager->isUserOnline(blazeId))
            {
                mComponent->invalidateCachedMemberInfo(blazeId, mClubId);

                // Update user extended data
                error = mComponent->removeMemberUserExtendedData(blazeId, mClubId, UPDATE_REASON_CLUB_DESTROYED);
                if (error != Blaze::ERR_OK)
                {
                    return error;
                }

            }
            // Unsubscribe from club ticker messages
            mComponent->setClubTickerSubscription(mClubId, blazeId, false);

            // Send notification about removal
            error = mComponent->notifyMemberRemoved(
                mClubId,
                rtorBlazeId,
                blazeId);

            if (error != Blaze::ERR_OK)
            {
                WARN_LOG("[" << mCmdName << "] Could not send notifications after removing member " << blazeId
                    << " from club " << mClubId << " because custom processing code notifyUserRemoved returned error: " << SbFormats::HexLower(error) << ".");
            }

            mComponent->mPerfJoinsLeaves.increment();
        }

        return Blaze::ERR_OK;
    }

    BlazeRpcError removeAndPurgeClub(ClubId mClubId)
    {
        const char8_t* mCmdName = "RemoveAndPurgeClub";

        // Remove the club
        ClubsDbConnector dbc(mComponent);
        if (dbc.acquireDbConnection(false) == nullptr)
            return Blaze::ERR_SYSTEM;
        BlazeRpcError error = dbc.getDb().removeClub(mClubId);
        dbc.releaseConnection();
        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("[" << mCmdName << "] Failed to delete club [clubid]: " << mClubId << " Error was: " << ErrorHelp::getErrorName(error));
            return error;
        }

        // Purge the club

        // Remove club from cache
        mComponent->invalidateCachedClubInfo(mClubId, true);

        // Delete stats of the club and members
        Stats::StatsSlaveImpl* stats = static_cast<Stats::StatsSlaveImpl*>(gController->getComponent(Stats::StatsSlave::COMPONENT_ID));
        if (stats != nullptr)
        {
            if( mComponent->isUsingClubKeyscopedStats() )
            {
                Stats::ScopeName scopeName = "club";
                Stats::ScopeValue scopeValue = mClubId;
                Stats::DeleteAllStatsByKeyScopeRequest delClubMemberStatsRequest;
            
                delClubMemberStatsRequest.getKeyScopeNameValueMap().clear();
                delClubMemberStatsRequest.getKeyScopeNameValueMap().insert(Stats::ScopeNameValueMap::value_type(scopeName, scopeValue));
                stats->deleteAllStatsByKeyScope(delClubMemberStatsRequest);
            }

            Stats::DeleteAllStatsByEntityRequest delClubStatsRequest;
            delClubStatsRequest.setEntityType(ENTITY_TYPE_CLUB);
            delClubStatsRequest.setEntityId(mClubId);
            stats->deleteAllStatsByEntity(delClubStatsRequest);
        }

        return Blaze::ERR_OK;
    }

    DisbandClubCommandStub::Errors execute() override
    {
        const char8_t* mCmdName = "DisbandClubCommand";
        ClubId mClubId = mRequest.getClubId();

        // Figure out authorization level.
        bool rtorIsAdmin = UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_ADMINISTRATOR, true);
        if (!rtorIsAdmin)
        {
            TRACE_LOG("[DisbandClubCommand] user [" << gCurrentUserSession->getBlazeId()
                << "] is not admin for club [" << mClubId << "] and cannot disband club");
            return ERR_AUTHORIZATION_REQUIRED;
        }

        TRACE_LOG("[" << mCmdName << "] Disband club start.");

        BlazeRpcError error = purgeClubMembers(mClubId);
        if (error != Blaze::ERR_OK)
        {
            return commandErrorFromBlazeError(error);
        }

        error = removeAndPurgeClub(mClubId);
        if (error != Blaze::ERR_OK)
        {
            return commandErrorFromBlazeError(error);
        }

        // Update counters
        mComponent->mPerfAdminActions.increment();

        TRACE_LOG("[" << mCmdName << "] Disband club end.");

        return ERR_OK;
    }

};
// static factory method impl
DEFINE_DISBANDCLUB_CREATE()

} // Clubs
} // Blaze
