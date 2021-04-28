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
#include "framework/rpc/usersessionsslave/lookupusergeoipdata_stub.h"

namespace Blaze
{

    class LookupUserGeoIPDataCommand : public LookupUserGeoIPDataCommandStub
    {
    public:

        LookupUserGeoIPDataCommand(Message* message, Blaze::UserIdentification* request, UserSessionsSlave* componentImpl)
            : LookupUserGeoIPDataCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~LookupUserGeoIPDataCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        LookupUserGeoIPDataCommandStub::Errors execute() override
        {
            BLAZE_TRACE_LOG(Log::USER, "[LookupUserGeoIPDataCommand].start()");

            if(EA_UNLIKELY(gUserSessionManager == nullptr))
            {                
                return commandErrorFromBlazeError(Blaze::ERR_SYSTEM);
            }
                
            mResponse.setBlazeId(mRequest.getBlazeId());
            return commandErrorFromBlazeError(gUserSessionManager->fetchGeoIPDataById(mResponse));
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_LOOKUPUSERGEOIPDATA_CREATE_COMPNAME(UserSessionManager)
}


