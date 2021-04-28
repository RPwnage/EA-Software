/*! ************************************************************************************************/
/*!
    \file startmatchmakingscenario_slavecommand.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/connectutil.h"
#include "framework/util/random.h"
#include "framework/controller/controller.h"
#include "framework/usersessions/usersessionmanager.h"

#include "gamemanager/rpc/gamemanagerslave/startmatchmakingscenario_stub.h"
#include "gamemanager/rpc/gamemanagerslave/cancelmatchmakingscenario_stub.h"
#include "gamemanager/rpc/gamemanagerslave/startpseudomatchmakingscenario_stub.h"
#include "gamemanager/rpc/gamemanagerslave/getscenarioattributesconfig_stub.h"
#include "gamemanager/rpc/gamemanagerslave/getscenariodetails_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"
#include "gamemanager/gamemanagervalidationutils.h"
#include "gamemanager/scenario.h"
#include "gamemanager/rpc/gamemanagerslave/getscenariosandattributes_stub.h"


namespace Blaze
{
namespace GameManager
{

    // method to convert a MM component error to a GM component error
    BlazeRpcError convertMatchmakingComponentErrorToGameManagerError(const BlazeRpcError &mmRpcError);

    /*! ************************************************************************************************/
    /*! \brief Start a matchmaking scenario; on success we register for async notifications from the session.
    ***************************************************************************************************/
    class StartMatchmakingScenarioCommand : public StartMatchmakingScenarioCommandStub
    {
    public:
        StartMatchmakingScenarioCommand(Message* message, StartMatchmakingScenarioRequest* request, GameManagerSlaveImpl* componentImpl)
            :   StartMatchmakingScenarioCommandStub(message, request), mComponent(*componentImpl)
        {
        }

        ~StartMatchmakingScenarioCommand() override {}

    private:
        
        StartMatchmakingScenarioCommandStub::Errors execute() override
        {
            // Convert the Player Join Data (for back compat)
            for (auto playerJoinData : mRequest.getPlayerJoinData().getPlayerDataList())
                convertToPlatformInfo(playerJoinData->getUser());

            BlazeRpcError err = mComponent.getScenarioManager().validateMaxActiveMMScenarios(UserSession::getCurrentUserSessionId());
            if (err != Blaze::ERR_OK)
            {
                WARN_LOG("[StartMatchmakingScenarioCommand].execute(): Too many scenarios are active for session id "<< UserSession::getCurrentUserSessionId() << ", Error '" << ErrorHelp::getErrorName(err) << "'.");
                gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(err), RiverPoster::matchmaking_failed_to_start);
                return commandErrorFromBlazeError(err);;
            }

            err = mComponent.getScenarioManager().createScenario(mRequest, mResponse, mErrorResponse, getPeerInfo()->getPeerAddress());
            if (err != Blaze::ERR_OK)
            {
                ERR_LOG("[StartMatchmakingScenarioCommand].execute(): Problem creating Scenario, got error '"<< ErrorHelp::getErrorName(err) << "'.");
                
                if (err != ERR_COULDNT_CONNECT)
                {
                    gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(err), RiverPoster::matchmaking_failed_to_start);
                }
                return commandErrorFromBlazeError(err);
            }

            return commandErrorFromBlazeError(ERR_OK);
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    //! \brief static creation factory method for command (from stub)
    DEFINE_STARTMATCHMAKINGSCENARIO_CREATE()

/*! ************************************************************************************************/
    /*! \brief Start a pseudo matchmaking scenario; on success we register for async notifications from the session.
    ***************************************************************************************************/
    class StartPseudoMatchmakingScenarioCommand : public StartPseudoMatchmakingScenarioCommandStub
    {
    public:
        StartPseudoMatchmakingScenarioCommand(Message* message, StartPseudoMatchmakingScenarioRequest* request, GameManagerSlaveImpl* componentImpl)
            :   StartPseudoMatchmakingScenarioCommandStub(message, request), mComponent(*componentImpl)
        {
        }

        ~StartPseudoMatchmakingScenarioCommand() override {}

    private:
        
        StartPseudoMatchmakingScenarioCommandStub::Errors execute() override
        {
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_START_DEBUG_MATCHMAKING_SCENARIO))
            {
                return ERR_AUTHORIZATION_REQUIRED;
            }

            // verify that the request is for a pseudo request- don't allow non-pseudo requests!
            const char8_t* pseudoRequestAttrName = mComponent.getScenarioManager().getGlobalScenarioAttributeNameByTdfMemberName(
                "StartMatchmakingRequest.sessionData.pseudoRequest");

            if (pseudoRequestAttrName == nullptr)
            {
                pseudoRequestAttrName = mComponent.getScenarioManager().getGlobalScenarioAttributeNameByTdfMemberName(
                    "Blaze::GameManager::StartMatchmakingRequest.sessionData.pseudoRequest");
            }

            if (pseudoRequestAttrName == nullptr)
            {
                // pseudo request attribute isn't defined in the configuration!
                ERR_LOG("[StartPseudoMatchmakingScenarioCommand].execute(): Problem creating Scenario, there is no pseudoRequest attribute configured for scenario '"
                    << mRequest.getStartMatchmakingScenarioRequest().getScenarioName() << "'.");
                return ERR_SYSTEM;
            }

            ScenarioAttributes::const_iterator pseudoRequestIter = mRequest.getStartMatchmakingScenarioRequest().getScenarioAttributes().find(pseudoRequestAttrName);


            if (pseudoRequestIter == mRequest.getStartMatchmakingScenarioRequest().getScenarioAttributes().end())
            {
                // pseudo request attribute was missing
                WARN_LOG("[StartPseudoMatchmakingScenarioCommand].execute(): Problem creating Scenario, the scenario attribute "
                    << pseudoRequestAttrName << "' was not defined, inserting into request.");
                mRequest.getStartMatchmakingScenarioRequest().getScenarioAttributes()[pseudoRequestAttrName] = mRequest.getStartMatchmakingScenarioRequest().getScenarioAttributes().allocate_element();
                mRequest.getStartMatchmakingScenarioRequest().getScenarioAttributes()[pseudoRequestAttrName]->set(true);
            }
            else if (!pseudoRequestIter->second->getValue().asBool())
            {
                // pseudo request attribute isn't set to true!
                WARN_LOG("[StartPseudoMatchmakingScenarioCommand].execute(): Problem creating Scenario, the scenario attribute "
                    << pseudoRequestAttrName << "' wasn't set to true, overriding.");
                pseudoRequestIter->second->set(true);
            }

            // for pseudo requests we don't limit the number of concurrent requests
            BlazeRpcError err = mComponent.getScenarioManager().createScenario(mRequest.getStartMatchmakingScenarioRequest(), 
                mResponse, mErrorResponse, getPeerInfo()->getPeerAddress(), &mRequest.getOverrideUserSessionInfo());
            if (err != Blaze::ERR_OK)
            {
                ERR_LOG("[StartPseudoMatchmakingScenarioCommand].execute(): Problem creating Scenario, got error '" 
                    << ErrorHelp::getErrorName(err) << "'.");
                return commandErrorFromBlazeError(err);
            }

            return commandErrorFromBlazeError(ERR_OK);
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    //! \brief static creation factory method for command (from stub)
    DEFINE_STARTPSEUDOMATCHMAKINGSCENARIO_CREATE()


    /*! ************************************************************************************************/
    /*! \brief Cancel a matchmaking session;
    ***************************************************************************************************/
    class CancelMatchmakingScenarioCommand : public CancelMatchmakingScenarioCommandStub
    {
    public:
        CancelMatchmakingScenarioCommand(Message* message, CancelMatchmakingScenarioRequest* request, GameManagerSlaveImpl* componentImpl)
            :   CancelMatchmakingScenarioCommandStub(message, request), mComponent(*componentImpl)
        {
        }

        ~CancelMatchmakingScenarioCommand() override {}

    private:

        CancelMatchmakingScenarioCommandStub::Errors execute() override
        {
            BlazeRpcError err = mComponent.getScenarioManager().cancelScenario(mRequest.getMatchmakingScenarioId());
            return commandErrorFromBlazeError(err);
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    //! \brief static creation factory method for command (from stub)
    DEFINE_CANCELMATCHMAKINGSCENARIO_CREATE()



    /*! ************************************************************************************************/
    /*! \brief Get information about the matchmaking scenario config;
    ***************************************************************************************************/
    class GetScenarioAttributesConfigCommand : public GetScenarioAttributesConfigCommandStub
    {
    public:
        GetScenarioAttributesConfigCommand(Message* message, GameManagerSlaveImpl* componentImpl)
            :   GetScenarioAttributesConfigCommandStub(message), mComponent(*componentImpl)
        {
        }

        ~GetScenarioAttributesConfigCommand() override {}

    private:

        GetScenarioAttributesConfigCommandStub::Errors execute() override
        {
            BlazeRpcError err = mComponent.getScenarioManager().getScenarioAttributesConfig(mResponse);
            return commandErrorFromBlazeError(err);
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    //! \brief static creation factory method for command (from stub)
    DEFINE_GETSCENARIOATTRIBUTESCONFIG_CREATE()

    /*! ************************************************************************************************/
    /*! \brief Get Scenario Details Command
    ***************************************************************************************************/
    class GetScenarioDetailsCommand : public GetScenarioDetailsCommandStub
    {
    public:
        GetScenarioDetailsCommand(Message* message, GameManagerSlaveImpl* componentImpl)
            :   GetScenarioDetailsCommandStub(message), mComponent(*componentImpl)
        {
        }

        ~GetScenarioDetailsCommand() override {}

    private:

        GetScenarioDetailsCommandStub::Errors execute() override
        {
            BlazeRpcError err = mComponent.getScenarioManager().getScenarioDetails(mResponse);
            return commandErrorFromBlazeError(err);
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    //! \brief static creation factory method for command (from stub)
    DEFINE_GETSCENARIODETAILS_CREATE()


    /*! ************************************************************************************************/
    /*! \brief Get information about the matchmaking scenario config;
    ***************************************************************************************************/
    class GetScenariosAndAttributesCommand : public GetScenariosAndAttributesCommandStub
    {
    public:
        GetScenariosAndAttributesCommand(Message* message, GameManagerSlaveImpl* componentImpl)
            : GetScenariosAndAttributesCommandStub(message), mComponent(*componentImpl)
        {
        }

        virtual ~GetScenariosAndAttributesCommand() {}

    private:

        GetScenariosAndAttributesCommandStub::Errors execute()
        {
            BlazeRpcError err = mComponent.getScenarioManager().getScenariosAndAttributes(mResponse);
            return commandErrorFromBlazeError(err);
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    //! \brief static creation factory method for command (from stub)
    DEFINE_GETSCENARIOSANDATTRIBUTES_CREATE()
} // namespace GameManager
} // namespace Blaze
