/*************************************************************************************************/
/*!
    \file   lockclubsinternal_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class LockClubsInternal

    Lock clubs.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "clubs/rpc/clubsslave/lockclubsinternal_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"
#include "clubsconstants.h"

namespace Blaze
{
namespace Clubs
{

class LockClubsInternalCommand : public LockClubsInternalCommandStub, private ClubsCommandBase
{
public:
    LockClubsInternalCommand(Message* message, LockClubsRequest* request, ClubsSlaveImpl* componentImpl)
        : LockClubsInternalCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~LockClubsInternalCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    LockClubsInternalCommandStub::Errors execute() override
    {
        BlazeRpcError result = Blaze::ERR_OK;

        TransactionContextPtr ptr = mComponent->getTransactionContext(mRequest.getTransactionContextId());
        if (ptr == nullptr)
        {
            TRACE_LOG("[LockClubsInternalCommand] cannot obtain transaction for transaction context id[" << mRequest.getTransactionContextId() <<"].");
            return ERR_SYSTEM;
        }

        DbConnPtr dbPtr = static_cast<ClubsTransactionContext&>(*ptr).getDatabaseConnection(); 

        result = mComponent->lockClubs(dbPtr, &(mRequest.getClubIds()));

        return commandErrorFromBlazeError(result);
    }

};
// static factory method impl
DEFINE_LOCKCLUBSINTERNAL_CREATE()

} // Clubs
} // Blaze
