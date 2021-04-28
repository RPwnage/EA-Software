/*************************************************************************************************/
/*!
    \file   unbanmember_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UnbanMemberCommand

    Removes some club's ban for a user.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "removeandbanmember_commandbase.h"
#include "clubs/rpc/clubsslave/unbanmember_stub.h"

namespace Blaze
{
namespace Clubs
{

    // NOTE: The order of inheritance matters, because we use 'this' in both this class and
    // RemoveAndBanMemberCommandBase to identify the object for logging purposes.
    class UnbanMemberCommand : private RemoveAndBanMemberCommandBase, 
                               public UnbanMemberCommandStub
    {
    public:
    
        UnbanMemberCommand(Message* message, BanUnbanMemberRequest* request, ClubsSlaveImpl* componentImpl)
        :   RemoveAndBanMemberCommandBase(componentImpl, "UnbanMemberCommand"),
            UnbanMemberCommandStub(message, request)
        {
        }

        ~UnbanMemberCommand() override
        {
        }

    private:

        UnbanMemberCommandStub::Errors execute() override
        {
            return commandErrorFromBlazeError(
                RemoveAndBanMemberCommandBase::executeGeneric(mRequest.getClubId()));
        }
        
        BlazeRpcError executeConcrete(ClubsDbConnector* dbc) override
        {
            return banMember(mRequest.getClubId(), mRequest.getBlazeId(), BAN_ACTION_UNBAN, dbc);
        }
    };

    // static factory method impl
    DEFINE_UNBANMEMBER_CREATE()

} // Clubs
} // Blaze
