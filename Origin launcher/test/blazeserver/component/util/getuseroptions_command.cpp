/*************************************************************************************************/
/*!
\file   getuseroptions_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class GetUserOptionsCommand

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
#include "util/rpc/utilslave/getuseroptions_stub.h"


namespace Blaze
{
namespace Util
{

class GetUserOptionsCommand : public GetUserOptionsCommandStub
{
public:

    GetUserOptionsCommand(Message* message, Blaze::Util::GetUserOptionsRequest* request, UtilSlaveImpl* componentImpl);
    ~GetUserOptionsCommand() override {};

private:
    // Not owned memory.
    UtilSlaveImpl* mComponent;

    // States
    GetUserOptionsCommandStub::Errors execute() override;
};

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Factory Method ******************************************************************************/
DEFINE_GETUSEROPTIONS_CREATE()


/*** Public Methods ******************************************************************************/
GetUserOptionsCommand::GetUserOptionsCommand(Message* message,
                                             GetUserOptionsRequest* request,
                                             UtilSlaveImpl* componentImpl)
                                             : GetUserOptionsCommandStub(message, request), mComponent(componentImpl)
{
} // ctor


/*** Protected Methods ***************************************************************************/
/*** Private Methods *****************************************************************************/
GetUserOptionsCommand::Errors GetUserOptionsCommand::execute()
{
    mResponse.setBlazeId(mRequest.getBlazeId());
    return commandErrorFromBlazeError(mComponent->getUserOptionsFromDb(mResponse));
} // start

} // Util
} // Blaze
