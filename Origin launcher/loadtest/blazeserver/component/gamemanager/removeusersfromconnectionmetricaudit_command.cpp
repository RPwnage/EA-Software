/*! ************************************************************************************************/
/*!
    \file removeuserstoconnectionmetricaudit_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/removeusersfromconnectionmetricaudit_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"

#include "framework/usersessions/usersessionmanager.h"

namespace Blaze
{
namespace GameManager
{

    /*! ************************************************************************************************/
    /*! \brief Instruct each GM master to remove the users from its local audit list and from the DB
    ***************************************************************************************************/
    class RemoveUsersFromConnectionMetricAuditCommand : public RemoveUsersFromConnectionMetricAuditCommandStub
    {
    public:

        RemoveUsersFromConnectionMetricAuditCommand(Message* message, UserAuditInfoRequest *request, GameManagerSlaveImpl* componentImpl)
            : RemoveUsersFromConnectionMetricAuditCommandStub(message, request),
              mComponent(*componentImpl)
        {
        }

        ~RemoveUsersFromConnectionMetricAuditCommand() override
        {
        }

    private:

        RemoveUsersFromConnectionMetricAuditCommandStub::Errors execute() override
        {
            // Does user have permission to add users to the audit list
            if (!UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_CANCEL_AUDIT))
            {
                ERR_LOG("[RemoveUsersFromConnectionMetricAuditCommand].execute: User does not have permission to remove user from connection metric audit.");
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
                            ERR_LOG("[RemoveUsersFromConnectionMetricAuditCommand].execute: Platform '" << ClientPlatformTypeToString(platform) << "' (for user with persona name '" << id.getPersonaName() <<
                                "') is not hosted. A valid platform must be set when removing users from audit list by PersonaName on shared-cluster deployments.");
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
                            WARN_LOG("[RemoveUsersFromConnectionMetricAuditCommand].execute: Failed to lookup user for persona name " << id.getPersonaName() << " in namespace '" << personaNamespace << "' on platform '" << ClientPlatformTypeToString(platform) << "'");
                            return ERR_SYSTEM;
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
                TRACE_LOG("[RemoveUsersFromConnectionMetricAuditCommand].execute: No users specified in request.");
                return ERR_OK;
            }

            // Need to tell each GM master to remove the users from their local cache
            Component::InstanceIdList instances;
            mComponent.getMaster()->getComponentInstances(instances);
            Component::InstanceIdList::const_iterator iter = instances.begin();
            Component::InstanceIdList::const_iterator end = instances.end();
            RpcCallOptions opts;
            for (; iter != end; ++iter)
            {
                opts.routeTo.setInstanceId(*iter);
                BlazeRpcError rc = mComponent.getMaster()->removeUsersFromConnectionMetricAuditMaster(req, opts);
                if (rc != ERR_OK)
                    return commandErrorFromBlazeError(rc);
            }

            return RemoveUsersFromConnectionMetricAuditCommandStub::ERR_OK;
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    //static creation factory method of command's stub class
    DEFINE_REMOVEUSERSFROMCONNECTIONMETRICAUDIT_CREATE()

}
}