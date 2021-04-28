/*************************************************************************************************/
/*!
    \file   updateaccesstoken_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files ********************************************************************************/
#include "framework/blaze.h"
#include "framework/oauth/oauthslaveimpl.h"
#include "framework/oauth/accesstokenutil.h"
#include "authentication/rpc/authenticationslave/updateaccesstoken_stub.h"
#include "authentication/authenticationimpl.h"
#include "authentication/util/authenticationutil.h"

namespace Blaze
{
namespace Authentication
{

class UpdateAccessTokenCommand : public UpdateAccessTokenCommandStub
{
public:
    UpdateAccessTokenCommand(Message* message, UpdateAccessTokenRequest* request, AuthenticationSlaveImpl* componentImpl)
        : UpdateAccessTokenCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

private:
    UpdateAccessTokenCommandStub::Errors execute() override;
    BlazeRpcError checkTokenInfo(const NucleusConnect::GetTokenInfoResponse& newTokenInfo, const NucleusConnect::GetTokenInfoResponse& oldTokenInfo);

private:
    AuthenticationSlaveImpl* mComponent;
};

UpdateAccessTokenCommandStub::Errors UpdateAccessTokenCommand::execute()
{
    BlazeRpcError err = Blaze::ERR_SYSTEM;
    OAuth::OAuthSlaveImpl* oAuthSlave = mComponent->oAuthSlaveImpl();
    UserSessionId userSessionId = gCurrentUserSession->getUserSessionId();
    const char8_t* serviceName = getPeerInfo()->getServiceName();
    ClientPlatformType clientPlatform = gController->getServicePlatform(serviceName);
    InetAddress inetAddr = AuthenticationUtil::getRealEndUserAddr(this);
    const char8_t* clientId = mComponent->getBlazeServerNucleusClientId();
    const char8_t* accessToken = mRequest.getAccessToken();

    //step 1: get token info of new access token
    NucleusConnect::GetTokenInfoRequest tokenInfoRequest;
    tokenInfoRequest.setAccessToken(accessToken);
    tokenInfoRequest.setIpAddress(inetAddr.getIpAsString());
    tokenInfoRequest.setClientId(clientId);

    NucleusConnect::GetTokenInfoResponse tokenInfoResponse;
    NucleusConnect::JwtPayloadInfo jwtPayloadInfo;
    err = AuthenticationUtil::authFromOAuthError(oAuthSlave->getTokenInfo(tokenInfoRequest, tokenInfoResponse, jwtPayloadInfo, clientPlatform));
    if (err != Blaze::ERR_OK)
    {
        ERR_LOG("[UpdateAccessTokenCommand].execute: Failed to get token info of new access token with error(" << ErrorHelp::getErrorName(err) << ") for UserSessionId(" << userSessionId << ")");
        return commandErrorFromBlazeError(err);
    }

    //step 2: get old access token
    OAuth::AccessTokenUtil tokenUtil;
    err = tokenUtil.retrieveCurrentUserAccessToken(OAuth::TOKEN_TYPE_NONE);
    if (err != Blaze::ERR_OK)
    {
        ERR_LOG("[UpdateAccessTokenCommand].execute: Failed to retrieve old access token with error(" << ErrorHelp::getErrorName(err) << ") for UserSessionId(" << userSessionId << ")");
        return commandErrorFromBlazeError(err);
    }
    const char8_t* oldAccessToken = tokenUtil.getAccessToken();

    //step 3: get token info of old access token and check against new access token
    tokenInfoRequest.setAccessToken(oldAccessToken);
    NucleusConnect::GetTokenInfoResponse oldTokenInfoResponse;
    err = AuthenticationUtil::authFromOAuthError(oAuthSlave->getTokenInfo(tokenInfoRequest, oldTokenInfoResponse, jwtPayloadInfo, clientPlatform));

    if (err == Blaze::ERR_OK)
    {
        err = checkTokenInfo(tokenInfoResponse, oldTokenInfoResponse);
        if (err != Blaze::ERR_OK)
        {
            ERR_LOG("[UpdateAccessTokenCommand].execute: New access token info does not match old access token info for UserSessionId(" << userSessionId << ")");
            return commandErrorFromBlazeError(err);
        }
    }
    else
    {
        WARN_LOG("[UpdateAccessTokenCommand].execute: Failed to get token info of old access token with error(" << ErrorHelp::getErrorName(err) << ") for UserSessionId(" << userSessionId << "). Skipping token info check against old access token.");
    }

    //step 4: update user session cached token with new access token
    err = AuthenticationUtil::authFromOAuthError(oAuthSlave->updateUserSessionAccessToken(userSessionId, accessToken, tokenInfoResponse.getExpiresIn()));
    if (err != Blaze::ERR_OK)
    {
        ERR_LOG("[UpdateAccessTokenCommand].execute: Failed to update user session access token with error(" << ErrorHelp::getErrorName(err) << ") for UserSessionId(" << userSessionId << ")");
        return commandErrorFromBlazeError(err);
    }

    mResponse.setTokenUpdated(true);
    TRACE_LOG("[UpdateAccessTokenCommand].execute: Updated access token for UserSessionId(" << userSessionId << "), new token is: " << accessToken);
    return commandErrorFromBlazeError(err);
}

BlazeRpcError UpdateAccessTokenCommand::checkTokenInfo(const NucleusConnect::GetTokenInfoResponse& newTokenInfo, const NucleusConnect::GetTokenInfoResponse& oldTokenInfo)
{
    UserSessionId userSessionId = gCurrentUserSession->getUserSessionId();

    if (blaze_strcmp(newTokenInfo.getAuthSource(), oldTokenInfo.getAuthSource()) != 0)
    {
        ERR_LOG("[UpdateAccessTokenCommand].checkTokenInfo: New access token Auth Source(" << newTokenInfo.getAuthSource() << ") does not match old access token Auth Source(" << oldTokenInfo.getAuthSource() << ") for UserSessionId(" << userSessionId << ")");
        return Blaze::AUTH_ERR_INVALID_TOKEN;
    }

    if (blaze_strcmp(newTokenInfo.getClientId(), oldTokenInfo.getClientId()) != 0)
    {
        ERR_LOG("[UpdateAccessTokenCommand].checkTokenInfo: New access token Client Id(" << newTokenInfo.getClientId() << ") does not match old access token Client Id(" << oldTokenInfo.getClientId() << ") for UserSessionId(" << userSessionId << ")");
        return Blaze::AUTH_ERR_INVALID_TOKEN;
    }

    if (blaze_strcmp(newTokenInfo.getConnectionType(), oldTokenInfo.getConnectionType()) != 0)
    {
        ERR_LOG("[UpdateAccessTokenCommand].checkTokenInfo: New access token Connection Type(" << newTokenInfo.getConnectionType() << ") does not match old access token Connection Type(" << oldTokenInfo.getConnectionType() << ") for UserSessionId(" << userSessionId << ")");
        return Blaze::AUTH_ERR_INVALID_TOKEN;
    }

    if (blaze_strcmp(newTokenInfo.getDeviceId(), oldTokenInfo.getDeviceId()) != 0)
    {
        ERR_LOG("[UpdateAccessTokenCommand].checkTokenInfo: New access token Device Id(" << newTokenInfo.getDeviceId() << ") does not match old access token Device Id(" << oldTokenInfo.getDeviceId() << ") for UserSessionId(" << userSessionId << ")");
        return Blaze::AUTH_ERR_INVALID_TOKEN;
    }

    if (newTokenInfo.getPersonaId() != oldTokenInfo.getPersonaId())
    {
        ERR_LOG("[UpdateAccessTokenCommand].checkTokenInfo: New access token Persona Id(" << newTokenInfo.getPersonaId() << ") does not match old access token Persona Id(" << oldTokenInfo.getPersonaId() << ") for UserSessionId(" << userSessionId << ")");
        return Blaze::AUTH_ERR_INVALID_TOKEN;
    }

    if (blaze_strcmp(newTokenInfo.getPidType(), oldTokenInfo.getPidType()) != 0)
    {
        ERR_LOG("[UpdateAccessTokenCommand].checkTokenInfo: New access token Pid Type(" << newTokenInfo.getPidType() << ") does not match old access token Pid Type(" << oldTokenInfo.getPidType() << ") for UserSessionId(" << userSessionId << ")");
        return Blaze::AUTH_ERR_INVALID_TOKEN;
    }

    if (blaze_strcmp(newTokenInfo.getPidId(), oldTokenInfo.getPidId()) != 0)
    {
        ERR_LOG("[UpdateAccessTokenCommand].checkTokenInfo: New access token Pid Id(" << newTokenInfo.getPidId() << ") does not match old access token Pid Id(" << oldTokenInfo.getPidId() << ") for UserSessionId(" << userSessionId << ")");
        return Blaze::AUTH_ERR_INVALID_TOKEN;
    }

    if (blaze_strcmp(newTokenInfo.getConsoleEnv(), oldTokenInfo.getConsoleEnv()) != 0)
    {
        ERR_LOG("[UpdateAccessTokenCommand].checkTokenInfo: New access token Console Env(" << newTokenInfo.getConsoleEnv() << ") does not match old access token Console Env(" << oldTokenInfo.getConsoleEnv() << ") for UserSessionId(" << userSessionId << ")");
        return Blaze::AUTH_ERR_INVALID_TOKEN;
    }

    if (blaze_strcmp(newTokenInfo.getStopProcess(), oldTokenInfo.getStopProcess()) != 0)
    {
        ERR_LOG("[UpdateAccessTokenCommand].checkTokenInfo: New access token Stop Process(" << newTokenInfo.getStopProcess() << ") does not match old access token Stop Process(" << oldTokenInfo.getStopProcess() << ") for UserSessionId(" << userSessionId << ")");
        return Blaze::AUTH_ERR_INVALID_TOKEN;
    }

    if (blaze_strcmp(newTokenInfo.getAccountId(), oldTokenInfo.getAccountId()) != 0)
    {
        ERR_LOG("[UpdateAccessTokenCommand].checkTokenInfo: New access token Account Id(" << newTokenInfo.getAccountId() << ") does not match old access token Account Id(" << oldTokenInfo.getAccountId() << ") for UserSessionId(" << userSessionId << ")");
        return Blaze::AUTH_ERR_INVALID_TOKEN;
    }

    if (newTokenInfo.getExpiresIn() < oldTokenInfo.getExpiresIn())
    {
        WARN_LOG("[UpdateAccessTokenCommand].checkTokenInfo: New access token Expires In(" << newTokenInfo.getExpiresIn() << ") is smaller than old access token Expires In(" << oldTokenInfo.getExpiresIn() << ") for UserSessionId(" << userSessionId << ")");
    }

    return Blaze::ERR_OK;
}

DEFINE_UPDATEACCESSTOKEN_CREATE()

} // Authentication
} // Blaze
