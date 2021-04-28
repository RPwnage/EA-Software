/*************************************************************************************************/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "statsslaveimpl.h"
#include "statsconfig.h"
#include "stats/rpc/statsslave/abortstatstransaction_stub.h"
#include "updatestatshelper.h"

namespace Blaze
{
namespace Stats
{

class AbortStatsTransactionCommand : public AbortStatsTransactionCommandStub
{
public:
    AbortStatsTransactionCommand(Message *message, AbortTransactionRequest *request, StatsSlaveImpl* componentImpl) 
    : AbortStatsTransactionCommandStub(message, request),
      mComponent(componentImpl)
    {
    }

    AbortStatsTransactionCommandStub::Errors execute() override
    {
        BlazeRpcError error = Blaze::ERR_OK;

        uint64_t transId = mRequest.getTransactionContextId();
        error = mComponent->rollbackTransaction(transId); 
        
        return (commandErrorFromBlazeError(error));
    }
private:
    StatsSlaveImpl* mComponent;
};

DEFINE_ABORTSTATSTRANSACTION_CREATE()

} // Stats
} // Blaze
