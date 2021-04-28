/*************************************************************************************************/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "statsslaveimpl.h"
#include "statsconfig.h"
#include "stats/rpc/statsslave/initializestatstransaction_stub.h"
#include "updatestatshelper.h"

namespace Blaze
{
namespace Stats
{

class InitializeStatsTransactionCommand : public InitializeStatsTransactionCommandStub
{
public:
    InitializeStatsTransactionCommand(Message *message, InitializeStatsTransactionRequest *request, StatsSlaveImpl* componentImpl) 
    : InitializeStatsTransactionCommandStub(message, request),
      mComponent(componentImpl)
    {
    }

    InitializeStatsTransactionCommandStub::Errors execute() override
    {
        BlazeRpcError error = Blaze::ERR_OK;
        if (mComponent == nullptr)
            return ERR_SYSTEM;

        TimeValue timeout;
        if (mRequest.getTimeout() == 0)
        {
            timeout.setSeconds(static_cast<int64_t>(TRANSACTION_TIMEOUT_DEFAULT));
        }
        else
        {
            timeout.setSeconds(static_cast<int64_t>(mRequest.getTimeout()));
        }

        TransactionContextHandlePtr contextHandlePtr; 
        BlazeRpcError result = mComponent->startTransaction(contextHandlePtr, 0, timeout);
        if (result != Blaze::ERR_OK || contextHandlePtr == nullptr)
        {
            WARN_LOG("[InitializeStatsTransactionCommand] unable to start transaction for " << mComponent->getName());
            return ERR_SYSTEM;
        }
        
        uint64_t transId = *contextHandlePtr;
        mResponse.setTransactionContextId(transId);

        // can't have more than one reference to a transaction context outside of the transaction context map;
        // therefore, we enclose transPtr so that the ref count properly decrements when transPtr falls out of scope
        // and allow the rollbackTransaction() call to acquire a reference
        {
            TransactionContextPtr transPtr = mComponent->getTransactionContext(transId);
            if (transPtr == nullptr)
            {
                WARN_LOG("[InitializeStatsTransactionCommand] transaction ID(" << transId << ") does not exist");
                return ERR_SYSTEM;
            }

            StatsTransactionContext *transData = &static_cast<StatsTransactionContext&>(*transPtr);

            transData->reset();
            transData->setProcessGlobalStats(mRequest.getGlobalStats());
            transData->setTimeout(mRequest.getTimeout());
            // Make our own copy as we will be making modifications
            mRequest.getStatUpdates().copyInto(transData->getIncomingRequest());
            // After taking our copy of the request, we now may have to add additional rows as a result of
            // multi-category derived stats
            if (mComponent->getConfigData()->hasMultiCategoryDerivedStats())
            {
                error = transData->preprocessMultiCategoryDerivedStats();
            }

            // Validate requested update
            if (error == ERR_OK)
            {
                error = transData->validateRequest(mRequest.getStrict());
            }
            if (error == ERR_OK)
            {
                //initialize stats provider.
                transData->initializeStatsProvider();

                transData->setUpdateState(transData->getUpdateState() | INITIALIZED);

                // this handle is remote and not bound to local fibers
                contextHandlePtr->clearOwningFiber();
                // this handle's lifetime will not be associated with the transaction
                contextHandlePtr->dissociate();

                return (commandErrorFromBlazeError(error));
            }
        }

        result = mComponent->rollbackTransaction(transId);
        WARN_LOG("[InitializeStatsTransactionCommand] failed: " << ErrorHelp::getErrorName(error)
            << "; Rollback attempt returned: " << ErrorHelp::getErrorName(result) );
        return (commandErrorFromBlazeError(error));
    }

private:
    StatsSlaveImpl* mComponent;
};

DEFINE_INITIALIZESTATSTRANSACTION_CREATE()

} // Stats
} // Blaze
