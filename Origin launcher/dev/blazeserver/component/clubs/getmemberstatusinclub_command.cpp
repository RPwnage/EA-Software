/*************************************************************************************************/
/*!
    \file   getmemberstatusinclub_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetMemberStatusInClubCommand

    get member status in the given club for users.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/controller/controller.h"

// clubs includes
#include "clubs/rpc/clubsslave/getmemberstatusinclub_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

// messaging includes
#include "messaging/messagingslaveimpl.h"
#include "messaging/tdf/messagingtypes.h"

namespace Blaze
{
namespace Clubs
{

class GetMemberStatusInClubCommand : public GetMemberStatusInClubCommandStub, private ClubsCommandBase
{
public:
    GetMemberStatusInClubCommand(Message* message, GetMemberStatusInClubRequest* request, ClubsSlaveImpl* componentImpl)
        : GetMemberStatusInClubCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~GetMemberStatusInClubCommand() override
    {
    }

        /* Private methods *******************************************************************************/
private:

    GetMemberStatusInClubCommandStub::Errors execute() override
    {
        ClubsDbConnector dbc(mComponent); 
        if (dbc.acquireDbConnection(true) == nullptr)
            return ERR_SYSTEM;

        BlazeRpcError result = mComponent->getMemberStatusInClub(mRequest.getClubId(),
                                                                 mRequest.getBlazeIds(),
                                                                 dbc.getDb(),
                                                                 mResponse.getStatus());
                            
        return commandErrorFromBlazeError(result);
    }
};

// static factory method impl
DEFINE_GETMEMBERSTATUSINCLUB_CREATE()

} // Clubs
} // Blaze
