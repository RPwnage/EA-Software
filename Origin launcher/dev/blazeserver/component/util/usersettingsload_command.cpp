/*************************************************************************************************/
/*!
    \file   usersettingsload_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UserSettingsLoadCommand

    Loads user's (persona's) game specific settings from db.
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
#include "util/rpc/utilslave/usersettingsload_stub.h"

namespace Blaze
{
namespace Util
{

class UserSettingsLoadCommand : public UserSettingsLoadCommandStub
{
public:
    UserSettingsLoadCommand(Message* message, UserSettingsLoadRequest* request, UtilSlaveImpl* componentImpl);
    ~UserSettingsLoadCommand() override {};

private:
    UserSettingsLoadCommandStub::Errors execute() override;

    // Not owned memory.
    UtilSlaveImpl* mComponent;
};

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Factory Method ******************************************************************************/
DEFINE_USERSETTINGSLOAD_CREATE()

/*** Public Methods ******************************************************************************/
UserSettingsLoadCommand::UserSettingsLoadCommand(Message* message, UserSettingsLoadRequest* request, UtilSlaveImpl* componentImpl)
    : UserSettingsLoadCommandStub(message, request), mComponent(componentImpl)
{
}

/*** Protected Methods ***************************************************************************/
/*** Private Methods *****************************************************************************/
UserSettingsLoadCommandStub::Errors UserSettingsLoadCommand::execute()
{
    // get the blaze ID we want to load and determine if it's us or another user
    BlazeId blazeId = mRequest.getBlazeId();
    bool isOtherUser = ((gCurrentUserSession == nullptr) || ((blazeId != INVALID_BLAZE_ID) && (blazeId != gCurrentUserSession->getBlazeId())));

    if (blazeId == INVALID_BLAZE_ID && !isOtherUser)
    {
        blazeId = gCurrentUserSession->getBlazeId();
    }

    if (blazeId == INVALID_BLAZE_ID)
    {
        ERR_LOG("[UserSettingsLoadCommand] no user provided");
        return ERR_SYSTEM;
    }
    else if (blazeId < 0)
    {
        WARN_LOG("[UserSettingsLoadCommand]: blazeId (" << blazeId << ") can not be negative (is this a dedicated server user?)");
        return ERR_SYSTEM;
    }

    // check permission
    if (isOtherUser && !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_OTHER_USER_SETTINGS_VIEW))
    {
        BlazeId curBlazeId = gCurrentUserSession != nullptr ? gCurrentUserSession->getBlazeId() : INVALID_BLAZE_ID;
        WARN_LOG("[UserSettingsLoadCommand] user [" << curBlazeId << "] attempted to load user settings for [" << blazeId << "], no permission!");
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
        ERR_LOG("[UserSettingsLoadCommand] failed to obtain a connection (platform: " << ClientPlatformTypeToString(platform) << ")");
        return UTIL_USS_DB_ERROR;
    }

    // Build the query.
    QueryPtr query = DB_NEW_QUERY_PTR(dbConn);
    query->append("SELECT `id`, `key`, `data`"
        " FROM `util_user_small_storage`"
        " WHERE `id` = $D AND `key` = '$s'", blazeId, mRequest.getKey());

    // Execute query.
    DbResultPtr dbResult;
    BlazeRpcError error = dbConn->executeQuery(query, dbResult);
    if (error != DB_ERR_OK)
    {
        ERR_LOG("[UserSettingsLoadCommand] failed to read record id(" << blazeId << ") key(" << mRequest.getKey() << ")");
        return UTIL_USS_DB_ERROR;
    }

    // Process query.
    if (dbResult->empty())
    {
        TRACE_LOG("[UserSettingsLoadCommand] no records found for id(" << blazeId << ") key(" << mRequest.getKey() << ")");
        return UTIL_USS_RECORD_NOT_FOUND;
    }

    // Set results.
    const DbRow* row = *dbResult->begin();
    mResponse.setKey(row->getString("key"));
    mResponse.setData(row->getString("data"));

    if (isOtherUser)
    {
        if (gCurrentUserSession != nullptr)
        {
            INFO_LOG("[UserSettingsLoadCommand] user [" << gCurrentUserSession->getBlazeId() << "] loaded user settings for [" << blazeId << "], had permission.");
        }
        else
        {
            TRACE_LOG("[UserSettingsLoadCommand] super-user loaded user settings for [" << blazeId << "], had permission.");
        }
    }

    return ERR_OK;
}

} // Util
} // Blaze
