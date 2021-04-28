/*! ************************************************************************************************/
/*!
    \file adduserstoconnectionmetricaudit_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/adduserstoconnectionmetricaudit_stub.h"
#include "gamemanagerslaveimpl.h"
#include "framework/usersessions/usersessionmanager.h"

namespace Blaze
{
namespace GameManager
{

    /*! ************************************************************************************************/
    /*! \brief Instruct each GM master to add the users to its local audit list and to the DB
    ***************************************************************************************************/
    class AddUsersToConnectionMetricAuditCommand : public AddUsersToConnectionMetricAuditCommandStub
    {
    public:

        AddUsersToConnectionMetricAuditCommand(Message* message, UserAuditInfoRequest *request, GameManagerSlaveImpl* componentImpl)
            : AddUsersToConnectionMetricAuditCommandStub(message, request),
              mComponent(*componentImpl)
        {
        }

        ~AddUsersToConnectionMetricAuditCommand() override
        {
        }

    private:

        AddUsersToConnectionMetricAuditCommandStub::Errors execute() override
        {
            // Does user have permission to add users to the audit list
            if (!UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_CREATE_AUDIT))
            {
                ERR_LOG("[AddUsersToConnectionMetricAuditCommand].execute: User does not have permission to add user to connection metric audit.");
                return ERR_AUTHORIZATION_REQUIRED;
            }

            ClientPlatformType platform = gController->isSharedCluster() ? mRequest.getPlatform() : gController->getDefaultClientPlatform();
            const char8_t* personaNamespace = gController->getNamespaceFromPlatform(platform);

            UserAuditInfoMasterRequest req;
            UserAuditInfoRequest::UserAuditIdentifierList::const_iterator idIter = mRequest.getUserIdentifiers().begin();
            UserAuditInfoRequest::UserAuditIdentifierList::const_iterator idEnd = mRequest.getUserIdentifiers().end();
            for (; idIter != idEnd; ++idIter)
            {
                const UserAuditIdentifier& id = **idIter;
                switch (id.getActiveMember())
                {
                case UserAuditIdentifier::MEMBER_BLAZEID:
                    {
                        req.getBlazeIds().push_back(id.getBlazeId());
                        break;
                    }
                case UserAuditIdentifier::MEMBER_PERSONANAME:
                    {
                        if (!gController->isPlatformHosted(platform))
                        {
                            ERR_LOG("[AddUsersToConnectionMetricAuditCommand].execute: Platform '" << ClientPlatformTypeToString(platform) << "' (for user with persona name '" << id.getPersonaName() <<
                                "') is not hosted. A valid platform must be set when adding users to audit list by PersonaName on shared-cluster deployments.");
                            return ERR_SYSTEM;
                        }

                        UserInfoPtr user;
                        BlazeRpcError err = gUserSessionManager->lookupUserInfoByPersonaName(id.getPersonaName(), platform, user, personaNamespace);
                        if (err == ERR_OK)
                        {
                            req.getBlazeIds().push_back(user->getId());
                        }
                        else
                        {
                            if (err != USER_ERR_USER_NOT_FOUND)
                            {
                                WARN_LOG("[AddUsersToConnectionMetricAuditCommand].execute: Failed to lookup user for persona name " << id.getPersonaName() << " in namespace '" << personaNamespace << "' on platform '" << ClientPlatformTypeToString(platform) << "'");
                                return commandErrorFromBlazeError(err);
                            }
                            return GAMEMANAGER_ERR_USER_NOT_FOUND;
                        }
                        break;
                    }
                default:
                    {
                        ERR_LOG("[AddUsersToConnectionMetricAuditCommand].execute: BlazeId or PersonaName must be set when adding user to audit list.");
                        return ERR_SYSTEM;
                    }
                }
            }

            if (req.getBlazeIds().empty())
            {
                TRACE_LOG("[AddUsersToConnectionMetricAuditCommand].execute: No users specified in request.");
                return ERR_OK;
            }

            req.setExpiration(mRequest.getExpiration());

            // Need to tell each GM master to add the users to their local cache
            Component::InstanceIdList instances;
            mComponent.getMaster()->getComponentInstances(instances);
            Component::InstanceIdList::const_iterator iter = instances.begin();
            Component::InstanceIdList::const_iterator end = instances.end();
            RpcCallOptions opts;
            for (; iter != end; ++iter)
            {
                opts.routeTo.setInstanceId(*iter);
                BlazeRpcError rc = mComponent.getMaster()->addUsersToConnectionMetricAuditMaster(req, opts);
                if (rc != ERR_OK)
                    return commandErrorFromBlazeError(rc);
            }

            return ERR_OK;
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    //static creation factory method of command's stub class
    DEFINE_ADDUSERSTOCONNECTIONMETRICAUDIT_CREATE()

}
}
