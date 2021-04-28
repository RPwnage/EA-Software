/*************************************************************************************************/
/*!
    \file  deleteusersettings_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class DeleteUserSettingsCommand

    Delete user's specific settings from db.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
#include "EASTL/string.h"

// framework includes
#include "framework/database/dbconn.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/query.h"
#include "framework/usersessions/usersession.h"
#include "framework/usersessions/authorization.h"

// util includes
#include "util/utilslaveimpl.h"
#include "util/rpc/utilslave/deleteusersettings_stub.h"

namespace Blaze
{
namespace Util
{

class DeleteUserSettingsCommand : public DeleteUserSettingsCommandStub
{
public:
    DeleteUserSettingsCommand(Message* message, DeleteUserSettingsRequest* request, UtilSlaveImpl* componentImpl);
    ~DeleteUserSettingsCommand() override {};

private:
    DeleteUserSettingsCommandStub::Errors execute() override;

    // Not owned memory.
    UtilSlaveImpl* mComponent;
};

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Factory Method ******************************************************************************/
DEFINE_DELETEUSERSETTINGS_CREATE()

/*** Public Methods ******************************************************************************/
DeleteUserSettingsCommand::DeleteUserSettingsCommand(Message* message, DeleteUserSettingsRequest* request, UtilSlaveImpl* componentImpl)
    : DeleteUserSettingsCommandStub(message, request), mComponent(componentImpl)
{
}

/*** Protected Methods ***************************************************************************/
/*** Private Methods *****************************************************************************/
DeleteUserSettingsCommandStub::Errors DeleteUserSettingsCommand::execute()
{
    // get the blaze ID we want to load and determine if it's us or another user
    BlazeId blazeId = mRequest.getBlazeId();
    bool isOtherUser = ((gCurrentUserSession == nullptr) || (blazeId != INVALID_BLAZE_ID && blazeId != gCurrentUserSession->getBlazeId()));

    if (blazeId == INVALID_BLAZE_ID && !isOtherUser)
    {
        blazeId = gCurrentUserSession->getBlazeId();
    }

    if (blazeId == INVALID_BLAZE_ID)
    {
        ERR_LOG("[DeleteUserSettingsCommand] no user provided");
        return ERR_SYSTEM;
    }

    // check permission
    if (isOtherUser && !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_OTHER_USER_SETTINGS))
    {
        BlazeId curBlazeId = gCurrentUserSession != nullptr ? gCurrentUserSession->getBlazeId() : INVALID_BLAZE_ID;
        WARN_LOG("[DeleteUserSettingsCommand] user [" << curBlazeId << "] attempted to delete user settings for [" << blazeId << "], no permission!");
        return ERR_AUTHORIZATION_REQUIRED;
    }

    ClientPlatformType platform = ClientPlatformType::INVALID;
    if (!isOtherUser)
    {
        platform = gCurrentUserSession->getPlatformInfo().getClientPlatform();
    }
    else
    {
        UserInfoPtr userInfo = nullptr;
        BlazeRpcError lookupError = gUserSessionManager->lookupUserInfoByBlazeId(blazeId, userInfo);
        if (lookupError != Blaze::ERR_OK)
        {
            return commandErrorFromBlazeError(lookupError);
        }
        platform = userInfo->getPlatformInfo().getClientPlatform();
    }

    // Get a database connection.
    DbConnPtr dbConn = gDbScheduler->getReadConnPtr(mComponent->getDbId(platform));
    if (dbConn == nullptr)
    {
        ERR_LOG("[DeleteUserSettingsCommand] failed to obtain a connection (platform: " << ClientPlatformTypeToString(platform) << ")");
        return UTIL_USS_DB_ERROR;
    }

    // Build the query.
    QueryPtr query = DB_NEW_QUERY_PTR(dbConn);
    query->append("DELETE FROM `util_user_small_storage`"
        " WHERE `id` = $D AND `key` = '$s'", blazeId, mRequest.getKey());

    // Execute query.
    DbResultPtr dbResult;
    BlazeRpcError error = dbConn->executeQuery(query, dbResult);
    if (error != DB_ERR_OK)
    {
        ERR_LOG("[DeleteUserSettingsCommand] failed to delete record id(" << blazeId << ") key(" << mRequest.getKey() << ")");
        return UTIL_USS_DB_ERROR;
    }

    if (isOtherUser)
    {
        if (gCurrentUserSession != nullptr)
        {
            INFO_LOG("[DeleteUserSettingsCommand] user [" << gCurrentUserSession->getBlazeId() << "] deleted user settings for [" << blazeId << "], had permission.");
        }
        else
        {
            TRACE_LOG("[DeleteUserSettingsCommand] super-user deleted user settings for [" << blazeId << "], had permission.");
        }
    }

    return ERR_OK;
}

} // Util
} // Blaze
