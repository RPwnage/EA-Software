/*************************************************************************************************/
/*!
    \file   getuniquedeviceid_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetUniqueDeviceIdCommand

    Get the unique device id by the user's session id.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/getuniquedeviceid_stub.h"

namespace Blaze
{

    class GetUniqueDeviceIdCommand : public GetUniqueDeviceIdCommandStub
    {
    public:

       GetUniqueDeviceIdCommand(Message* message, UserSessionsSlave* componentImpl)
            :GetUniqueDeviceIdCommandStub(message),
              mComponent(componentImpl)
        {
        }

        ~GetUniqueDeviceIdCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

       GetUniqueDeviceIdCommandStub::Errors execute() override
        {
            BLAZE_TRACE_LOG(Log::USER, "[GetUniqueDeviceIdCommand].start()");

            const char8_t* deviceId = gUserSessionManager->getUniqueDeviceId(UserSession::getCurrentUserSessionId());
            if (deviceId != nullptr)
            {
                mResponse.setUniqueDeviceId(deviceId);
            }
            return commandErrorFromBlazeError(ERR_OK);
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    
    DEFINE_GETUNIQUEDEVICEID_CREATE_COMPNAME(UserSessionManager)
   


} // Blaze
