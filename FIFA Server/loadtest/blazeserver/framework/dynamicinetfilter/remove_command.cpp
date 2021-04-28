/*************************************************************************************************/
/*!
    \file   remove_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class RemoveCommand

    Removes an entry from the DynamicInetFilter.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/database/dbconn.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/query.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/dynamicinetfilter/rpc/dynamicinetfilterslave/remove_stub.h"

#include "framework/dynamicinetfilter/dynamicinetfilterslaveimpl.h"

namespace Blaze
{
namespace DynamicInetFilter
{

class RemoveCommand : public RemoveCommandStub
{
public:
    RemoveCommand(Message* message, RemoveRequest* request, DynamicInetFilterSlaveImpl* componentImpl);
    ~RemoveCommand() override {}
    
private:
    // Not owned memory.
    DynamicInetFilterSlaveImpl* mComponent;

    // States
    RemoveCommandStub::Errors execute() override;
};

DEFINE_REMOVE_CREATE();

RemoveCommand::RemoveCommand(
    Message* message, RemoveRequest* request, DynamicInetFilterSlaveImpl* componentImpl)
    : RemoveCommandStub(message, request),
    mComponent(componentImpl)
{
}

/* Private methods *******************************************************************************/    
RemoveCommandStub::Errors RemoveCommand::execute()
{
    BLAZE_INFO_LOG(Log::DYNAMIC_INET, "[RemoveCommand].execute: Attempted-" 
        << UserSessionManager::formatAdminActionContext(mMessage) << ", req = \n" << mRequest);

    // Check if the current user has the permission to access DynamicInetFilter
    if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_SERVER_MAINTENANCE))
    {
        BLAZE_WARN_LOG(Log::DYNAMIC_INET, "[RemoveCommand].execute: " << UserSessionManager::formatAdminActionContext(mMessage) 
            << ". Attempted to remove entry in DB, no permission!");
        return ERR_AUTHORIZATION_REQUIRED;
    }

    // Validate inputs: rowId must be set to something
    if (mRequest.getRowId() == INVALID_ROW_ID)
        return DYNAMICINETFILTER_ERR_ROW_NOT_FOUND;
    
    // Remove entry in DB (save the DynamicInetFilter modification)
    RemoveCommandStub::Errors result = RemoveCommandStub::ERR_OK;
    DbConnPtr dbConnPtr = gDbScheduler->getConnPtr(mComponent->getDbId());
    if ( dbConnPtr != nullptr )
    {
        QueryPtr query = DB_NEW_QUERY_PTR(dbConnPtr);
        if ( query != nullptr )
        {
            query->append("DELETE FROM dynamicinetfilter where `row_id`=$u;", mRequest.getRowId());

            DbResultPtr dbResultPtr;
            DbError dbErr = dbConnPtr->executeQuery(query, dbResultPtr);

            if (dbErr != DB_ERR_OK)
                result = RemoveCommandStub::ERR_SYSTEM;
            else
            {
                // Send request to other slaves for removing an entry in DynamicInetFilter
                mComponent->sendRemoveNotificationToAllSlaves(&mRequest);
            }
        }
        else
            result = RemoveCommandStub::ERR_SYSTEM;
    }
    else
        result = RemoveCommandStub::ERR_SYSTEM;
    
    if (result == ERR_OK)
    {
        BLAZE_INFO_LOG(Log::DYNAMIC_INET, "[RemoveCommand].execute: Succeeded-" << UserSessionManager::formatAdminActionContext(mMessage) 
            << ". Removed entry in DB, had permission.");
    }
    return result;
}

}
} // Blaze
