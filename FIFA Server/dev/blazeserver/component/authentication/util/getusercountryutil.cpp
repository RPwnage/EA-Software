#include "framework/blaze.h"
#include "framework/connection/outboundhttpservice.h"

#include "authentication/util/getusercountryutil.h"
#include "authenticationimpl.h"
#include "framework/oauth/accesstokenutil.h"
#include "authentication/util/authenticationutil.h"

#include "nucleusidentity/rpc/nucleusidentityslave.h"
#include "nucleusidentity/tdf/nucleusidentity.h"

namespace Blaze
{
namespace Authentication
{

const char8_t* GetUserCountryUtil::getUserCountry(PersonaId personaId)
{
    NucleusIdentity::NucleusIdentitySlave* ident = (NucleusIdentity::NucleusIdentitySlave*) gOutboundHttpService->getService(NucleusIdentity::NucleusIdentitySlave::COMPONENT_INFO.name);
    if (ident == nullptr)
    {
        WARN_LOG("[GetUserCountryUtil].getUserCountry: NucleusIdentity component is not available");
        return nullptr;
    }

    OAuth::AccessTokenUtil tokenUtil;
    BlazeRpcError rc = tokenUtil.retrieveCurrentUserAccessToken(OAuth::TOKEN_TYPE_BEARER);
    if (rc != Blaze::ERR_OK)
    {
        ERR_LOG("[GetUserCountryUtil].getUserCountry: Failed to retrieve user access token for user " << personaId);
        return nullptr;
    }

    NucleusIdentity::GetAccountRequest req;
    NucleusIdentity::GetAccountResponse resp;
    NucleusIdentity::IdentityError error;
    //for better compatibility with old version Nucleus API, do not use dual token on getAccount call
    req.getAuthCredentials().setAccessToken(tokenUtil.getAccessToken());
    req.getAuthCredentials().setClientId(tokenUtil.getClientId());
    req.getAuthCredentials().setIpAddress(mPeerInfo->getRealPeerAddress().getIpAsString());

    rc = ident->getAccount(req, resp, error);
    if (rc != ERR_OK)
    {
        rc = AuthenticationUtil::parseIdentityError(error, rc);
        ERR_LOG("[GetUserCountryUtil].getUserCountry: getAccount failed with error(" << ErrorHelp::getErrorName(rc) << ")");
        return nullptr;
    }

    return resp.getPid().getCountry();
}

} //namespace Authentication
} //namespace Blaze
