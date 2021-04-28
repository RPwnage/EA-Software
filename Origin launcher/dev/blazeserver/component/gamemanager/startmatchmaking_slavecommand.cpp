/*! ************************************************************************************************/
/*!
    \file startmatchmaking_slavecommand.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/connectutil.h"
#include "framework/util/random.h"
#include "framework/controller/controller.h"
#include "framework/usersessions/usersessionmanager.h"

#include "gamemanager/rpc/gamemanagerslave/getmatchmakingconfig_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"
#include "gamemanager/gamemanagervalidationutils.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"
#include "gamemanager/gamemanagerhelperutils.h"

namespace Blaze
{
namespace GameManager
{

    // method to convert a MM component error to a GM component error
    BlazeRpcError convertMatchmakingComponentErrorToGameManagerError(const BlazeRpcError &mmRpcError);

    /*! ************************************************************************************************/
    /*! \brief Get the matchmaking configuration from the Matchmaker component;
    ***************************************************************************************************/
    class GetMatchmakingConfigCommand : public GetMatchmakingConfigCommandStub
    {
    public:
        GetMatchmakingConfigCommand(Message* message, GameManagerSlaveImpl* componentImpl)
            :   GetMatchmakingConfigCommandStub(message), mComponent(*componentImpl)
        {
        }

        ~GetMatchmakingConfigCommand() override {}

    private:

        GetMatchmakingConfigCommandStub::Errors execute() override
        {
            BlazeRpcError error;
            Blaze::Matchmaker::MatchmakerSlave* matchmakerComponent = 
                static_cast<Blaze::Matchmaker::MatchmakerSlave*>(gController->getComponent(Blaze::Matchmaker::MatchmakerSlave::COMPONENT_ID, false, true, &error));
            if (matchmakerComponent == nullptr)
            {
                ERR_LOG("[GetMatchmakingConfigCommand].execute() - Unable to resolve matchmaker component.");
                return GetMatchmakingConfigCommandStub::ERR_SYSTEM;
            }
            BlazeRpcError err = matchmakerComponent->getMatchmakingConfig(mResponse);
            return commandErrorFromBlazeError(convertMatchmakingComponentErrorToGameManagerError(err));
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    BlazeRpcError convertMatchmakingComponentErrorToGameManagerError(const BlazeRpcError &mmRpcError)
    {
        switch(mmRpcError)
        {

        case ERR_OK:
            return ERR_OK;
        case ERR_TIMEOUT:
            return ERR_TIMEOUT;
        case ERR_GUEST_SESSION_NOT_ALLOWED:
            return ERR_GUEST_SESSION_NOT_ALLOWED;
        case ERR_AUTHORIZATION_REQUIRED:
            return ERR_AUTHORIZATION_REQUIRED;
        case ERR_AUTHENTICATION_REQUIRED:
            return ERR_AUTHENTICATION_REQUIRED;
        case MATCHMAKER_ERR_INVALID_MATCHMAKING_CRITERIA:
            return GAMEMANAGER_ERR_INVALID_MATCHMAKING_CRITERIA;
        case MATCHMAKER_ERR_UNKNOWN_MATCHMAKING_SESSION_ID:
            return GAMEMANAGER_ERR_UNKNOWN_MATCHMAKING_SESSION_ID;
        case MATCHMAKER_ERR_NOT_MATCHMAKING_SESSION_OWNER:
            return GAMEMANAGER_ERR_NOT_MATCHMAKING_SESSION_OWNER;
        case MATCHMAKER_ERR_MATCHMAKING_NO_JOINABLE_GAMES:
            return GAMEMANAGER_ERR_MATCHMAKING_NO_JOINABLE_GAMES;
        case MATCHMAKER_ERR_MATCHMAKING_USERSESSION_NOT_FOUND:
            return GAMEMANAGER_ERR_MATCHMAKING_USERSESSION_NOT_FOUND;
        case MATCHMAKER_ERR_MATCHMAKING_EXCEEDED_MAX_REQUESTS:
            return GAMEMANAGER_ERR_MATCHMAKING_EXCEEDED_MAX_REQUESTS;
        case MATCHMAKER_ERR_INVALID_GAME_ENTRY_TYPE:
            return GAMEMANAGER_ERR_INVALID_GAME_ENTRY_TYPE;
        case MATCHMAKER_ERR_INVALID_GROUP_ID:
            return GAMEMANAGER_ERR_INVALID_GROUP_ID;
        case MATCHMAKER_ERR_PLAYER_NOT_IN_GROUP:
            return GAMEMANAGER_ERR_PLAYER_NOT_IN_GROUP;
        case MATCHMAKER_ERR_GAME_CAPACITY_TOO_SMALL:
            return GAMEMANAGER_ERR_GAME_CAPACITY_TOO_SMALL;
        case MATCHMAKER_ERR_INVALID_GAME_ENTRY_CRITERIA:
            return GAMEMANAGER_ERR_INVALID_GAME_ENTRY_CRITERIA;
        case MATCHMAKER_ERR_ROLE_NOT_ALLOWED:
            return GAMEMANAGER_ERR_ROLE_NOT_ALLOWED;
        case MATCHMAKER_ERR_SESSION_TEMPLATE_NOT_SUPPORTED:
            return GAMEMANAGER_ERR_SESSION_TEMPLATE_NOT_SUPPORTED;
        case ERR_SYSTEM:
            return ERR_SYSTEM;
        default:
            // caught an unrecognized error code, pass it through, but log an error:
            ERR_LOG("[MatchmakingCommand:].convertMatchmakingComponentErrorToGameManagerError() - Unrecognized error '" << ErrorHelp::getErrorName(mmRpcError) 
                << "' returned from matchmaker component.");
            return mmRpcError;
        }
    }
    //! \brief static creation factory method for command (from stub)
    DEFINE_GETMATCHMAKINGCONFIG_CREATE()
} // namespace GameManager
} // namespace Blaze
