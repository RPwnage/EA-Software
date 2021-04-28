/*! ************************************************************************************************/
/*!
    \file creategame_slavecommand.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/creategame_stub.h"
#include "gamemanager/rpc/gamemanagerslave/createpseudogame_stub.h"
#include "gamemanager/rpc/gamemanagerslave/creategametemplate_stub.h"
#include "gamemanager/rpc/gamemanagerslave/createtournamentgame_stub.h"
#include "gamemanager/rpc/gamemanagerslave/createorjoingame_stub.h"
#include "gamemanager/rpc/gamemanagerslave/getcreategametemplateattributesconfig_stub.h"
#include "gamemanager/rpc/gamemanagerslave/gettemplatesandattributes_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"
#include "framework/util/connectutil.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/tdf/userevents_server.h" // for MAX_IPADDRESS_LEN in makeClientInfoLogStr()
#include "gamemanager/gamemanagervalidationutils.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"
#include "gamemanager/gamemanagerhelperutils.h"


#include "EAStdC/EAString.h"

namespace Blaze
{
namespace GameManager
{
    class CreateGameBase
    {
        NON_COPYABLE(CreateGameBase);
    public:
        CreateGameBase(CreateGameRequest& request, CreateGameResponse& response, GameManagerSlaveImpl* componentImpl, bool isPseudoGame = false)
            : mRequestBase(request), mResponseBase(response), mComponent(*componentImpl), mIsPseudoGame(isPseudoGame)
        {
            // Convert the Player Join Data (for back compat)
            for (auto playerJoinData : request.getPlayerJoinData().getPlayerDataList())
                convertToPlatformInfo(playerJoinData->getUser());

            // Convert old Xbox session params to new params (for back compat).
            oldToNewExternalSessionIdentificationParams(request.getGameCreationData().getExternalSessionTemplateName(), nullptr, nullptr, nullptr, request.getGameCreationData().getExternalSessionIdentSetup());
        }

        virtual ~CreateGameBase()
        {
        }

    protected:

        virtual const Command* getCallingCommand() = 0;

        BlazeRpcError executeCreateGame(Command* cmd, CreateGameMasterErrorInfo& errInfo)
        {
            BlazeRpcError err = mComponent.createGameInternal(cmd, nullptr, mRequestBase, mResponseBase, errInfo, mIsPseudoGame);

            if (err != ERR_OK)
            {
                gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(err), RiverPoster::game_creation_failed);
            }
            return err;
        }

    private:
        CreateGameRequest& mRequestBase;
        CreateGameResponse& mResponseBase;
        GameManagerSlaveImpl &mComponent;
        bool mIsPseudoGame;
    };

    class CreateGameCommand : public CreateGameCommandStub, CreateGameBase
    {
    public:
        CreateGameCommand(Message* message, CreateGameRequest *request, GameManagerSlaveImpl* componentImpl)
            : CreateGameCommandStub(message, request), CreateGameBase(mRequest, mResponse, componentImpl), mComponent(componentImpl)
        {
        }

        ~CreateGameCommand() override
        {
        }

    protected:
        const Command* getCallingCommand() override
        {
            return this;
        }

    private:
        CreateGameCommandStub::Errors execute() override
        {
            CreateGameMasterErrorInfo errInfo;
            return commandErrorFromBlazeError(executeCreateGame(this, errInfo));
        }

        GameManagerSlaveImpl* mComponent; // memory owned by creator, don't free
    };

    //static creation factory method of command's stub class
    DEFINE_CREATEGAME_CREATE()

    class CreateOrJoinGameCommand : public CreateOrJoinGameCommandStub, CreateGameBase
    {
    public:
        CreateOrJoinGameCommand(Message* message, CreateGameRequest *request, GameManagerSlaveImpl* componentImpl)
            : CreateOrJoinGameCommandStub(message, request), 
              CreateGameBase(mRequest, mCreateGameResponse, componentImpl), 
              mComponent(componentImpl),
              mMasterToUse(INVALID_INSTANCE_ID)
        {
        }

        ~CreateOrJoinGameCommand() override {}

    protected:
        const Command* getCallingCommand() override
        {
            return this;
        }

        virtual InstanceId getMasterToUse()
        {
            return mMasterToUse;
        }

    private:
        CreateOrJoinGameCommandStub::Errors execute() override
        {
            BlazeRpcError error = mComponent->createOrJoinGameInternal(this, nullptr, mRequest, mResponse);
            return commandErrorFromBlazeError(error);
        }

        CreateGameResponse mCreateGameResponse;
        GameManagerSlaveImpl* mComponent; // memory owned by creator, don't free
        InstanceId mMasterToUse;
    };

    //static creation factory method of command's stub class
    DEFINE_CREATEORJOINGAME_CREATE()


    class CreatePseudoGameCommand : public CreatePseudoGameCommandStub, CreateGameBase
    {
    public:
        CreatePseudoGameCommand(Message* message, CreateGameRequest *request, GameManagerSlaveImpl* componentImpl)
            : CreatePseudoGameCommandStub(message, request), CreateGameBase(mRequest, mResponse, componentImpl, true), mComponent(componentImpl)
        {
        }

        ~CreatePseudoGameCommand() override
        {
        }

    protected:
        const Command* getCallingCommand() override
        {
            return this;
        }

        virtual InstanceId getMasterToUse()
        {
            return INVALID_INSTANCE_ID;
        }

    private:
        CreatePseudoGameCommandStub::Errors execute() override
        {
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CREATE_PSEUDO_GAMES, false))
            {
                return ERR_AUTHORIZATION_REQUIRED;
            }

            CreateGameMasterErrorInfo errInfo;
            return commandErrorFromBlazeError(executeCreateGame(this, errInfo));
        }

        GameManagerSlaveImpl* mComponent; // memory owned by creator, don't free
    };

    //static creation factory method of command's stub class
    DEFINE_CREATEPSEUDOGAME_CREATE()


    class CreateGameTemplateCommand : public CreateGameTemplateCommandStub
    {
    public:
        CreateGameTemplateCommand(Message* message, CreateGameTemplateRequest *request, GameManagerSlaveImpl* componentImpl)
            : CreateGameTemplateCommandStub(message, request), mComponent(componentImpl)
        {
        }

        ~CreateGameTemplateCommand() override
        {
        }

    protected:
        virtual const Command* getCallingCommand()
        {
            return this;
        }

    private:
        CreateGameTemplateCommandStub::Errors execute() override
        {
            // Convert the Player Join Data (for back compat)
            for (auto playerJoinData : mRequest.getPlayerJoinData().getPlayerDataList())
                convertToPlatformInfo(playerJoinData->getUser());

            return commandErrorFromBlazeError(mComponent->createGameTemplate(this, mRequest, mResponse));
        }

        GameManagerSlaveImpl* mComponent; // memory owned by creator, don't free
    };

    //static creation factory method of command's stub class
    DEFINE_CREATEGAMETEMPLATE_CREATE()

    /*! ************************************************************************************************/
    /*! \brief Get information about the create game template config;
    ***************************************************************************************************/
    class GetCreateGameTemplateAttributesConfigCommand : public GetCreateGameTemplateAttributesConfigCommandStub
    {
    public:
        GetCreateGameTemplateAttributesConfigCommand(Message* message, GameManagerSlaveImpl* componentImpl)
            :   GetCreateGameTemplateAttributesConfigCommandStub(message), mComponent(*componentImpl)
        {
        }

        ~GetCreateGameTemplateAttributesConfigCommand() override {}

    private:

        GetCreateGameTemplateAttributesConfigCommandStub::Errors execute() override
        {
            BlazeRpcError err = mComponent.internalGetCreateGameTemplateAttributesConfig(mResponse);
            return commandErrorFromBlazeError(err);
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    //! \brief static creation factory method for command (from stub)
    DEFINE_GETCREATEGAMETEMPLATEATTRIBUTESCONFIG_CREATE()


    class CreateTournamentGameCommand : public CreateTournamentGameCommandStub
    {
    public:
        CreateTournamentGameCommand(Message* message, CreateGameTemplateRequest *request, GameManagerSlaveImpl* componentImpl)
            : CreateTournamentGameCommandStub(message, request), mComponent(componentImpl)
        {
        }

        ~CreateTournamentGameCommand() override
        {
        }

    protected:
        virtual const Command* getCallingCommand()
        {
            return this;
        }

    private:
        CreateTournamentGameCommandStub::Errors execute() override
        {
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_TOURNAMENT_PROVIDER, false))
            {
                return ERR_AUTHORIZATION_REQUIRED;
            }

            // Convert the Player Join Data (for back compat)
            for (auto playerJoinData : mRequest.getPlayerJoinData().getPlayerDataList())
                convertToPlatformInfo(playerJoinData->getUser());

            JoinGameResponse response;
            BlazeRpcError err = mComponent->createGameTemplate(this, mRequest, response, &mErrorResponse.getFailedEncryptedBlazeIds(), true);
            if (err == ERR_OK)
            {
                const ExternalSessionUtil* util = mComponent->getExternalSessionUtilManager().getUtil(xone);
                if (util && !util->getConfig().getScidAsTdfString().empty())
                    mResponse.setScid(util->getConfig().getScid());

                mResponse.setGameId(response.getGameId());
                mResponse.setJoinState(response.getJoinState());
                response.getExternalSessionIdentification().copyInto(mResponse.getExternalSessionIdentification());
            }
            return commandErrorFromBlazeError(err);
        }

        GameManagerSlaveImpl* mComponent; // memory owned by creator, don't free
    };

    //static creation factory method of command's stub class
    DEFINE_CREATETOURNAMENTGAME_CREATE()

    /*! ************************************************************************************************/
    /*! \brief Get information about the matchmaking scenario config;
    ***************************************************************************************************/
    class GetTemplatesAndAttributesCommand : public GetTemplatesAndAttributesCommandStub
    {
    public:
        GetTemplatesAndAttributesCommand(Message* message, GameManagerSlaveImpl* componentImpl)
            : GetTemplatesAndAttributesCommandStub(message), mComponent(*componentImpl)
        {
        }

        virtual ~GetTemplatesAndAttributesCommand() {}

    private:

        GetTemplatesAndAttributesCommandStub::Errors execute()
        {
            BlazeRpcError err = mComponent.internalGetTemplatesAndAttributes(mResponse);
            return commandErrorFromBlazeError(err);
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    //! \brief static creation factory method for command (from stub)
    DEFINE_GETTEMPLATESANDATTRIBUTES_CREATE()
} // namespace GameManager
} // namespace Blaze
