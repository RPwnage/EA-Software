/*************************************************************************************************/
/*!
    \file   updateusersessionclientdata_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdateSessionClientDataCommand 

    process user session extended data retrieval requests from the client.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/updateusersessionclientdata_stub.h"
#include "framework/tdf/usersessiontypes_server.h"
#include "framework/util/connectutil.h"

namespace Blaze
{
    class UpdateUserSessionClientDataCommand : public UpdateUserSessionClientDataCommandStub
    {
    public:

        UpdateUserSessionClientDataCommand(Message* message, Blaze::UpdateUserSessionClientDataRequest* request, UserSessionsSlave* componentImpl)
            : UpdateUserSessionClientDataCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~UpdateUserSessionClientDataCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        UpdateUserSessionClientDataCommandStub::Errors execute() override
        {
            UpdateUserSessionClientDataMasterRequest request;
            request.setSessionId(gCurrentUserSession->getSessionId());
            request.setReq(mRequest);
            return (commandErrorFromBlazeError(gUserSessionManager->getMaster()->updateUserSessionClientDataMaster(request)));
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_UPDATEUSERSESSIONCLIENTDATA_CREATE_COMPNAME(UserSessionManager)

} // Blaze
