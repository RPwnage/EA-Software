/*************************************************************************************************/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "statsslaveimpl.h"
#include "statsconfig.h"
#include "stats/rpc/statsslave/calcderivedstats_stub.h"
#include "updatestatshelper.h"

namespace Blaze
{
namespace Stats
{

class CalcDerivedStatsCommand : public CalcDerivedStatsCommandStub
{
public:
    CalcDerivedStatsCommand(Message *message, CalcDerivedStatsRequest *request, StatsSlaveImpl* componentImpl) 
    : CalcDerivedStatsCommandStub(message, request),
      mComponent(componentImpl)
    {
    }

    CalcDerivedStatsCommandStub::Errors execute() override
    {
        BlazeRpcError error = Blaze::ERR_OK;

        uint64_t transId = mRequest.getTransactionContextId();
        TransactionContextPtr transPtr = mComponent->getTransactionContext(transId);
        if (transPtr == nullptr)
        {
            WARN_LOG("[CalcDerivedStatsCommand].execute: transaction Id: (" << transId << ") does not exist.");
            return ERR_SYSTEM;
        }

        StatsTransactionContext *transData = &static_cast<StatsTransactionContext&>(*transPtr);

        FetchedRowUpdateList& cacheList = mRequest.getCacheRowList();
        transData->updateProviderRows(cacheList);

        bool applyUpdates = false;  // Only do the core update if it hasn't been done before. 
        bool applyDerived = true;   // We always apply the derived update here, even if it's already been done.
        if ((transData->getUpdateState() & APPLIED_UPDATES) == 0)
        {
            applyUpdates = true;
        }

        error = transData->calcStatsChange(applyUpdates, applyDerived);

        return (commandErrorFromBlazeError(error));
    }
private:
    StatsSlaveImpl* mComponent;
};

DEFINE_CALCDERIVEDSTATS_CREATE()

} // Stats
} // Blaze
