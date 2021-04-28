/*************************************************************************************************/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "statsslaveimpl.h"
#include "statsconfig.h"
#include "stats/rpc/statsslave/commitstatstransaction_stub.h"


namespace Blaze
{
namespace Stats
{

class CommitStatsTransactionCommand : public CommitStatsTransactionCommandStub
{
public:
    CommitStatsTransactionCommand(Message *message, CommitTransactionRequest *request, StatsSlaveImpl* componentImpl) 
    : CommitStatsTransactionCommandStub(message, request),
      mComponent(componentImpl)
    {
    }

    CommitStatsTransactionCommandStub::Errors execute() override
    {
        BlazeRpcError result = Blaze::ERR_OK;

        uint64_t transId = mRequest.getTransactionContextId();

        // can't have more than one reference to a transaction context outside of the transaction context map;
        // therefore, we enclose transPtr so that the ref count properly decrements when transPtr falls out of scope
        // and allow the commitTransaction() call to acquire a reference
        {
            TransactionContextPtr transPtr = mComponent->getTransactionContext(transId);
            if (transPtr == nullptr)
            {
                WARN_LOG("[CommitStatsTransactionCommand] transaction ID(" << transId << ") does not exist");
                return ERR_SYSTEM;
            }

            StatsTransactionContext *transData = &static_cast<StatsTransactionContext&>(*transPtr);

            // request can be empty when only global stats update is broadcasted
            if (transData->getIncomingRequest().empty() && transData->getGlobalStats().getStatUpdates().empty())
            {
                return ERR_SYSTEM;
            }

            FetchedRowUpdateList& cacheList = mRequest.getCacheRowList();
            transData->updateProviderRows(cacheList);
        }

        result = mComponent->commitTransaction(transId); 

        return commandErrorFromBlazeError(result);
    }
private:
    StatsSlaveImpl* mComponent;
};

DEFINE_COMMITSTATSTRANSACTION_CREATE()

} // Stats
} // Blaze
