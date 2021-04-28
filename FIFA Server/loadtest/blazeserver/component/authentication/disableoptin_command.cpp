/*************************************************************************************************/
/*!
    \file   disableoptin_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class DisableOptInCommand 

    Add or delete the opt-in for the current user
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "authentication/authenticationimpl.h"
#include "authentication/rpc/authenticationslave/disableoptin_stub.h"
#include "authentication/tdf/authentication.h"

#include "authentication/util/setoptinutil.h"

namespace Blaze
{
namespace Authentication
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/
class DisableOptInCommand : public DisableOptInCommandStub
{
public:
    DisableOptInCommand(Message* message, OptInRequest *request, AuthenticationSlaveImpl* componentImpl);

    ~DisableOptInCommand() override {};

private:
    // Not owned memory.
    AuthenticationSlaveImpl* mComponent;

    // Owned memory
    SetOptInUtil mSetOptInUtil;

    // States
    DisableOptInCommandStub::Errors execute() override;
};

DEFINE_DISABLEOPTIN_CREATE();

DisableOptInCommand::DisableOptInCommand(Message* message, OptInRequest *request, AuthenticationSlaveImpl* componentImpl)
    : DisableOptInCommandStub(message,request),
    mComponent(componentImpl),
    mSetOptInUtil(*componentImpl, getPeerInfo(), gCurrentUserSession->getClientType())
{
}

/* Private methods *******************************************************************************/
DisableOptInCommandStub::Errors DisableOptInCommand::execute()
{
    if(mRequest.getOptInName()[0] == '\0')
    {
        TRACE_LOG("[DisableOptInCommand].execute has no the opt-in name.");
        return AUTH_ERR_OPTIN_NAME_REQUIRED;
    }

    AccountId accountId = gCurrentUserSession->getAccountId();

    BlazeRpcError error = mSetOptInUtil.SetOptIn(accountId, mRequest.getOptInName(), mComponent->getNucleusSourceHeader(gCurrentUserSession->getProductName()), false);

    return commandErrorFromBlazeError(error);
}

} // Authentication
} // Blaze
