/*************************************************************************************************/
/*!
    \file   getinvitations_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetInvitationsCommand

    Get invitations sent to or from a particular user

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/dbutil.h"
#include "framework/database/query.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"

// clubs includes
#include "clubs/rpc/clubsslave/getinvitations_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

class GetInvitationsCommand : public GetInvitationsCommandStub, private ClubsCommandBase
{
public:
    GetInvitationsCommand(Message* message, GetInvitationsRequest* request, ClubsSlaveImpl* componentImpl)
        : GetInvitationsCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~GetInvitationsCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:

    GetInvitationsCommandStub::Errors execute() override
    {
        TRACE_LOG("[GetInvitationsCommand] start");

        BlazeId requestorId = gCurrentUserSession->getBlazeId();

        ClubsDbConnector dbc(mComponent); 
        if (dbc.acquireDbConnection(true) == nullptr)
            return ERR_SYSTEM;

        BlazeRpcError error;
        bool rtorIsAdmin = false;
        ClubId requestorClubId = mRequest.getClubId();

        switch(mRequest.getInvitationsType())
        {
        case CLUBS_SENT_TO_ME:
            {
                error = dbc.getDb().getClubMessagesByUserReceived(requestorId, CLUBS_INVITATION,
                    mRequest.getSortType(), mResponse.getClubInvList());
                break;
            }
        case CLUBS_SENT_BY_ME:
            {
                error = dbc.getDb().getClubMessagesByUserSent(requestorId, CLUBS_INVITATION,
                    mRequest.getSortType(), mResponse.getClubInvList());
                break;
            }
        case CLUBS_SENT_BY_CLUB:
            {
                // club admin and club members can get club invitations
                MembershipStatus mshipStatus;
                MemberOnlineStatus onlSatus;
                TimeValue memberSince;
                error = mComponent->getMemberStatusInClub(
                    requestorClubId, 
                    requestorId, 
                    dbc.getDb(), 
                    mshipStatus, 
                    onlSatus,
                    memberSince);

                if (error != (BlazeRpcError)ERR_OK)
                {
                    rtorIsAdmin = UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_ADMINISTRATOR_VIEW, true);

                    if (!rtorIsAdmin)
                    {
                        TRACE_LOG("[GetInvitationsCommand] user [" << requestorId << "] does not have admin view for club ["
                            << requestorClubId << "] on CLUBS_SENT_BY_CLUB and is not allowed this action");
                        return CLUBS_ERR_USER_NOT_MEMBER;
                    }
                }

                error = dbc.getDb().getClubMessagesByClub(requestorClubId, 
                    CLUBS_INVITATION, mRequest.getSortType(), mResponse.getClubInvList());
                break;
            }
        default:
            {
                error = Blaze::CLUBS_ERR_INVALID_ARGUMENT;
                break;
            }
        }
        dbc.releaseConnection();

        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("[GetInvitationsCommand] failed to get invitation type [invitationType]: " << mRequest.getInvitationsType() 
                << " user [blazeid]: " << requestorId << " club [clubid]: " << requestorClubId << " error is " << ErrorHelp::getErrorName(error));

            return commandErrorFromBlazeError(error);
        }

        ClubMessageList::iterator it = mResponse.getClubInvList().begin();
        for(it = mResponse.getClubInvList().begin(); it != mResponse.getClubInvList().end(); it++)
        {
            ClubMessage* clubMsg = *it;
            UserInfoPtr userInfo;
            error = gUserSessionManager->lookupUserInfoByBlazeId(clubMsg->getReceiver().getBlazeId(), userInfo);
            if (error == Blaze::ERR_OK)
            {
                UserInfo::filloutUserCoreIdentification(*userInfo, clubMsg->getReceiver());
            }
            else
            {
                WARN_LOG("[GetInvitationsCommand] error (" << ErrorHelp::getErrorName(error) << ") looking up user " << clubMsg->getReceiver().getBlazeId()
                    << " for club " << requestorClubId);
                if (error != USER_ERR_USER_NOT_FOUND)
                {
                    return commandErrorFromBlazeError(error);
                }
            }
            error = gUserSessionManager->lookupUserInfoByBlazeId(clubMsg->getSender().getBlazeId(), userInfo);
            if (error == Blaze::ERR_OK)
            {
                UserInfo::filloutUserCoreIdentification(*userInfo, clubMsg->getSender());
            }
            else
            {
                WARN_LOG("[GetInvitationsCommand] error (" << ErrorHelp::getErrorName(error) << ") looking up user " << clubMsg->getSender().getBlazeId()
                    << " for club " << requestorClubId);
                if (error != USER_ERR_USER_NOT_FOUND)
                {
                    return commandErrorFromBlazeError(error);
                }
            }
        }

        return ERR_OK;
    }

};
// static factory method impl
DEFINE_GETINVITATIONS_CREATE()

} // Clubs
} // Blaze
