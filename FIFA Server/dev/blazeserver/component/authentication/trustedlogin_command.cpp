/*************************************************************************************************/
/*!
    \file   trustedlogin_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class LoginCommand

    Sends Nucleus an email and password for authentication.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/util/shared/blazestring.h"
#include "framework/usersessions/usersession.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/tdf/userevents_server.h"
#include "framework/oauth/oauthslaveimpl.h"

#include "authentication/authenticationimpl.h"
#include "authentication/tdf/authentication.h"
#include "authentication/rpc/authenticationslave/trustedlogin_stub.h"
#include "authentication/util/authenticationutil.h"

#include "nucleusconnect/tdf/nucleusconnect.h"
#include "nucleusconnect/rpc/nucleusconnectslave.h"

namespace Blaze
{
namespace Authentication
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/


class TrustedLoginCommand : public TrustedLoginCommandStub
{
public:
    TrustedLoginCommand(Message* message, TrustedLoginRequest* request, AuthenticationSlaveImpl* componentImpl);
    BlazeRpcError processToken(const char8_t* accessToken, const ClientPlatformType clientPlatform, eastl::string& tokenScopes, NucleusIdentity::IpGeoLocation& ipGeoLocation);

    ~TrustedLoginCommand() override {};

private:
    AuthenticationSlaveImpl* mComponent;
    BlazeId mPersonaId;

    TrustedLoginCommandStub::Errors execute() override;
    BlazeRpcError doLogin(ClientPlatformType clientPlatform, ClientType clientType, const char8_t* productName);

    /* Private methods *******************************************************************************/
};

DEFINE_TRUSTEDLOGIN_CREATE()

TrustedLoginCommand::TrustedLoginCommand(
    Message* message, TrustedLoginRequest* request, AuthenticationSlaveImpl* componentImpl)
    : TrustedLoginCommandStub(message, request),
    mComponent(componentImpl),
    mPersonaId(INVALID_BLAZE_ID)
{
}

BlazeRpcError TrustedLoginCommand::processToken(const char8_t* accessToken, const ClientPlatformType clientPlatform, eastl::string& tokenScopes, NucleusIdentity::IpGeoLocation& ipGeoLocation)
{
    OAuth::OAuthSlaveImpl* oAuthSlave = mComponent->oAuthSlaveImpl();
    if (oAuthSlave == nullptr)
    {
        ERR_LOG("[TrustedLoginCommand].processToken: oAuthSlave is nullptr");
        return Blaze::ERR_SYSTEM;
    }

    Blaze::OAuth::CachedTrustedToken trustedToken;
    if (oAuthSlave->getCachedTrustedToken(accessToken, trustedToken))
    {
        tokenScopes.assign(trustedToken.getScopes());
        trustedToken.getIpGeoLocation().copyInto(ipGeoLocation);

        return ERR_OK;
    }

    // This token is not in cache. Check with Nucleus.
    NucleusConnect::GetTokenInfoRequest tokenInfoRequest;
    tokenInfoRequest.setAccessToken(accessToken);
    tokenInfoRequest.setIpAddress(OAuth::OAuthUtil::getRealEndUserAddr(this).getIpAsString());
    tokenInfoRequest.setClientId(gController->getBlazeServerNucleusClientId());

    NucleusConnect::GetTokenInfoResponse tokenInfoResponse;
    NucleusConnect::JwtPayloadInfo jwtPayloadInfo;
    BlazeRpcError err = AuthenticationUtil::authFromOAuthError(oAuthSlave->getTokenInfo(tokenInfoRequest, tokenInfoResponse, jwtPayloadInfo, clientPlatform));
    if (err == ERR_OK)
    {
        // Fill in the return data.
        tokenScopes.assign(tokenInfoResponse.getScope());
        tokenInfoResponse.getIpGeoLocation().copyInto(ipGeoLocation);
        
        // Seeing the validated token for the first time. Cache it.
        oAuthSlave->cacheTrustedToken(accessToken, tokenInfoResponse);
    }
    else if (err == ERR_COULDNT_CONNECT)
    {
        return err;
    }
    else
    {
        WARN_LOG("[TrustedLoginCommand].processToken: token is most likely expired or not valid for some other reason. Client need to retry after getting a new one from Nucleus.");
        return AUTH_ERR_INVALID_TOKEN;
    }

    return err;
}

BlazeRpcError TrustedLoginCommand::doLogin(ClientPlatformType clientPlatform, ClientType clientType, const char8_t* productName)
{
    // Parse out the token from the Authorization header (e.g., "Authorization: NEXUS_S2S <token>")
    // Check that it starts with "NEXUS_S2S "
    const char8_t* method = "NEXUS_S2S ";
    const uint32_t methodLen = strlen(method);
    const uint32_t tokenLen = strlen(mRequest.getAccessToken());
    if (tokenLen <= methodLen)
    {
        ERR_LOG("[TrustedLoginCommand].execute: Access token is not long enough.");
        return AUTH_ERR_INVALID_TOKEN;
    }

    if (blaze_strncmp(mRequest.getAccessToken(), method, methodLen) != 0)
    {
        ERR_LOG("[TrustedLoginCommand].execute: Access token does not start with NEXUS_S2S.");
        return AUTH_ERR_INVALID_TOKEN;
    }

    char8_t* saveptr = nullptr;
    blaze_strtok((char8_t*)mRequest.getAccessToken(), " ", &saveptr);
    const char8_t* token = blaze_strtok(nullptr, " ", &saveptr);

    if (token == nullptr)
    {
        ERR_LOG("[TrustedLoginCommand].execute: Unable to parse valid access token. This should never happen!");
        return AUTH_ERR_INVALID_TOKEN;
    }

    eastl::string tokenScopes;
    NucleusIdentity::IpGeoLocation ipGeoLocation;
    BlazeRpcError err = processToken(token, clientPlatform, tokenScopes, ipGeoLocation);

    if (err == ERR_OK)
    {
        // We have got to a point where token is valid. Let's check if they have permission to create a trusted user session
        if (!mComponent->isTrustedSource(this) && !getPeerInfo()->getTrustedClient())
        {
            ERR_LOG("[TrustedLoginCommand:" << this << "].doLogin: Peer is not a mTLS trusted client (verify that a client cert was sent AND had correct hash AND ssl context configuration is set to verify peer), " <<
                "and (" << AuthenticationUtil::getRealEndUserAddr(this).getIpAsString()
                << ") could not be trusted based on IP whitelisting. Failing trusted Login.");
            return ERR_AUTHORIZATION_REQUIRED;
        }

        UserInfoData userData;
        userData.setId(gUserSessionManager->allocateNegativeId());

        convertToPlatformInfo(userData.getPlatformInfo(), INVALID_EXTERNAL_ID, nullptr, INVALID_ACCOUNT_ID, clientPlatform);

        userData.setPersonaName(mRequest.getId());

        bool geoIpSucceeded = true;
        err = gUserSessionMaster->createTrustedUserSession(userData, *getPeerInfo(), clientType, getConnectionUserIndex(), productName, tokenScopes.c_str(), ipGeoLocation, geoIpSucceeded);
        if (err == ERR_MOVED)
        {
            // ERR_MOVED means the client was redirected to another coreSlave because we're not yet ready to own a UserSession, and basically, nothing has really happened.
            return err;
        }

        if (err == Blaze::ERR_OK)
        {
            UserLoginInfo& userLoginInfo = mResponse.getUserLoginInfo();

            userLoginInfo.setBlazeId(gCurrentLocalUserSession->getBlazeId());
            userLoginInfo.setGeoIpSucceeded(geoIpSucceeded);
            char8_t sessionKey[MAX_SESSION_KEY_LENGTH];
            gCurrentLocalUserSession->getKey(sessionKey);
            userLoginInfo.setSessionKey(sessionKey);
        }
        else
        {
            ERR_LOG("[TrustedLoginCommand].execute: Failed to login usersession for " << mRequest.getIdType() << " user(" << mRequest.getId()
                << ") with error(" << ErrorHelp::getErrorName(err) << ").");
        }
    }
    else
        TRACE_LOG("[TrustedLoginCommand].execute: Failed to login " << mRequest.getIdType() << " user(" << mRequest.getId()
            << ") with error(" << ErrorHelp::getErrorName(err) << ").");

    return err;
}

TrustedLoginCommandStub::Errors TrustedLoginCommand::execute()
{
    const char8_t* serviceName = getPeerInfo()->getServiceName();

    // productName is not specified in the request; use the default of the service. 
    const char8_t* productName = gController->getServiceProductName(serviceName);

    ClientPlatformType servicePlatform = gController->getServicePlatform(serviceName);

    ClientType clientType = mRequest.getClientType();
    if (clientType == CLIENT_TYPE_INVALID)
        clientType = getPeerInfo()->getClientType();

    if (getPeerInfo()->getClientInfo() == nullptr || mRequest.getSetClientInfo())
    {
        TRACE_LOG("[TrustedLoginCommand].execute : Attempting to override client info with provided data.");
        getPeerInfo()->setClientInfo(&mRequest.getClientInfo());
    }

    const char8_t* clientVersion = "";
    const char8_t* sdkVersion = "";
    if (getPeerInfo() != nullptr && getPeerInfo()->getClientInfo() != nullptr)
    {
        const ClientPlatformType clientInfoPlatform = getPeerInfo()->getClientInfo()->getPlatform();
        TRACE_LOG("[TrustedLoginCommand].execute : Client's platform (" << ClientPlatformTypeToString(clientInfoPlatform) << "), service's platform(" << ClientPlatformTypeToString(servicePlatform) << ").");
       
        if (clientInfoPlatform != servicePlatform)
        {
            WARN_LOG("[TrustedLoginCommand].execute : Client's platform (" << ClientPlatformTypeToString(clientInfoPlatform) << ") via preAuth "
                << "does not match service's platform(" << ClientPlatformTypeToString(servicePlatform) << "). Override with service's platform.");

            getPeerInfo()->getClientInfo()->setPlatform(servicePlatform);
        }

        clientVersion = getPeerInfo()->getClientInfo()->getClientVersion();
        sdkVersion = getPeerInfo()->getClientInfo()->getBlazeSDKVersion();
    }

    if (!mComponent->isBelowPsuLimitForClientType(clientType))
    {
        INFO_LOG("[TrustedLoginCommand].execute : PSU limit of " << mComponent->getPsuLimitForClientType(clientType) << " for client type "
            << ClientTypeToString(clientType) << " has been reached.");
        return AUTH_ERR_EXCEEDS_PSU_LIMIT;
    }

    BlazeRpcError err = doLogin(servicePlatform, clientType, productName);

    if (err != Blaze::ERR_OK)
    {
        mComponent->getMetricsInfoObj()->mLoginFailures.increment(1, productName, clientType);
        char8_t addrStr[Login::MAX_IPADDRESS_LEN] = {0};
        getPeerInfo()->getRealPeerAddress().toString(addrStr, sizeof(addrStr));

        GeoLocationData geoIpData;
        gUserSessionManager->getGeoIpData(getPeerInfo()->getRealPeerAddress(), geoIpData);

        bool sendPINErrorEvent = (err == ERR_COULDNT_CONNECT) || (err == AUTH_ERR_INVALID_TOKEN);

        PlatformInfo platformInfo;
        convertToPlatformInfo(platformInfo, INVALID_EXTERNAL_ID, nullptr, INVALID_ACCOUNT_ID, servicePlatform);
        gUserSessionMaster->generateLoginEvent(err, getUserSessionId(), USER_SESSION_TRUSTED, platformInfo, "", DEVICELOCALITY_UNKNOWN,
            mPersonaId, mRequest.getId(), getPeerInfo()->getLocale(), getPeerInfo()->getCountry(), sdkVersion, clientType,
            addrStr, serviceName, productName, geoIpData, mComponent->getProjectId(productName), LOCALE_BLANK, 0, clientVersion, sendPINErrorEvent);
    }
    else
    {
        mComponent->getMetricsInfoObj()->mLogins.increment(1, productName, clientType);
        char8_t lastAuth[MAX_DATETIME_LENGTH];
        Blaze::TimeValue curTime = Blaze::TimeValue::getTimeOfDay();
        curTime.toAccountString(lastAuth, sizeof(lastAuth));
        mComponent->getMetricsInfoObj()->setLastAuthenticated(lastAuth);
    }

    return commandErrorFromBlazeError(err);
}

}
}
