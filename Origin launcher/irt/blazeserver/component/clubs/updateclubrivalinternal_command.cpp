/*************************************************************************************************/
/*!
    \file   updateclubrivalinternal_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdateClubRivalInternal

    Update club rival.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "clubs/rpc/clubsslave/updateclubrivalinternal_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"
#include "clubsconstants.h"

namespace Blaze
{
namespace Clubs
{

class UpdateClubRivalInternalCommand : public UpdateClubRivalInternalCommandStub, private ClubsCommandBase
{
public:
    UpdateClubRivalInternalCommand(Message* message, UpdateClubRivalRequest* request, ClubsSlaveImpl* componentImpl)
        : UpdateClubRivalInternalCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~UpdateClubRivalInternalCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    UpdateClubRivalInternalCommandStub::Errors execute() override
    {
        BlazeRpcError result = Blaze::ERR_OK;

        TransactionContextPtr ptr = mComponent->getTransactionContext(mRequest.getTransactionContextId());
        if (ptr == nullptr)
        {
            TRACE_LOG("[UpdateClubRivalInternalCommand] cannot obtain transaction for transaction context id[" << mRequest.getTransactionContextId() <<"].");
            return ERR_SYSTEM;
        }

        DbConnPtr dbPtr = static_cast<ClubsTransactionContext&>(*ptr).getDatabaseConnection(); 

        result = mComponent->updateClubRival(dbPtr, mRequest.getClubId(), mRequest.getClubRival());

        return commandErrorFromBlazeError(result);
    }

};
// static factory method impl
DEFINE_UPDATECLUBRIVALINTERNAL_CREATE()

} // Clubs
} // Blaze
