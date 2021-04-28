/*************************************************************************************************/
/*!
    \file   getuserxbltoken_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetUserXBLTokenCommand

    Gets an XBL token for the specified user.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "framework/oauth/oauthslaveimpl.h"
#include "framework/rpc/oauthslave/getuserxbltoken_stub.h"
#include "framework/oauth/oauthutil.h"
#include "framework/usersessions/usersessionmanager.h"

namespace Blaze
{
namespace OAuth
{
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/


class GetUserXblTokenCommand : public GetUserXblTokenCommandStub
{
public:
    GetUserXblTokenCommand(Message* message, GetUserXblTokenRequest* request, OAuthSlaveImpl* componentImpl);

    ~GetUserXblTokenCommand() override {};

private:
    OAuthSlaveImpl* mComponent;

    GetUserXblTokenCommandStub::Errors execute() override;

    /* Private methods *******************************************************************************/
};

DEFINE_GETUSERXBLTOKEN_CREATE()

GetUserXblTokenCommand::GetUserXblTokenCommand(
    Message* message, GetUserXblTokenRequest* request, OAuthSlaveImpl* componentImpl)
    : GetUserXblTokenCommandStub(message, request),
    mComponent(componentImpl)
{
}

GetUserXblTokenCommandStub::Errors GetUserXblTokenCommand::execute()
{
    BlazeRpcError error = mComponent->getUserXblToken(mRequest, mResponse, OAuthUtil::getRealEndUserAddr(this));


    if (getPeerInfo() == nullptr || getPeerInfo()->getClientType() != CLIENT_TYPE_DEDICATED_SERVER)
    {
        if ((error == OAUTH_ERR_INVALID_USER) || (error == OAUTH_ERR_NO_XBLTOKEN))
        {
            gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(error), RiverPoster::authentication_failed);
        }
        else if (error == ERR_COULDNT_CONNECT)
        {
            gUserSessionManager->sendPINErrorEvent(RiverPoster::ExternalSystemUnavailableErrToString(RiverPoster::NUCLEUS_SERVICE_UNAVAILABLE), RiverPoster::external_system_unavailable);
        }
    }

    return commandErrorFromBlazeError(error);
}

} // namespace OAuth
} // namespace Blaze
