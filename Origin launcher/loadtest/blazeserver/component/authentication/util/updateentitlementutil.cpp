/*************************************************************************************************/
/*!
    \file updateentitlementutil.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "framework/blaze.h"
#include "framework/connection/outboundhttpservice.h"

// Authentication
#include "authentication/authenticationimpl.h"

// Authentication utils
#include "framework/oauth/accesstokenutil.h"
#include "authentication/util/authenticationutil.h"
#include "authentication/util/updateentitlementutil.h"

#include "proxycomponent/nucleusidentity/rpc/nucleusidentityslave.h"
#include "proxycomponent/nucleusidentity/tdf/nucleusidentity.h"

namespace Blaze
{
namespace Authentication
{

UpdateEntitlementUtil::UpdateEntitlementUtil(AuthenticationSlaveImpl &componentImpl, const PeerInfo* peerInfo, const ClientType &clientType)
  : mComponent(componentImpl)
    , mPeerInfo(peerInfo)
{
}

BlazeRpcError UpdateEntitlementUtil::updateEntitlement(AccountId accountId, int64_t entitlementId, const char8_t* productName,
                                                       const char8_t* useCount, const char8_t* terminationDate, uint32_t version,
                                                       const Blaze::Nucleus::EntitlementStatus::Code status,
                                                       const Blaze::Nucleus::StatusReason::Code statusReason)
{
    if (status != Blaze::Nucleus::EntitlementStatus::UNKNOWN && 
        status != Blaze::Nucleus::EntitlementStatus::DISABLED &&
        status != Blaze::Nucleus::EntitlementStatus::DELETED && 
        status != Blaze::Nucleus::EntitlementStatus::BANNED)
    {
        return AUTH_ERR_MODIFIED_STATUS_INVALID;
    }

    BlazeRpcError rc = ERR_SYSTEM;
    OAuth::AccessTokenUtil tokenUtil;

    if (Blaze::OAuth::shouldUseCurrentUserToken(INVALID_BLAZE_ID, accountId))
        rc = tokenUtil.retrieveCurrentUserAccessToken(OAuth::TOKEN_TYPE_BEARER);
    else
        rc = tokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_BEARER);
    if (rc != ERR_OK)
    {
        ERR_LOG("[UpdateEntitlementUtil].updateEntitlement : Failed to retrieve access token for user " << gCurrentUserSession->getBlazeId());
        return rc;
    }

    NucleusIdentity::GetEntitlementsRequest req;
    NucleusIdentity::GetEntitlementResponse resp;
    NucleusIdentity::IdentityError error;

    req.setEntitlementId(entitlementId);
    req.getAuthCredentials().setAccessToken(tokenUtil.getAccessToken());
    req.getAuthCredentials().setClientId(tokenUtil.getClientId());
    
    const char8_t* ipAddress = mPeerInfo ? mPeerInfo->getRealPeerAddress().getIpAsString() : nullptr;
    req.getAuthCredentials().setIpAddress(ipAddress);

    NucleusIdentity::NucleusIdentitySlave* ident = (NucleusIdentity::NucleusIdentitySlave*)(gOutboundHttpService->getService(NucleusIdentity::NucleusIdentitySlave::COMPONENT_INFO.name));
    rc = ident->getEntitlement(req, resp, error);
    if (rc != ERR_OK)
    {
        if (rc != ERR_COULDNT_CONNECT)
            rc = AUTH_ERR_UNKNOWN_ENTITLEMENT;
        ERR_LOG("[UpdateEntitlementUtil].updateEntitlement: getEntitlement failed with error(" << ErrorHelp::getErrorName(rc) << ")");
        return rc;
    }

    NucleusIdentity::EntitlementInfo *info = &resp.getEntitlement(); // Only one entitlement should be returned

    if (!mComponent.passClientModifyWhitelist(productName, info->getGroupName()))
    {
        TRACE_LOG("[ModifyEntitlement2Command].execute check white list failed. Groupname(" << info->getGroupName() << ") not defined in ClientEntitlementModifyWhitelist.");
        gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(AUTH_ERR_WHITELIST), RiverPoster::entitlement_modify_failed);
        return AUTH_ERR_WHITELIST;
    }

    if (useCount && useCount[0] != '\0')
    {
        uint32_t iUseCount;
        blaze_str2int(useCount, &iUseCount);

        if ((iUseCount > info->getUseCount()) &&
            !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_INCREMENT_ENTITLEMENT_USECOUNT, true))
        {
            WARN_LOG("[UpdateEntitlementUtil].updateEntitlement: UserSession(" << UserSession::getCurrentUserSessionId()
                << ") attempted to update Entitlement, no permission!");
            gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(AUTH_ERR_USECOUNT_INCREMENT), RiverPoster::entitlement_modify_failed);
            return AUTH_ERR_USECOUNT_INCREMENT;
        }
        else
            info->setUseCount(iUseCount);
    }

    if (terminationDate && terminationDate[0] != '\0')
    {
        if (info->getTerminationDate()[0] != '\0' &&
            TimeValue::parseAccountTime(terminationDate) > TimeValue::parseAccountTime(info->getTerminationDate()))
        {
            gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(AUTH_ERR_TERMINATION_INVALID), RiverPoster::entitlement_modify_failed);
            return AUTH_ERR_TERMINATION_INVALID;
        }
        else
        {
            info->setTerminationDate(terminationDate);
        }
    }

    if (status != Blaze::Nucleus::EntitlementStatus::UNKNOWN)
        info->setStatus(Blaze::Nucleus::EntitlementStatus::CodeToString(status));

    if (statusReason != Blaze::Nucleus::StatusReason::UNKNOWN)
        info->setStatusReasonCode(Blaze::Nucleus::StatusReason::CodeToString(statusReason));

    Blaze::NucleusIdentity::UpdateEntitlementRequest updateReq;
    Blaze::NucleusIdentity::UpsertEntitlementResponse updateResp;
    updateReq.getAuthCredentials().setAccessToken(tokenUtil.getAccessToken());
    updateReq.getAuthCredentials().setClientId(tokenUtil.getClientId());
    updateReq.getAuthCredentials().setIpAddress(ipAddress);

    updateReq.setPid(accountId);

    // Required parameter values
    updateReq.setEntitlementId(info->getEntitlementId());
    updateReq.getEntitlementInfo().setEntitlementTag(info->getEntitlementTag());
    updateReq.getEntitlementInfo().setStatus(info->getStatus());
    updateReq.getEntitlementInfo().setVersion(info->getVersion());

    // Optional parameter values
    updateReq.getEntitlementInfo().setManagedLifecycle(info->getManagedLifecycle());
    updateReq.getEntitlementInfo().setOriginPermissions(info->getOriginPermissions());
    if (info->getEntitlementSource()[0] != '\0')
        updateReq.getEntitlementInfo().setEntitlementSource(info->getEntitlementSource());
    if ((info->getEntitlementType()[0] != '\0') && (strcmp(info->getEntitlementType(), "UNKNOWN") != 0))
        updateReq.getEntitlementInfo().setEntitlementType(info->getEntitlementType());
    if (info->getGrantDate()[0] != '\0')
        updateReq.getEntitlementInfo().setGrantDate(info->getGrantDate());
    if (info->getGroupName()[0] != '\0')
        updateReq.getEntitlementInfo().setGroupName(info->getGroupName());
    if ((info->getProductCatalog()[0] != '\0') && (strcmp(info->getProductCatalog(), "UNKNOWN") != 0))
        updateReq.getEntitlementInfo().setProductCatalog(info->getProductCatalog());
    if (info->getProductId()[0] != '\0')
        updateReq.getEntitlementInfo().setProductId(info->getProductId());
    if (info->getProjectId()[0] != '\0')
        updateReq.getEntitlementInfo().setProjectId(info->getProjectId());
    if ((info->getStatusReasonCode()[0] != '\0') && (strcmp(info->getStatusReasonCode(), "UNKNOWN") != 0))
        updateReq.getEntitlementInfo().setStatusReasonCode(info->getStatusReasonCode());
    if (info->getTerminationDate()[0] != '\0')
        updateReq.getEntitlementInfo().setTerminationDate(info->getTerminationDate());
    updateReq.getEntitlementInfo().setUseCount(info->getUseCount());

    rc = ident->updateEntitlement(updateReq, updateResp, error);
    if (rc != ERR_OK)
    {
        rc = AuthenticationUtil::parseIdentityError(error, rc);
        ERR_LOG("[UpdateEntitlementUtil].updateEntitlement: updateEntitlement failed with error(" << ErrorHelp::getErrorName(rc) << ")");
        return rc;
    }

    return rc;
}

} // Authentication
} // Blaze
