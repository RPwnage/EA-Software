/*************************************************************************************************/
/*!
    \file   getdevicelocality_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetDeviceLocalityCommand

    Get the device type: cloud vs local by the user's session Id.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/getdevicelocality_stub.h"

namespace Blaze
{

    class GetDeviceLocalityCommand : public GetDeviceLocalityCommandStub
    {
    public:

        GetDeviceLocalityCommand(Message* message, UserSessionsSlave* componentImpl)
            :GetDeviceLocalityCommandStub(message),
            mComponent(componentImpl)
        {
        }

        ~GetDeviceLocalityCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        GetDeviceLocalityCommandStub::Errors execute() override
        {
            BLAZE_TRACE_LOG(Log::USER, "[GetDeviceLocalityCommand].start()");

            DeviceLocality deviceLocality = gUserSessionManager->getDeviceLocality(UserSession::getCurrentUserSessionId());
            mResponse.setDeviceLocality(deviceLocality);
            return commandErrorFromBlazeError(ERR_OK);
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };


    DEFINE_GETDEVICELOCALITY_CREATE_COMPNAME(UserSessionManager)



} // Blaze
