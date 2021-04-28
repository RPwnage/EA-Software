/*************************************************************************************************/
/*!
    \file   getclubmembershipforusers_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetClubMembershipForUsersCommand

    lookup club membership information for a given list of users

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"

// clubs includes
#include "clubs/rpc/clubsslave/getclubmembershipforusers_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

class GetClubMembershipForUsersCommand : public GetClubMembershipForUsersCommandStub, private ClubsCommandBase
{
public:
    GetClubMembershipForUsersCommand(Message* message, GetClubMembershipForUsersRequest* request, ClubsSlaveImpl* componentImpl)
        : GetClubMembershipForUsersCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~GetClubMembershipForUsersCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    GetClubMembershipForUsersCommandStub::Errors execute() override
    {
        TRACE_LOG("[GetClubMembershipForUsersCommand] start");

        if (mRequest.getBlazeIdList().size() == 0)
            return CLUBS_ERR_INVALID_ARGUMENT;

        ClubsDbConnector dbc(mComponent);
        if (dbc.acquireDbConnection(true) == nullptr)
            return ERR_SYSTEM;

        TRACE_LOG("[GetClubMembershipForUsersCommand] received DB connection.");

        BlazeRpcError error = dbc.getDb().getClubMembershipForUsers(mRequest.getBlazeIdList(), 
            mResponse.getMembershipMap());
        dbc.releaseConnection();

        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("[GetClubMembershipForUsersCommand] Database error.");
            return ERR_SYSTEM;
        }

        // resolve identities
        Blaze::BlazeIdToUserInfoMap idMap;

        error = gUserSessionManager->lookupUserInfoByBlazeIds(mRequest.getBlazeIdList().asVector(), idMap);

        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("[GetClubMembershipForUsersCommand] User identity provider returned an error (" << ErrorHelp::getErrorName(error) << ")");
            return commandErrorFromBlazeError(error);
        }

        ClubMembershipMap::iterator it = mResponse.getMembershipMap().begin();
        for (; it != mResponse.getMembershipMap().end(); it++)
        {
            Blaze::BlazeIdToUserInfoMap::const_iterator itm = idMap.find(it->first);
            if (itm != idMap.end())
            {
                ClubMembershipList &membershipList = it->second->getClubMembershipList();
                ClubMembershipList::iterator msItr = membershipList.begin();
                ClubMembershipList::iterator msEnd = membershipList.end();
                for (; msItr != msEnd; ++msItr)
                {
                    UserInfo::filloutUserCoreIdentification(*itm->second, (*msItr)->getClubMember().getUser());
                }
            }
        }
        // set club online status
        ClubMemberList memberList;
        for (it = mResponse.getMembershipMap().begin();
            it != mResponse.getMembershipMap().end();
            it++)
        {
            ClubMembershipList &membershipList = it->second->getClubMembershipList();
            ClubMembershipList::iterator msItr = membershipList.begin();
            ClubMembershipList::iterator msEnd = membershipList.end();
            for (; msItr != msEnd; ++msItr)
            {
                MemberOnlineStatus status = mComponent->getMemberOnlineStatus(it->first, (*msItr)->getClubId());
                (*msItr)->getClubMember().setOnlineStatus(status);
                memberList.push_back(&((*msItr)->getClubMember()));
            }
        }
        
        mComponent->overrideOnlineStatus(memberList);

        return commandErrorFromBlazeError(error);
    }

};
// static factory method impl
DEFINE_GETCLUBMEMBERSHIPFORUSERS_CREATE()

} // Clubs
} // Blaze
