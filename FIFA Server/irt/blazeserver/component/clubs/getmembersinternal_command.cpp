/*************************************************************************************************/
/*!
    \file   getmembersinternal_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetMembersInternal

    Get members for a specify club.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "clubs/rpc/clubsslave/getmembersinternal_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"
#include "clubsconstants.h"

namespace Blaze
{
namespace Clubs
{

class GetMembersInternalCommand : public GetMembersInternalCommandStub, private ClubsCommandBase
{
public:
    GetMembersInternalCommand(Message* message, GetMembersInternalRequest* request, ClubsSlaveImpl* componentImpl)
        : GetMembersInternalCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~GetMembersInternalCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    GetMembersInternalCommandStub::Errors execute() override
    {
        BlazeRpcError result = Blaze::ERR_OK;

        TransactionContextPtr ptr = mComponent->getTransactionContext(mRequest.getTransactionContextId());
        if (ptr == nullptr)
        {
            TRACE_LOG("[GetMembersInternalCommand] cannot obtain transaction for transaction context id[" << mRequest.getTransactionContextId() <<"].");
            return ERR_SYSTEM;
        }

        DbConnPtr dbPtr = static_cast<ClubsTransactionContext&>(*ptr).getDatabaseConnection(); 

        result = mComponent->getClubMembers(
            dbPtr,
            mRequest.getClubId(),
            mRequest.getMaxResultCount(),
            mRequest.getOffset(),
            mRequest.getOrderType(),
            mRequest.getOrderMode(),
            mRequest.getMemberType(),
            mRequest.getSkipCalcDbRows(),
            mRequest.getPersonaNamePattern(),
            mResponse);

        mComponent->overrideOnlineStatus(mResponse.getClubMemberList());

        return commandErrorFromBlazeError(result);
    }

};
// static factory method impl
DEFINE_GETMEMBERSINTERNAL_CREATE()

} // Clubs
} // Blaze
