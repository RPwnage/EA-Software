/*************************************************************************************************/
/*!
    \file   getuserpsntoken_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetUserPSNTokenCommand

    Gets an PSN token for the specified user.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "framework/oauth/oauthslaveimpl.h"
#include "framework/rpc/oauthslave/getuserpsntoken_stub.h"
#include "framework/oauth/oauthutil.h"

namespace Blaze
{
namespace OAuth
{

class GetUserPsnTokenCommand : public GetUserPsnTokenCommandStub
{
public:
    GetUserPsnTokenCommand(Message* message, GetUserPsnTokenRequest* request, OAuthSlaveImpl* componentImpl);

    ~GetUserPsnTokenCommand() override {};

private:
    OAuthSlaveImpl* mComponent;

    GetUserPsnTokenCommandStub::Errors execute() override;
};

DEFINE_GETUSERPSNTOKEN_CREATE()

GetUserPsnTokenCommand::GetUserPsnTokenCommand(
    Message* message, GetUserPsnTokenRequest* request, OAuthSlaveImpl* componentImpl)
    : GetUserPsnTokenCommandStub(message, request),
    mComponent(componentImpl)
{
}

GetUserPsnTokenCommandStub::Errors GetUserPsnTokenCommand::execute()
{
    return commandErrorFromBlazeError(mComponent->getUserPsnToken(mRequest, mResponse, OAuthUtil::getRealEndUserAddr(this)));
}

} // namespace OAuth
} // namespace Blaze
