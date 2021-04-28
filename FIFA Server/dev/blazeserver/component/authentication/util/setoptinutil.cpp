/*************************************************************************************************/
/*!
    \file setoptinutil.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "framework/blaze.h"
#include "framework/connection/outboundhttpservice.h"

#include "framework/oauth/accesstokenutil.h"
#include "authentication/util/authenticationutil.h"
#include "authentication/util/setoptinutil.h"

#include "proxycomponent/nucleusidentity/rpc/nucleusidentityslave.h"
#include "proxycomponent/nucleusidentity/tdf/nucleusidentity.h"

namespace Blaze
{
namespace Authentication
{

SetOptInUtil::SetOptInUtil(AuthenticationSlaveImpl &componentImpl, const PeerInfo* peerInfo, const ClientType &clientType)
    : mComponent(componentImpl)
    , mPeerInfo(peerInfo)
{
}

BlazeRpcError SetOptInUtil::SetOptIn(AccountId accountId, const char8_t* optInName, const char8_t* nucleusSourceHeader, bool optInValue)
{
    BlazeRpcError rc = ERR_OK;

    OAuth::AccessTokenUtil serverTokenUtil;
    {
        //retrieve full scope server access token in a block to avoid changing function context permission
        UserSession::SuperUserPermissionAutoPtr autoPtr(true);
        rc = serverTokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_BEARER);
        if (rc != Blaze::ERR_OK)
        {
            ERR_LOG("[SetOptInUtil].SetOptIn: Failed to retrieve server access token");
            return rc;
        }
    }

    OAuth::AccessTokenUtil userTokenUtil;
    rc = userTokenUtil.retrieveCurrentUserAccessToken(OAuth::TOKEN_TYPE_NONE);
    if (rc != Blaze::ERR_OK)
    {
        ERR_LOG("[SetOptInUtil].SetOptIn: Failed to retrieve user access token for user " << gCurrentUserSession->getBlazeId());
        return rc;
    }

    NucleusIdentity::OptinRequest req;
    NucleusIdentity::OptinResponse resp;
    NucleusIdentity::IdentityError error;
    req.setOptinType(optInName);
    req.setPid(accountId);
    req.getAuthCredentials().setAccessToken(serverTokenUtil.getAccessToken());
    req.getAuthCredentials().setUserAccessToken(userTokenUtil.getAccessToken());
    req.getAuthCredentials().setClientId(userTokenUtil.getClientId());

    req.getAuthCredentials().setIpAddress(mPeerInfo->getRealPeerAddress().getIpAsString());

    NucleusIdentity::NucleusIdentitySlave* ident = (NucleusIdentity::NucleusIdentitySlave*)(gOutboundHttpService->getService(NucleusIdentity::NucleusIdentitySlave::COMPONENT_INFO.name));

    if (optInValue)
    {
        rc = ident->postOptin(req, resp, error);
        if (rc != ERR_OK)
        {
            rc = AuthenticationUtil::parseIdentityError(error, rc);
            ERR_LOG("[SetOptInUtil].setOptIn: setOptin failed with error(" << ErrorHelp::getErrorName(rc) << ")");
            return rc;
        }
    }
    else
    {
        rc = ident->deleteOptin(req, error);
        if (rc != ERR_OK)
        {
            rc = AuthenticationUtil::parseIdentityError(error, rc);
            ERR_LOG("[GetOptInUtil].getOptIn: deleteOptin failed with error(" << ErrorHelp::getErrorName(rc) << ")");
            return rc;
        }
    }

    return rc;
}

} // Authentication
} // Blaze
