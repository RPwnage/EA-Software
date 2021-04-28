/*************************************************************************************************/
/*!
    \file getoptinutil.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "framework/blaze.h"
#include "framework/connection/outboundhttpservice.h"

#include "authentication/rpc/authentication_defines.h"
#include "authentication/tdf/authentication.h"

#include "framework/oauth/accesstokenutil.h"
#include "authentication/util/authenticationutil.h"
#include "authentication/util/getoptinutil.h"

#include "proxycomponent/nucleusidentity/rpc/nucleusidentityslave.h"
#include "proxycomponent/nucleusidentity/tdf/nucleusidentity.h"

namespace Blaze
{
namespace Authentication
{

GetOptInUtil::GetOptInUtil(AuthenticationSlaveImpl &componentImpl, const PeerInfo* peerInfo, const ClientType &clientType)
  : mComponent(componentImpl),
    mPeerInfo(peerInfo)
{
}

BlazeRpcError GetOptInUtil::getOptIn(AccountId accountId, const char8_t* optInName, OptInValue& optInValue)
{
    BlazeRpcError rc = ERR_OK;

    OAuth::AccessTokenUtil serverTokenUtil;
    rc = serverTokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_BEARER);
    if (rc != ERR_OK)
    {
        ERR_LOG("[GetOptInUtil].getOptIn: Failed to retrieve server access token with error(" << ErrorHelp::getErrorName(rc) << ")");
        return rc;
    }

    OAuth::AccessTokenUtil userTokenUtil;
    rc = userTokenUtil.retrieveCurrentUserAccessToken(OAuth::TOKEN_TYPE_NONE);
    if (rc != ERR_OK)
    {
        ERR_LOG("[GetOptInUtil].getOptIn: Failed to retrieve user access token with error(" << ErrorHelp::getErrorName(rc) << ") for user " << gCurrentUserSession->getBlazeId());
        return rc;
    }

    NucleusIdentity::OptinRequest req;
    NucleusIdentity::OptinResponse resp;
    NucleusIdentity::IdentityError error;
    req.setPid(accountId);
    req.setOptinType(optInName);
    req.getAuthCredentials().setAccessToken(serverTokenUtil.getAccessToken());
    req.getAuthCredentials().setUserAccessToken(userTokenUtil.getAccessToken());
    req.getAuthCredentials().setClientId(userTokenUtil.getClientId());

    req.getAuthCredentials().setIpAddress(mPeerInfo->getRealPeerAddress().getIpAsString());

    NucleusIdentity::NucleusIdentitySlave* ident = (NucleusIdentity::NucleusIdentitySlave*)(gOutboundHttpService->getService(NucleusIdentity::NucleusIdentitySlave::COMPONENT_INFO.name));
    
    rc = ident->getOptin(req, resp, error);
    if (rc != ERR_OK)
    {
        rc = AuthenticationUtil::parseIdentityError(error, rc);
        if (rc != AUTH_ERR_NO_SUCH_OPTIN)
        {
            ERR_LOG("[GetOptInUtil].getOptIn: getOptin failed with error(" << ErrorHelp::getErrorName(rc) << ")");
            return rc;
        }
    }

    if (rc == AUTH_ERR_NO_SUCH_OPTIN)
    {
        optInValue.setOptInValue(false);
        rc = ERR_OK;
    }
    else
    {
        optInValue.setOptInValue(true);
    }

    return rc;
}

} // Authentication
} // Blaze
