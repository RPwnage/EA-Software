/*************************************************************************************************/
/*!
    \file   modifyentitlement2_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ModifyEntitlement2Command

    Sends Nucleus an email and password for authentication.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "authentication/authenticationimpl.h"
#include "authentication/tdf/authentication.h"
#include "authentication/rpc/authenticationslave/modifyentitlement2_stub.h"
#include "authentication/util/updateentitlementutil.h"

#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"
#include "framework/controller/controller.h"

namespace Blaze
{
namespace Authentication
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

class ModifyEntitlement2Command : public ModifyEntitlement2CommandStub
{
public:
    ModifyEntitlement2Command(Message* message, Blaze::Authentication::ModifyEntitlement2Request* request, AuthenticationSlaveImpl* componentImpl);
    ~ModifyEntitlement2Command() override {};

private:
    // Not owned memory.
    AuthenticationSlaveImpl* mComponent;

    // Owned memory
    UpdateEntitlementUtil mModifyEntitlementUtil;

    // States
    ModifyEntitlement2CommandStub::Errors execute() override;
};

DEFINE_MODIFYENTITLEMENT2_CREATE();

ModifyEntitlement2Command::ModifyEntitlement2Command(Message* message, Blaze::Authentication::ModifyEntitlement2Request* request, AuthenticationSlaveImpl* componentImpl)
    : ModifyEntitlement2CommandStub(message, request),
    mComponent(componentImpl),
    mModifyEntitlementUtil(*componentImpl, getPeerInfo(), gCurrentUserSession->getClientType())
{
}

/* Private methods *******************************************************************************/
ModifyEntitlement2CommandStub::Errors ModifyEntitlement2Command::execute()
{
    // default account id is current user
    AccountId accountId = gCurrentUserSession->getAccountId();

    BlazeRpcError error = Blaze::ERR_OK;
    bool hasPermission = false;
    if (mRequest.getBlazeId() != INVALID_BLAZE_ID)
    {
        UserInfoPtr userInfo;
        error = gUserSessionManager->lookupUserInfoByBlazeId(mRequest.getBlazeId(), userInfo);
        if (error != Blaze::ERR_OK)
        {
            TRACE_LOG("[ModifyEntitlement2Command].execute has invalid user in the request.");
            gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(AUTH_ERR_INVALID_USER), RiverPoster::entitlement_modify_failed);
            return AUTH_ERR_INVALID_USER;
        }

        if (accountId != userInfo->getPlatformInfo().getEaIds().getNucleusAccountId())
        {
            accountId = userInfo->getPlatformInfo().getEaIds().getNucleusAccountId();

            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLIENT_WRITE_OTHER_USER_ENTITLEMENTS))
            {
                WARN_LOG("[ModifyEntitlement2Command].execute: User [" << gCurrentUserSession->getBlazeId() << "] attemped to modify entitlement for another user [" 
                    << mRequest.getBlazeId() << "], no permission!");
                error = Blaze::ERR_AUTHORIZATION_REQUIRED;
            }
            else
                hasPermission = true;
        }
    }

    if (error == Blaze::ERR_OK)
    {
        UserSession::SuperUserPermissionAutoPtr ptr(hasPermission);
        error = mModifyEntitlementUtil.updateEntitlement(accountId,
            mRequest.getEntitlementId(),
            gCurrentUserSession->getProductName(),
            mRequest.getUseCount(),
            mRequest.getTermination(), mRequest.getVersion(),
            mRequest.getStatus(), mRequest.getStatusReasonCode());

        if (error == ERR_COULDNT_CONNECT)
        {
            gUserSessionManager->sendPINErrorEvent(RiverPoster::ExternalSystemUnavailableErrToString(RiverPoster::NUCLEUS_SERVICE_UNAVAILABLE), RiverPoster::external_system_unavailable);
        }    
    }

    return commandErrorFromBlazeError(error);
}

} // Authentication
} // Blaze
