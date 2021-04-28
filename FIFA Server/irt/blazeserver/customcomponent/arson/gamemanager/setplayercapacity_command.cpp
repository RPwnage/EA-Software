/*************************************************************************************************/
/*!
    \file   setplayercapacity_command.cpp

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
#include "arson/rpc/arsonslave/setplayercapacity_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"

namespace Blaze
{
namespace Arson
{
class SetPlayerCapacityCommand : public SetPlayerCapacityCommandStub
{
public:
    SetPlayerCapacityCommand(
        Message* message, Blaze::Arson::SetPlayerCapacityRequest* request, ArsonSlaveImpl* componentImpl)
        : SetPlayerCapacityCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~SetPlayerCapacityCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    SetPlayerCapacityCommandStub::Errors execute() override
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

        Blaze::GameManager::SwapPlayersErrorInfo errorInfo;
        UserSession::SuperUserPermissionAutoPtr autoPtr(true);
        
        BlazeRpcError err = gameManagerComponent->setPlayerCapacity(mRequest.getSetPlayerCapacityRequest(), errorInfo);

        return arsonErrorFromGameManagerError(err);
    }

    static Errors arsonErrorFromGameManagerError(BlazeRpcError error);
};

DEFINE_SETPLAYERCAPACITY_CREATE()

SetPlayerCapacityCommandStub::Errors SetPlayerCapacityCommand::arsonErrorFromGameManagerError(BlazeRpcError error)
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
        case GAMEMANAGER_ERR_INVALID_GAME_ID: result = ARSON_ERR_INVALID_GAME_ID; break;
        case GAMEMANAGER_ERR_INVALID_GAME_STATE_ACTION: result = ARSON_ERR_INVALID_GAME_STATE_ACTION; break;
        case GAMEMANAGER_ERR_PLAYER_CAPACITY_TOO_SMALL: result = ARSON_ERR_PLAYER_CAPACITY_TOO_SMALL; break;
        case GAMEMANAGER_ERR_PLAYER_CAPACITY_TOO_LARGE: result = ARSON_ERR_PLAYER_CAPACITY_TOO_LARGE; break;
        case GAMEMANAGER_ERR_PLAYER_CAPACITY_IS_ZERO: result = ARSON_ERR_PLAYER_CAPACITY_IS_ZERO; break;
        case GAMEMANAGER_ERR_PERMISSION_DENIED: result = ARSON_ERR_PERMISSION_DENIED; break;
        case GAMEMANAGER_ERR_INVALID_TEAM_CAPACITIES_VECTOR_SIZE: result = ARSON_ERR_INVALID_TEAM_CAPACITIES_VECTOR_SIZE; break;
        case GAMEMANAGER_ERR_DUPLICATE_TEAM_CAPACITY: result = ARSON_ERR_DUPLICATE_TEAM_CAPACITY; break;
        case GAMEMANAGER_ERR_INVALID_TEAM_ID_IN_TEAM_CAPACITIES_VECTOR: result = ARSON_ERR_INVALID_TEAM_ID_IN_TEAM_CAPACITIES_VECTOR; break;
        case GAMEMANAGER_ERR_TEAM_NOT_ALLOWED: result = ARSON_ERR_TEAM_NOT_ALLOWED; break;
        case GAMEMANAGER_ERR_TOTAL_TEAM_CAPACITY_INVALID: result = ARSON_ERR_TOTAL_TEAM_CAPACITY_INVALID; break;
        case GAMEMANAGER_ERR_PLAYER_CAPACITY_NOT_EVENLY_DIVISIBLE_BY_TEAMS: result = ARSON_ERR_INVALID_TEAM_CAPACITY; break;
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[SetPlayerCapacityCommand].arsonErrorFromGameManagerError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
