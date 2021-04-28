/*************************************************************************************************/
/*!
    \file   add_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class AddCommand

    Adds an entry to the DynamicInetFilter.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/database/dbconn.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/query.h"
#include "framework/usersessions/usersessionmanager.h"

#include "framework/dynamicinetfilter/rpc/dynamicinetfilterslave/add_stub.h"

#include "framework/dynamicinetfilter/dynamicinetfilterslaveimpl.h"

namespace Blaze
{
namespace DynamicInetFilter
{

class AddCommand : public AddCommandStub
{
public:
    AddCommand(Message* message, AddRequest* request, DynamicInetFilterSlaveImpl* componentImpl);
    ~AddCommand() override {}
    
private:
    // Not owned memory.
    DynamicInetFilterSlaveImpl* mComponent;

    // States
    AddCommandStub::Errors execute() override;
};

DEFINE_ADD_CREATE();

AddCommand::AddCommand(
    Message* message, AddRequest* request, DynamicInetFilterSlaveImpl* componentImpl)
    : AddCommandStub(message, request),
    mComponent(componentImpl)
{
}

/* Private methods *******************************************************************************/    

AddCommandStub::Errors AddCommand::execute()
{
    BLAZE_INFO_LOG(Log::DYNAMIC_INET, "[AddCommand].execute: Attempted-" 
        << UserSessionManager::formatAdminActionContext(mMessage) << ", req = \n" << mRequest);

    // check if the current user has the permission to access DynamicInetFilter
    if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_SERVER_MAINTENANCE))
    {
        BLAZE_WARN_LOG(Log::DYNAMIC_INET, "[AddCommand].execute: " << UserSessionManager::formatAdminActionContext(mMessage)
            << ". Attempted to add new entry to DB, no permission!");
        return ERR_AUTHORIZATION_REQUIRED;
    }

    // validate inputs; all elements except comment must be non-empty, and strings cannot contain arbitrary characters
    if (!DynamicInetFilterImpl::validateString(mRequest.getGroup(), false))
        return DYNAMICINETFILTER_ERR_INVALID_GROUP;
    if (!DynamicInetFilterImpl::validateString(mRequest.getOwner(), false))
        return DYNAMICINETFILTER_ERR_INVALID_OWNER;
    if (!DynamicInetFilterImpl::validateSubNet(mRequest.getSubNet(), false))
        return DYNAMICINETFILTER_ERR_INVALID_SUBNET;
    if (!DynamicInetFilterImpl::validateString(mRequest.getComment(), true))
        return DYNAMICINETFILTER_ERR_INVALID_COMMENT;

    // Add new entry to DB (save the DynamicInetFilter modification)
    AddCommandStub::Errors result = AddCommandStub::ERR_OK;
    DbConnPtr dbConn = gDbScheduler->getConnPtr(mComponent->getDbId());
    if ( dbConn != nullptr )
    {
        QueryPtr query = DB_NEW_QUERY_PTR(dbConn);
        if ( query != nullptr )
        {
            query->append("INSERT INTO dynamicinetfilter (`group`, `owner`, `ip`, `prefix_length`, `comment`, `created_date`) "
                "VALUES ('$s', '$s', '$s', $u, '$s', NULL);",
                mRequest.getGroup(), mRequest.getOwner(),
                mRequest.getSubNet().getIp(), mRequest.getSubNet().getPrefixLength(), mRequest.getComment());

            DbResultPtr dbResultPtr;
            BlazeRpcError dbErr = dbConn->executeQuery(query, dbResultPtr);

            if (dbErr != DB_ERR_OK)
                result = AddCommandStub::ERR_SYSTEM;
            else
            {
                RowId lastInsertId = (RowId)(dbResultPtr->getLastInsertId());
                mResponse.setRowId(lastInsertId);

                Entry entry;
                entry.setRowId(lastInsertId);
                entry.setGroup(mRequest.getGroup());
                entry.setOwner(mRequest.getOwner());
                mRequest.getSubNet().copyInto(entry.getSubNet());
                entry.setComment(mRequest.getComment());
                // Send request to other slaves for adding request to DynamicInetFilter
                mComponent->sendAddOrUpdateNotificationToAllSlaves(&entry);
            }

        }
        else
            result = AddCommandStub::ERR_SYSTEM;
    }
    else
        result = AddCommandStub::ERR_SYSTEM;
    
    if (result == ERR_OK)
    {
        BLAZE_INFO_LOG(Log::DYNAMIC_INET, "[AddCommand].execute: Succeeded-" << UserSessionManager::formatAdminActionContext(mMessage) 
            << ". Added new entry to DB, had permission.");
    }
    return result;
}

}
} // Blaze
