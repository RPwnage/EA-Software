/*************************************************************************************************/
/*!
    \file   update_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdateCommand

    Updates the contents of an entry in the DynamicInetFilter.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/database/dbconn.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/query.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/dynamicinetfilter/rpc/dynamicinetfilterslave/update_stub.h"

#include "framework/dynamicinetfilter/dynamicinetfilterslaveimpl.h"

namespace Blaze
{
namespace DynamicInetFilter
{

class UpdateCommand : public UpdateCommandStub
{
public:
    UpdateCommand(Message* message, UpdateRequest* request, DynamicInetFilterSlaveImpl* componentImpl);
    ~UpdateCommand() override {}

private:
    // Not owned memory.
    DynamicInetFilterSlaveImpl* mComponent;

    // States
    UpdateCommandStub::Errors execute() override;
};

DEFINE_UPDATE_CREATE();

UpdateCommand::UpdateCommand(
    Message* message, UpdateRequest* request, DynamicInetFilterSlaveImpl* componentImpl)
    : UpdateCommandStub(message, request),
    mComponent(componentImpl)
{
}

/* Private methods *******************************************************************************/

UpdateCommandStub::Errors UpdateCommand::execute()
{
    BLAZE_INFO_LOG(Log::DYNAMIC_INET, "[UpdateCommand].execute: Attempted-" 
        << UserSessionManager::formatAdminActionContext(mMessage) << ", req = \n" << mRequest);

    // Check if the current user has the permission to access DynamicInetFilter
    if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_SERVER_MAINTENANCE))
    {
        BLAZE_WARN_LOG(Log::DYNAMIC_INET, "[UpdateCommand].execute: " << UserSessionManager::formatAdminActionContext(mMessage) 
            << ". Attempted to modify entry in DB, no permission!");
        return ERR_AUTHORIZATION_REQUIRED;
    }

    // Validate inputs; all elements except comment must be non-empty, and strings cannot contain arbitrary characters
    if (mRequest.getRowId() == INVALID_ROW_ID)
        return DYNAMICINETFILTER_ERR_ROW_NOT_FOUND;
    if (!DynamicInetFilterImpl::validateString(mRequest.getGroup(), false))
        return DYNAMICINETFILTER_ERR_INVALID_GROUP;
    if (!DynamicInetFilterImpl::validateString(mRequest.getOwner(), false))
        return DYNAMICINETFILTER_ERR_INVALID_OWNER;
    if (!DynamicInetFilterImpl::validateSubNet(mRequest.getSubNet(), false))
        return DYNAMICINETFILTER_ERR_INVALID_SUBNET;
    if (!DynamicInetFilterImpl::validateString(mRequest.getComment(), true))
        return DYNAMICINETFILTER_ERR_INVALID_COMMENT;
    
    // Modify entry in DB (save the DynamicInetFilter modification)
    UpdateCommandStub::Errors result = UpdateCommandStub::ERR_OK;
    DbConnPtr dbConn = gDbScheduler->getConnPtr(mComponent->getDbId());
    if ( dbConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(dbConn);
        if ( query != nullptr )
        {
            query->append("UPDATE dynamicinetfilter SET `group`='$s', `owner`='$s', `ip`='$s', `prefix_length`=$u, `comment`='$s' WHERE `row_id`=$u;",
                mRequest.getGroup(), mRequest.getOwner(),
                mRequest.getSubNet().getIp(), mRequest.getSubNet().getPrefixLength(), mRequest.getComment(), mRequest.getRowId());

            DbResultPtr dbResult;
            DbError dbErr = dbConn->executeQuery(query, dbResult);

            if (dbErr != DB_ERR_OK)
                result = UpdateCommandStub::ERR_SYSTEM;
            else
            {
                // Send request to other slaves for changing the contents of entry in DynamicInetFilter
                Entry entry;
                entry.setRowId(mRequest.getRowId());
                entry.setGroup(mRequest.getGroup());
                entry.setOwner(mRequest.getOwner());
                mRequest.getSubNet().copyInto(entry.getSubNet());
                entry.setComment(mRequest.getComment());
                mComponent->sendAddOrUpdateNotificationToAllSlaves(&entry);
            }
        }
        else
            result = UpdateCommandStub::ERR_SYSTEM;
    }
    else
        result = UpdateCommandStub::ERR_SYSTEM;
    
    if (result == ERR_OK)
    {
        BLAZE_INFO_LOG(Log::DYNAMIC_INET, "[UpdateCommand].execute: Succeeded-" << UserSessionManager::formatAdminActionContext(mMessage) 
            << ". Modified entry in DB, had permission.");
    }
    return result;
}

}
} // Blaze
