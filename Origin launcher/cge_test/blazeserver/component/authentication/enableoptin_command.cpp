/*************************************************************************************************/
/*!
    \file   enableoptin_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class EnableOptInCommand 

    Add or delete the opt-in for the current user
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "authentication/authenticationimpl.h"
#include "authentication/rpc/authenticationslave/enableoptin_stub.h"
#include "authentication/tdf/authentication.h"

#include "authentication/util/setoptinutil.h"

namespace Blaze
{
namespace Authentication
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/
class EnableOptInCommand : public EnableOptInCommandStub
{
public:
    EnableOptInCommand(Message* message, OptInRequest *request, AuthenticationSlaveImpl* componentImpl);

    ~EnableOptInCommand() override {};

private:
    // Not owned memory.
    AuthenticationSlaveImpl* mComponent;

    // Owned memory
    SetOptInUtil mSetOptInUtil;

    // States
    EnableOptInCommandStub::Errors execute() override;
};

DEFINE_ENABLEOPTIN_CREATE();

EnableOptInCommand::EnableOptInCommand(Message* message, OptInRequest *request, AuthenticationSlaveImpl* componentImpl)
    : EnableOptInCommandStub(message,request),
    mComponent(componentImpl),
    mSetOptInUtil(*componentImpl, getPeerInfo(), gCurrentUserSession->getClientType())
{
}

/* Private methods *******************************************************************************/
EnableOptInCommandStub::Errors EnableOptInCommand::execute()
{
    if(mRequest.getOptInName()[0] == '\0')
    {
        TRACE_LOG("[EnableOptInCommand].execute has no the opt-in name.");
        return AUTH_ERR_OPTIN_NAME_REQUIRED;
    }

    AccountId accountId = gCurrentUserSession->getAccountId();

    BlazeRpcError error = mSetOptInUtil.SetOptIn(accountId, mRequest.getOptInName(), mComponent->getNucleusSourceHeader(gCurrentUserSession->getProductName()), true);

    return commandErrorFromBlazeError(error);
}

} // Authentication
} // Blaze
