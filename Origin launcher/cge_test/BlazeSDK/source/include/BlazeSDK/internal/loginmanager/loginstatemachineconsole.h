/**************************************************************************************************/
/*! 
    \file loginstatemachineconsole.h
    
    
    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

#ifndef LOGINSTATEMACHINECONSOLE_H
#define LOGINSTATEMACHINECONSOLE_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

// Include files
#include "BlazeSDK/internal/loginmanager/loginstatemachine.h"

#include "BlazeSDK/component/util/tdf/utiltypes.h"

namespace Blaze
{
namespace LoginManager
{

/*! ***********************************************************************************************/
/*! \class LoginStateInitConsole (DEPRECATED)
    
    \brief Implementation of LoginStateInit that obtains a Blaze auth token from Nucleus without using NucleusSDK.
           (Uses DirtySDK to obtain a platform-specific auth token, then supplies that token in a /connect/auth HTTP request to Nucleus.)

           Note that this functionality is now deprecated and will be removed (LoginStateInit will replace LoginStateInitConsole in LoginStateMachineImplConsole).
           It is already unsupported on NX.
***************************************************************************************************/
class LoginStateInitConsole : public LoginStateInit
{
public:
    LoginStateInitConsole(LoginStateMachine& loginStateMachine, LoginManagerImpl& loginManager, MemoryGroupId memGroupId = MEM_GROUP_LOGINMANAGER_TEMP);

    virtual void onStartLoginProcess(const char8_t* productName = "", Authentication::CrossPlatformOptSetting crossPlatformOpt = Authentication::CrossPlatformOptSetting::DEFAULT);
    virtual void idle(const uint32_t currentTime, const uint32_t elapsedTime);

protected:
    void startConsoleLogin();
    BlazeError setExternalLoginData(Authentication::LoginRequest &req);

    void doRequestTicket(bool refresh);
    void idleRequestTicket();
    void onRequestTicketDone(const uint8_t *ticketData, int32_t ticketSize);

    void doNucleusLogin(const char8_t *ticketHeader);
    void idleNucleusLogin();
    void failLogin(BlazeError error, bool isSdkError = false, const char8_t* portalUrl = "");

    Util::FetchConfigResponse mIdentityConfig;
    Authentication::LoginRequest mLoginRequest;

private:
    typedef Blaze::hash_map<Blaze::string, Blaze::string, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> UrlQueryParamMap;
    void parseUrlQueryString(const char8_t* url, UrlQueryParamMap& queryMap);
    void onFetchClientConfig(const Util::FetchConfigResponse* response, BlazeError err, JobId jobId);

    bool mLoginInitStarted;

#if defined(EA_PLATFORM_PS4_CROSSGEN)
    static const char8_t* HEADER_STRING_XGEN;
#elif defined(EA_PLATFORM_PS4)
    static const char8_t* HEADER_STRING;
    const size_t HEADER_STRING_SIZE = 28;
#endif
#if defined(EA_PLATFORM_PS5)
    static const char8_t* HEADER_STRING;
#endif

    ProtoHttpRefT *mTokenReq;

    enum IdlerState
    {
        IDLER_STATE_NONE,
        IDLER_STATE_REQUESTING_TICKET,
        IDLER_STATE_NUCLEUS_LOGIN
    };

    IdlerState mIdlerState;
    IdlerState getIdlerState() { return mIdlerState; }
    void setIdlerState(IdlerState idlerState);

    enum LoginMode
    {
        LOGIN_MODE_NONE,
        LOGIN_MODE_NUCLEUS_AUTH_CODE
    };

    LoginMode mLoginMode;
    LoginMode getLoginMode() { return mLoginMode; }
    void setLoginMode(LoginMode loginMode);
    bool mRefreshedTicket;
};

/*! ***********************************************************************************************/
/*! \class LoginStateMachineImplConsole
    
    \brief Concrete implementation of LoginStateMachine for Consoles.
***************************************************************************************************/
class LoginStateMachineImplConsole : public LoginStateMachine
{
public:
    // Note: It's safe to use *this in the initializer list here because the states don't use the
    //       references during construction.
#ifdef EA_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable:4355) // 'this' : used in base member initializer list
#endif
    LoginStateMachineImplConsole(LoginManagerImpl& loginManager, SessionData& session, MemoryGroupId memGroupId = MEM_GROUP_LOGINMANAGER_TEMP)
    :   mLoginStateInitConsole(*this, loginManager, memGroupId),
        mLoginStateAuthenticated(*this, loginManager, memGroupId)
    {
        mStates[LOGIN_STATE_INIT]              = &mLoginStateInitConsole;
        mStates[LOGIN_STATE_AUTHENTICATED]     = &mLoginStateAuthenticated;
        
        changeState(LOGIN_STATE_INIT);
    }
#ifdef EA_COMPILER_MSVC
#pragma warning(pop)
#endif

protected:
    LoginStateInitConsole            mLoginStateInitConsole;
    LoginStateAuthenticated          mLoginStateAuthenticated;

    NON_COPYABLE(LoginStateMachineImplConsole)
};

} // LoginManager
} // Blaze

#endif // LOGINSTATEMACHINECONSOLE_H
