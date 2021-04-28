/*************************************************************************************************/
/*!
    \file   oauthslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_OAUTH_OAUTHSLAVEIMPL_H
#define BLAZE_OAUTH_OAUTHSLAVEIMPL_H

#include "framework/rpc/oauthslave_stub.h"
#include "framework/tdf/oauth_server.h"
#include "framework/oauth/jwtutil.h"
#include "framework/oauth/nucleus/error_codes.h"
#include "framework/redis/rediskeymap.h"
#include "nucleusconnect/tdf/nucleusconnect.h"

namespace Blaze
{

class InetAddress;

namespace OAuth
{

enum AccessTokenType
{
    TOKENTYPE_SERVER_FULL_SCOPE = 0,
    TOKENTYPE_SERVER_RESTRICTED_SCOPE // ReadOnly scopes
};

class OAuthSlaveImpl : public OAuthSlaveStub
{
    NON_COPYABLE(OAuthSlaveImpl);
public:

    OAuthSlaveImpl();
    ~OAuthSlaveImpl() override;

    static const Blaze::Nucleus::ErrorCodes* getErrorCodes() { return &mErrorCodes; }

    //+ Rpc implementations of this component
    BlazeRpcError DEFINE_ASYNC_RET(getServerAccessToken(const GetServerAccessTokenRequest& request, GetServerAccessTokenResponse& response));
    BlazeRpcError DEFINE_ASYNC_RET(getUserAccessToken(const GetUserAccessTokenRequest& request, GetUserAccessTokenResponse& response, const InetAddress& inetAddr));
    BlazeRpcError DEFINE_ASYNC_RET(getUserXblToken(const GetUserXblTokenRequest& request, GetUserXblTokenResponse& xblToken, const InetAddress& inetAddr));
    BlazeRpcError DEFINE_ASYNC_RET(getUserPsnToken(const GetUserPsnTokenRequest& request, GetUserPsnTokenResponse& ps4Token, const InetAddress& inetAddr));
    BlazeRpcError DEFINE_ASYNC_RET(getIdentityContext(const IdentityContextRequest& request, IdentityContext& response));
    //-

    //+ Nucleus Call helpers
    BlazeRpcError DEFINE_ASYNC_RET(getAccessTokenByAuthCode(const char8_t *authorizationCode, NucleusConnect::GetAccessTokenResponse &response, const char8_t* productName, const InetAddress& inetAddr));
    BlazeRpcError DEFINE_ASYNC_RET(getAccessTokenByRefreshToken(const char8_t *refreshToken, NucleusConnect::GetAccessTokenResponse &response, const char8_t* productName, const InetAddress& inetAddr, bool isUserToken));
    BlazeRpcError DEFINE_ASYNC_RET(getAccessTokenByPassword(const char8_t *email, const char8_t *password, NucleusConnect::GetAccessTokenResponse &response, const char8_t* productName, const InetAddress& inetAddr));
    BlazeRpcError DEFINE_ASYNC_RET(getTokenInfo(const NucleusConnect::GetTokenInfoRequest& request, NucleusConnect::GetTokenInfoResponse& response));
    BlazeRpcError DEFINE_ASYNC_RET(getAuthCodeForPersona(const char8_t* personaName, const char8_t* personaNamespace, const char8_t* accessToken, const char8_t* productName, NucleusConnect::GetAuthCodeResponse& resp));
    BlazeRpcError DEFINE_ASYNC_RET(getJwtPublicKeyInfo(NucleusConnect::GetJwtPublicKeyInfoResponse& response));

    //+ UserSession Token cache management
    void addUserSessionTokenInfoToCache(const NucleusConnect::GetAccessTokenResponse& accessTokenResponse, const char8_t* productName, const char8_t* ipAddr);
    void removeUserSessionTokenInfoFromCache(UserSessionId userSessionId);
    void onLocalUserSessionLogout(UserSessionId userSessionId); // Called by the authentication component
    //-

    //+ TrustedLogin token management
    bool cacheTrustedToken(const char8_t* accessToken, NucleusConnect::GetTokenInfoResponse& response);
    bool getCachedTrustedToken(const char8_t* accessToken, CachedTrustedToken& trustedToken);
    //-

    void getStatusInfo(ComponentStatus& status) const override;

    //+ PSN s2s server access token management
    eastl::string getPsnServerAccessToken(const UserIdentification* user);
    BlazeError requestNewPsnServerAccessTokenAndWait();
    void renewPsnServerAccessToken();
    //-

    bool isMock() const;

    [[deprecated]] BlazeRpcError DEFINE_ASYNC_RET(getAccessTokenByExchange(const char8_t *accessToken, NucleusConnect::GetAccessTokenResponse &response, const char8_t* productName, const InetAddress& inetAddr));

private:

    bool onConfigure() override;
    bool onReconfigure() override;
    bool onResolve() override;
    void onShutdown() override;
    bool onActivate() override;
    bool onValidateConfig(OAuthConfig& config, const OAuthConfig* referenceConfigPtr, Blaze::ConfigureValidationErrors& validationErrors) const override;

    bool configure(bool isReconfigure);

    // The key's string is used when the user needs different tokens for different APIs/contexts, for instance the XBL token relyingParty:
    typedef eastl::pair<BlazeId, eastl::string> FirstPartyTokenCacheKey;

    BlazeRpcError DEFINE_ASYNC_RET(getServerAccessToken(eastl::string& accessToken, TimeValue& validUntil, const char8_t* clientId, const char8_t* allowedScope = nullptr));


    BlazeRpcError DEFINE_ASYNC_RET(getAccessTokenByUserSessionId(UserSessionId userSessionId, const InetAddress& inetAddr, CachedTokenInfo& cachedUserToken, TimeValue& validUntil)); // Get access token if the user is currently logged in. Error otherwise.
    BlazeRpcError DEFINE_ASYNC_RET(getXBLToken(CachedFirstPartyToken& result, const FirstPartyTokenCacheKey& cacheKey, const InetAddress& inetAddr, bool forceRefresh, const char8_t* relyingParty));
    BlazeRpcError DEFINE_ASYNC_RET(getPSNToken(CachedFirstPartyToken& result, const FirstPartyTokenCacheKey& cacheKey, const InetAddress& inetAddr, bool forceRefresh, ClientPlatformType platform, bool crossgen));
    BlazeRpcError DEFINE_ASYNC_RET(doGetXBLToken(const GetUserXblTokenRequest& request, GetUserXblTokenResponse& xblToken, const InetAddress& inetAddr));
    BlazeRpcError DEFINE_ASYNC_RET(doGetPSNToken(const GetUserPsnTokenRequest& request, GetUserPsnTokenResponse& psnToken, const InetAddress& inetAddr));
    void addUserToFirstPartyCache(CachedFirstPartyToken& cachedFirstPartyToken, const FirstPartyTokenCacheKey& cacheKey, PersonaId personaId, const char8_t* authToken, const char8_t* proofKey, int64_t expiresInS, ClientPlatformType platform);


    BlazeRpcError DEFINE_ASYNC_RET(updateUserSessionCachedTokenIfNeeded(UserSessionId userSessionId, CachedTokenInfo& cachedToken, const InetAddress& inetAddr, TimeValue minValidDuration));
    BlazeRpcError DEFINE_ASYNC_RET(updateUserSessionCachedToken(UserSessionId userSessionId, CachedTokenInfo& cachedToken, const InetAddress& inetAddr, TimeValue minValidDuration));
    BlazeRpcError DEFINE_ASYNC_RET(updateServerCachedToken(CachedTokenInfo& cachedToken));

    CachedTokenInfo* getServerCachedToken(AccessTokenType tokenType, const char8_t* clientId);

    // Access Token Life Time Management:
    // User
    // Access Token - When the access token is requested, it is automatically updated if the remaining valid duration of the access token falls below the configured duration
    // (OAuthConfig::mMinValidDurationForUserSessionAccessToken).The access token is not updated in the background to avoid unnecessary server load on Nucleus and first party.
    // This does mean slightly increased latency when the cached token is set to expire in OAuthConfig::mMinValidDurationForUserSessionAccessToken duration.

    // Refresh Token - The refresh token (and access token as a side effect) is updated in the background if the remaining valid duration of the refresh token falls below the
    // configured duration (OAuthConfig::mMinValidDurationForUserSessionRefreshToken). This is done periodically to make sure that we don't lose the refresh token. Because if we
    // do, the user will have to log in again.

    // Server
    // Access Token - The access token (and refresh token as a side effect) is updated in the background if the remaining valid duration of the access token falls below the
    // configured duration (OAuthConfig::mMinValidDurationForServerAccessToken). This is done periodically to make sure that the code path on server needing the token does not have
    // added latency for the token retrieval. Since the server tokens are fairly small in number, this does not constitute an increase in load.

    // Refresh Token - As the access token are updated in the background periodically, there is no need to update the refresh token separately. That update happens as a side effect
    // of the previous update.
    void onUpdateUserSessionCachedTokensTimer();
    void onUpdateServerCachedTokensTimer();

    BlazeRpcError DEFINE_ASYNC_RET(getAccessToken(NucleusConnect::GetAccessTokenRequest &request, NucleusConnect::GetAccessTokenResponse &response, const char8_t* productName));

    void populateCachedTokenInfo(CachedTokenInfo& cachedTokenInfo, const NucleusConnect::GetAccessTokenResponse& nucleusResponse, const char8_t* clientId, const char8_t* productName, const char8_t* ipAddr);

    void forceUserLogout(const UserSessionId userSessionId, const char8_t* noteToLog);
    void forceUserLogoutJob(const UserSessionId userSessionId);

private:
    static Blaze::Nucleus::ErrorCodes mErrorCodes;

    typedef RedisKeyMap<FirstPartyTokenCacheKey, CachedFirstPartyToken> BlazeIdToCachedFirstPartyTokenMap;
    BlazeIdToCachedFirstPartyTokenMap mBlazeIdToCachedFirstPartyTokens; // Single user/player has 1 first party access token despite the possibility of having multiple user sessions.

    typedef RedisKeyMap<UserSessionId, CachedTokenInfo> UserSessionIdToCachedUserTokenMap;
    UserSessionIdToCachedUserTokenMap mTokenInfoByUserSessionIdCache; // Single user can have multiple UseSessions and token is separate for each.

    typedef RedisKeyMap<eastl::string, CachedTrustedToken> AccessTokenToCachedTrustedTokenMap;
    AccessTokenToCachedTrustedTokenMap mAccessTokenToCachedTrustedTokens;

    // Caching support for pure access token based requests
    typedef RedisKeyMap<eastl::string, IdentityContext> AccessTokenToIdentityContextMap;
    AccessTokenToIdentityContextMap mAccessTokenToIdentityContexts;

    // + Access token for the server itself.
    // Normally, the map (keyed by clientId) has just 1 entry because the Blaze server config has a single Nucleus client id.
    // However, if the client id gets reconfigured, multiple access token may co-exist transiently (reconfiguring the client id is slightly odd but apparently allowed).
    typedef eastl::pair<CachedTokenInfo, CachedTokenInfo> ServerTokenPair; // <Full scope token, Restricted scope token>
    typedef eastl::hash_map<eastl::string, ServerTokenPair, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> ServerTokensByClientIdMap;

    // These tokens are cached locally since they are instance specific and not shared across cluster
    ServerTokensByClientIdMap mServerTokens;
    //-

    typedef eastl::map<eastl::string, eastl::vector<eastl::string> > ComponentScopesMap;
    ComponentScopesMap mComponentScopesMap;

    TimerId mUpdateUserSessionTokensTimerId;
    TimerId mUpdateServerTokensTimerId;

    FiberJobQueue mBackgroundJobQueue;

    //metrics
    Metrics::Counter mServerAccessTokenRefreshFails;
    Metrics::Counter mUserAccessTokenRefreshFails;
    Metrics::Counter mUserFirstPartyTokenRetrievalFails;
    Metrics::Counter mUserTokenFailForcedLogouts;

    struct PsnServerAccessTokenInfo
    {
        eastl::string accessToken;
        TimeValue expiresAt;

        bool isValid() const
        {
            return !accessToken.empty() && (expiresAt < TimeValue::getTimeOfDay());
        }
    };

    PsnServerAccessTokenInfo mPsnServerAccessTokenInfo;
    Fiber::WaitList mPsnServerAccessTokenWaitList;
    TimerId mPsnServerAccessTokenRefreshTimerId;
    JwtUtil mJwtUtil;
};

} //OAuth

extern EA_THREAD_LOCAL OAuth::OAuthSlaveImpl *gOAuthManager;

} // Blaze

#endif  // BLAZE_OAUTH_OAUTHSLAVEIMPL_H
