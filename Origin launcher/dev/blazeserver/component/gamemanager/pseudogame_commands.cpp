/*! ************************************************************************************************/
/*!
    \file creategame_slavecommand.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/createpseudogames_stub.h"
#include "gamemanager/rpc/gamemanagerslave/cancelcreatepseudogames_stub.h"
#include "gamemanager/rpc/gamemanagerslave/destroyallpseudogames_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"
#include "framework/util/connectutil.h"
#include "framework/tdf/userevents_server.h" // for MAX_IPADDRESS_LEN in makeClientInfoLogStr()
#include "gamemanager/gamemanagervalidationutils.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"
#include "gamemanager/gamemanagerhelperutils.h"

namespace Blaze
{
namespace GameManager
{
    class CreatePseudoGamesCommand : public CreatePseudoGamesCommandStub
    {
    public:
        CreatePseudoGamesCommand(Message* message, CreatePseudoGamesRequest *request, GameManagerSlaveImpl* componentImpl)
            : CreatePseudoGamesCommandStub(message, request), mComponent(componentImpl)
        {
        }

        ~CreatePseudoGamesCommand() override
        {
        }

    private:
        CreatePseudoGamesCommandStub::Errors execute() override
        {
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CREATE_PSEUDO_GAMES, false))
            {
                return ERR_AUTHORIZATION_REQUIRED;
            }

            // We should probably confer with the host and just choose one slave to do all the operations.
            mComponent->startCreatePseudoGamesThread(mRequest.getPseudoGameVariantCountMap());

            CreateGameMasterErrorInfo errInfo;
            return commandErrorFromBlazeError(ERR_OK);
        }

        GameManagerSlaveImpl* mComponent; // memory owned by creator, don't free
    };

    //static creation factory method of command's stub class
    DEFINE_CREATEPSEUDOGAMES_CREATE()


    class CancelCreatePseudoGamesCommand : public CancelCreatePseudoGamesCommandStub
    {
    public:
        CancelCreatePseudoGamesCommand(Message* message, GameManagerSlaveImpl* componentImpl)
            : CancelCreatePseudoGamesCommandStub(message), mComponent(componentImpl)
        {
        }

        ~CancelCreatePseudoGamesCommand() override
        {
        }

    private:
        CancelCreatePseudoGamesCommandStub::Errors execute() override
        {
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CREATE_PSEUDO_GAMES, false))
            {
                return ERR_AUTHORIZATION_REQUIRED;
            }

            // We should probably confer with the host and just choose one slave to do all the operations.
            mComponent->cancelCreatePseudoGamesThread();

            CreateGameMasterErrorInfo errInfo;
            return commandErrorFromBlazeError(ERR_OK);
        }

        GameManagerSlaveImpl* mComponent; // memory owned by creator, don't free
    };

    //static creation factory method of command's stub class
    DEFINE_CANCELCREATEPSEUDOGAMES_CREATE()

    class DestroyAllPseudoGamesCommand : public DestroyAllPseudoGamesCommandStub
    {
    public:
        DestroyAllPseudoGamesCommand(Message* message, GameManagerSlaveImpl* componentImpl)
            : DestroyAllPseudoGamesCommandStub(message), mComponent(componentImpl)
        {
        }

        ~DestroyAllPseudoGamesCommand() override
        {
        }

    private:
        DestroyAllPseudoGamesCommandStub::Errors execute() override
        {
            // check caller's admin rights
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_MANAGE_PSEUDO_GAMES, false))
            {
                return ERR_AUTHORIZATION_REQUIRED;
            }
            
            Component::InstanceIdList instances;
            mComponent->getMaster()->getComponentInstances(instances, false);

            // Send the request to all the masters:
            InstanceIdList::iterator idIter = instances.begin();
            for (; idIter != instances.end(); ++idIter)
            {
                RpcCallOptions opts;
                opts.routeTo.setInstanceId(*idIter);
                BlazeRpcError error = mComponent->getMaster()->destroyAllPseudoGamesMaster(opts);
                if (error != ERR_OK)
                {
                    return commandErrorFromBlazeError(error);
                }
            }
            return commandErrorFromBlazeError(ERR_OK);
        }

        GameManagerSlaveImpl* mComponent; // memory owned by creator, don't free
    };

    //static creation factory method of command's stub class
    DEFINE_DESTROYALLPSEUDOGAMES_CREATE()


} // namespace GameManager
} // namespace Blaze
