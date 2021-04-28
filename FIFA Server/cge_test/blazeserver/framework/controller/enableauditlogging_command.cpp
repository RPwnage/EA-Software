/*************************************************************************************************/
/*!
    \file   enableauditlogging_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class EnableAuditLoggingCommand

    Enable audit logging for the passed in list of users, deviceIds, and/or IP Addresses.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/blazecontrollerslave/enableauditlogging_stub.h"

namespace Blaze
{

class EnableAuditLoggingCommand : public EnableAuditLoggingCommandStub
{
public:
    EnableAuditLoggingCommand(Message* message, Blaze::UpdateAuditLoggingRequest* request, BlazeControllerSlave* componentImpl)
        : EnableAuditLoggingCommandStub(message, request),
          mComponent(componentImpl)
    {
    }

    ~EnableAuditLoggingCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    EnableAuditLoggingCommandStub::Errors execute() override
    {
        BLAZE_TRACE_LOG(Log::CONTROLLER, "[EnableAuditLoggingCommand].execute()");

        // check if the current user has the permission to enable audit logging
        if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CHANGE_AUDIT_LOGGING) &&
            !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CHANGE_AUDIT_LOGGING_UNRESTRICTED))
        {
            BLAZE_WARN_LOG(Log::CONTROLLER, "[EnableAuditLoggingCommand].execute: User [" << gCurrentUserSession->getBlazeId() << "] " 
                << " attempted to enable audit logging without permission!");

            return ERR_AUTHORIZATION_REQUIRED;
        }

        return commandErrorFromBlazeError(gController->enableAuditLogging(mRequest, UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CHANGE_AUDIT_LOGGING_UNRESTRICTED)));
    }

private:
    BlazeControllerSlave* mComponent;  // memory owned by creator, don't free
};

DEFINE_ENABLEAUDITLOGGING_CREATE_COMPNAME(Controller)

} // Blaze
