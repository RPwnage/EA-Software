/*************************************************************************************************/
/*!
    \file   getmembers_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetMembersCommand

    get a list of members of a club

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
#include "clubs/rpc/clubsslave/getmembers_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

class GetMembersCommand : public GetMembersCommandStub, private ClubsCommandBase
{
public:
    GetMembersCommand(Message* message, GetMembersRequest* request, ClubsSlaveImpl* componentImpl)
        : GetMembersCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~GetMembersCommand() override
    {
    }

        /* Private methods *******************************************************************************/
private:
    GetMembersCommandStub::Errors execute() override
    {
        TRACE_LOG("[GetMembersCommand] start");

        ClubsDbConnector dbc(mComponent); 
        if (dbc.acquireDbConnection(true) == nullptr)
            return ERR_SYSTEM;

        TRACE_LOG("[GetMembersCommand] received DB connection.");

        BlazeRpcError error = mComponent->getClubMembers(
            dbc.getDbConn(),
            mRequest.getClubId(),
            mRequest.getMaxResultCount(),
            mRequest.getOffset(),
            mRequest.getOrderType(),
            mRequest.getOrderMode(),
            mRequest.getMemberType(),
            mRequest.getSkipCalcDbRows(),
            mRequest.getPersonaNamePattern(),
            mResponse);

        dbc.releaseConnection();
        
        mComponent->overrideOnlineStatus(mResponse.getClubMemberList());
        
        return commandErrorFromBlazeError(error);
    }

};
// static factory method impl
DEFINE_GETMEMBERS_CREATE()

} // Clubs
} // Blaze
