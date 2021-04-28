/*************************************************************************************************/
/*!
    \file   awardclubinternal_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class AwardClubInternal

    Get awards for a specific club.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "clubs/rpc/clubsslave/awardclubinternal_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"
#include "clubsconstants.h"

namespace Blaze
{
namespace Clubs
{

class AwardClubInternalCommand : public AwardClubInternalCommandStub, private ClubsCommandBase
{
public:
    AwardClubInternalCommand(Message* message, AwardClubRequest* request, ClubsSlaveImpl* componentImpl)
        : AwardClubInternalCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~AwardClubInternalCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    AwardClubInternalCommandStub::Errors execute() override
    {
        BlazeRpcError result = Blaze::ERR_OK;

        TransactionContextPtr ptr = mComponent->getTransactionContext(mRequest.getTransactionContextId());
        if (ptr == nullptr)
        {
            TRACE_LOG("[AwardClubInternalCommand] cannot obtain transaction for transaction context id[" << mRequest.getTransactionContextId() <<"].");
            return ERR_SYSTEM;
        }

        DbConnPtr dbPtr = static_cast<ClubsTransactionContext&>(*ptr).getDatabaseConnection(); 

        result = mComponent->awardClub(dbPtr, mRequest.getClubId(), mRequest.getAwardId());

        return commandErrorFromBlazeError(result);
    }

};
// static factory method impl
DEFINE_AWARDCLUBINTERNAL_CREATE()

} // Clubs
} // Blaze
