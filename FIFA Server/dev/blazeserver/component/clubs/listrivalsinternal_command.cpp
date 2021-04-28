/*************************************************************************************************/
/*!
    \file   listrivalsinternal_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ListRivalsInternal

    List rivals for a specify club.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "clubs/rpc/clubsslave/listrivalsinternal_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"
#include "clubsconstants.h"

namespace Blaze
{
namespace Clubs
{

class ListRivalsInternalCommand : public ListRivalsInternalCommandStub, private ClubsCommandBase
{
public:
    ListRivalsInternalCommand(Message* message, ListRivalsInternalRequest* request, ClubsSlaveImpl* componentImpl)
        : ListRivalsInternalCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~ListRivalsInternalCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    ListRivalsInternalCommandStub::Errors execute() override
    {
        BlazeRpcError result = Blaze::ERR_OK;

        TransactionContextPtr ptr = mComponent->getTransactionContext(mRequest.getTransactionContextId());
        if (ptr == nullptr)
        {
            TRACE_LOG("[ListRivalsInternalCommand] cannot obtain transaction for transaction context id[" << mRequest.getTransactionContextId() <<"].");
            return ERR_SYSTEM;
        }

        DbConnPtr dbPtr = static_cast<ClubsTransactionContext&>(*ptr).getDatabaseConnection(); 

        result = mComponent->listRivals(
            dbPtr, 
            mRequest.getClubId(), 
            mResponse.getClubRivalList());

        return commandErrorFromBlazeError(result);
    }

};
// static factory method impl
DEFINE_LISTRIVALSINTERNAL_CREATE()

} // Clubs
} // Blaze
