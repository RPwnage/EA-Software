/*************************************************************************************************/
/*!
    \file   decrementusecount_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "authentication/authenticationimpl.h"
#include "authentication/rpc/authenticationslave/decrementusecount_stub.h"
#include "authentication/tdf/authentication.h"

#include "authentication/util/decremententitlementusecountutil.h"

namespace Blaze
{
namespace Authentication
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/
class DecrementUseCountCommand : public DecrementUseCountCommandStub
{
public:
    DecrementUseCountCommand(Message* message, DecrementUseCountRequest* request, AuthenticationSlaveImpl* componentImpl);

    ~DecrementUseCountCommand() override {};

private:
    // Not owned memory.
    AuthenticationSlaveImpl* mComponent;

    // Owned memory
    DecrementEntitlementUseCountUtil mDecrementUsecountUtil;

    // States
    DecrementUseCountCommandStub::Errors execute() override;

};
DEFINE_DECREMENTUSECOUNT_CREATE()

DecrementUseCountCommand::DecrementUseCountCommand(Message* message, DecrementUseCountRequest* request, AuthenticationSlaveImpl* componentImpl)
    : DecrementUseCountCommandStub(message, request),
    mComponent(componentImpl),
    mDecrementUsecountUtil(*componentImpl, getPeerInfo(), gCurrentUserSession->getClientType())
{
}

/* Private methods *******************************************************************************/
DecrementUseCountCommandStub::Errors DecrementUseCountCommand::execute()
{
    // entitlement tag is required
    if (mRequest.getEntitlementTag()[0] == '\0')
    {
        TRACE_LOG("[DecrementUseCountCommandStub].execute() - empty entitlement tag.");
        gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(AUTH_ERR_ENTITLEMETNTAG_EMPTY), RiverPoster::entitlement_modify_failed);
        return AUTH_ERR_ENTITLEMETNTAG_EMPTY;
    }

    uint32_t consumedUseCount = 0;
    uint32_t remainedUseCount = 0;
    BlazeRpcError error = mDecrementUsecountUtil.decrementEntitlementUseCount(gCurrentUserSession->getAccountId(), mRequest, consumedUseCount, remainedUseCount);
    if (error != Blaze::ERR_OK)
    {
        if (error == ERR_COULDNT_CONNECT)
        {
            gUserSessionManager->sendPINErrorEvent(RiverPoster::ExternalSystemUnavailableErrToString(RiverPoster::NUCLEUS_SERVICE_UNAVAILABLE), RiverPoster::external_system_unavailable);
        }
        else if (error == AUTH_ERR_USECOUNT_ZERO)
        {
            gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(error), RiverPoster::entitlement_modify_failed);
        }
        return commandErrorFromBlazeError(error);
    }
    mResponse.setUseCountConsumed(consumedUseCount);
    mResponse.setUseCountRemain(remainedUseCount);

    return ERR_OK;
}

} // Authentication
} // Blaze
