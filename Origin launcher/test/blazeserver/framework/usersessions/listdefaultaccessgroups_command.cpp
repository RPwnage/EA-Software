/*************************************************************************************************/
/*!
\file   listdefaultaccessgroups_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class ListDefaultAccessGroupCommand 

return the default group for each client type.

\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/listdefaultaccessgroup_stub.h"

namespace Blaze
{
    class ListDefaultAccessGroupCommand : public ListDefaultAccessGroupCommandStub
    {
    public:
        ListDefaultAccessGroupCommand(Message* message, UserSessionsSlave* componentImpl)
            : ListDefaultAccessGroupCommandStub(message),
            mComponent(componentImpl)
        {
        }

        ~ListDefaultAccessGroupCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:
        ListDefaultAccessGroupCommandStub::Errors execute() override
        {
            BLAZE_TRACE_LOG(Log::USER, "[ListDefaultAccessGroupCommand].start()");
            if(EA_UNLIKELY(gUserSessionManager == nullptr))
            {
                return ERR_SYSTEM;
            }

            // check if the current user has the permission to get the auth group name
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_GET_AUTHGROUP_NAME))
            {
                BLAZE_WARN_LOG(Log::USER, "[ListDefaultAccessGroupCommand].execute: User " << gCurrentUserSession->getBlazeId()
                    << " attempted to get the default access group list, no permission!");

                return ERR_AUTHORIZATION_REQUIRED;
            }

            mResponse.getDefaultAccessGroups().reserve(Blaze::CLIENT_TYPE_INVALID);
            //read the data from cache
            for (ClientType thisClientType = Blaze::CLIENT_TYPE_GAMEPLAY_USER;
                 thisClientType < Blaze::CLIENT_TYPE_INVALID;
                 thisClientType = Blaze::ClientType(thisClientType + 1))
            {
                ClientTypeAccessGroup* currentGroup = mResponse.getDefaultAccessGroups().pull_back();
                currentGroup->setClientType(thisClientType);
                currentGroup->setGroupName(gUserSessionManager->getGroupManager()->getDefaultGroupName(thisClientType)->c_str());
            }

            BLAZE_INFO_LOG(Log::USER, "[ListDefaultAccessGroupCommand].execute: User " << gCurrentUserSession->getBlazeId() 
                << " got the default access group list, had permission.");

            return ListDefaultAccessGroupCommand::ERR_OK;
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_LISTDEFAULTACCESSGROUP_CREATE_COMPNAME(UserSessionManager)
} // Blaze
