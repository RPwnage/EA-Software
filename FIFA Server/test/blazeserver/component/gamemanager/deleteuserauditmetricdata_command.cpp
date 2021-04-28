/*! ************************************************************************************************/
/*!
    \file deleteuserauditmetricdata_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/deleteuserauditmetricdata_stub.h"
#include "gamemanagerslaveimpl.h"

namespace Blaze
{
namespace GameManager
{

    /*! ************************************************************************************************/
    /*! \brief Delete a user's audit metric data from the DB, subject to appropriate permission
    ***************************************************************************************************/
    class DeleteUserAuditMetricDataCommand : public DeleteUserAuditMetricDataCommandStub
    {
    public:

        DeleteUserAuditMetricDataCommand(Message* message, DeleteUserAuditMetricDataRequest *request, GameManagerSlaveImpl* componentImpl)
            : DeleteUserAuditMetricDataCommandStub(message, request),
              mComponent(*componentImpl)
        {
        }

        ~DeleteUserAuditMetricDataCommand() override
        {
        }

    private:

        DeleteUserAuditMetricDataCommandStub::Errors execute() override
        {
            if (!UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_DELETE_AUDIT_DATA))
            {
                ERR_LOG("[DeleteUserAuditMetricDataCommand].execute: User does not have permission to delete audit data.");
                return ERR_AUTHORIZATION_REQUIRED;
            }

            if (mRequest.getBlazeId() == INVALID_BLAZE_ID)
            {
                INFO_LOG("[DeleteUserAuditMetricDataCommand].execute: No blazeId specified");
                return commandErrorFromBlazeError(GAMEMANAGER_ERR_USER_NOT_FOUND);
            }

            FetchAuditedUsersResponse response;
            BlazeRpcError rc = mComponent.getMaster()->fetchAuditedUsersMaster(response);
            if (rc != ERR_OK)
            {
                return commandErrorFromBlazeError(rc);
            }

            FetchAuditedUsersResponse::AuditedUserList::const_iterator iter = response.getAuditedUsers().begin();
            FetchAuditedUsersResponse::AuditedUserList::const_iterator end = response.getAuditedUsers().end();
            for (; iter != end; ++iter)
            {
                if ((*iter)->getCoreIdentification().getBlazeId() == mRequest.getBlazeId())
                    break;
            }

            if (iter != end)
            {
                INFO_LOG("[DeleteUserAuditMetricDataCommand].execute: Metric data for user " << mRequest.getBlazeId() << " cannot be deleted as user is currently being audited.");
                return commandErrorFromBlazeError(GAMEMANAGER_ERR_USER_CURRENTLY_AUDITED);
            }

            return commandErrorFromBlazeError(mComponent.deleteUserAuditMetricData(mRequest.getBlazeId()));
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    //static creation factory method of command's stub class
    DEFINE_DELETEUSERAUDITMETRICDATA_CREATE()

}
}
