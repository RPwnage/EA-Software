#include "framework/blaze.h"
#include "framework/oauth/accesstokenutil.h"
#include "framework/oauth/oauthslaveimpl.h"

#include "framework/controller/controller.h"
#include "framework/usersessions/usersessionmanager.h"

#include "framework/tdf/oauth_server.h"

namespace Blaze
{
namespace OAuth
{

BlazeRpcError AccessTokenUtil::retrieveCurrentUserAccessToken(TokenType prefixType)
{
    BlazeRpcError err = ERR_SYSTEM;
    if (gCurrentUserSession != nullptr)
    {
        err = retrieveUserAccessToken(prefixType, gCurrentUserSession->getUserSessionId());
    }
    else
    {
        BLAZE_ERR_LOG(Log::OAUTH, "[AccessTokenUtil].retrieveCurrentUserAccessToken: Trying to retrieve the access token of the current user session but no user session exists!");
    }
    return err;
}

BlazeRpcError AccessTokenUtil::retrieveUserAccessToken(TokenType prefixType, UserSessionId userSessionId)
{
    OAuthSlave *oAuthSlave = static_cast<OAuthSlave*>(gController->getComponent(OAuth::OAuthSlave::COMPONENT_ID, false));
    if (oAuthSlave == nullptr)
    {
        BLAZE_ERR_LOG(Log::OAUTH, "[AccessTokenUtil].retrieveUserAccessToken: OAuth component is nullptr!");
        return ERR_SYSTEM;
    }

    bool hasPermission = UserSession::hasSuperUserPrivilege() ||
                        (gCurrentUserSession != nullptr && gCurrentUserSession->getUserSessionId() == userSessionId);

    if (!hasPermission)
    {
        BLAZE_ERR_LOG(Log::OAUTH, "[AccessTokenUtil].retrieveUserAccessToken: The caller does not have permission to get the access token for UserSession(" << userSessionId << ").");
        return ERR_SYSTEM;
    }

    BlazeRpcError err = ERR_SYSTEM;

    GetUserAccessTokenRequest request;
    request.setUserSessionId(userSessionId);

    GetUserAccessTokenResponse response;
    err = oAuthSlave->getUserAccessToken(request, response);
    if (err == Blaze::ERR_OK)
    {
        mClientId.sprintf("%s", response.getClientId());
        mAccessToken.sprintf("%s %s", gTokenTypeStr[prefixType], response.getAccessToken()).trim();
        mValidUntil = response.getValidUntil();
    }
    else
    {
        BLAZE_ERR_LOG(Log::OAUTH, "[AccessTokenUtil].retrieveUserAccessToken: Error retrieving access token for (" << userSessionId << "). Error (" << ErrorHelp::getErrorName(err) << ").");
    }

    return err;
}

BlazeRpcError AccessTokenUtil::retrieveServerAccessToken(TokenType prefixType, const char8_t* allowedScope/* = nullptr*/)
{
    OAuthSlave *oAuthSlave = static_cast<OAuthSlave*>(gController->getComponent(Blaze::OAuth::OAuthSlave::COMPONENT_ID, false));
    if (oAuthSlave == nullptr)
    {
        BLAZE_ERR_LOG(Log::OAUTH, "[AccessTokenUtil].retrieveServerAccessToken: OAuth component is nullptr!");
        return ERR_SYSTEM;
    }

    GetServerAccessTokenRequest request;

    if (allowedScope != nullptr)
        request.setAllowedScopes(allowedScope);

    GetServerAccessTokenResponse response;
    BlazeRpcError err = oAuthSlave->getServerAccessToken(request, response);
    if (err == Blaze::ERR_OK)
    {
        mClientId.sprintf("%s", response.getClientId());
        mAccessToken.sprintf("%s %s", gTokenTypeStr[prefixType], response.getAccessToken()).trim();
        mValidUntil = response.getValidUntil();
    }

    return err;
}

}
}
