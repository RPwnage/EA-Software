/*************************************************************************************************/
/*!
    \file listentitlementsutil.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "framework/blaze.h"
#include "framework/connection/outboundhttpservice.h"

#include "framework/oauth/accesstokenutil.h"
#include "authentication/util/authenticationutil.h"
#include "authentication/util/listentitlementsutil.h"

#include "proxycomponent/nucleusidentity/rpc/nucleusidentityslave.h"

namespace Blaze
{
namespace Authentication
{

ListEntitlementsUtil::ListEntitlementsUtil()
{
    mNucleusIdentityComp = (NucleusIdentity::NucleusIdentitySlave*)gOutboundHttpService->getService(NucleusIdentity::NucleusIdentitySlave::COMPONENT_INFO.name);
}

BlazeRpcError ListEntitlementsUtil::listEntitlementsForPersona(PersonaId personaId, const GroupNameList& groupNameList, uint16_t pageSize, uint16_t pageNo,
                                                               Entitlements::EntitlementList& resultList, const Command* callingCommand/* = nullptr*/)
{
    BlazeRpcError rc = ERR_SYSTEM;

    if (mNucleusIdentityComp == nullptr)
    {
        WARN_LOG("[ListEntitlementsUtil].listEntitlementsForPersona: nucleusidentity is nullptr");
        return rc;
    }

    NucleusIdentity::GetEntitlementsRequest req;
    NucleusIdentity::GetEntitlementsResponse resp;
    NucleusIdentity::IdentityError error;

    req.getAuthCredentials().setIpAddress(AuthenticationUtil::getRealEndUserAddr(callingCommand).getIpAsString());

    rc = fetchAuthToken(req, personaId, INVALID_ACCOUNT_ID);
    if (rc != ERR_OK)
        return rc;

    req.setPersonaId(personaId);
    req.setExpandResults("true");
    if (pageSize > 0)
        req.setPageSize(pageSize);
    if (pageNo > 0)
        req.setPageNumber(pageNo);

    rc = AuthenticationSlaveImpl::isValidGroupNameList(groupNameList);
    if (rc != ERR_OK)
    {
        // Nucleus does not allow empty string as a group name filter, caller should pass NONE if they mean ""
        ERR_LOG("[ListEntitlementsUtil].listEntitlementsForPersona: Nucleus does not allow an empty string as a group name filter. "
            << "Caller should pass NONE to search for those without groupname. Failed with error[" << ErrorHelp::getErrorName(rc) << "]");
        return rc;
    }

    groupNameList.copyInto(req.getEntitlementSearchParams().getGroupNames());

    rc = mNucleusIdentityComp->getPersonaEntitlements(req, resp, error);
    if (rc != ERR_OK)
    {
        rc = AuthenticationUtil::parseIdentityError(error, rc);
        ERR_LOG("[ListEntitlementsUtil].listEntitlementsForPersona: listEntitlements for personaId(" << personaId << ") failed with error(" << ErrorHelp::getErrorName(rc) << ")");
        return rc;
    }

    NucleusIdentity::EntitlementInfoList& entitlements = resp.getEntitlements().getEntitlement();
    uint32_t entitlementCount = entitlements.size();
    TRACE_LOG("[ListEntitlementsUtil].listEntitlementsForPersona: listEntitlements for nucleus returned " << entitlementCount << " entitlements");

    for (uint32_t i = 0; i < entitlementCount; ++i)
    {
        Entitlement *entitlement = resultList.pull_back();
        AuthenticationUtil::parseEntitlementForResponse(personaId, *entitlements[i], *entitlement);
    }

    return rc;
}

BlazeRpcError ListEntitlementsUtil::listEntitlementsForUser(AccountId accountId, const GroupNameList& groupNameList, uint16_t pageSize, uint16_t pageNo,
                                                            Entitlements::EntitlementList& resultList, const Command* callingCommand/* = nullptr*/, bool hasAuthorizedPersona/* = true*/)
{
    BlazeRpcError rc = ERR_SYSTEM;

    if (mNucleusIdentityComp == nullptr)
    {
        WARN_LOG("[ListEntitlementsUtil].listEntitlementsForUser: nucleusidentity is nullptr");
        return rc;
    }

    NucleusIdentity::GetEntitlementsRequest req;
    NucleusIdentity::GetEntitlementsResponse resp;
    NucleusIdentity::IdentityError error;

    req.getAuthCredentials().setIpAddress(AuthenticationUtil::getRealEndUserAddr(callingCommand).getIpAsString());

    rc = fetchAuthToken(req, INVALID_BLAZE_ID, accountId);
    if (rc != ERR_OK)
        return rc;

    req.setPid(accountId);
    req.setExpandResults("true");
    req.getEntitlementSearchParams().setHasAuthorizedPersona(hasAuthorizedPersona ? "true" : "false");
    if (pageSize > 0)
        req.setPageSize(pageSize);
    if (pageNo > 0)
        req.setPageNumber(pageNo);

    rc = AuthenticationSlaveImpl::isValidGroupNameList(groupNameList);
    if (rc != ERR_OK)
    {
        // Nucleus does not allow empty string as a group name filter, caller should pass NONE if they mean ""
        ERR_LOG("[ListEntitlementsUtil].listEntitlementsForUser: Nucleus does not allow an empty string as a group name filter. "
            << "Caller should pass NONE to search for those without groupname. Failed with error[" << ErrorHelp::getErrorName(rc) << "]");
        return rc;
    }

    groupNameList.copyInto(req.getEntitlementSearchParams().getGroupNames());

    rc = mNucleusIdentityComp->getAccountEntitlements(req, resp, error);
    if (rc != ERR_OK)
    {
        rc = AuthenticationUtil::parseIdentityError(error, rc);
        ERR_LOG("[ListEntitlementsUtil].listEntitlementsForUser: listEntitlements for accountId(" << accountId << ") failed with error(" << ErrorHelp::getErrorName(rc) << ")");
        return rc;
    }

    NucleusIdentity::EntitlementInfoList& entitlements = resp.getEntitlements().getEntitlement();
    uint32_t entitlementCount = entitlements.size();
    TRACE_LOG("[ListEntitlementsUtil].listEntitlementsForUser: listEntitlements for nucleus returned " << entitlementCount << " entitlements");

    for (uint32_t i = 0; i < entitlementCount; ++i)
    {
        Entitlement *entitlement = resultList.pull_back();
        AuthenticationUtil::parseEntitlementForResponse(0, *entitlements[i], *entitlement);
    }

    return rc;
}

BlazeRpcError ListEntitlementsUtil::listEntitlements2ForPersona(PersonaId personaId, ListPersonaEntitlements2Request& request, Entitlements::EntitlementList& resultList,
                                                                const Command* callingCommand/* = nullptr*/)
{
    BlazeRpcError rc = ERR_SYSTEM;

    if (mNucleusIdentityComp == nullptr)
    {
        WARN_LOG("[ListEntitlementsUtil].listEntitlements2ForPersona: nucleusidentity is nullptr");
        return rc;
    }

    NucleusIdentity::GetEntitlementsRequest req;
    NucleusIdentity::GetEntitlementsResponse resp;
    NucleusIdentity::IdentityError error;

    req.getAuthCredentials().setIpAddress(AuthenticationUtil::getRealEndUserAddr(callingCommand).getIpAsString());

    rc = fetchAuthToken(req, personaId, INVALID_ACCOUNT_ID);
    if (rc != ERR_OK)
        return rc;

    req.setPersonaId(personaId);
    if (request.getPageSize() > 0)
        req.setPageSize(request.getPageSize());
    if (request.getPageNo() > 0)
        req.setPageNumber(request.getPageNo());
    req.setRecursiveSearch(request.getRecursiveSearch() ? "true" : "false");
    req.setExpandResults("true");

    NucleusIdentity::EntitlementSearchParams& params = req.getEntitlementSearchParams();
    params.setProjectId(request.getProjectId());
    params.setEntitlementTag(request.getEntitlementTag());
    params.setProductId(request.getProductId());
    params.setStartGrantDate(request.getStartGrantDate());
    params.setEndGrantDate(request.getEndGrantDate());
    params.setStartTerminationDate(request.getStartTerminationDate());
    params.setEndTerminationDate(request.getEndTerminationDate());
    if (request.getEntitlementType() != Blaze::Nucleus::EntitlementType::UNKNOWN)
        params.setEntitlementType(Blaze::Nucleus::EntitlementType::CodeToString(request.getEntitlementType()));
    if (request.getStatus() != Blaze::Nucleus::EntitlementStatus::UNKNOWN)
        params.setStatus(Blaze::Nucleus::EntitlementStatus::CodeToString(request.getStatus()));
    params.setHasAuthorizedPersona("false");

    rc = AuthenticationSlaveImpl::isValidGroupNameList(request.getGroupNameList());
    if (rc != ERR_OK)
    {
        // Nucleus does not allow empty string as a group name filter, caller should pass NONE if they mean ""
        ERR_LOG("[ListEntitlementsUtil].listEntitlements2ForPersona: Nucleus does not allow an empty string as a group name filter. "
            << "Caller should pass NONE to search for those without groupname. Failed with error[" << ErrorHelp::getErrorName(rc) << "]");
        if (rc == AUTH_ERR_GROUPNAME_REQUIRED)
            gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(rc), RiverPoster::entitlement_access_failed);
        return rc;
    }

    request.getGroupNameList().copyInto(params.getGroupNames());

    rc = mNucleusIdentityComp->getPersonaEntitlements(req, resp, error);
    if (rc != ERR_OK)
    {
        rc = AuthenticationUtil::parseIdentityError(error, rc);
        ERR_LOG("[ListEntitlementsUtil].listEntitlements2ForPersona: listEntitlements for personaId(" << personaId << ") failed with error(" << ErrorHelp::getErrorName(rc) << ")");
        return rc;
    }

    uint32_t entitlementCount = resp.getEntitlements().getEntitlement().size();
    TRACE_LOG("[ListEntitlementsUtil].listEntitlements2ForPersona: listEntitlements for nucleus returned " << entitlementCount << " entitlements");

    for (uint32_t i = 0; i < entitlementCount; ++i)
    {
        Entitlement *entitlement = resultList.pull_back();
        AuthenticationUtil::parseEntitlementForResponse(personaId, *resp.getEntitlements().getEntitlement()[i], *entitlement);
    }

    return rc;
}

BlazeRpcError ListEntitlementsUtil::listEntitlements2ForUser(AccountId accountId, ListUserEntitlements2Request& request, Entitlements::EntitlementList& resultList,
                                                             const Command* callingCommand/* = nullptr*/)
{
    BlazeRpcError rc = ERR_SYSTEM;

    if (mNucleusIdentityComp == nullptr)
    {
        WARN_LOG("[ListEntitlementsUtil].listEntitlements2ForUser: nucleusidentity is nullptr");
        return rc;
    }

    NucleusIdentity::GetEntitlementsRequest req;
    NucleusIdentity::GetEntitlementsResponse resp;
    NucleusIdentity::IdentityError error;

    req.getAuthCredentials().setIpAddress(AuthenticationUtil::getRealEndUserAddr(callingCommand).getIpAsString());

    rc = fetchAuthToken(req, INVALID_BLAZE_ID, accountId);
    if (rc != ERR_OK)
        return rc;

    req.setPid(accountId);
    if (request.getPageSize() > 0)
        req.setPageSize(request.getPageSize());
    if (request.getPageNo() > 0)
        req.setPageNumber(request.getPageNo());
    req.setRecursiveSearch(request.getRecursiveSearch() ? "true" : "false");
    req.setExpandResults("true");

    NucleusIdentity::EntitlementSearchParams& params = req.getEntitlementSearchParams();
    params.setProjectId(request.getProjectId());
    params.setEntitlementTag(request.getEntitlementTag());
    params.setProductId(request.getProductId());
    params.setStartGrantDate(request.getStartGrantDate());
    params.setEndGrantDate(request.getEndGrantDate());
    params.setStartTerminationDate(request.getStartTerminationDate());
    params.setEndTerminationDate(request.getEndTerminationDate());
    if (request.getEntitlementType() != Blaze::Nucleus::EntitlementType::UNKNOWN)
        params.setEntitlementType(Blaze::Nucleus::EntitlementType::CodeToString(request.getEntitlementType()));
    if (request.getStatus() != Blaze::Nucleus::EntitlementStatus::UNKNOWN)
        params.setStatus(Blaze::Nucleus::EntitlementStatus::CodeToString(request.getStatus()));
    params.setHasAuthorizedPersona(request.getHasAuthorizedPersona() ? "true" : "false");

    rc = AuthenticationSlaveImpl::isValidGroupNameList(request.getGroupNameList());
    if (rc != ERR_OK)
    {
        // Nucleus does not allow empty string as a group name filter, caller should pass NONE if they mean ""
        ERR_LOG("[ListEntitlementsUtil].listEntitlements2ForUser: Nucleus does not allow an empty string as a group name filter. "
            << "Caller should pass NONE to search for those without groupname Failed with error[" << ErrorHelp::getErrorName(rc) << "]");
        return rc;
    }

    request.getGroupNameList().copyInto(params.getGroupNames());

    rc = mNucleusIdentityComp->getAccountEntitlements(req, resp, error);
    if (rc != ERR_OK)
    {
        rc = AuthenticationUtil::parseIdentityError(error, rc);
        ERR_LOG("[ListEntitlementsUtil].listEntitlements2ForUser: listEntitlements for accountId(" << accountId << ") failed with error(" << ErrorHelp::getErrorName(rc) << ")");
        return rc;
    }

    uint32_t entitlementCount = resp.getEntitlements().getEntitlement().size();
    TRACE_LOG("[ListEntitlementsUtil].listEntitlements2ForUser: listEntitlements for nucleus returned " << entitlementCount << " entitlements");

    for (uint32_t i = 0; i < entitlementCount; ++i)
    {
        Entitlement *entitlement = resultList.pull_back();
        AuthenticationUtil::parseEntitlementForResponse(0, *resp.getEntitlements().getEntitlement()[i], *entitlement);
    }

    return rc;
}

BlazeRpcError ListEntitlementsUtil::fetchAuthToken(NucleusIdentity::GetEntitlementsRequest& req, PersonaId personaId, AccountId accountId) const
{
    BlazeRpcError rc = ERR_SYSTEM;
    OAuth::AccessTokenUtil tokenUtil;
    
    if (Blaze::OAuth::shouldUseCurrentUserToken(personaId, accountId)) 
    {
        rc = tokenUtil.retrieveCurrentUserAccessToken(OAuth::TOKEN_TYPE_BEARER);
    }
    else
    {
        rc = tokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_BEARER);
    }
    if (rc != ERR_OK)
    {
        ERR_LOG("[ListEntitlementsUtil].fetchAuthToken: Failed to retrieve access token.");
        return rc;
    }

    req.getAuthCredentials().setAccessToken(tokenUtil.getAccessToken());
    req.getAuthCredentials().setClientId(tokenUtil.getClientId());

    return rc;
}

} // Authentication
} // Blaze
