// (c) Electronic Arts.  All Rights Reserved.

// main include
#include "framework/blaze.h"

// shield includes
#include "heatshield/heatshieldslaveimpl.h"
#include "heatshield/modules/backendmodule.h"
#include "heatshield/rpc/heatshieldslave/clientchallenge_stub.h"
#include "heatshield/tdf/heatshieldtypes.h"

namespace Blaze
{
namespace HeatShield
{

class ClientChallengeCommand : public ClientChallengeCommandStub
{
public:
	ClientChallengeCommand(Message* message, ClientChallengeRequest* request, HeatShieldSlaveImpl* componentImpl)
        : ClientChallengeCommandStub(message, request), m_component(componentImpl) { }

    virtual ~ClientChallengeCommand() override { }

private:
	ClientChallengeCommandStub::Errors execute() override
	{
		bool isAuthorized = UserSession::isCurrentContextAuthorized(
			Blaze::Authorization::HEAT_SHIELD_PERMISSION_CLIENT_EXISTENCE, false);

		if (isAuthorized)
		{
			if (gCurrentUserSession != NULL)
			{
				BackendModule backendModule(m_component);
				BlazeRpcError errorCode = backendModule.clientChallenge(gCurrentUserSession->getUserId(), gCurrentUserSession->getAccountId(), gCurrentUserSession->getUserSessionId(),
					gCurrentUserSession->getClientPlatform(), gCurrentUserSession->getClientVersion(), mRequest.getClientChallengeType(), mRequest.getGameTypeName(), &mResponse.getData());
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
DEFINE_CLIENTCHALLENGE_CREATE()

} // HeatShield
} // Blaze
