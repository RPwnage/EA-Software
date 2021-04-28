/*************************************************************************************************/
/*!
    \file   lookupusersessionid_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class LookupUserSessionIdCommand

    Lookup the user session id by the user's blaze id.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/lookupusersessionid_stub.h"

namespace Blaze
{

    class LookupUserSessionIdCommand : public LookupUserSessionIdCommandStub
    {
    public:

       LookupUserSessionIdCommand(Message* message, Blaze::LookupUserSessionIdRequest* request, UserSessionsSlave* componentImpl)
            :LookupUserSessionIdCommandStub(message, request),
              mComponent(componentImpl)
        {
        }

        ~LookupUserSessionIdCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

       LookupUserSessionIdCommandStub::Errors execute() override
        {
            BLAZE_TRACE_LOG(Log::USER, "[LookupUserSessionIdCommand].start()");

            eastl::string callerNamespace;
            bool crossPlatformOptIn;
            const ClientPlatformSet* platformSet = nullptr;
            BlazeRpcError err = UserSessionManager::setPlatformsAndNamespaceForCurrentUserSession(platformSet, callerNamespace, crossPlatformOptIn);
            if (err != ERR_OK)
                return commandErrorFromBlazeError(err);

            //lookup user by blaze id
            UserInfoPtr userInfo = nullptr;
            err = gUserSessionManager->lookupUserInfoByBlazeId(mRequest.getBlazeId(), userInfo);
            if (err != ERR_OK)
                return commandErrorFromBlazeError(err);
            else if (userInfo == nullptr)
                return LookupUserSessionIdCommandStub::USER_ERR_USER_NOT_FOUND;

            // Make sure the caller is permitted to look up users on this platform
            ClientPlatformType callerPlatform = gCurrentUserSession->getClientPlatform();
            if (err == ERR_OK && callerPlatform != common)
            {
                if (userInfo->getPlatformInfo().getClientPlatform() != callerPlatform)
                {
                    const ClientPlatformSet& unrestrictedPlatforms = gUserSessionManager->getUnrestrictedPlatforms(callerPlatform);
                    if (unrestrictedPlatforms.find(userInfo->getPlatformInfo().getClientPlatform()) == unrestrictedPlatforms.end())
                        return LookupUserSessionIdCommandStub::USER_ERR_DISALLOWED_PLATFORM;
                    else if (!crossPlatformOptIn)
                        return LookupUserSessionIdCommandStub::USER_ERR_CROSS_PLATFORM_OPTOUT;
                }
            }

            UserSessionIdList ids;
            err = gUserSessionManager->getSessionIds(mRequest.getBlazeId(), ids);
            for(UserSessionIdList::const_iterator it = ids.begin(); it != ids.end(); ++it)
            {
                UserSessionId sessionId = *it;
                if(sessionId != INVALID_USER_SESSION_ID)
                    mResponse.getUsersessionidList().push_back(sessionId);
            }

            return commandErrorFromBlazeError(err);
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    
    DEFINE_LOOKUPUSERSESSIONID_CREATE_COMPNAME(UserSessionManager)
   


} // Blaze
