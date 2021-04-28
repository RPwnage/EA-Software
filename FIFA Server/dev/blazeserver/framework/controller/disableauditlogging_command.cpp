/*************************************************************************************************/
/*!
    \file   disableauditlogging_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class DisableAuditLoggingCommand

    Disable audit logging for the passed in list of users, deviceIds, and/or IP Addresses.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/blazecontrollerslave/disableauditlogging_stub.h"

namespace Blaze
{

class DisableAuditLoggingCommand : public DisableAuditLoggingCommandStub
{
public:
    DisableAuditLoggingCommand(Message* message, Blaze::UpdateAuditLoggingRequest* request, BlazeControllerSlave* componentImpl)
        : DisableAuditLoggingCommandStub(message, request),
          mComponent(componentImpl)
    {
    }

    ~DisableAuditLoggingCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    DisableAuditLoggingCommandStub::Errors execute() override
    {
        BLAZE_TRACE_LOG(Log::CONTROLLER, "[DisableAuditLoggingCommand].execute()");

        // check if the current user has the permission to disable audit logging
        if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CHANGE_AUDIT_LOGGING) &&
            !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CHANGE_AUDIT_LOGGING_UNRESTRICTED))
        {
            BLAZE_WARN_LOG(Log::CONTROLLER, "[DisableAuditLoggingCommand].execute: User [" << gCurrentUserSession->getBlazeId() << "] " 
                << " attempted to disable audit logging without permission!" );

            return ERR_AUTHORIZATION_REQUIRED;
        }

        return commandErrorFromBlazeError(gController->disableAuditLogging(mRequest));
    }

private:
    BlazeControllerSlave* mComponent;  // memory owned by creator, don't free
};

DEFINE_DISABLEAUDITLOGGING_CREATE_COMPNAME(Controller)

} // Blaze
