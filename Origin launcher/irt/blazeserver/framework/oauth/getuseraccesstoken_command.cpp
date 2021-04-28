/*************************************************************************************************/
/*!
    \file   getuseraccesstoken_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetUserAccessTokenCommand

    Gets an access token for the specified user.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "framework/oauth/oauthslaveimpl.h"
#include "framework/rpc/oauthslave/getuseraccesstoken_stub.h"
#include "framework/oauth/oauthutil.h"
#include "framework/connection/inetaddress.h"

namespace Blaze
{
namespace OAuth
{
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

class GetUserAccessTokenCommand : public GetUserAccessTokenCommandStub
{
public:
    GetUserAccessTokenCommand(Message* message, GetUserAccessTokenRequest* request, OAuthSlaveImpl* componentImpl);

    ~GetUserAccessTokenCommand() override {};

private:
    OAuthSlaveImpl* mComponent;

    GetUserAccessTokenCommandStub::Errors execute() override;

    /* Private methods *******************************************************************************/
};

DEFINE_GETUSERACCESSTOKEN_CREATE()

GetUserAccessTokenCommand::GetUserAccessTokenCommand(
    Message* message, GetUserAccessTokenRequest* request, OAuthSlaveImpl* componentImpl)
    : GetUserAccessTokenCommandStub(message, request),
    mComponent(componentImpl)
{
}

GetUserAccessTokenCommandStub::Errors GetUserAccessTokenCommand::execute()
{
    /*InetAddress inetAddr =*/ OAuthUtil::getRealEndUserAddr(this);
    return commandErrorFromBlazeError(mComponent->getUserAccessToken(mRequest, mResponse, OAuthUtil::getRealEndUserAddr(this)));
}

} // namespace OAuth
} // namespace Blaze
