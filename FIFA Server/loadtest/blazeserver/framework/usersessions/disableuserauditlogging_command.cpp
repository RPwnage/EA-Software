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
#include "framework/rpc/usersessionsslave/disableuserauditlogging_stub.h"

namespace Blaze
{

class DisableUserAuditLoggingCommand : public DisableUserAuditLoggingCommandStub
{
public:

    DisableUserAuditLoggingCommand(Message* message, Blaze::DisableUserAuditLoggingRequest* request, UserSessionsSlave* componentImpl)
        :DisableUserAuditLoggingCommandStub(message, request),
            mComponent(componentImpl)
    {
    }

    ~DisableUserAuditLoggingCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:

    DisableUserAuditLoggingCommandStub::Errors execute() override
    {
        BLAZE_TRACE_LOG(Log::USER, "[DisableUserAuditLoggingCommand].execute()");

        // check if the current user has the permission to disable audit logging
        if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CHANGE_AUDIT_LOGGING) &&
            !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CHANGE_AUDIT_LOGGING_UNRESTRICTED))
        {
            BLAZE_WARN_LOG(Log::USER, "[DisableUserAuditLoggingCommand].execute: User [" << gCurrentUserSession->getBlazeId() << "] "
                << " attempted to disable audit logging for other users without permission!");

            return ERR_AUTHORIZATION_REQUIRED;
        }

        UpdateAuditLoggingRequest req;
        for (auto& blazeId : mRequest.getBlazeIdList())
        {
            AuditLogEntryPtr entry = req.getAuditLogEntries().pull_back();
            entry->setBlazeId(blazeId);
        }

        return commandErrorFromBlazeError(gController->disableAuditLogging(req));

    }

private:
    UserSessionsSlave* mComponent;  // memory owned by creator, don't free
};

DEFINE_DISABLEUSERAUDITLOGGING_CREATE_COMPNAME(UserSessionsSlaveStub)

} // Blaze
