/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "loginmanager.h"
#include "stressinstance.h"
#include "stress.h"
#include "framework/connection/selector.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/config/config_map.h"

#include <fstream>
#include <string>

namespace Blaze
{
namespace Stress
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

static const uint64_t STRESS_ACCOUNT_ID_BASE_DEFAULT = 1000000000000LL;

/*** Public Methods ******************************************************************************/

//used in LoginManager::getLogin
Login::Login(StressConnection* connection, LoginManager* owner, uint64_t userId, ClientPlatformType platformType, bool connectToCommonServer)
  : mOwner(owner),
    mProxy(*connection)
{
    mStressConnection = connection;
    mIdent = userId;
    mPlatformType = platformType;
    blaze_snzprintf(mEmail, sizeof(mEmail), owner->getEmailFormat(mPlatformType), owner->getStartIndex() + mIdent);
    mConnectToCommonServer = connectToCommonServer;
}

BlazeRpcError Login::login()
{
    BlazeRpcError result = ERR_SYSTEM;
    eastl::string loginMethodString;
    if (mOwner->getUseStressLogin())
    {
        loginMethodString.sprintf("Login Method(Stress Login).");
    }
    else
    {
        loginMethodString.sprintf("Login Method(Nucleus Login), Auth Option(%s).", mOwner->getLoginByJwt() ? "JWT Access Token" : "Auth Code");
    }

    TimeValue serverLoginTimeTaken;
    TimeValue loginFlowStartTime = TimeValue::getTimeOfDay();

    if (mOwner->getUseStressLogin())
    {
        BLAZE_TRACE_LOG(Log::SYSTEM, "[Login].login: Doing Stress Login. Index(" << mIdent << "), Platform(" << ClientPlatformTypeToString(mPlatformType) << "), Email(" << mEmail << ").");
        result = stressLogin(serverLoginTimeTaken);
    }
    else
    {
        BLAZE_TRACE_LOG(Log::SYSTEM, "[Login].login: Doing Nucleus Login. Index(" << mIdent << "), Platform(" << ClientPlatformTypeToString(mPlatformType) << "), Email(" << mEmail << ").");
        result = nucleusLogin(serverLoginTimeTaken);
    }
    if (result != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[Login].login: Failed to login user. " <<
            "Index(" << mIdent << "), Platform(" << ClientPlatformTypeToString(mPlatformType) << "), Email(" << mEmail << "). " <<
            loginMethodString);
        return result;
    }

    BlazeId blazeId = mUserLoginInfo.getPersonaDetails().getPersonaId();
    mStressConnection->setBlazeId(blazeId);

    if (gLogContext != nullptr)
        gLogContext->set(mIdent, blazeId);

    TimeValue loginFlowTimeTaken = TimeValue::getTimeOfDay() - loginFlowStartTime;
    BLAZE_INFO_LOG(Log::SYSTEM, "[Login].login: Logged in user. " <<
        "Index(" << mIdent << "), Platform(" << ClientPlatformTypeToString(mPlatformType) << "), BlazeId(" << blazeId << "), Email(" << mEmail << "), DisplayName(" << mUserLoginInfo.getPersonaDetails().getDisplayName() << "). " <<
        loginMethodString << " Server login took " << serverLoginTimeTaken.getMillis() << " ms, login flow took " << loginFlowTimeTaken.getMillis() << " ms.");

    return result;
}

BlazeRpcError Login::stressLogin(TimeValue& serverLoginTimeTaken)
{
    BlazeRpcError result = ERR_SYSTEM;
    uint64_t startIndex = mOwner->getStartIndex();

    char8_t personaBuf[256];
    blaze_snzprintf(personaBuf, sizeof(personaBuf), mOwner->getPersonaFormat(mPlatformType), startIndex + mIdent);
    uint64_t accountId = mOwner->getStartNucleusId() + startIndex + mIdent;

    Authentication::StressLoginRequest stressLoginRequest;
    stressLoginRequest.setEmail(mEmail);
    stressLoginRequest.setPersonaName(personaBuf);
    stressLoginRequest.setAccountId(accountId);

    BLAZE_TRACE_LOG(Log::SYSTEM, "[Login].stressLogin: Starting Blaze Server Stress Login for email(" << mEmail << "), persona name(" << personaBuf << "), account id(" << accountId << ")");
    Authentication::LoginResponse loginResponse;

    TimeValue serverLoginStartTime = TimeValue::getTimeOfDay();
    result = mProxy.stressLogin(stressLoginRequest, loginResponse);
    serverLoginTimeTaken = TimeValue::getTimeOfDay() - serverLoginStartTime;

    if (result != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[Login].stressLogin: Failed to do proxy login with error(" << Blaze::ErrorHelp::getErrorName(result) << ")");
        return result;
    }

    loginResponse.getUserLoginInfo().copyInto(mUserLoginInfo);
    mOwner->adjustLoginCount(1);

    return result;
}

// Auth Code login flow:
// 1. For pc: requestOriginAccessToken -> requestServerAuthCode -> blaze login.
// 2. For consoles: requestClientAuthCode -> requestClientAccessToken -> requestServerAuthCode -> blaze login.
//
// JWT Access Token login flow:
// 1. For pc: requestOriginAccessToken -> requestClientAuthCode -> requestClientAccessToken -> blaze login.
// 2. For consoles: requestClientAuthCode -> requestClientAccessToken -> blaze login.
BlazeRpcError Login::nucleusLogin(TimeValue& serverLoginTimeTaken)
{
    BlazeRpcError result = ERR_SYSTEM;

    Authentication::LoginRequest loginRequest;
    loginRequest.getPlatformInfo().setClientPlatform(mPlatformType);
    if (mOwner->getLoginByJwt())
    {
        const char8_t* cachedToken = mOwner->getCachedJwtToken(mIdent);
        if (cachedToken != nullptr && cachedToken[0] != '\0')
        {
            loginRequest.setAccessToken(cachedToken);
        }
        else
        {
            eastl::string clientAccessToken;
            bool retValue = mOwner->retrieveClientAccessToken(mIdent, mPlatformType, mConnectToCommonServer, clientAccessToken);
            if (!retValue)
            {
                BLAZE_ERR_LOG(Log::SYSTEM, "[Login].nucleusLogin: Failed to retrieve Client Access Token");
                return result;
            }

            mOwner->cacheJwtTokenIfAllowed(mIdent, clientAccessToken.c_str());
            loginRequest.setAccessToken(clientAccessToken.c_str());
        }
    }
    else
    {
        eastl::string serverAuthCode;
        bool retValue = mOwner->retrieveServerAuthCode(mIdent, mPlatformType, mConnectToCommonServer, serverAuthCode);
        if (!retValue)
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[Login].nucleusLogin: Failed to retrieve Server Auth Code");
            return result;
        }

        loginRequest.setAuthCode(serverAuthCode.c_str());
    }

    Authentication::LoginResponse loginResponse;

    TimeValue serverLoginStartTime = TimeValue::getTimeOfDay();
    result = mProxy.login(loginRequest, loginResponse);
    serverLoginTimeTaken = TimeValue::getTimeOfDay() - serverLoginStartTime;

    if (result != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[Login].nucleusLogin: Failed to do proxy login with error(" << Blaze::ErrorHelp::getErrorName(result) << ")");
        return result;
    }

    loginResponse.getUserLoginInfo().copyInto(mUserLoginInfo);
    mOwner->adjustLoginCount(1);
    return result;
}

BlazeRpcError Login::logout()
{
    BlazeRpcError result = mProxy.logout();
    if (result == ERR_OK)
    {
        mOwner->adjustLoginCount(-1);
        BLAZE_INFO_LOG(Log::SYSTEM, "[Login].logout: Logged out user. " << 
            "Index(" << mIdent << "), Platform(" << ClientPlatformTypeToString(mPlatformType) << "), BlazeId(" << mUserLoginInfo.getPersonaDetails().getPersonaId() << "), Email(" << mEmail << "), DisplayName(" << mUserLoginInfo.getPersonaDetails().getDisplayName() << ").");
    }
    else
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[Login].logout: Failed to logout user. " <<
            "Index(" << mIdent << "), Platform(" << ClientPlatformTypeToString(mPlatformType) << "), BlazeId(" << mUserLoginInfo.getPersonaDetails().getPersonaId() << "), Email(" << mEmail << "), DisplayName(" << mUserLoginInfo.getPersonaDetails().getDisplayName() << ").");
    }

    return result;
}

LoginManager::LoginManager()
{
    mHttpConnectionManager = nullptr;
    mLoginCount = 0;
    mUseStressLogin = false;
    mLoginByJwt = false;
    mCacheJwtToken = false;
    mStartNucleusId = Blaze::INVALID_ACCOUNT_ID;
    mStartIndex = 0;
    mPortNumber = 0;
    mResponseType[0] = '\0';
    mHeader[0] = '\0';
    mHeaderType[0] = '\0';
    mAuthenticationUri[0] = '\0';
    mAuthCodePath[0] = '\0';
    mAccessTokenPath[0] = '\0';
    mPreAuthClientConfig[0] = '\0';
    mCommonServerClientId[0] = '\0';
    mCommonServerRedirectUri[0] = '\0';
}

LoginManager::~LoginManager()
{
    LoginsByInstanceId::iterator i = mLogins.begin();
    LoginsByInstanceId::iterator e = mLogins.end();
    for(; i != e; ++i)
        delete i->second;
}

bool LoginManager::initialize(const ConfigMap& config, const CmdLineArguments& arguments)
{
    const char8_t* responseType = config.getString("response-type", "code");
    const char8_t* header = config.getString("header", "Location");
    const char8_t* headerType = config.getString("header-type", "Content-Type:application/x-www-form-urlencoded");
    const char8_t* authenticationUri = config.getString("authentication-uri", "https://accounts.lt.ea.com");
    const char8_t* authCodePath = config.getString("auth-code-path", "/connect/auth");
    const char8_t* accessTokenPath = config.getString("access-token-path", "/connect/token");
    const char8_t* preAuthClientConfig = config.getString("pre-auth-client-config", "BlazeSDK");
    const char8_t* commonServerClientId = config.getString("common-server-client-id", "");
    const char8_t* commonServerRedirectUri = config.getString("common-server-redirect-uri", "");

    if (arguments.useStressLogin.empty())
    {
        mUseStressLogin = config.getBool("use-stress-login", false);
    }
    else
    {
        mUseStressLogin = (arguments.useStressLogin.compare("true") == 0);
    }
    if (arguments.loginByJwt.empty())
    {
        mLoginByJwt = config.getBool("login-by-jwt", false);
    }
    else
    {
        mLoginByJwt = (arguments.loginByJwt.compare("true") == 0);
    }
    if (arguments.cacheJwtToken.empty())
    {
        mCacheJwtToken = config.getBool("cache-jwt-token", false);
    }
    else
    {
        mCacheJwtToken = (arguments.cacheJwtToken.compare("true") == 0);
    }
    if (arguments.startIndex >= 0)
    {
        mStartIndex = arguments.startIndex;
    }
    else
    {
        mStartIndex = config.getUInt64("start-index", 0);
    }

    mStartNucleusId = config.getInt64("start-nucleus-id", STRESS_ACCOUNT_ID_BASE_DEFAULT);  //used by stress login
    mPortNumber = config.getUInt16("port-number", 443);
    blaze_strnzcpy(mResponseType, responseType, sizeof(mResponseType));
    blaze_strnzcpy(mHeader, header, sizeof(mHeader));
    blaze_strnzcpy(mHeaderType, headerType, sizeof(mHeaderType));
    blaze_strnzcpy(mAuthenticationUri, authenticationUri, sizeof(mAuthenticationUri));
    blaze_strnzcpy(mAuthCodePath, authCodePath, sizeof(mAuthCodePath));
    blaze_strnzcpy(mAccessTokenPath, accessTokenPath, sizeof(mAccessTokenPath));
    blaze_strnzcpy(mPreAuthClientConfig, preAuthClientConfig, sizeof(mPreAuthClientConfig));
    blaze_strnzcpy(mCommonServerClientId, commonServerClientId, sizeof(mCommonServerClientId));
    blaze_strnzcpy(mCommonServerRedirectUri, commonServerRedirectUri, sizeof(mCommonServerRedirectUri));

    if (!initializeAccountByPlatformConfig(config.getMap("nucleusLoginByPlatformConfig"), mNucleusLoginByPlatformConfig))
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager].initialize: Failed to initialize nucleusLoginByPlatformConfig");
        return false;
    }
    if (!initializeAccountByPlatformConfig(config.getMap("stressLoginByPlatformConfig"), mStressLoginByPlatformConfig))
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager].initialize: Failed to initialize stressLoginByPlatformConfig");
        return false;
    }

    initializeHttpConnection();
    return true;
}

bool LoginManager::initializeAccountByPlatformConfig(const ConfigMap* config, AccountByPlatformTypeConfigMap& accountByPlatformConfig)
{
    if (config == nullptr)
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager].initializeAccountByPlatformConfig: Input config is null");
        return false;
    }

    while (config->hasNext())
    {
        const char8_t* platform = config->nextKey();
        const ConfigMap* accountConfigMap = config->getMap(platform);
        if (accountConfigMap == nullptr)
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager].initializeAccountByPlatformConfig: Account config map is null for platform(" << platform << ")");
            return false;
        }

        const char8_t* serviceName = accountConfigMap->getString("service-name", "");
        const char8_t* grantType = accountConfigMap->getString("grant-type", "");
        const char8_t* emailFormat = accountConfigMap->getString("email-format", "blazepcstress%07d@ea.com");
        const char8_t* tokenFormat = accountConfigMap->getString("token-format", "");
        const char8_t* personaFormat = accountConfigMap->getString("persona-format", "blzpc%07d");
        const char8_t* password = accountConfigMap->getString("password", "loadtest1234");
        const char8_t* clientId = accountConfigMap->getString("client-id", "");
        const char8_t* clientSecret = accountConfigMap->getString("client-secret", "");
        const char8_t* redirectUri = accountConfigMap->getString("redirect-uri", "");
        const char8_t* display = accountConfigMap->getString("display", "");
        const char8_t* authSource = accountConfigMap->getString("auth-source", "");
        const char8_t* serverClientId = accountConfigMap->getString("server-client-id", "");
        const char8_t* serverredirectUri = accountConfigMap->getString("server-redirect-uri", "");

        AccountPlatformConfig accountConfig;
        blaze_strnzcpy(accountConfig.mServiceName, serviceName, sizeof(accountConfig.mServiceName));
        blaze_strnzcpy(accountConfig.mGrantType, grantType, sizeof(accountConfig.mGrantType));
        blaze_strnzcpy(accountConfig.mEmailFormat, emailFormat, sizeof(accountConfig.mEmailFormat));
        blaze_strnzcpy(accountConfig.mTokenFormat, tokenFormat, sizeof(accountConfig.mTokenFormat));
        blaze_strnzcpy(accountConfig.mPersonaFormat, personaFormat, sizeof(accountConfig.mPersonaFormat));
        blaze_strnzcpy(accountConfig.mPassword, password, sizeof(accountConfig.mPassword));
        blaze_strnzcpy(accountConfig.mClientId, clientId, sizeof(accountConfig.mClientId));
        blaze_strnzcpy(accountConfig.mClientSecret, clientSecret, sizeof(accountConfig.mClientSecret));
        blaze_strnzcpy(accountConfig.mRedirectUri, redirectUri, sizeof(accountConfig.mRedirectUri));
        blaze_strnzcpy(accountConfig.mDisplay, display, sizeof(accountConfig.mDisplay));
        blaze_strnzcpy(accountConfig.mAuthSource, authSource, sizeof(accountConfig.mAuthSource));
        blaze_strnzcpy(accountConfig.mServerClientId, serverClientId, sizeof(accountConfig.mServerClientId));
        blaze_strnzcpy(accountConfig.mServerRedirectUri, serverredirectUri, sizeof(accountConfig.mServerRedirectUri));

        ClientPlatformType platformType;
        if (!ParseClientPlatformType(platform, platformType))
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager].initializeAccountByPlatformConfig: Invalid platform type(" << platform << ")");
            return false;
        }

        accountByPlatformConfig[platformType] = accountConfig;
    }

    return true;
}

void LoginManager::initializeHttpConnection()
{
    BLAZE_INFO_LOG(Log::SYSTEM, "[LoginManager].intializeHttpConnection: Initializing the HttpConnectionManager");
    mHttpConnectionManager = BLAZE_NEW OutboundHttpConnectionManager("blazestress"); // This service name isn't actually used in our case, but is required by the ctor

    const char8_t* serviceHostname = nullptr;
    bool serviceSecure = false;
    HttpProtocolUtil::getHostnameFromConfig(mAuthenticationUri, serviceHostname, serviceSecure);
    InetAddress addr = InetAddress(serviceHostname, mPortNumber);
    mHttpConnectionManager->initialize(addr, 4, serviceSecure, false);
}

bool LoginManager::retrieveServerAuthCode(const uint64_t id, const ClientPlatformType platformType, const bool connectToCommonServer, eastl::string& serverAuthCode)
{
    bool retValue = false;
    eastl::string accessToken;

    if (platformType == pc)
    {
        retValue = requestOriginAccessToken(id, platformType, accessToken);
        if (!retValue)
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager].retrieveServerAuthCode: Failed to request Origin Access Token");
            return retValue;
        }
    }
    else
    {
        eastl::string clientAuthCode;

        retValue = requestClientAuthCode(id, platformType, "", clientAuthCode);
        if (!retValue)
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager].retrieveServerAuthCode: Failed to request Client Auth Code");
            return retValue;
        }
        retValue = requestClientAccessToken(id, platformType, clientAuthCode, accessToken);
        if (!retValue)
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager].retrieveServerAuthCode: Failed to request Client Access Token");
            return retValue;
        }
    }

    retValue = requestServerAuthCode(id, platformType, connectToCommonServer, accessToken, serverAuthCode);
    if (!retValue)
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager].retrieveServerAuthCode: Failed to request Server Auth Code");
        return retValue;
    }

    return retValue;
}

bool LoginManager::retrieveClientAccessToken(const uint64_t id, const ClientPlatformType platformType, const bool connectToCommonServer, eastl::string& clientAccessToken)
{
    bool retValue = false;
    eastl::string originAccessToken;
    eastl::string clientAuthCode;

    if (platformType == pc)
    {
        retValue = requestOriginAccessToken(id, platformType, originAccessToken);
        if (!retValue)
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager].retrieveClientAccessToken: Failed to request Origin Access Token");
            return retValue;
        }
    }
    else
    {
        originAccessToken = "";
    }

    retValue = requestClientAuthCode(id, platformType, originAccessToken, clientAuthCode);
    if (!retValue)
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager].retrieveClientAccessToken: Failed to request Client Auth Code");
        return retValue;
    }

    retValue = requestClientAccessToken(id, platformType, clientAuthCode, clientAccessToken);
    if (!retValue)
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager].retrieveClientAccessToken: Failed to request Client Access Token");
        return retValue;
    }

    return retValue;
}

//Get an auth code for the game client. Client side client id is used in the request to Nucleus API.
bool LoginManager::requestClientAuthCode(const uint64_t id, const ClientPlatformType platformType, const eastl::string& originAccessToken, eastl::string& clientAuthCode)
{
    bool retValue = false;
    BlazeRpcError err = ERR_SYSTEM;
    StressHttpResult httpResult;
    HttpParam params[4];
    uint32_t paramCount = 0;

    eastl::string strHttpHeader;
    if (platformType == pc)
    {
        //sample: "access_token: QVQxOjIuMDozLjA6MjQwOnFOazc0YVpGYXNQME12cW92b1B0VUtsbkVmd1doeTJXTzo1NDA3OnBrdHZh"
        strHttpHeader.sprintf(mNucleusLoginByPlatformConfig[platformType].mTokenFormat, originAccessToken.c_str());
    }
    else
    {
        //sample: "xonetoken: xone_11341650600001", "ps4_ticket: ps4_21341650600001", "nsatoken: nsa_blazenxstress-0600001"
        strHttpHeader.sprintf(mNucleusLoginByPlatformConfig[platformType].mTokenFormat, mStartIndex + id);
    }
    const char8_t* httpHeaders[1];
    httpHeaders[0] = strHttpHeader.c_str();

    params[paramCount].name = "response_type";
    params[paramCount].value = mResponseType;
    params[paramCount].encodeValue = true;
    ++paramCount;
    params[paramCount].name = "client_id";
    params[paramCount].value = mNucleusLoginByPlatformConfig[platformType].mClientId;
    params[paramCount].encodeValue = true;
    ++paramCount;
    params[paramCount].name = "redirect_uri";
    params[paramCount].value = mNucleusLoginByPlatformConfig[platformType].mRedirectUri;
    params[paramCount].encodeValue = true;
    ++paramCount;
    if (strlen(mNucleusLoginByPlatformConfig[platformType].mDisplay) > 0)
    {
        params[paramCount].name = "display";
        params[paramCount].value = mNucleusLoginByPlatformConfig[platformType].mDisplay;
        params[paramCount].encodeValue = true;
        ++paramCount;
    }

    BLAZE_TRACE_LOG(Log::SYSTEM, "[LoginManager].requestClientAuthCode: Sending request to get Client Auth Code with http header(" << strHttpHeader << ")");
    TimeValue requestStartTime = TimeValue::getTimeOfDay();

    err = mHttpConnectionManager->sendRequest(HttpProtocolUtil::HTTP_GET,
        mAuthCodePath,
        params,
        paramCount,
        httpHeaders,
        sizeof(httpHeaders) / sizeof(char8_t*),
        &httpResult);

    TimeValue requestTimeTaken = TimeValue::getTimeOfDay() - requestStartTime;
    BLAZE_TRACE_LOG(Log::SYSTEM, "[LoginManager].requestClientAuthCode: Nucleus request took " << requestTimeTaken.getMillis() << " ms.");

    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager].requestClientAuthCode: Failed to send request");
        return retValue;
    }
    if (httpResult.hasError())
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager].requestClientAuthCode: Http result has Status Code(" << httpResult.getHttpStatusCode() << ")");
        return retValue;
    }

    const char8_t* code = httpResult.getCode(mHeader);
    if (code[0] == '\0')
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager].requestClientAuthCode: Empty Client Auth Code from httpResult");
        return retValue;
    }

    clientAuthCode.sprintf(code);
    BLAZE_TRACE_LOG(Log::SYSTEM, "[LoginManager].requestClientAuthCode: Retrieved Client Auth Code(" << code << ")");
    retValue = true;

    return retValue;
}

//Get an auth code for blaze server login. Server side client id is used in the request to Nucleus API.
bool LoginManager::requestServerAuthCode(const uint64_t id, const ClientPlatformType platformType, const bool connectToCommonServer, const eastl::string& accessToken, eastl::string& serverAuthCode)
{
    bool retValue = false;
    BlazeRpcError err = ERR_SYSTEM;
    StressHttpResult httpResult;
    HttpParam params[5];
    uint32_t paramCount = 0;

    eastl::string strHttpHeader;
    strHttpHeader.sprintf("access_token:%s", accessToken.c_str());
    const char8_t* httpHeaders[1];
    httpHeaders[0] = strHttpHeader.c_str();

    params[paramCount].name = "release_type";
    params[paramCount].value = "lt";
    params[paramCount].encodeValue = true;
    ++paramCount;
    params[paramCount].name = "response_type";
    params[paramCount].value = mResponseType;
    params[paramCount].encodeValue = true;
    ++paramCount;
    params[paramCount].name = "client_id";
    params[paramCount].value = connectToCommonServer ? mCommonServerClientId : mNucleusLoginByPlatformConfig[platformType].mServerClientId;
    params[paramCount].encodeValue = true;
    ++paramCount;
    params[paramCount].name = "redirect_uri";
    params[paramCount].value = connectToCommonServer ? mCommonServerRedirectUri : mNucleusLoginByPlatformConfig[platformType].mServerRedirectUri;
    params[paramCount].encodeValue = true;
    ++paramCount;
    params[paramCount].name = "authentication_source";
    params[paramCount].value = mNucleusLoginByPlatformConfig[platformType].mAuthSource;
    params[paramCount].encodeValue = true;
    ++paramCount;

    BLAZE_TRACE_LOG(Log::SYSTEM, "[LoginManager].requestServerAuthCode: Sending request to get Server Auth Code with http header(" << strHttpHeader << ")");
    TimeValue requestStartTime = TimeValue::getTimeOfDay();

    err = mHttpConnectionManager->sendRequest(HttpProtocolUtil::HTTP_GET,
        mAuthCodePath,
        params,
        paramCount,
        httpHeaders,
        sizeof(httpHeaders) / sizeof(char8_t*),
        &httpResult);

    TimeValue requestTimeTaken = TimeValue::getTimeOfDay() - requestStartTime;
    BLAZE_TRACE_LOG(Log::SYSTEM, "[LoginManager].requestServerAuthCode: Nucleus request took " << requestTimeTaken.getMillis() << " ms.");

    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager].requestServerAuthCode: Failed to send request");
        return retValue;
    }
    if (httpResult.hasError())
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager].requestServerAuthCode: Http result has Status Code(" << httpResult.getHttpStatusCode() << ")");
        return retValue;
    }

    const char8_t* code = httpResult.getCode(mHeader);
    if (code[0] == '\0')
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager].requestServerAuthCode: Empty Server Auth Code from httpResult");
        return retValue;
    }

    serverAuthCode.sprintf(code);
    BLAZE_TRACE_LOG(Log::SYSTEM, "[LoginManager].requestServerAuthCode: Retrieved Server Auth Code(" << code << ")");
    retValue = true;

    return retValue;
}

bool LoginManager::requestClientAccessToken(const uint64_t id, const ClientPlatformType platformType, const eastl::string& clientAuthCode, eastl::string& clientAccessToken)
{
    bool retValue = false;
    BlazeRpcError err = ERR_SYSTEM;
    StressHttpResult httpResult;
    HttpParam params[6];
    uint32_t paramCount = 0;

    eastl::string strHttpHeader;
    strHttpHeader.sprintf(mHeaderType);
    const char8_t *httpHeaders[1];
    httpHeaders[0] = strHttpHeader.c_str();

    params[paramCount].name = "grant_type";
    params[paramCount].value = mNucleusLoginByPlatformConfig[platformType].mGrantType;
    params[paramCount].encodeValue = true;
    ++paramCount;
    params[paramCount].name = "code";
    params[paramCount].value = clientAuthCode.c_str();
    params[paramCount].encodeValue = true;
    ++paramCount;
    params[paramCount].name = "client_id";
    params[paramCount].value = mNucleusLoginByPlatformConfig[platformType].mClientId;
    params[paramCount].encodeValue = true;
    ++paramCount;
    params[paramCount].name = "client_secret";
    params[paramCount].value = mNucleusLoginByPlatformConfig[platformType].mClientSecret;
    params[paramCount].encodeValue = true;
    ++paramCount;
    params[paramCount].name = "redirect_uri";
    params[paramCount].value = mNucleusLoginByPlatformConfig[platformType].mRedirectUri;
    params[paramCount].encodeValue = true;
    ++paramCount;
    if (mLoginByJwt)
    {
        params[paramCount].name = "token_format";
        params[paramCount].value = "JWS";
        params[paramCount].encodeValue = true;
        ++paramCount;
    }

    BLAZE_TRACE_LOG(Log::SYSTEM, "[LoginManager].requestClientAccessToken: Sending request to get Client Access Token with Client Auth Code(" << clientAuthCode << ")");
    TimeValue requestStartTime = TimeValue::getTimeOfDay();

    err = mHttpConnectionManager->sendRequest(HttpProtocolUtil::HTTP_POST,
        mAccessTokenPath,
        params,
        paramCount,
        httpHeaders,
        sizeof(httpHeaders) / sizeof(char8_t*),
        &httpResult);

    TimeValue requestTimeTaken = TimeValue::getTimeOfDay() - requestStartTime;
    BLAZE_TRACE_LOG(Log::SYSTEM, "[LoginManager].requestClientAccessToken: Nucleus request took " << requestTimeTaken.getMillis() << " ms.");

    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager].requestClientAccessToken: Failed to send request");
        return retValue;
    }
    if (httpResult.hasError())
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager].requestClientAccessToken: Http result has Status Code(" << httpResult.getHttpStatusCode() << ")");
        return retValue;
    }

    const char8_t* token = httpResult.getAccessToken();
    if (token[0] == '\0')
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager].requestClientAccessToken: Empty Client Access Token from httpResult");
        return retValue;
    }

    clientAccessToken.sprintf(token);
    BLAZE_TRACE_LOG(Log::SYSTEM, "[LoginManager].requestClientAccessToken: Retrieved Client Access Token(" << token << ")");
    retValue = true;

    return retValue;
}

bool LoginManager::requestOriginAccessToken(const uint64_t id, const ClientPlatformType platformType, eastl::string& originAccessToken)
{
    bool retValue = false;
    BlazeRpcError err = ERR_SYSTEM;
    StressHttpResult httpResult;
    HttpParam params[5];
    uint32_t paramCount = 0;

    eastl::string strHttpHeader;
    strHttpHeader.sprintf(mHeaderType);
    const char8_t* httpHeaders[1];
    httpHeaders[0] = strHttpHeader.c_str();

    eastl::string email(eastl::string::CtorSprintf(), mNucleusLoginByPlatformConfig[platformType].mEmailFormat, mStartIndex + id);
    params[paramCount].name = "grant_type";
    params[paramCount].value = "password";
    params[paramCount].encodeValue = true;
    ++paramCount;
    params[paramCount].name = "client_id";
    params[paramCount].value = "test_token_issuer";
    params[paramCount].encodeValue = true;
    ++paramCount;
    params[paramCount].name = "client_secret";
    params[paramCount].value = "test_token_issuer_secret";
    params[paramCount].encodeValue = true;
    ++paramCount;
    params[paramCount].name = "username";
    params[paramCount].value = email.c_str();
    params[paramCount].encodeValue = true;
    ++paramCount;
    params[paramCount].name = "password";
    params[paramCount].value = mNucleusLoginByPlatformConfig[platformType].mPassword;
    params[paramCount].encodeValue = true;
    ++paramCount;

    BLAZE_TRACE_LOG(Log::SYSTEM, "[LoginManager].requestOriginAccessToken: Sending request to get Origin Access Token with grant_type(password), username(" << email.c_str() << "), password(" << mNucleusLoginByPlatformConfig[platformType].mPassword << ")");
    TimeValue requestStartTime = TimeValue::getTimeOfDay();

    err = mHttpConnectionManager->sendRequest(HttpProtocolUtil::HTTP_POST,
        mAccessTokenPath,
        params,
        paramCount,
        httpHeaders,
        sizeof(httpHeaders) / sizeof(char8_t*),
        &httpResult);

    TimeValue requestTimeTaken = TimeValue::getTimeOfDay() - requestStartTime;
    BLAZE_TRACE_LOG(Log::SYSTEM, "[LoginManager].requestOriginAccessToken: Nucleus request took " << requestTimeTaken.getMillis() << " ms.");

    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager].requestOriginAccessToken: Failed to send request");
        return retValue;
    }
    if (httpResult.hasError())
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager].requestOriginAccessToken: Http result has Status Code(" << httpResult.getHttpStatusCode() << ")");
        return retValue;
    }

    const char8_t* token = httpResult.getAccessToken();
    if (token[0] == '\0')
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager].requestOriginAccessToken: Empty Origin Access Token from httpResult");
        return retValue;
    }

    originAccessToken.sprintf(token);
    BLAZE_TRACE_LOG(Log::SYSTEM, "[LoginManager].requestOriginAccessToken: Retrieved Origin Access Token(" << token << ")");
    retValue = true;

    return retValue;
}

Login* LoginManager::getLogin(StressConnection* connection, ClientPlatformType platformType, bool connectToCommonServer)
{
    Login* login = nullptr;
    uint64_t id = connection->getIdent();

    LoginsByInstanceId::iterator find = mLogins.find(id);
    if (find != mLogins.end())
    {
        login = find->second;
        return login;
    }

    auto& configMap = mUseStressLogin ? mStressLoginByPlatformConfig : mNucleusLoginByPlatformConfig;
    if (configMap.find(platformType) == configMap.end())
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager].getLogin: No " << (mUseStressLogin ? "stress" : "nucleus" ) << "LoginByPlatformConfig for platform(" << ClientPlatformTypeToString(platformType) << ")");
        return login;
    }

    login = BLAZE_NEW Login(connection, this, id, platformType, connectToCommonServer);
    mLogins[id] = login;
    return login;
}

void LoginManager::onDisconnected(StressConnection* conn)
{
    BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager].onDisconnected: Lost connection to server.");
}

const char8_t* LoginManager::getCachedJwtToken(const uint64_t id)
{
    JwtTokenById::const_iterator iter = mJwtTokenByIdCache.find(id);
    if (iter != mJwtTokenByIdCache.end())
        return iter->second.c_str();

    return nullptr;
}

void LoginManager::cacheJwtTokenIfAllowed(const uint64_t id, const char8_t* token)
{
    if (mCacheJwtToken)
        mJwtTokenByIdCache[id] = token;
}

LoginManager::StressHttpResult::StressHttpResult()
    : mErr(ERR_OK),
      mAuthCode(""),
      mAccessToken("")
{
}

void LoginManager::StressHttpResult::setValue(const char8_t* fullname, const char8_t* data, size_t dataLen)
{
    mAccessToken.clear();
    const char8_t* token = blaze_strstr(data, "access_token");
    if (token != nullptr)
    {
        const char8_t* startPos = token + strlen("access_token") + 2;
        bool foundStart = false;
        for (size_t index = 0; index < dataLen; ++index)
        {
            if (foundStart && startPos[index] == '"')
            {
                break;
            }
            if (!foundStart && startPos[index] == '"')
            {
                foundStart = true;
                continue;
            }
            if (foundStart)
            {
                mAccessToken.push_back(startPos[index]);
            }
        }
    }
    else
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[StressHttpResult].setValue: token is null - data value " << data);
    }
}

const char8_t* LoginManager::StressHttpResult::getCode(const char8_t* header)
{
    mAuthCode.clear();
    HttpHeaderMap::const_iterator it = getHeaderMap().find(header);
    if (it != getHeaderMap().end())
    {
        const char8_t* address = it->second.c_str();
        BLAZE_TRACE_LOG(Log::SYSTEM, "[StressHttpResult].getCode: " << it->first.c_str() << " : " << address);

        char8_t* code = blaze_strstr(address, "code=");
        if (code != nullptr)
            mAuthCode.sprintf(code + 5);    // +5 for length of code=
    }

    return mAuthCode.c_str();
}

} // namespace Stress
} // namespace Blaze

