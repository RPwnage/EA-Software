/*************************************************************************************************/
/*!
\file   getusergeoipdata_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class getusergeoipdata

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/getusergeoipdata_stub.h"

namespace Blaze
{

    class GetUserGeoIpDataCommand : public GetUserGeoIpDataCommandStub
    {
    public:

        GetUserGeoIpDataCommand(Message* message, Blaze::GetUserGeoIpDataRequest* request, UserSessionsSlave* componentImpl)
            : GetUserGeoIpDataCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~GetUserGeoIpDataCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        GetUserGeoIpDataCommandStub::Errors execute() override
        {
            BLAZE_TRACE_LOG(Log::USER, "[GetUserGeoIpDataCommand].start()");

            if (EA_UNLIKELY((gUserSessionManager == nullptr) || (gUserSessionMaster == nullptr)))
            {
                return commandErrorFromBlazeError(Blaze::ERR_SYSTEM);
            }

            UserSessionMasterPtr session = gUserSessionMaster->getLocalSession(mRequest.getUserSessionId());
            if (session != nullptr)
            {
                if (mRequest.getBlazeId() != INVALID_BLAZE_ID)
                {
                    // If a valid BlazeId is supplied, use it to search the database for location overrides
                    BlazeRpcError err = gUserSessionManager->getGeoIpDataWithOverrides(mRequest.getBlazeId(), session->getConnectionAddr(), mResponse);
                    return commandErrorFromBlazeError(err);
                }

                // If no valid BlazeId is supplied, just fetch data from GeoIP without searching for overrides
                if (!gUserSessionManager->getGeoIpData(session->getConnectionAddr(), mResponse))
                    return GetUserGeoIpDataError::USER_ERR_GEOIP_LOOKUP_FAILED;
            }

            return GetUserGeoIpDataError::USER_ERR_GEOIP_LOOKUP_FAILED;
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_GETUSERGEOIPDATA_CREATE_COMPNAME(UserSessionManager)
}


