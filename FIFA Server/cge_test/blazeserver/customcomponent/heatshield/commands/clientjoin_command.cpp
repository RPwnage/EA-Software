// (c) Electronic Arts.  All Rights Reserved.

// main include
#include "framework/blaze.h"

// shield includes
#include "heatshield/heatshieldslaveimpl.h"
#include "heatshield/modules/backendmodule.h"
#include "heatshield/rpc/heatshieldslave/clientjoin_stub.h"
#include "heatshield/tdf/heatshieldtypes.h"

namespace Blaze
{
namespace HeatShield
{

class ClientJoinCommand : public ClientJoinCommandStub
{
public:
	ClientJoinCommand(Message* message, ClientJoinRequest* request, HeatShieldSlaveImpl* componentImpl)
        : ClientJoinCommandStub(message, request), m_component(componentImpl) { }

    virtual ~ClientJoinCommand() override { }

private:
	ClientJoinCommandStub::Errors execute() override
	{
		bool isAuthorized = UserSession::isCurrentContextAuthorized(
			Blaze::Authorization::HEAT_SHIELD_PERMISSION_CLIENT_EXISTENCE, false);

		if (isAuthorized)
		{
			if (gCurrentUserSession != NULL)
			{
				BackendModule backendModule(m_component);
				BlazeRpcError errorCode = backendModule.clientJoin(gCurrentUserSession->getUserId(), gCurrentUserSession->getAccountId(), gCurrentUserSession->getUserSessionId(),
					mRequest.getPinSessionId(), gCurrentUserSession->getClientPlatform(), gCurrentUserSession->getClientVersion(), mRequest.getFlowType(), &mResponse.getData());
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
DEFINE_CLIENTJOIN_CREATE()

} // HeatShield
} // Blaze
