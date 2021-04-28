/*************************************************************************************************/
/*!
\file   listauthorization_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class ListAuthorizationCommand 

return the full list of users with non default permissions, including email, group and client type

\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/listauthorization_stub.h"

namespace Blaze
{
    class ListAuthorizationCommand : public ListAuthorizationCommandStub
    {
    public:
        ListAuthorizationCommand(Message* message, UserSessionsSlave* componentImpl)
            : ListAuthorizationCommandStub(message),
            mComponent(componentImpl)
        {
        }

        ~ListAuthorizationCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:
        ListAuthorizationCommandStub::Errors execute() override
        {
            BLAZE_TRACE_LOG(Log::USER, "[ListAuthorizationCommand].start()");
            if(EA_UNLIKELY(gUserSessionManager == nullptr))
            {
                return ERR_SYSTEM;
            }

            // check if the current user has the permission to get the auth group name
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_GET_AUTHGROUP_NAME))
            {
                BLAZE_WARN_LOG(Log::USER, "[ListAuthorizationCommand].execute: User " << gCurrentUserSession->getBlazeId() 
                    << " attempted to get the authorization list, no permission!");

                return ERR_AUTHORIZATION_REQUIRED;
            }

            gUserSessionManager->getGroupManager()->getAuthorizationList(mResponse.getAuthorizationList());

            BLAZE_INFO_LOG(Log::USER, "[ListAuthorizationCommand].execute: User " << gCurrentUserSession->getBlazeId() 
                << " got the authorization list, had permission.");

            return ListAuthorizationCommandStub::ERR_OK;
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_LISTAUTHORIZATION_CREATE_COMPNAME(UserSessionManager)
} // Blaze
