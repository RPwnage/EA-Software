// (c) Electronic Arts.  All Rights Reserved.

// main include
#include "framework/blaze.h"

// shield includes
#include "heatshield/heatshieldslaveimpl.h"
#include "heatshield/modules/backendmodule.h"
#include "heatshield/rpc/heatshieldslave/heartbeat_stub.h"
#include "heatshield/tdf/heatshieldtypes.h"

namespace Blaze
{
namespace HeatShield
{

class HeartbeatCommand : public HeartbeatCommandStub
{
public:
    HeartbeatCommand(Message* message, HeartbeatRequest* request, HeatShieldSlaveImpl* componentImpl)
        : HeartbeatCommandStub(message, request), m_component(componentImpl) { }

    virtual ~HeartbeatCommand() override { }

private:
    HeartbeatCommandStub::Errors execute() override
    {
		bool isAuthorized = UserSession::isCurrentContextAuthorized(
			Blaze::Authorization::HEAT_SHIELD_PERMISSION_HEARTBEAT, false);

		if (isAuthorized)
		{
			if (gCurrentUserSession != NULL)
			{
				BackendModule backendModule(m_component);
				BlazeRpcError errorCode = backendModule.clientRequest(gCurrentUserSession->getUserId(), gCurrentUserSession->getAccountId(),
					gCurrentUserSession->getUserSessionId(), gCurrentUserSession->getClientPlatform(), gCurrentUserSession->getClientVersion(), mRequest.getData().getData(),
					mRequest.getData().getSize(), &mResponse.getData());
				return commandErrorFromBlazeError(errorCode);
			}
			else
			{
				return commandErrorFromBlazeError(Blaze::ERR_SYSTEM);
			}
		}
		else
		{
			return commandErrorFromBlazeError(Blaze::ERR_AUTHORIZATION_REQUIRED);
		}
    }

private:
    // Not owned memory.
    HeatShieldSlaveImpl* m_component;
};

// static factory method impl
DEFINE_HEARTBEAT_CREATE()

} // HeatShield
} // Blaze
