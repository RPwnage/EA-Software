#ifndef BLAZE_OAUTH_ACCESSTOKENUTIL_H
#define BLAZE_OAUTH_ACCESSTOKENUTIL_H

namespace Blaze
{
namespace OAuth
{

class AccessTokenUtil
{
public:

    AccessTokenUtil() { }
    ~AccessTokenUtil() { }

    BlazeRpcError retrieveCurrentUserAccessToken(TokenType prefixType);
    BlazeRpcError retrieveUserAccessToken(TokenType prefixType, UserSessionId userSessionId);

    BlazeRpcError retrieveServerAccessToken(TokenType prefixType, const char8_t* allowedScope = nullptr);

    [[deprecated("remove serviceName parameter")]]
    BlazeRpcError retrieveCurrentUserAccessToken(const char8_t* serviceName, TokenType prefixType) { return retrieveCurrentUserAccessToken(prefixType); }

    [[deprecated("remove serviceName parameter")]]
    BlazeRpcError retrieveUserAccessToken(const char8_t* serviceName, TokenType prefixType, UserSessionId userSessionId) { return retrieveUserAccessToken(prefixType, userSessionId); }

    [[deprecated("remove serviceName parameter")]]
    BlazeRpcError retrieveServerAccessToken(const char8_t* serviceName, TokenType prefixType, const char8_t* allowedScope = nullptr) { return retrieveServerAccessToken(prefixType, allowedScope); }

    const char8_t* getAccessToken() const { return mAccessToken.c_str(); }
    const char8_t* getClientId() const { return mClientId.c_str(); }
    const TimeValue& getValidUntil() const { return mValidUntil; }

private:
    eastl::string mAccessToken;
    eastl::string mClientId;
    TimeValue mValidUntil;
};

bool shouldUseCurrentUserToken(BlazeId blazeId, AccountId accountId);
} //namespace OAuth
} //namespace Blaze

#endif
