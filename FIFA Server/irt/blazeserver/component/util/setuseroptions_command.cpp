/*************************************************************************************************/
/*!
\file   setuseroptions_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class SetUserOptionsCommand

Set user's options to db.
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
#include "util/rpc/utilslave/setuseroptions_stub.h"

namespace Blaze
{
namespace Util
{

class SetUserOptionsCommand : public SetUserOptionsCommandStub
{
public:
    SetUserOptionsCommand(Message* message, Blaze::Util::UserOptions* request, UtilSlaveImpl* componentImpl);
    ~SetUserOptionsCommand() override {};

private:
    // Not owned memory.
    UtilSlaveImpl* mComponent;

    // States
    SetUserOptionsCommandStub::Errors execute() override;
};

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Factory Method ******************************************************************************/
DEFINE_SETUSEROPTIONS_CREATE()

/*** Public Methods ******************************************************************************/
SetUserOptionsCommand::SetUserOptionsCommand(Message* message, Blaze::Util::UserOptions* request, UtilSlaveImpl* componentImpl)
    : SetUserOptionsCommandStub(message, request), mComponent(componentImpl)
{
} // ctor

/*** Protected Methods ***************************************************************************/
/*** Private Methods *****************************************************************************/
SetUserOptionsCommand::Errors SetUserOptionsCommand::execute()
{
    return commandErrorFromBlazeError(mComponent->setUserOptionsFromDb(mRequest));
} // start

} // Util
} // Blaze
