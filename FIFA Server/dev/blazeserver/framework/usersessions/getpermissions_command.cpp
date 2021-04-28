/*************************************************************************************************/
/*!
    \file   getpermissions_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetPermissionsCommand 

    process user session extended data retrieval requests from the client.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/getpermissions_stub.h"

namespace Blaze
{
    class GetPermissionsCommand : public GetPermissionsCommandStub
    {
    public:
        GetPermissionsCommand(Message* message, UserSessionsSlave* componentImpl)
            : GetPermissionsCommandStub(message),
              mComponent(componentImpl)
        {
        }

        ~GetPermissionsCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:
        GetPermissionsCommandStub::Errors execute() override
        {
            BLAZE_TRACE_LOG(Log::USER, "[GetPermissionsCommand].start()");
            if(EA_UNLIKELY(gUserSessionManager == nullptr))
            {                
                return ERR_SYSTEM;
            }

            bool getPermission = gUserSessionManager->getGroupManager()->getPermissionMap(gCurrentUserSession->getAuthGroupName(), 
                                                                                          mResponse.getPermissionStringMap());
            if (!getPermission)
            {
                BLAZE_WARN_LOG(Log::USER, "[GetPermissionsCommandStub] failed to get permission map for the given user(" << gCurrentUserSession->getBlazeId() << ")\n.");
                return ERR_SYSTEM;
            }

            return ERR_OK;
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_GETPERMISSIONS_CREATE_COMPNAME(UserSessionManager)
} // Blaze
