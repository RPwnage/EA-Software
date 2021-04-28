/**************************************************************************************************/
/*! 
    \file expresslogin_command.cpp
    
    
    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

/*** Include Files ********************************************************************************/
#include "framework/blaze.h"
#include "framework/connection/outboundhttpservice.h"
#include "framework/controller/controller.h"
#include "framework/tdf/userevents_server.h"
#include "framework/tdf/usersessiontypes_server.h"
#include "framework/tdf/userdefines.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/util/hashutil.h"
#include "framework/oauth/oauthslaveimpl.h"
#include "framework/oauth/oauthutil.h"
#include "framework/oauth/accesstokenutil.h"

#include "proxycomponent/nucleusconnect/rpc/nucleusconnectslave.h"
#include "proxycomponent/nucleusconnect/tdf/nucleusconnect.h"
#include "proxycomponent/nucleusidentity/rpc/nucleusidentityslave.h"
#include "proxycomponent/nucleusidentity/tdf/nucleusidentity.h"

// Authentication
#include "authentication/authenticationimpl.h"
#include "authentication/rpc/authenticationslave/expresslogin_stub.h"

// Authentication utils
#include "authentication/util/authenticationutil.h"
#include "authentication/util/consolerenameutil.h"
#include "authentication/util/updateuserinfoutil.h"

namespace Blaze
{
namespace Authentication
{

/*** Public Methods *******************************************************************************/


/*! ***********************************************************************************************/
/*! \class ExpressLoginCommand

\brief Uses email address, password, and optional persona name to login a user.

\note ExpressLoginCommand follows these steps:
\li Fetch an ID 2.0 access token using the email and password
\li If a persona is provided, exchange the access token using the persona
\li Fetch information for the access token
\li Fetch account and Origin persona information
\li Add user session.
***************************************************************************************************/
class ExpressLoginCommand : public ExpressLoginCommandStub
{
public:

    ExpressLoginCommand(Message* message, ExpressLoginRequest* request, AuthenticationSlaveImpl* componentImpl)
      : ExpressLoginCommandStub(message, request),
        mComponent(componentImpl),
        mUpdateUserInfoUtil(*componentImpl),
        mAccountId(INVALID_ACCOUNT_ID),
        mPersonaId(INVALID_BLAZE_ID),
        mIsUnderage(false)
    {
    }

/*** Private Methods ******************************************************************************/
private:
    // Not owned memory.
    AuthenticationSlaveImpl* mComponent;

    // Owned memory
    UpdateUserInfoUtil mUpdateUserInfoUtil;
    AccountId mAccountId;
    BlazeId mPersonaId;
    bool mIsUnderage;
    eastl::string mDeviceId;
    DeviceLocality mDeviceLocality;

    bool isEmailWhitelisted()
    {
        // Check that the requested email address is white-listed
        AuthenticationConfig::StringExpressLoginEmailWhitelistList::const_iterator iter = mComponent->getConfig().getExpressLoginEmailWhitelist().begin();
        AuthenticationConfig::StringExpressLoginEmailWhitelistList::const_iterator end = mComponent->getConfig().getExpressLoginEmailWhitelist().end();
        for (; iter != end; ++iter)
        {
            const char8_t* filter = *iter;
            const char8_t* email = mRequest.getEmail();
            bool match = true;
            while (*filter != '\0' && *email != '\0')
            {
                if (*filter == '*')
                {
                    // Advance the email pointer until we find '@'
                    while (*email != '\0' && *email != '@')
                    {
                        ++email;
                    }
                    ++filter;
                }
                if (*filter != *email)
                {
                    match = false;
                    break;
                }
                if (*filter == '\0' || *email == '\0')
                    break;

                ++filter;
                ++email;
            }
            if (match && *filter == '\0' && *email == '\0')
                break;
        }

        if (iter == end)
            return false;

        return true;
    }

    BlazeRpcError doExpressLogin(ClientPlatformType clientPlatform, ClientType clientType, const char8_t* productName)
    {
        const char8_t* namespaceName = gController->getNamespaceFromPlatform(clientPlatform);

        BlazeRpcError err = ERR_SYSTEM;

        char8_t emailHash[HashUtil::SHA256_STRING_OUT];
        emailHash[0] = '\0';
        HashUtil::generateSHA256Hash(mRequest.getEmail(), strlen(mRequest.getEmail()), emailHash, HashUtil::SHA256_STRING_OUT);
        if (!mComponent->shouldBypassRateLimiter(clientType))
        {
            err = mComponent->tickAndCheckLoginRateLimit();
            if (err != Blaze::ERR_OK)
            {
                // an error occurred while the fiber was asleep, so we must exit with the error.
                WARN_LOG("[ExpressLoginCommand].doExpressLogin: tickAndCheckLoginRateLimit for email[" << emailHash << "] failed with error[" << ErrorHelp::getErrorName(err) << "]");
                return err;
            }
        }

        bool personaLogin = mRequest.getPersonaName()[0] != '\0';
        if (!personaLogin && clientType != Blaze::CLIENT_TYPE_HTTP_USER)
        {
            WARN_LOG("[ExpressLoginCommand].doExpressLogin: Non-HTTP clients must provide a persona name when using ExpressLogin. Failed with error[AUTH_ERR_FIELD_MISSING]");
            return Blaze::AUTH_ERR_FIELD_MISSING;
        }

        if (!isEmailWhitelisted())
        {
            INFO_LOG("[ExpressLoginCommand].doExpressLogin : Email is not white-listed.");
            return ERR_AUTHORIZATION_REQUIRED;
        }

        OAuth::OAuthSlaveImpl* oAuthSlave = mComponent->oAuthSlaveImpl();
        if (oAuthSlave == nullptr)
        {
            ERR_LOG("[ExpressLoginCommand].doExpressLogin: oAuthSlave is nullptr");
            return Blaze::ERR_SYSTEM;
        }

        InetAddress inetAddr = AuthenticationUtil::getRealEndUserAddr(this);
        const char8_t* addrStr = inetAddr.getIpAsString();
        const char8_t* clientId = mComponent->getBlazeServerNucleusClientId();

        // Step 1: Fetch an access token using the provided email and password
        NucleusConnect::GetAccessTokenResponse emailTokenResponse;
        NucleusConnect::GetAccessTokenResponse personaTokenResponse;
        NucleusConnect::GetAccessTokenResponse* tokenResponse = &emailTokenResponse;
        err = AuthenticationUtil::authFromOAuthError(oAuthSlave->getAccessTokenByPassword(mRequest.getEmail(), mRequest.getPassword(), emailTokenResponse, productName, inetAddr));
        if (err != ERR_OK)
        {
            WARN_LOG("[ExpressLoginCommand].doExpressLogin: getAccessTokenByPassword failed with error[" << ErrorHelp::getErrorName(err) << "]");
            if (err == AUTH_ERR_INVALID_TOKEN)
                err = AUTH_ERR_INVALID_USER; // Convert invalid token error to invalid user since we authenticate with email/pw
            return err;
        }

        // Step 2: If a persona is provided, exchange the access token for an auth code, including the persona and persona namespace
        if (personaLogin)
        {
            NucleusConnect::GetAuthCodeResponse resp;
            err = AuthenticationUtil::authFromOAuthError(oAuthSlave->getAuthCodeForPersona(mRequest.getPersonaName(), namespaceName, emailTokenResponse.getAccessToken(), productName, resp));
            if (err == ERR_OK)
            {
                char8_t* authCode = blaze_strstr(resp.getAuthCode(), "code=");
                if (authCode == nullptr)
                {
                    WARN_LOG("[ExpressLoginCommand].doExpressLogin: Auth code was not returned from Nucleus.");
                    return AUTH_ERR_INVALID_USER;
                }

                authCode += 5; // length of 'code='
                // Step 2b: Fetch a new access token using the auth code obtained above
                err = AuthenticationUtil::authFromOAuthError(oAuthSlave->getAccessTokenResponse("", authCode, productName, inetAddr, personaTokenResponse));
                if (err != ERR_OK)
                {
                    WARN_LOG("[ExpressLoginCommand].doExpressLogin: getAccessTokenResponse failed with error[" << ErrorHelp::getErrorName(err) << "]");
                    if (err == AUTH_ERR_INVALID_TOKEN)
                        err = AUTH_ERR_INVALID_USER; // Convert invalid token error to invalid user since we authenticate with email/pw
                    return err;
                }

                tokenResponse = &personaTokenResponse;
            }
            else
            {
                // Common case here is that the persona doesn't have a 1st-party external reference in Nucleus,
                // which will happen if the account is created via expressCreateAccount.
                // So we'll skip attempting to associate the token with the persona and leave it associated with just the account.
                if (err == AUTH_ERR_PERSONA_EXTREFID_REQUIRED)
                {
                    INFO_LOG("[ExpressLoginCommand].doExpressLogin: getAuthCodeForPersona failed with error[" << ErrorHelp::getErrorName(err) << "]. Access token will be associated only with the account.");
                }
                else
                {
                    ERR_LOG("[ExpressLoginCommand].doExpressLogin: getAuthCodeForPersona failed with error[" << ErrorHelp::getErrorName(err) << "]");
                    return err;
                }
            }
        }

        const char8_t* userAccessToken = tokenResponse->getAccessToken();
        eastl::string userTokenBearer(eastl::string::CtorSprintf(), "%s %s", tokenResponse->getTokenType(), userAccessToken);

        OAuth::AccessTokenUtil tokenUtil;
        err = tokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_BEARER);
        if (err != Blaze::ERR_OK)
        {
            ERR_LOG("[ExpressLoginCommand].doExpressLogin: Failed to retrieve server access token with error(" << ErrorHelp::getErrorName(err) << ")");
            return err;
        }
        const char8_t* serverAccessToken = tokenUtil.getAccessToken();

        // Step 3: Get the token info
        NucleusConnect::GetTokenInfoRequest tokenInfoRequest;
        tokenInfoRequest.setAccessToken(userAccessToken);
        tokenInfoRequest.setIpAddress(addrStr);
        tokenInfoRequest.setClientId(clientId);
        
        NucleusConnect::JwtPayloadInfo jwtPayloadInfo;
        NucleusConnect::GetTokenInfoResponse tokenInfoResponse;
        err = AuthenticationUtil::authFromOAuthError(oAuthSlave->getTokenInfo(tokenInfoRequest, tokenInfoResponse, jwtPayloadInfo, clientPlatform));
        if (err != Blaze::ERR_OK)
        {
            WARN_LOG("[ExpressLoginCommand].doExpressLogin: getTokenInfo failed");
            return err;
        }

        BlazeId pidId = INVALID_BLAZE_ID;
        blaze_str2int(tokenInfoResponse.getPidId(), &pidId);
        mAccountId = INVALID_ACCOUNT_ID;
        blaze_str2int(tokenInfoResponse.getAccountId(), &mAccountId);
        mPersonaId = tokenInfoResponse.getPersonaId();
        if (mPersonaId == INVALID_BLAZE_ID)
        {
            if (personaLogin)
            {
                // We get here if we failed to associate the access token with the requested persona
                // because the persona doesn't have an external reference
                // So look up the persona manually
                NucleusIdentity::GetPersonaListRequest personaListRequest;
                NucleusIdentity::GetPersonaListResponse personaListResponse;
                personaListRequest.getAuthCredentials().setAccessToken(serverAccessToken);
                personaListRequest.getAuthCredentials().setUserAccessToken(userAccessToken);
                personaListRequest.getAuthCredentials().setClientId(clientId);
                personaListRequest.getAuthCredentials().setIpAddress(addrStr);
                personaListRequest.setPid(mAccountId);
                personaListRequest.setExpandResults("true");
                personaListRequest.getFilter().setDisplayName(mRequest.getPersonaName());
                personaListRequest.getFilter().setNamespaceName(namespaceName);
                personaListRequest.getFilter().setStatus("ACTIVE");

                err = AuthenticationUtil::authFromOAuthError(oAuthSlave->getPersonaList(personaListRequest, personaListResponse, &jwtPayloadInfo));
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[ExpressLoginCommand].doExpressLogin: getPersonaList failed with error(" << ErrorHelp::getErrorName(err) << ")");
                    return err;
                }

                if (personaListResponse.getPersonas().getPersona().empty())
                {
                    ERR_LOG("[ExpressLoginCommand].doExpressLogin: personaListResponse list is empty");
                    return err;
                }
                mPersonaId = personaListResponse.getPersonas().getPersona()[0]->getPersonaId();
            }
            else
                mPersonaId = gUserSessionManager->allocateNegativeId();
        }

        mDeviceId = tokenInfoResponse.getDeviceId();
        mDeviceLocality = DEVICELOCALITY_UNKNOWN;
        int8_t deviceLocalityInt = 0;
        blaze_str2int(tokenInfoResponse.getConnectionType(), &deviceLocalityInt);
        mDeviceLocality = static_cast<DeviceLocality>(deviceLocalityInt);
        if (mDeviceLocality == DEVICELOCALITY_UNKNOWN) 
        {
            WARN_LOG("[ExpressLoginCommand].doExpressLogin: Device Locality is UNKNOWN.");
        }
        
        LogContextOverride contextOverride(mPersonaId, mDeviceId.c_str(), mRequest.getPersonaName(), getPeerInfo()->getRealPeerAddress(), mAccountId, clientPlatform);

        // Step 4: get the full account info
        NucleusIdentity::GetAccountRequest accountRequest;
        NucleusIdentity::GetAccountResponse accountResponse;
        //for better compatibility with old version Nucleus API, do not use dual token on getAccount call
        accountRequest.getAuthCredentials().setAccessToken(userTokenBearer.c_str());
        accountRequest.getAuthCredentials().setClientId(clientId);
        accountRequest.getAuthCredentials().setIpAddress(addrStr);

        err = AuthenticationUtil::authFromOAuthError(oAuthSlave->getAccount(accountRequest, accountResponse, &jwtPayloadInfo, clientPlatform));
        if (err != Blaze::ERR_OK)
        {
            ERR_LOG("[ExpressLoginCommand].doExpressLogin: getAccount failed with error(" << ErrorHelp::getErrorName(err) << ")");
            return err;
        }

        // The nexus accounts are messed up, and so the ExternalRefValue may now be an XBL account name, and the XBL id. (Or Nucleus AccountId)
        ExternalId externalId = (clientPlatform == nx ? INVALID_EXTERNAL_ID : EA::StdC::AtoU64(accountResponse.getPid().getExternalRefValue()));
        const char8_t* externalString = (clientPlatform == nx ? accountResponse.getPid().getExternalRefValue() : nullptr);

        // TODO_MC: Ask Jesse Labate why this is necessary for the PIN changes.  It messes up various lookup functions when dealing with guest sessions.
        //if (externalId == accountResponse.getPid().getPidId())
        //    externalId = 0;

        mIsUnderage = accountResponse.getPid().getUnderagePid();
        mResponse.setIsUnderage(accountResponse.getPid().getUnderagePid());
        mResponse.setIsOfLegalContactAge(!accountResponse.getPid().getUnderagePid());
        mResponse.setIsAnonymous(accountResponse.getPid().getAnonymousPid());

        // Step 5: Find the Origin namespace persona for this account (used in the loginUserSession below)
        NucleusIdentity::GetPersonaListRequest personaListRequest;
        NucleusIdentity::GetPersonaListResponse personaListResponse;
        personaListRequest.getAuthCredentials().setAccessToken(serverAccessToken);
        personaListRequest.getAuthCredentials().setUserAccessToken(userAccessToken);
        personaListRequest.getAuthCredentials().setClientId(clientId);
        personaListRequest.getAuthCredentials().setIpAddress(addrStr);
        personaListRequest.setPid(mAccountId);
        personaListRequest.setExpandResults("true");
        personaListRequest.getFilter().setNamespaceName(NAMESPACE_ORIGIN);
        personaListRequest.getFilter().setStatus("ACTIVE");

        err = AuthenticationUtil::authFromOAuthError(oAuthSlave->getPersonaList(personaListRequest, personaListResponse, &jwtPayloadInfo));
        if (err != Blaze::ERR_OK)
        {
            ERR_LOG("[ExpressLoginCommand].doExpressLogin: getPersonaList failed with error(" << ErrorHelp::getErrorName(err) << ")");
            return err;
        }

        OriginPersonaId originPersonaId = INVALID_BLAZE_ID;
        const char8_t* originPersonaName = "";
        if (!personaListResponse.getPersonas().getPersona().empty())
        {
            originPersonaId = personaListResponse.getPersonas().getPersona()[0]->getPersonaId();
            originPersonaName = personaListResponse.getPersonas().getPersona()[0]->getDisplayName();
        }

        if (clientPlatform == nx && personaLogin && originPersonaId == INVALID_BLAZE_ID)
        {
            ERR_LOG("[ExpressLoginCommand].doExpressLogin: Rejecting Switch login; Origin account is required but no active origin personas exist for accountId:" << mAccountId);
            return AUTH_ERR_INVALID_TOKEN;
        }

        // Step 6: Fill in all the required information to login the user, and log them in.
        int64_t lastAuthenticated = TimeValue::getTimeOfDay().getSec();
        UserInfoData userData;
        userData.setId(mPersonaId);

        // This code sets the (fake) external id for Mock pc users. 
        // This is the second step after setting the platform of the Mock user in utilslaveimpl.cpp.
        // NOTE:  Since ExpressLogin is mostly deprecated, if you're in here we can assume that you're either a pc, someone using mock services.
        //   In which case, we set the external id (for all currently mocked platforms)
        if (gController->getUseMockConsoleServices() && (clientPlatform == xone || clientPlatform == xbsx || clientPlatform == ps4 || clientPlatform == ps5))
        {
            // External id matching the account id means that this was a PC user (or a pc login at least)
            // For compatiblity with the old mock design, we use BlazeId for the externalId, rather than the account id. 
            externalId = (ExternalId)userData.getId();
        }

        convertToPlatformInfo(userData.getPlatformInfo(), personaLogin ? externalId : INVALID_EXTERNAL_ID, personaLogin ? externalString : nullptr, mAccountId, originPersonaId, originPersonaName, clientPlatform);

        // fetch settings from Player Settings Service
        NucleusIdentity::GetPlayerSettingsRequest playerSettingsRequest;
        NucleusIdentity::GetPlayerSettingsResponse playerSettingsResponse;
        playerSettingsRequest.getAuthCredentials().setAccessToken(serverAccessToken);
        playerSettingsRequest.getAuthCredentials().setUserAccessToken(userAccessToken);
        playerSettingsRequest.setName(mComponent->getConfig().getPssSchemaName());
        playerSettingsRequest.setAccountId(mAccountId);

        err = AuthenticationUtil::authFromOAuthError(oAuthSlave->getPlayerSettings(playerSettingsRequest, playerSettingsResponse));
        if (err != Blaze::ERR_OK)
        {
            WARN_LOG("[ExpressLoginCommand].doExpressLogin: getPlayerSettings failed with error(" << ErrorHelp::getErrorName(err) << ") -- using defaults");
        }

        userData.setPidId(pidId);
        userData.setAccountLocale(LocaleTokenCreateFromDelimitedString(accountResponse.getPid().getLocale()));
        userData.setAccountCountry(LocaleTokenGetShortFromString(accountResponse.getPid().getCountry())); // account country is not locale-related, but re-using locale macro for convenience
        userData.setOriginPersonaId(originPersonaId);
        userData.setLastLoginDateTime(lastAuthenticated);
        userData.setLastLocaleUsed(getPeerInfo()->getLocale());
        userData.setLastCountryUsed(getPeerInfo()->getCountry());
        userData.setLastAuthErrorCode(Blaze::ERR_OK);
        userData.setIsChildAccount(accountResponse.getPid().getUnderagePid());
        userData.setVoipDisabled(playerSettingsResponse.getVoipDisabled());
        bool updateCrossPlatformOpt = mRequest.getCrossPlatformOpt() != DEFAULT;
        if (updateCrossPlatformOpt)
            userData.setCrossPlatformOptIn(mRequest.getCrossPlatformOpt() == OPTIN);

        if (clientPlatform == nx)
        {
            // Switch persona names are not unique, so we use the Origin persona name on the nx platform
            userData.setPersonaName(originPersonaName);
            userData.setPersonaNamespace(NAMESPACE_ORIGIN);
        }
        else
        {
            userData.setPersonaName(mRequest.getPersonaName());
            userData.setPersonaNamespace(namespaceName);
        }

        const char8_t* ipAsString = AuthenticationUtil::getRealEndUserAddr(this).getIpAsString();

        //Make the call to register the session
        bool geoIpSucceeded = true;
        UserInfoDbCalls::UpsertUserInfoResults upsertUserInfoResults(userData.getAccountCountry());
        err = gUserSessionMaster->createNormalUserSession(userData, *getPeerInfo(), clientType, getConnectionUserIndex(), productName, true,
            tokenInfoResponse.getIpGeoLocation(), geoIpSucceeded, upsertUserInfoResults, false, updateCrossPlatformOpt, tokenInfoResponse.getDeviceId(), mDeviceLocality, tokenResponse, ipAsString);
        if (err == ERR_MOVED)
        {
            // ERR_MOVED means the client was redirected to another coreSlave because we're not yet ready to own a UserSession, and basically, nothing has really happened.
            return err;
        }

        if (err == USER_ERR_EXISTS)
        {
            //we hit a conflict in user info table which means that existing entries are outdated -- update them
            Blaze::Authentication::ConsoleRenameUtil consoleRenameUtil(getPeerInfo());
            err = consoleRenameUtil.doRename(userData, upsertUserInfoResults, updateCrossPlatformOpt, true);
            if (err == ERR_OK)
            {
                err = gUserSessionMaster->createNormalUserSession(userData, *getPeerInfo(), clientType, getConnectionUserIndex(), productName, true,
                    tokenInfoResponse.getIpGeoLocation(), geoIpSucceeded, upsertUserInfoResults, true, updateCrossPlatformOpt, tokenInfoResponse.getDeviceId(), mDeviceLocality, tokenResponse, ipAsString);
            }
        }

        if (err == Blaze::ERR_OK)
        {
            if (gCurrentLocalUserSession == nullptr)
            {
                BLAZE_ASSERT_LOG(Log::USER, "ExpressLoginCommand.doExpressLogin: gUserSessionMaster->createNormalUserSession() returned ERR_OK, but the gCurrentLocalUserSession is not set, which should never happen");
                return ERR_SYSTEM;
            }

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
            userLoginInfo.getPersonaDetails().setPersonaId(gCurrentLocalUserSession->getBlazeId());
            userLoginInfo.setGeoIpSucceeded(geoIpSucceeded);
            if (personaLogin)
            {
                userLoginInfo.getPersonaDetails().setDisplayName(userInfo.getPersonaName());
                userLoginInfo.getPersonaDetails().setExtId(getExternalIdFromPlatformInfo(userData.getPlatformInfo()));  
                mResponse.getUserLoginInfo().getPersonaDetails().setLastAuthenticated(static_cast<uint32_t>(lastAuthenticated));
            }
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

            ERR_LOG("[ExpressLoginCommand].doExpressLogin: loginUserSession() failed with error[" << ErrorHelp::getErrorName(err) << "]");
        }

        return err;
    }

    ExpressLoginCommandStub::Errors execute() override
    {
        if (!mComponent->isTrustedSource(this) && !getPeerInfo()->getTrustedClient())
        {
            INFO_LOG("[ExpressLoginCommand].execute : failed with ERR_AUTHORIZATION_REQUIRED.  (" << getPeerInfo()->getRealPeerAddress() << ") is not a trusted source.");
            return ERR_AUTHORIZATION_REQUIRED;
        }

        if (mComponent->areExpressCommandsDisabled(getCommandNameForLogging(), getPeerInfo(), mRequest.getEmail()))
        {
            // See areExpressCommandsDisabled for the log that is generated
            return ERR_SYSTEM;
        }

        const char8_t* serviceName = getPeerInfo()->getServiceName();

        const char8_t* productName = mRequest.getProductName();
        if (productName[0] == '\0') // productName is optional. If not specified, use the default of the service. 
            productName = gController->getServiceProductName(serviceName);

        if (!mComponent->isValidProductName(productName))
        {
            ERR_LOG("[ExpressLoginCommand].execute: Invalid product name " << productName << " specified in request.");
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

        if (servicePlatform == common)
        {
            INFO_LOG("[ExpressLoginCommand].execute: ExpressLogin command used without a real platform. The service(" << serviceName << ") does not belong to a real platform. PersonaName (" << mRequest.getPersonaName() << ") will not have an entry in any userinfo DB table.");
        }

        ClientType clientType = mRequest.getClientType();
        if (clientType == CLIENT_TYPE_INVALID)
            clientType = getPeerInfo()->getClientType();

        const char8_t* clientVersion = "";
        const char8_t* sdkVersion = "";
        if (getPeerInfo() != nullptr && getPeerInfo()->getClientInfo() != nullptr)
        {
            const ClientPlatformType clientInfoPlatform = getPeerInfo()->getClientInfo()->getPlatform();
            TRACE_LOG("[ExpressLoginCommand].execute : Client's platform (" << ClientPlatformTypeToString(clientInfoPlatform) << "), service's platform(" << ClientPlatformTypeToString(servicePlatform) << ").");
            
            if (clientInfoPlatform != servicePlatform)
            {
                WARN_LOG("[ExpressLoginCommand].execute : Client's platform (" << ClientPlatformTypeToString(clientInfoPlatform) << ") via preAuth "
                    << "does not match service's platform(" << ClientPlatformTypeToString(servicePlatform) << "). Override with service's platform.");

                getPeerInfo()->getClientInfo()->setPlatform(servicePlatform);
            }

            clientVersion = getPeerInfo()->getClientInfo()->getClientVersion();
            sdkVersion = getPeerInfo()->getClientInfo()->getBlazeSDKVersion();
        }

        const char8_t* namespaceName = gController->getNamespaceFromPlatform(servicePlatform);

        if (!mComponent->isBelowPsuLimitForClientType(clientType))
        {
            INFO_LOG("[ExpressLoginCommand].execute : PSU limit of " << mComponent->getPsuLimitForClientType(clientType) << " for client type "
                << ClientTypeToString(clientType) << " has been reached.");
            return AUTH_ERR_EXCEEDS_PSU_LIMIT;
        }

        BlazeRpcError err = doExpressLogin(servicePlatform, clientType, productName);
        if (err == ERR_MOVED)
        {
            // ERR_MOVED means the client was redirected to another coreSlave because we're not yet ready to own a UserSession, and basically, nothing has really happened.
            return commandErrorFromBlazeError(err);
        }

        if (mPersonaId != INVALID_BLAZE_ID && !UserSessionManager::isStatelessUser(mPersonaId))
        {
            mUpdateUserInfoUtil.updateLocaleAndErrorByBlazeId(mPersonaId, getPeerInfo()->getLocale(), getPeerInfo()->getCountry(), err);
        }
        else if(mRequest.getPersonaName()[0] != '\0')
        {
            mUpdateUserInfoUtil.updateLocaleAndErrorByPersonaName(servicePlatform, namespaceName, mRequest.getPersonaName(), getPeerInfo()->getLocale(), getPeerInfo()->getCountry(), err);
        }

        if (err != Blaze::ERR_OK)
        {
            mComponent->getMetricsInfoObj()->mLoginFailures.increment(1, productName, clientType);
            char8_t addrStr[Login::MAX_IPADDRESS_LEN] = {0};
            getPeerInfo()->getRealPeerAddress().toString(addrStr, sizeof(addrStr));

            GeoLocationData geoIpData;
            gUserSessionManager->getGeoIpData(getPeerInfo()->getRealPeerAddress(), geoIpData);

            PlatformInfo platformInfo;
            convertToPlatformInfo(platformInfo, INVALID_EXTERNAL_ID, nullptr, mAccountId, servicePlatform);
            gUserSessionMaster->generateLoginEvent(err, getUserSessionId(), USER_SESSION_NORMAL, platformInfo, mDeviceId.c_str(), mDeviceLocality,
               mPersonaId, mRequest.getPersonaName(), getPeerInfo()->getLocale(), getPeerInfo()->getCountry(), sdkVersion, clientType,
               addrStr, serviceName, productName, geoIpData, mComponent->getProjectId(productName), LOCALE_BLANK, 0, clientVersion, false, mIsUnderage);
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

};
DEFINE_EXPRESSLOGIN_CREATE()

} // Authentication
} // Blaze
