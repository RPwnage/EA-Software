/*************************************************************************************************/
/*!
    \file   getclubbans_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetClubBansCommand

        Get ban statuses for all ex-members of a club that are currently banned from it.

    \note
        Ban status conveys the authority type that banned the member. Non-banned users still in 
        the club are filtered out.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"

// clubs includes
#include "clubs/rpc/clubsslave/getclubbans_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

class GetClubBansCommand : public GetClubBansCommandStub, private ClubsCommandBase
{
public:
    GetClubBansCommand(Message* message, GetClubBansRequest* request, ClubsSlaveImpl* componentImpl)
        : GetClubBansCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~GetClubBansCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    GetClubBansCommandStub::Errors execute() override
    {
        TRACE_LOG("[GetClubBansCommand] start");

        const ClubId banClubId = mRequest.getClubId();
        const BlazeId rtorBlazeId = gCurrentUserSession->getBlazeId();

        ClubsDbConnector dbc(mComponent); 

        if (dbc.acquireDbConnection(true) == nullptr)
        {
            return ERR_SYSTEM;
        }

        BlazeRpcError error = Blaze::ERR_SYSTEM;

        bool rtorIsAdmin = false;
        bool rtorIsThisClubGm = false;

        // Only GMs or Clubs Admins can access this RPC.
        // Firstly, check if he is the GM of the club for which
        // information is requested.
        MembershipStatus rtorMshipStatus = CLUBS_MEMBER;
        MemberOnlineStatus rtorOnlSatus = CLUBS_MEMBER_OFFLINE;
        TimeValue rtotMemberSince = 0;
        error = mComponent->getMemberStatusInClub(
            banClubId,
            rtorBlazeId,
            dbc.getDb(), 
            rtorMshipStatus, 
            rtorOnlSatus,
            rtotMemberSince);

        if (error == Blaze::ERR_OK)
        {
            rtorIsThisClubGm = (rtorMshipStatus >= CLUBS_GM);
        }

        if (!rtorIsThisClubGm)
        {
            rtorIsAdmin = UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_ADMINISTRATOR_VIEW, true);

            if (!rtorIsAdmin)
            {
                TRACE_LOG("[GetClubBansCommand] user [" << rtorBlazeId << "] does not have admin view for club ["
                    << banClubId << "] and is not allowed this action");
                if (error == Blaze::ERR_OK)
                {
                    return CLUBS_ERR_NO_PRIVILEGE;
                }
                return commandErrorFromBlazeError(error);
            }
        }

        error = dbc.getDb().getClubBans(banClubId, &mResponse.getBlazeIdToBanStatusMap());

        if (error != Blaze::ERR_OK)
        {
            return commandErrorFromBlazeError(error);
        }

        return ERR_OK;
    }

};
// static factory method impl
DEFINE_GETCLUBBANS_CREATE()

} // Clubs
} // Blaze
