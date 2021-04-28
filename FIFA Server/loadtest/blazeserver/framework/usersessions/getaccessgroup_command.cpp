/*************************************************************************************************/
/*!
    \file   getaccessgroup_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \classGetAccessGroupCommand 

    process user session extended data retrieval requests from the client.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/getaccessgroup_stub.h"

namespace Blaze
{
    class GetAccessGroupCommand : public GetAccessGroupCommandStub
    {
    public:
       GetAccessGroupCommand(Message* message, Blaze::GetAccessGroupRequest* request, UserSessionsSlave* componentImpl)
            :GetAccessGroupCommandStub(message, request),
              mComponent(componentImpl)
        {
        }

        ~GetAccessGroupCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:
       GetAccessGroupCommandStub::Errors execute() override
        {
            BLAZE_TRACE_LOG(Log::USER, "[GetPermissionsCommand].start()");
            if(EA_UNLIKELY(gUserSessionManager == nullptr))
            {
                return ERR_SYSTEM;
            }

            // check if the current user has the permission to get the auth group name
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_GET_AUTHGROUP_NAME))
            {
                BLAZE_WARN_LOG(Log::USER, "[GetAccessGroupCommand].execute: User " << gCurrentUserSession->getBlazeId() 
                    << " attempted to get access group name for given external key[" << mRequest.getUserKeyTypeInfo().getExternalKey() 
                    << "] and client type[" << mRequest.getUserKeyTypeInfo().getClientType() << "], no permission!");

                return ERR_AUTHORIZATION_REQUIRED;
            }

            AccessGroupName groupName;
            const AccessGroupName* groupNamePtr = &groupName;
            // get default group for the user
            groupNamePtr = gUserSessionManager->getGroupManager()->getDefaultGroupName(mRequest.getUserKeyTypeInfo().getClientType());
            

            if (groupNamePtr == nullptr || groupNamePtr->c_str()[0] == '\0')
            {
                BLAZE_WARN_LOG(Log::USER, "[GetAccessGroupCommandStub] failed to get access group name for given external key("
                    << mRequest.getUserKeyTypeInfo().getExternalKey() << ") and client type("
                    << mRequest.getUserKeyTypeInfo().getClientType() << ")\n.");
                return ACCESS_GROUP_ERR_NO_GROUP_FOUND;
            }

            mResponse.setGroupName(groupNamePtr->c_str());

            BLAZE_INFO_LOG(Log::USER, "[GetAccessGroupCommand].execute: User " << gCurrentUserSession->getBlazeId() 
                << " got access group name for given external key[" << mRequest.getUserKeyTypeInfo().getExternalKey() << "] and client type[" 
                << mRequest.getUserKeyTypeInfo().getClientType() << "], had permission.");

            return ERR_OK;
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_GETACCESSGROUP_CREATE_COMPNAME(UserSessionManager)
} // Blaze
