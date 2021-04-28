// (c) Electronic Arts.  All Rights Reserved.

// main include
#include "framework/blaze.h"

// shield includes
#include "heatshield/heatshieldslaveimpl.h"
#include "heatshield/modules/backendmodule.h"
#include "heatshield/rpc/heatshieldslave/clientchallengematchmaking_stub.h"
#include "heatshield/tdf/heatshieldtypes.h"

namespace Blaze
{
namespace HeatShield
{

class ClientChallengeMatchmakingCommand : public ClientChallengeMatchmakingCommandStub
{
public:
	ClientChallengeMatchmakingCommand(Message* message, ClientChallengeMatchmakingRequest* request, HeatShieldSlaveImpl* componentImpl)
        : ClientChallengeMatchmakingCommandStub(message, request), m_component(componentImpl) { }

    virtual ~ClientChallengeMatchmakingCommand() override { }

private:
	ClientChallengeMatchmakingCommandStub::Errors execute() override
	{
		if (gCurrentUserSession != NULL && mRequest.getGameTypeName() != NULL)
		{
			// For Matchmaking, we only receive the integer identifier for the game mode as the GAME_MODE matchmaking scenario attribute.
			// Prepend gameType to the string here for consistency with MatchEnd game type names.
			eastl::string gameTypeNameFull = "gameType";
			gameTypeNameFull.append(mRequest.getGameTypeName());

			BackendModule backendModule(m_component);
			BlazeRpcError errorCode = backendModule.clientChallenge(gCurrentUserSession->getUserId(), gCurrentUserSession->getAccountId(), gCurrentUserSession->getUserSessionId(),
				gCurrentUserSession->getClientPlatform(), gCurrentUserSession->getClientVersion(), CLIENT_CHALLENGE_MATCHMAKING, gameTypeNameFull.c_str(), &mResponse.getData());
			return commandErrorFromBlazeError(errorCode);
		}
		else
		{
			return commandErrorFromBlazeError(Blaze::ERR_SYSTEM);
		}
	}

private:
    // Not owned memory.
    HeatShieldSlaveImpl* m_component;
};

// static factory method impl
DEFINE_CLIENTCHALLENGEMATCHMAKING_CREATE()

} // HeatShield
} // Blaze
