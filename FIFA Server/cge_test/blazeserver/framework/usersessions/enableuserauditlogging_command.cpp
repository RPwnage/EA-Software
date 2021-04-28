/*************************************************************************************************/
/*!
    \file   enableuserauditlogging_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class EnableUserAuditLoggingCommand

    Enable audit logging for the passed in list of users.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/controller/controller.h"
#include "framework/rpc/usersessionsslave/enableuserauditlogging_stub.h"

namespace Blaze
{

class EnableUserAuditLoggingCommand : public EnableUserAuditLoggingCommandStub
{
public:

    EnableUserAuditLoggingCommand(Message* message, Blaze::EnableUserAuditLoggingRequest* request, UserSessionsSlave* componentImpl)
        :EnableUserAuditLoggingCommandStub(message, request),
            mComponent(componentImpl)
    {
    }

    ~EnableUserAuditLoggingCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:

    EnableUserAuditLoggingCommandStub::Errors execute() override
    {
        BLAZE_TRACE_LOG(Log::USER, "[EnableUserAuditLoggingCommand].execute()");

        // check if the current user has the permission to enable audit logging
        if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CHANGE_AUDIT_LOGGING) &&
            !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CHANGE_AUDIT_LOGGING_UNRESTRICTED))
        {
            BLAZE_WARN_LOG(Log::USER, "[EnableUserAuditLoggingCommand].execute: User [" << gCurrentUserSession->getBlazeId() << "] "
                << " attempted to enable audit logging for other users without permission!");

            return ERR_AUTHORIZATION_REQUIRED;
        }

        UpdateAuditLoggingRequest req;
        for (auto& blazeId : mRequest.getBlazeIdList())
        {
            AuditLogEntryPtr entry = req.getAuditLogEntries().pull_back();
            entry->setBlazeId(blazeId);
        }

        return commandErrorFromBlazeError(gController->enableAuditLogging(req));
    }

private:
    UserSessionsSlave* mComponent;  // memory owned by creator, don't free
};

DEFINE_ENABLEUSERAUDITLOGGING_CREATE_COMPNAME(UserSessionsSlaveStub)

} // Blaze
