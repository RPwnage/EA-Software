/*************************************************************************************************/
/*!
    \file   usersettingsloadmultiple_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UserSettingsLoadMultipleCommand

    Loads specified settings of the provided user/persona (or current logged-in user/persona) from db.
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
#include "util/rpc/utilslave/usersettingsloadmultiple_stub.h"

namespace Blaze
{
namespace Util
{

class UserSettingsLoadMultipleCommand : public UserSettingsLoadMultipleCommandStub
{
public:
    UserSettingsLoadMultipleCommand(Message* message, UserSettingsLoadMultipleRequest* request, UtilSlaveImpl* componentImpl);
    ~UserSettingsLoadMultipleCommand() override {};

private:
    UserSettingsLoadMultipleCommandStub::Errors execute() override;

    // Not owned memory.
    UtilSlaveImpl* mComponent;
};

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Factory Method ******************************************************************************/
DEFINE_USERSETTINGSLOADMULTIPLE_CREATE()

/*** Public Methods ******************************************************************************/
UserSettingsLoadMultipleCommand::UserSettingsLoadMultipleCommand(Message* message, UserSettingsLoadMultipleRequest* request, UtilSlaveImpl* componentImpl)
    : UserSettingsLoadMultipleCommandStub(message, request), mComponent(componentImpl)
{
}

/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/
UserSettingsLoadMultipleCommandStub::Errors UserSettingsLoadMultipleCommand::execute()
{
    // get the blaze ID we want to load and determine if it's us or another user
    BlazeId curBlazeId = gCurrentUserSession != nullptr ? gCurrentUserSession->getBlazeId() : INVALID_BLAZE_ID;
    BlazeId blazeId = mRequest.getBlazeId();
    bool isOtherUser = ((blazeId != INVALID_BLAZE_ID) && (blazeId != curBlazeId));

    if (blazeId == INVALID_BLAZE_ID && !isOtherUser)
    {
        blazeId = curBlazeId;
    }

    if (blazeId == INVALID_BLAZE_ID)
    {
        ERR_LOG("[UserSettingsLoadMultipleCommand] no user provided");
        return ERR_SYSTEM;
    }
    else if (blazeId < 0)
    {
        WARN_LOG("[UserSettingsLoadMultipleCommand]: blazeId (" << blazeId << ") can not be negative (is this a dedicated server user?)");
        return ERR_SYSTEM;
    }

    // check permission
    if (isOtherUser && !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_OTHER_USER_SETTINGS_VIEW))
    {
        WARN_LOG("[UserSettingsLoadMultipleCommand] user [" << curBlazeId << "] attempted to load user settings for [" << blazeId << "], no permission!");
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
        ERR_LOG("[UserSettingsLoadMultipleCommand] failed to obtain a connection (platform: " << ClientPlatformTypeToString(platform) << ")");
        return UTIL_USS_DB_ERROR;
    }

    // collect the keys into a unique set
    typedef eastl::hash_set<const char8_t*, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > KeySet;
    KeySet keySet;
    UserSettingsLoadMultipleRequest::StringUserSettingKeyList::const_iterator keyReqItr = mRequest.getKeys().begin();
    UserSettingsLoadMultipleRequest::StringUserSettingKeyList::const_iterator keyReqEnd = mRequest.getKeys().end();
    for (; keyReqItr != keyReqEnd; ++keyReqItr)
    {
        keySet.insert(keyReqItr->c_str());
    }

    // Build the query.
    QueryPtr query = DB_NEW_QUERY_PTR(dbConn);
    query->append("SELECT `key`, `data` FROM `util_user_small_storage` WHERE `id` = $D", blazeId);

    eastl::string keysLogStr;
    if (!keySet.empty())
    {
        query->append(" AND `key` IN (");

        KeySet::const_iterator keySetItr = keySet.begin();
        KeySet::const_iterator keySetEnd = keySet.end();
        query->append("'$s'", *keySetItr);
        keysLogStr.append_sprintf("'%s'", *keySetItr);
        while (++keySetItr != keySetEnd)
        {
            query->append(",'$s'", *keySetItr);
            keysLogStr.append_sprintf(",'%s'", *keySetItr);
        }

        query->append(")");
    }

    // Execute query.
    DbResultPtr dbResult;
    BlazeRpcError error = dbConn->executeQuery(query, dbResult);
    if (error != DB_ERR_OK)
    {
        ERR_LOG("[UserSettingsLoadMultipleCommand] failed to read record id(" << blazeId << ") keys(" << keysLogStr.c_str() << ")");
        return UTIL_USS_DB_ERROR;
    }

    // Process query.
    if (dbResult->empty())
    {
        TRACE_LOG("[UserSettingsLoadMultipleCommand] no records found for id(" << blazeId << ") keys(" << keysLogStr.c_str() << ")");
        return UTIL_USS_RECORD_NOT_FOUND;
    }

    // Set results.
    UserSettingsDataMap &map = mResponse.getDataMap();
    map.reserve(dbResult->size());
    // iterate resultset and build response
    DbResult::const_iterator resultItr = dbResult->begin();
    DbResult::const_iterator resultEnd = dbResult->end();
    for (; resultItr != resultEnd; ++resultItr)
    {
        DbRow *dbRow = *resultItr;
        const char8_t* key = dbRow->getString("key");
        const char8_t* data = dbRow->getString("data");
        map[key] = data;
    }

    if (isOtherUser)
    {
        if (curBlazeId != INVALID_BLAZE_ID)
        {
            INFO_LOG("[UserSettingsLoadMultipleCommand] user [" << curBlazeId << "] loaded user settings for [" << blazeId << "], had permission.");
        }
        else
        {
            TRACE_LOG("[UserSettingsLoadMultipleCommand] super-user loaded user settings for [" << blazeId << "], had permission.");
        }
    }

    return ERR_OK;
}

} // Util
} // Blaze
