/*************************************************************************************************/
/*!
    \file   arsonmatchmakingfillserveroverride_command.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"

// arson includes
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/arsonmatchmakingfillserversoverride_stub.h"
#include "gamemanager/rpc/gamemanagerslave.h"

namespace Blaze
{
namespace Arson
{

/*! ************************************************************************************************/
/*! \brief Calls the MatchmakingFillServersOverride blaze slave command.
    Provides arson server access to Blazeserver's MatchmakingFillServersOverride (non-SDK) rpc
*************************************************************************************************/
class ArsonMatchmakingFillServersOverrideCommand : public ArsonMatchmakingFillServersOverrideCommandStub
{
public:
    ArsonMatchmakingFillServersOverrideCommand(Message* message,
        Blaze::GameManager::MatchmakingFillServersOverrideList* request, ArsonSlaveImpl* componentImpl)
        : ArsonMatchmakingFillServersOverrideCommandStub(message, request),
        mComponent(componentImpl)
    {
    }
    ~ArsonMatchmakingFillServersOverrideCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

private:

    ArsonMatchmakingFillServersOverrideCommandStub::Errors execute() override
    {
        GameManager::GameManagerSlave* gameManagerComponent = nullptr;
        gameManagerComponent = static_cast<GameManager::GameManagerSlave*>(gController->getComponent(GameManager::GameManagerSlave::COMPONENT_ID));

        if (gameManagerComponent == nullptr)
        {
            return ARSON_ERR_GAMEMANAGER_COMPONENT_NOT_FOUND;
        }
        BlazeRpcError err = gameManagerComponent->matchmakingFillServersOverride(mRequest);
        return arsonErrorFromGameManagerError(err);
    }
    static Errors arsonErrorFromGameManagerError(BlazeRpcError error);
};

DEFINE_ARSONMATCHMAKINGFILLSERVERSOVERRIDE_CREATE()

ArsonMatchmakingFillServersOverrideCommandStub::Errors ArsonMatchmakingFillServersOverrideCommand::arsonErrorFromGameManagerError(BlazeRpcError error)
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
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[ArsonMatchmakingFilldServersOverrideCommand].arsonErrorFromGameManagerError: unexpected error(0x" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
