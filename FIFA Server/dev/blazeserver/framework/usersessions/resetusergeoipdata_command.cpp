/*************************************************************************************************/
/*!
\file   resetusergeoipdata_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class resetusergeoipdata



    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/resetusergeoipdata_stub.h"


namespace Blaze
{
    class ResetUserGeoIPDataCommand : public ResetUserGeoIPDataCommandStub
    {
    public:

        ResetUserGeoIPDataCommand(Message* message,  Blaze::ResetUserGeoIPDataRequest* request, UserSessionsSlave* componentImpl)
            : ResetUserGeoIPDataCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~ResetUserGeoIPDataCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        ResetUserGeoIPDataCommandStub::Errors execute() override
        {
            BLAZE_TRACE_LOG(Log::USER, "[ResetUserGeoIPDataCommand].start()");

            if(gUserSessionManager == nullptr || gCurrentUserSession == nullptr)
            {                
                return commandErrorFromBlazeError(Blaze::ERR_SYSTEM);
            }

            Blaze::BlazeId blazeId;
            if (mRequest.getBlazeId() != INVALID_BLAZE_ID)
            {
                if (mRequest.getBlazeId() != gCurrentUserSession->getBlazeId() &&
                    !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_OVERRIDE_GEOIP_DATA))
                {
                    BLAZE_WARN_LOG(Log::USER, "[ResetUserGeoIPDataCommand].execute: User " << gCurrentUserSession->getBlazeId() 
                        << " attempted to reset geo-data of the specified user (blazeId=" << mRequest.getBlazeId() << "), no permission!");
                    return commandErrorFromBlazeError(Blaze::ERR_AUTHORIZATION_REQUIRED);
                }
                blazeId = mRequest.getBlazeId();
            }
            else
            {
                // blaze id supplied by client is ignored
                blazeId = gCurrentUserSession->getBlazeId();
            }
            return commandErrorFromBlazeError(gUserSessionManager->sessionResetGeoIPById(blazeId));
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_RESETUSERGEOIPDATA_CREATE_COMPNAME(UserSessionManager)
}




