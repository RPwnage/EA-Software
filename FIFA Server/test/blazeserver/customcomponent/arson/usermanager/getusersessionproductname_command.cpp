
/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/getusersessionproductname_stub.h"

namespace Blaze
{
namespace Arson
{
class GetUserSessionProductNameCommand : public GetUserSessionProductNameCommandStub
{
public:
    GetUserSessionProductNameCommand(Message* message, ArsonSlaveImpl* componentImpl)
        : GetUserSessionProductNameCommandStub(message),
        mComponent(componentImpl)
    {
    }
    ~GetUserSessionProductNameCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    GetUserSessionProductNameCommandStub::Errors execute() override
    {
        if(gCurrentUserSession == nullptr)
        {
            ERR_LOG("[GetUserSessionProductNameCommandStub].execute: No current usersession.");
            return ERR_SYSTEM;
        }

        mResponse.setProductName(gCurrentUserSession->getProductName());     
        return ERR_OK;
    }
};

DEFINE_GETUSERSESSIONPRODUCTNAME_CREATE()

} //Arson
} //Blaze
