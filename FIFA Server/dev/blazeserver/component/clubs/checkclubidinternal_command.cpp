/*************************************************************************************************/
/*!
    \file   checkclubidinternal_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class CheckClubIdInternal

    Check club id.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "clubs/rpc/clubsslave/checkclubidinternal_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"
#include "clubsconstants.h"

namespace Blaze
{
namespace Clubs
{

class CheckClubIdInternalCommand : public CheckClubIdInternalCommandStub, private ClubsCommandBase
{
public:
    CheckClubIdInternalCommand(Message* message, CheckClubIdRequest* request, ClubsSlaveImpl* componentImpl)
        : CheckClubIdInternalCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~CheckClubIdInternalCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    CheckClubIdInternalCommandStub::Errors execute() override
    {
        BlazeRpcError result = Blaze::ERR_OK;

        TransactionContextPtr ptr = mComponent->getTransactionContext(mRequest.getTransactionContextId());
        if (ptr == nullptr)
        {
            TRACE_LOG("[CheckClubIdInternalCommand] cannot obtain transaction for transaction context id[" << mRequest.getTransactionContextId() <<"].");
            return ERR_SYSTEM;
        }

        DbConnPtr dbPtr = static_cast<ClubsTransactionContext&>(*ptr).getDatabaseConnection(); 

        if (!mComponent->checkClubId(dbPtr, mRequest.getClubId()))
        {
            result = Blaze::CLUBS_ERR_INVALID_CLUB_ID;
        }

        return commandErrorFromBlazeError(result);
    }

};
// static factory method impl
DEFINE_CHECKCLUBIDINTERNAL_CREATE()

} // Clubs
} // Blaze
