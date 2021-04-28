/*************************************************************************************************/
/*!
    \file   getidentitycontext_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetIdentityContextCommand

    Gets an identity context from provided request.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "framework/oauth/oauthslaveimpl.h"
#include "framework/rpc/oauthslave/getidentitycontext_stub.h"
#include "framework/oauth/oauthutil.h"

namespace Blaze
{
namespace OAuth
{
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

class GetIdentityContextCommand : public GetIdentityContextCommandStub
{
public:
    GetIdentityContextCommand(Message* message, IdentityContextRequest* request, OAuthSlaveImpl* componentImpl);

    ~GetIdentityContextCommand() override {};

private:
    OAuthSlaveImpl* mComponent;

    GetIdentityContextCommandStub::Errors execute() override;

    /* Private methods *******************************************************************************/
};

DEFINE_GETIDENTITYCONTEXT_CREATE()

GetIdentityContextCommand::GetIdentityContextCommand(
    Message* message, IdentityContextRequest* request, OAuthSlaveImpl* componentImpl)
    : GetIdentityContextCommandStub(message, request),
    mComponent(componentImpl)
{
}

GetIdentityContextCommandStub::Errors GetIdentityContextCommand::execute()
{
    return commandErrorFromBlazeError(mComponent->getIdentityContext(mRequest, mResponse));
}

} // namespace OAuth
} // namespace Blaze
