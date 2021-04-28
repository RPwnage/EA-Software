/*************************************************************************************************/
/*!
    \file   getusecount_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "authentication/authenticationimpl.h"
#include "authentication/tdf/authentication.h"
#include "authentication/rpc/authenticationslave/getusecount_stub.h"
#include "authentication/util/hasentitlementutil.h"

#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"
#include "framework/controller/controller.h"

namespace Blaze
{
namespace Authentication
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/
class GetUseCountCommand : public GetUseCountCommandStub
{
public:
    GetUseCountCommand(Message* message, GetUseCountRequest* request, AuthenticationSlaveImpl* componentImpl);

    ~GetUseCountCommand() override {};

private:
    // Not owned memory.
    AuthenticationSlaveImpl* mComponent;

    // Owned memory
    HasEntitlementUtil mGetEntitlementUtil;

    // States
    GetUseCountCommandStub::Errors execute() override;

};
DEFINE_GETUSECOUNT_CREATE()

GetUseCountCommand::GetUseCountCommand(Message* message, GetUseCountRequest* request, AuthenticationSlaveImpl* componentImpl)
    : GetUseCountCommandStub(message, request),
    mComponent(componentImpl),
    mGetEntitlementUtil(*componentImpl, gCurrentUserSession->getClientType())
{
}

/* Private methods *******************************************************************************/
GetUseCountCommandStub::Errors GetUseCountCommand::execute()
{
    uint32_t useCount = 0;

    // default account id is current user
    AccountId accountId = gCurrentUserSession->getAccountId();

    if (mRequest.getBlazeId() != INVALID_BLAZE_ID)
    {
        UserInfoPtr userInfo;
        BlazeRpcError lookupErr = gUserSessionManager->lookupUserInfoByBlazeId(mRequest.getBlazeId(), userInfo);
        if (lookupErr != Blaze::ERR_OK)
        {
            TRACE_LOG("[GetUseCountCommandStub].execute() - no such user, blaze id: " << mRequest.getBlazeId() << " found.");
            return AUTH_ERR_INVALID_USER;
        }

        if (userInfo->getPlatformInfo().getEaIds().getNucleusAccountId() != accountId &&
            !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_LIST_OTHER_USER_ENTITLEMENTS))
        {
            WARN_LOG("[GetUseCountCommandStub].execute:  Account Id (" << accountId << ") attempting to look up account id (" << userInfo->getPlatformInfo().getEaIds().getNucleusAccountId() << ")'s entitlements without proper permissions.");
            BlazeRpcError error = Blaze::ERR_AUTHORIZATION_REQUIRED;

            gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(error), RiverPoster::entitlement_access_failed);
            return commandErrorFromBlazeError(error);;
        }

        accountId = userInfo->getPlatformInfo().getEaIds().getNucleusAccountId();
    }

    // entitlement tag is required
    if (mRequest.getEntitlementTag()[0] == '\0')
    {
        TRACE_LOG("[GetUseCountCommandStub].execute() - empty entitlement tag.");
        gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(AUTH_ERR_ENTITLEMETNTAG_EMPTY), RiverPoster::entitlement_usage_count_fetch_failed);
        return AUTH_ERR_ENTITLEMETNTAG_EMPTY;
    }

    BlazeRpcError error = mGetEntitlementUtil.getUseCount(accountId, mRequest.getProductId(), mRequest.getEntitlementTag(),
        mRequest.getGroupName(), mRequest.getProjectId(), useCount);

    if (error == Blaze::ERR_OK)
        mResponse.setUseCount(useCount);
    else if (error == ERR_COULDNT_CONNECT)
        gUserSessionManager->sendPINErrorEvent(RiverPoster::ExternalSystemUnavailableErrToString(RiverPoster::NUCLEUS_SERVICE_UNAVAILABLE), RiverPoster::external_system_unavailable);

    return commandErrorFromBlazeError(error);
}

} // Authentication
} // Blaze
