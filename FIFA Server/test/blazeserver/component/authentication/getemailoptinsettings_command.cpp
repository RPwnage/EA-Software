/*************************************************************************************************/
/*!
    \file   getemailoptinsettings_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetEmailOptInSettingsCommand

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/connection/outboundhttpservice.h"
#include "framework/controller/controller.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"
#include "framework/util/locales.h"

// Authentication
#include "authentication/authenticationimpl.h"
#include "authentication/tdf/authentication.h"
#include "authentication/rpc/authenticationslave/getemailoptinsettings_stub.h"

// Authentication utils
#include "framework/oauth/accesstokenutil.h"
#include "authentication/util/authenticationutil.h"
#include "authentication/util/getusercountryutil.h"

#include "nucleusidentity/rpc/nucleusidentityslave.h"
#include "nucleusidentity/tdf/nucleusidentity.h"

namespace Blaze
{
namespace Authentication
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

class GetEmailOptInSettingsCommand : public GetEmailOptInSettingsCommandStub
{
public:
    GetEmailOptInSettingsCommand(Message* message, GetEmailOptInSettingsRequest* request, AuthenticationSlaveImpl* componentImpl);

    ~GetEmailOptInSettingsCommand() override {};

private:
    // Not owned memory
    AuthenticationSlaveImpl* mComponent;

    // Owned memory
    GetUserCountryUtil mGetUserCountryUtil;

    // States
    GetEmailOptInSettingsCommandStub::Errors execute() override;

};
DEFINE_GETEMAILOPTINSETTINGS_CREATE()

GetEmailOptInSettingsCommand::GetEmailOptInSettingsCommand(Message* message, GetEmailOptInSettingsRequest* request, AuthenticationSlaveImpl* componentImpl)
  : GetEmailOptInSettingsCommandStub(message, request),
    mComponent(componentImpl),
    mGetUserCountryUtil(*componentImpl, getPeerInfo())
{
}

GetEmailOptInSettingsCommandStub::Errors GetEmailOptInSettingsCommand::execute()
{
    const char8_t *country = mRequest.getIsoCountryCode();

    if(country[0] == '\0')
    {
        country = mGetUserCountryUtil.getUserCountry(UserSession::getCurrentUserBlazeId());
        if (country == nullptr)
            return ERR_SYSTEM;
    }

    NucleusIdentity::NucleusIdentitySlave* ident = (NucleusIdentity::NucleusIdentitySlave*) gOutboundHttpService->getService(NucleusIdentity::NucleusIdentitySlave::COMPONENT_INFO.name);
    if (ident == nullptr)
    {
        WARN_LOG("[GetEmailOptInSettingsCommand].execute(): NucleusIdentity component is not available");
        return ERR_SYSTEM;
    }

    OAuth::AccessTokenUtil tokenUtil;
    BlazeRpcError rc = tokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_BEARER);
    if (rc != Blaze::ERR_OK)
    {
        ERR_LOG("[GetEmailOptInSettingsCommand].execute(): Failed to retrieve access token");
        return commandErrorFromBlazeError(rc);
    }

    NucleusIdentity::GetInfoSharingRequirementsRequest req;
    NucleusIdentity::GetInfoSharingRequirementsResponse resp;
    NucleusIdentity::IdentityError error;
    req.setCountry(country);
    req.getAuthCredentials().setAccessToken(tokenUtil.getAccessToken());
    req.getAuthCredentials().setIpAddress(getPeerInfo()->getRealPeerAddress().getIpAsString());

    rc = ident->getInfoSharingRequirements(req, resp, error);
    if (rc != ERR_OK)
    {
        rc = AuthenticationUtil::parseIdentityError(error, rc);
        ERR_LOG("[GetEmailOptInSettingsCommand].execute: getInfoSharingRequirements failed with error(" << ErrorHelp::getErrorName(rc) << ")");
        return commandErrorFromBlazeError(rc);
    }

    mResponse.setEaMayContact(resp.getInfoSharingReqs().getEaMayContact());
    mResponse.setPartnersMayContact(resp.getInfoSharingReqs().getEaPartnersMayContact());

    return ERR_OK;
}

} // Authentication
} // Blaze
