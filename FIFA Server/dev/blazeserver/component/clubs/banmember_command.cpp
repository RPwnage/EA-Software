/*************************************************************************************************/
/*!
    \file   banmember_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class BanMemberCommand

    Bans a member or a non-member user from a club.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "removeandbanmember_commandbase.h"
#include "clubs/rpc/clubsslave/banmember_stub.h"

namespace Blaze
{
namespace Clubs
{

// NOTE: The order of inheritance matters, because we use 'this' in
// RemoveAndBanMemberCommandBase to identify the object for logging purposes.
class BanMemberCommand : private RemoveAndBanMemberCommandBase,
    public BanMemberCommandStub
{
public:
    BanMemberCommand(Message* message, BanUnbanMemberRequest* request, ClubsSlaveImpl* componentImpl)
        :RemoveAndBanMemberCommandBase(componentImpl, "BanMemberCommand"),
        BanMemberCommandStub(message, request)
    {
    }

    ~BanMemberCommand() override
    {
    }

private:

    BanMemberCommandStub::Errors execute() override
    {
        return commandErrorFromBlazeError(RemoveAndBanMemberCommandBase::executeGeneric(mRequest.getClubId()));
    }

    BlazeRpcError executeConcrete(ClubsDbConnector* dbc) override
    {
        // Insert a ban record unconditionally: even if the user is not a member of the club, we
        // still want to ban them to prevent them from ever joining the club.
        BlazeRpcError error = banMember(mRequest.getClubId(), mRequest.getBlazeId(), BAN_ACTION_BAN, dbc);

        if (error == Blaze::ERR_OK)
        {
            error = removeMember(mRequest.getClubId(), mRequest.getBlazeId(), dbc);

            // User can be preemptively banned even if he/she is not currently a member of the
            // club.
            if (error == Blaze::CLUBS_ERR_USER_NOT_MEMBER)
            {
                error = Blaze::ERR_OK;
            }
        }

        return error;
    }

};
// static factory method impl
DEFINE_BANMEMBER_CREATE()

} // Clubs
} // Blaze
