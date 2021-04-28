/*************************************************************************************************/
/*!
\file   DecrementEntitlementUseCountUtil.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "framework/blaze.h"
#include "framework/connection/outboundhttpservice.h"

#include "authentication/rpc/authentication_defines.h"

#include "authentication/util/authenticationutil.h"
#include "framework/oauth/accesstokenutil.h"
#include "authentication/util/decremententitlementusecountutil.h"

#include "proxycomponent/nucleusidentity/rpc/nucleusidentityslave.h"
#include "proxycomponent/nucleusidentity/tdf/nucleusidentity.h"

#include "EAStdC/EAString.h"

namespace Blaze
{
namespace Authentication
{

BlazeRpcError DecrementEntitlementUseCountUtil::decrementEntitlementUseCount(const AccountId accountId, const DecrementUseCountRequest& request, uint32_t& consumedCount, uint32_t& remainedCount)
{
    TRACE_LOG("[DecrementEntitlementUseCountUtil].decrementEntitlementUseCount - Fetch entitlement");

    uint32_t totalUseCount = 0;
    uint32_t useCountConsuming = 0;

    // let's get use count for the required entitlements
    BlazeRpcError rc = mGetEntitlementUtil.getUseCount(accountId, request.getProductId(), request.getEntitlementTag(),
        request.getGroupName(), request.getProjectId(), totalUseCount);
    if (rc != Blaze::ERR_OK)
        return rc;

    // if no use count for the found entitlements, rc out
    if (totalUseCount == 0)
    {
        TRACE_LOG("[DecrementEntitlementUseCountUtil].decrementEntitlementUseCount - the entitlement to be decreased has 0 use count.");
        return AUTH_ERR_USECOUNT_ZERO;
    }

    uint32_t useCountRemained = totalUseCount;
    uint32_t useCountPreConsumed = useCountRemained;

    uint32_t useCountTobeConsumed = request.getDecrementCount();

    // update use count 
    Entitlements::EntitlementList::const_iterator it = mGetEntitlementUtil.getEntitlementsList().begin();
    Entitlements::EntitlementList::const_iterator itend = mGetEntitlementUtil.getEntitlementsList().end();
    for (; it != itend; ++it)
    {
        if ((useCountTobeConsumed == 0) ||
            ((request.getDecrementCount() - useCountTobeConsumed) >= totalUseCount))
        {
            // decrement is done
            break; 
        }

        const Entitlement* info = *it;

        // how many use counts need to get decrement 
        useCountPreConsumed = useCountConsuming;
        if (useCountTobeConsumed > info->getUseCount())
        {
            useCountConsuming += info->getUseCount();
            useCountRemained = 0;
        }
        else 
        {
            useCountConsuming += useCountTobeConsumed;
            useCountRemained = info->getUseCount() - useCountTobeConsumed;
        }

        OAuth::AccessTokenUtil serverTokenUtil;
        {
            //retrieve full scope server access token in a block to avoid changing function context permission
            UserSession::SuperUserPermissionAutoPtr autoPtr(true);
            rc = serverTokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_BEARER);
            if (rc != Blaze::ERR_OK)
            {
                ERR_LOG("[DecrementEntitlementUseCountUtil].decrementEntitlementUseCount: Failed to retrieve server access token with error(" << ErrorHelp::getErrorName(rc) << ")");
                return rc;
            }
        }

        OAuth::AccessTokenUtil userTokenUtil;
        rc = userTokenUtil.retrieveCurrentUserAccessToken(OAuth::TOKEN_TYPE_NONE);
        if (rc != Blaze::ERR_OK)
        {
            ERR_LOG("[DecrementEntitlementUseCountUtil].decrementEntitlementUseCount: Failed to retrieve user access token with error(" << ErrorHelp::getErrorName(rc) << ") for user " << gCurrentUserSession->getBlazeId());
            return rc;
        }

        // check if entitlement exists
        NucleusIdentity::UpdateEntitlementRequest req;
        NucleusIdentity::UpsertEntitlementResponse resp;
        NucleusIdentity::IdentityError error;
        req.getAuthCredentials().setAccessToken(serverTokenUtil.getAccessToken());
        req.getAuthCredentials().setUserAccessToken(userTokenUtil.getAccessToken());
        req.getAuthCredentials().setClientId(userTokenUtil.getClientId());

        req.getAuthCredentials().setIpAddress(mPeerInfo->getRealPeerAddress().getIpAsString());

        req.setPid(accountId);
        req.setEntitlementId(info->getId());

        // Required parameter values
        req.getEntitlementInfo().setUseCount(useCountRemained);
        req.getEntitlementInfo().setVersion(info->getVersion());

        // Optional parameter values
        if (EA::StdC::AtoU64(info->getDeviceUri()) != 0)
            req.getEntitlementInfo().setDeviceId(EA::StdC::AtoU64(info->getDeviceUri()));
        if (info->getEntitlementTag()[0] != '\0')
            req.getEntitlementInfo().setEntitlementTag(info->getEntitlementTag());
        if (info->getEntitlementType() != Blaze::Nucleus::EntitlementType::UNKNOWN)
            req.getEntitlementInfo().setEntitlementType(Blaze::Nucleus::EntitlementType::CodeToString(info->getEntitlementType()));
        if (info->getGrantDate()[0] != '\0')
            req.getEntitlementInfo().setGrantDate(info->getGrantDate());
        if (info->getGroupName()[0] != '\0')
            req.getEntitlementInfo().setGroupName(info->getGroupName());
        if (info->getProductCatalog() != Blaze::Nucleus::ProductCatalog::UNKNOWN)
            req.getEntitlementInfo().setProductCatalog(Blaze::Nucleus::ProductCatalog::CodeToString(info->getProductCatalog()));
        if (info->getProductId()[0] != '\0')
            req.getEntitlementInfo().setProductId(info->getProductId());
        if (info->getProjectId()[0] != '\0')
            req.getEntitlementInfo().setProjectId(info->getProjectId());
        if (info->getStatus() != Blaze::Nucleus::EntitlementStatus::UNKNOWN)
            req.getEntitlementInfo().setStatus(Blaze::Nucleus::EntitlementStatus::CodeToString(info->getStatus()));
        if (info->getStatusReasonCode() != Blaze::Nucleus::StatusReason::UNKNOWN)
            req.getEntitlementInfo().setStatusReasonCode(Nucleus::StatusReason::CodeToString(info->getStatusReasonCode()));
        if (info->getTerminationDate()[0] != '\0')
            req.getEntitlementInfo().setTerminationDate(info->getTerminationDate());

        NucleusIdentity::NucleusIdentitySlave* ident = (NucleusIdentity::NucleusIdentitySlave*)(gOutboundHttpService->getService(NucleusIdentity::NucleusIdentitySlave::COMPONENT_INFO.name));

        rc = ident->updateEntitlement(req, resp, error);
        if (rc != Blaze::ERR_OK)
        {
            rc = AuthenticationUtil::parseIdentityError(error, rc);
            ERR_LOG("[DecrementEntitlementUseCountUtil].decrementEntitlementUseCount: updateEntitlement failed with error(" << ErrorHelp::getErrorName(rc) << ")");
            useCountConsuming = useCountPreConsumed;
            break;
        }

        useCountTobeConsumed = useCountTobeConsumed - useCountConsuming;
    }

    consumedCount = useCountConsuming;

    if (totalUseCount > useCountConsuming)
    {
        remainedCount = totalUseCount - useCountConsuming;
    }

    return Blaze::ERR_OK;
}

}
}
