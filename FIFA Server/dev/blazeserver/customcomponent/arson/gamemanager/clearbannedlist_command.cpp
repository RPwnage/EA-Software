/*************************************************************************************************/
/*!
    \file   clearbannedlist_command.cpp

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
#include "arson/rpc/arsonslave/clearbannedlist_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"

namespace Blaze
{
namespace Arson
{
class ClearBannedListCommand : public ClearBannedListCommandStub
{
public:
    ClearBannedListCommand(
        Message* message, Blaze::Arson::ClearBannedListRequest* request, ArsonSlaveImpl* componentImpl)
        : ClearBannedListCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~ClearBannedListCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    ClearBannedListCommandStub::Errors execute() override
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
        BlazeRpcError err = gameManagerComponent->clearBannedList(mRequest.getBannedListRequest());

        return arsonErrorFromGameManagerError(err);
    }

    static Errors arsonErrorFromGameManagerError(BlazeRpcError error);
};

DEFINE_CLEARBANNEDLIST_CREATE()

ClearBannedListCommandStub::Errors ClearBannedListCommand::arsonErrorFromGameManagerError(BlazeRpcError error)
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
        case GAMEMANAGER_ERR_PERMISSION_DENIED: result = ARSON_ERR_PERMISSION_DENIED; break;
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[ClearBannedListCommand].arsonErrorFromGameManagerError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
