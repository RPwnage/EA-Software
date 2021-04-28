/*************************************************************************************************/
/*!
    \file   login_command.cpp


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
#include "framework/connection/outboundhttpservice.h"
#include "framework/protocol/restprotocolutil.h"
#include "framework/util/locales.h"
#include "framework/util/shared/blazestring.h"
#include "framework/usersessions/usersession.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/tdf/userevents_server.h"
#include "framework/tdf/userdefines.h"
#include "framework/oauth/oauthslaveimpl.h"

#include "authentication/authenticationimpl.h"
#include "authentication/rpc/authenticationslave/login_stub.h"
#include "authentication/tdf/authentication.h"

#include "authentication/util/authenticationutil.h"
#include "authentication/util/checkentitlementutil.h"
#include "authentication/util/consolerenameutil.h"

#include "proxycomponent/nucleusidentity/rpc/nucleusidentityslave.h"
#include "proxycomponent/nucleusidentity/tdf/nucleusidentity.h"

#include "nucleusconnect/tdf/nucleusconnect.h"
#include "nucleusconnect/rpc/nucleusconnectslave.h"

namespace Blaze
{
namespace Authentication
{
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/


class LoginCommand : public LoginCommandStub
{
public:
    LoginCommand(Message* message, LoginRequest* request, AuthenticationSlaveImpl* componentImpl);

    /* Private methods *******************************************************************************/
private:
    LoginCommandStub::Errors execute() override;
    BlazeRpcError doLogin(ClientPlatformType clientPlatform, ClientType clientType, const char8_t* productName, AccountId& accountId, PersonaId& personaId, eastl::string& personaName, ExternalId& externalId, eastl::string& externalString);

private:
    AuthenticationSlaveImpl* mComponent;
    bool mIsUnderage;
    eastl::string mDeviceId;
    DeviceLocality mDeviceLocality;
};

DEFINE_LOGIN_CREATE()

LoginCommand::LoginCommand(Message* message, LoginRequest* request, AuthenticationSlaveImpl* componentImpl)
  : LoginCommandStub(message, request),
    mComponent(componentImpl),
    mIsUnderage(false)
{
}

LoginCommandStub::Errors LoginCommand::execute()
{
    // Backwards compatibility convert the LoginRequest into the expected format: 
    convertToPlatformInfo(mRequest.getPlatformInfo(), mRequest.getExternalId(), nullptr, INVALID_ACCOUNT_ID, INVALID);

    const char8_t* serviceName = getPeerInfo()->getServiceName();

    const char8_t* productName = mRequest.getProductName();
    if (productName[0] == '\0') // productName is optional. If not specified, use the default of the service. 
        productName = gController->getServiceProductName(serviceName);

    if (!mComponent->isValidProductName(productName))
    {
        ERR_LOG("[LoginCommand].execute: Invalid product name " << productName << " specified in request.");
        return AUTH_ERR_INVALID_FIELD;
    }

    ClientPlatformType servicePlatform = gController->getServicePlatform(serviceName);
    if (servicePlatform == common)
    {
        // If connected to the 'common' default service name (i.e. via gRPC on the EADP Service Mesh),
        // then the product name can be used to specify the service platform
        ClientPlatformType productPlatform = mComponent->getProductPlatform(productName);
        if (productPlatform != INVALID)
            servicePlatform = productPlatform;
    }

    // For login call, use the platform info from the service that user is trying to connect to.
    // This way, we can verify whether the credentials provided by the user are appropriate for the service's
    // platform.
    if (servicePlatform == common)
    {
        ERR_LOG("[LoginCommand].execute: Login command can only be used with a real platform. The service(" << serviceName << ") does not belong to a real platform.");
        return AUTH_ERR_INVALID_PLATFORM;
    }
    
    ClientType clientType = mRequest.getClientType();
    if (clientType == CLIENT_TYPE_INVALID)
        clientType = getPeerInfo()->getClientType();

    const char8_t* clientVersion = "";
    const char8_t* sdkVersion = "";
    if (getPeerInfo()->getClientInfo() != nullptr)
    {
        const ClientPlatformType clientInfoPlatform = getPeerInfo()->getClientInfo()->getPlatform();
        TRACE_LOG("[LoginCommand].execute : Client's platform (" << ClientPlatformTypeToString(clientInfoPlatform) << "), service's platform(" << ClientPlatformTypeToString(servicePlatform) << ").");
        
        if (clientInfoPlatform != servicePlatform)
        {
            WARN_LOG("[LoginCommand].execute : Client's platform (" << ClientPlatformTypeToString(clientInfoPlatform) << ") via preAuth "
                << "does not match service's platform("<< ClientPlatformTypeToString(servicePlatform) << "). Override with service's platform.");
            
            getPeerInfo()->getClientInfo()->setPlatform(servicePlatform);
        }

        clientVersion = getPeerInfo()->getClientInfo()->getClientVersion();
        sdkVersion = getPeerInfo()->getClientInfo()->getBlazeSDKVersion();
    }

    AccountId accountId = INVALID_ACCOUNT_ID;
    PersonaId personaId = INVALID_BLAZE_ID;
    ExternalId externalId = INVALID_EXTERNAL_ID;
    eastl::string externalString;
    eastl::string personaName;

    if (!mComponent->isBelowPsuLimitForClientType(clientType))
    {
        INFO_LOG("[LoginCommand].execute : PSU limit of " << mComponent->getPsuLimitForClientType(clientType) << " for client type "
            << ClientTypeToString(clientType) << " has been reached.");

        char8_t addrStr[Login::MAX_IPADDRESS_LEN] = { 0 };
        getPeerInfo()->getRealPeerAddress().toString(addrStr, sizeof(addrStr));

        PlatformInfo platformInfo;
        convertToPlatformInfo(platformInfo, externalId, externalString.c_str(), accountId, servicePlatform);

        gUserSessionMaster->sendLoginPINError(AUTH_ERR_EXCEEDS_PSU_LIMIT, getUserSessionId(), platformInfo, personaId,
            getPeerInfo()->getLocale(), getPeerInfo()->getCountry(), sdkVersion, clientType, addrStr, serviceName, productName, clientVersion, mIsUnderage);
        return AUTH_ERR_EXCEEDS_PSU_LIMIT;
    }

    BlazeRpcError err = doLogin(servicePlatform, clientType, productName, accountId, personaId, personaName, externalId, externalString);
    if (err == Blaze::ERR_OK)
    {
        mComponent->getMetricsInfoObj()->mLogins.increment(1, productName, clientType);

        char8_t lastAuth[Blaze::Authentication::MAX_DATETIME_LENGTH];
        Blaze::TimeValue curTime = Blaze::TimeValue::getTimeOfDay();
        curTime.toAccountString(lastAuth, sizeof(lastAuth));
        mComponent->getMetricsInfoObj()->setLastAuthenticated(lastAuth);
    }
    else
    {
        mComponent->getMetricsInfoObj()->mLoginFailures.increment(1, productName, clientType);

        char8_t addrStr[Login::MAX_IPADDRESS_LEN] = {0};
        getPeerInfo()->getRealPeerAddress().toString(addrStr, sizeof(addrStr));

        GeoLocationData geoIpData;
        gUserSessionManager->getGeoIpData(getPeerInfo()->getRealPeerAddress(), geoIpData);

        bool sendPINErrorEvent = (err == AUTH_ERR_INVALID_TOKEN) || (err == AUTH_ERR_EXCEEDS_PSU_LIMIT) || (err == AUTH_ERR_BANNED) || (err == ERR_COULDNT_CONNECT);
 
        PlatformInfo platformInfo;
        convertToPlatformInfo(platformInfo, externalId, externalString.c_str(), accountId, servicePlatform);
        gUserSessionMaster->generateLoginEvent(err, getUserSessionId(), USER_SESSION_NORMAL, platformInfo, mDeviceId.c_str(), mDeviceLocality, personaId, personaName.c_str(),
            getPeerInfo()->getLocale(), getPeerInfo()->getCountry(), sdkVersion, clientType, addrStr, serviceName, geoIpData, mComponent->getProjectId(productName),
            LOCALE_BLANK, 0, clientVersion, sendPINErrorEvent, mIsUnderage);
    }

    return commandErrorFromBlazeError(err);
}


BlazeRpcError LoginCommand::doLogin(ClientPlatformType clientPlatform, ClientType clientType, const char8_t* productName, AccountId& accountId, PersonaId& personaId, eastl::string& personaName, ExternalId& externalId, eastl::string& externalString)
{
    BlazeRpcError err;

    if (!mComponent->shouldBypassRateLimiter(clientType))
    {
        err = mComponent->tickAndCheckLoginRateLimit();
        if (err != Blaze::ERR_OK)
        {
            // an error occurred while the fiber was asleep, so we must exit with the error.
            WARN_LOG("[LoginCommand].doLogin(): tickAndCheckLoginRateLimit failed with error[" << ErrorHelp::getErrorName(err) << "]. Login attempts are being rate limited. (See totalLoginsPerMinute in authentication.cfg)" );
            return err;
        }
    }

    // Step 1: Get the access token from Nucleus
    NucleusConnect::GetAccessTokenResponse tokenResponse;
    InetAddress inetAddr = AuthenticationUtil::getRealEndUserAddr(this);
    const char8_t* clientId = mComponent->getBlazeServerNucleusClientId();
    err = AuthenticationUtil::authFromOAuthError(mComponent->oAuthSlaveImpl()->getAccessTokenByAuthCode(mRequest.getAuthCode(), tokenResponse, productName, inetAddr));

    if (err != Blaze::ERR_OK)
    {
        WARN_LOG("[LoginCommand].doLogin: getAccessTokenByAuthCode() failed with AuthCode(" << mRequest.getAuthCode() << "), error[" << ErrorHelp::getErrorName(err) << "]");
        return err;
    }

    const char8_t *token = tokenResponse.getAccessToken();
    const char8_t *tokenType = tokenResponse.getTokenType();

    // The token and the type need to be concatenated when passing the Authorization header to Nucleus
    eastl::string tokenBearer(eastl::string::CtorSprintf(), "%s %s", tokenType, token);

    // Step 2: Get the token info
    NucleusConnect::GetTokenInfoRequest tokenInfoRequest;
    tokenInfoRequest.setAccessToken(token);
    tokenInfoRequest.setIpAddress(inetAddr.getIpAsString());
    tokenInfoRequest.setClientId(clientId);

    NucleusConnect::GetTokenInfoResponse tokenInfoResponse;
    err = AuthenticationUtil::authFromOAuthError(mComponent->oAuthSlaveImpl()->getTokenInfo(tokenInfoRequest, tokenInfoResponse));
    if (err != Blaze::ERR_OK)
    {
        WARN_LOG("[LoginCommand].doLogin: getTokenInfo failed");
        return err;
    }

    // Validate the deviceId
    mDeviceId = tokenInfoResponse.getDeviceId();
    mDeviceLocality = DEVICELOCALITY_UNKNOWN;
    int8_t deviceLocalityInt = 0;
    blaze_str2int(tokenInfoResponse.getConnectionType(), &deviceLocalityInt);
    mDeviceLocality = static_cast<DeviceLocality>(deviceLocalityInt);
    if (mDeviceLocality == DEVICELOCALITY_UNKNOWN) 
    {
        WARN_LOG("[LoginCommand].doLogin: Device Locality is UNKNOWN.");
    }

    ProductMap::const_iterator itr = mComponent->getConfig().getProducts().find(productName);
    if (itr != mComponent->getConfig().getProducts().end())
    {
        if ((itr->second->getDeviceIdLogLevel() == LOG_ALWAYS) ||
            ((itr->second->getDeviceIdLogLevel() == LOG_WHEN_EMPTY) && mDeviceId.empty()))
        {
            INFO_LOG("[LoginCommand].doLogin: DeviceId(" << mDeviceId
                << ") for account(" << tokenInfoResponse.getAccountId()
                << "), console environment(" << tokenInfoResponse.getConsoleEnv()
                << "), token clientId(" << tokenInfoResponse.getClientId()
                << "), is underage(" << tokenInfoResponse.getIsUnderage()
                << "), stop process(" << tokenInfoResponse.getStopProcess()
                << "), clientType(" << ClientTypeToString(getPeerInfo()->getClientType())
                << "), IP(" << getPeerInfo()->getRealPeerAddress()
                << "), platform(" << ClientPlatformTypeToString(clientPlatform)
                << "), deviceType(" << mDeviceLocality << ").");
        }

        if (mDeviceId.empty() && itr->second->getRequireDeviceId())
        {
            INFO_LOG("[LoginCommand].doLogin: Login failed; DeviceId is empty.");
            return AUTH_ERR_INVALID_TOKEN;
        }

        const char8_t* authSource = itr->second->getAuthenticationSource();
        if (authSource == nullptr || authSource[0] == '\0')
        {
            ERR_LOG("[LoginCommand].doLogin: authSource not found in tokenInfo. Failing login." << tokenInfoResponse);
            return AUTH_ERR_INVALID_AUTHCODE;
        }
        
        
        if (mComponent->getConfig().getVerifyAuthSource()) // kill switch for the auth source verification functionality 
        {
            // Due to a variety of reasons, we have ended up in a lame situation. Configuration of authSource on the client wasn't a requirement until recently (Circa March 2019). If the auth source
            // was not provisioned, Nucleus returns the clientId as the auth source. Coincidentally, this is bad for the shared cluster where the clientId is same for all the platforms.
            
            // check top level
            bool authSourceMismatch = (blaze_stricmp(tokenInfoResponse.getAuthSource(), authSource) != 0);
            if (authSourceMismatch)
            {
                // check nested configuration for this platform/product
                const AccessList& accessList = itr->second->getAccess();
                for (const auto& access : accessList)
                {
                    if (blaze_stricmp(tokenInfoResponse.getAuthSource(), access->getProjectId()) == 0)
                    {
                        authSourceMismatch = false;
                        break;
                    }
                }
            }
            
            if (authSourceMismatch)
            {
                // In a shared cluster, the specification of a authenticationSource on client is a MUST (except PC Dev).
                // In single platform scenario, the we accept clientId as an authentication source.
                if (gController->isSharedCluster())
                {
                    // pc dev often does not have origin launcher support so skip it.
                    bool mismatchAllowed = (clientPlatform == pc) && (blaze_stricmp(gController->getServiceEnvironment(), "dev") == 0);
                    if (!mismatchAllowed)
                    {
                        ERR_LOG("[LoginCommand].doLogin: The authSource (" << tokenInfoResponse.getAuthSource() << ") returned by Nucleus for blaze product (" << productName << ") user is trying to log into did not match "
                            << " the configured authSource/projectId(" << authSource << "). authSource must match with the projectId for the shared cluster."
                            << "Failing login.");
                        return AUTH_ERR_INVALID_AUTHCODE;
                    }
                }
                else
                {
                    // Nucleus returns the nucleus client Id as authSource of an auth_code if the client did not specify a authSource. So we check that to support old flow.
                    if (blaze_stricmp(tokenInfoResponse.getAuthSource(), gController->getBlazeServerNucleusClientId()) != 0)
                    {
                        ERR_LOG("[LoginCommand].doLogin: In fallback path, the authSource (" << tokenInfoResponse.getAuthSource() << ") returned by Nucleus for the blaze product (" << productName << ") user is trying to log"
                            << " into did not match with the configured authSource/projectId(" << authSource << ") or " << "clientId (" << gController->getBlazeServerNucleusClientId() << "). Failing login.");
                        return AUTH_ERR_INVALID_AUTHCODE;
                    }
                }
            }
        }
    }
    else
    {
        // This should have already failed in the execute() but adding here as a safety net.
        ERR_LOG("[LoginCommand].doLogin: Invalid product name " << productName << " specified in request.");
        return AUTH_ERR_INVALID_FIELD;
    }

    const ClientTypeFlags& flags = gUserSessionManager->getClientTypeDescription(clientType);
    if (flags.getValidateSandboxId())
    {
        // Verify that the console environment used is valid: 
        err = mComponent->verifyConsoleEnv(clientPlatform, productName, tokenInfoResponse.getConsoleEnv());
        if (err != Blaze::ERR_OK)
        {
            // Error logging performed in verifyConsoleEnv
            TRACE_LOG("[LoginCommand].doLogin: An error occurred while verifying console environment (" << tokenInfoResponse.getConsoleEnv() << ").");
            return err;
        }
    }

    // Get the value of the 'StopProcess' flag on the users account.  See https://developer.ea.com/display/CI/GOPFR-4171%09%5BGDPR%5D+Subject+Access+Requests+-+Pull+and+delete+per-user+records+design+spec
    const bool stopProcess = (blaze_stricmp(tokenInfoResponse.getStopProcess(), "on") == 0);

    AccountId pidId = INVALID_BLAZE_ID;
    blaze_str2int(tokenInfoResponse.getPidId(), &pidId);
    blaze_str2int(tokenInfoResponse.getAccountId(), &accountId);

    NucleusIdentity::IdentityError identityError;
    NucleusIdentity::NucleusIdentitySlave* identComp = (NucleusIdentity::NucleusIdentitySlave*)gOutboundHttpService->getService(NucleusIdentity::NucleusIdentitySlave::COMPONENT_INFO.name);
    if (identComp == nullptr)
    {
        ERR_LOG("[LoginCommand].doLogin: nucleusconnect is nullptr");
        return ERR_SYSTEM;
    }

    // Step 3: Check online access entitlements
    if (clientPlatform == steam)
    {
        TRACE_LOG("[LoginCommand].doLogin: calling getRefreshExternalEntitlements for accountId(" << accountId << ") and personaId (" << tokenInfoResponse.getPersonaId() << ").");
        
        NucleusIdentity::GetRefreshExternalEntitlementsRequest req;
        NucleusIdentity::GetRefreshExternalEntitlementsResponse resp;
        req.setPidId(pidId);
        req.getAuthCredentials().setAccessToken(tokenBearer.c_str());
        
        // GOSOPS-198001:
        // This call times out at 7s on C&I's end when talking to Steam. After 7s, C&I may end up sending us 200 response without actually refreshing the entitlements in steam (the
        // current entitlements will be returned). This means we can run into a very transient error if this is the first time the user is logging in and the call to Steam takes over
        // 7s.
        err = identComp->getRefreshExternalEntitlements(req, resp, identityError);

        if (err != Blaze::ERR_OK)
        {
            err = AuthenticationUtil::parseIdentityError(identityError, err);
            ERR_LOG("[LoginCommand].doLogin: getRefreshExternalEntitlements failed with error(" << ErrorHelp::getErrorName(err) << ")");
            return err;
        }
    }

    CheckEntitlementUtil checkEntUtil(*mComponent, clientType);
    err = mComponent->commonEntitlementCheck(accountId, checkEntUtil, clientType, productName, this, tokenBearer.c_str(), clientId);
    if (err != Blaze::ERR_OK)
    {
        ERR_LOG("[LoginCommand].doLogin: commonEntitlementCheck failed with error (" << ErrorHelp::getErrorName(err) << ") for accountId(" << accountId 
            << ") on platform(" << Blaze::ClientPlatformTypeToString(clientPlatform) << "). Soft check(" << checkEntUtil.getSoftCheck() << ").");
        
        if (!checkEntUtil.getSoftCheck())
            return err;
    }
    

    const char8_t* addrStr = inetAddr.getIpAsString();
    
    // Step 4: get the full account info
    NucleusIdentity::GetAccountRequest accountRequest;
    NucleusIdentity::GetAccountResponse accountResponse;
    accountRequest.getAuthCredentials().setAccessToken(tokenBearer.c_str());
    accountRequest.getAuthCredentials().setClientId(clientId);
    accountRequest.getAuthCredentials().setIpAddress(addrStr);

    err = identComp->getAccount(accountRequest, accountResponse, identityError);
    if (err != Blaze::ERR_OK)
    {
        err = AuthenticationUtil::parseIdentityError(identityError, err);
        ERR_LOG("[LoginCommand].doLogin: getAccount failed with error(" << ErrorHelp::getErrorName(err) << ")");
        return err;
    }

    if (clientPlatform == nx)
        externalString = accountResponse.getPid().getExternalRefValue();
    else
        externalId = EA::StdC::AtoU64(accountResponse.getPid().getExternalRefValue());

    // Step 5: Find the Origin namespace persona for this account (used in the loginUserSession below)
    NucleusIdentity::GetPersonaListRequest personaListRequest;
    NucleusIdentity::GetPersonaListResponse personaListResponse;
    personaListRequest.getAuthCredentials().setAccessToken(tokenBearer.c_str());
    personaListRequest.getAuthCredentials().setClientId(clientId);
    personaListRequest.getAuthCredentials().setIpAddress(addrStr);
    personaListRequest.setPid(pidId);
    personaListRequest.setExpandResults("true");
    personaListRequest.getFilter().setNamespaceName(NAMESPACE_ORIGIN);
    personaListRequest.getFilter().setStatus("ACTIVE");

    err = identComp->getPersonaList(personaListRequest, personaListResponse, identityError);
    if (err != Blaze::ERR_OK)
    {
        err = AuthenticationUtil::parseIdentityError(identityError, err);
        ERR_LOG("[LoginCommand].doLogin: getPersonaList failed with error(" << ErrorHelp::getErrorName(err) << ")");
        return err;
    }

    OriginPersonaId originPersonaId = INVALID_BLAZE_ID;
    const char8_t* originPersonaName = nullptr;
    personaId = tokenInfoResponse.getPersonaId();
    if (!personaListResponse.getPersonas().getPersona().empty())
    {
        originPersonaId = personaListResponse.getPersonas().getPersona()[0]->getPersonaId();
        originPersonaName = personaListResponse.getPersonas().getPersona()[0]->getDisplayName();
    }

    // Step 5.5: Get personaId for PC platform (steam/epic) as persona_id returned in getTokenInfo is null; Get external refernce id as well since pc platform games doesn't return it in the GetAccount api call.
    if (personaId == INVALID_BLAZE_ID && clientPlatform == steam)
    {
        //reusing personaListRequest but setting namespace from client platform
        personaListRequest.getFilter().setNamespaceName(gController->getNamespaceFromPlatform(clientPlatform));
        NucleusIdentity::GetPersonaListResponse filteredPersonaListResponse;
        err = identComp->getPersonaList(personaListRequest, filteredPersonaListResponse, identityError);

        if (err != Blaze::ERR_OK)
        {
            err = AuthenticationUtil::parseIdentityError(identityError, err);
            ERR_LOG("[LoginCommand].doLogin: getPersonaList failed with error(" << ErrorHelp::getErrorName(err) << ")");
            return err;
        }

        if (!filteredPersonaListResponse.getPersonas().getPersona().empty())
        {
            personaId = filteredPersonaListResponse.getPersonas().getPersona()[0]->getPersonaId();
        }

        NucleusIdentity::GetExternalRefRequest externalRefRequest;
        NucleusIdentity::GetExternalRefResponse externalRefResponse;
        externalRefRequest.getAuthCredentials().setAccessToken(tokenBearer.c_str());
        externalRefRequest.getAuthCredentials().setClientId(clientId);
        externalRefRequest.getAuthCredentials().setIpAddress(addrStr);
        externalRefRequest.setExpandResults("true");
        externalRefRequest.setPersonaId(personaId);

        err = identComp->getExternalRef(externalRefRequest, externalRefResponse, identityError);

        if (err != Blaze::ERR_OK)
        {
            err = AuthenticationUtil::parseIdentityError(identityError, err);
            ERR_LOG("[LoginCommand].doLogin: getExternalRef failed with error(" << ErrorHelp::getErrorName(err) << ")");
            return err;
        }
        else
        {
            if(externalRefResponse.getPersonaReference().size() > 0 )
                externalId = EA::StdC::AtoU64(externalRefResponse.getPersonaReference().front()->getReferenceValue());
            else
            {
                ERR_LOG("[LoginCommand].doLogin: getExternalRef failed to get external reference id");
                return ERR_SYSTEM;
            }
        }
    }

    // Step 6: Get the info for the persona associated with the access token and set response info
    NucleusIdentity::GetPersonaInfoRequest personaInfoRequest;
    NucleusIdentity::GetPersonaInfoResponse personaInfoResponse;
    const char8_t* personaNamespace = nullptr;
    if (personaId != INVALID_BLAZE_ID && clientPlatform != nx) // Switch persona names are not unique, so we use the Origin persona name on the nx platform
    {
        personaInfoRequest.getAuthCredentials().setAccessToken(tokenBearer.c_str());
        personaInfoRequest.getAuthCredentials().setClientId(clientId);
        personaInfoRequest.getAuthCredentials().setIpAddress(addrStr);
        personaInfoRequest.setPid(pidId);
        personaInfoRequest.setPersonaId(personaId);

        err = identComp->getPersonaInfo(personaInfoRequest, personaInfoResponse, identityError);
        if (err != Blaze::ERR_OK)
        {
            err = AuthenticationUtil::parseIdentityError(identityError, err);
            ERR_LOG("[LoginCommand].doLogin: getPersonaInfo failed with error(" << ErrorHelp::getErrorName(err) << ")");
            return err;
        }

        personaName = personaInfoResponse.getPersona().getDisplayName();
        personaNamespace = personaInfoResponse.getPersona().getNamespaceName();

        // check that the persona tied to the token is in the same namespace as this server
        // is configured for to ensure there's no namespace mismatch
        const char8_t* serviceNamespace = (clientPlatform != INVALID) ? gController->getNamespaceFromPlatform(clientPlatform) : "";
        if (clientPlatform == INVALID || blaze_stricmp(serviceNamespace, personaNamespace) != 0)
        {
            WARN_LOG("[LoginCommand].doLogin: Persona namespace(" << personaNamespace << ") does not match clientPlatform (" << ClientPlatformTypeToString(clientPlatform) << ")'s expected namespace ("<< serviceNamespace <<").");
            return AUTH_ERR_INVALID_TOKEN;
        }
    }
    else
    {
        // check that this is an origin login enabled server, otherwise, token is invalid if
        // it doesn't contain personaId
        if (personaId == INVALID_BLAZE_ID && blaze_stricmp(gController->getNamespaceFromPlatform(clientPlatform), NAMESPACE_ORIGIN) != 0)
        {
            WARN_LOG("[LoginCommand].doLogin: Server is not configured for Origin namespace, but personaId was not specified in token.");
            return AUTH_ERR_INVALID_TOKEN;
        }

        if (originPersonaId == static_cast<OriginPersonaId>(INVALID_BLAZE_ID))
        {
            WARN_LOG("[LoginCommand].doLogin: Rejecting " << (clientPlatform == nx ? "Switch login; Origin account is required but" : "Origin login; token does not contain a valid persona and" )
                << " no active origin personas exist for accountId:" << accountId);
            return AUTH_ERR_INVALID_TOKEN;
        }

        personaNamespace = NAMESPACE_ORIGIN;
        if (personaId == INVALID_BLAZE_ID)
            personaId = originPersonaId;
        personaName = originPersonaName;
    }

    LogContextOverride contextOverride(personaId, mDeviceId.c_str(), personaName.c_str(), getPeerInfo()->getRealPeerAddress(), accountId, clientPlatform);

    mIsUnderage = accountResponse.getPid().getUnderagePid();
    mResponse.setIsUnderage(accountResponse.getPid().getUnderagePid());
    mResponse.setIsOfLegalContactAge(!accountResponse.getPid().getUnderagePid());
    mResponse.setIsAnonymous(accountResponse.getPid().getAnonymousPid());

    // Step 7: Fill in all the required information to login the user, and log them in.

    // Adding warning logging for users that are possibly hackers so that game teams can further investigate.  
    // Essentially we want to validate, and warn otherwise:
    // 1) XOne: the externalId passed up in the request TDF matches the value returned from Nucleus for this account 
    //          (except on ps4, where we fetch externalId from Nucleus only and don't send it from the client).
    // 2) ExternalId returned from Nucleus is valid (all platforms).
    bool checkClientExternalIdMatchesNucleus = ((clientPlatform == ClientPlatformType::xone) || (clientPlatform == ClientPlatformType::xbsx)) && flags.getCheckExternalId() &&
        !mComponent->oAuthSlaveImpl()->isMock();

    // Only checking Xbl here, since that's all that's available:
    if ((checkClientExternalIdMatchesNucleus && mRequest.getPlatformInfo().getExternalIds().getXblAccountId() != externalId) || externalId == INVALID_EXTERNAL_ID)
    {
        WARN_LOG("[LoginCommand].doLogin: Possible hacker/fraudulent account! XblId(" << externalId << ") returned by Nucleus vs tdf request XblId(" << mRequest.getPlatformInfo().getExternalIds().getXblAccountId()
            << ") for accountId[" << accountId << "], persona[" << personaName << "], IP address[" << addrStr << "]");
    }

    // Copy the returned external value into the PlatformInfo, before copying that into the UserInfoData:
    convertToPlatformInfo(mRequest.getPlatformInfo(), externalId, externalString.c_str(), accountId, originPersonaId, originPersonaName, clientPlatform);

    // fetch settings from Player Settings Service
    NucleusIdentity::GetPlayerSettingsRequest playerSettingsRequest;
    NucleusIdentity::GetPlayerSettingsResponse playerSettingsResponse;
    playerSettingsRequest.getAuthCredentials().setAccessToken(tokenBearer.c_str());
    playerSettingsRequest.setName(mComponent->getConfig().getPssSchemaName());
    playerSettingsRequest.setAccountId(accountId);

    err = identComp->getPlayerSettings(playerSettingsRequest, playerSettingsResponse, identityError);
    if (err != Blaze::ERR_OK)
    {
        err = AuthenticationUtil::parseIdentityError(identityError, err);
        WARN_LOG("[LoginCommand].doLogin: getPlayerSettings failed with error(" << ErrorHelp::getErrorName(err) << ") -- using defaults");
    }

    // Step 8: Fill in all the required information to login the user, and log them in.
    UserInfoData userData;
    mRequest.getPlatformInfo().copyInto(userData.getPlatformInfo());
    userData.setId(personaId);
    userData.setPidId(pidId);
    userData.setPersonaName(personaName.c_str());
    userData.setAccountLocale(LocaleTokenCreateFromDelimitedString(accountResponse.getPid().getLocale()));
    userData.setAccountCountry(LocaleTokenGetShortFromString(accountResponse.getPid().getCountry())); // account country is not locale-related, but re-using locale macro for convenience
    userData.setOriginPersonaId(originPersonaId);
    userData.setLastLoginDateTime(TimeValue::getTimeOfDay().getSec());
    userData.setLastLocaleUsed(getPeerInfo()->getLocale());
    userData.setLastCountryUsed(getPeerInfo()->getCountry());
    userData.setLastAuthErrorCode(Blaze::ERR_OK);
    userData.setIsChildAccount(accountResponse.getPid().getUnderagePid());
    userData.setStopProcess(stopProcess);
    userData.setVoipDisabled(playerSettingsResponse.getVoipDisabled());
    userData.setPersonaNamespace(personaNamespace);
    bool updateCrossPlatformOpt = mRequest.getCrossPlatformOpt() != DEFAULT;
    if (updateCrossPlatformOpt)
        userData.setCrossPlatformOptIn(mRequest.getCrossPlatformOpt() == OPTIN);

    const char8_t* ipAsString = AuthenticationUtil::getRealEndUserAddr(this).getIpAsString();

    //Make the call to register the session
    bool geoIpSucceeded = true;
    UserInfoDbCalls::UpsertUserInfoResults upsertUserInfoResults(userData.getAccountCountry());
    err = gUserSessionMaster->createNormalUserSession(userData, *getPeerInfo(), clientType, getConnectionUserIndex(), productName, false,
        tokenInfoResponse.getIpGeoLocation(), geoIpSucceeded, upsertUserInfoResults, false, updateCrossPlatformOpt, tokenInfoResponse.getDeviceId(), mDeviceLocality, &tokenResponse, ipAsString);
    if (err == USER_ERR_EXISTS)
    {
        //we hit a conflict in user info table which means that existing entries are outdated -- update them
        Blaze::Authentication::ConsoleRenameUtil consoleRenameUtil(getPeerInfo());
        err = consoleRenameUtil.doRename(userData, upsertUserInfoResults, updateCrossPlatformOpt, true);
        if (err == ERR_OK)
        {
            err = gUserSessionMaster->createNormalUserSession(userData, *getPeerInfo(), clientType, getConnectionUserIndex(), productName, false,
                tokenInfoResponse.getIpGeoLocation(), geoIpSucceeded, upsertUserInfoResults, true, updateCrossPlatformOpt, tokenInfoResponse.getDeviceId(), mDeviceLocality, &tokenResponse, ipAsString);
        }
    }

    if (err == Blaze::ERR_OK)
    {
        UserLoginInfo& userLoginInfo = mResponse.getUserLoginInfo();
        const UserInfo& userInfo = gCurrentLocalUserSession->getUserInfo();

        userLoginInfo.setBlazeId(gCurrentLocalUserSession->getBlazeId());
        userLoginInfo.setIsFirstLogin(gCurrentLocalUserSession->getSessionFlags().getFirstLogin());
        userLoginInfo.setIsFirstConsoleLogin(gCurrentLocalUserSession->getSessionFlags().getFirstConsoleLogin());
        userLoginInfo.setLastLoginDateTime(userInfo.getPreviousLoginDateTime());
        char8_t sessionKey[MAX_SESSION_KEY_LENGTH];
        gCurrentLocalUserSession->getKey(sessionKey);
        userLoginInfo.setSessionKey(sessionKey);
        userInfo.getPlatformInfo().copyInto(userLoginInfo.getPlatformInfo());
        userLoginInfo.setAccountId(userInfo.getPlatformInfo().getEaIds().getNucleusAccountId());
        userLoginInfo.getPersonaDetails().setDisplayName(userInfo.getPersonaName());
        userLoginInfo.getPersonaDetails().setExtId(getExternalIdFromPlatformInfo(userData.getPlatformInfo()));
        userLoginInfo.getPersonaDetails().setPersonaId(gCurrentLocalUserSession->getBlazeId());
        userLoginInfo.setGeoIpSucceeded(geoIpSucceeded);
    }
    else
    {
        if (err == ERR_DUPLICATE_LOGIN)
        {
            err = AUTH_ERR_DUPLICATE_LOGIN;
        }
        else if (err == ERR_NOT_PRIMARY_PERSONA)
        {
            err = AUTH_ERR_NOT_PRIMARY_PERSONA;
        }
        ERR_LOG("[LoginCommand].doLogin: loginUserSession() failed with error[" << ErrorHelp::getErrorName(err) << "]");
    }

    return err;
}

} // Authentication
} // Blaze
