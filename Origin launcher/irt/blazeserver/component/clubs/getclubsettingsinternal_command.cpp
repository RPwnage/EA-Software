/*************************************************************************************************/
/*!
    \file   getclubsettingsinternal_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetClubSettingsInternal

    Get club settings.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "clubs/rpc/clubsslave/getclubsettingsinternal_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"
#include "clubsconstants.h"

namespace Blaze
{
namespace Clubs
{

class GetClubSettingsInternalCommand : public GetClubSettingsInternalCommandStub, private ClubsCommandBase
{
public:
    GetClubSettingsInternalCommand(Message* message, GetClubSettingsRequest* request, ClubsSlaveImpl* componentImpl)
        : GetClubSettingsInternalCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~GetClubSettingsInternalCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    GetClubSettingsInternalCommandStub::Errors execute() override
    {
        BlazeRpcError result = Blaze::ERR_OK;

        TransactionContextPtr ptr = mComponent->getTransactionContext(mRequest.getTransactionContextId());
        if (ptr == nullptr)
        {
            TRACE_LOG("[GetClubSettingsInternalCommand] cannot obtain transaction for transaction context id[" << mRequest.getTransactionContextId() <<"].");
            return ERR_SYSTEM;
        }

        DbConnPtr dbPtr = static_cast<ClubsTransactionContext&>(*ptr).getDatabaseConnection(); 

        result = mComponent->getClubSettings(dbPtr, mRequest.getClubId(), mResponse.getClubSettings());

        return commandErrorFromBlazeError(result);
    }

};
// static factory method impl
DEFINE_GETCLUBSETTINGSINTERNAL_CREATE()

} // Clubs
} // Blaze
