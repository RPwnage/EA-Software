/*************************************************************************************************/
/*!
    \file   creategame_command.cpp

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
#include "arson/rpc/arsonslave/creategame_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"

namespace Blaze
{
namespace Arson
{
class CreateGameCommand : public CreateGameCommandStub
{
public:
    CreateGameCommand(
        Message* message, Blaze::Arson::ArsonCreateGameRequest* request, ArsonSlaveImpl* componentImpl)
        : CreateGameCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~CreateGameCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    CreateGameCommandStub::Errors execute() override
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
        BlazeRpcError err = gameManagerComponent->createGame(mRequest.getCreateGameRequest(), mResponse);

        return arsonErrorFromGameManagerError(err);

    }

    static Errors arsonErrorFromGameManagerError(BlazeRpcError error);
};

DEFINE_CREATEGAME_CREATE()

CreateGameCommandStub::Errors CreateGameCommand::arsonErrorFromGameManagerError(BlazeRpcError error)
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
        case GAMEMANAGER_ERR_GAME_CAPACITY_TOO_SMALL: result = ARSON_ERR_GAME_CAPACITY_TOO_SMALL; break;
        case GAMEMANAGER_ERR_INVALID_ACTION_FOR_GROUP: result = ARSON_ERR_INVALID_ACTION_FOR_GROUP; break;
        case GAMEMANAGER_ERR_INVALID_GAME_ENTRY_CRITERIA: result = ARSON_ERR_INVALID_GAME_ENTRY_CRITERIA; break;
        case GAMEMANAGER_ERR_PLAYER_CAPACITY_TOO_LARGE: result = ARSON_ERR_PLAYER_CAPACITY_TOO_LARGE; break;
        case GAMEMANAGER_ERR_MAX_PLAYER_CAPACITY_TOO_LARGE: result = ARSON_ERR_MAX_PLAYER_CAPACITY_TOO_LARGE; break;
        case GAMEMANAGER_ERR_PARTICIPANT_SLOTS_FULL: result = ARSON_ERR_GAME_FULL; break;
        case GAMEMANAGER_ERR_SPECTATOR_SLOTS_FULL: result = ARSON_ERR_GAME_FULL; break;
        case GAMEMANAGER_ERR_INVALID_GROUP_ID: result = ARSON_ERR_INVALID_GROUP_ID; break;
        case GAMEMANAGER_ERR_PLAYER_NOT_IN_GROUP: result = ARSON_ERR_PLAYER_NOT_IN_GROUP; break;
        case GAMEMANAGER_ERR_PLAYER_CAPACITY_IS_ZERO: result = ARSON_ERR_PLAYER_CAPACITY_IS_ZERO; break;
        case GAMEMANAGER_ERR_INVALID_TEAM_CAPACITIES_VECTOR_SIZE: result = ARSON_ERR_INVALID_TEAM_CAPACITIES_VECTOR_SIZE; break;
        case GAMEMANAGER_ERR_DUPLICATE_TEAM_CAPACITY: result = ARSON_ERR_DUPLICATE_TEAM_CAPACITY; break;
        case GAMEMANAGER_ERR_INVALID_TEAM_ID_IN_TEAM_CAPACITIES_VECTOR: result = ARSON_ERR_INVALID_TEAM_ID_IN_TEAM_CAPACITIES_VECTOR; break;
        case GAMEMANAGER_ERR_TEAM_NOT_ALLOWED: result = ARSON_ERR_TEAM_NOT_ALLOWED; break;
        case GAMEMANAGER_ERR_TOTAL_TEAM_CAPACITY_INVALID: result = ARSON_ERR_TOTAL_TEAM_CAPACITY_INVALID; break;
        case GAMEMANAGER_ERR_TEAM_FULL: result = ARSON_ERR_TEAM_FULL; break;

        case GAMEMANAGER_ERR_INVALID_GAME_SETTINGS: result = ARSON_ERR_INVALID_GAME_SETTINGS; break;
        case GAMEMANAGER_ERR_INVALID_PERSISTED_GAME_ID_OR_SECRET: result = ARSON_ERR_INVALID_PERSISTED_GAME_ID_OR_SECRET; break;
        case GAMEMANAGER_ERR_PERSISTED_GAME_ID_IN_USE: result = ARSON_ERR_PERSISTED_GAME_ID_IN_USE; break;
        case GAMEMANAGER_ERR_RESERVED_GAME_ID_INVALID: result = ARSON_ERR_RESERVED_GAME_ID_INVALID; break;
        case GAMEMANAGER_ERR_PLAYER_CAPACITY_NOT_EVENLY_DIVISIBLE_BY_TEAMS: result = ARSON_ERR_INVALID_TEAM_CAPACITY; break;
        case GAMEMANAGER_ERR_INVALID_GAME_ENTRY_TYPE: result = ARSON_ERR_INVALID_GAME_ENTRY_TYPE; break;
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[CreateGameCommand].arsonErrorFromGameManagerError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
