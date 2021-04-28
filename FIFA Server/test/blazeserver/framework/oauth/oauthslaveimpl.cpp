/*************************************************************************************************/
/*!
    \file   oauthslaveimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class OAuthSlaveImpl

    OAuth Slave implementation.

    \note

*/
/*************************************************************************************************/


#include "framework/blaze.h"
#include "framework/logger.h"
#include "framework/util/shared/blazestring.h"
#include "framework/oauth/oauthslaveimpl.h"
#include "framework/oauth/accesstokenutil.h"
#include "framework/oauth/oauthutil.h"
#include "framework/tdf/oauth_server.h"
#include "framework/tdf/usersessiontypes_server.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/connection/outboundhttpservice.h"
#include "framework/protocol/restprotocolutil.h"
#include "framework/util/random.h"

#include "nucleusconnect/rpc/nucleusconnectslave.h"
#include "nucleusidentity/rpc/nucleusidentityslave.h"

namespace Blaze
{

EA_THREAD_LOCAL OAuth::OAuthSlaveImpl *gOAuthManager = nullptr;

namespace OAuth
{

static const int64_t USER_REFRESH_TOKEN_CHECK_GRACE_PERIOD = 10 * 60 * 1000LL * 1000LL; // 10 minutes
static const int64_t PSN_S2S_ACCESS_TOKEN_REFRESH_PERIOD_SEC = 5 * 60; // Request a new PSN s2s access toekn this many second prior to the current one expiring
const char8_t* kJwtTokenFormat = "JWS"; //JWS is not a typo.

OAuthSlave* OAuthSlave::createImpl()
{
    return BLAZE_NEW_NAMED("OAuthSlave") OAuthSlaveImpl();
}

Blaze::Nucleus::ErrorCodes OAuthSlaveImpl::mErrorCodes(true);

OAuthSlaveImpl::OAuthSlaveImpl()
    : mBlazeIdToCachedFirstPartyTokens(RedisId("OAuth", "mBlazeIdToCachedFirstPartyTokens"), false)
    , mTokenInfoByUserSessionIdCache(RedisId("OAuth", "mUserSessionIdToCachedUserTokenMap"), false)
    , mAccessTokenToCachedTrustedTokens(RedisId("OAuth", "mAccessTokenToCachedTrustedTokens"), false)
    , mAccessTokenToIdentityContexts(RedisId("OAuth", "mAccessTokenToIdentityContexts"), false)
    , mUpdateUserSessionTokensTimerId(INVALID_TIMER_ID)
    , mUpdateServerTokensTimerId(INVALID_TIMER_ID)
    , mBackgroundJobQueue("OAuthSlaveImpl::mBackgroundJobQueue")
    , mServerAccessTokenRefreshFails(*Metrics::gFrameworkCollection, "oauth.serverAccessTokenRefreshFails")
    , mUserAccessTokenRefreshFails(*Metrics::gFrameworkCollection, "oauth.userAccessTokenRefreshFails")
    , mUserFirstPartyTokenRetrievalFails(*Metrics::gFrameworkCollection, "oauth.userFirstPartyTokenRetrievalFails")
    , mUserTokenFailForcedLogouts(*Metrics::gFrameworkCollection, "oauth.userTokenFailForcedLogouts")
    , mPsnServerAccessTokenRefreshTimerId(INVALID_TIMER_ID)
    , mJwtUtil(this)
{
    gOAuthManager = this;

    mBackgroundJobQueue.setMaximumWorkerFibers(FiberJobQueue::LARGE_WORKER_LIMIT);
}

OAuthSlaveImpl::~OAuthSlaveImpl()
{
    mBackgroundJobQueue.cancelAllWork();
    gSelector->cancelTimer(mPsnServerAccessTokenRefreshTimerId);
    mPsnServerAccessTokenRefreshTimerId = INVALID_TIMER_ID;

    gOAuthManager = nullptr;
}

bool OAuthSlaveImpl::isMock() const
{
    return (getConfig().getUseMock() || gController->getUseMockConsoleServices());
}

BlazeRpcError OAuthSlaveImpl::getAccessTokenByAuthCode(const char8_t *authorizationCode, NucleusConnect::GetAccessTokenResponse &response,
    const char8_t* productName, const InetAddress& inetAddr)
{
    NucleusConnect::GetAccessTokenRequest request;

    request.setGrantType("authorization_code");
    request.setCode(authorizationCode);
    request.setRedirectUri(getConfig().getRedirectUri());
    request.setIpAddress(inetAddr.getIpAsString());

    if (getConfig().getTokenFormat() == TOKEN_FORMAT_JWT)
        request.setTokenFormat(kJwtTokenFormat);

    return getAccessToken(request, response, productName);
}

BlazeRpcError OAuthSlaveImpl::getAccessTokenByRefreshToken(const char8_t *refreshToken, NucleusConnect::GetAccessTokenResponse &response,
    const char8_t* productName, const InetAddress& inetAddr, bool isUserToken)
{
    NucleusConnect::GetAccessTokenRequest request;

    request.setGrantType("refresh_token");
    request.setRefreshToken(refreshToken);
    request.setIpAddress(inetAddr.getIpAsString());

    if (getConfig().getTokenFormat() == TOKEN_FORMAT_JWT && isUserToken)
        request.setTokenFormat(kJwtTokenFormat);

    return getAccessToken(request, response, productName);
}

BlazeRpcError OAuthSlaveImpl::getAccessTokenByPassword(const char8_t *email, const char8_t *password, NucleusConnect::GetAccessTokenResponse &response,
    const char8_t* productName, const InetAddress& inetAddr)
{
    NucleusConnect::GetAccessTokenRequest request;

    request.setGrantType("password");
    request.setEmail(email);
    request.setPassword(password);
    request.setIpAddress(inetAddr.getIpAsString());

    request.setClientId("test_token_issuer");
    request.setClientSecret("test_token_issuer_secret");
    request.setReleaseType(gUserSessionManager->getProductReleaseType(productName));

    HttpConnectionManagerPtr nucleusConnectExtConn = gOutboundHttpService->getConnection("nucleusconnect_ext");
    if (nucleusConnectExtConn == nullptr)
    {
        BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getAccessTokenByPassword: nucleusconnect is nullptr");
        return ERR_SYSTEM;
    }

    NucleusConnect::ErrorResponse errorResponse;
    BlazeRpcError err = RestProtocolUtil::sendHttpRequest(nucleusConnectExtConn, NucleusConnect::NucleusConnectSlave::CMD_INFO_GETACCESSTOKEN.restResourceInfo,
        &request, RestProtocolUtil::getContentTypeFromEncoderType(Encoder::JSON), &response, &errorResponse);
    if (err != ERR_OK)
    {
        err = OAuthUtil::parseNucleusConnectError(errorResponse, err);
        BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getAccessTokenByPassword: sendHttpRequest failed with error(" << ErrorHelp::getErrorName(err) << ")");
        return err;
    }

    return err;
}

BlazeRpcError OAuthSlaveImpl::getAccessTokenByExchange(const char8_t *accessToken, NucleusConnect::GetAccessTokenResponse &response, const char8_t* productName, const InetAddress& inetAddr)
{
    NucleusConnect::GetAccessTokenRequest request;

    request.setGrantType("exchange");
    request.setCurrentAccessToken(accessToken);
    request.setIpAddress(inetAddr.getIpAsString());

    return getAccessToken(request, response, productName);
}

BlazeRpcError OAuthSlaveImpl::getAuthCodeForPersona(const char8_t* personaName, const char8_t* personaNamespace, const char8_t* accessToken, const char8_t* productName, NucleusConnect::GetAuthCodeResponse& resp)
{
    NucleusConnect::NucleusConnectSlave* connectComp = (NucleusConnect::NucleusConnectSlave*)gOutboundHttpService->getService(NucleusConnect::NucleusConnectSlave::COMPONENT_INFO.name);
    if (connectComp == nullptr)
    {
        BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getAuthCodeForPersona: nucleusconnect is nullptr");
        return ERR_SYSTEM;
    }

    NucleusConnect::GetAuthCodeRequest req;
    NucleusConnect::ErrorResponse errorResponse;

    req.setResponseType("code");
    req.setAccessToken(accessToken);
    req.setPersonaName(personaName);
    req.setPersonaNamespace(personaNamespace);
    req.setClientId(gController->getBlazeServerNucleusClientId());
    req.setReleaseType(gUserSessionManager->getProductReleaseType(productName));

    BlazeRpcError rc = connectComp->getAuthCode(req, resp, errorResponse);
    if (rc != ERR_OK)
    {
        rc = OAuthUtil::parseNucleusConnectError(errorResponse, rc);
        BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getAuthCodeForPersona: getAuthCode failed with error(" << ErrorHelp::getErrorName(rc) << ")");
        return rc;
    }

    return rc;
}

BlazeRpcError OAuthSlaveImpl::getTokenInfo(const NucleusConnect::GetTokenInfoRequest& request, NucleusConnect::GetTokenInfoResponse& response)
{
    BlazeRpcError rc = ERR_SYSTEM;

    if (mJwtUtil.isJwtFormat(request.getAccessToken()))
    {
        rc = mJwtUtil.decodeJwtAccessToken(request.getAccessToken(), response);
        if (rc != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getTokenInfo: Failed to decode JWT access token with error(" << ErrorHelp::getErrorName(rc) << "), token is: " << request.getAccessToken());
            return rc;
        }
    }
    else
    {
        NucleusConnect::NucleusConnectSlave* comp = (NucleusConnect::NucleusConnectSlave*)gOutboundHttpService->getService(NucleusConnect::NucleusConnectSlave::COMPONENT_INFO.name);
        if (comp == nullptr)
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getTokenInfo: nucleusconnect is nullptr");
            return ERR_SYSTEM;
        }

        NucleusConnect::ErrorResponse errorResponse;
        rc = comp->getTokenInfo(request, response, errorResponse);
        if (rc != ERR_OK)
        {
            rc = OAuthUtil::parseNucleusConnectError(errorResponse, rc);
            BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getTokenInfo: getTokenInfo failed with error(" << ErrorHelp::getErrorName(rc) << ")");
            return rc;
        }
    }

    return rc;
}

BlazeRpcError OAuthSlaveImpl::getJwtPublicKeyInfo(NucleusConnect::GetJwtPublicKeyInfoResponse& response)
{
    NucleusConnect::NucleusConnectSlave* comp = (NucleusConnect::NucleusConnectSlave*)gOutboundHttpService->getService(NucleusConnect::NucleusConnectSlave::COMPONENT_INFO.name);
    if (comp == nullptr)
    {
        BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getJwtPublicKeyInfo: nucleusconnect is nullptr");
        return ERR_SYSTEM;
    }

    NucleusConnect::ErrorResponse errorResponse;
    BlazeRpcError rc = comp->getJwtPublicKeyInfo(response, errorResponse);
    if (rc != ERR_OK)
    {
        rc = OAuthUtil::parseNucleusConnectError(errorResponse, rc);
        BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getJwtPublicKeyInfo: getJwtPublicKeyInfo failed with error(" << ErrorHelp::getErrorName(rc) << ")");
        return rc;
    }

    return rc;
}

BlazeRpcError OAuthSlaveImpl::getServerAccessToken(const GetServerAccessTokenRequest& request, GetServerAccessTokenResponse& response)
{
    BlazeRpcError err = ERR_OK;

    const char8_t* clientId = gController->getBlazeServerNucleusClientId();
    if (clientId == nullptr || *clientId == '\0')
    {
        BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getServerAccessToken: failed to get Blaze server Nucleus client id.");
        return ERR_SYSTEM;
    }

    eastl::string accessToken;
    TimeValue validUntil;
    err = getServerAccessToken(accessToken, validUntil, clientId, request.getAllowedScopes());
    if (err == ERR_OK)
    {
        response.setClientId(clientId);
        response.setAccessToken(accessToken.c_str());
        response.setValidUntil(validUntil);
    }
    else
    {
        BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getServerAccessToken: failed to get a server access token. Error(" << ErrorHelp::getErrorName(err) << ")");
    }

    return err;
}

BlazeRpcError OAuthSlaveImpl::getServerAccessToken(eastl::string& accessToken, TimeValue& validUntil, const char8_t* clientId, const char8_t* allowedScope/* = nullptr*/)
{
    BlazeRpcError err = ERR_SYSTEM;

    // https://eadpjira.ea.com/browse/GOS-31481 added release_type when acquiring user access token.
    // However, the code flow is such that it acquires release_type of the default product for the server's access token.
    // After talking to Identity, while release_type is not used for the server access token, they did not rule out it's consumption in future.
    // So for server's access token, we use default product's release type.
    const char8_t* defaultProductName = gController->getDefaultProductName();

    // Check the cached tokens first if we are not fetching a token with specific scope. These tokens are cached.
    if ((allowedScope == nullptr) || (allowedScope[0] == '\0'))
    {
        bool fetchRestrictedToken = !UserSession::hasSuperUserPrivilege();

        CachedTokenInfo* serverTokenInfo = getServerCachedToken((fetchRestrictedToken ? TOKENTYPE_SERVER_RESTRICTED_SCOPE : TOKENTYPE_SERVER_FULL_SCOPE), clientId);
        if (serverTokenInfo != nullptr && serverTokenInfo->getAccessToken()[0] != '\0')
        {
            if (TimeValue::getTimeOfDay() + getConfig().getMinValidDurationForServerAccessToken() > (serverTokenInfo->getAccessTokenExpire()))
            {
                NucleusConnect::GetAccessTokenResponse accessTokenResponse;
                err = getAccessTokenByRefreshToken(serverTokenInfo->getRefreshToken(), accessTokenResponse, defaultProductName, NameResolver::getInternalAddress(), false);
                if (err == ERR_OK)
                {
                    populateCachedTokenInfo(*serverTokenInfo, accessTokenResponse, clientId, defaultProductName, NameResolver::getInternalAddress().getIpAsString());
                }
                else
                {
                    mServerAccessTokenRefreshFails.increment();
                }
            }
            else
            {
                // We found a cached token that is valid for the minimum duration we are comfortable with.
                err = ERR_OK;
            }
        }

        // We failed to find the cached server access token (it could be expired or we never had one or failed to refresh it via refresh token), so fetch a fresh one from Nucleus
        if (err != ERR_OK)
        {
            NucleusConnect::GetAccessTokenRequest request;
            request.setGrantType("client_credentials");
            request.setIpAddress(NameResolver::getInternalAddress().getIpAsString());

            if (fetchRestrictedToken)
                request.setScope(getConfig().getRestrictedScope());
            //else
            // Nucleus returns token with full scopes configured for the client if no scope is specified.

            NucleusConnect::GetAccessTokenResponse accessTokenResponse;
            err = getAccessToken(request, accessTokenResponse, defaultProductName);
            if (err == ERR_OK)
            {
                if (serverTokenInfo == nullptr)
                {
                    serverTokenInfo = fetchRestrictedToken ? &mServerTokens[clientId].second : &mServerTokens[clientId].first;
                }
                populateCachedTokenInfo(*serverTokenInfo, accessTokenResponse, clientId, defaultProductName, NameResolver::getInternalAddress().getIpAsString());
            }
        }

        if (err == ERR_OK && serverTokenInfo != nullptr)
        {
            accessToken.assign(serverTokenInfo->getAccessToken());
            validUntil = TimeValue::getTimeOfDay() + getConfig().getMinValidDurationForServerAccessToken();
            if (validUntil > serverTokenInfo->getAccessTokenExpire())
            {
                BLAZE_WARN_LOG(Log::OAUTH, "[OAuthSlaveImpl].getServerAccessToken: cached token expires in less than configured min valid duration");
                validUntil = serverTokenInfo->getAccessTokenExpire();
            }
        }
        else
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getServerAccessToken: failed to get a server access token. Error(" << ErrorHelp::getErrorName(err) << ")");
        }
    }
    else
    {
        // fetch access token from nucleus by the specified allowed scopes. These tokens are not cached.
        NucleusConnect::GetAccessTokenRequest request;
        request.setGrantType("client_credentials");
        request.setIpAddress(NameResolver::getInternalAddress().getIpAsString());
        request.setScope(allowedScope);

        NucleusConnect::GetAccessTokenResponse response;
        err = getAccessToken(request, response, defaultProductName);
        if (err == ERR_OK)
        {
            accessToken.assign(response.getAccessToken());
            if (getConfig().getMinValidDurationForServerAccessToken() > ((int64_t)response.getExpiresIn() * 1000 * 1000))
            {
                BLAZE_WARN_LOG(Log::OAUTH, "[OAuthSlaveImpl].getServerAccessToken: non-cached token expires in less than configured min valid duration");
                validUntil = TimeValue::getTimeOfDay() + ((int64_t)response.getExpiresIn() * 1000 * 1000);
            }
            else
            {
                validUntil = TimeValue::getTimeOfDay() + getConfig().getMinValidDurationForServerAccessToken();
            }
        }
        else
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getServerAccessToken: failed to get a server access token. Error(" << ErrorHelp::getErrorName(err) << "). Using scope(" << (allowedScope ? allowedScope : "<null>") << ").");
        }
    }

    return err;
}

BlazeRpcError OAuthSlaveImpl::getAccessTokenByUserSessionId(UserSessionId userSessionId, const InetAddress& inetAddr, CachedTokenInfo& cachedUserToken, TimeValue& validUntil)
{
    if (userSessionId == INVALID_USER_SESSION_ID && gCurrentUserSession != nullptr)
    {
        // Unfortunately, this fallback is needed because Arson calls getUserAccessToken rpc directly and expects us to return the user's access token to
        // send it to other services.
        userSessionId = gCurrentUserSession->getUserSessionId();
    }

    if (userSessionId == INVALID_USER_SESSION_ID)
    {
        BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getAccessTokenByUserSessionId: userSessionId is invalid.");
        return ERR_SYSTEM;
    }

    BlazeRpcError err = Blaze::ERR_SYSTEM;

    RedisError redisError = mTokenInfoByUserSessionIdCache.find(userSessionId, cachedUserToken);
    if (redisError == REDIS_ERR_OK && cachedUserToken.getAccessToken()[0] != '\0')
    {
        // user is logged in and we have their access token in cache. Update the access token if necessary.
        err = updateUserSessionCachedTokenIfNeeded(userSessionId, cachedUserToken, inetAddr, getConfig().getMinValidDurationForUserSessionAccessToken());
        if (err != ERR_OK)
            BLAZE_WARN_LOG(Log::OAUTH, "[OAuthSlaveImpl].getAccessTokenByUserSessionId: Failed to fetch access token from cache with error(" << ErrorHelp::getErrorName(err) << ")");
    }
    else
    {
        BLAZE_WARN_LOG(Log::OAUTH, "[OAuthSlaveImpl].getAccessTokenByUserSessionId: mTokenInfoByUserSessionIdCache.find had a redis error (" << (int)redisError.error << ") or was missing the access token.");
    }

    if (err == ERR_OK)
    {
        validUntil = TimeValue::getTimeOfDay() + getConfig().getMinValidDurationForUserSessionAccessToken();
        if (validUntil > cachedUserToken.getAccessTokenExpire())
        {
            BLAZE_WARN_LOG(Log::OAUTH, "[OAuthSlaveImpl].getAccessTokenByUserSessionId: cached token for id(" << userSessionId << ") expires in less than configured min valid duration");
            validUntil = cachedUserToken.getAccessTokenExpire();
        }
    }

    return err;
}


BlazeRpcError OAuthSlaveImpl::getUserAccessToken(const GetUserAccessTokenRequest& request, GetUserAccessTokenResponse& response, const InetAddress& inetAddr)
{
    // Need to pass this to the getAccessToken methods in the case that a new access token needs to be fetched.
    // This value can be overridden if a token already exists for a user, but has a different clientId (i.e., the clientId has been reconfigured)
    const char8_t* clientId = gController->getBlazeServerNucleusClientId();
    if (clientId == nullptr || *clientId == '\0')
    {
        BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getUserAccessToken: failed to get blaze server client id.");
        return ERR_SYSTEM;
    }

    BlazeRpcError err = ERR_SYSTEM;

    CachedTokenInfo cachedUserToken;
    TimeValue validUntil;
    err = getAccessTokenByUserSessionId(request.getUserSessionId(), inetAddr, cachedUserToken, validUntil);

    if (err == ERR_OK)
    {
        response.setAccessToken(cachedUserToken.getAccessToken());
        response.setClientId(cachedUserToken.getClientId());
        response.setValidUntil(validUntil);
    }
    else
    {
        BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getUserAccessToken: failed to get an access token for UserSessionId(" << request.getUserSessionId() << "). "
            << "Error("<<ErrorHelp::getErrorName(err)<<").");

        if (err == USER_ERR_USER_NOT_FOUND)
        {
            err = OAUTH_ERR_INVALID_USER;
            gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(err), RiverPoster::authentication_failed);
        }
    }

    return err;
}



BlazeRpcError OAuthSlaveImpl::getUserXblToken(const GetUserXblTokenRequest& request, GetUserXblTokenResponse& xblToken, const InetAddress& inetAddr)
{
    BlazeRpcError rc = ERR_OK;

    const char8_t* clientId = gController->getBlazeServerNucleusClientId();
    if (clientId == nullptr || *clientId == '\0')
    {
        BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getUserXblToken: failed to get blaze server client id.");
        return ERR_SYSTEM;
    }

    switch (request.getRetrieveUsing().getActiveMember())
    {
    case XblTokenOwnerId::MEMBER_PERSONAID:
    {
        rc = doGetXBLToken(request, xblToken, inetAddr);
        if (rc != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getUserXblToken: Failed to retrieve xbl token for persona id(" << request.getRetrieveUsing().getPersonaId() << "), error(" << ErrorHelp::getErrorName(rc) << ").");
        }
        break;
    }

    case XblTokenOwnerId::MEMBER_EXTERNALID:
    {
        ExternalXblAccountId xblId = request.getRetrieveUsing().getExternalId();
        UserInfoPtr userInfo;
        BlazeId personaId = INVALID_BLAZE_ID;

        PlatformInfo tempPlatformInfo;
        convertToPlatformInfo(tempPlatformInfo, xblId, nullptr, INVALID_ACCOUNT_ID, xone);
        rc = gUserSessionManager->lookupUserInfoByPlatformInfo(tempPlatformInfo, userInfo);
        if (rc == USER_ERR_USER_NOT_FOUND)
        {
            // We have no knowledge of this user, but they may still exist in Nucleus, so check there
            NucleusConnect::GetAccessTokenResponse response;
            OAuth::AccessTokenUtil tokenUtil;
            rc = tokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_BEARER);

            if (rc == ERR_OK)
            {
                NucleusIdentity::NucleusIdentitySlave* comp = (NucleusIdentity::NucleusIdentitySlave*)gOutboundHttpService->getService(NucleusIdentity::NucleusIdentitySlave::COMPONENT_INFO.name);
                if (comp == nullptr)
                {
                    BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getUserXblToken: nucleusidentity is nullptr");
                    return ERR_SYSTEM;
                }

                char8_t buf[32];
                blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, xblId);

                NucleusIdentity::GetPersonaInfoExtIdRequest req;
                req.getAuthCredentials().setAccessToken(tokenUtil.getAccessToken());
                req.getAuthCredentials().setClientId(tokenUtil.getClientId());
                req.getFilter().setExternalRefType("XBOX");
                req.getFilter().setExternalRefValue(buf);
                req.setExpandResults("true");

                NucleusIdentity::GetPersonaListResponse resp;
                NucleusIdentity::IdentityError error;
                rc = comp->getPersonaInfoByExtId(req, resp, error);
                if (rc == ERR_OK)
                {
                    size_t personaCount = resp.getPersonas().getPersona().size();
                    if (personaCount == 1)
                        personaId = resp.getPersonas().getPersona().front()->getPersonaId();
                    else
                    {
                        BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getUserXblToken: Get persona by externalId returned " << personaCount << " personas.");
                        return OAUTH_ERR_NO_XBLTOKEN;
                    }
                }
                else
                {
                    rc = OAuthUtil::parseNucleusIdentityError(error, rc);
                    BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getUserXblToken: getPersonaInfoByExtId failed with error(" << ErrorHelp::getErrorName(rc) << ")");
                }
            }
        }
        else if (rc == ERR_OK)
        {
            personaId = userInfo->getId();
        }

        if (personaId != INVALID_BLAZE_ID && rc == ERR_OK)
        {
            GetUserXblTokenRequest xblTokenRequest;
            xblTokenRequest.getRetrieveUsing().setPersonaId(personaId);
            xblTokenRequest.setForceRefresh(request.getForceRefresh());
            xblTokenRequest.setRelyingParty(request.getRelyingParty());
            rc = doGetXBLToken(xblTokenRequest, xblToken, inetAddr);
        }
        else
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getUserXblToken: Failed to look up personaId for xblId(" << xblId << "), error(" << ErrorHelp::getErrorName(rc) << ").");
        }
        break;
    }
    default:
    {
        BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getUserXblToken: No known method for user access token retrieval set. Method is("<< request.getRetrieveUsing().getActiveMember() <<").");
        return ERR_SYSTEM;
    }
    }

    return rc;
}

BlazeRpcError OAuthSlaveImpl::getUserPsnToken(const GetUserPsnTokenRequest& request, GetUserPsnTokenResponse& psnToken, const InetAddress& inetAddr)
{
    BlazeRpcError rc = ERR_OK;

    const char8_t* clientId = gController->getBlazeServerNucleusClientId();
    if (clientId == nullptr || *clientId == '\0')
    {
        BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getUserPsnToken: failed to get blaze server client id.");
        return ERR_SYSTEM;
    }

    switch (request.getRetrieveUsing().getActiveMember())
    {

        case PsnTokenOwnerId::MEMBER_PERSONAID:
        {
            BLAZE_TRACE_LOG(Log::OAUTH, "[OAuthSlaveImpl].getUserPsnToken: retrieving token for user by persona id(" << request.getRetrieveUsing().getPersonaId() << ")");
            rc = doGetPSNToken(request, psnToken, inetAddr);
            break;
        }
        case PsnTokenOwnerId::MEMBER_EXTERNALID:
        {
            ExternalPsnAccountId psnId = request.getRetrieveUsing().getExternalId();
            if (psnId == INVALID_EXTERNAL_ID)
            {
                BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getUserPsnToken: unexpected internal error. requests PSN Token Owner Id ExternalId was missing.");
                return ERR_SYSTEM;
            }
            UserInfoPtr userInfo;
            BlazeId personaId = INVALID_BLAZE_ID;
            BLAZE_TRACE_LOG(Log::OAUTH, "[OAuthSlaveImpl].getUserPsnToken: retrieving token for user by external id(" << psnId << ").");

            PlatformInfo tempPlatformInfo;
            convertToPlatformInfo(tempPlatformInfo, psnId, nullptr, INVALID_ACCOUNT_ID, request.getPlatform());
            rc = gUserSessionManager->lookupUserInfoByPlatformInfo(tempPlatformInfo, userInfo);
            if (rc == USER_ERR_USER_NOT_FOUND)
            {
                // unlike X1, no need trying Nucleus here, to see if its a new-to-title user, as NP sessions don't support reservations
                BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getUserPsnToken: Get persona by psnId(" << psnId << ") returned no personas, error(" << ErrorHelp::getErrorName(rc) << ").");
                return OAUTH_ERR_INVALID_USER;
            }
            else if (rc == ERR_OK)
            {
                personaId = userInfo->getId();
            }

            if (personaId != INVALID_BLAZE_ID && rc == ERR_OK)
            {
                GetUserPsnTokenRequest tokenRequest;
                tokenRequest.getRetrieveUsing().setPersonaId(personaId);
                tokenRequest.setForceRefresh(request.getForceRefresh());
                tokenRequest.setPlatform(request.getPlatform());
                rc = doGetPSNToken(tokenRequest, psnToken, inetAddr);
            }
            else
            {
                BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getUserPsnToken: Failed to look up personaId for psnId(" << psnId << "), error(" << ErrorHelp::getErrorName(rc) << ").");
            }
            break;
        }
        default:
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getUserPsnToken: No known method for user access token retrieval set. Method is(" << request.getRetrieveUsing().getActiveMember() << ").");
            return ERR_SYSTEM;
        }
    }

    return rc;
}

BlazeRpcError OAuthSlaveImpl::getIdentityContext(const IdentityContextRequest& identityContextRequest, IdentityContext& identityContext)
{
    RedisError redisErr = mAccessTokenToIdentityContexts.find(identityContextRequest.getAccessToken(), identityContext);

    // If we find a cached identity context in Redis and it belongs to the requested platform, return it.
    // But if the platform does not match, crack the token again.
    if (redisErr == REDIS_ERR_OK
        && identityContext.getPlatform() == identityContextRequest.getPlatform())
    {
        // So this is weird but our hand is forced by Identity/Services non-standard usage of token prefixes. The oauth token in authorization header should
        // always be prefixed with "Bearer". But custom prefixes like "Nexus", "NEXUS_S2S" have been invented and the same token can flow in the system using
        // different prefixes. So we simply fill in the prefix type for the token based on what was requested.
        identityContext.setAccessTokenType(identityContextRequest.getAccessTokenType());
    }
    else
    {
        NucleusConnect::GetTokenInfoRequest tokenInfoRequest;
        tokenInfoRequest.setAccessToken(identityContextRequest.getAccessToken());
        tokenInfoRequest.setIpAddress(identityContextRequest.getIpAddr());

        // The token was requested with a Nucleus client id that belongs to the user of the Blaze (so a game client). The client id here
        // is Blaze's client id to simply inform the Nucleus on who is cracking the token. This client id need not be the client id that
        // requested the token (and this is common case).
        tokenInfoRequest.setClientId(gController->getBlazeServerNucleusClientId());

        NucleusConnect::GetTokenInfoResponse tokenInfoResponse;
        BlazeRpcError err = getTokenInfo(tokenInfoRequest, tokenInfoResponse);
        if (err != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getIdentityContext: Failed to get token info from Nucleus(" << ErrorHelp::getErrorName(err) << ")");
            return OAUTH_ERR_INVALID_TOKEN;
        }
        else
        {
            // + This information is being copied from the request because it has been already validated by the getTokenInfo rpc call above.
            // (meaning we are not acting solely on user provided info).
            identityContext.setAccessTokenType(identityContextRequest.getAccessTokenType());
            identityContext.setAccessToken(identityContextRequest.getAccessToken());
            identityContext.setPlatform(identityContextRequest.getPlatform());
            //-

            eastl::vector<eastl::string> scopes;
            blaze_stltokenize(eastl::string(tokenInfoResponse.getScope()), " ", scopes);
            for (auto& scope : scopes)
                identityContext.getAccessTokenScopes().push_back().set(scope.c_str());

            AccountId accountId = INVALID_ACCOUNT_ID;
            blaze_str2int(tokenInfoResponse.getAccountId(), &accountId);
            identityContext.setNucleusId(accountId);

            identityContext.setPersonaId(tokenInfoResponse.getPersonaId());
        }

        // Add more info to identityContext here in future if required.



        TimeValue tokenTTL = tokenInfoResponse.getExpiresIn() * 1000 * 1000;
        // upsert instead of insert in case another request is using the same token on another instance concurrently
        redisErr = mAccessTokenToIdentityContexts.upsertWithTtl(identityContextRequest.getAccessToken(), identityContext, tokenTTL);
        if (redisErr != REDIS_ERR_OK)
        {
            // Not being able to cache in redis is not fatal.
            BLAZE_WARN_LOG(Log::OAUTH, "[OAuthSlaveImpl].getIdentityContext: Failed to cache identityContext in Redis(" << RedisErrorHelper::getName(redisErr) << ")");
        }
    }

    // Before returning the IdentityContext, let's make sure that it has sufficient privileges for the component hosting the RPC that the caller wants to execute.
    bool hasComponentAuthorization = false;
    {
        auto compScopesIter = mComponentScopesMap.find(identityContextRequest.getComponentName());
        if (compScopesIter == mComponentScopesMap.end())
        {
            // If the token is valid and no additional scoping requirement is added for the component, then the token has component level authorization.
            hasComponentAuthorization = true;
        }
        else
        {
            const auto& allowedScopes = compScopesIter->second;
            for (const auto& allowedScope : allowedScopes)
            {
                if (eastl::find(identityContext.getAccessTokenScopes().begin(), identityContext.getAccessTokenScopes().end(), allowedScope)
                    != identityContext.getAccessTokenScopes().end())
                {
                    hasComponentAuthorization = true;
                    break;
                }
            }
        }
    }

    if (!hasComponentAuthorization)
    {
        BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getIdentityContext: The token does not have sufficient component level scope"
            << " to execute " << identityContextRequest.getComponentName() << "::" << identityContextRequest.getRpcName()<<" rpc. Associated scopes: "<< identityContext.getAccessTokenScopes());

        return OAUTH_ERR_INSUFFICIENT_SCOPES;
    }

    return ERR_OK;
}


void OAuthSlaveImpl::addUserSessionTokenInfoToCache(const NucleusConnect::GetAccessTokenResponse& accessTokenResponse, const char8_t* productName, const char8_t* ipAddr)
{
    const char8_t* clientId = gController->getBlazeServerNucleusClientId();

    CachedTokenInfo tokenInfo;
    populateCachedTokenInfo(tokenInfo, accessTokenResponse, clientId, productName, ipAddr);

    RedisError err = mTokenInfoByUserSessionIdCache.insertWithTtl(gCurrentUserSession->getCurrentUserSessionId(), tokenInfo, (int64_t)accessTokenResponse.getRefreshExpiresIn() * 1000 * 1000);
    if (err != REDIS_ERR_OK)
    {
        BLAZE_WARN_LOG(Log::OAUTH, "[OAuthSlaveImpl].addUserSessionTokenInfoToCache: failed to add the user's access token to redis for user session id ("
            << gCurrentUserSession->getCurrentUserSessionId() << "). Redis Error (" << RedisErrorHelper::getName(err) << ").");
    }
}

void OAuthSlaveImpl::removeUserSessionTokenInfoFromCache(UserSessionId userSessionId)
{
    int64_t keysRemoved = 0;
    RedisError err = mTokenInfoByUserSessionIdCache.erase(userSessionId, &keysRemoved);
    if (err != REDIS_ERR_OK)
    {
        BLAZE_WARN_LOG(Log::OAUTH, "[OAuthSlaveImpl].removeUserSessionTokenInfoFromCache: failed to remove the user's token info from redis for user session id " << userSessionId
            << ". RedisError: " << RedisErrorHelper::getName(err) << ".");
    }
    else
    {
        BLAZE_TRACE_LOG(Log::OAUTH, "[OAuthSlaveImpl].removeUserSessionTokenInfoFromCache: " << keysRemoved << " cached token info removed for user session id " << userSessionId << " from redis.");
    }
}

void OAuthSlaveImpl::forceUserLogout(const UserSessionId userSessionId, const char8_t* noteToLog)
{
    if (userSessionId == INVALID_USER_SESSION_ID)
    {
        BLAZE_WARN_LOG(Log::OAUTH, "[OAuthSlaveImpl].forceUserLogout: UserSessionId(" << userSessionId << ") is invalid. No forced logout.");
        return;
    }
    if (gCurrentUserSession == nullptr || gCurrentUserSession->getUserSessionId() != userSessionId)
    {
        BLAZE_WARN_LOG(Log::OAUTH, "[OAuthSlaveImpl].forceUserLogout: UserSessionId(" << userSessionId << ") does not match Current User Session UserSessionId(" << (gCurrentUserSession == nullptr ? INVALID_USER_SESSION_ID : gCurrentUserSession->getUserSessionId()) << ")");
    }

    const char8_t* personaName = gUserSessionManager->getPersonaName(userSessionId);
    if (personaName == nullptr)
    {
        BLAZE_WARN_LOG(Log::OAUTH, "[OAuthSlaveImpl].forceUserLogout: PersonaName is null for UserSessionId(" << userSessionId << ")");
        personaName = "null";
    }

    BLAZE_WARN_LOG(Log::OAUTH, "[OAuthSlaveImpl].forceUserLogout: Scheduling forced logout for UserSessionId(" << userSessionId << ") PersonaName(" << personaName << "). " << noteToLog << ".");
    mBackgroundJobQueue.queueFiberJob(this, &OAuthSlaveImpl::forceUserLogoutJob, userSessionId, "OAuthSlaveImpl::forceUserLogoutJob");
}

void OAuthSlaveImpl::forceUserLogoutJob(const UserSessionId userSessionId)
{
    //no forced logout if user is logged out already
    if (gUserSessionManager == nullptr || gUserSessionManager->getSession(userSessionId) == nullptr)
        return;

    ForceSessionLogoutRequest request;
    request.setSessionId(userSessionId);
    request.setReason(DISCONNECTTYPE_FORCED_LOGOUT);
    request.setForcedLogoutReason(FORCED_LOGOUTTYPE_RELOGIN_REQUIRED);

    BlazeRpcError err = gUserSessionManager->getMaster()->forceSessionLogoutMaster(request);
    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].forceUserLogoutJob: Error(" << ErrorHelp::getErrorName(err) << ") occurred when forceSessionLogoutMaster was called.");
        return;
    }

    mUserTokenFailForcedLogouts.increment();
    BLAZE_WARN_LOG(Log::OAUTH, "[OAuthSlaveImpl].forceUserLogoutJob: Forced logout for UserSessionId(" << userSessionId << ")");
}

bool OAuthSlaveImpl::onConfigure()
{
    return configure(false);
}

bool OAuthSlaveImpl::onReconfigure()
{
    return configure(true);
}

bool OAuthSlaveImpl::configure(bool isReconfigure)
{
    if (mUpdateUserSessionTokensTimerId != INVALID_TIMER_ID)
        gSelector->cancelTimer(mUpdateUserSessionTokensTimerId);

    if (mUpdateServerTokensTimerId != INVALID_TIMER_ID)
        gSelector->cancelTimer(mUpdateServerTokensTimerId);


    mUpdateUserSessionTokensTimerId = gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay() + getConfig().getMinValidDurationForUserSessionRefreshToken(),
        this, &OAuthSlaveImpl::onUpdateUserSessionCachedTokensTimer, "onUpdateUserSessionCachedTokensTimer");

    mUpdateServerTokensTimerId = gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay() + getConfig().getMinValidDurationForServerAccessToken(),
        this, &OAuthSlaveImpl::onUpdateServerCachedTokensTimer, "onUpdateServerCachedTokensTimer");

    // Cache the config in a more friendly tokenized structures
    mComponentScopesMap.clear();
    for (const auto& entry : getConfig().getComponentScopes())
    {
        eastl::vector<eastl::string> scopes;
        blaze_stltokenize(eastl::string(entry.second.c_str()), " ", scopes);
        mComponentScopesMap[entry.first.c_str()] = scopes;
    }

    if (!isReconfigure)
    {
        // Kick off the recurring refresh requests to PSN to get the server access token
        renewPsnServerAccessToken();
    }
    else
    {
        // On a reconfigure, the recurring request to PSN is already scheduled.  But, in case the reconfigure
        // actually changed something, lets schedule the refresh to occur within the next 10 seconds.
        if (mPsnServerAccessTokenRefreshTimerId != INVALID_TIMER_ID)
            gSelector->updateTimer(mPsnServerAccessTokenRefreshTimerId, TimeValue::getTimeOfDay() + (gRandom->getRandomNumber(10) * 1000 * 1000));
    }

    return true;
}

bool OAuthSlaveImpl::onResolve()
{
    return true;
}

void OAuthSlaveImpl::onShutdown()
{
    mBackgroundJobQueue.join();

    if (mUpdateUserSessionTokensTimerId != INVALID_TIMER_ID)
        gSelector->cancelTimer(mUpdateUserSessionTokensTimerId);

    if (mUpdateServerTokensTimerId != INVALID_TIMER_ID)
        gSelector->cancelTimer(mUpdateServerTokensTimerId);
}

bool OAuthSlaveImpl::onActivate()
{
    // nucleusconnect and nucleusidentity proxy components must be defined in framework.cfg for OAuth component
    // to be able to talk to Nucleus 2.0
    const Component* comp = gOutboundHttpService->getService(NucleusConnect::NucleusConnectSlave::COMPONENT_INFO.name);
    if (comp == nullptr)
    {
        BLAZE_FATAL_LOG(Log::OAUTH, "[OAuthSlaveImpl].onActivate: OAuth component has dependency on " << NucleusConnect::NucleusConnectSlave::COMPONENT_INFO.name
            << "HTTP service to be defined in framework.cfg.  Please make sure this service is defined.");
        return false;
    }
    comp = gOutboundHttpService->getService(NucleusIdentity::NucleusIdentitySlave::COMPONENT_INFO.name);
    if (comp == nullptr)
    {
        BLAZE_FATAL_LOG(Log::OAUTH, "[OAuthSlaveImpl].onActivate: OAuth component has dependency on " << NucleusIdentity::NucleusIdentitySlave::COMPONENT_INFO.name
            << "HTTP service to be defined in framework.cfg.  Please make sure this service is defined");
        return false;
    }

    return true;
}


void OAuthSlaveImpl::onLocalUserSessionLogout(UserSessionId userSessionId)
{
    mBackgroundJobQueue.queueFiberJob(this, &OAuthSlaveImpl::removeUserSessionTokenInfoFromCache, userSessionId, "OAuthSlaveImpl::removeUserSessionTokenInfoFromCache");
}

bool OAuthSlaveImpl::onValidateConfig(OAuthConfig& config, const OAuthConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
{
    if (config.getUseMock() &&
        gController->isPlatformHosted(xone) &&
        (blaze_stricmp(gController->getServiceEnvironment(), "dev") != 0) &&
        (blaze_stricmp(gController->getServiceEnvironment(), "test") != 0))
    {
        FixedString64 msg(FixedString64::CtorSprintf(), "[OAuth Config Failure] : The xone useMock setting is not supported for service environment %s.", gController->getServiceEnvironment());
        validationErrors.getErrorMessages().push_back().set(msg.c_str());
    }

    for (const auto& entry : config.getComponentScopes())
    {
        if (Blaze::ComponentData::getComponentDataByName(entry.first.c_str()) == nullptr)
        {
            FixedString64 msg(FixedString64::CtorSprintf(), "[OAuth Config Failure] : The component(%s) not found.", entry.first.c_str());
            validationErrors.getErrorMessages().push_back().set(msg.c_str());
        }
    }

    return validationErrors.getErrorMessages().empty();
}

BlazeRpcError OAuthSlaveImpl::getAccessToken(NucleusConnect::GetAccessTokenRequest &request, NucleusConnect::GetAccessTokenResponse &response, const char8_t* productName)
{
    if ((productName == nullptr) || (*productName == '\0'))
    {
        BLAZE_TRACE_LOG(Log::OAUTH, "[OAuthSlaveImpl].getAccessToken: productName not specified.");
        return ERR_SYSTEM;
    }

    if (*request.getClientId() == '\0')
    {
        const char8_t* clientId = gController->getBlazeServerNucleusClientId();
        if (EA_UNLIKELY(clientId == nullptr || *clientId == '\0'))
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getAccessToken: ClientId is empty.");
            return ERR_SYSTEM;
        }
        request.setClientId(clientId);
    }

    request.setReleaseType(gUserSessionManager->getProductReleaseType(productName));

    NucleusConnect::NucleusConnectSlave* comp = (NucleusConnect::NucleusConnectSlave*)gOutboundHttpService->getService(NucleusConnect::NucleusConnectSlave::COMPONENT_INFO.name);
    if (comp == nullptr)
    {
        BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getAccessToken: nucleusconnect is nullptr");
        return ERR_SYSTEM;
    }

    NucleusConnect::ErrorResponse errorResponse;
    BlazeRpcError err = comp->getAccessToken(request, response, errorResponse);
    if (err != ERR_OK)
    {
        err = OAuthUtil::parseNucleusConnectError(errorResponse, err);
        BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getAccessToken: getAccessToken failed with error(" << ErrorHelp::getErrorName(err) << ")");
        return err;
    }

    return err;
}

BlazeRpcError OAuthSlaveImpl::getXBLToken(CachedFirstPartyToken& result, const FirstPartyTokenCacheKey& cacheKey, const InetAddress& inetAddr, bool forceRefresh, const char8_t* relyingParty)
{
    const BlazeId blazeId = cacheKey.first;

    // We need to fetch the first party access token from Nucleus. We can use our's if it is the current user session or use server's.
    // OAUTH_TODO - Why not always use server's?
    eastl::string accessToken;
    TimeValue validUntil;

    const char8_t* clientId = gController->getBlazeServerNucleusClientId();

    BlazeRpcError rc = ERR_SYSTEM;
    if (gCurrentUserSession != nullptr && gCurrentUserSession->getBlazeId() == blazeId)
    {
        CachedTokenInfo cachedToken;
        rc = getAccessTokenByUserSessionId(gCurrentUserSession->getUserSessionId(), inetAddr, cachedToken, validUntil);
        accessToken.assign(cachedToken.getAccessToken());
        if (rc != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getXBLToken: getAccessTokenByUserSessionId failed to get a token.");
            return rc;
        }
    }
    else
    {
        rc = getServerAccessToken(accessToken, validUntil, clientId);
        if (rc != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getXBLToken: getServerAccessToken failed to get a token.");
            return rc;
        }
    }

    NucleusIdentity::NucleusIdentitySlave* comp = (NucleusIdentity::NucleusIdentitySlave*)gOutboundHttpService->getService(NucleusIdentity::NucleusIdentitySlave::COMPONENT_INFO.name);
    if (comp == nullptr)
    {
        BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getXBLToken: nucleusidentity is nullptr");
        return ERR_SYSTEM;
    }

    NucleusIdentity::GetXBLTokenRequest xblTokenReq;
    xblTokenReq.setPersonaId(blazeId);

    xblTokenReq.getAuthCredentials().setAccessToken(eastl::string(eastl::string::CtorSprintf(), "%s %s", "Bearer", accessToken.c_str()).c_str());
    xblTokenReq.getAuthCredentials().setClientId(clientId);
    xblTokenReq.setSandboxId(gController->getFrameworkConfigTdf().getSandboxId());
    xblTokenReq.setForceRefresh(forceRefresh ? "true" : "false");
    xblTokenReq.setUseMock(isMock() ? "true" : "false");
    if (relyingParty != nullptr && relyingParty[0] != '\0')
    {
        xblTokenReq.setRelyingParty(relyingParty);
    }

    xblTokenReq.getAuthCredentials().setIpAddress(inetAddr.getIpAsString());

    NucleusIdentity::GetXBLTokenResponse xblTokenResponse;
    NucleusIdentity::IdentityError error;

    rc = comp->getXBLToken(xblTokenReq, xblTokenResponse, error);
    if (isMock())
    {
        xblTokenResponse.getXblToken().setAuthToken(eastl::string().sprintf("%" PRIu64, blazeId).c_str());
        xblTokenResponse.getXblToken().setPersonaId(blazeId);
        xblTokenResponse.getXblToken().setExpiresIn(3600);
        // To avoid blocking tests, ignore mock nucleus errors (logged)
        rc = ERR_OK;
    }
    if (rc != ERR_OK)
    {
        rc = OAuthUtil::parseNucleusIdentityError(error, rc);
        BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getXBLToken: getXBLToken failed with error(" << ErrorHelp::getErrorName(rc) << ")");
        return rc;
    }

    addUserToFirstPartyCache(result, cacheKey, xblTokenResponse.getXblToken().getPersonaId(), xblTokenResponse.getXblToken().getAuthToken(), xblTokenResponse.getXblToken().getProofKey(),
        xblTokenResponse.getXblToken().getExpiresIn(), xone);

    return rc;
}

BlazeRpcError OAuthSlaveImpl::doGetXBLToken(const GetUserXblTokenRequest& request, GetUserXblTokenResponse& xblTokenResponse, const InetAddress& inetAddr)
{
    BlazeRpcError rc = ERR_OK;
    BlazeId personaId = request.getRetrieveUsing().getPersonaId();

    bool hasPermission = (gCurrentUserSession != nullptr && gCurrentUserSession->getBlazeId() == personaId)
                         || UserSession::hasSuperUserPrivilege();
    if (!hasPermission)
    {
        BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].doGetXBLToken: looking up XBL token of user: " << personaId << " in current context: " <<
            Fiber::getCurrentContext() << " by user: " << (gCurrentUserSession != nullptr ? gCurrentUserSession->getBlazeId() : INVALID_BLAZE_ID) << " without superuser privilege is not allowed.");
        return ERR_SYSTEM;
    }

    CachedFirstPartyToken cachedFirstPartyToken;
    FirstPartyTokenCacheKey cacheKey(personaId, request.getRelyingParty());
    RedisError error = mBlazeIdToCachedFirstPartyTokens.find(cacheKey, cachedFirstPartyToken);

    bool fetchToken = (error != REDIS_ERR_OK
                    || cachedFirstPartyToken.getPersonaId() == INVALID_BLAZE_ID
                    || request.getForceRefresh());

    if (!fetchToken)
    {
        // Other pre-conditions don't require us to fetch the token. Let's check if the token we have in cache has enough validity remaining. If not, fetch a new one.
        TimeValue tokenTTL;
        RedisError redisError = mBlazeIdToCachedFirstPartyTokens.getTimeToLive(cacheKey, tokenTTL);
        if (redisError == REDIS_ERR_OK)
            fetchToken = (tokenTTL < getConfig().getMinValidDurationForUserSessionAccessToken());
        else
        {
            BLAZE_WARN_LOG(Log::OAUTH, "[OAuthSlaveImpl].doGetXBLToken: Redis Error when checking the token expiry for personaId(" << personaId <<
                "). RedisError(" << RedisErrorHelper::getName(redisError) << "). Erasing current entry and fetching new token.");
            mBlazeIdToCachedFirstPartyTokens.erase(cacheKey);
            fetchToken = true;
        }
    }

    if (fetchToken)
    {
        rc = getXBLToken(cachedFirstPartyToken, cacheKey, inetAddr, request.getForceRefresh(), request.getRelyingParty());
        if (rc != ERR_OK)
        {
            mUserFirstPartyTokenRetrievalFails.increment();
            if (rc == OAUTH_ERR_INVALID_REQUEST)
            {
                //OAUTH_ERR_INVALID_REQUEST indicates blaze request was rejected by Nucleus with 4xx http error, have to logout user
                forceUserLogout(gUserSessionManager->getUserSessionIdByBlazeId(personaId), "User XBL token retrieval request was rejected by Nucleus with 4xx http error");
            }
        }
    }

    if (rc == ERR_OK)
    {
        BLAZE_ASSERT_COND_LOG(Log::OAUTH, (cachedFirstPartyToken.getPlatform() == xone), "[OAuthSlaveImpl].doGetXBLToken: user(" << personaId << ") had invalid cached token that was NOT of type xone, actual type(" << ClientPlatformTypeToString(cachedFirstPartyToken.getPlatform()) << ").");
        xblTokenResponse.setXblToken(cachedFirstPartyToken.getAuthToken());
        xblTokenResponse.setProofKey(cachedFirstPartyToken.getProofKey());
    }
    return rc;
}

BlazeRpcError OAuthSlaveImpl::getPSNToken(CachedFirstPartyToken& result, const FirstPartyTokenCacheKey& cacheKey, const InetAddress& inetAddr, bool forceRefresh, ClientPlatformType platform, bool crossgen)
{
    eastl::string accessToken;
    TimeValue validUntil;
    const BlazeId blazeId = cacheKey.first;
    BlazeRpcError rc = ERR_SYSTEM;

    const char8_t* clientId = gController->getBlazeServerNucleusClientId();

    if (gCurrentUserSession != nullptr && gCurrentUserSession->getBlazeId() == blazeId)
    {
        CachedTokenInfo cachedToken;
        rc = getAccessTokenByUserSessionId(gCurrentUserSession->getUserSessionId(), inetAddr, cachedToken, validUntil);
        accessToken.assign(cachedToken.getAccessToken());
        if (rc != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getPSNToken: getAccessTokenByUserSessionId failed to get a token.");
            return rc;
        }
    }
    else
    {
        rc = getServerAccessToken(accessToken, validUntil, clientId);
        if (rc != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getPSNToken: getServerAccessToken failed to get a token.");
            return rc;
        }
    }

    NucleusIdentity::NucleusIdentitySlave* comp = (NucleusIdentity::NucleusIdentitySlave*)gOutboundHttpService->getService(NucleusIdentity::NucleusIdentitySlave::COMPONENT_INFO.name);
    if (comp == nullptr)
    {
        BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getPSNToken: nucleusidentity is nullptr");
        return ERR_SYSTEM;
    }

    NucleusIdentity::GetPSNTokenRequest tokenReq;
    NucleusIdentity::IdentityError error;
    tokenReq.setPersonaId(blazeId);
    tokenReq.getAuthCredentials().setAccessToken(eastl::string(eastl::string::CtorSprintf(), "%s %s", "Bearer", accessToken.c_str()).c_str());
    tokenReq.getAuthCredentials().setClientId(clientId);
    tokenReq.setForceRefresh(forceRefresh ? "true" : "false");
    if (blaze_stricmp(gController->getServiceEnvironment(), "cert") == 0)
    {
        // Nucleus: only specify this param for Sony prod-qa (cert), to distinguish Nucleus INT tokens from regular sp-int
        tokenReq.setEnvironmentQueryParam("&environment=PROD_QA");
    }

    tokenReq.getAuthCredentials().setIpAddress(inetAddr.getIpAsString());


    int64_t expiresIn = 0;
    PersonaId personaId = 0;
    eastl::string psnAccessToken;
    if (!isMock())
    {
        if ((platform == ps5) || ((platform == ps4) && crossgen))
        {
            NucleusIdentity::GetPS5OnlineTokenResponse psnToken;
            rc = comp->getPS5OnlineToken(tokenReq, psnToken, error);
            expiresIn = psnToken.getPs5AccessToken().getExpiresIn();
            personaId = psnToken.getPs5AccessToken().getPersonaId();
            psnAccessToken = psnToken.getPs5AccessToken().getAccessToken();
        }
        else if (platform == ps4)
        {
            NucleusIdentity::GetPSNTokenResponse psnToken;
            rc = comp->getPSNToken(tokenReq, psnToken, error);
            expiresIn = psnToken.getPsnAccessToken().getExpiresIn();
            personaId = psnToken.getPsnAccessToken().getPersonaId();
            psnAccessToken = psnToken.getPsnAccessToken().getPsnAccessToken();
        }
        else
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getPSNToken: unhandled platform(" << ClientPlatformTypeToString(platform) << "). Unable to fetch PSN token.");
            return ERR_SYSTEM;
        }
    }
    else
    {
        // Nucleus doesn't mock PSN.
        expiresIn = 3600;
        personaId = blazeId;
        psnAccessToken.sprintf("%" PRIu64, blazeId);
        rc = ERR_OK;
    }

    if (rc != ERR_OK)
    {
        rc = OAuthUtil::parseNucleusIdentityError(error, rc);
        BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].getPSNToken: getPSNToken failed with error(" << ErrorHelp::getErrorName(rc) << ")");
        return rc;
    }

    addUserToFirstPartyCache(result, cacheKey, personaId, psnAccessToken.c_str(), "", expiresIn, platform);

    return rc;
}

BlazeRpcError OAuthSlaveImpl::doGetPSNToken(const GetUserPsnTokenRequest& request, GetUserPsnTokenResponse& psnToken, const InetAddress& inetAddr)
{
    BlazeRpcError rc = ERR_OK;
    BlazeId personaId = request.getRetrieveUsing().getPersonaId();

    // if request is made by a logged in user for someone else's access token, bail out if he does not have superuser's privilege
    if ((gCurrentUserSession != nullptr) && (personaId != gCurrentUserSession->getBlazeId()) && !UserSession::hasSuperUserPrivilege())
    {
        BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].doGetPSNToken: looking up PSN token of user: " << personaId << " in current context: " <<
            Fiber::getCurrentContext() << " by user: " << gCurrentUserSession->getBlazeId() << " without superuser privilege is not allowed.");
        return ERR_SYSTEM;
    }


    CachedFirstPartyToken cachedFirstPartyToken;
    // distinguish between ps4 and ps5 for cache redis key. Distinguish crossgen and non in case e.g. GM uses crossgen while custom code won't
    FirstPartyTokenCacheKey cacheKey(personaId, ClientPlatformTypeToString(request.getPlatform()));
    if (request.getCrossgen() && (request.getPlatform() == ps4))
        cacheKey.second.append_sprintf("xgen");
    RedisError error = mBlazeIdToCachedFirstPartyTokens.find(cacheKey, cachedFirstPartyToken);

    bool fetchToken = (error != REDIS_ERR_OK
        || cachedFirstPartyToken.getPersonaId() == INVALID_BLAZE_ID
        || request.getForceRefresh());

    if (!fetchToken)
    {
        TimeValue tokenTTL;
        RedisError redisError = mBlazeIdToCachedFirstPartyTokens.getTimeToLive(cacheKey, tokenTTL);
        if (redisError == REDIS_ERR_OK)
            fetchToken = (tokenTTL < getConfig().getMinValidDurationForUserSessionAccessToken());
        else
        {
            BLAZE_WARN_LOG(Log::OAUTH, "[OAuthSlaveImpl].doGetPSNToken: Redis Error when checking the token expiry for personaId(" << personaId <<
                "). RedisError(" << RedisErrorHelper::getName(redisError) << "). Erasing current entry and fetching new token.");
            mBlazeIdToCachedFirstPartyTokens.erase(cacheKey);
            fetchToken = true;
        }
    }

    if (fetchToken)
    {
        rc = getPSNToken(cachedFirstPartyToken, cacheKey, inetAddr, request.getForceRefresh(), request.getPlatform(), request.getCrossgen());
        if (rc != ERR_OK)
        {
            mUserFirstPartyTokenRetrievalFails.increment();
            if (rc == OAUTH_ERR_INVALID_REQUEST)
            {
                //OAUTH_ERR_INVALID_REQUEST indicates blaze request was rejected by Nucleus with 4xx http error, have to logout user
                forceUserLogout(gUserSessionManager->getUserSessionIdByBlazeId(personaId), "User PSN token retrieval request was rejected by Nucleus with 4xx http error");
            }
        }
    }

    if (rc == ERR_OK)
    {
        BLAZE_ASSERT_COND_LOG(Log::OAUTH, (cachedFirstPartyToken.getPlatform() == request.getPlatform()), "[OAuthSlaveImpl].doGetPSNToken: user(" << personaId << ") had invalid cached token that was NOT of type(" << ClientPlatformTypeToString(request.getPlatform()) << "), actual type(" << ClientPlatformTypeToString(cachedFirstPartyToken.getPlatform()) << ").");
        psnToken.setPsnToken(cachedFirstPartyToken.getAuthToken());
        psnToken.setExpiresAt(cachedFirstPartyToken.getTokenExpires());
    }
    return rc;
}

/*! ************************************************************************************************/
/*! \param[out] cached1stPartyToken The token to update and upsert to the cache.
***************************************************************************************************/
void OAuthSlaveImpl::addUserToFirstPartyCache(CachedFirstPartyToken& cachedFirstPartyToken, const FirstPartyTokenCacheKey& cacheKey, PersonaId personaId,
    const char8_t* authToken, const char8_t* proofKey, int64_t expiresInS, ClientPlatformType platform)
{
    TimeValue tokenTTL = expiresInS * 1000 * 1000;

    cachedFirstPartyToken.setPlatform(platform);
    cachedFirstPartyToken.setPersonaId(personaId);
    cachedFirstPartyToken.setTokenExpires(TimeValue::getTimeOfDay() + tokenTTL);
    cachedFirstPartyToken.setAuthToken(authToken);
    cachedFirstPartyToken.setProofKey(proofKey);

    RedisError error = mBlazeIdToCachedFirstPartyTokens.upsertWithTtl(cacheKey, cachedFirstPartyToken, tokenTTL);
    if (error != REDIS_ERR_OK)
    {
        BLAZE_WARN_LOG(Log::OAUTH, "[OAuthSlaveImpl].addUserToFirstPartyCache: failed to add token to cache in redis for BlazeId(" << cacheKey.first << "), err=" << RedisErrorHelper::getName(error));
    }
}

BlazeRpcError OAuthSlaveImpl::updateUserSessionCachedTokenIfNeeded(UserSessionId userSessionId, CachedTokenInfo &cachedToken, const InetAddress& inetAddr, TimeValue minValidDuration)
{
    BlazeRpcError err = ERR_OK;

    if (TimeValue::getTimeOfDay() + minValidDuration > (cachedToken.getAccessTokenExpire())
        || blaze_stricmp(gController->getBlazeServerNucleusClientId(), cachedToken.getClientId()) != 0) // Check if the clientId has been reconfigured and no longer matches cached token.
    {
        err = updateUserSessionCachedToken(userSessionId, cachedToken, inetAddr, minValidDuration);
    }
    return err;
}

void OAuthSlaveImpl::onUpdateUserSessionCachedTokensTimer()
{
    if (gUserSessionMaster == nullptr)
        return;

    BLAZE_TRACE_LOG(Log::OAUTH, "[OAuthSlaveImpl].onUpdateUserSessionCachedTokensTimer: Refreshing tokens if the refresh token is about to expire in our configured duration");

    TimeValue currentTime = TimeValue::getTimeOfDay();
    mUpdateUserSessionTokensTimerId = gSelector->scheduleFiberTimerCall(currentTime + getConfig().getMinValidDurationForUserSessionRefreshToken(), this, &OAuthSlaveImpl::onUpdateUserSessionCachedTokensTimer, "onUpdateUserSessionTokensTimer");

    // Grab the current userIds on this instance in one shot so we avoid making blocking calls while iterating over this map
    UserSessionIdList userIds;
    userIds.reserve(gUserSessionMaster->getOwnedUserSessionsMap().size());
    for (auto& entry : gUserSessionMaster->getOwnedUserSessionsMap())
        userIds.push_back(entry.first);


    BlazeRpcError err = ERR_OK;
    for (auto& userSessionId : userIds)
    {
        CachedTokenInfo cachedUserToken;
        RedisError redisErr = mTokenInfoByUserSessionIdCache.find(userSessionId, cachedUserToken);

        // We add a grace period in our check because we don't want to lose refresh token due to minor timing differences that can arise due to network latencies between
        // Blaze and Nucleus, Blaze server being late to schedule the timer for any number of reasons etc. So we end up refreshing any token that are set to expire in our
        // timer fire interval plus grace period.

        if (redisErr == REDIS_ERR_OK &&
            (currentTime + getConfig().getMinValidDurationForUserSessionRefreshToken() + TimeValue(USER_REFRESH_TOKEN_CHECK_GRACE_PERIOD) > cachedUserToken.getRefreshTokenExpire()))
        {
            err = updateUserSessionCachedToken(userSessionId, cachedUserToken, cachedUserToken.getIpAddr(), getConfig().getMinValidDurationForUserSessionAccessToken());
            if (err != ERR_OK)
            {
                BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].onUpdateUserSessionCachedTokensTimer: Failed to update access and refresh tokens for user " << userSessionId);
            }
        }
    }
}

void OAuthSlaveImpl::onUpdateServerCachedTokensTimer()
{
    BLAZE_TRACE_LOG(Log::OAUTH, "[OAuthSlaveImpl].onUpdateServerCachedTokensTimer: Refreshing any server tokens that'll expire soon.");

    TimeValue currentTime = TimeValue::getTimeOfDay();
    mUpdateServerTokensTimerId = gSelector->scheduleFiberTimerCall(currentTime + getConfig().getMinValidDurationForServerAccessToken(), this,
        &OAuthSlaveImpl::onUpdateServerCachedTokensTimer, "onUpdateServerCachedTokensTimer");

    // Get rid of the clientId's that are no longer configured
    for (ServerTokensByClientIdMap::iterator it = mServerTokens.begin(), itEnd = mServerTokens.end(); it != itEnd;)
    {
        if (!gController->isNucleusClientIdInUse(it->first.c_str()))
        {
            it = mServerTokens.erase(it);
        }
        else
        {
            ++it;
        }
    }

    for (ServerTokensByClientIdMap::iterator it = mServerTokens.begin(), itEnd = mServerTokens.end(); it != itEnd; ++it)
    {
        // 1. Update cached server token if they have been requested already (*ScopeToken->getAccessToken is non-null).
        // 2. If we run into any error (say could not connect to Nucleus), we don't erase our existing token in this method as currently cached token is still good.
        // The background refresh here is just a latency optimization.
        const char8_t* clientId = it->first.c_str();
        {
            CachedTokenInfo* fullScopeToken = getServerCachedToken(TOKENTYPE_SERVER_FULL_SCOPE, clientId);
            if (fullScopeToken != nullptr && fullScopeToken->getAccessToken()[0] != '\0'
                && currentTime + getConfig().getMinValidDurationForServerAccessToken() > fullScopeToken->getAccessTokenExpire())
            {
                BlazeRpcError err = updateServerCachedToken(*fullScopeToken);
                if (err != ERR_OK)
                {
                    BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].onUpdateServerCachedTokensTimer: Failed to update full scope access and refresh tokens for clientId " << clientId << ". Will try again after configured interval.");
                }
            }
        }

        {
            CachedTokenInfo* restrictedScopeToken = getServerCachedToken(TOKENTYPE_SERVER_RESTRICTED_SCOPE, clientId);
            if (restrictedScopeToken != nullptr && restrictedScopeToken->getAccessToken()[0] != '\0'
                && currentTime + getConfig().getMinValidDurationForServerAccessToken() > restrictedScopeToken->getAccessTokenExpire())
            {
                BlazeRpcError err = updateServerCachedToken(*restrictedScopeToken);
                if (err != ERR_OK)
                {
                    BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].onUpdateServerCachedTokensTimer: Failed to update restricted scope access and refresh tokens for clientId " << clientId << ". Will try again after configured interval.");
                }
            }
        }
    }
}

BlazeRpcError OAuthSlaveImpl::updateUserSessionCachedToken(UserSessionId userSessionId, CachedTokenInfo& cachedToken, const InetAddress& inetAddr, TimeValue minValidDuration /* = TimeValue */)
{
    ProductName productName;
    gUserSessionManager->getProductName(userSessionId, productName);

    NucleusConnect::GetAccessTokenResponse accessTokenResponse;
    BlazeRpcError err = getAccessTokenByRefreshToken(cachedToken.getRefreshToken(), accessTokenResponse, productName.c_str(), inetAddr, true);
    if (err != ERR_OK)
    {
        mUserAccessTokenRefreshFails.increment();
        BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].updateUserSessionCachedToken: failed to get a new access token while using a refresh token for UserSessionId(" << userSessionId << ").");
        if (err == OAUTH_ERR_INVALID_REQUEST)
        {
            //OAUTH_ERR_INVALID_REQUEST indicates blaze request was rejected by Nucleus with 4xx http error, have to logout user
            forceUserLogout(userSessionId, "User access token refresh request was rejected by Nucleus with 4xx http error");
        }
        return err;
    }

    // fetch the cached token from cache again since it could have been removed when we were off making a blocking call to Nucleus
    RedisError redisErr = mTokenInfoByUserSessionIdCache.find(userSessionId, cachedToken);

    if (redisErr == REDIS_ERR_OK)
    {
        populateCachedTokenInfo(cachedToken, accessTokenResponse, gController->getBlazeServerNucleusClientId(), productName, cachedToken.getIpAddr());

        if (TimeValue::getTimeOfDay() + minValidDuration > cachedToken.getAccessTokenExpire())
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].updateUserSessionCachedToken: requested minValidDuration(" << minValidDuration.getSec() << "s) exceeds max lifetime of access token returned by Nucleus.");
            return ERR_SYSTEM;
        }

        redisErr = mTokenInfoByUserSessionIdCache.upsert(userSessionId, cachedToken);
        if (redisErr == REDIS_ERR_OK)
        {
            redisErr = mTokenInfoByUserSessionIdCache.pexpire(userSessionId, (int64_t)accessTokenResponse.getRefreshExpiresIn() * 1000 * 1000);
            if (redisErr != REDIS_ERR_OK)
            {
                BLAZE_WARN_LOG(Log::OAUTH, "[OAuthSlaveImpl].updateUserSessionCachedToken: Failed to add expiry in Redis to cached access token for usersession id " << userSessionId);
            }
        }
        else
        {
            BLAZE_WARN_LOG(Log::OAUTH, "[OAuthSlaveImpl].updateUserSessionCachedToken: Failed to cache access token in Redis for usersession id " << userSessionId);
        }
    }
    else
    {
        // cached entry was removed from cache, which means user logged out
        err = ERR_SYSTEM;
    }

    return err;
}

BlazeRpcError OAuthSlaveImpl::updateServerCachedToken(CachedTokenInfo& cachedToken)
{
    NucleusConnect::GetAccessTokenResponse accessTokenResponse;

    BlazeRpcError err = getAccessTokenByRefreshToken(cachedToken.getRefreshToken(), accessTokenResponse, cachedToken.getProductName(), NameResolver::getInternalAddress(), false);
    if (err != ERR_OK)
    {
        mServerAccessTokenRefreshFails.increment();
        BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].updateServerCachedToken: failed to get a new server access token while using a refresh token.");
        return err;
    }

    populateCachedTokenInfo(cachedToken, accessTokenResponse, cachedToken.getClientId(), cachedToken.getProductName(), NameResolver::getInternalAddress().getIpAsString());
    return err;
}

CachedTokenInfo* OAuthSlaveImpl::getServerCachedToken(AccessTokenType tokenType, const char8_t* clientId)
{
    ServerTokensByClientIdMap::iterator iter = mServerTokens.find(clientId);
    if (iter != mServerTokens.end())
    {
        if (tokenType == TOKENTYPE_SERVER_FULL_SCOPE)
            return &iter->second.first;
        else
            return &iter->second.second;
    }

    return nullptr;
}


void OAuthSlaveImpl::populateCachedTokenInfo(CachedTokenInfo& cachedTokenInfo, const NucleusConnect::GetAccessTokenResponse& nucleusResponse, const char8_t* clientId, const char8_t* productName, const char8_t* ipAddr)
{
    TimeValue currentTime = TimeValue::getTimeOfDay();

    cachedTokenInfo.setAccessTokenExpire(currentTime + ((int64_t)nucleusResponse.getExpiresIn() * 1000 * 1000));
    cachedTokenInfo.setRefreshTokenExpire(currentTime + ((int64_t)nucleusResponse.getRefreshExpiresIn() * 1000 * 1000));
    cachedTokenInfo.setAccessToken(nucleusResponse.getAccessToken());
    cachedTokenInfo.setTokenType(nucleusResponse.getTokenType());
    cachedTokenInfo.setRefreshToken(nucleusResponse.getRefreshToken());

    cachedTokenInfo.setClientId(clientId);
    cachedTokenInfo.setProductName(productName);
    cachedTokenInfo.setIpAddr(ipAddr);
}


bool OAuthSlaveImpl::cacheTrustedToken(const char8_t* accessToken, NucleusConnect::GetTokenInfoResponse& response)
{
    CachedTrustedToken trustedToken;
    trustedToken.setAccessToken(accessToken);
    trustedToken.setScopes(response.getScope());

    TimeValue tokenTTL = response.getExpiresIn() * 1000 * 1000;
    // upsert instead of insert in case another trusted client is using the same token on another instance concurrently
    RedisError err = mAccessTokenToCachedTrustedTokens.upsertWithTtl(accessToken, trustedToken, tokenTTL);
    if (err != REDIS_ERR_OK)
    {
        BLAZE_WARN_LOG(Log::OAUTH, "[OAuthSlaveImpl].cacheTrustedToken: Failed to cache trusted token in Redis(" << RedisErrorHelper::getName(err) << ")");
        return false;
    }

    return true;
}

bool OAuthSlaveImpl::getCachedTrustedToken(const char8_t* accessToken, CachedTrustedToken& trustedToken)
{
    RedisError err = mAccessTokenToCachedTrustedTokens.find(accessToken, trustedToken);
    return (err == REDIS_ERR_OK);
}

void OAuthSlaveImpl::getStatusInfo(ComponentStatus& status) const
{
    ComponentStatus::InfoMap& map = status.getInfo();
    char8_t buf[32];

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mServerAccessTokenRefreshFails.getTotal());
    map["TotalServerAccessTokenRefreshFails"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mUserAccessTokenRefreshFails.getTotal());
    map["TotalUserAccessTokenRefreshFails"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mUserFirstPartyTokenRetrievalFails.getTotal());
    map["TotalUserFirstPartyTokenRetrievalFails"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mUserTokenFailForcedLogouts.getTotal());
    map["TotalUserTokenFailForcedLogouts"] = buf;
}

eastl::string OAuthSlaveImpl::getPsnServerAccessToken(const UserIdentification* user)
{
    // just in case mock psn service not able to return proper tokens, return dummy mock value
    if (mPsnServerAccessTokenInfo.accessToken.empty() && isMock())
    {
        // Matches use client creds, not user tokens, so other than for logging/debug use, the mock PS5 service doesn't *need* a user's id
        // set as token to know who call is for. Whatever dummy value must be non-0 tho for mock server side validations to pass:
        return eastl::string(eastl::string::CtorSprintf(), "%" PRIu64, (user ? user->getPlatformInfo().getExternalIds().getPsnAccountId() : 123));
    }

    return mPsnServerAccessTokenInfo.accessToken;
}

BlazeError OAuthSlaveImpl::requestNewPsnServerAccessTokenAndWait()
{
    if (mPsnServerAccessTokenRefreshTimerId != INVALID_TIMER_ID)
    {
        // The only time mPsnServerAccessTokenRefreshTimerId is not set is when renewPsnServerAccessToken() is in-progress
        gSelector->updateTimer(mPsnServerAccessTokenRefreshTimerId, TimeValue::getTimeOfDay());
    }

    return mPsnServerAccessTokenWaitList.wait("OAuthSlaveImpl::requestNewPsnServerAccessTokenAndWait");
}

void OAuthSlaveImpl::renewPsnServerAccessToken()
{
    gSelector->cancelTimer(mPsnServerAccessTokenRefreshTimerId);
    mPsnServerAccessTokenRefreshTimerId = INVALID_TIMER_ID;

    BlazeError err = ERR_SYSTEM;
    int64_t secondsUntilNextRefresh = 0;

    eastl::string accessToken; //Nucleus access
    TimeValue validUntil;
    if (!gController->isPlatformHosted(Blaze::ps5))
    {
        BLAZE_SPAM_LOG(Log::OAUTH, "[OAuthSlaveImpl].renewPsnServerAccessToken: Skipping retrieval of server access token because PS5 is not hosted by this instance");
    }
    else if (getConfig().getPsnClientCredential().getIdentityScopeAsTdfString().empty())
    {
        BLAZE_WARN_LOG(Log::OAUTH, "[OAuthSlaveImpl].renewPsnServerAccessToken: No value configured for the externalSessionClientCredentialScope setting. Can't get token!");
        return; //empty identityScope maybe intentionally used to disable this feature
    }
    else if (ERR_OK != getServerAccessToken(accessToken, validUntil, gController->getBlazeServerNucleusClientId(), getConfig().getPsnClientCredential().getIdentityScope())) //any clientId ok (unused)
    {
        BLAZE_WARN_LOG(Log::OAUTH, "[OAuthSlaveImpl].renewPsnServerAccessToken: getServerAccessToken failed. Can't get PSN token!"); //details logged
    }
    else
    {
        NucleusIdentity::IdentityError rspErr;
        NucleusIdentity::GetPS5ClientCredentialTokenResponse rsp;
        NucleusIdentity::GetPSNTokenRequest req;
        RawBufferPtr rspRaw = BLAZE_NEW RawBuffer(0);
        req.getAuthCredentials().setAccessToken(eastl::string(eastl::string::CtorSprintf(), "%s %s", "Bearer", accessToken.c_str()).c_str());
        if (!getConfig().getPsnClientCredential().getIdentityEnvironmentAsTdfString().empty())
        {
            req.setEnvironmentQueryParam(eastl::string(eastl::string::CtorSprintf(), "?environment=%s", getConfig().getPsnClientCredential().getIdentityEnvironment()).c_str());
        }

        auto* comp = (NucleusIdentity::NucleusIdentitySlave*)gOutboundHttpService->getService(NucleusIdentity::NucleusIdentitySlave::COMPONENT_INFO.name);
        if (comp != nullptr)
        {
            err = comp->getPS5ClientCredentialToken(req, rsp, rspErr, Blaze::RpcCallOptions(), rspRaw.get());
            if (err == ERR_OK)
            {
                mPsnServerAccessTokenInfo.accessToken.sprintf("Bearer %s", rsp.getPs5AccessToken().getAccessToken());
                mPsnServerAccessTokenInfo.expiresAt = TimeValue::getTimeOfDay() + (rsp.getPs5AccessToken().getExpiresIn() * 1000 * 1000);

                // refresh five minutes before the token expires
                secondsUntilNextRefresh = rsp.getPs5AccessToken().getExpiresIn() - PSN_S2S_ACCESS_TOKEN_REFRESH_PERIOD_SEC;
                BLAZE_TRACE_LOG(Log::OAUTH, "[OAuthSlaveImpl].renewPsnServerAccessToken: got PSN token. Expires in (" << rsp.getPs5AccessToken().getExpiresIn() << ")s. Refresh in (" << secondsUntilNextRefresh << ")s.");
            }
            else
            {
                BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].renewPsnServerAccessToken: get PSN token call failed, error: " << err << " (" << ErrorHelp::getErrorDescription(err) << "). Raw response: " << eastl::string(reinterpret_cast<const char8_t*>(rspRaw->data()), rspRaw->datasize()).c_str());
            }
        }
        else
        {
            err = ERR_SYSTEM;
            BLAZE_ERR_LOG(Log::OAUTH, "[OAuthSlaveImpl].renewPsnServerAccessToken: NucleusIdentitySlave is nullptr at line:" << __LINE__);
        }
    }

    // We don't want to slam the PSN servers with requests, especially if it just returned an error.
    // But, we want to get access to their services ASAP. So we wait at least 5 seconds (which was arbitrarily chosen),
    // plus a small randomly chosen additional few second in order to spread out the requests we send to Sony.
    secondsUntilNextRefresh = eastl::max<int64_t>(secondsUntilNextRefresh, 5 + gRandom->getRandomNumber(5));
    mPsnServerAccessTokenRefreshTimerId = gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay() + (secondsUntilNextRefresh * 1000 * 1000),
        this, &OAuthSlaveImpl::renewPsnServerAccessToken, "OAuthSlaveImpl::renewPsnServerAccessToken");

    if (err == ERR_OK)
        mPsnServerAccessTokenWaitList.signal(ERR_OK);
}

} // OAuth
} // Blaze
