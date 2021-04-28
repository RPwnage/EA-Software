#include "framework/blaze.h"
#include "framework/connection/outboundhttpservice.h"

#include "framework/oauth/accesstokenutil.h"
#include "authentication/util/authenticationutil.h"
#include "authentication/util/legaldocscheckutil.h"

#include "nucleusidentity/rpc/nucleusidentityslave.h"
#include "nucleusidentity/tdf/nucleusidentity.h"

namespace Blaze
{
namespace Authentication
{

BlazeRpcError LegalDocsCheckUtil::checkForLegalDoc(AccountId id, const char8_t* tosString, bool& accepted)
{
    accepted = false;

    if ((tosString == nullptr) || (strlen(tosString) == 0))
    {
        TRACE_LOG("[LegalDocsCheckUtil].checkForLegalDoc(): Invalid legal doc uri");
        return ERR_SYSTEM;
    }

    NucleusIdentity::NucleusIdentitySlave* ident = (NucleusIdentity::NucleusIdentitySlave*) gOutboundHttpService->getService(NucleusIdentity::NucleusIdentitySlave::COMPONENT_INFO.name);
    if (ident == nullptr)
    {
        WARN_LOG("[LegalDocsCheckUtil].checkForLegalDoc(): NucleusIdentity component is not available");
        return ERR_SYSTEM;
    }

    OAuth::AccessTokenUtil tokenUtil;
    BlazeRpcError rc = tokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_BEARER);
    if (rc != Blaze::ERR_OK)
    {
        ERR_LOG("[LegalDocsCheckUtil].checkForLegalDoc(): Failed to retrieve access token");
        return rc;
    }

    NucleusIdentity::GetTosRequest req;
    NucleusIdentity::GetTosResponse resp;
    NucleusIdentity::IdentityError error;
    req.setPid(id);
    req.setTosString(tosString);
    req.getAuthCredentials().setAccessToken(tokenUtil.getAccessToken());
    req.getAuthCredentials().setClientId(tokenUtil.getClientId());
    req.getAuthCredentials().setIpAddress(mPeerInfo->getRealPeerAddress().getIpAsString());

    rc = ident->getTos(req, resp, error);
    if (rc != ERR_OK)
    {
        rc = AuthenticationUtil::parseIdentityError(error, rc);
        ERR_LOG("[LegalDocsCheckUtil].checkForLegalDoc(): getTos failed with error(" << ErrorHelp::getErrorName(rc) << ")");
        return rc;
    }

    if (!resp.getTosUri().empty())
    {
        accepted = true;
    }

    return rc;
}

}
}
