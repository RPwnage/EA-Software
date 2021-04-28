/*************************************************************************************************/
/*!
    \file   getoptin_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetOptInCommand 

    Retrieve the list of opt-in that the current user has
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "authentication/authenticationimpl.h"
#include "authentication/rpc/authenticationslave/getoptin_stub.h"
#include "authentication/tdf/authentication.h"

#include "authentication/util/getoptinutil.h"

namespace Blaze
{
namespace Authentication
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/
class GetOptInCommand : public GetOptInCommandStub
{
public:
    GetOptInCommand(Message* message, OptInRequest *request, AuthenticationSlaveImpl* componentImpl);

    ~GetOptInCommand() override {};

private:
    // Not owned memory.
    AuthenticationSlaveImpl* mComponent;

    // Owned memory
    GetOptInUtil mGetOptInUtil;

    // States
    GetOptInCommandStub::Errors execute() override;
};

DEFINE_GETOPTIN_CREATE();

GetOptInCommand::GetOptInCommand(Message* message, OptInRequest *request, AuthenticationSlaveImpl* componentImpl)
    : GetOptInCommandStub(message,request),
    mComponent(componentImpl),
    mGetOptInUtil(*componentImpl, getPeerInfo(), gCurrentUserSession->getClientType())
{
}

/* Private methods *******************************************************************************/
GetOptInCommandStub::Errors GetOptInCommand::execute()
{
    if(mRequest.getOptInName()[0]  == '\0')
    {
        TRACE_LOG("[GetOptInCommand].execute has no the opt-in name.");
        return AUTH_ERR_OPTIN_NAME_REQUIRED;
    }

    AccountId accountId = gCurrentUserSession->getAccountId();

    BlazeRpcError error = mGetOptInUtil.getOptIn(accountId, mRequest.getOptInName(), mResponse);
   
    return  commandErrorFromBlazeError(error);
}

} // Authentication
} // Blaze
