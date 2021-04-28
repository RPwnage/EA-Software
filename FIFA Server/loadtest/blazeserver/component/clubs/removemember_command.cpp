/*************************************************************************************************/
/*!
    \file   removemember_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class RemoveMemberCommand

    Remove a member from a club

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "removeandbanmember_commandbase.h"
#include "clubs/rpc/clubsslave/removemember_stub.h"

namespace Blaze
{
namespace Clubs
{

    // NOTE: The order of inheritance matters, because we use 'this' in both this class and
    // RemoveAndBanMemberCommandBase to identify the object for logging purposes.
    class RemoveMemberCommand : private RemoveAndBanMemberCommandBase, 
                                public RemoveMemberCommandStub
    {
    public:
    
        RemoveMemberCommand(Message* message, RemoveMemberRequest* request, ClubsSlaveImpl* componentImpl)
        :   RemoveAndBanMemberCommandBase(componentImpl, "RemoveMemberCommand"),
            RemoveMemberCommandStub(message, request)
        {
        }

        ~RemoveMemberCommand() override
        {
        }

    private:

        RemoveMemberCommandStub::Errors execute() override
        {
            return commandErrorFromBlazeError(
                RemoveAndBanMemberCommandBase::executeGeneric(mRequest.getClubId()));
        }
        
        BlazeRpcError executeConcrete(ClubsDbConnector* dbc) override
        {
            return removeMember(mRequest.getClubId(), mRequest.getBlazeId(), dbc);
        }
    };

    // static factory method impl
    DEFINE_REMOVEMEMBER_CREATE()

} // Clubs
} // Blaze
