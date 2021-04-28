
/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/connection/socketutil.h"
#include "framework/tdf/userevents_server.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/getusersessionclientip_stub.h"

namespace Blaze
{
namespace Arson
{
class GetUserSessionClientIpCommand : public GetUserSessionClientIpCommandStub
{
public:
    GetUserSessionClientIpCommand(Message* message, ArsonSlaveImpl* componentImpl)
        : GetUserSessionClientIpCommandStub(message),
        mComponent(componentImpl)
    {
    }
    ~GetUserSessionClientIpCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    GetUserSessionClientIpCommandStub::Errors execute() override
    {
        if(gCurrentUserSession == nullptr)
        {
            ERR_LOG("[GetUserSessionClientIpCommandStub].execute: No current usersession.");
            return ERR_SYSTEM;
        }

        uint32_t clientIp = gCurrentUserSession->getExistenceData().getClientIp();

        if (clientIp != 0)
        {
            char8_t addrStr[Login::MAX_IPADDRESS_LEN] = { 0 };
            blaze_snzprintf(addrStr, sizeof(addrStr), "%d.%d.%d.%d", NIPQUAD(clientIp));
            mResponse.setClientIp(addrStr);
            return ERR_OK;
        }
        else
        {
            ERR_LOG("[GetUserSessionClientIpCommandStub].execute: ClientIp shouldn't be 0.");
            return ERR_SYSTEM;
        }        
    }
};

DEFINE_GETUSERSESSIONCLIENTIP_CREATE()

} //Arson
} //Blaze
