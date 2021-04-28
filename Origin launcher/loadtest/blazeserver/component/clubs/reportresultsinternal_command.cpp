/*************************************************************************************************/
/*!
    \file   reportresultsinternal_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ReportResultsInternal

    Report result for club.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "clubs/rpc/clubsslave/reportresultsinternal_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"
#include "clubsconstants.h"

namespace Blaze
{
namespace Clubs
{

class ReportResultsInternalCommand : public ReportResultsInternalCommandStub, private ClubsCommandBase
{
public:
    ReportResultsInternalCommand(Message* message, ReportResultsRequest* request, ClubsSlaveImpl* componentImpl)
        : ReportResultsInternalCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~ReportResultsInternalCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    ReportResultsInternalCommandStub::Errors execute() override
    {
        BlazeRpcError result = Blaze::ERR_OK;

        TransactionContextPtr ptr = mComponent->getTransactionContext(mRequest.getTransactionContextId());
        if (ptr == nullptr)
        {
            TRACE_LOG("[ReportResultsInternalCommand] cannot obtain transaction for transaction context id[" << mRequest.getTransactionContextId() <<"].");
            return ERR_SYSTEM;
        }

        DbConnPtr dbPtr = static_cast<ClubsTransactionContext&>(*ptr).getDatabaseConnection(); 

        result = mComponent->reportResults(dbPtr, mRequest.getUpperClubId(), mRequest.getGameResultForUpperClub(), 
                                            mRequest.getLowerClubId(), mRequest.getGameResultForLowerClub());

        return commandErrorFromBlazeError(result);
    }

};
// static factory method impl
DEFINE_REPORTRESULTSINTERNAL_CREATE()

} // Clubs
} // Blaze
