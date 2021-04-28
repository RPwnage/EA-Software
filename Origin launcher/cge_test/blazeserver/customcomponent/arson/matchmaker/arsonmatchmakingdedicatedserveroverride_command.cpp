/*************************************************************************************************/
/*!
    \file   arsonmatchmakingdedicatedserveroverride_command.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"

// arson includes
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/arsonmatchmakingdedicatedserveroverride_stub.h"
#include "gamemanager/rpc/gamemanagerslave.h"

namespace Blaze
{
namespace Arson
{

/*! ************************************************************************************************/
/*! \brief Calls the MatchmakingDedicatedServerOverride blaze slave command.
    Provides arson server access to Blazeserver's MatchmakingDedicatedServerOverride (non-SDK) rpc
*************************************************************************************************/
class ArsonMatchmakingDedicatedServerOverrideCommand : public ArsonMatchmakingDedicatedServerOverrideCommandStub
{
public:
    ArsonMatchmakingDedicatedServerOverrideCommand(Message* message,
        Blaze::GameManager::MatchmakingDedicatedServerOverrideRequest* request, ArsonSlaveImpl* componentImpl)
        : ArsonMatchmakingDedicatedServerOverrideCommandStub(message, request),
        mComponent(componentImpl)
    {
    }
    ~ArsonMatchmakingDedicatedServerOverrideCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

private:

    ArsonMatchmakingDedicatedServerOverrideCommandStub::Errors execute() override
    {
        GameManager::GameManagerSlave* gameManagerComponent = nullptr;
        gameManagerComponent = static_cast<GameManager::GameManagerSlave*>(gController->getComponent(GameManager::GameManagerSlave::COMPONENT_ID));

        if (gameManagerComponent == nullptr)
        {
            return ARSON_ERR_GAMEMANAGER_COMPONENT_NOT_FOUND;
        }

        BlazeRpcError err = gameManagerComponent->matchmakingDedicatedServerOverride(mRequest);
        return arsonErrorFromGameManagerError(err);
    }
    static Errors arsonErrorFromGameManagerError(BlazeRpcError error);
};

DEFINE_ARSONMATCHMAKINGDEDICATEDSERVEROVERRIDE_CREATE()

ArsonMatchmakingDedicatedServerOverrideCommandStub::Errors ArsonMatchmakingDedicatedServerOverrideCommand::arsonErrorFromGameManagerError(BlazeRpcError error)
{
    Errors result = ERR_SYSTEM;
    switch (error)
    {
        case ERR_OK: result = ERR_OK; break;
        case ERR_SYSTEM: result = ERR_SYSTEM; break;
        case ERR_TIMEOUT: result = ERR_TIMEOUT; break;
        case ERR_AUTHORIZATION_REQUIRED: result = ERR_AUTHORIZATION_REQUIRED; break;
        case ARSON_ERR_GAMEMANAGER_COMPONENT_NOT_FOUND: result = ARSON_ERR_GAMEMANAGER_COMPONENT_NOT_FOUND; break;
        
        case GAMEMANAGER_ERR_INVALID_GAME_ID: result = ARSON_ERR_INVALID_GAME_ID; break;
        case GAMEMANAGER_ERR_PLAYER_NOT_FOUND: result = ARSON_ERR_PLAYER_NOT_FOUND; break;
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[ArsonMatchmakingDedicatedServerOverrideCommand].arsonErrorFromGameManagerError: unexpected error(0x" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
