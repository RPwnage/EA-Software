/*************************************************************************************************/
/*!
    \file   checkmembershipinternal_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class CheckMembershipInternal

    Check membership.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "clubs/rpc/clubsslave/checkmembershipinternal_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"
#include "clubsconstants.h"

namespace Blaze
{
namespace Clubs
{

class CheckMembershipInternalCommand : public CheckMembershipInternalCommandStub, private ClubsCommandBase
{
public:
    CheckMembershipInternalCommand(Message* message, CheckMembershipRequest* request, ClubsSlaveImpl* componentImpl)
        : CheckMembershipInternalCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~CheckMembershipInternalCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    CheckMembershipInternalCommandStub::Errors execute() override
    {
        BlazeRpcError result = Blaze::ERR_OK;

        TransactionContextPtr ptr = mComponent->getTransactionContext(mRequest.getTransactionContextId());
        if (ptr == nullptr)
        {
            TRACE_LOG("[CheckMembershipInternalCommand] cannot obtain transaction for transaction context id[" << mRequest.getTransactionContextId() <<"].");
            return ERR_SYSTEM;
        }

        DbConnPtr dbPtr = static_cast<ClubsTransactionContext&>(*ptr).getDatabaseConnection(); 

        if (!mComponent->checkMembership(dbPtr, mRequest.getClubId(), mRequest.getBlazeId()))
        {
            result = Blaze::CLUBS_ERR_USER_NOT_MEMBER;
        }

        return commandErrorFromBlazeError(result);
    }

};
// static factory method impl
DEFINE_CHECKMEMBERSHIPINTERNAL_CREATE()

} // Clubs
} // Blaze
