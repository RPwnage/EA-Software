/**************************************************************************************************/
/*! 
    \file loginstatemachineconsole.cpp
    
    
    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

// Include files
#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/internal/loginmanager/loginstatemachineconsole.h"
#include "BlazeSDK/internal/loginmanager/loginmanagerimpl.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"
#include "BlazeSDK/component/authenticationcomponent.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "BlazeSDK/blazesdkdefs.h"
#include "BlazeSDK/util/utilapi.h" // for UtilAPI in doNucleusLogin

// Note: NX does not support the LoginStateInitConsole login flow (NetConnControl('tick') is not implemented), but this file still needs to build
#if defined(EA_PLATFORM_LINUX) || defined(EA_PLATFORM_IPHONE) || defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_NX) || defined(EA_PLATFORM_OSX)
    #define strtok_s strtok_r
#endif

namespace Blaze
{
namespace LoginManager
{

using namespace Authentication;

LoginStateInitConsole::LoginStateInitConsole(LoginStateMachine& loginStateMachine, LoginManagerImpl& loginManager, MemoryGroupId memGroupId)
    : LoginStateInit(loginStateMachine, loginManager, memGroupId),
      mLoginInitStarted(false),
      mTokenReq(nullptr),
      mIdlerState(IDLER_STATE_NONE),
      mLoginMode(LOGIN_MODE_NONE),
      mRefreshedTicket(false)
{
}

#if defined(EA_PLATFORM_NX)  || defined(EA_PLATFORM_STADIA) || !defined(BLAZESDK_CONSOLE_PLATFORM)
void LoginStateInitConsole::onRequestTicketDone(const uint8_t *ticketData, int32_t ticketSize)
{
}

BlazeError LoginStateInitConsole::setExternalLoginData(Authentication::LoginRequest &req)
{
    return ERR_OK;
}
#endif

// These methods are shared implementations for all consoles

void LoginStateInitConsole::failLogin(BlazeError error, bool isSdkError /* = false */, const char8_t* portalUrl /* = "" */)
{
    mLoginInitStarted = false;
    if (isSdkError)
        getDispatcher()->delayedDispatch("onSdkError", &LoginManagerListener::onSdkError, error);
    else
        getDispatcher()->delayedDispatch("onLoginFailure", &LoginManagerListener::onLoginFailure, error, portalUrl);
}

void LoginStateInitConsole::startConsoleLogin()
{
    if (!getBlazeHub()->getConnectionManager()->isConnected())
    {
        failLogin(SDK_ERR_NOT_CONNECTED, true);
        return;
    }
    mLoginRequest.setAuthCode("");
    mLoginRequest.setProductName(mProductName);
    mLoginRequest.getPlatformInfo().setClientPlatform(getBlazeHub()->getClientPlatformType());
    mLoginRequest.setCrossPlatformOpt(mCrossPlatformOpt);


#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_GDK)
    // Set the URN of the token. This is used only on Xbox One.
    const char8_t *xblTokenUrn;
    bool tokenExist = getBlazeHub()->getConnectionManager()->getServerConfigString(BLAZESDK_CONFIG_KEY_IDENTITY_XBL_TOKEN_URN, &xblTokenUrn);
    if(tokenExist)
    {
        NetConnControl('aurn', 0, 0, (void*)xblTokenUrn, nullptr);
    }
    else
    {
        BLAZE_SDK_DEBUGF("[LoginStateInitConsole::startConsoleLogin()] getServerConfigString returned an null ptr for tokenUrn\n");
    }
#endif

    // The idler state should already be NONE, but just incase.
    setIdlerState(IDLER_STATE_NONE);

    // reset login mode
    mLoginMode = LOGIN_MODE_NONE;
    mPrevStateHint = -1;

    // reset refreshed flag
    mRefreshedTicket = false;

    // The first authentication attempt will use the Nucleus /auth API.
    setLoginMode(LOGIN_MODE_NUCLEUS_AUTH_CODE);
}

void LoginStateInitConsole::setLoginMode(LoginMode loginMode)
{
    if (mLoginMode != loginMode)
    {
        mLoginMode = loginMode;
        switch (mLoginMode)
        {
            case LOGIN_MODE_NONE:
            {
                break;
            }
            case LOGIN_MODE_NUCLEUS_AUTH_CODE:
            {
                doRequestTicket(false);
                break;
            }
        }
    }
    else
    {
        BLAZE_SDK_DEBUGF("[LoginStateInitConsole::setLoginMode()] mLoginMode(%d) is not changed, ignoring operation\n", mLoginMode);
    }
}

void LoginStateInitConsole::setIdlerState(IdlerState idlerState)
{
    if (mIdlerState != idlerState)
    {
        mIdlerState = idlerState;
        if (mIdlerState == IDLER_STATE_NONE)
        {
            getBlazeHub()->removeIdler(this);
        }
        else
        {
            getBlazeHub()->addIdler(this, "LoginStateBaseConsole");
        }
    }
}

void LoginStateInitConsole::idle(const uint32_t currentTime, const uint32_t elapsedTime)
{
    // If the conn is bad just fail out, the connection manager will handle it from here.
    int32_t connStatus = NetConnStatus('conn', 0, nullptr, 0);
    if ((connStatus >> 24) == '-')
    {
        BLAZE_SDK_DEBUGF("[LoginStateInitConsole::idle()] NetConnStatus returned an error %d\n", connStatus);
        setIdlerState(IDLER_STATE_NONE);
    }
    else if (mLoginCancelled)
    {
        // The login has been cancelled before it could complete, so abort this attempt
        BLAZE_SDK_DEBUGF("[LoginStateInitConsole::idle()] Login attempt has been cancelled by client\n");
        mLoginInitStarted = false;
        
        mLoginCancelled = false;
        if (mTokenReq != nullptr)
        {
            ProtoHttpDestroy(mTokenReq);
            mTokenReq = nullptr;
        }

        setIdlerState(IDLER_STATE_NONE);
    }
    else
    {
        switch (getIdlerState())
        {
            case IDLER_STATE_NONE:
            {
                // Nothing to do
                break;
            }
            case IDLER_STATE_REQUESTING_TICKET:
            {
                idleRequestTicket();
                break;
            }
            case IDLER_STATE_NUCLEUS_LOGIN:
            {
                idleNucleusLogin();
                break;
            }
        }
    }
}

void LoginStateInitConsole::doRequestTicket(bool refresh)
{
    // If a refresh is requested, call NetConnControl to clear out an existing ticket.
    if (refresh)
    {
        NetConnControl('tick', mSession.mDirtySockUserIndex, 0, nullptr, nullptr);
        mRefreshedTicket = true;
    }

    // Initiate the ticket request.
    if (NetConnStatus('tick', mSession.mDirtySockUserIndex, nullptr, 0) < 0)
    {
        BLAZE_SDK_DEBUGF("[LoginStateInitConsole::doRequestTicket()] Error initiating first party ticket request for user at index %u.\n", mSession.mDirtySockUserIndex);
    }

    // Either the ticket request was initiated successfully, it is already available,
    // or an error occured. In any case, the result will be picked up in the idleRequestTicket() function.
    setIdlerState(IDLER_STATE_REQUESTING_TICKET);
}

void LoginStateInitConsole::idleRequestTicket()
{
    int32_t ticketSize = NetConnStatus('tick', mSession.mDirtySockUserIndex, nullptr, 0);
    if (ticketSize != 0)
    {
        // We're done. Either an error occurred or everything's fine, either way we
        // need to reset the idler state.
        setIdlerState(IDLER_STATE_NONE);

        if (ticketSize > 0)
        {
            // The ticket is available. Allocate a buffer to read the ticket contents from DS.
            uint8_t *ticketData = BLAZE_NEW_ARRAY(uint8_t, ticketSize, MEM_GROUP_LOGINMANAGER_TEMP, "ticketData");
            NetConnStatus('tick', mSession.mDirtySockUserIndex, ticketData, ticketSize);

            onRequestTicketDone(ticketData, ticketSize);

            BLAZE_DELETE_ARRAY(MEM_GROUP_LOGINMANAGER_TEMP, ticketData);
        }
        else
        {
            if (mRefreshedTicket)
            {
                // The ticket is not available, and an error occured.
                BLAZE_SDK_DEBUGF("[LoginStateInitConsole::idleRequestTicket()] Error while waiting for first-party ticket for user at index %u.\n", mSession.mDirtySockUserIndex);
                onRequestTicketDone(nullptr, 0);
            }
            else
            {
                // there was a problem getting a ticket last time, let's request for a new one
                doRequestTicket(true);
            }
        }
    }
    else
    {
        // Further idle()s are required for the ticket request to complete.
    }
}

void LoginStateInitConsole::doNucleusLogin(const char8_t *ticketHeader)
{
    Blaze::ConnectionManager::ConnectionManager* connMgr = getBlazeHub()->getConnectionManager();

    if (connMgr == nullptr)
    {
        BLAZE_SDK_DEBUGF("[LoginStateInitConsole::doNucleusLogin()] ConnectionManager is nullptr!\n");
        failLogin(SDK_ERR_NOT_CONNECTED);
        return;
    }

    mTokenReq = ProtoHttpCreate(PROTO_HTTP_BUFFER_SIZE);

    ProtoHttpControl(mTokenReq, 'apnd', 0, 0, (void*)ticketHeader);
    ProtoHttpControl(mTokenReq, 'rmax', 0, 0, nullptr);

    //use config overrides found in util.cfg
    Blaze::Util::UtilAPI::createAPI(*getBlazeHub());
    getBlazeHub()->getUtilAPI()->OverrideConfigs(mTokenReq, "LoginStateMachineConsole");

    const char8_t* host;
    connMgr->getServerConfigString(BLAZESDK_CONFIG_KEY_IDENTITY_CONNECT_HOST, &host);

    char8_t tokenUrl[1024];
    blaze_snzprintf(tokenUrl, sizeof(tokenUrl), "%s/connect/auth?response_type=code", host);


    for (Util::FetchConfigResponse::ConfigMap::const_iterator iter = mIdentityConfig.getConfig().begin();
         iter != mIdentityConfig.getConfig().end();
         ++iter)
    {
        char8_t buf[128];
        blaze_snzprintf(buf, sizeof(buf), "&%s=", iter->first.c_str());

        if (blaze_strcmp(iter->first.c_str(), BLAZESDK_CONFIG_KEY_IDENTITY_DISPLAY) == 0)
        {
            const BlazeHub::InitParameters& params = getBlazeHub()->getInitParams();
            const char8_t* displayUrl = nullptr;
            if (params.Override.DisplayUri[0] != '\0')
                displayUrl = params.Override.DisplayUri;
            else
                displayUrl = iter->second.c_str();

            if (displayUrl != nullptr)
                ProtoHttpUrlEncodeStrParm(tokenUrl, sizeof(tokenUrl), buf, displayUrl);
        }
        else
        {
            ProtoHttpUrlEncodeStrParm(tokenUrl, sizeof(tokenUrl), buf, iter->second.c_str());
        }
    }

    ProtoHttpUrlEncodeStrParm(tokenUrl, sizeof(tokenUrl), "&client_id=", connMgr->getClientId());
    ProtoHttpUrlEncodeStrParm(tokenUrl, sizeof(tokenUrl), "&release_type=", connMgr->getReleaseType());

#if defined EA_PLATFORM_PS4 || defined EA_PLATFORM_PS5
    const char8_t* enviStr = "prod";
    int32_t envi = NetConnStatus('envi', 0, nullptr, 0);
    if ((envi == NETCONN_PLATENV_DEV) || (envi == NETCONN_PLATENV_TEST))
    {
        enviStr = "int";
    }
    else if (envi == NETCONN_PLATENV_CERT)
    {
        enviStr = "prod_qa";
    }
#if defined EA_PLATFORM_PS4
    ProtoHttpUrlEncodeStrParm(tokenUrl, sizeof(tokenUrl), "&ps4_env=", enviStr);
#elif defined EA_PLATFORM_PS5
    ProtoHttpUrlEncodeStrParm(tokenUrl, sizeof(tokenUrl), "&ps5_env=", enviStr);
#endif
#endif

    ProtoHttpGet(mTokenReq, tokenUrl, 0);

    setIdlerState(IDLER_STATE_NUCLEUS_LOGIN);
}

void LoginStateInitConsole::idleNucleusLogin()
{
    BlazeError error = SDK_ERR_NUCLEUS_RESPONSE;
    char8_t *location = nullptr;

    ProtoHttpUpdate(mTokenReq);

    int32_t done = ProtoHttpStatus(mTokenReq, 'done', nullptr, 0);

    if (done != 0)
    {
        // We're done. Either an error occurred or everything's fine, either way we
        // need to reset the idler state.
        setIdlerState(IDLER_STATE_NONE);

        if (done > 0)
        {
            int32_t httpStatusCode = ProtoHttpStatus(mTokenReq, 'code', nullptr, 0);
            if (httpStatusCode == 300 || httpStatusCode == 302)
            {
                int32_t headersSize = ProtoHttpStatus(mTokenReq, 'head', nullptr, 0) + 1;
                char8_t *headers = BLAZE_NEW_ARRAY(char8_t, headersSize, MEM_GROUP_LOGINMANAGER_TEMP, "headers");
                ProtoHttpStatus(mTokenReq, 'htxt', headers, headersSize);

                int32_t locationSize;
                if ((locationSize = ProtoHttpGetLocationHeader(mTokenReq, headers, nullptr, 0, nullptr)) > 0)
                {
                    location =  BLAZE_NEW_ARRAY(char8_t, locationSize, MEM_GROUP_LOGINMANAGER_TEMP, "location");
                    ProtoHttpGetLocationHeader(mTokenReq, headers, location, locationSize, nullptr);

                    BLAZE_SDK_DEBUGF("LoginStateInitConsole::idleNucleusLogin(): Got 300 Redirect to (%s)\n", location);

                    UrlQueryParamMap queryStringValues(MEM_GROUP_LOGINMANAGER_TEMP, "queryStringValues");
                    parseUrlQueryString(location, queryStringValues);

                    const char8_t* redirectUrl;
                    redirectUrl = mIdentityConfig.getConfig()[BLAZESDK_CONFIG_KEY_IDENTITY_REDIRECT];
                    if (blaze_stricmp(redirectUrl, queryStringValues[""].c_str()) == 0)
                    {
                        UrlQueryParamMap::const_iterator itr = queryStringValues.find("code");
                        if (itr != queryStringValues.end())
                        {
                            mLoginInitStarted = false;
                            mLoginRequest.setAuthCode(itr->second.c_str());
                            doLogin(mLoginRequest);

                            error = ERR_OK;
                        }
                        else
                        {
                            // If we received a redirect response but not an auth code, it's most likely 
                            // because the user and/or persona is banned. In this case, Location would include 
                            // error invalid_user and error_code user_status_invalid or persona_status_invalid.
                            itr = queryStringValues.find("error");
                            if (itr != queryStringValues.end() && blaze_stricmp(itr->second.c_str(), "invalid_user") == 0)
                            {
                                UrlQueryParamMap::const_iterator errCodeItr = queryStringValues.find("error_code");
                                if (errCodeItr != queryStringValues.end() && blaze_stricmp(errCodeItr->second.c_str(), "user_underage") == 0)
                                    error = AUTH_ERR_TOO_YOUNG;
                                else
                                    error = AUTH_ERR_INVALID_USER;

                                // Leave error as SDK_ERR_NUCLEUS_RESPONSE if we didn't find an auth code for a different reason.
                            }
                        }
                    }
                    else
                    {
                        error = AUTH_ERR_PERSONA_NOT_FOUND;
                    }
                }

                BLAZE_DELETE_ARRAY(MEM_GROUP_LOGINMANAGER_TEMP, headers);
            }
            else
            {
                BLAZE_SDK_DEBUGF("[LoginStateInitConsole::idleNucleusLogin()] Got unexpected response from Nucleus 2.0.  Response code (%" PRIi32 ")\n", httpStatusCode);
                char8_t recvBuf[4096];
                if (ProtoHttpRecvAll(mTokenReq, recvBuf, sizeof(recvBuf)) >= 0)
                {
                    BLAZE_SDK_DEBUGF("[LoginStateInitConsole::idleNucleusLogin()]: httprecvall -> %s\n", recvBuf);
                }

                if (!mRefreshedTicket)
                {
                    BLAZE_SDK_DEBUGF("[LoginStateInitConsole::idleNucleusLogin()] Ticket may have expired, trying again with new ticket\n");
                    doRequestTicket(true);
                    ProtoHttpDestroy(mTokenReq);
                    mTokenReq = nullptr;
                    return;
                }
            }
        }
        else
        {
            BLAZE_SDK_DEBUGF("[LoginStateInitConsole::idleNucleusLogin()] Unable to get the authorization code from Nucleus. Failed to send the request\n");
        }

        if (error != ERR_OK)
        {
            // this is bad as it does a delayed dispatch, and location gets deleted below....
            failLogin(error, false, ((mStateMachine.getUseExternalLoginFlow() && error == AUTH_ERR_PERSONA_NOT_FOUND) ? location : ""));
        }

        ProtoHttpDestroy(mTokenReq);
        mTokenReq = nullptr;
    }

    BLAZE_DELETE_ARRAY(MEM_GROUP_LOGINMANAGER_TEMP, location);
}

void LoginStateInitConsole::parseUrlQueryString(const char8_t* url, UrlQueryParamMap& queryMap)
{
    char8_t* temp = BLAZE_NEW_ARRAY(char8_t, strlen(url) + 1, MEM_GROUP_LOGINMANAGER_TEMP, "Temp");
    strcpy(temp, url);

    char8_t *name = nullptr;
    char8_t *tokPtr = nullptr;
    char8_t *tok = strtok_s(temp, "?", &tokPtr);
    if (tok != nullptr)
    {
        queryMap[""] = tok;

        tok = strtok_s(nullptr, "=", &tokPtr);
        while (tok != nullptr)
        {
            name = tok;
            tok = strtok_s(nullptr, "&", &tokPtr);
            if (tok != nullptr)
            {
                queryMap[name] = tok;
                tok = strtok_s(nullptr, "=", &tokPtr);
            }
        }
    }

    BLAZE_DELETE_ARRAY(MEM_GROUP_LOGINMANAGER_TEMP, temp);
}

void LoginStateInitConsole::onStartLoginProcess(const char8_t* productName/* = ""*/, Authentication::CrossPlatformOptSetting crossPlatformOpt/* = DEFAULT*/)
{
    if (mLoginInitStarted)
    {
        BLAZE_SDK_DEBUGF("[LoginStateInitConsole::onStartLoginProcess()] Error - login initialization already in progress\n");
        return;
    }
    else
    {
        mLoginInitStarted = true;
    }

    mLoginCancelled = false;
    mProductName[0] = '\0';
    blaze_strnzcpy(mProductName, productName, sizeof(mProductName));
    mCrossPlatformOpt = crossPlatformOpt;

    if (mIdentityConfig.getConfig().empty())
    {
        Util::UtilComponent* util = getBlazeHub()->getComponentManager()->getUtilComponent();

        if (util != nullptr)
        {
            Util::FetchClientConfigRequest req;
            req.setConfigSection(IDENTITY_CONFIG_SECTION);

            Util::UtilComponent::FetchClientConfigCb cb(this, &LoginStateInitConsole::onFetchClientConfig);
            util->fetchClientConfig(req, cb);
            return;
        }
    }

    startConsoleLogin();
}

void LoginStateInitConsole::onFetchClientConfig(const Util::FetchConfigResponse* response, BlazeError err, JobId jobId)
{
    if (err != ERR_OK)
    {
        failLogin(err);
    }
    else
    {
        response->copyInto(mIdentityConfig);
        startConsoleLogin();
    }
}

} // LoginManager
} // Blaze
