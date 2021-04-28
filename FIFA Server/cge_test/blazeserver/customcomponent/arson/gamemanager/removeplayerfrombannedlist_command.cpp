/*************************************************************************************************/
/*!
    \file   removeplayerfrombannedlist_command.cpp

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
#include "arson/rpc/arsonslave/removeplayerfrombannedlist_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"

namespace Blaze
{
namespace Arson
{
class RemovePlayerFromBannedListCommand : public RemovePlayerFromBannedListCommandStub
{
public:
    RemovePlayerFromBannedListCommand(
        Message* message, Blaze::Arson::RemovePlayerFromBannedListRequest* request, ArsonSlaveImpl* componentImpl)
        : RemovePlayerFromBannedListCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~RemovePlayerFromBannedListCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    RemovePlayerFromBannedListCommandStub::Errors execute() override
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
        BlazeRpcError err = gameManagerComponent->removePlayerFromBannedList(mRequest.getRemovePlayerFromBannedListRequest());

        return arsonErrorFromGameManagerError(err);
    }

    static Errors arsonErrorFromGameManagerError(BlazeRpcError error);
};

DEFINE_REMOVEPLAYERFROMBANNEDLIST_CREATE()

RemovePlayerFromBannedListCommandStub::Errors RemovePlayerFromBannedListCommand::arsonErrorFromGameManagerError(BlazeRpcError error)
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
        case GAMEMANAGER_ERR_BANNED_PLAYER_NOT_FOUND: result = ARSON_ERR_BANNED_PLAYER_NOT_FOUND; break;
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[RemovePlayerFromBannedListCommand].arsonErrorFromGameManagerError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
