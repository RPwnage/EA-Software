/*************************************************************************************************/
/*!
    \file   usersettingsloadallforuserid_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UserSettingsLoadAllCommand

    Loads all user's (persona's) game specific settings from db.
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
#include "framework/util/shared/blazestring.h"

// util includes
#include "util/utilslaveimpl.h"
#include "util/rpc/utilslave/usersettingsloadallforuserid_stub.h"

namespace Blaze
{
namespace Util
{

class UserSettingsLoadAllForUserIdCommand : public UserSettingsLoadAllForUserIdCommandStub
{
public:
    UserSettingsLoadAllForUserIdCommand(Message* message, Blaze::Util::UserSettingsLoadAllRequest* request, UtilSlaveImpl* componentImpl);
    ~UserSettingsLoadAllForUserIdCommand() override {};

private:
    // Not owned memory.
    UtilSlaveImpl* mComponent;

    // States
    UserSettingsLoadAllForUserIdCommandStub::Errors execute() override;
};

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Factory Method ******************************************************************************/
DEFINE_USERSETTINGSLOADALLFORUSERID_CREATE()

/*** Public Methods ******************************************************************************/
UserSettingsLoadAllForUserIdCommand::UserSettingsLoadAllForUserIdCommand(Message* message, Blaze::Util::UserSettingsLoadAllRequest* request, UtilSlaveImpl* componentImpl)
    : UserSettingsLoadAllForUserIdCommandStub(message,request), mComponent(componentImpl)
{
} // ctor


/*** Protected Methods ***************************************************************************/
/*** Private Methods *****************************************************************************/
UserSettingsLoadAllForUserIdCommand::Errors UserSettingsLoadAllForUserIdCommand::execute()
{
    BlazeId blazeId = mRequest.getBlazeId();
    bool isOtherUser = ((gCurrentUserSession == nullptr) || ((blazeId != INVALID_BLAZE_ID) && (blazeId != gCurrentUserSession->getBlazeId())));
    
    // Get User Id.
    if (blazeId == INVALID_BLAZE_ID && !isOtherUser)
    {
        blazeId = gCurrentUserSession->getBlazeId();
    }
    
    if (blazeId == INVALID_BLAZE_ID)
    {
        ERR_LOG("[UserSettingsLoadAllForUserIdCommand]: no user provided");
        return ERR_SYSTEM;
    }
    else if (blazeId < 0)
    {
        WARN_LOG("[UserSettingsLoadAllForUserIdCommand]: blazeId (" << blazeId << ") can not be negative (is this a dedicated server user?)");
        return ERR_SYSTEM;
    }

    // check permission
    if (isOtherUser && !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_OTHER_USER_SETTINGS_VIEW))
    {
        WARN_LOG("[UserSettingsLoadAllForUserIdCommand].execute: User [" << gCurrentUserSession->getBlazeId() << "] attempted to load user settings all for ["
            << mRequest.getBlazeId() << "], no permission!");
        return ERR_AUTHORIZATION_REQUIRED;
    }

    if (!mComponent->readUserSettingsForUserIdFromDb(blazeId, &mResponse))
    {
        return UTIL_USS_DB_ERROR;
    }

    if (isOtherUser)
    {
        INFO_LOG("[UserSettingsLoadAllForUserIdCommand].execute: User [" << gCurrentUserSession->getBlazeId() << "] loaded user settings all for ["
            << mRequest.getBlazeId() << "], had permission.");
    }
    return ERR_OK;
} // start

} // Util
} // Blaze
