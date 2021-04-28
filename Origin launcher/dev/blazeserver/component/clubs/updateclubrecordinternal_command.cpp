/*************************************************************************************************/
/*!
    \file   updateclubrecordinternal_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdateClubRecordInternal

    Update club record.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "clubs/rpc/clubsslave/updateclubrecordinternal_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"
#include "clubsconstants.h"

namespace Blaze
{
namespace Clubs
{

class UpdateClubRecordInternalCommand : public UpdateClubRecordInternalCommandStub, private ClubsCommandBase
{
public:
    UpdateClubRecordInternalCommand(Message* message, UpdateClubRecordRequest* request, ClubsSlaveImpl* componentImpl)
        : UpdateClubRecordInternalCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~UpdateClubRecordInternalCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    UpdateClubRecordInternalCommandStub::Errors execute() override
    {
        BlazeRpcError result = Blaze::ERR_OK;

        TransactionContextPtr ptr = mComponent->getTransactionContext(mRequest.getTransactionContextId());
        if (ptr == nullptr)
        {
            TRACE_LOG("[UpdateClubRecordInternalCommand] cannot obtain transaction for transaction context id[" << mRequest.getTransactionContextId() <<"].");
            return ERR_SYSTEM;
        }


        DbConnPtr dbPtr = static_cast<ClubsTransactionContext&>(*ptr).getDatabaseConnection(); 

        ClubRecordbookRecord record;

        record.clubId = mRequest.getClubRecordbook().getClubId();
        record.recordId = mRequest.getClubRecordbook().getRecordId();
        record.blazeId = mRequest.getClubRecordbook().getUser().getBlazeId();
        record.statType = mRequest.getClubRecordbook().getStatType();
        record.statValueInt = mRequest.getClubRecordbook().getStatValueInt();
        record.statValueFloat = mRequest.getClubRecordbook().getStatValueFloat();
        record.persona = mRequest.getClubRecordbook().getUser().getName();
        record.lastUpdate = mRequest.getClubRecordbook().getLastUpdate();

        result = mComponent->updateClubRecord(dbPtr, record);

        return commandErrorFromBlazeError(result);
    }

};
// static factory method impl
DEFINE_UPDATECLUBRECORDINTERNAL_CREATE()

} // Clubs
} // Blaze
