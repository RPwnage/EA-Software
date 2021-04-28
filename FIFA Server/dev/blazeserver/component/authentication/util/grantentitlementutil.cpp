/*************************************************************************************************/
/*!
    \file grantentitlementutil.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/outboundhttpservice.h"

#include "framework/oauth/accesstokenutil.h"
#include "authentication/util/authenticationutil.h"
#include "authentication/util/grantentitlementutil.h"

#include "proxycomponent/nucleusidentity/rpc/nucleusidentityslave.h"
#include "proxycomponent/nucleusidentity/tdf/nucleusidentity.h"

#include "EAStdC/EAString.h"

namespace Blaze
{
namespace Authentication
{

GrantEntitlementUtil::GrantEntitlementUtil(const PeerInfo* peerInfo,
                                           const AuthenticationConfig::NucleusBypassURIList* bypassList, NucleusBypassType bypassType)
  : mPeerInfo(peerInfo)
{
}

// Note:
// Before calling grantEntitlement, callers need to verify ClientGrantWhitelist based on serverName and groupName on their own.
BlazeRpcError GrantEntitlementUtil::grantEntitlementLegacy(AccountId accountId, PersonaId personaId,
                                                     const char8_t* productId, const char8_t* entitlementTag, 
                                                     const char8_t* groupName, const char8_t* projectId,
                                                     const char8_t* entitlementSource, const char8_t* productName,
                                                     Blaze::Nucleus::EntitlementType::Code entitlementType /*=Blaze::Nucleus::EntitlementType::DEFAULT*/,
                                                     Blaze::Nucleus::EntitlementStatus::Code entitlementStatus /*= Blaze::Nucleus::EntitlementStatus::ACTIVE*/,
                                                     bool withPersona /*= true*/)
{
    GrantEntitlement2Request request;
    GrantEntitlement2Response response;
    request.setProductId(productId);
    request.setEntitlementTag(entitlementTag);
    request.setStatus(entitlementStatus);
    request.setGroupName(groupName);
    request.setProjectId(projectId);
    request.setEntitlementType(entitlementType);
    request.setIsSearch(false);
    request.setWithPersona(withPersona);

    return grantEntitlement2(accountId, personaId, request, response, entitlementSource, productName);
}

BlazeRpcError GrantEntitlementUtil::grantEntitlement2(AccountId accountId, PersonaId personaId,
                                                      GrantEntitlement2Request& request, GrantEntitlement2Response &response, 
                                                      const char8_t* entitlementSource, const char8_t* productName)
{
    BlazeRpcError rc = ERR_SYSTEM;
    OAuth::AccessTokenUtil tokenUtil;
    {
        //retrieve full scope server access token in a block to avoid changing function context permission
        UserSession::SuperUserPermissionAutoPtr autoPtr(true);
        rc = tokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_BEARER);
        if (rc != Blaze::ERR_OK)
        {
            ERR_LOG("[GrantEntitlementUtil].grantEntitlement2: Failed to retrieve access token");
            return rc;
        }
    }

    NucleusIdentity::PostEntitlementRequest req;
    NucleusIdentity::UpsertEntitlementResponse resp;
    NucleusIdentity::IdentityError error;

    req.setReleaseType(gUserSessionManager->getProductReleaseType(productName));
    req.getAuthCredentials().setAccessToken(tokenUtil.getAccessToken());
    req.getAuthCredentials().setClientId(tokenUtil.getClientId());

    const char8_t* ipAddress = mPeerInfo ? mPeerInfo->getRealPeerAddress().getIpAsString() : nullptr;
    req.getAuthCredentials().setIpAddress(ipAddress);

    req.setPid(accountId);

    // Required parameter values
    req.getEntitlementInfo().setEntitlementTag(request.getEntitlementTag());
    req.getEntitlementInfo().setStatus(Blaze::Nucleus::EntitlementStatus::CodeToString(request.getStatus()));

    // Optional parameter values
    if (atol(request.getDeviceId()) != 0)
        req.getEntitlementInfo().setDeviceId(atol(request.getDeviceId()));
    if (entitlementSource != nullptr && entitlementSource[0] != '\0')
        req.getEntitlementInfo().setEntitlementSource(entitlementSource);
    if (request.getEntitlementType() != Blaze::Nucleus::EntitlementType::UNKNOWN)
        req.getEntitlementInfo().setEntitlementType(Blaze::Nucleus::EntitlementType::CodeToString(request.getEntitlementType()));
    if (request.getGrantDate()[0] != '\0')
        req.getEntitlementInfo().setGrantDate(request.getGrantDate());
    if (request.getGroupName()[0] != '\0')
        req.getEntitlementInfo().setGroupName(request.getGroupName());
    if (request.getProductCatalog() != Blaze::Nucleus::ProductCatalog::UNKNOWN)
        req.getEntitlementInfo().setProductCatalog(Blaze::Nucleus::ProductCatalog::CodeToString(request.getProductCatalog()));
    if (request.getProductId()[0] != '\0')
        req.getEntitlementInfo().setProductId(request.getProductId());
    if (request.getProjectId()[0] != '\0')
        req.getEntitlementInfo().setProjectId(request.getProjectId());
    if (request.getStatusReasonCode() != Blaze::Nucleus::StatusReason::UNKNOWN)
        req.getEntitlementInfo().setStatusReasonCode(Blaze::Nucleus::StatusReason::CodeToString(request.getStatusReasonCode()));
    if (request.getTermination()[0] != '\0')
        req.getEntitlementInfo().setTerminationDate(request.getTermination());
    if (EA::StdC::AtoU64(request.getUseCount()) != 0)
        req.getEntitlementInfo().setUseCount(EA::StdC::AtoU64(request.getUseCount()));

    NucleusIdentity::NucleusIdentitySlave* ident = (NucleusIdentity::NucleusIdentitySlave*)(gOutboundHttpService->getService(NucleusIdentity::NucleusIdentitySlave::COMPONENT_INFO.name));
    rc = ident->searchOrPostEntitlement(req, resp, error);
    if (rc == NUCLEUS_IDENTITY_ENTITLEMENT_GRANTED)
    {
        response.setIsGranted(true);
    }
    else if (rc != ERR_OK)
    {
        rc = AuthenticationUtil::parseIdentityError(error, rc);
        ERR_LOG("[GrantEntitlementUtil].grantEntitlement2: searchOrPostEntitlement failed with error(" << ErrorHelp::getErrorName(rc) << ")");
        return rc;
    }

    int64_t entitlementId = 0;
    const char8_t *entitlements = blaze_strstr(resp.getEntitlementUri(), "/entitlements/");
    if (entitlements != nullptr)
    {
        entitlements += sizeof("/entitlements/") - 1;
        blaze_str2int(entitlements, &entitlementId);
    }
    else
    {
        return ERR_SYSTEM;
    }

    if (request.getWithPersona())
    {
        // Attempt to post the persona link. This may fail with LINK_ALREADY_EXISTS, in which case, just grab the entitlement info
        NucleusIdentity::PostEntitlementPersonaLinkRequest linkReq;
        NucleusIdentity::PostEntitlementPersonaLinkResponse linkResp;
        linkReq.setEntitlementId(entitlementId);
        linkReq.setPersonaId(personaId);
        linkReq.getAuthCredentials().setAccessToken(tokenUtil.getAccessToken());
        linkReq.getAuthCredentials().setClientId(tokenUtil.getClientId());
        linkReq.getAuthCredentials().setIpAddress(ipAddress);

        rc = ident->postEntitlementPersonaLink(linkReq, linkResp, error);
        if (rc != ERR_OK)
        {
            if (rc == ERR_COULDNT_CONNECT)
            {
                return rc;
            }
            else if (!error.getError().getFailure().empty())
            {
                NucleusIdentity::FailureDetails *failureDetails = error.getError().getFailure().at(0);
                Blaze::Nucleus::NucleusCause::Code cause = Blaze::Nucleus::NucleusCause::UNKNOWN;
                Blaze::Nucleus::NucleusCause::ParseCode(failureDetails->getCause(), cause);
                if (cause != Blaze::Nucleus::NucleusCause::LINK_ALREADY_EXISTS)
                {
                    return AUTH_ERR_LINK_PERSONA;
                }
            }
            else
            {
                return AUTH_ERR_LINK_PERSONA;
            }
        }
    }

    NucleusIdentity::GetEntitlementsRequest getReq;
    NucleusIdentity::GetEntitlementResponse getResp;
    getReq.setEntitlementId(entitlementId);
    getReq.getAuthCredentials().setAccessToken(tokenUtil.getAccessToken());
    getReq.getAuthCredentials().setClientId(tokenUtil.getClientId());
    getReq.getAuthCredentials().setIpAddress(ipAddress);

    rc = ident->getEntitlement(getReq, getResp, error);
    if (rc != ERR_OK)
    {
        rc = AuthenticationUtil::parseIdentityError(error, rc);
        ERR_LOG("[GrantEntitlementUtil].grantEntitlement2: getEntitlement failed with error(" << ErrorHelp::getErrorName(rc) << ")");
        return rc;
    }

    AuthenticationUtil::parseEntitlementForResponse(personaId, getResp.getEntitlement(), response.getEntitlementInfo());

    return rc;
}

} // Authentication
} // Blaze
