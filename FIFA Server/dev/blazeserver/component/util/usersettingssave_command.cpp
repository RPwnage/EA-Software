/*************************************************************************************************/
/*!
    \file   usersettingssave_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UserSettingsSaveCommand

    Loads user's (persona's) game specific settings from db.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

// framework includes
#include "framework/database/dbconn.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/query.h"
#include "framework/event/eventmanager.h"
#include "framework/usersessions/usersession.h"
#include "framework/util/shared/blazestring.h"
#include "framework/usersessions/authorization.h"
#include "EASTL/string.h"

// util includes
#include "util/utilslaveimpl.h"
#include "util/rpc/utilslave/usersettingssave_stub.h"


namespace Blaze
{
namespace Util
{

class UserSettingsSaveCommand : public UserSettingsSaveCommandStub
{
public:
    UserSettingsSaveCommand(Message* message, UserSettingsSaveRequest* request, UtilSlaveImpl* componentImpl);
    ~UserSettingsSaveCommand() override {};

private:
    // Not owned memory.
    UtilSlaveImpl* mComponent;

    // States
    UserSettingsSaveCommandStub::Errors execute() override;
};

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Factory Method ******************************************************************************/
DEFINE_USERSETTINGSSAVE_CREATE()

/*** Public Methods ******************************************************************************/
UserSettingsSaveCommand::UserSettingsSaveCommand(Message* message, UserSettingsSaveRequest* request, UtilSlaveImpl* componentImpl)
    : UserSettingsSaveCommandStub(message, request), mComponent(componentImpl)
{
}

/*** Protected Methods ***************************************************************************/
/*** Private Methods *****************************************************************************/
UserSettingsSaveCommandStub::Errors UserSettingsSaveCommand::execute()
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
        ERR_LOG("[UserSettingsSaveCommand]: no user provided");
        return ERR_SYSTEM;
    }
    else if (blazeId < 0)
    {
        WARN_LOG("[UserSettingsSaveCommand]: blazeId (" << blazeId << ") can not be negative (is this a dedicated server user?)");
        return ERR_SYSTEM;
    }

    // check permission
    if (isOtherUser && !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_OTHER_USER_SETTINGS))
    {
        BlazeId curBlazeId = gCurrentUserSession != nullptr ? gCurrentUserSession->getBlazeId() : INVALID_BLAZE_ID;
        WARN_LOG("[UserSettingsSaveCommand] user [" << curBlazeId << "] attempted to save user settings for [" << blazeId << "], no permission!");
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

    uint32_t dbId = mComponent->getDbId(platform);

    // To prevent deadlock, make sure the read-only DbConnPtr goes out of scope
    // before we request a read-write DbConnPtr later
    {
        DbConnPtr dbConn = gDbScheduler->getReadConnPtr(dbId);
        if (dbConn == nullptr)
        {
            ERR_LOG("[UserSettingsSaveCommand] failed to obtain a read-only connection (platform: " << ClientPlatformTypeToString(platform) << ")");
            return UTIL_USS_DB_ERROR;
        }

        // Check how many existing keys there are.
        // ----
        // Build the query.
        QueryPtr query = DB_NEW_QUERY_PTR(dbConn);
        query->append("SELECT `key`"
            " FROM `util_user_small_storage`"
            " WHERE `id` = $D", blazeId);

        // Execute query.
        DbResultPtr dbResult;
        BlazeRpcError error = dbConn->executeQuery(query, dbResult);
        if (error != DB_ERR_OK)
        {
            ERR_LOG("[UserSettingsSaveCommand] failed to read any records for id(" << blazeId << ") key(" << mRequest.getKey() << ")");
            return UTIL_USS_DB_ERROR;
        }

        // IF it is updating an existing key, continue to let it (even if there are more keys
        // in the database than allowed).
        if (dbResult->size() >= mComponent->getMaxKeys())
        {
            DbResult::const_iterator itr = dbResult->begin();
            DbResult::const_iterator end = dbResult->end();
            for (; itr != end; ++itr)
            {
                const DbRow* row = *itr;
                if (blaze_stricmp(row->getString("key"), mRequest.getKey()) == 0)
                {
                    break;
                } // if
            }

            // If not an existing key, fail too many keys.
            if (itr == end)
            {
                ERR_LOG("[UserSettingsSaveCommand] new key (" << mRequest.getKey() << ") for id(" 
                    << blazeId << ") exceeds maximum allowed keys (" << mComponent->getMaxKeys() << ")");
                return UTIL_USS_TOO_MANY_KEYS;
            }
        }
    }

    // Get a read-write database connection.
    DbConnPtr dbConn = gDbScheduler->getConnPtr(dbId);
    if (dbConn == nullptr)
    {
        ERR_LOG("[UserSettingsSaveCommand] failed to obtain a read-write connection");
        return UTIL_USS_DB_ERROR;
    }

    // Build the query to save settings.
    PreparedStatement* stmt = dbConn->getPreparedStatement(mComponent->getQueryId(dbId, UtilSlaveImpl::USER_SETTINGS_SAVE));
    stmt->setInt64(0, blazeId);
    stmt->setString(1, mRequest.getKey());
    stmt->setBinary(2, reinterpret_cast<const uint8_t*>(mRequest.getData()), strlen(mRequest.getData()));
    stmt->setBinary(3, reinterpret_cast<const uint8_t*>(mRequest.getData()), strlen(mRequest.getData()));

    eastl::string strQuery;
    TRACE_LOG("[UserSettingsSaveCommand] query: " << stmt->toString(strQuery));

    DbResultPtr dbResult;
    BlazeRpcError error = dbConn->executePreparedStatement(stmt, dbResult);
    if (error != DB_ERR_OK)
    {
        ERR_LOG("[UserSettingsSaveCommand] failed to write record id(" << blazeId << ") key(" << mRequest.getKey() << ")");
        return UTIL_USS_DB_ERROR;
    }

    if (isOtherUser)
    {
        if (gCurrentUserSession != nullptr)
        {
            INFO_LOG("[UserSettingsSaveCommand] user [" << gCurrentUserSession->getBlazeId() << "] saved user settings for [" << blazeId << "], had permission.");
        }
        else
        {
            TRACE_LOG("[UserSettingsSaveCommand] super-user saved user settings for [" << blazeId << "], had permission.");
        }
    }

    UserSettingsSaved userSettingsSavedEvent;
    userSettingsSavedEvent.setUserPersonaId(blazeId);
    userSettingsSavedEvent.setKey(mRequest.getKey());
    gEventManager->submitEvent(static_cast<uint32_t>(UtilSlave::EVENT_USERSETTINGSSAVEDEVENT), userSettingsSavedEvent, true);

    return ERR_OK;
}

} // Util
} // Blaze
