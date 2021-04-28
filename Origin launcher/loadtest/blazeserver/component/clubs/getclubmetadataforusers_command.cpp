/*************************************************************************************************/
/*!
    \file   getclubmetadataforusers_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetClubMetadataForUsersCommand

    lookup club metadata for a given list of users

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
#include "clubs/rpc/clubsslave/getclubmetadataforusers_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

class GetClubMetadataForUsersCommand : public GetClubMetadataForUsersCommandStub, private ClubsCommandBase
{
public:
    GetClubMetadataForUsersCommand(Message* message, GetClubMetadataForUsersRequest* request, ClubsSlaveImpl* componentImpl)
        : GetClubMetadataForUsersCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~GetClubMetadataForUsersCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    GetClubMetadataForUsersCommandStub::Errors execute() override
    {
        TRACE_LOG("[GetClubMetadataForUsersCommand] start");

        if (mRequest.getBlazeIdList().size() == 0)
            return CLUBS_ERR_INVALID_ARGUMENT;

        ClubsDbConnector dbc(mComponent);
        if (dbc.acquireDbConnection(true) == nullptr)
            return ERR_SYSTEM;

        TRACE_LOG("[GetClubMetadataForUsersCommand] received DB connection.");

        BlazeRpcError error = dbc.getDb().getClubMetadataForUsers(mRequest.getBlazeIdList(), mResponse.getMetadataMap());
        dbc.releaseConnection();

        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("[GetClubMetadataForUsersCommand] Database error.");
            return ERR_SYSTEM;
        }

		return ERR_OK; 
    }

};
// static factory method impl
DEFINE_GETCLUBMETADATAFORUSERS_CREATE()

} // Clubs
} // Blaze
