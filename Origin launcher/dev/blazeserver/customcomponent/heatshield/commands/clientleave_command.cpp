// (c) Electronic Arts.  All Rights Reserved.

// main include
#include "framework/blaze.h"

// heat shield includes
#include "heatshield/heatshieldslaveimpl.h"
#include "heatshield/modules/backendmodule.h"
#include "heatshield/rpc/heatshieldslave/clientleave_stub.h"
#include "heatshield/tdf/heatshieldtypes.h"

namespace Blaze
{
namespace HeatShield
{

class ClientLeaveCommand : public ClientLeaveCommandStub
{
public:
	ClientLeaveCommand(Message* message, ClientLeaveRequest* request, HeatShieldSlaveImpl* componentImpl)
		: ClientLeaveCommandStub(message, request), m_component(componentImpl) { }

	~ClientLeaveCommand() override { }

private:
	ClientLeaveCommandStub::Errors execute() override
	{
		bool isAuthorized = UserSession::isCurrentContextAuthorized(
			Blaze::Authorization::HEAT_SHIELD_PERMISSION_CLIENT_EXISTENCE, false);

		if (isAuthorized)
		{
			if (gCurrentUserSession != NULL)
			{
				BackendModule backendModule(m_component);
				BlazeRpcError errorCode = backendModule.clientLeave(gCurrentUserSession->getUserId(), gCurrentUserSession->getAccountId(), 
					gCurrentUserSession->getUserSessionId(), gCurrentUserSession->getClientPlatform(), gCurrentUserSession->getClientVersion(), mRequest.getFlowType(), &mResponse.getData());
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
DEFINE_CLIENTLEAVE_CREATE()

} // HeatShield
} // Blaze
