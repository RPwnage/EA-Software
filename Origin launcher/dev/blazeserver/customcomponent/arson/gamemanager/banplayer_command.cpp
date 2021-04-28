/*************************************************************************************************/
/*!
    \file   banplayer_command.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/banplayer_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"

namespace Blaze
{
namespace Arson
{
class BanPlayerCommand : public BanPlayerCommandStub
{
public:
    BanPlayerCommand(
        Message* message, Blaze::Arson::BanPlayerRequest* request, ArsonSlaveImpl* componentImpl)
        : BanPlayerCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~BanPlayerCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    BanPlayerCommandStub::Errors execute() override
    {
        GameManager::GameManagerSlave* gameManagerComponent = nullptr;
        BlazeRpcError error;
        gameManagerComponent = static_cast<GameManager::GameManagerSlave*>(gController->getComponent(GameManager::GameManagerSlave::COMPONENT_ID, false, true, &error));

        if (gameManagerComponent == nullptr)
        {
            return ARSON_ERR_GAMEMANAGER_COMPONENT_NOT_FOUND;
        }

        if(mRequest.getNullCurrentUserSession())
        {
            // Set the gCurrentUserSession to nullptr
            UserSession::setCurrentUserSessionId(INVALID_USER_SESSION_ID);
        }

        UserSession::SuperUserPermissionAutoPtr autoPtr(true);
        BlazeRpcError err = gameManagerComponent->banPlayer(mRequest.getBanPlayerRequest());

        return arsonErrorFromGameManagerError(err);
    }

    static Errors arsonErrorFromGameManagerError(BlazeRpcError error);
};

DEFINE_BANPLAYER_CREATE()

BanPlayerCommandStub::Errors BanPlayerCommand::arsonErrorFromGameManagerError(BlazeRpcError error)
{
    Errors result = ERR_SYSTEM;
    switch (error)
    {
        case ERR_OK: result = ERR_OK; break;
        case ERR_SYSTEM: result = ERR_SYSTEM; break;
        case ERR_TIMEOUT: result = ERR_TIMEOUT; break;
        case ERR_AUTHORIZATION_REQUIRED: result = ERR_AUTHORIZATION_REQUIRED; break;
        case ERR_AUTHENTICATION_REQUIRED: result = ERR_AUTHENTICATION_REQUIRED; break;
        case ARSON_ERR_GAMEMANAGER_COMPONENT_NOT_FOUND: result = ARSON_ERR_GAMEMANAGER_COMPONENT_NOT_FOUND; break;
        case GAMEMANAGER_ERR_PLAYER_NOT_FOUND: result = ARSON_ERR_PLAYER_NOT_FOUND; break;
        case GAMEMANAGER_ERR_INVALID_GAME_ID: result = ARSON_ERR_INVALID_GAME_ID; break;
        case GAMEMANAGER_ERR_PERMISSION_DENIED: result = ARSON_ERR_PERMISSION_DENIED; break;
        case GAMEMANAGER_ERR_REMOVE_PLAYER_FAILED: result = ARSON_ERR_REMOVE_PLAYER_FAILED; break;
        case GAMEMANAGER_ERR_INVALID_PLAYER_PASSEDIN: result = ARSON_ERR_INVALID_PLAYER_PASSEDIN; break;
        case GAMEMANAGER_ERR_BANNED_LIST_MAX: result = ARSON_ERR_BANNED_LIST_MAX; break;
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[BanPlayerCommand].arsonErrorFromGameManagerError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
