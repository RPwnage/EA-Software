/*************************************************************************************************/
/*!
    \file hasentitlementutil.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "framework/blaze.h"
#include "framework/connection/outboundhttpservice.h"
#include "framework/protocol/restprotocolutil.h"
#include "framework/oauth/oauthslaveimpl.h"

#include "authentication/authenticationimpl.h"
#include "framework/oauth/accesstokenutil.h"
#include "authentication/util/authenticationutil.h"
#include "authentication/util/hasentitlementutil.h"

#include "proxycomponent/nucleusidentity/rpc/nucleusidentityslave.h"

namespace Blaze
{
namespace Authentication
{

BlazeRpcError HasEntitlementUtil::hasEntitlement(AccountId accountId, const char8_t* productId, const char8_t* entitlementTag, const GroupNameList& groupNameList, const char8_t* projectId, bool autoGrantEnabled,
                                                 Blaze::Nucleus::EntitlementType::Code entitlementType, Blaze::Nucleus::EntitlementStatus::Code entitlementStatus, Blaze::Nucleus::EntitlementSearchType::Type searchType, PersonaId personaId,
                                                 const Command* callingCommand/* = nullptr*/, const char8_t* accessToken/* = nullptr*/, const char8_t* clientId/* = nullptr*/)
{
    mEntitlementsList.clear();

    TRACE_LOG("[HasEntitlementUtil].hasEntitlement: checking entitlementType (" << Blaze::Nucleus::EntitlementType::CodeToString(entitlementType) << ") for accountId(" << accountId
        << ").");
    
    NucleusIdentity::GetEntitlementsResponse response;
    BlazeRpcError rpcError = getEntitlements(accountId, productId, entitlementTag, groupNameList, projectId, entitlementType, entitlementStatus,
        searchType, personaId, response, callingCommand, accessToken, clientId);
    if (rpcError != ERR_OK)
    {
        BLAZE_LOG((autoGrantEnabled ? Blaze::Logging::TRACE : Blaze::Logging::ERR), LOGGER_CATEGORY, "[HasEntitlementUtil].hasEntitlement: checking entitlementType (" << Blaze::Nucleus::EntitlementType::CodeToString(entitlementType) << ") for accountId(" << accountId
            << ") ran into error (" << Blaze::ErrorHelp::getErrorName(rpcError) << ").");
        return rpcError;
    }

    NucleusIdentity::EntitlementInfoList::const_iterator it = response.getEntitlements().getEntitlement().begin();
    NucleusIdentity::EntitlementInfoList::const_iterator itend = response.getEntitlements().getEntitlement().end();
    for (; it != itend; ++it)
    {
        Entitlement* info = mEntitlementsList.pull_back();
        AuthenticationUtil::parseEntitlementForResponse(personaId, **it, *info);
    }

    // check if entitlement exists
    if (response.getEntitlements().getEntitlement().empty())
    {
        BLAZE_LOG((autoGrantEnabled ? Blaze::Logging::TRACE : Blaze::Logging::ERR), LOGGER_CATEGORY, "[HasEntitlementUtil].hasEntitlement: checking entitlementType (" << Blaze::Nucleus::EntitlementType::CodeToString(entitlementType) << ") for accountId(" << accountId
            << ") resulted in empty entitlements. Response:" << response);
        return AUTH_ERR_NO_SUCH_ENTITLEMENT;
    }
    return ERR_OK;
}

BlazeRpcError HasEntitlementUtil::getUseCount(AccountId accountId, const char8_t* productId, const char8_t* entitlementTag, const char8_t* groupName, const char8_t* projectId,
                                              uint32_t& useCount, Blaze::Nucleus::EntitlementType::Code entitlementType, Blaze::Nucleus::EntitlementStatus::Code entitlementStatus)
{
    mEntitlementsList.clear();

    GroupNameList groupNameList;
    if (groupName && (groupName[0] != '\0'))
    {
        groupNameList.push_back(groupName);
    }

    // fetch entitlements matching the criteria that's on this account but not tied to any specific persona because we don't want to count
    // entitlements that match the criteria and is on this account but tied to personas other than the currently logged in persona
    NucleusIdentity::GetEntitlementsResponse response;
    BlazeRpcError rpcError = getEntitlements(accountId, productId, entitlementTag, groupNameList, projectId, entitlementType, entitlementStatus, Blaze::Nucleus::EntitlementSearchType::USER, 0, response);
    if (rpcError != ERR_OK)
        return rpcError;

    // reset initial use count
    useCount = 0;

    // check if entitlement exists and go through each of them to get use count
    bool noSuchEntitlement = response.getEntitlements().getEntitlement().empty();
    NucleusIdentity::EntitlementInfoList::const_iterator it = response.getEntitlements().getEntitlement().begin();
    NucleusIdentity::EntitlementInfoList::const_iterator itend = response.getEntitlements().getEntitlement().end();
    for (; it != itend; ++it)
    {
        Entitlement *info = mEntitlementsList.pull_back();
        AuthenticationUtil::parseEntitlementForResponse(0, **it, *info);
        useCount += info->getUseCount();
    }

    response.getEntitlements().getEntitlement().clear();

    // fetch entitlements matching the criteria that's on this account and tied to the logged in persona
    rpcError = getEntitlements(accountId, productId, entitlementTag, groupNameList, projectId, entitlementType, entitlementStatus, Blaze::Nucleus::EntitlementSearchType::PERSONA,
        gCurrentUserSession->getBlazeId(), response);
    if (rpcError != ERR_OK)
        return rpcError;

    noSuchEntitlement = noSuchEntitlement && response.getEntitlements().getEntitlement().empty();

    if (noSuchEntitlement)
        return AUTH_ERR_NO_SUCH_ENTITLEMENT;

    it = response.getEntitlements().getEntitlement().begin();
    itend = response.getEntitlements().getEntitlement().end();
    for (; it != itend; ++it)
    {
        Entitlement *info = mEntitlementsList.pull_back();
        AuthenticationUtil::parseEntitlementForResponse(gCurrentUserSession->getBlazeId(), **it, *info);
        useCount += info->getUseCount();
    }

    return ERR_OK;
}

BlazeRpcError HasEntitlementUtil::getEntitlements(AccountId accountId, const char8_t* productId, const char8_t* entitlementTag, const GroupNameList& groupNameList, const char8_t* projectId,
                                                 Blaze::Nucleus::EntitlementType::Code entitlementType, Blaze::Nucleus::EntitlementStatus::Code entitlementStatus, Blaze::Nucleus::EntitlementSearchType::Type searchType, PersonaId personaId,
                                                  NucleusIdentity::GetEntitlementsResponse &response, const Command* callingCommand/* = nullptr*/, const char8_t* accessToken/* = nullptr*/,
                                                  const char8_t* clientId/* = nullptr*/)
{
    
    TRACE_LOG("[HasEntitlementUtil].getEntitlement: Calling Nucleus to get entitlements for personaId(" << personaId << "), accountId(" << accountId << ")");

    BlazeRpcError rc = ERR_SYSTEM;
    OAuth::AccessTokenUtil tokenUtil;
    mIsSuccess = false;

    if(accessToken == nullptr || accessToken[0] == '\0' || clientId == nullptr || clientId[0] == '\0')
    {
        if (Blaze::OAuth::shouldUseCurrentUserToken(personaId, accountId))
            rc = tokenUtil.retrieveCurrentUserAccessToken(OAuth::TOKEN_TYPE_BEARER);
        else
            rc = tokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_BEARER);

        if (rc != Blaze::ERR_OK)
        {
            ERR_LOG("[HasEntitlementUtil].getEntitlement: Failed to retrieve access token for user with personaId(" << personaId << "), accountId(" << accountId
                << ") due to Error(" << ErrorHelp::getErrorName(rc) << ")for product(" << productId << ")");
            return rc;
        }

        accessToken = tokenUtil.getAccessToken();
        clientId = tokenUtil.getClientId();
    }

    NucleusIdentity::GetEntitlementsRequest req;
    NucleusIdentity::IdentityError error;

    req.getAuthCredentials().setAccessToken(accessToken);
    req.getAuthCredentials().setClientId(clientId);
    req.getAuthCredentials().setIpAddress(AuthenticationUtil::getRealEndUserAddr(callingCommand).getIpAsString());

    req.setPid(accountId);
    req.setPersonaId(personaId);
    req.setExpandResults("true");

    if (searchType == Blaze::Nucleus::EntitlementSearchType::USER)
        req.getEntitlementSearchParams().setHasAuthorizedPersona("false");
    if ((productId != nullptr) && (productId[0] != '\0'))
        req.getEntitlementSearchParams().setProductId(productId);
    if ((projectId != nullptr) && (projectId[0] != '\0'))
        req.getEntitlementSearchParams().setProjectId(projectId);
    if ((entitlementTag != nullptr) && (entitlementTag[0] != '\0'))
        req.getEntitlementSearchParams().setEntitlementTag(entitlementTag);
    if (entitlementType != Blaze::Nucleus::EntitlementType::UNKNOWN)
        req.getEntitlementSearchParams().setEntitlementType(Blaze::Nucleus::EntitlementType::CodeToString(entitlementType));
    if (entitlementStatus != Blaze::Nucleus::EntitlementStatus::UNKNOWN)
        req.getEntitlementSearchParams().setStatus(Blaze::Nucleus::EntitlementStatus::CodeToString(entitlementStatus));

    if ((rc = AuthenticationSlaveImpl::isValidGroupNameList(groupNameList)) != ERR_OK)
    {
        // Nucleus does not allow empty string as a group name filter, caller should pass NONE if they mean ""
        ERR_LOG("[HasEntitlementUtil].getEntitlements: Nucleus does not allow an empty string as a group name filter. "
            << "Caller should pass NONE to search for those without groupname. Failed with error[" << ErrorHelp::getErrorName(rc) << "]");
        return rc;
    }

    groupNameList.copyInto(req.getEntitlementSearchParams().getGroupNames());

    ComponentId componentId = NucleusIdentity::NucleusIdentitySlave::COMPONENT_ID;
    CommandId commandId;

    switch (searchType)
    {
        case Blaze::Nucleus::EntitlementSearchType::USER:
        case Blaze::Nucleus::EntitlementSearchType::USER_OR_PERSONA:
        {
            commandId = NucleusIdentity::NucleusIdentitySlave::CMD_GETACCOUNTENTITLEMENTS;
            break;
        }
        case Blaze::Nucleus::EntitlementSearchType::PERSONA:
        {
            commandId = NucleusIdentity::NucleusIdentitySlave::CMD_GETPERSONAENTITLEMENTS;
            break;
        }
        default:
        {
            ERR_LOG("[HasEntitlementUtil].getEntitlements: search by type: " <<  TypeToString(searchType) << " is not yet implemented");
            return ERR_SYSTEM;
        }
    }

    // This is a workaround as currently going through gOutboundHttpService and the component RPC interface does not allow you to access the status code
    const RestResourceInfo* restInfo = BlazeRpcComponentDb::getRestResourceInfo(componentId, commandId);
    if (restInfo == nullptr)
    {
        WARN_LOG("[HasEntitlementUtil].getEntitlements: No rest resource info found for (component= '" 
            << BlazeRpcComponentDb::getComponentNameById(componentId) 
            << "', command='" << BlazeRpcComponentDb::getCommandNameById(componentId, commandId) << "').");
        return ERR_SYSTEM;
    }

    HttpConnectionManagerPtr connMgr = gOutboundHttpService->getConnection(NucleusIdentity::NucleusIdentitySlave::COMPONENT_INFO.name);
    if (connMgr.get() == nullptr)
    {
        WARN_LOG("[HasEntitlementUtil].getEntitlements: HTTP service not available for '" << NucleusIdentity::NucleusIdentitySlave::COMPONENT_INFO.name << "' proxy service.");
        return ERR_SYSTEM;
    }

    eastl::string uriPrefix;
    if (restInfo->apiVersion != nullptr && restInfo->apiVersion[0] != '\0')
    {
        uriPrefix.append_sprintf("/%s", restInfo->apiVersion);
    }

    const char8_t* contentType = connMgr->getDefaultContentType();
    if (restInfo->contentType != nullptr)
        contentType = restInfo->contentType;

    HttpStatusCode statusCode = 0;

    rc = RestProtocolUtil::sendHttpRequest(connMgr, componentId, commandId, &req, contentType, restInfo->addEncodedPayload, &response, &error,
        nullptr, uriPrefix.c_str(), &statusCode);

    error.copyInto(mError);

    if (rc != ERR_OK)
    {
        StringBuilder restRequestLog;
        RestProtocolUtil::printHttpRequest(componentId, commandId, &req, restRequestLog);
        ERR_LOG("[HasEntitlementUtil].getEntitlements: failed request " << restRequestLog << " with rpc error " << ErrorHelp::getErrorName(rc) 
            << " and status error " << statusCode <<  "]. Error response: " << error);
    }
    else
    {
        mIsSuccess = (statusCode == HTTP_STATUS_OK) || (statusCode == HTTP_STATUS_SUCCESS);

        if (!mIsSuccess)
        {
            StringBuilder restRequestLog;
            RestProtocolUtil::printHttpRequest(componentId, commandId, &req, restRequestLog);
            ERR_LOG("[HasEntitlementUtil].getEntitlements: failed request due to http status " << restRequestLog << " with rpc error " << ErrorHelp::getErrorName(rc)
                << " and status error " << statusCode << "]. Error response: " << error);

            return AuthenticationUtil::authFromOAuthError(mComponent.oAuthSlaveImpl()->getErrorCodes()->getError(getError()));
        }
    }

    return rc;
}

} // Authentication
} // Blaze
