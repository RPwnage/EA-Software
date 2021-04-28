/*************************************************************************************************/
/*!
    \file   updatehadwareflags_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdateHardwareFlagsCommand 

    process user session extended data retrieval requests from the client.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/updatehardwareflags_stub.h"
#include "framework/tdf/usersessiontypes_server.h"
#include "framework/util/connectutil.h"

namespace Blaze
{
    class UpdateHardwareFlagsCommand : public UpdateHardwareFlagsCommandStub
    {
    public:

        UpdateHardwareFlagsCommand(Message* message, Blaze::UpdateHardwareFlagsRequest* request, UserSessionsSlave* componentImpl)
            : UpdateHardwareFlagsCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~UpdateHardwareFlagsCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        UpdateHardwareFlagsCommandStub::Errors execute() override
        {
            UpdateHardwareFlagsMasterRequest request;
            request.setSessionId(gCurrentUserSession->getSessionId());
            request.setReq(mRequest);
            return (commandErrorFromBlazeError(gUserSessionManager->getMaster()->updateHardwareFlagsMaster(request)));
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_UPDATEHARDWAREFLAGS_CREATE_COMPNAME(UserSessionManager)

} // Blaze
