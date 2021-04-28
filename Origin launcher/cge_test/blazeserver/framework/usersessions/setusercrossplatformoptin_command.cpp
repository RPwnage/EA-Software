/*************************************************************************************************/
/*!
\file   setusercrossplatformoptin_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class setusercrossplatformoptin



    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/setusercrossplatformoptin_stub.h"


namespace Blaze
{
    class SetUserCrossPlatformOptInCommand : public SetUserCrossPlatformOptInCommandStub
    {
    public:

        SetUserCrossPlatformOptInCommand(Message* message, Blaze::OptInRequest* request, UserSessionsSlave* componentImpl)
            : SetUserCrossPlatformOptInCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~SetUserCrossPlatformOptInCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        SetUserCrossPlatformOptInCommand::Errors execute() override
        {
            if (gUserSessionManager == nullptr || gCurrentUserSession == nullptr)
            {                
                return commandErrorFromBlazeError(Blaze::ERR_SYSTEM);
            }

            if (mRequest.getBlazeId() != INVALID_BLAZE_ID)
            {
                 if (mRequest.getBlazeId() != gCurrentUserSession->getBlazeId() && !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_OVERRIDE_CROSSPLAY_OPTIN))
                 {
                     BLAZE_WARN_LOG(Log::USER, "[SetUserCrossPlatformOptInCommand].execute: User " << gCurrentUserSession->getBlazeId() 
                         << " attempted to override cross-platform opt-in of some other specified user (blazeId=" << mRequest.getBlazeId() << "), no permission!");
                     return commandErrorFromBlazeError(Blaze::ERR_AUTHORIZATION_REQUIRED);
                 }
            }
            else
            {
                // blaze id supplied by client is ignored
                mRequest.setBlazeId(gCurrentUserSession->getBlazeId());
            }

            UserInfoPtr userInfo;
            BlazeRpcError lookupError = gUserSessionManager->lookupUserInfoByBlazeId(mRequest.getBlazeId(), userInfo);
            if (lookupError != Blaze::ERR_OK)
            {
                return commandErrorFromBlazeError(lookupError);
            }

            return commandErrorFromBlazeError(gUserSessionManager->updateUserCrossPlatformOptIn(mRequest, userInfo->getPlatformInfo().getClientPlatform()));
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_SETUSERCROSSPLATFORMOPTIN_CREATE_COMPNAME(UserSessionManager)
}




