/*************************************************************************************************/
/*!
\file   setusergeooptin_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class setusergeooptin



    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/setusergeooptin_stub.h"


namespace Blaze
{
    class SetUserGeoOptInCommand : public SetUserGeoOptInCommandStub
    {
    public:

        SetUserGeoOptInCommand(Message* message, Blaze::OptInRequest* request, UserSessionsSlave* componentImpl)
            : SetUserGeoOptInCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~SetUserGeoOptInCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        SetUserGeoOptInCommandStub::Errors execute() override
        {
            if (gUserSessionManager == nullptr || gCurrentUserSession == nullptr)
            {                
                return commandErrorFromBlazeError(Blaze::ERR_SYSTEM);
            }

            if (mRequest.getBlazeId() != INVALID_BLAZE_ID)
            {
                 if (mRequest.getBlazeId() != gCurrentUserSession->getBlazeId() &&
                     !gCurrentUserSession->isAuthorized(Blaze::Authorization::PERMISSION_OVERRIDE_GEOIP_DATA))
                 {
                     BLAZE_WARN_LOG(Log::USER, "[SetUserGeoOptInCommand].execute: User " << gCurrentUserSession->getBlazeId() 
                         << " attempted to override geo-data of the specified user (blazeId=" << mRequest.getBlazeId() << "), no permission!");
                     return commandErrorFromBlazeError(Blaze::ERR_AUTHORIZATION_REQUIRED);
                 }
            }
            else
            {
                // blaze id supplied by client is ignored
                mRequest.setBlazeId(gCurrentUserSession->getBlazeId());
            }

            // Not allow child account to override geo-data
            UserInfoPtr userInfo;
            BlazeRpcError lookupError = gUserSessionManager->lookupUserInfoByBlazeId(mRequest.getBlazeId(), userInfo);
            if (lookupError != Blaze::ERR_OK)
            {
                return commandErrorFromBlazeError(lookupError);
            }

            if (userInfo->getIsChildAccount() != 0)
            {
                 BLAZE_WARN_LOG(Log::USER, "[OverrideUserGeoIPDataCommand].execute: User " << mRequest.getBlazeId() 
                    << " is a child account who has no permission to override geo-data!");
                return commandErrorFromBlazeError(Blaze::ERR_AUTHORIZATION_REQUIRED);
            }

            return commandErrorFromBlazeError(gUserSessionManager->setUserGeoOptIn(mRequest.getBlazeId(), userInfo->getPlatformInfo().getClientPlatform(), mRequest.getOptIn()));
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_SETUSERGEOOPTIN_CREATE_COMPNAME(UserSessionManager)
}




