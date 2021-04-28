
/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

#include "authentication/authenticationimpl.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/updateentitlement_stub.h"

namespace Blaze
{
namespace Arson
{
class UpdateEntitlementCommand : public UpdateEntitlementCommandStub
{
public:
    UpdateEntitlementCommand(Message* message, Blaze::Arson::UpdateEntitlementREQ* request, ArsonSlaveImpl* componentImpl)
        : UpdateEntitlementCommandStub(message, request),
        mComponent(componentImpl)
    {
    }
    ~UpdateEntitlementCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    UpdateEntitlementCommandStub::Errors execute() override
    {
        Blaze::Authentication::AuthenticationSlaveImpl* authComponent = 
            static_cast<Blaze::Authentication::AuthenticationSlaveImpl*>(gController->getComponent(Blaze::Authentication::AuthenticationSlave::COMPONENT_ID));

        if(authComponent == nullptr)
        {
            WARN_LOG("[UpdateEntitlementCommand].execute() - authentication component is not available");
            return ERR_SYSTEM;
        }

        Authentication::ModifyEntitlement2Request req;
        req.setEntitlementId(mRequest.getEntitlementId());
        req.setStatus(mRequest.getEntitlementStatusCode());
        req.setStatusReasonCode(mRequest.getStatusReasonCode());
        req.setVersion(mRequest.getVersion());
        req.setTermination(mRequest.getTerminationDate());

        return commandErrorFromBlazeError(authComponent->modifyEntitlement2(req));
    }

    static Errors arsonErrorFromAuthenticationError(BlazeRpcError error);
};

DEFINE_UPDATEENTITLEMENT_CREATE()

} //Arson
} //Blaze
