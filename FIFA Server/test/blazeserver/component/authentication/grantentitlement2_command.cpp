/*************************************************************************************************/
/*!
    \file   grantentitlement2_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GrantEntitlement2Command

    Sends Nucleus an email and password for authentication.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"
#include "framework/tdf/userevents_server.h"

#include "authentication/authenticationimpl.h"
#include "authentication/rpc/authenticationslave/grantentitlement2_stub.h"
#include "authentication/tdf/authentication.h"
#include "authentication/util/grantentitlementutil.h"

namespace Blaze
{
namespace Authentication
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/
class GrantEntitlement2Command : public GrantEntitlement2CommandStub
{
public:
    GrantEntitlement2Command(Message* message, Blaze::Authentication::GrantEntitlement2Request* request, AuthenticationSlaveImpl* componentImpl);
    ~GrantEntitlement2Command() override {};

private:
    // Not owned memory.
    AuthenticationSlaveImpl* mComponent;

    // States
    GrantEntitlement2CommandStub::Errors execute() override;
};

DEFINE_GRANTENTITLEMENT2_CREATE();

GrantEntitlement2Command::GrantEntitlement2Command(Message* message, Blaze::Authentication::GrantEntitlement2Request* request, AuthenticationSlaveImpl* componentImpl)
    : GrantEntitlement2CommandStub(message, request),
    mComponent(componentImpl)
{
}

/* Private methods *******************************************************************************/
GrantEntitlement2CommandStub::Errors GrantEntitlement2Command::execute()
{
    const char8_t* serviceName = gCurrentUserSession != nullptr ? gCurrentUserSession->getServiceName() : gController->getDefaultServiceName();
    const char8_t* productName = gCurrentUserSession != nullptr ? gCurrentUserSession->getProductName() : gController->getServiceProductName(serviceName);

    char8_t addrStr[Login::MAX_IPADDRESS_LEN];
    addrStr[0] = '\0';
    char8_t localeStr[Login::MAX_LOCALE_LEN];
    localeStr[0] = '\0';
    char8_t countryStr[Login::MAX_COUNTRY_LEN];
    countryStr[0] = '\0';
    ClientInfo* clientInfo = nullptr;
    StringBuilder sb;

    if (!mRequest.getGroupName() || mRequest.getGroupName()[0] == '\0')
    {
        TRACE_LOG("[GrantEntitlement2Command].execute has no group name in the request.");
        gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(AUTH_ERR_GROUPNAME_REQUIRED), RiverPoster::entitlement_grant_failed);
        return AUTH_ERR_GROUPNAME_REQUIRED;
    }

    if (!mRequest.getEntitlementTag() || mRequest.getEntitlementTag()[0] == '\0')
    {
        TRACE_LOG("[GrantEntitlement2Command].execute(): No entitlement tag in the request.");
        gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(AUTH_ERR_ENTITLEMENT_TAG_REQUIRED), RiverPoster::entitlement_grant_failed);
        return AUTH_ERR_ENTITLEMENT_TAG_REQUIRED;
    }

    if (!mComponent->passClientGrantWhitelist(productName, mRequest.getGroupName()) &&
        !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_IGNORE_CLIENT_WHITELIST))
    {
        if (getPeerInfo() != nullptr && getPeerInfo()->isClient())
        {
            LocaleTokenCreateLocalityString(localeStr, getPeerInfo()->getLocale());
            LocaleTokenCreateCountryString(countryStr, getPeerInfo()->getCountry()); // connection/session country is not locale-related, but re-using locale macro for convenience
            sb << "IP(" << getPeerInfo()->getRealPeerAddress().toString(addrStr, Login::MAX_IPADDRESS_LEN)
                << "), ServiceName(" << getPeerInfo()->getServiceName() << "), Locale("
                << localeStr << "), Country(" << countryStr << ") ClientType(" << ClientTypeToString(getPeerInfo()->getClientType()) << "), ";
            clientInfo = getPeerInfo()->getClientInfo();
            if (clientInfo != nullptr)
            {
                sb << "\nClientInfo:" << clientInfo;
            }
        }

        ERR_LOG("[GrantEntitlement2Command].execute(): passClientGrantWhitelist failed for accountId[" << (gCurrentUserSession != nullptr ? gCurrentUserSession->getAccountId() : INVALID_ACCOUNT_ID)
            << "] " << sb.get() << "when attempting to grant entitlement tag[" << mRequest.getEntitlementTag()
            << "], type[" <<Blaze::Nucleus::EntitlementType::CodeToString(mRequest.getEntitlementType()) << "], productId[" << mRequest.getProductId() << "], projectId[" << mRequest.getProjectId() << "] to blazeid " << mRequest.getBlazeId()
            << ". Groupname(" << mRequest.getGroupName() << ") not defined in ClientEntitlementGrantWhitelist for product "<< productName << ".");
        gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(AUTH_ERR_WHITELIST), RiverPoster::entitlement_grant_failed);
        return AUTH_ERR_WHITELIST;
    }

    AccountId accountId = INVALID_ACCOUNT_ID;
    PersonaId personaId =  INVALID_BLAZE_ID;

    bool hasPermission = false;
    BlazeRpcError error = Blaze::ERR_OK;
    if (mRequest.getBlazeId() != INVALID_BLAZE_ID)
    {
        UserInfoPtr userInfo;
        error = gUserSessionManager->lookupUserInfoByBlazeId(mRequest.getBlazeId(), userInfo);
        if (error != Blaze::ERR_OK)
        {
            TRACE_LOG("[GrantEntitlement2Command].execute(): Invalid user in the request.");
            gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(AUTH_ERR_INVALID_USER), RiverPoster::entitlement_grant_failed);
            return AUTH_ERR_INVALID_USER;
        }

        accountId = userInfo->getPlatformInfo().getEaIds().getNucleusAccountId();
        personaId = userInfo->getId();

        if (gCurrentUserSession == nullptr || gCurrentUserSession->getAccountId() != accountId)
        {
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLIENT_WRITE_OTHER_USER_ENTITLEMENTS))
            {
                if (getPeerInfo() != nullptr && getPeerInfo()->isClient())
                {
                    LocaleTokenCreateLocalityString(localeStr, getPeerInfo()->getLocale());
                    LocaleTokenCreateCountryString(countryStr, getPeerInfo()->getCountry()); // connection/session country is not locale-related, but re-using locale macro for convenience
                    sb << "IP(" << getPeerInfo()->getRealPeerAddress().toString(addrStr, Login::MAX_IPADDRESS_LEN)
                        << "), ServiceName(" << getPeerInfo()->getServiceName() << "), Locale("
                        << localeStr << "), Country(" << countryStr << ") ClientType(" << ClientTypeToString(getPeerInfo()->getClientType()) << "), ";
                    clientInfo = getPeerInfo()->getClientInfo();
                    if (clientInfo != nullptr)
                    {
                        sb << "\nClientInfo:" << clientInfo;
                    }
                }

                ERR_LOG("[GrantEntitlement2Command].execute(): Permission check failed for " << sb.get() << "when attempting to grant entitlement tag[" << mRequest.getEntitlementTag()
                    << "], type[" <<Blaze::Nucleus::EntitlementType::CodeToString(mRequest.getEntitlementType()) << "], productId[" << mRequest.getProductId() << "], projectId[" << mRequest.getProjectId() << "]. No permission for user accountId["
                    << (gCurrentUserSession != nullptr ? gCurrentUserSession->getAccountId() : INVALID_ACCOUNT_ID)
                    << "] to grant entitlement for id[" << mRequest.getBlazeId() << "], failed with error[ERR_AUTHORIZATION_REQUIRED]");
                error = Blaze::ERR_AUTHORIZATION_REQUIRED;
            }
            else
                hasPermission = true;
        }
    }
    else if (gCurrentUserSession != nullptr)
    {
        //If nothing was specified, use the current user as the entitlement target.
        accountId = gCurrentUserSession->getAccountId();
        personaId =  gCurrentUserSession->getBlazeId();
    }
    else
    {
        gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(AUTH_ERR_INVALID_USER), RiverPoster::entitlement_grant_failed);
        return AUTH_ERR_INVALID_USER;
    }

    if (error == Blaze::ERR_OK)
    {
        UserSession::SuperUserPermissionAutoPtr ptr(hasPermission);
        GrantEntitlementUtil grantEntitlementUtil(
            gCurrentLocalUserSession != nullptr ? gCurrentLocalUserSession->getPeerInfo() : getPeerInfo(),
            &(mComponent->getNucleusBypassList()), gCurrentUserSession != nullptr ? mComponent->isBypassNucleusEnabled(gCurrentUserSession->getClientType()) : BYPASS_NONE);

        error = grantEntitlementUtil.grantEntitlement2(accountId, personaId, mRequest, mResponse, mComponent->getEntitlementSource(productName), productName);
        if (error == AUTH_ERR_LINK_PERSONA)
        {
            gUserSessionManager->sendPINErrorEvent(ErrorHelp::getErrorName(AUTH_ERR_LINK_PERSONA), RiverPoster::entitlement_grant_failed);
        }
        else if (error == ERR_COULDNT_CONNECT)
        {
            gUserSessionManager->sendPINErrorEvent(RiverPoster::ExternalSystemUnavailableErrToString(RiverPoster::NUCLEUS_SERVICE_UNAVAILABLE), RiverPoster::external_system_unavailable);
        }
    }

    return commandErrorFromBlazeError(error);
}

} // Authentication
} // Blaze
