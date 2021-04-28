/*************************************************************************************************/
/*!
\file   postauth_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class PostAuthCommand

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
#include "util/rpc/utilslave/postauth_stub.h"

namespace Blaze
{
namespace Util
{

class PostAuthCommand : public PostAuthCommandStub
{
public:
    PostAuthCommand(Message* message, PostAuthRequest* request, UtilSlaveImpl* componentImpl);
    ~PostAuthCommand() override {};

private:
    // Not owned memory.
    UtilSlaveImpl* mComponent;

    // States
    PostAuthCommand::Errors execute() override;
};

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Factory Method ******************************************************************************/
DEFINE_POSTAUTH_CREATE()


/*** Public Methods ******************************************************************************/
PostAuthCommand::PostAuthCommand(Message* message, PostAuthRequest* request, UtilSlaveImpl* componentImpl)
    : PostAuthCommandStub(message, request), mComponent(componentImpl)
{
} // ctor

/*** Protected Methods ***************************************************************************/
/*** Private Methods *****************************************************************************/
PostAuthCommand::Errors PostAuthCommand::execute()
{
    return commandErrorFromBlazeError(mComponent->postAuthExc(mRequest, mResponse, this));
} // start

} // Util
} // Blaze
