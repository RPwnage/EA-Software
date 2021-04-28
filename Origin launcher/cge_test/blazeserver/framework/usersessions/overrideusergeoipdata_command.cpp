/*************************************************************************************************/
/*!
\file   lookupusergeoipdata_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class lookupusergeoipdata



    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/overrideusergeoipdata_stub.h"
#include "framework/protocol/shared/httpparam.h"
#include "framework/protocol/shared/httpprotocolutil.h"
#include "framework/protocol/outboundhttpresult.h"
#include "framework/connection/outboundhttpconnectionmanager.h"


namespace Blaze
{
    // use these to block junk data from getting into the user session
    static const float MAX_DEGREES_LONGITUDE = 180;
    static const float MIN_DEGREES_LONGITUDE = -180;
    static const float MAX_DEGREES_LATTITUDE = 90;
    static const float MIN_DEGREES_LATTITUDE = -90;

    class OverrideUserGeoIPDataCommand : public OverrideUserGeoIPDataCommandStub
    {
    public:

        OverrideUserGeoIPDataCommand(Message* message, Blaze::GeoLocationData* request, UserSessionsSlave* componentImpl)
            : OverrideUserGeoIPDataCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~OverrideUserGeoIPDataCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        OverrideUserGeoIPDataCommandStub::Errors execute() override
        {
            BLAZE_TRACE_LOG(Log::USER, "[OverrideUserGeoIPDataCommand].execute()");

            if (gUserSessionManager == nullptr || gCurrentUserSession == nullptr)
            {
                return commandErrorFromBlazeError(Blaze::ERR_SYSTEM);
            }

            if (mRequest.getBlazeId() != INVALID_BLAZE_ID)
            {
                 if (mRequest.getBlazeId() != gCurrentUserSession->getBlazeId() &&
                     !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_OVERRIDE_GEOIP_DATA))
                 {
                     BLAZE_WARN_LOG(Log::USER, "[OverrideUserGeoIPDataCommand].execute: User " << gCurrentUserSession->getBlazeId() 
                         << " attempted to override geo-data of the specified user (blazeId=" << mRequest.getBlazeId() << "), no permission!");
                     return commandErrorFromBlazeError(Blaze::ERR_AUTHORIZATION_REQUIRED);
                 }
            }
            else
            {
                // blaze id supplied by client is ignored
                mRequest.setBlazeId(gCurrentUserSession->getBlazeId());
            }

            if (( mRequest.getLatitude() > MAX_DEGREES_LATTITUDE || mRequest.getLatitude() < MIN_DEGREES_LATTITUDE ) || 
                ( mRequest.getLongitude() > MAX_DEGREES_LONGITUDE || mRequest.getLongitude() < MIN_DEGREES_LONGITUDE ))
            {
                BLAZE_WARN_LOG(Log::USER, "[OverrideUserGeoIPDataCommand].execute: "
                    << "latitude or longitude was out of bounds.");
                return GEOIP_INCOMPLETE_PARAMETERS;
            }

            return commandErrorFromBlazeError(gUserSessionManager->sessionOverrideGeoIPById(mRequest));
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_OVERRIDEUSERGEOIPDATA_CREATE_COMPNAME(UserSessionManager)
}




