#ifndef BLAZE_AUTHENTICATION_ACCESSTOKENUTIL_H
#define BLAZE_AUTHENTICATION_ACCESSTOKENUTIL_H

namespace Blaze
{
namespace Authentication
{

// Note: This old API exists only for backward compatibility purposes. All new code (and custom code) should use Blaze::OAuth::AccessTokenUtil. The new api provides similar capabilities to the ones below
// however remove the various fallbacks to the server access token.

class AccessTokenUtil
{
public:
    AccessTokenUtil()
    {
        mBearerToken[0] = '\0';
    }

    [[deprecated("use Blaze::OAuth::AccessTokenUtil::retrieveCurrentUserAccessToken or Blaze::OAuth::AccessTokenUtil::retrieveServerAccessToken instead")]]
    BlazeRpcError retrieveAccessToken(const char8_t* serviceName, bool includePrefix = true, bool fetchServerToken = true);

    [[deprecated("use Blaze::OAuth::AccessTokenUtil::retrieveServerAccessToken instead")]]
    BlazeRpcError retrieveServerAccessToken(const char8_t* serviceName, bool includePrefix = true, const char8_t* allowedScope = nullptr);

    [[deprecated("use Blaze::OAuth::AccessTokenUtil::retrieveCurrentUserAccessToken or Blaze::OAuth::AccessTokenUtil::retrieveServerAccessToken instead")]]
    BlazeRpcError retrieveAccessTokenByAccountId(const char8_t* serviceName, AccountId accountId = INVALID_ACCOUNT_ID, bool includePrefix = true, bool fetchServerToken = true);

    [[deprecated("use Blaze::OAuth::AccessTokenUtil::retrieveCurrentUserAccessToken or Blaze::OAuth::AccessTokenUtil::retrieveServerAccessToken instead")]]
    BlazeRpcError retrieveAccessTokenByBlazeId(const char8_t* serviceName, BlazeId blazeId = INVALID_BLAZE_ID, bool includePrefix = true, bool fetchServerToken = true);

    const char8_t *getAccessToken() const { return mBearerToken.c_str(); }
    const char8_t *getClientId() const { return mClientId.c_str(); }

private:
    eastl::string mBearerToken;
    eastl::string mClientId;
};

} //namespace Authentication
} //namespace Blaze

#endif
