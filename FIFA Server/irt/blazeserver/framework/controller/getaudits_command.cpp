/*************************************************************************************************/
/*!
    \file   getaudits_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetAuditsCommand

    Gets the current audit entries.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/blazecontrollerslave/getaudits_stub.h"

namespace Blaze
{

class GetAuditsCommand : public GetAuditsCommandStub
{
public:
    GetAuditsCommand(Message* message, BlazeControllerSlave* componentImpl)
        : GetAuditsCommandStub(message),
        mComponent(componentImpl)
    {
    }

    ~GetAuditsCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    GetAuditsCommandStub::Errors execute() override
    {
        BLAZE_TRACE_LOG(Log::CONTROLLER, "[GetAuditsCommand].execute()");

        // check if the current user has the permission to enable audit logging
        if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_GET_AUDIT_LOGGING))
        {
            BLAZE_WARN_LOG(Log::CONTROLLER, "[GetAuditsCommand].execute: User [" << gCurrentUserSession->getBlazeId() << "] "
                << " attempted to get current audit logging without permission!");

            return ERR_AUTHORIZATION_REQUIRED;
        }

        Logger::getAudits(mResponse.getAuditLogEntries());
        return commandErrorFromBlazeError(ERR_OK);
    }

private:
    BlazeControllerSlave* mComponent;  // memory owned by creator, don't free
};

DEFINE_GETAUDITS_CREATE_COMPNAME(Controller)

} // Blaze
