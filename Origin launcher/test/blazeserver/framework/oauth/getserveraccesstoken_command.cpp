/*************************************************************************************************/
/*!
    \file   getserveraccesstoken_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetServerAccessTokenCommand

    Gets an access token for the server.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "framework/oauth/oauthslaveimpl.h"
#include "framework/rpc/oauthslave/getserveraccesstoken_stub.h"
#include "framework/oauth/oauthutil.h"

namespace Blaze
{
namespace OAuth
{
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

class GetServerAccessTokenCommand : public GetServerAccessTokenCommandStub
{
public:
    GetServerAccessTokenCommand(Message* message, GetServerAccessTokenRequest* request, OAuthSlaveImpl* componentImpl);

    ~GetServerAccessTokenCommand() override {};

private:
    OAuthSlaveImpl* mComponent;

    GetServerAccessTokenCommandStub::Errors execute() override;

    /* Private methods *******************************************************************************/
};

DEFINE_GETSERVERACCESSTOKEN_CREATE()

GetServerAccessTokenCommand::GetServerAccessTokenCommand(
    Message* message, GetServerAccessTokenRequest* request, OAuthSlaveImpl* componentImpl)
    : GetServerAccessTokenCommandStub(message, request),
    mComponent(componentImpl)
{
}

GetServerAccessTokenCommandStub::Errors GetServerAccessTokenCommand::execute()
{
    return commandErrorFromBlazeError(mComponent->getServerAccessToken(mRequest, mResponse));
}

} // namespace OAuth
} // namespace Blaze
