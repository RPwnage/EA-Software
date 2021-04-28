/*************************************************************************************************/
/*!
    \file   usersettingsloadall_command.cpp


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
#include "util/rpc/utilslave/usersettingsloadall_stub.h"

namespace Blaze
{
namespace Util
{

class UserSettingsLoadAllCommand : public UserSettingsLoadAllCommandStub
{
public:
    UserSettingsLoadAllCommand(Message* message, UtilSlaveImpl* componentImpl);
    ~UserSettingsLoadAllCommand() override {};

private:
    // Not owned memory.
    UtilSlaveImpl* mComponent;

    // States
    UserSettingsLoadAllCommandStub::Errors execute() override;
};

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Factory Method ******************************************************************************/
DEFINE_USERSETTINGSLOADALL_CREATE()


/*** Public Methods ******************************************************************************/
UserSettingsLoadAllCommand::UserSettingsLoadAllCommand(Message* message, UtilSlaveImpl* componentImpl)
    : UserSettingsLoadAllCommandStub(message), mComponent(componentImpl)
{
} // ctor


/*** Protected Methods ***************************************************************************/
/*** Private Methods *****************************************************************************/
UserSettingsLoadAllCommandStub::Errors UserSettingsLoadAllCommand::execute()
{
    BlazeId blazeId = gCurrentUserSession->getBlazeId();
    if (blazeId < 0)
    {
        WARN_LOG("[UserSettingsLoadAllCommand]: BlazeId (" << blazeId << ") can not be negative (is this a dedicated server user?)");
        return ERR_SYSTEM;
    }

    if (!mComponent->readUserSettingsForUserIdFromDb(blazeId,&mResponse))
    {
        return UTIL_USS_DB_ERROR;
    }
    return ERR_OK;
} // start

} // Util
} // Blaze
