/*************************************************************************************************/
/*!
    \file   logout_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class LogoutCommand

    Cleans up a user session and disassociates it with the connection.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "authentication/authenticationimpl.h"
#include "authentication/rpc/authenticationslave/logout_stub.h"
#include "framework/connection/connection.h"
#include "framework/usersessions/usersession.h"
#include "framework/usersessions/usersessionmanager.h"


namespace Blaze
{
namespace Authentication
{

class LogoutCommand : public LogoutCommandStub
{
public:
    LogoutCommand(Message* message, AuthenticationSlaveImpl *component) : LogoutCommandStub(message), mComponent(component) {}
    ~LogoutCommand() override {};

private:
    // States
    LogoutCommandStub::Errors execute() override
    {
        // Copy the productName before logging out the session
        char8_t productName[MAX_PRODUCTNAME_LENGTH];
        blaze_strnzcpy(productName, gCurrentUserSession->getProductName(), sizeof(productName));

        // Get the current user info.
        BlazeRpcError error = gUserSessionMaster->destroyUserSession(getUserSessionId(), DISCONNECTTYPE_CLIENT_LOGOUT, 0, FORCED_LOGOUTTYPE_INVALID);

        if (error == Blaze::ERR_OK)
        {
            mComponent->getMetricsInfoObj()->mLogouts.increment(1, productName, getPeerInfo()->getClientType());
        }

        return (commandErrorFromBlazeError(error));  
    }

    AuthenticationSlaveImpl *mComponent;

};
DEFINE_LOGOUT_CREATE()

} // Authentication
} // Blaze
