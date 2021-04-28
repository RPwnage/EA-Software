#include "framework/blaze.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/oauth/accesstokenutil.h"

#include "authentication/rpc/authenticationslave.h"
#include "authentication/tdf/authentication.h"

#include "authentication/util/accesstokenutil.h"

namespace Blaze
{
namespace Authentication
{

// While the code below looks odd, it is doing what the original code did to preserve backward compatibility but it is written in terms of new api.


BlazeRpcError AccessTokenUtil::retrieveAccessToken(const char8_t* serviceName, bool includePrefix/* = true*/, bool fetchServerToken/* = true*/)
{
    WARN_LOG("[AccessTokenUtil].retrieveAccessToken: This api is deprecated. Please use Blaze::OAuth::AccessTokenUtil::<retrieveCurrentUserAccessToken or retrieveServerAccessToken> depending on your needs.");
    
    BlazeRpcError err = ERR_SYSTEM;

    OAuth::TokenType prefixType = includePrefix ? OAuth::TOKEN_TYPE_BEARER : OAuth::TOKEN_TYPE_NONE;
    OAuth::AccessTokenUtil tokenUtil;
    
    if (UserSession::hasSuperUserPrivilege() && fetchServerToken)
    {
        err = tokenUtil.retrieveServerAccessToken(prefixType);
    }
    else if (gCurrentUserSession != nullptr)
    {
        err = tokenUtil.retrieveCurrentUserAccessToken(prefixType);
    }

    if (err == ERR_OK)
    {
        mBearerToken = tokenUtil.getAccessToken();
        mClientId = tokenUtil.getClientId();
    }

    return err;
}

BlazeRpcError AccessTokenUtil::retrieveServerAccessToken(const char8_t* serviceName, bool includePrefix/* = true*/, const char8_t* allowedScope/* = nullptr*/)
{
    WARN_LOG("[AccessTokenUtil].retrieveServerAccessToken: This api is deprecated. Please use Blaze::OAuth::AccessTokenUtil::retrieveServerAccessToken api.");
    
    BlazeRpcError err = ERR_SYSTEM;

    OAuth::TokenType prefixType = includePrefix ? OAuth::TOKEN_TYPE_BEARER : OAuth::TOKEN_TYPE_NONE;
    Blaze::OAuth::AccessTokenUtil tokenUtil;
    err = tokenUtil.retrieveServerAccessToken(prefixType, allowedScope);
    
    if (err == ERR_OK)
    {
        mBearerToken = tokenUtil.getAccessToken();
        mClientId = tokenUtil.getClientId();
    }

    return err;
}

BlazeRpcError AccessTokenUtil::retrieveAccessTokenByAccountId(const char8_t* serviceName, AccountId accountId /*= INVALID_ACCOUNT_ID*/, bool includePrefix/* = true*/, bool fetchServerToken/* = true*/)
{
    WARN_LOG("[AccessTokenUtil].retrieveAccessTokenByAccountId: This api is deprecated. Please use Blaze::OAuth::AccessTokenUtil::<retrieveCurrentUserAccessToken or retrieveServerAccessToken> depending on your needs.");
    
    if (accountId == INVALID_ACCOUNT_ID)
    {
        if (gCurrentUserSession != nullptr)
            accountId = gCurrentUserSession->getAccountId();
        else
            return ERR_SYSTEM;
    }

    BlazeRpcError err = ERR_SYSTEM;
    OAuth::TokenType prefixType = includePrefix ? OAuth::TOKEN_TYPE_BEARER : OAuth::TOKEN_TYPE_NONE;
    OAuth::AccessTokenUtil tokenUtil;

    if (gCurrentUserSession != nullptr && gCurrentUserSession->getAccountId() == accountId)
    {
        err = tokenUtil.retrieveCurrentUserAccessToken(prefixType);
    }
    else if (accountId != INVALID_ACCOUNT_ID && fetchServerToken)
    {
        err = tokenUtil.retrieveServerAccessToken(prefixType);
    }

    if (err == ERR_OK)
    {
        mBearerToken = tokenUtil.getAccessToken();
        mClientId = tokenUtil.getClientId();
    }

    return err;
}

BlazeRpcError AccessTokenUtil::retrieveAccessTokenByBlazeId(const char8_t* serviceName, BlazeId blazeId /*= INVALID_BLAZE_ID*/, bool includePrefix/* = true*/, bool fetchServerToken/* = true*/)
{
    WARN_LOG("[AccessTokenUtil].retrieveAccessTokenByBlazeId: This api is deprecated. Please use Blaze::OAuth::AccessTokenUtil::<retrieveCurrentUserAccessToken or retrieveServerAccessToken> depending on your needs.");
    
    if (blazeId == INVALID_BLAZE_ID)
    {
        if (gCurrentUserSession != nullptr)
            blazeId = gCurrentUserSession->getBlazeId();
        else
            return ERR_SYSTEM;
    }

    BlazeRpcError err = ERR_SYSTEM;
    OAuth::TokenType prefixType = includePrefix ? OAuth::TOKEN_TYPE_BEARER : OAuth::TOKEN_TYPE_NONE;
    OAuth::AccessTokenUtil tokenUtil;

    if (gCurrentUserSession != nullptr && gCurrentUserSession->getBlazeId() == blazeId)
    {
        err = tokenUtil.retrieveCurrentUserAccessToken(prefixType);
    }
    else if (blazeId != INVALID_BLAZE_ID && fetchServerToken)
    {
        err = tokenUtil.retrieveServerAccessToken(prefixType);
    }

    if (err == ERR_OK)
    {
        mBearerToken = tokenUtil.getAccessToken();
        mClientId = tokenUtil.getClientId();
    }

    return err;
}

}
}
